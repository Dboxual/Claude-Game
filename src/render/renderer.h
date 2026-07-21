// Renderer: scene begin/end, the shared primitive mesh kit, frustum +
// distance culling, and per-frame draw statistics. Everything drawn in the
// 3D scene goes through DrawLit so lighting and stats stay consistent.
#pragma once
#include "raylib.h"
#include "render/lighting.h"
#include "render/sky.h"

struct RenderStats {
    int drawCalls = 0;
    int culledChunks = 0;
    int culledProps = 0;
    int particles = 0;
    int pointLights = 0;
};

// Six inward-facing planes (xyz = normal, w = distance term).
struct Frustum {
    Vector4 planes[6];
    bool VisibleAABB(const BoundingBox& box) const;
    bool VisibleSphere(Vector3 center, float radius) const;
};

class Renderer {
public:
    void Init();
    void Shutdown();

    // Computes the frustum, uploads per-frame light uniforms, draws the sky,
    // and enters 3D mode. Pair with EndScene.
    void BeginScene(const Camera3D& cam, float aspect, float time);
    void EndScene();

    void DrawLit(const Mesh& mesh, Matrix transform, Color tint, float emissive = 0.0f);
    bool Visible(const BoundingBox& box) const { return frustum.VisibleAABB(box); }
    bool Visible(Vector3 center, float radius) const { return frustum.VisibleSphere(center, radius); }

    LightingSystem lighting;
    SkyRenderer sky;
    RenderStats stats;

    // Primitive kit shared by world/props/player. Unit-sized; scale via the
    // transform you pass to DrawLit.
    Mesh cube = {};       // 1x1x1 centered
    Mesh cylinder = {};   // r=0.5 h=1, base at origin, +Y
    Mesh cone = {};       // r=0.5 h=1, base at origin, +Y
    Mesh sphere = {};     // r=0.5 centered

private:
    Frustum frustum;
    Material litMat = {};
    float lastEmissive = -1.0f;
};
