// One fully-simulated zone: terrain + contents + interactables. This is the
// future one-server-per-zone boundary — everything a zone simulates lives
// behind this struct, and Game only ever holds the current instance.
#pragma once
#include "world/interact.h"
#include "world/terrain.h"
#include "world/world.h"
#include "zone/zone_def.h"

struct ZoneInstance {
    const ZoneDef* def = nullptr;
    Terrain terrain;
    World world;
    InteractionSystem interact;

    void Generate(unsigned int worldSeed, int zoneId);
    void Shutdown();
};
