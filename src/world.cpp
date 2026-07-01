#include "world.h"

void World::add(glm::vec3 min, glm::vec3 max, glm::vec3 color, bool checkerTop) {
    boxes_.push_back(WorldBox{AABB{min, max}, color, checkerTop});
}

bool World::overlapsAny(const AABB& box) const {
    for (const WorldBox& wb : boxes_) {
        if (aabbOverlap(box, wb.box)) return true;
    }
    return false;
}

// A 40x40 m enclosed arena. There is intentionally no step-up mechanic yet,
// so every walkable surface is flat and elevation changes go through jumps.
void World::buildTestMap() {
    boxes_.clear();

    const glm::vec3 floorGrey{0.62f, 0.64f, 0.66f};
    const glm::vec3 wallSlate{0.35f, 0.42f, 0.52f};
    const glm::vec3 crateOrange{0.85f, 0.55f, 0.25f};
    const glm::vec3 stepTeal{0.30f, 0.65f, 0.60f};
    const glm::vec3 tunnelPurple{0.55f, 0.45f, 0.65f};
    const glm::vec3 pillarRed{0.70f, 0.35f, 0.35f};

    // Floor (checkered so speed is easy to read) and perimeter walls.
    add({-20, -1, -20}, {20, 0, 20}, floorGrey, true);
    add({-20, 0, -21}, {20, 4, -20}, wallSlate); // north
    add({-20, 0, 20}, {20, 4, 21}, wallSlate);   // south
    add({-21, 0, -20}, {-20, 4, 20}, wallSlate); // west
    add({20, 0, -20}, {21, 4, 20}, wallSlate);   // east

    // Crate row east of spawn: 0.5 m (easy jump), 1.0 m (needs a clean jump
    // from a crate), 1.5 m (too tall on purpose - a reference obstacle).
    add({4.0f, 0, -2.0f}, {5.5f, 0.5f, -0.5f}, crateOrange, true);
    add({7.0f, 0, -2.0f}, {8.5f, 1.0f, -0.5f}, crateOrange, true);
    add({10.0f, 0, -2.0f}, {11.5f, 1.5f, -0.5f}, crateOrange, true);

    // Jump staircase up to a 2 m platform (0.5 m rises, all jumpable).
    add({-6, 0, 6}, {-4, 0.5f, 8}, stepTeal, true);
    add({-8, 0, 6}, {-6, 1.0f, 8}, stepTeal, true);
    add({-10, 0, 6}, {-8, 1.5f, 8}, stepTeal, true);
    add({-14, 0, 6}, {-10, 2.0f, 10}, stepTeal, true);

    // Crouch tunnel: 2 m wide gap with a roof 1.4 m up - stand height (1.8)
    // does not fit, crouch height (1.3) does.
    add({2, 0, 9.6f}, {8, 2.2f, 10.0f}, tunnelPurple);   // near side wall
    add({2, 0, 12.0f}, {8, 2.2f, 12.4f}, tunnelPurple);  // far side wall
    add({2, 1.4f, 10.0f}, {8, 2.2f, 12.4f}, tunnelPurple); // roof

    // Pillars for strafe practice.
    add({-4.5f, 0, -8.5f}, {-3.5f, 3, -7.5f}, pillarRed);
    add({-7.5f, 0, -8.5f}, {-6.5f, 3, -7.5f}, pillarRed);
    add({-10.5f, 0, -8.5f}, {-9.5f, 3, -7.5f}, pillarRed);

    // A long wall segment mid-map for wall-sliding and counter-strafe drills.
    add({-2, 0, -14}, {14, 2.5f, -13.4f}, wallSlate);

    spawnPoint = {0.0f, 0.01f, 0.0f};
}
