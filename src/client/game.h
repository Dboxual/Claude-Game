#pragma once

#include "client/platform/filesystem.h"
#include "client/platform/input.h"
#include "client/renderer/render_types.h"
#include "client/settings.h"
#include "shared/config.h"
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
//   MainMenu -> Singleplayer -> (start test world) -> gameplay (ui None)
//            -> Multiplayer  -> (networking placeholder)
//            -> Settings     -> back to wherever it was opened from
//   gameplay -> ESC -> PauseMain -> Resume | Settings | Quit to Menu
class Game {
public:
    bool init(IFileSystem& fs);
    void update(const InputState& in, double frameDt, int viewportW, int viewportH);
    RenderFrame buildRenderFrame(int viewportW, int viewportH) const;

    // For the composition root: UI state drives mouse capture and text
    // input, quit ends the loop, dirty settings get re-applied.
    bool uiActive() const { return ui_ != UiScreen::None; }
    bool wantsTextInput() const { return ui_ == UiScreen::Multiplayer; }
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
    enum class UiScreen { None, MainMenu, Singleplayer, Multiplayer, Settings, PauseMain };

    struct UiButton {
        enum class Id {
            // main menu
            OpenSingleplayer, OpenMultiplayer, OpenSettings, QuitApp,
            // singleplayer screen
            StartTestWorld, CreateWorldSoon, BackToMain,
            // multiplayer screen
            Connect, QuickLocalhost, ServerBrowserSoon,
            // pause menu
            Resume, QuitToMenu,
            // settings screen
            Back, ResetDefaults,
            SensDown, SensUp, FovDown, FovUp,
            MasterDown, MasterUp, MusicDown, MusicUp, SfxDown, SfxUp,
            ToggleFullscreen, ToggleVsync, ToggleHud,
        };
        Id id;
        float x, y, w, h;
        std::string label;   // drawn centered inside the rect
        bool enabled = true; // disabled = greyed out, no hover, no click
    };

    void loadMovementConfig();
    bool loadSettings(); // returns whether a settings file existed
    void saveSettings();
    void startTestWorld();
    std::vector<UiButton> buildMenuLayout(int viewportW, int viewportH) const;
    void activateButton(UiButton::Id id);
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
    glm::vec3 prevPos_{0.0f}; // position before the latest tick, for interpolation

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
