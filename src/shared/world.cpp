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
        if (e.active && e.solid && !e.carried && aabbOverlap(box, e.bounds())) return true;
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
    e.carryable = def.carryable;
    e.gear = def.gear;
    e.weaponId = def.weaponId;
    e.maxHealth = def.maxHealth;
    e.respawnSeconds = respawnOverride >= 0.0f ? respawnOverride : def.respawnSeconds;
    e.harvestItemId = def.harvestItemId;
    e.harvestCount = def.harvestCount;
    e.harvestHits = def.harvestHits;
    e.toolClass = def.toolClass;
    e.itemId = def.itemId;
    e.itemCount = def.itemCount;
    e.lootItemId = def.lootItemId;
    e.lootCount = def.lootCount;
    e.hostile = def.hostile;
    e.contactDamage = def.contactDamage;
    e.id = nextEntityId_++;
    e.health = def.maxHealth;
    e.hitsLeft = def.harvestHits;
    entities_.push_back(std::move(e));
}

const WorldEntity* World::entityById(unsigned id) const {
    for (const WorldEntity& e : entities_) {
        if (e.id == id) return &e;
    }
    return nullptr;
}

WorldEntity* World::entityById(unsigned id) {
    for (WorldEntity& e : entities_) {
        if (e.id == id) return &e;
    }
    return nullptr;
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
                e.hitsLeft = e.harvestHits; // depleted nodes come back full
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
        if (!e.active || e.carried) continue;
        if (mask == RayMask::Interactables) {
            if (e.weaponId.empty() && !e.carryable && !e.gear &&
                e.harvestItemId.empty() && e.itemId.empty()) continue;
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

// The RPG sandbox test zone: a 40x40 m grassy glade ringed by earthen
// cliffs. Terrain only - trees, rocks, ore, mobs, and props are entities
// placed by Game::startTestWorld so they can respawn, be gathered, and be
// saved. No step-up mechanic yet, so elevation changes go through jumps.
void World::buildTestMap() {
    boxes_.clear();
    entities_.clear();

    const glm::vec3 grass{0.36f, 0.58f, 0.30f};
    const glm::vec3 grassLight{0.42f, 0.64f, 0.33f};
    const glm::vec3 dirt{0.52f, 0.40f, 0.26f};
    const glm::vec3 cliff{0.48f, 0.42f, 0.36f};
    const glm::vec3 cliffTop{0.40f, 0.52f, 0.30f};
    const glm::vec3 stone{0.55f, 0.55f, 0.58f};

    // Grass floor and the cliff ring boxing the zone in. The cliffs get a
    // grassy cap so the horizon reads "hills", not "arena walls".
    add({-20, -1, -20}, {20, 0, 20}, grass);
    add({-20, 0, -21}, {20, 4, -20}, cliff);   // north
    add({-20, 4, -21}, {20, 4.6f, -20}, cliffTop);
    add({-20, 0, 20}, {20, 4, 21}, cliff);     // south
    add({-20, 4, 20}, {20, 4.6f, 21}, cliffTop);
    add({-21, 0, -20}, {-20, 4, 20}, cliff);   // west
    add({-21, 4, -20}, {-20, 4.6f, 20}, cliffTop);
    add({20, 0, -20}, {21, 4, 20}, cliff);     // east
    add({20, 4, -20}, {21, 4.6f, 20}, cliffTop);

    // Dirt path from spawn north toward the goblin camp, with a fork east
    // to the mining corner. Thin plates on the grass (no step: 2 cm).
    add({-1.2f, 0, -14.0f}, {1.2f, 0.02f, 2.0f}, dirt);
    add({1.2f, 0, -9.5f}, {12.0f, 0.02f, -7.0f}, dirt);

    // Village green by spawn: a lighter grass patch marking "home".
    add({-6.5f, 0, 1.0f}, {-1.5f, 0.02f, 6.0f}, grassLight);

    // Mining corner (east): a low stone shelf the ore sits against - one
    // jumpable step so movement still has something to climb.
    add({13.0f, 0, -13.0f}, {19.5f, 1.0f, -8.0f}, stone, true);
    add({14.5f, 1.0f, -12.0f}, {19.5f, 1.8f, -9.0f}, stone, true);

    // Goblin camp mound (north): raised dirt disc so the camp reads as a
    // place from across the map.
    add({-9.0f, 0, -17.5f}, {-2.0f, 0.35f, -11.5f}, dirt, true);

    spawnPoint = {0.0f, 0.01f, 8.0f};
}
