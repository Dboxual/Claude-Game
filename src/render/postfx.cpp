#include "render/postfx.h"
#include "core/log.h"
#include "core/mathx.h"

// Shared fullscreen-pass vertex stage (raylib's default handles this, so
// only fragment shaders are custom).

static const char* BRIGHT_FS = R"GLSL(
#version 330
in vec2 fragTexCoord;
uniform sampler2D texture0;
out vec4 finalColor;
void main() {
    vec3 c = texture(texture0, fragTexCoord).rgb;
    // Luminance-keyed soft threshold: only the hottest pixels feed bloom.
    float lum = dot(c, vec3(0.299, 0.587, 0.114));
    float k = smoothstep(0.76, 0.95, lum);
    finalColor = vec4(c * k, 1.0);
}
)GLSL";

static const char* BLUR_FS = R"GLSL(
#version 330
in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec2 blurDir;      // (1/w, 0) or (0, 1/h)
out vec4 finalColor;
void main() {
    // 9-tap gaussian, weights sum to 1.
    float w[5] = float[](0.227027, 0.194594, 0.121621, 0.054054, 0.016216);
    vec3 acc = texture(texture0, fragTexCoord).rgb * w[0];
    for (int i = 1; i < 5; i++) {
        acc += texture(texture0, fragTexCoord + blurDir * float(i)).rgb * w[i];
        acc += texture(texture0, fragTexCoord - blurDir * float(i)).rgb * w[i];
    }
    finalColor = vec4(acc, 1.0);
}
)GLSL";

static const char* COMPOSITE_FS = R"GLSL(
#version 330
in vec2 fragTexCoord;
uniform sampler2D texture0;    // scene
uniform sampler2D bloomTex;
uniform float bloomStrength;
out vec4 finalColor;
void main() {
    vec3 c = texture(texture0, fragTexCoord).rgb;
    if (bloomStrength > 0.0)
        c += texture(bloomTex, fragTexCoord).rgb * bloomStrength;

    // Grade: gentle contrast S-curve, saturation lift, warm tilt — the
    // PS2-fantasy "storybook" finish.
    c = mix(c, c * c * (3.0 - 2.0 * c), 0.30);
    float lum = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(lum), c, 1.12);
    c *= vec3(1.02, 1.0, 0.985);

    // Vignette, subtle.
    vec2 uv = fragTexCoord - 0.5;
    c *= 1.0 - dot(uv, uv) * 0.55;

    finalColor = vec4(c, 1.0);
}
)GLSL";

void PostFx::Init(float renderScale) {
    brightShader = LoadShaderFromMemory(nullptr, BRIGHT_FS);
    blurShader = LoadShaderFromMemory(nullptr, BLUR_FS);
    compositeShader = LoadShaderFromMemory(nullptr, COMPOSITE_FS);
    if (!brightShader.id || !blurShader.id || !compositeShader.id)
        LOG_ERROR("PostFx shader failed to compile");
    locBlurDir = GetShaderLocation(blurShader, "blurDir");
    locBloomStrength = GetShaderLocation(compositeShader, "bloomStrength");
    locBloomTexture = GetShaderLocation(compositeShader, "bloomTex");
    EnsureTargets(renderScale);
}

void PostFx::Shutdown() {
    UnloadRenderTexture(scene);
    UnloadRenderTexture(bright);
    UnloadRenderTexture(blurA);
    UnloadRenderTexture(blurB);
    UnloadShader(brightShader);
    UnloadShader(blurShader);
    UnloadShader(compositeShader);
}

void PostFx::EnsureTargets(float renderScale) {
    int w = (int)(GetScreenWidth() * renderScale);
    int h = (int)(GetScreenHeight() * renderScale);
    if (w == width && h == height) return;
    if (scene.id) {
        UnloadRenderTexture(scene);
        UnloadRenderTexture(bright);
        UnloadRenderTexture(blurA);
        UnloadRenderTexture(blurB);
    }
    width = w; height = h; scale = renderScale;
    scene = LoadRenderTexture(w, h);
    bright = LoadRenderTexture(w / 4, h / 4);
    blurA = LoadRenderTexture(w / 4, h / 4);
    blurB = LoadRenderTexture(w / 4, h / 4);
    SetTextureFilter(scene.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(bright.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(blurA.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(blurB.texture, TEXTURE_FILTER_BILINEAR);
    LOG_INFO("PostFx targets %dx%d (scale %.2f)", w, h, renderScale);
}

void PostFx::BeginScene() {
    BeginTextureMode(scene);
}

// Draws src into dst through a shader (RT sources are Y-flipped, hence the
// negative source height).
static void Pass(RenderTexture2D& dst, Texture2D src, Shader sh) {
    BeginTextureMode(dst);
    BeginShaderMode(sh);
    DrawTexturePro(src, { 0, 0, (float)src.width, -(float)src.height },
                   { 0, 0, (float)dst.texture.width, (float)dst.texture.height },
                   { 0, 0 }, 0, WHITE);
    EndShaderMode();
    EndTextureMode();
}

void PostFx::Composite(bool bloomEnabled) {
    EndTextureMode();   // close the scene RT

    float strength = 0.0f;
    if (bloomEnabled) {
        strength = 0.7f;
        Pass(bright, scene.texture, brightShader);

        Vector2 dirH = { 1.0f / bright.texture.width, 0 };
        Vector2 dirV = { 0, 1.0f / bright.texture.height };
        SetShaderValue(blurShader, locBlurDir, &dirH, SHADER_UNIFORM_VEC2);
        Pass(blurA, bright.texture, blurShader);
        SetShaderValue(blurShader, locBlurDir, &dirV, SHADER_UNIFORM_VEC2);
        Pass(blurB, blurA.texture, blurShader);
        // Second blur round widens the glow pleasantly.
        SetShaderValue(blurShader, locBlurDir, &dirH, SHADER_UNIFORM_VEC2);
        Pass(blurA, blurB.texture, blurShader);
        SetShaderValue(blurShader, locBlurDir, &dirV, SHADER_UNIFORM_VEC2);
        Pass(blurB, blurA.texture, blurShader);
    }

    if (strength != lastBloomStrength) {
        SetShaderValue(compositeShader, locBloomStrength, &strength, SHADER_UNIFORM_FLOAT);
        lastBloomStrength = strength;
    }
    BeginShaderMode(compositeShader);
    if (bloomEnabled)
        SetShaderValueTexture(compositeShader, locBloomTexture, blurB.texture);
    DrawTexturePro(scene.texture, { 0, 0, (float)width, (float)-height },
                   { 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() },
                   { 0, 0 }, 0, WHITE);
    EndShaderMode();
}
