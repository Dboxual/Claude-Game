#include "shared/content.h"

void ContentRegistry::registerBuiltins() {
    defs_.clear();
    weapons_.clear();

    // --- weapons. Knife-style stats are deliberately ordinary: the karambit
    // is a skin-tier knife, not a power item.
    {
        WeaponDef w;
        w.id = "fists";
        w.displayName = "Fists";
        w.kind = WeaponKind::Melee;
        w.damage = 10.0f;
        w.range = 1.6f;
        w.cooldownSeconds = 0.4f;
        weapons_.push_back(w);
    }
    {
        WeaponDef w;
        w.id = "karambit";
        w.displayName = "Karambit";
        w.kind = WeaponKind::Melee;
        w.damage = 35.0f;
        w.range = 1.9f;
        w.cooldownSeconds = 0.5f;
        weapons_.push_back(w);
    }
    {
        WeaponDef w;
        w.id = "glock";
        w.displayName = "Glock";
        w.kind = WeaponKind::Hitscan;
        w.damage = 25.0f;
        w.range = 60.0f;
        w.cooldownSeconds = 0.15f; // semi-auto; ammo is a later phase
        weapons_.push_back(w);
    }

    // --- entity defs. 'fists' has no pickup: it is the base loadout.

    const glm::vec3 gunMetal{0.16f, 0.17f, 0.20f};
    const glm::vec3 gunGrip{0.24f, 0.22f, 0.20f};
    const glm::vec3 blade{0.78f, 0.83f, 0.88f};
    const glm::vec3 handleDark{0.12f, 0.12f, 0.14f};

    {
        // Karambit pickup: handle + finger ring at the back, blade segments
        // stepping down toward the tip to read as a curved claw.
        EntityDef d;
        d.id = "karambit";
        d.displayName = "Karambit Pickup";
        d.category = ContentCategory::Weapon;
        d.size = {0.45f, 0.30f, 0.45f}; // generous interact box, easy to target
        d.color = blade;
        d.weaponId = "karambit";
        d.respawnSeconds = 10.0f;
        d.visual = {
            {{-0.13f, 0.10f, 0.0f}, {0.12f, 0.05f, 0.035f}, handleDark}, // handle
            {{-0.21f, 0.10f, 0.0f}, {0.05f, 0.07f, 0.025f}, blade},      // finger ring
            {{-0.02f, 0.095f, 0.0f}, {0.11f, 0.035f, 0.02f}, blade},     // blade root
            {{0.07f, 0.075f, 0.0f}, {0.09f, 0.03f, 0.02f}, blade},       // blade mid
            {{0.14f, 0.05f, 0.0f}, {0.07f, 0.026f, 0.02f}, blade},       // blade curve
            {{0.185f, 0.028f, 0.0f}, {0.04f, 0.02f, 0.02f}, blade},      // tip
        };
        defs_.push_back(d);
    }
    {
        // Glock pickup: slide over a grip with a trigger guard.
        EntityDef d;
        d.id = "glock";
        d.displayName = "Glock Pickup";
        d.category = ContentCategory::Weapon;
        d.size = {0.45f, 0.30f, 0.45f};
        d.color = gunMetal;
        d.weaponId = "glock";
        d.respawnSeconds = 10.0f;
        d.visual = {
            {{0.0f, 0.175f, 0.0f}, {0.30f, 0.07f, 0.05f}, gunMetal},   // slide
            {{-0.16f, 0.175f, 0.0f}, {0.04f, 0.05f, 0.04f}, gunMetal}, // muzzle
            {{0.10f, 0.085f, 0.0f}, {0.075f, 0.13f, 0.045f}, gunGrip}, // grip
            {{0.02f, 0.115f, 0.0f}, {0.09f, 0.02f, 0.03f}, gunGrip},   // trigger guard
        };
        defs_.push_back(d);
    }
    {
        // Training dummy: base + torso + head, so headshots have somewhere
        // to land once hitboxes exist. One AABB for now.
        EntityDef d;
        d.id = "training_dummy";
        d.displayName = "Training Dummy";
        d.category = ContentCategory::Bot;
        d.size = {0.60f, 1.80f, 0.60f};
        d.color = {0.82f, 0.72f, 0.45f};
        d.solid = true;
        d.maxHealth = 100.0f;
        d.respawnSeconds = 5.0f; // respawns after being destroyed
        d.visual = {
            {{0.0f, 0.06f, 0.0f}, {0.60f, 0.12f, 0.60f}, {0.35f, 0.35f, 0.38f}}, // base
            {{0.0f, 0.42f, 0.0f}, {0.14f, 0.60f, 0.14f}, {0.45f, 0.40f, 0.30f}}, // post
            {{0.0f, 1.05f, 0.0f}, {0.52f, 0.66f, 0.30f}, {0.82f, 0.72f, 0.45f}}, // torso
            {{0.0f, 1.55f, 0.0f}, {0.28f, 0.30f, 0.26f}, {0.88f, 0.80f, 0.58f}}, // head
        };
        defs_.push_back(d);
    }
    {
        EntityDef d;
        d.id = "crate";
        d.displayName = "Crate";
        d.category = ContentCategory::Prop;
        d.size = {1.00f, 1.00f, 1.00f};
        d.color = {0.85f, 0.55f, 0.25f};
        d.solid = true; // props are solid and do not respawn
        defs_.push_back(d);
    }
}

const EntityDef* ContentRegistry::find(const std::string& id) const {
    for (const EntityDef& def : defs_) {
        if (def.id == id) return &def;
    }
    return nullptr;
}

const WeaponDef* ContentRegistry::findWeapon(const std::string& id) const {
    for (const WeaponDef& def : weapons_) {
        if (def.id == id) return &def;
    }
    return nullptr;
}
