#include "engine/config.h"

#include <cstdlib>
#include <sstream>

namespace {

std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    size_t begin = s.find_first_not_of(ws);
    if (begin == std::string::npos) return {};
    size_t end = s.find_last_not_of(ws);
    return s.substr(begin, end - begin + 1);
}

} // namespace

void ConfigFile::parseText(const std::string& text) {
    values_.clear();

    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        size_t hash = line.find('#');
        if (hash != std::string::npos) line.erase(hash);

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));
        if (key.empty() || value.empty()) continue;

        char* end = nullptr;
        float parsed = std::strtof(value.c_str(), &end);
        if (end == value.c_str()) continue; // not a number
        values_[key] = parsed;
    }
}

float ConfigFile::get(const std::string& key, float fallback) const {
    auto it = values_.find(key);
    return it != values_.end() ? it->second : fallback;
}

void MovementConfig::applyFrom(const ConfigFile& cfg) {
    tickRate = cfg.get("tick_rate", tickRate);
    mouseSensitivity = cfg.get("mouse_sensitivity", mouseSensitivity);
    fovDegrees = cfg.get("fov_degrees", fovDegrees);

    runSpeed = cfg.get("run_speed", runSpeed);
    walkSpeed = cfg.get("walk_speed", walkSpeed);
    crouchSpeed = cfg.get("crouch_speed", crouchSpeed);

    groundAccel = cfg.get("ground_accel", groundAccel);
    friction = cfg.get("friction", friction);
    stopSpeed = cfg.get("stop_speed", stopSpeed);

    airAccel = cfg.get("air_accel", airAccel);
    airSpeedCap = cfg.get("air_speed_cap", airSpeedCap);
    gravity = cfg.get("gravity", gravity);
    jumpSpeed = cfg.get("jump_speed", jumpSpeed);
    autoBhop = cfg.get("auto_bhop", autoBhop);
    jumpBufferMs = cfg.get("jump_buffer_ms", jumpBufferMs);

    playerRadius = cfg.get("player_radius", playerRadius);
    standHeight = cfg.get("stand_height", standHeight);
    crouchHeight = cfg.get("crouch_height", crouchHeight);
    eyeOffset = cfg.get("eye_offset", eyeOffset);
    crouchTransitionSpeed = cfg.get("crouch_transition_speed", crouchTransitionSpeed);
}
