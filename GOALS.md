# Zion goals and phased roadmap

## Product direction

Zion is an original first-person-first, fully third-person-playable sandbox
action RPG. Its long-term systems target classless equipment-driven combat,
gathering, refining, crafting, progression, economy, guilds/warbands,
factions, territories, PvE, and PvP. It may study broad genre patterns, but
must not copy proprietary code, maps, art, text, names, UI, sound, or extracted
assets from Albion Online or any other game.

The visual target is crisp, colorful, vibrant, and readable, with low-to-mid
poly PS2-era personality and some of Borderlands 2's color energy, without
black outlines. Performance and visual quality are co-equal requirements.
Dynamic resolution must be an explicit player setting and clearly disclosed;
it must not silently hide rendering cost or make the default image blurry.

## Performance targets

| Tier | Broad hardware | Resolution/settings | Target |
|---|---|---|---|
| Minimum | Older 4-core laptop, integrated GPU, 8 GB RAM | Native 720p, Low | Stable 30 FPS |
| Normal laptop | Modern integrated GPU, 8–16 GB RAM | Native 900p/1080p, Medium | Stable 45–60 FPS |
| Gaming desktop | Ryzen 7 7700, RTX 4060 Ti, 32 GB RAM | Native 1080p, High | Stable 60+ FPS |

Each tier needs frametime percentiles, CPU/GPU timing, memory, draw-call and
triangle budgets, not only an average FPS. Fog, bloom, darkness, temporal
smearing, motion blur, depth of field, or hidden resolution reduction are not
acceptable substitutes for optimization.

## Phases

1. **Audit and baseline (complete):** protect the repository, inventory the
   prototype, reproduce validation, identify placeholders and architecture
   risks, and define trustworthy native-resolution benchmark work.
2. **Foundation stabilization:** modularize without a rewrite; pin dependencies;
   restore automated checks; add deterministic fixed-step simulation,
   save/settings schemas, explicit quality presets, and reliable profiling.
3. **Core feel vertical slice:** make first- and third-person movement, camera,
   collision, combat, feedback, controller input, and accessibility coherent.
4. **Data-driven progression slice:** move items, abilities, gathering, crafting,
   equipment, mastery, quests, and encounters out of presentation code.
5. **World and economy slice:** streaming/partitioning, settlements, bank,
   market simulation, durability, transport, and content authoring pipeline.
6. **Networking spike:** authoritative fixed-step server, replication,
   persistence boundaries, parties, and security/performance tests. Do this
   before building large multiplayer-dependent systems.
7. **Social and territorial systems:** warbands, factions, territories,
   campaigns, PvE/PvP rules, and economy balancing.
8. **Content, scale, and release hardening:** original asset pipeline, broader
   world/content, hardware lab validation, soak tests, accessibility, and
   platform packaging.

## Phase 1 exit criteria

- [x] Exact repository and Git state confirmed; pre-audit checkpoint created.
- [x] Existing documentation, implementation, assets, tools, and history read.
- [x] Current runtime and software-renderer validation reproduced.
- [x] Working, broken, unfinished, and placeholder systems classified.
- [x] Architecture and performance risks prioritized.
- [x] Later work divided into bounded phases.
- [ ] Native-resolution tests on representative physical GPUs recorded.
- [ ] CPU/GPU frame timing and peak runtime memory captured on target hardware.

The two unchecked items are baseline follow-ups, not authorization to add
gameplay in Phase 1. See `PHASE1_AUDIT.md` for findings and evidence.

## Phase 2 status — honest native-resolution rendering

- [x] Default rendering is 100% manual scale with dynamic resolution off.
- [x] Low, Medium, High, and Custom graphics states are explicit.
- [x] Reduced manual scale is player-selected, visible, and persisted.
- [x] Dynamic scaling is optional, bounded, slow-adjusting, and independently
  reported without changing the saved manual scale.
- [x] Renderer identity and SwiftShader classification are visible.
- [x] Four fixed-scene native-resolution configurations were tested.
- [ ] Repeat the benchmark on representative physical GPUs; this environment
  could not create a hardware WebGL context.

See `PHASE2_REPORT.md`. Phase 2 does not include the fixed-step or broader
foundation work listed in later phases.
