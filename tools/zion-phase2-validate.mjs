import { chromium } from 'playwright';
import fs from 'fs';

const url = process.env.ZION_URL || 'http://127.0.0.1:8091/';
const output = process.env.ZION_VALIDATION_OUT || '../captures/phase2-native-resolution/validation.json';
const errors = [];
const browser = await chromium.launch({ args: ['--use-gl=swiftshader', '--no-sandbox'] });
const page = await browser.newPage({ viewport: { width: 1280, height: 720 }, deviceScaleFactor: 1 });
page.on('pageerror', error => errors.push(`PAGE: ${error.message}`));
page.on('console', message => { if (message.type() === 'error') errors.push(`CONSOLE: ${message.text()}`); });
await page.addInitScript(() => localStorage.removeItem('zion.graphics.v1'));
await page.goto(url, { waitUntil: 'networkidle', timeout: 60000 });
await page.click('#enterBtn');
await page.waitForTimeout(1200);

const defaults = await page.evaluate(() => window.__zion.graphics.diagnostics());
await page.keyboard.press('v');
await page.waitForTimeout(200);
const firstPerson = await page.evaluate(() => ({ mode: window.__zion.cameraRig.first, arms: window.__zion.camera.getObjectByName('firstPersonArms')?.visible, playerHidden: !window.__zion.player.visible }));
await page.keyboard.press('v');
await page.waitForTimeout(200);
const thirdPerson = await page.evaluate(() => ({ mode: !window.__zion.cameraRig.first, playerVisible: window.__zion.player.visible }));
await page.click('[data-panel="graphics"]');
const graphicsPanelVisible = await page.locator('#panel').evaluate(element => !element.classList.contains('hidden'));
await page.keyboard.press('Escape');
await page.keyboard.press('i');
const inventoryVisible = await page.locator('#panel').evaluate(element => !element.classList.contains('hidden'));
await page.keyboard.press('Escape');

await page.evaluate(() => window.__zion.graphics.update({ renderScale: 0.5 }));
const halfScale = await page.evaluate(() => window.__zion.graphics.diagnostics());
await page.setViewportSize({ width: 1024, height: 768 });
await page.waitForTimeout(300);
const resizedHalfScale = await page.evaluate(() => window.__zion.graphics.diagnostics());
await page.evaluate(() => window.__zion.graphics.applyPreset('Medium'));
const restoredNative = await page.evaluate(() => window.__zion.graphics.diagnostics());
await page.evaluate(() => window.__zion.graphics.update({ dynamicResolution: true, dynamicTargetFps: 60, dynamicMinScale: 0.67, dynamicMaxScale: 1 }));
await page.waitForTimeout(7500);
const dynamicActive = await page.evaluate(() => window.__zion.graphics.diagnostics());
await page.evaluate(() => window.__zion.graphics.applyPreset('Medium'));

await page.reload({ waitUntil: 'networkidle' });
await page.click('#enterBtn');
await page.waitForTimeout(500);
const persisted = await page.evaluate(() => window.__zion.graphics.diagnostics());

const result = { defaults, firstPerson, thirdPerson, graphicsPanelVisible, inventoryVisible, halfScale, resizedHalfScale, restoredNative, dynamicActive, persisted, errors };
fs.mkdirSync(new URL('.', `file://${process.cwd()}/${output}`).pathname, { recursive: true });
fs.writeFileSync(output, `${JSON.stringify(result, null, 2)}\n`);
console.log(JSON.stringify(result, null, 2));
await browser.close();
