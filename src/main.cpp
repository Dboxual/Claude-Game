// TacMove - Milestone 1: tactical FPS movement prototype.
// SDL3 windowing/input, OpenGL 3.3 core rendering, GLM math.
//
// Architecture: the simulation runs on a fixed tick (default 128 Hz) with
// render-side position interpolation; mouse look is applied per frame for
// minimal input latency.

#include "config.h"
#include "debug_text.h"
#include "gl_loader.h"
#include "player.h"
#include "renderer.h"
#include "world.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

int main(int argc, char** argv) {
    long long maxFrames = -1; // --frames N: exit after N frames (smoke testing)
    bool vsync = true;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            maxFrames = std::atoll(argv[++i]);
        } else if (std::strcmp(argv[i], "--novsync") == 0) {
            vsync = false;
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // macOS core profile
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* window = SDL_CreateWindow("TacMove - Movement Prototype (Milestone 1)",
                                          1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        SDL_Log("SDL_GL_CreateContext failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_SetSwapInterval(vsync ? 1 : 0);

    if (!loadGLFunctions()) {
        SDL_Log("Failed to load OpenGL 3.3 entry points");
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_Log("GL renderer: %s", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    SDL_Log("GL version:  %s", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    Renderer renderer;
    DebugText text;
    if (!renderer.init() || !text.init()) {
        SDL_Log("Renderer init failed");
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    ConfigFile cfgFile;
    MovementConfig cfg;
    std::string cfgPath = findConfigPath(SDL_GetBasePath());
    if (!cfgPath.empty() && cfgFile.loadFromFile(cfgPath)) {
        cfg.applyFrom(cfgFile);
        SDL_Log("Loaded movement config: %s", cfgPath.c_str());
    } else {
        SDL_Log("config/movement.cfg not found - using built-in defaults");
    }

    World world;
    world.buildTestMap();

    Player player;
    player.respawn(world.spawnPoint);

    SDL_SetWindowRelativeMouseMode(window, true);

    float yaw = 0.0f;
    float pitch = 0.0f;
    double tickDt = 1.0 / std::max(30.0f, cfg.tickRate);
    double accumulator = 0.0;
    double jumpBufferTimer = -1.0;
    double fpsSmoothed = 0.0;
    bool showHud = true;
    bool walkHeld = false;
    long long frameCount = 0;

    glm::vec3 prevPos = player.pos;
    float eyeHeight = cfg.standHeight - cfg.eyeOffset;

    const Uint64 perfFreq = SDL_GetPerformanceFrequency();
    Uint64 lastCounter = SDL_GetPerformanceCounter();

    bool running = true;
    while (running) {
        // ---- events ----
        float mouseDx = 0.0f;
        float mouseDy = 0.0f;
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (SDL_GetWindowRelativeMouseMode(window)) {
                    mouseDx += e.motion.xrel;
                    mouseDy += e.motion.yrel;
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                SDL_SetWindowRelativeMouseMode(window, true);
                break;
            case SDL_EVENT_KEY_DOWN:
                if (e.key.repeat) break;
                if (e.key.key == SDLK_ESCAPE) {
                    bool captured = SDL_GetWindowRelativeMouseMode(window);
                    SDL_SetWindowRelativeMouseMode(window, !captured);
                } else if (e.key.key == SDLK_F5) {
                    if (cfgFile.reload()) {
                        cfg.applyFrom(cfgFile);
                        SDL_Log("Movement config reloaded");
                    }
                } else if (e.key.key == SDLK_F1) {
                    showHud = !showHud;
                } else if (e.key.scancode == SDL_SCANCODE_SPACE) {
                    jumpBufferTimer = cfg.jumpBufferMs / 1000.0;
                }
                break;
            default:
                break;
            }
        }

        // ---- frame timing ----
        Uint64 now = SDL_GetPerformanceCounter();
        double frameDt = double(now - lastCounter) / double(perfFreq);
        lastCounter = now;
        frameDt = std::min(frameDt, 0.25);
        if (frameDt > 0.0) {
            double fps = 1.0 / frameDt;
            fpsSmoothed = fpsSmoothed <= 0.0 ? fps : fpsSmoothed * 0.95 + fps * 0.05;
        }

        // ---- mouse look (per frame) ----
        yaw += glm::radians(mouseDx * 0.022f * cfg.mouseSensitivity);
        pitch -= glm::radians(mouseDy * 0.022f * cfg.mouseSensitivity);
        pitch = glm::clamp(pitch, glm::radians(-89.0f), glm::radians(89.0f));

        // ---- input snapshot ----
        const bool* keys = SDL_GetKeyboardState(nullptr);
        PlayerInput in;
        in.moveForward = (keys[SDL_SCANCODE_W] ? 1.0f : 0.0f) - (keys[SDL_SCANCODE_S] ? 1.0f : 0.0f);
        in.moveRight = (keys[SDL_SCANCODE_D] ? 1.0f : 0.0f) - (keys[SDL_SCANCODE_A] ? 1.0f : 0.0f);
        in.crouch = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_C];
        in.walk = keys[SDL_SCANCODE_LSHIFT];
        in.yaw = yaw;
        walkHeld = in.walk;

        // ---- fixed-tick simulation ----
        accumulator += frameDt;
        int tickSafety = 0;
        while (accumulator >= tickDt && tickSafety++ < 10) {
            in.jump = jumpBufferTimer > 0.0 || (cfg.autoBhop != 0.0f && keys[SDL_SCANCODE_SPACE]);
            prevPos = player.pos;
            player.tick(in, world, cfg, float(tickDt));
            if (player.justJumped) jumpBufferTimer = -1.0;
            jumpBufferTimer -= tickDt;
            accumulator -= tickDt;
        }
        if (tickSafety >= 10) accumulator = 0.0; // spiral-of-death guard

        // ---- camera ----
        float alpha = float(accumulator / tickDt);
        glm::vec3 renderPos = glm::mix(prevPos, player.pos, alpha);
        float targetEye = player.height(cfg) - cfg.eyeOffset;
        eyeHeight += (targetEye - eyeHeight) * std::min(1.0f, cfg.crouchTransitionSpeed * float(frameDt));

        glm::vec3 eye = renderPos + glm::vec3(0.0f, eyeHeight, 0.0f);
        glm::vec3 lookDir(std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch));
        glm::mat4 view = glm::lookAt(eye, eye + lookDir, glm::vec3(0.0f, 1.0f, 0.0f));

        int pixelW = 0, pixelH = 0;
        SDL_GetWindowSizeInPixels(window, &pixelW, &pixelH);
        float aspect = pixelH > 0 ? float(pixelW) / float(pixelH) : 1.0f;
        glm::mat4 proj = glm::perspective(glm::radians(cfg.fovDegrees), aspect, 0.05f, 300.0f);

        // ---- render ----
        renderer.beginFrame(pixelW, pixelH, proj, view);
        renderer.drawWorld(world);

        text.begin();
        if (showHud) {
            const glm::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
            const glm::vec4 dim(0.8f, 0.85f, 0.9f, 1.0f);
            const float s = 2.0f;
            const float lineH = 20.0f;
            float ty = 12.0f;
            char buf[256];

            std::snprintf(buf, sizeof(buf), "SPEED %6.2f M/S", player.horizontalSpeed());
            text.add(12, ty, s, white, buf);
            ty += lineH;
            std::snprintf(buf, sizeof(buf), "VEL X %+6.2f  Y %+6.2f  Z %+6.2f",
                          player.vel.x, player.vel.y, player.vel.z);
            text.add(12, ty, s, dim, buf);
            ty += lineH;
            std::snprintf(buf, sizeof(buf), "POS X %+7.2f  Y %+7.2f  Z %+7.2f",
                          player.pos.x, player.pos.y, player.pos.z);
            text.add(12, ty, s, dim, buf);
            ty += lineH;

            const char* mode;
            bool moving = player.horizontalSpeed() > 0.1f;
            if (!player.grounded) mode = player.crouched ? "AIR CROUCH" : "AIR";
            else if (player.crouched) mode = "CROUCH";
            else if (walkHeld && moving) mode = "WALK";
            else mode = "RUN";
            std::snprintf(buf, sizeof(buf), "GROUNDED %s   MODE %s",
                          player.grounded ? "YES" : "NO", mode);
            text.add(12, ty, s, white, buf);
            ty += lineH;
            std::snprintf(buf, sizeof(buf), "FPS %5.0f   TICK %d HZ", fpsSmoothed, int(cfg.tickRate));
            text.add(12, ty, s, dim, buf);
            ty += lineH;
            text.add(12, ty, s, glm::vec4(0.7f, 0.75f, 0.8f, 1.0f),
                     "F5 RELOAD CFG  F1 HUD  ESC MOUSE  SHIFT WALK  CTRL CROUCH");

            // Big speedometer under the crosshair; green when above run speed
            // (i.e. you gained speed through air-strafing).
            float hSpeed = player.horizontalSpeed();
            glm::vec4 speedColor = hSpeed > cfg.runSpeed + 0.05f
                                       ? glm::vec4(0.45f, 1.0f, 0.5f, 1.0f)
                                       : white;
            std::snprintf(buf, sizeof(buf), "%.1f", hSpeed);
            text.addCentered(pixelW * 0.5f, pixelH * 0.60f, 3.0f, speedColor, buf);
        }
        // Crosshair.
        text.addCentered(pixelW * 0.5f, pixelH * 0.5f - 7.0f, 2.0f,
                         glm::vec4(1.0f, 1.0f, 1.0f, 0.9f), "+");
        text.flush(pixelW, pixelH);

        SDL_GL_SwapWindow(window);

        ++frameCount;
        if (maxFrames >= 0 && frameCount >= maxFrames) running = false;
    }

    text.shutdown();
    renderer.shutdown();
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
