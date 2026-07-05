#pragma once

#include "client/platform/filesystem.h"
#include "client/platform/input.h"
#include "client/renderer/render_types.h"
#include "client/settings.h"
#include "shared/config.h"
#include "shared/content.h"
#include "shared/inventory.h"
#include "shared/player.h"
#include "shared/world.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

// The whole client behind the platform seams: main menu shell, pause menu,
// settings, and the fixed-tick movement sim. Consumes InputState, emits a
// RenderFrame. Contains no windowing, graphics-API, or OS calls - that is
// what keeps the client portable and the dedicated server free of UI code.
//
// UI flow:
//   MainMenu -> Singleplayer -> test world | Create World | Load World
//            -> Multiplayer  -> (networking placeholder)
//            -> Settings     -> back to wherever it was opened from
//   gameplay -> ESC -> PauseMain -> Resume | Settings | Quit to Menu
//   gameplay -> Q   -> SpawnMenu (dev/admin: spawn pickups/bots/props)
class Game {
public:
    bool init(IFileSystem& fs);
    void update(const InputState& in, double frameDt, int viewportW, int viewportH);
    RenderFrame buildRenderFrame(int viewportW, int viewportH) const;

    // For the composition root: UI state drives mouse capture and text
    // input, quit ends the loop, dirty settings get re-applied.
    bool uiActive() const { return ui_ != UiScreen::None; }
    bool wantsTextInput() const {
        return ui_ == UiScreen::Multiplayer || ui_ == UiScreen::CreateWorld;
    }
    bool quitRequested() const { return quitRequested_; }
    const GameSettings& settings() const { return settings_; }
    bool settingsDirty() const { return settingsDirty_; }
    void clearSettingsDirty() { settingsDirty_ = false; }
    void overrideVsync(bool enabled) { settings_.vsync = enabled; settingsDirty_ = true; }

    // Dev flags: skip the main menu (--world) or start paused in-world (--paused).
    void startInWorld() { startTestWorld(); }
    void startInWorldPaused() { startTestWorld(); ui_ = UiScreen::PauseMain; }

    // Purely informational (shown on the debug HUD).
    void setRendererName(std::string name) { rendererName_ = std::move(name); }

private:
    enum class UiScreen {
        None, MainMenu, Singleplayer, Multiplayer, Settings, PauseMain,
        CreateWorld, LoadWorld, SpawnMenu,
    };

    struct UiButton {
        enum class Id {
            // main menu
            OpenSingleplayer, OpenMultiplayer, OpenSettings, QuitApp,
            // singleplayer screen
            StartTestWorld, OpenCreateWorld, OpenLoadWorld, BackToMain,
            // create/load world screens
            ConfirmCreateWorld, LoadWorldEntry, BackToSingleplayer,
            // multiplayer screen
            Connect, QuickLocalhost, ServerBrowserSoon,
            // pause menu
            Resume, QuitToMenu,
            // dev spawn menu
            SelectSpawnCategory, SpawnEntity, CloseSpawnMenu,
            // settings screen
            Back, ResetDefaults,
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
    void tryAttack();
    void showToast(std::string text);

    std::vector<UiButton> buildMenuLayout(int viewportW, int viewportH) const;
    void activateButton(UiButton::Id id, int payload = -1);
    void appendMenuDraws(RenderFrame& frame) const;
    void appendHudDraws(RenderFrame& frame) const;

    IFileSystem* fs_ = nullptr;
    std::string cfgPath_;
    std::string settingsPath_;
    ConfigFile cfgFile_;
    MovementConfig cfg_;
    GameSettings settings_;
    bool settingsDirty_ = false;

    World world_;
    Player player_;
    ContentRegistry content_;
    glm::vec3 prevPos_{0.0f}; // position before the latest tick, for interpolation

    // Sandbox world session. currentWorldFile_ empty = test arena (not saved).
    std::string worldsRoot_; // "worlds/" resolved next to the config dir
    std::string currentWorldFile_;
    std::string currentWorldName_;
    std::vector<WorldListEntry> worldList_; // refreshed on entering LoadWorld
    std::string worldNameInput_;            // CreateWorld text field

    // Combat / interaction state (reset by beginSession).
    Inventory inventory_;
    float attackCooldown_ = 0.0f;   // seconds until the equipped weapon can fire
    float swingTimer_ = 0.0f;       // placeholder melee-swing feedback
    float muzzleFlashTimer_ = 0.0f; // placeholder pistol-fire feedback
    float hitMarkerTimer_ = 0.0f;   // crosshair hit confirmation
    std::string hudToast_;          // transient gameplay message
    double hudToastTimer_ = 0.0;
    World::EntityHit aimPickup_;    // pickup under the crosshair, in range
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
