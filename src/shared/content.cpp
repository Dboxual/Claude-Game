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
        w.damage = 34.0f;          // ~3 body shots to down a MiniCS enemy
        w.range = 60.0f;
        w.cooldownSeconds = 0.14f; // snappy semi-auto
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

    // --- RPG tools/weapons (held via the item inventory's main-hand slot).
    // Tools swing fast but hit soft; the crafted sword is a real weapon but
    // below the legacy sandbox sword; the staff is a slow magic bolt that
    // reuses the hitscan path.
    {
        WeaponDef w;
        w.id = "crude_axe";
        w.displayName = "Crude Axe";
        w.kind = WeaponKind::Melee;
        w.damage = 20.0f;
        w.range = 1.9f;
        w.cooldownSeconds = 0.5f;
        w.weight = 0.9f;
        weapons_.push_back(w);
    }
    {
        WeaponDef w;
        w.id = "crude_pickaxe";
        w.displayName = "Crude Pickaxe";
        w.kind = WeaponKind::Melee;
        w.damage = 15.0f;
        w.range = 1.9f;
        w.cooldownSeconds = 0.5f;
        w.weight = 0.95f;
        weapons_.push_back(w);
    }
    {
        WeaponDef w;
        w.id = "basic_sword";
        w.displayName = "Basic Sword";
        w.kind = WeaponKind::Melee;
        w.damage = 35.0f;
        w.range = 2.2f;
        w.cooldownSeconds = 0.6f;
        w.weight = 1.0f;
        weapons_.push_back(w);
    }
    {
        WeaponDef w;
        w.id = "basic_staff";
        w.displayName = "Basic Staff";
        w.kind = WeaponKind::Hitscan; // magic bolt: hitscan with a slow cadence
        w.damage = 18.0f;
        w.range = 30.0f;
        w.cooldownSeconds = 0.8f;
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
    // --- RPG resource nodes. Solid, chunky, colorful; the collision box is
    // the trunk/boulder only so canopies never block movement. Nodes respawn
    // by default: the world is a renewable playground, and a world save can
    // still override per placement.
    {
        EntityDef d;
        d.id = "tree";
        d.displayName = "Tree";
        d.category = ContentCategory::Node;
        d.size = {0.55f, 3.0f, 0.55f}; // trunk-only collision
        d.color = {0.44f, 0.30f, 0.16f};
        d.solid = true;
        d.harvestItemId = "wood";
        d.harvestCount = 1;
        d.harvestHits = 4;
        d.toolClass = "axe";
        d.respawnSeconds = 30.0f;
        const glm::vec3 bark{0.44f, 0.30f, 0.16f};
        const glm::vec3 barkDark{0.36f, 0.24f, 0.13f};
        const glm::vec3 leaf{0.28f, 0.58f, 0.26f};
        const glm::vec3 leafLight{0.36f, 0.68f, 0.30f};
        d.visual = {
            {{0.0f, 1.1f, 0.0f}, {0.5f, 2.2f, 0.5f}, bark},        // trunk
            {{0.12f, 0.35f, 0.10f}, {0.60f, 0.7f, 0.60f}, barkDark}, // root flare
            {{0.0f, 2.75f, 0.0f}, {1.9f, 1.1f, 1.9f}, leaf},       // canopy base
            {{0.15f, 3.55f, -0.10f}, {1.3f, 0.8f, 1.3f}, leafLight}, // canopy mid
            {{-0.05f, 4.15f, 0.05f}, {0.7f, 0.5f, 0.7f}, leaf},    // canopy top
        };
        defs_.push_back(d);
    }
    {
        EntityDef d;
        d.id = "rock";
        d.displayName = "Rock";
        d.category = ContentCategory::Node;
        d.size = {1.1f, 0.9f, 1.1f};
        d.color = {0.55f, 0.57f, 0.60f};
        d.solid = true;
        d.harvestItemId = "stone";
        d.harvestCount = 1;
        d.harvestHits = 3;
        d.toolClass = "pickaxe";
        d.respawnSeconds = 30.0f;
        const glm::vec3 grey{0.55f, 0.57f, 0.60f};
        const glm::vec3 greyDark{0.44f, 0.46f, 0.50f};
        d.visual = {
            {{0.0f, 0.38f, 0.0f}, {1.05f, 0.76f, 0.95f}, grey},      // main boulder
            {{0.42f, 0.20f, 0.30f}, {0.5f, 0.40f, 0.45f}, greyDark}, // side chunk
            {{-0.35f, 0.62f, -0.15f}, {0.45f, 0.40f, 0.40f}, greyDark}, // top chunk
        };
        defs_.push_back(d);
    }
    {
        EntityDef d;
        d.id = "ore_node";
        d.displayName = "Ore Vein";
        d.category = ContentCategory::Node;
        d.size = {1.0f, 0.9f, 1.0f};
        d.color = {0.38f, 0.36f, 0.40f};
        d.solid = true;
        d.harvestItemId = "ore";
        d.harvestCount = 1;
        d.harvestHits = 3;
        d.toolClass = "pickaxe";
        d.respawnSeconds = 45.0f; // ore is the scarcer resource
        const glm::vec3 dark{0.38f, 0.36f, 0.40f};
        const glm::vec3 amber{0.90f, 0.60f, 0.22f};
        d.visual = {
            {{0.0f, 0.40f, 0.0f}, {0.95f, 0.80f, 0.90f}, dark},        // host rock
            {{0.30f, 0.55f, 0.25f}, {0.28f, 0.26f, 0.24f}, amber},     // ore fleck
            {{-0.28f, 0.30f, -0.20f}, {0.24f, 0.22f, 0.22f}, amber},   // ore fleck
            {{0.05f, 0.72f, -0.22f}, {0.20f, 0.18f, 0.18f}, amber},    // ore fleck
        };
        defs_.push_back(d);
    }

    // --- RPG mob: the goblin is the first "real" enemy - short, green,
    // readable at a glance next to the bronze duelist and tan dummy. Very
    // simple AI (client-side chase + contact strike) on purpose.
    {
        EntityDef d;
        d.id = "goblin";
        d.displayName = "Goblin";
        d.category = ContentCategory::Bot;
        d.size = {0.60f, 1.25f, 0.60f};
        d.color = {0.38f, 0.62f, 0.30f};
        d.solid = true;
        d.maxHealth = 40.0f;
        d.hostile = true;
        d.contactDamage = 7.0f;
        d.lootItemId = "ore";
        d.lootCount = 1;
        d.respawnSeconds = 0.0f; // world placements opt into respawn
        const glm::vec3 skin{0.38f, 0.62f, 0.30f};
        const glm::vec3 skinDark{0.30f, 0.50f, 0.24f};
        const glm::vec3 rags{0.45f, 0.32f, 0.22f};
        d.visual = {
            {{-0.14f, 0.14f, 0.0f}, {0.16f, 0.28f, 0.20f}, skinDark}, // left leg
            {{0.14f, 0.14f, 0.0f}, {0.16f, 0.28f, 0.20f}, skinDark},  // right leg
            {{0.0f, 0.52f, 0.0f}, {0.46f, 0.50f, 0.32f}, rags},       // tunic torso
            {{-0.30f, 0.55f, 0.0f}, {0.13f, 0.38f, 0.15f}, skin},     // left arm
            {{0.30f, 0.55f, 0.0f}, {0.13f, 0.38f, 0.15f}, skin},      // right arm
            {{0.0f, 0.98f, 0.0f}, {0.34f, 0.32f, 0.30f}, skin},       // head
            {{-0.26f, 1.04f, 0.0f}, {0.16f, 0.08f, 0.05f}, skinDark}, // left ear
            {{0.26f, 1.04f, 0.0f}, {0.16f, 0.08f, 0.05f}, skinDark},  // right ear
        };
        defs_.push_back(d);
    }

    // --- MiniCS enemy: the ranged bot the round loop fights. Distinct from
    // the melee duel_bot - this one advances, strafes, aims, and shoots. Its
    // whole brain (movement/aim/fire/flinch/death) lives in client code
    // (Game::updateMinicsEnemies); the def is just the silhouette + hitbox.
    // Crimson on purpose: it must read as ENEMY at a glance against the tan
    // arena and slate walls. maxHealth is tuned to ~3 Glock body shots.
    {
        EntityDef d;
        d.id = "minics_bot";
        d.displayName = "Enemy";
        d.category = ContentCategory::Bot;
        d.size = {0.64f, 1.82f, 0.64f};
        d.color = {0.78f, 0.22f, 0.20f};
        d.solid = true;
        d.maxHealth = 90.0f;
        d.spawnable = false;      // placed by the round loop, not the build menu
        d.respawnSeconds = 0.0f;  // round-based: dead is dead until the next round
        // Fallback single-figure visual. The live enemy is drawn procedurally
        // (walk/aim/shoot/flinch) so this only shows if it ever loses its brain.
        const glm::vec3 vest{0.78f, 0.22f, 0.20f};
        const glm::vec3 vestDark{0.60f, 0.16f, 0.15f};
        const glm::vec3 gearDark{0.20f, 0.21f, 0.24f};
        const glm::vec3 skin{0.86f, 0.68f, 0.54f};
        d.visual = {
            {{-0.15f, 0.34f, 0.0f}, {0.20f, 0.70f, 0.24f}, gearDark}, // left leg
            {{0.15f, 0.34f, 0.0f}, {0.20f, 0.70f, 0.24f}, gearDark},  // right leg
            {{0.0f, 1.02f, 0.0f}, {0.52f, 0.66f, 0.34f}, vest},       // torso vest
            {{0.0f, 1.28f, 0.0f}, {0.60f, 0.16f, 0.40f}, vestDark},   // shoulders
            {{0.0f, 1.58f, 0.0f}, {0.28f, 0.30f, 0.28f}, skin},       // head
            {{0.0f, 1.72f, 0.0f}, {0.30f, 0.10f, 0.30f}, gearDark},   // helmet cap
        };
        defs_.push_back(d);
    }

    // --- ground item drops: what mobs leave behind and what loose resources
    // look like lying in the world. E picks them up into the item inventory.
    // Small hovering cubes tinted like their item; ids are "drop_" + item id
    // (spawnLootDrop relies on that convention).
    {
        EntityDef d;
        d.id = "drop_wood";
        d.displayName = "Wood (drop)";
        d.category = ContentCategory::Pickup;
        d.size = {0.4f, 0.35f, 0.4f};
        d.color = {0.58f, 0.40f, 0.22f};
        d.itemId = "wood";
        d.itemCount = 1;
        d.visual = {
            {{0.0f, 0.12f, 0.0f}, {0.30f, 0.14f, 0.14f}, {0.58f, 0.40f, 0.22f}}, // log
            {{0.05f, 0.24f, 0.05f}, {0.24f, 0.12f, 0.12f}, {0.48f, 0.33f, 0.18f}},
        };
        defs_.push_back(d);
    }
    {
        EntityDef d;
        d.id = "drop_stone";
        d.displayName = "Stone (drop)";
        d.category = ContentCategory::Pickup;
        d.size = {0.4f, 0.35f, 0.4f};
        d.color = {0.58f, 0.60f, 0.63f};
        d.itemId = "stone";
        d.itemCount = 1;
        d.visual = {
            {{0.0f, 0.10f, 0.0f}, {0.24f, 0.18f, 0.22f}, {0.58f, 0.60f, 0.63f}},
            {{0.10f, 0.20f, -0.04f}, {0.14f, 0.12f, 0.13f}, {0.48f, 0.50f, 0.54f}},
        };
        defs_.push_back(d);
    }
    {
        EntityDef d;
        d.id = "drop_ore";
        d.displayName = "Ore (drop)";
        d.category = ContentCategory::Pickup;
        d.size = {0.4f, 0.35f, 0.4f};
        d.color = {0.85f, 0.58f, 0.25f};
        d.itemId = "ore";
        d.itemCount = 1;
        d.visual = {
            {{0.0f, 0.10f, 0.0f}, {0.22f, 0.17f, 0.20f}, {0.40f, 0.38f, 0.42f}}, // rock
            {{0.05f, 0.18f, 0.04f}, {0.13f, 0.12f, 0.12f}, {0.85f, 0.58f, 0.25f}}, // fleck
        };
        defs_.push_back(d);
    }

    {
        // Neon pillar: a glowing landmark prop (also light cover). Emissive
        // cyan core + magenta cap + amber base ring on a dark column - the
        // arena's neon vocabulary in one placeable object. Solid, static.
        EntityDef d;
        d.id = "neon_pillar";
        d.displayName = "Neon Pillar";
        d.category = ContentCategory::Prop;
        d.size = {0.5f, 3.0f, 0.5f};
        d.color = {0.15f, 0.9f, 1.0f};
        d.solid = true;
        const glm::vec3 column{0.10f, 0.11f, 0.15f};
        const glm::vec3 cyan{0.15f, 0.95f, 1.0f};
        const glm::vec3 magenta{1.0f, 0.20f, 0.75f};
        const glm::vec3 amber{1.0f, 0.62f, 0.12f};
        d.visual = {
            {{0.0f, 1.5f, 0.0f}, {0.42f, 3.0f, 0.42f}, column, 0.0f},   // dark column
            {{0.0f, 1.55f, 0.0f}, {0.5f, 2.5f, 0.14f}, cyan, 1.0f},     // glow panel X
            {{0.0f, 1.55f, 0.0f}, {0.14f, 2.5f, 0.5f}, cyan, 1.0f},     // glow panel Z
            {{0.0f, 3.06f, 0.0f}, {0.56f, 0.2f, 0.56f}, magenta, 1.0f}, // magenta cap
            {{0.0f, 0.09f, 0.0f}, {0.62f, 0.18f, 0.62f}, amber, 1.0f},  // amber base ring
        };
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
