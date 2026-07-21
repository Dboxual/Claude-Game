# ZION — Sandbox MMO Master Roadmap

Last reviewed: 2026-07-21

This is the product-completeness map for turning Zion into a first-person /
third-person fantasy sandbox MMO with the depth, interdependence, and
risk-versus-reward cadence players expect from Albion Online.

The target is **system parity, not content duplication**. Zion may reproduce
genre mechanics and player loops, but it must use original names, lore, maps,
art, audio, interface composition, numbers, balance, and progression layouts.
`ROADMAP.md` remains the short phase summary, `CODEX_HANDOFF.md` remains the
engineering execution contract, and this file answers the larger question:
"what must exist before Zion feels like a complete sandbox MMO?"

`ALBION_PARITY_CHECKLIST.md` is the researched, versioned implementation and
acceptance-test matrix. When this broader product map conflicts with an exact
coverage detail there, the checklist wins.

## 1. Product promise

Zion is a classless fantasy sandbox where what a player equips determines how
they fight, what they repeatedly do determines what they master, and nearly
every useful object begins as something gathered, refined, transported, and
crafted by a player. The world is made of large, distinct zones with rising
danger and value. Combat, gathering, travel, trade, social organization, and
territorial conflict all feed the same economy.

Zion's differentiators are:

- Both first-person and over-the-shoulder third-person are complete ways to
  play, not novelty camera modes.
- Direct movement, aiming, gathering, and melee replace an isometric cursor.
- Chunky PS2-era silhouettes meet luminous mythic effects and rich color.
- Information is readable and tactical without turning the screen into a
  dense spreadsheet.
- Every reward has a physical journey: burst, hang, home, collect, celebrate.

## 2. Current baseline

| Area | Current state | Next proof |
| --- | --- | --- |
| Engine | Native C++20/raylib, fixed 60 Hz simulation | Keep all gameplay server-shaped |
| World | Nine deterministic 1280 m zones and hard gate transitions | Persist per-zone state and soak transitions |
| Camera | Blended first/third-person camera rig | Validate every interaction and combat telegraph in both |
| Presentation | Stylized light, fog, sky, bloom, particles, day/night, HUD scaffold | Establish one UI/FX language across all screens |
| Player | Kinematic movement and camera-forward procedural body | Reusable animation pose system, weapons, hit reactions |
| Interaction | Gaze/proximity prompts, shrines, reward wisps | Gathering channels, loot, NPCs, stations |
| Data | Settings and save JSON | Content schemas for items, abilities, creatures, recipes |
| Online | No networking yet, intentionally | Headless zone server only after the offline MMO loop works |

## 3. Complete-system parity map

Each row is a product capability, not a promise to copy another game's exact
implementation. A row is complete only when its systems, UI, VFX, SFX,
persistence, balance hooks, and FP/TP behavior ship together.

### Identity, equipment, and progression

- Classless equipment build: weapon, off-hand where applicable, head, chest,
  feet, cape, bag, mount, food, and potion.
- Equipped items grant active/passive abilities; changing gear changes role.
- A large original mastery network ("the Ledger") covering combat families,
  armor weights, gathering, refining, farming, and crafting.
- Use-to-improve progression, tier unlocks, specialization, respec rules, and
  catch-up learning items.
- Item power derived from tier, enchantment, quality, mastery, durability, and
  contextual bonuses; inspectable before a fight.
- Loadout builder, saved equipment sets, quick re-equip, repair, salvage, and
  an appearance layer that never hides danger-relevant weapon silhouettes.

### Combat

- Basic attacks plus equipment abilities with cooldown, cast, channel,
  interrupt, energy, displacement, crowd-control, cleanse, immunity, and
  diminishing-return rules.
- Melee arcs, physical/magical projectiles, ground targets, beams, traps,
  summons, damage-over-time, healing-over-time, buffs, and debuffs.
- Clear friendly/enemy telegraph language at eye level and from the TP camera.
- Soft target assistance that preserves direct aiming; optional hard lock for
  accessibility, never required for optimal play.
- Build roles emerge from equipment combinations: frontline, bruiser,
  skirmisher, ranged pressure, control, healer, support, gatherer, escape.
- Downed/knockout rules in safer regions and lethal full-loot death in the most
  dangerous regions; durability loss and item destruction maintain demand.
- Duels, damage/healing breakdowns, combat log, practice dummies, and a build
  testing area before competitive modes.

### World, zones, travel, and risk

- Large instanced zones connected as a graph, with roads, gates, landmarks,
  towns, wilderness, interiors, and short readable transitions.
- Biome-specific primary/secondary/tertiary resources, creature ecologies,
  weather, lighting, set dressing, music, and navigation silhouettes.
- Eight content tiers at full scale; early builds prove tiers 1–4 first.
- Risk bands: sanctuary, guarded, contested, lethal, and lawless. Rules and
  death consequences are visible before crossing a boundary.
- Local map, world map, compass, discovered exits, activity markers, pings,
  party markers, death markers, and configurable marker filters.
- Mounts with distinct speed, carry weight, toughness, abilities, summon time,
  dismount rules, and a strong risk/economy role.
- Fast travel constrained by luggage and cost so transport remains gameplay.
- Dynamic resource hotspots, roaming elite threats, treasure, rare spawns,
  weather events, and temporary portals prevent solved farming routes.

### Gathering and resources

- Wood, ore, stone, fiber, hide, fish, crops, herbs, and rare magical
  catalysts, each with tools, tiers, enchantments, yield, and gather speed.
- Resource nodes telegraph remaining charges and rarity without requiring a
  tooltip; depletion and regeneration persist per zone.
- Biome distributions make travel and trade necessary rather than cosmetic.
- Gathering armor/food/build choices create escape and efficiency tradeoffs.
- Rare large resource creatures/aspects require combat or groups.
- Gathering feedback: anticipation loop, tool impact, material-specific break,
  colorful shards, homing pickup, number pop, inventory response, XP arc.

### Refining, crafting, items, and economy

- Raw-to-refined material chains with lower-tier inputs keeping old zones
  economically relevant.
- City/region production bonuses, station fees, resource-return rates, focus
  efficiency, capacity, and ownership create geographic specialization.
- Weapon, armor, tool, consumable, mount, furniture, building, and artifact
  crafting with tier, enchantment, quality, and crafter attribution.
- Local markets with buy orders, sell orders, history, tax, fees, search,
  favorites, presets, and regional price separation.
- Banks and market inventories are local; transport is physical and risky.
- Silver faucets and sinks: creature loot, tasks, repairs, taxes, station fees,
  market fees, islands, buildings, rerolls, and cosmetics.
- A system equivalent in purpose to a black-market buyer turns player-made
  equipment into PvE loot and closes the economic loop without copying its
  fiction or exact rules.
- Item loss, partial trashing, repair, salvage, studying/research, and
  consumables continuously remove supply.

### PvE and instanced activities

- Open-world creature camps, factions, patrols, elites, bosses, resource
  creatures, chests, escort threats, and world events.
- Solo and group dungeons with generated layouts, escalating rooms, bosses,
  shrine choices, and exit/escape decisions.
- Competitive PvE/PvP portals, corrupted solo hunts, two-team arenas, and
  open multi-party objectives—implemented with original rules and fiction.
- A shifting portal-road network for exploration, gathering, ambushes, and
  hideout logistics.
- A solo/small-group mist realm with uncertain exits and concentrated risk.
- Expeditions/tutorial instances for low-risk teaching and repeatable practice.
- Difficulty tiers, modifiers, matchmaking bands, anti-farm protections, and
  rewards tied back into the player economy.

### PvP, factions, territory, and guild warfare

- Consent/flagging and reputation rules in guarded regions; unrestricted
  conflict in lawless regions.
- Solo, small-scale, objective, arena, faction-frontline, territory, convoy,
  castle, fortress, and large-army conflict.
- Faction enlistment, province objectives, front lines, ranks, campaign
  rewards, and visible world-state changes.
- Guilds, alliances, roles, permissions, tax, activity feed, treasury, shared
  storage, recruitment, emblems, and season progression.
- Territories, hideouts, headquarters, vulnerability windows, supply, power,
  launch/defense rules, and season resets.
- Anti-zerg visibility and scaling tools, cluster queues, handoff rules, and
  performance budgets that keep large fights playable.
- Kill/death history, inspectable loadouts, leaderboards, trophies, and replay
  data sufficient for competitive learning and moderation.

### Social, ownership, and long-term life

- Friends, ignore/block, direct messages, local/global/trade/help/guild/party
  chat, moderation tools, mail, invites, pings, and inspect.
- Parties, raids, role markers, ready checks, loot rules, shared objective
  state, and reconnect handling.
- Personal islands, guild islands, housing, farms, pastures, laborers/workers,
  furniture, trophies, storage, and placeable crafting stations.
- Player and guild journals, achievements, daily/weekly goals, collections,
  vanity rewards, and a seasonal challenge track.
- Character appearance, wardrobe, emotes, mount skins, nameplates, portrait,
  and photo-safe HUD hiding.

### Service, safety, and access

- Accounts, characters, regions/shards, authentication, persistence,
  reconnect, maintenance messaging, patching, and version compatibility.
- Authoritative servers, prediction/reconciliation, interpolation, interest
  management, zone handoff, anti-cheat, rate limits, logs, and admin tooling.
- Mouse/keyboard and gamepad parity; remapping, sensitivity, inversion,
  hold/toggle variants, subtitles, scalable UI, colorblind-safe telegraphs,
  screen-shake sliders, reduced flashes, and motion options.
- Performance tiers, dynamic resolution option, load-time budgets, crash-safe
  saves, telemetry with consent, and repeatable benchmark scenes.
- Regional pricing and monetization are designed later; no pay-for-power.

## 4. Build order and gates

### Milestone 0 — Presentation identity (current)

Goal: make the existing foundation feel like Zion before expanding breadth.

- Finish Epic A persistence and transition soak.
- Ship the first UI style system, title/pause/settings composition, zone HUD,
  compass, interaction prompt, reward counter, banners, and onboarding hints.
- Establish `VISUAL_DIRECTION.md` as the review checklist for every asset.
- Keep XP/mastery wisps readable as violet/gold; assign separate consistent
  families to loot, healing, navigation, and hostile magic.
- Finish controller feel, player silhouette, set dressing, ground feedback,
  resolution/window options, gamepad, and macOS verification.

Exit gate: a new player can launch, understand controls, travel, activate a
shrine, receive a memorable reward, pause/configure, save, and return in both
cameras with a coherent visual identity and 100+ FPS target performance.

### Milestone 1 — The first complete player loop

Goal: fight, gather, craft, equip, improve, and repeat without placeholder UI.

- Player stats, combat framework, three weapon families, three armor weights,
  three creature families, death/respawn, loot, target UI.
- Five core gathered resources through tier 4, tools, inventory, equipment,
  weight, durability, refining, starter stations, and 30+ useful recipes.
- Ledger progression for every shipped activity and an inspectable build page.
- One safe town plus three surrounding risk tiers and one complete dungeon.

Exit gate: a two-hour offline session supports at least three viable goals and
all earned items/resources participate in a closed economic simulation.

### Milestone 2 — Regional economy and world depth

Goal: make place, distance, specialization, and transport matter.

- Eight tiers, enchanted resources, item quality, artifacts, rare spawns.
- Local market simulation, buy/sell orders, banks, station ownership/fees,
  city bonuses, transport contracts, and economy telemetry.
- Mount ecosystem, weather/day-night, more biomes, world events, fishing,
  farming, islands, housing, workers, and generated dungeon families.
- Local/world maps, route planning, market comparison, and journal goals.

Exit gate: automated economy soaks remain stable, no required item is created
from nothing, and profitable trade routes naturally emerge between regions.

### Milestone 3 — Competitive sandbox offline proof

Goal: prove risk and large-system interactions before networking multiplies
their failure modes.

- Full-loot rules, corpse recovery, hostile humanoid AI using player builds,
  factions, province objectives, territory, hideout, convoy, and siege sims.
- Solo, small-group, portal-road, mist, arena, and large-objective prototypes.
- Guild/party/market interfaces against local service implementations.
- Balance telemetry, combat logs, build comparisons, and bot-run scenario tests.

Exit gate: bots can gather, craft, trade, equip, travel, fight, die, lose gear,
and replace it for multi-day simulations without economic collapse.

### Milestone 4 — Networked alpha

Goal: turn the already-proven zone simulation boundary into server processes.

- Shared simulation library and headless authoritative zone server.
- Accounts, character persistence, snapshots, input commands, prediction,
  reconciliation, remote interpolation, events, reconnect, and zone handoff.
- Parties, chat, trading, markets, guilds, permissions, and moderation services.
- Staged tests: 2 players, 16, 64, 200 per zone, then multi-zone soak.

Exit gate: the complete Milestone 1 loop works online through disconnects and
zone transitions; authority and duplication exploits are covered by tests.

### Milestone 5 — Closed beta sandbox

Goal: validate the social economy and risk ecosystem with real communities.

- Player-run markets and stations, guild territories/hideouts, faction war,
  seasons, scoreboards, objectives, and full-loot lawless regions.
- Content cadence tools, server operations, customer support, reporting,
  compensation, rollback strategy, economy dashboards, and live configuration.
- Accessibility, onboarding, controller parity, performance matrix, and
  comprehensive new-player funnel telemetry.

Exit gate: repeatable seasons run without manual database intervention,
retention does not depend on giveaways, and no single activity bypasses the
shared economy.

### Milestone 6 — Launch and live expansion

- Scale regions based on measured demand; cross-region character policy.
- Seasonal content, weapons, biomes, dungeons, faction arcs, events, and
  cosmetics released through data-driven pipelines.
- Competitive integrity, economy health, server cost, accessibility, and
  community safety are treated as permanent features, not launch cleanup.

## 5. UI delivery ladder

UI is built in dependency order so screens do not become disconnected mockups:

1. Global tokens, typography, panels, buttons, tabs, focus, tooltip, modal,
   notification, input glyph, safe-area, and scaling rules.
2. Title, character entry, pause, settings, reconnect, loading, error states.
3. Gameplay HUD: health/energy, abilities, target, cast/channel, compass,
   zone/risk, party, chat, prompts, rewards, status effects.
4. Inventory/equipment, item tooltip/compare, loot, split/drag/drop, loadouts.
5. Gathering and crafting flows, station queue, recipe filters, quality odds.
6. Ledger progression, stats, journal, achievements, appearance, inspect.
7. Local map, world map, route/risk preview, activity and party filters.
8. Market, bank, trade, mail, repair, salvage, research, transport.
9. Party, guild, alliance, faction, territory, season, housing permissions.
10. Admin, moderation, support, patch/maintenance, and live-event surfaces.

Every UI slice must be operable at 720p, 1080p, and 4K; with mouse/keyboard
and gamepad; with FP and TP scene contrast; and without relying on color alone.

## 6. Immediate next sequence

1. Finish the current presentation-identity slice.
2. Finish repeated gate-transition soak and per-zone shrine persistence.
3. Add compass/gate pips and the 3x3 world-map screen.
4. Extract the procedural player pose into `src/anim/`.
5. Build the smallest combat loop: health/energy, one sword kit, one creature,
   target frame, ability bar, hit reaction, death, loot wisps.
6. Build the smallest economy loop: wood/ore/stone nodes, inventory/equipment,
   one refiner, one forge, three recipes, Ledger XP.

Do not skip directly to multiplayer, a giant inventory UI, or dozens of empty
content definitions. Each sequence must finish as a playable, feedback-complete
loop before the next layer expands it.

## 7. Definition of complete

Zion reaches the intended sandbox-MMO target when a player's equipment,
mastery, location, allies, enemies, carried goods, and economic choices all
change what they can safely attempt—and when all of those decisions remain
clear and satisfying in first-person and third-person.

The project is not complete merely because every row has code. It is complete
when gathering feeds crafting, crafting feeds combat, combat consumes gear,
risk changes prices, travel creates opportunity, guilds reshape territory,
and every loop returns value to the same living economy.

## Reference snapshot

This parity map was checked against Albion Online's official 2023–2026 guides
and update notes on crafting, refining, zone risk/flagging, the 2025 feature
set, and the 2026 visual-overhaul direction. References are for system coverage
only; no protected assets, writing, names, maps, or interface layouts should be
copied.

- https://albiononline.com/news/guide-crafting
- https://albiononline.com/news/guide-refining
- https://albiononline.com/news/guide-zone-types-flagging
- https://albiononline.com/news/2025-in-review
- https://albiononline.com/news/dev-talk-visual-overhaul
