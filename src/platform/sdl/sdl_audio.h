#pragma once

#include "platform/audio.h"

// Initializes the SDL audio subsystem so the device/driver plumbing is proven
// on every platform we run on, but plays nothing yet (Milestone 1 has no
// sounds by design).
class SdlAudio final : public IAudio {
public:
    bool init() override;
    void shutdown() override;
    const char* name() const override { return "SDL3 audio (no sounds yet)"; }

private:
    bool initialized_ = false;
};
