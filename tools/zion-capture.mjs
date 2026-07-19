// Gameplay capture: screenshots + video of the current build.
// Runs software WebGL (SwiftShader) — slower than a real GPU, but faithful.
import { chromium } from 'playwright';
import fs from 'fs';

const OUT = process.env.OUT || '/tmp/zion-caps';
fs.mkdirSync(OUT, { recursive: true });
const errors = [];

const browser = await chromium.launch({ args: ['--use-gl=swiftshader', '--no-sandbox'] });
const ctx = await browser.newContext({
  viewport: { width: 1280, height: 720 },
  recordVideo: { dir: OUT, size: { width: 1280, height: 720 } },
});
const page = await ctx.newPage();
page.on('pageerror', e => errors.push('PAGE: ' + e.message));
page.on('console', m => { if (m.type() === 'error') errors.push('CONSOLE: ' + m.text()); });

await page.goto('http://127.0.0.1:8091/', { waitUntil: 'networkidle', timeout: 60000 });
await page.screenshot({ path: `${OUT}/01-boot.png` });
await page.click('#enterBtn');
await page.waitForTimeout(4000);
await page.screenshot({ path: `${OUT}/02-spawn.png` });

const key = async (k, ms) => { await page.keyboard.down(k); await page.waitForTimeout(ms); await page.keyboard.up(k); };

// walk the route — forward through the forest
await key('w', 2600);
await page.screenshot({ path: `${OUT}/03-forest.png` });

// orbit the camera for a wide look
await page.mouse.move(640, 360);
await page.mouse.down({ button: 'right' });
await page.mouse.move(880, 330, { steps: 12 });
await page.mouse.up({ button: 'right' });
await page.waitForTimeout(400);
await page.screenshot({ path: `${OUT}/04-vista.png` });

// keep moving, try to find action; lock-on + ability
await key('w', 2200);
await key('a', 900);
await key('w', 1800);
await page.keyboard.press('q');           // lock-on if anything is near
await page.waitForTimeout(300);
await page.keyboard.press('1');           // Riven Arc
await page.waitForTimeout(700);
await page.screenshot({ path: `${OUT}/05-combat.png` });
await page.keyboard.press('2');           // Hero's Rush
await page.waitForTimeout(900);
await page.screenshot({ path: `${OUT}/06-action.png` });

// menus
await page.keyboard.press('i');
await page.waitForTimeout(600);
await page.screenshot({ path: `${OUT}/07-inventory.png` });
await page.keyboard.press('Escape');
await page.keyboard.press('v');           // first person
await page.waitForTimeout(500);
await key('w', 1500);
await page.screenshot({ path: `${OUT}/08-firstperson.png` });
await page.keyboard.press('v');
await key('w', 1200);

const m = await page.evaluate(() => ({
  metrics: window.__zion?.metrics ?? null,
  hud: document.getElementById('fpsHud')?.textContent ?? '(empty)',
}));
console.log('metrics:', JSON.stringify(m.metrics));
console.log('hud:', m.hud);
console.log('errors:', errors.length ? errors.slice(0, 6) : 'NONE');

await ctx.close();                         // finalizes the video
const vid = fs.readdirSync(OUT).find(f => f.endsWith('.webm'));
if (vid) fs.renameSync(`${OUT}/${vid}`, `${OUT}/zion-gameplay.webm`);
await browser.close();
console.log('captures in', OUT);
