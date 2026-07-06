#pragma once

// THE_CHORD CARTRIDGE — GPU State Management (Rasterized)
// =============================================================
//
// CPU/GPU data contract: structs, buffers, textures, bind groups.
// Terrain grid is GPU-derived from vertex_index — zero geometry uploaded.
//
// See world.wgsl for the GPU scroll (single source of truth).
//
// ─── BINDING MAP ────────────────────────────────────────────────────────────
//
//   Binding   Entity               Compute          Render
//   ───────   ──────               ───────          ──────
//   0         frame_signal         Storage          —
//   1         design_config        Uniform          —
//   2         vp_matrix            Storage          —
//   20        terrain_state        Storage          —
//   21        (reserved)           —                —
//   25        tile_grid            Uniform          —
//   26        solid_instances      Uniform          —
//   40        (reserved)           —                —
//   60        agent_state          Storage          —    (unified entity buffer, slot 0 = player)
//   80        camera_state         Storage          —
//   100       sphere_state         Storage          —
//   101       trajectories         Storage          —
//   120       ribbon_state         Uniform          —
//   121       ring_transforms      Storage          —
//   200       frame_signal         —                ReadOnlyStorage
//   201       vp_matrix            —                ReadOnlyStorage
//   220       terrain_state        —                ReadOnlyStorage
//   260       agent_state          —                ReadOnlyStorage  (unified; VS reads via possessed_slot)
//   280       camera_state         —                ReadOnlyStorage
//   300       sphere_state         —                ReadOnlyStorage
//   320       directional_light    —                ReadOnlyStorage
//   321       point_lights         —                ReadOnlyStorage
//   340       patch_instances      —                ReadOnlyStorage
//   360       ribbon_state         —                ReadOnlyStorage
//   361       ring_transforms      —                ReadOnlyStorage
//
//   Texture bindings (Group 1):
//     Compute write: 0-2 (reserved — formerly legacy heightfield/color stubs)
//     Render read:   22=bilinear_sampler, 23=nearest_sampler,
//                    25=shadow_map, 26=shadow_sampler,
//                    28=patch_heightfield, 29=patch_cells
//     Render read:   20-21, 24 (reserved — formerly legacy texture reads)
//     Mesh gen:      40-45 (reserved — formerly legacy cell mesh gen)
//     Patch gen:     23=params, 24=heightfield_write, 25=tile_grid,
//                    26=solids, 27=cell_color_write
//     Terrain IB:    22=terrain_index_buffer
//

#include "analysis/analysis_signal.hpp"
#include <webgpu/webgpu_cpp.h>
#include <cstring>
#include <array>
#include <vector>
#include <cmath>

namespace t7 {
    namespace the_chord {

        // =====================================================================
        // S1 DIMENSIONS — Grid sizes, mesh resolutions, buffer capacities
        // =====================================================================

        namespace Dim {
            // Grid dimensions
            // (bindings 21, 40 reserved — formerly proximity_field, cell_states)
            constexpr int      MAX_TRAJECTORIES = 16;

            // Terrain mesh — GPU-derived grid, GPU-generated index buffer
            constexpr uint32_t TERRAIN_MESH_N = 256;
            constexpr uint32_t TERRAIN_MESH_VERTS = (TERRAIN_MESH_N + 1) * (TERRAIN_MESH_N + 1);
            constexpr uint32_t TERRAIN_INDEX_COUNT = TERRAIN_MESH_N * TERRAIN_MESH_N * 6;

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

            // (legacy cell mesh constants removed — bindings 40-45 reserved)
            // Unified pier system (deterministic slot addressing)
            constexpr uint32_t PIER_TEST_RIG_BASE = 0;     // slots 0-2 (ramp, plateau, block)
            constexpr uint32_t PIER_TEST_RIG_COUNT = 3;
            constexpr uint32_t PIER_ARCH_BASE = 4;          // slots 4-35 (16 arches × 2 piers)
            constexpr uint32_t PIER_COLUMN_BASE = 36;       // slots 36-67 (32 columns × 1 pier)
            constexpr uint32_t PIER_TOTAL = 68;

            // Patch streaming system
            constexpr float    PATCH_EXTENT = 50.0f;    // world units per patch side
            constexpr uint32_t PATCH_HEIGHTFIELD_N = 256;     // texels per patch heightfield side
            constexpr uint32_t PATCH_CELL_N = 16;      // cell color texture side per patch
            constexpr uint32_t PATCH_GRID_RADIUS = 3;       // inner priority radius (7×7)
            constexpr uint32_t PATCH_GRID_SIDE = 2 * PATCH_GRID_RADIUS + 1;       // 7
            constexpr uint32_t PATCH_RENDER_RADIUS = 5;       // outer render radius (11×11, 250 world units)
            constexpr uint32_t PATCH_RENDER_SIDE = 2 * PATCH_RENDER_RADIUS + 1;     // 11
            constexpr uint32_t PATCH_PREGEN_RADIUS = 7;                                // deep pre-gen buffer (15×15, 350 world units)
            constexpr uint32_t PATCH_PREGEN_SIDE = 2 * PATCH_PREGEN_RADIUS + 1;     // 15
            constexpr uint32_t MAX_ACTIVE_PATCHES = PATCH_PREGEN_SIDE * PATCH_PREGEN_SIDE; // 225
            constexpr uint32_t PATCH_MESH_N = 64;      // mesh subdivisions per patch (LOD-0)
            constexpr uint32_t PATCH_INDEX_COUNT = PATCH_MESH_N * PATCH_MESH_N * 6;
            constexpr uint32_t PATCH_MESH_N_LOD1 = 32;  // LOD-1: half resolution
            constexpr uint32_t PATCH_INDEX_COUNT_LOD1 = PATCH_MESH_N_LOD1 * PATCH_MESH_N_LOD1 * 6;

            // Mathematical constants
            constexpr float    PI = 3.14159265359f;

            // Atmosphere
            constexpr float    FOG_COLOR_R = 0.85f;
            constexpr float    FOG_COLOR_G = 0.78f;
            constexpr float    FOG_COLOR_B = 0.72f;

            // Lighting
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

            // Generative pyramids — GPU mesh gen (slot-based addressing)
            constexpr uint32_t MAX_PYRAMID_INSTANCES = 8;
            constexpr uint32_t PMG_MAX_VERTS_PER_SLOT = 36;   // truncated: 12 tris × 3 (sides + top + bottom)
            constexpr uint32_t PMG_MAX_INDICES_PER_SLOT = 36;  // unindexed triangles (1:1 vert:idx)
            constexpr uint32_t PMG_TOTAL_VERTICES = MAX_PYRAMID_INSTANCES * PMG_MAX_VERTS_PER_SLOT;   // 288
            constexpr uint32_t PMG_TOTAL_INDICES = MAX_PYRAMID_INSTANCES * PMG_MAX_INDICES_PER_SLOT; // 288

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
            constexpr uint32_t GROUND_ATLAS_PYRAMID = 48;   //  8 slots
            constexpr uint32_t GROUND_ATLAS_PALM = 56;   // 24 slots
            constexpr uint32_t GROUND_ATLAS_CACTUS = 80;   // 20 slots
            constexpr uint32_t GROUND_ATLAS_BLADE = 100;  // 32 slots
            constexpr uint32_t GROUND_ATLAS_USED = 132;

            // GoL zone system — per-zone automaton grids
            constexpr uint32_t MAX_GOL_ZONES = 8;

            // Floating entity system — split into sphere (orbital) + cube (hover-bob)
            constexpr uint32_t MAX_SPHERE_INSTANCES = 8;
            // Phase 3 bumped MAX_CUBE_INSTANCES from 64 to 256 to support the
            // drone-show aesthetic: large coordinated populations rather than
            // sparse individual flyers. update_cube is a single-thread kernel
            // (@workgroup_size(1)) so cost scales linearly — at 256 slots
            // with ~30 ops/slot, ~7.5K ops/frame, well within budget.
            constexpr uint32_t MAX_CUBE_INSTANCES = 256;
            constexpr uint32_t CUBE_SLOT_OFFSET = MAX_SPHERE_INSTANCES;
            constexpr uint32_t TOTAL_FLOATING_SLOTS = MAX_SPHERE_INSTANCES + MAX_CUBE_INSTANCES;  // 264
            constexpr uint32_t GOL_ZONE_GRID = 32;      // cells per zone side
            constexpr uint32_t GOL_ZONE_CELLS = GOL_ZONE_GRID * GOL_ZONE_GRID;  // 1024
            constexpr uint32_t GOL_ZONE_LIFE_STRIDE = GOL_ZONE_CELLS * 7;  // 7 slots: visual, velocity, target, next, height_factor, color_visual, color_velocity

            // Zone cell mesh extrusion budget
            constexpr uint32_t ZONE_MESH_MAX_VERTICES = 50000;
            constexpr uint32_t ZONE_MESH_MAX_INDICES = 75000;
            constexpr uint32_t MAX_ZONE_MESH_VERTICES = 200000;  // 8 zones × up to 1024 cells × 5 quads × 4 verts
            constexpr uint32_t MAX_ZONE_MESH_INDICES = 300000;

            // Orb sky layer — luminous points on a dome above the world
            constexpr uint32_t MAX_ORBS = 256;

            // Agent system — unified entity layer. Slot 0 is the player's
            // body; slots 1..MAX_AGENTS-1 are mood-authored agents. See
            // modules/agents.inl and agent_system_design.md.
            constexpr uint32_t MAX_AGENTS = 32;
        }


        // =====================================================================
        // S2 IDLE — Default values for state initialization
        // =====================================================================

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
            constexpr float SPHERE_RADIUS = 2.5f;
            constexpr float SPHERE_ORBIT_RADIUS = 25.0f;
            constexpr float SPHERE_ORBIT_SPEED = 0.3f;
            constexpr float SPHERE_HOVER_HEIGHT = 8.0f;
            constexpr float SPHERE_INFLUENCE_RADIUS = 12.0f;
            constexpr float TRAJECTORY_VALUE = 1.0f;
            constexpr float TRAJECTORY_VELOCITY = 0.0f;
            constexpr float TRAJECTORY_FIELD_VALUE = 0.0f;
            constexpr float TRAJECTORY_FIELD_VELOCITY = 0.0f;
            constexpr float WAVE_TIME_SCALE = 1.0f;
            constexpr float PAWN_SPEED = 15.0f;
            constexpr float CAMERA_SENSITIVITY = 0.005f;
            constexpr float ACTIVE_CELL_SIZE = 64.0f;
        }


        // =====================================================================
        // S3 COUPLING BITS — Bitmask flags for entity-to-entity wires
        // =====================================================================

        // DONE[state:L1] reserved-slot annotations mirrored from
        //   world.wgsl §2 (lines 1675–1696). MUST match those bit
        //   values 1:1 — semantic drift here would corrupt every
        //   GPU-side coupling read silently. Reserved slots stay
        //   declared because their bits flow through legacy code
        //   paths; reusing them needs cross-side coordination.
        namespace Coupling {
            constexpr uint32_t POLYPHONY_TO_AMPLITUDE    = 1u << 0;
            constexpr uint32_t TERRAIN_TO_PAWN_Y         = 1u << 1;
            constexpr uint32_t TERRAIN_TO_PAWN_TILT      = 1u << 2;
            constexpr uint32_t PAWN_TO_CAMERA_TARGET     = 1u << 3;
            constexpr uint32_t INPUT_MOVES_PAWN          = 1u << 4;
            constexpr uint32_t INPUT_ORBITS_CAMERA       = 1u << 5;
            constexpr uint32_t INPUT_ZOOMS_CAMERA        = 1u << 6;
            constexpr uint32_t PAWN_TO_FIELD_COLOR       = 1u << 7;   // (reserved — legacy proximity field)
            constexpr uint32_t SPHERE_TO_FIELD_COLOR     = 1u << 8;   // (reserved — legacy proximity field)
            constexpr uint32_t POLYPHONY_TO_CELL_COLOR   = 1u << 9;   // (reserved — legacy cell system)
            constexpr uint32_t PAWN_TO_CELL_COLOR        = 1u << 10;  // (reserved — legacy cell system)
            constexpr uint32_t SPHERE_TO_CELL_COLOR      = 1u << 11;  // (reserved — legacy cell system)
            constexpr uint32_t POLYPHONY_TO_SPHERE_COLOR = 1u << 12;
            constexpr uint32_t SPHERE_TO_TERRAIN_TINT    = 1u << 13;
            constexpr uint32_t TERRAIN_TO_SPHERE_HEIGHT  = 1u << 14;
            constexpr uint32_t RANDOM_TO_CELL_GOALS      = 1u << 15;  // (reserved — legacy cell system)
            constexpr uint32_t PAWN_TO_SUN_VP            = 1u << 16;
            constexpr uint32_t PAWN_TO_ZONE_HEIGHT       = 1u << 17;
            constexpr uint32_t PAWN_TO_ZONE_COLOR        = 1u << 18;
            constexpr uint32_t SPHERE_TO_ZONE_HEIGHT     = 1u << 19;
            constexpr uint32_t SPHERE_TO_ZONE_COLOR      = 1u << 20;
            // Naming alignment with WGSL (same bit values).
            constexpr uint32_t PAWN_TO_PROXIMITY_FIELD   = PAWN_TO_FIELD_COLOR;
            constexpr uint32_t SPHERE_TO_PROXIMITY_FIELD = SPHERE_TO_FIELD_COLOR;
            constexpr uint32_t ALL = 0x1FFFFFu;
            constexpr uint32_t NONE = 0u;
            constexpr uint32_t STIMULUS = POLYPHONY_TO_AMPLITUDE | PAWN_TO_FIELD_COLOR | SPHERE_TO_FIELD_COLOR | POLYPHONY_TO_CELL_COLOR | PAWN_TO_CELL_COLOR | SPHERE_TO_CELL_COLOR | POLYPHONY_TO_SPHERE_COLOR | SPHERE_TO_TERRAIN_TINT;
            constexpr uint32_t DERIVED = TERRAIN_TO_PAWN_Y | TERRAIN_TO_PAWN_TILT | PAWN_TO_CAMERA_TARGET | TERRAIN_TO_SPHERE_HEIGHT | PAWN_TO_SUN_VP;
            constexpr uint32_t ACCUMULATED = INPUT_MOVES_PAWN | INPUT_ORBITS_CAMERA | INPUT_ZOOMS_CAMERA;
            constexpr uint32_t SIGNAL = POLYPHONY_TO_AMPLITUDE | POLYPHONY_TO_CELL_COLOR | POLYPHONY_TO_SPHERE_COLOR;
            constexpr uint32_t TERRAIN = TERRAIN_TO_PAWN_Y | TERRAIN_TO_PAWN_TILT | TERRAIN_TO_SPHERE_HEIGHT;
            constexpr uint32_t INPUT = INPUT_MOVES_PAWN | INPUT_ORBITS_CAMERA | INPUT_ZOOMS_CAMERA;
            constexpr uint32_t CAMERA = PAWN_TO_CAMERA_TARGET | INPUT_ORBITS_CAMERA | INPUT_ZOOMS_CAMERA;
            constexpr uint32_t FIELD = PAWN_TO_FIELD_COLOR | SPHERE_TO_FIELD_COLOR;
            constexpr uint32_t CELLS = POLYPHONY_TO_CELL_COLOR | PAWN_TO_CELL_COLOR | SPHERE_TO_CELL_COLOR | RANDOM_TO_CELL_GOALS;
            constexpr uint32_t PAWN_INFLUENCE = PAWN_TO_FIELD_COLOR | PAWN_TO_CELL_COLOR | PAWN_TO_ZONE_HEIGHT | PAWN_TO_ZONE_COLOR;
            constexpr uint32_t SPHERE_INFLUENCE = SPHERE_TO_FIELD_COLOR | SPHERE_TO_CELL_COLOR | SPHERE_TO_TERRAIN_TINT | SPHERE_TO_ZONE_HEIGHT | SPHERE_TO_ZONE_COLOR;
            constexpr uint32_t SPHERE_APPEARANCE = POLYPHONY_TO_SPHERE_COLOR;
            constexpr uint32_t ZONE = PAWN_TO_ZONE_HEIGHT | PAWN_TO_ZONE_COLOR | SPHERE_TO_ZONE_HEIGHT | SPHERE_TO_ZONE_COLOR;
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
            float _pad1;
            // Sky mode: ribbon head pose handed to the GPU pawn update so the
            // possessed pawn snaps onto the flown head. SEAM[ribbon:sky-mode].
            uint32_t sky_mode;       // 0 = grounded, 1 = mounted on the ribbon head
            float    sky_head_x;
            float    sky_head_y;
            float    sky_head_z;
            float    sky_heading;
            float    _pad2;
            float    _pad3;
            float    _pad4;
        };

        struct alignas(16) GPUDesignConfig {
            // ─── Debug mutes ────────────────────────────────────────
            uint32_t mute_dynamics_0d;
            uint32_t mute_dynamics_2d;
            uint32_t mute_signal;
            uint32_t mute_couplings;

            // ─── Interaction ────────────────────────────────────────
            float wave_time_scale;
            float pawn_speed;
            float camera_sensitivity;
            uint32_t freeze_sphere;
            float active_cell_size;
            uint32_t fpv_mode;

            // ─── Terrain wave control ───────────────────────────────
            uint32_t wave_enable_mask;        // per-band enable bits
            uint32_t wave_freeze_mask;        // per-band freeze bits
            float wave_frozen_t[3];           // frozen time per band triplet
            uint32_t world_seed;              // master seed for GPU-side terrain/zone generation

            // ─── Lighting & atmosphere ──────────────────────────────
            float sun_direction[3];
            float aura_enabled;               // 0.0 = off, 1.0 = on (guards all aura sampling)
            float pawn_amp_scale;
            float pawn_height_bias;
            float pawn_aura_height;           // 0.0 = no aura extrusion, >0 = world units of rise
            float fog_density;                // exponential fog coefficient (default 0.003)
            float fog_color[3];               // fog/sky color RGB

            // ─── Transition overlay ─────────────────────────────────
            float fade_alpha;                 // 0.0 = no overlay, 1.0 = fully opaque
            float fade_color[3];              // transition overlay RGB

            // ─── World structure ────────────────────────────────────
            uint32_t pier_count;              // active pier count for bounded iteration
            float world_bound_min[2];         // XZ min clamp (0,0 = infinite)
            float world_bound_max[2];         // XZ max clamp (0,0 = infinite)
            uint32_t placement_patch_count;   // active patches for entity Y-correction (decoupled from photographer)
            float terrain_amp_ceiling;        // max per-wave amplitude (0 = unlimited, >0 = clamp for indoor)
            float ceiling_height;             // indoor ceiling Y (0 = no ceiling, >0 = camera Y clamp)
            float terrain_time;               // t_beats for terrain evaluation (0 = frozen, >0 = animated)

            // ─── Polyphony-driven band motion ────────────────────────────
            // Per-band blend factor: -1.0 = use activity field (default),
            // [0,1] = direct blend between frozen and moving terrain.
            // CPU smoothly ramps these based on polyphony count.
            // Indexed as band_blend[i]: 0=continental, 1=regional, 2=local,
            //                           3=detail, 4=fine, 5=tectonic
            float band_blend_0;               // [0] continental
            float band_blend_1;               // [1] regional
            float band_blend_2;               // [2] local
            float band_blend_3;               // [3] detail
            float band_blend_4;               // [4] fine
            float band_blend_5;               // [5] tectonic
            // Per-band phase origin (t_beats when band activated).
            // Moving phase = phase_base + (t_beats - origin) * freq,
            // so at activation moment the moving phase equals frozen phase.
            // Indexed as band_phase_origin[i], same band order as above.
            float band_phase_origin_0;        // [0] continental
            float band_phase_origin_1;        // [1] regional
            float band_phase_origin_2;        // [2] local
            float band_phase_origin_3;        // [3] detail
            float band_phase_origin_4;        // [4] fine
            float band_phase_origin_5;        // [5] tectonic

            // ─── Musical animation modes ─────────────────────────────────
            // Each mode is an independently toggleable coupling circuit.
            // Values are [0,1] intensity, driven by polyphony through trajectory ramp.
            float mode_color_shift;           // bias added to smooth→discrete mode field
            float mode_checker_scatter;       // bias subtracted from sparse survival threshold
            float mode_palette_target;        // [0,3] target palette index (0=sand 1=salmon 2=green 3=grey)
            float mode_palette_intensity;     // [0,1] how strongly to override spatial palette weights
            float mode_discrete_tier;         // [0,4] target discrete tier (0=color 1=tinted 2=BW 3=chessBW 4=chessColor)
            float mode_gol_tick_scale;        // tick period multiplier (1.0=normal, <1=faster, >1=slower)
            float mode_gol_height_scale;      // alive_height multiplier (1.0=normal, >1=taller)
            // ─── Floater system dials ────────────────────────────────────
            // System-level coordination knob for cube behaviors. At 0.0
            // each cube runs its behavior with maximum individual variation
            // (independent phases / high-spatial-frequency noise samples).
            // At 1.0 cubes lock to shared parameters (synchronized phases
            // / low-frequency shared noise samples). The transition is
            // a continuous lerp inside each behavior's force function.
            // See modules/cube_behaviors.inl for behavior-by-behavior wiring.
            // Repurposes the previous _pad_mode_3 slot — no struct growth.
            float floater_coordination;

            // ─── Radial pulse ring buffer ────────────────────────────────
            // 8 recent note onsets, indexed as pulse_data[i*4 + field]:
            //   field 0 = origin_x, 1 = origin_z, 2 = onset_beats, 3 = amplitude
            // Evaluated in terrain VS + behavior_player_controlled as expanding ring wavefronts.
            uint32_t pulse_count;             // active entries (0–8)
            // ─── Agent system ────────────────────────────────────────────
            // Slot index of the player's current body in agent_state[].
            // Piggybacks on the existing _pulse_pad triple (kept size 384).
            uint32_t possessed_slot;          // slot 0 at session start
            float _pulse_pad[2];
            float pulse_data[32];             // 8 × {origin_x, origin_z, onset_beats, amplitude}
            // ─── LOD-band pawn position ─────────────────────────────────
            // The CPU bands patches into LOD0/LOD1 in stream_patches based
            // on player_.readback_x/z, which lags the GPU pawn by 1-2 frames.
            // The GPU's frustum-cull shader applies the LOD0 distance
            // gate, but if it reads the live GPU pawn position the CPU's
            // banding and the GPU's gate disagree at the boundary annulus
            // (~3.5 patches × 50 = ~175 world units from the pawn). The
            // disagreement makes patches flicker in/out and z-fight as the
            // camera moves. Solution: push the CPU's banding pawn here
            // and have the cull shader read it instead, so both sides
            // partition with the same yardstick by construction.
            float lod_pawn_x;
            float lod_pawn_z;
            float _lod_pawn_pad[2];           // pad to vec4 alignment
        };

        // Tile grid: modifier field for smooth archetype interpolation.
        // The archetype index (0–2) is carried for downstream consumers
        // (cell behavior, entity color anchoring) that need discrete terrain type.
        struct alignas(16) GPUTileGridEntry {
            float amp_scale;
            float height_bias;
            float activation_scale;
            uint32_t archetype;        // 0=mountainous, 1=varied, 2=basin, 3=pool
        };
        static_assert(sizeof(GPUTileGridEntry) == 16, "GPUTileGridEntry must be 16 bytes");

        static constexpr uint32_t TILE_GRID_SIDE = 2 * (Dim::PATCH_PREGEN_RADIUS + 1) + 1;  // 17 (pregen + 1 pad each side)
        static constexpr uint32_t TILE_GRID_MAX = TILE_GRID_SIDE * TILE_GRID_SIDE;  // 289
        struct alignas(16) GPUTileGrid {
            int32_t origin_x;          // grid-space X of entry [0][0]
            int32_t origin_z;          // grid-space Z of entry [0][0]
            uint32_t side;             // grid dimension (13)
            float cell_extent;         // world units per cell (50.0)
            GPUTileGridEntry entries[TILE_GRID_MAX];
        };
        static_assert(sizeof(GPUTileGrid) == 16 + TILE_GRID_MAX * 16, "GPUTileGrid must match WGSL layout");

        struct alignas(16) GPUTrajectory {
            float value;
            float velocity;
            float _pad0;
            float _pad1;
        };

        struct alignas(16) GPUTerrainState {
            float amplitude_scale;
            float max_amplitude;
            float size;
            float lipschitz_factor;
            float tint[3];
            float _pad;
        };

        // Unified entity state — the player's body and all mood-authored
        // agents share this layout. Slot 0 is the player (possessed on
        // session start); slots 1..MAX_AGENTS-1 are mood-spawned.
        //
        // Scalar fields throughout (no vec3) so WGSL uniform/storage
        // layout matches C++ without vec3 alignment surprises.
        //
        // Orientation is stored (not derived) because the player's body
        // renders with a terrain-tilt quaternion written by
        // behavior_player_controlled; keeping it per-slot keeps the tilt
        // out of the vertex shader.
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
            uint32_t behavior_id;  // 48 — AgentBehaviorId (see modules/agents.inl)
            uint32_t tier_idx;     // 52 — AgentTierId     (see modules/agents.inl)
            uint32_t is_active;    // 56 — 0 = inactive (collapsed in VS + skipped in update)
            int32_t  portal_trigger; // 60 — only meaningful on the possessed slot (-1 = none)
            float orient_x;        // 64 — heading ⊗ tilt quaternion
            float orient_y;        // 68
            float orient_z;        // 72
            float orient_w;        // 76
        };                         // 80 total

        // ─── Agent registry GPU structs ──────────────────────────────
        //
        // PAIRED DECLARATIONS — KEEP IN SYNC:
        //   GPUAgentBehaviorDef (C++, here)        ↔ AgentBehaviorParams (WGSL, world.wgsl)
        //   GPUAgentTierDef     (C++, here)        ↔ AgentTierParams     (WGSL, world.wgsl)
        //
        // The C++ side is uploaded as a uniform buffer (bindings 110/111);
        // the WGSL side reads from those bindings. Field count, field
        // order, and total size MUST match exactly. A field-order
        // mismatch produces silent data corruption (no compile error,
        // agents read parameters from the wrong column). The static_asserts
        // below catch size drift; field order is on the human.
        //
        // The translator (upload_agent_registries_to_gpu in agents.inl)
        // bridges from the CPU-authoring structs (AgentBehaviorDef /
        // AgentTierDef) — those structs may have additional fields like
        // `id` and `name` that don't get uploaded.

        // GPU-side mirror of AgentBehaviorDef (modules/agents.inl) without
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
        // enums in modules/agents.inl. Kept here so state.hpp can size
        // its registry buffers and bind-group entries without depending
        // on agents.inl (which is included after state.hpp into the
        // cartridge class scope). Asserts in agents.inl verify
        // these stay in sync.
        static constexpr uint32_t GPU_AGENT_BEHAVIOR_COUNT = 10;
        static constexpr uint32_t GPU_AGENT_TIER_COUNT = 4;

        // GPU-side mirror of AgentTierDef (modules/agents.inl) without
        // the `id` and `name` fields. Uploaded once at world-init from
        // the C++ AGENT_TIER_GAINS table and read by the agent compute
        // kernels via storage binding 111.
        //
        // Field layout matches WGSL `struct AgentTierParams` exactly so
        // the WGSL side can read this buffer with the same struct shape
        // it had when AGENT_TIER_GAINS_WGSL was a const array literal.
        struct alignas(16) GPUAgentTierDef {
            float step_gain;       //  0 — multiplies behavior.step_size impulse
            float persist_gain;    //  4 — multiplies behavior.persistence (and home_pull)
            float speed_gain;      //  8 — multiplies behavior.speed_cap
            float color_r;         // 12 — vertex shader entity color (line 3798 in world.wgsl)
            float color_g;         // 16
            float color_b;         // 20
            float _pad[2];         // 24-31 — pad to 32 bytes (16-byte alignment)
        };                         // 32 total (16-byte aligned)

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
            // The cube motion model decomposes into:
            //   home  = analytical rest position (anchor.xz + ground + bob)
            //   pos   = home + drift
            // drift_vel integrates spring-to-zero plus behavior forces.
            // Spheres leave drift / drift_vel zero and ignore stiffness/drag.
            float spring_stiffness;    // 136: pulls drift toward zero (1/s²)
            float drag;                // 140: exponential damping on drift_vel (1/s)
            float drift[3];            // 144: position offset from home (cube)
            uint32_t tier_idx;         // 156: runtime tier lookup for gain tables
            float drift_vel[3];        // 160: drift integrator velocity
            uint32_t behavior_id;      // 172: cube behavior registry index (Phase 3)
            // ─── Kite mode (Phase 3.3) ───────────────────────────────
            // When follow_pawn is non-zero, the cube kernel replaces
            //   home_xz = anchor.xz + pawn_offset.xz + pawn.xz
            //   home.y  =  ground(...) + orbit_height + bob_y + pawn_offset.y
            // becomes
            //   home_xz = pawn.xz + pawn_offset.xz
            //   home.y  = pawn.y  + pawn_offset.y + orbit_height + bob_y
            // i.e. anchor is dynamically tied to the pawn's pose. The
            // cube follows the pawn around the world, like a kite on
            // an invisible string, while CurlField/PhaseWave still
            // operate on top via the drift integrator.
            //
            // pawn_offset is captured at toggle-on time so each cube
            // keeps its world-space position at the moment of toggle —
            // no visual snap. Toggling off freezes anchor at the cube's
            // current world position (also no snap).
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
            uint32_t _pad0;            // 196
            uint32_t _pad1;            // 200
            uint32_t _pad2;            // 204
        };                             // 208 total (13×16)

        // Trail-frame ribbon: the body is the trail of a harmonic-oscillator
        // head sampled at older head positions along its length. Visible cycles
        // emerge from *_freq × travel_time; cycles are no longer stored.
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
        };                                                                      // 112 total (size enforced by static_assert below; mirrors world.wgsl RibbonState)

        // Pre-computed per-ring transform (compute pass output, VS + update_world input)
        struct alignas(16) GPURibbonRingTransform {
            float motor_p0[4];         // PGA motor rotor part        (16)
            float motor_p1[4];         // PGA motor translator part   (16)
            float center[3];           // ring world-space center     (12)
            float terrain_y;           // tile-modified terrain height ( 4) = 48
        };
        static_assert(sizeof(GPURibbonRingTransform) == 48, "GPURibbonRingTransform must be 48 bytes");

        // Unified pier instance — terrain-raising volume with tier metadata.
        // Replaces the old GPUSolidInstance/GPUSolidArray. Deterministic slot
        // addressing: test rig at 0-2, arches at 4-35, columns at 36-67.
        // Must match WGSL PierInstance exactly.
        struct alignas(16) GPUPierInstance {
            float origin[2];           // world XZ center of footprint
            float half_size[2];        // half-extent in rotated local X and Z
            float height_near;         // height delta at local −X edge
            float height_far;          // height delta at local +X edge
            float rotation;            // Y-axis rotation (radians, 0 = aligned with world +X)
            float edge_blend;          // smoothstep transition width at boundaries (world units)
            uint32_t tier;             // pier tier (metadata for future use)
            uint32_t is_active;        // 0 = inactive, contributes nothing to heightfield
            uint32_t _pad0;
            uint32_t _pad1;
        };
        static_assert(sizeof(GPUPierInstance) == 48, "GPUPierInstance must be 48 bytes");

        // Pier tier enum (carried in tier field, not read by evaluation today)
        namespace PierTier {
            constexpr uint32_t TEST_RIG = 0;
            constexpr uint32_t ARCH_DOORWAY = 1;
            constexpr uint32_t ARCH_STANDARD = 2;
            constexpr uint32_t ARCH_MONUMENTAL = 3;
            constexpr uint32_t COL_PILLAR = 4;
            constexpr uint32_t COL_DORIC = 5;
            constexpr uint32_t COL_ORNATE = 6;
            constexpr uint32_t COL_ANTENNA = 7;
            constexpr uint32_t COL_ANTENNA_SQUAT = 8;
            constexpr uint32_t COL_ANTENNA_COLOSSAL = 9;
        }

        // Per-arch ground truth: CPU writes both pier foot XZ positions,
        // GPU compute samples terrain at both and takes the min for ground_y.
        // VS reads ground_y to position arch mesh at true terrain height.
        struct alignas(16) GPUArchGroundEntry {
            float pier_left_x;
            float pier_left_z;
            float pier_right_x;
            float pier_right_z;
            float ground_y;         // written by GPU compute: min of corrected terrain at both piers
            uint32_t is_active;
            float pier_correction_left;   // CPU: max_pier_at_left - own_pier_at_left
            float pier_correction_right;  // CPU: max_pier_at_right - own_pier_at_right
        };
        static_assert(sizeof(GPUArchGroundEntry) == 32, "GPUArchGroundEntry must be 32 bytes");

        // Per-column ground truth: CPU writes center XZ + active flag + pier correction,
        // GPU compute corrects ground_y by sampling heightfield minus pier correction.
        struct alignas(16) GPUColumnGroundEntry {
            float center_x;
            float center_z;
            float ground_y;         // written by GPU compute from heightfield - correction
            uint32_t is_active;
            float pier_correction;  // CPU: max_pier_at_center - own_pier_at_center
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

        // Per-pyramid ground truth: CPU writes center XZ + own height + active flag,
        // GPU compute corrects ground_y by sampling heightfield and subtracting
        // the pyramid's own contribution (heightfield includes pyramid).
        struct alignas(16) GPUPyramidGroundEntry {
            float center_x;
            float center_z;
            float ground_y;         // GPU writes: min terrain Y under base
            float own_height;       // CPU writes: pyramid apex height (to subtract)
            uint32_t is_active;
            float half_x;           // CPU writes: base half extent X (for edge sampling)
            float half_z;           // CPU writes: base half extent Z
            float rotation;         // CPU writes: base rotation (for rotated edge points)
        };
        static_assert(sizeof(GPUPyramidGroundEntry) == 32, "GPUPyramidGroundEntry must be 32 bytes");

        // GPU pyramid mesh generation parameters (CPU → GPU per-slot).
        //
        // MUST match world.wgsl::PyramidMeshParams (around line 8369).
        // If this struct gains/loses a field, the WGSL side and
        // cpu_gpu_pair_manifest.md must be updated together.
        struct alignas(16) GPUPyramidMeshParams {
            float center_x;
            float center_z;
            float rotation;
            float half_x;
            float half_z;
            float height;
            float truncation;
            float color_r;
            float color_g;
            float color_b;
            uint32_t is_active;
            uint32_t _pad;
        };
        static_assert(sizeof(GPUPyramidMeshParams) == 48,
            "GPUPyramidMeshParams must be 48 bytes — keep in sync with world.wgsl::PyramidMeshParams");

        // GPU arch mesh generation parameters (CPU → GPU per-slot).
        //
        // MUST match world.wgsl::ArchMeshParams (around line 8569).
        // If this struct gains/loses a field, the WGSL side and
        // cpu_gpu_pair_manifest.md must be updated together.
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

        // GPU column mesh generation parameters (CPU → GPU per-slot).
        //
        // MUST match world.wgsl::ColumnMeshParams (around line 8914).
        // If this struct gains/loses a field, the WGSL side and
        // cpu_gpu_pair_manifest.md must be updated together.
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
        // MUST match world.wgsl::PalmMeshParams (around line 9406).
        // If this struct gains/loses a field, the WGSL side and
        // cpu_gpu_pair_manifest.md must be updated together.
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
        // MUST match world.wgsl::CactusMeshParams (around line 9724).
        // If this struct gains/loses a field, the WGSL side and
        // cpu_gpu_pair_manifest.md must be updated together.
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
        // MUST match world.wgsl::BladeClusterMeshParams (around line 10050).
        // If this struct gains/loses a field, the WGSL side and
        // cpu_gpu_pair_manifest.md must be updated together.
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

        // Zone parameter derivation: CPU → GPU request buffer.
        // CPU sends slot + lattice coords + algorithm + height flag.
        // GPU compute derives all tier/Gaussian parameters and writes zone_config directly.
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
            uint32_t motion_rule;        // 48: 0=Brownian, 1=Orbital, 2=Frozen
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
            float    dome_center_x;      //160: dome center world X
            float    dome_center_y;      //164: dome center world Y (typically 0)
            float    dome_center_z;      //168: dome center world Z
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

        // (GPUCellState removed — legacy cell system no longer active)

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

        // Patch system — streaming terrain around the pawn.
        // Patches are rectangular windows that evaluate and cache the height
        // function over their region. As the pawn moves, new patches spawn
        // ahead and old ones behind are recycled.

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

        // Spatial index for O(1) sample_terrain_y_at lookup in compute shaders.
        // CPU populates entries[lz*side + lx] with (layer + 1) for GENERATED or
        // NEEDS_REGEN patches — 0 encodes an empty slot. Anchor (origin_x,
        // origin_z) and cell_extent (PATCH_EXTENT) make the struct self-describing;
        // the shader doesn't need any external patch_count parameter.
        //
        // Sized to MAX_ACTIVE_PATCHES (PATCH_PREGEN_SIDE² = 15² = 225).
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
        static_assert(sizeof(GPUDesignConfig) == 400, "GPUDesignConfig must be 400 bytes");

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
        static_assert(sizeof(GPUTrajectory) == 16, "GPUTrajectory must be 16 bytes");
        static_assert(sizeof(GPUTerrainState) == 32, "GPUTerrainState must be 32 bytes");
        static_assert(sizeof(GPUAgentState) == 80, "GPUAgentState must be 80 bytes");
        static_assert(sizeof(GPUAgentState) % 16 == 0, "GPUAgentState must be 16-byte aligned");
        static_assert(sizeof(GPUAgentBehaviorDef) == 32, "GPUAgentBehaviorDef must be 32 bytes");
        static_assert(sizeof(GPUAgentBehaviorDef) % 16 == 0, "GPUAgentBehaviorDef must be 16-byte aligned");
        static_assert(sizeof(GPUAgentTierDef) == 32, "GPUAgentTierDef must be 32 bytes");
        static_assert(sizeof(GPUAgentTierDef) % 16 == 0, "GPUAgentTierDef must be 16-byte aligned");
        static_assert(sizeof(GPUCameraState) == 48, "GPUCameraState must be 48 bytes");
        static_assert(sizeof(GPUFloatingEntityState) == 208, "GPUFloatingEntityState must be 208 bytes");
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


        // =====================================================================
        // S5-S10  GPU STATE CLASS
        // =====================================================================

        class GPUState {

            // =================================================================
            // S5 MEMBERS — Buffers, textures, bind groups, samplers
            // =================================================================

            wgpu::Device device_;
            GPUDesignConfig config_{};
            bool configDirty_ = true;      // true at boot → first frame always uploads
            bool configDynamic_ = false;   // mood override: true = upload every frame
            wgpu::TextureFormat colorFormat_ = wgpu::TextureFormat::BGRA8Unorm;  // set in initOffscreenResources

            wgpu::Buffer signalBuffer_, configBuffer_, terrainBuffer_;
            // Agent system — unified entity buffer. Slot 0 is the player's
            // body; slots 1..MAX_AGENTS-1 are mood-authored agents.
            wgpu::Buffer agentStateBuffer_;
            wgpu::Buffer agentStateReadbackStaging_;
            wgpu::Buffer floatingEntityReadbackStaging_;
            // Agent registries — uploaded once at world-init from the C++
            // AGENT_BEHAVIORS / AGENT_TIER_GAINS tables. The single source
            // of truth lives in modules/agents.inl; the GPU side reads
            // these buffers via storage bindings 110 / 111.
            wgpu::Buffer agentBehaviorsBuffer_;
            wgpu::Buffer agentTierGainsBuffer_;
            wgpu::Buffer cameraBuffer_, floatingEntityBuffer_, trajectoriesBuffer_;
            wgpu::Buffer ribbonBuffer_;
            wgpu::Buffer ringTransformsBuffer_;
            wgpu::Buffer headPosesBuffer_;  // ribbon body poses — written via upload_ribbon_head_poses (the head mover lives in modules/ribbon.inl); read by ribbon_centerline_at
            // (bindings 21, 40 reserved — formerly proximity_field, cell_states)
            wgpu::Buffer pierBuffer_;   // unified pier instances (Storage | CopyDst)
            wgpu::Buffer vpBuffer_;
            wgpu::Buffer directionalLightBuffer_;
            wgpu::Buffer pointLightsBuffer_;
            wgpu::Buffer spotLightArrayBuffer_;
            wgpu::Buffer spotVPStagingBuffer_;   // 4 × 64 bytes: pre-staged per-light VPs for atlas copy
            wgpu::Buffer portalArrayBuffer_;
            wgpu::Buffer ribbonReadbackStaging_; // ring transform readback (diagnostic)

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

            // Patch heightfield texture array (49 layers × 256×256, RGBA16Float)
            wgpu::Texture patchHeightfieldArrayTexture_;
            wgpu::TextureView patchHeightfieldArrayWriteView_;  // full array for storage write
            wgpu::TextureView patchHeightfieldArrayReadView_;   // full array for sampling

            // Patch cell color texture array (49 layers × 16×16, RGBA8Unorm)
            // RGB = cell color, A = mode (0=smooth, 1=discrete)
            wgpu::Texture patchCellColorArrayTexture_;
            wgpu::TextureView patchCellColorArrayWriteView_;
            wgpu::TextureView patchCellColorArrayReadView_;

            // Cell spatial field LUT (49 layers × 16×16, RGBA16Float)
            // Baked in generate_patch_cells. R=mode(post-coupling), G=style, B=sparse, A=reserved
            wgpu::Texture cellFieldsArrayTexture_;
            wgpu::TextureView cellFieldsArrayWriteView_;
            wgpu::TextureView cellFieldsArrayReadView_;

            wgpu::Buffer terrainIndexBuffer_;
            // (legacy cell mesh buffers removed — bindings 43-45 reserved)
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

            wgpu::Buffer pyramidVertexBuffer_, pyramidIndexBuffer_;
            wgpu::Buffer pyramidGroundBuffer_;  // per-pyramid ground Y correction
            wgpu::Buffer pyramidInstancesBuffer_;  // GPU-side pyramid array for heightfield baking
            wgpu::Buffer pyramidMeshParamsBuffer_;  // GPU mesh gen: per-slot parameters
            uint32_t pyramidIndexCount_ = 0;

            // Indoor shell (ceiling + walls)
            wgpu::Buffer shellVertexBuffer_, shellIndexBuffer_;
            uint32_t shellIndexCount_ = 0;

            wgpu::BindGroupLayout pyramidMeshGenLayout_;  // bindings 190-192
            wgpu::BindGroupLayout archMeshGenLayout_;    // bindings 193-195
            wgpu::BindGroupLayout columnMeshGenLayout_;  // bindings 196-198
            wgpu::BindGroupLayout palmMeshGenLayout_;    // bindings 180-182
            wgpu::BindGroupLayout cactusMeshGenLayout_;  // bindings 183-185
            wgpu::BindGroupLayout bladeMeshGenLayout_;   // bindings 186-188
            wgpu::BindGroup pyramidMeshGenBindGroup_;
            wgpu::BindGroup archMeshGenBindGroup_;
            wgpu::BindGroup columnMeshGenBindGroup_;
            wgpu::BindGroup palmMeshGenBindGroup_;
            wgpu::BindGroup cactusMeshGenBindGroup_;
            wgpu::BindGroup bladeMeshGenBindGroup_;

            // GoL zone system buffers
            wgpu::Buffer zoneConfigBuffer_;        // GPUGoLZoneArray storage (read_write)
            wgpu::Buffer zoneDeriveRequestBuffer_; // GPUZoneDeriveRequestArray uniform
            wgpu::Buffer zoneLifeBuffer_;          // life state: MAX_ZONES × 4096 floats
            wgpu::Texture zoneLifeTexture_;        // 32×32 × MAX_ZONES r32float texture array
            wgpu::TextureView zoneLifeWriteView_;  // storage texture write (compute)
            wgpu::TextureView zoneLifeReadView_;   // sampled texture read (fragment)

            // Zone cell mesh extrusion buffers
            wgpu::Buffer zoneMeshVertexBuffer_;    // CellMeshVertex[] for zone extrusions
            wgpu::Buffer zoneMeshIndexBuffer_;     // u32[] triangle indices
            wgpu::Buffer zoneMeshIndirectBuffer_;  // DrawIndexedIndirect + atomic counters

            // Pawn aura system
            wgpu::Buffer pawnAuraConfigBuffer_;    // GPUPawnAuraConfig uniform
            wgpu::Buffer pawnAuraCellsBuffer_;     // GPUPawnAuraCell[] storage (N×N toroidal grid)
            wgpu::Texture pawnAuraTexture_;         // N×N RGBA16Float (compute writes, FS reads)
            wgpu::TextureView pawnAuraWriteView_;   // storage texture write (compute)
            wgpu::TextureView pawnAuraReadView_;    // sampled texture read (fragment)
            wgpu::BindGroupLayout pawnAuraComputeLayout_;
            wgpu::BindGroup pawnAuraComputeGroup_;

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

            // (legacy 1×1 stub textures removed — render bindings 20-21, 24 reserved)

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
            // (meshGenBindGroupLayout_ removed — legacy cell mesh gen)
            wgpu::BindGroupLayout terrainIndexGenLayout_;
            wgpu::BindGroupLayout patchGenLayout_;
            wgpu::BindGroupLayout ribbonComputeLayout_;

            wgpu::BindGroup computeEntityBindGroup_, renderEntityBindGroup_;
            wgpu::BindGroup renderTextureBindGroup_, shadowTextureBindGroup_;
            wgpu::BindGroup computeTextureBindGroup_;  // live-contributor textures for flyer/walker compute
            wgpu::BindGroup meshGenEntityBindGroup_;  // still used by fade overlay
            // (meshGenBindGroup_ removed — legacy cell mesh gen)
            wgpu::BindGroup terrainIndexGenBindGroup_;
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
            wgpu::Buffer visiblePatchIndicesBuffer_;      // MAX_ACTIVE_PATCHES × u32 — LOD0 visible list
            wgpu::BindGroupLayout frustumCullLayout_;
            wgpu::BindGroup frustumCullBindGroup_;

            // GoL zone compute (dedicated layout: bindings 160-162, 167-169)
            wgpu::BindGroupLayout zoneGolComputeLayout_;
            wgpu::BindGroup zoneGolComputeBindGroup_;

            wgpu::BindGroup photographerRenderEntityBindGroup_;
            wgpu::Sampler paintingSampler_;


        public:

            // =================================================================
            // S6 IDENTITY — Non-copyable, default-constructible
            // =================================================================

            GPUState() = default;
            GPUState(const GPUState&) = delete;
            GPUState& operator=(const GPUState&) = delete;


            // =================================================================
            // S7 BOOT — Device initialization sequence
            // =================================================================

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


            // =================================================================
            // S8 PER-FRAME — Upload methods called each tick
            // =================================================================

            void upload_signal(wgpu::Queue& queue, const GPUFrameSignal& signal) {
                queue.WriteBuffer(signalBuffer_, 0, &signal, sizeof(GPUFrameSignal));
            }

            // Re-write only the sky_* block of the frame signal. Used to re-sync the
            // pawn mount after advance_ribbon_head so the pawn is sampled at the same
            // frame as the ribbon it rides (removes the one-frame mount lag). The eight
            // sky_* words are contiguous in GPUFrameSignal — a targeted sub-range write,
            // the same idiom as upload_ribbon_time. The local block mirrors that field
            // order exactly; the static_assert guards the 8-word size.
            void resync_sky_head(wgpu::Queue& queue, uint32_t sky_mode,
                                 float head_x, float head_y, float head_z, float heading) {
                struct SkyBlock {
                    uint32_t sky_mode;
                    float head_x, head_y, head_z, heading;
                    float pad2, pad3, pad4;
                } block{ sky_mode, head_x, head_y, head_z, heading, 0.0f, 0.0f, 0.0f };
                static_assert(sizeof(SkyBlock) == 32,
                    "SkyBlock must mirror GPUFrameSignal's eight contiguous sky_* words");
                queue.WriteBuffer(signalBuffer_, offsetof(GPUFrameSignal, sky_mode),
                                  &block, sizeof(block));
            }

            // Upload the agent behavior + tier registries to the GPU.
            // Called once at world-init from the cartridge — values are
            // constexpr-equivalent (sourced from AGENT_BEHAVIORS /
            // AGENT_TIER_GAINS in modules/agents.inl) and never change
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
                queue.WriteBuffer(agentBehaviorsBuffer_, 0, behaviors,
                    behavior_count * sizeof(GPUAgentBehaviorDef));
                queue.WriteBuffer(agentTierGainsBuffer_, 0, tiers,
                    tier_count * sizeof(GPUAgentTierDef));
            }

            void upload_config(wgpu::Queue& queue) {
                if (!configDirty_ && !configDynamic_) return;
                configDirty_ = false;
                queue.WriteBuffer(configBuffer_, 0, &config_, sizeof(GPUDesignConfig));
            }

            // Targeted 4-byte upload of pier_count only — called from write_pier/clear_pier.
            // Bypasses the config dirty flag since pier changes happen mid-frame during spawn.
            void upload_pier_count(wgpu::Queue& queue) {
                static_assert(offsetof(GPUDesignConfig, pier_count) == 124,
                    "pier_count offset must be 124 for targeted upload");
                queue.WriteBuffer(configBuffer_, 124, &config_.pier_count, sizeof(uint32_t));
            }

            // Targeted 4-byte upload of placement_patch_count — called from stream_patches
            // after world_state_.all_patch_count is finalized, so the placement compute pass reads the
            // current frame's patch set (decoupled from the photographer config).
            void upload_placement_patch_count(wgpu::Queue& queue) {
                static_assert(offsetof(GPUDesignConfig, placement_patch_count) == 144,
                    "placement_patch_count offset must be 144 for targeted upload");
                queue.WriteBuffer(configBuffer_, 144, &config_.placement_patch_count, sizeof(uint32_t));
            }

            // Targeted 8-byte upload of lod_pawn_x/z — called from stream_patches each
            // frame so the GPU frustum-cull shader uses the same pawn position as the
            // CPU's LOD banding (eliminates LOD0/LOD1 boundary flicker).
            void upload_lod_pawn(wgpu::Queue& queue) {
                static_assert(offsetof(GPUDesignConfig, lod_pawn_x) == 384,
                    "lod_pawn_x offset must be 384 for targeted upload");
                queue.WriteBuffer(configBuffer_,
                    offsetof(GPUDesignConfig, lod_pawn_x),
                    &config_.lod_pawn_x, sizeof(float) * 2);
            }

            void upload_directional_light(wgpu::Queue& queue, const GPUDirectionalLight& light) {
                queue.WriteBuffer(directionalLightBuffer_, 0, &light, sizeof(GPUDirectionalLight));
            }

            void upload_point_lights(wgpu::Queue& queue, const GPUPointLightArray& lights) {
                queue.WriteBuffer(pointLightsBuffer_, 0, &lights, sizeof(GPUPointLightArray));
            }

            void upload_spot_lights(wgpu::Queue& queue, const GPUSpotLightArray& arr) {
                queue.WriteBuffer(spotLightArrayBuffer_, 0, &arr, sizeof(GPUSpotLightArray));
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
                queue.WriteBuffer(patchParamsBuffer_, 0, &params, sizeof(GPUPatchParams));
            }

            void upload_tile_grid(wgpu::Queue& queue, const GPUTileGrid& grid) {
                queue.WriteBuffer(tileGridBuffer_, 0, &grid, sizeof(GPUTileGrid));
            }

            void upload_patch_instances(wgpu::Queue& queue, const GPUPatchInstance* instances, uint32_t count) {
                queue.WriteBuffer(patchInstancesBuffer_, 0, instances, sizeof(GPUPatchInstance) * count);
            }

            void upload_patch_grid(wgpu::Queue& queue, const GPUPatchGrid& grid) {
                queue.WriteBuffer(patchGridBuffer_, 0, &grid, sizeof(GPUPatchGrid));
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
                queue.WriteBuffer(ribbonBuffer_, 0, &ribbon, sizeof(GPURibbonState));
            }

            void upload_ribbon_wave_amps(wgpu::Queue& queue, float lateral_amp, float vertical_amp) {
                queue.WriteBuffer(ribbonBuffer_, offsetof(GPURibbonState, lateral_amp), &lateral_amp, sizeof(float));
                queue.WriteBuffer(ribbonBuffer_, offsetof(GPURibbonState, vertical_amp), &vertical_amp, sizeof(float));
            }

            // Ribbon body upload — a dumb wire. The head mover lives in
            // modules/ribbon.inl (ribbon_rebuild_body_upload computes the
            // poses; this uploads them). CPU authors intent; the GPU
            // realizes geometry — this is the one write between them.
            void upload_ribbon_head_poses(wgpu::Queue& queue, const float* data, size_t bytes) {
                queue.WriteBuffer(headPosesBuffer_, 0, data, bytes);
            }

            void upload_floating_entity_slot(wgpu::Queue& queue, uint32_t slot, const GPUFloatingEntityState& entity) {
                queue.WriteBuffer(floatingEntityBuffer_,
                    slot * sizeof(GPUFloatingEntityState),
                    &entity, sizeof(GPUFloatingEntityState));
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
            // every active cube without re-uploading the whole 192-byte
            // struct. Field offset is calculated at compile time.
            void upload_cube_behavior_id(wgpu::Queue& queue, uint32_t slot, uint32_t behavior_id) {
                size_t base = (Dim::CUBE_SLOT_OFFSET + slot) * sizeof(GPUFloatingEntityState);
                size_t off = offsetof(GPUFloatingEntityState, behavior_id);
                queue.WriteBuffer(floatingEntityBuffer_, base + off, &behavior_id, sizeof(uint32_t));
            }

            // Partial write: anchor[3] inside a cube slot. Used by the
            // corral diagnostic to relocate every active cube to a small
            // ring around the pawn. The kernel re-derives home from the
            // new anchor on the next frame, drift integrator pulls toward
            // it, and the cube reappears near the pawn within ~half a
            // second of spring settle.
            void upload_cube_anchor(wgpu::Queue& queue, uint32_t slot, float ax, float ay, float az) {
                size_t base = (Dim::CUBE_SLOT_OFFSET + slot) * sizeof(GPUFloatingEntityState);
                size_t off = offsetof(GPUFloatingEntityState, anchor);
                float a[3] = { ax, ay, az };
                queue.WriteBuffer(floatingEntityBuffer_, base + off, a, sizeof(a));
            }

            // Partial writes for kite-mode state (Phase 3.3). Updated
            // independently because a typical kite-mode toggle changes
            // both the flag and the offset, while corral-while-kited
            // changes only the offset.
            void upload_cube_follow_pawn(wgpu::Queue& queue, uint32_t slot, uint32_t follow) {
                size_t base = (Dim::CUBE_SLOT_OFFSET + slot) * sizeof(GPUFloatingEntityState);
                size_t off = offsetof(GPUFloatingEntityState, follow_pawn);
                queue.WriteBuffer(floatingEntityBuffer_, base + off, &follow, sizeof(uint32_t));
            }
            void upload_cube_pawn_offset(wgpu::Queue& queue, uint32_t slot, float ox, float oy, float oz) {
                size_t base = (Dim::CUBE_SLOT_OFFSET + slot) * sizeof(GPUFloatingEntityState);
                size_t off = offsetof(GPUFloatingEntityState, pawn_offset);
                float o[3] = { ox, oy, oz };
                queue.WriteBuffer(floatingEntityBuffer_, base + off, o, sizeof(o));
            }

            void upload_pier_slot(wgpu::Queue& queue, uint32_t slot, const GPUPierInstance& pier) {
                queue.WriteBuffer(pierBuffer_,
                    slot * sizeof(GPUPierInstance),
                    &pier, sizeof(GPUPierInstance));
            }

            // GPU mesh gen: write params for a single arch slot (64 bytes per spawn/evict)
            void upload_arch_mesh_params_slot(wgpu::Queue& queue, uint32_t slot, const GPUArchMeshParams& params) {
                queue.WriteBuffer(archMeshParamsBuffer_,
                    slot * sizeof(GPUArchMeshParams),
                    &params, sizeof(GPUArchMeshParams));
            }

            // Arch GPU mesh gen bind group (dedicated layout — bindings 193-195)
            wgpu::BindGroupLayout arch_mesh_gen_layout() const { return archMeshGenLayout_; }
            wgpu::BindGroup arch_mesh_gen_group() const { return archMeshGenBindGroup_; }

            void upload_painting_slots(wgpu::Queue& queue, const GPUPaintingSlot* slots, uint32_t count) {
                queue.WriteBuffer(paintingSlotsBuffer_, 0, slots, sizeof(GPUPaintingSlot) * count);
            }

            void upload_painting_slot(wgpu::Queue& queue, uint32_t index, const GPUPaintingSlot& slot) {
                queue.WriteBuffer(paintingSlotsBuffer_,
                    index * sizeof(GPUPaintingSlot), &slot, sizeof(GPUPaintingSlot));
            }

            void deactivate_painting_slot(wgpu::Queue& queue, uint32_t index) {
                uint32_t zero = 0;
                queue.WriteBuffer(paintingSlotsBuffer_,
                    index * sizeof(GPUPaintingSlot) + offsetof(GPUPaintingSlot, is_active),
                    &zero, sizeof(uint32_t));
            }

            void upload_photographer_vp(wgpu::Queue& queue, const GPUVPMatrix& vp) {
                queue.WriteBuffer(photographerVPBuffer_, 0, &vp, sizeof(GPUVPMatrix));
            }

            void upload_photographer_camera(wgpu::Queue& queue, float x, float y, float z) {
                GPUCameraState cam{};
                cam.pos[0] = x; cam.pos[1] = y; cam.pos[2] = z;
                queue.WriteBuffer(photographerCameraBuffer_, 0, &cam, sizeof(GPUCameraState));
            }

            void upload_photographer_config(wgpu::Queue& queue, const GPUPhotographerConfig& cfg) {
                queue.WriteBuffer(photographerConfigBuffer_, 0, &cfg, sizeof(GPUPhotographerConfig));
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
                    std::array<wgpu::BindGroupEntry, 3> entries{};
                    entries[0].binding = 50;
                    entries[0].buffer = paintingSlotsBuffer_;
                    entries[0].size = sizeof(GPUPaintingSlot) * Dim::PAINTING_MAX_SLOTS;
                    entries[1].binding = 51;
                    entries[1].textureView = exhibitionReadView_;
                    entries[2].binding = 52;
                    entries[2].sampler = paintingSampler_;

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


            // =================================================================
            // S9 DESIGN MODE — Runtime config toggles
            // =================================================================

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
            void set_mute_signal(bool m) {
                uint32_t v = m ? 1 : 0;
                if (config_.mute_signal != v) { config_.mute_signal = v; configDirty_ = true; }
            }
            void set_mute_dynamics_0d(bool m) {
                uint32_t v = m ? 1 : 0;
                if (config_.mute_dynamics_0d != v) { config_.mute_dynamics_0d = v; configDirty_ = true; }
            }
            // (set_mute_dynamics_2d removed — config field kept for alignment, never read by GPU)
            void set_mute_coupling(uint32_t b, bool m) {
                uint32_t prev = config_.mute_couplings;
                if (m) config_.mute_couplings |= b; else config_.mute_couplings &= ~b;
                if (config_.mute_couplings != prev) configDirty_ = true;
            }
            void set_mute_couplings(uint32_t mask) {
                if (config_.mute_couplings != mask) { config_.mute_couplings = mask; configDirty_ = true; }
            }

            // Tuning
            // (set_wave_time_scale, set_active_cell_size removed — config fields kept for alignment)
            void set_pawn_speed(float s) {
                if (config_.pawn_speed != s) { config_.pawn_speed = s; configDirty_ = true; }
            }
            void set_camera_sensitivity(float s) {
                if (config_.camera_sensitivity != s) { config_.camera_sensitivity = s; configDirty_ = true; }
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
            // Reset the player's agent slot to the idle pose. Pass 1
            // replaces the old reset_pawn: the player's body lives in
            // agent_state[0], so we write slot 0 with PlayerControlled
            // defaults and clear the remaining slots. Callers should
            // set config.possessed_slot = 0 separately (or rely on the
            // set_possessed_slot setter for subsequent transfers).
            //
            // tier_idx is preserved across mood transitions: the tier the
            // player inhabits is part of their identity, not a property
            // of the old mood. Caller passes the desired tier (typically
            // the tier of whatever slot the player was in at teardown).
            // Defaults to Worker for initial session spawn.
            void reset_player_agent(wgpu::Queue& queue, uint32_t tier_idx = 0u) {
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
                queue.WriteBuffer(agentStateBuffer_, 0, buf, sizeof(buf));
            }
            // Upload the full 32-slot agent array. Slot 0 (player) is rewritten
            // to whatever the caller has in agent_state_.slots[0] — caller is responsible
            // for keeping that mirror consistent with the player's idle pose.
            void upload_agent_state_all(wgpu::Queue& queue, const GPUAgentState* src) {
                queue.WriteBuffer(agentStateBuffer_, 0, src,
                    Dim::MAX_AGENTS * sizeof(GPUAgentState));
            }
            // Upload one slot only. Used by per-frame respawns so writes
            // don't race with the GPU's own update of slot 0 (the player).
            void upload_agent_slot(wgpu::Queue& queue,
                uint32_t slot,
                const GPUAgentState* src) {
                if (slot >= Dim::MAX_AGENTS) return;
                queue.WriteBuffer(agentStateBuffer_,
                    slot * sizeof(GPUAgentState),
                    src, sizeof(GPUAgentState));
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

            // Sphere freeze
            void toggle_freeze_sphere() {
                config_.freeze_sphere = config_.freeze_sphere ? 0 : 1;
                configDirty_ = true;
            }
            void set_freeze_sphere(bool f) {
                uint32_t v = f ? 1 : 0;
                if (config_.freeze_sphere != v) { config_.freeze_sphere = v; configDirty_ = true; }
            }

            // (wave toggle/freeze methods removed — legacy fixed-wave system.
            //  Config fields wave_enable_mask, wave_freeze_mask, wave_frozen_t
            //  remain in struct for byte alignment but are inert.)

            // --- Upload frequency control ---
            // Mood compositions call set_config_dynamic(true) for worlds with
            // continuously varying parameters (moving sun, breathing fog, etc.).
            // Static worlds leave this false and only upload on actual change.
            void set_config_dynamic(bool d) { configDynamic_ = d; }
            void mark_config_dirty() { configDirty_ = true; }

            // --- Config field setters (dirty-flagged, for fields previously written directly) ---
            void set_pawn_aura_height(float h) {
                if (config_.pawn_aura_height != h) { config_.pawn_aura_height = h; configDirty_ = true; }
            }
            void set_pawn_amp_scale(float s) {
                if (config_.pawn_amp_scale != s) { config_.pawn_amp_scale = s; configDirty_ = true; }
            }
            void set_pawn_height_bias(float b) {
                if (config_.pawn_height_bias != b) { config_.pawn_height_bias = b; configDirty_ = true; }
            }


            // =================================================================
            // S10 ACCESSORS — Grouped by render-pass concern
            // =================================================================

            // --- Config ---
            GPUDesignConfig& config() { return config_; }
            const GPUDesignConfig& config() const { return config_; }
            uint32_t get_fpv_mode() const { return config_.fpv_mode; }
            // (get_active_cell_size, wave accessors removed — legacy)

            // --- Compute pass ---
            wgpu::BindGroup compute_entity_group() const { return computeEntityBindGroup_; }
            wgpu::BindGroupLayout compute_entity_layout() const { return computeEntityBindGroupLayout_; }
            // Live-contributor textures (Group 1) for compute pipelines that
            // evaluate query_ground_flyer / query_ground_walker.
            wgpu::BindGroup compute_texture_group() const { return computeTextureBindGroup_; }
            wgpu::BindGroupLayout compute_texture_layout() const { return computeTextureBindGroupLayout_; }
            wgpu::BindGroup terrain_index_gen_group() const { return terrainIndexGenBindGroup_; }
            wgpu::BindGroupLayout terrain_index_gen_layout() const { return terrainIndexGenLayout_; }
            wgpu::BindGroup patch_gen_group() const { return patchGenBindGroup_; }
            wgpu::BindGroupLayout patch_gen_layout() const { return patchGenLayout_; }
            wgpu::BindGroup ribbon_compute_group() const { return ribbonComputeBindGroup_; }
            wgpu::BindGroupLayout ribbon_compute_layout() const { return ribbonComputeLayout_; }
            wgpu::BindGroup mesh_gen_entity_group() const { return meshGenEntityBindGroup_; }
            wgpu::BindGroupLayout mesh_gen_entity_layout() const { return meshGenEntityBindGroupLayout_; }
            // (mesh_gen_group/layout removed — legacy cell mesh gen)

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
            wgpu::Buffer terrain_index_buffer() const { return terrainIndexBuffer_; }
            static constexpr uint32_t terrain_index_count() { return Dim::TERRAIN_INDEX_COUNT; }
            wgpu::Buffer patch_index_buffer() const { return patchIndexBuffer_; }
            uint32_t patch_index_count() const { return patchIndexCount_; }
            wgpu::Buffer patch_index_buffer_lod1() const { return patchIndexBufferLOD1_; }
            uint32_t patch_index_count_lod1() const { return patchIndexCountLOD1_; }

            // --- GPU frustum culling ---
            wgpu::Buffer frustum_indirect_lod0() const { return frustumIndirectLOD0_; }
            wgpu::Buffer frustum_compute_buffer() const { return frustumComputeBuffer_; }
            wgpu::Buffer visible_patch_indices_buffer() const { return visiblePatchIndicesBuffer_; }
            wgpu::BindGroupLayout frustum_cull_layout() const { return frustumCullLayout_; }
            wgpu::BindGroup frustum_cull_group() const { return frustumCullBindGroup_; }

            // Reset LOD0 indirect args in the compute buffer.
            // Writes constant fields (indexCount, firstIndex=0, baseVertex=0, firstInstance=0)
            // and zeros instanceCount. Compute shader then atomicAdds instanceCount.
            // After compute, CopyBufferToBuffer transfers to frustumIndirectLOD0_ for the draw.
            void reset_frustum_indirect(wgpu::Queue& queue) {
                uint32_t args[5] = { patchIndexCount_, 0, 0, 0, 0 };
                queue.WriteBuffer(frustumComputeBuffer_, 0, args, sizeof(args));
            }

            // (legacy cell mesh accessors removed — bindings 43-45 reserved)
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
                queue.WriteBuffer(archGroundBuffer_, 0, entries,
                    sizeof(GPUArchGroundEntry) * std::min(count, Dim::MAX_ARCH_INSTANCES));
            }
            wgpu::Buffer column_vertex_buffer() const { return columnVertexBuffer_; }
            wgpu::Buffer column_index_buffer() const { return columnIndexBuffer_; }
            wgpu::Buffer column_ground_buffer() const { return columnGroundBuffer_; }
            uint32_t column_index_count() const { return columnIndexCount_; }
            void set_column_index_count(uint32_t count) { columnIndexCount_ = count; }

            // GPU mesh gen: write params for a single column slot (80 bytes per spawn/evict)
            void upload_column_mesh_params_slot(wgpu::Queue& queue, uint32_t slot, const GPUColumnMeshParams& params) {
                queue.WriteBuffer(columnMeshParamsBuffer_,
                    slot * sizeof(GPUColumnMeshParams),
                    &params, sizeof(GPUColumnMeshParams));
            }

            // Column GPU mesh gen bind group
            wgpu::BindGroupLayout column_mesh_gen_layout() const { return columnMeshGenLayout_; }
            wgpu::BindGroup column_mesh_gen_group() const { return columnMeshGenBindGroup_; }

            void upload_column_origins(wgpu::Queue& queue, const GPUColumnGroundEntry* entries, uint32_t count) {
                queue.WriteBuffer(columnGroundBuffer_, 0, entries,
                    sizeof(GPUColumnGroundEntry) * std::min(count, Dim::MAX_COLUMN_INSTANCES));
            }

            // --- Palm accessors and upload ---
            wgpu::Buffer palm_vertex_buffer() const { return palmVertexBuffer_; }
            wgpu::Buffer palm_index_buffer() const { return palmIndexBuffer_; }
            uint32_t palm_index_count() const { return palmIndexCount_; }
            void set_palm_index_count(uint32_t count) { palmIndexCount_ = count; }

            void upload_palm_mesh_params_slot(wgpu::Queue& queue, uint32_t slot, const GPUPalmMeshParams& params) {
                queue.WriteBuffer(palmMeshParamsBuffer_,
                    slot * sizeof(GPUPalmMeshParams),
                    &params, sizeof(GPUPalmMeshParams));
            }

            wgpu::BindGroupLayout palm_mesh_gen_layout() const { return palmMeshGenLayout_; }
            wgpu::BindGroup palm_mesh_gen_group() const { return palmMeshGenBindGroup_; }

            // --- Cactus accessors and upload ---
            wgpu::Buffer cactus_vertex_buffer() const { return cactusVertexBuffer_; }
            wgpu::Buffer cactus_index_buffer() const { return cactusIndexBuffer_; }
            uint32_t cactus_index_count() const { return cactusIndexCount_; }
            void set_cactus_index_count(uint32_t count) { cactusIndexCount_ = count; }
            void upload_cactus_mesh_params_slot(wgpu::Queue& queue, uint32_t slot, const GPUCactusMeshParams& params) {
                queue.WriteBuffer(cactusMeshParamsBuffer_,
                    slot * sizeof(GPUCactusMeshParams),
                    &params, sizeof(GPUCactusMeshParams));
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
                queue.WriteBuffer(bladeMeshParamsBuffer_,
                    slot * sizeof(GPUBladeClusterMeshParams),
                    &params, sizeof(GPUBladeClusterMeshParams));
            }
            wgpu::BindGroupLayout blade_mesh_gen_layout() const { return bladeMeshGenLayout_; }
            wgpu::BindGroup blade_mesh_gen_group() const { return bladeMeshGenBindGroup_; }

            // --- Pyramid accessors and upload ---
            wgpu::Buffer pyramid_vertex_buffer() const { return pyramidVertexBuffer_; }
            wgpu::Buffer pyramid_index_buffer() const { return pyramidIndexBuffer_; }
            wgpu::Buffer pyramid_ground_buffer() const { return pyramidGroundBuffer_; }
            uint32_t pyramid_index_count() const { return pyramidIndexCount_; }
            void set_pyramid_index_count(uint32_t count) { pyramidIndexCount_ = count; }

            void upload_pyramids(wgpu::Queue& queue, const GPUPyramidArray& arr) {
                queue.WriteBuffer(pyramidInstancesBuffer_, 0, &arr, sizeof(GPUPyramidArray));
            }

            // GPU mesh gen: write params for a single slot (48 bytes per spawn/evict)
            void upload_pyramid_mesh_params_slot(wgpu::Queue& queue, uint32_t slot, const GPUPyramidMeshParams& params) {
                queue.WriteBuffer(pyramidMeshParamsBuffer_,
                    slot * sizeof(GPUPyramidMeshParams),
                    &params, sizeof(GPUPyramidMeshParams));
            }

            // GPU mesh gen bind group
            wgpu::BindGroupLayout pyramid_mesh_gen_layout() const { return pyramidMeshGenLayout_; }
            wgpu::BindGroup pyramid_mesh_gen_group() const { return pyramidMeshGenBindGroup_; }

            void upload_pyramid_origins(wgpu::Queue& queue, const GPUPyramidGroundEntry* entries, uint32_t count) {
                queue.WriteBuffer(pyramidGroundBuffer_, 0, entries,
                    sizeof(GPUPyramidGroundEntry) * std::min(count, Dim::MAX_PYRAMID_INSTANCES));
            }

            // Indoor shell accessors
            wgpu::Buffer shell_vertex_buffer() const { return shellVertexBuffer_; }
            wgpu::Buffer shell_index_buffer() const { return shellIndexBuffer_; }
            uint32_t shell_index_count() const { return shellIndexCount_; }
            void set_shell_index_count(uint32_t count) { shellIndexCount_ = count; }

            void upload_shell_mesh(wgpu::Queue& queue,
                const ShellVertex* verts, uint32_t vertCount,
                const uint32_t* indices, uint32_t idxCount) {
                queue.WriteBuffer(shellVertexBuffer_, 0, verts,
                    sizeof(ShellVertex) * std::min(vertCount, Dim::SHELL_MAX_VERTICES));
                queue.WriteBuffer(shellIndexBuffer_, 0, indices,
                    sizeof(uint32_t) * std::min(idxCount, Dim::SHELL_MAX_INDICES));
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
            void set_possessed_slot(uint32_t slot) {
                if (config_.possessed_slot != slot) {
                    config_.possessed_slot = slot;
                    configDirty_ = true;
                }
            }
            wgpu::Buffer ribbon_readback_staging() const { return ribbonReadbackStaging_; }
            wgpu::Buffer ring_transforms_buffer() const { return ringTransformsBuffer_; }
            static constexpr size_t ribbon_ring_readback_size() { return sizeof(GPURibbonRingTransform) * Dim::RIBBON_MAX_RINGS; }

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
            // Zone mesh gen uses the SAME merged layout/group (all zone entry points share it)
            wgpu::BindGroup zone_mesh_gen_group() const { return zoneGolComputeBindGroup_; }
            wgpu::BindGroupLayout zone_mesh_gen_layout() const { return zoneGolComputeLayout_; }

            // Pawn aura accessors
            wgpu::BindGroupLayout pawn_aura_compute_layout() const { return pawnAuraComputeLayout_; }
            wgpu::BindGroup pawn_aura_compute_group() const { return pawnAuraComputeGroup_; }
            void upload_pawn_aura_config(wgpu::Queue& queue, const GPUPawnAuraConfig& cfg) {
                queue.WriteBuffer(pawnAuraConfigBuffer_, 0, &cfg, sizeof(GPUPawnAuraConfig));
            }
            void upload_pawn_aura_frame(wgpu::Queue& queue, float dt, float t_beats) {
                queue.WriteBuffer(pawnAuraConfigBuffer_, offsetof(GPUPawnAuraConfig, dt), &dt, sizeof(float));
                queue.WriteBuffer(pawnAuraConfigBuffer_, offsetof(GPUPawnAuraConfig, t_beats), &t_beats, sizeof(float));
            }
            static constexpr uint32_t pawn_aura_workgroups() { return PAWN_AURA_N / 8; }

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
                queue.WriteBuffer(orbConfigBuffer_, 0, &cfg, sizeof(GPUOrbConfig));
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
            // Pass 12: three flocking force signs written as a packed triple
            // at flock_sep_sign. Each force picks up its direction multiplier
            // from the dynamics kernel on the next dispatch.
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
            // Orbital gesture bundle — two non-contiguous floats
            // (412 and 428, interleaved by tier-flock-gain blocks).
            // Two tiny writes — only fires on gesture cycle, not per frame.
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
            // Dome center: world-space anchor point added at render time by orb_vs.
            // 12-byte write; _pad_anchor is left untouched for future use.
            void upload_orb_dome_center(wgpu::Queue& queue,
                float x, float y, float z) {
                struct { float x, y, z; } packed = { x, y, z };
                queue.WriteBuffer(orbConfigBuffer_,
                    offsetof(GPUOrbConfig, dome_center_x),
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
            wgpu::Buffer zone_mesh_vertex_buffer() const { return zoneMeshVertexBuffer_; }
            wgpu::Buffer zone_mesh_index_buffer() const { return zoneMeshIndexBuffer_; }
            wgpu::Buffer zone_mesh_indirect_buffer() const { return zoneMeshIndirectBuffer_; }

            void upload_zone_config(wgpu::Queue& queue, const GPUGoLZoneArray& config) {
                queue.WriteBuffer(zoneConfigBuffer_, 0, &config, sizeof(GPUGoLZoneArray));
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
                queue.WriteBuffer(zoneDeriveRequestBuffer_, 0, &requests, sizeof(GPUZoneDeriveRequestArray));
            }

            void upload_portal_array(wgpu::Queue& queue, const GPUPortalArray& arr) {
                queue.WriteBuffer(portalArrayBuffer_, 0, &arr, sizeof(GPUPortalArray));
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
                // Slot 5: color visual (initial = target)
                queue.WriteBuffer(zoneLifeBuffer_, base + slot_stride * 5, life_data, cells_bytes);
                // Slot 6: color velocity (zero)
                queue.WriteBuffer(zoneLifeBuffer_, base + slot_stride * 6, zeros.data(), cells_bytes);
            }

            // --- Dispatch dimensions ---
            static constexpr uint32_t terrain_mesh_workgroups() { return Dim::TERRAIN_MESH_N / 8; }
            // (terrain_height_field_workgroups, cell_grid_workgroups, surface_color_workgroups removed)
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

            // =================================================================
            // S7 BOOT (continued) — Private creation methods
            // =================================================================


            wgpu::Buffer makeBuffer(const char* label, uint64_t size, wgpu::BufferUsage usage) {
                wgpu::BufferDescriptor d{}; d.label = label; d.size = size; d.usage = usage;
                return device_.CreateBuffer(&d);
            }

            bool createBuffers() {
                auto SU = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
                auto UU = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
                signalBuffer_ = makeBuffer("Frame Signal", sizeof(GPUFrameSignal), wgpu::BufferUsage::Uniform | wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                configBuffer_ = makeBuffer("Design Config", sizeof(GPUDesignConfig), UU);
                terrainBuffer_ = makeBuffer("Terrain State", sizeof(GPUTerrainState), SU);
                agentStateBuffer_ = makeBuffer("Agent State",
                    Dim::MAX_AGENTS * sizeof(GPUAgentState),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc);
                // Agent registries — uniform on the GPU, written once at
                // world-init from C++ tables (see upload_agent_registries).
                // Uniform (not storage) to stay within the 10-per-stage
                // storage buffer cap on the compute kernels.
                agentBehaviorsBuffer_ = makeBuffer("Agent Behaviors Table",
                    GPU_AGENT_BEHAVIOR_COUNT * sizeof(GPUAgentBehaviorDef),
                    wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst);
                agentTierGainsBuffer_ = makeBuffer("Agent Tier Gains Table",
                    GPU_AGENT_TIER_COUNT * sizeof(GPUAgentTierDef),
                    wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst);
                cameraBuffer_ = makeBuffer("Camera State", sizeof(GPUCameraState), SU);
                floatingEntityBuffer_ = makeBuffer("Floating Entity Array",
                    Dim::TOTAL_FLOATING_SLOTS * sizeof(GPUFloatingEntityState),
                    SU | wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopySrc);
                ribbonBuffer_ = makeBuffer("Ribbon State", sizeof(GPURibbonState), SU | wgpu::BufferUsage::Uniform);
                ringTransformsBuffer_ = makeBuffer("Ring Transforms",
                    sizeof(GPURibbonRingTransform) * Dim::RIBBON_MAX_RINGS,
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc);
                headPosesBuffer_ = makeBuffer("Ribbon Head Poses",
                    sizeof(float) * 4 * Dim::RIBBON_MAX_RINGS,
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                trajectoriesBuffer_ = makeBuffer("Trajectories", sizeof(GPUTrajectory) * Dim::MAX_TRAJECTORIES, SU);
                // (proximity_field and terrain_cells stubs removed — bindings 21, 40 reserved)
                vpBuffer_ = makeBuffer("VP Matrix", sizeof(GPUVPMatrix),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                directionalLightBuffer_ = makeBuffer("Directional Light", sizeof(GPUDirectionalLight), SU);
                pointLightsBuffer_ = makeBuffer("Point Lights", sizeof(GPUPointLightArray), SU);
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
                    sd.label = "Ribbon Ring Readback Staging";
                    sd.size = sizeof(GPURibbonRingTransform) * Dim::RIBBON_MAX_RINGS;
                    sd.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
                    ribbonReadbackStaging_ = device_.CreateBuffer(&sd);
                }
                patchParamsBuffer_ = makeBuffer("Patch Params", sizeof(GPUPatchParams), UU);
                patchStagingBuffer_ = makeBuffer("Patch Params Staging",
                    sizeof(GPUPatchParams) * Dim::MAX_ACTIVE_PATCHES,
                    wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc);
                tileGridBuffer_ = makeBuffer("Tile Grid", sizeof(GPUTileGrid), UU);
                pierBuffer_ = makeBuffer("Pier Instances",
                    Dim::PIER_TOTAL * sizeof(GPUPierInstance),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                patchInstancesBuffer_ = makeBuffer("Patch Instances",
                    sizeof(GPUPatchInstance) * Dim::MAX_ACTIVE_PATCHES,
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                patchGridBuffer_ = makeBuffer("Patch Grid",
                    sizeof(GPUPatchGrid),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                patchHeightScratchBuffer_ = makeBuffer("Patch Height Scratch",
                    Dim::PATCH_HEIGHTFIELD_N * Dim::PATCH_HEIGHTFIELD_N * 2 * sizeof(float),
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

                return signalBuffer_ && configBuffer_ && terrainBuffer_ &&
                    agentStateBuffer_ && agentStateReadbackStaging_ &&
                    cameraBuffer_ && floatingEntityBuffer_ && trajectoriesBuffer_ && ringTransformsBuffer_ && headPosesBuffer_ &&
                    vpBuffer_ && spotLightArrayBuffer_ && spotVPStagingBuffer_ && directionalLightBuffer_ && pointLightsBuffer_ && patchParamsBuffer_ &&
                    patchStagingBuffer_ && tileGridBuffer_ && pierBuffer_ && patchInstancesBuffer_ &&
                    patchGridBuffer_ &&
                    patchHeightScratchBuffer_ &&
                    photographerVPBuffer_ && photographerCameraBuffer_ &&
                    photographerConfigBuffer_ && paintingSlotsBuffer_ &&
                    portalArrayBuffer_ && ribbonReadbackStaging_ &&
                    frustumIndirectLOD0_ && frustumComputeBuffer_ && visiblePatchIndicesBuffer_;
            }


            bool createMeshBuffers() {
                // Terrain index buffer -- filled once by compute shader, read every frame
                terrainIndexBuffer_ = makeBuffer("Terrain IB",
                    Dim::TERRAIN_INDEX_COUNT * 4,
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::Index);
                if (!terrainIndexBuffer_) return false;

                // Patch index buffer -- CPU-generated, shared by all patch instances
                // LOD-0: full 64×64 mesh (24576 indices)
                {
                    std::vector<uint32_t> idx;
                    idx.reserve(Dim::PATCH_INDEX_COUNT);
                    for (uint32_t z = 0; z < Dim::PATCH_MESH_N; z++) {
                        for (uint32_t x = 0; x < Dim::PATCH_MESH_N; x++) {
                            uint32_t stride = Dim::PATCH_MESH_N + 1;
                            uint32_t i00 = z * stride + x;
                            uint32_t i10 = i00 + 1;
                            uint32_t i01 = i00 + stride;
                            uint32_t i11 = i01 + 1;
                            idx.push_back(i00); idx.push_back(i01); idx.push_back(i10);
                            idx.push_back(i10); idx.push_back(i01); idx.push_back(i11);
                        }
                    }
                    patchIndexCount_ = (uint32_t)idx.size();
                    patchIndexBuffer_ = makeBuffer("Patch IB",
                        patchIndexCount_ * 4,
                        wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst);
                    if (!patchIndexBuffer_) return false;
                    auto q = device_.GetQueue();
                    q.WriteBuffer(patchIndexBuffer_, 0, idx.data(), idx.size() * 4);
                }

                // LOD-1: half-res 32×32 mesh stepping by 2 through the same 65×65 vertex grid.
                // The vertex shader doesn't know it's LOD-1 — it gets different indices
                // that happen to skip every other vertex, producing correct but coarser UVs.
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

            // (createCellMeshBuffers removed — legacy cell mesh gen)

            bool createSphereMesh() {
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
                // VB: Vertex + CopyDst (transition fallback) + Storage (GPU compute writes)
                pyramidVertexBuffer_ = makeBuffer("Pyramid VB (GPU mesh gen)",
                    Dim::PMG_TOTAL_VERTICES * sizeof(ArchVertex),
                    wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage);
                // IB: Index + CopyDst (transition fallback) + Storage (GPU compute writes)
                pyramidIndexBuffer_ = makeBuffer("Pyramid IB (GPU mesh gen)",
                    Dim::PMG_TOTAL_INDICES * sizeof(uint32_t),
                    wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage);
                pyramidGroundBuffer_ = makeBuffer("Pyramid Ground Y",
                    Dim::MAX_PYRAMID_INSTANCES * sizeof(GPUPyramidGroundEntry),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                pyramidInstancesBuffer_ = makeBuffer("Pyramid Instances (GPU uniform)",
                    sizeof(GPUPyramidArray),
                    wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst);
                // Mesh gen params buffer (8 × 48 bytes)
                pyramidMeshParamsBuffer_ = makeBuffer("Pyramid Mesh Params",
                    Dim::MAX_PYRAMID_INSTANCES * sizeof(GPUPyramidMeshParams),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);

                if (!pyramidVertexBuffer_ || !pyramidIndexBuffer_ ||
                    !pyramidGroundBuffer_ || !pyramidInstancesBuffer_ ||
                    !pyramidMeshParamsBuffer_) return false;

                // Index count starts at 0 — updated by cartridge as pyramids spawn/evict
                pyramidIndexCount_ = 0;

                // Zero-init the instances buffer
                GPUPyramidArray empty{};
                device_.GetQueue().WriteBuffer(pyramidInstancesBuffer_, 0, &empty, sizeof(GPUPyramidArray));

                // Zero-init params buffer (all slots inactive → degenerates on first dispatch)
                {
                    GPUPyramidMeshParams emptyParams[Dim::MAX_PYRAMID_INSTANCES]{};
                    device_.GetQueue().WriteBuffer(pyramidMeshParamsBuffer_, 0, emptyParams,
                        sizeof(GPUPyramidMeshParams) * Dim::MAX_PYRAMID_INSTANCES);
                }

                // Zero-init VB so degenerate indices have safe arch_index=0
                {
                    std::vector<uint8_t> zeros(Dim::PMG_TOTAL_VERTICES * sizeof(ArchVertex), 0);
                    device_.GetQueue().WriteBuffer(pyramidVertexBuffer_, 0, zeros.data(), zeros.size());
                }

                std::cout << "[GPUState] Pyramid buffers (GPU mesh gen): "
                    << Dim::PMG_TOTAL_VERTICES << " vert, "
                    << Dim::PMG_TOTAL_INDICES << " index capacity\n";
                return true;
            }

            bool createShellMesh() {
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

                // Zone life texture: 32×32 × MAX_ZONES, RG32Float (R=height, G=color)
                {
                    wgpu::TextureDescriptor desc{};
                    desc.label = "GoL Zone Life Texture Array";
                    desc.size = { Dim::GOL_ZONE_GRID, Dim::GOL_ZONE_GRID, Dim::MAX_GOL_ZONES };
                    desc.format = wgpu::TextureFormat::RG32Float;
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

                // Zone cell mesh extrusion buffers
                zoneMeshVertexBuffer_ = makeBuffer("Zone Mesh Vertices",
                    Dim::ZONE_MESH_MAX_VERTICES * 44,  // 44 bytes per CellMeshVertex (pos3+normal3+uv2+color3)
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::Vertex);

                zoneMeshIndexBuffer_ = makeBuffer("Zone Mesh Indices",
                    Dim::ZONE_MESH_MAX_INDICES * sizeof(uint32_t),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::Index);

                zoneMeshIndirectBuffer_ = makeBuffer("Zone Mesh Indirect",
                    6 * sizeof(uint32_t),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::Indirect);

                if (!zoneMeshVertexBuffer_ || !zoneMeshIndexBuffer_ || !zoneMeshIndirectBuffer_) return false;

                std::cout << "[GPUState] Zone mesh buffers: "
                    << Dim::ZONE_MESH_MAX_VERTICES << " vert, "
                    << Dim::ZONE_MESH_MAX_INDICES << " index capacity\n";

                // Pawn aura buffers
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
                // (legacy 1×1 stub textures removed — bindings 20-21, 24 and compute 0-2 reserved)

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
                    desc.label = "Patch Heightfield Array (225x256x256, RGBA16Float)";
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
                    desc.label = "Patch Cell Color Array (225x16x16, RGBA8Unorm)";
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

                // Cell spatial field LUT (RGBA16Float, 16×16 × MAX_ACTIVE_PATCHES)
                // Baked during generate_patch_cells: mode, style, sparse, reserved.
                // Terrain FS reads via textureLoad to skip 3 lattice noise chains.
                {
                    wgpu::TextureDescriptor desc{};
                    desc.label = "Cell Fields LUT (RGBA16Float 16x16)";
                    desc.size = { Dim::PATCH_CELL_N, Dim::PATCH_CELL_N, Dim::MAX_ACTIVE_PATCHES };
                    desc.dimension = wgpu::TextureDimension::e2D;
                    desc.format = wgpu::TextureFormat::RGBA16Float;
                    desc.usage = wgpu::TextureUsage::StorageBinding | wgpu::TextureUsage::TextureBinding;
                    cellFieldsArrayTexture_ = device_.CreateTexture(&desc);
                    if (!cellFieldsArrayTexture_) return false;

                    wgpu::TextureViewDescriptor viewDesc{};
                    viewDesc.dimension = wgpu::TextureViewDimension::e2DArray;
                    viewDesc.arrayLayerCount = Dim::MAX_ACTIVE_PATCHES;
                    viewDesc.label = "Cell Fields LUT Write";
                    cellFieldsArrayWriteView_ = cellFieldsArrayTexture_.CreateView(&viewDesc);
                    viewDesc.label = "Cell Fields LUT Read";
                    cellFieldsArrayReadView_ = cellFieldsArrayTexture_.CreateView(&viewDesc);
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

                // Painting and offscreen textures created later in initOffscreenResources()
                // when the swapchain color format is known.

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

                // -- Compute entity layout (Group 0) -- 20-slot system ranges --
                //
                // Shared:    0-19    (signal, config, vp)
                // Terrain:  20-39    (terrain_state, proximity_field)
                // Cells:    40-59    (terrain_cells)
                // Agents:   60-79    (agent_state — unified entity buffer)
                // Camera:   80-99    (camera_state)
                // Sphere:  100-119   (sphere_state, trajectories)
                //
                {
                    std::array<wgpu::BindGroupLayoutEntry, 19> entries{};

                    entries[0].binding = 0;
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = 1;
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[2].binding = 2;
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[3].binding = 20;
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].buffer.type = wgpu::BufferBindingType::Storage;

                    // (bindings 21, 40 removed — formerly proximity_field, cell_states)
                    // (binding 120 removed — ribbon_state only used by compute_ribbon_rings, separate group)

                    entries[4].binding = 60;
                    entries[4].visibility = wgpu::ShaderStage::Compute;
                    entries[4].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[5].binding = 80;
                    entries[5].visibility = wgpu::ShaderStage::Compute;
                    entries[5].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[6].binding = 100;
                    entries[6].visibility = wgpu::ShaderStage::Compute;
                    entries[6].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[7].binding = 101;
                    entries[7].visibility = wgpu::ShaderStage::Compute;
                    entries[7].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[8].binding = 25;
                    entries[8].visibility = wgpu::ShaderStage::Compute;
                    entries[8].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[9].binding = 26;   // pier_instances (storage, read)
                    entries[9].visibility = wgpu::ShaderStage::Compute;
                    entries[9].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[10].binding = 30;   // pyramid_instances (uniform — used by effective_ground_y)
                    entries[10].visibility = wgpu::ShaderStage::Compute;
                    entries[10].buffer.type = wgpu::BufferBindingType::Uniform;

                    // GoL zone state — used by effective_ground_y for cell height contribution
                    entries[11].binding = 160;  // zone_config (storage — matches var<storage, read_write>)
                    entries[11].visibility = wgpu::ShaderStage::Compute;
                    entries[11].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[12].binding = 161;  // zone_life (storage, rw — matches WGSL var declaration)
                    entries[12].visibility = wgpu::ShaderStage::Compute;
                    entries[12].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[13].binding = 62;   // portal_array (uniform — proximity check in behavior_player_controlled)
                    entries[13].visibility = wgpu::ShaderStage::Compute;
                    entries[13].buffer.type = wgpu::BufferBindingType::Uniform;

                    // Cached patch heightfield + sampler + spatial index — sample_terrain_y_at
                    // (POLICY_BAKED_HEIGHTFIELD via texture). Required by compute pipelines that
                    // do per-frame baked-path Y lookups: update_camera, update_agents,
                    // and any future cached-heightfield consumer that lives on this shared layout.
                    entries[14].binding = 145;  // photo_heightfield (texture_2d_array)
                    entries[14].visibility = wgpu::ShaderStage::Compute;
                    entries[14].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[14].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    entries[15].binding = 146;  // photo_sampler (filtering)
                    entries[15].visibility = wgpu::ShaderStage::Compute;
                    entries[15].sampler.type = wgpu::SamplerBindingType::Filtering;

                    entries[16].binding = 152;  // patch_grid (O(1) spatial index for sample_terrain_y_at)
                    entries[16].visibility = wgpu::ShaderStage::Compute;
                    entries[16].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    // Agent registries (uniform — read-only, fixed size,
                    // never changes during a session). Originally tried as
                    // ReadOnlyStorage but pushed compute storage buffer
                    // count past the 10-per-stage limit; uniform has its
                    // own 12-per-stage budget and these tables (≤ 512 B
                    // total) fit comfortably.
                    entries[17].binding = 110;  // agent_behaviors
                    entries[17].visibility = wgpu::ShaderStage::Compute;
                    entries[17].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[18].binding = 111;  // agent_tier_gains
                    entries[18].visibility = wgpu::ShaderStage::Compute;
                    entries[18].buffer.type = wgpu::BufferBindingType::Uniform;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Compute Entity Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    computeEntityBindGroupLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!computeEntityBindGroupLayout_) return false;
                }

                // -- Render entity layout (Group 0) -- +200 offset from compute --
                //
                // Shared:   200-219  (render_signal, render_vp)
                // Terrain:  220-239  (render_terrain)
                // Agents:   260-279  (render_agents — VS-side mirror)
                // Camera:   280-299  (render_camera)
                // Sphere:   300-319  (render_sphere)
                // Light:    320-339  (render_light, render_point_lights)
                //
                // Vertex shaders need entity state for positioning + VP for transform.
                // Fragment shaders need camera for fog distance.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 19> entries{};

                    entries[0].binding = 1;    // config (uniform — fog, world_seed, aura_enabled, fade)
                    entries[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = 200;
                    entries[1].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[1].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[2].binding = 201;
                    entries[2].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[2].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[3].binding = 220;
                    entries[3].visibility = wgpu::ShaderStage::Fragment;
                    entries[3].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[4].binding = 260;
                    entries[4].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[4].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[5].binding = 280;
                    entries[5].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[5].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[6].binding = 300;
                    entries[6].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[6].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[7].binding = 320;
                    entries[7].visibility = wgpu::ShaderStage::Fragment;
                    entries[7].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[8].binding = 321;
                    entries[8].visibility = wgpu::ShaderStage::Fragment;
                    entries[8].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[9].binding = 322;
                    entries[9].visibility = wgpu::ShaderStage::Fragment;
                    entries[9].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[10].binding = 340;
                    entries[10].visibility = wgpu::ShaderStage::Vertex;
                    entries[10].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[11].binding = 360;
                    entries[11].visibility = wgpu::ShaderStage::Vertex;
                    entries[11].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[12].binding = 361;
                    entries[12].visibility = wgpu::ShaderStage::Vertex;
                    entries[12].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    // Tile grid (uniform — terrain wave delta needs amp_scale in VS,
                    // animated_cell_color needs archetype lookup in FS)
                    entries[13].binding = 25;
                    entries[13].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[13].buffer.type = wgpu::BufferBindingType::Uniform;

                    // Entity ground atlas (r32float 256×1 — VS reads ground_y via textureLoad)
                    entries[14].binding = 390;
                    entries[14].visibility = wgpu::ShaderStage::Vertex;
                    entries[14].texture.sampleType = wgpu::TextureSampleType::UnfilterableFloat;
                    entries[14].texture.viewDimension = wgpu::TextureViewDimension::e2D;

                    // Visible patch indices (GPU frustum cull output — VS reads indirection)
                    entries[15].binding = 391;
                    entries[15].visibility = wgpu::ShaderStage::Vertex;
                    entries[15].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    // Orb state (VS reads per-instance position/color/size for billboards)
                    entries[16].binding = 400;
                    entries[16].visibility = wgpu::ShaderStage::Vertex;
                    entries[16].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    // Orb config (VS reads dome_center for anchored mode in Pass 7)
                    entries[17].binding = 411;
                    entries[17].visibility = wgpu::ShaderStage::Vertex;
                    entries[17].buffer.type = wgpu::BufferBindingType::Uniform;

                    // Agent tier registry — same buffer as compute binding 111.
                    // Read by pawn_vs for entity color (tg.color_r/g/b).
                    // Uniform (not storage) to stay under the per-stage
                    // storage buffer cap; same buffer is bound as uniform
                    // on the compute side too.
                    entries[18].binding = 111;
                    entries[18].visibility = wgpu::ShaderStage::Vertex;
                    entries[18].buffer.type = wgpu::BufferBindingType::Uniform;

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

                    entries[0].binding = 1;
                    entries[0].visibility = wgpu::ShaderStage::Compute | wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Mesh Gen Entity Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    meshGenEntityBindGroupLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!meshGenEntityBindGroupLayout_) return false;
                }

                // (compute texture layout removed — 0D compute shaders use Group 0 only.
                //  Bindings 0-2 reserved for future Group 1 compute textures.)

                // -- Shadow texture layout (Group 1) -- bindings 22-23, 28 --
                // Used during shadow pass: samplers + patch heightfield only, NO shadow map.
                // Avoids read/write conflict (shadow map is depth attachment).
                // (bindings 20, 21, 24 removed — formerly legacy stub textures)
                {
                    std::array<wgpu::BindGroupLayoutEntry, 3> entries{};

                    entries[0].binding = 22;
                    entries[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[0].sampler.type = wgpu::SamplerBindingType::Filtering;

                    entries[1].binding = 23;
                    entries[1].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[1].sampler.type = wgpu::SamplerBindingType::NonFiltering;

                    entries[2].binding = 28;
                    entries[2].visibility = wgpu::ShaderStage::Vertex;
                    entries[2].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[2].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Shadow Texture Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    shadowTextureBindGroupLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!shadowTextureBindGroupLayout_) return false;
                }

                // -- Render texture layout (Group 1) -- bindings 22-23, 25-27, 28-30, 31-33 -
                // Used during main render pass: samplers + shadow maps + patches + field LUT + GoL zones + pawn aura.
                // (bindings 20, 21, 24 removed — formerly legacy stub textures)
                {
                    std::array<wgpu::BindGroupLayoutEntry, 11> entries{};

                    entries[0].binding = 22;
                    entries[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[0].sampler.type = wgpu::SamplerBindingType::Filtering;

                    entries[1].binding = 23;
                    entries[1].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[1].sampler.type = wgpu::SamplerBindingType::NonFiltering;

                    entries[2].binding = 25;
                    entries[2].visibility = wgpu::ShaderStage::Fragment;
                    entries[2].texture.sampleType = wgpu::TextureSampleType::Depth;
                    entries[2].texture.viewDimension = wgpu::TextureViewDimension::e2D;

                    entries[3].binding = 26;
                    entries[3].visibility = wgpu::ShaderStage::Fragment;
                    entries[3].sampler.type = wgpu::SamplerBindingType::Comparison;

                    // Spot shadow atlas (depth texture, sampled in spot PCF)
                    entries[4].binding = 27;
                    entries[4].visibility = wgpu::ShaderStage::Fragment;
                    entries[4].texture.sampleType = wgpu::TextureSampleType::Depth;
                    entries[4].texture.viewDimension = wgpu::TextureViewDimension::e2D;

                    entries[5].binding = 28;
                    entries[5].visibility = wgpu::ShaderStage::Vertex;
                    entries[5].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[5].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    entries[6].binding = 29;
                    entries[6].visibility = wgpu::ShaderStage::Fragment;
                    entries[6].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[6].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    // GoL zone life texture (fragment reads alive/dead per zone)
                    entries[7].binding = 31;
                    entries[7].visibility = wgpu::ShaderStage::Fragment;
                    entries[7].texture.sampleType = wgpu::TextureSampleType::UnfilterableFloat;
                    entries[7].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    // GoL zone config (fragment reads zone origins for lookup)
                    entries[8].binding = 32;
                    entries[8].visibility = wgpu::ShaderStage::Fragment;
                    entries[8].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    // Pawn aura texture (VS + FS read persistent influence field)
                    entries[9].binding = 33;
                    entries[9].visibility = wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Vertex;
                    entries[9].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[9].texture.viewDimension = wgpu::TextureViewDimension::e2D;

                    // Cell spatial field LUT (FS reads baked mode/style/sparse via textureLoad)
                    entries[10].binding = 30;
                    entries[10].visibility = wgpu::ShaderStage::Fragment;
                    entries[10].texture.sampleType = wgpu::TextureSampleType::UnfilterableFloat;
                    entries[10].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Render Texture Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    renderTextureBindGroupLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!renderTextureBindGroupLayout_) return false;
                }

                // -- Compute texture layout (Group 1) -- bindings 23, 33 -------
                // Live-contributor textures for compute pipelines that evaluate
                // ground policies which require sample_pawn_aura (POLICY_FLYER,
                // POLICY_WALKER). GoL and pyramid contributors read their
                // storage buffers directly through Group 0 (bindings 30, 160,
                // 161), so this layout only needs the aura texture + sampler.
                //
                // Attached to update_sphere, update_cube, update_agents, and
                // update_camera pipelines so contrib_pawn_aura_at →
                // sample_pawn_aura can run in the compute stage. Pipelines
                // that stay on the baked heightfield path
                // (compute_photographer_vp) do not bind this group.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 3> entries{};

                    entries[0].binding = 22;   // bilinear_sampler (used by sample_pawn_aura)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].sampler.type = wgpu::SamplerBindingType::Filtering;

                    entries[1].binding = 23;   // nearest_sampler (matches render texture layout; retained for future compute consumers)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].sampler.type = wgpu::SamplerBindingType::NonFiltering;

                    entries[2].binding = 33;   // pawn_aura_read (rgba16float, filterable)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[2].texture.viewDimension = wgpu::TextureViewDimension::e2D;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Compute Texture Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    computeTextureBindGroupLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!computeTextureBindGroupLayout_) return false;
                }

                // (mesh generation layout removed — legacy cell mesh gen.
                //  Bindings 40-45 reserved for future Group 1 systems.)

                // -- Terrain index gen layout (Group 0) -- binding 22 --------
                // One-shot compute pass: fills terrain index buffer at init.
                // Lives in terrain range (20-39).
                {
                    std::array<wgpu::BindGroupLayoutEntry, 1> entries{};

                    entries[0].binding = 22;
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Storage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Terrain Index Gen Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    terrainIndexGenLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!terrainIndexGenLayout_) return false;
                }

                // -- Patch heightfield gen layout (Group 0) -- bindings 23-24 -
                // Per-patch compute pass: fills one heightfield layer.
                // Dispatched when a patch enters the active set.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 9> entries{};

                    entries[0].binding = 1;    // config (uniform — world_seed for cell color gen)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = 23;
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[2].binding = 24;
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].storageTexture.access = wgpu::StorageTextureAccess::WriteOnly;
                    entries[2].storageTexture.format = wgpu::TextureFormat::RGBA16Float;
                    entries[2].storageTexture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    entries[3].binding = 25;
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[4].binding = 26;   // pier_instances (storage, read)
                    entries[4].visibility = wgpu::ShaderStage::Compute;
                    entries[4].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[5].binding = 27;
                    entries[5].visibility = wgpu::ShaderStage::Compute;
                    entries[5].storageTexture.access = wgpu::StorageTextureAccess::WriteOnly;
                    entries[5].storageTexture.format = wgpu::TextureFormat::RGBA8Unorm;
                    entries[5].storageTexture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    entries[6].binding = 30;  // pyramid_instances
                    entries[6].visibility = wgpu::ShaderStage::Compute;
                    entries[6].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[7].binding = 28;  // patch_height_scratch (two-pass heightfield gen)
                    entries[7].visibility = wgpu::ShaderStage::Compute;
                    entries[7].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[8].binding = 29;  // cell_fields_write (spatial field LUT, RGBA16Float)
                    entries[8].visibility = wgpu::ShaderStage::Compute;
                    entries[8].storageTexture.access = wgpu::StorageTextureAccess::WriteOnly;
                    entries[8].storageTexture.format = wgpu::TextureFormat::RGBA16Float;
                    entries[8].storageTexture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Patch Gen Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    patchGenLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!patchGenLayout_) return false;
                }

                // -- Ribbon compute layout (Group 0) -- dedicated pass ----------
                // Pre-computes ring transforms: motor + terrain_y for each ring.
                // Runs BEFORE update_world so pawn overlay can read results.
                //
                // Bindings: tile_grid @25, pier_instances @26, ribbon_state @120,
                // ring_xforms @121, head_poses @122.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 5> entries{};

                    entries[0].binding = 25;
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = 26;   // pier_instances (storage, read)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[2].binding = 120;
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[3].binding = 121;
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[4].binding = 122;  // head_poses (storage, read) — ribbon head-path
                    entries[4].visibility = wgpu::ShaderStage::Compute;
                    entries[4].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

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

                    entries[0].binding = 1;    // config (uniform — fog for FS, terrain_wave_overlay for VS)
                    entries[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = 201;
                    entries[1].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[1].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[2].binding = 280;
                    entries[2].visibility = wgpu::ShaderStage::Fragment;
                    entries[2].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[3].binding = 320;
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
                    std::array<wgpu::BindGroupLayoutEntry, 3> entries{};

                    entries[0].binding = 50;
                    entries[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[1].binding = 51;
                    entries[1].visibility = wgpu::ShaderStage::Fragment;
                    entries[1].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[1].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    entries[2].binding = 52;
                    entries[2].visibility = wgpu::ShaderStage::Fragment;
                    entries[2].sampler.type = wgpu::SamplerBindingType::Filtering;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Gallery Texture Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    galleryTextureBindGroupLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!galleryTextureBindGroupLayout_) return false;
                }

                // -- Photographer compute layout (Group 0) -- VP + terrain clamp --
                // Reads GPU pawn position + config → builds VP, clamps camera above terrain.
                // Entity Y-correction is handled separately by compute_entity_placement.
                // 1 config uniform + 3 storage + 1 read-only + 1 uniform + 1 texture + 1 sampler + 1 patch_grid = 9 entries.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 9> entries{};

                    entries[0].binding = 1;    // config (DesignConfig — possessed_slot lookup via compute_pawn_pos)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = 60;   // agent_state (read pos via possessed_slot)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[2].binding = 140;  // photographer_config (camera params)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[3].binding = 141;  // photographer_vp (output VP matrix)
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[4].binding = 142;  // photographer_camera_out (output camera pos)
                    entries[4].visibility = wgpu::ShaderStage::Compute;
                    entries[4].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[5].binding = 144;  // patch_instances (terrain lookup)
                    entries[5].visibility = wgpu::ShaderStage::Compute;
                    entries[5].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[6].binding = 145;  // patch_heightfield (terrain texture)
                    entries[6].visibility = wgpu::ShaderStage::Compute;
                    entries[6].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[6].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    entries[7].binding = 146;  // bilinear_sampler
                    entries[7].visibility = wgpu::ShaderStage::Compute;
                    entries[7].sampler.type = wgpu::SamplerBindingType::Filtering;

                    entries[8].binding = 152;  // patch_grid (O(1) spatial index for sample_terrain_y_at)
                    entries[8].visibility = wgpu::ShaderStage::Compute;
                    entries[8].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Photographer Compute Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    photographerComputeLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!photographerComputeLayout_) return false;
                }

                // -- Entity placement compute layout (Group 0) -- Y-correction --
                // Runs every frame, unconditionally. Samples the baked heightfield
                // and subtracts CPU-computed pier_correction to isolate each entity's
                // own pier contribution (removing foreign pier contamination).
                // 14 entries: config + pawn + painting slots + heightfield + entity grounds + GoL + ground atlas write + patch_grid.
                // Palm+cactus+blade share one buffer at binding 150: [0..23] palm, [24..43] cactus, [44..75] blade.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 14> entries{};

                    entries[0].binding = 1;
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = 60;
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[2].binding = 143;
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[3].binding = 144;
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[4].binding = 145;
                    entries[4].visibility = wgpu::ShaderStage::Compute;
                    entries[4].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[4].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    entries[5].binding = 146;
                    entries[5].visibility = wgpu::ShaderStage::Compute;
                    entries[5].sampler.type = wgpu::SamplerBindingType::Filtering;

                    entries[6].binding = 147;
                    entries[6].visibility = wgpu::ShaderStage::Compute;
                    entries[6].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[7].binding = 148;
                    entries[7].visibility = wgpu::ShaderStage::Compute;
                    entries[7].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[8].binding = 149;
                    entries[8].visibility = wgpu::ShaderStage::Compute;
                    entries[8].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[9].binding = 160;
                    entries[9].visibility = wgpu::ShaderStage::Compute;
                    entries[9].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[10].binding = 161;
                    entries[10].visibility = wgpu::ShaderStage::Compute;
                    entries[10].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[11].binding = 150;  // plant_ground: palm[0..23] + cactus[24..43] + blade[44..75]
                    entries[11].visibility = wgpu::ShaderStage::Compute;
                    entries[11].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[12].binding = 151;  // entity_ground_atlas_write (r32float storage texture)
                    entries[12].visibility = wgpu::ShaderStage::Compute;
                    entries[12].storageTexture.access = wgpu::StorageTextureAccess::WriteOnly;
                    entries[12].storageTexture.format = wgpu::TextureFormat::R32Float;
                    entries[12].storageTexture.viewDimension = wgpu::TextureViewDimension::e2D;

                    entries[13].binding = 152;  // patch_grid (O(1) spatial index for sample_terrain_y_at)
                    entries[13].visibility = wgpu::ShaderStage::Compute;
                    entries[13].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Entity Placement Compute Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    entityPlacementComputeLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!entityPlacementComputeLayout_) return false;
                }

                // -- Frustum cull compute layout (Group 0) -- GPU patch visibility --
                // Reads VP + camera + pawn + config + patch instances, writes visible indices + indirect draw args.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 7> entries{};

                    entries[0].binding = 1;     // config (uniform — render_patch_count)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = 2;     // vp_data (storage — VP matrix for frustum planes)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[2].binding = 80;    // camera_state (storage — retained for future use)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[3].binding = 340;   // patch_instances (storage — all patches)
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[4].binding = 500;   // visible_patch_indices (storage, rw — output)
                    entries[4].visibility = wgpu::ShaderStage::Compute;
                    entries[4].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[5].binding = 501;   // frustum_indirect (storage, rw — atomic draw args)
                    entries[5].visibility = wgpu::ShaderStage::Compute;
                    entries[5].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[6].binding = 60;    // agent_state (LOD distance reads possessed_slot)
                    entries[6].visibility = wgpu::ShaderStage::Compute;
                    entries[6].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Frustum Cull Compute Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    frustumCullLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!frustumCullLayout_) return false;
                }

                // -- GoL zone compute layout (Group 0) -- bindings 25,26,30,160-169 --
                // Shared by ALL zone entry points: sync, evolve, mesh_reset, mesh_gen, derive_params.
                // Mesh gen reads tile_grid, solids, pyramids for terrain height evaluation.
                // Mesh gen also samples the baked heightfield for exact terrain alignment.
                // derive_params writes zone_config.zones[slot] from tier tables.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 14> entries{};

                    entries[0].binding = 1;    // config (uniform — world_seed, fog, etc.)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    // Terrain evaluation (mesh gen needs these for zone_terrain_height)
                    entries[1].binding = 25;   // tile_grid (uniform)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[2].binding = 26;   // pier_instances (storage, read)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[3].binding = 30;   // pyramid_instances (uniform)
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].buffer.type = wgpu::BufferBindingType::Uniform;

                    // Zone state (storage — derive_params writes, sync/evolve/mesh read-write)
                    entries[4].binding = 160;  // zone_config (storage, rw)
                    entries[4].visibility = wgpu::ShaderStage::Compute;
                    entries[4].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[5].binding = 161;  // zone_life (storage, read/write)
                    entries[5].visibility = wgpu::ShaderStage::Compute;
                    entries[5].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[6].binding = 162;  // zone_life_tex (storage texture, write)
                    entries[6].visibility = wgpu::ShaderStage::Compute;
                    entries[6].storageTexture.access = wgpu::StorageTextureAccess::WriteOnly;
                    entries[6].storageTexture.format = wgpu::TextureFormat::RG32Float;
                    entries[6].storageTexture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    // Mesh output
                    entries[7].binding = 167;  // zone_mesh_vertices (storage, rw)
                    entries[7].visibility = wgpu::ShaderStage::Compute;
                    entries[7].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[8].binding = 168;  // zone_mesh_indices (storage, rw)
                    entries[8].visibility = wgpu::ShaderStage::Compute;
                    entries[8].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[9].binding = 169;  // zone_mesh_indirect (storage, rw)
                    entries[9].visibility = wgpu::ShaderStage::Compute;
                    entries[9].buffer.type = wgpu::BufferBindingType::Storage;

                    // Heightfield sampling (mesh gen uses baked terrain for exact alignment)
                    entries[10].binding = 163;  // patch_heightfield_array (sampled texture)
                    entries[10].visibility = wgpu::ShaderStage::Compute;
                    entries[10].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[10].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    entries[11].binding = 164; // heightfield_sampler (bilinear)
                    entries[11].visibility = wgpu::ShaderStage::Compute;
                    entries[11].sampler.type = wgpu::SamplerBindingType::Filtering;

                    entries[12].binding = 165; // patch_instances (read-only storage)
                    entries[12].visibility = wgpu::ShaderStage::Compute;
                    entries[12].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[13].binding = 166; // zone_derive_requests (uniform)
                    entries[13].visibility = wgpu::ShaderStage::Compute;
                    entries[13].buffer.type = wgpu::BufferBindingType::Uniform;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "GoL Zone Compute Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    zoneGolComputeLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!zoneGolComputeLayout_) return false;
                }

                // -- Pawn aura compute layout (Group 0) -- bindings 1, 60, 170-172 --
                {
                    std::array<wgpu::BindGroupLayoutEntry, 5> entries{};

                    entries[0].binding = 1;    // config (uniform — world_seed for cell color)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[1].binding = 60;   // agent_state (read possessed_slot position)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[2].binding = 170;  // pawn_aura_config (uniform)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[3].binding = 171;  // pawn_aura_cells (storage, rw)
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[4].binding = 172;  // pawn_aura_tex (storage texture, write)
                    entries[4].visibility = wgpu::ShaderStage::Compute;
                    entries[4].storageTexture.access = wgpu::StorageTextureAccess::WriteOnly;
                    entries[4].storageTexture.format = wgpu::TextureFormat::RGBA16Float;
                    entries[4].storageTexture.viewDimension = wgpu::TextureViewDimension::e2D;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Pawn Aura Compute Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    pawnAuraComputeLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!pawnAuraComputeLayout_) return false;
                }

                // -- Orb compute layout (Group 0) -- bindings 410 RW, 411 U, 412 RO --
                // Used by init / dynamics / recolor. orb_state is read_write so these
                // kernels can update it; orb_state_prev is read-only because only the
                // copy kernel writes it (through orbCopyLayout_).
                {
                    std::array<wgpu::BindGroupLayoutEntry, 3> entries{};

                    entries[0].binding = 410;  // orb_state (storage, read_write)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[1].binding = 411;  // orb_config (uniform)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[2].binding = 412;  // orb_state_prev (storage, read-only)
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

                    entries[0].binding = 413;  // orb_state_ro (storage, read-only)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[1].binding = 411;  // orb_config (uniform — declared at module scope)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[2].binding = 414;  // orb_state_prev_rw (storage, read_write)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Storage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Orb Copy Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    orbCopyLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!orbCopyLayout_) return false;
                }

                // -- Pyramid mesh gen layout (Group 0) -- bindings 190-192 --
                // Isolated from terrain evaluation: pure geometry generation.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 3> entries{};

                    entries[0].binding = 190;  // pmg_params (read-only storage)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[1].binding = 191;  // pmg_vertices (storage, rw)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[2].binding = 192;  // pmg_indices (storage, rw)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Storage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Pyramid Mesh Gen Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    pyramidMeshGenLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!pyramidMeshGenLayout_) return false;
                }

                // -- Arch mesh gen layout (Group 0) -- bindings 193-195 --
                {
                    std::array<wgpu::BindGroupLayoutEntry, 3> entries{};

                    entries[0].binding = 193;  // amg_params (read-only storage)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[1].binding = 194;  // amg_vertices (storage, rw)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[2].binding = 195;  // amg_indices (storage, rw)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Storage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Arch Mesh Gen Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    archMeshGenLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!archMeshGenLayout_) return false;
                }

                // -- Column mesh gen layout (Group 0) -- bindings 196-198 --
                {
                    std::array<wgpu::BindGroupLayoutEntry, 3> entries{};

                    entries[0].binding = 196;  // cmg_params (read-only storage)
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[1].binding = 197;  // cmg_vertices (storage, rw)
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[2].binding = 198;  // cmg_indices (storage, rw)
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Storage;

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

                    entries[0].binding = 180;
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[1].binding = 181;
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[2].binding = 182;
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
                    entries[0].binding = 183;
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
                    entries[1].binding = 184;
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Storage;
                    entries[2].binding = 185;
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
                    entries[0].binding = 186;
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
                    entries[1].binding = 187;
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Storage;
                    entries[2].binding = 188;
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Storage;
                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Blade Mesh Gen Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    bladeMeshGenLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!bladeMeshGenLayout_) return false;
                }


                // -- Bind group instances ------------------------------------

                // Compute entity bind group (17 entries: systems + terrain + GoL zones + portals + cached heightfield)
                {
                    std::array<wgpu::BindGroupEntry, 19> entries{};

                    entries[0].binding = 0;
                    entries[0].buffer = signalBuffer_;
                    entries[0].size = sizeof(GPUFrameSignal);

                    entries[1].binding = 1;
                    entries[1].buffer = configBuffer_;
                    entries[1].size = sizeof(GPUDesignConfig);

                    entries[2].binding = 2;
                    entries[2].buffer = vpBuffer_;
                    entries[2].size = sizeof(GPUVPMatrix);

                    entries[3].binding = 20;
                    entries[3].buffer = terrainBuffer_;
                    entries[3].size = sizeof(GPUTerrainState);

                    // (bindings 21, 40 removed — formerly proximity_field, cell_states)
                    // (binding 120 removed — ribbon_state only used by compute_ribbon_rings, separate group)

                    entries[4].binding = 60;
                    entries[4].buffer = agentStateBuffer_;
                    entries[4].size = Dim::MAX_AGENTS * sizeof(GPUAgentState);

                    entries[5].binding = 80;
                    entries[5].buffer = cameraBuffer_;
                    entries[5].size = sizeof(GPUCameraState);

                    entries[6].binding = 100;
                    entries[6].buffer = floatingEntityBuffer_;
                    entries[6].size = Dim::TOTAL_FLOATING_SLOTS * sizeof(GPUFloatingEntityState);

                    entries[7].binding = 101;
                    entries[7].buffer = trajectoriesBuffer_;
                    entries[7].size = sizeof(GPUTrajectory) * Dim::MAX_TRAJECTORIES;

                    entries[8].binding = 25;
                    entries[8].buffer = tileGridBuffer_;
                    entries[8].size = sizeof(GPUTileGrid);

                    // Pier Instances — unified terrain-raising volumes
                    entries[9].binding = 26;
                    entries[9].buffer = pierBuffer_;
                    entries[9].size = Dim::PIER_TOTAL * sizeof(GPUPierInstance);

                    // Pyramid Instances — tapered height in effective_ground_y
                    entries[10].binding = 30;
                    entries[10].buffer = pyramidInstancesBuffer_;
                    entries[10].size = sizeof(GPUPyramidArray);

                    // GoL zone state — cell height in effective_ground_y
                    entries[11].binding = 160;
                    entries[11].buffer = zoneConfigBuffer_;
                    entries[11].size = sizeof(GPUGoLZoneArray);

                    entries[12].binding = 161;
                    entries[12].buffer = zoneLifeBuffer_;
                    entries[12].size = Dim::MAX_GOL_ZONES * Dim::GOL_ZONE_LIFE_STRIDE * sizeof(float);

                    entries[13].binding = 62;
                    entries[13].buffer = portalArrayBuffer_;
                    entries[13].size = sizeof(GPUPortalArray);

                    // Cached patch heightfield — sample_terrain_y_at consumed by
                    // update_camera, update_agents, and any future
                    // POLICY_BAKED_HEIGHTFIELD compute consumer. Photographer
                    // and entity placement keep their dedicated layouts that
                    // also bind these — same handles, different layout slots.
                    entries[14].binding = 145;
                    entries[14].textureView = patchHeightfieldArrayReadView_;

                    entries[15].binding = 146;
                    entries[15].sampler = bilinearSampler_;

                    entries[16].binding = 152;
                    entries[16].buffer = patchGridBuffer_;
                    entries[16].size = sizeof(GPUPatchGrid);

                    // Agent registries — see modules/agents.inl for the
                    // authoring tables and GPUAgentBehaviorDef /
                    // GPUAgentTierDef in this file for GPU layout.
                    entries[17].binding = 110;
                    entries[17].buffer = agentBehaviorsBuffer_;
                    entries[17].size = GPU_AGENT_BEHAVIOR_COUNT * sizeof(GPUAgentBehaviorDef);

                    entries[18].binding = 111;
                    entries[18].buffer = agentTierGainsBuffer_;
                    entries[18].size = GPU_AGENT_TIER_COUNT * sizeof(GPUAgentTierDef);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Compute Entity BindGroup";
                    desc.layout = computeEntityBindGroupLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    computeEntityBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!computeEntityBindGroup_) return false;
                }

                // Render entity bind group (19 entries: config + spaced by system +200, plus shared agent_tier_gains at 111)
                {
                    std::array<wgpu::BindGroupEntry, 19> entries{};

                    entries[0].binding = 1;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);

                    entries[1].binding = 200;
                    entries[1].buffer = signalBuffer_;
                    entries[1].size = sizeof(GPUFrameSignal);

                    entries[2].binding = 201;
                    entries[2].buffer = vpBuffer_;
                    entries[2].size = sizeof(GPUVPMatrix);

                    entries[3].binding = 220;
                    entries[3].buffer = terrainBuffer_;
                    entries[3].size = sizeof(GPUTerrainState);

                    entries[4].binding = 260;
                    entries[4].buffer = agentStateBuffer_;
                    entries[4].size = Dim::MAX_AGENTS * sizeof(GPUAgentState);

                    entries[5].binding = 280;
                    entries[5].buffer = cameraBuffer_;
                    entries[5].size = sizeof(GPUCameraState);

                    entries[6].binding = 300;
                    entries[6].buffer = floatingEntityBuffer_;
                    entries[6].size = Dim::TOTAL_FLOATING_SLOTS * sizeof(GPUFloatingEntityState);

                    entries[7].binding = 320;
                    entries[7].buffer = directionalLightBuffer_;
                    entries[7].size = sizeof(GPUDirectionalLight);

                    entries[8].binding = 321;
                    entries[8].buffer = pointLightsBuffer_;
                    entries[8].size = sizeof(GPUPointLightArray);

                    entries[9].binding = 322;
                    entries[9].buffer = spotLightArrayBuffer_;
                    entries[9].size = sizeof(GPUSpotLightArray);

                    entries[10].binding = 340;
                    entries[10].buffer = patchInstancesBuffer_;
                    entries[10].size = sizeof(GPUPatchInstance) * Dim::MAX_ACTIVE_PATCHES;

                    entries[11].binding = 360;
                    entries[11].buffer = ribbonBuffer_;
                    entries[11].size = sizeof(GPURibbonState);

                    // Ring transforms (read-only for ribbon VS)
                    entries[12].binding = 361;
                    entries[12].buffer = ringTransformsBuffer_;
                    entries[12].size = sizeof(GPURibbonRingTransform) * Dim::RIBBON_MAX_RINGS;

                    entries[13].binding = 25;
                    entries[13].buffer = tileGridBuffer_;
                    entries[13].size = sizeof(GPUTileGrid);

                    // Entity ground atlas (r32float 256×1 — VS reads ground_y)
                    entries[14].binding = 390;
                    entries[14].textureView = entityGroundAtlasReadView_;

                    // Visible patch indices (GPU frustum cull output)
                    entries[15].binding = 391;
                    entries[15].buffer = visiblePatchIndicesBuffer_;
                    entries[15].size = Dim::MAX_ACTIVE_PATCHES * sizeof(uint32_t);

                    // Orb state (VS reads per-instance)
                    entries[16].binding = 400;
                    entries[16].buffer = orbStateBuffer_;
                    entries[16].size = Dim::MAX_ORBS * sizeof(GPUOrbState);

                    // Orb config (VS reads dome_center for anchored mode)
                    entries[17].binding = 411;
                    entries[17].buffer = orbConfigBuffer_;
                    entries[17].size = sizeof(GPUOrbConfig);

                    // Agent tier gains — same buffer as compute binding 111.
                    // Read by pawn_vs in the vertex stage for entity color
                    // (tier_idx → tg.color_r/g/b). Single source of truth
                    // is the C++ AGENT_TIER_GAINS table in modules/agents.inl.
                    entries[18].binding = 111;
                    entries[18].buffer = agentTierGainsBuffer_;
                    entries[18].size = GPU_AGENT_TIER_COUNT * sizeof(GPUAgentTierDef);

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

                    entries[0].binding = 1;
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

                // Compute texture bind group (3 entries: bindings 0-2)
                // (compute texture bind group removed — no Group 1 for 0D compute)

                // Shadow texture bind group (3 entries: bindings 22-23, 28)
                {
                    std::array<wgpu::BindGroupEntry, 3> entries{};

                    entries[0].binding = 22;
                    entries[0].sampler = bilinearSampler_;

                    entries[1].binding = 23;
                    entries[1].sampler = nearestSampler_;

                    entries[2].binding = 28;
                    entries[2].textureView = patchHeightfieldArrayReadView_;

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Shadow Texture BindGroup";
                    desc.layout = shadowTextureBindGroupLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    shadowTextureBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!shadowTextureBindGroup_) return false;
                }

                // Render texture bind group (11 entries: 22-23, 25-27, 28-30, 31-33)
                {
                    std::array<wgpu::BindGroupEntry, 11> entries{};

                    entries[0].binding = 22;
                    entries[0].sampler = bilinearSampler_;

                    entries[1].binding = 23;
                    entries[1].sampler = nearestSampler_;

                    entries[2].binding = 25;
                    entries[2].textureView = shadowMapView_;

                    entries[3].binding = 26;
                    entries[3].sampler = shadowSampler_;

                    entries[4].binding = 27;
                    entries[4].textureView = spotShadowMapView_;

                    entries[5].binding = 28;
                    entries[5].textureView = patchHeightfieldArrayReadView_;

                    entries[6].binding = 29;
                    entries[6].textureView = patchCellColorArrayReadView_;

                    entries[7].binding = 31;
                    entries[7].textureView = zoneLifeReadView_;

                    entries[8].binding = 32;
                    entries[8].buffer = zoneConfigBuffer_;
                    entries[8].size = sizeof(GPUGoLZoneArray);

                    entries[9].binding = 33;
                    entries[9].textureView = pawnAuraReadView_;

                    entries[10].binding = 30;  // cell_fields_read (spatial field LUT)
                    entries[10].textureView = cellFieldsArrayReadView_;

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Render Texture BindGroup";
                    desc.layout = renderTextureBindGroupLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    renderTextureBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!renderTextureBindGroup_) return false;
                }

                // Compute texture bind group (3 entries: 22 = bilinear_sampler, 23 = nearest_sampler, 33 = pawn_aura_read)
                {
                    std::array<wgpu::BindGroupEntry, 3> entries{};

                    entries[0].binding = 22;
                    entries[0].sampler = bilinearSampler_;

                    entries[1].binding = 23;
                    entries[1].sampler = nearestSampler_;

                    entries[2].binding = 33;
                    entries[2].textureView = pawnAuraReadView_;

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Compute Texture BindGroup";
                    desc.layout = computeTextureBindGroupLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    computeTextureBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!computeTextureBindGroup_) return false;
                }

                // (mesh generation bind group removed — legacy cell mesh gen)

                // Terrain index gen bind group (1 entry: binding 22)
                {
                    std::array<wgpu::BindGroupEntry, 1> entries{};

                    entries[0].binding = 22;
                    entries[0].buffer = terrainIndexBuffer_;
                    entries[0].size = Dim::TERRAIN_INDEX_COUNT * 4;

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Terrain Index Gen BindGroup";
                    desc.layout = terrainIndexGenLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    terrainIndexGenBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!terrainIndexGenBindGroup_) return false;
                }

                // Patch gen bind group (9 entries: binding 1, 23, 24, 25, 26, 27, 28, 29, 30)
                {
                    std::array<wgpu::BindGroupEntry, 9> entries{};

                    entries[0].binding = 1;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);

                    entries[1].binding = 23;
                    entries[1].buffer = patchParamsBuffer_;
                    entries[1].size = sizeof(GPUPatchParams);

                    entries[2].binding = 24;
                    entries[2].textureView = patchHeightfieldArrayWriteView_;

                    entries[3].binding = 25;
                    entries[3].buffer = tileGridBuffer_;
                    entries[3].size = sizeof(GPUTileGrid);

                    entries[4].binding = 26;
                    entries[4].buffer = pierBuffer_;
                    entries[4].size = Dim::PIER_TOTAL * sizeof(GPUPierInstance);

                    entries[5].binding = 27;
                    entries[5].textureView = patchCellColorArrayWriteView_;

                    entries[6].binding = 30;
                    entries[6].buffer = pyramidInstancesBuffer_;
                    entries[6].size = sizeof(GPUPyramidArray);

                    entries[7].binding = 28;  // patch_height_scratch
                    entries[7].buffer = patchHeightScratchBuffer_;
                    entries[7].size = Dim::PATCH_HEIGHTFIELD_N * Dim::PATCH_HEIGHTFIELD_N * 2 * sizeof(float);

                    entries[8].binding = 29;  // cell_fields_write
                    entries[8].textureView = cellFieldsArrayWriteView_;

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Patch Gen BindGroup";
                    desc.layout = patchGenLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    patchGenBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!patchGenBindGroup_) return false;
                }

                // Ribbon compute bind group (5 entries: dedicated ring transform pass)
                {
                    std::array<wgpu::BindGroupEntry, 5> entries{};

                    entries[0].binding = 25;  // tile_grid (matches @binding(25))
                    entries[0].buffer = tileGridBuffer_;
                    entries[0].size = sizeof(GPUTileGrid);

                    entries[1].binding = 26;  // pier_instances (matches @binding(26))
                    entries[1].buffer = pierBuffer_;
                    entries[1].size = Dim::PIER_TOTAL * sizeof(GPUPierInstance);

                    entries[2].binding = 120; // ribbon_state (matches @binding(120))
                    entries[2].buffer = ribbonBuffer_;
                    entries[2].size = sizeof(GPURibbonState);

                    entries[3].binding = 121; // ring_transforms (matches @binding(121))
                    entries[3].buffer = ringTransformsBuffer_;
                    entries[3].size = sizeof(GPURibbonRingTransform) * Dim::RIBBON_MAX_RINGS;

                    entries[4].binding = 122; // head_poses (matches @binding(122))
                    entries[4].buffer = headPosesBuffer_;
                    entries[4].size = sizeof(float) * 4 * Dim::RIBBON_MAX_RINGS;

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
                    entries[0].binding = 1;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);
                    entries[1].binding = 201;
                    entries[1].buffer = vpBuffer_;
                    entries[1].size = sizeof(GPUVPMatrix);
                    entries[2].binding = 280;
                    entries[2].buffer = cameraBuffer_;
                    entries[2].size = sizeof(GPUCameraState);
                    entries[3].binding = 320;
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

                // Gallery texture bind group created in initOffscreenResources()
                // (needs exhibitionReadView_ which depends on colorFormat)

                // Photographer render entity bind group (same layout as main, different VP)
                {
                    std::array<wgpu::BindGroupEntry, 19> entries{};
                    entries[0].binding = 1;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);
                    entries[1].binding = 200;
                    entries[1].buffer = signalBuffer_;
                    entries[1].size = sizeof(GPUFrameSignal);
                    entries[2].binding = 201;
                    entries[2].buffer = photographerVPBuffer_;  // ← THE DIFFERENCE
                    entries[2].size = sizeof(GPUVPMatrix);
                    entries[3].binding = 220;
                    entries[3].buffer = terrainBuffer_;
                    entries[3].size = sizeof(GPUTerrainState);
                    entries[4].binding = 260;
                    entries[4].buffer = agentStateBuffer_;
                    entries[4].size = Dim::MAX_AGENTS * sizeof(GPUAgentState);
                    entries[5].binding = 280;
                    entries[5].buffer = photographerCameraBuffer_;  // ← photographer pos for fog
                    entries[5].size = sizeof(GPUCameraState);
                    entries[6].binding = 300;
                    entries[6].buffer = floatingEntityBuffer_;
                    entries[6].size = Dim::TOTAL_FLOATING_SLOTS * sizeof(GPUFloatingEntityState);
                    entries[7].binding = 320;
                    entries[7].buffer = directionalLightBuffer_;
                    entries[7].size = sizeof(GPUDirectionalLight);
                    entries[8].binding = 321;
                    entries[8].buffer = pointLightsBuffer_;
                    entries[8].size = sizeof(GPUPointLightArray);
                    entries[9].binding = 322;
                    entries[9].buffer = spotLightArrayBuffer_;
                    entries[9].size = sizeof(GPUSpotLightArray);
                    entries[10].binding = 340;
                    entries[10].buffer = patchInstancesBuffer_;
                    entries[10].size = sizeof(GPUPatchInstance) * Dim::MAX_ACTIVE_PATCHES;
                    entries[11].binding = 360;
                    entries[11].buffer = ribbonBuffer_;
                    entries[11].size = sizeof(GPURibbonState);
                    entries[12].binding = 361;
                    entries[12].buffer = ringTransformsBuffer_;
                    entries[12].size = sizeof(GPURibbonRingTransform) * Dim::RIBBON_MAX_RINGS;
                    entries[13].binding = 25;
                    entries[13].buffer = tileGridBuffer_;
                    entries[13].size = sizeof(GPUTileGrid);
                    // Entity ground atlas (r32float 256×1 — VS reads ground_y)
                    entries[14].binding = 390;
                    entries[14].textureView = entityGroundAtlasReadView_;

                    // Visible patch indices (GPU frustum cull output)
                    entries[15].binding = 391;
                    entries[15].buffer = visiblePatchIndicesBuffer_;
                    entries[15].size = Dim::MAX_ACTIVE_PATCHES * sizeof(uint32_t);

                    // Orb state (VS reads per-instance) — same buffer as main path
                    entries[16].binding = 400;
                    entries[16].buffer = orbStateBuffer_;
                    entries[16].size = Dim::MAX_ORBS * sizeof(GPUOrbState);

                    // Orb config (VS reads dome_center) — same buffer as main path
                    entries[17].binding = 411;
                    entries[17].buffer = orbConfigBuffer_;
                    entries[17].size = sizeof(GPUOrbConfig);

                    // Agent tier gains — same buffer as main render path.
                    // Required because layout has it at index 18.
                    entries[18].binding = 111;
                    entries[18].buffer = agentTierGainsBuffer_;
                    entries[18].size = GPU_AGENT_TIER_COUNT * sizeof(GPUAgentTierDef);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Photographer Render Entity BindGroup";
                    desc.layout = renderEntityBindGroupLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    photographerRenderEntityBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!photographerRenderEntityBindGroup_) return false;
                }

                // Photographer compute bind group (9 entries: config + agents + outputs + terrain + patch_grid)
                {
                    std::array<wgpu::BindGroupEntry, 9> entries{};
                    entries[0].binding = 1;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);
                    entries[1].binding = 60;
                    entries[1].buffer = agentStateBuffer_;
                    entries[1].size = Dim::MAX_AGENTS * sizeof(GPUAgentState);
                    entries[2].binding = 140;
                    entries[2].buffer = photographerConfigBuffer_;
                    entries[2].size = sizeof(GPUPhotographerConfig);
                    entries[3].binding = 141;
                    entries[3].buffer = photographerVPBuffer_;
                    entries[3].size = sizeof(GPUVPMatrix);
                    entries[4].binding = 142;
                    entries[4].buffer = photographerCameraBuffer_;
                    entries[4].size = sizeof(GPUCameraState);
                    entries[5].binding = 144;
                    entries[5].buffer = patchInstancesBuffer_;
                    entries[5].size = sizeof(GPUPatchInstance) * Dim::MAX_ACTIVE_PATCHES;
                    entries[6].binding = 145;
                    entries[6].textureView = patchHeightfieldArrayReadView_;
                    entries[7].binding = 146;
                    entries[7].sampler = bilinearSampler_;
                    entries[8].binding = 152;
                    entries[8].buffer = patchGridBuffer_;
                    entries[8].size = sizeof(GPUPatchGrid);

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
                    std::array<wgpu::BindGroupEntry, 14> entries{};
                    entries[0].binding = 1;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);
                    entries[1].binding = 60;
                    entries[1].buffer = agentStateBuffer_;
                    entries[1].size = Dim::MAX_AGENTS * sizeof(GPUAgentState);
                    entries[2].binding = 143;
                    entries[2].buffer = paintingSlotsBuffer_;
                    entries[2].size = sizeof(GPUPaintingSlot) * Dim::PAINTING_MAX_SLOTS;
                    entries[3].binding = 144;
                    entries[3].buffer = patchInstancesBuffer_;
                    entries[3].size = sizeof(GPUPatchInstance) * Dim::MAX_ACTIVE_PATCHES;
                    entries[4].binding = 145;
                    entries[4].textureView = patchHeightfieldArrayReadView_;
                    entries[5].binding = 146;
                    entries[5].sampler = bilinearSampler_;
                    entries[6].binding = 147;
                    entries[6].buffer = archGroundBuffer_;
                    entries[6].size = sizeof(GPUArchGroundEntry) * Dim::MAX_ARCH_INSTANCES;
                    entries[7].binding = 148;
                    entries[7].buffer = columnGroundBuffer_;
                    entries[7].size = sizeof(GPUColumnGroundEntry) * Dim::MAX_COLUMN_INSTANCES;
                    entries[8].binding = 149;
                    entries[8].buffer = pyramidGroundBuffer_;
                    entries[8].size = sizeof(GPUPyramidGroundEntry) * Dim::MAX_PYRAMID_INSTANCES;
                    entries[9].binding = 160;
                    entries[9].buffer = zoneConfigBuffer_;
                    entries[9].size = sizeof(GPUGoLZoneArray);
                    entries[10].binding = 161;
                    entries[10].buffer = zoneLifeBuffer_;
                    entries[10].size = Dim::MAX_GOL_ZONES * Dim::GOL_ZONE_LIFE_STRIDE * sizeof(float);

                    // Combined plant ground: palm[0..23] + cactus[24..43] + blade[44..75]
                    static constexpr uint32_t PLANT_GROUND_COUNT =
                        Dim::MAX_PALM_INSTANCES + Dim::MAX_CACTUS_INSTANCES + Dim::MAX_BLADE_INSTANCES;
                    entries[11].binding = 150;
                    entries[11].buffer = plantComputeGroundBuffer_;
                    entries[11].size = sizeof(GPUPalmGroundEntry) * PLANT_GROUND_COUNT;

                    entries[12].binding = 151;
                    entries[12].textureView = entityGroundAtlasWriteView_;

                    entries[13].binding = 152;
                    entries[13].buffer = patchGridBuffer_;
                    entries[13].size = sizeof(GPUPatchGrid);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Entity Placement Compute BindGroup";
                    desc.layout = entityPlacementComputeLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    entityPlacementComputeBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!entityPlacementComputeBindGroup_) return false;
                }

                // Frustum cull compute bind group (7 entries: +agent_state at binding 60)
                {
                    std::array<wgpu::BindGroupEntry, 7> entries{};
                    entries[0].binding = 1;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);
                    entries[1].binding = 2;
                    entries[1].buffer = vpBuffer_;
                    entries[1].size = sizeof(GPUVPMatrix);
                    entries[2].binding = 80;
                    entries[2].buffer = cameraBuffer_;
                    entries[2].size = sizeof(GPUCameraState);
                    entries[3].binding = 340;
                    entries[3].buffer = patchInstancesBuffer_;
                    entries[3].size = sizeof(GPUPatchInstance) * Dim::MAX_ACTIVE_PATCHES;
                    entries[4].binding = 500;
                    entries[4].buffer = visiblePatchIndicesBuffer_;
                    entries[4].size = Dim::MAX_ACTIVE_PATCHES * sizeof(uint32_t);
                    entries[5].binding = 501;
                    entries[5].buffer = frustumComputeBuffer_;
                    entries[5].size = 5 * sizeof(uint32_t);
                    entries[6].binding = 60;
                    entries[6].buffer = agentStateBuffer_;
                    entries[6].size = Dim::MAX_AGENTS * sizeof(GPUAgentState);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Frustum Cull Compute BindGroup";
                    desc.layout = frustumCullLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    frustumCullBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!frustumCullBindGroup_) return false;
                }

                // GoL zone compute bind group (14 entries: config + terrain eval + zone state + mesh output + heightfield + derive requests)
                {
                    std::array<wgpu::BindGroupEntry, 14> entries{};

                    entries[0].binding = 1;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);

                    // Terrain evaluation
                    entries[1].binding = 25;
                    entries[1].buffer = tileGridBuffer_;
                    entries[1].size = sizeof(GPUTileGrid);
                    entries[2].binding = 26;
                    entries[2].buffer = pierBuffer_;
                    entries[2].size = Dim::PIER_TOTAL * sizeof(GPUPierInstance);
                    entries[3].binding = 30;
                    entries[3].buffer = pyramidInstancesBuffer_;
                    entries[3].size = sizeof(GPUPyramidArray);

                    // Zone state
                    entries[4].binding = 160;
                    entries[4].buffer = zoneConfigBuffer_;
                    entries[4].size = sizeof(GPUGoLZoneArray);
                    entries[5].binding = 161;
                    entries[5].buffer = zoneLifeBuffer_;
                    entries[5].size = Dim::MAX_GOL_ZONES * Dim::GOL_ZONE_LIFE_STRIDE * sizeof(float);
                    entries[6].binding = 162;
                    entries[6].textureView = zoneLifeWriteView_;

                    // Mesh output
                    entries[7].binding = 167;
                    entries[7].buffer = zoneMeshVertexBuffer_;
                    entries[7].size = Dim::ZONE_MESH_MAX_VERTICES * 44;
                    entries[8].binding = 168;
                    entries[8].buffer = zoneMeshIndexBuffer_;
                    entries[8].size = Dim::ZONE_MESH_MAX_INDICES * sizeof(uint32_t);
                    entries[9].binding = 169;
                    entries[9].buffer = zoneMeshIndirectBuffer_;
                    entries[9].size = 6 * sizeof(uint32_t);

                    // Heightfield sampling
                    entries[10].binding = 163;
                    entries[10].textureView = patchHeightfieldArrayReadView_;
                    entries[11].binding = 164;
                    entries[11].sampler = bilinearSampler_;
                    entries[12].binding = 165;
                    entries[12].buffer = patchInstancesBuffer_;
                    entries[12].size = sizeof(GPUPatchInstance) * Dim::MAX_ACTIVE_PATCHES;

                    // Derive request buffer
                    entries[13].binding = 166;
                    entries[13].buffer = zoneDeriveRequestBuffer_;
                    entries[13].size = sizeof(GPUZoneDeriveRequestArray);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "GoL Zone Compute BindGroup";
                    desc.layout = zoneGolComputeLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    zoneGolComputeBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!zoneGolComputeBindGroup_) return false;
                }

                // Pawn aura compute bind group (5 entries: 1, 60, 170-172)
                {
                    std::array<wgpu::BindGroupEntry, 5> entries{};

                    entries[0].binding = 1;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);

                    entries[1].binding = 60;
                    entries[1].buffer = agentStateBuffer_;
                    entries[1].size = Dim::MAX_AGENTS * sizeof(GPUAgentState);

                    entries[2].binding = 170;
                    entries[2].buffer = pawnAuraConfigBuffer_;
                    entries[2].size = sizeof(GPUPawnAuraConfig);

                    entries[3].binding = 171;
                    entries[3].buffer = pawnAuraCellsBuffer_;
                    entries[3].size = PAWN_AURA_N * PAWN_AURA_N * sizeof(GPUPawnAuraCell);

                    entries[4].binding = 172;
                    entries[4].textureView = pawnAuraWriteView_;

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Pawn Aura Compute BindGroup";
                    desc.layout = pawnAuraComputeLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    pawnAuraComputeGroup_ = device_.CreateBindGroup(&desc);
                    if (!pawnAuraComputeGroup_) return false;
                }

                // Orb compute bind group (2 entries: state storage rw + config uniform)
                {
                    std::array<wgpu::BindGroupEntry, 3> entries{};

                    entries[0].binding = 410;
                    entries[0].buffer = orbStateBuffer_;
                    entries[0].size = Dim::MAX_ORBS * sizeof(GPUOrbState);

                    entries[1].binding = 411;
                    entries[1].buffer = orbConfigBuffer_;
                    entries[1].size = sizeof(GPUOrbConfig);

                    entries[2].binding = 412;
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

                    entries[0].binding = 413;
                    entries[0].buffer = orbStateBuffer_;
                    entries[0].size = Dim::MAX_ORBS * sizeof(GPUOrbState);

                    entries[1].binding = 411;
                    entries[1].buffer = orbConfigBuffer_;
                    entries[1].size = sizeof(GPUOrbConfig);

                    entries[2].binding = 414;
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

                // Pyramid mesh gen bind group
                {
                    std::array<wgpu::BindGroupEntry, 3> entries{};

                    entries[0].binding = 190;
                    entries[0].buffer = pyramidMeshParamsBuffer_;
                    entries[0].size = sizeof(GPUPyramidMeshParams) * Dim::MAX_PYRAMID_INSTANCES;

                    entries[1].binding = 191;
                    entries[1].buffer = pyramidVertexBuffer_;
                    entries[1].size = Dim::PMG_TOTAL_VERTICES * sizeof(ArchVertex);

                    entries[2].binding = 192;
                    entries[2].buffer = pyramidIndexBuffer_;
                    entries[2].size = Dim::PMG_TOTAL_INDICES * sizeof(uint32_t);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Pyramid Mesh Gen BindGroup";
                    desc.layout = pyramidMeshGenLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    pyramidMeshGenBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!pyramidMeshGenBindGroup_) return false;
                }

                // Arch mesh gen bind group (dedicated layout — bindings 193-195)
                {
                    std::array<wgpu::BindGroupEntry, 3> entries{};

                    entries[0].binding = 193;
                    entries[0].buffer = archMeshParamsBuffer_;
                    entries[0].size = sizeof(GPUArchMeshParams) * Dim::MAX_ARCH_INSTANCES;

                    entries[1].binding = 194;
                    entries[1].buffer = archVertexBuffer_;
                    entries[1].size = Dim::AMG_TOTAL_VERTICES * sizeof(ArchVertex);

                    entries[2].binding = 195;
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

                // Column mesh gen bind group (dedicated layout — bindings 196-198)
                {
                    std::array<wgpu::BindGroupEntry, 3> entries{};

                    entries[0].binding = 196;
                    entries[0].buffer = columnMeshParamsBuffer_;
                    entries[0].size = sizeof(GPUColumnMeshParams) * Dim::MAX_COLUMN_INSTANCES;

                    entries[1].binding = 197;
                    entries[1].buffer = columnVertexBuffer_;
                    entries[1].size = Dim::CMG_TOTAL_VERTICES * sizeof(ArchVertex);

                    entries[2].binding = 198;
                    entries[2].buffer = columnIndexBuffer_;
                    entries[2].size = Dim::CMG_TOTAL_INDICES * sizeof(uint32_t);

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

                    entries[0].binding = 180;
                    entries[0].buffer = palmMeshParamsBuffer_;
                    entries[0].size = Dim::MAX_PALM_INSTANCES * sizeof(GPUPalmMeshParams);

                    entries[1].binding = 181;
                    entries[1].buffer = palmVertexBuffer_;
                    entries[1].size = Dim::PALMG_TOTAL_VERTICES * sizeof(ArchVertex);

                    entries[2].binding = 182;
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
                    entries[0].binding = 183;
                    entries[0].buffer = cactusMeshParamsBuffer_;
                    entries[0].size = Dim::MAX_CACTUS_INSTANCES * sizeof(GPUCactusMeshParams);
                    entries[1].binding = 184;
                    entries[1].buffer = cactusVertexBuffer_;
                    entries[1].size = Dim::CACTUSG_TOTAL_VERTICES * sizeof(ArchVertex);
                    entries[2].binding = 185;
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
                    entries[0].binding = 186;
                    entries[0].buffer = bladeMeshParamsBuffer_;
                    entries[0].size = Dim::MAX_BLADE_INSTANCES * sizeof(GPUBladeClusterMeshParams);
                    entries[1].binding = 187;
                    entries[1].buffer = bladeVertexBuffer_;
                    entries[1].size = Dim::BLADEG_TOTAL_VERTICES * sizeof(ArchVertex);
                    entries[2].binding = 188;
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
                config_.mute_dynamics_2d = 0;
                config_.mute_signal = 0;
                config_.mute_couplings = Coupling::NONE;
                config_.wave_time_scale = Idle::WAVE_TIME_SCALE;
                config_.pawn_speed = Idle::PAWN_SPEED;
                config_.camera_sensitivity = Idle::CAMERA_SENSITIVITY;
                config_.freeze_sphere = 0;
                config_.active_cell_size = Idle::ACTIVE_CELL_SIZE;
                config_.fpv_mode = 0;
                config_.wave_enable_mask = 0x7;  // All 3 waves enabled
                config_.wave_freeze_mask = 0;
                config_.wave_frozen_t[0] = 0.0f;
                config_.wave_frozen_t[1] = 0.0f;
                config_.wave_frozen_t[2] = 0.0f;
                config_.world_seed = 42;
                // Default sun direction (normalized) — matches MOOD_TABLE[0].sun_direction
                {
                    float d[3] = { 0.69f, -0.71f, -0.14f };
                    float len = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
                    config_.sun_direction[0] = d[0] / len;
                    config_.sun_direction[1] = d[1] / len;
                    config_.sun_direction[2] = d[2] / len;
                }
                config_.aura_enabled = 1.0f;
                config_.pawn_amp_scale = 1.0f;
                config_.pawn_height_bias = 0.0f;
                config_.pawn_aura_height = 0.0f;
                config_.fog_density = 0.003f;
                config_.fog_color[0] = 0.85f;
                config_.fog_color[1] = 0.78f;
                config_.fog_color[2] = 0.72f;
                config_.fade_alpha = 0.0f;
                config_.fade_color[0] = 0.0f;
                config_.fade_color[1] = 0.0f;
                config_.fade_color[2] = 0.0f;
                config_.pier_count = 0;
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
                // Band motion: -1 = use activity field (default for all moods at boot)
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

                GPUTerrainState terrain{};
                terrain.amplitude_scale = Idle::AMPLITUDE_SCALE;
                terrain.max_amplitude = Idle::MAX_AMPLITUDE;
                terrain.size = Idle::SIZE;
                terrain.lipschitz_factor = Idle::LIPSCHITZ_FACTOR;
                terrain.tint[0] = 1.0f;
                terrain.tint[1] = 1.0f;
                terrain.tint[2] = 1.0f;
                terrain._pad = 0.0f;
                queue.WriteBuffer(terrainBuffer_, 0, &terrain, sizeof(terrain));

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
                // Populate slot 0 with default sphere (idle orbit around origin)
                {
                    GPUFloatingEntityState fe{};
                    fe.pos[0] = Idle::SPHERE_ORBIT_RADIUS;
                    fe.pos[1] = Idle::SPHERE_HOVER_HEIGHT;
                    fe.pos[2] = 0.0f;
                    fe.body_radius = Idle::SPHERE_RADIUS;
                    fe.orientation[0] = 0.0f;
                    fe.orientation[1] = 0.0f;
                    fe.orientation[2] = 0.0f;
                    fe.orientation[3] = 1.0f;
                    fe.influence_radius = Idle::SPHERE_INFLUENCE_RADIUS;
                    fe.t = 0.0f;
                    fe.orbit_radius = Idle::SPHERE_ORBIT_RADIUS;
                    fe.orbit_speed = Idle::SPHERE_ORBIT_SPEED;
                    fe.color[0] = 0.95f;
                    fe.color[1] = 0.75f;
                    fe.color[2] = 0.4f;
                    fe.orbit_height = Idle::SPHERE_HOVER_HEIGHT;
                    fe.anchor[0] = 0.0f;
                    fe.anchor[1] = 0.0f;
                    fe.anchor[2] = 0.0f;
                    fe.face_variance = 0.0f;
                    fe.base_color[0] = 0.95f;
                    fe.base_color[1] = 0.75f;
                    fe.base_color[2] = 0.4f;
                    fe.geometry_type = 0;    // sphere
                    fe.motion_type = 0;      // orbit
                    fe.spin_speed = 0.0f;
                    fe.bob_amplitude = 0.0f;
                    fe.bob_period = 0.0f;
                    fe.spin_tilt_x = 0.0f;
                    fe.spin_tilt_z = 0.0f;
                    fe.entity_seed = 0;
                    fe.is_active = 1;
                    fe.aspect_y = 1.0f;
                    fe.aspect_z = 1.0f;
                    // Drift-integrator substrate — unused on spheres; zero
                    // so update_cube would skip cleanly if motion_type were
                    // ever flipped to hover-bob on this slot.
                    fe.spring_stiffness = 0.0f;
                    fe.drag = 0.0f;
                    fe.tier_idx = 0;
                    fe.drift[0] = 0.0f; fe.drift[1] = 0.0f; fe.drift[2] = 0.0f;
                    fe.drift_vel[0] = 0.0f; fe.drift_vel[1] = 0.0f; fe.drift_vel[2] = 0.0f;
                    // Behavior registry fields — unused on spheres; zero
                    // means CUBE_BEHAVIOR_STATIONARY (no behavior force).
                    fe.behavior_id = 0;
                    fe.behavior_phase = 0;
                    // Kite-mode fields — unused on spheres.
                    fe.follow_pawn = 0;
                    fe.pawn_offset[0] = 0.0f; fe.pawn_offset[1] = 0.0f; fe.pawn_offset[2] = 0.0f;
                    queue.WriteBuffer(floatingEntityBuffer_, 0, &fe, sizeof(fe));
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

                GPUTrajectory trajectories[Dim::MAX_TRAJECTORIES]{};
                for (int i = 0; i < Dim::MAX_TRAJECTORIES; ++i) {
                    trajectories[i].value = Idle::TRAJECTORY_VALUE;
                    trajectories[i].velocity = Idle::TRAJECTORY_VELOCITY;
                }
                queue.WriteBuffer(trajectoriesBuffer_, 0, trajectories, sizeof(trajectories));

                // Pier instances (all inactive — cartridge uploads test rig at setup)
                {
                    std::vector<GPUPierInstance> emptyPiers(Dim::PIER_TOTAL);
                    queue.WriteBuffer(pierBuffer_, 0, emptyPiers.data(),
                        sizeof(GPUPierInstance) * Dim::PIER_TOTAL);
                }

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

    } // namespace the_chord
} // namespace t7