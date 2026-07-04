#include "client/renderer/mobile/gles_renderer.h"

#include "shared/log.h"

bool GlesRenderer::init(IWindow&) {
    eng::logError("Mobile GLES backend is not implemented yet (planned: Android/iOS). "
                  "Run with --renderer gl for now.");
    return false;
}
