#include "game/game.h"
#include "core/app.h"
#include "core/log.h"
#include "core/mathx.h"

static constexpr unsigned int WORLD_SEED = 20260720u;
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
    postfx.Init(settings.gfx.renderScale);
    audio.Init(settings.audio);
    particles.Init();
    particles.SetDensity(settings.gfx.particleDensity);
    ui.Init();

    zones.Init(WORLD_SEED, renderer);
    int firstZone = (opts.startZone >= 0 && opts.startZone < ZoneCount())
                        ? opts.startZone : DEFAULT_ZONE;
    zones.LoadZone(firstZone, -1, player, camRig);
    player.Spawn(zones.Current().world.SpawnPos());
    camRig.Init(zones.Current().world.SpawnYaw());

    if (opts.autoplay || opts.startZone >= 0) {
        StartPlaying(false);     // soak / zone-test runs skip the title
    } else {
        state = GameState::Title;
        SetMouseCapture(false);
    }
}

void Game::Shutdown() {
    if (state != GameState::Title) SaveNow();
    settings.Save(input);
    LOG_INFO("Session end: %.1fs, anima %d, blessings %d, zone %s",
             time, session.anima, session.blessings, zones.CurrentDef().name);
    ui.Shutdown();
    particles.Shutdown();
    audio.Shutdown();
    postfx.Shutdown();
    zones.Current().Shutdown();
    renderer.Shutdown();
}

// ---- flow helpers ---------------------------------------------------------

void Game::StartPlaying(bool fromSave) {
    if (fromSave) {
        session = SaveSystem::Load(SAVE_SLOT);
        if (session.currentZone != zones.CurrentId() &&
            session.currentZone >= 0 && session.currentZone < ZoneCount())
            zones.LoadZone(session.currentZone, -1, player, camRig);
        player.Spawn(session.playerPos);
        camRig.Init(session.yaw);
        camRig.pitch = session.pitch;
        camRig.SetMode(session.cameraMode == 1 ? CameraView::ThirdPerson
                                               : CameraView::FirstPerson);
    } else {
        session = SaveData{};
        session.valid = true;
        session.currentZone = zones.CurrentId();
        player.Spawn(zones.Current().world.SpawnPos());
        camRig.Init(zones.Current().world.SpawnYaw());
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
    session.currentZone = zones.CurrentId();
    SaveSystem::Write(SAVE_SLOT, session);
}

void Game::SetMouseCapture(bool on) {
    if (on == mouseCaptured) return;
    mouseCaptured = on;
    if (opts.smokeFrames > 0) return;
    if (on) DisableCursor(); else EnableCursor();
}

void Game::ApplySettingsDiffs() {
    GraphicsSettings& g = settings.gfx;
    GraphicsSettings& pg = prevSettings.gfx;
    if (g.fullscreen != pg.fullscreen) app->SetFullscreen(g.fullscreen);
    if (g.fpsCap != pg.fpsCap) app->SetFpsCap(g.fpsCap);
    if (g.particleDensity != pg.particleDensity) particles.SetDensity(g.particleDensity);
    AudioSettings& a = settings.audio;
    AudioSettings& pa = prevSettings.audio;
    if (a.master != pa.master || a.sfx != pa.sfx ||
        a.music != pa.music || a.ambience != pa.ambience)
        audio.Apply(a);
    prevSettings = settings;
}

float Game::EffectiveViewDistance() const {
    // Dense/foggy biomes trade view distance for draw budget.
    return settings.gfx.viewDistance * zones.CurrentDef().biome.viewDistanceScale;
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
            particles.Update(dt);
            break;
    }

    ApplySettingsDiffs();
    DrawFrame(dt);

    for (; ui.hoverEvents > 0; ui.hoverEvents--) audio.Play(SoundId::UiHover, 0.5f);
    for (; ui.clickEvents > 0; ui.clickEvents--) audio.Play(SoundId::UiClick, 0.7f);
}

// ---- title ----------------------------------------------------------------

void Game::UpdateTitle(float dt) {
    titleOrbit += dt * 0.07f;
    float r = 40.0f;
    float base = zones.CurrentDef().temple ? zones.Current().terrain.plateauHeight
                                           : zones.Current().terrain.HeightAt(0, 0);
    camera.position = { sinf(titleOrbit) * r, base + 12.0f + sinf(titleOrbit * 0.7f) * 2.0f,
                        cosf(titleOrbit) * r };
    camera.target = { 0, base + 3.0f, 0 };
    camera.up = { 0, 1, 0 };
    camera.fovy = 55.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    particles.Update(dt);
    particles.AmbientMotes(camera.position, dt);
}

// ---- playing --------------------------------------------------------------

void Game::UpdatePlaying(float dt) {
    session.playtime += dt;
    ZoneInstance& zone = zones.Current();

    if (input.Pressed(Action::Pause) && !zones.Transitioning()) {
        state = GameState::Paused;
        SetMouseCapture(false);
        return;
    }
    if (input.Pressed(Action::ToggleCamera)) camRig.ToggleMode();
    if (input.Pressed(Action::DebugOverlay)) showDebug = !showDebug;

    if (mouseCaptured && !zones.Transitioning())
        camRig.AddLook(input.LookDelta(), settings.controls.mouseSensitivity,
                       settings.controls.invertY);

    PlayerIntents intents;
    if (!zones.Transitioning()) {
        intents.move = input.MoveAxis();
        intents.sprintDown = input.Down(Action::Sprint);
        if (input.Pressed(Action::Jump)) pendingJump = true;
    }

    gateCooldown = fmaxf(gateCooldown - dt, 0.0f);
    if (opts.autoplay && !zones.Transitioning()) {
        // Scripted soak: steer toward the nearest available interactable
        // (shrines first; gates once armed shrines run out), hop sometimes.
        intents.move = { 0, 1 };
        intents.sprintDown = fmodf(time, 9.0f) > 6.0f;
        const Interactable* nearest = nullptr;
        float bestD2 = 1e9f;
        for (const Interactable& it : zone.interact.Items()) {
            if (it.rearm > 0.0f) continue;
            if (it.type == InteractType::ZoneGate && gateCooldown > 0.0f) continue;
            float dx = it.pos.x - player.pos.x, dz = it.pos.z - player.pos.z;
            float d2 = dx * dx + dz * dz;
            // Prefer shrines: gates count as farther than they are.
            if (it.type == InteractType::ZoneGate) d2 *= 4.0f;
            if (d2 < bestD2) { bestD2 = d2; nearest = &it; }
        }
        if (nearest) {
            float toYaw = atan2f(nearest->pos.x - player.pos.x,
                                 -(nearest->pos.z - player.pos.z));
            float targetDist = sqrtf(bestD2);
            if (targetDist > 2.0f) {
                auto probeBlocked = [&](float yaw) {
                    float distance = fminf(targetDist * 0.5f, 1.4f);
                    Vector3 p = { player.pos.x + sinf(yaw) * distance,
                                  player.pos.y + PlayerController::HEIGHT * 0.5f,
                                  player.pos.z - cosf(yaw) * distance };
                    return zone.world.PointBlocked(p, PlayerController::RADIUS + 0.12f);
                };
                if (probeBlocked(toYaw)) {
                    float left = toYaw - 0.9f;
                    float right = toYaw + 0.9f;
                    toYaw = !probeBlocked(left) ? left : right;
                }
            }
            camRig.yaw = ExpDecayAngle(camRig.yaw, toYaw, 2.5f, dt);
            camRig.pitch = ExpDecay(camRig.pitch, bestD2 < 120.0f ? 0.4f : 0.0f, 2.0f, dt);
        } else {
            camRig.yaw += dt * 0.22f;
        }
        autoplayJumpTimer += dt;
        if (autoplayJumpTimer > 4.0f) { pendingJump = true; autoplayJumpTimer = 0.0f; }
    }

    int steps = simClock.AddFrame(dt);
    lastSimSteps = steps;
    for (int i = 0; i < steps; i++) {
        intents.jumpPressed = pendingJump;
        pendingJump = false;
        PlayerEvents ev = player.FixedUpdate(intents, camRig.yaw, zone.world);
        Vector3 feet = player.pos;
        if (ev.jumped) audio.Play(SoundId::Jump, 0.55f);
        if (ev.landed) {
            camRig.OnLand(ev.landSpeed);
            audio.Play(SoundId::Land, Clamp(ev.landSpeed * 0.1f, 0.3f, 1.0f));
            particles.LandBurst(feet, ev.landSpeed);
        }
        if (ev.footstep) {
            // Ground material varies the step: rock rings tighter/higher.
            GroundKind kind = zone.terrain.KindAt(feet.x, feet.z);
            float pitch = kind == GroundKind::Rock ? 1.3f
                        : kind == GroundKind::Dry ? 1.12f : 1.0f;
            float vol = (player.sprinting ? 0.6f : 0.4f) *
                        (kind == GroundKind::Rock ? 0.8f : 1.0f);
            audio.Play(SoundId::Footstep, vol, pitch);
            particles.FootstepPuff(feet, player.vel);
        }
    }

    camera = camRig.Update(dt, player.InterpPos(simClock.Alpha()), player.vel,
                           player.grounded, player.sprinting, zone.world,
                           settings.gfx.fovY);

    // Interaction: gaze-based targeting, identical in both cameras.
    currentTarget = -1;
    if (!zones.Transitioning()) {
        Vector3 eyeFw = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        currentTarget = zone.interact.FindTarget(player.pos, camera.position, eyeFw);
        bool wantInteract = input.Pressed(Action::Interact) ||
                            (opts.autoplay && currentTarget >= 0 &&
                             (zone.interact.Items()[currentTarget].type != InteractType::ZoneGate ||
                              gateCooldown <= 0.0f));
        if (wantInteract && currentTarget >= 0) {
            const Interactable& it = zone.interact.Items()[currentTarget];
            if (it.type == InteractType::ZoneGate) {
                zones.RequestTransition(it.payload);
                gateCooldown = 8.0f;
                audio.Play(SoundId::ShrineChime, 0.55f, 1.15f, false);
            } else {
                bool heart = (it.type == InteractType::HeartShrine);
                if (zone.interact.Activate(currentTarget, heart ? 14 : 9)) {
                    audio.Play(SoundId::ShrineChime, 0.9f, 1.0f, false);
                    if (heart) audio.Play(SoundId::Blessing, 0.9f, 1.0f, false);
                    particles.ShrineBurst(it.pos, Color{ 120, 245, 210, 255 });
                    session.blessings++;
                    hud.OnBanner(heart ? "The Heart of the Vale stirs"
                                       : "The wayshrine acknowledges you");
                    SaveNow();
                }
            }
        }
    }

    Vector3 chest = Vector3Add(player.InterpPos(simClock.Alpha()), { 0, 1.1f, 0 });
    InteractEvents ev = zone.interact.Update(dt, chest, particles);
    wispChainTimer = fmaxf(wispChainTimer - dt, 0.0f);
    if (wispChainTimer <= 0.0f) wispChain = 0;
    for (int i = 0; i < ev.wispsCollected; i++) {
        session.anima++;
        wispChain = (wispChain < 12) ? wispChain + 1 : 12;
        wispChainTimer = 1.2f;
        audio.Play(SoundId::WispPickup, 0.55f, 1.0f + wispChain * 0.055f, false);
        hud.OnAnimaGained();
    }

    zones.Update(dt, player, camRig);
    particles.Update(dt);
    particles.AmbientMotes(player.pos, dt);
}

// ---- drawing --------------------------------------------------------------

void Game::DrawFrame(float dt) {
    postfx.EnsureTargets(settings.gfx.renderScale);
    ZoneInstance& zone = zones.Current();

    BeginDrawing();
    postfx.BeginScene();
    Vector3 fog = renderer.lighting.fogColor;
    ClearBackground(Color{ (unsigned char)(fog.x * 255), (unsigned char)(fog.y * 255),
                           (unsigned char)(fog.z * 255), 255 });

    float aspect = ui.ScreenW() / ui.ScreenH();
    renderer.BeginScene(camera, aspect, time);

    PointLight lights[MAX_POINT_LIGHTS];
    int lightCount = 0;
    int sourceLightCount = (int)zone.world.Lights().size();
    if (sourceLightCount > MAX_POINT_LIGHTS) sourceLightCount = MAX_POINT_LIGHTS;
    for (int i = 0; i < sourceLightCount; i++) {
        PointLight light = zone.world.Lights()[i];
        float boost = zone.interact.LightBoost(i);
        light.color = Vector3Scale(light.color, boost);
        light.range *= 0.8f + 0.2f * boost;
        if (!renderer.Visible(light.pos, light.range)) continue;
        lights[lightCount++] = light;
    }
    renderer.lighting.SetPointLights(lights, lightCount);
    renderer.stats.pointLights = lightCount;

    zone.terrain.Draw(renderer, camera.position);
    zone.world.Draw(renderer, camera.position, EffectiveViewDistance(), time);
    zone.interact.DrawWisps(renderer, time);

    bool showBody = (state == GameState::Title) || camRig.TpVisibility() > 0.25f;
    if (showBody) DrawPlayerBody(player.InterpPos(simClock.Alpha()));

    particles.Draw(camera);
    renderer.stats.particles = particles.AliveCount();
    renderer.EndScene();

    postfx.Composite(settings.gfx.bloom);
    DrawUiLayer(dt);

    // Zone-travel fade rides above everything, including menus.
    if (zones.FadeAlpha() > 0.001f) {
        DrawRectangle(0, 0, (int)ui.ScreenW(), (int)ui.ScreenH(),
                      Fade(BLACK, zones.FadeAlpha()));
        if (zones.Transitioning() && zones.FadeAlpha() > 0.35f)
            ui.LabelCentered({ ui.ScreenW() / 2, ui.ScreenH() * 0.5f },
                             TextFormat("Traveling to %s...", zones.TravelTargetName()),
                             30, Fade(ui.theme.accentGlow, Saturate(zones.FadeAlpha())));
    }
    EndDrawing();
}

void Game::DrawUiLayer(float) {
    ZoneInstance& zone = zones.Current();
    Hud::DrawParams hp;
    hp.firstPerson = (camRig.Mode() == CameraView::FirstPerson) &&
                     camRig.TpVisibility() < 0.5f;
    hp.menuOpen = (state != GameState::Playing);
    hp.anima = session.anima;
    hp.blessings = session.blessings;
    if (state == GameState::Playing && currentTarget >= 0) {
        const Interactable& it = zone.interact.Items()[currentTarget];
        hp.promptVerb = it.rearm > 0.0f ? "Resting..." : it.verb.c_str();
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

// Placeholder wanderer: hooded robe from primitives, with a light
// procedural walk lean (the animation framework grows from here).
void Game::DrawPlayerBody(Vector3 feet) {
    const Color robe = { 52, 96, 104, 255 };
    const Color robeDark = { 40, 74, 82, 255 };
    const Color sash = { 214, 178, 96, 255 };
    const Color skin = { 224, 188, 152, 255 };
    float yaw = camRig.yaw;

    // Lean into horizontal velocity; a touch of bounce while striding.
    float speed = player.SpeedXZ();
    float lean = Clamp(speed * 0.022f, 0.0f, 0.16f);
    float bounce = player.grounded && speed > 0.8f
                       ? fabsf(sinf(time * speed * 2.9f)) * 0.05f : 0.0f;

    Matrix rot = MatrixMultiply(MatrixRotateX(lean), MatrixRotateY(yaw));
    auto put = [&](const Mesh& mesh, Vector3 off, Vector3 scl, Color c) {
        Matrix m = MatrixMultiply(MatrixScale(scl.x, scl.y, scl.z), rot);
        Vector3 o = Vector3RotateByAxisAngle(off, { 0, 1, 0 }, yaw);
        m = MatrixMultiply(m, MatrixTranslate(feet.x + o.x, feet.y + o.y + bounce, feet.z + o.z));
        renderer.DrawLit(mesh, m, c);
    };

    put(renderer.cone, { 0, 0, 0 }, { 0.85f, 1.15f, 0.85f }, robeDark);
    put(renderer.cylinder, { 0, 0.9f, 0 }, { 0.62f, 0.55f, 0.62f }, robe);
    put(renderer.cube, { 0, 1.18f, 0 }, { 0.58f, 0.12f, 0.5f }, sash);
    put(renderer.sphere, { 0, 1.58f, 0 }, { 0.34f, 0.36f, 0.34f }, skin);
    put(renderer.cone, { 0, 1.52f, -0.02f }, { 0.5f, 0.5f, 0.5f }, robe);
}

void Game::DrawDebugOverlay() {
    const RenderStats& st = renderer.stats;
    Vector3 p = player.pos;
    DrawRectangle(8, 8, 380, 178, Color{ 10, 14, 20, 190 });
    DrawText(TextFormat("fps %d  (%.2f ms)", (int)(1.0f / fmaxf(smoothDt, 0.0001f)),
                        smoothDt * 1000.0f), 16, 14, 20, GREEN);
    DrawText(TextFormat("zone %s (t%d)", zones.CurrentDef().name, zones.CurrentDef().tier),
             16, 38, 20, SKYBLUE);
    DrawText(TextFormat("sim steps %d  alpha %.2f", lastSimSteps, simClock.Alpha()),
             16, 62, 20, RAYWHITE);
    DrawText(TextFormat("draws %d  culled %d chunks / %d props",
                        st.drawCalls, st.culledChunks, st.culledProps), 16, 86, 20, RAYWHITE);
    DrawText(TextFormat("particles %d  lights %d", st.particles, st.pointLights),
             16, 110, 20, RAYWHITE);
    DrawText(TextFormat("pos %.0f %.0f %.0f  %s", p.x, p.y, p.z,
                        camRig.Mode() == CameraView::FirstPerson ? "FP" : "TP"),
             16, 134, 20, RAYWHITE);
    DrawText(TextFormat("vel %.1f  grounded %d", player.SpeedXZ(), player.grounded),
             16, 158, 20, RAYWHITE);
}
