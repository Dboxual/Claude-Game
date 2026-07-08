#include "shared/world_template.h"

namespace worldgen {

namespace {

// --- geometry helpers -----------------------------------------------------
// Small builders so each template reads as a layout, not a wall of vectors.

void box(std::vector<WorldBox>& v, glm::vec3 mn, glm::vec3 mx, glm::vec3 color,
         bool checkerTop = false, float emissive = 0.0f) {
    v.push_back(WorldBox{AABB{mn, mx}, color, checkerTop, emissive});
}

// A full-bright neon/LED accent strip. Thin and flush (or high on a wall) so it
// reads as a glowing light, not gameplay geometry - collision is technically
// present but placed where it never affects movement or shots.
void neon(std::vector<WorldBox>& v, glm::vec3 mn, glm::vec3 mx, glm::vec3 color) {
    v.push_back(WorldBox{AABB{mn, mx}, color, false, 1.0f});
}

// Floor plate plus a 1 m-thick wall ring, half-extents (hx, hz) on the
// ground plane and wallH tall. The classic enclosed sandbox box.
void enclose(std::vector<WorldBox>& v, float hx, float hz, float wallH,
             glm::vec3 floorCol, glm::vec3 wallCol, bool checkerFloor) {
    box(v, {-hx, -1.0f, -hz}, {hx, 0.0f, hz}, floorCol, checkerFloor);
    box(v, {-hx, 0.0f, -hz - 1.0f}, {hx, wallH, -hz}, wallCol);      // north
    box(v, {-hx, 0.0f, hz}, {hx, wallH, hz + 1.0f}, wallCol);        // south
    box(v, {-hx - 1.0f, 0.0f, -hz}, {-hx, wallH, hz}, wallCol);      // west
    box(v, {hx, 0.0f, -hz}, {hx + 1.0f, wallH, hz}, wallCol);        // east
}

// --- the palette (shared so worlds feel like one game) --------------------

const glm::vec3 kFloorGrey{0.62f, 0.64f, 0.66f};
const glm::vec3 kWallSlate{0.35f, 0.42f, 0.52f};
const glm::vec3 kArenaSand{0.58f, 0.53f, 0.44f};
const glm::vec3 kArenaStone{0.44f, 0.45f, 0.50f};
const glm::vec3 kRangeFloor{0.50f, 0.52f, 0.57f};
const glm::vec3 kRangeWall{0.30f, 0.34f, 0.40f};
const glm::vec3 kMarker{0.80f, 0.78f, 0.42f};
const glm::vec3 kObstacle{0.55f, 0.48f, 0.40f};
const glm::vec3 kPillar{0.46f, 0.50f, 0.58f};
const glm::vec3 kPlazaFloor{0.55f, 0.57f, 0.60f};
const glm::vec3 kPlazaWall{0.44f, 0.46f, 0.52f};
const glm::vec3 kPlazaAccent{0.60f, 0.50f, 0.38f};
// MiniCS goes for a dark neon look: a cool charcoal shell so the emissive
// cyan/magenta/amber accents (and the crimson enemies) pop hard against it.
const glm::vec3 kCsFloor{0.17f, 0.18f, 0.23f};   // dark cool concrete
const glm::vec3 kCsWall{0.12f, 0.13f, 0.18f};    // near-black slate
const glm::vec3 kCsCover{0.20f, 0.21f, 0.27f};   // crates / low walls
const glm::vec3 kCsRamp{0.16f, 0.19f, 0.26f};
const glm::vec3 kNeonCyan{0.15f, 0.95f, 1.0f};
const glm::vec3 kNeonMagenta{1.0f, 0.20f, 0.75f};
const glm::vec3 kNeonAmber{1.0f, 0.62f, 0.12f};
const glm::vec3 kNeonLime{0.55f, 1.0f, 0.30f};
const glm::vec3 kPitFloor{0.30f, 0.33f, 0.30f};  // grim survival pit
const glm::vec3 kPitWall{0.22f, 0.24f, 0.24f};
const glm::vec3 kPitAccent{0.36f, 0.30f, 0.26f};

// --- the templates --------------------------------------------------------

WorldTemplate flatSandbox() {
    WorldTemplate t;
    t.id = "flat_sandbox";
    t.displayName = "Flat Sandbox";
    t.description = "Empty enclosed flat map. Good for spawning and testing.";
    enclose(t.boxes, 20.0f, 20.0f, 4.0f, kFloorGrey, kWallSlate, true);
    t.spawnPoint = {0.0f, 0.01f, 0.0f};
    return t;
}

WorldTemplate duelYard() {
    WorldTemplate t;
    t.id = "duel_yard";
    t.displayName = "Duel Yard";
    t.description = "Small arena with sword, shield and a duelist bot. For melee.";
    enclose(t.boxes, 12.0f, 12.0f, 4.0f, kArenaSand, kArenaStone, false);
    // A low stone lip framing the fighting circle (cosmetic, jumpable).
    box(t.boxes, {-11.0f, 0.0f, -11.0f}, {-10.0f, 0.3f, 11.0f}, kArenaStone);
    box(t.boxes, {10.0f, 0.0f, -11.0f}, {11.0f, 0.3f, 11.0f}, kArenaStone);
    t.spawnPoint = {0.0f, 0.01f, 8.0f};
    // Weapons within reach of spawn, the sparring partners across the yard.
    t.placements.push_back({"sword", {-1.6f, 0.0f, 5.0f}, -1.0f});
    t.placements.push_back({"shield", {1.6f, 0.0f, 5.0f}, -1.0f});
    t.placements.push_back({"duel_bot", {0.0f, 0.0f, -4.0f}, 12.0f});
    t.placements.push_back({"training_dummy", {-5.5f, 0.0f, -6.0f}, 10.0f});
    return t;
}

WorldTemplate aimRange() {
    WorldTemplate t;
    t.id = "aim_range";
    t.displayName = "Aim Range";
    t.description = "Long lane with a Glock and dummies at distance.";
    // 12 m wide, 60 m long lane; shooter fires north (-z) from the south end.
    enclose(t.boxes, 6.0f, 30.0f, 4.0f, kRangeFloor, kRangeWall, false);
    // Distance marker stripes down the lane (thin plates every ~10 m).
    for (float z = 15.0f; z >= -25.0f; z -= 10.0f) {
        box(t.boxes, {-6.0f, 0.0f, z - 0.1f}, {6.0f, 0.02f, z + 0.1f}, kMarker);
    }
    t.spawnPoint = {0.0f, 0.01f, 27.0f};
    // Glock at the firing line; dummies stepping away down the lane.
    t.placements.push_back({"glock", {0.0f, 0.0f, 24.0f}, -1.0f});
    t.placements.push_back({"training_dummy", {-2.5f, 0.0f, 15.0f}, 6.0f});
    t.placements.push_back({"training_dummy", {2.5f, 0.0f, 5.0f}, 6.0f});
    t.placements.push_back({"training_dummy", {-1.5f, 0.0f, -6.0f}, 6.0f});
    t.placements.push_back({"training_dummy", {2.0f, 0.0f, -17.0f}, 6.0f});
    t.placements.push_back({"training_dummy", {0.0f, 0.0f, -26.0f}, 6.0f});
    return t;
}

WorldTemplate movementCourse() {
    WorldTemplate t;
    t.id = "movement_course";
    t.displayName = "Movement Course";
    t.description = "Jumps, a crouch tunnel and strafe pillars. Geometry only.";
    enclose(t.boxes, 10.0f, 30.0f, 5.0f, kFloorGrey, kWallSlate, true);

    // Jump staircase near spawn: 0.4 m steps (each jumpable, no step-up yet).
    box(t.boxes, {-3.0f, 0.0f, 17.0f}, {3.0f, 0.4f, 20.0f}, kObstacle, true);
    box(t.boxes, {-3.0f, 0.0f, 14.0f}, {3.0f, 0.8f, 17.0f}, kObstacle, true);
    box(t.boxes, {-3.0f, 0.0f, 11.0f}, {3.0f, 1.2f, 14.0f}, kObstacle, true);
    box(t.boxes, {-3.0f, 0.0f, 8.0f}, {3.0f, 1.6f, 11.0f}, kObstacle, true);

    // Crouch tunnel (z 4..-2): side walls with a low roof - clearance 1.1 m,
    // so you must crouch to pass along the x-centered corridor.
    box(t.boxes, {-4.0f, 0.0f, -2.0f}, {-2.0f, 2.5f, 4.0f}, kPillar);
    box(t.boxes, {2.0f, 0.0f, -2.0f}, {4.0f, 2.5f, 4.0f}, kPillar);
    box(t.boxes, {-2.0f, 1.1f, -2.0f}, {2.0f, 2.5f, 4.0f}, kPillar);

    // Strafe pillars (z -7..-16): a slalom to weave through.
    box(t.boxes, {-1.0f, 0.0f, -8.0f}, {1.0f, 3.0f, -7.0f}, kPillar);
    box(t.boxes, {-4.5f, 0.0f, -12.0f}, {-2.5f, 3.0f, -11.0f}, kPillar);
    box(t.boxes, {2.5f, 0.0f, -12.0f}, {4.5f, 3.0f, -11.0f}, kPillar);
    box(t.boxes, {-1.0f, 0.0f, -16.0f}, {1.0f, 3.0f, -15.0f}, kPillar);

    // Obstacle path (z -19..-26): low blocks to hop over.
    box(t.boxes, {-6.0f, 0.0f, -20.0f}, {-3.0f, 0.6f, -19.0f}, kObstacle, true);
    box(t.boxes, {3.0f, 0.0f, -22.0f}, {6.0f, 0.9f, -21.0f}, kObstacle, true);
    box(t.boxes, {-2.0f, 0.0f, -25.0f}, {2.0f, 0.5f, -24.0f}, kObstacle, true);

    t.spawnPoint = {0.0f, 0.01f, 27.0f};
    return t;
}

WorldTemplate socialHub() {
    WorldTemplate t;
    t.id = "social_hub";
    t.displayName = "Social Hub";
    t.description = "Open plaza with a few props. A local hangout world type.";
    enclose(t.boxes, 20.0f, 20.0f, 5.0f, kPlazaFloor, kPlazaWall, false);
    // Central raised dais and a couple of low benches to break up the space.
    box(t.boxes, {-3.0f, 0.0f, -3.0f}, {3.0f, 0.5f, 3.0f}, kPlazaAccent, true);
    box(t.boxes, {-9.0f, 0.0f, 6.0f}, {-6.0f, 0.45f, 7.0f}, kPlazaAccent);
    box(t.boxes, {6.0f, 0.0f, -7.0f}, {9.0f, 0.45f, -6.0f}, kPlazaAccent);
    t.spawnPoint = {0.0f, 0.01f, 12.0f};
    // A few carryable props so the plaza is not bare.
    t.placements.push_back({"crate", {6.0f, 0.0f, 3.0f}, 0.0f});
    t.placements.push_back({"crate", {-6.5f, 0.0f, -2.0f}, 0.0f});
    t.placements.push_back({"barrel", {5.0f, 0.0f, 8.0f}, 0.0f});
    t.placements.push_back({"barrel", {-8.0f, 0.0f, -5.0f}, 0.0f});
    return t;
}

WorldTemplate minicsArena() {
    WorldTemplate t;
    t.id = "minics_arena";
    t.displayName = "MiniCS Arena";
    t.description = "Compact neon FPS arena: charcoal shell, LED lanes, crate cover.";
    // 36 x 44 walled arena; players push from the south, bots hold the north.
    enclose(t.boxes, 18.0f, 22.0f, 5.0f, kCsFloor, kCsWall, false);

    // Central mid structure: a raised platform with a ramp, splitting sightlines.
    box(t.boxes, {-4.0f, 0.0f, -3.0f}, {4.0f, 1.2f, 3.0f}, kCsRamp, true);
    box(t.boxes, {-4.0f, 0.0f, 3.0f}, {4.0f, 0.4f, 5.0f}, kCsRamp, true);   // ramp lip up
    box(t.boxes, {-4.0f, 0.0f, 5.0f}, {4.0f, 0.8f, 6.5f}, kCsRamp, true);
    // Amber LED rim glowing along the top edge of the mid platform.
    neon(t.boxes, {-4.05f, 1.16f, -3.05f}, {4.05f, 1.24f, -2.9f}, kNeonAmber);
    neon(t.boxes, {-4.05f, 1.16f, 2.9f}, {4.05f, 1.24f, 3.05f}, kNeonAmber);
    neon(t.boxes, {-4.05f, 1.16f, -3.0f}, {-3.9f, 1.24f, 3.0f}, kNeonAmber);
    neon(t.boxes, {3.9f, 1.16f, -3.0f}, {4.05f, 1.24f, 3.0f}, kNeonAmber);

    // Crate cover clusters (jumpable, breakable sightlines) around the arena.
    box(t.boxes, {-11.0f, 0.0f, 8.0f}, {-9.0f, 1.2f, 10.0f}, kCsCover, true);
    box(t.boxes, {-11.0f, 0.0f, 10.0f}, {-9.5f, 2.0f, 11.5f}, kCsCover, true);
    box(t.boxes, {9.0f, 0.0f, 6.0f}, {11.0f, 1.2f, 8.0f}, kCsCover, true);
    box(t.boxes, {8.0f, 0.0f, -9.0f}, {10.0f, 1.4f, -7.0f}, kCsCover, true);
    box(t.boxes, {-10.5f, 0.0f, -8.0f}, {-8.5f, 1.4f, -6.0f}, kCsCover, true);
    box(t.boxes, {-2.0f, 0.0f, -14.0f}, {2.0f, 1.6f, -12.5f}, kCsCover, true); // north choke
    box(t.boxes, {13.0f, 0.0f, -2.0f}, {14.5f, 2.2f, 4.0f}, kCsWall);          // side wall bit
    box(t.boxes, {-14.5f, 0.0f, -2.0f}, {-13.0f, 2.2f, 4.0f}, kCsWall);

    // Wall-top LED strips near the top of each wall (high up, purely visual):
    // cyan down the long sides, magenta across the ends. This is what gives the
    // arena its neon identity and a bright horizon line through the fog.
    const float wy0 = 4.3f, wy1 = 4.6f;
    neon(t.boxes, {-18.0f, wy0, -22.02f}, {18.0f, wy1, -21.98f}, kNeonMagenta); // north
    neon(t.boxes, {-18.0f, wy0, 21.98f}, {18.0f, wy1, 22.02f}, kNeonMagenta);   // south
    neon(t.boxes, {-18.02f, wy0, -22.0f}, {-17.98f, wy1, 22.0f}, kNeonCyan);    // west
    neon(t.boxes, {17.98f, wy0, -22.0f}, {18.02f, wy1, 22.0f}, kNeonCyan);      // east

    // Floor LED lane lines: a cyan centre lane guiding the push north, plus two
    // magenta flank lines. Flush to the floor (2 cm), so no collision matters.
    neon(t.boxes, {-0.15f, 0.0f, -12.0f}, {0.15f, 0.02f, 14.0f}, kNeonCyan);    // centre
    neon(t.boxes, {-9.15f, 0.0f, -10.0f}, {-8.85f, 0.02f, 12.0f}, kNeonMagenta); // west flank
    neon(t.boxes, {8.85f, 0.0f, -10.0f}, {9.15f, 0.02f, 12.0f}, kNeonMagenta);  // east flank
    // A lime start line at the south spawn.
    neon(t.boxes, {-6.0f, 0.0f, 16.9f}, {6.0f, 0.02f, 17.1f}, kNeonLime);

    t.spawnPoint = {0.0f, 0.01f, 18.0f}; // south spawn, looking into the arena

    // Neon pillar landmarks (also light cover) framing the mid lanes.
    t.placements.push_back({"neon_pillar", {-6.5f, 0.0f, 0.0f}, 0.0f});
    t.placements.push_back({"neon_pillar", {6.5f, 0.0f, 0.0f}, 0.0f});
    t.placements.push_back({"neon_pillar", {0.0f, 0.0f, -18.5f}, 0.0f});
    t.placements.push_back({"neon_pillar", {-13.8f, 0.0f, 9.5f}, 0.0f});
    t.placements.push_back({"neon_pillar", {13.8f, 0.0f, 9.5f}, 0.0f});

    // Loadout support: a spare pistol at spawn; the ranged opposition holds the
    // north. The MiniCS round loop (client) replaces these with minics_bot
    // enemies + a spare Glock (Game::placeMinicsEntities); this list is what
    // Deathmatch (which shares this map) uses.
    t.placements.push_back({"glock", {2.0f, 0.0f, 16.5f}, -1.0f});
    t.placements.push_back({"duel_bot", {-6.0f, 0.0f, -12.0f}, 12.0f});
    t.placements.push_back({"duel_bot", {6.0f, 0.0f, -13.0f}, 12.0f});
    t.placements.push_back({"training_dummy", {0.0f, 1.2f, -1.0f}, 6.0f});
    t.placements.push_back({"training_dummy", {-10.0f, 0.0f, -12.0f}, 6.0f});
    t.placements.push_back({"training_dummy", {10.0f, 0.0f, -10.0f}, 6.0f});
    return t;
}

WorldTemplate zombiePit() {
    WorldTemplate t;
    t.id = "zombie_pit";
    t.displayName = "Zombie Pit";
    t.description = "Walled survival pit seeded with a horde of hostiles.";
    enclose(t.boxes, 14.0f, 14.0f, 6.0f, kPitFloor, kPitWall, false);
    // A couple of low barricades to break line of sight and kite around.
    box(t.boxes, {-6.0f, 0.0f, -2.0f}, {-4.0f, 1.3f, 2.0f}, kPitAccent, true);
    box(t.boxes, {4.0f, 0.0f, -3.0f}, {6.0f, 1.3f, 1.0f}, kPitAccent, true);
    box(t.boxes, {-1.5f, 0.0f, 6.0f}, {1.5f, 1.0f, 7.5f}, kPitAccent, true);
    t.spawnPoint = {0.0f, 0.01f, 11.0f};
    // The horde: goblins stand in for zombies until a real mob exists. They
    // respawn fast so the pit stays dangerous.
    t.placements.push_back({"glock", {1.5f, 0.0f, 10.0f}, -1.0f});
    t.placements.push_back({"goblin", {-4.0f, 0.0f, -8.0f}, 8.0f});
    t.placements.push_back({"goblin", {0.0f, 0.0f, -10.0f}, 8.0f});
    t.placements.push_back({"goblin", {4.0f, 0.0f, -8.0f}, 8.0f});
    t.placements.push_back({"goblin", {-7.0f, 0.0f, -3.0f}, 10.0f});
    t.placements.push_back({"goblin", {7.0f, 0.0f, -3.0f}, 10.0f});
    return t;
}

const std::vector<WorldTemplate>& registry() {
    static const std::vector<WorldTemplate> kTemplates = {
        flatSandbox(), minicsArena(), duelYard(), aimRange(),
        movementCourse(), socialHub(), zombiePit(),
    };
    return kTemplates;
}

} // namespace

const std::vector<WorldTemplate>& templates() { return registry(); }

const WorldTemplate* findTemplate(const std::string& id) {
    for (const WorldTemplate& t : registry()) {
        if (t.id == id) return &t;
    }
    return nullptr;
}

const WorldTemplate& fallbackTemplate() {
    // flat_sandbox is guaranteed to be registry()[0].
    return registry().front();
}

} // namespace worldgen
