// tools/generate_sounds.cpp
//
// Generates every sound effect in server/content/sounds/ from scratch using
// nothing but sine/noise synthesis - no recordings, no third-party samples,
// no game rips. Everything this program outputs is original work dedicated
// to the public domain (CC0); the per-file listing lives in
// server/content/sounds/CREDITS.md.
//
// Build & run from the repo root (any C++17 compiler, no dependencies):
//   MSVC:      cl /EHsc /O2 /std:c++17 tools\generate_sounds.cpp
//              generate_sounds.exe
//   clang/gcc: c++ -O2 -std=c++17 tools/generate_sounds.cpp -o generate_sounds
//              ./generate_sounds
// Optional argv[1]: output directory (default "server/content/sounds/").

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {

constexpr int kRate = 44100; // mono, 16-bit PCM

// Deterministic noise so re-running the tool reproduces identical files.
struct Rng {
    std::uint32_t s = 0x2545F491u;
    float next() { // uniform [-1, 1)
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return float(s) / 2147483648.0f - 1.0f;
    }
};

// One-pole filters; cutoff in Hz. Call step() once per sample.
struct OnePole {
    float y = 0.0f;
    float step(float x, float cutoffHz) {
        float a = 1.0f - std::exp(-2.0f * 3.14159265f * cutoffHz / kRate);
        y += a * (x - y);
        return y;
    }
};

float envExp(float t, float tau) { return std::exp(-t / tau); }

// sin^2 bell over [0, dur]: silent at both ends, peak in the middle.
float envBell(float t, float dur) {
    if (t < 0.0f || t > dur) return 0.0f;
    float s = std::sin(3.14159265f * t / dur);
    return s * s;
}

std::vector<float> makeBuffer(float seconds) {
    return std::vector<float>(size_t(seconds * kRate), 0.0f);
}

// Peak-normalize to the requested level and fade the last 5 ms so no sound
// ends on a click.
void finalize(std::vector<float>& buf, float peakTarget) {
    float peak = 0.0f;
    for (float v : buf) peak = std::max(peak, std::abs(v));
    if (peak > 0.0f) {
        float g = peakTarget / peak;
        for (float& v : buf) v *= g;
    }
    size_t fade = size_t(0.005f * kRate);
    fade = std::min(fade, buf.size());
    for (size_t i = 0; i < fade; ++i) {
        buf[buf.size() - fade + i] *= 1.0f - float(i + 1) / float(fade);
    }
}

bool writeWav(const std::string& path, const std::vector<float>& buf) {
    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) {
        std::fprintf(stderr, "ERROR: cannot write %s\n", path.c_str());
        return false;
    }
    auto u32 = [&](std::uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto u16 = [&](std::uint16_t v) { std::fwrite(&v, 2, 1, f); };
    std::uint32_t dataBytes = std::uint32_t(buf.size() * 2);
    std::fwrite("RIFF", 1, 4, f); u32(36 + dataBytes); std::fwrite("WAVE", 1, 4, f);
    std::fwrite("fmt ", 1, 4, f); u32(16);
    u16(1); u16(1); u32(kRate); u32(kRate * 2); u16(2); u16(16); // PCM mono 16-bit
    std::fwrite("data", 1, 4, f); u32(dataBytes);
    for (float v : buf) {
        float c = std::max(-1.0f, std::min(1.0f, v));
        std::int16_t s = std::int16_t(std::lround(c * 32767.0f));
        std::fwrite(&s, 2, 1, f);
    }
    std::fclose(f);
    std::printf("wrote %s (%zu samples)\n", path.c_str(), buf.size());
    return true;
}

// --- the seven sounds ----------------------------------------------------

// Footstep: low noise thud - noise through a falling lowpass plus a 70 Hz
// heel bump. Soft on purpose; it plays constantly.
std::vector<float> footstep() {
    auto buf = makeBuffer(0.15f);
    Rng rng; OnePole lp;
    for (size_t i = 0; i < buf.size(); ++i) {
        float t = float(i) / kRate;
        float cutoff = 420.0f - 240.0f * std::min(t / 0.12f, 1.0f);
        float noise = lp.step(rng.next(), cutoff) * envExp(t, 0.035f);
        float thump = std::sin(2 * 3.14159265f * 70.0f * t) * envExp(t, 0.05f) * 0.6f;
        buf[i] = noise + thump;
    }
    finalize(buf, 0.55f);
    return buf;
}

// Pickup: two quick rising sine notes (D5 -> A5) with a light 2nd harmonic.
std::vector<float> pickup() {
    auto buf = makeBuffer(0.30f);
    auto note = [&](float startS, float freq, float amp) {
        size_t s0 = size_t(startS * kRate);
        for (size_t i = s0; i < buf.size(); ++i) {
            float t = float(i - s0) / kRate;
            float env = std::min(t / 0.008f, 1.0f) * envExp(t, 0.09f);
            float v = std::sin(2 * 3.14159265f * freq * t)
                      + 0.25f * std::sin(2 * 3.14159265f * freq * 2.0f * t);
            buf[i] += v * env * amp;
        }
    };
    note(0.0f, 587.33f, 0.8f);  // D5
    note(0.09f, 880.0f, 1.0f);  // A5
    finalize(buf, 0.45f);
    return buf;
}

// Melee swing: a whoosh - noise band (lowpass minus a trailing lowpass)
// whose center sweeps upward under a bell envelope.
std::vector<float> swing() {
    auto buf = makeBuffer(0.28f);
    Rng rng; OnePole lpHi, lpLo;
    for (size_t i = 0; i < buf.size(); ++i) {
        float t = float(i) / kRate;
        float sweep = t / 0.28f;
        float hi = 600.0f + 1800.0f * sweep; // band top
        float lo = 250.0f + 700.0f * sweep;  // band bottom
        float n = rng.next();
        float band = lpHi.step(n, hi) - lpLo.step(n, lo);
        buf[i] = band * envBell(t, 0.26f);
    }
    finalize(buf, 0.50f);
    return buf;
}

// Glock shot: an instant noise crack whose brightness collapses fast, over
// a pitch-dropping low thump, saturated for punch.
std::vector<float> gunshot() {
    auto buf = makeBuffer(0.40f);
    Rng rng; OnePole lp;
    for (size_t i = 0; i < buf.size(); ++i) {
        float t = float(i) / kRate;
        float bright = 8000.0f * envExp(t, 0.05f) + 400.0f;
        float crack = lp.step(rng.next(), bright) * envExp(t, 0.09f) * 1.6f;
        float f = 110.0f - 50.0f * std::min(t / 0.15f, 1.0f);
        float body = std::sin(2 * 3.14159265f * f * t) * envExp(t, 0.09f) * 0.9f;
        buf[i] = std::tanh((crack + body) * 1.8f);
    }
    finalize(buf, 0.95f);
    return buf;
}

// Dummy hit: dull body impact - a pitch-dropping low sine knock with a
// short mid noise burst.
std::vector<float> dummyHit() {
    auto buf = makeBuffer(0.20f);
    Rng rng; OnePole lp;
    for (size_t i = 0; i < buf.size(); ++i) {
        float t = float(i) / kRate;
        float f = 130.0f - 40.0f * std::min(t / 0.12f, 1.0f);
        float knock = std::sin(2 * 3.14159265f * f * t) * envExp(t, 0.06f);
        float thud = lp.step(rng.next(), 700.0f) * envExp(t, 0.025f) * 0.7f;
        buf[i] = std::tanh((knock + thud) * 1.4f);
    }
    finalize(buf, 0.70f);
    return buf;
}

// Melee hit: sharper and brighter than the dummy thud - a high crack (noise
// above 1.2 kHz) over a short 200 Hz knock.
std::vector<float> meleeHit() {
    auto buf = makeBuffer(0.14f);
    Rng rng; OnePole lp;
    for (size_t i = 0; i < buf.size(); ++i) {
        float t = float(i) / kRate;
        float n = rng.next();
        float crack = (n - lp.step(n, 1200.0f)) * envExp(t, 0.012f) * 1.1f;
        float knock = std::sin(2 * 3.14159265f * 200.0f * t) * envExp(t, 0.04f) * 0.8f;
        buf[i] = std::tanh((crack + knock) * 1.5f);
    }
    finalize(buf, 0.70f);
    return buf;
}

// UI click: a tiny 1.9 kHz tick with a breath of noise. Quiet by design.
std::vector<float> uiClick() {
    auto buf = makeBuffer(0.06f);
    Rng rng;
    for (size_t i = 0; i < buf.size(); ++i) {
        float t = float(i) / kRate;
        float tick = std::sin(2 * 3.14159265f * 1900.0f * t) * envExp(t, 0.009f);
        float n = rng.next() * envExp(t, 0.003f) * 0.3f;
        buf[i] = tick + n;
    }
    finalize(buf, 0.35f);
    return buf;
}

} // namespace

int main(int argc, char** argv) {
    std::string outDir = argc > 1 ? argv[1] : "server/content/sounds/";
    if (!outDir.empty() && outDir.back() != '/' && outDir.back() != '\\') outDir += '/';

    struct Entry {
        const char* name;
        std::vector<float> (*make)();
    };
    const Entry entries[] = {
        {"footstep", footstep}, {"pickup", pickup},      {"swing", swing},
        {"gunshot", gunshot},   {"dummy_hit", dummyHit}, {"melee_hit", meleeHit},
        {"ui_click", uiClick},
    };

    bool ok = true;
    for (const Entry& e : entries) {
        ok &= writeWav(outDir + e.name + ".wav", e.make());
    }
    return ok ? 0 : 1;
}
