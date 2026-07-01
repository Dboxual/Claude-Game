#pragma once

#include <string>
#include <unordered_map>

// Flat "key = value" file. '#' starts a comment. All values are floats.
class ConfigFile {
public:
    bool loadFromFile(const std::string& path);
    bool reload();
    float get(const std::string& key, float fallback) const;
    const std::string& path() const { return path_; }

private:
    std::string path_;
    std::unordered_map<std::string, float> values_;
};

// Every tunable movement/camera constant, filled from config/movement.cfg.
// Defaults here match the shipped config file so the game still runs if the
// file is missing.
struct MovementConfig {
    float tickRate = 128.0f;
    float mouseSensitivity = 1.8f;
    float fovDegrees = 74.0f;

    float runSpeed = 6.0f;
    float walkSpeed = 2.6f;
    float crouchSpeed = 1.8f;

    float groundAccel = 12.0f;
    float friction = 7.0f;
    float stopSpeed = 1.2f;

    float airAccel = 14.0f;
    float airSpeedCap = 0.95f;
    float gravity = 20.0f;
    float jumpSpeed = 5.1f;
    float autoBhop = 0.0f;
    float jumpBufferMs = 80.0f;

    float playerRadius = 0.4f;
    float standHeight = 1.8f;
    float crouchHeight = 1.3f;
    float eyeOffset = 0.15f;
    float crouchTransitionSpeed = 10.0f;

    void applyFrom(const ConfigFile& cfg);
};

// Searches likely locations (working directory, then the exe directory) for
// config/movement.cfg. Returns an empty string if not found.
std::string findConfigPath(const char* exeDir);
