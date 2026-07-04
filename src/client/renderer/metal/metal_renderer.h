#pragma once

#include "client/renderer/renderer.h"

// Planned backend: macOS/iOS. Compiles everywhere (references no Metal
// headers); init() fails with a clear message until implemented.
// Implementation notes: will live in an Objective-C++ (.mm) file on Apple
// platforms and acquire a CAMetalLayer from the SDL window.
class MetalRenderer final : public IRenderer {
public:
    bool init(IWindow& window) override;
    void shutdown() override {}
    void render(const RenderFrame&) override {}
    const char* name() const override { return "Metal (stub)"; }
};
