# ZION — Visual Direction Bible

Last reviewed: 2026-07-21

Primary moving-image reference supplied by the project owner:
[YouTube - visual aesthetic reference](https://www.youtube.com/watch?v=7ud_xj8IWxs).
Treat it as the target for overall color, material, light, atmosphere, and
reward energy. Exact shot/frame notes are still pending because the current
tool session exposed no usable browser and YouTube blocked text extraction.
Future agents must inspect it directly when browser access works, or use owner-
provided screenshots; they must not invent details they have not seen.

This file is the visual acceptance test for Zion. The desired feeling is an
old-school PS2 fantasy adventure remembered more beautifully than it really
was: chunky, readable forms; strong colors; mysterious distance; and magical
rewards that feel almost tangible.

## 1. Emotional target

The world is ancient but not miserable. Ruins are warm in sunlight, forests
carry colored haze, dangerous places are seductive, and magic feels rare
enough to deserve brightness. The player should alternate between quiet
wandering, tactile work, sudden danger, and extravagant reward.

Five words guide reviews: **mythic, colorful, tactile, mysterious, readable**.

Avoid gray realism, generic high-fantasy filigree, flat mobile-game panels,
neon sci-fi glow, tiny details that disappear in motion, and visual noise that
hides combat information.

## 2. Rendering language

- Silhouettes first: every creature, weapon, resource, landmark, and station
  must be recognizable from shape before color or texture.
- Low-poly geometry is intentional. Use broad planes, visible facets, tapered
  trunks, blocky cloth, strong weapon profiles, and a few oversized accents.
- Lighting is storybook theatrical: warm directional sun, cool sky fill,
  colored ground bounce, soft distance fog, bright emissive magic.
- Bloom belongs to magic, treasure, sun glints, and reward moments—not every
  pale surface. A bright object should earn its halo.
- Texture detail stays broad and painterly when file-based art arrives. No
  noisy photogrammetry or micro-surface realism.
- Ground detail is living cover, not litter: grasses, flowers, moss, and biome
  growth are welcome; loose branches, decorative pebbles, and purposeless debris
  are not scattered merely to fill space.
- Atmospheric motion—motes, drifting spores, leaves, mist bands, cloud
  shadows—makes otherwise simple geometry feel alive.

## 3. Core palette tokens

| Token | Color | Use |
| --- | --- | --- |
| Night ink | `#0B1422` | Deep panels, silhouettes, shadow anchors |
| Slate ink | `#18283A` | Raised panels and controls |
| Ancient gold | `#D5A84F` | Borders, titles, sacred landmarks |
| Candle gold | `#FFD77A` | Selected states and reward highlights |
| Spirit teal | `#78E8D0` | Anima, interaction, navigation magic |
| Moon violet | `#A990F2` | rare knowledge, arcane effects |
| Ember coral | `#F07B59` | danger, damage, hostile telegraphs |
| Moss green | `#6EA45D` | life, recovery, safe-land identity |
| Bone white | `#E8E3D3` | primary text and sacred stone |
| Mist blue | `#91A8C8` | secondary text and distant atmosphere |

Each biome receives one dominant land color, one atmospheric color, and one
magical accent. Interface colors remain stable so risk and state never change
meaning when the scenery changes.

## 4. UI language: the Wayfinder's Archive

The UI should feel like a dark traveling archive made of inked glass, aged
metal, and luminous runes—not a parchment sheet and not a sci-fi HUD.

- Panels: near-black navy with enough transparency to preserve the world,
  layered shadow, thin cool inner line, warm-gold structural accent.
- Corners: clipped/rounded hybrid shapes with small diamond or notch motifs.
  Decoration concentrates at corners and headers, leaving content calm.
- Typography: high-contrast display treatment for place names and ceremonies;
  highly readable sans-serif for body, numbers, combat, and settings.
- Buttons: broad and tactile, dark at rest, warm edge on focus, gold-to-teal
  selection response, short luminous underline, immediate press feedback.
- Icons: bold single silhouettes inside circles, diamonds, or shield shapes.
  Do not rely on fine line art at gameplay size.
- Layout: breathing room and stable anchors. World/risk at top-left, compass
  top-center, currencies/rewards top-right, combat at bottom-center,
  communication bottom-left, contextual interaction near the reticle.
- Motion: 100–180 ms control response, 250–400 ms panel transition, soft
  overshoot only for rewards. Menus should feel deliberate, not floaty.

The screen must remain useful at 1280x720. All HUD groups live inside a 24 px
720p safe margin and expand outward at larger resolutions.

## 5. Reward and FX grammar

Every important reward follows a readable five-beat ceremony:

1. **Impact** — the source flashes, cracks, rings, or releases a burst.
2. **Scatter** — rewards occupy space long enough to be recognized.
3. **Anticipation** — pieces hover, orbit, or hesitate for a fraction.
4. **Homing** — color trails connect world reward to player.
5. **Confirmation** — UI count/bar responds with sound and a restrained pop.

Magical families remain consistent:

| Family | Core / halo | Motion | Meaning |
| --- | --- | --- | --- |
| Anima / XP | violet / blue | soft spiral, quick home | general mastery / life essence |
| Mastery | gold / amber | rising arc, bell-like hang | progression, discoveries, milestones |
| Arcana | violet / blue | orbit, angular sparks | rare magic, artifacts, knowledge |
| Vitality | green / yellow | pulse, petal burst | healing, growth, nature |
| Corruption | coral / magenta | snap, smoke pull | danger, curses, hostile power |

XP should not appear as a plain number alone. Use a pulsing violet or gold orb
or shard in the world, an arcing trail toward the relevant Ledger line, then a
compact text confirmation. Large level-ups may use a short fountain and chord,
but normal rewards must not interrupt control.

## 6. Animation language

- The body carries weight through anticipation, overshoot, and settle even
  when built from primitives.
- Walk/run cycles favor strong readable poses over realistic mocap subtlety.
- Equipment mass changes pose: heavy gear lowers center of gravity; staves and
  bows lead turns; capes lag and settle.
- Interactions show tool/source/player as one chain of motion: reach, contact,
  recoil, recovery, reward.
- Hit reactions point back toward impact direction. Major attacks telegraph in
  body silhouette before floor decals.
- FP animation is authored separately for comfort and clarity; TP animation
  communicates intent to opponents. Timings and gameplay events remain shared.

## 7. Camera rules

First-person should feel embodied, not shaky. Keep bob small, landing motion
short, sprint FOV restrained, hands/tools readable, and interaction targets
large enough at close range. Never place essential telegraphs only under the
player's feet.

Third-person should feel like an adventure game, not an isometric imitation.
The character silhouette stays readable against terrain, the boom respects
walls quickly, and combat effects leave clear space around the body.

UI information is identical between cameras unless it is genuinely redundant:
the FP crosshair can disappear in TP, but risk, target, status, cooldown,
interaction, and reward information cannot.

## 8. Zone identity

| Zone | Dominant feeling | Land / air / magic |
| --- | --- | --- |
| Elder Vale | welcoming sacred beginning | moss green / mist blue / candle gold |
| Thornwood | enclosed and watchful | deep pine / blue-green haze / spirit teal |
| Gilded Steppe | exposed, wind-cut abundance | ochre grass / warm dust / sun gold |
| Mirrormarsh | reflective and uncanny | blue-green mud / silver fog / violet |
| Cinder Barrens | dry danger and old fire | rust earth / amber dust / ember coral |
| Skyreach Highlands | heroic isolation | sage rock / clear cobalt / bone white |
| Verdant Maze | excessive life | saturated leaf / green haze / warm yellow |
| Sunken Necropolis | quiet funerary weight | gray olive / violet fog / pale cyan |
| Starfall Crater | beautiful cosmic damage | muted teal / indigo air / moon violet |

Biome differentiation requires more than tint. Each zone needs a unique horizon
shape, vegetation silhouette, ground rhythm, landmark grammar, ambient motion,
weather tendency, and audio bed.

## 9. Audio pairing

Color and motion are incomplete without sound. Favor wooden knocks, stone
weight, cloth movement, airy whooshes, glassy bells, low ritual voices, and
short melodic reward intervals. Avoid harsh UI beeps and constant epic music.
Silence and wind are part of the composition.

Each magical family gets a related interval/instrument identity. Pitch may rise
across a collection chain, but volume should not stack into fatigue.

## 10. Review checklist

Before calling a visual slice finished, verify:

- Can it be recognized as a silhouette in a grayscale thumbnail?
- Does its color have one clear purpose and adequate contrast?
- Does magic remain brighter than ordinary materials?
- Does motion communicate gameplay timing before decoration?
- Does the effect read in FP at close range and TP at several meters?
- Does UI remain legible over the brightest and darkest zones?
- Does it fit at 1280x720 with no clipped text or controls?
- Does it preserve 100+ FPS target performance and obey density settings?
- Is the result original rather than a traced asset, copied layout, or renamed
  piece of another game's world?

## 11. Current polish order

1. Global UI tokens and ornamental widget pass.
2. Title/pause/settings composition and zone HUD/compass/reward presentation.
3. Violet/gold luminous XP wisps and collection ceremony.
4. Grounded movement audio/material particles and procedural animation module.
5. Biome-specific set dressing, ambient motion, water, and weather.
6. Combat telegraphs, ability icons, hit/kill ceremony, and target frames.
7. Inventory, equipment, crafting, Ledger, map, and market screens.
