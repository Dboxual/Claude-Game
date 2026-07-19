import { chromium } from 'playwright';

const url = process.env.ZION_URL || 'http://host.docker.internal:8080';
const angle = process.env.ZION_ANGLE || 'gl';
const glBackend = process.env.ZION_GL || 'angle';
const requireHardware = process.env.ZION_REQUIRE_HARDWARE !== '0';
const headless = process.env.ZION_HEADLESS !== '0';
const browser = await chromium.launch({
  headless,
  args: [
    '--enable-gpu',
    '--enable-webgl',
    '--ignore-gpu-blocklist',
    ...(requireHardware ? ['--disable-software-rasterizer'] : []),
    `--use-gl=${glBackend}`,
    `--use-angle=${angle}`,
  ],
});

const page = await browser.newPage({ viewport: { width: 1440, height: 900 }, deviceScaleFactor: 1 });
await page.goto(url, { waitUntil: 'networkidle' });
await page.click('#enterBtn');
await page.waitForTimeout(1500);
const webgl = await page.evaluate(() => {
  const canvas = document.querySelector('#world');
  const gl = canvas.getContext('webgl2') || canvas.getContext('webgl');
  if (!gl) return { error: 'WebGL context creation failed' };
  const debug = gl.getExtension('WEBGL_debug_renderer_info');
  const parameter = (name) => gl.getParameter(gl[name]);
  return {
    vendor: parameter('VENDOR'),
    renderer: parameter('RENDERER'),
    unmaskedVendor: debug ? gl.getParameter(debug.UNMASKED_VENDOR_WEBGL) : null,
    unmaskedRenderer: debug ? gl.getParameter(debug.UNMASKED_RENDERER_WEBGL) : null,
    version: parameter('VERSION'),
    glslVersion: parameter('SHADING_LANGUAGE_VERSION'),
    maxTextureSize: parameter('MAX_TEXTURE_SIZE'),
    maxRenderbufferSize: parameter('MAX_RENDERBUFFER_SIZE'),
    maxVertexTextureUnits: parameter('MAX_VERTEX_TEXTURE_IMAGE_UNITS'),
    rendererPixelRatio: window.__zion?.renderer?.getPixelRatio() ?? null,
    canvasInternalResolution: `${canvas.width}x${canvas.height}`,
    visibleBrowserResolution: `${innerWidth}x${innerHeight}`,
    devicePixelRatio,
  };
});

const cdp = await browser.newBrowserCDPSession();
const systemInfo = await cdp.send('SystemInfo.getInfo');
console.log(JSON.stringify({ angle, webgl, gpu: systemInfo.gpu }, null, 2));
await browser.close();
