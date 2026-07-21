#include "render/sky.h"
#include "core/log.h"
#include "raymath.h"
#include "rlgl.h"

static const char* SKY_VS = R"GLSL(
#version 330
in vec3 vertexPosition;
uniform mat4 mvp;
out vec3 fragDir;
void main() {
    fragDir = vertexPosition;
    // .xyww pins depth to the far plane; with depth writes off the dome can
    // never occlude world geometry.
    gl_Position = (mvp * vec4(vertexPosition, 1.0)).xyww;
}
)GLSL";

static const char* SKY_FS = R"GLSL(
#version 330
in vec3 fragDir;
uniform vec3 sunDir;
uniform vec3 zenithColor;
uniform vec3 horizonColor;
uniform vec3 sunColor;
uniform float time;
out vec4 finalColor;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}
float vnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1, 0)), u.x),
               mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), u.x), u.y);
}

void main() {
    vec3 d = normalize(fragDir);
    vec3 sky = mix(horizonColor, zenithColor, pow(max(d.y, 0.0), 0.55));
    if (d.y < 0.0) sky = mix(sky, horizonColor * 0.92, clamp(-d.y * 10.0, 0.0, 1.0));

    // Sun: tight disc plus a wide warm halo.
    float s = max(dot(d, -sunDir), 0.0);
    sky += sunColor * (smoothstep(0.9997, 0.99985, s) * 1.6 + pow(s, 24.0) * 0.22);

    // Two octaves of drifting value noise, projected onto a virtual plane,
    // fading out at the horizon.
    if (d.y > 0.015) {
        vec2 uv = d.xz / (d.y + 0.18) * 0.55 + vec2(time * 0.006, time * 0.0025);
        float n = vnoise(uv * 1.6) * 0.62 + vnoise(uv * 3.7 + 17.0) * 0.38;
        float cloud = smoothstep(0.56, 0.78, n) * clamp(d.y * 3.5, 0.0, 1.0);
        vec3 cloudCol = mix(vec3(1.0, 0.985, 0.95), sunColor, 0.25);
        sky = mix(sky, cloudCol, cloud * 0.5);
    }
    finalColor = vec4(sky, 1.0);
}
)GLSL";

void SkyRenderer::Init() {
    shader = LoadShaderFromMemory(SKY_VS, SKY_FS);
    if (shader.id == 0) LOG_ERROR("Sky shader failed to compile");
    locSunDir   = GetShaderLocation(shader, "sunDir");
    locZenith   = GetShaderLocation(shader, "zenithColor");
    locHorizon  = GetShaderLocation(shader, "horizonColor");
    locSunColor = GetShaderLocation(shader, "sunColor");
    locTime     = GetShaderLocation(shader, "time");

    dome = GenMeshSphere(1.0f, 12, 24);
    mat = LoadMaterialDefault();
    mat.shader = shader;
}

void SkyRenderer::Shutdown() {
    UnloadMesh(dome);
    // mat.shader is ours; free the material's default maps but not twice.
    mat.shader = {};
    UnloadMaterial(mat);
    UnloadShader(shader);
}

static bool Same(Vector3 a, Vector3 b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

void SkyRenderer::Draw(const Camera3D& cam, Vector3 sunDir, float time) {
    if (!uniformCacheValid || !Same(sunDir, cachedSunDir)) {
        Vector3 normalized = Vector3Normalize(sunDir);
        SetShaderValue(shader, locSunDir, &normalized, SHADER_UNIFORM_VEC3);
        cachedSunDir = sunDir;
    }
    if (!uniformCacheValid || !Same(zenithColor, cachedZenith)) {
        SetShaderValue(shader, locZenith, &zenithColor, SHADER_UNIFORM_VEC3);
        cachedZenith = zenithColor;
    }
    if (!uniformCacheValid || !Same(horizonColor, cachedHorizon)) {
        SetShaderValue(shader, locHorizon, &horizonColor, SHADER_UNIFORM_VEC3);
        cachedHorizon = horizonColor;
    }
    if (!uniformCacheValid || !Same(sunColor, cachedSunColor)) {
        SetShaderValue(shader, locSunColor, &sunColor, SHADER_UNIFORM_VEC3);
        cachedSunColor = sunColor;
    }
    SetShaderValue(shader, locTime, &time, SHADER_UNIFORM_FLOAT);
    uniformCacheValid = true;

    // Dome radius must sit inside the far clip plane (raylib default 1000).
    Matrix xf = MatrixMultiply(MatrixScale(800, 800, 800),
                               MatrixTranslate(cam.position.x, cam.position.y, cam.position.z));
    rlDisableDepthMask();
    rlDisableBackfaceCulling();     // we render the sphere from inside
    DrawMesh(dome, mat, xf);
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}
