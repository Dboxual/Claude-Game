// Stylized lighting: hemisphere ambient + wrapped sun + a few point lights
// + distance fog, all in one forward shader. Shader source is embedded (no
// asset files). Tuning the mood happens through the public color members.
#pragma once
#include "raylib.h"

constexpr int MAX_POINT_LIGHTS = 8;

struct PointLight {
    Vector3 pos = { 0, 0, 0 };
    Vector3 color = { 1, 1, 1 };   // premultiply intensity into the color
    float range = 8.0f;
};

class LightingSystem {
public:
    void Init();
    void Shutdown();

    // Per-frame uniforms (camera position drives fog).
    void SetFrame(Vector3 cameraPos);
    void SetPointLights(const PointLight* lights, int count);
    // Per-draw emissive factor: 0 = fully lit, 1 = unlit glow (bloom feed).
    void SetEmissive(float e);

    Shader shader = {};

    // Scene mood — the "golden hour over ancient stone" baseline. Sun sits
    // low so vertical surfaces (columns, trunks) catch warm side light.
    Vector3 sunDir = { -0.58f, -0.42f, 0.40f };     // direction light travels
    Vector3 sunColor = { 1.08f, 0.94f, 0.70f };
    Vector3 skyAmbient = { 0.42f, 0.50f, 0.64f };   // upper hemisphere fill
    Vector3 groundAmbient = { 0.32f, 0.28f, 0.23f };// bounce off the ground
    Vector3 fogColor = { 0.66f, 0.72f, 0.86f };     // must match sky horizon
    float fogDensity = 0.0045f;

private:
    int locSunDir = 0, locSunColor = 0, locSkyAmb = 0, locGroundAmb = 0;
    int locFogColor = 0, locFogDensity = 0, locCamPos = 0, locEmissive = 0;
    int locLightCount = 0, locLightPos = 0, locLightColor = 0, locLightRange = 0;
};
