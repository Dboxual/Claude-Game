# ZION

An original MMORPG in early development. Native desktop game (Windows +
macOS), C++20 + raylib, no browser tech. Currently in **Phase 1:
Foundation** — a polished single-player vertical slice ("Elder Vale")
proving the engine: dual first/third-person cameras, stylized rendering,
fixed-timestep simulation, settings/saves, UI, audio, particles.

Read first: [GOALS.md](GOALS.md) · [ROADMAP.md](ROADMAP.md) ·
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

## Where files live

Settings, saves, and logs go to the per-user data dir:
`%APPDATA%\Zion` (Windows) / `~/Library/Application Support/Zion` (macOS).
