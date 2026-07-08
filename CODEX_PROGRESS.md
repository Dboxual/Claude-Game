# CODEX_PROGRESS - TacMove / MiniCS

Last updated: 2026-07-08

## Current goal

Make TacMove feel less like a blueprint and more like a real playable neon
social/action shooter, with MiniCS as the first polished vertical slice.

Stay focused on MiniCS feel, visual identity, readability, feedback, and safe
optimization. Do not add multiplayer, new game modes, huge menus, procedural
worlds, renderer rewrites, or VRChat-scale social systems yet.

## Completed milestones

- Baseline takeover: pulled GitHub state with `git pull --ff-only` and confirmed
  the branch was already up to date.
- Baseline validation: built with the documented Windows path and smoke-tested
  MiniCS, all six modes, and the dedicated server.
- Milestone 1: created this progress ledger and cleaned stale continuation docs
  so future Codex runs do not chase already-completed MiniCS work.
- Milestone 2: added a real `PLAY MINICS` quick-launch button on the main menu,
  added a direct MiniCS entry to the Play submenu, and made menu/card surfaces
  more opaque for readability over the neon arena backdrop.
- Milestone 3: strengthened MiniCS visual identity with authored LED arena
  panels/trims, render-only animated neon sweeps/equalizers, stronger dusk fog,
  neon-accented enemies, and clearer MiniCS HUD panels/ammo pips.
- Recent MiniCS combat pass: lane-pressure bots, headshots, ADS, faster reload,
  hidden MiniCS enemy health clutter, death silhouettes, extra neon pillars, and
  animated lane pulse.

## Current branch

- `wip-m3-content-platform`
- Remote: `origin https://github.com/Dboxual/Claude-Game.git`

## How to build

- Windows documented path: `run_windows.bat`
- Direct Windows build: `cmake --preset desktop && cmake --build build --config Release`
- Current output: `build/Release/tacmove.exe` and `build/Release/tacmove_server.exe`

## How to run

- Main menu: `build/Release/tacmove.exe`
- Smoke test fixed frames: `build/Release/tacmove.exe --frames 120`
- Dedicated server smoke: `build/Release/tacmove_server.exe --ticks 384`

## How to launch MiniCS

- From menu: `PLAY -> GAME MODES -> MINICS`
- Direct dev launch: `build/Release/tacmove.exe --mode minics`
- Deterministic screenshot example:
  `build/Release/tacmove.exe --mode minics --frames 520 --screenshot build/codex_baseline_minics_pressure.bmp`

## Latest commits

- Improve MiniCS neon identity (current commit)
- `f12cfe1 Make MiniCS launch obvious`
- `6456726 Add Codex progress ledger`
- `d11549f Polish MiniCS combat feel`
- `43de102 MiniCS vertical slice + neon identity + Codex handoff`

## Latest validation

- `.\run_windows.bat --mode minics --frames 180`
- All modes: `minics`, `deathmatch`, `sandbox`, `zombies`, `parkour`, `social`
  with `--frames 120`
- `build\Release\tacmove_server.exe --ticks 384`
- `cmake --build build --config Release --parallel`
- `build\Release\tacmove.exe --mode minics --frames 90 --screenshot build\codex_m3_minics_countdown.bmp`
- `build\Release\tacmove.exe --mode minics --frames 360 --screenshot build\codex_m3_minics_live.bmp`
- `build\Release\tacmove.exe --frames 90 --screenshot build\codex_m3_mainmenu.bmp`
- `.\run_windows.bat --mode minics --frames 180`
- `build\Release\tacmove.exe --mode deathmatch --frames 120`
- `build\Release\tacmove.exe --mode sandbox --frames 120`
- Screenshots:
  - `build\codex_baseline_gamemodes.bmp`
  - `build\codex_baseline_minics_pressure.bmp`
  - `build\codex_m2_mainmenu.bmp`
  - `build\codex_m2_play.bmp`
  - `build\codex_m2_gamemodes_final.bmp`
  - `build\codex_m3_minics_countdown.bmp`
  - `build\codex_m3_minics_live.bmp`
  - `build\codex_m3_mainmenu.bmp`

## Known bugs

- Offscreen/background frame rate can vary when SDL throttles unfocused windows;
  use `--frames` for deterministic smoke tests and screenshots.
- Non-MiniCS modes are still Early or sandbox-focused and should not drive the
  near-term scope.
- The menu backdrop is intentionally active; keep checking screenshots when
  changing menu/card opacity or camera angles.
- MiniCS enemy fire is still hitscan with distance/movement accuracy rather than
  projectile leading or suppression.
- The latest arena polish adds more emissive geometry; continue checking
  screenshots when changing fog, menu opacity, or HUD placement.

## What still feels bad

- MiniCS is more visually cohesive, but it is still box-art: the weapon,
  enemies, cover props, and round-end screen need more authored shape language.
- Enemy movement has cheap lane routing, not true tactical pathfinding or cover
  selection.
- MiniCS needs better round-end summary and more satisfying hit/kill particles.
- Gun feel needs the next focused pass: recoil timing, dry fire/reload beats,
  muzzle/tracer/hit feedback, and combat pacing.
- Shared `game.cpp` is large; avoid broad rewrites, but keep tuning constants
  organized as changes continue.

## What should happen next

Checkpoint for human review: run MiniCS and judge the new neon arena/HUD
direction in motion. If it feels acceptable, continue to Milestone 4: weapon
feel, including Glock recoil, muzzle flash/tracer readability, hit/kill marker
timing, reload/dry-fire feedback, and combat pacing.

## Do you need to test now?

Yes. This reaches the requested checkpoint range after three meaningful takeover
commits, and the visual direction should be judged in motion before the weapon
feel pass builds on it.
