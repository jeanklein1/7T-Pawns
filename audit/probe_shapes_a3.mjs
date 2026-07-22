// AUDIT-3 [A3-5] — Stage-6/7 shape probes (throwaway WGSL, witness Dawn).
// (i)  two-pass card writer (the bake's scratch pattern at card size)
// (ii) true-band delta kernel (bounded band×node loops, frozen↔moving mix)
// (iii) contact gather (33-slot uniform-bounded pair loop, capped spring)
// Witness is Tint/Vulkan (SwiftShader) — the FXC word stays with Jean.
import { createRequire } from 'node:module';
import http from 'node:http';
const require = createRequire(import.meta.url);
let chromium;
try { ({ chromium } = require('playwright-core')); } catch { ({ chromium } = require('playwright')); }

const TWO_PASS = `
const CARD_N: u32 = 512u;
struct Cfg { origin: vec2<f32>, texel: f32, t: f32 }
@group(0) @binding(0) var<uniform> cfg: Cfg;
@group(0) @binding(1) var<storage, read_write> scratch: array<f32>;   // 512*512 heights
@group(0) @binding(2) var card_out: texture_storage_2d<rgba16float, write>;

fn band_delta_at(p: vec2<f32>, t: f32) -> f32 {
  var h = 0.0;
  for (var b: u32 = 0u; b < 4u; b++) {
    let f = 0.03 * pow(2.5, f32(b));
    let ph = dot(p, vec2(f, f * 0.7));
    h += (sin(ph + t * (0.2 + 0.1 * f32(b))) - sin(ph)) * (8.0 / pow(2.0, f32(b)));
  }
  return h;
}

@compute @workgroup_size(8, 8, 1)
fn probe_pass1_heights(@builtin(global_invocation_id) gid: vec3<u32>) {
  if (gid.x >= CARD_N || gid.y >= CARD_N) { return; }
  let p = cfg.origin + (vec2(f32(gid.x), f32(gid.y)) + vec2(0.5)) * cfg.texel;
  scratch[gid.y * CARD_N + gid.x] = band_delta_at(p, cfg.t);
}

@compute @workgroup_size(8, 8, 1)
fn probe_pass2_gradients(@builtin(global_invocation_id) gid: vec3<u32>) {
  if (gid.x >= CARD_N || gid.y >= CARD_N) { return; }
  let xm = max(gid.x, 1u) - 1u;  let xp = min(gid.x + 1u, CARD_N - 1u);
  let ym = max(gid.y, 1u) - 1u;  let yp = min(gid.y + 1u, CARD_N - 1u);
  let h  = scratch[gid.y * CARD_N + gid.x];
  let gx = (scratch[gid.y * CARD_N + xp] - scratch[gid.y * CARD_N + xm]) / (2.0 * cfg.texel);
  let gz = (scratch[yp * CARD_N + gid.x] - scratch[ym * CARD_N + gid.x]) / (2.0 * cfg.texel);
  textureStore(card_out, vec2<i32>(gid.xy), vec4(h, gx, gz, 0.0));
}`;

const BAND_DELTA = `
// bounded band x 3x3-node loops, frozen & moving phases, band_act mix —
// the lattice-wave evaluator's loop shape, miniaturized.
struct Cfg { seed: f32, t_beats: f32, act0: f32, act1: f32, act2: f32, act3: f32 }
@group(0) @binding(0) var<uniform> cfg: Cfg;
@group(0) @binding(1) var<storage, read_write> out_h: array<f32>;

fn hash2(p: vec2<f32>, s: f32) -> f32 {
  return fract(sin(dot(p, vec2(127.1, 311.7)) + s) * 43758.5453);
}

@compute @workgroup_size(64, 1, 1)
fn probe_band_delta(@builtin(global_invocation_id) gid: vec3<u32>) {
  let p = vec2(f32(gid.x % 512u), f32(gid.x / 512u)) * 1.5625;
  var acts = array<f32, 4>(cfg.act0, cfg.act1, cfg.act2, cfg.act3);
  var delta = 0.0;
  for (var b: u32 = 0u; b < 4u; b++) {
    let spacing = 200.0 / pow(2.5, f32(b));
    let node = floor(p / spacing);
    var band_sum = 0.0;
    for (var nz: i32 = -1; nz <= 1; nz++) {
      for (var nx: i32 = -1; nx <= 1; nx++) {
        let n = node + vec2(f32(nx), f32(nz));
        let nw = (n + vec2(0.5)) * spacing;
        let r = hash2(n, cfg.seed + f32(b));
        let freq = (0.03 + r * 0.02) * pow(2.5, f32(b));
        let amp = (8.0 + r * 3.0) / pow(2.0, f32(b));
        let d = normalize(vec2(cos(r * 6.283), sin(r * 6.283)));
        let phase_base = freq * dot(d, p - nw) + r * 6.283;
        let phase_moving = phase_base + cfg.t_beats * (0.2 + 0.1 * f32(b)) * 6.283;
        let damp = exp(-length(p - nw) * 0.004 * pow(2.0, f32(b)));
        let frozen = sin(phase_base) * amp * damp;
        let moving = sin(phase_moving) * amp * damp;
        band_sum += moving - frozen;
      }
    }
    delta += band_sum * acts[b];
  }
  out_h[gid.x] = delta;
}`;

const CONTACT = `
// flock2d's bounded-loop shape with a tier-table radius source:
// 33-slot pair loop, distance test, capped spring accumulate.
struct Agent { pos: vec3<f32>, tier: u32, vel: vec3<f32>, alive: u32 }
struct Tier { contact_radius: f32, mass: f32, _p0: f32, _p1: f32 }
@group(0) @binding(0) var<storage, read_write> agents: array<Agent, 33>;
@group(0) @binding(1) var<uniform> tiers: array<Tier, 4>;

@compute @workgroup_size(33, 1, 1)
fn probe_contact_gather(@builtin(global_invocation_id) gid: vec3<u32>) {
  let self_idx = gid.x;
  var a = agents[self_idx];
  if (a.alive == 0u) { return; }
  let my = tiers[min(a.tier, 3u)];
  var push = vec2(0.0);
  for (var j: u32 = 0u; j < 33u; j++) {
    if (j == self_idx) { continue; }
    let b = agents[j];
    if (b.alive == 0u) { continue; }
    let tb = tiers[min(b.tier, 3u)];
    let r = my.contact_radius + tb.contact_radius;
    let d = a.pos.xz - b.pos.xz;
    let dist = length(d);
    if (dist < r && dist > 0.0001) {
      let pen = (r - dist) / r;
      let w = tb.mass / (my.mass + tb.mass);
      push += (d / dist) * min(pen * pen * 12.0, 6.0) * w;
    }
  }
  a.vel = vec3(a.vel.x + push.x, a.vel.y, a.vel.z + push.y);
  agents[self_idx] = a;
}`;

const server = http.createServer((_q, r) => { r.setHeader('content-type', 'text/html'); r.end('<!doctype html><title>p</title>ok'); });
await new Promise(r => server.listen(0, '127.0.0.1', r));
const browser = await chromium.launch({
  executablePath: '/opt/pw-browsers/chromium',
  args: ['--no-sandbox', '--enable-unsafe-webgpu', '--enable-features=Vulkan',
         '--use-webgpu-adapter=swiftshader', '--disable-gpu-sandbox'],
});
const page = await browser.newPage();
await page.goto(`http://127.0.0.1:${server.address().port}/`);

const result = await page.evaluate(async ({ probes }) => {
  const out = {};
  const adapter = await navigator.gpu.requestAdapter();
  const device = await adapter.requestDevice();
  for (const [name, spec] of Object.entries(probes)) {
    const r = {};
    let t = performance.now();
    const m = device.createShaderModule({ code: spec.code });
    const info = await m.getCompilationInfo();
    r.module_ms = +(performance.now() - t).toFixed(1);
    r.messages = info.messages.map(x => `${x.type} L${x.lineNum}: ${x.message}`);
    try {
      const bgl = device.createBindGroupLayout({ entries: spec.entries });
      const plo = device.createPipelineLayout({ bindGroupLayouts: [bgl] });
      r.pipelines = {};
      for (const ep of spec.eps) {
        t = performance.now();
        await device.createComputePipelineAsync({ layout: plo, compute: { module: m, entryPoint: ep } });
        r.pipelines[ep] = +(performance.now() - t).toFixed(1);
      }
      r.ok = true;
    } catch (e) { r.ok = false; r.error = e.message ?? String(e); }
    out[name] = r;
  }
  return out;
}, { probes: {
  two_pass_card_writer: { code: TWO_PASS, eps: ['probe_pass1_heights', 'probe_pass2_gradients'],
    entries: [
      { binding: 0, visibility: 4, buffer: { type: 'uniform' } },
      { binding: 1, visibility: 4, buffer: { type: 'storage' } },
      { binding: 2, visibility: 4, storageTexture: { access: 'write-only', format: 'rgba16float', viewDimension: '2d' } },
    ] },
  true_band_delta: { code: BAND_DELTA, eps: ['probe_band_delta'],
    entries: [
      { binding: 0, visibility: 4, buffer: { type: 'uniform' } },
      { binding: 1, visibility: 4, buffer: { type: 'storage' } },
    ] },
  contact_gather: { code: CONTACT, eps: ['probe_contact_gather'],
    entries: [
      { binding: 0, visibility: 4, buffer: { type: 'storage' } },
      { binding: 1, visibility: 4, buffer: { type: 'uniform' } },
    ] },
}});
console.log(JSON.stringify(result, null, 2));
await browser.close();
server.close();
