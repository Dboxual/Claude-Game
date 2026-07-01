#include "platform/sdl/sdl_window.h"

#include "engine/log.h"

namespace {

// Free-function wrapper so glProcLoader can return a plain function pointer.
void* sdlGlGetProc(const char* name) {
    return reinterpret_cast<void*>(SDL_GL_GetProcAddress(name));
}

} // namespace

std::unique_ptr<SdlWindow> SdlWindow::create(const WindowDesc& desc) {
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;

    if (desc.api == GraphicsApi::OpenGL) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // macOS core profile
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        flags |= SDL_WINDOW_OPENGL;
    }
    // Future backends: Vulkan adds SDL_WINDOW_VULKAN, Metal SDL_WINDOW_METAL.
    // The stubs fail at renderer init before surface creation matters.

    std::unique_ptr<SdlWindow> w(new SdlWindow());
    w->api_ = desc.api;
    w->window_ = SDL_CreateWindow(desc.title, desc.width, desc.height, flags);
    if (!w->window_) {
        eng::logError("SDL_CreateWindow failed: %s", SDL_GetError());
        return nullptr;
    }

    if (desc.api == GraphicsApi::OpenGL) {
        w->glContext_ = SDL_GL_CreateContext(w->window_);
        if (!w->glContext_) {
            eng::logError("SDL_GL_CreateContext failed: %s", SDL_GetError());
            return nullptr;
        }
        SDL_GL_SetSwapInterval(desc.vsync ? 1 : 0);
    }
    return w;
}

SdlWindow::~SdlWindow() {
    if (glContext_) SDL_GL_DestroyContext(glContext_);
    if (window_) SDL_DestroyWindow(window_);
}

void SdlWindow::pixelSize(int& w, int& h) const {
    w = h = 0;
    SDL_GetWindowSizeInPixels(window_, &w, &h);
}

void SdlWindow::present() {
    if (api_ == GraphicsApi::OpenGL) SDL_GL_SwapWindow(window_);
}

void SdlWindow::setRelativeMouseMode(bool enabled) {
    SDL_SetWindowRelativeMouseMode(window_, enabled);
}

bool SdlWindow::relativeMouseMode() const {
    return SDL_GetWindowRelativeMouseMode(window_);
}

IWindow::GLProc SdlWindow::glProcLoader() const {
    return api_ == GraphicsApi::OpenGL ? &sdlGlGetProc : nullptr;
}
