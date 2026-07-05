#include "client/game.h"

#include "shared/content_loader.h"
#include "shared/log.h"
#include "shared/protocol.h"
#include "shared/world_save.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

// Menu metrics (pixels; the layout centers on the viewport).
constexpr float kBtnW = 300.0f;
constexpr float kBtnH = 48.0f;
constexpr float kBtnGap = 62.0f;
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

    // Placeholder one-shots (original, synthesized - see content/sounds/).
    // The event -> sound-name mapping is client code for now by design.
    if (audio_) {
        const char* names[] = {"footstep", "swing", "gunshot", "pickup", "dummy_hit"};
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

    world_.buildTestMap();
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
    attackCooldown_ = 0.0f;
    swingTimer_ = muzzleFlashTimer_ = hitMarkerTimer_ = 0.0f;
    footstepDistance_ = 0.0f;
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
    currentWorldFile_.clear(); // the test arena is never saved
    currentWorldName_ = "TEST ARENA";
    beginSession();
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

    world_.buildFlatMap();
    currentWorldName_ = worldNameInput_;
    currentWorldFile_ = file;
    saveCurrentWorld();
    eng::logInfo("Created world '%s' at %s", currentWorldName_.c_str(), file.c_str());
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

    world_.buildFlatMap();
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
    eng::logInfo("Loaded world '%s' (%d entities)", currentWorldName_.c_str(),
                 int(world_.entities().size()));
    beginSession();
}

void Game::saveCurrentWorld() {
    if (currentWorldFile_.empty()) return; // test arena
    WorldSaveData data;
    data.displayName = currentWorldName_;
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

void Game::sfx(const char* soundName) const {
    if (audio_) audio_->playSound(soundName);
}

void Game::updateCarriedProp() {
    if (carriedEntityId_ == 0) return;
    WorldEntity* e = world_.entityById(carriedEntityId_);
    if (!e || !e->active) { // destroyed or gone while carried (shouldn't happen)
        carriedEntityId_ = 0;
        return;
    }
    // Float in front of the eye; the prop is non-colliding while carried.
    glm::vec3 hold = eyePosition() + lookDirection() * 2.0f;
    e->pos = hold - glm::vec3(0.0f, e->size.y * 0.5f, 0.0f);
}

void Game::dropCarriedProp() {
    if (carriedEntityId_ == 0) return;
    WorldEntity* e = world_.entityById(carriedEntityId_);
    carriedEntityId_ = 0;
    if (!e) return;
    e->carried = false;

    // Settle onto whatever is below (floor, crate stack); props have no
    // physics, so without this a drop would leave them hanging mid-air.
    glm::vec3 from = e->pos + glm::vec3(0.0f, 0.05f, 0.0f);
    glm::vec3 down(0.0f, -1.0f, 0.0f);
    float t = world_.raycastGeometry(from, down, 50.0f);
    World::EntityHit under = world_.raycastEntities(from, down, t, World::RayMask::AttackTargets);
    if (under.index >= 0) t = under.t;
    if (t > 0.05f && t < 50.0f) e->pos.y -= (t - 0.05f);

    saveCurrentWorld();
    sfx("pickup");
    showToast("DROPPED " + toUpperAscii(e->defId));
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
    return glm::vec3(std::sin(yaw_) * std::cos(pitch_), std::sin(pitch_),
                     -std::cos(yaw_) * std::cos(pitch_));
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

void Game::tryAttack() {
    if (attackCooldown_ > 0.0f) return;
    const WeaponDef* weapon = content_.findWeapon(inventory_.equippedId());
    if (!weapon) return;

    attackCooldown_ = weapon->cooldownSeconds;
    if (weapon->kind == WeaponKind::Hitscan) {
        muzzleFlashTimer_ = 0.07f;
        sfx("gunshot");
    } else {
        swingTimer_ = 0.18f;
        sfx("swing");
    }

    const glm::vec3 eye = eyePosition();
    const glm::vec3 dir = lookDirection();
    float geoT = world_.raycastGeometry(eye, dir, weapon->range);
    World::EntityHit hit =
        world_.raycastEntities(eye, dir, geoT, World::RayMask::AttackTargets);
    if (hit.index < 0) return;

    const WorldEntity& target = world_.entities()[size_t(hit.index)];
    if (target.maxHealth <= 0.0f) return; // solid but not damageable (crates)

    std::string targetId = target.defId;
    hitMarkerTimer_ = 0.12f;
    sfx("dummy_hit");
    bool destroyed = world_.damageEntity(hit.index, weapon->damage);
    aimInteract_ = {};
    aimInfo_ = {}; // entity vector may have shifted; refreshed next frame
    if (destroyed) {
        const EntityDef* def = content_.find(targetId);
        showToast(toUpperAscii(def ? def->displayName : targetId) + " DESTROYED");
    }
}

void Game::showToast(std::string text) {
    hudToast_ = std::move(text);
    hudToastTimer_ = 1.6;
}

void Game::update(const InputState& in, double frameDt, int viewportW, int viewportH) {
    menuTime_ += frameDt;

    if (in.escapePressed) {
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
        case UiScreen::Singleplayer:
        case UiScreen::Multiplayer: // back to the main menu
            menuStatus_.clear();
            ui_ = UiScreen::MainMenu;
            break;
        case UiScreen::CreateWorld:
        case UiScreen::LoadWorld: // back to the singleplayer screen
            menuStatus_.clear();
            ui_ = UiScreen::Singleplayer;
            break;
        case UiScreen::SpawnMenu: // close, back to gameplay
            menuStatus_.clear();
            ui_ = UiScreen::None;
            break;
        case UiScreen::MainMenu:
            break;
        }
    }

    // Q toggles the dev spawn menu, but only in a gameplay context (never
    // while typing in a text field or sitting in the pause/main menus).
    if (in.spawnMenuPressed && inGame_) {
        if (ui_ == UiScreen::None) {
            menuStatus_.clear();
            ui_ = UiScreen::SpawnMenu;
        } else if (ui_ == UiScreen::SpawnMenu) {
            menuStatus_.clear();
            ui_ = UiScreen::None;
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
        // Server-address text entry (only the multiplayer screen has a field).
        if (ui_ == UiScreen::Multiplayer) {
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

    PlayerInput pin;
    pin.moveForward = in.moveForward;
    pin.moveRight = in.moveRight;
    pin.yaw = yaw_;
    pin.crouch = in.crouchHeld;
    pin.walk = in.walkHeld;

    accumulator_ += frameDt;
    int tickSafety = 0;
    while (accumulator_ >= tickDt_ && tickSafety++ < 10) {
        pin.jump = jumpBufferTimer_ > 0.0 || (cfg_.autoBhop != 0.0f && in.jumpHeld);
        prevPos_ = player_.pos;
        player_.tick(pin, world_, cfg_, float(tickDt_));
        world_.tick(float(tickDt_)); // entity respawns + hit-flash decay

        // Footsteps: distance-based cadence, so walking naturally slows the
        // rhythm and standing still is silent.
        if (player_.grounded) {
            footstepDistance_ += player_.horizontalSpeed() * float(tickDt_);
            if (footstepDistance_ >= 2.1f) {
                footstepDistance_ = 0.0f;
                if (player_.horizontalSpeed() > 0.5f) sfx("footstep");
            }
        }

        if (player_.justJumped) jumpBufferTimer_ = -1.0;
        jumpBufferTimer_ -= tickDt_;
        accumulator_ -= tickDt_;
    }
    if (tickSafety >= 10) accumulator_ = 0.0; // spiral-of-death guard

    float targetEye = player_.height(cfg_) - cfg_.eyeOffset;
    eyeHeight_ += (targetEye - eyeHeight_) * std::min(1.0f, cfg_.crouchTransitionSpeed * float(frameDt));

    // Combat and interaction (per frame, like mouse look, for minimal latency).
    attackCooldown_ = std::max(0.0f, attackCooldown_ - float(frameDt));
    swingTimer_ = std::max(0.0f, swingTimer_ - float(frameDt));
    muzzleFlashTimer_ = std::max(0.0f, muzzleFlashTimer_ - float(frameDt));
    hitMarkerTimer_ = std::max(0.0f, hitMarkerTimer_ - float(frameDt));
    if (hudToastTimer_ > 0.0) hudToastTimer_ -= frameDt;

    updateCarriedProp(); // held prop follows the camera
    updateAimTarget();
    if (in.slotPressed > 0) inventory_.selectSlot(in.slotPressed - 1);
    if (in.interactPressed) tryInteract();
    if (in.attackPressed) tryAttack();
}

void Game::activateButton(UiButton::Id id, int payload) {
    using Id = UiButton::Id;
    bool settingsChanged = false;
    switch (id) {
    // Navigation.
    case Id::OpenSingleplayer: ui_ = UiScreen::Singleplayer; menuStatus_.clear(); break;
    case Id::OpenMultiplayer: ui_ = UiScreen::Multiplayer; menuStatus_.clear(); break;
    case Id::OpenSettings: settingsReturn_ = ui_; ui_ = UiScreen::Settings; break;
    case Id::BackToMain: ui_ = UiScreen::MainMenu; menuStatus_.clear(); break;
    case Id::BackToSingleplayer: ui_ = UiScreen::Singleplayer; menuStatus_.clear(); break;
    case Id::Back: saveSettings(); ui_ = settingsReturn_; break;
    case Id::QuitApp: quitRequested_ = true; break;

    // Singleplayer.
    case Id::StartTestWorld: startTestWorld(); break;
    case Id::OpenCreateWorld:
        worldNameInput_.clear();
        menuStatus_.clear();
        ui_ = UiScreen::CreateWorld;
        break;
    case Id::OpenLoadWorld:
        refreshWorldList();
        menuStatus_ = worldList_.empty() ? "NO WORLDS YET - CREATE ONE FIRST" : "";
        ui_ = UiScreen::LoadWorld;
        break;

    // Create/load world screens.
    case Id::ConfirmCreateWorld: createWorld(); break;
    case Id::LoadWorldEntry: loadWorld(payload); break;

    // Multiplayer (networking is a later milestone; protocol.h reserves v1).
    case Id::SelectMpTab:
        mpTab_ = payload;
        menuStatus_.clear();
        break;
    case Id::Connect:
    case Id::QuickLocalhost:
        if (id == Id::QuickLocalhost) serverAddress_ = "127.0.0.1:27015";
        menuStatus_ = "CONNECTION SYSTEM NOT IMPLEMENTED YET - NETWORKING IS A LATER MILESTONE";
        break;

    // Pause menu.
    case Id::Resume: ui_ = UiScreen::None; break;
    case Id::QuitToMenu:
        dropCarriedProp(); // settle + persist a held prop before leaving
        saveCurrentWorld();
        currentWorldFile_.clear();
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
        column(viewportH * 0.38f, {
            {Id::OpenSingleplayer, 0, 0, 0, 0, "SINGLEPLAYER", true},
            {Id::OpenMultiplayer, 0, 0, 0, 0, "MULTIPLAYER", true},
            {Id::OpenSettings, 0, 0, 0, 0, "SETTINGS", true},
            {Id::QuitApp, 0, 0, 0, 0, "QUIT", true},
        });
        break;

    case UiScreen::Singleplayer:
        column(viewportH * 0.38f, {
            {Id::StartTestWorld, 0, 0, 0, 0, "START TEST WORLD", true},
            {Id::OpenCreateWorld, 0, 0, 0, 0, "CREATE WORLD", true},
            {Id::OpenLoadWorld, 0, 0, 0, 0, "LOAD WORLD", true},
            {Id::BackToMain, 0, 0, 0, 0, "BACK", true},
        });
        break;

    case UiScreen::CreateWorld:
        column(viewportH * 0.42f, {
            {Id::ConfirmCreateWorld, 0, 0, 0, 0, "CREATE", true},
            {Id::BackToSingleplayer, 0, 0, 0, 0, "BACK", true},
        });
        break;

    case UiScreen::LoadWorld: {
        float y = viewportH * 0.30f;
        int shown = std::min(int(worldList_.size()), kMaxWorldButtons);
        for (int i = 0; i < shown; ++i) {
            buttons.push_back({Id::LoadWorldEntry, cx - kBtnW / 2, y, kBtnW, kBtnH,
                               toUpperAscii(worldList_[size_t(i)].displayName), true, i});
            y += kBtnGap;
        }
        y += 18.0f;
        buttons.push_back({Id::BackToSingleplayer, cx - kBtnW / 2, y, kBtnW, kBtnH,
                           "BACK", true});
        break;
    }

    case UiScreen::SpawnMenu: {
        // GMod-style: a row of category tabs, entries of the open tab below.
        static const char* kTabNames[] = {"WEAPONS", "BOTS", "PROPS", "PICKUPS"};
        constexpr int kTabCount = 4;
        const float tabW = 150.0f, tabH = 40.0f, tabGap = 10.0f;
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

    case UiScreen::Multiplayer: {
        // CS-style server browser shell. The tabs are real; the lists are
        // honestly empty until server discovery exists (no fake servers).
        static const char* kMpTabs[] = {"FAVORITES", "LAN", "HISTORY", "DIRECT CONNECT"};
        constexpr int kMpTabCount = 4;
        const float tabW = 170.0f, tabH = 40.0f, tabGap = 10.0f;
        float tabX = cx - (kMpTabCount * tabW + (kMpTabCount - 1) * tabGap) / 2;
        const float tabY = viewportH * 0.22f;
        for (int i = 0; i < kMpTabCount; ++i) {
            buttons.push_back({Id::SelectMpTab, tabX, tabY, tabW, tabH,
                               kMpTabs[i], true, i, i == mpTab_});
            tabX += tabW + tabGap;
        }

        if (mpTab_ == 3) { // Direct Connect: address box + actions
            column(viewportH * 0.46f, {
                {Id::Connect, 0, 0, 0, 0, "CONNECT", true},
                {Id::QuickLocalhost, 0, 0, 0, 0, "LOCALHOST 127.0.0.1", true},
                {Id::BackToMain, 0, 0, 0, 0, "BACK", true},
            });
        } else {
            column(viewportH * 0.62f, {
                {Id::BackToMain, 0, 0, 0, 0, "BACK", true},
            });
        }
        break;
    }

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
        buttons.push_back({Id::Back, cx - kBtnW / 2, y, kBtnW, kBtnH, "BACK", true});
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

    if (inGame_) {
        // First-person camera: interpolate between the last two ticks.
        float alpha = float(std::clamp(accumulator_ / tickDt_, 0.0, 1.0));
        glm::vec3 renderPos = glm::mix(prevPos_, player_.pos, alpha);
        glm::vec3 eye = renderPos + glm::vec3(0.0f, eyeHeight_, 0.0f);
        glm::vec3 lookDir(std::sin(yaw_) * std::cos(pitch_), std::sin(pitch_),
                          -std::cos(yaw_) * std::cos(pitch_));
        frame.view = glm::lookAt(eye, eye + lookDir, glm::vec3(0.0f, 1.0f, 0.0f));
    } else {
        // Menu backdrop: a slow orbit over the arena.
        float t = float(menuTime_) * 0.08f;
        glm::vec3 eye(std::sin(t) * 16.0f, 9.0f, std::cos(t) * 16.0f);
        frame.view = glm::lookAt(eye, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    float aspect = viewportH > 0 ? float(viewportW) / float(viewportH) : 1.0f;
    frame.proj = glm::perspective(glm::radians(settings_.fovDegrees), aspect, 0.05f, 300.0f);

    // World geometry.
    frame.boxes.reserve(world_.boxes().size() + world_.entities().size());
    for (const WorldBox& wb : world_.boxes()) {
        BoxDraw draw;
        draw.center = (wb.box.min + wb.box.max) * 0.5f;
        draw.size = wb.box.max - wb.box.min;
        draw.color = wb.color;
        draw.checkerTop = wb.checkerTop;
        frame.boxes.push_back(draw);
    }

    // Spawned sandbox entities (pickups, bots, props). Multi-part visuals
    // come from the content def; collision stays the plain entity AABB.
    for (size_t ei = 0; ei < world_.entities().size(); ++ei) {
        const WorldEntity& e = world_.entities()[ei];
        if (!e.active) continue; // picked up / destroyed, awaiting respawn

        glm::vec3 base = e.pos;
        if (!e.weaponId.empty()) {
            // Weapon pickups hover and bob slightly so they read as items.
            base.y += 0.06f + 0.04f * float(std::sin(menuTime_ * 2.5 + double(ei) * 1.7));
        }
        float flash = glm::clamp(e.hitFlash / 0.15f, 0.0f, 1.0f);

        const EntityDef* def = content_.find(e.defId);
        if (def && !def->visual.empty()) {
            for (const VisualPart& part : def->visual) {
                BoxDraw draw;
                draw.center = base + part.offset;
                draw.size = part.size;
                draw.color = glm::mix(part.color, glm::vec3(1.0f, 0.15f, 0.1f), flash);
                frame.boxes.push_back(draw);
            }
        } else {
            AABB b = e.bounds();
            BoxDraw draw;
            draw.center = (b.min + b.max) * 0.5f + (base - e.pos);
            draw.size = b.max - b.min;
            draw.color = glm::mix(e.color, glm::vec3(1.0f, 0.15f, 0.1f), flash);
            frame.boxes.push_back(draw);
        }
    }

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
        addLine(glm::vec4(0.7f, 0.75f, 0.8f, 1.0f),
                "ESC PAUSE  Q SPAWN  E PICK UP  LMB ATTACK  1-3 WEAPONS  F5 CFG  F1 HUD");

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

    TextDraw crosshair;
    crosshair.centered = true;
    crosshair.x = viewportW * 0.5f;
    crosshair.y = viewportH * 0.5f - 7.0f;
    crosshair.scale = 2.0f;
    crosshair.color = glm::vec4(1.0f, 1.0f, 1.0f, 0.9f);
    crosshair.text = "+";
    frame.texts.push_back(std::move(crosshair));

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
        std::snprintf(info, sizeof(info), "%s  %d/%d",
                      toUpperAscii(def ? def->displayName : e.defId).c_str(),
                      int(std::ceil(e.health)), int(e.maxHealth));
        centeredLine(viewportH * 0.5f + 34.0f, 2.0f, dim, info);
    }

    // Transient gameplay message (pickups, kills).
    if (hudToastTimer_ > 0.0 && !hudToast_.empty()) {
        centeredLine(viewportH * 0.5f + 60.0f, 2.0f, glm::vec4(0.6f, 1.0f, 0.65f, 1.0f),
                     hudToast_);
    }

    // Weapon bar: owned weapons with the equipped one bracketed.
    std::string bar;
    for (size_t i = 0; i < inventory_.weapons.size(); ++i) {
        const WeaponDef* w = content_.findWeapon(inventory_.weapons[i]);
        std::string name = toUpperAscii(w ? w->displayName : inventory_.weapons[i]);
        bool eq = int(i) == inventory_.equipped;
        if (!bar.empty()) bar += "   ";
        bar += (eq ? "[" : "") + std::to_string(i + 1) + " " + name + (eq ? "]" : "");
    }
    centeredLine(viewportH - 46.0f, 2.0f, white, bar);
}

// First-person hands + held weapon, composed from screen-space rects
// (Doom-style, anchored at the bottom edge). Purely cosmetic placeholder:
// no art pipeline, no rotation - just enough that fists read as fists, the
// karambit as a claw in a fist, and the glock as a pistol you sight over.
void Game::appendViewmodelDraws(RenderFrame& frame) const {
    const float vw = float(frame.viewportW);
    const float vh = float(frame.viewportH);
    const float cx = vw * 0.5f;

    const glm::vec4 skin(0.85f, 0.66f, 0.52f, 1.0f);
    const glm::vec4 sleeve(0.24f, 0.28f, 0.34f, 1.0f);
    const glm::vec4 metal(0.14f, 0.15f, 0.18f, 1.0f);
    const glm::vec4 metalLight(0.26f, 0.28f, 0.32f, 1.0f);
    const glm::vec4 silver(0.80f, 0.84f, 0.90f, 1.0f);

    // Swing animation: 0 -> 1 over the melee cooldown flash, arcing up-left.
    float swing = swingTimer_ > 0.0f ? 1.0f - swingTimer_ / 0.18f : 0.0f;
    float arc = std::sin(swing * 3.14159f);
    // Pistol recoil kick, decaying with the muzzle flash.
    float kick = muzzleFlashTimer_ > 0.0f ? muzzleFlashTimer_ / 0.07f : 0.0f;

    auto rect = [&](float x, float y, float w, float h, const glm::vec4& c) {
        frame.rects.push_back({x, y, w, h, c});
    };

    const std::string& weapon = inventory_.equippedId();
    if (weapon == "glock") {
        float gx = cx + 70.0f;          // gun center x
        float gy = vh - 168.0f + kick * 12.0f;
        // Hands gripping from both sides, forearms running off-screen.
        rect(gx - 52.0f, gy + 64.0f, 44.0f, 60.0f, skin);
        rect(gx + 8.0f, gy + 64.0f, 44.0f, 60.0f, skin);
        rect(gx - 60.0f, gy + 118.0f, 56.0f, vh, sleeve);
        rect(gx + 4.0f, gy + 118.0f, 56.0f, vh, sleeve);
        // Grip and back of the slide (what you see when sighting a pistol).
        rect(gx - 20.0f, gy + 36.0f, 40.0f, 44.0f, metalLight);
        rect(gx - 26.0f, gy, 52.0f, 40.0f, metal);
        rect(gx - 4.0f, gy - 8.0f, 8.0f, 10.0f, metalLight); // rear sight
        if (muzzleFlashTimer_ > 0.0f) {
            rect(gx - 16.0f, gy - 42.0f, 32.0f, 30.0f, {1.0f, 0.85f, 0.30f, 0.9f});
            rect(gx - 7.0f, gy - 34.0f, 14.0f, 16.0f, {1.0f, 1.0f, 0.80f, 0.95f});
        }
    } else if (weapon == "karambit") {
        // Right fist holding the claw; the whole cluster arcs on a swing.
        float fx = cx + 150.0f - arc * 90.0f;
        float fy = vh - 120.0f - arc * 110.0f;
        rect(fx, fy, 62.0f, 62.0f, skin);
        rect(fx + 4.0f, fy + 56.0f, 54.0f, vh, sleeve);
        rect(fx + 14.0f, fy - 10.0f, 18.0f, 14.0f, silver);  // finger ring
        rect(fx - 34.0f, fy + 10.0f, 36.0f, 13.0f, silver);  // blade root
        rect(fx - 58.0f, fy + 20.0f, 28.0f, 11.0f, silver);  // blade mid
        rect(fx - 74.0f, fy + 32.0f, 20.0f, 9.0f, silver);   // curving tip
    } else { // fists (and any unknown weapon id)
        float lx = cx - 210.0f;
        float rx = cx + 148.0f - arc * 110.0f; // right fist throws the punch
        float ry = vh - 104.0f - arc * 130.0f;
        rect(lx, vh - 104.0f, 62.0f, 62.0f, skin);
        rect(lx + 4.0f, vh - 48.0f, 54.0f, vh, sleeve);
        rect(rx, ry, 62.0f, 62.0f, skin);
        rect(rx + 4.0f, ry + 56.0f, 54.0f, vh, sleeve);
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

    // Titles.
    switch (ui_) {
    case UiScreen::MainMenu:
        centeredText(cx, viewportH * 0.16f, 6.0f, white, "TACMOVE");
        centeredText(cx, viewportH * 0.16f + 52.0f, 2.0f, dimText,
                     "EARLY SANDBOX FOUNDATION - PHASE 1");
        std::snprintf(buf, sizeof(buf), "PROTOCOL V%d - NETWORKING NOT IMPLEMENTED YET",
                      net::kProtocolVersion);
        centeredText(cx, viewportH - 34.0f, 2.0f, dimText, buf);
        break;
    case UiScreen::Singleplayer:
        centeredText(cx, viewportH * 0.24f, 4.0f, white, "SINGLEPLAYER");
        break;
    case UiScreen::CreateWorld:
        centeredText(cx, viewportH * 0.16f, 4.0f, white, "CREATE WORLD");
        break;
    case UiScreen::LoadWorld:
        centeredText(cx, viewportH * 0.18f, 4.0f, white, "LOAD WORLD");
        break;
    case UiScreen::SpawnMenu:
        centeredText(cx, viewportH * 0.14f, 4.0f, white, "SPAWN MENU");
        centeredText(cx, viewportH * 0.14f + 42.0f, 2.0f, dimText,
                     "DEV / ADMIN - Q OR ESC CLOSES");
        if (spawnableDefs(ContentCategory(spawnCategory_)).empty()) {
            centeredText(cx, viewportH * 0.45f, 2.0f, dimText,
                         "NOTHING IN THIS CATEGORY YET");
        }
        break;
    case UiScreen::Multiplayer:
        centeredText(cx, viewportH * 0.13f, 4.0f, white, "MULTIPLAYER");
        break;
    case UiScreen::Settings:
        centeredText(cx, viewportH * 0.10f, 4.0f, white, "SETTINGS");
        break;
    case UiScreen::PauseMain:
        centeredText(cx, viewportH * 0.28f, 4.0f, white, "PAUSED");
        break;
    case UiScreen::None:
        break;
    }

    // Create World: world-name input box (always focused on this screen).
    if (ui_ == UiScreen::CreateWorld) {
        float boxX = cx - kAddressBoxW / 2;
        float boxY = viewportH * 0.28f;
        centeredText(cx, boxY - 26.0f, 2.0f, label, "WORLD NAME");
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
    }

    // Status line for the sandbox screens (create/load feedback, spawn info).
    if (!menuStatus_.empty() &&
        (ui_ == UiScreen::CreateWorld || ui_ == UiScreen::LoadWorld ||
         ui_ == UiScreen::SpawnMenu)) {
        centeredText(cx, viewportH * 0.88f, 2.0f, notice, menuStatus_);
    }

    // Multiplayer: tab body. Direct Connect has the address box; the list
    // tabs state plainly that nothing is discoverable yet - no fake servers.
    if (ui_ == UiScreen::Multiplayer) {
        if (mpTab_ == 3) {
            float boxX = cx - kAddressBoxW / 2;
            float boxY = viewportH * 0.32f;
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
        } else {
            const char* line1 = "";
            const char* line2 = "";
            switch (mpTab_) {
            case 0:
                line1 = "NO FAVORITES YET";
                line2 = "SERVERS CAN BE FAVORITED ONCE JOINING THEM EXISTS";
                break;
            case 1:
                line1 = "NO SERVERS FOUND";
                line2 = "LAN DISCOVERY IS NOT IMPLEMENTED YET";
                break;
            default:
                line1 = "NO HISTORY";
                line2 = "PAST CONNECTIONS WILL SHOW UP HERE";
                break;
            }
            centeredText(cx, viewportH * 0.40f, 2.5f, dimText, line1);
            centeredText(cx, viewportH * 0.40f + 30.0f, 2.0f, dimText, line2);
        }

        if (!menuStatus_.empty()) {
            centeredText(cx, viewportH * 0.85f, 2.0f, notice, menuStatus_);
        }
    }

    // Buttons (same layout the hit-testing used this frame).
    std::vector<UiButton> buttons = buildMenuLayout(viewportW, viewportH);
    for (size_t i = 0; i < buttons.size(); ++i) {
        const UiButton& b = buttons[i];
        bool hovered = int(i) == hoveredButton_;
        glm::vec4 fill = !b.enabled    ? glm::vec4(0.13f, 0.15f, 0.19f, 0.85f)
                       : hovered       ? glm::vec4(0.36f, 0.46f, 0.60f, 0.95f)
                       : b.highlighted ? glm::vec4(0.28f, 0.42f, 0.38f, 0.95f)
                                       : glm::vec4(0.18f, 0.22f, 0.30f, 0.92f);
        frame.rects.push_back({b.x, b.y, b.w, b.h, fill});

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
