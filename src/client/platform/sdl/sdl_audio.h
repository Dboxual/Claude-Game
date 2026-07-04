#pragma once

#include "client/platform/audio.h"

// Initializes the SDL audio subsystem so the device/driver plumbing is proven
// on every platform we run on, but plays nothing yet (Milestone 1 has no
// sounds by design).
class SdlAudio final : public IAudio {
public:
    bool init() override;
    void shutdown() override;
    void setVolumes(float master, float music, float sfx) override;
    const char* name() const override { return "SDL3 audio (no sounds yet)"; }

private:
    bool initialized_ = false;
    float masterVolume_ = 1.0f;
    float musicVolume_ = 1.0f;
    float sfxVolume_ = 1.0f;
};
