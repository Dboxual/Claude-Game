#include "client/platform/sdl/sdl_audio.h"

#include "shared/log.h"

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

void SdlAudio::setVolumes(float master, float music, float sfx) {
    // Nothing plays yet; stored so the future mixer starts at the user's levels.
    masterVolume_ = master;
    musicVolume_ = music;
    sfxVolume_ = sfx;
}
