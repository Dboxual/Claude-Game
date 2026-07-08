#pragma once

#include "shared/content.h"

#include <glm/glm.hpp>

#include <vector>

struct AABB {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
};

// Strict overlap: faces merely touching do not count.
inline bool aabbOverlap(const AABB& a, const AABB& b) {
    return a.min.x < b.max.x && a.max.x > b.min.x &&
           a.min.y < b.max.y && a.max.y > b.min.y &&
           a.min.z < b.max.z && a.max.z > b.min.z;
}

struct WorldBox {
    AABB box;
    glm::vec3 color{1.0f};
    bool checkerTop = false; // draw a checker pattern on upward faces
    float emissive = 0.0f;   // 0 = lit, 1 = full-bright (neon/LED accent strips)
};

// Slab test: distance along the (normalized) ray to the box, or a negative
// value on miss. Origins inside the box hit at t = 0 (point-blank melee).
float rayVsAABB(const glm::vec3& origin, const glm::vec3& dir, const AABB& box);

// A spawned sandbox object (pickup, bot, prop). Definition fields are
// snapshot from the EntityDef at spawn/load time; only defId + pos +
// respawnSeconds go into save files. Runtime state (health, respawn timers)
// is never saved: worlds always load with everything alive and in place.
struct WorldEntity {
    std::string defId;
    glm::vec3 pos{0.0f}; // bottom-center, same convention as the player
    glm::vec3 size{1.0f};
    glm::vec3 color{1.0f};
    bool solid = false;
    bool carryable = false;      // light prop the player may E-carry
    bool gear = false;           // equipment pickup (shield): E equips it
    std::string weaponId;        // non-empty = pickup granting this weapon
    float maxHealth = 0.0f;      // >0 = damageable
    float respawnSeconds = 0.0f; // 0 = never respawns (consumed/destroyed for good)

    // RPG snapshot (see EntityDef for semantics).
    std::string harvestItemId;   // non-empty = resource node
    int harvestCount = 1;
    int harvestHits = 3;
    std::string toolClass;
    std::string itemId;          // non-empty = ground item pickup
    int itemCount = 1;
    std::string lootItemId;      // dropped into the world on destruction
    int lootCount = 1;
    bool hostile = false;        // simple mob: chases + contact attack
    float contactDamage = 6.0f;

    // Runtime.
    unsigned id = 0;         // stable per-world handle (indices shift on erase)
    bool active = true;      // false = picked up / destroyed, waiting to respawn
    bool carried = false;    // held by the player: no collision, no ray hits
    float health = 0.0f;
    float respawnTimer = 0.0f;
    float hitFlash = 0.0f;   // seconds of red flash left after taking a hit
    int hitsLeft = 0;        // resource nodes: gathers remaining before depletion

    AABB bounds() const {
        glm::vec3 half(size.x * 0.5f, 0.0f, size.z * 0.5f);
        return AABB{pos - half, pos + glm::vec3(half.x, size.y, half.z)};
    }
};

struct WorldTemplate; // shared/world_template.h

class World {
public:
    void buildTestMap();
    // Replace geometry + spawn point with a template's layout (entities are
    // cleared; the caller places the template's starting entities and/or the
    // saved ones through addEntity so content lookups stay caller-side).
    void buildFromTemplate(const WorldTemplate& t);
    bool overlapsAny(const AABB& box) const;
    const std::vector<WorldBox>& boxes() const { return boxes_; }

    // respawnOverride >= 0 replaces the def's default (per-instance, saved).
    void addEntity(const EntityDef& def, const glm::vec3& pos, float respawnOverride = -1.0f);
    void clearEntities() { entities_.clear(); }
    const std::vector<WorldEntity>& entities() const { return entities_; }

    // Fixed-tick update: respawn timers and hit-flash decay.
    void tick(float dt);

    // Nearest static-geometry hit within maxT, or maxT if the ray is clear.
    float raycastGeometry(const glm::vec3& origin, const glm::vec3& dir, float maxT) const;

    // Nearest active, non-carried entity the ray hits within maxT.
    // AttackTargets sees solid or damageable entities (bullets pass through
    // pickups); Interactables sees weapon pickups and carryable props.
    // Callers occlude with raycastGeometry first.
    enum class RayMask { AttackTargets, Interactables };
    struct EntityHit {
        int index = -1;
        float t = 0.0f;
    };
    EntityHit raycastEntities(const glm::vec3& origin, const glm::vec3& dir,
                              float maxT, RayMask mask) const;

    // Stable-handle lookup; nullptr once the entity was erased. Prefer ids
    // over indices for anything held across frames (e.g. a carried prop).
    const WorldEntity* entityById(unsigned id) const;
    WorldEntity* entityById(unsigned id);

    // Applies damage; true if this hit destroyed the entity. Entities with
    // respawnSeconds == 0 are erased outright, so the index (and any
    // EntityHit) is invalid after a destroying call.
    bool damageEntity(int index, float damage);

    // Consumes a pickup: deactivates it for its respawn time, or erases it
    // if it never respawns (same index caveat as damageEntity).
    void consumePickup(int index);

    glm::vec3 spawnPoint{0.0f, 0.01f, 0.0f};

private:
    void add(glm::vec3 min, glm::vec3 max, glm::vec3 color, bool checkerTop = false);
    std::vector<WorldBox> boxes_;
    std::vector<WorldEntity> entities_;
    unsigned nextEntityId_ = 1;
};
