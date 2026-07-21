#include "world/day_night.h"
#include "core/mathx.h"
#include "render/renderer.h"
#include "zone/zone_def.h"
#include <cstdio>

static Vector3 MixV3(Vector3 a, Vector3 b, float t) {
    return Vector3Lerp(a, b, Saturate(t));
}

void DayNightCycle::AdvanceHours(float hours) {
    hour = fmodf(hour + hours, 24.0f);
    if (hour < 0.0f) hour += 24.0f;
}

void DayNightCycle::SetHour(float value) {
    hour = fmodf(value, 24.0f);
    if (hour < 0.0f) hour += 24.0f;
}

void DayNightCycle::Update(float dt, const BiomeStyle& biome, Renderer& renderer,
                           bool advance) {
    if (advance) AdvanceHours(dt * 24.0f / REAL_SECONDS_PER_GAME_DAY);
    Apply(biome, renderer);
}

void DayNightCycle::Apply(const BiomeStyle& b, Renderer& r) {
    // Solar elevation: sunrise 06:00, noon 12:00, sunset 18:00.
    float solar = (hour - 6.0f) / 12.0f * PI;
    float elevation = sinf(solar);
    daylight = SmoothStep01((elevation + 0.08f) / 0.34f);
    float twilight = 1.0f - SmoothStep01(fabsf(elevation) / 0.28f);

    // sunTo points from the world toward the sun; the lighting shader expects
    // the direction light travels, hence the negation.
    float azimuth = (hour - 6.0f) / 24.0f * PI * 2.0f;
    Vector3 sunTo = Vector3Normalize({ cosf(azimuth) * 0.78f, elevation,
                                       sinf(azimuth) * 0.78f });
    r.lighting.sunDir = Vector3Negate(sunTo);

    // Nights stay saturated and readable rather than crushing the terrain to
    // black: PS2-era moonlight is a color mood, not a visibility punishment.
    const Vector3 nightZenith = { 0.030f, 0.060f, 0.145f };
    const Vector3 nightHorizon = { 0.080f, 0.125f, 0.215f };
    const Vector3 nightFog = { 0.070f, 0.105f, 0.165f };
    const Vector3 nightSkyAmbient = { 0.300f, 0.380f, 0.560f };
    const Vector3 nightGroundAmbient = { 0.210f, 0.255f, 0.360f };
    const Vector3 moonLight = { 0.50f, 0.58f, 0.78f };
    const Vector3 sunsetLight = { 1.30f, 0.66f, 0.34f };
    const Vector3 sunsetHorizon = { 0.92f, 0.39f, 0.22f };

    Vector3 horizon = MixV3(nightHorizon, b.fogColor, daylight);
    horizon = MixV3(horizon, sunsetHorizon, twilight * 0.48f);

    r.sky.zenithColor = MixV3(nightZenith, b.skyZenith, daylight);
    r.sky.horizonColor = horizon;
    r.sky.sunColor = MixV3(moonLight, b.sunColor, daylight);
    r.sky.sunColor = MixV3(r.sky.sunColor, sunsetLight, twilight * 0.55f);

    r.lighting.fogColor = MixV3(nightFog, b.fogColor, daylight);
    r.lighting.skyAmbient = MixV3(nightSkyAmbient, b.skyAmbient, daylight);
    r.lighting.groundAmbient = MixV3(nightGroundAmbient, b.groundAmbient, daylight);
    r.lighting.sunColor = MixV3(moonLight, b.sunColor, daylight);
    r.lighting.sunColor = MixV3(r.lighting.sunColor, sunsetLight, twilight * 0.65f);
    r.lighting.fogDensity = b.fogDensity * (1.0f + (1.0f - daylight) * 0.16f);
}

const char* DayNightCycle::ClockText() const {
    static char text[16];
    int totalMinutes = (int)(hour * 60.0f + 0.5f) % (24 * 60);
    int h24 = totalMinutes / 60;
    int minutes = totalMinutes % 60;
    const char* suffix = h24 >= 12 ? "PM" : "AM";
    int h12 = h24 % 12;
    if (h12 == 0) h12 = 12;
    snprintf(text, sizeof(text), "%d:%02d %s", h12, minutes, suffix);
    return text;
}
