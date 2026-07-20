#include "save/save.h"
#include "core/log.h"
#include "core/paths.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using nlohmann::json;

namespace SaveSystem {

bool Exists(int slot) {
    std::error_code ec;
    return std::filesystem::exists(SaveSlotPath(slot), ec);
}

SaveData Load(int slot) {
    SaveData d;
    std::ifstream f(SaveSlotPath(slot));
    if (!f.is_open()) return d;
    try {
        json j = json::parse(f);
        d.playerPos.x = j.value("px", 0.0f);
        d.playerPos.y = j.value("py", 0.0f);
        d.playerPos.z = j.value("pz", 0.0f);
        d.yaw = j.value("yaw", 0.0f);
        d.pitch = j.value("pitch", 0.0f);
        d.cameraMode = j.value("camera", 0);
        d.anima = j.value("anima", 0);
        d.blessings = j.value("blessings", 0);
        d.playtime = j.value("playtime", 0.0f);
        d.valid = true;
        LOG_INFO("Loaded save slot %d (%.0f min played)", slot, d.playtime / 60.0f);
    } catch (const std::exception& e) {
        LOG_WARN("Save slot %d unreadable: %s", slot, e.what());
    }
    return d;
}

bool Write(int slot, const SaveData& d) {
    json j = {
        { "px", d.playerPos.x }, { "py", d.playerPos.y }, { "pz", d.playerPos.z },
        { "yaw", d.yaw }, { "pitch", d.pitch }, { "camera", d.cameraMode },
        { "anima", d.anima }, { "blessings", d.blessings }, { "playtime", d.playtime },
    };
    std::ofstream f(SaveSlotPath(slot));
    if (!f.is_open()) { LOG_ERROR("Cannot write save slot %d", slot); return false; }
    f << j.dump(2);
    return true;
}

}   // namespace SaveSystem
