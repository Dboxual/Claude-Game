# TacMove — Tactical FPS Movement Prototype (Milestone 1)

A from-scratch C++20 engine prototype focused on tactical-FPS movement feel.
No game engine — just **SDL3** (window/input), **OpenGL 3.3 core** (rendering,
via a hand-rolled loader on top of `SDL_GL_GetProcAddress`), **GLM** (math),
**CMake** + **vcpkg** (build/dependencies).

This build is the **early sandbox foundation (M3, community-server
direction)**: on top of the movement prototype you can create and load local
worlds, spawn objects from a categorized dev menu, pick up and use weapons
(fists / karambit / Glock / sword), carry light props, and fight a
respawning training dummy — with **3D first-person viewmodels** (animated
hands + held weapon rendered in perspective, not flat sprites), a full set
of original synthesized sounds (see
[server/content/sounds/CREDITS.md](server/content/sounds/CREDITS.md)), and
weapon & entity definitions loaded from data files under `server/content/`.
Real multiplayer, content packs, and plugins are later phases — what an
online server list would take is written down in
[ONLINE_SERVERS.md](ONLINE_SERVERS.md).

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
| E | Pick up the item you are looking at; carry/drop light props |
| Left mouse | Attack with the equipped weapon |
| 1 2 3 4 | Switch weapon slot |
| Esc | In game: pause/resume. In menus: go back a screen |
| F5 | Hot-reload `config/movement.cfg` |
| F1 | Toggle debug HUD (saved to settings) |
| — | CLI flags: `--frames N` (exit after N frames, smoke test), `--novsync`, `--renderer gl\|vulkan\|metal\|gles`, `--world` (skip menu, straight into the test world), `--paused` (start in-world, paused), `--equip <weapon id>` (start in-world holding that weapon — viewmodel work) |

## Main menu, pause menu & settings

The client boots to a **main menu** (over a slow orbit of the arena):
**Singleplayer** (Start Test World, Create World, Load World),
**Multiplayer** (a CS-style server-browser shell with **Online / Favorites /
LAN / History / Direct Connect** tabs — the Online tab states plainly that
an online list requires the master server backend described in
[ONLINE_SERVERS.md](ONLINE_SERVERS.md), the other lists are honestly empty
until server discovery exists, and Direct Connect's address box reports
that the connection system is a later milestone; nothing is faked),
**Settings**, and **Quit**. Every button press plays a small original UI
click.

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

**Weapons.** You start with fists. Look at a karambit, Glock, or sword
pickup and press **E** — it is added to your inventory (bar at the bottom
of the screen), auto-equipped, and vanishes from the world until it
respawns. Switch with **1/2/3/4**, attack with **left mouse**. Fists,
karambit, and the fantasy sword are melee (the sword trades the slowest
swing for the longest reach); the Glock is a semi-auto hitscan pistol
(unlimited ammo for now). The **training dummy** has 100 HP, flashes red on
hits, shows its health when you aim at it, and respawns 5 seconds after
being destroyed.

**First-person viewmodels.** The held weapon is drawn as a real 3D model in
the player's hands — lit, oriented geometry in camera space with its own
fixed-FOV projection (so world FOV changes never distort it) and a cleared
depth buffer (so it never clips into walls). Every weapon is animated:
fists punch, the Glock kicks with an emissive muzzle flash, the karambit
slashes across the screen, and the sword makes a diagonal cut. On top of
that: walk bob locked to the footstep cadence, a slow idle breath, and a
raise animation when switching weapons. Still no art pipeline — every shape
is composed from boxes by `Game::appendViewmodelDraws` — but the weapons
read as items held in-hand, not flat sprites.

**Props.** Crates are light enough to carry: press **E** to pick one up, it
floats in front of you (and stops blocking movement), **E** again drops it
and it settles onto whatever is below. Barrels are obviously too heavy —
`carryable` is a plain boolean in the entity definition, not a weight
system. Props never respawn: destroyed or consumed means gone.

**Sound.** Footsteps, melee swings, melee hits, gunshots, bullet impacts,
pickups, and UI clicks play through SDL3 at your master/SFX volume
settings. Every sound is **original** — pure sine/noise synthesis by
[tools/generate_sounds.cpp](tools/generate_sounds.cpp) (no recordings, no
third-party samples, no game rips), reproducible by re-running the tool —
and every file is listed with its source and license in
[server/content/sounds/CREDITS.md](server/content/sounds/CREDITS.md).
Sounds without a provably safe license are rejected.

## Content platform (server/)

Weapon and entity definitions live in **data files**, not C++:

```
server/
├── server.cfg               # dedicated server config
└── content/
    ├── weapons/*.cfg        # id, kind, damage, range, cooldown
    ├── entities/*.cfg       # size, color, solid, carryable, weapon,
    │                        #   max_health, respawn_seconds, visual parts
    ├── items/               # reserved (empty)
    ├── maps/                # reserved (worlds are still code-built)
    └── sounds/              # original synthesized WAVs + CREDITS.md (licenses)
```

The client loads this folder at startup; a file with the same id as a C++
builtin replaces it (edit `server/content/weapons/glock.cfg`, restart, and
the pistol hits differently). The builtins in `src/shared/content.cpp` are
a fallback so the game runs even without the folder. World saves reference
string ids only, so retuning content never breaks worlds. See
[server/README.md](server/README.md) for the format and what is still
hard-coded (map geometry, the sound-event mapping).

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
├── server/                      # content platform: server.cfg + content/ (see server/README.md)
│   └── content/                 # weapons/, entities/, sounds/, items/, maps/
├── worlds/<name>/world.cfg      # sandbox world saves (created in game, gitignored)
├── src/
│   ├── shared/                  # compiled into BOTH client and server (std + glm only)
│   │   ├── player.{h,cpp}       # movement controller (accel/friction/air/jump/crouch)
│   │   ├── world.{h,cpp}        # AABB world + entities: raycasts, damage, respawns
│   │   ├── content.{h,cpp}      # registry + builtin fallback defs
│   │   ├── content_loader.{h,cpp} # parses server/content weapon/entity files
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
├── tools/generate_sounds.cpp    # standalone generator for every shipped sound (CC0)
├── run_windows.bat              # one-click build + run client (Windows)
├── run_linux.sh                 # build + run client (Linux)
├── run_macos.sh                 # build + run client (macOS)
├── CMakeLists.txt               # targets: tac_shared, tac_client_*, tacmove, tacmove_server
├── CMakePresets.json
├── PORTING.md                   # how future platforms slot in
├── ONLINE_SERVERS.md            # what a real online server list needs (not built)
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
tacmove_server --config path/to/server.cfg  # default: server/server.cfg
tacmove_server --ticks 384                  # exit after N ticks (smoke test)
```

The server runs without graphics, prints timestamped console logs (startup
banner, 10-second status lines, clean Ctrl+C shutdown), and reads
[server/server.cfg](server/server.cfg): `server_name`, `max_players`,
`tick_rate` (the old `config/server.cfg` location still works as a
fallback). It builds the same shared world and movement constants as the
client, so server-side player simulation drops in when networking lands
(`src/shared/protocol.h` reserves the version handshake).

**Where the ecosystem grows from here:** custom items, weapons, and props
are already content definitions under `server/content/` consumed through
the shared registry; custom maps become data files loaded by `shared/world`
(both sides load identical geometry — still code-built today); game modes
and mods become more content kinds in the same layout; community servers
configure themselves through `server/server.cfg`. The server does not read
the content folder yet — that lands with server-side simulation.

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
