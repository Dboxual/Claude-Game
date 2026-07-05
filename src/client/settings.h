#pragma once

#include "shared/config.h"

#include <string>

// User-facing settings, edited by the in-game settings menu and persisted to
// config/settings.cfg (same key=value format as the movement config, safe to
// hand-edit). Movement physics tunables stay in config/movement.cfg; this is
// only what a player would change.
struct GameSettings {
    float mouseSensitivity = 1.8f;
    float fovDegrees = 74.0f; // vertical
    float masterVolume = 1.0f; // 0..1; plumbed into IAudio, no sounds yet
    float musicVolume = 0.7f;
    float sfxVolume = 1.0f;
    bool fullscreen = false;
    bool vsync = true;
    bool showDebugHud = false; // dev overlay; F1 toggles it on when wanted

    void applyFrom(const ConfigFile& cfg);
    std::string serialize() const; // config-file text, comments included
    void clampRanges();
};
