#include "client/platform/sdl/sdl_audio.h"

#include "shared/log.h"

namespace {
constexpr int kVoices = 8; // simultaneous one-shots; oldest requests just drop
}

bool SdlAudio::init() {
    initialized_ = SDL_InitSubSystem(SDL_INIT_AUDIO);
    if (!initialized_) {
        eng::logError("SDL audio init failed (continuing without audio): %s", SDL_GetError());
        return false;
    }

    device_ = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!device_) {
        eng::logError("SDL_OpenAudioDevice failed (continuing without audio): %s",
                      SDL_GetError());
        return false;
    }

    const char* driver = SDL_GetCurrentAudioDriver();
    eng::logInfo("Audio ready (driver: %s, %d voices)", driver ? driver : "none", kVoices);
    return true;
}

void SdlAudio::shutdown() {
    for (SDL_AudioStream* s : streams_) SDL_DestroyAudioStream(s); // unbinds too
    streams_.clear();
    sounds_.clear();
    if (device_) SDL_CloseAudioDevice(device_);
    device_ = 0;
    if (initialized_) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    initialized_ = false;
}

bool SdlAudio::loadSound(const std::string& name, const std::string& path) {
    if (!device_) return false;
    SDL_AudioSpec spec{};
    Uint8* buf = nullptr;
    Uint32 len = 0;
    if (!SDL_LoadWAV(path.c_str(), &spec, &buf, &len)) {
        eng::logError("Could not load sound '%s' from %s: %s", name.c_str(), path.c_str(),
                      SDL_GetError());
        return false;
    }
    Sound& s = sounds_[name];
    s.spec = spec;
    s.data.assign(buf, buf + len);
    SDL_free(buf);
    return true;
}

void SdlAudio::playSound(const std::string& name) {
    if (!device_) return;
    auto it = sounds_.find(name);
    if (it == sounds_.end()) return; // missing sounds are a silent no-op
    const Sound& sound = it->second;

    // Find an idle voice; grow the pool up to the cap on demand.
    SDL_AudioStream* voice = nullptr;
    for (SDL_AudioStream* s : streams_) {
        if (SDL_GetAudioStreamAvailable(s) == 0 && SDL_GetAudioStreamQueued(s) == 0) {
            voice = s;
            break;
        }
    }
    if (!voice && int(streams_.size()) < kVoices) {
        voice = SDL_CreateAudioStream(&sound.spec, nullptr);
        if (voice && !SDL_BindAudioStream(device_, voice)) {
            SDL_DestroyAudioStream(voice);
            voice = nullptr;
        }
        if (voice) streams_.push_back(voice);
    }
    if (!voice) return; // all voices busy: drop the one-shot

    SDL_SetAudioStreamFormat(voice, &sound.spec, nullptr);
    SDL_SetAudioStreamGain(voice, masterVolume_ * sfxVolume_);
    SDL_PutAudioStreamData(voice, sound.data.data(), int(sound.data.size()));
}

void SdlAudio::setVolumes(float master, float music, float sfx) {
    masterVolume_ = master;
    musicVolume_ = music; // stored for the day music exists
    sfxVolume_ = sfx;
}
