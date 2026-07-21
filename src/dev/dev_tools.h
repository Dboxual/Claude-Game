// Opt-in developer controls. This layer reads raw diagnostic keys and returns
// commands; normal gameplay never includes it unless the process starts with
// --dev, and production mechanics never depend on these shortcuts.
#pragma once

struct DevCommands {
    bool toggleOverlay = false;
    bool toggleFly = false;
    bool toggleTurbo = false;
    bool spawnReward = false;
    bool nextZone = false;
    bool returnToSpawn = false;
    bool advanceTime = false;
    float vertical = 0.0f;
};

class DevTools {
public:
    DevCommands Poll() const;
};
