// Procedural sky dome: vertical gradient, sun disc with glow, slow wispy
// clouds. Drawn first each frame at depth 1.0 so the world renders over it.
// Horizon color should stay in sync with LightingSystem::fogColor so distant
// geometry melts into the sky.
#pragma once
#include "raylib.h"

class SkyRenderer {
public:
    void Init();
    void Shutdown();
    void Draw(const Camera3D& cam, Vector3 sunDir, float time);

    Vector3 zenithColor  = { 0.22f, 0.38f, 0.72f };
    Vector3 horizonColor = { 0.66f, 0.72f, 0.86f };
    Vector3 sunColor     = { 1.00f, 0.88f, 0.62f };

private:
    Shader shader = {};
    Mesh dome = {};
    Material mat = {};
    int locSunDir = 0, locZenith = 0, locHorizon = 0, locSunColor = 0, locTime = 0;
};
