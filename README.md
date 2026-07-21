# ZION

An original MMORPG in early development. Native desktop game (Windows +
macOS), C++20 + raylib, no browser tech. Currently in **Phase 1:
Foundation** — a polished single-player vertical slice ("Elder Vale")
proving the engine: dual first/third-person cameras, stylized rendering,
fixed-timestep simulation, settings/saves, UI, audio, particles.

Read first: [GOALS.md](GOALS.md) · [ROADMAP.md](ROADMAP.md) ·
[MASTER_ROADMAP.md](MASTER_ROADMAP.md) ·
[ALBION_PARITY_CHECKLIST.md](ALBION_PARITY_CHECKLIST.md) ·
[VISUAL_DIRECTION.md](VISUAL_DIRECTION.md) ·
[ARCHITECTURE.md](ARCHITECTURE.md) · [TASKS.md](TASKS.md)

## Build & run

### Windows

Double-click `run_windows.bat` — it finds/installs vcpkg, configures,
builds, and launches. Or manually:

```
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=%USERPROFILE%\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release --parallel
build\Release\zion.exe
```

Requires: git, CMake ≥ 3.21, Visual Studio 2022 (or Build Tools) with the
C++ workload.

### macOS

```
./run_macos.sh
```

Requires: Xcode command-line tools, CMake, git.

## Controls (defaults, rebindable in Settings)

| Action | Input |
| --- | --- |
| Move | WASD |
| Look | Mouse |
| Jump | Space |
| Sprint | Left Shift (hold) |
| Interact | E |
| Toggle first/third person | V |
| Pause / menus | Esc |
| Debug overlay | F3 |

## Smoke-test flags

```
zion.exe --frames 300 --screenshot shot.png   # render 300 frames, capture, exit
zion.exe --autoplay                           # scripted soak: walks to shrines, collects wisps
zion.exe --autoplay --tp                      # same, starting in third person
zion.exe --width 1280 --height 720 --fullscreen
```

Plain `--frames` runs show the title screen (UI capture); `--autoplay` jumps
straight into gameplay.

Opt-in creative testing (never required by normal gameplay):

```
zion --dev --data-dir Builds/dev-data
zion --dev --time 22 --zone 4 --frames 300   # deterministic night smoke
zion --tour --frames 1600 --record-dir Builds/tour-frames
python3 tools/make_test_recording.py Builds/tour-frames Builds/tour.webp 12
```

`--tour` is a normal-mode validation run, not creative mode. It exercises the
regular Action path for WASD, Shift, Space, V, F3, E, and Esc, performs camera
look, visibly pauses/resumes, and collects rewards. `--record-dir` samples the
actual rendered back buffer at 12 fps; the helper assembles those PNGs into an
animated WebP without OS screen-recording permission.

`F1` panel, `F4` flight, `F5` turbo, `F6` reward-path test, `F7` next zone,
`F8` return to spawn, `F9` advance three hours. While flying, Space/Ctrl move
up/down. Use isolated `--data-dir` paths for AI and automated testing.

## Where files live

Settings, saves, and logs go to the per-user data dir:
`%APPDATA%\Zion` (Windows) / `~/Library/Application Support/Zion` (macOS).
Window-resolution changes apply live on Windows. On macOS the selected size is
applied on the next launch to avoid a raylib 6 Retina resize bug; the menu labels
this requirement.
