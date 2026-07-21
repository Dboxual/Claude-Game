# ZION — Tasks (living document)

Phase 1 — Foundation. Update as work completes; never delete history,
strike it through or move it to Done.

## In progress

- [ ] Epic A: zone infrastructure (huge zones, zone graph, transitions) —
      full spec in CODEX_HANDOFF.md §6. The handoff file is the
      authoritative backlog from here; keep both it and this file updated.

## Foundation usability and world-feel — completed 2026-07-21

- [x] Add researched `ALBION_PARITY_CHECKLIST.md` covering the live 2026 feature
      set, original-expression boundary, FP/TP translation, schemas, and tests
- [x] Make Escape a permanent safety key and title-screen exit route; prevent
      its opening press from being re-consumed as an immediate pause-menu resume
- [x] Add persisted resolution choices to Graphics settings (live on Windows;
      pre-window startup apply/restart on macOS to avoid raylib Retina corruption)
- [x] Make the active character face camera-forward at every yaw, independent
      of forward/back/strafe travel direction
- [x] Replace wall-clock/root-bounce walking with planted, distance-driven stride,
      zero head bob, and render-only ground-height smoothing
- [x] Add biome-aware day/night lighting/sky/fog and a HUD clock
- [x] Add an opt-in `--dev` creative panel and isolated test-data support
- [x] Add provisional vitals/ability-bar HUD template
- [x] Turn uniform scatter into groves, boulder fields, clear routes, and
      route-adjacent Echo Wells; make the central Waystone a save anchor
- [x] Replace smooth natural-form spheres with a faceted low-poly mesh baseline
- [x] Add clean, non-colliding grass coverage while keeping roads/hubs readable;
      add no branch, pebble, or litter scatter

## Presentation identity — completed 2026-07-21

- [x] Add the exhaustive sandbox-MMO feature parity/build-gate plan in
      MASTER_ROADMAP.md and the PS2 mythic art/UI/FX bible in
      VISUAL_DIRECTION.md
- [x] Establish the Wayfinder's Archive UI palette and ornamented global
      panel/button material
- [x] Recompose the title screen as an ink-glass navigation slab over a live
      world vista
- [x] Add gameplay zone/risk plaque, live heading compass, short onboarding
      control card, and anima/Ledger reward plate
- [x] Upgrade crosshair, interaction prompt, and reward banner presentation
- [x] Give XP/anima wisps clearly pulsing violet and gold families with emissive
      halos and orbiting sparks
- [x] Replace the central shrine's pinkish illumination with a blue crystal,
      blue light, and cool-blue altar surface
- [x] Add permission-free PNG-frame/WebP recordings plus a normal-mode action
      tour covering WASD, sprint, jump, camera look/toggle, interact, F3, and Esc

## Next up (Phase 1 polish candidates — now Epic B of CODEX_HANDOFF.md)

- [ ] Gamepad support in the input system (bindings reserve space for it)
- [ ] Footstep material variation (grass vs stone via terrain paint lookup)
- [ ] Biome-specific grass/flower variation without branches, loose rocks, or litter
- [ ] God-ray-ish post shader when facing the sun
- [ ] Extract player poses into a reusable `src/anim/` framework
- [ ] Settings: apply-and-confirm countdown for disruptive display changes
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
- [x] 2026-07-21 normal-mode recorded action tour: all four movement directions,
      sprint/jump/look, FP/TP, F3, real E interaction, visible Esc pause/resume;
      all nine spawned wisps collected and camera-forward facing retained
- [x] 2026-07-21 production shrine recording: central Waystone activated,
      14/14 wisps collected, one blessing earned, blue shrine treatment reviewed
- [x] 960x540 launch/screenshot confirms the HUD and world fill the selected size

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
