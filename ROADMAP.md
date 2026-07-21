# ZION — Roadmap

Work happens in phases. Each phase ends with a playable, tested, optimized,
polished build before the next begins. A phase may span many sessions.

For the exhaustive sandbox-MMO feature/parity matrix, build gates, and UI
delivery order, read **MASTER_ROADMAP.md**. For the PS2-era mythic art, UI,
FX, animation, and palette rules, read **VISUAL_DIRECTION.md**. This file is
the compact phase view.

For the versioned, researched, literal feature checklist and FP/TP acceptance
contract, read **ALBION_PARITY_CHECKLIST.md**. It is authoritative for system
coverage; this file remains the compact delivery sequence.

> **2026-07-20 direction update:** the world is a graph of HUGE zones
> (1280 m+ per side), each zone an isolated simulation island — the same
> shape Albion uses (one server instance per zone, hard border transitions).
> Zone infrastructure is pulled forward ahead of combat, and the detailed
> execution plan now lives in **CODEX_HANDOFF.md** (epics A–H). That file is
> the authoritative backlog; this roadmap stays the high-level phase view.

## Phase 1 — Foundation  ◀ CURRENT

Goal: an exceptionally polished single-player vertical slice that proves the
engine foundation. No multiplayer, no economy, no crafting yet.

Deliverables:
- Project architecture + documentation (this repo layout)
- Rendering framework: stylized lighting, sky, fog, post-processing, culling
- Game loop: fixed-timestep simulation + interpolated rendering
- Input system: action-based, rebindable
- Settings system: graphics / audio / controls, persisted per-user
- Save system: slot-based JSON saves
- Asset pipeline: procedural-first (geometry, sounds, textures generated in
  code); file-based assets can layer in later without pipeline changes
- Camera system: first-person + third-person, smooth transitions, both
  first-class
- Polished character movement (accel/friction model, coyote time, jump
  buffering, sprint, ground snapping)
- Interaction framework (world interactables, prompts, events)
- UI framework (immediate-mode widget kit) + menus + HUD
- Audio framework (buses, synthesized SFX, ambience)
- Particle framework (pooled, budgeted, scalable)
- Lighting framework (sun + hemisphere ambient + point lights + height fog)
- Biome-aware day/night presentation foundation and visible world clock
- Optimization framework (frustum/distance culling, render scale, debug
  overlay with timings, graphics quality options)
- A small handcrafted-feeling zone ("Elder Vale") that validates all of the
  above: terrain, ancient ruins, trees, shrines, collectible wisps

## Phase 2 — Combat & Creatures

Ability system (cast/channel/instant, cooldowns), weapon archetypes with
distinct movesets, hit detection, damage/health, creature AI (idle/aggro/
attack/leash), death/respawn, combat VFX/SFX payoffs, target UI. Both
cameras validated for combat.

## Phase 3 — Gathering & Progression Core

Resource nodes (wood/ore/stone/fiber/hide/fish) with tiered spawns, gathering
tools, destruction payoffs, inventory + equipment data model, the classless
"use it to level it" progression board (original design), item tiers and
quality.

## Phase 4 — Crafting & Economy Foundations

Refining chains, crafting stations, recipes, item stats derived from
materials, local storage/banks, NPC vendors as placeholder economy.

## Phase 5 — World at Scale

Zone streaming, multiple biomes, dungeons (entrance → instanced layout),
mounts, fast-travel rules, authoritative day/night persistence/gameplay hooks,
weather.

## Phase 6 — Multiplayer Foundation

Authoritative server, client prediction/reconciliation, interest management,
persistence service, accounts. (Architecture from Phase 1 keeps simulation
deterministic and separated from presentation to make this tractable.)

## Phase 7 — MMO Systems

Parties, guilds, chat, marketplaces/auction houses, player trade, PvP
flagging and zone tiers (safe → full-loot), territory.

## Phase 8+ — Depth & Live Content

Farming, housing, fishing, transportation/logistics gameplay, achievements,
seasonal events, console/mobile port work.

## Rules

- Never start a phase before the previous one is playable, tested, and
  polished.
- Read GOALS.md, ARCHITECTURE.md, TASKS.md before new work; never recreate
  completed systems.
- Refactors are welcome when justified; document them in ARCHITECTURE.md.
