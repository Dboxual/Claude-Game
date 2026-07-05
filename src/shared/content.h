#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

// Base game content: entity definitions the sandbox can spawn (weapon
// pickups, bots, props) and the weapons the player can hold. Phase 2 still
// hard-codes the built-in set in registerBuiltins(); the registry is the
// seam where data-driven content lands later (content packs register extra
// defs at startup). Gameplay and world files only ever reference defs by
// their stable string id, so moving definitions from C++ to content files
// does not touch world saves or spawn code.

enum class ContentCategory { Weapon, Bot, Prop, Pickup };

// --- weapons (held by the player) ---------------------------------------

enum class WeaponKind { Melee, Hitscan };

// Gameplay stats only. Anything cosmetic (future models, skins, sounds)
// must hang off the id elsewhere so a skinned karambit stays identical in
// combat to the base one.
struct WeaponDef {
    std::string id;          // "fists", "karambit", "glock"
    std::string displayName;
    WeaponKind kind = WeaponKind::Melee;
    float damage = 0.0f;
    float range = 1.5f;          // melee reach / max hitscan distance, meters
    float cooldownSeconds = 0.5f;
};

// --- world entities (things placed in a world) ---------------------------

// One box of a multi-part visual, relative to the entity position
// (bottom-center). Offsets are part centers. Cosmetic only: collision and
// interaction always use EntityDef::size.
struct VisualPart {
    glm::vec3 offset{0.0f};
    glm::vec3 size{0.1f};
    glm::vec3 color{1.0f};
};

struct EntityDef {
    std::string id;          // stable id, used in world save files
    std::string displayName; // shown in menus
    ContentCategory category = ContentCategory::Prop;
    glm::vec3 size{1.0f};    // collision/interaction AABB; origin bottom-center
    glm::vec3 color{1.0f};   // fallback single-box visual
    bool solid = false;      // solid entities join world collision
    bool spawnable = true;   // listed in the dev spawn menu

    std::string weaponId;        // non-empty = pickup granting this weapon
    float maxHealth = 0.0f;      // >0 = damageable (training dummy)
    float respawnSeconds = 0.0f; // default respawn after pickup/death; 0 = never

    std::vector<VisualPart> visual; // multi-box visual; empty = one size/color box
};

class ContentRegistry {
public:
    void registerBuiltins();

    const EntityDef* find(const std::string& id) const;
    const std::vector<EntityDef>& defs() const { return defs_; }

    const WeaponDef* findWeapon(const std::string& id) const;
    const std::vector<WeaponDef>& weapons() const { return weapons_; }

private:
    std::vector<EntityDef> defs_;
    std::vector<WeaponDef> weapons_;
};
