# Zion Development Progress

## 2026-07-20 - Foundation stability and physics polish

Completed:

- Fixed the shutdown access violation caused by reading the active zone name
  after `ZoneInstance::Shutdown()` cleared its definition pointer.
- Removed temporary crash-bisection environment switches and per-frame trace
  logging left in the zone work in progress.
- Fixed prop placement so the caller receives the sampled terrain height.
  Colliders, point lights, and interaction targets now line up vertically with
  their rendered props across every biome.
- Corrected scaled prop collider heights, removed the invisible upper half of
  broken-column colliders, added the missing altar-block collider, and fixed
  camera sphere overlap checks.
- Improved cylinder and world-border collision response: exact-center overlaps
  resolve deterministically and inward velocity is removed while tangent
  velocity is preserved for wall sliding.
- Added steep-slope rejection and based footsteps on actual distance traveled,
  preventing uphill rim climbing and footsteps while pushing into a wall.
- Guarded invalid zone-gate transition requests and made spatial-grid query
  stamp rollover safe. Travel text now retains its destination through fade-in.
- Added local obstacle avoidance to autoplay so the enlarged-zone smoke test
  can route around temple columns and exercise the reward loop again.

Verification:

- Release build passes with MSVC `/W4`.
- FP and TP 300-frame screenshot smokes pass at about 137 FPS.
- All nine zones initialize, render, and shut down cleanly.
- A 2,000-frame TP soak passes at about 143 FPS and writes valid save/settings
  JSON in an isolated data directory.
- A 3,000-frame reward soak passes at about 142 FPS, activates one shrine, and
  collects all 14 emitted anima wisps.

Next:

- Finish dedicated repeated gate-transition soak coverage and per-zone state
  persistence before marking Epic A complete.
- Add focused controller/world collision tests when a non-rendering test target
  is introduced.

Deviations: none. This session stayed within foundation and Epic A polish.

## 2026-07-20 - Output-preserving render optimization

Completed:

- Cached unchanged lighting, atmosphere, sky, and point-light uniforms instead
  of resending identical values to the graphics driver every frame.
- Cached post-processing shader locations and skipped the bloom texture sample
  when bloom is disabled.
- Frustum-culled point-light influence spheres before the forward lighting pass;
  lights that cannot affect visible fragments no longer run in the fragment
  shader.
- Added conservative 160 m terrain cull blocks. A rejected block skips 25
  per-chunk frustum tests without changing terrain LODs or visible geometry.
- Exposed the active point-light count in the F3 overlay for profiling.

Visual tradeoffs: none. Terrain density, LOD distances, view distance, bloom
quality, lighting equations, and particle budgets are unchanged.

Verification:

- Primary and isolated Release builds pass with MSVC `/W4`.
- FP and TP screenshots match the existing composition with bloom enabled.
- Bloom-disabled rendering passes without reading the unused bloom texture.
- All nine zones initialize, render, and shut down cleanly.
- A 2,000-frame TP reward soak completes with 14 anima and 1 blessing at the
  monitor's 143 FPS VSync ceiling.
