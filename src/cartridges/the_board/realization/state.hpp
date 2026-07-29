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
//                        groups; grown per L5/L4 (src/docs/LAWS.md)
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

#include "analysis/analysis_signal.hpp"
#include "cartridges/the_board/demos/demo.hpp"   // ROSTER via the selected sentence (GPUState::init gates creation)
#include "cartridges/the_board/realization/binding_registry.hpp"  // C6: bind::g0::* / bind::g1::* — the single source of truth for binding NUMBERS (the layout+group pair references one named const)
#include "cartridges/the_board/surface/terrain_looks.hpp"          // THE TERRAIN_LOOKS PANEL (C++ room): palette quartet REST + motion/mode rest pins — boot init reads the panel
#include "cartridges/the_board/bodies/pawn_figures.hpp"            // typed figure registry (H1: PawnFigureDef / PAWN_FIGURES) — constexpr-only, self-contained
#include "cartridges/the_board/contracts/point.hpp"                 // POINT_BUBBLE_RADIUS — source of truth for the CONTACT_2 boot pin
#include <webgpu/webgpu_cpp.h>
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
            constexpr uint32_t RIBBON_VERTEX_COUNT = (RIBBON_MAX_RINGS - 1) * RIBBON_TUBE_VERTS_PER_SEG + RIBBON_CAP_VERTS;


            // Patch streaming system
            constexpr float    PATCH_EXTENT = 50.0f;    // world units per patch side
            constexpr uint32_t PATCH_HEIGHTFIELD_N = 256;     // texels per patch heightfield side
            constexpr uint32_t PATCH_CELL_N = 16;      // cell color texture side per patch
            // THE CELL — one spelling, and this is it. The pawn aura, the GoL
            // zone extent + corner snap, the card's window origin and the
            // cell-exactness assert below all read THIS name.
            // L3 MIRROR: world.wgsl PATCH_CELL_SIZE. Change both rooms together.
            constexpr float    PATCH_CELL_SIZE = PATCH_EXTENT / (float)PATCH_CELL_N;  // 3.125
            constexpr uint32_t PATCH_GRID_RADIUS = 3;       // inner priority radius (7×7)
            constexpr uint32_t PATCH_GRID_SIDE = 2 * PATCH_GRID_RADIUS + 1;       // 7
            constexpr uint32_t PATCH_PREGEN_RADIUS = 8;                                // deep pre-gen buffer (17×17, 400 world units)
            constexpr uint32_t PATCH_PREGEN_SIDE = 2 * PATCH_PREGEN_RADIUS + 1;     // 17
            constexpr uint32_t MAX_ACTIVE_PATCHES = PATCH_PREGEN_SIDE * PATCH_PREGEN_SIDE; // 289

            // ── THE LIVE CARD (GROUND_CARD_1) ──
            // One 2D RGBA16F field over the ground window, point-centered,
            // fully rewritten per frame. R = waves+pulses Δh; G/B = wave ∂x/∂z
            // (waves-only this campaign — pulse shading is Stage 6); A = raw
            // GoL lift. Window ORIGIN SNAPS to the PATCH_CELL_SIZE grid so a
            // nearest fetch of .a is cell-exact.
            // L3 MIRROR: world.wgsl LIVE_CARD_SIZE / LIVE_CARD_EXTENT — same
            // names, same values, both rooms change together.
            constexpr uint32_t LIVE_CARD_SIZE   = 640;
            constexpr float    LIVE_CARD_EXTENT = 1000.0f;

            // THE WINDOW COVENANT, stated with the numbers that hold.
            // The origin snaps DOWN to the PATCH_CELL_SIZE grid, so the
            // window is not exactly centred: the guaranteed half-extent is
            //   +x/+z : EXTENT/2 − PATCH_CELL_SIZE = 496.875 wu
            //   −x/−z : EXTENT/2                   = 500.0   wu
            // What it must cover is NOT the EXIST ring. Agents and floaters
            // are EXIST-gated at 350, but flora, columns, arches and outdoor
            // paintings are PATCH-lifetime: they live as long as their host
            // patch, out to the allocation window, which reaches 450 wu per
            // axis (patches span [pawnGX−8, pawnGX+8] and the pawn sits
            // anywhere inside its own cell), 453.125 with the snap.
            // Slack 43.75 wu. Outside the window a sample returns the
            // ClampToEdge texel — sample_live_card does no bounds test.

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

            // The two writer dispatches divide the card side by 8 (pass 1,
            // write_live_card_heights) and 16 (pass 2, write_live_card_resolve).
            // Neither kernel guards a remainder, so a size that is not a
            // multiple of BOTH silently under-dispatches and leaves a strip of
            // the card never written.
            static_assert(LIVE_CARD_SIZE % 8u == 0u && LIVE_CARD_SIZE % 16u == 0u,
                "live card: SIZE must divide by both writer workgroup sizes "
                "(8 = write_live_card_heights, 16 = write_live_card_resolve)");

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
            //   EXIST (350; the pregen ring now reaches 400 — the
            //     deepened buffer): existence eviction (agents unified at
            //     350 — V1; floaters 400, the flagged spawn-headroom fork).
            // LIVE values ride config (veil_ring/veil_icing/lod0_radius,
            // tunable — "ring at 6.5 feels right vs 5.5, config-tune live").
            constexpr float LOD0_RADIUS_DEFAULT = 3.5f * PATCH_EXTENT;   // 175
            constexpr float VEIL_RING_DEFAULT   = 6.5f * PATCH_EXTENT;   // 325 — THE RING
            constexpr float VEIL_ICING_DEFAULT  = 40.0f;                 // δ (~25-50, tunable)
            constexpr float EXIST_RADIUS        = 7.0f * PATCH_EXTENT;   // 350
            static_assert(PATCH_PREGEN_RADIUS * PATCH_EXTENT >= EXIST_RADIUS,
                "VEIL CHAIN: PREGEN >= EXIST (nothing exists off resident ground)");
            static_assert(EXIST_RADIUS > VEIL_RING_DEFAULT,
                "VEIL CHAIN: EXIST > RING (existence outlives the draw set)");
            static_assert(VEIL_RING_DEFAULT > LOD0_RADIUS_DEFAULT,
                "VEIL CHAIN: RING > LOD0 (the draw set contains the full-mesh core)");
            static_assert(VEIL_RING_DEFAULT - VEIL_ICING_DEFAULT > LOD0_RADIUS_DEFAULT,
                "VEIL CHAIN: the icing band sits wholly outside the LOD0 core");
            constexpr uint32_t PATCH_MESH_N = 64;      // mesh subdivisions per patch (LOD-0)
            constexpr uint32_t PATCH_INDEX_COUNT = PATCH_MESH_N * PATCH_MESH_N * 6;
            constexpr uint32_t PATCH_MESH_N_LOD1 = 32;  // LOD-1: half resolution
            constexpr uint32_t PATCH_INDEX_COUNT_LOD1 = PATCH_MESH_N_LOD1 * PATCH_MESH_N_LOD1 * 6;

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
            constexpr float    FOG_COLOR_R = 0.85f;
            constexpr float    FOG_COLOR_G = 0.78f;
            constexpr float    FOG_COLOR_B = 0.72f;

            // Lighting
            // TWIN: world.wgsl `const SHADOW_MAP_SIZE: f32` (— Shadow
            // constants). The WGSL twin feeds BOTH PCF texel_size reads.
            // Change it in BOTH rooms or the PCF world-footprint silently
            // rescales while the texture resizes. (L3 MIRROR — the
            // TILE_GRID_CAPACITY pattern; no compile-time bridge spans the
            // runtime-loaded seam.)
            constexpr uint32_t SHADOW_MAP_SIZE = 4096;
            constexpr uint32_t MAX_POINT_LIGHTS = 8;

            // Indoor shell (ceiling + walls for finite indoor scenes)
            constexpr uint32_t SHELL_MAX_VERTICES = 2048;
            constexpr uint32_t SHELL_MAX_INDICES = 8192;

            // Painting system: staging + exhibition
            constexpr uint32_t PAINTING_MAX_SLOTS = 32;       // max exhibited paintings
            constexpr uint32_t PAINTING_RESOLUTION = 1024;
            constexpr uint32_t STAGING_LAYERS = 16;            // per staging array (snapshot + authored)
            constexpr uint32_t EXHIBITION_LAYERS = 32;         // exhibition array
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
            // 256 slots: drone-show populations. update_cube is single-thread
            // (@workgroup_size(1)) — cost scales linearly, ~7.5K ops/frame.
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
            constexpr float PAWN_HEADING = 0.0f;
            constexpr float CAMERA_POS_X = 0.0f;
            constexpr float CAMERA_POS_Y = 15.0f;
            constexpr float CAMERA_POS_Z = 30.0f;
            constexpr float CAMERA_AZIMUTH = 0.0f;
            constexpr float CAMERA_ELEVATION = 0.4f;
            constexpr float CAMERA_DISTANCE = 30.0f;
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
            constexpr uint32_t POLYPHONY_TO_SPHERE_COLOR = 1u << 12;
            constexpr uint32_t TERRAIN_TO_SPHERE_HEIGHT  = 1u << 14;
            constexpr uint32_t PAWN_TO_SUN_VP            = 1u << 16;
            constexpr uint32_t ALL = 0x1FFFFFu;
            constexpr uint32_t NONE = 0u;
            constexpr uint32_t DERIVED = TERRAIN_TO_PAWN_Y | TERRAIN_TO_PAWN_TILT | PAWN_TO_CAMERA_TARGET | TERRAIN_TO_SPHERE_HEIGHT | PAWN_TO_SUN_VP;
            constexpr uint32_t ACCUMULATED = INPUT_MOVES_PAWN | INPUT_ORBITS_CAMERA | INPUT_ZOOMS_CAMERA;
            constexpr uint32_t TERRAIN = TERRAIN_TO_PAWN_Y | TERRAIN_TO_PAWN_TILT | TERRAIN_TO_SPHERE_HEIGHT;
            constexpr uint32_t INPUT = INPUT_MOVES_PAWN | INPUT_ORBITS_CAMERA | INPUT_ZOOMS_CAMERA;
            constexpr uint32_t CAMERA = PAWN_TO_CAMERA_TARGET | INPUT_ORBITS_CAMERA | INPUT_ZOOMS_CAMERA;
            constexpr uint32_t SPHERE_APPEARANCE = POLYPHONY_TO_SPHERE_COLOR;
        }

        // =====================================================================
        // S4 GPU STRUCTURES — Byte-aligned data contracts (must match WGSL)
        // =====================================================================

        struct alignas(16) GPUFrameSignal {
            float t_seconds;
            float t_beats;
            float dt;
            float aspect_ratio;
            std::array<float, 64> stats;
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
            // Sky block: ribbon head POSE handed to the GPU pawn update so the
            // possessed pawn snaps onto the flown head. MODE routing lives on
            // the host machine (config.point_host, RESIDUE_3) — the sky_mode
            // word below has no GPU reader; it survives as the block's leading
            // word (the drain/resync boundary) under this batch's layout law,
            // written from the host fact.
            uint32_t sky_mode;       // consumer-free; 1 while the RIBBON hosts (debug mirror of the host)
            float    sky_head_x;
            float    sky_head_y;
            float    sky_head_z;
            float    sky_heading;
            float    sky_yaw_off;    // tangent-align yaw deflection (rad)
            float    sky_pitch;      // tangent-align pitch (rad)
            float    sky_roll;       // bank into the lateral swing (rad, clamped)
        };

        // Field ORDER is the cross-room contract — world.wgsl's
        // DesignConfig mirrors this struct field-for-field. Adding a
        // knob: THE GROWTH LAW, L5 in src/docs/LAWS.md. Choosing where
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
            uint32_t point_host;              // 0 = pawn (the kite), 1 = camera (free-fly)
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
            float point_bubble_radius;     // CONTACT_2 C3a: the point's bounded awareness (rest 20; boot-pinned from contracts/point.hpp POINT_BUBBLE_RADIUS). Fills the checker tail pad — sizeof unchanged.
            float cube_plasticity;         // CONTACT_3 K2c: global λ master (rest 0.6; boot-pinned from Idle::CUBE_PLASTICITY_DEFAULT). Also fills the checker tail pad — sizeof unchanged.
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
        };

        struct alignas(16) GPUTileGridEntry {
            float amp_scale;
            float height_bias;
            float activation_scale;
            uint32_t archetype;        // 0=mountainous, 1=varied, 2=basin, 3=pool
        };
        static_assert(sizeof(GPUTileGridEntry) == 16, "GPUTileGridEntry must be 16 bytes");

        static constexpr uint32_t TILE_GRID_SIDE = 2 * (Dim::PATCH_PREGEN_RADIUS + 1) + 1;  // 19 (pregen + 1 pad each side)
        static constexpr uint32_t TILE_GRID_MAX = TILE_GRID_SIDE * TILE_GRID_SIDE;  // 361
        static_assert(TILE_GRID_MAX <= Dim::TILE_GRID_CAPACITY,
            "TILE_GRID ceiling exceeded: raise TILE_GRID_CAPACITY in BOTH "
            "rooms (state.hpp Dim + world.wgsl) — the pair is pinned, not "
            "derived");
        struct alignas(16) GPUTileGrid {
            int32_t origin_x;          // grid-space X of entry [0][0]
            int32_t origin_z;          // grid-space Z of entry [0][0]
            uint32_t side;             // grid dimension (19)
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
            float home_y;          // 36
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
            float _pad;            // 28 — pad to 32 bytes
        };                         // 32 total (16-byte aligned)

        // Counts mirror the AGENT_BEHAVIOR_COUNT / AGENT_TIER_COUNT
        // enums in bodies/agents.hpp. Kept here so state.hpp can size
        // its registry buffers and bind-group entries without depending
        // on bodies/agents.hpp (which is included after state.hpp, at
        // file scope in the cartridge cohort). Asserts in
        // bodies/agents.hpp verify these stay in sync.
        static constexpr uint32_t GPU_AGENT_BEHAVIOR_COUNT = 10;
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
            uint32_t is_roaming;                                                // 80 (0 = stationary spine = today; 1 = head roams, wired stage 1b)
            float _pad1;                                                        // 84
            float _pad2;                                                        // 88
            float _pad3;                                                        // 92
            float color_b[3];                                                   // 96 — second checker median (CONTRAST)
            float hue_spread;                                                   // 108 — radians; per-cell hue rotation amplitude (CONTRAST skin; 0 = CB-1 look)
        };                                                                      // 112 total (mirrors world.wgsl RibbonState)

        // Pre-computed per-ring transform (compute_ribbon_rings output;
        // ribbon_vs + shadow_ribbon_vs input via render_ring_xforms)
        struct alignas(16) GPURibbonRingTransform {
            float motor_p0[4];         // PGA motor rotor part        (16)
            float motor_p1[4];         // PGA motor translator part   (16)
            float center[3];           // ring world-space center     (12)
            float _pad0;               // ( 4) = 48 — explicit padding. Was
                                       // `terrain_y`, always 0.0 since ribbons
                                       // stopped following terrain: two writers
                                       // in world.wgsl, zero readers anywhere.
                                       // The 48 bytes are REQUIRED — this is the
                                       // array stride of a storage buffer the VS
                                       // indexes (bindings 121 / 361), and the
                                       // readback sizes off sizeof(). The shader
                                       // still zeroes it so unused slots read
                                       // clean; that write is now honest about
                                       // what it is zeroing.
        };
        static_assert(sizeof(GPURibbonRingTransform) == 48, "GPURibbonRingTransform must be 48 bytes");

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
        };
        static_assert(sizeof(GPUArchMeshParams) == 64,
            "GPUArchMeshParams must be 64 bytes — keep in sync with world.wgsl::ArchMeshParams");

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
            float _pad128[3];           // pad to 128 bytes
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
            float spring_stiffness;
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
            float _zpad0;
            float _zpad1;
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
            uint32_t _pad0;
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
            uint32_t effect_mask;      // bitfield: bit 0=color, bit 1=height (future)
            uint32_t aura_n;           // grid side (64)
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

        struct alignas(16) GPUPatchParams {
            float origin[2];           // world-space XZ of patch origin
            float extent;              // patch side length in world units
            uint32_t resolution;       // texels per side (e.g. 256)
            uint32_t master_seed;      // master seed for terrain generation
            float time;                // for animated waves (0 for static generation)
            uint32_t layer;            // which layer of the heightfield array to write
            float _pad1;
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

        static_assert(sizeof(GPUFrameSignal) == 336, "GPUFrameSignal must be 336 bytes");
        static_assert(sizeof(GPUDesignConfig) == 560,
            "GPUDesignConfig must be 560 bytes. PRUNING_1 P3 removed nine "
            "zero-read fields (44 B) and added 12 B of DECLARED PAD: WGSL "
            "aligns vec3 to 16 while C++ packs float[3] at 4, and dropping "
            "44 B moved all four vec3 members off their boundaries. "
            "592 - 44 + 12 = 560. The pads ARE the mirror, not waste — the "
            "offsetof asserts below are what prove it.");
        // THE ALIGNMENT LAW (L4, src/docs/LAWS.md). These four are the only
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
            uint32_t _pad;            // 28
        };
        struct alignas(16) GPUPortalArray {
            uint32_t count;
            uint32_t _pad[3];
            GPUPortalEntry portals[MAX_GPU_PORTALS];
        };
        static_assert(sizeof(GPUPortalArray) == 16 + MAX_GPU_PORTALS * 32,
            "GPUPortalArray layout check");
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
        static_assert(sizeof(GPUPatchParams) == 32, "GPUPatchParams must be 32 bytes");
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
            inline constexpr uint32_t StreamPatches       = 2;
            inline constexpr uint32_t EntityMeshGen       = 6;
            inline constexpr uint32_t LiveCardWrite       = 8;
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

        class GPUState {

            wgpu::Device device_;
            GPUDesignConfig config_{};
            bool configDirty_ = true;      // true at boot → first frame always uploads
            bool configDynamic_ = false;   // mood override: true = upload every frame
            wgpu::TextureFormat colorFormat_ = wgpu::TextureFormat::BGRA8Unorm;  // set in initOffscreenResources

            wgpu::Buffer signalBuffer_, configBuffer_;
            // Agent system — unified entity buffer. Slot 0 is the player's
            // body; slots 1..MAX_AGENTS-1 are mood-authored agents.
            wgpu::Buffer agentStateBuffer_;
            wgpu::Buffer agentStateReadbackStaging_;
            wgpu::Buffer floatingEntityReadbackStaging_;
            wgpu::Buffer cameraStateReadbackStaging_;   // the point readback (camera-host only)
            // Agent registries — uploaded once at world-init from the C++
            // AGENT_BEHAVIORS / AGENT_TIER_GAINS tables. The single source
            // of truth lives in bodies/agents.hpp; the GPU side reads
            // these buffers via storage bindings 110 / 111.
            wgpu::Buffer agentBehaviorsBuffer_;
            wgpu::Buffer agentTierGainsBuffer_;
            wgpu::Buffer figureProfilesBuffer_;   // GPUPawnFigure[PAWN_FIGURE_COUNT] — uniform, render VS only (H2)
            wgpu::Buffer cameraBuffer_, floatingEntityBuffer_;
            wgpu::Buffer ribbonBuffer_;
            wgpu::Buffer ringTransformsBuffer_;
            wgpu::Buffer headPosesBuffer_;  // ribbon body poses — written via upload_ribbon_head_poses (the head mover lives in bodies/ribbon.hpp); read by ribbon_centerline_at
            // (bindings 21, 40 reserved — formerly proximity_field, cell_states)
            wgpu::Buffer vpBuffer_;
            wgpu::Buffer directionalLightBuffer_;
            wgpu::Buffer pointLightsBuffer_;
            wgpu::Buffer spotLightArrayBuffer_;
            wgpu::Buffer spotVPStagingBuffer_;   // 4 × 64 bytes: pre-staged per-light VPs for atlas copy
            wgpu::Buffer portalArrayBuffer_;
            wgpu::Buffer ribbonReadbackStaging_; // ring transform readback (diagnostic)

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

            wgpu::Buffer patchParamsBuffer_;
            wgpu::Buffer patchStagingBuffer_;    // N×GPUPatchParams for batched generation
            wgpu::Buffer patchInstancesBuffer_;
            wgpu::Buffer patchGridBuffer_;         // GPUPatchGrid — O(1) spatial index for sample_terrain_y_at
            wgpu::Buffer patchHeightScratchBuffer_;  // 256×256×2 floats (height+complexity) for two-pass heightfield gen
            wgpu::Buffer patchIndexBuffer_;
            wgpu::Buffer patchIndexBufferLOD1_;   // half-res index buffer for distant patches
            wgpu::Buffer tileGridBuffer_;
            uint32_t patchIndexCount_ = 0;
            uint32_t patchIndexCountLOD1_ = 0;

            // Patch heightfield texture array (289 layers × 256×256, RGBA16Float)
            wgpu::Texture patchHeightfieldArrayTexture_;
            wgpu::TextureView patchHeightfieldArrayWriteView_;  // full array for storage write
            wgpu::TextureView patchHeightfieldArrayReadView_;   // full array for sampling

            // Patch cell color texture array (289 layers × 16×16, RGBA8Unorm)
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

            wgpu::BindGroupLayout archMeshGenLayout_;    // bindings 193-195
            wgpu::BindGroupLayout columnMeshGenLayout_;  // bindings 196-198
            wgpu::BindGroupLayout agentOccupierLayout_;  // THE AGENTS' ROOM (group 2): g2:0-1
            wgpu::BindGroupLayout palmMeshGenLayout_;    // bindings 180-182
            wgpu::BindGroupLayout cactusMeshGenLayout_;  // bindings 183-185
            wgpu::BindGroupLayout bladeMeshGenLayout_;   // bindings 186-188
            wgpu::BindGroup archMeshGenBindGroup_;
            wgpu::BindGroup columnMeshGenBindGroup_;
            wgpu::BindGroup palmMeshGenBindGroup_;
            wgpu::BindGroup cactusMeshGenBindGroup_;
            wgpu::BindGroup bladeMeshGenBindGroup_;
            wgpu::BindGroup agentOccupierBindGroup_;     // THE AGENTS' ROOM (group 2)

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
            wgpu::BindGroupLayout pawnAuraComputeLayout_;
            wgpu::BindGroup pawnAuraComputeGroup_;

            // Live card (GROUND_CARD_1) — 512×512 RGBA16Float deformation field
            wgpu::Texture liveCardTexture_;         // compute writes, VS/FS/compute read
            wgpu::TextureView liveCardWriteView_;   // storage texture write (writer kernel)
            wgpu::TextureView liveCardView_;        // sampled read (render + compute)
            wgpu::Buffer liveCardScratchBuffer_;    // LIVE_CARD_SIZE² × 2 floats (Δh + gol) — two-pass writer scratch (TRUEBAND_CONTACT_1)
            wgpu::BindGroupLayout liveCardWriterLayout_;
            wgpu::BindGroup liveCardWriterGroup_;
            wgpu::BindGroupLayout zoneMaskLayout_;      // UNIFIED_GROUND_1 U5
            wgpu::BindGroup zoneMaskGroup_;

            // ── Orb sky layer ────────────────────────────────────────
            wgpu::Buffer orbStateBuffer_;          // MAX_ORBS × GPUOrbState (storage, read_write)
            wgpu::Buffer orbStatePrevBuffer_;      // MAX_ORBS × GPUOrbState (snapshot for flocking)
            wgpu::Buffer orbConfigBuffer_;         // GPUOrbConfig (uniform, per-frame)
            wgpu::Buffer orbQuadVB_;               // 4 vertices: billboard quad
            wgpu::Buffer orbQuadIB_;               // 6 indices: two triangles
            wgpu::BindGroupLayout orbComputeLayout_;  // dynamics/init/recolor: orb_state RW, prev RO
            wgpu::BindGroup orbComputeGroup_;
            wgpu::BindGroupLayout orbCopyLayout_;     // copy-prev: orb_state RO, prev RW
            wgpu::BindGroup orbCopyGroup_;

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

            wgpu::BindGroupLayout computeEntityBindGroupLayout_, renderEntityBindGroupLayout_;
            wgpu::BindGroupLayout renderTextureBindGroupLayout_, shadowTextureBindGroupLayout_;
            // computeTextureBindGroupLayout_ — Group 1 for compute shaders that
            // call query_ground_flyer / query_ground_walker. Provides
            // nearest_sampler + pawn_aura_read (live-contributor path for
            // sample_pawn_aura). Re-added for sphere/cube compute migration
            // to POLICY_FLYER; 0D compute shaders that stay on the baked
            // heightfield (camera clamps) do not bind this group.
            wgpu::BindGroupLayout computeTextureBindGroupLayout_;
            wgpu::BindGroupLayout meshGenEntityBindGroupLayout_;  // binding 1 only — still used by fade overlay
            wgpu::BindGroupLayout patchGenLayout_;
            wgpu::BindGroupLayout ribbonComputeLayout_;

            wgpu::BindGroup computeEntityBindGroup_, renderEntityBindGroup_;
            wgpu::BindGroup renderTextureBindGroup_, shadowTextureBindGroup_;
            wgpu::BindGroup computeTextureBindGroup_;  // live-contributor textures for flyer/walker compute
            wgpu::BindGroup meshGenEntityBindGroup_;  // still used by fade overlay
            wgpu::BindGroup patchGenBindGroup_;
            wgpu::BindGroup ribbonComputeBindGroup_;

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

            wgpu::BindGroupLayout galleryEntityBindGroupLayout_;
            wgpu::BindGroupLayout galleryTextureBindGroupLayout_;
            wgpu::BindGroupLayout photographerComputeLayout_;
            wgpu::BindGroupLayout entityPlacementComputeLayout_;
            wgpu::BindGroup galleryEntityBindGroup_;
            wgpu::BindGroup galleryTextureBindGroup_;
            wgpu::BindGroup photographerComputeBindGroup_;
            wgpu::BindGroup entityPlacementComputeBindGroup_;

            // GPU frustum culling — LOD0 only (Dawn D3D12 limit: one indirect draw per pass).
            // LOD1 always uses direct DrawIndexed; CPU computes its count.
            wgpu::Buffer frustumIndirectLOD0_;            // Indirect|CopyDst — DrawIndexedIndirect target
            wgpu::Buffer frustumComputeBuffer_;           // Storage|CopySrc|CopyDst — compute writes here
            wgpu::Buffer probeNullIndirectBuffer_;        // PROBE C3 (HELD) — zeroed 5×u32, null indirect args
            wgpu::Buffer visiblePatchIndicesBuffer_;      // MAX_ACTIVE_PATCHES × u32 — LOD0 visible list
            wgpu::BindGroupLayout frustumCullLayout_;
            wgpu::BindGroup frustumCullBindGroup_;

            // GoL zone compute (dedicated layout: bindings 160-162, 167-169)
            wgpu::BindGroupLayout zoneGolComputeLayout_;
            wgpu::BindGroup zoneGolComputeBindGroup_;

            wgpu::BindGroup photographerRenderEntityBindGroup_;
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
            // offsetof field-writers (resync_sky_head, the config sub-writers,
            // upload_ribbon_time/color/wave_amps, upload_*_frame, zone header/life).
            // A third tempo — left bespoke, offsetof asserts undisturbed.
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

            // E-3 (mechanized): the eight contiguous sky_* words — the TRAILING
            // 32 bytes of GPUFrameSignal — have ONE author, resync_sky_head, so
            // they are NOT part of the whole-signal drain. The drain uploads
            // everything UP TO the sky block; the sky block's sole per-frame
            // writer is R7 (the ribbon tick), with a boot-neutral covering the
            // ribbon-off case. No neutral-then-overwrite relay, no cross-function
            // submission-order dance: the drain and the sky author write DISJOINT
            // regions of the buffer.
            void upload_signal(wgpu::Queue& queue, const GPUFrameSignal& signal) {
                static_assert(offsetof(GPUFrameSignal, sky_mode) == 304,
                    "sky_* must be the trailing 32 bytes for the split (sky-less) signal drain");
                queue.WriteBuffer(signalBuffer_, 0, &signal, offsetof(GPUFrameSignal, sky_mode));
            }

            // Re-write only the sky_* block of the frame signal. Used to re-sync the
            // pawn mount after advance_ribbon_head so the pawn is sampled at the same
            // frame as the ribbon it rides (removes the one-frame mount lag). The eight
            // sky_* words are contiguous in GPUFrameSignal — a targeted sub-range write,
            // the same idiom as upload_ribbon_time. The last three words carry the
            // saddle's frame angles. The local block mirrors that field
            // order exactly; the static_assert guards the 8-word size.
            void resync_sky_head(wgpu::Queue& queue, uint32_t sky_mode,
                                 float head_x, float head_y, float head_z, float heading,
                                 float yaw_off, float pitch, float roll) {
                struct SkyBlock {
                    uint32_t sky_mode;
                    float head_x, head_y, head_z, heading;
                    float yaw_off, pitch, roll;
                } block{ sky_mode, head_x, head_y, head_z, heading, yaw_off, pitch, roll };
                static_assert(sizeof(SkyBlock) == 32,
                    "SkyBlock must mirror GPUFrameSignal's eight contiguous sky_* words");
                queue.WriteBuffer(signalBuffer_, offsetof(GPUFrameSignal, sky_mode),
                                  &block, sizeof(block));
            }

            // Upload the agent behavior + tier registries to the GPU.
            // Called once at world-init from the cartridge — values are
            // constexpr-equivalent (sourced from AGENT_BEHAVIORS /
            // AGENT_TIER_GAINS in bodies/agents.hpp) and never change
            // during a session. Source data is passed as raw pointers
            // because the C++ tables are defined inside the cartridge
            // class scope and aren't visible from state.hpp; the cartridge
            // has both a translation step (CPU table → GPU struct) and
            // the queue access, so it owns the call site.
            void upload_agent_registries(wgpu::Queue& queue,
                const GPUAgentBehaviorDef* behaviors,
                uint32_t behavior_count,
                const GPUAgentTierDef* tiers,
                uint32_t tier_count) {
                writeArray(queue, agentBehaviorsBuffer_, behaviors, behavior_count);
                writeArray(queue, agentTierGainsBuffer_, tiers, tier_count);
            }

            // One-shot upload of the pawn figure table. Packs PAWN_FIGURES →
            // GPUPawnFigure[PAWN_FIGURE_COUNT] → buffer. Called once at world-init
            // from upload_agent_registries_to_gpu (bodies/agents.hpp).
            void upload_pawn_figures(wgpu::Queue& queue) {
                GPUPawnFigure figs[PAWN_FIGURE_COUNT];
                pack_pawn_figures(figs);
                queue.WriteBuffer(figureProfilesBuffer_, 0, figs, sizeof(figs));
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

            void upload_directional_light(wgpu::Queue& queue, const GPUDirectionalLight& light) {
                writeStruct(queue, directionalLightBuffer_, light);
            }

            void upload_point_lights(wgpu::Queue& queue, const GPUPointLightArray& lights) {
                writeStruct(queue, pointLightsBuffer_, lights);
            }

            void upload_spot_lights(wgpu::Queue& queue, const GPUSpotLightArray& arr) {
                writeStruct(queue, spotLightArrayBuffer_, arr);
            }

            // Stage all active spot light VPs into the staging buffer (4 × 64 bytes).
            // Caller then encodes CopyBufferToBuffer per light before each shadow sub-pass.
            void stage_spot_vps(wgpu::Queue& queue, const GPUSpotLightArray& arr) {
                for (uint32_t i = 0; i < std::min(arr.count, MAX_SPOT_LIGHTS); i++) {
                    queue.WriteBuffer(spotVPStagingBuffer_, i * 64,
                        arr.lights[i].view_proj, 64);
                }
            }

            wgpu::Buffer spot_vp_staging() const { return spotVPStagingBuffer_; }
            wgpu::Buffer vp_buffer() const { return vpBuffer_; }
            static constexpr size_t light_vp_offset() { return offsetof(GPUVPMatrix, light_vp); }
            static constexpr size_t light_vp_size() { return 16 * sizeof(float); }

            void upload_patch_params(wgpu::Queue& queue, const GPUPatchParams& params) {
                writeStruct(queue, patchParamsBuffer_, params);
            }

            void upload_tile_grid(wgpu::Queue& queue, const GPUTileGrid& grid) {
                writeStruct(queue, tileGridBuffer_, grid);
            }

            void upload_patch_instances(wgpu::Queue& queue, const GPUPatchInstance* instances, uint32_t count) {
                writeArray(queue, patchInstancesBuffer_, instances, count);
            }

            void upload_patch_grid(wgpu::Queue& queue, const GPUPatchGrid& grid) {
                writeStruct(queue, patchGridBuffer_, grid);
            }

            void upload_ribbon_time(wgpu::Queue& queue, float time) {
                // Only update the time field (offset 12 = after anchor[3])
                queue.WriteBuffer(ribbonBuffer_, offsetof(GPURibbonState, time), &time, sizeof(float));
            }

            void upload_ribbon_color(wgpu::Queue& queue, const float (&color)[3]) {
                // Only update the color[3] field (offset 32 — see GPURibbonState layout)
                queue.WriteBuffer(ribbonBuffer_, offsetof(GPURibbonState, color), color, sizeof(color));
            }

            void upload_ribbon(wgpu::Queue& queue, const GPURibbonState& ribbon) {
                writeStruct(queue, ribbonBuffer_, ribbon);
            }

            void upload_ribbon_wave_amps(wgpu::Queue& queue, float lateral_amp, float vertical_amp) {
                queue.WriteBuffer(ribbonBuffer_, offsetof(GPURibbonState, lateral_amp), &lateral_amp, sizeof(float));
                queue.WriteBuffer(ribbonBuffer_, offsetof(GPURibbonState, vertical_amp), &vertical_amp, sizeof(float));
            }

            void upload_ribbon_head_poses(wgpu::Queue& queue, const float* data, size_t bytes) {
                queue.WriteBuffer(headPosesBuffer_, 0, data, bytes);
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

            // Partial writes for the anchor law (ONE_ANCHOR_1). The
            // toggle writes only the follow_pawn sentinel (the kernel
            // captures/releases from the true present); the corral
            // writes only the glide target (the kernel walks the live
            // param toward it). No CPU hand ever moves anchor or
            // offset directly.
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

            // GPU mesh gen: write params for a single arch slot (64 bytes per spawn/evict)
            void upload_arch_mesh_params_slot(wgpu::Queue& queue, uint32_t slot, const GPUArchMeshParams& params) {
                writeSlot(queue, archMeshParamsBuffer_, slot, params);
            }

            // Arch GPU mesh gen bind group (dedicated layout — bindings 193-195)
            wgpu::BindGroupLayout arch_mesh_gen_layout() const { return archMeshGenLayout_; }
            wgpu::BindGroup arch_mesh_gen_group() const { return archMeshGenBindGroup_; }
            wgpu::BindGroupLayout agent_occupier_layout() const { return agentOccupierLayout_; }
            wgpu::BindGroup agent_occupier_group() const { return agentOccupierBindGroup_; }

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
                        desc.label = label;
                        desc.size = { Dim::PAINTING_RESOLUTION, Dim::PAINTING_RESOLUTION, layers };
                        desc.dimension = wgpu::TextureDimension::e2D;
                        desc.format = colorFormat;
                        desc.usage = usage;
                        return device_.CreateTexture(&desc);
                    };

                auto makeArrayView = [&](wgpu::Texture tex, const char* label, uint32_t layers) -> wgpu::TextureView {
                    wgpu::TextureViewDescriptor vd{};
                    vd.dimension = wgpu::TextureViewDimension::e2DArray;
                    vd.arrayLayerCount = layers;
                    vd.label = label;
                    return tex.CreateView(&vd);
                    };

                // Snapshot staging — photographer writes here, promotion copies from here
                snapshotStagingTexture_ = makeTextureArray("Snapshot Staging",
                    Dim::STAGING_LAYERS,
                    wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc);
                if (!snapshotStagingTexture_) return false;
                snapshotStagingReadView_ = makeArrayView(snapshotStagingTexture_,
                    "Snapshot Staging View", Dim::STAGING_LAYERS);

                // Authored staging — disk images loaded here, promotion copies from here
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
                    desc.label = "Offscreen Snapshot Color";
                    desc.size = { Dim::PAINTING_RESOLUTION, Dim::PAINTING_RESOLUTION, 1 };
                    desc.format = colorFormat;
                    desc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
                    offscreenColorTexture_ = device_.CreateTexture(&desc);
                    if (!offscreenColorTexture_) return false;
                    offscreenColorView_ = offscreenColorTexture_.CreateView();
                }
                {
                    wgpu::TextureDescriptor desc{};
                    desc.label = "Offscreen Snapshot Depth";
                    desc.size = { Dim::PAINTING_RESOLUTION, Dim::PAINTING_RESOLUTION, 1 };
                    desc.format = wgpu::TextureFormat::Depth24Plus;
                    desc.usage = wgpu::TextureUsage::RenderAttachment;
                    offscreenDepthTexture_ = device_.CreateTexture(&desc);
                    if (!offscreenDepthTexture_) return false;
                    offscreenDepthView_ = offscreenDepthTexture_.CreateView();
                }

                // Recreate gallery texture bind group with exhibition view
                {
                    std::array<wgpu::BindGroupEntry, 5> entries{};
                    entries[0].binding = bind::g1::painting_slots;
                    entries[0].buffer = paintingSlotsBuffer_;
                    entries[0].size = sizeof(GPUPaintingSlot) * Dim::PAINTING_MAX_SLOTS;
                    entries[1].binding = bind::g1::painting_array;
                    entries[1].textureView = exhibitionReadView_;
                    entries[2].binding = bind::g1::painting_sampler_filt;
                    entries[2].sampler = paintingSampler_;
                    entries[3].binding = bind::g1::bilinear_sampler;
                    entries[3].sampler = bilinearSampler_;
                    entries[4].binding = bind::g1::live_card_read;
                    entries[4].textureView = liveCardView_;

                    wgpu::BindGroupDescriptor bd{};
                    bd.label = "Gallery Texture BindGroup";
                    bd.layout = galleryTextureBindGroupLayout_;
                    bd.entryCount = entries.size();
                    bd.entries = entries.data();
                    galleryTextureBindGroup_ = device_.CreateBindGroup(&bd);
                    if (!galleryTextureBindGroup_) return false;
                }

                return true;
            }

            // Mode presets
            void enter_design_mode() {
                if (config_.mute_signal != 1 || config_.mute_couplings != Coupling::ALL) {
                    config_.mute_signal = 1; config_.mute_couplings = Coupling::ALL;
                    configDirty_ = true;
                }
            }
            void enter_performance_mode() {
                if (config_.mute_signal != 0 || config_.mute_couplings != Coupling::NONE) {
                    config_.mute_signal = 0; config_.mute_couplings = Coupling::NONE;
                    configDirty_ = true;
                }
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
            // NO configDirty_: these fields ride
            // targeted sub-range uploads (
            // upload_placement_patch_count / upload_lod_point) or the
            // next dirty/dynamic full upload (floater_coordination) —
            // exactly the raw config() pokes they replace, identical
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
            void stage_floater_coordination(float v)     { config_.floater_coordination = v; }
            const GPUDesignConfig& config() const { return config_; }
            uint32_t get_fpv_mode() const { return config_.fpv_mode; }

            // --- Compute pass ---
            wgpu::BindGroup compute_entity_group() const { return computeEntityBindGroup_; }
            wgpu::BindGroupLayout compute_entity_layout() const { return computeEntityBindGroupLayout_; }
            // Live-contributor textures (Group 1) for compute pipelines that
            // evaluate query_ground_flyer / query_ground_walker.
            wgpu::BindGroup compute_texture_group() const { return computeTextureBindGroup_; }
            wgpu::BindGroupLayout compute_texture_layout() const { return computeTextureBindGroupLayout_; }
            wgpu::BindGroup patch_gen_group() const { return patchGenBindGroup_; }
            wgpu::BindGroupLayout patch_gen_layout() const { return patchGenLayout_; }
            wgpu::BindGroup ribbon_compute_group() const { return ribbonComputeBindGroup_; }
            wgpu::BindGroupLayout ribbon_compute_layout() const { return ribbonComputeLayout_; }
            wgpu::BindGroup mesh_gen_entity_group() const { return meshGenEntityBindGroup_; }
            wgpu::BindGroupLayout mesh_gen_entity_layout() const { return meshGenEntityBindGroupLayout_; }

            // --- Render pass ---
            wgpu::BindGroup render_entity_group() const { return renderEntityBindGroup_; }
            wgpu::BindGroupLayout render_entity_layout() const { return renderEntityBindGroupLayout_; }
            wgpu::BindGroup render_texture_group() const { return renderTextureBindGroup_; }
            wgpu::BindGroupLayout render_texture_layout() const { return renderTextureBindGroupLayout_; }

            // --- Shadow pass ---
            wgpu::BindGroup shadow_texture_group() const { return shadowTextureBindGroup_; }
            wgpu::BindGroupLayout shadow_texture_layout() const { return shadowTextureBindGroupLayout_; }
            wgpu::TextureView shadow_map_view() const { return shadowMapView_; }
            wgpu::TextureView spot_shadow_map_view() const { return spotShadowMapView_; }

            // --- Mesh buffers ---
            wgpu::Buffer patch_index_buffer() const { return patchIndexBuffer_; }
            uint32_t patch_index_count() const { return patchIndexCount_; }
            wgpu::Buffer patch_index_buffer_lod1() const { return patchIndexBufferLOD1_; }
            uint32_t patch_index_count_lod1() const { return patchIndexCountLOD1_; }

            // --- GPU frustum culling ---
            wgpu::Buffer frustum_indirect_lod0() const { return frustumIndirectLOD0_; }
            wgpu::Buffer frustum_compute_buffer() const { return frustumComputeBuffer_; }
            wgpu::Buffer probe_null_indirect() const { return probeNullIndirectBuffer_; }
            wgpu::Buffer visible_patch_indices_buffer() const { return visiblePatchIndicesBuffer_; }
            wgpu::BindGroupLayout frustum_cull_layout() const { return frustumCullLayout_; }
            wgpu::BindGroup frustum_cull_group() const { return frustumCullBindGroup_; }

            void reset_frustum_indirect(wgpu::Queue& queue) {
                uint32_t args[5] = { patchIndexCount_, 0, 0, 0, 0 };
                queue.WriteBuffer(frustumComputeBuffer_, 0, args, sizeof(args));
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
            }

            // Column GPU mesh gen bind group
            wgpu::BindGroupLayout column_mesh_gen_layout() const { return columnMeshGenLayout_; }
            wgpu::BindGroup column_mesh_gen_group() const { return columnMeshGenBindGroup_; }

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

            wgpu::BindGroupLayout palm_mesh_gen_layout() const { return palmMeshGenLayout_; }
            wgpu::BindGroup palm_mesh_gen_group() const { return palmMeshGenBindGroup_; }

            // --- Cactus accessors and upload ---
            wgpu::Buffer cactus_vertex_buffer() const { return cactusVertexBuffer_; }
            wgpu::Buffer cactus_index_buffer() const { return cactusIndexBuffer_; }
            uint32_t cactus_index_count() const { return cactusIndexCount_; }
            void set_cactus_index_count(uint32_t count) { cactusIndexCount_ = count; }
            void upload_cactus_mesh_params_slot(wgpu::Queue& queue, uint32_t slot, const GPUCactusMeshParams& params) {
                writeSlot(queue, cactusMeshParamsBuffer_, slot, params);
            }
            wgpu::BindGroupLayout cactus_mesh_gen_layout() const { return cactusMeshGenLayout_; }
            wgpu::BindGroup cactus_mesh_gen_group() const { return cactusMeshGenBindGroup_; }

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
            wgpu::BindGroupLayout blade_mesh_gen_layout() const { return bladeMeshGenLayout_; }
            wgpu::BindGroup blade_mesh_gen_group() const { return bladeMeshGenBindGroup_; }

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
            wgpu::Buffer floating_entity_buffer() const { return floatingEntityBuffer_; }
            static constexpr size_t floating_entity_buffer_size() {
                return Dim::TOTAL_FLOATING_SLOTS * sizeof(GPUFloatingEntityState);
            }
            static constexpr size_t agent_state_buffer_size() { return Dim::MAX_AGENTS * sizeof(GPUAgentState); }
            static constexpr size_t agent_slot_size() { return sizeof(GPUAgentState); }
            // The point readback (option A): camera-host
            // only — the spine copies the camera state to staging and
            // harvests pos.xz as THE POINT's position. Pawn-host never
            // touches these (that path stays the agent readback).
            wgpu::Buffer camera_state_buffer() const { return cameraBuffer_; }
            wgpu::Buffer camera_state_readback_staging() const { return cameraStateReadbackStaging_; }
            static constexpr size_t camera_state_buffer_size() { return sizeof(GPUCameraState); }
            void set_possessed_slot(uint32_t slot) {
                if (config_.possessed_slot != slot) {
                    config_.possessed_slot = slot;
                    configDirty_ = true;
                }
            }
            wgpu::Buffer ribbon_readback_staging() const { return ribbonReadbackStaging_; }
            wgpu::Buffer ring_transforms_buffer() const { return ringTransformsBuffer_; }
            static constexpr size_t ribbon_ring_readback_size() { return sizeof(GPURibbonRingTransform) * Dim::RIBBON_MAX_RINGS; }

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
            wgpu::BindGroup gallery_entity_group() const { return galleryEntityBindGroup_; }
            wgpu::BindGroupLayout gallery_entity_layout() const { return galleryEntityBindGroupLayout_; }
            wgpu::BindGroup gallery_texture_group() const { return galleryTextureBindGroup_; }
            wgpu::BindGroupLayout gallery_texture_layout() const { return galleryTextureBindGroupLayout_; }
            wgpu::BindGroup photographer_render_entity_group() const { return photographerRenderEntityBindGroup_; }
            wgpu::TextureView offscreen_color_view() const { return offscreenColorView_; }
            wgpu::TextureView offscreen_depth_view() const { return offscreenDepthView_; }
            wgpu::Texture offscreen_color_texture() const { return offscreenColorTexture_; }

            // Three-array painting system accessors
            wgpu::Texture snapshot_staging_texture() const { return snapshotStagingTexture_; }
            wgpu::Texture authored_staging_texture() const { return authoredStagingTexture_; }
            wgpu::Texture exhibition_texture() const { return exhibitionTexture_; }

            // Promote a staging layer to an exhibition layer (GPU copy, call within encoder scope)
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
            wgpu::BindGroup photographer_compute_group() const { return photographerComputeBindGroup_; }
            wgpu::BindGroupLayout photographer_compute_layout() const { return photographerComputeLayout_; }
            wgpu::BindGroup entity_placement_compute_group() const { return entityPlacementComputeBindGroup_; }
            wgpu::BindGroupLayout entity_placement_compute_layout() const { return entityPlacementComputeLayout_; }

            // --- GoL zone system ---
            wgpu::BindGroup zone_gol_compute_group() const { return zoneGolComputeBindGroup_; }
            wgpu::BindGroupLayout zone_gol_compute_layout() const { return zoneGolComputeLayout_; }

            // Pawn aura accessors
            wgpu::BindGroupLayout pawn_aura_compute_layout() const { return pawnAuraComputeLayout_; }
            wgpu::BindGroup pawn_aura_compute_group() const { return pawnAuraComputeGroup_; }
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
            wgpu::BindGroupLayout live_card_writer_layout() const { return liveCardWriterLayout_; }
            wgpu::BindGroup live_card_writer_group() const { return liveCardWriterGroup_; }
            wgpu::BindGroupLayout zone_mask_layout() const { return zoneMaskLayout_; }
            wgpu::BindGroup zone_mask_group() const { return zoneMaskGroup_; }

            // Orb sky layer accessors
            wgpu::Buffer orb_state_buffer() const { return orbStateBuffer_; }
            wgpu::Buffer orb_config_buffer() const { return orbConfigBuffer_; }
            wgpu::Buffer orb_quad_vb() const { return orbQuadVB_; }
            wgpu::Buffer orb_quad_ib() const { return orbQuadIB_; }
            wgpu::BindGroupLayout orb_compute_layout() const { return orbComputeLayout_; }
            wgpu::BindGroup orb_compute_group() const { return orbComputeGroup_; }
            wgpu::BindGroupLayout orb_copy_layout() const { return orbCopyLayout_; }
            wgpu::BindGroup orb_copy_group() const { return orbCopyGroup_; }
            void upload_orb_config(wgpu::Queue& queue, const GPUOrbConfig& cfg) {
                writeStruct(queue, orbConfigBuffer_, cfg);
            }
            void upload_orb_frame(wgpu::Queue& queue, float dt, float t_seconds) {
                queue.WriteBuffer(orbConfigBuffer_, offsetof(GPUOrbConfig, dt), &dt, sizeof(float));
                queue.WriteBuffer(orbConfigBuffer_, offsetof(GPUOrbConfig, t_seconds), &t_seconds, sizeof(float));
            }
            void upload_orb_force(wgpu::Queue& queue, float radial) {
                queue.WriteBuffer(orbConfigBuffer_,
                    offsetof(GPUOrbConfig, force_radial), &radial, sizeof(float));
            }
            void upload_orb_noise(wgpu::Queue& queue, float noise) {
                queue.WriteBuffer(orbConfigBuffer_,
                    offsetof(GPUOrbConfig, noise_amp), &noise, sizeof(float));
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
                writeStruct(queue, portalArrayBuffer_, arr);
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
            static constexpr uint32_t patch_heightfield_workgroups() { return Dim::PATCH_HEIGHTFIELD_N / 16; }
            static constexpr uint32_t patch_cell_workgroups() { return Dim::PATCH_CELL_N / 8; }
            static constexpr uint32_t ribbon_ring_workgroups() { return (Dim::RIBBON_MAX_RINGS + 63) / 64; }

            // --- Batch patch generation ---
            wgpu::Buffer patch_params_buffer() const { return patchParamsBuffer_; }
            wgpu::Buffer patch_staging_buffer() const { return patchStagingBuffer_; }
            void upload_patch_staging(wgpu::Queue& queue, const GPUPatchParams* params,
                uint32_t count, uint32_t offset = 0) {
                queue.WriteBuffer(patchStagingBuffer_,
                    offset * sizeof(GPUPatchParams),
                    params, sizeof(GPUPatchParams) * count);
            }

        private:

            wgpu::Buffer makeBuffer(const char* label, uint64_t size, wgpu::BufferUsage usage) {
                wgpu::BufferDescriptor d{}; d.label = label; d.size = size; d.usage = usage;
                return device_.CreateBuffer(&d);
            }

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
                agentBehaviorsBuffer_ = makeBuffer("Agent Behaviors Table",
                    GPU_AGENT_BEHAVIOR_COUNT * sizeof(GPUAgentBehaviorDef),
                    wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst);
                agentTierGainsBuffer_ = makeBuffer("Agent Tier Gains Table",
                    GPU_AGENT_TIER_COUNT * sizeof(GPUAgentTierDef),
                    wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst);
                // Pawn figure table — UNIFORM (not storage): the render VS storage cap
                // is full (see Render Entity Layout entry[17/18]). 14 × 288 B = 4032 B.
                figureProfilesBuffer_ = makeBuffer("Figure Profiles Table",
                    PAWN_FIGURE_COUNT * sizeof(GPUPawnFigure),
                    wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst);
                cameraBuffer_ = makeBuffer("Camera State", sizeof(GPUCameraState),
                    SU | wgpu::BufferUsage::CopySrc);   // CopySrc: the point readback (camera-host)
                floatingEntityBuffer_ = makeBuffer("Floating Entity Array",
                    Dim::TOTAL_FLOATING_SLOTS * sizeof(GPUFloatingEntityState),
                    SU | wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopySrc);
                // LATENT[gate-a-shared] ribbon (SH·mb): ribbonRing pipeline + readback staging droppable, but ribbonBuffer_/ringTransformsBuffer_ are exclusive-in-Render-Entity + Photographer. Retire = re-section those groups.
                ribbonBuffer_ = makeBuffer("Ribbon State", sizeof(GPURibbonState), SU | wgpu::BufferUsage::Uniform);
                ringTransformsBuffer_ = makeBuffer("Ring Transforms",
                    sizeof(GPURibbonRingTransform) * Dim::RIBBON_MAX_RINGS,
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc);
                headPosesBuffer_ = makeBuffer("Ribbon Head Poses",
                    sizeof(float) * 4 * Dim::RIBBON_MAX_RINGS,
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                vpBuffer_ = makeBuffer("VP Matrix", sizeof(GPUVPMatrix),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                directionalLightBuffer_ = makeBuffer("Directional Light", sizeof(GPUDirectionalLight), SU);
                pointLightsBuffer_ = makeBuffer("Point Lights", sizeof(GPUPointLightArray), SU);
                // LATENT[gate-a-shared] spot_lights (SH·mb): spotVPStagingBuffer_ + spotShadowMapTexture_ (atlas) partly dedicated, but spotLightArrayBuffer_ is exclusive-in-Render-Entity + Photographer and the atlas is bound in Shadow Texture. Retire = re-section those groups.
                spotLightArrayBuffer_ = makeBuffer("Spot Light Array", sizeof(GPUSpotLightArray), SU);
                spotVPStagingBuffer_ = makeBuffer("Spot VP Staging",
                    MAX_SPOT_LIGHTS * 64,
                    wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc);
                portalArrayBuffer_ = makeBuffer("Portal Array", sizeof(GPUPortalArray), UU);
                {
                    wgpu::BufferDescriptor sd{};
                    sd.label = "Agent State Readback Staging";
                    sd.size = Dim::MAX_AGENTS * sizeof(GPUAgentState);
                    sd.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
                    agentStateReadbackStaging_ = device_.CreateBuffer(&sd);
                }
                {
                    wgpu::BufferDescriptor sd{};
                    sd.label = "Floating Entity Readback Staging";
                    sd.size = Dim::TOTAL_FLOATING_SLOTS * sizeof(GPUFloatingEntityState);
                    sd.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
                    floatingEntityReadbackStaging_ = device_.CreateBuffer(&sd);
                }
                {
                    wgpu::BufferDescriptor sd{};
                    sd.label = "Camera State Readback Staging";
                    sd.size = sizeof(GPUCameraState);
                    sd.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
                    cameraStateReadbackStaging_ = device_.CreateBuffer(&sd);
                }
                {
                    wgpu::BufferDescriptor sd{};
                    sd.label = "Ribbon Ring Readback Staging";
                    sd.size = sizeof(GPURibbonRingTransform) * Dim::RIBBON_MAX_RINGS;
                    sd.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
                    ribbonReadbackStaging_ = device_.CreateBuffer(&sd);
                }
                // THE FRAME METER — GPU half. Created only when the device
                // carries timestamp-query (the cartridge prints the loud
                // boot line when absent); CPU rows are unaffected either way.
                meterEnabled_ = device_.HasFeature(wgpu::FeatureName::TimestampQuery);
                if (meterEnabled_) {
                    wgpu::QuerySetDescriptor qd{};
                    qd.label = "Frame Meter Timestamps";
                    qd.type = wgpu::QueryType::Timestamp;
                    qd.count = METER_QUERY_COUNT;
                    meterQuerySet_ = device_.CreateQuerySet(&qd);
                    meterResolveBuffer_ = makeBuffer("Frame Meter Resolve",
                        METER_QUERY_COUNT * sizeof(uint64_t),
                        wgpu::BufferUsage::QueryResolve | wgpu::BufferUsage::CopySrc);
                    wgpu::BufferDescriptor sd{};
                    sd.label = "Frame Meter Readback Staging";
                    sd.size = METER_QUERY_COUNT * sizeof(uint64_t);
                    sd.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
                    meterReadbackStaging_ = device_.CreateBuffer(&sd);
                }
                patchParamsBuffer_ = makeBuffer("Patch Params", sizeof(GPUPatchParams), UU);
                patchStagingBuffer_ = makeBuffer("Patch Params Staging",
                    sizeof(GPUPatchParams) * Dim::MAX_ACTIVE_PATCHES,
                    wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc);
                tileGridBuffer_ = makeBuffer("Tile Grid", sizeof(GPUTileGrid), UU);
                patchInstancesBuffer_ = makeBuffer("Patch Instances",
                    sizeof(GPUPatchInstance) * Dim::MAX_ACTIVE_PATCHES,
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                patchGridBuffer_ = makeBuffer("Patch Grid",
                    sizeof(GPUPatchGrid),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                patchHeightScratchBuffer_ = makeBuffer("Patch Height Scratch",
                    Dim::PATCH_HEIGHTFIELD_N * Dim::PATCH_HEIGHTFIELD_N * 2 * sizeof(float),
                    wgpu::BufferUsage::Storage);
                // The card writer's two-pass scratch (the patch pattern at card
                // size — TRUEBAND_CONTACT_1)
                liveCardScratchBuffer_ = makeBuffer("Live Card Scratch",
                    Dim::LIVE_CARD_SIZE * Dim::LIVE_CARD_SIZE * 2 * sizeof(float),
                    wgpu::BufferUsage::Storage);

                // Self-Portrait Gallery
                photographerVPBuffer_ = makeBuffer("Photographer VP",
                    sizeof(GPUVPMatrix),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                photographerCameraBuffer_ = makeBuffer("Photographer Camera",
                    sizeof(GPUCameraState),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                photographerConfigBuffer_ = makeBuffer("Photographer Config",
                    sizeof(GPUPhotographerConfig),
                    wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst);
                // LATENT[gate-a-shared] gallery (SH·mb): gallery/wall-painting/photographer pipelines + offscreen textures + gallery groups droppable (photographer rides gallery's bit — LATENT[roster-split:photographer]), but paintingSlotsBuffer_ is exclusive-in-Compute-Entity + Entity-Placement. Retire = re-section those groups.
                paintingSlotsBuffer_ = makeBuffer("Painting Slots",
                    sizeof(GPUPaintingSlot) * Dim::PAINTING_MAX_SLOTS,
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);

                // GPU frustum culling — LOD0 only (Dawn D3D12 limitation: one indirect draw per pass).
                // Compute writes args+atomic to frustumComputeBuffer_, then CopyBufferToBuffer to indirect.
                frustumIndirectLOD0_ = makeBuffer("Frustum Indirect LOD0",
                    5 * sizeof(uint32_t),
                    wgpu::BufferUsage::Indirect | wgpu::BufferUsage::CopyDst);
                frustumComputeBuffer_ = makeBuffer("Frustum Compute Staging",
                    5 * sizeof(uint32_t),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst);
                visiblePatchIndicesBuffer_ = makeBuffer("Visible Patch Indices",
                    Dim::MAX_ACTIVE_PATCHES * sizeof(uint32_t),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);

                // PROBE C3 (HELD) — zero-initialized null indirect args
                // (WebGPU zero-inits buffers: indexCount 0, instanceCount 0
                // is a valid, behaviorally null draw).
                probeNullIndirectBuffer_ = makeBuffer("PROBE C3 Null Indirect",
                    5 * sizeof(uint32_t),
                    wgpu::BufferUsage::Indirect);

                return signalBuffer_ && configBuffer_ &&
                    agentStateBuffer_ && agentStateReadbackStaging_ &&
                    cameraBuffer_ && floatingEntityBuffer_ && ringTransformsBuffer_ && headPosesBuffer_ &&
                    vpBuffer_ && spotLightArrayBuffer_ && spotVPStagingBuffer_ && directionalLightBuffer_ && pointLightsBuffer_ && patchParamsBuffer_ &&
                    patchStagingBuffer_ && tileGridBuffer_ && patchInstancesBuffer_ &&
                    patchGridBuffer_ &&
                    patchHeightScratchBuffer_ && liveCardScratchBuffer_ &&
                    photographerVPBuffer_ && photographerCameraBuffer_ &&
                    photographerConfigBuffer_ && paintingSlotsBuffer_ &&
                    portalArrayBuffer_ && ribbonReadbackStaging_ && cameraStateReadbackStaging_ &&
                    frustumIndirectLOD0_ && frustumComputeBuffer_ && visiblePatchIndicesBuffer_ &&
                    probeNullIndirectBuffer_;
            }

            bool createMeshBuffers() {
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

                // LOD-0: cap tiles + curtains + skirt — ~50,688 indices
                // (UNIFIED_GROUND_1: the legacy 64×64 grid quads are replaced by
                //  per-cell cap tiles [16 quads/cell] + curtain quads [16/cell];
                //  the skirt ring keeps its legacy slots, top edge re-aimed at
                //  cap outer verts. The legacy grid+skirt decode space [0, 4481)
                //  remains the LOD-1/soft space — the LOD-1 IB below is
                //  byte-untouched.)
                {
                    std::vector<uint32_t> idx;
                    idx.reserve(Dim::UG_CELLS_PER_PATCH * (16 + 16) * 6 + 4 * Dim::PATCH_MESH_N * 6);
                    // The n=4 perimeter walk of a cell's 5×5 cap grid — the same
                    // CW bottom/right/top/left shape as skirt_grid_index (n=64).
                    auto cell_perimeter = [](uint32_t k, uint32_t& lx, uint32_t& lz) {
                        const uint32_t n = Dim::UG_QUADS_PER_CELL;   // 4
                        if      (k < n)     { lx = k;               lz = 0; }
                        else if (k < 2 * n) { lx = n;               lz = k - n; }
                        else if (k < 3 * n) { lx = n - (k - 2 * n); lz = n; }
                        else                { lx = 0;               lz = n - (k - 3 * n); }
                    };
                    // CAP QUADS — 256 cells × 16 quads (the house winding).
                    for (uint32_t cell = 0; cell < Dim::UG_CELLS_PER_PATCH; cell++) {
                        const uint32_t cap0 = Dim::UG_CAP_BASE + cell * Dim::UG_CAP_VERTS_PER_CELL;
                        for (uint32_t qz = 0; qz < Dim::UG_QUADS_PER_CELL; qz++) {
                            for (uint32_t qx = 0; qx < Dim::UG_QUADS_PER_CELL; qx++) {
                                uint32_t i00 = cap0 + qz * Dim::UG_CAP_STRIDE_C + qx;
                                uint32_t i10 = i00 + 1;
                                uint32_t i01 = i00 + Dim::UG_CAP_STRIDE_C;
                                uint32_t i11 = i01 + 1;
                                idx.push_back(i00); idx.push_back(i01); idx.push_back(i10);
                                idx.push_back(i10); idx.push_back(i01); idx.push_back(i11);
                            }
                        }
                    }
                    // CURTAIN QUADS — 256 cells × 16 quads: cap perimeter verts
                    // welded to their curtain-bottom twins (the skirt-quad shape,
                    // same winding as the ring below).
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
                            idx.push_back(a); idx.push_back(b); idx.push_back(sa);
                            idx.push_back(b); idx.push_back(sb); idx.push_back(sa);
                        }
                    }
                    // SKIRT RING — the legacy loop; top edge = cap outer verts;
                    // copies keep the legacy ring slots (their decode changes in U3).
                    auto skirt_cap_index = [](uint32_t k) {
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
                        return Dim::UG_CAP_BASE + (cz * Dim::PATCH_CELL_N + cx) * Dim::UG_CAP_VERTS_PER_CELL
                             + lz * Dim::UG_CAP_STRIDE_C + lx;
                    };
                    for (uint32_t k = 0; k < SKIRT_RING; k++) {
                        uint32_t k1 = (k + 1) % SKIRT_RING;
                        uint32_t a  = skirt_cap_index(k);
                        uint32_t b  = skirt_cap_index(k1);
                        uint32_t sa = SKIRT_GRID_VERTS + k;
                        uint32_t sb = SKIRT_GRID_VERTS + k1;
                        idx.push_back(a); idx.push_back(b); idx.push_back(sa);
                        idx.push_back(b); idx.push_back(sb); idx.push_back(sa);
                    }
                    patchIndexCount_ = (uint32_t)idx.size();
                    patchIndexBuffer_ = makeBuffer("Patch IB",
                        patchIndexCount_ * 4,
                        wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst);
                    if (!patchIndexBuffer_) return false;
                    auto q = device_.GetQueue();
                    q.WriteBuffer(patchIndexBuffer_, 0, idx.data(), idx.size() * 4);
                }

                {
                    constexpr uint32_t step = Dim::PATCH_MESH_N / Dim::PATCH_MESH_N_LOD1;  // = 2
                    constexpr uint32_t stride = Dim::PATCH_MESH_N + 1;  // 65 verts per row
                    std::vector<uint32_t> idx;
                    idx.reserve(Dim::PATCH_INDEX_COUNT_LOD1);
                    for (uint32_t z = 0; z < Dim::PATCH_MESH_N_LOD1; z++) {
                        for (uint32_t x = 0; x < Dim::PATCH_MESH_N_LOD1; x++) {
                            uint32_t i00 = (z * step) * stride + (x * step);
                            uint32_t i10 = i00 + step;
                            uint32_t i01 = i00 + step * stride;
                            uint32_t i11 = i01 + step;
                            idx.push_back(i00); idx.push_back(i01); idx.push_back(i10);
                            idx.push_back(i10); idx.push_back(i01); idx.push_back(i11);
                        }
                    }
                    // Skirt: coarse ring — LOD-1 uses every `step`th perimeter vert
                    // so the skirt top matches the LOD-1 interior edge exactly.
                    for (uint32_t k = 0; k < SKIRT_RING; k += step) {
                        uint32_t k1 = (k + step) % SKIRT_RING;
                        uint32_t a  = skirt_grid_index(k);
                        uint32_t b  = skirt_grid_index(k1);
                        uint32_t sa = SKIRT_GRID_VERTS + k;
                        uint32_t sb = SKIRT_GRID_VERTS + k1;
                        idx.push_back(a); idx.push_back(b); idx.push_back(sa);
                        idx.push_back(b); idx.push_back(sb); idx.push_back(sa);
                    }
                    patchIndexCountLOD1_ = (uint32_t)idx.size();
                    patchIndexBufferLOD1_ = makeBuffer("Patch IB LOD1",
                        patchIndexCountLOD1_ * 4,
                        wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst);
                    if (!patchIndexBufferLOD1_) return false;
                    auto q = device_.GetQueue();
                    q.WriteBuffer(patchIndexBufferLOD1_, 0, idx.data(), idx.size() * 4);
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
                // Mesh gen params buffer (16 × 64 bytes)
                archMeshParamsBuffer_ = makeBuffer("Arch Mesh Params",
                    Dim::MAX_ARCH_INSTANCES * sizeof(GPUArchMeshParams),
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
                    desc.label = "GoL Zone Life Texture Array";
                    desc.size = { Dim::GOL_ZONE_GRID, Dim::GOL_ZONE_GRID, Dim::MAX_GOL_ZONES };
                    desc.format = wgpu::TextureFormat::R32Float;
                    desc.usage = wgpu::TextureUsage::StorageBinding | wgpu::TextureUsage::TextureBinding;
                    desc.dimension = wgpu::TextureDimension::e2D;
                    zoneLifeTexture_ = device_.CreateTexture(&desc);
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
                orbStateBuffer_ = makeBuffer("Orb State",
                    Dim::MAX_ORBS * sizeof(GPUOrbState),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
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
                    desc.label = "Pawn Aura (RGBA16Float)";
                    desc.size = { PAWN_AURA_N, PAWN_AURA_N, 1 };
                    desc.format = wgpu::TextureFormat::RGBA16Float;
                    desc.usage = wgpu::TextureUsage::StorageBinding | wgpu::TextureUsage::TextureBinding;
                    pawnAuraTexture_ = device_.CreateTexture(&desc);
                    if (!pawnAuraTexture_) return false;
                    pawnAuraWriteView_ = pawnAuraTexture_.CreateView();
                    pawnAuraReadView_ = pawnAuraTexture_.CreateView();
                }

                // Live card (512×512 RGBA16Float — GROUND_CARD_1; writer kernel
                // rewrites it per frame, render + compute sample it)
                {
                    wgpu::TextureDescriptor desc{};
                    desc.label = "Live Card (512x512, RGBA16Float — GROUND_CARD_1)";
                    desc.size = { Dim::LIVE_CARD_SIZE, Dim::LIVE_CARD_SIZE, 1 };
                    desc.format = wgpu::TextureFormat::RGBA16Float;
                    desc.usage = wgpu::TextureUsage::StorageBinding | wgpu::TextureUsage::TextureBinding;
                    liveCardTexture_ = device_.CreateTexture(&desc);
                    if (!liveCardTexture_) return false;
                    liveCardWriteView_ = liveCardTexture_.CreateView();
                    liveCardView_ = liveCardTexture_.CreateView();
                }

                // Entity ground atlas (r32float 256×1 — compute writes ground_y, VS textureLoad)
                {
                    wgpu::TextureDescriptor desc{};
                    desc.label = "Entity Ground Atlas (r32float 256x1)";
                    desc.size = { Dim::GROUND_ATLAS_WIDTH, 1, 1 };
                    desc.format = wgpu::TextureFormat::R32Float;
                    desc.usage = wgpu::TextureUsage::StorageBinding | wgpu::TextureUsage::TextureBinding;
                    desc.dimension = wgpu::TextureDimension::e2D;
                    desc.mipLevelCount = 1;
                    desc.sampleCount = 1;
                    entityGroundAtlasTexture_ = device_.CreateTexture(&desc);
                    if (!entityGroundAtlasTexture_) return false;
                    entityGroundAtlasWriteView_ = entityGroundAtlasTexture_.CreateView();
                    entityGroundAtlasReadView_ = entityGroundAtlasTexture_.CreateView();
                }

                {
                    wgpu::TextureDescriptor desc{};
                    desc.label = "Patch Heightfield Array (289x256x256, RGBA16Float; 289 = Dim::MAX_ACTIVE_PATCHES)";
                    desc.size = { Dim::PATCH_HEIGHTFIELD_N, Dim::PATCH_HEIGHTFIELD_N, Dim::MAX_ACTIVE_PATCHES };
                    desc.dimension = wgpu::TextureDimension::e2D;
                    desc.format = wgpu::TextureFormat::RGBA16Float;
                    desc.usage = wgpu::TextureUsage::StorageBinding | wgpu::TextureUsage::TextureBinding;
                    patchHeightfieldArrayTexture_ = device_.CreateTexture(&desc);
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
                    desc.label = "Patch Cell Color Array (289x16x16, RGBA8Unorm; 289 = Dim::MAX_ACTIVE_PATCHES)";
                    desc.size = { Dim::PATCH_CELL_N, Dim::PATCH_CELL_N, Dim::MAX_ACTIVE_PATCHES };
                    desc.dimension = wgpu::TextureDimension::e2D;
                    desc.format = wgpu::TextureFormat::RGBA8Unorm;
                    desc.usage = wgpu::TextureUsage::StorageBinding | wgpu::TextureUsage::TextureBinding;
                    patchCellColorArrayTexture_ = device_.CreateTexture(&desc);
                    if (!patchCellColorArrayTexture_) return false;

                    wgpu::TextureViewDescriptor viewDesc{};
                    viewDesc.dimension = wgpu::TextureViewDimension::e2DArray;
                    viewDesc.arrayLayerCount = Dim::MAX_ACTIVE_PATCHES;
                    viewDesc.label = "Patch Cell Color Array Write";
                    patchCellColorArrayWriteView_ = patchCellColorArrayTexture_.CreateView(&viewDesc);
                    viewDesc.label = "Patch Cell Color Array Read";
                    patchCellColorArrayReadView_ = patchCellColorArrayTexture_.CreateView(&viewDesc);
                }

                // Shadow map (Depth32Float: directional light depth)
                {
                    wgpu::TextureDescriptor desc{};
                    desc.label = "Shadow Map";
                    desc.size = { Dim::SHADOW_MAP_SIZE, Dim::SHADOW_MAP_SIZE, 1 };
                    desc.format = wgpu::TextureFormat::Depth32Float;
                    desc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
                    shadowMapTexture_ = device_.CreateTexture(&desc);
                    if (!shadowMapTexture_) return false;
                    shadowMapView_ = shadowMapTexture_.CreateView();
                }

                // Spot shadow atlas (Depth32Float: 2×2 tiled for up to 4 spot lights)
                {
                    wgpu::TextureDescriptor desc{};
                    desc.label = "Spot Shadow Atlas";
                    desc.size = { Dim::SHADOW_MAP_SIZE, Dim::SHADOW_MAP_SIZE, 1 };
                    desc.format = wgpu::TextureFormat::Depth32Float;
                    desc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
                    spotShadowMapTexture_ = device_.CreateTexture(&desc);
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

            bool createBindGroups() {

                //
                {
                    std::array<wgpu::BindGroupLayoutEntry, 12> entries{};

                    entries[0].binding = bind::g0::signal;
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = bind::g0::config;
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[2].binding = bind::g0::vp_data;
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Storage;


                    entries[3].binding = bind::g0::agent_state;
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[4].binding = bind::g0::camera_state;
                    entries[4].visibility = wgpu::ShaderStage::Compute;
                    entries[4].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[5].binding = bind::g0::floating_entities;
                    entries[5].visibility = wgpu::ShaderStage::Compute;
                    entries[5].buffer.type = wgpu::BufferBindingType::Storage;

                    // (GROUND_CARD_1 [5c] evictions: tile_grid 25, pier_instances 26,
                    //  pyramid_instances 30, zone_config 160, zone_life 161 left this
                    //  layout. Agents ride the baked ground path (145/146/152) + the
                    //  card (g1:34); the zone pair is now bound only by the GoL sim
                    //  layouts and the live-card writer.)

                    entries[6].binding = bind::g0::portal_array;   // portal_array (uniform — proximity check in behavior_player_controlled)
                    entries[6].visibility = wgpu::ShaderStage::Compute;
                    entries[6].buffer.type = wgpu::BufferBindingType::Uniform;

                    // LIVE — the agents' baked ground path (sample_terrain_y_at
                    // in the rewired query family — GROUND_CARD_1 H5).
                    entries[7].binding = bind::g0::photo_heightfield;  // photo_heightfield (texture_2d_array)
                    entries[7].visibility = wgpu::ShaderStage::Compute;
                    entries[7].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[7].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    entries[8].binding = bind::g0::photo_sampler;  // photo_sampler (filtering)
                    entries[8].visibility = wgpu::ShaderStage::Compute;
                    entries[8].sampler.type = wgpu::SamplerBindingType::Filtering;

                    entries[9].binding = bind::g0::patch_grid;  // patch_grid (O(1) spatial index for sample_terrain_y_at)
                    entries[9].visibility = wgpu::ShaderStage::Compute;
                    entries[9].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    // Agent registries (uniform — read-only, fixed size,
                    // never changes during a session). Originally tried as
                    // ReadOnlyStorage but pushed compute storage buffer
                    // count past the 10-per-stage limit; uniform has its
                    // own 12-per-stage budget and these tables (≤ 512 B
                    // total) fit comfortably.
                    entries[10].binding = bind::g0::agent_behaviors;  // agent_behaviors
                    entries[10].visibility = wgpu::ShaderStage::Compute;
                    entries[10].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[11].binding = bind::g0::agent_tier_gains;  // agent_tier_gains
                    entries[11].visibility = wgpu::ShaderStage::Compute;
                    entries[11].buffer.type = wgpu::BufferBindingType::Uniform;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Compute Entity Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    computeEntityBindGroupLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!computeEntityBindGroupLayout_) return false;
                }

                //
                // Vertex shaders need entity state for positioning + VP for transform.
                // Fragment shaders need camera for fog distance.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 18> entries{};

                    entries[0].binding = bind::g0::config;    // config (uniform — fog, world_seed, aura_enabled, fade)
                    entries[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = bind::g0::render_vp;
                    entries[1].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[1].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[2].binding = bind::g0::render_agents;
                    entries[2].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[2].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[3].binding = bind::g0::render_camera;
                    entries[3].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[3].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[4].binding = bind::g0::render_floating;
                    entries[4].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[4].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[5].binding = bind::g0::render_light;
                    entries[5].visibility = wgpu::ShaderStage::Fragment;
                    entries[5].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[6].binding = bind::g0::render_point_lights;
                    entries[6].visibility = wgpu::ShaderStage::Fragment;
                    entries[6].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[7].binding = bind::g0::render_spot_lights;
                    entries[7].visibility = wgpu::ShaderStage::Fragment;
                    entries[7].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[8].binding = bind::g0::patch_instances;
                    entries[8].visibility = wgpu::ShaderStage::Vertex;
                    entries[8].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[9].binding = bind::g0::render_ribbon;
                    entries[9].visibility = wgpu::ShaderStage::Vertex;
                    entries[9].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[10].binding = bind::g0::render_ring_xforms;
                    entries[10].visibility = wgpu::ShaderStage::Vertex;
                    entries[10].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    // Tile grid (uniform — terrain wave delta needs amp_scale in VS,
                    // animated_cell_color needs archetype lookup in FS)
                    entries[11].binding = bind::g0::tile_grid;
                    entries[11].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[11].buffer.type = wgpu::BufferBindingType::Uniform;

                    // Entity ground atlas (r32float 256×1 — VS reads ground_y via textureLoad)
                    entries[12].binding = bind::g0::entity_ground_atlas;
                    entries[12].visibility = wgpu::ShaderStage::Vertex;
                    entries[12].texture.sampleType = wgpu::TextureSampleType::UnfilterableFloat;
                    entries[12].texture.viewDimension = wgpu::TextureViewDimension::e2D;

                    // Visible patch indices (GPU frustum cull output — VS reads indirection)
                    entries[13].binding = bind::g0::visible_patch_indices;
                    entries[13].visibility = wgpu::ShaderStage::Vertex;
                    entries[13].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    // Orb state (VS reads per-instance position/color/size for billboards)
                    entries[14].binding = bind::g0::render_orb_state;
                    entries[14].visibility = wgpu::ShaderStage::Vertex;
                    entries[14].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    // Orb config (radius/palette/tiers; dome_center dead wire — Pass 7)
                    entries[15].binding = bind::g0::orb_config;
                    entries[15].visibility = wgpu::ShaderStage::Vertex;
                    entries[15].buffer.type = wgpu::BufferBindingType::Uniform;

                    // Agent tier registry — same buffer as compute binding 111.
                    // Read by pawn_vs for entity color (tg.color_r/g/b).
                    // Uniform (not storage) to stay under the per-stage
                    // storage buffer cap; same buffer is bound as uniform
                    // on the compute side too.
                    entries[16].binding = bind::g0::agent_tier_gains;
                    entries[16].visibility = wgpu::ShaderStage::Vertex;
                    entries[16].buffer.type = wgpu::BufferBindingType::Uniform;

                    // Pawn figure table — read by pawn_vs / shadow_pawn_vs for per-figure
                    // profile + palette. Uniform (not storage) for the same reason as
                    // entry[17]: the VS storage-buffer cap is full. 4032 B, session-constant.
                    entries[17].binding = bind::g0::agent_figure_profiles;
                    entries[17].visibility = wgpu::ShaderStage::Vertex;
                    entries[17].buffer.type = wgpu::BufferBindingType::Uniform;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Render Entity Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    renderEntityBindGroupLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!renderEntityBindGroupLayout_) return false;
                }

                // -- Mesh gen entity layout (Group 0) -- binding 1 only ------
                // Only config uniform needed for active_cell_size.
                // Avoids exceeding the 8 storage buffer per-stage limit.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 1> entries{};

                    entries[0].binding = bind::g0::config;
                    entries[0].visibility = wgpu::ShaderStage::Compute | wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Mesh Gen Entity Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    meshGenEntityBindGroupLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!meshGenEntityBindGroupLayout_) return false;
                }

                // -- Shadow texture layout (Group 1) -- bindings 22-23, 28, 34 --
                // Used during shadow pass: samplers + patch heightfield only, NO shadow map.
                // Avoids read/write conflict (shadow map is depth attachment).
                {
                    std::array<wgpu::BindGroupLayoutEntry, 4> entries{};

                    entries[0].binding = bind::g1::bilinear_sampler;
                    entries[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[0].sampler.type = wgpu::SamplerBindingType::Filtering;

                    entries[1].binding = bind::g1::nearest_sampler;
                    entries[1].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[1].sampler.type = wgpu::SamplerBindingType::NonFiltering;

                    entries[2].binding = bind::g1::patch_heightfield_array_read;
                    entries[2].visibility = wgpu::ShaderStage::Vertex;
                    entries[2].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[2].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    // The live card (GROUND_CARD_1) — shadow VS height term
                    entries[3].binding = bind::g1::live_card_read;
                    entries[3].visibility = wgpu::ShaderStage::Vertex;
                    entries[3].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[3].texture.viewDimension = wgpu::TextureViewDimension::e2D;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Shadow Texture Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    shadowTextureBindGroupLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!shadowTextureBindGroupLayout_) return false;
                }

                // -- Render texture layout (Group 1) -- bindings 22-23, 25-29, 31-34 -
                // Used during main render pass: samplers + shadow maps + patches + GoL zones + pawn aura.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 11> entries{};

                    entries[0].binding = bind::g1::bilinear_sampler;
                    entries[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[0].sampler.type = wgpu::SamplerBindingType::Filtering;

                    entries[1].binding = bind::g1::nearest_sampler;
                    entries[1].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[1].sampler.type = wgpu::SamplerBindingType::NonFiltering;

                    entries[2].binding = bind::g1::shadow_map;
                    entries[2].visibility = wgpu::ShaderStage::Fragment;
                    entries[2].texture.sampleType = wgpu::TextureSampleType::Depth;
                    entries[2].texture.viewDimension = wgpu::TextureViewDimension::e2D;

                    entries[3].binding = bind::g1::shadow_sampler;
                    entries[3].visibility = wgpu::ShaderStage::Fragment;
                    entries[3].sampler.type = wgpu::SamplerBindingType::Comparison;

                    // Spot shadow atlas (depth texture, sampled in spot PCF)
                    entries[4].binding = bind::g1::spot_shadow_map;
                    entries[4].visibility = wgpu::ShaderStage::Fragment;
                    entries[4].texture.sampleType = wgpu::TextureSampleType::Depth;
                    entries[4].texture.viewDimension = wgpu::TextureViewDimension::e2D;

                    entries[5].binding = bind::g1::patch_heightfield_array_read;
                    entries[5].visibility = wgpu::ShaderStage::Vertex;
                    entries[5].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[5].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    entries[6].binding = bind::g1::patch_cell_color_array_read;
                    entries[6].visibility = wgpu::ShaderStage::Fragment;
                    entries[6].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[6].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    // GoL zone life texture (fragment reads alive/dead per zone)
                    entries[7].binding = bind::g1::zone_life_read;
                    entries[7].visibility = wgpu::ShaderStage::Fragment;
                    entries[7].texture.sampleType = wgpu::TextureSampleType::UnfilterableFloat;
                    entries[7].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    // GoL zone config (fragment reads zone origins for lookup)
                    entries[8].binding = bind::g1::zone_params;
                    entries[8].visibility = wgpu::ShaderStage::Fragment;
                    entries[8].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    // Pawn aura texture (VS + FS read persistent influence field)
                    entries[9].binding = bind::g1::pawn_aura_read;
                    entries[9].visibility = wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Vertex;
                    entries[9].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[9].texture.viewDimension = wgpu::TextureViewDimension::e2D;

                    // The live card (GROUND_CARD_1) — VS height/gradient + FS debug eye
                    entries[10].binding = bind::g1::live_card_read;
                    entries[10].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[10].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[10].texture.viewDimension = wgpu::TextureViewDimension::e2D;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Render Texture Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    renderTextureBindGroupLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!renderTextureBindGroupLayout_) return false;
                }

                //
                {
                    std::array<wgpu::BindGroupLayoutEntry, 4> entries{};

                    entries[0].binding = bind::g1::bilinear_sampler;   // bilinear_sampler (used by sample_pawn_aura)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].sampler.type = wgpu::SamplerBindingType::Filtering;

                    entries[1].binding = bind::g1::nearest_sampler;   // LIVE — the card's cell-exact GoL fetch
                                                                      // (sample_live_card_gol — GROUND_CARD_1 H5).
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].sampler.type = wgpu::SamplerBindingType::NonFiltering;

                    entries[2].binding = bind::g1::pawn_aura_read;   // pawn_aura_read (rgba16float, filterable)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[2].texture.viewDimension = wgpu::TextureViewDimension::e2D;

                    entries[3].binding = bind::g1::live_card_read;   // LIVE — the card (agents' live surface + cell-exact GoL; GROUND_CARD_1 H5)
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[3].texture.viewDimension = wgpu::TextureViewDimension::e2D;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Compute Texture Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    computeTextureBindGroupLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!computeTextureBindGroupLayout_) return false;
                }

                // -- Patch heightfield gen layout (Group 0) -- bindings 23-24 -
                // Per-patch compute pass: fills one heightfield layer.
                // Dispatched when a patch enters the active set.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 7> entries{};

                    entries[0].binding = bind::g0::config;    // config (uniform — world_seed for cell color gen)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = bind::g0::patch_params;
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[2].binding = bind::g0::patch_heightfield_array_write;
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].storageTexture.access = wgpu::StorageTextureAccess::WriteOnly;
                    entries[2].storageTexture.format = wgpu::TextureFormat::RGBA16Float;
                    entries[2].storageTexture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    entries[3].binding = bind::g0::tile_grid;
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[4].binding = bind::g0::patch_cell_color_array_write;
                    entries[4].visibility = wgpu::ShaderStage::Compute;
                    entries[4].storageTexture.access = wgpu::StorageTextureAccess::WriteOnly;
                    entries[4].storageTexture.format = wgpu::TextureFormat::RGBA8Unorm;
                    entries[4].storageTexture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    entries[5].binding = bind::g0::pyramid_instances;  // pyramid_instances
                    entries[5].visibility = wgpu::ShaderStage::Compute;
                    entries[5].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[6].binding = bind::g0::patch_height_scratch;  // patch_height_scratch (two-pass heightfield gen)
                    entries[6].visibility = wgpu::ShaderStage::Compute;
                    entries[6].buffer.type = wgpu::BufferBindingType::Storage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Patch Gen Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    patchGenLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!patchGenLayout_) return false;
                }

                //
                // Bindings: ribbon_state @120, ring_xforms @121, head_poses @122.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 3> entries{};

                    entries[0].binding = bind::g0::ribbon_state;
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = bind::g0::ring_xforms;
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[2].binding = bind::g0::head_poses;  // head_poses (storage, read) — ribbon head-path
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Ribbon Compute Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    ribbonComputeLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!ribbonComputeLayout_) return false;
                }

                // -- Gallery entity layout (Group 0) -- minimal for painting frames --
                {
                    std::array<wgpu::BindGroupLayoutEntry, 4> entries{};

                    entries[0].binding = bind::g0::config;    // config (uniform — fog for FS, veil ring + LOD point for VS)
                    entries[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = bind::g0::render_vp;
                    entries[1].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[1].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[2].binding = bind::g0::render_camera;
                    entries[2].visibility = wgpu::ShaderStage::Fragment;
                    entries[2].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[3].binding = bind::g0::render_light;
                    entries[3].visibility = wgpu::ShaderStage::Fragment;
                    entries[3].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Gallery Entity Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    galleryEntityBindGroupLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!galleryEntityBindGroupLayout_) return false;
                }

                // -- Gallery texture layout (Group 1) -- unified painting data --
                {
                    std::array<wgpu::BindGroupLayoutEntry, 5> entries{};

                    entries[0].binding = bind::g1::painting_slots;
                    entries[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[1].binding = bind::g1::painting_array;
                    entries[1].visibility = wgpu::ShaderStage::Fragment;
                    entries[1].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[1].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    entries[2].binding = bind::g1::painting_sampler_filt;
                    entries[2].visibility = wgpu::ShaderStage::Fragment;
                    entries[2].sampler.type = wgpu::SamplerBindingType::Filtering;

                    // The live card (GROUND_CARD_1) + its sampler — wall_painting_vs
                    // now rides the card for its ground term (H4 [4c]); this pipeline
                    // family binds gallery groups, so the pair lives here too.
                    entries[3].binding = bind::g1::bilinear_sampler;
                    entries[3].visibility = wgpu::ShaderStage::Vertex;
                    entries[3].sampler.type = wgpu::SamplerBindingType::Filtering;

                    entries[4].binding = bind::g1::live_card_read;
                    entries[4].visibility = wgpu::ShaderStage::Vertex;
                    entries[4].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[4].texture.viewDimension = wgpu::TextureViewDimension::e2D;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Gallery Texture Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    galleryTextureBindGroupLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!galleryTextureBindGroupLayout_) return false;
                }

                // -- Photographer compute layout (Group 0) -- VP + terrain clamp --
                // Reads THE POINT's position (host-sourced) + config →
                // builds VP, clamps camera above terrain.
                // Entity Y-correction is handled separately by compute_entity_placement.
                // 1 config uniform + 4 storage + 1 uniform + 1 texture + 1 sampler + 1 patch_grid = 9 entries.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 9> entries{};

                    entries[0].binding = bind::g0::config;    // config (DesignConfig — possessed_slot lookup via compute_pawn_pos)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = bind::g0::agent_state;   // agent_state (read pos via possessed_slot)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[8].binding = bind::g0::camera_state;   // camera_state (point_pos — the point's camera-host source)
                    entries[8].visibility = wgpu::ShaderStage::Compute;
                    entries[8].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[2].binding = bind::g0::photographer_config;  // photographer_config (camera params)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[3].binding = bind::g0::photographer_vp;  // photographer_vp (output VP matrix)
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[4].binding = bind::g0::photographer_camera_out;  // photographer_camera_out (output camera pos)
                    entries[4].visibility = wgpu::ShaderStage::Compute;
                    entries[4].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[5].binding = bind::g0::photo_heightfield;  // patch_heightfield (terrain texture)
                    entries[5].visibility = wgpu::ShaderStage::Compute;
                    entries[5].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[5].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    entries[6].binding = bind::g0::photo_sampler;  // bilinear_sampler
                    entries[6].visibility = wgpu::ShaderStage::Compute;
                    entries[6].sampler.type = wgpu::SamplerBindingType::Filtering;

                    entries[7].binding = bind::g0::patch_grid;  // patch_grid (O(1) spatial index for sample_terrain_y_at)
                    entries[7].visibility = wgpu::ShaderStage::Compute;
                    entries[7].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Photographer Compute Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    photographerComputeLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!photographerComputeLayout_) return false;
                }

                // -- Entity placement compute layout (Group 0) -- Y-correction --
                // Runs every frame, unconditionally. Samples the baked heightfield
                // (pier_correction is retired — always 0 since the pier erasure.)
                // 9 entries: config + painting slots + heightfield + entity grounds + ground atlas write + patch_grid.
                // GoL rides group 1 — the card.
                // Palm+cactus+blade share one buffer at binding 150: [0..23] palm, [24..43] cactus, [44..75] blade.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 9> entries{};

                    entries[0].binding = bind::g0::config;
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = bind::g0::photo_painting_slots;
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[2].binding = bind::g0::photo_heightfield;
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[2].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    entries[3].binding = bind::g0::photo_sampler;
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].sampler.type = wgpu::SamplerBindingType::Filtering;

                    entries[4].binding = bind::g0::arch_ground;
                    entries[4].visibility = wgpu::ShaderStage::Compute;
                    entries[4].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[5].binding = bind::g0::column_ground;
                    entries[5].visibility = wgpu::ShaderStage::Compute;
                    entries[5].buffer.type = wgpu::BufferBindingType::Storage;

                    // (GROUND_CARD_1 [5c]: zone_config 160 + zone_life 161 evicted —
                    //  placement's GoL term rides the card's cell-exact fetch via
                    //  group 1. The zone pair is now bound only by the GoL sim
                    //  layouts and the live-card writer.)

                    entries[6].binding = bind::g0::plant_ground;  // plant_ground: palm[0..23] + cactus[24..43] + blade[44..75]
                    entries[6].visibility = wgpu::ShaderStage::Compute;
                    entries[6].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[7].binding = bind::g0::entity_ground_atlas_write;  // entity_ground_atlas_write (r32float storage texture)
                    entries[7].visibility = wgpu::ShaderStage::Compute;
                    entries[7].storageTexture.access = wgpu::StorageTextureAccess::WriteOnly;
                    entries[7].storageTexture.format = wgpu::TextureFormat::R32Float;
                    entries[7].storageTexture.viewDimension = wgpu::TextureViewDimension::e2D;

                    entries[8].binding = bind::g0::patch_grid;  // patch_grid (O(1) spatial index for sample_terrain_y_at)
                    entries[8].visibility = wgpu::ShaderStage::Compute;
                    entries[8].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Entity Placement Compute Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    entityPlacementComputeLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!entityPlacementComputeLayout_) return false;
                }

                // -- Frustum cull compute layout (Group 0) -- GPU patch visibility --
                // Reads VP + config + patch instances, writes visible indices + indirect draw args.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 5> entries{};

                    entries[0].binding = bind::g0::config;     // config (uniform — render_patch_count)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = bind::g0::vp_data;     // vp_data (storage — VP matrix for frustum planes)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[2].binding = bind::g0::patch_instances;   // patch_instances (storage — all patches)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[3].binding = bind::g0::fc_visible;   // visible_patch_indices (storage, rw — output)
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[4].binding = bind::g0::fc_indirect;   // frustum_indirect (storage, rw — atomic draw args)
                    entries[4].visibility = wgpu::ShaderStage::Compute;
                    entries[4].buffer.type = wgpu::BufferBindingType::Storage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Frustum Cull Compute Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    frustumCullLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!frustumCullLayout_) return false;
                }

                // -- GoL zone compute layout (Group 0) -- bindings 1, 160-162, 166 --
                // Shared by all three zone entry points: sync, evolve, derive_params.
                // derive_params writes zone_config.zones[slot] from tier tables.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 5> entries{};

                    entries[0].binding = bind::g0::config;    // config (uniform — world_seed, fog, etc.)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    // Zone state (storage — derive_params writes, sync/evolve/mesh read-write)
                    entries[1].binding = bind::g0::zone_config;  // zone_config (storage, rw)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[2].binding = bind::g0::zone_life;  // zone_life (storage, read/write)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[3].binding = bind::g0::zone_life_tex_write;  // zone_life_tex (storage texture, write)
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].storageTexture.access = wgpu::StorageTextureAccess::WriteOnly;
                    entries[3].storageTexture.format = wgpu::TextureFormat::R32Float;
                    entries[3].storageTexture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    entries[4].binding = bind::g0::zone_derive_requests; // zone_derive_requests (uniform)
                    entries[4].visibility = wgpu::ShaderStage::Compute;
                    entries[4].buffer.type = wgpu::BufferBindingType::Uniform;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "GoL Zone Compute Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    zoneGolComputeLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!zoneGolComputeLayout_) return false;
                }

                // -- Pawn aura compute layout (Group 0) -- bindings 1, 25, 60, 170-172 --
                // (tile_grid joined at TERRAIN PROGRAM Phase 1: gol_composite_cell_color
                //  collapsed into the one evaluator, which reads the tile archetype.)
                {
                    std::array<wgpu::BindGroupLayoutEntry, 6> entries{};

                    entries[0].binding = bind::g0::config;    // config (uniform — world_seed for cell color)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = bind::g0::agent_state;   // agent_state (read possessed_slot position)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[2].binding = bind::g0::pawn_aura_cfg;  // pawn_aura_config (uniform)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[3].binding = bind::g0::pawn_aura_cells;  // pawn_aura_cells (storage, rw)
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[4].binding = bind::g0::pawn_aura_tex_write;  // pawn_aura_tex (storage texture, write)
                    entries[4].visibility = wgpu::ShaderStage::Compute;
                    entries[4].storageTexture.access = wgpu::StorageTextureAccess::WriteOnly;
                    entries[4].storageTexture.format = wgpu::TextureFormat::RGBA16Float;
                    entries[4].storageTexture.viewDimension = wgpu::TextureViewDimension::e2D;

                    entries[5].binding = bind::g0::tile_grid;   // tile_grid (uniform — evaluator's archetype lookup)
                    entries[5].visibility = wgpu::ShaderStage::Compute;
                    entries[5].buffer.type = wgpu::BufferBindingType::Uniform;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Pawn Aura Compute Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    pawnAuraComputeLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!pawnAuraComputeLayout_) return false;
                }

                // -- Live card writer layout (Group 0) -- bindings 0, 1, 160, 161, 31, 32 --
                // (GROUND_CARD_1) The writer kernel calls the existing evaluators at
                // texel centers: signal (band blends + pulse clock), config (waves +
                // pulses + lod_point origin), the zone pair (raw GoL lift), the
                // card's storage-texture write, and the two-pass scratch
                // (TRUEBAND_CONTACT_1). The Compute Entity eviction has LANDED —
                // this layout IS the zone pair's sole home outside the GoL sim.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 6> entries{};

                    entries[0].binding = bind::g0::signal;    // signal (uniform — band blends, t_seconds)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = bind::g0::config;    // config (uniform — waves, pulses, lod_point)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[2].binding = bind::g0::zone_config;  // zone_config (storage — matches var<storage, read_write>)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[3].binding = bind::g0::zone_life;  // zone_life (storage, rw — matches WGSL var declaration)
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[4].binding = bind::g0::live_card_write;  // live_card_write (storage texture, write)
                    entries[4].visibility = wgpu::ShaderStage::Compute;
                    entries[4].storageTexture.access = wgpu::StorageTextureAccess::WriteOnly;
                    entries[4].storageTexture.format = wgpu::TextureFormat::RGBA16Float;
                    entries[4].storageTexture.viewDimension = wgpu::TextureViewDimension::e2D;

                    entries[5].binding = bind::g0::live_card_scratch;  // two-pass scratch (TRUEBAND_CONTACT_1)
                    entries[5].visibility = wgpu::ShaderStage::Compute;
                    entries[5].buffer.type = wgpu::BufferBindingType::Storage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Live Card Writer Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    liveCardWriterLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!liveCardWriterLayout_) return false;
                }

                // -- Zone mask layout (Group 0) -- bindings 1, 25, 160, 161, 166 --
                // (UNIFIED_GROUND_1 U5) The birth-moment mask kernel: config +
                // tile_grid feed the color system's rest predicate; the zone pair
                // is Storage (their declarations are var<storage, read_write> —
                // the H-batch precedent); derive requests scope the dispatch.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 5> entries{};

                    entries[0].binding = bind::g0::config;    // config (uniform — the field evaluators)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = bind::g0::tile_grid;   // tile_grid (uniform — archetype lookup)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[2].binding = bind::g0::zone_config;  // zone_config (storage — matches var<storage, read_write>)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[3].binding = bind::g0::zone_life;  // zone_life (storage, rw — the height_factor plane)
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[4].binding = bind::g0::zone_derive_requests;  // zone_derive_requests (uniform)
                    entries[4].visibility = wgpu::ShaderStage::Compute;
                    entries[4].buffer.type = wgpu::BufferBindingType::Uniform;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Zone Mask Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    zoneMaskLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!zoneMaskLayout_) return false;
                }

                // -- Orb compute layout (Group 0) -- bindings 410 RW, 411 U, 412 RO --
                // Used by init / dynamics / recolor. orb_state is read_write so these
                // kernels can update it; orb_state_prev is read-only because only the
                // copy kernel writes it (through orbCopyLayout_).
                {
                    std::array<wgpu::BindGroupLayoutEntry, 3> entries{};

                    entries[0].binding = bind::g0::orb_state;  // orb_state (storage, read_write)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[1].binding = bind::g0::orb_config;  // orb_config (uniform)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[2].binding = bind::g0::orb_state_prev;  // orb_state_prev (storage, read-only)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Orb Compute Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    orbComputeLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!orbComputeLayout_) return false;
                }

                // -- Orb copy layout (Group 0) -- bindings 413 RO, 411 U, 414 RW --
                // Dedicated bindings for orb_state_prev_copy because WebGPU
                // requires each shader declaration's access mode to match its
                // layout entry exactly. We can't reuse 410/412 (which the main
                // layout binds RW/RO) with flipped access. 413 aliases orb_state
                // through a read-only view; 414 aliases orb_state_prev through
                // a read_write view.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 3> entries{};

                    entries[0].binding = bind::g0::orb_state_ro;  // orb_state_ro (storage, read-only)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[1].binding = bind::g0::orb_config;  // orb_config (uniform — declared at module scope)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[2].binding = bind::g0::orb_state_prev_rw;  // orb_state_prev_rw (storage, read_write)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Storage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Orb Copy Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    orbCopyLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!orbCopyLayout_) return false;
                }

                // -- Arch mesh gen layout (Group 0) -- bindings 193-195 --
                {
                    std::array<wgpu::BindGroupLayoutEntry, 3> entries{};

                    entries[0].binding = bind::g0::amg_params;  // amg_params (read-only storage)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[1].binding = bind::g0::amg_vertices;  // amg_vertices (storage, rw)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[2].binding = bind::g0::amg_indices;  // amg_indices (storage, rw)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Storage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Arch Mesh Gen Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    archMeshGenLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!archMeshGenLayout_) return false;
                }

                // -- THE AGENTS' ROOM (Group 2) -- bindings g2:0-1 --
                // Option B (BATCH F-B ruling): the agent kernels' own bind
                // group, so agent-side growth never touches the six pipelines
                // sharing the entity layout. Exactly two ReadOnlyStorage
                // windows onto the SAME mesh-param buffers the mesh-gen
                // groups bind — one fact, one home. The room grows only when
                // a named tenant arrives.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 2> entries{};

                    entries[0].binding = bind::g2::occupier_cmg;  // occupier_cmg (read-only storage)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[1].binding = bind::g2::occupier_amg;  // occupier_amg (read-only storage)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Agent Occupier Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    agentOccupierLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!agentOccupierLayout_) return false;
                }

                // -- Column mesh gen layout (Group 0) -- bindings 190/191 + 196-198 --
                {
                    std::array<wgpu::BindGroupLayoutEntry, 5> entries{};

                    entries[0].binding = bind::g0::cmg_params;  // cmg_params (read-only storage)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[1].binding = bind::g0::cmg_vertices;  // cmg_vertices (storage, rw)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[2].binding = bind::g0::cmg_indices;  // cmg_indices (storage, rw)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Storage;

                    // COLUMN CEILING FIT: the kernel derives effective column
                    // height = ceiling − ground_y (indoors), so the cmg strip
                    // reads the design config + the correction pass's output.
                    entries[3].binding = bind::g0::cmg_config;  // DesignConfig (the ceiling gate)
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[4].binding = bind::g0::cmg_column_ground;  // column_ground, read-only view
                    entries[4].visibility = wgpu::ShaderStage::Compute;
                    entries[4].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Column Mesh Gen Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    columnMeshGenLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!columnMeshGenLayout_) return false;
                }

                // Palm mesh gen layout (bindings 180-182)
                {
                    std::array<wgpu::BindGroupLayoutEntry, 3> entries{};

                    entries[0].binding = bind::g0::palmg_params;
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[1].binding = bind::g0::palmg_vertices;
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[2].binding = bind::g0::palmg_indices;
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Storage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Palm Mesh Gen Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    palmMeshGenLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!palmMeshGenLayout_) return false;
                }

                // Cactus mesh gen layout (bindings 183-185)
                {
                    std::array<wgpu::BindGroupLayoutEntry, 3> entries{};
                    entries[0].binding = bind::g0::cactusg_params;
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
                    entries[1].binding = bind::g0::cactusg_vertices;
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Storage;
                    entries[2].binding = bind::g0::cactusg_indices;
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Storage;
                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Cactus Mesh Gen Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    cactusMeshGenLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!cactusMeshGenLayout_) return false;
                }

                // Blade mesh gen layout (bindings 186-188)
                {
                    std::array<wgpu::BindGroupLayoutEntry, 3> entries{};
                    entries[0].binding = bind::g0::bladeg_params;
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
                    entries[1].binding = bind::g0::bladeg_vertices;
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Storage;
                    entries[2].binding = bind::g0::bladeg_indices;
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Storage;
                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Blade Mesh Gen Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    bladeMeshGenLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!bladeMeshGenLayout_) return false;
                }

                // Compute entity bind group (12 entries: systems + portals + the baked ground path)
                {
                    std::array<wgpu::BindGroupEntry, 12> entries{};

                    entries[0].binding = bind::g0::signal;
                    entries[0].buffer = signalBuffer_;
                    entries[0].size = sizeof(GPUFrameSignal);

                    entries[1].binding = bind::g0::config;
                    entries[1].buffer = configBuffer_;
                    entries[1].size = sizeof(GPUDesignConfig);

                    entries[2].binding = bind::g0::vp_data;
                    entries[2].buffer = vpBuffer_;
                    entries[2].size = sizeof(GPUVPMatrix);


                    entries[3].binding = bind::g0::agent_state;
                    entries[3].buffer = agentStateBuffer_;
                    entries[3].size = Dim::MAX_AGENTS * sizeof(GPUAgentState);

                    entries[4].binding = bind::g0::camera_state;
                    entries[4].buffer = cameraBuffer_;
                    entries[4].size = sizeof(GPUCameraState);

                    entries[5].binding = bind::g0::floating_entities;
                    entries[5].buffer = floatingEntityBuffer_;
                    entries[5].size = Dim::TOTAL_FLOATING_SLOTS * sizeof(GPUFloatingEntityState);

                    // (GROUND_CARD_1 [5c] evictions: tile_grid 25, pier_instances 26,
                    //  pyramid_instances 30, zone_config 160, zone_life 161 left this
                    //  layout. Agents ride the baked ground path (145/146/152) + the
                    //  card (g1:34); the zone pair is now bound only by the GoL sim
                    //  layouts and the live-card writer.)

                    entries[6].binding = bind::g0::portal_array;
                    entries[6].buffer = portalArrayBuffer_;
                    entries[6].size = sizeof(GPUPortalArray);

                    // LIVE — the agents' baked ground path (sample_terrain_y_at
                    // in the rewired query family — GROUND_CARD_1 H5).
                    entries[7].binding = bind::g0::photo_heightfield;
                    entries[7].textureView = patchHeightfieldArrayReadView_;

                    entries[8].binding = bind::g0::photo_sampler;
                    entries[8].sampler = bilinearSampler_;

                    entries[9].binding = bind::g0::patch_grid;
                    entries[9].buffer = patchGridBuffer_;
                    entries[9].size = sizeof(GPUPatchGrid);

                    // Agent registries — see bodies/agents.hpp for the
                    // authoring tables and GPUAgentBehaviorDef /
                    // GPUAgentTierDef in this file for GPU layout.
                    entries[10].binding = bind::g0::agent_behaviors;
                    entries[10].buffer = agentBehaviorsBuffer_;
                    entries[10].size = GPU_AGENT_BEHAVIOR_COUNT * sizeof(GPUAgentBehaviorDef);

                    entries[11].binding = bind::g0::agent_tier_gains;
                    entries[11].buffer = agentTierGainsBuffer_;
                    entries[11].size = GPU_AGENT_TIER_COUNT * sizeof(GPUAgentTierDef);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Compute Entity BindGroup";
                    desc.layout = computeEntityBindGroupLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    computeEntityBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!computeEntityBindGroup_) return false;
                }

                // Render entity bind group (18 entries: config + spaced by system +200, plus shared agent_tier_gains at 111 and agent_figure_profiles at 112)
                {
                    std::array<wgpu::BindGroupEntry, 18> entries{};

                    entries[0].binding = bind::g0::config;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);

                    entries[1].binding = bind::g0::render_vp;
                    entries[1].buffer = vpBuffer_;
                    entries[1].size = sizeof(GPUVPMatrix);

                    entries[2].binding = bind::g0::render_agents;
                    entries[2].buffer = agentStateBuffer_;
                    entries[2].size = Dim::MAX_AGENTS * sizeof(GPUAgentState);

                    entries[3].binding = bind::g0::render_camera;
                    entries[3].buffer = cameraBuffer_;
                    entries[3].size = sizeof(GPUCameraState);

                    entries[4].binding = bind::g0::render_floating;
                    entries[4].buffer = floatingEntityBuffer_;
                    entries[4].size = Dim::TOTAL_FLOATING_SLOTS * sizeof(GPUFloatingEntityState);

                    entries[5].binding = bind::g0::render_light;
                    entries[5].buffer = directionalLightBuffer_;
                    entries[5].size = sizeof(GPUDirectionalLight);

                    entries[6].binding = bind::g0::render_point_lights;
                    entries[6].buffer = pointLightsBuffer_;
                    entries[6].size = sizeof(GPUPointLightArray);

                    entries[7].binding = bind::g0::render_spot_lights;
                    entries[7].buffer = spotLightArrayBuffer_;
                    entries[7].size = sizeof(GPUSpotLightArray);

                    entries[8].binding = bind::g0::patch_instances;
                    entries[8].buffer = patchInstancesBuffer_;
                    entries[8].size = sizeof(GPUPatchInstance) * Dim::MAX_ACTIVE_PATCHES;

                    entries[9].binding = bind::g0::render_ribbon;
                    entries[9].buffer = ribbonBuffer_;
                    entries[9].size = sizeof(GPURibbonState);

                    // Ring transforms (read-only for ribbon VS)
                    entries[10].binding = bind::g0::render_ring_xforms;
                    entries[10].buffer = ringTransformsBuffer_;
                    entries[10].size = sizeof(GPURibbonRingTransform) * Dim::RIBBON_MAX_RINGS;

                    entries[11].binding = bind::g0::tile_grid;
                    entries[11].buffer = tileGridBuffer_;
                    entries[11].size = sizeof(GPUTileGrid);

                    // Entity ground atlas (r32float 256×1 — VS reads ground_y)
                    entries[12].binding = bind::g0::entity_ground_atlas;
                    entries[12].textureView = entityGroundAtlasReadView_;

                    // Visible patch indices (GPU frustum cull output)
                    entries[13].binding = bind::g0::visible_patch_indices;
                    entries[13].buffer = visiblePatchIndicesBuffer_;
                    entries[13].size = Dim::MAX_ACTIVE_PATCHES * sizeof(uint32_t);

                    // Orb state (VS reads per-instance)
                    entries[14].binding = bind::g0::render_orb_state;
                    entries[14].buffer = orbStateBuffer_;
                    entries[14].size = Dim::MAX_ORBS * sizeof(GPUOrbState);

                    // Orb config (radius/palette/tiers; dome_center dead wire)
                    entries[15].binding = bind::g0::orb_config;
                    entries[15].buffer = orbConfigBuffer_;
                    entries[15].size = sizeof(GPUOrbConfig);

                    // Agent tier gains — same buffer as compute binding 111.
                    // Read by pawn_vs in the vertex stage for entity color
                    // (tier_idx → tg.color_r/g/b). Single source of truth
                    // is the C++ AGENT_TIER_GAINS table in bodies/agents.hpp.
                    entries[16].binding = bind::g0::agent_tier_gains;
                    entries[16].buffer = agentTierGainsBuffer_;
                    entries[16].size = GPU_AGENT_TIER_COUNT * sizeof(GPUAgentTierDef);

                    // Pawn figure table (uniform, binding 112). Shares the render
                    // entity layout, so this group MUST bind it or CreateBindGroup fails.
                    entries[17].binding = bind::g0::agent_figure_profiles;
                    entries[17].buffer = figureProfilesBuffer_;
                    entries[17].size = PAWN_FIGURE_COUNT * sizeof(GPUPawnFigure);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Render Entity BindGroup";
                    desc.layout = renderEntityBindGroupLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    renderEntityBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!renderEntityBindGroup_) return false;
                }

                // Mesh gen entity bind group (1 entry: binding 1 = config)
                {
                    std::array<wgpu::BindGroupEntry, 1> entries{};

                    entries[0].binding = bind::g0::config;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Mesh Gen Entity BindGroup";
                    desc.layout = meshGenEntityBindGroupLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    meshGenEntityBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!meshGenEntityBindGroup_) return false;
                }

                // Shadow texture bind group (4 entries: bindings 22-23, 28, 34)
                {
                    std::array<wgpu::BindGroupEntry, 4> entries{};

                    entries[0].binding = bind::g1::bilinear_sampler;
                    entries[0].sampler = bilinearSampler_;

                    entries[1].binding = bind::g1::nearest_sampler;
                    entries[1].sampler = nearestSampler_;

                    entries[2].binding = bind::g1::patch_heightfield_array_read;
                    entries[2].textureView = patchHeightfieldArrayReadView_;

                    entries[3].binding = bind::g1::live_card_read;
                    entries[3].textureView = liveCardView_;

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Shadow Texture BindGroup";
                    desc.layout = shadowTextureBindGroupLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    shadowTextureBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!shadowTextureBindGroup_) return false;
                }

                // Render texture bind group (11 entries: 22-23, 25-29, 31-34)
                {
                    std::array<wgpu::BindGroupEntry, 11> entries{};

                    entries[0].binding = bind::g1::bilinear_sampler;
                    entries[0].sampler = bilinearSampler_;

                    entries[1].binding = bind::g1::nearest_sampler;
                    entries[1].sampler = nearestSampler_;

                    entries[2].binding = bind::g1::shadow_map;
                    entries[2].textureView = shadowMapView_;

                    entries[3].binding = bind::g1::shadow_sampler;
                    entries[3].sampler = shadowSampler_;

                    entries[4].binding = bind::g1::spot_shadow_map;
                    entries[4].textureView = spotShadowMapView_;

                    entries[5].binding = bind::g1::patch_heightfield_array_read;
                    entries[5].textureView = patchHeightfieldArrayReadView_;

                    entries[6].binding = bind::g1::patch_cell_color_array_read;
                    entries[6].textureView = patchCellColorArrayReadView_;

                    entries[7].binding = bind::g1::zone_life_read;
                    entries[7].textureView = zoneLifeReadView_;

                    entries[8].binding = bind::g1::zone_params;
                    entries[8].buffer = zoneConfigBuffer_;
                    entries[8].size = sizeof(GPUGoLZoneArray);

                    entries[9].binding = bind::g1::pawn_aura_read;
                    entries[9].textureView = pawnAuraReadView_;

                    entries[10].binding = bind::g1::live_card_read;
                    entries[10].textureView = liveCardView_;

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Render Texture BindGroup";
                    desc.layout = renderTextureBindGroupLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    renderTextureBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!renderTextureBindGroup_) return false;
                }

                // Compute texture bind group (4 entries: 22 = bilinear_sampler, 23 = nearest_sampler, 33 = pawn_aura_read, 34 = live_card_read)
                {
                    std::array<wgpu::BindGroupEntry, 4> entries{};

                    entries[0].binding = bind::g1::bilinear_sampler;
                    entries[0].sampler = bilinearSampler_;

                    entries[1].binding = bind::g1::nearest_sampler;
                    entries[1].sampler = nearestSampler_;

                    entries[2].binding = bind::g1::pawn_aura_read;
                    entries[2].textureView = pawnAuraReadView_;

                    entries[3].binding = bind::g1::live_card_read;
                    entries[3].textureView = liveCardView_;

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Compute Texture BindGroup";
                    desc.layout = computeTextureBindGroupLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    computeTextureBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!computeTextureBindGroup_) return false;
                }

                // Patch gen bind group (7 entries: binding 1, 23, 24, 25, 27, 28, 30)
                {
                    std::array<wgpu::BindGroupEntry, 7> entries{};

                    entries[0].binding = bind::g0::config;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);

                    entries[1].binding = bind::g0::patch_params;
                    entries[1].buffer = patchParamsBuffer_;
                    entries[1].size = sizeof(GPUPatchParams);

                    entries[2].binding = bind::g0::patch_heightfield_array_write;
                    entries[2].textureView = patchHeightfieldArrayWriteView_;

                    entries[3].binding = bind::g0::tile_grid;
                    entries[3].buffer = tileGridBuffer_;
                    entries[3].size = sizeof(GPUTileGrid);

                    entries[4].binding = bind::g0::patch_cell_color_array_write;
                    entries[4].textureView = patchCellColorArrayWriteView_;

                    entries[5].binding = bind::g0::pyramid_instances;
                    entries[5].buffer = pyramidInstancesBuffer_;
                    entries[5].size = sizeof(GPUPyramidArray);

                    entries[6].binding = bind::g0::patch_height_scratch;  // patch_height_scratch
                    entries[6].buffer = patchHeightScratchBuffer_;
                    entries[6].size = Dim::PATCH_HEIGHTFIELD_N * Dim::PATCH_HEIGHTFIELD_N * 2 * sizeof(float);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Patch Gen BindGroup";
                    desc.layout = patchGenLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    patchGenBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!patchGenBindGroup_) return false;
                }

                // Ribbon compute bind group (3 entries: dedicated ring transform pass)
                {
                    std::array<wgpu::BindGroupEntry, 3> entries{};

                    entries[0].binding = bind::g0::ribbon_state; // ribbon_state (matches @binding(120))
                    entries[0].buffer = ribbonBuffer_;
                    entries[0].size = sizeof(GPURibbonState);

                    entries[1].binding = bind::g0::ring_xforms; // ring_transforms (matches @binding(121))
                    entries[1].buffer = ringTransformsBuffer_;
                    entries[1].size = sizeof(GPURibbonRingTransform) * Dim::RIBBON_MAX_RINGS;

                    entries[2].binding = bind::g0::head_poses; // head_poses (matches @binding(122))
                    entries[2].buffer = headPosesBuffer_;
                    entries[2].size = sizeof(float) * 4 * Dim::RIBBON_MAX_RINGS;

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Ribbon Compute BindGroup";
                    desc.layout = ribbonComputeLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    ribbonComputeBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!ribbonComputeBindGroup_) return false;
                }

                // Gallery entity bind group (4 entries: config + VP + camera + light)
                {
                    std::array<wgpu::BindGroupEntry, 4> entries{};
                    entries[0].binding = bind::g0::config;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);
                    entries[1].binding = bind::g0::render_vp;
                    entries[1].buffer = vpBuffer_;
                    entries[1].size = sizeof(GPUVPMatrix);
                    entries[2].binding = bind::g0::render_camera;
                    entries[2].buffer = cameraBuffer_;
                    entries[2].size = sizeof(GPUCameraState);
                    entries[3].binding = bind::g0::render_light;
                    entries[3].buffer = directionalLightBuffer_;
                    entries[3].size = sizeof(GPUDirectionalLight);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Gallery Entity BindGroup";
                    desc.layout = galleryEntityBindGroupLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    galleryEntityBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!galleryEntityBindGroup_) return false;
                }

                // Photographer render entity bind group (same layout as main, different VP)
                {
                    std::array<wgpu::BindGroupEntry, 18> entries{};
                    entries[0].binding = bind::g0::config;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);
                    entries[1].binding = bind::g0::render_vp;
                    entries[1].buffer = photographerVPBuffer_;  // ← THE DIFFERENCE
                    entries[1].size = sizeof(GPUVPMatrix);
                    entries[2].binding = bind::g0::render_agents;
                    entries[2].buffer = agentStateBuffer_;
                    entries[2].size = Dim::MAX_AGENTS * sizeof(GPUAgentState);
                    entries[3].binding = bind::g0::render_camera;
                    entries[3].buffer = photographerCameraBuffer_;  // ← photographer pos for fog
                    entries[3].size = sizeof(GPUCameraState);
                    entries[4].binding = bind::g0::render_floating;
                    entries[4].buffer = floatingEntityBuffer_;
                    entries[4].size = Dim::TOTAL_FLOATING_SLOTS * sizeof(GPUFloatingEntityState);
                    entries[5].binding = bind::g0::render_light;
                    entries[5].buffer = directionalLightBuffer_;
                    entries[5].size = sizeof(GPUDirectionalLight);
                    entries[6].binding = bind::g0::render_point_lights;
                    entries[6].buffer = pointLightsBuffer_;
                    entries[6].size = sizeof(GPUPointLightArray);
                    entries[7].binding = bind::g0::render_spot_lights;
                    entries[7].buffer = spotLightArrayBuffer_;
                    entries[7].size = sizeof(GPUSpotLightArray);
                    entries[8].binding = bind::g0::patch_instances;
                    entries[8].buffer = patchInstancesBuffer_;
                    entries[8].size = sizeof(GPUPatchInstance) * Dim::MAX_ACTIVE_PATCHES;
                    entries[9].binding = bind::g0::render_ribbon;
                    entries[9].buffer = ribbonBuffer_;
                    entries[9].size = sizeof(GPURibbonState);
                    entries[10].binding = bind::g0::render_ring_xforms;
                    entries[10].buffer = ringTransformsBuffer_;
                    entries[10].size = sizeof(GPURibbonRingTransform) * Dim::RIBBON_MAX_RINGS;
                    entries[11].binding = bind::g0::tile_grid;
                    entries[11].buffer = tileGridBuffer_;
                    entries[11].size = sizeof(GPUTileGrid);
                    // Entity ground atlas (r32float 256×1 — VS reads ground_y)
                    entries[12].binding = bind::g0::entity_ground_atlas;
                    entries[12].textureView = entityGroundAtlasReadView_;

                    // Visible patch indices (GPU frustum cull output)
                    entries[13].binding = bind::g0::visible_patch_indices;
                    entries[13].buffer = visiblePatchIndicesBuffer_;
                    entries[13].size = Dim::MAX_ACTIVE_PATCHES * sizeof(uint32_t);

                    // Orb state (VS reads per-instance) — same buffer as main path
                    entries[14].binding = bind::g0::render_orb_state;
                    entries[14].buffer = orbStateBuffer_;
                    entries[14].size = Dim::MAX_ORBS * sizeof(GPUOrbState);

                    // Orb config (dome_center dead wire) — same buffer as main path
                    entries[15].binding = bind::g0::orb_config;
                    entries[15].buffer = orbConfigBuffer_;
                    entries[15].size = sizeof(GPUOrbConfig);

                    // Agent tier gains — same buffer as main render path.
                    // Required because layout has it at index 18.
                    entries[16].binding = bind::g0::agent_tier_gains;
                    entries[16].buffer = agentTierGainsBuffer_;
                    entries[16].size = GPU_AGENT_TIER_COUNT * sizeof(GPUAgentTierDef);

                    // Pawn figure table (uniform, binding 112) — same shared layout,
                    // so the photographer group must bind it too.
                    entries[17].binding = bind::g0::agent_figure_profiles;
                    entries[17].buffer = figureProfilesBuffer_;
                    entries[17].size = PAWN_FIGURE_COUNT * sizeof(GPUPawnFigure);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Photographer Render Entity BindGroup";
                    desc.layout = renderEntityBindGroupLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    photographerRenderEntityBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!photographerRenderEntityBindGroup_) return false;
                }

                // Photographer compute bind group (9 entries: config + agents + camera + outputs + terrain + patch_grid)
                {
                    std::array<wgpu::BindGroupEntry, 9> entries{};
                    entries[0].binding = bind::g0::config;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);
                    entries[1].binding = bind::g0::agent_state;
                    entries[1].buffer = agentStateBuffer_;
                    entries[1].size = Dim::MAX_AGENTS * sizeof(GPUAgentState);
                    entries[8].binding = bind::g0::camera_state;
                    entries[8].buffer = cameraBuffer_;
                    entries[8].size = sizeof(GPUCameraState);
                    entries[2].binding = bind::g0::photographer_config;
                    entries[2].buffer = photographerConfigBuffer_;
                    entries[2].size = sizeof(GPUPhotographerConfig);
                    entries[3].binding = bind::g0::photographer_vp;
                    entries[3].buffer = photographerVPBuffer_;
                    entries[3].size = sizeof(GPUVPMatrix);
                    entries[4].binding = bind::g0::photographer_camera_out;
                    entries[4].buffer = photographerCameraBuffer_;
                    entries[4].size = sizeof(GPUCameraState);
                    entries[5].binding = bind::g0::photo_heightfield;
                    entries[5].textureView = patchHeightfieldArrayReadView_;
                    entries[6].binding = bind::g0::photo_sampler;
                    entries[6].sampler = bilinearSampler_;
                    entries[7].binding = bind::g0::patch_grid;
                    entries[7].buffer = patchGridBuffer_;
                    entries[7].size = sizeof(GPUPatchGrid);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Photographer Compute BindGroup";
                    desc.layout = photographerComputeLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    photographerComputeBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!photographerComputeBindGroup_) return false;
                }

                // Entity placement compute bind group (heightfield sampling + ground Y correction)
                {
                    std::array<wgpu::BindGroupEntry, 9> entries{};
                    entries[0].binding = bind::g0::config;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);
                    entries[1].binding = bind::g0::photo_painting_slots;
                    entries[1].buffer = paintingSlotsBuffer_;
                    entries[1].size = sizeof(GPUPaintingSlot) * Dim::PAINTING_MAX_SLOTS;
                    entries[2].binding = bind::g0::photo_heightfield;
                    entries[2].textureView = patchHeightfieldArrayReadView_;
                    entries[3].binding = bind::g0::photo_sampler;
                    entries[3].sampler = bilinearSampler_;
                    entries[4].binding = bind::g0::arch_ground;
                    entries[4].buffer = archGroundBuffer_;
                    entries[4].size = sizeof(GPUArchGroundEntry) * Dim::MAX_ARCH_INSTANCES;
                    entries[5].binding = bind::g0::column_ground;
                    entries[5].buffer = columnGroundBuffer_;
                    entries[5].size = sizeof(GPUColumnGroundEntry) * Dim::MAX_COLUMN_INSTANCES;
                    // Combined plant ground: palm[0..23] + cactus[24..43] + blade[44..75]
                    static constexpr uint32_t PLANT_GROUND_COUNT =
                        Dim::MAX_PALM_INSTANCES + Dim::MAX_CACTUS_INSTANCES + Dim::MAX_BLADE_INSTANCES;
                    entries[6].binding = bind::g0::plant_ground;
                    entries[6].buffer = plantComputeGroundBuffer_;
                    entries[6].size = sizeof(GPUPalmGroundEntry) * PLANT_GROUND_COUNT;

                    entries[7].binding = bind::g0::entity_ground_atlas_write;
                    entries[7].textureView = entityGroundAtlasWriteView_;

                    entries[8].binding = bind::g0::patch_grid;
                    entries[8].buffer = patchGridBuffer_;
                    entries[8].size = sizeof(GPUPatchGrid);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Entity Placement Compute BindGroup";
                    desc.layout = entityPlacementComputeLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    entityPlacementComputeBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!entityPlacementComputeBindGroup_) return false;
                }

                // Frustum cull compute bind group (5 entries)
                {
                    std::array<wgpu::BindGroupEntry, 5> entries{};
                    entries[0].binding = bind::g0::config;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);
                    entries[1].binding = bind::g0::vp_data;
                    entries[1].buffer = vpBuffer_;
                    entries[1].size = sizeof(GPUVPMatrix);
                    entries[2].binding = bind::g0::patch_instances;
                    entries[2].buffer = patchInstancesBuffer_;
                    entries[2].size = sizeof(GPUPatchInstance) * Dim::MAX_ACTIVE_PATCHES;
                    entries[3].binding = bind::g0::fc_visible;
                    entries[3].buffer = visiblePatchIndicesBuffer_;
                    entries[3].size = Dim::MAX_ACTIVE_PATCHES * sizeof(uint32_t);
                    entries[4].binding = bind::g0::fc_indirect;
                    entries[4].buffer = frustumComputeBuffer_;
                    entries[4].size = 5 * sizeof(uint32_t);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Frustum Cull Compute BindGroup";
                    desc.layout = frustumCullLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    frustumCullBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!frustumCullBindGroup_) return false;
                }

                // GoL zone compute bind group (5 entries: config + zone state + life tex + derive requests)
                {
                    std::array<wgpu::BindGroupEntry, 5> entries{};

                    entries[0].binding = bind::g0::config;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);

                    // Zone state
                    entries[1].binding = bind::g0::zone_config;
                    entries[1].buffer = zoneConfigBuffer_;
                    entries[1].size = sizeof(GPUGoLZoneArray);
                    entries[2].binding = bind::g0::zone_life;
                    entries[2].buffer = zoneLifeBuffer_;
                    entries[2].size = Dim::MAX_GOL_ZONES * Dim::GOL_ZONE_LIFE_STRIDE * sizeof(float);
                    entries[3].binding = bind::g0::zone_life_tex_write;
                    entries[3].textureView = zoneLifeWriteView_;

                    // Derive request buffer
                    entries[4].binding = bind::g0::zone_derive_requests;
                    entries[4].buffer = zoneDeriveRequestBuffer_;
                    entries[4].size = sizeof(GPUZoneDeriveRequestArray);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "GoL Zone Compute BindGroup";
                    desc.layout = zoneGolComputeLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    zoneGolComputeBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!zoneGolComputeBindGroup_) return false;
                }

                // Pawn aura compute bind group (6 entries: 1, 25, 60, 170-172)
                {
                    std::array<wgpu::BindGroupEntry, 6> entries{};

                    entries[0].binding = bind::g0::config;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);

                    entries[1].binding = bind::g0::agent_state;
                    entries[1].buffer = agentStateBuffer_;
                    entries[1].size = Dim::MAX_AGENTS * sizeof(GPUAgentState);

                    entries[2].binding = bind::g0::pawn_aura_cfg;
                    entries[2].buffer = pawnAuraConfigBuffer_;
                    entries[2].size = sizeof(GPUPawnAuraConfig);

                    entries[3].binding = bind::g0::pawn_aura_cells;
                    entries[3].buffer = pawnAuraCellsBuffer_;
                    entries[3].size = PAWN_AURA_N * PAWN_AURA_N * sizeof(GPUPawnAuraCell);

                    entries[4].binding = bind::g0::pawn_aura_tex_write;
                    entries[4].textureView = pawnAuraWriteView_;

                    entries[5].binding = bind::g0::tile_grid;
                    entries[5].buffer = tileGridBuffer_;
                    entries[5].size = sizeof(GPUTileGrid);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Pawn Aura Compute BindGroup";
                    desc.layout = pawnAuraComputeLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    pawnAuraComputeGroup_ = device_.CreateBindGroup(&desc);
                    if (!pawnAuraComputeGroup_) return false;
                }

                // Live card writer bind group (5 entries: 0, 1, 160, 161, 31)
                {
                    std::array<wgpu::BindGroupEntry, 6> entries{};

                    entries[0].binding = bind::g0::signal;
                    entries[0].buffer = signalBuffer_;
                    entries[0].size = sizeof(GPUFrameSignal);

                    entries[1].binding = bind::g0::config;
                    entries[1].buffer = configBuffer_;
                    entries[1].size = sizeof(GPUDesignConfig);

                    entries[2].binding = bind::g0::zone_config;
                    entries[2].buffer = zoneConfigBuffer_;
                    entries[2].size = sizeof(GPUGoLZoneArray);

                    entries[3].binding = bind::g0::zone_life;
                    entries[3].buffer = zoneLifeBuffer_;
                    entries[3].size = Dim::MAX_GOL_ZONES * Dim::GOL_ZONE_LIFE_STRIDE * sizeof(float);

                    entries[4].binding = bind::g0::live_card_write;
                    entries[4].textureView = liveCardWriteView_;

                    entries[5].binding = bind::g0::live_card_scratch;
                    entries[5].buffer = liveCardScratchBuffer_;
                    entries[5].size = Dim::LIVE_CARD_SIZE * Dim::LIVE_CARD_SIZE * 2 * sizeof(float);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Live Card Writer BindGroup";
                    desc.layout = liveCardWriterLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    liveCardWriterGroup_ = device_.CreateBindGroup(&desc);
                    if (!liveCardWriterGroup_) return false;
                }

                // Zone mask bind group (5 entries: 1, 25, 160, 161, 166)
                {
                    std::array<wgpu::BindGroupEntry, 5> entries{};

                    entries[0].binding = bind::g0::config;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);

                    entries[1].binding = bind::g0::tile_grid;
                    entries[1].buffer = tileGridBuffer_;
                    entries[1].size = sizeof(GPUTileGrid);

                    entries[2].binding = bind::g0::zone_config;
                    entries[2].buffer = zoneConfigBuffer_;
                    entries[2].size = sizeof(GPUGoLZoneArray);

                    entries[3].binding = bind::g0::zone_life;
                    entries[3].buffer = zoneLifeBuffer_;
                    entries[3].size = Dim::MAX_GOL_ZONES * Dim::GOL_ZONE_LIFE_STRIDE * sizeof(float);

                    entries[4].binding = bind::g0::zone_derive_requests;
                    entries[4].buffer = zoneDeriveRequestBuffer_;
                    entries[4].size = sizeof(GPUZoneDeriveRequestArray);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Zone Mask BindGroup";
                    desc.layout = zoneMaskLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    zoneMaskGroup_ = device_.CreateBindGroup(&desc);
                    if (!zoneMaskGroup_) return false;
                }

                // Orb compute bind group (3 entries: state storage rw + config uniform)
                {
                    std::array<wgpu::BindGroupEntry, 3> entries{};

                    entries[0].binding = bind::g0::orb_state;
                    entries[0].buffer = orbStateBuffer_;
                    entries[0].size = Dim::MAX_ORBS * sizeof(GPUOrbState);

                    entries[1].binding = bind::g0::orb_config;
                    entries[1].buffer = orbConfigBuffer_;
                    entries[1].size = sizeof(GPUOrbConfig);

                    entries[2].binding = bind::g0::orb_state_prev;
                    entries[2].buffer = orbStatePrevBuffer_;
                    entries[2].size = Dim::MAX_ORBS * sizeof(GPUOrbState);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Orb Compute BindGroup";
                    desc.layout = orbComputeLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    orbComputeGroup_ = device_.CreateBindGroup(&desc);
                    if (!orbComputeGroup_) return false;
                }

                // Orb copy bind group — bindings 413/411/414 alias the same
                // physical buffers as the main layout's 410/411/412, just
                // through read-only / read-write views that match the
                // dedicated WGSL declarations used by orb_state_prev_copy.
                {
                    std::array<wgpu::BindGroupEntry, 3> entries{};

                    entries[0].binding = bind::g0::orb_state_ro;
                    entries[0].buffer = orbStateBuffer_;
                    entries[0].size = Dim::MAX_ORBS * sizeof(GPUOrbState);

                    entries[1].binding = bind::g0::orb_config;
                    entries[1].buffer = orbConfigBuffer_;
                    entries[1].size = sizeof(GPUOrbConfig);

                    entries[2].binding = bind::g0::orb_state_prev_rw;
                    entries[2].buffer = orbStatePrevBuffer_;
                    entries[2].size = Dim::MAX_ORBS * sizeof(GPUOrbState);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Orb Copy BindGroup";
                    desc.layout = orbCopyLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    orbCopyGroup_ = device_.CreateBindGroup(&desc);
                    if (!orbCopyGroup_) return false;
                }

                // Arch mesh gen bind group (dedicated layout — bindings 193-195)
                {
                    std::array<wgpu::BindGroupEntry, 3> entries{};

                    entries[0].binding = bind::g0::amg_params;
                    entries[0].buffer = archMeshParamsBuffer_;
                    entries[0].size = sizeof(GPUArchMeshParams) * Dim::MAX_ARCH_INSTANCES;

                    entries[1].binding = bind::g0::amg_vertices;
                    entries[1].buffer = archVertexBuffer_;
                    entries[1].size = Dim::AMG_TOTAL_VERTICES * sizeof(ArchVertex);

                    entries[2].binding = bind::g0::amg_indices;
                    entries[2].buffer = archIndexBuffer_;
                    entries[2].size = Dim::AMG_TOTAL_INDICES * sizeof(uint32_t);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Arch Mesh Gen BindGroup";
                    desc.layout = archMeshGenLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    archMeshGenBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!archMeshGenBindGroup_) return false;
                }

                // The agents' room bind group (group 2) — the two occupier
                // windows, bound once at boot from the existing buffers.
                {
                    std::array<wgpu::BindGroupEntry, 2> entries{};

                    entries[0].binding = bind::g2::occupier_cmg;
                    entries[0].buffer = columnMeshParamsBuffer_;
                    entries[0].size = sizeof(GPUColumnMeshParams) * Dim::MAX_COLUMN_INSTANCES;

                    entries[1].binding = bind::g2::occupier_amg;
                    entries[1].buffer = archMeshParamsBuffer_;
                    entries[1].size = sizeof(GPUArchMeshParams) * Dim::MAX_ARCH_INSTANCES;

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Agent Occupier BindGroup";
                    desc.layout = agentOccupierLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    agentOccupierBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!agentOccupierBindGroup_) return false;
                }

                // Column mesh gen bind group (dedicated layout — bindings 190/191 + 196-198)
                {
                    std::array<wgpu::BindGroupEntry, 5> entries{};

                    entries[0].binding = bind::g0::cmg_params;
                    entries[0].buffer = columnMeshParamsBuffer_;
                    entries[0].size = sizeof(GPUColumnMeshParams) * Dim::MAX_COLUMN_INSTANCES;

                    entries[1].binding = bind::g0::cmg_vertices;
                    entries[1].buffer = columnVertexBuffer_;
                    entries[1].size = Dim::CMG_TOTAL_VERTICES * sizeof(ArchVertex);

                    entries[2].binding = bind::g0::cmg_indices;
                    entries[2].buffer = columnIndexBuffer_;
                    entries[2].size = Dim::CMG_TOTAL_INDICES * sizeof(uint32_t);

                    entries[3].binding = bind::g0::cmg_config;
                    entries[3].buffer = configBuffer_;
                    entries[3].size = sizeof(GPUDesignConfig);

                    entries[4].binding = bind::g0::cmg_column_ground;
                    entries[4].buffer = columnGroundBuffer_;
                    entries[4].size = sizeof(GPUColumnGroundEntry) * Dim::MAX_COLUMN_INSTANCES;

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Column Mesh Gen BindGroup";
                    desc.layout = columnMeshGenLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    columnMeshGenBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!columnMeshGenBindGroup_) return false;
                }

                // Palm mesh gen bind group
                {
                    std::array<wgpu::BindGroupEntry, 3> entries{};

                    entries[0].binding = bind::g0::palmg_params;
                    entries[0].buffer = palmMeshParamsBuffer_;
                    entries[0].size = Dim::MAX_PALM_INSTANCES * sizeof(GPUPalmMeshParams);

                    entries[1].binding = bind::g0::palmg_vertices;
                    entries[1].buffer = palmVertexBuffer_;
                    entries[1].size = Dim::PALMG_TOTAL_VERTICES * sizeof(ArchVertex);

                    entries[2].binding = bind::g0::palmg_indices;
                    entries[2].buffer = palmIndexBuffer_;
                    entries[2].size = Dim::PALMG_TOTAL_INDICES * sizeof(uint32_t);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Palm Mesh Gen BindGroup";
                    desc.layout = palmMeshGenLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    palmMeshGenBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!palmMeshGenBindGroup_) return false;
                }

                // Cactus mesh gen bind group
                {
                    std::array<wgpu::BindGroupEntry, 3> entries{};
                    entries[0].binding = bind::g0::cactusg_params;
                    entries[0].buffer = cactusMeshParamsBuffer_;
                    entries[0].size = Dim::MAX_CACTUS_INSTANCES * sizeof(GPUCactusMeshParams);
                    entries[1].binding = bind::g0::cactusg_vertices;
                    entries[1].buffer = cactusVertexBuffer_;
                    entries[1].size = Dim::CACTUSG_TOTAL_VERTICES * sizeof(ArchVertex);
                    entries[2].binding = bind::g0::cactusg_indices;
                    entries[2].buffer = cactusIndexBuffer_;
                    entries[2].size = Dim::CACTUSG_TOTAL_INDICES * sizeof(uint32_t);
                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Cactus Mesh Gen BindGroup";
                    desc.layout = cactusMeshGenLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    cactusMeshGenBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!cactusMeshGenBindGroup_) return false;
                }

                // Blade mesh gen bind group
                {
                    std::array<wgpu::BindGroupEntry, 3> entries{};
                    entries[0].binding = bind::g0::bladeg_params;
                    entries[0].buffer = bladeMeshParamsBuffer_;
                    entries[0].size = Dim::MAX_BLADE_INSTANCES * sizeof(GPUBladeClusterMeshParams);
                    entries[1].binding = bind::g0::bladeg_vertices;
                    entries[1].buffer = bladeVertexBuffer_;
                    entries[1].size = Dim::BLADEG_TOTAL_VERTICES * sizeof(ArchVertex);
                    entries[2].binding = bind::g0::bladeg_indices;
                    entries[2].buffer = bladeIndexBuffer_;
                    entries[2].size = Dim::BLADEG_TOTAL_INDICES * sizeof(uint32_t);
                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Blade Mesh Gen BindGroup";
                    desc.layout = bladeMeshGenLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    bladeMeshGenBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!bladeMeshGenBindGroup_) return false;
                }

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
                config_.cube_plasticity = Idle::CUBE_PLASTICITY_DEFAULT;  // CONTACT_3 K2c: boot-pin the live λ master; rest 0.6
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

                GPUCameraState camera{};
                camera.pos[0] = Idle::CAMERA_POS_X;
                camera.pos[1] = Idle::CAMERA_POS_Y;
                camera.pos[2] = Idle::CAMERA_POS_Z;
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