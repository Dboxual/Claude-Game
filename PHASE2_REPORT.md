# Zion Phase 2: honest native-resolution rendering

Date: 2026-07-19 UTC

## Outcome

Zion now defaults to native configured resolution. Dynamic resolution is off,
manual render scale is 100%, and reduced resolution cannot occur unless the
player explicitly selects it or enables the clearly reported dynamic option.
Low-end presets reduce actual scene workload rather than lowering resolution.

## Original blur root cause

The old adaptive-quality block calculated the desktop ceiling as
`devicePixelRatio × 0.6` and passed that value to `renderer.setPixelRatio()`.
With a 1440×900 viewport and DPR 1, the drawing buffer was therefore
`1440 × 0.6` by `900 × 0.6`, or 864×540. Touch started at no more than 60% and
could use a 50% ceiling. Automatic FPS logic could lower the value further to
30%. This was a previous optimization policy, not SwiftShader detection, a
preset, or browser automation.

## Resolution model

The concepts are now separate and reported:

1. Browser viewport (`innerWidth`/`innerHeight`).
2. Canvas CSS/display size in CSS pixels.
3. WebGL drawing-buffer size.
4. Browser device pixel ratio.
5. Saved manual render scale.
6. Temporary optional dynamic multiplier.

Formula:

```text
drawing buffer = CSS canvas size × DPR × manual scale × dynamic multiplier
```

The dynamic multiplier is exactly 1 when disabled. A final allocation clamp is
applied only when the requested dimension would exceed WebGL
`MAX_TEXTURE_SIZE`; diagnostics expose `allocationClamped` and effective scale.
The HTML/CSS HUD is independent of the WebGL drawing buffer and stays sharp.

Before: 1440×900 at DPR 1 defaulted to 864×540 (60%).

After: 1440×900 at DPR 1 defaults to 1440×900 (100%). A deliberate 50%
selection produces 720×450 and is labeled in settings and diagnostics.

## Settings and presets

Settings are stored under `localStorage` key `zion.graphics.v1`.

| Preset | Scale | Bloom | Foliage | Particles | Enemy view | Point lights |
|---|---:|---|---:|---:|---:|---|
| Low | 100% | Off | 55% | 36 | 20 units | Reduced |
| Medium | 100% | Off | 80% | 54 | 24 units | On |
| High | 100% | On | 100% | 72 | 30 units | On |
| Custom | Explicit choice | Retains selected workload base | — | — | — | — |

Manual scale choices are 50%, 67%, 75%, 85%, and 100%. The control warns:
“Lower render scale improves performance but reduces image sharpness.” Changing
an individual setting activates Custom without losing the underlying workload
preset.

Dynamic resolution defaults off. When enabled it has a 30/45/60 FPS target,
50–100% configurable bounds, two-second samples, a three-second adjustment
cooldown, asymmetric hysteresis, and slow five-percentage-point steps. It
multiplies but never edits the saved manual scale.

## Renderer detection

The Graphics panel and benchmark report WebGL version, masked and unmasked
vendor/renderer strings when available, viewport, CSS canvas, drawing buffer,
DPR, preset, manual/dynamic/effective scale, and allocation clamp state.

Software strings including SwiftShader, llvmpipe, or software are classified as
CPU software rendering. Recognizable integrated or dedicated renderer strings
are classified with the qualifier “renderer-reported.” If only generic masked
strings are available, the class is unknown/privacy-restricted. Zion never
substitutes the host PCI device name for the browser's active renderer.

## Benchmark method

`tools/zion-benchmark.mjs` opens a fresh page for each configuration, clears
graphics settings, selects the preset, fixes the player at `(0, -18)`, fixes the
third-person camera direction and distance, waits 2.5 seconds, then samples the
same scene for eight seconds. It records average/minimum observed FPS, average
and slowest frame, peak draw calls/triangles, complete diagnostics, settings,
camera checks, and browser errors. Screenshots and JSON are stored together in
`captures/phase2-native-resolution/`.

## Results

All results below are native drawing-buffer resolution, DPR 1, manual scale
100%, dynamic resolution off, with no console/page errors.

| Test | Configuration | Average FPS | Minimum observed | Avg frame | Slowest frame | Calls | Triangles |
|---|---|---:|---:|---:|---:|---:|---:|
| A | 1280×720 Low | 41.08 | 29.85 | 24.34 ms | 33.50 ms | 77 | 34,249 |
| B | 1600×900 Medium | 24.45 | 19.96 | 40.90 ms | 50.10 ms | 91 | 35,374 |
| C | 1920×1080 High | 7.42 | 6.66 | 134.72 ms | 150.10 ms | 105 | 35,856 |
| D | 1920×1080 Low | 22.16 | 19.96 | 45.13 ms | 50.10 ms | 77 | 34,249 |

Renderer for every accepted result:

- WebGL 2.0 through ANGLE Vulkan.
- Unmasked renderer: Google SwiftShader Device (Subzero).
- Classification: CPU software renderer.
- Physical GPU result: **not available**.

The host exposes an AMD Radeon 780M PCI device, but the physical-GPU Chromium
attempt failed to create a WebGL context and reported WebGL disabled. These
numbers must not be treated as Radeon 780M or target-tier performance. The very
low High result largely reflects full-resolution bloom running in software and
is honest evidence of that test environment.

## Validation

- Default 1280×720 CSS canvas produced a 1280×720 drawing buffer.
- Explicit 50% produced 640×360 while the CSS UI stayed 1280×720.
- Resizing to 1024×768 at 50% produced 512×384; returning to Medium restored
  1024×768 native rendering.
- Graphics settings survived reload.
- First person: mode toggled, arms/weapon visible, third-person actor hidden.
- Third person: actor visible after switching back.
- Inventory and Graphics panels opened; HUD, reticle, text, and menus remained
  CSS-rendered and readable.
- Mouse/touch handlers, camera collision code, interaction prompts, and gameplay
  systems were left intact.
- Dynamic resolution activation and bounds are covered by the Phase 2 validator.
- JavaScript syntax checks and browser validation completed without errors.

## Files changed

- `graphics-settings.js`: settings, presets, resolution calculation, dynamic
  controller, persistence, and renderer diagnostics.
- `game3d.js`: controller integration, workload presets, HUD, Graphics panel,
  renderer counters, resize/fullscreen behavior, and benchmark view hook.
- `index.html`, `styles.css`: Graphics entry point and focused settings styles.
- `tools/zion-benchmark.mjs`: repeatable four-case benchmark.
- `tools/zion-phase2-validate.mjs`: camera, UI, persistence, resize, and scale
  regression checks.
- `captures/phase2-native-resolution/`: results, validation, and four images.
- `README.md`, `GOALS.md`, `PROGRESS.md`, `PHASE2_REPORT.md`: established
  documentation and changelog updates.

## Remaining risks and next small slice

- Physical toaster/iGPU/desktop measurements are still required. Run the same
  harness in a graphical session with `ZION_RENDERER=hardware ZION_HEADLESS=0`
  and accept results only when diagnostics identify non-software rendering.
- Presets currently control only costs the prototype exposes; there are no real
  shadow maps, LOD models, texture tiers, or complex water settings to tune.
- High bloom is disproportionately expensive in software and needs physical-GPU
  evidence before deciding whether its current implementation is appropriate.
- Browser storage can be cleared by the user and is not a general game-save
  system; broader persistence remains out of scope.

Recommended next slice: run the unchanged benchmark on representative physical
hardware, then make only evidence-driven workload adjustments to reach the
native-resolution tier budgets. Do not begin fixed timestep, networking, or new
gameplay as part of that measurement slice.
