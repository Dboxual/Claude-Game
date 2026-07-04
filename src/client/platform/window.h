#pragma once

#include "client/platform/graphics_api.h"

struct WindowDesc {
    const char* title = "TacMove";
    int width = 1280;
    int height = 720;
    GraphicsApi api = GraphicsApi::OpenGL;
    bool vsync = true;
};

// Platform window + graphics surface. The renderer receives one of these at
// init and must not assume which windowing system is behind it.
class IWindow {
public:
    virtual ~IWindow() = default;

    virtual void pixelSize(int& w, int& h) const = 0;

    // Presents the current frame (buffer swap for GL). Backends that present
    // through their own swapchain treat this as a no-op.
    virtual void present() = 0;

    virtual void setRelativeMouseMode(bool enabled) = 0;
    virtual bool relativeMouseMode() const = 0;

    // Both are no-ops when the value is already current.
    virtual void setFullscreen(bool enabled) = 0;
    virtual void setVsync(bool enabled) = 0;

    // While active, the platform delivers typed text (InputState::typedText)
    // and may show an on-screen keyboard on touch platforms.
    virtual void setTextInputActive(bool enabled) = 0;

    // GL-family windows only: the loader used to resolve GL entry points for
    // the current context. Returns nullptr for non-GL windows.
    using GLProc = void* (*)(const char* name);
    virtual GLProc glProcLoader() const = 0;
};
