#pragma once

#include "client/platform/audio.h"

#include <SDL3/SDL.h>

#include <unordered_map>
#include <vector>

// SDL3 playback: one device, a small pool of bound audio streams for
// polyphony (SDL mixes bound streams). Each playSound pushes a loaded WAV
// into the first idle stream at the current master*sfx gain.
class SdlAudio final : public IAudio {
public:
    bool init() override;
    void shutdown() override;
    bool loadSound(const std::string& name, const std::string& path) override;
    void playSound(const std::string& name, float pitch) override;
    void setVolumes(float master, float music, float sfx) override;
    const char* name() const override { return "SDL3 audio"; }

private:
    struct Sound {
        SDL_AudioSpec spec{};
        std::vector<Uint8> data;
    };

    bool initialized_ = false;
    SDL_AudioDeviceID device_ = 0;
    std::vector<SDL_AudioStream*> streams_; // fixed voice pool, bound to device_
    size_t stealNext_ = 0; // round-robin voice steal when the pool is full
    std::unordered_map<std::string, Sound> sounds_;
    float masterVolume_ = 1.0f;
    float musicVolume_ = 1.0f;
    float sfxVolume_ = 1.0f;
};
