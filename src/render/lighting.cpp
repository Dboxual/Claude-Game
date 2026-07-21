#include "render/lighting.h"
#include "core/log.h"
#include "raymath.h"

static const char* VS = R"GLSL(
#version 330
in vec3 vertexPosition;
in vec3 vertexNormal;
in vec4 vertexColor;
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
out vec3 fragPos;
out vec3 fragNormal;
out vec4 fragColor;
void main() {
    fragPos = vec3(matModel * vec4(vertexPosition, 1.0));
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));
    fragColor = vertexColor;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)GLSL";

static const char* FS = R"GLSL(
#version 330
in vec3 fragPos;
in vec3 fragNormal;
in vec4 fragColor;
uniform vec4 colDiffuse;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec3 skyAmbient;
uniform vec3 groundAmbient;
uniform vec3 fogColor;
uniform float fogDensity;
uniform vec3 cameraPos;
uniform float emissive;
#define MAX_LIGHTS 8
uniform int lightCount;
uniform vec3 lightPos[MAX_LIGHTS];
uniform vec3 lightColor[MAX_LIGHTS];
uniform float lightRange[MAX_LIGHTS];
out vec4 finalColor;
void main() {
    vec3 albedo = fragColor.rgb * colDiffuse.rgb;
    vec3 n = normalize(fragNormal);

    // Hemisphere ambient: sky color from above, warm bounce from below.
    vec3 ambient = mix(groundAmbient, skyAmbient, n.y * 0.5 + 0.5);

    // Sun with a soft wrap term - keeps shadow sides readable and stylized
    // instead of pitch black.
    float ndl = dot(n, -sunDir);
    float lit = mix(max(ndl, 0.0), max((ndl + 0.35) / 1.35, 0.0), 0.4);
    vec3 light = ambient + sunColor * lit;

    for (int i = 0; i < lightCount; i++) {
        vec3 d = lightPos[i] - fragPos;
        float dist = length(d);
        float att = clamp(1.0 - dist / lightRange[i], 0.0, 1.0);
        att *= att;
        light += lightColor[i] * att * max(dot(n, d / max(dist, 0.001)), 0.0);
    }

    vec3 color = albedo * light;
    // Emissive surfaces glow past their albedo so the bloom pass can pick
    // them up; they ignore scene lighting entirely.
    color = mix(color, albedo * 1.5, emissive);

    float dist = length(fragPos - cameraPos);
    float fog = 1.0 - exp(-fogDensity * dist);
    color = mix(color, fogColor, fog);

    finalColor = vec4(color, fragColor.a * colDiffuse.a);
}
)GLSL";

void LightingSystem::Init() {
    shader = LoadShaderFromMemory(VS, FS);
    if (shader.id == 0) LOG_ERROR("Lighting shader failed to compile");

    locSunDir     = GetShaderLocation(shader, "sunDir");
    locSunColor   = GetShaderLocation(shader, "sunColor");
    locSkyAmb     = GetShaderLocation(shader, "skyAmbient");
    locGroundAmb  = GetShaderLocation(shader, "groundAmbient");
    locFogColor   = GetShaderLocation(shader, "fogColor");
    locFogDensity = GetShaderLocation(shader, "fogDensity");
    locCamPos     = GetShaderLocation(shader, "cameraPos");
    locEmissive   = GetShaderLocation(shader, "emissive");
    locLightCount = GetShaderLocation(shader, "lightCount");
    locLightPos   = GetShaderLocation(shader, "lightPos");
    locLightColor = GetShaderLocation(shader, "lightColor");
    locLightRange = GetShaderLocation(shader, "lightRange");
    SetEmissive(0.0f);
}

void LightingSystem::Shutdown() {
    UnloadShader(shader);
}

static bool Same(Vector3 a, Vector3 b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

void LightingSystem::SetFrame(Vector3 cameraPos) {
    if (!frameCacheValid || !Same(sunDir, cachedSunDir)) {
        Vector3 normalized = Vector3Normalize(sunDir);
        SetShaderValue(shader, locSunDir, &normalized, SHADER_UNIFORM_VEC3);
        cachedSunDir = sunDir;
    }
    if (!frameCacheValid || !Same(sunColor, cachedSunColor)) {
        SetShaderValue(shader, locSunColor, &sunColor, SHADER_UNIFORM_VEC3);
        cachedSunColor = sunColor;
    }
    if (!frameCacheValid || !Same(skyAmbient, cachedSkyAmbient)) {
        SetShaderValue(shader, locSkyAmb, &skyAmbient, SHADER_UNIFORM_VEC3);
        cachedSkyAmbient = skyAmbient;
    }
    if (!frameCacheValid || !Same(groundAmbient, cachedGroundAmbient)) {
        SetShaderValue(shader, locGroundAmb, &groundAmbient, SHADER_UNIFORM_VEC3);
        cachedGroundAmbient = groundAmbient;
    }
    if (!frameCacheValid || !Same(fogColor, cachedFogColor)) {
        SetShaderValue(shader, locFogColor, &fogColor, SHADER_UNIFORM_VEC3);
        cachedFogColor = fogColor;
    }
    if (!frameCacheValid || fogDensity != cachedFogDensity) {
        SetShaderValue(shader, locFogDensity, &fogDensity, SHADER_UNIFORM_FLOAT);
        cachedFogDensity = fogDensity;
    }
    SetShaderValue(shader, locCamPos, &cameraPos, SHADER_UNIFORM_VEC3);
    frameCacheValid = true;
}

void LightingSystem::SetPointLights(const PointLight* lights, int count) {
    if (count > MAX_POINT_LIGHTS) count = MAX_POINT_LIGHTS;
    bool unchanged = count == cachedLightCount;
    for (int i = 0; unchanged && i < count; i++) {
        unchanged = Same(lights[i].pos, cachedLights[i].pos) &&
                    Same(lights[i].color, cachedLights[i].color) &&
                    lights[i].range == cachedLights[i].range;
    }
    if (unchanged) return;

    Vector3 pos[MAX_POINT_LIGHTS];
    Vector3 col[MAX_POINT_LIGHTS];
    float range[MAX_POINT_LIGHTS];
    for (int i = 0; i < count; i++) {
        pos[i] = lights[i].pos;
        col[i] = lights[i].color;
        range[i] = lights[i].range;
        cachedLights[i] = lights[i];
    }
    SetShaderValue(shader, locLightCount, &count, SHADER_UNIFORM_INT);
    if (count > 0) {
        SetShaderValueV(shader, locLightPos, pos, SHADER_UNIFORM_VEC3, count);
        SetShaderValueV(shader, locLightColor, col, SHADER_UNIFORM_VEC3, count);
        SetShaderValueV(shader, locLightRange, range, SHADER_UNIFORM_FLOAT, count);
    }
    cachedLightCount = count;
}

void LightingSystem::SetEmissive(float e) {
    SetShaderValue(shader, locEmissive, &e, SHADER_UNIFORM_FLOAT);
}
