# ZION — Goals

## What Zion is

Zion is an original MMORPG in the spirit of sandbox games like Albion Online:
classless equipment-driven progression, a player-run economy, gathering /
refining / crafting professions, open-world PvE and PvP, guilds, territory,
and risk-vs-reward zones. Every system is an **original implementation** —
no copied assets, names, maps, lore, UI, or code from any existing game.

Zion ships as a **native downloadable desktop game** (Windows + macOS).
No HTML, no Electron, no browser tech. The architecture must keep future
console and mobile ports possible without rewrites (platform access is
isolated behind a small set of modules).

## Pillars

1. **Rewarding by default.** Nearly every player action pays out visible,
   audible feedback: XP wisps that fly to you, loot that bursts outward,
   resources that shatter, hits that flash. If an action feels flat, it is
   unfinished.
2. **Two first-class cameras.** First person is primary, third person is
   secondary, and *both* must feel deliberate. Movement, combat, gathering,
   interaction, and UI are designed against both from day one.
3. **PS2-era fantasy charm, modern polish.** Bold colors, readable
   silhouettes, stylized handcrafted-feeling environments, beautiful skies,
   atmospheric fog, satisfying particles. Never photorealism.
4. **An ancient, explorable world.** Giant ruins, forgotten temples, forests,
   mountains, caves, mythical creatures. Exploration is constantly rewarded
   with discoveries.
5. **Heavy, responsive combat.** Windups, impacts, trails, shockwaves,
   hitstop, sound. Successful hits are unmistakable, especially in PvP.
6. **Performance is a feature.** Target: 100+ FPS on a GTX 1650-class GPU.
   Profile regularly; never let performance debt accumulate. Scalable
   graphics settings exist from the beginning.
7. **Architecture over expedience.** Modular isolated systems, no giant
   files, no tight coupling, clear naming. Refactor thoughtfully when a
   better design is found; never rebuild everything on a whim.

## The one question

Every decision must answer **"Does this make Zion feel rewarding to play?"**
If not, improve it before moving on.

## Long-term system targets (original implementations)

Classless equipment system · gathering · refining · crafting · professions ·
gathering/crafting/weapon/armor progression trees · open world · PvE · PvP ·
guilds · parties · mounts · marketplaces · player economy · farming ·
housing · fishing · storage · transportation · world exploration · dungeons ·
risk-vs-reward zone tiers.

None of these are built until their phase arrives (see ROADMAP.md), but
foundation decisions must not block them.
