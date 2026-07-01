#pragma once

#include <string>
#include <unordered_map>

// Flat "key = value" format. '#' starts a comment. All values are floats.
// Deliberately parses from a string, not a file: file access goes through
// the platform IFileSystem so the same code works on packaged platforms.
class ConfigFile {
public:
    void parseText(const std::string& text); // replaces current contents
    float get(const std::string& key, float fallback) const;
    bool empty() const { return values_.empty(); }

private:
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
