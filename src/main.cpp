// TacMove - composition root.
//
// This is the only file where the layers meet: it picks a platform factory
// (SDL3 desktop), picks a renderer backend, and pumps the loop. Everything
// below it talks through interfaces (IWindow/IInput/IFileSystem/IAudio,
// IRenderer), so porting means providing new factories - not touching the
// game.

#include "engine/log.h"
#include "game/game.h"
#include "platform/platform.h"
#include "platform/sdl/sdl_platform.h"
#include "renderer/renderer.h"

#include <chrono>
#include <cstdlib>
#include <cstring>

namespace {

GraphicsApi parseRendererArg(const char* value) {
    if (std::strcmp(value, "vulkan") == 0) return GraphicsApi::Vulkan;
    if (std::strcmp(value, "metal") == 0) return GraphicsApi::Metal;
    if (std::strcmp(value, "gles") == 0) return GraphicsApi::MobileGLES;
    if (std::strcmp(value, "gl") != 0 && std::strcmp(value, "opengl") != 0) {
        eng::logError("Unknown renderer '%s' (expected gl, vulkan, metal, or gles); using OpenGL", value);
    }
    return GraphicsApi::OpenGL;
}

} // namespace

int main(int argc, char** argv) {
    long long maxFrames = -1; // --frames N: exit after N frames (smoke testing)
    WindowDesc desc;
    desc.title = "TacMove - Movement Prototype (Milestone 1)";

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            maxFrames = std::atoll(argv[++i]);
        } else if (std::strcmp(argv[i], "--novsync") == 0) {
            desc.vsync = false;
        } else if (std::strcmp(argv[i], "--renderer") == 0 && i + 1 < argc) {
            desc.api = parseRendererArg(argv[++i]);
        }
    }

    PlatformSystems platform = createSdlPlatform(desc);
    if (!platform.valid()) {
        eng::logError("Platform init failed");
        return 1;
    }
    platform.audio->init(); // failure is non-fatal: we just run silent

    std::unique_ptr<IRenderer> renderer = createRenderer(desc.api);
    if (!renderer || !renderer->init(*platform.window)) {
        eng::logError("Renderer backend '%s' failed to initialize", graphicsApiName(desc.api));
        destroySdlPlatform(platform);
        return 1;
    }
    eng::logInfo("Renderer backend: %s", renderer->name());

    Game game;
    if (!game.init(*platform.fileSystem)) {
        eng::logError("Game init failed");
        renderer->shutdown();
        destroySdlPlatform(platform);
        return 1;
    }
    game.setRendererName(renderer->name());

    platform.window->setRelativeMouseMode(true);

    using Clock = std::chrono::steady_clock;
    Clock::time_point last = Clock::now();
    long long frameCount = 0;
    bool running = true;

    while (running) {
        const InputState& in = platform.input->pump();
        if (in.quitRequested) break;

        // Mouse-capture policy lives here, not in the input layer.
        if (in.escapePressed) {
            platform.window->setRelativeMouseMode(!platform.window->relativeMouseMode());
        } else if (in.mouseClicked) {
            platform.window->setRelativeMouseMode(true);
        }

        Clock::time_point now = Clock::now();
        double frameDt = std::chrono::duration<double>(now - last).count();
        last = now;
        frameDt = std::min(frameDt, 0.25);

        game.update(in, frameDt);

        int w = 0, h = 0;
        platform.window->pixelSize(w, h);
        renderer->render(game.buildRenderFrame(w, h));

        ++frameCount;
        if (maxFrames >= 0 && frameCount >= maxFrames) running = false;
    }

    renderer->shutdown();
    destroySdlPlatform(platform);
    return 0;
}
