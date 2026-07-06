#include "client/settings.h"

#include <algorithm>
#include <cstdio>

void GameSettings::applyFrom(const ConfigFile& cfg) {
    mouseSensitivity = cfg.get("mouse_sensitivity", mouseSensitivity);
    fovDegrees = cfg.get("fov_degrees", fovDegrees);
    masterVolume = cfg.get("master_volume", masterVolume);
    musicVolume = cfg.get("music_volume", musicVolume);
    sfxVolume = cfg.get("sfx_volume", sfxVolume);
    fullscreen = cfg.get("fullscreen", fullscreen ? 1.0f : 0.0f) != 0.0f;
    vsync = cfg.get("vsync", vsync ? 1.0f : 0.0f) != 0.0f;
    showDebugHud = cfg.get("show_debug_hud", showDebugHud ? 1.0f : 0.0f) != 0.0f;
    thirdPerson = cfg.get("third_person", thirdPerson ? 1.0f : 0.0f) != 0.0f;
    std::string policy = cfg.getString("camera_policy", "allow_toggle");
    if (policy == "first_person_only") cameraPolicy = CameraPolicy::FirstPersonOnly;
    else if (policy == "third_person_only") cameraPolicy = CameraPolicy::ThirdPersonOnly;
    else cameraPolicy = CameraPolicy::AllowToggle;
    clampRanges();
}

void GameSettings::clampRanges() {
    mouseSensitivity = std::clamp(mouseSensitivity, 0.1f, 10.0f);
    fovDegrees = std::clamp(fovDegrees, 60.0f, 110.0f);
    masterVolume = std::clamp(masterVolume, 0.0f, 1.0f);
    musicVolume = std::clamp(musicVolume, 0.0f, 1.0f);
    sfxVolume = std::clamp(sfxVolume, 0.0f, 1.0f);
}

std::string GameSettings::serialize() const {
    char buf[1024];
    std::snprintf(buf, sizeof(buf),
                  "# TacMove user settings.\n"
                  "# Written by the in-game settings menu; safe to hand-edit.\n"
                  "# Movement physics tunables live in movement.cfg instead.\n"
                  "\n"
                  "mouse_sensitivity = %.2f\n"
                  "fov_degrees       = %.0f\n"
                  "master_volume     = %.2f\n"
                  "music_volume      = %.2f\n"
                  "sfx_volume        = %.2f\n"
                  "fullscreen        = %d\n"
                  "vsync             = %d\n"
                  "show_debug_hud    = %d\n"
                  "third_person      = %d\n"
                  "# camera_policy: allow_toggle | first_person_only | third_person_only\n"
                  "camera_policy     = %s\n",
                  mouseSensitivity, fovDegrees, masterVolume, musicVolume, sfxVolume,
                  fullscreen ? 1 : 0, vsync ? 1 : 0, showDebugHud ? 1 : 0,
                  thirdPerson ? 1 : 0,
                  cameraPolicy == CameraPolicy::FirstPersonOnly   ? "first_person_only"
                  : cameraPolicy == CameraPolicy::ThirdPersonOnly ? "third_person_only"
                                                                  : "allow_toggle");
    return buf;
}
