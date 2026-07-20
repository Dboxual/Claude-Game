// The zone: prop placement, collision, walkable platforms, and the point
// lights the props emit. Prop geometry is composed from the renderer's
// primitive kit at draw time — no model files. Placement is fully seeded.
#pragma once
#include "raylib.h"
#include "render/lighting.h"     // PointLight
#include "world/terrain.h"
#include <vector>

class Renderer;

enum class PropType {
    Column, BrokenColumn, Monolith, StandingStone,
    Tree, Rock, Shrine, Altar,
};

struct Prop {
    PropType type;
    Vector3 pos;          // feet on the ground
    float yaw = 0.0f;     // radians
    float scale = 1.0f;
    float tint = 1.0f;    // brightness variation, seeded
    float boundRadius = 3.0f;   // world-space cull sphere radius (scaled)
};

// Vertical cylinder obstacle (columns, trunks, stones).
struct CylinderCollider {
    Vector2 xz;
    float radius;
    float y0, y1;
};

// Flat walkable disc (the temple dais). GroundHeightAt uses these.
struct Platform {
    Vector2 xz;
    float radius;
    float topY;
};

// Where a shrine's crystal floats — the interaction system's focus points.
struct ShrineInfo {
    Vector3 crystalPos;
    int lightIndex;      // index into Lights() this shrine drives
    bool heart;          // the central altar shrine
};

class World {
public:
    void Generate(unsigned int seed, const Terrain& terrain);
    const std::vector<ShrineInfo>& Shrines() const { return shrines; }

    // Ground the player stands on: terrain, unless a platform disc is
    // underfoot and reachable (small step up) from the query height.
    float GroundHeightAt(float x, float z, float fromY) const;

    // Pushes a capsule of `radius` at feet position `pos` out of all
    // overlapping obstacles (horizontal resolution only). Also clamps to the
    // playable area. Returns the corrected position.
    Vector3 ResolveCollisions(Vector3 pos, float radius, float height) const;

    // True if a point sits inside terrain or an obstacle (with padding).
    // Used by the third-person camera boom to avoid clipping.
    bool PointBlocked(Vector3 p, float pad) const;

    void Draw(Renderer& r, Vector3 camPos, float viewDistance, float time) const;

    const std::vector<PointLight>& Lights() const { return lights; }
    Vector3 SpawnPos() const { return spawnPos; }
    float SpawnYaw() const { return spawnYaw; }

private:
    void Place(const Terrain& t, Prop p);
    void DrawProp(Renderer& r, const Prop& p, float time) const;

    const Terrain* terrain = nullptr;
    std::vector<ShrineInfo> shrines;
    std::vector<Prop> props;
    std::vector<CylinderCollider> colliders;
    std::vector<Platform> platforms;
    std::vector<PointLight> lights;
    Vector3 spawnPos = { 0, 0, 0 };
    float spawnYaw = 0.0f;
};
