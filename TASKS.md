# ZION — Tasks (living document)

Phase 1 — Foundation. Update as work completes; never delete history,
strike it through or move it to Done.

## In progress

- [ ] Epic A: zone infrastructure (huge zones, zone graph, transitions) —
      full spec in CODEX_HANDOFF.md §6. The handoff file is the
      authoritative backlog from here; keep both it and this file updated.

## Next up (Phase 1 polish candidates — now Epic B of CODEX_HANDOFF.md)

- [ ] Gamepad support in the input system (bindings reserve space for it)
- [ ] Footstep material variation (grass vs stone via terrain paint lookup)
- [ ] More mid-ground set dressing: flower patches, fallen logs, path stones
- [ ] God-ray-ish post shader when facing the sun
- [ ] Player body: simple walk-cycle lean/bob (animation framework seed)
- [ ] Settings: resolution picker (currently borderless/window only)
- [ ] macOS build verification (scripts exist, untested this round)

## Phase 1 breakdown — DONE 2026-07-20

### Docs & scaffolding
- [x] GOALS.md / ROADMAP.md / ARCHITECTURE.md / TASKS.md / README.md
- [x] CMake + vcpkg (raylib, nlohmann-json) + presets + run scripts

### Core
- [x] App loop: runtime custom-frame-control detection, manual poll/swap/dt,
      fps cap; fixed-step clock (60 Hz) with render interpolation alpha
- [x] Log (console + user-dir file), paths (per-OS user data dir), seeded RNG
- [x] Input system: 10 actions, dual-slot rebinding with capture + conflict
      stealing, ESC-latch fix, serialization via settings JSON
- [x] Settings system: graphics/audio/controls, forward-compatible JSON
- [x] Save system: slot JSON (pos, camera, anima, blessings, playtime),
      autosave at shrines and on quit; Continue on title

### World — "Elder Vale"
- [x] Heightfield terrain: FBM hills, temple plateau, rim mountains,
      8x8 world-space chunks, vertex-color paint (grass/gold/rock/scree)
- [x] Props: temple (dais + 8-column ring, 2 broken), 4 wayshrines,
      6 rune monoliths, standing-stone ring, ~120 trees, ~60 rocks
- [x] Collision: capsule vs cylinders + walkable platform discs + world border
- [x] Interaction framework: gaze+proximity targeting (camera-agnostic),
      activation events, 30 s shrine re-arm, light-boost flash
- [x] Wisp rewards: burst → scatter → velocity-steered homing → collect,
      rising-pitch chime chain, HUD pop (validated: soak collects all 9)

### Player & cameras
- [x] Kinematic controller: ExpDecay accel/friction, coyote + jump buffer,
      sprint, ground snap + step-up, landing/footstep events
- [x] FP camera: eye bob, landing dip spring, sprint FOV kick
- [x] TP camera: sprung boom with point-sampled collision (snap in/ease out),
      shoulder offset, smooth FP<->TP blend; --tp smoke flag

### Rendering
- [x] Lighting: hemisphere ambient + wrapped sun + 8 point lights + distance
      fog, emissive path for crystals/runes (feeds bloom)
- [x] Sky dome: gradient, sun disc + halo, two-octave drifting clouds
- [x] Post-fx: render-scale scene RT, bright-pass + double gaussian bloom,
      grade (contrast/saturation/warmth), vignette
- [x] Culling: geometric frustum planes (self-orienting) vs chunk AABBs and
      prop spheres + distance cull; stats in debug overlay

### UI / Audio / FX
- [x] UI kit: panel/label/button/slider/toggle/selector/bind-button,
      hover glow anims, id-hashed state, system TTF with fallback
- [x] Menus: title (orbit cinematic), pause, settings (3 tabs, live apply)
- [x] HUD: crosshair (FP only), key-chip interact prompt, anima gem counter
      with pop, blessing banner, fade-in
- [x] Debug overlay (F3): fps/ms, sim steps, draws, culled, particles, pos
- [x] Audio: 8 synthesized sounds + seamless wind-loop stream, bus volumes,
      pitch jitter, UI hover/click hooks
- [x] Particles: 4096 pool, alpha + additive passes, footstep/land/shrine/
      wisp-trail/collect/ambient-mote emitters, density setting

### Verification
- [x] Smoke flags --frames/--screenshot/--autoplay/--tp wired end to end
- [x] Screenshot passes: spawn FP, autoplay roam, title UI, TP body, shrine
      banner — all reviewed
- [x] Soak: 2000 frames autoplay = 1 blessing, 9/9 wisps, ~143 fps avg
- [x] Perf: ~140 fps at 1600x900 with bloom on dev machine; ~200 draws
      typical after culling (GTX 1650 target has headroom)

## Done

- 2026-07-20: Project reset for Zion. Previous GODGUN prototype preserved in
  git stash "GODGUN prototype backup (pre-Zion foundation)". Docs written.
- 2026-07-20: Phase 1 foundation vertical slice complete (all systems above).

## Notes / parked ideas

- Wisp orbit bug fixed via velocity steering (ExpDecay toward desired vel);
  raw acceleration let them circle at v^2/a radius — keep this pattern for
  future homing (loot, projectiles).
- raylib vcpkg build has SUPPORT_CUSTOM_FRAME_CONTROL: App handles poll/swap
  manually. Never assume EndDrawing does frame work.
- UI font atlas is ASCII-95 only — no em-dashes/unicode in UI strings.
