const STORAGE_KEY = 'zion.graphics.v1';
const SCALE_CHOICES = [0.5, 0.67, 0.75, 0.85, 1];

export const GRAPHICS_PRESETS = {
  Low: { renderScale: 1, bloom: false, foliage: 0.55, particles: 36, viewDistance: 20, extraLights: false },
  Medium: { renderScale: 1, bloom: false, foliage: 0.8, particles: 54, viewDistance: 24, extraLights: true },
  High: { renderScale: 1, bloom: true, foliage: 1, particles: 72, viewDistance: 30, extraLights: true },
};

const DEFAULTS = {
  preset: 'Medium',
  basePreset: 'Medium',
  renderScale: 1,
  dynamicResolution: false,
  dynamicTargetFps: 45,
  dynamicMinScale: 0.67,
  dynamicMaxScale: 1,
};

const clamp = (value, min, max) => Math.max(min, Math.min(max, value));
const validScale = value => SCALE_CHOICES.includes(Number(value));

function loadSettings() {
  try {
    const saved = JSON.parse(localStorage.getItem(STORAGE_KEY) || '{}');
    const settings = { ...DEFAULTS, ...saved };
    if (!GRAPHICS_PRESETS[settings.preset] && settings.preset !== 'Custom') settings.preset = DEFAULTS.preset;
    if (!GRAPHICS_PRESETS[settings.basePreset]) settings.basePreset = settings.preset === 'Custom' ? DEFAULTS.basePreset : settings.preset;
    if (!validScale(settings.renderScale)) settings.renderScale = DEFAULTS.renderScale;
    settings.dynamicResolution = settings.dynamicResolution === true;
    settings.dynamicTargetFps = clamp(Number(settings.dynamicTargetFps) || 45, 30, 60);
    settings.dynamicMinScale = clamp(Number(settings.dynamicMinScale) || 0.67, 0.5, 1);
    settings.dynamicMaxScale = clamp(Number(settings.dynamicMaxScale) || 1, settings.dynamicMinScale, 1);
    return settings;
  } catch {
    return { ...DEFAULTS };
  }
}

function classifyRenderer(renderer, vendor) {
  const combined = `${renderer} ${vendor}`.toLowerCase();
  if (combined.includes('swiftshader') || combined.includes('llvmpipe') || combined.includes('software')) return 'CPU software renderer';
  if (!renderer || /webkit webgl|angle|unknown/i.test(renderer.trim())) return 'Unknown or privacy-restricted renderer';
  if (/nvidia|geforce|radeon rx|arc\(tm\)|intel arc/i.test(combined)) return 'Dedicated GPU (renderer-reported)';
  if (/intel|iris|uhd|vega|radeon 780m|radeon graphics|apple/i.test(combined)) return 'Integrated GPU (renderer-reported)';
  return 'Unknown GPU class';
}

export function createGraphicsController({ renderer, composer, bloom, canvas, onChange }) {
  const settings = loadSettings();
  let dynamicScale = 1;
  let allocationScale = 1;
  let frameCount = 0;
  let elapsed = 0;
  let lastAdjustment = 0;
  let measuredFps = 0;
  let effects = { ...GRAPHICS_PRESETS[settings.basePreset] };

  function save() {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(settings));
  }

  function desiredPixelRatio() {
    return devicePixelRatio * settings.renderScale * (settings.dynamicResolution ? dynamicScale : 1);
  }

  function applyResolution() {
    const desired = desiredPixelRatio();
    const maxDimension = renderer.capabilities.maxTextureSize || 8192;
    const cssWidth = Math.max(1, innerWidth);
    const cssHeight = Math.max(1, innerHeight);
    allocationScale = Math.min(1, maxDimension / Math.max(cssWidth * desired, cssHeight * desired));
    renderer.setPixelRatio(desired * allocationScale);
    renderer.setSize(cssWidth, cssHeight, false);
    composer.setPixelRatio(desired * allocationScale);
    composer.setSize(cssWidth, cssHeight);
  }

  function applyEffects() {
    bloom.enabled = effects.bloom;
    onChange?.(effects, settings);
  }

  function applyPreset(name, persist = true) {
    if (!GRAPHICS_PRESETS[name]) return;
    settings.preset = name;
    settings.basePreset = name;
    effects = { ...GRAPHICS_PRESETS[name] };
    settings.renderScale = effects.renderScale;
    dynamicScale = 1;
    applyResolution();
    applyEffects();
    if (persist) save();
  }

  function markCustom() {
    if (settings.preset !== 'Custom') settings.preset = 'Custom';
  }

  function update(partial, persist = true) {
    if ('preset' in partial && GRAPHICS_PRESETS[partial.preset]) {
      applyPreset(partial.preset, persist);
      return;
    }
    if ('renderScale' in partial && validScale(partial.renderScale)) settings.renderScale = Number(partial.renderScale);
    if ('dynamicResolution' in partial) settings.dynamicResolution = partial.dynamicResolution === true;
    if ('dynamicTargetFps' in partial) settings.dynamicTargetFps = clamp(Number(partial.dynamicTargetFps), 30, 60);
    if ('dynamicMinScale' in partial) settings.dynamicMinScale = clamp(Number(partial.dynamicMinScale), 0.5, 1);
    if ('dynamicMaxScale' in partial) settings.dynamicMaxScale = clamp(Number(partial.dynamicMaxScale), settings.dynamicMinScale, 1);
    if (settings.dynamicMinScale > settings.dynamicMaxScale) settings.dynamicMinScale = settings.dynamicMaxScale;
    markCustom();
    if (!settings.dynamicResolution) dynamicScale = 1;
    applyResolution();
    applyEffects();
    if (persist) save();
  }

  function sample(dt, now) {
    frameCount++;
    elapsed += dt;
    if (elapsed < 2) return;
    measuredFps = frameCount / elapsed;
    frameCount = 0;
    elapsed = 0;
    if (!settings.dynamicResolution || now - lastAdjustment < 3000) return;
    const target = settings.dynamicTargetFps;
    let next = dynamicScale;
    if (measuredFps < target - 4) next = Math.max(settings.dynamicMinScale, dynamicScale - 0.05);
    else if (measuredFps > target + 8) next = Math.min(settings.dynamicMaxScale, dynamicScale + 0.05);
    if (next !== dynamicScale) {
      dynamicScale = next;
      applyResolution();
      lastAdjustment = now;
    }
  }

  function diagnostics() {
    const gl = renderer.getContext();
    const debug = gl.getExtension('WEBGL_debug_renderer_info');
    const vendor = gl.getParameter(gl.VENDOR) || '';
    const rendererName = gl.getParameter(gl.RENDERER) || '';
    const unmaskedVendor = debug ? gl.getParameter(debug.UNMASKED_VENDOR_WEBGL) : '';
    const unmaskedRenderer = debug ? gl.getParameter(debug.UNMASKED_RENDERER_WEBGL) : '';
    const bestRenderer = unmaskedRenderer || rendererName;
    const bestVendor = unmaskedVendor || vendor;
    const rect = canvas.getBoundingClientRect();
    const effectiveScale = settings.renderScale * (settings.dynamicResolution ? dynamicScale : 1) * allocationScale;
    return {
      webglVersion: gl.getParameter(gl.VERSION),
      renderer: rendererName || 'Unknown',
      vendor: vendor || 'Unknown',
      unmaskedRenderer: unmaskedRenderer || 'Unavailable (privacy-restricted)',
      unmaskedVendor: unmaskedVendor || 'Unavailable (privacy-restricted)',
      rendererClass: classifyRenderer(bestRenderer, bestVendor),
      swiftShader: /swiftshader/i.test(`${bestRenderer} ${bestVendor}`),
      canvasCssResolution: `${Math.round(rect.width)}x${Math.round(rect.height)}`,
      drawingBufferResolution: `${gl.drawingBufferWidth}x${gl.drawingBufferHeight}`,
      viewportResolution: `${innerWidth}x${innerHeight}`,
      devicePixelRatio,
      preset: settings.preset,
      manualRenderScale: settings.renderScale,
      dynamicResolution: settings.dynamicResolution,
      dynamicScale: settings.dynamicResolution ? dynamicScale : 1,
      effectiveScale,
      allocationClamped: allocationScale < 1,
      measuredFps,
    };
  }

  effects = { ...GRAPHICS_PRESETS[settings.basePreset] };
  applyResolution();
  applyEffects();

  return { settings, get effects() { return effects; }, applyPreset, update, applyResolution, sample, diagnostics };
}
