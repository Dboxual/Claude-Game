#pragma once

#include "shared/content.h"

#include <string>

// Parses one weapon or entity definition from a content file's text
// (server/content/weapons/*.cfg and server/content/entities/*.cfg - the
// same flat "key = value" format as every other TacMove config, one
// definition per file). String -> struct only: the client reads the files
// through IFileSystem, the dedicated server can use plain streams.
//
// Missing keys keep the field's default, so files stay short. Returns
// false (and leaves out untouched) if the text has no usable id.
//
// Weapon keys:  id, name, kind (melee|hitscan), damage, range, cooldown
// Entity keys:  id, name, category (weapon|bot|prop|pickup),
//               size (x y z), color (r g b), solid, spawnable, carryable,
//               weapon (id granted on pickup), max_health, respawn_seconds,
//               part1..part16 (ox oy oz sx sy sz r g b - visual boxes)
bool parseWeaponDef(const std::string& text, WeaponDef& out);
bool parseEntityDef(const std::string& text, EntityDef& out);
