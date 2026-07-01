#pragma once

#include "renderer/renderer.h"

// Planned backend: Android/iOS bring-up path (OpenGL ES 3 / ANGLE). Compiles
// everywhere; init() fails with a clear message until implemented.
// Implementation notes: largely a port of GLRenderer with ES shader headers
// (#version 300 es) and without the desktop-only enums; the shared font5x7
// and RenderFrame data are reused as-is.
class GlesRenderer final : public IRenderer {
public:
    bool init(IWindow& window) override;
    void shutdown() override {}
    void render(const RenderFrame&) override {}
    const char* name() const override { return "Mobile GLES (stub)"; }
};
