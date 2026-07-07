#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

// World save file (worlds/<folder>/world.cfg): plain text, one "key = value"
// per line, '#' comments. Repeatable 'entity' lines hold the placed objects:
//
//   version = 3
//   name = My World
//   world_type = duel_yard
//   entity = crate 3.00 0.00 5.00 0
//   entity = glock 1.50 0.00 3.00 10
//
// world_type (v3) is the world template the map geometry comes from; older
// files without it load fine and fall back to flat_sandbox. The optional 5th
// entity field is respawn seconds (v2); v1 files without it fall back to the
// definition's default. Runtime state (who picked what up, current health)
// is never saved.
//
// String <-> struct only; file IO stays with the caller (the client goes
// through IFileSystem, the dedicated server can use plain streams later).

struct WorldSaveData {
    std::string displayName;
    std::string worldType; // template id; empty = flat_sandbox (pre-v3 saves)
    int version = 3;

    struct Spawn {
        std::string defId;
        glm::vec3 pos{0.0f};
        float respawnSeconds = -1.0f; // < 0 = use the definition's default
    };
    std::vector<Spawn> entities;
};

std::string serializeWorldSave(const WorldSaveData& data);

// Tolerant parse: unknown keys are ignored, malformed entity lines skipped.
// Returns false only if the text has no recognizable world data at all.
bool parseWorldSave(const std::string& text, WorldSaveData& out);
