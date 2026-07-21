# ZION - Albion Gameplay Parity Checklist

Research snapshot: **2026-07-21**. This is the authoritative feature-coverage
document for the intended product: Albion Online's sandbox gameplay breadth,
rebuilt from the ground up for first-person and third-person play, with a small
amount of Elder Scrolls Online influence in camera presence, world scale,
exploration readability, and character presentation.

"Parity" in this file means comparable player-facing rules, systemic depth,
and activity coverage. Zion must use original names, lore, maps, models,
textures, audio, animation, UI art, writing, balance data, and network code. Do
not copy Albion or ESO assets, trade dress, source code, proprietary data, or
marketing. Reference games are behavioral test oracles, not content libraries.

## How future developers and AI agents use this file

- Read `GOALS.md`, this file, `MASTER_ROADMAP.md`, `VISUAL_DIRECTION.md`,
  `ARCHITECTURE.md`, `CODEX_HANDOFF.md`, and `TASKS.md` before implementation.
- `[x]` means implemented and verified in Zion, not merely documented.
- `[ ]` means absent or incomplete. Add a dated note when a partial slice lands.
- Never mark a section complete without normal-mode FP and TP tests. Dev-mode
  shortcuts may set up a test, but may not be required by production gameplay.
- Every feature needs data, simulation, UI, audio/visual feedback, save/network
  behavior, performance limits, and acceptance tests.
- Where exact live balance values matter, record the Albion client/version and
  observation date in a separate research table. Never guess hidden formulas.
- Preserve gameplay-server boundaries: authoritative fixed-tick state may not
  depend on camera, particles, UI, or developer controls.

## Current foundation status

- [x] Native C++20/raylib client, fixed 60 Hz simulation, interpolated rendering.
- [x] Toggleable first-person and third-person cameras with a shared interaction ray.
- [x] Nine deterministic 1280 m zones, hard gate transitions, biome palettes.
- [x] Action input, persistent settings, key rebinding, save slot, menus.
- [x] Permanent Escape fallback: Pause in play, Back in settings, Quit on title.
- [x] Runtime window-resolution selector and persisted width/height.
- [x] Camera-forward character at every yaw; strafing/backpedaling never turns
      the model away from the first-person forward direction.
- [x] Distance-driven procedural stride and planted character root.
- [x] Day/night presentation cycle with biome-aware dawn, day, dusk, and night.
- [x] Starter HUD scaffold: zone/risk, compass, clock, reward ledger, vitals,
      ability slots, interaction prompt, reticle, and reward banners.
- [x] Opt-in `--dev` creative tools isolated from production mechanics.
- [x] Groves, boulder fields, clear travel corridors, and route-adjacent wells.
- [x] Central Waystone anchors the journey/save; Echo Wells issue anima rewards.
- [ ] Per-zone state persistence and repeated 20-transition leak soak.
- [ ] Stable imported skinned-character path. `wayfarer.glb` remains disabled
      because raylib 6 `LoadModel` currently corrupts the heap on some layouts.

## 1. Non-negotiable product rules

- [ ] One character may eventually learn every combat, gathering, refining, and
      crafting branch; no permanent combat class locks.
- [ ] Equipped items define the active build and most combat abilities.
- [ ] Gathering, refining, crafting, hauling, trading, PvE, and PvP form one
      circular player economy with meaningful sources, transformations, and sinks.
- [ ] World danger escalates visibly from safe learning spaces to lethal full-loot
      regions, and increased danger produces increased opportunity.
- [ ] Storage and markets are local. Transporting goods is real gameplay.
- [ ] Death, durability, item destruction, station nutrition, consumables,
      territory upkeep, and construction keep the economy from saturating.
- [ ] The world supports solo, duo, small group, guild, faction, and large-scale
      play without making any single scale the only viable game.
- [ ] FP and TP are equal product modes. Neither is a novelty camera.
- [ ] Actions feel rewarding: readable anticipation, impact, persistent world
      change, loot/anima motion, sound, HUD response, and controller feedback.
- [ ] Original PS2-era mythic presentation remains readable under MMO-scale load.

## 2. Character creation, identity, and appearance

Albion reference behavior includes editable basic appearance, faces, hairstyles,
beards, underwear, wardrobe vanity, and mount skins. Zion should cover that
freedom while using a more physical, ESO-influenced character presentation.

- [ ] Character creation flow: name validation, body frame, face preset, skin
      tone, eye color, hair style/color, facial hair, markings, voice, preview.
- [ ] Optional restrained body/face sliders that remain compatible with armor.
- [ ] Neutral and action pose previews under representative day/night lighting.
- [ ] Rotate, zoom, FP hand preview, and TP full-body preview.
- [ ] Appearance is independent of combat progression and can be changed later.
- [ ] Account-wide cosmetic unlock catalog with clear obtained/source status.
- [ ] Wardrobe slots visually override head, chest, shoes, cape/back, weapon, and
      off-hand without changing stats.
- [ ] Per-slot hide toggles, including helmet and cape/back item.
- [ ] Mount appearance/skin is independent from mount statistics.
- [ ] Dye/material channels use Zion-original palettes and readable rarity rules.
- [ ] Emotes, portrait, inspect view, nameplate, guild crest, faction identity.
- [ ] All body types share hit rules; cosmetics cannot create competitive concealment.
- [ ] Armor fitting tests cover every body frame, locomotion, mount, and camera.
- [ ] First-person arms/hands reflect equipped body, gloves, weapon, and skin tone.
- [ ] Save schema separates immutable character ID, mutable appearance, wardrobe,
      equipment, and progression.

Acceptance:

- [ ] Create two visibly distinct characters, equip identical gear, and confirm
      identical simulation stats and correct FP/TP visuals.
- [ ] Change appearance after creation without losing equipment or progression.
- [ ] Mix six cosmetic slots and a mount skin with no clipping in baseline poses.

## 3. Inventory, equipment, items, and loadouts

Albion's reference inventory has ten main equipment slots: head, chest, shoes,
main hand, off-hand, bag, cape, food, potion, and mount, plus a fixed backpack.

- [ ] Implement the ten reference equipment roles in Zion's original UI.
- [ ] One- and two-handed rules; two-handed weapons disable the off-hand slot.
- [ ] Item instance ID, definition ID, stack, tier, enchantment, quality,
      durability, max durability, bound/attuned data, creator, and history.
- [ ] Tiers 1-8 with equipment-use and crafting unlock requirements.
- [ ] Enchantment states `.0`, `.1` Uncommon, `.2` Rare, `.3` Exceptional, and
      `.4` Pristine, with unmistakable color/glow that is not Albion trade dress.
- [ ] Five quality bands from baseline through masterpiece-equivalent.
- [ ] Item Power-equivalent derives from tier, item type, enchantment, quality,
      mastery, and specialization; server is authoritative.
- [ ] Durability loss, repair price, broken-state rules, and trash chance on death.
- [ ] Weight/load model; overload progressively slows movement.
- [ ] Bag, mount, food, and passive effects may change carrying capacity.
- [ ] Stack, split, sort, search, transfer, compare, quick-equip, trash confirmation.
- [ ] Local bank tabs, player chests, guild storage, battle vaults, permissions.
- [ ] Loadout templates include gear, chosen spells, consumables, acceptable tier,
      enchantment and quality ranges, and optional backpack contents.
- [ ] Lost-equipment loadout makes regearing understandable after lethal death.
- [ ] Armory/recommended-build browser uses anonymized Zion match/activity data;
      filter by solo/group/PvE/PvP and show transparent sample/performance context.
- [ ] Inspect view exposes abilities, stats, durability, value estimate, creator,
      and history without exposing hidden player information.

Acceptance:

- [ ] Swapping one equipped item immediately changes only its declared stats,
      abilities, appearance, weight, and valid off-hand state.
- [ ] A loadout can pull matching gear from authorized storage and clearly list
      missing or substituted items before confirmation.
- [ ] Inventory state round-trips through save and later authoritative server data.

## 4. Classless combat build system

Reference structure: weapon families supply shared first/second active choices,
a weapon-specific third active, and passives. Head, chest, and shoes add their
own active ability choices. Food, potion, cape, and mount add further effects.

- [ ] Six primary HUD actions: three weapon abilities, helmet, chest, and shoes.
- [ ] Separate potion, mount, food/status, interact, dodge, block, and basic-attack
      affordances where the equipped build supports them.
- [ ] Ability definitions are data-driven: cast type, target type, range, radius,
      cost, cooldown, cast/channel time, tags, effects, coefficients, VFX/SFX IDs.
- [ ] Equipment screen lets the player choose unlocked active and passive spells
      on each item and persists choices per item/loadout.
- [ ] Weapon families: Axe, Sword, Mace, Hammer, War Gloves, Crossbow, Bow,
      Dagger, Spear, Quarterstaff, Shapeshifter Staff, Nature Staff, Fire Staff,
      Holy Staff, Arcane Staff, Frost Staff, and Cursed Staff.
- [ ] Off-hand families: Shield, Torch, and Tome/Book equivalents.
- [ ] Standard, artifact/special, Avalonian-equivalent, mist-equivalent, crystal,
      and future weapon variants plug into the same family schema.
- [ ] Cloth, leather, and plate head/chest/shoes families, each with distinct
      offensive, utility, defensive, mobility, and gathering identities.
- [ ] Passive choices unlock by tier/mastery and update item inspection.
- [ ] Cape/back-item triggered passives use explicit trigger and internal cooldown.
- [ ] Consumable actives and persistent food buffs have dedicated slots and timers.

### Combat simulation

- [ ] Basic attacks, attack cadence, projectile/melee traces, animation events.
- [ ] Instant, cast-time, channel, toggle, charge, and ground-target abilities.
- [ ] Self, ally, enemy, cone, line, sphere, ground area, chain, and dash targets.
- [ ] Health, energy/focus, regeneration, armor, resistance, damage, healing.
- [ ] Buff/debuff stacking, cleanse, purge, immunity, reflect, shield, lifesteal.
- [ ] Crowd control: slow, root, stun, silence, knockback, knock-up, fear, sleep.
- [ ] Interrupts, invulnerability, invisibility/reveal, aggro drop, forced movement.
- [ ] Cooldown reduction, cast speed, diminishing-return/CC rules where required.
- [ ] AoE escalation and focus-fire protection equivalents for large fights.
- [ ] Friendly-fire, party, guild, alliance, faction, duel, and flag legality filters.
- [ ] Downed/knockdown, execute/death, revive, respawn, corpse/loot ownership.
- [ ] Damage/heal/CC combat log and deterministic replay-friendly event stream.

### FP/TP translation contract

- [ ] Simulation uses character origin/aim intent; camera only produces aim input.
- [ ] TP ray is reconciled from camera reticle to character/weapon origin so shots
      cannot pass through cover beside or behind the character.
- [ ] FP uses visible hands/weapon while preserving the same server hit volumes.
- [ ] Soft-target assist is optional, strength-limited, controller-aware, and never
      selects an illegal or fully occluded target.
- [ ] Ground targeting has range clamp, slope validation, nav/collision validation,
      clear valid/invalid shapes, and cancel behavior in both cameras.
- [ ] Enemy windups remain readable at FP peripheral limits; add spatial sound,
      edge indicators, and FOV-safe telegraphs instead of changing damage rules.
- [ ] Camera cannot reveal stealthed actors or information the character lacks.
- [ ] All mobility checks character collision, not camera clearance.

Acceptance:

- [ ] Every ability test matrix covers FP/TP, mouse/controller, moving/stationary,
      latency simulation, obstruction, edge range, cancellation, death, and zoning.
- [ ] Identical input and authoritative state produce identical damage in FP and TP.

## 5. Fame, mastery, and the Destiny-style board

- [ ] No global combat level that replaces activity mastery.
- [ ] Fame-equivalent categories: combat, gathering, refining, crafting, farming,
      adventurer, reaver/PvE, and specialization.
- [ ] Fame goes to the equipment/tool/activity actually used, under explicit rules.
- [ ] Central board shows prerequisites, progress, unlocks, tracked nodes, and search.
- [ ] Unlock higher item tiers; mastery broadens choices; specialization improves
      efficiency/power inside a family.
- [ ] One character can eventually master every board branch.
- [ ] Learning-point-equivalent may finish a node only after partial progress;
      acquisition and spend rules must avoid pay-to-win.
- [ ] Focus-equivalent is a capped regenerating production resource that improves
      return/efficiency but is not required to craft.
- [ ] Respec rules, catch-up rules, fame-credit items, study/salvage progression.
- [ ] Journal tracks account/character exploration, economy, PvE, PvP, gathering,
      islands, and social milestones with claimable original rewards.
- [ ] Progress tracker pins selected goals to HUD without turning sandbox play into
      a compulsory linear quest chain.

## 6. Resources, tools, and gathering

Reference natural resources are wood, ore, fiber, hide, stone, and fish.

- [ ] Wood uses axe; ore uses pickaxe; fiber uses sickle; hide uses skinning knife;
      stone uses stone hammer; fish uses fishing rod.
- [ ] Siege/demolition hammer and tracking toolkit are separate utility tools.
- [ ] Tools have tier gates, durability, gathering speed/damage, and clear silhouettes.
- [ ] Gathering gear sets: lumberjack, miner, harvester, skinner, quarrier, fisher;
      head/chest/shoes/backpack abilities reinforce the activity.
- [ ] Resource nodes store type, tier, enchantment, charges, max charges, health,
      respawn schedule, claimant/interaction state, and zone-spawn identity.
- [ ] A tool gathers its allowed resource/tier; higher mastery improves speed/yield.
- [ ] Enchanted node levels are visible at distance via color, particles, sound,
      material response, and map rules without relying only on hue.
- [ ] Partial nodes persist. Multiple gatherers and interrupted channels are safe.
- [ ] Resource mobs/aspects combine combat and gathering for exceptional payouts.
- [ ] Dynamic resource hotspots create temporary contested concentrations.
- [ ] Tracking follows clues to roaming creatures and rare alchemical materials.
- [ ] Fishing supports freshwater/saltwater, biome fish, rarity, bait, and minigame.

### Biome/resource distribution reference

- [ ] Steppe-equivalent: hide, fiber, ore.
- [ ] Forest-equivalent: wood, hide, stone.
- [ ] Swamp-equivalent: fiber, wood, hide.
- [ ] Mountain-equivalent: ore, stone, fiber.
- [ ] Highlands-equivalent: wood, stone, ore.
- [ ] Fish in all five biomes with distinct tables.
- [ ] Region tier, quality, risk, biome, time, weather, and activity modify spawns
      through authored tables, never arbitrary prop scatter.

### Reward-feel gate for every node

- [ ] Anticipation: tool equip, stance, contact point, readable progress.
- [ ] Contact: hit stop/pose accent, chips/sparks/dust, material-specific sound.
- [ ] Completion: node visibly depletes, larger burst, item/anima arcs to player.
- [ ] HUD: item icon, quantity, yield bonus, mastery fame, rare-drop banner.
- [ ] Persistence: charge is truly removed and respawns by authoritative timer.
- [ ] Performance: effects pool correctly with 50 nearby gatherers.

## 7. Refining, crafting, food, potions, and stations

- [ ] Raw-to-refined chains: logs/planks, ore/bars, fiber/cloth, hide/leather,
      stone/blocks; fish/food and herbs/potions have their own production chains.
- [ ] Dedicated stations equivalent to Warrior Forge, Hunter Lodge, Mage Tower,
      Toolmaker, Smelter, Lumbermill, Tanner, Weaver, Stonemason, Cook, Alchemist.
- [ ] Recipe schema declares station, tier, inputs, amounts, artifact/special input,
      output, nutrition use, time, silver/fee, unlock, and return-rate category.
- [ ] Batch crafting, queue preview, estimated return, focus use, fee, capacity.
- [ ] Craft quality roll depends on station/context, food/buffs, and specialization.
- [ ] Enchanted materials of the same level produce enchanted gear; `.4` follows
      special crafting/source rules and cannot be casually upgraded from `.3`.
- [ ] Artifact Foundry-equivalent enchants items and handles special artifacts.
- [ ] Awakened weapon equivalent: eligible top enchantment, owner attunement,
      PvE-earned points, traits, rerolls, strain/escalating cost, history, trade rules.
- [ ] Item study/salvage/destruction returns progression or materials under clear rules.
- [ ] Station owner sets access and fee; station requires food/nutrition and repair.
- [ ] Local production bonuses create geographically different best activities.
- [ ] Reference refining specialization mapping to preserve transport pressure:
      planks/forest-city equivalent, cloth/swamp-city, blocks/steppe-city,
      leather/highlands-city, bars/mountain-city (use Zion-original cities).
- [ ] Food gives long-duration economic/combat/gathering buffs and feeds stations.
- [ ] Potions give short active effects; support self, target, and ground throws.
- [ ] Farming ingredients and tracked rare materials close the consumable loop.

## 8. Markets, banks, transport, currency, and economic integrity

- [ ] Each city/major hub has an independent player marketplace and bank.
- [ ] Sell orders, buy orders, setup fee, transaction tax, duration, partial fills.
- [ ] Search/filter by item, tier, enchantment, quality, price, and location.
- [ ] Average/history charts use real transactions and resist wash-trade manipulation.
- [ ] Direct player trade with final review, change detection, capacity validation.
- [ ] Black-Market-equivalent NPC posts demand-based buy orders for player-made gear;
      purchased items feed eligible PvE loot, closing the craft-to-combat loop.
- [ ] Fast travel with carried goods is forbidden or priced to preserve hauling.
- [ ] Mount choice, carry weight, route danger, and local prices make transport a role.
- [ ] Silver-equivalent faucets/sinks are enumerated and telemetry-balanced.
- [ ] Gold/premium monetization is not assumed. Any real-money system requires a
      separate ethical, legal, platform, age-rating, and economy design review.
- [ ] Audit ledger for trades, markets, mail, guild storage, crafting, and deaths.
- [ ] Anti-duplication idempotency for every item/currency mutation.

## 9. Mounts and travel

- [ ] Mount item/slot, mount-up channel, voluntary dismount, forced dismount.
- [ ] Mount stats: health, armor, magic resistance, CC resistance, move speed,
      gallop speed, gallop delay, carry capacity, passive, and active abilities.
- [ ] Gallop begins after uninterrupted travel and ends on damage/threat conditions.
- [ ] Persistent mount remains nearby after voluntary dismount; radius controls
      quick remount and whether its carry bonus applies.
- [ ] Enemy dismount grants brief CC protection; remount lockouts are explicit.
- [ ] Weapon/armor ability lockouts after voluntary dismount follow authored rules.
- [ ] Journey-back equivalent channels to the last safe location with cost based on
      distance and carried goods; interruption and lethal-zone rules are explicit.
- [ ] Starter mule, agile riding, armored, transport, faction, rare, and battle roles.
- [ ] Horses/oxen and other domestic mounts can be raised and saddled on islands.
- [ ] Battle mounts support group buffs/disruption and alliance/region disarray limits.
- [ ] Mounted gathering, mounted combat exceptions, terrain collision, swimming,
      gates, interiors, knockback, FP view, TP camera, and animation are tested.

## 10. World topology, zones, danger, and travel rules

- [x] Large zones are isolated simulation islands connected by gates.
- [x] Nine-zone deterministic starter graph with zone-local generation.
- [ ] Two-continent-equivalent topology: safer developed continent and lethal
      guild-focused outer continent, using original geography and names.
- [ ] Five major biome identities with biome-specific resources and creatures.
- [ ] Starter towns and major cities with stations, market, bank, islands, factions.
- [ ] Portal towns/realmgates connect developed cities to lethal frontier regions.
- [ ] Region quality rises toward dangerous/deep territory and scales rewards/difficulty.
- [ ] Player-count/risk indicator uses ranges, not exact tracking exploits.
- [ ] Local map, world map, route planner, discovered exits, party/guild/faction data.
- [ ] Roads-equivalent: shifting portal graph, variable charges/group access, full-loot
      rules, hidden resources, chests, settlements, and unpredictable exits.
- [ ] Mists-equivalent: temporary solo/duo realms, mixed resources, creatures,
      unstable exits, and a hidden city reached through play.
- [ ] Smuggler-den/network equivalent offers frontier banking/travel/trade routes.
- [ ] Dynamic encampments, boss lairs, coffers, resource hotspots, and roaming
      crystal-equivalent creatures appear temporarily with personal/group rewards.
- [ ] Every permanent prop belongs to navigation, ecology, production, conflict,
      traversal, story, safety, or reward. Decorative scatter obeys composition rules.

### Danger bands

- [ ] Blue/sanctuary equivalent: no hostile open-world PvP; learning and low reward.
- [ ] Yellow/guarded equivalent: voluntary flagging and PvP knockdown, not full loot.
- [ ] Red/contested equivalent: lethal flagged PvP/full loot, reputation, visible
      hostile count/risk information, and lawful consequences.
- [ ] Black/lawless equivalent: unrestricted lethal PvP/full loot and no reputation loss.
- [ ] Clearly marked unrestricted-PvP subzones inherit the parent zone's death result.
- [ ] Flagging channel requires stillness, is interruptible, visible, and cannot ambush.
- [ ] Reputation gates access and recovers through defined lawful activity/time rules.
- [ ] Zerg markers begin at authored nearby-player thresholds and scale in size.

## 11. PvE, dungeons, and open-world activities

- [ ] Creature families/factions with tier, role, ability kit, aggro, leash, fame, loot.
- [ ] Open-world mobs, veterans, elites, mini-bosses, world bosses, resource creatures.
- [ ] Dynamic solo and veteran camps: timed locations, fame-fill objective, personal
      chest, one clear per spawn, explicit group scaling.
- [ ] Solo/group randomized dungeons with entrance lifecycle, layouts, shrines, chests.
- [ ] Static/open dungeons support natural PvP intrusion and local spawn competition.
- [ ] Expedition-equivalent safe instanced PvE and hard modes with normalized rules.
- [ ] Raid-scale PvE with role-readable telegraphs in FP/TP.
- [ ] Boss lair maps and limited-time entrances.
- [ ] Hellgate-equivalent 2v2, 5v5, and larger PvPvE variants with lethal/nonlethal tiers.
- [ ] Corrupted-equivalent solo dungeon: PvE route, invasion, direct duel or shard escape,
      and three risk/difficulty bands.
- [ ] Abyssal/Depths-equivalent high-pressure extraction PvP with bounded gear cost.
- [ ] Tracking hunts and rare creature ecosystems.
- [ ] Treasure chests, maps, lore, locks, vistas, and non-randomized points of interest.

## 12. Structured PvP and faction warfare

- [ ] Duels and duel safety/forfeit boundaries.
- [ ] Ranked 1v1 arena; normalized rule set, queue, rating, seasons, anti-win-trading.
- [ ] Arena/Crystal-equivalent 5v5 objective mode, practice and ranked queues.
- [ ] Larger battleground/crystal-league equivalents only after combat/network proof.
- [ ] City factions, enlist/unenlist, faction flag, points, ranks, rewards, transport.
- [ ] Faction outposts/camps, region control, bandit/event windows, defense/assault.
- [ ] Faction fortresses and siege objectives with readable attack/defense incentives.
- [ ] Faction mounts, capes/back items, resources, and hearts use original content.
- [ ] Matchmaking, reconnect, deserter, spectator, replay, reporting, and moderation.

## 13. Guilds, alliances, territories, hideouts, and seasons

- [ ] Guild create/join/find, roles, permissions, tax, activity log, crest, description.
- [ ] Alliance create/join, shared identity, relation rules, leave protection.
- [ ] Guild bank tabs, logs, loadouts/regear workflow, island and building permissions.
- [ ] Open-world territories, guards, tower defense, resource value, season ownership.
- [ ] Territory attacks require declared cost/window and resolve in open-world conflict.
- [ ] Hideout placement, construction, power, vulnerability, destruction, access, market,
      bank, crafting bonuses, headquarters protection, and evacuation/recovery rules.
- [ ] Castles/outposts and large-group objectives generate conflict and season points.
- [ ] Season cadence, invasion/reset days, rankings, might/favor, guild/member rewards.
- [ ] Siphoned-energy/power-core equivalents connect territory value to upkeep/conflict.
- [ ] Battle-mount limits, zerg indicators, cluster queues, and performance budgets.

## 14. Islands, buildings, farming, labor, and housing

- [ ] Personal island tied to a city; upgrade levels add building/farming plots.
- [ ] Guild island and territory farming plots with distinct access/value.
- [ ] Access-right UI for owner, co-owner, guild, friends, public, custom lists.
- [ ] Place, build, feed, repair, upgrade, move/demolish under explicit refund rules.
- [ ] Farm, herb garden, pasture, and kennel equivalents.
- [ ] Crops, herbs, animals, rare offspring, seed return, watering/nurture via focus.
- [ ] Real-time growth persists offline and is server-time authoritative.
- [ ] Mount raising and saddling connects farming to transport/combat.
- [ ] Houses, furniture, trophies, chests, visual customization, visitation.
- [ ] Laborer/work-order equivalent only if it creates healthy logistics and sinks.
- [ ] Food/potions/station nutrition consume farming output continuously.

## 15. Social, communication, groups, and safety

- [ ] Friends, blocks, recent players, inspect, whisper, mail, ignore, presence privacy.
- [ ] Chat channels: local, party, guild, alliance, faction, trade, help, system.
- [ ] Party/raid roles, invites, leader, ready check, markers, loot rules, finder.
- [ ] Guild finder, activity tags, language/timezone, application workflow.
- [ ] Pings and contextual world markers that cannot reveal hidden information.
- [ ] Trading, gifting, mail attachments, and COD with scam-resistant confirmation.
- [ ] Report categories, mute/block, chat filters, rate limits, audit trail, GM tools.
- [ ] Streamer mode and privacy options.
- [ ] Names, chat, guild identity, and user content pass moderation and platform rules.

## 16. UI, controls, accessibility, and platform parity

- [x] Scaled immediate-mode UI theme and basic menu/HUD framework.
- [x] Resolution, fullscreen, FPS, render scale, FOV, view distance, bloom, particles.
- [ ] HUD editor with anchor, scale, opacity, visibility, reset, and presets.
- [ ] Inventory/equipment, abilities, destiny board, map, journal, market, bank,
      crafting, faction, guild, island, mail, social, settings, and help screens.
- [ ] Mouse/keyboard and full controller navigation with last-input prompt switching.
- [ ] Controller radial targeting/menus and smart targeting tuned independently of mouse.
- [ ] Rebinding for keyboard, mouse, and controller, including conflict resolution.
- [ ] Text scale, UI scale, subtitle/caption options, chat scale, high contrast.
- [ ] Color-blind-safe rarity, danger, faction, targeting, and node enchantment cues.
- [ ] Reduce camera motion, screen shake, flashes, bloom, particles, and combat clutter.
- [ ] Hold/toggle alternatives, aim sensitivity/acceleration, deadzones, invert axes.
- [ ] Spatial audio cues have visual alternatives; important visuals have sound cues.
- [ ] Connection-health UI, reconnect state, latency and packet-loss display.
- [ ] Performance presets and large-battle character/effect limits.
- [ ] FP/TP camera toggle and scroll transition inspired by ESO's usability, while
      retaining Zion's own camera tuning and shared combat rules.

## 17. Visual, audio, animation, and world-feel checklist

- [ ] Use the linked YouTube reference in `VISUAL_DIRECTION.md`; exact frame study
      remains pending until screenshots or a working browser session are available.
- [ ] PS2-era clarity: deliberate silhouettes, selective detail, grounded materials,
      saturated magical accents, stable shapes, controlled texture resolution.
- [ ] Magical/mystic/mythologic effects use anticipation, rune/shape language,
      color family, directional motion, impact, motes, decay, and matching sound.
- [x] XP/fame/anima orbs pulse violet or gold, scatter, home, collect, and retain
      visible hue under bloom.
- [x] Clean grass cover adds living detail without branch/rock/debris litter.
- [ ] Mining/gathering effects are material-specific and scale with tier/enchantment.
- [ ] Biomes have stronger identity through lighting, vegetation, ground, water,
      cloud shadows, windborne details, insects/petals/snow/sand, and ambient audio.
- [ ] Trees read as species with trunks/branches/canopy architecture, not spheres.
- [ ] Rocks read as fractured geology with planes/strata, not stretched spheres.
- [ ] Weather and day/night preserve interactable, enemy, and danger readability.
- [ ] Locomotion set: idle, start, walk, run, sprint, stop, pivots, strafes, jump,
      fall, land, gather, tool, weapon, cast, hit, downed, death, revive, mount.
- [x] Camera-forward facing and distance-based stride stay frame-rate independent;
      presentation-only Y smoothing removes heightfield/fixed-tick jitter.
- [ ] Armor, weapons, tools, capes, hair, and mounts have attachment/cloth rules.
- [ ] Large battles enforce effect priority: self > threat > party > objective > ambient.

## 18. Day/night and weather

- [x] Presentation clock advances and grades sun, sky, ambient light, fog, and biome.
- [x] Dev-only F9 advances three hours for testing.
- [ ] Sun/moon, stars, cloud shadows, windows/torches, and ambient creatures.
- [ ] Weather profiles per biome: clear, rain, fog, wind, snow, sand, magical storm.
- [ ] Server-authoritative world time persists and synchronizes across zone processes.
- [ ] Decide and document gameplay consequences before adding them; day/night may not
      silently change competitive visibility, spawns, or gathering yield.
- [ ] Accessibility setting bounds darkness and weather obstruction.
- [ ] Save/load, zone transition, pause, reconnect, and long-session cycle tests.

## 19. Server, persistence, security, and operations

- [ ] Authoritative server process per zone/instance with fixed-tick simulation.
- [ ] Gateway/session, character, inventory, market, guild, chat, match, and telemetry
      services have explicit ownership and idempotent APIs.
- [ ] Client prediction/reconciliation for locomotion; lag compensation policy for combat.
- [ ] Interest management, entity IDs, snapshots/deltas, event ordering, rate limits.
- [ ] Transactional inventory/currency/market/crafting/death operations.
- [ ] Persistent zone timers and safe transfer handshake across gates.
- [ ] Disconnect, reconnect, crash recovery, rollback, migration, maintenance mode.
- [ ] Anti-cheat validates movement, cooldown, range, line-of-sight, inventory, trades.
- [ ] Metrics: latency, tick time, queue, entity count, economy, sources/sinks, errors.
- [ ] Structured logs and trace IDs without exposing secrets or personal data.
- [ ] Backups, restore drills, schema migrations, data retention, account deletion.
- [ ] Regional deployment, capacity/cluster queue, DDoS protection, incident playbook.
- [ ] Bots/RMT detection with appealable enforcement and careful false-positive review.

## 20. Developer mode and automated verification

- [x] `--dev` explicitly enables creative controls; normal launch cannot invoke them.
- [x] F1 panel, F4 fly, F5 turbo, F6 production-path reward burst, F7 next zone,
      F8 spawn return, F9 time advance; flight uses Space/Ctrl vertically.
- [x] `--data-dir` isolates automated settings, saves, and logs.
- [x] `--dev --time H` provides deterministic day/night screenshots without
      changing the normal world's time rules.
- [x] `--frames`, `--screenshot`, `--autoplay`, `--tp`, and `--zone` smoke controls.
- [x] `--record-dir` and normal-mode `--tour` capture movement/look/key evidence
      without OS input or screen-recording permissions.
- [ ] Dev command registry with console, permission levels, discovery/help, audit.
- [ ] Spawn item/node/mob, set fame, teleport, kill/revive, time/weather, network fault.
- [ ] Production mechanics never query dev state for validity or success.
- [ ] Normal-mode tests create/gather/craft/equip/fight/die/loot/trade through public APIs.
- [ ] FP and TP screenshot baselines at common aspect ratios and UI scales.
- [ ] Determinism test: same seed/input produces same authoritative event hash.
- [ ] 20-transition leak soak, 60-minute locomotion soak, 100-player bot combat soak.
- [ ] Save compatibility, corrupt settings fallback, Escape safety, resolution switch,
      controller-only navigation, reconnect, and transaction retry tests.

## 21. Delivery order and release gates

### Gate A - foundation integrity (current)

- [x] Escape, resolution, facing, stride jitter, day/night foundation, UI scaffold.
- [x] Purposeful route/prop composition and opt-in dev controls.
- [ ] Per-zone persistence and repeated-transition soak.
- [ ] Imported character pipeline fixed or replaced with a stable original format.

### Gate B - one complete gathering/equipment loop

- [ ] Inventory/item instance schema, one armor set, one weapon, one tool, one tree node.
- [ ] Gather -> reward -> inventory -> refine -> craft -> equip -> fame progression.
- [ ] Node visuals meet the reward-feel gate and persist after save/zone/relaunch.

### Gate C - combat vertical slice

- [ ] Three weapon families, cloth/leather/plate sample sets, consumable, cape passive.
- [ ] Three creatures, one boss, death/respawn/loot, FP/TP parity and controller.

### Gate D - economy and world risk

- [ ] All six resources, refining/crafting stations, local bank/market, transport mount.
- [ ] Sanctuary/guarded/contested/lawless rules with simulated player adversaries.

### Gate E - multiplayer proof

- [ ] Two clients, authoritative zone, prediction/reconciliation, combat, loot, gate transfer.
- [ ] Transaction-safe trade/market and reconnect with no item duplication.

### Gate F - complete sandbox alpha

- [ ] All sections above have feature owners, data coverage, tests, telemetry, and UI.
- [ ] Content volume targets are set separately and met with original assets/content.
- [ ] Performance, accessibility, moderation, security, recovery, and legal reviews pass.

## Primary research sources

Prefer these first-party pages, then the official Albion Wiki for mechanical
taxonomy. Re-check them whenever Albion ships a major update.

- [Albion: world, continents, biomes, cities, Roads, and Mists](https://albiononline.com/news/guide-world-albion)
- [Albion: zone types, flagging, reputation, factions, and zerg markers](https://albiononline.com/news/guide-zone-types-flagging)
- [Albion: crafting, enchantments, artifacts, markets, and item sinks](https://albiononline.com/news/guide-crafting)
- [Albion: refining and local production bonuses](https://albiononline.com/news/guide-refining)
- [Albion: mounts and persistent-dismount rules](https://albiononline.com/news/guide-mounts)
- [Albion: farming, islands, crops, animals, and focus](https://albiononline.com/news/guide-farming)
- [Albion: Learning Points and board progression](https://albiononline.com/news/guide-learning-points)
- [Albion: character appearance and mount skins](https://albiononline.com/news/devtalk-character-customization-mount-skins)
- [Albion: Awakened Weapons](https://albiononline.com/news/dev-talk-awakened-weapons)
- [Albion: Guild Seasons, territories, and hideouts](https://albiononline.com/news/guide-guild-seasons)
- [Albion: dynamic open-world objectives and HUD editor](https://albiononline.com/news/horizons-announcement)
- [Albion: 2025 feature recap](https://albiononline.com/news/dev-talk-2025-recap)
- [Albion: 2026 Radiant Wilds, Armory, controller, and arenas](https://albiononline.com/news/dev-talk-radiant-wilds)
- [Albion Wiki: weapon taxonomy](https://wiki.albiononline.com/wiki/Weapon)
- [Albion Wiki: inventory/equipment slots](https://wiki.albiononline.com/wiki/Inventory_UI)
- [Albion Wiki: tools](https://wiki.albiononline.com/wiki/Tools)
- [Albion Wiki: Item Power](https://wiki.albiononline.com/wiki/Item_Power)
- [ESO official support: first/third-person camera switching](https://help.elderscrollsonline.com/app/answers/detail/a_id/4598/~/how-do-i-switch-between-first-and-third-person-in-the-elder-scrolls-online%253F)
- [ESO official new-player exploration guide](https://www.elderscrollsonline.com/en-us/newplayerguide/questing)

Research gaps to close with direct observation before exact balance work:

- [ ] Capture the supplied YouTube visual reference at representative frames.
- [ ] Record live Albion 2026 HUD/action mappings, targeting options, controller UX,
      character creator fields, market filters, death flows, and all current weapon
      variants in a versioned observation appendix.
- [ ] Validate every numerical formula against official patch notes/client tooltips;
      do not treat community-wiki numbers as immutable server truth.
