// One zone's contents: biome-driven prop placement, collision, walkable
// platforms, point lights, gates to neighboring zones. All spatial queries
// route through uniform grids (see spatial_grid.h) so HUGE zones stay
// O(local). Prop geometry composes from the renderer's primitive kit at
// draw time; placement is a pure function of (worldSeed, ZoneDef).
#pragma once
#include "raylib.h"
#include "render/lighting.h"     // PointLight
#include "world/spatial_grid.h"
#include "world/terrain.h"
#include "zone/zone_def.h"
#include <vector>

class Renderer;

enum class PropType {
    Column, BrokenColumn, Monolith, StandingStone,
    Tree, Rock, Shrine, Altar, Gate,
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

// A travel gate to a neighboring zone (mirrors ZoneDef::gates[gateIndex]).
struct GateWorldInfo {
    Vector3 pos;         // portal center (interaction focus)
    int gateIndex;
};

class World {
public:
    void Generate(unsigned int worldSeed, const Terrain& terrain, const ZoneDef& def);

    const std::vector<ShrineInfo>& Shrines() const { return shrines; }
    const std::vector<GateWorldInfo>& Gates() const { return gates; }

    // Ground the player stands on: terrain unless a reachable platform disc
    // is underfoot.
    float GroundHeightAt(float x, float z, float fromY) const;
    Vector3 GroundNormalAt(float x, float z, float fromY) const;

    // Pushes a capsule out of overlapping obstacles (horizontal only) and
    // clamps to the playable area.
    Vector3 ResolveCollisions(Vector3 pos, float radius, float height,
                              Vector3* velocity = nullptr) const;

    // True if a point sits inside terrain or an obstacle (with padding).
    bool PointBlocked(Vector3 p, float pad) const;

    void Draw(Renderer& r, Vector3 camPos, float viewDistance, float time) const;

    const std::vector<PointLight>& Lights() const { return lights; }
    Vector3 SpawnPos() const { return spawnPos; }
    float SpawnYaw() const { return spawnYaw; }

private:
    void Place(const Terrain& t, Prop& p);
    void AddCollider(const CylinderCollider& c);
    void DrawProp(Renderer& r, const Prop& p, float time) const;

    const Terrain* terrain = nullptr;
    const ZoneDef* def = nullptr;
    std::vector<ShrineInfo> shrines;
    std::vector<GateWorldInfo> gates;
    std::vector<Prop> props;
    std::vector<CylinderCollider> colliders;
    std::vector<Platform> platforms;
    std::vector<PointLight> lights;
    SpatialGrid propGrid;        // prop indices by position+bound radius
    SpatialGrid colliderGrid;    // collider indices
    mutable std::vector<int> scratch;   // reused query buffer (no per-frame alloc)
    Vector3 spawnPos = { 0, 0, 0 };
    float spawnYaw = 0.0f;
};
