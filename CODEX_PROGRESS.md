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

- `d11549f Polish MiniCS combat feel`
- `43de102 MiniCS vertical slice + neon identity + Codex handoff`
- `37a22de Add NEXT_STEPS_FOR_CHATGPT review notes`
- `05d7928 Restructure to game-mode-first hub; add MiniCS as first playable mode`
- `1571c12 Add world type templates: flat, duel, aim, movement, social`

## Latest validation

- `.\run_windows.bat --mode minics --frames 180`
- All modes: `minics`, `deathmatch`, `sandbox`, `zombies`, `parkour`, `social`
  with `--frames 120`
- `build\Release\tacmove_server.exe --ticks 384`
- Screenshots:
  - `build\codex_baseline_gamemodes.bmp`
  - `build\codex_baseline_minics_pressure.bmp`

## Known bugs

- Offscreen/background frame rate can vary when SDL throttles unfocused windows;
  use `--frames` for deterministic smoke tests and screenshots.
- Non-MiniCS modes are still Early or sandbox-focused and should not drive the
  near-term scope.
- Menu/game-mode cards can visually overlap the bright arena backdrop lines in
  screenshots; readability needs a small visual polish pass.
- MiniCS enemy fire is still hitscan with distance/movement accuracy rather than
  projectile leading or suppression.

## What still feels bad

- MiniCS is playable but still box-art: enemies, weapon, arena, and props need
  more authored shape language.
- Enemy movement has cheap lane routing, not true tactical pathfinding or cover
  selection.
- MiniCS needs better round-end summary and more satisfying hit/kill particles.
- The menu backdrop is more alive than before, but card readability and visual
  hierarchy still feel prototype-like.
- Shared `game.cpp` is large; avoid broad rewrites, but keep tuning constants
  organized as changes continue.

## What should happen next

Milestone 2 should make the MiniCS launch path more obvious and reliable
without adding new menu depth: clarify docs/flags if needed, improve menu card
readability, and keep direct `--mode minics` launch validated.

## Do you need to test now?

Not yet. Current state builds and smoke-tests cleanly. Ask for human feel testing
after 3-5 meaningful commits or after the next gameplay/visual pass changes how
MiniCS feels in hand.
