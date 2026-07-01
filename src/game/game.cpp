#include "game/game.h"

#include "engine/log.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>

bool Game::init(IFileSystem& fs) {
    fs_ = &fs;

    const std::string relative = "config/movement.cfg";
    if (fs.exists(relative)) cfgPath_ = relative;
    else if (fs.exists(fs.basePath() + relative)) cfgPath_ = fs.basePath() + relative;

    loadConfig();
    tickDt_ = 1.0 / std::max(30.0f, cfg_.tickRate); // startup only; F5 keeps the tick

    world_.buildTestMap();
    player_.respawn(world_.spawnPoint);
    prevPos_ = player_.pos;
    eyeHeight_ = cfg_.standHeight - cfg_.eyeOffset;
    return true;
}

void Game::loadConfig() {
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

void Game::update(const InputState& in, double frameDt) {
    if (in.reloadConfigPressed) loadConfig();
    if (in.toggleHudPressed) showHud_ = !showHud_;
    if (in.jumpPressed) jumpBufferTimer_ = cfg_.jumpBufferMs / 1000.0;
    walkHeldHud_ = in.walkHeld;

    if (frameDt > 0.0) {
        double fps = 1.0 / frameDt;
        fpsSmoothed_ = fpsSmoothed_ <= 0.0 ? fps : fpsSmoothed_ * 0.95 + fps * 0.05;
    }

    // Mouse look is per-frame for minimal latency; movement reads yaw at tick
    // time. 0.022 degrees per count matches the tactical-FPS convention.
    yaw_ += glm::radians(in.lookDx * 0.022f * cfg_.mouseSensitivity);
    pitch_ -= glm::radians(in.lookDy * 0.022f * cfg_.mouseSensitivity);
    pitch_ = glm::clamp(pitch_, glm::radians(-89.0f), glm::radians(89.0f));

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

RenderFrame Game::buildRenderFrame(int viewportW, int viewportH) const {
    RenderFrame frame;
    frame.viewportW = viewportW;
    frame.viewportH = viewportH;

    // Camera: interpolate between the last two ticks for smooth motion.
    float alpha = float(std::clamp(accumulator_ / tickDt_, 0.0, 1.0));
    glm::vec3 renderPos = glm::mix(prevPos_, player_.pos, alpha);
    glm::vec3 eye = renderPos + glm::vec3(0.0f, eyeHeight_, 0.0f);
    glm::vec3 lookDir(std::sin(yaw_) * std::cos(pitch_), std::sin(pitch_),
                      -std::cos(yaw_) * std::cos(pitch_));
    frame.view = glm::lookAt(eye, eye + lookDir, glm::vec3(0.0f, 1.0f, 0.0f));
    float aspect = viewportH > 0 ? float(viewportW) / float(viewportH) : 1.0f;
    frame.proj = glm::perspective(glm::radians(cfg_.fovDegrees), aspect, 0.05f, 300.0f);

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

    // Debug HUD.
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

    if (showHud_) {
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
                "F5 RELOAD CFG  F1 HUD  ESC MOUSE  SHIFT WALK  CTRL CROUCH");

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

    return frame;
}
