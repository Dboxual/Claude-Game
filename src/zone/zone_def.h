// Zone definitions. The world is a graph of HUGE zones; each zone is an
// isolated simulation island (the future one-server-per-zone boundary, the
// same shape Albion uses). Everything about a zone derives from its ZoneDef
// + the world seed — generation is deterministic per (worldSeed, zoneId).
#pragma once
#include "raylib.h"
#include <vector>

// Every zone is a square of this size, in meters.
constexpr float ZONE_SIZE = 1280.0f;
constexpr float ZONE_HALF = ZONE_SIZE * 0.5f;

// Edge indices for gates. N faces -Z, S faces +Z.
enum ZoneEdge { EDGE_N = 0, EDGE_E = 1, EDGE_S = 2, EDGE_W = 3 };

struct GateDef {
    int edge;            // ZoneEdge
    float offset;        // position along the edge, meters from its center
    int targetZone;      // zone id this gate leads to
    int targetGate;      // gate index inside the target zone's gate list
};

// The visual + generation personality of a zone.
struct BiomeStyle {
    // terrain paint
    Color grassA, grassB;        // ground base blend
    Color rock;                  // steep slopes
    Color dry;                   // plateau / clearings accent
    // vegetation
    Color canopyA, canopyB, trunk;
    float treeDensity;           // trees per 1000 m^2 (vale baseline ~0.9)
    float rockDensity;           // rocks per 1000 m^2
    // atmosphere (applied to lighting + sky on zone load)
    Vector3 fogColor;
    float fogDensity;
    Vector3 skyZenith;
    Vector3 sunColor;
    Vector3 skyAmbient, groundAmbient;
    float viewDistanceScale;     // dense/foggy biomes see shorter
    // terrain shape
    float hillAmp;               // base hill amplitude (vale = 11)
    float hillFreq;              // base wavelength divisor (vale = 95)
};

struct ZoneDef {
    int id;
    const char* name;
    int tier;                    // 1 safe .. 4 lawless (risk rules later)
    bool temple;                 // has the central temple plateau (Elder Vale)
    int wayshrines;              // scattered shrine count
    int monoliths;
    bool stoneCircle;
    BiomeStyle biome;
    std::vector<GateDef> gates;

    unsigned int SeedFor(unsigned int worldSeed) const {
        return worldSeed * 2654435761u + (unsigned int)id * 40503u;
    }
};

// The starter world: a 3x3 grid, Elder Vale in the center (id 4).
constexpr int WORLD_ZONES_X = 3;
constexpr int WORLD_ZONES_Z = 3;
constexpr int DEFAULT_ZONE = 4;

int ZoneCount();
const ZoneDef& ZoneById(int id);

// Gate anchor position in zone-local world space (y = 0; sample terrain).
Vector2 GateXZ(const GateDef& g);
// Direction pointing from the gate into the zone interior (unit XZ).
Vector2 GateInwardDir(int edge);
// Yaw (radians, 0 faces -Z) looking inward from this gate.
float GateInwardYaw(int edge);
