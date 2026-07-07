#pragma once

#include "shared/world.h" // WorldBox / AABB

#include <glm/glm.hpp>

#include <string>
#include <vector>

// World templates: the starting layout a new world is created from. Each
// template is pure data - static geometry (floor/walls/obstacles), a spawn
// point, and a list of starting entity placements (by string id, resolved
// against the content registry by the caller). This is the foundation for
// "different kinds of worlds"; the same struct a future loader can fill from
// server/content/maps/*.cfg, so the templates stay data even while they are
// still defined in code today.
//
// Server-safe: std + glm only, no IO, no content lookups. The caller owns
// resolving placement ids into EntityDefs (World::addEntity) and file IO.

struct WorldTemplate {
    std::string id;          // stable id stored in world.cfg as world_type
    std::string displayName; // shown in the Create World menu
    std::string description; // one line explaining what the world is for

    std::vector<WorldBox> boxes;                // static geometry
    glm::vec3 spawnPoint{0.0f, 0.01f, 0.0f};    // where the player starts

    // Starting entities placed into a NEWLY created world. Loaded worlds
    // ignore these - their entities come from the save file instead.
    struct Placement {
        std::string defId;
        glm::vec3 pos{0.0f};
        float respawnSeconds = -1.0f; // < 0 = use the definition's default
    };
    std::vector<Placement> placements;
};

namespace worldgen {

// The built-in templates, in menu order. flat_sandbox is always first and is
// the fallback for old saves that predate world types.
const std::vector<WorldTemplate>& templates();

// Look up by id; nullptr if unknown (e.g. a save from a newer build).
const WorldTemplate* findTemplate(const std::string& id);

// flat_sandbox: the guaranteed fallback when a world has no/unknown type.
const WorldTemplate& fallbackTemplate();

} // namespace worldgen
