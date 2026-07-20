// Audio framework: bus volumes (master/sfx/music/ambience), a synthesized
// placeholder sound set (no asset files), and a looping wind-ambience
// stream. Real recorded assets can replace the generators later behind the
// same SoundId interface.
#pragma once
#include "raylib.h"

struct AudioSettings;

enum class SoundId : int {
    Footstep, Jump, Land,
    UiHover, UiClick,
    ShrineChime, WispPickup, Blessing,
    COUNT
};

class AudioSystem {
public:
    void Init(const AudioSettings& s);
    void Shutdown();
    void Apply(const AudioSettings& s);          // live volume changes
    void Update(float dt);                       // ambience stream upkeep

    // pitch jitters slightly by default so repeats never sound robotic.
    void Play(SoundId id, float volume = 1.0f, float pitch = 1.0f, bool jitter = true);

private:
    Sound sounds[(int)SoundId::COUNT] = {};
    Music wind = {};
    unsigned char* windData = nullptr;           // backing WAV bytes for the stream
    float busMaster = 0.8f, busSfx = 1.0f, busAmbience = 0.9f;
    unsigned int rngState = 0xBEEFu;
    float Rand01();
    bool ready = false;
};
