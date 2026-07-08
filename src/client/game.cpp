#include "client/game.h"

#include "shared/content_loader.h"
#include "shared/log.h"
#include "shared/protocol.h"
#include "shared/world_save.h"
#include "shared/world_template.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {

// Menu metrics (pixels; the layout centers on the viewport).
constexpr float kBtnW = 300.0f;
constexpr float kBtnH = 48.0f;
constexpr float kBtnGap = 62.0f;
constexpr float kModeCardW = 540.0f; // Game Modes: wide "card" buttons
constexpr float kModeCardH = 60.0f;
constexpr float kRowH = 48.0f;
constexpr float kStepBtn = 36.0f; // size of the square -/+ buttons
constexpr float kAddressBoxW = 460.0f;
constexpr float kAddressBoxH = 44.0f;
constexpr size_t kAddressMaxLen = 32;

// Sandbox limits (Phase 1).
constexpr size_t kWorldNameMaxLen = 24;
constexpr int kMaxWorldButtons = 7;      // load screen shows at most this many
constexpr size_t kMaxEntities = 256;     // keeps save files and draw lists sane
constexpr float kSpawnDistance = 2.5f;   // meters in front of the player

// Interaction (Phase 2).
constexpr float kInteractRange = 2.5f; // max distance for E-pickup
constexpr float kAimInfoRange = 12.0f; // max distance for the target readout

// RPG foundation (M4).
constexpr float kMobAggroRange = 9.0f;  // hostile mobs notice the player
constexpr float kMobStrikeRange = 1.7f; // ...and swing inside this
constexpr float kMobSpeed = 2.3f;       // m/s chase speed (player runs 6)
constexpr float kMobStrikeCooldown = 1.2f;
constexpr float kThirdPersonDist = 3.4f; // camera boom length behind the eye

// Viewmodel animation.
constexpr float kEquipDuration = 0.25f; // weapon-raise time after switching

const char* attackName(melee::Attack a) {
    switch (a) {
    case melee::Attack::SlashLeft: return "SLASH LEFT";
    case melee::Attack::SlashRight: return "SLASH RIGHT";
    case melee::Attack::Overhead: return "OVERHEAD";
    case melee::Attack::Stab: return "STAB";
    case melee::Attack::Kick: return "KICK";
    case melee::Attack::Jab: return "JAB";
    }
    return "?";
}

// Composes translate * yaw * pitch * roll (degrees) for viewmodel groups
// and boxes. Camera space: +X right, +Y up, -Z forward.
glm::mat4 vmCompose(const glm::mat4& parent, glm::vec3 pos, glm::vec3 rotDeg) {
    glm::mat4 m = glm::translate(parent, pos);
    if (rotDeg.y != 0.0f) m = glm::rotate(m, glm::radians(rotDeg.y), glm::vec3(0, 1, 0));
    if (rotDeg.x != 0.0f) m = glm::rotate(m, glm::radians(rotDeg.x), glm::vec3(1, 0, 0));
    if (rotDeg.z != 0.0f) m = glm::rotate(m, glm::radians(rotDeg.z), glm::vec3(0, 0, 1));
    return m;
}

bool contains(float px, float py, float x, float y, float w, float h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}

std::string toUpperAscii(std::string s) {
    for (char& c : s) {
        if (c >= 'a' && c <= 'z') c = char(c - 'a' + 'A');
    }
    return s;
}

// Display name -> folder name: lowercase, spaces to underscores, everything
// but [a-z0-9_-] dropped. Empty result means the name was unusable.
std::string sanitizeWorldFolder(const std::string& name) {
    std::string out;
    for (char c : name) {
        if (c >= 'A' && c <= 'Z') out += char(c - 'A' + 'a');
        else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_') out += c;
        else if (c == ' ' && !out.empty() && out.back() != '_') out += '_';
    }
    while (!out.empty() && out.back() == '_') out.pop_back();
    if (out.size() > kWorldNameMaxLen) out.resize(kWorldNameMaxLen);
    return out;
}

} // namespace

bool Game::init(IFileSystem& fs, IAudio* audio) {
    fs_ = &fs;
    audio_ = audio;
    content_.registerBuiltins();
    items_.registerBuiltins();
    recipes_.registerBuiltins();

    const std::string relative = "config/movement.cfg";
    if (fs.exists(relative)) cfgPath_ = relative;
    else if (fs.exists(fs.basePath() + relative)) cfgPath_ = fs.basePath() + relative;

    // Worlds and the server content platform live next to wherever the
    // config directory was found: the repo root when launched from there,
    // otherwise beside the executable.
    worldsRoot_ = "worlds/";
    serverRoot_ = "server/";
    if (!cfgPath_.empty() && cfgPath_ != relative) {
        worldsRoot_ = fs.basePath() + "worlds/";
        serverRoot_ = fs.basePath() + "server/";
    }

    // Data-driven content: defs from server/content/ replace the builtins.
    loadContentFiles();

    // Placeholder one-shots (original, synthesized by tools/generate_sounds -
    // see content/sounds/CREDITS.md). The event -> sound-name mapping is
    // client code for now by design.
    if (audio_) {
        const char* names[] = {"footstep",  "swing",       "gunshot",   "pickup",
                               "dummy_hit", "melee_hit",   "ui_click",  "block",
                               "parry",     "shield_block", "kick_hit",
                               "guard_break", "throw",     "windup_cue",
                               // MiniCS: pistol handling + round/kill feedback.
                               "reload",    "dry_fire",    "kill_confirm",
                               "enemy_shot", "round_win",  "round_lose"};
        for (const char* n : names) {
            audio_->loadSound(n, serverRoot_ + "content/sounds/" + n + ".wav");
        }
    }

    // Settings live next to the movement config; created on first save.
    if (!cfgPath_.empty()) {
        settingsPath_ = cfgPath_.substr(0, cfgPath_.size() - std::string("movement.cfg").size())
                        + "settings.cfg";
    } else {
        settingsPath_ = "config/settings.cfg";
    }

    loadMovementConfig();
    bool hadSettingsFile = loadSettings();
    if (!hadSettingsFile) {
        // First run: write the defaults so the file exists for hand-editing.
        saveSettings();
        eng::logInfo("Created default settings file: %s", settingsPath_.c_str());
    }
    tickDt_ = 1.0 / std::max(30.0f, cfg_.tickRate); // startup only; F5 keeps the tick

    // Menu backdrop: orbit the neon MiniCS arena so the hub reads as a real
    // game the instant it opens (this world is replaced the moment any mode or
    // sandbox world is entered).
    if (const WorldTemplate* menuMap = worldgen::findTemplate("minics_arena")) {
        world_.buildFromTemplate(*menuMap);
        for (const WorldTemplate::Placement& p : menuMap->placements) {
            if (const EntityDef* def = content_.find(p.defId)) {
                world_.addEntity(*def, p.pos, p.respawnSeconds);
            }
        }
        currentWorldType_ = "minics_arena";
    } else {
        world_.buildTestMap();
    }
    player_.respawn(world_.spawnPoint);
    prevPos_ = player_.pos;
    eyeHeight_ = cfg_.standHeight - cfg_.eyeOffset;
    return true;
}

void Game::loadMovementConfig() {
    if (cfgPath_.empty()) {
        eng::logInfo("config/movement.cfg not found - using built-in defaults");
        return;
    }
    if (auto text = fs_->readTextFile(cfgPath_)) {
        cfgFile_.parseText(*text);
        cfg_.applyFrom(cfgFile_);
        eng::logInfo("Loaded movement config: %s", cfgPath_.c_str());
    } else {
        eng::logError("Failed to read movement config: %s", cfgPath_.c_str());
    }
}

bool Game::loadSettings() {
    bool existed = false;
    if (auto text = fs_->readTextFile(settingsPath_)) {
        ConfigFile parsed;
        parsed.parseText(*text);
        settings_.applyFrom(parsed);
        eng::logInfo("Loaded settings: %s", settingsPath_.c_str());
        existed = true;
    }
    settingsDirty_ = true; // composition root applies fullscreen/vsync/volumes
    return existed;
}

void Game::saveSettings() {
    if (!fs_->writeTextFile(settingsPath_, settings_.serialize())) {
        eng::logError("Failed to save settings to %s", settingsPath_.c_str());
    }
}

void Game::beginSession() {
    player_.respawn(world_.spawnPoint);
    prevPos_ = player_.pos;
    yaw_ = 0.0f;
    pitch_ = 0.0f;
    eyeHeight_ = cfg_.standHeight - cfg_.eyeOffset;
    accumulator_ = 0.0;
    jumpBufferTimer_ = -1.0;
    inventory_.reset("fists");
    itemInv_.reset();
    mobs_.clear();
    attackCooldown_ = 0.0f;
    muzzleFlashTimer_ = hitMarkerTimer_ = 0.0f;
    footstepDistance_ = 0.0f;
    bobPhase_ = bobIntensity_ = equipTimer_ = 0.0f;
    lastEquippedId_.clear(); // triggers the raise animation on the first frame
    playerMelee_ = melee::Actor{};
    prevMeleePhase_ = melee::Phase::Idle;
    hasShield_ = false;
    nextSlashLeft_ = false;
    heavyEligible_ = false;
    thrown_.clear();
    sparks_.clear();
    tracers_.clear();
    bots_.clear();
    hitStop_ = shakeTime_ = shakeAmp_ = damageFlash_ = 0.0f;
    playerHealth_ = playerHealthShown_ = 100.0f;
    sinceDamaged_ = 999.0f;
    // MiniCS + gun feel reset (a fresh session is never mid-round or mid-reload).
    minicsActive_ = false;
    minicsBrains_.clear();
    scorePopups_.clear();
    minicsScore_ = minicsKills_ = 0;
    minicsEnemiesTotal_ = minicsEnemiesAlive_ = 0;
    killMarkerTimer_ = hurtTimer_ = 0.0f;
    magAmmo_ = reserveAmmo_ = 0;
    reloadTimer_ = recoilPitch_ = recoilYaw_ = 0.0f;
    swayX_ = swayY_ = landDipTimer_ = landDipAmount_ = 0.0f;
    wasGrounded_ = true;
    controlHintTimer_ = 8.0; // a few seconds of "here's how to play"
    carriedEntityId_ = 0;
    hudToast_.clear();
    hudToastTimer_ = 0.0;
    aimInteract_ = {};
    aimInfo_ = {};
    inGame_ = true;
    ui_ = UiScreen::None;
    menuStatus_.clear();
}

void Game::startTestWorld() {
    world_.buildTestMap();

    // Furnish the glade: the whole RPG loop should be walkable without the
    // spawn menu - gather wood/stone at spawn, craft tools, mine the east
    // shelf, fight the goblin camp up north. respawn -1 keeps each def's
    // default (nodes renew, drops don't); this world is never saved.
    auto place = [&](const char* id, glm::vec3 pos, float respawnSeconds = -1.0f) {
        if (const EntityDef* def = content_.find(id)) {
            world_.addEntity(*def, pos, respawnSeconds);
        }
    };

    // Tree line around the glade edges.
    place("tree", {-16.0f, 0.0f, -4.0f});
    place("tree", {-14.0f, 0.0f, 6.0f});
    place("tree", {-11.0f, 0.0f, 12.5f});
    place("tree", {6.0f, 0.0f, -16.0f});
    place("tree", {10.0f, 0.0f, 15.0f});
    place("tree", {14.0f, 0.0f, 9.5f});
    place("tree", {16.5f, 0.0f, 2.0f});
    place("tree", {17.5f, 0.0f, 16.5f});

    // Mining corner: rocks at the foot of the stone shelf, ore on top of it
    // (one jump up) plus one vein at ground level for the first pickaxe.
    place("rock", {11.0f, 0.0f, -11.0f});
    place("rock", {12.0f, 0.0f, -5.0f});
    place("rock", {17.5f, 0.0f, -5.5f});
    place("ore_node", {14.5f, 0.0f, -6.5f});
    place("ore_node", {16.0f, 1.0f, -10.5f});
    place("ore_node", {18.3f, 1.0f, -11.5f});

    // Goblin camp on the north mound: two goblins guarding their stash.
    place("goblin", {-5.5f, 0.35f, -14.5f}, 25.0f);
    place("goblin", {-7.5f, 0.35f, -13.0f}, 25.0f);
    place("crate", {-3.6f, 0.35f, -16.0f}, 0.0f);
    place("barrel", {-7.0f, 0.35f, -16.2f}, 0.0f);

    // Village green: a practice dummy and some props to carry around.
    place("training_dummy", {-4.2f, 0.02f, 3.2f}, 10.0f);
    place("crate", {-5.9f, 0.02f, 4.8f}, 0.0f);
    place("crate", {-2.6f, 0.02f, 5.4f}, 0.0f);
    place("barrel", {2.4f, 0.0f, 4.6f}, 0.0f);

    // Loose starter resources by the spawn so E-pickup teaches itself.
    place("drop_wood", {1.2f, 0.0f, 6.5f}, 0.0f);
    place("drop_wood", {-0.8f, 0.0f, 5.6f}, 0.0f);
    place("drop_stone", {0.6f, 0.0f, 4.4f}, 0.0f);

    currentWorldFile_.clear(); // the test glade is never saved
    currentWorldName_ = "SANDBOX";
    currentWorldType_ = gamemodes::kTestGladeMap;
    currentModeName_ = "SANDBOX";
    activeModeStatus_ = ModeStatus::Playable;
    sandboxMode_ = true; // the glade is the Sandbox mode: build tools belong here
    lastModeId_ = "sandbox";
    beginSession();
}

// Launch a game mode: build its map (a world template, or the glade), place
// its starting entities, then apply the loadout on top of the fresh session.
// Preview modes still drop you into a real world; they just say honestly what
// part of the ruleset is not built yet.
void Game::startGameMode(const GameMode& mode) {
    lastModeId_ = mode.id;
    const bool isMinics = (mode.id == "minics");

    if (mode.mapTemplateId == gamemodes::kTestGladeMap) {
        startTestWorld(); // builds the glade + beginSession (+ sandbox fields)
    } else {
        const WorldTemplate* tmpl = worldgen::findTemplate(mode.mapTemplateId);
        if (!tmpl) {
            eng::logError("Game mode '%s': unknown map '%s', using fallback",
                          mode.id.c_str(), mode.mapTemplateId.c_str());
            tmpl = &worldgen::fallbackTemplate();
        }
        world_.buildFromTemplate(*tmpl);
        if (isMinics) {
            // MiniCS owns its opposition: skip the template's melee duel-bots
            // and dummies and place the ranged round-loop enemies instead.
            placeMinicsEntities();
        } else {
            for (const WorldTemplate::Placement& p : tmpl->placements) {
                if (const EntityDef* def = content_.find(p.defId)) {
                    world_.addEntity(*def, p.pos, p.respawnSeconds);
                }
            }
        }
        currentWorldFile_.clear(); // mode sessions are not saved as maps
        currentWorldName_ = toUpperAscii(mode.displayName);
        currentWorldType_ = tmpl->id;
        sandboxMode_ = false;
        beginSession();
    }

    // Loadout goes on after beginSession() (which resets to fists).
    if (!mode.loadoutWeapon.empty() && content_.findWeapon(mode.loadoutWeapon)) {
        inventory_.acquireAndEquip(mode.loadoutWeapon);
    }

    currentModeName_ = toUpperAscii(mode.displayName);
    activeModeStatus_ = mode.status;
    if (isMinics) startMinicsRound(); // build enemy brains + start the countdown
    eng::logInfo("Started game mode '%s' on map '%s'", mode.id.c_str(),
                 mode.mapTemplateId.c_str());
    if (mode.status == ModeStatus::Preview && !mode.previewNote.empty()) {
        showToast(mode.previewNote); // honest "what's not done yet"
    }
}

// Pop one level up the UI tree. Shared by ESC and every Back button so there
// is a single definition of "where does back go from here".
void Game::goBack() {
    switch (ui_) {
    case UiScreen::None: // gameplay -> pause
        ui_ = UiScreen::PauseMain;
        jumpBufferTimer_ = -1.0; // stale presses should not fire on resume
        break;
    case UiScreen::PauseMain: // pause -> resume
        ui_ = UiScreen::None;
        break;
    case UiScreen::Settings: // back to wherever settings was opened from
        saveSettings();
        ui_ = settingsReturn_;
        break;
    case UiScreen::Play:
    case UiScreen::Create:
    case UiScreen::Profile: // top-level submenu -> main menu
        menuStatus_.clear();
        ui_ = UiScreen::MainMenu;
        break;
    case UiScreen::GameModes:
    case UiScreen::Servers:
    case UiScreen::RecentGames: // -> Play
        menuStatus_.clear();
        ui_ = UiScreen::Play;
        break;
    case UiScreen::ServerList:
    case UiScreen::DirectConnect: // -> Servers
        menuStatus_.clear();
        ui_ = UiScreen::Servers;
        break;
    case UiScreen::CreateWorld:
    case UiScreen::LoadWorld:
    case UiScreen::Skins:
    case UiScreen::Publish: // -> Create
        menuStatus_.clear();
        ui_ = UiScreen::Create;
        break;
    case UiScreen::SpawnMenu: // close, back to gameplay
    case UiScreen::Inventory:
        menuStatus_.clear();
        ui_ = UiScreen::None;
        break;
    case UiScreen::MainMenu:
        break;
    }
}

void Game::debugOpenScreen(const std::string& name) {
    // Dev/screenshot helper: jump onto a menu screen so an automated run can
    // capture it (there is no click-through path in --frames mode).
    struct Entry { const char* key; UiScreen screen; };
    static const Entry kScreens[] = {
        {"play", UiScreen::Play},          {"gamemodes", UiScreen::GameModes},
        {"servers", UiScreen::Servers},    {"serverlist", UiScreen::ServerList},
        {"directconnect", UiScreen::DirectConnect},
        {"recent", UiScreen::RecentGames}, {"create", UiScreen::Create},
        {"createmap", UiScreen::CreateWorld}, {"editmap", UiScreen::LoadWorld},
        {"skins", UiScreen::Skins},        {"publish", UiScreen::Publish},
        {"profile", UiScreen::Profile},    {"settings", UiScreen::Settings},
    };
    for (const Entry& e : kScreens) {
        if (name == e.key) {
            if (e.screen == UiScreen::CreateWorld) worldNameInput_.clear();
            if (e.screen == UiScreen::LoadWorld) refreshWorldList();
            ui_ = e.screen;
            return;
        }
    }
    eng::logError("--menu: unknown screen '%s'", name.c_str());
}

void Game::createWorld() {
    std::string folder = sanitizeWorldFolder(worldNameInput_);
    if (folder.empty()) {
        menuStatus_ = "ENTER A WORLD NAME (LETTERS, NUMBERS, SPACES)";
        return;
    }
    std::string dir = worldsRoot_ + folder;
    std::string file = dir + "/world.cfg";
    if (fs_->exists(file)) {
        menuStatus_ = "A WORLD WITH THAT NAME ALREADY EXISTS";
        return;
    }
    if (!fs_->createDirectory(dir)) {
        eng::logError("Failed to create world directory: %s", dir.c_str());
        menuStatus_ = "COULD NOT CREATE THE WORLD FOLDER";
        return;
    }

    // Build the chosen world type: its geometry, then its starting entities
    // (resolved through the content registry, like a loaded world's are).
    const std::vector<WorldTemplate>& tmpls = worldgen::templates();
    const WorldTemplate& tmpl =
        (createTemplateIndex_ >= 0 && createTemplateIndex_ < int(tmpls.size()))
            ? tmpls[size_t(createTemplateIndex_)]
            : worldgen::fallbackTemplate();
    world_.buildFromTemplate(tmpl);
    for (const WorldTemplate::Placement& p : tmpl.placements) {
        if (const EntityDef* def = content_.find(p.defId)) {
            world_.addEntity(*def, p.pos, p.respawnSeconds);
        }
    }

    currentWorldName_ = worldNameInput_;
    currentWorldType_ = tmpl.id;
    currentWorldFile_ = file;
    saveCurrentWorld();
    eng::logInfo("Created world '%s' (%s) at %s", currentWorldName_.c_str(),
                 tmpl.id.c_str(), file.c_str());
    beginSession();
}

void Game::loadWorld(int listIndex) {
    if (listIndex < 0 || listIndex >= int(worldList_.size())) return;
    const WorldListEntry& entry = worldList_[size_t(listIndex)];
    std::string file = worldsRoot_ + entry.folder + "/world.cfg";

    auto text = fs_->readTextFile(file);
    if (!text) {
        menuStatus_ = "FAILED TO READ " + toUpperAscii(entry.folder);
        return;
    }
    WorldSaveData data;
    if (!parseWorldSave(*text, data)) {
        menuStatus_ = "WORLD FILE IS INVALID: " + toUpperAscii(entry.folder);
        return;
    }

    // Geometry from the saved world type; entities from the save file. An
    // empty or unknown type falls back to flat_sandbox so old and
    // newer-than-us saves both still open.
    const WorldTemplate* tmpl = worldgen::findTemplate(data.worldType);
    if (!tmpl) {
        if (!data.worldType.empty()) {
            eng::logError("World '%s': unknown world_type '%s', using flat_sandbox",
                          entry.folder.c_str(), data.worldType.c_str());
        }
        tmpl = &worldgen::fallbackTemplate();
    }
    world_.buildFromTemplate(*tmpl);
    currentWorldType_ = tmpl->id;

    int skipped = 0;
    for (const WorldSaveData::Spawn& s : data.entities) {
        if (const EntityDef* def = content_.find(s.defId)) {
            world_.addEntity(*def, s.pos, s.respawnSeconds);
        } else {
            ++skipped; // content from a newer build/pack: keep the file intact
        }
    }
    if (skipped > 0) {
        eng::logError("World '%s': %d entities reference unknown content, skipped",
                      entry.folder.c_str(), skipped);
    }

    currentWorldName_ = data.displayName.empty() ? entry.folder : data.displayName;
    currentWorldFile_ = file;
    eng::logInfo("Loaded world '%s' (%s, %d entities)", currentWorldName_.c_str(),
                 tmpl->id.c_str(), int(world_.entities().size()));
    beginSession();
}

void Game::saveCurrentWorld() {
    if (currentWorldFile_.empty()) return; // test arena
    WorldSaveData data;
    data.displayName = currentWorldName_;
    data.worldType = currentWorldType_;
    data.entities.reserve(world_.entities().size());
    for (const WorldEntity& e : world_.entities()) {
        // Inactive entities (picked up / destroyed, awaiting respawn) keep
        // their spawn point in the file; consumed-for-good ones are already
        // erased from the vector.
        data.entities.push_back({e.defId, e.pos, e.respawnSeconds});
    }
    if (!fs_->writeTextFile(currentWorldFile_, serializeWorldSave(data))) {
        eng::logError("Failed to save world to %s", currentWorldFile_.c_str());
    }
}

void Game::loadContentFiles() {
    struct Dir {
        const char* sub;
        bool weapons;
    };
    const Dir dirs[] = {{"content/weapons/", true}, {"content/entities/", false}};
    int loaded = 0;
    for (const Dir& dir : dirs) {
        std::string path = serverRoot_ + dir.sub;
        for (const std::string& file : fs_->listDirectory(path)) {
            if (file.size() < 4 || file.substr(file.size() - 4) != ".cfg") continue;
            auto text = fs_->readTextFile(path + file);
            if (!text) continue;
            bool ok = false;
            if (dir.weapons) {
                WeaponDef w;
                ok = parseWeaponDef(*text, w);
                if (ok) content_.addOrReplace(w);
            } else {
                EntityDef d;
                ok = parseEntityDef(*text, d);
                if (ok) content_.addOrReplace(d);
            }
            if (ok) ++loaded;
            else eng::logError("Skipping content file with no usable id: %s%s",
                               path.c_str(), file.c_str());
        }
    }
    if (loaded > 0) {
        eng::logInfo("Loaded %d content definitions from %scontent/", loaded,
                     serverRoot_.c_str());
    } else {
        eng::logInfo("No content files found under %scontent/ - using built-in defaults",
                     serverRoot_.c_str());
    }
}

void Game::sfx(const char* soundName, float pitch) const {
    if (audio_) audio_->playSound(soundName, pitch);
}

float Game::rand01() {
    fxRng_ ^= fxRng_ << 13;
    fxRng_ ^= fxRng_ >> 17;
    fxRng_ ^= fxRng_ << 5;
    return float(fxRng_ % 10000u) / 10000.0f;
}

void Game::updateCarriedProp() {
    if (carriedEntityId_ == 0) return;
    WorldEntity* e = world_.entityById(carriedEntityId_);
    if (!e || !e->active) { // destroyed or gone while carried (shouldn't happen)
        carriedEntityId_ = 0;
        return;
    }
    // Float in front of the eye, pulled in when a wall is closer so the
    // carried prop never sticks through geometry.
    const glm::vec3 eye = eyePosition();
    const glm::vec3 look = lookDirection();
    float margin = glm::length(glm::vec3(e->size.x, 0.0f, e->size.z)) * 0.5f + 0.1f;
    float geoT = world_.raycastGeometry(eye, look, 2.0f + margin);
    float holdDist = glm::clamp(geoT - margin, 0.7f, 2.0f);
    e->pos = eye + look * holdDist - glm::vec3(0.0f, e->size.y * 0.5f, 0.0f);
}

void Game::dropCarriedProp() {
    if (carriedEntityId_ == 0) return;
    WorldEntity* e = world_.entityById(carriedEntityId_);
    if (!e) {
        carriedEntityId_ = 0;
        return;
    }

    // Find a legal resting spot: from the hold point inward toward the
    // player, settle each candidate onto whatever is below and take the
    // first that fits without intersecting walls, props, or the player.
    // Nothing ever gets wedged into geometry - if nowhere fits, keep
    // carrying and say so.
    const glm::vec3 eye = eyePosition();
    const glm::vec3 look = lookDirection();
    const glm::vec3 down(0.0f, -1.0f, 0.0f);
    AABB playerBox{player_.pos - glm::vec3(0.5f, 0.0f, 0.5f),
                   player_.pos + glm::vec3(0.5f, 2.0f, 0.5f)};

    const float tries[] = {2.0f, 1.4f, 0.9f, 0.6f};
    for (float dist : tries) {
        glm::vec3 center = eye + look * dist;
        glm::vec3 pos = center - glm::vec3(0.0f, e->size.y * 0.5f, 0.0f);

        // Settle downward (floor or the top of a stack).
        glm::vec3 from = pos + glm::vec3(0.0f, 0.05f, 0.0f);
        float t = world_.raycastGeometry(from, down, 50.0f);
        World::EntityHit under =
            world_.raycastEntities(from, down, t, World::RayMask::AttackTargets);
        if (under.index >= 0) t = under.t;
        if (t > 0.05f && t < 50.0f) pos.y -= (t - 0.05f);

        glm::vec3 half(e->size.x * 0.5f, 0.0f, e->size.z * 0.5f);
        AABB rest{pos - half + glm::vec3(0.0f, 0.01f, 0.0f),
                  pos + glm::vec3(half.x, e->size.y - 0.01f, half.z)};
        if (world_.overlapsAny(rest) || aabbOverlap(rest, playerBox)) continue;

        e->pos = pos;
        e->carried = false;
        carriedEntityId_ = 0;
        saveCurrentWorld();
        sfx("pickup", 0.9f);
        showToast("DROPPED " + toUpperAscii(e->defId));
        return;
    }
    showToast("NO ROOM TO DROP IT HERE");
}

void Game::refreshWorldList() {
    worldList_.clear();
    for (const std::string& name : fs_->listDirectory(worldsRoot_)) {
        std::string file = worldsRoot_ + name + "/world.cfg";
        if (!fs_->exists(file)) continue; // not a world folder
        WorldListEntry entry{name, name};
        if (auto text = fs_->readTextFile(file)) {
            WorldSaveData data;
            if (parseWorldSave(*text, data) && !data.displayName.empty()) {
                entry.displayName = data.displayName;
            }
        }
        worldList_.push_back(std::move(entry));
    }
    std::sort(worldList_.begin(), worldList_.end(),
              [](const WorldListEntry& a, const WorldListEntry& b) {
                  return a.displayName < b.displayName;
              });
}

std::vector<const EntityDef*> Game::spawnableDefs(ContentCategory category) const {
    std::vector<const EntityDef*> out;
    for (const EntityDef& def : content_.defs()) {
        if (def.spawnable && def.category == category) out.push_back(&def);
    }
    return out;
}

void Game::spawnFromMenu(int spawnableIndex) {
    std::vector<const EntityDef*> defs = spawnableDefs(ContentCategory(spawnCategory_));
    if (spawnableIndex < 0 || spawnableIndex >= int(defs.size())) return;
    if (world_.entities().size() >= kMaxEntities) {
        menuStatus_ = "ENTITY LIMIT REACHED";
        return;
    }
    const EntityDef& def = *defs[size_t(spawnableIndex)];

    glm::vec3 forward(std::sin(yaw_), 0.0f, -std::cos(yaw_));
    glm::vec3 pos = player_.pos + forward * kSpawnDistance;
    world_.addEntity(def, pos);
    saveCurrentWorld(); // persistent worlds save on every change

    menuStatus_ = "SPAWNED " + toUpperAscii(def.displayName);
    if (currentWorldFile_.empty()) menuStatus_ += " - TEST ARENA IS NOT SAVED";
}

glm::vec3 Game::eyePosition() const {
    return player_.pos + glm::vec3(0.0f, eyeHeight_, 0.0f);
}

glm::vec3 Game::lookDirection() const {
    // Recoil is a real (recovering) view-punch: it nudges the aim up and a
    // touch sideways, so sustained fire climbs and must be controlled, then
    // settles back to the crosshair. Shots read the direction BEFORE the new
    // kick is applied (see tryAttack), so the first round is always true.
    float y = yaw_ + glm::radians(recoilYaw_);
    float p = glm::clamp(pitch_ + glm::radians(recoilPitch_),
                         glm::radians(-89.0f), glm::radians(89.0f));
    return glm::vec3(std::sin(y) * std::cos(p), std::sin(p), -std::cos(y) * std::cos(p));
}

// --- RPG systems: items in hand, gathering, ground drops, mobs, camera ----

std::string Game::heldWeaponId() const {
    if (!itemInv_.equippedItemId.empty()) {
        if (const ItemDef* item = items_.find(itemInv_.equippedItemId)) {
            if (!item->weaponId.empty()) return item->weaponId;
        }
    }
    return inventory_.equippedId();
}

bool Game::thirdPersonActive() const {
    switch (settings_.cameraPolicy) {
    case CameraPolicy::FirstPersonOnly: return false;
    case CameraPolicy::ThirdPersonOnly: return true;
    case CameraPolicy::AllowToggle: break;
    }
    return settings_.thirdPerson;
}

// E on a resource node: one gather per press. The right tool doubles the
// yield; bare hands always work so a fresh spawn is never locked out of the
// loop. Depletion goes through consumePickup so the def's respawn rule is
// honored exactly like weapon pickups.
void Game::tryGather(int entityIndex) {
    const WorldEntity& src = world_.entities()[size_t(entityIndex)];
    WorldEntity* node = world_.entityById(src.id);
    if (!node || !node->active || node->harvestItemId.empty()) return;

    const ItemDef* item = items_.find(node->harvestItemId);
    bool rightTool = false;
    if (!node->toolClass.empty() && !itemInv_.equippedItemId.empty()) {
        if (const ItemDef* tool = items_.find(itemInv_.equippedItemId)) {
            rightTool = tool->toolClass == node->toolClass;
        }
    }
    int yield = node->harvestCount * (rightTool ? 2 : 1);
    int added = itemInv_.add(node->harvestItemId, yield, item ? item->maxStack : 99);
    if (added <= 0) {
        showToast("INVENTORY FULL");
        return;
    }

    node->hitFlash = 0.12f; // same red blink the damage path uses
    node->hitsLeft -= 1;
    glm::vec3 at = node->pos + glm::vec3(0.0f, node->size.y * 0.6f, 0.0f);
    addSparks(at, 4, item ? item->color : glm::vec3(0.8f));
    sfx("dummy_hit", rightTool ? 1.1f : 0.85f);

    std::string name = toUpperAscii(item ? item->displayName : node->harvestItemId);
    if (node->hitsLeft <= 0) {
        showToast("+" + std::to_string(added) + " " + name + " - DEPLETED");
        world_.consumePickup(entityIndex);
        aimInteract_ = {};
        aimInfo_ = {}; // indices may have shifted; refreshed next frame
    } else {
        showToast("+" + std::to_string(added) + " " + name);
    }
}

// E on a dropped item: straight into the item inventory.
void Game::collectGroundItem(int entityIndex) {
    const WorldEntity& e = world_.entities()[size_t(entityIndex)];
    const ItemDef* item = items_.find(e.itemId);
    int added = itemInv_.add(e.itemId, e.itemCount, item ? item->maxStack : 99);
    if (added <= 0) {
        showToast("INVENTORY FULL");
        return;
    }
    std::string name = toUpperAscii(item ? item->displayName : e.itemId);
    world_.consumePickup(entityIndex);
    aimInteract_ = {};
    aimInfo_ = {};
    sfx("pickup");
    showToast("+" + std::to_string(added) + " " + name);
}

// Destroyed entities with a loot table leave their items on the ground as
// "drop_<item>" entities (content.cpp keeps that naming convention).
void Game::spawnLootDrop(const std::string& deadDefId, glm::vec3 pos) {
    const EntityDef* dead = content_.find(deadDefId);
    if (!dead || dead->lootItemId.empty()) return;
    const EntityDef* drop = content_.find("drop_" + dead->lootItemId);
    if (!drop) return;
    for (int i = 0; i < dead->lootCount; ++i) {
        if (world_.entities().size() >= kMaxEntities) break;
        glm::vec3 at = pos + glm::vec3((rand01() - 0.5f) * 0.7f, 0.05f,
                                       (rand01() - 0.5f) * 0.7f);
        world_.addEntity(*drop, at, 0.0f); // loot never respawns
    }
    saveCurrentWorld();
}

// Deliberately tiny mob brain: hostiles amble toward a nearby player and
// strike on contact. No pathing, no obstacle avoidance, no wander - mobs
// walk straight lines and stop at strike range. Movement mutates entity
// positions through stable ids so respawns/erases can never dangle.
void Game::updateMobs(float dt) {
    std::vector<unsigned> ids;
    for (const WorldEntity& e : world_.entities()) {
        if (e.hostile && e.active) ids.push_back(e.id);
    }
    for (unsigned id : ids) {
        WorldEntity* e = world_.entityById(id);
        if (!e) continue;
        MobBrain& brain = mobs_[id];
        brain.attackCooldown = std::max(0.0f, brain.attackCooldown - dt);

        glm::vec3 toPlayer = player_.pos - e->pos;
        toPlayer.y = 0.0f;
        float dist = glm::length(toPlayer);
        if (dist > kMobAggroRange || dist < 0.01f) continue;

        if (dist > kMobStrikeRange) {
            glm::vec3 next = e->pos + (toPlayer / dist) * (kMobSpeed * dt);
            next.x = glm::clamp(next.x, -19.0f, 19.0f);
            next.z = glm::clamp(next.z, -19.0f, 19.0f);
            // Settle onto whatever is underfoot (the camp mound, the shelf).
            glm::vec3 from = next + glm::vec3(0.0f, 0.6f, 0.0f);
            float down = world_.raycastGeometry(from, glm::vec3(0.0f, -1.0f, 0.0f), 3.0f);
            if (down < 3.0f) next.y = from.y - down + 0.005f;
            e->pos = next;
        } else if (brain.attackCooldown <= 0.0f) {
            brain.attackCooldown = kMobStrikeCooldown;
            damageFlash_ = 0.35f;
            triggerShake(0.4f, 0.2f);
            sfx("kick_hit");
            damagePlayer(e->contactDamage);
        }
    }
}

void Game::updateAimTarget() {
    const glm::vec3 eye = eyePosition();
    const glm::vec3 dir = lookDirection();

    // Walls occlude both the interact prompt and the target-info readout.
    float geoT = world_.raycastGeometry(eye, dir, kAimInfoRange);
    aimInteract_ = world_.raycastEntities(eye, dir, std::min(geoT, kInteractRange),
                                          World::RayMask::Interactables);
    aimInfo_ = world_.raycastEntities(eye, dir, geoT, World::RayMask::AttackTargets);
    if (aimInfo_.index >= 0 &&
        world_.entities()[size_t(aimInfo_.index)].maxHealth <= 0.0f) {
        aimInfo_ = {}; // only damageable entities get the info readout
    }
}

void Game::tryInteract() {
    // Carrying something: E drops it, whatever is under the crosshair.
    if (carriedEntityId_ != 0) {
        dropCarriedProp();
        return;
    }
    if (aimInteract_.index < 0) return;
    const WorldEntity& e = world_.entities()[size_t(aimInteract_.index)];

    if (!e.weaponId.empty()) { // weapon pickup
        std::string weaponId = e.weaponId;
        world_.consumePickup(aimInteract_.index);
        aimInteract_ = {};
        aimInfo_ = {}; // indices may have shifted; refreshed next frame

        const WeaponDef* weapon = content_.findWeapon(weaponId);
        const char* name = weapon ? weapon->displayName.c_str() : weaponId.c_str();
        bool newlyAcquired = inventory_.acquireAndEquip(weaponId);
        showToast(std::string(newlyAcquired ? "PICKED UP " : "EQUIPPED ") + toUpperAscii(name));
        sfx("pickup");
        eng::logInfo("Picked up weapon '%s'", weaponId.c_str());
        return;
    }

    if (e.gear) { // equipment pickup: the shield rides along with melee weapons
        world_.consumePickup(aimInteract_.index);
        aimInteract_ = {};
        aimInfo_ = {};
        hasShield_ = true;
        sfx("pickup");
        showToast("SHIELD EQUIPPED - HOLD RIGHT CLICK TO RAISE IT");
        return;
    }

    if (!e.itemId.empty()) { // ground item drop -> item inventory
        collectGroundItem(aimInteract_.index);
        return;
    }

    if (!e.harvestItemId.empty()) { // resource node: gather one hit
        tryGather(aimInteract_.index);
        return;
    }

    if (e.carryable) { // light prop: start carrying by stable id
        WorldEntity* prop = world_.entityById(e.id);
        if (!prop) return;
        prop->carried = true;
        carriedEntityId_ = prop->id;
        aimInteract_ = {};
        aimInfo_ = {};
        sfx("pickup");
        showToast("CARRYING " + toUpperAscii(prop->defId) + " - E TO DROP");
    }
}

// Hitscan weapons only (the Glock and the staff's magic bolt). Melee attacks
// run through the shared combat module via updateMelee/performMeleeStrike.
void Game::tryAttack() {
    const WeaponDef* weapon = content_.findWeapon(heldWeaponId());
    if (!weapon || weapon->kind != WeaponKind::Hitscan) return;

    // In MiniCS the trigger only fires while the round is Live; on the end
    // screen the click means "play again" (handled in update()).
    if (minicsActive_ && minicsPhase_ != MiniCSPhase::Live) return;
    if (reloadTimer_ > 0.0f) return; // hands busy chambering a round
    if (attackCooldown_ > 0.0f) return;

    // Ammo gate: an empty magazine dry-fires (a flat click) and auto-reloads
    // instead of shooting. Only the Glock uses ammo; the staff is cooldown-only.
    if (usesAmmo()) {
        if (magAmmo_ <= 0) {
            sfx("dry_fire", 0.98f + rand01() * 0.06f);
            attackCooldown_ = 0.18f; // don't machine-gun the empty click
            if (reserveAmmo_ > 0) tryReload();
            else showToast("OUT OF AMMO");
            return;
        }
        magAmmo_ -= 1;
    }

    attackCooldown_ = weapon->cooldownSeconds;
    muzzleFlashTimer_ = 0.07f;
    // The staff whooshes instead of banging; same audio set, different read.
    if (weapon->id == "basic_staff") sfx("swing", 0.55f + rand01() * 0.06f);
    else sfx("gunshot", 0.96f + rand01() * 0.08f);

    const glm::vec3 eye = eyePosition();
    const glm::vec3 dir = lookDirection();
    float geoT = world_.raycastGeometry(eye, dir, weapon->range);
    World::EntityHit hit =
        world_.raycastEntities(eye, dir, geoT, World::RayMask::AttackTargets);

    // Tracer: a short streak that leaves the muzzle and travels to the
    // impact point at a visible speed; the impact puff pops when it ARRIVES
    // (handled in the update loop), not the instant the trigger clicks.
    // The muzzle offset approximates where the viewmodel muzzle sits on
    // screen (the viewmodel renders through its own fixed-FOV projection,
    // so a perfect match is impossible by design - close is correct here).
    float endT = hit.index >= 0 ? hit.t : geoT;
    glm::vec3 impact = eye + dir * endT;
    glm::vec3 right = glm::normalize(glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, dir));
    Tracer tr;
    tr.from = eye + dir * 0.60f + right * 0.17f - up * 0.125f;
    tr.to = impact;
    float dist = glm::length(tr.to - tr.from);
    tr.life = tr.total = std::max(0.02f, dist / 130.0f); // ~130 m/s visible speed
    tr.impact = endT < weapon->range - 0.01f;
    tracers_.push_back(tr);

    // Recoil view-punch AFTER the round leaves: the muzzle snaps up and a hair
    // sideways, then recovers in update(). Capped so it never spins the camera.
    if (usesAmmo()) {
        recoilPitch_ = std::min(7.0f, recoilPitch_ + 0.62f + rand01() * 0.30f);
        recoilYaw_ += (rand01() - 0.5f) * 0.55f;
    }
    if (hit.index < 0) return;

    const WorldEntity& target = world_.entities()[size_t(hit.index)];
    if (target.maxHealth <= 0.0f) return; // solid but not damageable (crates)

    std::string targetId = target.defId;
    unsigned targetEid = target.id;
    glm::vec3 targetPos = target.pos;
    hitMarkerTimer_ = 0.12f;
    sfx("dummy_hit");
    // A struck MiniCS enemy flinches: interrupt its aim and make it reel.
    auto brainIt = minicsBrains_.find(targetEid);
    if (brainIt != minicsBrains_.end()) {
        brainIt->second.state = MinicsEnemy::State::Flinch;
        brainIt->second.flinchTimer = std::max(brainIt->second.flinchTimer, 0.22f);
        brainIt->second.aimBlend *= 0.4f;
    }
    bool destroyed = world_.damageEntity(hit.index, weapon->damage);
    aimInteract_ = {};
    aimInfo_ = {}; // entity vector may have shifted; refreshed next frame
    if (destroyed) {
        bots_.erase(targetEid); // a respawned duelist starts with a fresh brain
        mobs_.erase(targetEid);
        if (minicsBrains_.erase(targetEid) > 0) {
            onMinicsEnemyKilled(targetPos); // score + kill confirm + popup
        } else {
            spawnLootDrop(targetId, targetPos);
            const EntityDef* def = content_.find(targetId);
            showToast(toUpperAscii(def ? def->displayName : targetId) + " DESTROYED");
        }
    }
}

void Game::showToast(std::string text) {
    hudToast_ = std::move(text);
    hudToastTimer_ = 1.6;
}

// --- melee combat: the client side of shared/melee ------------------------
// The Actor state machine lives in shared code; this layer feeds it inputs,
// does the targeting raycasts, and turns outcomes into sound/sparks/shake.

void Game::updateMelee(const InputState& in, float dt) {
    using melee::Attack;
    melee::Actor& A = playerMelee_;
    const WeaponDef* weapon = content_.findWeapon(heldWeaponId());
    bool isMelee = weapon && weapon->kind == WeaponKind::Melee;

    A.hasShield = hasShield_;
    A.weight = isMelee ? weapon->weight : 1.0f;
    melee::setBlock(A, isMelee && in.blockHeld);
    melee::tick(A, dt);

    // Swing whoosh the instant the windup releases into the hit window.
    if (prevMeleePhase_ == melee::Phase::Windup && A.phase == melee::Phase::Active &&
        A.attack != Attack::Kick && A.attack != Attack::Jab) {
        sfx("swing");
    }

    if (isMelee) {
        // Attack inputs: left click slashes (sides alternate), Alt or a mouse
        // side button forces the other side, wheel up/down are overhead/stab,
        // F kicks. Anything started while a parry's riposte window runs
        // becomes a riposte automatically (shared module handles it).
        if (in.attackPressed) {
            Attack side = nextSlashLeft_ ? Attack::SlashLeft : Attack::SlashRight;
            if (melee::startAttack(A, side, false)) {
                nextSlashLeft_ = !nextSlashLeft_;
                heavyEligible_ = true; // keep holding to commit to a heavy
            }
        }
        if (in.altAttackPressed) {
            Attack side = nextSlashLeft_ ? Attack::SlashRight : Attack::SlashLeft;
            if (melee::startAttack(A, side, false)) heavyEligible_ = false;
        }
        if (in.wheelDelta > 0 && melee::startAttack(A, Attack::Overhead, false)) {
            heavyEligible_ = false;
        }
        if (in.wheelDelta < 0 && melee::startAttack(A, Attack::Stab, false)) {
            heavyEligible_ = false;
        }
        if (in.kickPressed && melee::startAttack(A, Attack::Kick, false)) {
            heavyEligible_ = false;
        }
        if (in.jabPressed && melee::startAttack(A, Attack::Jab, false)) {
            heavyEligible_ = false;
        }

        // Hold left click through the windup to commit the slash to a heavy.
        if (heavyEligible_ && A.phase == melee::Phase::Windup && !A.heavy) {
            if (!in.attackHeld) heavyEligible_ = false;
            else if (A.phaseTotal - A.phaseLeft >= melee::kHeavyHoldTime) {
                melee::upgradeToHeavy(A);
            }
        }

        // Feint: cancel the windup (costs stamina) and flow into a new attack.
        if (in.feintPressed && melee::feint(A)) heavyEligible_ = false;

        if (in.throwPressed) tryThrowWeapon();

        // The active window keeps sweeping until it connects once or closes.
        if (A.phase == melee::Phase::Active && !A.strikeDone) {
            performMeleeStrike(*weapon);
        }
    } else {
        heavyEligible_ = false;
    }

    prevMeleePhase_ = A.phase;
}

void Game::performMeleeStrike(const WeaponDef& weapon) {
    using melee::Attack;
    melee::Actor& A = playerMelee_;
    bool kick = A.attack == Attack::Kick;
    bool jab = A.attack == Attack::Jab;
    bool blunt = kick || jab;
    float range = kick ? melee::kKickRange : jab ? melee::kJabRange : weapon.range;

    // The blade travels through space during the release: slashes sweep an
    // arc across the view (a left slash crosses left -> right, a right
    // slash right -> left), overheads sweep top to bottom, stabs and jabs
    // thrust straight. The sweep is camera-relative, so turning INTO the
    // swing lands contact earlier (accel) and turning away drags it later -
    // simple, readable Chivalry-style timing control.
    const glm::vec3 look = lookDirection();
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    const glm::vec3 camRight = glm::normalize(glm::cross(look, worldUp));
    float p = melee::phaseProgress(A);
    float yawOff = 0.0f, pitchOff = 0.0f; // degrees off the crosshair
    switch (A.attack) {
    case Attack::SlashLeft: yawOff = glm::mix(32.0f, -32.0f, p); break; // +yaw = left
    case Attack::SlashRight: yawOff = glm::mix(-32.0f, 32.0f, p); break;
    case Attack::Overhead: pitchOff = glm::mix(28.0f, -22.0f, p); break;
    default: break; // Stab / Jab / Kick: straight thrust
    }
    glm::mat4 sweep = glm::rotate(glm::mat4(1.0f), glm::radians(yawOff), worldUp);
    sweep = glm::rotate(sweep, glm::radians(pitchOff), camRight);
    glm::vec3 dir = glm::normalize(glm::vec3(sweep * glm::vec4(look, 0.0f)));

    const glm::vec3 eye = eyePosition();
    float geoT = world_.raycastGeometry(eye, dir, range);
    World::EntityHit hit =
        world_.raycastEntities(eye, dir, geoT, World::RayMask::AttackTargets);

    // Nothing along the blade's current line: keep sweeping until the
    // window closes (a full whiff recovers slower - see melee::tick).
    if (hit.index < 0 && geoT >= range) return;

    glm::vec3 hitPos = eye + dir * (hit.index >= 0 ? hit.t : geoT);

    // The blade met a wall or an undamageable prop: clang off it and eat
    // the bounce recovery. Swinging into the level is a real mistake.
    if (hit.index < 0 || world_.entities()[size_t(hit.index)].maxHealth <= 0.0f) {
        if (!blunt) {
            sfx("block");
            addSparks(hitPos, 3, {0.85f, 0.85f, 0.90f});
            triggerShake(0.15f, 0.1f);
        }
        melee::bounceOff(A);
        return;
    }

    const WorldEntity& target = world_.entities()[size_t(hit.index)];
    std::string targetId = target.defId;
    unsigned targetEid = target.id;
    glm::vec3 targetPos = target.pos;
    auto brainIt = bots_.find(targetEid);

    auto applyDamage = [&]() {
        hitMarkerTimer_ = 0.12f;
        sfx(blunt ? "kick_hit" : "melee_hit");
        hitStop_ = std::max(hitStop_, jab ? 0.03f : A.heavy ? 0.09f : 0.05f);
        if (A.heavy) triggerShake(0.5f, 0.25f);
        bool destroyed = world_.damageEntity(hit.index, melee::damageOf(A, weapon.damage));
        aimInteract_ = {};
        aimInfo_ = {}; // indices may have shifted; refreshed next frame
        if (destroyed) {
            bots_.erase(targetEid); // if it ever comes back, fresh brain
            mobs_.erase(targetEid);
            spawnLootDrop(targetId, targetPos);
            const EntityDef* def = content_.find(targetId);
            showToast(toUpperAscii(def ? def->displayName : targetId) + " DESTROYED");
        }
    };

    if (brainIt == bots_.end()) {
        // Passive target (training dummy): plain damage, no exchange.
        A.strikeDone = true;
        applyDamage();
        return;
    }

    // Duelist bot: the exchange goes through the shared resolver, which
    // settles stamina and phases for both sides; we present the outcome.
    melee::Outcome out = melee::resolveStrike(A, brainIt->second.actor);
    using melee::Outcome;
    switch (out) {
    case Outcome::Hit:
        applyDamage();
        break;
    case Outcome::Interrupted:
        applyDamage();
        showToast("INTERRUPTED ITS ATTACK");
        break;
    case Outcome::Blocked:
    case Outcome::ShieldBlocked:
        sfx("block");
        addSparks(hitPos, 5, {1.0f, 0.75f, 0.35f});
        triggerShake(0.2f, 0.12f);
        break;
    case Outcome::Deflected: // it was mid-riposte: protected
        sfx("block");
        addSparks(hitPos, 6, {0.9f, 0.9f, 0.95f});
        showToast("DEFLECTED - RIPOSTES ARE PROTECTED");
        break;
    case Outcome::GuardBroken:
        sfx("guard_break");
        addSparks(hitPos, 14, {1.0f, 0.85f, 0.45f});
        showToast("GUARD BROKEN - STRIKE NOW");
        break;
    case Outcome::GuardOpened:
        sfx("kick_hit");
        addSparks(hitPos, 6, {0.9f, 0.9f, 0.95f});
        showToast("GUARD KICKED OPEN");
        break;
    case Outcome::Parried:        // bots hold plain blocks and never parry,
    case Outcome::PerfectCountered: // but keep the cases honest if that changes
        sfx("parry");
        addSparks(hitPos, 10, {1.0f, 0.9f, 0.5f});
        break;
    }
}

void Game::updateBots(float dt) {
    const std::vector<WorldEntity>& ents = world_.entities();
    for (size_t i = 0; i < ents.size(); ++i) {
        const WorldEntity& e = ents[i];
        if (e.defId != "duel_bot" || !e.active) continue;

        BotBrain& b = bots_[e.id];
        melee::Actor& B = b.actor;
        B.weight = 1.7f;    // slow, telegraphed "basic attacks"
        B.counters = false; // bots neither parry nor counter (training partner)
        if (b.rng == 0x9E3779B9u) b.rng ^= e.id * 2654435761u + 1u;
        auto roll = [&b]() {
            b.rng ^= b.rng << 13;
            b.rng ^= b.rng >> 17;
            b.rng ^= b.rng << 5;
            return b.rng;
        };

        melee::Phase prev = B.phase;
        b.guardLeft = std::max(0.0f, b.guardLeft - dt);
        melee::setBlock(B, b.guardLeft > 0.0f);
        B.blockTime = 1.0f; // held guard only: the parry window never applies
        melee::tick(B, dt);

        glm::vec3 chest = e.pos + glm::vec3(0.0f, e.size.y * 0.65f, 0.0f);
        float dist = glm::distance(eyePosition(), chest);

        // Decision loop ("no full AI"): near the player it alternates between
        // slow random attacks and, sometimes, a held guard to kick open.
        if (dist < 4.5f && B.phase == melee::Phase::Idle && b.guardLeft <= 0.0f) {
            b.decideIn -= dt;
            if (b.decideIn <= 0.0f) {
                melee::Attack pick = melee::Attack(int(roll() % 4u));
                if (melee::startAttack(B, pick, false)) {
                    sfx("windup_cue"); // audible telegraph: this is the parry timer
                    b.decideIn = 1.7f + float(roll() % 100u) * 0.014f;
                }
            }
        }

        // Its strike lands once, a beat into the hit window (the blade has
        // visibly turned red by then), if the player is still in reach;
        // otherwise the window closes into a whiffed recovery.
        if (B.phase == melee::Phase::Active && !B.strikeDone &&
            melee::phaseProgress(B) >= 0.35f && dist <= 2.6f) {
            applyBotStrikeOnPlayer(B);
        }

        // After an attack resolves it sometimes raises its guard, so kicks
        // and guard breaks have something to practice on.
        if (prev == melee::Phase::Recovery && B.phase == melee::Phase::Idle &&
            roll() % 100u < 45u) {
            b.guardLeft = 2.6f;
        }
    }
}

void Game::applyBotStrikeOnPlayer(melee::Actor& bot) {
    melee::Outcome out = melee::resolveStrike(bot, playerMelee_);
    glm::vec3 clash = eyePosition() + lookDirection() * 0.7f - glm::vec3(0.0f, 0.12f, 0.0f);
    using melee::Outcome;
    switch (out) {
    case Outcome::Hit:
        damageFlash_ = 0.4f;
        triggerShake(0.7f, 0.3f);
        sfx("melee_hit");
        damagePlayer(melee::damageOf(bot, 12.0f)); // bot blade: modest base damage
        showToast("HIT - HOLD RIGHT CLICK TO BLOCK");
        break;
    case Outcome::Interrupted: // caught mid-windup: your swing is gone too
        damageFlash_ = 0.45f;
        triggerShake(0.75f, 0.3f);
        sfx("melee_hit");
        damagePlayer(melee::damageOf(bot, 12.0f));
        showToast("INTERRUPTED - DON'T SWING INTO ITS ATTACK");
        break;
    case Outcome::Deflected: // your riposte windup shrugged it off
        sfx("block");
        addSparks(clash, 8, {0.9f, 0.9f, 0.95f});
        showToast("RIPOSTE DEFLECTED IT - KEEP SWINGING");
        break;
    case Outcome::Blocked:
        sfx("block");
        addSparks(clash, 5, {1.0f, 0.75f, 0.35f});
        triggerShake(0.3f, 0.15f);
        break;
    case Outcome::ShieldBlocked:
        sfx("shield_block");
        addSparks(clash, 4, {0.9f, 0.8f, 0.6f});
        triggerShake(0.22f, 0.12f);
        break;
    case Outcome::Parried:
        sfx("parry");
        addSparks(clash, 12, {1.0f, 0.92f, 0.55f});
        hitStop_ = std::max(hitStop_, 0.05f);
        showToast("PARRY - RIPOSTE NOW");
        break;
    case Outcome::PerfectCountered:
        sfx("parry");
        addSparks(clash, 18, {1.0f, 0.95f, 0.7f});
        hitStop_ = std::max(hitStop_, 0.08f);
        triggerShake(0.35f, 0.2f);
        showToast("PERFECT COUNTER");
        break;
    case Outcome::GuardBroken:
        sfx("guard_break");
        damageFlash_ = 0.25f;
        triggerShake(0.9f, 0.45f);
        showToast("GUARD BROKEN - BACK OFF AND RECOVER");
        break;
    case Outcome::GuardOpened: // bots do not kick; case kept complete
        sfx("kick_hit");
        break;
    }
}

void Game::tryThrowWeapon() {
    melee::Actor& A = playerMelee_;
    if (!itemInv_.equippedItemId.empty()) {
        // Crafted tools/weapons live in the item inventory; throwing would
        // desync the two systems. Unequip via the inventory screen instead.
        showToast("CRAFTED GEAR CANNOT BE THROWN");
        return;
    }
    const WeaponDef* w = content_.findWeapon(inventory_.equippedId());
    if (!w || w->kind != WeaponKind::Melee || w->id == "fists") {
        showToast("NOTHING THROWABLE IN HAND");
        return;
    }
    if (A.phase == melee::Phase::Windup || A.phase == melee::Phase::Active ||
        A.phase == melee::Phase::Stagger) {
        return; // mid-swing or reeling: hands are busy
    }
    if (!melee::spendStamina(A, melee::kThrowCost)) {
        showToast("TOO TIRED TO THROW");
        return;
    }
    std::string id = inventory_.removeEquipped();
    if (id.empty()) return;

    ThrownWeapon t;
    t.weaponId = id;
    t.pos = eyePosition() + lookDirection() * 0.4f;
    t.vel = lookDirection() * 15.0f + glm::vec3(0.0f, 2.4f, 0.0f); // slight lob
    thrown_.push_back(std::move(t));
    sfx("throw");
}

void Game::updateThrownWeapons(float dt) {
    for (size_t i = 0; i < thrown_.size();) {
        ThrownWeapon& t = thrown_[i];
        t.life += dt;
        t.spin += dt * 13.0f; // end-over-end tumble
        t.vel.y -= 20.0f * dt; // same gravity as the movement sim
        glm::vec3 next = t.pos + t.vel * dt;
        glm::vec3 seg = next - t.pos;
        float len = glm::length(seg);
        bool landed = false;

        if (len > 1e-5f) {
            glm::vec3 dir = seg / len;
            float geoT = world_.raycastGeometry(t.pos, dir, len);
            World::EntityHit hit =
                world_.raycastEntities(t.pos, dir, geoT, World::RayMask::AttackTargets);
            if (hit.index >= 0) {
                const WorldEntity& e = world_.entities()[size_t(hit.index)];
                glm::vec3 hitPos = t.pos + dir * hit.t;
                std::string targetId = e.defId;
                unsigned targetEid = e.id;
                auto brainIt = bots_.find(targetEid);
                if (brainIt != bots_.end() && melee::guardUp(brainIt->second.actor)) {
                    // Swatted out of the air by a raised guard.
                    sfx("block");
                    addSparks(hitPos, 5, {1.0f, 0.75f, 0.35f});
                } else if (e.maxHealth > 0.0f) {
                    const WeaponDef* w = content_.findWeapon(t.weaponId);
                    glm::vec3 targetPos = e.pos;
                    hitMarkerTimer_ = 0.12f;
                    sfx("melee_hit");
                    bool destroyed =
                        world_.damageEntity(hit.index, (w ? w->damage : 20.0f) * 1.1f);
                    aimInteract_ = {};
                    aimInfo_ = {};
                    if (destroyed) {
                        bots_.erase(targetEid);
                        mobs_.erase(targetEid);
                        spawnLootDrop(targetId, targetPos);
                        const EntityDef* def = content_.find(targetId);
                        showToast(toUpperAscii(def ? def->displayName : targetId) +
                                  " DESTROYED");
                    }
                }
                spawnDroppedWeapon(t.weaponId, hitPos);
                landed = true;
            } else if (geoT < len) {
                spawnDroppedWeapon(t.weaponId, t.pos + dir * std::max(0.0f, geoT - 0.05f));
                sfx("footstep", 0.8f); // low clatter (original sound set)
                landed = true;
            }
        }
        if (!landed && t.life > 5.0f) { // safety: never orbit forever
            spawnDroppedWeapon(t.weaponId, t.pos);
            landed = true;
        }

        if (landed) {
            thrown_.erase(thrown_.begin() + long(i));
        } else {
            t.pos = next;
            ++i;
        }
    }
}

// A thrown weapon that landed becomes a normal pickup at the impact point
// (settled to the floor). respawn 0: once re-taken it is gone from the world.
void Game::spawnDroppedWeapon(const std::string& weaponId, glm::vec3 nearPos) {
    const EntityDef* def = content_.find(weaponId); // pickup defs share weapon ids
    if (!def) return;
    glm::vec3 from = nearPos + glm::vec3(0.0f, 0.1f, 0.0f);
    float down = world_.raycastGeometry(from, glm::vec3(0.0f, -1.0f, 0.0f), 60.0f);
    glm::vec3 pos = from;
    if (down < 60.0f) pos.y = from.y + 0.1f - down + 0.01f;
    if (world_.entities().size() < kMaxEntities) {
        world_.addEntity(*def, pos, 0.0f);
        saveCurrentWorld();
    }
}

void Game::addSparks(const glm::vec3& pos, int count, const glm::vec3& color) {
    unsigned s = unsigned(sparks_.size() * 747796405u) +
                 unsigned(menuTime_ * 1000.0) + 2891336453u;
    auto rnd = [&s]() { // [-1, 1)
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return float(s % 2000u) / 1000.0f - 1.0f;
    };
    for (int i = 0; i < count; ++i) {
        Spark p;
        p.pos = pos;
        p.vel = glm::vec3(rnd() * 2.4f, 1.6f + rnd() * 1.6f, rnd() * 2.4f);
        p.color = color;
        p.total = p.life = 0.22f + (rnd() * 0.5f + 0.5f) * 0.18f;
        sparks_.push_back(p);
    }
}

void Game::updateSparks(float dt) {
    for (size_t i = 0; i < sparks_.size();) {
        Spark& p = sparks_[i];
        p.life -= dt;
        if (p.life <= 0.0f) {
            sparks_.erase(sparks_.begin() + long(i));
            continue;
        }
        p.vel.y -= 12.0f * dt;
        p.pos += p.vel * dt;
        ++i;
    }
}

// Real damage on the player (duelist bot strikes). Going down is soft in
// the sandbox: you respawn at the spawn point with full health and keep
// your weapons - the lesson is the drained bar, not lost progress.
void Game::damagePlayer(float amount) {
    if (amount <= 0.0f) return;
    playerHealth_ = std::max(0.0f, playerHealth_ - amount);
    sinceDamaged_ = 0.0f;
    if (playerHealth_ > 0.0f) return;

    if (minicsActive_) {
        // MiniCS is a round: going down loses it (no free sandbox respawn).
        if (minicsPhase_ == MiniCSPhase::Live) minicsLose("YOU WERE ELIMINATED");
        return;
    }

    player_.respawn(world_.spawnPoint);
    player_.vel = glm::vec3(0.0f);
    prevPos_ = player_.pos;
    playerHealth_ = 100.0f;
    playerHealthShown_ = 0.0f; // the bar visibly refills on respawn
    playerMelee_ = melee::Actor{};
    prevMeleePhase_ = melee::Phase::Idle;
    if (carriedEntityId_ != 0) { // release the held prop and settle it down
        if (WorldEntity* held = world_.entityById(carriedEntityId_)) {
            held->carried = false;
            glm::vec3 from = held->pos + glm::vec3(0.0f, 0.05f, 0.0f);
            float down = world_.raycastGeometry(from, glm::vec3(0.0f, -1.0f, 0.0f), 50.0f);
            if (down < 50.0f) held->pos.y = from.y - down + 0.01f;
        }
        carriedEntityId_ = 0;
    }
    damageFlash_ = 0.9f;
    triggerShake(1.0f, 0.5f);
    showToast("YOU WENT DOWN - BACK AT SPAWN");
}

void Game::triggerShake(float amplitude, float seconds) {
    // A stronger shake replaces a weaker one; a weaker one never cuts a
    // strong shake short.
    if (amplitude >= shakeAmp_ || shakeTime_ <= 0.0f) {
        shakeAmp_ = amplitude;
        shakeTime_ = shakeTotal_ = seconds;
    }
}

// --- MiniCS: the round-based FPS loop --------------------------------------
// A self-contained match controller: spawn a set of ranged enemies, count
// down, fight until they are all eliminated (win) or the player goes down /
// the clock runs out (lose), then offer a one-press restart. Everything here
// is gated on minicsActive_, so the sandbox and other modes never see it.

// ===========================================================================
// MINICS TUNING INDEX - the knobs that shape how the round feels. Most live
// right here; the rest are pointed to so they are easy to find:
//   * Round length / countdown ....... kMinicsRoundSeconds (below);
//                                       countdown = startMinicsRound()'s 2.0f
//   * Enemy count + spawn spots ....... placeMinicsEntities()
//   * Enemy engage range .............. kMinicsEngageMin/Max (below)
//   * Enemy speed / fire cadence ...... updateMinicsEnemies() (3.1 / 2.5 m/s;
//                                       fireCooldown 0.85 + rf()*0.95)
//   * Enemy accuracy + damage ......... minicsEnemyFire() (acc, 8 + rand*5)
//   * Enemy toughness ................. minics_bot.cfg max_health (90) + builtin
//   * Glock damage / cadence .......... glock.cfg (34 / 0.14) + builtin
//   * Magazine / reserve / reload ..... game.h magSize_/reloadDuration_,
//                                       startMinicsRound() reserveAmmo_
//   * Recoil kick / recovery .......... tryAttack() recoilPitch_, update() 11*dt
//   * Neon look / atmosphere .......... world_template.cpp minicsArena(),
//                                       buildRenderFrame() atmosphere block
// ===========================================================================
namespace {
constexpr float kMinicsRoundSeconds = 75.0f; // round clock during the fight
constexpr float kMinicsEngageMin = 6.0f;     // enemies try to hold this range...
constexpr float kMinicsEngageMax = 15.0f;    // ...and close/open toward it
} // namespace

bool Game::usesAmmo() const {
    // Ammo, reload and recoil are MiniCS gun-feel: scoped to the round so the
    // sandbox Glock stays exactly as it was (cooldown-only, no magazine, no
    // recoil, no ammo HUD).
    return minicsActive_ && heldWeaponId() == "glock";
}

void Game::tryReload() {
    if (!usesAmmo() || reloadTimer_ > 0.0f) return;
    if (magAmmo_ >= magSize_) return;          // already topped off
    if (reserveAmmo_ <= 0) {
        showToast("NO SPARE MAGAZINES");
        return;
    }
    reloadTimer_ = reloadDuration_;
    recoilPitch_ = recoilYaw_ = 0.0f; // hands leave the sights for the reload
    sfx("reload");
}

// The MiniCS arena loadout: a spare pistol near spawn and the enemy line
// holding the north half. Called instead of the template's own placements so
// Deathmatch (which shares minics_arena) keeps its melee duel-bots.
void Game::placeMinicsEntities() {
    auto place = [&](const char* id, glm::vec3 pos, float respawn = 0.0f) {
        if (const EntityDef* def = content_.find(id)) world_.addEntity(*def, pos, respawn);
    };
    place("glock", {2.0f, 0.0f, 16.5f}, -1.0f); // spare pistol at the south spawn
    // Neon pillar landmarks (also light cover) framing the mid lanes.
    place("neon_pillar", {-6.5f, 0.0f, 0.0f});
    place("neon_pillar", {6.5f, 0.0f, 0.0f});
    place("neon_pillar", {0.0f, 0.0f, -18.5f});
    // Five enemies spread across the north lanes and the mid platform edges.
    place("minics_bot", {-8.0f, 0.0f, -12.0f});
    place("minics_bot", {8.0f, 0.0f, -13.0f});
    place("minics_bot", {0.0f, 0.0f, -16.5f});
    place("minics_bot", {-12.0f, 0.0f, -4.0f});
    place("minics_bot", {12.0f, 0.0f, -6.0f});
}

void Game::startMinicsRound() {
    minicsActive_ = true;
    minicsBrains_.clear();
    scorePopups_.clear();
    minicsScore_ = 0;
    minicsKills_ = 0;
    minicsResultNote_.clear();

    // One brain per placed enemy; count them for the win condition.
    int count = 0;
    for (const WorldEntity& e : world_.entities()) {
        if (e.defId != "minics_bot" || !e.active) continue;
        MinicsEnemy brain;
        brain.rng = 0x9E3779B9u ^ (e.id * 2654435761u + 1u);
        brain.yaw = std::atan2(player_.pos.x - e.pos.x, -(player_.pos.z - e.pos.z));
        brain.stateTimer = 0.4f + float(brain.rng % 100u) * 0.006f;
        brain.strafeSign = (brain.rng & 1u) ? 1.0f : -1.0f;
        minicsBrains_[e.id] = brain;
        ++count;
    }
    minicsEnemiesTotal_ = minicsEnemiesAlive_ = count;

    minicsPhase_ = MiniCSPhase::Countdown;
    minicsPhaseTimer_ = 2.0f; // a short "GET READY" beat before the fight
    minicsBannerTimer_ = 0.0f;

    // Fresh magazine + reserve for the round; clear any carried recoil/reload.
    magAmmo_ = magSize_;
    reserveAmmo_ = 68;
    reloadTimer_ = recoilPitch_ = recoilYaw_ = 0.0f;
    playerHealth_ = playerHealthShown_ = 100.0f;
    sinceDamaged_ = 999.0f;
    controlHintTimer_ = 6.0;
    sfx("windup_cue", 0.8f); // a low "get ready" tone
}

void Game::updateMinics(float dt) {
    if (!minicsActive_) return;
    minicsBannerTimer_ += dt;
    switch (minicsPhase_) {
    case MiniCSPhase::Countdown:
        minicsPhaseTimer_ -= dt;
        if (minicsPhaseTimer_ <= 0.0f) {
            minicsPhase_ = MiniCSPhase::Live;
            minicsPhaseTimer_ = kMinicsRoundSeconds;
            minicsBannerTimer_ = 0.0f;
            sfx("kill_confirm", 0.8f); // bright "go" tone as the fight starts
        }
        break;
    case MiniCSPhase::Live: {
        int alive = 0;
        for (const WorldEntity& e : world_.entities()) {
            if (e.defId == "minics_bot" && e.active) ++alive;
        }
        minicsEnemiesAlive_ = alive;
        minicsPhaseTimer_ -= dt;
        if (alive <= 0) minicsWin();
        else if (minicsPhaseTimer_ <= 0.0f) minicsLose("THE CLOCK RAN OUT");
        break;
    }
    case MiniCSPhase::Won:
    case MiniCSPhase::Lost:
        minicsPhaseTimer_ += dt; // seconds since the round ended (restart gate)
        break;
    }
}

void Game::minicsWin() {
    if (minicsPhase_ != MiniCSPhase::Live) return;
    int timeBonus = int(std::max(0.0f, minicsPhaseTimer_) * 10.0f); // reward speed
    minicsScore_ += timeBonus;
    minicsResultNote_ = "ALL ENEMIES ELIMINATED";
    minicsPhase_ = MiniCSPhase::Won;
    minicsPhaseTimer_ = 0.0f; // now counts up (restart cooldown)
    minicsBannerTimer_ = 0.0f;
    sfx("round_win");
}

void Game::minicsLose(const char* reason) {
    if (minicsPhase_ != MiniCSPhase::Live) return;
    minicsResultNote_ = reason ? reason : "";
    minicsPhase_ = MiniCSPhase::Lost;
    minicsPhaseTimer_ = 0.0f;
    minicsBannerTimer_ = 0.0f;
    damageFlash_ = 0.9f;
    triggerShake(1.0f, 0.5f);
    sfx("round_lose");
}

void Game::onMinicsEnemyKilled(const glm::vec3& at) {
    minicsKills_ += 1;
    minicsScore_ += 100;
    killMarkerTimer_ = 0.30f; // bigger crosshair X takes over from the hit tick
    hitMarkerTimer_ = 0.0f;
    sfx("kill_confirm");
    // A hot burst where the enemy fell + a floating "+100" near the crosshair.
    addSparks(at + glm::vec3(0.0f, 0.9f, 0.0f), 16, {1.0f, 0.5f, 0.22f});
    addSparks(at + glm::vec3(0.0f, 1.2f, 0.0f), 8, {1.0f, 0.85f, 0.4f});
    addScorePopup("+100", {1.0f, 0.9f, 0.4f});
    triggerShake(0.25f, 0.12f);
}

void Game::addScorePopup(std::string text, const glm::vec3& color) {
    ScorePopup p;
    p.text = std::move(text);
    p.color = color;
    p.life = p.total = 1.1f;
    p.driftX = (rand01() - 0.5f) * 44.0f;
    scorePopups_.push_back(std::move(p));
}

// Deliberately small ranged brain: advance to a firing range, strafe to stay
// hard to hit, pause to aim (a readable telegraph), fire one shot, repeat.
// No pathfinding - enemies move in straight lines, slide off walls, and settle
// onto whatever is underfoot. Movement mutates positions through stable ids.
void Game::updateMinicsEnemies(float dt) {
    if (!minicsActive_ || minicsPhase_ != MiniCSPhase::Live) return;
    const glm::vec3 playerEye = eyePosition();

    std::vector<unsigned> ids;
    ids.reserve(minicsBrains_.size());
    for (const auto& kv : minicsBrains_) ids.push_back(kv.first);

    for (unsigned id : ids) {
        WorldEntity* e = world_.entityById(id);
        auto it = minicsBrains_.find(id);
        if (!e || !e->active || it == minicsBrains_.end()) continue;
        MinicsEnemy& b = it->second;
        auto roll = [&b]() {
            b.rng ^= b.rng << 13; b.rng ^= b.rng >> 17; b.rng ^= b.rng << 5;
            return b.rng;
        };
        auto rf = [&]() { return float(roll() % 10000u) / 10000.0f; };

        b.fireCooldown = std::max(0.0f, b.fireCooldown - dt);
        b.muzzle = std::max(0.0f, b.muzzle - dt);
        b.stateTimer -= dt;

        glm::vec3 chest = e->pos + glm::vec3(0.0f, e->size.y * 0.82f, 0.0f);
        glm::vec3 toPlayer = player_.pos - e->pos;
        toPlayer.y = 0.0f;
        float dist = glm::length(toPlayer);
        glm::vec3 fwd = dist > 0.01f ? toPlayer / dist : glm::vec3(0, 0, 1);
        glm::vec3 side(-fwd.z, 0.0f, fwd.x); // horizontal perpendicular

        glm::vec3 toEye = playerEye - chest;
        float eyeDist = glm::length(toEye);
        glm::vec3 eyeDir = eyeDist > 0.01f ? toEye / eyeDist : fwd;
        bool los = world_.raycastGeometry(chest, eyeDir, eyeDist) >= eyeDist - 0.3f;

        float desiredYaw = std::atan2(toPlayer.x, -toPlayer.z);

        using State = MinicsEnemy::State;
        if (b.flinchTimer > 0.0f) {
            b.flinchTimer -= dt;
            b.state = State::Flinch;
        } else {
            switch (b.state) {
            case State::Flinch:
                b.state = State::Advance;
                break;
            case State::Advance:
                if (los && dist <= kMinicsEngageMax && dist >= kMinicsEngageMin &&
                    b.fireCooldown <= 0.0f) {
                    b.state = State::Aim;
                    b.stateTimer = 0.32f + rf() * 0.22f;
                } else if (los && dist < kMinicsEngageMin) {
                    b.state = State::Strafe;
                    b.stateTimer = 0.5f + rf() * 0.5f;
                }
                break;
            case State::Strafe:
                if (b.stateTimer <= 0.0f) {
                    if (los && b.fireCooldown <= 0.0f && dist <= kMinicsEngageMax) {
                        b.state = State::Aim;
                        b.stateTimer = 0.32f + rf() * 0.22f;
                    } else if (!los || dist > kMinicsEngageMax) {
                        b.state = State::Advance;
                    } else {
                        b.strafeSign = -b.strafeSign;
                        b.stateTimer = 0.5f + rf() * 0.6f;
                    }
                }
                break;
            case State::Aim:
                if (b.stateTimer <= 0.0f) b.state = los ? State::Fire : State::Advance;
                break;
            case State::Fire:
                minicsEnemyFire(*e);
                b.fireCooldown = 0.85f + rf() * 0.95f;
                b.muzzle = 0.07f;
                b.strafeSign = rf() < 0.5f ? -1.0f : 1.0f;
                b.state = State::Strafe;
                b.stateTimer = 0.45f + rf() * 0.6f;
                break;
            }
        }

        // Movement per state (aiming/firing enemies hold still).
        glm::vec3 move(0.0f);
        float speed = 0.0f;
        bool aiming = (b.state == State::Aim || b.state == State::Fire);
        if (b.flinchTimer <= 0.0f && !aiming) {
            if (b.state == State::Advance) {
                move = fwd + side * b.strafeSign * 0.4f;
                speed = 3.1f;
            } else if (b.state == State::Strafe) {
                move = side * b.strafeSign;
                if (dist < kMinicsEngageMin) move -= fwd * 0.8f;
                else if (dist > kMinicsEngageMax) move += fwd * 0.8f;
                speed = 2.5f;
            }
        }
        if (speed > 0.0f && glm::length(move) > 0.01f) {
            move = glm::normalize(move);
            float stepLen = speed * dt;
            glm::vec3 probe = e->pos + glm::vec3(0.0f, 0.6f, 0.0f);
            float wall = world_.raycastGeometry(probe, move, stepLen + 0.5f);
            if (wall > stepLen + 0.45f) {
                glm::vec3 next = e->pos + move * stepLen;
                next.x = glm::clamp(next.x, -16.6f, 16.6f);
                next.z = glm::clamp(next.z, -20.6f, 20.6f);
                glm::vec3 from = next + glm::vec3(0.0f, 1.0f, 0.0f);
                float down = world_.raycastGeometry(from, glm::vec3(0, -1, 0), 3.0f);
                if (down < 3.0f) next.y = from.y - down + 0.005f;
                e->pos = next;
                b.walkPhase += speed * 1.7f * dt;
                b.walkAmt += (1.0f - b.walkAmt) * std::min(1.0f, 8.0f * dt);
            } else {
                // Wall/cover ahead: sidestep around it instead of grinding in.
                if (b.state == State::Advance) {
                    b.state = State::Strafe;
                    b.stateTimer = 0.4f + rf() * 0.4f;
                }
                b.strafeSign = -b.strafeSign;
                b.walkAmt += (0.0f - b.walkAmt) * std::min(1.0f, 8.0f * dt);
            }
        } else {
            b.walkAmt += (0.0f - b.walkAmt) * std::min(1.0f, 8.0f * dt);
        }

        // Smooth facing + ease the aim pose.
        float dyaw = desiredYaw - b.yaw;
        while (dyaw > 3.14159265f) dyaw -= 6.2831853f;
        while (dyaw < -3.14159265f) dyaw += 6.2831853f;
        b.yaw += dyaw * std::min(1.0f, 9.0f * dt);
        float aimTarget = aiming ? 1.0f : 0.0f;
        b.aimBlend += (aimTarget - b.aimBlend) * std::min(1.0f, 9.0f * dt);
    }
}

void Game::minicsEnemyFire(WorldEntity& e) {
    auto bit = minicsBrains_.find(e.id);
    if (bit == minicsBrains_.end()) return;
    const MinicsEnemy& b = bit->second;
    glm::vec3 facing(std::sin(b.yaw), 0.0f, -std::cos(b.yaw));
    glm::vec3 muzzle = e.pos + glm::vec3(0.0f, 1.08f, 0.0f) + facing * 0.42f;

    const glm::vec3 playerEye = eyePosition();
    glm::vec3 toEye = playerEye - muzzle;
    float dist = glm::length(toEye);
    glm::vec3 dir = dist > 0.01f ? toEye / dist : facing;

    // Accuracy falls off with range; a miss still whips a tracer past you so
    // incoming fire is always legible (and adds pressure without the damage).
    float acc = glm::clamp(0.90f - dist * 0.020f, 0.30f, 0.85f);
    bool hit = rand01() < acc;

    glm::vec3 impact = playerEye;
    if (!hit) {
        glm::vec3 right = glm::normalize(glm::cross(dir, glm::vec3(0, 1, 0)));
        glm::vec3 up = glm::normalize(glm::cross(right, dir));
        impact = playerEye + right * ((rand01() - 0.5f) * 2.2f) +
                 up * ((rand01() - 0.5f) * 1.4f + 0.4f);
    }

    addSparks(muzzle, 3, {1.0f, 0.7f, 0.25f}); // muzzle flash puff
    Tracer tr;
    tr.from = muzzle;
    tr.to = impact;
    tr.color = {1.0f, 0.34f, 0.20f}; // red = incoming
    tr.life = tr.total = std::max(0.02f, dist / 150.0f);
    tr.impact = false; // no surface puff (it ends on/near the player)
    tracers_.push_back(tr);
    sfx("enemy_shot", 0.9f + rand01() * 0.16f);

    if (hit) {
        glm::vec3 toShooter = e.pos - player_.pos;
        hurtAngle_ = std::atan2(toShooter.x, -toShooter.z) - yaw_; // screen-relative
        hurtTimer_ = 0.85f;
        damageFlash_ = std::max(damageFlash_, 0.5f);
        triggerShake(0.4f, 0.16f);
        damagePlayer(8.0f + rand01() * 5.0f);
    }
}

void Game::update(const InputState& in, double frameDt, int viewportW, int viewportH) {
    menuTime_ += frameDt;

    if (in.escapePressed) goBack();

    // B toggles the dev spawn/build menu, and Tab the inventory - both are
    // Sandbox features. Gun modes like MiniCS keep the player on the combat
    // path and never open the sandbox tools.
    if (in.spawnMenuPressed && inGame_ && sandboxMode_) {
        if (ui_ == UiScreen::None) {
            menuStatus_.clear();
            ui_ = UiScreen::SpawnMenu;
        } else if (ui_ == UiScreen::SpawnMenu) {
            menuStatus_.clear();
            ui_ = UiScreen::None;
        }
    }

    // Tab/I toggles the inventory/crafting screen in a gameplay context.
    if (in.inventoryPressed && inGame_ && sandboxMode_) {
        if (ui_ == UiScreen::None) {
            menuStatus_.clear();
            ui_ = UiScreen::Inventory;
        } else if (ui_ == UiScreen::Inventory) {
            menuStatus_.clear();
            ui_ = UiScreen::None;
        }
    }

    // T flips the camera, unless the world's camera policy pins it.
    if (in.cameraTogglePressed && inGame_ && ui_ == UiScreen::None) {
        if (settings_.cameraPolicy == CameraPolicy::AllowToggle) {
            settings_.thirdPerson = !settings_.thirdPerson;
            saveSettings();
            showToast(settings_.thirdPerson ? "THIRD PERSON" : "FIRST PERSON");
        } else {
            showToast("CAMERA IS LOCKED BY THIS WORLD'S SETTINGS");
        }
    }

    if (in.toggleHudPressed) {
        settings_.showDebugHud = !settings_.showDebugHud;
        saveSettings();
    }
    if (in.reloadConfigPressed) loadMovementConfig();

    if (frameDt > 0.0) {
        double fps = 1.0 / frameDt;
        fpsSmoothed_ = fpsSmoothed_ <= 0.0 ? fps : fpsSmoothed_ * 0.95 + fps * 0.05;
    }

    if (uiActive()) {
        // Server-address text entry (only the direct-connect screen has a field).
        if (ui_ == UiScreen::DirectConnect) {
            for (char c : in.typedText) {
                if (c >= 32 && c < 127 && serverAddress_.size() < kAddressMaxLen) {
                    serverAddress_ += c;
                }
            }
            if (in.backspacePressed && !serverAddress_.empty()) serverAddress_.pop_back();
            if (in.enterPressed) activateButton(UiButton::Id::Connect);
        }

        // World-name entry on the create screen; the folder name is derived
        // from this on create, so only sensible characters get in.
        if (ui_ == UiScreen::CreateWorld) {
            for (char c : in.typedText) {
                bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                          (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '_';
                if (ok && worldNameInput_.size() < kWorldNameMaxLen) worldNameInput_ += c;
            }
            if (in.backspacePressed && !worldNameInput_.empty()) worldNameInput_.pop_back();
            if (in.enterPressed) activateButton(UiButton::Id::ConfirmCreateWorld);
        }

        // Hover + click handling against this frame's layout.
        std::vector<UiButton> buttons = buildMenuLayout(viewportW, viewportH);
        hoveredButton_ = -1;
        for (size_t i = 0; i < buttons.size(); ++i) {
            if (buttons[i].enabled &&
                contains(in.mouseX, in.mouseY, buttons[i].x, buttons[i].y,
                         buttons[i].w, buttons[i].h)) {
                hoveredButton_ = int(i);
                break;
            }
        }
        if (in.mouseClicked && hoveredButton_ >= 0) {
            activateButton(buttons[size_t(hoveredButton_)].id,
                           buttons[size_t(hoveredButton_)].payload);
        }
        return; // the simulation is frozen while any UI is up
    }
    hoveredButton_ = -1;

    // Mouse look is per-frame for minimal latency; movement reads yaw at tick
    // time. 0.022 degrees per count matches the tactical-FPS convention.
    yaw_ += glm::radians(in.lookDx * 0.022f * settings_.mouseSensitivity);
    pitch_ -= glm::radians(in.lookDy * 0.022f * settings_.mouseSensitivity);
    pitch_ = glm::clamp(pitch_, glm::radians(-89.0f), glm::radians(89.0f));

    if (in.jumpPressed) jumpBufferTimer_ = cfg_.jumpBufferMs / 1000.0;
    walkHeldHud_ = in.walkHeld;

    // Impact pause: a clean hit freezes the world for a few hundredths of a
    // second. Mouse look stays realtime; the simulation and every combat
    // timer crawl through the same scaled clock.
    double simDt = frameDt;
    if (hitStop_ > 0.0f) {
        hitStop_ = std::max(0.0f, hitStop_ - float(frameDt));
        simDt *= 0.08;
    }
    const float dt = float(simDt);

    PlayerInput pin;
    pin.moveForward = in.moveForward;
    pin.moveRight = in.moveRight;
    pin.yaw = yaw_;
    pin.crouch = in.crouchHeld;
    // A raised shield weighs on you: guard up with a shield forces walk speed.
    pin.walk = in.walkHeld || (hasShield_ && melee::guardUp(playerMelee_));

    accumulator_ += simDt;
    int tickSafety = 0;
    while (accumulator_ >= tickDt_ && tickSafety++ < 10) {
        pin.jump = jumpBufferTimer_ > 0.0 || (cfg_.autoBhop != 0.0f && in.jumpHeld);
        prevPos_ = player_.pos;
        float fallSpeed = -player_.vel.y; // before the tick resolves the landing
        player_.tick(pin, world_, cfg_, float(tickDt_));
        world_.tick(float(tickDt_)); // entity respawns + hit-flash decay

        // Footsteps: distance-based cadence with a little pitch variation,
        // so walking naturally slows the rhythm and never machine-guns.
        if (player_.grounded) {
            footstepDistance_ += player_.horizontalSpeed() * float(tickDt_);
            if (footstepDistance_ >= 2.1f) {
                footstepDistance_ = 0.0f;
                if (player_.horizontalSpeed() > 0.5f) {
                    sfx("footstep", 0.92f + rand01() * 0.16f);
                }
            }
        }

        // Landing: a low thud and a brief viewmodel dip scaled by the fall.
        if (!wasGrounded_ && player_.grounded && fallSpeed > 3.0f) {
            sfx("footstep", 0.70f);
            landDipAmount_ = glm::clamp(fallSpeed / 14.0f, 0.25f, 1.0f);
            landDipTimer_ = 0.26f;
        }
        wasGrounded_ = player_.grounded;

        if (player_.justJumped) jumpBufferTimer_ = -1.0;
        jumpBufferTimer_ -= tickDt_;
        accumulator_ -= tickDt_;
    }
    if (tickSafety >= 10) accumulator_ = 0.0; // spiral-of-death guard

    float targetEye = player_.height(cfg_) - cfg_.eyeOffset;
    eyeHeight_ += (targetEye - eyeHeight_) * std::min(1.0f, cfg_.crouchTransitionSpeed * float(frameDt));

    // Combat and interaction (per frame, like mouse look, for minimal latency).
    attackCooldown_ = std::max(0.0f, attackCooldown_ - dt);
    muzzleFlashTimer_ = std::max(0.0f, muzzleFlashTimer_ - dt);
    hitMarkerTimer_ = std::max(0.0f, hitMarkerTimer_ - dt);
    killMarkerTimer_ = std::max(0.0f, killMarkerTimer_ - dt);
    hurtTimer_ = std::max(0.0f, hurtTimer_ - dt);
    shakeTime_ = std::max(0.0f, shakeTime_ - dt);
    damageFlash_ = std::max(0.0f, damageFlash_ - dt * 1.8f);
    if (hudToastTimer_ > 0.0) hudToastTimer_ -= frameDt;

    // Recoil recovery: the view-punch springs back toward the crosshair. Fast
    // enough to feel snappy, slow enough that fast fire visibly climbs first.
    float recover = std::min(1.0f, 11.0f * float(frameDt));
    recoilPitch_ -= recoilPitch_ * recover;
    recoilYaw_ -= recoilYaw_ * recover;

    // Reload: the magazine refills from reserve when the animation completes.
    if (reloadTimer_ > 0.0f) {
        reloadTimer_ = std::max(0.0f, reloadTimer_ - float(frameDt));
        if (reloadTimer_ == 0.0f) {
            int need = magSize_ - magAmmo_;
            int take = std::min(need, reserveAmmo_);
            magAmmo_ += take;
            reserveAmmo_ -= take;
        }
    }

    // Score popups drift up and fade.
    for (size_t i = 0; i < scorePopups_.size();) {
        scorePopups_[i].life -= float(frameDt);
        if (scorePopups_[i].life <= 0.0f) scorePopups_.erase(scorePopups_.begin() + long(i));
        else ++i;
    }

    // Health: slow regen once you have stayed out of trouble for a few
    // seconds (sandbox-friendly - a lost duel round is not a dead end),
    // and the shown bar eases toward the real value so damage reads as a
    // visible drain instead of a teleporting rectangle.
    sinceDamaged_ += dt;
    if (sinceDamaged_ > 5.0f && playerHealth_ < 100.0f) {
        playerHealth_ = std::min(100.0f, playerHealth_ + 8.0f * dt);
    }
    playerHealthShown_ += (playerHealth_ - playerHealthShown_)
                          * std::min(1.0f, 6.0f * float(frameDt));

    // Viewmodel bob: phase advances with distance so it locks to the
    // footstep cadence (one dip every ~2.1 m); intensity eases in and out
    // so stopping settles the hands instead of freezing them mid-offset.
    float hSpeed = player_.horizontalSpeed();
    bool bobbing = player_.grounded && hSpeed > 0.5f;
    bobIntensity_ += ((bobbing ? std::min(hSpeed / cfg_.runSpeed, 1.15f) : 0.0f) - bobIntensity_)
                     * std::min(1.0f, 8.0f * float(frameDt));
    if (bobbing) bobPhase_ += hSpeed * 1.5f * float(frameDt);

    // Look sway: the hands lag a beat behind the camera, then settle.
    float swayTX = glm::clamp(in.lookDx * 0.05f, -1.5f, 1.5f);
    float swayTY = glm::clamp(in.lookDy * 0.05f, -1.2f, 1.2f);
    swayX_ += (swayTX - swayX_) * std::min(1.0f, 10.0f * float(frameDt));
    swayY_ += (swayTY - swayY_) * std::min(1.0f, 10.0f * float(frameDt));
    landDipTimer_ = std::max(0.0f, landDipTimer_ - float(frameDt));
    if (controlHintTimer_ > 0.0) controlHintTimer_ -= frameDt;

    // Bullet tracers travel; on arrival the impact puff pops exactly where
    // the shot landed (a bright flash plus a few sparks).
    for (size_t i = 0; i < tracers_.size();) {
        tracers_[i].life -= dt;
        if (tracers_[i].life <= 0.0f) {
            if (tracers_[i].impact) {
                glm::vec3 at = tracers_[i].to;
                addSparks(at, 4, {0.95f, 0.85f, 0.55f});
                Spark flash;
                flash.pos = at;
                flash.color = {1.0f, 0.95f, 0.75f};
                flash.life = flash.total = 0.05f;
                flash.size = 3.0f; // tighter puff: less of a screen-filling blob
                sparks_.push_back(flash);
            }
            tracers_.erase(tracers_.begin() + long(i));
        } else {
            ++i;
        }
    }

    // Weapon switch (slot key or pickup) plays the raise animation and
    // cancels any in-flight attack from the previous weapon.
    if (inventory_.equippedId() != lastEquippedId_) {
        lastEquippedId_ = inventory_.equippedId();
        equipTimer_ = kEquipDuration;
        muzzleFlashTimer_ = 0.0f;
        recoilPitch_ = recoilYaw_ = 0.0f;
        reloadTimer_ = 0.0f;
        if (playerMelee_.phase != melee::Phase::Stagger) {
            playerMelee_.phase = melee::Phase::Idle;
            playerMelee_.phaseLeft = playerMelee_.phaseTotal = 0.0f;
            playerMelee_.comboDepth = 0;
        }
        // Raising a Glock racks a fresh magazine (and tops the reserve up).
        if (usesAmmo()) {
            magAmmo_ = magSize_;
            reserveAmmo_ = std::max(reserveAmmo_, 68);
        }
    }
    equipTimer_ = std::max(0.0f, equipTimer_ - float(frameDt));

    // MiniCS round: an end screen (Won/Lost) takes over the inputs - a press
    // restarts the round, ESC already popped to pause above. A short delay
    // stops the winning/losing click from instantly restarting.
    if (minicsActive_ &&
        (minicsPhase_ == MiniCSPhase::Won || minicsPhase_ == MiniCSPhase::Lost)) {
        updateMinics(dt);
        updateSparks(dt);
        bool press = in.attackPressed || in.jumpPressed || in.interactPressed ||
                     in.feintPressed;
        if (press && minicsPhaseTimer_ > 0.6f) {
            if (const GameMode* m = gamemodes::find("minics")) startGameMode(*m);
        }
        return; // no combat, aim, or weapon handling on the end screen
    }

    updateMelee(in, dt);      // stamina, attack phases, blocks, feints, throws
    updateBots(dt);           // duelist bot brains: telegraphs, strikes, guards
    updateMobs(dt);           // hostile mobs: chase + contact strikes
    updateMinics(dt);         // MiniCS: countdown/round timer, win/lose
    updateMinicsEnemies(dt);  // MiniCS enemy movement / aim / fire brains
    updateThrownWeapons(dt);  // thrown weapon arcs -> impacts -> pickups
    updateSparks(dt);

    updateCarriedProp(); // held prop follows the camera
    updateAimTarget();
    if (in.slotPressed > 0 && inventory_.selectSlot(in.slotPressed - 1)) {
        itemInv_.equippedItemId.clear(); // slot keys return to sandbox weapons
    }
    // R (or middle mouse) reloads while a Glock is in hand. In melee modes the
    // same input is the feint, so it is only claimed here for gun weapons.
    if (usesAmmo() && in.feintPressed) tryReload();
    if (in.interactPressed) tryInteract();
    if (in.attackPressed) tryAttack(); // hitscan only; melee ran in updateMelee
}

void Game::activateButton(UiButton::Id id, int payload) {
    using Id = UiButton::Id;
    sfx("ui_click");
    bool settingsChanged = false;
    switch (id) {
    // Navigation.
    case Id::OpenPlay: ui_ = UiScreen::Play; menuStatus_.clear(); break;
    case Id::OpenCreate: ui_ = UiScreen::Create; menuStatus_.clear(); break;
    case Id::OpenProfile: ui_ = UiScreen::Profile; menuStatus_.clear(); break;
    case Id::OpenSettings: settingsReturn_ = ui_; ui_ = UiScreen::Settings; break;
    case Id::NavBack: goBack(); break;
    case Id::QuitApp: quitRequested_ = true; break;

    // Play submenu.
    case Id::OpenGameModes: ui_ = UiScreen::GameModes; menuStatus_.clear(); break;
    case Id::OpenServers: ui_ = UiScreen::Servers; menuStatus_.clear(); break;
    case Id::OpenRecentGames: ui_ = UiScreen::RecentGames; menuStatus_.clear(); break;

    // Game modes: a mode is a game inside the game (map + ruleset + loadout).
    case Id::SelectGameMode: {
        const std::vector<GameMode>& modes = gamemodes::all();
        if (payload >= 0 && payload < int(modes.size())) {
            startGameMode(modes[size_t(payload)]);
        }
        break;
    }
    case Id::QuickLaunchRecent: {
        if (const GameMode* m = gamemodes::find(lastModeId_)) startGameMode(*m);
        break;
    }

    // Servers submenu. Real discovery/hosting arrives with the networking
    // milestone (protocol.h reserves v1); these are honest until it does.
    case Id::OpenServerList:
        menuStatus_.clear();
        ui_ = UiScreen::ServerList;
        break;
    case Id::OpenDirectConnect:
        menuStatus_.clear();
        ui_ = UiScreen::DirectConnect;
        break;
    case Id::HostLocalServer:
        menuStatus_ = "HOSTING NEEDS THE NETWORKING MILESTONE - PLAY GAME MODES LOCALLY FOR NOW";
        break;
    case Id::Connect:
    case Id::QuickLocalhost:
        if (id == Id::QuickLocalhost) serverAddress_ = "127.0.0.1:27015";
        menuStatus_ = "CONNECTION SYSTEM NOT IMPLEMENTED YET - NETWORKING IS A LATER MILESTONE";
        break;

    // Create submenu: maps and skins players make (Publish comes with online).
    case Id::OpenCreateMap:
        worldNameInput_.clear();
        createTemplateIndex_ = 0; // default to the first base map
        menuStatus_.clear();
        ui_ = UiScreen::CreateWorld;
        break;
    case Id::OpenEditMap:
        refreshWorldList();
        menuStatus_ = worldList_.empty() ? "NO SAVED MAPS YET - CREATE ONE FIRST" : "";
        ui_ = UiScreen::LoadWorld;
        break;
    case Id::OpenSkins:
        menuStatus_.clear();
        ui_ = UiScreen::Skins;
        break;
    case Id::OpenPublish:
        menuStatus_ = "PUBLISHING ARRIVES WITH ONLINE PLAY - MAPS AND SKINS SAVE LOCALLY FOR NOW";
        ui_ = UiScreen::Publish;
        break;

    // Create/edit map screens.
    case Id::SelectWorldType: createTemplateIndex_ = payload; break;
    case Id::ConfirmCreateWorld: createWorld(); break;
    case Id::LoadWorldEntry: loadWorld(payload); break;

    // Skins placeholder: pick a highlight color (saved with settings later).
    case Id::SelectSkinColor:
        skinColorIndex_ = payload;
        menuStatus_ = "SKIN PREVIEW ONLY - FULL SKIN SYSTEM IS COMING";
        break;

    // Pause menu.
    case Id::Resume: ui_ = UiScreen::None; break;
    case Id::QuitToMenu:
        dropCarriedProp(); // settle + persist a held prop before leaving
        if (carriedEntityId_ != 0) { // nowhere legal to drop: set it down as-is
            if (WorldEntity* held = world_.entityById(carriedEntityId_)) {
                held->carried = false;
            }
            carriedEntityId_ = 0;
        }
        saveCurrentWorld();
        currentWorldFile_.clear();
        minicsActive_ = false; // end any MiniCS round cleanly on the way out
        minicsBrains_.clear();
        inGame_ = false;
        ui_ = UiScreen::MainMenu;
        menuStatus_.clear();
        break;

    // Dev spawn menu.
    case Id::SelectSpawnCategory:
        spawnCategory_ = payload;
        menuStatus_.clear();
        break;
    case Id::SpawnEntity: spawnFromMenu(payload); break;
    case Id::CloseSpawnMenu: menuStatus_.clear(); ui_ = UiScreen::None; break;

    // Inventory / crafting screen.
    case Id::InvEquipItem: {
        if (payload < 0 || payload >= int(itemInv_.stacks.size())) break;
        const std::string itemId = itemInv_.stacks[size_t(payload)].itemId;
        const ItemDef* item = items_.find(itemId);
        if (!item || item->weaponId.empty()) break; // resources are not equippable
        if (itemInv_.equippedItemId == itemId) {
            itemInv_.equippedItemId.clear(); // click again to unequip
            menuStatus_ = "UNEQUIPPED " + toUpperAscii(item->displayName);
        } else {
            itemInv_.equippedItemId = itemId;
            menuStatus_ = "EQUIPPED " + toUpperAscii(item->displayName);
        }
        break;
    }
    case Id::CraftRecipe: {
        const std::vector<Recipe>& rs = recipes_.recipes();
        if (payload < 0 || payload >= int(rs.size())) break;
        const Recipe& r = rs[size_t(payload)];
        if (craft(itemInv_, r, items_)) {
            const ItemDef* out = items_.find(r.outputItemId);
            menuStatus_ = "CRAFTED " + toUpperAscii(out ? out->displayName : r.outputItemId);
            sfx("pickup");
        } else {
            menuStatus_ = "MISSING MATERIALS OR INVENTORY FULL";
        }
        break;
    }
    case Id::CloseInventory: menuStatus_.clear(); ui_ = UiScreen::None; break;

    // Settings.
    case Id::ResetDefaults: settings_ = GameSettings{}; settingsChanged = true; break;
    case Id::SensDown: settings_.mouseSensitivity -= 0.1f; settingsChanged = true; break;
    case Id::SensUp: settings_.mouseSensitivity += 0.1f; settingsChanged = true; break;
    case Id::FovDown: settings_.fovDegrees -= 2.0f; settingsChanged = true; break;
    case Id::FovUp: settings_.fovDegrees += 2.0f; settingsChanged = true; break;
    case Id::MasterDown: settings_.masterVolume -= 0.1f; settingsChanged = true; break;
    case Id::MasterUp: settings_.masterVolume += 0.1f; settingsChanged = true; break;
    case Id::MusicDown: settings_.musicVolume -= 0.1f; settingsChanged = true; break;
    case Id::MusicUp: settings_.musicVolume += 0.1f; settingsChanged = true; break;
    case Id::SfxDown: settings_.sfxVolume -= 0.1f; settingsChanged = true; break;
    case Id::SfxUp: settings_.sfxVolume += 0.1f; settingsChanged = true; break;
    case Id::ToggleFullscreen: settings_.fullscreen = !settings_.fullscreen; settingsChanged = true; break;
    case Id::ToggleVsync: settings_.vsync = !settings_.vsync; settingsChanged = true; break;
    case Id::ToggleHud: settings_.showDebugHud = !settings_.showDebugHud; settingsChanged = true; break;
    }
    if (settingsChanged) {
        settings_.clampRanges();
        settingsDirty_ = true;
        saveSettings();
    }
}

std::vector<Game::UiButton> Game::buildMenuLayout(int viewportW, int viewportH) const {
    using Id = UiButton::Id;
    std::vector<UiButton> buttons;
    const float cx = viewportW * 0.5f;

    auto column = [&](float startY, std::initializer_list<UiButton> list) {
        float y = startY;
        for (UiButton b : list) {
            b.x = cx - kBtnW / 2;
            b.y = y;
            b.w = kBtnW;
            b.h = kBtnH;
            buttons.push_back(std::move(b));
            y += kBtnGap;
        }
    };

    switch (ui_) {
    case UiScreen::MainMenu:
        column(viewportH * 0.36f, {
            {Id::OpenPlay, 0, 0, 0, 0, "PLAY", true},
            {Id::OpenCreate, 0, 0, 0, 0, "CREATE", true},
            {Id::OpenProfile, 0, 0, 0, 0, "PROFILE", true},
            {Id::OpenSettings, 0, 0, 0, 0, "SETTINGS", true},
            {Id::QuitApp, 0, 0, 0, 0, "QUIT", true},
        });
        break;

    case UiScreen::Play:
        column(viewportH * 0.40f, {
            {Id::OpenGameModes, 0, 0, 0, 0, "GAME MODES", true},
            {Id::OpenServers, 0, 0, 0, 0, "SERVERS", true},
            {Id::OpenRecentGames, 0, 0, 0, 0, "RECENT GAMES", true},
            {Id::NavBack, 0, 0, 0, 0, "BACK", true},
        });
        break;

    case UiScreen::GameModes: {
        // One wide card per mode; the tagline + status badge are drawn over
        // each rect in appendMenuDraws (same layout, keyed by index).
        const std::vector<GameMode>& modes = gamemodes::all();
        const float mW = kModeCardW, mH = kModeCardH, mGap = kModeCardH + 14.0f;
        float y = viewportH * 0.26f;
        for (int i = 0; i < int(modes.size()); ++i) {
            buttons.push_back({Id::SelectGameMode, cx - mW / 2, y, mW, mH,
                               toUpperAscii(modes[size_t(i)].displayName), true, i});
            y += mGap;
        }
        y += 8.0f;
        buttons.push_back({Id::NavBack, cx - kBtnW / 2, y, kBtnW, kBtnH, "BACK", true});
        break;
    }

    case UiScreen::Servers:
        column(viewportH * 0.40f, {
            {Id::OpenServerList, 0, 0, 0, 0, "SERVER LIST", true},
            {Id::OpenDirectConnect, 0, 0, 0, 0, "DIRECT CONNECT", true},
            {Id::HostLocalServer, 0, 0, 0, 0, "HOST LOCAL SERVER", true},
            {Id::NavBack, 0, 0, 0, 0, "BACK", true},
        });
        break;

    case UiScreen::ServerList:
        // No live discovery yet: the screen is an honest empty list + Back.
        column(viewportH * 0.66f, {
            {Id::NavBack, 0, 0, 0, 0, "BACK", true},
        });
        break;

    case UiScreen::RecentGames: {
        float y = viewportH * 0.42f;
        const GameMode* recent = gamemodes::find(lastModeId_);
        if (recent) {
            buttons.push_back({Id::QuickLaunchRecent, cx - kBtnW / 2, y, kBtnW, kBtnH,
                               "PLAY " + toUpperAscii(recent->displayName), true});
            y += kBtnGap;
        }
        buttons.push_back({Id::NavBack, cx - kBtnW / 2, y, kBtnW, kBtnH, "BACK", true});
        break;
    }

    case UiScreen::Create:
        column(viewportH * 0.36f, {
            {Id::OpenCreateMap, 0, 0, 0, 0, "CREATE MAP", true},
            {Id::OpenEditMap, 0, 0, 0, 0, "EDIT MAP", true},
            {Id::OpenSkins, 0, 0, 0, 0, "SKINS", true},
            {Id::OpenPublish, 0, 0, 0, 0, "PUBLISH", true},
            {Id::NavBack, 0, 0, 0, 0, "BACK", true},
        });
        break;

    case UiScreen::Skins: {
        // A row of highlight-color swatches (placeholder for the skin system).
        const int n = 6;
        const float sw = 92.0f, sh = 60.0f, sgap = 12.0f;
        float sx = cx - (n * sw + (n - 1) * sgap) / 2.0f;
        const float sy = viewportH * 0.40f;
        for (int i = 0; i < n; ++i) {
            buttons.push_back({Id::SelectSkinColor, sx, sy, sw, sh, "",
                               true, i, i == skinColorIndex_});
            sx += sw + sgap;
        }
        buttons.push_back({Id::NavBack, cx - kBtnW / 2, viewportH * 0.66f,
                           kBtnW, kBtnH, "BACK", true});
        break;
    }

    case UiScreen::Publish:
    case UiScreen::Profile:
        column(viewportH * 0.66f, {
            {Id::NavBack, 0, 0, 0, 0, "BACK", true},
        });
        break;

    case UiScreen::CreateWorld: {
        // A row of world-type tabs (highlight the selected one), then the
        // create/back column below the type description.
        const std::vector<WorldTemplate>& tmpls = worldgen::templates();
        const int n = int(tmpls.size());
        const float tabW = 128.0f, tabH = 40.0f, tabGap = 8.0f;
        float tabX = cx - (n * tabW + (n - 1) * tabGap) / 2.0f;
        const float tabY = viewportH * 0.40f;
        for (int i = 0; i < n; ++i) {
            // Short tab caption: the first word of the display name.
            std::string cap = tmpls[size_t(i)].displayName;
            size_t sp = cap.find(' ');
            if (sp != std::string::npos) cap.resize(sp);
            buttons.push_back({Id::SelectWorldType, tabX, tabY, tabW, tabH,
                               toUpperAscii(cap), true, i, i == createTemplateIndex_});
            tabX += tabW + tabGap;
        }
        column(viewportH * 0.58f, {
            {Id::ConfirmCreateWorld, 0, 0, 0, 0, "CREATE", true},
            {Id::NavBack, 0, 0, 0, 0, "BACK", true},
        });
        break;
    }

    case UiScreen::LoadWorld: {
        float y = viewportH * 0.30f;
        int shown = std::min(int(worldList_.size()), kMaxWorldButtons);
        for (int i = 0; i < shown; ++i) {
            buttons.push_back({Id::LoadWorldEntry, cx - kBtnW / 2, y, kBtnW, kBtnH,
                               toUpperAscii(worldList_[size_t(i)].displayName), true, i});
            y += kBtnGap;
        }
        y += 18.0f;
        buttons.push_back({Id::NavBack, cx - kBtnW / 2, y, kBtnW, kBtnH,
                           "BACK", true});
        break;
    }

    case UiScreen::SpawnMenu: {
        // GMod-style: a row of category tabs, entries of the open tab below.
        static const char* kTabNames[] = {"WEAPONS", "BOTS", "PROPS", "PICKUPS", "NODES"};
        constexpr int kTabCount = 5;
        const float tabW = 138.0f, tabH = 40.0f, tabGap = 10.0f;
        float tabX = cx - (kTabCount * tabW + (kTabCount - 1) * tabGap) / 2;
        const float tabY = viewportH * 0.26f;
        for (int i = 0; i < kTabCount; ++i) {
            buttons.push_back({Id::SelectSpawnCategory, tabX, tabY, tabW, tabH,
                               kTabNames[i], true, i, i == spawnCategory_});
            tabX += tabW + tabGap;
        }

        std::vector<const EntityDef*> defs = spawnableDefs(ContentCategory(spawnCategory_));
        float y = tabY + tabH + 26.0f;
        for (int i = 0; i < int(defs.size()); ++i) {
            buttons.push_back({Id::SpawnEntity, cx - kBtnW / 2, y, kBtnW, kBtnH,
                               toUpperAscii(defs[size_t(i)]->displayName), true, i});
            y += kBtnGap;
        }
        y += 18.0f;
        buttons.push_back({Id::CloseSpawnMenu, cx - kBtnW / 2, y, kBtnW, kBtnH,
                           "CLOSE", true});
        break;
    }

    case UiScreen::Inventory: {
        // Two columns: item stacks on the left (click a tool/weapon to
        // equip/unequip it), recipes on the right (greyed until affordable).
        const float rowW = 380.0f, rowH2 = 40.0f, rowGap = 8.0f;
        const float leftX = cx - rowW - 30.0f;
        const float rightX = cx + 30.0f;
        const float y0 = viewportH * 0.26f;

        float y = y0;
        for (int i = 0; i < int(itemInv_.stacks.size()); ++i) {
            const ItemStack& s = itemInv_.stacks[size_t(i)];
            const ItemDef* item = items_.find(s.itemId);
            std::string label = toUpperAscii(item ? item->displayName : s.itemId);
            if (s.count > 1) label += "  X " + std::to_string(s.count);
            bool equippable = item && !item->weaponId.empty();
            bool equipped = equippable && itemInv_.equippedItemId == s.itemId;
            if (equipped) label += "  [MAIN HAND]";
            buttons.push_back({Id::InvEquipItem, leftX, y, rowW, rowH2,
                               std::move(label), equippable, i, equipped});
            y += rowH2 + rowGap;
        }

        y = y0;
        for (int i = 0; i < int(recipes_.recipes().size()); ++i) {
            const Recipe& r = recipes_.recipes()[size_t(i)];
            const ItemDef* out = items_.find(r.outputItemId);
            std::string label = toUpperAscii(out ? out->displayName : r.outputItemId);
            label += "  (";
            for (size_t k = 0; k < r.inputs.size(); ++k) {
                const ItemDef* in = items_.find(r.inputs[k].itemId);
                if (k > 0) label += " + ";
                label += std::to_string(r.inputs[k].count) + " " +
                         toUpperAscii(in ? in->displayName : r.inputs[k].itemId);
            }
            label += ")";
            buttons.push_back({Id::CraftRecipe, rightX, y, rowW, rowH2,
                               std::move(label), canCraft(itemInv_, r), i});
            y += rowH2 + rowGap;
        }

        buttons.push_back({Id::CloseInventory, cx - kBtnW / 2, viewportH * 0.84f,
                           kBtnW, kBtnH, "CLOSE", true});
        break;
    }

    case UiScreen::DirectConnect:
        // One honest screen: a direct-connect address box and nothing that
        // pretends. Reached from Play -> Servers -> Direct Connect.
        column(viewportH * 0.44f, {
            {Id::Connect, 0, 0, 0, 0, "CONNECT", true},
            {Id::QuickLocalhost, 0, 0, 0, 0, "LOCALHOST 127.0.0.1", true},
            {Id::NavBack, 0, 0, 0, 0, "BACK", true},
        });
        break;

    case UiScreen::PauseMain:
        column(viewportH * 0.40f, {
            {Id::Resume, 0, 0, 0, 0, "RESUME", true},
            {Id::OpenSettings, 0, 0, 0, 0, "SETTINGS", true},
            {Id::QuitToMenu, 0, 0, 0, 0, "QUIT TO MENU", true},
        });
        break;

    case UiScreen::Settings: {
        // Settings rows: [-] value [+] for numbers, single ON/OFF button for
        // toggles. Row labels are plain text added in appendMenuDraws.
        float y = viewportH * 0.20f;
        auto stepRow = [&](Id down, Id up) {
            float by = y + (kRowH - kStepBtn) / 2;
            buttons.push_back({down, cx + 30, by, kStepBtn, kStepBtn, "-", true});
            buttons.push_back({up, cx + 240, by, kStepBtn, kStepBtn, "+", true});
            y += kRowH;
        };
        auto toggleRow = [&](Id id, bool on) {
            float by = y + (kRowH - kStepBtn) / 2;
            buttons.push_back({id, cx + 30, by, 140, kStepBtn, on ? "ON" : "OFF", true});
            y += kRowH;
        };

        stepRow(Id::SensDown, Id::SensUp);
        stepRow(Id::FovDown, Id::FovUp);
        stepRow(Id::MasterDown, Id::MasterUp);
        stepRow(Id::MusicDown, Id::MusicUp);
        stepRow(Id::SfxDown, Id::SfxUp);
        toggleRow(Id::ToggleFullscreen, settings_.fullscreen);
        toggleRow(Id::ToggleVsync, settings_.vsync);
        toggleRow(Id::ToggleHud, settings_.showDebugHud);

        y += 18.0f;
        buttons.push_back({Id::ResetDefaults, cx - kBtnW / 2, y, kBtnW, kBtnH,
                           "RESET TO DEFAULTS", true});
        y += kBtnGap;
        buttons.push_back({Id::NavBack, cx - kBtnW / 2, y, kBtnW, kBtnH, "BACK", true});
        break;
    }

    case UiScreen::None:
        break;
    }
    return buttons;
}

RenderFrame Game::buildRenderFrame(int viewportW, int viewportH) const {
    RenderFrame frame;
    frame.viewportW = viewportW;
    frame.viewportH = viewportH;

    // Per-mode atmosphere: MiniCS (and Deathmatch, same map) get a dark neon
    // dusk so the emissive accents, tracers and muzzle flashes glow; every
    // other world keeps the daytime defaults baked into RenderFrame.
    if (currentWorldType_ == "minics_arena") {
        frame.skyTop = {0.04f, 0.05f, 0.11f};
        frame.skyHorizon = {0.24f, 0.09f, 0.30f};
        frame.fogColor = {0.10f, 0.07f, 0.16f};
        frame.fogScale = 70.0f;
        frame.fogMax = 0.55f;
    }

    glm::vec3 playerRenderPos = player_.pos; // interpolated below; body draw uses it
    if (inGame_) {
        // Camera: interpolate between the last two ticks.
        float alpha = float(std::clamp(accumulator_ / tickDt_, 0.0, 1.0));
        glm::vec3 renderPos = glm::mix(prevPos_, player_.pos, alpha);
        playerRenderPos = renderPos;
        glm::vec3 eye = renderPos + glm::vec3(0.0f, eyeHeight_, 0.0f);
        // Screen shake: decaying high-frequency wobble on the eye point.
        if (shakeTime_ > 0.0f) {
            float f = shakeAmp_ * (shakeTime_ / shakeTotal_) * 0.035f;
            float t = float(menuTime_);
            eye += glm::vec3(std::sin(t * 61.0f), std::sin(t * 47.3f + 2.1f),
                             std::sin(t * 53.7f + 4.2f)) *
                   f;
        }
        glm::vec3 lookDir = lookDirection(); // includes the recoil view-punch
        if (thirdPersonActive()) {
            // Boom camera: pull back along the look ray (slightly raised),
            // snubbed against geometry so walls never occlude the player.
            glm::vec3 boom = -lookDir * kThirdPersonDist + glm::vec3(0.0f, 0.35f, 0.0f);
            float boomLen = glm::length(boom);
            glm::vec3 boomDir = boom / boomLen;
            float t = world_.raycastGeometry(eye, boomDir, boomLen);
            glm::vec3 camPos = eye + boomDir * std::max(0.5f, t - 0.25f);
            frame.view = glm::lookAt(camPos, eye + lookDir * 1.5f,
                                     glm::vec3(0.0f, 1.0f, 0.0f));
        } else {
            frame.view = glm::lookAt(eye, eye + lookDir, glm::vec3(0.0f, 1.0f, 0.0f));
        }
    } else {
        // Menu backdrop: a slow orbit over the arena.
        float t = float(menuTime_) * 0.08f;
        glm::vec3 eye(std::sin(t) * 16.0f, 9.0f, std::cos(t) * 16.0f);
        frame.view = glm::lookAt(eye, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    float aspect = viewportH > 0 ? float(viewportW) / float(viewportH) : 1.0f;
    frame.proj = glm::perspective(glm::radians(settings_.fovDegrees), aspect, 0.05f, 300.0f);
    // Fixed FOV for the first-person viewmodel so hands keep their size and
    // shape no matter what world FOV the player runs.
    frame.viewmodelProj = glm::perspective(glm::radians(58.0f), aspect, 0.02f, 8.0f);

    // World geometry.
    frame.boxes.reserve(world_.boxes().size() + world_.entities().size());
    for (const WorldBox& wb : world_.boxes()) {
        BoxDraw draw;
        draw.center = (wb.box.min + wb.box.max) * 0.5f;
        draw.size = wb.box.max - wb.box.min;
        draw.color = wb.color;
        draw.checkerTop = wb.checkerTop;
        draw.emissive = wb.emissive; // neon/LED accent strips glow through fog
        frame.boxes.push_back(draw);
    }

    // World-space oriented boxes: spinning weapon pickups, duelist bot
    // weapon telegraphs, thrown weapons tumbling through the air, sparks,
    // bullet tracers.
    auto obox = [&frame](const glm::mat4& parent, glm::vec3 pos, glm::vec3 rotDeg,
                         glm::vec3 size, glm::vec3 color, float emissive = 0.0f) {
        ViewmodelBoxDraw d;
        d.transform = glm::scale(vmCompose(parent, pos, rotDeg), size);
        d.color = color;
        d.emissive = emissive;
        frame.orientedBoxes.push_back(d);
    };

    // Spawned sandbox entities (pickups, bots, props). Multi-part visuals
    // come from the content def; collision stays the plain entity AABB.
    for (size_t ei = 0; ei < world_.entities().size(); ++ei) {
        const WorldEntity& e = world_.entities()[ei];
        if (!e.active) continue; // picked up / destroyed, awaiting respawn
        if (minicsBrains_.count(e.id)) continue; // MiniCS enemies draw animated

        glm::vec3 base = e.pos;
        bool isItemLike = !e.weaponId.empty() || !e.itemId.empty();
        if (isItemLike) {
            // Weapon pickups and item drops hover and bob so they read as
            // things to grab instead of stray level geometry.
            base.y += 0.06f + 0.04f * float(std::sin(menuTime_ * 2.5 + double(ei) * 1.7));
        }
        float flash = glm::clamp(e.hitFlash / 0.15f, 0.0f, 1.0f);

        // Impact reaction: damageable entities visibly flinch away from the
        // player on a hit, and a staggered duelist reels back for the whole
        // stagger - the target reacts like something was driven into it.
        glm::vec3 lean(0.0f);
        if (e.maxHealth > 0.0f) {
            glm::vec3 away = e.pos - player_.pos;
            away.y = 0.0f;
            float len = glm::length(away);
            if (len > 0.01f) {
                away /= len;
                lean = away * (0.16f * flash);
                auto botIt = bots_.find(e.id);
                if (botIt != bots_.end() &&
                    botIt->second.actor.phase == melee::Phase::Stagger) {
                    const melee::Actor& B = botIt->second.actor;
                    float reel = B.phaseTotal > 0.0f ? B.phaseLeft / B.phaseTotal : 0.0f;
                    lean += away * (0.22f * reel);
                }
            }
        }

        const EntityDef* def = content_.find(e.defId);
        if (def && !def->visual.empty() && isItemLike) {
            // Item-like entities hover (base bob above) AND slowly spin.
            float spinDeg = float(std::fmod(menuTime_ * 42.0 + double(e.id) * 47.0, 360.0));
            glm::mat4 group = vmCompose(glm::translate(glm::mat4(1.0f), base),
                                        glm::vec3(0.0f), {0.0f, spinDeg, 0.0f});
            for (const VisualPart& part : def->visual) {
                obox(group, part.offset, {0, 0, 0}, part.size, part.color, part.emissive);
            }
        } else if (def && !def->visual.empty()) {
            float entityH = std::max(0.2f, e.size.y);
            for (const VisualPart& part : def->visual) {
                BoxDraw draw;
                // Taller parts lean further: the whole stack tips, not slides.
                draw.center = base + part.offset + lean * (part.offset.y / entityH);
                draw.size = part.size;
                draw.color = glm::mix(part.color, glm::vec3(1.0f, 0.15f, 0.1f), flash);
                draw.emissive = part.emissive; // glowing LED/neon parts
                frame.boxes.push_back(draw);
            }
        } else {
            AABB b = e.bounds();
            BoxDraw draw;
            draw.center = (b.min + b.max) * 0.5f + (base - e.pos) + lean * 0.5f;
            draw.size = b.max - b.min;
            draw.color = glm::mix(e.color, glm::vec3(1.0f, 0.15f, 0.1f), flash);
            frame.boxes.push_back(draw);
        }
    }

    // Bot blades: the telegraph IS the animation. The blade winds to the
    // side the cut will come from (that side + the windup cue is what you
    // parry or match for a perfect counter), sweeps to the strike point,
    // then settles - or lies across the front while its guard is up.
    for (const auto& kv : bots_) {
        const WorldEntity* e = world_.entityById(kv.first);
        if (!e || !e->active) continue;
        const melee::Actor& B = kv.second.actor;

        glm::vec3 toPlayer = player_.pos - e->pos;
        float yawDeg = glm::degrees(std::atan2(toPlayer.x, -toPlayer.z));
        glm::mat4 group = vmCompose(glm::translate(glm::mat4(1.0f), e->pos),
                                    glm::vec3(0.0f), {0.0f, yawDeg, 0.0f});

        struct Pose {
            glm::vec3 p;
            glm::vec3 r;
        };
        const Pose idleP{{0.42f, 1.05f, -0.15f}, {25.0f, 0.0f, 0.0f}};
        const Pose guardP{{0.0f, 1.20f, -0.50f}, {0.0f, 90.0f, 0.0f}};
        const Pose strikeP{{0.0f, 1.30f, -0.60f}, {-6.0f, 0.0f, 0.0f}};
        Pose windP;
        switch (B.attack) {
        case melee::Attack::SlashLeft: windP = {{-0.80f, 1.40f, 0.05f}, {12.0f, -55.0f, 0.0f}}; break;
        case melee::Attack::SlashRight: windP = {{0.80f, 1.40f, 0.05f}, {12.0f, 55.0f, 0.0f}}; break;
        case melee::Attack::Overhead: windP = {{0.10f, 2.05f, 0.25f}, {55.0f, 0.0f, 0.0f}}; break;
        default: windP = {{0.30f, 1.25f, 0.45f}, {4.0f, 0.0f, 0.0f}}; break; // Stab
        }

        float prog = melee::phaseProgress(B);
        Pose pose = idleP;
        glm::vec3 bladeColor(0.72f, 0.76f, 0.82f);
        float glow = 0.0f;
        switch (B.phase) {
        case melee::Phase::Windup: {
            float ease = 1.0f - (1.0f - prog) * (1.0f - prog);
            pose.p = glm::mix(idleP.p, windP.p, ease);
            pose.r = glm::mix(idleP.r, windP.r, ease);
            bladeColor = glm::mix(bladeColor, glm::vec3(1.0f, 0.72f, 0.25f), prog);
            glow = 0.25f * prog; // brightening amber = incoming
            break;
        }
        case melee::Phase::Active:
            pose.p = glm::mix(windP.p, strikeP.p, prog);
            pose.r = glm::mix(windP.r, strikeP.r, prog);
            bladeColor = {1.0f, 0.35f, 0.25f};
            glow = 0.45f;
            break;
        case melee::Phase::Recovery:
            pose.p = glm::mix(strikeP.p, idleP.p, prog);
            pose.r = glm::mix(strikeP.r, idleP.r, prog);
            break;
        case melee::Phase::Stagger:
            pose = {{0.55f, 0.70f, -0.20f}, {60.0f, 15.0f, 30.0f}};
            bladeColor = {0.5f, 0.52f, 0.58f};
            break;
        case melee::Phase::Idle:
            if (melee::guardUp(B)) pose = guardP;
            break;
        }

        glm::mat4 hand = vmCompose(group, pose.p, pose.r);
        obox(hand, {0.0f, 0.0f, 0.0f}, {0, 0, 0}, {0.16f, 0.16f, 0.16f},
             {0.36f, 0.38f, 0.44f});                              // fist/gauntlet
        obox(hand, {0.0f, 0.0f, -0.55f}, {0, 0, 0}, {0.05f, 0.09f, 0.95f},
             bladeColor, glow);                                   // blade
    }

    // Thrown weapons: the pickup's own visual parts, tumbling end over end.
    for (const ThrownWeapon& t : thrown_) {
        glm::vec3 flat(t.vel.x, 0.0f, t.vel.z);
        float yawDeg = glm::length(flat) > 0.01f
                           ? glm::degrees(std::atan2(flat.x, -flat.z))
                           : 0.0f;
        glm::mat4 group = vmCompose(glm::translate(glm::mat4(1.0f), t.pos),
                                    glm::vec3(0.0f),
                                    {glm::degrees(t.spin), yawDeg, 0.0f});
        const EntityDef* def = content_.find(t.weaponId);
        if (def && !def->visual.empty()) {
            for (const VisualPart& part : def->visual) {
                obox(group, part.offset - glm::vec3(0.0f, 0.1f, 0.0f), {0, 0, 0},
                     part.size, part.color);
            }
        } else {
            obox(group, {0, 0, 0}, {0, 0, 0}, {0.3f, 0.05f, 0.05f},
                 {0.7f, 0.7f, 0.75f});
        }
    }

    // Sparks: tiny unlit cubes that shrink as they die.
    for (const Spark& p : sparks_) {
        float sz = (0.008f + 0.028f * (p.life / p.total)) * p.size;
        obox(glm::translate(glm::mat4(1.0f), p.pos), glm::vec3(0.0f),
             glm::vec3(0.0f), {sz, sz, sz}, p.color, 1.0f);
    }

    // Bullet tracers: a short bright streak flying from muzzle to impact.
    // Only the traveled part is ever drawn, so nothing smears across the
    // screen or hangs in the world.
    for (const Tracer& t : tracers_) {
        glm::vec3 seg = t.to - t.from;
        float len = glm::length(seg);
        if (len < 0.05f) continue;
        glm::vec3 d = seg / len;
        float progress = glm::clamp(1.0f - t.life / t.total, 0.0f, 1.0f);
        glm::vec3 head = t.from + d * (len * progress);
        float streak = std::min(3.6f, len * progress); // never longer than traveled
        if (streak < 0.05f) continue;
        glm::vec3 tail = head - d * streak;
        float yawDeg = glm::degrees(std::atan2(d.x, -d.z));
        float pitchDeg = glm::degrees(std::asin(glm::clamp(d.y, -1.0f, 1.0f)));
        glm::mat4 group =
            vmCompose(glm::translate(glm::mat4(1.0f), (head + tail) * 0.5f),
                      glm::vec3(0.0f), {pitchDeg, yawDeg, 0.0f});
        // A slim, long streak: thin enough that firing along your own view ray
        // reads as a dash instead of a fat square, long enough to survive the
        // near-parallel projection. A small bright head sells the "bullet".
        // The color is per-tracer: warm gold for the player, red for incoming.
        obox(group, {0, 0, 0}, {0, 0, 0}, {0.03f, 0.03f, streak}, t.color, 1.0f);
        glm::vec3 headCol = glm::mix(t.color, glm::vec3(1.0f), 0.5f);
        glm::mat4 headM = glm::translate(glm::mat4(1.0f), head);
        obox(headM, {0, 0, 0}, {0, 0, 0}, {0.07f, 0.07f, 0.10f}, headCol, 1.0f);
    }

    // MiniCS enemies: animated (walk / aim / shoot / flinch) crimson soldiers.
    if (minicsActive_) appendMinicsEnemyDraws(frame);

    if (uiActive()) {
        appendMenuDraws(frame);
    } else {
        appendHudDraws(frame);
    }
    return frame;
}

void Game::appendHudDraws(RenderFrame& frame) const {
    const int viewportW = frame.viewportW;
    const int viewportH = frame.viewportH;
    const glm::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
    const glm::vec4 dim(0.8f, 0.85f, 0.9f, 1.0f);
    const float s = 2.0f;
    const float lineH = 20.0f;
    float ty = 12.0f;
    char buf[256];

    auto addLine = [&](const glm::vec4& color, const char* text) {
        TextDraw t;
        t.x = 12.0f;
        t.y = ty;
        t.scale = s;
        t.color = color;
        t.text = text;
        frame.texts.push_back(std::move(t));
        ty += lineH;
    };

    // Red vignette while the player was just struck (fades in update).
    if (damageFlash_ > 0.0f) {
        frame.rects.push_back({0.0f, 0.0f, float(viewportW), float(viewportH),
                               glm::vec4(0.85f, 0.10f, 0.08f, damageFlash_ * 0.38f)});
    }

    // MiniCS end screen owns the whole HUD (no crosshair, viewmodel, or bars).
    if (minicsActive_ && (minicsPhase_ == MiniCSPhase::Won ||
                          minicsPhase_ == MiniCSPhase::Lost)) {
        appendMinicsHud(frame);
        return;
    }

    if (settings_.showDebugHud) {
        std::snprintf(buf, sizeof(buf), "SPEED %6.2f M/S", player_.horizontalSpeed());
        addLine(white, buf);
        std::snprintf(buf, sizeof(buf), "VEL X %+6.2f  Y %+6.2f  Z %+6.2f",
                      player_.vel.x, player_.vel.y, player_.vel.z);
        addLine(dim, buf);
        std::snprintf(buf, sizeof(buf), "POS X %+7.2f  Y %+7.2f  Z %+7.2f",
                      player_.pos.x, player_.pos.y, player_.pos.z);
        addLine(dim, buf);

        const char* mode;
        bool moving = player_.horizontalSpeed() > 0.1f;
        if (!player_.grounded) mode = player_.crouched ? "AIR CROUCH" : "AIR";
        else if (player_.crouched) mode = "CROUCH";
        else if (walkHeldHud_ && moving) mode = "WALK";
        else mode = "RUN";
        std::snprintf(buf, sizeof(buf), "GROUNDED %s   MODE %s",
                      player_.grounded ? "YES" : "NO", mode);
        addLine(white, buf);
        std::snprintf(buf, sizeof(buf), "FPS %5.0f   TICK %d HZ   RENDERER %s",
                      fpsSmoothed_, int(cfg_.tickRate), rendererName_.c_str());
        addLine(dim, buf);
        std::snprintf(buf, sizeof(buf), "WORLD %s%s   ENTITIES %d",
                      toUpperAscii(currentWorldName_).c_str(),
                      currentWorldFile_.empty() ? " (NOT SAVED)" : "",
                      int(world_.entities().size()));
        addLine(dim, buf);
        const char* phaseName;
        switch (playerMelee_.phase) {
        case melee::Phase::Windup: phaseName = "WINDUP"; break;
        case melee::Phase::Active: phaseName = "ACTIVE"; break;
        case melee::Phase::Recovery: phaseName = "RECOVERY"; break;
        case melee::Phase::Stagger: phaseName = "STAGGER"; break;
        default: phaseName = "IDLE"; break;
        }
        std::snprintf(buf, sizeof(buf), "STAMINA %3.0f   PHASE %s%s   COMBO %d   RIPOSTE %.1f",
                      playerMelee_.stamina, phaseName,
                      playerMelee_.heavy ? " (HEAVY)" : "",
                      playerMelee_.comboDepth, playerMelee_.riposteWindow);
        addLine(white, buf);
        addLine(glm::vec4(0.7f, 0.75f, 0.8f, 1.0f),
                "B SPAWN  E USE  LMB SLASH (HOLD=HEAVY)  RMB BLOCK  WHEEL OVH/STAB");
        addLine(glm::vec4(0.7f, 0.75f, 0.8f, 1.0f),
                "ALT ALT-SLASH  V JAB  F KICK  R FEINT  Q THROW  1-4 WEAPONS  F1 HUD");

        // Big speedometer under the crosshair; green when above run speed
        // (i.e. you gained speed through air-strafing).
        float hSpeed = player_.horizontalSpeed();
        TextDraw speedo;
        speedo.centered = true;
        speedo.x = viewportW * 0.5f;
        speedo.y = viewportH * 0.60f;
        speedo.scale = 3.0f;
        speedo.color = hSpeed > cfg_.runSpeed + 0.05f ? glm::vec4(0.45f, 1.0f, 0.5f, 1.0f) : white;
        std::snprintf(buf, sizeof(buf), "%.1f", hSpeed);
        speedo.text = buf;
        frame.texts.push_back(std::move(speedo));
    }

    // Crosshair: four thin drawn arms with a gap (not a font glyph).
    {
        const glm::vec4 chCol(1.0f, 1.0f, 1.0f, 0.85f);
        const float chX = viewportW * 0.5f, chY = viewportH * 0.5f;
        const float gap = 5.0f, arm = 7.0f, th = 2.0f;
        frame.rects.push_back({chX - gap - arm, chY - th * 0.5f, arm, th, chCol});
        frame.rects.push_back({chX + gap, chY - th * 0.5f, arm, th, chCol});
        frame.rects.push_back({chX - th * 0.5f, chY - gap - arm, th, arm, chCol});
        frame.rects.push_back({chX - th * 0.5f, chY + gap, th, arm, chCol});
    }

    const float cx = viewportW * 0.5f;
    auto centeredLine = [&](float y, float scale, const glm::vec4& color, std::string text) {
        TextDraw t;
        t.centered = true;
        t.x = cx;
        t.y = y;
        t.scale = scale;
        t.color = color;
        t.text = std::move(text);
        frame.texts.push_back(std::move(t));
    };

    // Riposte window: an unmissable prompt - this is the read->react->punish
    // loop's "punish now" beat.
    if (playerMelee_.riposteWindow > 0.0f) {
        centeredLine(viewportH * 0.5f - 48.0f, 2.5f,
                     glm::vec4(1.0f, 0.9f, 0.35f, 0.95f), "RIPOSTE!");
    }

    // Hit marker: four small squares around the crosshair while a hit
    // confirmation is live.
    if (hitMarkerTimer_ > 0.0f) {
        const glm::vec4 red(1.0f, 0.25f, 0.2f, 0.95f);
        const float chY = viewportH * 0.5f;
        for (float sx : {-1.0f, 1.0f}) {
            for (float sy : {-1.0f, 1.0f}) {
                frame.rects.push_back({cx + sx * 13.0f - 2.5f, chY + sy * 13.0f - 2.5f,
                                       5.0f, 5.0f, red});
            }
        }
    }

    // First-person hands + held weapon (simple shapes; no art pipeline yet).
    appendViewmodelDraws(frame);

    // Interaction prompt / target readout under the crosshair.
    char info[128];
    const glm::vec4 promptGold(1.0f, 0.9f, 0.5f, 1.0f);
    if (carriedEntityId_ != 0) {
        const WorldEntity* held = world_.entityById(carriedEntityId_);
        std::snprintf(info, sizeof(info), "CARRYING %s - PRESS E TO DROP",
                      toUpperAscii(held ? held->defId : "PROP").c_str());
        centeredLine(viewportH * 0.5f + 34.0f, 2.0f, promptGold, info);
    } else if (aimInteract_.index >= 0) {
        const WorldEntity& e = world_.entities()[size_t(aimInteract_.index)];
        if (!e.weaponId.empty()) {
            const WeaponDef* w = content_.findWeapon(e.weaponId);
            std::snprintf(info, sizeof(info), "PRESS E TO PICK UP %s",
                          toUpperAscii(w ? w->displayName : e.weaponId).c_str());
        } else {
            const EntityDef* def = content_.find(e.defId);
            std::snprintf(info, sizeof(info), "PRESS E TO CARRY %s",
                          toUpperAscii(def ? def->displayName : e.defId).c_str());
        }
        centeredLine(viewportH * 0.5f + 34.0f, 2.0f, promptGold, info);
    } else if (aimInfo_.index >= 0) {
        const WorldEntity& e = world_.entities()[size_t(aimInfo_.index)];
        const EntityDef* def = content_.find(e.defId);
        auto botIt = bots_.find(e.id);
        if (botIt != bots_.end()) {
            const melee::Actor& B = botIt->second.actor;
            char state[48] = "";
            if (B.phase == melee::Phase::Stagger) {
                std::snprintf(state, sizeof(state), "  STAGGERED");
            } else if (melee::guardUp(B)) {
                std::snprintf(state, sizeof(state), "  GUARDING");
            } else if (B.phase == melee::Phase::Windup) {
                // Naming the incoming attack teaches the counter language:
                // answer it with the same attack to perfect-counter.
                std::snprintf(state, sizeof(state), "  WINDING UP %s",
                              attackName(B.attack));
            }
            std::snprintf(info, sizeof(info), "%s  %d/%d%s",
                          toUpperAscii(def ? def->displayName : e.defId).c_str(),
                          int(std::ceil(e.health)), int(e.maxHealth), state);
        } else {
            std::snprintf(info, sizeof(info), "%s  %d/%d",
                          toUpperAscii(def ? def->displayName : e.defId).c_str(),
                          int(std::ceil(e.health)), int(e.maxHealth));
        }
        centeredLine(viewportH * 0.5f + 34.0f, 2.0f, dim, info);
    }

    // Transient gameplay message (pickups, kills).
    if (hudToastTimer_ > 0.0 && !hudToast_.empty()) {
        centeredLine(viewportH * 0.5f + 60.0f, 2.0f, glm::vec4(0.6f, 1.0f, 0.65f, 1.0f),
                     hudToast_);
    }

    // First-spawn control hint: a few seconds of "here's how to play" so a
    // fresh player never needs the debug HUD to find the keys. The hint is
    // mode-aware - a gun mode never advertises the melee/build keys, and the
    // build menu only shows in Sandbox (its home), keeping the dev tools off
    // the normal player path.
    if (controlHintTimer_ > 0.0) {
        float a = float(glm::clamp(controlHintTimer_ / 1.5, 0.0, 1.0)); // fade out
        const WeaponDef* hw = content_.findWeapon(heldWeaponId());
        const char* hint;
        if (sandboxMode_) {
            hint = "WASD MOVE   E INTERACT   LMB ATTACK   TAB INVENTORY   B BUILD MENU   ESC MENU";
        } else if (hw && hw->kind == WeaponKind::Hitscan) {
            hint = "WASD MOVE   MOUSE AIM   LMB FIRE   R RELOAD   ESC MENU";
        } else {
            hint = "WASD MOVE   E INTERACT   LMB ATTACK   RMB BLOCK   ESC MENU";
        }
        // A subtle mode banner above the controls, so you always know what
        // you dropped into (and whether it's an early mode).
        if (!currentModeName_.empty()) {
            std::string banner = currentModeName_;
            if (activeModeStatus_ == ModeStatus::Preview) banner += "  -  EARLY ACCESS";
            centeredLine(viewportH - 122.0f, 2.0f,
                         glm::vec4(0.62f, 0.78f, 1.0f, a * 0.9f), banner);
        }
        centeredLine(viewportH - 96.0f, 2.0f, glm::vec4(0.9f, 0.93f, 1.0f, a), hint);
    }

    // Health bar, bottom-left: ghost bar (recent damage) trails behind the
    // fill, the fill flashes white on the hit itself, and the color walks
    // green -> amber -> red as it empties.
    {
        const float hw = 240.0f, hh = 14.0f;
        const float hx = 24.0f, hy = viewportH - 46.0f;
        frame.rects.push_back({hx - 3.0f, hy - 3.0f, hw + 6.0f, hh + 6.0f,
                               glm::vec4(0.05f, 0.07f, 0.10f, 0.85f)});
        float frac = playerHealth_ / 100.0f;
        float shownFrac = glm::clamp(playerHealthShown_ / 100.0f, 0.0f, 1.0f);
        if (shownFrac > frac) { // draining ghost
            frame.rects.push_back({hx, hy, hw * shownFrac, hh,
                                   glm::vec4(0.75f, 0.20f, 0.15f, 0.75f)});
        }
        glm::vec4 col = frac > 0.6f   ? glm::vec4(0.42f, 0.85f, 0.40f, 0.95f)
                        : frac > 0.3f ? glm::vec4(0.95f, 0.75f, 0.25f, 0.95f)
                                      : glm::vec4(0.92f, 0.25f, 0.18f, 0.95f);
        if (sinceDamaged_ < 0.15f) col = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        frame.rects.push_back({hx, hy, hw * frac, hh, col});

        std::snprintf(buf, sizeof(buf), "%d", int(std::ceil(playerHealth_)));
        TextDraw hp;
        hp.x = hx + hw + 12.0f;
        hp.y = hy - 3.0f;
        hp.scale = 2.5f;
        hp.color = white;
        hp.text = buf;
        frame.texts.push_back(std::move(hp));
    }

    // Stamina bar: the melee resource. Only shown when a melee weapon is
    // in hand (the pistol does not spend it). Green when healthy, red and
    // angrier once low stamina starts weakening the guard.
    const WeaponDef* equippedW = content_.findWeapon(inventory_.equippedId());
    if (equippedW && equippedW->kind == WeaponKind::Melee) {
        const float sw = 260.0f, sh = 8.0f;
        const float sx = cx - sw / 2.0f, sy = viewportH - 70.0f;
        frame.rects.push_back({sx - 2.0f, sy - 2.0f, sw + 4.0f, sh + 4.0f,
                               glm::vec4(0.06f, 0.08f, 0.11f, 0.85f)});
        float frac = playerMelee_.stamina / melee::kMaxStamina;
        glm::vec4 col = playerMelee_.stamina < melee::kLowStamina
                            ? glm::vec4(0.95f, 0.35f, 0.25f, 0.95f)
                            : glm::vec4(0.55f, 0.92f, 0.60f, 0.90f);
        if (playerMelee_.phase == melee::Phase::Stagger) {
            col = glm::vec4(1.0f, 0.55f, 0.15f, 0.95f); // reeling: guard is gone
        }
        frame.rects.push_back({sx, sy, sw * frac, sh, col});
    }

    // Weapon bar: owned weapons with the equipped one bracketed. MiniCS runs
    // a single loadout and its own ammo HUD, so it hides the slot bar.
    if (!minicsActive_) {
        std::string bar;
        for (size_t i = 0; i < inventory_.weapons.size(); ++i) {
            const WeaponDef* w = content_.findWeapon(inventory_.weapons[i]);
            std::string name = toUpperAscii(w ? w->displayName : inventory_.weapons[i]);
            bool eq = int(i) == inventory_.equipped;
            if (!bar.empty()) bar += "   ";
            bar += (eq ? "[" : "") + std::to_string(i + 1) + " " + name + (eq ? "]" : "");
        }
        if (hasShield_) bar += "   + SHIELD";
        centeredLine(viewportH - 46.0f, 2.0f, white, bar);
    }

    // MiniCS combat HUD: ammo, round clock, score, banners, markers, popups.
    if (minicsActive_) appendMinicsHud(frame);
}

// First-person hands + held weapon in real 3D: lit, oriented boxes in
// camera space, drawn by the backends through RenderFrame::viewmodelProj
// over a cleared depth buffer. Still no art pipeline - every shape is a
// box - but the result reads as an actual item in the hand: perspective,
// shading, movement bob, and per-weapon animation (punch / recoil / slash /
// swing) instead of flat screen rectangles.
void Game::appendViewmodelDraws(RenderFrame& frame) const {
    const glm::vec3 skin(0.82f, 0.63f, 0.49f);
    const glm::vec3 skinShade(0.72f, 0.53f, 0.40f);
    const glm::vec3 sleeve(0.23f, 0.27f, 0.33f);
    const glm::vec3 metalDark(0.13f, 0.14f, 0.17f);
    const glm::vec3 metalMid(0.26f, 0.28f, 0.32f);
    const glm::vec3 steel(0.72f, 0.76f, 0.82f);
    const glm::vec3 steelBright(0.86f, 0.89f, 0.94f);
    const glm::vec3 steelDark(0.55f, 0.59f, 0.65f);
    const glm::vec3 gripBrown(0.32f, 0.21f, 0.12f);
    const glm::vec3 guardGold(0.72f, 0.58f, 0.22f);

    auto box = [&frame](const glm::mat4& parent, glm::vec3 pos, glm::vec3 rotDeg,
                        glm::vec3 size, glm::vec3 color, float emissive = 0.0f) {
        ViewmodelBoxDraw draw;
        draw.transform = glm::scale(vmCompose(parent, pos, rotDeg), size);
        draw.color = color;
        draw.emissive = emissive;
        frame.viewmodelBoxes.push_back(draw);
    };

    // Shared animation inputs: pistol recoil decays with the muzzle flash,
    // the equip raise runs after weapon switches.
    float kick = muzzleFlashTimer_ > 0.0f ? muzzleFlashTimer_ / 0.07f : 0.0f;
    float lower = equipTimer_ > 0.0f ? equipTimer_ / kEquipDuration : 0.0f;

    // Melee animation offsets, driven by the shared combat actor's phase:
    // pull back through the windup, sweep to the strike point through the
    // active window, settle during recovery. The same keyframes animate
    // every melee weapon (the group anchor differs per weapon), so fists
    // hook/hammer/jab with the same inputs the sword slashes with.
    const melee::Actor& A = playerMelee_;
    glm::vec3 aPos(0.0f), aRot(0.0f);
    const std::string& weapon = inventory_.equippedId();
    bool meleeHeld = weapon != "glock";
    if (meleeHeld) {
        struct Key {
            glm::vec3 p;
            glm::vec3 r;
        };
        auto keyFor = [](melee::Attack t, bool windup) -> Key {
            switch (t) {
            case melee::Attack::SlashLeft:
                return windup ? Key{{-0.17f, 0.05f, 0.07f}, {6, 32, 30}}
                              : Key{{0.26f, -0.07f, -0.24f}, {-16, -48, -32}};
            case melee::Attack::SlashRight:
                return windup ? Key{{0.15f, 0.06f, 0.07f}, {2, -36, -26}}
                              : Key{{-0.28f, -0.09f, -0.24f}, {-16, 44, 30}};
            case melee::Attack::Overhead:
                return windup ? Key{{0.02f, 0.22f, 0.12f}, {52, 4, 6}}
                              : Key{{-0.02f, -0.16f, -0.30f}, {-58, -4, -8}};
            case melee::Attack::Stab: // pull to the hip, then a deep thrust
                return windup ? Key{{0.06f, -0.05f, 0.19f}, {8, 6, 4}}
                              : Key{{-0.06f, 0.03f, -0.46f}, {-6, -7, -2}};
            case melee::Attack::Jab: // short pommel bash: a sharp forward pop
                return windup ? Key{{0.03f, -0.01f, 0.10f}, {3, -8, -12}}
                              : Key{{-0.05f, 0.02f, -0.30f}, {-4, 6, 16}};
            case melee::Attack::Kick: // hands just dip while the boot flies
            default:
                return windup ? Key{{0.04f, -0.09f, 0.05f}, {-9, 6, 8}}
                              : Key{{0.05f, -0.11f, 0.06f}, {-12, 8, 10}};
            }
        };
        float p = melee::phaseProgress(A);
        float heavyMul = A.heavy ? 1.30f : 1.0f; // heavies wind further back
        Key w = keyFor(A.attack, true), st = keyFor(A.attack, false);
        switch (A.phase) {
        case melee::Phase::Windup: {
            float e = 1.0f - (1.0f - p) * (1.0f - p);
            aPos = w.p * e * heavyMul;
            aRot = w.r * e * heavyMul;
            break;
        }
        case melee::Phase::Active: {
            // Smoothstep, not linear: the blade accelerates out of the
            // windup and decelerates into the strike point, which is what
            // makes the swing read as a swing instead of a slide.
            float sp = p * p * (3.0f - 2.0f * p);
            aPos = glm::mix(w.p * heavyMul, st.p, sp);
            aRot = glm::mix(w.r * heavyMul, st.r, sp);
            break;
        }
        case melee::Phase::Recovery: {
            float e = (1.0f - p) * (1.0f - p);
            aPos = st.p * e;
            aRot = st.r * e;
            break;
        }
        case melee::Phase::Stagger: {
            float wob = std::sin(float(menuTime_) * 26.0f) * 0.012f;
            aPos = {wob, -0.15f + wob, 0.05f};
            aRot = {16.0f, 0.0f, 24.0f};
            break;
        }
        case melee::Phase::Idle:
            if (melee::guardUp(A) && !hasShield_) {
                aPos = {-0.06f, 0.07f, 0.03f}; // weapon guard: blade across the view
                aRot = {14.0f, 40.0f, 58.0f};
            }
            break;
        }
    }

    // Root: walk bob (phase locked to the footstep cadence), a slow idle
    // breath, look sway (the hands lag a beat behind the camera), a dip
    // when landing a fall, and the equip animation raising from below.
    float landDip = 0.0f;
    if (landDipTimer_ > 0.0f) {
        landDip = std::sin((landDipTimer_ / 0.26f) * 3.14159f) * landDipAmount_;
    }
    glm::vec3 rootPos(std::sin(bobPhase_) * 0.013f * bobIntensity_
                          - swayX_ * 0.010f,
                      -std::abs(std::sin(bobPhase_)) * 0.011f * bobIntensity_
                          - 0.004f * std::sin(float(menuTime_) * 1.6f)
                          + swayY_ * 0.008f - landDip * 0.07f
                          - lower * lower * 0.30f,
                      0.0f);
    glm::mat4 root = vmCompose(glm::mat4(1.0f), rootPos,
                               {lower * -35.0f - landDip * 6.0f + swayY_ * 1.2f,
                                swayX_ * -1.6f,
                                lower * 12.0f + swayX_ * 1.1f});

    // A clenched fist with the forearm sleeve running down off-screen.
    auto fistAndArm = [&](glm::vec3 pos, glm::vec3 rotDeg, bool left) {
        float s = left ? -1.0f : 1.0f;
        glm::mat4 hand = vmCompose(root, pos, rotDeg);
        box(hand, {0.0f, 0.0f, 0.0f}, {0, 0, 0}, {0.105f, 0.095f, 0.115f}, skin);
        box(hand, {0.0f, -0.008f, -0.068f}, {0, 0, 0}, {0.098f, 0.082f, 0.034f},
            skinShade); // curled fingers
        box(hand, {s * -0.058f, -0.012f, -0.01f}, {0, 0, 0}, {0.032f, 0.055f, 0.07f},
            skinShade); // thumb side
        box(hand, {s * 0.008f, -0.03f, 0.10f}, {0, 0, 0}, {0.095f, 0.085f, 0.09f},
            skin); // wrist
        box(hand, {s * 0.03f, -0.14f, 0.26f}, {50.0f, s * -8.0f, 0.0f},
            {0.115f, 0.115f, 0.36f}, sleeve); // forearm
    };

    if (weapon == "glock") {
        // Two-handed pistol held center-right. Recoil: the muzzle snaps up
        // hard on the shot and settles fast (quadratic decay), the gun drives
        // straight back a touch, and a hint of roll sells the torque. The
        // accumulated view-punch (recoilPitch_) adds visible climb on fast
        // fire. Reloading dips and tilts the gun while the magazine is swapped.
        float snap = kick * kick;
        float rprog = reloadTimer_ > 0.0f ? 1.0f - reloadTimer_ / reloadDuration_ : 0.0f;
        float reloadDip = reloadTimer_ > 0.0f ? std::sin(rprog * 3.14159265f) : 0.0f;
        glm::vec3 gunPos = glm::vec3(0.15f, -0.145f, -0.38f)
                           + glm::vec3(0.0f, snap * 0.014f, kick * 0.055f)
                           + glm::vec3(-0.02f * reloadDip, -0.17f * reloadDip,
                                       0.05f * reloadDip);
        glm::mat4 gun = vmCompose(root, gunPos,
                                  {snap * 17.0f + recoilPitch_ * 1.0f - 2.0f,
                                   -4.0f - reloadDip * 8.0f,
                                   -2.0f - snap * 4.0f - reloadDip * 26.0f});
        // Slide, top rib, sights, frame.
        box(gun, {0.0f, 0.043f, -0.055f}, {0, 0, 0}, {0.05f, 0.052f, 0.245f}, metalDark);
        box(gun, {0.0f, 0.072f, -0.055f}, {0, 0, 0}, {0.036f, 0.012f, 0.245f}, metalMid);
        box(gun, {0.0f, 0.086f, 0.058f}, {0, 0, 0}, {0.034f, 0.016f, 0.014f}, metalMid);
        box(gun, {0.0f, 0.088f, -0.168f}, {0, 0, 0}, {0.010f, 0.018f, 0.012f}, metalMid);
        box(gun, {0.0f, -0.002f, -0.075f}, {0, 0, 0}, {0.046f, 0.038f, 0.19f}, metalMid);
        // Raked-back grip, mag base, trigger guard.
        box(gun, {0.0f, -0.075f, 0.05f}, {-16.0f, 0, 0}, {0.046f, 0.155f, 0.062f}, metalDark);
        box(gun, {0.0f, -0.148f, 0.072f}, {-16.0f, 0, 0}, {0.052f, 0.022f, 0.07f}, metalMid);
        box(gun, {0.0f, -0.045f, -0.052f}, {0, 0, 0}, {0.012f, 0.042f, 0.012f}, metalDark);
        box(gun, {0.0f, -0.062f, -0.012f}, {0, 0, 0}, {0.012f, 0.012f, 0.08f}, metalDark);
        // Hands: right palm on the grip, fingers wrapping the front strap,
        // left hand cupping from the left with the thumb forward.
        box(gun, {0.035f, -0.08f, 0.055f}, {-16.0f, 0, 0}, {0.036f, 0.13f, 0.08f}, skin);
        box(gun, {0.0f, -0.07f, 0.018f}, {-16.0f, 0, 0}, {0.058f, 0.095f, 0.032f}, skinShade);
        box(gun, {-0.036f, -0.09f, 0.045f}, {-16.0f, 0, 0}, {0.034f, 0.115f, 0.085f}, skin);
        box(gun, {-0.02f, -0.052f, -0.03f}, {0, 0, 0}, {0.03f, 0.026f, 0.07f}, skinShade);
        // Forearms.
        box(gun, {0.07f, -0.24f, 0.24f}, {50.0f, -10.0f, 0.0f}, {0.115f, 0.115f, 0.34f}, sleeve);
        box(gun, {-0.075f, -0.25f, 0.22f}, {50.0f, 12.0f, 0.0f}, {0.115f, 0.115f, 0.32f}, sleeve);
        // Reload: a fresh magazine drops out of the well (first half) then
        // seats home (second half), read as the off hand swapping mags.
        if (reloadTimer_ > 0.0f) {
            float magDrop = rprog < 0.5f ? 0.14f * (rprog * 2.0f)
                                         : 0.14f * (1.0f - (rprog - 0.5f) * 2.0f);
            box(gun, {-0.02f, -0.16f - magDrop, 0.06f}, {-16.0f, 0, 0},
                {0.05f, 0.14f, 0.07f}, metalMid);
        }
        // Muzzle flash: a bright emissive star that flares big on the shot and
        // collapses to a white-hot core as the timer runs out.
        if (muzzleFlashTimer_ > 0.0f) {
            float f = muzzleFlashTimer_ / 0.07f;   // 1 -> 0 across the flash
            float s = 0.65f + 0.5f * f;            // flares wide, then shrinks
            box(gun, {0.0f, 0.043f, -0.235f}, {0, 0, 45.0f},
                {0.085f * s, 0.085f * s, 0.05f}, {1.0f, 0.62f, 0.15f}, 1.0f); // amber petals
            box(gun, {0.0f, 0.043f, -0.235f}, {0, 0, 0},
                {0.06f * s, 0.06f * s, 0.05f}, {1.0f, 0.82f, 0.32f}, 1.0f);   // inner glow
            box(gun, {0.0f, 0.043f, -0.255f}, {0, 0, 0},
                {0.032f * s, 0.032f * s, 0.14f * s}, {1.0f, 0.97f, 0.85f}, 1.0f); // white-hot core
        }
    } else if (weapon == "karambit") {
        // Reverse grip: safety ring over the fist, claw blade hooking down
        // out of the pinky side. Attack phases drive the whole arm.
        glm::vec3 hPos = glm::vec3(0.20f, -0.125f, -0.42f) + aPos;
        glm::vec3 hRot = glm::vec3(14.0f, 24.0f, -8.0f) + aRot;
        glm::mat4 hand = vmCompose(root, hPos, hRot);
        box(hand, {0.0f, 0.0f, 0.0f}, {0, 0, 0}, {0.105f, 0.095f, 0.115f}, skin);
        box(hand, {0.0f, -0.008f, -0.068f}, {0, 0, 0}, {0.098f, 0.082f, 0.034f}, skinShade);
        box(hand, {-0.058f, -0.012f, -0.01f}, {0, 0, 0}, {0.032f, 0.055f, 0.07f}, skinShade);
        box(hand, {0.008f, -0.03f, 0.10f}, {0, 0, 0}, {0.095f, 0.085f, 0.09f}, skin);
        box(hand, {0.03f, -0.14f, 0.26f}, {50.0f, -8.0f, 0.0f}, {0.115f, 0.115f, 0.36f}, sleeve);
        // Safety ring standing on the top of the fist.
        box(hand, {0.0f, 0.062f, 0.028f}, {0, 0, 0}, {0.016f, 0.018f, 0.055f}, steel);
        box(hand, {0.0f, 0.082f, 0.028f}, {0, 0, 0}, {0.016f, 0.016f, 0.038f}, steelBright);
        // Claw: juts forward out of the bottom of the fist, each segment
        // pitching further down so the blade reads as a hook aimed ahead.
        box(hand, {0.0f, -0.062f, -0.075f}, {-12.0f, 0, 0}, {0.022f, 0.032f, 0.13f}, steel);
        box(hand, {0.0f, -0.085f, -0.155f}, {-40.0f, 0, 0}, {0.020f, 0.028f, 0.11f}, steelBright);
        box(hand, {0.0f, -0.125f, -0.20f}, {-75.0f, 0, 0}, {0.018f, 0.024f, 0.09f}, steel);
    } else if (weapon == "sword") {
        // Both hands stacked on the grip low-right, blade angled up across
        // the view; attack phases carry the whole assembly through windup,
        // cut, and recovery.
        glm::vec3 sPos = glm::vec3(0.21f, -0.17f, -0.50f) + aPos;
        glm::vec3 sRot = glm::vec3(34.0f, -10.0f, -8.0f) + aRot;
        glm::mat4 sw = vmCompose(root, sPos, sRot);
        // Grip, pommel, crossguard.
        box(sw, {0.0f, 0.0f, 0.10f}, {0, 0, 0}, {0.034f, 0.045f, 0.15f}, gripBrown);
        box(sw, {0.0f, 0.0f, 0.185f}, {0, 0, 0}, {0.052f, 0.06f, 0.038f}, guardGold);
        box(sw, {0.0f, 0.0f, 0.015f}, {0, 0, 0}, {0.19f, 0.026f, 0.034f}, guardGold);
        // Blade with a raised fuller and a tapering tip.
        box(sw, {0.0f, 0.0f, -0.33f}, {0, 0, 0}, {0.016f, 0.055f, 0.62f}, steel);
        box(sw, {0.0f, 0.0f, -0.30f}, {0, 0, 0}, {0.019f, 0.018f, 0.52f}, steelDark);
        box(sw, {0.0f, 0.0f, -0.67f}, {0, 0, 0}, {0.014f, 0.040f, 0.07f}, steelBright);
        box(sw, {0.0f, 0.0f, -0.715f}, {0, 0, 0}, {0.012f, 0.022f, 0.045f}, steelBright);
        // Hands on the grip (the left moves to the shield when one is worn),
        // forearms running down off-screen.
        box(sw, {0.0f, 0.0f, 0.065f}, {0, 0, 0}, {0.062f, 0.075f, 0.065f}, skin);
        box(sw, {0.05f, -0.14f, 0.30f}, {50.0f, -10.0f, 0.0f}, {0.115f, 0.115f, 0.34f}, sleeve);
        if (!hasShield_) {
            box(sw, {0.0f, 0.0f, 0.135f}, {0, 0, 0}, {0.060f, 0.072f, 0.062f}, skinShade);
            box(sw, {-0.02f, -0.16f, 0.32f}, {52.0f, 10.0f, 0.0f}, {0.115f, 0.115f, 0.32f}, sleeve);
        }
    } else { // fists (and any unknown weapon id)
        // Punches alternate hands: a left slash IS the left fist's swing
        // (the SlashLeft keys pull to screen-left and drive across to the
        // right - exactly a left hook), everything else stays on the right.
        // One fist punching while the other holds guard reads like boxing;
        // the old both-attacks-on-one-fist version read like a glitch.
        bool inSwing = A.phase == melee::Phase::Windup ||
                       A.phase == melee::Phase::Active ||
                       A.phase == melee::Phase::Recovery;
        bool leftPunch = inSwing && !hasShield_ &&
                         A.attack == melee::Attack::SlashLeft;
        glm::vec3 rOff = leftPunch ? glm::vec3(0.0f) : aPos;
        glm::vec3 rRotOff = leftPunch ? glm::vec3(0.0f) : aRot;
        fistAndArm(glm::vec3(0.235f, -0.22f, -0.48f) + rOff,
                   glm::vec3(8.0f, -10.0f, -6.0f) + rRotOff, false);
        if (!hasShield_) {
            glm::vec3 lOff = leftPunch ? aPos : glm::vec3(0.0f);
            glm::vec3 lRotOff = leftPunch ? aRot : glm::vec3(0.0f);
            fistAndArm(glm::vec3(-0.24f, -0.24f, -0.50f) + lOff,
                       glm::vec3(10.0f, 14.0f, 8.0f) + lRotOff, true);
        }
    }

    // Shield on the left arm: resting low at the edge, raised to cover the
    // view while blocking, drooping when the guard was kicked open or broken.
    if (meleeHeld && hasShield_) {
        bool raised = melee::guardUp(A);
        glm::vec3 sp = raised ? glm::vec3(-0.13f, -0.05f, -0.40f)
                              : glm::vec3(-0.31f, -0.24f, -0.50f);
        glm::vec3 sr = raised ? glm::vec3(4.0f, 14.0f, 2.0f)
                              : glm::vec3(8.0f, 38.0f, -12.0f);
        if (A.phase == melee::Phase::Stagger) {
            sp += glm::vec3(-0.06f, -0.13f, 0.0f);
            sr.z -= 35.0f;
        }
        glm::mat4 sh = vmCompose(root, sp, sr);
        box(sh, {0.0f, 0.0f, 0.0f}, {0, 0, 0}, {0.34f, 0.44f, 0.035f},
            {0.46f, 0.30f, 0.16f}); // wooden face
        box(sh, {0.0f, 0.0f, -0.032f}, {0, 0, 0}, {0.10f, 0.10f, 0.05f}, steel); // boss
        box(sh, {0.0f, 0.225f, 0.0f}, {0, 0, 0}, {0.34f, 0.022f, 0.045f}, metalMid);
        box(sh, {0.0f, -0.225f, 0.0f}, {0, 0, 0}, {0.34f, 0.022f, 0.045f}, metalMid);
        box(sh, {-0.175f, 0.0f, 0.0f}, {0, 0, 0}, {0.022f, 0.44f, 0.045f}, metalMid);
        box(sh, {0.175f, 0.0f, 0.0f}, {0, 0, 0}, {0.022f, 0.44f, 0.045f}, metalMid);
        box(sh, {0.02f, -0.17f, 0.11f}, {50.0f, 8.0f, 0.0f}, {0.115f, 0.115f, 0.34f},
            sleeve); // left forearm behind the straps
    }

    // Kick: a boot drives up from the bottom of the screen through the
    // attack phases, whatever else the hands are doing.
    if (meleeHeld && A.attack == melee::Attack::Kick &&
        (A.phase == melee::Phase::Windup || A.phase == melee::Phase::Active ||
         A.phase == melee::Phase::Recovery)) {
        float p = melee::phaseProgress(A);
        float sp = p * p * (3.0f - 2.0f * p); // smooth extension, no linear pop
        float ext = A.phase == melee::Phase::Windup   ? 0.25f * sp
                    : A.phase == melee::Phase::Active ? 0.25f + 0.75f * sp
                                                      : 1.0f - sp;
        glm::mat4 leg = vmCompose(root,
                                  {0.06f, -0.36f + 0.16f * ext, -0.28f - 0.44f * ext},
                                  {-72.0f + 48.0f * ext, 4.0f, 0.0f});
        box(leg, {0.0f, 0.0f, 0.22f}, {0, 0, 0}, {0.15f, 0.15f, 0.46f}, sleeve); // shin
        box(leg, {0.0f, -0.02f, -0.07f}, {0, 0, 0}, {0.13f, 0.10f, 0.27f},
            {0.10f, 0.10f, 0.12f}); // boot
    }
}

// MiniCS enemies, drawn procedurally so they can walk, aim, shoot and flinch
// (the entity def's own visual is skipped for these - see buildRenderFrame).
// Every part is a lit world-space box, same as the duelist telegraphs.
void Game::appendMinicsEnemyDraws(RenderFrame& frame) const {
    auto obox = [&frame](const glm::mat4& parent, glm::vec3 pos, glm::vec3 rotDeg,
                         glm::vec3 size, glm::vec3 color, float emissive = 0.0f) {
        ViewmodelBoxDraw d;
        d.transform = glm::scale(vmCompose(parent, pos, rotDeg), size);
        d.color = color;
        d.emissive = emissive;
        frame.orientedBoxes.push_back(d);
    };

    const glm::vec3 vest{0.80f, 0.22f, 0.20f};
    const glm::vec3 vestDark{0.60f, 0.16f, 0.15f};
    const glm::vec3 gear{0.20f, 0.21f, 0.24f};
    const glm::vec3 skin{0.86f, 0.68f, 0.54f};
    const glm::vec3 gunCol{0.11f, 0.11f, 0.13f};

    for (const auto& kv : minicsBrains_) {
        const WorldEntity* e = world_.entityById(kv.first);
        if (!e || !e->active) continue;
        const MinicsEnemy& b = kv.second;

        float legSwing = std::sin(b.walkPhase) * 24.0f * b.walkAmt;
        float bodyBob = std::abs(std::sin(b.walkPhase)) * 0.05f * b.walkAmt;
        float flinch = glm::clamp(b.flinchTimer / 0.22f, 0.0f, 1.0f);
        float hitFlash = glm::clamp(e->hitFlash / 0.15f, 0.0f, 1.0f);
        float lean = -flinch * 15.0f; // reel backward when struck
        glm::vec3 tint = glm::mix(glm::vec3(1.0f), glm::vec3(1.0f, 0.28f, 0.22f), hitFlash);
        auto C = [&](glm::vec3 c) { return c * tint; };

        glm::vec3 base = e->pos + glm::vec3(0.0f, bodyBob, 0.0f);
        glm::mat4 group = vmCompose(glm::translate(glm::mat4(1.0f), base),
                                    glm::vec3(0.0f), {lean, glm::degrees(b.yaw), 0.0f});

        // Legs (swing in opposite phase), torso, belt, shoulders, head, helmet.
        obox(group, {-0.16f, 0.36f, 0.0f}, {legSwing, 0, 0}, {0.20f, 0.72f, 0.24f}, C(gear));
        obox(group, {0.16f, 0.36f, 0.0f}, {-legSwing, 0, 0}, {0.20f, 0.72f, 0.24f}, C(gear));
        obox(group, {0.0f, 0.74f, 0.0f}, {0, 0, 0}, {0.54f, 0.12f, 0.36f}, C(gear));
        obox(group, {0.0f, 1.04f, 0.0f}, {0, 0, 0}, {0.52f, 0.66f, 0.34f}, C(vest));
        obox(group, {0.0f, 1.32f, 0.0f}, {0, 0, 0}, {0.60f, 0.16f, 0.40f}, C(vestDark));
        obox(group, {0.0f, 1.58f, 0.0f}, {0, 0, 0}, {0.28f, 0.30f, 0.28f}, C(skin));
        obox(group, {0.0f, 1.73f, 0.0f}, {0, 0, 0}, {0.31f, 0.11f, 0.31f}, C(gear));

        // Arms + pistol. Aiming raises both arms level and pushes the gun
        // forward (local -Z, which faces the player); at rest they hang lower.
        float aim = b.aimBlend;
        float armPitch = glm::mix(28.0f, 82.0f, aim);
        float armZ = glm::mix(0.02f, -0.12f, aim);
        obox(group, {-0.30f, 1.14f, armZ}, {armPitch, 0, 0}, {0.13f, 0.42f, 0.15f}, C(vestDark));
        obox(group, {0.30f, 1.14f, armZ}, {armPitch, 0, 0}, {0.13f, 0.42f, 0.15f}, C(vestDark));

        float gunY = glm::mix(0.96f, 1.15f, aim);
        float gunZ = glm::mix(-0.30f, -0.54f, aim);
        glm::mat4 gun = vmCompose(group, {0.17f, gunY, gunZ}, {0, 0, 0});
        obox(gun, {0.0f, 0.02f, 0.0f}, {0, 0, 0}, {0.07f, 0.10f, 0.26f}, gunCol); // slide
        obox(gun, {0.0f, -0.10f, 0.05f}, {0, 0, 0}, {0.06f, 0.12f, 0.06f}, gunCol); // grip
        if (b.muzzle > 0.0f) { // muzzle flash on the shot
            obox(gun, {0.0f, 0.02f, -0.20f}, {0, 0, 45.0f}, {0.09f, 0.09f, 0.07f},
                 {1.0f, 0.82f, 0.30f}, 1.0f);
        }
    }
}

// MiniCS combat HUD: ammo, round clock + enemies, score, the countdown/FIGHT
// banners, floating score popups, the kill marker, a directional damage
// indicator, and the win/loss end screen. Drawn over the shared HUD.
void Game::appendMinicsHud(RenderFrame& frame) const {
    const int vw = frame.viewportW, vh = frame.viewportH;
    const float cx = vw * 0.5f, cy = vh * 0.5f;
    const glm::vec4 white(1, 1, 1, 1);
    char buf[128];

    auto text = [&](float x, float y, float sc, glm::vec4 col, std::string s,
                    bool center = false) {
        TextDraw t;
        t.x = x; t.y = y; t.scale = sc; t.color = col; t.centered = center;
        t.text = std::move(s);
        frame.texts.push_back(std::move(t));
    };

    // --- End screen (Won / Lost): a dim overlay with the result + restart.
    if (minicsPhase_ == MiniCSPhase::Won || minicsPhase_ == MiniCSPhase::Lost) {
        bool won = minicsPhase_ == MiniCSPhase::Won;
        frame.rects.push_back({0, 0, float(vw), float(vh),
                               glm::vec4(0.04f, 0.06f, 0.09f, 0.62f)});
        glm::vec4 head = won ? glm::vec4(0.45f, 1.0f, 0.55f, 1.0f)
                             : glm::vec4(1.0f, 0.40f, 0.34f, 1.0f);
        text(cx, vh * 0.24f, 6.5f, head, won ? "VICTORY" : "DEFEAT", true);
        frame.rects.push_back({cx - 150.0f, vh * 0.24f + 58.0f, 300.0f, 3.0f, head});
        text(cx, vh * 0.24f + 74.0f, 2.0f, glm::vec4(0.82f, 0.86f, 0.92f, 1.0f),
             minicsResultNote_, true);
        std::snprintf(buf, sizeof(buf), "ENEMIES ELIMINATED   %d / %d", minicsKills_,
                      minicsEnemiesTotal_);
        text(cx, vh * 0.45f, 2.4f, white, buf, true);
        std::snprintf(buf, sizeof(buf), "SCORE   %d", minicsScore_);
        text(cx, vh * 0.45f + 36.0f, 2.6f, glm::vec4(1.0f, 0.9f, 0.4f, 1.0f), buf, true);
        if (minicsPhaseTimer_ > 0.6f) {
            float a = 0.6f + 0.4f * std::sin(float(menuTime_) * 5.0f);
            text(cx, vh * 0.64f, 2.6f, glm::vec4(1, 1, 1, a),
                 "FIRE OR SPACE  -  PLAY AGAIN", true);
            text(cx, vh * 0.64f + 34.0f, 2.0f, glm::vec4(0.75f, 0.8f, 0.85f, 1.0f),
                 "ESC  -  MENU", true);
        }
        return;
    }

    // --- Ammo block, bottom-right: big magazine count over a small reserve.
    {
        float ax = vw - 196.0f, ay = vh - 78.0f;
        frame.rects.push_back({ax - 18.0f, ay - 14.0f, 190.0f, 62.0f,
                               glm::vec4(0.05f, 0.07f, 0.10f, 0.45f)});
        glm::vec4 magCol = magAmmo_ == 0 ? glm::vec4(0.96f, 0.30f, 0.24f, 1.0f)
                           : magAmmo_ <= 5 ? glm::vec4(0.98f, 0.76f, 0.30f, 1.0f)
                                           : white;
        std::snprintf(buf, sizeof(buf), "%d", magAmmo_);
        text(ax, ay - 8.0f, 4.8f, magCol, buf);
        std::snprintf(buf, sizeof(buf), "/ %d", reserveAmmo_);
        text(ax + 92.0f, ay + 18.0f, 2.2f, glm::vec4(0.72f, 0.78f, 0.85f, 1.0f), buf);
        if (reloadTimer_ > 0.0f) {
            text(ax + 6.0f, ay - 34.0f, 2.0f, glm::vec4(1.0f, 0.9f, 0.4f, 1.0f), "RELOADING");
        }
    }
    if (reloadTimer_ <= 0.0f && magAmmo_ == 0 && reserveAmmo_ > 0) {
        float a = 0.6f + 0.4f * std::sin(float(menuTime_) * 8.0f);
        text(cx, vh * 0.63f, 2.6f, glm::vec4(1.0f, 0.85f, 0.3f, a), "PRESS R TO RELOAD", true);
    }

    // --- Round clock + enemies remaining, top-center; score top-left.
    float remain = minicsPhase_ == MiniCSPhase::Live ? std::max(0.0f, minicsPhaseTimer_)
                                                     : kMinicsRoundSeconds;
    std::snprintf(buf, sizeof(buf), "%d:%02d", int(remain) / 60, int(remain) % 60);
    text(cx, 16.0f, 3.2f, remain <= 10.0f ? glm::vec4(0.98f, 0.35f, 0.28f, 1.0f) : white,
         buf, true);
    std::snprintf(buf, sizeof(buf), "ENEMIES  %d / %d", minicsEnemiesAlive_,
                  minicsEnemiesTotal_);
    text(cx, 56.0f, 2.0f, glm::vec4(0.92f, 0.62f, 0.55f, 1.0f), buf, true);
    std::snprintf(buf, sizeof(buf), "SCORE  %d", minicsScore_);
    text(16.0f, 14.0f, 2.4f, glm::vec4(1.0f, 0.9f, 0.4f, 1.0f), buf);

    // --- Countdown / FIGHT banners.
    if (minicsPhase_ == MiniCSPhase::Countdown) {
        text(cx, vh * 0.33f, 3.2f, glm::vec4(0.7f, 0.85f, 1.0f, 1.0f), "GET READY", true);
        std::snprintf(buf, sizeof(buf), "%d", std::max(1, int(std::ceil(minicsPhaseTimer_))));
        text(cx, vh * 0.40f, 6.5f, white, buf, true);
        text(cx, vh * 0.52f, 2.0f, glm::vec4(0.8f, 0.85f, 0.9f, 1.0f),
             "ELIMINATE ALL ENEMIES", true);
    } else if (minicsPhase_ == MiniCSPhase::Live && minicsBannerTimer_ < 1.1f) {
        float a = 1.0f - minicsBannerTimer_ / 1.1f;
        text(cx, vh * 0.35f, 5.0f, glm::vec4(1.0f, 0.95f, 0.5f, a), "FIGHT!", true);
    }

    // --- Kill marker: a bigger, brighter red X over the crosshair.
    if (killMarkerTimer_ > 0.0f) {
        float k = killMarkerTimer_ / 0.30f;
        float r = 15.0f + (1.0f - k) * 5.0f; // expands slightly as it fades
        glm::vec4 col(1.0f, 0.28f, 0.24f, glm::clamp(k + 0.2f, 0.0f, 1.0f));
        for (float sx : {-1.0f, 1.0f}) {
            for (float sy : {-1.0f, 1.0f}) {
                frame.rects.push_back({cx + sx * r - 3.0f, cy + sy * r - 3.0f, 6.0f, 6.0f, col});
            }
        }
    }

    // --- Directional damage indicator: a red arc toward the last shooter.
    if (hurtTimer_ > 0.0f) {
        float a = glm::clamp(hurtTimer_ / 0.85f, 0.0f, 1.0f) * 0.85f;
        const float R = 52.0f;
        for (int i = -2; i <= 2; ++i) {
            float ang = hurtAngle_ + float(i) * 0.10f;
            float px = cx + std::sin(ang) * R;
            float py = cy - std::cos(ang) * R;
            frame.rects.push_back({px - 4.0f, py - 4.0f, 8.0f, 8.0f,
                                   glm::vec4(1.0f, 0.25f, 0.2f, a)});
        }
    }

    // --- Floating score popups, rising and fading near the crosshair.
    for (const ScorePopup& p : scorePopups_) {
        float k = p.life / p.total;
        float y = cy - 52.0f - (1.0f - k) * 46.0f;
        text(cx + p.driftX, y, 2.6f, glm::vec4(p.color, glm::clamp(k * 1.4f, 0.0f, 1.0f)),
             p.text, true);
    }
}

void Game::appendMenuDraws(RenderFrame& frame) const {
    const int viewportW = frame.viewportW;
    const int viewportH = frame.viewportH;
    const float cx = viewportW * 0.5f;
    const glm::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
    const glm::vec4 label(0.85f, 0.88f, 0.92f, 1.0f);
    const glm::vec4 dimText(0.65f, 0.70f, 0.78f, 1.0f);
    const glm::vec4 notice(1.0f, 0.85f, 0.45f, 1.0f);
    char buf[64];

    auto centeredText = [&](float x, float y, float scale, const glm::vec4& color,
                            std::string text) {
        TextDraw t;
        t.centered = true;
        t.x = x;
        t.y = y;
        t.scale = scale;
        t.color = color;
        t.text = std::move(text);
        frame.texts.push_back(std::move(t));
    };

    // Dim the world (frozen gameplay or the orbiting menu backdrop).
    frame.rects.push_back({0, 0, float(viewportW), float(viewportH),
                           glm::vec4(0.05f, 0.07f, 0.10f, ui_ == UiScreen::MainMenu ? 0.55f : 0.62f)});

    // A screen title with a short accent underline beneath it, so every
    // sub-screen reads as part of one hub instead of a bare label.
    const glm::vec4 accent(0.42f, 0.62f, 0.95f, 1.0f);
    auto screenTitle = [&](float y, const char* text) {
        centeredText(cx, y, 4.0f, white, text);
        frame.rects.push_back({cx - 60.0f, y + 34.0f, 120.0f, 3.0f, accent});
    };

    // Titles.
    switch (ui_) {
    case UiScreen::MainMenu:
        centeredText(cx, viewportH * 0.15f, 6.5f, white, "TACMOVE");
        frame.rects.push_back({cx - 150.0f, viewportH * 0.15f + 56.0f, 300.0f, 3.0f, accent});
        centeredText(cx, viewportH * 0.15f + 74.0f, 2.0f, dimText,
                     "CROSS-PLATFORM GAME HUB");
        centeredText(cx, viewportH - 34.0f, 2.0f, dimText,
                     "LOCAL PLAY NOW  -  ONLINE COMING  -  MACOS / LINUX / WINDOWS");
        break;
    case UiScreen::Play:
        screenTitle(viewportH * 0.24f, "PLAY");
        break;
    case UiScreen::GameModes:
        screenTitle(viewportH * 0.13f, "GAME MODES");
        centeredText(cx, viewportH * 0.13f + 54.0f, 2.0f, dimText,
                     "THE GAMES INSIDE THE GAME - PICK ONE TO JUMP IN");
        break;
    case UiScreen::Servers:
        screenTitle(viewportH * 0.24f, "SERVERS");
        break;
    case UiScreen::ServerList:
        screenTitle(viewportH * 0.16f, "SERVER LIST");
        centeredText(cx, viewportH * 0.42f, 2.0f, dimText,
                     "NO SERVERS FOUND");
        centeredText(cx, viewportH * 0.42f + 30.0f, 2.0f, dimText,
                     "LIVE SERVER DISCOVERY ARRIVES WITH THE NETWORKING MILESTONE");
        break;
    case UiScreen::RecentGames:
        screenTitle(viewportH * 0.16f, "RECENT GAMES");
        if (!gamemodes::find(lastModeId_)) {
            centeredText(cx, viewportH * 0.42f, 2.0f, dimText,
                         "NOTHING PLAYED YET - START A GAME MODE FIRST");
        } else {
            centeredText(cx, viewportH * 0.30f, 2.0f, dimText, "JUMP BACK IN");
        }
        break;
    case UiScreen::Create:
        screenTitle(viewportH * 0.24f, "CREATE");
        centeredText(cx, viewportH * 0.24f + 52.0f, 2.0f, dimText,
                     "MAKE MAPS AND SKINS - GAME MODES ARE BUILT-IN");
        break;
    case UiScreen::Skins:
        screenTitle(viewportH * 0.16f, "SKINS");
        centeredText(cx, viewportH * 0.28f, 2.0f, dimText,
                     "PICK A HIGHLIGHT COLOR - FULL CHARACTER SKINS ARE COMING");
        break;
    case UiScreen::Publish:
        screenTitle(viewportH * 0.16f, "PUBLISH");
        centeredText(cx, viewportH * 0.42f, 2.0f, dimText,
                     "SHARE MAPS AND SKINS ONLINE");
        centeredText(cx, viewportH * 0.42f + 30.0f, 2.0f, dimText,
                     "PUBLISHING ARRIVES WITH ONLINE PLAY - EVERYTHING SAVES LOCALLY FOR NOW");
        break;
    case UiScreen::Profile:
        screenTitle(viewportH * 0.16f, "PROFILE");
        centeredText(cx, viewportH * 0.36f, 2.5f, white, "PLAYER");
        centeredText(cx, viewportH * 0.36f + 34.0f, 2.0f, dimText,
                     lastModeId_.empty()
                         ? "LAST PLAYED: NOTHING YET"
                         : ("LAST PLAYED: " + toUpperAscii(
                               gamemodes::find(lastModeId_)
                                   ? gamemodes::find(lastModeId_)->displayName
                                   : lastModeId_)));
        centeredText(cx, viewportH * 0.36f + 64.0f, 2.0f, dimText,
                     "STATS AND ACCOUNTS ARRIVE WITH ONLINE PLAY");
        break;
    case UiScreen::CreateWorld:
        screenTitle(viewportH * 0.15f, "CREATE MAP");
        break;
    case UiScreen::LoadWorld:
        screenTitle(viewportH * 0.17f, "EDIT MAP");
        break;
    case UiScreen::SpawnMenu:
        screenTitle(viewportH * 0.14f, "BUILD MENU");
        centeredText(cx, viewportH * 0.14f + 48.0f, 2.0f, dimText,
                     "SPAWN PROPS, BOTS AND PICKUPS - B OR ESC CLOSES");
        if (spawnableDefs(ContentCategory(spawnCategory_)).empty()) {
            centeredText(cx, viewportH * 0.45f, 2.0f, dimText,
                         "NOTHING IN THIS CATEGORY YET");
        }
        break;
    case UiScreen::Inventory: {
        centeredText(cx, viewportH * 0.10f, 4.0f, white, "INVENTORY");
        centeredText(cx, viewportH * 0.10f + 42.0f, 2.0f, dimText,
                     "TAB OR ESC CLOSES - CLICK A TOOL OR WEAPON TO EQUIP IT");

        // Column headers over the two button columns (layout mirrors
        // buildMenuLayout's Inventory case).
        const float rowW = 380.0f;
        centeredText(cx - rowW * 0.5f - 30.0f, viewportH * 0.26f - 34.0f, 2.0f,
                     label, "ITEMS");
        centeredText(cx + rowW * 0.5f + 30.0f, viewportH * 0.26f - 34.0f, 2.0f,
                     label, "CRAFTING");

        // Item color swatches beside each stack row.
        float sy = viewportH * 0.26f;
        for (const ItemStack& s : itemInv_.stacks) {
            if (const ItemDef* item = items_.find(s.itemId)) {
                frame.rects.push_back({cx - rowW - 30.0f - 26.0f, sy + 11.0f, 18.0f, 18.0f,
                                       glm::vec4(item->color, 1.0f)});
            }
            sy += 48.0f;
        }
        if (itemInv_.stacks.empty()) {
            centeredText(cx - rowW * 0.5f - 30.0f, viewportH * 0.40f, 2.0f, dimText,
                         "EMPTY - GATHER WITH E");
        }

        const ItemDef* held = items_.find(itemInv_.equippedItemId);
        centeredText(cx, viewportH * 0.79f, 2.0f, notice,
                     "MAIN HAND: " + (held ? toUpperAscii(held->displayName)
                                           : std::string("BARE HANDS")));
        break;
    }
    case UiScreen::DirectConnect:
        screenTitle(viewportH * 0.13f, "DIRECT CONNECT");
        break;
    case UiScreen::Settings:
        centeredText(cx, viewportH * 0.10f, 4.0f, white, "SETTINGS");
        break;
    case UiScreen::PauseMain:
        centeredText(cx, viewportH * 0.28f, 4.0f, white, "PAUSED");
        if (!currentModeName_.empty()) {
            centeredText(cx, viewportH * 0.28f + 40.0f, 2.0f, dimText,
                         currentModeName_ +
                             (activeModeStatus_ == ModeStatus::Preview ? "  -  EARLY" : ""));
        }
        break;
    case UiScreen::None:
        break;
    }

    // Create World: world-name input box (always focused on this screen)
    // plus the world-type heading and the selected type's description. The
    // type tabs themselves are buttons from buildMenuLayout.
    if (ui_ == UiScreen::CreateWorld) {
        float boxX = cx - kAddressBoxW / 2;
        float boxY = viewportH * 0.24f;
        centeredText(cx, boxY - 26.0f, 2.0f, label, "MAP NAME");
        frame.rects.push_back({boxX, boxY, kAddressBoxW, kAddressBoxH,
                               glm::vec4(0.10f, 0.12f, 0.16f, 0.95f)});
        frame.rects.push_back({boxX, boxY + kAddressBoxH - 3.0f, kAddressBoxW, 3.0f,
                               glm::vec4(0.45f, 0.55f, 0.70f, 1.0f)});
        std::string shown = worldNameInput_;
        if (std::fmod(menuTime_, 1.0) < 0.55) shown += "_";
        TextDraw nameText;
        nameText.x = boxX + 12.0f;
        nameText.y = boxY + (kAddressBoxH - 14.0f) / 2;
        nameText.scale = 2.0f;
        nameText.color = white;
        nameText.text = std::move(shown);
        frame.texts.push_back(std::move(nameText));

        centeredText(cx, viewportH * 0.40f - 28.0f, 2.0f, label, "BASE MAP");
        const std::vector<WorldTemplate>& tmpls = worldgen::templates();
        if (createTemplateIndex_ >= 0 && createTemplateIndex_ < int(tmpls.size())) {
            const WorldTemplate& sel = tmpls[size_t(createTemplateIndex_)];
            centeredText(cx, viewportH * 0.40f + 54.0f, 2.0f, notice,
                         toUpperAscii(sel.description));
        }
    }

    // Status line: create/load feedback, spawn info, and the honest notices
    // set by Host Local Server / Skins.
    if (!menuStatus_.empty() &&
        (ui_ == UiScreen::CreateWorld || ui_ == UiScreen::LoadWorld ||
         ui_ == UiScreen::SpawnMenu || ui_ == UiScreen::Inventory ||
         ui_ == UiScreen::Servers || ui_ == UiScreen::Skins)) {
        centeredText(cx, viewportH * 0.92f, 2.0f, notice, menuStatus_);
    }

    // Direct Connect: address box plus one honest status line.
    if (ui_ == UiScreen::DirectConnect) {
        float boxX = cx - kAddressBoxW / 2;
        float boxY = viewportH * 0.27f;
        centeredText(cx, boxY - 26.0f, 2.0f, label, "SERVER ADDRESS");
        frame.rects.push_back({boxX, boxY, kAddressBoxW, kAddressBoxH,
                               glm::vec4(0.10f, 0.12f, 0.16f, 0.95f)});
        frame.rects.push_back({boxX, boxY + kAddressBoxH - 3.0f, kAddressBoxW, 3.0f,
                               glm::vec4(0.45f, 0.55f, 0.70f, 1.0f)}); // underline accent
        std::string shown = serverAddress_;
        if (std::fmod(menuTime_, 1.0) < 0.55) shown += "_"; // blinking caret
        TextDraw addr;
        addr.x = boxX + 12.0f;
        addr.y = boxY + (kAddressBoxH - 14.0f) / 2;
        addr.scale = 2.0f;
        addr.color = white;
        addr.text = std::move(shown);
        frame.texts.push_back(std::move(addr));

        centeredText(cx, viewportH * 0.72f, 2.0f, dimText,
                     "ONLINE PLAY ARRIVES WITH THE NETWORKING MILESTONE");
        if (!menuStatus_.empty()) {
            centeredText(cx, viewportH * 0.85f, 2.0f, notice, menuStatus_);
        }
    }

    // Skin swatch palette (Skins screen placeholder selection).
    static const glm::vec3 kSkinSwatches[] = {
        {0.85f, 0.30f, 0.28f}, {0.90f, 0.62f, 0.24f}, {0.55f, 0.80f, 0.35f},
        {0.30f, 0.68f, 0.85f}, {0.55f, 0.45f, 0.85f}, {0.90f, 0.90f, 0.92f},
    };

    // Buttons (same layout the hit-testing used this frame).
    std::vector<UiButton> buttons = buildMenuLayout(viewportW, viewportH);
    for (size_t i = 0; i < buttons.size(); ++i) {
        const UiButton& b = buttons[i];
        bool hovered = int(i) == hoveredButton_;

        // Skin swatches paint their own color; a selected one gets a border.
        if (b.id == UiButton::Id::SelectSkinColor) {
            int idx = b.payload >= 0 && b.payload < 6 ? b.payload : 0;
            if (hovered || b.highlighted) {
                frame.rects.push_back({b.x - 4, b.y - 4, b.w + 8, b.h + 8,
                                       glm::vec4(0.95f, 0.97f, 1.0f, 1.0f)});
            }
            frame.rects.push_back({b.x, b.y, b.w, b.h,
                                   glm::vec4(kSkinSwatches[idx], 1.0f)});
            continue;
        }

        glm::vec4 fill = !b.enabled    ? glm::vec4(0.13f, 0.15f, 0.19f, 0.85f)
                       : hovered       ? glm::vec4(0.30f, 0.44f, 0.64f, 0.96f)
                       : b.highlighted ? glm::vec4(0.24f, 0.40f, 0.40f, 0.95f)
                                       : glm::vec4(0.16f, 0.20f, 0.28f, 0.92f);
        frame.rects.push_back({b.x, b.y, b.w, b.h, fill});
        // A left accent stripe on the active button reads as a selection cue.
        if (hovered || b.highlighted) {
            frame.rects.push_back({b.x, b.y, 4.0f, b.h, accent});
        }

        // Game-mode cards: name left-aligned, tagline under it, status badge
        // on the right. Everything else is a plain centered label.
        if (b.id == UiButton::Id::SelectGameMode) {
            const std::vector<GameMode>& modes = gamemodes::all();
            const GameMode* m = (b.payload >= 0 && b.payload < int(modes.size()))
                                    ? &modes[size_t(b.payload)] : nullptr;
            TextDraw name;
            name.x = b.x + 18.0f;
            name.y = b.y + 12.0f;
            name.scale = 2.4f;
            name.color = white;
            name.text = b.label;
            frame.texts.push_back(std::move(name));
            if (m) {
                TextDraw tag;
                tag.x = b.x + 18.0f;
                tag.y = b.y + 38.0f;
                tag.scale = 1.5f;
                tag.color = dimText;
                tag.text = toUpperAscii(m->tagline);
                frame.texts.push_back(std::move(tag));

                bool playable = m->status == ModeStatus::Playable;
                const char* badge = playable ? "PLAYABLE" : "EARLY";
                glm::vec4 badgeCol = playable ? glm::vec4(0.35f, 0.72f, 0.40f, 1.0f)
                                              : glm::vec4(0.85f, 0.62f, 0.25f, 1.0f);
                float bw = float(std::strlen(badge)) * 12.0f + 16.0f;
                float bx = b.x + b.w - bw - 14.0f;
                frame.rects.push_back({bx, b.y + 16.0f, bw, 24.0f,
                                       glm::vec4(0.08f, 0.10f, 0.13f, 0.9f)});
                TextDraw bt;
                bt.centered = true;
                bt.x = bx + bw / 2;
                bt.y = b.y + 21.0f;
                bt.scale = 1.5f;
                bt.color = badgeCol;
                bt.text = badge;
                frame.texts.push_back(std::move(bt));
            }
            continue;
        }

        TextDraw t;
        t.centered = true;
        t.x = b.x + b.w / 2;
        t.scale = 2.0f;
        t.y = b.y + (b.h - 7.0f * t.scale) / 2;
        t.color = b.enabled ? white : glm::vec4(0.55f, 0.58f, 0.64f, 1.0f);
        t.text = b.label;
        frame.texts.push_back(std::move(t));
    }

    if (ui_ != UiScreen::Settings) return;

    // Settings row labels and current values, aligned with buildMenuLayout.
    std::snprintf(buf, sizeof(buf), "%.1f", settings_.mouseSensitivity);
    std::string sens = buf;
    std::snprintf(buf, sizeof(buf), "%d", int(settings_.fovDegrees));
    std::string fov = buf;
    auto pct = [&](float v) {
        std::snprintf(buf, sizeof(buf), "%d%%", int(std::lround(v * 100.0f)));
        return std::string(buf);
    };
    struct Row {
        const char* name;
        std::string value;
    };
    const Row rows[] = {
        {"MOUSE SENSITIVITY", sens},
        {"FOV", fov},
        {"MASTER VOLUME", pct(settings_.masterVolume)},
        {"MUSIC VOLUME", pct(settings_.musicVolume)},
        {"SFX VOLUME", pct(settings_.sfxVolume)},
        {"FULLSCREEN", ""},
        {"VSYNC", ""},
        {"DEBUG HUD", ""},
    };

    float y = viewportH * 0.20f;
    for (const Row& row : rows) {
        TextDraw name;
        name.x = cx - 310;
        name.scale = 2.0f;
        name.y = y + (kRowH - 7.0f * name.scale) / 2;
        name.color = label;
        name.text = row.name;
        frame.texts.push_back(std::move(name));

        if (!row.value.empty()) {
            centeredText(cx + 153.0f, y + (kRowH - 14.0f) / 2, 2.0f, white, row.value);
        }
        y += kRowH;
    }
}
