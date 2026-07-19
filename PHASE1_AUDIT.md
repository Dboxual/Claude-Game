# Zion Phase 1 technical and visual audit

Audit date: 2026-07-19 UTC

## Executive assessment

Zion is a playable, attractive proof of concept, not yet a scalable game
foundation. The current branch contains a static browser build with roughly
400 lines of active, densely packed JavaScript and procedurally constructed
Three.js art. It demonstrates the intended loop and presentation vocabulary,
but most long-term systems are shallow UI/state simulations in one module.

Preserve the authored Gloamwood composition, palette, basic camera modes,
equipment-driven ability concept, interaction vocabulary, pooled particles,
instancing/batching experiments, and profiling hooks. Do not build the MMO
feature set directly on the current monolith. Stabilize and separate simulation,
rendering, content data, UI, input, persistence, and diagnostics first.

The current 58 FPS validation is not evidence that Zion meets a hardware tier:
it is CPU-rendered SwiftShader at 60% internal resolution. No native-resolution
physical-GPU baseline exists yet.

## Repository protection and status

- Exact root: `/home/umbrel/umbrel/home/Documents/Zion`
- Branch at audit start: `main`, clean, two commits ahead of `origin/main`
- Modified/untracked files at audit start: none
- Important uncommitted work: none detected
- Checkpoint: `2cab5a9 checkpoint: Zion before phase 1 audit`
- Git author identity was absent; the recent local commit identity (`Claude
  <noreply@anthropic.com>`) was set in this repository only to create the
  requested checkpoint.
- The checkpoint makes the branch three commits ahead of `origin/main`. Nothing
  was reset, discarded, or deleted.

Recent history replaces, rather than evolves, the earlier native project:

- `f7821ca`: C++20/SDL3/OpenGL movement prototype with fixed 128 Hz simulation.
- `93424a2`: compiler-enforced shared/client/server layers, settings, menu, and
  headless dedicated-server shell.
- `f2e126f`: deletes that 3,622-line native foundation and replaces it with the
  current browser RPG prototype.
- `8a1b425` and `318e7d4`: adaptive scaling/touch and validation captures.

The referenced GitHub repository is the current remote/history of this local
project, not a separate comparable implementation. Its most useful reference
material is therefore the pre-replacement history described above.

## Project inventory

### Active runtime

- Languages: HTML, CSS, browser JavaScript (ES modules).
- Framework/rendering: Three.js 0.166.1, `WebGLRenderer`, ACES tone mapping,
  optional `EffectComposer`/`UnrealBloomPass`.
- Delivery/build: static files over HTTP; no build, bundling, asset pipeline,
  linting, type checking, CI, or release packaging.
- Dependency delivery: Three.js and addons load at runtime from jsDelivr via an
  import map. Offline play and reproducible dependency supply are not provided.
- Entry point: `index.html` loads `game3d.js`; `gloamwood-visuals.js` builds
  procedural actors/world art.
- `game.js`: older 2D canvas implementation, currently not loaded.
- Tools: ad hoc Playwright validation/capture and GPU diagnostic scripts in
  `tools/`; `tools/package.json` has no useful scripts and its test command
  intentionally fails.
- Runtime requirements: modern browser with WebGL2 preferred, ES modules,
  network access to jsDelivr, and an HTTP server.

### Repository footprint and assets

- Working tree excluding Git: approximately 72 MB.
- Captures: approximately 60 MB, including screenshots and duplicate MP4/WebM
  recordings. These are validation artifacts, not game assets.
- Runtime visual/audio assets: no external models, textures, music, or samples.
  Meshes and synthesized tones/noise are generated in code.
- `tools/node_modules` exists locally (about 13 MB) but is ignored. There is a
  package lock inside the ignored directory, not a normal checked-in lockfile.

### Documentation read

`README.md`, `DESIGN-BIBLE.md`, `PROGRESS.md`, `ASSET_CREDITS.md`, `.gitignore`,
`tools/package.json`, validation/profile JSON, source comments, current Git
history, and the earlier native README/build configuration were inspected.
There was no `GOALS.md`; it has been created as the single phased roadmap.

## Main loop and system boundaries

`requestAnimationFrame` calls a variable-step `update(dt, t)`, renders, samples
FPS, and schedules the next frame. `dt` is capped at 33 ms. There is no fixed
simulation accumulator, interpolation, pause model, deterministic clock, or
separation between simulation and presentation. Timers for attacks, gathering,
respawns, and UI effects use wall-clock `setTimeout`, so they can advance while
frame simulation stalls and cannot be authoritatively replayed.

`game3d.js` owns renderer setup, quality adaptation, global state, input, touch,
audio synthesis, combat, AI, gathering, crafting, progression, quests, UI HTML,
camera, effects, and the loop. `window.__zion` exposes internals for diagnostics.
This is expedient for a slice but cannot safely support multiplayer authority,
large content sets, automated simulation tests, save migration, or parallel
feature work.

## What works now

Reproduced or supported by direct implementation inspection:

- Static HTTP boot and Three.js scene initialization.
- Third-person camera, first-person arms/view toggle, orbit, zoom, basic camera
  obstruction raycast, and target lock.
- WASD/tap movement over height-function terrain; keyboard, mouse, and touch
  bindings exist.
- Basic melee, three abilities, cooldown/energy, simple enemy pursuit/attacks,
  health, death, respawn, hit effects, XP orbs, and loot pickups.
- Eight gathering nodes with hide/12-second respawn behavior.
- Inventory display, six recipes, three short quest stages, action-based mastery
  counters, one-time faction selection, and local warband founding.
- Authored procedural Gloamwood route, procedural characters/enemies, first-
  person weapon, UI shell, minimap, synthesized feedback audio.
- Static geometry merging, four ground-cover instanced meshes, instanced contact
  shadows, a 72-spark pool, enemy distance culling, and throttled minimap/camera
  collision updates.
- JavaScript syntax checks pass. Fresh SwiftShader smoke run produced no console
  or page errors.

## Broken, misleading, or not reproducible

- The default render ceiling is 60% of CSS resolution on a 1x desktop display;
  it can fall to 30%. This is silent internal resolution reduction despite a
  small FPS HUD and is visibly soft. At the tested 1440×900 viewport the canvas
  was only 864×540. It conflicts with the crisp native-resolution direction.
- The adaptive policy treats FPS above 55 as headroom and may re-enable bloom
  before reaching native resolution; it has no explicit player preset, no
  persistence, and no GPU/CPU-bound diagnosis.
- `tools/zion-validate.mjs` fails on a clean invocation here because Chromium
  system libraries are undeclared. It ran only after pointing `LD_LIBRARY_PATH`
  at previously extracted libraries under `/tmp`.
- The README says 1/2/4 are abilities but the HTML includes a locked slot 3;
  this is presentation rather than a complete equipment ability system.
- The guild UI does not disable its button when funds are insufficient (the
  click handler silently refuses); most panel tabs are decorative and inactive.
- Documentation calls the architecture "persistence-ready," but there is no
  save/load, settings persistence, schema, database, or network boundary.
- Historical performance JSON files describe different code states and
  resolutions. They are not comparable benchmark runs with hardware metadata.
- Source initialization constructs extensive legacy scenery, actors, nodes, and
  enemies, then removes/clears them and replaces them with Gloamwood. They do
  not remain draw cost, but create needless startup allocations and obscure the
  active implementation.

## Unfinished or placeholder content

- All actors, world geometry, materials, icons, portrait, item art, animations,
  sounds, and environment treatment are procedural primitives/placeholders.
  They are original and useful for iteration, but not production assets.
- Movement is direct position translation: no acceleration, robust collision,
  stairs/slopes rules, jumping, crouch, swimming, navmesh, or controller support.
- Enemy AI is radius pursuit and timer attacks; there is no pathfinding,
  encounter state, leash, crowd behavior, or robust collision.
- Equipment cannot actually be swapped and does not drive abilities. Stats,
  item power, rarity, loot, mastery unlocks, factions, guilds, quests, economy,
  territories, PvP, multiplayer, and persistence are prototype facades.
- Crafting has a handful of in-memory recipes. There is no data format, station,
  quality, durability, provenance, bank, market, transport, or balancing model.
- No save/load, options menu, remapping, gamepad, localization, accessibility
  settings, content pipeline, networking, server, telemetry, or automated tests.
- World bounds are one small route. No streaming, LOD system, occlusion plan,
  spatial index, chunk ownership, or scalable entity lifecycle exists.

## Fresh performance and hardware baseline

Host observed during audit:

- CPU: AMD Ryzen 7 255 with Radeon 780M Graphics, 8 cores/16 threads.
- RAM: 60 GiB visible, about 54 GiB available during inspection.
- Physical display adapter: AMD Phoenix3 Radeon 780M, `amdgpu` driver.
- OS/runtime: Debian Linux 6.12.85, Node 22.13.0, npm 10.9.2,
  Python 3.13.5, Playwright 1.55.0.

Fresh smoke result after supplying local browser libraries:

| Metric | Result |
|---|---:|
| Renderer actually used | ANGLE Vulkan SwiftShader (CPU software renderer) |
| Visible viewport | 1440×900 |
| Internal canvas | 864×540 (60%) |
| Reported rolling FPS | 58.1 |
| Draw calls | 65 |
| Triangles | 34,812 |
| Active combat particles | 0 at sample |
| Console/page errors | 0 |

The test did **not** use the Radeon 780M. Chromium reported software GPU
compositing, WebGL readback, no hardware Vulkan, and SwiftShader as the device.
It provides a repeatable functional stress reference only. It does not reveal
physical GPU utilization, GPU frametime, power, VRAM/shared-memory pressure, or
performance on a toaster laptop, normal laptop, or RTX 4060 Ti desktop.

Historical correction data reported 41.4 average FPS, 33.4 ms p95, a 20 FPS
minimum, 132 peak draw calls, 37,752 peak triangles, and 43 peak particles at
50% scale. A later historical validation reported 60 FPS, 40 calls, and 28,719
triangles at 60% scale. These runs differ in scene/action and cannot establish a
trend without a fixed benchmark protocol.

### Likely performance causes, ordered by present evidence

1. Fill rate and post-processing at higher resolution: the project responds by
   lowering resolution and optionally dropping the full-screen composer.
2. CPU software rendering in headless validation, which makes existing numbers
   unrepresentative of physical GPUs.
3. Full-world visibility: historical `world-hidden` tests rose to about 59 FPS,
   while actor hiding barely changed results; static world rendering dominates
   the measured slice more than actors.
4. Per-frame CPU/GC work: DOM queries and style writes, array copies/splices,
   vector clones, all-pollen buffer writes, enemy loops, shadow matrices, and
   allocations in orb/lock/camera paths occur in the central update.
5. No scalable spatial/LOD/streaming architecture. Current distance hiding only
   covers enemies; one compact world can be batched, but larger worlds cannot.
6. Startup waste from constructing and discarding the legacy scene.

Exact CPU-vs-GPU attribution remains unknown until timer queries or browser
tracing and OS GPU counters are captured on physical target hardware.

## Visual baseline

Strengths worth preserving are the warm bronze/violet UI identity, readable
silhouettes, strong central route composition, colorful restrained magic,
first/third-person support, and an original low-poly vocabulary.

Current captures also show major baseline defects:

- Low internal resolution makes terrain and silhouettes soft/smeared; scanlines
  compound the loss of clarity.
- Terrain is visibly faceted and stretched, with repetitive muddy color bands.
- Several captures frame empty terrain/sky rather than readable combat action.
- Character anatomy, faces, weapons, foliage, ruins, resource icons, and audio
  are clear placeholders.
- UI is polished relative to the world but oversized and crowded at 720p;
  management panels contain decorative, nonfunctional tabs and empty slots.
- Lighting can crush actors into dark silhouettes, and the scene has limited
  material separation without relying on emissive accents.

## Architecture fitness for the long-term plan

The current architecture cannot support the full plan without focused
restructuring, but a large rewrite is neither necessary nor advisable now.
Three.js/WebGL can continue to support a deliberately scoped client and is
useful for rapid iteration. The blocker is not JavaScript alone; it is the lack
of boundaries, deterministic simulation, data-driven content, persistence,
tooling, and an authoritative server model.

The earlier native history contains principles worth recovering, not code to
blindly restore: fixed-step simulation with interpolation, config-driven
movement/settings, platform/input separation, renderer abstraction where
needed, compiler-enforced shared/client/server boundaries, and a headless smoke
target. Its tactical FPS mechanics and native rendering code do not map directly
onto Zion and should not be copied wholesale.

## Priority order

1. Make measurement trustworthy: fixed native-resolution scenes at 720p,
   900p, and 1080p; Low/Medium/High presets; warm-up and scripted routes;
   p50/p95/p99 and 1% lows; CPU/GPU timing; memory; renderer/driver metadata.
2. Stop silent blur: default to native resolution, expose dynamic resolution as
   opt-in/explicit, and make quality decisions by measured cost.
3. Restore reproducibility: pin/localize dependencies, add a real lockfile and
   scripts, make browser prerequisites clear, and run smoke tests in CI.
4. Introduce a fixed-step simulation clock and replace gameplay `setTimeout`
   behavior with simulation-owned timers. Preserve render interpolation.
5. Split the existing module incrementally into input, simulation, rendering,
   UI, content definitions, audio, persistence, and diagnostics. Keep behavior
   stable while extracting tests.
6. Remove legacy startup construction only after regression coverage confirms it
   is unused. Do not delete the old 2D file merely because it appears unused;
   decide its archival role explicitly.
7. Build a minimal settings/save schema and gamepad/remapping path.
8. Validate first/third-person movement, collision, camera, and combat feel
   before adding progression breadth.
9. Perform an authoritative networking spike before committing large economy,
   guild, territory, PvP, or persistent-world systems.

## Phase 1 conclusion

The slice is worth preserving as a visual/gameplay prototype and benchmark
scene. It proves a direction, not the target architecture or performance. Phase
2 should be a bounded foundation-and-measurement pass, with no major gameplay
expansion and no wholesale engine rewrite.
