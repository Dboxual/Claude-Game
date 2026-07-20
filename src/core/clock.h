// Fixed-timestep accumulator. Gameplay simulates at SIM_HZ regardless of
// framerate; rendering interpolates between the last two sim states using
// Alpha(). See ARCHITECTURE.md "Frame model".
#pragma once

constexpr float SIM_HZ = 60.0f;
constexpr float SIM_DT = 1.0f / SIM_HZ;

class FixedClock {
public:
    // Feeds one frame's dt; returns how many fixed steps to run now.
    // Steps are capped so a long hitch (debugger, window drag) cannot
    // trigger a spiral of death — excess time is dropped.
    int AddFrame(float dt, int maxSteps = 5) {
        accumulator += dt;
        int steps = 0;
        while (accumulator >= SIM_DT && steps < maxSteps) { accumulator -= SIM_DT; steps++; }
        if (accumulator > SIM_DT) accumulator = SIM_DT;   // drop the backlog
        return steps;
    }

    // Interpolation factor between previous and current sim state [0,1).
    float Alpha() const { return accumulator / SIM_DT; }

private:
    float accumulator = 0.0f;
};
