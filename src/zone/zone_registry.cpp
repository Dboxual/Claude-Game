#include "zone/zone_def.h"
#include <cmath>

// The gate arch sits this far inside the zone border.
static constexpr float GATE_INSET = 16.0f;

Vector2 GateXZ(const GateDef& g) {
    float d = ZONE_HALF - GATE_INSET;
    switch (g.edge) {
        case EDGE_N: return { g.offset, -d };
        case EDGE_E: return { d, g.offset };
        case EDGE_S: return { g.offset, d };
        default:     return { -d, g.offset };
    }
}

Vector2 GateInwardDir(int edge) {
    switch (edge) {
        case EDGE_N: return { 0, 1 };
        case EDGE_E: return { -1, 0 };
        case EDGE_S: return { 0, -1 };
        default:     return { 1, 0 };
    }
}

float GateInwardYaw(int edge) {
    // yaw 0 faces -Z; forward = (sin yaw, 0, -cos yaw)
    switch (edge) {
        case EDGE_N: return PI;            // walk toward +Z
        case EDGE_E: return -PI * 0.5f;    // walk toward -X
        case EDGE_S: return 0.0f;          // walk toward -Z
        default:     return PI * 0.5f;     // walk toward +X
    }
}

// ---- biome palette shorthand ---------------------------------------------

static BiomeStyle Vale() {
    BiomeStyle b{};
    b.grassA = { 92, 152, 74, 255 };  b.grassB = { 116, 168, 70, 255 };
    b.rock = { 128, 118, 106, 255 };  b.dry = { 158, 150, 82, 255 };
    b.canopyA = { 74, 142, 66, 255 }; b.canopyB = { 118, 162, 58, 255 };
    b.trunk = { 104, 76, 50, 255 };
    b.treeDensity = 0.9f; b.rockDensity = 0.55f;
    b.fogColor = { 0.66f, 0.72f, 0.86f }; b.fogDensity = 0.0045f;
    b.skyZenith = { 0.22f, 0.38f, 0.72f };
    b.sunColor = { 1.08f, 0.94f, 0.70f };
    b.skyAmbient = { 0.42f, 0.50f, 0.64f }; b.groundAmbient = { 0.32f, 0.28f, 0.23f };
    b.viewDistanceScale = 1.0f;
    b.hillAmp = 11.0f; b.hillFreq = 95.0f;
    return b;
}

static BiomeStyle Thornwood() {
    BiomeStyle b = Vale();
    b.grassA = { 58, 112, 58, 255 }; b.grassB = { 78, 128, 54, 255 };
    b.canopyA = { 44, 104, 52, 255 }; b.canopyB = { 70, 120, 44, 255 };
    b.trunk = { 84, 60, 40, 255 };
    b.treeDensity = 2.3f; b.rockDensity = 0.4f;
    b.fogColor = { 0.55f, 0.64f, 0.66f }; b.fogDensity = 0.0075f;
    b.skyZenith = { 0.20f, 0.34f, 0.58f };
    b.skyAmbient = { 0.34f, 0.44f, 0.50f };
    b.viewDistanceScale = 0.72f;
    b.hillAmp = 9.0f; b.hillFreq = 70.0f;
    return b;
}

static BiomeStyle GildedSteppe() {
    BiomeStyle b = Vale();
    b.grassA = { 168, 152, 84, 255 }; b.grassB = { 188, 164, 80, 255 };
    b.dry = { 196, 176, 104, 255 };
    b.canopyA = { 120, 138, 58, 255 }; b.canopyB = { 148, 148, 62, 255 };
    b.treeDensity = 0.25f; b.rockDensity = 0.5f;
    b.fogColor = { 0.78f, 0.76f, 0.70f }; b.fogDensity = 0.0035f;
    b.sunColor = { 1.12f, 0.94f, 0.62f };
    b.viewDistanceScale = 1.1f;
    b.hillAmp = 6.0f; b.hillFreq = 120.0f;
    return b;
}

static BiomeStyle Mirrormarsh() {
    BiomeStyle b = Vale();
    b.grassA = { 54, 96, 76, 255 }; b.grassB = { 70, 110, 74, 255 };
    b.rock = { 96, 100, 96, 255 }; b.dry = { 96, 118, 88, 255 };
    b.canopyA = { 52, 108, 84, 255 }; b.canopyB = { 74, 118, 74, 255 };
    b.trunk = { 72, 62, 48, 255 };
    b.treeDensity = 1.1f; b.rockDensity = 0.3f;
    b.fogColor = { 0.52f, 0.62f, 0.60f }; b.fogDensity = 0.0085f;
    b.skyZenith = { 0.30f, 0.42f, 0.52f };
    b.sunColor = { 0.92f, 0.90f, 0.72f };
    b.skyAmbient = { 0.36f, 0.46f, 0.46f }; b.groundAmbient = { 0.24f, 0.28f, 0.24f };
    b.viewDistanceScale = 0.65f;
    b.hillAmp = 3.5f; b.hillFreq = 60.0f;
    return b;
}

static BiomeStyle CinderBarrens() {
    BiomeStyle b = Vale();
    b.grassA = { 150, 108, 66, 255 }; b.grassB = { 170, 122, 62, 255 };
    b.rock = { 122, 92, 78, 255 }; b.dry = { 186, 140, 82, 255 };
    b.canopyA = { 120, 96, 48, 255 }; b.canopyB = { 140, 104, 44, 255 };
    b.trunk = { 76, 56, 44, 255 };
    b.treeDensity = 0.18f; b.rockDensity = 1.3f;
    b.fogColor = { 0.80f, 0.66f, 0.52f }; b.fogDensity = 0.0055f;
    b.skyZenith = { 0.40f, 0.36f, 0.52f };
    b.sunColor = { 1.15f, 0.82f, 0.52f };
    b.skyAmbient = { 0.48f, 0.42f, 0.38f }; b.groundAmbient = { 0.36f, 0.28f, 0.20f };
    b.viewDistanceScale = 0.95f;
    b.hillAmp = 13.0f; b.hillFreq = 80.0f;
    return b;
}

static BiomeStyle Highlands() {
    BiomeStyle b = Vale();
    b.grassA = { 96, 132, 88, 255 }; b.grassB = { 118, 142, 92, 255 };
    b.rock = { 140, 138, 134, 255 };
    b.canopyA = { 62, 118, 74, 255 }; b.canopyB = { 88, 128, 70, 255 };
    b.treeDensity = 0.4f; b.rockDensity = 1.0f;
    b.fogColor = { 0.72f, 0.78f, 0.90f }; b.fogDensity = 0.0035f;
    b.skyZenith = { 0.24f, 0.40f, 0.76f };
    b.viewDistanceScale = 1.15f;
    b.hillAmp = 24.0f; b.hillFreq = 110.0f;
    return b;
}

static BiomeStyle VerdantMaze() {
    BiomeStyle b = Vale();
    b.grassA = { 66, 150, 70, 255 }; b.grassB = { 96, 172, 62, 255 };
    b.canopyA = { 48, 138, 64, 255 }; b.canopyB = { 96, 164, 52, 255 };
    b.treeDensity = 2.6f; b.rockDensity = 0.35f;
    b.fogColor = { 0.62f, 0.74f, 0.68f }; b.fogDensity = 0.0065f;
    b.sunColor = { 1.02f, 0.98f, 0.72f };
    b.viewDistanceScale = 0.75f;
    b.hillAmp = 8.0f; b.hillFreq = 55.0f;
    return b;
}

static BiomeStyle Necropolis() {
    BiomeStyle b = Vale();
    b.grassA = { 108, 112, 104, 255 }; b.grassB = { 124, 126, 110, 255 };
    b.rock = { 110, 106, 112, 255 }; b.dry = { 132, 128, 118, 255 };
    b.canopyA = { 92, 100, 78, 255 }; b.canopyB = { 108, 108, 84, 255 };
    b.trunk = { 70, 62, 58, 255 };
    b.treeDensity = 0.3f; b.rockDensity = 0.8f;
    b.fogColor = { 0.64f, 0.62f, 0.70f }; b.fogDensity = 0.0070f;
    b.skyZenith = { 0.28f, 0.28f, 0.46f };
    b.sunColor = { 0.94f, 0.86f, 0.74f };
    b.skyAmbient = { 0.40f, 0.40f, 0.48f }; b.groundAmbient = { 0.26f, 0.25f, 0.28f };
    b.viewDistanceScale = 0.8f;
    b.hillAmp = 9.0f; b.hillFreq = 85.0f;
    return b;
}

static BiomeStyle StarfallCrater() {
    BiomeStyle b = Vale();
    b.grassA = { 76, 128, 118, 255 }; b.grassB = { 96, 146, 128, 255 };
    b.rock = { 104, 100, 130, 255 }; b.dry = { 120, 140, 150, 255 };
    b.canopyA = { 58, 120, 128, 255 }; b.canopyB = { 88, 140, 130, 255 };
    b.trunk = { 78, 70, 88, 255 };
    b.treeDensity = 0.7f; b.rockDensity = 0.9f;
    b.fogColor = { 0.60f, 0.62f, 0.84f }; b.fogDensity = 0.0060f;
    b.skyZenith = { 0.26f, 0.26f, 0.62f };
    b.sunColor = { 0.96f, 0.90f, 0.86f };
    b.skyAmbient = { 0.40f, 0.42f, 0.60f }; b.groundAmbient = { 0.28f, 0.26f, 0.34f };
    b.viewDistanceScale = 0.85f;
    b.hillAmp = 15.0f; b.hillFreq = 75.0f;
    return b;
}

// ---- registry -------------------------------------------------------------

static std::vector<ZoneDef> BuildWorld() {
    std::vector<ZoneDef> zones(WORLD_ZONES_X * WORLD_ZONES_Z);

    auto def = [&](int id, const char* name, int tier, BiomeStyle biome,
                   bool temple, int wayshrines, int monoliths, bool circle) {
        ZoneDef& z = zones[id];
        z.id = id; z.name = name; z.tier = tier; z.biome = biome;
        z.temple = temple; z.wayshrines = wayshrines; z.monoliths = monoliths;
        z.stoneCircle = circle;
    };

    // Grid layout (x right, z down):
    //   0 Skyreach   1 Thornwood   2 Starfall
    //   3 Cinder     4 ElderVale   5 Steppe
    //   6 Verdant    7 Mirrormarsh 8 Necropolis
    def(0, "Skyreach Highlands", 4, Highlands(),     false, 2, 8, false);
    def(1, "Thornwood",          2, Thornwood(),     false, 3, 4, true);
    def(2, "Starfall Crater",    4, StarfallCrater(),false, 2, 10, true);
    def(3, "Cinder Barrens",     3, CinderBarrens(), false, 2, 6, false);
    def(4, "Elder Vale",         1, Vale(),          true,  4, 6, true);
    def(5, "Gilded Steppe",      2, GildedSteppe(),  false, 3, 4, false);
    def(6, "Verdant Maze",       3, VerdantMaze(),   false, 2, 3, true);
    def(7, "Mirrormarsh",        3, Mirrormarsh(),   false, 3, 4, false);
    def(8, "Sunken Necropolis",  4, Necropolis(),    false, 2, 14, true);

    // Wire gates between grid neighbors. Offsets vary per shared edge so
    // gates are not all dead-center; both sides of an edge share the offset
    // so the crossing lines up spatially.
    for (int zz = 0; zz < WORLD_ZONES_Z; zz++) {
        for (int zx = 0; zx < WORLD_ZONES_X; zx++) {
            int id = zz * WORLD_ZONES_X + zx;
            if (zx + 1 < WORLD_ZONES_X) {          // east neighbor
                ZoneDef& a = zones[id];
                ZoneDef& b = zones[id + 1];
                float off = (float)(((zx * 7 + zz * 13) % 3) - 1) * 90.0f;
                int ga = (int)a.gates.size(), gb = (int)b.gates.size();
                a.gates.push_back({ EDGE_E, off, b.id, gb });
                b.gates.push_back({ EDGE_W, off, a.id, ga });
            }
            if (zz + 1 < WORLD_ZONES_Z) {          // south neighbor
                ZoneDef& a = zones[id];
                ZoneDef& b = zones[id + WORLD_ZONES_X];
                float off = (float)(((zx * 11 + zz * 5) % 3) - 1) * 90.0f;
                int ga = (int)a.gates.size(), gb = (int)b.gates.size();
                a.gates.push_back({ EDGE_S, off, b.id, gb });
                b.gates.push_back({ EDGE_N, off, a.id, ga });
            }
        }
    }
    return zones;
}

static const std::vector<ZoneDef>& World() {
    static std::vector<ZoneDef> world = BuildWorld();
    return world;
}

int ZoneCount() { return (int)World().size(); }
const ZoneDef& ZoneById(int id) { return World()[id]; }
