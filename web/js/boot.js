/* ============================================================================
   7T WEB — boot
   Terrain family live: mirror module (validated clean = PORT_MAP §5 stride
   assertion) → explicit layouts from the §2c census → async pipeline creation
   (each timed, boot report is cumulative) → streaming conductor → frustum
   cull → LOD0-indirect + LOD1-direct terrain draw under an interim fixed
   camera (camera/vp is next in lift order).

   Boot staging: stage 1 first pixel = clear (pre-terrain), stage 2 = first
   frame with terrain draws. SwiftShader timings are rehearsal; Jean's Chrome
   numbers are what PORT_MAP records.
   ========================================================================== */

import { createState, clearShadowMaps, D } from './state.js';
import { makeConfig, makeSignal, writeInterimCamera, writeLights } from './uniforms.js';
import { TerrainStreamer, encodeCullAndDraw } from './passes/terrain.js';

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
const PALETTE = { ink: hex('--ink'), sky: hex('--sky'), gold: hex('--gold'), orb: hex('--orb-red') };

/* --- device ----------------------------------------------------------------- */
if (!('gpu' in navigator)) fail('WebGPU not available. Open in Chrome/Edge over http(s) or localhost.');

const tBoot = performance.now();
let t0 = performance.now();
const adapter = await navigator.gpu.requestAdapter();
if (!adapter) fail('no GPU adapter');
timed('adapter request', performance.now() - t0);

const L = adapter.limits;
log(`adapter limits — storageBufs/stage: ${L.maxStorageBuffersPerShaderStage}` +
    `  storageTex/stage: ${L.maxStorageTexturesPerShaderStage}` +
    `  texArrayLayers: ${L.maxTextureArrayLayers}  bindings/group: ${L.maxBindingsPerBindGroup}`);
if (adapter.info) log(`adapter info — ${adapter.info.vendor || '?'} / ${adapter.info.architecture || '?'}`);

/* Census §2c: default limits suffice (≤8 storage/stage; 225 ≤ 256 tex layers). */
t0 = performance.now();
const device = await adapter.requestDevice();
timed('device request', performance.now() - t0);
device.lost.then((info) => console.error('[7T] device lost: ' + info.message));
device.addEventListener('uncapturederror', (e) => console.error('[7T] uncaptured error: ' + e.error.message));

/* --- surface ----------------------------------------------------------------- */
const ctx    = canvas.getContext('webgpu');
const format = navigator.gpu.getPreferredCanvasFormat();
ctx.configure({ device, format, alphaMode: 'opaque' });
log(`surface configured — format: ${format}`);

let depthTex = null;
function resize() {
  const dpr = Math.min(window.devicePixelRatio || 1, 2);
  const w = Math.max(1, Math.floor(canvas.clientWidth * dpr));
  const h = Math.max(1, Math.floor(canvas.clientHeight * dpr));
  if (canvas.width !== w || canvas.height !== h) {
    canvas.width = w; canvas.height = h;
    depthTex?.destroy();
    depthTex = device.createTexture({
      label: 'main depth', format: 'depth24plus',
      size: { width: w, height: h },
      usage: GPUTextureUsage.RENDER_ATTACHMENT,
    });
  }
}
window.addEventListener('resize', resize);
resize();

/* --- mirror module (the stride assertion) ------------------------------------ */
t0 = performance.now();
const resp = await fetch('./shaders/world.wgsl');
if (!resp.ok) fail(`mirror fetch: HTTP ${resp.status}`);
const wgsl = await resp.text();
timed('mirror fetch', performance.now() - t0, `${(wgsl.length / 1024).toFixed(0)} KiB`);

t0 = performance.now();
const worldModule = device.createShaderModule({ label: 'world.wgsl (mirror)', code: wgsl });
const compInfo = await worldModule.getCompilationInfo();
timed('module create + validate', performance.now() - t0, 'whole mirror, roster-independent');
const errors = compInfo.messages.filter((m) => m.type === 'error');
const warnings = compInfo.messages.filter((m) => m.type === 'warning');
for (const m of compInfo.messages.slice(0, 12)) {
  console[m.type === 'error' ? 'error' : 'warn'](`[7T] wgsl ${m.type} ${m.lineNum}:${m.linePos} — ${m.message}`);
}
if (errors.length || warnings.length) {
  fail(`mirror module: ${errors.length} error(s), ${warnings.length} warning(s) — 0-warning standard`);
}
log('STRIDE ASSERTION PASS — mirror validated clean (0 errors, 0 warnings)');

/* --- state + pipelines (async; creation IS the mechanical census check) ------ */
t0 = performance.now();
const R = createState(device);
timed('state create', performance.now() - t0, 'buffers/textures/layouts/bind groups');

async function computePipe(name, entryPoint, layout) {
  const t = performance.now();
  const p = await device.createComputePipelineAsync({
    label: entryPoint, layout: device.createPipelineLayout({ bindGroupLayouts: [layout] }),
    compute: { module: worldModule, entryPoint },
  });
  timed(`pipeline ${entryPoint}`, performance.now() - t);
  return p;
}
async function terrainRenderPipe(label, indirection) {
  const t = performance.now();
  const p = await device.createRenderPipelineAsync({
    label,
    layout: device.createPipelineLayout({ bindGroupLayouts: [R.renderEntityLayout, R.renderTextureLayout] }),
    vertex: {
      module: worldModule, entryPoint: 'patch_terrain_vs',
      constants: indirection ? { USE_PATCH_INDIRECTION: 1 } : {},
    },
    fragment: { module: worldModule, entryPoint: 'patch_terrain_fs', targets: [{ format }] },
    primitive: { topology: 'triangle-list', cullMode: 'back', frontFace: 'ccw' },
    depthStencil: { format: 'depth24plus', depthWriteEnabled: true, depthCompare: 'less' },
  });
  timed(`pipeline ${label}`, performance.now() - t);
  return p;
}

[R.pipeHeights, R.pipeGradients, R.pipeCells, R.pipeCull, R.pipeTerrainIndirect, R.pipeTerrainDirect] =
  await Promise.all([
    computePipe('heights', 'generate_patch_heights', R.patchGenLayout),
    computePipe('gradients', 'generate_patch_gradients', R.patchGenLayout),
    computePipe('cells', 'generate_patch_cells', R.patchGenLayout),
    computePipe('cull', 'frustum_cull_patches', R.frustumLayout),
    terrainRenderPipe('patch_terrain (indirect)', true),
    terrainRenderPipe('patch_terrain (direct LOD1)', false),
  ]);
log('terrain pipelines created — census check passed (explicit layouts validated)');

/* --- init writes -------------------------------------------------------------- */
clearShadowMaps(device, R);
const config = makeConfig(device, R.config);
const signal = makeSignal(device, R.signal, PALETTE);
R.aspect = canvas.width / Math.max(canvas.height, 1);
writeInterimCamera(device, R);
writeLights(device, R);
config.flush();

const terrain = new TerrainStreamer(device, R, config);

timed('time to first pixel — stage 1 (clear)', performance.now() - tBoot);
log('boot ok — streaming terrain…');
console.log('[7T] BOOT OK');

/* --- frame -------------------------------------------------------------------- */
let last = performance.now();
let firstTerrainFrame = true;
let streamed = 0;

function frame(now) {
  const dt = Math.min((now - last) / 1000, 0.05); last = now;
  resize();
  if (canvas.width === 0 || canvas.height === 0) { requestAnimationFrame(frame); return; }

  signal.frame(now / 1000, dt, canvas.width / Math.max(canvas.height, 1));

  const enc = device.createCommandEncoder();
  const n = terrain.encode(enc);           // stream_patches: params copy + 3 dispatches per patch
  config.flush();                          // placement_patch_count / lod_point after banding
  encodeCullAndDraw(device, R, enc,
    ctx.getCurrentTexture().createView(), depthTex.createView(),
    { r: PALETTE.ink[0], g: PALETTE.ink[1], b: PALETTE.ink[2], a: 1 },
    terrain.counts);
  device.queue.submit([enc.finish()]);

  if (n > 0) {
    streamed += n;
    if (streamed >= terrain.patches.size && terrain.pending.length === 0) {
      log(`terrain window complete — ${streamed} patches (lod0 ${terrain.counts.lod0}, ` +
          `render ${terrain.counts.render}, all ${terrain.counts.all})`);
    }
  }
  if (firstTerrainFrame && terrain.counts.render > 0) {
    firstTerrainFrame = false;
    timed('time to first pixel — stage 2 (terrain)', performance.now() - tBoot);
    console.table(bootReport);
    console.log('[7T] TERRAIN DRAWING');
  }
  requestAnimationFrame(frame);
}
requestAnimationFrame(frame);
