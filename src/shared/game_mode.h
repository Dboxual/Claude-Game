#pragma once

#include <string>
#include <vector>

// Game Modes: "the games inside the game". A GameMode is the top-level thing a
// player chooses to play - a ruleset + a default map + a starting loadout.
// Maps live INSIDE modes (a mode names the map template it loads); skins are
// cosmetic and orthogonal. This is the spine of the game-hub identity: Play ->
// Game Modes -> MiniCS / Deathmatch / Sandbox / ...
//
// Server-safe: std only, no glm, no IO, no content lookups. A mode references
// its map by world-template id (see shared/world_template.h) and its loadout
// by weapon id (see shared/content.h); the caller resolves those. The special
// map id kTestGladeMap means "the furnished sandbox glade" (World::buildTestMap)
// rather than a template.

// How finished a mode is, so the UI can be honest instead of faking depth.
//   Playable - a complete local loop you can actually play right now.
//   Preview  - drops you into a working world, but the full ruleset (scoring,
//              waves, timers, online) is not built yet; carries an EARLY badge.
enum class ModeStatus { Playable, Preview };

struct GameMode {
    std::string id;            // stable id ("minics", "sandbox", ...)
    std::string displayName;   // "MiniCS"
    std::string tagline;       // one line shown on the mode card
    std::string mapTemplateId; // world-template id, or kTestGladeMap
    std::string loadoutWeapon; // weapon id started equipped ("" = fists)
    ModeStatus status = ModeStatus::Preview;
    std::string previewNote;   // honest "what's not done yet", shown on entry
};

namespace gamemodes {

// Sentinel map id: load the furnished sandbox glade, not a world template.
inline constexpr const char* kTestGladeMap = "test_glade";

// The built-in modes, in menu order. MiniCS is first (the flagship playable).
const std::vector<GameMode>& all();

// Look up by id; nullptr if unknown.
const GameMode* find(const std::string& id);

} // namespace gamemodes
