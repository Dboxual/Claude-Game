# TacMove — Tactical FPS Movement Prototype (Milestone 1)

A from-scratch C++20 engine prototype focused on tactical-FPS movement feel.
No game engine — just **SDL3** (window/input), **OpenGL 3.3 core** (rendering,
via a hand-rolled loader on top of `SDL_GL_GetProcAddress`), **GLM** (math),
**CMake** + **vcpkg** (build/dependencies).

This build is the **early sandbox foundation (Phase 2)**: on top of the
movement prototype you can create and load local worlds, spawn objects from
a categorized dev menu, pick up and use weapons (fists / karambit / Glock),
and fight a respawning training dummy. Multiplayer, content packs, and
plugins are later phases.

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
# Debian/Ubuntu system packages the vcpkg dependency build needs:
sudo apt-get install -y build-essential git cmake pkg-config \
  autoconf autoconf-archive automake libtool libltdl-dev \
  libx11-dev libxft-dev libxrender-dev libxext-dev libxrandr-dev \
  libxcursor-dev libxi-dev libxfixes-dev libxss-dev libxtst-dev \
  libxkbcommon-dev libxkbcommon-x11-dev libwayland-dev wayland-protocols \
  libdecor-0-dev libegl1-mesa-dev libgl1-mesa-dev libdrm-dev libgbm-dev \
  libasound2-dev libpulse-dev libpipewire-0.3-dev libudev-dev \
  libdbus-1-dev libibus-1.0-dev

./run_linux.sh
```

The first run clones vcpkg into `~/vcpkg`. If you already have vcpkg, set
`VCPKG_ROOT` and the scripts will use it.

## Run it — macOS (Apple Silicon or Intel)

Prerequisite (one-time): the Xcode Command Line Tools —

```bash
xcode-select --install
```

Then:

```bash
./run_macos.sh
```

The first run clones vcpkg into `~/vcpkg` (set `VCPKG_ROOT` to reuse an
existing install) and compiles SDL3 from source — give it a few minutes.
Every later run configures, builds, and starts the game in seconds. Homebrew
is not required: if `cmake` is not on your PATH, the script fetches a private
copy through vcpkg (a Homebrew cmake gets picked up automatically if you have
one).

macOS uses the same OpenGL 3.3 core renderer as Windows/Linux (Apple
deprecated OpenGL but still ships it; a Metal backend is a later milestone —
see [PORTING.md](PORTING.md)).

## Controls

| Input | Action |
|---|---|
| Mouse | Look |
| W A S D | Move |
| Space | Jump (buffered — a slightly early press still counts) |
| Left Ctrl or C | Crouch (hold) |
| Left Shift | Walk (hold) |
| Q | Toggle the dev spawn menu (in game) |
| E | Pick up the item you are looking at |
| Left mouse | Attack with the equipped weapon |
| 1 2 3 | Switch weapon slot |
| Esc | In game: pause/resume. In menus: go back a screen |
| F5 | Hot-reload `config/movement.cfg` |
| F1 | Toggle debug HUD (saved to settings) |
| — | CLI flags: `--frames N` (exit after N frames, smoke test), `--novsync`, `--renderer gl\|vulkan\|metal\|gles`, `--world` (skip menu, straight into the test world), `--paused` (start in-world, paused) |

## Main menu, pause menu & settings

The client boots to a **main menu** (over a slow orbit of the arena):
**Singleplayer** (Start Test World, Create World, Load World),
**Multiplayer** (server-address field with typing, Connect and Localhost
quick-connect buttons — both report that the connection system is a later
milestone; Server Browser is a disabled coming-soon slot), **Settings**,
and **Quit**.

In game, Esc pauses: the simulation freezes, the mouse unlocks, and the pause
menu offers **Resume / Settings / Quit to Menu**. The settings screen is
shared between the main menu and the pause menu: mouse sensitivity, FOV,
master/music/SFX volume (stored now, audible once sounds exist), fullscreen,
vsync, debug HUD, reset-to-defaults, and back.

Settings persist to `config/settings.cfg` — created with defaults on first
run, saved whenever you change something in the menu, loaded on startup, and
safe to hand-edit (same `key = value` format as the movement config). Mouse
sensitivity and FOV live there now rather than in `movement.cfg`, since they
are player preferences rather than physics.

## Sandbox worlds, weapons & dev spawn menu (Phase 2)

**Create World** asks for a name and drops you into a fresh, enclosed flat
map. Each world lives in its own folder, `worlds/<folder_name>/world.cfg` —
a plain-text file (`key = value`, one `entity` line per placed object) that
is safe to hand-edit and diff:

```
version = 2
name = My World
entity = crate 3.00 0.00 5.00 0
entity = glock 1.50 0.00 3.00 10
```

The last number is **respawn seconds**: pickups/bots come back that long
after being taken or destroyed; 0 means gone for good (v1 files without the
field still load). **Load World** lists every world under `worlds/` and
reopens it with all placed objects. The classic **test arena** is still
there via Start Test World — it is a fixed training map and is never saved.

In game, **Q** opens the dev/admin spawn menu (GMod-style category tabs:
**Weapons / Bots / Props / Pickups**). Objects spawn a couple of meters in
front of you; crates and dummies are solid and block movement, weapon
pickups hover, bob, and are non-solid. Persistent worlds save automatically
on every spawn and on Quit to Menu.

**Weapons.** You start with fists. Look at a karambit or Glock pickup and
press **E** — it is added to your inventory (bar at the bottom of the
screen), auto-equipped, and vanishes from the world until it respawns.
Switch with **1/2/3**, attack with **left mouse**. Fists and karambit are
melee (short reach, swing cooldown); the Glock is a semi-auto hitscan
pistol (unlimited ammo for now). The **training dummy** has 100 HP, flashes
red on hits, shows its health when you aim at it, and respawns 5 seconds
after being destroyed.

All of it is data in `src/shared/content.cpp`: `WeaponDef` holds combat
stats (damage/range/cooldown — deliberately knife-tier numbers, skins stay
cosmetic), `EntityDef` holds world objects (size, solidity, health, respawn,
multi-box placeholder visuals). World files reference string ids only, so
definitions can move from C++ to data-driven content files in a later phase
without touching saves.

## Tuning the movement

Everything physics-related is in [config/movement.cfg](config/movement.cfg) —
speeds, acceleration, friction, air control cap, gravity, jump velocity, body
dimensions. Edit it while the game runs and press **F5**. Only `tick_rate`
needs a restart. Jump apex height is `jump_speed² / (2 × gravity)`; the
default 7.2 with gravity 20 gives ≈1.3 m.

Things to try:

- Raise `air_speed_cap` to 30 for full Quake-style air-strafing.
- Set `auto_bhop = 1` and hold Space to chain jumps without losing speed to friction.
- Counter-strafe drill: run along the checkered floor with D, then tap A —
  watch the speedometer snap to ~0 far faster than releasing keys does.

## Architecture (platform-safe layering)

The project is split into a **game client**, a **headless dedicated server**,
and **shared code** both compile — the foundation for a community-server
platform. Each piece is a separate CMake target, so the boundaries are
enforced by the compiler: gameplay never sees SDL or OpenGL headers, and the
server (`tacmove_server`) links only `tac_shared`, so it can never silently
grow a graphics dependency. See [PORTING.md](PORTING.md) for the platform
guide.

```
├── config/movement.cfg          # movement physics tunables (F5 hot-reload, shared)
├── config/settings.cfg          # client user settings, written by the pause menu (auto-created)
├── config/server.cfg            # dedicated server config: name, max players, tick rate
├── worlds/<name>/world.cfg      # sandbox world saves (created in game, gitignored)
├── src/
│   ├── shared/                  # compiled into BOTH client and server (std + glm only)
│   │   ├── player.{h,cpp}       # movement controller (accel/friction/air/jump/crouch)
│   │   ├── world.{h,cpp}        # AABB world + entities: raycasts, damage, respawns
│   │   ├── content.{h,cpp}      # weapon stats + entity defs (karambit/glock/dummy/crate)
│   │   ├── inventory.h          # owned weapons + equipped slot (plain data)
│   │   ├── world_save.{h,cpp}   # world file format: serialize/parse (string <-> struct)
│   │   ├── config.{h,cpp}       # key=value parser + movement constants
│   │   ├── log.{h,cpp}          # timestamped logging
│   │   └── protocol.h           # future client<->server protocol (version stub only)
│   ├── client/                  # the playable prototype
│   │   ├── main.cpp             # composition root: picks platform + renderer, pumps loop
│   │   ├── game.{h,cpp}         # fixed-tick sim orchestration, HUD + pause menu, RenderFrame
│   │   ├── settings.{h,cpp}     # user settings (sensitivity/FOV/volumes/display)
│   │   ├── platform/            # interfaces: IWindow, IInput, IFileSystem, IAudio
│   │   │   └── sdl/             # SDL3 implementations (the ONLY code touching SDL)
│   │   └── renderer/            # IRenderer + GL 3.3 backend + Vulkan/Metal/GLES stubs
│   └── server/                  # headless dedicated server (no SDL, no GL)
│       ├── main.cpp             # CLI entry: --config <path>, --ticks <n>
│       └── dedicated_server.{h,cpp}  # config, fixed-tick loop, status logs
├── run_windows.bat              # one-click build + run client (Windows)
├── run_linux.sh                 # build + run client (Linux)
├── run_macos.sh                 # build + run client (macOS)
├── CMakeLists.txt               # targets: tac_shared, tac_client_*, tacmove, tacmove_server
├── CMakePresets.json
├── PORTING.md                   # how future platforms slot in
├── vcpkg.json                   # dependencies: sdl3, glm
└── .github/workflows/build.yml  # CI: Windows x64, Linux x64, macOS arm64
```

Select a client renderer backend with `--renderer gl|vulkan|metal|gles` (only
`gl` is implemented; the others are compiling stubs that fail init with a
clear message, proving the seam).

## Dedicated server

```
# from the repo root, after building:
build\Release\tacmove_server.exe            (Windows)
./build/tacmove_server                      (Linux/macOS)

# options:
tacmove_server --config path/to/server.cfg  # default: config/server.cfg
tacmove_server --ticks 384                  # exit after N ticks (smoke test)
```

The server runs without graphics, prints timestamped console logs (startup
banner, 10-second status lines, clean Ctrl+C shutdown), and reads
[config/server.cfg](config/server.cfg): `server_name`, `max_players`,
`tick_rate`. It builds the same shared world and movement constants as the
client, so server-side player simulation drops in when networking lands
(`src/shared/protocol.h` reserves the version handshake).

**Where the ecosystem grows from here:** custom maps become data files loaded
by `shared/world` (both sides load identical geometry); custom items, game
modes, and mods become content definitions in `shared/` consumed by client
and server alike; community servers configure themselves through
`server.cfg`. None of that exists yet — this milestone is only the clean
split.

## Building by hand

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release --parallel
# Client:  build\Release\tacmove.exe          (Linux/macOS: ./build/tacmove)
# Server:  build\Release\tacmove_server.exe   (Linux/macOS: ./build/tacmove_server)
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
