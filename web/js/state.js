/* ============================================================================
   7T WEB — state.js
   Buffers, textures, samplers, bind group layouts + bind groups for the
   terrain family, authored from the PORT_MAP §2c census dump. Group numbers
   and binding slots are fixed by the mirror WGSL; visibility flags are
   trimmed per stage exactly to the dump.

   Web capacity note (PORT_MAP §5): desktop runs 289 patch layers
   (PREGEN_RADIUS=8, 17×17) but core maxTextureArrayLayers is 256. The web
   host caps the window at 15×15 = 225 layers (radius 7) — a JS-host tuning,
   no WGSL change; the shader arrays are runtime-sized.
   ========================================================================== */

export const D = {
  PATCH_EXTENT: 50,
  HF_N: 256,                    // heightfield texels per patch side
  CELL_N: 16,
  MESH_N: 64,                   // LOD0 quads per side (65-vertex grid)
  PATCH_INDEX_COUNT: 26112,     // 64*64*6 + 256 skirt segs * 6
  PATCH_INDEX_COUNT_LOD1: 6912, // 32*32*6 + 128 skirt segs * 6
  MAX_PATCHES_WEB: 225,         // desktop 289; see capacity note above
  WINDOW_RADIUS: 7,             // 15×15 patch window (desktop PREGEN_RADIUS=8)
  MAX_AGENTS: 32,
  CONFIG_SIZE: 560,
  SIGNAL_SIZE: 336,
};

/* CPU-built skirted patch index buffers — port of state.hpp:2933-3021.
   skirt_grid_index MIRRORS world.wgsl patch_skirt_grid; the two MUST agree. */
function buildPatchIndices(step) {
  const N = D.MESH_N, S = N + 1, RING = 4 * N, GRID = S * S;
  const skirt = (k) => {
    let vx, vz;
    if      (k < N)     { vx = k;               vz = 0; }
    else if (k < 2 * N) { vx = N;               vz = k - N; }
    else if (k < 3 * N) { vx = N - (k - 2 * N); vz = N; }
    else                { vx = 0;               vz = N - (k - 3 * N); }
    return vz * S + vx;
  };
  const idx = [];
  const M = N / step;
  for (let z = 0; z < M; z++) for (let x = 0; x < M; x++) {
    const i00 = (z * step) * S + (x * step), i10 = i00 + step;
    const i01 = i00 + step * S, i11 = i01 + step;
    idx.push(i00, i01, i10, i10, i01, i11);
  }
  for (let k = 0; k < RING; k += step) {
    const k1 = (k + step) % RING;
    idx.push(skirt(k), skirt(k1), GRID + k, skirt(k1), GRID + k1, GRID + k);
  }
  return new Uint32Array(idx);
}

export function createState(device) {
  const B = GPUBufferUsage, T = GPUTextureUsage, V = GPUShaderStage;
  const buf = (label, size, usage) => device.createBuffer({ label, size, usage });

  /* --- buffers (sizes from the terrain build sheet) ------------------------ */
  const R = {
    config:   buf('DesignConfig', D.CONFIG_SIZE, B.UNIFORM | B.COPY_DST),
    signal:   buf('FrameSignal', D.SIGNAL_SIZE, B.UNIFORM | B.STORAGE | B.COPY_DST),
    vp:       buf('VPMatrix', 128, B.STORAGE | B.COPY_DST),
    camera:   buf('CameraState', 48, B.STORAGE | B.COPY_DST),
    agents:   buf('AgentState x32', 96 * D.MAX_AGENTS, B.STORAGE | B.COPY_DST),
    floating: buf('FloatingEntityArray', 264 * 208, B.UNIFORM | B.STORAGE | B.COPY_DST),
    dirLight: buf('DirectionalLight', 48, B.STORAGE | B.COPY_DST),
    pointLights: buf('PointLightArray', 272, B.STORAGE | B.COPY_DST),
    spotLights:  buf('SpotLightArray', 528, B.STORAGE | B.COPY_DST),
    zoneParams:  buf('GoLZoneArray', 656, B.STORAGE | B.COPY_DST),

    patchParams:    buf('PatchParams', 32, B.UNIFORM | B.COPY_DST),
    patchStaging:   buf('Patch staging', 32 * D.MAX_PATCHES_WEB, B.COPY_SRC | B.COPY_DST),
    patchInstances: buf('PatchInstances', 16 * D.MAX_PATCHES_WEB, B.STORAGE | B.COPY_DST),
    heightScratch:  buf('Patch height scratch', D.HF_N * D.HF_N * 2 * 4, B.STORAGE),
    piers:    buf('PierInstances', 48 * 68, B.STORAGE | B.COPY_DST),
    pyramids: buf('PyramidArray', 272, B.UNIFORM | B.COPY_DST),
    tileGrid: buf('TileGrid', 16400, B.UNIFORM | B.COPY_DST),

    visibleIndices: buf('Visible patch indices', 4 * D.MAX_PATCHES_WEB, B.STORAGE | B.COPY_DST),
    frustumCompute: buf('Frustum compute', 20, B.STORAGE | B.COPY_SRC | B.COPY_DST),
    frustumIndirect: buf('Frustum indirect LOD0', 20, B.INDIRECT | B.COPY_DST),
  };

  /* index buffers (CPU-built with skirts) */
  const idx0 = buildPatchIndices(1), idx1 = buildPatchIndices(2);
  if (idx0.length !== D.PATCH_INDEX_COUNT || idx1.length !== D.PATCH_INDEX_COUNT_LOD1) {
    throw new Error(`patch index counts drifted: ${idx0.length}/${idx1.length}`);
  }
  R.patchIB     = buf('Patch IB', idx0.byteLength, B.INDEX | B.COPY_DST);
  R.patchIBLOD1 = buf('Patch IB LOD1', idx1.byteLength, B.INDEX | B.COPY_DST);
  device.queue.writeBuffer(R.patchIB, 0, idx0);
  device.queue.writeBuffer(R.patchIBLOD1, 0, idx1);

  /* --- textures ------------------------------------------------------------ */
  const tex = (label, format, w, h, layers, usage) =>
    device.createTexture({ label, format, size: { width: w, height: h, depthOrArrayLayers: layers }, usage });

  R.heightfield = tex('Patch heightfield', 'rgba16float', D.HF_N, D.HF_N, D.MAX_PATCHES_WEB,
    T.STORAGE_BINDING | T.TEXTURE_BINDING);
  R.cellColor = tex('Patch cell color', 'rgba8unorm', D.CELL_N, D.CELL_N, D.MAX_PATCHES_WEB,
    T.STORAGE_BINDING | T.TEXTURE_BINDING);
  R.cellFields = tex('Cell fields LUT', 'rgba16float', D.CELL_N, D.CELL_N, D.MAX_PATCHES_WEB,
    T.STORAGE_BINDING | T.TEXTURE_BINDING);
  R.pawnAura = tex('Pawn aura', 'rgba16float', 64, 64, 1, T.TEXTURE_BINDING);
  R.zoneLife = tex('Zone life', 'rg32float', 16, 16, 1, T.TEXTURE_BINDING);
  /* Interim 16×16 depth maps until the shadow pass lifts (neutral = cleared
     to 1.0 → comparison Less always passes → fully lit). Real size is 4096. */
  R.shadowMap     = tex('Shadow map (interim)', 'depth32float', 16, 16, 1, T.RENDER_ATTACHMENT | T.TEXTURE_BINDING);
  R.spotShadowMap = tex('Spot shadow map (interim)', 'depth32float', 16, 16, 1, T.RENDER_ATTACHMENT | T.TEXTURE_BINDING);

  const arrayView = (t) => t.createView({ dimension: '2d-array' });
  R.heightfieldView = arrayView(R.heightfield);
  R.cellColorView = arrayView(R.cellColor);
  R.cellFieldsView = arrayView(R.cellFields);
  R.zoneLifeView = arrayView(R.zoneLife);

  R.bilinear = device.createSampler({ label: 'bilinear', magFilter: 'linear', minFilter: 'linear' });
  R.nearest  = device.createSampler({ label: 'nearest' });
  R.shadowSampler = device.createSampler({ label: 'shadow', compare: 'less', magFilter: 'linear', minFilter: 'linear' });

  /* --- bind group layouts (visibility trimmed to the §2c dump) ------------- */
  const ro = { type: 'read-only-storage' }, rw = { type: 'storage' }, un = { type: 'uniform' };

  R.patchGenLayout = device.createBindGroupLayout({
    label: 'patch gen (g0)',
    entries: [
      { binding: 1,  visibility: V.COMPUTE, buffer: un },
      { binding: 23, visibility: V.COMPUTE, buffer: un },
      { binding: 24, visibility: V.COMPUTE, storageTexture: { format: 'rgba16float', viewDimension: '2d-array' } },
      { binding: 25, visibility: V.COMPUTE, buffer: un },
      { binding: 26, visibility: V.COMPUTE, buffer: ro },
      { binding: 27, visibility: V.COMPUTE, storageTexture: { format: 'rgba8unorm', viewDimension: '2d-array' } },
      { binding: 28, visibility: V.COMPUTE, buffer: rw },
      { binding: 29, visibility: V.COMPUTE, storageTexture: { format: 'rgba16float', viewDimension: '2d-array' } },
      { binding: 30, visibility: V.COMPUTE, buffer: un },
    ],
  });

  R.frustumLayout = device.createBindGroupLayout({
    label: 'frustum cull (g0)',
    entries: [
      { binding: 1,   visibility: V.COMPUTE, buffer: un },
      { binding: 2,   visibility: V.COMPUTE, buffer: ro },
      { binding: 340, visibility: V.COMPUTE, buffer: ro },
      { binding: 500, visibility: V.COMPUTE, buffer: rw },
      { binding: 501, visibility: V.COMPUTE, buffer: rw },
    ],
  });

  R.renderEntityLayout = device.createBindGroupLayout({
    label: 'render entity (g0)',
    entries: [
      { binding: 1,   visibility: V.VERTEX | V.FRAGMENT, buffer: un },
      { binding: 25,  visibility: V.FRAGMENT, buffer: un },   // tile_grid — FS archetype shading
      { binding: 200, visibility: V.VERTEX, buffer: ro },
      { binding: 201, visibility: V.VERTEX | V.FRAGMENT, buffer: ro },
      { binding: 260, visibility: V.VERTEX | V.FRAGMENT, buffer: ro },
      { binding: 280, visibility: V.FRAGMENT, buffer: ro },
      { binding: 300, visibility: V.FRAGMENT, buffer: un },
      { binding: 320, visibility: V.FRAGMENT, buffer: ro },
      { binding: 321, visibility: V.FRAGMENT, buffer: ro },
      { binding: 322, visibility: V.FRAGMENT, buffer: ro },
      { binding: 340, visibility: V.VERTEX, buffer: ro },
      { binding: 391, visibility: V.VERTEX, buffer: ro },
    ],
  });

  R.renderTextureLayout = device.createBindGroupLayout({
    label: 'render texture (g1)',
    entries: [
      { binding: 22, visibility: V.VERTEX | V.FRAGMENT, sampler: { type: 'filtering' } },
      { binding: 23, visibility: V.FRAGMENT, sampler: { type: 'non-filtering' } },
      { binding: 25, visibility: V.FRAGMENT, texture: { sampleType: 'depth' } },
      { binding: 26, visibility: V.FRAGMENT, sampler: { type: 'comparison' } },
      { binding: 27, visibility: V.FRAGMENT, texture: { sampleType: 'depth' } },
      { binding: 28, visibility: V.VERTEX, texture: { sampleType: 'float', viewDimension: '2d-array' } },
      { binding: 29, visibility: V.FRAGMENT, texture: { sampleType: 'float', viewDimension: '2d-array' } },
      { binding: 30, visibility: V.FRAGMENT, texture: { sampleType: 'float', viewDimension: '2d-array' } },
      { binding: 31, visibility: V.FRAGMENT, texture: { sampleType: 'unfilterable-float', viewDimension: '2d-array' } },
      { binding: 32, visibility: V.FRAGMENT, buffer: ro },
      { binding: 33, visibility: V.VERTEX | V.FRAGMENT, texture: { sampleType: 'float' } },
    ],
  });

  /* --- bind groups ---------------------------------------------------------- */
  const e = (binding, resource) => ({ binding, resource });
  const bb = (binding, buffer) => e(binding, { buffer });

  R.patchGenBG = device.createBindGroup({
    label: 'patch gen', layout: R.patchGenLayout,
    entries: [
      bb(1, R.config), bb(23, R.patchParams), e(24, R.heightfieldView), bb(25, R.tileGrid),
      bb(26, R.piers), e(27, R.cellColorView), bb(28, R.heightScratch), e(29, R.cellFieldsView),
      bb(30, R.pyramids),
    ],
  });

  R.frustumBG = device.createBindGroup({
    label: 'frustum cull', layout: R.frustumLayout,
    entries: [bb(1, R.config), bb(2, R.vp), bb(340, R.patchInstances),
              bb(500, R.visibleIndices), bb(501, R.frustumCompute)],
  });

  R.renderEntityBG = device.createBindGroup({
    label: 'render entity', layout: R.renderEntityLayout,
    entries: [
      bb(1, R.config), bb(25, R.tileGrid), bb(200, R.signal), bb(201, R.vp), bb(260, R.agents), bb(280, R.camera),
      bb(300, R.floating), bb(320, R.dirLight), bb(321, R.pointLights), bb(322, R.spotLights),
      bb(340, R.patchInstances), bb(391, R.visibleIndices),
    ],
  });

  R.renderTextureBG = device.createBindGroup({
    label: 'render texture', layout: R.renderTextureLayout,
    entries: [
      e(22, R.bilinear), e(23, R.nearest), e(25, R.shadowMap.createView()), e(26, R.shadowSampler),
      e(27, R.spotShadowMap.createView()), e(28, R.heightfieldView), e(29, R.cellColorView),
      e(30, R.cellFieldsView), e(31, R.zoneLifeView), bb(32, R.zoneParams),
      e(33, R.pawnAura.createView()),
    ],
  });

  return R;
}

/* Clear both interim shadow maps to 1.0 (fully lit). Encoder-level, once. */
export function clearShadowMaps(device, R) {
  const enc = device.createCommandEncoder({ label: 'shadow clear' });
  for (const t of [R.shadowMap, R.spotShadowMap]) {
    enc.beginRenderPass({
      colorAttachments: [],
      depthStencilAttachment: {
        view: t.createView(), depthClearValue: 1.0, depthLoadOp: 'clear', depthStoreOp: 'store',
      },
    }).end();
  }
  device.queue.submit([enc.finish()]);
}
