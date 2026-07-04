#include "client/game.h"

#include "shared/log.h"
#include "shared/protocol.h"

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

bool contains(float px, float py, float x, float y, float w, float h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}

} // namespace

bool Game::init(IFileSystem& fs) {
    fs_ = &fs;

    const std::string relative = "config/movement.cfg";
    if (fs.exists(relative)) cfgPath_ = relative;
    else if (fs.exists(fs.basePath() + relative)) cfgPath_ = fs.basePath() + relative;

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

void Game::startTestWorld() {
    player_.respawn(world_.spawnPoint);
    prevPos_ = player_.pos;
    yaw_ = 0.0f;
    pitch_ = 0.0f;
    eyeHeight_ = cfg_.standHeight - cfg_.eyeOffset;
    accumulator_ = 0.0;
    jumpBufferTimer_ = -1.0;
    inGame_ = true;
    ui_ = UiScreen::None;
    menuStatus_.clear();
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
        case UiScreen::MainMenu:
            break;
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
            activateButton(buttons[size_t(hoveredButton_)].id);
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
        if (player_.justJumped) jumpBufferTimer_ = -1.0;
        jumpBufferTimer_ -= tickDt_;
        accumulator_ -= tickDt_;
    }
    if (tickSafety >= 10) accumulator_ = 0.0; // spiral-of-death guard

    float targetEye = player_.height(cfg_) - cfg_.eyeOffset;
    eyeHeight_ += (targetEye - eyeHeight_) * std::min(1.0f, cfg_.crouchTransitionSpeed * float(frameDt));
}

void Game::activateButton(UiButton::Id id) {
    using Id = UiButton::Id;
    bool settingsChanged = false;
    switch (id) {
    // Navigation.
    case Id::OpenSingleplayer: ui_ = UiScreen::Singleplayer; menuStatus_.clear(); break;
    case Id::OpenMultiplayer: ui_ = UiScreen::Multiplayer; menuStatus_.clear(); break;
    case Id::OpenSettings: settingsReturn_ = ui_; ui_ = UiScreen::Settings; break;
    case Id::BackToMain: ui_ = UiScreen::MainMenu; menuStatus_.clear(); break;
    case Id::Back: saveSettings(); ui_ = settingsReturn_; break;
    case Id::QuitApp: quitRequested_ = true; break;

    // Singleplayer.
    case Id::StartTestWorld: startTestWorld(); break;
    case Id::CreateWorldSoon: break; // disabled

    // Multiplayer (networking is a later milestone; protocol.h reserves v1).
    case Id::Connect:
    case Id::QuickLocalhost:
        if (id == Id::QuickLocalhost) serverAddress_ = "127.0.0.1:27015";
        menuStatus_ = "CONNECTION SYSTEM NOT IMPLEMENTED YET - NETWORKING IS A LATER MILESTONE";
        break;
    case Id::ServerBrowserSoon: break; // disabled

    // Pause menu.
    case Id::Resume: ui_ = UiScreen::None; break;
    case Id::QuitToMenu: inGame_ = false; ui_ = UiScreen::MainMenu; menuStatus_.clear(); break;

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
            {Id::CreateWorldSoon, 0, 0, 0, 0, "CREATE WORLD (COMING SOON)", false},
            {Id::BackToMain, 0, 0, 0, 0, "BACK", true},
        });
        break;

    case UiScreen::Multiplayer:
        column(viewportH * 0.40f, {
            {Id::Connect, 0, 0, 0, 0, "CONNECT", true},
            {Id::QuickLocalhost, 0, 0, 0, 0, "LOCALHOST 127.0.0.1", true},
            {Id::ServerBrowserSoon, 0, 0, 0, 0, "SERVER BROWSER (COMING SOON)", false},
            {Id::BackToMain, 0, 0, 0, 0, "BACK", true},
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
    frame.boxes.reserve(world_.boxes().size());
    for (const WorldBox& wb : world_.boxes()) {
        BoxDraw draw;
        draw.center = (wb.box.min + wb.box.max) * 0.5f;
        draw.size = wb.box.max - wb.box.min;
        draw.color = wb.color;
        draw.checkerTop = wb.checkerTop;
        frame.boxes.push_back(draw);
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
        addLine(glm::vec4(0.7f, 0.75f, 0.8f, 1.0f),
                "ESC PAUSE  F5 RELOAD CFG  F1 HUD  SHIFT WALK  CTRL CROUCH");

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
                     "MOVEMENT PROTOTYPE - FOUNDATION BUILD");
        std::snprintf(buf, sizeof(buf), "PROTOCOL V%d - NETWORKING NOT IMPLEMENTED YET",
                      net::kProtocolVersion);
        centeredText(cx, viewportH - 34.0f, 2.0f, dimText, buf);
        break;
    case UiScreen::Singleplayer:
        centeredText(cx, viewportH * 0.24f, 4.0f, white, "SINGLEPLAYER");
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

    // Multiplayer: server-address input box (always focused on this screen).
    if (ui_ == UiScreen::Multiplayer) {
        float boxX = cx - kAddressBoxW / 2;
        float boxY = viewportH * 0.26f;
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

        if (!menuStatus_.empty()) {
            centeredText(cx, viewportH * 0.40f + 4 * kBtnGap + 10.0f, 2.0f, notice, menuStatus_);
        }
    }

    // Buttons (same layout the hit-testing used this frame).
    std::vector<UiButton> buttons = buildMenuLayout(viewportW, viewportH);
    for (size_t i = 0; i < buttons.size(); ++i) {
        const UiButton& b = buttons[i];
        bool hovered = int(i) == hoveredButton_;
        glm::vec4 fill = !b.enabled ? glm::vec4(0.13f, 0.15f, 0.19f, 0.85f)
                       : hovered    ? glm::vec4(0.36f, 0.46f, 0.60f, 0.95f)
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
