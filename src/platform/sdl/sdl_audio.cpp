#include "platform/sdl/sdl_audio.h"

#include "engine/log.h"

#include <SDL3/SDL.h>

bool SdlAudio::init() {
    initialized_ = SDL_InitSubSystem(SDL_INIT_AUDIO);
    if (!initialized_) {
        eng::logError("SDL audio init failed (continuing without audio): %s", SDL_GetError());
        return false;
    }
    const char* driver = SDL_GetCurrentAudioDriver();
    eng::logInfo("Audio ready (driver: %s)", driver ? driver : "none");
    return true;
}

void SdlAudio::shutdown() {
    if (initialized_) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    initialized_ = false;
}
