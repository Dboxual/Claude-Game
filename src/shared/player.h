#pragma once

#include "shared/config.h"
#include "shared/world.h"

#include <glm/glm.hpp>

struct PlayerInput {
    float moveForward = 0.0f; // +1 = forward, -1 = back
    float moveRight = 0.0f;   // +1 = right, -1 = left
    float yaw = 0.0f;         // radians, 0 looks down -Z
    bool jump = false;        // buffered "wants to jump this tick"
    bool crouch = false;
    bool walk = false;
};

// First-person movement controller on a fixed tick.
//
// The acceleration model is the classic add-speed-along-wishdir formulation:
//   addSpeed = wishSpeed - dot(velocity, wishDir)
// which is what produces the tactical-FPS feel: counter-strafing stops you
// far faster than friction (pressing the opposite key makes addSpeed huge),
// and clamping wishSpeed in the air gives limited air control.
class Player {
public:
    glm::vec3 pos{0.0f}; // feet: bottom-center of the collision box
    glm::vec3 vel{0.0f};
    bool grounded = false;
    bool crouched = false;
    bool justJumped = false; // true for the tick a jump was consumed

    void respawn(const glm::vec3& p);
    void tick(const PlayerInput& in, const World& world, const MovementConfig& cfg, float dt);

    float height(const MovementConfig& cfg) const;
    float horizontalSpeed() const;

private:
    AABB bounds(float height) const;
    void applyFriction(const MovementConfig& cfg, float dt);
    void accelerate(const glm::vec3& wishDir, float wishSpeed, float accel, float dt);
    void airAccelerate(const glm::vec3& wishDir, float wishSpeed, const MovementConfig& cfg, float dt);
    void slideMove(const World& world, const glm::vec3& delta, float height);
    void updateGrounded(const World& world, float height);

    float radius_ = 0.4f; // refreshed from config each tick
};
