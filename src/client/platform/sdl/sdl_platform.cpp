#include "client/platform/sdl/sdl_platform.h"

#include "shared/log.h"
#include "client/platform/sdl/sdl_audio.h"
#include "client/platform/sdl/sdl_filesystem.h"
#include "client/platform/sdl/sdl_input.h"
#include "client/platform/sdl/sdl_window.h"

#include <SDL3/SDL.h>

PlatformSystems createSdlPlatform(const WindowDesc& desc) {
    PlatformSystems systems;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        eng::logError("SDL_Init failed: %s", SDL_GetError());
        return systems;
    }

    auto window = SdlWindow::create(desc);
    if (!window) {
        SDL_Quit();
        return systems;
    }

    systems.input = std::make_unique<SdlInput>(*window);
    systems.fileSystem = std::make_unique<SdlFileSystem>();
    systems.audio = std::make_unique<SdlAudio>();
    systems.window = std::move(window);
    return systems;
}

void destroySdlPlatform(PlatformSystems& systems) {
    systems.input.reset();
    if (systems.audio) systems.audio->shutdown();
    systems.audio.reset();
    systems.window.reset(); // destroys the GL context before SDL_Quit
    systems.fileSystem.reset();
    SDL_Quit();
}
