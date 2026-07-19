import { chromium } from 'playwright';
import fs from 'fs';
import path from 'path';

const url = process.env.ZION_URL || 'http://127.0.0.1:8091/';
const outDir = process.env.ZION_BENCHMARK_OUT || '../captures/phase2-native-resolution';
const durationMs = Number(process.env.ZION_BENCHMARK_DURATION_MS || 8000);
const requestedBackend = process.env.ZION_RENDERER || 'swiftshader';
const launchArgs = ['--no-sandbox'];
if (requestedBackend === 'swiftshader') launchArgs.push('--use-gl=swiftshader');
else launchArgs.push('--enable-gpu', '--enable-webgl', '--ignore-gpu-blocklist');

const tests = [
  { id: 'A', width: 1280, height: 720, preset: 'Low' },
  { id: 'B', width: 1600, height: 900, preset: 'Medium' },
  { id: 'C', width: 1920, height: 1080, preset: 'High' },
  { id: 'D', width: 1920, height: 1080, preset: 'Low' },
];

fs.mkdirSync(outDir, { recursive: true });
const browser = await chromium.launch({ headless: process.env.ZION_HEADLESS !== '0', args: launchArgs });
const results = [];

for (const test of tests) {
  const page = await browser.newPage({ viewport: { width: test.width, height: test.height }, deviceScaleFactor: 1 });
  const errors = [];
  page.on('pageerror', error => errors.push(`PAGE: ${error.message}`));
  page.on('console', message => { if (message.type() === 'error') errors.push(`CONSOLE: ${message.text()}`); });
  await page.addInitScript(() => localStorage.removeItem('zion.graphics.v1'));
  await page.goto(url, { waitUntil: 'networkidle', timeout: 60000 });
  await page.click('#enterBtn');
  await page.waitForFunction(() => window.__zion?.graphics, null, { timeout: 15000 });
  await page.evaluate(preset => {
    window.__zion.graphics.applyPreset(preset);
    window.__zion.setBenchmarkView();
  }, test.preset);
  await page.waitForTimeout(2500);

  const sample = await page.evaluate(async duration => {
    const frameTimes = [];
    const calls = [];
    const triangles = [];
    let previous = performance.now();
    const started = previous;
    await new Promise(resolve => {
      function frame(now) {
        frameTimes.push(now - previous);
        previous = now;
        calls.push(window.__zion.renderer.info.render.calls);
        triangles.push(window.__zion.renderer.info.render.triangles);
        if (now - started >= duration) resolve();
        else requestAnimationFrame(frame);
      }
      requestAnimationFrame(frame);
    });
    frameTimes.shift();
    const total = frameTimes.reduce((sum, value) => sum + value, 0);
    const averageFrameMs = total / frameTimes.length;
    const slowestFrameMs = Math.max(...frameTimes);
    return {
      durationMs: total,
      frames: frameTimes.length,
      averageFps: 1000 / averageFrameMs,
      minimumObservedFps: 1000 / slowestFrameMs,
      averageFrameMs,
      slowestFrameMs,
      drawCalls: Math.max(...calls),
      triangles: Math.max(...triangles),
      diagnostics: window.__zion.graphics.diagnostics(),
      firstPersonRigPresent: !!window.__zion.camera.getObjectByName('firstPersonArms'),
      thirdPersonPlayerVisible: window.__zion.player.visible,
    };
  }, durationMs);

  await page.screenshot({ path: path.join(outDir, `test-${test.id}-${test.width}x${test.height}-${test.preset.toLowerCase()}.png`) });
  results.push({ test: test.id, requestedBackend, ...test, renderScale: 1, dynamicResolution: false, ...sample, errors });
  await page.close();
}

await browser.close();
const report = { generatedAt: new Date().toISOString(), method: 'fixed Gloamwood spawn, third-person camera, 2.5 s warm-up, fixed-duration rAF sample', results };
const outputPath = path.join(outDir, 'benchmark-results.json');
fs.writeFileSync(outputPath, `${JSON.stringify(report, null, 2)}\n`);
console.log(JSON.stringify(report, null, 2));
console.log(`Saved ${outputPath}`);
