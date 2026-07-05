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
        w.weight = 0.6f; // snappy but weak
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
        w.weight = 0.75f;
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
    {
        WeaponDef w;
        w.id = "sword";
        w.displayName = "Sword";
        w.kind = WeaponKind::Melee;
        w.damage = 45.0f;
        w.range = 2.4f;          // longest melee reach...
        w.cooldownSeconds = 0.7f; // ...paid for with the heaviest swing
        w.weight = 1.0f;
        weapons_.push_back(w);
    }

    // --- entity defs. 'fists' has no pickup: it is the base loadout.

    const glm::vec3 gunMetal{0.16f, 0.17f, 0.20f};
    const glm::vec3 gunGrip{0.24f, 0.22f, 0.20f};
    const glm::vec3 blade{0.78f, 0.83f, 0.88f};
    const glm::vec3 handleDark{0.12f, 0.12f, 0.14f};
    const glm::vec3 guardGold{0.72f, 0.58f, 0.22f};
    const glm::vec3 gripBrown{0.32f, 0.21f, 0.12f};

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
        d.respawnSeconds = 0.0f; // taken = gone, unless a def/world overrides
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
        d.respawnSeconds = 0.0f; // taken = gone, unless a def/world overrides
        d.visual = {
            {{0.0f, 0.175f, 0.0f}, {0.30f, 0.07f, 0.05f}, gunMetal},   // slide
            {{-0.16f, 0.175f, 0.0f}, {0.04f, 0.05f, 0.04f}, gunMetal}, // muzzle
            {{0.10f, 0.085f, 0.0f}, {0.075f, 0.13f, 0.045f}, gunGrip}, // grip
            {{0.02f, 0.115f, 0.0f}, {0.09f, 0.02f, 0.03f}, gunGrip},   // trigger guard
        };
        defs_.push_back(d);
    }
    {
        // Sword pickup: straight blade, gold crossguard and pommel, leather
        // grip - a simple fantasy arming sword lying flat.
        EntityDef d;
        d.id = "sword";
        d.displayName = "Sword Pickup";
        d.category = ContentCategory::Weapon;
        d.size = {0.60f, 0.30f, 0.45f};
        d.color = blade;
        d.weaponId = "sword";
        d.respawnSeconds = 0.0f; // taken = gone, unless a def/world overrides
        d.visual = {
            {{0.09f, 0.105f, 0.0f}, {0.40f, 0.045f, 0.018f}, blade},     // blade
            {{0.315f, 0.105f, 0.0f}, {0.07f, 0.03f, 0.015f}, blade},     // tip taper
            {{-0.125f, 0.105f, 0.0f}, {0.035f, 0.13f, 0.035f}, guardGold}, // crossguard
            {{-0.20f, 0.105f, 0.0f}, {0.12f, 0.04f, 0.032f}, gripBrown},  // grip
            {{-0.275f, 0.105f, 0.0f}, {0.04f, 0.055f, 0.04f}, guardGold}, // pommel
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
        d.respawnSeconds = 0.0f; // destroyed = gone, unless a def/world overrides
        d.visual = {
            {{0.0f, 0.06f, 0.0f}, {0.60f, 0.12f, 0.60f}, {0.35f, 0.35f, 0.38f}}, // base
            {{0.0f, 0.42f, 0.0f}, {0.14f, 0.60f, 0.14f}, {0.45f, 0.40f, 0.30f}}, // post
            {{0.0f, 1.05f, 0.0f}, {0.52f, 0.66f, 0.30f}, {0.82f, 0.72f, 0.45f}}, // torso
            {{0.0f, 1.55f, 0.0f}, {0.28f, 0.30f, 0.26f}, {0.88f, 0.80f, 0.58f}}, // head
        };
        defs_.push_back(d);
    }
    {
        // Duelist bot: the melee sparring partner. Same silhouette family as
        // the training dummy but armored colors so the two read differently.
        // Its slow telegraphed attacks/guard live in client code for now
        // (no full AI); the combat exchange itself is shared/melee.
        EntityDef d;
        d.id = "duel_bot";
        d.displayName = "Duelist Bot";
        d.category = ContentCategory::Bot;
        d.size = {0.70f, 1.90f, 0.70f};
        d.color = {0.62f, 0.42f, 0.20f};
        d.solid = true;
        d.maxHealth = 150.0f;
        d.respawnSeconds = 0.0f; // destroyed = gone, unless a def/world overrides
        // Bronze armor on purpose: the arena walls are slate blue, and the
        // duelist must never camouflage - reading it IS the gameplay.
        d.visual = {
            {{0.0f, 0.06f, 0.0f}, {0.66f, 0.12f, 0.66f}, {0.30f, 0.30f, 0.34f}},  // base
            {{0.0f, 0.45f, 0.0f}, {0.16f, 0.66f, 0.16f}, {0.36f, 0.38f, 0.44f}},  // legs post
            {{0.0f, 1.10f, 0.0f}, {0.54f, 0.64f, 0.32f}, {0.62f, 0.42f, 0.20f}},  // cuirass
            {{0.0f, 1.47f, 0.0f}, {0.62f, 0.10f, 0.36f}, {0.42f, 0.30f, 0.18f}},  // shoulders
            {{0.0f, 1.66f, 0.0f}, {0.28f, 0.28f, 0.26f}, {0.85f, 0.82f, 0.74f}},  // helm
        };
        defs_.push_back(d);
    }
    {
        // Round shield pickup: equipping it makes blocks cheaper but slows
        // you while raised, and kicks can smash it open. Gear, not a weapon:
        // it rides along with whatever melee weapon is held.
        EntityDef d;
        d.id = "shield";
        d.displayName = "Shield";
        d.category = ContentCategory::Pickup;
        d.size = {0.55f, 0.35f, 0.55f};
        d.color = {0.46f, 0.30f, 0.16f};
        d.gear = true;
        d.respawnSeconds = 0.0f; // taken = gone, unless a def/world overrides
        d.visual = {
            {{0.0f, 0.10f, 0.0f}, {0.50f, 0.06f, 0.50f}, {0.46f, 0.30f, 0.16f}},   // face
            {{0.0f, 0.135f, 0.0f}, {0.16f, 0.055f, 0.16f}, {0.62f, 0.64f, 0.70f}}, // boss
            {{0.0f, 0.08f, 0.0f}, {0.55f, 0.025f, 0.55f}, {0.30f, 0.32f, 0.36f}},  // rim
        };
        defs_.push_back(d);
    }
    {
        // Light wooden crate: solid, carryable, never respawns (props are
        // gone for good once destroyed/consumed - respawnSeconds stays 0).
        EntityDef d;
        d.id = "crate";
        d.displayName = "Crate";
        d.category = ContentCategory::Prop;
        d.size = {1.00f, 1.00f, 1.00f};
        d.color = {0.85f, 0.55f, 0.25f};
        d.solid = true;
        d.carryable = true;
        defs_.push_back(d);
    }
    {
        // Heavy metal barrel: solid, NOT carryable, never respawns. Exists
        // partly to prove the carryable seam has both kinds of prop.
        EntityDef d;
        d.id = "barrel";
        d.displayName = "Barrel";
        d.category = ContentCategory::Prop;
        d.size = {0.70f, 1.05f, 0.70f};
        d.color = {0.36f, 0.42f, 0.48f};
        d.solid = true;
        d.visual = {
            {{0.0f, 0.5f, 0.0f}, {0.66f, 1.00f, 0.66f}, {0.36f, 0.42f, 0.48f}},  // body
            {{0.0f, 0.25f, 0.0f}, {0.70f, 0.06f, 0.70f}, {0.28f, 0.32f, 0.38f}}, // ring
            {{0.0f, 0.80f, 0.0f}, {0.70f, 0.06f, 0.70f}, {0.28f, 0.32f, 0.38f}}, // ring
        };
        defs_.push_back(d);
    }
}

void ContentRegistry::addOrReplace(const EntityDef& def) {
    for (EntityDef& d : defs_) {
        if (d.id == def.id) {
            d = def;
            return;
        }
    }
    defs_.push_back(def);
}

void ContentRegistry::addOrReplace(const WeaponDef& def) {
    for (WeaponDef& d : weapons_) {
        if (d.id == def.id) {
            d = def;
            return;
        }
    }
    weapons_.push_back(def);
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
