// Game owns every system and the top-level state machine. App stays
// ignorant of gameplay; systems stay ignorant of each other (game/ is the
// only glue layer). See ARCHITECTURE.md.
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
#include "world/interact.h"
#include "world/terrain.h"
#include "world/world.h"

struct GameLaunchOptions {
    int smokeFrames = 0;         // >0: run N frames then request quit
    std::string screenshotPath;  // captured near the end of a smoke run
    bool autoplay = false;       // scripted movement for soak tests
    bool thirdPerson = false;    // start in third person (smoke validation)
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
    // flow
    void StartPlaying(bool fromSave);
    void SaveNow();
    void SetMouseCapture(bool on);
    void ApplySettingsDiffs();

    // per-state update
    void UpdateTitle(float dt);
    void UpdatePlaying(float dt);

    // draw
    void DrawFrame(float dt);
    void DrawUiLayer(float dt);
    void DrawPlayerBody(Vector3 feet);
    void DrawDebugOverlay();

    App* app = nullptr;
    GameLaunchOptions opts;
    GameState state = GameState::Title;
    GameState settingsReturn = GameState::Title;

    InputSystem input;
    Settings settings, prevSettings;    // prev = applied-state for diffing
    AudioSystem audio;
    Renderer renderer;
    PostFx postfx;
    Terrain terrain;
    World world;
    InteractionSystem interact;
    ParticleSystem particles;
    PlayerController player;
    CameraRig camRig;
    Ui ui;
    Hud hud;
    MenuState menuState;
    FixedClock simClock;
    Camera3D camera = {};

    SaveData session;                   // live progress (anima, blessings, playtime)
    int currentTarget = -1;             // interactable under the gaze, or -1
    float time = 0.0f;
    float smoothDt = 1.0f / 60.0f;
    int lastSimSteps = 0;
    bool pendingJump = false;
    bool showDebug = false;
    bool mouseCaptured = false;
    bool wantQuit = false;
    float autoplayJumpTimer = 0.0f;
    int wispChain = 0;                  // rising-pitch pickup combo
    float wispChainTimer = 0.0f;
    float titleOrbit = 0.0f;
};
