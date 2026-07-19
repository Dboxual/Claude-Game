# ZION — Ashes of the First Gods

An original 3D mythological sandbox action-RPG prototype inspired by the depth and player freedom of classless MMOs and the bold visual direction of PS2-era adventures.

Project direction and the phased roadmap are in [GOALS.md](GOALS.md). The
current technical, visual, architecture, and performance baseline is recorded
in [PHASE1_AUDIT.md](PHASE1_AUDIT.md). Native-resolution graphics behavior and
current benchmark evidence are in [PHASE2_REPORT.md](PHASE2_REPORT.md).

## Current vertical slice

The playable browser build now opens in **The Ruins of Gloamwood**, a compact authored action-RPG route containing:

- A sculpted rolling terrain mesh with distinct path, moss, soil, wet bank, and elevated regions
- Forest threshold, stream bridge, ruined courtyard, raider camp, temple entrance, boss arena, and distant watch-spire
- Original procedural trees with branching silhouettes, exposed roots, living/dead variants, and grouped undergrowth
- An articulated Wayfarer with armor, clothing, limbs, weapon, locomotion, and attack poses
- Gloamwood Raiders, Hollow Guardians, and the oversized Hollow Colossus boss
- Restrained magic lighting, warm directional sun, cool forest shadows, distance fog, firelight, and particles
- Physical merging XP orbs, rarity-aware loot, varied synthesized feedback, and surface footsteps
- Static geometry batching and constrained shadow/particle budgets for browser performance
- Instanced vegetation, contact shadows, and combat particles
- Profiled Chromium combat performance averaging above 40 FPS in the software-rendered validation environment

## Run

The 3D module must be served over HTTP:

```bash
python3 -m http.server 8080
```

Then visit `http://localhost:8080`.

## Controls

- WASD: player-relative movement
- Mouse: orbit camera; wheel: zoom
- Click: attack; Q: lock-on targeting
- V: switch third-person / first-person
- E: gather nearby resource
- 1 / 2 / 4: abilities
- I / C / P / F / G: inventory, crafting, mastery, factions, guild
- Escape: close menu

## Graphics and resolution

The gear button opens explicit Low, Medium, High, and Custom graphics settings.
All presets default to 100% render scale and dynamic resolution is off by
default. Manual scale choices are 50%, 67%, 75%, 85%, and 100%; reduced values
are labeled as less sharp. The bottom-left HUD and Graphics panel show the
drawing-buffer resolution, preset, renderer details, and dynamic multiplier.

The internal drawing buffer is calculated as:

```text
CSS canvas size × device pixel ratio × manual render scale × dynamic scale
```

The result is clamped only if it would exceed the WebGL maximum texture size,
and that clamp is reported in diagnostics. HTML/CSS interface elements remain
at browser resolution when the 3D drawing buffer is reduced.

## Repeatable benchmark

Serve the project, then in another terminal run:

```bash
cd tools
node zion-benchmark.mjs
```

This runs the fixed Gloamwood benchmark at 1280×720 Low, 1600×900 Medium,
1920×1080 High, and 1920×1080 Low, all at 100% with dynamic resolution off.
Results and screenshots are written to `captures/phase2-native-resolution/`.

The default benchmark deliberately requests SwiftShader. To request a physical
GPU in an environment with a working graphical session and browser GPU access:

```bash
cd tools
ZION_RENDERER=hardware ZION_HEADLESS=0 node zion-benchmark.mjs
```

Check `diagnostics.rendererClass`, `unmaskedRenderer`, and `swiftShader` in the
result. A requested hardware run is not a physical-GPU result unless those
reported values identify hardware and `swiftShader` is false. Install the
Playwright browser dependencies for the host OS if Chromium cannot launch.

## Prototype systems

- True 3D world with third-person shoulder camera and optional first-person mode
- Restrained emissive magic, directional lighting, atmospheric particles, and animated weapon feedback
- Mythic melee, shields, staves and magic only—no guns
- Gathering nodes with respawn timers
- Inventory, equipment paper doll, rarity and stats
- Refining and crafting recipes
- Equipment-style ability bar, energy and cooldowns
- PvE enemies, loot, health, death and respawn
- Renown levels and action-based masteries
- Quest threads with staged rewards
- Three divine factions
- Player-created warbands (guilds)

## Product direction

This prototype uses original worldbuilding and interface assets. It studies broad design patterns from sandbox MMOs without reproducing another game's copyrighted art, text, maps, names, or exact UI.
