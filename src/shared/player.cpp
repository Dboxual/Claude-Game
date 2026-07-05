#include "shared/player.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kSkin = 0.001f;       // gap kept between player and geometry
constexpr float kGroundProbe = 0.05f; // how far below the feet counts as ground
constexpr float kKillPlaneY = -25.0f;
} // namespace

void Player::respawn(const glm::vec3& p) {
    pos = p;
    vel = glm::vec3(0.0f);
    grounded = false;
    crouched = false;
    justJumped = false;
}

float Player::height(const MovementConfig& cfg) const {
    return crouched ? cfg.crouchHeight : cfg.standHeight;
}

float Player::horizontalSpeed() const {
    return std::sqrt(vel.x * vel.x + vel.z * vel.z);
}

AABB Player::bounds(float height) const {
    return AABB{{pos.x - radius_, pos.y, pos.z - radius_},
                {pos.x + radius_, pos.y + height, pos.z + radius_}};
}

void Player::applyFriction(const MovementConfig& cfg, float dt) {
    float speed = horizontalSpeed();
    if (speed < 0.0001f) {
        vel.x = vel.z = 0.0f;
        return;
    }
    // Below stopSpeed, friction acts as if you were moving at stopSpeed so you
    // come to a definite halt instead of asymptotically sliding.
    float control = std::max(speed, cfg.stopSpeed);
    float drop = control * cfg.friction * dt;
    float scale = std::max(speed - drop, 0.0f) / speed;
    vel.x *= scale;
    vel.z *= scale;
}

void Player::accelerate(const glm::vec3& wishDir, float wishSpeed, float accel, float dt) {
    float currentSpeed = vel.x * wishDir.x + vel.z * wishDir.z;
    float addSpeed = wishSpeed - currentSpeed;
    if (addSpeed <= 0.0f) return;
    float accelSpeed = std::min(accel * wishSpeed * dt, addSpeed);
    vel.x += accelSpeed * wishDir.x;
    vel.z += accelSpeed * wishDir.z;
}

void Player::airAccelerate(const glm::vec3& wishDir, float wishSpeed, const MovementConfig& cfg, float dt) {
    // The cap on wish speed (not on velocity!) is what limits air control:
    // you can only add up to airSpeedCap of speed along any one direction,
    // but redirecting velocity by strafing remains possible.
    float cappedWish = std::min(wishSpeed, cfg.airSpeedCap);
    float currentSpeed = vel.x * wishDir.x + vel.z * wishDir.z;
    float addSpeed = cappedWish - currentSpeed;
    if (addSpeed <= 0.0f) return;
    float accelSpeed = std::min(cfg.airAccel * wishSpeed * dt, addSpeed);
    vel.x += accelSpeed * wishDir.x;
    vel.z += accelSpeed * wishDir.z;
}

void Player::tick(const PlayerInput& in, const World& world, const MovementConfig& cfg, float dt) {
    radius_ = cfg.playerRadius;
    justJumped = false;

    // Crouch is instant; standing back up requires headroom.
    if (in.crouch) {
        crouched = true;
    } else if (crouched) {
        AABB standBox = bounds(cfg.standHeight);
        standBox.min += glm::vec3(kSkin);
        standBox.max -= glm::vec3(kSkin);
        if (!world.overlapsAny(standBox)) crouched = false;
    }
    float h = height(cfg);

    // Wish direction on the ground plane from look yaw.
    glm::vec3 forward(std::sin(in.yaw), 0.0f, -std::cos(in.yaw));
    glm::vec3 right(std::cos(in.yaw), 0.0f, std::sin(in.yaw));
    glm::vec3 wishDir = forward * in.moveForward + right * in.moveRight;
    float wishLen = glm::length(wishDir);
    bool hasWish = wishLen > 0.001f;
    if (hasWish) wishDir /= wishLen;

    float wishSpeed = cfg.runSpeed;
    if (crouched) wishSpeed = cfg.crouchSpeed;
    else if (in.walk) wishSpeed = cfg.walkSpeed;

    if (grounded) {
        applyFriction(cfg, dt);
        if (hasWish) accelerate(wishDir, wishSpeed, cfg.groundAccel, dt);
        if (in.jump) {
            vel.y = cfg.jumpSpeed;
            grounded = false;
            justJumped = true;
        }
    } else if (hasWish) {
        airAccelerate(wishDir, wishSpeed, cfg, dt);
    }
    if (!grounded) vel.y -= cfg.gravity * dt;

    slideMove(world, vel * dt, h);
    updateGrounded(world, h);

    if (pos.y < kKillPlaneY) respawn(world.spawnPoint);
}

// Axis-separated move-and-clamp against every world box and every solid
// entity (crates, dummies). Horizontal axes go first so you slide along
// walls; Y last so you land cleanly on top of boxes you moved over this tick.
void Player::slideMove(const World& world, const glm::vec3& delta, float height) {
    const int axisOrder[3] = {0, 2, 1};
    for (int axis : axisOrder) {
        if (delta[axis] == 0.0f) continue;
        pos[axis] += delta[axis];

        auto clampAgainst = [&](const AABB& box) {
            AABB pb = bounds(height);
            if (!aabbOverlap(pb, box)) return;
            if (axis == 1) {
                if (delta.y > 0.0f) pos.y = box.min.y - height - kSkin; // head hit ceiling
                else pos.y = box.max.y + kSkin;                         // feet hit floor
            } else {
                if (delta[axis] > 0.0f) pos[axis] = box.min[axis] - radius_ - kSkin;
                else pos[axis] = box.max[axis] + radius_ + kSkin;
            }
            vel[axis] = 0.0f;
        };

        for (const WorldBox& wb : world.boxes()) clampAgainst(wb.box);
        for (const WorldEntity& e : world.entities()) {
            if (e.active && e.solid && !e.carried) clampAgainst(e.bounds());
        }
    }
}

void Player::updateGrounded(const World& world, float height) {
    (void)height;
    if (vel.y > 0.1f) { // rising: definitely airborne
        grounded = false;
        return;
    }
    // Probe a slightly shrunk footprint just below the feet, so brushing a
    // wall does not count as standing on it.
    float shrink = 0.01f;
    AABB probe{{pos.x - radius_ + shrink, pos.y - kGroundProbe, pos.z - radius_ + shrink},
               {pos.x + radius_ - shrink, pos.y + 0.01f, pos.z + radius_ - shrink}};
    grounded = world.overlapsAny(probe);
}
