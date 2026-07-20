# ZION — Architecture

## Stack

- **Language:** C++20 (MSVC on Windows, AppleClang on macOS)
- **Build:** CMake ≥ 3.21 + vcpkg manifest mode (`vcpkg.json`)
- **Platform/render/audio base:** raylib (OpenGL 3.3) — windowing, input
  polling, mesh/shader/texture/audio primitives. All raylib use is confined
  to leaf modules so a future backend swap (consoles) touches a bounded set
  of files.
- **JSON:** nlohmann-json (settings, saves, bindings)
- **Assets:** procedural-first. Geometry, textures, and sounds are generated
  in code at startup — zero binary assets in the repo. File-based assets can
  be layered in later behind the same load points.

### Critical toolchain note

The vcpkg raylib is built with `SUPPORT_CUSTOM_FRAME_CONTROL`:
`EndDrawing()` does **not** swap buffers, poll input, or track frame time.
`core/app.cpp` detects this at runtime and drives `PollInputEvents()`,
`SwapScreenBuffer()`, and dt computation manually. Never add a second
draw loop that assumes standard raylib behavior.

## Frame model

```
main.cpp
  └─ App::RunLoop
       ├─ poll input, compute clamped dt
       ├─ Game::Frame(dt)
       │    ├─ InputSystem::BeginFrame        (actions from raw input)
       │    ├─ state machine (Title / Playing / Paused / Settings)
       │    ├─ SIMULATION: fixed steps @ 60 Hz (accumulator)
       │    │    ├─ PlayerController::FixedUpdate  (movement vs World)
       │    │    └─ World::FixedUpdate             (interactables, wisps)
       │    ├─ PRESENTATION: once per frame, uses interpolation alpha
       │    │    ├─ CameraRig::Update   (look is per-frame for latency)
       │    │    ├─ Particles::Update, Audio::Update
       │    │    └─ Renderer: sky → world (culled) → particles → post → UI
       └─ swap buffers, frame cap
```

- **Fixed timestep (60 Hz) simulation, interpolated rendering.** Gameplay
  math is framerate-independent and deterministic-friendly (required for the
  Phase 6 server). Entities store `posPrev`/`pos`; rendering lerps by the
  accumulator alpha.
- **Mouse look updates per-frame**, not per-tick, for minimal input latency.

## Module map (`src/`)

| Module | Files | Owns | May depend on |
| --- | --- | --- | --- |
| `core/` | app, log, paths, clock, mathx, rng | window+loop, logging, user-data dirs, fixed-step clock, easing/springs, seeded RNG | raylib only |
| `input/` | input | action enum, bindings, rebind capture, per-frame action state | core |
| `settings/` | settings | graphics/audio/control values, JSON persistence | core, input (bindings) |
| `save/` | save | save slots (player pos, progress), JSON persistence | core |
| `world/` | terrain, world, interact | heightfield gen + chunk meshes + height/normal queries, prop placement, capsule collision, interactable registry | core, render (mesh kit) |
| `player/` | controller, camera_rig | kinematic movement state machine; FP/TP camera, transitions, boom collision | core, input, world |
| `render/` | lighting, sky, renderer, postfx | lighting shader + uniforms, sky dome, culled scene draw + stats, bloom/grade chain | core |
| `fx/` | particles | pooled particle sim + billboard draw, emitter helpers | core, render (camera) |
| `audio/` | audio | device, buses, synthesized SFX/ambience, play API | core |
| `ui/` | ui, hud | immediate-mode widgets, theme; HUD + debug overlay | core, input |
| `game/` | game, menus | system ownership, state machine, slice content wiring, menu screens | everything above |

Dependency rule: arrows point downward only (game → systems → core).
Systems never include `game/`. Cross-system communication happens in
`game/` glue code or via small event structs returned from updates.

## Key decisions & rationale

- **Kinematic capsule vs. analytic colliders** (heightfield + cylinders +
  AABBs), no physics engine. MMO servers need cheap, deterministic character
  collision; a rigid-body engine adds cost and nondeterminism for no gain.
- **Immediate-mode UI.** Menus/HUD are rebuilt per frame from state — no
  retained widget tree to desync. Animation state (hover glow etc.) lives in
  a small id-keyed map inside the UI context.
- **Procedural placeholder assets.** Keeps iteration instant and the repo
  tiny while systems mature; art passes later replace generators behind the
  same interfaces.
- **Settings & saves in the OS user-data dir** (`%APPDATA%/Zion` on Windows,
  `~/Library/Application Support/Zion` on macOS), never next to the exe.
- **Renderer statistics are always on** (draw calls, culled counts, frame
  ms) and surfaced in the F3 overlay — performance visibility from day one.

## Conventions

- One system per directory; headers expose a struct + free functions or a
  small class. No file should grow past ~500 lines — split it.
- Tuning constants live at the top of the file that uses them, commented.
- `float` seconds everywhere; world units are meters; +Y is up.
- Comment *why*, not *what*. Complex math gets a short derivation note.
- Every feature must work in both cameras before it counts as done.

## Smoke testing

The exe supports `--frames N` (run N frames then exit with a status line),
`--screenshot PATH` (capture near the end of the run), `--width/--height`,
and `--autoplay` (scripted movement for soak tests). CI-friendly; used
constantly during development to verify builds render correctly.
