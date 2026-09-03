#pragma once

// THE_BOARD CARTRIDGE — GPU State Management (Rasterized)
// =============================================================
//
// CPU/GPU data contract: structs, buffers, textures, bind groups.
// Terrain grid is GPU-derived from vertex_index — zero geometry uploaded.
//
// Binding numbers: realization/binding_registry.hpp (C6 — the single source).
//
// ─── INTERIOR MAP (grep the names; no line anchors) ───────────────
//   Dim::                world dimensions & capacities (veil-chain law)
//   Idle:: / Coupling::  boot-rest values / the mute-bit registry
//   GPUFrameSignal       the per-frame hot uniform (sky tail: E-3)
//   GPUDesignConfig      the design mirror — paneled by ─── system
//                        groups; grown per L5/L4 (docs/LAWS.md)
//   GPU* DTO region      the mirror vocabulary, per-family banners,
//                        every struct sizeof-witnessed
//   class GPUState
//     init()             create{Buffers,MeshBuffers,Textures,
//                        Samplers,BindGroups} → initializeState
//     UPLOAD COLLAPSE    the three write shapes — the cadence
//                        taxonomy (dirty / commit / count); the
//                        per-frame hot tier stays bespoke, offsetof-
//                        witnessed
//     upload_signal/config   the drains (disjoint-region law)
//     set_* surface      grouped by ─── system headers
//     bind groups        bind::g0/g1 only — the registry is the map

#include "core/instruments.hpp"                  // THE INSTRUMENTS DIAL: INSTRUMENTS.frame_meter gates the GPU half's creation + arming (compile-time, T7_INSTRUMENTS; default off)
#include "core/boot_params.hpp"                  // DOMESDAY_2 B10: effective_msaa — boot-read sample count for the snapshot targets
#include "cartridges/the_board/demos/demo.hpp"   // ROSTER via the selected sentence (GPUState::init gates creation)
#include "cartridges/the_board/realization/binding_registry.hpp"  // C6: bind::g0::* / bind::g1::* — the single source of truth for binding NUMBERS (the layout+group pair references one named const)
#include "cartridges/the_board/surface/terrain_looks.hpp"          // THE TERRAIN_LOOKS PANEL (C++ room): palette quartet REST + motion/mode rest pins — boot init reads the panel
#include "cartridges/the_board/bodies/pawn_figures.hpp"            // typed figure registry (H1: PawnFigureDef / PAWN_FIGURES) — constexpr-only, self-contained
#include "cartridges/the_board/contracts/point.hpp"                 // POINT_BUBBLE_RADIUS, CAMERA_CHASE_FF, CAMERA_PUSH_* — source of truth for the point-house boot pins
#include "cartridges/the_board/contracts/control_panel.hpp"         // THE PANEL — the field dials' rests, boot-pinned into the config
#include "cartridges/the_board/contracts/ribbon_surface.hpp"        // RIBBON_1 — RIBBON_LIVE: the ribbon's dials, boot-pinned into the config beside the field's
#include <webgpu/webgpu_cpp.h>
#include <cstdio>    // the params ring's clamp report
#include <iostream>  // ATRIUM_11 — the camera witness's one line
#include <iomanip>   // ATRIUM_11 — std::fixed, std::setprecision, same
#include <cstring>
#include <array>
#include <vector>
#include <cmath>
#include <type_traits>   // the frame meter's timestamp-writes name binding

namespace t7 {
    namespace the_board {

        namespace Dim {
            // Grid dimensions

            // Entity meshes — pawn
            constexpr int      PAWN_SEGMENTS = 48;
            constexpr int      PAWN_RINGS = 16;
            constexpr uint32_t PAWN_BODY_VERTICES = (PAWN_RINGS - 1) * PAWN_SEGMENTS * 6;    // 4320
            constexpr uint32_t PAWN_CAP_VERTICES = PAWN_SEGMENTS * 3;                        // 144
            constexpr uint32_t PAWN_VERTEX_COUNT = PAWN_BODY_VERTICES + PAWN_CAP_VERTICES;   // 4464

            // Entity meshes — sphere
            constexpr int      SPHERE_STACKS = 20;
            constexpr int      SPHERE_SLICES = 20;

            // Entity meshes — sky ribbon (square tube)
            constexpr uint32_t RIBBON_MAX_RINGS = 400;
            constexpr uint32_t RIBBON_TUBE_VERTS_PER_SEG = 24;    // 4 faces × 2 tri × 3 verts
            constexpr uint32_t RIBBON_CAP_VERTS = 12;    // 2 caps × 2 tri × 3 verts
            // RIBBON_1 — the body's own dimensions. The spine is a CHORD ring:
            // ring k reads slot head−k, so the ring needs one slot per ring plus
            // the head's own, plus the one the head writes next and no ring reads
            // this frame. The emit table is the field's coarse view of the tube;
            // the deform table is two ping-pong halves of (offset, velocity).
            constexpr uint32_t RIBBON_SPINE_SLOTS  = RIBBON_MAX_RINGS + 2;     // 402
            constexpr uint32_t RIBBON_EMIT_STRIDE  = 4;
            constexpr uint32_t RIBBON_EMIT_SLOTS   = RIBBON_MAX_RINGS / RIBBON_EMIT_STRIDE;   // 100
            // RIBBON_2: the emit table is TWO halves. The body writes half
            // (tick & 1); the head and every ring read the other, so the body
            // can be a thing in the Sky Rule without reading its own frame;
            // the field, which runs after the body, reads the written one.
            constexpr uint32_t RIBBON_EMIT_TABLE   = 2 * RIBBON_EMIT_SLOTS;                   // 200
            constexpr uint32_t RIBBON_DEFORM_SLOTS = 4 * RIBBON_MAX_RINGS;     // 2 halves × (offset, velocity) × rings
            // THE DRAW IS THE LIVE COUNT (RIBBON_1): the ribbon draws the rings it
            // has, not the rings it could have. RIBBON_VERTEX_COUNT stays as the
            // ceiling the buffers are sized against.
            constexpr uint32_t ribbon_vertex_count_for(uint32_t rings) {
                return (rings - 1) * RIBBON_TUBE_VERTS_PER_SEG + RIBBON_CAP_VERTS;
            }
            constexpr uint32_t RIBBON_VERTEX_COUNT = ribbon_vertex_count_for(RIBBON_MAX_RINGS);


            // Patch streaming system
            constexpr float    PATCH_EXTENT = 50.0f;    // world units per patch side
            constexpr uint32_t PATCH_CELL_N = 16;      // cell color texture side per patch
            // THE CELL — one spelling, and this is it. The pawn aura, the GoL
            // zone extent + corner snap, the card's window origin and the
            // cell-exactness assert below all read THIS name.
            // L3 MIRROR: world.wgsl PATCH_CELL_SIZE. Change both rooms together.
            constexpr float    PATCH_CELL_SIZE = PATCH_EXTENT / (float)PATCH_CELL_N;  // 3.125
            constexpr uint32_t PATCH_GRID_RADIUS = 3;       // inner priority radius (7×7)
            constexpr uint32_t PATCH_GRID_SIDE = 2 * PATCH_GRID_RADIUS + 1;       // 7
            constexpr uint32_t PATCH_PREGEN_RADIUS = 7;                                // deep pre-gen buffer (15×15, 350 world units; OPT_1b — fits default maxTextureArrayLayers, veil chain holds exactly: 7·50 = EXIST_RADIUS)
            constexpr uint32_t PATCH_PREGEN_SIDE = 2 * PATCH_PREGEN_RADIUS + 1;     // 15
            constexpr uint32_t MAX_ACTIVE_PATCHES = PATCH_PREGEN_SIDE * PATCH_PREGEN_SIDE; // 225

            // minUniformBufferOffsetAlignment, core default — the ring stride
            // for a dynamic uniform seat. A device that reports a SMALLER
            // alignment still accepts multiples of 256, so no floor is stood on.
            constexpr uint32_t UNIFORM_DYNAMIC_STRIDE = 256;

            // ── THE LIVE CARD (GROUND_CARD_1) ──
            // One 2D RGBA16F field over the ground window, point-centered.
            // R = waves+pulses Δh; G/B = wave ∂x/∂z (waves-only this campaign
            // — pulse shading is Stage 6); A = raw GoL lift. Window ORIGIN
            // SNAPS to the PATCH_CELL_SIZE grid so a nearest fetch of .a is
            // cell-exact.
            // NOT "fully rewritten per frame" — it is rewritten every frame
            // the card is LIVE, and the rest law (world.wgsl, above
            // write_live_card) skips the whole dispatch when it is not: the
            // caller returns before it, one clearing write on entry to rest.
            // L3 MIRROR: world.wgsl LIVE_CARD_SIZE / LIVE_CARD_EXTENT — same
            // names, same values, both rooms change together.
            constexpr uint32_t LIVE_CARD_SIZE   = 640;
            constexpr float    LIVE_CARD_EXTENT = 1000.0f;
            // IOS_3 A — THE MIRROR OF world.wgsl's const_assert, and it
            // guards a UB hazard rather than a size. write_live_card is
            // @workgroup_size(16, 16); 640 = 40 x 16 exactly, so no
            // workgroup is partial and its bounds guard never fires. The
            // bake earns this line by counter-example: PATCH_HEIGHTFIELD_N
            // is 65 = 4 x 16 + 1, its last workgroup row and column carry
            // one valid lane in sixteen, and its guard therefore sits BELOW
            // both workgroupBarrier() calls. A barrier reached by only some
            // lanes is undefined behaviour in WGSL — permissive desktop
            // compilers accept it, a strict one need not. If this assert
            // ever fires, the card kernel's guard must move below its three
            // barriers before the size changes.
            static_assert(LIVE_CARD_SIZE % 16 == 0,
                "LIVE_CARD_SIZE must be a multiple of write_live_card's 16x16 "
                "workgroup: a partial workgroup makes the bounds guard "
                "load-bearing, and it currently sits above no barrier only "
                "because it never fires");

            // THE WINDOW COVENANT, stated with the numbers that hold.
            // The origin snaps DOWN to the PATCH_CELL_SIZE grid, so the
            // window is not exactly centred: the guaranteed half-extent is
            //   +x/+z : EXTENT/2 − PATCH_CELL_SIZE = 496.875 wu
            //   −x/−z : EXTENT/2                   = 500.0   wu
            // What it must cover is NOT the EXIST ring. Agents and floaters
            // are EXIST-gated at 350, but flora, columns, arches and outdoor
            // paintings are PATCH-lifetime: they live as long as their host
            // patch, out to the allocation window, which reaches
            // (PATCH_PREGEN_RADIUS + 1) · PATCH_EXTENT = 400 wu per axis
            // (patches span [pawnGX−7, pawnGX+7] and the pawn sits anywhere
            // inside its own cell), 403.125 with the snap.
            //
            // AND THAT IS WHERE AN ANCHOR CAN BE, NOT WHERE A READ CAN BE
            // (LATTICE_4 R-A). Every entity VS samples the card at its
            // VERTEX position, and a mesh reaches past its anchor. The
            // binding case is the arch: `arch_vs` has NO ring gate (unlike
            // palm/cactus/blade, which kill per vertex beyond veil_ring), so
            // an arch anywhere in the allocation window is drawn and read at
            // every vertex. Its per-axis reach is half_span + half_depth,
            // and cpu_sample_gaussian clamps z to ±3, so the widest is
            // STANDARD's SPAN μ50 σ15 → half_span 47.5, plus half-depth 4.45
            // — 51.95; MONUMENTAL's is 53.00. The farthest card read is
            // therefore 403.125 + 53 = 456.125 wu, not 403.125.
            // Slack 40.75 wu against the +x/+z guarantee — was 93.75 when
            // measured against the anchor alone. Outside the window a sample
            // returns the ClampToEdge texel — sample_live_card does no bounds
            // test — so an under-sized window would mis-lift the far legs of
            // far arches rather than fault. SHRINK THIS WINDOW AGAINST
            // 456.125, never against 403.125.

            // CELL-EXACTNESS — the relation the nearest .a fetch stands on: one
            // patch cell is EXACTLY two card texels, so both texels inside a
            // cell were written from the same GoL cell and either one answers.
            // Stated in INTEGER arithmetic (both denominators cleared) so the
            // check never rests on float rounding:
            //   2·EXTENT/SIZE == PATCH_EXTENT/PATCH_CELL_N
            //     ⟺  2·EXTENT·PATCH_CELL_N == PATCH_EXTENT·SIZE
            static_assert(2u * (uint32_t)LIVE_CARD_EXTENT * PATCH_CELL_N
                          == (uint32_t)PATCH_EXTENT * LIVE_CARD_SIZE,
                "live card: one patch cell must be EXACTLY two card texels — "
                "the nearest .a fetch is cell-exact only if it is");
            // The same relation named through PATCH_CELL_SIZE, for the reader.
            // Exact here because every operand is a binary fraction.
            static_assert(2.0f * LIVE_CARD_EXTENT == PATCH_CELL_SIZE * (float)LIVE_CARD_SIZE,
                "live card: 2 × extent must equal PATCH_CELL_SIZE × size");

            // ONE writer dispatch since LATTICE_4 fused the pair; it divides
            // the card side by 16. The kernel's bounds check sits AFTER its
            // barrier (every lane must reach the barrier), so it tolerates a
            // remainder — but a remainder would still mean a partly-idle
            // trailing workgroup, and the divisibility is free. Pass 1's
            // by-8 half of this assert retired with pass 1.
            static_assert(LIVE_CARD_SIZE % 16u == 0u,
                "live card: SIZE must divide by the writer's workgroup side "
                "(16 = write_live_card)");

            // TILE_GRID ceiling — the pinned capacity pair's C++ half;
            // twin: world.wgsl TILE_GRID_CAPACITY. Authored, NOT derived
            // from the radius — the dial never touches it. Raise it in
            // BOTH rooms or glaw1/Dawn objects.
            constexpr uint32_t TILE_GRID_CAPACITY = 1024;

            // ═══ THE VEIL CHAIN (re-ruled) — THE RING is the DRAW authority; ═══
            // fog is ICING. Declared here (the registry pattern: authored
            // once, checked by asserts, never computed); point-anchored.
            //   RING (325 = 6.5 patches, Jean's enlargement from 5.5): the
            //     SOLE draw authority — terrain banding, entity cull, flora,
            //     agents, floaters all gate DRAW MEMBERSHIP on it. Metric:
            //     nearest-edge for patches; center±extent for entities.
            //     Nothing is drawn beyond the ring — no wall-colored
            //     silhouettes against the orb sky.
            //   ICING (δ 40): the NARROW fog band [RING−δ, RING] in shade_lit
            //     — draw-set joins materialize inside the fade. Cosmetic
            //     only; no geometry relies on it for concealment.
            //   LOD0 (175): the full-mesh/half-mesh terrain split (unchanged).
            //   EXIST (350; the pregen ring reaches exactly 350 since
            //     OPT_1b — PREGEN·EXTENT == EXIST, the assert below is now
            //     tight, not slack): existence eviction (agents unified at
            //     350 — V1; floaters 400, the flagged spawn-headroom fork).
            // LIVE values ride config (veil_ring/veil_icing/lod0_radius,
            // tunable — "ring at 6.5 feels right vs 5.5, config-tune live").
            constexpr float LOD0_RADIUS_DEFAULT = 3.5f * PATCH_EXTENT;   // 175
            constexpr float VEIL_RING_DEFAULT   = 6.84f * PATCH_EXTENT;  // 342 — THE RING (desk-tuned
                                                                         // from 6.5; the chain asserts
                                                                         // below hold, but EXIST is now
                                                                         // only 8 wu further out)
            constexpr float VEIL_ICING_DEFAULT  = 42.0f;                 // δ (~25-50, tunable)
            constexpr float EXIST_RADIUS        = 7.0f * PATCH_EXTENT;   // 350
            static_assert(PATCH_PREGEN_RADIUS * PATCH_EXTENT >= EXIST_RADIUS,
                "VEIL CHAIN: PREGEN >= EXIST (nothing exists off resident ground)");
            static_assert(EXIST_RADIUS > VEIL_RING_DEFAULT,
                "VEIL CHAIN: EXIST > RING (existence outlives the draw set)");
            static_assert(VEIL_RING_DEFAULT > LOD0_RADIUS_DEFAULT,
                "VEIL CHAIN: RING > LOD0 (the draw set contains the full-mesh core)");
            static_assert(VEIL_RING_DEFAULT - VEIL_ICING_DEFAULT > LOD0_RADIUS_DEFAULT,
                "VEIL CHAIN: the icing band sits wholly outside the LOD0 core");

            // ── THE MOSAIC (MOSAIC_0/1/2) — trencadís rests ──
            // SHARD: wu per cell (~10× under the terrain cell 3.125; a
            //   per-entity batch jitters it ±30%). PASSAGE: the coarse
            //   palette lattice — slightly larger than a body. BLEND:
            //   boundary-zone width as a fraction of PASSAGE, one dial
            //   for both halves of the zone. FACET: plate-lean strength.
            // All live-tunable via config; these are the rests.
            constexpr float MOSAIC_SHARD_SIZE_DEFAULT = 0.30f;
            constexpr float MOSAIC_PASSAGE_DEFAULT    = 12.0f;
            // MOSAIC_2: the radius/icing pair is DELETED. Grain is now
            // 1 − veil_t — the veil's own icing smoothstep — so a body
            // materializes at the ring already ceramic and gains its
            // grain across exactly the band where it materializes. One
            // fact, one home; the veil chain assert above is the only
            // chain there was ever reason to have.
            constexpr float MOSAIC_BLEND_DEFAULT      = 0.18f;
            constexpr float MOSAIC_FACET_DEFAULT      = 0.25f;
            constexpr uint32_t PATCH_MESH_N = 64;      // mesh subdivisions per patch (LOD-0)
            // THE BAKE IS THE READER'S SHADOW (LATTICE_1). The heightfield is
            // baked at the lattice patch_terrain_vs decodes: texel i IS lattice
            // point i, so the VS reads a value that was computed FOR it rather
            // than interpolated toward it. It was 256 — four texels per mesh
            // step, 15.5x the memory, and every one of them thrown away by a
            // bilinear fetch that landed between them.
            //
            // DECLARED HERE, after PATCH_MESH_N, because C++ needs the operand
            // first; WGSL's twin may sit anywhere and does.
            constexpr uint32_t PATCH_HEIGHTFIELD_N = PATCH_MESH_N + 1;   // 65 — one texel per lattice point
            constexpr uint32_t PATCH_INDEX_COUNT = PATCH_MESH_N * PATCH_MESH_N * 6;

            // ── THE UNIFIED GROUND (UNIFIED_GROUND_1) ──
            // Vertex-index bands. Legacy space [0, UG_CAP_BASE) keeps its
            // decode (grid+skirt — the LOD1/soft space). CAP band: 25 cell-
            // owned verts per cell (5×5, cells lift independently). BASE
            // band: 16 curtain-bottom twins per cell (no lift — the gap IS
            // the curtain). Emission: caps 16 quads/cell + curtains 16
            // quads/cell + the legacy skirt ring re-aimed at cap outer verts.
            constexpr uint32_t UG_CELLS_PER_PATCH = PATCH_CELL_N * PATCH_CELL_N;   // 256
            constexpr uint32_t UG_QUADS_PER_CELL  = PATCH_MESH_N / PATCH_CELL_N;   // 4 (quads per cell edge)
            constexpr uint32_t UG_CAP_VERTS_PER_CELL  = (UG_QUADS_PER_CELL + 1) * (UG_QUADS_PER_CELL + 1); // 25
            constexpr uint32_t UG_CAP_STRIDE_C = UG_QUADS_PER_CELL + 1;            // 5 (cap verts per cell edge)
            constexpr uint32_t UG_BASE_VERTS_PER_CELL = 4 * UG_QUADS_PER_CELL;     // 16
            constexpr uint32_t UG_CAP_BASE  = (PATCH_MESH_N + 1) * (PATCH_MESH_N + 1) + 4 * PATCH_MESH_N; // 4481
            constexpr uint32_t UG_BASE_BASE = UG_CAP_BASE + UG_CELLS_PER_PATCH * UG_CAP_VERTS_PER_CELL;   // 10881
            constexpr uint32_t UG_DECODE_VERTS = UG_BASE_BASE + UG_CELLS_PER_PATCH * UG_BASE_VERTS_PER_CELL; // 14977
            static_assert(UG_QUADS_PER_CELL == 4 && UG_CAP_BASE == 4481
                       && UG_BASE_BASE == 10881 && UG_DECODE_VERTS == 14977,
              "unified ground: band arithmetic — cell borders must lie on "
              "mesh vertex lines (PATCH_MESH_N divisible by PATCH_CELL_N)");

            // Mathematical constants
            constexpr float    PI = 3.14159265359f;

            // Atmosphere
            constexpr float    FOG_COLOR_R = 0.88f;
            constexpr float    FOG_COLOR_G = 0.74f;
            constexpr float    FOG_COLOR_B = 0.58f;

            // Lighting
            // TWIN: world.wgsl `const SHADOW_MAP_SIZE: f32` (— Shadow
            // constants). FOUR consumers read the WGSL twin:
            //   1. the SPOT PCF's texel_size;
            //   2. SHADOW_TEXEL_WORLD — the unit of the sun frustum's snap
            //      and of the sun receiver normal offset;
            //   3. the SPOT normal offset's per-fragment texel size, which
            //      reads it DIRECTLY as the tile's X texel count
            //      (SHADOW_MAP_SIZE * 0.5), not through (2);
            //   4. TEXEL_UV — the SUN PCF's tap offsets, which are
            //      half-texel and so ride the uv rather than the integer
            //      `offset` parameter (PENUMBRA_2 N1).
            // Entry 4 has been removed and re-added once already: UMBRA_8
            // moved the sun taps to const integer offsets and struck it,
            // N1 moved them back. Change SHADOW_MAP_SIZE in BOTH rooms or
            // the spot sample grid, the snap lattice, BOTH normal offsets
            // and the sun tap spacing silently rescale while the texture
            // resizes.
            // (L3 MIRROR — the TILE_GRID_CAPACITY pattern; no compile-time
            // bridge spans the runtime-loaded seam.)
            //
            // ONE FACT, ONE HOME, deliberately: this sizes the spot atlas
            // as well as the sun map, because the sun map IS the spot
            // atlas's first texture during indoor moods (it is idle then).
            // Splitting the constant would give the two halves of one
            // atlas different tile widths. At 2048 the pair costs 33.6 MB
            // of VRAM — the spot half of that spend buys 1024x2048 indoor
            // tiles, which is not waste, but it is real.
            //
            // PORT_5a — 4096 -> 2048 (Jean's stamp). The pair was 134.2 MB
            // of a 681 MiB boot request that a shared-memory GPU could not
            // make resident; this returns 96 MiB. The texel doubles from
            // 0.20508 to 0.41016 wu (2*SUN_HALF_EXTENT/SHADOW_MAP_SIZE),
            // and everything expressed in TEXELS follows automatically —
            // SHADOW_TEXEL_WORLD, TEXEL_UV, both normal offsets, the snap
            // lattice, the spot PCF grid. What does NOT follow is
            // depthBiasClamp (renderer.hpp), a WORLD quantity tuned at the
            // old texel; see this unit's commit body.
            constexpr uint32_t SHADOW_MAP_SIZE = 2048;
            constexpr uint32_t MAX_POINT_LIGHTS = 8;

            // Indoor shell (ceiling + walls for finite indoor scenes)
            constexpr uint32_t SHELL_MAX_VERTICES = 2048;
            constexpr uint32_t SHELL_MAX_INDICES = 8192;

            // Painting system: staging + exhibition
            // The budget this must cover is proved by a static_assert beside
            // WALL_ART in bodies/gallery.hpp — the first site that can see all
            // three dials. Do not restate the sum here.
            constexpr uint32_t PAINTING_MAX_SLOTS = 288;      // max exhibited paintings
            // ONE RESOLUTION, for everything that can land in an exhibition
            // layer. A second, smaller one was tried and reverted: it made
            // promotion a PARTIAL write, and partial writes do not erase the
            // layer's previous occupant. See promote_to_exhibition.
            //
            // PORT_5b — 1024 -> 512 (Jean's stamp). This one number sizes
            // FIVE textures, which is the point of the ruling above and the
            // reason the diet is a single edit: Exhibition (160 -> 40 MiB),
            // Authored Staging (128 -> 32), Snapshot Staging (128 -> 32),
            // and the two offscreen snapshot targets (4 -> 1 each). It also
            // sizes the CPU scale-to-fit box in the authored loader
            // (bodies/gallery.hpp, `RES`), which reads THIS constant — so
            // the fit box and the destination cannot drift apart. Layer
            // counts are untouched; only the per-layer resolution moves.
            constexpr uint32_t PAINTING_RESOLUTION = 512;
            // Both raised by SUPPLY. The old 16 capped `to_load` at a sixteenth
            // of the paintings on disk and made content, not geometry, the
            // thing that ended a row — one wall would take the whole pool and
            // the other three stayed bare. The layer budget these must satisfy
            // is proved beside WALL_ART in bodies/gallery.hpp, with the slot
            // budget; do not restate it here.
            constexpr uint32_t STAGING_LAYERS = 32;            // per staging array (snapshot + authored)
            constexpr uint32_t EXHIBITION_LAYERS = 40;         // exhibition array
            constexpr uint32_t PAINTING_QUAD_N = 8;
            constexpr uint32_t PAINTING_QUAD_VERTS = PAINTING_QUAD_N * PAINTING_QUAD_N * 6;
            constexpr uint32_t PAINTING_FRAME_VERTS_PER = 78;
            constexpr uint32_t PAINTING_FRAME_VERTEX_COUNT = PAINTING_MAX_SLOTS * PAINTING_FRAME_VERTS_PER;

            // Generative catenary arches — GPU mesh gen (slot-based addressing)
            constexpr uint32_t MAX_ARCH_INSTANCES = 16;
            constexpr uint32_t AMG_MAX_VERTS_PER_SLOT = 2000;  // monumental worst case: 1470
            constexpr uint32_t AMG_MAX_INDICES_PER_SLOT = 7500;  // monumental worst case: 7488 (must be divisible by 3!)
            constexpr uint32_t AMG_TOTAL_VERTICES = MAX_ARCH_INSTANCES * AMG_MAX_VERTS_PER_SLOT;   // 32000
            constexpr uint32_t AMG_TOTAL_INDICES = MAX_ARCH_INSTANCES * AMG_MAX_INDICES_PER_SLOT; // 120000

            // Generative columns — GPU mesh gen (slot-based addressing)
            constexpr uint32_t MAX_COLUMN_INSTANCES = 32;
            constexpr uint32_t MAX_COLUMN_ONLY = 16;            // classical columns: slots 0-15
            constexpr uint32_t MAX_ANTENNA_ONLY = 16;            // antennas: slots 16-31
            constexpr uint32_t ANTENNA_SLOT_OFFSET = 16;         // GPU slot offset for antenna family
            static_assert(MAX_COLUMN_ONLY + MAX_ANTENNA_ONLY == MAX_COLUMN_INSTANCES);
            constexpr uint32_t CMG_MAX_VERTS_PER_SLOT = 1500;  // ornate worst case: 1249
            constexpr uint32_t CMG_MAX_INDICES_PER_SLOT = 6000;  // ornate worst case: 5880 (divisible by 3)
            constexpr uint32_t CMG_TOTAL_VERTICES = MAX_COLUMN_INSTANCES * CMG_MAX_VERTS_PER_SLOT;   // 48000
            constexpr uint32_t CMG_TOTAL_INDICES = MAX_COLUMN_INSTANCES * CMG_MAX_INDICES_PER_SLOT; // 192000

            // Generative pyramids — the instance array IS terrain
            // (contrib_pyramids_at); there is no mesh-gen scratch.
            constexpr uint32_t MAX_PYRAMID_INSTANCES = 8;

            // Generative palms — GPU mesh gen (slot-based addressing)
            constexpr uint32_t MAX_PALM_INSTANCES = 24;
            constexpr uint32_t PALMG_MAX_VERTS_PER_SLOT = 1200;
            constexpr uint32_t PALMG_MAX_INDICES_PER_SLOT = 6000;
            constexpr uint32_t PALMG_TOTAL_VERTICES = MAX_PALM_INSTANCES * PALMG_MAX_VERTS_PER_SLOT;   // 28800
            constexpr uint32_t PALMG_TOTAL_INDICES = MAX_PALM_INSTANCES * PALMG_MAX_INDICES_PER_SLOT; // 144000

            // Generative cacti — GPU mesh gen (slot-based addressing)
            constexpr uint32_t MAX_CACTUS_INSTANCES = 20;
            constexpr uint32_t CACTUSG_MAX_VERTS_PER_SLOT = 1500;
            constexpr uint32_t CACTUSG_MAX_INDICES_PER_SLOT = 7998;  // must be divisible by 3!
            constexpr uint32_t CACTUSG_TOTAL_VERTICES = MAX_CACTUS_INSTANCES * CACTUSG_MAX_VERTS_PER_SLOT;
            constexpr uint32_t CACTUSG_TOTAL_INDICES = MAX_CACTUS_INSTANCES * CACTUSG_MAX_INDICES_PER_SLOT;

            // Generative blade clusters — GPU mesh gen (slot-based addressing)
            constexpr uint32_t MAX_BLADE_INSTANCES = 32;
            constexpr uint32_t BLADEG_MAX_VERTS_PER_SLOT = 500;
            constexpr uint32_t BLADEG_MAX_INDICES_PER_SLOT = 1998;  // must be divisible by 3!
            constexpr uint32_t BLADEG_TOTAL_VERTICES = MAX_BLADE_INSTANCES * BLADEG_MAX_VERTS_PER_SLOT;
            constexpr uint32_t BLADEG_TOTAL_INDICES = MAX_BLADE_INSTANCES * BLADEG_MAX_INDICES_PER_SLOT;

            // Entity ground atlas (r32float, 256×1 — VS reads ground_y via textureLoad)
            constexpr uint32_t GROUND_ATLAS_WIDTH = 256;
            constexpr uint32_t GROUND_ATLAS_ARCH = 0;    // 16 slots
            constexpr uint32_t GROUND_ATLAS_COLUMN = 16;   // 32 slots
            // slots 48..55: DOCUMENTED HOLE — the retired pyramid range (readers
            // cut, then the write path — the ground-atlas residue). Do NOT re-pack; the
            // offsets below are hand-mirrored with world.wgsl's atlas table.
            constexpr uint32_t GROUND_ATLAS_PALM = 56;   // 24 slots
            constexpr uint32_t GROUND_ATLAS_CACTUS = 80;   // 20 slots
            constexpr uint32_t GROUND_ATLAS_BLADE = 100;  // 32 slots
            constexpr uint32_t GROUND_ATLAS_USED = 132;

            // GoL zone system — per-zone automaton grids
            constexpr uint32_t MAX_GOL_ZONES = 8;

            // Floating entity system — split into sphere (orbital) + cube (hover-bob)
            constexpr uint32_t MAX_SPHERE_INSTANCES = 8;
            // 256 slots: drone-show populations. update_cube runs ONE LANE
            // PER SLOT since PANORAMA_0 RIDE_0 (4 workgroups of 64), so the
            // cost is a constant depth rather than linear in the population.
            constexpr uint32_t MAX_CUBE_INSTANCES = 256;
            constexpr uint32_t CUBE_SLOT_OFFSET = MAX_SPHERE_INSTANCES;
            constexpr uint32_t TOTAL_FLOATING_SLOTS = MAX_SPHERE_INSTANCES + MAX_CUBE_INSTANCES;  // 264
            // CAPACITY — the MAXIMUM cells per zone side, and the side of the
            // life-buffer plane and the life texture. A zone's ACTUAL side is
            // its tier's grid_cells ∈ {8,16,24,32} (bodies/gol_zones.hpp), and
            // every index and bound is derived from THAT. Never mix the two.
            constexpr uint32_t GOL_ZONE_GRID = 32;
            constexpr uint32_t GOL_ZONE_CELLS = GOL_ZONE_GRID * GOL_ZONE_GRID;  // 1024
            constexpr uint32_t GOL_ZONE_LIFE_STRIDE = GOL_ZONE_CELLS * 5;  // 5 slots: visual, velocity, target, next, height_factor

            // Orb sky layer — luminous points on a dome above the world
            constexpr uint32_t MAX_ORBS = 256;

            // Agent system — unified entity layer. Slot 0 is the player's
            // body; slots 1..MAX_AGENTS-1 are mood-authored agents. See
            // bodies/agents.hpp.
            constexpr uint32_t MAX_AGENTS = 32;

            // FIELD_2: one force slot per subscriber — 32 agents + 8 spheres
            // + 256 cubes (capacity, not the 252 living). Index map lives at
            // the WGSL field consts: [0..31] agents · [32..39] spheres ·
            // [40..295] cubes.
            constexpr uint32_t FIELD_SUBSCRIBER_CAP = MAX_AGENTS + MAX_SPHERE_INSTANCES + MAX_CUBE_INSTANCES;  // 296
            // L3 LOCKSTEP WITNESS (FIELD_2b) — the one this fact never had.
            // world.wgsl declares `const FIELD_SUBSCRIBERS: u32 = 296u` and
            // sizes field_forces by THAT name (one home in that room). A
            // dimension cannot ride the config uniform — an array bound is
            // not a runtime value — so the two rooms are pinned by this
            // number instead. Change the population caps above and this
            // fires; edit both rooms in the same commit.
            static_assert(FIELD_SUBSCRIBER_CAP == 296,
                "FIELD_SUBSCRIBER_CAP and world.wgsl's FIELD_SUBSCRIBERS are "
                "one fact in two rooms (L3): update the WGSL const in the "
                "same commit, then move this number.");
        }

        namespace Idle {
            constexpr float AMPLITUDE_SCALE = 1.0f;
            constexpr float MAX_AMPLITUDE = 2.0f;
            constexpr float SIZE = 100.0f;
            constexpr float LIPSCHITZ_FACTOR = 1.0f;
            constexpr float BASE_R = 0.55f;
            constexpr float BASE_G = 0.45f;
            constexpr float BASE_B = 0.35f;
            constexpr float CHECKER_LIGHT = 1.15f;
            constexpr float CHECKER_DARK = 0.85f;
            constexpr float PAWN_POS_X = 0.0f;
            constexpr float PAWN_POS_Y = 0.0f;
            constexpr float PAWN_POS_Z = 0.0f;
            // ATRIUM_5 — THE BOOT GAZE FACES THE LONG DIAGONAL of a finite
            // room's asymmetric bounds. [-r*PE, (r+1)*PE] leaves the arrival
            // point 50 wu from the wall behind it and 100 ahead on each axis,
            // so the room a boot can compose into runs +X +Z. The pawn kernel
            // reads the heading as forward = (sin h, cos h) (world.wgsl,
            // coupling_velocity_to_pawn_heading writes h = atan2(vel.x, vel.z);
            // the aura reads it back), so PI/4 is the diagonal exactly.
            // Nothing else composes on the heading.
            constexpr float PAWN_HEADING = 0.78539816f;    // PI/4 — forward (0.7071, 0.7071)
            // AND THE CAMERA IS THE HEADING'S TWIN, PI APART. The orbit puts
            // the eye at look_at + d*(cos_el*sin_az, sin_el, cos_el*cos_az)
            // and looks back along it (compose_camera_position_from_orbit),
            // so the view direction in XZ is -(sin az, cos az) — the NEGATIVE
            // of the pawn's forward at the same angle. Both constants sat at
            // 0, which put the camera in FRONT of the pawn looking at its
            // face, with everything placed along the gaze behind the viewer.
            // az = h + PI is what makes the camera look where the pawn looks.
            constexpr float CAMERA_AZIMUTH = 3.92699082f;  // PI/4 + PI
            constexpr float CAMERA_ELEVATION = 0.4f;
            constexpr float CAMERA_DISTANCE = 15.0f;
            constexpr float PAWN_SPEED = 15.0f;
            constexpr float CUBE_PLASTICITY_DEFAULT = 1.0f;  // CONTACT_5 P2b: the live λ master;
                                                             // raised 0.6 -> 1.0 so a shove RELOCATES
                                                             // rather than partially returns -- "shove
                                                             // them freely all over the scene" is λ=1.
                                                             // Jean-tunable; 0 = elastic (pre-CONTACT_3),
                                                             // 1 = fully sculptable (the shove relocates).
                                                             // The per-tier CUBE_TIER_GAINS.plasticity
                                                             // column keeps the character spread.
        }

        // ── heading_to_bearing (ATRIUM_2) ────────────────────────
        // The pawn kernel's heading, spoken in the door grammar's
        // bearing. Two conventions meet here and neither moves:
        //   the kernel's forward is (sin h, cos h) in world XZ
        //     (world.wgsl, coupling_velocity_to_pawn_heading writes
        //      h = atan2(vel.x, vel.z); the aura's forward reads it back)
        //   a door at bearing b stands at (cos b, sin b) and its opening
        //     faces b + PI (force_spawn_door_fallback, the one grammar)
        // so bearing = atan2(cos h, sin h) = PI/2 - h, exactly. One home,
        // beside the pose it converts: the arc (force_spawn_atrium_arc)
        // and the hang (place_atrium_poster) both read it, and neither
        // re-derives the identity.
        constexpr float heading_to_bearing(float h) { return 1.57079633f - h; }

        // Mirrors world.wgsl's COUPLING_* bit-flag block. MUST match those
        // bit values 1:1 — semantic drift here would corrupt every GPU-side
        // coupling read silently.
        namespace Coupling {
            constexpr uint32_t TERRAIN_TO_PAWN_Y         = 1u << 1;
            constexpr uint32_t TERRAIN_TO_PAWN_TILT      = 1u << 2;
            constexpr uint32_t PAWN_TO_CAMERA_TARGET     = 1u << 3;
            constexpr uint32_t INPUT_MOVES_PAWN          = 1u << 4;
            constexpr uint32_t INPUT_ORBITS_CAMERA       = 1u << 5;
            constexpr uint32_t INPUT_ZOOMS_CAMERA        = 1u << 6;
            constexpr uint32_t TERRAIN_TO_SPHERE_HEIGHT  = 1u << 14;
            constexpr uint32_t PAWN_TO_SUN_VP            = 1u << 16;
            constexpr uint32_t ALL = 0x1FFFFFu;
            constexpr uint32_t NONE = 0u;
            constexpr uint32_t DERIVED = TERRAIN_TO_PAWN_Y | TERRAIN_TO_PAWN_TILT | PAWN_TO_CAMERA_TARGET | TERRAIN_TO_SPHERE_HEIGHT | PAWN_TO_SUN_VP;
            constexpr uint32_t ACCUMULATED = INPUT_MOVES_PAWN | INPUT_ORBITS_CAMERA | INPUT_ZOOMS_CAMERA;
            constexpr uint32_t TERRAIN = TERRAIN_TO_PAWN_Y | TERRAIN_TO_PAWN_TILT | TERRAIN_TO_SPHERE_HEIGHT;
            constexpr uint32_t INPUT = INPUT_MOVES_PAWN | INPUT_ORBITS_CAMERA | INPUT_ZOOMS_CAMERA;
            constexpr uint32_t CAMERA = PAWN_TO_CAMERA_TARGET | INPUT_ORBITS_CAMERA | INPUT_ZOOMS_CAMERA;
        }

        // =====================================================================
        // S4 GPU STRUCTURES — Byte-aligned data contracts (must match WGSL)
        // =====================================================================

        struct alignas(16) GPUFrameSignal {
            float t_seconds;
            float t_beats;
            float dt;
            float aspect_ratio;
            float move_x;
            float move_z;
            float look_az_delta;
            float look_el_delta;
            float zoom_delta;
            float pan_x_delta;
            float pan_y_delta;
            float dt_beats;       // beats elapsed since last frame; mirrors
                                  // world.wgsl GPUFrameSignal.dt_beats,
                                  // consumed by step_trigger.
            // THE MOUNT BLOCK (RIBBON_1) — a TRAJECTORY, not a pose. The pose
            // is the GPU's (ribbon_body_read.saddle, written by the body kernel
            // the same pass); the CPU authors only the EDGE — where the body was
            // when the host changed, and how far along the ease it is. The whole
            // signal drains in one write again: the split drain went with the
            // sky block it existed for.
            float    mount_phase;         // 48  0→1 over the trajectory; 1 = arrived
            uint32_t mount_kind;          // 52  0 none, 1 boarding (→ saddle), 2 landing (→ the walked pose)
            float    mount_from_heading;  // 56
            uint32_t _mp0;                // 60
            float    mount_from[3];       // 64  the pose the body left, CPU-captured at the host edge
            float    _mp1;                // 76
        };

        // ═══ THE SUBTRACTION MASKS' VOCABULARY (PANORAMA_1) ═════════════
        // The bit names live beside the fields they name, because the bit IS
        // the field's meaning. Read at the draw sites (render_passes.hpp) and
        // pinned open at the boot block below; nothing else may author them.
        namespace DrawBit {
            inline constexpr uint32_t TERRAIN_A = 1u << 0;   // full IB, zone-overlapped
            inline constexpr uint32_t TERRAIN_B = 1u << 1;   // cap-only IB, clean LOD0
            inline constexpr uint32_t TERRAIN_C = 1u << 2;   // LOD1 IB
            inline constexpr uint32_t TABLE     = 1u << 3;   // the drawable table
            inline constexpr uint32_t RIBBON    = 1u << 4;   // a table MEMBER since ECONOMY_1 —
                                                             // subtracted through DrawBind, not
                                                             // by skipping the table
            inline constexpr uint32_t PAINTINGS = 1u << 5;   // both draws (wall + gallery frames)
            inline constexpr uint32_t ORBS      = 1u << 6;
            inline constexpr uint32_t FADE      = 1u << 7;
        }
        namespace ShadowBit {
            inline constexpr uint32_t TERRAIN = 1u << 0;
            inline constexpr uint32_t TABLE   = 1u << 1;   // entities + the two artwork draws
        }
        inline constexpr uint32_t DRAW_MASK_ALL   = 0xFFu;
        inline constexpr uint32_t SHADOW_MASK_ALL = 0x3u;

        // Field ORDER is the cross-room contract — world.wgsl's
        // DesignConfig mirrors this struct field-for-field. Adding a
        // knob: THE GROWTH LAW, L5 in docs/LAWS.md. Choosing where
        // to put it: THE ALIGNMENT LAW, L4.
        struct alignas(16) GPUDesignConfig {
            // ─── Debug mutes ────────────────────────────────────────
            uint32_t mute_dynamics_0d;
            uint32_t mute_signal;
            uint32_t mute_couplings;

            // ─── Interaction ────────────────────────────────────────
            float pawn_speed;
            uint32_t freeze_sphere;
            uint32_t fpv_mode;
            // (pawn_tilt_tau belongs to THIS group semantically — it sits in
            //  the struct's trailing 4-byte pad instead. See the note at the
            //  tail of the struct for why appending here is not available.)

            // ─── Terrain wave control ───────────────────────────────
            uint32_t world_seed;              // master seed for GPU-side terrain/zone generation

            // ─── Lighting & atmosphere ──────────────────────────────
            uint32_t _pad_sun;                // WGSL aligns vec3 to 16, C++ packs
                                              // float[3] at 4 — see the GROWTH LAW
            float sun_direction[3];
            float aura_enabled;               // 0.0 = off, 1.0 = on (guards all aura sampling)
            float pawn_aura_height;           // 0.0 = no aura extrusion, >0 = world units of rise
            float fog_density;                // exponential fog coefficient (default 0.003)
            uint32_t _pad_fog[2];             // ditto
            float fog_color[3];               // fog/sky color RGB

            // ─── Transition overlay ─────────────────────────────────
            float fade_alpha;                 // 0.0 = no overlay, 1.0 = fully opaque
            float fade_color[3];              // transition overlay RGB

            // ─── World structure ────────────────────────────────────
            uint32_t _pad_pier_retired;       // pier_count's slot (BATCH G erasure). WGSL's
                                              //   vec2 align re-pads here implicitly; this pad is
                                              //   the C++ mirror of that. Offsets after it are
                                              //   UNMOVED — the 560/352 pins below are the proof.
            float world_bound_min[2];         // XZ min clamp (0,0 = infinite)
            float world_bound_max[2];         // XZ max clamp (0,0 = infinite)
            uint32_t placement_patch_count;   // active patches for entity Y-correction (decoupled from photographer)
            float terrain_amp_ceiling;        // max per-wave amplitude (0 = unlimited, >0 = clamp for indoor)
            float ceiling_height;             // indoor ceiling Y (0 = no ceiling, >0 = camera Y clamp)
            float terrain_time;               // t_beats for terrain evaluation (0 = frozen, >0 = animated)

            // ─── Polyphony-driven band motion ────────────────────────────
            float band_blend_0;               // [0] continental
            float band_blend_1;               // [1] regional
            float band_blend_2;               // [2] local
            float band_blend_3;               // [3] detail
            float band_blend_4;               // [4] fine
            float band_blend_5;               // [5] tectonic
            float band_phase_origin_0;        // [0] continental
            float band_phase_origin_1;        // [1] regional
            float band_phase_origin_2;        // [2] local
            float band_phase_origin_3;        // [3] detail
            float band_phase_origin_4;        // [4] fine
            float band_phase_origin_5;        // [5] tectonic

            // ─── Musical animation modes ─────────────────────────────────
            // Each mode is an independently toggleable coupling circuit.
            // Values are [0,1] intensity, driven by polyphony through trajectory ramp.
            float mode_color_shift;           // SIGNED axis on the mode field; rest 0 is the CENTER (− retreats, + advances); range graduates at Movement 1 close
            float mode_checker_scatter;       // SIGNED axis on sparse survival; rest 0 the center (− extinguishes, + populates); range graduates at Movement 1 close
            float mode_palette_target;        // [0,3] target palette index (0=sand 1=salmon 2=green 3=warm)
            float mode_palette_intensity;     // [0,1] how strongly to override spatial palette weights
            float mode_discrete_tier;         // [0,4] target discrete tier (0=color 1=tinted 2=BW 3=chessBW 4=chessColor)
            float mode_gol_tick_scale;        // tick period multiplier (1.0=normal, <1=faster, >1=slower)
            float mode_gol_height_scale;      // alive_height multiplier (1.0=normal, >1=taller)
            // ─── Floater system dials ────────────────────────────────────
            float floater_coordination;

            // ─── Radial pulse ring buffer ────────────────────────────────
            uint32_t pulse_count;             // active entries (0–8)
            // ─── Agent system ────────────────────────────────────────────
            // Slot index of the player's current body in agent_state[].
            // Piggybacks on the existing _pulse_pad triple (size witnessed by the sizeof static_assert below — 560).
            uint32_t possessed_slot;          // slot 0 at session start
            // THE RIM taste knob (config-gated): 0 = icing tints to fog
            // (default); >0.5 = icing DITHER-dissolves (geometry condenses
            // instead of tinting) at the two icing FS sites. Repurposes one
            // of the pulse pad floats — no struct-size delta.
            float veil_dither;
            // Indoor GoL height cap (0 = disabled): zone_derive_params bounds
            // each new zone's alive_height so the MAXIMUM realised cell lift
            // equals this exactly (it divides by the height_factor clamp
            // bound to get there). Staged by
            // apply_mood_lighting — INDOOR_HEIGHT_CAP_FRACTION ×
            // ceiling_height indoors, 0 elsewhere. Repurposes the last
            // pulse pad float — no struct-size delta (the sizeof witness
            // 560 stands).
            float indoor_height_cap;
            float pulse_data[32];             // 8 × {origin_x, origin_z, onset_seconds, amplitude}
            // ─── LOD-band point position ────────────────────────────────
            // (renamed lod_pawn → lod_point: the value has been THE POINT
            // — the name was a fossil.) The CPU bands patches
            // into LOD0/LOD1 in stream_patches from point_.x/z
            // (the point, 1 frame stale — law E-4). The GPU's frustum-cull
            // shader applies the same lod0_radius gate; if it read the
            // LIVE point instead, CPU banding and GPU gate would disagree
            // at the boundary annulus and patches would flicker/z-fight.
            // So the CPU pushes its banding point here and the cull shader
            // reads it — both sides partition with one yardstick by
            // construction.
            float lod_point_x;
            float lod_point_z;
            // ─── The point ───────────────────────────────────────────
            // Host flag + fly speed — piggybacked on the lod-point pad
            // pair (the possessed_slot precedent). Mirror order matches
            // world.wgsl's Config.
            uint32_t point_host;              // 0 = pawn (the kite), 1 = camera (free-fly), 2 = ribbon
            float point_fly_speed;            // W/S/A/D velocity; 0 → WGSL PAWN_SPEED fallback

            // ─── THE VEIL (re-ruled: RING = draw authority, fog = icing) ──
            // veil_ring: the SOLE draw authority (325 default) — terrain
            //   banding, entity cull, flora/agent/floater VS gates all read
            //   it; nothing draws beyond it. Live-tunable.
            // veil_icing: δ — the narrow fade band [ring−δ, ring] in
            //   shade_lit (cosmetic; joins materialize inside it).
            // veil_strength: 1 outdoors, 0 in finite/indoor (walls define
            //   the boundary there, not fog — staged per frame by U5).
            // lod0_radius: the terrain full-mesh/half-mesh split (175) —
            //   read by the CPU band AND the GPU LOD0 gate (one yardstick).
            float veil_ring;
            float veil_icing;
            float veil_strength;
            float lod0_radius;

            // ─── The palette mirror (FORK-tier graduation) ───────────────
            // The four terrain palettes, graduated from WGSL consts so a
            // color voice can move the MEDIANS ("a spectrum moves the
            // median" writes palette_center). Layout
            // matches WGSL: array<vec4,4> stride 16 (rgb in xyz, w pad);
            // weight = one vec4, component i = palette i (sums 1.0).
            // REST = the pre-graduation literals (boot init) — bit-identical.
            float palette_center[4][4];   // [palette] = {r,g,b,pad}
            float palette_light[4][4];    // [palette] = {r,g,b,pad}
            float palette_weight[4];      // selection probabilities

            // ─── CHECKER-REBUILD: the pitch-class color field ───────
            // Re-semanticized AND resized (Jean OK'd the witness move).
            // The checker voice's window pc-length vector becomes a
            // weighted-average COLOR (resultant), enveloped 2-beat attack
            // / 8-beat release. The GPU pulls each discrete cell toward
            // the resultant, wanders each region around it (re-rolled per
            // 4-beat window), and widens each region's own spread by the
            // distinct-pc count. REST = amount 0 (the GPU maps that to the
            // cell's seed color) + variance 0 — identity by construction;
            // the bake passes amount 0 (seam-proof). Mirrors world.wgsl
            // DesignConfig: vec3 + f32 + f32 + f32 (two 16-byte slots, 8 B tail
            // pad — CONTACT_2 added point_bubble_radius). Driver: the visual canvas, flushed at U4.
            float checker_resultant[3];    // the music color (weighted pc-average), enveloped
            float checker_music_amount;    // enveloped presence [0,1] — S1 pull + S2 wander scale
            float checker_music_variance;  // enveloped distinct-pc count — S3 within-patch spread
            float point_bubble_radius;     // CONTACT_2 C3a: the point's bounded awareness (rest 80; boot-pinned from contracts/point.hpp POINT_BUBBLE_RADIUS). Fills the checker tail pad — sizeof unchanged.
            float cube_plasticity;         // CONTACT_3 K2c: global λ master (rest 1.0 — raised from 0.6 at CONTACT_5 P2b; boot-pinned from Idle::CUBE_PLASTICITY_DEFAULT). Also fills the checker tail pad — sizeof unchanged.
            // CLOSURE_PAWN [6] — possessed body's terrain-tilt lag, seconds.
            // 0 = instant (the pawn's response, byte-identical to the pre-[6]
            // hard assignment). The CPU picks it from the possessed figure's
            // PAWN_FIGURES row each frame; the setter's guard keeps that free.
            //
            // WHY IT LIVES HERE AND NOT IN ─── Interaction ───, where it
            // belongs: growth law (1) says re-use a pad, and the LAST 4 bytes
            // are that pad (the sizeof witness below names them). Appending
            // inside Interaction is NOT available — it would push
            // sun_direction from 64 to 68, and the WGSL mirror declares that
            // field vec3<f32>, whose uniform-layout alignment is 16. WGSL
            // would round 68 up to 80 while C++ sits at 68, silently
            // diverging every field from there to the end of the struct.
            // sizeof stays 592 in both rooms, so neither the sizeof witness
            // nor any compiler would have caught it. The tail pad costs
            // nothing and shifts nothing — the same move point_bubble_radius
            // (CONTACT_2) and cube_plasticity (CONTACT_3) made above.
            float pawn_tilt_tau;
            // ─── THE MOSAIC (MOSAIC_0/1/2) — trencadís dials ─────────
            // Mirror of world.wgsl DesignConfig tail — GROWTH LAW (same
            // commit, same order). Rests: Dim::MOSAIC_* via the boot pins.
            // MOSAIC_2 retired radius/icing (grain is 1 − veil_t; the
            // veil owns the band once) and added blend: six dials + two
            // pads became FIVE + THREE. Still 8 floats — sizeof 592 is
            // unmoved and the witness below must not change. The pads
            // are structure, not reservation: the WGSL mirror has no
            // invisible padding, so a hole must be declared here.
            float mosaic_enable;
            float mosaic_shard_size;
            float mosaic_passage_scale;
            float mosaic_blend;
            float mosaic_facet;
            // TUNE_1 A3 — the possessed figure's eye height, in world units.
            // CPU-derived (FPV_EYE_RATIO x that figure's own height) because
            // the compute stage cannot see scene_constants.figure_profiles:
            // g2:200 is a render-VS-only uniform block and update_camera_vp's
            // layout does not carry it. First of the three tail pads, reused in place — same
            // position, same type, sizeof 592 UNMOVED (the possessed_slot /
            // veil_dither / indoor_height_cap precedent). Was _pad592_0.
            float fpv_eye_height;
            // ─── THE FIELD'S DIALS (FIELD_2b) — the panel's first
            // graduation. Mirror of world.wgsl DesignConfig tail —
            // GROWTH LAW (same commit, same order, same types).
            // Rests: contracts/control_panel.hpp, boot-pinned below —
            // the panel authors, this struct transports, both rooms
            // read. The WGSL consts these replace are GONE, so the
            // LOCKSTEP hazard they carried is gone with them.
            // Tail-append per L4: the two _pad592 floats are consumed
            // here and six more follow, so sizeof moves 592 -> 624
            // (the witness below is the handshake). Appending anywhere
            // above would shift a vec3 off its 16-byte boundary and
            // diverge the rooms silently — the pawn_tilt_tau note
            // states the law.
            float field_slack;             // shell factor over summed radii (rest 3.0)
            float field_k;                 // accel per unit of quadratic shell depth (rest 300.0)
            float field_fmax;              // magnitude clamp on the summed force (rest 600.0)
            float field_occupier_gain;     // mute: standing geometry (rest 1.0)
            float field_authored_gain;     // mute: the authored table (rest 1.0)
            float field_gain_cube;         // subscriber-class gain, applied after the clamp (rest 4.0)
            float field_gain_sphere;       // (rest 1.0)
            float field_gain_agent;        // (rest 4.0)
            // HEM_0 — the possessed figure's own radius, world units. CPU-derived
            // from PAWN_FIGURES[skin] each frame on the wire pawn_tilt_tau /
            // fpv_eye_height already ride. It is the BOUNDARY INSET, not a
            // collision radius: nothing in the contact chain reads it. Reuses the
            // first FIELD_2b tail pad in place — same position, same type, sizeof
            // 624 UNMOVED (the fpv_eye_height / veil_dither precedent).
            // Was _pad624_0.
            float pawn_body_radius;
            // ─── THE RIBBON'S DIALS (RIBBON_1) — GROWTH LAW, same commit, same
            // order, same types as world.wgsl DesignConfig. THE PANEL
            // (contracts/ribbon_surface.hpp RIBBON_LIVE) authors the rests; the
            // boot pins them here; the organ edits them. The ribbon's kernels
            // read these and nothing else — the CPU head that used to read
            // RIBBON_LIVE directly is gone, so the panel reaches the flight
            // through this transport and the rooms cannot drift.
            // _pad624_1 consumed, eleven appended, one fresh pad to the
            // boundary: sizeof 624 -> 672.
            float ribbon_max_speed;        // 620
            float ribbon_yaw_rate;         // 624
            float ribbon_r_min;            // 628
            float ribbon_floor_margin;     // 632
            float ribbon_alt_smooth_dist;  // 636
            float ribbon_alt_stiff;        // 640
            float ribbon_climb_rate;       // 644
            float ribbon_mount_setback;    // 648
            float ribbon_lookahead;        // 652
            float ribbon_clear_head;       // 656
            float ribbon_clear_body;       // 660
            // THE HANDS' TAU. It was RIBBON_LIVE.sky_yaw_tau, read by the CPU
            // head's eased yaw; RIBBON_1 moved the easing into the head kernel
            // and would have left the dial writing nothing. It rides here
            // instead, so the panel's word still reaches the hand it names —
            // and now eases the throttle beside the yaw.
            float ribbon_hands_tau;        // 664
            // ─── THE WANDER BRAIN'S DIALS (RIBBON_2) — GROWTH LAW, same
            // commit, same order, same types as the WGSL twin. The brain
            // lives in the head kernel now, so its dials travel the same road
            // the flight's do: _pad672_0 consumed, three appended, one fresh
            // pad to the boundary; sizeof 672 -> 688.
            float ribbon_wander_soft;      // 668
            float ribbon_wander_yaw_max;   // 672
            float ribbon_wander_arrive;    // 676
            float ribbon_roam_radius;      // 680
            // KITE_1 — THE CHASE'S FEED-FORWARD. Mirror of the WGSL twin —
            // GROWTH LAW, same commit, same position, same type. Rest
            // authored at contracts/point.hpp (CAMERA_CHASE_FF); the boot
            // pins it. The tail pad is consumed IN PLACE, so sizeof 688 is
            // unmoved and no witness below moves either.
            float camera_chase_ff;         // 684
            // KITE_1 — THE WITNESS'S PRESENCE. Mirror of the WGSL twin,
            // GROWTH LAW, same commit, same order, same types. Rests
            // authored at contracts/point.hpp; the boot pins them. The tail
            // pad went to camera_chase_ff, so these two append and two fresh
            // pads carry the struct back to its boundary: 688 -> 704.
            float camera_push_gain;        // 688
            float camera_push_radius;      // 692
            // ATRIUM_7 — THE DOORWAY'S OWN SHELL. Mirror of the WGSL twin —
            // GROWTH LAW, same commit, same position, same type. Rest
            // authored at contracts/control_panel.hpp (FIELD_ARCH_SLACK);
            // the boot pins it. A tail pad is consumed IN PLACE, so sizeof
            // 704 is unmoved and no witness below moves either. Was
            // _pad704_0.
            float field_arch_slack;        // 696
            // ─── The subtraction dials (PANORAMA_1) ──────────────────────
            // THE INSTRUMENT THE METER LACKS. `[METER]` brackets a PASS; it
            // cannot say which draw inside the pass spent the milliseconds,
            // and the main pass is 11-12 ms on Kepler for a vertex and pixel
            // count that does not explain it. These two words let a draw be
            // SUBTRACTED at the encoder — not culled in the shader, so the
            // pass row measures its absence — and the difference is the
            // draw's cost, read off the same meter that could not split it.
            //
            // Default is every bit set (the boot pins it). A cleared bit is a
            // measurement, never a shipped state.
            //
            // draw_mask, main pass:
            //   bit0 terrain plan A (full IB, zone-overlapped)
            //   bit1 terrain plan B (cap-only IB, clean LOD0)
            //   bit2 terrain plan C (LOD1 IB)
            //   bit3 the drawable table   bit4 (rides bit3 — the ribbon is a
            //                                   table member since ECONOMY_1)
            //   bit5 the two painting draws   bit6 the orbs   bit7 the fade
            // Mirror of the WGSL twin — GROWTH LAW, same commit, same
            // position, same type. Was _pad704_1, consumed IN PLACE.
            uint32_t draw_mask;            // 700
            // shadow_mask: bit0 terrain, bit1 the entity table + the artworks.
            // Appended past the boundary, so three fresh pads carry the struct
            // back to it: 704 -> 720.
            uint32_t shadow_mask;          // 704
            // THE TAP COUNT (PANORAMA_1). 16 (today's 4x4) or 4 (the inner
            // 2x2). PANORAMA_0 priced the taps at 1-2 ms of main pass and
            // could not read them; this is the A/B that does. Mirror of the
            // WGSL twin — GROWTH LAW, same commit, same position, same type.
            // A pad is consumed IN PLACE, so sizeof 720 is unmoved.
            // Was _pad720_0.
            uint32_t shadow_pcf_taps;      // 708
            // STATURE_0 — the possessed figure's own height, world units.
            // THE FACT fpv_eye_height IS A RATIO OF, shipped in its own right:
            // the photographer frames the same subject at a different fraction,
            // and two rooms scaling one transported number cannot drift the way
            // two literals did. CPU-derived from PAWN_FIGURES on the wire
            // pawn_tilt_tau / pawn_body_radius / fpv_eye_height already ride.
            // Reuses the first tail pad in place — same position, same type,
            // sizeof 720 UNMOVED (the fpv_eye_height / pawn_body_radius
            // precedent). Was _pad720_1.
            float possessed_height;        // 712
            float _pad720_2;               // 716
        };

        struct alignas(16) GPUTileGridEntry {
            float amp_scale;
            float height_bias;
            float activation_scale;
            uint32_t archetype;        // 0=mountainous, 1=varied, 2=basin, 3=pool
        };
        static_assert(sizeof(GPUTileGridEntry) == 16, "GPUTileGridEntry must be 16 bytes");

        static constexpr uint32_t TILE_GRID_SIDE = 2 * (Dim::PATCH_PREGEN_RADIUS + 1) + 1;  // 17 (pregen + 1 pad each side)
        static constexpr uint32_t TILE_GRID_MAX = TILE_GRID_SIDE * TILE_GRID_SIDE;  // 289
        static_assert(TILE_GRID_MAX <= Dim::TILE_GRID_CAPACITY,
            "TILE_GRID ceiling exceeded: raise TILE_GRID_CAPACITY in BOTH "
            "rooms (state.hpp Dim + world.wgsl) — the pair is pinned, not "
            "derived");
        struct alignas(16) GPUTileGrid {
            int32_t origin_x;          // grid-space X of entry [0][0]
            int32_t origin_z;          // grid-space Z of entry [0][0]
            uint32_t side;             // grid dimension (runtime; ≤ TILE_GRID_SIDE)
            float cell_extent;         // world units per cell (50.0)
            // Capacity-sized (the pinned ceiling); the live extent is
            // side² — slots beyond are uploaded as zeros, never read.
            GPUTileGridEntry entries[Dim::TILE_GRID_CAPACITY];
        };
        static_assert(sizeof(GPUTileGrid) == 16 + Dim::TILE_GRID_CAPACITY * 16, "GPUTileGrid must match WGSL layout");

        struct alignas(16) GPUAgentState {
            float pos_x;           //  0
            float pos_y;           //  4
            float pos_z;           //  8
            float t;               // 12 — personal clock
            float vel_x;           // 16
            float vel_y;           // 20
            float vel_z;           // 24
            float heading;         // 28
            float home_x;          // 32
            uint32_t route;        // 36 — ATRIUM_4. Was home_y: the tether is planar
                                   //      and nothing ever read it (R4). PASSER's route
                                   //      state, packed (leg<<12)|(cur<<4)|(phase<<1)|1;
                                   //      0 = fresh. Zero on every other behaviour.
            float home_z;          // 40
            uint32_t seed;         // 44 — stable noise source
            uint32_t behavior_id;  // 48 — AgentBehaviorId (see bodies/agents.hpp)
            uint32_t tier_idx;     // 52 — AgentTierId     (see bodies/agents.hpp)
            uint32_t is_active;    // 56 — 0 = inactive (collapsed in VS + skipped in update)
            int32_t  portal_trigger; // 60 — only meaningful on the possessed slot (-1 = none)
            float orient_x;        // 64 — heading ⊗ tilt quaternion
            float orient_y;        // 68
            float orient_z;        // 72
            float orient_w;        // 76
            float color_r;         // 80 — per-agent body color (palette pick at spawn)
            float color_g;         // 84
            float color_b;         // 88
            uint32_t skin_id;      // 92 — PawnFigureDef row (0 = regular pawn). Was _pad0.
        };                         // 96 total

        // ─── Agent registry GPU structs ──────────────────────────────
        //
        // The C++ side is uploaded as a uniform buffer (bindings 110/111);
        // the WGSL side reads from those bindings. Field count, field
        // order, and total size MUST match exactly. A field-order
        // mismatch produces silent data corruption (no compile error,
        // agents read parameters from the wrong column). The static_asserts
        // below catch size drift; field order is on the human.

        // GPU-side mirror of AgentBehaviorDef (bodies/agents.hpp) without
        // the `id` and `name` fields. Uploaded once at world-init from
        // the C++ AGENT_BEHAVIORS table (single source of truth) and read
        // by the agent compute kernels via storage binding 110.
        struct alignas(16) GPUAgentBehaviorDef {
            float step_rate;       //  0 — steps per beat
            float step_size;       //  4 — world units per step (or waypoint radius)
            float persistence;     //  8 — [0,1] directional commitment
            float drag;            // 12 — 1/s velocity decay coefficient
            float home_pull;       // 16 — 1/s² tether spring coefficient
            float neighbor_radius; // 20 — world units, flock/cohesion sample radius
            float speed_cap;       // 24 — world units/s
            float aux;             // 28 — ATRIUM_4: behaviour-specific scalar (PASSER = the band, wu; 0 elsewhere). Was _pad.
        };                         // 32 total (16-byte aligned)

        // Counts mirror the AGENT_BEHAVIOR_COUNT / AGENT_TIER_COUNT
        // enums in bodies/agents.hpp. Kept here so state.hpp can size
        // its registry buffers and bind-group entries without depending
        // on bodies/agents.hpp (which is included after state.hpp, at
        // file scope in the cartridge cohort). Asserts in
        // bodies/agents.hpp verify these stay in sync.
        static constexpr uint32_t GPU_AGENT_BEHAVIOR_COUNT = 11;
        static constexpr uint32_t GPU_AGENT_TIER_COUNT = 4;

        //
        // Field layout matches WGSL `struct AgentTierParams` exactly so
        // the WGSL side can read this buffer with the same struct shape
        // it had when AGENT_TIER_GAINS_WGSL was a const array literal.
        struct alignas(16) GPUAgentTierDef {
            float step_gain;       //  0 — multiplies behavior.step_size impulse
            float persist_gain;    //  4 — multiplies behavior.persistence (and home_pull)
            float speed_gain;      //  8 — multiplies behavior.speed_cap
            float color_r;         // 12 — vertex shader entity color (world.wgsl §6.3)
            float color_g;         // 16
            float color_b;         // 20
            float contact_radius;  // 24 — TRUEBAND_CONTACT_1: body radius (wu)
            float contact_mass;    // 28 — relative yield authority
            float personal_radius; // 32 — CONTACT_2: social shell (flock sense + flee trigger)
            float flee_gain_player;// 36 — CONTACT_2: flee response gain vs the point-source
            float _pad0;           // 40 — pad to 48 (uniform array stride, 16-aligned)
            float _pad1;           // 44
        };                         // 48 total (16-byte aligned)

        // ── Pawn figure table (GPU) ──────────────────────────────────────────
        // Flat pack of PawnFigureDef (bodies/pawn_figures.hpp, H1) so ONE uniform
        // buffer serves both profile idioms.
        //
        // UNIFORM ADDRESS SPACE MIRROR (H3): WGSL declares prof/pal as
        // array<vec4<f32>, 8> — the uniform address space forces a 16-byte array
        // stride, so array<f32,32> would pad each float to 16 B (512 B) and break
        // this layout. float[32] here and array<vec4<f32>,8> there are the SAME
        // 128 contiguous bytes, so this C++ struct and the pack below need no
        // change — only the WGSL declaration and its indexing differ.
        //   struct total: 16 (header) + 128 (prof) + 128 (pal) + 16 (drift) = 288,
        //   and 288 % 16 == 0 satisfies the uniform array-stride rule for
        //   array<PawnFigure, 14>.
        //
        // Interpreted per-family in world.wgsl (H3):
        //   prof[] : FAM_SMOOTH  → 16 SmoothProfile fields, order-identical to H1.
        //            FAM_HERALDIC → seg k at prof[k*4 + {0:h, 1:r_bot, 2:r_top, 3:shape}].
        //            FAM_REGULAR  → unused (WGSL uses the hardcoded pawn_profile_radius).
        //   pal[]  : up to 8 stops × {t,r,g,b}; first stop with t < 0 ends the list.
        //            pal[0] < 0 means "no palette" (regular → legacy color path).
        //   drift  : {hue_deg, sat, val, _pad}.
        struct alignas(16) GPUPawnFigure {
            uint32_t family;     //   0
            uint32_t seg_count;  //   4  (heraldic only; 0 otherwise)
            float    height;     //   8
            float    radius;     //  12
            float    prof[32];   //  16
            float    pal[32];    // 144
            float    drift[4];   // 272
        };                       // 288

        // Typed registry → GPU array. Called once at world-init by
        // GPUState::upload_pawn_figures.
        inline void pack_pawn_figures(GPUPawnFigure out[PAWN_FIGURE_COUNT]) {
            for (uint32_t i = 0; i < PAWN_FIGURE_COUNT; ++i) {
                const PawnFigureDef& f = PAWN_FIGURES[i];
                GPUPawnFigure& g = out[i];
                g = GPUPawnFigure{};
                g.family = static_cast<uint32_t>(f.family);
                g.height = f.height;
                g.radius = f.radius;

                if (f.smooth) {   // FAM_SMOOTH — 16 fields, order matches H1 SmoothProfile
                    const SmoothProfile& s = *f.smooth;
                    const float src[16] = {
                        s.start_r, s.flare_r, s.flare_t, s.peak_r, s.peak_t,
                        s.base_t, s.body_start_r, s.body_t, s.waist_r,
                        s.neck_t, s.neck_r, s.collar_t, s.collar_bulge,
                        s.head_t, s.head_base_r, s.head_sphere_r,
                    };
                    for (int k = 0; k < 16; ++k) g.prof[k] = src[k];
                } else if (f.heraldic) {   // FAM_HERALDIC — packed segments
                    const HeraldicProfile& h = *f.heraldic;
                    g.seg_count = h.seg_count;
                    for (uint32_t k = 0; k < h.seg_count && k < 7u; ++k) {
                        g.prof[k*4 + 0] = h.seg[k].height;
                        g.prof[k*4 + 1] = h.seg[k].r_bot;
                        g.prof[k*4 + 2] = h.seg[k].r_top;
                        g.prof[k*4 + 3] = static_cast<float>(h.seg[k].shape);
                    }
                }
                // else FAM_REGULAR: prof left zero — WGSL uses the hardcoded path.

                // palette (≤8 stops, t<0 sentinel). Regular figures have palette==null.
                if (f.palette) {
                    int k = 0;
                    for (; k < 8 && f.palette[k].t >= 0.0f; ++k) {
                        g.pal[k*4 + 0] = f.palette[k].t;
                        g.pal[k*4 + 1] = f.palette[k].r;
                        g.pal[k*4 + 2] = f.palette[k].g;
                        g.pal[k*4 + 3] = f.palette[k].b;
                    }
                    if (k < 8) g.pal[k*4 + 0] = -1.0f;   // terminator
                } else {
                    g.pal[0] = -1.0f;                    // no palette → legacy color
                }

                g.drift[0] = f.drift.hue_deg;
                g.drift[1] = f.drift.sat;
                g.drift[2] = f.drift.val;
                g.drift[3] = 0.0f;
            }
        }

        // ── FIELD_4: the authored table ── CPU-sovereign source terms
        // for the field (the GPU buffer is the derived copy). L4-safe
        // by packing: no bare vec3 — each emitter i is two vec4 rows:
        //   rows[2i]   = { x, y, z, S }        (center, strength)
        //   rows[2i+1] = { r0, R, enable, _ }  (ring, reach, gate)
        struct GPUFieldAuthored {
            uint32_t count;
            uint32_t _p0, _p1, _p2;    // 16 B header
            float rows[8][4];          // 4 emitters × 2 vec4 = 128 B
        };
        static_assert(sizeof(GPUFieldAuthored) == 144,
            "GPUFieldAuthored: the two-rooms handshake (16 B header + 8 vec4)");

        struct alignas(16) GPUCameraState {
            float pos[3];
            float azimuth;
            float elevation;
            float distance;
            float pan_x;
            float pan_y;
            float aim_point[3];   // damped target (lerps toward possessed agent's pos)
            float _pad;           // pad to 16-byte boundary
        };
        // ═══ THE CAMERA WITNESS (ATRIUM_11) ══════════════════════════
        //
        // THE ORBIT HAS NO CPU HOME. compose_camera_position_from_orbit is
        // WGSL, update_camera_vp ACCUMULATES the look deltas into camera_state,
        // and the CHORD_3 block copy carries the result GPU-to-GPU — the CPU
        // is not in that loop and must not be. So the pose Jean makes with
        // his own mouse is unreadable from here without a readback, and this
        // is the print at the end of one (cartridge.hpp, the camera arm of
        // phase_witness_harvest, beside the pawn's and the floaters').
        //
        // IT SPEAKS THE ARRIVAL ROW'S THREE NAMES, so a pose made by hand
        // can be read off and authored: distance, elevation in degrees, and
        // the azimuth as an OFFSET on the gaze. The base is the ARRIVAL
        // gaze and nothing else — pass Idle::PAWN_HEADING, not the live
        // heading — or the number printed is not the number to type and the
        // instrument lies. (PLUMB_0 C3: this named apply_mood_arrival as the
        // thing that adds the offset. That function was declared and never
        // defined; the base is a constant, not a function's input.)
        //
        // Printed on CHANGE and no faster than 4 Hz: a settled camera says
        // nothing, and silence is the resting state.

        // To (-180, 180]. std::remainder rounds the quotient to NEAREST, so
        // it lands in [-180, 180] already; the one open end is fixed here.
        inline float wrap_deg(float d) {
            const float r = std::remainder(d, 360.0f);
            return (r <= -180.0f) ? r + 360.0f : r;
        }

        inline void dump_camera_orbit(const GPUCameraState& cam, float gaze_heading, float t) {
            constexpr float RAD2DEG = 180.0f / 3.14159265359f;
            static float last_t = -1.0e9f;
            static float last_d = 0.0f, last_e = 0.0f, last_a = 0.0f;
            const float el  = cam.elevation * RAD2DEG;
            const float off = wrap_deg((cam.azimuth - gaze_heading) * RAD2DEG);
            const bool moved = std::fabs(cam.distance - last_d) > 0.05f
                            || std::fabs(el  - last_e) > 0.1f
                            || std::fabs(off - last_a) > 0.1f;
            if (!moved || t - last_t < 0.25f) return;
            last_t = t; last_d = cam.distance; last_e = el; last_a = off;
            std::cout << "[Camera] orbit: distance=" << std::fixed << std::setprecision(2) << last_d
                      << " elevation=" << std::setprecision(1) << last_e
                      << " azimuth-offset=" << last_a << "\n";
        }

        struct alignas(16) GPUFloatingEntityState {
            float pos[3];              //   0: world position (computed by GPU)
            float body_radius;         //  12: bounding/visual radius
            float orientation[4];      //  16: quaternion
            float influence_radius;    //  32: zone/terrain influence range
            float t;                   //  36: curve parameter
            float orbit_radius;        //  40: distance from anchor (orbit mode)
            float orbit_speed;         //  44: angular velocity (orbit mode)
            float color[3];            //  48: current appearance (coupling-driven)
            float orbit_height;        //  60: base altitude above terrain
            float anchor[3];           //  64: world anchor point
            float face_variance;       //  76: per-face color spread (monolith)
            float base_color[3];       //  80: seed-derived rest color
            uint32_t geometry_type;    //  92: 0=sphere, 1=monolith
            uint32_t motion_type;      //  96: 0=orbit, 1=hover-bob
            float spin_speed;          // 100: Y-axis rotation rate (hover-bob)
            float bob_amplitude;       // 104: vertical oscillation amplitude
            float bob_period;          // 108: vertical oscillation period
            float spin_tilt_x;         // 112: axis tilt X
            float spin_tilt_z;         // 116: axis tilt Z
            uint32_t entity_seed;      // 120: seed for VS face color hashing
            uint32_t is_active;        // 124: 0=inactive, 1=active
            float aspect_y;            // 128: Y-axis scale multiplier (1.0=cube, >1=tall, <1=flat)
            float aspect_z;            // 132: Z-axis scale multiplier (1.0=cube, <1=thin slab)
            // ─── Drift-integrator substrate (cube use; spheres ignore) ────
            float spring_stiffness;    // 136: pulls drift toward zero (1/s²)
            float drag;                // 140: exponential damping on drift_vel (1/s)
            float drift[3];            // 144: position offset from home (cube)
            uint32_t tier_idx;         // 156: runtime tier lookup for gain tables
            float drift_vel[3];        // 160: drift integrator velocity
            uint32_t behavior_id;      // 172: cube behavior registry index (Phase 3)
            // ─── Kite mode (Phase 3.3) ───────────────────────────────
            //
            // Field order: pawn_offset must sit at a 16-aligned offset
            // (176) for WGSL's vec3 alignment rules. behavior_phase
            // and follow_pawn (both u32, 4-aligned) follow it. Putting
            // pawn_offset later would force WGSL to insert 8 bytes of
            // padding, growing the GPU struct to 224 while C++ stayed
            // at 208 — buffer-binding size mismatch on any pipeline
            // that reads the array.
            float    pawn_offset[3];   // 176/180/184: cube position relative to pawn
            uint32_t behavior_phase;   // 188: per-slot phase hash for behavior diversity
            uint32_t follow_pawn;      // 192: 0=anchor-relative, 1=pawn-relative
            float plasticity;          // 196 — CONTACT_2 λ (0=elastic; drift→anchor leak). Was _pad0.
            // ─── The anchor law (ONE_ANCHOR_1) ────────────────────────
            // Goals may leap; values only walk. The CPU (and later the
            // music couplings) author these targets; update_cube walks
            // the live param (anchor.xz in mode 0, pawn_offset.xz in
            // mode 1) toward them each frame. At rest target == param
            // and the glide term is exactly zero.
            float target_x;            // 200 — glide target x. Was _pad1.
            float target_z;            // 204 — glide target z. Was _pad2.
        };                             // 208 total (13×16)

        struct alignas(16) GPURibbonState {
            float anchor[3];                                                    // 0
            float time;                                                         // 12
            uint32_t cube_count;                                                // 16
            float cube_size;                                                    // 20
            float height;                                                       // 24
            float checker_scatter;                                              // 28 — per-cell color jitter amplitude (CONTRAST skin)
            float color[3];                                                     // 32
            float lateral_amp;                                                  // 44
            float lateral_freq;                                                 // 48 (rad/s, head oscillation rate)
            float vertical_amp;                                                 // 52
            float vertical_freq;                                                // 56
            uint32_t seed;                                                      // 60 — spawn seed (GPU-side per-ribbon hash key)
            float propagation_speed;                                            // 64 (world units/s; head→tail trail rate)
            uint32_t is_visible;                                                // 68
            float orientation;                                                  // 72 (heading radians)
            uint32_t color_mode;                                                // 76
            uint32_t is_wander;                                                 // 80 — 1: the wander brain authors intent (the two fields below); 0: parked. Ridden when config.point_host == 2, whoever authored.
            float _pad1;                                                        // 84 — RIBBON_2: the brain moved to the head kernel; nothing writes a yaw here
            float wander_throttle;                                              // 88 — [0, 1]
            float _pad3;                                                        // 92
            float color_b[3];                                                   // 96 — second checker median (CONTRAST)
            float hue_spread;                                                   // 108 — radians; per-cell hue rotation amplitude (CONTRAST skin; 0 = CB-1 look)
        };                                                                      // 112 total (mirrors world.wgsl RibbonState)

        // Per-ring pose, written by ribbon_body, read by ribbon_vs /
        // shadow_ribbon_vs through g2:143. 48 B stride (vec4, vec4, vec3 +
        // alignment).
        struct alignas(16) GPURibbonRingTransform {
            float motor_p0[4];         // PGA motor rotor part        (16)
            float motor_p1[4];         // PGA motor translator part   (16)
            float center[3];           // ring world-space center     (12)
        };
        static_assert(sizeof(GPURibbonRingTransform) == 48, "GPURibbonRingTransform must be 48 bytes");

        // ── THE RIBBON'S BODY (RIBBON_1) — GPU-sovereign. The CPU writes the 48-B
        // head once, zeroed, at commit (seeded = 0); the head kernel seeds itself
        // and nothing is ever read back. Mirrors world.wgsl's RibbonHeadState /
        // RibbonSaddle / RibbonBody BYTE-FOR-BYTE (L3).
        struct alignas(16) GPURibbonHeadState {
            float    pos[3];          //  0  the pen (the live integrated head)
            float    heading;         // 12  unbounded; tailward = +dir(heading), flight = −dir(heading)
            float    y_vel;           // 16
            float    alt_target;      // 20
            uint32_t head_slot;       // 24  spine slot of the newest chord sample
            uint32_t seeded;          // 28  0 = lay the spawn arc on the next tick
            uint32_t tick;            // 32  frames since seed; parity selects the deform half
            float    yaw_eased;       // 36  the hands, low-passed
            float    throttle_eased;  // 40
            float    yaw_cmd;         // 44  RIBBON_3 — the one command (hands + rule), slew-limited
            float    wander_tx;       // 48  the wander brain's target (RIBBON_2 — the brain came home)
            float    wander_tz;       // 52
            uint32_t wander_seq;      // 56  targets drawn so far (0 = none yet)
            float    rule_eased;      // 60  RIBBON_3 — the rule's lateral word, low-passed
        };                            // 64
        struct alignas(16) GPURibbonSaddle {
            float pos[3];             //  0  ring 0's top face, set back toward the tail
            float heading;            // 12  the flight heading
            float yaw_off;            // 16  drawn-tangent yaw minus heading, wrapped
            float pitch;              // 20
            float roll;               // 24
            float _pad0;              // 28
        };                            // 32
        struct alignas(16) GPURibbonBody {
            GPURibbonHeadState head;                         //     0
            GPURibbonSaddle    saddle;                       //    64
            float emit[Dim::RIBBON_EMIT_TABLE][4];           //    96  [half][slot]: every EMIT_STRIDE-th ring,
                                                             //        xyz center, w radius. The body writes half
                                                             //        (tick & 1); the head and every ring read the
                                                             //        other; the field, after the body, reads the
                                                             //        written one.
            float deform[Dim::RIBBON_DEFORM_SLOTS][4];       //  3296  [half][ring][offset|velocity], ping-pong by tick parity
        };                                                   // 28896
        static_assert(sizeof(GPURibbonHeadState) == 64);
        static_assert(sizeof(GPURibbonSaddle) == 32);
        static_assert(sizeof(GPURibbonBody) == 28896);
        static_assert(offsetof(GPURibbonBody, saddle) == 64);
        static_assert(offsetof(GPURibbonBody, emit) == 96);
        static_assert(offsetof(GPURibbonBody, deform) == 3296);

        struct alignas(16) GPUArchGroundEntry {
            float pier_left_x;
            float pier_left_z;
            float pier_right_x;
            float pier_right_z;
            float ground_y;         // written by GPU compute: min of terrain at both legs
            uint32_t is_active;
            float pier_correction_left;   // retired (always 0 — the pier bake is gone)
            float pier_correction_right;  // retired (always 0 — the pier bake is gone)
        };
        static_assert(sizeof(GPUArchGroundEntry) == 32, "GPUArchGroundEntry must be 32 bytes");

        // Per-column ground truth: CPU writes center XZ + active flag;
        // GPU compute writes ground_y from the heightfield.
        struct alignas(16) GPUColumnGroundEntry {
            float center_x;
            float center_z;
            float ground_y;         // written by GPU compute from heightfield - correction
            uint32_t is_active;
            float pier_correction;  // retired (always 0 — the pier bake is gone)
            float _pad0;
            float _pad1;
            float _pad2;
        };
        static_assert(sizeof(GPUColumnGroundEntry) == 32, "GPUColumnGroundEntry must be 32 bytes");

        // Generative pyramids — tapered height function baked into heightfield.
        // Pawn blocked by step-height on steep faces (no separate collision).
        struct alignas(16) GPUPyramidInstance {
            float origin[2];        // world XZ center
            float half_size[2];     // base half extents (XZ)
            float height;           // apex height above origin
            float rotation;         // radians, rotates base footprint
            float edge_blend;       // smoothstep width at base perimeter
            float truncation;       // 0.0 = pointed, 0.5 = half flat-top
        };
        static_assert(sizeof(GPUPyramidInstance) == 32, "GPUPyramidInstance must be 32 bytes");

        struct alignas(16) GPUPyramidArray {
            uint32_t count;
            uint32_t _pad[3];
            GPUPyramidInstance instances[Dim::MAX_PYRAMID_INSTANCES];
        };
        static_assert(sizeof(GPUPyramidArray) == 16 + Dim::MAX_PYRAMID_INSTANCES * 32,
            "GPUPyramidArray must match WGSL layout");

        // MUST match world.wgsl::ArchMeshParams (§9.1).
        // If this struct gains/loses a field, the WGSL side and
        // its sizeof static_assert must be updated together.
        // Catenary parameter 'a' is precomputed on CPU (50-iter
        // bisection) to keep the shader simple.
        struct alignas(16) GPUArchMeshParams {
            float center_x;
            float center_z;
            float rotation;
            float half_span;
            float rise;
            float depth;
            float thickness;
            float pier_height;
            float burial;
            float catenary_a;
            uint32_t segs_u;
            uint32_t segs_v;
            float color_r;
            float color_g;
            float color_b;
            uint32_t is_active;
            // MOSAIC_1: 0 = plain; 1..65535 rides the index channel as
            // enc = mosaic_seed·64 + slot. GROWTH LAW: 64 → 80 with the
            // WGSL twin, same commit. Zero-init {} keeps every direct-
            // build path (portals) plain by construction.
            uint32_t mosaic_seed;
            uint32_t _pad80_0;
            uint32_t _pad80_1;
            uint32_t _pad80_2;
        };
        static_assert(sizeof(GPUArchMeshParams) == 80,
            "GPUArchMeshParams must be 80 bytes — keep in sync with world.wgsl::ArchMeshParams (MOSAIC_1: 64 → 80)");

        //
        // MUST match world.wgsl::ColumnMeshParams (§9.2).
        // If this struct gains/loses a field, the WGSL side and
        // its sizeof static_assert must be updated together.
        struct alignas(16) GPUColumnMeshParams {
            float center_x;
            float center_z;
            float height;
            float shaft_radius;
            float taper;
            float entasis;
            float base_height;
            float base_overhang;
            float capital_height;
            float capital_overhang;
            float burial;
            float color_r;
            float color_g;
            float color_b;
            uint32_t base_layers;
            uint32_t capital_layers;
            uint32_t segs_around;
            uint32_t shaft_rings;
            uint32_t is_active;
            uint32_t tier;              // 0=Pillar, 1=Doric, 2=Ornate, 3=Antenna
            // ─── Antenna drum colors (3 drums × RGB) ─────────────
            float drum_color_r1, drum_color_g1, drum_color_b1;
            float drum_color_r2, drum_color_g2, drum_color_b2;
            float drum_color_r3, drum_color_g3, drum_color_b3;
            uint32_t mosaic_seed;       // MOSAIC_1 — first pad repurposed (the indoor_height_cap precedent); 0 = plain
            float _pad128[2];           // pad to 128 bytes
        };
        static_assert(sizeof(GPUColumnMeshParams) == 128,
            "GPUColumnMeshParams must be 128 bytes — keep in sync with world.wgsl::ColumnMeshParams");

        // ─── Palm GPU structs ─────────────────────────────────────────────
        //
        // MUST match world.wgsl::PalmMeshParams (§9.3).
        // If this struct gains/loses a field, the WGSL side and
        // its sizeof static_assert must be updated together.
        struct alignas(16) GPUPalmMeshParams {
            float center_x, center_z;
            float height;
            float base_r, top_r;
            float lean, lean_dir;
            float bark_rings, bark_depth;
            float frond_count;
            float frond_len, frond_width;
            float frond_droop, frond_arch;
            float crown_spread, crown_skirt;
            float burial;
            float trunk_r, trunk_g, trunk_b;
            float frond_r, frond_g, frond_b;
            float aged_r, aged_g, aged_b;
            uint32_t trunk_segs, frond_segs;
            uint32_t is_active;
            float _pad0;
        };
        static_assert(sizeof(GPUPalmMeshParams) == 128,
            "GPUPalmMeshParams must be 128 bytes — keep in sync with world.wgsl::PalmMeshParams");

        struct alignas(16) GPUPalmGroundEntry {
            float center_x;
            float center_z;
            float ground_y;
            uint32_t is_active;
            float _pad0, _pad1, _pad2, _pad3;
        };
        static_assert(sizeof(GPUPalmGroundEntry) == 32, "GPUPalmGroundEntry must be 32 bytes");

        // ─── Cactus GPU structs ──────────────────────────────────────────
        //
        // MUST match world.wgsl::CactusMeshParams (§9.4).
        // If this struct gains/loses a field, the WGSL side and
        // its sizeof static_assert must be updated together.
        // 21 floats + 4 uint32_t = 100 bytes data + 28 bytes pad = 128
        struct alignas(16) GPUCactusMeshParams {
            float center_x, center_z;                    // 2 floats
            float height, radius, taper;                 // 3 floats
            float ribs, rib_depth;                       // 2 floats
            float lean, lean_dir;                        // 2 floats
            float cap_round;                             // 1 float
            float arm_count;                             // 1 float
            float arm_height, arm_length, arm_radius;    // 3 floats
            float arm_curve;                             // 1 float
            float body_r, body_g, body_b;                // 3 floats
            float rib_r, rib_g, rib_b;                   // 3 floats  = 21 floats
            uint32_t trunk_segs, arm_segs;               // 2 uint32
            uint32_t is_active;                          // 1 uint32
            uint32_t seed;                               // 1 uint32  = 4 uint32 = 25 fields = 100 bytes
            float _pad0, _pad1, _pad2, _pad3, _pad4, _pad5, _pad6;  // 7 pad = 128 bytes
        };
        static_assert(sizeof(GPUCactusMeshParams) == 128,
            "GPUCactusMeshParams must be 128 bytes — keep in sync with world.wgsl::CactusMeshParams");

        struct alignas(16) GPUCactusGroundEntry {
            float center_x;
            float center_z;
            float ground_y;
            uint32_t is_active;
            float _pad0, _pad1, _pad2, _pad3;
        };
        static_assert(sizeof(GPUCactusGroundEntry) == 32, "GPUCactusGroundEntry must be 32 bytes");

        // ─── Blade Cluster GPU structs ──────────────────────────────────
        //
        // MUST match world.wgsl::BladeClusterMeshParams (§9.5).
        // If this struct gains/loses a field, the WGSL side and
        // its sizeof static_assert must be updated together.
        struct alignas(16) GPUBladeClusterMeshParams {
            float center_x, center_z;                        // 2 floats
            float blade_count;                               // 1 float (cast to u32 in shader)
            float blade_h, blade_h_var, blade_w;             // 3 floats
            float splay, curve, twist, taper;                // 4 floats
            float blade_r, blade_g, blade_b;                 // 3 floats
            float aged_r, aged_g, aged_b;                    // 3 floats  = 16 floats = 64 bytes
            uint32_t blade_segs;                             // 1 uint32
            uint32_t is_active;                              // 1 uint32
            uint32_t seed;                                   // 1 uint32
            uint32_t _pad0;                                  // 1 uint32  = 4 uint32 = 16 bytes
        };                                                   // total = 80 bytes
        static_assert(sizeof(GPUBladeClusterMeshParams) == 80,
            "GPUBladeClusterMeshParams must be 80 bytes — keep in sync with world.wgsl::BladeClusterMeshParams");

        struct alignas(16) GPUBladeClusterGroundEntry {
            float center_x;
            float center_z;
            float ground_y;
            uint32_t is_active;
            float _pad0, _pad1, _pad2, _pad3;
        };
        static_assert(sizeof(GPUBladeClusterGroundEntry) == 32,
            "GPUBladeClusterGroundEntry must be 32 bytes");

        // GoL zone config — per-zone parameters for compute + fragment shader
        struct alignas(16) GPUGoLZoneConfig {
            float origin[2];
            float extent;
            uint32_t grid_size;
            float tick_period;
            // GOL_TEMPO_2 U5: the spring_stiffness word, renamed in
            // place when the column died — written by both derive
            // branches, read by nothing. Same offsets, same 80 bytes;
            // the static_assert below is the witness.
            float _pad_spring;
            float alive_height;
            float transition_fraction;
            uint32_t color_mode;
            float target_r;
            float target_g;
            float target_b;
            uint32_t algorithm;
            float wander_radius;
            float phase_randomness;
            uint32_t boundary_mode;
            float tempo_randomness;    // per-cell frequency scatter [0,1]
            float spring_variance;     // per-cell spring speed scatter [0,1]
            // GOL_RULES_1: the two trailing pad words, renamed in place.
            // Same offsets, same 80 bytes — the static_assert below is the
            // witness that nothing grew. Each slot carries exactly one
            // meaning: rule_mask is the Conway rows', field_fn the Pulse
            // rows', and each is 0 on the other algorithm's rows.
            uint32_t rule_mask;        // Conway B/S bitset: bit n birth, bit 9+n survival
            uint32_t field_fn;         // Pulse field function id (PulseField::)
        };
        static_assert(sizeof(GPUGoLZoneConfig) == 80, "GPUGoLZoneConfig must be 80 bytes");

        struct alignas(16) GPUGoLZoneArray {
            uint32_t count;            // number of active zones
            float t_beats;             // current musical time
            float dt;                  // frame delta time
            uint32_t tick_mask;        // bit N = zone N should tick Conway this frame
            GPUGoLZoneConfig zones[Dim::MAX_GOL_ZONES];
        };
        static_assert(sizeof(GPUGoLZoneArray) == 16 + Dim::MAX_GOL_ZONES * 80,
            "GPUGoLZoneArray must match WGSL layout");

        struct alignas(16) GPUZoneDeriveRequest {
            uint32_t slot;             // zone_config.zones[slot] to write
            int32_t nx;                // lattice node X
            int32_t nz;                // lattice node Z
            uint32_t algorithm;        // 0=Conway, 1=Pulse
            uint32_t height_enabled;   // 0 or 1
            uint32_t world_seed;       // master seed
            // GOL_TEMPO_2 U1: the leading pad word, renamed in place.
            // Same offsets, same 32 bytes — the static_assert below is
            // the witness that nothing grew (the GOL_RULES_1 precedent).
            // THE ONE-AUTHOR LAW: the CPU draws and snaps the tick to
            // GOL_TICK_LADDER, and the GPU reads this instead of drawing.
            float tick_period;         // beats, already on the ladder
            uint32_t _pad1;
        };
        static_assert(sizeof(GPUZoneDeriveRequest) == 32, "GPUZoneDeriveRequest must be 32 bytes");

        struct alignas(16) GPUZoneDeriveRequestArray {
            uint32_t count;
            uint32_t _pad0;
            uint32_t _pad1;
            uint32_t _pad2;
            GPUZoneDeriveRequest requests[Dim::MAX_GOL_ZONES];
        };
        static_assert(sizeof(GPUZoneDeriveRequestArray) == 16 + Dim::MAX_GOL_ZONES * 32,
            "GPUZoneDeriveRequestArray must match WGSL layout");

        // ── Pawn Aura: toroidal spring grid for persistent terrain influence ──
        static constexpr uint32_t PAWN_AURA_N = 64;

        struct alignas(16) GPUPawnAuraConfig {
            float cell_size;           // 3.125 (matches terrain cells)
            float influence_radius;    // world units
            float attack_stiffness;    // spring constant for attack
            float attack_damping;      // damping ratio
            float release_rate;        // exponential decay rate
            float dt;
            uint32_t effect_mask;      // STATUS: INTENT — uploaded, never read by the
                                       // kernel (TUNE_1 A4 census). tint_strength and
                                       // height_scale are the live gates.
            uint32_t aura_n;           // STATUS: INTENT — uploaded, never read; the
                                       // kernel uses the WGSL PAWN_AURA_N constant.
            float tint_strength;       // [0,1] blend strength
            float tint_r, tint_g, tint_b;  // signature color
            uint32_t delta_mode;       // 0=convergent (toward tint), 1=random per-cell
            float delta_magnitude;     // random mode: max offset per channel
            float t_beats;             // current musical time (for oscillation)
            float height_scale;        // height contribution scale (world units)
        };
        static_assert(sizeof(GPUPawnAuraConfig) == 64, "GPUPawnAuraConfig must be 64 bytes");

        struct alignas(16) GPUPawnAuraCell {
            int32_t cell_gx;           // world cell X (0x7FFFFFFF = empty)
            int32_t cell_gz;           // world cell Z
            float intensity;           // attack/release spring value [0,1]
            float velocity;            // attack/release spring velocity
            float delta_r;             // contextual color delta R
            float delta_g;             // contextual color delta G
            float delta_b;             // contextual color delta B
            float height_delta;        // per-cell height multiplier [0,1]
            float color_osc;           // color oscillation spring value [0,1]
            float color_osc_vel;       // color oscillation spring velocity
            float _pad0;
            float _pad1;
        };
        static_assert(sizeof(GPUPawnAuraCell) == 48, "GPUPawnAuraCell must be 48 bytes");

        // ── Orb sky layer: per-orb state + per-frame config ──
        // Orbs live on a fixed dome (sphere shell) centered at world origin.
        // Bones pass: static positions, color drift disabled, no force fields.
        struct alignas(16) GPUOrbState {
            float    pos[3];            //  0: dome-local position (world_pos = dome_center + pos)
            float    _pad0;             // 12
            float    vel[3];            // 16: world-space velocity (zero in bones)
            float    _pad1;             // 28
            float    base_color[3];     // 32: rgb at spawn (seed-derived)
            float    brightness;        // 44: 0..1
            float    current_color[3];  // 48: rgb, drifts near base_color
            float    twinkle_phase;     // 60: radians, seed-derived
            float    size;              // 64: world-space sprite half-size
            float    mass;              // 68: per-tier mass (1.0 if legacy)
            float    drag;              // 72: velocity decay rate (1/s)
            uint32_t tier_idx;          // 76: Pass 8 tier index (0 if legacy)
        };
        static_assert(sizeof(GPUOrbState) == 80, "GPUOrbState must be 80 bytes");

        struct alignas(16) GPUOrbConfig {
            // ── Core (offsets preserved since Pass 3) ────────────
            uint32_t count;              //  0: active orb count
            uint32_t seed;               //  4: world seed for init
            float    base_hue;           //  8: legacy (unused when palette_count > 0)
            float    hue_variance;       // 12: legacy (unused when palette_count > 0)
            float    brightness;         // 16: global brightness (palette value center)
            float    drag;               // 20: per-mood drag override
            float    noise_amp;          // 24: current noise amplitude (coupling-driven)
            float    dome_radius;        // 28: dome shell radius
            float    base_size;          // 32: sprite half-size
            float    dt;                 // 36: frame delta time
            float    t_seconds;          // 40: elapsed seconds
            float    force_radial;       // 44: polyphony-driven radial force
            // ── Pass 4: motion rules + dome rotation ─────────────
            uint32_t motion_rule;        // 48: 0=Brownian, 1=Orbital, 2=Frozen, 3=Flocking
            float    rotation_speed;     // 52: dome angular velocity (rad/s)
            float    rotation_axis_x;    // 56: rotation axis X
            float    rotation_axis_y;    // 60: rotation axis Y
            float    rotation_axis_z;    // 64: rotation axis Z
            float    orbital_base_speed; // 68: orbital angular velocity (rad/s)
            // ── Pass 4b+5: multi-pocket palette ──────────────────
            uint32_t palette_count;      // 72: 0=legacy single hue, 1..4 use palette
            float    value_variance;     // 76: per-orb HSV value spread
            float    pal0_hue;           // 80
            float    pal0_hue_var;       // 84
            float    pal0_sat;           // 88
            float    pal0_weight;        // 92
            float    pal1_hue;           // 96
            float    pal1_hue_var;       //100
            float    pal1_sat;           //104
            float    pal1_weight;        //108
            float    pal2_hue;           //112
            float    pal2_hue_var;       //116
            float    pal2_sat;           //120
            float    pal2_weight;        //124
            float    pal3_hue;           //128
            float    pal3_hue_var;       //132
            float    pal3_sat;           //136
            float    pal3_weight;        //140
            // ── Pass 5: color dynamics (coupling-driven + target) ─
            float    color_pulse;        //144: brightness-pulse intensity (0..1)
            float    color_converge;     //148: hue-convergence intensity (0..1)
            float    color_surge;        //152: saturation-surge intensity (0..1)
            float    hue_converge_target;//156: target hue for convergence (0..1)
            // ── Pass 7: pawn-anchored dome ────────────────────────
            float    dome_center_x;      //160: DEAD WIRE (orb VS eye-centers; ABI bytes)
            float    dome_center_y;      //164: dead wire
            float    dome_center_z;      //168: dead wire
            float    _pad_anchor;        //172: reserved (future anchor mode/rate)
            // ── Pass 8: tier profiles (header + 4 tier blocks) ────
            // tier_count = 0 → legacy uniform population.
            // Each tier block is 40 bytes (10 floats) at offsets
            // 192, 232, 272, 312. Fields per tier:
            //   mass_mult, drag_mult,
            //   size_min, size_max,
            //   brightness_min, brightness_max,
            //   noise_gain, force_gain, color_gain,
            //   cumulative_weight (CPU-computed from weights)
            uint32_t tier_count;         //176
            float    brownian_radial_sign;     //180  (±1; was _pad_tier0)
            float    brownian_vert_bias;       //184  (0 or 1; was _pad_tier1)
            float    brownian_coherence;       //188  (0 or 1; was _pad_tier2)
            float    tier0_mass_mult;         //192
            float    tier0_drag_mult;         //196
            float    tier0_size_min;          //200
            float    tier0_size_max;          //204
            float    tier0_brightness_min;    //208
            float    tier0_brightness_max;    //212
            float    tier0_noise_gain;        //216
            float    tier0_force_gain;        //220
            float    tier0_color_gain;        //224
            float    tier0_cumulative_weight; //228
            float    tier1_mass_mult;         //232
            float    tier1_drag_mult;         //236
            float    tier1_size_min;          //240
            float    tier1_size_max;          //244
            float    tier1_brightness_min;    //248
            float    tier1_brightness_max;    //252
            float    tier1_noise_gain;        //256
            float    tier1_force_gain;        //260
            float    tier1_color_gain;        //264
            float    tier1_cumulative_weight; //268
            float    tier2_mass_mult;         //272
            float    tier2_drag_mult;         //276
            float    tier2_size_min;          //280
            float    tier2_size_max;          //284
            float    tier2_brightness_min;    //288
            float    tier2_brightness_max;    //292
            float    tier2_noise_gain;        //296
            float    tier2_force_gain;        //300
            float    tier2_color_gain;        //304
            float    tier2_cumulative_weight; //308
            float    tier3_mass_mult;         //312
            float    tier3_drag_mult;         //316
            float    tier3_size_min;          //320
            float    tier3_size_max;          //324
            float    tier3_brightness_min;    //328
            float    tier3_brightness_max;    //332
            float    tier3_noise_gain;        //336
            float    tier3_force_gain;        //340
            float    tier3_color_gain;        //344
            float    tier3_cumulative_weight; //348
            // ── Pass 9: flocking mood-level parameters ──────────
            float    flock_sep_radius;            //352
            float    flock_align_radius;          //356
            float    flock_coh_radius;            //360
            float    flock_sep_weight;            //364
            float    flock_align_weight;          //368
            float    flock_coh_weight;            //372
            float    flock_max_speed;             //376
            float    flock_coupling_intensity;    //380  (0..1, polyphony-smoothed)
            // ── Pass 12: per-force signs + per-rule drag multipliers ──
            float    flock_sep_sign;              //384  (±1 — was flock_weight_sign)
            float    flock_align_sign;            //388  (±1 — was _pad_flock1)
            float    flock_coh_sign;              //392  (±1 — was _pad_flock2)
            float    rule_drag_brownian;          //396  (multiplier — was _pad_flock3)
            float    rule_drag_orbital;           //400  (multiplier — was _pad_flock4)
            float    rule_drag_frozen;            //404  (multiplier — was _pad_flock5)
            float    rule_drag_flocking;          //408  (multiplier — was _pad_flock6)
            float    orbital_alignment_mode;      //412  (0=scatter 1=parallel 2=mirror; was _pad_flock7)
            // ── Pass 9: per-tier flocking gains ─────────────────
            float    tier0_flock_sep_gain;        //416
            float    tier0_flock_align_gain;      //420
            float    tier0_flock_coh_gain;        //424
            float    orbital_speed_var_mult;      //428  (variance multiplier; was _tier0_flock_pad)
            float    tier1_flock_sep_gain;        //432
            float    tier1_flock_align_gain;      //436
            float    tier1_flock_coh_gain;        //440
            float    speed_mult;                  //444  (was _tier1_flock_pad; 1.0 = identity, population speed multiplier)
            float    tier2_flock_sep_gain;        //448
            float    tier2_flock_align_gain;      //452
            float    tier2_flock_coh_gain;        //456
            float    _tier2_flock_pad;            //460
            float    tier3_flock_sep_gain;        //464
            float    tier3_flock_align_gain;      //468
            float    tier3_flock_coh_gain;        //472
            float    _tier3_flock_pad;            //476
        };
        static_assert(sizeof(GPUOrbConfig) == 480, "GPUOrbConfig must be 480 bytes");

        struct alignas(16) GPUVPMatrix {
            float m[16];               // camera view-projection
            float light_vp[16];        // directional light shadow VP
        };

        struct alignas(16) GPUDirectionalLight {
            float direction[3];
            float _pad0;
            float color[3];
            float intensity;
            float ambient;
            float _pad1;
            float _pad2;
            float _pad3;
        };

        struct alignas(16) GPUPointLight {
            float position[3];
            float range;
            float color[3];
            float intensity;
        };

        struct alignas(16) GPUPointLightArray {
            uint32_t count;
            uint32_t _pad0;
            uint32_t _pad1;
            uint32_t _pad2;
            GPUPointLight lights[Dim::MAX_POINT_LIGHTS];
        };

        // Spot light for indoor scenes: ceiling-mounted cone with distance falloff.
        // Each light carries its own shadow VP for atlas shadow mapping.
        static constexpr uint32_t MAX_SPOT_LIGHTS = 4;

        struct alignas(16) GPUSpotLight {
            float position[3];
            float _pad0;
            float direction[3];
            float _pad1;
            float color[3];
            float intensity;
            float inner_cone;      // cos(inner angle)
            float outer_cone;      // cos(outer angle)
            float range;
            float _pad2;
            float view_proj[16];   // perspective shadow VP for this light
        };
        static_assert(sizeof(GPUSpotLight) == 128, "GPUSpotLight must be 128 bytes");

        struct alignas(16) GPUSpotLightArray {
            uint32_t count;
            uint32_t _pad0;
            uint32_t _pad1;
            uint32_t _pad2;
            GPUSpotLight lights[MAX_SPOT_LIGHTS];
        };
        static_assert(sizeof(GPUSpotLightArray) == 16 + MAX_SPOT_LIGHTS * 128,
            "GPUSpotLightArray layout check");

        // WALLET_1revA — THE LIGHTING BLOCK. One uniform binding where the
        // fragment stage used to spend three storage seats. The three
        // members keep their own structs and their own static_asserts
        // above; this only gives them one home on the GPU.
        // TWIN: world.wgsl `struct Lighting` (L3 MIRROR — same members,
        // same order, both rooms in one commit; GROWTH LAW).
        struct alignas(16) GPULighting {
            GPUDirectionalLight sun;      // offset   0, 48 B
            GPUPointLightArray  points;   // offset  48, 272 B
            GPUSpotLightArray   spots;    // offset 320, 528 B
        };
        static_assert(sizeof(GPULighting) == 848, "GPULighting must be 848 bytes");
        static_assert(offsetof(GPULighting, sun) == 0, "Lighting.sun at 0");
        static_assert(offsetof(GPULighting, points) == 48, "Lighting.points at 48");
        static_assert(offsetof(GPULighting, spots) == 320, "Lighting.spots at 320");

        // THE DRAW PLAN (ECONOMY_1 closing arm) — CPU face of the cull
        // kernel's classification input. TWIN: world.wgsl DrawPlanParams
        // (L3 MIRROR — same fields, same order, both rooms together).
        struct GPUDrawPlanParams {
            uint32_t lod0_count;
            uint32_t render_count;
            uint32_t rect_count;
            uint32_t _pad0;
            float    rects[8][4];   // corner.xy, extent.xy per active zone
        };
        static_assert(sizeof(GPUDrawPlanParams) == 16 + 8 * 16,
            "draw plan: header + 8 vec4 rects — mirror of WGSL DrawPlanParams");

        // FC list/args geometry — TWIN: world.wgsl FC_SEG_* consts.
        inline constexpr uint32_t FC_SEG_A_OFF   = 0;     // bytes
        inline constexpr uint32_t FC_SEG_A_BYTES = 512;   // 128 entries
        inline constexpr uint32_t FC_SEG_B_OFF   = 512;
        inline constexpr uint32_t FC_SEG_B_BYTES = 512;
        inline constexpr uint32_t FC_SEG_C_OFF   = 1024;
        inline constexpr uint32_t FC_SEG_C_BYTES = 1024;  // 256 entries
        inline constexpr uint32_t FC_LIST_BYTES  = 2048;
        inline constexpr uint32_t FC_ARGS_BYTES  = 15 * sizeof(uint32_t);  // 3 x 5-u32 draw-args slots

        // ─── ATLAS_1revB D3" — the shadow tile's light-index windows ──
        // One 256-byte window per spot light; window i holds the literal
        // i. 256 is minUniformBufferOffsetAlignment's core default and
        // therefore a legal dynamic offset on every conforming adapter
        // (the limit is "better is lower", so an adapter's own value is a
        // power of two <= 256 and divides 256). The payload is 4 bytes;
        // the rest of each window is padding the alignment demands.
        // ─── FORMAT_1 D1 — THE ONE AUTHORITY FOR THE SHADOW DEPTH FORMAT ──
        //
        // PASS_0 F1 found this format spelled at four coupled sites with no
        // twin marker, no shared constant and no static_assert between them:
        // the two texture descriptors below, renderer.hpp's shared
        // shadowDepth.format in the DepthStencilState every shadow pipeline
        // takes, and texel_bytes' case. A partial edit across those four does
        // not fail at compile time — it fails at PIPELINE CREATION, at
        // runtime, when a pipeline's depth format stops matching its
        // attachment's.
        //
        // One fact, one home. The class of defect F1 names is now
        // unconstructible: there is nothing left to edit partially.
        //
        // Whatever this becomes, two things must move with it and are
        // checked by boot rather than by the compiler: texel_bytes must
        // carry a case for it (or the [GPU Budget] line prints UNDERCOUNT),
        // and the comparison stack must stay comparison-only — no
        // filterable-float read of a unorm depth texture. FORMAT_1's U0
        // verified the second at this HEAD: every shadow read in world.wgsl
        // is textureSampleCompare or textureSampleCompareLevel.
        // FORMAT_1 U2 — Depth16Unorm. Core WebGPU (absent from
        // GPUFeatureName, so no grant is needed), 2 B/texel: both shadow
        // textures halve, 16 -> 8 MiB each, resident 32 -> 16, and the
        // per-frame model at four indoor lights 32 -> 16 MiB against the
        // 96 this era began with.
        inline constexpr wgpu::TextureFormat kShadowDepthFormat =
            wgpu::TextureFormat::Depth16Unorm;

        struct MeshVertex {
            float pos[3];
            float normal[3];
        };

        // Extended vertex for arch meshes: per-vertex color enables
        // terrain-derived tones and palette overrides per instance.
        struct ArchVertex {
            float pos[3];
            float normal[3];
            float color[3];
            uint32_t arch_index;
        };

        // Indoor shell vertex: position + normal + color (no entity index).
        // Used for ceiling, walls, and floor of indoor scenes.
        struct ShellVertex {
            float pos[3];
            float normal[3];
            float color[3];
        };

        // LATTICE_1 — what the bake reads, and nothing else. `extent` was
        // always Dim::PATCH_EXTENT (make_patch_params wrote the constant);
        // `resolution` is now Dim::PATCH_HEIGHTFIELD_N; `time` had no reader in
        // either kernel; `master_seed` folded into config.world_seed, which is
        // the same fact written with active_seed at world birth — and the
        // HEIGHTS already baked from the config copy while only the cells read
        // the struct, so the fold also retires a one-frame disagreement.
        // 32 B -> 16 B, and the twin is world.wgsl's PatchParams.
        struct alignas(16) GPUPatchParams {
            float origin[2];           // world XZ of the patch CENTER
            uint32_t layer;            // heightfield / cell-color array layer
            uint32_t _pad0;
        };

        struct GPUPatchInstance {
            float origin[2];           // world-space XZ of patch center
            float extent;              // side length in world units
            uint32_t layer;            // heightfield array layer to sample
        };

        //
        // No alignas(16): this is a <storage> struct, all members are 4-byte,
        // natural alignment is 4. alignas(16) would pad sizeof up to 928,
        // adding 12 dead bytes the shader never reads.
        struct GPUPatchGrid {
            int32_t  origin_x;         // grid-coord anchor (min patch_gx in active set)
            int32_t  origin_z;         //                   (min patch_gz in active set)
            uint32_t side;             // PATCH_PREGEN_SIDE
            float    cell_extent;      // PATCH_EXTENT
            uint32_t entries[Dim::MAX_ACTIVE_PATCHES];
        };

        static_assert(sizeof(GPUFrameSignal) == 80, "GPUFrameSignal must be 80 bytes (CUT_1f: the 256 B dead stats mirror left both rooms)");
        static_assert(offsetof(GPUFrameSignal, mount_phase) == 48,
            "RIBBON_1: the mount block took the sky block's trailing 32 bytes, "
            "same total, same boundary — the WGSL twin mirrors it field for field");
        // FIELD_2b: the field's eight dials graduated from WGSL consts to
        // this struct (the panel authors their rests). Eight floats where
        // two tail pads stood, plus two fresh pads to land the 16-byte
        // boundary: 592 - 8 + 32 + 8 = 624. Both rooms, same commit.
        // RIBBON_1: the ribbon's eleven dials land at that same tail — one pad
        // consumed, ten floats appended, two fresh pads to the boundary;
        // 624 -> 672. Both rooms, same commit.
        // RIBBON_2: the wander brain's four join them — one pad consumed,
        // three appended, one fresh pad; 672 -> 688. Both rooms, same commit.
        static_assert(sizeof(GPUDesignConfig) == 720,
            "GPUDesignConfig must be 720 bytes. PRUNING_1 P3 removed nine "
            "zero-read fields (44 B) and added 12 B of DECLARED PAD: WGSL "
            "aligns vec3 to 16 while C++ packs float[3] at 4, and dropping "
            "44 B moved all four vec3 members off their boundaries. "
            "592 - 44 + 12 = 560. The pads ARE the mirror, not waste — the "
            "offsetof asserts below are what prove it. "
            "(MOSAIC_0: +8 floats, 560 -> 592 — Jean OK'd at handoff. "
            "MOSAIC_2 re-cut that tail from six dials + two pads to FIVE + "
            "THREE — still 8 floats, so 592 is unmoved. FIELD_2b: the "
            "field's eight dials land at the tail — two pads consumed, six "
            "floats appended, two fresh pads to the boundary; 592 -> 624. "
            "RIBBON_1: the ribbon's twelve dials at the same tail — one pad "
            "consumed, eleven appended, one fresh pad; 624 -> 672. RIBBON_2: "
            "the wander brain's four at the same tail — one pad consumed, "
            "three appended, one fresh pad; 672 -> 688. KITE_1: the chase's "
            "feed-forward consumes that last pad IN PLACE — 688 is unmoved, "
            "and the trailing pad is spent. The witness's two presence dials "
            "met that: no pad to reuse, two appended, two fresh pads to the "
            "boundary; 688 -> 704. PANORAMA_1: the two subtraction masks — one pad consumed IN PLACE, one appended, three fresh pads to the "
            "boundary; 704 -> 720. PANORAMA_1: the PCF tap count consumes one of those three pads IN PLACE — 720 unmoved.)");
        // THE ALIGNMENT LAW (L4, docs/LAWS.md). These four are the only
        // offsets where the two rooms can disagree, and no witness here fires
        // when they do — grow at the TAIL (after checker_resultant's group) or
        // pad. The trailing 4-byte pad is spent, so the next knob meets this.
        static_assert(offsetof(GPUDesignConfig, sun_direction)     % 16 == 0
                   && offsetof(GPUDesignConfig, fog_color)         % 16 == 0
                   && offsetof(GPUDesignConfig, fade_color)        % 16 == 0
                   && offsetof(GPUDesignConfig, checker_resultant) % 16 == 0,
            "every float[3] whose WGSL twin is vec3<f32> must sit on a 16-byte "
            "boundary — WGSL rounds vec3 up to align 16 and C++ does not, so an "
            "off-boundary one silently shifts the whole mirror (see GROWTH LAW)");

        // Portal ellipse array — uploaded when portal set changes.
        // GPU behavior_player_controlled tests pawn against arch-shaped ellipses and writes portal_trigger.
        static constexpr uint32_t MAX_GPU_PORTALS = 32;
        struct alignas(16) GPUPortalEntry {
            float x;                  //  0  world XZ center
            float z;                  //  4
            float facing_cos;         //  8  cos(rotation)
            float facing_sin;         // 12  sin(rotation)
            float inv_span_sq;        // 16  1 / half_span²  (lateral — foot-to-foot)
            float inv_depth_sq;       // 20  1 / (depth*0.5)² (forward — walk-through)
            uint32_t arch_index;      // 24  maps to CPU entities_state_.arches[index]
            uint32_t kind;            // 28 — 0 forward, 1 back (ATRIUM_2: the passers walk forward doors only)
        };
        struct alignas(16) GPUPortalArray {
            uint32_t count;
            uint32_t _pad[3];
            GPUPortalEntry portals[MAX_GPU_PORTALS];
        };
        static_assert(sizeof(GPUPortalArray) == 16 + MAX_GPU_PORTALS * 32,
            "GPUPortalArray layout check");

        // ── THE AGENTS' ROOM CONSTANTS (CHORD_1) ──────────────────────
        // Five uniform seats became one block. Everything here is
        // CPU-authored at world/mood cadence, so one block is one binding
        // and one upload per beat of that clock. Mirrors
        // world.wgsl::AgentRoomConstants BYTE-FOR-BYTE; the asserts below
        // are the handshake (L3 — both rooms, same commit).
        //
        // WINDOWS, NOT HOMES (docs/CHORD.md): tier_gains and the two
        // occupier arrays each have exactly one CPU home and one authoring
        // site; this block is a transport window onto them, and the
        // authoring site writes every window it owns.
        struct alignas(16) GPUAgentRoomConstants {
            GPUPortalArray      portals;                                  //    0
            GPUAgentBehaviorDef behaviors[GPU_AGENT_BEHAVIOR_COUNT];      // 1040 (ATRIUM_4: 11 rows, +32 B)
            GPUAgentTierDef     tier_gains[GPU_AGENT_TIER_COUNT];         // 1392
            GPUColumnMeshParams occupier_cmg[Dim::MAX_COLUMN_INSTANCES];  // 1584
            GPUArchMeshParams   occupier_amg[Dim::MAX_ARCH_INSTANCES];    // 5680
        };
        static_assert(sizeof(GPUAgentRoomConstants) == 6960);
        static_assert(offsetof(GPUAgentRoomConstants, behaviors)    == 1040);
        static_assert(offsetof(GPUAgentRoomConstants, tier_gains)   == 1392);
        static_assert(offsetof(GPUAgentRoomConstants, occupier_cmg) == 1584);
        static_assert(offsetof(GPUAgentRoomConstants, occupier_amg) == 5680);
        // The two registries are contiguous, which is what lets
        // upload_agent_registries spend ONE write on both.
        static_assert(offsetof(GPUAgentRoomConstants, behaviors)
            + sizeof(GPUAgentBehaviorDef) * GPU_AGENT_BEHAVIOR_COUNT
            == offsetof(GPUAgentRoomConstants, tier_gains),
            "behaviors and tier_gains must stay adjacent — the registry upload writes them as one range");
        static_assert(sizeof(GPUAgentState) == 96, "GPUAgentState must be 96 bytes");
        static_assert(sizeof(GPUAgentState) % 16 == 0, "GPUAgentState must be 16-byte aligned");
        static_assert(sizeof(GPUAgentBehaviorDef) == 32, "GPUAgentBehaviorDef must be 32 bytes");
        static_assert(sizeof(GPUAgentBehaviorDef) % 16 == 0, "GPUAgentBehaviorDef must be 16-byte aligned");
        static_assert(sizeof(GPUAgentTierDef) == 48, "GPUAgentTierDef must be 48 bytes (CONTACT_2 added personal_radius + flee_gain_player; 40 data -> 48 with 8 B pad — the uniform array stride)");
        static_assert(sizeof(GPUAgentTierDef) % 16 == 0, "GPUAgentTierDef must be 16-byte aligned");
        static_assert(sizeof(GPUPawnFigure) == 288, "GPUPawnFigure must be 288 bytes");
        static_assert(sizeof(GPUPawnFigure) % 16 == 0, "GPUPawnFigure must be 16-byte aligned");
        static_assert(sizeof(GPUCameraState) == 48, "GPUCameraState must be 48 bytes");
        static_assert(sizeof(GPUFloatingEntityState) == 208, "GPUFloatingEntityState must be 208 bytes");
        static_assert(offsetof(GPUFloatingEntityState, target_x) == 200, "target_x must sit at _pad1's retired slot (200)");
        static_assert(offsetof(GPUFloatingEntityState, target_z) == 204, "target_z must sit at _pad2's retired slot (204)");
        static_assert(sizeof(GPURibbonState) == 112, "GPURibbonState must be 112 bytes");
        static_assert(offsetof(GPURibbonState, checker_scatter) == 28, "checker_scatter must sit at twist_amp's retired slot (28)");
        static_assert(offsetof(GPURibbonState, seed) == 60, "seed must sit at twist_freq's retired slot (60)");
        static_assert(offsetof(GPURibbonState, color_b) == 96, "color_b must sit 16-aligned at the old struct end (96)");
        static_assert(offsetof(GPURibbonState, hue_spread) == 108, "hue_spread must sit at CB-1's retired tail pad (108)");
        static_assert(sizeof(GPUVPMatrix) == 128, "GPUVPMatrix must be 128 bytes");

        // ── THE FIELD BUS (CHORD_2) ───────────────────────────────────
        // The field's two uniform windows in one block: the ribbon state
        // in, the authored emitter table in. Frame cadence — the fastest
        // member governs. Mirrors world.wgsl::FieldBus BYTE-FOR-BYTE (L3).
        //
        // RIBBON_1 TOOK THE RINGS OUT. The 6400 B of head poses was a CPU
        // walk uploaded twice a frame; the body is GPU-resident now and the
        // field reads its emit table through ribbon_body_read (g2:145). The
        // block went 6656 B to 256.
        //
        // WINDOWS, NOT HOMES (docs/CHORD.md): every member's home is
        // elsewhere and stays there. `ribbon` is also seated by the ribbon
        // room (g2:140) and the render rooms (g2:200) on the SAME buffers;
        // those seats are other windows, not other facts, and each
        // authoring site writes every window it owns.
        struct alignas(16) GPUFieldBus {
            GPURibbonState   ribbon;     //   0
            GPUFieldAuthored authored;   // 112
        };
        static_assert(sizeof(GPUFieldBus) == 256);
        static_assert(offsetof(GPUFieldBus, authored) == 112);

        // ── FRAME R (CHORD_3) ─────────────────────────────────────────
        // The render frame's block: lighting, the view-projection matrix,
        // the camera. Mirrors world.wgsl::FrameR BYTE-FOR-BYTE (L3).
        //
        // TWO AUTHORS, ONE BLOCK. lighting is CPU-authored (upload_lights,
        // mood.hpp). vp and camera are GPU-SOVEREIGN — update_camera_vp
        // writes both, and they reach this block by
        // CopyBufferToBuffer on the frame's own encoder. The CPU never
        // reads them back; the readback law is why the copy exists at all.
        // sphere_pos (BEQ_A) rides the same law: sphere slot 0's
        // position, written by update_sphere, copied 12 bytes from
        // Floating Entity Array offset 0 — the terrain fragment
        // stage's former per-pixel storage read, now one uniform.
        //
        // TWO INSTANCES back the two bind groups over frameRLayout_: main
        // (vp_data / camera_state) and photographer (photographer_vp /
        // photographer_camera_out). Same layout, same block, two frames
        // of reference.
        struct alignas(16) GPUFrameR {
            GPULighting    lighting;         //    0
            GPUVPMatrix    vp;               //  848
            GPUCameraState camera;           //  976
            float          sphere_pos[3];    // 1024 — BEQ_A third passenger
            float          _pad_sphere;      // 1036
        };
        static_assert(sizeof(GPUFrameR) == 1040);
        static_assert(offsetof(GPUFrameR, vp)         == 848);
        static_assert(offsetof(GPUFrameR, camera)     == 976);
        static_assert(offsetof(GPUFrameR, sphere_pos) == 1024);

        // ── SCENE CONSTANTS (CHORD_4) ─────────────────────────────────
        // The render room's mood-cadence block: the tier-gains window,
        // the figure profiles, the ribbon window. Mirrors
        // world.wgsl::SceneConstants BYTE-FOR-BYTE (L3).
        //
        // UNIFORM, not storage, for the reason the tier registry and the
        // figure table each carried alone before the merge: the render
        // VERTEX stage sits at the per-stage STORAGE cap and uniform has
        // its own budget. Do not "upgrade" this block.
        //
        // TWO SEATS, ONE BLOCK: the scene layout and the shadow layout
        // both bind it at g2:200. And two of its three members are
        // WINDOWS (docs/CHORD.md) — tier_gains' home also shows through
        // agent_room.tier_gains, ribbon's through the ribbon pipeline's
        // g2:140 and field_bus.ribbon. Each authoring site writes every
        // window it owns.
        struct alignas(16) GPUSceneConstants {
            GPUAgentTierDef tier_gains[GPU_AGENT_TIER_COUNT];   //    0
            GPUPawnFigure   figure_profiles[PAWN_FIGURE_COUNT]; //  192
            GPURibbonState  ribbon;                             // 4224
        };
        static_assert(sizeof(GPUSceneConstants) == 4336);
        static_assert(offsetof(GPUSceneConstants, figure_profiles) == 192);
        static_assert(offsetof(GPUSceneConstants, ribbon)          == 4224);
        static_assert(sizeof(GPUDirectionalLight) == 48, "GPUDirectionalLight must be 48 bytes");
        static_assert(sizeof(GPUPointLight) == 32, "GPUPointLight must be 32 bytes");
        static_assert(sizeof(GPUPointLightArray) == 272, "GPUPointLightArray must be 272 bytes");
        static_assert(sizeof(MeshVertex) == 24, "MeshVertex must be 24 bytes");
        // C1 (cable management): pin the GPU-written vertex formats to the vertex-buffer
        // arrayStride the render/shadow pipelines declare — the stride is ALSO the WGSL
        // ArchVertexInput/ShellVertexInput layout, so this is the C++<->WGSL contract that
        // was previously unguarded. ArchVertex is the
        // SHARED format for six families (arch/column/palm/cactus/blade/pyramid mesh-gen).
        static_assert(sizeof(ArchVertex) == 40, "ArchVertex must be 40 bytes (arch/shadow VBL arrayStride = 40; WGSL ArchVertexInput)");
        static_assert(sizeof(ShellVertex) == 36, "ShellVertex must be 36 bytes (shell/shadow VBL arrayStride = 36; WGSL ShellVertexInput)");
        static_assert(sizeof(GPUPatchParams) == 16, "LATTICE_1: the twin is 16 bytes");
        static_assert(sizeof(GPUPatchInstance) == 16, "GPUPatchInstance must be 16 bytes");
        static_assert(sizeof(GPUPatchGrid) == 16 + Dim::MAX_ACTIVE_PATCHES * 4,
            "GPUPatchGrid must be 16 bytes header + 4 bytes/entry");

        // Unified painting slot — CPU mirror of WGSL UnifiedPaintingSlot (must match).
        // Both terrain-quad (photographer) and wall-frame (indoor) forms use this.
        // Each pipeline reads the fields it needs; form_type selects which pipeline draws it.
        namespace FormType { constexpr uint32_t TERRAIN_QUAD = 0; constexpr uint32_t WALL_FRAME = 1; }
        namespace ContentSource { constexpr uint32_t AUTHORED = 0; constexpr uint32_t SNAPSHOT = 1; }

        struct alignas(16) GPUPaintingSlot {
            // --- Common ---
            float position[3];          // world-space center
            uint32_t texture_layer;     // index into unified texture array
            float forward[3];           // facing direction (quad) / wall normal (frame)
            uint32_t form_type;         // 0 = TERRAIN_QUAD, 1 = WALL_FRAME
            float up[3];               // local up direction
            uint32_t is_active;         // 0 = empty slot

            // --- Sizing (both forms) ---
            float scale_x;             // width in world units
            float scale_y;             // height in world units
            float uv_scale_x;         // texture sub-region X [0,1]
            float uv_scale_y;         // texture sub-region Y [0,1]

            // --- Terrain quad fields ---
            float geometry_seed;       // deformation seed [0,1]
            uint32_t content_source;   // 0 = AUTHORED, 1 = SNAPSHOT
            int32_t patch_gx;         // lifecycle: owning patch grid X
            int32_t patch_gz;         // lifecycle: owning patch grid Z

            // --- Wall frame fields ---
            float frame_depth;         // frame extrusion depth
            float frame_width;         // frame border width
            float canvas_recess;       // canvas behind frame front
            float _pad0;

            float frame_color[3];      // frame wood color
            float _pad1;

            float _pad2[4];            // pad to 128 bytes
        };
        static_assert(sizeof(GPUPaintingSlot) == 128, "GPUPaintingSlot must be 128 bytes");

        // Photographer camera configuration (CPU → GPU uniform).
        // GPU compute shader reads this + actual pawn position → builds VP.
        struct alignas(16) GPUPhotographerConfig {
            float sun_direction[3];  // for shadow VP computation
            float azimuth;           // horizontal angle around pawn
            float elevation;         // vertical angle (radians)
            float distance;          // distance from pawn
            float fov_rad;           // vertical FOV in radians
            float aspect_ratio;      // width / height
            uint32_t patch_count;    // active patches for terrain sampling
            float frame_offset_x;   // pawn horizontal shift in frame [-1, 1] (0 = centered)
            float frame_offset_y;   // pawn vertical shift in frame [-1, 1] (0 = centered)
            float _pad0;
        };
        static_assert(sizeof(GPUPhotographerConfig) == 48, "GPUPhotographerConfig must be 48 bytes");

        // ═══ THE FRAME METER — GPU HALF (timestamp queries) ═══════════════
        // The spine's FrameMeter (cartridge.hpp) owns the census; the GPU
        // half lives here because every pass-encode site already reaches
        // gpuState_ through its face (MachineCtx / the per-module deps) —
        // the tree's one reachability grammar; no contract grows a member.
        //
        // NAME BINDING: the timestamp-writes struct types are DERIVED from
        // the pass descriptors themselves, so this binds to the header's
        // exact spelling under both Dawn generations (split ComputePass/
        // RenderPass structs, or the newer merged PassTimestampWrites) —
        // the descriptor member IS the binding. Field spellings
        // { querySet, beginningOfPassWriteIndex, endOfPassWriteIndex }
        // are common to both.
        using MeterComputeTsW = std::remove_const_t<std::remove_pointer_t<
            decltype(wgpu::ComputePassDescriptor::timestampWrites)>>;
        using MeterRenderTsW = std::remove_const_t<std::remove_pointer_t<
            decltype(wgpu::RenderPassDescriptor::timestampWrites)>>;

        // One armed pass = one { row, begin } pair; end index is begin+1.
        struct MeterPair { uint32_t row = 0; uint32_t begin_idx = 0; };

        // The meter's row registry: RENDER_SPINE row indices as raw ids,
        // visible to every pass-encode module (RPhase's home is
        // cartridge.hpp, included last in the cohort). Every value is
        // pinned to its RPhase by a static_assert beside the enum —
        // drift fails glaw1.
        namespace meter_row {
            // Row 8 (LiveCardWrite) carries NO GPU row: SPINE_2 B made the
            // card the first dispatch of the compute pass, so its cost is
            // inside DispatchCompute's span. The spine row still exists and
            // still owns the rest law — only the timestamp pair retired.
            inline constexpr uint32_t StreamPatches       = 2;
            inline constexpr uint32_t EntityMeshGen       = 6;
            inline constexpr uint32_t DispatchCompute     = 9;
            inline constexpr uint32_t GolDeriveFlush      = 11;
            inline constexpr uint32_t GolZoneCompute      = 12;
            inline constexpr uint32_t PawnAura            = 13;
            inline constexpr uint32_t OrbSky              = 14;
            inline constexpr uint32_t PlacementCorrection = 16;
            inline constexpr uint32_t FrustumCull         = 17;
            inline constexpr uint32_t ShadowPass          = 18;
            inline constexpr uint32_t MainPass            = 19;
            inline constexpr uint32_t SnapshotPass        = 20;
        }

        // GROUP 1 CARRIES A DYNAMIC SEAT (shadow_slot), so every bind of it
        // passes one offset. Everything outside the shadow atlas loop reads
        // record 0, which holds light 0. It lives here because both binders
        // — render_passes.hpp and bodies/gallery.hpp — include this file.
        inline constexpr uint32_t kFrameSlotZero = 0;

        class GPUState {
          public:
            // THE DRAW LEDGER'S VOCABULARY (BUNDLE_1) — declared at the head
            // of the class because the members below are of these types, and
            // public because the draw sites name records by hand
            // (GPUState::DR_ARCH). The ledger's own prose lives with its
            // methods, beside reset_frustum_indirect.
            enum DrawRecord : uint32_t {
                DR_ARCH, DR_COLUMN, DR_PALM, DR_CACTUS, DR_BLADE, DR_SHELL,
                DR_RIBBON, DR_WALL, DR_GALLERY_FRAME, DR_ORBS,
                DR_SHADOW_TERRAIN,          // both bands at LOD1 density, ONE draw
                DR_COUNT
            };
            // Stride 32 (not 20) so a record starts on a readable hex-dump
            // boundary; DrawIndexedIndirect takes any 4-aligned offset.
            static constexpr uint32_t DRAW_RECORD_STRIDE = 32;
            // indexed:     indexCount, instanceCount, firstIndex, baseVertex, firstInstance
            // non-indexed: vertexCount, instanceCount, firstVertex, firstInstance, (unused)
            struct DrawArgs { uint32_t a[5]; };

          private:
            wgpu::Device device_;
            GPUDesignConfig config_{};
            // ORGAN — one bit per panel-writable block, raised by
            // organ_mark_dirty and cleared by organ_flush.
            //
            // TOUCHED, NOT DIRTY. For lightingStage_ and agentRoomStage_
            // this bit IS the dirty bit, organ_flush being their only upload
            // path. For config_ it is the witness's record alone: config_
            // has its own dirty bit — configDirty_, which upload_config
            // tests — and a fact's home has one. The drivers' room (block 3)
            // has no GPU block and so no dirty bit anywhere: the seams that
            // read it each tick are its flush.
            uint32_t organTouched_ = 0;
            uint32_t organLastFlush_ = 0;
            bool configDirty_ = true;      // true at boot → first frame always uploads
            bool configDynamic_ = false;   // mood override: true = upload every frame
            wgpu::TextureFormat colorFormat_ = wgpu::TextureFormat::BGRA8Unorm;  // set in initOffscreenResources

            wgpu::Buffer signalBuffer_, configBuffer_;
            // Agent system — unified entity buffer. Slot 0 is the player's
            // body; slots 1..MAX_AGENTS-1 are mood-authored agents.
            wgpu::Buffer agentStateBuffer_;
            wgpu::Buffer agentStateReadbackStaging_;
            wgpu::Buffer floatingEntityReadbackStaging_;
            wgpu::Buffer cameraReadbackStaging_;   // ATRIUM_11 — always created; the pose is a spine fact (PLUMB_0 C1)
            // CHORD_1 — THE AGENTS' ROOM, one buffer where five stood
            // (portals, behaviors, tier gains, and the two occupier
            // windows). agentRoomStage_ is the sovereign CPU copy: every
            // authoring site updates it in place and then spends ONE
            // WriteBuffer at that member's own offset, so no site has to
            // know what the others wrote.
            wgpu::Buffer agentRoomBuffer_;
            GPUAgentRoomConstants agentRoomStage_{};
            // CHORD_4 — SCENE CONSTANTS, one buffer where three stood
            // (the tier registry's render window, the figure table, the
            // render-side ribbon window). Seated twice: scene and shadow.
            wgpu::Buffer sceneConstantsBuffer_;
            wgpu::Buffer cameraBuffer_, floatingEntityBuffer_;
            wgpu::Buffer ribbonBuffer_;
            wgpu::Buffer ringTransformsBuffer_;
            wgpu::Buffer ribbonSpineBuffer_;  // RIBBON_1: the chord ring — one vec4 per chord of flight, written by ribbon_head, read by ribbon_body
            wgpu::Buffer ribbonBodyBuffer_;   // RIBBON_1: head + saddle + emit + deform — GPU-sovereign; the CPU writes only the zeroed 48-B head at commit
            wgpu::Buffer fieldForcesBuffer_;  // FIELD_2: vec4<f32>[FIELD_SUBSCRIBER_CAP] — the field's one output, GPU-only (g2:3)
            // CHORD_2 — THE FIELD BUS, one buffer where three stood
            // (head poses, ribbon state, authored table). Each window's
            // authoring site writes it at its own offset; nothing here is
            // a home, so there is no stage copy to drift.
            wgpu::Buffer fieldBusBuffer_;
            GPUFieldAuthored fieldAuthoredStage_{};  // FIELD_4: the sovereign CPU copy, kept at the writer (the ribbon's lure reads it)
            // (bindings 21, 40 reserved — formerly proximity_field, cell_states)
            wgpu::Buffer vpBuffer_;
            // CHORD_3 — FRAME R, two instances of one 1024 B block where
            // three uniform buffers stood. Inside it, WALLET_1revA's
            // lighting still holds the ground it won: one 848 B uniform
            // member where three storage buffers used to be.
            wgpu::Buffer frameRMainBuffer_;
            // The light a shadow pass serves, on a dynamic-offset uniform
            // seat: MAX_SPOT_LIGHTS records holding 0..3, written once at
            // boot. Every group-1 render bind carries one offset.
            wgpu::Buffer shadowSlotBuffer_;
            wgpu::Buffer frameRPhotoBuffer_;
            // ORGAN — THE LIGHTING HOME. upload_lighting stores through it,
            // so it always carries what the GPU last received and the panel
            // edits it in place. A mood change re-authors it, which is
            // correct: a mood is a bigger authority than a dial.
            GPULighting lightingStage_{};

            // THE FRAME METER — GPU half. Query set + resolve/readback pair
            // (created only when the device carries timestamp-query) and the
            // per-frame arming state. The pass descriptors store a POINTER
            // to a writes struct — member storage keeps addresses stable
            // through encoding; rebuilt each frame (meter_frame_begin).
            static constexpr uint32_t METER_QUERY_COUNT = 64;   // 20 pass sites × 2 → next pow2, min 64
            static constexpr uint32_t METER_MAX_PAIRS = METER_QUERY_COUNT / 2;
            bool meterEnabled_ = false;
            wgpu::QuerySet meterQuerySet_;
            wgpu::Buffer meterResolveBuffer_;
            wgpu::Buffer meterReadbackStaging_;
            MeterComputeTsW meterComputeWrites_[METER_MAX_PAIRS] = {};
            MeterRenderTsW meterRenderWrites_[METER_MAX_PAIRS] = {};
            MeterPair meterPairs_[METER_MAX_PAIRS] = {};
            uint32_t meterPairCount_ = 0;
            uint32_t meterNextIndex_ = 0;

            // The frame's whole batch, contiguous, 16 B a patch, on a
            // read-only STORAGE seat: written once per frame and indexed by
            // workgroup_id.z inside the one fused dispatch (LATTICE_1).
            wgpu::Buffer patchParamsBuffer_;
            wgpu::Buffer patchInstancesBuffer_;
            // OIL_1 U10: shadow of the last-uploaded instance packing +
            // first-upload flag — the upload_patch_instances gate.
            GPUPatchInstance lastPatchInstances_[Dim::MAX_ACTIVE_PATCHES] = {};
            uint32_t lastPatchInstanceCount_ = 0;
            bool patchInstancesEverUploaded_ = false;
            wgpu::Buffer patchGridBuffer_;         // GPUPatchGrid — O(1) spatial index for sample_terrain_y_at
            // The draw ledger (BUNDLE_1) — the stage, what the GPU holds,
            // and the buffer the indirect draws read.
            DrawArgs drawLedgerStage_[DR_COUNT]{};
            DrawArgs drawLedgerShipped_[DR_COUNT]{};
            wgpu::Buffer drawLedgerBuffer_;
            bool bundlesDirty_ = true;      // BUNDLE_1 — record before the first frame

            wgpu::Buffer patchIndexBuffer_;
            wgpu::Buffer patchIndexBufferLOD1_;   // half-res index buffer for distant patches
            wgpu::Buffer patchIndexBufferCapOnly_; // ECONOMY_1 E1 — caps + skirt, no curtain band
            wgpu::Buffer tileGridBuffer_;
            uint32_t patchIndexCount_ = 0;
            uint32_t patchIndexCountRingClean_ = 0;   // CELL_1 rev2 — caps + skirt prefix
            uint32_t patchIndexCountRingZoned_ = 0;   // CELL_1 rev2 — prefix + curtain tail
            uint32_t patchIndexCountCapOnly_ = 0;
            // ECONOMY_1 E1 — the lift-conservative switch: true whenever a
            // zone lift COULD be nonzero (any zone slot active). The full IB
            // is always correct; the cap-only IB is correct exactly when all
            // lifts are zero, so either flip timing is safe.
            bool curtainsActive_ = true;
            // OPT_1e — the LOD1 rest switch: false only when ZERO zone
            // slots are active anywhere (true rest). Global by design —
            // zones outlive the veil ring's reach, and the curtain tail is
            // the only thing sealing lifted slab walls in the 175–325 wu
            // annulus, so RingZoned is always correct and the clean prefix
            // is correct exactly at rest. Defaults zoned (conservative).
            bool zonesActiveAnywhere_ = true;

            // Patch heightfield texture array — MAX_ACTIVE_PATCHES = 225
            // layers of PATCH_HEIGHTFIELD_N² (65×65), RGBA16Float. 7.6 MB,
            // from 118 (LATTICE_1): a texel per lattice point instead of
            // sixteen per mesh quad. RGBA16Float and not rg16float because
            // rg16float is not a core storage format; .zw stay unused.
            wgpu::Texture patchHeightfieldArrayTexture_;
            wgpu::TextureView patchHeightfieldArrayWriteView_;  // full array for storage write
            wgpu::TextureView patchHeightfieldArrayReadView_;   // full array for sampling

            // Patch cell color texture array (MAX_ACTIVE_PATCHES = 225 layers × 16×16, RGBA8Unorm)
            // RGB = cell color, A = mode (0=smooth, 1=discrete)
            wgpu::Texture patchCellColorArrayTexture_;
            wgpu::TextureView patchCellColorArrayWriteView_;
            wgpu::TextureView patchCellColorArrayReadView_;

            wgpu::Buffer sphereVertexBuffer_, sphereIndexBuffer_;
            uint32_t sphereIndexCount_ = 0;
            wgpu::Buffer monolithVertexBuffer_, monolithIndexBuffer_;
            uint32_t monolithIndexCount_ = 0;

            wgpu::Buffer archVertexBuffer_, archIndexBuffer_;
            wgpu::Buffer archGroundBuffer_;  // per-arch ground Y correction (GPU-corrected)
            wgpu::Buffer archMeshParamsBuffer_;  // GPU mesh gen: per-slot parameters
            uint32_t archIndexCount_ = 0;

            wgpu::Buffer columnVertexBuffer_, columnIndexBuffer_;
            wgpu::Buffer columnGroundBuffer_;  // per-column ground Y correction (GPU-corrected)
            wgpu::Buffer columnMeshParamsBuffer_;  // GPU mesh gen: per-slot parameters
            uint32_t columnIndexCount_ = 0;

            wgpu::Buffer palmVertexBuffer_, palmIndexBuffer_;
            wgpu::Buffer palmMeshParamsBuffer_;
            uint32_t palmIndexCount_ = 0;

            wgpu::Buffer cactusVertexBuffer_, cactusIndexBuffer_;
            wgpu::Buffer cactusMeshParamsBuffer_;
            uint32_t cactusIndexCount_ = 0;

            wgpu::Buffer bladeVertexBuffer_, bladeIndexBuffer_;
            wgpu::Buffer bladeMeshParamsBuffer_;
            uint32_t bladeIndexCount_ = 0;

            // Combined palm+cactus+blade ground buffer for compute Y-correction.
            // [0..23] palm, [24..43] cactus, [44..75] blade.  One storage binding.
            wgpu::Buffer plantComputeGroundBuffer_;

            wgpu::Buffer pyramidInstancesBuffer_;  // GPU-side pyramid array for heightfield baking (LIVE)

            // Indoor shell (ceiling + walls)
            wgpu::Buffer shellVertexBuffer_, shellIndexBuffer_;
            uint32_t shellIndexCount_ = 0;

            // ── LOOM_2 recut strata — layouts and groups (created in
            // binding_surface.gen.inc; declared here) ──
            wgpu::BindGroupLayout worldLayout_;
            wgpu::BindGroupLayout frameRLayout_;
            wgpu::BindGroupLayout frameCLayout_;
            wgpu::BindGroupLayout agentsStateLayout_;
            wgpu::BindGroupLayout agentsTexturesLayout_;
            wgpu::BindGroupLayout auraStateLayout_;
            wgpu::BindGroupLayout auraTexturesLayout_;
            wgpu::BindGroupLayout cullStateLayout_;
            wgpu::BindGroupLayout frameKStateLayout_;
            wgpu::BindGroupLayout frameKTexturesLayout_;
            wgpu::BindGroupLayout galleryStateLayout_;
            wgpu::BindGroupLayout galleryTexturesLayout_;
            wgpu::BindGroupLayout photoKStateLayout_;
            wgpu::BindGroupLayout photoKTexturesLayout_;
            wgpu::BindGroupLayout meshgenStateLayout_;
            wgpu::BindGroupLayout orbsAStateLayout_;
            wgpu::BindGroupLayout orbsBStateLayout_;
            wgpu::BindGroupLayout patchgenStateLayout_;
            wgpu::BindGroupLayout patchgenTexturesLayout_;
            wgpu::BindGroupLayout placeStateLayout_;
            wgpu::BindGroupLayout placeTexturesLayout_;
            wgpu::BindGroupLayout ribbonStateLayout_;
            wgpu::BindGroupLayout ribbonTexturesLayout_;   // SPINE_2 — the room reads the baked ground
            wgpu::BindGroupLayout sceneStateLayout_;
            wgpu::BindGroupLayout sceneTexturesLayout_;
            wgpu::BindGroupLayout shadowStateLayout_;
            wgpu::BindGroupLayout shadowTexturesLayout_;
            wgpu::BindGroupLayout zonesStateLayout_;
            wgpu::BindGroupLayout zonesTexturesLayout_;
            wgpu::BindGroupLayout emptyLayout_;
            wgpu::BindGroup worldGroup_;
            wgpu::BindGroup frameRGroup_;
            wgpu::BindGroup frameCGroup_;
            wgpu::BindGroup framePhotographerGroup_;
            wgpu::BindGroup agentsStateGroup_;
            wgpu::BindGroup agentsTexturesGroup_;
            wgpu::BindGroup auraStateGroup_;
            wgpu::BindGroup auraTexturesGroup_;
            wgpu::BindGroup cullStateGroup_;
            wgpu::BindGroup frameKStateGroup_;
            wgpu::BindGroup frameKTexturesGroup_;
            wgpu::BindGroup galleryStateGroup_;
            wgpu::BindGroup galleryTexturesGroup_;
            wgpu::BindGroup photoKStateGroup_;
            wgpu::BindGroup photoKTexturesGroup_;
            wgpu::BindGroup meshgenStateGroup_;
            wgpu::BindGroup meshgenStateColumnGroup_;
            wgpu::BindGroup meshgenStatePalmGroup_;
            wgpu::BindGroup meshgenStateCactusGroup_;
            wgpu::BindGroup meshgenStateBladeGroup_;
            wgpu::BindGroup orbsAStateGroup_;
            wgpu::BindGroup orbsBStateGroup_;
            wgpu::BindGroup patchgenStateGroup_;
            wgpu::BindGroup patchgenTexturesGroup_;
            wgpu::BindGroup placeStateGroup_;
            wgpu::BindGroup placeTexturesGroup_;
            wgpu::BindGroup ribbonStateGroup_;
            wgpu::BindGroup ribbonTexturesGroup_;   // SPINE_2 — was the shared EMPTY filler
            wgpu::BindGroup sceneStateGroup_;
            // DOMESDAY_1 B5 (R2): the PlanB / PlanC / Photographer scene
            // groups collapsed into sceneStateGroup_ — B3 retired the
            // segment windows that were their only difference.
            wgpu::BindGroup sceneTexturesGroup_;
            wgpu::BindGroup shadowStateGroup_;
            wgpu::BindGroup shadowTexturesGroup_;
            wgpu::BindGroup zonesStateGroup_;
            wgpu::BindGroup zonesTexturesGroup_;
            wgpu::BindGroup emptyGroup_;

            // GoL zone system buffers
            wgpu::Buffer zoneConfigBuffer_;        // GPUGoLZoneArray storage (read_write)
            wgpu::Buffer zoneDeriveRequestBuffer_; // GPUZoneDeriveRequestArray uniform
            wgpu::Buffer zoneLifeBuffer_;          // life state: MAX_ZONES × GOL_ZONE_LIFE_STRIDE (5120) floats
            wgpu::Texture zoneLifeTexture_;        // 32×32 × MAX_ZONES R32Float texture array
            wgpu::TextureView zoneLifeWriteView_;  // storage texture write (compute)
            wgpu::TextureView zoneLifeReadView_;   // sampled texture read (fragment)

            // Pawn aura system
            wgpu::Buffer pawnAuraConfigBuffer_;    // GPUPawnAuraConfig uniform
            wgpu::Buffer pawnAuraCellsBuffer_;     // GPUPawnAuraCell[] storage (N×N toroidal grid)
            wgpu::Texture pawnAuraTexture_;         // N×N RGBA16Float (compute writes, FS reads)
            wgpu::TextureView pawnAuraWriteView_;   // storage texture write (compute)
            wgpu::TextureView pawnAuraReadView_;    // sampled texture read (fragment)

            // Live card (GROUND_CARD_1) — RGBA16Float deformation field, LIVE_CARD_SIZE²
            wgpu::Texture liveCardTexture_;         // compute writes, VS/FS/compute read
            wgpu::TextureView liveCardWriteView_;   // storage texture write (writer kernel)
            wgpu::TextureView liveCardView_;        // sampled read (render + compute)

            // ── Orb sky layer ────────────────────────────────────────
            wgpu::Buffer orbStateBuffer_;          // MAX_ORBS × GPUOrbState (storage, read_write)
            wgpu::Buffer orbStatePrevBuffer_;      // MAX_ORBS × GPUOrbState (snapshot for flocking)
            wgpu::Buffer orbConfigBuffer_;         // GPUOrbConfig (uniform, per-frame)
            wgpu::Buffer orbQuadVB_;               // 4 vertices: billboard quad
            wgpu::Buffer orbQuadIB_;               // 6 indices: two triangles

            // Entity ground atlas (r32float, 256×1) — compute writes, VS reads
            wgpu::Texture entityGroundAtlasTexture_;
            wgpu::TextureView entityGroundAtlasWriteView_;   // storage texture (compute)
            wgpu::TextureView entityGroundAtlasReadView_;    // sampled texture (VS)

            wgpu::Texture shadowMapTexture_;
            wgpu::TextureView shadowMapView_;
            wgpu::Texture spotShadowMapTexture_;
            wgpu::TextureView spotShadowMapView_;

            wgpu::Sampler bilinearSampler_, nearestSampler_;
            wgpu::Sampler shadowSampler_;

            // --- Self-Portrait Gallery (photographer system) -------------------------
            wgpu::Buffer paintingSlotsBuffer_;
            wgpu::Buffer photographerVPBuffer_;
            wgpu::Buffer photographerCameraBuffer_;
            wgpu::Buffer photographerConfigBuffer_;

            // Three-array painting system: staging (2) + exhibition (1)
            wgpu::Texture snapshotStagingTexture_;    // 16 layers — photographer writes here
            wgpu::TextureView snapshotStagingReadView_;
            wgpu::Texture authoredStagingTexture_;     // 16 layers — disk images loaded here
            wgpu::TextureView authoredStagingReadView_;
            wgpu::Texture exhibitionTexture_;          // 32 layers — promoted images, GPU reads
            wgpu::TextureView exhibitionReadView_;

            wgpu::Texture offscreenColorTexture_;
            wgpu::TextureView offscreenColorView_;
            wgpu::Texture offscreenDepthTexture_;
            wgpu::TextureView offscreenDepthView_;
            wgpu::Texture offscreenMsaaColorTexture_;     // B10: msaa=4 only
            wgpu::TextureView offscreenMsaaColorView_;


            // GPU frustum culling — LOD0 only.
            // LOD1 always uses direct DrawIndexed; CPU computes its count.
            wgpu::Buffer frustumIndirectLOD0_;            // Indirect|CopyDst — 3 x 5-u32 draw-args (the plan)
            wgpu::Buffer frustumComputeBuffer_;           // Storage|CopySrc|CopyDst — compute writes here
            // WRAP_0 U4 — the slot line's staging. The draw plan's three
            // instance counters are GPU-side atomics; this is the only way to
            // read them, and it is read ONCE PER METER WINDOW, so its cost is
            // one 60-byte copy and one map a second. The source already
            // carries CopySrc for its own indirect draws.
            wgpu::Buffer frustumCountReadback_;
            wgpu::Buffer drawPlanBuffer_;                 // Uniform — counts + zone rects for the cull kernel
            // OIL_1 U7 (ledger: R17, C1): the shadow of the last-uploaded
            // plan + a first-upload flag. The plan's inputs move per band
            // change and per zone commit but were re-shipped every frame;
            // an event-only dirty flag would be WRONG (it would miss band
            // crossings), so the gate is compare-before-write against the
            // freshly packed plan.
            GPUDrawPlanParams lastDrawPlan_{};
            bool drawPlanEverUploaded_ = false;
            wgpu::Buffer visiblePatchIndicesBuffer_;      // MAX_ACTIVE_PATCHES × u32 — LOD0 visible list

            // GoL zone compute (dedicated layout: bindings 160-162, 167-169)

            wgpu::Sampler paintingSampler_;

        public:

            GPUState() = default;
            GPUState(const GPUState&) = delete;
            GPUState& operator=(const GPUState&) = delete;

            bool init(wgpu::Device device) {
                device_ = device;
                if (!createBuffers()) return false;
                if (!createMeshBuffers()) return false;
                if (!createTextures()) return false;
                if (!createSamplers()) return false;
                if (!createBindGroups()) return false;
                if (!initializeState()) return false;
                // PORT_4b — the budget report USED to fire here, and it was
                // wrong by 62 per cent: initOffscreenResources (the three
                // painting arrays, 416 MiB) runs LATER, from the cartridge's
                // init_renderer, so the largest family in the program was
                // absent from its own leaderboard. The report now fires at
                // the end of init_renderer, after the last allocation.
                return true;
            }

            // THE UPLOAD COLLAPSE: the WriteBuffer pattern lives in three shape
            // helpers, and the SIZE is derived from the value's type — a write can
            // never again carry a mismatched sizeof (the silent-corruption
            // class). The three shapes ARE a CADENCE taxonomy — the graph's first
            // TEMPORAL edges:
            //   writeStruct — Shape A: DIRTY-DRIVEN whole-buffer writes
            //   writeSlot   — Shape B: COMMIT-DRIVEN slot writes (base = a header
            //                 that precedes the slot array)
            //   writeArray  — Shape C: COUNT-DRIVEN array writes
            // The frame's PER-FRAME HOT fields are NOT here: they are the bespoke
            // offsetof field-writers (the config sub-writers, upload_*_frame,
            // zone header/life). A third tempo — left bespoke, offsetof asserts
            // undisturbed. (RIBBON_1 retired the ribbon's four: one whole-struct
            // upload_ribbon a frame says everything they said.)
            template <class T>
            void writeStruct(wgpu::Queue& queue, const wgpu::Buffer& buf, const T& v) {
                queue.WriteBuffer(buf, 0, &v, sizeof(T));
            }
            template <class T>
            void writeSlot(wgpu::Queue& queue, const wgpu::Buffer& buf, uint32_t slot,
                           const T& v, uint64_t base = 0) {
                queue.WriteBuffer(buf, base + (uint64_t)slot * sizeof(T), &v, sizeof(T));
            }
            template <class T>
            void writeArray(wgpu::Queue& queue, const wgpu::Buffer& buf,
                            const T* data, uint32_t count) {
                queue.WriteBuffer(buf, 0, data, (size_t)sizeof(T) * count);
            }

            // RIBBON_1: THE SIGNAL DRAINS WHOLE AGAIN. E-3's split existed because
            // the sky block carried a POSE the ribbon tick had to re-write after
            // the drain, so the two authors wrote disjoint regions. The mount
            // block that replaced it carries only the EDGE — captured by
            // possess(), advanced by the CPU, read by the GPU — so the frame's
            // one signal author says all of it in one write.
            void upload_signal(wgpu::Queue& queue, const GPUFrameSignal& signal) {
                writeStruct(queue, signalBuffer_, signal);
            }

            // Upload the agent behavior + tier registries to the GPU.
            // Called at world-init from the cartridge, and again at the
            // frame boundary when the panel edits the tier bank.
            // Behaviors are constexpr-equivalent (AGENT_BEHAVIORS,
            // bodies/agents.hpp) and never change during a session; the
            // tiers come from TIER_LIVE (contracts/agent_tiers.hpp), the
            // world's definition. Source data is passed as raw pointers
            // because the C++ tables are defined inside the cartridge
            // class scope and aren't visible from state.hpp; the cartridge
            // has both a translation step (CPU table → GPU struct) and
            // the queue access, so it owns the call site.
            void upload_agent_registries(wgpu::Queue& queue,
                const GPUAgentBehaviorDef* behaviors,
                uint32_t behavior_count,
                const GPUAgentTierDef* tiers,
                uint32_t tier_count) {
                std::memcpy(agentRoomStage_.behaviors, behaviors,
                    sizeof(GPUAgentBehaviorDef) * behavior_count);
                std::memcpy(agentRoomStage_.tier_gains, tiers,
                    sizeof(GPUAgentTierDef) * tier_count);
                // CHORD_1: both registries in one write — they are adjacent
                // in the block and the static_assert above says so.
                queue.WriteBuffer(agentRoomBuffer_,
                    offsetof(GPUAgentRoomConstants, behaviors),
                    agentRoomStage_.behaviors,
                    sizeof(agentRoomStage_.behaviors) + sizeof(agentRoomStage_.tier_gains));
                // CHORD_4: the registry's second window is the scene block's
                // now — the standalone buffer C1 kept alive retired with it.
                queue.WriteBuffer(sceneConstantsBuffer_,
                    offsetof(GPUSceneConstants, tier_gains),
                    agentRoomStage_.tier_gains, sizeof(agentRoomStage_.tier_gains));
            }

            // One-shot upload of the pawn figure table. Packs PAWN_FIGURES →
            // GPUPawnFigure[PAWN_FIGURE_COUNT] → buffer. Called once at world-init
            // from upload_agent_registries_to_gpu (bodies/agents.hpp).
            void upload_pawn_figures(wgpu::Queue& queue) {
                GPUPawnFigure figs[PAWN_FIGURE_COUNT];
                pack_pawn_figures(figs);
                queue.WriteBuffer(sceneConstantsBuffer_,
                    offsetof(GPUSceneConstants, figure_profiles), figs, sizeof(figs));
            }

            void upload_config(wgpu::Queue& queue) {
                if (!configDirty_ && !configDynamic_) return;
                configDirty_ = false;
                writeStruct(queue, configBuffer_, config_);   // Shape A, dirty-driven — the canonical cadence
            }


            // Targeted 4-byte upload of placement_patch_count — called from stream_patches
            // after world_state_.all_patch_count is finalized, so the placement compute pass reads the
            // current frame's patch set (decoupled from the photographer config).
            // THE OFFSET IS DERIVED (offsetof — it cannot drift).
            void upload_placement_patch_count(wgpu::Queue& queue) {
                queue.WriteBuffer(configBuffer_, offsetof(GPUDesignConfig, placement_patch_count),
                    &config_.placement_patch_count, sizeof(uint32_t));
            }

            // Targeted 8-byte upload of lod_point_x/z — called from stream_patches each
            // frame so the GPU frustum-cull shader uses the same POINT position as the
            // CPU's LOD banding (eliminates LOD0/LOD1 boundary flicker).
            void upload_lod_point(wgpu::Queue& queue) {
                static_assert(offsetof(GPUDesignConfig, lod_point_x) == 352,
                    "lod_point_x offset must be 384 for targeted upload");
                queue.WriteBuffer(configBuffer_,
                    offsetof(GPUDesignConfig, lod_point_x),
                    &config_.lod_point_x, sizeof(float) * 2);
            }

            // WALLET_1revA E8 — ONE write, not three. The three sources
            // already shared a single function (upload_lights, mood.hpp),
            // so the block is composed there and written whole; the
            // offset-write alternative at 0/48/320 was not needed.
            void upload_lighting(wgpu::Queue& queue, const GPULighting& lighting) {
                lightingStage_ = lighting;   // the home records what shipped
                // CHORD_3: two windows, one home. The block sits at offset 0
                // of both instances, and the photographer lights the same
                // world the main camera does.
                queue.WriteBuffer(frameRMainBuffer_, offsetof(GPUFrameR, lighting),
                    &lighting, sizeof(GPULighting));
                queue.WriteBuffer(frameRPhotoBuffer_, offsetof(GPUFrameR, lighting),
                    &lighting, sizeof(GPULighting));
            }

            // ── CHORD_3: THE GPU TRUTH ARRIVES BY COPY ────────────────
            // vp and camera are written by update_camera_vp and
            // read by the render stages. The CPU is not in that loop and
            // must not be (the readback law), so the frame's own encoder
            // carries them from their sovereign homes into the block,
            // after the pass that wrote them has closed. Both offsets and
            // both sizes are multiples of 4, as copyBufferToBuffer needs.
            void encode_frame_r_main_sync(wgpu::CommandEncoder& encoder) {
                encoder.CopyBufferToBuffer(vpBuffer_, 0, frameRMainBuffer_,
                    offsetof(GPUFrameR, vp), sizeof(GPUVPMatrix));
                encoder.CopyBufferToBuffer(cameraBuffer_, 0, frameRMainBuffer_,
                    offsetof(GPUFrameR, camera), sizeof(GPUCameraState));
                // BEQ_A third passenger — same encoder, same frame,
                // same sovereignty as vp and camera above.
                encoder.CopyBufferToBuffer(floatingEntityBuffer_, 0, frameRMainBuffer_,
                    offsetof(GPUFrameR, sphere_pos), 3 * sizeof(float));
            }

            // The photographer's instance, same law, its own sources.
            void encode_frame_r_photo_sync(wgpu::CommandEncoder& encoder) {
                encoder.CopyBufferToBuffer(photographerVPBuffer_, 0, frameRPhotoBuffer_,
                    offsetof(GPUFrameR, vp), sizeof(GPUVPMatrix));
                encoder.CopyBufferToBuffer(photographerCameraBuffer_, 0, frameRPhotoBuffer_,
                    offsetof(GPUFrameR, camera), sizeof(GPUCameraState));
                encoder.CopyBufferToBuffer(floatingEntityBuffer_, 0, frameRPhotoBuffer_,
                    offsetof(GPUFrameR, sphere_pos), 3 * sizeof(float));
            }

            // ATLAS_1revB U3" — stage_spot_vps and spot_vp_staging() are
            // retired. They filled a 256-byte duplicate of bytes
            // upload_lighting had already sent to the lighting block from the
            // same cpuSpotLights_ array: every spot light's view_proj is a
            // member of GPUSpotLight, at offset 320 of GPULighting. The
            // shadow VS reads it there now, through shadow_light_vp(), which
            // is the same array sample_spot_shadow_pcf has always indexed in
            // the fragment stage. One fact, one home.
            wgpu::Buffer vp_buffer() const { return vpBuffer_; }
            static constexpr size_t light_vp_offset() { return offsetof(GPUVPMatrix, light_vp); }
            static constexpr size_t light_vp_size() { return 16 * sizeof(float); }

            // The light index rides a dynamic offset on frame R: record i holds i.
            static constexpr uint32_t shadow_slot_offset(uint32_t light) { return light * Dim::UNIFORM_DYNAMIC_STRIDE; }

            // THE BATCH'S PARAMS, one contiguous write (LATTICE_1). The bake
            // reads them as a storage ARRAY indexed by workgroup_id.z, so the
            // records pack at their own 16-byte stride where they used to sit
            // one per 256-byte dynamic-uniform slot — 57.6 KB of ring becomes
            // 3.6 KB. The dispatch that reads them is recorded after this call
            // and executes after the write on the queue timeline.
            //
            // NO CURSOR: stream_patches makes exactly ONE batch a frame now
            // (LATTICE_1, the R-D fold), so every batch starts at record 0.
            // The clamp stays — a bound stated is a bound that cannot rot.
            uint32_t upload_patch_params(wgpu::Queue& queue, const GPUPatchParams* params,
                                         uint32_t count) {
                if (count > Dim::MAX_ACTIVE_PATCHES) {
                    std::fprintf(stderr, "[PatchParams] batch of %u exceeds the %u-record buffer; clamped\n",
                                 count, Dim::MAX_ACTIVE_PATCHES);
                    count = Dim::MAX_ACTIVE_PATCHES;
                }
                if (count > 0)
                    queue.WriteBuffer(patchParamsBuffer_, 0, params,
                                      count * sizeof(GPUPatchParams));
                return count;
            }

            void upload_tile_grid(wgpu::Queue& queue, const GPUTileGrid& grid) {
                writeStruct(queue, tileGridBuffer_, grid);
            }

            void upload_patch_instances(wgpu::Queue& queue, const GPUPatchInstance* instances, uint32_t count) {
                // OIL_1 U10 (ledger: R3 band_patches, C1): the caller
                // re-bands every frame (that recompute is the change
                // detector — band membership must track the moving
                // point); the WRITE fires only when the packed bytes
                // moved. Same compare-before-write pattern as the draw
                // plan (U7). memcmp is total: GPUPatchInstance is four
                // 4-byte members, padding-free (static_assert'd at the
                // struct), every field assigned at the pack site. Sole
                // caller: band_patches.
                if (patchInstancesEverUploaded_ &&
                    count == lastPatchInstanceCount_ &&
                    std::memcmp(lastPatchInstances_, instances,
                                (size_t)count * sizeof(GPUPatchInstance)) == 0) return;
                std::memcpy(lastPatchInstances_, instances,
                            (size_t)count * sizeof(GPUPatchInstance));
                lastPatchInstanceCount_ = count;
                patchInstancesEverUploaded_ = true;
                writeArray(queue, patchInstancesBuffer_, instances, count);
            }

            void upload_patch_grid(wgpu::Queue& queue, const GPUPatchGrid& grid) {
                writeStruct(queue, patchGridBuffer_, grid);
            }

            // ── CHORD_2: the ribbon state wears THREE windows ─────────
            // ribbonBuffer_ is the home the ribbon room and the render rooms
            // read (g2:140, g2:200); field_bus.ribbon is the agents room's
            // window onto the same fact. RIBBON_1 left ONE writer — the whole
            // struct, three windows, once a frame — so the rule the partial
            // writers obeyed field by field is now obeyed by construction.
            // RIBBON_WINDOW is the one place the second address is computed;
            // it is 0 today and named anyway, because a window is an address
            // and not a coincidence.
            static constexpr uint64_t RIBBON_WINDOW = offsetof(GPUFieldBus, ribbon);
            // CHORD_4 gave the same fact a third window: the render room's
            // (scene + shadow bind it at g2:200). Same rule, one more address.
            static constexpr uint64_t RIBBON_SCENE_WINDOW = offsetof(GPUSceneConstants, ribbon);

            void upload_ribbon(wgpu::Queue& queue, const GPURibbonState& ribbon) {
                writeStruct(queue, ribbonBuffer_, ribbon);
                queue.WriteBuffer(fieldBusBuffer_, RIBBON_WINDOW, &ribbon, sizeof(GPURibbonState));
                queue.WriteBuffer(sceneConstantsBuffer_, RIBBON_SCENE_WINDOW, &ribbon, sizeof(GPURibbonState));
            }

            // RIBBON_1: the CPU's only word to the body — "you are unseeded".
            // The head kernel lays the spawn arc and seeds itself; the body
            // kernel zeroes its deformation on its first tick after a seed.
            void reset_ribbon_body(wgpu::Queue& queue) {
                GPURibbonHeadState zero{};
                queue.WriteBuffer(ribbonBodyBuffer_, 0, &zero, sizeof(zero));
            }

            void upload_floating_entity_slot(wgpu::Queue& queue, uint32_t slot, const GPUFloatingEntityState& entity) {
                writeSlot(queue, floatingEntityBuffer_, slot, entity);
            }

            // Sphere slots: 0 .. MAX_SPHERE_INSTANCES-1  (direct offset)
            void upload_sphere_entity_slot(wgpu::Queue& queue, uint32_t slot, const GPUFloatingEntityState& entity) {
                upload_floating_entity_slot(queue, slot, entity);
            }

            // Cube slots: 0 .. MAX_CUBE_INSTANCES-1  (offset by CUBE_SLOT_OFFSET in buffer)
            void upload_cube_entity_slot(wgpu::Queue& queue, uint32_t slot, const GPUFloatingEntityState& entity) {
                upload_floating_entity_slot(queue, Dim::CUBE_SLOT_OFFSET + slot, entity);
            }

            // Partial write: just the behavior_id u32 inside a cube slot.
            // Used by the Phase-3 override-cycling path to flip behavior on
            // every active cube without re-uploading the whole 208-byte
            // struct. Field offset is calculated at compile time.
            void upload_cube_behavior_id(wgpu::Queue& queue, uint32_t slot, uint32_t behavior_id) {
                size_t base = (Dim::CUBE_SLOT_OFFSET + slot) * sizeof(GPUFloatingEntityState);
                size_t off = offsetof(GPUFloatingEntityState, behavior_id);
                queue.WriteBuffer(floatingEntityBuffer_, base + off, &behavior_id, sizeof(uint32_t));
            }
            void upload_cube_orbit_height(wgpu::Queue& queue, uint32_t slot, float h) {
                size_t base = (Dim::CUBE_SLOT_OFFSET + slot) * sizeof(GPUFloatingEntityState);
                size_t off = offsetof(GPUFloatingEntityState, orbit_height);
                queue.WriteBuffer(floatingEntityBuffer_, base + off, &h, sizeof(float));
            }
            void upload_cube_body_radius(wgpu::Queue& queue, uint32_t slot, float r) {
                size_t base = (Dim::CUBE_SLOT_OFFSET + slot) * sizeof(GPUFloatingEntityState);
                size_t off = offsetof(GPUFloatingEntityState, body_radius);
                queue.WriteBuffer(floatingEntityBuffer_, base + off, &r, sizeof(float));
            }
            void upload_cube_aspects(wgpu::Queue& queue, uint32_t slot, float ay, float az) {
                size_t base = (Dim::CUBE_SLOT_OFFSET + slot) * sizeof(GPUFloatingEntityState);
                size_t off = offsetof(GPUFloatingEntityState, aspect_y);
                float a[2] = { ay, az };   // aspect_z rides aspect_y — adjacent (128/132)
                queue.WriteBuffer(floatingEntityBuffer_, base + off, a, sizeof(a));
            }
            void upload_cube_face_variance(wgpu::Queue& queue, uint32_t slot, float v) {
                size_t base = (Dim::CUBE_SLOT_OFFSET + slot) * sizeof(GPUFloatingEntityState);
                size_t off = offsetof(GPUFloatingEntityState, face_variance);
                queue.WriteBuffer(floatingEntityBuffer_, base + off, &v, sizeof(float));
            }

            // Partial writes for the anchor law (ONE_ANCHOR_1). The
            // toggle writes only the follow_pawn sentinel (the kernel
            // captures/releases from the true present); the corral
            // writes only the glide target (the kernel walks the live
            // param toward it). No CPU hand ever moves anchor or
            // offset directly.
            // ── FIELD_4: the authored table's hot writer ── the CPU
            // stage is SOVEREIGN; the GPU buffer is the derived copy.
            // 144 B WriteBuffer per frame — lean, no dirty gate.
            void upload_field_authored(wgpu::Queue& queue, const GPUFieldAuthored& t) {
                fieldAuthoredStage_ = t;
                // CHORD_2: the table's only GPU seat is the bus window now.
                queue.WriteBuffer(fieldBusBuffer_, offsetof(GPUFieldBus, authored),
                    &t, sizeof(GPUFieldAuthored));
            }
            const GPUFieldAuthored& field_authored_stage() const { return fieldAuthoredStage_; }

            void upload_cube_follow_pawn(wgpu::Queue& queue, uint32_t slot, uint32_t follow) {
                size_t base = (Dim::CUBE_SLOT_OFFSET + slot) * sizeof(GPUFloatingEntityState);
                size_t off = offsetof(GPUFloatingEntityState, follow_pawn);
                queue.WriteBuffer(floatingEntityBuffer_, base + off, &follow, sizeof(uint32_t));
            }
            void upload_cube_glide_target(wgpu::Queue& queue, uint32_t slot, float tx, float tz) {
                size_t base = (Dim::CUBE_SLOT_OFFSET + slot) * sizeof(GPUFloatingEntityState);
                size_t off = offsetof(GPUFloatingEntityState, target_x);
                float t[2] = { tx, tz };   // target_z rides target_x — pinned adjacent (200/204)
                queue.WriteBuffer(floatingEntityBuffer_, base + off, t, sizeof(t));
            }
            void upload_cube_color(wgpu::Queue& queue, uint32_t slot, float r, float g, float b) {
                size_t base = (Dim::CUBE_SLOT_OFFSET + slot) * sizeof(GPUFloatingEntityState);
                size_t off = offsetof(GPUFloatingEntityState, color);
                float col[3] = { r, g, b };   // color[3] rides one write — contiguous floats (48/52/56)
                queue.WriteBuffer(floatingEntityBuffer_, base + off, col, sizeof(col));
            }

            // GPU mesh gen: write params for a single arch slot (64 bytes per spawn/evict)
            void upload_arch_mesh_params_slot(wgpu::Queue& queue, uint32_t slot, const GPUArchMeshParams& params) {
                writeSlot(queue, archMeshParamsBuffer_, slot, params);
                // CHORD_1 — the occupier window on the same row. The mesh-gen
                // kernel reads the storage face above; the agents room reads
                // this one. One authored geometry, one home; the authoring
                // site writes every window it owns, so the rows and the mesh
                // can never disagree.
                agentRoomStage_.occupier_amg[slot] = params;
                writeSlot(queue, agentRoomBuffer_, slot, params,
                    offsetof(GPUAgentRoomConstants, occupier_amg));
            }

            // Arch GPU mesh gen bind group (dedicated layout — bindings 193-195)
            // ── LOOM_2 recut strata accessors ──
            wgpu::BindGroupLayout world_layout() const { return worldLayout_; }
            wgpu::BindGroupLayout frame_r_layout() const { return frameRLayout_; }
            wgpu::BindGroupLayout frame_c_layout() const { return frameCLayout_; }
            wgpu::BindGroupLayout agents_state_layout() const { return agentsStateLayout_; }
            wgpu::BindGroupLayout agents_textures_layout() const { return agentsTexturesLayout_; }
            wgpu::BindGroupLayout aura_state_layout() const { return auraStateLayout_; }
            wgpu::BindGroupLayout aura_textures_layout() const { return auraTexturesLayout_; }
            wgpu::BindGroupLayout cull_state_layout() const { return cullStateLayout_; }
            wgpu::BindGroupLayout frame_k_state_layout() const { return frameKStateLayout_; }
            wgpu::BindGroupLayout frame_k_textures_layout() const { return frameKTexturesLayout_; }
            wgpu::BindGroupLayout gallery_state_layout() const { return galleryStateLayout_; }
            wgpu::BindGroupLayout gallery_textures_layout() const { return galleryTexturesLayout_; }
            wgpu::BindGroupLayout photo_k_state_layout() const { return photoKStateLayout_; }
            wgpu::BindGroupLayout photo_k_textures_layout() const { return photoKTexturesLayout_; }
            wgpu::BindGroupLayout meshgen_state_layout() const { return meshgenStateLayout_; }
            wgpu::BindGroupLayout orbs_a_state_layout() const { return orbsAStateLayout_; }
            wgpu::BindGroupLayout orbs_b_state_layout() const { return orbsBStateLayout_; }
            wgpu::BindGroupLayout patchgen_state_layout() const { return patchgenStateLayout_; }
            wgpu::BindGroupLayout patchgen_textures_layout() const { return patchgenTexturesLayout_; }
            wgpu::BindGroupLayout place_state_layout() const { return placeStateLayout_; }
            wgpu::BindGroupLayout place_textures_layout() const { return placeTexturesLayout_; }
            wgpu::BindGroupLayout ribbon_state_layout() const { return ribbonStateLayout_; }
            wgpu::BindGroupLayout ribbon_textures_layout() const { return ribbonTexturesLayout_; }
            wgpu::BindGroupLayout scene_state_layout() const { return sceneStateLayout_; }
            wgpu::BindGroupLayout scene_textures_layout() const { return sceneTexturesLayout_; }
            wgpu::BindGroupLayout shadow_state_layout() const { return shadowStateLayout_; }
            wgpu::BindGroupLayout shadow_textures_layout() const { return shadowTexturesLayout_; }
            wgpu::BindGroupLayout zones_state_layout() const { return zonesStateLayout_; }
            wgpu::BindGroupLayout zones_textures_layout() const { return zonesTexturesLayout_; }
            wgpu::BindGroupLayout empty_layout() const { return emptyLayout_; }
            wgpu::BindGroup world_group() const { return worldGroup_; }
            wgpu::BindGroup frame_r_group() const { return frameRGroup_; }
            wgpu::BindGroup frame_c_group() const { return frameCGroup_; }
            wgpu::BindGroup frame_photographer_group() const { return framePhotographerGroup_; }
            wgpu::BindGroup agents_state_group() const { return agentsStateGroup_; }
            wgpu::BindGroup agents_textures_group() const { return agentsTexturesGroup_; }
            wgpu::BindGroup aura_state_group() const { return auraStateGroup_; }
            wgpu::BindGroup aura_textures_group() const { return auraTexturesGroup_; }
            wgpu::BindGroup cull_state_group() const { return cullStateGroup_; }
            wgpu::BindGroup frame_k_state_group() const { return frameKStateGroup_; }
            wgpu::BindGroup frame_k_textures_group() const { return frameKTexturesGroup_; }
            wgpu::BindGroup gallery_state_group() const { return galleryStateGroup_; }
            wgpu::BindGroup gallery_textures_group() const { return galleryTexturesGroup_; }
            wgpu::BindGroup photo_k_state_group() const { return photoKStateGroup_; }
            wgpu::BindGroup photo_k_textures_group() const { return photoKTexturesGroup_; }
            wgpu::BindGroup meshgen_state_group() const { return meshgenStateGroup_; }
            wgpu::BindGroup meshgen_state_column_group() const { return meshgenStateColumnGroup_; }
            wgpu::BindGroup meshgen_state_palm_group() const { return meshgenStatePalmGroup_; }
            wgpu::BindGroup meshgen_state_cactus_group() const { return meshgenStateCactusGroup_; }
            wgpu::BindGroup meshgen_state_blade_group() const { return meshgenStateBladeGroup_; }
            wgpu::BindGroup orbs_a_state_group() const { return orbsAStateGroup_; }
            wgpu::BindGroup orbs_b_state_group() const { return orbsBStateGroup_; }
            wgpu::BindGroup patchgen_state_group() const { return patchgenStateGroup_; }
            wgpu::BindGroup patchgen_textures_group() const { return patchgenTexturesGroup_; }
            wgpu::BindGroup place_state_group() const { return placeStateGroup_; }
            wgpu::BindGroup place_textures_group() const { return placeTexturesGroup_; }
            wgpu::BindGroup ribbon_state_group() const { return ribbonStateGroup_; }
            wgpu::BindGroup ribbon_textures_group() const { return ribbonTexturesGroup_; }
            wgpu::BindGroup scene_state_group() const { return sceneStateGroup_; }
            wgpu::BindGroup scene_textures_group() const { return sceneTexturesGroup_; }
            wgpu::BindGroup shadow_state_group() const { return shadowStateGroup_; }
            wgpu::BindGroup shadow_textures_group() const { return shadowTexturesGroup_; }
            wgpu::BindGroup zones_state_group() const { return zonesStateGroup_; }
            wgpu::BindGroup zones_textures_group() const { return zonesTexturesGroup_; }
            wgpu::BindGroup empty_group() const { return emptyGroup_; }

            void upload_painting_slots(wgpu::Queue& queue, const GPUPaintingSlot* slots, uint32_t count) {
                writeArray(queue, paintingSlotsBuffer_, slots, count);
            }

            void upload_painting_slot(wgpu::Queue& queue, uint32_t index, const GPUPaintingSlot& slot) {
                writeSlot(queue, paintingSlotsBuffer_, index, slot);
            }

            void deactivate_painting_slot(wgpu::Queue& queue, uint32_t index) {
                uint32_t zero = 0;
                queue.WriteBuffer(paintingSlotsBuffer_,
                    index * sizeof(GPUPaintingSlot) + offsetof(GPUPaintingSlot, is_active),
                    &zero, sizeof(uint32_t));
            }

            void upload_photographer_vp(wgpu::Queue& queue, const GPUVPMatrix& vp) {
                writeStruct(queue, photographerVPBuffer_, vp);
            }

            void upload_photographer_camera(wgpu::Queue& queue, float x, float y, float z) {
                GPUCameraState cam{};
                cam.pos[0] = x; cam.pos[1] = y; cam.pos[2] = z;
                writeStruct(queue, photographerCameraBuffer_, cam);
            }

            void upload_photographer_config(wgpu::Queue& queue, const GPUPhotographerConfig& cfg) {
                writeStruct(queue, photographerConfigBuffer_, cfg);
            }

            // Upload an authored image into the unified painting texture array.
            // Handles R↔B swap if the array is in BGRA format (Windows/Dawn).
            void upload_authored_painting(wgpu::Queue& queue, uint32_t layer,
                const uint8_t* rgba_data, uint32_t width, uint32_t height)
            {
                bool need_swap = (colorFormat_ == wgpu::TextureFormat::BGRA8Unorm);
                std::vector<uint8_t> swapped;
                const uint8_t* src = rgba_data;

                if (need_swap) {
                    uint32_t n = width * height * 4;
                    swapped.resize(n);
                    for (uint32_t i = 0; i < width * height; ++i) {
                        swapped[i * 4 + 0] = rgba_data[i * 4 + 2]; // B
                        swapped[i * 4 + 1] = rgba_data[i * 4 + 1]; // G
                        swapped[i * 4 + 2] = rgba_data[i * 4 + 0]; // R
                        swapped[i * 4 + 3] = rgba_data[i * 4 + 3]; // A
                    }
                    src = swapped.data();
                }

                wgpu::TexelCopyTextureInfo dest{};
                dest.texture = authoredStagingTexture_;
                dest.mipLevel = 0;
                dest.origin = { 0, 0, layer };
                dest.aspect = wgpu::TextureAspect::All;

                wgpu::TexelCopyBufferLayout layout{};
                layout.offset = 0;
                layout.bytesPerRow = width * 4;
                layout.rowsPerImage = height;

                wgpu::Extent3D extent = { width, height, 1 };
                queue.WriteTexture(&dest, src, width * height * 4, &layout, &extent);
            }

            void fill_painting_layer_solid(wgpu::Queue& queue, uint32_t layer,
                uint8_t r, uint8_t g, uint8_t b)
            {
                bool need_swap = (colorFormat_ == wgpu::TextureFormat::BGRA8Unorm);
                uint32_t N = Dim::PAINTING_RESOLUTION;
                std::vector<uint8_t> pixels(N * N * 4);
                for (uint32_t i = 0; i < N * N; ++i) {
                    pixels[i * 4 + 0] = need_swap ? b : r;
                    pixels[i * 4 + 1] = g;
                    pixels[i * 4 + 2] = need_swap ? r : b;
                    pixels[i * 4 + 3] = 255;
                }
                upload_authored_painting(queue, layer, pixels.data(), N, N);
            }

            // Late init: offscreen resources need the swapchain color format,
            // which isn't known until init_renderer time.
            bool initOffscreenResources(wgpu::TextureFormat colorFormat) {
                colorFormat_ = colorFormat;

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

                auto makeArrayView = [&](wgpu::Texture tex, const char* label, uint32_t layers) -> wgpu::TextureView {
                    wgpu::TextureViewDescriptor vd{};
                    vd.dimension = wgpu::TextureViewDimension::e2DArray;
                    vd.arrayLayerCount = layers;
                    vd.label = label;
                    return tex.CreateView(&vd);
                    };

                // Snapshot staging — photographer writes here, promotion copies from here
                // DEEDED: the photographer's portfolio. 32 speculative shots
                // persist here between capture and hang-time curation; retiring
                // this pool changes which shots hang. Do not retire without a
                // photographer-model ruling (DOMESDAY R5 withdrawal, _1 report B8).
                snapshotStagingTexture_ = makeTextureArray("Snapshot Staging",
                    Dim::STAGING_LAYERS,
                    wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc);
                if (!snapshotStagingTexture_) return false;
                snapshotStagingReadView_ = makeArrayView(snapshotStagingTexture_,
                    "Snapshot Staging View", Dim::STAGING_LAYERS);

                // Authored staging — disk images loaded here, promotion copies from here
                // DEEDED: the painting inventory. Prefetch-decoded canvases wait
                // here across world rebuilds; on unified memory a CPU-side
                // inventory costs the same bytes, and decode-at-hang regresses
                // the transition. Re-open only via a browser-owned (ImageBitmap)
                // path on a vendored port (DOMESDAY R5 withdrawal).
                authoredStagingTexture_ = makeTextureArray("Authored Staging",
                    Dim::STAGING_LAYERS,
                    wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc);
                if (!authoredStagingTexture_) return false;
                authoredStagingReadView_ = makeArrayView(authoredStagingTexture_,
                    "Authored Staging View", Dim::STAGING_LAYERS);

                // Exhibition — promoted images live here, GPU reads for rendering
                exhibitionTexture_ = makeTextureArray("Exhibition",
                    Dim::EXHIBITION_LAYERS,
                    wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding);
                if (!exhibitionTexture_) return false;
                exhibitionReadView_ = makeArrayView(exhibitionTexture_,
                    "Exhibition View", Dim::EXHIBITION_LAYERS);

                // Offscreen snapshot render target
                {
                    wgpu::TextureDescriptor desc{};
                    desc.size = { Dim::PAINTING_RESOLUTION, Dim::PAINTING_RESOLUTION, 1 };
                    desc.format = colorFormat;
                    desc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
                    offscreenColorTexture_ = makeTexture("Offscreen Snapshot Color", desc);
                    if (!offscreenColorTexture_) return false;
                    offscreenColorView_ = offscreenColorTexture_.CreateView();
                }
                {
                    wgpu::TextureDescriptor desc{};
                    desc.size = { Dim::PAINTING_RESOLUTION, Dim::PAINTING_RESOLUTION, 1 };
                    desc.format = wgpu::TextureFormat::Depth24Plus;
                    desc.sampleCount = effective_msaa();   // B10: 1 = pre-B10 shape
                    desc.usage = wgpu::TextureUsage::RenderAttachment;
                    offscreenDepthTexture_ = makeTexture("Offscreen Snapshot Depth", desc);
                    if (!offscreenDepthTexture_) return false;
                    offscreenDepthView_ = offscreenDepthTexture_.CreateView();
                }
                // DOMESDAY_2 B10 — the snapshot's msaa color target, created
                // only when the boot param asked for it: the pass renders here
                // at count 4 and resolves into offscreenColorTexture_; the
                // portfolio copy chain downstream reads the resolved
                // single-sample color exactly as today.
                if (effective_msaa() == 4u) {
                    wgpu::TextureDescriptor desc{};
                    desc.size = { Dim::PAINTING_RESOLUTION, Dim::PAINTING_RESOLUTION, 1 };
                    desc.format = colorFormat;
                    desc.sampleCount = 4;
                    desc.usage = wgpu::TextureUsage::RenderAttachment;
                    offscreenMsaaColorTexture_ = makeTexture("Offscreen Snapshot MSAA Color", desc);
                    if (!offscreenMsaaColorTexture_) return false;
                    offscreenMsaaColorView_ = offscreenMsaaColorTexture_.CreateView();
                }

                if (!create_gallery_texture_group()) return false;

                return true;
            }

            // Muting
            void set_mute_coupling(uint32_t b, bool m) {
                uint32_t prev = config_.mute_couplings;
                if (m) config_.mute_couplings |= b; else config_.mute_couplings &= ~b;
                if (config_.mute_couplings != prev) configDirty_ = true;
            }

            // ─── The point's host + fly speed ────────────────────────
            void set_point_host(uint32_t h) {
                if (config_.point_host != h) { config_.point_host = h; configDirty_ = true; }
            }
            void set_point_fly_speed(float s) {
                if (config_.point_fly_speed != s) { config_.point_fly_speed = s; configDirty_ = true; }
            }
            void set_shadow_pcf_taps(uint32_t n) {
                if (config_.shadow_pcf_taps != n) { config_.shadow_pcf_taps = n; configDirty_ = true; }
            }
            // THE SUBTRACTION MASKS ARE THE BUNDLES' ONE LIVE RAISER
            // (BUNDLE_1). PANORAMA_1's rule is that a cleared bit is skipped
            // AT THE ENCODER, so the pass row reads the absence — culling in
            // the shader would still pay the vertex work and the meter would
            // read no difference. Under a bundle the encoder-time skip is
            // taken at RECORDING, so a mask that changed without a re-record
            // would do nothing at all and the instrument would silently stop
            // working. Nothing in the tree could have caught that: it is not
            // a type error, not a validation error, and not a wrong pixel —
            // it is a dial that stopped answering.
            void set_draw_mask(uint32_t m) {
                if (config_.draw_mask != m) { config_.draw_mask = m; configDirty_ = true; bundlesDirty_ = true; }
            }
            void set_shadow_mask(uint32_t m) {
                if (config_.shadow_mask != m) { config_.shadow_mask = m; configDirty_ = true; bundlesDirty_ = true; }
            }

            // BUNDLES CAPTURE OBJECTS, NOT VALUES. Raised by every site that
            // recreates a bind group or a buffer a bundle holds — R-B found
            // none post-boot today, so in practice this fires once at boot
            // and again whenever a mask dial turns. A NEW recreation site
            // must raise it; that is the standing rule, filed in OPEN.md.
            bool bundles_dirty() const { return bundlesDirty_; }
            void raise_bundles_dirty() { bundlesDirty_ = true; }
            void clear_bundles_dirty() { bundlesDirty_ = false; }
            void set_fpv_mode(uint32_t m) {
                if (config_.fpv_mode != m) { config_.fpv_mode = m; configDirty_ = true; }
            }
            void set_sun_direction(float x, float y, float z) {
                if (config_.sun_direction[0] != x || config_.sun_direction[1] != y || config_.sun_direction[2] != z) {
                    config_.sun_direction[0] = x; config_.sun_direction[1] = y; config_.sun_direction[2] = z;
                    configDirty_ = true;
                }
            }
            void set_world_seed(uint32_t seed) {
                if (config_.world_seed != seed) { config_.world_seed = seed; configDirty_ = true; }
            }
            //
            // skin_id rides with tier + color: the preserved-body set (CLOSURE_PAWN [5]).
            // Default 0 = regular pawn, for callers with no prior body (boot).
            void reset_player_agent(wgpu::Queue& queue, uint32_t tier_idx = 0u,
                                    float color_r = 0.0f, float color_g = 0.0f, float color_b = 0.0f,
                                    uint32_t skin_id = 0u) {
                GPUAgentState buf[Dim::MAX_AGENTS] = {};
                auto& p = buf[0];
                p.pos_x = Idle::PAWN_POS_X;
                p.pos_y = Idle::PAWN_POS_Y;
                p.pos_z = Idle::PAWN_POS_Z;
                p.heading = Idle::PAWN_HEADING;
                p.orient_x = 0.0f;
                p.orient_y = 0.0f;
                p.orient_z = 0.0f;
                p.orient_w = 1.0f;
                p.is_active = 1u;
                p.behavior_id = 0u;  // AGENT_BEHAVIOR_PLAYER_CONTROLLED
                p.tier_idx = tier_idx;
                p.portal_trigger = -1;
                p.color_r = color_r;
                p.color_g = color_g;
                p.color_b = color_b;
                p.skin_id = skin_id;
                queue.WriteBuffer(agentStateBuffer_, 0, buf, sizeof(buf));
            }
            // Upload the full 32-slot agent array. Slot 0 (player) is rewritten
            // to whatever the caller has in agent_state_.slots[0] — caller is responsible
            // for keeping that mirror consistent with the player's idle pose.
            void upload_agent_state_all(wgpu::Queue& queue, const GPUAgentState* src) {
                writeArray(queue, agentStateBuffer_, src, Dim::MAX_AGENTS);
            }
            // Upload one slot only. Used by per-frame respawns so writes
            // don't race with the GPU's own update of slot 0 (the player).
            void upload_agent_slot(wgpu::Queue& queue,
                uint32_t slot,
                const GPUAgentState* src) {
                if (slot >= Dim::MAX_AGENTS) return;
                writeSlot(queue, agentStateBuffer_, slot, *src);
            }
            void set_fog(float density, float r, float g, float b) {
                if (config_.fog_density != density ||
                    config_.fog_color[0] != r || config_.fog_color[1] != g || config_.fog_color[2] != b) {
                    config_.fog_density = density;
                    config_.fog_color[0] = r; config_.fog_color[1] = g; config_.fog_color[2] = b;
                    configDirty_ = true;
                }
            }
            void set_aura_enabled(bool on) {
                float v = on ? 1.0f : 0.0f;
                if (config_.aura_enabled != v) { config_.aura_enabled = v; configDirty_ = true; }
            }
            // Possessed body's tilt lag. Called every frame; the guard means the
            // config only dirties when the possessed figure actually changes.
            void set_pawn_tilt_tau(float tau) {
                if (config_.pawn_tilt_tau != tau) {
                    config_.pawn_tilt_tau = tau;
                    configDirty_ = true;
                }
            }
            // Possessed body's boundary inset (HEM_0) — its own radius, not a
            // collision radius. Same cadence and same guard as the tilt above.
            void set_pawn_body_radius(float r) {
                if (config_.pawn_body_radius != r) {
                    config_.pawn_body_radius = r;
                    configDirty_ = true;
                }
            }
            void set_world_bounds(float min_x, float min_z, float max_x, float max_z) {
                if (config_.world_bound_min[0] != min_x || config_.world_bound_min[1] != min_z ||
                    config_.world_bound_max[0] != max_x || config_.world_bound_max[1] != max_z) {
                    config_.world_bound_min[0] = min_x; config_.world_bound_min[1] = min_z;
                    config_.world_bound_max[0] = max_x; config_.world_bound_max[1] = max_z;
                    configDirty_ = true;
                }
            }
            void set_terrain_amp_ceiling(float ceiling) {
                if (config_.terrain_amp_ceiling != ceiling) {
                    config_.terrain_amp_ceiling = ceiling;
                    configDirty_ = true;
                }
            }
            void set_ceiling_height(float h) {
                if (config_.ceiling_height != h) {
                    config_.ceiling_height = h;
                    configDirty_ = true;
                }
            }
            void set_indoor_height_cap(float cap) {
                if (config_.indoor_height_cap != cap) {
                    config_.indoor_height_cap = cap;
                    configDirty_ = true;
                }
            }
            void set_terrain_time(float t) {
                if (config_.terrain_time != t) {
                    config_.terrain_time = t;
                    configDirty_ = true;
                }
            }
            void set_band_motion(const float blend[6], const float phase_origin[6]) {
                config_.band_blend_0 = blend[0];
                config_.band_blend_1 = blend[1];
                config_.band_blend_2 = blend[2];
                config_.band_blend_3 = blend[3];
                config_.band_blend_4 = blend[4];
                config_.band_blend_5 = blend[5];
                config_.band_phase_origin_0 = phase_origin[0];
                config_.band_phase_origin_1 = phase_origin[1];
                config_.band_phase_origin_2 = phase_origin[2];
                config_.band_phase_origin_3 = phase_origin[3];
                config_.band_phase_origin_4 = phase_origin[4];
                config_.band_phase_origin_5 = phase_origin[5];
                configDirty_ = true;
            }
            void set_fade(float alpha, float r, float g, float b) {
                if (config_.fade_alpha != alpha ||
                    config_.fade_color[0] != r || config_.fade_color[1] != g || config_.fade_color[2] != b) {
                    config_.fade_alpha = alpha;
                    config_.fade_color[0] = r; config_.fade_color[1] = g; config_.fade_color[2] = b;
                    configDirty_ = true;
                }
            }

            // ─── Musical animation mode parameters ───────────────────────
            void set_mode_color_shift(float v) {
                if (config_.mode_color_shift != v) { config_.mode_color_shift = v; configDirty_ = true; }
            }
            void set_mode_checker_scatter(float v) {
                if (config_.mode_checker_scatter != v) { config_.mode_checker_scatter = v; configDirty_ = true; }
            }

            void set_mode_palette_drift(float target, float intensity, float discrete_tier) {
                if (config_.mode_palette_target != target || config_.mode_palette_intensity != intensity
                    || config_.mode_discrete_tier != discrete_tier) {
                    config_.mode_palette_target = target;
                    config_.mode_palette_intensity = intensity;
                    config_.mode_discrete_tier = discrete_tier;
                    configDirty_ = true;
                }
            }
            // CHECKER-REBUILD: the pc-color field — one call carries the
            // fan (resultant rgb + music amount + music variance travel on
            // one span). Enveloping lives in the coupling decode, never here.
            void set_checker_color_field(const float resultant[3], float amount, float variance) {
                if (config_.checker_resultant[0] != resultant[0]
                    || config_.checker_resultant[1] != resultant[1]
                    || config_.checker_resultant[2] != resultant[2]
                    || config_.checker_music_amount != amount
                    || config_.checker_music_variance != variance) {
                    config_.checker_resultant[0] = resultant[0];
                    config_.checker_resultant[1] = resultant[1];
                    config_.checker_resultant[2] = resultant[2];
                    config_.checker_music_amount = amount;
                    config_.checker_music_variance = variance;
                    configDirty_ = true;
                }
            }
            void set_mode_gol_scales(float tick_scale, float height_scale) {
                if (config_.mode_gol_tick_scale != tick_scale || config_.mode_gol_height_scale != height_scale) {
                    config_.mode_gol_tick_scale = tick_scale;
                    config_.mode_gol_height_scale = height_scale;
                    configDirty_ = true;
                }
            }
            void set_pulse_data(uint32_t count, const float data[32]) {
                config_.pulse_count = count;
                std::memcpy(config_.pulse_data, data, 32 * sizeof(float));
                configDirty_ = true;
            }

            void set_config_dynamic(bool d) { configDynamic_ = d; }
            void mark_config_dirty() { configDirty_ = true; }

            // --- Config field setters (dirty-flagged) ---
            void set_pawn_aura_height(float h) {
                if (config_.pawn_aura_height != h) { config_.pawn_aura_height = h; configDirty_ = true; }
            }

            // --- Config ---
            GPUDesignConfig& config() { return config_; }

            // ── Staged config writes (the poke idiom). Deliberately
            // NO configDirty_: these fields ride targeted sub-range
            // uploads (upload_placement_patch_count / upload_lod_point)
            // — exactly the raw config() pokes they replace, identical
            // in upload behavior.
            void stage_placement_patch_count(uint32_t n) { config_.placement_patch_count = n; }
            void stage_lod_point(float x, float z)       { config_.lod_point_x = x; config_.lod_point_z = z; }

            // ── THE VEIL (re-ruled): RING = draw authority, fog = icing.
            // Dirty-gated config setters (ride the U8 drain); getters feed
            // the CPU band + entity cull so every draw gate and the GPU
            // share ONE live value.
            void set_veil_strength(float s) {
                if (config_.veil_strength != s) { config_.veil_strength = s; configDirty_ = true; }
            }
            void set_veil_dither(float d) {   // THE RIM knob: 0 tint / >0.5 dither-dissolve
                if (config_.veil_dither != d) { config_.veil_dither = d; configDirty_ = true; }
            }
            float veil_ring()   const { return config_.veil_ring; }
            float lod0_radius() const { return config_.lod0_radius; }
            const GPUDesignConfig& config() const { return config_; }

            uint32_t get_fpv_mode() const { return config_.fpv_mode; }

            // --- Compute pass ---
            // Live-contributor textures (Group 1) for compute pipelines that
            // evaluate query_ground_flyer / query_ground_walker.

            // --- Render pass ---

            // --- Shadow pass ---
            wgpu::TextureView shadow_map_view() const { return shadowMapView_; }
            wgpu::TextureView spot_shadow_map_view() const { return spotShadowMapView_; }

            // --- Mesh buffers ---
            wgpu::Buffer patch_index_buffer() const { return patchIndexBuffer_; }
            uint32_t patch_index_count() const { return patchIndexCount_; }
            wgpu::Buffer patch_index_buffer_cap_only() const { return patchIndexBufferCapOnly_; }
            uint32_t patch_index_count_cap_only() const { return patchIndexCountCapOnly_; }
            // ECONOMY_1 E1 — flag + the flag-selected LOD0 pair: buffer and
            // count move together so they can never split.
            //
            // ONE CARRIER remains, the snapshot pass (bodies/gallery.hpp).
            // The other three the list used to name are gone: the draw plan
            // retired the global-flag selection for the main pass and the
            // indirect reset (reset_frustum_indirect writes patchIndexCount_
            // and patchIndexCountCapOnly_ as separate plan slots, not through
            // this pair), and the shadow pass draws BOTH bands through
            // patch_index_buffer_lod1(), so "shadow band 0" never read these
            // at all. render_main_pass already says as much; this room had
            // not caught up.
            void set_curtains_active(bool a) { curtainsActive_ = a; }
            bool curtains_active() const { return curtainsActive_; }
            wgpu::Buffer patch_index_buffer_lod0_live() const {
                return curtainsActive_ ? patchIndexBuffer_ : patchIndexBufferCapOnly_;
            }
            uint32_t patch_index_count_lod0_live() const {
                return curtainsActive_ ? patchIndexCount_ : patchIndexCountCapOnly_;
            }
            wgpu::Buffer patch_index_buffer_lod1() const { return patchIndexBufferLOD1_; }
            uint32_t patch_index_count_ring_clean() const { return patchIndexCountRingClean_; }
            uint32_t patch_index_count_ring_zoned() const { return patchIndexCountRingZoned_; }
            // OPT_1e — flag + the flag-selected LOD1 count (ONE buffer;
            // the clean count is a prefix of the zoned IB, so only the
            // count moves and the pair can never split). Staged once per
            // frame in R17, read by slot C's indirect reset and the sun's
            // two LOD1 draws (R18 follows R17 in the spine).
            void set_zones_active_anywhere(bool a) { zonesActiveAnywhere_ = a; }
            uint32_t patch_index_count_lod1_live() const {
                return zonesActiveAnywhere_ ? patchIndexCountRingZoned_ : patchIndexCountRingClean_;
            }

            // --- GPU frustum culling ---
            wgpu::Buffer frustum_indirect_lod0() const { return frustumIndirectLOD0_; }
            wgpu::Buffer frustum_compute_buffer() const { return frustumComputeBuffer_; }
            wgpu::Buffer frustum_count_readback() const { return frustumCountReadback_; }
            static constexpr size_t frustum_indirect_size() { return 15 * sizeof(uint32_t); }
            wgpu::Buffer visible_patch_indices_buffer() const { return visiblePatchIndicesBuffer_; }

            // ═══ THE DRAW LEDGER (BUNDLE_1) ══════════════════════════════
            //
            // ONE RECORD PER DRAWABLE WHOSE COUNT THE CPU AUTHORS, and every
            // draw of it is an indirect draw from that record. The CPU writes
            // a record only when its number moves (compare-before-write, the
            // OIL_1 idiom). The terrain's own records stay the cull's
            // (frustumComputeBuffer_ — GPU-authored); this is the
            // conductor's half.
            //
            // A RECORD IS PER DRAWABLE, NOT PER (PASS, DRAWABLE). Every
            // shadow verb takes the same count as its main twin and passes
            // it through the same way — draw_shadow_arch and draw_arch are
            // one number — so both passes read one record. Two records for
            // one number would be two homes for one fact.
            //
            // WHY THIS EXISTS: an indirect draw's count lives on the GPU, so
            // the draw can be RECORDED ONCE into a render bundle and replayed
            // every frame while its number keeps moving. Without it a bundle
            // would freeze the counts it was recorded with.
            //
            // THE GUARDS DISSOLVE INTO THE RECORDS. Every `if (count == 0)
            // return;` and every `if (!active) return;` that used to sit in
            // a draw verb was an ENCODER-TIME skip — exactly what a bundle
            // cannot do, because a bundle recorded in a frame where a family
            // was empty would omit that family forever, and every family is
            // empty at boot. A record of zeros draws nothing, so the guard
            // becomes the number. stage_draw_ledger (render_passes.hpp) is
            // where every one of them now lives, in one readable place.
            static constexpr uint64_t draw_record_offset(DrawRecord r) {
                return static_cast<uint64_t>(r) * DRAW_RECORD_STRIDE;
            }
            wgpu::Buffer draw_ledger_buffer() const { return drawLedgerBuffer_; }

            void stage_draw_indexed(DrawRecord r, uint32_t indexCount, uint32_t instanceCount) {
                drawLedgerStage_[r] = DrawArgs{ { indexCount, instanceCount, 0u, 0u, 0u } };
            }
            void stage_draw_verts(DrawRecord r, uint32_t vertexCount, uint32_t instanceCount) {
                drawLedgerStage_[r] = DrawArgs{ { vertexCount, instanceCount, 0u, 0u, 0u } };
            }

            // FIRST-INSTANCE IS ALWAYS ZERO in this ledger, and that is a
            // constraint, not a choice: core WebGPU forbids a non-zero
            // firstInstance in an INDIRECT draw without the
            // `indirect-first-instance` feature, which the wallet does not
            // request. The one draw that needs one — the monolith's
            // Dim::CUBE_SLOT_OFFSET — is run-constant and stays a literal
            // direct draw for exactly this reason.
            static_assert(sizeof(DrawArgs) == 5 * sizeof(uint32_t),
                "a draw record is the five u32 the indirect draw reads");

            // The frame boundary's write: at most DR_COUNT small WriteBuffers,
            // and in a steady frame zero. A writeBuffer issued before the
            // submit lands ahead of the command buffer on the queue timeline
            // — the same ordering upload_patch_params relies on.
            void flush_draw_ledger(wgpu::Queue& queue) {
                for (uint32_t r = 0; r < DR_COUNT; r++) {
                    if (std::memcmp(&drawLedgerStage_[r], &drawLedgerShipped_[r],
                                    sizeof(DrawArgs)) == 0) continue;
                    queue.WriteBuffer(drawLedgerBuffer_,
                        draw_record_offset(static_cast<DrawRecord>(r)),
                        &drawLedgerStage_[r], sizeof(DrawArgs));
                    drawLedgerShipped_[r] = drawLedgerStage_[r];
                }
            }

            void reset_frustum_indirect(wgpu::Queue& queue) {
                // THE DRAW PLAN: three 5-u32 arg slots — A full IB, B
                // cap-only IB, C LOD1 IB. A and B's indexCounts are
                // constants of their buffers; C's is the LIVE LOD1 count
                // (OPT_1e — clean prefix at true rest, zoned otherwise);
                // instanceCounts are the kernel's three atomics
                // (indices 1 / 6 / 11).
                uint32_t args[15] = {
                    patchIndexCount_,        0, 0, 0, 0,
                    patchIndexCountCapOnly_, 0, 0, 0, 0,
                    patch_index_count_lod1_live(), 0, 0, 0, 0,
                };
                queue.WriteBuffer(frustumComputeBuffer_, 0, args, sizeof(args));
            }
            void upload_draw_plan(wgpu::Queue& queue, const GPUDrawPlanParams& p) {
                // OIL_1 U7: skip on equality — the caller packs the plan
                // fresh every frame (that recompute IS the change
                // detector); the write fires only when the bytes moved.
                // memcmp is total: the struct is padding-free (assert
                // below) and the pack site zero-inits _pad0.
                static_assert(sizeof(GPUDrawPlanParams) ==
                    4 * sizeof(uint32_t) + sizeof(float) * 8 * 4,
                    "draw plan must be padding-free for the memcmp gate");
                if (drawPlanEverUploaded_ &&
                    std::memcmp(&lastDrawPlan_, &p, sizeof(p)) == 0) return;
                lastDrawPlan_ = p;
                drawPlanEverUploaded_ = true;
                queue.WriteBuffer(drawPlanBuffer_, 0, &p, sizeof(p));
            }

            static constexpr uint32_t pawn_vertex_count() { return Dim::PAWN_VERTEX_COUNT; }
            wgpu::Buffer sphere_vertex_buffer() const { return sphereVertexBuffer_; }
            wgpu::Buffer sphere_index_buffer() const { return sphereIndexBuffer_; }
            uint32_t sphere_index_count() const { return sphereIndexCount_; }
            wgpu::Buffer monolith_vertex_buffer() const { return monolithVertexBuffer_; }
            wgpu::Buffer monolith_index_buffer() const { return monolithIndexBuffer_; }
            uint32_t monolith_index_count() const { return monolithIndexCount_; }
            wgpu::Buffer arch_vertex_buffer() const { return archVertexBuffer_; }
            wgpu::Buffer arch_index_buffer() const { return archIndexBuffer_; }
            wgpu::Buffer arch_ground_buffer() const { return archGroundBuffer_; }
            uint32_t arch_index_count() const { return archIndexCount_; }
            void set_arch_index_count(uint32_t count) { archIndexCount_ = count; }

            void upload_arch_origins(wgpu::Queue& queue, const GPUArchGroundEntry* entries, uint32_t count) {
                writeArray(queue, archGroundBuffer_, entries, std::min(count, Dim::MAX_ARCH_INSTANCES));
            }
            wgpu::Buffer column_vertex_buffer() const { return columnVertexBuffer_; }
            wgpu::Buffer column_index_buffer() const { return columnIndexBuffer_; }
            wgpu::Buffer column_ground_buffer() const { return columnGroundBuffer_; }
            uint32_t column_index_count() const { return columnIndexCount_; }
            void set_column_index_count(uint32_t count) { columnIndexCount_ = count; }

            // GPU mesh gen: write params for a single column slot (80 bytes per spawn/evict)
            void upload_column_mesh_params_slot(wgpu::Queue& queue, uint32_t slot, const GPUColumnMeshParams& params) {
                writeSlot(queue, columnMeshParamsBuffer_, slot, params);
                // CHORD_1 — the occupier window on the same row (see
                // upload_arch_mesh_params_slot for the ruling).
                agentRoomStage_.occupier_cmg[slot] = params;
                writeSlot(queue, agentRoomBuffer_, slot, params,
                    offsetof(GPUAgentRoomConstants, occupier_cmg));
            }

            // Column GPU mesh gen bind group

            void upload_column_origins(wgpu::Queue& queue, const GPUColumnGroundEntry* entries, uint32_t count) {
                writeArray(queue, columnGroundBuffer_, entries, std::min(count, Dim::MAX_COLUMN_INSTANCES));
            }

            // --- Palm accessors and upload ---
            wgpu::Buffer palm_vertex_buffer() const { return palmVertexBuffer_; }
            wgpu::Buffer palm_index_buffer() const { return palmIndexBuffer_; }
            uint32_t palm_index_count() const { return palmIndexCount_; }
            void set_palm_index_count(uint32_t count) { palmIndexCount_ = count; }

            void upload_palm_mesh_params_slot(wgpu::Queue& queue, uint32_t slot, const GPUPalmMeshParams& params) {
                writeSlot(queue, palmMeshParamsBuffer_, slot, params);
            }


            // --- Cactus accessors and upload ---
            wgpu::Buffer cactus_vertex_buffer() const { return cactusVertexBuffer_; }
            wgpu::Buffer cactus_index_buffer() const { return cactusIndexBuffer_; }
            uint32_t cactus_index_count() const { return cactusIndexCount_; }
            void set_cactus_index_count(uint32_t count) { cactusIndexCount_ = count; }
            void upload_cactus_mesh_params_slot(wgpu::Queue& queue, uint32_t slot, const GPUCactusMeshParams& params) {
                writeSlot(queue, cactusMeshParamsBuffer_, slot, params);
            }

            // --- Blade Cluster accessors and upload ---
            wgpu::Buffer blade_vertex_buffer() const { return bladeVertexBuffer_; }
            wgpu::Buffer blade_index_buffer() const { return bladeIndexBuffer_; }
            wgpu::Buffer plant_compute_ground_buffer() const { return plantComputeGroundBuffer_; }
            uint32_t blade_index_count() const { return bladeIndexCount_; }
            void set_blade_index_count(uint32_t count) { bladeIndexCount_ = count; }
            void upload_blade_mesh_params_slot(wgpu::Queue& queue, uint32_t slot,
                const GPUBladeClusterMeshParams& params) {
                writeSlot(queue, bladeMeshParamsBuffer_, slot, params);
            }

            // --- Pyramid accessors and upload --- (the instance array only:
            //   pyramids ARE terrain, so there is no mesh to generate.)
            void upload_pyramids(wgpu::Queue& queue, const GPUPyramidArray& arr) {
                writeStruct(queue, pyramidInstancesBuffer_, arr);
            }

            // Indoor shell accessors
            wgpu::Buffer shell_vertex_buffer() const { return shellVertexBuffer_; }
            wgpu::Buffer shell_index_buffer() const { return shellIndexBuffer_; }
            uint32_t shell_index_count() const { return shellIndexCount_; }
            void set_shell_index_count(uint32_t count) { shellIndexCount_ = count; }

            void upload_shell_mesh(wgpu::Queue& queue,
                const ShellVertex* verts, uint32_t vertCount,
                const uint32_t* indices, uint32_t idxCount) {
                writeArray(queue, shellVertexBuffer_, verts, std::min(vertCount, Dim::SHELL_MAX_VERTICES));
                writeArray(queue, shellIndexBuffer_, indices, std::min(idxCount, Dim::SHELL_MAX_INDICES));
                shellIndexCount_ = idxCount;
            }

            static constexpr uint32_t ribbon_vertex_count() { return Dim::RIBBON_VERTEX_COUNT; }
            wgpu::Buffer ribbon_buffer() const { return ribbonBuffer_; }
            wgpu::Buffer agent_state_buffer() const { return agentStateBuffer_; }
            wgpu::Buffer agent_state_readback_staging() const { return agentStateReadbackStaging_; }
            wgpu::Buffer floating_entity_readback_staging() const { return floatingEntityReadbackStaging_; }
            wgpu::Buffer camera_buffer() const { return cameraBuffer_; }                              // ATRIUM_11
            wgpu::Buffer camera_readback_staging() const { return cameraReadbackStaging_; }           // ATRIUM_11
            wgpu::Buffer floating_entity_buffer() const { return floatingEntityBuffer_; }
            static constexpr size_t floating_entity_buffer_size() {
                return Dim::TOTAL_FLOATING_SLOTS * sizeof(GPUFloatingEntityState);
            }
            static constexpr size_t agent_state_buffer_size() { return Dim::MAX_AGENTS * sizeof(GPUAgentState); }
            static constexpr size_t camera_state_buffer_size() { return sizeof(GPUCameraState); }   // ATRIUM_11
            static constexpr size_t agent_slot_size() { return sizeof(GPUAgentState); }
            // The point readback (option A): camera-host
            // only — the spine copies the camera state to staging and
            // harvests pos.xz as THE POINT's position. Pawn-host never
            // touches these (that path stays the agent readback).
            void set_possessed_slot(uint32_t slot) {
                if (config_.possessed_slot != slot) {
                    config_.possessed_slot = slot;
                    configDirty_ = true;
                }
            }
            wgpu::Buffer ring_transforms_buffer() const { return ringTransformsBuffer_; }

            // ─── THE FRAME METER — GPU half (arming + resolve plumbing) ───
            // arm fills the next writes struct { querySet, i, i+1 }, records
            // the { row, i } pair, advances by 2. Returns nullptr when the
            // meter is off or indices are exhausted — later passes simply go
            // unmetered that frame, never a fault. Two spellings, one
            // allocator: compute + render descriptors may carry distinct
            // writes types (older Dawn) or the same merged one (newer).
            // Index reuse across frames is safe: queue order puts last
            // frame's resolve before this frame's writes.
            bool meter_gpu_supported() const { return meterEnabled_; }
            void meter_frame_begin() { meterPairCount_ = 0; meterNextIndex_ = 0; }
            uint32_t meter_arm_alloc(uint32_t row) {   // shared allocator: begin index or UINT32_MAX
                // THE DIAL first: a compile-time refusal, so every one of the
                // twenty arm sites folds to `descriptor.timestampWrites =
                // nullptr` and the driver never sees a pass-boundary write.
                if constexpr (!INSTRUMENTS.frame_meter) return UINT32_MAX;
                if (!meterEnabled_ || meterNextIndex_ + 2 > METER_QUERY_COUNT) return UINT32_MAX;
                const uint32_t i = meterNextIndex_;
                meterPairs_[meterPairCount_] = { row, i };
                meterPairCount_++;
                meterNextIndex_ += 2;
                return i;
            }
            const MeterComputeTsW* meter_arm_compute(uint32_t row) {
                const uint32_t i = meter_arm_alloc(row);
                if (i == UINT32_MAX) return nullptr;
                auto& w = meterComputeWrites_[i / 2];
                w.querySet = meterQuerySet_;
                w.beginningOfPassWriteIndex = i;
                w.endOfPassWriteIndex = i + 1;
                return &w;
            }
            const MeterRenderTsW* meter_arm_render(uint32_t row) {
                const uint32_t i = meter_arm_alloc(row);
                if (i == UINT32_MAX) return nullptr;
                auto& w = meterRenderWrites_[i / 2];
                w.querySet = meterQuerySet_;
                w.beginningOfPassWriteIndex = i;
                w.endOfPassWriteIndex = i + 1;
                return &w;
            }
            uint32_t meter_pair_count() const { return meterPairCount_; }
            const MeterPair* meter_pairs() const { return meterPairs_; }
            wgpu::QuerySet meter_query_set() const { return meterQuerySet_; }
            wgpu::Buffer meter_resolve_buffer() const { return meterResolveBuffer_; }
            wgpu::Buffer meter_readback_staging() const { return meterReadbackStaging_; }
            static constexpr uint32_t meter_max_pairs() { return METER_MAX_PAIRS; }
            static constexpr size_t meter_readback_size() { return METER_QUERY_COUNT * sizeof(uint64_t); }

            // --- Gallery system ---
            wgpu::TextureView offscreen_color_view() const { return offscreenColorView_; }
            wgpu::TextureView offscreen_depth_view() const { return offscreenDepthView_; }
            // B10: null when msaa=1 — the snapshot pass reads the null as
            // "render straight into the offscreen color, exactly as before".
            wgpu::TextureView offscreen_msaa_color_view() const { return offscreenMsaaColorView_; }
            wgpu::Texture offscreen_color_texture() const { return offscreenColorTexture_; }

            // Three-array painting system accessors
            wgpu::Texture snapshot_staging_texture() const { return snapshotStagingTexture_; }
            wgpu::Texture authored_staging_texture() const { return authoredStagingTexture_; }
            wgpu::Texture exhibition_texture() const { return exhibitionTexture_; }

            // Promote a staging layer to an exhibition layer (GPU copy, call within encoder scope)
            // FULL-LAYER OVERWRITE IS AN INVARIANT OF LAYER REUSE, and this
            // copy is the site that guarantees it.
            //
            // Exhibition layers are recycled — find_free_exhibition_layer hands
            // back a layer whose previous occupant was never cleared, because
            // this copy has always covered every texel of it. That was true by
            // accident of one resolution rather than by design, and nothing said
            // so: SUPPLY made snapshots 512, the copy became a PARTIAL write,
            // and outdoor quads showed the new picture in one corner with the
            // last tenant's image around it.
            //
            // So: any scheme that writes less than a full layer must CLEAR the
            // layer first. Do not reintroduce a partial extent here without it.
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
            static constexpr uint32_t painting_quad_verts() { return Dim::PAINTING_QUAD_VERTS; }
            static constexpr uint32_t painting_max_slots() { return Dim::PAINTING_MAX_SLOTS; }
            static constexpr uint32_t painting_frame_vertex_count() { return Dim::PAINTING_FRAME_VERTEX_COUNT; }

            // --- GoL zone system ---

            // Pawn aura accessors
            void upload_pawn_aura_config(wgpu::Queue& queue, const GPUPawnAuraConfig& cfg) {
                writeStruct(queue, pawnAuraConfigBuffer_, cfg);
            }
            void upload_pawn_aura_frame(wgpu::Queue& queue, float dt, float t_beats) {
                queue.WriteBuffer(pawnAuraConfigBuffer_, offsetof(GPUPawnAuraConfig, dt), &dt, sizeof(float));
                queue.WriteBuffer(pawnAuraConfigBuffer_, offsetof(GPUPawnAuraConfig, t_beats), &t_beats, sizeof(float));
            }
            static constexpr uint32_t pawn_aura_workgroups() { return PAWN_AURA_N / 8; }

            // Live card accessors (GROUND_CARD_1)
            wgpu::TextureView live_card_view() const { return liveCardView_; }

            // Orb sky layer accessors
            wgpu::Buffer orb_state_buffer() const { return orbStateBuffer_; }
            wgpu::Buffer orb_config_buffer() const { return orbConfigBuffer_; }
            wgpu::Buffer orb_quad_vb() const { return orbQuadVB_; }
            wgpu::Buffer orb_quad_ib() const { return orbQuadIB_; }
            void upload_orb_config(wgpu::Queue& queue, const GPUOrbConfig& cfg) {
                writeStruct(queue, orbConfigBuffer_, cfg);
            }
            void upload_orb_frame(wgpu::Queue& queue, float dt, float t_seconds) {
                // OIL_1 U4 (ledger: R14, C8): t_seconds rides dt — adjacent
                // (36/40), one 8-byte write, the same packed grammar as the
                // cube aspect/glide writers. Byte-identical payload.
                static_assert(offsetof(GPUOrbConfig, t_seconds) == offsetof(GPUOrbConfig, dt) + 4,
                    "orb frame pair: t_seconds must ride dt for the coalesced write");
                float frame[2] = { dt, t_seconds };
                queue.WriteBuffer(orbConfigBuffer_, offsetof(GPUOrbConfig, dt), frame, sizeof(frame));
            }
            void upload_orb_force(wgpu::Queue& queue, float radial) {
                queue.WriteBuffer(orbConfigBuffer_,
                    offsetof(GPUOrbConfig, force_radial), &radial, sizeof(float));
            }
            // The PANEL's writer, reached through the cartridge's
            // console-mask block. The kernel reads noise_amp every frame, so
            // a targeted 4-byte partial is the whole cost of a dial that
            // moves under the finger.
            void upload_orb_noise(wgpu::Queue& queue, float noise) {
                queue.WriteBuffer(orbConfigBuffer_,
                    offsetof(GPUOrbConfig, noise_amp), &noise, sizeof(float));
            }
            // Its sibling, minted for the same caller. The dome radius is
            // read per frame by the dynamics kernel (the shell re-projection)
            // and by the init kernel; the partial moves the live sky without
            // re-seeding it, which the full configure_orbs path would.
            void upload_orb_dome_radius(wgpu::Queue& queue, float radius) {
                queue.WriteBuffer(orbConfigBuffer_,
                    offsetof(GPUOrbConfig, dome_radius), &radius, sizeof(float));
            }
            void upload_orb_flock_intensity(wgpu::Queue& queue, float intensity) {
                queue.WriteBuffer(orbConfigBuffer_,
                    offsetof(GPUOrbConfig, flock_coupling_intensity),
                    &intensity, sizeof(float));
            }
            // Pass 10: runtime motion rule switch (no orb-state reset).
            void upload_orb_motion_rule(wgpu::Queue& queue, uint32_t rule) {
                queue.WriteBuffer(orbConfigBuffer_,
                    offsetof(GPUOrbConfig, motion_rule),
                    &rule, sizeof(uint32_t));
            }
            void upload_orb_flock_signs(wgpu::Queue& queue,
                float sep, float align, float coh) {
                struct { float s, a, c; } packed = { sep, align, coh };
                queue.WriteBuffer(orbConfigBuffer_,
                    offsetof(GPUOrbConfig, flock_sep_sign),
                    &packed, sizeof(packed));
            }
            // Brownian gesture bundle — three contiguous floats
            // at offset 180 (radial_sign, vert_bias, coherence).
            void upload_orb_brownian_gesture(wgpu::Queue& queue,
                float radial_sign,
                float vert_bias,
                float coherence) {
                struct { float r, v, c; } packed = { radial_sign, vert_bias, coherence };
                queue.WriteBuffer(orbConfigBuffer_,
                    offsetof(GPUOrbConfig, brownian_radial_sign),
                    &packed, sizeof(packed));
            }
            void upload_orb_orbital_gesture(wgpu::Queue& queue,
                float alignment_mode,
                float speed_var_mult) {
                queue.WriteBuffer(orbConfigBuffer_,
                    offsetof(GPUOrbConfig, orbital_alignment_mode),
                    &alignment_mode, sizeof(float));
                queue.WriteBuffer(orbConfigBuffer_,
                    offsetof(GPUOrbConfig, orbital_speed_var_mult),
                    &speed_var_mult, sizeof(float));
            }
            // Population speed multiplier (1.0 = identity). Fires
            // only when the attractor smoother moves — quiet at rest.
            void upload_orb_speed_mult(wgpu::Queue& queue, float mult) {
                queue.WriteBuffer(orbConfigBuffer_,
                    offsetof(GPUOrbConfig, speed_mult),
                    &mult, sizeof(float));
            }
            // Per-frame color dynamics: pulse / converge / surge intensities.
            // hue_converge_target lives at offset 156 and changes only on mood
            // entry, so it's written via the full upload_orb_config path.
            void upload_orb_color_dynamics(wgpu::Queue& queue,
                float pulse, float converge, float surge) {
                struct { float pulse, converge, surge; } packed = { pulse, converge, surge };
                queue.WriteBuffer(orbConfigBuffer_,
                    offsetof(GPUOrbConfig, color_pulse),
                    &packed, sizeof(packed));
            }
            // Partial upload of the palette slice (palette_count..pal3_weight).
            // 72 bytes contiguous from offset 72. Preserves per-frame fields
            // (dt, t_seconds, noise_amp, force_radial) elsewhere in the struct.
            void upload_orb_palette(wgpu::Queue& queue,
                uint32_t palette_count,
                float value_variance,
                const float palette_data[16]) {
                struct { uint32_t count; float var; float pal[16]; } packed;
                packed.count = palette_count;
                packed.var = value_variance;
                for (int i = 0; i < 16; i++) packed.pal[i] = palette_data[i];
                static_assert(sizeof(packed) == 72, "orb palette slice must be 72 bytes");
                queue.WriteBuffer(orbConfigBuffer_,
                    offsetof(GPUOrbConfig, palette_count),
                    &packed, sizeof(packed));
            }

            void upload_zone_config(wgpu::Queue& queue, const GPUGoLZoneArray& config) {
                writeStruct(queue, zoneConfigBuffer_, config);
            }

            // Header-only upload: count, t_beats, dt, tick_mask.
            // Does NOT overwrite per-zone configs (GPU-derived via zone_derive_params).
            void upload_zone_config_header(wgpu::Queue& queue, uint32_t count,
                float t_beats, float dt, uint32_t tick_mask) {
                struct { uint32_t count; float t_beats; float dt; uint32_t tick_mask; } header;
                header.count = count;
                header.t_beats = t_beats;
                header.dt = dt;
                header.tick_mask = tick_mask;
                queue.WriteBuffer(zoneConfigBuffer_, 0, &header, 16);
            }

            // Deactivate a single zone slot by zeroing its transition_fraction.
            // Safe to call on slots whose config was GPU-derived.
            void deactivate_zone_slot(wgpu::Queue& queue, uint32_t slot) {
                float zero = 0.0f;
                size_t offset = 16 + slot * sizeof(GPUGoLZoneConfig)
                    + offsetof(GPUGoLZoneConfig, transition_fraction);
                queue.WriteBuffer(zoneConfigBuffer_, offset, &zero, sizeof(float));
            }

            void upload_zone_derive_requests(wgpu::Queue& queue, const GPUZoneDeriveRequestArray& requests) {
                writeStruct(queue, zoneDeriveRequestBuffer_, requests);
            }

            void upload_portal_array(wgpu::Queue& queue, const GPUPortalArray& arr) {
                agentRoomStage_.portals = arr;
                queue.WriteBuffer(agentRoomBuffer_,
                    offsetof(GPUAgentRoomConstants, portals),
                    &agentRoomStage_.portals, sizeof(agentRoomStage_.portals));
            }

            void upload_zone_life(wgpu::Queue& queue, uint32_t slot,
                const float* life_data, const float* height_factors,
                uint32_t cell_count) {
                size_t base = slot * Dim::GOL_ZONE_LIFE_STRIDE * sizeof(float);
                size_t cells_bytes = cell_count * sizeof(float);
                size_t slot_stride = Dim::GOL_ZONE_CELLS * sizeof(float);
                std::vector<float> zeros(cell_count, 0.0f);
                // Slot 0: visual (initial = target)
                queue.WriteBuffer(zoneLifeBuffer_, base + slot_stride * 0, life_data, cells_bytes);
                // Slot 1: velocity (zero)
                queue.WriteBuffer(zoneLifeBuffer_, base + slot_stride * 1, zeros.data(), cells_bytes);
                // Slot 2: target (initial alive/dead)
                queue.WriteBuffer(zoneLifeBuffer_, base + slot_stride * 2, life_data, cells_bytes);
                // Slot 3: next_target (same as target)
                queue.WriteBuffer(zoneLifeBuffer_, base + slot_stride * 3, life_data, cells_bytes);
                // Slot 4: per-cell height factor (persistent)
                queue.WriteBuffer(zoneLifeBuffer_, base + slot_stride * 4, height_factors, cells_bytes);
            }

            // --- Dispatch dimensions ---
            // Rounds UP: 65 is not a multiple of the 16x16 tile, so the
            // kernel guards its own upper bound and the edge tile idles.
            static constexpr uint32_t patch_heightfield_workgroups() { return (Dim::PATCH_HEIGHTFIELD_N + 15) / 16; }
            static constexpr uint32_t patch_cell_workgroups() { return Dim::PATCH_CELL_N / 8; }
            static constexpr uint32_t ribbon_ring_workgroups() { return (Dim::RIBBON_MAX_RINGS + 63) / 64; }
            // PANORAMA_0 RIDE_0 — the two kernels that stopped being 0D/32-wide.
            // Both round UP: the kernels guard their own upper bound, so a
            // count that is not a multiple of 64 costs idle lanes, never a
            // dropped slot.
            static constexpr uint32_t cube_workgroups() { return (Dim::MAX_CUBE_INSTANCES + 63) / 64; }
            static constexpr uint32_t field_lane_workgroups() { return (Dim::FIELD_SUBSCRIBER_CAP + 63) / 64; }

            // --- Batch patch generation ---
            // PROBATE_I: the three staging doors — patch_params_buffer,
            // patch_staging_buffer and upload_patch_staging — are retired
            // with the two buffers behind them. A batch's params are CPU
            // values now, set per patch on the pass encoder.

        private:

            // ═══ PORT_3b — THE GPU BUDGET ════════════════════════════
            //
            // What we ASK the GPU for, summed where the asking happens.
            // Not a bookkeeping system: two counters, a five-slot
            // leaderboard, and one call inside each of the two makers
            // every allocation already routes through. Adding a resource
            // without its bytes landing here now requires bypassing the
            // maker, which is the thing to notice anyway.
            //
            // IT IS AN ESTIMATE, and the report says so: logical texel
            // bytes, uncompressed, with no driver padding, no alignment
            // rounding, and no view/descriptor overhead. Real footprint
            // is somewhat higher. The number's job is to make a class of
            // decision (does this fit a phone?) possible at all — it was
            // previously unmeasured, and an approximate measurement beats
            // an exact guess.
            uint64_t gpuBufferBytes_  = 0;
            uint64_t gpuTextureBytes_ = 0;
            struct AllocNote { const char* label = nullptr; uint64_t bytes = 0; };
            static constexpr uint32_t GPU_TOP_N = 5;
            AllocNote gpuTop_[GPU_TOP_N]{};
            uint32_t  gpuUnknownFormats_ = 0;   // texels we could not size; reported, never hidden

            void noteAlloc(const char* label, uint64_t bytes, bool is_texture) {
                if (is_texture) gpuTextureBytes_ += bytes;
                else            gpuBufferBytes_  += bytes;
                // Insertion sort into the five-slot leaderboard.
                for (uint32_t i = 0; i < GPU_TOP_N; i++) {
                    if (bytes > gpuTop_[i].bytes) {
                        for (uint32_t j = GPU_TOP_N - 1; j > i; j--) gpuTop_[j] = gpuTop_[j - 1];
                        gpuTop_[i] = AllocNote{ label, bytes };
                        return;
                    }
                }
            }

            // Bytes per texel for the formats this cartridge actually
            // creates. Returns 0 for anything else — counted and reported
            // rather than silently undercounted.
            static uint64_t texel_bytes(wgpu::TextureFormat f) {
                switch (f) {
                    case wgpu::TextureFormat::RGBA16Float:  return 8;
                    case wgpu::TextureFormat::RGBA8Unorm:   return 4;
                    case wgpu::TextureFormat::BGRA8Unorm:   return 4;
                    case wgpu::TextureFormat::R32Float:     return 4;
                    // FORMAT_1 D1 — the shadow format's byte width answers to
                    // kShadowDepthFormat. Kept as an explicit case per format
                    // rather than a lookup so the switch stays exhaustive and
                    // the UNDERCOUNT path keeps its meaning.
                    case wgpu::TextureFormat::Depth32Float: return 4;
                    case wgpu::TextureFormat::Depth16Unorm: return 2;
                    case wgpu::TextureFormat::Depth24Plus:  return 4;
                    default:                                return 0;
                }
            }

            wgpu::Buffer makeBuffer(const char* label, uint64_t size, wgpu::BufferUsage usage) {
                wgpu::BufferDescriptor d{}; d.label = label; d.size = size; d.usage = usage;
                noteAlloc(label, size, /*is_texture=*/false);
                return device_.CreateBuffer(&d);
            }

            // The texture twin of makeBuffer, same shape: the label is a
            // parameter and this is its ONE home, so the report can never
            // name a texture differently from the descriptor.
            wgpu::Texture makeTexture(const char* label, wgpu::TextureDescriptor& desc) {
                desc.label = label;
                const uint64_t bpp = texel_bytes(desc.format);
                if (bpp == 0) gpuUnknownFormats_++;
                const uint32_t mips = desc.mipLevelCount ? desc.mipLevelCount : 1u;
                const uint32_t samples = desc.sampleCount ? desc.sampleCount : 1u;
                uint64_t bytes = 0;
                for (uint32_t m = 0; m < mips; m++) {
                    // Ternary, not std::max: this file includes no
                    // <algorithm> of its own, and std::max(1u, uint32_t)
                    // is a deduction hazard where uint32_t is not `unsigned
                    // int`. Mip clamp, spelled without a dependency.
                    const uint32_t mw = desc.size.width  >> m;
                    const uint32_t mh = desc.size.height >> m;
                    const uint64_t w = (mw > 0u) ? mw : 1u;
                    const uint64_t h = (mh > 0u) ? mh : 1u;
                    bytes += w * h * desc.size.depthOrArrayLayers * bpp;
                }
                noteAlloc(label, bytes * samples, /*is_texture=*/true);
                return device_.CreateTexture(&desc);
            }

            static void print_mib(const char* what, uint64_t bytes) {
                std::cout << "[GPU Budget] " << what << " "
                    << (static_cast<double>(bytes) / (1024.0 * 1024.0)) << " MiB\n";
            }

        public:
            // ═══ ORGAN — THE PANEL'S WRITE SURFACE ═══════════════════════
            // Exactly these three homes and nothing else: the sovereignty
            // boundary written as code. A home the panel may write has an
            // accessor here; a home it may not has none, so GPU truth —
            // positions, vp, camera, simulation state — is absent by
            // construction.

            // Raw pointers are deliberate: the registry addresses members by
            // offsetof from the block's base, and the ABI refuses any (block,
            // offset, type) triple the registry does not carry, so the
            // pointer is never a general-purpose door.
            GPUDesignConfig*       organ_config_home()     { return &config_; }
            GPULighting*           organ_lighting_home()   { return &lightingStage_; }
            GPUAgentRoomConstants* organ_agent_room_home() { return &agentRoomStage_; }

            // The panel's one door for "this home changed": it raises the
            // home's OWN dirty flag where there is one, and the witness bit
            // always. Which flag a home uses is the home's knowledge.
            void organ_mark_dirty(uint32_t block) {
                organTouched_ |= (1u << block);
                if (block == 0) configDirty_ = true;   // config_'s one dirty bit
            }
            uint32_t organ_last_flush_count() const { return organLastFlush_; }

            // The frame-boundary flush (docs/ORGAN.md, "The write path").
            // A slider drag is many events and one WriteBuffer: the events
            // only set bits, and this runs once a frame through the program's
            // own upload paths. Nothing here duplicates an upload.
            void organ_flush(wgpu::Queue& queue) {
                organLastFlush_ = 0;
                if (!organTouched_) return;
                if (organTouched_ & (1u << 0)) {        // DesignConfig
                    // NO UPLOAD HERE, on purpose: config_ is staged, and
                    // the spine sends it every frame off configDirty_, which
                    // organ_mark_dirty has already raised. The count still
                    // rises because the panel's edit WAS reconciled.
                    ++organLastFlush_;
                }
                if (organTouched_ & (1u << 1)) {        // Lighting
                    upload_lighting(queue, lightingStage_);
                    ++organLastFlush_;
                }
                if (organTouched_ & (1u << 2)) {        // the agents' room
                    // tier_gains is ONE home with TWO windows, agent_room's
                    // and scene_constants'; the authoring site writes every
                    // window it owns, so both go here.
                    queue.WriteBuffer(agentRoomBuffer_,
                        offsetof(GPUAgentRoomConstants, tier_gains),
                        agentRoomStage_.tier_gains,
                        sizeof(agentRoomStage_.tier_gains));
                    queue.WriteBuffer(sceneConstantsBuffer_,
                        offsetof(GPUSceneConstants, tier_gains),
                        agentRoomStage_.tier_gains,
                        sizeof(agentRoomStage_.tier_gains));
                    ++organLastFlush_;
                }
                if (organTouched_ & (1u << 3)) {        // the drivers' room
                    // DRIVERS (block 3) — no upload and no flag: the seams
                    // that read it each tick ARE its flush. The count still
                    // rises, the edit having been reconciled where it landed.
                    ++organLastFlush_;
                }
                // THE GRADUATED BANKS (blocks 4 and up). Every one is
                // CPU-read by its own module each tick, so block 3's rule
                // covers them all: no upload, no flag, and the count rises
                // once per block whose edit was reconciled. One branch,
                // because they share one reason.
                {
                    uint32_t banks = organTouched_ >> 4;
                    while (banks) { banks &= banks - 1u; ++organLastFlush_; }
                }
                organTouched_ = 0;
            }

            // PORT_4b — public: the correct call site is the END of the
            // cartridge's init_renderer, after initOffscreenResources and
            // after the authored paintings are staged. The budget's job is
            // to be complete, so it is called from wherever "complete" is.
            void report_gpu_budget() const {
                std::cout << "\n[GPU Budget] ---- allocation request, boot ----\n";
                print_mib("buffers ", gpuBufferBytes_);
                print_mib("textures", gpuTextureBytes_);
                print_mib("TOTAL   ", gpuBufferBytes_ + gpuTextureBytes_);
                std::cout << "[GPU Budget] largest single allocations:\n";
                for (uint32_t i = 0; i < GPU_TOP_N; i++) {
                    if (!gpuTop_[i].label) break;
                    std::cout << "[GPU Budget]   " << (i + 1) << ". "
                        << (static_cast<double>(gpuTop_[i].bytes) / (1024.0 * 1024.0))
                        << " MiB  " << gpuTop_[i].label << "\n";
                }
                if (gpuUnknownFormats_ > 0) {
                    std::cout << "[GPU Budget] WARNING: " << gpuUnknownFormats_
                        << " texture(s) of unsized format — total is an UNDERCOUNT\n";
                }
                std::cout << "[GPU Budget] estimate: logical texels, uncompressed, "
                    "no driver padding. Excludes the surface backbuffer and the "
                    "console depth texture (host-owned).\n\n";
            }

        private:   // PORT_4b — the makers and creators stay private
            bool createBuffers() {
                auto SU = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
                auto UU = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
                signalBuffer_ = makeBuffer("Frame Signal", sizeof(GPUFrameSignal), wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst);
                // (Storage dropped with binding 200 — the read-only-storage
                //  mirror was its only storage consumer. The buffer stays:
                //  it still backs the LIVE uniform `signal` at binding 0.)
                configBuffer_ = makeBuffer("Design Config", sizeof(GPUDesignConfig), UU);
                // Skew beacon: creation + every bind entry derive from sizeof(GPUDesignConfig) —
                // if Dawn reports "bound with size N ... requires M" for this buffer, the binary
                // is STALE against the hot-loaded world.wgsl mirror. Rebuild; do not edit sizes.
                std::cout << "[GPUState] Design Config: " << sizeof(GPUDesignConfig)
                    << " B (C++ side; WGSL DesignConfig mirror must match)\n";
                agentStateBuffer_ = makeBuffer("Agent State",
                    Dim::MAX_AGENTS * sizeof(GPUAgentState),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc);
                // CHORD_1: one 6960 B uniform block where five buffers stood.
                agentRoomBuffer_ = makeBuffer("Agents' Room Constants",
                    sizeof(GPUAgentRoomConstants), UU);
                // CHORD_4: one 4336 B uniform block where three buffers stood.
                // UNIFORM (not storage) for the reason the figure table carried
                // alone before the merge: the render VS storage cap is full.
                sceneConstantsBuffer_ = makeBuffer("Scene Constants",
                    sizeof(GPUSceneConstants), UU);
                cameraBuffer_ = makeBuffer("Camera State", sizeof(GPUCameraState),
                    SU | wgpu::BufferUsage::CopySrc);   // CopySrc: the point readback (camera-host) AND the CHORD_3 block copy
                    // CHORD_3: Uniform dropped. DOMESDAY_0 B2's g1:4 window
                    // is gone; the camera reaches the render stages as
                    // frame_r.camera, copied from here on the frame encoder.
                    // The compute face (g2:241 camera_state) still binds as
                    // storage.
                floatingEntityBuffer_ = makeBuffer("Floating Entity Array",
                    Dim::TOTAL_FLOATING_SLOTS * sizeof(GPUFloatingEntityState),
                    SU | wgpu::BufferUsage::CopySrc);
                    // CHORD_5: Uniform dropped. g2:6 render_floating was this
                    // buffer's only uniform binding, and it is read-only
                    // storage again — the 54,912 B block was at 83.8% of the
                    // uniform binding ceiling, which is a wall on entity
                    // growth rather than a saving. Same buffer, same bytes,
                    // no repack (behaviour preservation, L3).
                // LATENT[gate-a-shared] ribbon (SH·mb): the two ribbon kernels are
                // droppable with the family, but ribbonBuffer_/ringTransformsBuffer_
                // are exclusive-in-Render-Entity + Photographer. Retire = re-section
                // those groups. (RIBBON_1: no ring readback staging was ever built,
                // so there is none to drop with them.)
                ribbonBuffer_ = makeBuffer("Ribbon State", sizeof(GPURibbonState), SU | wgpu::BufferUsage::Uniform);
                // RIBBON_2: CopySrc dropped. It was allocated for a ring
                // readback that was never built, and the LATENT note above
                // says so in the same breath — a usage bit that names a
                // reader nobody wrote is false authority (L30).
                ringTransformsBuffer_ = makeBuffer("Ring Transforms",
                    sizeof(GPURibbonRingTransform) * Dim::RIBBON_MAX_RINGS,
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                // RIBBON_1 — the chord spine and the body. Both GPU-sovereign:
                // CopyDst is here for the one CPU word (reset_ribbon_body's zeroed
                // head) and for nothing else. Boot-allocated, never reallocated.
                ribbonSpineBuffer_ = makeBuffer("Ribbon Spine",
                    sizeof(float) * 4 * Dim::RIBBON_SPINE_SLOTS,
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                ribbonBodyBuffer_ = makeBuffer("Ribbon Body",
                    sizeof(GPURibbonBody),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                fieldForcesBuffer_ = makeBuffer("Field Forces",
                    sizeof(float) * 4 * Dim::FIELD_SUBSCRIBER_CAP,
                    wgpu::BufferUsage::Storage);
                // CHORD_2: one 6656 B uniform block where three buffers stood.
                fieldBusBuffer_ = makeBuffer("Field Bus", sizeof(GPUFieldBus), UU);
                vpBuffer_ = makeBuffer("VP Matrix", sizeof(GPUVPMatrix),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst
                    | wgpu::BufferUsage::CopySrc);
                    // CHORD_3: Uniform out, CopySrc in. DOMESDAY_0 B1's g1:3
                    // window is gone; the matrix reaches the render stages
                    // as frame_r.vp, copied from here on the frame encoder.
                    // The compute face (g2:240 vp_data) still binds as storage.
                // LATENT[gate-a-shared] spot_lights (SH·mb): the staging buffer half of this note is spent — ATLAS_1revB U3" retired spotVPStagingBuffer_, so what remains dedicated is spotShadowMapTexture_ (the atlas) alone; the spot array rides the frame-R block's lighting member (CHORD_3), which is exclusive-in-Render-Entity + Photographer and carries the sun and point arrays too, and the atlas is bound in Shadow Texture. Retire = re-section those groups AND split the block.
                // CHORD_3: two instances of the 1024 B render-frame block.
                // WALLET_1revA's ruling rides inside it unchanged — UNIFORM,
                // not storage, because the whole point of the lighting block
                // is that it stops spending F-stage storage seats.
                frameRMainBuffer_  = makeBuffer("Frame R (Main)", sizeof(GPUFrameR), UU);
                shadowSlotBuffer_ = makeBuffer("Shadow Slot Ring",
                    MAX_SPOT_LIGHTS * Dim::UNIFORM_DYNAMIC_STRIDE,
                    wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst);
                // Record i holds i, written once and never again: the light
                // index is the OFFSET, so the contents are a constant ladder.
                for (uint32_t i = 0; i < MAX_SPOT_LIGHTS; i++)
                    device_.GetQueue().WriteBuffer(shadowSlotBuffer_,
                        i * Dim::UNIFORM_DYNAMIC_STRIDE, &i, sizeof(uint32_t));
                frameRPhotoBuffer_ = makeBuffer("Frame R (Photographer)", sizeof(GPUFrameR), UU);
                // PORT_3b — routed through makeBuffer like every other
                // buffer, so the budget sees them. Same label, size and
                // usage; the descriptor was hand-rolled only because
                // makeBuffer post-dates these two lines.
                agentStateReadbackStaging_ = makeBuffer("Agent State Readback Staging",
                    Dim::MAX_AGENTS * sizeof(GPUAgentState),
                    wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead);
                floatingEntityReadbackStaging_ = makeBuffer("Floating Entity Readback Staging",
                    Dim::TOTAL_FLOATING_SLOTS * sizeof(GPUFloatingEntityState),
                    wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead);
                // ATRIUM_11 — THE THIRD READBACK, and the smallest: 48 bytes,
                // once per frame. The camera's source already carries CopySrc
                // for the CHORD_3 block copy, so nothing else changes to make
                // this legal.
                //
                // PLUMB_0 C1 (RULING-1) — UNGATED, BECAUSE THE POSE IS NOW A
                // SPINE FACT AND NOT AN INSTRUMENT. It was created "only when
                // the witness is armed", which was harmless while
                // camera_witness was true in every column — and would have
                // become a copy into a null buffer the day the witness took
                // the retirement its own banner promises, or the day a column
                // turned it off. Promoting the copy without promoting the
                // buffer is half a promotion and the wrong half.
                cameraReadbackStaging_ = makeBuffer("Camera State Readback Staging",
                    sizeof(GPUCameraState),
                    wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead);
                // THE FRAME METER — GPU half. Created only when the
                // instruments dial arms the meter AND the device carries
                // timestamp-query (the cartridge prints the loud boot line
                // when the device is the reason); CPU rows are unaffected
                // either way. Dial off → no query set, no resolve buffer, no
                // readback staging, and meter_arm_* returns nullptr at every
                // pass site (core/instruments.hpp).
                meterEnabled_ = INSTRUMENTS.frame_meter
                    && device_.HasFeature(wgpu::FeatureName::TimestampQuery);
                if (meterEnabled_) {
                    wgpu::QuerySetDescriptor qd{};
                    qd.label = "Frame Meter Timestamps";
                    qd.type = wgpu::QueryType::Timestamp;
                    qd.count = METER_QUERY_COUNT;
                    meterQuerySet_ = device_.CreateQuerySet(&qd);
                    meterResolveBuffer_ = makeBuffer("Frame Meter Resolve",
                        METER_QUERY_COUNT * sizeof(uint64_t),
                        wgpu::BufferUsage::QueryResolve | wgpu::BufferUsage::CopySrc);
                    meterReadbackStaging_ = makeBuffer("Frame Meter Readback Staging",
                        METER_QUERY_COUNT * sizeof(uint64_t),
                        wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead);
                }
                tileGridBuffer_ = makeBuffer("Tile Grid", sizeof(GPUTileGrid), UU);
                patchInstancesBuffer_ = makeBuffer("Patch Instances",
                    sizeof(GPUPatchInstance) * Dim::MAX_ACTIVE_PATCHES,
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                patchGridBuffer_ = makeBuffer("Patch Grid",
                    sizeof(GPUPatchGrid),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                patchParamsBuffer_ = makeBuffer("Patch Params Batch",
                    Dim::MAX_ACTIVE_PATCHES * sizeof(GPUPatchParams),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                // The draw ledger (BUNDLE_1) — one record per drawable whose
                // count the CPU authors. Zero-initialised, which is the right
                // rest: every family is empty until something stages it, and
                // a record of zeros draws nothing.
                drawLedgerBuffer_ = makeBuffer("Draw Ledger",
                    DR_COUNT * DRAW_RECORD_STRIDE,
                    wgpu::BufferUsage::Indirect | wgpu::BufferUsage::CopyDst);
                // Self-Portrait Gallery
                photographerVPBuffer_ = makeBuffer("Photographer VP",
                    sizeof(GPUVPMatrix),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst
                    | wgpu::BufferUsage::CopySrc);
                    // CHORD_3: Uniform out, CopySrc in — DOMESDAY_0 B1's
                    // shared frameRLayout_ slot is gone; the matrix reaches
                    // the photographer's block by copy. The kernel face
                    // (g2:161 photographer_vp) still binds as storage.
                photographerCameraBuffer_ = makeBuffer("Photographer Camera",
                    sizeof(GPUCameraState),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst
                    | wgpu::BufferUsage::CopySrc);
                    // CHORD_3: Uniform out, CopySrc in — DOMESDAY_0 B2's
                    // shared frameRLayout_ slot is gone; the camera reaches
                    // the photographer's block by copy. The kernel face
                    // (g2:162 photographer_camera_out) still binds as storage.
                photographerConfigBuffer_ = makeBuffer("Photographer Config",
                    sizeof(GPUPhotographerConfig),
                    wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst);
                // LATENT[gate-a-shared] gallery (SH·mb): gallery/wall-painting/photographer pipelines + offscreen textures + gallery groups droppable (photographer rides gallery's bit — LATENT[roster-split:photographer]), but paintingSlotsBuffer_ is exclusive-in-Compute-Entity + Entity-Placement. Retire = re-section those groups.
                paintingSlotsBuffer_ = makeBuffer("Painting Slots",
                    sizeof(GPUPaintingSlot) * Dim::PAINTING_MAX_SLOTS,
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);

                // GPU frustum culling — LOD0 only.
                // Compute writes args+atomic to frustumComputeBuffer_, then CopyBufferToBuffer to indirect.
                // THE DRAW PLAN segments (ECONOMY_1 closing arm) — one list
                // buffer, three 256-aligned segments, read by three sibling
                // bind groups at static offsets. TWIN: world.wgsl FC_SEG_*
                // (L3 MIRROR — change both rooms together). Capacities are
                // A/B 128 (the handoff said 64; finite_radius 4 puts 81
                // LOD0 patches in the plan — 64 would overflow its own
                // indoor clause), C 256.
                //   A [   0,  512): LOD0 zone-overlapped -> full IB
                //   B [ 512, 1024): LOD0 clean           -> cap-only IB
                //   C [1024, 2048): LOD1 frustum-visible -> LOD1 IB
                frustumIndirectLOD0_ = makeBuffer("Frustum Indirect LOD0",
                    FC_ARGS_BYTES,
                    wgpu::BufferUsage::Indirect | wgpu::BufferUsage::CopyDst);
                frustumComputeBuffer_ = makeBuffer("Frustum Compute Staging",
                    FC_ARGS_BYTES,
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst);
                // WRAP_0 U4 — created only with the meter, like the meter's own
                // staging: the audience's build allocates nothing for a reading
                // it never takes.
                if (meterEnabled_)
                    frustumCountReadback_ = makeBuffer("Frustum Count Readback",
                        FC_ARGS_BYTES,
                        wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead);
                visiblePatchIndicesBuffer_ = makeBuffer("Visible Patch Indices",
                    FC_LIST_BYTES,
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst
                    | wgpu::BufferUsage::Vertex);
                    // DOMESDAY_0 B3: + Vertex — the render side pulls the
                    // visible list as an instance-step attribute now; the
                    // cull kernel's g2:63 fc_visible face still writes it
                    // as storage (compute-written vertex pull is legal;
                    // usage is a creation flag).
                drawPlanBuffer_ = makeBuffer("Draw Plan Params",
                    sizeof(GPUDrawPlanParams),
                    wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst);
                if (!drawPlanBuffer_) return false;
                // ATRIUM_11 — UNGATED (PLUMB_0 closing refuter). C1 made the
                // buffer unconditional and left its null-check under the
                // instrument, so with the witness off a failed allocation went
                // unreported and the promoted copy would have run against
                // nothing. The third residue of the same half-promotion.
                if (!cameraReadbackStaging_) return false;

                return signalBuffer_ && configBuffer_ &&
                    agentStateBuffer_ && agentStateReadbackStaging_ &&
                    cameraBuffer_ && floatingEntityBuffer_ && ringTransformsBuffer_ && ribbonSpineBuffer_ && ribbonBodyBuffer_ && fieldForcesBuffer_ && fieldBusBuffer_ &&
                    vpBuffer_ && frameRMainBuffer_ && frameRPhotoBuffer_ &&
                    shadowSlotBuffer_ &&
                    tileGridBuffer_ && patchInstancesBuffer_ &&
                    patchGridBuffer_ && patchParamsBuffer_ && drawLedgerBuffer_ &&
                    photographerVPBuffer_ && photographerCameraBuffer_ &&
                    photographerConfigBuffer_ && paintingSlotsBuffer_ &&
                    agentRoomBuffer_ && sceneConstantsBuffer_ &&
                    frustumIndirectLOD0_ && frustumComputeBuffer_ && visiblePatchIndicesBuffer_;
            }

            bool createMeshBuffers() {
                // THE PATCH INDEX BUFFERS ARE UINT16 (LATTICE_3). Every index
                // the three builders emit is a vertex ordinal under the
                // unified decode's address space, so 16 bits is not a
                // narrowing choice — it is the width the data always had.
                // Halves the index fetch per LOD0 instance (~203 KB -> ~101 KB)
                // and costs nothing: zero visual change, same draw counts.
                //
                // The FIVE MESH-GEN FAMILIES keep uint32 — their indices are
                // slot-based absolute addresses into a shared VB and run well
                // past 65,535.
                static_assert(Dim::UG_DECODE_VERTS - 1 <= 0xFFFFu,
                    "LATTICE_3: the unified ground addresses in 16 bits");

                // WriteBuffer wants a 4-byte multiple and an odd index count is
                // 2 mod 4. Pad with one index that is never drawn — the draw
                // reads the count, which is captured BEFORE this runs. Every
                // builder emits triangles in quad pairs, so the counts are even
                // today; a bound stated is a bound that cannot rot.
                auto pad_u16 = [](std::vector<uint16_t>& v) {
                    if (v.size() & 1u) v.push_back(0);
                };
                // The padded byte count for `count` uint16 indices — the SAME
                // number pad_u16 produces, said in terms of the draw count so
                // the RESOURCES ledger row names it. Buffer size and write
                // size are one expression, not two that happen to agree.
                auto ib_bytes_u16 = [](uint32_t count) -> uint64_t {
                    return (static_cast<uint64_t>(count) * 2u + 3u) & ~uint64_t{3};
                };

                // Patch index buffer -- CPU-generated, shared by all patch instances
                // SKIRTS (weld #2): each LOD appends a full-perimeter skirt — the
                // edge ring duplicated as verts [SKIRT_GRID_VERTS + k], which the
                // VS drops by PATCH_SKIRT_DEPTH and quad-strips ring->copy to hide
                // inter-patch cracks. skirt_grid_index MIRRORS world.wgsl
                // patch_skirt_grid — the two MUST agree. Winding (a,b,sa)+(b,sb,sa)
                // faces outward on all four edges (matched to the grid's +Y-front
                // convention under cullMode=Back/frontFace=CCW).
                constexpr uint32_t SKIRT_RING = 4 * Dim::PATCH_MESH_N;                                   // 256
                constexpr uint32_t SKIRT_GRID_VERTS = (Dim::PATCH_MESH_N + 1) * (Dim::PATCH_MESH_N + 1); // 4225
                auto skirt_grid_index = [](uint32_t k) -> uint32_t {
                    const uint32_t N = Dim::PATCH_MESH_N;
                    const uint32_t S = N + 1;
                    uint32_t vx, vz;
                    if      (k < N)     { vx = k;             vz = 0; }
                    else if (k < 2 * N) { vx = N;             vz = k - N; }
                    else if (k < 3 * N) { vx = N - (k - 2 * N); vz = N; }
                    else                { vx = 0;             vz = N - (k - 3 * N); }
                    return vz * S + vx;
                };
                    // The n=4 perimeter walk of a cell's 5×5 cap grid — the same
                    // CW bottom/right/top/left shape as skirt_grid_index (n=64).
                    auto cell_perimeter = [](uint32_t k, uint32_t& lx, uint32_t& lz) {
                        const uint32_t n = Dim::UG_QUADS_PER_CELL;   // 4
                        if      (k < n)     { lx = k;               lz = 0; }
                        else if (k < 2 * n) { lx = n;               lz = k - n; }
                        else if (k < 3 * n) { lx = n - (k - 2 * n); lz = n; }
                        else                { lx = 0;               lz = n - (k - 3 * n); }
                    };
                    // THE INVERSE OF cell_perimeter — a cap-perimeter local (lx,lz)
                    // to its base-band slot k. Well-defined because every skirt ring
                    // vertex sits on a cell perimeter by construction, at either
                    // stride: the ring walks vz==0, vx==N, vz==N, vx==0, and
                    // N/UG_QUADS_PER_CELL lands on a cell edge with cx/cz clamped to
                    // 15, so lx or lz is 0 or 4 at every slot. LOD1's stride-2 locals
                    // are the subset {0,2,4}, and a function total over all 16
                    // perimeter positions is total over any subset of them. Branch
                    // ORDER carries the four corners: (0,0) is claimed by the bottom
                    // walk, not the left, exactly as cell_perimeter emits it at k=0.
                    constexpr auto cell_perimeter_slot = [](uint32_t lx, uint32_t lz) -> uint32_t {
                        const uint32_t n = Dim::UG_QUADS_PER_CELL;          // 4
                        if (lz == 0 && lx <  n) return lx;                  // bottom (0..3, 0)
                        if (lx == n && lz <  n) return n + lz;              // right  (4, 0..3)
                        if (lz == n && lx >  0) return 2 * n + (n - lx);    // top    (4..1, 4)
                        return 3 * n + (n - lz);                            // left   (0, 4..1)
                    };
                    static_assert(Dim::UG_QUADS_PER_CELL == 4 && Dim::UG_BASE_VERTS_PER_CELL == 16,
                                  "SKIRT_WELD_1/P: cell_perimeter_slot is written for n=4/16 slots");
                    // THE INVERSE IS ASSERTED, NOT ASSUMED — all sixteen slots round
                    // trip, and the sixteen results are 0..15 each exactly once, so
                    // the map is a bijection of the cap perimeter onto the base band.
                    // The right-hand column IS cell_perimeter's emission, which §4
                    // freezes; stated as literals because cell_perimeter is a runtime
                    // lambda and cannot be called from a constant expression without
                    // editing it, which this campaign may not do.
                    static_assert(
                        cell_perimeter_slot(0, 0) ==  0 && cell_perimeter_slot(1, 0) ==  1 &&
                        cell_perimeter_slot(2, 0) ==  2 && cell_perimeter_slot(3, 0) ==  3 &&
                        cell_perimeter_slot(4, 0) ==  4 && cell_perimeter_slot(4, 1) ==  5 &&
                        cell_perimeter_slot(4, 2) ==  6 && cell_perimeter_slot(4, 3) ==  7 &&
                        cell_perimeter_slot(4, 4) ==  8 && cell_perimeter_slot(3, 4) ==  9 &&
                        cell_perimeter_slot(2, 4) == 10 && cell_perimeter_slot(1, 4) == 11 &&
                        cell_perimeter_slot(0, 4) == 12 && cell_perimeter_slot(0, 3) == 13 &&
                        cell_perimeter_slot(0, 2) == 14 && cell_perimeter_slot(0, 1) == 15,
                        "SKIRT_WELD_1/P: cell_perimeter_slot must invert cell_perimeter on all 16 slots");

                    // SKIRT_WELD_1/P — the ring's top edge on the BASE band. It hung
                    // on cap verts, and cap verts are CELL-OWNED (UNIFIED_GROUND_1):
                    // ring slot k and k+1 straddle a cell seam at every multiple of
                    // UG_QUADS_PER_CELL, so one live cell turned that quad into a ramp
                    // from ground+alive_height to ground — an extra triangle, dragged
                    // up by a cell it does not belong to, on top of a curtain that
                    // already sealed the same edge. Base twins carry wall = 1 ->
                    // lift_scale = 0 (WALL_1): they never lift, so the ring is one
                    // surface again and the discontinuity stays the curtain's job.
                    auto skirt_base_index = [&](uint32_t k) {
                        const uint32_t N = Dim::PATCH_MESH_N;
                        uint32_t vx, vz;
                        if      (k < N)     { vx = k;               vz = 0; }
                        else if (k < 2 * N) { vx = N;               vz = k - N; }
                        else if (k < 3 * N) { vx = N - (k - 2 * N); vz = N; }
                        else                { vx = 0;               vz = N - (k - 3 * N); }
                        uint32_t cx = vx / Dim::UG_QUADS_PER_CELL; if (cx > 15) cx = 15;
                        uint32_t cz = vz / Dim::UG_QUADS_PER_CELL; if (cz > 15) cz = 15;
                        uint32_t lx = vx - Dim::UG_QUADS_PER_CELL * cx;
                        uint32_t lz = vz - Dim::UG_QUADS_PER_CELL * cz;
                        return Dim::UG_BASE_BASE
                             + (cz * Dim::PATCH_CELL_N + cx) * Dim::UG_BASE_VERTS_PER_CELL
                             + cell_perimeter_slot(lx, lz);
                    };

                // LOD-0: cap tiles + curtains + skirt — ~50,688 indices
                // (UNIFIED_GROUND_1: the legacy 64×64 grid quads are replaced by
                //  per-cell cap tiles [16 quads/cell] + curtain quads [16/cell];
                //  the skirt ring keeps its legacy slots, top edge re-aimed at
                //  cap outer verts. The LOD-1 IB below indexes cap corners,
                //  base corners and skirt copies (CELL_1); the legacy grid
                //  [0, PATCH_GRID_VERT_COUNT) is zero-reader.)
                // ECONOMY_1 E1: one builder, one parameter. The curtain block
                // is the only conditional emission — cap + skirt are identical
                // in both buffers, so the cap-only IB is the full IB with the
                // curtain band absent (indices shift down; bands stay in cap,
                // [curtain,] skirt order).
                auto build_lod0_ib = [&](bool with_curtains) {
                    std::vector<uint16_t> idx;
                    idx.reserve(Dim::UG_CELLS_PER_PATCH * (16 + 16) * 6 + 4 * Dim::PATCH_MESH_N * 6);
                    // CAP QUADS — 256 cells × 16 quads (the house winding).
                    for (uint32_t cell = 0; cell < Dim::UG_CELLS_PER_PATCH; cell++) {
                        const uint32_t cap0 = Dim::UG_CAP_BASE + cell * Dim::UG_CAP_VERTS_PER_CELL;
                        for (uint32_t qz = 0; qz < Dim::UG_QUADS_PER_CELL; qz++) {
                            for (uint32_t qx = 0; qx < Dim::UG_QUADS_PER_CELL; qx++) {
                                uint32_t i00 = cap0 + qz * Dim::UG_CAP_STRIDE_C + qx;
                                uint32_t i10 = i00 + 1;
                                uint32_t i01 = i00 + Dim::UG_CAP_STRIDE_C;
                                uint32_t i11 = i01 + 1;
                                idx.push_back(static_cast<uint16_t>(i00)); idx.push_back(static_cast<uint16_t>(i01)); idx.push_back(static_cast<uint16_t>(i10));
                                idx.push_back(static_cast<uint16_t>(i10)); idx.push_back(static_cast<uint16_t>(i01)); idx.push_back(static_cast<uint16_t>(i11));
                            }
                        }
                    }
                    // CURTAIN QUADS — 256 cells × 16 quads: cap perimeter verts
                    // welded to their curtain-bottom twins (the skirt-quad shape,
                    // same winding as the ring below). Curtains seal DISCONTINUOUS
                    // per-cell lift; with no zone lift active every one is
                    // degenerate — the cap-only build omits the band.
                    if (with_curtains)
                    for (uint32_t cell = 0; cell < Dim::UG_CELLS_PER_PATCH; cell++) {
                        const uint32_t cap0  = Dim::UG_CAP_BASE  + cell * Dim::UG_CAP_VERTS_PER_CELL;
                        const uint32_t base0 = Dim::UG_BASE_BASE + cell * Dim::UG_BASE_VERTS_PER_CELL;
                        for (uint32_t k = 0; k < Dim::UG_BASE_VERTS_PER_CELL; k++) {
                            uint32_t k1 = (k + 1) % Dim::UG_BASE_VERTS_PER_CELL;
                            uint32_t lx, lz, lx1, lz1;
                            cell_perimeter(k, lx, lz);
                            cell_perimeter(k1, lx1, lz1);
                            uint32_t a  = cap0 + lz * Dim::UG_CAP_STRIDE_C + lx;
                            uint32_t b  = cap0 + lz1 * Dim::UG_CAP_STRIDE_C + lx1;
                            uint32_t sa = base0 + k;
                            uint32_t sb = base0 + k1;
                            idx.push_back(static_cast<uint16_t>(a)); idx.push_back(static_cast<uint16_t>(b)); idx.push_back(static_cast<uint16_t>(sa));
                            idx.push_back(static_cast<uint16_t>(b)); idx.push_back(static_cast<uint16_t>(sb)); idx.push_back(static_cast<uint16_t>(sa));
                        }
                    }
                    // SKIRT RING — the legacy loop; top edge = BASE-band twins
                    // (SKIRT_WELD_1/P), copies keep the legacy ring slots.
                    constexpr uint32_t SKIRT_RING_N = 4 * Dim::PATCH_MESH_N;
                    constexpr uint32_t SKIRT_GRID_V = (Dim::PATCH_MESH_N + 1) * (Dim::PATCH_MESH_N + 1);
                    for (uint32_t k = 0; k < SKIRT_RING_N; k++) {
                        uint32_t k1 = (k + 1) % SKIRT_RING_N;
                        uint32_t a  = skirt_base_index(k);
                        uint32_t b  = skirt_base_index(k1);
                        uint32_t sa = SKIRT_GRID_V + k;
                        uint32_t sb = SKIRT_GRID_V + k1;
                        idx.push_back(static_cast<uint16_t>(a)); idx.push_back(static_cast<uint16_t>(b)); idx.push_back(static_cast<uint16_t>(sa));
                        idx.push_back(static_cast<uint16_t>(b)); idx.push_back(static_cast<uint16_t>(sb)); idx.push_back(static_cast<uint16_t>(sa));
                    }
                    return idx;
                };
                {
                    std::vector<uint16_t> idx = build_lod0_ib(true);
                    patchIndexCount_ = (uint32_t)idx.size();   // the DRAW count, before the pad
                    pad_u16(idx);
                    patchIndexBuffer_ = makeBuffer("Patch IB (u16)",
                        ib_bytes_u16(patchIndexCount_),
                        wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst);
                    if (!patchIndexBuffer_) return false;
                    auto q = device_.GetQueue();
                    q.WriteBuffer(patchIndexBuffer_, 0, idx.data(), ib_bytes_u16(patchIndexCount_));
                }
                {
                    // ECONOMY_1 E1: the cap-only twin — caps + skirt, no curtain
                    // band. Selected by the lift-conservative switch when no
                    // zone lift can be nonzero.
                    std::vector<uint16_t> idx = build_lod0_ib(false);
                    patchIndexCountCapOnly_ = (uint32_t)idx.size();   // the DRAW count, before the pad
                    pad_u16(idx);
                    patchIndexBufferCapOnly_ = makeBuffer("Patch IB Cap-Only (u16)",
                        ib_bytes_u16(patchIndexCountCapOnly_),
                        wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst);
                    if (!patchIndexBufferCapOnly_) return false;
                    auto q = device_.GetQueue();
                    q.WriteBuffer(patchIndexBufferCapOnly_, 0, idx.data(), ib_bytes_u16(patchIndexCountCapOnly_));
                }

                {
                    // LOD-1: CELL SLABS (CELL_1, rev2 stride-2). A cell is a
                    // slab at every distance — the representation no longer
                    // changes across the LOD boundary, only the density does.
                    // The ring's rate is 1.5625 wu per quad edge (the stride-2
                    // cap lattice — every even local offset already decodes).
                    // Emission order caps -> skirt -> curtains makes
                    // caps+skirt a usable prefix: TWO counts, ONE buffer.
                    // RingClean_ stops at the prefix (shadow casters, and the
                    // held clean-ring draw-plan segment); RingZoned_ takes the
                    // curtain tail too (the main pass, via slot C). Indices
                    // land in the cap band, base band (curtain tail only) and
                    // skirt copies; the legacy grid [0, PATCH_GRID_VERT_COUNT)
                    // has no reader.
                    constexpr uint32_t s = Dim::UG_QUADS_PER_CELL / 2;   // stride-2 lattice
                    std::vector<uint16_t> idx;
                    idx.reserve(Dim::UG_CELLS_PER_PATCH * (4 + 8) * 6
                              + (4 * Dim::PATCH_MESH_N / s) * 6);  // 19200
                    // caps: 2×2 quads per cell on the corner lattice
                    for (uint32_t cell = 0; cell < Dim::UG_CELLS_PER_PATCH; cell++) {
                        const uint32_t cap0 = Dim::UG_CAP_BASE + cell * Dim::UG_CAP_VERTS_PER_CELL;
                        for (uint32_t qz = 0; qz < 2; qz++) {
                            for (uint32_t qx = 0; qx < 2; qx++) {
                                uint32_t i00 = cap0 + (qz * s) * Dim::UG_CAP_STRIDE_C + qx * s;
                                uint32_t i10 = i00 + s;
                                uint32_t i01 = i00 + s * Dim::UG_CAP_STRIDE_C;
                                uint32_t i11 = i01 + s;
                                idx.push_back(static_cast<uint16_t>(i00)); idx.push_back(static_cast<uint16_t>(i01)); idx.push_back(static_cast<uint16_t>(i10));
                                idx.push_back(static_cast<uint16_t>(i10)); idx.push_back(static_cast<uint16_t>(i01)); idx.push_back(static_cast<uint16_t>(i11));
                            }
                        }
                    }
                    // skirt: the stride-2 ring — top edge on the BASE band
                    // (SKIRT_WELD_1/P), copies on the matching legacy ring slots
                    for (uint32_t k = 0; k < SKIRT_RING; k += s) {
                        uint32_t k1 = (k + s) % SKIRT_RING;
                        uint32_t a  = skirt_base_index(k);
                        uint32_t b  = skirt_base_index(k1);
                        uint32_t sa = SKIRT_GRID_VERTS + k;
                        uint32_t sb = SKIRT_GRID_VERTS + k1;
                        idx.push_back(static_cast<uint16_t>(a)); idx.push_back(static_cast<uint16_t>(b)); idx.push_back(static_cast<uint16_t>(sa));
                        idx.push_back(static_cast<uint16_t>(b)); idx.push_back(static_cast<uint16_t>(sb)); idx.push_back(static_cast<uint16_t>(sa));
                    }
                    // caps + skirt is a usable prefix: the clean count stops here
                    patchIndexCountRingClean_ = (uint32_t)idx.size();
                    // curtains: the stride-2 perimeter walk, cap vert to base twin
                    for (uint32_t cell = 0; cell < Dim::UG_CELLS_PER_PATCH; cell++) {
                        const uint32_t cap0  = Dim::UG_CAP_BASE  + cell * Dim::UG_CAP_VERTS_PER_CELL;
                        const uint32_t base0 = Dim::UG_BASE_BASE + cell * Dim::UG_BASE_VERTS_PER_CELL;
                        for (uint32_t k = 0; k < Dim::UG_BASE_VERTS_PER_CELL; k += s) {
                            uint32_t k1 = (k + s) % Dim::UG_BASE_VERTS_PER_CELL;
                            uint32_t lx, lz, lx1, lz1;
                            cell_perimeter(k, lx, lz);
                            cell_perimeter(k1, lx1, lz1);
                            uint32_t a  = cap0 + lz * Dim::UG_CAP_STRIDE_C + lx;
                            uint32_t b  = cap0 + lz1 * Dim::UG_CAP_STRIDE_C + lx1;
                            uint32_t sa = base0 + k;
                            uint32_t sb = base0 + k1;
                            idx.push_back(static_cast<uint16_t>(a)); idx.push_back(static_cast<uint16_t>(b)); idx.push_back(static_cast<uint16_t>(sa));
                            idx.push_back(static_cast<uint16_t>(b)); idx.push_back(static_cast<uint16_t>(sb)); idx.push_back(static_cast<uint16_t>(sa));
                        }
                    }
                    patchIndexCountRingZoned_ = (uint32_t)idx.size();   // the DRAW count, before the pad
                    pad_u16(idx);
                    patchIndexBufferLOD1_ = makeBuffer("Patch IB LOD1 (u16)",
                        ib_bytes_u16(patchIndexCountRingZoned_),
                        wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst);
                    if (!patchIndexBufferLOD1_) return false;
                    auto q = device_.GetQueue();
                    q.WriteBuffer(patchIndexBufferLOD1_, 0, idx.data(), ib_bytes_u16(patchIndexCountRingZoned_));
                }

                return createSphereMesh() && createMonolithMesh() && createArchMesh() && createColumnMesh() && createPalmMesh() && createCactusMesh() && createBladeMesh() && createPyramidMesh() && createShellMesh() && createGoLZoneBuffers();
            }

            bool createSphereMesh() {
                // LATENT[gate-a-shared] sphere (SH·dc): VB/IB exclusive+droppable, but co-owns floatingEntityBuffer_ (sphere+cube) and draw_sphere isn't self-count-gated. Retire = draw self-gate, then skip.
                std::vector<MeshVertex> v;
                std::vector<uint32_t> idx;
                const int ST = Dim::SPHERE_STACKS, SL = Dim::SPHERE_SLICES;
                for (int st = 0; st <= ST; st++) {
                    float phi = Dim::PI * float(st) / float(ST), sp = std::sin(phi), cp = std::cos(phi);
                    for (int sl = 0; sl <= SL; sl++) {
                        float th = 2.0f * Dim::PI * float(sl) / float(SL), sth = std::sin(th), cth = std::cos(th);
                        float x = sp * cth, y = cp, z = sp * sth;
                        v.push_back({ {x,y,z},{x,y,z} });
                    }
                }
                for (int st = 0; st < ST; st++)
                    for (int sl = 0; sl < SL; sl++) {
                        uint32_t i00 = st * (SL + 1) + sl, i10 = i00 + 1, i01 = i00 + (SL + 1), i11 = i01 + 1;
                        if (st != 0) { idx.push_back(i00); idx.push_back(i10); idx.push_back(i11); }
                        if (st != ST - 1) { idx.push_back(i00); idx.push_back(i11); idx.push_back(i01); }
                    }
                sphereIndexCount_ = (uint32_t)idx.size();
                sphereVertexBuffer_ = makeBuffer("Sphere VB", v.size() * sizeof(MeshVertex), wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst);
                sphereIndexBuffer_ = makeBuffer("Sphere IB", idx.size() * 4, wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst);
                if (!sphereVertexBuffer_ || !sphereIndexBuffer_) return false;
                auto q = device_.GetQueue();
                q.WriteBuffer(sphereVertexBuffer_, 0, v.data(), v.size() * sizeof(MeshVertex));
                q.WriteBuffer(sphereIndexBuffer_, 0, idx.data(), idx.size() * 4);
                return true;
            }

            bool createMonolithMesh() {
                // LATENT[gate-a-shared] cube (SH·dc): monolith VB/IB exclusive+droppable, but co-owns floatingEntityBuffer_ (sphere+cube) and draw_monolith isn't self-count-gated. Retire = draw self-gate, then skip.
                // Imperfect unit cube: 6 faces, slightly jittered corners.
                // Same MeshVertex format as sphere (pos + normal).
                // Face index derived from normal direction in VS.
                static constexpr float J = 0.06f;  // jitter magnitude
                auto jit = [](int corner, int axis) -> float {
                    uint32_t h = (uint32_t)(corner * 3 + axis);
                    h = (h * 2654435769u) ^ (h >> 16);
                    return ((float)(h & 0xFFFFu) / 65535.0f - 0.5f) * J * 2.0f;
                    };

                // 8 corners: y in [-1,+1], z in [-1,+1], x in [-1,+1]
                float corners[8][3];
                int ci = 0;
                for (int y = -1; y <= 1; y += 2)
                    for (int z = -1; z <= 1; z += 2)
                        for (int x = -1; x <= 1; x += 2) {
                            corners[ci][0] = (float)x + jit(ci, 0);
                            corners[ci][1] = (float)y + jit(ci, 1);
                            corners[ci][2] = (float)z + jit(ci, 2);
                            ci++;
                        }

                // 6 faces (CCW winding from outside)
                static constexpr int FACES[6][4] = {
                    {1, 5, 7, 3},  // +X
                    {0, 2, 6, 4},  // -X
                    {4, 6, 7, 5},  // +Y
                    {0, 1, 3, 2},  // -Y
                    {2, 3, 7, 6},  // +Z
                    {0, 4, 5, 1},  // -Z
                };

                std::vector<MeshVertex> v;
                std::vector<uint32_t> idx;

                for (int f = 0; f < 6; f++) {
                    float* c0 = corners[FACES[f][0]];
                    float* c1 = corners[FACES[f][1]];
                    float* c2 = corners[FACES[f][2]];
                    float* c3 = corners[FACES[f][3]];

                    // Face normal from cross product of edges
                    float e1[3] = { c1[0] - c0[0], c1[1] - c0[1], c1[2] - c0[2] };
                    float e2[3] = { c2[0] - c0[0], c2[1] - c0[1], c2[2] - c0[2] };
                    float nx = e1[1] * e2[2] - e1[2] * e2[1];
                    float ny = e1[2] * e2[0] - e1[0] * e2[2];
                    float nz = e1[0] * e2[1] - e1[1] * e2[0];
                    float len = std::sqrt(nx * nx + ny * ny + nz * nz);
                    nx /= len; ny /= len; nz /= len;

                    uint32_t base = (uint32_t)v.size();
                    v.push_back({ {c0[0],c0[1],c0[2]}, {nx,ny,nz} });
                    v.push_back({ {c1[0],c1[1],c1[2]}, {nx,ny,nz} });
                    v.push_back({ {c2[0],c2[1],c2[2]}, {nx,ny,nz} });
                    v.push_back({ {c3[0],c3[1],c3[2]}, {nx,ny,nz} });

                    idx.push_back(base); idx.push_back(base + 1); idx.push_back(base + 2);
                    idx.push_back(base); idx.push_back(base + 2); idx.push_back(base + 3);
                }

                monolithIndexCount_ = (uint32_t)idx.size();
                monolithVertexBuffer_ = makeBuffer("Monolith VB", v.size() * sizeof(MeshVertex),
                    wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst);
                monolithIndexBuffer_ = makeBuffer("Monolith IB", idx.size() * 4,
                    wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst);
                if (!monolithVertexBuffer_ || !monolithIndexBuffer_) return false;
                auto q = device_.GetQueue();
                q.WriteBuffer(monolithVertexBuffer_, 0, v.data(), v.size() * sizeof(MeshVertex));
                q.WriteBuffer(monolithIndexBuffer_, 0, idx.data(), idx.size() * 4);

                std::cout << "[GPUState] Monolith mesh: "
                    << v.size() << " verts, " << idx.size() << " indices\n";
                return true;
            }

            bool createArchMesh() {
                // LATENT[gate-a-shared] arch (SH·mb): mesh VB/IB/params + 3 pipelines droppable, but archGroundBuffer_ is exclusive-in-Entity-Placement. Retire = re-section Entity Placement.
                // VB: Vertex + CopyDst (transition fallback) + Storage (GPU compute writes)
                archVertexBuffer_ = makeBuffer("Arch VB (GPU mesh gen)",
                    Dim::AMG_TOTAL_VERTICES * sizeof(ArchVertex),
                    wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage);
                // IB: Index + CopyDst (transition fallback) + Storage (GPU compute writes)
                archIndexBuffer_ = makeBuffer("Arch IB (GPU mesh gen)",
                    Dim::AMG_TOTAL_INDICES * sizeof(uint32_t),
                    wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage);
                // Per-arch ground Y correction buffer (GPU compute writes, VS reads)
                archGroundBuffer_ = makeBuffer("Arch Ground Y",
                    Dim::MAX_ARCH_INSTANCES * sizeof(GPUArchGroundEntry),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                // Mesh gen params buffer (16 × 80 bytes — MOSAIC_1 growth; size derives from sizeof below)
                archMeshParamsBuffer_ = makeBuffer("Arch Mesh Params",
                    Dim::MAX_ARCH_INSTANCES * sizeof(GPUArchMeshParams),
                    // CHORD_1: Uniform dropped — the occupier window it served
                    // now rides agentRoomBuffer_; the mesh-gen kernels'
                    // read-only storage face is all that is left on this buffer.
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);

                if (!archVertexBuffer_ || !archIndexBuffer_ || !archGroundBuffer_ ||
                    !archMeshParamsBuffer_) return false;

                // Index count starts at 0 — updated by cartridge as arches spawn/evict
                archIndexCount_ = 0;

                // Zero-init params buffer (all slots inactive)
                {
                    GPUArchMeshParams emptyParams[Dim::MAX_ARCH_INSTANCES]{};
                    device_.GetQueue().WriteBuffer(archMeshParamsBuffer_, 0, emptyParams,
                        sizeof(GPUArchMeshParams) * Dim::MAX_ARCH_INSTANCES);
                }

                // Zero-init VB so degenerate indices (pointing to vertex 0) have
                // safe arch_index=0, preventing out-of-bounds ground_y lookups in VS.
                {
                    std::vector<uint8_t> zeros(Dim::AMG_TOTAL_VERTICES * sizeof(ArchVertex), 0);
                    device_.GetQueue().WriteBuffer(archVertexBuffer_, 0, zeros.data(), zeros.size());
                }

                std::cout << "[GPUState] Arch buffers (GPU mesh gen): "
                    << Dim::AMG_TOTAL_VERTICES << " vert, "
                    << Dim::AMG_TOTAL_INDICES << " index capacity\n";
                return true;
            }

            bool createColumnMesh() {
                // LATENT[gate-a-shared] column (SH·mb): mesh VB/IB/params + 3 pipelines droppable (antenna rides this mesh — NO-RES), but columnGroundBuffer_ is exclusive-in-Entity-Placement. Retire = re-section Entity Placement.
                columnVertexBuffer_ = makeBuffer("Column VB (GPU mesh gen)",
                    Dim::CMG_TOTAL_VERTICES * sizeof(ArchVertex),
                    wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage);
                columnIndexBuffer_ = makeBuffer("Column IB (GPU mesh gen)",
                    Dim::CMG_TOTAL_INDICES * sizeof(uint32_t),
                    wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage);
                columnGroundBuffer_ = makeBuffer("Column Ground Y",
                    Dim::MAX_COLUMN_INSTANCES * sizeof(GPUColumnGroundEntry),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                columnMeshParamsBuffer_ = makeBuffer("Column Mesh Params",
                    Dim::MAX_COLUMN_INSTANCES * sizeof(GPUColumnMeshParams),
                    // CHORD_1: Uniform dropped — the occupier window it served
                    // now rides agentRoomBuffer_; the mesh-gen kernels'
                    // read-only storage face is all that is left on this buffer.
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);

                if (!columnVertexBuffer_ || !columnIndexBuffer_ || !columnGroundBuffer_ ||
                    !columnMeshParamsBuffer_) return false;

                columnIndexCount_ = 0;

                // Zero-init params (all slots inactive)
                {
                    GPUColumnMeshParams emptyParams[Dim::MAX_COLUMN_INSTANCES]{};
                    device_.GetQueue().WriteBuffer(columnMeshParamsBuffer_, 0, emptyParams,
                        sizeof(GPUColumnMeshParams) * Dim::MAX_COLUMN_INSTANCES);
                }

                // Zero-init VB for safe degenerate vertex access
                {
                    std::vector<uint8_t> zeros(Dim::CMG_TOTAL_VERTICES * sizeof(ArchVertex), 0);
                    device_.GetQueue().WriteBuffer(columnVertexBuffer_, 0, zeros.data(), zeros.size());
                }

                std::cout << "[GPUState] Column buffers (GPU mesh gen): "
                    << Dim::CMG_TOTAL_VERTICES << " vert, "
                    << Dim::CMG_TOTAL_INDICES << " index capacity\n";
                return true;
            }

            bool createPalmMesh() {
                // LATENT[gate-a-shared] palm (SH·dc): VB/IB/params exclusive+droppable, but co-owns plantComputeGroundBuffer_ (palm+cactus+blade) and draw_palm isn't self-count-gated. Retire = draw self-gate, then skip.
                palmVertexBuffer_ = makeBuffer("Palm VB (GPU mesh gen)",
                    Dim::PALMG_TOTAL_VERTICES * sizeof(ArchVertex),
                    wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage);
                palmIndexBuffer_ = makeBuffer("Palm IB (GPU mesh gen)",
                    Dim::PALMG_TOTAL_INDICES * sizeof(uint32_t),
                    wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage);
                palmMeshParamsBuffer_ = makeBuffer("Palm Mesh Params",
                    Dim::MAX_PALM_INSTANCES * sizeof(GPUPalmMeshParams),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);

                if (!palmVertexBuffer_ || !palmIndexBuffer_ ||
                    !palmMeshParamsBuffer_) return false;

                palmIndexCount_ = 0;
                {
                    GPUPalmMeshParams emptyParams[Dim::MAX_PALM_INSTANCES]{};
                    device_.GetQueue().WriteBuffer(palmMeshParamsBuffer_, 0, emptyParams,
                        sizeof(GPUPalmMeshParams) * Dim::MAX_PALM_INSTANCES);
                }
                {
                    std::vector<uint8_t> zeros(Dim::PALMG_TOTAL_VERTICES * sizeof(ArchVertex), 0);
                    device_.GetQueue().WriteBuffer(palmVertexBuffer_, 0, zeros.data(), zeros.size());
                }
                return true;
            }

            bool createCactusMesh() {
                // LATENT[gate-a-shared] cactus (SH·dc): VB/IB/params exclusive+droppable, but co-owns plantComputeGroundBuffer_ (palm+cactus+blade) and draw_cactus isn't self-count-gated. Retire = draw self-gate, then skip.
                cactusVertexBuffer_ = makeBuffer("Cactus VB (GPU mesh gen)",
                    Dim::CACTUSG_TOTAL_VERTICES * sizeof(ArchVertex),
                    wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage);
                cactusIndexBuffer_ = makeBuffer("Cactus IB (GPU mesh gen)",
                    Dim::CACTUSG_TOTAL_INDICES * sizeof(uint32_t),
                    wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage);
                cactusMeshParamsBuffer_ = makeBuffer("Cactus Mesh Params",
                    Dim::MAX_CACTUS_INSTANCES * sizeof(GPUCactusMeshParams),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                if (!cactusVertexBuffer_ || !cactusIndexBuffer_ ||
                    !cactusMeshParamsBuffer_) return false;
                cactusIndexCount_ = 0;
                {
                    GPUCactusMeshParams emptyParams[Dim::MAX_CACTUS_INSTANCES]{};
                    device_.GetQueue().WriteBuffer(cactusMeshParamsBuffer_, 0, emptyParams,
                        sizeof(GPUCactusMeshParams) * Dim::MAX_CACTUS_INSTANCES);
                }
                {
                    std::vector<uint8_t> zeros(Dim::CACTUSG_TOTAL_VERTICES * sizeof(ArchVertex), 0);
                    device_.GetQueue().WriteBuffer(cactusVertexBuffer_, 0, zeros.data(), zeros.size());
                }
                return true;
            }

            bool createBladeMesh() {
                // LATENT[gate-a-shared] blade (SH·dc): VB/IB/params exclusive+droppable, but co-owns plantComputeGroundBuffer_ (palm+cactus+blade, allocated below) and draw_blade isn't self-count-gated. Retire = draw self-gate, then skip.
                bladeVertexBuffer_ = makeBuffer("Blade VB (GPU mesh gen)",
                    Dim::BLADEG_TOTAL_VERTICES * sizeof(ArchVertex),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst);
                bladeIndexBuffer_ = makeBuffer("Blade IB (GPU mesh gen)",
                    Dim::BLADEG_TOTAL_INDICES * sizeof(uint32_t),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst);
                bladeMeshParamsBuffer_ = makeBuffer("Blade Mesh Params",
                    Dim::MAX_BLADE_INSTANCES * sizeof(GPUBladeClusterMeshParams),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                if (!bladeVertexBuffer_ || !bladeIndexBuffer_ ||
                    !bladeMeshParamsBuffer_) return false;
                bladeIndexCount_ = 0;
                {
                    GPUBladeClusterMeshParams emptyParams[Dim::MAX_BLADE_INSTANCES]{};
                    device_.GetQueue().WriteBuffer(bladeMeshParamsBuffer_, 0, emptyParams,
                        sizeof(GPUBladeClusterMeshParams) * Dim::MAX_BLADE_INSTANCES);
                }
                {
                    std::vector<uint8_t> zeros(Dim::BLADEG_TOTAL_VERTICES * sizeof(ArchVertex), 0);
                    device_.GetQueue().WriteBuffer(bladeVertexBuffer_, 0, zeros.data(), zeros.size());
                }

                // Combined plant ground buffer for compute (palm + cactus + blade)
                static constexpr uint32_t PLANT_GROUND_COUNT =
                    Dim::MAX_PALM_INSTANCES + Dim::MAX_CACTUS_INSTANCES + Dim::MAX_BLADE_INSTANCES;
                plantComputeGroundBuffer_ = makeBuffer("Plant Compute Ground Y",
                    PLANT_GROUND_COUNT * sizeof(GPUPalmGroundEntry),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                if (!plantComputeGroundBuffer_) return false;

                return true;
            }

            bool createPyramidMesh() {
                // Pyramids are TERRAIN (not drawn geometry): the instance array is
                // baked via contrib_pyramids_at. The GPU mesh-gen VB/IB/params were
                // the sweep's target; the ground-atlas buffer fell as residue
                // (its slot-48 write was reader-free).
                pyramidInstancesBuffer_ = makeBuffer("Pyramid Instances (GPU uniform)",
                    sizeof(GPUPyramidArray),
                    wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst);

                if (!pyramidInstancesBuffer_) return false;

                // Zero-init the instances buffer
                GPUPyramidArray empty{};
                device_.GetQueue().WriteBuffer(pyramidInstancesBuffer_, 0, &empty, sizeof(GPUPyramidArray));
                return true;
            }

            bool createShellMesh() {
                // ROSTER-GATE indoor_shell (a) — SEPARABLE: skip shell VB/IB
                // creation when disabled (zero GPU allocation, Rider A). The
                // shell is drawn only via draw_shell / draw_shadow_shell, both
                // of which early-return on shell_index_count==0; the count
                // stays 0 because apply_mood_indoor_shell is (b)-gated, so the
                // null buffers are never bound. The one SEPARABLE piece in
                // this arc; everything else is SH·* and
                // stays created-pristine.
                if constexpr (!ROSTER.indoor_shell) return true;
                shellVertexBuffer_ = makeBuffer("Shell VB",
                    Dim::SHELL_MAX_VERTICES * sizeof(ShellVertex),
                    wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst);
                shellIndexBuffer_ = makeBuffer("Shell IB",
                    Dim::SHELL_MAX_INDICES * sizeof(uint32_t),
                    wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst);
                if (!shellVertexBuffer_ || !shellIndexBuffer_) return false;
                shellIndexCount_ = 0;
                std::cout << "[GPUState] Shell buffers: "
                    << Dim::SHELL_MAX_VERTICES << " vert, "
                    << Dim::SHELL_MAX_INDICES << " index capacity\n";
                return true;
            }

            bool createGoLZoneBuffers() {
                // LATENT[gate-a-shared] gol (SH·mb): zone-mesh buffers + zoneLifeTexture_ + GoL Zone group + 5 gol pipelines droppable, but zoneConfigBuffer_/zoneLifeBuffer_ are exclusive-in-Compute-Entity + Entity-Placement. Retire = re-section both groups. (Residue recipe stays the Phase-I pristine form — gol is SH, not SEP.)
                // NOTE: this function also creates the pawn-aura and orb buffers below (each its own LATENT tag).
                zoneConfigBuffer_ = makeBuffer("GoL Zone Config",
                    sizeof(GPUGoLZoneArray),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);

                zoneDeriveRequestBuffer_ = makeBuffer("Zone Derive Requests",
                    sizeof(GPUZoneDeriveRequestArray),
                    wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst);

                zoneLifeBuffer_ = makeBuffer("GoL Zone Life State",
                    Dim::MAX_GOL_ZONES * Dim::GOL_ZONE_LIFE_STRIDE * sizeof(float),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);

                if (!zoneConfigBuffer_ || !zoneLifeBuffer_) return false;

                // Zone life texture: 32×32 × MAX_ZONES, R32Float (R = the cell's spring visual)
                {
                    wgpu::TextureDescriptor desc{};
                    desc.size = { Dim::GOL_ZONE_GRID, Dim::GOL_ZONE_GRID, Dim::MAX_GOL_ZONES };
                    desc.format = wgpu::TextureFormat::R32Float;
                    desc.usage = wgpu::TextureUsage::StorageBinding | wgpu::TextureUsage::TextureBinding;
                    desc.dimension = wgpu::TextureDimension::e2D;
                    zoneLifeTexture_ = makeTexture("GoL Zone Life Texture Array", desc);
                    if (!zoneLifeTexture_) return false;

                    // Write view (compute, storage texture)
                    wgpu::TextureViewDescriptor wvd{};
                    wvd.dimension = wgpu::TextureViewDimension::e2DArray;
                    wvd.arrayLayerCount = Dim::MAX_GOL_ZONES;
                    zoneLifeWriteView_ = zoneLifeTexture_.CreateView(&wvd);

                    // Read view (fragment, sampled texture)
                    wgpu::TextureViewDescriptor rvd{};
                    rvd.dimension = wgpu::TextureViewDimension::e2DArray;
                    rvd.arrayLayerCount = Dim::MAX_GOL_ZONES;
                    zoneLifeReadView_ = zoneLifeTexture_.CreateView(&rvd);
                }

                // Zero-init the config buffer
                GPUGoLZoneArray empty{};
                device_.GetQueue().WriteBuffer(zoneConfigBuffer_, 0, &empty, sizeof(GPUGoLZoneArray));

                std::cout << "[GPUState] GoL zone buffers: " << Dim::MAX_GOL_ZONES
                    << " zones × " << Dim::GOL_ZONE_GRID << "×" << Dim::GOL_ZONE_GRID << " grid\n";

                // Pawn aura buffers
                // LATENT[gate-a-shared] pawn_aura (SH·mb): config/cells buffers + Pawn Aura group + pawnAura pipeline droppable, but pawnAuraTexture_ (created in createTextures) is sampled by the terrain FS → bound in Render Texture + Compute Texture groups. Retire = re-section those two groups.
                pawnAuraConfigBuffer_ = makeBuffer("Pawn Aura Config",
                    sizeof(GPUPawnAuraConfig),
                    wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst);
                pawnAuraCellsBuffer_ = makeBuffer("Pawn Aura Cells",
                    PAWN_AURA_N * PAWN_AURA_N * sizeof(GPUPawnAuraCell),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                if (!pawnAuraConfigBuffer_ || !pawnAuraCellsBuffer_) return false;

                // Initialize aura cells to empty (cell_gx = 0x7FFFFFFF)
                {
                    std::vector<GPUPawnAuraCell> init_cells(PAWN_AURA_N * PAWN_AURA_N);
                    for (auto& c : init_cells) {
                        c.cell_gx = 0x7FFFFFFF;
                        c.cell_gz = 0x7FFFFFFF;
                    }
                    wgpu::Queue q = device_.GetQueue();
                    q.WriteBuffer(pawnAuraCellsBuffer_, 0, init_cells.data(),
                        init_cells.size() * sizeof(GPUPawnAuraCell));
                }

                // Orb sky layer buffers
                // LATENT[gate-a-shared] orbs (SH·mb): orbStatePrev + quad VB/IB + Orb Compute/Copy groups + 5 orb pipelines droppable, but orbStateBuffer_/orbConfigBuffer_ are read by the entity render + photographer passes → exclusive-in-Render-Entity + Photographer. Retire = re-section those groups.
                // Vertex usage: ORB_V binds this buffer as the orb pipeline's
                // instance-step vertex buffer (renderer.hpp, orbStateVBL); the
                // orb compute kernels still reach it as storage.
                orbStateBuffer_ = makeBuffer("Orb State",
                    Dim::MAX_ORBS * sizeof(GPUOrbState),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::Vertex |
                    wgpu::BufferUsage::CopyDst);
                // Pass 9: previous-frame snapshot for flocking neighbor reads.
                orbStatePrevBuffer_ = makeBuffer("Orb State Prev",
                    Dim::MAX_ORBS * sizeof(GPUOrbState),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                orbConfigBuffer_ = makeBuffer("Orb Config",
                    sizeof(GPUOrbConfig),
                    wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst);
                if (!orbStateBuffer_ || !orbStatePrevBuffer_ || !orbConfigBuffer_) return false;

                // Billboard quad: 4 corner positions (2-component), 6 indices
                {
                    const float quadVerts[] = {
                        -1.0f, -1.0f,
                         1.0f, -1.0f,
                         1.0f,  1.0f,
                        -1.0f,  1.0f,
                    };
                    orbQuadVB_ = makeBuffer("Orb Quad VB",
                        sizeof(quadVerts),
                        wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst);
                    if (!orbQuadVB_) return false;
                    wgpu::Queue q = device_.GetQueue();
                    q.WriteBuffer(orbQuadVB_, 0, quadVerts, sizeof(quadVerts));

                    const uint16_t quadIndices[] = { 0, 1, 2, 0, 2, 3 };
                    orbQuadIB_ = makeBuffer("Orb Quad IB",
                        sizeof(quadIndices),
                        wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst);
                    if (!orbQuadIB_) return false;
                    q.WriteBuffer(orbQuadIB_, 0, quadIndices, sizeof(quadIndices));
                }

                return true;
            }

            bool createTextures() {

                // Pawn aura texture (64×64 RGBA16Float — compute writes, FS reads)
                {
                    wgpu::TextureDescriptor desc{};
                    desc.size = { PAWN_AURA_N, PAWN_AURA_N, 1 };
                    desc.format = wgpu::TextureFormat::RGBA16Float;
                    desc.usage = wgpu::TextureUsage::StorageBinding | wgpu::TextureUsage::TextureBinding;
                    pawnAuraTexture_ = makeTexture("Pawn Aura (RGBA16Float)", desc);
                    if (!pawnAuraTexture_) return false;
                    pawnAuraWriteView_ = pawnAuraTexture_.CreateView();
                    pawnAuraReadView_ = pawnAuraTexture_.CreateView();
                }

                // Live card (RGBA16Float, LIVE_CARD_SIZE² — GROUND_CARD_1; the
                // writer kernel rewrites it on every live frame, render +
                // compute sample it)
                {
                    wgpu::TextureDescriptor desc{};
                    desc.size = { Dim::LIVE_CARD_SIZE, Dim::LIVE_CARD_SIZE, 1 };
                    desc.format = wgpu::TextureFormat::RGBA16Float;
                    desc.usage = wgpu::TextureUsage::StorageBinding | wgpu::TextureUsage::TextureBinding;
                    liveCardTexture_ = makeTexture("Live Card (RGBA16Float — GROUND_CARD_1)", desc);
                    if (!liveCardTexture_) return false;
                    liveCardWriteView_ = liveCardTexture_.CreateView();
                    liveCardView_ = liveCardTexture_.CreateView();
                }

                // Entity ground atlas (r32float 256×1 — compute writes ground_y, VS textureLoad)
                {
                    wgpu::TextureDescriptor desc{};
                    desc.size = { Dim::GROUND_ATLAS_WIDTH, 1, 1 };
                    desc.format = wgpu::TextureFormat::R32Float;
                    desc.usage = wgpu::TextureUsage::StorageBinding | wgpu::TextureUsage::TextureBinding;
                    desc.dimension = wgpu::TextureDimension::e2D;
                    desc.mipLevelCount = 1;
                    desc.sampleCount = 1;
                    entityGroundAtlasTexture_ = makeTexture("Entity Ground Atlas (r32float 256x1)", desc);
                    if (!entityGroundAtlasTexture_) return false;
                    entityGroundAtlasWriteView_ = entityGroundAtlasTexture_.CreateView();
                    entityGroundAtlasReadView_ = entityGroundAtlasTexture_.CreateView();
                }

                {
                    wgpu::TextureDescriptor desc{};
                    desc.size = { Dim::PATCH_HEIGHTFIELD_N, Dim::PATCH_HEIGHTFIELD_N, Dim::MAX_ACTIVE_PATCHES };
                    desc.dimension = wgpu::TextureDimension::e2D;
                    desc.format = wgpu::TextureFormat::RGBA16Float;
                    desc.usage = wgpu::TextureUsage::StorageBinding | wgpu::TextureUsage::TextureBinding;
                    patchHeightfieldArrayTexture_ = makeTexture("Patch Heightfield Array (225x65x65, RGBA16Float; 225 = Dim::MAX_ACTIVE_PATCHES)", desc);
                    if (!patchHeightfieldArrayTexture_) return false;

                    wgpu::TextureViewDescriptor viewDesc{};
                    viewDesc.dimension = wgpu::TextureViewDimension::e2DArray;
                    viewDesc.arrayLayerCount = Dim::MAX_ACTIVE_PATCHES;
                    viewDesc.label = "Patch Heightfield Array Write";
                    patchHeightfieldArrayWriteView_ = patchHeightfieldArrayTexture_.CreateView(&viewDesc);
                    viewDesc.label = "Patch Heightfield Array Read";
                    patchHeightfieldArrayReadView_ = patchHeightfieldArrayTexture_.CreateView(&viewDesc);
                }

                {
                    wgpu::TextureDescriptor desc{};
                    desc.size = { Dim::PATCH_CELL_N, Dim::PATCH_CELL_N, Dim::MAX_ACTIVE_PATCHES };
                    desc.dimension = wgpu::TextureDimension::e2D;
                    desc.format = wgpu::TextureFormat::RGBA8Unorm;
                    desc.usage = wgpu::TextureUsage::StorageBinding | wgpu::TextureUsage::TextureBinding;
                    patchCellColorArrayTexture_ = makeTexture("Patch Cell Color Array (225x16x16, RGBA8Unorm; 225 = Dim::MAX_ACTIVE_PATCHES)", desc);
                    if (!patchCellColorArrayTexture_) return false;

                    wgpu::TextureViewDescriptor viewDesc{};
                    viewDesc.dimension = wgpu::TextureViewDimension::e2DArray;
                    viewDesc.arrayLayerCount = Dim::MAX_ACTIVE_PATCHES;
                    viewDesc.label = "Patch Cell Color Array Write";
                    patchCellColorArrayWriteView_ = patchCellColorArrayTexture_.CreateView(&viewDesc);
                    viewDesc.label = "Patch Cell Color Array Read";
                    patchCellColorArrayReadView_ = patchCellColorArrayTexture_.CreateView(&viewDesc);
                }

                // Shadow map (Depth32Float). Sun depth outdoors — and the
                // spot atlas's FIRST texture indoors, holding lights 0-1 in
                // two half-width tiles while the sun is idle. Not
                // directional-only; render_shadow_pass picks it for li < 2.
                {
                    wgpu::TextureDescriptor desc{};
                    desc.size = { Dim::SHADOW_MAP_SIZE, Dim::SHADOW_MAP_SIZE, 1 };
                    desc.format = kShadowDepthFormat;   // FORMAT_1 D1
                    desc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
                    shadowMapTexture_ = makeTexture("Shadow Map", desc);
                    if (!shadowMapTexture_) return false;
                    shadowMapView_ = shadowMapTexture_.CreateView();
                }

                // Spot shadow atlas, SECOND texture (Depth32Float): lights
                // 2-3, one half-width full-height tile each. The tiling is
                // 1x2 per texture across TWO textures — this one and the sun
                // map — for 4 spot slots, NOT a 2x2 grid in one texture. (The
                // 2x2 grid is the retired scheme sample_spot_shadow_pcf's
                // banner names as "the old single-texture 2×2 grid".)
                {
                    wgpu::TextureDescriptor desc{};
                    desc.size = { Dim::SHADOW_MAP_SIZE, Dim::SHADOW_MAP_SIZE, 1 };
                    desc.format = kShadowDepthFormat;   // FORMAT_1 D1
                    desc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
                    spotShadowMapTexture_ = makeTexture("Spot Shadow Atlas", desc);
                    if (!spotShadowMapTexture_) return false;
                    spotShadowMapView_ = spotShadowMapTexture_.CreateView();
                }

                return true;
            }

            bool createSamplers() {
                {
                    wgpu::SamplerDescriptor desc{};
                    desc.label = "Bilinear Sampler (height field interpolation)";
                    desc.magFilter = wgpu::FilterMode::Linear;
                    desc.minFilter = wgpu::FilterMode::Linear;
                    desc.addressModeU = wgpu::AddressMode::ClampToEdge;
                    desc.addressModeV = wgpu::AddressMode::ClampToEdge;
                    bilinearSampler_ = device_.CreateSampler(&desc);
                    if (!bilinearSampler_) return false;
                }

                {
                    wgpu::SamplerDescriptor desc{};
                    desc.label = "Nearest Sampler (cell boundaries)";
                    desc.magFilter = wgpu::FilterMode::Nearest;
                    desc.minFilter = wgpu::FilterMode::Nearest;
                    desc.addressModeU = wgpu::AddressMode::ClampToEdge;
                    desc.addressModeV = wgpu::AddressMode::ClampToEdge;
                    nearestSampler_ = device_.CreateSampler(&desc);
                    if (!nearestSampler_) return false;
                }

                {
                    wgpu::SamplerDescriptor desc{};
                    desc.label = "Shadow Sampler (PCF comparison)";
                    desc.compare = wgpu::CompareFunction::Less;
                    desc.magFilter = wgpu::FilterMode::Linear;
                    desc.minFilter = wgpu::FilterMode::Linear;
                    desc.addressModeU = wgpu::AddressMode::ClampToEdge;
                    desc.addressModeV = wgpu::AddressMode::ClampToEdge;
                    shadowSampler_ = device_.CreateSampler(&desc);
                    if (!shadowSampler_) return false;
                }

                {
                    wgpu::SamplerDescriptor desc{};
                    desc.label = "Painting Sampler (bilinear, clamp)";
                    desc.magFilter = wgpu::FilterMode::Linear;
                    desc.minFilter = wgpu::FilterMode::Linear;
                    desc.addressModeU = wgpu::AddressMode::ClampToEdge;
                    desc.addressModeV = wgpu::AddressMode::ClampToEdge;
                    paintingSampler_ = device_.CreateSampler(&desc);
                    if (!paintingSampler_) return false;
                }

                return true;
            }

            #include "binding_surface.gen.inc"

            bool createBindGroups() {
                if (!create_binding_layouts()) return false;
                if (!create_binding_groups_boot()) return false;

                return true;
            }

            bool initializeState() {
                wgpu::Queue queue = device_.GetQueue();

                config_.mute_dynamics_0d = 0;
                config_.mute_signal = 0;
                config_.mute_couplings = Coupling::NONE;
                config_.pawn_speed = Idle::PAWN_SPEED;
                config_.point_host = 0;             // the pawn hosts (the kite)
                config_.point_fly_speed = 0.0f;     // 0 → WGSL PAWN_SPEED fallback (the panel authors it)
                config_.point_bubble_radius = POINT_BUBBLE_RADIUS;  // CONTACT_2: boot-pin the bubble from contracts/point.hpp (source of truth); rest 20.0
                config_.cube_plasticity = Idle::CUBE_PLASTICITY_DEFAULT;  // CONTACT_3 K2c: boot-pin the live λ master; rest 1.0 (CONTACT_5 P2b)
                // THE FLOATERS' SYNCHRONY, GIVEN A HOME. Until the desk asked
                // for a nonzero rest this row had no author at all — the
                // value-initialised `config_` was its only value, and 0 turns
                // the beacon emitter off outright (contracts/control_panel.hpp:
                // "strength is s * coord"). Two readers, one fact: the beacon's
                // strength gate and the cube-behaviour synchrony knob in
                // world.wgsl. Its dial is Interaction · Cubes.
                config_.floater_coordination = 0.11f;
                // FIELD_2b — the field's dials, boot-pinned from THE PANEL
                // (contracts/control_panel.hpp). One home authors; this is
                // the transport. The ribbon dialect reads the same names
                // directly, so the two rooms cannot drift.
                config_.field_slack         = FIELD_SLACK;
                config_.field_arch_slack    = FIELD_ARCH_SLACK;   // ATRIUM_7 — an arch leg's own shell
                // THE SUBTRACTION DIALS REST OPEN (PANORAMA_1): every draw
                // draws. A cleared bit is a measurement in progress, never a
                // shipped state, so the rest is the only value the audience
                // ever sees.
                config_.draw_mask           = DRAW_MASK_ALL;
                config_.shadow_mask         = SHADOW_MASK_ALL;
                // The kernel today, so 16 is the rest and 4 is the question.
                config_.shadow_pcf_taps     = 16u;
                config_.field_k             = FIELD_K;
                config_.field_fmax          = FIELD_FMAX;
                config_.field_occupier_gain = FIELD_OCCUPIER_GAIN;
                config_.field_authored_gain = FIELD_AUTHORED_GAIN;
                config_.field_gain_cube     = FIELD_GAIN_CUBE;
                config_.field_gain_sphere   = FIELD_GAIN_SPHERE;
                config_.field_gain_agent    = FIELD_GAIN_AGENT;
                // RIBBON_1 — the ribbon's dials, boot-pinned from THE PANEL
                // (contracts/ribbon_surface.hpp RIBBON_LIVE) by the same idiom
                // and at the same site. The head and the body are kernels: this
                // is the ONLY road from the panel to the flight.
                config_.ribbon_max_speed       = RIBBON_LIVE.max_speed;
                config_.ribbon_yaw_rate        = RIBBON_LIVE.yaw_rate;
                config_.ribbon_r_min           = RIBBON_LIVE.r_min;
                config_.ribbon_floor_margin    = RIBBON_LIVE.floor_margin;
                config_.ribbon_alt_smooth_dist = RIBBON_LIVE.alt_smooth_dist;
                config_.ribbon_alt_stiff       = RIBBON_LIVE.alt_stiff;
                config_.ribbon_climb_rate      = RIBBON_LIVE.climb_rate;
                config_.ribbon_mount_setback   = RIBBON_LIVE.mount_setback;
                config_.ribbon_lookahead       = RIBBON_LIVE.lookahead;
                config_.ribbon_clear_head      = RIBBON_LIVE.clear_head;
                config_.ribbon_clear_body      = RIBBON_LIVE.clear_body;
                config_.ribbon_hands_tau       = RIBBON_LIVE.sky_yaw_tau;
                // RIBBON_2 — the wander brain's dials, same road, same site.
                config_.ribbon_wander_soft     = RIBBON_LIVE.wander_soft;
                config_.ribbon_wander_yaw_max  = RIBBON_LIVE.wander_yaw_max;
                config_.ribbon_wander_arrive   = RIBBON_LIVE.wander_arrive;
                config_.ribbon_roam_radius     = RIBBON_LIVE.roam_radius;
                // KITE_1 — the witness's own dial, boot-pinned from the point's
                // house (contracts/point.hpp) by the POINT_BUBBLE_RADIUS idiom.
                config_.camera_chase_ff        = CAMERA_CHASE_FF;
                config_.camera_push_gain       = CAMERA_PUSH_GAIN;
                config_.camera_push_radius     = CAMERA_PUSH_RADIUS;
                config_.freeze_sphere = 0;
                config_.fpv_mode = 0;
                config_.world_seed = 42;
                config_.aura_enabled = 1.0f;
                config_.pawn_aura_height = 0.0f;
                // THE PALETTE MIRROR — rest = the pre-graduation WGSL
                // literals (bit-identical by construction).
                // Values authored at THE TERRAIN_LOOKS PANEL ROW 1
                // (surface/terrain_looks.hpp) — boot reads the panel.
                {
                    for (uint32_t i = 0; i < 4; i++) {
                        for (uint32_t c = 0; c < 3; c++) {
                            config_.palette_center[i][c] = terrain_looks::PALETTE_CENTER_REST[i][c];
                            config_.palette_light[i][c]  = terrain_looks::PALETTE_LIGHT_REST[i][c];
                        }
                        config_.palette_center[i][3] = 0.0f;
                        config_.palette_light[i][3]  = 0.0f;
                    }
                    for (uint32_t i = 0; i < 4; i++) {
                        config_.palette_weight[i] = terrain_looks::PALETTE_WEIGHT_REST[i];
                    }
                }
                // THE VEIL — chain defaults (Dim: ring 325 / icing 40 /
                // lod0 175); strength staged per frame by U5 (0 in
                // finite/indoor). Boot outdoor-on.
                config_.veil_ring   = Dim::VEIL_RING_DEFAULT;
                config_.veil_icing  = Dim::VEIL_ICING_DEFAULT;
                config_.veil_strength = 1.0f;
                config_.lod0_radius = Dim::LOD0_RADIUS_DEFAULT;
                config_.veil_dither = 0.0f;   // THE RIM: default = icing tints (mechanism 1 alone)
                // THE MOSAIC (MOSAIC_0/1) — trencadís dials, pinned at rest.
                // THE MOSAIC IS ON (MOSAIC_2). The probe's reason for a
                // runtime gate is discharged — the walk compiles on the
                // supported floor. (The compiler that first proved it was
                // FXC, retired at PIVOT_0; docs/FXC_LAWS_RECORD.md
                // §PROBATE.) The dial stays as the panel's switch and
                // METER_1's A/B, and it rests open. No key binds it: a key
                // is diagnostics, this is the design.
                config_.mosaic_enable        = 1.0f;
                config_.mosaic_shard_size    = Dim::MOSAIC_SHARD_SIZE_DEFAULT;
                config_.mosaic_passage_scale = Dim::MOSAIC_PASSAGE_DEFAULT;
                config_.mosaic_blend         = Dim::MOSAIC_BLEND_DEFAULT;
                config_.mosaic_facet         = Dim::MOSAIC_FACET_DEFAULT;
                config_.fog_density = 0.003f;
                config_.fog_color[0] = 0.85f;
                config_.fog_color[1] = 0.78f;
                config_.fog_color[2] = 0.72f;
                config_.fade_alpha = 0.0f;
                config_.fade_color[0] = 0.0f;
                config_.fade_color[1] = 0.0f;
                config_.fade_color[2] = 0.0f;
                config_.mode_gol_tick_scale = 1.0f;
                config_.mode_gol_height_scale = 1.0f;
                config_.pulse_count = 0;
                for (int i = 0; i < 32; i++) config_.pulse_data[i] = 0.0f;
                config_.possessed_slot = 0;  // player starts in slot 0
                config_.world_bound_min[0] = 0.0f;
                config_.world_bound_min[1] = 0.0f;
                config_.world_bound_max[0] = 0.0f;
                config_.world_bound_max[1] = 0.0f;
                config_.terrain_amp_ceiling = 0.0f;  // 0 = unlimited (outdoor default)
                config_.ceiling_height = 0.0f;       // 0 = no ceiling (outdoor default)
                config_.terrain_time = 0.0f;         // 0 = frozen terrain (default)
                // Band motion: -1 = inactive sentinel (default for all moods at boot)
                config_.band_blend_0 = -1.0f;
                config_.band_blend_1 = -1.0f;
                config_.band_blend_2 = -1.0f;
                config_.band_blend_3 = -1.0f;
                config_.band_blend_4 = -1.0f;
                config_.band_blend_5 = -1.0f;
                config_.band_phase_origin_0 = 0.0f;
                config_.band_phase_origin_1 = 0.0f;
                config_.band_phase_origin_2 = 0.0f;
                config_.band_phase_origin_3 = 0.0f;
                config_.band_phase_origin_4 = 0.0f;
                config_.band_phase_origin_5 = 0.0f;
                queue.WriteBuffer(configBuffer_, 0, &config_, sizeof(config_));

                // Agent buffer: zero-init all 32 slots, then populate slot 0
                // with the player at the idle pose. Slots 1..31 are mood-
                // populated by the Cartridge after mood application.
                {
                    GPUAgentState agents[Dim::MAX_AGENTS] = {};
                    auto& p = agents[0];
                    p.pos_x = Idle::PAWN_POS_X;
                    p.pos_y = Idle::PAWN_POS_Y;
                    p.pos_z = Idle::PAWN_POS_Z;
                    p.heading = Idle::PAWN_HEADING;
                    p.orient_x = 0.0f;
                    p.orient_y = 0.0f;
                    p.orient_z = 0.0f;
                    p.orient_w = 1.0f;
                    p.is_active = 1u;
                    p.behavior_id = 0u;   // AGENT_BEHAVIOR_PLAYER_CONTROLLED
                    p.tier_idx = 0u;   // AGENT_TIER_WORKER
                    p.portal_trigger = -1;
                    queue.WriteBuffer(agentStateBuffer_, 0, agents, sizeof(agents));
                }

                // pos is UNSEEDED: update_camera_vp composes it from
                // azimuth/elevation/distance in the same lane that then reads
                // it, on frame 1 and every frame after. A boot value
                // here would be a second author for a fact the orbit owns.
                GPUCameraState camera{};
                camera.azimuth = Idle::CAMERA_AZIMUTH;
                camera.elevation = Idle::CAMERA_ELEVATION;
                camera.distance = Idle::CAMERA_DISTANCE;
                camera.pan_x = 0.0f;
                camera.pan_y = 0.0f;
                // aim_point starts at the player's idle position so the
                // camera doesn't lerp from origin on first frames.
                camera.aim_point[0] = Idle::PAWN_POS_X;
                camera.aim_point[1] = Idle::PAWN_POS_Y;
                camera.aim_point[2] = Idle::PAWN_POS_Z;
                queue.WriteBuffer(cameraBuffer_, 0, &camera, sizeof(camera));

                // Zero-init all floating entity slots (inactive)
                {
                    std::vector<uint8_t> zeros(Dim::TOTAL_FLOATING_SLOTS * sizeof(GPUFloatingEntityState), 0);
                    queue.WriteBuffer(floatingEntityBuffer_, 0, zeros.data(), zeros.size());
                }

                // Default state — overwritten by spawner
                GPURibbonState ribbon{};
                ribbon.anchor[0] = 0.0f;
                ribbon.anchor[1] = 0.0f;
                ribbon.anchor[2] = 0.0f;
                ribbon.time = 0.0f;
                ribbon.cube_count = 200;
                ribbon.cube_size = 3.0f;
                ribbon.height = 40.0f;
                ribbon.color[0] = 0.85f;
                ribbon.color[1] = 0.12f;
                ribbon.color[2] = 0.08f;
                // Default (hidden) ribbon — set trail-frame fields directly
                // (bypasses commit; is_visible=0 so values are placeholders).
                ribbon.lateral_amp = 2.7f;
                ribbon.lateral_freq = 1.1f;
                ribbon.vertical_amp = 1.7f;
                ribbon.vertical_freq = 1.2f;
                ribbon.propagation_speed = 40.0f;  // placeholder (hidden ribbon, never drawn)
                ribbon.is_visible = 0u;  // hidden until spawning system activates one
                queue.WriteBuffer(ribbonBuffer_, 0, &ribbon, sizeof(ribbon));

                // Painting slots — all inactive initially
                {
                    GPUPaintingSlot emptySlots[Dim::PAINTING_MAX_SLOTS]{};
                    for (uint32_t i = 0; i < Dim::PAINTING_MAX_SLOTS; i++) {
                        emptySlots[i].is_active = 0;
                    }
                    queue.WriteBuffer(paintingSlotsBuffer_, 0, emptySlots,
                        sizeof(GPUPaintingSlot) * Dim::PAINTING_MAX_SLOTS);
                }
                {
                    GPUVPMatrix zeroVP{};
                    queue.WriteBuffer(photographerVPBuffer_, 0, &zeroVP, sizeof(GPUVPMatrix));
                }

                return true;
            }
        };

    } // namespace the_board
} // namespace t7
