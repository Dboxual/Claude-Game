#pragma once

#include "platform/window.h"

#include <SDL3/SDL.h>

#include <memory>

class SdlWindow final : public IWindow {
public:
    static std::unique_ptr<SdlWindow> create(const WindowDesc& desc);
    ~SdlWindow() override;

    void pixelSize(int& w, int& h) const override;
    void present() override;
    void setRelativeMouseMode(bool enabled) override;
    bool relativeMouseMode() const override;
    GLProc glProcLoader() const override;

    SDL_Window* sdlWindow() const { return window_; }

private:
    SdlWindow() = default;

    SDL_Window* window_ = nullptr;
    SDL_GLContext glContext_ = nullptr;
    GraphicsApi api_ = GraphicsApi::OpenGL;
};
