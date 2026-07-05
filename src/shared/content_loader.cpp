#include "shared/content_loader.h"

#include "shared/config.h"

#include <cstdio>

namespace {

glm::vec3 getVec3(const ConfigFile& cfg, const std::string& key, glm::vec3 fallback) {
    std::string v = cfg.getString(key, "");
    glm::vec3 out;
    if (std::sscanf(v.c_str(), "%f %f %f", &out.x, &out.y, &out.z) == 3) return out;
    return fallback;
}

bool getBool(const ConfigFile& cfg, const std::string& key, bool fallback) {
    return cfg.get(key, fallback ? 1.0f : 0.0f) != 0.0f;
}

} // namespace

bool parseWeaponDef(const std::string& text, WeaponDef& out) {
    ConfigFile cfg;
    cfg.parseText(text);
    std::string id = cfg.getString("id", "");
    if (id.empty()) return false;

    WeaponDef w;
    w.id = id;
    w.displayName = cfg.getString("name", id);
    w.kind = cfg.getString("kind", "melee") == "hitscan" ? WeaponKind::Hitscan
                                                         : WeaponKind::Melee;
    w.damage = cfg.get("damage", w.damage);
    w.range = cfg.get("range", w.range);
    w.cooldownSeconds = cfg.get("cooldown", w.cooldownSeconds);
    w.weight = cfg.get("weight", w.weight);
    out = w;
    return true;
}

bool parseEntityDef(const std::string& text, EntityDef& out) {
    ConfigFile cfg;
    cfg.parseText(text);
    std::string id = cfg.getString("id", "");
    if (id.empty()) return false;

    EntityDef d;
    d.id = id;
    d.displayName = cfg.getString("name", id);
    std::string cat = cfg.getString("category", "prop");
    if (cat == "weapon") d.category = ContentCategory::Weapon;
    else if (cat == "bot") d.category = ContentCategory::Bot;
    else if (cat == "pickup") d.category = ContentCategory::Pickup;
    else d.category = ContentCategory::Prop;

    d.size = getVec3(cfg, "size", d.size);
    d.color = getVec3(cfg, "color", d.color);
    d.solid = getBool(cfg, "solid", d.solid);
    d.spawnable = getBool(cfg, "spawnable", d.spawnable);
    d.carryable = getBool(cfg, "carryable", d.carryable);
    d.gear = getBool(cfg, "gear", d.gear);
    d.weaponId = cfg.getString("weapon", "");
    d.maxHealth = cfg.get("max_health", d.maxHealth);
    d.respawnSeconds = cfg.get("respawn_seconds", d.respawnSeconds);

    // Visual boxes: part1..part16 = "ox oy oz sx sy sz r g b". ConfigFile is
    // a flat map, so parts are numbered keys rather than repeated ones.
    for (int i = 1; i <= 16; ++i) {
        char key[16];
        std::snprintf(key, sizeof(key), "part%d", i);
        std::string v = cfg.getString(key, "");
        if (v.empty()) break;
        VisualPart p;
        if (std::sscanf(v.c_str(), "%f %f %f %f %f %f %f %f %f",
                        &p.offset.x, &p.offset.y, &p.offset.z,
                        &p.size.x, &p.size.y, &p.size.z,
                        &p.color.r, &p.color.g, &p.color.b) == 9) {
            d.visual.push_back(p);
        }
    }
    out = d;
    return true;
}
