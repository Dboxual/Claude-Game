import { chromium } from 'playwright';
const errors = [];
const browser = await chromium.launch({ args: ['--use-gl=swiftshader', '--no-sandbox'] });
const page = await browser.newPage({ viewport: { width: 1280, height: 800 } });
page.on('pageerror', e => errors.push('PAGE: ' + e.message));
page.on('console', m => { if (m.type() === 'error') errors.push('CONSOLE: ' + m.text()); });
await page.goto('http://127.0.0.1:8091/', { waitUntil: 'networkidle', timeout: 45000 });
await page.click('#enterBtn');
await page.waitForTimeout(9000);   // let the adaptive engine take at least one sample
const m = await page.evaluate(() => ({
  metrics: window.__zion?.metrics ?? null,
  hud: document.getElementById('fpsHud')?.textContent ?? '(hud empty)',
  touchCluster: !!document.querySelector('.touch-cluster'),
}));
console.log('metrics:', JSON.stringify(m.metrics));
console.log('fps hud:', m.hud);
console.log('touch cluster in DOM:', m.touchCluster);
console.log('errors:', errors.length ? errors.slice(0, 5) : 'NONE');
await browser.close();
