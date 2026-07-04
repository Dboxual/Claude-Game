#include "client/renderer/metal/metal_renderer.h"

#include "shared/log.h"

bool MetalRenderer::init(IWindow&) {
    eng::logError("Metal backend is not implemented yet (planned: macOS/iOS). "
                  "Run with --renderer gl for now.");
    return false;
}
