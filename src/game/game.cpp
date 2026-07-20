#include "game/game.h"
#include "core/app.h"
#include "core/log.h"
#include "core/mathx.h"

static constexpr unsigned int WORLD_SEED = 20260720u;   // Elder Vale
static constexpr int SAVE_SLOT = 1;

// ---- lifecycle ------------------------------------------------------------

void Game::Init(App& appRef, const GameLaunchOptions& launch) {
    app = &appRef;
    opts = launch;

    settings.Load(input);
    prevSettings = settings;
    app->SetFpsCap(settings.gfx.fpsCap);
    app->SetFullscreen(settings.gfx.fullscreen);

    renderer.Init();
    // The sky horizon, lighting fog, and clear color must agree so distant
    // terrain melts into the sky seamlessly.
    renderer.sky.horizonColor = renderer.lighting.fogColor;
    postfx.Init(settings.gfx.renderScale);

    audio.Init(settings.audio);
    particles.Init();
    particles.SetDensity(settings.gfx.particleDensity);
    ui.Init();

    double t0 = GetTime();
    terrain.Generate(WORLD_SEED);
    world.Generate(WORLD_SEED, terrain);
    for (const ShrineInfo& s : world.Shrines())
        interact.Add({ s.heart ? InteractType::HeartShrine : InteractType::Wayshrine,
                       s.crystalPos, s.heart ? 4.5f : 3.4f, s.lightIndex, 0.0f, 0.0f,
                       "Commune" });
    LOG_INFO("Elder Vale generated in %d ms", (int)((GetTime() - t0) * 1000));

    player.Spawn(world.SpawnPos());
    camRig.Init(world.SpawnYaw());
    titleOrbit = 0.0f;

    if (opts.autoplay) {
        StartPlaying(false);     // soak runs skip the title
    } else {
        state = GameState::Title;   // plain --frames runs capture the title UI
        SetMouseCapture(false);
    }
}

void Game::Shutdown() {
    if (state != GameState::Title) SaveNow();
    settings.Save(input);
    ui.Shutdown();
    particles.Shutdown();
    audio.Shutdown();
    postfx.Shutdown();
    terrain.Shutdown();
    renderer.Shutdown();
    LOG_INFO("Session end: %.1fs, anima %d, blessings %d",
             time, session.anima, session.blessings);
}

// ---- flow helpers ---------------------------------------------------------

void Game::StartPlaying(bool fromSave) {
    if (fromSave) {
        session = SaveSystem::Load(SAVE_SLOT);
        player.Spawn(session.playerPos);
        camRig.Init(session.yaw);
        camRig.pitch = session.pitch;
        camRig.SetMode(session.cameraMode == 1 ? CameraView::ThirdPerson
                                               : CameraView::FirstPerson);
    } else {
        session = SaveData{};
        session.valid = true;
        player.Spawn(world.SpawnPos());
        camRig.Init(world.SpawnYaw());
        if (opts.thirdPerson) camRig.SetMode(CameraView::ThirdPerson);
    }
    state = GameState::Playing;
    simClock = FixedClock{};
    SetMouseCapture(true);
    hud.StartFadeIn(1.1f);
}

void Game::SaveNow() {
    session.playerPos = player.pos;
    session.yaw = camRig.yaw;
    session.pitch = camRig.pitch;
    session.cameraMode = (camRig.Mode() == CameraView::ThirdPerson) ? 1 : 0;
    SaveSystem::Write(SAVE_SLOT, session);
}

void Game::SetMouseCapture(bool on) {
    if (on == mouseCaptured) return;
    mouseCaptured = on;
    if (opts.smokeFrames > 0) return;   // headless-ish runs leave the cursor alone
    if (on) DisableCursor(); else EnableCursor();
}

void Game::ApplySettingsDiffs() {
    GraphicsSettings& g = settings.gfx;
    GraphicsSettings& pg = prevSettings.gfx;
    if (g.fullscreen != pg.fullscreen) app->SetFullscreen(g.fullscreen);
    if (g.fpsCap != pg.fpsCap) app->SetFpsCap(g.fpsCap);
    if (g.particleDensity != pg.particleDensity) particles.SetDensity(g.particleDensity);
    // renderScale is handled by EnsureTargets during draw (also covers resizes).
    AudioSettings& a = settings.audio;
    AudioSettings& pa = prevSettings.audio;
    if (a.master != pa.master || a.sfx != pa.sfx ||
        a.music != pa.music || a.ambience != pa.ambience)
        audio.Apply(a);
    prevSettings = settings;
}

// ---- frame ----------------------------------------------------------------

void Game::Frame(float dt) {
    time += dt;
    smoothDt = ExpDecay(smoothDt, dt, 4.0f, dt);
    input.BeginFrame();
    audio.Update(dt);
    ui.Begin(dt);
    hud.Update(dt);

    switch (state) {
        case GameState::Title:   UpdateTitle(dt); break;
        case GameState::Playing: UpdatePlaying(dt); break;
        case GameState::Paused:
        case GameState::SettingsMenu:
            particles.Update(dt);           // world stays alive behind menus
            break;
    }

    ApplySettingsDiffs();
    DrawFrame(dt);

    // UI feedback sounds, queued by widgets this frame.
    for (; ui.hoverEvents > 0; ui.hoverEvents--) audio.Play(SoundId::UiHover, 0.5f);
    for (; ui.clickEvents > 0; ui.clickEvents--) audio.Play(SoundId::UiClick, 0.7f);
}

// ---- title ----------------------------------------------------------------

void Game::UpdateTitle(float dt) {
    // Slow cinematic orbit around the temple.
    titleOrbit += dt * 0.07f;
    float r = 40.0f;
    Vector3 pos = { sinf(titleOrbit) * r, terrain.plateauHeight + 12.0f +
                    sinf(titleOrbit * 0.7f) * 2.0f, cosf(titleOrbit) * r };
    camera.position = pos;
    camera.target = { 0, terrain.plateauHeight + 3.0f, 0 };
    camera.up = { 0, 1, 0 };
    camera.fovy = 55.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    particles.Update(dt);
    particles.AmbientMotes(camera.position, dt);
}

// ---- playing --------------------------------------------------------------

void Game::UpdatePlaying(float dt) {
    session.playtime += dt;

    if (input.Pressed(Action::Pause)) {
        state = GameState::Paused;
        SetMouseCapture(false);
        return;
    }
    if (input.Pressed(Action::ToggleCamera)) camRig.ToggleMode();
    if (input.Pressed(Action::DebugOverlay)) showDebug = !showDebug;

    if (mouseCaptured)
        camRig.AddLook(input.LookDelta(), settings.controls.mouseSensitivity,
                       settings.controls.invertY);

    PlayerIntents intents;
    intents.move = input.MoveAxis();
    intents.sprintDown = input.Down(Action::Sprint);
    if (input.Pressed(Action::Jump)) pendingJump = true;

    if (opts.autoplay) {
        // Scripted soak: walk forward, hop periodically, and steer toward the
        // nearest shrine so payoff systems (wisps, chimes, saves) get soaked.
        intents.move = { 0, 1 };
        intents.sprintDown = fmodf(time, 9.0f) > 6.0f;
        const Interactable* nearest = nullptr;
        float bestD2 = 1e9f;
        for (const Interactable& it : interact.Items()) {
            float dx = it.pos.x - player.pos.x, dz = it.pos.z - player.pos.z;
            float d2 = dx * dx + dz * dz;
            if (it.rearm <= 0.0f && d2 < bestD2) { bestD2 = d2; nearest = &it; }
        }
        if (nearest) {
            float toYaw = atan2f(nearest->pos.x - player.pos.x,
                                 -(nearest->pos.z - player.pos.z));
            camRig.yaw = ExpDecayAngle(camRig.yaw, toYaw, 2.5f, dt);
            camRig.pitch = ExpDecay(camRig.pitch, bestD2 < 30.0f ? 0.5f : 0.0f, 2.0f, dt);
        } else {
            camRig.yaw += dt * 0.22f;
        }
        autoplayJumpTimer += dt;
        if (autoplayJumpTimer > 4.0f) { pendingJump = true; autoplayJumpTimer = 0.0f; }
    }

    // Fixed-step simulation with per-tick feel events.
    int steps = simClock.AddFrame(dt);
    lastSimSteps = steps;
    for (int i = 0; i < steps; i++) {
        intents.jumpPressed = pendingJump;
        pendingJump = false;
        PlayerEvents ev = player.FixedUpdate(intents, camRig.yaw, world);
        Vector3 feet = player.pos;
        if (ev.jumped) audio.Play(SoundId::Jump, 0.55f);
        if (ev.landed) {
            camRig.OnLand(ev.landSpeed);
            audio.Play(SoundId::Land, Clamp(ev.landSpeed * 0.1f, 0.3f, 1.0f));
            particles.LandBurst(feet, ev.landSpeed);
        }
        if (ev.footstep) {
            audio.Play(SoundId::Footstep, player.sprinting ? 0.6f : 0.4f);
            particles.FootstepPuff(feet, player.vel);
        }
    }

    camera = camRig.Update(dt, player.InterpPos(simClock.Alpha()), player.vel,
                           player.grounded, player.sprinting, world,
                           settings.gfx.fovY);

    // Interaction: gaze-based targeting works identically in both cameras.
    Vector3 eyeFw = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    currentTarget = interact.FindTarget(player.pos, camera.position, eyeFw);
    bool wantInteract = input.Pressed(Action::Interact) ||
                        (opts.autoplay && currentTarget >= 0);
    if (wantInteract && currentTarget >= 0) {
        const Interactable& it = interact.Items()[currentTarget];
        bool heart = (it.type == InteractType::HeartShrine);
        if (interact.Activate(currentTarget, heart ? 14 : 9)) {
            audio.Play(SoundId::ShrineChime, 0.9f, 1.0f, false);
            if (heart) audio.Play(SoundId::Blessing, 0.9f, 1.0f, false);
            particles.ShrineBurst(it.pos, Color{ 120, 245, 210, 255 });
            session.blessings++;
            hud.OnBanner(heart ? "The Heart of the Vale stirs"
                               : "The wayshrine acknowledges you");
            SaveNow();   // shrines double as autosave points
        }
    }

    // Wisps home to the chest, not the feet.
    Vector3 chest = Vector3Add(player.InterpPos(simClock.Alpha()), { 0, 1.1f, 0 });
    InteractEvents ev = interact.Update(dt, chest, particles);
    wispChainTimer = fmaxf(wispChainTimer - dt, 0.0f);
    if (wispChainTimer <= 0.0f) wispChain = 0;
    for (int i = 0; i < ev.wispsCollected; i++) {
        session.anima++;
        wispChain = (wispChain < 12) ? wispChain + 1 : 12;
        wispChainTimer = 1.2f;
        // Rising pitch per pickup in a chain — the payout crescendo.
        audio.Play(SoundId::WispPickup, 0.55f, 1.0f + wispChain * 0.055f, false);
        hud.OnAnimaGained();
    }

    particles.Update(dt);
    particles.AmbientMotes(player.pos, dt);
}

// ---- drawing --------------------------------------------------------------

void Game::DrawFrame(float dt) {
    postfx.EnsureTargets(settings.gfx.renderScale);   // no-op unless changed

    BeginDrawing();
    postfx.BeginScene();
    Vector3 fog = renderer.lighting.fogColor;
    ClearBackground(Color{ (unsigned char)(fog.x * 255), (unsigned char)(fog.y * 255),
                           (unsigned char)(fog.z * 255), 255 });

    float aspect = ui.ScreenW() / ui.ScreenH();
    renderer.BeginScene(camera, aspect, time);

    // Shrine activation flashes boost their world lights.
    PointLight lights[MAX_POINT_LIGHTS];
    int lightCount = (int)world.Lights().size();
    if (lightCount > MAX_POINT_LIGHTS) lightCount = MAX_POINT_LIGHTS;
    for (int i = 0; i < lightCount; i++) {
        lights[i] = world.Lights()[i];
        float boost = interact.LightBoost(i);
        lights[i].color = Vector3Scale(lights[i].color, boost);
        lights[i].range *= 0.8f + 0.2f * boost;
    }
    renderer.lighting.SetPointLights(lights, lightCount);

    terrain.Draw(renderer);
    world.Draw(renderer, camera.position, settings.gfx.viewDistance, time);
    interact.DrawWisps(renderer, time);

    bool showBody = (state == GameState::Title) || camRig.TpVisibility() > 0.25f;
    if (showBody) DrawPlayerBody(player.InterpPos(simClock.Alpha()));

    particles.Draw(camera);
    renderer.stats.particles = particles.AliveCount();
    renderer.EndScene();

    postfx.Composite(settings.gfx.bloom);
    DrawUiLayer(dt);
    EndDrawing();
}

void Game::DrawUiLayer(float) {
    Hud::DrawParams hp;
    hp.firstPerson = (camRig.Mode() == CameraView::FirstPerson) &&
                     camRig.TpVisibility() < 0.5f;
    hp.menuOpen = (state != GameState::Playing);
    hp.anima = session.anima;
    hp.blessings = session.blessings;
    if (state == GameState::Playing && currentTarget >= 0) {
        const Interactable& it = interact.Items()[currentTarget];
        hp.promptVerb = it.rearm > 0.0f ? "Resting..." : it.verb;
        hp.promptKey = InputSystem::CodeLabel(input.bindings[(int)Action::Interact].primary);
    }
    if (state != GameState::Title) hud.Draw(ui, hp);

    switch (state) {
        case GameState::Title: {
            TitleAction a = DrawTitleMenu(ui, SaveSystem::Exists(SAVE_SLOT), time);
            if (a == TitleAction::Continue) StartPlaying(true);
            if (a == TitleAction::NewJourney) StartPlaying(false);
            if (a == TitleAction::Settings) { settingsReturn = GameState::Title; state = GameState::SettingsMenu; }
            if (a == TitleAction::Quit) wantQuit = true;
            break;
        }
        case GameState::Paused: {
            PauseAction a = DrawPauseMenu(ui);
            if (a == PauseAction::None && input.Pressed(Action::Pause)) a = PauseAction::Resume;
            if (a == PauseAction::Resume) { state = GameState::Playing; SetMouseCapture(true); }
            if (a == PauseAction::Settings) { settingsReturn = GameState::Paused; state = GameState::SettingsMenu; }
            if (a == PauseAction::SaveAndTitle) { SaveNow(); state = GameState::Title; SetMouseCapture(false); }
            if (a == PauseAction::QuitDesktop) { SaveNow(); wantQuit = true; }
            break;
        }
        case GameState::SettingsMenu: {
            SettingsAction a = DrawSettingsMenu(ui, menuState, settings, input);
            bool escBack = input.Pressed(Action::Pause) && !input.CaptureActive();
            if (a == SettingsAction::Back || escBack) {
                settings.Save(input);
                state = settingsReturn;
                if (state == GameState::Playing) SetMouseCapture(true);
            }
            break;
        }
        case GameState::Playing:
            if (showDebug) DrawDebugOverlay();
            break;
    }
}

// Placeholder wanderer: hooded robe from primitives. Replaced by the real
// character + animation framework in a later phase; only game/ knows the
// player's visual form.
void Game::DrawPlayerBody(Vector3 feet) {
    const Color robe = { 52, 96, 104, 255 };
    const Color robeDark = { 40, 74, 82, 255 };
    const Color sash = { 214, 178, 96, 255 };
    const Color skin = { 224, 188, 152, 255 };
    float yaw = camRig.yaw;

    Matrix rot = MatrixRotateY(yaw);
    auto put = [&](const Mesh& mesh, Vector3 off, Vector3 scl, Color c) {
        Matrix m = MatrixMultiply(MatrixScale(scl.x, scl.y, scl.z), rot);
        Vector3 o = Vector3RotateByAxisAngle(off, { 0, 1, 0 }, yaw);
        m = MatrixMultiply(m, MatrixTranslate(feet.x + o.x, feet.y + o.y, feet.z + o.z));
        renderer.DrawLit(mesh, m, c);
    };

    put(renderer.cone, { 0, 0, 0 }, { 0.85f, 1.15f, 0.85f }, robeDark);        // skirt
    put(renderer.cylinder, { 0, 0.9f, 0 }, { 0.62f, 0.55f, 0.62f }, robe);     // torso
    put(renderer.cube, { 0, 1.18f, 0 }, { 0.58f, 0.12f, 0.5f }, sash);         // belt
    put(renderer.sphere, { 0, 1.58f, 0 }, { 0.34f, 0.36f, 0.34f }, skin);      // head
    put(renderer.cone, { 0, 1.52f, -0.02f }, { 0.5f, 0.5f, 0.5f }, robe);      // hood
}

void Game::DrawDebugOverlay() {
    const RenderStats& st = renderer.stats;
    Vector3 p = player.pos;
    DrawRectangle(8, 8, 350, 154, Color{ 10, 14, 20, 190 });
    DrawText(TextFormat("fps %d  (%.2f ms)", (int)(1.0f / fmaxf(smoothDt, 0.0001f)),
                        smoothDt * 1000.0f), 16, 14, 20, GREEN);
    DrawText(TextFormat("sim steps %d  alpha %.2f", lastSimSteps, simClock.Alpha()),
             16, 38, 20, RAYWHITE);
    DrawText(TextFormat("draws %d  culled %d chunks / %d props",
                        st.drawCalls, st.culledChunks, st.culledProps), 16, 62, 20, RAYWHITE);
    DrawText(TextFormat("particles %d", st.particles), 16, 86, 20, RAYWHITE);
    DrawText(TextFormat("pos %.1f %.1f %.1f  %s", p.x, p.y, p.z,
                        camRig.Mode() == CameraView::FirstPerson ? "FP" : "TP"),
             16, 110, 20, RAYWHITE);
    DrawText(TextFormat("vel %.1f  grounded %d", player.SpeedXZ(), player.grounded),
             16, 134, 20, RAYWHITE);
}
