#include "world/terrain.h"
#include "core/mathx.h"
#include "render/renderer.h"
#include <cstring>

// ---- deterministic value noise (identical across platforms) --------------

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
    return Lerp(Lerp(a, b, ux), Lerp(c, d, ux), uz);
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

static constexpr float RIM_START = 90.0f;    // rim begins this far from border
static constexpr float RIM_RAMP = 62.0f;
static constexpr float RIM_HEIGHT = 30.0f;

float Terrain::SampleRaw(float x, float z) const {
    const BiomeStyle& b = def->biome;

    // Biome-scaled rolling forms + medium undulation.
    float h = Fbm(x / b.hillFreq, z / b.hillFreq, seed, 4, 2.1f, 0.5f) * b.hillAmp
            + Fbm(x / 26.0f, z / 26.0f, seed + 7u, 3, 2.0f, 0.5f) * (2.0f + b.hillAmp * 0.06f);

    // Border rim mountains contain the zone...
    float edge = fmaxf(fabsf(x), fabsf(z));
    if (edge > ZONE_HALF - RIM_START) {
        float t = Saturate((edge - (ZONE_HALF - RIM_START)) / RIM_RAMP);
        float rim = t * t * RIM_HEIGHT *
                    (0.75f + 0.25f * Fbm(x / 18.0f, z / 18.0f, seed + 31u, 2, 2.0f, 0.5f));
        // ...except in a clearing around each gate, where a pass cuts through.
        float suppress = 0.0f;
        for (const GateDef& g : def->gates) {
            Vector2 gp = GateXZ(g);
            float d = sqrtf((x - gp.x) * (x - gp.x) + (z - gp.y) * (z - gp.y));
            suppress = fmaxf(suppress, 1.0f - SmoothStep01((d - 24.0f) / 34.0f));
        }
        h += rim * (1.0f - suppress);
    }

    // Temple plateau (Elder Vale only).
    if (def->temple) {
        float dist = sqrtf(x * x + z * z);
        if (dist < PLATEAU_RADIUS + 18.0f) {
            float t = 1.0f - SmoothStep01((dist - PLATEAU_RADIUS) / 18.0f);
            h = Lerp(h, 3.2f, t);
        }
    }
    return h;
}

// ---- generation -----------------------------------------------------------

void Terrain::Generate(unsigned int worldSeed, const ZoneDef& zoneDef) {
    def = &zoneDef;
    seed = zoneDef.SeedFor(worldSeed);
    plateauHeight = 3.2f;

    const int NV = TERRAIN_N + 1;
    heights.assign((size_t)NV * NV, 0.0f);
    normals.assign((size_t)NV * NV, Vector3{ 0, 1, 0 });
    colors.assign((size_t)NV * NV, WHITE);

    for (int iz = 0; iz < NV; iz++)
        for (int ix = 0; ix < NV; ix++)
            H(ix, iz) = SampleRaw(-ZONE_HALF + ix * TERRAIN_CELL,
                                  -ZONE_HALF + iz * TERRAIN_CELL);

    // Cache normals (central differences) and biome paint per grid vertex so
    // every LOD mesh just copies instead of recomputing.
    const BiomeStyle& b = def->biome;
    const Color highRock = { 148, 140, 134, 255 };
    for (int iz = 0; iz < NV; iz++) {
        for (int ix = 0; ix < NV; ix++) {
            float wx = -ZONE_HALF + ix * TERRAIN_CELL;
            float wz = -ZONE_HALF + iz * TERRAIN_CELL;
            int xm = ix > 0 ? ix - 1 : ix, xp = ix < NV - 1 ? ix + 1 : ix;
            int zm = iz > 0 ? iz - 1 : iz, zp = iz < NV - 1 ? iz + 1 : iz;
            Vector3 n = Vector3Normalize({ Hc(xm, iz) - Hc(xp, iz),
                                           2.0f * TERRAIN_CELL * (xp - xm) * 0.5f,
                                           Hc(ix, zm) - Hc(ix, zp) });
            normals[iz * NV + ix] = n;

            float patch = ValueNoise(wx / 17.0f, wz / 17.0f, seed + 51u);
            Color c = ColorLerpF(b.grassA, b.grassB, patch);
            if (def->temple) {
                float dist = sqrtf(wx * wx + wz * wz);
                if (dist < PLATEAU_RADIUS + 14.0f)
                    c = ColorLerpF(b.dry, c, SmoothStep01((dist - PLATEAU_RADIUS + 4.0f) / 14.0f));
            }
            if (n.y < 0.82f)
                c = ColorLerpF(c, b.rock, SmoothStep01((0.82f - n.y) / 0.14f));
            float wy = Hc(ix, iz);
            float scree = b.hillAmp + RIM_HEIGHT * 0.55f;
            if (wy > scree)
                c = ColorLerpF(c, highRock, SmoothStep01((wy - scree) / 10.0f));
            colors[iz * NV + ix] = c;
        }
    }

    // ---- chunk meshes, three LODs each, with skirts ----------------------
    chunks.assign((size_t)CHUNKS_PER_SIDE * CHUNKS_PER_SIDE, Chunk{});

    for (int cz = 0; cz < CHUNKS_PER_SIDE; cz++) {
        for (int cx = 0; cx < CHUNKS_PER_SIDE; cx++) {
            Chunk& chunk = chunks[cz * CHUNKS_PER_SIDE + cx];
            Vector3 bbMin = { 1e9f, 1e9f, 1e9f }, bbMax = { -1e9f, -1e9f, -1e9f };

            for (int lod = 0; lod < 3; lod++) {
                int step = 1 << lod;
                int n = CHUNK_CELLS / step;              // cells per side
                int vs = n + 1;                          // grid verts per side
                int gridVerts = vs * vs;
                int skirtVerts = 4 * vs;
                int gridTris = n * n * 2;
                int skirtTris = 4 * n * 2 * 2;   // both windings: always visible

                Mesh& m = chunk.lods[lod];
                m = Mesh{};
                m.vertexCount = gridVerts + skirtVerts;
                m.triangleCount = gridTris + skirtTris;
                m.vertices = (float*)MemAlloc(m.vertexCount * 3 * sizeof(float));
                m.normals = (float*)MemAlloc(m.vertexCount * 3 * sizeof(float));
                m.colors = (unsigned char*)MemAlloc(m.vertexCount * 4);
                m.indices = (unsigned short*)MemAlloc(m.triangleCount * 3 * sizeof(unsigned short));

                auto writeVert = [&](int v, int gx, int gz, float yDrop) {
                    const int NVv = TERRAIN_N + 1;
                    float wx = -ZONE_HALF + gx * TERRAIN_CELL;
                    float wz = -ZONE_HALF + gz * TERRAIN_CELL;
                    float wy = Hc(gx, gz) - yDrop;
                    m.vertices[v * 3 + 0] = wx;
                    m.vertices[v * 3 + 1] = wy;
                    m.vertices[v * 3 + 2] = wz;
                    Vector3 nn = normals[gz * NVv + gx];
                    m.normals[v * 3 + 0] = nn.x;
                    m.normals[v * 3 + 1] = nn.y;
                    m.normals[v * 3 + 2] = nn.z;
                    Color c = colors[gz * NVv + gx];
                    m.colors[v * 4 + 0] = c.r; m.colors[v * 4 + 1] = c.g;
                    m.colors[v * 4 + 2] = c.b; m.colors[v * 4 + 3] = 255;
                    if (lod == 0 && yDrop == 0.0f) {
                        bbMin = Vector3Min(bbMin, { wx, wy, wz });
                        bbMax = Vector3Max(bbMax, { wx, wy, wz });
                    }
                };

                int v = 0;
                for (int iz = 0; iz <= n; iz++)
                    for (int ix = 0; ix <= n; ix++, v++)
                        writeVert(v, cx * CHUNK_CELLS + ix * step,
                                  cz * CHUNK_CELLS + iz * step, 0.0f);

                // Skirt ring: edge verts dropped 6 m hide cracks between
                // neighboring chunks at different LODs.
                int skirtBase = v;
                for (int i = 0; i <= n; i++, v++)   // north edge (iz = 0)
                    writeVert(v, cx * CHUNK_CELLS + i * step, cz * CHUNK_CELLS, 6.0f);
                for (int i = 0; i <= n; i++, v++)   // south edge
                    writeVert(v, cx * CHUNK_CELLS + i * step, cz * CHUNK_CELLS + n * step, 6.0f);
                for (int i = 0; i <= n; i++, v++)   // west edge
                    writeVert(v, cx * CHUNK_CELLS, cz * CHUNK_CELLS + i * step, 6.0f);
                for (int i = 0; i <= n; i++, v++)   // east edge
                    writeVert(v, cx * CHUNK_CELLS + n * step, cz * CHUNK_CELLS + i * step, 6.0f);

                int t = 0;
                auto quad = [&](int a, int bq, int c2, int d2) {
                    m.indices[t++] = (unsigned short)a;
                    m.indices[t++] = (unsigned short)bq;
                    m.indices[t++] = (unsigned short)c2;
                    m.indices[t++] = (unsigned short)c2;
                    m.indices[t++] = (unsigned short)bq;
                    m.indices[t++] = (unsigned short)d2;
                };
                for (int iz = 0; iz < n; iz++)
                    for (int ix = 0; ix < n; ix++) {
                        int i0 = iz * vs + ix;
                        quad(i0, i0 + vs, i0 + 1, i0 + vs + 1);
                    }
                // Skirt quads, emitted with both windings so backface culling
                // can never hide them regardless of viewing side.
                auto skirtQuad = [&](int topA, int topB, int botA, int botB) {
                    m.indices[t++] = (unsigned short)topA;
                    m.indices[t++] = (unsigned short)botA;
                    m.indices[t++] = (unsigned short)topB;
                    m.indices[t++] = (unsigned short)topB;
                    m.indices[t++] = (unsigned short)botA;
                    m.indices[t++] = (unsigned short)botB;
                    m.indices[t++] = (unsigned short)topB;
                    m.indices[t++] = (unsigned short)botA;
                    m.indices[t++] = (unsigned short)topA;
                    m.indices[t++] = (unsigned short)botB;
                    m.indices[t++] = (unsigned short)botA;
                    m.indices[t++] = (unsigned short)topB;
                };
                for (int i = 0; i < n; i++) {
                    skirtQuad(0 * vs + i, 0 * vs + i + 1,
                              skirtBase + 0 * vs + i, skirtBase + 0 * vs + i + 1);
                    skirtQuad(n * vs + i, n * vs + i + 1,
                              skirtBase + 1 * vs + i, skirtBase + 1 * vs + i + 1);
                    skirtQuad(i * vs + 0, (i + 1) * vs + 0,
                              skirtBase + 2 * vs + i, skirtBase + 2 * vs + i + 1);
                    skirtQuad(i * vs + n, (i + 1) * vs + n,
                              skirtBase + 3 * vs + i, skirtBase + 3 * vs + i + 1);
                }

                UploadMesh(&m, false);
            }

            bbMin.y -= 6.5f;   // include skirts in culling bounds
            chunk.bounds = { bbMin, bbMax };
            chunk.center = Vector3Scale(Vector3Add(bbMin, bbMax), 0.5f);
        }
    }

    static_assert(CHUNKS_PER_SIDE % TERRAIN_BLOCK_CHUNKS == 0);
    cullBlocks.clear();
    cullBlocks.reserve((size_t)TERRAIN_BLOCKS_PER_SIDE * TERRAIN_BLOCKS_PER_SIDE);
    for (int bz = 0; bz < TERRAIN_BLOCKS_PER_SIDE; bz++) {
        for (int bx = 0; bx < TERRAIN_BLOCKS_PER_SIDE; bx++) {
            BoundingBox bounds = {
                { 1e9f, 1e9f, 1e9f },
                { -1e9f, -1e9f, -1e9f },
            };
            int chunkX = bx * TERRAIN_BLOCK_CHUNKS;
            int chunkZ = bz * TERRAIN_BLOCK_CHUNKS;
            for (int z = 0; z < TERRAIN_BLOCK_CHUNKS; z++) {
                for (int x = 0; x < TERRAIN_BLOCK_CHUNKS; x++) {
                    const BoundingBox& child =
                        chunks[(chunkZ + z) * CHUNKS_PER_SIDE + chunkX + x].bounds;
                    bounds.min = Vector3Min(bounds.min, child.min);
                    bounds.max = Vector3Max(bounds.max, child.max);
                }
            }
            cullBlocks.push_back({ bounds, chunkX, chunkZ });
        }
    }
    generated = true;
}

void Terrain::Shutdown() {
    if (!generated) return;
    for (Chunk& c : chunks)
        for (Mesh& m : c.lods) UnloadMesh(m);
    chunks.clear();
    cullBlocks.clear();
    generated = false;
}

// ---- sampling -------------------------------------------------------------

float Terrain::HeightAt(float x, float z) const {
    float fx = (x + ZONE_HALF) / TERRAIN_CELL;
    float fz = (z + ZONE_HALF) / TERRAIN_CELL;
    fx = Clamp(fx, 0.0f, (float)TERRAIN_N - 0.001f);
    fz = Clamp(fz, 0.0f, (float)TERRAIN_N - 0.001f);
    int ix = (int)fx, iz = (int)fz;
    float tx = fx - ix, tz = fz - iz;
    float a = Hc(ix, iz),     b = Hc(ix + 1, iz);
    float c = Hc(ix, iz + 1), d = Hc(ix + 1, iz + 1);
    return Lerp(Lerp(a, b, tx), Lerp(c, d, tx), tz);
}

Vector3 Terrain::NormalAt(float x, float z) const {
    float e = TERRAIN_CELL;
    float hl = HeightAt(x - e, z), hr = HeightAt(x + e, z);
    float hd = HeightAt(x, z - e), hu = HeightAt(x, z + e);
    return Vector3Normalize({ hl - hr, 2.0f * e, hd - hu });
}

GroundKind Terrain::KindAt(float x, float z) const {
    if (NormalAt(x, z).y < 0.80f) return GroundKind::Rock;
    if (def && def->temple && sqrtf(x * x + z * z) < PLATEAU_RADIUS + 12.0f)
        return GroundKind::Dry;
    return GroundKind::Grass;
}

void Terrain::Draw(Renderer& r, Vector3 camPos) const {
    for (const CullBlock& block : cullBlocks) {
        if (!r.Visible(block.bounds)) {
            r.stats.culledChunks += TERRAIN_BLOCK_CHUNKS * TERRAIN_BLOCK_CHUNKS;
            continue;
        }
        for (int z = 0; z < TERRAIN_BLOCK_CHUNKS; z++) {
            for (int x = 0; x < TERRAIN_BLOCK_CHUNKS; x++) {
                const Chunk& c = chunks[(block.chunkZ + z) * CHUNKS_PER_SIDE +
                                        block.chunkX + x];
                if (!r.Visible(c.bounds)) { r.stats.culledChunks++; continue; }
                float dx = c.center.x - camPos.x, dz = c.center.z - camPos.z;
                float d2 = dx * dx + dz * dz;
                int lod = d2 > LOD2_DIST * LOD2_DIST ? 2
                        : d2 > LOD1_DIST * LOD1_DIST ? 1 : 0;
                r.DrawLit(c.lods[lod], MatrixIdentity(), WHITE);
            }
        }
    }
}
