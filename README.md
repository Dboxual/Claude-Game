# ZION — Ashes of the First Gods

An original 3D mythological sandbox action-RPG prototype inspired by the depth and player freedom of classless MMOs and the bold visual direction of PS2-era adventures.

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
