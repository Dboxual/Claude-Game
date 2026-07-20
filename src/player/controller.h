// Kinematic character movement, simulated at the fixed tick. Feel targets:
// snappy acceleration, forgiving jumps (coyote time + input buffer), clean
// ground snapping on slopes. Returns feel events (jump/land/footstep) each
// tick so game glue can fire audio/particles/camera reactions.
#pragma once
#include "raylib.h"
#include "raymath.h"
#include <cmath>

class World;

struct PlayerIntents {
    Vector2 move = { 0, 0 };     // camera-relative, +y forward, normalized
    bool jumpPressed = false;
    bool sprintDown = false;
};

struct PlayerEvents {
    bool jumped = false;
    bool landed = false;
    float landSpeed = 0.0f;      // downward m/s at impact
    bool footstep = false;
};

class PlayerController {
public:
    static constexpr float RADIUS = 0.35f;
    static constexpr float HEIGHT = 1.75f;
    static constexpr float EYE = 1.62f;

    void Spawn(Vector3 feetPos);
    PlayerEvents FixedUpdate(const PlayerIntents& in, float camYaw, const World& world);

    // Feet position for rendering, lerped between the last two sim states.
    Vector3 InterpPos(float alpha) const { return Vector3Lerp(posPrev, pos, alpha); }
    float SpeedXZ() const { return sqrtf(vel.x * vel.x + vel.z * vel.z); }

    Vector3 pos = {};        // feet
    Vector3 posPrev = {};
    Vector3 vel = {};
    bool grounded = false;
    bool sprinting = false;

private:
    float coyote = 0.0f;
    float jumpBuffer = 0.0f;
    float strideAccum = 0.0f;
};
