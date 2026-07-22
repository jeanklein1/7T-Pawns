// [TRUEBAND_CONTACT_1 T3] Dawn validation witness — POST-BATCH variant.
//
// Same method as probe_dawn_witness.mjs (headless Chromium = real Dawn), but
// against the BATCH tree: layouts re-parsed into audit/cc6_output_post_tc1.json;
// the Live Card Writer family is now the SPLIT pair (write_live_card_heights +
// write_live_card_resolve — TRUEBAND_CONTACT_1 [T1b]); everything else is the
// U roster. A green baseline witnesses that every paired layout/group/shader
// edit landed coherently. Writes audit/probe_results_post_tc1.json.
//
// Usage: node audit/probe_dawn_witness_post_tc1.mjs   (from repo root)
import { createRequire } from 'node:module';
import { readFileSync, writeFileSync } from 'node:fs';
import http from 'node:http';

const require = createRequire(import.meta.url);
let chromium;
try {
  ({ chromium } = require('playwright-core'));
} catch {
  // container fallback: the global `playwright` package re-exports chromium
  ({ chromium } = require('playwright'));
}

const wgsl = readFileSync('src/cartridges/the_board/realization/world.wgsl', 'utf8');
const cc6 = JSON.parse(readFileSync('audit/cc6_output_post_tc1.json', 'utf8'));

// Pipeline families at GROUND_CARD_1 batch HEAD (renderer.hpp createComputePipelines).
const LIVE = ['update_player_agent', 'update_other_agents', 'update_camera', 'update_sphere', 'update_cube'];
const FAMILIES = {
  live_contrib: { groups: ['Compute Entity Layout', 'Compute Texture Layout'], eps: LIVE },
  compute_vp: { groups: ['Compute Entity Layout'], eps: ['compute_vp'] },
  terrain_index_gen: { groups: ['Terrain Index Gen Layout'], eps: ['generate_terrain_indices'] },
  patch_gen: { groups: ['Patch Gen Layout'], eps: ['generate_patch_heights', 'generate_patch_gradients', 'generate_patch_cells'] },
  ribbon: { groups: ['Ribbon Compute Layout'], eps: ['compute_ribbon_rings'] },
  photographer: { groups: ['Photographer Compute Layout'], eps: ['compute_photographer_vp'] },
  // H5: placement rides [placement, compute texture] — the card's cell-exact GoL fetch.
  entity_placement: { groups: ['Entity Placement Compute Layout', 'Compute Texture Layout'], eps: ['compute_entity_placement'] },
  frustum_cull: { groups: ['Frustum Cull Compute Layout'], eps: ['frustum_cull_patches'] },
  pawn_aura: { groups: ['Pawn Aura Compute Layout'], eps: ['compute_pawn_aura'] },
  // H3: the live card writer (GROUND_CARD_1).
  live_card: { groups: ['Live Card Writer Layout'], eps: ['write_live_card_heights', 'write_live_card_resolve'] },
  zone_mask: { groups: ['Zone Mask Layout'], eps: ['zone_seed_mask'] },
  orb: { groups: ['Orb Compute Layout'], eps: ['orb_init', 'orb_dynamics', 'orb_recolor'] },
  orb_copy: { groups: ['Orb Copy Layout'], eps: ['orb_state_prev_copy'] },
  gol_zone: { groups: ['GoL Zone Compute Layout'], eps: ['zone_gol_sync', 'zone_gol_evolve', 'zone_derive_params'] },
  arch_mesh: { groups: ['Arch Mesh Gen Layout'], eps: ['arch_mesh_gen'] },
  column_mesh: { groups: ['Column Mesh Gen Layout'], eps: ['column_mesh_gen'] },
  palm_mesh: { groups: ['Palm Mesh Gen Layout'], eps: ['palm_mesh_gen'] },
  cactus_mesh: { groups: ['Cactus Mesh Gen Layout'], eps: ['cactus_mesh_gen'] },
  blade_mesh: { groups: ['Blade Mesh Gen Layout'], eps: ['blade_cluster_mesh_gen'] },
};

const server = http.createServer((_req, res) => {
  res.setHeader('content-type', 'text/html');
  res.end('<!doctype html><title>dawn-witness</title><body>ok</body>');
});
await new Promise(r => server.listen(0, '127.0.0.1', r));
const port = server.address().port;

const browser = await chromium.launch({
  executablePath: '/opt/pw-browsers/chromium',
  args: ['--no-sandbox', '--enable-unsafe-webgpu', '--enable-features=Vulkan',
         '--use-webgpu-adapter=swiftshader', '--disable-gpu-sandbox'],
});
const page = await browser.newPage();
await page.goto(`http://127.0.0.1:${port}/`);

const result = await page.evaluate(async ({ wgsl, layouts, families }) => {
  const out = { env: {}, module: {}, baseline: {} };
  const adapter = await navigator.gpu.requestAdapter();
  out.env.adapterInfo = { vendor: adapter.info?.vendor, architecture: adapter.info?.architecture };
  out.env.wgslLanguageFeatures = [...navigator.gpu.wgslLanguageFeatures];
  out.env.adapterLimits = {
    maxStorageBuffersPerShaderStage: adapter.limits.maxStorageBuffersPerShaderStage,
    maxUniformBuffersPerShaderStage: adapter.limits.maxUniformBuffersPerShaderStage,
    maxStorageTexturesPerShaderStage: adapter.limits.maxStorageTexturesPerShaderStage,
  };
  const device = await adapter.requestDevice({
    requiredLimits: {
      maxStorageBuffersPerShaderStage: adapter.limits.maxStorageBuffersPerShaderStage,
      maxStorageTexturesPerShaderStage: adapter.limits.maxStorageTexturesPerShaderStage,
      maxSampledTexturesPerShaderStage: adapter.limits.maxSampledTexturesPerShaderStage,
      maxUniformBuffersPerShaderStage: adapter.limits.maxUniformBuffersPerShaderStage,
      maxBindingsPerBindGroup: adapter.limits.maxBindingsPerBindGroup,
    },
  });

  const VIS = { Compute: GPUShaderStage.COMPUTE, Vertex: GPUShaderStage.VERTEX, Fragment: GPUShaderStage.FRAGMENT };
  const BUF = { Storage: 'storage', ReadOnlyStorage: 'read-only-storage', Uniform: 'uniform' };
  const SAMP = { Filtering: 'filtering', NonFiltering: 'non-filtering', Comparison: 'comparison' };
  const TEX = { Float: 'float', UnfilterableFloat: 'unfilterable-float', Depth: 'depth', Uint: 'uint', Sint: 'sint' };
  const FMT = { RGBA16Float: 'rgba16float', RGBA8Unorm: 'rgba8unorm', RG32Float: 'rg32float', R32Float: 'r32float', RGBA32Float: 'rgba32float' };

  function layoutDesc(lay) {
    const entries = [];
    for (const e of lay.entries) {
      const ent = { binding: e.binding, visibility: e.stages.reduce((a, s) => a | VIS[s], 0) };
      if (e.class === 'storage_buffer' || e.class === 'uniform_buffer' || e.class === 'other_buffer') {
        ent.buffer = { type: BUF[e.detail] ?? 'uniform' };
      } else if (e.class === 'sampler') {
        ent.sampler = { type: SAMP[e.detail] ?? 'filtering' };
      } else if (e.class === 'sampled_texture') {
        ent.texture = { sampleType: TEX[e.detail] ?? 'float', viewDimension: e.viewDimension ?? '2d' };
      } else if (e.class === 'storage_texture') {
        ent.storageTexture = { access: 'write-only', format: FMT[e.detail], viewDimension: e.viewDimension ?? '2d' };
      }
      entries.push(ent);
    }
    return { entries };
  }

  const t0 = performance.now();
  const module = device.createShaderModule({ code: wgsl, label: 'world.wgsl (post-GC1 witness)' });
  const info = await module.getCompilationInfo();
  out.module.create_plus_info_ms = +(performance.now() - t0).toFixed(1);
  out.module.messages = info.messages.map(m => `${m.type} L${m.lineNum}: ${m.message}`);

  const layByLabel = Object.fromEntries(layouts.map(l => [l.label, l]));

  for (const [name, fam] of Object.entries(families)) {
    const rec = { groups: fam.groups, results: {} };
    let plo;
    try {
      const bgls = fam.groups.map(label => device.createBindGroupLayout(layoutDesc(layByLabel[label])));
      plo = device.createPipelineLayout({ bindGroupLayouts: bgls });
    } catch (e) {
      rec.layout_error = String(e);
      out.baseline[name] = rec;
      continue;
    }
    for (const ep of fam.eps) {
      const t = performance.now();
      try {
        await device.createComputePipelineAsync({ layout: plo, compute: { module, entryPoint: ep } });
        rec.results[ep] = { ok: true, ms: +(performance.now() - t).toFixed(1) };
      } catch (e) {
        rec.results[ep] = { ok: false, error: e.message ?? String(e) };
      }
    }
    out.baseline[name] = rec;
  }

  return out;
}, { wgsl, layouts: cc6.layouts, families: FAMILIES });

writeFileSync('audit/probe_results_post_tc1.json', JSON.stringify(result, null, 2));
const flat = [];
for (const [fam, rec] of Object.entries(result.baseline)) {
  if (rec.layout_error) { flat.push(`${fam}: LAYOUT ERROR ${rec.layout_error}`); continue; }
  for (const [ep, r] of Object.entries(rec.results)) {
    if (!r.ok) flat.push(`${fam}/${ep}: FAIL ${r.error}`);
  }
}
console.log(JSON.stringify({
  module_ms: result.module.create_plus_info_ms,
  module_messages: result.module.messages,
  failures: flat,
  verdict: flat.length === 0 ? 'ALL PIPELINE FAMILIES GREEN' : `${flat.length} FAILURES`,
}, null, 2));
await browser.close();
server.close();
