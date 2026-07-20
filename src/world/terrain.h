// Heightfield terrain: seeded FBM hills shaped around a central temple
// plateau, with a rim of border mountains that naturally contains the zone.
// Meshes are built once at generation into 8x8 world-space chunks so the
// renderer can frustum-cull them; gameplay samples HeightAt/NormalAt.
#pragma once
#include "raylib.h"

class Renderer;

// World spans [-TERRAIN_HALF, +TERRAIN_HALF] on X and Z, in meters.
constexpr float TERRAIN_HALF = 160.0f;
constexpr float TERRAIN_CELL = 2.0f;
constexpr int TERRAIN_N = (int)(TERRAIN_HALF * 2 / TERRAIN_CELL);   // 160 cells
constexpr int TERRAIN_CHUNKS = 8;                                    // per side

class Terrain {
public:
    void Generate(unsigned int seed);
    void Shutdown();

    float HeightAt(float x, float z) const;      // bilinear, clamped at borders
    Vector3 NormalAt(float x, float z) const;
    void Draw(Renderer& r) const;

    // The flattened area around the temple (used by prop placement).
    static constexpr float PLATEAU_RADIUS = 30.0f;
    float plateauHeight = 0.0f;

private:
    float SampleRaw(float x, float z) const;     // generation-time shape
    float heights[TERRAIN_N + 1][TERRAIN_N + 1] = {};
    Mesh chunks[TERRAIN_CHUNKS][TERRAIN_CHUNKS] = {};
    BoundingBox chunkBounds[TERRAIN_CHUNKS][TERRAIN_CHUNKS] = {};
    unsigned int seed = 0;
    bool generated = false;
};
