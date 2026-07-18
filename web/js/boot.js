/* ============================================================================
   7T WEB — boot
   Phase 1 opener (terrain family, step 0): device acquisition, mirror module
   compile, boot report. Compiling the full mirror IS the PORT_MAP §5
   binding-0 array-stride assertion — WGSL address-space layout rules are
   validated at createShaderModule time, so a clean compile here proves
   FrameSignal.stats: array<f32,64> under var<uniform> is accepted by Chrome.

   Boot-cost model (PORT_MAP §5): module create = whole-module parse/validate,
   roster-independent; pipeline creation = per-entry-point backend compile,
   roster-scoped, async — timed separately as families land.
   ========================================================================== */

const statusEl = document.getElementById('status');
const canvas   = document.getElementById('stage');

const statusLines = [];
function show(line) {
  statusLines.push(line);
  statusEl.textContent = statusLines.slice(-28).join('\n');
}
function log(line) { console.log('[7T] ' + line); show(line); }
function fail(line) {
  console.error('[7T] BOOT FAIL — ' + line);
  show('BOOT FAIL — ' + line);
  throw new Error(line);
}

/* --- boot report ------------------------------------------------------------ */
const bootReport = [];
function timed(step, ms, note = '') {
  bootReport.push({ step, ms: +ms.toFixed(1), note });
  log(`${step}: ${ms.toFixed(1)} ms${note ? '  (' + note + ')' : ''}`);
}

/* --- palette tier → JS ------------------------------------------------------ */
const css = getComputedStyle(document.documentElement);
const hex = (name) => {
  const h = css.getPropertyValue(name).trim().replace('#', '');
  return [parseInt(h.slice(0, 2), 16) / 255, parseInt(h.slice(2, 4), 16) / 255, parseInt(h.slice(4, 6), 16) / 255];
};
const PALETTE = {
  ink:  hex('--ink'),
  sky:  hex('--sky'),
  gold: hex('--gold'),
  orb:  hex('--orb-red'),
};

/* --- device ----------------------------------------------------------------- */
if (!('gpu' in navigator)) {
  fail('WebGPU not available. Open in Chrome/Edge over http(s) or localhost.');
}

const tBoot = performance.now();

let t0 = performance.now();
const adapter = await navigator.gpu.requestAdapter();
if (!adapter) fail('no GPU adapter');
timed('adapter request', performance.now() - t0);

const L = adapter.limits;
log(`adapter limits — storageBufs/stage: ${L.maxStorageBuffersPerShaderStage}` +
    `  storageTex/stage: ${L.maxStorageTexturesPerShaderStage}` +
    `  uniformBufs/stage: ${L.maxUniformBuffersPerShaderStage}` +
    `  bindings/group: ${L.maxBindingsPerBindGroup}`);
if (adapter.info) {
  log(`adapter info — ${adapter.info.vendor || '?'} / ${adapter.info.architecture || '?'}` +
      `${adapter.info.description ? ' / ' + adapter.info.description : ''}`);
}

/* Census §2c: every entry point ≤8 storage buffers per stage by static usage,
   so the device request is DEFAULT limits — no raise, no TODO(portability). */
t0 = performance.now();
const device = await adapter.requestDevice();
timed('device request', performance.now() - t0);

device.lost.then((info) => console.error('[7T] device lost: ' + info.message));
device.addEventListener('uncapturederror', (e) =>
  console.error('[7T] uncaptured error: ' + e.error.message));

/* --- surface ---------------------------------------------------------------- */
const ctx    = canvas.getContext('webgpu');
const format = navigator.gpu.getPreferredCanvasFormat();
ctx.configure({ device, format, alphaMode: 'opaque' });
log(`surface configured — format: ${format}`);

function resize() {
  const dpr = Math.min(window.devicePixelRatio || 1, 2);
  const w = Math.max(1, Math.floor(canvas.clientWidth  * dpr));
  const h = Math.max(1, Math.floor(canvas.clientHeight * dpr));
  if (canvas.width !== w || canvas.height !== h) { canvas.width = w; canvas.height = h; }
}
window.addEventListener('resize', resize);
resize();

/* --- mirror fetch + module compile (the smoke assertion) -------------------- */
t0 = performance.now();
const resp = await fetch('./shaders/world.wgsl');
if (!resp.ok) fail(`mirror fetch: HTTP ${resp.status}`);
const wgsl = await resp.text();
timed('mirror fetch', performance.now() - t0, `${(wgsl.length / 1024).toFixed(0)} KiB`);

t0 = performance.now();
const worldModule = device.createShaderModule({ label: 'world.wgsl (mirror)', code: wgsl });
const compInfo = await worldModule.getCompilationInfo();
timed('module create + validate', performance.now() - t0, 'whole mirror, roster-independent');

const errors   = compInfo.messages.filter((m) => m.type === 'error');
const warnings = compInfo.messages.filter((m) => m.type === 'warning');
for (const m of compInfo.messages.slice(0, 12)) {
  console[m.type === 'error' ? 'error' : 'warn'](
    `[7T] wgsl ${m.type} ${m.lineNum}:${m.linePos} — ${m.message}`);
}
if (errors.length) {
  fail(`mirror module has ${errors.length} WGSL error(s) — stride assertion FAILED, ` +
       'see PORT_MAP §5 fallback (flip binding 0 to read-only storage, upstream)');
}
log(`STRIDE ASSERTION PASS — mirror validated clean ` +
    `(${errors.length} errors, ${warnings.length} warnings)`);

/* ======================= TERRAIN FAMILY LANDS HERE =========================
   Next steps in lift order (PORT_MAP §0): terrain layouts from the §2c dump →
   patch-gen pipelines → streaming conductor → patch_terrain draw, then
   camera/vp. Until the terrain draw exists, the frame below is a clear pass
   in --ink so the page is runnable and the surface chain is proven.
   ========================================================================== */

timed('time to first pixel (clear)', performance.now() - tBoot);
console.table(bootReport);
log('boot ok — terrain family lands next');
console.log('[7T] BOOT OK');

function frame() {
  resize();
  if (canvas.width === 0 || canvas.height === 0) { requestAnimationFrame(frame); return; }
  const enc = device.createCommandEncoder();
  const pass = enc.beginRenderPass({
    colorAttachments: [{
      view: ctx.getCurrentTexture().createView(),
      clearValue: { r: PALETTE.ink[0], g: PALETTE.ink[1], b: PALETTE.ink[2], a: 1 },
      loadOp: 'clear', storeOp: 'store',
    }],
  });
  pass.end();
  device.queue.submit([enc.finish()]);
  requestAnimationFrame(frame);
}
requestAnimationFrame(frame);
