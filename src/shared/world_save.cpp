#include "shared/world_save.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace {

std::string trim(const std::string& s) {
    size_t begin = s.find_first_not_of(" \t\r");
    if (begin == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r");
    return s.substr(begin, end - begin + 1);
}

} // namespace

std::string serializeWorldSave(const WorldSaveData& data) {
    std::string out;
    out += "# TacMove world save. One 'entity' line per placed object:\n";
    out += "#   entity = <def_id> <x> <y> <z> <respawn_seconds>\n";
    out += "# respawn_seconds: pickups/bots come back this long after being\n";
    out += "# taken/destroyed; 0 = never respawns.\n";
    out += "# world_type: the world template this map's geometry comes from.\n";
    out += "version = 3\n";
    out += "name = " + data.displayName + "\n";
    if (!data.worldType.empty()) out += "world_type = " + data.worldType + "\n";
    char buf[160];
    for (const WorldSaveData::Spawn& s : data.entities) {
        std::snprintf(buf, sizeof(buf), "entity = %s %.2f %.2f %.2f %.1f\n",
                      s.defId.c_str(), s.pos.x, s.pos.y, s.pos.z,
                      s.respawnSeconds < 0.0f ? 0.0f : s.respawnSeconds);
        out += buf;
    }
    return out;
}

bool parseWorldSave(const std::string& text, WorldSaveData& out) {
    out = WorldSaveData{};
    bool sawAnything = false;

    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        size_t hash = line.find('#');
        if (hash != std::string::npos) line.resize(hash);
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));
        if (key.empty()) continue;

        if (key == "version") {
            out.version = std::atoi(value.c_str());
            sawAnything = true;
        } else if (key == "name") {
            out.displayName = value;
            sawAnything = true;
        } else if (key == "world_type") {
            out.worldType = value;
            sawAnything = true;
        } else if (key == "entity") {
            char defId[64] = {0};
            float x = 0.0f, y = 0.0f, z = 0.0f, respawn = -1.0f;
            int fields = std::sscanf(value.c_str(), "%63s %f %f %f %f",
                                     defId, &x, &y, &z, &respawn);
            if (fields >= 4) { // 4 fields = v1 line, respawn falls back to the def
                out.entities.push_back({defId, {x, y, z}, fields >= 5 ? respawn : -1.0f});
                sawAnything = true;
            }
        }
        // Unknown keys: ignored, so newer files still open in older builds.
    }
    return sawAnything;
}
