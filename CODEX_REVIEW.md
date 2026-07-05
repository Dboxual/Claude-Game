# Codex handoff — Phase 2 + M3 content platform

**M3 addendum (latest drop)** — on top of everything below, this branch now
has: data-driven content (`server/content/weapons|entities/*.cfg` parsed by
`src/shared/content_loader.{h,cpp}`; builtins are fallback, file ids replace
builtins), `server.cfg` relocated to `server/server.cfg` (old path still
read), explicit `respawn_seconds` everywhere (props 0/never — crate + new
barrel; pickups 10 s; dummy 5 s), E-carry/drop for `carryable` props (stable
`WorldEntity::id` handles, carried props leave collision/rays, drop settles
downward), real SDL3 audio (8 mixed voices, 5 synthesized original WAVs in
`server/content/sounds/`, master*sfx gain), a rect-composed first-person
viewmodel (fists/karambit/glock with swing + recoil), and a CS-style server
browser shell (Favorites/LAN/History/Direct Connect, honestly empty lists,
no fake servers). Review focus for M3: the content_loader parse defaults,
the carried-prop lifecycle (pause/quit/save while carrying), and that the
server binary intentionally does not read `server/content/` yet.
`claude_status.txt` lists done/unfinished/broken for this drop.

---

# Phase 2 handoff (previous drop): spawned things are real

Phase 2 turns Phase 1's inert markers into interactive objects: solid props
that block movement, weapon pickups you grab with E, a working karambit
(melee) and Glock (hitscan), a damageable respawning training dummy, and a
categorized GMod-style spawn menu. Still deliberately absent: multiplayer,
plugins, ammo, real physics, textures/models, advanced rendering.

## What changed

**Shared (`src/shared/`, std + glm only, server-safe):**

- `content.h/.cpp` — split content into `WeaponDef` (id, kind melee/hitscan,
  damage/range/cooldown; fists 10/1.6/0.4s, karambit 35/1.9/0.5s, glock
  25/60/0.15s — knife-tier on purpose, skins must stay cosmetic) and
  `EntityDef` (now with `weaponId` linking pickups to weapons, `maxHealth`,
  `respawnSeconds`, and `visual`: a list of axis-aligned box parts so the
  glock reads as a pistol and the karambit as a curved claw without a
  texture system). `fists` is no longer an entity def — it is the base
  loadout weapon only. Categories are now Weapon/Bot/Prop/Pickup.
- `world.h/.cpp` — `rayVsAABB` slab test; `WorldEntity` carries runtime
  state (`active`, `health`, `respawnTimer`, `hitFlash`) plus per-instance
  `respawnSeconds`; `World::tick` drives respawns; `raycastGeometry` +
  `raycastEntities` (masks: AttackTargets sees solid/damageable and lets
  bullets pass pickups; Pickups sees only pickups); `damageEntity` (red
  flash, death → respawn timer or erase) and `consumePickup`.
- `player.cpp` — **bug fix**: `slideMove` clamped only against static world
  boxes, so "solid" entities never actually blocked movement (crouch and
  grounded checks used `overlapsAny`, movement did not). It now clamps
  against solid active entities too. Movement math itself is untouched.
- `inventory.h` — new, header-only: owned weapon ids + equipped slot,
  `acquireAndEquip`, `selectSlot`.
- `world_save.h/.cpp` — format v2: `entity = <id> <x> <y> <z> <respawn_s>`.
  v1 4-field lines still parse (respawn falls back to the def default).
  Runtime state is never saved; consumed-for-good pickups are simply absent.

**Client (`src/client/`):**

- `platform/input.h` + `platform/sdl/sdl_input.cpp` — `interactPressed` (E),
  `attackPressed` (left mouse edge), `slotPressed` (1–9).
- `game.h/.cpp` — combat/interaction loop: per-frame aim raycast (walls
  occlude), "PRESS E TO PICK UP …" prompt, target health readout, pickup →
  auto-equip + toast, attack with cooldown, hit marker, placeholder muzzle
  flash / melee swing rects, bottom weapon bar with bracketed equipped slot.
  `World::tick` runs inside the fixed-tick loop. Spawn menu got category
  tabs (Weapons/Bots/Props/Pickups; Pickups is an empty placeholder tab).
  Entity rendering: inactive entities skipped, multi-part visuals, weapon
  pickups hover/bob, damage flashes red.

**Server:** untouched; builds and runs (tac_shared only, no SDL/GL).

## Files changed

- New: `src/shared/inventory.h`
- Modified: `src/shared/content.h/.cpp`, `src/shared/world.h/.cpp`,
  `src/shared/world_save.h/.cpp`, `src/shared/player.cpp`,
  `src/client/game.h/.cpp`, `src/client/platform/input.h`,
  `src/client/platform/sdl/sdl_input.cpp`, `README.md`,
  `worlds/demo/world.cfg` (local, gitignored), `CODEX_REVIEW.md`

## Build / run

```bash
./run_macos.sh                                  # macOS: build + run (menu)
./run_macos.sh --frames 120 --world --novsync   # smoke test
./build/tacmove_server --ticks 384              # dedicated server smoke test
```

## How this was tested

- Clean build, `-Wall -Wextra`, zero warnings (client + server, Apple M2).
- Unit tests (scratch, not checked in): rayVsAABB edge cases, raycast
  masks/occlusion, damage → death → respawn-with-full-health, pickup
  consumption (respawning and permanent), inventory, save v2 round trip and
  v1 fallback.
- **Headless integration test** driving the real `Game` class with synthetic
  input through the actual menus (possible because `tac_client_game` links
  no SDL/GL): spawn glock via spawn-menu clicks → walk over → E pickup →
  auto-equip → 4 shots destroy the dummy (health readout verified at
  75/100) → dummy respawns at 100/100 → karambit pickup → solid dummy
  blocks the player → 3 slashes destroy it → slot switch → spawned crate
  blocks the player → Create World via UI → auto-save on spawn verified in
  the file → Quit → Load World → entity restored. This test caught the
  slideMove bug above.
- On-screen smoke test (`--frames 120 --world --novsync`) + server ticks.

## Risky areas / known cuts

- **Entity indices are positional**: `damageEntity`/`consumePickup` can
  erase from the entities vector, invalidating indices/EntityHits. Game
  clears its cached aim hits after mutating and re-resolves next frame, but
  this is easy to misuse — consider stable ids before anything holds a
  reference across frames.
- **Pickup rays ignore solid entities**: you can grab a pickup through a
  crate/dummy (only static geometry occludes). Cosmetic-tier for now.
- **Melee is a single center ray**: strafing targets at the reach limit can
  be missed; a swept/cone check would feel better.
- **No ammo**, Glock is semi-auto unlimited by design this phase.
- **Spawn placement still naive** (2.5 m ahead at foot height): you can
  embed a crate in a wall or spawn it overlapping yourself (collision keeps
  you from getting stuck, but it looks rough).
- **Auto-save rewrites the file on every spawn**; fine at the 256-entity
  cap.
- Dummy is one big AABB (no head hitbox); `visual` parts are cosmetic only.

## What Codex should review

1. The `slideMove` entity-clamp fix in `src/shared/player.cpp` — movement
   feel must be unchanged against static geometry (same clamp math, just
   applied to more boxes).
2. Raycast mask semantics in `World::raycastEntities` — especially bullets
   passing through pickups and the geometry-occlusion contract being
   caller-side.
3. Index invalidation contract described above.
4. `Game::update` is accreting combat state; consider extracting a
   `CombatController` (shared candidate for the server) before Phase 3.
5. The frame-vs-tick split: attack cooldowns and toasts tick per frame,
   respawns per fixed tick — fine single-player, needs a decision before
   networking.

## Phase 3 suggestions

1. **Player health + damage intake**: dummy fights back or hazards exist;
   HUD health bar (shared damage path already exists).
2. **Ammo + reload for the Glock**; pickup grants a magazine; ammo HUD.
3. **Head hitbox**: use the dummy's visual head part as a second AABB with
   a damage multiplier.
4. **Data-driven content**: load WeaponDef/EntityDef from `content/*.cfg`
   through the registry; builtins become fallback (ids already decouple
   saves).
5. **Prop manipulation**: E-grab/carry/drop for crates (GMod physgun-lite).
6. **Server world simulation**: `tacmove_server --world <folder>` loading
   the same save + ticking `World` — first real shared-sim milestone.
7. **Viewmodel**: first-person weapon rendering (needs oriented boxes or a
   tiny mesh path in the renderer).
