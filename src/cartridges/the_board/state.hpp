#pragma once

// THE_BOARD CARTRIDGE — GPU State Management (Rasterized)
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
//   60        pawn_state           Storage          —
//   80        camera_state         Storage          —
//   100       sphere_state         Storage          —
//   101       trajectories         Storage          —
//   120       ribbon_state         Uniform          —
//   121       ring_transforms      Storage          —
//   200       frame_signal         —                ReadOnlyStorage
//   201       vp_matrix            —                ReadOnlyStorage
//   220       terrain_state        —                ReadOnlyStorage
//   260       pawn_state           —                ReadOnlyStorage
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
    namespace the_board {

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

            // GoL zone system — per-zone automaton grids
            constexpr uint32_t MAX_GOL_ZONES = 8;
            constexpr uint32_t GOL_ZONE_GRID = 32;      // cells per zone side
            constexpr uint32_t GOL_ZONE_CELLS = GOL_ZONE_GRID * GOL_ZONE_GRID;  // 1024
            constexpr uint32_t GOL_ZONE_LIFE_STRIDE = GOL_ZONE_CELLS * 7;  // 7 slots: visual, velocity, target, next, height_factor, color_visual, color_velocity

            // Zone cell mesh extrusion budget
            constexpr uint32_t ZONE_MESH_MAX_VERTICES = 50000;
            constexpr uint32_t ZONE_MESH_MAX_INDICES = 75000;
            constexpr uint32_t MAX_ZONE_MESH_VERTICES = 200000;  // 8 zones × up to 1024 cells × 5 quads × 4 verts
            constexpr uint32_t MAX_ZONE_MESH_INDICES = 300000;
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

        namespace Coupling {
            constexpr uint32_t POLYPHONY_TO_AMPLITUDE = 1u << 0;
            constexpr uint32_t PAWN_TO_FIELD_COLOR = 1u << 7;
            constexpr uint32_t SPHERE_TO_FIELD_COLOR = 1u << 8;
            // Naming alignment with WGSL (same bit values)
            constexpr uint32_t PAWN_TO_PROXIMITY_FIELD = PAWN_TO_FIELD_COLOR;
            constexpr uint32_t SPHERE_TO_PROXIMITY_FIELD = SPHERE_TO_FIELD_COLOR;
            constexpr uint32_t POLYPHONY_TO_CELL_COLOR = 1u << 9;
            constexpr uint32_t PAWN_TO_CELL_COLOR = 1u << 10;
            constexpr uint32_t SPHERE_TO_CELL_COLOR = 1u << 11;
            constexpr uint32_t POLYPHONY_TO_SPHERE_COLOR = 1u << 12;
            constexpr uint32_t SPHERE_TO_TERRAIN_TINT = 1u << 13;
            constexpr uint32_t TERRAIN_TO_PAWN_Y = 1u << 1;
            constexpr uint32_t TERRAIN_TO_PAWN_TILT = 1u << 2;
            constexpr uint32_t PAWN_TO_CAMERA_TARGET = 1u << 3;
            constexpr uint32_t TERRAIN_TO_SPHERE_HEIGHT = 1u << 14;
            constexpr uint32_t RANDOM_TO_CELL_GOALS = 1u << 15;
            constexpr uint32_t INPUT_MOVES_PAWN = 1u << 4;
            constexpr uint32_t INPUT_ORBITS_CAMERA = 1u << 5;
            constexpr uint32_t INPUT_ZOOMS_CAMERA = 1u << 6;
            constexpr uint32_t PAWN_TO_SUN_VP = 1u << 16;
            constexpr uint32_t PAWN_TO_ZONE_HEIGHT = 1u << 17;
            constexpr uint32_t PAWN_TO_ZONE_COLOR = 1u << 18;
            constexpr uint32_t SPHERE_TO_ZONE_HEIGHT = 1u << 19;
            constexpr uint32_t SPHERE_TO_ZONE_COLOR = 1u << 20;
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
            float _pad_mode_3;

            // ─── Radial pulse ring buffer ────────────────────────────────
            // 8 recent note onsets, indexed as pulse_data[i*4 + field]:
            //   field 0 = origin_x, 1 = origin_z, 2 = onset_beats, 3 = amplitude
            // Evaluated in terrain VS + update_pawn as expanding ring wavefronts.
            uint32_t pulse_count;             // active entries (0–8)
            float _pulse_pad[3];
            float pulse_data[32];             // 8 × {origin_x, origin_z, onset_beats, amplitude}
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

        struct alignas(16) GPUPawnState {
            float pos[3];
            float heading;
            float orientation[4];      // quaternion (x, y, z, w)
            float velocity[2];         // XZ velocity (for force field radius)
            int32_t portal_trigger;    // -1 = none, >=0 = arch index that triggered
            float _pad1;
        };

        struct alignas(16) GPUCameraState {
            float pos[3];
            float azimuth;
            float elevation;
            float distance;
            float pan_x;
            float pan_y;
        };

        struct alignas(16) GPUSphereState {
            float pos[3];              //  0: world position (computed by GPU)
            float radius;              // 12: body radius
            float orientation[4];      // 16: quaternion
            float influence_radius;    // 32: zone/terrain influence range
            float t;                   // 36: curve parameter
            float orbit_radius;        // 40: distance from anchor
            float orbit_speed;         // 44: angular velocity
            float color[3];            // 48: current appearance (coupling-driven)
            float orbit_height;        // 60: base altitude above terrain
            float anchor[3];           // 64: orbit center (world XZ, Y=0)
            float color_mode;          // 76: (reserved — future color tier)
            float base_color[3];       // 80: seed-derived rest color
            float _pad0;               // 92
        };                             // 96 total

        struct alignas(16) GPURibbonState {
            float anchor[3];                                                    // 0
            float time;                                                         // 12
            uint32_t cube_count;                                                // 16
            float cube_size;                                                    // 20
            float height;                                                       // 24
            float twist_amp;                                                    // 28
            float color[3];                                                     // 32
            float lateral_amp;                                                  // 44
            float lateral_cycles;                                               // 48
            float lateral_speed;                                                // 52
            float vertical_amp;                                                 // 56
            float vertical_cycles;                                              // 60
            float vertical_speed;                                               // 64
            float twist_cycles;                                                 // 68
            float twist_speed;                                                  // 72
            uint32_t is_visible;                                                // 76
            float orientation;                                                  // 80 (heading radians)
            uint32_t color_mode;                                                // 84
            float _pad0;                                                        // 88
            float _pad1;                                                        // 92
        };                                                                      // 96 total

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
        // Must match WGSL PyramidMeshParams exactly.
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
        static_assert(sizeof(GPUPyramidMeshParams) == 48, "GPUPyramidMeshParams must be 48 bytes");

        // GPU arch mesh generation parameters (CPU → GPU per-slot).
        // Must match WGSL ArchMeshParams exactly. Catenary parameter 'a'
        // is precomputed on CPU (50-iter bisection) to keep the shader simple.
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
        static_assert(sizeof(GPUArchMeshParams) == 64, "GPUArchMeshParams must be 64 bytes");

        // GPU column mesh generation parameters (CPU → GPU per-slot).
        // Must match WGSL ColumnMeshParams exactly.
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
        static_assert(sizeof(GPUColumnMeshParams) == 128, "GPUColumnMeshParams must be 128 bytes");

        // ─── Palm GPU structs ─────────────────────────────────────────────
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
        static_assert(sizeof(GPUPalmMeshParams) == 128, "GPUPalmMeshParams must be 128 bytes");

        struct alignas(16) GPUPalmGroundEntry {
            float center_x;
            float center_z;
            float ground_y;
            uint32_t is_active;
            float _pad0, _pad1, _pad2, _pad3;
        };
        static_assert(sizeof(GPUPalmGroundEntry) == 32, "GPUPalmGroundEntry must be 32 bytes");

        // ─── Cactus GPU structs ──────────────────────────────────────────
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
        static_assert(sizeof(GPUCactusMeshParams) == 128, "GPUCactusMeshParams must be 128 bytes");

        struct alignas(16) GPUCactusGroundEntry {
            float center_x;
            float center_z;
            float ground_y;
            uint32_t is_active;
            float _pad0, _pad1, _pad2, _pad3;
        };
        static_assert(sizeof(GPUCactusGroundEntry) == 32, "GPUCactusGroundEntry must be 32 bytes");

        // ─── Blade Cluster GPU structs ──────────────────────────────────

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
            "GPUBladeClusterMeshParams must be 80 bytes");

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

        static_assert(sizeof(GPUFrameSignal) == 304, "GPUFrameSignal must be 304 bytes");
        static_assert(sizeof(GPUDesignConfig) == 384, "GPUDesignConfig must be 384 bytes");

        // Portal proximity array — uploaded when portal set changes.
        // GPU update_pawn checks pawn proximity and writes portal_trigger.
        static constexpr uint32_t MAX_GPU_PORTALS = 32;
        struct alignas(16) GPUPortalEntry {
            float x;
            float z;
            float trigger_radius;
            uint32_t arch_index;       // maps to CPU activeArches_[index]
        };
        struct alignas(16) GPUPortalArray {
            uint32_t count;
            uint32_t _pad[3];
            GPUPortalEntry portals[MAX_GPU_PORTALS];
        };
        static_assert(sizeof(GPUPortalArray) == 16 + MAX_GPU_PORTALS * 16,
            "GPUPortalArray layout check");
        static_assert(sizeof(GPUTrajectory) == 16, "GPUTrajectory must be 16 bytes");
        static_assert(sizeof(GPUTerrainState) == 32, "GPUTerrainState must be 32 bytes");
        static_assert(sizeof(GPUPawnState) == 48, "GPUPawnState must be 48 bytes");
        static_assert(sizeof(GPUCameraState) == 32, "GPUCameraState must be 32 bytes");
        static_assert(sizeof(GPUSphereState) == 96, "GPUSphereState must be 96 bytes");
        static_assert(sizeof(GPURibbonState) == 96, "GPURibbonState must be 96 bytes");
        static_assert(sizeof(GPUVPMatrix) == 128, "GPUVPMatrix must be 128 bytes");
        static_assert(sizeof(GPUDirectionalLight) == 48, "GPUDirectionalLight must be 48 bytes");
        static_assert(sizeof(GPUPointLight) == 32, "GPUPointLight must be 32 bytes");
        static_assert(sizeof(GPUPointLightArray) == 272, "GPUPointLightArray must be 272 bytes");
        static_assert(sizeof(MeshVertex) == 24, "MeshVertex must be 24 bytes");
        static_assert(sizeof(GPUPatchParams) == 32, "GPUPatchParams must be 32 bytes");
        static_assert(sizeof(GPUPatchInstance) == 16, "GPUPatchInstance must be 16 bytes");

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

            wgpu::Buffer signalBuffer_, configBuffer_, terrainBuffer_, pawnBuffer_;
            wgpu::Buffer cameraBuffer_, sphereBuffer_, trajectoriesBuffer_;
            wgpu::Buffer ribbonBuffer_;
            wgpu::Buffer ringTransformsBuffer_;
            // (bindings 21, 40 reserved — formerly proximity_field, cell_states)
            wgpu::Buffer pierBuffer_;   // unified pier instances (Storage | CopyDst)
            wgpu::Buffer vpBuffer_;
            wgpu::Buffer directionalLightBuffer_;
            wgpu::Buffer pointLightsBuffer_;
            wgpu::Buffer spotLightArrayBuffer_;
            wgpu::Buffer spotVPStagingBuffer_;   // 4 × 64 bytes: pre-staged per-light VPs for atlas copy
            wgpu::Buffer portalArrayBuffer_;
            wgpu::Buffer pawnReadbackStaging_;   // full GPUPawnState readback (position + portal trigger)

            wgpu::Buffer patchParamsBuffer_;
            wgpu::Buffer patchStagingBuffer_;    // N×GPUPatchParams for batched generation
            wgpu::Buffer patchInstancesBuffer_;
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

            wgpu::Buffer terrainIndexBuffer_;
            // (legacy cell mesh buffers removed — bindings 43-45 reserved)
            wgpu::Buffer sphereVertexBuffer_, sphereIndexBuffer_;
            uint32_t sphereIndexCount_ = 0;

            wgpu::Buffer archVertexBuffer_, archIndexBuffer_;
            wgpu::Buffer archGroundBuffer_;  // per-arch ground Y correction (GPU-corrected)
            wgpu::Buffer archMeshParamsBuffer_;  // GPU mesh gen: per-slot parameters
            uint32_t archIndexCount_ = 0;

            wgpu::Buffer columnVertexBuffer_, columnIndexBuffer_;
            wgpu::Buffer columnGroundBuffer_;  // per-column ground Y correction (GPU-corrected)
            wgpu::Buffer columnMeshParamsBuffer_;  // GPU mesh gen: per-slot parameters
            uint32_t columnIndexCount_ = 0;

            wgpu::Buffer palmVertexBuffer_, palmIndexBuffer_;
            wgpu::Buffer palmGroundBuffer_;
            wgpu::Buffer palmMeshParamsBuffer_;
            uint32_t palmIndexCount_ = 0;

            wgpu::Buffer cactusVertexBuffer_, cactusIndexBuffer_;
            wgpu::Buffer cactusGroundBuffer_;
            wgpu::Buffer cactusMeshParamsBuffer_;
            uint32_t cactusIndexCount_ = 0;

            wgpu::Buffer bladeVertexBuffer_, bladeIndexBuffer_;
            wgpu::Buffer bladeGroundBuffer_;
            wgpu::Buffer bladeMeshParamsBuffer_;
            uint32_t bladeIndexCount_ = 0;

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

            // (legacy 1×1 stub textures removed — render bindings 20-21, 24 reserved)

            wgpu::Texture shadowMapTexture_;
            wgpu::TextureView shadowMapView_;
            wgpu::Texture spotShadowMapTexture_;
            wgpu::TextureView spotShadowMapView_;

            wgpu::Sampler bilinearSampler_, nearestSampler_;
            wgpu::Sampler shadowSampler_;

            wgpu::BindGroupLayout computeEntityBindGroupLayout_, renderEntityBindGroupLayout_;
            wgpu::BindGroupLayout renderTextureBindGroupLayout_, shadowTextureBindGroupLayout_;
            // (computeTextureBindGroupLayout_ removed — 0D compute shaders use Group 0 only)
            wgpu::BindGroupLayout meshGenEntityBindGroupLayout_;  // binding 1 only — still used by fade overlay
            // (meshGenBindGroupLayout_ removed — legacy cell mesh gen)
            wgpu::BindGroupLayout terrainIndexGenLayout_;
            wgpu::BindGroupLayout patchGenLayout_;
            wgpu::BindGroupLayout ribbonComputeLayout_;

            wgpu::BindGroup computeEntityBindGroup_, renderEntityBindGroup_;
            wgpu::BindGroup renderTextureBindGroup_, shadowTextureBindGroup_;
            // (computeTextureBindGroup_ removed)
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
            // after allPatchCount_ is finalized, so the placement compute pass reads the
            // current frame's patch set (decoupled from the photographer config).
            void upload_placement_patch_count(wgpu::Queue& queue) {
                static_assert(offsetof(GPUDesignConfig, placement_patch_count) == 144,
                    "placement_patch_count offset must be 144 for targeted upload");
                queue.WriteBuffer(configBuffer_, 144, &config_.placement_patch_count, sizeof(uint32_t));
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

            void upload_ribbon_time(wgpu::Queue& queue, float time) {
                // Only update the time field (offset 12 = after anchor[3])
                queue.WriteBuffer(ribbonBuffer_, offsetof(GPURibbonState, time), &time, sizeof(float));
            }

            void upload_ribbon(wgpu::Queue& queue, const GPURibbonState& ribbon) {
                queue.WriteBuffer(ribbonBuffer_, 0, &ribbon, sizeof(GPURibbonState));
            }

            void upload_sphere(wgpu::Queue& queue, const GPUSphereState& sphere) {
                queue.WriteBuffer(sphereBuffer_, 0, &sphere, sizeof(GPUSphereState));
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
            void reset_pawn(wgpu::Queue& queue) {
                GPUPawnState pawn{};
                pawn.pos[0] = Idle::PAWN_POS_X;
                pawn.pos[1] = Idle::PAWN_POS_Y;
                pawn.pos[2] = Idle::PAWN_POS_Z;
                pawn.heading = Idle::PAWN_HEADING;
                pawn.orientation[0] = 0.0f;
                pawn.orientation[1] = 0.0f;
                pawn.orientation[2] = 0.0f;
                pawn.orientation[3] = 1.0f;
                pawn.portal_trigger = -1;
                queue.WriteBuffer(pawnBuffer_, 0, &pawn, sizeof(pawn));
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
            // (compute_texture_group/layout removed — 0D compute uses Group 0 only)
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
            // (legacy cell mesh accessors removed — bindings 43-45 reserved)
            static constexpr uint32_t pawn_vertex_count() { return Dim::PAWN_VERTEX_COUNT; }
            wgpu::Buffer sphere_vertex_buffer() const { return sphereVertexBuffer_; }
            wgpu::Buffer sphere_index_buffer() const { return sphereIndexBuffer_; }
            uint32_t sphere_index_count() const { return sphereIndexCount_; }
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
            wgpu::Buffer palm_ground_buffer() const { return palmGroundBuffer_; }
            uint32_t palm_index_count() const { return palmIndexCount_; }
            void set_palm_index_count(uint32_t count) { palmIndexCount_ = count; }

            void upload_palm_mesh_params_slot(wgpu::Queue& queue, uint32_t slot, const GPUPalmMeshParams& params) {
                queue.WriteBuffer(palmMeshParamsBuffer_,
                    slot * sizeof(GPUPalmMeshParams),
                    &params, sizeof(GPUPalmMeshParams));
            }

            wgpu::BindGroupLayout palm_mesh_gen_layout() const { return palmMeshGenLayout_; }
            wgpu::BindGroup palm_mesh_gen_group() const { return palmMeshGenBindGroup_; }

            void upload_palm_origins(wgpu::Queue& queue, const GPUPalmGroundEntry* entries, uint32_t count) {
                queue.WriteBuffer(palmGroundBuffer_, 0, entries,
                    sizeof(GPUPalmGroundEntry) * std::min(count, Dim::MAX_PALM_INSTANCES));
            }

            // --- Cactus accessors and upload ---
            wgpu::Buffer cactus_vertex_buffer() const { return cactusVertexBuffer_; }
            wgpu::Buffer cactus_index_buffer() const { return cactusIndexBuffer_; }
            wgpu::Buffer cactus_ground_buffer() const { return cactusGroundBuffer_; }
            uint32_t cactus_index_count() const { return cactusIndexCount_; }
            void set_cactus_index_count(uint32_t count) { cactusIndexCount_ = count; }
            void upload_cactus_mesh_params_slot(wgpu::Queue& queue, uint32_t slot, const GPUCactusMeshParams& params) {
                queue.WriteBuffer(cactusMeshParamsBuffer_,
                    slot * sizeof(GPUCactusMeshParams),
                    &params, sizeof(GPUCactusMeshParams));
            }
            wgpu::BindGroupLayout cactus_mesh_gen_layout() const { return cactusMeshGenLayout_; }
            wgpu::BindGroup cactus_mesh_gen_group() const { return cactusMeshGenBindGroup_; }
            void upload_cactus_origins(wgpu::Queue& queue, const GPUCactusGroundEntry* entries, uint32_t count) {
                queue.WriteBuffer(cactusGroundBuffer_, 0, entries,
                    sizeof(GPUCactusGroundEntry) * std::min(count, Dim::MAX_CACTUS_INSTANCES));
            }

            // --- Blade Cluster accessors and upload ---
            wgpu::Buffer blade_vertex_buffer() const { return bladeVertexBuffer_; }
            wgpu::Buffer blade_index_buffer() const { return bladeIndexBuffer_; }
            wgpu::Buffer blade_ground_buffer() const { return bladeGroundBuffer_; }
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
            void upload_blade_origins(wgpu::Queue& queue,
                const GPUBladeClusterGroundEntry* entries, uint32_t count) {
                queue.WriteBuffer(bladeGroundBuffer_, 0, entries,
                    sizeof(GPUBladeClusterGroundEntry) * std::min(count, Dim::MAX_BLADE_INSTANCES));
            }

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
            wgpu::Buffer sphere_buffer() const { return sphereBuffer_; }
            wgpu::Buffer pawn_buffer() const { return pawnBuffer_; }
            wgpu::Buffer pawn_readback_staging() const { return pawnReadbackStaging_; }
            static constexpr size_t pawn_state_size() { return sizeof(GPUPawnState); }

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
                pawnBuffer_ = makeBuffer("Pawn State", sizeof(GPUPawnState),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc);
                cameraBuffer_ = makeBuffer("Camera State", sizeof(GPUCameraState), SU);
                sphereBuffer_ = makeBuffer("Sphere State", sizeof(GPUSphereState), SU | wgpu::BufferUsage::Uniform);
                ribbonBuffer_ = makeBuffer("Ribbon State", sizeof(GPURibbonState), SU | wgpu::BufferUsage::Uniform);
                ringTransformsBuffer_ = makeBuffer("Ring Transforms",
                    sizeof(GPURibbonRingTransform) * Dim::RIBBON_MAX_RINGS,
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
                    sd.label = "Pawn Readback Staging";
                    sd.size = sizeof(GPUPawnState);  // full pawn state: position + portal trigger
                    sd.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
                    pawnReadbackStaging_ = device_.CreateBuffer(&sd);
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

                return signalBuffer_ && configBuffer_ && terrainBuffer_ && pawnBuffer_ &&
                    cameraBuffer_ && sphereBuffer_ && trajectoriesBuffer_ && ringTransformsBuffer_ &&
                    vpBuffer_ && spotLightArrayBuffer_ && spotVPStagingBuffer_ && directionalLightBuffer_ && pointLightsBuffer_ && patchParamsBuffer_ &&
                    patchStagingBuffer_ && tileGridBuffer_ && pierBuffer_ && patchInstancesBuffer_ &&
                    patchHeightScratchBuffer_ &&
                    photographerVPBuffer_ && photographerCameraBuffer_ &&
                    photographerConfigBuffer_ && paintingSlotsBuffer_ &&
                    portalArrayBuffer_ && pawnReadbackStaging_;
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

                return createSphereMesh() && createArchMesh() && createColumnMesh() && createPalmMesh() && createCactusMesh() && createBladeMesh() && createPyramidMesh() && createShellMesh() && createGoLZoneBuffers();
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
                palmGroundBuffer_ = makeBuffer("Palm Ground Y",
                    Dim::MAX_PALM_INSTANCES * sizeof(GPUPalmGroundEntry),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst);
                palmMeshParamsBuffer_ = makeBuffer("Palm Mesh Params",
                    Dim::MAX_PALM_INSTANCES * sizeof(GPUPalmMeshParams),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);

                if (!palmVertexBuffer_ || !palmIndexBuffer_ || !palmGroundBuffer_ ||
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
                cactusGroundBuffer_ = makeBuffer("Cactus Ground Y",
                    Dim::MAX_CACTUS_INSTANCES * sizeof(GPUCactusGroundEntry),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst);
                cactusMeshParamsBuffer_ = makeBuffer("Cactus Mesh Params",
                    Dim::MAX_CACTUS_INSTANCES * sizeof(GPUCactusMeshParams),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                if (!cactusVertexBuffer_ || !cactusIndexBuffer_ || !cactusGroundBuffer_ ||
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
                bladeGroundBuffer_ = makeBuffer("Blade Ground Y",
                    Dim::MAX_BLADE_INSTANCES * sizeof(GPUBladeClusterGroundEntry),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst);
                bladeMeshParamsBuffer_ = makeBuffer("Blade Mesh Params",
                    Dim::MAX_BLADE_INSTANCES * sizeof(GPUBladeClusterMeshParams),
                    wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst);
                if (!bladeVertexBuffer_ || !bladeIndexBuffer_ || !bladeGroundBuffer_ ||
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
                // Pawn:     60-79    (pawn_state)
                // Camera:   80-99    (camera_state)
                // Sphere:  100-119   (sphere_state, trajectories)
                //
                {
                    std::array<wgpu::BindGroupLayoutEntry, 15> entries{};

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

                    entries[8].binding = 120;  // uniform — avoids 8 storage buffer limit
                    entries[8].visibility = wgpu::ShaderStage::Compute;
                    entries[8].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[9].binding = 25;
                    entries[9].visibility = wgpu::ShaderStage::Compute;
                    entries[9].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[10].binding = 26;   // pier_instances (storage, read)
                    entries[10].visibility = wgpu::ShaderStage::Compute;
                    entries[10].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    entries[11].binding = 30;   // pyramid_instances (uniform — used by effective_ground_y)
                    entries[11].visibility = wgpu::ShaderStage::Compute;
                    entries[11].buffer.type = wgpu::BufferBindingType::Uniform;

                    // GoL zone state — used by effective_ground_y for cell height contribution
                    entries[12].binding = 160;  // zone_config (storage — matches var<storage, read_write>)
                    entries[12].visibility = wgpu::ShaderStage::Compute;
                    entries[12].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[13].binding = 161;  // zone_life (storage, rw — matches WGSL var declaration)
                    entries[13].visibility = wgpu::ShaderStage::Compute;
                    entries[13].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[14].binding = 62;   // portal_array (uniform — proximity check in update_pawn)
                    entries[14].visibility = wgpu::ShaderStage::Compute;
                    entries[14].buffer.type = wgpu::BufferBindingType::Uniform;

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
                // Pawn:     260-279  (render_pawn)
                // Camera:   280-299  (render_camera)
                // Sphere:   300-319  (render_sphere)
                // Light:    320-339  (render_light, render_point_lights)
                //
                // Vertex shaders need entity state for positioning + VP for transform.
                // Fragment shaders need camera for fog distance.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 20> entries{};

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
                    entries[5].visibility = wgpu::ShaderStage::Fragment;
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

                    // Arch ground Y corrections (GPU-compute-corrected, VS reads)
                    entries[13].binding = 380;
                    entries[13].visibility = wgpu::ShaderStage::Vertex;
                    entries[13].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    // Column ground Y corrections (GPU-compute-corrected, VS reads)
                    entries[14].binding = 381;
                    entries[14].visibility = wgpu::ShaderStage::Vertex;
                    entries[14].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    // Pyramid ground Y corrections (GPU-compute-corrected, VS reads)
                    entries[15].binding = 382;
                    entries[15].visibility = wgpu::ShaderStage::Vertex;
                    entries[15].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    // Tile grid (uniform — terrain wave delta needs amp_scale in VS,
                    // animated_cell_color needs archetype lookup in FS)
                    entries[16].binding = 25;
                    entries[16].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
                    entries[16].buffer.type = wgpu::BufferBindingType::Uniform;

                    // Palm ground Y (VS reads)
                    entries[17].binding = 383;
                    entries[17].visibility = wgpu::ShaderStage::Vertex;
                    entries[17].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[18].binding = 384;
                    entries[18].visibility = wgpu::ShaderStage::Vertex;
                    entries[18].buffer.type = wgpu::BufferBindingType::Uniform;

                    entries[19].binding = 385;
                    entries[19].visibility = wgpu::ShaderStage::Vertex;
                    entries[19].buffer.type = wgpu::BufferBindingType::Uniform;

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

                // -- Render texture layout (Group 1) -- bindings 22-23, 25-27, 28-29, 31-33 -
                // Used during main render pass: samplers + shadow maps + patches + GoL zones + pawn aura.
                // (bindings 20, 21, 24 removed — formerly legacy stub textures)
                {
                    std::array<wgpu::BindGroupLayoutEntry, 10> entries{};

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

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Render Texture Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    renderTextureBindGroupLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!renderTextureBindGroupLayout_) return false;
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
                    std::array<wgpu::BindGroupLayoutEntry, 8> entries{};

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
                // Reuses existing WGSL binding numbers: ribbon_state @120,
                // tile_grid @25, solid_instances @26. New: ring_xforms @121.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 4> entries{};

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

                // -- Photographer compute layout (Group 0) -- VP + terrain coupling --
                // Reads GPU pawn position + config → builds VP, clamps camera above terrain,
                // corrects entity + painting Y positions from baked heightfield + GoL zones.
                // 9 storage buffers + 2 uniform + 1 texture + 1 sampler = 13 entries.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 13> entries{};

                    // pawn_state: read actual GPU pawn position
                    entries[0].binding = 60;
                    entries[0].visibility = wgpu::ShaderStage::Compute;
                    entries[0].buffer.type = wgpu::BufferBindingType::Storage;

                    // photographer_config: camera parameters (CPU-uploaded)
                    entries[1].binding = 140;
                    entries[1].visibility = wgpu::ShaderStage::Compute;
                    entries[1].buffer.type = wgpu::BufferBindingType::Uniform;

                    // photographer_vp: output VP matrix
                    entries[2].binding = 141;
                    entries[2].visibility = wgpu::ShaderStage::Compute;
                    entries[2].buffer.type = wgpu::BufferBindingType::Storage;

                    // photographer_camera_out: output camera position (for fog)
                    entries[3].binding = 142;
                    entries[3].visibility = wgpu::ShaderStage::Compute;
                    entries[3].buffer.type = wgpu::BufferBindingType::Storage;

                    // painting_slots: read/write for Y correction
                    entries[4].binding = 143;
                    entries[4].visibility = wgpu::ShaderStage::Compute;
                    entries[4].buffer.type = wgpu::BufferBindingType::Storage;

                    // patch_instances: which patches are active (for terrain lookup)
                    entries[5].binding = 144;
                    entries[5].visibility = wgpu::ShaderStage::Compute;
                    entries[5].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

                    // patch_heightfield: terrain height texture array
                    entries[6].binding = 145;
                    entries[6].visibility = wgpu::ShaderStage::Compute;
                    entries[6].texture.sampleType = wgpu::TextureSampleType::Float;
                    entries[6].texture.viewDimension = wgpu::TextureViewDimension::e2DArray;

                    // bilinear_sampler: for terrain interpolation
                    entries[7].binding = 146;
                    entries[7].visibility = wgpu::ShaderStage::Compute;
                    entries[7].sampler.type = wgpu::SamplerBindingType::Filtering;

                    // arch_ground: per-arch Y correction (read/write)
                    entries[8].binding = 147;
                    entries[8].visibility = wgpu::ShaderStage::Compute;
                    entries[8].buffer.type = wgpu::BufferBindingType::Storage;

                    // column_ground: per-column Y correction (read/write)
                    entries[9].binding = 148;
                    entries[9].visibility = wgpu::ShaderStage::Compute;
                    entries[9].buffer.type = wgpu::BufferBindingType::Storage;

                    // pyramid_ground: per-pyramid Y correction (read/write)
                    entries[10].binding = 149;
                    entries[10].visibility = wgpu::ShaderStage::Compute;
                    entries[10].buffer.type = wgpu::BufferBindingType::Storage;

                    // GoL zone config (storage — must match var<storage, read_write> in WGSL)
                    entries[11].binding = 160;
                    entries[11].visibility = wgpu::ShaderStage::Compute;
                    entries[11].buffer.type = wgpu::BufferBindingType::Storage;

                    // GoL zone life (storage — painting Y reads per-cell height)
                    entries[12].binding = 161;
                    entries[12].visibility = wgpu::ShaderStage::Compute;
                    entries[12].buffer.type = wgpu::BufferBindingType::Storage;

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
                // 13 entries: config + pawn + painting slots + heightfield + entity grounds + GoL.
                {
                    std::array<wgpu::BindGroupLayoutEntry, 13> entries{};

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

                    entries[11].binding = 150;
                    entries[11].visibility = wgpu::ShaderStage::Compute;
                    entries[11].buffer.type = wgpu::BufferBindingType::Storage;

                    entries[12].binding = 151;
                    entries[12].visibility = wgpu::ShaderStage::Compute;
                    entries[12].buffer.type = wgpu::BufferBindingType::Storage;

                    wgpu::BindGroupLayoutDescriptor desc{};
                    desc.label = "Entity Placement Compute Layout";
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    entityPlacementComputeLayout_ = device_.CreateBindGroupLayout(&desc);
                    if (!entityPlacementComputeLayout_) return false;
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

                    entries[1].binding = 60;   // pawn_state (read position)
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

                // Compute entity bind group (15 entries: systems + terrain + GoL zones + portals)
                {
                    std::array<wgpu::BindGroupEntry, 15> entries{};

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

                    entries[4].binding = 60;
                    entries[4].buffer = pawnBuffer_;
                    entries[4].size = sizeof(GPUPawnState);

                    entries[5].binding = 80;
                    entries[5].buffer = cameraBuffer_;
                    entries[5].size = sizeof(GPUCameraState);

                    entries[6].binding = 100;
                    entries[6].buffer = sphereBuffer_;
                    entries[6].size = sizeof(GPUSphereState);

                    entries[7].binding = 101;
                    entries[7].buffer = trajectoriesBuffer_;
                    entries[7].size = sizeof(GPUTrajectory) * Dim::MAX_TRAJECTORIES;

                    entries[8].binding = 120;
                    entries[8].buffer = ribbonBuffer_;
                    entries[8].size = sizeof(GPURibbonState);

                    entries[9].binding = 25;
                    entries[9].buffer = tileGridBuffer_;
                    entries[9].size = sizeof(GPUTileGrid);

                    // Pier Instances — unified terrain-raising volumes
                    entries[10].binding = 26;
                    entries[10].buffer = pierBuffer_;
                    entries[10].size = Dim::PIER_TOTAL * sizeof(GPUPierInstance);

                    // Pyramid Instances — tapered height in effective_ground_y
                    entries[11].binding = 30;
                    entries[11].buffer = pyramidInstancesBuffer_;
                    entries[11].size = sizeof(GPUPyramidArray);

                    // GoL zone state — cell height in effective_ground_y
                    entries[12].binding = 160;
                    entries[12].buffer = zoneConfigBuffer_;
                    entries[12].size = sizeof(GPUGoLZoneArray);

                    entries[13].binding = 161;
                    entries[13].buffer = zoneLifeBuffer_;
                    entries[13].size = Dim::MAX_GOL_ZONES * Dim::GOL_ZONE_LIFE_STRIDE * sizeof(float);

                    entries[14].binding = 62;
                    entries[14].buffer = portalArrayBuffer_;
                    entries[14].size = sizeof(GPUPortalArray);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Compute Entity BindGroup";
                    desc.layout = computeEntityBindGroupLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    computeEntityBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!computeEntityBindGroup_) return false;
                }

                // Render entity bind group (20 entries: config + spaced by system +200)
                {
                    std::array<wgpu::BindGroupEntry, 20> entries{};

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
                    entries[4].buffer = pawnBuffer_;
                    entries[4].size = sizeof(GPUPawnState);

                    entries[5].binding = 280;
                    entries[5].buffer = cameraBuffer_;
                    entries[5].size = sizeof(GPUCameraState);

                    entries[6].binding = 300;
                    entries[6].buffer = sphereBuffer_;
                    entries[6].size = sizeof(GPUSphereState);

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

                    entries[13].binding = 380;
                    entries[13].buffer = archGroundBuffer_;
                    entries[13].size = sizeof(GPUArchGroundEntry) * Dim::MAX_ARCH_INSTANCES;

                    entries[14].binding = 381;
                    entries[14].buffer = columnGroundBuffer_;
                    entries[14].size = sizeof(GPUColumnGroundEntry) * Dim::MAX_COLUMN_INSTANCES;

                    entries[15].binding = 382;
                    entries[15].buffer = pyramidGroundBuffer_;
                    entries[15].size = sizeof(GPUPyramidGroundEntry) * Dim::MAX_PYRAMID_INSTANCES;

                    entries[16].binding = 25;
                    entries[16].buffer = tileGridBuffer_;
                    entries[16].size = sizeof(GPUTileGrid);

                    entries[17].binding = 383;
                    entries[17].buffer = palmGroundBuffer_;
                    entries[17].size = sizeof(GPUPalmGroundEntry) * Dim::MAX_PALM_INSTANCES;

                    entries[18].binding = 384;
                    entries[18].buffer = cactusGroundBuffer_;
                    entries[18].size = sizeof(GPUCactusGroundEntry) * Dim::MAX_CACTUS_INSTANCES;

                    entries[19].binding = 385;
                    entries[19].buffer = bladeGroundBuffer_;
                    entries[19].size = sizeof(GPUBladeClusterGroundEntry) * Dim::MAX_BLADE_INSTANCES;

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

                // Render texture bind group (10 entries: 22-23, 25-27, 28-29, 31-33)
                {
                    std::array<wgpu::BindGroupEntry, 10> entries{};

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

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Render Texture BindGroup";
                    desc.layout = renderTextureBindGroupLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    renderTextureBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!renderTextureBindGroup_) return false;
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

                // Patch gen bind group (8 entries: binding 1, 23, 24, 25, 26, 27, 28, 30)
                {
                    std::array<wgpu::BindGroupEntry, 8> entries{};

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

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Patch Gen BindGroup";
                    desc.layout = patchGenLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    patchGenBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!patchGenBindGroup_) return false;
                }

                // Ribbon compute bind group (4 entries: dedicated ring transform pass)
                {
                    std::array<wgpu::BindGroupEntry, 4> entries{};

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
                    std::array<wgpu::BindGroupEntry, 20> entries{};
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
                    entries[4].buffer = pawnBuffer_;
                    entries[4].size = sizeof(GPUPawnState);
                    entries[5].binding = 280;
                    entries[5].buffer = photographerCameraBuffer_;  // ← photographer pos for fog
                    entries[5].size = sizeof(GPUCameraState);
                    entries[6].binding = 300;
                    entries[6].buffer = sphereBuffer_;
                    entries[6].size = sizeof(GPUSphereState);
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
                    entries[13].binding = 380;
                    entries[13].buffer = archGroundBuffer_;
                    entries[13].size = sizeof(GPUArchGroundEntry) * Dim::MAX_ARCH_INSTANCES;
                    entries[14].binding = 381;
                    entries[14].buffer = columnGroundBuffer_;
                    entries[14].size = sizeof(GPUColumnGroundEntry) * Dim::MAX_COLUMN_INSTANCES;
                    entries[15].binding = 382;
                    entries[15].buffer = pyramidGroundBuffer_;
                    entries[15].size = sizeof(GPUPyramidGroundEntry) * Dim::MAX_PYRAMID_INSTANCES;
                    entries[16].binding = 25;
                    entries[16].buffer = tileGridBuffer_;
                    entries[16].size = sizeof(GPUTileGrid);
                    entries[17].binding = 383;
                    entries[17].buffer = palmGroundBuffer_;
                    entries[17].size = sizeof(GPUPalmGroundEntry) * Dim::MAX_PALM_INSTANCES;
                    entries[18].binding = 384;
                    entries[18].buffer = cactusGroundBuffer_;
                    entries[18].size = sizeof(GPUCactusGroundEntry) * Dim::MAX_CACTUS_INSTANCES;

                    entries[19].binding = 385;
                    entries[19].buffer = bladeGroundBuffer_;
                    entries[19].size = sizeof(GPUBladeClusterGroundEntry) * Dim::MAX_BLADE_INSTANCES;

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Photographer Render Entity BindGroup";
                    desc.layout = renderEntityBindGroupLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    photographerRenderEntityBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!photographerRenderEntityBindGroup_) return false;
                }

                // Photographer compute bind group (13 entries: systems + terrain + GoL zones)
                {
                    std::array<wgpu::BindGroupEntry, 13> entries{};
                    entries[0].binding = 60;
                    entries[0].buffer = pawnBuffer_;
                    entries[0].size = sizeof(GPUPawnState);
                    entries[1].binding = 140;
                    entries[1].buffer = photographerConfigBuffer_;
                    entries[1].size = sizeof(GPUPhotographerConfig);
                    entries[2].binding = 141;
                    entries[2].buffer = photographerVPBuffer_;
                    entries[2].size = sizeof(GPUVPMatrix);
                    entries[3].binding = 142;
                    entries[3].buffer = photographerCameraBuffer_;
                    entries[3].size = sizeof(GPUCameraState);
                    entries[4].binding = 143;
                    entries[4].buffer = paintingSlotsBuffer_;
                    entries[4].size = sizeof(GPUPaintingSlot) * Dim::PAINTING_MAX_SLOTS;
                    entries[5].binding = 144;
                    entries[5].buffer = patchInstancesBuffer_;
                    entries[5].size = sizeof(GPUPatchInstance) * Dim::MAX_ACTIVE_PATCHES;
                    entries[6].binding = 145;
                    entries[6].textureView = patchHeightfieldArrayReadView_;
                    entries[7].binding = 146;
                    entries[7].sampler = bilinearSampler_;
                    entries[8].binding = 147;
                    entries[8].buffer = archGroundBuffer_;
                    entries[8].size = sizeof(GPUArchGroundEntry) * Dim::MAX_ARCH_INSTANCES;
                    entries[9].binding = 148;
                    entries[9].buffer = columnGroundBuffer_;
                    entries[9].size = sizeof(GPUColumnGroundEntry) * Dim::MAX_COLUMN_INSTANCES;
                    entries[10].binding = 149;
                    entries[10].buffer = pyramidGroundBuffer_;
                    entries[10].size = sizeof(GPUPyramidGroundEntry) * Dim::MAX_PYRAMID_INSTANCES;
                    entries[11].binding = 160;
                    entries[11].buffer = zoneConfigBuffer_;
                    entries[11].size = sizeof(GPUGoLZoneArray);
                    entries[12].binding = 161;
                    entries[12].buffer = zoneLifeBuffer_;
                    entries[12].size = Dim::MAX_GOL_ZONES * Dim::GOL_ZONE_LIFE_STRIDE * sizeof(float);

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Photographer Compute BindGroup";
                    desc.layout = photographerComputeLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    photographerComputeBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!photographerComputeBindGroup_) return false;
                }

                // Entity placement compute bind group (heightfield sampling + pier correction)
                {
                    std::array<wgpu::BindGroupEntry, 13> entries{};
                    entries[0].binding = 1;
                    entries[0].buffer = configBuffer_;
                    entries[0].size = sizeof(GPUDesignConfig);
                    entries[1].binding = 60;
                    entries[1].buffer = pawnBuffer_;
                    entries[1].size = sizeof(GPUPawnState);
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

                    entries[11].binding = 150;
                    entries[11].buffer = palmGroundBuffer_;
                    entries[11].size = sizeof(GPUPalmGroundEntry) * Dim::MAX_PALM_INSTANCES;

                    entries[12].binding = 151;
                    entries[12].buffer = cactusGroundBuffer_;
                    entries[12].size = sizeof(GPUCactusGroundEntry) * Dim::MAX_CACTUS_INSTANCES;

                    wgpu::BindGroupDescriptor desc{};
                    desc.label = "Entity Placement Compute BindGroup";
                    desc.layout = entityPlacementComputeLayout_;
                    desc.entryCount = entries.size();
                    desc.entries = entries.data();
                    entityPlacementComputeBindGroup_ = device_.CreateBindGroup(&desc);
                    if (!entityPlacementComputeBindGroup_) return false;
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
                    entries[1].buffer = pawnBuffer_;
                    entries[1].size = sizeof(GPUPawnState);

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

                GPUPawnState pawn{};
                pawn.pos[0] = Idle::PAWN_POS_X;
                pawn.pos[1] = Idle::PAWN_POS_Y;
                pawn.pos[2] = Idle::PAWN_POS_Z;
                pawn.heading = Idle::PAWN_HEADING;
                pawn.orientation[0] = 0.0f;
                pawn.orientation[1] = 0.0f;
                pawn.orientation[2] = 0.0f;
                pawn.orientation[3] = 1.0f;
                pawn.portal_trigger = -1;
                queue.WriteBuffer(pawnBuffer_, 0, &pawn, sizeof(pawn));

                GPUCameraState camera{};
                camera.pos[0] = Idle::CAMERA_POS_X;
                camera.pos[1] = Idle::CAMERA_POS_Y;
                camera.pos[2] = Idle::CAMERA_POS_Z;
                camera.azimuth = Idle::CAMERA_AZIMUTH;
                camera.elevation = Idle::CAMERA_ELEVATION;
                camera.distance = Idle::CAMERA_DISTANCE;
                camera.pan_x = 0.0f;
                camera.pan_y = 0.0f;
                queue.WriteBuffer(cameraBuffer_, 0, &camera, sizeof(camera));

                GPUSphereState sphere{};
                sphere.pos[0] = Idle::SPHERE_ORBIT_RADIUS;
                sphere.pos[1] = Idle::SPHERE_HOVER_HEIGHT;
                sphere.pos[2] = 0.0f;
                sphere.radius = Idle::SPHERE_RADIUS;
                sphere.orientation[0] = 0.0f;
                sphere.orientation[1] = 0.0f;
                sphere.orientation[2] = 0.0f;
                sphere.orientation[3] = 1.0f;
                sphere.influence_radius = Idle::SPHERE_INFLUENCE_RADIUS;
                sphere.t = 0.0f;
                sphere.orbit_radius = Idle::SPHERE_ORBIT_RADIUS;
                sphere.orbit_speed = Idle::SPHERE_ORBIT_SPEED;
                sphere.color[0] = 0.95f;
                sphere.color[1] = 0.75f;
                sphere.color[2] = 0.4f;
                sphere.orbit_height = Idle::SPHERE_HOVER_HEIGHT;
                sphere.anchor[0] = 0.0f;
                sphere.anchor[1] = 0.0f;
                sphere.anchor[2] = 0.0f;
                sphere.color_mode = 0.0f;
                sphere.base_color[0] = 0.95f;
                sphere.base_color[1] = 0.75f;
                sphere.base_color[2] = 0.4f;
                sphere._pad0 = 0.0f;
                queue.WriteBuffer(sphereBuffer_, 0, &sphere, sizeof(sphere));

                // Default state — overwritten by spawner
                GPURibbonState ribbon{};
                ribbon.anchor[0] = 0.0f;
                ribbon.anchor[1] = 0.0f;
                ribbon.anchor[2] = 0.0f;
                ribbon.time = 0.0f;
                ribbon.cube_count = 200;
                ribbon.cube_size = 3.0f;
                ribbon.height = 40.0f;
                ribbon.twist_amp = 1.0f;
                ribbon.color[0] = 0.85f;
                ribbon.color[1] = 0.12f;
                ribbon.color[2] = 0.08f;
                ribbon.lateral_amp = 2.7f;
                ribbon.lateral_cycles = 2.0f;
                ribbon.lateral_speed = 1.1f;
                ribbon.vertical_amp = 1.7f;
                ribbon.vertical_cycles = 1.0f;
                ribbon.vertical_speed = 1.2f;
                ribbon.twist_cycles = 2.5f;
                ribbon.twist_speed = 1.0f;
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

    } // namespace the_board
} // namespace t7