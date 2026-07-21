// Game owns every system and the top-level state machine. App stays
// ignorant of gameplay; systems stay ignorant of each other (game/ is the
// only glue layer). World content lives inside ZoneManager's current
// ZoneInstance. See ARCHITECTURE.md and CODEX_HANDOFF.md.
#pragma once
#include <string>
#include "audio/audio.h"
#include "core/clock.h"
#include "fx/particles.h"
#include "game/menus.h"
#include "input/input.h"
#include "player/camera_rig.h"
#include "player/controller.h"
#include "render/postfx.h"
#include "render/renderer.h"
#include "save/save.h"
#include "settings/settings.h"
#include "ui/hud.h"
#include "ui/ui.h"
#include "zone/zone_manager.h"

struct GameLaunchOptions {
    int smokeFrames = 0;         // >0: run N frames then request quit
    std::string screenshotPath;  // captured near the end of a smoke run
    bool autoplay = false;       // scripted movement for soak tests
    bool thirdPerson = false;    // start in third person (smoke validation)
    int startZone = -1;          // --zone N: skip title into this zone
};

enum class GameState { Title, Playing, Paused, SettingsMenu };

class App;

class Game {
public:
    void Init(App& app, const GameLaunchOptions& opts);
    void Frame(float dt);
    void Shutdown();
    bool WantsQuit() const { return wantQuit; }

private:
    void StartPlaying(bool fromSave);
    void SaveNow();
    void SetMouseCapture(bool on);
    void ApplySettingsDiffs();
    float EffectiveViewDistance() const;

    void UpdateTitle(float dt);
    void UpdatePlaying(float dt);

    void DrawFrame(float dt);
    void DrawUiLayer(float dt);
    void DrawPlayerBody(Vector3 feet);
    void DrawDebugOverlay();

    App* app = nullptr;
    GameLaunchOptions opts;
    GameState state = GameState::Title;
    GameState settingsReturn = GameState::Title;

    InputSystem input;
    Settings settings, prevSettings;
    AudioSystem audio;
    Renderer renderer;
    PostFx postfx;
    ZoneManager zones;
    ParticleSystem particles;
    PlayerController player;
    CameraRig camRig;
    Ui ui;
    Hud hud;
    MenuState menuState;
    FixedClock simClock;
    Camera3D camera = {};

    SaveData session;
    int currentTarget = -1;
    float time = 0.0f;
    float smoothDt = 1.0f / 60.0f;
    int lastSimSteps = 0;
    bool pendingJump = false;
    bool showDebug = false;
    bool mouseCaptured = false;
    bool wantQuit = false;
    float autoplayJumpTimer = 0.0f;
    float gateCooldown = 0.0f;   // autoplay: don't bounce straight back
    int wispChain = 0;
    float wispChainTimer = 0.0f;
    float titleOrbit = 0.0f;
};
