#pragma once

#include <string>

// Audio seam. Gameplay asks for sounds by name; the platform side owns the
// device, decoding, mixing, and volume. All shipped sounds are original
// synthesized placeholders (see server/content/sounds/).
class IAudio {
public:
    virtual ~IAudio() = default;

    virtual bool init() = 0;
    virtual void shutdown() = 0;

    // Registers a named sound from a WAV file path. Safe to call for a
    // missing file: it logs and later playSound() calls become no-ops.
    virtual bool loadSound(const std::string& name, const std::string& path) = 0;

    // Fire-and-forget one-shot playback at the current master*sfx volume.
    // pitch scales playback speed (1 = as recorded); small variations keep
    // repeated sounds (footsteps) from feeling machine-gunned. Unknown
    // names are silently ignored so gameplay never has to care whether
    // audio is available.
    virtual void playSound(const std::string& name, float pitch = 1.0f) = 0;

    // Volumes are 0..1. Applied to every subsequent playSound.
    virtual void setVolumes(float master, float music, float sfx) = 0;

    virtual const char* name() const = 0;
};
