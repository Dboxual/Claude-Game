# ZION — CODEX HANDOFF & TRADEOFF FILE

This is the execution contract for building Zion from the current Phase 1
foundation into a full zone-based MMORPG (single-player-first, server-shaped).
Read this file, then GOALS.md, ARCHITECTURE.md, TASKS.md before writing code.
Keep CODEX_PROGRESS.md as your ledger (create it; one dated entry per work
session: what was done, what's next, any deviations from this file).

> **2026-07-21 addendum:** `ALBION_PARITY_CHECKLIST.md` is now authoritative
> for live feature coverage and FP/TP acceptance criteria. This handoff remains
> authoritative for architecture and epic sequencing. Its older current-state
> inventory is historical; check `CODEX_PROGRESS.md` and `TASKS.md` for the
> latest completed foundation work.
>
> **Latest movement/visual contract:** the TP body always faces camera-forward
> (including strafing/backpedaling), locomotion uses a planted root with
> presentation-only Y smoothing, shrine light/surfaces are blue, XP wisps are
> violet/gold, and added ground dressing must be living grass/flowers rather
> than branches, loose stones, or ambient litter. Use `--tour --record-dir` to
> validate the normal action path, including visible Escape pause/resume.

---

## 1. What Zion is (one paragraph)

An original MMORPG in the spirit of Albion Online — classless equipment
progression, gathering/refining/crafting, open-world PvE/PvP, player economy
— built as a native C++20 desktop game (Windows + macOS), PS2-era fantasy
art direction, dual first/third-person cameras (both first-class), and
"rewarding by default" feedback on every action. No copied assets, names,
maps, or content from any existing game. The world is a graph of HUGE zones;
each zone is architected as its own simulation island (exactly how Albion
runs each cluster as a separate server instance) so the later multiplayer
split is a process boundary, not a rewrite.

## 2. Current state (verified 2026-07-20)

Phase 1 foundation slice is DONE and playable at ~140 fps @ 1600x900:

- Fixed-step sim (60 Hz) + interpolated render; custom-frame-control-aware
  app loop (see gotchas — this one bites)
- Rebindable action input; settings + slot saves (JSON, `%APPDATA%/Zion`)
- One zone ("Elder Vale", 320x320 m): FBM heightfield terrain with vertex
  paint, temple/wayshrines/monoliths/trees/rocks, capsule collision,
  walkable platform discs
- FP/TP camera rig (bob, landing dip spring, sprint FOV kick, collision
  boom, blended toggle on V)
- Lighting (hemisphere + wrapped sun + 8 point lights + fog), procedural sky
  dome, bloom/grade/vignette post chain, frustum + distance culling
- Immediate-mode UI kit + title/pause/settings menus (full rebinding UI),
  HUD (prompt chips, anima counter, banners), F3 debug overlay
- 8 synthesized sounds + wind-loop stream, bus volumes
- 4096-particle pool (alpha + additive), footstep/land/shrine/trail/collect/
  mote emitters
- Interaction framework (gaze + proximity, camera-agnostic) + wisp rewards
  (burst → scatter → velocity-steered homing → rising-pitch collect chain)
- Smoke testing: `--frames N --screenshot x.png --autoplay --tp`
  (autoplay steers to shrines and validates the whole reward loop; a 2000-
  frame soak collects 9/9 wisps and triggers autosave)

File map: `src/core` (app/log/paths/clock/mathx/rng), `src/input`,
`src/settings`, `src/save`, `src/render` (lighting/sky/renderer/postfx),
`src/world` (terrain/world/interact), `src/player` (controller/camera_rig),
`src/fx` (particles), `src/audio`, `src/ui` (ui/hud), `src/game`
(game/menus). ~4k lines, 46 files.

Git: current work is uncommitted on branch `wip-m3-content-platform`; the
tree also shows deletions of the old MiniCS project. Make an initial "Zion
Phase 1 foundation" commit before starting, then commit per completed task.
DO NOT touch or drop the git stash "GODGUN prototype backup" — it is the
only copy of the previous prototype.

## 3. Build, run, test

```
run_windows.bat                      # one-click: vcpkg + cmake + build + run
cmake --build build --config Release --parallel
build\Release\zion.exe --frames 300 --screenshot shot.png   # smoke
build\Release\zion.exe --frames 2000 --autoplay             # reward-loop soak
```

Deps via vcpkg manifest: raylib, nlohmann-json. Add new deps to vcpkg.json
only with a written justification in CODEX_PROGRESS.md (see §5 tradeoffs —
default is "build it ourselves on raylib").

After EVERY task: build clean (0 warnings at /W4), run a smoke + screenshot,
and eyeball the screenshot. Extend the autoplay script whenever you add a
mechanic so soaks exercise it (this is how the wisp orbit bug was caught).

## 4. Hard rules & gotchas (violating these breaks the game)

1. **Custom frame control**: the vcpkg raylib is built with
   SUPPORT_CUSTOM_FRAME_CONTROL. `EndDrawing()` does NOT poll input, swap
   buffers, or track time. Only `core/app.cpp` may call `PollInputEvents` /
   `SwapScreenBuffer`. Never add a second loop that assumes stock raylib.
2. **Fixed timestep**: gameplay state changes happen ONLY in 60 Hz fixed
   ticks (`FixedClock`); rendering interpolates (`posPrev`/`pos` + alpha).
   Mouse look is per-frame. New gameplay systems must follow this split —
   it is the future server tick.
3. **Layering**: `game/` is the only glue. Systems never include `game/`
   and never include each other's internals (see ARCHITECTURE.md table).
   Cross-system effects flow through small event structs returned from
   updates (see `PlayerEvents`, `InteractEvents` — copy that pattern).
4. **Screenshot paths**: raylib `TakeScreenshot` breaks on absolute paths;
   main.cpp uses `LoadImageFromScreen` + `ExportImage`. Keep it.
5. **UI font is ASCII-95 only** (LoadFontEx default set). No em-dashes or
   unicode in UI strings, or you get "?".
6. **Both cameras always**: every mechanic must be tested FP and TP
   (`--tp` flag). Targeting/aim must go through the camera eye+forward like
   `InteractionSystem::FindTarget` does — never FP-only math.
7. **Perf budget**: 100+ fps on GTX 1650-class. Practically: keep typical
   draw calls under ~600 after culling, no per-frame heap allocation in hot
   paths (pool everything like `ParticleSystem`), profile with F3 overlay
   after each epic. Every new visual cost needs a graphics setting.
8. **Feedback is mandatory**: a mechanic without VFX + SFX + UI response is
   not done. Reuse the wisp/chime/banner patterns.
9. **Determinism**: world generation is pure function of (worldSeed,
   zoneId). No `rand()`; use `core/rng.h` / hash noise like terrain.cpp.
10. **No giant files**: split before ~500 lines. Tuning constants at top of
    the file that uses them, commented.
11. Windows/macOS only APIs are confined to `core/paths.cpp` and `ui/ui.cpp`
    font paths. Keep platform code out of gameplay.
12. This repo lives in OneDrive — occasional file-lock hiccups on rebuild
    are OneDrive, not you; retry before debugging.

## 5. Standing design decisions & tradeoffs (do not relitigate silently)

| Decision | Chosen | Why | Cost accepted |
| --- | --- | --- | --- |
| Engine | raylib + own systems, no Unity/Godot/physics engine | full control, deterministic, server-shaped sim, tiny footprint | we build tooling ourselves |
| Assets | procedural-first (geometry/sfx/textures in code) | instant iteration, zero pipeline, stylized look hides it | art pass later swaps generators behind same interfaces |
| Character collision | analytic capsule vs cylinders/AABBs/heightfield | cheap, deterministic, MMO-server friendly | no ragdolls/physics toys |
| Sim model | 60 Hz fixed tick + interpolation | framerate-independent, becomes the server tick verbatim | slight added complexity in every system |
| Zones | Albion-style: hard borders, each zone its own simulation island, transition via gates with a short load | maps 1:1 onto zone-per-server-process multiplayer; bounds memory; allows huge world | no seamless cross-zone sightlines; loading moment at borders |
| Zone size | HUGE: 1280x1280 m per zone (16x Elder Vale's area), 2 m cells | "huge zones, huge map" directive; still one server's worth of sim | requires terrain LOD + spatial partitioning (Epic A) |
| World map | start 3x3 zone graph, expandable grid | enough biome/tier variety to prove systems | more zones = content work, added later |
| Multiplayer | LAST (Epic H). Single-player first, but every system server-shaped | shipping playable builds continuously | some rework risk, minimized by zone-island + fixed-tick discipline |
| Data | gameplay tables (items, recipes, abilities, creatures) as JSON in `content/` loaded at boot | modding-friendly, hot-iterable, Codex-friendly | schema validation code needed |
| Saves | JSON per slot + per-zone state deltas | debuggable, forward-compatible via `.value()` defaults | not cheat-proof (fine until multiplayer) |

If you believe a decision is wrong, write the argument in CODEX_PROGRESS.md
and STOP that epic — don't fork the architecture unilaterally.

## 6. Zone architecture spec (Epic A — build this first)

Target shape:

```
src/zone/
  zone_def.h        ZoneId, ZoneDef { name, biome, tier, size, seed offset,
                    gate list (edge, position, target zone+gate) }
  zone_registry.cpp the 3x3 starter world graph (data, not code, if easy)
  zone_instance.h/.cpp   owns Terrain + World + InteractionSystem +
                    (later) creatures/nodes. Everything simulated per-zone.
  zone_manager.h/.cpp    current instance, transition state machine:
                    fade out -> unload -> generate/load target -> spawn at
                    matching gate -> fade in. Async generation if >150 ms.
```

- `Game` stops owning Terrain/World/Interact directly; it owns ZoneManager.
- Zone generation = f(worldSeed, zoneId): biome params (palette, tree/rock
  density, prop set, fog color, ambient) + tier (see §7 PvP/risk later).
- **Terrain scale-up** (the real work): 1280 m @ 2 m cells = 641x641 heights
  (heap, ~1.6 MB — fine). Mesh in 32 m chunks (40x40 grid). LOD: full-res
  mesh within ~160 m, half-res to ~400 m, quarter-res beyond; pick LOD per
  chunk per frame by camera distance (build all LOD meshes at gen time;
  memory is cheap, stalls are not). Keep skirt strips or accept tiny cracks
  (stylized look forgives them — test visually).
- **Spatial partition**: props + colliders + interactables + resource nodes
  into a uniform grid (cell 16 m). All queries (collision resolve, camera
  boom, culling, interaction search) go through it. Kill every O(all-props)
  loop currently in world.cpp.
- **Gates**: stone archways at zone edges with a glowing portal plane +
  prompt ("Travel to <Zone>"). Autoplay must be able to path to a gate and
  transition (soak the transition 20x in a row for leaks).
- **Per-zone persistence**: save file gains `zones: { "<id>": { shrines,
  node respawn timers, ... } }` deltas + `currentZone`.
- Starter world (original names, adjust freely): center **Elder Vale**
  (safe, t1, the existing zone rebuilt at new size); N **Thornwood** (forest
  t2); E **Gilded Steppe** (plains t2); S **Mirrormarsh** (wetland t3); W
  **Cinder Barrens** (badlands t3); corners: **Skyreach Highlands** (t4),
  **Sunken Necropolis** (t4, dungeon-heavy), **Verdant Maze** (t3),
  **Starfall Crater** (t4, magical).

Acceptance: walk Elder Vale → gate → Thornwood → back; different biome
look; 100+ fps in both; state persists across transitions and save/load;
20-transition soak has flat memory (log `MemAlloc` count or working set).

## 7. The full mechanics backlog (epics in build order)

Each epic: keep TASKS.md checkboxes; every mechanic data-driven where a
table is named; every mechanic has feedback (VFX+SFX+UI) and works FP+TP;
extend autoplay to exercise it; screenshot review before marking done.

### Epic A — Zone infrastructure (spec in §6)

### Epic B — Foundation polish (parallel-friendly, small tasks)
- Gamepad: full action mapping + right-stick look (sensitivity setting),
  UI navigation (dpad focus), prompt chips show pad glyphs when last input
  was pad
- Footstep material variation: sample terrain vertex paint under player →
  grass/stone/dirt sound + particle tint; dais = stone
- Player body: procedural walk-cycle (leg/arm swing from stride phase,
  lean into acceleration, air pose) — this seeds the animation framework:
  `src/anim/` pose struct + spring/phase helpers reusable by creatures
- Set dressing pass per biome: flower patches, fallen logs, path stones,
  ruined arches; all through the spatial grid
- Settings: resolution picker + windowed/borderless, apply-and-confirm
- macOS: verify run_macos.sh builds; fix font path fallback; document
- Minimap/compass strip (needed once zones are huge): N/E/S/W + gate pips
  + shrine pips; simple bar at top, no full map yet
- World map screen (M): zone graph diagram, current zone highlighted,
  discovered gates listed

### Epic C — Combat & creatures (Phase 2 of ROADMAP)
Data: `content/abilities.json`, `content/creatures.json`,
`content/weapons.json`.
- Stats core: health/energy, damage types (phys/magic), armor/resist,
  regen out of combat; damage pipeline with events (for numbers/lifesteal
  later)
- Ability system: slots Q/W/E from weapon + R from armor (original spin on
  classless), cast types instant/cast-time/channel/ground-target, cooldowns,
  energy costs, cast bar UI, ground telegraph decals (flattened cylinder
  mesh on terrain), interrupt on move for cast-time
- Weapon archetypes v1 (each = auto-attack + 3 abilities, distinct feel):
  Sword (melee arc cleave, dash strike, parry riposte), Bow (projectile
  auto, charged pierce, rain AoE, disengage leap), Fire Staff (bolt,
  ignite DoT, ground flame burst). Melee = swept-arc hit tests vs capsules;
  projectiles = pooled entities, gravity optional, trail particles
- Combat feel (mandatory): hitstop 40-70 ms on melee connect, hit flash on
  victim (emissive pulse), damage numbers (pooled, arc + fade), screenshake
  scaled by impact, weapon trails (ribbon of quads along swing), kill
  payoff = wisp burst using existing system
- Creatures v1 via `src/creatures/`: rig from primitives + anim framework;
  AI states idle/wander/aggro/attack/leash/dead; aggro radius by tier;
  loot = wisps + (Epic D) items. Starter set: Thicket Stalker (wolf-shape,
  melee), Stone Sentinel (slow, heavy, telegraphed slam), Marsh Wisp
  (ranged bolt). Spawn tables per biome/tier in creatures.json
- Death/respawn: player downed → fade → respawn at last shrine
  (shrines gain "attuned" = respawn point role)
- Target assist: soft-lock reticle highlight of creature under gaze (both
  cameras), nameplate + health bar (world-space, fades by distance)

### Epic D — Gathering, inventory, progression (Phase 3)
Data: `content/items.json`, `content/nodes.json`,
`content/progression.json`.
- Inventory: weight-based (Albion-style tradeoff: carry more = slower at
  threshold), grid UI with drag/drop, stack splitting, item tooltips;
  equipment slots: weapon, head, chest, feet, cape, bag, mount (bag raises
  capacity)
- Resource nodes: wood/stone/ore/fiber/hide(from creatures), tiers 1-4,
  per-biome distribution through spatial grid; gather = channel with
  progress ring; payoff = chunks fly out (mesh shards) + magnet to player +
  per-material sound; node depletes with regrowth timer (persisted per
  zone); higher tiers need higher-tier tool
- Tools as equipment with durability
- Classless progression ("the Ledger"): using X levels X — weapon lines,
  armor weights, each gathering profession, each crafting line. Mastery
  unlocks next tier usage + small stat bonuses. Board UI = tree of nodes
  per line with progress rings; XP events funnel through one
  `progression.Grant(line, xp)` API; level-up = full-screen-safe payoff
  (banner + chord + wisp fountain)
- Item tiers T1-T4 with quality rolls (normal/fine/exceptional) affecting
  stats

### Epic E — Refining, crafting, economy seed (Phase 4)
Data: `content/recipes.json`.
- Refining chains: log→plank, ore→bar, stone→block, fiber→cloth, hide→
  leather; requires station + lower-tier material (Albion-style recursive
  cost — keeps T1 relevant)
- Stations in Elder Vale temple town: sawmill, forge, kiln, loom, tannery,
  each a placed prop with interaction UI (queue, timer, collect with
  payoff burst)
- Crafting: gear from refined mats, stats derived from materials + quality
  roll + crafter's Ledger level; salvage returns partial mats
- Economy seed (pre-multiplayer): NPC vendor buying/selling at fixed
  prices as silver sink/source; silver currency + loot drops; marketplace
  UI shell (buy/sell orders) built against a local `IMarket` so the real
  player market later is a backend swap

### Epic F — World depth (Phase 5)
- Mounts: horse from primitives; summon/dismiss with cast time, speed +
  carry bonus, dismount on damage; mount = equipment slot item
- Day/night cycle + zone weather (fog density/color, rain particles,
  wind gusts affecting audio); night = spawn table shift (risk/reward)
- Dungeons: entrance props in t3/t4 zones → generated interior zone
  (rooms + corridors from prefab parts, its own ZoneInstance — reuses ALL
  zone machinery), boss room with unique creature + loot chest ceremony
- Fishing: water plane ponds/rivers in biomes, cast → bobber minigame
  (timing ring), fish items + trophy variants
- Farming plot at Elder Vale town: till/plant/water/harvest cycle on
  real-time timers (persisted), crops as crafting mats
- Housing v1: instanced personal island zone (again a ZoneInstance) with
  placeable props/stations/storage chests

### Epic G — Risk & PvP scaffolding (Phase 7 prep, single-player sim)
- Zone tiers gate risk: t1 safe; t2-3 "contested" (creatures ambush,
  higher value); t4 "lawless" — on death you drop carried loot (a corpse
  chest persists in zone state) — the full-loot tension without PvP yet
- Bandit AI (humanoid creatures using player weapon kits) in t3/t4 as
  PvP stand-ins to tune combat vs player-shaped enemies
- Party/guild data models + UI shells (roster, tags) against local stubs

### Epic H — Multiplayer foundation (Phase 6 — LAST, gated)
Do not start until A-F are polished and stable. Then:
- Extract per-zone sim into a headless target (`zion_server`) sharing the
  sim code; client keeps presentation. The ZoneInstance boundary IS the
  server process boundary — this was the point of Epic A
- Transport: UDP w/ reliability layer (consider GameNetworkingSockets via
  vcpkg — justify in ledger); protocol = client input commands @ 60 Hz +
  server snapshot deltas @ 20-30 Hz + events; client prediction for own
  movement (the controller is already deterministic fixed-tick),
  interpolation for remotes
- Zone handoff between server processes via a tiny coordinator; accounts +
  persistence service (SQLite first)
- THEN the real player market, trade, parties, guilds, open PvP flags

## 8. Definition of done (every task)

1. Builds clean at /W4, zero warnings
2. Smoke run + screenshot reviewed; autoplay extended if mechanic-facing
3. Works in FP and TP; keyboard+mouse (and pad after Epic B)
4. Feedback complete (VFX + SFX + UI), respects settings scales
5. 100+ fps maintained (check F3; if a feature costs >1 ms, it gets a
   setting and a note in the ledger)
6. TASKS.md checkbox updated; CODEX_PROGRESS.md entry written; committed
   with a clear message

## 9. What NOT to do

- No HTML/JS/Electron/WebAssembly anywhere, including tooling UIs
- No engine swap, no ECS rewrite, no "quick physics library" — see §5
- No copyrighted names/lore/asset mimicry (original everything)
- No multiplayer networking before Epic H gate
- No silent architecture forks — ledger first
- Do not drop the GODGUN stash; do not force-push

— Claude (Phase 1), 2026-07-20
