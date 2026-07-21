#include "core/app.h"
#include "core/log.h"
#include "raylib.h"
#include "raymath.h"

void App::Init(const AppConfig& cfg) {
    SetTraceLogLevel(LOG_WARNING);   // raylib's own chatter: warnings only
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(cfg.width, cfg.height, cfg.title);
    // Place the window near the top-left so its title bar and close button stay
    // on-screen even on smaller displays (opening larger than the monitor made
    // the game hard to move or quit). The default size is kept modest; we avoid
    // a runtime SetWindowSize, which fights the retina framebuffer sizing.
    if (cfg.fullscreen) ToggleBorderlessWindowed();
    else SetWindowPosition(40, 50);
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

void App::SetResolution(int width, int height) {
    if (this->IsFullscreen()) return;   // borderless always follows the monitor
    width = Clamp(width, 960, 7680);
    height = Clamp(height, 540, 4320);

    if (GetScreenWidth() == width && GetScreenHeight() == height) return;

#if defined(__APPLE__)
    // On macOS, raylib 6 can switch a non-HiDPI window to a Retina framebuffer
    // during SetWindowSize, leaving its projection/viewport in one quadrant.
    // main.cpp loads persisted dimensions before InitWindow, so menu changes
    // safely take effect on the next launch.
    LOG_INFO("Window resolution %dx%d saved; restart required on macOS", width, height);
    return;
#endif

    int mon = GetCurrentMonitor();
    int mw = GetMonitorWidth(mon), mh = GetMonitorHeight(mon);
    if (mw > 0 && mh > 0) {
        width = (width > mw - 40) ? mw - 40 : width;
        height = (height > mh - 80) ? mh - 80 : height;
    }
    SetWindowSize(width, height);
    if (mw > 0 && mh > 0)
        SetWindowPosition((mw - width) / 2, (mh - height) / 2);
    LOG_INFO("Windowed resolution %dx%d", width, height);
}

bool App::IsFullscreen() const { return IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE); }
