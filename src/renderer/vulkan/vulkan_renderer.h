#pragma once

#include "renderer/renderer.h"

// Planned backend: Windows/Linux/Android. Compiles everywhere (references no
// Vulkan headers); init() fails with a clear message until implemented.
// Implementation notes: the SDL window will add SDL_WINDOW_VULKAN and expose
// surface-creation hooks next to IWindow::glProcLoader().
class VulkanRenderer final : public IRenderer {
public:
    bool init(IWindow& window) override;
    void shutdown() override {}
    void render(const RenderFrame&) override {}
    const char* name() const override { return "Vulkan (stub)"; }
};
