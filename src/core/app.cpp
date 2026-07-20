#include "core/app.h"
#include "core/log.h"
#include "raylib.h"
#include "raymath.h"

void App::Init(const AppConfig& cfg) {
    SetTraceLogLevel(LOG_WARNING);   // raylib's own chatter: warnings only
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(cfg.width, cfg.height, cfg.title);
    if (cfg.fullscreen) ToggleBorderlessWindowed();
    SetExitKey(KEY_NULL);            // ESC opens the pause menu instead

    // Detect SUPPORT_CUSTOM_FRAME_CONTROL builds: after a normal frame,
    // standard raylib reports a nonzero frame time; custom builds report 0
    // because EndDrawing() skipped the timing/poll/swap block.
    BeginDrawing(); ClearBackground(BLACK); EndDrawing();
    customFrame = (GetFrameTime() == 0.0f);
    if (customFrame) { SwapScreenBuffer(); PollInputEvents(); }

    prevTime = GetTime();
    LOG_INFO("Window %dx%d, custom frame control: %s",
             GetScreenWidth(), GetScreenHeight(), customFrame ? "yes" : "no");
}

void App::Shutdown() {
    CloseWindow();
}

float App::BeginFrame() {
    if (customFrame) PollInputEvents();
    frameStart = GetTime();
    // Clamp: hitches (window drags, debugger stops) must not explode physics.
    float dt = Clamp((float)(frameStart - prevTime), 0.0001f, 0.05f);
    prevTime = frameStart;
    return dt;
}

void App::EndFrame() {
    if (!customFrame) return;        // standard raylib swapped in EndDrawing
    SwapScreenBuffer();
    if (fpsCap > 0) {
        double used = GetTime() - frameStart;
        double target = 1.0 / (double)fpsCap;
        if (used < target) WaitTime(target - used);   // vsync usually gates first
    }
}

bool App::ShouldClose() const { return WindowShouldClose(); }

void App::SetFullscreen(bool on) {
    if (on != IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) ToggleBorderlessWindowed();
}

bool App::IsFullscreen() const { return IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE); }
