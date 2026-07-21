// Slot-based JSON saves in the user data dir. Phase 1 persists position,
// camera, and the reward counters; later phases extend the same schema
// (missing keys always default, so old saves keep loading).
#pragma once
#include "raylib.h"

struct SaveData {
    bool valid = false;          // false = no save on disk
    Vector3 playerPos = {};
    float yaw = 0.0f;
    float pitch = 0.0f;
    int cameraMode = 0;          // 0 FP, 1 TP
    int anima = 0;               // collected wisps
    int blessings = 0;           // shrines communed
    float playtime = 0.0f;       // seconds
    int currentZone = 4;         // zone id (DEFAULT_ZONE = Elder Vale)
};

namespace SaveSystem {
    SaveData Load(int slot);
    bool Write(int slot, const SaveData& data);
    bool Exists(int slot);
}
