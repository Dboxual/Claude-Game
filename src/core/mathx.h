// Math helpers shared by every system. World units are meters, +Y is up,
// all times are float seconds.
#pragma once
#include "raylib.h"
#include "raymath.h"
#include <cmath>

// Framerate-independent exponential smoothing: moves a toward b, covering
// (1 - e^-rate) of the remaining distance per second. Stable for any dt.
inline float ExpDecay(float a, float b, float rate, float dt) {
    return b + (a - b) * expf(-rate * dt);
}
inline Vector3 ExpDecayV3(Vector3 a, Vector3 b, float rate, float dt) {
    float k = expf(-rate * dt);
    return { b.x + (a.x - b.x) * k, b.y + (a.y - b.y) * k, b.z + (a.z - b.z) * k };
}

// Moves a toward b by at most maxDelta (linear, for speeds/timers).
inline float Approach(float a, float b, float maxDelta) {
    if (a < b) return fminf(a + maxDelta, b);
    return fmaxf(a - maxDelta, b);
}

// (Remap comes from raymath.h)
inline float Saturate(float v) { return fminf(fmaxf(v, 0.0f), 1.0f); }

inline float EaseOutCubic(float t)  { t = Saturate(t); float u = 1.0f - t; return 1.0f - u * u * u; }
inline float EaseInOutCubic(float t){
    t = Saturate(t);
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}
inline float SmoothStep01(float t)  { t = Saturate(t); return t * t * (3.0f - 2.0f * t); }

// Shortest-path angle interpolation helpers (radians).
inline float WrapAngle(float a) {
    while (a >  PI) a -= 2.0f * PI;
    while (a < -PI) a += 2.0f * PI;
    return a;
}
inline float ExpDecayAngle(float a, float b, float rate, float dt) {
    return a + WrapAngle(b - a) * (1.0f - expf(-rate * dt));
}

inline Color ColorLerpF(Color a, Color b, float t) {
    t = Saturate(t);
    return { (unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
             (unsigned char)(a.b + (b.b - a.b) * t), (unsigned char)(a.a + (b.a - a.a) * t) };
}
// Multiply RGB by s (clamped); alpha preserved.
inline Color ColorMul(Color c, float s) {
    auto m = [&](unsigned char v) { float f = v * s; return (unsigned char)(f > 255.0f ? 255.0f : (f < 0.0f ? 0.0f : f)); };
    return { m(c.r), m(c.g), m(c.b), c.a };
}
