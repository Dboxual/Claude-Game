// ZION entry point. Flags:
//   --frames N         run N frames then exit (smoke test)
//   --screenshot PATH  save a screenshot near the end of a smoke run
//   --width W --height H --fullscreen
//   --autoplay         scripted movement for soak tests
#include "core/app.h"
#include "core/log.h"
#include "core/paths.h"
#include "game/game.h"
#include "raylib.h"
#include <cstdlib>
#include <string>

int main(int argc, char** argv) {
    AppConfig cfg;
    GameLaunchOptions launch;

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        auto next = [&]() { return (i + 1 < argc) ? argv[++i] : ""; };
        if      (a == "--frames")     launch.smokeFrames = atoi(next());
        else if (a == "--screenshot") launch.screenshotPath = next();
        else if (a == "--width")      cfg.width = atoi(next());
        else if (a == "--height")     cfg.height = atoi(next());
        else if (a == "--fullscreen") cfg.fullscreen = true;
        else if (a == "--autoplay")   launch.autoplay = true;
        else if (a == "--tp")         launch.thirdPerson = true;
    }

    LogOpenFile(LogFilePath());
    LOG_INFO("ZION starting (data dir: %s)", UserDataDir().c_str());

    App app;
    app.Init(cfg);

    static Game game;   // large aggregate once systems land; keep off the stack
    game.Init(app, launch);

    int frame = 0;
    float elapsed = 0.0f;
    while (!app.ShouldClose() && !game.WantsQuit()) {
        float dt = app.BeginFrame();
        elapsed += dt;

        game.Frame(dt);
        frame++;

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

    game.Shutdown();
    app.Shutdown();
    LogClose();
    return 0;
}
