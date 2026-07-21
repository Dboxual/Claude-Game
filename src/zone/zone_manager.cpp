#include "zone/zone_manager.h"
#include "core/log.h"
#include "core/mathx.h"
#include "player/camera_rig.h"
#include "player/controller.h"
#include "render/renderer.h"

static constexpr float FADE_SPEED = 2.8f;   // full fade in ~0.36 s

void ZoneInstance::Generate(unsigned int worldSeed, int zoneId) {
    def = &ZoneById(zoneId);
    double t0 = GetTime();
    terrain.Generate(worldSeed, *def);
    world.Generate(worldSeed, terrain, *def);

    interact.Reset();
    for (const ShrineInfo& s : world.Shrines())
        interact.Add({ s.heart ? InteractType::HeartShrine : InteractType::Wayshrine,
                       s.crystalPos, s.heart ? 4.5f : 3.4f, s.lightIndex,
                       0.0f, 0.0f, "Commune", -1 });
    for (const GateWorldInfo& g : world.Gates()) {
        const GateDef& gd = def->gates[g.gateIndex];
        interact.Add({ InteractType::ZoneGate, g.pos, 4.5f, -1, 0.0f, 0.0f,
                       std::string("Travel to ") + ZoneById(gd.targetZone).name,
                       g.gateIndex });
    }
    LOG_INFO("Zone '%s' generated in %d ms", def->name, (int)((GetTime() - t0) * 1000));
}

void ZoneInstance::Shutdown() {
    terrain.Shutdown();
    def = nullptr;
}

void ZoneManager::Init(unsigned int seed, Renderer& r) {
    worldSeed = seed;
    renderer = &r;
}

void ZoneManager::ApplyBiomeAtmosphere() {
    const BiomeStyle& b = CurrentDef().biome;
    LightingSystem& l = renderer->lighting;
    l.fogColor = b.fogColor;
    l.fogDensity = b.fogDensity;
    l.sunColor = b.sunColor;
    l.skyAmbient = b.skyAmbient;
    l.groundAmbient = b.groundAmbient;
    renderer->sky.horizonColor = b.fogColor;    // horizon melts into fog
    renderer->sky.zenithColor = b.skyZenith;
    renderer->sky.sunColor = { b.sunColor.x * 0.93f, b.sunColor.y * 0.93f,
                               b.sunColor.z * 0.93f };
}

void ZoneManager::LoadZone(int zoneId, int arriveGate, PlayerController& player,
                           CameraRig& rig) {
    if (currentId >= 0) current.Shutdown();
    current.Generate(worldSeed, zoneId);
    currentId = zoneId;
    ApplyBiomeAtmosphere();

    if (arriveGate >= 0 && arriveGate < (int)CurrentDef().gates.size()) {
        const GateDef& g = CurrentDef().gates[arriveGate];
        Vector2 gxz = GateXZ(g);
        Vector2 in = GateInwardDir(g.edge);
        Vector3 spawn = { gxz.x + in.x * 5.0f, 0, gxz.y + in.y * 5.0f };
        spawn.y = current.world.GroundHeightAt(spawn.x, spawn.z, 1000.0f);
        player.Spawn(spawn);
        rig.Init(GateInwardYaw(g.edge));
    }
    // arriveGate < 0: caller places the player (new game / loaded save).
}

void ZoneManager::RequestTransition(int gateIndex) {
    if (phase != Phase::None) return;
    if (gateIndex < 0 || gateIndex >= (int)CurrentDef().gates.size()) {
        LOG_WARN("Ignored invalid gate index %d in zone %s", gateIndex, CurrentDef().name);
        return;
    }
    const GateDef& g = CurrentDef().gates[gateIndex];
    pendingZone = g.targetZone;
    pendingGate = g.targetGate;
    phase = Phase::FadeOut;
    LOG_INFO("Travel: %s -> %s", CurrentDef().name, ZoneById(pendingZone).name);
}

const char* ZoneManager::TravelTargetName() const {
    return pendingZone >= 0 ? ZoneById(pendingZone).name : "";
}

void ZoneManager::Update(float dt, PlayerController& player, CameraRig& rig) {
    switch (phase) {
        case Phase::None:
            fade = fmaxf(fade - FADE_SPEED * dt, 0.0f);
            break;
        case Phase::FadeOut:
            fade += FADE_SPEED * dt;
            if (fade >= 1.0f) {
                fade = 1.0f;
                // Swap happens fully hidden; synchronous generation (~0.5 s)
                // reads as part of the transition.
                LoadZone(pendingZone, pendingGate, player, rig);
                pendingGate = -1;
                phase = Phase::FadeIn;
            }
            break;
        case Phase::FadeIn:
            fade -= FADE_SPEED * 0.7f * dt;
            if (fade <= 0.0f) {
                fade = 0.0f;
                pendingZone = -1;
                phase = Phase::None;
            }
            break;
    }
}
