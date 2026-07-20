// Pooled CPU particle system with billboard rendering. Two blend groups
// (alpha for dust/debris, additive for magic/glow) drawn in two passes.
// The budget scales with the particle-density graphics setting; emitters
// silently drop particles when the pool is full — never allocate at runtime.
#pragma once
#include "raylib.h"

struct Particle {
    Vector3 pos = {};
    Vector3 vel = {};
    float life = 0.0f;       // remaining seconds; <= 0 means dead
    float maxLife = 1.0f;
    float sizeA = 0.2f, sizeB = 0.05f;   // start/end world size
    Color colA = WHITE, colB = WHITE;    // start/end tint (alpha included)
    float gravity = 0.0f;    // m/s^2 downward (negative floats upward)
    float drag = 0.0f;       // velocity damping rate
    bool additive = false;
};

class ParticleSystem {
public:
    void Init();
    void Shutdown();
    void SetDensity(float d) { density = d; }   // 0.25 .. 1.0

    void Update(float dt);
    void Draw(const Camera3D& cam);
    int AliveCount() const { return alive; }

    // Core emit — all helpers funnel through here.
    void Emit(const Particle& p);

    // Feel helpers used by game glue.
    void FootstepPuff(Vector3 feet, Vector3 vel);
    void LandBurst(Vector3 feet, float impactSpeed);
    void ShrineBurst(Vector3 center, Color color);
    void WispTrail(Vector3 pos, Color color);
    void CollectBurst(Vector3 pos, Color color);
    void AmbientMotes(Vector3 around, float dt);   // call every frame

private:
    static constexpr int MAX_PARTICLES = 4096;
    Particle pool[MAX_PARTICLES];
    int alive = 0;
    float density = 1.0f;
    float moteAccum = 0.0f;
    Texture2D softDot = {};
    unsigned int rngState = 0x12345u;
    float Rand01();
    float RandRange(float lo, float hi) { return lo + (hi - lo) * Rand01(); }
};
