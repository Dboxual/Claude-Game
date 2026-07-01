#pragma once

// Audio seam. Milestone 1 plays no sound; the interface exists so gameplay
// never talks to a platform audio API directly once sounds arrive.
class IAudio {
public:
    virtual ~IAudio() = default;

    virtual bool init() = 0;
    virtual void shutdown() = 0;
    virtual const char* name() const = 0;
};
