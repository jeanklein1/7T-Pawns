# MIP_0 — THE EXHIBITION'S CHAIN

*Handoff · authored 2026-09-08 · Claude → CC · Jean holds build, visual, merge, deploy.*
*Filed to `docs/HANDOFFS/MIP_0.md` while open; the directory dies with the campaign.*

## THE IDEA, IN ONE PARAGRAPH

The painting arrays have one mip level. Every painting pixel at a distance bilinear-samples
a 1024² texture with no chain: four scattered texels per pixel, cache-hostile, and the
shimmer PLATE_0 made harder — the laptop's hit. WebGPU generates no mipmaps, so the program
does: for a **painting**, the loader builds the eleven-level chain on the CPU inside the
upload it already runs (a 2×2 box mean per level), the authored staging carries it, and the
promotion copies it level by level into the exhibition; the pad beyond the picture becomes
its replicated edge, so no level's border blends with black. For a **photograph** the chain
does not exist — its higher levels are never written — and the shader samples it at level 0
exactly as today. The two kinds are told apart by the slot's `content_source`, which the GPU
already holds. Paintings gain a real chain (sharper close, quiet at a distance, fewer
fetches); photographs are unchanged; the postcard reads level 0 and is untouched.

## AUTHORITY

Base: `master` at HEAD `e2cf155c` (PLATE_0 U4d), fetched 2026-09-08. Rides a **held branch
`claude/mip-0`** (P12: a shader and an estate change; the visual gate looks for a change on
the far wall and none up close). Disjoint from RAZOR_0's files.

| file | blob at base |
| --- | --- |
| `src/cartridges/the_board/realization/state.hpp` | `1f9e883c180024f740fe133fe8f23302eccc1b16` |
| `src/cartridges/the_board/bodies/gallery.hpp` | `d51dd8b1120428129b07ab8e33b047cd2e626f57` |
| `src/cartridges/the_board/realization/world.wgsl` | `5c6534b7423f3491c70e3243921a36c215caad88` |
| `tools/binding_gen.py` | `afc4dbb1da568e6c52ce042d74655e18f0cda764` |
| `tools/gates/glaw2/run.py` | `a66e5650ba5b412a573974b3503853a4842c097c` |

**Pinned:** `state.hpp` (BINDING + MIRROR), `gallery.hpp` (BINDING + COMMAND), `world.wgsl`
(BINDING + MIRROR). Red from U1 until U3 regenerates.

**HALT classes (P17), scoped to the unit (P15):** unreachable file; stale authority (blob
differs AND a FIND misses). Everything else DEFAULT-AND-FLAG. Symbols, not ranges (P2).
Expected match count **1** everywhere. LF only, no BOM.

**REHEARSED, WITH ONE GATE LEFT TO CC.** All sixteen FINDs at count 1 on the base. After U1:
G-LAW 1 GREEN, TU gate zero diagnostics, `binding_gen.py --write` byte-identical (the census
reads the lambda's fourth argument now — U1.9), every census row PASS. After U2: G-LAW 2
GREEN (328 fn, 335 const — the helper and its constant), the mirror census clean. **The WGSL
gate (naga) cannot run in the rehearsal sandbox; it is the first thing U2 runs on the
machine.** The shader edit is a helper, a flat varying and two call sites; its one subtlety
— implicit derivatives may not sit under a non-uniform branch — is handled by taking them in
uniform flow and using `textureSampleGrad`, which may.

## THE RULINGS THIS CAMPAIGN LANDS

1. **The chain is built where the texels already pass through the CPU.** `upload_authored_
   painting` already swaps and writes every authored square (the loader's and the boot's
   solid fill alike); it now writes the chain too. No new pass, no new pipeline, no new
   binding — the estate census sees a fourth lambda argument and nothing else.
2. **A photograph has no chain and asks for none.** Its higher levels are never written
   and never read; the shader samples it at level 0 through `sample_exhibition`, keyed on
   the slot's `content_source` (already on the GPU). The alternative — a blit pass to build
   photograph chains on the GPU — is a pipeline, a layout, seats and a census enrollment,
   for seven layers; registered as the residual, not built.
3. **The choice is per instance, so it is not uniform control flow.** `textureSample`
   (implicit derivatives) may not sit inside such a branch; the derivatives are taken once,
   uniformly, and handed to `textureSampleGrad`. `dpdx`/`dpdy`/`textureSampleGrad` are
   admitted to G-LAW 2's predeclared set — language facts, the gate's own patch (its banner
   says so).
4. **The pad is the edge, replicated.** Zero padding would put a dark rim on every painting
   that grows with distance; the last column and row are replicated across the square
   before upload. The quads sample only 0..uv_scale and the postcard crops the pad away, so
   nothing else moves.
5. **The estate grows by a third where the chain lives:** exhibition 160 → 213.3 MiB,
   authored staging 128 → 170.7; the snapshot staging stays at one level. **+96 MiB.** The
   phones passed PLATE_0 at ~456; this round's residency row is the same row again.
6. **The promotion copies as many levels as its source has.** `promote_to_exhibition` takes
   `levels`: the chain for an authored source, 1 for a shot. The full-layer law holds at
   every level copied.

## WHAT WAS EXAMINED AND FOUND ALREADY TIGHT (so nobody re-derives it)

The survey asked one question of every per-frame cost: does this work change a pixel? These
do not need touching — the tree already answers:

- **The sun's sixteen taps are skipped where they cannot show.** `calc_directional_light`
  samples the shadow map only when `ndotl > 0` and `spots.count == 0`; point lights loop
  over `count`, not the cap.
- **The two shadow passes are an either/or.** `render_shadow_pass` renders the spot atlas
  when spots are active and the sun map otherwise — never both — and indoors the sun map
  is the atlas's first tile.
- **The per-frame compute rows are dirty-gated.** Placement correction, mesh generation,
  the aura (`aura_presence > 0 || aura_needs_clear`), the zone derive flush; the readback
  is 32 agents.
- **A fog early-out cannot fire.** The ring is the draw authority at 6.84 × 50 = 342 wu;
  at the densest mood (0.0168) fog reaches 0.9968 at the ring, under any threshold that is
  pixel-identical at 8 bits (≥ 0.9995). Three lines that never run are not a razor.
- **The dial for the sun's kernel already exists.** `config.shadow_pcf_taps` has a live
  setter and a 2×2 kernel beside the 4×4; it is an ORGAN dial (`?organ=1`). It is a quality
  trade, so it is not in these handoffs — but it needs no code to try.

Two things remained: work done on pixels that are then covered (RAZOR_0), and texels
fetched from a 1024² array with no chain (MIP_0).

## UNITS, IN ORDER — every commit compiles

| unit | what | files | commit |
| --- | --- | --- | --- |
| U0 | preflight | — | none |
| U1 | the chain: the constant, the arrays, the sampler, the promotion, the upload, the pad, the census | `state.hpp`, `gallery.hpp`, `tools/binding_gen.py` | 1 |
| U2 | sampling by kind; the gate's predeclared names | `world.wgsl`, `tools/gates/glaw2/run.py` | 1 |
| U3 | ledgers; OPEN.md; push | `audit/*`, `docs/OPEN.md` | 1 |

---

## U0 — PREFLIGHT (no commit)

```
git fetch origin master && git checkout -b claude/mip-0 origin/master
git ls-tree HEAD src/cartridges/the_board/realization/state.hpp src/cartridges/the_board/bodies/gallery.hpp src/cartridges/the_board/realization/world.wgsl tools/binding_gen.py tools/gates/glaw2/run.py
grep -c "PAINTING_RESOLUTION = 1024" src/cartridges/the_board/realization/state.hpp   # 1 — PLATE_0 is the base
python3 tools/wgsl_gate.py      # PASS before anything moves — this gate is the round's, and the sandbox could not run it
```

---

## U1 — THE CHAIN

### M1.1 — the chain's length, beside the resolution (`src/cartridges/the_board/realization/state.hpp`)

FIND
```
            constexpr uint32_t PAINTING_RESOLUTION = 1024;
```
REPLACE
```
            constexpr uint32_t PAINTING_RESOLUTION = 1024;
            // MIP_0 — THE CHAIN, every level down to 1x1: 1024 -> 11 levels.
            // The exhibition and the authored staging carry it (a painting is
            // sampled through it); the snapshot staging carries level 0 only,
            // and the shader samples a photograph at level 0 (world.wgsl,
            // sample_exhibition) — so its higher levels, never written, are
            // never read. One number, asserted against the resolution so the
            // two cannot drift.
            constexpr uint32_t PAINTING_MIP_LEVELS = 11;
            static_assert((1u << (PAINTING_MIP_LEVELS - 1u)) == PAINTING_RESOLUTION,
                "MIP_0: the chain must end at 1x1 — PAINTING_MIP_LEVELS is log2(RES) + 1");
```

### M1.2 — the array lambda takes the chain's length (`src/cartridges/the_board/realization/state.hpp`)

FIND
```
                auto makeTextureArray = [&](const char* label, uint32_t layers,
                    wgpu::TextureUsage usage) -> wgpu::Texture
                    {
                        wgpu::TextureDescriptor desc{};
                        desc.size = { Dim::PAINTING_RESOLUTION, Dim::PAINTING_RESOLUTION, layers };
                        desc.dimension = wgpu::TextureDimension::e2D;
                        desc.format = colorFormat;
                        desc.usage = usage;
                        return makeTexture(label, desc);
                    };
```
REPLACE
```
                auto makeTextureArray = [&](const char* label, uint32_t layers,
                    wgpu::TextureUsage usage, uint32_t mips) -> wgpu::Texture
                    {
                        wgpu::TextureDescriptor desc{};
                        desc.size = { Dim::PAINTING_RESOLUTION, Dim::PAINTING_RESOLUTION, layers };
                        desc.dimension = wgpu::TextureDimension::e2D;
                        desc.format = colorFormat;
                        desc.usage = usage;
                        desc.mipLevelCount = mips;   // MIP_0 — the chain, or 1; noteAlloc prices the levels
                        return makeTexture(label, desc);
                    };
```

### M1.3 — the snapshot staging: level 0 only (`src/cartridges/the_board/realization/state.hpp`)

FIND
```
                snapshotStagingTexture_ = makeTextureArray("Snapshot Staging",
                    Dim::STAGING_LAYERS,
                    wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc);
```
REPLACE
```
                snapshotStagingTexture_ = makeTextureArray("Snapshot Staging",
                    Dim::STAGING_LAYERS,
                    wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc,
                    1u);   // MIP_0 — a shot is one level; the promotion copies one level
```

### M1.4 — the authored staging carries the chain (`src/cartridges/the_board/realization/state.hpp`)

FIND
```
                authoredStagingTexture_ = makeTextureArray("Authored Staging",
                    Dim::STAGING_LAYERS,
                    wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc);
```
REPLACE
```
                authoredStagingTexture_ = makeTextureArray("Authored Staging",
                    Dim::STAGING_LAYERS,
                    wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc,
                    Dim::PAINTING_MIP_LEVELS);   // MIP_0 — the chain is uploaded here and copied from here
```

### M1.5 — the exhibition carries the chain (`src/cartridges/the_board/realization/state.hpp`)

FIND
```
                exhibitionTexture_ = makeTextureArray("Exhibition",
                    Dim::EXHIBITION_LAYERS,
                    wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::TextureBinding);
```
REPLACE
```
                exhibitionTexture_ = makeTextureArray("Exhibition",
                    Dim::EXHIBITION_LAYERS,
                    wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::TextureBinding,
                    Dim::PAINTING_MIP_LEVELS);   // MIP_0 — sampled through the chain (paintings) or at level 0 (photographs)
```

### M1.6 — the sampler filters between levels (`src/cartridges/the_board/realization/state.hpp`)

FIND
```
                    desc.label = "Painting Sampler (bilinear, clamp)";
                    desc.magFilter = wgpu::FilterMode::Linear;
                    desc.minFilter = wgpu::FilterMode::Linear;
                    desc.addressModeU = wgpu::AddressMode::ClampToEdge;
                    desc.addressModeV = wgpu::AddressMode::ClampToEdge;
                    paintingSampler_ = device_.CreateSampler(&desc);
```
REPLACE
```
                    desc.label = "Painting Sampler (trilinear, clamp)";
                    desc.magFilter = wgpu::FilterMode::Linear;
                    desc.minFilter = wgpu::FilterMode::Linear;
                    desc.mipmapFilter = wgpu::MipmapFilterMode::Linear;   // MIP_0 — between levels too; a level-0 sample (photographs) ignores it
                    desc.addressModeU = wgpu::AddressMode::ClampToEdge;
                    desc.addressModeV = wgpu::AddressMode::ClampToEdge;
                    paintingSampler_ = device_.CreateSampler(&desc);
```

### M1.7 — the promotion copies the chain, or one level (`src/cartridges/the_board/realization/state.hpp`)

FIND
```
            void promote_to_exhibition(wgpu::CommandEncoder& encoder,
                wgpu::Texture srcTexture, uint32_t srcLayer,
                uint32_t dstLayer)
            {
                wgpu::TexelCopyTextureInfo src{};
                src.texture = srcTexture;
                src.mipLevel = 0;
                src.origin = { 0, 0, srcLayer };
                src.aspect = wgpu::TextureAspect::All;

                wgpu::TexelCopyTextureInfo dst{};
                dst.texture = exhibitionTexture_;
                dst.mipLevel = 0;
                dst.origin = { 0, 0, dstLayer };
                dst.aspect = wgpu::TextureAspect::All;

                wgpu::Extent3D extent = { Dim::PAINTING_RESOLUTION, Dim::PAINTING_RESOLUTION, 1 };
                encoder.CopyTextureToTexture(&src, &dst, &extent);
            }
```
REPLACE
```
            // MIP_0 — `levels`: the chain's length for an authored source, 1
            // for a shot. Each level is one full copy of its own extent, so
            // the full-layer law above holds at every level the source has;
            // a photograph's higher levels stay as the layer's last tenant
            // left them and are never sampled (sample_exhibition, world.wgsl).
            void promote_to_exhibition(wgpu::CommandEncoder& encoder,
                wgpu::Texture srcTexture, uint32_t srcLayer,
                uint32_t dstLayer, uint32_t levels)
            {
                for (uint32_t level = 0; level < levels; ++level) {
                    wgpu::TexelCopyTextureInfo src{};
                    src.texture = srcTexture;
                    src.mipLevel = level;
                    src.origin = { 0, 0, srcLayer };
                    src.aspect = wgpu::TextureAspect::All;

                    wgpu::TexelCopyTextureInfo dst{};
                    dst.texture = exhibitionTexture_;
                    dst.mipLevel = level;
                    dst.origin = { 0, 0, dstLayer };
                    dst.aspect = wgpu::TextureAspect::All;

                    const uint32_t side = Dim::PAINTING_RESOLUTION >> level;
                    wgpu::Extent3D extent = { side, side, 1 };
                    encoder.CopyTextureToTexture(&src, &dst, &extent);
                }
            }
```

### M1.8 — every authored upload builds its chain (`src/cartridges/the_board/realization/state.hpp`)

FIND
```
                wgpu::Extent3D extent = { width, height, 1 };
                queue.WriteTexture(&dest, src, width * height * 4, &layout, &extent);
            }

            void fill_painting_layer_solid(wgpu::Queue& queue, uint32_t layer,
```
REPLACE
```
                wgpu::Extent3D extent = { width, height, 1 };
                queue.WriteTexture(&dest, src, width * height * 4, &layout, &extent);

                // MIP_0 — THE CHAIN, built here so every authored upload has
                // one (the loader's padded square and the boot's solid fill
                // alike): each level is the 2x2 box mean of the level above,
                // in the texel order already chosen. WriteTexture's row pitch
                // has no 256-byte law (that is the buffer copies'), so the
                // small levels write as they are. The loader replicates the
                // picture's edge across the pad before calling here, so no
                // level's border blends with black (gallery.hpp).
                std::vector<uint8_t> prev(src, src + (size_t)width * height * 4);
                uint32_t w = width, h = height;
                for (uint32_t level = 1; level < Dim::PAINTING_MIP_LEVELS && w > 1 && h > 1; ++level) {
                    const uint32_t nw = w / 2, nh = h / 2;
                    std::vector<uint8_t> next((size_t)nw * nh * 4);
                    for (uint32_t y = 0; y < nh; ++y) {
                        for (uint32_t x = 0; x < nw; ++x) {
                            const size_t a = ((size_t)(2 * y) * w + 2 * x) * 4;
                            const size_t b = a + (size_t)w * 4;
                            for (uint32_t ch = 0; ch < 4; ++ch) {
                                const uint32_t sum = prev[a + ch] + prev[a + 4 + ch] + prev[b + ch] + prev[b + 4 + ch];
                                next[((size_t)y * nw + x) * 4 + ch] = (uint8_t)((sum + 2u) / 4u);
                            }
                        }
                    }
                    dest.mipLevel = level;
                    layout.bytesPerRow = nw * 4;
                    layout.rowsPerImage = nh;
                    wgpu::Extent3D e = { nw, nh, 1 };
                    queue.WriteTexture(&dest, next.data(), (size_t)nw * nh * 4, &layout, &e);
                    prev.swap(next); w = nw; h = nh;
                }
            }

            void fill_painting_layer_solid(wgpu::Queue& queue, uint32_t layer,
```

### M1.9 — the estate census reads the lambda's fourth argument (`tools/binding_gen.py`)

FIND
```
        args = _split_args(m.group(3))
        if len(args) != 2:
            stop("RESOURCES: unparsed makeTextureArray for %s" % m.group(1))
```
REPLACE
```
        args = _split_args(m.group(3))
        # MIP_0 — the lambda gained a fourth argument, the mip count; the
        # row's shape (size, format) is still the lambda's own.
        if len(args) not in (2, 3):
            stop("RESOURCES: unparsed makeTextureArray for %s" % m.group(1))
```

### M1.10 — the pad is the edge, replicated; the loader's call is unchanged (`src/cartridges/the_board/bodies/gallery.hpp`)

FIND
```
    gpu.upload_authored_painting(queue, staging_layer, padded.data(), RES, RES);

    auto& rec = gs.authored_staging[staging_layer];
```
REPLACE
```
    // MIP_0 — THE PAD IS THE EDGE, REPLICATED. The square beyond dst_w x
    // dst_h was zero (transparent black). A chain averages 2x2 blocks, so
    // at every coarser level the picture's border texels would blend with
    // the pad and a dark rim would grow with distance. Replicating the last
    // column and the last row across the pad makes every level's border
    // blend with itself. The quads never sample the pad directly (uv runs 0
    // to uv_scale) and the postcard crops it away (crop_w / crop_h), so
    // nothing else moves.
    if (dst_w > 0 && dst_h > 0) {
        for (uint32_t dy = 0; dy < dst_h; ++dy) {
            const uint8_t* edge = &padded[((size_t)dy * RES + (dst_w - 1)) * 4];
            for (uint32_t dx = dst_w; dx < RES; ++dx)
                std::memcpy(&padded[((size_t)dy * RES + dx) * 4], edge, 4);
        }
        for (uint32_t dy = dst_h; dy < RES; ++dy)
            std::memcpy(&padded[(size_t)dy * RES * 4], &padded[(size_t)(dst_h - 1) * RES * 4], (size_t)RES * 4);
    }
    gpu.upload_authored_painting(queue, staging_layer, padded.data(), RES, RES);

    auto& rec = gs.authored_staging[staging_layer];
```

### M1.11 — the promotion names its levels by source (`src/cartridges/the_board/bodies/gallery.hpp`)

FIND
```
        c->gpuState_.promote_to_exhibition(encoder, src, p.staging_layer, p.exhibition_layer);
```
REPLACE
```
        c->gpuState_.promote_to_exhibition(encoder, src, p.staging_layer, p.exhibition_layer,
                                           p.is_snapshot ? 1u : Dim::PAINTING_MIP_LEVELS);   // MIP_0
```


### WITNESS — U1
```
grep -c "PAINTING_MIP_LEVELS = 11"     src/cartridges/the_board/realization/state.hpp   # 1
grep -c "PAINTING_MIP_LEVELS"          src/cartridges/the_board/realization/state.hpp   # 6
grep -c "desc.mipLevelCount = mips"    src/cartridges/the_board/realization/state.hpp   # 1
grep -c "MipmapFilterMode::Linear"     src/cartridges/the_board/realization/state.hpp   # 1
grep -c "uint32_t dstLayer, uint32_t levels" src/cartridges/the_board/realization/state.hpp   # 1
grep -c "p.is_snapshot ? 1u : Dim::PAINTING_MIP_LEVELS" src/cartridges/the_board/bodies/gallery.hpp   # 1
grep -c "THE PAD IS THE EDGE, REPLICATED" src/cartridges/the_board/bodies/gallery.hpp   # 1
grep -c "if len(args) not in (2, 3)"   tools/binding_gen.py                             # 1
sh tools/gates/glaw1/run.sh && python3 tools/gates/console_gate/run.py       # GREEN / PASS
python3 tools/binding_gen.py --write && git status --short audit src/console  # byte-identical: nothing listed
python3 tools/binding_gen.py --check                                          # every row PASS; S-6 until the push
```
Commit: `MIP_0 U1 — the exhibition's chain: 11 levels on the exhibition and the authored staging, built on the CPU at every authored upload, copied level by level at promotion; the pad is the replicated edge; trilinear sampler`

---

## U2 — SAMPLING BY KIND

### M2.1 — the gallery varying carries the kind (`src/cartridges/the_board/realization/world.wgsl`)

FIND
```
struct GalleryVarying {
    @builtin(position) clip_pos: vec4<f32>,
    @location(0) uv: vec2<f32>,
    @location(1) world_pos: vec3<f32>,
    @location(2) world_normal: vec3<f32>,
    @location(3) @interpolate(flat) texture_layer: u32,
};
```
REPLACE
```
struct GalleryVarying {
    @builtin(position) clip_pos: vec4<f32>,
    @location(0) uv: vec2<f32>,
    @location(1) world_pos: vec3<f32>,
    @location(2) world_normal: vec3<f32>,
    @location(3) @interpolate(flat) texture_layer: u32,
    @location(4) @interpolate(flat) content_source: u32,   // MIP_0 — painting or photograph: which way to sample
};
```

### M2.2 — the gallery vertex shader writes the kind (`src/cartridges/the_board/realization/world.wgsl`)

FIND
```
    out.world_normal = g.fwd;
    out.texture_layer = g.texture_layer;
    return out;
}
```
REPLACE
```
    out.world_normal = g.fwd;
    out.texture_layer = g.texture_layer;
    out.content_source = painting_slots[min(iid, PAINTING_MAX_SLOTS - 1u)].content_source;   // MIP_0
    return out;
}
```

### M2.3 — sampling by kind, and the outdoor frame's sample (`src/cartridges/the_board/realization/world.wgsl`)

FIND
```
@fragment
fn gallery_frame_fs(in: GalleryVarying) -> @location(0) vec4<f32> {
    let painting_color = textureSample(painting_array, painting_sampler_filt, in.uv, in.texture_layer);
```
REPLACE
```
// ═══ MIP_0 — SAMPLING THE EXHIBITION, BY KIND ═══════════════════════════
// A painting samples through its chain (built on the CPU at decode, copied
// level by level at promotion); a photograph samples level 0 only, because
// its higher levels are never written (the snapshot staging has one level,
// the promotion copies one). The choice is per instance, so it is NOT
// uniform control flow, and textureSample (implicit derivatives) may not sit
// inside it. The derivatives are taken here, uniformly, and handed to
// textureSampleGrad, which may. CONTENT_SOURCE_SNAPSHOT mirrors
// ContentSource::SNAPSHOT (state.hpp) — an L3 mirror with no bridge.
const CONTENT_SOURCE_SNAPSHOT: u32 = 1u;
fn sample_exhibition(uv: vec2<f32>, layer: u32, kind: u32) -> vec4<f32> {
    let ddx = dpdx(uv);
    let ddy = dpdy(uv);
    if (kind == CONTENT_SOURCE_SNAPSHOT) {
        return textureSampleLevel(painting_array, painting_sampler_filt, uv, layer, 0.0);
    }
    return textureSampleGrad(painting_array, painting_sampler_filt, uv, layer, ddx, ddy);
}

@fragment
fn gallery_frame_fs(in: GalleryVarying) -> @location(0) vec4<f32> {
    let painting_color = sample_exhibition(in.uv, in.texture_layer, in.content_source);   // MIP_0
```

### M2.4 — the indoor canvas's sample (`src/cartridges/the_board/realization/world.wgsl`)

FIND
```
    let slot = painting_slots[in.painting_index];
    let tex_color = textureSample(painting_array, painting_sampler_filt, in.uv, slot.texture_layer);
```
REPLACE
```
    let slot = painting_slots[in.painting_index];
    let tex_color = sample_exhibition(in.uv, slot.texture_layer, slot.content_source);   // MIP_0
```

### M2.5 — G-LAW 2 admits the builtin the shader now calls (`tools/gates/glaw2/run.py`)

FIND
```
PREDECLARED = {"array", "atomicSub"}
```
REPLACE
```
#   dpdx, dpdy, textureSampleGrad  predeclared builtin functions (MIP_0's
#               sample_exhibition is their first use: the derivatives are
#               taken in uniform control flow and handed to the explicit-
#               gradient sample; textureSample and textureSampleLevel were
#               in the recorded set).
PREDECLARED = {"array", "atomicSub", "dpdx", "dpdy", "textureSampleGrad"}
```


### WITNESS — U2
```
python3 tools/wgsl_gate.py                       # PASS — naga parses the helper; the uniformity rule is why the derivatives sit outside the branch
python3 tools/gates/glaw2/run.py                 # GREEN — 328 fn, 335 const
grep -c "fn sample_exhibition"                   src/cartridges/the_board/realization/world.wgsl   # 1
grep -c "sample_exhibition(in.uv"                src/cartridges/the_board/realization/world.wgsl   # 2 — the two fragment shaders
grep -c "textureSample(painting_array"           src/cartridges/the_board/realization/world.wgsl   # 0 — no bare sample of the exhibition survives
grep -c '"dpdx", "dpdy", "textureSampleGrad"'    tools/gates/glaw2/run.py                          # 1
```
If `wgsl_gate.py` refuses `textureSampleGrad` on a `texture_2d_array` (it should not — core
WGSL), HALT U2 and report naga's exact line: the alternative is a level computed by hand
(`log2(max(length(ddx), length(ddy)) * 1024)` into `textureSampleLevel`) and it is a
different round.

Commit: `MIP_0 U2 — sampling by kind: a painting through its chain (textureSampleGrad, derivatives taken in uniform flow), a photograph at level 0; G-LAW 2 admits the three builtins`

---

## U3 — LEDGERS, THE REGISTER, THE PUSH

```
python3 tools/binding_ledger.py && python3 tools/command_census.py && python3 tools/mirror_census.py && python3 tools/organ_ledger.py
python3 tools/binding_ledger.py --check && python3 tools/command_census.py --check && python3 tools/mirror_census.py --check && python3 tools/organ_ledger.py --check
python3 tools/organ_readers.py && python3 tools/organ_gap.py --gate
python3 tools/gates/score/run.py && python3 tools/gates/shell_gate/run.py && python3 tools/gates/glaw2/run.py && python3 tools/gates/sha256_gate/run.py && python3 tools/wgsl_gate.py
python3 tools/binding_gen.py --check      # all PASS; S-6 until the push
```
Rehearsed diffstat: three ledgers, pins and line-cites (414 insertions, 411 deletions —
`world.wgsl` moved every cite below the helper).

`docs/OPEN.md`, at the top of the register:
```
## MIP_0 — THE EXHIBITION'S CHAIN (on `claude/mip-0`; Jean's gates open)

Paintings sample through an eleven-level chain the CPU builds at upload; photographs
sample level 0 as before; the two are told apart by the slot's content_source. No new
pass, pipeline or binding. +96 MiB where the chain lives.

| ruling | where it lives now |
|---|---|
| The chain is built where the texels already pass through the CPU | `state.hpp` `upload_authored_painting`; `Dim::PAINTING_MIP_LEVELS` |
| A photograph has no chain and asks for none — level 0, by kind | `world.wgsl` `sample_exhibition`, `CONTENT_SOURCE_SNAPSHOT` (L3 mirror of `ContentSource::SNAPSHOT`) |
| Derivatives in uniform flow, the sample under the branch | `sample_exhibition` (`dpdx`/`dpdy` → `textureSampleGrad`) |
| The pad is the edge, replicated | `gallery.hpp` `authored_stage_decoded_image` |
| The promotion copies as many levels as its source has | `state.hpp` `promote_to_exhibition(…, levels)`; `gallery.hpp` `drain_gallery_promotions` |
| The estate census reads the lambda's fourth argument | `tools/binding_gen.py` |

### Residuals — MIP_0
- **Photographs still shimmer at a distance** — unchanged from before, by ruling 2. A
  chain for them is one blit pass (fullscreen triangle, level k → k+1, per promoted
  layer): a pipeline, a layout, seats, a census enrollment. Priced; built when the wall's
  seven photographs are worth a mechanism.
- **+96 MiB of estate** (exhibition 213.3, authored staging 170.7). The phones' residency
  row is this round's gate as it was PLATE_0's; the lever if one refuses is the chain on
  the exhibition only, with the levels written at promotion from a CPU copy the staging
  record retains (same bytes, moved from GPU to heap).
- **The CPU decode grows by a third** (the box means); the `authored6` mark is still the
  witness, and the browser-decode road registered at PLATE_0 still answers it.
```
Commit: `MIP_0 U3 — ledgers regenerated (state/gallery/world.wgsl pins); OPEN.md: MIP_0 open on claude/mip-0, residuals registered`
Push: `git push -u origin claude/mip-0`.

---

## JEAN'S GATES

1. **The boot card.** `[GPU Budget]` textures **+96.0 MiB** over PLATE_0's line; the
   leaderboard reads Exhibition 213.3 and Authored Staging 170.7. The Pixel and the iPhone
   boot with tabs beside them, as at PLATE_0 (L48).
2. **The far wall is quiet.** From the ribbon and from across a gallery, a painting no
   longer shimmers as the eye moves; a photograph looks exactly as it did (its row is *no
   change*).
3. **Up close, nothing moved.** Stand before a painting at PLATE_0's distance: the same
   texels (level 0 is byte-identical). The border shows no rim at any distance — ruling 4's
   witness.
4. **The postcard is untouched.** Take a painting: the same JPEG as PLATE_0's (level 0,
   cropped). Take a photograph: the same as before.
5. **The meter, on the laptop.** `main_pass` in a gallery on foot and from the ribbon,
   PLATE_0 vs this branch: this is where the laptop's hit should come back.
6. **Firefox** (the immediates fallback card is still the boundary): naga's own parse of
   `textureSampleGrad` on an array is the third-arm witness of this shader.
