#include "audio/audio.h"
#include "core/log.h"
#include "settings/settings.h"
#include <cmath>
#include <cstring>
#include <vector>

static constexpr int SR = 22050;   // synthesis sample rate (mono, 16-bit)
static constexpr float TAU = 6.28318530718f;

// ---- synthesis toolkit ----------------------------------------------------

static float NoiseF(unsigned int& st) {
    st ^= st << 13; st ^= st >> 17; st ^= st << 5;
    return ((st & 0xFFFFFF) / 8388607.5f) - 1.0f;
}

static Sound BakeSound(std::vector<float>& s) {
    // Normalize to 80% and quantize to PCM16.
    float peak = 1e-5f;
    for (float v : s) peak = fmaxf(peak, fabsf(v));
    std::vector<short> pcm(s.size());
    for (size_t i = 0; i < s.size(); i++)
        pcm[i] = (short)(s[i] / peak * 0.8f * 32767.0f);

    Wave w = {};
    w.frameCount = (unsigned int)pcm.size();
    w.sampleRate = SR;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = pcm.data();
    return LoadSoundFromWave(w);   // raylib copies the buffer
}

static std::vector<float> Seconds(float t) { return std::vector<float>((size_t)(SR * t), 0.0f); }

// One-pole lowpass state helper.
struct LP { float v = 0; float Step(float in, float a) { v += (in - v) * a; return v; } };

// ---- the placeholder sound set -------------------------------------------

static Sound SynthFootstep(unsigned int& rng) {
    auto s = Seconds(0.07f);
    LP lp;
    for (size_t i = 0; i < s.size(); i++) {
        float t = (float)i / SR;
        float env = expf(-t * 55.0f);
        s[i] = lp.Step(NoiseF(rng), 0.18f) * env
             + sinf(TAU * 95.0f * t) * env * 0.35f;
    }
    return BakeSound(s);
}

static Sound SynthJump(unsigned int& rng) {
    auto s = Seconds(0.14f);
    LP lp;
    for (size_t i = 0; i < s.size(); i++) {
        float t = (float)i / SR;
        float x = t / 0.14f;
        float env = sinf(x * PI);                      // swell in and out
        float cutoff = 0.08f + x * 0.4f;               // opening filter=whoosh
        s[i] = lp.Step(NoiseF(rng), cutoff) * env * 0.8f;
    }
    return BakeSound(s);
}

static Sound SynthLand(unsigned int& rng) {
    auto s = Seconds(0.16f);
    LP lp;
    for (size_t i = 0; i < s.size(); i++) {
        float t = (float)i / SR;
        float env = expf(-t * 26.0f);
        s[i] = sinf(TAU * 82.0f * t * (1.0f - t * 0.8f)) * env
             + lp.Step(NoiseF(rng), 0.3f) * expf(-t * 70.0f) * 0.5f;
    }
    return BakeSound(s);
}

static Sound SynthUiHover(unsigned int&) {
    auto s = Seconds(0.045f);
    for (size_t i = 0; i < s.size(); i++) {
        float t = (float)i / SR;
        s[i] = sinf(TAU * 1250.0f * t) * expf(-t * 90.0f) * 0.5f;
    }
    return BakeSound(s);
}

static Sound SynthUiClick(unsigned int&) {
    auto s = Seconds(0.09f);
    for (size_t i = 0; i < s.size(); i++) {
        float t = (float)i / SR;
        float f = (t < 0.035f) ? 880.0f : 1320.0f;     // two-tone tick
        float lt = (t < 0.035f) ? t : t - 0.035f;
        s[i] = sinf(TAU * f * lt) * expf(-lt * 70.0f) * 0.6f;
    }
    return BakeSound(s);
}

// Shared bell voice: fundamental + soft octave shimmer, long decay.
static void AddBell(std::vector<float>& s, float freq, float start, float amp, float decay) {
    size_t i0 = (size_t)(start * SR);
    for (size_t i = i0; i < s.size(); i++) {
        float t = (float)(i - i0) / SR;
        float env = expf(-t * decay) * amp;
        s[i] += (sinf(TAU * freq * t) + sinf(TAU * freq * 2.005f * t) * 0.35f) * env;
    }
}

static Sound SynthShrineChime(unsigned int&) {
    auto s = Seconds(1.1f);
    AddBell(s, 523.25f, 0.00f, 0.9f, 3.2f);   // C5
    AddBell(s, 659.25f, 0.14f, 0.8f, 3.2f);   // E5
    AddBell(s, 783.99f, 0.28f, 0.9f, 2.6f);   // G5
    return BakeSound(s);
}

static Sound SynthWispPickup(unsigned int&) {
    auto s = Seconds(0.28f);
    for (size_t i = 0; i < s.size(); i++) {
        float t = (float)i / SR;
        float f = 880.0f * (1.0f + t * 1.1f);          // rising = reward
        float env = expf(-t * 14.0f);
        s[i] = (sinf(TAU * f * t) + sinf(TAU * f * 2.0f * t) * 0.3f) * env;
    }
    return BakeSound(s);
}

static Sound SynthBlessing(unsigned int& rng) {
    auto s = Seconds(1.6f);
    AddBell(s, 261.63f, 0.0f, 0.55f, 1.6f);   // C4 major chord, slow bloom
    AddBell(s, 329.63f, 0.05f, 0.5f, 1.6f);
    AddBell(s, 392.00f, 0.10f, 0.5f, 1.6f);
    AddBell(s, 523.25f, 0.18f, 0.65f, 1.4f);
    LP lp;
    for (size_t i = 0; i < s.size(); i++) {    // faint magical air on top
        float t = (float)i / SR;
        s[i] += lp.Step(NoiseF(rng), 0.6f) * expf(-t * 2.0f) * 0.04f;
    }
    return BakeSound(s);
}

// 8-second seamless wind loop, packaged as an in-memory WAV for streaming.
static unsigned char* BuildWindWav(unsigned int& rng, int* outSize) {
    const float len = 8.0f;
    const int frames = (int)(SR * len);
    std::vector<float> s(frames);
    LP lp1, lp2;
    float brown = 0.0f;
    for (int i = 0; i < frames; i++) {
        float t = (float)i / SR;
        brown = (brown + NoiseF(rng) * 0.035f) * 0.997f;
        float gust = 0.55f + 0.45f * sinf(TAU * t / len);        // one LFO cycle = seamless
        s[i] = lp2.Step(lp1.Step(brown, 0.12f), 0.12f) * gust;
    }
    // Crossfade tail into head to hide the noise seam.
    int xf = SR / 2;
    for (int i = 0; i < xf; i++) {
        float a = (float)i / xf;
        s[frames - xf + i] = s[frames - xf + i] * (1.0f - a) + s[i] * a;
    }

    float peak = 1e-5f;
    for (float v : s) peak = fmaxf(peak, fabsf(v));

    int dataBytes = frames * 2;
    int total = 44 + dataBytes;
    unsigned char* buf = (unsigned char*)MemAlloc(total);
    // Minimal RIFF/WAVE header (PCM16 mono).
    auto put32 = [&](int off, unsigned int v) { memcpy(buf + off, &v, 4); };
    auto put16 = [&](int off, unsigned short v) { memcpy(buf + off, &v, 2); };
    memcpy(buf, "RIFF", 4); put32(4, total - 8); memcpy(buf + 8, "WAVE", 4);
    memcpy(buf + 12, "fmt ", 4); put32(16, 16); put16(20, 1); put16(22, 1);
    put32(24, SR); put32(28, SR * 2); put16(32, 2); put16(34, 16);
    memcpy(buf + 36, "data", 4); put32(40, dataBytes);
    short* pcm = (short*)(buf + 44);
    for (int i = 0; i < frames; i++) pcm[i] = (short)(s[i] / peak * 0.7f * 32767.0f);

    *outSize = total;
    return buf;
}

// ---- system ---------------------------------------------------------------

float AudioSystem::Rand01() {
    rngState ^= rngState << 13; rngState ^= rngState >> 17; rngState ^= rngState << 5;
    return (rngState & 0xFFFF) / 65535.0f;
}

void AudioSystem::Init(const AudioSettings& s) {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) { LOG_WARN("Audio device unavailable; running silent"); return; }

    unsigned int rng = 0x5EED5u;
    sounds[(int)SoundId::Footstep] = SynthFootstep(rng);
    sounds[(int)SoundId::Jump] = SynthJump(rng);
    sounds[(int)SoundId::Land] = SynthLand(rng);
    sounds[(int)SoundId::UiHover] = SynthUiHover(rng);
    sounds[(int)SoundId::UiClick] = SynthUiClick(rng);
    sounds[(int)SoundId::ShrineChime] = SynthShrineChime(rng);
    sounds[(int)SoundId::WispPickup] = SynthWispPickup(rng);
    sounds[(int)SoundId::Blessing] = SynthBlessing(rng);

    int windSize = 0;
    windData = BuildWindWav(rng, &windSize);
    wind = LoadMusicStreamFromMemory(".wav", windData, windSize);
    wind.looping = true;
    PlayMusicStream(wind);

    ready = true;
    Apply(s);
    LOG_INFO("Audio ready: %d synthesized sounds + wind ambience", (int)SoundId::COUNT);
}

void AudioSystem::Shutdown() {
    if (ready) {
        StopMusicStream(wind);
        UnloadMusicStream(wind);
        for (Sound& snd : sounds) UnloadSound(snd);
    }
    if (windData) { MemFree(windData); windData = nullptr; }
    CloseAudioDevice();
}

void AudioSystem::Apply(const AudioSettings& s) {
    busMaster = s.master;
    busSfx = s.sfx;
    busAmbience = s.ambience;
    if (ready) SetMusicVolume(wind, busAmbience * busMaster * 0.55f);
}

void AudioSystem::Update(float) {
    if (ready) UpdateMusicStream(wind);
}

void AudioSystem::Play(SoundId id, float volume, float pitch, bool jitter) {
    if (!ready) return;
    Sound& s = sounds[(int)id];
    if (jitter) pitch *= 0.95f + Rand01() * 0.1f;
    SetSoundVolume(s, volume * busSfx * busMaster);
    SetSoundPitch(s, pitch);
    PlaySound(s);
}
