#include "shared/world.h"

#include <algorithm>
#include <cmath>

float rayVsAABB(const glm::vec3& origin, const glm::vec3& dir, const AABB& box) {
    float tMin = 0.0f;
    float tMax = 1e30f;
    for (int i = 0; i < 3; ++i) {
        if (std::fabs(dir[i]) < 1e-8f) {
            if (origin[i] < box.min[i] || origin[i] > box.max[i]) return -1.0f;
            continue;
        }
        float inv = 1.0f / dir[i];
        float t1 = (box.min[i] - origin[i]) * inv;
        float t2 = (box.max[i] - origin[i]) * inv;
        if (t1 > t2) std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
        if (tMin > tMax) return -1.0f;
    }
    return tMin;
}

void World::add(glm::vec3 min, glm::vec3 max, glm::vec3 color, bool checkerTop) {
    boxes_.push_back(WorldBox{AABB{min, max}, color, checkerTop});
}

bool World::overlapsAny(const AABB& box) const {
    for (const WorldBox& wb : boxes_) {
        if (aabbOverlap(box, wb.box)) return true;
    }
    for (const WorldEntity& e : entities_) {
        if (e.active && e.solid && aabbOverlap(box, e.bounds())) return true;
    }
    return false;
}

void World::addEntity(const EntityDef& def, const glm::vec3& pos, float respawnOverride) {
    WorldEntity e;
    e.defId = def.id;
    e.pos = pos;
    e.size = def.size;
    e.color = def.color;
    e.solid = def.solid;
    e.weaponId = def.weaponId;
    e.maxHealth = def.maxHealth;
    e.respawnSeconds = respawnOverride >= 0.0f ? respawnOverride : def.respawnSeconds;
    e.health = def.maxHealth;
    entities_.push_back(std::move(e));
}

void World::tick(float dt) {
    for (WorldEntity& e : entities_) {
        if (e.hitFlash > 0.0f) e.hitFlash -= dt;
        if (!e.active) {
            e.respawnTimer -= dt;
            if (e.respawnTimer <= 0.0f) {
                e.active = true;
                e.health = e.maxHealth;
                e.hitFlash = 0.0f;
            }
        }
    }
}

float World::raycastGeometry(const glm::vec3& origin, const glm::vec3& dir, float maxT) const {
    float nearest = maxT;
    for (const WorldBox& wb : boxes_) {
        float t = rayVsAABB(origin, dir, wb.box);
        if (t >= 0.0f && t < nearest) nearest = t;
    }
    return nearest;
}

World::EntityHit World::raycastEntities(const glm::vec3& origin, const glm::vec3& dir,
                                        float maxT, RayMask mask) const {
    EntityHit hit;
    float nearest = maxT;
    for (size_t i = 0; i < entities_.size(); ++i) {
        const WorldEntity& e = entities_[i];
        if (!e.active) continue;
        if (mask == RayMask::Pickups) {
            if (e.weaponId.empty()) continue;
        } else { // AttackTargets: bullets pass through non-solid pickups
            if (!e.solid && e.maxHealth <= 0.0f) continue;
        }
        float t = rayVsAABB(origin, dir, e.bounds());
        if (t >= 0.0f && t < nearest) {
            nearest = t;
            hit.index = int(i);
            hit.t = t;
        }
    }
    return hit;
}

bool World::damageEntity(int index, float damage) {
    if (index < 0 || index >= int(entities_.size())) return false;
    WorldEntity& e = entities_[size_t(index)];
    if (!e.active || e.maxHealth <= 0.0f) return false;
    e.health -= damage;
    e.hitFlash = 0.15f;
    if (e.health > 0.0f) return false;

    if (e.respawnSeconds > 0.0f) {
        e.active = false;
        e.respawnTimer = e.respawnSeconds;
    } else {
        entities_.erase(entities_.begin() + index);
    }
    return true;
}

void World::consumePickup(int index) {
    if (index < 0 || index >= int(entities_.size())) return;
    WorldEntity& e = entities_[size_t(index)];
    if (e.respawnSeconds > 0.0f) {
        e.active = false;
        e.respawnTimer = e.respawnSeconds;
    } else {
        entities_.erase(entities_.begin() + index);
    }
}

// Sandbox base for created worlds: an enclosed 40x40 m flat plate. Placed
// entities come from the world save file; the base geometry is implied by
// the format version, so saves stay tiny and readable.
void World::buildFlatMap() {
    boxes_.clear();
    entities_.clear();

    const glm::vec3 floorGrey{0.62f, 0.64f, 0.66f};
    const glm::vec3 wallSlate{0.35f, 0.42f, 0.52f};

    add({-20, -1, -20}, {20, 0, 20}, floorGrey, true);
    add({-20, 0, -21}, {20, 4, -20}, wallSlate); // north
    add({-20, 0, 20}, {20, 4, 21}, wallSlate);   // south
    add({-21, 0, -20}, {-20, 4, 20}, wallSlate); // west
    add({20, 0, -20}, {21, 4, 20}, wallSlate);   // east

    spawnPoint = {0.0f, 0.01f, 0.0f};
}

// A 40x40 m enclosed arena. There is intentionally no step-up mechanic yet,
// so every walkable surface is flat and elevation changes go through jumps.
void World::buildTestMap() {
    boxes_.clear();
    entities_.clear();

    const glm::vec3 floorGrey{0.62f, 0.64f, 0.66f};
    const glm::vec3 wallSlate{0.35f, 0.42f, 0.52f};
    const glm::vec3 crateOrange{0.85f, 0.55f, 0.25f};
    const glm::vec3 stepTeal{0.30f, 0.65f, 0.60f};
    const glm::vec3 tunnelPurple{0.55f, 0.45f, 0.65f};
    const glm::vec3 pillarRed{0.70f, 0.35f, 0.35f};

    // Floor (checkered so speed is easy to read) and perimeter walls.
    add({-20, -1, -20}, {20, 0, 20}, floorGrey, true);
    add({-20, 0, -21}, {20, 4, -20}, wallSlate); // north
    add({-20, 0, 20}, {20, 4, 21}, wallSlate);   // south
    add({-21, 0, -20}, {-20, 4, 20}, wallSlate); // west
    add({20, 0, -20}, {21, 4, 20}, wallSlate);   // east

    // Crate row east of spawn: 0.5 m and 1.0 m (both jumpable with the ~1.3 m
    // jump apex), 1.5 m (needs a hop from a crate - a reference obstacle).
    add({4.0f, 0, -2.0f}, {5.5f, 0.5f, -0.5f}, crateOrange, true);
    add({7.0f, 0, -2.0f}, {8.5f, 1.0f, -0.5f}, crateOrange, true);
    add({10.0f, 0, -2.0f}, {11.5f, 1.5f, -0.5f}, crateOrange, true);

    // Jump staircase up to a 2 m platform (0.5 m rises, all jumpable).
    add({-6, 0, 6}, {-4, 0.5f, 8}, stepTeal, true);
    add({-8, 0, 6}, {-6, 1.0f, 8}, stepTeal, true);
    add({-10, 0, 6}, {-8, 1.5f, 8}, stepTeal, true);
    add({-14, 0, 6}, {-10, 2.0f, 10}, stepTeal, true);

    // Crouch tunnel: 2 m wide gap with a roof 1.4 m up - stand height (1.8)
    // does not fit, crouch height (1.3) does.
    add({2, 0, 9.6f}, {8, 2.2f, 10.0f}, tunnelPurple);     // near side wall
    add({2, 0, 12.0f}, {8, 2.2f, 12.4f}, tunnelPurple);    // far side wall
    add({2, 1.4f, 10.0f}, {8, 2.2f, 12.4f}, tunnelPurple); // roof

    // Pillars for strafe practice.
    add({-4.5f, 0, -8.5f}, {-3.5f, 3, -7.5f}, pillarRed);
    add({-7.5f, 0, -8.5f}, {-6.5f, 3, -7.5f}, pillarRed);
    add({-10.5f, 0, -8.5f}, {-9.5f, 3, -7.5f}, pillarRed);

    // A long wall segment mid-map for wall-sliding and counter-strafe drills.
    add({-2, 0, -14}, {14, 2.5f, -13.4f}, wallSlate);

    spawnPoint = {0.0f, 0.01f, 0.0f};
}
