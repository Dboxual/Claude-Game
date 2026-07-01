# TacMove — Tactical FPS Movement Prototype (Milestone 1)

A from-scratch C++20 engine prototype focused on tactical-FPS movement feel.
No game engine — just **SDL3** (window/input), **OpenGL 3.3 core** (rendering,
via a hand-rolled loader on top of `SDL_GL_GetProcAddress`), **GLM** (math),
**CMake** + **vcpkg** (build/dependencies).

## What's in Milestone 1

- First-person camera with mouse look (per-frame look, fixed 128 Hz simulation with render interpolation)
- WASD movement with ground acceleration and friction
- **Counter-strafing**: tapping the opposite key decelerates you far faster than friction — this falls out of the classic `addSpeed = wishSpeed − dot(vel, wishDir)` acceleration model
- Air acceleration with **limited air control** (wish speed capped in air)
- Jumping (with a small jump-input buffer; optional auto-bhop in config)
- Crouching (with headroom check before standing back up)
- Walking (hold Shift — slower, no sprint by design)
- Axis-aligned box collision (floor/walls/boxes) with wall sliding
- Test arena: checkered floor, crate row, jump staircase, crouch tunnel, strafe pillars
- Debug HUD: speed, velocity, position, grounded state, movement mode, FPS + a big speedometer under the crosshair (turns green when you exceed run speed via air-strafing)
- Every movement constant lives in [config/movement.cfg](config/movement.cfg) — **press F5 in game to hot-reload it**

## Run it — Windows (one click)

Prerequisites (one-time):

| Tool | Install |
|---|---|
| Git | `winget install Git.Git` |
| CMake ≥ 3.21 | `winget install Kitware.CMake` |
| Visual Studio 2022 Build Tools + C++ workload | `winget install Microsoft.VisualStudio.2022.BuildTools --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"` |

Then **double-click `run_windows.bat`** (or run it from a terminal).

The first run clones vcpkg into `%USERPROFILE%\vcpkg` and compiles SDL3 from
source — give it a few minutes. Every later run configures, builds, and starts
the game in seconds.

## Run it — Linux

```bash
# Debian/Ubuntu system packages SDL3 needs to build:
sudo apt-get install -y build-essential git cmake pkg-config \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev \
  libxfixes-dev libxss-dev libxkbcommon-dev libwayland-dev \
  wayland-protocols libegl1-mesa-dev libgl1-mesa-dev libdrm-dev \
  libgbm-dev libasound2-dev libpulse-dev libudev-dev libdbus-1-dev \
  libibus-1.0-dev

./run_linux.sh
```

The first run clones vcpkg into `~/vcpkg`. If you already have vcpkg, set
`VCPKG_ROOT` and both scripts will use it.

## Controls

| Input | Action |
|---|---|
| Mouse | Look |
| W A S D | Move |
| Space | Jump (buffered — a slightly early press still counts) |
| Left Ctrl or C | Crouch (hold) |
| Left Shift | Walk (hold) |
| F5 | Hot-reload `config/movement.cfg` |
| F1 | Toggle debug HUD |
| Esc | Release/capture the mouse (click to recapture) |
| — | `--frames N` CLI flag: exit after N frames (smoke test); `--novsync` disables vsync |

## Tuning the movement

Everything is in [config/movement.cfg](config/movement.cfg) — speeds,
acceleration, friction, air control cap, gravity, jump velocity, body
dimensions, sensitivity, FOV. Edit it while the game runs and press **F5**.
Only `tick_rate` needs a restart.

Things to try:

- Raise `air_speed_cap` to 30 for full Quake-style air-strafing.
- Set `auto_bhop = 1` and hold Space to chain jumps without losing speed to friction.
- Counter-strafe drill: run along the checkered floor with D, then tap A —
  watch the speedometer snap to ~0 far faster than releasing keys does.

## Project layout

```
├── config/movement.cfg      # all tunable movement constants (F5 hot-reload)
├── src/
│   ├── main.cpp             # game loop: fixed-tick sim + interpolated render
│   ├── player.{h,cpp}       # movement controller (accel/friction/air/jump/crouch)
│   ├── world.{h,cpp}        # AABB collision world + test map
│   ├── renderer.{h,cpp}     # GL 3.3 renderer, embedded shaders, cube batch
│   ├── debug_text.{h,cpp}   # HUD text, embedded 5x7 bitmap font
│   ├── config.{h,cpp}       # key=value config parser + movement constants
│   └── gl_loader.{h,cpp}    # hand-rolled GL function loader (no glad/glew)
├── run_windows.bat          # one-click build + run (Windows)
├── run_linux.sh             # build + run (Linux)
├── CMakeLists.txt
├── vcpkg.json               # dependencies: sdl3, glm
└── .github/workflows/build.yml  # CI: Windows x64, Linux x64, macOS arm64
```

## Building by hand

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release --parallel
# Windows: build\Release\tacmove.exe   Linux/macOS: ./build/tacmove
```

## Design notes

- **Units** are meters and seconds; gravity is 20 m/s² (games feel floaty at 9.81).
- **Fixed tick** (default 128 Hz) keeps movement physics frame-rate independent;
  the camera interpolates between the last two ticks, and mouse look is applied
  per frame so aiming stays latency-free.
- **Collision** is axis-separated AABB clamping (X, Z, then Y) with a 1 mm skin,
  which gives free wall-sliding. There is no step-up yet, so all walkable
  surfaces are flat — elevation changes go through jumps.
- **Crouching in air** currently shrinks the box from the feet (no mid-air
  "tuck"); crouch-jumping onto ledges is a later milestone.
- Known scope cuts for M1: no ramps/stairs, no networking, no audio, no menus.

## CI

`.github/workflows/build.yml` builds Windows x64 (MSVC), Linux x64 (GCC), and
macOS arm64 (AppleClang, OpenGL 3.3 core still works there) on every push and
uploads the binaries + config as artifacts.
