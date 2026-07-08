// TacMove - composition root.
//
// This is the only file where the layers meet: it picks a platform factory
// (SDL3 desktop), picks a renderer backend, and pumps the loop. Everything
// below it talks through interfaces (IWindow/IInput/IFileSystem/IAudio,
// IRenderer), so porting means providing new factories - not touching the
// game.

#include "shared/log.h"
#include "client/game.h"
#include "client/platform/platform.h"
#include "client/platform/sdl/sdl_platform.h"
#include "client/renderer/renderer.h"

#include <chrono>
#include <cstdlib>
#include <cstring>

namespace {

GraphicsApi parseRendererArg(const char* value) {
    if (std::strcmp(value, "vulkan") == 0) return GraphicsApi::Vulkan;
    if (std::strcmp(value, "metal") == 0) return GraphicsApi::Metal;
    if (std::strcmp(value, "gles") == 0) return GraphicsApi::MobileGLES;
    if (std::strcmp(value, "gl") != 0 && std::strcmp(value, "opengl") != 0) {
        eng::logError("Unknown renderer '%s' (expected gl, vulkan, metal, or gles); using OpenGL", value);
    }
    return GraphicsApi::OpenGL;
}

} // namespace

int main(int argc, char** argv) {
    long long maxFrames = -1;  // --frames N: exit after N frames (smoke testing)
    bool startInWorld = false; // --world: skip the main menu, straight into the test world
    bool startPaused = false;  // --paused: start in-world on the pause menu (dev/testing)
    const char* equipWeapon = nullptr; // --equip <id>: start in-world holding a weapon
    const char* startMode = nullptr;   // --mode <id>: boot straight into a game mode
    const char* menuScreen = nullptr;  // --menu <screen>: boot onto a menu (screenshots)
    const char* shotPath = nullptr;    // --screenshot <path>: BMP of the last frame
    long long fireAtFrame = -1;        // --fire-at N: inject one attack press (dev/testing)
    bool forceNoVsync = false;
    WindowDesc desc;
    desc.title = "TacMove - Cross-Platform Game Hub";

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            maxFrames = std::atoll(argv[++i]);
        } else if (std::strcmp(argv[i], "--novsync") == 0) {
            desc.vsync = false;
            forceNoVsync = true;
        } else if (std::strcmp(argv[i], "--renderer") == 0 && i + 1 < argc) {
            desc.api = parseRendererArg(argv[++i]);
        } else if (std::strcmp(argv[i], "--paused") == 0) {
            startPaused = true;
        } else if (std::strcmp(argv[i], "--world") == 0) {
            startInWorld = true;
        } else if (std::strcmp(argv[i], "--equip") == 0 && i + 1 < argc) {
            equipWeapon = argv[++i];
        } else if (std::strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            startMode = argv[++i];
        } else if (std::strcmp(argv[i], "--menu") == 0 && i + 1 < argc) {
            menuScreen = argv[++i];
        } else if (std::strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) {
            shotPath = argv[++i];
        } else if (std::strcmp(argv[i], "--fire-at") == 0 && i + 1 < argc) {
            fireAtFrame = std::atoll(argv[++i]);
        }
    }

    PlatformSystems platform = createSdlPlatform(desc);
    if (!platform.valid()) {
        eng::logError("Platform init failed");
        return 1;
    }
    platform.audio->init(); // failure is non-fatal: we just run silent

    std::unique_ptr<IRenderer> renderer = createRenderer(desc.api);
    if (!renderer || !renderer->init(*platform.window)) {
        eng::logError("Renderer backend '%s' failed to initialize", graphicsApiName(desc.api));
        destroySdlPlatform(platform);
        return 1;
    }
    eng::logInfo("Renderer backend: %s", renderer->name());

    Game game;
    if (!game.init(*platform.fileSystem, platform.audio.get())) {
        eng::logError("Game init failed");
        renderer->shutdown();
        destroySdlPlatform(platform);
        return 1;
    }
    game.setRendererName(renderer->name());
    if (forceNoVsync) game.overrideVsync(false); // CLI beats the saved setting
    if (startPaused) game.startInWorldPaused();
    else if (startMode) game.startInMode(startMode);
    else if (equipWeapon) game.startInWorldEquipped(equipWeapon);
    else if (startInWorld) game.startInWorld();
    else if (menuScreen) game.debugOpenScreen(menuScreen);

    // The game boots to the main menu, so the mouse starts free and text
    // input off; the loop below keeps both in sync with the UI state.
    bool mouseCaptured = false;
    bool textInputActive = false;

    using Clock = std::chrono::steady_clock;
    Clock::time_point last = Clock::now();
    long long frameCount = 0;
    bool running = true;

    while (running) {
        const InputState& in = platform.input->pump();
        if (in.quitRequested) break;

        Clock::time_point now = Clock::now();
        double frameDt = std::chrono::duration<double>(now - last).count();
        last = now;
        frameDt = std::min(frameDt, 0.25);
        // Framed runs (smoke tests / screenshots) step a fixed 60 Hz clock so a
        // given frame number always lands at the same sim time - otherwise the
        // offscreen window's wildly variable frame rate makes timed captures
        // (e.g. --fire-at past a countdown) non-reproducible.
        if (maxFrames >= 0) frameDt = 1.0 / 60.0;

        int w = 0, h = 0;
        platform.window->pixelSize(w, h);

        // Dev input injection: one synthetic attack press, so automated runs
        // can capture tracers / swing animation mid-flight.
        if (fireAtFrame >= 0 && frameCount == fireAtFrame) {
            InputState synthetic = in;
            synthetic.attackPressed = true;
            synthetic.attackHeld = true;
            game.update(synthetic, frameDt, w, h);
        } else {
            game.update(in, frameDt, w, h);
        }
        if (game.quitRequested()) break;

        // Mouse-capture policy: captured while playing, free in any menu.
        bool wantCapture = !game.uiActive();
        if (wantCapture != mouseCaptured) {
            platform.window->setRelativeMouseMode(wantCapture);
            mouseCaptured = wantCapture;
        }

        // Text input only while a screen with a text field is up.
        bool wantText = game.wantsTextInput();
        if (wantText != textInputActive) {
            platform.window->setTextInputActive(wantText);
            textInputActive = wantText;
        }

        // Settings the game cannot apply itself (platform-side) get applied
        // here whenever they change; the setters no-op on unchanged values.
        if (game.settingsDirty()) {
            const GameSettings& s = game.settings();
            platform.window->setFullscreen(s.fullscreen);
            platform.window->setVsync(s.vsync);
            platform.audio->setVolumes(s.masterVolume, s.musicVolume, s.sfxVolume);
            game.clearSettingsDirty();
        }

        // Dev screenshot: capture the last frame of a --frames run (or an
        // early frame of an open-ended one) for automated visual checks.
        if (shotPath && frameCount == (maxFrames > 0 ? maxFrames - 1 : 90)) {
            renderer->requestScreenshot(shotPath);
        }
        renderer->render(game.buildRenderFrame(w, h));

        ++frameCount;
        if (maxFrames >= 0 && frameCount >= maxFrames) running = false;
    }

    renderer->shutdown();
    destroySdlPlatform(platform);
    return 0;
}
