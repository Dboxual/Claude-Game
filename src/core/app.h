// App owns the window and raw frame mechanics: event polling, frame time,
// buffer swap, frame cap. The vcpkg raylib is built with
// SUPPORT_CUSTOM_FRAME_CONTROL — EndDrawing() does NOT poll input, swap
// buffers, or track time — so App detects that at runtime and drives all
// three manually. Nothing else in the codebase may poll or swap.
#pragma once

struct AppConfig {
    int width = 1200;
    int height = 700;
    bool fullscreen = false;
    const char* title = "ZION";
};

class App {
public:
    void Init(const AppConfig& cfg);
    void Shutdown();

    float BeginFrame();          // polls input; returns clamped frame dt (seconds)
    void EndFrame();             // swaps buffers, applies the fps cap
    bool ShouldClose() const;    // window X / Alt+F4 (ESC is a game key)

    void SetFpsCap(int fps) { fpsCap = fps; }        // 0 = uncapped
    void SetFullscreen(bool on);                     // borderless toggle
    void SetResolution(int width, int height);       // windowed client size
    bool IsFullscreen() const;

private:
    bool customFrame = false;
    double prevTime = 0.0;
    double frameStart = 0.0;
    int fpsCap = 240;
};
