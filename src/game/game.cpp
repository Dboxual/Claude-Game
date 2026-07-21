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
    if (!settings.gfx.fullscreen)
        app->SetResolution(settings.gfx.width, settings.gfx.height);
    app->SetFullscreen(settings.gfx.fullscreen);

    renderer.Init();

    // Original rigged Wayfarer model + its skeletal clips, rendered through the
    // scene's lit shader (per-vertex part colors survive as albedo).
    //
    // DISABLED: raylib 6.0's LoadModel corrupts the heap while loading our
    // skinned glTF (deterministic crash on some heap layouts, vanishes under a
    // debugger -> classic heap overflow). Root cause not yet found; the loader's
    // bone/color/index allocations all check out, so this needs an ASan run that
    // can init GL, or a minimal raylib repro. Until then we use the procedural
    // DrawPlayerBody (arms/legs/cape/walk/shadow). The whole pipeline + the
    // generator (tools/gen_wayfarer.py) stay in place; flip this to re-enable.
    const bool kEnableGltfHero = false;
    if (kEnableGltfHero) {
        heroModel = LoadModel("assets/models/wayfarer.glb");
        heroModelOk = heroModel.meshCount > 0;
        if (heroModelOk) {
            heroDefaultShader = heroModel.materials[0].shader;
            for (int i = 0; i < heroModel.materialCount; i++)
                heroModel.materials[i].shader = renderer.lighting.shader;
            heroAnims = LoadModelAnimations("assets/models/wayfarer.glb", &heroAnimCount);
            LOG_INFO("Wayfarer model: %d meshes, %d anims", heroModel.meshCount, heroAnimCount);
        }
    }

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
    playerFacingYaw = camRig.yaw;
    playerVisualPos = player.pos;
    if (opts.devMode && opts.devStartHour >= 0.0f)
        dayNight.SetHour(opts.devStartHour);
    dayNight.Update(0.0f, zones.CurrentDef().biome, renderer, false);

    if (opts.autoplay || opts.inputTour || opts.startZone >= 0) {
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
    if (heroModelOk) {
        if (heroAnims) UnloadModelAnimations(heroAnims, heroAnimCount);
        // Restore the model's own shader so UnloadModel doesn't free the shared
        // lit shader (renderer.Shutdown owns it).
        for (int i = 0; i < heroModel.materialCount; i++)
            heroModel.materials[i].shader = heroDefaultShader;
        UnloadModel(heroModel);
    }
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
    playerFacingYaw = camRig.yaw;
    playerStridePhase = 0.0f;
    playerVisualSpeed = 0.0f;
    playerVisualPos = player.pos;
    inputTourTime = 0.0f;
    tourJumped = tourEnteredFp = tourReturnedTp = false;
    tourDebugShown = tourDebugHidden = tourRewardSpawned = false;
    tourInteractPressed = tourPauseOpened = tourPauseResumed = false;
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
    bool fullscreenChanged = g.fullscreen != pg.fullscreen;
    bool resolutionChanged = g.width != pg.width || g.height != pg.height;
    if (fullscreenChanged) app->SetFullscreen(g.fullscreen);
    if (!g.fullscreen && (resolutionChanged || fullscreenChanged))
        app->SetResolution(g.width, g.height);
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
    pauseOpenedThisFrame = false;

    // Permission-free input tour. These are normal Action states, so the
    // recording exercises the same gameplay handlers as the bound keys while
    // remaining independent of opt-in creative/dev mode.
    if (opts.inputTour) {
        inputTourTime += dt;
        if (inputTourTime < 2.5f)       input.InjectTestAction(Action::MoveForward, true);
        else if (inputTourTime < 5.0f)  input.InjectTestAction(Action::MoveRight, true);
        else if (inputTourTime < 7.5f)  input.InjectTestAction(Action::MoveBack, true);
        else if (inputTourTime < 10.0f) input.InjectTestAction(Action::MoveLeft, true);
        else                            input.InjectTestAction(Action::MoveForward, true);
        if (inputTourTime >= 13.0f && inputTourTime < 16.0f)
            input.InjectTestAction(Action::MoveRight, true);
        if (inputTourTime >= 10.0f && inputTourTime < 13.0f)
            input.InjectTestAction(Action::Sprint, true);

        if (!tourJumped && inputTourTime >= 10.6f) {
            input.InjectTestAction(Action::Jump, true, true);
            tourJumped = true;
        }
        if (!tourEnteredFp && inputTourTime >= 12.0f) {
            input.InjectTestAction(Action::ToggleCamera, true, true);
            tourEnteredFp = true;
        }
        if (!tourReturnedTp && inputTourTime >= 14.0f) {
            input.InjectTestAction(Action::ToggleCamera, true, true);
            tourReturnedTp = true;
        }
        if (!tourDebugShown && inputTourTime >= 15.0f) {
            input.InjectTestAction(Action::DebugOverlay, true, true);
            tourDebugShown = true;
        }
        if (!tourDebugHidden && inputTourTime >= 17.0f) {
            input.InjectTestAction(Action::DebugOverlay, true, true);
            tourDebugHidden = true;
        }
        if (!tourInteractPressed && inputTourTime >= 17.55f) {
            input.InjectTestAction(Action::Interact, true, true);
            tourInteractPressed = true;
        }
        if (!tourPauseOpened && inputTourTime >= 18.65f) {
            input.InjectTestAction(Action::Pause, true, true);
            tourPauseOpened = true;
        } else if (!tourPauseResumed && inputTourTime >= 19.35f) {
            input.InjectTestAction(Action::Pause, true, true);
            tourPauseResumed = true;
        }
    }
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

    // Re-apply after zone transitions, whose biome loader establishes the
    // daytime baseline before this cycle grades it for the current hour.
    dayNight.Update(dt, zones.CurrentDef().biome, renderer,
                    state == GameState::Playing);

    ApplySettingsDiffs();
    DrawFrame(dt);

    for (; ui.hoverEvents > 0; ui.hoverEvents--) audio.Play(SoundId::UiHover, 0.5f);
    for (; ui.clickEvents > 0; ui.clickEvents--) audio.Play(SoundId::UiClick, 0.7f);
}

// ---- title ----------------------------------------------------------------

void Game::UpdateTitle(float dt) {
    // Escape is a permanent exit route from the title screen. In gameplay it
    // opens Pause, where Save & Quit remains available.
    if (input.Pressed(Action::Pause)) {
        wantQuit = true;
        return;
    }
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
        pauseOpenedThisFrame = true;
        SetMouseCapture(false);
        return;
    }
    if (input.Pressed(Action::ToggleCamera)) camRig.ToggleMode();
    if (input.Pressed(Action::DebugOverlay)) showDebug = !showDebug;

    DevCommands dev;
    if (opts.devMode) {
        dev = devTools.Poll();
        if (dev.toggleOverlay) devOverlay = !devOverlay;
        if (dev.toggleTurbo) devTurbo = !devTurbo;
        if (dev.advanceTime) {
            dayNight.AdvanceHours(3.0f);
            hud.OnBanner("World time advanced by three hours");
        }
        if (dev.toggleFly) {
            devFly = !devFly;
            player.vel = {};
            player.posPrev = player.pos;
            if (!devFly) {
                Vector3 landed = player.pos;
                landed.y = zone.world.GroundHeightAt(landed.x, landed.z, 1000.0f);
                player.Spawn(landed);
            }
            hud.OnBanner(devFly ? "Creative flight enabled" : "Creative flight disabled");
        }
        if (dev.nextZone) {
            int next = (zones.CurrentId() + 1) % ZoneCount();
            zones.LoadZone(next, -1, player, camRig);
            player.Spawn(zones.Current().world.SpawnPos());
            camRig.Init(zones.Current().world.SpawnYaw());
            playerFacingYaw = camRig.yaw;
            playerStridePhase = 0.0f;
            playerVisualPos = player.pos;
            currentTarget = -1;
            hud.OnBanner(TextFormat("Dev travel: %s", zones.CurrentDef().name));
        }
        if (dev.returnToSpawn) {
            player.Spawn(zone.world.SpawnPos());
            playerFacingYaw = camRig.yaw;
            playerStridePhase = 0.0f;
            playerVisualPos = player.pos;
            hud.OnBanner("Returned to the zone approach");
        }
        if (dev.spawnReward) {
            Vector3 origin = Vector3Add(player.pos, { 0, 1.2f, 0 });
            zone.interact.SpawnReward(origin, 18);
            hud.OnBanner("Dev reward burst (production pickup path)");
        }
    }

    if (mouseCaptured && !zones.Transitioning())
        camRig.AddLook(input.LookDelta(), settings.controls.mouseSensitivity,
                       settings.controls.invertY);

    PlayerIntents intents;
    if (!zones.Transitioning()) {
        intents.move = input.MoveAxis();
        intents.sprintDown = input.Down(Action::Sprint) || (opts.devMode && devTurbo);
        if (input.Pressed(Action::Jump)) pendingJump = true;
    }

    if (opts.inputTour && !zones.Transitioning()) {
        // Mouse-look portion of the Action tour. Locomotion and every button
        // above flow through InputSystem; only pointer deltas are applied here
        // because macOS blocks synthetic OS mouse events without permission.
        if (inputTourTime >= 10.0f && inputTourTime < 13.0f) {
            camRig.yaw += dt * 0.52f;                             // mouse look
        } else if (inputTourTime >= 13.0f && inputTourTime < 16.0f) {
            camRig.yaw -= dt * 0.42f;
            camRig.pitch = 0.08f + sinf(inputTourTime * 1.7f) * 0.12f;
        } else if (inputTourTime < 18.65f || inputTourTime >= 19.35f) {
            camRig.yaw += dt * 0.16f;
            camRig.pitch = ExpDecay(camRig.pitch, -0.04f, 3.0f, dt);
        }

        if (!tourRewardSpawned && inputTourTime >= 17.35f) {
            Vector3 forward = { sinf(camRig.yaw), 0.0f, -cosf(camRig.yaw) };
            Vector3 rewardOrigin = Vector3Add(player.pos, Vector3Scale(forward, 3.5f));
            rewardOrigin.y += 1.2f;
            Interactable rewardTest{};
            rewardTest.type = InteractType::Wayshrine;
            rewardTest.pos = rewardOrigin;
            rewardTest.radius = 5.5f;
            rewardTest.verb = "Test reward path";
            zone.interact.Add(rewardTest);
            tourRewardSpawned = true;
        }
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

    if (opts.devMode && devFly) {
        simClock = FixedClock{};
        lastSimSteps = 0;
        pendingJump = false;
        Vector3 fw = { sinf(camRig.yaw), 0, -cosf(camRig.yaw) };
        Vector3 rt = { cosf(camRig.yaw), 0, sinf(camRig.yaw) };
        Vector3 wish = Vector3Add(Vector3Scale(fw, intents.move.y),
                                  Vector3Scale(rt, intents.move.x));
        wish.y = dev.vertical;
        if (Vector3LengthSqr(wish) > 1.0f) wish = Vector3Normalize(wish);
        float flySpeed = devTurbo ? 30.0f : 12.0f;
        player.posPrev = player.pos;
        player.vel = Vector3Scale(wish, flySpeed);
        player.pos = Vector3Add(player.pos, Vector3Scale(player.vel, dt));
        player.posPrev = player.pos; // creative flight is a frame-rate test tool
        player.grounded = false;
    } else {
        int steps = simClock.AddFrame(dt);
        lastSimSteps = steps;
        for (int i = 0; i < steps; i++) {
            intents.jumpPressed = pendingJump;
            pendingJump = false;
            PlayerEvents ev = player.FixedUpdate(intents, camRig.yaw, zone.world);
            Vector3 feet = player.pos;
            // Movement sounds (jump/land/footstep) intentionally muted; visual
            // feedback (camera dip, dust) is kept.
            if (ev.landed) {
                camRig.OnLand(ev.landSpeed);
                particles.LandBurst(feet, ev.landSpeed);
            }
            if (ev.footstep) {
                particles.FootstepPuff(feet, player.vel);
            }
        }
    }

    // Third-person body orientation follows camera yaw exactly. Strafing and
    // backing up never turn the character away from the first-person forward
    // direction; this is the authored control rule, not a locomotion heuristic.
    float speed = player.SpeedXZ();
    playerFacingYaw = camRig.yaw;
    playerVisualSpeed = ExpDecay(playerVisualSpeed,
        player.grounded ? speed : 0.0f, 12.0f, dt);
    if (playerVisualSpeed > 0.12f)
        playerStridePhase += playerVisualSpeed * dt * (2.0f * PI / 2.15f);

    // Smooth only presentation Y to absorb tiny heightfield/fixed-tick steps.
    // X/Z stay fully interpolated and responsive. Large changes are teleports.
    Vector3 simFeet = player.InterpPos(simClock.Alpha());
    playerVisualPos.x = simFeet.x;
    playerVisualPos.z = simFeet.z;
    if (fabsf(simFeet.y - playerVisualPos.y) > 2.0f)
        playerVisualPos.y = simFeet.y;
    else
        playerVisualPos.y = ExpDecay(playerVisualPos.y, simFeet.y, 30.0f, dt);

    camera = camRig.Update(dt, playerVisualPos, player.vel,
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
                    particles.ShrineBurst(it.pos, Color{ 76, 170, 255, 255 });
                    if (heart) {
                        session.blessings++;
                        hud.OnBanner("Waystone attuned - journey anchored");
                        SaveNow();
                    } else {
                        hud.OnBanner("Anima drawn from the echo well");
                    }
                }
            }
        }
    }

    Vector3 chest = Vector3Add(playerVisualPos, { 0, 1.1f, 0 });
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

    int previousZone = zones.CurrentId();
    zones.Update(dt, player, camRig);
    if (zones.CurrentId() != previousZone) {
        playerFacingYaw = camRig.yaw;
        playerStridePhase = 0.0f;
        playerVisualSpeed = 0.0f;
        playerVisualPos = player.pos;
    }
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
    if (showBody) {
        Vector3 feet = playerVisualPos;
        DrawContactShadow(feet);
        if (heroModelOk) DrawHeroModel(feet, dt);
        else DrawPlayerBody(feet);
    }

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
    hp.zoneName = zones.CurrentDef().name;
    hp.zoneTier = zones.CurrentDef().tier;
    hp.cameraYaw = camRig.yaw;
    hp.clockText = dayNight.ClockText();
    hp.showControlHints = session.playtime < 18.0f;
    hp.anima = session.anima;
    hp.blessings = session.blessings;
    hp.devMode = opts.devMode;
    hp.devOverlay = devOverlay;
    hp.devFly = devFly;
    hp.devTurbo = devTurbo;
    if (state == GameState::Playing && currentTarget >= 0) {
        const Interactable& it = zone.interact.Items()[currentTarget];
        hp.promptVerb = it.rearm > 0.0f ? "Resting..." : it.verb.c_str();
        hp.promptKey = InputSystem::CodeLabel(input.bindings[(int)Action::Interact].primary);
    }
    if (state != GameState::Title) hud.Draw(ui, hp);

    switch (state) {
        case GameState::Title: {
            TitleAction a = DrawTitleMenu(ui, SaveSystem::Exists(SAVE_SLOT),
                                          zones.CurrentDef().name, time);
            if (a == TitleAction::Continue) StartPlaying(true);
            if (a == TitleAction::NewJourney) StartPlaying(false);
            if (a == TitleAction::Settings) { settingsReturn = GameState::Title; state = GameState::SettingsMenu; }
            if (a == TitleAction::Quit) wantQuit = true;
            break;
        }
        case GameState::Paused: {
            PauseAction a = DrawPauseMenu(ui);
            if (a == PauseAction::None && input.Pressed(Action::Pause) &&
                !pauseOpenedThisFrame) a = PauseAction::Resume;
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

// Soft, alpha-blended contact shadow that grounds the character on the terrain.
// Drawn before the body/model so opaque parts occlude it; fades while airborne.
void Game::DrawContactShadow(Vector3 feet) {
    float air = player.grounded ? 1.0f : 0.55f;
    float r = 0.52f * air;
    Vector3 s0 = { feet.x, feet.y + 0.03f, feet.z };
    Vector3 s1 = { feet.x, feet.y + 0.05f, feet.z };
    BeginBlendMode(BLEND_ALPHA);
    DrawCylinderEx(s0, s1, r, r, 20, Color{ 12, 20, 14, (unsigned char)(120 * air) });
    DrawCylinderEx(s0, s1, r * 0.6f, r * 0.6f, 18, Color{ 8, 14, 10, (unsigned char)(90 * air) });
    EndBlendMode();
}

// Draw the rigged Wayfarer model: pick Walk/Idle by movement, advance the clip,
// CPU-skin it, and render through the scene's lit shader facing travel.
void Game::DrawHeroModel(Vector3 feet, float dt) {
    // Both authored character bodies use +Z as their front. raylib's +Y matrix
    // rotation has the opposite yaw sign to CameraRig, so this adapter must
    // invert the camera angle as well as apply the half-turn. At every yaw the
    // body's forward axis now exactly equals CameraRig::Forward() projected XZ.
    float renderYaw = PI - playerFacingYaw;
    if (heroAnimCount <= 0 || !heroAnims) {   // safety: draw static bind pose
        renderer.SetEmissive(0.0f);
        DrawModelEx(heroModel, feet, Vector3{ 0, 1, 0 }, renderYaw * RAD2DEG,
                    Vector3{ 1, 1, 1 }, WHITE);
        return;
    }
    float speed = player.SpeedXZ();
    bool moving = player.grounded && speed > 0.6f;
    float dur = moving ? 1.0f : 2.6f;
    int idx = (!moving && heroAnimCount > 1) ? 1 : 0;
    for (int i = 0; i < heroAnimCount; i++)
        if (TextIsEqual(heroAnims[i].name, moving ? "Walk" : "Idle")) { idx = i; break; }

    heroAnimTime += dt;
    const ModelAnimation& a = heroAnims[idx];
    int frame = a.keyframeCount > 0
        ? (int)(fmodf(heroAnimTime, dur) / dur * a.keyframeCount) % a.keyframeCount : 0;
    UpdateModelAnimation(heroModel, a, frame);

    renderer.SetEmissive(0.0f);
    DrawModelEx(heroModel, feet, Vector3{ 0, 1, 0 }, renderYaw * RAD2DEG,
                Vector3{ 1, 1, 1 }, WHITE);
}

// Chunky low-poly adventurer built from the primitive kit: hooded head with
// a face band, tunic with a vest and bronze pauldrons, swinging arms/legs with
// boots, a mythic cape and glowing gem, and a sheathed blade. A speed-driven
// walk cycle animates the limbs. Seed of the future animation framework.
void Game::DrawPlayerBody(Vector3 feet) {
    // Palette: teal cloth identity, warm leather, bronze metal, mythic accents.
    const Color pants    = { 44, 52, 60, 255 };
    const Color leather  = { 92, 62, 42, 255 };
    const Color cloth    = { 50, 98, 106, 255 };
    const Color pauldron = { 170, 128, 72, 255 };
    const Color skin     = { 226, 178, 140, 255 };
    const Color hood     = { 58, 50, 70, 255 };
    const Color faceBand = { 30, 36, 44, 255 };
    const Color cape     = { 74, 60, 92, 255 };
    const Color blade    = { 150, 220, 214, 255 };
    const Color gem      = { 120, 235, 220, 255 };

    float yaw = PI - playerFacingYaw;
    float speed = playerVisualSpeed;
    bool moving = player.grounded && speed > 0.6f;

    // Stride phase comes from actual distance traveled. The root stays planted;
    // only articulated parts move, eliminating the previous vertical jitter.
    float phase = sinf(playerStridePhase);
    float swingAmt = moving ? Clamp(speed * 0.10f, 0.2f, 0.75f) : 0.0f;
    float legSwing = phase * swingAmt;
    float armSwing = moving ? -phase * swingAmt * 0.85f : sinf(time * 1.6f) * 0.05f;
    float lean = Clamp(speed * 0.02f, 0.0f, 0.14f);
    float bounce = 0.0f;
    float capePitch = -0.14f - speed * 0.04f
                      + (moving ? sinf(playerStridePhase * 1.1f) * 0.045f
                                : sinf(time * 1.6f) * 0.02f);

    // Body-local frame -> lean (X) then facing (Y).
    Matrix rot = MatrixMultiply(MatrixRotateX(lean), MatrixRotateY(yaw));

    // Draw a part centered at `center` (body-local), rotated by pitch/roll about
    // `pivot` (a joint), then oriented by the body and planted at the feet.
    auto part = [&](const Mesh& mesh, Vector3 center, Vector3 scl, Color c,
                    float pitch, float roll, Vector3 pivot, float emis) {
        Matrix m = MatrixScale(scl.x, scl.y, scl.z);
        m = MatrixMultiply(m, MatrixTranslate(center.x - pivot.x, center.y - pivot.y, center.z - pivot.z));
        m = MatrixMultiply(m, MatrixMultiply(MatrixRotateX(pitch), MatrixRotateZ(roll)));
        m = MatrixMultiply(m, MatrixTranslate(pivot.x, pivot.y, pivot.z));
        m = MatrixMultiply(m, rot);
        m = MatrixMultiply(m, MatrixTranslate(feet.x, feet.y + bounce, feet.z));
        renderer.DrawLit(mesh, m, c, emis);
    };
    auto put = [&](const Mesh& mesh, Vector3 center, Vector3 scl, Color c) {
        part(mesh, center, scl, c, 0.0f, 0.0f, center, 0.0f);
    };

    // Legs + boots (swing about the hips, left/right opposed).
    for (int s = -1; s <= 1; s += 2) {
        Vector3 hip = { 0.13f * s, 0.84f, 0.0f };
        float sw = legSwing * s;
        part(renderer.cube, { 0.13f * s, 0.44f, 0.0f }, { 0.17f, 0.82f, 0.21f }, pants, sw, 0.0f, hip, 0.0f);
        part(renderer.cube, { 0.13f * s, 0.07f, 0.05f }, { 0.2f, 0.14f, 0.32f }, leather, sw, 0.0f, hip, 0.0f);
    }

    // Hips / belt, tunic torso (slightly slimmer + taller), chest vest, gem.
    const Color glove = { 74, 52, 36, 255 };
    put(renderer.cube, { 0, 0.9f, 0 }, { 0.42f, 0.22f, 0.3f }, leather);
    put(renderer.cube, { 0, 1.24f, 0 }, { 0.46f, 0.56f, 0.3f }, cloth);
    put(renderer.cube, { 0, 1.26f, 0.15f }, { 0.34f, 0.4f, 0.06f }, leather);
    put(renderer.sphere, { 0, 1.3f, 0.19f }, { 0.09f, 0.09f, 0.07f }, gem);
    part(renderer.sphere, { 0, 1.3f, 0.19f }, { 0.09f, 0.09f, 0.07f }, gem, 0, 0, { 0, 1.3f, 0.19f }, 0.9f);

    // Pauldrons.
    for (int s = -1; s <= 1; s += 2)
        put(renderer.cube, { 0.29f * s, 1.48f, 0 }, { 0.22f, 0.16f, 0.3f }, pauldron);

    // Arms: cloth upper + leather bracer forearm + glove, splayed slightly
    // outward and swinging about the shoulders opposite the legs.
    for (int s = -1; s <= 1; s += 2) {
        Vector3 shoulder = { 0.30f * s, 1.48f, 0.0f };
        float sw = armSwing * s;
        float splay = 0.13f * s;
        part(renderer.cube, { 0.30f * s, 1.18f, 0.0f }, { 0.13f, 0.44f, 0.15f }, cloth, sw, splay, shoulder, 0.0f);
        part(renderer.cube, { 0.30f * s, 0.82f, 0.02f }, { 0.14f, 0.42f, 0.16f }, leather, sw, splay, shoulder, 0.0f);
        part(renderer.cube, { 0.30f * s, 0.6f, 0.03f }, { 0.14f, 0.16f, 0.16f }, glove, sw, splay, shoulder, 0.0f);
    }

    // Neck, head, face band, and a proper hood/cowl (no witch-hat point).
    put(renderer.cube, { 0, 1.56f, 0 }, { 0.15f, 0.13f, 0.15f }, skin);
    put(renderer.sphere, { 0, 1.7f, 0 }, { 0.33f, 0.37f, 0.33f }, skin);
    put(renderer.cube, { 0, 1.7f, 0.15f }, { 0.2f, 0.1f, 0.06f }, faceBand);
    put(renderer.sphere, { 0, 1.75f, -0.05f }, { 0.42f, 0.4f, 0.46f }, hood);
    put(renderer.cube, { 0, 1.84f, 0.05f }, { 0.34f, 0.1f, 0.3f }, hood);   // brim shading the face
    put(renderer.cube, { 0, 1.6f, 0.0f }, { 0.44f, 0.16f, 0.44f }, hood);   // cowl over the shoulders

    // Cape flowing from the shoulders.
    part(renderer.cube, { 0, 1.1f, -0.22f }, { 0.5f, 0.9f, 0.05f }, cape,
         capePitch, 0.0f, { 0, 1.52f, -0.2f }, 0.0f);

    // Sheathed blade slung across the back (glowing edge = mythic identity).
    part(renderer.cube, { -0.12f, 1.2f, -0.26f }, { 0.06f, 0.92f, 0.03f }, blade, 0.0f, 0.5f, { -0.12f, 1.2f, -0.26f }, 0.35f);
    part(renderer.cube, { 0.05f, 0.86f, -0.26f }, { 0.28f, 0.07f, 0.09f }, pauldron, 0.0f, 0.5f, { 0.05f, 0.86f, -0.26f }, 0.0f);
    part(renderer.cube, { 0.12f, 0.74f, -0.26f }, { 0.06f, 0.2f, 0.07f }, leather, 0.0f, 0.5f, { 0.12f, 0.74f, -0.26f }, 0.0f);
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
