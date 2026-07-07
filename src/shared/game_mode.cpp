#include "shared/game_mode.h"

namespace gamemodes {

const std::vector<GameMode>& all() {
    // Each mode names a map (world-template id, or the glade sentinel) and a
    // loadout the caller applies after the world is built. Preview modes still
    // drop you into a real, working world - they are honest about the ruleset
    // depth that is not there yet, not dead "coming soon" buttons.
    static const std::vector<GameMode> kModes = {
        {"minics", "MiniCS",
         "Compact competitive FPS. Pistol, cover, bot opponents.",
         "minics_arena", "glock", ModeStatus::Playable, ""},

        {"deathmatch", "Deathmatch",
         "Free-for-all arena. Frag the bots, stay moving.",
         "minics_arena", "glock", ModeStatus::Preview,
         "DEATHMATCH IS EARLY - NO SCORING OR RESPAWNING ENEMIES YET"},

        {"sandbox", "Sandbox",
         "The full open world: gather, craft, build, spawn anything.",
         kTestGladeMap, "", ModeStatus::Playable, ""},

        {"zombies", "Zombies",
         "Survive the horde in a walled pit. Bring a gun.",
         "zombie_pit", "glock", ModeStatus::Preview,
         "ZOMBIES IS EARLY - NO WAVE SYSTEM OR ROUND SCORING YET"},

        {"parkour", "Parkour",
         "Jumps, tunnels and strafe pillars. Run the course.",
         "movement_course", "", ModeStatus::Preview,
         "PARKOUR IS EARLY - NO TIMER OR CHECKPOINTS YET"},

        {"social", "Social Hub",
         "Open plaza to hang out and mess around. Local for now.",
         "social_hub", "", ModeStatus::Preview,
         "SOCIAL HUB IS LOCAL ONLY UNTIL ONLINE PLAY LANDS"},
    };
    return kModes;
}

const GameMode* find(const std::string& id) {
    for (const GameMode& m : all()) {
        if (m.id == id) return &m;
    }
    return nullptr;
}

} // namespace gamemodes
