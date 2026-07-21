# Zion Development Progress

## 2026-07-21 - Camera-forward locomotion, blue shrines, clean grass, recorded key tour

Completed:

- Corrected the full yaw-space character adapter (`PI - cameraYaw`): the
  procedural body now faces exactly where the camera faces at every heading,
  including strafing, backing up, and live orbit turns.
- Removed locomotion bob/root bounce and added render-only terrain-height
  smoothing, eliminating the visible walking jitter without softening X/Z input.
- Recolored shrine crystals, light, activation burst, and the central altar
  surface to blue. XP/anima orbs now pulse in violet or gold only; alpha-blended
  bodies preserve their hue while emissive bloom and trails supply the glow.
- Added lightweight, non-colliding grass patches in deliberate meadow coverage.
  No branches, pebbles, fallen debris, or other ambient litter were added.
- Fixed Escape's same-frame open/resume bug. The initial Escape now leaves the
  pause menu open; a later Escape resumes, and the menu retains Save/Quit actions.
- Added `--record-dir` back-buffer sampling and
  `tools/make_test_recording.py` for deterministic animated WebP evidence.
  `--tour` runs outside dev mode and injects normal Action states, not mechanic
  shortcuts, covering WASD, Shift, Space, V, F3, E, Esc, and camera look.

Verification:

- Warning-free macOS build and `git diff --check` pass.
- `final_action_keys_v3.webp`: 274 frames / 22.9 seconds, real E activation,
  nine wisps collected, visible pause/resume, camera-forward body through yaw.
- `final_blue_shrine_v2.webp`: production autoplay activated the central
  Waystone, collected 14/14 wisps, and earned one blessing.
- A 960x540 launch and screenshot verified the selected framebuffer size and
  scaled UI composition.

Next:

- Resume Epic A persistence/transition work or begin the first complete
  gather-to-crafted-equipment loop from `ALBION_PARITY_CHECKLIST.md`.
- Replace procedural placeholder trees/rocks only with authored species and
  geology; preserve the no-litter ground-composition rule.

## 2026-07-21 - Player feel, world purpose, day/night, dev mode, parity contract

Completed:

- Corrected the active Wayfarer fallback's initial 180-degree facing error.
  This was later superseded by exact camera-forward yaw; see the entry above.
- Replaced wall-clock walk cadence and whole-body bounce with distance-driven
  stride phase and a planted root; reduced residual FP camera sway.
- Made Escape a permanent navigation key, including quitting from the title,
  and wired persisted window resolutions into the app. Windows applies live;
  macOS reads before `InitWindow` and labels restart-required, avoiding a verified
  raylib Retina framebuffer/projection corruption during live resize.
- Added a 36-real-minute biome-aware day/night presentation cycle with sun,
  sky, ambient, fog, HUD clock, and a dev-only three-hour time jump.
- Added opt-in `--dev` creative flight/turbo/reward/zone/spawn/time controls.
  Reward testing uses the same production pickup path; normal gameplay never
  depends on dev state. `--data-dir` isolates AI/automation output.
- Added a provisional combat HUD (vitals + equipment-ability slots) as a
  redesignable template.
- Reworked procedural placement into groves and boulder fields, reserved clear
  hub-to-gate travel corridors, moved Echo Wells near travel routes, and gave
  the central Waystone an attune/save purpose. Natural forms now use a faceted
  low-poly mesh instead of smooth spheres.
- Added `ALBION_PARITY_CHECKLIST.md`, researched against first-party 2023-2026
  Albion material and the official wiki. It specifies classless builds,
  equipment, progression, gathering, crafting, economy, danger zones, mounts,
  PvE/PvP, guilds, islands, social, server, UI/accessibility, FP/TP translation,
  developer tests, and milestone gates.

Verification:

- macOS build passes with `-Wall -Wextra` and no warnings.
- 360-frame TP autoplay/dev smoke passes at about 115 FPS at 1240x720.
- A saved 960x540 setting now creates a correctly filled 960x540 window on macOS;
  the post-fix 180-frame TP smoke passes at about 108 FPS.
- A final 2,400-frame normal-mode TP soak completed at about 119 FPS, activated
  the Waystone, and collected all 14 emitted anima wisps (1 blessing).
- A deterministic `--dev --time 22` night capture verified the full atmosphere
  path; moonlit ambient was raised after review so terrain, ruins, and the player
  remain readable without losing the deep-blue night mood.
- Screenshot confirms the active character faces toward travel, the UI template
  scales, and the dev panel is visibly opt-in.
- `git diff --check` passes.

Known limits:

- The supplied YouTube reference is recorded in the visual docs, but the browser
  runtime exposed no usable browser, so an exact frame study still needs attached
  screenshots or a later browser-enabled session.
- The skinned GLB loader remains disabled due to the already documented raylib 6
  heap corruption. The corrected procedural fallback is active.

Next:

- Finish per-zone persistence and a repeated gate-transition leak soak.
- Build the item/inventory schema and one complete tree-to-crafted-gear loop.
- Replace placeholder natural forms with authored species/geology assets while
  retaining the new placement grammar.

## 2026-07-21 - Character overhaul, presentation polish, glTF pipeline (blocked)

Completed:

- Fixed the macOS build: it never linked `glfw` or `-framework CoreVideo`, so
  raylib's window/input symbols were undefined. Verified `run_macos.sh` builds
  and runs (~110 FPS) on Apple Silicon.
- Replaced the placeholder cone-wizard `DrawPlayerBody` with a procedural hooded
  adventurer (jointed arms/legs, cape, sheathed blade, speed-driven walk cycle,
  lean/bob) and added a soft alpha-blended contact shadow that grounds it.
- Muted movement SFX (jump/land/footstep); kept dust + camera feedback.
- Window: default 1200x700, positioned on-screen so the title bar/close button
  stay reachable (opening larger than the display made it hard to move/quit).
- Calmer first-person: halved head-bob amplitude, cut horizontal sway and the
  sprint FOV kick.
- Trees redone: tapered trunk + clumpy layered canopy instead of a lollipop.
- Built the original-asset model pipeline: `tools/gen_wayfarer.py` authors a
  100% original rigged, skeletally-animated Wayfarer (glTF), and the engine
  loads/animates it through the scene's lit shader. Added `Renderer::SetEmissive`.

Blocked / deviation:

- The glTF hero path is DISABLED (`kEnableGltfHero = false` in game.cpp). raylib
  6.0 `LoadModel` heap-corrupts on our skinned glTF (deterministic SIGSEGV on
  some heap layouts, no crash under a debugger). Model data validates clean;
  raylib's loader allocations check out. Needs an ASan/headless-GL run or a
  minimal repro. Procedural body is the active character meanwhile. Also fixed a
  real raylib bug: its animation loader deref's `skin.joints[0]->parent`, so the
  generator wraps the skeleton in an Armature node.

## 2026-07-21 - Presentation identity and master roadmap

Completed:

- Added `MASTER_ROADMAP.md`, an exhaustive product-parity map covering the
  classless build system, combat, gathering, crafting, regional economy,
  risk zones, PvE/PvP activities, factions, guild territory, housing, social
  systems, service architecture, accessibility, milestone gates, and UI order.
- Added `VISUAL_DIRECTION.md`, the visual acceptance bible for Zion's colorful
  PS2-era mythic look, including palette tokens, UI materials, reward/FX
  grammar, camera rules, zone identity, animation, and review checks.
- Rebuilt the title composition around a dark wayfinder archive slab that
  preserves the live world vista and added a unified night-ink, ancient-gold,
  and spirit-teal widget material.
- Added a gameplay zone/risk plaque, heading compass, first-session control
  hints, anima/Ledger plate, mythic FP reticle, richer interaction prompt, and
  ceremonial reward banner.
- Expanded shrine wisps with magical halos, orbiting sparks, colored trails,
  and pickups. The active XP palette was later narrowed to violet/gold.

Verification:

- macOS Release build passes with `-Wall -Wextra` and no warnings.
- Fresh title, FP, and TP screenshot smokes pass; a repeated TP capture
  confirmed a one-frame screenshot artifact was not reproducible.
- A 3,000-frame TP reward soak completes at about 119 FPS, activates one
  shrine, collects all 14 wisps, and updates the anima/Ledger HUD correctly.

Next:

- Finish Epic A per-zone persistence and repeated gate-transition soak.
- Add gate/shrine pips to the compass and build the 3x3 world-map screen.
- Extract the procedural body pose into `src/anim/` before creatures/combat.

Deviations: none. The UI layout and art are original; Albion references were
used only to check system coverage in the product roadmap.

## 2026-07-20 - Foundation stability and physics polish

Completed:

- Fixed the shutdown access violation caused by reading the active zone name
  after `ZoneInstance::Shutdown()` cleared its definition pointer.
- Removed temporary crash-bisection environment switches and per-frame trace
  logging left in the zone work in progress.
- Fixed prop placement so the caller receives the sampled terrain height.
  Colliders, point lights, and interaction targets now line up vertically with
  their rendered props across every biome.
- Corrected scaled prop collider heights, removed the invisible upper half of
  broken-column colliders, added the missing altar-block collider, and fixed
  camera sphere overlap checks.
- Improved cylinder and world-border collision response: exact-center overlaps
  resolve deterministically and inward velocity is removed while tangent
  velocity is preserved for wall sliding.
- Added steep-slope rejection and based footsteps on actual distance traveled,
  preventing uphill rim climbing and footsteps while pushing into a wall.
- Guarded invalid zone-gate transition requests and made spatial-grid query
  stamp rollover safe. Travel text now retains its destination through fade-in.
- Added local obstacle avoidance to autoplay so the enlarged-zone smoke test
  can route around temple columns and exercise the reward loop again.

Verification:

- Release build passes with MSVC `/W4`.
- FP and TP 300-frame screenshot smokes pass at about 137 FPS.
- All nine zones initialize, render, and shut down cleanly.
- A 2,000-frame TP soak passes at about 143 FPS and writes valid save/settings
  JSON in an isolated data directory.
- A 3,000-frame reward soak passes at about 142 FPS, activates one shrine, and
  collects all 14 emitted anima wisps.

Next:

- Finish dedicated repeated gate-transition soak coverage and per-zone state
  persistence before marking Epic A complete.
- Add focused controller/world collision tests when a non-rendering test target
  is introduced.

Deviations: none. This session stayed within foundation and Epic A polish.

## 2026-07-20 - Output-preserving render optimization

Completed:

- Cached unchanged lighting, atmosphere, sky, and point-light uniforms instead
  of resending identical values to the graphics driver every frame.
- Cached post-processing shader locations and skipped the bloom texture sample
  when bloom is disabled.
- Frustum-culled point-light influence spheres before the forward lighting pass;
  lights that cannot affect visible fragments no longer run in the fragment
  shader.
- Added conservative 160 m terrain cull blocks. A rejected block skips 25
  per-chunk frustum tests without changing terrain LODs or visible geometry.
- Exposed the active point-light count in the F3 overlay for profiling.

Visual tradeoffs: none. Terrain density, LOD distances, view distance, bloom
quality, lighting equations, and particle budgets are unchanged.

Verification:

- Primary and isolated Release builds pass with MSVC `/W4`.
- FP and TP screenshots match the existing composition with bloom enabled.
- Bloom-disabled rendering passes without reading the unused bloom texture.
- All nine zones initialize, render, and shut down cleanly.
- A 2,000-frame TP reward soak completes with 14 anima and 1 blessing at the
  monitor's 143 FPS VSync ceiling.
