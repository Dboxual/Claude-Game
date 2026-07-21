// Owns the current ZoneInstance and runs gate transitions: fade to black,
// swap the zone at the fade peak (synchronous generation hides behind it),
// spawn at the matching gate facing inward, fade back in. Also applies the
// zone's biome atmosphere to the renderer on load.
#pragma once
#include "zone/zone_instance.h"

class Renderer;
class PlayerController;
class CameraRig;

class ZoneManager {
public:
    void Init(unsigned int worldSeed, Renderer& renderer);

    // Load a zone immediately (no fade). arriveGate -1 = zone default spawn;
    // otherwise the player appears at that gate, facing inward.
    void LoadZone(int zoneId, int arriveGate, PlayerController& player, CameraRig& rig);

    void RequestTransition(int gateIndex);       // gate of the CURRENT zone
    void Update(float dt, PlayerController& player, CameraRig& rig);

    bool Transitioning() const { return phase != Phase::None; }
    float FadeAlpha() const { return fade; }
    const char* TravelTargetName() const;        // valid while transitioning

    ZoneInstance& Current() { return current; }
    const ZoneDef& CurrentDef() const { return *current.def; }
    int CurrentId() const { return currentId; }

private:
    enum class Phase { None, FadeOut, FadeIn };
    void ApplyBiomeAtmosphere();

    ZoneInstance current;
    Renderer* renderer = nullptr;
    unsigned int worldSeed = 0;
    int currentId = -1;
    int pendingZone = -1, pendingGate = -1;
    Phase phase = Phase::None;
    float fade = 0.0f;
};
