#include "player/controller.h"
#include "core/clock.h"
#include "core/mathx.h"
#include "world/world.h"

// Movement tuning — the heart of game feel; adjust here, nowhere else.
static constexpr float WALK_SPEED = 4.3f;
static constexpr float SPRINT_SPEED = 7.3f;
static constexpr float ACCEL_RATE_GROUND = 11.0f;  // ExpDecay rate: higher = snappier
static constexpr float ACCEL_RATE_AIR = 2.6f;
static constexpr float GRAVITY = 23.0f;
static constexpr float TERMINAL_FALL = 32.0f;
static constexpr float JUMP_VELOCITY = 7.3f;       // ~1.15 m apex
static constexpr float COYOTE_TIME = 0.12f;
static constexpr float JUMP_BUFFER = 0.12f;
static constexpr float GROUND_SNAP = 0.45f;        // max step-down before airborne
static constexpr float STEP_UP = 0.55f;            // max ledge auto-step
static constexpr float STRIDE_LENGTH = 2.15f;      // meters between footsteps

void PlayerController::Spawn(Vector3 feetPos) {
    pos = posPrev = feetPos;
    vel = {};
    grounded = true;
    coyote = jumpBuffer = strideAccum = 0.0f;
}

PlayerEvents PlayerController::FixedUpdate(const PlayerIntents& in, float camYaw, const World& world) {
    PlayerEvents ev;
    posPrev = pos;
    const float dt = SIM_DT;

    // Camera-relative move direction (yaw 0 faces -Z).
    Vector3 fw = { sinf(camYaw), 0, -cosf(camYaw) };
    Vector3 rt = { cosf(camYaw), 0, sinf(camYaw) };
    Vector3 wish = Vector3Add(Vector3Scale(fw, in.move.y), Vector3Scale(rt, in.move.x));
    bool moving = Vector3LengthSqr(wish) > 0.001f;

    sprinting = in.sprintDown && moving;
    float targetSpeed = sprinting ? SPRINT_SPEED : WALK_SPEED;

    // Exponential approach doubles as both acceleration and friction, and it
    // is framerate-safe by construction.
    float rate = grounded ? ACCEL_RATE_GROUND : ACCEL_RATE_AIR;
    Vector3 targetVel = Vector3Scale(wish, targetSpeed);
    vel.x = ExpDecay(vel.x, targetVel.x, rate, dt);
    vel.z = ExpDecay(vel.z, targetVel.z, rate, dt);

    // Jump forgiveness: buffer the press, allow it shortly after a ledge.
    coyote = grounded ? COYOTE_TIME : fmaxf(coyote - dt, 0.0f);
    jumpBuffer = in.jumpPressed ? JUMP_BUFFER : fmaxf(jumpBuffer - dt, 0.0f);
    if (jumpBuffer > 0.0f && (grounded || coyote > 0.0f)) {
        vel.y = JUMP_VELOCITY;
        grounded = false;
        coyote = jumpBuffer = 0.0f;
        ev.jumped = true;
    }

    if (!grounded) vel.y = fmaxf(vel.y - GRAVITY * dt, -TERMINAL_FALL);

    pos = Vector3Add(pos, Vector3Scale(vel, dt));
    pos = world.ResolveCollisions(pos, RADIUS, HEIGHT);

    float ground = world.GroundHeightAt(pos.x, pos.z, posPrev.y);
    if (grounded) {
        float dy = ground - pos.y;
        if (dy >= -GROUND_SNAP && dy <= STEP_UP) {
            pos.y = ground;              // follow slopes and small steps
            vel.y = 0.0f;
        } else if (dy < -GROUND_SNAP) {
            grounded = false;            // walked off a ledge
        } else {
            // Wall taller than a step: undo horizontal motion into it.
            pos.x = posPrev.x;
            pos.z = posPrev.z;
            pos.y = world.GroundHeightAt(pos.x, pos.z, posPrev.y);
        }
    } else if (vel.y <= 0.0f && pos.y <= ground) {
        ev.landed = true;
        ev.landSpeed = -vel.y;
        pos.y = ground;
        vel.y = 0.0f;
        grounded = true;
    }

    // Footstep cadence from actual distance covered.
    if (grounded && SpeedXZ() > 1.0f) {
        strideAccum += SpeedXZ() * dt;
        if (strideAccum >= STRIDE_LENGTH) {
            strideAccum -= STRIDE_LENGTH;
            ev.footstep = true;
        }
    } else {
        strideAccum = STRIDE_LENGTH * 0.7f;   // first step lands quickly
    }
    return ev;
}
