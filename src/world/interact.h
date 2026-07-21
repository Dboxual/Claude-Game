// Interaction framework: world interactables with proximity + facing
// targeting, and the wisp reward system (collectible motes that magnet to
// the player — the foundation for XP/loot payout feel in later phases).
// Works identically in both cameras: targeting uses the camera eye+forward.
#pragma once
#include "raylib.h"
#include <string>
#include <vector>

class ParticleSystem;
class Renderer;

enum class InteractType { HeartShrine, Wayshrine, ZoneGate };

struct Interactable {
    InteractType type;
    Vector3 pos;             // focus point (crystal / portal center)
    float radius = 3.4f;     // interaction reach from the player
    int lightIndex = -1;     // world light this boosts when activated
    float rearm = 0.0f;      // seconds until usable again
    float glow = 0.0f;       // 0..1 activation flash, decays; drives light boost
    std::string verb = "Draw Anima";
    int payload = -1;        // ZoneGate: index into the ZoneDef gate list
};

// A reward mote: drifts briefly, then homes to the player with rising speed.
struct Wisp {
    Vector3 pos;
    Vector3 vel;
    Color color = { 140, 245, 215, 255 };
    float age = 0.0f;
    float delay = 0.0f;      // scatter time before homing starts
    bool alive = false;
};

struct InteractEvents {
    int activatedIndex = -1;         // interactable triggered this tick
    int wispsCollected = 0;
    Vector3 lastCollectPos = {};
};

class InteractionSystem {
public:
    void Reset();
    int Add(const Interactable& item);

    // The single interactable the player could use right now (or -1):
    // within reach of the player AND roughly under the camera's gaze.
    int FindTarget(Vector3 playerPos, Vector3 eyePos, Vector3 eyeForward) const;

    // Fires `index` if armed: spawns a wisp burst, returns false if on cooldown.
    bool Activate(int index, int wispCount);
    // Shared reward path used by production activations and opt-in dev tests.
    void SpawnReward(Vector3 origin, int wispCount);

    InteractEvents Update(float dt, Vector3 playerChest, ParticleSystem& fx);

    const std::vector<Interactable>& Items() const { return items; }
    float LightBoost(int lightIndex) const;   // multiplier for world lights
    void DrawWisps(Renderer& r, float time) const;

private:
    std::vector<Interactable> items;
    std::vector<Wisp> wisps;
    unsigned int rngState = 0x715E9u;
    float Rand01();
};
