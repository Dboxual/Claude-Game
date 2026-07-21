// Heightfield terrain for one HUGE zone (ZONE_SIZE x ZONE_SIZE meters).
// Biome-parameterized generation (shape + paint from the ZoneDef), rim
// mountains along every border with clearings carved at gates, and 32 m
// mesh chunks with three LOD levels (full/half/quarter) picked per frame by
// camera distance. Skirt strips hide LOD-boundary cracks. Gameplay samples
// HeightAt/NormalAt; footsteps ask GroundKind.
#pragma once
#include "raylib.h"
#include "zone/zone_def.h"
#include <vector>

class Renderer;

constexpr float TERRAIN_CELL = 2.0f;
constexpr int TERRAIN_N = (int)(ZONE_SIZE / TERRAIN_CELL);        // 640 cells
constexpr int CHUNK_CELLS = 16;                                    // 32 m chunks
constexpr int CHUNKS_PER_SIDE = TERRAIN_N / CHUNK_CELLS;           // 40
constexpr int TERRAIN_BLOCK_CHUNKS = 5;                             // 160 m cull blocks
constexpr int TERRAIN_BLOCKS_PER_SIDE = CHUNKS_PER_SIDE / TERRAIN_BLOCK_CHUNKS;
// LOD switch distances (camera to chunk center).
constexpr float LOD1_DIST = 170.0f;
constexpr float LOD2_DIST = 420.0f;

enum class GroundKind { Grass, Rock, Dry };

class Terrain {
public:
    void Generate(unsigned int worldSeed, const ZoneDef& def);
    void Shutdown();

    float HeightAt(float x, float z) const;      // bilinear, clamped
    Vector3 NormalAt(float x, float z) const;
    GroundKind KindAt(float x, float z) const;   // footstep/fx material
    void Draw(Renderer& r, Vector3 camPos) const;

    static constexpr float PLATEAU_RADIUS = 30.0f;
    float plateauHeight = 3.2f;                  // valid when def->temple

private:
    float SampleRaw(float x, float z) const;
    float& H(int ix, int iz) { return heights[iz * (TERRAIN_N + 1) + ix]; }
    float Hc(int ix, int iz) const { return heights[iz * (TERRAIN_N + 1) + ix]; }

    std::vector<float> heights;                  // (N+1)^2 grid samples
    std::vector<Vector3> normals;                // cached per grid vertex
    std::vector<Color> colors;                   // cached biome paint
    struct Chunk { Mesh lods[3]; BoundingBox bounds; Vector3 center; };
    std::vector<Chunk> chunks;                   // CHUNKS_PER_SIDE^2
    struct CullBlock { BoundingBox bounds; int chunkX, chunkZ; };
    std::vector<CullBlock> cullBlocks;
    const ZoneDef* def = nullptr;
    unsigned int seed = 0;
    bool generated = false;
};
