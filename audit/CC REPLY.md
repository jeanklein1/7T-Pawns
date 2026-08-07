U1 census is rich — `SHADOW_MAP_SIZE` is an explicit two-room twin, and `renderer.hpp:2118` mentions `8.192/4096`, which looks like a world-unit constant tuned against 4096². Reading both:

Critical U1 finding located. Now the U2 and U3 censuses:

U2's scale target may already derive from the same constant (`gallery.hpp:1521`). Confirming, and running the U3 photographer census:

U2's scale target **already derives from `Dim::PAINTING_RESOLUTION`** (gallery.hpp:1521) — one fact, one home already. Now the decisive U3 question: is the photographer driven by possession, or by a cadence?

**U3 is decided: the photographer is REACHABLE.** It's driven by the *point's walked distance*, not possession — confirming before I STOP:

U3 confirmed reachable — `capture_snapshot` writes the staging records and the promotion path consumes them. Now executing U1:

**U2.** One note first — the single constant also governs Snapshot Staging and the offscreen pair, so the saving exceeds the handoff's 216 MiB. Reading the constant's home:

The tree has already ruled this: *"ONE RESOLUTION, for everything that can land in an exhibition layer. A second, smaller one was tried and reverted."* Splitting it would recreate that exact defect — so one constant moves:

**U3 → STOP.** The premise is falsified; reporting it rather than editing. Now U4's limits census — the table before any edit:

Workgroups max at 16×16 = 256 — exactly the core default, not over. Completing the size census:

Census complete. Reading the current web device request to restructure it:

Now U4. Restructuring the device request so it can be made twice:

Now replacing the old inline device request with the call:

# PORT_5 — THE BUDGET DIET — campaign report

**Three units landed, one STOP.** Pushed `2773f25..7e5d604`.

| unit | commit |
|---|---|
| U1 shadows 2048² | `cef0903` |
| U2 paintings 512² | `1d70726` |
| U3 snapshot staging | **STOP — premise falsified** |
| U4 modest limits | `7e5d604` |

## U1 — derived sites, and the one world-unit site

`SHADOW_MAP_SIZE` is **two homes by declared design** — an L3 mirror with no compile-time bridge (the WGSL side is runtime-loaded). Both changed: `state.hpp:256` and `world.wgsl:3759`.

**Every derived site follows automatically** — that's the finding, not an assumption. All are ratios or texel-relative: `SHADOW_TEXEL_WORLD` (0.20508 → 0.41016 wu), `TEXEL_UV`, the sun normal offset (`SHADOW_TEXEL_WORLD × PCF_RADIUS_TEXELS`), `spot_texel_world`, the spot PCF `texel_size`, `TILE_W/TILE_H` (indoor tiles now 1024×2048), and both texture descriptors. No static_assert spans the seam; there cannot be one.

**The world-unit site, reported and deliberately not retuned:**

```
renderer.hpp:2171   shadowDepth.depthBiasClamp = 1.527e-3f;   // = 1.680 wu / 1100.0 wu
```

Its own comment block is explicit that 1.680 wu was carried across UMBRA_5 **by texel ratio**, and that the retired per-texel form "tracked RESOLUTION" — so it was tuned at the old texel, and the texel just doubled. If you see peter-panning or acne: `1.680 × 2 = 3.360`, `3.360 / 1100.0 = **3.055e-3**`. Not applied — a ceiling too large is as real a defect as one too small, and you judge from the render. The other two instruments need nothing: `depthBiasSlopeScale` is dimensionless in resolution, and the normal-offset floor is already in texels.

## U2 — the scale target was already one home

`gallery.hpp:1521` reads `constexpr uint32_t RES = Dim::PAINTING_RESOLUTION;`, and every downstream number derives from `RES` — the fit ratio, the `dst_w/dst_h` clamps, the padded upload buffer, the `uv_scale_x/y` the quad samples with, and the `[Authored] Scaled →` log itself. **No correction needed.** No WGSL twin either (shaders sample by uv; the resolution never enters as a number) — checked, not assumed.

**Divergence in our favour:** the unit scoped 216 MiB across two arrays; the constant sizes **five** textures, so the real saving is **318 MiB** (Exhibition −120, Authored Staging −96, Snapshot Staging −96, the two offscreen targets −6). I did *not* split the constant to hold Snapshot Staging at 1024, because the tree rules against it four lines above: *"ONE RESOLUTION… A second, smaller one was tried and reverted: it made promotion a PARTIAL write."* Snapshot staging is a promotion **source** — a 1024 source into a 512 destination is exactly that reverted defect.

## U3 — STOP: the photographer is live

The premise is false. `update_photographer` (gallery.hpp:783) never reads `point_.host`; it is driven by the **point's walked distance**:

```
step = |Δ(point_.x, point_.z)| → cumulative_distance ≥ next_threshold
  → pending_shots = sample_shot_count() → capture_snapshot(...)
```

Post-CUT_1e the pawn *is* the point, so walking the pawn triggers it. `capture_snapshot` writes `gs.snapshot_staging[layer]` (gallery.hpp:859), copies into `snapshot_staging_texture()` (gallery.hpp:1493), and the promotion path consumes those records (gallery.hpp:1309/1345) into Exhibition. `RPhase::SnapshotPass` is gated `true`; `WitnessPhotographer` rides `ROSTER.gallery`, which is on. Reporting and stopping — removing a live feature is your call.

## U4 — the limits table

| limit | need | core default | verdict |
|---|---|---|---|
| maxStorageBuffersPerShaderStage | **9** | 8 | **EXCEEDS** |
| maxTextureArrayLayers | 225 | 256 | ok (OPT_1b) |
| maxTextureDimension2D | 2048 | 8192 | ok (PORT_5a) |
| maxUniformBufferBindingSize | 16,400 | 65,536 | ok |
| maxStorageBufferBindingSize | ~3.3 MiB | 128 MiB | ok |
| maxComputeInvocationsPerWorkgroup | 256 | 256 | ok, *at* it |
| maxComputeWorkgroupSizeX/Y | 16 | 256 | ok |

**Exactly one exceedance remains** — the one PORT_0 named. When C6 merges, the `9` becomes an `8` and this is a pure defaults request.

Two nets, because a mis-census must degrade to today's behavior: (1) on failure, the reason prints verbatim and the request reissues with full passthrough; (2) on *success with limits below the censused floor*, the device is discarded and passthrough reissued — that net exists because the modest set relies on value-initialised `wgpu::Limits` meaning "undefined ⇒ default", which is the documented contract but which I cannot compile to confirm, and whose failure mode would otherwise surface at pipeline creation far from this line.

## Expected `[GPU Budget]`

**681.1 → ~267 MiB** (−414). Since U3 stopped, Snapshot Staging stays at 32 MiB rather than vanishing, so this lands above the handoff's ~241 projection — the 512² cut already took it from 128 to 32. New top five: **Patch Heightfield 112.5**, Exhibition 40, Snapshot Staging 32, Authored Staging 32, Shadow Map 16. Patch Heightfield becomes the largest single allocation, as projected.

Nothing was compiled. Encoding clean including `world.wgsl` (L1: LF, no BOM); brace balance clean. `glaw1` gates all three — U1 and U2 are both-twin changes.