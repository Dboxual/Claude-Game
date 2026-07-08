#pragma once

#include "client/platform/audio.h"
#include "client/platform/filesystem.h"
#include "client/platform/input.h"
#include "client/renderer/render_types.h"
#include "client/settings.h"
#include "shared/config.h"
#include "shared/content.h"
#include "shared/crafting.h"
#include "shared/game_mode.h"
#include "shared/inventory.h"
#include "shared/items.h"
#include "shared/melee.h"
#include "shared/player.h"
#include "shared/world.h"

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>
#include <vector>

// The whole client behind the platform seams: main menu shell, pause menu,
// settings, and the fixed-tick movement sim. Consumes InputState, emits a
// RenderFrame. Contains no windowing, graphics-API, or OS calls - that is
// what keeps the client portable and the dedicated server free of UI code.
//
// UI flow (game-hub shell - "game modes are the games inside the game"):
//   MainMenu -> Play    -> Game Modes -> MiniCS / Deathmatch / Sandbox / ...
//                       -> Servers    -> Server List | Direct Connect | Host
//                       -> Recent Games
//            -> Create  -> Create Map | Edit Map | Skins | Publish
//            -> Profile
//            -> Settings -> back to wherever it was opened from
//   gameplay -> ESC -> PauseMain -> Resume | Settings | Quit to Menu
//   gameplay -> B   -> SpawnMenu (build/spawn tools; a Sandbox feature)
class Game {
public:
    // audio may be null (headless tests); sounds become no-ops.
    bool init(IFileSystem& fs, IAudio* audio = nullptr);
    void update(const InputState& in, double frameDt, int viewportW, int viewportH);
    RenderFrame buildRenderFrame(int viewportW, int viewportH) const;

    // For the composition root: UI state drives mouse capture and text
    // input, quit ends the loop, dirty settings get re-applied.
    bool uiActive() const { return ui_ != UiScreen::None; }
    bool wantsTextInput() const {
        return ui_ == UiScreen::DirectConnect || ui_ == UiScreen::CreateWorld;
    }
    bool quitRequested() const { return quitRequested_; }
    const GameSettings& settings() const { return settings_; }
    bool settingsDirty() const { return settingsDirty_; }
    void clearSettingsDirty() { settingsDirty_ = false; }
    void overrideVsync(bool enabled) { settings_.vsync = enabled; settingsDirty_ = true; }

    // Dev flags: skip the main menu (--world), start paused in-world
    // (--paused), or start holding a weapon (--equip <id>, viewmodel work).
    void startInWorld() { startTestWorld(); }
    void startInWorldPaused() { startTestWorld(); ui_ = UiScreen::PauseMain; }
    void startInWorldEquipped(const std::string& weaponId) {
        startTestWorld();
        if (content_.findWeapon(weaponId)) inventory_.acquireAndEquip(weaponId);
    }
    // --mode <id>: boot straight into a game mode (dev/testing + screenshots).
    void startInMode(const std::string& modeId) {
        if (const GameMode* m = gamemodes::find(modeId)) startGameMode(*m);
        else startTestWorld();
    }
    // --menu <screen>: boot onto a specific menu screen (dev/screenshots only).
    void debugOpenScreen(const std::string& name);

    // Purely informational (shown on the debug HUD).
    void setRendererName(std::string name) { rendererName_ = std::move(name); }

private:
    enum class UiScreen {
        None, MainMenu, Play, GameModes, Servers, ServerList, DirectConnect,
        RecentGames, Create, CreateWorld, LoadWorld, Skins, Publish, Profile,
        Settings, PauseMain, SpawnMenu, Inventory,
    };

    struct UiButton {
        enum class Id {
            // main menu
            LaunchMiniCS, OpenPlay, OpenCreate, OpenProfile, OpenSettings, QuitApp,
            // play submenu
            OpenGameModes, OpenServers, OpenRecentGames,
            // game modes list (payload = index into gamemodes::all())
            SelectGameMode,
            // servers submenu
            OpenServerList, OpenDirectConnect, HostLocalServer,
            Connect, QuickLocalhost,
            // recent games
            QuickLaunchRecent,
            // create submenu
            OpenCreateMap, OpenEditMap, OpenSkins, OpenPublish,
            // create/edit map screens
            ConfirmCreateWorld, LoadWorldEntry, SelectWorldType,
            // skins placeholder
            SelectSkinColor,
            // navigation (pops one level, mirrors ESC)
            NavBack,
            // pause menu
            Resume, QuitToMenu,
            // dev spawn / build menu
            SelectSpawnCategory, SpawnEntity, CloseSpawnMenu,
            // inventory / crafting screen
            InvEquipItem, CraftRecipe, CloseInventory,
            // settings screen
            ResetDefaults,
            SensDown, SensUp, FovDown, FovUp,
            MasterDown, MasterUp, MusicDown, MusicUp, SfxDown, SfxUp,
            ToggleFullscreen, ToggleVsync, ToggleHud,
        };
        Id id;
        float x, y, w, h;
        std::string label;        // drawn centered inside the rect
        bool enabled = true;      // disabled = greyed out, no hover, no click
        int payload = -1;         // list index / category for payload buttons
        bool highlighted = false; // e.g. the selected spawn-menu category tab
    };

    struct WorldListEntry {
        std::string folder;      // directory name under worlds/
        std::string displayName; // 'name' from the save file
    };

    void loadMovementConfig();
    bool loadSettings(); // returns whether a settings file existed
    void saveSettings();
    void startTestWorld();
    void startGameMode(const GameMode& mode); // build mode's map + loadout, play
    void goBack();       // pop one UI level (shared by ESC and Back buttons)
    void beginSession(); // respawn + reset timers/inventory, drop into gameplay
    void createWorld();
    void loadWorld(int listIndex);
    void saveCurrentWorld(); // no-op for the unsaved test arena
    void refreshWorldList();
    void spawnFromMenu(int spawnableIndex);
    std::vector<const EntityDef*> spawnableDefs(ContentCategory category) const;

    // Combat / interaction (gameplay only).
    glm::vec3 eyePosition() const;
    glm::vec3 lookDirection() const;
    void updateAimTarget();
    void tryInteract();

    // RPG systems. The weapon actually in the player's hands is the item
    // inventory's main-hand item when one is equipped, else the legacy
    // sandbox weapon slot - heldWeaponId() is the single truth every combat
    // and viewmodel path reads.
    std::string heldWeaponId() const;
    void tryGather(int entityIndex);       // E on a resource node
    void collectGroundItem(int entityIndex); // E on a dropped item
    void updateMobs(float dt);             // hostile chase + contact strikes
    void spawnLootDrop(const std::string& deadDefId, glm::vec3 pos);
    bool thirdPersonActive() const;        // player preference under the policy
    void tryAttack(); // hitscan weapons only; melee runs through updateMelee
    void updateCarriedProp();
    void dropCarriedProp();
    void showToast(std::string text);
    void sfx(const char* soundName, float pitch = 1.0f) const; // no-op without audio
    float rand01(); // cheap deterministic rng for sound/fx variation

    // Melee combat (client side of shared/melee: inputs, targeting, feedback).
    void updateMelee(const InputState& in, float dt);
    void performMeleeStrike(const WeaponDef& weapon);
    void updateBots(float dt);
    void updateThrownWeapons(float dt);
    void tryThrowWeapon();
    void spawnDroppedWeapon(const std::string& weaponId, glm::vec3 nearPos);
    void addSparks(const glm::vec3& pos, int count, const glm::vec3& color);
    void updateSparks(float dt);
    void triggerShake(float amplitude, float seconds);
    void applyBotStrikeOnPlayer(melee::Actor& bot);

    // --- MiniCS mode: the flagship round-based FPS loop. Every MiniCS path is
    // gated on minicsActive_, so the sandbox and other modes stay untouched. --
    void placeMinicsEntities();          // arena loadout: spare Glock + enemy spawns
    void startMinicsRound();             // (re)scan enemies, reset round state
    void updateMinics(float dt);         // round timer, countdown, win/lose
    void updateMinicsEnemies(float dt);  // enemy movement / aim / fire brains
    void minicsEnemyFire(WorldEntity& e); // one enemy shot at the player
    void minicsWin();
    void minicsLose(const char* reason);
    void onMinicsEnemyKilled(const glm::vec3& at, bool headshot, float yaw,
                             const glm::vec3& knockDir); // score + kill feedback
    void addScorePopup(std::string text, const glm::vec3& color);
    void appendMinicsEnemyDraws(RenderFrame& frame) const; // animated enemies
    void appendMinicsHud(RenderFrame& frame) const;        // ammo/score/round HUD

    // --- Glock feel: client-side ammo + a recovering recoil view-punch. -----
    bool usesAmmo() const;               // true while a Glock is in hand
    void tryReload();

    void loadContentFiles(); // server/content/ defs replace matching builtins

    std::vector<UiButton> buildMenuLayout(int viewportW, int viewportH) const;
    void activateButton(UiButton::Id id, int payload = -1);
    void appendMenuDraws(RenderFrame& frame) const;
    void appendHudDraws(RenderFrame& frame) const;
    void appendViewmodelDraws(RenderFrame& frame) const;

    IFileSystem* fs_ = nullptr;
    IAudio* audio_ = nullptr;
    std::string cfgPath_;
    std::string serverRoot_; // "server/" resolved like the config dir
    std::string settingsPath_;
    ConfigFile cfgFile_;
    MovementConfig cfg_;
    GameSettings settings_;
    bool settingsDirty_ = false;

    World world_;
    Player player_;
    ContentRegistry content_;
    ItemRegistry items_;
    RecipeRegistry recipes_;
    glm::vec3 prevPos_{0.0f}; // position before the latest tick, for interpolation

    // Sandbox world session. currentWorldFile_ empty = test arena (not saved).
    std::string worldsRoot_; // "worlds/" resolved next to the config dir
    std::string currentWorldFile_;
    std::string currentWorldName_;
    std::vector<WorldListEntry> worldList_; // refreshed on entering LoadWorld
    std::string worldNameInput_;            // CreateWorld text field
    int createTemplateIndex_ = 0;           // world type chosen on Create World
    std::string currentWorldType_;          // template id of the live world (saved)

    // Combat / interaction state (reset by beginSession).
    Inventory inventory_;   // legacy sandbox weapon slots (1-9)
    ItemInventory itemInv_; // RPG item stacks + main-hand equipment

    // Hostile mob timers, keyed by stable entity id (same pattern as bots_).
    struct MobBrain {
        float attackCooldown = 0.0f;
    };
    std::unordered_map<unsigned, MobBrain> mobs_;
    float attackCooldown_ = 0.0f;   // hitscan fire interval (melee paces itself)
    float muzzleFlashTimer_ = 0.0f; // pistol-fire viewmodel flash/recoil
    float slideCycleTimer_ = 0.0f;  // visible Glock slide snap after a shot
    float dryFireTimer_ = 0.0f;     // empty-trigger twitch / click feedback
    float reloadBeatTimer_ = 0.0f;  // short "mag seated / ready" flash
    float hitMarkerTimer_ = 0.0f;   // crosshair hit confirmation
    float footstepDistance_ = 0.0f; // meters walked since the last step sound
    float bobPhase_ = 0.0f;         // walk-cycle phase driving viewmodel bob
    float bobIntensity_ = 0.0f;     // 0..~1, eased so stopping settles the hands
    float equipTimer_ = 0.0f;       // weapon-raise animation after switching
    std::string lastEquippedId_;    // change detection for the equip animation

    // Melee combat state. playerMelee_ is the shared-module actor the future
    // server would own; everything below it is client presentation.
    melee::Actor playerMelee_;
    melee::Phase prevMeleePhase_ = melee::Phase::Idle; // sound/anim edge detection
    bool hasShield_ = false;
    bool nextSlashLeft_ = false;    // left click alternates slash sides
    bool heavyEligible_ = false;    // current windup was started by LMB (hold = heavy)

    struct ThrownWeapon {
        std::string weaponId;
        glm::vec3 pos{0.0f};
        glm::vec3 vel{0.0f};
        float spin = 0.0f; // end-over-end rotation, radians
        float life = 0.0f;
    };
    std::vector<ThrownWeapon> thrown_;

    struct Spark {
        glm::vec3 pos{0.0f};
        glm::vec3 vel{0.0f};
        glm::vec3 color{1.0f};
        float life = 0.0f; // counts down to 0
        float total = 0.3f;
        float size = 1.0f; // draw-size multiplier (impact flashes are bigger)
    };
    std::vector<Spark> sparks_;

    // A duelist bot's combat brain, keyed by stable entity id. The actor is
    // the same shared struct the player uses; the few timers around it are
    // the "no full AI yet" decision loop.
    struct BotBrain {
        melee::Actor actor;
        float decideIn = 1.5f;  // seconds until it may start the next attack
        float guardLeft = 0.0f; // >0 = holding its guard up
        unsigned rng = 0x9E3779B9u;
    };
    std::unordered_map<unsigned, BotBrain> bots_;

    // Combat feedback (presentation only).
    float hitStop_ = 0.0f;      // impact pause: sim time crawls while > 0
    float shakeTime_ = 0.0f;    // screen shake remaining / total / strength
    float shakeTotal_ = 1.0f;
    float shakeAmp_ = 0.0f;
    float damageFlash_ = 0.0f;  // red vignette after the player is struck

    // Player health (bot strikes deal real damage now; going down respawns
    // you). healthShown_ trails health_ so the bar visibly drains.
    float playerHealth_ = 100.0f;
    float playerHealthShown_ = 100.0f;
    float sinceDamaged_ = 999.0f; // seconds since the last hit (regen gate + bar flash)
    void damagePlayer(float amount);

    // Bullet tracers (Glock): a short bright streak that travels from the
    // muzzle to the impact point at a visible (not instant) speed, then
    // pops an impact puff where it lands.
    struct Tracer {
        glm::vec3 from{0.0f};
        glm::vec3 to{0.0f};
        glm::vec3 color{1.0f, 0.92f, 0.60f}; // player = warm gold, enemy = red
        float life = 0.0f;   // counts down; 0 = arrived
        float total = 0.06f; // travel time = distance / tracer speed
        bool impact = false; // ends on a surface: puff on arrival
    };
    std::vector<Tracer> tracers_;

    // --- MiniCS round state (only meaningful while minicsActive_). -----------
    enum class MiniCSPhase { Countdown, Live, Won, Lost };

    // One ranged enemy's brain, keyed by the entity's stable id. The state
    // drives BOTH the decision loop and the walk/aim/shoot/flinch animation.
    struct MinicsEnemy {
        enum class State { Advance, Strafe, Aim, Fire, Flinch };
        State state = State::Advance;
        float stateTimer = 0.0f;   // seconds until the next decision
        float fireCooldown = 1.0f; // gate between shots
        float flinchTimer = 0.0f;  // >0 = reeling from a hit (interrupts aim)
        float aimBlend = 0.0f;     // 0..1 eased weapon-raise for the aim pose
        float walkPhase = 0.0f;    // leg/torso walk-cycle phase
        float walkAmt = 0.0f;      // 0..1 eased "am I moving" for the bob
        float strafeSign = 1.0f;   // +1 / -1 current strafe side
        float yaw = 0.0f;          // facing, radians (smoothed)
        float muzzle = 0.0f;       // >0 = muzzle-flash / gun-recoil after firing
        float laneX = 0.0f;        // preferred flank lane around mid cover
        float laneDrift = 0.0f;    // small per-enemy offset to avoid bunching
        float repathTimer = 0.0f;  // lane-switch cadence after blocked movement
        unsigned rng = 0x9E3779B9u;
    };
    std::unordered_map<unsigned, MinicsEnemy> minicsBrains_;

    // Short scripted death silhouettes. MiniCS enemies are erased from the
    // shared world on death, then this client-only visual sells the fall.
    struct MinicsDeathFx {
        glm::vec3 pos{0.0f};
        glm::vec3 knockDir{0.0f, 0.0f, 1.0f};
        float yaw = 0.0f;
        float life = 0.0f;
        float total = 0.95f;
        bool headshot = false;
    };
    std::vector<MinicsDeathFx> minicsDeaths_;

    bool minicsActive_ = false;
    MiniCSPhase minicsPhase_ = MiniCSPhase::Countdown;
    float minicsPhaseTimer_ = 0.0f;   // countdown secs, then round secs remaining
    float minicsBannerTimer_ = 0.0f;  // pulse for the "FIGHT" / result banner
    int minicsScore_ = 0;
    int minicsKills_ = 0;
    int minicsHeadshots_ = 0;
    int minicsEnemiesTotal_ = 0;
    int minicsEnemiesAlive_ = 0;
    std::string minicsResultNote_;    // end-screen subtitle (loss reason)
    float killMarkerTimer_ = 0.0f;    // bigger crosshair X + kill confirm
    float headshotMarkerTimer_ = 0.0f; // gold center flash + HEADSHOT text
    float hurtTimer_ = 0.0f;          // directional damage indicator life
    float hurtAngle_ = 0.0f;          // screen angle to the last damage source

    // Floating score popups near the crosshair ("+100"), rise and fade.
    struct ScorePopup {
        std::string text;
        glm::vec3 color{1.0f};
        float life = 0.0f;
        float total = 1.1f;
        float driftX = 0.0f;
    };
    std::vector<ScorePopup> scorePopups_;

    // Glock ammo + recoil. Ammo is client-side (a magazine + a reserve, a
    // reload, and a dry-fire); recoil is a view-punch that recovers to zero.
    int magAmmo_ = 0;
    int reserveAmmo_ = 0;
    int magSize_ = 17;
    float reloadTimer_ = 0.0f;    // >0 = mid-reload (mag refills on completion)
    float recoilPitch_ = 0.0f;    // degrees of view-punch up, recovers to 0
    float recoilYaw_ = 0.0f;      // small horizontal kick, recovers to 0
    float reloadDuration_ = 1.08f;
    float adsBlend_ = 0.0f;       // RMB aim-down-sights ease, MiniCS Glock only

    // Feel-polish state: look sway, landing dip, first-spawn control hint.
    float swayX_ = 0.0f;            // smoothed horizontal look velocity
    float swayY_ = 0.0f;
    float landDipTimer_ = 0.0f;     // viewmodel dip after landing a fall
    float landDipAmount_ = 0.0f;
    bool wasGrounded_ = true;
    double controlHintTimer_ = 0.0; // "WASD MOVE / E INTERACT / ..." on spawn
    unsigned fxRng_ = 0xBEEF5EEDu;  // tiny rng for sound pitch variation
    unsigned carriedEntityId_ = 0;  // world id of the carried prop; 0 = none
    std::string hudToast_;          // transient gameplay message
    double hudToastTimer_ = 0.0;
    World::EntityHit aimInteract_;  // pickup/carryable under the crosshair, in range
    World::EntityHit aimInfo_;      // damageable entity under the crosshair
    int spawnCategory_ = 0;         // int(ContentCategory) of the open tab

    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    float eyeHeight_ = 1.65f;
    double tickDt_ = 1.0 / 128.0;
    double accumulator_ = 0.0;
    double jumpBufferTimer_ = -1.0;
    double fpsSmoothed_ = 0.0;
    bool walkHeldHud_ = false;
    std::string rendererName_ = "unknown";

    // Active game mode (a mode is "a game inside the game": ruleset + map +
    // loadout). Set by startGameMode; currentModeName_ shows on the HUD/pause,
    // sandboxMode_ gates the build/spawn tools onto the Sandbox path only.
    std::string currentModeName_;
    ModeStatus activeModeStatus_ = ModeStatus::Playable;
    bool sandboxMode_ = false;
    std::string lastModeId_; // Recent Games quick-launch (last mode played)
    int skinColorIndex_ = 0; // Skins screen placeholder selection

    // UI state.
    bool inGame_ = false; // a world session is active behind the UI
    UiScreen ui_ = UiScreen::MainMenu;
    UiScreen settingsReturn_ = UiScreen::MainMenu;
    std::string serverAddress_ = "127.0.0.1:27015";
    std::string menuStatus_; // e.g. the networking-not-implemented notice
    double menuTime_ = 0.0;  // drives the menu backdrop orbit + caret blink
    bool quitRequested_ = false;
    int hoveredButton_ = -1; // index into buildMenuLayout() order
};
