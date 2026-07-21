// In-world HUD: crosshair (FP), interact prompt, anima counter with reward
// pop, blessing banner, and screen fade. Owns only its animation state;
// all game data arrives through Draw parameters or event calls.
#pragma once
#include "raylib.h"

class Ui;

class Hud {
public:
    void Update(float dt);

    // Events (fire-and-forget; the HUD animates the response).
    void OnAnimaGained();
    void OnBanner(const char* text);      // e.g. "The shrine acknowledges you"
    void StartFadeIn(float seconds);

    struct DrawParams {
        bool firstPerson = true;
        bool menuOpen = false;
        const char* promptVerb = nullptr;   // null = no interact target
        const char* promptKey = "E";
        const char* zoneName = "Elder Vale";
        int zoneTier = 1;
        float cameraYaw = 0.0f;
        const char* clockText = "9:00 AM";
        bool showControlHints = false;
        int anima = 0;
        int blessings = 0;
        bool devMode = false;
        bool devOverlay = false;
        bool devFly = false;
        bool devTurbo = false;
    };
    void Draw(Ui& ui, const DrawParams& p);

private:
    float animaPop = 0.0f;       // 1 -> 0 after a gain
    float bannerTimer = 0.0f;
    char bannerText[96] = {};
    float fadeTimer = 0.0f, fadeLen = 0.0f;
    float promptPhase = 0.0f;
    float dt = 1.0f / 60.0f;
};
