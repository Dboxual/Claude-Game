// User-tunable settings, persisted as JSON in the user data dir.
// Applying settings to live systems (window mode, volumes) is game/'s job;
// this module only owns the values and their persistence.
#pragma once

class InputSystem;

struct GraphicsSettings {
    bool fullscreen = false;
    int fpsCap = 240;                // 0 = uncapped (vsync still gates)
    float renderScale = 1.0f;        // 0.5 .. 1.25, scene RT resolution factor
    float fovY = 70.0f;              // degrees, first-person baseline
    float viewDistance = 300.0f;     // prop draw distance (m)
    bool bloom = true;
    float particleDensity = 1.0f;    // 0.25 .. 1.0 budget multiplier
};

struct AudioSettings {
    float master = 0.8f;
    float sfx = 1.0f;
    float music = 0.8f;
    float ambience = 0.9f;
};

struct ControlSettings {
    float mouseSensitivity = 0.11f;  // degrees per pixel
    bool invertY = false;
};

struct Settings {
    GraphicsSettings gfx;
    AudioSettings audio;
    ControlSettings controls;

    // Bindings live in InputSystem; settings persist them alongside.
    void Load(InputSystem& input);         // missing file/keys -> defaults kept
    void Save(const InputSystem& input) const;
};
