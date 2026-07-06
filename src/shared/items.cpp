#include "shared/items.h"

void ItemRegistry::registerBuiltins() {
    defs_.clear();

    // Starter economy: three raw resources, two gathering tools, two weapons.
    // Colors are the chunky readable palette - each item family owns a hue
    // (wood browns, stone greys, ore amber) so drops and swatches read at a
    // glance without icons.
    {
        ItemDef d;
        d.id = "wood";
        d.displayName = "Wood";
        d.kind = ItemKind::Resource;
        d.color = {0.58f, 0.40f, 0.22f};
        defs_.push_back(d);
    }
    {
        ItemDef d;
        d.id = "stone";
        d.displayName = "Stone";
        d.kind = ItemKind::Resource;
        d.color = {0.58f, 0.60f, 0.63f};
        defs_.push_back(d);
    }
    {
        ItemDef d;
        d.id = "ore";
        d.displayName = "Ore";
        d.kind = ItemKind::Resource;
        d.color = {0.85f, 0.58f, 0.25f};
        defs_.push_back(d);
    }
    {
        ItemDef d;
        d.id = "crude_axe";
        d.displayName = "Crude Axe";
        d.kind = ItemKind::Tool;
        d.maxStack = 1;
        d.color = {0.70f, 0.72f, 0.76f};
        d.weaponId = "crude_axe";
        d.toolClass = "axe"; // double yield on axe nodes (trees)
        defs_.push_back(d);
    }
    {
        ItemDef d;
        d.id = "crude_pickaxe";
        d.displayName = "Crude Pickaxe";
        d.kind = ItemKind::Tool;
        d.maxStack = 1;
        d.color = {0.62f, 0.64f, 0.70f};
        d.weaponId = "crude_pickaxe";
        d.toolClass = "pickaxe"; // double yield on pickaxe nodes (rock + ore)
        defs_.push_back(d);
    }
    {
        ItemDef d;
        d.id = "basic_sword";
        d.displayName = "Basic Sword";
        d.kind = ItemKind::Weapon;
        d.maxStack = 1;
        d.color = {0.78f, 0.83f, 0.88f};
        d.weaponId = "basic_sword";
        defs_.push_back(d);
    }
    {
        ItemDef d;
        d.id = "basic_staff";
        d.displayName = "Basic Staff";
        d.kind = ItemKind::Weapon;
        d.maxStack = 1;
        d.color = {0.45f, 0.60f, 0.95f};
        d.weaponId = "basic_staff";
        defs_.push_back(d);
    }
}

void ItemRegistry::addOrReplace(const ItemDef& def) {
    for (ItemDef& d : defs_) {
        if (d.id == def.id) {
            d = def;
            return;
        }
    }
    defs_.push_back(def);
}

const ItemDef* ItemRegistry::find(const std::string& id) const {
    for (const ItemDef& def : defs_) {
        if (def.id == id) return &def;
    }
    return nullptr;
}
