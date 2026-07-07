# TacMove — Handoff / Status

Single source of truth for the current state of the project. Replaces the old
`claude_handoff.txt`, `claude_status.txt`, and `CODEX_REVIEW.md` (all
consolidated here). For deep feature docs see `README.md`; for the online
server checklist see `ONLINE_SERVERS.md`; for cross-platform notes see
`PORTING.md`.

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
| **MiniCS**  | `minics_arena` (new)  | glock   | Playable | Flagship FPS loop: pistol, crate cover, duel-bot + dummy opponents |
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
- **MiniCS** is a genuine first playable FPS loop: a compact walled arena
  (`minics_arena`) with a raised mid, crate cover, a spare pistol at spawn,
  and moving duel-bot opponents plus dummies to shoot. Gun-appropriate control
  hint, glock equipped on entry.
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

Example visual check:
`build/tacmove --mode minics --fire-at 90 --frames 96 --screenshot shot.bmp --novsync`

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

1. **Real multiplayer foundation** (the big one): a client/server connection
   path over `protocol.h`, player spawn, a remote-player representation, then
   movement replication, then networked health/death/respawn. Move player
   health server-side as part of this. `ONLINE_SERVERS.md` covers discovery.
2. **Make MiniCS a round loop**: bomb/defuse or round-based scoring, enemy
   respawns, a scoreboard — turn the arena into an actual match.
3. **Host Local Server**: launch the dedicated server as a child process and
   auto-connect once the connection path exists (currently honest-placeholder).
4. **Deathmatch scoring** + **Zombies waves** + **Parkour timer/checkpoints**
   to graduate those modes from Early to Playable.
5. **Skins/Publish**: persist a chosen skin color with settings; wire Publish
   to a local map/skin library, then online when networking lands.
6. **Art pass** beyond box-art once a texture/model pipeline exists.
