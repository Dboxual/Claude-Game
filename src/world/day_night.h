// Presentation-level day/night cycle. Gameplay consequences (spawn tables,
// gathering bonuses, visibility, etc.) must be added by their owning systems;
// this class only advances world time and derives a coherent atmosphere from
// the active biome.
#pragma once

struct BiomeStyle;
class Renderer;

class DayNightCycle {
public:
    void Update(float dt, const BiomeStyle& biome, Renderer& renderer,
                bool advance = true);
    void AdvanceHours(float hours);
    void SetHour(float value);

    float Hour() const { return hour; }
    float Daylight() const { return daylight; }
    const char* ClockText() const;

private:
    void Apply(const BiomeStyle& biome, Renderer& renderer);

    float hour = 9.0f;
    float daylight = 1.0f;
    // A full day is 36 real minutes: visible during play, slow enough to keep
    // a stable mood. Dev mode can jump forward in three-hour increments.
    static constexpr float REAL_SECONDS_PER_GAME_DAY = 36.0f * 60.0f;
};
