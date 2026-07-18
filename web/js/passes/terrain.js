/* ============================================================================
   7T WEB — passes/terrain.js
   Streaming conductor (patch_system.hpp analogue) + tile state
   (tile_world.hpp port). Fixed lod_point (0,0) until camera/vp lifts, so
   eviction is idle by construction; the allocate→generate→band phases follow
   the desktop shape.

   Seed parity: cpu_hash / tile_seed are exact ports of seed_utils.hpp — with
   world_seed 42 the web tile archetypes match desktop. Terrain tokens are
   skipped: at boot the desktop has no active tokens (emission is driven by
   musical outcomes, a Phase 3 concern).
   ========================================================================== */

import { D } from '../state.js';

/* --- seed_utils.hpp ports (u32-exact) -------------------------------------- */
function cpuHash(seed, prop) {
  let h = (Math.imul(seed, 747796405) + Math.imul(prop, 2891336453) + 1) >>> 0;
  h = Math.imul((h >>> 16) ^ h, 2654435769) >>> 0;
  h = Math.imul((h >>> 16) ^ h, 2654435769) >>> 0;
  return ((h >>> 16) ^ h) >>> 0;
}
const cpuHashF = (seed, prop) => cpuHash(seed, prop) / 0xFFFFFFFF;

function tileSeed(master, gx, gz) {
  let h = master >>> 0;
  h = (h ^ Math.imul(gx, 73856093)) >>> 0;
  h = (h ^ Math.imul(gz, 19349663)) >>> 0;
  h = Math.imul((h ^ (h >>> 16)) >>> 0, 2654435769) >>> 0;
  h = Math.imul((h ^ (h >>> 16)) >>> 0, 2654435769) >>> 0;
  return h;
}

/* --- tile_world.hpp: archetype selection ----------------------------------- */
/* {amp, bias, act, base_weight, amp_jit, bias_jit} — TW:43 */
const ARCHETYPES = [
  { amp: 2.0,  bias: 4.0,  act: 0.7, w: 1.8, aj: 0.3,  bj: 1.0 },   // mountainous
  { amp: 1.0,  bias: 0.0,  act: 1.0, w: 1.3, aj: 0.3,  bj: 1.0 },   // varied
  { amp: 0.5,  bias: -2.0, act: 1.3, w: 1.0, aj: 0.3,  bj: 1.0 },   // basin
  { amp: 0.04, bias: -0.5, act: 0.2, w: 0.0, aj: 0.02, bj: 0.2 },   // pool
];
const POOL_WEIGHT_OUTDOOR = 0.05;

export class TerrainStreamer {
  constructor(device, R, config, worldSeed = 42) {
    this.device = device;
    this.R = R;
    this.config = config;
    this.seed = worldSeed;
    this.tiles = new Map();          // "gx,gz" -> {amp,bias,act,archetype}
    this.patches = new Map();        // "gx,gz" -> {gx,gz,layer}
    this.layerFree = Array.from({ length: D.MAX_PATCHES_WEB }, (_, i) => D.MAX_PATCHES_WEB - 1 - i);
    this.tileGridDirty = true;
    this.bandDirty = false;
    this.counts = { lod0: 0, render: 0, all: 0 };

    // wanted cells: full radius-7 square, nearest (patch center) first
    const rad = D.WINDOW_RADIUS;
    this.pending = [];
    for (let gz = -rad; gz <= rad; gz++) for (let gx = -rad; gx <= rad; gx++) this.pending.push([gx, gz]);
    const c = (g) => (g + 0.5) * D.PATCH_EXTENT;
    this.pending.sort((a, b) => (c(a[0]) ** 2 + c(a[1]) ** 2) - (c(b[0]) ** 2 + c(b[1]) ** 2));
    this.staged = 0;                 // staging slots used (monotonic; window is generated once)
  }

  ensureTile(gx, gz) {
    const key = `${gx},${gz}`;
    if (this.tiles.has(key)) return;
    // 8-neighbor archetype census over already-generated tiles
    const counts = [0, 0, 0, 0];
    for (let dz = -1; dz <= 1; dz++) for (let dx = -1; dx <= 1; dx++) {
      if (!dx && !dz) continue;
      const n = this.tiles.get(`${gx + dx},${gz + dz}`);
      if (n) counts[n.archetype]++;
    }
    const w = ARCHETYPES.map((a) => a.w);
    w[3] = POOL_WEIGHT_OUTDOOR;
    for (let a = 0; a < 4; a++) {
      if (counts[a] >= 4) w[a] *= 0.2;         // DOMINANT
      else if (counts[a] >= 2) w[a] *= 1.5;    // COMMON
      else if (counts[a] === 1) w[a] *= 2.0;   // PRESENT
    }
    const total = w.reduce((s, x) => s + x, 0);
    const seed = tileSeed(this.seed, gx, gz);
    const roll = cpuHashF(seed, 300) * total;
    let acc = 0, pick = 0;
    for (let a = 0; a < 4; a++) { acc += w[a]; if (roll <= acc) { pick = a; break; } }
    const p = ARCHETYPES[pick];
    const ampJit = 1 + (cpuHashF(seed, 301) - 0.5) * p.aj;
    const biasJit = (cpuHashF(seed, 302) - 0.5) * p.bj;
    this.tiles.set(key, {
      amp: p.amp * ampJit, bias: p.bias + biasJit, act: p.act, archetype: pick,
    });
    this.tileGridDirty = true;
  }

  /* per-frame: allocate + generate up to `budget` patches (desktop
     patches_budget_this_frame analogue), then band if anything changed */
  encode(enc, budget = 6) {
    let n = 0;
    while (this.pending.length && n < budget) {
      const [gx, gz] = this.pending.shift();
      // tile + 3×3 padding (hermite interp in the shader reads neighbors)
      for (let dz = -1; dz <= 1; dz++) for (let dx = -1; dx <= 1; dx++) this.ensureTile(gx + dx, gz + dz);
      const layer = this.layerFree.pop();
      if (layer === undefined) break;   // window > capacity would be a bug; radius 7 fits 225 exactly

      // PatchParams: origin[2], extent, resolution(u32), master_seed(u32), time, layer(u32), pad
      const slot = this.staged++;
      const ab = new ArrayBuffer(32);
      const f = new Float32Array(ab), u = new Uint32Array(ab);
      f[0] = (gx + 0.5) * D.PATCH_EXTENT; f[1] = (gz + 0.5) * D.PATCH_EXTENT;
      f[2] = D.PATCH_EXTENT; u[3] = D.HF_N; u[4] = this.seed; f[5] = 0; u[6] = layer;
      this.device.queue.writeBuffer(this.R.patchStaging, slot * 32, ab);

      enc.copyBufferToBuffer(this.R.patchStaging, slot * 32, this.R.patchParams, 0, 32);
      const pass = enc.beginComputePass({ label: `patch gen ${gx},${gz}` });
      pass.setBindGroup(0, this.R.patchGenBG);
      pass.setPipeline(this.R.pipeHeights);   pass.dispatchWorkgroups(16, 16, 1);
      pass.setPipeline(this.R.pipeGradients); pass.dispatchWorkgroups(16, 16, 1);
      pass.setPipeline(this.R.pipeCells);     pass.dispatchWorkgroups(2, 2, 1);
      pass.end();

      this.patches.set(`${gx},${gz}`, { gx, gz, layer });
      this.bandDirty = true;
      n++;
    }
    if (this.bandDirty) this.band();
    if (this.tileGridDirty) this.uploadTileGrid();
    return n;
  }

  /* band_patches analogue: pack PatchInstance [lod0 | lod1 | pregen] by
     nearest-edge distance from lod_point (0,0); upload + publish counts */
  band() {
    const lod0 = [], lod1 = [], pregen = [];
    const half = D.PATCH_EXTENT / 2;
    for (const p of this.patches.values()) {
      const cx = (p.gx + 0.5) * D.PATCH_EXTENT, cz = (p.gz + 0.5) * D.PATCH_EXTENT;
      const dx = Math.max(0, Math.abs(cx) - half), dz = Math.max(0, Math.abs(cz) - half);
      const d2 = dx * dx + dz * dz;
      (d2 <= 175 * 175 ? lod0 : d2 <= 325 * 325 ? lod1 : pregen).push(p);
    }
    const packed = [...lod0, ...lod1, ...pregen];
    const ab = new ArrayBuffer(16 * D.MAX_PATCHES_WEB);
    const f = new Float32Array(ab), u = new Uint32Array(ab);
    packed.forEach((p, i) => {
      f[i * 4] = (p.gx + 0.5) * D.PATCH_EXTENT;
      f[i * 4 + 1] = (p.gz + 0.5) * D.PATCH_EXTENT;
      f[i * 4 + 2] = D.PATCH_EXTENT;
      u[i * 4 + 3] = p.layer;
    });
    this.device.queue.writeBuffer(this.R.patchInstances, 0, ab);
    this.counts = { lod0: lod0.length, render: lod0.length + lod1.length, all: packed.length };
    this.config.setPlacementPatchCount(packed.length);
    this.config.setLodPoint(0, 0);
    this.bandDirty = false;
  }

  /* GPUTileGrid: origin(i32×2), side(u32), cell_extent, entries[side²] dense */
  uploadTileGrid() {
    const rad = D.WINDOW_RADIUS;
    const side = 2 * (rad + 1) + 1;          // 17 (window + 1 padding ring)
    const ox = -(rad + 1), oz = -(rad + 1);
    const ab = new ArrayBuffer(16400);
    const i32 = new Int32Array(ab), u = new Uint32Array(ab), f = new Float32Array(ab);
    i32[0] = ox; i32[1] = oz; u[2] = side; f[3] = D.PATCH_EXTENT;
    for (let lz = 0; lz < side; lz++) for (let lx = 0; lx < side; lx++) {
      const t = this.tiles.get(`${ox + lx},${oz + lz}`);
      const base = 4 + (lz * side + lx) * 4;
      f[base] = t ? t.amp : 1.0;
      f[base + 1] = t ? t.bias : 0.0;
      f[base + 2] = t ? t.act : 1.0;
      u[base + 3] = t ? t.archetype : 1;
    }
    this.device.queue.writeBuffer(this.R.tileGrid, 0, ab);
    this.tileGridDirty = false;
  }
}

/* Frustum cull + terrain draws — RENDER_SPINE R17/R19 terrain slice. */
const INDIRECT_RESET = new Uint32Array([D.PATCH_INDEX_COUNT, 0, 0, 0, 0]);

export function encodeCullAndDraw(device, R, enc, colorView, depthView, clearValue, counts) {
  // R17 — reset args, cull, copy to the indirect buffer
  device.queue.writeBuffer(R.frustumCompute, 0, INDIRECT_RESET);
  const cpass = enc.beginComputePass({ label: 'frustum cull' });
  cpass.setPipeline(R.pipeCull);
  cpass.setBindGroup(0, R.frustumBG);
  cpass.dispatchWorkgroups(4, 1, 1);   // 256 threads; LOD0 packs first (desktop-matched)
  cpass.end();
  enc.copyBufferToBuffer(R.frustumCompute, 0, R.frustumIndirect, 0, 20);

  // R19 — main pass: LOD0 indirect + LOD1 direct
  const rpass = enc.beginRenderPass({
    label: 'main',
    colorAttachments: [{ view: colorView, clearValue, loadOp: 'clear', storeOp: 'store' }],
    depthStencilAttachment: {
      view: depthView, depthClearValue: 1.0, depthLoadOp: 'clear', depthStoreOp: 'store',
    },
  });
  if (counts.render > 0) {
    rpass.setBindGroup(0, R.renderEntityBG);
    rpass.setBindGroup(1, R.renderTextureBG);
    rpass.setPipeline(R.pipeTerrainIndirect);
    rpass.setIndexBuffer(R.patchIB, 'uint32');
    rpass.drawIndexedIndirect(R.frustumIndirect, 0);
    const lod1Count = counts.render - counts.lod0;
    if (lod1Count > 0) {
      rpass.setPipeline(R.pipeTerrainDirect);
      rpass.setIndexBuffer(R.patchIBLOD1, 'uint32');
      rpass.drawIndexed(D.PATCH_INDEX_COUNT_LOD1, lod1Count, 0, 0, counts.lod0);
    }
  }
  rpass.end();
}
