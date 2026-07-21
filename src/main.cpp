// ZION entry point. Flags:
//   --frames N         run N frames then exit (smoke test)
//   --screenshot PATH  save a screenshot near the end of a smoke run
//   --record-dir PATH  sample rendered frames for a test recording
//   --width W --height H --fullscreen
//   --autoplay         scripted movement for soak tests
//   --tour             normal-input action tour for recorded validation
//   --dev              enable visible creative testing controls
//   --time H           with --dev, start the visual clock at hour H
//   --data-dir PATH    isolate settings/saves/logs for automated tests
#include "core/app.h"
#include "core/log.h"
#include "core/paths.h"
#include "game/game.h"
#include "raylib.h"
#include "settings/settings.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

int main(int argc, char** argv) {
    AppConfig cfg;
    GameLaunchOptions launch;
    std::string dataDir;
    std::string recordDir;
    int cliWidth = -1, cliHeight = -1;
    bool cliFullscreen = false;

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        auto next = [&]() { return (i + 1 < argc) ? argv[++i] : ""; };
        if      (a == "--frames")     launch.smokeFrames = atoi(next());
        else if (a == "--screenshot") launch.screenshotPath = next();
        else if (a == "--record-dir") recordDir = next();
        else if (a == "--width")      cliWidth = atoi(next());
        else if (a == "--height")     cliHeight = atoi(next());
        else if (a == "--fullscreen") cliFullscreen = true;
        else if (a == "--autoplay")   launch.autoplay = true;
        else if (a == "--tour") {
            launch.inputTour = true;
            launch.thirdPerson = true;
        }
        else if (a == "--tp")         launch.thirdPerson = true;
        else if (a == "--dev")        launch.devMode = true;
        else if (a == "--time")       launch.devStartHour = (float)atof(next());
        else if (a == "--data-dir")   dataDir = next();
        else if (a == "--zone")       launch.startZone = atoi(next());
    }

    if (!dataDir.empty()) SetUserDataOverride(dataDir);
    if (!recordDir.empty()) std::filesystem::create_directories(recordDir);

    GraphicsSettings startupGraphics;
    if (LoadStartupGraphics(startupGraphics)) {
        cfg.width = startupGraphics.width;
        cfg.height = startupGraphics.height;
        cfg.fullscreen = startupGraphics.fullscreen;
    }
    if (cliWidth > 0) cfg.width = cliWidth;
    if (cliHeight > 0) cfg.height = cliHeight;
    if (cliFullscreen) cfg.fullscreen = true;

    LogOpenFile(LogFilePath());
    LOG_INFO("ZION starting (data dir: %s)", UserDataDir().c_str());

    App app;
    app.Init(cfg);

    static Game game;   // large aggregate once systems land; keep off the stack
    game.Init(app, launch);

    int frame = 0;
    float elapsed = 0.0f;
    float recordAccumulator = 0.0f;
    int recordedFrames = 0;
    while (!app.ShouldClose() && !game.WantsQuit()) {
        float dt = app.BeginFrame();
        elapsed += dt;

        game.Frame(dt);
        frame++;

        // Deterministic, permission-free test recording. Sampling at 12 fps
        // keeps capture overhead and artifact size reasonable while preserving
        // enough motion to review facing, camera turns, and locomotion jitter.
        if (!recordDir.empty()) {
            recordAccumulator += dt;
            constexpr float RECORD_INTERVAL = 1.0f / 12.0f;
            if (recordAccumulator >= RECORD_INTERVAL) {
                recordAccumulator = fmodf(recordAccumulator, RECORD_INTERVAL);
                Image shot = LoadImageFromScreen();
                int targetW = GetScreenWidth() / 2;
                int targetH = GetScreenHeight() / 2;
                ImageResize(&shot, targetW, targetH);
                char name[32];
                snprintf(name, sizeof(name), "frame_%04d.png", recordedFrames++);
                std::filesystem::path path = std::filesystem::path(recordDir) / name;
                ExportImage(shot, path.string().c_str());
                UnloadImage(shot);
            }
        }

        // Capture from the back buffer, before EndFrame's swap invalidates it.
        // (Not TakeScreenshot: it prepends the working dir, breaking absolute paths.)
        if (launch.smokeFrames > 0 && !launch.screenshotPath.empty() &&
            frame == launch.smokeFrames - 2) {
            Image shot = LoadImageFromScreen();
            ExportImage(shot, launch.screenshotPath.c_str());
            UnloadImage(shot);
        }

        app.EndFrame();
        if (launch.smokeFrames > 0 && frame >= launch.smokeFrames) break;
    }

    if (launch.smokeFrames > 0)
        printf("[smoke] ok - %d frames in %.2fs (~%d fps avg)\n",
               frame, elapsed, (int)(frame / (elapsed > 0.0f ? elapsed : 1.0f)));
    if (recordedFrames > 0)
        printf("[record] %d frames -> %s\n", recordedFrames, recordDir.c_str());

    game.Shutdown();
    app.Shutdown();
    LogClose();
    return 0;
}
