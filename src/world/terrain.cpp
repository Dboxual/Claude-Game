#include "world/terrain.h"
#include "core/mathx.h"
#include "render/renderer.h"
#include <cstring>

// ---- deterministic value noise -------------------------------------------
// Hash-based so the same seed always produces the same world on every
// platform; good enough visually, and dependency-free.

static float HashN(int x, int z, unsigned int seed) {
    unsigned int h = (unsigned int)x * 374761393u + (unsigned int)z * 668265263u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return ((h ^ (h >> 16)) & 0xFFFFFF) / 16777215.0f;
}

static float ValueNoise(float x, float z, unsigned int seed) {
    int ix = (int)floorf(x), iz = (int)floorf(z);
    float fx = x - ix, fz = z - iz;
    float ux = fx * fx * (3.0f - 2.0f * fx);
    float uz = fz * fz * (3.0f - 2.0f * fz);
    float a = HashN(ix, iz, seed),     b = HashN(ix + 1, iz, seed);
    float c = HashN(ix, iz + 1, seed), d = HashN(ix + 1, iz + 1, seed);
    return Lerp(Lerp(a, b, ux), Lerp(c, d, ux), uz);   // 0..1
}

static float Fbm(float x, float z, unsigned int seed, int octaves, float lacunarity, float gain) {
    float sum = 0.0f, amp = 0.5f, freq = 1.0f, norm = 0.0f;
    for (int i = 0; i < octaves; i++) {
        sum += (ValueNoise(x * freq, z * freq, seed + (unsigned int)i * 101u) * 2.0f - 1.0f) * amp;
        norm += amp;
        freq *= lacunarity;
        amp *= gain;
    }
    return sum / norm;   // -1..1
}

// ---- shape ----------------------------------------------------------------

float Terrain::SampleRaw(float x, float z) const {
    // Rolling hills: broad forms + medium undulation + fine detail.
    float h = Fbm(x / 95.0f, z / 95.0f, seed, 4, 2.1f, 0.5f) * 11.0f
            + Fbm(x / 26.0f, z / 26.0f, seed + 7u, 3, 2.0f, 0.5f) * 2.6f;

    // Border rim: the zone reads as a valley ringed by mountains instead of
    // ending at an arbitrary edge.
    float edge = fmaxf(fabsf(x), fabsf(z));
    if (edge > 118.0f) {
        float t = Saturate((edge - 118.0f) / 42.0f);
        h += t * t * 26.0f * (0.75f + 0.25f * Fbm(x / 18.0f, z / 18.0f, seed + 31u, 2, 2.0f, 0.5f));
    }

    // Temple plateau: flatten toward a fixed height so ruins sit naturally.
    float dist = sqrtf(x * x + z * z);
    if (dist < PLATEAU_RADIUS + 18.0f) {
        float t = 1.0f - SmoothStep01((dist - PLATEAU_RADIUS) / 18.0f);
        h = Lerp(h, 3.2f, t);
    }
    return h;
}

// ---- generation -----------------------------------------------------------

void Terrain::Generate(unsigned int worldSeed) {
    seed = worldSeed;
    plateauHeight = 3.2f;

    for (int iz = 0; iz <= TERRAIN_N; iz++)
        for (int ix = 0; ix <= TERRAIN_N; ix++)
            heights[ix][iz] = SampleRaw(-TERRAIN_HALF + ix * TERRAIN_CELL,
                                        -TERRAIN_HALF + iz * TERRAIN_CELL);

    // Vertex colors: grass with seeded patch variation, dry gold near the
    // plateau, rock on steep slopes, scree high on the rim.
    const Color grassA = { 92, 152, 74, 255 };
    const Color grassB = { 116, 168, 70, 255 };
    const Color dryGold = { 158, 150, 82, 255 };
    const Color rock = { 128, 118, 106, 255 };
    const Color highRock = { 148, 140, 134, 255 };

    const int cellsPerChunk = TERRAIN_N / TERRAIN_CHUNKS;    // 20
    const int vertsPerSide = cellsPerChunk + 1;               // 21

    for (int cz = 0; cz < TERRAIN_CHUNKS; cz++) {
        for (int cx = 0; cx < TERRAIN_CHUNKS; cx++) {
            Mesh& m = chunks[cx][cz];
            m = Mesh{};
            m.vertexCount = vertsPerSide * vertsPerSide;
            m.triangleCount = cellsPerChunk * cellsPerChunk * 2;
            m.vertices = (float*)MemAlloc(m.vertexCount * 3 * sizeof(float));
            m.normals = (float*)MemAlloc(m.vertexCount * 3 * sizeof(float));
            m.colors = (unsigned char*)MemAlloc(m.vertexCount * 4);
            m.indices = (unsigned short*)MemAlloc(m.triangleCount * 3 * sizeof(unsigned short));

            Vector3 bbMin = { 1e9f, 1e9f, 1e9f }, bbMax = { -1e9f, -1e9f, -1e9f };
            int v = 0;
            for (int iz = 0; iz <= cellsPerChunk; iz++) {
                for (int ix = 0; ix <= cellsPerChunk; ix++, v++) {
                    int gx = cx * cellsPerChunk + ix;
                    int gz = cz * cellsPerChunk + iz;
                    float wx = -TERRAIN_HALF + gx * TERRAIN_CELL;
                    float wz = -TERRAIN_HALF + gz * TERRAIN_CELL;
                    float wy = heights[gx][gz];

                    m.vertices[v * 3 + 0] = wx;
                    m.vertices[v * 3 + 1] = wy;
                    m.vertices[v * 3 + 2] = wz;

                    Vector3 n = NormalAt(wx, wz);
                    m.normals[v * 3 + 0] = n.x;
                    m.normals[v * 3 + 1] = n.y;
                    m.normals[v * 3 + 2] = n.z;

                    float patch = ValueNoise(wx / 17.0f, wz / 17.0f, seed + 51u);
                    Color c = ColorLerpF(grassA, grassB, patch);
                    float dist = sqrtf(wx * wx + wz * wz);
                    if (dist < PLATEAU_RADIUS + 14.0f)   // sun-dried temple ground
                        c = ColorLerpF(dryGold, c, SmoothStep01((dist - PLATEAU_RADIUS + 4.0f) / 14.0f));
                    if (n.y < 0.82f)                      // steep -> rock
                        c = ColorLerpF(c, rock, SmoothStep01((0.82f - n.y) / 0.14f));
                    if (wy > 16.0f)                       // rim scree
                        c = ColorLerpF(c, highRock, SmoothStep01((wy - 16.0f) / 10.0f));

                    m.colors[v * 4 + 0] = c.r;
                    m.colors[v * 4 + 1] = c.g;
                    m.colors[v * 4 + 2] = c.b;
                    m.colors[v * 4 + 3] = 255;

                    bbMin = Vector3Min(bbMin, { wx, wy, wz });
                    bbMax = Vector3Max(bbMax, { wx, wy, wz });
                }
            }

            int t = 0;
            for (int iz = 0; iz < cellsPerChunk; iz++) {
                for (int ix = 0; ix < cellsPerChunk; ix++) {
                    unsigned short i0 = (unsigned short)(iz * vertsPerSide + ix);
                    unsigned short i1 = (unsigned short)(i0 + 1);
                    unsigned short i2 = (unsigned short)(i0 + vertsPerSide);
                    unsigned short i3 = (unsigned short)(i2 + 1);
                    m.indices[t++] = i0; m.indices[t++] = i2; m.indices[t++] = i1;
                    m.indices[t++] = i1; m.indices[t++] = i2; m.indices[t++] = i3;
                }
            }

            UploadMesh(&m, false);
            chunkBounds[cx][cz] = { bbMin, bbMax };
        }
    }
    generated = true;
}

void Terrain::Shutdown() {
    if (!generated) return;
    for (int cz = 0; cz < TERRAIN_CHUNKS; cz++)
        for (int cx = 0; cx < TERRAIN_CHUNKS; cx++)
            UnloadMesh(chunks[cx][cz]);
    generated = false;
}

// ---- sampling -------------------------------------------------------------

float Terrain::HeightAt(float x, float z) const {
    float fx = (x + TERRAIN_HALF) / TERRAIN_CELL;
    float fz = (z + TERRAIN_HALF) / TERRAIN_CELL;
    fx = Clamp(fx, 0.0f, (float)TERRAIN_N - 0.001f);
    fz = Clamp(fz, 0.0f, (float)TERRAIN_N - 0.001f);
    int ix = (int)fx, iz = (int)fz;
    float tx = fx - ix, tz = fz - iz;
    float a = heights[ix][iz],     b = heights[ix + 1][iz];
    float c = heights[ix][iz + 1], d = heights[ix + 1][iz + 1];
    return Lerp(Lerp(a, b, tx), Lerp(c, d, tx), tz);
}

Vector3 Terrain::NormalAt(float x, float z) const {
    // Central differences over one cell; robust and cheap.
    float e = TERRAIN_CELL;
    float hl = HeightAt(x - e, z), hr = HeightAt(x + e, z);
    float hd = HeightAt(x, z - e), hu = HeightAt(x, z + e);
    return Vector3Normalize({ hl - hr, 2.0f * e, hd - hu });
}

void Terrain::Draw(Renderer& r) const {
    for (int cz = 0; cz < TERRAIN_CHUNKS; cz++) {
        for (int cx = 0; cx < TERRAIN_CHUNKS; cx++) {
            if (!r.Visible(chunkBounds[cx][cz])) { r.stats.culledChunks++; continue; }
            r.DrawLit(chunks[cx][cz], MatrixIdentity(), WHITE);
        }
    }
}
