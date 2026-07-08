# TacMove — Handoff / Status

Single source of truth for the current state of the project. **New here?
Read [`CODEX_HANDOFF.md`](CODEX_HANDOFF.md) first** — it is the structured
orientation (build/run/MiniCS/what-to-do-next); this file is the deeper living
status. For deep feature docs see `README.md`; for the online server checklist
see `ONLINE_SERVERS.md`; for cross-platform notes see `PORTING.md`. (The old
`claude_handoff.txt` / `claude_status.txt` / `CODEX_REVIEW.md` /
`NEXT_STEPS_FOR_CHATGPT.txt` notes are all consolidated into these two files.)

---

## Current branch

`wip-m3-content-platform` — the active WIP. **Do not merge to `main` yet**;
test first, then promote once confirmed good.

## Game identity (new direction)

**TacMove is a cross-platform game hub where "Game Modes" are the games inside
the game.** The old "world-first" framing (Play → Worlds) is gone. Players now
pick a **Game Mode** to play; **Maps** live inside modes; **Skins** are
cosmetic; **Servers** are where people will connect once networking lands.

- **Game Mode** = a ruleset + a default map + a starting loadout.
- **Map** = the place you play (a world template, or the sandbox glade).
- **Skin** = visual customization (placeholder today).
- **Server** = where people connect (not implemented yet; honest placeholders).

## Menu / game-mode structure

```
Main Menu
├── Play
│   ├── Game Modes ── MiniCS · Deathmatch · Sandbox · Zombies · Parkour · Social Hub
│   ├── Servers ───── Server List · Direct Connect · Host Local Server
│   └── Recent Games (quick-launch last mode)
├── Create
│   ├── Create Map · Edit Map · Skins · Publish
├── Profile
├── Settings
└── Quit
```

Game modes (see `src/shared/game_mode.cpp`):

| Mode        | Map (world template)  | Loadout | Status   | Notes |
|-------------|-----------------------|---------|----------|-------|
| **MiniCS**  | `minics_arena`        | glock   | Playable | **Round-based FPS vertical slice**: countdown → firefight vs 5 ranged bots → win/lose → restart. Ammo/reload/recoil, kill rewards, round HUD. See the dedicated section below. |
| Deathmatch  | `minics_arena`        | glock   | Early    | Arena + bots; no scoring/respawn loop yet |
| Sandbox     | test glade            | fists   | Playable | The full open world: gather/craft/build, spawn menu (B) |
| Zombies     | `zombie_pit` (new)    | glock   | Early    | Goblin "horde" stand-ins; no wave system yet |
| Parkour     | `movement_course`     | fists   | Early    | Jumps/tunnels/pillars; no timer/checkpoints yet |
| Social Hub  | `social_hub`          | fists   | Early    | Open plaza; local only until online lands |

Playable = a complete local loop. Early = still drops you into a real, working
world (never a dead "coming soon" button), but the full ruleset is not built —
each Early mode shows an `EARLY` badge and an honest on-entry note.

## What works

- **Game-mode-first menu** end to end: Play → Game Modes with per-mode cards
  (name, tagline, PLAYABLE/EARLY badge), Servers, Recent Games; Create with
  Create/Edit Map, Skins, Publish; Profile; Settings; Quit. `ESC` and every
  `BACK` button pop exactly one level (single `goBack()` definition).
- **MiniCS is a polished local vertical slice** (the focus of this pass): a
  full round loop (spawn → `GET READY` countdown → live firefight → `VICTORY`/
  `DEFEAT` → one-press restart) against five crimson **ranged** bots that
  advance, strafe, aim (a readable telegraph), fire red tracers at you, flinch
  when hit, and die with a hot spark burst. The Glock has a magazine, reload
  (R), dry-fire, and a recovering recoil view-punch; kills pay out a kill
  marker, `+100` popup, kill-confirm chime and score. Clean combat HUD: ammo,
  round clock, enemies remaining, score, damage direction indicator. See the
  **MiniCS vertical slice** section below for the full breakdown.
- All six modes boot, run, and exit cleanly (smoke-tested `--frames 120`).
- **Sandbox** is unchanged in feel: the furnished RPG glade with gather/craft,
  the build/spawn menu (B), inventory (Tab), carry/drop, melee + duel bot.
- **Cleaner presentation**: new hub title + accent, "games inside the game"
  mode cards, status badges, mode banner in-world, mode-aware control hint
  (gun modes never advertise melee/build keys; the build menu is a Sandbox
  feature only), skin-swatch picker placeholder, honest Servers/Publish text.
- **Tracers/gun** refined: slimmer, longer streak with a bright head; tighter
  impact puff (less of a screen-filling blob).
- Cross-platform build intact (macOS/Linux/Windows; OpenGL 3.3). Client +
  dedicated server build with **zero warnings** (`-Wall -Wextra` / `/W4`).

## What does NOT work yet

- **No real networking.** Servers → Server List / Host Local Server / Direct
  Connect all show honest "networking is a later milestone" messages; nothing
  fakes a connection. `src/shared/protocol.h` only reserves a version. What an
  online server list needs is written down in `ONLINE_SERVERS.md`.
- **No multiplayer replication**: no client/server connection path, remote
  players, movement replication, or networked health/death yet. Modes are
  single-player-vs-bots today.
- **Deathmatch/Zombies/Parkour/Social** are Early: no scoring, no wave system,
  no run timer, no online — just the working world + loadout.
- **Skins/Publish/Profile** are placeholders (swatch preview, honest "coming
  with online" copy, last-played readout). No persisted skin or account yet.
- Player **health lives client-side** (`playerHealth_` in `game.cpp`); it must
  move server-side when networking lands.
- Dedicated server is still config-only: it does not build worlds from
  templates, load `server/content/`, or simulate combat (the shared modules
  are ready for it when the sim/networking milestone starts).
- Viewmodels/world are still box-art (no texture/model pipeline).

## MiniCS vertical slice (current focus)

MiniCS is the one mode built to feel good, not just boot. The whole controller
is gated on `minicsActive_`, so the sandbox and every other mode are untouched.

### What changed this pass

- **Round loop** (`Game::startMinicsRound / updateMinics / minicsWin /
  minicsLose`, `game.cpp`): player spawns south, five enemies hold the north;
  a ~2 s `GET READY` countdown, then a 75 s live round. Win when all enemies
  are eliminated (with a speed/time bonus), lose when the player is downed or
  the clock expires. A dim `VICTORY` / `DEFEAT` end screen shows kills + score
  and a one-press restart (`FIRE`/`SPACE`; `ESC` → menu). Going down in a round
  ends it (no free sandbox respawn — see `damagePlayer`).
- **Ranged enemy AI** (`minics_bot` def; `Game::updateMinicsEnemies /
  minicsEnemyFire`): a small state machine — Advance → Strafe → Aim → Fire →
  Flinch. Enemies hold a 6–15 m engage band, sidestep around cover when a wall
  blocks them, telegraph with an aim pose, fire hitscan with distance-based
  accuracy (a miss still whips a red tracer past you), flinch on hits, and die
  at ~3 Glock body shots (90 HP). Drawn procedurally (walk/aim/shoot/flinch) in
  `appendMinicsEnemyDraws`, so they animate instead of standing like the dummy.
- **Glock feel**: client-side magazine (17) + reserve (68), `R` reload with a
  gun-dip/mag-swap animation, dry-fire click + auto-reload on empty, and a
  recoil view-punch that climbs on fast fire and springs back to the crosshair
  (`recoilPitch_`, folded into `lookDirection()` and the camera).
- **Feedback**: hit marker (existing) + a bigger red **kill marker**, a rising
  `+100` **score popup**, a **kill-confirm** chime, a directional **damage
  indicator** arc, screen shake, and round win/lose stingers. New synthesized
  sounds: `reload`, `dry_fire`, `kill_confirm`, `enemy_shot`, `round_win`,
  `round_lose` (all original/CC0, see `tools/generate_sounds.cpp` + CREDITS).
- **HUD** (`appendMinicsHud`): big ammo `MAG / RESERVE` bottom-right, round
  clock + `ENEMIES n/m` top-center, `SCORE` top-left, countdown/`FIGHT!`
  banners, reload prompt, end screen. The sandbox weapon/stamina bars are
  hidden in MiniCS; `B`/`Tab` (build/inventory) are gated to Sandbox only.

### Neon visual pass (latest session)

MiniCS (and its shell) got a dark-neon identity so the game reads as real, not
a blueprint — all inside the OpenGL 3.3 box-art budget:

- **Emissive rendering**: `BoxDraw`, `WorldBox` and `VisualPart` gained an
  `emissive` field (0 = lit, 1 = full-bright); the world fragment shader now
  takes per-frame fog color/scale/max, and emissive beats shading + fog.
  `RenderFrame` carries per-mode `skyTop/skyHorizon/fogColor/fogScale/fogMax`.
- **MiniCS atmosphere** (`buildRenderFrame`, only for `minics_arena`): dark
  indigo-to-magenta dusk sky + tighter purple fog. Every other world keeps the
  daytime defaults untouched (sandbox verified unchanged).
- **Neon arena** (`world_template.cpp` `minicsArena()`): charcoal shell, cyan
  wall-top LED strips, magenta/cyan floor lane lines, an amber glowing platform
  rim, a lime start line, and `neon_pillar` landmark props.
- **Menu backdrop** now orbits the neon arena (`Game::init`), so the hub looks
  like a game the instant it opens.
- **Punchier muzzle flash** (flaring multi-petal star + white-hot core).

New content: `neon_pillar` (builtin + `server/content/entities/neon_pillar.cfg`;
`.cfg` visual parts now accept an optional 10th value = emissive). No renderer
replacement, no new render features — just emissive + per-frame atmosphere.

### How to run MiniCS

- From the menu: **Play → Game Modes → MiniCS**, or launch straight in with
  `build/Release/tacmove --mode minics` (Windows) / `./run_*.sh` then pick it.
- Controls: `WASD` move, mouse aim, `LMB` fire, `R` reload, `ESC` pause/menu.
- Kill all five enemies before the 75 s clock; on the end screen press fire or
  space to play again.

### What was visually verified (screenshots, `--fire-at` + `--screenshot`)

- Countdown banner + full HUD; the live firefight (muzzle flash, gold player
  tracer, **red incoming enemy tracers**, enemies in aim poses, ammo counting
  down, health draining under fire); a confirmed **kill** (score → 100, `+100`
  popup, red kill-marker X, death sparks); the **VICTORY** end screen with
  score + restart prompt; a mid-**reload** (gun dipped, `RELOADING`); and that
  the **sandbox** Glock is unchanged (no ammo HUD, infinite, no recoil).

### What still feels bad / rough edges

- **Enemy movement has no pathfinding.** They sidestep around cover but can
  still bunch near the mid platform and won't aggressively rush; a player who
  camps at spawn is fairly safe until they push forward. Biggest feel gap.
- **Enemy fire is hitscan with distance-rolled accuracy** — no leading or
  suppression, so incoming damage can feel a touch random.
- **No headshots** (single AABB), so aim skill only rewards hitting at all.
- **Enemy death is a spark burst** (entity erased), not a fall/ragdoll — reads
  as a kill but is abrupt.
- Muzzle-flash boxes (player + enemy) are a little chunky.

### Exact files + values to tune next

- **Difficulty / pacing** — `src/client/game.cpp`:
  - `kMinicsRoundSeconds` (75 s round), countdown `minicsPhaseTimer_ = 2.0f`
    in `startMinicsRound`.
  - Enemy count + spawn spots: `placeMinicsEntities()` (5 `minics_bot`).
  - Engage band `kMinicsEngageMin/Max` (6/15 m); speeds `3.1` (advance) / `2.5`
    (strafe) in `updateMinicsEnemies`.
  - Enemy fire: cadence `0.85 + rf()*0.95` (`updateMinicsEnemies`); accuracy
    `0.90 - dist*0.020` and damage `8 + rand*5` (`minicsEnemyFire`).
- **Enemy toughness** — `server/content/entities/minics_bot.cfg`
  (`max_health = 90`) and the matching `content.cpp` builtin.
- **Gun** — `server/content/weapons/glock.cfg` (`damage = 34`,
  `cooldown = 0.14`) + builtin; magazine `magSize_ = 17`, `reserveAmmo_ = 68`,
  `reloadDuration_ = 1.45f`, recoil kick `recoilPitch_ += 0.62..0.92` and
  recovery `11.0f * dt` (all in `game.{h,cpp}`).
- **Enemy look** — `appendMinicsEnemyDraws` (body boxes, aim pose, walk swing).

## Build / run

First build on a bare machine self-bootstraps vcpkg + a private cmake (no
Homebrew needed on macOS). First run compiles SDL3 from source (a few minutes).

- **macOS:** `./run_macos.sh` (needs Xcode Command Line Tools)
- **Linux:** `./run_linux.sh` (apt deps listed in `README.md`)
- **Windows:** `run_windows.bat` (VS 2022 Build Tools + C++ workload)
- **Dedicated server:** `build/tacmove_server --ticks 384`

### Dev / testing flags (client)

| Flag | Effect |
|------|--------|
| `--mode <id>` | Boot straight into a game mode (`minics`, `deathmatch`, `sandbox`, `zombies`, `parkour`, `social`) |
| `--menu <screen>` | Boot onto a menu screen for screenshots (`play`, `gamemodes`, `servers`, `create`, `skins`, `profile`, `createmap`, `editmap`, ...) |
| `--world` | Skip the menu into the Sandbox glade |
| `--equip <id>` | Start holding a weapon (`glock`, `sword`, `karambit`) |
| `--frames N` | Exit after N frames (smoke testing) |
| `--novsync` | Uncap the frame rate |
| `--fire-at N` | Inject one attack press at frame N (capture recoil/tracers) |
| `--screenshot <path>` | Dump the last frame as a BMP (automated visual checks) |
| `--paused` | Start in-world on the pause menu |

**Framed runs use a fixed 60 Hz sim clock** (`main.cpp`): when `--frames` is
set, each frame advances exactly 1/60 s regardless of real frame rate, so a
given frame number always lands at the same sim time. This makes timed captures
reproducible — e.g. the countdown ends at frame 120, so `--fire-at 130`
reliably fires just after the round goes live. Frame N → N/60 seconds.

Example visual check (minics is live by frame ~200; capture a shot mid-fight):
`build/Release/tacmove --mode minics --fire-at 396 --frames 400 --screenshot shot.bmp`

macOS BMP→PNG for viewing: `sips -s format png shot.bmp --out shot.png`

## Where things live (for the next change)

- **Game modes:** `src/shared/game_mode.{h,cpp}` (registry + statuses/notes).
- **Map templates** (incl. new `minics_arena`, `zombie_pit`):
  `src/shared/world_template.cpp`.
- **Menu state machine / layout / drawing:** `src/client/game.cpp` —
  `buildMenuLayout()` (per-screen buttons), `activateButton()` (actions),
  `appendMenuDraws()` (titles, cards, placeholders), `goBack()` (nav),
  `startGameMode()` (mode → world + loadout).
- **UI enums / state:** `src/client/game.h` (`UiScreen`, `UiButton::Id`,
  `currentModeName_`, `sandboxMode_`, `lastModeId_`).
- **HUD / control hint / tracers:** `appendHudDraws()` and the Tracer block in
  `buildRenderFrame()`, both in `game.cpp`.
- **CLI flags:** `src/client/main.cpp`.

## What to do next (suggested order)

1. **Keep polishing MiniCS feel** (continue this pass): the biggest gap is
   enemy movement — add light pathfinding / lane routing so they push and flank
   instead of bunching at the mid; then headshot hitboxes, a proper death
   reaction, best-of-N rounds or a difficulty ramp, and an aim-down-sights.
   All the tuning knobs are listed in the MiniCS section above.
2. **Real multiplayer foundation** (the big one): a client/server connection
   path over `protocol.h`, player spawn, a remote-player representation, then
   movement replication, then networked health/death/respawn. Move player
   health server-side as part of this. `ONLINE_SERVERS.md` covers discovery.
3. **Host Local Server**: launch the dedicated server as a child process and
   auto-connect once the connection path exists (currently honest-placeholder).
4. **Deathmatch scoring** + **Zombies waves** + **Parkour timer/checkpoints**
   to graduate those modes from Early to Playable.
5. **Skins/Publish**: persist a chosen skin color with settings; wire Publish
   to a local map/skin library, then online when networking lands.
6. **Art pass** beyond box-art once a texture/model pipeline exists.
