// ─── world.wgsl ──────────────────────────────────────────────────────
//
// THE_BOARD CARTRIDGE — GPU spine.
//
// Counterpart to cartridge.hpp. The CPU spine authors intent (mood
// transitions, entity lifecycle, dispatch tables, signal preparation);
// this file is what the GPU runs every frame. All struct shapes in
// §2.1 must mirror their C++ counterparts in state.hpp byte-for-byte;
// all per-policy contributor masks in §3.4 must mirror POLICIES[]
// in contracts/ground_architecture.hpp; the deterministic-randomness
// helpers in §1.5 must produce bit-identical results to the CPU
// mirrors in primitives/seed_utils.hpp.
//
// Navigation: §1-§9 chapter-numbered structure. Section numbers
// reflect file order — search by section number to jump.
// TUNING SURFACE DIRECTORY (below) lists the constants that shape
// terrain, color, and entity behavior. SECTION MAP gives the
// chapter outline.
//
// SEAM[world.wgsl:owns] this file is the canonical source of truth
//   for everything the GPU does — terrain evaluation, entity
//   compute, render pipelines, mesh generation. CPU code (cartridge
//   spine + modules) authors intent; this file realizes geometry
//   and pixels. The motto: GPU is sovereign. CPU dead-reckoning
//   exists only for placement, picking, and step decisions; the
//   visual reality is here.
// SEAM[world.wgsl:contract] CPU/GPU struct contracts. §2.1 structs
//   (FrameSignal, AgentState, AgentBehaviorParams,
//   AgentTierParams) mirror state.hpp byte-for-byte. The
//   PAIRED DECLARATIONS comment at §2.1 AGENT REGISTRIES
//   names the C++ counterparts. Drift would mean the GPU reads
//   different fields than CPU writes. Same family as
//   seed_utils:contract and ground_architecture:contract.
// SEAM[world.wgsl:ground-architecture-mirror] the §3.4 Ground
//   Architecture block (§3.4) is the GPU companion to
//   contracts/ground_architecture.hpp. The CPU file declares
//   ContributorId / PolicyId / CONTRIBUTOR_DAG / POLICIES[] +
//   compile-time DAG closure validation; this file declares the
//   contrib_*_at functions, the per-policy POLICY_*_MASK constants
//   (which must match POLICIES[].contributors bitmasks exactly),
//   and the query_ground_<policy> dispatch functions. Adding a
//   contributor or policy: see "Extension patterns" subsection
//   below for the exact step list across both files.
// SEAM[world.wgsl:fxc-constraints] the Windows D3D12/FXC backend
//   imposes hard limits, honored by structure in this file:
//     - instance structs in hot loops stay lean and byte-pinned
//       (GPUPierInstance: 48 B, static_assert in state.hpp — the
//       successor of the retired 32-byte SolidInstance rule)
//     - the collision/ground chain admits no new runtime branching;
//       evaluate_pier's caller bounds its loop by a uniform
//       (config.pier_count) and dispatch is by uniform function
//       choice, not branches
//     - texture-array stamps in the collision chain hang FXC
//     - one DrawIndexedIndirect per render pass maximum
//     - storage buffers / stage = 10; uniform buffers / stage = 12
// ─────────────────────────────────────────────────────────────────────

// THE_BOARD CARTRIDGE — GPU Scroll
//
// ═══════════════════════════════════════════════════════════════════════
// TUNING SURFACE DIRECTORY — GPU-side compositional control
// ═══════════════════════════════════════════════════════════════════════
//
// All constants that shape terrain, color, and cell behavior.
// Change a number, recompile, see the result. No logic edits needed.
// Section references (§N.M) are stable; search by section number.
//
// The surface voice's color + movement rows are consolidated at THE
// TERRAIN_LOOKS PANEL (§2.2, the WGSL room; C++ room = surface/
// terrain_looks.hpp — palette REST + motion/mode rest pins).
//
// ── Color Palettes (§2.2 — GRADUATED to the config uniform, 2b) ────
//   config.palette_center[4]      Sand/salmon/green/warm RGB medians
//   config.palette_light[4]       Light variant per palette
//   config.palette_weight         Selection probability (vec4)
//   (PALETTE_VARIANCE retired with its dead consumer palette_color)
//
// ── Spatial Field Lattices (§2.2) ─────────────────────────────────
//   PALETTE_LATTICE_SPACING       300 wu — palette blob size
//   MODE_LATTICE_SPACING          120 wu — smooth/discrete clusters
//   MODE_DISCRETE_THRESHOLD       0.70 — gate for checkerboard
//   MODE_BIAS_EXPONENT            5.0 — quintic: ~83% smooth
//   TRANSITION_LATTICE_SPACING    200 wu — blend/scatter zones
//   SPARSE_BASE_SPACING           160 wu — isolated cell regions
//   SPARSE_CLUSTER_SPACING        40 wu — small dense patches
//   CHESS_LATTICE_SPACING         55 wu — B&W alternation zones
//   DISCRETE_COLOR_LATTICE_SPACING  80 wu — colored cell blobs
//   DISCRETE_MONO_LATTICE_SPACING   250 wu — B&W tendency zones
//
// (Terrain-Mode Coupling section RETIRED — Phase 1, ruling 6.)
//
// ── Terrain Waves (→ §2.2 TERRAIN_LOOKS ROW 7) ────────────────────
//   WAVE_THRESHOLD[6]             Per-band activity gate
//   ACTIVITY_LATTICE_SPACING      400 wu — activity envelope
//
// ── GoL Zones (§2.2, §7.0b) ──────────────────────────────────────
//   GOL_TIERS[7]                  Tier params (density, tick, spring)
//   GOL_PULSE_TIERS[3]                Pulse algorithm params
//   GOL_ZONE_SPAWN_CHANCE         0.15 — fraction of discrete zones
//   GOL_ZONE_HEIGHT_CHANCE        0.30 — fraction with extrusion
//   GOL_COLOR_WEIGHTS             Color mode probabilities
//
// ── Pawn (§2.2) ──────────────────────────────────────────────────
//   PAWN_HEIGHT / PAWN_RADIUS     Physical dimensions
//   PAWN_STEP_HEIGHT              Max terrain step
//
// ── Radial Pulses (§3.5) ─────────────────────────────────────────
//   PULSE_SPEED / MAX_AGE / DAMPING  Ring dynamics
//
// For CPU-side tuning surfaces (moods, entity tiers, spawn chances,
// terrain tokens), see the companion directory in cartridge.hpp.
// ═══════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════
// SECTION MAP
// ═══════════════════════════════════════════════════════════════════════
//
// §1    FOUNDATIONS         PGA algebra, utilities, terrain height
// §2    STATE               Structs, constants, muting control
// §3    COUPLINGS           Signal/input/terrain/entity cross-wiring
// §4    DYNAMICS            PGA motor integration (pawn, camera)
// §5    COMPOSITION         0D update split across compute entry points
// §6    RENDERING           Lighting, terrain VS/FS, entity VS/FS, ribbon, shadows
// §7    COMPUTE             Bindings, entry points, GoL zones, pawn aura
// §8    GALLERY             Photographer, terrain paintings, wall paintings
// §9    ENTITY MESH GEN     GPU-sovereign geometry: arches, columns
//
// PANELS (the federation's strips — one sitting each; grep the names,
// no line anchors):
//   §2.2   TERRAIN_LOOKS — the master terrain panel; ROW 9 is the
//          roster of pointers to every other strip.
//   §7.0b  GOL ZONE panel — per-zone config struct + named visual
//          constants (enums pinned to their CPU twins).
//   PULSE_SPEED..PULSE_AGE_DECAY — the RADIAL PULSE panel, directly
//          above fn contrib_radial_pulses_at.
//   pawn_aura_cfg (§7.4) — the aura's uniform strip.
//   CPU rooms: terrain_looks.hpp (rest pins), population_themes.hpp
//          (population), state.hpp GPUDesignConfig (paneled by system
//          groups; GROWTH LAW at its head).
//
// Subsystem-specific bindings live with their consumers (§7, §8, §9).
// Global bindings (signal, config, VP, render mirrors, lights) are in §7.0.

// §1 FOUNDATIONS

// ── Pipeline specialization overrides (set at pipeline creation) ────
// Controls which VS path is used (direct patch access vs indirection through
// visible_patch_indices — the latter is needed for GPU frustum-culled draws).
override USE_PATCH_INDIRECTION: bool = false;  // true = read through visible_patch_indices

// §1.1 PROJECTIVE GEOMETRIC ALGEBRA

struct Motor {
    p0: vec4<f32>,  // [s, e23, e31, e12]     — scalar + rotation bivectors
    p1: vec4<f32>,  // [e01, e02, e03, e0123] — translation + pseudoscalar
}

struct Line {
    d: vec3<f32>,   // [e23, e31, e12] — direction/axis
    m: vec3<f32>,   // [e01, e02, e03] — moment
}

struct Plane {
    v: vec4<f32>,   // [e0, e1, e2, e3] = [d, nx, ny, nz]
}

struct Point {
    v: vec4<f32>,   // [e123, e032, e013, e021] = [w, x, y, z]
}

// --- PGA CONSTANTS

const PI: f32 = 3.14159265359;
const EPSILON: f32 = 1e-7;
const MOTOR_IDENTITY: Motor = Motor(vec4<f32>(1.0, 0.0, 0.0, 0.0), vec4<f32>(0.0));
const LINE_Y: Line = Line(vec3<f32>(0.0, 1.0, 0.0), vec3<f32>(0.0)); // Y-axis

// --- PGA CONSTRUCTORS

fn point_from_vec3(p: vec3<f32>) -> Point {
    return Point(vec4<f32>(1.0, p.x, p.y, p.z));
}

fn point_to_vec3(p: Point) -> vec3<f32> {
    let w = p.v.x;
    if (abs(w) < EPSILON) { return vec3<f32>(0.0); }
    return p.v.yzw / w;
}

fn rotor(axis: vec3<f32>, angle: f32) -> Motor {
    // Creates a Motor that rotates around the given axis by the given angle.
    // The axis passes through the origin.
    let len_sq = dot(axis, axis);
    if (len_sq < EPSILON) { return MOTOR_IDENTITY; }
    let half_angle = angle * 0.5;
    let n = axis * inverseSqrt(len_sq);
    let c = cos(half_angle);
    let s = sin(half_angle);
    // Negative s for counter-clockwise convention
    return Motor(vec4<f32>(c, -s * n.x, -s * n.y, -s * n.z), vec4<f32>(0.0));
}

fn translator(dir: vec3<f32>, dist: f32) -> Motor {
    // Creates a Motor that translates along the given direction by the given distance.
    let len_sq = dot(dir, dir);
    if (len_sq < EPSILON) { return MOTOR_IDENTITY; }
    let n = dir * inverseSqrt(len_sq);
    let half_dist = dist * (-0.5); // Negative for correct sandwich product
    return Motor(vec4<f32>(1.0, 0.0, 0.0, 0.0), vec4<f32>(half_dist * n, 0.0));
}

// --- PGA OPERATIONS

fn gp_mm(a: Motor, b: Motor) -> Motor {
    // Geometric Product (Motor * Motor)
    // This is how transformations compose in PGA.
    let p0 = vec4<f32>(
        a.p0.x * b.p0.x - dot(a.p0.yzw, b.p0.yzw),
        a.p0.x * b.p0.yzw + b.p0.x * a.p0.yzw + cross(a.p0.yzw, b.p0.yzw)
    );
    let p1 = vec4<f32>(
        a.p0.x * b.p1.xyz + b.p0.x * a.p1.xyz + cross(a.p0.yzw, b.p1.xyz) + cross(a.p1.xyz, b.p0.yzw) - b.p1.w * a.p0.yzw - a.p1.w * b.p0.yzw,
        a.p0.x * b.p1.w + b.p0.x * a.p1.w + dot(a.p0.yzw, b.p1.xyz) + dot(a.p1.xyz, b.p0.yzw)
    );
    return Motor(p0, p1);
}

fn sw_mp(m: Motor, p: vec3<f32>) -> vec3<f32> {
    // Sandwich Product (Motor * Point * ~Motor) — optimized for vec3 input
    let t = cross(p, m.p0.yzw) - m.p1.xyz;
    return (m.p0.x * t + cross(t, m.p0.yzw) - m.p0.yzw * m.p1.w) * 2.0 + p;
}

fn sw_motor_point(m: Motor, p: Point) -> Point {
    // Sandwich Product for Point type
    let pos = point_to_vec3(p);
    let transformed = sw_mp(m, pos);
    return point_from_vec3(transformed);
}



// §1.3 COORDINATE SYSTEMS

// Terrain mesh — grid subdivisions along each axis.
// Vertex shader receives deduplicated vertex ID via index buffer (GPU-generated once).
const TERRAIN_MESH_N: u32 = 256u;
const TERRAIN_MESH_STRIDE: u32 = TERRAIN_MESH_N + 1u;  // vertices per row (fence posts)

// Patch system — streaming terrain
const PATCH_HEIGHTFIELD_N: u32 = 256u;  // texels per patch heightfield side
const PATCH_CELL_N: u32 = 16u;          // cell color texture side per patch
const PATCH_EXTENT: f32 = 50.0;         // world units per patch side

// ── THE LIVE CARD (GROUND_CARD_1; C++ room: Dim::LIVE_CARD_*) ──
const LIVE_CARD_SIZE: u32 = 512u;
const LIVE_CARD_EXTENT: f32 = 800.0;
const LIVE_CARD_DEBUG_VIEW: u32 = 0u;  // 1 = paint the card (terrain FS eye — lands with H4 [4a])
fn live_card_origin() -> vec2<f32> {
    let cs = PATCH_EXTENT / f32(PATCH_CELL_N);           // 3.125
    let raw = vec2(config.lod_point_x, config.lod_point_z)
            - vec2(LIVE_CARD_EXTENT * 0.5);
    return floor(raw / cs) * cs;                          // cell snap
}
const PATCH_MESH_N: u32 = 64u;          // mesh subdivisions per patch (VS bilinear-samples 256-texel heightfield)
const PATCH_MESH_STRIDE: u32 = PATCH_MESH_N + 1u;

// THE ONE-ADDRESS LAW (SEAMLESSNESS corollary — charter C8). A cell
// has exactly ONE address: its world cell index. Every consumer —
// hash, roll, noise, color/tag texel, bake write — derives from it. A
// texel is COMPUTED FROM the address; patch_uv never addresses
// anything by itself again.
fn cell_address(world_xz: vec2<f32>) -> vec2<i32> {
    let cs = PATCH_EXTENT / f32(PATCH_CELL_N);
    return vec2<i32>(floor(world_xz / cs));
}

// ─── Patch skirts (weld #2, SKIRTS) ─────────────────────────────────
// Each patch skirts its FULL perimeter to hide inter-patch cracks
// (precision + LOD/T-junction) with one mechanism: duplicate the edge
// ring, drop the copies below the composited surface, quad-strip
// ring→copy. Skirt verts have vertex_index >= PATCH_GRID_VERT_COUNT; the
// index geometry is appended by the C++ patch-IB gen (state.hpp).
const PATCH_GRID_VERT_COUNT: u32 = PATCH_MESH_STRIDE * PATCH_MESH_STRIDE;  // 65*65 = 4225
const PATCH_SKIRT_RING: u32 = 4u * PATCH_MESH_N;                           // 256 perimeter verts
// Curtain depth (world units below the composited edge). For a heightfield
// the curtain only ever shows at the crack it fills or, in a finite world,
// the outer rim — so start generous; rig-tuned.
const PATCH_SKIRT_DEPTH: f32 = 8.0;

// ── THE UNIFIED GROUND (UNIFIED_GROUND_1; C++ room: Dim::UG_*) ──
// Bands stacked ABOVE the legacy grid+skirt space [0, UG_CAP_BASE):
// CAP [UG_CAP_BASE, UG_BASE_BASE) = 256 cells × 25 cell-owned verts
// (5×5 — cells lift independently); BASE [UG_BASE_BASE, …) = 256 × 16
// curtain-bottom twins (no lift — the gap IS the curtain).
const UG_QUADS: u32 = 4u;         // quads per cell edge (PATCH_MESH_N / PATCH_CELL_N)
const UG_CAP_STRIDE: u32 = 5u;    // cap verts per cell edge (UG_QUADS + 1)
const UG_CAP_BASE: u32 = 4481u;   // (65*65) + 4*64 — first cap vert
const UG_BASE_BASE: u32 = 10881u; // UG_CAP_BASE + 256*25 — first base vert

// Skirt ring index k in [0, PATCH_SKIRT_RING) -> its perimeter grid vertex
// (vx, vz), each in [0, PATCH_MESH_N]. CW walk: bottom, right, top, left.
// MIRROR of the C++ skirt_grid_index (state.hpp patch IB) — the two MUST
// agree so each skirt quad's top edge reads the right composited height.
fn patch_skirt_grid(k: u32) -> vec2<u32> {
    let n = PATCH_MESH_N;
    if (k < n)         { return vec2<u32>(k, 0u); }
    else if (k < 2u*n) { return vec2<u32>(n, k - n); }
    else if (k < 3u*n) { return vec2<u32>(n - (k - 2u*n), n); }
    else               { return vec2<u32>(0u, n - (k - 3u*n)); }
}

// The n=4 perimeter walk of a cell's 5×5 cap grid — same CW shape
// as patch_skirt_grid; mirrors the CPU emission's cell_perimeter.
fn ug_cell_perimeter(k: u32) -> vec2<u32> {
    let n = UG_QUADS;
    if (k < n)         { return vec2<u32>(k, 0u); }
    else if (k < 2u*n) { return vec2<u32>(n, k - n); }
    else if (k < 3u*n) { return vec2<u32>(n - (k - 2u*n), n); }
    else               { return vec2<u32>(0u, n - (k - 3u*n)); }
}

// THE UNIFIED DECODE (UNIFIED_GROUND_1; bands: Dim::UG_*)
// Comparisons only — the A2-5(i) blessed shape (no loops, no tables).
struct UgVert {
    vx: u32,
    vz: u32,
    cellx: u32,
    cellz: u32,
    lift_scale: f32,   // 1 cap/legacy/skirt, 0 base (no lift — the gap IS the curtain)
    drop: f32,         // PATCH_SKIRT_DEPTH on skirt ring copies
    wall: f32,         // 1 on curtain-bottom + skirt copies
}
fn ug_decode(vi: u32) -> UgVert {
    var d: UgVert;
    d.lift_scale = 1.0;
    d.drop = 0.0;
    d.wall = 0.0;
    if (vi < PATCH_GRID_VERT_COUNT) {
        // legacy grid (the LOD1/soft space)
        d.vx = vi % PATCH_MESH_STRIDE;
        d.vz = vi / PATCH_MESH_STRIDE;
    } else if (vi < UG_CAP_BASE) {
        // skirt ring copy — keeps its legacy slot, drops after compositing
        let g = patch_skirt_grid(vi - PATCH_GRID_VERT_COUNT);
        d.vx = g.x;
        d.vz = g.y;
        d.drop = PATCH_SKIRT_DEPTH;
        d.wall = 1.0;
    } else if (vi < UG_BASE_BASE) {
        // cap band: cell-owned 5×5 vert
        let r = vi - UG_CAP_BASE;
        let cell = r / 25u;          // UG_CAP_VERTS_PER_CELL
        let k = r % 25u;
        let lx = k % UG_CAP_STRIDE;
        let lz = k / UG_CAP_STRIDE;
        d.vx = (cell % PATCH_CELL_N) * UG_QUADS + lx;
        d.vz = (cell / PATCH_CELL_N) * UG_QUADS + lz;
    } else {
        // base band: curtain-bottom twin of a cap perimeter vert
        let r = vi - UG_BASE_BASE;
        let cell = r / 16u;          // UG_BASE_VERTS_PER_CELL
        let g = ug_cell_perimeter(r % 16u);
        d.vx = (cell % PATCH_CELL_N) * UG_QUADS + g.x;
        d.vz = (cell / PATCH_CELL_N) * UG_QUADS + g.y;
        d.lift_scale = 0.0;
        d.wall = 1.0;
    }
    // the uniform cell-assignment rule (legacy/skirt verts included)
    d.cellx = min(d.vx / UG_QUADS, PATCH_CELL_N - 1u);
    d.cellz = min(d.vz / UG_QUADS, PATCH_CELL_N - 1u);
    return d;
}

// The cell-lift fetch: the card's raw GoL (.a, nearest) at the CELL
// CENTER — every vert of a cell reads one value; cells lift as slabs.
fn ug_cell_lift(pi_origin: vec2<f32>, pi_extent: f32,
                cellx: u32, cellz: u32) -> f32 {
    let cs = pi_extent / f32(PATCH_CELL_N);
    let center = pi_origin - vec2(pi_extent * 0.5)
               + (vec2(f32(cellx), f32(cellz)) + vec2(0.5)) * cs;
    return sample_live_card_gol(center);   // nearest, cell-snapped
}


// §1.4 UTILITIES

fn quat_from_axis_angle(axis: vec3<f32>, angle: f32) -> vec4<f32> {
    let half_angle = angle * 0.5;
    return vec4(axis * sin(half_angle), cos(half_angle));
}

fn quat_multiply(a: vec4<f32>, b: vec4<f32>) -> vec4<f32> {
    return vec4(
        a.w * b.xyz + b.w * a.xyz + cross(a.xyz, b.xyz),
        a.w * b.w - dot(a.xyz, b.xyz)
    );
}

fn quat_rotate(q: vec4<f32>, v: vec3<f32>) -> vec3<f32> {
    let u = q.xyz;
    let s = q.w;
    return 2.0 * dot(u, v) * u + (s * s - dot(u, u)) * v + 2.0 * s * cross(u, v);
}

// §1.5 DETERMINISTIC RANDOMNESS

// --- Variation System — Seed-Driven Property Hashing
fn hash_property(seed: u32, property: u32) -> f32 {
    var h = seed * 747796405u + property * 2891336453u + 1u;
    h = ((h >> 16u) ^ h) * 2654435769u;
    h = ((h >> 16u) ^ h) * 2654435769u;
    h = (h >> 16u) ^ h;
    return f32(h) / f32(0xFFFFFFFFu);
}

// --- Variation System — Distribution Shaping
const GAUSSIAN_PAIR_OFFSET: u32 = 1000u;

fn sample_gaussian(seed: u32, property: u32, mean: f32, sigma: f32) -> f32 {
    // Box-Muller transform: two independent uniform samples → Gaussian sample.
    // u1 drives the radius (Rayleigh-distributed), u2 drives the angle.
    let u1 = max(hash_property(seed, property), 1e-6);  // clamp away from 0 to avoid log(0)
    let u2 = hash_property(seed, property + GAUSSIAN_PAIR_OFFSET);
    let z = sqrt(-2.0 * log(u1)) * cos(2.0 * PI * u2);

    // Truncate at ±3σ — bounded output, no outliers beyond 3 standard deviations.
    return mean + clamp(z, -3.0, 3.0) * sigma;
}


// §1.6 TERRAIN HEIGHT FUNCTION
// Stateless terrain height evaluation. Given (world_xz, master_seed, time),
struct TerrainBand {
    spacing: f32,        // lattice spacing (world units between nodes)
    freq_mean: f32,      // μ for spatial frequency (cycles per unit)
    freq_sigma: f32,     // σ for spatial frequency
    amp_mean: f32,       // μ for amplitude (world units of height)
    amp_sigma: f32,      // σ for amplitude
    damping_mean: f32,   // μ for damping coefficient (Gaussian draw)
    damping_sigma: f32,  // σ for damping coefficient
    damping_min: f32,    // floor — no wave extends beyond ~3/damping_min units
    activation: f32,     // probability that a lattice node contributes at all
    temporal_freq: f32,  // per-band multiplier on pool beat frequency
}

const TERRAIN_BAND_COUNT: u32 = 6u;

const TERRAIN_BANDS = array<TerrainBand, 6>(
    //              spacing  freq_μ  freq_σ  amp_μ  amp_σ  damp_μ  damp_σ  damp_min activ  t_freq
    //                                                                      reach≈3/min
    TerrainBand(    200.0,   0.030,  0.010,  8.0,   3.0,   0.008,  0.004,  0.005,  0.70,  0.05  ),  // 0: continental  reach≈600
    TerrainBand(     80.0,   0.080,  0.025,  3.0,   1.5,   0.020,  0.010,  0.010,  0.65,  0.10  ),  // 1: regional     reach≈300
    TerrainBand(     30.0,   0.200,  0.060,  1.2,   0.5,   0.040,  0.020,  0.020,  0.60,  0.20  ),  // 2: local        reach≈150
    TerrainBand(     12.0,   0.500,  0.150,  0.4,   0.2,   0.080,  0.040,  0.040,  0.55,  0.40  ),  // 3: detail       reach≈75
    TerrainBand(      5.0,   1.200,  0.350,  0.12,  0.05,  0.150,  0.075,  0.060,  0.50,  0.80  ),  // 4: fine         reach≈50
    TerrainBand(    500.0,   0.012,  0.004,  15.0,  6.0,   0.004,  0.002,  0.003,  0.75,  0.02  ),  // 5: tectonic     reach≈1000
);

// Property indices for deriving wave parameters from a lattice node seed.
// These occupy their own range (200+) to avoid collisions with entity
// property indices (0-119) and the Gaussian pair offset (1000+).
const WAVE_PROP_TYPE: u32      = 200u;  // radial vs directional
const WAVE_PROP_FREQ: u32      = 201u;  // spatial frequency (Gaussian draw)
const WAVE_PROP_AMP: u32       = 202u;  // amplitude (Gaussian draw)
const WAVE_PROP_DAMPING: u32   = 203u;  // damping coefficient (Gaussian draw)
const WAVE_PROP_DIR_ANGLE: u32 = 204u;  // direction angle (directional) or center offset angle (radial)
const WAVE_PROP_CENTER_R: u32  = 205u;  // center offset radius (radial only)
const WAVE_PROP_PHASE: u32     = 206u;  // initial phase offset
const WAVE_PROP_ACTIVE: u32    = 208u;  // activation gate (uniform draw vs band activation)


// SPATIAL FIELD MANIFEST — seed plumbing only. The tunable activity/
// band values (ACTIVITY_LATTICE_SPACING, ACTIVITY_BEAT_FREQ_LO/HI,
// WAVE_THRESHOLD[6] + softness) moved to THE TERRAIN_LOOKS PANEL
// ROW 7 (§2.2) — the movement third of the surface voice.
const ACTIVITY_SEED_BAND: u32       = 50u;      // lattice seed band (separate from terrain)
const ACTIVITY_PROP_LEVEL: u32      = 220u;     // property index: activity intensity
const ACTIVITY_PROP_BEAT_FREQ: u32  = 221u;     // property index: beat frequency

fn band_activity_level(raw_activity: f32, band_index: u32) -> f32 {
    let threshold = WAVE_THRESHOLD[band_index];
    return smoothstep(threshold, threshold + WAVE_THRESHOLD_SOFTNESS, raw_activity);
}

// Evaluate the activity field at a world position.
// Returns vec2(activity [0,1], beat_freq [0.25, 2.0] cycles/beat).
fn terrain_activity_at(world_xz: vec2<f32>, master_seed: u32) -> vec2<f32> {
    // Phase 1 dedupe (charter D3.1): routed through lattice_coord /
    // lattice_weight. The log-interp beat_freq draw stays its own line.
    let lc = lattice_coord(world_xz, ACTIVITY_LATTICE_SPACING);

    var activity: f32 = 0.0;
    var beat_freq: f32 = 0.0;

    for (var dz: i32 = 0; dz <= 1; dz++) {
        for (var dx: i32 = 0; dx <= 1; dx++) {
            let node = lc.base + vec2<i32>(dx, dz);
            let seed = lattice_node_seed(master_seed, node, ACTIVITY_SEED_BAND);

            let node_activity = hash_property(seed, ACTIVITY_PROP_LEVEL);
            let node_beat_freq = ACTIVITY_BEAT_FREQ_LO
                * pow(ACTIVITY_BEAT_FREQ_HI / ACTIVITY_BEAT_FREQ_LO, hash_property(seed, ACTIVITY_PROP_BEAT_FREQ));

            let weight = lattice_weight(lc, dx, dz);

            activity += node_activity * weight;
            beat_freq += node_beat_freq * weight;
        }
    }

    return vec2(activity, beat_freq);
}

// --- Lattice Node Seed
fn lattice_node_seed(master_seed: u32, node: vec2<i32>, band: u32) -> u32 {
    var h = master_seed;
    h ^= u32(node.x) * 73856093u;
    h ^= u32(node.y) * 19349663u;
    h ^= band * 83492791u;
    h = (h ^ (h >> 16u)) * 2654435769u;
    h = (h ^ (h >> 16u)) * 2654435769u;
    return h;
}

// --- Wave Evaluation — Two Primitives

fn evaluate_directional_wave(
    world_xz: vec2<f32>,
    node_world: vec2<f32>,
    freq: f32,
    amp: f32,
    damping: f32,
    dir: vec2<f32>,
    phase: f32,
) -> f32 {
    // Spatial phase: globally coherent ridges along `dir`.
    // Damping: envelope decays with perpendicular distance from the line
    // through node_world in direction `dir`.
    let offset = world_xz - node_world;
    let perp_dist = abs(offset.x * dir.y - offset.y * dir.x);
    let envelope = exp(-damping * perp_dist);
    return amp * envelope * sin(dot(dir, world_xz) * freq + phase);
}

fn evaluate_radial_wave(
    world_xz: vec2<f32>,
    center: vec2<f32>,
    freq: f32,
    amp: f32,
    damping: f32,
    phase: f32,
) -> f32 {
    // Concentric rings from `center`.
    // Damping: envelope decays with distance from center.
    let dist = length(world_xz - center);
    let envelope = exp(-damping * dist);
    return amp * envelope * sin(dist * freq + phase);
}

// --- Per-Node Wave Evaluation
// THE PAIR FORM (TRUEBAND_CONTACT_1 T1a) — the one derivation. The body
// is the T0-era evaluate_lattice_wave, moved verbatim (every expression
// in its existing float order — the U5a discipline); both branches now
// return (frozen, moving) instead of mixing. band_act stays in the
// signature for stability (the mix consumer applies it; the delta
// consumer applies it per node).
fn evaluate_lattice_wave_pair(
    world_xz: vec2<f32>,
    node: vec2<i32>,
    node_seed: u32,
    band: TerrainBand,
    band_act: f32,       // activity level for this band [0,1] (from hierarchy)
    beat_freq: f32,      // cycles per beat from activity field
    t_beats: f32,        // current time in beats
) -> vec2<f32> {
    // Activation gate: if draw exceeds band activation, this node is silent.
    // Spatially coherent — a silent node is silent for all query points.
    if (hash_property(node_seed, WAVE_PROP_ACTIVE) > band.activation) {
        return vec2(0.0, 0.0);
    }

    // Derive parameters from seed via Gaussian / uniform draws
    let is_radial = hash_property(node_seed, WAVE_PROP_TYPE) > 0.5;
    let freq = abs(sample_gaussian(node_seed, WAVE_PROP_FREQ, band.freq_mean, band.freq_sigma));
    var amp = abs(sample_gaussian(node_seed, WAVE_PROP_AMP, band.amp_mean, band.amp_sigma));
    // Indoor amplitude ceiling: clamp large waves to keep terrain gentle
    if (config.terrain_amp_ceiling > 0.0) {
        amp = min(amp, config.terrain_amp_ceiling);
    }
    let damping = max(abs(sample_gaussian(node_seed, WAVE_PROP_DAMPING, band.damping_mean, band.damping_sigma)), band.damping_min);
    let phase_base = hash_property(node_seed, WAVE_PROP_PHASE) * 2.0 * PI;

    // Two phases: frozen is the reference shape, moving advances with beats.
    // band.temporal_freq scales the pool's beat_freq per band:
    //   fine bands ripple fast, continental bands swell slowly.
    let phase_frozen = phase_base;
    let phase_moving = phase_base + t_beats * beat_freq * band.temporal_freq * 2.0 * PI;

    // Node world position: center of this lattice cell
    let node_world = (vec2<f32>(node) + 0.5) * band.spacing;

    if (is_radial) {
        let offset_angle = hash_property(node_seed, WAVE_PROP_DIR_ANGLE) * 2.0 * PI;
        let offset_r = hash_property(node_seed, WAVE_PROP_CENTER_R) * band.spacing * 0.3;
        let center = node_world + vec2(cos(offset_angle), sin(offset_angle)) * offset_r;
        let val_frozen = evaluate_radial_wave(world_xz, center, freq, amp, damping, phase_frozen);
        let val_moving = evaluate_radial_wave(world_xz, center, freq, amp, damping, phase_moving);
        return vec2(val_frozen, val_moving);
    } else {
        let dir_angle = hash_property(node_seed, WAVE_PROP_DIR_ANGLE) * 2.0 * PI;
        let dir = vec2(cos(dir_angle), sin(dir_angle));
        let val_frozen = evaluate_directional_wave(world_xz, node_world, freq, amp, damping, dir, phase_frozen);
        let val_moving = evaluate_directional_wave(world_xz, node_world, freq, amp, damping, dir, phase_moving);
        return vec2(val_frozen, val_moving);
    }
}

fn evaluate_lattice_wave(
    world_xz: vec2<f32>,
    node: vec2<i32>,
    node_seed: u32,
    band: TerrainBand,
    band_act: f32,       // activity level for this band [0,1] (from hierarchy)
    beat_freq: f32,      // cycles per beat from activity field
    t_beats: f32,        // current time in beats
) -> f32 {
    // One derivation, two consumers: the SAME two floats, the SAME mix
    // the old body applied — bake path bit-exact by construction.
    let pair = evaluate_lattice_wave_pair(world_xz, node, node_seed, band,
                                          band_act, beat_freq, t_beats);
    return mix(pair.x, pair.y, band_act);
}

// --- THE TRUE-BAND GATES (TRUEBAND_CONTACT_1): the writer's per-band
// multiplier + skip sentinel; held at rest (−1) by the boot block until
// the gen-2 band couplings land. The pools decide WHERE a woken band
// breathes; these decide THAT it breathes.
fn get_band_blend(band_index: u32) -> f32 {
    switch(band_index) {
        case 0u: { return config.band_blend_0; }
        case 1u: { return config.band_blend_1; }
        case 2u: { return config.band_blend_2; }
        case 3u: { return config.band_blend_3; }
        case 4u: { return config.band_blend_4; }
        case 5u: { return config.band_blend_5; }
        default: { return -1.0; }
    }
}

fn get_band_phase_origin(band_index: u32) -> f32 {
    switch(band_index) {
        case 0u: { return config.band_phase_origin_0; }
        case 1u: { return config.band_phase_origin_1; }
        case 2u: { return config.band_phase_origin_2; }
        case 3u: { return config.band_phase_origin_3; }
        case 4u: { return config.band_phase_origin_4; }
        case 5u: { return config.band_phase_origin_5; }
        default: { return 0.0; }
    }
}

// --- Per-Band Contribution
fn terrain_band_contribution(
    world_xz: vec2<f32>,
    master_seed: u32,
    t_beats: f32,
    band_index: u32,
    raw_activity: f32,   // from activity field [0,1]
    beat_freq: f32,      // from activity field (cycles/beat)
) -> vec2<f32> {
    let band = TERRAIN_BANDS[band_index];
    let band_act = band_activity_level(raw_activity, band_index);

    // Lattice position (continuous) and base node (integer)
    let lattice_pos = world_xz / band.spacing;
    let lattice_base = vec2<i32>(floor(lattice_pos));
    let frac = fract(lattice_pos);

    // Hermite smoothstep weights — C1 continuous at lattice boundaries
    let w = frac * frac * (3.0 - 2.0 * frac);

    // Blend contributions from 2×2 nearest lattice nodes
    var height: f32 = 0.0;
    var complexity: f32 = 0.0;
    for (var dz: i32 = 0; dz <= 1; dz++) {
        for (var dx: i32 = 0; dx <= 1; dx++) {
            let node = lattice_base + vec2<i32>(dx, dz);
            let node_seed = lattice_node_seed(master_seed, node, band_index);

            let wx = select(1.0 - w.x, w.x, dx == 1);
            let wz = select(1.0 - w.y, w.y, dz == 1);
            let weight = wx * wz;

            let wave_value = evaluate_lattice_wave(
                world_xz, node, node_seed, band,
                band_act, beat_freq, t_beats
            );
            height += wave_value * weight;

            // Track whether this node is active (contributes a wave)
            let is_active = f32(hash_property(node_seed, WAVE_PROP_ACTIVE) <= band.activation);
            complexity += weight * is_active;
        }
    }

    return vec2(height, complexity);
}

// THE DELTA FORM (TRUEBAND_CONTACT_1 T1a) — terrain_band_contribution's
// node walk (same lattice, same seeds, same Hermite weights), summing
// band_act × (moving − frozen) per node; no complexity accumulation.
// band_act via band_activity_level: the seeded pools SHAPE where a
// woken band breathes (campaign v2 §6).
fn true_band_delta_contribution(world_xz: vec2<f32>, seed: u32,
    t_eff_beats: f32, band_idx: u32,
    raw_activity: f32, beat_freq: f32) -> f32 {
    let band = TERRAIN_BANDS[band_idx];
    let band_act = band_activity_level(raw_activity, band_idx);

    let lattice_pos = world_xz / band.spacing;
    let lattice_base = vec2<i32>(floor(lattice_pos));
    let frac = fract(lattice_pos);
    let w = frac * frac * (3.0 - 2.0 * frac);

    var delta: f32 = 0.0;
    for (var dz: i32 = 0; dz <= 1; dz++) {
        for (var dx: i32 = 0; dx <= 1; dx++) {
            let node = lattice_base + vec2<i32>(dx, dz);
            let node_seed = lattice_node_seed(seed, node, band_idx);

            let wx = select(1.0 - w.x, w.x, dx == 1);
            let wz = select(1.0 - w.y, w.y, dz == 1);
            let weight = wx * wz;

            let pair = evaluate_lattice_wave_pair(
                world_xz, node, node_seed, band,
                band_act, beat_freq, t_eff_beats
            );
            delta += band_act * (pair.y - pair.x) * weight;
        }
    }
    return delta;
}

// --- Total Height
// (fn terrain_height_at RETIRED — UNIFIED_GROUND_1 U4; A2-3 census)

// --- Height + Complexity
fn terrain_height_and_complexity(world_xz: vec2<f32>, master_seed: u32, t_beats: f32) -> vec2<f32> {
    let af = terrain_activity_at(world_xz, master_seed);
    let raw_activity = af.x;
    let beat_freq = af.y;

    var height: f32 = 0.0;
    var complexity: f32 = 0.0;
    for (var b: u32 = 0u; b < TERRAIN_BAND_COUNT; b++) {
        let hc = terrain_band_contribution(world_xz, master_seed, t_beats, b, raw_activity, beat_freq);
        height += hc.x;
        complexity += hc.y;
    }
    return vec2(height, complexity / f32(TERRAIN_BAND_COUNT));
}


// §2 STATE

// §2.1 STRUCTS

// --- [STATE:signal] FrameSignal

struct FrameSignal {
    t_seconds: f32,
    t_beats: f32,
    dt: f32,
    aspect_ratio: f32,
    // DRIVERLESS: no shader consumer since M1-C. Kept as infrastructure;
    // whether GPU-side direct coupling exists at all is a parked gen-2
    // design decision.
    // vec4 element type: core WGSL requires 16-byte array strides in the
    // uniform address space (stride-4 f32 arrays are rejected by current
    // Tint). Byte layout unchanged: 256 B at offset 16; CPU mirror stays
    // std::array<float, 64>. (Re-applied after CHECKER-2 imported a
    // pre-fix copy of this file — see commit 951faf4 / PORT_MAP §5.)
    stats: array<vec4<f32>, 16>,
    move_x: f32,
    move_z: f32,
    look_az_delta: f32,
    look_el_delta: f32,
    zoom_delta: f32,
    pan_x_delta: f32,
    pan_y_delta: f32,
    dt_beats: f32,        // beat-time delta (currentBeats_ - prevBeats_)
    sky_mode: u32,        // 0 = grounded, 1 = pawn mounted on the ribbon head
    sky_head_x: f32,
    sky_head_y: f32,
    sky_head_z: f32,
    sky_heading: f32,
    // The saddle's FRAME — CPU-computed beside the mount point
    // (bodies/ribbon.hpp MOUNT_* mirrors); composed into the possessed agent's
    // quaternion in behavior_player_controlled. Zeros = level.
    sky_yaw_off: f32,     // tangent-align yaw deflection (rad)
    sky_pitch: f32,       // tangent-align pitch (rad)
    sky_roll: f32,        // bank into the lateral swing (rad, clamped)
}

// --- [STATE:terrain] TerrainState REMOVED — the dead terrain
//     buffer's struct + its bindings 20/220 are gone; no shader read them.

// --- [STATE:agent] AgentState
//
// Unified entity state — mirrors GPUAgentState in state.hpp (96 bytes).
// Slot 0 is the player's body (possessed at session start); slots 1..31
// are mood-authored agents driven by AGENT_BEHAVIORS. Scalar fields
// throughout so WGSL uniform/storage layout matches C++ without vec3
// alignment surprises. Orientation stored (not derived) to preserve
// terrain-tilt transparency for the possessed slot.
//
// See bodies/agents.hpp for rationale.
struct AgentState {
    pos_x: f32,
    pos_y: f32,
    pos_z: f32,
    t: f32,         // reserved (per-agent local clock; padding to vec4)
    vel_x: f32,
    vel_y: f32,
    vel_z: f32,
    heading: f32,
    home_x: f32,
    home_y: f32,
    home_z: f32,
    seed: u32,
    behavior_id: u32,
    tier_idx: u32,
    is_active: u32,
    portal_trigger: i32,   // only meaningful on possessed slot; -1 = none
    orient_x: f32,
    orient_y: f32,
    orient_z: f32,
    orient_w: f32,
    color_r: f32,   // per-agent body color (palette pick at spawn; 0 = tier fallback)
    color_g: f32,
    color_b: f32,
    _pad0: f32,
}

// ═══ AGENT REGISTRIES (read-only uniform buffers) ═══════════════════════
//
// PAIRED DECLARATIONS — KEEP IN SYNC:
//   AgentBehaviorParams (WGSL, here)   ↔ GPUAgentBehaviorDef (C++, state.hpp)
//   AgentTierParams     (WGSL, here)   ↔ GPUAgentTierDef     (C++, state.hpp)
//
// Both struct shapes must match field-for-field. The C++ side is
// authoritative — the values come from AGENT_BEHAVIORS / AGENT_TIER_GAINS
// in bodies/agents.hpp, uploaded once at world-init via the translator
// upload_agent_registries_to_gpu (which bridges CPU AgentBehaviorDef /
// AgentTierDef → GPU structs → these uniform buffers).
//
// If you change anything below, also update the matching C++ struct
// (and the translator copy field-list, and the WGSL schema comment
// reminder a few lines down). Field-order mismatches produce silent
// runtime corruption — no compile error.
//
// Schema reminder — fields below match GPUAgentBehaviorDef and
// GPUAgentTierDef in state.hpp, and AgentBehaviorDef / AgentTierDef
// in bodies/agents.hpp. Field-by-field translation lives in
// upload_agent_registries_to_gpu in bodies/agents.hpp.
//
//   AgentBehaviorParams columns:
//     step_rate       — steps/beat (musical time)
//     step_size       — world units/step
//     persistence     — [0,1] directional commitment
//     drag            — 1/s velocity decay
//     home_pull       — 1/s² tether spring coefficient
//     neighbor_radius — world units, flock/cohesion sample radius
//     speed_cap       — world units/s
//
//   AgentTierParams columns:
//     step_gain       — multiplies behavior.step_size impulse
//     persist_gain    — multiplies behavior.persistence (and home_pull)
//     speed_gain      — multiplies behavior.speed_cap
//     color_r/g/b     — vertex shader entity color

struct AgentBehaviorParams {
    step_rate:       f32,  // steps/beat (musical time)
    step_size:       f32,  // world units/step
    persistence:     f32,  // [0,1] biased-walk angle persistence
    drag:            f32,  // 1/s velocity decay
    home_pull:       f32,  // 1/s² spring toward home
    neighbor_radius: f32,  // flock neighbor search
    speed_cap:       f32,  // max speed
    _pad:            f32,  // pad to 32 bytes (matches GPUAgentBehaviorDef)
}

const AGENT_BEHAVIOR_COUNT_WGSL: u32 = 10u;

@group(0) @binding(110) var<uniform> agent_behaviors: array<AgentBehaviorParams, 10>;

struct AgentTierParams {
    step_gain:     f32,
    persist_gain:  f32,
    speed_gain:    f32,
    color_r:       f32,
    color_g:       f32,
    color_b:       f32,
    _pad0:         f32,  // pad to 32 bytes (matches GPUAgentTierDef)
    _pad1:         f32,
}

const AGENT_TIER_COUNT_WGSL: u32 = 4u;

@group(0) @binding(111) var<uniform> agent_tier_gains: array<AgentTierParams, 4>;

// --- [STATE:camera] CameraState

struct CameraState {
    pos: vec3<f32>,
    azimuth: f32,
    elevation: f32,
    distance: f32,
    pan_x: f32,
    pan_y: f32,
    // Damped aim point — the camera orbits this rather than the
    // possessed agent's raw position. Lerps toward the agent's pos
    // each frame in update_camera with a soft time constant. This
    // makes possession transfers (Caps Lock) glide rather than
    // teleport, while normal walking lag stays imperceptible.
    aim_point: vec3<f32>,
}

// --- [STATE:floating_entity] FloatingEntityState
// Replaces SphereState. Supports spheres, monoliths, future geometry.
// Motion type (orbit vs hover-bob) and geometry type are per-instance.

struct FloatingEntityState {
    pos: vec3<f32>,            //   0: world position (computed by GPU)
    body_radius: f32,          //  12: bounding/visual radius
    orientation: vec4<f32>,    //  16: quaternion
    influence_radius: f32,     //  32: zone/terrain influence range
    t: f32,                    //  36: curve parameter
    orbit_radius: f32,         //  40: distance from anchor (orbit mode)
    orbit_speed: f32,          //  44: angular velocity (orbit mode)
    color: vec3<f32>,          //  48: current appearance (coupling-driven)
    orbit_height: f32,         //  60: base altitude above terrain
    anchor: vec3<f32>,         //  64: world anchor point
    face_variance: f32,        //  76: per-face color spread (monolith)
    base_color: vec3<f32>,     //  80: seed-derived rest color
    geometry_type: u32,        //  92: 0=sphere, 1=monolith
    motion_type: u32,          //  96: 0=orbit, 1=hover-bob
    spin_speed: f32,           // 100: Y-axis rotation rate (hover-bob)
    bob_amplitude: f32,        // 104: vertical oscillation amplitude
    bob_period: f32,           // 108: vertical oscillation period
    spin_tilt_x: f32,          // 112: axis tilt X
    spin_tilt_z: f32,          // 116: axis tilt Z
    entity_seed: u32,          // 120: seed for VS face color hashing
    is_active: u32,            // 124: 0=inactive, 1=active
    aspect_y: f32,             // 128: Y-axis scale (1.0=cube, >1=tall, <1=flat)
    aspect_z: f32,             // 132: Z-axis scale (1.0=cube, <1=thin slab)
    // Drift-integrator substrate (cube use; spheres leave at zero).
    //   home = analytical rest (anchor.xz + ground + bob)
    //   pos  = home + drift
    spring_stiffness: f32,     // 136: pulls drift toward zero (1/s²)
    drag: f32,                 // 140: exponential damping on drift_vel (1/s)
    drift: vec3<f32>,          // 144: position offset from home (cube)
    tier_idx: u32,             // 156: runtime tier lookup for gain tables
    drift_vel: vec3<f32>,      // 160: drift integrator velocity
    behavior_id: u32,          // 172: cube behavior registry index
    // Kite mode: when follow_pawn != 0, home is computed
    // from pawn position + pawn_offset rather than from anchor + ground.
    // pawn_offset must sit at a 16-aligned offset (176) for vec3 layout —
    // see state.hpp for the C++ ordering and rationale.
    pawn_offset: vec3<f32>,    // 176/180/184: cube position relative to pawn
    behavior_phase: u32,       // 188: per-slot phase hash for behavior diversity
    follow_pawn: u32,          // 192: 0=anchor-relative, 1=pawn-relative
    _pad0: u32,                // 196: align to 208 (13×16)
    _pad1: u32,                // 200
    _pad2: u32,                // 204
}                              // 208 total

struct FloatingEntityArray {
    entities: array<FloatingEntityState, 264>,
}

// --- [STATE:ribbon] RibbonState
// Mirrors GPURibbonState in state.hpp BYTE-FOR-BYTE (112 B / 16-aligned).
struct RibbonState {
    anchor: vec3<f32>,      // world-space center of the ribbon
    time: f32,              // animation time (seconds) — drives the head's oscillation
    cube_count: u32,        // number of cross-section rings along the tube
    cube_size: f32,         // cross-section side length (ring spacing = cube_size)
    height: f32,            // base height above terrain
    checker_scatter: f32,   // per-cell color jitter amplitude (CONTRAST skin)
    color: vec3<f32>,       // ribbon color
    lateral_amp: f32,       // lateral wave amplitude (XZ plane sway)
    lateral_freq: f32,      // lateral head oscillation rate (rad/s)
    vertical_amp: f32,      // vertical wave amplitude (world Y)
    vertical_freq: f32,     // vertical head oscillation rate (rad/s)
    seed: u32,              // spawn seed (GPU-side per-ribbon hash key)
    propagation_speed: f32, // head→tail trail rate (world units/s)
    is_visible: u32,        // 0 = hidden, 1 = flying
    orientation: f32,       // heading angle (radians, 0 = +X axis)
    color_mode: u32,        // 0=smooth, 1=tinted, 2=contrast
    is_roaming: u32,        // constant 1 (analytic stationary spine removed); field retained until the next struct relayout
    _pad1: f32,
    _pad2: f32,
    _pad3: f32,
    color_b: vec3<f32>,     // second checker median (CONTRAST)
    hue_spread: f32,        // radians — per-cell hue rotation amplitude (CONTRAST skin; 0 = CB-1 look)
}

// Pre-computed per-ring transform (compute pass → VS + pawn overlay)
struct RibbonRingTransform {
    motor_p0: vec4<f32>,    // PGA motor rotor part
    motor_p1: vec4<f32>,    // PGA motor translator part
    center: vec3<f32>,      // ring world-space center (extracted from motor)
    terrain_y: f32,         // always 0.0 — flying ribbons (no terrain follow);
                            // retained for 48-byte stride (cleanup-campaign
                            // item: VS input layout stride)
}

// --- [STATE:patch] PatchParams
struct PatchParams {
    origin: vec2<f32>,      // world-space origin of the patch center
    extent: f32,            // side length in world units
    resolution: u32,        // texels per side (e.g. 128)
    master_seed: u32,       // world seed — deterministic terrain from this
    time: f32,              // current time for animated wave phases
    layer: u32,             // which layer of the heightfield array to write
    _pad1: f32,
}

// Per-patch rendering data. Storage buffer indexed by instance_index.
// Uploaded each frame with the current active patch set.
struct PatchInstance {
    origin: vec2<f32>,      // world-space XZ of patch center
    extent: f32,            // side length in world units
    layer: u32,             // heightfield array layer to sample
}

// Tile grid: smooth modifier field for archetype interpolation.
struct TileGridEntry {
    amp_scale: f32,
    height_bias: f32,
    activation_scale: f32,
    archetype: u32,            // 0=mountainous, 1=varied, 2=basin, 3=pool
}

// TILE_GRID ceiling — the pinned capacity pair's WGSL half; twin:
// Dim::TILE_GRID_CAPACITY (state.hpp). Authored, NOT derived from the
// radius — the dial never touches it. Raise it in BOTH rooms or
// glaw1/Dawn objects.
const TILE_GRID_CAPACITY: u32 = 1024u;

struct TileGrid {
    origin_x: i32,         // grid-space X of entry [0][0]
    origin_z: i32,         // grid-space Z of entry [0][0]
    side: u32,             // grid dimension (up to 19)
    cell_extent: f32,      // world units per cell (50.0)
    entries: array<TileGridEntry, TILE_GRID_CAPACITY>,
}

// Look up a tile grid entry by grid coordinate.
// Returns default (varied archetype, neutral modifiers) if outside the grid.
fn tile_grid_lookup(gx: i32, gz: i32) -> TileGridEntry {
    let lx = gx - tile_grid.origin_x;
    let lz = gz - tile_grid.origin_z;
    let s = i32(tile_grid.side);
    if (lx < 0 || lx >= s || lz < 0 || lz >= s) {
        return TileGridEntry(1.0, 0.0, 1.0, 1u);  // default: varied
    }
    return tile_grid.entries[lz * s + lx];
}

// Interpolate tile modifiers at a world position.
// STATUS: LATENT[tile-activation] — the .z channel (activation_scale) is
// authored CPU-side and Hermite-interpolated here, but consumed by
// neither height caller (contrib_static_base_at and
// ground_formed_with_complexity read only .x/.y). It is the intended
// tile-character axis; wiring is one multiply into band activation when
// wanted. No field changes — kept both sides.
fn tile_modifiers_at(world_xz: vec2<f32>) -> vec3<f32> {
    let cell = tile_grid.cell_extent;
    let gpos = world_xz / cell - 0.5;
    let gbase = vec2<i32>(floor(gpos));
    let frac = fract(gpos);
    let w = frac * frac * (3.0 - 2.0 * frac);

    var amp: f32 = 0.0;
    var bias: f32 = 0.0;
    var act: f32 = 0.0;

    for (var dz: i32 = 0; dz <= 1; dz++) {
        for (var dx: i32 = 0; dx <= 1; dx++) {
            let entry = tile_grid_lookup(gbase.x + dx, gbase.y + dz);
            let wx = select(1.0 - w.x, w.x, dx == 1);
            let wz = select(1.0 - w.y, w.y, dz == 1);
            let wt = wx * wz;
            amp += entry.amp_scale * wt;
            bias += entry.height_bias * wt;
            act += entry.activation_scale * wt;
        }
    }

    return vec3(amp, bias, act);
}

// --- Lattice Interpolation Helper
struct LatticeCoord {
    base: vec2<i32>,       // integer grid cell containing the position
    w: vec2<f32>,          // Hermite-smoothed fractional weights
}

fn lattice_coord(world_xz: vec2<f32>, spacing: f32) -> LatticeCoord {
    let gpos = world_xz / spacing;
    let gbase = vec2<i32>(floor(gpos));
    let frac = fract(gpos);
    let w = frac * frac * (3.0 - 2.0 * frac);  // Hermite smoothstep
    return LatticeCoord(gbase, w);
}

fn lattice_weight(lc: LatticeCoord, dx: i32, dz: i32) -> f32 {
    let wx = select(1.0 - lc.w.x, lc.w.x, dx == 1);
    let wz = select(1.0 - lc.w.y, lc.w.y, dz == 1);
    return wx * wz;
}

// --- Terrain palette & mode spatial fields
fn color_lattice_seed(node: vec2<i32>, band: u32) -> u32 {
    return lattice_node_seed(config.world_seed, node, band + 100u);
}

// At each palette lattice node: roll which palette dominates.
// Returns vec4(sand_weight, salmon_weight, green_weight, grey_weight).
// The dominant palette gets PALETTE_DOMINANT_WEIGHT, each other
// PALETTE_MINOR_WEIGHT (TERRAIN_LOOKS ROW 3, §2.2).
fn palette_weights_at_node(node: vec2<i32>) -> vec4<f32> {
    let seed = color_lattice_seed(node, 0u);
    let roll = hash_property(seed, 500u);

    // Cumulative weight selection from PALETTE_WEIGHT array
    var dominant: u32 = 3u;
    var cumul: f32 = 0.0;
    for (var i: u32 = 0u; i < 4u; i++) {
        cumul += config.palette_weight[i];
        if (roll < cumul) { dominant = i; break; }
    }

    // Build weights: the dominant palette takes PALETTE_DOMINANT_WEIGHT,
    // the other three PALETTE_MINOR_WEIGHT each (rows = dominant id,
    // columns = palette id — an implicit 4×4 constant matrix written as
    // branches; sums 1.0 per row). Dials at TERRAIN_LOOKS ROW 3 (§2.2).
    let dw = PALETTE_DOMINANT_WEIGHT;
    let mw = PALETTE_MINOR_WEIGHT;
    var w: vec4<f32>;
    if (dominant == 0u) {
        w = vec4(dw, mw, mw, mw);
    } else if (dominant == 1u) {
        w = vec4(mw, dw, mw, mw);
    } else if (dominant == 2u) {
        w = vec4(mw, mw, dw, mw);
    } else {
        w = vec4(mw, mw, mw, dw);
    }
    return w;
}

// At each mode lattice node: roll smooth/discrete tendency.
// Quintic bias — vast majority of the world is smooth sand.
// Raw uniform [0,1] → quintic [0,1] with mean ~0.17.
fn mode_tendency_at_node(node: vec2<i32>) -> f32 {
    let seed = color_lattice_seed(node, 1u);
    let raw = hash_property(seed, 501u);
    return pow(raw, MODE_BIAS_EXPONENT);  // high exponent → vast majority smooth
}

// Hermite-interpolated palette weights at a world position.
fn palette_field_at(world_xz: vec2<f32>) -> vec4<f32> {
    let lc = lattice_coord(world_xz, PALETTE_LATTICE_SPACING);
    var result = vec4(0.0);
    for (var dz: i32 = 0; dz <= 1; dz++) {
        for (var dx: i32 = 0; dx <= 1; dx++) {
            result += palette_weights_at_node(lc.base + vec2(dx, dz)) * lattice_weight(lc, dx, dz);
        }
    }
    return result;
}

// ZONE-GEOMETRY WARP — the vocabulary fields (mode, style, sparse)
// sample a warped domain: world' = world + vocab_warp(world). Two
// low-frequency value noises (lattice_coord/lattice_weight, fresh
// seed bands 17/18, prop 540) shear the square lattice's axis-aligned
// plateaus into organic coastlines. AMP = 0 → warp ≡ 0 → today's
// world, bit-exact. STRUCTURAL (world authorship): turning the dial
// redraws geography; baked patches show it only after regen — tune in
// DEBUG VIEW 4 (live), commit once. SEAMLESS by construction (pure
// world-space). THE FENCE: NOT applied to palette, region, chess,
// mono lattices; NOT to terrain_activity_at or any ground/height
// lattice. Color-side vocabulary only.
fn vocab_warp_channel(world_xz: vec2<f32>, band: u32) -> f32 {
    let lc = lattice_coord(world_xz, MODE_WARP_SCALE);
    var v: f32 = 0.0;
    for (var dz: i32 = 0; dz <= 1; dz++) {
        for (var dx: i32 = 0; dx <= 1; dx++) {
            let seed = color_lattice_seed(lc.base + vec2(dx, dz), band);
            v += hash_property(seed, 540u) * lattice_weight(lc, dx, dz);
        }
    }
    return v * 2.0 - 1.0;   // [-1, 1]
}

fn vocab_warp(world_xz: vec2<f32>) -> vec2<f32> {
    if (MODE_WARP_AMP <= 0.0) { return vec2(0.0); }   // identity, folded out at 0
    return vec2(vocab_warp_channel(world_xz, 17u),
                vocab_warp_channel(world_xz, 18u)) * MODE_WARP_AMP;
}

// Hermite-interpolated mode tendency at a world position. [0,1]
// Samples the WARPED domain (zone-geometry warp) — every caller
// (bake, live, instrument) gets the same geography automatically.
fn mode_field_at(world_xz: vec2<f32>) -> f32 {
    let wp = world_xz + vocab_warp(world_xz);
    let lc = lattice_coord(wp, MODE_LATTICE_SPACING);
    var result: f32 = 0.0;
    for (var dz: i32 = 0; dz <= 1; dz++) {
        for (var dx: i32 = 0; dx <= 1; dx++) {
            result += mode_tendency_at_node(lc.base + vec2(dx, dz)) * lattice_weight(lc, dx, dz);
        }
    }
    return result;
}

// Transition style at each lattice node: blend vs scatter.
// Trimodal: nodes commit to blend (0.0), scatter (1.0), or hybrid (0.5).
// Scatter slightly favored so probability fade-outs are more common.
fn transition_style_at_node(node: vec2<i32>) -> f32 {
    let seed = color_lattice_seed(node, 2u);
    let roll = hash_property(seed, 510u);
    // 25% pure blend, 30% hybrid, 45% pure scatter
    if (roll < 0.25) { return 0.0; }
    if (roll < 0.55) { return 0.5; }
    return 1.0;
}

// Hermite-interpolated transition style at world position.
// 0.0 = smooth color blend, 1.0 = probability scatter.
// Samples the WARPED domain (one warp with mode/sparse — transition
// character stays registered to the zones it dresses).
fn transition_style_at(world_xz: vec2<f32>) -> f32 {
    let wp = world_xz + vocab_warp(world_xz);
    let lc = lattice_coord(wp, TRANSITION_LATTICE_SPACING);
    var result: f32 = 0.0;
    for (var dz: i32 = 0; dz <= 1; dz++) {
        for (var dx: i32 = 0; dx <= 1; dx++) {
            result += transition_style_at_node(lc.base + vec2(dx, dz)) * lattice_weight(lc, dx, dz);
        }
    }
    return result;
}

// --- Sparse scatter: isolated cells and small clusters on smooth terrain
fn sparse_base_at_node(node: vec2<i32>) -> f32 {
    let seed = color_lattice_seed(node, 3u);
    let raw = hash_property(seed, 520u);
    return pow(raw, SPARSE_BASE_EXPONENT);
}

fn sparse_cluster_at_node(node: vec2<i32>) -> f32 {
    let seed = color_lattice_seed(node, 4u);
    return hash_property(seed, 521u);
}

fn sparse_field_at(world_xz: vec2<f32>) -> f32 {
    // Samples the WARPED domain — ONE warp evaluation for base AND
    // cluster (and the same warp mode/style ride), keeping the sparse
    // texture registered to the zone geography.
    let wp = world_xz + vocab_warp(world_xz);

    // Base: broad sparse tendency
    let lc_b = lattice_coord(wp, SPARSE_BASE_SPACING);
    var base_val: f32 = 0.0;
    for (var dz: i32 = 0; dz <= 1; dz++) {
        for (var dx: i32 = 0; dx <= 1; dx++) {
            base_val += sparse_base_at_node(lc_b.base + vec2(dx, dz)) * lattice_weight(lc_b, dx, dz);
        }
    }

    // Cluster: small-scale density modulation within sparse regions
    let lc_c = lattice_coord(wp, SPARSE_CLUSTER_SPACING);
    var cluster_val: f32 = 0.0;
    for (var dz: i32 = 0; dz <= 1; dz++) {
        for (var dx: i32 = 0; dx <= 1; dx++) {
            cluster_val += sparse_cluster_at_node(lc_c.base + vec2(dx, dz)) * lattice_weight(lc_c, dx, dz);
        }
    }

    // Combine: base sets the envelope, cluster creates hot spots within it.
    let cluster_boost = 1.0 + cluster_val * 2.0;
    return base_val * cluster_boost;
}

// (Terrain-mode coupling field RETIRED — Phase 1, ruling 6. Lattice 9
//  — seeds 15/16, props 530/531 — sat behind a magnitude dial parked
//  at 0.0: three evaluators computed on every bake and multiplied to
//  zero. Retired whole: the evaluators, the ROW 6 dials, and the shift
//  block in evaluate_cell_fields. The seeds and property IDs stay
//  reserved — do not reuse 15/16 or 530/531. The identifiers and the
//  ledger live in the charter (lattice table row 9, ruling 6).)

// --- Chess board field: strict two-color alternation
// (CHESS_LATTICE_SPACING defined in Color Field Spatial Config block below)

fn chess_tendency_at_node(node: vec2<i32>) -> f32 {
    let seed = color_lattice_seed(node, 12u);
    let raw = hash_property(seed, 850u);
    // pow(25) — B&W chess nearly extinct. Only raw > 0.99 contributes.
    let r5 = raw * raw * raw * raw * raw;
    let r25 = r5 * r5 * r5 * r5 * r5;
    return r25;
}

// Each chess region rolls a pair of colors.
// Returns vec3 for color A. Color B is derived separately.
fn chess_color_a_at_node(node: vec2<i32>) -> vec3<f32> {
    let seed = color_lattice_seed(node, 13u);
    return vec3(
        hash_property(seed, 860u),
        hash_property(seed, 861u),
        hash_property(seed, 862u)
    );
}

fn chess_color_b_at_node(node: vec2<i32>) -> vec3<f32> {
    let seed = color_lattice_seed(node, 14u);
    return vec3(
        hash_property(seed, 870u),
        hash_property(seed, 871u),
        hash_property(seed, 872u)
    );
}

struct ChessField {
    tendency: f32,
    color_a: vec3<f32>,
    color_b: vec3<f32>,
}

fn chess_field_at(world_xz: vec2<f32>) -> ChessField {
    let lc = lattice_coord(world_xz, CHESS_LATTICE_SPACING);
    var tendency: f32 = 0.0;
    var color_a = vec3(0.0);
    var color_b = vec3(0.0);

    for (var dz: i32 = 0; dz <= 1; dz++) {
        for (var dx: i32 = 0; dx <= 1; dx++) {
            let node = lc.base + vec2(dx, dz);
            let wt = lattice_weight(lc, dx, dz);
            tendency += chess_tendency_at_node(node) * wt;
            color_a += chess_color_a_at_node(node) * wt;
            color_b += chess_color_b_at_node(node) * wt;
        }
    }

    return ChessField(tendency, color_a, color_b);
}

// (palette_color_smooth — the palette's governing expression —
//  relocated to THE TERRAIN_LOOKS PANEL ROW 8, §2.2: it lives beside
//  its dials. The palette_color + PALETTE_VARIANCE retirement record
//  (fork resolved RETIRE) is carried by the panel's ROW 1.)

// --- Discrete cell color system
// (DISCRETE_*_LATTICE_SPACING defined in Color Field Spatial Config block)

// Per-node: roll RGB mean and variance for a discrete color region —
// the region's anatomy: seed color mean and spread.
// (Prop 804 "receptivity" RETIRED — Phase 1, ruling 3: the pc-color
//  field ships without it; chess_eff/mono_eff carry the listening
//  geography. The property ID stays reserved — do not reuse 804.)
struct DiscreteRegion {
    mean: vec3<f32>,
    variance: f32,
}

fn discrete_region_at_node(node: vec2<i32>) -> DiscreteRegion {
    let seed = color_lattice_seed(node, 10u);
    var out: DiscreteRegion;
    out.mean = vec3(hash_property(seed, 800u),
                    hash_property(seed, 801u),
                    hash_property(seed, 802u));
    // Variance: how much cells spread from the mean. [0.02 .. 0.25]
    out.variance = 0.02 + hash_property(seed, 803u) * 0.23;
    return out;
}

// Per-node: monochrome tendency [0, 1].
// Quintic — B&W cells within colorful zones are uncommon.
fn discrete_mono_at_node(node: vec2<i32>) -> f32 {
    let seed = color_lattice_seed(node, 11u);
    let raw = hash_property(seed, 810u);
    // pow(20) — B&W cells nearly extinct within colorful zones.
    let r5 = raw * raw * raw * raw * raw;
    let r10 = r5 * r5;
    let r20 = r10 * r10;
    return r20;
}

// Interpolated discrete region parameters at a world position.
// Phase 1 dedupe (charter D3.1): routed through lattice_coord /
// lattice_weight — the same Hermite math the hand-inlined form carried.
fn discrete_region_at(world_xz: vec2<f32>) -> DiscreteRegion {
    let lc = lattice_coord(world_xz, DISCRETE_COLOR_LATTICE_SPACING);

    var mean = vec3(0.0);
    var variance = 0.0;
    for (var dz: i32 = 0; dz <= 1; dz++) {
        for (var dx: i32 = 0; dx <= 1; dx++) {
            let val = discrete_region_at_node(lc.base + vec2(dx, dz));
            let ww = lattice_weight(lc, dx, dz);
            mean += val.mean * ww;
            variance += val.variance * ww;
        }
    }
    var out: DiscreteRegion;
    out.mean = mean;
    out.variance = variance;
    return out;
}

// Interpolated monochrome tendency at a world position. [0,1]
// Phase 1 dedupe (charter D3.1): routed through lattice_coord / lattice_weight.
fn discrete_mono_at(world_xz: vec2<f32>) -> f32 {
    let lc = lattice_coord(world_xz, DISCRETE_MONO_LATTICE_SPACING);

    var result: f32 = 0.0;
    for (var dz: i32 = 0; dz <= 1; dz++) {
        for (var dx: i32 = 0; dx <= 1; dx++) {
            let val = discrete_mono_at_node(lc.base + vec2(dx, dz));
            result += val * lattice_weight(lc, dx, dz);
        }
    }
    return result;
}

// CHECKER-REBUILD: the region's music-driven median (S1 + S2). Pull the
// region from its seed color toward the resultant by presence (S1), and
// wander it AROUND the resultant with a per-region hue-space offset scaled
// by presence (S2). CONSTITUTIONAL: the wander seeds on the region node
// ONLY — a STATIC, one-per-region personality held for the whole session.
// All time-variation enters through the CPU envelope (Segments); the GPU
// receives only glided values and static seeds. A stepped GPU-side hash is
// a teleport by construction and is BANNED in this module.
// QUANTUM CLAUSE (charter C8): the node derives from the CELL — by type.
fn checker_region_median(cell: vec2<i32>, seed_mean: vec3<f32>,
                         resultant: vec3<f32>, music_amount: f32) -> vec3<f32> {
    // Region id: the 80-unit node containing the CELL CENTER.
    let cs = PATCH_EXTENT / f32(PATCH_CELL_N);
    let center = (vec2<f32>(cell) + 0.5) * cs;
    let node = vec2<i32>(floor(center / DISCRETE_COLOR_LATTICE_SPACING));
    let rid = color_lattice_seed(node, 20u);
    let wander = (vec3(hash_property(rid, 601u),
                       hash_property(rid, 602u),
                       hash_property(rid, 603u)) - 0.5) * 2.0 * CHECKER_WANDER * music_amount;
    let music_median = clamp(resultant + wander, vec3(0.0), vec3(1.0));
    return mix(seed_mean, music_median, music_amount);   // S1: pull by presence
}

// (discrete_cell_color — the Phase-1 thin resolver — DELETED, Phase 2
//  D2.1. discrete_cell_color_at_tier below is THE pigment authority;
//  callers pass identity.tier directly. One function, two moments:
//  the bake calls it with VOICE = rest literals, the live path with
//  VOICE = now — same body, so silence ⇒ the bake, bit for bit.)

// THE PIGMENT AUTHORITY (Phase 2, D2.1). Resolves a cell's discrete
// color for a tier FIELD already decided (the cascade in
// evaluate_cell_fields — or a forced tier: the
// palette-drift pass targets one) — no cascade re-derivation here. Each tier derives from the cell's own seeds/fields; the music
// enters only through checker_region_median (unchanged, law-marked) and
// the variance widening. The anchors — chess pair, chess B&W, pure B&W,
// grey bases — are the protected set (charter C3), unchanged.
//   0 = full color       (turned median + widened spread)
//   1 = tinted mono      (dark/light grey, tinted toward the turned median)
//   2 = pure B&W         (0.02 or 0.95)
//   3 = chess B&W        (0.03 or 0.95 by parity)
//   4 = chess colorful   (chess.color_a/b by parity)
fn discrete_cell_color_at_tier(
    world_xz: vec2<f32>, cell_gx: i32, cell_gz: i32, cell_seed: u32, tier: u32,
    resultant: vec3<f32>, music_amount: f32, music_variance: f32
) -> vec3<f32> {
    let bw_roll = hash_property(cell_seed, 830u);
    let parity = (cell_gx + cell_gz) & 1;

    switch (tier) {
        case 4u: {
            let chess = chess_field_at(world_xz);
            return select(chess.color_a, chess.color_b, parity == 1);
        }
        case 3u: {
            return select(vec3(0.03), vec3(0.95), parity == 1);
        }
        case 2u: {
            return select(vec3(0.02), vec3(0.95), bw_roll > 0.5);
        }
        case 1u: {
            let region = discrete_region_at(world_xz);
            let base_grey = select(0.12, 0.85, bw_roll > 0.5);
            // CHECKER-REBUILD: tint target = the region's music median
            // (grey base stays a seed-pure anchor).
            let turned = checker_region_median(vec2(cell_gx, cell_gz), region.mean, resultant, music_amount);
            return mix(vec3(base_grey), turned, DISCRETE_TINT_STRENGTH);
        }
        default: {
            let region = discrete_region_at(world_xz);
            let nr = (hash_property(cell_seed, 840u) - 0.5) * 2.0;
            let ng = (hash_property(cell_seed, 841u) - 0.5) * 2.0;
            let nb = (hash_property(cell_seed, 842u) - 0.5) * 2.0;
            // CHECKER-REBUILD: median = pull + wander; spread widened by
            // the distinct-pc count (S3).
            let turned = checker_region_median(vec2(cell_gx, cell_gz), region.mean, resultant, music_amount);
            return clamp(turned + vec3(nr, ng, nb)
                             * (region.variance + min(CHECKER_VAR_MAX, music_variance * CHECKER_VAR_PER_NOTE)),
                         vec3(0.0), vec3(1.0));
        }
    }
}

// --- [STATE:config] DesignConfig

// ─── GROWTH LAW ── this struct mirrors state.hpp GPUDesignConfig
// FIELD-FOR-FIELD; grow only in lockstep with the C++ room — same
// commit, same position, same type; the C++ sizeof witness is the
// handshake. Comments here carry sentinels/semantics; VALUES rest in
// the ROW blocks (§2.2) or the CPU boot pins — never as kernel
// literals.
struct DesignConfig {
    mute_dynamics_0d: u32,
    mute_dynamics_2d: u32,
    mute_signal: u32,
    mute_couplings: u32,
    wave_time_scale: f32,
    pawn_speed: f32,
    camera_sensitivity: f32,
    freeze_sphere: u32,
    active_cell_size: f32,
    fpv_mode: u32,
    wave_enable_mask: u32,
    wave_freeze_mask: u32,
    // three scalars, not array<f32,3>: core WGSL rejects stride-4 arrays in
    // uniform address space. Same 12 bytes, same offsets; CPU mirror stays
    // float[3].
    wave_frozen_t0: f32,
    wave_frozen_t1: f32,
    wave_frozen_t2: f32,
    world_seed: u32,              // master seed for terrain/zone generation
    sun_direction: vec3<f32>,
    aura_enabled: f32,            // 0.0 = off, 1.0 = on (guards all aura sampling)
    pawn_amp_scale: f32,
    pawn_height_bias: f32,
    pawn_aura_height: f32,
    fog_density: f32,             // exponential fog coefficient (default 0.003)
    fog_color: vec3<f32>,         // fog/sky color RGB
    fade_alpha: f32,              // 0.0 = no overlay, 1.0 = fully opaque
    fade_color: vec3<f32>,        // transition overlay RGB
    pier_count: u32,              // active pier count for bounded iteration
    world_bound_min: vec2<f32>,   // XZ min clamp (0,0 = infinite)
    world_bound_max: vec2<f32>,   // XZ max clamp (0,0 = infinite)
    placement_patch_count: u32,   // active patches for entity Y-correction (decoupled from photographer)
    terrain_amp_ceiling: f32,     // max per-wave amplitude (0 = unlimited, >0 = clamp for indoor)
    ceiling_height: f32,          // indoor ceiling Y for camera clamp (0 = no ceiling)
    terrain_time: f32,            // t_beats for terrain evaluation (0 = frozen)
    // ─── Polyphony-driven band motion ────────────────────────────
    // Per-band blend: -1 = inactive sentinel (the evaluators skip the band); [0,1] = activation.
    band_blend_0: f32,            // continental
    band_blend_1: f32,            // regional
    band_blend_2: f32,            // local
    band_blend_3: f32,            // detail
    band_blend_4: f32,            // fine
    band_blend_5: f32,            // tectonic
    // Per-band phase origin (t_beats at activation).
    band_phase_origin_0: f32,
    band_phase_origin_1: f32,
    band_phase_origin_2: f32,
    band_phase_origin_3: f32,
    band_phase_origin_4: f32,
    band_phase_origin_5: f32,
    // ─── Musical animation modes ─────────────────────────────────
    mode_color_shift: f32,        // SIGNED axis on the mode field; rest 0 is the CENTER (− retreats, + advances); range graduates at Movement 1 close
    mode_checker_scatter: f32,    // SIGNED axis on sparse survival; rest 0 the center (− extinguishes, + populates); range graduates at Movement 1 close
    mode_palette_target: f32,     // [0,3] target palette (0=sand 1=salmon 2=green 3=warm)
    mode_palette_intensity: f32,  // [0,1] drift strength toward target palette
    mode_discrete_tier: f32,      // [0,4] target discrete tier (0=color 1=tinted 2=BW 3=chessBW 4=chessColor)
    mode_gol_tick_scale: f32,     // tick period multiplier (1.0=normal, <1=faster)
    mode_gol_height_scale: f32,   // alive_height multiplier (1.0=normal, >1=taller)
    floater_coordination: f32,    // [0,1] cube behavior synchrony knob
    // ─── Radial pulse ring buffer ────────────────────────────────
    pulse_count: u32,
    // Agent system: slot index of the player's current body in
    // agent_state[]. Piggybacks on the radial-pulse pad triple (no
    // struct size delta). Order matches GPUDesignConfig in state.hpp.
    possessed_slot: u32,
    veil_dither: f32,     // THE RIM taste knob: >0.5 → icing dither-dissolves (mirror of GPUDesignConfig)
    indoor_height_cap: f32,  // indoor GoL height cap, 0 = disabled (mirror of GPUDesignConfig — last pulse pad repurposed)
    pulse_data: array<vec4<f32>, 8>,  // each: (origin_x, origin_z, onset_seconds, amplitude)
    // CPU-banded POINT position for LOD0/LOD1 partition (renamed
    // lod_pawn → lod_point: the value has been THE POINT).
    // Read by the frustum-cull shader so its dist² test agrees with
    // the CPU's banding point (no 1-2 frame disagreement at the
    // boundary annulus).
    lod_point_x: f32,
    lod_point_z: f32,
    // The point's host + fly speed — piggybacked on the
    // lod-point pad pair (no struct size delta; the possessed_slot
    // precedent). Order matches GPUDesignConfig in state.hpp.
    point_host: u32,
    point_fly_speed: f32,
    // THE VEIL (re-ruled: RING = draw authority, fog = icing). Mirrors
    // GPUDesignConfig (state.hpp):
    //   veil_ring — the SOLE draw authority (325): the band's outer
    //     gate, the entity cull, and every VS draw gate read it;
    //     nothing draws beyond it.
    //   veil_icing — δ: the narrow fade band [ring−δ, ring] in
    //     shade_lit (cosmetic; joins materialize inside it).
    //   veil_strength — 1 outdoors, 0 finite/indoor (staged by U5).
    //   lod0_radius — the terrain full/half-mesh split (175), read by
    //     the frustum-cull LOD0 gate + the CPU band (one yardstick).
    veil_ring: f32,
    veil_icing: f32,
    veil_strength: f32,
    lod0_radius: f32,
    // ── The palette mirror (FORK-tier graduation) — C++ twin in
    //    GPUDesignConfig; rest = the pre-graduation literals. rgb in
    //    xyz (w pad); weight component i = palette i.
    palette_center: array<vec4<f32>, 4>,
    palette_light: array<vec4<f32>, 4>,
    palette_weight: vec4<f32>,
    // ── CHECKER-REBUILD: the pitch-class color field — C++ twin in
    //    GPUDesignConfig (vec3 + f32 + f32; the config GREW a second
    //    16-byte slot, sizeof witness 576 -> 592, Jean OK'd). resultant =
    //    the music color (a weighted pc-length average, enveloped);
    //    music_amount = presence [0,1] (S1 pull + S2 wander scale);
    //    music_variance = distinct-pc count (S3 within-patch spread).
    //    REST = amount 0 -> the cell's seed color; the bake passes
    //    amount 0 (seam-proof).
    checker_resultant: vec3<f32>,
    checker_music_amount: f32,
    checker_music_variance: f32,
}

// §2.2 — THE TERRAIN_LOOKS PANEL (WGSL room)
// ═══════════════════════════════════════════════════════════════════
// The D2 surface-voice authoring surface: geometry, color, movement —
// what the terrain LOOKS like. Third foundational panel, after
// CameraControls and population_themes (the population
// panel). THE MAPPING CONVERSATION convenes OVER this panel:
// wires land as rows beside the parameters they drive.
//
// TWO ROOMS, ONE PANEL — the mirror rule (the binding-registry
// ceiling, said honestly): the C++ room is surface/terrain_looks.hpp.
// Same rows, same order. No machine gate crosses the language gap —
// glaw1 is WGSL-blind and WGSL cannot include headers — so the
// discipline is: every VALUE lives in exactly ONE room, and the other
// room carries the row as a named pointer only (no cross-language
// number exists to drift). The only shared text is the row index
// below. Nets: the boot rig (the world must look identical) for
// values; Dawn binding validation for uniform layout; the C6 registry
// for binding numbers.
//
// ROW INDEX (identical in both rooms):
//   ROW 1 — THE PALETTE QUARTET        values in the C++ room (REST) → config uniform
//   ROW 2 — MOTION & MODE REST PINS    values in the C++ room → setters at boot
//   ROW 3 — PALETTE COMPOSITION        values HERE
//   ROW 4 — FIELD LATTICES             values HERE
//   ROW 5 — COMPOSITE CUTS & EDGES     values HERE
//   ROW 6 — TERRAIN-MODE COUPLING      values HERE
//   ROW 7 — THE MOVEMENT THIRD         values HERE
//   ROW 8 — GOVERNING EXPRESSIONS      text HERE
//   ROW 9 — THE CONTRIBUTOR ROSTER     pointers, both rooms
//
// STAYS OUT (machinery / other jurisdictions): streaming, the tile
// cache, manifold dispatch, bake kernel bodies, spawn/population
// (population_themes' panel), the veil (visibility's jurisdiction),
// GoL/pulse internals (ROW 9 points at the shared tint funnel).

// RAYMARCH/SDF EXCAVATION: the legacy WAVES table + WAVE_COUNT +
// HEIGHT_MAX_AMPLITUDE + the amplitude-trajectory feeder constants
// (IDLE_AMPLITUDE_SCALE / AMPLITUDE_ATTACK_TIME / AMPLITUDE_RELEASE_TIME)
// were the animated field the dead SDF marched — read only by the
// (also removed) update_terrain_config lipschitz chain, never by any live
// VS/FS. Removed. NOT the live OVERLAY_WAVES voice (that stays).
// SAND_DUNE_CENTER / SAND_DUNE_VARIANCE removed — used only by the
// (removed) coupling_sphere_to_terrain_tint. (RAYMARCH/SDF excavation)

// ── ROW 1 — THE PALETTE QUARTET (GRADUATED) ────────────────────────
// The four palettes now live in the CONFIG UNIFORM (config.palette_center
// / palette_light / palette_weight — C++ twin GPUDesignConfig, setters
// set_palette_*): the FORK-tier graduation that makes the MEDIANS
// couplable ("a spectrum moves the median" writes palette_center).
// AXES: index = palette id (0 sand / 1 salmon / 2 green / 3 warm),
//   shared with the dominant-weight branch in palette_weights_at_node.
// UNITS: center/light = rgb 0-1 (vec4, w pad); weight = selection
//   probability, component i = palette i (sums 1.0).
// CONSUMERS: palette_color_smooth (center/light mixed by complexity —
//   pinned at PALETTE_COMPLEXITY, ROW 3); palette_target_color (drift);
//   palette_weights_at_node (weight).
// REST (the boot values, bit-identical to the pre-graduation consts —
//   authored at the C++ room's ROW 1, terrain_looks::PALETTE_*_REST):
//   center: sand {.85,.70,.50} · salmon {.88,.58,.48} ·
//           green {.45,.58,.38} (rare) · warm {.82,.55,.42}
//   light:  {.92,.82,.65} · {.95,.72,.62} · {.62,.72,.52} · {.92,.72,.58}
//   weight: .42 · .28 · .04 (green rare) · .26
// (PALETTE_VARIANCE retired with its dead consumer palette_color — the
//  2b fork resolved RETIRE under the bit-identical law; values were
//  .08/.14/.20/.12, held by git + the designer tool's design space.)

// ── ROW 2 — MOTION & MODE REST PINS (values in the C++ room) ───────
// The surface voice's silence. config.terrain_time / the band
// blend+phase-origin arrays / mode_color_shift / mode_checker_scatter
// / mode_palette_{target,intensity,discrete_tier} rest at
// terrain_looks::REST_* (C++ ROW 2), written once by the cartridge
// boot-pin block; nothing else authors them today. terrain_time ≤ 0
// freezes the true-band writer (ROW 7's consumer — the card's heights
// pass; TRUEBAND_CONTACT_1) — rest IS today's stillness. The mode trio is DRIVERLESS since the gen-1
// retirement: driver-ready dials held at rest, read by
// animated_cell_color — the one live composite body (analytic since
// Commit C) — (mode_bias, sparse_bias, drift).
// CHECKER-REBUILD THE PITCH-CLASS COLOR FIELD (LIVE, gen-2):
// config.checker_resultant carries the length-weighted average of the
// voice's WINDOW pc-length vector over Jean's authored PC_COLOR table
// (coupling/visual_canvas.hpp; absolute pitch class → RGB, index 0 = D).
// config.checker_music_amount = presence (enveloped 2-beat attack /
// 8-beat release); config.checker_music_variance = distinct-pc count.
// Rest at REST_CHECKER_* (C++ ROW 2). Consumed by
// discrete_cell_color_at_tier — THE pigment authority since Phase 2:
// each discrete cell is PULLED toward the resultant (S1, by
// amount), its region WANDERS around it by a STATIC per-region offset
// (S2), and its own spread WIDENS by the count (S3); the mode field's
// composite gating decides which cells (between smooth sections) show
// it. Dials: ROW 5 CHECKER_WANDER / CHECKER_DEBUG_VIEW. Bake passes
// amount 0 → seed (seam-proof by law).

// ── ROW 3 — PALETTE COMPOSITION ─────────────────────────────────────
// How the quartet becomes a color: the dominant-branch matrix weights
// + the complexity mix dial.
const PALETTE_DOMINANT_WEIGHT: f32 = 0.85;  // dominant palette's share at a lattice node — sharpness dial (raise for harder palette regions)
const PALETTE_MINOR_WEIGHT: f32 = 0.05;     // each non-dominant share; dominant + 3×minor sums 1.0 per row (palette_weights_at_node's row-stochastic matrix)
// The center/light mix dial — palette_color_smooth's and
// palette_target_color's `complexity` argument, PINNED since the husk
// sweep removed the live complexity channel. Stated ONCE here; every
// call site (5 today: 3× palette_color_smooth, 2× palette_target_color)
// reads this constant. The terrain_color_designer carries it as a
// preview dial.
const PALETTE_COMPLEXITY: f32 = 0.5;

// ── ROW 4 — FIELD LATTICES (Color Field Spatial Config) ────────────
const PALETTE_LATTICE_SPACING: f32 = 300.0;     // ~6 patches — large palette blobs
const MODE_LATTICE_SPACING:    f32 = 120.0;      // ~2.4 patches — smooth/discrete clusters
const MODE_DISCRETE_THRESHOLD: f32 = 0.70;       // above → discrete cells
const MODE_BIAS_EXPONENT: f32 = 5.0;             // quintic — vast majority smooth
const TRANSITION_LATTICE_SPACING: f32 = 200.0;   // large blend/scatter zones
const SPARSE_BASE_SPACING: f32 = 160.0;          // broad sparse tendency regions
const SPARSE_CLUSTER_SPACING: f32 = 40.0;         // small dense patches within sparse
const SPARSE_BASE_EXPONENT: f32 = 3.0;           // cubic — moderately rare singles
const CHESS_LATTICE_SPACING: f32 = 55.0;          // very small B&W alternation zones
const DISCRETE_COLOR_LATTICE_SPACING: f32 = 80.0; // medium colored cell blobs
const DISCRETE_MONO_LATTICE_SPACING: f32 = 250.0; // large B&W tendency zones

// ── ZONE-GEOMETRY (ROW 4 group) — the vocabulary domain warp ────────
// STRUCTURAL dials: they redraw where checkers may live. AMP 0 =
// identity (today's world, bit-exact; the warp folds out). Tune LIVE
// in TERRAIN_DEBUG_VIEW 4 — the art shows it only after patch regen.
const MODE_WARP_AMP: f32 = 0.0;      // wu displacement; 0 = identity
const MODE_WARP_SCALE: f32 = 240.0;  // wavelength (~2× mode spacing)

// (temperament retired — YAGNI, Movement 1 close; prop 502 reserved. charter.)

// ── ROW 5 — COMPOSITE CUTS & EDGES ──────────────────────────────────
// The decision thresholds of the color composite, promoted OUT of the
// stage bodies WITH NAMES (TERRAIN_LOOKS gather; behavior-identical —
// each const carries the exact expression/literal it replaced).
// UNITS: cuts on [0,1] field values unless noted.
// CONSUMERS: composite_cell_color — the ONE body since Phase 2 (mode
//   edges, sparse survival); discrete_cell_color_at_tier — THE pigment
//   authority since Phase 2 (chess/mono cuts, tint); the cascade in
//   evaluate_cell_fields (chess/mono cuts).
// Mode edges — where smooth hands over to discrete, anchored on
// MODE_DISCRETE_THRESHOLD (ROW 4's gate):
const MODE_BLEND_EDGE_LO: f32 = MODE_DISCRETE_THRESHOLD - 0.15;      // blend ramp start
const MODE_BLEND_EDGE_HI: f32 = MODE_DISCRETE_THRESHOLD + 0.05;      // blend ramp end
const MODE_SCATTER_CORE_EDGE: f32 = MODE_DISCRETE_THRESHOLD + 0.05;  // scatter: full survival
const MODE_SCATTER_FLOOR_EDGE: f32 = MODE_DISCRETE_THRESHOLD - 0.35; // scatter: zero survival; also the mode-zone floor for sparse exclusion
// Sparse survival — isolated cells outside mode zones:
const SPARSE_SURVIVAL_THRESHOLD: f32 = 0.22;   // sparse field value where cells begin to survive
const SPARSE_SURVIVAL_WINDOW: f32 = 0.35;      // smoothstep width above the threshold
// Chess / mono tiers — discrete_cell_color's ladder:
const CHESS_TENDENCY_CUT: f32 = 0.45;     // chess field gate (per-cell jittered)
const CHESS_COLORFUL_CUT: f32 = 0.65;     // above → colored pair; below → B&W chess
const MONO_BW_CUT: f32 = 0.35;            // mono field gate → pure black/white cells
const MONO_TINT_CUT: f32 = 0.20;          // mono field gate → tinted monochrome cells
const DISCRETE_TINT_STRENGTH: f32 = 0.15; // grey→palette-mean mix weight; PINNED — no uniform behind it (couplable literal, flagged by the color-stack recon)

// CHECKER-REBUILD — the pc-color field's GPU dials (the wheel's
// COMMIT_CURVE / RECEPTIVITY_FLOOR / STRENGTH are retired):
//   CHECKER_WANDER — S2, the between-patch spread. Each region's median
//     is placed AROUND the resultant by a STATIC per-region hue-space
//     offset (seeded on the region node ONLY — one fixed personality per
//     region, held for the whole session), scaled by presence. 0 = every
//     region sits exactly on the resultant. All time-variation is CPU-
//     enveloped; no GPU-side time stepping (a stepped hash teleports).
const CHECKER_WANDER: f32 = 0.12;
// S3 within-patch spread (hot-reloadable): the CPU ships the enveloped
// distinct-pc-count surplus (music_variance = glided max(0, n-1)); the
// GPU scales it here and adds it to each region's seed spread. 0 or 1
// distinct → 0 extra. Tune by eye.
const CHECKER_VAR_PER_NOTE: f32 = 0.025;   // seed-var add per distinct pc beyond the first
const CHECKER_VAR_MAX: f32 = 0.30;         // ceiling on the music spread add
// DOOR FADE WIDTHS — roll-band half-widths (flip→glide, charter C2/2i).
// Ruled 0.07 (Movement 1). _ZONE softens the static sparse-territory mask.
const DOOR_FADE_W_SCATTER: f32 = 0.07;
const DOOR_FADE_W_SPARSE: f32 = 0.07;
const DOOR_FADE_W_ZONE: f32 = 0.07;
// CHECKER_DEBUG_VIEW — the institutionalized instrument (hot-reload):
//   0 = the art. 1 = WHEEL METER: all ground painted by the wheel as
//   the shader receives it (angle → hue on the same recipe as the
//   seat table: 0° red · 60° yellow · 240° blue; length → vividness).
//   Static gray under music = the CPU→GPU path is cut. 2 = the
//   FIELD-COVERAGE view (re-pointed Phase 1; receptivity map retired
//   with prop 804): green = live path, gray = baked composite.
//   (Phase-2 TIER VIEW retired — VOICE-COHERENCE shot served; the
//   one-address law verified.)
//   Branches on a module const — folded out at 0, zero cost.
const CHECKER_DEBUG_VIEW: u32 = 0u;
// TERRAIN_DEBUG_VIEW — the registry's terrain slots (target layout:
// 0 art · 1 wheel meter · 2 coverage · 3 skirt · 4 zone geometry;
// full single-registry migration is Phase 4). 0 = off. 3 = SKIRT
// PAINT (permanent): skirt fragments magenta. 4 = ZONE-GEOMETRY
// SCULPTING ROOM (permanent): live mode field grayscale + patch
// border lines + red coastline isoline — the warp tuning view.
// (View 5, the sliver microscope, RETIRED — INCIDENT #3b CLOSED:
// term 1 convicted the wander's 80-unit floor; the quantum fix and
// the sample-point law landed; the LUT retired with the incident.
// INCIDENT #2's I1/I2 audits retired earlier — suspects exonerated.)
const TERRAIN_DEBUG_VIEW: u32 = 0u;

// ── ROW 6 — RETIRED (Phase 1, ruling 6) ───────────────────────────
// Terrain-mode coupling. The magnitude dial was parked at 0.0 — every
// dial here multiplied to zero. The row retired whole with its
// lattice (9: seeds 15/16, props 530/531) and evaluators (§1.6). If
// terrain-mode coupling ever returns, it returns as a FIELD under the
// SEAMLESSNESS treaty, not as this mechanism. Ledger: the charter.

// ── ROW 7 — THE MOVEMENT THIRD ──────────────────────────────────────
// The surface voice's motion vocabulary (moved here from §1.6 —
// TERRAIN_LOOKS gather; values unchanged). REST pins live in the C++
// room (ROW 2): terrain_time ≤ 0 freezes both overlay evaluators —
// rest IS today's stillness; band blend -1 = inactive.

// Activity envelope — the authorless static field that gates band
// motion (its seed plumbing — ACTIVITY_SEED_BAND / ACTIVITY_PROP_* —
// stays with the field fn at §1.6).
const ACTIVITY_LATTICE_SPACING: f32 = 400.0;    // world units between activity nodes
const ACTIVITY_BEAT_FREQ_LO: f32    = 0.25;     // lowest beat frequency (cycles/beat)
const ACTIVITY_BEAT_FREQ_HI: f32    = 2.0;      // highest beat frequency (cycles/beat)

// Per-band activity thresholds — which terrain bands respond at what intensity.
// Index matches TERRAIN_BANDS: 0=continental, 5=tectonic.
const WAVE_THRESHOLD = array<f32, 6>(
    0.85,  // 0: continental — only the most active pools move the bones
    0.70,  // 1: regional
    0.50,  // 2: local
    0.35,  // 3: detail
    0.20,  // 4: fine — responds to even mild pools
    0.90,  // 5: tectonic — almost geological, only extreme activity animates
);
const WAVE_THRESHOLD_SOFTNESS: f32 = 0.15;      // crossfade width at threshold boundary

// ─── The Movement Third — THE TRUE BANDS (TRUEBAND_CONTACT_1) ───────
// The overlay matrix is RETIRED: the terrain animates with its OWN
// waves — the card writer's heights pass sums per-band
// true_band_delta_contribution (TERRAIN_BANDS, §1.5), shaped by the
// WAVE_THRESHOLD pools above and gated by get_band_blend's per-band
// wires. WAVE_THRESHOLD + softness STAY — they are the true-band pool
// thresholds.

// ── ROW 8 — GOVERNING EXPRESSIONS ───────────────────────────────────
// The palette's governing expression lives in-room (below). The
// composite's governing contract — composite_cell_color(id, discrete):
// blend smooth→discrete at the ROW 5 mode edges; scatter-survive by
// cell_roll; mix the two styles by the transition field; sparse cells
// survive outside mode zones. Phase 1: the doors are FIELD outputs
// (door_values, fed from evaluate_cell_fields — the ONE derivation,
// bake and live alike since Commit C retired the LUT reconstruction;
// the biased twin deleted at Phase 2).

// Smooth palette color: weighted blend modulated by complexity only.
// No per-cell noise — produces continuous gradients. (Relocated from
// the field-function neighborhood — TERRAIN_LOOKS gather.)
fn palette_color_smooth(weights: vec4<f32>, complexity: f32) -> vec3<f32> {
    var color = vec3(0.0);
    let w = array<f32, 4>(weights.x, weights.y, weights.z, weights.w);
    for (var i: u32 = 0u; i < 4u; i++) {
        color += mix(config.palette_light[i].rgb, config.palette_center[i].rgb, complexity) * w[i];
    }
    return clamp(color, vec3(0.0), vec3(1.0));
}

// ── ROW 9 — THE CONTRIBUTOR ROSTER (pointers; navigable, NOT annexed)─
//   Static landform: CONTRIBUTOR_DAG / POLICIES[] — the §3.4 Ground
//     Architecture seam (CPU twin: contracts/ground_architecture.hpp).
//   GoL zone tint: gol_composite_cell_color (§7.0b) — reads ROW 1/3
//     through palette_color_smooth; GoL/pulse keep their own panel.
//   Radial pulses (music-onset rings): the PULSE_SPEED..PULSE_AGE_DECAY
//     dials directly above fn contrib_radial_pulses_at.
//   Pawn aura tint: pawn_aura_cfg.tint_strength — a uniform field of
//     the aura system, NOT this panel's DISCRETE_TINT_STRENGTH.
//   Population (what stands on the surface): population_themes.hpp +
//     the spawn panel (compose_spawn_chance).
// ═══════════════════════════════════════════════════════════ end §2.2

// --- Pawn constants

const PAWN_HEIGHT: f32 = 1.5;
const PAWN_RADIUS: f32 = 0.5;
const PAWN_SPEED: f32 = 15.0;
// Max height the pawn can "step up" in a single frame.
// Anything higher is treated as a solid wall (XZ motion is blocked).
const PAWN_STEP_HEIGHT: f32 = 0.5;
const PAWN_TURN_SPEED: f32 = 8.0;

// Chess pawn mesh resolution (GPU-generated from vertex_index)
const PAWN_SEGMENTS: u32 = 48u;
const PAWN_RINGS: u32 = 16u;
const PAWN_BODY_VERTICES: u32 = (PAWN_RINGS - 1u) * PAWN_SEGMENTS * 6u;  // 15 × 48 × 6 = 4320

// --- Camera constants

const CAMERA_FOV: f32 = 1.0;
const CAMERA_MIN_DISTANCE: f32 = 5.0;
const CAMERA_MAX_DISTANCE: f32 = 100.0;
const CAMERA_MIN_ELEVATION: f32 = -0.5;
const CAMERA_MAX_ELEVATION: f32 = 1.5;

// --- FPV (First-Person View) constants

const FPV_EYE_HEIGHT: f32 = PAWN_HEIGHT + 0.2;  // Camera at eye level
const FPV_MIN_ELEVATION: f32 = -1.4;             // Look down ~80°
const FPV_MAX_ELEVATION: f32 = 1.5;              // Look up ~86°

// --- Floating entity constants
// Per-entity parameters (radius, orbit_radius, orbit_height, orbit_speed,
// influence_radius, base_color) now live in FloatingEntityState fields.
// Buffer layout: slots 0..7 = spheres (orbital), slots 8..263 = cubes (hover-bob).
// MUST match Dim::MAX_SPHERE_INSTANCES, Dim::MAX_CUBE_INSTANCES,
// Dim::CUBE_SLOT_OFFSET, Dim::TOTAL_FLOATING_SLOTS in state.hpp.

const SPHERE_SLOT_COUNT: u32 = 8u;
const CUBE_SLOT_OFFSET: u32 = 8u;
const CUBE_SLOT_COUNT: u32 = 256u;
const TOTAL_FLOATING_SLOTS: u32 = 264u;

const SPHERE_COLOR_RELEASE_RATE: f32 = 2.0;
const SPHERE_MIN_TERRAIN_CLEARANCE: f32 = 5.0;

// Cubes bob and now drift via Phase-3 behaviors; the drift integrator
// can pull pos below ground if a behavior pushes drift.y negative
// faster than the spring restores it. update_cube clamps drift.y from
// below to keep the cube's base (center minus half-height) at least this
// far above local ground.
const CUBE_TERRAIN_CLEARANCE: f32 = 3.0;

// Bindings 21 and 40 reserved (currently unused — kept open for
// future cell-system features).

// --- Cell Behavior Tag Encoding
const CELL_ANIM_GOL:  u32 = 1u;     // bit 0: Game of Life
// Future: CELL_ANIM_PAWN = 2u, CELL_ANIM_SPHERE = 4u, CELL_ANIM_HUE = 8u

fn pack_cell_tag(mode: u32, tier: u32, height_enabled: bool) -> f32 {
    let tag = mode | ((tier & 0x7u) << 4u) | (select(0u, 1u, height_enabled) << 7u);
    return f32(tag) / 255.0;
}

fn unpack_cell_tag_mode(alpha: f32) -> u32 {
    return u32(alpha * 255.0 + 0.5) & 0xFu;
}

fn unpack_cell_tag_tier(alpha: f32) -> u32 {
    return (u32(alpha * 255.0 + 0.5) >> 4u) & 0x7u;
}

fn unpack_cell_tag_height(alpha: f32) -> bool {
    return (u32(alpha * 255.0 + 0.5) & 0x80u) != 0u;
}

// --- Game of Life Zone Config
const GOL_ZONE_SEED_BAND: u32      = 250u;      // lattice seed for zone decisions
const GOL_ZONE_PROP_SPAWN: u32     = 920u;      // spawn roll
const GOL_ZONE_PROP_TIER: u32      = 921u;      // tier selection roll
const GOL_ZONE_PROP_HEIGHT: u32    = 922u;      // height factor roll

const GOL_ZONE_SPAWN_CHANCE: f32   = 0.15;      // 15% of checkerboard zones
const GOL_ZONE_HEIGHT_CHANCE: f32  = 0.30;      // 30% of GoL zones get height
const GOL_ZONE_MODE_THRESHOLD: f32 = 0.50;      // min interpolated mode for eligibility
                                                  // (above scatter_edge, well into discrete)
struct GoLTierParams {
    // --- Initial conditions
    density_mean: f32,             // fraction alive at spawn
    density_sigma: f32,

    // --- Temporal
    tick_period_mean: f32,         // beats between generations
    tick_period_sigma: f32,

    // --- Visual transition
    spring_stiffness_mean: f32,    // spring constant for alive↔dead
    spring_stiffness_sigma: f32,
    transition_fraction_mean: f32, // fraction of tick_period for spring transition
    transition_fraction_sigma: f32,

    // --- Height
    alive_height_mean: f32,        // terrain rise when alive (world units)
    alive_height_sigma: f32,

    // --- Per-cell variation
    spring_variance: f32,          // [0,1] per-cell spring speed scatter

    // --- Selection
    weight: f32,                   // tier probability (must sum to 1.0)
    force_no_height: u32,          // 1 = height always disabled for this tier

    // --- Size (UNIFIED_GROUND_1 U5; cells, not world units)
    grid_cells: u32,               // zone side in cells ∈ {8..32} (Jean-tunable)
}

const GOL_TIER_COUNT: u32 = 7u;

//                                                 dens_μ  σ     tick_μ  σ    spring_μ σ    trans_μ  σ     ht_μ    σ    sv    wt    no_h  cells
// (cells column: UNIFIED_GROUND_1 U5 — authored defaults by weight
//  order thirds, 32/24/16; Jean-tunable per row.)
const GOL_TIERS = array<GoLTierParams, 7>(
    /* 0: PILLARS  */ GoLTierParams(0.30, 0.05,   8.0, 2.0,   0.5, 0.1,   0.05, 0.01,  30.0, 9.0,  0.30,  0.10, 0u, 16u),
    /* 1: SPARSE   */ GoLTierParams(0.15, 0.05,   2.0, 0.5,   4.0, 1.0,   0.12, 0.03,  18.0, 6.0,  0.20,  0.20, 0u, 32u),
    /* 2: MODERATE */ GoLTierParams(0.30, 0.08,   1.0, 0.3,   8.0, 2.0,   0.15, 0.03,   9.0, 3.0,  0.15,  0.18, 0u, 32u),
    /* 3: DENSE    */ GoLTierParams(0.45, 0.10,   0.5, 0.15, 12.0, 3.0,   0.25, 0.05,   6.0, 1.5,  0.10,  0.10, 0u, 16u),
    /* 4: FLASH    */ GoLTierParams(0.35, 0.10,  0.25, 0.05, 20.0, 5.0,   0.30, 0.05,   0.0, 0.0,  0.40,  0.17, 1u, 24u),
    /* 5: MONOLITH */ GoLTierParams(0.20, 0.03,  12.0, 3.0,   0.3, 0.05,  0.03, 0.01,  42.0, 12.0, 0.05,  0.12, 0u, 16u),
    /* 6: GLACIER  */ GoLTierParams(0.12, 0.03,   4.0, 1.0,   2.0, 0.5,   0.08, 0.02,  24.0, 7.5,  0.25,  0.13, 0u, 24u),
);

// --- Pulse Algorithm Tier Definitions ────────────────────────────────────
//
// Pulse zones: periodic breathing of cell color/height, no neighbor rules.
// Each cell oscillates between terrain base and a displaced target.
// CPU selects tier and uploads parameters via GoLZoneConfig; these
// definitions have a live CPU twin in bodies/gol_zones.hpp (GOL_TIERS
// / GOL_PULSE_TIERS) — the GPU renders from these, the CPU seeds/ticks
// from the twin, so both are authoritative and a tuner must edit both.
//
// Algorithm and boundary mode constants (shared CPU ↔ GPU):
//   GOL_ALGORITHM_CONWAY = 0   GOL_ALGORITHM_PULSE = 1
//   GOL_BOUNDARY_REFLECT = 0   GOL_BOUNDARY_WRAP   = 1
// (defined below in the GoL zone system section)

struct GolPulseTierParams {
    // --- Temporal
    tick_period_mean: f32,
    tick_period_sigma: f32,
    // --- Visual transition
    spring_stiffness_mean: f32,
    spring_stiffness_sigma: f32,
    transition_fraction_mean: f32,
    transition_fraction_sigma: f32,
    // --- Phase scatter
    phase_randomness_mean: f32,
    phase_randomness_sigma: f32,
    // --- Tempo scatter
    tempo_randomness_mean: f32,
    tempo_randomness_sigma: f32,
    // --- Height
    alive_height_mean: f32,
    alive_height_sigma: f32,
    // --- Wander
    wander_radius_mean: f32,
    wander_radius_sigma: f32,
    // --- Per-cell variation
    spring_variance: f32,
    // --- Selection
    weight: f32,
    force_no_height: u32,     // 0 = allow height, 1 = force no height
    boundary_mode: u32,       // 0 = reflect, 1 = wrap
    // --- Size (UNIFIED_GROUND_1 U5; cells, not world units)
    grid_cells: u32,          // zone side in cells (Jean-tunable)
}

const GOL_PULSE_TIER_COUNT: u32 = 3u;

//                                                       tick_μ   σ    spring_μ σ    trans_μ  σ    phase_μ  σ    tempo_μ σ    ht_μ   σ    wand_μ  σ    sv    wt    no_h  bnd  cells
// (cells column: UNIFIED_GROUND_1 U5 — 32/16/8 by weight order; Jean-tunable.)
const GOL_PULSE_TIERS = array<GolPulseTierParams, 3>(
    /* 0: Breathe  */ GolPulseTierParams( 2.0, 0.5,   4.0, 1.0,   0.20, 0.05,   0.15, 0.05,   0.10, 0.03,   2.0, 0.8,  10.0, 3.0,   0.20,  0.45, 0u, 0u, 32u ),
    /* 1: Sparkle  */ GolPulseTierParams( 0.5, 0.15, 12.0, 3.0,   0.25, 0.05,   0.90, 0.10,   0.60, 0.15,   0.0, 0.0,   5.0, 2.0,   0.50,  0.30, 1u, 0u, 16u ),
    /* 2: Drift    */ GolPulseTierParams( 4.0, 1.0,   1.5, 0.4,   0.10, 0.03,   0.50, 0.15,   0.40, 0.10,   4.0, 1.5,  25.0, 8.0,   0.35,  0.25, 0u, 1u, 8u ),
);

// Probability of a zone being Pulse (vs Conway) — must match CPU GOL_PULSE_ALGORITHM_CHANCE.
const GOL_PULSE_ALGORITHM_CHANCE: f32 = 0.35;

// --- Pawn Safety Force Field
const PAWN_FORCEFIELD_ENABLED: bool = true;

// --- Compile-time feature gates
// These prune heavy dependency chains from update_player_agent's pipeline compilation.
// Set to false to cut compile time when iterating on unrelated features.
const PAWN_GOL_GROUND_ENABLED: bool = false;    // Pawn walks on GoL extrusions
const PAWN_FORCEFIELD_RADIUS_STATIONARY: f32 = 6.0;  // Radius when not moving
const PAWN_FORCEFIELD_RADIUS_MOVING: f32 = 2.0;      // Radius at max speed
const PAWN_FORCEFIELD_FALLOFF: f32 = 2.0;            // Edge softness (smoothstep width)
const PAWN_FORCEFIELD_SPEED_SCALE: f32 = 1.0;        // How quickly radius shrinks with speed


// §2.3 MUTING CONTROL

// --- Coupling bit flags

const COUPLING_POLYPHONY_TO_AMPLITUDE:       u32 = 1u << 0u;
const COUPLING_TERRAIN_TO_PAWN_Y:            u32 = 1u << 1u;
const COUPLING_TERRAIN_TO_PAWN_TILT:         u32 = 1u << 2u;
const COUPLING_PAWN_TO_CAMERA_TARGET:        u32 = 1u << 3u;
const COUPLING_INPUT_MOVES_PLAYER:           u32 = 1u << 4u;
const COUPLING_INPUT_ORBITS_CAMERA:          u32 = 1u << 5u;
const COUPLING_INPUT_ZOOMS_CAMERA:           u32 = 1u << 6u;
const COUPLING_PAWN_TO_PROXIMITY_FIELD:      u32 = 1u << 7u;   // (reserved — legacy proximity field)
const COUPLING_SPHERE_TO_PROXIMITY_FIELD:    u32 = 1u << 8u;   // (reserved — legacy proximity field)
const COUPLING_POLYPHONY_TO_CELL_COLOR:      u32 = 1u << 9u;   // (reserved — legacy cell system)
const COUPLING_PAWN_TO_CELL_COLOR:           u32 = 1u << 10u;  // (reserved — legacy cell system)
const COUPLING_SPHERE_TO_CELL_COLOR:         u32 = 1u << 11u;  // (reserved — legacy cell system)
const COUPLING_POLYPHONY_TO_SPHERE_COLOR:    u32 = 1u << 12u;
const COUPLING_SPHERE_TO_TERRAIN_TINT:       u32 = 1u << 13u;
const COUPLING_TERRAIN_TO_SPHERE_HEIGHT:     u32 = 1u << 14u;
const COUPLING_RANDOM_TO_CELL_GOALS:         u32 = 1u << 15u;  // (reserved — legacy cell system)
const COUPLING_PAWN_TO_SUN_VP:               u32 = 1u << 16u;
const COUPLING_PAWN_TO_ZONE_HEIGHT:          u32 = 1u << 17u;  // pawn suppresses zone extrusion
const COUPLING_PAWN_TO_ZONE_COLOR:           u32 = 1u << 18u;  // pawn tints zone cell color
const COUPLING_SPHERE_TO_ZONE_HEIGHT:        u32 = 1u << 19u;  // sphere suppresses zone extrusion
const COUPLING_SPHERE_TO_ZONE_COLOR:         u32 = 1u << 20u;  // sphere tints zone cell color

// --- Muting query functions

fn coupling_active(bit: u32) -> bool {
    return (config.mute_couplings & bit) == 0u;
}

fn dynamics_0d_active() -> bool {
    return config.mute_dynamics_0d == 0u;
}

fn signal_active() -> bool {
    return config.mute_signal == 0u;
}

fn sphere_frozen() -> bool {
    return config.freeze_sphere != 0u;
}

fn fpv_mode_active() -> bool {
    return config.fpv_mode != 0u;
}

// The point's host flag (contracts/point.hpp mirror):
// true = the CAMERA hosts the point (free-fly); false = the pawn
// hosts (the kite — every body-path read below is unchanged).
fn point_camera_hosted() -> bool {
    return config.point_host == 1u;
}


// §3 COUPLINGS

// §3.1 signal → terrain

// --- [COUPLING:signal.polyphony→terrain:amplitude]
// DRIVERLESS since gen-1 retirement (the 8th capability — raw
// signal.stats[0] terrain-amplitude coupling, retired M1-C). Revive
// only as a deliberately designed gen-2 idiom; direct shader reads of
// the signal bypass canvas and bank (sovereignty decision, parked).

// RAYMARCH/SDF EXCAVATION: coupling_signal_polyphony_to_terrain_amplitude
// removed — the amplitude-trajectory feeder into the dead lipschitz limb
// (its only consumer was update_terrain_config, also removed).

// §3.2 signal → entities

// --- [COUPLING:signal.polyphony→sphere:color]
fn coupling_signal_polyphony_to_sphere_color(polyphony: f32, current: vec3<f32>, base_color: vec3<f32>, dt: f32) -> vec3<f32> {
    let intensity = saturate(polyphony / 8.0);
    
    // --- HYBRID APPROACH
    if (intensity < 0.01) {
        // Silent: smooth return to base color using existing release rate
        return current + (base_color - current) * (1.0 - exp(-SPHERE_COLOR_RELEASE_RATE * dt));
    }
    
    // Active: PGA spiral (rotation scales with intensity, no idle drift)
    let hue_speed = intensity * 2.5;
    let sat_push = intensity * 0.5;
    let val_climb = intensity * 0.2;
    
    return pga_color_motor(current, hue_speed, sat_push, val_climb, dt);
}

// §3.3 input → entities

// --- [COUPLING:input→pawn:velocity]

fn coupling_input_to_pawn_velocity(input_dir: vec2<f32>, camera_azimuth: f32) -> vec2<f32> {
    let cos_az = cos(camera_azimuth);
    let sin_az = sin(camera_azimuth);
    return vec2(
        input_dir.x * cos_az + input_dir.y * sin_az,
        -input_dir.x * sin_az + input_dir.y * cos_az
    );
}

// --- [COUPLING:input→camera:distance]

fn coupling_input_to_camera_distance(zoom_delta: f32, cam: CameraState) -> CameraState {
    var c = cam;
    c.distance = clamp(c.distance + zoom_delta, CAMERA_MIN_DISTANCE, CAMERA_MAX_DISTANCE);
    return c;
}

// --- [COUPLING:input→camera:pan]

fn coupling_input_to_camera_pan(pan_delta: vec2<f32>, cam: CameraState) -> CameraState {
    var c = cam;
    c.pan_x += pan_delta.x * cam.distance * 0.5;
    c.pan_y += pan_delta.y * cam.distance * 0.5;
    return c;
}

// §3.4 terrain → entities

// --- [COUPLING:pawn_forcefield]
const SPHERE_FORCEFIELD_RADIUS: f32 = 10.0;
const SPHERE_FORCEFIELD_FALLOFF: f32 = 3.0;

// --- Parameterized force field functions (binding-independent)
fn zone_pawn_ff(world_xz: vec2<f32>, pawn_pos: vec3<f32>, pawn_vel: vec2<f32>) -> f32 {
    if (!PAWN_FORCEFIELD_ENABLED) { return 1.0; }
    let pawn_dist = length(world_xz - pawn_pos.xz);
    let pawn_speed = length(pawn_vel);
    let speed_factor = saturate(pawn_speed * PAWN_FORCEFIELD_SPEED_SCALE / PAWN_SPEED);
    let radius = mix(PAWN_FORCEFIELD_RADIUS_STATIONARY, PAWN_FORCEFIELD_RADIUS_MOVING, speed_factor);
    return smoothstep(radius - PAWN_FORCEFIELD_FALLOFF, radius + PAWN_FORCEFIELD_FALLOFF, pawn_dist);
}

fn zone_sphere_ff(world_xz: vec2<f32>, sphere_pos: vec3<f32>) -> f32 {
    let dist = length(world_xz - sphere_pos.xz);
    return smoothstep(
        SPHERE_FORCEFIELD_RADIUS - SPHERE_FORCEFIELD_FALLOFF,
        SPHERE_FORCEFIELD_RADIUS + SPHERE_FORCEFIELD_FALLOFF,
        dist
    );
}

// --- Zone force field tint parameters
const ZONE_PAWN_TINT: vec3<f32> = vec3(0.4, 0.2, 0.5);     // purple shift near pawn
const ZONE_SPHERE_TINT: vec3<f32> = vec3(0.5, 0.35, 0.0);  // gold shift near sphere
const ZONE_PAWN_TINT_STRENGTH: f32 = 0.6;
const ZONE_SPHERE_TINT_STRENGTH: f32 = 0.5;

// --- Pawn GoL suppression radii (shared between height_at + extrusion VS)
const ZONE_SUPPRESS_INNER: f32 = 4.0;   // full suppression inside this radius
const ZONE_SUPPRESS_OUTER: f32 = 15.0;  // zero suppression beyond this radius

// THE ONE SUPPRESSION FORM (UNIFIED_GROUND_1) — the walker's exact
// inline shape, extracted. pawn_xz is the caller's stage-appropriate
// point source: compute passes qi.consumer_pos.xz, render passes
// render_pawn_pos().xz. Returns the suppression FACTOR (1 at the
// pawn, 0 beyond OUTER).
fn pawn_gol_suppression(world_xz: vec2<f32>, pawn_xz: vec2<f32>) -> f32 {
    return 1.0 - smoothstep(ZONE_SUPPRESS_INNER, ZONE_SUPPRESS_OUTER,
                            distance(world_xz, pawn_xz));
}

// --- [COUPLING:cells→terrain:height]
const PIER_TOTAL: u32 = 68u;

struct PierInstance {
    origin:      vec2<f32>,   // world XZ center of footprint
    half_size:   vec2<f32>,   // half-extent in rotated local X and Z
    height_near: f32,         // height delta at local −X edge
    height_far:  f32,         // height delta at local +X edge
    rotation:    f32,         // Y-axis rotation (radians, 0 = world +X)
    edge_blend:  f32,         // smoothstep transition width (world units)
    tier:        u32,         // pier tier (metadata — not read by evaluation)
    is_active:   u32,         // 0 = inactive, contributes nothing
    _pad0:       u32,
    _pad1:       u32,
};

@group(0) @binding(26) var<storage, read> pier_instances: array<PierInstance, 68>;

// Evaluate a single pier instance at a world position.
// Returns the height delta (0 outside footprint or inactive, blended at edges).
fn evaluate_pier(world_xz: vec2<f32>, inst: PierInstance) -> f32 {
    if (inst.is_active == 0u) { return 0.0; }

    // Transform to local coordinates (rotate by −rotation)
    let d = world_xz - inst.origin;
    let c = cos(-inst.rotation);
    let s = sin(-inst.rotation);
    let local = vec2(d.x * c - d.y * s, d.x * s + d.y * c);

    let hx = inst.half_size.x;
    let hz = inst.half_size.y;

    // Early reject: well outside footprint + blend zone
    let blend = max(inst.edge_blend, 0.0);
    if (abs(local.x) > hx + blend || abs(local.y) > hz + blend) {
        return 0.0;
    }

    // Smooth footprint mask: 1.0 inside, 0.0 outside, smooth at edges
    var mask = 1.0;
    if (blend > 0.001) {
        let fx_lo = smoothstep(-hx - blend, -hx + blend, local.x);
        let fx_hi = 1.0 - smoothstep(hx - blend, hx + blend, local.x);
        let fz_lo = smoothstep(-hz - blend, -hz + blend, local.y);
        let fz_hi = 1.0 - smoothstep(hz - blend, hz + blend, local.y);
        mask = fx_lo * fx_hi * fz_lo * fz_hi;
    } else {
        // Hard edge: boolean inside/outside
        if (abs(local.x) > hx || abs(local.y) > hz) {
            return 0.0;
        }
    }

    if (mask < 0.001) { return 0.0; }

    // Height interpolation along local X: near at −hx, far at +hx
    let t = clamp((local.x + hx) / max(2.0 * hx, 0.001), 0.0, 1.0);
    let raw_h = mix(inst.height_near, inst.height_far, t);

    return raw_h * mask;
}

// Evaluate all active pier instances, return the max height delta at world_xz.
// Loop bounded by config.pier_count (highest active slot + 1) to keep FXC happy.
fn structure_height_at(world_xz: vec2<f32>) -> f32 {
    var best = 0.0;
    let count = min(config.pier_count, PIER_TOTAL);
    for (var i = 0u; i < count; i++) {
        let h = evaluate_pier(world_xz, pier_instances[i]);
        best = max(best, h);
    }
    return best;
}

// --- [DATA-DRIVEN PYRAMID INSTANCES]
const MAX_PYRAMID_INSTANCES: u32 = 8u;

struct PyramidInstance {
    origin_x: f32,
    origin_z: f32,
    half_size_x: f32,
    half_size_z: f32,
    height: f32,
    rotation: f32,
    edge_blend: f32,
    truncation: f32,
}

struct PyramidArray {
    count: u32,
    _pad0: u32,
    _pad1: u32,
    _pad2: u32,
    instances: array<PyramidInstance, 8>,
}

@group(0) @binding(30) var<uniform> pyramid_instances: PyramidArray;

fn evaluate_pyramid(world_xz: vec2<f32>, inst: PyramidInstance) -> f32 {
    let dx = world_xz.x - inst.origin_x;
    let dz = world_xz.y - inst.origin_z;
    let c = cos(-inst.rotation);
    let s = sin(-inst.rotation);
    let lx = dx * c - dz * s;
    let lz = dx * s + dz * c;

    let hx = inst.half_size_x;
    let hz = inst.half_size_z;
    let blend = max(inst.edge_blend, 0.0);

    let abs_lx = abs(lx);
    let abs_lz = abs(lz);

    // Outside extended footprint: no contribution
    if (abs_lx > hx + blend || abs_lz > hz + blend) { return 0.0; }

    // Edge blend mask (same smoothstep approach as solids)
    var mask: f32 = 1.0;
    if (blend > 0.001) {
        let fx_lo = smoothstep(-hx - blend, -hx + blend, lx);
        let fx_hi = 1.0 - smoothstep(hx - blend, hx + blend, lx);
        let fz_lo = smoothstep(-hz - blend, -hz + blend, lz);
        let fz_hi = 1.0 - smoothstep(hz - blend, hz + blend, lz);
        mask = fx_lo * fx_hi * fz_lo * fz_hi;
    } else {
        if (abs_lx > hx || abs_lz > hz) { return 0.0; }
    }
    if (mask < 0.001) { return 0.0; }

    // Chebyshev distance: 0 at center, 1 at base edge
    let chebyshev = max(abs_lx / max(hx, 0.001), abs_lz / max(hz, 0.001));

    // Taper: full height at center, zero at base edge
    // Truncation: flat top from center out to truncation fraction
    let taper = clamp((1.0 - chebyshev) / max(1.0 - inst.truncation, 0.001), 0.0, 1.0);

    return inst.height * taper * mask;
}

// CONTRIB_PYRAMIDS — static_landform, global.
// Contributes: tapered height of the tallest active pyramid covering xz.
// Dependencies (via DAG): CONTRIB_TERRAIN_LATTICE, CONTRIB_TILE_MODIFIERS, CONTRIB_SOLIDS.
fn contrib_pyramids_at(world_xz: vec2<f32>) -> f32 {
    var best: f32 = 0.0;
    let count = min(pyramid_instances.count, MAX_PYRAMID_INSTANCES);
    for (var i = 0u; i < count; i++) {
        let h = evaluate_pyramid(world_xz, pyramid_instances.instances[i]);
        best = max(best, h);
    }
    return best;
}

// CONTRIB_GOL_ZONES — slow_dynamic, global.
// Contributes: raw GoL cell extrusion (visual × alive_height × per-cell factor).
// Dependencies (via DAG): none — composes onto the static stack additively.
// Notes: no consumer-local suppression here; that is contrib_gol_suppression_at.
fn contrib_gol_zones_at(world_xz: vec2<f32>) -> f32 {
    for (var z: u32 = 0u; z < zone_config.count; z++) {
        let zp = zone_config.zones[z];
        if (zp.transition_fraction <= 0.0) { continue; }
        if (zp.alive_height < 0.01) { continue; }

        let zone_corner = zp.origin - zp.extent * 0.5;
        let cell_size = zp.extent / f32(zp.grid_size);
        let rel = world_xz - zone_corner;
        let cx = i32(floor(rel.x / cell_size));
        let cy = i32(floor(rel.y / cell_size));

        if (cx < 0 || cx >= i32(zp.grid_size) || cy < 0 || cy >= i32(zp.grid_size)) { continue; }

        let base = z * GOL_ZONE_STRIDE;
        let idx = u32(cy) * zp.grid_size + u32(cx);
        let visual = zone_life[base + GOL_CELL_VISUAL + idx];
        let height_factor = zone_life[base + GOL_CELL_HEIGHT_FACTOR + idx];
        return visual * zp.alive_height * height_factor * config.mode_gol_height_scale;
    }
    return 0.0;
}

// CONTRIB_GOL_SUPPRESSION — deformation_field, consumer-local (subtractive).
// Contributes: subtractive GoL height that flattens the zone near consumer_pos.
// Dependencies (via DAG): none — orthogonal to the static stack.
// Notes: returns h * (1 - smoothstep(SUPPRESS_INNER, SUPPRESS_OUTER, dist)),
//   so contrib_gol_zones_at(xz) - contrib_gol_suppression_at(xz, pos)
//   == h * smoothstep(...), matching the pre-refactor h *= (1 - supp).
//   Evaluates raw GoL internally (calls contrib_gol_zones_at).
//
// USED ONLY BY: standalone consumers that want the subtractive form as
// a separate value. The walker-family queries (query_ground_walker,
// query_ground_walker_pair) inline the GoL + suppression math together
// to avoid double-evaluating the zone loop, which compounds
// significantly under FXC loop unrolling. New walker-side consumers
// should do the same.
// STATUS: LATENT[policy-surface] — the standalone form has zero callers
// today (the contributor is realized inline in walker/tilt/pair per the
// FXC fusion above); kept as the reference form for any consumer that
// wants suppression as a separate value.
fn contrib_gol_suppression_at(world_xz: vec2<f32>, consumer_pos: vec3<f32>) -> f32 {
    let h = contrib_gol_zones_at(world_xz);
    return h * pawn_gol_suppression(world_xz, consumer_pos.xz);
}

// --- Ground Architecture: contributor and policy ids ---
//
// Mirror of contracts/ground_architecture.hpp. Shader code references
// contributors and policies by these symbols. The canonical ids and
// policy bitmasks live on the C++ side; these consts exist so WGSL
// can refer to the same numeric values. Keep in sync with POLICIES[]
// in that header.

const CONTRIB_TERRAIN_LATTICE   : u32 = 0u;
const CONTRIB_TILE_MODIFIERS    : u32 = 1u;
const CONTRIB_SOLIDS            : u32 = 2u;
const CONTRIB_PYRAMIDS          : u32 = 3u;
const CONTRIB_PAINTINGS_BASES   : u32 = 4u;
const CONTRIB_VEGETATION_BASES  : u32 = 5u;
const CONTRIB_GOL_ZONES         : u32 = 6u;
const CONTRIB_TERRAIN_WAVES     : u32 = 7u;
const CONTRIB_RADIAL_PULSES     : u32 = 8u;
const CONTRIB_PAWN_AURA         : u32 = 9u;
const CONTRIB_GOL_SUPPRESSION   : u32 = 10u;
const CONTRIB_COUNT             : u32 = 11u;

// Policy IDs — the manifold interface's policy selector (b1).
// MUST mirror enum PolicyId in ground_architecture.hpp
// byte-for-byte (same order/values); the masks above are per-policy
// contributor sets, these are the discrete ids manifold_resolve
// switches on.
const POLICY_PLACEMENT_PYRAMID    : u32 = 0u;
const POLICY_PLACEMENT_PAINTING   : u32 = 1u;
const POLICY_PLACEMENT_VEGETATION : u32 = 2u;
const POLICY_BAKED_HEIGHTFIELD    : u32 = 3u;
const POLICY_FLYER                : u32 = 4u;
const POLICY_WALKER               : u32 = 5u;
const POLICY_WALKER_TILT          : u32 = 6u;
const POLICY_WALKER_AGENT         : u32 = 7u;
const POLICY_CELESTIAL            : u32 = 8u;
const POLICY_TERRAIN_RENDER       : u32 = 9u;

// Fused-static-base bitmask: lattice + tile_modifiers + solids travel
// together in every policy that wants a landform base.
const GROUND_STATIC_BASE_MASK: u32 =
    (1u << CONTRIB_TERRAIN_LATTICE)
  | (1u << CONTRIB_TILE_MODIFIERS)
  | (1u << CONTRIB_SOLIDS);

// Per-policy contributor masks — kept in sync with POLICIES[] in
// contracts/ground_architecture.hpp. The query specializations in
// Step 3 select their contributors at compile time from these.
const POLICY_PLACEMENT_PYRAMID_MASK    : u32 = GROUND_STATIC_BASE_MASK;
const POLICY_PLACEMENT_PAINTING_MASK   : u32 = GROUND_STATIC_BASE_MASK
                                              | (1u << CONTRIB_PYRAMIDS)
                                              | (1u << CONTRIB_GOL_ZONES);
const POLICY_PLACEMENT_VEGETATION_MASK : u32 = GROUND_STATIC_BASE_MASK;
const POLICY_BAKED_HEIGHTFIELD_MASK    : u32 = GROUND_STATIC_BASE_MASK
                                              | (1u << CONTRIB_PYRAMIDS);
const POLICY_FLYER_MASK                : u32 = GROUND_STATIC_BASE_MASK
                                              | (1u << CONTRIB_PYRAMIDS)
                                              | (1u << CONTRIB_GOL_ZONES)
                                              | (1u << CONTRIB_TERRAIN_WAVES)
                                              | (1u << CONTRIB_RADIAL_PULSES)
                                              | (1u << CONTRIB_PAWN_AURA);
const POLICY_WALKER_MASK               : u32 = GROUND_STATIC_BASE_MASK
                                              | (1u << CONTRIB_PYRAMIDS)
                                              | (1u << CONTRIB_GOL_ZONES)
                                              | (1u << CONTRIB_TERRAIN_WAVES)
                                              | (1u << CONTRIB_RADIAL_PULSES)
                                              | (1u << CONTRIB_PAWN_AURA)
                                              | (1u << CONTRIB_GOL_SUPPRESSION);
const POLICY_WALKER_TILT_MASK          : u32 = GROUND_STATIC_BASE_MASK
                                              | (1u << CONTRIB_PYRAMIDS)
                                              | (1u << CONTRIB_GOL_ZONES)
                                              | (1u << CONTRIB_TERRAIN_WAVES)
                                              | (1u << CONTRIB_RADIAL_PULSES)
                                              | (1u << CONTRIB_GOL_SUPPRESSION);  // pawn-centered; same suppression walker applies (truth-fix)
const POLICY_WALKER_AGENT_MASK         : u32 = GROUND_STATIC_BASE_MASK
                                              | (1u << CONTRIB_PYRAMIDS)
                                              | (1u << CONTRIB_GOL_ZONES)
                                              | (1u << CONTRIB_TERRAIN_WAVES)
                                              | (1u << CONTRIB_RADIAL_PULSES)
                                              | (1u << CONTRIB_PAWN_AURA);
const POLICY_CELESTIAL_MASK            : u32 = 0u;
// POLICY_TERRAIN_RENDER — the fused render-side set: the baked
// heightfield (static base + pyramids) + pawn aura + terrain waves +
// radial pulses. Deliberately NO CONTRIB_GOL_ZONES (the patch
// heightfield does not cache GoL; zones render as their own extrusion
// pass). Fused-only policy: there is NO query_ground_* function by
// design — realizations are patch_terrain_vs (full set) and
// shadow_patch_terrain_vs (baked + waves subset). The ~15
// entity/painting VS sites adding contrib_terrain_waves_at alone atop
// the entity ground atlas are sanctioned single-contributor
// consumptions of this same render set. STATUS: REALIZED (fused-only).
const POLICY_TERRAIN_RENDER_MASK       : u32 = GROUND_STATIC_BASE_MASK
                                              | (1u << CONTRIB_PYRAMIDS)
                                              | (1u << CONTRIB_TERRAIN_WAVES)
                                              | (1u << CONTRIB_RADIAL_PULSES)
                                              | (1u << CONTRIB_GOL_ZONES)   // realized as the card's .a, cell-nearest, pawn-suppressed — UNIFIED_GROUND_1 (DAG: GoL has no ancestors)
                                              | (1u << CONTRIB_PAWN_AURA);

// ═══ Ground Architecture ═══════════════════════════════════════════
//
// SEAM[world.wgsl:ground-architecture-mirror] anchor — this is the
//   GPU companion to contracts/ground_architecture.hpp. The header
//   declares ContributorId / PolicyId / CONTRIBUTOR_DAG / POLICIES[]
//   plus compile-time DAG closure validation; this block declares
//   the contrib_*_at functions, per-policy POLICY_*_MASK constants
//   (which must mirror POLICIES[].contributors bitmasks exactly),
//   and the query_ground_<policy> dispatch functions.
//   See ground_architecture:contract for the cross-file integrity
//   requirement.
//
// The ground at a world XZ is a graph of named contributors filtered
// through a set of named policies. Each consumer declares its policy;
// a single per-policy query_ground_* function evaluates the
// policy-selected contributor sum.
//
// See:
//   contracts/ground_architecture.hpp    — ContributorId / PolicyId /
//                                          CONTRIBUTOR_DAG / POLICIES[]
//                                          plus compile-time DAG closure
//                                          validation
//
// ── Contributors (contrib_*_at in world.wgsl below) ────────────────
//
// A contributor is a function `contrib_<id>_at(xz[, args])` returning
// an f32 delta at a world XZ. Contributors are additive. Three classes:
//
//   static_landform   — placed once, baked. DAG edges among themselves.
//                       (terrain_lattice, tile_modifiers, solids,
//                        pyramids, paintings_bases, vegetation_bases)
//   slow_dynamic      — changes per frame but not per instantiation.
//                       (gol_zones)
//   deformation_field — acts across the static+dynamic stack.
//                       Not part of the DAG.
//                       (terrain_waves, radial_pulses, pawn_aura,
//                        gol_suppression — subtractive, consumer-local)
//
// CONTRIB_PAWN_AURA has two consumer-facing forms:
//   contrib_pawn_aura_at_self()      — scalar peak (constant). Used by
//                                      POLICY_WALKER because the pawn
//                                      sits at its own aura peak and
//                                      sampling the grid at the pawn's
//                                      XZ reads directional bias that
//                                      oscillates under locomotion.
//   contrib_pawn_aura_at_external(xz) — grid sample. Used by POLICY_FLYER,
//                                      POLICY_WALKER_AGENT, and inline
//                                      render-side samples (patch VS,
//                                      zone extrusion VS). These
//                                      consumers are not at the pawn's
//                                      position, so the grid is what
//                                      they need.
//
// The three fused static-base contributors (lattice × tile_modifiers
// + solids) evaluate together as contrib_static_base_at — the current
// composition is inseparable; they're declared separately in the DAG
// so policy closure validation operates on logical ids.
//
// ── Policies (query_ground_* in world.wgsl below) ──────────────────
//
// A policy is a compile-time contributor bitmask. A consumer declares
// its policy by calling query_ground_<policy>. FXC sees a uniform
// function choice and dead-code-eliminates contributors outside the
// mask. The policy is part of the consumer's *identity* — changing
// what a consumer sees requires changing its declared policy.
//
//   POLICY_PLACEMENT_*         spawn-time Y correction (no deformation)
//   POLICY_BAKED_HEIGHTFIELD   what the patch heightfield texture caches
//   POLICY_FLYER               live all-global-deformations (spheres, cubes)
//   POLICY_WALKER              flyer + consumer-local gol_suppression
//                              (used for the pawn's resolved standing y)
//   POLICY_WALKER_TILT         walker minus the self aura (a zero-
//                              gradient scalar); carries the SAME
//                              pawn-centered GoL suppression as the
//                              walker. Used for tilt and step-climb so
//                              the aura's radial profile doesn't
//                              manufacture slopes between ε-samples.
//   POLICY_WALKER_AGENT        walker minus suppression
//   POLICY_CELESTIAL           empty — ground is 0.0 (STATUS: INTENT)
//   POLICY_TERRAIN_RENDER      the fused render set: baked + aura +
//                              waves + pulses, no GoL. Fused-only —
//                              no query fn; see its mask block.
//
// Cached-texture variant: POLICY_BAKED_HEIGHTFIELD can be consumed
// analytically via query_ground_baked_heightfield, OR by sampling the
// pre-baked patch heightfield texture via sample_terrain_y_at. The
// texture is cheaper per-frame but static-only; its contributor set
// matches POLICY_BAKED_HEIGHTFIELD exactly. Per-frame Y-correction
// passes, camera clamps, and shadow VPs use the cached path.
//
// Query entry points — one per policy:
//   query_ground_<policy>(xz [, QueryInputs])           -> f32
//   query_ground_<policy>_gradient(xz, qi, eps)         -> vec3 (h, dh/dx, dh/dz)
//   query_ground_walker_walkable(xz, qi, eps, step_h)   -> vec3 (cliff-clamped)
// QueryInputs bundles consumer_pos + t_seconds so future additions
// don't break call signatures. Placement queries skip QueryInputs
// (no deformation fields consumed at spawn time).
//
// ── Extension patterns ────────────────────────────────────────────
//
// Add a new contributor:
//   1. Add a ContributorId in contracts/ground_architecture.hpp; bump CONTRIB_COUNT.
//   2. Declare its DAG edges (if static_landform) in CONTRIBUTOR_DAG;
//      the closure assert iterates the table — no further edit there.
//   3. Implement contrib_<name>_at in world.wgsl with a header comment
//      naming the contributor id, class, and deps.
//   4. Add it to the relevant POLICIES[].contributors masks (C++ side)
//      and the matching WGSL const POLICY_*_MASK.
//   5. Add its dispatch line to every query_ground_* function whose
//      policy includes it.
//   6. If used by the fused patch terrain VS or ground_formed_with_complexity,
//      update those too.
//
// Add a new policy:
//   1. Add a PolicyId in contracts/ground_architecture.hpp; bump POLICY_COUNT.
//   2. Add a row to POLICIES[] with the contributor mask.
//   3. Add the matching WGSL const POLICY_*_MASK.
//   4. Implement query_ground_<policy> in world.wgsl.
//   5. If gradient-supported, also query_ground_<policy>_gradient
//      (and _walkable for walker-family).
//
// ── Fused inline evaluations ──────────────────────────────────────
//
// Two hot paths keep hand-fused copies of policy-equivalent
// evaluations for per-vertex/per-texel performance (design doc §8):
//
//   ground_formed_with_complexity (two-pass patch heightfield gen)
//     ≡ POLICY_BAKED_HEIGHTFIELD, plus a complexity byproduct.
//   patch_terrain_vs (main terrain VS, ~256×256 invocations/patch)
//     ≡ POLICY_TERRAIN_RENDER (baked heightfield + aura + waves +
//       pulses; no GoL — zones draw as their own extrusion pass).
//
// Both must stay consistent with their policy's contributor set. If
// a policy gains or loses a contributor, update the fused function.

// CONTRIB_STATIC_BASE — static_landform (fused triple), global.
// Contributes: lattice × tile_modifiers + solids — the inseparable
//   multiplicative composition that forms the base terrain surface.
// Dependencies (via DAG): none (these three are roots of the DAG).
// Notes: the three logical contributors (CONTRIB_TERRAIN_LATTICE,
//   CONTRIB_TILE_MODIFIERS, CONTRIB_SOLIDS) are declared separately in
//   the registry so policy-closure validation operates on logical ids,
//   but their evaluation is fused here because the multiplicative form
//   doesn't decompose additively.
// (fn contrib_static_base_at RETIRED — UNIFIED_GROUND_1 U4; A2-3 census)

// CONTRIB_PAINTINGS_BASES — static_landform, global.
// Contributes: 0.0 — placeholder stub.
// Dependencies (via DAG): CONTRIB_TERRAIN_LATTICE, CONTRIB_TILE_MODIFIERS,
//   CONTRIB_SOLIDS, CONTRIB_PYRAMIDS.
// Notes: reserved for a future flat-bases-under-paintings contributor;
//   the registry slot exists so policies can declare intent now.
fn contrib_paintings_base_at(world_xz: vec2<f32>) -> f32 {
    return 0.0;
}

// CONTRIB_VEGETATION_BASES — static_landform, global.
// Contributes: 0.0 — placeholder stub.
// Dependencies (via DAG): CONTRIB_TERRAIN_LATTICE, CONTRIB_TILE_MODIFIERS,
//   CONTRIB_SOLIDS.
// Notes: reserved for a future planters/tile-slots contributor; the
//   registry slot exists so policies can declare intent now.
fn contrib_vegetation_base_at(world_xz: vec2<f32>) -> f32 {
    return 0.0;
}

// Combined height + complexity — avoids evaluating terrain lattice waves twice.
// terrain_height_and_complexity does the same lattice work as terrain_height_at
// but also accumulates the complexity metric. Used by two-pass heightfield gen.
//
// Height output is the POLICY_BAKED_HEIGHTFIELD contributor set exactly:
//   contrib_static_base_at(xz) + contrib_pyramids_at(xz)
// Kept hand-fused (not dispatched through query_ground_baked_heightfield)
// because the complexity metric is a free byproduct of the same lattice
// pass and per-texel cost dominates patch generation. If the baked policy's
// contributor set ever changes, update this function to match.
// See contracts/ground_architecture.hpp (fused inline evaluations).
fn ground_formed_with_complexity(world_xz: vec2<f32>) -> vec2<f32> {
    let hc = terrain_height_and_complexity(world_xz, config.world_seed, 0.0);
    let mods = tile_modifiers_at(world_xz);
    let height = hc.x * mods.x + mods.y + structure_height_at(world_xz) + contrib_pyramids_at(world_xz);
    return vec2(height, hc.y);
}

// ─── Polyphony-driven wave overlay ──────────────────────────────────────
//
// 6 cheap directional sine waves layered on the frozen lattice terrain.
// Polyphony count activates waves progressively (fine ripples first,
// continental swells last). Blend ramp and phase origin prevent teleportation.
// Seed-derived jitter makes each finite outdoor world feel different.
//
// Cost: 6 sin() calls per evaluation point. Called in VS + pawn + camera.
//
// ─── Overlay wave band table → THE TERRAIN_LOOKS PANEL ─────────────
// (OverlayWave struct + OVERLAY_WAVE_COUNT + OVERLAY_WAVES + the
//  design matrix moved to ROW 7, §2.2 — the movement third of the
//  surface voice. The two evaluators below stay with the deformation
//  machinery.)



// (fn contrib_terrain_waves_at RETIRED — UNIFIED_GROUND_1 U4; A2-3 census)

// (terrain_wave_overlay forwarder and terrain_wave_overlay_gradient
// removed in Step 5. Callers now invoke contrib_terrain_waves_at
// directly; nothing referenced the gradient-only helper.)

// Fused height + analytical gradient for the wave overlay.
// Returns vec3(height, dh/dx, dh/dz).
// Replaces the 5x finite-difference approach with 1x loop + analytical derivatives.
// Used by patch_terrain_vs where per-vertex cost dominates frame time.
// (fn terrain_wave_overlay_with_gradient RETIRED — TRUEBAND_CONTACT_1 T1c;
//  the true-band writer replaced the overlay; A3-3d certified the sole caller.)

// ─── Radial pulses: expanding ring wavefronts from note onsets ──────────
//
// Each pulse is an expanding ring centered on the pawn's position at onset.
// The wavefront radius grows with time; a gaussian envelope makes the ring
// thin. Distance damping and age decay give natural falloff.
//
// Cost: 8 iterations per evaluation point (one per ring buffer slot).
// Dead entries (age > max or amplitude = 0) early-exit cheaply.

const PULSE_SPEED: f32 = 30.0;         // world units per second (ring expansion rate)
const PULSE_MAX_AGE: f32 = 8.0;        // seconds — pulses older than this are ignored
const PULSE_RING_SHARPNESS: f32 = 0.3; // gaussian falloff around wavefront (lower = wider ring)
const PULSE_DAMPING: f32 = 0.012;      // distance damping (attenuation per world unit)
const PULSE_AGE_DECAY: f32 = 0.4;      // age damping (1/seconds — half amplitude at ~1.7s)

// CONTRIB_RADIAL_PULSES — deformation_field, global.
// Contributes: sum of expanding gaussian ring wavefronts from note onsets.
// Dependencies (via DAG): none — orthogonal to the static stack.
// Notes: t_seconds is an explicit parameter so the contributor works in
//   both render stages (render_signal.t_seconds) and compute stages
//   (signal.t_seconds). 8-slot ring buffer; dead entries early-exit.
// DRIVERLESS since gen-1 retirement — held at neutral by the boot
// block; revive via a gen-2 coupling or delete on the next pass here.
fn contrib_radial_pulses_at(world_xz: vec2<f32>, t_seconds: f32) -> f32 {
    if (config.pulse_count == 0u) { return 0.0; }

    var h: f32 = 0.0;
    let count = min(config.pulse_count, 8u);

    for (var i: u32 = 0u; i < count; i++) {
        let p = config.pulse_data[i];  // (origin_x, origin_z, onset_seconds, amplitude)
        let age = t_seconds - p.z;
        if (age < 0.0 || age > PULSE_MAX_AGE || p.w < 0.001) { continue; }

        let dist = length(world_xz - p.xy);

        // Expanding ring: wavefront at radius = age × speed
        let wavefront_r = age * PULSE_SPEED;
        let ring_dist = dist - wavefront_r;

        // Gaussian ring envelope (sharp peak at wavefront)
        let ring = exp(-ring_dist * ring_dist * PULSE_RING_SHARPNESS);

        // Damping: distance from origin + age
        let spatial_damp = exp(-dist * PULSE_DAMPING);
        let age_damp = exp(-age * PULSE_AGE_DECAY);

        h += p.w * ring * spatial_damp * age_damp;
    }

    return h;
}

// CONTRIB_PAWN_AURA — deformation_field, global (pawn-anchored).
// Has two consumer-facing forms; policies pick per consumer.
//
// ─ external form ──────────────────────────────────────────────
// Grid sample at an arbitrary world xz, using the possessed agent's
// position as the field's anchor for the bounding-box check. Used by
// consumers querying away from the pawn: POLICY_FLYER (spheres, cubes,
// camera), POLICY_WALKER_AGENT (non-pawn walkers), and inline render-
// side samples in patch_terrain_vs / zone_extrusion_vs.
// Contributes: height extrusion at xz based on the directional-biased
//   aura cell that xz falls into.
// Dependencies (via DAG): none — orthogonal to the static stack.
// Notes: wraps sample_pawn_aura (defined later — WGSL resolves
//   function references module-wide).
fn contrib_pawn_aura_at_external(world_xz: vec2<f32>) -> f32 {
    return sample_pawn_aura(world_xz, compute_pawn_pos().xz).r * config.pawn_aura_height;
}

// ─ self form ─────────────────────────────────────────────────
// Scalar peak value for the pawn's own Y. The pawn sits at the center
// of its own aura dome; the dome's peak is config.pawn_aura_height
// (presence ramping already folded in CPU-side via auraPresence_).
// Used by POLICY_WALKER.
//
// Why not sample the grid at pawn_state.pos.xz?  compute_pawn_aura
// computes a directional-biased cell value (see §7.4 PAWN AURA:
// leading ramp toward heading, steeper drop behind). Sampling
// at the pawn's own XZ reads that bias — and as the pawn walks across
// cells, the bias produces vertical oscillation (visible as bobbing).
// The pre-refactor pawn Y code was a flat scalar add; this restores
// that semantics inside the policy system.
//
// Returns a constant (no xz dependence), so the contributor's gradient
// is zero — including it or excluding it from POLICY_WALKER_TILT is
// mathematically equivalent; the tilt policy continues to exclude it
// for conceptual clarity.
fn contrib_pawn_aura_at_self() -> f32 {
    return config.pawn_aura_height;
}

// ════════════════════════════════════════════════════════════════
// Ground Query API — per-policy specializations
//
// One entry point per policy. Each consumer declares its policy at
// its own call site (a compile-time constant choice of function) so
// FXC sees uniform branching and can dead-code-eliminate anything
// outside that policy's contributor set. Runtime policy dispatch is
// deliberately avoided — see contracts/ground_architecture.hpp (POLICIES[]).
//
// Contributor sets mirror POLICIES[] in contracts/ground_architecture.hpp.
// The architecture overview above this section explains classes, DAG,
// extension patterns, and the fused-inline hot paths that bypass this
// API for performance.
// ════════════════════════════════════════════════════════════════

struct QueryInputs {
    consumer_pos: vec3<f32>,
    t_seconds:    f32,
}

// --- Placement policies: no deformation fields (spawn-time Y correction) ---

// POLICY_PLACEMENT_PYRAMID — pyramid spawn-time Y correction.
// Contributors: CONTRIB_TERRAIN_LATTICE, CONTRIB_TILE_MODIFIERS, CONTRIB_SOLIDS
//   (i.e. contrib_static_base_at).
// Typical consumers: pyramid spawn engines deciding candidate Y at a tile.
// Notes: "pyramids don't see themselves" — no CONTRIB_PYRAMIDS in the set.
// STATUS: LATENT[policy-surface] — declared placement query, no live
//   caller; the live Y path is compute_entity_placement's baked hybrid
//   (whose set INCLUDES pyramids — the declared-intent exclusion is a
//   possible future aesthetic ruling; see the POLICIES[] row). Rewiring
//   candidate when placement moves onto the policy API.
fn query_ground_placement_pyramid(xz: vec2<f32>) -> f32 {
    // GROUND_CARD_1: rides the baked path (which INCLUDES pyramids). The
    // declared no-pyramids intent predates the baked hybrid and returns
    // when placement moves onto the policy API — see STATUS above.
    return sample_terrain_y_at(xz);
}

// POLICY_PLACEMENT_PAINTING — painting spawn-time Y correction.
// Contributors: static_base + CONTRIB_PYRAMIDS + CONTRIB_GOL_ZONES.
// Typical consumers: painting spawn engines (terrain quads + wall frames).
// Notes: paintings sit on current GoL extrusion. No deformation fields,
//   so spawn position is stable against animated terrain.
// STATUS: LATENT[policy-surface] — declared placement query, no live
//   caller; the live path is compute_entity_placement's documented
//   baked + analytic-GoL hybrid (same contributor set). Rewiring
//   candidate when placement moves onto the policy API.
fn query_ground_placement_painting(xz: vec2<f32>) -> f32 {
    // GROUND_CARD_1: base(p) + raw cell-exact GoL — the same composition
    // compute_entity_placement's painting hybrid runs.
    return sample_terrain_y_at(xz) + sample_live_card_gol(xz);
}

// POLICY_PLACEMENT_VEGETATION — tree / column / arch spawn-time Y correction.
// Contributors: CONTRIB_TERRAIN_LATTICE, CONTRIB_TILE_MODIFIERS, CONTRIB_SOLIDS
//   (i.e. contrib_static_base_at).
// Typical consumers: vegetation spawn engines (palm, cactus, blade, columns,
//   antennas, arches).
// Notes: "trees don't stand on pyramids" — no CONTRIB_PYRAMIDS in the set.
// STATUS: LATENT[policy-surface] — declared placement query, no live
//   caller; the live vegetation Y path samples the baked heightfield
//   (whose set INCLUDES pyramids — the declared-intent exclusion is a
//   possible future aesthetic ruling; see the POLICIES[] row). Rewiring
//   candidate when placement moves onto the policy API.
fn query_ground_placement_vegetation(xz: vec2<f32>) -> f32 {
    // GROUND_CARD_1: rides the baked path (includes pyramids — the live
    // vegetation Y path already did; see STATUS above).
    return sample_terrain_y_at(xz);
}

// --- Baked heightfield: all static, no dynamic, no deformation ---

// POLICY_BAKED_HEIGHTFIELD — what the cached patch heightfield texture caches.
// Contributors: contrib_static_base_at + CONTRIB_PYRAMIDS.
// Typical consumers: zone-mesh analytical fallback, any compute that wants
//   the ground-without-dynamics. The texture variant is sample_terrain_y_at.
// Notes: must stay consistent with ground_formed_with_complexity (the
//   two-pass patch heightfield generator) — same contributor set.
// (fn query_ground_baked_heightfield RETIRED — UNIFIED_GROUND_1 U4; A2-3 census)

// ─── The shared dynamic-overlay stack (b2a) ─────────────────────────
// The additive fold every DYNAMIC ground policy shares, authored ONCE
// (GROUND_CARD_1 — the compute rewire):
//   sample_terrain_y_at (base: static + pyramids, the baked path)
//     + <gol_term> + sample_live_card(xz).x (waves + pulses — the card)
// The mover-anchored pawn aura is NOT here — callers add their own aura
// form (external / self / none) AFTER this stack, so the per-body
// divergence stays explicit at the call site (world-anchored terms live
// in the stack; mover-anchored terms live at the caller).
//
// gol_term is the caller's already-resolved GoL contribution: the raw
// contrib_gol_zones_at for flyer/agent, or the inline pawn-suppressed
// form gol*(1 − supp_factor) for the walkers — kept a SINGLE term so the
// walker stays bit-identical (see query_ground_walker).
//
// ORDER: the sum preserves the historical operand order (GoL before
// waves) so every caller is bit-identical. The terrain hierarchy's
// canonical layering puts waves at the BOTTOM of the overlay stack; that
// order is DOCUMENTATION here — reordering plain-additive terms only
// shifts float round-off, so it is deferred until a layer stops being a
// plain add (reordered then under its own rig gate).
fn manifold_overlay_stack(xz: vec2<f32>, qi: QueryInputs, gol_term: f32) -> f32 {
    // (qi retained for signature stability; pulses now ride the card.)
    var h = sample_terrain_y_at(xz);   // base(p): static base + pyramids (baked)
    h += gol_term;                     // GoL before waves — historical operand order kept
    h += sample_live_card(xz).x;       // live(p).x: waves + pulses (the card)
    return h;
}

// --- Fly-over: all global deformation fields included ---

// POLICY_FLYER — non-walking entities that ride animated terrain.
// Contributors: static_base + CONTRIB_PYRAMIDS + CONTRIB_GOL_ZONES +
//   CONTRIB_TERRAIN_WAVES + CONTRIB_RADIAL_PULSES + CONTRIB_PAWN_AURA
//   (external form — grid sample at xz).
// Typical consumers: sphere orbit clearance, cube hover base, primary
//   camera clamp.
// Notes: no CONTRIB_GOL_SUPPRESSION — flyers don't flatten GoL at their
//   own position. Aura uses contrib_pawn_aura_at_external because flyers
//   sample away from the pawn's position. Gradient variant:
//   query_ground_flyer_gradient.
fn query_ground_flyer(xz: vec2<f32>, qi: QueryInputs) -> f32 {
    // Raw GoL (flyers don't self-suppress) over the shared stack; external
    // aura (sampled away from the pawn) added after as the mover term.
    return manifold_overlay_stack(xz, qi, sample_live_card_gol(xz))
         + contrib_pawn_aura_at_external(xz);
}

// --- Walkers: everything the flyer sees, plus walker-specific fields ---

// POLICY_WALKER — the pawn's resolved standing height.
// Contributors: static_base + CONTRIB_PYRAMIDS + CONTRIB_GOL_ZONES +
//   CONTRIB_TERRAIN_WAVES + CONTRIB_RADIAL_PULSES + CONTRIB_PAWN_AURA
//   (self form — scalar peak) - CONTRIB_GOL_SUPPRESSION (subtractive,
//   centered on qi.consumer_pos).
// Typical consumers: pawn_ground_resolve (final resolved Y).
// Notes: the walker stands on aura-lifted ground. Aura uses
//   contrib_pawn_aura_at_self — the pawn knows it sits at its own aura
//   peak without reading the grid (which has directional bias that
//   produces bobbing under locomotion; see the aura contributor header).
//   Gradient: use query_ground_walker_tilt for tilt/step-climb to avoid
//   manufactured slopes from gol_suppression (which IS position-dependent).
//   Walkable variant: query_ground_walker_walkable (cliff-clamped).
//
// Implementation: evaluates contrib_gol_zones_at ONCE and applies the
// pawn-centered suppression factor inline — algebraically identical to
//   h += contrib_gol_zones_at(xz);
//   h -= contrib_gol_suppression_at(xz, consumer_pos);
// but avoids a second full pass over the GoL zone loop (which
// compounds significantly under FXC loop unrolling). See
// contrib_gol_suppression_at for the standalone subtractive form.
fn query_ground_walker(xz: vec2<f32>, qi: QueryInputs) -> f32 {
    // GoL with inline pawn-centered suppression. Equivalent to
    //   contrib_gol_zones_at(xz) - contrib_gol_suppression_at(xz, consumer_pos)
    // but evaluates the zone loop once instead of twice. The suppression
    // factor pulls the GoL lift toward zero near the consumer — walker
    // intent: "GoL doesn't push me up into the air while I'm standing on it."
    // Kept a SINGLE term (gol*(1−supp)) so the walker stays bit-identical.
    let gol = sample_live_card_gol(xz);
    let supp_factor = pawn_gol_suppression(xz, qi.consumer_pos.xz);
    // Shared world stack + the mover-anchored self-aura (added after so the
    // pawn stands on its own aura peak without reading the grid).
    return manifold_overlay_stack(xz, qi, gol * (1.0 - supp_factor))
         + contrib_pawn_aura_at_self();
}

// POLICY_WALKER_TILT — walker minus the self-centered pawn aura.
// Contributors: static_base + CONTRIB_PYRAMIDS + CONTRIB_GOL_ZONES
//   (pawn-suppressed) + CONTRIB_TERRAIN_WAVES + CONTRIB_RADIAL_PULSES.
// Typical consumers: terrain_normal_at (pawn tilt), pawn_ground_resolve
//   step-climb decisions.
// Notes:
//   - Excludes CONTRIB_PAWN_AURA: the self form is a constant scalar with
//     zero gradient, so including or excluding it from tilt is equivalent;
//     excluded for clarity — self-centered fields never drive tilt.
//   - GoL carries the SAME pawn-centered suppression as the walker, so it
//     is zero within the immediate radius. This is what stops the body
//     leaning on cells its feet stand flat on. No radial slope is
//     manufactured: terrain_normal_at samples at eps = 0.5, entirely inside
//     INNER = 4, where supp_factor = 1 with zero gradient — the smoothstep
//     only ramps over 4→15, which the normal samples never reach.
fn query_ground_walker_tilt(xz: vec2<f32>, qi: QueryInputs) -> f32 {
    // Same pawn-suppressed GoL as the walker; NO aura (the self field is a
    // constant scalar with zero tilt gradient — excluded for clarity). Just
    // the shared world stack.
    let gol = sample_live_card_gol(xz);
    let supp_factor = pawn_gol_suppression(xz, qi.consumer_pos.xz);
    return manifold_overlay_stack(xz, qi, gol * (1.0 - supp_factor));
}

// Paired walker + walker_tilt query.
// Returns vec2(walker_height, walker_tilt_height) — both values
// computed from a single evaluation of the shared 5-contributor
// tilt base. Consumers that need both heights at the same XZ
// (pawn_ground_resolve; future agent ground resolves) should use
// this in preference to two separate query calls — it halves the
// shared-contributor work per XZ.
//
// Semantics: bit-identical to separate calls to query_ground_walker
// and query_ground_walker_tilt at the same xz — both now apply the same
// pawn-centered GoL suppression (supp_factor), so tilt and walker share
// the suppressed GoL and differ only by the pawn-self aura.
//
// Shape of the return vec2:
//   .x = walker      = tilt + pawn_aura_self
//   .y = walker_tilt = base(baked: static+pyramids) + gol*(1 − supp_factor) + live.x(waves+pulses)
fn query_ground_walker_pair(xz: vec2<f32>, qi: QueryInputs) -> vec2<f32> {
    // Shared base (GoL not yet suppressed) — GROUND_CARD_1: one baked
    // fetch + one card fetch + one cell-exact GoL fetch, shared by both
    // outputs exactly as the analytic quintet was.
    let base   = sample_terrain_y_at(xz);   // static base + pyramids (baked)
    let gol    = sample_live_card_gol(xz);
    let live_h = sample_live_card(xz).x;    // waves + pulses (the card)

    // Pawn-centered GoL suppression — zero within the immediate radius —
    // applied to the tilt so it doesn't lean on cells the pawn stands flat
    // on. Walker is that same tilt plus the pawn-self aura peak; they now
    // differ only by the aura.
    let supp_factor = pawn_gol_suppression(xz, qi.consumer_pos.xz);
    let gol_supp = gol * (1.0 - supp_factor);
    let tilt   = base + gol_supp + live_h;
    let walker = tilt + contrib_pawn_aura_at_self();

    return vec2(walker, tilt);
}

// POLICY_WALKER_AGENT — non-pawn walkers (NPCs).
// Contributors: static_base + CONTRIB_PYRAMIDS + CONTRIB_GOL_ZONES +
//   CONTRIB_TERRAIN_WAVES + CONTRIB_RADIAL_PULSES + CONTRIB_PAWN_AURA
//   (external form — grid sample at xz, since the agent is not the pawn).
// Typical consumers: agent ground resolve (none today; reserved for
//   the agent system).
// Notes: agents feel the full GoL lift — no self-suppression. Design
//   doc §3.3: revisit if agents stuck on top of GoL zones look wrong.
//   When agents grow their own self-centered auras, add analogous
//   contrib_<agent>_aura_at_self() forms (defer until a second consumer
//   asks for one).
fn query_ground_walker_agent(xz: vec2<f32>, qi: QueryInputs) -> f32 {
    // Full GoL lift (agents don't self-suppress) over the shared stack;
    // external aura (the agent is not the pawn) added after as the mover term.
    return manifold_overlay_stack(xz, qi, sample_live_card_gol(xz))
         + contrib_pawn_aura_at_external(xz);
}

// --- Celestial: empty contributor set ---

// POLICY_CELESTIAL — sun, stars, sky entities.
// Contributors: none.
// Typical consumers: future celestial entity placement (none today).
// Notes: returns 0.0 unconditionally; kept for symmetry.
// STATUS: INTENT — declared, zero realization; the row says so too.
fn query_ground_celestial(xz: vec2<f32>) -> f32 {
    return 0.0;
}

// --- Gradient variants ---
// Central finite differences on the policy's query function. Caller
// supplies eps; walker_walkable additionally clamps cliff-like steps.

// POLICY_FLYER gradient.
// Returns vec3(h, dh/dx, dh/dz). Five samples (center + four neighbors)
// of query_ground_flyer.
// STATUS: LATENT[policy-surface] — zero callers; plausible consumer:
//   multi-sample flyer slope/approach users (e.g. slope-aware hover).
fn query_ground_flyer_gradient(xz: vec2<f32>, qi: QueryInputs, eps: f32) -> vec3<f32> {
    let h   = query_ground_flyer(xz,                     qi);
    let hpx = query_ground_flyer(xz + vec2(eps,  0.0),   qi);
    let hmx = query_ground_flyer(xz - vec2(eps,  0.0),   qi);
    let hpz = query_ground_flyer(xz + vec2(0.0,  eps),   qi);
    let hmz = query_ground_flyer(xz - vec2(0.0,  eps),   qi);
    let inv_2eps = 0.5 / eps;
    return vec3(h, (hpx - hmx) * inv_2eps, (hpz - hmz) * inv_2eps);
}

// POLICY_WALKER gradient (full walker, including self-centered fields).
// Returns vec3(h, dh/dx, dh/dz). Five samples of query_ground_walker.
// Notes: for tilt and step-climb, prefer query_ground_walker_tilt to
//   avoid manufactured slopes from the pawn's own aura/suppression.
// STATUS: LATENT[policy-surface] — zero callers; the live tilt path is
//   terrain_normal_at's 3-tap over walker_tilt. Plausible consumer:
//   full-walker slope users that want the self fields included.
fn query_ground_walker_gradient(xz: vec2<f32>, qi: QueryInputs, eps: f32) -> vec3<f32> {
    let h   = query_ground_walker(xz,                     qi);
    let hpx = query_ground_walker(xz + vec2(eps,  0.0),   qi);
    let hmx = query_ground_walker(xz - vec2(eps,  0.0),   qi);
    let hpz = query_ground_walker(xz + vec2(0.0,  eps),   qi);
    let hmz = query_ground_walker(xz - vec2(0.0,  eps),   qi);
    let inv_2eps = 0.5 / eps;
    return vec3(h, (hpx - hmx) * inv_2eps, (hpz - hmz) * inv_2eps);
}

// POLICY_WALKER walkable gradient — cliff-clamped variant.
// Returns vec3(h0, dh/dx, dh/dz) with neighbor heights clamped to h0
// when |neighbor - h0| > step_h, treating cliffs as flat for gradient
// purposes.
// Notes: as with query_ground_walker_gradient, the self-centered
//   fields can produce noisy gradients near the pawn — prefer the
//   tilt-policy variant for tilt and step-climb decisions.
// STATUS: LATENT[policy-surface] — zero callers; plausible consumer:
//   cliff-aware locomotion (walkability tests that must not read a
//   cliff face as a slope).
fn query_ground_walker_walkable(xz: vec2<f32>, qi: QueryInputs, eps: f32, step_h: f32) -> vec3<f32> {
    let h0 = query_ground_walker(xz, qi);

    var hpx = query_ground_walker(xz + vec2(eps,  0.0), qi);
    var hmx = query_ground_walker(xz - vec2(eps,  0.0), qi);
    var hpz = query_ground_walker(xz + vec2(0.0,  eps), qi);
    var hmz = query_ground_walker(xz - vec2(0.0,  eps), qi);

    if (abs(hpx - h0) > step_h) { hpx = h0; }
    if (abs(hmx - h0) > step_h) { hmx = h0; }
    if (abs(hpz - h0) > step_h) { hpz = h0; }
    if (abs(hmz - h0) > step_h) { hmz = h0; }

    let inv_2eps = 0.5 / eps;
    return vec3(h0, (hpx - hmx) * inv_2eps, (hpz - hmz) * inv_2eps);
}

// ════════════════════════════════════════════════════════════════
// THE MANIFOLD INTERFACE (ratified).
//
// The ONE query every consumer uses to ask "where is the surface, and
// how is it oriented, at this coordinate, within this boundary?" The
// heightfield is the sole CAST behind it in Stage 1 — position/normal
// are RETURNED by the cast, not reconstructed by the caller (the
// Y-up 1.0 lives here now, not at every call site). A spherical cast
// (Stage 3) implements this signature UNCHANGED: query_pos projects to
// the sphere direction, position = center + dir*radius, normal = the
// tangent-perturbed direction — the caller never learns which cast
// answered. See audit/TERRAIN2_STAGE1_INTERFACE.md.
//
// INPUT: query_pos is a WORLD-SPACE position; the cast projects it into
// its own parameter space (the heightfield reads .xz). No coordinate
// generic — a world position is the universal coordinate.
// BOUNDARY: Boundary{center, extent} + valid stay DECLARED-DORMANT.
// b3 RULING: containment (the finite
// world_bound) is a SEPARATE flat-manifold shell — an "am I allowed
// here?" clamp AROUND the query — NOT folded into this "where is the
// surface?" query, which stays PURE. On the heightfield the ground
// extends infinitely even in a finite world (world_bound is an
// invisible wall, not terrain extent — MOOD_FINITE_OUTDOOR is finite
// with no walls), so valid is always 1u and consumers clamp via the
// separate containment layer as today. Boundary's real home is the
// manifold FAMILY's closedness story: a sphere is CLOSED — no edge,
// valid always 1 — closedness replaces containment on closed
// manifolds; the type is kept declared for that future, not for a
// heightfield containment fold.

struct SurfaceHit {
    position: vec3<f32>,   // the surface point in world space (the cast fills all 3 axes)
    normal:   vec3<f32>,   // the true surface normal (cast-computed; heightfield: (-gx,1,-gz))
    valid:    u32,         // stays 1u — containment is a separate layer, not folded in (b3 ruling)
}

struct Boundary {
    center: vec3<f32>,
    extent: f32,           // 0 = infinite (mirrors config.world_bound's (0,0,0,0) convention)
}

// THE HEIGHTFIELD CAST — the scalar height for a policy at an xz.
// Dispatches to the existing per-policy query functions (delegation =
// byte-identical values by construction). POLICY_TERRAIN_RENDER is a
// fused VS weld with no scalar query (not a resolve policy); CELESTIAL
// is groundless — both fall to the static base. The switch arms go
// live as consumers migrate onto manifold_resolve across the b1 cohort.
//
// THE FOLD (b2 RULING): this switch IS the
// ONE declared place the composition fold runs — in the order the
// POLICIES[] masks declare (contracts/ground_architecture.hpp). b2a's
// structural goal (one declared fold, every live consumer running it)
// is MET here; no analytic site hand-copies the fold. The physical
// merge of the per-policy query_ground_* BODIES into a single
// manifold_fold is NOT pulled — it is low-value (they don't drift) and
// high-FP-risk blind shader work; the real drift hazard lives in the
// WELDS (the bake, patch_terrain_vs, shadow_patch_terrain_vs), which
// Stage 3 rewrites — the body-merge rides that weld work IF wanted.
// b2b (the agreement flip: baked consumers seeing the live overlays)
// stays deferred to its own gated cut.
fn manifold_height_hf(xz: vec2<f32>, policy: u32, qi: QueryInputs) -> f32 {
    switch policy {
        case 0u:  { return query_ground_placement_pyramid(xz); }     // POLICY_PLACEMENT_PYRAMID
        case 1u:  { return query_ground_placement_painting(xz); }    // POLICY_PLACEMENT_PAINTING
        case 2u:  { return query_ground_placement_vegetation(xz); }  // POLICY_PLACEMENT_VEGETATION
        case 3u:  { return sample_terrain_y_at(xz); }                // POLICY_BAKED_HEIGHTFIELD (texture form — byte-consistent by construction; the analytic body stays for the zone baked-sampler fallback chain)
        case 4u:  { return query_ground_flyer(xz, qi); }             // POLICY_FLYER
        case 5u:  { return query_ground_walker(xz, qi); }            // POLICY_WALKER
        case 6u:  { return query_ground_walker_tilt(xz, qi); }       // POLICY_WALKER_TILT
        case 7u:  { return query_ground_walker_agent(xz, qi); }      // POLICY_WALKER_AGENT
        default:  { return sample_terrain_y_at(xz); }                // CELESTIAL/RENDER: baked-path fallback (GROUND_CARD_1 — the inline contributor arm rewired per H5)
    }
}

// THE POSITION-ONLY FACE (b1-cohort). The surface POINT
// without its orientation — for consumers that snap to the surface but
// do not orient to it (the camera clearance clamp, flyer/entity
// ground). PERF-NEUTRAL BY CONSTRUCTION: exactly one height evaluation,
// no normal work — no reliance on the compiler DCE-ing an unused
// normal. Cast-agnostic like manifold_resolve: a spherical cast returns
// center + dir*radius here (the surface point), the tangent-normal work
// only in the full resolve. In Stage 1 the heightfield cast returns
// (x, height, z) and consumers read .y (Y-up stays TRUE, provided by
// the cast); the sphere rewrites those .y reads to the full-vec3 form
// at Stage 3 (the Y-up movement weld).
fn manifold_position(query_pos: vec3<f32>, policy: u32, qi: QueryInputs) -> vec3<f32> {
    let xz = query_pos.xz;
    return vec3(xz.x, manifold_height_hf(xz, policy, qi), xz.y);
}

// THE RESOLVE (heightfield cast). position = the surface point (via
// manifold_position); normal = the heightfield gradient normal
// (finite-diff, eps=0.5) — bit-identical to terrain_normal_at's form
// for POLICY_WALKER_TILT. valid=1u until b3 activates the boundary.
fn manifold_resolve(query_pos: vec3<f32>, policy: u32, qi: QueryInputs) -> SurfaceHit {
    let xz = query_pos.xz;
    let eps = 0.5;
    let p0  = manifold_position(query_pos, policy, qi);
    let h0  = p0.y;
    let h_x = manifold_height_hf(xz + vec2(eps, 0.0),   policy, qi);
    let h_z = manifold_height_hf(xz + vec2(0.0, eps),   policy, qi);
    let dx = (h_x - h0) / eps;
    let dz = (h_z - h0) / eps;
    var hit: SurfaceHit;
    hit.position = p0;
    hit.normal   = normalize(vec3(-dx, 1.0, -dz));
    hit.valid    = 1u;
    return hit;
}

// ════════════════════════════════════════════════════════════════
// End Ground Query API.
// ════════════════════════════════════════════════════════════════

// --- [COUPLING:terrain→sphere:orbit_height]

fn coupling_terrain_to_sphere_orbit_height(sphere_xz: vec2<f32>, base_height: f32) -> f32 {
    if (!coupling_active(COUPLING_TERRAIN_TO_SPHERE_HEIGHT)) {
        return base_height;
    }

    // POLICY_FLYER — sphere rides static base + pyramids + gol zones +
    // terrain waves + radial pulses + pawn aura. No gol_suppression
    // (flyers don't flatten GoL at their own position).
    // consumer_pos is unused by flyer (no consumer-local fields); pass
    // a placeholder Y — only xz matters.
    let qi = QueryInputs(vec3(sphere_xz.x, 0.0, sphere_xz.y), signal.t_seconds);
    let ground = manifold_position(vec3(sphere_xz.x, 0.0, sphere_xz.y), POLICY_FLYER, qi).y;

    // Ensure minimum clearance above ground
    let min_height = ground + SPHERE_MIN_TERRAIN_CLEARANCE;

    return max(base_height, min_height);
}

// §3.5 entities → entities

// --- [COUPLING:pawn→camera:target]

fn coupling_pawn_to_camera_target(pawn_pos: vec3<f32>, cam: CameraState) -> vec3<f32> {
    return compose_camera_position_from_orbit(pawn_pos, cam);
}

// --- [COUPLING:pawn→sun:view_proj]
const SUN_ALTITUDE: f32 = 250.0;
const SUN_HALF_EXTENT: f32 = 300.0;
const SUN_NEAR: f32 = 0.1;
const SUN_FAR: f32 = 600.0;
const SHADOW_SNAP_SIZE: f32 = 2.0;   // world units — shadow VP snaps to this grid

fn coupling_pawn_to_sun_vp(pawn_pos: vec3<f32>, direction: vec3<f32>) -> mat4x4<f32> {
    // Snap pawn XZ to shadow grid for temporal stability.
    // Shadow map content is pixel-perfect between grid crossings,
    // enabling the CPU to skip the shadow pass on idle frames.
    var snapped = pawn_pos;
    snapped.x = round(pawn_pos.x / SHADOW_SNAP_SIZE) * SHADOW_SNAP_SIZE;
    snapped.z = round(pawn_pos.z / SHADOW_SNAP_SIZE) * SHADOW_SNAP_SIZE;

    // Sun position: offset from snapped pawn opposite to light direction
    let sun_pos = snapped - direction * SUN_ALTITUDE;

    // Forward = light direction (into the scene)
    let fwd = direction;

    // Up vector (choose one not parallel to direction)
    var world_up: vec3<f32>;
    if (abs(fwd.y) > 0.99) {
        world_up = vec3(0.0, 0.0, 1.0);
    } else {
        world_up = vec3(0.0, 1.0, 0.0);
    }

    // Right = normalize(cross(fwd, up))
    let right = normalize(cross(fwd, world_up));

    // True up = cross(right, fwd)
    let true_up = cross(right, fwd);

    // View matrix: world → light space
    // Column-major: each vec4 is a column
    let tx = -dot(right, sun_pos);
    let ty = -dot(true_up, sun_pos);
    let tz = dot(fwd, sun_pos);

    let view = mat4x4<f32>(
        vec4(right.x,    true_up.x,  -fwd.x,  0.0),
        vec4(right.y,    true_up.y,  -fwd.y,  0.0),
        vec4(right.z,    true_up.z,  -fwd.z,  0.0),
        vec4(tx,         ty,          tz,      1.0)
    );

    // Orthographic projection (WebGPU [0,1] depth range)
    let he = SUN_HALF_EXTENT;
    let r_depth = 1.0 / (SUN_FAR - SUN_NEAR);

    let proj = mat4x4<f32>(
        vec4(1.0 / he,  0.0,       0.0,                   0.0),
        vec4(0.0,        1.0 / he,  0.0,                   0.0),
        vec4(0.0,        0.0,      -r_depth,               0.0),
        vec4(0.0,        0.0,      -SUN_NEAR * r_depth,    1.0)
    );

    return proj * view;
}

// --- [COUPLING:velocity→pawn:heading]

fn coupling_velocity_to_pawn_heading(velocity: vec2<f32>, current_heading: f32, dt: f32) -> f32 {
    let speed_sq = dot(velocity, velocity);
    if (speed_sq < 0.001) {
        return current_heading;
    }
    
    let goal_heading = atan2(velocity.x, velocity.y);
    var diff = goal_heading - current_heading;
    
    if (diff > 3.14159) { diff -= 6.28318; }
    else if (diff < -3.14159) { diff += 6.28318; }
    
    let max_turn = PAWN_TURN_SPEED * dt;
    var new_heading = current_heading + clamp(diff, -max_turn, max_turn);
    
    if (new_heading > 3.14159) { new_heading -= 6.28318; }
    else if (new_heading < -3.14159) { new_heading += 6.28318; }
    
    return new_heading;
}

// §3.6 entities → terrain

// RAYMARCH/SDF EXCAVATION: coupling_sphere_to_terrain_tint removed — it fed
// only the dead terrain_state.tint store (the residue's one entangled wire,
// severed surgically from the live update_sphere kernel).

// §3.7 GoL → evolution

// --- [COUPLING:gol→next_state] Conway's rules
fn coupling_gol_next_state(alive: bool, neighbors: i32) -> f32 {
    if (alive) {
        return select(0.0, 1.0, neighbors == 2 || neighbors == 3);
    } else {
        return select(0.0, 1.0, neighbors == 3);
    }
}

// RAYMARCH/SDF EXCAVATION: the legacy fixed-wave dynamics limb removed —
// wave_enabled + dynamics_terrain_gradient_max were the last survivors of
// the SDF cone-march (gradient_max → lipschitz_factor, a step bound read by
// nobody). Whole chain (WAVES → gradient_max → lipschitz) was dead; gone
// with update_terrain_config.

// §4 DYNAMICS
// §4.1 PGA MOTOR INTEGRATION
// These functions use Projective Geometric Algebra for elegant transformations.
fn pga_color_motor(current_rgb: vec3<f32>, hue_speed: f32, sat_push: f32, val_climb: f32, dt: f32) -> vec3<f32> {
    // 1. CONVERT COLOR TO POINT
    let p_color = point_from_vec3(current_rgb);
    
    // 2. DEFINE AXIS OF LUMINANCE (The Grey Line)
    let axis_dir = normalize(vec3(0.60, 0.20, 0.10));
    
    // 3. HUE ROTOR (Twist)
    //    Rotation around the luminance axis shifts hue
    let r_hue = rotor(axis_dir, hue_speed * dt);
    
    // 4. VALUE TRANSLATOR (Climb)
    //    Translation along the luminance axis changes value
    let t_val = translator(axis_dir, val_climb * dt);
    
    // 5. SATURATION TRANSLATOR (Push)
    //    Translation perpendicular to luminance axis changes saturation
    //    Push AWAY from the grey line.
    let parallel = dot(current_rgb, axis_dir) * axis_dir;
    let perpendicular = current_rgb - parallel;
    var sat_dir = vec3(0.0);
    if (length(perpendicular) > 0.0001) {
        sat_dir = normalize(perpendicular);
    }
    let t_sat = translator(sat_dir, sat_push * dt);
    
    // 6. COMPOSE MOTOR: Hue -> Sat -> Val
    //    The geometric product composes transformations elegantly
    var m = gp_mm(t_sat, r_hue);
    m = gp_mm(t_val, m);
    
    // 7. APPLY AND RETURN
    let p_new = sw_motor_point(m, p_color);
    return clamp(point_to_vec3(p_new), vec3(0.0), vec3(1.0));
}

// --- [DYNAMICS:PGA] Sphere Orbit
fn dynamics_sphere_motor_orbit(t: f32, fe: FloatingEntityState) -> FloatingEntityState {
    var s: FloatingEntityState;

    // 1. DEFINITION
    //    Axis: The vertical line through the origin (Line Y)
    //    Speed: From per-entity state
    let orbit_axis = LINE_Y.d; 
    let angle = t * fe.orbit_speed;
    
    // 2. THE MOTOR (The Spell)
    //    Create a rotor that spins around the Y axis.
    let m_orbit = rotor(orbit_axis, angle); 
    
    // 3. LIFT (The Input)
    //    Offset from anchor — orbit_radius on X, flat plane.
    //    Height is added after; terrain coupling adjusts it.
    let offset = vec3(fe.orbit_radius, 0.0, 0.0);
    let p_start = point_from_vec3(offset);
    
    // 4. MOTIVATE (The Transformation)
    //    Apply the rotor to the offset point.
    let p_moved = sw_motor_point(m_orbit, p_start);
    let local_pos = point_to_vec3(p_moved);
    
    // 5. DROP (The Output)
    //    Anchor translation + base orbit height.
    s.pos = fe.anchor + vec3(local_pos.x, fe.orbit_height, local_pos.z);
    
    // Bonus: PGA gives us the orientation for free!
    // The sphere rotates to face its path.
    s.orientation = m_orbit.p0; 
    
    s.body_radius = fe.body_radius;
    s.influence_radius = fe.influence_radius;
    s.t = t;
    
    return s;
}

// §5 COMPOSITION

// Execution orchestration - where couplings and dynamics are applied.
fn compose_sphere_from_orbit_pga(t: f32, fe: FloatingEntityState) -> FloatingEntityState {
    // 1. Get the pure PGA orbit (circular motion via Motor)
    var s = dynamics_sphere_motor_orbit(t, fe);
    
    // 2. Apply terrain coupling (Height adjustment)
    //    This is an "effect" applied after the ideal motion
    let base_height = s.pos.y;  // orbit_height from motor + anchor
    let adjusted_height = coupling_terrain_to_sphere_orbit_height(
        vec2(s.pos.x, s.pos.z),
        base_height
    );
    
    // Update position with terrain-adjusted height
    s.pos.y = adjusted_height;
    
    // Note: Orientation from motor is already set by dynamics_sphere_motor_orbit
    // Color is set by coupling in main loop (not here)
    
    return s;
}

// --- Helper: Camera position from orbital parameters

fn compose_camera_position_from_orbit(aim_point: vec3<f32>, cam: CameraState) -> vec3<f32> {
    let cos_el = cos(cam.elevation);
    let sin_el = sin(cam.elevation);
    let cos_az = cos(cam.azimuth);
    let sin_az = sin(cam.azimuth);
    
    let forward = vec3(-cos_el * sin_az, -sin_el, -cos_el * cos_az);
    let right = vec3(cos_az, 0.0, -sin_az);
    let up = cross(right, forward);
    
    let look_at = aim_point + right * cam.pan_x + up * cam.pan_y;
    let offset = cam.distance * vec3(cos_el * sin_az, sin_el, cos_el * cos_az);
    
    return look_at + offset;
}

// §5.1 0D COMPOSITION — split into 4 entry points (§7.1):
//   update_player_agent, update_other_agents, update_camera, update_sphere
//   (update_terrain_config removed — RAYMARCH/SDF excavation)
struct VPMatrix {
    m: mat4x4<f32>,
    light_vp: mat4x4<f32>,
}

fn build_view_projection_matrix(
    eye: vec3<f32>,
    azimuth: f32,
    elevation: f32,
    aspect: f32
) -> mat4x4<f32> {
    // --- Camera frame (look-direction basis for the rasterization view matrix)
    let cos_el = cos(elevation);
    let sin_el = sin(elevation);
    let cos_az = cos(azimuth);
    let sin_az = sin(azimuth);

    let orbital = vec3(cos_el * sin_az, sin_el, cos_el * cos_az);
    let forward = -orbital;
    let right = vec3(cos_az, 0.0, -sin_az);
    let up = cross(orbital, right);

    // --- View matrix (world → camera space)
    //
    // Camera space: right → +X, up → +Y, -forward → +Z
    let view = mat4x4<f32>(
        vec4(right.x,   up.x,   -forward.x,  0.0),
        vec4(right.y,   up.y,   -forward.y,  0.0),
        vec4(right.z,   up.z,   -forward.z,  0.0),
        vec4(-dot(right, eye), -dot(up, eye), dot(forward, eye), 1.0)
    );

    // --- Perspective projection (WebGPU [0,1] depth range)
    let fov_y = CAMERA_FOV;  // 1.0 radians
    let near = 0.1;
    let far = 600.0;   // covers largest finite rooms (radius 4 → diagonal ~318)
    let f = 1.0 / tan(fov_y * 0.5);

    let proj = mat4x4<f32>(
        vec4(f / aspect,  0.0,  0.0,                        0.0),
        vec4(0.0,         f,    0.0,                        0.0),
        vec4(0.0,         0.0,  -far / (far - near),       -1.0),
        vec4(0.0,         0.0,  -near * far / (far - near),  0.0)
    );

    return proj * view;
}


// §6.1 LIGHTING SYSTEM
// Directional light (sun/moon) with shadow map + N point lights (lamp posts).
struct DirectionalLight {
    direction: vec3<f32>,     // normalized, points FROM light toward scene
    _pad0: f32,
    color: vec3<f32>,
    intensity: f32,
    ambient: f32,             // base ambient (low at night, higher at day)
    _pad1: f32,
    _pad2: f32,
    _pad3: f32,
}

const MAX_POINT_LIGHTS: u32 = 8u;

struct PointLight {
    position: vec3<f32>,
    range: f32,
    color: vec3<f32>,
    intensity: f32,
}

struct PointLightArray {
    count: u32,
    _pad0: u32,
    _pad1: u32,
    _pad2: u32,
    lights: array<PointLight, 8>,  // MAX_POINT_LIGHTS
}

struct SpotLight {
    position: vec3<f32>,
    _pad0: f32,
    direction: vec3<f32>,
    _pad1: f32,
    color: vec3<f32>,
    intensity: f32,
    inner_cone: f32,      // cos(inner angle)
    outer_cone: f32,      // cos(outer angle)
    range: f32,
    _pad2: f32,
    view_proj: mat4x4<f32>,
}

const MAX_SPOT_LIGHTS: u32 = 4u;

struct SpotLightArray {
    count: u32,
    _pad0: u32,
    _pad1: u32,
    _pad2: u32,
    lights: array<SpotLight, 4>,
}

// --- Shadow constants

const SHADOW_MAP_SIZE: f32 = 4096.0;
const SHADOW_BIAS_MIN: f32 = 0.0001;
const SHADOW_BIAS_MAX: f32 = 0.002;

// --- Shadow Sampling with 4x4 PCF

fn sample_shadow_pcf(world_pos: vec3<f32>, normal: vec3<f32>) -> f32 {
    // Transform to light clip space
    let light_clip = render_vp.light_vp * vec4(world_pos, 1.0);
    let light_ndc = light_clip.xyz / light_clip.w;

    let shadow_uv = vec2(
        light_ndc.x * 0.5 + 0.5,
        -light_ndc.y * 0.5 + 0.5
    );

    // Slope-scaled bias: more bias when surface is at grazing angle to light
    let light_dir = -render_light.direction;
    let cos_theta = max(dot(normal, light_dir), 0.0);
    let bias = mix(SHADOW_BIAS_MAX, SHADOW_BIAS_MIN, cos_theta);

    let current_depth = light_ndc.z - bias;

    // Safety: if somehow outside shadow map, return fully lit
    let out_of_bounds = shadow_uv.x < 0.0 || shadow_uv.x > 1.0 ||
                        shadow_uv.y < 0.0 || shadow_uv.y > 1.0 ||
                        light_ndc.z < 0.0 || light_ndc.z > 1.0;

    let clamped_uv = clamp(shadow_uv, vec2(0.001), vec2(0.999));

    // 4x4 PCF kernel — always execute (uniform control flow)
    let texel_size = 1.0 / SHADOW_MAP_SIZE;
    var shadow: f32 = 0.0;
    for (var y: i32 = -2; y <= 1; y++) {
        for (var x: i32 = -2; x <= 1; x++) {
            shadow += textureSampleCompare(
                shadow_map,
                shadow_sampler,
                clamped_uv + vec2(f32(x), f32(y)) * texel_size,
                current_depth
            );
        }
    }
    let pcf_result = shadow / 16.0;

    return select(pcf_result, 1.0, out_of_bounds);
}

// --- Directional Light

fn calc_directional_light(world_pos: vec3<f32>, normal: vec3<f32>) -> vec3<f32> {
    let light_dir = -render_light.direction;  // toward light
    let ndotl = max(dot(normal, light_dir), 0.0);

    // Shadow: skip when spot lights are active — light_vp is being used
    // for spot atlas tiles, so the directional PCF would sample wrong.
    var shadow = 1.0;
    if (render_spot_lights.count == 0u) {
        shadow = sample_shadow_pcf(world_pos, normal);
    }

    return render_light.color * render_light.intensity * ndotl * shadow;
}

// --- Point Lights (diffuse only, no shadows)

fn calc_point_lights(world_pos: vec3<f32>, normal: vec3<f32>) -> vec3<f32> {
    var total = vec3(0.0);
    let count = min(render_point_lights.count, MAX_POINT_LIGHTS);

    for (var i: u32 = 0u; i < count; i++) {
        let light = render_point_lights.lights[i];
        let light_vec = light.position - world_pos;
        let dist = length(light_vec);
        let light_dir = light_vec / max(dist, 0.001);

        // Smooth distance attenuation
        let attenuation = clamp(1.0 - dist / light.range, 0.0, 1.0);
        let attenuation_sq = attenuation * attenuation;

        // Diffuse
        let ndotl = max(dot(normal, light_dir), 0.0);

        total += light.color * light.intensity * attenuation_sq * ndotl;
    }
    return total;
}

// --- Spot Light Shadow Sampling (perspective, two-texture atlas)
//
// Two 4096×4096 depth textures, each split left/right into 2048×4096 tiles:
//   shadow_map      (repurposed sun map) → lights 0, 1
//   spot_shadow_map                      → lights 2, 3
// Doubles per-tile resolution vs the old single-texture 2×2 grid,
// with zero extra VRAM — the sun map is idle during indoor moods.
//
// Bias strategy for terrain under perspective:
//   1. Distance-scaled depth bias — divided by clip.w so it compensates
//      for hyperbolic 1/z depth distribution. Near surfaces get larger
//      bias (high precision), far surfaces get less.
//   2. Per-pixel slope bias — computed from radial light direction
//      (rays fan from point source). Steep slopes get more bias.
//
// No normal offset — it breaks contact shadows (disconnects pawn shadow
// from feet by lifting the comparison point above the occluder depth).

const SPOT_DEPTH_BIAS: f32 = 0.0015;        // base bias, scaled by 1/clip.w
const SPOT_SLOPE_BIAS_MAX: f32 = 0.005;     // extra bias at grazing angles

fn sample_spot_shadow_pcf(world_pos: vec3<f32>, normal: vec3<f32>, light_index: u32) -> f32 {
    let light = render_spot_lights.lights[light_index];

    // Transform to light clip space (perspective)
    let light_clip = light.view_proj * vec4(world_pos, 1.0);
    let light_ndc = light_clip.xyz / light_clip.w;

    // NDC to UV (flip Y), then scale + offset into atlas tile.
    // Each texture holds 2 tiles side by side (left/right halves).
    // Tile = half width (0.5), full height (1.0).
    let raw_uv = vec2(
        light_ndc.x * 0.5 + 0.5,
        -light_ndc.y * 0.5 + 0.5
    );
    let within = light_index % 2u;
    let tile_offset = vec2(f32(within) * 0.5, 0.0);
    let shadow_uv = raw_uv * vec2(0.5, 1.0) + tile_offset;

    // Distance-scaled depth bias + per-pixel slope bias.
    // clip.w floor = 1.0 matches the projection near plane — no visible
    // fragment should have clip.w below this, so clamping prevents runaway
    // bias from numerical edge cases near the frustum boundary.
    let light_dir = normalize(light.position - world_pos);
    let cos_theta = max(dot(normal, light_dir), 0.001);
    let slope_bias = SPOT_SLOPE_BIAS_MAX * (1.0 - cos_theta) / cos_theta;
    let total_bias = (SPOT_DEPTH_BIAS + slope_bias) / max(light_clip.w, 1.0);

    let current_depth = light_ndc.z - total_bias;

    let out_of_bounds = raw_uv.x < 0.0 || raw_uv.x > 1.0 ||
                        raw_uv.y < 0.0 || raw_uv.y > 1.0 ||
                        current_depth < 0.0 || current_depth > 1.0;

    let clamped_uv = clamp(shadow_uv,
        tile_offset + vec2(0.001, 0.001),
        tile_offset + vec2(0.499, 0.999));
    let clamped_depth = clamp(current_depth, 0.0, 1.0);

    // 4x4 PCF kernel — branch on texture (lights 0-1 on sun map, 2-3 on spot map)
    let texel_size = 1.0 / SHADOW_MAP_SIZE;
    var shadow: f32 = 0.0;
    if (light_index < 2u) {
        for (var y: i32 = -2; y <= 1; y++) {
            for (var x: i32 = -2; x <= 1; x++) {
                let offset = vec2(f32(x) + 0.5, f32(y) + 0.5) * texel_size;
                shadow += textureSampleCompare(
                    shadow_map,
                    shadow_sampler,
                    clamped_uv + offset,
                    clamped_depth
                );
            }
        }
    } else {
        for (var y: i32 = -2; y <= 1; y++) {
            for (var x: i32 = -2; x <= 1; x++) {
                let offset = vec2(f32(x) + 0.5, f32(y) + 0.5) * texel_size;
                shadow += textureSampleCompare(
                    spot_shadow_map,
                    shadow_sampler,
                    clamped_uv + offset,
                    clamped_depth
                );
            }
        }
    }
    return select(shadow / 16.0, 0.0, out_of_bounds);
}

// --- Spot Lights (cone + distance + shadow atlas, indoor moods)

fn calc_spot_light(world_pos: vec3<f32>, normal: vec3<f32>) -> vec3<f32> {
    var total = vec3(0.0);
    let count = min(render_spot_lights.count, MAX_SPOT_LIGHTS);

    for (var i: u32 = 0u; i < count; i++) {
        let light = render_spot_lights.lights[i];
        if (light.intensity <= 0.0) { continue; }

        let light_vec = light.position - world_pos;
        let dist = length(light_vec);
        let light_dir = light_vec / max(dist, 0.001);

        // Distance attenuation
        let attenuation = clamp(1.0 - dist / light.range, 0.0, 1.0);
        let attenuation_sq = attenuation * attenuation;

        // Cone falloff
        let spot_cos = dot(-light_dir, light.direction);
        let cone_falloff = smoothstep(light.outer_cone, light.inner_cone, spot_cos);

        // Diffuse
        let ndotl = max(dot(normal, light_dir), 0.0);

        // Per-light shadow from atlas tile
        let shadow = sample_spot_shadow_pcf(world_pos, normal, i);

        total += light.color * light.intensity * attenuation_sq * cone_falloff * ndotl * shadow;
    }
    return total;
}

// --- Unified Shading
// THE RIM dither knob's noise — world-space stipple so the dissolve sticks
// to the geometry (it "condenses" rather than screen-crawls). Taste knob;
// off by default.
fn veil_dither_noise(p: vec2<f32>) -> f32 {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

// veil_scale: 1.0 = the family joins the veil (terrain + all entity_fs
// users); 0.0 = a ruled exemption (ribbon — a flown structure that shares
// ENTITY_FS but must stay visible at range). NOT an anchor knob — the
// veil always measures from the point.
fn shade_lit(world_pos: vec3<f32>, normal: vec3<f32>, base_color: vec3<f32>, veil_scale: f32) -> vec3<f32> {
    // Ambient (always present)
    let ambient = base_color * render_light.ambient;

    // Directional light with shadows
    let sun = base_color * calc_directional_light(world_pos, normal);

    // Point lights (diffuse only)
    let points = base_color * calc_point_lights(world_pos, normal);

    // Spot light (cone + distance, indoor moods)
    let spot = base_color * calc_spot_light(world_pos, normal);

    let lit = ambient + sun + points + spot;

    // Fog — the EYE-anchored atmospheric term (a view effect; stays).
    let dist = distance(world_pos, render_camera.pos);
    let fog = 1.0 - exp(-dist * config.fog_density);
    let fogged = mix(lit, config.fog_color, fog);

    // THE ICING (re-ruled) — the POINT-anchored fade band AT the ring,
    // composed AFTER the eye-fog. Cosmetic, not concealment: the RING is
    // the draw authority (nothing is drawn beyond it), and this narrow
    // band [ring−δ, ring] is where draw-set joins materialize. Zero
    // inside ring−δ (pixel-identical there); full fog/horizon color at
    // the ring (the rim melts into sky). strength: 0 in finite/indoor.
    let point_d = distance(world_pos.xz, render_point_pos().xz);
    let veil = smoothstep(config.veil_ring - config.veil_icing, config.veil_ring, point_d)
             * config.veil_strength * veil_scale;
    // THE RIM taste knob: veil_dither > 0.5 → the icing band DITHER-
    // dissolves (geometry condenses) instead of tinting to fog.
    if (config.veil_dither > 0.5) {
        if (veil_dither_noise(world_pos.xz) < veil) { discard; }
        return fogged;
    }
    return mix(fogged, config.fog_color, veil);
}


// §6.2 PATCH TERRAIN RENDERING
// Instanced rendering of streaming terrain patches. Each instance is one
struct PatchTerrainVarying {
    @builtin(position) clip_pos: vec4<f32>,
    @location(0) world_pos: vec3<f32>,
    @location(1) gradients: vec2<f32>,
    // (complexity varying REMOVED — LATENT[complexity], read by no FS)
    @location(2) patch_uv: vec2<f32>,    // UV within the patch [0,1] for cell sampling
    @location(3) @interpolate(flat) layer: u32,  // heightfield/cell array layer
    // TEMPORARY (INCIDENT #2, I3): 1.0 on skirt ring-copy verts, 0.0 on
    // the surface — wall fragments interpolate toward 1. Remove with
    // the instruments after conviction.
    @location(4) skirt: f32,
}

// patch_terrain_vs — hand-fused POLICY_TERRAIN_RENDER evaluation.
//
// Inlines the contributor sum for per-vertex performance:
//   patch heightfield texture (cached CONTRIB_STATIC_BASE + CONTRIB_PYRAMIDS)
//   + CONTRIB_PAWN_AURA
//   + CONTRIB_TERRAIN_WAVES
//   + CONTRIB_RADIAL_PULSES
//
// Does NOT include CONTRIB_GOL_ZONES — the patch heightfield does not
// cache GoL; zones are rendered as a separate extrusion pass.
//
// Uses terrain_wave_overlay_with_gradient (not contrib_terrain_waves_at)
// because the gradient is needed for the fragment normal and is computed
// analytically in the same loop pass.
//
// Keep consistent with POLICY_TERRAIN_RENDER: if that policy's mask
// gains or loses a contributor, update this function to match — or
// document the divergence at the mask block. The patch VS runs ~256×256
// invocations per patch, so a function-call-per-contributor dispatch
// would dominate frame time; that's why this stays hand-fused.
//
// See contracts/ground_architecture.hpp (fused inline evaluations).
@vertex
fn patch_terrain_vs(
    @builtin(vertex_index) vi: u32,
    @builtin(instance_index) patch_id: u32
) -> PatchTerrainVarying {
    // Direct or indirect patch lookup (override-controlled per pipeline variant)
    var actual_id = patch_id;
    if (USE_PATCH_INDIRECTION) { actual_id = visible_patch_indices[patch_id]; }
    let pi = patch_instances[actual_id];

    // THE UNIFIED DECODE (UNIFIED_GROUND_1): legacy grid + skirt copies
    // + cap band + curtain-bottom band — one arithmetic split (Dim::UG_*).
    let d = ug_decode(vi);

    // UV within the patch [0, 1]
    let uv = vec2(
        f32(d.vx) / f32(PATCH_MESH_N),
        f32(d.vz) / f32(PATCH_MESH_N)
    );

    // Remap UV to align with texel centers in the heightfield.
    let res = f32(PATCH_HEIGHTFIELD_N);
    let sample_uv = (uv * (res - 1.0) + 0.5) / res;

    // Sample heightfield from this patch's array layer
    // .x = height, .yz = gradients, .w = unused (was complexity — swept)
    let height_data = textureSampleLevel(
        patch_heightfield_array_read, bilinear_sampler,
        sample_uv, i32(pi.layer), 0.0
    );

    // World position: patch origin + local offset
    let half = pi.extent * 0.5;
    var world_pos = vec3(
        pi.origin.x + (uv.x - 0.5) * pi.extent,
        height_data.x,
        pi.origin.y + (uv.y - 0.5) * pi.extent
    );

    // Pawn aura: raise terrain under the pawn's influence footprint
    // Uses config.pawn_aura_height so terrain and pawn always agree (includes expansion + presence ramp)
    let aura = sample_pawn_aura(world_pos.xz, render_pawn_pos().xz);
    world_pos.y += aura.r * config.pawn_aura_height;

    // The live card (GROUND_CARD_1): waves + pulses ride one field —
    // live.x = Δh (waves+pulses), live.yz = waves-only gradient
    // (parity with the old fused overlay; pulse shading = Stage 6).
    let live = sample_live_card(world_pos.xz);
    world_pos.y += live.x;

    // THE CELL LIFT (UNIFIED_GROUND_1): GoL rides the ground itself —
    // the card's .a, nearest at the vertex's OWN cell center, pawn-
    // suppressed. BASE verts take no lift (that gap IS the curtain);
    // d.drop subsumes the old skirt ring drop.
    let lift = ug_cell_lift(pi.origin, pi.extent, d.cellx, d.cellz)
             * pawn_gol_suppression(world_pos.xz, render_pawn_pos().xz);
    world_pos.y += lift * d.lift_scale - d.drop;

    var out: PatchTerrainVarying;
    out.clip_pos = render_vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.gradients = height_data.yz + live.yz;
    // (out.complexity REMOVED — the LATENT[complexity] varying;
    //  the .w channel it read is now unused, no FS ever consumed it.)
    out.patch_uv = uv;
    out.layer = pi.layer;
    out.skirt = d.wall;   // the INCIDENT-#2 instrument, generalized — 1 on
                          // curtain-bottom + skirt copies (wall fragments
                          // interpolate toward 1)
    return out;
}

// THE COMPOSITION ORDER (Phase 2 D3 — ruling 4, now declared law; the
// color sibling of THE FOLD above manifold_height_hf): this FS IS the
// one declared place the color composition runs, in this order —
//
//   0 rim discard → 1 baked color → 2 live analytic recolor (REPLACES)
//   → 3 GoL tint → 4 pawn FF (nested) → 5 sphere FF (nested)
//   → 6 aura color delta → 7 aura brighten → 8 aura normal perturb
//   → 9 shade_lit (fog/veil last)
//
// Color guests do NOT commute (mix vs additive vs replace) — the order
// is semantically load-bearing. Adopted verbatim from the emergent
// order (charter C6). Reordering is a RULING, not a refactor.
@fragment
fn patch_terrain_fs(in: PatchTerrainVarying) -> @location(0) vec4<f32> {
    // THE RIM — the terrain sibling of the flora/zone per-vertex kill:
    // discard beyond the ring so the visible edge is a smooth CIRCLE (the
    // patch-granular banded draw set is its invisible superset). Staged
    // point (lod_point) — concentric with the flora/zone/instance kills, so
    // every hard draw-set edge is ONE circle (no scallops, no silhouettes).
    // Optional dither-dissolve inside the icing band handled in shade_lit.
    if (distance(in.world_pos.xz, vec2(config.lod_point_x, config.lod_point_z)) > config.veil_ring) {
        discard;
    }

    // THE DEBUG EYE (GROUND_CARD_1): R = |Δh|, G = GoL lift. Card black
    // at rest; paints under music / near zones. After the rim discard so
    // the eye respects the veil ring.
    if (LIVE_CARD_DEBUG_VIEW == 1u) {
        let c = sample_live_card(in.world_pos.xz);
        return vec4(clamp(abs(c.x) * 0.25, 0.0, 1.0),
                    clamp(c.w * 0.25, 0.0, 1.0),
                    0.0, 1.0);
    }

    var normal = normalize(vec3(-in.gradients.x, 1.0, -in.gradients.y));

    // THE ONE-ADDRESS LAW (charter C8, SEAMLESSNESS corollary): this
    // fragment's cell is cell_address(world_pos.xz) — the SAME address
    // every hash and roll derives from. The texel is COMPUTED FROM that
    // address by inverting the VS mapping (§6.2 patch_terrain_vs):
    //     world_pos.xz = pi.origin + (uv - 0.5) * pi.extent
    //     out.patch_uv = uv
    // so min_corner = world_pos.xz - patch_uv * PATCH_EXTENT, and the
    // patch grid index = round(min_corner / PATCH_EXTENT) — exact, because
    // origins are (g + 0.5) * PATCH_EXTENT and extent ≡ PATCH_EXTENT for
    // every LOD ring (patch_system.hpp:392/586). round() snaps
    // interpolation noise; patch_uv recovers only the per-patch CONSTANT —
    // it never addresses a texel by itself again.
    let patch_grid = vec2<i32>(round((in.world_pos.xz - in.patch_uv * PATCH_EXTENT) / PATCH_EXTENT));
    let cell_texel = clamp(
        cell_address(in.world_pos.xz) - patch_grid * i32(PATCH_CELL_N),
        vec2(0), vec2(i32(PATCH_CELL_N) - 1));
    // OWNERSHIP RESOLUTION (INCIDENT #3): the cell this fragment READS
    // is the cell it is PAINTED FROM — the clamped texel's cell. A
    // border-sliver fragment (rendered by this patch, world floor in
    // the neighbor's first cell) re-homes to the owned edge cell, so
    // FIELDS (this texel) and HASHES (this address) can never mix
    // cells — the chimera is inexpressible. In-domain fragments:
    // raw == clamped ⇒ addr_used == the world floor, bit-identical.
    let addr_used = patch_grid * i32(PATCH_CELL_N) + cell_texel;

    // Color fully composited at gen-time in the cell texture.
    // Alpha carries the cell behavior tag (0.0 = static, nonzero = animated).
    // One-address: loaded at the law texel (was a nearest-neighbor SAMPLE
    // by patch_uv — the second addressing that made the seam expressible).
    let color_sample = textureLoad(
        patch_cell_color_array_read, cell_texel, i32(in.layer), 0);

    var base_color = color_sample.rgb;

    // --- Musical animation modes: re-evaluate cell color with biases
    // Runtime guard: every live color channel at rest -> skip the live
    // re-evaluation (the baked gen-time composite stands). Drivers are gen-2
    // couplings through the visual canvas; the mood retired as author.
    // CHECKER-REBUILD: the checker field is live whenever music_amount
    // rises off 0 (the pull, the wander, and the door all key off it).
    // SIGNED DOORS (charter): the door biases are axes centered on rest.
    let has_mode_bias = (abs(config.mode_color_shift) > 0.001)
                     || (abs(config.mode_checker_scatter) > 0.001)
                     || (config.mode_palette_intensity > 0.001)
                     || (config.checker_music_amount > 0.001);
    if (CHECKER_DEBUG_VIEW == 1u) {
        // COLOR METER: the resultant color as the shader receives it,
        // faded by presence (gray at rest). Static gray under music = the
        // CPU->GPU path is cut.
        base_color = mix(vec3(0.5), config.checker_resultant, config.checker_music_amount);
    } else if (CHECKER_DEBUG_VIEW == 2u) {
        // FIELD-COVERAGE VIEW: green where the live path runs
        // (has_mode_bias), gray where the baked composite stands.
        // The full instrument registry arrives Phase 4.
        base_color = select(vec3(0.45), vec3(0.25, 0.7, 0.35), has_mode_bias);
    } else if (has_mode_bias) {
        // SAMPLE-POINT + ANALYTIC (charter C8): the bake's evaluator,
        // at the cell center — one function, two moments, no cache.
        let cell_center = (vec2<f32>(addr_used) + 0.5) * (PATCH_EXTENT / f32(PATCH_CELL_N));
        base_color = animated_cell_color(cell_center, addr_used);
    }

    // ── TERRAIN_DEBUG_VIEW — the instrument registry's terrain slots.
    //    (INCIDENT #2's I1 texel audit / I2 LUT field audit RETIRED —
    //    duty served, all five suspects exonerated; the skirt paint
    //    stays as the permanent slot 3.) Painted AFTER the music
    //    branch so each view shows the live-path truth with music
    //    playing. Shading/fog still compose after — legible.
    if (TERRAIN_DEBUG_VIEW == 3u) {
        // SKIRT PAINT (permanent) — skirt fragments magenta over the
        // art: shows where the perimeter curtains present as pixels.
        if (in.skirt > 0.01) {
            base_color = vec3(1.0, 0.0, 1.0);
        }
    } else if (TERRAIN_DEBUG_VIEW == 4u) {
        // ZONE-GEOMETRY SCULPTING ROOM (permanent) — computed LIVE in
        // the FS, never from a bake, so it shows the field AS
        // CURRENTLY DEFINED (post-warp) at the current dials with no
        // rebake (since Commit C the art's live path evaluates the
        // SAME field — the instrument and the artwork agree):
        //   grayscale = the mode field · thin dark lines = actual
        //   patch borders (world grid every PATCH_EXTENT) · red
        //   isoline = the zone coastline (|mode − threshold| < eps).
        // Conviction key: zone edges IGNORE the border lines →
        // lattice anisotropy convicted (warp aimed right); edges HUG
        // the lines → STOP, send the shot — new hunt.
        let m = mode_field_at(in.world_pos.xz);
        var c = vec3(m);
        let bf = vec2(fract(in.world_pos.x / PATCH_EXTENT),
                      fract(in.world_pos.z / PATCH_EXTENT));
        let border_d = min(min(bf.x, 1.0 - bf.x), min(bf.y, 1.0 - bf.y)) * PATCH_EXTENT;
        if (border_d < 0.15) { c = vec3(0.05); }
        const COAST_EPS: f32 = 0.015;
        if (abs(m - MODE_DISCRETE_THRESHOLD) < COAST_EPS) { c = vec3(0.9, 0.1, 0.1); }
        base_color = c;
    }

    // --- GoL zone visualization
    // Runtime guard: cell behavior tag alpha is nonzero only inside active zones.
    let tag_alpha = color_sample.a;
    if (tag_alpha > 0.001) {
        let mode = unpack_cell_tag_mode(tag_alpha);
        if ((mode & CELL_ANIM_GOL) != 0u) {
            let cam_dist = distance(render_camera.pos, in.world_pos);
            let fade = 1.0 - smoothstep(GOL_FADE_NEAR, GOL_FADE_FAR, cam_dist);

            if (fade > 0.01) {
                let zone_node = vec2<i32>(floor(in.world_pos.xz / MODE_LATTICE_SPACING));

                for (var z: u32 = 0u; z < zone_params.count; z++) {
                    let zp = zone_params.zones[z];
                    if (zp.transition_fraction <= 0.0) { continue; }
                    let zn = vec2<i32>(floor(zp.origin / MODE_LATTICE_SPACING));
                    if (zn.x == zone_node.x && zn.y == zone_node.y) {
                        let zone_corner = zp.origin - zp.extent * 0.5;
                        let cell_size = zp.extent / f32(zp.grid_size);
                        let rel = in.world_pos.xz - zone_corner;
                        let local_cell = vec2<i32>(floor(rel / cell_size));

                        if (local_cell.x < 0 || local_cell.x >= i32(zp.grid_size) ||
                            local_cell.y < 0 || local_cell.y >= i32(zp.grid_size)) { break; }

                        let uv = (vec2<f32>(local_cell) + 0.5) / f32(zp.grid_size);
                        let life_sample = textureSampleLevel(
                            zone_life_read, nearest_sampler, uv, i32(z), 0.0
                        );
                        let color_val = life_sample.y;  // G channel = color spring

                        if (color_val > 0.01) {
                            base_color = apply_gol_color(
                                base_color, zp,
                                u32(local_cell.x), u32(local_cell.y),
                                color_val * fade
                            );

                            // Pawn force field: tint zone cells near pawn (render context)
                            let pawn_ff = 1.0 - zone_pawn_ff(in.world_pos.xz, render_pawn_pos(), render_pawn_vel_xz());
                            if (pawn_ff > 0.01) {
                                base_color = mix(base_color, ZONE_PAWN_TINT, pawn_ff * ZONE_PAWN_TINT_STRENGTH * color_val);
                            }

                            // Sphere force field: tint zone cells near sphere (render context)
                            let sphere_ff = 1.0 - zone_sphere_ff(in.world_pos.xz, render_floating.entities[0].pos);
                            if (sphere_ff > 0.01) {
                                base_color = mix(base_color, ZONE_SPHERE_TINT, sphere_ff * ZONE_SPHERE_TINT_STRENGTH * color_val);
                            }
                        }
                        break;
                    }
                }
            }
        }
    }

    // --- Pawn aura: persistent contextual tinting from toroidal spring grid
    // Texture encoding: R=height_blend, GBA=pre-multiplied color delta (with oscillation)
    // Runtime guard: sample returns near-zero when the aura system is idle.
    {
        let aura = sample_pawn_aura(in.world_pos.xz, render_pawn_pos().xz);
        let aura_active = max(aura.r, max(abs(aura.g), max(abs(aura.b), abs(aura.a))));
        if (aura_active > 0.01) {
            // Color oscillation: GBA already modulated by compute
            base_color = clamp(base_color + aura.gba, vec3(0.0), vec3(1.0));
            // Height contribution: brighten terrain proportional to R (simulates raised surface)
            let height_boost = aura.r * 0.15;
            base_color = clamp(base_color + vec3(height_boost), vec3(0.0), vec3(1.0));
            // Perturb normal slightly upward (simulates raised terrain catching more light)
            normal = normalize(normal + vec3(0.0, aura.r * 0.3, 0.0));
        }
    }

    return vec4(shade_lit(in.world_pos, normal, base_color, 1.0), 1.0);
}

// Shadow pass variant — same geometry, light VP instead of camera VP.
// THE RIM (flagged, not chased): the shadow pass is DEPTH-ONLY (no FS), so
// the visible rim's per-fragment discard has no equivalent here — terrain
// (and flora/zone, whose shadow VS also omit the ring kill) cast shadows
// out to the patch-granular banded set, ~one patch (≤50wu) beyond the
// smooth visible rim. Logged for the rig; a shadow-side ring gate is a
// follow-on only if it reads as a shadow with no visible caster.
@vertex
fn shadow_patch_terrain_vs(
    @builtin(vertex_index) vi: u32,
    @builtin(instance_index) patch_id: u32
) -> ShadowVarying {
    let pi = patch_instances[patch_id];

    // Same unified decode as patch_terrain_vs — the shadow pass shares
    // the patch index buffers (cap + curtain + skirt bands).
    let d = ug_decode(vi);

    let uv = vec2(
        f32(d.vx) / f32(PATCH_MESH_N),
        f32(d.vz) / f32(PATCH_MESH_N)
    );

    let res = f32(PATCH_HEIGHTFIELD_N);
    let sample_uv = (uv * (res - 1.0) + 0.5) / res;

    let height_data = textureSampleLevel(
        patch_heightfield_array_read, bilinear_sampler,
        sample_uv, i32(pi.layer), 0.0
    );

    let wx = pi.origin.x + (uv.x - 0.5) * pi.extent;
    let wz = pi.origin.y + (uv.y - 0.5) * pi.extent;
    var world_pos = vec3(wx, height_data.x + sample_live_card(vec2(wx, wz)).x, wz);
    // THE CELL LIFT (UNIFIED_GROUND_1) — shadow surface = heightfield +
    // the card + the pawn-suppressed cell lift; d.drop subsumes the old
    // skirt ring drop.
    let lift = ug_cell_lift(pi.origin, pi.extent, d.cellx, d.cellz)
             * pawn_gol_suppression(world_pos.xz, render_pawn_pos().xz);
    world_pos.y += lift * d.lift_scale - d.drop;

    var out: ShadowVarying;
    out.clip_pos = render_vp.light_vp * vec4(world_pos, 1.0);
    return out;
}


// §6.3 ENTITY RENDERING — Pawn and Sphere
// Pawn: Chess pawn profile, GPU-generated from @builtin(vertex_index).
struct MeshVertexInput {
    @location(0) pos: vec3<f32>,
    @location(1) normal: vec3<f32>,
}

struct EntityVarying {
    @builtin(position) clip_pos: vec4<f32>,
    @location(0) world_pos: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) entity_color: vec3<f32>,
}


// --- Chess Pawn Profile
fn pawn_profile_radius(t: f32) -> f32 {
    // Base section: t = 0.00 to 0.14
    if (t < 0.04) {
        return mix(0.75, 0.88, t / 0.04);
    }
    if (t < 0.10) {
        let u = (t - 0.04) / 0.06;
        return mix(0.88, 0.92, sin(u * PI * 0.5));
    }
    if (t < 0.14) {
        let u = (t - 0.10) / 0.04;
        return mix(0.92, 0.70, u * u);
    }

    // Body section: t = 0.14 to 0.50
    if (t < 0.50) {
        let u = (t - 0.14) / 0.36;
        let ease = smoothstep(0.0, 1.0, u);
        return mix(0.70, 0.22, ease);
    }

    // Neck section: t = 0.50 to 0.62
    if (t < 0.62) {
        let u = (t - 0.50) / 0.12;
        return mix(0.22, 0.18, u);
    }

    // Collar section: t = 0.62 to 0.70
    if (t < 0.70) {
        let u = (t - 0.62) / 0.08;
        let bulge = sin(u * PI);
        return 0.18 + 0.08 * bulge;
    }

    // Head section: t = 0.70 to 0.95
    if (t < 0.95) {
        let u = (t - 0.70) / 0.25;
        let y_normalized = u * 2.0 - 1.0;
        let sphere_r = sqrt(max(0.0, 1.0 - y_normalized * y_normalized));
        return 0.15 + 0.35 * sphere_r;
    }

    // Tip: t = 0.95 to 1.00
    let u = (t - 0.95) / 0.05;
    return mix(0.15, 0.0, smoothstep(0.0, 1.0, u));
}

// Compute outward-facing normal from profile slope via finite differences
fn pawn_profile_normal_2d(t: f32) -> vec2<f32> {
    let eps = 0.005;
    let t0 = max(0.0, t - eps);
    let t1 = min(1.0, t + eps);

    let r0 = pawn_profile_radius(t0);
    let r1 = pawn_profile_radius(t1);
    let h0 = t0;
    let h1 = t1;

    let dr = r1 - r0;
    let dh = h1 - h0;

    let len = sqrt(dr * dr + dh * dh);
    if (len < 0.0001) {
        return vec2(1.0, 0.0);
    }
    return vec2(dh / len, -dr / len);
}


// --- Pawn Vertex Shader (chess pawn, GPU-generated)

@vertex
fn pawn_vs(@builtin(vertex_index) vid: u32,
           @builtin(instance_index) inst: u32) -> EntityVarying {
    let agent = render_agents[inst];
    // Collapse inactive slots to a degenerate point at the agent's pos.
    // (Same trick as sphere_vs: zero-scale local geometry → no fragments.)
    // THE RING (draw authority): agents exist to 350 but DRAW only inside
    // the ring. The pawn is NOT exempt (ruled): in camera-host the
    // abandoned body leaves the draw set with everything else.
    let agent_in_ring = distance(vec2(agent.pos_x, agent.pos_z),
                                 vec2(config.lod_point_x, config.lod_point_z))
                        - 5.0 <= config.veil_ring;   // 5 wu: agent body half-extent
    let active_f = f32(agent.is_active) * f32(agent_in_ring);

    var local_pos: vec3<f32>;
    var local_normal: vec3<f32>;

    if (vid < PAWN_BODY_VERTICES) {
        // Body quads: each band connects ring[i] to ring[i+1]
        let verts_per_band = PAWN_SEGMENTS * 6u;
        let band = vid / verts_per_band;
        let band_vid = vid % verts_per_band;
        let seg = band_vid / 6u;
        let tri_vert = band_vid % 6u;

        let ring_lo = band;
        let ring_hi = band + 1u;

        let seg_next = (seg + 1u) % PAWN_SEGMENTS;
        let angle0 = f32(seg) / f32(PAWN_SEGMENTS) * 2.0 * PI;
        let angle1 = f32(seg_next) / f32(PAWN_SEGMENTS) * 2.0 * PI;

        let t_lo = f32(ring_lo) / f32(PAWN_RINGS - 1u);
        let t_hi = f32(ring_hi) / f32(PAWN_RINGS - 1u);

        let r_lo = pawn_profile_radius(t_lo) * PAWN_RADIUS;
        let r_hi = pawn_profile_radius(t_hi) * PAWN_RADIUS;

        let y_lo = t_lo * PAWN_HEIGHT;
        let y_hi = t_hi * PAWN_HEIGHT;

        let p00 = vec3(cos(angle0) * r_lo, y_lo, sin(angle0) * r_lo);
        let p10 = vec3(cos(angle1) * r_lo, y_lo, sin(angle1) * r_lo);
        let p01 = vec3(cos(angle0) * r_hi, y_hi, sin(angle0) * r_hi);
        let p11 = vec3(cos(angle1) * r_hi, y_hi, sin(angle1) * r_hi);

        let n2d_lo = pawn_profile_normal_2d(t_lo);
        let n2d_hi = pawn_profile_normal_2d(t_hi);

        let n00 = normalize(vec3(n2d_lo.x * cos(angle0), n2d_lo.y, n2d_lo.x * sin(angle0)));
        let n10 = normalize(vec3(n2d_lo.x * cos(angle1), n2d_lo.y, n2d_lo.x * sin(angle1)));
        let n01 = normalize(vec3(n2d_hi.x * cos(angle0), n2d_hi.y, n2d_hi.x * sin(angle0)));
        let n11 = normalize(vec3(n2d_hi.x * cos(angle1), n2d_hi.y, n2d_hi.x * sin(angle1)));

        switch tri_vert {
            case 0u: { local_pos = p00; local_normal = n00; }
            case 1u: { local_pos = p10; local_normal = n10; }
            case 2u: { local_pos = p01; local_normal = n01; }
            case 3u: { local_pos = p01; local_normal = n01; }
            case 4u: { local_pos = p10; local_normal = n10; }
            case 5u: { local_pos = p11; local_normal = n11; }
            default: { local_pos = p00; local_normal = n00; }
        }

    } else {
        // Bottom cap: fan triangles
        let cap_vid = vid - PAWN_BODY_VERTICES;
        let seg = cap_vid / 3u;
        let tri_vert = cap_vid % 3u;

        let seg_next = (seg + 1u) % PAWN_SEGMENTS;
        let angle0 = f32(seg) / f32(PAWN_SEGMENTS) * 2.0 * PI;
        let angle1 = f32(seg_next) / f32(PAWN_SEGMENTS) * 2.0 * PI;

        let r = pawn_profile_radius(0.0) * PAWN_RADIUS;

        local_normal = vec3(0.0, -1.0, 0.0);

        switch tri_vert {
            case 0u: { local_pos = vec3(0.0, 0.0, 0.0); }
            case 1u: { local_pos = vec3(cos(angle1) * r, 0.0, sin(angle1) * r); }
            case 2u: { local_pos = vec3(cos(angle0) * r, 0.0, sin(angle0) * r); }
            default: { local_pos = vec3(0.0, 0.0, 0.0); }
        }
    }

    // Per-instance transform from this agent slot. orient_* carries
    // heading + (player only) terrain tilt; pos_* is the world XZ + Y.
    let pawn_q = vec4(agent.orient_x, agent.orient_y, agent.orient_z, agent.orient_w);
    let pawn_p = vec3(agent.pos_x, agent.pos_y, agent.pos_z);
    let rotated_pos = quat_rotate(pawn_q, local_pos * active_f);
    let rotated_normal = quat_rotate(pawn_q, local_normal);

    // Body color — per-agent palette pick (resolved CPU-side at spawn and
    // carried per-instance). The per-tier color is the fallback when a slot
    // carries no per-agent color (all-zero) — e.g. the never-possessed player.
    let tier = min(agent.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let tg = agent_tier_gains[tier];
    let agent_color = vec3(agent.color_r, agent.color_g, agent.color_b);
    let body_color = select(vec3(tg.color_r, tg.color_g, tg.color_b),
                            agent_color, any(agent_color > vec3(0.0)));

    var out: EntityVarying;
    out.clip_pos = render_vp.m * vec4(rotated_pos + pawn_p, 1.0);
    out.world_pos = rotated_pos + pawn_p;
    out.normal = rotated_normal;
    out.entity_color = body_color;
    return out;
}

@vertex
fn sphere_vs(@builtin(instance_index) inst: u32, in: MeshVertexInput) -> EntityVarying {
    let fe = render_floating.entities[inst];
    // Skip non-sphere geometry (degenerate triangle for rasterizer discard).
    // THE RING (draw authority): floaters exist to 400 (the flagged spawn-
    // headroom fork) but DRAW only inside the ring — center − extent ≤ ring.
    let in_ring = distance(fe.pos.xz, vec2(config.lod_point_x, config.lod_point_z))
                  - fe.body_radius <= config.veil_ring;
    let r = select(0.0, fe.body_radius, fe.geometry_type == 0u && fe.is_active != 0u && in_ring);
    let world_pos = in.pos * r + fe.pos;

    var out: EntityVarying;
    out.clip_pos = render_vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.normal = in.normal;
    out.entity_color = fe.color;
    return out;
}

@fragment
fn entity_fs(in: EntityVarying) -> @location(0) vec4<f32> {
    return vec4(shade_lit(in.world_pos, normalize(in.normal), in.entity_color, 1.0), 1.0);
}

// Ribbon FS — same shading as entity_fs but veil-EXEMPT (ruled fork): the
// ribbon is a flown sky structure, meant to be seen/ridden far beyond the
// band; veil_scale 0.0 keeps it whole while everything else condenses.
@fragment
fn ribbon_fs(in: EntityVarying) -> @location(0) vec4<f32> {
    return vec4(shade_lit(in.world_pos, normalize(in.normal), in.entity_color, 0.0), 1.0);
}

// --- Monolith vertex shader (imperfect cube, per-face color from seed)
@vertex
fn monolith_vs(@builtin(instance_index) inst: u32, in: MeshVertexInput) -> EntityVarying {
    let fe = render_floating.entities[inst];
    // Skip non-monolith geometry. THE RING (draw authority): draw only
    // inside the ring — center − extent ≤ ring (extent 2r covers the
    // aspect-stretched axes conservatively; overshoot is fully iced).
    let in_ring = distance(fe.pos.xz, vec2(config.lod_point_x, config.lod_point_z))
                  - fe.body_radius * 2.0 <= config.veil_ring;
    let r = select(0.0, fe.body_radius, fe.geometry_type == 1u && fe.is_active != 0u && in_ring);

    // Apply orientation quaternion (monoliths spin)
    let scaled = in.pos * vec3(r, r * fe.aspect_y, r * fe.aspect_z);
    let rotated = quat_rotate(fe.orientation, scaled);
    let world_pos = rotated + fe.pos;
    let world_normal = quat_rotate(fe.orientation, in.normal);

    // Per-face color: derive face index from dominant normal axis
    let abs_n = abs(in.normal);
    var face_idx = 0u;
    if (abs_n.y > abs_n.x && abs_n.y > abs_n.z) {
        face_idx = select(2u, 3u, in.normal.y > 0.0);
    } else if (abs_n.z > abs_n.x) {
        face_idx = select(4u, 5u, in.normal.z > 0.0);
    } else {
        face_idx = select(0u, 1u, in.normal.x > 0.0);
    }
    let face_hash = hash_property(fe.entity_seed, 500u + face_idx);
    let face_delta = (face_hash - 0.5) * 2.0 * fe.face_variance;
    let face_color = clamp(fe.color + vec3(face_delta, face_delta * 0.7, face_delta * 0.5), vec3(0.0), vec3(1.0));

    var out: EntityVarying;
    out.clip_pos = render_vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.normal = world_normal;
    out.entity_color = face_color;
    return out;
}


// §6.4 SHADOW PASS VERTEX SHADERS
// Depth-only rendering from the light's point of view.
struct ShadowVarying {
    @builtin(position) clip_pos: vec4<f32>,
}

@vertex
fn shadow_pawn_vs(@builtin(vertex_index) vid: u32,
                  @builtin(instance_index) inst: u32) -> ShadowVarying {
    let agent = render_agents[inst];
    let active_f = f32(agent.is_active);

    var local_pos: vec3<f32>;

    if (vid < PAWN_BODY_VERTICES) {
        let verts_per_band = PAWN_SEGMENTS * 6u;
        let band = vid / verts_per_band;
        let band_vid = vid % verts_per_band;
        let seg = band_vid / 6u;
        let tri_vert = band_vid % 6u;

        let seg_next = (seg + 1u) % PAWN_SEGMENTS;
        let angle0 = f32(seg) / f32(PAWN_SEGMENTS) * 2.0 * PI;
        let angle1 = f32(seg_next) / f32(PAWN_SEGMENTS) * 2.0 * PI;

        let t_lo = f32(band) / f32(PAWN_RINGS - 1u);
        let t_hi = f32(band + 1u) / f32(PAWN_RINGS - 1u);

        let r_lo = pawn_profile_radius(t_lo) * PAWN_RADIUS;
        let r_hi = pawn_profile_radius(t_hi) * PAWN_RADIUS;
        let y_lo = t_lo * PAWN_HEIGHT;
        let y_hi = t_hi * PAWN_HEIGHT;

        let p00 = vec3(cos(angle0) * r_lo, y_lo, sin(angle0) * r_lo);
        let p10 = vec3(cos(angle1) * r_lo, y_lo, sin(angle1) * r_lo);
        let p01 = vec3(cos(angle0) * r_hi, y_hi, sin(angle0) * r_hi);
        let p11 = vec3(cos(angle1) * r_hi, y_hi, sin(angle1) * r_hi);

        switch tri_vert {
            case 0u: { local_pos = p00; }
            case 1u: { local_pos = p10; }
            case 2u: { local_pos = p01; }
            case 3u: { local_pos = p01; }
            case 4u: { local_pos = p10; }
            case 5u: { local_pos = p11; }
            default: { local_pos = p00; }
        }
    } else {
        let cap_vid = vid - PAWN_BODY_VERTICES;
        let seg = cap_vid / 3u;
        let tri_vert = cap_vid % 3u;

        let seg_next = (seg + 1u) % PAWN_SEGMENTS;
        let angle0 = f32(seg) / f32(PAWN_SEGMENTS) * 2.0 * PI;
        let angle1 = f32(seg_next) / f32(PAWN_SEGMENTS) * 2.0 * PI;
        let r = pawn_profile_radius(0.0) * PAWN_RADIUS;

        switch tri_vert {
            case 0u: { local_pos = vec3(0.0, 0.0, 0.0); }
            case 1u: { local_pos = vec3(cos(angle1) * r, 0.0, sin(angle1) * r); }
            case 2u: { local_pos = vec3(cos(angle0) * r, 0.0, sin(angle0) * r); }
            default: { local_pos = vec3(0.0, 0.0, 0.0); }
        }
    }

    let pawn_q = vec4(agent.orient_x, agent.orient_y, agent.orient_z, agent.orient_w);
    let pawn_p = vec3(agent.pos_x, agent.pos_y, agent.pos_z);
    let world_pos = quat_rotate(pawn_q, local_pos * active_f) + pawn_p;

    // Lift pawn above terrain in shadow map. With perspective projection
    // from a ceiling light, 0.01 is invisible in the depth buffer at 19+
    // units distance. 0.3 gives enough depth separation to clear the
    // terrain without visibly displacing the shadow shape from overhead.
    var shadow_pos = world_pos;
    shadow_pos.y += 0.3;

    var out: ShadowVarying;
    out.clip_pos = render_vp.light_vp * vec4(shadow_pos, 1.0);
    return out;
}

// Shadow: Sphere (same as sphere_vs, light VP)
@vertex
fn shadow_sphere_vs(@builtin(instance_index) inst: u32, in: MeshVertexInput) -> ShadowVarying {
    let fe = render_floating.entities[inst];
    let r = select(0.0, fe.body_radius, fe.geometry_type == 0u && fe.is_active != 0u);
    let world_pos = in.pos * r + fe.pos;

    var out: ShadowVarying;
    out.clip_pos = render_vp.light_vp * vec4(world_pos, 1.0);
    return out;
}

// Shadow: Monolith (quaternion rotation, light VP)
@vertex
fn shadow_monolith_vs(@builtin(instance_index) inst: u32, in: MeshVertexInput) -> ShadowVarying {
    let fe = render_floating.entities[inst];
    let r = select(0.0, fe.body_radius, fe.geometry_type == 1u && fe.is_active != 0u);
    let scaled = in.pos * vec3(r, r * fe.aspect_y, r * fe.aspect_z);
    let rotated = quat_rotate(fe.orientation, scaled);
    let world_pos = rotated + fe.pos;

    var out: ShadowVarying;
    out.clip_pos = render_vp.light_vp * vec4(world_pos, 1.0);
    return out;
}

// --- Catenary Arch
struct ArchVertexInput {
    @location(0) pos: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) color: vec3<f32>,
    @location(3) arch_index: f32,   // slot index as float (avoids GPU denorm flush on bitcast<f32>(u32))
};

@vertex
fn arch_vs(in: ArchVertexInput) -> EntityVarying {
    let idx = u32(in.arch_index);
    let ground_y = textureLoad(entity_ground_atlas, vec2<i32>(i32(idx) + GROUND_ATLAS_ARCH, 0), 0).r;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += sample_live_card(world_pos.xz).x;

    var out: EntityVarying;
    out.clip_pos = render_vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.normal = in.normal;
    out.entity_color = in.color;
    return out;
}

@vertex
fn shadow_arch_vs(in: ArchVertexInput) -> ShadowVarying {
    let idx = u32(in.arch_index);
    let ground_y = textureLoad(entity_ground_atlas, vec2<i32>(i32(idx) + GROUND_ATLAS_ARCH, 0), 0).r;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += sample_live_card(world_pos.xz).x;

    var out: ShadowVarying;
    out.clip_pos = render_vp.light_vp * vec4(world_pos, 1.0);
    return out;
}

// --- Generative Columns
@vertex
fn column_vs(in: ArchVertexInput) -> EntityVarying {
    let idx = u32(in.arch_index);
    let ground_y = textureLoad(entity_ground_atlas, vec2<i32>(i32(idx) + GROUND_ATLAS_COLUMN, 0), 0).r;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += sample_live_card(world_pos.xz).x;

    var out: EntityVarying;
    out.clip_pos = render_vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.normal = in.normal;
    out.entity_color = in.color;
    return out;
}

@vertex
fn shadow_column_vs(in: ArchVertexInput) -> ShadowVarying {
    let idx = u32(in.arch_index);
    let ground_y = textureLoad(entity_ground_atlas, vec2<i32>(i32(idx) + GROUND_ATLAS_COLUMN, 0), 0).r;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += sample_live_card(world_pos.xz).x;

    var out: ShadowVarying;
    out.clip_pos = render_vp.light_vp * vec4(world_pos, 1.0);
    return out;
}

// --- Generative Pyramids: pyramid_vs + shadow_pyramid_vs CUT (orphan
//     sweep) — the pyramid mesh was never drawn (draw_pyramid /
//     draw_shadow_pyramid were caller-free). Both read the
//     ground-atlas slot range (48..55); the write path followed as residue
//     (the pyramid-ground husk) — the range stays a documented hole in the
//     atlas table.

// --- Indoor Shell (ceiling + walls)
// Pre-baked world-space geometry, no per-instance transforms.
struct ShellVertexInput {
    @location(0) pos: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) color: vec3<f32>,
};

@vertex
fn shell_vs(in: ShellVertexInput) -> EntityVarying {
    var out: EntityVarying;
    out.clip_pos = render_vp.m * vec4(in.pos, 1.0);
    out.world_pos = in.pos;
    out.normal = in.normal;
    out.entity_color = in.color;
    return out;
}

@vertex
fn shadow_shell_vs(in: ShellVertexInput) -> ShadowVarying {
    var out: ShadowVarying;
    out.clip_pos = render_vp.light_vp * vec4(in.pos, 1.0);
    return out;
}

// §6.5 SKY RIBBON ENTITY
// A continuous square-section tube whose body is the TRAIL of a harmonic-
// oscillator head, sampled at progressively older head positions along its
// length:
//
// TRAIL-FRAME shape: each ring at parameter t shows the head's state at
//    age = t × total_length / propagation_speed seconds ago:
//      sin(freq × (time − t × total_length / propagation_speed))
//    Visible cycles emerge from freq × travel_time (preserving the authored
//    per-tier cycle counts); crests propagate head → tail at the single
//    propagation_speed (uniform across all three axes).
// The ribbon's centerline at parameter t, before the wave is layered on:
// lerp over head_poses, the CPU-rebuilt propagation body. A parked head has
// a constant past, so this reads as the straight spawn arc.
fn ribbon_centerline_at(t: f32, ribbon: RibbonState) -> vec3<f32> {
    let span = max(ribbon.cube_count, 2u) - 1u;
    let fidx = clamp(t, 0.0, 1.0) * f32(span);
    let i0 = u32(floor(fidx));
    let i1 = min(i0 + 1u, span);
    let frac = fidx - f32(i0);
    return mix(head_poses[i0].xyz, head_poses[i1].xyz, frac);
}

// The head's transverse displacement (the choreography) at echo time
// `phase_age`. Two components -- lateral (sway) and vertical (bob). This is
// the single displacement the body echoes along the ruler. The echo is
// ANALYTIC: the body re-evaluates the head's timetable at a delayed time —
// honest only while the script is a pure function of time.
// [SEAM:ribbon-displacement] To let music drive displacement at the head,
// record lat/vert into the propagation history beside heading and Y and read
// the delayed samples here. Coupling amp/freq parameters directly would move
// the whole body at once — a teleport, not a gesture.
fn ribbon_displacement_at(phase_age: f32, ribbon: RibbonState) -> vec2<f32> {
    let lateral  = sin(ribbon.lateral_freq  * phase_age) * ribbon.lateral_amp;
    let vertical = sin(ribbon.vertical_freq * phase_age) * ribbon.vertical_amp;
    return vec2(lateral, vertical);
}

// Analytic wave slopes — the displacement's derivative twin, per unit of
// phase_age. Used by the ring motor to aim the frame along the TRUE
// instantaneous motion (heading + wave) and to bank into the swing.
fn ribbon_wave_slopes(phase_age: f32, ribbon: RibbonState) -> vec2<f32> {
    let d_lat  = cos(ribbon.lateral_freq  * phase_age)
               * ribbon.lateral_amp  * ribbon.lateral_freq;
    let d_vert = cos(ribbon.vertical_freq * phase_age)
               * ribbon.vertical_amp * ribbon.vertical_freq;
    return vec2<f32>(d_lat, d_vert);
}

// Trail-frame phase for spine parameter t: how long ago (seconds) this
// ring's head-state was emitted. THE one expression — shared by the spine
// echo and the ring motor's frame law (BNK-1) so the two can never drift.
fn ribbon_phase_age(t: f32, ribbon: RibbonState) -> f32 {
    let total_length = f32(ribbon.cube_count) * ribbon.cube_size;
    return ribbon.time - t * total_length / max(ribbon.propagation_speed, 1e-6);
}

fn ribbon_spine_at(t: f32, ribbon: RibbonState) -> vec3<f32> {
    // Trail-frame phase: shared across all axes so crests stay synchronized.
    let phase_age = ribbon_phase_age(t, ribbon);

    // The body is the head's displacement echoed along the ruler. Place the
    // sway in the ring's OWN frame — the CPU-authored yaw channel
    // (head_poses[i].w; unwrapped, so plain lerp between rings is safe). The
    // SAME channel orients the ring motor and the pawn mount, so the three
    // never diverge: lateral on right = (-sin yaw, 0, cos yaw), vertical on
    // world-up.
    let d = ribbon_displacement_at(phase_age, ribbon);
    let center = ribbon_centerline_at(t, ribbon);
    let span_u = max(ribbon.cube_count, 2u) - 1u;
    let fidx = clamp(t, 0.0, 1.0) * f32(span_u);
    let i0 = u32(floor(fidx));
    let i1 = min(i0 + 1u, span_u);
    let yaw = mix(head_poses[i0].w, head_poses[i1].w, fidx - f32(i0));
    let right = vec3(-sin(yaw), 0.0, cos(yaw));
    return center + d.x * right + d.y * vec3(0.0, 1.0, 0.0);
}

// ── The frame law (BNK-1) ── how each ring's FRAME answers the wave.
// ALIGN: 0 = frames ignore the wave (the old yaw-only law); 1 = the nose
//   and every ring aim along the true instantaneous motion — heading
//   deflected by the wave's slope (the head faces where it swims).
// BANK: roll into the lateral swing — max lean crossing center, level at
//   the extremes, clamped. Slope scales with amplitude, so the sustain
//   swell deepens the carve and the lean with no extra pipe.
// Identity at 0/0. Hot-reloadable; tune by save.
// MIRRORED in bodies/ribbon.hpp (MOUNT_*) — keep in lockstep; the rider is the drift test.
const RIBBON_TANGENT_ALIGN: f32 = 1.0;
const RIBBON_BANK_GAIN: f32 = 0.9;
const RIBBON_BANK_MAX: f32 = 0.6;   // radians, clamp

// Build a PGA motor that places and orients one cross-section ring.
// Composes: orient * translate — rotate the local frame, then place it.
fn ribbon_ring_motor(ring_idx: u32, ribbon: RibbonState) -> Motor {
    let t = f32(ring_idx) / f32(max(ribbon.cube_count - 1u, 1u));
    let center = ribbon_spine_at(t, ribbon);

    // The ring yaw is CPU-AUTHORED — head_poses[i].w carries the unwrapped
    // per-ring tailward heading (ring 0 = the live flight heading). Adjacent
    // values are continuous by construction — the CPU unwraps while walking
    // the rings in order — so per-ring flips are impossible and no finite
    // difference is needed. One channel feeds ring orient, wave frame, and
    // pawn mount. Pure yaw about world-up: the path is planar; the wave
    // rides as displacement, not as frame pitch.
    // Negated: rotor+sw_mp map +X to (cos θ, −sin θ) (hand-verified), while
    // the channel — like the whole analytic codebase — speaks dir(θ) =
    // (cos θ, +sin θ). rotor(Y, −w) lands the tube axis on tailward exactly,
    // and the ring's lateral axis on (−sin w, 0, cos w) — identical to the
    // spine wave's explicit right and the mount's right. One convention,
    // three consumers, coherent.
    // The saddle wears the full frame (CPU mount angles composed in
    // the sky branch), no longer gimbal-level.

    // Phase age for THIS ring: the exact expression ribbon_spine_at uses,
    // shared via ribbon_phase_age (refactored to a helper, not mirrored,
    // so the frame answers the very wave the spine draws — no drift).
    let phase_age = ribbon_phase_age(t, ribbon);
    let slopes = ribbon_wave_slopes(phase_age, ribbon);
    let p = max(ribbon.propagation_speed, 1e-3);

    // Deflections of the frame toward the true tangent — NEGATED: the
    // tube axis runs TAILWARD (dir(w)), and the tailward tangent is
    // exactly dir(w − atan(slopes.x/p)), pitched −atan(slopes.y/p).
    // With a plus the nose crabs OUTWARD of the swing and dives on the
    // rise (the gate-4 failure): caught on screen (BNK-1 sweep),
    // confirmed by derivation — the apparent ring velocity equals
    // −p × the tailward tangent, so nose-along-motion and
    // axis-along-tangent impose the same sign.
    let yaw_off   = -RIBBON_TANGENT_ALIGN * atan(slopes.x / p);
    let pitch_off = -RIBBON_TANGENT_ALIGN * atan(slopes.y / p);
    let roll      = clamp(RIBBON_BANK_GAIN * (slopes.x / p),
                          -RIBBON_BANK_MAX, RIBBON_BANK_MAX);

    // Compose: base yaw (the CPU-authored heading, negated per the
    // convention note above) → pitch about the ring's LATERAL axis →
    // roll about the tube AXIS. Local axes in ring space before the
    // base yaw: tube axis = +X, lateral = +Z, up = +Y (per tube_corner /
    // tube_face_normal). Apply the local rotors FIRST, then the base
    // yaw, then translate — gp_mm order per the existing comment (first
    // argument applies first). Sign convention: RESOLVED — the
    // tangent-align terms enter negated (see above); the bank's sign
    // is aesthetic and stands as authored.
    let base_yaw = rotor(vec3(0.0, 1.0, 0.0), -(head_poses[ring_idx].w) - yaw_off);
    let r_pitch  = rotor(vec3(0.0, 0.0, 1.0), pitch_off);
    let r_roll   = rotor(vec3(1.0, 0.0, 0.0), roll);
    let orient   = gp_mm(gp_mm(r_roll, r_pitch), base_yaw);

    let trans = Motor(vec4(1.0, 0.0, 0.0, 0.0), vec4(-0.5 * center, 0.0));
    return gp_mm(orient, trans);
}

// --- Square Tube Geometry
const TUBE_VERTS_PER_SEGMENT: u32 = 24u;  // 4 faces × 6 verts
const TUBE_CAP_VERTS: u32 = 12u;          // 2 caps × 6 verts

fn tube_corner(idx: u32, half: f32) -> vec3<f32> {
    switch idx {
        case 0u: { return vec3(0.0,  half,  half); }
        case 1u: { return vec3(0.0,  half, -half); }
        case 2u: { return vec3(0.0, -half, -half); }
        default: { return vec3(0.0, -half,  half); }
    }
}

fn tube_face_normal(face: u32) -> vec3<f32> {
    switch face {
        case 0u: { return vec3(0.0,  1.0,  0.0); }  // top
        case 1u: { return vec3(0.0,  0.0, -1.0); }  // left
        case 2u: { return vec3(0.0, -1.0,  0.0); }  // bottom
        default: { return vec3(0.0,  0.0,  1.0); }  // right
    }
}

fn tube_face_corners(face: u32) -> vec2<u32> {
    switch face {
        case 0u: { return vec2(0u, 1u); }  // top:    TR -> TL
        case 1u: { return vec2(1u, 2u); }  // left:   TL -> BL
        case 2u: { return vec2(2u, 3u); }  // bottom: BL -> BR
        default: { return vec2(3u, 0u); }  // right:  BR -> TR
    }
}

// --- Compute ribbon ring transforms (flying ribbons; no terrain follow)
@compute @workgroup_size(64)
fn compute_ribbon_rings(@builtin(global_invocation_id) gid: vec3<u32>) {
    let ring_idx = gid.x;
    let ribbon = ribbon_state;

    // Early-out for unused rings
    if (ring_idx >= ribbon.cube_count || ribbon.is_visible == 0u || ribbon.cube_count < 2u) {
        // Zero out unused slots so VS reads clean data
        if (ring_idx < 400u) {
            ring_xforms[ring_idx].motor_p0 = vec4(1.0, 0.0, 0.0, 0.0);
            ring_xforms[ring_idx].motor_p1 = vec4(0.0);
            ring_xforms[ring_idx].center = vec3(0.0);
            ring_xforms[ring_idx].terrain_y = 0.0;
        }
        return;
    }

    // Compute PGA motor (translate + orient along spine).
    let motor = ribbon_ring_motor(ring_idx, ribbon);
    let center = sw_mp(motor, vec3(0.0));

    let terrain_y: f32 = 0.0;  // Only flying ribbons now; no terrain-following needed.

    ring_xforms[ring_idx].motor_p0 = motor.p0;
    ring_xforms[ring_idx].motor_p1 = motor.p1;
    ring_xforms[ring_idx].center = center;
    ring_xforms[ring_idx].terrain_y = terrain_y;
}

// Chroma constants for the checker skin's CB-1e reconstruction: DIR is
// the fallback direction for near-gray cells (pure red minus its gray
// component, normalized); FLOOR is the chroma magnitude every cell is
// guaranteed at FULL spread (pi), so both parities reach the same
// colorfulness at the same hue_var. Hot-reloadable; tune live.
const CHECKER_CHROMA_DIR: vec3<f32> = vec3<f32>(0.8165, -0.4082, -0.4082);
// Chroma at FULL spread (pi). Hot-reloadable; the punch dial.
const CHECKER_CHROMA_FLOOR: f32 = 0.22;

// Branchless hue rotation about the RGB gray axis (Rodrigues form).
// Identity at a = 0 by construction — the CB-1b hue-spread dial rests
// there for SMOOTH/TINTED and low-draw CONTRAST ribbons.
fn hue_rotate(c: vec3<f32>, a: f32) -> vec3<f32> {
    let k = vec3<f32>(0.577350269);
    let ca = cos(a); let sa = sin(a);
    return c * ca + cross(k, c) * sa + k * dot(k, c) * (1.0 - ca);
}

// --- Ribbon Vertex Shader: Square Tube
@vertex
fn ribbon_vs(@builtin(vertex_index) vid: u32) -> EntityVarying {
    let ribbon = render_ribbon;
    let ring_count = ribbon.cube_count;   // number of cross-section rings
    let half = ribbon.cube_size * 0.5;    // half cross-section size

    let body_verts = (ring_count - 1u) * TUBE_VERTS_PER_SEGMENT;
    let total_verts = body_verts + TUBE_CAP_VERTS;

    if (vid >= total_verts || ribbon.is_visible == 0u || ring_count < 2u) {
        var out: EntityVarying;
        out.clip_pos = vec4(0.0, 0.0, 0.0, 1.0);
        out.world_pos = vec3(0.0);
        out.normal = vec3(0.0, 1.0, 0.0);
        out.entity_color = vec3(0.0);
        return out;
    }

    var local_pos: vec3<f32>;
    var local_normal: vec3<f32>;
    var ring_idx: u32;

    if (vid < body_verts) {
        // --- Body: extruded tube segments
        let seg = vid / TUBE_VERTS_PER_SEGMENT;
        let seg_vid = vid % TUBE_VERTS_PER_SEGMENT;
        let face = seg_vid / 6u;
        let fv = seg_vid % 6u;

        let fc = tube_face_corners(face);
        local_normal = tube_face_normal(face);

        // Quad: tri0 = (lo_a, lo_b, hi_a), tri1 = (hi_a, lo_b, hi_b)
        var is_hi: bool;
        var corner: u32;
        switch fv {
            case 0u: { is_hi = false; corner = fc.x; }
            case 1u: { is_hi = false; corner = fc.y; }
            case 2u: { is_hi = true;  corner = fc.x; }
            case 3u: { is_hi = true;  corner = fc.x; }
            case 4u: { is_hi = false; corner = fc.y; }
            default: { is_hi = true;  corner = fc.y; }
        }

        ring_idx = select(seg, seg + 1u, is_hi);
        local_pos = tube_corner(corner, half);
    } else {
        // --- End caps
        let cap_vid = vid - body_verts;
        let is_front = cap_vid >= 6u;
        let cv = cap_vid % 6u;

        ring_idx = select(0u, ring_count - 1u, is_front);
        local_normal = select(vec3(-1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), is_front);

        // Back cap: 0,2,1 and 0,3,2 (outward toward -X)
        // Front cap: 0,1,2 and 0,2,3 (outward toward +X)
        var corner: u32;
        if (is_front) {
            switch cv {
                case 0u: { corner = 0u; }
                case 1u: { corner = 1u; }
                case 2u: { corner = 2u; }
                case 3u: { corner = 0u; }
                case 4u: { corner = 2u; }
                default: { corner = 3u; }
            }
        } else {
            switch cv {
                case 0u: { corner = 0u; }
                case 1u: { corner = 2u; }
                case 2u: { corner = 1u; }
                case 3u: { corner = 0u; }
                case 4u: { corner = 3u; }
                default: { corner = 2u; }
            }
        }
        local_pos = tube_corner(corner, half);
    }

    // Transform by ring's pre-computed PGA motor (with inline fallback)
    // If the compute pass hasn't run yet, motor_p0 is all zeros — compute inline.
    let xform = render_ring_xforms[ring_idx];
    let xform_valid = any(xform.motor_p0 != vec4(0.0));

    var motor: Motor;
    if (xform_valid) {
        motor = Motor(xform.motor_p0, xform.motor_p1);
    } else {
        // Pre-compute frame (rare): identity motor. The inline spine recompute
        // was retired so render/shadow don't reference head_poses; compute writes
        // the rings before render reads them, so this branch doesn't fire in
        // normal operation.
        motor = Motor(vec4(1.0, 0.0, 0.0, 0.0), vec4(0.0));
    }
    let orient = Motor(motor.p0, vec4(0.0));

    var world_pos = sw_mp(motor, local_pos);
    let world_normal = sw_mp(orient, local_normal);

    var out: EntityVarying;
    out.clip_pos = render_vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.normal = world_normal;

    // CONTRAST checker skin (CB-1): per-cell parity between the two
    // authored medians plus a seeded per-cell scatter. A cell is one
    // segment × face quad — cell_seg/cell_face restate the SAME vid
    // decomposition the body path uses, so all six vertices of a quad
    // agree and the cells sit flat. Caps are outside the cell grid by
    // declaration and keep median A. Branchless: SMOOTH/TINTED fall
    // through the select to ribbon.color.
    let cell_seg = vid / TUBE_VERTS_PER_SEGMENT;
    let cell_face = (vid % TUBE_VERTS_PER_SEGMENT) / 6u;
    let cell_parity = f32((cell_seg + cell_face) & 1u);
    let cell_key = (cell_seg * 4u + cell_face) ^ ribbon.seed;
    // CB-1b hue-spread: per-cell hue angle from a salted hash draw,
    // decorrelated from the value scatter; identity at spread 0.
    let hue_h = hash_property(cell_key ^ 0x9E3779B9u, 0u) - 0.5;
    let hue_a = hue_h * 2.0 * ribbon.hue_spread;
    var base = mix(ribbon.color, ribbon.color_b, cell_parity);

    // Chroma (CB-1e): decompose about the gray axis, rotate the DIRECTION
    // by the cell's angle, set the MAGNITUDE to max(existing, spread-
    // scaled floor). Both parities reach the same colorfulness at the
    // same hue_var; a near-gray median gains chroma instead of rotating
    // nothing. spread = 0 ⇒ angle 0 and magnitude = existing ⇒ EXACT
    // identity.
    let g  = vec3<f32>(0.577350269) * dot(vec3<f32>(0.577350269), base);
    let ch = base - g;
    let cl = length(ch);
    let cdir = select(ch / max(cl, 1e-4), CHECKER_CHROMA_DIR, cl < 1e-3);
    let cmag = max(cl, (ribbon.hue_spread * 0.318309886) * CHECKER_CHROMA_FLOOR);
    base = g + hue_rotate(cdir, hue_a) * cmag;

    // Value (CB-1e): travel a FRACTION of the available headroom toward
    // black or white. Cannot clip; reads with matched perceptual weight
    // on dark and light squares alike. One scalar per cell: lightness
    // texture only — chroma stays the hue axis's business.
    let vj = (hash_property(cell_key ^ 0x85EBCA6Bu, 0u) - 0.5) * 2.0;
    let pole = select(vec3<f32>(0.0), vec3<f32>(1.0), vj > 0.0);
    base = mix(base, pole, abs(vj) * ribbon.checker_scatter);

    let checker = clamp(base, vec3(0.0), vec3(1.0));
    out.entity_color = select(ribbon.color, checker,
        ribbon.color_mode == 2u && vid < body_verts);
    return out;
}

// --- Shadow: Sky Ribbon

@vertex
fn shadow_ribbon_vs(@builtin(vertex_index) vid: u32) -> ShadowVarying {
    let ribbon = render_ribbon;
    let ring_count = ribbon.cube_count;
    let half = ribbon.cube_size * 0.5;

    let body_verts = (ring_count - 1u) * TUBE_VERTS_PER_SEGMENT;
    let total_verts = body_verts + TUBE_CAP_VERTS;

    if (vid >= total_verts || ribbon.is_visible == 0u || ring_count < 2u) {
        var out: ShadowVarying;
        out.clip_pos = vec4(0.0, 0.0, 0.0, 1.0);
        return out;
    }

    var local_pos: vec3<f32>;
    var ring_idx: u32;

    if (vid < body_verts) {
        let seg = vid / TUBE_VERTS_PER_SEGMENT;
        let seg_vid = vid % TUBE_VERTS_PER_SEGMENT;
        let face = seg_vid / 6u;
        let fv = seg_vid % 6u;

        let fc = tube_face_corners(face);
        var is_hi: bool;
        var corner: u32;
        switch fv {
            case 0u: { is_hi = false; corner = fc.x; }
            case 1u: { is_hi = false; corner = fc.y; }
            case 2u: { is_hi = true;  corner = fc.x; }
            case 3u: { is_hi = true;  corner = fc.x; }
            case 4u: { is_hi = false; corner = fc.y; }
            default: { is_hi = true;  corner = fc.y; }
        }

        ring_idx = select(seg, seg + 1u, is_hi);
        local_pos = tube_corner(corner, half);
    } else {
        let cap_vid = vid - body_verts;
        let is_front = cap_vid >= 6u;
        let cv = cap_vid % 6u;

        ring_idx = select(0u, ring_count - 1u, is_front);

        var corner: u32;
        if (is_front) {
            switch cv {
                case 0u: { corner = 0u; }
                case 1u: { corner = 1u; }
                case 2u: { corner = 2u; }
                case 3u: { corner = 0u; }
                case 4u: { corner = 2u; }
                default: { corner = 3u; }
            }
        } else {
            switch cv {
                case 0u: { corner = 0u; }
                case 1u: { corner = 2u; }
                case 2u: { corner = 1u; }
                case 3u: { corner = 0u; }
                case 4u: { corner = 3u; }
                default: { corner = 2u; }
            }
        }
        local_pos = tube_corner(corner, half);
    }

    let xform = render_ring_xforms[ring_idx];
    let xform_valid = any(xform.motor_p0 != vec4(0.0));

    var motor: Motor;
    if (xform_valid) {
        motor = Motor(xform.motor_p0, xform.motor_p1);
    } else {
        motor = Motor(vec4(1.0, 0.0, 0.0, 0.0), vec4(0.0));  // see ribbon_vs note
    }
    var world_pos = sw_mp(motor, local_pos);

    var out: ShadowVarying;
    out.clip_pos = render_vp.light_vp * vec4(world_pos, 1.0);
    return out;
}

// §6.6 FADE OVERLAY
// Fullscreen triangle for transition fade. Drawn last with alpha blending.
// Reads fade_alpha and fade_color from DesignConfig.

struct FadeVarying {
    @builtin(position) pos: vec4<f32>,
}

@vertex
fn fade_overlay_vs(@builtin(vertex_index) vid: u32) -> FadeVarying {
    // Fullscreen triangle from vertex ID (covers clip space)
    let x = f32(i32(vid & 1u)) * 4.0 - 1.0;
    let y = f32(i32(vid >> 1u)) * 4.0 - 1.0;
    var out: FadeVarying;
    out.pos = vec4(x, y, 0.0, 1.0);
    return out;
}

@fragment
fn fade_overlay_fs(in: FadeVarying) -> @location(0) vec4<f32> {
    return vec4(config.fade_color, config.fade_alpha);
}

// §7.0 GLOBAL BINDINGS

// --- Compute bindings (Group 0: buffers, 20-slot system ranges)
@group(0) @binding(0)   var<uniform>             signal: FrameSignal;
@group(0) @binding(1)   var<uniform>             config: DesignConfig;
@group(0) @binding(2)   var<storage, read_write> vp_data: VPMatrix;
// @binding(20) terrain_state REMOVED (dead terrain buffer)

// Agent system — unified entity buffer. Slot 0 is the player's body;
// slots 1..31 are mood-authored agents. The player's relationship
// to this array is config.possessed_slot. Array size matches
// Dim::MAX_AGENTS (32) in state.hpp — keep in sync.
@group(0) @binding(60)  var<storage, read_write> agent_state: array<AgentState, 32>;

// Portal proximity array (uploaded by CPU, checked in behavior_player_controlled)
struct PortalEntry {
    x: f32,
    z: f32,
    facing_cos: f32,
    facing_sin: f32,
    inv_span_sq: f32,
    inv_depth_sq: f32,
    arch_index: u32,
    _pad: u32,
}
struct PortalArray {
    count: u32,
    _pad0: u32,
    _pad1: u32,
    _pad2: u32,
    portals: array<PortalEntry, 32>,
}
@group(0) @binding(62)  var<uniform> portal_array: PortalArray;

@group(0) @binding(80)  var<storage, read_write> camera_state: CameraState;
@group(0) @binding(100) var<storage, read_write> floating_entities: FloatingEntityArray;
@group(0) @binding(120) var<uniform>             ribbon_state: RibbonState;

// Possessed-agent helpers (compute stage). Every kernel that used to
// read pawn_state.pos now goes through these. Extracting here keeps
// the indexing + scalar→vec conversion at one site and the call sites
// read as "the pawn's pos" without repeating the slot lookup.
fn compute_pawn_pos() -> vec3<f32> {
    let a = agent_state[config.possessed_slot];
    return vec3(a.pos_x, a.pos_y, a.pos_z);
}
fn compute_pawn_vel_xz() -> vec2<f32> {
    let a = agent_state[config.possessed_slot];
    return vec2(a.vel_x, a.vel_z);
}
fn compute_pawn_heading() -> f32 {
    return agent_state[config.possessed_slot].heading;
}

// THE POINT's position (compute stage) — host-sourced, per the point
// contract (contracts/point.hpp): the possessed body
// when the pawn hosts, the camera eye when the camera hosts.
// VIEWPOINT-serving reads (the shadow-VP here; the CPU streaming
// center via the point readback) go through this; ENTITY-EMANATING
// fields (aura, forcefield) keep reading the pawn directly.
fn point_pos() -> vec3<f32> {
    if (point_camera_hosted()) { return camera_state.pos; }
    return compute_pawn_pos();
}

// --- [BINDINGS:compute] Group 0 — Render entity mirrors (read-only, +200 offset)
@group(0) @binding(200) var<storage, read> render_signal: FrameSignal;
@group(0) @binding(201) var<storage, read> render_vp: VPMatrix;
// @binding(220) render_terrain REMOVED (dead terrain buffer)
@group(0) @binding(260) var<storage, read> render_agents: array<AgentState, 32>;
@group(0) @binding(280) var<storage, read> render_camera: CameraState;
@group(0) @binding(300) var<uniform> render_floating: FloatingEntityArray;

// Possessed-agent helpers (render stage). VS/FS consumers that used
// to read render_pawn.pos etc. go through these.
fn render_pawn_pos() -> vec3<f32> {
    let a = render_agents[config.possessed_slot];
    return vec3(a.pos_x, a.pos_y, a.pos_z);
}
// THE POINT, render-stage (the veil's prerequisite — ruled). The render-
// side twin of the compute-only point_pos(): camera-hosted → the live
// camera; pawn-hosted → the possessed body. The veil anchors HERE — on
// the point, never the eye (in pawn-host 3rd person the eye orbits off
// the body; the eye-fog in shade_lit keeps the eye, the VEIL keeps the
// point).
fn render_point_pos() -> vec3<f32> {
    if (point_camera_hosted()) { return render_camera.pos; }
    return render_pawn_pos();
}
fn render_pawn_vel_xz() -> vec2<f32> {
    let a = render_agents[config.possessed_slot];
    return vec2(a.vel_x, a.vel_z);
}
fn render_pawn_orientation() -> vec4<f32> {
    let a = render_agents[config.possessed_slot];
    return vec4(a.orient_x, a.orient_y, a.orient_z, a.orient_w);
}

// --- Ribbon (Group 0: render, binding 360)
@group(0) @binding(360) var<uniform> render_ribbon: RibbonState;
@group(0) @binding(361) var<storage, read> render_ring_xforms: array<RibbonRingTransform, 400>;
// Entity ground atlas — VS reads ground_y via textureLoad (r32float, 256×1)
@group(0) @binding(390) var entity_ground_atlas: texture_2d<f32>;

// Atlas slot offsets (must match Dim:: constants in state.hpp)
const GROUND_ATLAS_ARCH: i32     = 0;
const GROUND_ATLAS_COLUMN: i32   = 16;
// slots 48..55: DOCUMENTED HOLE — the retired pyramid range (readers cut at
// cut; the write path followed as residue). Do NOT re-pack; these offsets are
// hand-mirrored with state.hpp Dim::GROUND_ATLAS_*.
const GROUND_ATLAS_PALM: i32     = 56;
const GROUND_ATLAS_CACTUS: i32   = 80;
const GROUND_ATLAS_BLADE: i32    = 100;

// --- Ribbon compute (Group 0: binding 121, separate pipeline layout)
// Written by compute_ribbon_rings, read by ribbon VS via render_ring_xforms.
@group(0) @binding(121) var<storage, read_write> ring_xforms: array<RibbonRingTransform, 400>;

// Ribbon body — rebuilt each frame from the propagation history (heading +
// Y replayed at P, XZ integrated tailward). .xyz = ring position, .w = the
// ring's unwrapped yaw; read by ribbon_centerline_at / ribbon_spine_at /
// ribbon_ring_motor.
@group(0) @binding(122) var<storage, read> head_poses: array<vec4<f32>, 400>;

// --- Light system (Group 0: render, bindings 320-339)
@group(0) @binding(320) var<storage, read> render_light: DirectionalLight;
@group(0) @binding(321) var<storage, read> render_point_lights: PointLightArray;
@group(0) @binding(322) var<storage, read> render_spot_lights: SpotLightArray;

// --- Render textures (Group 1: bindings 22-23, 25-26)
// (bindings 20, 21, 24 reserved — formerly legacy stub textures)
@group(1) @binding(22) var bilinear_sampler: sampler;
@group(1) @binding(23) var nearest_sampler: sampler;
@group(1) @binding(25) var shadow_map: texture_depth_2d;           // sun shadows (outdoor) / spot atlas lights 0-1 (indoor)
@group(1) @binding(26) var shadow_sampler: sampler_comparison;
@group(1) @binding(27) var spot_shadow_map: texture_depth_2d;     // spot atlas lights 2-3 (indoor)

// §7.0a PATCH GENERATION BINDINGS

// --- Shared mesh vertex struct (used by zone extrusion mesh gen)
// Bindings 40-45 reserved (currently unused).
struct CellMeshVertex {
    px: f32, py: f32, pz: f32,
    nx: f32, ny: f32, nz: f32,
    ux: f32, uy: f32,
    cr: f32, cg: f32, cb: f32,
}

// --- Terrain index generation (Group 0: binding 22, one-shot)
// Separate pipeline layout with a single storage buffer.
// Used once at initialization, never again.
@group(0) @binding(22) var<storage, read_write> terrain_mesh_indices: array<u32>;

// --- Patch heightfield generation (Group 0: bindings 23-24)
// Separate pipeline layout. Dispatched per-patch when a new patch enters
// the active set. Writes to one layer of the patch heightfield array.
@group(0) @binding(23) var<uniform> patch_params: PatchParams;
@group(0) @binding(24) var patch_heightfield_array_write: texture_storage_2d_array<rgba16float, write>;
@group(0) @binding(25) var<uniform> tile_grid: TileGrid;
@group(0) @binding(27) var patch_cell_color_array_write: texture_storage_2d_array<rgba8unorm, write>;
// (binding 29, cell_fields_write, RETIRED — Commit C, the LUT
//  retirement. Number reserved; do not reuse.)
@group(0) @binding(28) var<storage, read_write> patch_height_scratch: array<f32>;

// --- Patch rendering (Group 0: binding 340, 391; Group 1: bindings 28-29)
@group(0) @binding(340) var<storage, read> patch_instances: array<PatchInstance>;
@group(0) @binding(391) var<storage, read> visible_patch_indices: array<u32>;
@group(1) @binding(28) var patch_heightfield_array_read: texture_2d_array<f32>;
@group(1) @binding(29) var patch_cell_color_array_read: texture_2d_array<f32>;
// (binding 30, cell_fields_read, RETIRED — Commit C. Number reserved.)

// §7.0b GOL ZONE DEFINITIONS

// --- GoL zone system (Group 0: bindings 160-162, dedicated layout)
struct GoLZoneConfig {
    origin: vec2<f32>,
    extent: f32,
    grid_size: u32,
    tick_period: f32,
    spring_stiffness: f32,
    alive_height: f32,
    transition_fraction: f32,
    color_mode: u32,
    target_r: f32,
    target_g: f32,
    target_b: f32,
    algorithm: u32,
    wander_radius: f32,
    phase_randomness: f32,
    boundary_mode: u32,
    tempo_randomness: f32,       // per-cell frequency scatter [0,1]
    spring_variance: f32,        // per-cell spring speed scatter [0,1]
    _zpad0: f32,
    _zpad1: f32,
}

// --- GoL Zone Visual Parameters
const GOL_COLOR_NEUTRAL:  u32 = 0u;  // no color change (height-only extrusion)
const GOL_COLOR_LENS:     u32 = 1u;  // shift ground toward per-zone target color
const GOL_COLOR_BLACKISH: u32 = 2u;  // darken toward near-black

// Algorithm constants (must match CPU AlgorithmType::)
const GOL_ALGORITHM_CONWAY: u32 = 0u;
const GOL_ALGORITHM_PULSE:  u32 = 1u;

// Boundary mode constants (must match CPU BoundaryMode::)
const GOL_BOUNDARY_REFLECT: u32 = 0u;
const GOL_BOUNDARY_WRAP:    u32 = 1u;

// --- Distance fade (prevents aliasing flicker at distance)
const GOL_FADE_NEAR: f32 = 150.0;    // full effect inside this range
const GOL_FADE_FAR: f32  = 300.0;    // zero effect beyond this range

// --- Tint strength (spring-driven engagement at visual=1.0)
const GOL_TINT_STRENGTH: f32 = 0.70;

// --- Lens mode parameters
const GOL_LENS_BLEND_BASE: f32     = 0.30;  // gentle shift toward target (preserves 70% of base)
const GOL_LENS_VARIATION_RANGE: f32 = 0.15; // per-cell variation (±half)

// --- Blackish mode parameters
const GOL_BLACK_DARK_BASE: f32      = 0.35;  // base darkness (0=black, 1=full terrain)
const GOL_BLACK_DARK_RANGE: f32     = 0.20;  // per-cell variation on darkness
const GOL_BLACK_R_SHIFT_RANGE: f32  = 0.10;  // red warmth/coolness (±half)
const GOL_BLACK_G_SHIFT_RANGE: f32  = 0.06;  // green variation (±half)

// --- Neutral mode (extrusion only — no terrain color change)
const GOL_NEUTRAL_DARKEN: f32 = 0.75; // extrusion blocks: terrain × this

// --- Per-cell hash for consistent randomization
fn gol_cell_hash(cx: u32, cy: u32) -> u32 {
    return cx * 374761393u + cy * 668265263u;
}

fn gol_cell_variation(h: u32) -> f32 {
    return f32(h & 0xFFFFu) / 65535.0;
}

// --- Boundary mode functions for Pulse algorithm
fn reflect01(x: f32) -> f32 {
    // Triangle wave on [0,1]: bounces off boundaries, preserves energy
    return 1.0 - abs(fract(x * 0.5) * 2.0 - 1.0);
}

fn wrap01(x: f32) -> f32 {
    // Modular wrap: teleports across the interval
    return fract(x);
}

fn apply_boundary(x: f32, mode: u32) -> f32 {
    if (mode == GOL_BOUNDARY_WRAP) { return wrap01(x); }
    return reflect01(x);
}

// --- Pulse target: per-cell periodic intensity [0,1]
// Each cell computes a sinusoidal target modulated by per-cell phase offset.
// The result drives the same spring dynamics as Conway's binary alive/dead.
fn pulse_cell_target(cell_x: u32, cell_y: u32, t_beats: f32,
                     tick_period: f32, phase_randomness: f32,
                     tempo_randomness: f32) -> f32 {
    // Per-cell phase offset from hash
    let h = gol_cell_hash(cell_x, cell_y);
    let cell_phase = gol_cell_variation(h) * phase_randomness * 2.0 * PI;

    // Per-cell frequency jitter: each cell oscillates at a slightly different tempo
    // tempo_randomness=0 → all unison. =1 → ±50% frequency variation.
    let h2 = gol_cell_hash(cell_x + 137u, cell_y + 251u);
    let tempo_jitter = 1.0 + (gol_cell_variation(h2) - 0.5) * tempo_randomness;

    // DRIVERLESS since gen-1 retirement — held at neutral by the boot
    // block; revive via a gen-2 coupling or delete on the next pass here.
    let freq = tempo_jitter / max(tick_period * config.mode_gol_tick_scale, 0.1);
    let phase = t_beats * freq * 2.0 * PI + cell_phase;

    // Threshold the sine into a planted on/off target, the way a Conway
    // cell is binary alive/dead. The shared height spring smooths the 0↔1
    // transitions, so the cell rests ON the ground when off and at full
    // height when on, rather than hovering at the sine's 0.5 midpoint.
    return select(0.0, 1.0, sin(phase) > 0.0);
}

// --- Terrain cell color at a world position
// (GOL_TERRAIN_CELL_SIZE retired — a second spelling of the cell size;
//  the ONE-ADDRESS LAW's cell_address is the only address derivation.)

fn gol_composite_cell_color(world_xz: vec2<f32>) -> vec3<f32> {
    let addr = cell_address(world_xz);
    let cell_gx = addr.x;
    let cell_gz = addr.y;
    let cell_seed = lattice_node_seed(config.world_seed, vec2(cell_gx, cell_gz), 200u);

    // Phase 1 (charter D3.2): the manual field duplicate collapsed to the
    // ONE evaluator (bias 0 — no live door bias on the GoL path).
    // CHECKER-REBUILD ruling preserved verbatim: IDENTITY VOICE here —
    // GoL keeps its own panel (ROW 9); RULED separate at the CHECKER cut
    // (Jean): the zones' own coupling pass is coming. STATUS: INTENT —
    // revive when it convenes by passing config.checker_resultant /
    // _music_amount / _music_variance. Here amount 0 -> each cell's seed
    // color. (The identity's archetype is now the real tile lookup — the
    // aura compute layout gained tile_grid for it; archetype is unread by
    // the composite, so output is unchanged.)
    let id = evaluate_cell_fields(world_xz, cell_gx, cell_gz, cell_seed, 0.0, 0.0);
    let dcol = discrete_cell_color_at_tier(world_xz, cell_gx, cell_gz, cell_seed,
                                           id.tier, vec3(0.0), 0.0, 0.0);
    return composite_cell_color(id, dcol);
}

// --- Shared color application (called by terrain FS and extrusion FS)
fn apply_gol_color(base_color: vec3<f32>, zp: GoLZoneConfig, cx: u32, cy: u32, blend: f32) -> vec3<f32> {
    if (zp.color_mode == GOL_COLOR_NEUTRAL) {
        return base_color;  // no color change on terrain
    }

    let h = gol_cell_hash(cx, cy);
    let v = gol_cell_variation(h);

    if (zp.color_mode == GOL_COLOR_LENS) {
        // Gentle shift: keep most of the base color, nudge toward target
        let variation = v * GOL_LENS_VARIATION_RANGE - GOL_LENS_VARIATION_RANGE * 0.5;
        let tint_color = vec3(zp.target_r, zp.target_g, zp.target_b);
        let lens_color = mix(base_color, tint_color, GOL_LENS_BLEND_BASE + variation);
        return mix(base_color, lens_color, blend * GOL_TINT_STRENGTH);
    }

    // BLACKISH: darken the base color, preserving its character
    let dark_variation = v * GOL_BLACK_DARK_RANGE;
    let dark_factor = GOL_BLACK_DARK_BASE + dark_variation;
    let r_shift = f32((h >> 8u) & 0xFFu) / 255.0 * GOL_BLACK_R_SHIFT_RANGE - GOL_BLACK_R_SHIFT_RANGE * 0.5;
    let g_shift = f32((h >> 16u) & 0xFFu) / 255.0 * GOL_BLACK_G_SHIFT_RANGE - GOL_BLACK_G_SHIFT_RANGE * 0.5;
    let alive_color = clamp(base_color * dark_factor + vec3(r_shift, g_shift, -r_shift), vec3(0.0), vec3(1.0));
    return mix(base_color, alive_color, blend * GOL_TINT_STRENGTH);
}

// Extrusion block color: starts from per-cell terrain color, applies mode
// (fn apply_gol_extrusion_color RETIRED — UNIFIED_GROUND_1 U4; A2-3 census)

struct GoLZoneArray {
    count: u32,
    t_beats: f32,
    dt: f32,
    tick_mask: u32,              // bit N = zone N should tick Conway this frame
    zones: array<GoLZoneConfig, 8>,
}

// §7.0c PAWN AURA HELPERS

// --- Pawn Aura: persistent terrain influence via toroidal spring grid
const PAWN_AURA_N: i32 = 64;
const PAWN_AURA_EMPTY: i32 = 0x7FFFFFFF;

struct PawnAuraConfig {
    cell_size: f32,
    influence_radius: f32,
    attack_stiffness: f32,
    attack_damping: f32,
    release_rate: f32,
    dt: f32,
    effect_mask: u32,
    aura_n: u32,
    tint_strength: f32,
    tint_r: f32,
    tint_g: f32,
    tint_b: f32,
    delta_mode: u32,           // 0=convergent (toward tint), 1=random per-cell
    delta_magnitude: f32,      // random mode: max offset per channel
    t_beats: f32,              // current musical time (for oscillation)
    height_scale: f32,         // height contribution scale (world units)
}

const AURA_DELTA_CONVERGENT: u32 = 0u;
const AURA_DELTA_RANDOM: u32 = 1u;

// Helper: sample pawn aura with toroidal lookup and ghost rejection.
// Returns vec4(height_blend, delta_r, delta_g, delta_b) or vec4(0) if ghost/inactive.
//
// Called by contrib_pawn_aura_at_external(xz) and by the inline
// render-side consumers (patch_terrain_vs, zone_extrusion_vs,
// photo_painting FS tinting). NOT called by the pawn's own Y resolve:
// POLICY_WALKER uses contrib_pawn_aura_at_self() which returns the
// scalar peak directly — the pawn knows it sits at its own aura peak
// without reading the directionally-biased grid. See those functions
// for rationale.
//
// Sampler choice — bilinear_sampler, not nearest_sampler. The aura
// is a continuous influence field, not a discrete grid. Consumers
// reading across cell boundaries (terrain rendering, flyers passing
// through the aura's edge) need smooth interpolation; nearest-neighbor
// would produce visible banding at cell boundaries (aura_cs = 3.125 m).
// pawn_aura_read is rgba16float which supports bilinear filtering on
// all target hardware; color deltas (.gba) and zone extrusion's
// suppression target also benefit from the smoother interpolation.
fn sample_pawn_aura(world_xz: vec2<f32>, pawn_xz: vec2<f32>) -> vec4<f32> {
    if (config.aura_enabled < 0.5) { return vec4(0.0); }
    let aura_cs = 3.125;
    let aura_n = 64;
    let half_extent = f32(aura_n) * aura_cs * 0.5;
    if (abs(world_xz.x - pawn_xz.x) >= half_extent ||
        abs(world_xz.y - pawn_xz.y) >= half_extent) {
        return vec4(0.0);
    }
    let acx = i32(floor(world_xz.x / aura_cs));
    let acz = i32(floor(world_xz.y / aura_cs));
    let asx = ((acx % aura_n) + aura_n) % aura_n;
    let asz = ((acz % aura_n) + aura_n) % aura_n;
    let aura_uv = (vec2<f32>(f32(asx), f32(asz)) + 0.5) / f32(aura_n);
    return textureSampleLevel(pawn_aura_read, bilinear_sampler, aura_uv, 0.0);
}

struct PawnAuraCell {
    cell_gx: i32,
    cell_gz: i32,
    intensity: f32,
    velocity: f32,
    delta_r: f32,
    delta_g: f32,
    delta_b: f32,
    height_delta: f32,
    color_osc: f32,
    color_osc_vel: f32,
    _pad0: f32,
    _pad1: f32,
}

// §7.0d SYSTEM BINDINGS

@group(0) @binding(160) var<storage, read_write> zone_config: GoLZoneArray;
@group(0) @binding(161) var<storage, read_write> zone_life: array<f32>;
@group(0) @binding(162) var zone_life_tex_write: texture_storage_2d_array<rg32float, write>;

// --- GoL zone system (Group 1: bindings 31-32, render texture layout)
@group(1) @binding(31) var zone_life_read: texture_2d_array<f32>;
@group(1) @binding(32) var<storage, read> zone_params: GoLZoneArray;
@group(1) @binding(33) var pawn_aura_read: texture_2d<f32>;
@group(1) @binding(34) var live_card_read: texture_2d<f32>;  // GROUND_CARD_1: the live card (sampled; render + compute)

// --- Pawn Aura compute bindings
// (Group 0: dedicated layout with bindings 60, 170-172)
@group(0) @binding(170) var<uniform> pawn_aura_cfg: PawnAuraConfig;
@group(0) @binding(171) var<storage, read_write> pawn_aura_cells: array<PawnAuraCell>;
@group(0) @binding(172) var pawn_aura_tex_write: texture_storage_2d<rgba16float, write>;
@group(0) @binding(31) var live_card_write: texture_storage_2d<rgba16float, write>;  // GROUND_CARD_1: writer kernel
@group(0) @binding(32) var<storage, read_write> live_card_scratch: array<f32>;  // stride-2: Δh, gol — the two-pass writer (TRUEBAND_CONTACT_1)

// --- Zone mesh gen output (Group 0: bindings 167-169, same layout as GoL compute)

// --- Zone heightfield sampling (mesh gen terrain alignment)
// Runtime-sized: capacity is the bound buffer's — the CPU side
// (Dim::MAX_ACTIVE_PATCHES) is the single source; no WGSL twin exists.
// Zone terrain scan covers every active patch slot; overflow to the
// analytic fallback is thereby eliminated, not merely bounded.

// --- Zone Parameter Derivation (GPU-authoritative) ──────────────────────
//
// CPU identifies eligible lattice nodes and allocates zone slots.
// GPU derives all parameters (tier selection, Gaussian sampling, color mode)
// and writes directly to zone_config. Eliminates CPU-side hash computation.

struct ZoneDeriveRequest {
    slot: u32,              // zone_config.zones[slot] to write
    nx: i32,                // lattice node X
    nz: i32,                // lattice node Z
    algorithm: u32,         // 0=Conway, 1=Pulse
    height_enabled: u32,    // 0 or 1 (from CPU height chance roll)
    world_seed: u32,        // master seed for lattice_node_seed
    _pad0: u32,
    _pad1: u32,
}

struct ZoneDeriveRequestArray {
    count: u32,
    _pad0: u32,
    _pad1: u32,
    _pad2: u32,
    requests: array<ZoneDeriveRequest, 8>,
}

@group(0) @binding(166) var<uniform> zone_derive_requests: ZoneDeriveRequestArray;

// Constants for zone derivation (must match CPU GoLZoneSpawnConfig / GoLColorMode)
// (ZONE_DERIVE_EXTENT RETIRED — extent is tier-derived, grid_cells ×
//  ZONE_DERIVE_CELL_SIZE — UNIFIED_GROUND_1 U5; const parked.)
const ZONE_DERIVE_CELL_SIZE: f32      = 3.125;     // PATCH_EXTENT / PATCH_CELL_N
const ZONE_DERIVE_LENS_LO: f32       = 0.2;       // LENS target color floor
const ZONE_DERIVE_LENS_RANGE: f32     = 0.6;       // LENS target color range

// ── Zone color-mode weight vectors ──────────────────────────────
// WHAT: cumulative-weight distributions for a zone's color mode.
// AXES: index = GoLColorMode order [0 neutral, 1 lens, 2 blackish]
//   (must match CPU GoLColorMode, bodies/gol_zones.hpp). Which vector
//   applies is chosen by the zone's actual_height flag: HEIGHT for
//   extruding zones, NO_HEIGHT for flat ones.
// UNITS: probability (each row sums to 1.0).
// CONSUMER: zone_derive_params (the cumulative color-mode pick).
// SENTINEL: NO_HEIGHT[neutral] = 0.00 — a flat zone can never be
//   neutral (always lens or blackish, so flatness stays legible).
// Biography-adjacent: the pick is seed-deterministic per zone.
//                                              neutral  lens  blackish
const GOL_COLOR_WEIGHTS_HEIGHT    = array<f32, 3>(0.30, 0.40, 0.30);
const GOL_COLOR_WEIGHTS_NO_HEIGHT = array<f32, 3>(0.00, 0.55, 0.45);

// Property indices for zone parameter derivation (must match CPU GoLZoneProp / PulseZoneProp)
const ZONE_PROP_TIER: u32         = 921u;
const ZONE_PROP_COLOR_ROLL: u32   = 923u;
const ZONE_PROP_DENSITY: u32      = 930u;
const ZONE_PROP_TICK_PERIOD: u32  = 931u;
const ZONE_PROP_SPRING: u32       = 932u;
const ZONE_PROP_HEIGHT: u32       = 933u;
const ZONE_PROP_TRANSITION: u32   = 934u;
const ZONE_PROP_TARGET_R: u32     = 935u;
const ZONE_PROP_TARGET_G: u32     = 936u;
const ZONE_PROP_TARGET_B: u32     = 937u;
const ZONE_PROP_PULSE_TIER: u32   = 951u;
const ZONE_PROP_PHASE_RANDOM: u32 = 952u;
const ZONE_PROP_WANDER: u32       = 953u;
const ZONE_PROP_TEMPO_RANDOM: u32 = 954u;

// --- Zone Parameter Derivation ──────────────────────────────────────────
// CPU sends minimal requests (slot, node coords, algorithm, height flag).
// GPU derives all parameters from tier tables via Gaussian sampling.
// Dispatched once per newly-spawned zone, before sync/evolve.

@compute @workgroup_size(1)
fn zone_derive_params(@builtin(global_invocation_id) gid: vec3<u32>) {
    let req_idx = gid.x;
    if (req_idx >= zone_derive_requests.count) { return; }
    let req = zone_derive_requests.requests[req_idx];

    let seed = lattice_node_seed(req.world_seed, vec2(req.nx, req.nz), GOL_ZONE_SEED_BAND);

    // Raw center from the lattice node; the corner snap moves BELOW the
    // tier selection — extent is tier-derived (UNIFIED_GROUND_1 U5).
    let raw_cx = (f32(req.nx) + 0.5) * MODE_LATTICE_SPACING;
    let raw_cz = (f32(req.nz) + 0.5) * MODE_LATTICE_SPACING;

    var zc: GoLZoneConfig;
    zc.algorithm = req.algorithm;

    // Target colors (shared by both algorithms)
    zc.target_r = hash_property(seed, ZONE_PROP_TARGET_R) * ZONE_DERIVE_LENS_RANGE + ZONE_DERIVE_LENS_LO;
    zc.target_g = hash_property(seed, ZONE_PROP_TARGET_G) * ZONE_DERIVE_LENS_RANGE + ZONE_DERIVE_LENS_LO;
    zc.target_b = hash_property(seed, ZONE_PROP_TARGET_B) * ZONE_DERIVE_LENS_RANGE + ZONE_DERIVE_LENS_LO;

    let height_enabled = req.height_enabled != 0u;

    if (req.algorithm == GOL_ALGORITHM_CONWAY) {
        // Conway tier selection (cumulative weight)
        let tier_roll = hash_property(seed, ZONE_PROP_TIER);
        var tier_idx: u32 = GOL_TIER_COUNT - 1u;
        var cumul: f32 = 0.0;
        for (var t: u32 = 0u; t < GOL_TIER_COUNT; t++) {
            cumul += GOL_TIERS[t].weight;
            if (tier_roll < cumul) { tier_idx = t; break; }
        }
        let tp = GOL_TIERS[tier_idx];

        // Size: cells, not world units (UNIFIED_GROUND_1 U5)
        zc.grid_size = tp.grid_cells;
        zc.extent    = f32(zc.grid_size) * ZONE_DERIVE_CELL_SIZE;

        let actual_height = height_enabled && (tp.force_no_height == 0u);

        zc.tick_period = max(0.1,
            sample_gaussian(seed, ZONE_PROP_TICK_PERIOD, tp.tick_period_mean, tp.tick_period_sigma));
        zc.spring_stiffness = max(0.1,
            sample_gaussian(seed, ZONE_PROP_SPRING, tp.spring_stiffness_mean, tp.spring_stiffness_sigma));
        zc.transition_fraction = clamp(
            sample_gaussian(seed, ZONE_PROP_TRANSITION, tp.transition_fraction_mean, tp.transition_fraction_sigma),
            0.01, 0.5);
        zc.alive_height = select(0.0,
            max(0.5, sample_gaussian(seed, ZONE_PROP_HEIGHT, tp.alive_height_mean, tp.alive_height_sigma)),
            actual_height);
        zc.spring_variance = tp.spring_variance;

        // Pulse fields: zeroed for Conway
        zc.wander_radius = 0.0;
        zc.phase_randomness = 0.0;
        zc.boundary_mode = GOL_BOUNDARY_REFLECT;
        zc.tempo_randomness = 0.0;

        // Color mode selection (weighted by height state)
        let color_roll = hash_property(seed, ZONE_PROP_COLOR_ROLL);
        zc.color_mode = 2u; // fallback: blackish
        var ccum: f32 = 0.0;
        for (var c: u32 = 0u; c < 3u; c++) {
            if (actual_height) {
                ccum += GOL_COLOR_WEIGHTS_HEIGHT[c];
            } else {
                ccum += GOL_COLOR_WEIGHTS_NO_HEIGHT[c];
            }
            if (color_roll < ccum) { zc.color_mode = c; break; }
        }
    } else {
        // Pulse tier selection
        let tier_roll = hash_property(seed, ZONE_PROP_PULSE_TIER);
        var tier_idx: u32 = GOL_PULSE_TIER_COUNT - 1u;
        var cumul: f32 = 0.0;
        for (var t: u32 = 0u; t < GOL_PULSE_TIER_COUNT; t++) {
            cumul += GOL_PULSE_TIERS[t].weight;
            if (tier_roll < cumul) { tier_idx = t; break; }
        }
        let pp = GOL_PULSE_TIERS[tier_idx];

        // Size: cells, not world units (UNIFIED_GROUND_1 U5)
        zc.grid_size = pp.grid_cells;
        zc.extent    = f32(zc.grid_size) * ZONE_DERIVE_CELL_SIZE;

        let actual_height = height_enabled && (pp.force_no_height == 0u);

        zc.tick_period = max(0.1,
            sample_gaussian(seed, ZONE_PROP_TICK_PERIOD, pp.tick_period_mean, pp.tick_period_sigma));
        zc.spring_stiffness = max(0.1,
            sample_gaussian(seed, ZONE_PROP_SPRING, pp.spring_stiffness_mean, pp.spring_stiffness_sigma));
        zc.transition_fraction = clamp(
            sample_gaussian(seed, ZONE_PROP_TRANSITION, pp.transition_fraction_mean, pp.transition_fraction_sigma),
            0.01, 0.5);
        zc.alive_height = select(0.0,
            max(0.5, sample_gaussian(seed, ZONE_PROP_HEIGHT, pp.alive_height_mean, pp.alive_height_sigma)),
            actual_height);
        zc.phase_randomness = clamp(
            sample_gaussian(seed, ZONE_PROP_PHASE_RANDOM, pp.phase_randomness_mean, pp.phase_randomness_sigma),
            0.0, 1.0);
        zc.wander_radius = max(0.0,
            sample_gaussian(seed, ZONE_PROP_WANDER, pp.wander_radius_mean, pp.wander_radius_sigma));
        zc.boundary_mode = pp.boundary_mode;
        zc.tempo_randomness = clamp(
            sample_gaussian(seed, ZONE_PROP_TEMPO_RANDOM, pp.tempo_randomness_mean, pp.tempo_randomness_sigma),
            0.0, 1.0);
        zc.spring_variance = pp.spring_variance;

        // Pulse zones always use LENS color mode
        zc.color_mode = GOL_COLOR_LENS;
    }

    // Zone origin: snap corner to the cell grid with the TIER-DERIVED
    // extent, then center (the same snap formula; extent now varies).
    let corner_x = floor((raw_cx - zc.extent * 0.5) / ZONE_DERIVE_CELL_SIZE) * ZONE_DERIVE_CELL_SIZE;
    let corner_z = floor((raw_cz - zc.extent * 0.5) / ZONE_DERIVE_CELL_SIZE) * ZONE_DERIVE_CELL_SIZE;
    zc.origin = vec2(corner_x + zc.extent * 0.5, corner_z + zc.extent * 0.5);

    zone_config.zones[req.slot] = zc;
}

// THE VOCABULARY MASK (UNIFIED_GROUND_1 U5) — the birth-moment kernel.
// Multiplies the color system's REST discrete-visibility predicate into
// the CPU-Gaussian-seeded height_factor plane: smooth ground does not
// extrude, lift, or carry walker height. STATIC at birth (the dynamic,
// tide-following form is Layer E — campaign v2 §9).
// GRANULARITY TRUTH: the predicate is evaluated at ZONE-cell centers,
// which ARE color-mosaic cells (extent = grid × 3.125, corner cell-
// snapped) — one address, no resampling.
// ORDERING LAW: derive writes zone_config[slot]; this kernel reads it —
// sequential dispatches in ONE pass suffice (storage-buffer visibility
// between dispatches is guaranteed).
@compute @workgroup_size(8, 8, 1)
fn zone_seed_mask(@builtin(global_invocation_id) gid: vec3<u32>) {
    let req_idx = gid.z;
    if (req_idx >= zone_derive_requests.count) { return; }
    let req = zone_derive_requests.requests[req_idx];
    let zp = zone_config.zones[req.slot];
    if (gid.x >= zp.grid_size || gid.y >= zp.grid_size) { return; }
    let cs = zp.extent / f32(zp.grid_size);
    let corner = zp.origin - vec2(zp.extent * 0.5);
    let center = corner + (vec2(f32(gid.x), f32(gid.y)) + vec2(0.5)) * cs;
    let vis = step(0.5, discrete_visibility_rest(center, cell_address(center)));
    // the sim kernels' own dense row-major (idx = y * grid_size + x) —
    // NOT a fixed-32 stride; the U5b gate (a) verified convention.
    let idx = req.slot * GOL_ZONE_STRIDE + GOL_CELL_HEIGHT_FACTOR
            + gid.y * zp.grid_size + gid.x;
    zone_life[idx] = zone_life[idx] * vis;   // Gaussian seed × mask
}

// Sample baked heightfield at world XZ — exact terrain as rendered.
// Searches patch instances for the covering patch and samples its layer.
// (fn zone_sample_baked_terrain_y RETIRED — UNIFIED_GROUND_1 U4; A2-3 census)

const ZONE_MESH_MAX_INDICES: u32 = 75000u;


// §7.1 COMPUTE ENTRY POINTS
// Execution order (critical for correctness):
// RAYMARCH/SDF EXCAVATION: update_terrain_config removed — the writer kernel
// of the dead TerrainState buffer (it wrote amplitude_scale + lipschitz_factor,
// read by nobody). Its C++ dispatch + pipeline went with it; the terrain_state
// buffer/bindings/struct husk was swept.

// --- Walker terrain normal (forward-difference)
// POLICY_WALKER_TILT samples — static_base + pyramids + GoL zones
// (pawn-suppressed) + terrain waves + radial pulses. Excludes
// CONTRIB_PAWN_AURA (the self form is a zero-gradient scalar — self
// fields never drive tilt); it CARRIES the same pawn-centered GoL
// suppression the walker applies, which is flat (supp_factor = 1, zero
// gradient) within the eps = 0.5 sample ring, so no slope is
// manufactured. The pawn still stands on full POLICY_WALKER ground;
// only the tilt direction reads the tilt-safe policy.
fn terrain_normal_at(xz: vec2<f32>, qi: QueryInputs) -> vec3<f32> {
    // b1: the pawn's tilt normal now flows through
    // the manifold interface (POLICY_WALKER_TILT). BYTE-IDENTICAL — the
    // cast's normal is the same eps=0.5 finite-diff of
    // query_ground_walker_tilt this body computed inline (same three
    // samples, same dx/dz, same normalize(vec3(-dx,1,-dz))); query_pos.y
    // is ignored (only .xz is read). The normal is now RETURNED by the
    // cast, no longer reconstructed here — the Y-up 1.0 moved into the
    // cast, which is where the sphere will replace it.
    return manifold_resolve(vec3(xz.x, 0.0, xz.y), POLICY_WALKER_TILT, qi).normal;
}

// --- Pawn ground resolve
//
// Two policies, not one:
//   POLICY_WALKER       gives the resolved standing height (returned y).
//                       The pawn rides aura-lifted ground, so its y must
//                       include CONTRIB_PAWN_AURA + CONTRIB_GOL_SUPPRESSION.
//   POLICY_WALKER_TILT  gives step-climb-safe heights for the
//                       PAWN_STEP_HEIGHT comparison. Excludes the two
//                       self-centered contributors so the pawn can't
//                       "trip on its own aura" or self-suppression
//                       gradient between frames.
//
// Both values are computed together via query_ground_walker_pair, which
// evaluates the shared 5-contributor base once and returns both walker
// and tilt heights. This halves the compile-time expansion of the
// contributor chain compared to paired calls to the individual query
// functions.
//
// prev_y is the aura-lifted standing height from last frame's resolve.
// prev_y_tilt is computed fresh from the paired query at prev_xz (one
// extra evaluation per frame; no pawn_state field added — see follow-up
// brief Part C.3d for the rationale).
fn pawn_ground_resolve(
    new_xz: vec2<f32>, prev_xz: vec2<f32>, prev_y: f32, qi: QueryInputs
) -> vec4<f32> {
    // Paired queries at every candidate position — compute walker and
    // tilt together, share the 5-contributor base.
    let new_pair   = query_ground_walker_pair(new_xz,  qi);
    let prev_pair  = query_ground_walker_pair(prev_xz, qi);
    let y           = new_pair.x;
    let y_tilt      = new_pair.y;
    let prev_y_tilt = prev_pair.y;

    // No XZ movement (idle/bootstrap) or passable slope → just snap
    let moved = any(new_xz != prev_xz);
    if (!moved || y_tilt - prev_y_tilt <= PAWN_STEP_HEIGHT) {
        return vec4(new_xz.x, y, new_xz.y, 1.0);              // happy path
    }

    // Full move blocked — try axis-aligned slides. Each axis reads
    // both walker (the y the pawn would stand at) and walker_tilt
    // (the step-climb comparison) from a single paired query.
    let slide_x = vec2(new_xz.x, prev_xz.y);
    let x_pair  = query_ground_walker_pair(slide_x, qi);
    let x_ok = (x_pair.y - prev_y_tilt) <= PAWN_STEP_HEIGHT;

    let slide_z = vec2(prev_xz.x, new_xz.y);
    let z_pair  = query_ground_walker_pair(slide_z, qi);
    let z_ok = (z_pair.y - prev_y_tilt) <= PAWN_STEP_HEIGHT;

    if (x_ok && z_ok) {
        if (abs(new_xz.x - prev_xz.x) >= abs(new_xz.y - prev_xz.y)) {
            return vec4(slide_x.x, x_pair.x, slide_x.y, 1.0);
        }
        return vec4(slide_z.x, z_pair.x, slide_z.y, 1.0);
    }
    if (x_ok) { return vec4(slide_x.x, x_pair.x, slide_x.y, 1.0); }
    if (z_ok) { return vec4(slide_z.x, z_pair.x, slide_z.y, 1.0); }

    // Fully blocked — revert, reuse prev_y (was snapped last frame)
    return vec4(prev_xz.x, prev_y, prev_xz.y, 0.0);
}

// ═══ AGENT POST-STEP HELPER ══════════════════════════════════════
//
// Behaviors only modify a.vel_x / a.vel_z. The post-step applies the
// rest: drag, speed cap, position integration, ground snap, heading
// from velocity. Pulled out of each behavior body so FXC compiles
// the common epilogue once per kernel rather than ten times.

// ─── Shared step trigger ─────────────────────────────────────────
// Most behaviors are beat-gated: they apply an impulse once per
// step_rate-derived musical beat and otherwise let drag + post-step
// shape the motion. The helper checks whether a step boundary was
// crossed since last frame and returns the step index (used as a
// hash seed for the per-step direction draw).
//
// Pulled out so the eight step-driven behaviors don't each carry
// their own three-line floor-and-compare prelude.

struct StepTrigger {
    fired:    bool,
    step_idx: u32,
}

fn step_trigger(step_rate: f32) -> StepTrigger {
    let t_beats   = signal.t_beats;
    let dt_beats  = signal.dt_beats;
    let step_idx      = u32(floor(t_beats * step_rate));
    let prev_step_idx = u32(floor(max(0.0, t_beats - dt_beats) * step_rate));
    return StepTrigger(step_idx > prev_step_idx, step_idx);
}

// ─── Shared post-step ────────────────────────────────────────────
// Behaviors only modify a.vel_x / a.vel_z. This helper applies the
// rest: exponential drag, speed cap, position integration, ground
// snap, and heading-from-velocity. Pulled out of each behavior body
// so FXC compiles it once per kernel rather than ten times.
//
// Each behavior reads its own (drag, speed_cap) from
// agent_behaviors[id], with tier scaling applied via
// agent_tier_gains[tier_idx].speed_gain — that's why both the
// raw cap and the tier multiplier come in as parameters.
fn agent_post_step(agent_in: AgentState, drag: f32, speed_cap: f32, speed_gain: f32) -> AgentState {
    var a = agent_in;
    let dt = signal.dt;
    let t  = signal.t_seconds;

    // Drag (physical, in seconds).
    let decay = exp(-drag * dt);
    a.vel_x *= decay;
    a.vel_z *= decay;

    // Speed cap.
    let sp2 = a.vel_x * a.vel_x + a.vel_z * a.vel_z;
    let cap = speed_cap * speed_gain;
    if (sp2 > cap * cap) {
        let inv = cap / sqrt(sp2);
        a.vel_x *= inv;
        a.vel_z *= inv;
    }

    // Position integration.
    a.pos_x += a.vel_x * dt;
    a.pos_z += a.vel_z * dt;

    // Ground snap (walker policy — base + pyramids + GoL + waves + pulses + aura).
    let qi = QueryInputs(vec3(a.pos_x, a.pos_y, a.pos_z), t);
    a.pos_y = manifold_position(vec3(a.pos_x, 0.0, a.pos_z), POLICY_WALKER_AGENT, qi).y;

    // Heading from velocity (when moving).
    if (sp2 > 0.01) {
        a.heading = atan2(a.vel_x, a.vel_z);
        let hq = quat_from_axis_angle(vec3(0.0, 1.0, 0.0), a.heading);
        a.orient_x = hq.x;
        a.orient_y = hq.y;
        a.orient_z = hq.z;
        a.orient_w = hq.w;
    }

    return a;
}

// ═══ BEHAVIOR IMPLEMENTATIONS ════════════════════════════════════
//
// Each behavior is a pure function (AgentState) -> AgentState that
// modifies velocity only — drag, integration, ground snap, heading
// are factored into agent_post_step above. The kernel switch
// dispatches on agent.behavior_id; slot numbers must match the
// AgentBehaviorId enum in bodies/agents.hpp. Behavior parameters
// come from agent_behaviors[behavior_id] (uploaded once at world
// init from the C++ AGENT_BEHAVIORS table).

// ─── Behavior: PlayerControlled ──────────────────────────────────
// The body the player is currently inhabiting. Reads input/couplings,
// resolves ground, computes tilt orientation, detects portal triggers.
// Only meaningful for the slot whose behavior_id is PLAYER_CONTROLLED
// — by convention that is the same slot as config.possessed_slot.
fn behavior_player_controlled(agent_in: AgentState) -> AgentState {
    var agent = agent_in;

    // Sky mode: the pawn is mounted on the ribbon head. Snap to the head pose
    // (delivered per-frame in the signal) and skip walking, ground-resolve, and
    // terrain tilt entirely. SEAM[ribbon:sky-mode].
    if (signal.sky_mode != 0u) {
        agent.pos_x = signal.sky_head_x;
        agent.pos_y = signal.sky_head_y;
        agent.pos_z = signal.sky_head_z;
        agent.heading = signal.sky_heading;
        agent.vel_x = 0.0;
        agent.vel_y = 0.0;
        agent.vel_z = 0.0;
        // The saddle joins the frame law: the rider wears the FULL
        // frame — heading deflected by the tangent-align yaw, pitch with
        // the vertical wave, roll into the bank. Angles arrive CPU-computed
        // in the sky block (bodies/ribbon.hpp MOUNT_* mirrors); composed here in
        // ribbon_ring_motor's verified order — roll first, then pitch,
        // then yaw (quat_multiply applies its SECOND argument first).
        // SEAM[ribbon:sky-mode].
        // Negated: quat_rotate maps +X to (cos θ, −sin θ); the heading speaks
        // dir(θ) = (cos θ, +sin θ). Same mirror as the ring motor, same fix.
        let q_yaw   = quat_from_axis_angle(vec3(0.0, 1.0, 0.0),
                                           -signal.sky_heading - signal.sky_yaw_off);
        let q_pitch = quat_from_axis_angle(vec3(0.0, 0.0, 1.0), signal.sky_pitch);
        let q_roll  = quat_from_axis_angle(vec3(1.0, 0.0, 0.0), signal.sky_roll);
        let sky_q   = quat_multiply(q_yaw, quat_multiply(q_pitch, q_roll));
        agent.orient_x = sky_q.x;
        agent.orient_y = sky_q.y;
        agent.orient_z = sky_q.z;
        agent.orient_w = sky_q.w;
        return agent;
    }

    let dt = signal.dt;
    let prev_xz = vec2(agent.pos_x, agent.pos_z);
    let prev_y  = agent.pos_y;

    // The intent channel routes to the point's HOST —
    // when the camera hosts (free-fly) the body idles (the else arm
    // zeroes velocity; the pawn stands, snapped where it is).
    if (coupling_active(COUPLING_INPUT_MOVES_PLAYER) && !point_camera_hosted()) {
        let input_dir = vec2(signal.move_x, signal.move_z);
        let world_vel = coupling_input_to_pawn_velocity(input_dir, camera_state.azimuth);

        let speed = select(PAWN_SPEED, config.pawn_speed, config.pawn_speed > 0.0);
        agent.pos_x += world_vel.x * speed * dt;
        agent.pos_z += world_vel.y * speed * dt;

        if (fpv_mode_active()) {
            agent.heading = camera_state.azimuth;
        } else {
            agent.heading = coupling_velocity_to_pawn_heading(world_vel, agent.heading, dt);
        }

        agent.vel_x = world_vel.x * speed;
        agent.vel_z = world_vel.y * speed;
    } else {
        agent.vel_x = 0.0;
        agent.vel_z = 0.0;

        if (fpv_mode_active()) {
            agent.heading = camera_state.azimuth;
        }
    }

    // --- Finite world boundary clamp
    if (config.world_bound_max.x > 0.0) {
        agent.pos_x = clamp(agent.pos_x, config.world_bound_min.x, config.world_bound_max.x);
        agent.pos_z = clamp(agent.pos_z, config.world_bound_min.y, config.world_bound_max.y);
    }

    // --- Ground resolve: single query_ground_walker call.
    // POLICY_WALKER includes static base + pyramids + GoL zones + terrain
    // waves + radial pulses + pawn aura − consumer-local GoL suppression
    // (centered on the agent's start-of-frame position via qi.consumer_pos).
    let qi = QueryInputs(vec3(prev_xz.x, prev_y, prev_xz.y), signal.t_seconds);

    if (coupling_active(COUPLING_TERRAIN_TO_PAWN_Y)) {
        let resolved = pawn_ground_resolve(vec2(agent.pos_x, agent.pos_z), prev_xz, prev_y, qi);
        agent.pos_x = resolved.x;
        agent.pos_y = resolved.y;
        agent.pos_z = resolved.z;
        if (resolved.w < 0.5) {
            agent.vel_x = 0.0;
            agent.vel_z = 0.0;
        }
    }

    // --- Orientation: heading + walker-policy terrain tilt
    if (coupling_active(COUPLING_TERRAIN_TO_PAWN_TILT)) {
        let normal = terrain_normal_at(vec2(agent.pos_x, agent.pos_z), qi);

        let world_up = vec3(0.0, 1.0, 0.0);
        let d = dot(world_up, normal);
        var tilt_quat: vec4<f32>;
        if (d < 0.9999) {
            let axis = normalize(cross(world_up, normal));
            let angle = acos(clamp(d, -1.0, 1.0));
            tilt_quat = quat_from_axis_angle(axis, angle);
        } else {
            tilt_quat = vec4(0.0, 0.0, 0.0, 1.0);
        }

        let heading_quat = quat_from_axis_angle(vec3(0.0, 1.0, 0.0), agent.heading);
        let orient = quat_multiply(tilt_quat, heading_quat);
        agent.orient_x = orient.x;
        agent.orient_y = orient.y;
        agent.orient_z = orient.z;
        agent.orient_w = orient.w;
    }

    // --- Portal ellipse detection (GPU-authoritative)
    // Ellipse spans the arch opening: lateral = half_span, forward = depth/2.
    //
    // THE POINT'S BUBBLE: the portal is the bubble's FIRST
    // SENSOR — the probe is host-sourced. When the pawn hosts, the
    // probe is the body's pos THIS FRAME (agent.pos, the local —
    // byte-identical to the pre-point test; point_pos() is deliberately
    // NOT used here: it reads the storage copy, one frame stale).
    // When the camera hosts, the probe is the point (the camera) and
    // the bubble's VERTICAL GATE applies: the arch must sit within
    // POINT_BUBBLE_RADIUS of the point's altitude — skim over a portal
    // and it fires; fly high above its xz and the arch is outside the
    // bubble, no fire (Jean's altitude ruling). The ground query runs
    // only on an xz-hit (at most one per frame). The trigger stays on
    // the possessed slot's wire, harvested by the same P5 path.
    agent.portal_trigger = -1;
    var probe = vec3(agent.pos_x, agent.pos_y, agent.pos_z);
    if (point_camera_hosted()) { probe = camera_state.pos; }
    for (var pi = 0u; pi < portal_array.count; pi++) {
        let p = portal_array.portals[pi];
        let dx = probe.x - p.x;
        let dz = probe.z - p.z;
        let lat = dx * p.facing_cos + dz * p.facing_sin;
        let fwd = -dx * p.facing_sin + dz * p.facing_cos;
        let e = lat * lat * p.inv_span_sq + fwd * fwd * p.inv_depth_sq;
        if (e < 1.0) {
            var in_bubble = true;
            if (point_camera_hosted()) {
                let qi = QueryInputs(vec3(p.x, 0.0, p.z), signal.t_seconds);
                let arch_ground = manifold_position(vec3(p.x, 0.0, p.z), POLICY_FLYER, qi).y;
                in_bubble = abs(probe.y - arch_ground) < POINT_BUBBLE_RADIUS;
            }
            if (in_bubble) {
                agent.portal_trigger = i32(p.arch_index);
                break;
            }
        }
    }

    return agent;
}

// ─── Behavior: RandomWalk ────────────────────────────────────────
// Per-step velocity impulse in a random direction, decayed by drag
// and capped at speed_cap. Ground snaps via POLICY_WALKER_AGENT
// (static base + pyramids + GoL zones + waves + pulses + external
// pawn aura — no self-suppression). Heading follows velocity.
//
// Step timing is *musical*: step_rate is in steps per beat, and the
// floor() comparison runs against t_beats. At slow tempo, steps are
// rare and drag fully decays each impulse — discrete jumps. At fast
// tempo, steps fire more often, drag doesn't fully decay — continuous
// motion. Tempo shapes the rhythm of decisions; physical integration
// (drag, position update, speed cap) stays in seconds.
//
// Per-step direction is hash(seed, step_idx), making motion fully
// deterministic given the agent's spawn seed.
fn behavior_random_walk(agent_in: AgentState) -> AgentState {
    var a = agent_in;

    let b = agent_behaviors[1u];
    let tier = min(a.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let g = agent_tier_gains[tier];

    // Step trigger — beat-gated. step_rate is steps per beat.
    let s = step_trigger(b.step_rate);
    if (s.fired) {
        let theta   = hash_property(a.seed, 7000u + s.step_idx) * 6.28318530718;
        // Per-step velocity impulse — magnitude is fixed regardless of
        // tempo. Drag (in seconds) decays it; how much of it survives
        // until the next step depends on the tempo.
        let impulse = b.step_size * g.step_gain;
        a.vel_x += cos(theta) * impulse;
        a.vel_z += sin(theta) * impulse;
    }

    return agent_post_step(a, b.drag, b.speed_cap, g.speed_gain);
}

// ─── Behavior: BiasedWalk ────────────────────────────────────────
// RandomWalk with a persistent travel direction baked at spawn time
// from the seed. Each step's angle is the travel direction plus a
// noise term — the agent drifts mostly along its travel azimuth, with
// wobble. Soft cohesion with neighbors: each step samples a couple of
// random other slots; if any are within neighbor_radius, the agent
// gains a small impulse toward their centroid.
//
// Aesthetic: desert travelers. Loose groups drift across the terrain,
// individuals visibly heading somewhere rather than milling in place.
// Step rhythm in beats; physics in seconds.
fn behavior_biased_walk(agent_in: AgentState) -> AgentState {
    var a = agent_in;
    let dt        = signal.dt;

    let b = agent_behaviors[2u];
    let tier = min(a.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let g = agent_tier_gains[tier];

    // Travel direction — derived from seed, stable across the agent's life.
    let travel_dir = hash_property(a.seed, 9100u) * 6.28318530718;
    // persistence in [0,1]: 1 = always exactly travel_dir, 0 = full random.
    // Noise arc (radians) is (1 - persistence) × π.
    let noise_arc = (1.0 - clamp(b.persistence * g.persist_gain, 0.0, 1.0)) * 3.14159265;

    // Step trigger.
    let s = step_trigger(b.step_rate);
    if (s.fired) {
        let noise = (hash_property(a.seed, 9200u + s.step_idx) - 0.5) * 2.0 * noise_arc;
        let theta = travel_dir + noise;
        let impulse = b.step_size * g.step_gain;
        a.vel_x += cos(theta) * impulse;
        a.vel_z += sin(theta) * impulse;

        // Soft cohesion — sample 2 other slots for centroid pull.
        // Cheap O(1) — we don't iterate the full array.
        if (b.neighbor_radius > 0.0) {
            var cx = 0.0;
            var cz = 0.0;
            var n  = 0u;
            for (var k = 0u; k < 2u; k = k + 1u) {
                let other_slot = (a.seed + s.step_idx * 31u + k * 7919u) % 32u;
                if (other_slot == config.possessed_slot) { continue; }
                let other = agent_state[other_slot];
                if (other.is_active == 0u) { continue; }
                let odx = other.pos_x - a.pos_x;
                let odz = other.pos_z - a.pos_z;
                let od2 = odx * odx + odz * odz;
                if (od2 < b.neighbor_radius * b.neighbor_radius && od2 > 0.001) {
                    cx = cx + other.pos_x;
                    cz = cz + other.pos_z;
                    n = n + 1u;
                }
            }
            if (n > 0u) {
                let inv = 1.0 / f32(n);
                let centroid_x = cx * inv;
                let centroid_z = cz * inv;
                let pdx = centroid_x - a.pos_x;
                let pdz = centroid_z - a.pos_z;
                let plen = sqrt(pdx * pdx + pdz * pdz);
                if (plen > 0.001) {
                    // Small pull — at 20% of step impulse.
                    let pull = impulse * 0.2;
                    a.vel_x = a.vel_x + (pdx / plen) * pull;
                    a.vel_z = a.vel_z + (pdz / plen) * pull;
                }
            }
        }
    }

    return agent_post_step(a, b.drag, b.speed_cap, g.speed_gain);
}

// ─── Behavior: SlowPatrol ────────────────────────────────────────
// Walks slowly between successive waypoints near home. Each waypoint
// is a deterministic offset from home, derived from waypoint_idx via
// hash. Waypoint advances every N beats. Pauses (drag-only motion)
// for a half-beat after arriving before heading to the next.
//
// Aesthetic: gallery walkers. Patient, contemplative motion.
// Multiple agents authored with overlapping homes naturally form
// loose gathering groups when their waypoint cycles intersect.
fn behavior_slow_patrol(agent_in: AgentState) -> AgentState {
    var a = agent_in;
    let dt        = signal.dt;
    let t_beats   = signal.t_beats;

    let b = agent_behaviors[5u];
    let tier = min(a.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let g = agent_tier_gains[tier];

    // Waypoint advances at b.step_rate (per beat). Each waypoint
    // is a hash-derived offset from home, scaled by step_size × tier.
    let waypoint_idx = u32(floor(t_beats * b.step_rate));
    let theta = hash_property(a.seed, 9300u + waypoint_idx) * 6.28318530718;
    let radius = hash_property(a.seed, 9400u + waypoint_idx) * b.step_size * g.step_gain;
    let target_x = a.home_x + cos(theta) * radius;
    let target_z = a.home_z + sin(theta) * radius;

    // Steer toward target with constant force (proportional, soft).
    let dx = target_x - a.pos_x;
    let dz = target_z - a.pos_z;
    let dist = sqrt(dx * dx + dz * dz);

    if (dist > 0.5) {
        // Pull strength: home_pull (the spring constant) gives steady-state
        // velocity = pull × dt for each integration step. Tier persist_gain
        // scales the force — Sentinels pull harder, Scouts pull weaker.
        let pull = b.home_pull * g.persist_gain;
        let inv  = 1.0 / dist;
        a.vel_x = a.vel_x + dx * inv * pull * dt;
        a.vel_z = a.vel_z + dz * inv * pull * dt;
    }
    // (Within 0.5 units of target: drag-only motion = pause.)

    return agent_post_step(a, b.drag, b.speed_cap, g.speed_gain);
}

// ─── Behavior: Wanderer ──────────────────────────────────────────
// Random walk with a soft tether to home. Like RandomWalk but the
// agent has a place it belongs and slowly drifts back to it. No
// persistent direction (unlike BiasedWalk). Each step is fresh.
//
// Aesthetic: a creature grazing in its patch — free movement within
// a territory.
fn behavior_wanderer(agent_in: AgentState) -> AgentState {
    var a = agent_in;
    let dt        = signal.dt;

    let b = agent_behaviors[3u];
    let tier = min(a.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let g = agent_tier_gains[tier];

    // Step trigger.
    let s = step_trigger(b.step_rate);
    if (s.fired) {
        let theta = hash_property(a.seed, 7100u + s.step_idx) * 6.28318530718;
        let impulse = b.step_size * g.step_gain;
        a.vel_x += cos(theta) * impulse;
        a.vel_z += sin(theta) * impulse;
    }

    // Continuous home tether — gentle pull toward home each frame.
    let dx = a.home_x - a.pos_x;
    let dz = a.home_z - a.pos_z;
    let dist = sqrt(dx * dx + dz * dz);
    if (dist > 0.5) {
        let pull = b.home_pull * g.persist_gain;
        let inv  = 1.0 / dist;
        a.vel_x = a.vel_x + dx * inv * pull * dt;
        a.vel_z = a.vel_z + dz * inv * pull * dt;
    }

    return agent_post_step(a, b.drag, b.speed_cap, g.speed_gain);
}

// ─── Behavior: HomeSeeker ────────────────────────────────────────
// Strong spring to home with a small noise impulse on each step.
// Unlike Wanderer (soft tether) the home_pull dominates — agent
// always returns. Unlike SlowPatrol (waypoints), there is no fixed
// destination, just a centerpoint with restless local motion.
//
// Aesthetic: a sentinel at its post — paces, returns, paces again.
fn behavior_home_seeker(agent_in: AgentState) -> AgentState {
    var a = agent_in;
    let dt        = signal.dt;

    let b = agent_behaviors[4u];
    let tier = min(a.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let g = agent_tier_gains[tier];

    // Step trigger — small random noise impulse.
    let s = step_trigger(b.step_rate);
    if (s.fired) {
        let theta = hash_property(a.seed, 7200u + s.step_idx) * 6.28318530718;
        let impulse = b.step_size * g.step_gain;
        a.vel_x += cos(theta) * impulse;
        a.vel_z += sin(theta) * impulse;
    }

    // Strong spring to home — dominant force.
    let dx = a.home_x - a.pos_x;
    let dz = a.home_z - a.pos_z;
    let dist_sq = dx * dx + dz * dz;
    if (dist_sq > 0.25) {
        let pull = b.home_pull * g.persist_gain;
        a.vel_x = a.vel_x + dx * pull * dt;
        a.vel_z = a.vel_z + dz * pull * dt;
    }

    return agent_post_step(a, b.drag, b.speed_cap, g.speed_gain);
}

// ─── Behavior: Pursuit ───────────────────────────────────────────
// Steers toward the player. Engages only when the player is within
// neighbor_radius — outside that range, agent reverts to RandomWalk-
// style wandering (subtle drift). Direction is sampled fresh each
// frame from the live player position.
//
// Aesthetic: a curious follower — a child trailing behind a parent,
// a dog interested in the new visitor.
fn behavior_pursuit(agent_in: AgentState) -> AgentState {
    var a = agent_in;
    let dt        = signal.dt;

    let b = agent_behaviors[6u];
    let tier = min(a.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let g = agent_tier_gains[tier];

    // Read the player position (live, not staged).
    let player = agent_state[config.possessed_slot];
    let dx = player.pos_x - a.pos_x;
    let dz = player.pos_z - a.pos_z;
    let dist_sq = dx * dx + dz * dz;
    let detect_sq = b.neighbor_radius * b.neighbor_radius;

    if (dist_sq < detect_sq && dist_sq > 0.5) {
        // In range — steer toward player. Continuous force, frame-rate
        // independent (scaled by dt).
        let pull = b.home_pull * g.persist_gain;
        let inv = 1.0 / sqrt(dist_sq);
        a.vel_x = a.vel_x + dx * inv * pull * dt;
        a.vel_z = a.vel_z + dz * inv * pull * dt;
    } else {
        // Out of range — wander gently on beat-time.
        let s = step_trigger(b.step_rate);
        if (s.fired) {
            let theta = hash_property(a.seed, 7300u + s.step_idx) * 6.28318530718;
            let impulse = b.step_size * g.step_gain;
            a.vel_x += cos(theta) * impulse;
            a.vel_z += sin(theta) * impulse;
        }
    }

    return agent_post_step(a, b.drag, b.speed_cap, g.speed_gain);
}

// ─── Behavior: Flee ──────────────────────────────────────────────
// Inverse of Pursuit. Steers AWAY from the player when within
// neighbor_radius. Outside that range, gentle idle wander on
// beat-time so the agent doesn't appear frozen.
//
// Aesthetic: a shy creature, an avoider. Still alive when the
// player is far — drifting gently — but flees actively when the
// player approaches.
fn behavior_flee(agent_in: AgentState) -> AgentState {
    var a = agent_in;
    let dt        = signal.dt;

    let b = agent_behaviors[7u];
    let tier = min(a.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let g = agent_tier_gains[tier];

    let player = agent_state[config.possessed_slot];
    let dx = a.pos_x - player.pos_x;  // away vector
    let dz = a.pos_z - player.pos_z;
    let dist_sq = dx * dx + dz * dz;
    let alarm_sq = b.neighbor_radius * b.neighbor_radius;

    if (dist_sq < alarm_sq && dist_sq > 0.5) {
        // Player too close — flee. Force scales with proximity:
        // closer = stronger push.
        let proximity = 1.0 - sqrt(dist_sq) / b.neighbor_radius;
        let pull = b.home_pull * g.persist_gain * proximity;
        let inv = 1.0 / sqrt(dist_sq);
        a.vel_x = a.vel_x + dx * inv * pull * dt;
        a.vel_z = a.vel_z + dz * inv * pull * dt;
    } else {
        // Out of alarm range — gentle idle wander on beat-time.
        let s = step_trigger(b.step_rate);
        if (s.fired) {
            let theta = hash_property(a.seed, 7700u + s.step_idx) * 6.28318530718;
            let impulse = b.step_size * g.step_gain;
            a.vel_x += cos(theta) * impulse;
            a.vel_z += sin(theta) * impulse;
        }
    }

    return agent_post_step(a, b.drag, b.speed_cap, g.speed_gain);
}

// ─── Behavior: Flock2D ───────────────────────────────────────────
// Vicsek-style alignment. On each step, sample 3 random other slots;
// for those within neighbor_radius, accumulate (a) their position
// (centroid → cohesion) and (b) their velocity vector (heading →
// alignment). Blend agent's own velocity toward the average.
//
// Aesthetic: birds, fish, schools. The most visually striking
// behavior when populations are dense enough for emergent flow.
fn behavior_flock2d(agent_in: AgentState) -> AgentState {
    var a = agent_in;
    let dt        = signal.dt;

    let b = agent_behaviors[8u];
    let tier = min(a.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let g = agent_tier_gains[tier];

    // Step trigger — flock decisions are beat-gated.
    let s = step_trigger(b.step_rate);
    if (s.fired) {
        // Random direction noise (small — alignment dominates).
        let theta = hash_property(a.seed, 7400u + s.step_idx) * 6.28318530718;
        let noise_arc = (1.0 - clamp(b.persistence * g.persist_gain, 0.0, 1.0)) * 0.78;
        let noisy_theta = theta * noise_arc;
        let impulse_n = b.step_size * g.step_gain * 0.15;
        a.vel_x += cos(noisy_theta) * impulse_n;
        a.vel_z += sin(noisy_theta) * impulse_n;

        // Sample 4 neighbors for cohesion + alignment.
        // Wider sample improves chance of finding flock mates with
        // small populations.
        var cx = 0.0;
        var cz = 0.0;
        var ax = 0.0;
        var az = 0.0;
        var n  = 0u;
        for (var k = 0u; k < 4u; k = k + 1u) {
            let other_slot = (a.seed + s.step_idx * 31u + k * 7919u) % 32u;
            if (other_slot == config.possessed_slot) { continue; }
            let other = agent_state[other_slot];
            if (other.is_active == 0u) { continue; }
            let odx = other.pos_x - a.pos_x;
            let odz = other.pos_z - a.pos_z;
            let od2 = odx * odx + odz * odz;
            if (od2 < b.neighbor_radius * b.neighbor_radius && od2 > 0.001) {
                cx = cx + other.pos_x;
                cz = cz + other.pos_z;
                ax = ax + other.vel_x;
                az = az + other.vel_z;
                n = n + 1u;
            }
        }
        if (n > 0u) {
            let inv_n = 1.0 / f32(n);
            // Cohesion pull toward centroid.
            let centroid_x = cx * inv_n;
            let centroid_z = cz * inv_n;
            let pdx = centroid_x - a.pos_x;
            let pdz = centroid_z - a.pos_z;
            let plen = sqrt(pdx * pdx + pdz * pdz);
            if (plen > 0.001) {
                let cohesion = b.step_size * g.step_gain * 0.5;
                a.vel_x = a.vel_x + (pdx / plen) * cohesion;
                a.vel_z = a.vel_z + (pdz / plen) * cohesion;
            }
            // Alignment — strongly blend toward average velocity.
            let avg_vx = ax * inv_n;
            let avg_vz = az * inv_n;
            let align_strength = 0.75;
            a.vel_x = mix(a.vel_x, avg_vx, align_strength);
            a.vel_z = mix(a.vel_z, avg_vz, align_strength);
        }
    }

    return agent_post_step(a, b.drag, b.speed_cap, g.speed_gain);
}

// ─── Behavior: LevyFlight ────────────────────────────────────────
// Random walk with heavy-tailed step magnitudes. Most steps are
// small; occasionally a step is dramatic. Step magnitude sampled
// from inverse power-law on uniform.
//
// Aesthetic: searching, foraging. Bursts of motion punctuating
// quiet drift. Useful for "explorer" populations or insects.
fn behavior_levy_flight(agent_in: AgentState) -> AgentState {
    var a = agent_in;
    let dt        = signal.dt;

    let b = agent_behaviors[9u];
    let tier = min(a.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let g = agent_tier_gains[tier];

    let s = step_trigger(b.step_rate);
    if (s.fired) {
        let theta = hash_property(a.seed, 7500u + s.step_idx) * 6.28318530718;
        // Power-law magnitude. uniform in (0,1] (clamp to avoid /0).
        let u = max(0.05, hash_property(a.seed, 7600u + s.step_idx));
        // alpha = 1.5 → moderate tail (1=Cauchy-flat, 2=Gaussian-like).
        let alpha = 1.5;
        let magnitude_factor = pow(1.0 / u, 1.0 / alpha);
        // Cap to keep extreme jumps bounded.
        let capped = min(magnitude_factor, 8.0);
        let impulse = b.step_size * g.step_gain * capped;
        a.vel_x += cos(theta) * impulse;
        a.vel_z += sin(theta) * impulse;
    }

    return agent_post_step(a, b.drag, b.speed_cap, g.speed_gain);
}

// ═══ KERNELS ═════════════════════════════════════════════════════
//
// Two compute kernels share the agent state buffer:
//   update_player_agent  — 0D (1 thread, the possessed slot only).
//                          Walker policy + portal trigger + tilt.
//                          Compile cost contained by isolation.
//   update_other_agents  — 1D (32 threads, one per slot).
//                          Algorithmic behaviors + eviction.
//                          Skips the possessed slot internally.
// Order matters: dispatch player BEFORE other_agents so the player's
// updated position is visible to neighbor-sampling behaviors in the
// same frame.

// ─── Compute kernels ─────────────────────────────────────────────
//
// The agent kernel is split in two for FXC compile-time reasons.
// The original unified kernel placed behavior_player_controlled
// (heavy: walker policy, step-climb, tilt, full contributor chain)
// and behavior_random_walk (light: single agent-policy ground snap)
// in a single switch statement. FXC inlines both branch bodies for
// every one of 32 dispatched threads, producing a pipeline compile
// that landed at 48s. Adding more algorithmic behaviors would
// compound the cost.
//
// Split shape:
//   update_player_agent   — 1 thread. Only the possessed slot, only
//                           behavior_player_controlled. The walker
//                           policy is compiled once, for one slot.
//   update_other_agents   — 32 threads. All non-possessed slots,
//                           behavior switch for algorithmic behaviors.
//                           The walker policy is NOT inlined here.
//
// Dispatch order: player first, then others. THE POINT is
// the eviction reference for this frame's other agents — the
// player's updated position when the pawn hosts, the camera when it
// hosts. Matches the semantic "the point moves, the world adjusts
// around it" (presence follows the point; the ratified rule).
//
// MAX_AGENTS = 32 — must stay in sync with Dim::MAX_AGENTS.
//
// Eviction radius = THE VEIL CHAIN's EXIST ring (350 = the pregen edge;
// V1 fixed — was 360, overshooting patch residency by 10). Agents share
// the floaters' lifecycle: they exist anywhere in the loaded world out
// to the patch pre-gen edge, and evict AT it — always past the fog wall
// (chain law EXIST > FAR), so eviction is never visible.
// MUST match Dim::EXIST_RADIUS + AGENT_EVICTION_RADIUS in
// bodies/agents.hpp. No runtime upload — the WGSL needs it as a const.
const AGENT_EVICTION_RADIUS:    f32 = 350.0;
const AGENT_EVICTION_RADIUS_SQ: f32 = AGENT_EVICTION_RADIUS * AGENT_EVICTION_RADIUS;

// FLOATER_EVICTION_RADIUS — spheres and cubes that drift further than
// this from the point are evicted (set is_active=0 by their kernels).
// THE VEIL CHAIN FLAGGED FORK (ruled: "unify ≤ 350 unless a reason
// surfaces — flag if so"): a reason stands, documented below — floaters
// SPAWN out to the 350 pregen edge and need eviction headroom past it
// or they evict-at-spawn. Floaters are SKY objects (they never stand on
// unresident ground) and 400 > FAR(275) keeps every eviction behind the
// fog wall, so the overshoot is invisible by construction. Grounded
// existence (agents) is unified at EXIST = 350.
//
// Headroom over spawn radius. Floaters can spawn anywhere out to the
// pre-gen edge at 350 units. Two ways a fresh floater can be over the
// eviction line on its first kernel frame if the radii are too close:
//
//   1. Commit runs N frames after the trigger fires, while the player
//      keeps moving. At ~14 units/sec, even a 1-frame commit lag costs
//      ~0.25 units of headroom; busy-queue lags eat several units.
//   2. Sphere spawn places pos at `anchor + (orbit_radius, 0, 0)`,
//      which can put pos up to ~12 units further from the pawn than
//      anchor. A sphere spawned at 350-unit anchor distance can land
//      at 362-unit pos distance immediately.
//
// 400 = 350 spawn radius + 50 headroom covers both cases with margin.
// Earlier value of 360 (only +10 over spawn radius) caused near-100%
// eviction-at-spawn while the player was moving.
//
// GPU-only: no C++ constant mirrors this today. The CPU learns of
// kernel evictions through the is_active readback, not a duplicated
// radius (cartridge.hpp names this constant only in comments).
const FLOATER_EVICTION_RADIUS:    f32 = 400.0;
const FLOATER_EVICTION_RADIUS_SQ: f32 = FLOATER_EVICTION_RADIUS * FLOATER_EVICTION_RADIUS;

// POINT_BUBBLE_RADIUS — the point's bounded awareness (v3 §11; the
// bubble's first field). Today its one sensor is the portal's
// vertical gate in camera-host: an arch fires only within this many
// units of the point's altitude. MUST match POINT_BUBBLE_RADIUS in
// contracts/point.hpp (compile-time const, the eviction-radius
// pattern — no runtime upload).
const POINT_BUBBLE_RADIUS: f32 = 20.0;

// ─── Player kernel ───────────────────────────────────────────────
// Single thread, runs once per frame on config.possessed_slot.
// Contains the full walker policy (pawn_ground_resolve,
// terrain_normal_at, portal trigger).
@compute @workgroup_size(1)
fn update_player_agent() {
    if (!dynamics_0d_active()) { return; }

    let slot = config.possessed_slot;
    if (slot >= 32u) { return; }

    var agent = agent_state[slot];
    if (agent.is_active == 0u) { return; }

    // The player is always behavior 0 (PlayerControlled). Other
    // behavior ids in the possessed slot are a bug — treat as no-op.
    if (agent.behavior_id == 0u) {
        agent = behavior_player_controlled(agent);
    }

    // The player is never evicted — their slot is the reference
    // frame for eviction, not subject to it.
    agent_state[slot] = agent;
}

// ─── Other-agents kernel ─────────────────────────────────────────
// 32 threads, one per slot. Skips the possessed slot (handled by
// update_player_agent). Runs algorithmic behaviors only — the heavy
// walker-policy path never inlines here.
@compute @workgroup_size(32)
fn update_other_agents(@builtin(global_invocation_id) gid: vec3<u32>) {
    if (!dynamics_0d_active()) { return; }

    let slot = gid.x;
    if (slot >= 32u) { return; }
    if (slot == config.possessed_slot) { return; }   // handled separately

    var agent = agent_state[slot];
    if (agent.is_active == 0u) { return; }

    switch agent.behavior_id {
        // Behavior 0 (PlayerControlled) should never appear in a
        // non-possessed slot. If it does (stale state, bug), treat
        // as no-op rather than run the heavy walker path from the
        // wrong kernel.
        case 0u: { /* no-op */ }
        case 1u: { agent = behavior_random_walk(agent); }
        case 2u: { agent = behavior_biased_walk(agent); }
        case 3u: { agent = behavior_wanderer(agent); }
        case 4u: { agent = behavior_home_seeker(agent); }
        case 5u: { agent = behavior_slow_patrol(agent); }
        case 6u: { agent = behavior_pursuit(agent); }
        case 7u: { agent = behavior_flee(agent); }
        case 8u: { agent = behavior_flock2d(agent); }
        case 9u: { agent = behavior_levy_flight(agent); }
        default: { /* unknown behavior — no-op */ }
    }

    // Point-centered eviction (was the possessed slot).
    // Non-player agents that wander too far from THE POINT are
    // deactivated; the CPU readback path detects them on the next
    // frame and respawns fresh agents in a disk around the point.
    // Pawn-host identical (the point IS the possessed slot's pos
    // there); in free-fly the population lives under the camera.
    let pp = point_pos();
    let dx = agent.pos_x - pp.x;
    let dz = agent.pos_z - pp.z;
    if (dx * dx + dz * dz > AGENT_EVICTION_RADIUS_SQ) {
        agent.is_active = 0u;
    }

    agent_state[slot] = agent;
}

@compute @workgroup_size(1)
fn update_camera() {
    if (!dynamics_0d_active()) { return; }

    var camera = camera_state;

    // ─── THE CAMERA HOSTS THE POINT (free-fly) ───────────────────
    // The point is hosted here: input moves it and the camera
    // coincides with it (the point's permanent witness). One intent
    // channel: W/A/S/D author signal.move in this mode (the pawn's
    // input coupling is unrouted — the body idles). TERRAIN RULE =
    // NONE: every clamp below is skipped (clips freely — the
    // revision camera). The kite path below is byte-untouched when
    // the pawn hosts.
    if (point_camera_hosted()) {
        camera.azimuth += signal.look_az_delta;
        camera.elevation = clamp(camera.elevation + signal.look_el_delta,
            FPV_MIN_ELEVATION, FPV_MAX_ELEVATION);

        let cos_el = cos(camera.elevation);
        let sin_el = sin(camera.elevation);
        let cos_az = cos(camera.azimuth);
        let sin_az = sin(camera.azimuth);
        // The look frame (build_view_projection_matrix's convention):
        // W/S ride the look direction; A/D strafe the ground plane.
        let fly_forward = vec3(-cos_el * sin_az, -sin_el, -cos_el * cos_az);
        let fly_right = vec3(cos_az, 0.0, -sin_az);
        let fly_up = cross(fly_right, fly_forward);

        let fly_speed = select(PAWN_SPEED, config.point_fly_speed, config.point_fly_speed > 0.0);
        camera.pos += (fly_forward * (-signal.move_z) + fly_right * signal.move_x) * fly_speed * signal.dt;
        // Pan translates the point in the view plane (rotate + pan —
        // the fly's mouse; the orbit's pan scale kept for feel).
        camera.pos += (fly_right * signal.pan_x_delta + fly_up * signal.pan_y_delta) * camera.distance * 0.5;

        camera_state = camera;
        return;
    }

    if (coupling_active(COUPLING_INPUT_ORBITS_CAMERA)) {
        camera.azimuth += signal.look_az_delta;

        let min_el = select(CAMERA_MIN_ELEVATION, FPV_MIN_ELEVATION, fpv_mode_active());
        let max_el = select(CAMERA_MAX_ELEVATION, FPV_MAX_ELEVATION, fpv_mode_active());
        camera.elevation = clamp(camera.elevation + signal.look_el_delta, min_el, max_el);

        if (!fpv_mode_active()) {
            camera = coupling_input_to_camera_pan(vec2(signal.pan_x_delta, signal.pan_y_delta), camera);
        }
    }

    if (coupling_active(COUPLING_INPUT_ZOOMS_CAMERA) && !fpv_mode_active()) {
        camera = coupling_input_to_camera_distance(signal.zoom_delta, camera);
    }

    // Damped aim point — third-person orbit tracks aim_point rather
    // than the possessed agent's raw position. The lerp time constant
    // is tuned so walking lag is imperceptible (~3cm at 15 u/s walking
    // speed) but a Caps Lock transfer over ~10 units takes about a
    // second to settle. FPV bypasses this — first-person view requires
    // the camera to be exactly on the pawn each frame.
    let pawn_pos = compute_pawn_pos();
    {
        let tau = 0.30;
        let alpha = 1.0 - exp(-signal.dt / tau);
        camera.aim_point = mix(camera.aim_point, pawn_pos, alpha);
    }

    if (fpv_mode_active()) {
        camera.pos = pawn_pos + vec3(0.0, FPV_EYE_HEIGHT, 0.0);
    } else if (coupling_active(COUPLING_PAWN_TO_CAMERA_TARGET)) {
        camera.pos = coupling_pawn_to_camera_target(camera.aim_point, camera);
    }

    // ─── Camera terrain clamp: never go underground ──────────────
    //
    // POLICY_WALKER_TILT (b3 — "camera = live minus suppression"): the
    // walker's world surface (static base + pyramids + GoL zones + terrain
    // waves + radial pulses) with the pawn-centered GoL suppression applied,
    // minus the pawn self-aura. Suppression is centered on the PAWN
    // (consumer_pos = pawn_pos) so it matches the pawn body's own GoL
    // suppression: pawn-local GoL — where the body already stands flat —
    // does NOT lift the camera, while world-anchored waves/pulses and GoL
    // away from the pawn still clear the camera so animated ridges don't clip
    // it. Was POLICY_FLYER, which read the RAW (un-suppressed) GoL and lifted
    // the camera as the pawn passed through a zone (the b3 symptom).
    // update_camera's pipeline carries the live-contributor Group 1
    // (computeTextureLayout) — see follow-up brief Part D.
    {
        let min_clearance = 1.5;  // minimum height above terrain
        let qi = QueryInputs(pawn_pos, signal.t_seconds);  // suppression centered on the pawn (matches the pawn body)
        let ground_at_cam = manifold_position(camera.pos, POLICY_WALKER_TILT, qi).y;
        camera.pos.y = max(camera.pos.y, ground_at_cam + min_clearance);
    }

    // ─── Indoor boundary clamp: stay within walls and below ceiling ──
    {
        let bmin = config.world_bound_min;
        let bmax = config.world_bound_max;
        let has_bounds = (bmin.x != 0.0 || bmin.y != 0.0 || bmax.x != 0.0 || bmax.y != 0.0);
        if (has_bounds) {
            let wall_margin = 2.0;       // don't let camera press into walls
            camera.pos.x = clamp(camera.pos.x, bmin.x + wall_margin, bmax.x - wall_margin);
            camera.pos.z = clamp(camera.pos.z, bmin.y + wall_margin, bmax.y - wall_margin);
        }
        // Ceiling clamp (works for any mood with a ceiling_height > 0)
        if (config.ceiling_height > 0.0) {
            let ceiling_margin = 3.0;
            camera.pos.y = min(camera.pos.y, config.ceiling_height - ceiling_margin);
        }
    }

    camera_state = camera;
}

@compute @workgroup_size(1)
fn update_sphere() {
    if (!dynamics_0d_active()) { return; }

    let dt = signal.dt;
    let point_xz = point_pos().xz;

    // Update sphere slots only (orbital motion)
    for (var slot = 0u; slot < SPHERE_SLOT_COUNT; slot++) {
        var fe = floating_entities.entities[slot];
        if (fe.is_active == 0u) { continue; }

        // Lifecycle: point-distance eviction (was the pawn —
        // floaters follow the point, Jean's ruling). A sphere stays
        // alive within FLOATER_EVICTION_RADIUS of THE POINT.
        // Patch eviction no longer touches floaters (commit path skips
        // entity_refs for sphere/cube), so this is the sole death path.
        let to_point = fe.pos.xz - point_xz;
        if (dot(to_point, to_point) > FLOATER_EVICTION_RADIUS_SQ) {
            floating_entities.entities[slot].is_active = 0u;
            continue;
        }

        if (!sphere_frozen()) {
            fe.t = fe.t + dt;

            // Orbit: PGA motor around anchor
            let updated = compose_sphere_from_orbit_pga(fe.t, fe);
            fe.pos = updated.pos;
            fe.orientation = updated.orientation;

            floating_entities.entities[slot] = fe;
        }

        if (signal_active() && coupling_active(COUPLING_POLYPHONY_TO_SPHERE_COLOR)) {
            // DRIVERLESS (M1-C): raw signal.stats[0] substituted with the
            // neutral 0.0 — color rests at base_color.
            floating_entities.entities[slot].color = coupling_signal_polyphony_to_sphere_color(
                0.0,
                floating_entities.entities[slot].color,
                floating_entities.entities[slot].base_color,
                dt
            );
        }
    }

    // RAYMARCH/SDF EXCAVATION: the terrain-tint dead store removed. This
    // whole nearest-sphere search wrote only terrain_state.tint (the dead
    // TerrainState buffer, read by nobody); its locals fed nothing live.
    // update_sphere's live work (eviction + orbital motion + sphere color,
    // which write floating_entities) is above and untouched.
}

// ─── Cube behavior registry ─────────────────────────────────────
//
// Each behavior is a small force function that returns a vec3 added
// into the drift integrator each frame. Behaviors compose with the
// existing analytical home (anchor.xz + ground + bob); they never
// touch home directly. The spring in update_cube pulls drift toward
// zero, so forces have to be sustained for the cube to displace —
// the steady-state offset for constant force F is F / spring_stiffness.
//
// behavior_id is per-slot (FloatingEntityState field, set at spawn);
// behavior_phase is a per-slot u32 hash used by behaviors that need
// decorrelated sampling. config.floater_coordination is the system-
// wide synchrony knob in [0,1].
//
// Authoring note: keep force magnitudes small enough that the
// integrator stays stable (∼10× spring_stiffness is the noticeable
// limit; beyond that drift overshoots before damping catches it).

const CUBE_BEHAVIOR_STATIONARY: u32 = 0u;
const CUBE_BEHAVIOR_CURLFIELD:  u32 = 1u;
const CUBE_BEHAVIOR_PHASEWAVE:  u32 = 2u;

// MUST match bodies/cube_behaviors.hpp::CUBE_BEHAVIOR_COUNT
// (mirrors the agents pattern at AGENT_BEHAVIOR_COUNT_WGSL above).
const CUBE_BEHAVIOR_COUNT_WGSL: u32 = 3u;

// ─ Force: Stationary ─────────────────────────────────────────────
// No-op. Drift sits at zero, pos == home, identical to pre-Phase-3
// hover-bob visual. Default for every spawn.
fn cube_force_stationary() -> vec3<f32> {
    return vec3<f32>(0.0, 0.0, 0.0);
}

// ─ Force: CurlField ──────────────────────────────────────────────
// Velocity sampled from a 2D curl-noise field: organic XZ drift, no
// neighbor lookups. Coordination knob lerps between high spatial
// frequency (every cube samples a different point — individual drift)
// and low spatial frequency (neighbors share samples — flock-coherent
// drift). The curl of a scalar sin·cos field is analytical, so we
// avoid finite-difference noise sampling and stay cheap.
fn cube_force_curlfield(rest_xz: vec2<f32>, t: f32, coordination: f32) -> vec3<f32> {
    let freq_high  = 0.040;   // ~150-unit period — neighbors decorrelate
    let freq_low   = 0.005;   // ~1250-unit period — neighbors coherent
    let amplitude  = 12.0;    // force magnitude (10× spring) for visible drift
    let time_scale = 0.25;    // 1/s — slow evolution so motion feels organic

    // Lerp frequency by coordination. At 0 the population looks like
    // independent drifters; at 1 it looks like a coherent eddy field.
    let k = mix(freq_high, freq_low, coordination);
    let phase_t = t * time_scale;

    // Two octaves of analytical curl, summed for richer structure.
    // Curl of sin(kx)·cos(kz) is (-k·sin(kx)·sin(kz), -k·cos(kx)·cos(kz)).
    let p1 = rest_xz * k + vec2<f32>(phase_t, phase_t * 0.7);
    let p2 = rest_xz * (k * 2.3) + vec2<f32>(phase_t * 1.3, phase_t * 0.4);

    let v1 = vec2<f32>(-sin(p1.x) * sin(p1.y), -cos(p1.x) * cos(p1.y));
    let v2 = vec2<f32>(-sin(p2.x) * sin(p2.y), -cos(p2.x) * cos(p2.y)) * 0.5;

    let v = (v1 + v2) * amplitude;
    return vec3<f32>(v.x, 0.0, v.y);
}

// ─ Force: PhaseWave ──────────────────────────────────────────────
// Vertical sinusoid whose phase is a function of position. At
// coordination=1, every cube uses k_x·rest.x + k_z·rest.z, producing
// a traveling wavefront across the population. At coordination=0,
// every cube uses its own behavior_phase, producing uncorrelated
// vertical bobbing. Drone-show primitive.
fn cube_force_phasewave(rest_xz: vec2<f32>, t: f32, behavior_phase: u32, coordination: f32) -> vec3<f32> {
    let k_x        = 0.020;   // wavefront spatial frequency in X
    let k_z        = 0.012;   // and Z (asymmetric so the wave isn't axis-aligned)
    let omega      = 1.5;     // 1/s — wave temporal frequency
    let amplitude  = 30.0;    // force magnitude — pushes cubes vertically

    // Convert per-slot u32 hash to [0, 2π) for individual phase.
    let phase_individual = f32(behavior_phase & 0xFFFFu) * (6.283185 / 65536.0);
    let phase_shared     = k_x * rest_xz.x + k_z * rest_xz.y;

    // Lerp the phase by coordination. At 1 the field reads as a
    // coherent traveling wave; at 0 each cube oscillates on its own.
    let phase = mix(phase_individual, phase_shared, coordination) + omega * t;

    return vec3<f32>(0.0, sin(phase) * amplitude, 0.0);
}

// ─ Dispatch ──────────────────────────────────────────────────────
// Switch by behavior_id. New behaviors land here as additional cases
// alongside their authoring registry rows in bodies/cube_behaviors.hpp.
fn cube_behavior_force(fe: FloatingEntityState, t: f32, point_xz: vec2<f32>, coordination: f32) -> vec3<f32> {
    let rest_xz = vec2<f32>(fe.anchor.x, fe.anchor.z);
    switch (fe.behavior_id) {
        case 1u: { return cube_force_curlfield(rest_xz, t, coordination); }
        case 2u: { return cube_force_phasewave(rest_xz, t, fe.behavior_phase, coordination); }
        default: { return cube_force_stationary(); }
    }
}

@compute @workgroup_size(1)
fn update_cube() {
    if (!dynamics_0d_active()) { return; }

    let dt = signal.dt;
    let point_xz = point_pos().xz;

    // Update cube slots — drift integrator on top of analytical home.
    //
    // Decomposition:
    //   home  = analytical rest position (anchor.xz, ground + orbit_height + bob)
    //   pos   = home + drift
    //
    // drift integrates spring-to-zero plus per-slot behavior forces.
    // Stationary baseline: behavior force is zero. With drift and
    // drift_vel starting at zero, the spring sees no error, the
    // integrator adds nothing, and pos == home every frame — exact
    // visual parity with the pre-substrate hover-bob. Future behaviors
    // (CurlField, PhaseWave, …) push drift around without touching
    // the analytical home.
    let cube_end = CUBE_SLOT_OFFSET + CUBE_SLOT_COUNT;
    for (var slot = CUBE_SLOT_OFFSET; slot < cube_end; slot++) {
        var fe = floating_entities.entities[slot];
        if (fe.is_active == 0u) { continue; }

        // Lifecycle: point-distance eviction (was the pawn —
        // floaters follow the point). Cube stays alive as long as its
        // current position (home + drift) is within range of THE
        // POINT. Patch eviction no longer touches cubes — see the
        // matching test in update_sphere for the lifecycle rationale.
        let to_point = fe.pos.xz - point_xz;
        if (dot(to_point, to_point) > FLOATER_EVICTION_RADIUS_SQ) {
            floating_entities.entities[slot].is_active = 0u;
            continue;
        }

        if (!sphere_frozen()) {
            fe.t = fe.t + dt;

            // ── Kite-release transition ───────────────────────────
            // If follow_pawn == 2u, the CPU requested kite-mode release:
            // freeze the cube's CURRENT world xz as the new anchor,
            // zero drift, and switch to anchor mode (follow_pawn = 0).
            // This guarantees the cube's visible position is preserved
            // exactly across the toggle — anchor-mode home re-derives
            // ground at the new anchor.xz, drift = 0, so pos = home,
            // and the cube stays where the player saw it.
            //
            // Done in the kernel because pos.xz = home.xz + drift.xz
            // and drift lives only on GPU. Doing it on CPU would need
            // either a readback (one-frame latency, race-prone) or a
            // drift estimate (imperfect; CurlField can drift cubes by
            // ~3 units which would visibly jump on toggle).
            //
            // After this fires, the rest of update_cube runs as anchor
            // mode this frame, on the new anchor. No special-casing
            // needed — the if/else above already picked the kite path
            // for home, but with drift zeroed and pos overridden the
            // result is consistent: home is computed at kite.xz this
            // frame, pos = home + 0 = home, identical to what an
            // anchor-mode evaluation at the same xz would give next
            // frame.
            if (fe.follow_pawn == 2u) {
                fe.anchor = vec3(fe.pos.x, 0.0, fe.pos.z);
                fe.drift = vec3(0.0);
                fe.drift_vel = vec3(0.0);
                fe.follow_pawn = 0u;
            }

            // ── Analytical home ───────────────────────────────────
            // Two modes:
            //   follow_pawn = 0 (default): home.xz = anchor.xz; home.y
            //     terrain-relative at home.xz.
            //   follow_pawn = 1 (kite mode): home.xz = POINT.xz +
            //     offset (was the pawn — the kite target is the
            //     point, Jean's ruling; the CPU offset capture in
            //     cube_behaviors.hpp moved in lock-step, so the F7
            //     toggle still preserves world position exactly).
            //     home.y still terrain-relative at home.xz.
            //
            // Y is *always* terrain-relative in both modes. This makes
            // F7 toggle preserve world position cleanly: at the moment
            // of toggle, the home.xz interpretation switches but the
            // ground query underneath gives the same answer (anchor.xz
            // == point.xz + offset.xz at the toggle moment by
            // construction, since offset is captured as cube.cx -
            // point.xz). Cubes feel like balloons leashed to the point
            // — float at orbit_height above whatever terrain they're
            // over, regardless of the point's current altitude.
            //
            // POLICY_FLYER — the ground query picks up radial pulses
            // and pawn aura, same as anchor mode.
            let bob_y = sin(fe.t * 6.283185 / max(fe.bob_period, 0.1)) * fe.bob_amplitude;
            var home: vec3<f32>;
            if (fe.follow_pawn != 0u) {
                let point_p = point_pos();
                let kite_xz = vec2(point_p.x + fe.pawn_offset.x,
                                   point_p.z + fe.pawn_offset.z);
                let kite_qi = QueryInputs(vec3(kite_xz.x, 0.0, kite_xz.y),
                                          signal.t_seconds);
                let ground_k = manifold_position(vec3(kite_xz.x, 0.0, kite_xz.y), POLICY_FLYER, kite_qi).y;
                home = vec3(kite_xz.x, ground_k + fe.orbit_height + bob_y, kite_xz.y);
            } else {
                let home_xz = vec2(fe.anchor.x, fe.anchor.z);
                let qi = QueryInputs(fe.anchor, signal.t_seconds);
                let ground_a = manifold_position(vec3(home_xz.x, 0.0, home_xz.y), POLICY_FLYER, qi).y;
                home = vec3(fe.anchor.x, ground_a + fe.orbit_height + bob_y, fe.anchor.z);
            }

            // ── Drift integrator ──────────────────────────────────
            // Spring pulls drift toward zero (not pos toward home), so
            // a stationary cube with zero drift stays exactly at home
            // regardless of stiffness/drag tuning.
            //
            // Behavior force comes from the cube_behavior_force dispatch,
            // which switches on fe.behavior_id and reads the system-wide
            // coordination knob from config. Stationary returns zero,
            // making this a no-op for the default population.
            let behavior_force = cube_behavior_force(
                fe, signal.t_seconds, point_xz, config.floater_coordination);
            let spring_a = -fe.drift * fe.spring_stiffness;
            fe.drift_vel = fe.drift_vel + (spring_a + behavior_force) * dt;
            fe.drift_vel = fe.drift_vel * exp(-fe.drag * dt);
            fe.drift = fe.drift + fe.drift_vel * dt;

            // ── Terrain-clearance clamp on drift.y ────────────────
            // Query ground at the cube's *actual* xz, not at the home
            // anchor. This matters whenever a behavior moves the cube
            // in xz: CurlField can drift the cube onto terrain that's
            // higher than the home (a pyramid, a hill), and the clamp
            // needs to know about *that* ground, not the flat ground
            // at the anchor. PhaseWave doesn't move xz, so for it the
            // two queries return the same value.
            //
            // The kite-mode case is covered too — home.xz tracks the
            // pawn, drift.xz overlays on top, pos.xz is where the cube
            // actually is, ground is queried there.
            //
            // Clamp drift.y from below such that the cube's base (center
            // home.y + drift.y, minus its vertical half-extent) stays at
            // least CUBE_TERRAIN_CLEARANCE above local ground at pos.xz.
            // When the clamp engages we kill negative drift_vel.y
            // so the integrator doesn't accumulate downward energy
            // against the floor — when behavior force flips upward the
            // cube responds immediately.
            let pos_xz = vec2(home.x + fe.drift.x, home.z + fe.drift.z);
            let pos_qi = QueryInputs(vec3(pos_xz.x, 0.0, pos_xz.y), signal.t_seconds);
            let ground = manifold_position(vec3(pos_xz.x, 0.0, pos_xz.y), POLICY_FLYER, pos_qi).y;
            let cube_floor_y = ground + CUBE_TERRAIN_CLEARANCE + fe.body_radius * fe.aspect_y;
            let min_drift_y = cube_floor_y - home.y;
            if (fe.drift.y < min_drift_y) {
                fe.drift.y = min_drift_y;
                if (fe.drift_vel.y < 0.0) { fe.drift_vel.y = 0.0; }
            }

            // ── Compose final position ────────────────────────────
            fe.pos = home + fe.drift;

            // Spin around tilted Y axis (unchanged)
            let spin_angle = fe.t * fe.spin_speed;
            let axis = normalize(vec3(fe.spin_tilt_x, 1.0, fe.spin_tilt_z));
            let half_a = spin_angle * 0.5;
            fe.orientation = vec4(axis * sin(half_a), cos(half_a));

            floating_entities.entities[slot] = fe;
        }

        // Floater color — shares the sphere-color capability below.
        if (signal_active() && coupling_active(COUPLING_POLYPHONY_TO_SPHERE_COLOR)) {
            // DRIVERLESS (M1-C): raw signal.stats[0] substituted with the
            // neutral 0.0 — color rests at base_color.
            floating_entities.entities[slot].color = coupling_signal_polyphony_to_sphere_color(
                0.0,
                floating_entities.entities[slot].color,
                floating_entities.entities[slot].base_color,
                dt
            );
        }
    }
}

@compute @workgroup_size(1)
fn compute_vp() {
    // Build VP matrix from camera state (already updated by update_world)
    vp_data.m = build_view_projection_matrix(
        camera_state.pos,
        camera_state.azimuth,
        camera_state.elevation,
        signal.aspect_ratio
    );

    // Sun VP: kite coupling — the sun orbits THE POINT at fixed
    // offset (was the pawn; the 300-unit shadow box must cover
    // what the eye sees, so it follows the point's host — identical
    // when the pawn hosts, tracks the camera in free-fly).
    if (coupling_active(COUPLING_PAWN_TO_SUN_VP)) {
        vp_data.light_vp = coupling_pawn_to_sun_vp(
            point_pos(),
            config.sun_direction
        );
    }
}

// Fills the terrain index buffer with the standard quad triangulation pattern.
@compute @workgroup_size(8, 8)
fn generate_terrain_indices(@builtin(global_invocation_id) id: vec3<u32>) {
    if (id.x >= TERRAIN_MESH_N || id.y >= TERRAIN_MESH_N) { return; }

    let base = (id.y * TERRAIN_MESH_N + id.x) * 6u;
    let i00 = id.y * TERRAIN_MESH_STRIDE + id.x;
    let i10 = i00 + 1u;
    let i01 = i00 + TERRAIN_MESH_STRIDE;
    let i11 = i01 + 1u;

    // Same winding as the original CPU builder: (i00,i01,i10), (i10,i01,i11)
    terrain_mesh_indices[base + 0u] = i00;
    terrain_mesh_indices[base + 1u] = i01;
    terrain_mesh_indices[base + 2u] = i10;
    terrain_mesh_indices[base + 3u] = i10;
    terrain_mesh_indices[base + 4u] = i01;
    terrain_mesh_indices[base + 5u] = i11;
}

// --- Patch heightfield generation (two-pass, uses patchGen bind group layout)
//
// Pass 1: generate_patch_heights
//   Evaluates ground_formed_with_complexity() once per texel, stores the
//   height in the scratch buffer. (The stride-2 layout is kept; the +1
//   complexity slot is no longer written — no reader.)
//   This is the only expensive call — terrain waves + piers + pyramids.
//
// Pass 2: generate_patch_gradients
//   Reads height from 5 scratch neighbors.
//   Pure arithmetic: finite-difference gradients + textureStore (.w unused).
//   No terrain evaluation at all.
//
// The compute pass boundary between them provides the storage buffer
// barrier. Net effect: 1 terrain eval per texel total, not 6.

@compute @workgroup_size(16, 16)
fn generate_patch_heights(@builtin(global_invocation_id) id: vec3<u32>) {
    let res = patch_params.resolution;
    if (id.x >= res || id.y >= res) { return; }

    let res_f = f32(res);
    let uv = vec2<f32>(id.xy) / (res_f - 1.0);
    let world_xz = vec2<f32>(
        patch_params.origin.x + (uv.x - 0.5) * patch_params.extent,
        patch_params.origin.y + (uv.y - 0.5) * patch_params.extent
    );

    let hc = ground_formed_with_complexity(world_xz);
    let base = (id.y * res + id.x) * 2u;
    patch_height_scratch[base]      = hc.x;   // height (stride-2 layout kept; the
    // +1 complexity slot is no longer written — no reader)
}

// Workgroup shared tile: 20×20 heights (16×16 interior + 2-texel halo for 3-point edge stencil)
var<workgroup> sh_height: array<f32, 400>;

@compute @workgroup_size(16, 16)
fn generate_patch_gradients(
    @builtin(global_invocation_id) id: vec3<u32>,
    @builtin(local_invocation_id) lid: vec3<u32>,
    @builtin(workgroup_id) wid: vec3<u32>
) {
    let res = patch_params.resolution;
    let res_i = i32(res);

    // ── Cooperative tile load: 20×20 from global scratch ────────────
    // 256 threads load 400 cells via stride. Halo cells outside the
    // 256×256 grid clamp to boundary (safe: edge stencils only read
    // inward from the boundary, never into clamped halo).
    let thread_id = lid.y * 16u + lid.x;
    let tile_origin_x = i32(wid.x * 16u) - 2;
    let tile_origin_y = i32(wid.y * 16u) - 2;

    for (var t = thread_id; t < 400u; t += 256u) {
        let tx = i32(t % 20u);
        let ty = i32(t / 20u);
        let gx = clamp(tile_origin_x + tx, 0, res_i - 1);
        let gy = clamp(tile_origin_y + ty, 0, res_i - 1);
        sh_height[t] = patch_height_scratch[(u32(gy) * res + u32(gx)) * 2u];
    }
    workgroupBarrier();

    // ── Bounds check AFTER barrier (all threads must participate in load) ─
    if (id.x >= res || id.y >= res) { return; }

    // ── Read center height from shared (complexity readback REMOVED) ─
    let cx = lid.x + 2u;
    let cy = lid.y + 2u;
    let height = sh_height[cy * 20u + cx];

    let texel = vec2<i32>(id.xy);
    let layer = i32(patch_params.layer);
    let res_f = f32(res);
    let eps = patch_params.extent / (res_f - 1.0);

    let ix = id.x;
    let iy = id.y;
    let max_i = res - 1u;

    // Gradient computation: central difference in interior,
    // 3-point one-sided stencil at edges for matching O(eps²) accuracy.
    //   Forward:  (-3h[0] + 4h[1] - h[2]) / (2*eps)
    //   Backward: ( 3h[N] - 4h[N-1] + h[N-2]) / (2*eps)

    // ── Gradient X: all reads from shared tile ──────────────────────
    var grad_x: f32;
    if (ix == 0u) {
        let h0 = height;
        let h1 = sh_height[cy * 20u + cx + 1u];
        let h2 = sh_height[cy * 20u + cx + 2u];
        grad_x = (-3.0 * h0 + 4.0 * h1 - h2) / (2.0 * eps);
    } else if (ix == max_i) {
        let h0 = height;
        let h1 = sh_height[cy * 20u + cx - 1u];
        let h2 = sh_height[cy * 20u + cx - 2u];
        grad_x = (3.0 * h0 - 4.0 * h1 + h2) / (2.0 * eps);
    } else {
        let h_px = sh_height[cy * 20u + cx + 1u];
        let h_mx = sh_height[cy * 20u + cx - 1u];
        grad_x = (h_px - h_mx) / (2.0 * eps);
    }

    // ── Gradient Z: all reads from shared tile ──────────────────────
    var grad_z: f32;
    if (iy == 0u) {
        let h0 = height;
        let h1 = sh_height[(cy + 1u) * 20u + cx];
        let h2 = sh_height[(cy + 2u) * 20u + cx];
        grad_z = (-3.0 * h0 + 4.0 * h1 - h2) / (2.0 * eps);
    } else if (iy == max_i) {
        let h0 = height;
        let h1 = sh_height[(cy - 1u) * 20u + cx];
        let h2 = sh_height[(cy - 2u) * 20u + cx];
        grad_z = (3.0 * h0 - 4.0 * h1 + h2) / (2.0 * eps);
    } else {
        let h_pz = sh_height[(cy + 1u) * 20u + cx];
        let h_mz = sh_height[(cy - 1u) * 20u + cx];
        grad_z = (h_pz - h_mz) / (2.0 * eps);
    }

    // The .w channel is unused (was LATENT[complexity], removed by the husk
    // sweep — no consumer ever read it; palette calls read the pinned
    // PALETTE_COMPLEXITY, TERRAIN_LOOKS ROW 3).
    textureStore(patch_heightfield_array_write, texel, layer, vec4(height, grad_x, grad_z, 0.0));
}

// --- Patch cell color generation (uses patchGen bind group layout)
// CELLIDENTITY — Phase 1 contract (charter C2, ratified + amended).
// FIELD's complete answer for one cell: vocabulary, rolls, continuous
// door weights, region anatomy. PIGMENT realizes color from this and
// VOICE only. Receptivity retired (ruling 3). chess_eff/mono_eff ride
// so future cascade-glide never reshapes the struct (ruling 3).
struct CellIdentity {
    tier: u32,               // 0 full · 1 tint · 2 BW · 3 chess-BW · 4 chess-color
    // (Ratified order, ruling 3 — the field SET and order are law.
    //  Fields marked "authority-internal since P2" are UNFILLED: the
    //  pigment payload derives inside discrete_cell_color_at_tier
    //  since Phase 2 D2.1. Slimming the shape is a Phase 4 ruling.)
    parity: u32,             // (gx+gz)&1 — authority-internal since P2
    cell_roll: f32,          // prop 900
    sparse_roll: f32,        // prop 910
    bw_roll: f32,            // prop 830 — authority-internal since P2
    color_noise: vec3<f32>,  // props 840–842 — authority-internal since P2
    chess_eff: f32,          // tendency+jitter (the cascade's raw input)
    mono_eff: f32,           // mono+jitter
    blend_t: f32,            // door: smoothstep(.55,.75,mode)
    scatter_survival: f32,   // door: smoothstep(.35,.75,mode)
    sparse_survival: f32,    // door: smoothstep(thr,thr+.35,sparse)
    style: f32,
    in_mode_zone: f32,       // door: carried CONTINUOUS (fade form), not bool
    region_mean: vec3<f32>,      // authority-internal since P2
    region_variance: f32,        // authority-internal since P2
    region_wander: vec3<f32>,    // authority-internal since P2
    chess_color_a: vec3<f32>,    // authority-internal since P2
    chess_color_b: vec3<f32>,    // authority-internal since P2
    smooth_color: vec3<f32>,
    archetype: u32,
    mode: f32,
    sparse: f32,
}

// DOOR_FADE — ruling 2i (charter C2). The flip→glide law: a binary
// roll comparison becomes a linear band of width 2W around the roll.
// W = 0 reproduces the step EXACTLY (behavior-identical); W > 0 makes
// a moving bias dissolve cells across a soft front instead of popping
// them (SEAMLESSNESS in time). One dial per door, ROW 5.
fn door_fade(roll: f32, survival: f32, w: f32) -> f32 {
    return clamp((survival - roll) / max(2.0 * w, 1e-6) + 0.5, 0.0, 1.0);
}

// The four doors — ONE derivation, bake and live alike.
// x blend_t · y scatter_survival · z sparse_survival · w in_mode_zone.
// MONOTONE AXIS (charter C8): the zone gate reads mode_rest.
fn door_values(biased_mode: f32, mode_rest: f32, sparse: f32, sparse_bias: f32) -> vec4<f32> {
    let blend_t = smoothstep(MODE_BLEND_EDGE_LO, MODE_BLEND_EDGE_HI, biased_mode);
    let scatter_survival = smoothstep(MODE_SCATTER_FLOOR_EDGE, MODE_SCATTER_CORE_EDGE, biased_mode);
    let sparse_threshold = max(SPARSE_SURVIVAL_THRESHOLD - sparse_bias, 0.0);
    let sparse_survival = smoothstep(sparse_threshold, sparse_threshold + SPARSE_SURVIVAL_WINDOW, sparse);
    let in_mode_zone = door_fade(MODE_SCATTER_FLOOR_EDGE, mode_rest, DOOR_FADE_W_ZONE);
    return vec4(blend_t, scatter_survival, sparse_survival, in_mode_zone);
}

// (region_wander_raw — DELETED, Phase 2 D2.1: its only consumer was
//  the identity's region_wander fill, which fed the deleted resolver.
//  The S2 wander has ONE home now: checker_region_median derives it
//  internally — seed 20, props 601–603, law-marked, unchanged.)

// Stage 1: Evaluate all spatial fields at a cell's world position.
// Pure function — reads only spatial field lattices, no buffers.
fn evaluate_cell_fields(
    world_xz: vec2<f32>,
    cell_gx: i32,
    cell_gz: i32,
    cell_seed: u32,
    mode_bias: f32,     // live path passes config biases; the bake passes 0 —
    sparse_bias: f32    //   bias is a FIELD input now (charter Phase 1), not a composite arg
) -> CellIdentity {
    var id: CellIdentity;

    // Palette → smooth base color
    let palette_w = palette_field_at(world_xz);
    id.smooth_color = palette_color_smooth(palette_w, PALETTE_COMPLEXITY);

    // Spatial fields
    id.mode = mode_field_at(world_xz);
    id.style = transition_style_at(world_xz);
    id.sparse = sparse_field_at(world_xz);

    // Per-cell DOOR rolls (deterministic from seed). The PIGMENT rolls —
    // parity, bw_roll (830), color_noise (840–842) — derive inside
    // discrete_cell_color_at_tier since Phase 2 (D2.1): the identity
    // stopped being the pigment's data bus. The struct fields remain
    // per the ratified shape (ruling 3), unfilled; slimming the shape
    // is a Phase 4 ruling.
    id.cell_roll = hash_property(cell_seed, 900u);
    id.sparse_roll = hash_property(cell_seed, 910u);

    // Archetype from tile grid (nearest-cell lookup, not interpolated)
    let cell_extent = tile_grid.cell_extent;
    let tile_gx = i32(floor(world_xz.x / cell_extent));
    let tile_gz = i32(floor(world_xz.y / cell_extent));
    let tile = tile_grid_lookup(tile_gx, tile_gz);
    id.archetype = tile.archetype;

    // (Terrain-mode coupling shift RETIRED here — Phase 1, ruling 6.
    //  The magnitude sat at 0.0: the shift was always zero. id.mode is
    //  the mode field verbatim.)

    // (Region anatomy fills — region_mean/variance/wander — moved
    //  inside the pigment authority, Phase 2 D2.1: only the tinted +
    //  full-color tiers pay for them now.)

    // ── The tier cascade (charter C3 order; first gate wins) ─────
    // chess_eff carries the per-cell jitter; the colorful sub-cut reads
    // RAW tendency (today's law, preserved exactly). mono_eff likewise.
    // (chess_color_a/b fills moved inside the pigment authority, D2.1 —
    //  the FIELD needs only the tendency.)
    let chess = chess_field_at(world_xz);
    id.chess_eff = chess.tendency + (hash_property(cell_seed, 815u) - 0.5) * 0.03;
    id.mono_eff = discrete_mono_at(world_xz) + (hash_property(cell_seed, 820u) - 0.5) * 0.15;
    if (id.chess_eff > CHESS_TENDENCY_CUT) {
        id.tier = select(3u, 4u, chess.tendency > CHESS_COLORFUL_CUT);
    } else if (id.mono_eff > MONO_BW_CUT) {
        id.tier = 2u;
    } else if (id.mono_eff > MONO_TINT_CUT) {
        id.tier = 1u;
    } else {
        id.tier = 0u;
    }

    // Doors — bias upstream of the composite; zone gate reads the
    // REST field (MONOTONE AXIS, charter C8).
    let biased_mode = clamp(id.mode + mode_bias, 0.0, 1.0);
    let doors = door_values(biased_mode, id.mode, id.sparse, sparse_bias);
    id.blend_t = doors.x;
    id.scatter_survival = doors.y;
    id.sparse_survival = doors.z;
    id.in_mode_zone = doors.w;

    return id;
}

// Stage 2: Composite final terrain color from evaluated field state.
// The composite — Phase 1 (charter): pure arithmetic over the identity's
// precomputed door values. The doors were derived in evaluate_cell_fields
// via door_values (the ONE derivation since Commit C retired the LUT
// reconstruction) — bias is a FIELD input now.
// (Edges + cuts live at TERRAIN_LOOKS ROW 5, §2.2, read by door_values.)
// THE DISCRETE-VISIBILITY DOORS (UNIFIED_GROUND_1 U5) — the ONE
// derivation of the door math (Ruling 2i fade bands + the sparse
// product), extracted verbatim so the composite and the vocabulary
// mask read the SAME body. Bit-exact: the composite's own mix chain
// below is untouched (the U6 rest gate is bitwise).
struct DiscreteDoors {
    scatter_vis: f32,   // Ruling 2i fade band (W = 0 → the step)
    show_cell: f32,     // sparse fade × the zone fade's complement
}
fn discrete_visibility_doors(id: CellIdentity) -> DiscreteDoors {
    let scatter_vis = door_fade(id.cell_roll, id.scatter_survival, DOOR_FADE_W_SCATTER);
    let sparse_vis = door_fade(id.sparse_roll, id.sparse_survival, DOOR_FADE_W_SPARSE);
    return DiscreteDoors(scatter_vis, sparse_vis * (1.0 - id.in_mode_zone));
}

// THE REST PREDICATE (UNIFIED_GROUND_1 U5): the discrete-visibility
// WEIGHT at rest (bias 0), as one scalar — algebraically the exact
// weight the composite's affine mix chain applies (mix-linearity in
// the weight; s,d fixed). Consumers threshold it (step), so the
// float-order difference vs the composite's chain is immaterial
// HERE; the composite keeps its own chain for bitwise identity.
fn discrete_visibility_rest(cell_center: vec2<f32>, cell: vec2<i32>) -> f32 {
    let cell_seed = lattice_node_seed(config.world_seed, cell, 200u);
    let id = evaluate_cell_fields(cell_center, cell.x, cell.y, cell_seed, 0.0, 0.0);
    let d = discrete_visibility_doors(id);
    let mode_w = mix(id.blend_t, d.scatter_vis, id.style);
    return mix(mode_w, 1.0, d.show_cell);
}

fn composite_cell_color(id: CellIdentity, discrete_color: vec3<f32>) -> vec3<f32> {
    // The doors — the ONE derivation (discrete_visibility_doors above).
    let d = discrete_visibility_doors(id);

    // Blend style: smooth → discrete (gradual transition at mode boundary)
    let blend_color = mix(id.smooth_color, discrete_color, id.blend_t);

    // Scatter style: cell survives or is replaced by smooth background.
    let scatter_color = mix(id.smooth_color, discrete_color, d.scatter_vis);

    // Combine the two transition styles (blend vs scatter) via style field
    let mode_color = mix(blend_color, scatter_color, id.style);

    // Sparse scatter: isolated cells and small clusters outside mode zones
    // (CHECKER-TUNE A2 standing; Ruling 2i products — see the doors fn).
    return mix(mode_color, discrete_color, d.show_cell);
}

// (composite_cell_color_biased — the Phase-1 pass-through alias —
//  DELETED, Phase 2 D2.2: the twins are ONE body, composite_cell_color;
//  bias has been a FIELD input since Phase 1.)

// Target palette color from a continuous index [0,3].
// Integer values select a single palette; fractional values blend neighbors.
fn palette_target_color(palette_idx: f32, complexity: f32) -> vec3<f32> {
    let t = clamp(palette_idx, 0.0, 3.0);
    let lo = u32(floor(t));
    let hi = min(lo + 1u, 3u);
    let frac = t - floor(t);
    let color_lo = mix(config.palette_light[lo].rgb, config.palette_center[lo].rgb, complexity);
    let color_hi = mix(config.palette_light[hi].rgb, config.palette_center[hi].rgb, complexity);
    return mix(color_lo, color_hi, frac);
}

// (animated_cell_color_lut — the LUT reconstruction body — RETIRED,
//  Commit C: the Phase-2 deletion note's prophecy fulfilled ("a
//  revival is a five-line wrapper"). The cell_fields LUT died with
//  it; the name animated_cell_color returns, now THE one live
//  composite body.)

// THE ONE LIVE COMPOSITE BODY — the bake's evaluator at VOICE = now,
// sampled at the owned cell's center (charter C8). Rest look catches
// up on regen.
fn animated_cell_color(world_center: vec2<f32>, cell: vec2<i32>) -> vec3<f32> {
    let cell_seed = lattice_node_seed(config.world_seed, cell, 200u);
    let id = evaluate_cell_fields(world_center, cell.x, cell.y, cell_seed,
                                  config.mode_color_shift, config.mode_checker_scatter);
    let dcol = discrete_cell_color_at_tier(world_center, cell.x, cell.y, cell_seed, id.tier,
                                           config.checker_resultant, config.checker_music_amount,
                                           config.checker_music_variance);
    let base = composite_cell_color(id, dcol);

    let drift = config.mode_palette_intensity;
    if (drift > 0.001) {
        var drifted = id;
        drifted.smooth_color = palette_target_color(config.mode_palette_target, PALETTE_COMPLEXITY);
        let tier = u32(round(config.mode_discrete_tier));
        let drifted_dcol = discrete_cell_color_at_tier(world_center, cell.x, cell.y, cell_seed, tier,
                                                       config.checker_resultant, config.checker_music_amount,
                                                       config.checker_music_variance);
        let drifted_color = composite_cell_color(drifted, drifted_dcol);
        return mix(base, drifted_color, drift * 0.95);
    }

    return base;
}

// Stage 3: Tag cell behavior mode from field state.
fn tag_cell_behavior(id: CellIdentity, world_xz: vec2<f32>) -> f32 {
    // Only cells in clearly discrete zones are eligible
    if (id.mode < GOL_ZONE_MODE_THRESHOLD) { return 0.0; }

    // Zone-level seed: all cells in the same mode lattice cell share this
    let zone_node = vec2<i32>(floor(world_xz / MODE_LATTICE_SPACING));
    let zone_seed = lattice_node_seed(patch_params.master_seed, zone_node, GOL_ZONE_SEED_BAND);

    // GoL activation roll (per-zone, not per-cell)
    let spawn_roll = hash_property(zone_seed, GOL_ZONE_PROP_SPAWN);
    if (spawn_roll >= GOL_ZONE_SPAWN_CHANCE) { return 0.0; }

    // Tier selection (cumulative weight from tier matrix)
    let tier_roll = hash_property(zone_seed, GOL_ZONE_PROP_TIER);
    var tier: u32 = GOL_TIER_COUNT - 1u;
    var cumul: f32 = 0.0;
    for (var t: u32 = 0u; t < GOL_TIER_COUNT; t++) {
        cumul += GOL_TIERS[t].weight;
        if (tier_roll < cumul) { tier = t; break; }
    }

    // Height factor roll (per-zone)
    let height_roll = hash_property(zone_seed, GOL_ZONE_PROP_HEIGHT);
    let height_enabled = height_roll < GOL_ZONE_HEIGHT_CHANCE;

    return pack_cell_tag(CELL_ANIM_GOL, tier, height_enabled);
}

@compute @workgroup_size(8, 8)
fn generate_patch_cells(@builtin(global_invocation_id) id: vec3<u32>) {
    let cell_n = PATCH_CELL_N;
    if (id.x >= cell_n || id.y >= cell_n) {
        return;
    }

    let texel = vec2<i32>(id.xy);
    let layer = i32(patch_params.layer);

    // Map cell to world-space center position
    let uv = (vec2<f32>(id.xy) + 0.5) / f32(cell_n);
    let world_xz = vec2<f32>(
        patch_params.origin.x + (uv.x - 0.5) * patch_params.extent,
        patch_params.origin.y + (uv.y - 0.5) * patch_params.extent
    );

    // THE ONE-ADDRESS LAW, inverted: write texel T colors the cell at
    // address patch_origin_address + T — the SAME derivation the FS
    // uses, run backward. The patch grid index comes from the min
    // corner (origin is the patch CENTER: (g + 0.5) * extent,
    // patch_system.hpp make_patch_params); round() is exact on the
    // aligned grid. world_xz above stays the cell-center FIELD sample
    // point — bit-identical to the pre-law bake.
    let patch_grid = vec2<i32>(round((patch_params.origin - 0.5 * patch_params.extent) / patch_params.extent));
    let addr = patch_grid * i32(cell_n) + vec2<i32>(id.xy);
    let cell_gx = addr.x;
    let cell_gz = addr.y;
    let cell_seed = lattice_node_seed(patch_params.master_seed, vec2(cell_gx, cell_gz), 200u);

    // Stage 1: Evaluate the cell's identity (bias 0 — the bake is unbiased).
    // (Named cell_id: `id` is this entry point's invocation builtin.)
    let cell_id = evaluate_cell_fields(world_xz, cell_gx, cell_gz, cell_seed, 0.0, 0.0);

    // Stage 2: Resolve + composite through THE SAME chain the live path
    // runs — bake = PIGMENT(FIELD, VOICE = rest) is literal code now
    // (Phase 2, D2.4): the identity literals ARE the rest. CHECKER-
    // REBUILD law preserved: amount 0 -> seed color; patches bake at
    // rest by construction; the live pull rides the FS gate only.
    let dcol = discrete_cell_color_at_tier(world_xz, cell_gx, cell_gz, cell_seed,
                                           cell_id.tier, vec3(0.0), 0.0, 0.0);
    let final_color = composite_cell_color(cell_id, dcol);

    // Stage 3: Behavior tag — packed into alpha channel
    // 0.0 = static (no animation), nonzero = animation mode + tier + flags
    let behavior_tag = tag_cell_behavior(cell_id, world_xz);

    // Store: RGB = fully composited color, A = behavior tag
    textureStore(patch_cell_color_array_write, texel, layer, vec4(final_color, behavior_tag));

    // (cell_fields LUT store RETIRED — Commit C: the live path
    //  evaluates the same fields analytically at the cell center. The
    //  bake's one artifact is the color+tag texture — the cache of
    //  this function at VOICE = rest.)
}


// §7.2 GOL ZONE COMPUTE — Zone-local Game of Life
// Two compute passes per frame (when zones are active):
const GOL_ZONE_STRIDE: u32 = 7168u;     // floats per zone (7 slots × 1024 cells)
const GOL_CELL_VISUAL: u32 = 0u;        // slot 0: height spring visual [0,1]
const GOL_CELL_VELOCITY: u32 = 1024u;   // slot 1: height spring velocity
const GOL_CELL_TARGET: u32 = 2048u;     // slot 2: current target (binary, Conway reads)
const GOL_CELL_NEXT: u32 = 3072u;       // slot 3: next target (binary, Conway writes)
const GOL_CELL_HEIGHT_FACTOR: u32 = 4096u;  // slot 4: per-cell height multiplier (persistent)
const GOL_CELL_COLOR_VISUAL: u32 = 5120u;   // slot 5: color spring visual [0,1]
const GOL_CELL_COLOR_VELOCITY: u32 = 6144u; // slot 6: color spring velocity

@compute @workgroup_size(8, 8, 1)
fn zone_gol_sync(@builtin(global_invocation_id) gid: vec3<u32>) {
    let zone_id = gid.z;
    if (zone_id >= zone_config.count) { return; }
    let z = zone_config.zones[zone_id];
    if (z.transition_fraction <= 0.0) { return; }
    let cell = gid.xy;
    if (cell.x >= z.grid_size || cell.y >= z.grid_size) { return; }

    let base = zone_id * GOL_ZONE_STRIDE;
    let idx = cell.y * z.grid_size + cell.x;
    // Copy next_target → target (double-buffer sync, works for both Conway and Pulse)
    zone_life[base + GOL_CELL_TARGET + idx] = zone_life[base + GOL_CELL_NEXT + idx];
}

@compute @workgroup_size(8, 8, 1)
fn zone_gol_evolve(@builtin(global_invocation_id) gid: vec3<u32>) {
    let zone_id = gid.z;
    if (zone_id >= zone_config.count) { return; }
    let z = zone_config.zones[zone_id];
    if (z.transition_fraction <= 0.0) { return; }
    let cell = gid.xy;
    if (cell.x >= z.grid_size || cell.y >= z.grid_size) { return; }

    let base = zone_id * GOL_ZONE_STRIDE;
    let idx = cell.y * z.grid_size + cell.x;
    let dt = zone_config.dt;

    // --- Read per-cell state
    var visual = zone_life[base + GOL_CELL_VISUAL + idx];
    var velocity = zone_life[base + GOL_CELL_VELOCITY + idx];
    let tgt = zone_life[base + GOL_CELL_TARGET + idx];

    // --- Tick: algorithm-specific target generation
    let should_tick = (zone_config.tick_mask & (1u << zone_id)) != 0u;

    if (z.algorithm == GOL_ALGORITHM_CONWAY) {
        // Conway: count neighbors, apply birth/survival rules
        if (should_tick) {
            var count: i32 = 0;
            let gs = i32(z.grid_size);
            for (var dy: i32 = -1; dy <= 1; dy++) {
                for (var dx: i32 = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) { continue; }
                    let nx = u32((i32(cell.x) + dx + gs) % gs);
                    let ny = u32((i32(cell.y) + dy + gs) % gs);
                    let ni = ny * z.grid_size + nx;
                    if (zone_life[base + GOL_CELL_TARGET + ni] > 0.5) { count++; }
                }
            }
            let next = coupling_gol_next_state(tgt > 0.5, count);
            zone_life[base + GOL_CELL_NEXT + idx] = next;
        }
    } else {
        // Pulse: per-cell on/off target on a sinusoidal schedule (no neighbor
        // rules). Binary like Conway, so cells plant on the ground / at full
        // height instead of hovering at mid-extension; the shared spring
        // smooths the transitions. Written every frame (deterministic from
        // t_beats), not tick-gated.
        let raw_target = pulse_cell_target(
            cell.x, cell.y,
            zone_config.t_beats,
            z.tick_period,
            z.phase_randomness,
            z.tempo_randomness
        );
        zone_life[base + GOL_CELL_NEXT + idx] = raw_target;
        // For Pulse, also copy directly to TARGET (no sync delay needed)
        zone_life[base + GOL_CELL_TARGET + idx] = raw_target;
    }

    // --- Analytical critically damped spring — HEIGHT
    let current_tgt = zone_life[base + GOL_CELL_TARGET + idx];
    let transition_time = max(z.transition_fraction * z.tick_period, 0.01);

    // Per-cell spring variance: each cell settles at a slightly different rate
    // spring_variance=0 → all uniform. =1 → ±50% transition speed.
    var omega = 3.0 / transition_time;
    if (z.spring_variance > 0.001) {
        let sv_hash = gol_cell_hash(cell.x + 53u, cell.y + 97u);
        let sv_jitter = 1.0 + (gol_cell_variation(sv_hash) - 0.5) * z.spring_variance;
        omega = omega / max(sv_jitter, 0.3);
    }
    let omega_dt = omega * dt;
    let e = exp(-omega_dt);
    let d = visual - current_tgt;

    let new_visual = current_tgt + (d * (1.0 + omega_dt) + velocity * dt) * e;
    let new_velocity = (velocity * (1.0 - omega_dt) - d * omega * omega * dt) * e;

    if (abs(new_visual - current_tgt) < 0.001 && abs(new_velocity) < 0.01) {
        visual = current_tgt;
        velocity = 0.0;
    } else {
        visual = apply_boundary(new_visual, z.boundary_mode);
        velocity = select(new_velocity, 0.0, new_visual < 0.0 || new_visual > 1.0);
    }

    // --- Analytical critically damped spring — COLOR
    var color_visual = zone_life[base + GOL_CELL_COLOR_VISUAL + idx];
    var color_velocity = zone_life[base + GOL_CELL_COLOR_VELOCITY + idx];

    let cd = color_visual - current_tgt;
    let new_cv = current_tgt + (cd * (1.0 + omega_dt) + color_velocity * dt) * e;
    let new_cvv = (color_velocity * (1.0 - omega_dt) - cd * omega * omega * dt) * e;

    if (abs(new_cv - current_tgt) < 0.001 && abs(new_cvv) < 0.01) {
        color_visual = current_tgt;
        color_velocity = 0.0;
    } else {
        color_visual = apply_boundary(new_cv, z.boundary_mode);
        color_velocity = select(new_cvv, 0.0, new_cv < 0.0 || new_cv > 1.0);
    }

    // --- Write back
    zone_life[base + GOL_CELL_VISUAL + idx] = visual;
    zone_life[base + GOL_CELL_VELOCITY + idx] = velocity;
    zone_life[base + GOL_CELL_COLOR_VISUAL + idx] = color_visual;
    zone_life[base + GOL_CELL_COLOR_VELOCITY + idx] = color_velocity;

    // Write to texture: R = height visual, G = color visual
    textureStore(zone_life_tex_write, cell, i32(zone_id), vec4(visual, color_visual, 0.0, 0.0));
}


// §7.3 GOL ZONE MESH GENERATION — Cell extrusion geometry







// (fn zone_emit_quad RETIRED — UNIFIED_GROUND_1 U4; A2-3 census)

// (fn zone_mesh_gen_cell RETIRED — UNIFIED_GROUND_1 U4; A2-3 census)

// (fn zone_gol_mesh_reset RETIRED — UNIFIED_GROUND_1 U4; A2-3 census)

// (fn zone_gol_mesh_gen RETIRED — UNIFIED_GROUND_1 U4; A2-3 census)

// --- Zone extrusion render shaders

struct ZoneExtrusionVarying {
    @builtin(position) clip_pos: vec4<f32>,
    @location(0) world_pos: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) cell_color: vec3<f32>,    // pre-computed in compute pass
}

// (fn zone_extrusion_vs RETIRED — UNIFIED_GROUND_1 U4; A2-3 census)

// (fn zone_extrusion_fs RETIRED — UNIFIED_GROUND_1 U4; A2-3 census)

// (fn shadow_zone_extrusion_vs RETIRED — UNIFIED_GROUND_1 U4; A2-3 census)


// ═══ §7.3b THE LIVE CARD (GROUND_CARD_1) ═══════════════════════════════
// The per-frame deformation field. The writer CALLS the existing
// evaluators at texel centers — one derivation, one new sampling
// site (campaign v2 §5). Rest ⇒ zeros ⇒ every consumer adds 0.
fn live_card_uv(world_xz: vec2<f32>) -> vec2<f32> {
    return (world_xz - live_card_origin()) / LIVE_CARD_EXTENT;
}
fn sample_live_card(world_xz: vec2<f32>) -> vec4<f32> {
    return textureSampleLevel(live_card_read, bilinear_sampler,
                              live_card_uv(world_xz), 0.0);
}
fn sample_live_card_gol(world_xz: vec2<f32>) -> f32 {
    // nearest + cell-snapped origin ⇒ cell-exact raw lift
    return textureSampleLevel(live_card_read, nearest_sampler,
                              live_card_uv(world_xz), 0.0).w;
}
// THE TWO-PASS WRITER (TRUEBAND_CONTACT_1 T1b — the bake's model at
// card size). Pass 1 evaluates the TRUE-BAND delta (the terrain's own
// waves: Σ bands of blend × Σnodes band_act (moving − frozen)) + pulses
// into the stride-2 scratch; pass 2 resolves gradients (the bake's
// cooperative-tile stencils) and stores vec4(h, gx, gz, gol).
// terrain_time ≤ 0 ⇒ zeros (rest bit-frozen). Waking anti-teleport is
// inherited: t_eff = 0 at the origin ⇒ moving ≡ frozen ⇒ a woken band
// grows out of the frozen shape.
@compute @workgroup_size(8, 8, 1)
fn write_live_card_heights(@builtin(global_invocation_id) gid: vec3<u32>) {
    if (gid.x >= LIVE_CARD_SIZE || gid.y >= LIVE_CARD_SIZE) { return; }
    let texel = LIVE_CARD_EXTENT / f32(LIVE_CARD_SIZE);
    let p = live_card_origin()
          + (vec2<f32>(gid.xy) + vec2(0.5)) * texel;
    var dh = 0.0;
    if (config.terrain_time > 0.0) {
        let af = terrain_activity_at(p, config.world_seed);
        for (var b = 0u; b < TERRAIN_BAND_COUNT; b++) {
            if (b == 4u) { continue; }   // the fine ripple stays bake-only —
                                         // the Nyquist ruling (campaign v2 §6)
            let blend = get_band_blend(b);
            if (blend <= 0.0) { continue; }   // −1 sentinel + 0
            let t_eff = config.terrain_time - get_band_phase_origin(b);
            dh += clamp(blend, 0.0, 1.0)
                * true_band_delta_contribution(p, config.world_seed,
                      t_eff, b, af.x, af.y);
        }
    }
    dh += contrib_radial_pulses_at(p, signal.t_seconds);
    let base = (gid.y * LIVE_CARD_SIZE + gid.x) * 2u;
    live_card_scratch[base]      = dh;
    live_card_scratch[base + 1u] = contrib_gol_zones_at(p);
}

// Workgroup shared tile: 20×20 heights (16×16 interior + 2-texel halo
// for the 3-point edge stencils) — the bake's pass-2 clone at res 512.
var<workgroup> sh_card_h: array<f32, 400>;

@compute @workgroup_size(16, 16)
fn write_live_card_resolve(
    @builtin(global_invocation_id) id: vec3<u32>,
    @builtin(local_invocation_id) lid: vec3<u32>,
    @builtin(workgroup_id) wid: vec3<u32>
) {
    let res = LIVE_CARD_SIZE;
    let res_i = i32(res);

    let thread_id = lid.y * 16u + lid.x;
    let tile_origin_x = i32(wid.x * 16u) - 2;
    let tile_origin_y = i32(wid.y * 16u) - 2;

    for (var t = thread_id; t < 400u; t += 256u) {
        let tx = i32(t % 20u);
        let ty = i32(t / 20u);
        let gx = clamp(tile_origin_x + tx, 0, res_i - 1);
        let gy = clamp(tile_origin_y + ty, 0, res_i - 1);
        sh_card_h[t] = live_card_scratch[(u32(gy) * res + u32(gx)) * 2u];
    }
    workgroupBarrier();

    if (id.x >= res || id.y >= res) { return; }

    let cx = lid.x + 2u;
    let cy = lid.y + 2u;
    let height = sh_card_h[cy * 20u + cx];

    // eps = texel-CENTER spacing (extent / res): the card maps texel
    // centers across the window, unlike the bake's corner-pinned
    // (res − 1) grid — the one mapping difference from the model.
    let eps = LIVE_CARD_EXTENT / f32(LIVE_CARD_SIZE);

    let ix = id.x;
    let iy = id.y;
    let max_i = res - 1u;

    var grad_x: f32;
    if (ix == 0u) {
        let h0 = height;
        let h1 = sh_card_h[cy * 20u + cx + 1u];
        let h2 = sh_card_h[cy * 20u + cx + 2u];
        grad_x = (-3.0 * h0 + 4.0 * h1 - h2) / (2.0 * eps);
    } else if (ix == max_i) {
        let h0 = height;
        let h1 = sh_card_h[cy * 20u + cx - 1u];
        let h2 = sh_card_h[cy * 20u + cx - 2u];
        grad_x = (3.0 * h0 - 4.0 * h1 + h2) / (2.0 * eps);
    } else {
        let h_px = sh_card_h[cy * 20u + cx + 1u];
        let h_mx = sh_card_h[cy * 20u + cx - 1u];
        grad_x = (h_px - h_mx) / (2.0 * eps);
    }

    var grad_z: f32;
    if (iy == 0u) {
        let h0 = height;
        let h1 = sh_card_h[(cy + 1u) * 20u + cx];
        let h2 = sh_card_h[(cy + 2u) * 20u + cx];
        grad_z = (-3.0 * h0 + 4.0 * h1 - h2) / (2.0 * eps);
    } else if (iy == max_i) {
        let h0 = height;
        let h1 = sh_card_h[(cy - 1u) * 20u + cx];
        let h2 = sh_card_h[(cy - 2u) * 20u + cx];
        grad_z = (3.0 * h0 - 4.0 * h1 + h2) / (2.0 * eps);
    } else {
        let h_pz = sh_card_h[(cy + 1u) * 20u + cx];
        let h_mz = sh_card_h[(cy - 1u) * 20u + cx];
        grad_z = (h_pz - h_mz) / (2.0 * eps);
    }

    let base = (id.y * res + id.x) * 2u;
    textureStore(live_card_write, vec2<i32>(id.xy),
                 vec4(height, grad_x, grad_z, live_card_scratch[base + 1u]));
}

// §7.4 PAWN AURA — Persistent terrain influence via toroidal spring grid
// Single dispatch over 64×64 toroidal grid. Each thread:
const AURA_BEAT_PERIOD: f32 = 2.0;

@compute @workgroup_size(8, 8, 1)
fn compute_pawn_aura(@builtin(global_invocation_id) gid: vec3<u32>) {
    let sx = i32(gid.x);
    let sz = i32(gid.y);
    let N = PAWN_AURA_N;
    if (sx >= N || sz >= N) { return; }

    let slot_idx = u32(sz * N + sx);
    var cell = pawn_aura_cells[slot_idx];

    let pawn_xz = compute_pawn_pos().xz;
    let cs = pawn_aura_cfg.cell_size;
    let radius = pawn_aura_cfg.influence_radius;
    let dt = pawn_aura_cfg.dt;
    let t_beats = pawn_aura_cfg.t_beats;

    // Which world cell near the pawn maps to this slot via toroidal hash?
    let pgx = i32(floor(pawn_xz.x / cs));
    let pgz = i32(floor(pawn_xz.y / cs));

    // Nearest cell to pawn satisfying gx ≡ sx (mod N), gz ≡ sz (mod N)
    let gx = sx + N * i32(round(f32(pgx - sx) / f32(N)));
    let gz = sz + N * i32(round(f32(pgz - sz) / f32(N)));

    let cell_center = (vec2(f32(gx), f32(gz)) + 0.5) * cs;
    let dist = distance(cell_center, pawn_xz);
    let stimulus = smoothstep(radius, 0.0, dist);

    let is_occupied = (cell.cell_gx != PAWN_AURA_EMPTY);
    let matches = (cell.cell_gx == gx && cell.cell_gz == gz);

    if (stimulus > 0.001) {
        // Cell is within pawn influence
        if (!is_occupied || matches || cell.intensity < 0.05) {
            // Allocate new or update existing
            if (!matches) {
                cell.cell_gx = gx;
                cell.cell_gz = gz;
                cell.velocity = 0.0;
                cell.color_osc = 0.0;
                cell.color_osc_vel = 0.0;
                if (!is_occupied) { cell.intensity = 0.0; }

                // Compute contextual color delta based on delta_mode
                if (pawn_aura_cfg.delta_mode == AURA_DELTA_RANDOM) {
                    let h = u32(gx) * 374761393u + u32(gz) * 668265263u;
                    let mag = pawn_aura_cfg.delta_magnitude;
                    cell.delta_r = (f32((h) & 0xFFFFu) / 32767.5 - 1.0) * mag;
                    cell.delta_g = (f32((h >> 8u) & 0xFFFFu) / 32767.5 - 1.0) * mag;
                    cell.delta_b = (f32((h >> 16u) & 0xFFFFu) / 32767.5 - 1.0) * mag;
                } else {
                    let terrain_color = gol_composite_cell_color(cell_center);
                    let tint = vec3(pawn_aura_cfg.tint_r, pawn_aura_cfg.tint_g, pawn_aura_cfg.tint_b);
                    cell.delta_r = tint.r - terrain_color.r;
                    cell.delta_g = tint.g - terrain_color.g;
                    cell.delta_b = tint.b - terrain_color.b;
                }
            }

            // Attack spring: critically damped toward stimulus
            let goal = stimulus;
            let d = cell.intensity - goal;
            let stiff = pawn_aura_cfg.attack_stiffness;
            let damp = pawn_aura_cfg.attack_damping;
            let spring_f = -stiff * d;
            let damp_f = -damp * sqrt(stiff) * cell.velocity;
            cell.velocity += (spring_f + damp_f) * dt;
            cell.intensity += cell.velocity * dt;
            cell.intensity = clamp(cell.intensity, 0.0, 1.0);

            // Directional height bias: updates every frame as pawn turns.
            // Center = peak (1.0). Forward = gentle ramp down. Behind = steep drop.
            // This makes the pawn the highest point with a leading ramp.
            if (dist > 0.5) {
                let to_cell = (cell_center - pawn_xz) / dist;
                let heading = compute_pawn_heading();
                let forward = vec2(sin(heading), cos(heading));
                let facing = dot(to_cell, forward);  // -1=behind, +1=in front
                // Forward cells: gentle ramp (0.6–0.85). Behind: steeper (0.2–0.5).
                let forward_factor = clamp(facing * 0.15 + 0.7, 0.2, 0.85);
                // Distance falloff: closer to center = closer to 1.0
                let dist_falloff = 1.0 - smoothstep(0.0, radius, dist);
                cell.height_delta = mix(forward_factor, 1.0, dist_falloff * dist_falloff);
            } else {
                // At pawn center: peak height exactly 1.0
                cell.height_delta = 1.0;
            }

            // Color oscillation spring: target cycles between 0 and 1 every AURA_BEAT_PERIOD beats.
            // Per-cell phase offset for sparkle.
            let cell_hash = u32(cell.cell_gx) * 73856093u + u32(cell.cell_gz) * 19349663u;
            let cell_phase = f32(cell_hash & 0xFFFFu) / 65535.0 * 0.3;  // small phase scatter
            let osc_phase = (t_beats + cell_phase) / AURA_BEAT_PERIOD;
            let osc_target = sin(osc_phase * 2.0 * PI) * 0.5 + 0.5;

            // Spring toward oscillation target (shares stiffness with attack spring)
            let osc_d = cell.color_osc - osc_target;
            let osc_spring = -stiff * 0.5 * osc_d;  // softer than attack spring
            let osc_damp = -damp * sqrt(stiff * 0.5) * cell.color_osc_vel;
            cell.color_osc_vel += (osc_spring + osc_damp) * dt;
            cell.color_osc += cell.color_osc_vel * dt;
            cell.color_osc = clamp(cell.color_osc, 0.0, 1.0);

        }
    } else if (is_occupied) {
        // Outside influence — release toward zero
        let decay = 1.0 - exp(-pawn_aura_cfg.release_rate * dt);
        cell.intensity *= (1.0 - decay);
        cell.color_osc *= (1.0 - decay);  // oscillation fades with trail
        cell.velocity = 0.0;
        cell.color_osc_vel = 0.0;

        // Clear dead entries
        if (cell.intensity < 0.001) {
            cell.cell_gx = PAWN_AURA_EMPTY;
            cell.cell_gz = PAWN_AURA_EMPTY;
            cell.intensity = 0.0;
            cell.velocity = 0.0;
            cell.delta_r = 0.0;
            cell.delta_g = 0.0;
            cell.delta_b = 0.0;
            cell.height_delta = 0.0;
            cell.color_osc = 0.0;
            cell.color_osc_vel = 0.0;
        }
    }

    pawn_aura_cells[slot_idx] = cell;

    // Write to texture:
    //   R = height blend (0 when height disabled via height_scale=0)
    //   GBA = pre-multiplied color delta (modulated by oscillation)
    if (cell.intensity > 0.001) {
        let color_blend = cell.intensity * pawn_aura_cfg.tint_strength * cell.color_osc;
        let height_blend = select(cell.intensity * cell.height_delta, 0.0, pawn_aura_cfg.height_scale < 0.01);
        textureStore(pawn_aura_tex_write, vec2<i32>(gid.xy),
            vec4(height_blend,
                 cell.delta_r * color_blend,
                 cell.delta_g * color_blend,
                 cell.delta_b * color_blend));
    } else {
        textureStore(pawn_aura_tex_write, vec2<i32>(gid.xy), vec4(0.0));
    }
}


// §8 SELF-PORTRAIT GALLERY — Paintings of the point's journey over terrain
// (the body's self-portraits in pawn-host; the travelogue in free-fly)
// §8.0 PHOTOGRAPHER COMPUTE — GPU-coupled snapshot camera
struct PhotographerConfig {
    sun_direction: vec3<f32>,
    azimuth: f32,
    elevation: f32,
    distance: f32,
    fov_rad: f32,
    aspect_ratio: f32,
    patch_count: u32,
    frame_offset_x: f32,   // horizontal: -1 = pawn at left edge, +1 = right edge
    frame_offset_y: f32,   // vertical:   -1 = pawn at bottom, +1 = top
    _pad0: f32,
};

@group(0) @binding(140) var<uniform> photographer_config: PhotographerConfig;
@group(0) @binding(141) var<storage, read_write> photographer_vp: VPMatrix;
@group(0) @binding(142) var<storage, read_write> photographer_camera_out: CameraState;
@group(0) @binding(143) var<storage, read_write> photo_painting_slots: array<UnifiedPaintingSlot, 32>;
// (binding 144 photo_patch_instances removed — the coordinated edit the old
//  TODO[seam-map:cleanup] here called for; layouts dropped it in the same
//  commit — GROUND_CARD_1 H1.)
@group(0) @binding(145) var photo_heightfield: texture_2d_array<f32>;
@group(0) @binding(146) var photo_sampler: sampler;

struct ArchGroundEntry {
    pier_left_x: f32,
    pier_left_z: f32,
    pier_right_x: f32,
    pier_right_z: f32,
    ground_y: f32,
    is_active: u32,
    pier_correction_left: f32,    // CPU: max_pier - own_pier at left foot
    pier_correction_right: f32,   // CPU: max_pier - own_pier at right foot
};
@group(0) @binding(147) var<storage, read_write> arch_ground: array<ArchGroundEntry, 16>;

struct ColumnGroundEntry {
    center_x: f32,
    center_z: f32,
    ground_y: f32,
    is_active: u32,
    pier_correction: f32,         // CPU: max_pier - own_pier at column center
    _pad0: f32,
    _pad1: f32,
    _pad2: f32,
};
@group(0) @binding(148) var<storage, read_write> column_ground: array<ColumnGroundEntry, 32>;

// (binding 149 RETIRED — pyramid_ground, the residue-T2 husk: its computed
//  ground_y fed atlas slot 48, reader-free. Number parked.)

struct PalmGroundEntry {
    center_x: f32,
    center_z: f32,
    ground_y: f32,
    is_active: u32,
    _pad0: f32, _pad1: f32, _pad2: f32, _pad3: f32,
}
// Combined plant ground for compute Y-correction: palm[0..23] + cactus[24..43] + blade[44..75]
@group(0) @binding(150) var<storage, read_write> plant_ground: array<PalmGroundEntry, 76>;

// Entity ground atlas — compute writes corrected ground_y (r32float, 256×1)
@group(0) @binding(151) var entity_ground_atlas_write: texture_storage_2d<r32float, write>;

// Spatial index for O(1) patch lookup. CPU populates entries[lz*side + lx]
// with (layer + 1) for GENERATED/NEEDS_REGEN patches; 0 means empty slot.
// Replaces the old linear patch-instance scan in sample_terrain_y_at.
// Runtime-sized: capacity is the bound buffer's — the CPU side
// (Dim::MAX_ACTIVE_PATCHES) is the single source; no WGSL twin exists.
struct PatchGrid {
    origin_x: i32,
    origin_z: i32,
    side: u32,
    cell_extent: f32,
    entries: array<u32>,
}
@group(0) @binding(152) var<storage, read> patch_grid: PatchGrid;

// --- Terrain Height Sampling
// O(1) lookup: hash world_xz to patch grid cell, read layer, sample heightfield.
// Returns 0.0 outside the active patch window or on empty slots (preserves
// the old linear-scan behavior for out-of-range queries).
fn sample_terrain_y_at(world_xz: vec2<f32>) -> f32 {
    let gx = i32(floor(world_xz.x / patch_grid.cell_extent));
    let gz = i32(floor(world_xz.y / patch_grid.cell_extent));
    let lx = gx - patch_grid.origin_x;
    let lz = gz - patch_grid.origin_z;
    let s = i32(patch_grid.side);
    if (lx < 0 || lz < 0 || lx >= s || lz >= s) { return 0.0; }

    let packed = patch_grid.entries[lz * s + lx];
    if (packed == 0u) { return 0.0; }
    let layer = i32(packed - 1u);

    // Patch origin is the cell center; UV is local offset normalized to extent.
    let origin = vec2(f32(gx) + 0.5, f32(gz) + 0.5) * patch_grid.cell_extent;
    let local = world_xz - origin;
    let uv = local / patch_grid.cell_extent + 0.5;
    // Remap UV to texel centers (same as patch_terrain_vs)
    let res = f32(PATCH_HEIGHTFIELD_N);
    let sample_uv = (uv * (res - 1.0) + 0.5) / res;
    return textureSampleLevel(photo_heightfield, photo_sampler,
                              sample_uv, layer, 0.0).x;
}

// --- Look-At VP Matrix

fn build_lookat_vp(eye: vec3<f32>, aim_pt: vec3<f32>, fov_rad: f32, aspect: f32) -> mat4x4<f32> {
    let fwd = normalize(aim_pt - eye);

    var world_up = vec3(0.0, 1.0, 0.0);
    if (abs(fwd.y) > 0.99) { world_up = vec3(0.0, 0.0, 1.0); }

    let right = normalize(cross(fwd, world_up));
    let up = cross(right, fwd);

    let view = mat4x4<f32>(
        vec4(right.x,  up.x,  -fwd.x,  0.0),
        vec4(right.y,  up.y,  -fwd.y,  0.0),
        vec4(right.z,  up.z,  -fwd.z,  0.0),
        vec4(-dot(right, eye), -dot(up, eye), dot(fwd, eye), 1.0)
    );

    let near = 0.5;
    let far  = 500.0;
    let f = 1.0 / tan(fov_rad * 0.5);

    let proj = mat4x4<f32>(
        vec4(f / aspect,  0.0,  0.0,                        0.0),
        vec4(0.0,         f,    0.0,                        0.0),
        vec4(0.0,         0.0,  -far / (far - near),       -1.0),
        vec4(0.0,         0.0,  -near * far / (far - near),  0.0)
    );

    return proj * view;
}

// --- Photographer VP Compute (camera only — entity placement is separate)
// Dispatched only on snapshot frames. Builds the photographer camera VP
// and light VP for the snapshot render pass.
@compute @workgroup_size(1)
fn compute_photographer_vp() {
    let cfg = photographer_config;
    // THE POINT: the photographer frames the point — the body
    // when the pawn hosts (the self-portrait, identical to before),
    // the camera's vantage in free-fly (the travelogue — Jean's
    // ruling; the trigger already accumulates flight distance through
    // the point readback).
    let point_p = point_pos();

    // --- Camera position: spherical offset from the point
    let cos_el = cos(cfg.elevation);
    let sin_el = sin(cfg.elevation);
    let cos_az = cos(cfg.azimuth);
    let sin_az = sin(cfg.azimuth);

    let eye_raw = point_p + vec3(
        cfg.distance * cos_el * sin_az,
        cfg.distance * sin_el + 1.5,
        cfg.distance * cos_el * cos_az
    );

    // --- Clamp camera above actual terrain (O(1) patch_grid lookup).
    //
    // POLICY_BAKED_HEIGHTFIELD consumer (texture variant).
    // Same trade-off as the primary camera: the compute_photographer_vp
    // pipeline's bind group does not include live-contributor
    // resources, so this uses the cached heightfield (static_base +
    // pyramids only). See update_camera for the rationale.
    let terrain_at_cam = sample_terrain_y_at(eye_raw.xz);
    let eye = vec3(eye_raw.x, max(eye_raw.y, terrain_at_cam + 0.1), eye_raw.z);

    // --- Build VP looking at the point, with frame offset
    let aim_base = point_p + vec3(0.0, 1.0, 0.0);

    // Derive camera frame from initial eye→point direction
    let fwd_init = normalize(aim_base - eye);
    var world_up_init = vec3(0.0, 1.0, 0.0);
    if (abs(fwd_init.y) > 0.99) { world_up_init = vec3(0.0, 0.0, 1.0); }
    let right_init = normalize(cross(fwd_init, world_up_init));
    let up_init = cross(right_init, fwd_init);

    // Scale offset by FOV angular extent at the point's distance
    let half_fov = cfg.fov_rad * 0.5;
    let cam_to_point = length(aim_base - eye);
    let h_extent = tan(half_fov) * cam_to_point * cfg.aspect_ratio;
    let v_extent = tan(half_fov) * cam_to_point;

    // Shift aim point — camera looks away from the subject, placing it off-center
    let aim_pt = aim_base
        - right_init * cfg.frame_offset_x * h_extent
        - up_init * cfg.frame_offset_y * v_extent;

    photographer_vp.m = build_lookat_vp(eye, aim_pt, cfg.fov_rad, cfg.aspect_ratio);
    photographer_vp.light_vp = coupling_pawn_to_sun_vp(point_p, cfg.sun_direction);
    photographer_camera_out.pos = eye;
}

// --- Entity Placement Y-Correction
//
// GPU-as-single-source-of-truth for entity ground_y.
// CPU uploads ground_y = pier_offset_only (no terrain).
// This shader samples the heightfield and adds the terrain height.
//
// POLICY_BAKED_HEIGHTFIELD consumer (texture variant).
// sample_terrain_y_at reads the cached patch_heightfield_array_read,
// which is populated by the two-pass heightfield generator. That
// generator evaluates the baked heightfield's contributor set
// (static_base + pyramids) once per texel and caches the result.
// This Y-correction pass samples the cache rather than re-evaluating
// contributors analytically — it is *not* a spawn-time placement
// query. The analytical query_ground_placement_* functions have NO
// live caller today (STATUS: LATENT[policy-surface]): CPU spawn
// decisions stay on estimate_terrain_height (the tile-cache proxy in
// machine/spawn_engine.hpp), and this GPU pass is the live Y path via the
// baked heightfield.
//
// Paintings: terrain + GoL zone extrusion.
// Arch: 2-point min at pier feet + pier_height offset.
// Pyramid: 5-point min at center + 4 rotated corners.
// Column/antenna, palm, cactus, blade: single-point center.
//   (The blade GPU path IS live — this compute writes GROUND_ATLAS_BLADE
//   and the blade VS reads it; the old "excluded/CPU-mirror" note was stale.)
//
// b2b — WORLD-ANCHORED OVERLAY RIDE. The surface-STANDING
// families (paintings, column/antenna, palm/cactus/blade, arch feet) add
// contrib_gol_zones_at so they sit on the LIVE zone surface the mesh
// renders, not the baked static height — the sink/float fix. Raw GoL, no
// pawn suppression (structures are not movers). PYRAMIDS are EXCLUDED:
// they are CAST (buried occupiers the terrain drapes over), not
// surface-standers. GoL is the ONLY overlay ridden — and that is CORRECT,
// not a partial set:
//   - RADIAL PULSES: structures deliberately do NOT ride them — RULED a
//     non-issue (pulses are transient; nothing reads as floating over them,
//     the baked + GoL height is the right, working behavior). The
//     FrameSignal (signal.t_seconds) is NOT bound in this pipeline and is
//     NOT to be added. Recorded as a decision, not a gap.
//   - TERRAIN WAVES: the wave voice is dead today (config.terrain_time
//     gates it to 0 — a no-op regardless). Whether a revived wave should
//     carry structures is a future call, not wired here.
@compute @workgroup_size(1)
fn compute_entity_placement() {
    // Patch lookup is O(1) via patch_grid — no patch_count needed.

    // Y-correct all outdoor paintings (terrain quads + wall frame monuments).
    // Indoor wall frames use sentinel patch coords (0x7FFFFFFF) and are skipped.
    for (var i = 0u; i < 32u; i++) {
        if (photo_painting_slots[i].is_active != 0u &&
            photo_painting_slots[i].patch_gx != 0x7FFFFFFF) {
            let slot_xz = vec2(
                photo_painting_slots[i].position.x,
                photo_painting_slots[i].position.z
            );
            // Painting Y-correction — hybrid POLICY_PLACEMENT_PAINTING
            // evaluation. The cached heightfield (sample_terrain_y_at)
            // covers static_base + pyramids; GoL rides the card's
            // cell-exact raw lift (sample_live_card_gol — GROUND_CARD_1;
            // was an analytic zone loop). Equivalent in value to
            // query_ground_placement_painting(slot_xz) but cheaper per
            // call (baked texture lookup for the static portion).
            //
            // NO suppression term: POLICY_PLACEMENT_PAINTING does not
            // include CONTRIB_GOL_SUPPRESSION. Painting Y must be stable
            // against pawn position — placement does not depend on where
            // the pawn happens to stand. (Pre-refactor the subtraction
            // was present, but the policy declaration is the contract;
            // the code was wrong. See contracts/ground_architecture.hpp.)
            let ground = sample_terrain_y_at(slot_xz)
                       + sample_live_card_gol(slot_xz);
            // Terrain quads: center at ground + half-height (bottom at ground)
            // Wall frames: also lift by frame_width so the frame border clears the ground
            var lift = photo_painting_slots[i].scale_y * 0.5;
            if (photo_painting_slots[i].form_type == FORM_WALL_FRAME) {
                lift += photo_painting_slots[i].frame_width;
            }
            photo_painting_slots[i].position.y = ground + lift;
        }
    }

    // --- Column + antenna: single-point center sampling
    // Heightfield already includes pier contribution at entity position.
    for (var i = 0u; i < 32u; i++) {
        if (column_ground[i].is_active != 0u) {
            let xz = vec2(column_ground[i].center_x, column_ground[i].center_z);
            // b2b: ride the world-anchored GoL extrusion (raw — structures are
            // not movers, so no pawn suppression). Matches the painting hybrid;
            // seats the structure on the live zone surface instead of floating
            // on the baked static height. (Waves/pulses: see the b2b note at
            // compute_entity_placement's banner.)
            column_ground[i].ground_y = sample_terrain_y_at(xz) + sample_live_card_gol(xz);
            textureStore(entity_ground_atlas_write, vec2<i32>(i32(i) + GROUND_ATLAS_COLUMN, 0), vec4<f32>(column_ground[i].ground_y, 0.0, 0.0, 0.0));
        }
    }

    // --- Palm: plant_ground[0..23]
    for (var i = 0u; i < 24u; i++) {
        if (plant_ground[i].is_active != 0u) {
            let xz = vec2(plant_ground[i].center_x, plant_ground[i].center_z);
            // b2b: + world-anchored GoL (see column).
            plant_ground[i].ground_y = sample_terrain_y_at(xz) + sample_live_card_gol(xz);
            textureStore(entity_ground_atlas_write, vec2<i32>(i32(i) + GROUND_ATLAS_PALM, 0), vec4<f32>(plant_ground[i].ground_y, 0.0, 0.0, 0.0));
        }
    }

    // --- Cactus: plant_ground[24..43]
    for (var i = 0u; i < 20u; i++) {
        let slot = 24u + i;
        if (plant_ground[slot].is_active != 0u) {
            let xz = vec2(plant_ground[slot].center_x, plant_ground[slot].center_z);
            // b2b: + world-anchored GoL (see column).
            plant_ground[slot].ground_y = sample_terrain_y_at(xz) + sample_live_card_gol(xz);
            textureStore(entity_ground_atlas_write, vec2<i32>(i32(i) + GROUND_ATLAS_CACTUS, 0), vec4<f32>(plant_ground[slot].ground_y, 0.0, 0.0, 0.0));
        }
    }

    // --- Blade: plant_ground[44..75]
    for (var i = 0u; i < 32u; i++) {
        let slot = 44u + i;
        if (plant_ground[slot].is_active != 0u) {
            let xz = vec2(plant_ground[slot].center_x, plant_ground[slot].center_z);
            // b2b: + world-anchored GoL (see column).
            plant_ground[slot].ground_y = sample_terrain_y_at(xz) + sample_live_card_gol(xz);
            textureStore(entity_ground_atlas_write, vec2<i32>(i32(i) + GROUND_ATLAS_BLADE, 0), vec4<f32>(plant_ground[slot].ground_y, 0.0, 0.0, 0.0));
        }
    }

    // --- Arch: 2-point min at pier feet
    // Heightfield at pier feet already includes pier contribution.
    for (var i = 0u; i < 16u; i++) {
        if (arch_ground[i].is_active != 0u) {
            let left_xz = vec2(arch_ground[i].pier_left_x, arch_ground[i].pier_left_z);
            let right_xz = vec2(arch_ground[i].pier_right_x, arch_ground[i].pier_right_z);
            // b2b: each pier foot rides its own local GoL, then min (see column).
            let tl = sample_terrain_y_at(left_xz) + sample_live_card_gol(left_xz);
            let tr = sample_terrain_y_at(right_xz) + sample_live_card_gol(right_xz);
            arch_ground[i].ground_y = min(tl, tr);
            textureStore(entity_ground_atlas_write, vec2<i32>(i32(i) + GROUND_ATLAS_ARCH, 0), vec4<f32>(arch_ground[i].ground_y, 0.0, 0.0, 0.0));
        }
    }

    // (Pyramid arm REMOVED — the ground-atlas residue: it computed a 5-point ground_y and
    //  stored it to atlas slot 48, reader-free. Pyramids bake into
    //  the heightfield via contrib_pyramids_at — the live instance path.)
}


// §8.0.5 GPU FRUSTUM CULLING — Camera-visible patch selection
//
// Topical sibling of §7's compute passes (writes draw-indirect args
// for the main render), but located in §8 territory (between §8.0
// photographer compute and §8.1 gallery frame rendering) for
// bind-group locality with neighboring compute passes. Section
// number reflects file position; the topic is "GPU frustum culling
// of patch instances", which would belong in §7 by topic alone.
//
// Extracts 6 frustum planes from the VP matrix, tests each patch's AABB,
// classifies survivors into LOD0 (near) or LOD1 (far), and writes compact
// index arrays + indirect draw args via atomics.
//
// CPU pre-writes constant DrawIndexedIndirect fields and zeros instanceCount.
// This shader atomicAdds instanceCount and appends visible patch indices.
//
// Main pass reads through visible_patch_indices indirection.
// Shadow pass uses direct patch_instances[instance_index] (no frustum cull).

const FRUSTUM_PATCH_Y_MIN: f32 = -50.0;   // widened: terrain amplitude + entity heights
const FRUSTUM_PATCH_Y_MAX: f32 = 200.0;   // widened: tall entities (towers, antennas, ribbons)
// (FRUSTUM_LOD0_RADIUS_SQ REMOVED — the veil: the LOD0 gate now reads the
//  LIVE chain value fc_config.lod0_radius, the SAME config value the CPU
//  band reads — the four hand-kept spellings of 175 collapse to one.)

// Frustum cull compute bindings (dedicated bind group)
@group(0) @binding(1)   var<uniform>             fc_config: DesignConfig;
@group(0) @binding(2)   var<storage, read>       fc_vp: VPMatrix;
@group(0) @binding(340) var<storage, read>       fc_patches: array<PatchInstance>;
@group(0) @binding(500) var<storage, read_write> fc_visible: array<u32>;
@group(0) @binding(501) var<storage, read_write> fc_indirect: array<atomic<u32>, 5>;

// Extract frustum plane from VP matrix row combination.
// Row i of column-major M: (M[0][i], M[1][i], M[2][i], M[3][i])
// Left=row3+row0, Right=row3-row0, Bottom=row3+row1, Top=row3-row1, Near=row3+row2, Far=row3-row2
fn frustum_plane(m: mat4x4<f32>, col: u32, sign: f32) -> vec4<f32> {
    let p = vec4(
        m[0][3] + sign * m[0][col],
        m[1][3] + sign * m[1][col],
        m[2][3] + sign * m[2][col],
        m[3][3] + sign * m[3][col]
    );
    let len = length(p.xyz);
    if (len < EPSILON) { return vec4(0.0, 1.0, 0.0, 1000.0); }
    return p / len;
}

// Test AABB against 6 frustum planes. Returns true if potentially visible.
fn aabb_in_frustum(planes: array<vec4<f32>, 6>, bmin: vec3<f32>, bmax: vec3<f32>) -> bool {
    for (var i = 0u; i < 6u; i++) {
        let p = planes[i];
        // Positive vertex: AABB corner most aligned with plane normal
        let pv = vec3(
            select(bmin.x, bmax.x, p.x >= 0.0),
            select(bmin.y, bmax.y, p.y >= 0.0),
            select(bmin.z, bmax.z, p.z >= 0.0)
        );
        if (dot(p.xyz, pv) + p.w < 0.0) { return false; }
    }
    return true;
}

@compute @workgroup_size(64)
fn frustum_cull_patches(@builtin(global_invocation_id) id: vec3<u32>) {
    let patch_count = fc_config.placement_patch_count;
    if (id.x >= patch_count) { return; }

    let pi = fc_patches[id.x];
    if (pi.extent <= 0.0) { return; }

    // Build AABB with generous XZ margin (covers wave overlay, entity heights, pier reach,
    // and camera parallax between LOD boundary and actual geometry).
    let half = pi.extent * 0.5;
    let margin = pi.extent;   // 100% margin — one patch-width of slack
    let bmin = vec3(pi.origin.x - half - margin, FRUSTUM_PATCH_Y_MIN, pi.origin.y - half - margin);
    let bmax = vec3(pi.origin.x + half + margin, FRUSTUM_PATCH_Y_MAX, pi.origin.y + half + margin);

    // Extract 6 frustum planes from camera VP
    // WebGPU uses [0,1] depth range: near plane = row2 (not row3+row2)
    let vp = fc_vp.m;
    var planes: array<vec4<f32>, 6>;
    planes[0] = frustum_plane(vp, 0u, 1.0);   // left:   row3 + row0
    planes[1] = frustum_plane(vp, 0u, -1.0);  // right:  row3 - row0
    planes[2] = frustum_plane(vp, 1u, 1.0);   // bottom: row3 + row1
    planes[3] = frustum_plane(vp, 1u, -1.0);  // top:    row3 - row1
    planes[5] = frustum_plane(vp, 2u, -1.0);  // far:    row3 - row2

    // Near plane for [0,1] depth: cz >= 0 → just row2
    {
        let p = vec4(vp[0][2], vp[1][2], vp[2][2], vp[3][2]);
        let len = length(p.xyz);
        planes[4] = select(vec4(0.0, 1.0, 0.0, 1000.0), p / len, len >= EPSILON);
    }

    // Re-enabled: frustum test rejects out-of-view patches
    if (!aabb_in_frustum(planes, bmin, bmax)) { return; }

    // LOD0 only: nearest-edge distance² from the POINT XZ to patch edge.
    // Reads the CPU-banded point (lod_point_*) — NOT the live point —
    // so the GPU's LOD0 gate uses the same position the CPU used to
    // band patches into LOD0/LOD1 (else the two disagree at the
    // boundary annulus and patches flicker/z-fight). The radius is the
    // LIVE chain value lod0_radius — the same config value the CPU band
    // reads (one yardstick, both sides, by construction).
    let dx = max(0.0, abs(fc_config.lod_point_x - pi.origin.x) - half);
    let dz = max(0.0, abs(fc_config.lod_point_z - pi.origin.y) - half);
    let dist2 = dx * dx + dz * dz;

    if (dist2 <= fc_config.lod0_radius * fc_config.lod0_radius) {
        // Append to LOD0 visible list, atomicAdd indirect[1] (instanceCount)
        let slot = atomicAdd(&fc_indirect[1], 1u);
        fc_visible[slot] = id.x;
    }
    // else: LOD1 distance — not emitted; CPU handles via direct draw.
}


// §8.1 GALLERY FRAME RENDERING — Painting quads on the terrain

// --- Unified Painting Slot (must match GPUPaintingSlot in state.hpp, 128 bytes)
// Both terrain-quad and wall-frame forms read from this.

const FORM_TERRAIN_QUAD: u32 = 0u;
const FORM_WALL_FRAME:   u32 = 1u;

struct UnifiedPaintingSlot {
    // Common
    position:       vec3<f32>,
    texture_layer:  u32,
    forward:        vec3<f32>,
    form_type:      u32,
    up:             vec3<f32>,
    is_active:      u32,
    // Sizing (both forms)
    scale_x:        f32,
    scale_y:        f32,
    uv_scale_x:     f32,
    uv_scale_y:     f32,
    // Terrain quad fields
    geometry_seed:  f32,
    content_source: u32,
    patch_gx:       i32,
    patch_gz:       i32,
    // Wall frame fields
    frame_depth:    f32,
    frame_width:    f32,
    canvas_recess:  f32,
    _pad0:          f32,
    frame_color:    vec3<f32>,
    _pad1:          f32,
    _pad2:          vec4<f32>,
};

const PAINTING_MAX_SLOTS: u32 = 32u;

// --- Gallery Group 1 bindings (shared by terrain quads + wall frames)

@group(1) @binding(50) var<storage, read> painting_slots: array<UnifiedPaintingSlot, 32>;
@group(1) @binding(51) var painting_array: texture_2d_array<f32>;
@group(1) @binding(52) var painting_sampler_filt: sampler;

// --- Constants

const GALLERY_QUAD_N: u32 = 8u;

// --- Vertex Output

struct GalleryVarying {
    @builtin(position) clip_pos: vec4<f32>,
    @location(0) uv: vec2<f32>,
    @location(1) world_pos: vec3<f32>,
    @location(2) world_normal: vec3<f32>,
    @location(3) @interpolate(flat) texture_layer: u32,
};

// --- Frame Deformation
fn deform_gallery_frame(local: vec3<f32>, uv: vec2<f32>, seed: f32) -> vec3<f32> {
    var p = local;
    if (seed < 0.33) {
        let tilt = (seed / 0.33) * 0.02;
        p.z += tilt * sin(uv.x * 3.14159);
    } else if (seed < 0.66) {
        let t = (seed - 0.33) / 0.33;
        let bow_strength = 0.08 + t * 0.15;
        p.z += bow_strength * sin(uv.x * 3.14159) * p.x;
    } else {
        let t = (seed - 0.66) / 0.34;
        let freq = 3.0 + t * 2.0;
        let amp = 0.03 + t * 0.06;
        let n1 = sin(uv.x * freq * 6.283) * cos(uv.y * freq * 4.712);
        let n2 = sin((uv.x + 0.37) * freq * 5.1) * cos((uv.y + 0.71) * freq * 3.7);
        p.z += (n1 * 0.6 + n2 * 0.4) * amp;
    }
    return p;
}

// --- Vertex Shader
@vertex
fn gallery_frame_vs(
    @builtin(vertex_index) vid: u32,
    @builtin(instance_index) iid: u32,
) -> GalleryVarying {
    var out: GalleryVarying;
    let slot = painting_slots[iid];

    // Skip inactive or wall-frame slots (only terrain quads drawn here).
    // THE RING (draw authority): outdoor frames draw only inside the ring
    // (center − extent ≤ ring; 5 wu covers the frame's half-reach).
    let frame_in_ring = distance(slot.position.xz,
                                 vec2(config.lod_point_x, config.lod_point_z))
                        - 5.0 <= config.veil_ring;
    if (slot.is_active == 0u || slot.form_type != FORM_TERRAIN_QUAD || !frame_in_ring) {
        out.clip_pos = vec4(0.0, 0.0, 0.0, 1.0);
        out.uv = vec2(0.0);
        out.world_pos = vec3(0.0);
        out.world_normal = vec3(0.0, 1.0, 0.0);
        out.texture_layer = 0u;
        return out;
    }

    // Decode vid into grid vertex on the subdivided quad
    let N = GALLERY_QUAD_N;
    let tri_in_quad = vid / 3u;
    let vert_in_tri = vid % 3u;
    let quad_idx = tri_in_quad / 2u;
    let second_tri = tri_in_quad % 2u;
    let qx = quad_idx % N;
    let qy = quad_idx / N;

    // Grid vertex offsets within the quad cell
    //   Triangle 0: (0,0) (1,0) (0,1)
    //   Triangle 1: (1,0) (1,1) (0,1)
    var dx: u32; var dy: u32;
    if (second_tri == 0u) {
        switch (vert_in_tri) {
            case 0u: { dx = 0u; dy = 0u; }
            case 1u: { dx = 1u; dy = 0u; }
            default: { dx = 0u; dy = 1u; }
        }
    } else {
        switch (vert_in_tri) {
            case 0u: { dx = 1u; dy = 0u; }
            case 1u: { dx = 1u; dy = 1u; }
            default: { dx = 0u; dy = 1u; }
        }
    }

    let gx = qx + dx;
    let gy = qy + dy;
    let uv = vec2(f32(gx) / f32(N), f32(gy) / f32(N));

    // Local-space position: centered, scaled
    var local = vec3(
        (uv.x - 0.5) * slot.scale_x,
        (uv.y - 0.5) * slot.scale_y,
        0.0
    );

    // Apply seed-driven deformation
    local = deform_gallery_frame(local, uv, slot.geometry_seed);

    // Build local-to-world rotation from slot orientation vectors
    let fwd = normalize(slot.forward);
    let up_raw = normalize(slot.up);
    let right = normalize(cross(up_raw, fwd));
    let up = cross(fwd, right);

    // Transform to world space
    let world = slot.position + right * local.x + up * local.y + fwd * local.z;

    out.clip_pos = render_vp.m * vec4(world, 1.0);
    out.uv = vec2(uv.x, 1.0 - uv.y);
    out.world_pos = world;
    out.world_normal = fwd;
    out.texture_layer = slot.texture_layer;
    return out;
}

// §8.1.1 GALLERY FRAME SHADOW — CUT (orphan sweep): shadow_gallery_frame_vs
// was caller-free (draw_shadow_gallery_frames had no caller — gallery frames
// are drawn in the color pass but never cast a mesh-shadow). Shared helper
// deform_gallery_frame is retained (used by the live gallery_frame_vs).

// --- Fragment Shader
@fragment
fn gallery_frame_fs(in: GalleryVarying) -> @location(0) vec4<f32> {
    let painting_color = textureSample(painting_array, painting_sampler_filt, in.uv, in.texture_layer);
    if (painting_color.a < 0.01) { discard; }

    var color = painting_color.rgb;

    // Distance fog only (scene consistency — paintings far away dissolve into fog)
    let dist = distance(in.world_pos, render_camera.pos);
    let fog = 1.0 - exp(-dist * config.fog_density);
    color = mix(color, config.fog_color, fog);

    // THE ICING — outdoor terrain paintings share the band (own-FS
    // family; same term as shade_lit's). Their DRAW membership is the
    // ring gate in gallery_frame_vs; this fade is where their join
    // materializes. The gallery/indoor EXEMPTION is the MODE
    // (veil_strength = 0 in finite/indoor), not this family.
    // ANCHOR NOTE: this FS reads the STAGED point (config.lod_point_*,
    // the band's yardstick) rather than render_point_pos() — the gallery
    // pipeline's entity layout does not bind render_agents, and the
    // staged copy IS the point (1 frame stale, law E-4).
    let point_d = distance(in.world_pos.xz, vec2(config.lod_point_x, config.lod_point_z));
    let veil = smoothstep(config.veil_ring - config.veil_icing, config.veil_ring, point_d)
             * config.veil_strength;
    // THE RIM taste knob (same as shade_lit): dither-dissolve or tint.
    if (config.veil_dither > 0.5) {
        if (veil_dither_noise(in.world_pos.xz) < veil) { discard; }
    } else {
        color = mix(color, config.fog_color, veil);
    }

    return vec4(color, 1.0);
}


// §8.2 WALL-MOUNTED FRAMED PAINTINGS — reads from unified painting_slots
// ==============================================================
//
// 3D framed painting meshes. 78 vertices per painting.
// Reads from the same painting_slots array as terrain quads (binding 50).
// Only processes slots where form_type == WALL_FRAME.

const PAINTING_FRAME_VERTS_PER: u32 = 78u;

fn wp_right_from_slot(s: UnifiedPaintingSlot) -> vec3<f32> {
    return normalize(cross(s.up, s.forward));
}

struct WallPaintingVarying {
    @builtin(position) clip_pos: vec4<f32>,
    @location(0) world_pos: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
    @location(3) @interpolate(flat) painting_index: u32,
    @location(4) @interpolate(flat) is_canvas: u32,
    @location(5) frame_color: vec3<f32>,
}

fn compute_wall_painting_geometry(
    s: UnifiedPaintingSlot,
    pidx: u32,
    local_vid: u32
) -> WallPaintingVarying {
    var out: WallPaintingVarying;
    out.painting_index = pidx;
    out.frame_color = s.frame_color;
    out.clip_pos = vec4(0.0);

    let right = wp_right_from_slot(s);
    let hw = s.scale_x * 0.5;
    let hh = s.scale_y * 0.5;

    if (local_vid < 6u) {
        out.is_canvas = 1u;
        out.normal = s.forward;
        let canvas_pos = s.position + s.forward * (s.frame_depth - s.canvas_recess);

        var ci: u32;
        if (local_vid < 3u) { ci = local_vid; }
        else { let remap = array<u32, 3>(2u, 1u, 3u); ci = remap[local_vid - 3u]; }

        let corners = array<vec2<f32>, 4>(
            vec2(-hw, -hh), vec2(hw, -hh), vec2(-hw, hh), vec2(hw, hh)
        );
        let uvs = array<vec2<f32>, 4>(
            vec2(0.0, 1.0), vec2(1.0, 1.0), vec2(0.0, 0.0), vec2(1.0, 0.0)
        );
        let c = corners[ci];
        out.world_pos = canvas_pos + right * c.x + s.up * c.y;
        out.uv = uvs[ci] * vec2(s.uv_scale_x, s.uv_scale_y);

    } else if (local_vid < 54u) {
        out.is_canvas = 0u;
        let ev = local_vid - 6u;
        let side = ev / 12u;
        let tv = ev % 12u;
        let fw = s.frame_width;
        let fd = s.frame_depth;

        var is_: vec2<f32>; var ie: vec2<f32>;
        var os_: vec2<f32>; var oe: vec2<f32>;
        var sn: vec3<f32>;

        switch side {
            case 0u: {
                is_ = vec2(-hw, -hh); ie = vec2(hw, -hh);
                os_ = vec2(-hw - fw, -hh - fw); oe = vec2(hw + fw, -hh - fw);
                sn = -s.up;
            }
            case 1u: {
                is_ = vec2(hw, hh); ie = vec2(-hw, hh);
                os_ = vec2(hw + fw, hh + fw); oe = vec2(-hw - fw, hh + fw);
                sn = s.up;
            }
            case 2u: {
                is_ = vec2(-hw, hh); ie = vec2(-hw, -hh);
                os_ = vec2(-hw - fw, hh + fw); oe = vec2(-hw - fw, -hh - fw);
                sn = -right;
            }
            default: {
                is_ = vec2(hw, -hh); ie = vec2(hw, hh);
                os_ = vec2(hw + fw, -hh - fw); oe = vec2(hw + fw, hh + fw);
                sn = right;
            }
        }

        let qi = tv / 6u;
        let qv = tv % 6u;
        var ci: u32;
        if (qv < 3u) { ci = qv; }
        else { let remap = array<u32, 3>(2u, 1u, 3u); ci = remap[qv - 3u]; }

        if (qi == 0u) {
            out.normal = sn;
            let c2 = array<vec2<f32>, 4>(os_, oe, is_, ie);
            let depths = array<f32, 4>(fd, fd, 0.0, 0.0);
            let c = c2[ci];
            out.world_pos = s.position + right * c.x + s.up * c.y + s.forward * depths[ci];
        } else {
            out.normal = s.forward;
            let c2 = array<vec2<f32>, 4>(is_, ie, os_, oe);
            let c = c2[ci];
            out.world_pos = s.position + right * c.x + s.up * c.y + s.forward * fd;
        }
        out.uv = vec2(0.0);

    } else {
        out.is_canvas = 0u;
        out.normal = s.forward;
        let fv = local_vid - 54u;
        let ri = fv / 6u;
        let qv = fv % 6u;
        let fw = s.frame_width;
        let fd = s.frame_depth;

        var ci: u32;
        if (qv < 3u) { ci = qv; }
        else { let remap = array<u32, 3>(2u, 1u, 3u); ci = remap[qv - 3u]; }

        var rc: array<vec2<f32>, 4>;
        switch ri {
            case 0u: { rc = array<vec2<f32>, 4>(
                vec2(-hw - fw, -hh - fw), vec2(hw + fw, -hh - fw),
                vec2(-hw - fw, -hh),      vec2(hw + fw, -hh)); }
            case 1u: { rc = array<vec2<f32>, 4>(
                vec2(-hw - fw, hh),       vec2(hw + fw, hh),
                vec2(-hw - fw, hh + fw),  vec2(hw + fw, hh + fw)); }
            case 2u: { rc = array<vec2<f32>, 4>(
                vec2(-hw - fw, -hh),      vec2(-hw, -hh),
                vec2(-hw - fw, hh),       vec2(-hw, hh)); }
            default: { rc = array<vec2<f32>, 4>(
                vec2(hw, -hh),            vec2(hw + fw, -hh),
                vec2(hw, hh),             vec2(hw + fw, hh)); }
        }
        let c = rc[ci];
        out.world_pos = s.position + right * c.x + s.up * c.y + s.forward * fd;
        out.uv = vec2(0.0);
    }

    return out;
}

@vertex
fn wall_painting_vs(@builtin(vertex_index) vid: u32) -> WallPaintingVarying {
    let pidx = vid / PAINTING_FRAME_VERTS_PER;
    let local_vid = vid % PAINTING_FRAME_VERTS_PER;

    if (pidx >= PAINTING_MAX_SLOTS) {
        var out: WallPaintingVarying;
        out.clip_pos = vec4(0.0); out.world_pos = vec3(0.0);
        out.normal = vec3(0.0, 0.0, 1.0); out.uv = vec2(0.0);
        out.painting_index = 0u; out.is_canvas = 0u; out.frame_color = vec3(0.0);
        return out;
    }

    let slot = painting_slots[pidx];

    // Skip inactive or terrain-quad slots
    if (slot.is_active == 0u || slot.form_type != FORM_WALL_FRAME) {
        var out: WallPaintingVarying;
        out.clip_pos = vec4(0.0); out.world_pos = vec3(0.0);
        out.normal = vec3(0.0, 0.0, 1.0); out.uv = vec2(0.0);
        out.painting_index = 0u; out.is_canvas = 0u; out.frame_color = vec3(0.0);
        return out;
    }

    var out = compute_wall_painting_geometry(slot, pidx, local_vid);
    out.world_pos.y += sample_live_card(out.world_pos.xz).x;
    out.clip_pos = render_vp.m * vec4(out.world_pos, 1.0);
    return out;
}

@fragment
fn wall_painting_canvas_fs(in: WallPaintingVarying) -> @location(0) vec4<f32> {
    if (in.is_canvas == 0u) { discard; }

    let slot = painting_slots[in.painting_index];
    let tex_color = textureSample(painting_array, painting_sampler_filt, in.uv, slot.texture_layer);
    if (tex_color.a < 0.01) { discard; }

    let lit = tex_color.rgb * 0.9;
    let dist = distance(in.world_pos, render_camera.pos);
    let fog = 1.0 - exp(-dist * config.fog_density);
    return vec4(mix(lit, config.fog_color, fog), 1.0);
}

@fragment
fn wall_painting_frame_fs(in: WallPaintingVarying) -> @location(0) vec4<f32> {
    if (in.is_canvas == 1u) { discard; }

    let lit = in.frame_color * 0.8;
    let dist = distance(in.world_pos, render_camera.pos);
    let fog = 1.0 - exp(-dist * config.fog_density);
    return vec4(mix(lit, config.fog_color, fog), 1.0);
}

// §8.3 WALL PAINTING SHADOW — CUT (orphan sweep): shadow_wall_painting_vs
// was caller-free (draw_shadow_wall_paintings had no caller). Shared helper
// compute_wall_painting_geometry is retained (used by the live wall-painting
// color pass).


// ═══════════════════════════════════════════════════════════════════════
// §9 GPU ENTITY MESH GENERATION
// ═══════════════════════════════════════════════════════════════════════
//
// GPU-sovereign geometry: CPU authors intent (params), GPU realizes mesh.
// Each entity family writes into fixed per-slot regions of pre-allocated
// VB/IB buffers. Inactive slots receive degenerate (zero-area) triangles.
//
// Two families: arches (§9.1), columns (§9.2) — the pyramid's realization
// is the terrain itself (placement feeds the heightfield; no mesh, §9.0 retired).
//
// Vertex format: matches ArchVertex (pos[3], normal[3], color[3], index:u32)
// = 10 × f32 per vertex = 40 bytes. VB is accessed as array<f32>.

// §9.0 PYRAMID MESH GENERATION — REMOVED.
// The entire mesh-gen island (PMG_* consts, PyramidMeshParams, bindings
// 190-192, and the pmg_* writer/geometry helpers) was a write-only husk:
// its entry point pyramid_mesh_gen was cut long ago and no kernel read the
// buffers. Its C++ bind-group layout + buffers went with it. The LIVE pyramid
// path (instance array baked via contrib_pyramids_at; placement ground Y) is
// untouched.


// ─── §9.1 ARCH MESH GENERATION (catenary barrel vault) ───────────────
//
// Four sub-meshes per arch: outer shell, inner shell, front cap, back cap.
// Indexed vertices with shared edges (grid topology). The catenary
// parameter 'a' is precomputed on CPU and passed in params.
//
// Dispatch: (16, 4, 1) — 4 invocations per arch slot.
//   gid.x = slot index (0..15)
//   gid.y = sub-mesh (0=outer shell, 1=inner shell, 2=front cap, 3=back cap)
//
// Each sub-mesh writes to a deterministic offset within the slot's VB/IB
// region, computed from segs_u and segs_v. This distributes the buffer
// writes across 4 threads, avoiding per-thread execution limits.

// ── Constants ─────────────────────────────────────────────────────────

const AMG_MAX_VERTS_PER_SLOT: u32   = 2000u;
const AMG_MAX_INDICES_PER_SLOT: u32 = 7500u;  // must be divisible by 3 (triangle alignment)
const AMG_FLOATS_PER_VERTEX: u32    = 10u;   // pos(3) + normal(3) + color(3) + index(1)
const AMG_MAX_SLOTS: u32            = 16u;
const AMG_TOTAL_INDICES: u32        = 120000u; // 16 × 7500

// Sub-mesh IDs
const AMG_OUTER_SHELL: u32 = 0u;
const AMG_INNER_SHELL: u32 = 1u;
const AMG_FRONT_CAP: u32   = 2u;
const AMG_BACK_CAP: u32    = 3u;

// ── Parameter buffer ──────────────────────────────────────────────────
//
// MUST match state.hpp::GPUArchMeshParams (size: 64 bytes).
// If this struct gains/loses a field, the CPU side and
// its state.hpp sizeof static_assert must be updated together.

struct ArchMeshParams {
    center_x: f32,
    center_z: f32,
    rotation: f32,
    half_span: f32,
    rise: f32,
    depth: f32,
    thickness: f32,
    pier_height: f32,
    burial: f32,
    catenary_a: f32,
    segs_u: u32,
    segs_v: u32,
    color_r: f32,
    color_g: f32,
    color_b: f32,
    is_active: u32,
}

// ── Bindings (dedicated layout — different binding numbers from pyramid) ─

@group(0) @binding(193) var<storage, read>       amg_params: array<ArchMeshParams, 16>;
@group(0) @binding(194) var<storage, read_write>  amg_vertices: array<f32>;
@group(0) @binding(195) var<storage, read_write>  amg_indices: array<u32>;

// ── Helpers ───────────────────────────────────────────────────────────

fn amg_cosh(x: f32) -> f32 {
    let ex = exp(x);
    return (ex + 1.0 / ex) * 0.5;
}

fn amg_sinh(x: f32) -> f32 {
    let ex = exp(x);
    return (ex - 1.0 / ex) * 0.5;
}

fn amg_write_vertex(
    abs_idx: u32,
    px: f32, py: f32, pz: f32,
    nx: f32, ny: f32, nz: f32,
    cr: f32, cg: f32, cb: f32,
    entity_idx: u32
) {
    let i = abs_idx * AMG_FLOATS_PER_VERTEX;
    amg_vertices[i + 0u] = px;
    amg_vertices[i + 1u] = py;
    amg_vertices[i + 2u] = pz;
    amg_vertices[i + 3u] = nx;
    amg_vertices[i + 4u] = ny;
    amg_vertices[i + 5u] = nz;
    amg_vertices[i + 6u] = cr;
    amg_vertices[i + 7u] = cg;
    amg_vertices[i + 8u] = cb;
    amg_vertices[i + 9u] = f32(entity_idx);
}

// ── Shell generation (one sub-mesh thread) ────────────────────────────
//
// Writes (su+1)*(sv+1) vertices and su*sv*6 indices at the given offsets.
// offset = +half_t (outer) or -half_t (inner).
// nsign = +1.0 (outer) or -1.0 (inner).
//
// Catenary profile is precomputed once per invocation into thread-local
// arrays, then reused across all v-rows. This eliminates (sv) redundant
// exp() evaluations per u-column (13× reduction for monumental arches).

const AMG_MAX_PROFILE: u32 = 49u;  // max su+1 (monumental: 48+1)

fn amg_gen_shell(
    p: ArchMeshParams, slot: u32,
    vb_start: u32, ib_start: u32,
    offset: f32, nsign: f32,
    co: f32, si: f32, base_y: f32, a: f32, H: f32
) {
    let su = p.segs_u;
    let sv = p.segs_v;
    let half_d = p.depth * 0.5;
    let stride = su + 1u;

    // ── Precompute catenary profile (one exp pair per u-column) ──
    var cat_lx: array<f32, 49>;   // t + pnx * offset
    var cat_ly: array<f32, 49>;   // base_y + y + pny * offset
    var cat_pnx: array<f32, 49>;  // profile normal x (for world normal)
    var cat_pny: array<f32, 49>;  // profile normal y

    for (var iu = 0u; iu <= su; iu++) {
        let u = f32(iu) / f32(su);
        let t = -p.half_span + 2.0 * p.half_span * u;

        let y = H - a * (amg_cosh(t / a) - 1.0);
        let sh = amg_sinh(t / a);
        let plen = sqrt(sh * sh + 1.0);
        let pnx = sh / plen;
        let pny = 1.0 / plen;

        cat_lx[iu] = t + pnx * offset;
        cat_ly[iu] = base_y + y + pny * offset;
        cat_pnx[iu] = pnx;
        cat_pny[iu] = pny;
    }

    // ── Vertices: sweep profile across depth ──
    var vi = vb_start;
    for (var iv = 0u; iv <= sv; iv++) {
        let v = f32(iv) / f32(sv);
        let lz = -half_d + p.depth * v;
        for (var iu = 0u; iu <= su; iu++) {
            let wx = p.center_x + cat_lx[iu] * co - lz * si;
            let wy = cat_ly[iu];
            let wz = p.center_z + cat_lx[iu] * si + lz * co;

            let wnx = (cat_pnx[iu] * nsign) * co;
            let wny = cat_pny[iu] * nsign;
            let wnz = (cat_pnx[iu] * nsign) * si;

            amg_write_vertex(vi, wx, wy, wz, wnx, wny, wnz,
                p.color_r, p.color_g, p.color_b, slot);
            vi++;
        }
    }

    // ── Indices ──
    var ii = ib_start;
    for (var iv = 0u; iv < sv; iv++) {
        for (var iu = 0u; iu < su; iu++) {
            let i00 = vb_start + iv * stride + iu;
            let i10 = i00 + 1u;
            let i01 = i00 + stride;
            let i11 = i01 + 1u;
            if (nsign > 0.0) {
                amg_indices[ii] = i00; ii++;
                amg_indices[ii] = i01; ii++;
                amg_indices[ii] = i10; ii++;
                amg_indices[ii] = i10; ii++;
                amg_indices[ii] = i01; ii++;
                amg_indices[ii] = i11; ii++;
            } else {
                amg_indices[ii] = i00; ii++;
                amg_indices[ii] = i10; ii++;
                amg_indices[ii] = i01; ii++;
                amg_indices[ii] = i10; ii++;
                amg_indices[ii] = i11; ii++;
                amg_indices[ii] = i01; ii++;
            }
        }
    }
}

// ── Cap generation (one sub-mesh thread) ──────────────────────────────
//
// Writes 2*(su+1) vertices and su*6 indices at the given offsets.
// lz_pos = +half_d (front) or -half_d (back).
// nz_sign = +1.0 (front) or -1.0 (back).

fn amg_gen_cap(
    p: ArchMeshParams, slot: u32,
    vb_start: u32, ib_start: u32,
    lz_pos: f32, nz_sign: f32,
    co: f32, si: f32, base_y: f32, a: f32, H: f32
) {
    let su = p.segs_u;
    let half_t = p.thickness * 0.5;

    // Cap normal (rotated into world)
    let cap_nx = -nz_sign * si;
    let cap_ny = 0.0;
    let cap_nz = nz_sign * co;

    // Vertices: outer/inner pairs along catenary profile
    var vi = vb_start;
    for (var iu = 0u; iu <= su; iu++) {
        let u = f32(iu) / f32(su);
        let t = -p.half_span + 2.0 * p.half_span * u;

        let y = H - a * (amg_cosh(t / a) - 1.0);
        let sh = amg_sinh(t / a);
        let plen = sqrt(sh * sh + 1.0);
        let pnx = sh / plen;
        let pny = 1.0 / plen;

        // Outer vertex
        let olx = t + pnx * half_t;
        let oly = base_y + y + pny * half_t;
        amg_write_vertex(vi,
            p.center_x + olx * co - lz_pos * si,
            oly,
            p.center_z + olx * si + lz_pos * co,
            cap_nx, cap_ny, cap_nz,
            p.color_r, p.color_g, p.color_b, slot);
        vi++;

        // Inner vertex
        let ilx = t - pnx * half_t;
        let ily = base_y + y - pny * half_t;
        amg_write_vertex(vi,
            p.center_x + ilx * co - lz_pos * si,
            ily,
            p.center_z + ilx * si + lz_pos * co,
            cap_nx, cap_ny, cap_nz,
            p.color_r, p.color_g, p.color_b, slot);
        vi++;
    }

    // Indices: quad strip between outer/inner
    var ii = ib_start;
    for (var iu = 0u; iu < su; iu++) {
        let o0 = vb_start + iu * 2u;
        let i0 = o0 + 1u;
        let o1 = vb_start + (iu + 1u) * 2u;
        let i1 = o1 + 1u;
        if (nz_sign > 0.0) {
            amg_indices[ii] = o0; ii++;
            amg_indices[ii] = i0; ii++;
            amg_indices[ii] = o1; ii++;
            amg_indices[ii] = o1; ii++;
            amg_indices[ii] = i0; ii++;
            amg_indices[ii] = i1; ii++;
        } else {
            amg_indices[ii] = o0; ii++;
            amg_indices[ii] = o1; ii++;
            amg_indices[ii] = i0; ii++;
            amg_indices[ii] = o1; ii++;
            amg_indices[ii] = i1; ii++;
            amg_indices[ii] = i0; ii++;
        }
    }
}

// ── Compute entry point ───────────────────────────────────────────────
//
// Dispatch: (16, 4, 1)
//   gid.x = slot, gid.y = sub-mesh
//
// Sub-mesh VB/IB offsets within a slot (deterministic from segs_u, segs_v):
//   shell_verts  = (su+1)*(sv+1)
//   shell_indices = su*sv*6
//   cap_verts    = 2*(su+1)
//   cap_indices  = su*6
//
//   sub 0 (outer shell): vb=0,                 ib=0
//   sub 1 (inner shell): vb=shell_verts,       ib=shell_indices
//   sub 2 (front cap):   vb=2*shell_verts,     ib=2*shell_indices
//   sub 3 (back cap):    vb=2*shell_verts+cap, ib=2*shell_indices+cap_indices

@compute @workgroup_size(1, 1, 1)
fn arch_mesh_gen(@builtin(global_invocation_id) gid: vec3<u32>) {
    let slot = gid.x;
    let sub_mesh = gid.y;
    if (slot >= AMG_MAX_SLOTS || sub_mesh >= 4u) { return; }

    let p = amg_params[slot];
    let slot_vb = slot * AMG_MAX_VERTS_PER_SLOT;
    let slot_ib = slot * AMG_MAX_INDICES_PER_SLOT;

    // ── Inactive: each sub-mesh zeroes its quarter of the index range ─
    if (p.is_active == 0u) {
        let chunk = AMG_MAX_INDICES_PER_SLOT / 4u;
        let start = slot_ib + sub_mesh * chunk;
        for (var i = 0u; i < chunk; i++) {
            amg_indices[start + i] = slot_vb;
        }
        return;
    }

    // ── Active: shared constants ──────────────────────────────────
    let co = cos(p.rotation);
    let si = sin(p.rotation);
    let base_y = -p.burial;
    let a = p.catenary_a;
    let H = a * (amg_cosh(p.half_span / a) - 1.0);
    let half_t = p.thickness * 0.5;
    let half_d = p.depth * 0.5;
    let su = p.segs_u;
    let sv = p.segs_v;

    // Pre-compute sub-mesh offsets
    let shell_verts = (su + 1u) * (sv + 1u);
    let shell_indices = su * sv * 6u;
    let cap_verts = 2u * (su + 1u);
    let cap_indices = su * 6u;

    switch (sub_mesh) {
        case 0u: {
            // Outer shell
            amg_gen_shell(p, slot,
                slot_vb, slot_ib,
                half_t, 1.0,
                co, si, base_y, a, H);
        }
        case 1u: {
            // Inner shell
            amg_gen_shell(p, slot,
                slot_vb + shell_verts,
                slot_ib + shell_indices,
                -half_t, -1.0,
                co, si, base_y, a, H);
        }
        case 2u: {
            // Front cap
            amg_gen_cap(p, slot,
                slot_vb + 2u * shell_verts,
                slot_ib + 2u * shell_indices,
                half_d, 1.0,
                co, si, base_y, a, H);

            // This thread also zeroes unused indices after the caps.
            // Total used = 2*shell_indices + 2*cap_indices.
            let total_used = 2u * shell_indices + 2u * cap_indices;
            for (var i = total_used; i < AMG_MAX_INDICES_PER_SLOT; i++) {
                amg_indices[slot_ib + i] = slot_vb;
            }
        }
        case 3u: {
            // Back cap
            amg_gen_cap(p, slot,
                slot_vb + 2u * shell_verts + cap_verts,
                slot_ib + 2u * shell_indices + cap_indices,
                -half_d, -1.0,
                co, si, base_y, a, H);
        }
        default: {}
    }
}


// ─── §9.2 COLUMN MESH GENERATION (surface of revolution) ────────────
//
// Profile polyline: base layers → shaft (taper + entasis) → capital layers.
// Revolution around Y axis. Horizontal discs at radius transitions.
//
// Dispatch: (32, 1, 1) — one invocation per column slot.

// ── Constants ─────────────────────────────────────────────────────────

const CMG_MAX_VERTS_PER_SLOT: u32    = 1500u;
const CMG_MAX_INDICES_PER_SLOT: u32  = 6000u; // must be divisible by 3
const CMG_FLOATS_PER_VERTEX: u32     = 10u;   // pos(3) + normal(3) + color(3) + index(1)
const CMG_MAX_SLOTS: u32             = 32u;
const CMG_MAX_PROFILE: u32           = 32u;    // max profile polyline points
const CMG_MAX_DISCS: u32             = 12u;    // max horizontal disc records
const CMG_PI: f32                    = 3.14159265359;

// ── Parameter buffer ──────────────────────────────────────────────────
//
// MUST match state.hpp::GPUColumnMeshParams (size: 128 bytes).
// If this struct gains/loses a field, the CPU side and
// its state.hpp sizeof static_assert must be updated together.

struct ColumnMeshParams {
    center_x: f32,
    center_z: f32,
    height: f32,
    shaft_radius: f32,
    taper: f32,
    entasis: f32,
    base_height: f32,
    base_overhang: f32,
    capital_height: f32,
    capital_overhang: f32,
    burial: f32,
    color_r: f32,
    color_g: f32,
    color_b: f32,
    base_layers: u32,
    capital_layers: u32,
    segs_around: u32,
    shaft_rings: u32,
    is_active: u32,
    tier: u32,
    // Antenna drum colors (3 drums × RGB)
    drum_color_r1: f32, drum_color_g1: f32, drum_color_b1: f32,
    drum_color_r2: f32, drum_color_g2: f32, drum_color_b2: f32,
    drum_color_r3: f32, drum_color_g3: f32, drum_color_b3: f32,
    _pad128_0: f32, _pad128_1: f32, _pad128_2: f32,
}

// ── Bindings ──────────────────────────────────────────────────────────

@group(0) @binding(196) var<storage, read>       cmg_params: array<ColumnMeshParams, 32>;
@group(0) @binding(197) var<storage, read_write>  cmg_vertices: array<f32>;
@group(0) @binding(198) var<storage, read_write>  cmg_indices: array<u32>;
// COLUMN CEILING FIT: the ceiling gate + the correction pass's ground
// output (read-only view of binding 148's buffer — slot-aligned with
// cmg_params: columns 0.., antennas at ANTENNA_SLOT_OFFSET).
@group(0) @binding(190) var<uniform>             cmg_config: DesignConfig;
@group(0) @binding(191) var<storage, read>       cmg_column_ground: array<ColumnGroundEntry, 32>;

// COLUMN_MIN_INDOOR_HEIGHT: extreme-terrain floor — a column never
// collapses below this. PINNED PAIR with COLUMN_MIN_INDOOR_HEIGHT in
// contracts/indoor_module.hpp (authored in both rooms, named the
// same, never dial-derived — the TILE_GRID_CAPACITY pattern).
const COLUMN_MIN_INDOOR_HEIGHT: f32 = 1.0;

// ── Helpers ───────────────────────────────────────────────────────────

fn cmg_write_vertex(
    abs_idx: u32,
    px: f32, py: f32, pz: f32,
    nx: f32, ny: f32, nz: f32,
    cr: f32, cg: f32, cb: f32,
    entity_idx: u32
) {
    let i = abs_idx * CMG_FLOATS_PER_VERTEX;
    cmg_vertices[i + 0u] = px;
    cmg_vertices[i + 1u] = py;
    cmg_vertices[i + 2u] = pz;
    cmg_vertices[i + 3u] = nx;
    cmg_vertices[i + 4u] = ny;
    cmg_vertices[i + 5u] = nz;
    cmg_vertices[i + 6u] = cr;
    cmg_vertices[i + 7u] = cg;
    cmg_vertices[i + 8u] = cb;
    cmg_vertices[i + 9u] = f32(entity_idx);
}

// ── Compute entry point ───────────────────────────────────────────────

@compute @workgroup_size(1, 1, 1)
fn column_mesh_gen(@builtin(global_invocation_id) gid: vec3<u32>) {
    let slot = gid.x;
    if (slot >= CMG_MAX_SLOTS) { return; }

    let p = cmg_params[slot];
    let slot_vb = slot * CMG_MAX_VERTS_PER_SLOT;
    let slot_ib = slot * CMG_MAX_INDICES_PER_SLOT;

    const TIER_ANTENNA_FIRST: u32 = 3u;  // ANTENNA=3, ANTENNA_SQUAT=4, ANTENNA_COLOSSAL=5

    // COLUMN CEILING FIT (columns only — antennas keep CPU height:
    // CAP-law, not ceiling-flush): indoors the column's visual height
    // is ceiling-plane-relative — derived HERE, where ground_y is
    // known (GPU sovereignty; the CPU adapter fits proportions only).
    // Outdoors ceiling_height == 0: the select's false arm keeps
    // eff_h = p.height byte-identical. One select, one max.
    let eff_h = select(p.height,
                       max(cmg_config.ceiling_height - cmg_column_ground[slot].ground_y,
                           COLUMN_MIN_INDOOR_HEIGHT),
                       cmg_config.ceiling_height > 0.0 && p.tier < TIER_ANTENNA_FIRST);

    // ── Inactive: zero all indices ─────────────────────────────
    if (p.is_active == 0u) {
        for (var i = 0u; i < CMG_MAX_INDICES_PER_SLOT; i++) {
            cmg_indices[slot_ib + i] = slot_vb;
        }
        return;
    }

    // ── Build profile polyline ─────────────────────────────────
    // Thread-local arrays: (radius, y, color) per profile point
    var prof_r: array<f32, 32>;
    var prof_y: array<f32, 32>;
    var prof_cr: array<f32, 32>;
    var prof_cg: array<f32, 32>;
    var prof_cb: array<f32, 32>;
    var pc = 0u;  // profile count

    let top_radius = p.shaft_radius * p.taper;
    let base_top_y = p.base_height;
    let shaft_top_y = eff_h - p.capital_height;
    let bl = p.base_layers;
    let cl = p.capital_layers;
    let sr = p.shaft_rings;
    let sa = p.segs_around;

    let base_cr = p.color_r;
    let base_cg = p.color_g;
    let base_cb = p.color_b;

    // Drum colors (only used by antenna tier)
    var drum_cr: array<f32, 3>;
    var drum_cg: array<f32, 3>;
    var drum_cb: array<f32, 3>;
    drum_cr[0] = p.drum_color_r1; drum_cg[0] = p.drum_color_g1; drum_cb[0] = p.drum_color_b1;
    drum_cr[1] = p.drum_color_r2; drum_cg[1] = p.drum_color_g2; drum_cb[1] = p.drum_color_b2;
    drum_cr[2] = p.drum_color_r3; drum_cg[2] = p.drum_color_g3; drum_cb[2] = p.drum_color_b3;

    if (p.tier >= TIER_ANTENNA_FIRST) {
        // ── ANTENNA profile: post → (drum + spacer) × N ─────
        // Field reuse: base_layers=drum_count, base_height=drum_height,
        // base_overhang=drum_radius_overhang, capital_height=spacer_height,
        // taper=drum taper (top/bottom ratio).

        let post_r = p.shaft_radius;
        let drum_count = min(bl, 3u);
        let drum_h = p.base_height;
        let drum_overhang = p.base_overhang;
        let spacer_h = p.capital_height;
        let drum_taper = p.taper;

        // Total content height: drums + spacers + top post cap
        let content_h = f32(drum_count) * drum_h + f32(max(drum_count, 1u) - 1u) * spacer_h;
        // Post extends from ground up; drums sit in the upper portion
        let drum_start_y = p.height - content_h;

        // Post from ground to first drum
        prof_r[pc] = post_r; prof_y[pc] = -p.burial;
        prof_cr[pc] = base_cr; prof_cg[pc] = base_cg; prof_cb[pc] = base_cb; pc++;
        prof_r[pc] = post_r; prof_y[pc] = drum_start_y - p.burial;
        prof_cr[pc] = base_cr; prof_cg[pc] = base_cg; prof_cb[pc] = base_cb; pc++;

        for (var d = 0u; d < drum_count; d++) {
            let dy_base = drum_start_y + f32(d) * (drum_h + spacer_h);
            let bottom_r = post_r + drum_overhang * (0.6 + 0.4 * fract(f32(d) * 0.618 + f32(p.segs_around) * 0.1));  // independent size per drum
            let top_r = bottom_r * drum_taper;
            let dcr = drum_cr[d];
            let dcg = drum_cg[d];
            let dcb = drum_cb[d];

            // Widen from post to drum bottom
            prof_r[pc] = bottom_r; prof_y[pc] = dy_base - p.burial;
            prof_cr[pc] = dcr; prof_cg[pc] = dcg; prof_cb[pc] = dcb; pc++;

            // Drum body: a few rings for curvature
            for (var ri = 1u; ri <= 3u; ri++) {
                let t = f32(ri) / 4.0;
                let ry = dy_base + t * drum_h;
                let rr = bottom_r + (top_r - bottom_r) * t;
                prof_r[pc] = rr; prof_y[pc] = ry - p.burial;
                prof_cr[pc] = dcr; prof_cg[pc] = dcg; prof_cb[pc] = dcb; pc++;
            }

            // Drum top
            prof_r[pc] = top_r; prof_y[pc] = dy_base + drum_h - p.burial;
            prof_cr[pc] = dcr; prof_cg[pc] = dcg; prof_cb[pc] = dcb; pc++;

            // Narrow back to post (spacer between drums)
            if (d + 1u < drum_count) {
                prof_r[pc] = post_r; prof_y[pc] = dy_base + drum_h - p.burial;
                prof_cr[pc] = base_cr; prof_cg[pc] = base_cg; prof_cb[pc] = base_cb; pc++;
                prof_r[pc] = post_r; prof_y[pc] = dy_base + drum_h + spacer_h - p.burial;
                prof_cr[pc] = base_cr; prof_cg[pc] = base_cg; prof_cb[pc] = base_cb; pc++;
            }
        }

        // Top cap point (narrow to post)
        prof_r[pc] = post_r; prof_y[pc] = p.height - p.burial;
        prof_cr[pc] = base_cr; prof_cg[pc] = base_cg; prof_cb[pc] = base_cb; pc++;

    } else {
        // ── Classical column profile (Pillar / Doric / Ornate) ───

        // Base layers: concentric rings stepping inward
        if (bl > 0u && p.base_height > 0.001) {
            for (var i = 0u; i < bl; i++) {
                let t0 = f32(i) / f32(bl);
                let t1 = f32(i + 1u) / f32(bl);
                let r0 = p.shaft_radius + p.base_overhang * (1.0 - t0);
                let y_bot = t0 * p.base_height - p.burial;
                let y_top = t1 * p.base_height - p.burial;
                prof_r[pc] = r0; prof_y[pc] = y_bot;
                prof_cr[pc] = base_cr; prof_cg[pc] = base_cg; prof_cb[pc] = base_cb; pc++;
                prof_r[pc] = r0; prof_y[pc] = y_top;
                prof_cr[pc] = base_cr; prof_cg[pc] = base_cg; prof_cb[pc] = base_cb; pc++;
            }
            prof_r[pc] = p.shaft_radius; prof_y[pc] = base_top_y - p.burial;
            prof_cr[pc] = base_cr; prof_cg[pc] = base_cg; prof_cb[pc] = base_cb; pc++;
        } else {
            prof_r[pc] = p.shaft_radius; prof_y[pc] = -p.burial;
            prof_cr[pc] = base_cr; prof_cg[pc] = base_cg; prof_cb[pc] = base_cb; pc++;
        }

        // Shaft (with taper and entasis)
        for (var i = 0u; i <= sr; i++) {
            let t = f32(i) / f32(sr);
            let y = base_top_y + t * (shaft_top_y - base_top_y);
            var r = p.shaft_radius + (top_radius - p.shaft_radius) * t;
            r += p.entasis * p.shaft_radius * sin(t * CMG_PI);
            prof_r[pc] = r; prof_y[pc] = y - p.burial;
            prof_cr[pc] = base_cr; prof_cg[pc] = base_cg; prof_cb[pc] = base_cb; pc++;
        }

        // Capital layers: concentric rings stepping outward
        if (cl > 0u && p.capital_height > 0.001) {
            for (var i = 0u; i < cl; i++) {
                let t0 = f32(i) / f32(cl);
                let t1 = f32(i + 1u) / f32(cl);
                let r0 = top_radius + p.capital_overhang * t0;
                let y_bot = shaft_top_y + t0 * p.capital_height - p.burial;
                let y_top = shaft_top_y + t1 * p.capital_height - p.burial;
                prof_r[pc] = r0; prof_y[pc] = y_bot;
                prof_cr[pc] = base_cr; prof_cg[pc] = base_cg; prof_cb[pc] = base_cb; pc++;
                prof_r[pc] = r0; prof_y[pc] = y_top;
                prof_cr[pc] = base_cr; prof_cg[pc] = base_cg; prof_cb[pc] = base_cb; pc++;
            }
            let r_final = top_radius + p.capital_overhang;
            prof_r[pc] = r_final; prof_y[pc] = eff_h - p.burial;
            prof_cr[pc] = base_cr; prof_cg[pc] = base_cg; prof_cb[pc] = base_cb; pc++;
        } else {
            prof_r[pc] = top_radius; prof_y[pc] = eff_h - p.burial;
            prof_cr[pc] = base_cr; prof_cg[pc] = base_cg; prof_cb[pc] = base_cb; pc++;
        }
    }

    // ── Build disc records ─────────────────────────────────────
    // (r_inner, r_outer, y, normal_y)
    var disc_ri: array<f32, 12>;
    var disc_ro: array<f32, 12>;
    var disc_y: array<f32, 12>;
    var disc_ny: array<f32, 12>;
    // Per-disc colors (for antenna drums)
    var disc_cr: array<f32, 12>;
    var disc_cg: array<f32, 12>;
    var disc_cb: array<f32, 12>;
    var dc = 0u;  // disc count

    if (p.tier < TIER_ANTENNA_FIRST) {
        // Base annular discs (top face at each step)
        if (bl > 0u && p.base_height > 0.001) {
            for (var i = 0u; i < bl; i++) {
                let t0 = f32(i) / f32(bl);
                let t1 = f32(i + 1u) / f32(bl);
                let r0 = p.shaft_radius + p.base_overhang * (1.0 - t0);
                let r1 = p.shaft_radius + p.base_overhang * (1.0 - t1);
                let y_top = t1 * p.base_height - p.burial;
                if (r1 < r0 - 0.001) {
                    disc_ri[dc] = r1; disc_ro[dc] = r0;
                    disc_y[dc] = y_top; disc_ny[dc] = 1.0;
                    disc_cr[dc] = base_cr; disc_cg[dc] = base_cg; disc_cb[dc] = base_cb;
                    dc++;
                }
            }
        }

        // Capital annular discs (bottom face at each shelf)
        if (cl > 0u && p.capital_height > 0.001) {
            for (var i = 1u; i < cl; i++) {
                let t0 = f32(i) / f32(cl);
                let r0 = top_radius + p.capital_overhang * t0;
                let r_prev = top_radius + p.capital_overhang * (f32(i - 1u) / f32(cl));
                let y_bot = shaft_top_y + t0 * p.capital_height - p.burial;
                if (r0 > top_radius + 0.001) {
                    disc_ri[dc] = r_prev; disc_ro[dc] = r0;
                    disc_y[dc] = y_bot; disc_ny[dc] = -1.0;
                    disc_cr[dc] = base_cr; disc_cg[dc] = base_cg; disc_cb[dc] = base_cb;
                    dc++;
                }
            }
            // Top cap (full disc)
            let r_final = top_radius + p.capital_overhang;
            disc_ri[dc] = 0.0; disc_ro[dc] = r_final;
            disc_y[dc] = eff_h - p.burial; disc_ny[dc] = 1.0;
            disc_cr[dc] = base_cr; disc_cg[dc] = base_cg; disc_cb[dc] = base_cb;
            dc++;
        } else {
            // Simple top cap
            disc_ri[dc] = 0.0; disc_ro[dc] = top_radius;
            disc_y[dc] = eff_h - p.burial; disc_ny[dc] = 1.0;
            disc_cr[dc] = base_cr; disc_cg[dc] = base_cg; disc_cb[dc] = base_cb;
            dc++;
        }
    } else {
        // Antenna: disc faces at every drum shoulder (post↔drum transitions)
        // + top cap + bottom cap
        let post_r = p.shaft_radius;
        let drum_count_d = min(bl, 3u);
        let drum_h_d = p.base_height;
        let drum_ovh_d = p.base_overhang;
        let spacer_h_d = p.capital_height;
        let drum_taper_d = p.taper;
        let content_h_d = f32(drum_count_d) * drum_h_d + f32(max(drum_count_d, 1u) - 1u) * spacer_h_d;
        let drum_start_y_d = p.height - content_h_d;

        for (var d = 0u; d < drum_count_d; d++) {
            let dy_base_d = drum_start_y_d + f32(d) * (drum_h_d + spacer_h_d);
            let bottom_r_d = post_r + drum_ovh_d * (0.6 + 0.4 * fract(f32(d) * 0.618 + f32(p.segs_around) * 0.1));
            let top_r_d = bottom_r_d * drum_taper_d;
            let d_cr = drum_cr[d];
            let d_cg = drum_cg[d];
            let d_cb = drum_cb[d];

            // Bottom shoulder: annular disc facing up (post_r → bottom_r) at drum base
            if (dc < 12u) {
                disc_ri[dc] = post_r; disc_ro[dc] = bottom_r_d;
                disc_y[dc] = dy_base_d - p.burial; disc_ny[dc] = 1.0;
                disc_cr[dc] = d_cr; disc_cg[dc] = d_cg; disc_cb[dc] = d_cb;
                dc++;
            }
            // Top shoulder: annular disc facing down (post_r → top_r) at drum top
            if (dc < 12u) {
                disc_ri[dc] = post_r; disc_ro[dc] = top_r_d;
                disc_y[dc] = dy_base_d + drum_h_d - p.burial; disc_ny[dc] = -1.0;
                disc_cr[dc] = d_cr; disc_cg[dc] = d_cg; disc_cb[dc] = d_cb;
                dc++;
            }
        }

        // Top cap (post tip)
        if (dc < 12u) {
            disc_ri[dc] = 0.0; disc_ro[dc] = post_r;
            disc_y[dc] = p.height - p.burial; disc_ny[dc] = 1.0;
            disc_cr[dc] = base_cr; disc_cg[dc] = base_cg; disc_cb[dc] = base_cb;
            dc++;
        }
    }

    // Bottom cap (full disc, normal down) — all tiers
    let r_bottom = prof_r[0];
    disc_ri[dc] = 0.0; disc_ro[dc] = r_bottom;
    disc_y[dc] = -p.burial; disc_ny[dc] = -1.0;
    disc_cr[dc] = base_cr; disc_cg[dc] = base_cg; disc_cb[dc] = base_cb;
    dc++;

    // ── Precompute sin/cos table (shared by revolution + all discs) ──
    // sa+1 entries: one per angular step including the wrap-around.
    const CMG_MAX_SA: u32 = 29u;  // max segs_around + 1 (ornate: 28+1)
    var tbl_cos: array<f32, 29>;
    var tbl_sin: array<f32, 29>;
    for (var si = 0u; si <= sa; si++) {
        let theta = 2.0 * CMG_PI * f32(si) / f32(sa);
        tbl_cos[si] = cos(theta);
        tbl_sin[si] = sin(theta);
    }

    // ── Revolution: rotate profile around Y axis ───────────────
    var vi = slot_vb;
    var ii = slot_ib;

    // Vertices: (sa+1) rings × pc points
    for (var si = 0u; si <= sa; si++) {
        let ct = tbl_cos[si];
        let st = tbl_sin[si];

        for (var pi = 0u; pi < pc; pi++) {
            let r = prof_r[pi];
            let y = prof_y[pi];

            // Normal from profile slope (finite difference)
            var dy = 0.0;
            var dr = 0.0;
            if (pi + 1u < pc) {
                dy = prof_y[pi + 1u] - prof_y[pi];
                dr = prof_r[pi + 1u] - prof_r[pi];
            } else if (pi > 0u) {
                dy = prof_y[pi] - prof_y[pi - 1u];
                dr = prof_r[pi] - prof_r[pi - 1u];
            }
            let nlen = sqrt(dy * dy + dr * dr);
            var nx_local = 1.0;
            var ny_local = 0.0;
            if (nlen > 0.001) {
                nx_local = dy / nlen;
                ny_local = -dr / nlen;
            }

            cmg_write_vertex(vi,
                p.center_x + r * ct, y, p.center_z + r * st,
                nx_local * ct, ny_local, nx_local * st,
                prof_cr[pi], prof_cg[pi], prof_cb[pi], slot);
            vi++;
        }
    }

    // Wall indices: quads between adjacent rings
    let rev_vb = slot_vb;
    for (var si = 0u; si < sa; si++) {
        for (var pi = 0u; pi + 1u < pc; pi++) {
            let i00 = rev_vb + si * pc + pi;
            let i10 = i00 + 1u;
            let i01 = rev_vb + (si + 1u) * pc + pi;
            let i11 = i01 + 1u;
            cmg_indices[ii] = i00; ii++;
            cmg_indices[ii] = i01; ii++;
            cmg_indices[ii] = i10; ii++;
            cmg_indices[ii] = i10; ii++;
            cmg_indices[ii] = i01; ii++;
            cmg_indices[ii] = i11; ii++;
        }
    }

    // ── Horizontal discs (using precomputed trig table) ────────

    for (var di = 0u; di < dc; di++) {
        let d_ri = disc_ri[di];
        let d_ro = disc_ro[di];
        let d_y = disc_y[di];
        let d_ny = disc_ny[di];
        let d_cr = disc_cr[di];
        let d_cg = disc_cg[di];
        let d_cb = disc_cb[di];

        if (d_ri < 0.001) {
            // Full disc: center vertex + outer ring → triangle fan
            let center_vi = vi;
            cmg_write_vertex(vi,
                p.center_x, d_y, p.center_z,
                0.0, d_ny, 0.0,
                d_cr, d_cg, d_cb, slot);
            vi++;

            for (var si = 0u; si <= sa; si++) {
                cmg_write_vertex(vi,
                    p.center_x + d_ro * tbl_cos[si], d_y, p.center_z + d_ro * tbl_sin[si],
                    0.0, d_ny, 0.0,
                    d_cr, d_cg, d_cb, slot);
                vi++;
            }

            for (var si = 0u; si < sa; si++) {
                if (d_ny > 0.0) {
                    cmg_indices[ii] = center_vi;          ii++;
                    cmg_indices[ii] = center_vi + 1u + si; ii++;
                    cmg_indices[ii] = center_vi + 2u + si; ii++;
                } else {
                    cmg_indices[ii] = center_vi;          ii++;
                    cmg_indices[ii] = center_vi + 2u + si; ii++;
                    cmg_indices[ii] = center_vi + 1u + si; ii++;
                }
            }
        } else {
            // Annular ring: inner + outer ring → triangle strip
            let ring_vb = vi;
            for (var si = 0u; si <= sa; si++) {
                let ct = tbl_cos[si];
                let st = tbl_sin[si];

                cmg_write_vertex(vi,
                    p.center_x + d_ri * ct, d_y, p.center_z + d_ri * st,
                    0.0, d_ny, 0.0,
                    d_cr, d_cg, d_cb, slot);
                vi++;

                cmg_write_vertex(vi,
                    p.center_x + d_ro * ct, d_y, p.center_z + d_ro * st,
                    0.0, d_ny, 0.0,
                    d_cr, d_cg, d_cb, slot);
                vi++;
            }

            for (var si = 0u; si < sa; si++) {
                let in0 = ring_vb + si * 2u;
                let out0 = in0 + 1u;
                let in1 = ring_vb + (si + 1u) * 2u;
                let out1 = in1 + 1u;
                if (d_ny > 0.0) {
                    cmg_indices[ii] = in0;  ii++;
                    cmg_indices[ii] = out0; ii++;
                    cmg_indices[ii] = in1;  ii++;
                    cmg_indices[ii] = in1;  ii++;
                    cmg_indices[ii] = out0; ii++;
                    cmg_indices[ii] = out1; ii++;
                } else {
                    cmg_indices[ii] = in0;  ii++;
                    cmg_indices[ii] = in1;  ii++;
                    cmg_indices[ii] = out0; ii++;
                    cmg_indices[ii] = in1;  ii++;
                    cmg_indices[ii] = out1; ii++;
                    cmg_indices[ii] = out0; ii++;
                }
            }
        }
    }

    // ── Zero remaining indices ─────────────────────────────────
    let used = ii - slot_ib;
    for (var i = used; i < CMG_MAX_INDICES_PER_SLOT; i++) {
        cmg_indices[slot_ib + i] = slot_vb;
    }
}


// ─── §9.3 PALM MESH GENERATION (trunk + radial frond crown) ──────────────
//
// PalmMeshParams MUST match state.hpp::GPUPalmMeshParams (size: 128 bytes).
// If this struct gains/loses a field, the CPU side and
// its state.hpp sizeof static_assert must be updated together.

struct PalmMeshParams {
    center_x: f32, center_z: f32,
    height: f32,
    base_r: f32, top_r: f32,
    lean: f32, lean_dir: f32,
    bark_rings: f32, bark_depth: f32,
    frond_count: f32,
    frond_len: f32, frond_width: f32,
    frond_droop: f32, frond_arch: f32,
    crown_spread: f32, crown_skirt: f32,
    burial: f32,
    trunk_r: f32, trunk_g: f32, trunk_b: f32,
    frond_r: f32, frond_g: f32, frond_b: f32,
    aged_r: f32, aged_g: f32, aged_b: f32,
    trunk_segs: u32, frond_segs: u32,
    is_active: u32,
    _pad0: f32,
    _pad1: f32,
    _pad2: f32,
}

const PALMG_MAX_VERTS_PER_SLOT: u32 = 1200u;
const PALMG_MAX_INDICES_PER_SLOT: u32 = 6000u;
const PALMG_FLOATS_PER_VERTEX: u32 = 10u;
const PALMG_MAX_SLOTS: u32 = 24u;
const PALMG_TOTAL_INDICES: u32 = 144000u;

@group(0) @binding(180) var<storage, read>       palmg_params: array<PalmMeshParams, 24>;
@group(0) @binding(181) var<storage, read_write>  palmg_vertices: array<f32>;
@group(0) @binding(182) var<storage, read_write>  palmg_indices: array<u32>;

fn palmg_write_vertex(abs_idx: u32, px: f32, py: f32, pz: f32,
                      nx: f32, ny: f32, nz: f32,
                      cr: f32, cg: f32, cb: f32, entity_idx: u32) {
    let base = abs_idx * PALMG_FLOATS_PER_VERTEX;
    palmg_vertices[base + 0u] = px;
    palmg_vertices[base + 1u] = py;
    palmg_vertices[base + 2u] = pz;
    palmg_vertices[base + 3u] = nx;
    palmg_vertices[base + 4u] = ny;
    palmg_vertices[base + 5u] = nz;
    palmg_vertices[base + 6u] = cr;
    palmg_vertices[base + 7u] = cg;
    palmg_vertices[base + 8u] = cb;
    palmg_vertices[base + 9u] = f32(entity_idx);
}

@compute @workgroup_size(1, 1, 1)
fn palm_mesh_gen(@builtin(global_invocation_id) gid: vec3<u32>) {
    let slot = gid.x;
    if (slot >= PALMG_MAX_SLOTS) { return; }

    let p = palmg_params[slot];
    let vb_base = slot * PALMG_MAX_VERTS_PER_SLOT;
    let ib_base = slot * PALMG_MAX_INDICES_PER_SLOT;

    if (p.is_active == 0u) {
        for (var i = 0u; i < PALMG_MAX_INDICES_PER_SLOT; i++) {
            palmg_indices[ib_base + i] = vb_base;
        }
        return;
    }

    var vi = 0u;
    var ii = 0u;

    let cx = p.center_x;
    let cz = p.center_z;
    let burial = p.burial;
    let lean_cos = cos(p.lean_dir);
    let lean_sin = sin(p.lean_dir);

    // ── TRUNK: surface of revolution with taper + bark rings + lean ──

    let trunk_rings = min(u32(max(8.0, p.bark_rings)), 40u);
    let trunk_segs = min(p.trunk_segs, 24u);

    for (var ring = 0u; ring <= trunk_rings; ring++) {
        let t = f32(ring) / f32(trunk_rings);

        var r = p.base_r + (p.top_r - p.base_r) * t;
        let ring_phase = t * p.bark_rings * 2.0 * PI;
        r += sin(ring_phase) * p.bark_depth * (1.0 - t * 0.5);
        r = max(0.01, r);

        let lean_mag = p.lean * p.height * t * t;
        let lean_x = lean_mag * lean_cos;
        let lean_z = lean_mag * lean_sin;
        let y = t * p.height - burial;

        let shade = 0.85 + 0.15 * t;
        let cr = p.trunk_r * shade;
        let cg = p.trunk_g * shade;
        let cb = p.trunk_b * shade;

        for (var seg = 0u; seg < trunk_segs; seg++) {
            let angle = f32(seg) / f32(trunk_segs) * 2.0 * PI;
            let ca = cos(angle);
            let sa = sin(angle);

            palmg_write_vertex(vb_base + vi,
                cx + lean_x + ca * r, y, cz + lean_z + sa * r,
                ca, 0.0, sa,
                cr, cg, cb, slot);
            vi++;
        }
    }

    // Trunk indices: quads between consecutive rings
    for (var ring = 0u; ring < trunk_rings; ring++) {
        for (var seg = 0u; seg < trunk_segs; seg++) {
            let next_seg = (seg + 1u) % trunk_segs;
            let row0 = ring * trunk_segs;
            let row1 = (ring + 1u) * trunk_segs;

            let v00 = vb_base + row0 + seg;
            let v10 = vb_base + row1 + seg;
            let v11 = vb_base + row1 + next_seg;
            let v01 = vb_base + row0 + next_seg;

            palmg_indices[ib_base + ii] = v00; ii++;
            palmg_indices[ib_base + ii] = v10; ii++;
            palmg_indices[ib_base + ii] = v11; ii++;
            palmg_indices[ib_base + ii] = v00; ii++;
            palmg_indices[ib_base + ii] = v11; ii++;
            palmg_indices[ib_base + ii] = v01; ii++;
        }
    }

    // ── CROWN CAP: triangle fan at trunk top ──

    let top_lean_mag = p.lean * p.height;
    let crown_lean_x = top_lean_mag * lean_cos;
    let crown_lean_z = top_lean_mag * lean_sin;
    let crown_y = p.height - burial;
    let crown_r = p.top_r * 1.3;

    let crown_cr = p.trunk_r * 0.6 + p.frond_r * 0.4;
    let crown_cg = p.trunk_g * 0.6 + p.frond_g * 0.4;
    let crown_cb = p.trunk_b * 0.6 + p.frond_b * 0.4;

    let cap_tip_vi = vi;
    palmg_write_vertex(vb_base + vi,
        cx + crown_lean_x, crown_y + crown_r * 0.6, cz + crown_lean_z,
        0.0, 1.0, 0.0,
        crown_cr, crown_cg, crown_cb, slot);
    vi++;

    let cap_ring_vi = vi;
    for (var seg = 0u; seg < trunk_segs; seg++) {
        let angle = f32(seg) / f32(trunk_segs) * 2.0 * PI;
        palmg_write_vertex(vb_base + vi,
            cx + crown_lean_x + cos(angle) * crown_r,
            crown_y,
            cz + crown_lean_z + sin(angle) * crown_r,
            0.0, 1.0, 0.0,
            crown_cr, crown_cg, crown_cb, slot);
        vi++;
    }

    for (var seg = 0u; seg < trunk_segs; seg++) {
        let next_seg = (seg + 1u) % trunk_segs;
        palmg_indices[ib_base + ii] = vb_base + cap_tip_vi; ii++;
        palmg_indices[ib_base + ii] = vb_base + cap_ring_vi + seg; ii++;
        palmg_indices[ib_base + ii] = vb_base + cap_ring_vi + next_seg; ii++;
    }

    // ── FRONDS: radial quad strips with golden-angle packing ──

    let golden_angle = PI * (3.0 - sqrt(5.0));
    let n_fronds = min(u32(max(3.0, p.frond_count)), 18u);
    let frond_segs = min(p.frond_segs, 14u);
    let crown_frond_y = crown_y + crown_r * 0.3;

    for (var f = 0u; f < n_fronds; f++) {
        let base_angle = f32(f) * golden_angle;
        let rank = f32(f) / max(1.0, f32(n_fronds - 1u));

        let elev_top = p.crown_spread * PI * 0.42;
        let elev_bot = -p.crown_skirt * PI * 0.25;
        let elevation = elev_bot + (elev_top - elev_bot) * rank;

        let droop_scale = 0.25 + 0.75 * (1.0 - rank);
        let len_scale = 0.6 + 0.4 * (1.0 - rank);

        let cos_az = cos(base_angle);
        let sin_az = sin(base_angle);
        let cos_el = cos(elevation);
        let sin_el = sin(elevation);
        let fwd_x = cos_az * cos_el;
        let fwd_y = sin_el;
        let fwd_z = sin_az * cos_el;

        // Right vector: cross(forward, up)
        var right_x: f32; var right_y: f32; var right_z: f32;
        if (sin_el > 0.95) {
            right_x = -sin_az; right_y = 0.0; right_z = cos_az;
        } else {
            let rx = fwd_z; let rz = -fwd_x;
            let rl = sqrt(rx * rx + rz * rz);
            if (rl > 0.001) {
                right_x = rx / rl; right_y = 0.0; right_z = rz / rl;
            } else {
                right_x = -sin_az; right_y = 0.0; right_z = cos_az;
            }
        }

        let frond_len = p.frond_len * len_scale;
        let frond_vi_start = vi;

        for (var s = 0u; s <= frond_segs; s++) {
            let t = f32(s) / f32(frond_segs);
            let dist = t * frond_len;

            let arch_up = p.frond_arch * frond_len * sin(t * PI * 0.4);
            let droop_down = p.frond_droop * frond_len * t * t * t * droop_scale;
            let dy = arch_up - droop_down;

            let mx = cx + crown_lean_x + fwd_x * dist;
            let my = crown_frond_y + fwd_y * dist + dy;
            let mz = cz + crown_lean_z + fwd_z * dist;

            let w = p.frond_width * (1.0 - t * 0.85) * (0.3 + 0.7 * min(1.0, t * 3.0));
            let half_w = w * 0.5;
            let px_off = right_x * half_w;
            let py_off = right_y * half_w;
            let pz_off = right_z * half_w;

            // Color: blend young→aged by rank + tip aging
            let aged_blend = rank;
            let fr = p.aged_r + (p.frond_r - p.aged_r) * aged_blend;
            let fg = p.aged_g + (p.frond_g - p.aged_g) * aged_blend;
            let fb = p.aged_b + (p.frond_b - p.aged_b) * aged_blend;
            let tip_age = min(1.0, t * t * (1.0 - rank * 0.7)) * 0.3;
            let seg_r = fr + (p.aged_r - fr) * tip_age;
            let seg_g = fg + (p.aged_g - fg) * tip_age;
            let seg_b = fb + (p.aged_b - fb) * tip_age;
            let seg_shade = 0.75 + 0.25 * t;
            let frond_var = f32(f % 3u) * 0.03;

            let ny_approx = 0.8;
            let nx_approx = fwd_x * 0.2;
            let nz_approx = fwd_z * 0.2;

            // Left vertex
            palmg_write_vertex(vb_base + vi,
                mx + px_off, my + py_off, mz + pz_off,
                nx_approx, ny_approx, nz_approx,
                min(1.0, seg_r * seg_shade - frond_var * 0.5),
                min(1.0, seg_g * seg_shade + frond_var),
                min(1.0, seg_b * seg_shade - frond_var * 0.5),
                slot);
            vi++;

            // Right vertex
            palmg_write_vertex(vb_base + vi,
                mx - px_off, my - py_off, mz - pz_off,
                -nx_approx, ny_approx, -nz_approx,
                min(1.0, seg_r * seg_shade - frond_var * 0.5),
                min(1.0, seg_g * seg_shade + frond_var),
                min(1.0, seg_b * seg_shade - frond_var * 0.5),
                slot);
            vi++;
        }

        // Quad strip indices for this frond
        for (var s = 0u; s < frond_segs; s++) {
            let base_v = vb_base + frond_vi_start + s * 2u;
            let left0  = base_v;
            let right0 = base_v + 1u;
            let left1  = base_v + 2u;
            let right1 = base_v + 3u;

            palmg_indices[ib_base + ii] = left0;  ii++;
            palmg_indices[ib_base + ii] = left1;  ii++;
            palmg_indices[ib_base + ii] = right1; ii++;
            palmg_indices[ib_base + ii] = left0;  ii++;
            palmg_indices[ib_base + ii] = right1; ii++;
            palmg_indices[ib_base + ii] = right0; ii++;
        }
    }

    // Zero remaining indices (degenerate padding)
    for (var i = ii; i < PALMG_MAX_INDICES_PER_SLOT; i++) {
        palmg_indices[ib_base + i] = vb_base;
    }
}

// ─── Palm vertex shaders ─────────────────────────────────────────────

@vertex
fn palm_vs(in: ArchVertexInput) -> EntityVarying {
    let idx = u32(in.arch_index);
    let ground_y = textureLoad(entity_ground_atlas, vec2<i32>(i32(idx) + GROUND_ATLAS_PALM, 0), 0).r;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += sample_live_card(world_pos.xz).x;
    var out: EntityVarying;
    out.clip_pos = render_vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.normal = in.normal;
    out.entity_color = in.color;
    // THE RING (draw authority) — flora's first draw gate: per-vertex kill
    // beyond the ring (the mesh is baked world-space, no instance channel).
    // Anchor = the STAGED point (the band's yardstick). The boundary sits
    // where the icing is 1, so any mixed-triangle clip sliver is invisible.
    if (distance(world_pos.xz, vec2(config.lod_point_x, config.lod_point_z)) > config.veil_ring) {
        out.clip_pos = vec4(0.0, 0.0, -1e4, 1.0);   // far behind the near plane — clipped
    }
    return out;
}

@vertex
fn shadow_palm_vs(in: ArchVertexInput) -> ShadowVarying {
    let idx = u32(in.arch_index);
    let ground_y = textureLoad(entity_ground_atlas, vec2<i32>(i32(idx) + GROUND_ATLAS_PALM, 0), 0).r;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += sample_live_card(world_pos.xz).x;
    var out: ShadowVarying;
    out.clip_pos = render_vp.light_vp * vec4(world_pos, 1.0);
    return out;
}

// ─── §9.4 CACTUS MESH GENERATION (ribbed columnar trunk + forking arms) ──
//
// CactusMeshParams MUST match state.hpp::GPUCactusMeshParams (size: 128 bytes).
// If this struct gains/loses a field, the CPU side and
// its state.hpp sizeof static_assert must be updated together.

// 21 floats + 4 uint32_t + 7 pad floats = 32 fields × 4 = 128 bytes
struct CactusMeshParams {
    center_x: f32, center_z: f32,              // 1-2
    height: f32, radius: f32, taper: f32,      // 3-5
    ribs: f32, rib_depth: f32,                 // 6-7
    lean: f32, lean_dir: f32,                  // 8-9
    cap_round: f32,                            // 10
    arm_count: f32,                            // 11
    arm_height: f32, arm_length: f32, arm_radius: f32,  // 12-14
    arm_curve: f32,                            // 15
    body_r: f32, body_g: f32, body_b: f32,     // 16-18
    rib_r: f32, rib_g: f32, rib_b: f32,        // 19-21
    trunk_segs: u32, arm_segs: u32,            // 22-23
    is_active: u32,                            // 24
    seed: u32,                                 // 25
    _pad0: f32, _pad1: f32, _pad2: f32, _pad3: f32, _pad4: f32, _pad5: f32, _pad6: f32,  // 26-32 = 128 bytes
}

const CACTUSG_MAX_VERTS_PER_SLOT: u32 = 1500u;
const CACTUSG_MAX_INDICES_PER_SLOT: u32 = 7998u;
const CACTUSG_FLOATS_PER_VERTEX: u32 = 10u;
const CACTUSG_MAX_SLOTS: u32 = 20u;

@group(0) @binding(183) var<storage, read>       cactusg_params: array<CactusMeshParams, 20>;
@group(0) @binding(184) var<storage, read_write>  cactusg_vertices: array<f32>;
@group(0) @binding(185) var<storage, read_write>  cactusg_indices: array<u32>;

fn cactusg_write_vertex(abs_idx: u32, px: f32, py: f32, pz: f32,
                        nx: f32, ny: f32, nz: f32,
                        cr: f32, cg: f32, cb: f32, entity_idx: u32) {
    let base = abs_idx * CACTUSG_FLOATS_PER_VERTEX;
    cactusg_vertices[base + 0u] = px;
    cactusg_vertices[base + 1u] = py;
    cactusg_vertices[base + 2u] = pz;
    cactusg_vertices[base + 3u] = nx;
    cactusg_vertices[base + 4u] = ny;
    cactusg_vertices[base + 5u] = nz;
    cactusg_vertices[base + 6u] = cr;
    cactusg_vertices[base + 7u] = cg;
    cactusg_vertices[base + 8u] = cb;
    cactusg_vertices[base + 9u] = f32(entity_idx);
}

fn cactus_hash(seed: u32, prop: u32) -> f32 {
    var h = seed * 747796405u + prop * 2891336453u + 1u;
    h = ((h >> 16u) ^ h) * 2654435769u;
    h = ((h >> 16u) ^ h) * 2654435769u;
    h = (h >> 16u) ^ h;
    return f32(h) / 4294967295.0;
}

@compute @workgroup_size(1, 1, 1)
fn cactus_mesh_gen(@builtin(global_invocation_id) gid: vec3<u32>) {
    let slot = gid.x;
    if (slot >= CACTUSG_MAX_SLOTS) { return; }

    let p = cactusg_params[slot];
    let vb_base = slot * CACTUSG_MAX_VERTS_PER_SLOT;
    let ib_base = slot * CACTUSG_MAX_INDICES_PER_SLOT;

    if (p.is_active == 0u) {
        for (var i = 0u; i < CACTUSG_MAX_INDICES_PER_SLOT; i++) {
            cactusg_indices[ib_base + i] = vb_base;
        }
        return;
    }

    var vi = 0u;
    var ii = 0u;

    let cx = p.center_x;
    let cz = p.center_z;
    let lean_cos = cos(p.lean_dir);
    let lean_sin = sin(p.lean_dir);

    let ribs = u32(max(4.0, p.ribs));
    let around = min(max(ribs * 2u, 12u), 20u);
    let trunk_steps = min(u32(p.trunk_segs), 20u);

    // ── TRUNK: ribbed surface of revolution ──

    for (var ring = 0u; ring <= trunk_steps; ring++) {
        let t = f32(ring) / f32(trunk_steps);
        let r_base = p.radius * (1.0 + (p.taper - 1.0) * t);

        let cap_start = 1.0 - p.cap_round * 0.3;
        let cap_t = max(0.0, (t - cap_start) / (p.cap_round * 0.3 + 0.001));
        let cap_scale = select(1.0, cos(cap_t * PI * 0.5), cap_t > 0.0);

        let lean_mag = p.lean * p.height * t * t;
        let lx = lean_mag * lean_cos;
        let lz = lean_mag * lean_sin;
        let y = t * p.height;
        let shade = 0.85 + 0.15 * t;

        for (var seg = 0u; seg < around; seg++) {
            let angle = f32(seg) / f32(around) * 2.0 * PI;
            let rib_phase = angle * f32(ribs) / (2.0 * PI);
            let rib_mod = 1.0 + cos(rib_phase * 2.0 * PI) * p.rib_depth;
            let r = r_base * rib_mod * cap_scale;

            let ca = cos(angle);
            let sa = sin(angle);

            let rib_frac = (cos(rib_phase * 2.0 * PI) + 1.0) * 0.5;
            let cr = (p.body_r + (p.rib_r - p.body_r) * rib_frac * 0.6) * shade;
            let cg = (p.body_g + (p.rib_g - p.body_g) * rib_frac * 0.6) * shade;
            let cb = (p.body_b + (p.rib_b - p.body_b) * rib_frac * 0.6) * shade;

            cactusg_write_vertex(vb_base + vi,
                cx + lx + ca * r, y, cz + lz + sa * r,
                ca, 0.0, sa, cr, cg, cb, slot);
            vi++;
        }
    }

    // Trunk indices
    for (var ring = 0u; ring < trunk_steps; ring++) {
        for (var seg = 0u; seg < around; seg++) {
            let next_seg = (seg + 1u) % around;
            let row0 = ring * around;
            let row1 = (ring + 1u) * around;

            cactusg_indices[ib_base + ii] = vb_base + row0 + seg; ii++;
            cactusg_indices[ib_base + ii] = vb_base + row1 + seg; ii++;
            cactusg_indices[ib_base + ii] = vb_base + row1 + next_seg; ii++;
            cactusg_indices[ib_base + ii] = vb_base + row0 + seg; ii++;
            cactusg_indices[ib_base + ii] = vb_base + row1 + next_seg; ii++;
            cactusg_indices[ib_base + ii] = vb_base + row0 + next_seg; ii++;
        }
    }

    // ── TRUNK CAP (stitched to top ring) ──

    let top_ring_vi = trunk_steps * around;  // first vertex of trunk's last ring
    let top_lean = p.lean * p.height;
    let cap_cx = cx + top_lean * lean_cos;
    let cap_cz = cz + top_lean * lean_sin;
    let cap_y = p.height;
    let cap_r = p.radius * p.taper * 0.6;
    let cap_col_r = p.body_r * 0.6 + p.rib_r * 0.4;
    let cap_col_g = p.body_g * 0.6 + p.rib_g * 0.4;
    let cap_col_b = p.body_b * 0.6 + p.rib_b * 0.4;

    // Single tip vertex above center
    let cap_tip_vi = vi;
    cactusg_write_vertex(vb_base + vi,
        cap_cx, cap_y + cap_r * 0.6, cap_cz,
        0.0, 1.0, 0.0,
        cap_col_r, cap_col_g, cap_col_b, slot);
    vi++;

    // Fan from tip to trunk's existing top ring — no separate cap ring
    for (var seg = 0u; seg < around; seg++) {
        let next = (seg + 1u) % around;
        cactusg_indices[ib_base + ii] = vb_base + cap_tip_vi; ii++;
        cactusg_indices[ib_base + ii] = vb_base + top_ring_vi + seg; ii++;
        cactusg_indices[ib_base + ii] = vb_base + top_ring_vi + next; ii++;
    }

    // ── ARMS: ribbed columns along upward-curving paths ──

    let golden_angle = PI * (3.0 - sqrt(5.0));
    let n_arms = u32(max(0.0, p.arm_count));
    let arm_segs_u = min(u32(p.arm_segs), 12u);
    let arm_ribs = max(4u, ribs - 2u);
    let arm_around = min(max(arm_ribs * 2u, 8u), 12u);

    for (var a = 0u; a < n_arms; a++) {
        let arm_az = f32(a) * golden_angle + cactus_hash(p.seed, 1050u + a) * 0.5;
        let fork_frac = p.arm_height + (cactus_hash(p.seed, 1060u + a) - 0.5) * 0.15;
        let fork_y = p.height * fork_frac;
        let arm_len = p.arm_length * (0.8 + cactus_hash(p.seed, 1070u + a) * 0.4);
        let arm_r = p.arm_radius * (0.85 + cactus_hash(p.seed, 1080u + a) * 0.3);

        let lean_at_fork = p.lean * p.height * fork_frac * fork_frac;
        let fork_x = cx + cos(arm_az) * p.radius * p.taper * 0.9 + lean_at_fork * lean_cos * 0.3;
        let fork_z = cz + sin(arm_az) * p.radius * p.taper * 0.9 + lean_at_fork * lean_sin * 0.3;

        let out_x = cos(arm_az);
        let out_z = sin(arm_az);

        var apx = fork_x;
        var apy = fork_y;
        var apz = fork_z;
        let seg_len = arm_len / f32(arm_segs_u);

        let arm_vi_start = vi;

        for (var s = 0u; s <= arm_segs_u; s++) {
            let t = f32(s) / f32(arm_segs_u);
            let blend = t * p.arm_curve;
            let dx = out_x * (1.0 - blend);
            let dy = blend;
            let dz = out_z * (1.0 - blend);
            let dl = sqrt(dx * dx + dy * dy + dz * dz);
            let ndx = dx / max(dl, 0.001);
            let ndy = dy / max(dl, 0.001);
            let ndz = dz / max(dl, 0.001);

            let seg_r = arm_r * (1.0 - t * 0.3);

            var rx: f32; var rz: f32;
            if (abs(ndy) > 0.95) {
                rx = 1.0; rz = 0.0;
            } else {
                rx = ndz; rz = -ndx;
                let rl = sqrt(rx * rx + rz * rz);
                rx /= max(rl, 0.001);
                rz /= max(rl, 0.001);
            }
            let fx = 0.0 - rz * ndy;
            let fy = rz * ndx - rx * ndz;
            let fz = rx * ndy;

            let arm_shade = 0.85 + 0.15 * t;

            for (var seg = 0u; seg < arm_around; seg++) {
                let angle = f32(seg) / f32(arm_around) * 2.0 * PI;
                let ca = cos(angle);
                let sa = sin(angle);

                let arm_rib_phase = angle * f32(arm_ribs) / (2.0 * PI);
                let arm_rib_mod = 1.0 + cos(arm_rib_phase * 2.0 * PI) * p.rib_depth * 0.8;
                let r = seg_r * arm_rib_mod;

                let vx = apx + (rx * ca + fx * sa) * r;
                let vy = apy + fy * sa * r;
                let vz = apz + (rz * ca + fz * sa) * r;

                let arm_rib_frac = (cos(arm_rib_phase * 2.0 * PI) + 1.0) * 0.5;
                let cr = (p.body_r + (p.rib_r - p.body_r) * arm_rib_frac * 0.6) * arm_shade;
                let cg = (p.body_g + (p.rib_g - p.body_g) * arm_rib_frac * 0.6) * arm_shade;
                let cb = (p.body_b + (p.rib_b - p.body_b) * arm_rib_frac * 0.6) * arm_shade;

                cactusg_write_vertex(vb_base + vi,
                    vx, vy, vz, ca, 0.0, sa, cr, cg, cb, slot);
                vi++;
            }

            if (s < arm_segs_u) {
                apx += ndx * seg_len;
                apy += ndy * seg_len;
                apz += ndz * seg_len;
            }
        }

        // Arm indices
        for (var s = 0u; s < arm_segs_u; s++) {
            for (var seg = 0u; seg < arm_around; seg++) {
                let next_seg = (seg + 1u) % arm_around;
                let row0 = arm_vi_start + s * arm_around;
                let row1 = arm_vi_start + (s + 1u) * arm_around;

                cactusg_indices[ib_base + ii] = vb_base + row0 + seg; ii++;
                cactusg_indices[ib_base + ii] = vb_base + row1 + seg; ii++;
                cactusg_indices[ib_base + ii] = vb_base + row1 + next_seg; ii++;
                cactusg_indices[ib_base + ii] = vb_base + row0 + seg; ii++;
                cactusg_indices[ib_base + ii] = vb_base + row1 + next_seg; ii++;
                cactusg_indices[ib_base + ii] = vb_base + row0 + next_seg; ii++;
            }
        }

        // Arm cap
        let arm_cap_r = arm_r * 0.6;
        let arm_cap_tip = vi;
        cactusg_write_vertex(vb_base + vi,
            apx, apy + arm_cap_r * 0.6, apz,
            0.0, 1.0, 0.0,
            cap_col_r, cap_col_g, cap_col_b, slot);
        vi++;

        let arm_cap_ring = vi;
        let arm_cap_segs = min(arm_around, 8u);
        for (var seg = 0u; seg < arm_cap_segs; seg++) {
            let angle = f32(seg) / f32(arm_cap_segs) * 2.0 * PI;
            cactusg_write_vertex(vb_base + vi,
                apx + cos(angle) * arm_cap_r, apy, apz + sin(angle) * arm_cap_r,
                0.0, 1.0, 0.0,
                cap_col_r, cap_col_g, cap_col_b, slot);
            vi++;
        }

        for (var seg = 0u; seg < arm_cap_segs; seg++) {
            let next = (seg + 1u) % arm_cap_segs;
            cactusg_indices[ib_base + ii] = vb_base + arm_cap_tip; ii++;
            cactusg_indices[ib_base + ii] = vb_base + arm_cap_ring + seg; ii++;
            cactusg_indices[ib_base + ii] = vb_base + arm_cap_ring + next; ii++;
        }
    }

    // Zero remaining indices
    for (var i = ii; i < CACTUSG_MAX_INDICES_PER_SLOT; i++) {
        cactusg_indices[ib_base + i] = vb_base;
    }
}

// ─── Cactus vertex shaders ──────────────────────────────────────────

@vertex
fn cactus_vs(in: ArchVertexInput) -> EntityVarying {
    let idx = u32(in.arch_index);
    let ground_y = textureLoad(entity_ground_atlas, vec2<i32>(i32(idx) + GROUND_ATLAS_CACTUS, 0), 0).r;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += sample_live_card(world_pos.xz).x;
    var out: EntityVarying;
    out.clip_pos = render_vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.normal = in.normal;
    out.entity_color = in.color;
    // THE RING (draw authority) — see palm_vs: per-vertex kill beyond the ring.
    if (distance(world_pos.xz, vec2(config.lod_point_x, config.lod_point_z)) > config.veil_ring) {
        out.clip_pos = vec4(0.0, 0.0, -1e4, 1.0);
    }
    return out;
}

@vertex
fn shadow_cactus_vs(in: ArchVertexInput) -> ShadowVarying {
    let idx = u32(in.arch_index);
    let ground_y = textureLoad(entity_ground_atlas, vec2<i32>(i32(idx) + GROUND_ATLAS_CACTUS, 0), 0).r;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += sample_live_card(world_pos.xz).x;
    var out: ShadowVarying;
    out.clip_pos = render_vp.light_vp * vec4(world_pos, 1.0);
    return out;
}

// ─── §9.5 BLADE CLUSTER MESH GENERATION ─────────────────────────
//
// BladeClusterMeshParams MUST match state.hpp::GPUBladeClusterMeshParams
// (size: 80 bytes). If this struct gains/loses a field, the CPU side
// and its state.hpp sizeof static_assert must be updated together.

// 16 floats + 4 u32 = 20 fields × 4 = 80 bytes
struct BladeClusterMeshParams {
    center_x: f32, center_z: f32,                   // 1-2
    blade_count: f32,                                // 3
    blade_h: f32, blade_h_var: f32, blade_w: f32,   // 4-6
    splay: f32, curve: f32, twist: f32, taper: f32, // 7-10
    blade_r: f32, blade_g: f32, blade_b: f32,       // 11-13
    aged_r: f32, aged_g: f32, aged_b: f32,          // 14-16
    blade_segs: u32,                                 // 17
    is_active: u32,                                  // 18
    seed: u32,                                       // 19
    _pad0: u32,                                      // 20 = 80 bytes
}

const BLADEG_MAX_VERTS_PER_SLOT: u32 = 500u;
const BLADEG_MAX_INDICES_PER_SLOT: u32 = 1998u;
const BLADEG_FLOATS_PER_VERTEX: u32 = 10u;
const BLADEG_MAX_SLOTS: u32 = 32u;

@group(0) @binding(186) var<storage, read>       bladeg_params: array<BladeClusterMeshParams, 32>;
@group(0) @binding(187) var<storage, read_write>  bladeg_vertices: array<f32>;
@group(0) @binding(188) var<storage, read_write>  bladeg_indices: array<u32>;

fn bladeg_write_vertex(abs_idx: u32, px: f32, py: f32, pz: f32,
                       nx: f32, ny: f32, nz: f32,
                       cr: f32, cg: f32, cb: f32, entity_idx: u32) {
    let base = abs_idx * BLADEG_FLOATS_PER_VERTEX;
    bladeg_vertices[base + 0u] = px;
    bladeg_vertices[base + 1u] = py;
    bladeg_vertices[base + 2u] = pz;
    bladeg_vertices[base + 3u] = nx;
    bladeg_vertices[base + 4u] = ny;
    bladeg_vertices[base + 5u] = nz;
    bladeg_vertices[base + 6u] = cr;
    bladeg_vertices[base + 7u] = cg;
    bladeg_vertices[base + 8u] = cb;
    bladeg_vertices[base + 9u] = f32(entity_idx);
}

fn blade_hash(seed: u32, prop: u32) -> f32 {
    var h = seed * 747796405u + prop * 2891336453u + 1u;
    h = ((h >> 16u) ^ h) * 2654435769u;
    h = ((h >> 16u) ^ h) * 2654435769u;
    h = (h >> 16u) ^ h;
    return f32(h) / 4294967295.0;
}

@compute @workgroup_size(1)
fn blade_cluster_mesh_gen(@builtin(global_invocation_id) gid: vec3<u32>) {
    let slot = gid.x;
    if (slot >= BLADEG_MAX_SLOTS) { return; }

    let p = bladeg_params[slot];
    let vb_base = slot * BLADEG_MAX_VERTS_PER_SLOT;
    let ib_base = slot * BLADEG_MAX_INDICES_PER_SLOT;

    if (p.is_active == 0u) {
        for (var i = 0u; i < BLADEG_MAX_INDICES_PER_SLOT; i++) {
            bladeg_indices[ib_base + i] = vb_base;  // NOT 0u!
        }
        return;
    }

    var vi = 0u;
    var ii = 0u;

    let cx = p.center_x;
    let cz = p.center_z;
    let n_blades = u32(max(2.0, p.blade_count));
    let segs = max(3u, p.blade_segs);
    let GA = PI * (3.0 - sqrt(5.0));

    for (var b = 0u; b < n_blades; b++) {
        let azimuth = f32(b) * GA;
        let ca = cos(azimuth);
        let sa = sin(azimuth);

        // Per-blade height variation
        let h_mult = 1.0 + (blade_hash(p.seed, 970u + b) - 0.5) * p.blade_h_var * 2.0;
        let blade_h = p.blade_h * max(0.4, h_mult);

        // Splay: outer blades splay more
        let rank = f32(b) / max(1.0, f32(n_blades - 1u));
        let splay_ang = p.splay * (0.6 + 0.4 * (1.0 - rank));
        let splay_j = (blade_hash(p.seed, 980u + b) - 0.5) * 0.15;
        let final_splay = splay_ang + splay_j;

        let cos_s = cos(final_splay);
        let sin_s = sin(final_splay);
        let fwd_x = ca * sin_s;
        let fwd_y = cos_s;
        let fwd_z = sa * sin_s;

        // Right vector (perpendicular for blade width)
        var rx: f32; var ry: f32; var rz: f32;
        if (cos_s > 0.95) {
            rx = -sa; ry = 0.0; rz = ca;
        } else {
            // cross(fwd, up)
            rx = fwd_z; ry = 0.0; rz = -fwd_x;
            let rl = sqrt(rx * rx + rz * rz);
            rx /= max(rl, 0.001);
            rz /= max(rl, 0.001);
        }

        let twist_dir = select(-1.0, 1.0, b % 2u == 0u);
        let twist_amt = p.twist * twist_dir;

        // Base color for this blade
        let age_blend = (1.0 - rank) * 0.5;
        let base_r = p.blade_r + (p.aged_r - p.blade_r) * age_blend;
        let base_g = p.blade_g + (p.aged_g - p.blade_g) * age_blend;
        let base_b = p.blade_b + (p.aged_b - p.blade_b) * age_blend;

        let blade_vi_start = vi;

        // Two vertices per segment step (left + right of midrib)
        for (var s = 0u; s <= segs; s++) {
            let t = f32(s) / f32(segs);
            let dist = t * blade_h;

            // Curve: quadratic outward arc
            let curve_off = p.curve * blade_h * t * t;

            // Position along forward + curve
            let px = fwd_x * dist + ca * curve_off;
            let py = fwd_y * dist;
            let pz = fwd_z * dist + sa * curve_off;

            // Width: ramp in at root, taper to point
            let base_frac = 0.3 + 0.7 * min(1.0, t * 4.0);
            let tip_frac = 1.0 - pow(t, p.taper * 2.5 + 0.5);
            let w = p.blade_w * base_frac * tip_frac;

            // Twist
            let tw_angle = twist_amt * t * PI;
            let ct = cos(tw_angle);
            let st_tw = sin(tw_angle);
            let trx = rx * ct + fwd_x * st_tw;
            let try_ = ry * ct + fwd_y * st_tw;
            let trz = rz * ct + fwd_z * st_tw;

            let half_w = w * 0.5;
            let perp_x = trx * half_w;
            let perp_y = try_ * half_w;
            let perp_z = trz * half_w;

            // Color: shade by height, age at tip
            let shade = 0.7 + 0.3 * sin(t * PI * 0.8);
            let tip_age = t * t * 0.3;
            let cr = min(1.0, (base_r + (p.aged_r - base_r) * tip_age) * shade);
            let cg = min(1.0, (base_g + (p.aged_g - base_g) * tip_age) * shade);
            let cb = min(1.0, (base_b + (p.aged_b - base_b) * tip_age) * shade);

            // Normal: blade face normal (cross of forward and right)
            let nx = fwd_y * trz - fwd_z * try_;
            let ny = fwd_z * trx - fwd_x * trz;
            let nz = fwd_x * try_ - fwd_y * trx;
            let nl = sqrt(nx * nx + ny * ny + nz * nz);
            let nnx = nx / max(nl, 0.001);
            let nny = ny / max(nl, 0.001);
            let nnz = nz / max(nl, 0.001);

            // Left vertex
            bladeg_write_vertex(vb_base + vi,
                cx + px + perp_x, py + perp_y, cz + pz + perp_z,
                nnx, nny, nnz, cr, cg, cb, slot);
            vi++;

            // Right vertex
            bladeg_write_vertex(vb_base + vi,
                cx + px - perp_x, py - perp_y, cz + pz - perp_z,
                -nnx, -nny, -nnz, cr, cg, cb, slot);
            vi++;
        }

        // Index the quad strip: 2 tris per segment
        let vps = 2u;  // verts per step (left + right)
        for (var s = 0u; s < segs; s++) {
            let i0 = blade_vi_start + s * vps;       // left  row s
            let i1 = blade_vi_start + s * vps + 1u;  // right row s
            let i2 = blade_vi_start + (s + 1u) * vps;      // left  row s+1
            let i3 = blade_vi_start + (s + 1u) * vps + 1u; // right row s+1

            bladeg_indices[ib_base + ii] = vb_base + i0; ii++;
            bladeg_indices[ib_base + ii] = vb_base + i2; ii++;
            bladeg_indices[ib_base + ii] = vb_base + i3; ii++;
            bladeg_indices[ib_base + ii] = vb_base + i0; ii++;
            bladeg_indices[ib_base + ii] = vb_base + i3; ii++;
            bladeg_indices[ib_base + ii] = vb_base + i1; ii++;
        }
    }

    // Fill remaining indices with vb_base (NOT 0u!)
    for (var i = ii; i < BLADEG_MAX_INDICES_PER_SLOT; i++) {
        bladeg_indices[ib_base + i] = vb_base;
    }
}

// ─── Blade cluster vertex shaders ──────────────────────────────────

@vertex
fn blade_cluster_vs(in: ArchVertexInput) -> EntityVarying {
    let idx = u32(in.arch_index);
    let ground_y = textureLoad(entity_ground_atlas, vec2<i32>(i32(idx) + GROUND_ATLAS_BLADE, 0), 0).r;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += sample_live_card(world_pos.xz).x;
    var out: EntityVarying;
    out.clip_pos = render_vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.normal = in.normal;
    out.entity_color = in.color;
    // THE RING (draw authority) — see palm_vs: per-vertex kill beyond the ring.
    if (distance(world_pos.xz, vec2(config.lod_point_x, config.lod_point_z)) > config.veil_ring) {
        out.clip_pos = vec4(0.0, 0.0, -1e4, 1.0);
    }
    return out;
}

@vertex
fn shadow_blade_cluster_vs(in: ArchVertexInput) -> ShadowVarying {
    let idx = u32(in.arch_index);
    let ground_y = textureLoad(entity_ground_atlas, vec2<i32>(i32(idx) + GROUND_ATLAS_BLADE, 0), 0).r;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += sample_live_card(world_pos.xz).x;
    var out: ShadowVarying;
    out.clip_pos = render_vp.light_vp * vec4(world_pos, 1.0);
    return out;
}

// ═══════════════════════════════════════════════════════════════════
// §ORB — Sky orb layer: init, dynamics, render
// ═══════════════════════════════════════════════════════════════════
//
// Luminous points on a dome (sphere shell) above the world. A fixed
// population of billboarded quads driven by compute kernels and
// rendered additively into the main pass.
//
// Lifecycle:
//   orb_init            — one-shot: seed → dome position, palette-
//                         sampled color, tier-sampled physics
//   orb_recolor         — palette cycle: re-sample colors, keep
//                         positions/velocities/twinkle
//   orb_state_prev_copy — per-frame: snapshot state for neighbor
//                         queries (needed by flocking)
//   orb_dynamics        — per-frame: rule dispatch + couplings
//   orb_vs / orb_fs     — billboard draw into main render pass
//
// Motion rules (dispatched by orb_config.motion_rule):
//   0 Brownian  — drag, noise impulse, radial force
//   1 Orbital   — seed-derived great-circle paths, drag on perturbation
//   2 Frozen    — drag-to-zero, dome rotation only
//   3 Flocking  — neighbor-based boids with per-force signs
//
// Musical couplings — held at neutral (configure_orbs zeros them, no upload):
//   force_radial          polyphony → expansion force
//   noise_amp             polyphony → noise ceiling lerp
//   color_pulse/converge/surge   polyphony → three color trajectories
//   flock_coupling_intensity     polyphony → flock tightening
//
// Bind group topology:
//   Compute (orb_init, orb_dynamics, orb_recolor) — main layout:
//     @binding(410) orb_state         storage, read_write
//     @binding(411) orb_config        uniform
//     @binding(412) orb_state_prev    storage, read
//   Compute (orb_state_prev_copy) — copy layout, inverse access:
//     @binding(413) orb_state_ro      storage, read       (→ orb_state)
//     @binding(414) orb_state_prev_rw storage, read_write (→ orb_state_prev)
//   Render (orb_vs, orb_fs) — render_entity layout:
//     @binding(201) render_vp         (already declared)
//     @binding(280) render_camera     (already declared)
//     @binding(400) render_orb_state  storage, read

struct OrbState {
    pos:            vec3<f32>,
    _pad0:          f32,
    vel:            vec3<f32>,
    _pad1:          f32,
    base_color:     vec3<f32>,
    brightness:     f32,
    current_color:  vec3<f32>,
    twinkle_phase:  f32,
    size:           f32,
    mass:           f32,
    drag:           f32,
    tier_idx:       u32,
}

struct OrbConfig {
    // ── Base ─────────────────────────────────────────────────
    count:              u32,
    seed:               u32,
    base_hue:           f32,    // legacy; superseded by palette when palette_count > 0
    hue_variance:       f32,    // legacy
    brightness:         f32,
    drag:               f32,
    noise_amp:          f32,
    dome_radius:        f32,
    base_size:          f32,
    dt:                 f32,
    t_seconds:          f32,
    // DRIVERLESS since gen-1 retirement (force/color/flock/speed coupling
    // inputs) — held at neutral by configure_orbs' config-site zeros;
    // revive via a gen-2 coupling or delete on the next pass here.
    force_radial:       f32,    // polyphony-coupled expansion force
    motion_rule:        u32,    // 0=Brownian 1=Orbital 2=Frozen 3=Flocking
    rotation_speed:     f32,
    rotation_axis_x:    f32,
    rotation_axis_y:    f32,
    rotation_axis_z:    f32,
    orbital_base_speed: f32,    // rule 1 only

    // ── Palette (up to 4 HSV pockets, weight-selected) ───────
    palette_count:      u32,
    value_variance:     f32,
    //                hue         hue_var       saturation   weight
    pal0_hue:           f32,
    pal0_hue_var:       f32,
    pal0_sat:           f32,
    pal0_weight:        f32,
    pal1_hue:           f32,
    pal1_hue_var:       f32,
    pal1_sat:           f32,
    pal1_weight:        f32,
    pal2_hue:           f32,
    pal2_hue_var:       f32,
    pal2_sat:           f32,
    pal2_weight:        f32,
    pal3_hue:           f32,
    pal3_hue_var:       f32,
    pal3_sat:           f32,
    pal3_weight:        f32,

    // ── Color dynamics (polyphony-coupled, smoothed on CPU) ──
    color_pulse:         f32,   // 0..1
    color_converge:      f32,   // 0..1
    color_surge:         f32,   // 0..1
    hue_converge_target: f32,   // mood-scoped, changes at mood entry

    // ── Dome anchor — DEAD WIRE (the VS eye-centers the
    // dome; bytes retained for ABI, zero-filled by configure) ──
    dome_center_x:       f32,
    dome_center_y:       f32,
    dome_center_z:       f32,
    _pad_anchor:         f32,

    // ── Tier common ──────────────────────────────────────────
    tier_count:              u32,
    brownian_radial_sign:    f32,
    brownian_vert_bias:      f32,
    brownian_coherence:      f32,

    // ── Tier data (4 tiers × 10 fields, read as a 4×10 matrix) ──
    //                     mass   drag   s_min  s_max  b_min  b_max  n_gain f_gain c_gain cum_w
    tier0_mass_mult:         f32,
    tier0_drag_mult:         f32,
    tier0_size_min:          f32,
    tier0_size_max:          f32,
    tier0_brightness_min:    f32,
    tier0_brightness_max:    f32,
    tier0_noise_gain:        f32,
    tier0_force_gain:        f32,
    tier0_color_gain:        f32,
    tier0_cumulative_weight: f32,

    tier1_mass_mult:         f32,
    tier1_drag_mult:         f32,
    tier1_size_min:          f32,
    tier1_size_max:          f32,
    tier1_brightness_min:    f32,
    tier1_brightness_max:    f32,
    tier1_noise_gain:        f32,
    tier1_force_gain:        f32,
    tier1_color_gain:        f32,
    tier1_cumulative_weight: f32,

    tier2_mass_mult:         f32,
    tier2_drag_mult:         f32,
    tier2_size_min:          f32,
    tier2_size_max:          f32,
    tier2_brightness_min:    f32,
    tier2_brightness_max:    f32,
    tier2_noise_gain:        f32,
    tier2_force_gain:        f32,
    tier2_color_gain:        f32,
    tier2_cumulative_weight: f32,

    tier3_mass_mult:         f32,
    tier3_drag_mult:         f32,
    tier3_size_min:          f32,
    tier3_size_max:          f32,
    tier3_brightness_min:    f32,
    tier3_brightness_max:    f32,
    tier3_noise_gain:        f32,
    tier3_force_gain:        f32,
    tier3_color_gain:        f32,
    tier3_cumulative_weight: f32,

    // ── Flocking base (used when motion_rule == 3) ───────────
    flock_sep_radius:         f32,
    flock_align_radius:       f32,
    flock_coh_radius:         f32,
    flock_sep_weight:         f32,
    flock_align_weight:       f32,
    flock_coh_weight:         f32,
    flock_max_speed:          f32,
    flock_coupling_intensity: f32,   // polyphony-coupled tightening
    // Per-force signs from the active gesture (±1 each)
    flock_sep_sign:           f32,
    flock_align_sign:         f32,
    flock_coh_sign:           f32,
    // Per-rule drag multipliers (1.0 = pass-through, sanitized on CPU)
    rule_drag_brownian:       f32,
    rule_drag_orbital:        f32,
    rule_drag_frozen:         f32,
    rule_drag_flocking:       f32,
    orbital_alignment_mode:   f32,

    // ── Per-tier flocking gains (4 tiers × 4 fields) ─────────
    //                     sep_g  align_g coh_g  _pad
    tier0_flock_sep_gain:   f32,
    tier0_flock_align_gain: f32,
    tier0_flock_coh_gain:   f32,
    orbital_speed_var_mult: f32,

    tier1_flock_sep_gain:   f32,
    tier1_flock_align_gain: f32,
    tier1_flock_coh_gain:   f32,
    speed_mult:             f32,   // offset 444; 1.0 = identity. Applied
                                   // to each rule's dominant speed param.

    tier2_flock_sep_gain:   f32,
    tier2_flock_align_gain: f32,
    tier2_flock_coh_gain:   f32,
    _tier2_flock_pad:       f32,

    tier3_flock_sep_gain:   f32,
    tier3_flock_align_gain: f32,
    tier3_flock_coh_gain:   f32,
    _tier3_flock_pad:       f32,
}

// Rodrigues' rotation: rotate vector v by angle θ around unit axis k.
// v' = v·cos(θ) + (k × v)·sin(θ) + k·(k·v)·(1 - cos(θ))
fn rodrigues(v: vec3<f32>, k: vec3<f32>, theta: f32) -> vec3<f32> {
    let ct = cos(theta);
    let st = sin(theta);
    return v * ct + cross(k, v) * st + k * dot(k, v) * (1.0 - ct);
}

@group(0) @binding(410) var<storage, read_write> orb_state: array<OrbState>;
@group(0) @binding(411) var<uniform> orb_config: OrbConfig;
// Previous-frame snapshot (read-only view in main layout). Written
// by orb_state_prev_copy before each frame's dynamics dispatch so
// flocking can query neighbors against a stable previous frame.
@group(0) @binding(412) var<storage, read> orb_state_prev: array<OrbState>;
// Inverse-access views used only by orb_state_prev_copy. They
// reference the same physical buffers through a dedicated copy
// layout. WebGPU requires each shader declaration to match exactly
// one layout access mode, so 410/412 (bound read_write/read in the
// main layout) can't be re-used here with swapped access modes.
@group(0) @binding(413) var<storage, read>       orb_state_ro:      array<OrbState>;
@group(0) @binding(414) var<storage, read_write> orb_state_prev_rw: array<OrbState>;

// Render-side read-only view of the same orb_state buffer.
@group(0) @binding(400) var<storage, read> render_orb_state: array<OrbState>;

fn orb_rgb_to_hsv(rgb: vec3<f32>) -> vec3<f32> {
    let max_c = max(rgb.r, max(rgb.g, rgb.b));
    let min_c = min(rgb.r, min(rgb.g, rgb.b));
    let delta = max_c - min_c;

    var h: f32 = 0.0;
    if (delta > 0.0001) {
        if (max_c == rgb.r) {
            h = (rgb.g - rgb.b) / delta;
            if (h < 0.0) { h = h + 6.0; }
        } else if (max_c == rgb.g) {
            h = (rgb.b - rgb.r) / delta + 2.0;
        } else {
            h = (rgb.r - rgb.g) / delta + 4.0;
        }
        h = h / 6.0;
    }

    let s: f32 = select(0.0, delta / max_c, max_c > 0.0001);
    let v: f32 = max_c;

    return vec3<f32>(h, s, v);
}

fn orb_hsv_to_rgb(hsv: vec3<f32>) -> vec3<f32> {
    let h = hsv.x * 6.0;
    let s = hsv.y;
    let v = hsv.z;
    let c = v * s;
    let x = c * (1.0 - abs(h - 2.0 * floor(h * 0.5) - 1.0));
    let m = v - c;
    var rgb: vec3<f32>;
    if (h < 1.0) { rgb = vec3<f32>(c, x, 0.0); }
    else if (h < 2.0) { rgb = vec3<f32>(x, c, 0.0); }
    else if (h < 3.0) { rgb = vec3<f32>(0.0, c, x); }
    else if (h < 4.0) { rgb = vec3<f32>(0.0, x, c); }
    else if (h < 5.0) { rgb = vec3<f32>(x, 0.0, c); }
    else             { rgb = vec3<f32>(c, 0.0, x); }
    return rgb + vec3<f32>(m, m, m);
}

// Sample a color from the orb palette via weighted pocket selection.
// Returns HSV as vec3<f32>(hue, saturation, value).
// Layout is unrolled because WGSL uniform blocks can't hold arrays of
// structs cheaply and explicit ifs let FXC see a uniform-bounded chain.
fn orb_sample_palette(seed: u32) -> vec3<f32> {
    let roll = hash_property(seed, 8u);

    var hue: f32;
    var hue_var: f32;
    var sat: f32;

    var accum = orb_config.pal0_weight;
    if (roll < accum || orb_config.palette_count == 1u) {
        hue     = orb_config.pal0_hue;
        hue_var = orb_config.pal0_hue_var;
        sat     = orb_config.pal0_sat;
    } else {
        accum += orb_config.pal1_weight;
        if (roll < accum || orb_config.palette_count == 2u) {
            hue     = orb_config.pal1_hue;
            hue_var = orb_config.pal1_hue_var;
            sat     = orb_config.pal1_sat;
        } else {
            accum += orb_config.pal2_weight;
            if (roll < accum || orb_config.palette_count == 3u) {
                hue     = orb_config.pal2_hue;
                hue_var = orb_config.pal2_hue_var;
                sat     = orb_config.pal2_sat;
            } else {
                hue     = orb_config.pal3_hue;
                hue_var = orb_config.pal3_hue_var;
                sat     = orb_config.pal3_sat;
            }
        }
    }

    let h = fract(hue + (hash_property(seed, 9u) - 0.5) * 2.0 * hue_var);
    let s = clamp(sat + (hash_property(seed, 10u) - 0.5) * 0.15, 0.1, 1.0);

    // Value: linear distribution centered on global brightness. Floor of
    // 0.5 guarantees every orb is bright enough for its color to read.
    let val_hash = hash_property(seed, 11u);
    let v = clamp(
        orb_config.brightness - orb_config.value_variance * 0.5
        + val_hash * orb_config.value_variance,
        0.5, 1.0
    );

    return vec3<f32>(h, s, v);
}

// ─── Tier accessors ──────────────────────────────────────────────
//
// WGSL can't dynamically index struct fields, so each tier attribute
// gets its own 4-way dispatch. All branches are on uniform values
// (tier_count, per-invocation tier_idx) so FXC handles them without
// divergence penalty. The pattern is mechanical; it's kept in this
// compact form so the obvious repetition doesn't dominate the file.

// Coherent-noise seed. When Brownian's coherence gesture bit is
// active, neighbouring orbs share a hash by quantizing position to
// 80-unit blocks — "wind-gust" grouping roughly 1/5 the dome radius.
// Hard block edges; a smoother variant (interp across neighbours)
// is a future refinement.
fn orb_coherent_noise_seed(pos: vec3<f32>, t_seed: u32) -> u32 {
    let block = vec3<i32>(floor(pos / 80.0));
    let bx = bitcast<u32>(block.x);
    let by = bitcast<u32>(block.y);
    let bz = bitcast<u32>(block.z);
    return t_seed ^ (bx * 73856093u) ^ (by * 19349663u) ^ (bz * 83492791u);
}

fn orb_roll_tier(seed: u32) -> u32 {
    if (orb_config.tier_count == 0u) { return 0u; }
    let roll = hash_property(seed, 12u);
    if (roll < orb_config.tier0_cumulative_weight) { return 0u; }
    if (roll < orb_config.tier1_cumulative_weight) { return 1u; }
    if (roll < orb_config.tier2_cumulative_weight) { return 2u; }
    return 3u;
}

fn orb_tier_mass_mult(t: u32)      -> f32 { if (t == 0u) { return orb_config.tier0_mass_mult;      } if (t == 1u) { return orb_config.tier1_mass_mult;      } if (t == 2u) { return orb_config.tier2_mass_mult;      } return orb_config.tier3_mass_mult;      }
fn orb_tier_drag_mult(t: u32)      -> f32 { if (t == 0u) { return orb_config.tier0_drag_mult;      } if (t == 1u) { return orb_config.tier1_drag_mult;      } if (t == 2u) { return orb_config.tier2_drag_mult;      } return orb_config.tier3_drag_mult;      }
fn orb_tier_size_min(t: u32)       -> f32 { if (t == 0u) { return orb_config.tier0_size_min;       } if (t == 1u) { return orb_config.tier1_size_min;       } if (t == 2u) { return orb_config.tier2_size_min;       } return orb_config.tier3_size_min;       }
fn orb_tier_size_max(t: u32)       -> f32 { if (t == 0u) { return orb_config.tier0_size_max;       } if (t == 1u) { return orb_config.tier1_size_max;       } if (t == 2u) { return orb_config.tier2_size_max;       } return orb_config.tier3_size_max;       }
fn orb_tier_brightness_min(t: u32) -> f32 { if (t == 0u) { return orb_config.tier0_brightness_min; } if (t == 1u) { return orb_config.tier1_brightness_min; } if (t == 2u) { return orb_config.tier2_brightness_min; } return orb_config.tier3_brightness_min; }
fn orb_tier_brightness_max(t: u32) -> f32 { if (t == 0u) { return orb_config.tier0_brightness_max; } if (t == 1u) { return orb_config.tier1_brightness_max; } if (t == 2u) { return orb_config.tier2_brightness_max; } return orb_config.tier3_brightness_max; }
fn orb_tier_noise_gain(t: u32)     -> f32 { if (t == 0u) { return orb_config.tier0_noise_gain;     } if (t == 1u) { return orb_config.tier1_noise_gain;     } if (t == 2u) { return orb_config.tier2_noise_gain;     } return orb_config.tier3_noise_gain;     }
fn orb_tier_force_gain(t: u32)     -> f32 { if (t == 0u) { return orb_config.tier0_force_gain;     } if (t == 1u) { return orb_config.tier1_force_gain;     } if (t == 2u) { return orb_config.tier2_force_gain;     } return orb_config.tier3_force_gain;     }
fn orb_tier_color_gain(t: u32)     -> f32 { if (t == 0u) { return orb_config.tier0_color_gain;     } if (t == 1u) { return orb_config.tier1_color_gain;     } if (t == 2u) { return orb_config.tier2_color_gain;     } return orb_config.tier3_color_gain;     }

// Per-tier flocking gains (used when motion_rule == 3).
fn orb_tier_flock_sep_gain(t: u32)   -> f32 { if (t == 0u) { return orb_config.tier0_flock_sep_gain;   } if (t == 1u) { return orb_config.tier1_flock_sep_gain;   } if (t == 2u) { return orb_config.tier2_flock_sep_gain;   } return orb_config.tier3_flock_sep_gain;   }
fn orb_tier_flock_align_gain(t: u32) -> f32 { if (t == 0u) { return orb_config.tier0_flock_align_gain; } if (t == 1u) { return orb_config.tier1_flock_align_gain; } if (t == 2u) { return orb_config.tier2_flock_align_gain; } return orb_config.tier3_flock_align_gain; }
fn orb_tier_flock_coh_gain(t: u32)   -> f32 { if (t == 0u) { return orb_config.tier0_flock_coh_gain;   } if (t == 1u) { return orb_config.tier1_flock_coh_gain;   } if (t == 2u) { return orb_config.tier2_flock_coh_gain;   } return orb_config.tier3_flock_coh_gain;   }

// Snapshot orb_state → orb_state_prev so the dynamics kernel can
// read last frame's positions/velocities while writing the new ones.
// Uses the dedicated copy layout's bindings (413 read, 414 read_write)
// rather than 410/412, which are bound read_write/read in the main
// layout — see the binding-layout comment above.
@compute @workgroup_size(64)
fn orb_state_prev_copy(@builtin(global_invocation_id) id: vec3<u32>) {
    let i = id.x;
    if (i >= orb_config.count) { return; }
    orb_state_prev_rw[i] = orb_state_ro[i];
}

@compute @workgroup_size(64)
fn orb_init(@builtin(global_invocation_id) id: vec3<u32>) {
    let i = id.x;
    if (i >= orb_config.count) { return; }

    let seed = orb_config.seed ^ (i * 2654435761u);

    // Uniform point on upper hemisphere of dome.
    let u = hash_property(seed, 1u);
    let v = hash_property(seed, 2u);
    let theta = u * 6.28318530718;
    let phi = acos(1.0 - v);  // phi in [0, π/2]

    let sin_phi = sin(phi);
    let cos_phi = cos(phi);

    let dir = vec3<f32>(
        sin_phi * cos(theta),
        cos_phi,
        sin_phi * sin(theta)
    );
    let pos = dir * orb_config.dome_radius;

    // Tier: roll once from weighted table. tier_count == 0 → legacy
    // path (uniform mass/drag, ±30% size, full-range brightness).
    var tier_idx: u32 = 0u;
    var mass_final: f32 = 1.0;
    var drag_final: f32 = orb_config.drag;
    var size_mult: f32 = 0.7 + hash_property(seed, 7u) * 0.6;
    var bright_mult: f32 = 1.0;

    if (orb_config.tier_count > 0u) {
        tier_idx = orb_roll_tier(seed);

        let mm   = orb_tier_mass_mult(tier_idx);
        let dm   = orb_tier_drag_mult(tier_idx);
        let smin = orb_tier_size_min(tier_idx);
        let smax = orb_tier_size_max(tier_idx);
        let bmin = orb_tier_brightness_min(tier_idx);
        let bmax = orb_tier_brightness_max(tier_idx);

        let s_t = hash_property(seed, 13u);
        let b_t = hash_property(seed, 14u);

        mass_final  = mm;
        drag_final  = orb_config.drag * dm;
        size_mult   = smin + (smax - smin) * s_t;
        bright_mult = bmin + (bmax - bmin) * b_t;
    }

    // Color: multi-pocket palette when palette_count > 0, else legacy hue.
    var hsv: vec3<f32>;
    if (orb_config.palette_count > 0u) {
        hsv = orb_sample_palette(seed);
    } else {
        let hue_offset = (hash_property(seed, 3u) - 0.5) * 2.0 * orb_config.hue_variance;
        let h = fract(orb_config.base_hue + hue_offset);
        let s = 0.5 + hash_property(seed, 5u) * 0.3;
        let v = 0.8 + hash_property(seed, 6u) * 0.2;
        hsv = vec3<f32>(h, s, v);
    }
    hsv.z = clamp(hsv.z * bright_mult, 0.0, 1.0);
    let color = orb_hsv_to_rgb(hsv);

    // Size: correlated with brightness, then scaled by tier size_mult
    // (legacy path reuses the same formula with size_mult ≈ 0.7..1.3).
    let size = orb_config.base_size * (0.4 + hsv.z * 0.9) * size_mult;

    orb_state[i].pos = pos;
    orb_state[i]._pad0 = 0.0;
    orb_state[i].vel = vec3<f32>(0.0, 0.0, 0.0);
    orb_state[i]._pad1 = 0.0;
    orb_state[i].base_color = color;
    orb_state[i].brightness = orb_config.brightness;
    orb_state[i].current_color = color;
    orb_state[i].twinkle_phase = hash_property(seed, 4u) * 6.28318530718;
    orb_state[i].size = size;
    orb_state[i].mass = mass_final;
    orb_state[i].drag = drag_final;
    orb_state[i].tier_idx = tier_idx;
}

// Recolor kernel: writes new base_color, current_color, and size to
// every orb using the current palette. Position, velocity, twinkle
// phase, mass, and drag are preserved. A time-bucket salt is mixed
// into the seed so repeated presses within the same palette reshuffle
// pocket assignments instead of producing identical reruns.
@compute @workgroup_size(64)
fn orb_recolor(@builtin(global_invocation_id) id: vec3<u32>) {
    let i = id.x;
    if (i >= orb_config.count) { return; }

    let salt = bitcast<u32>(orb_config.t_seconds * 1000.0);
    let seed = orb_config.seed ^ (i * 2654435761u) ^ (salt * 374761393u);

    var hsv: vec3<f32>;
    if (orb_config.palette_count > 0u) {
        hsv = orb_sample_palette(seed);
    } else {
        let hue_offset = (hash_property(seed, 3u) - 0.5) * 2.0 * orb_config.hue_variance;
        let h = fract(orb_config.base_hue + hue_offset);
        let s = 0.5 + hash_property(seed, 5u) * 0.3;
        let v = 0.8 + hash_property(seed, 6u) * 0.2;
        hsv = vec3<f32>(h, s, v);
    }

    // Honour the orb's tier on recolor so the population keeps its
    // shape — preserve tier_idx, re-sample brightness/size from the
    // tier's ranges.
    var size_mult: f32 = 0.7 + hash_property(seed, 7u) * 0.6;
    if (orb_config.tier_count > 0u) {
        let t = orb_state[i].tier_idx;
        let bmin = orb_tier_brightness_min(t);
        let bmax = orb_tier_brightness_max(t);
        let b_t = hash_property(seed, 14u);
        hsv.z = clamp(hsv.z * (bmin + (bmax - bmin) * b_t), 0.0, 1.0);

        let smin = orb_tier_size_min(t);
        let smax = orb_tier_size_max(t);
        let s_t = hash_property(seed, 13u);
        size_mult = smin + (smax - smin) * s_t;
    }

    let color = orb_hsv_to_rgb(hsv);
    orb_state[i].base_color = color;
    orb_state[i].current_color = color;
    orb_state[i].size = orb_config.base_size * (0.4 + hsv.z * 0.9) * size_mult;
    // tier_idx is intentionally untouched — an orb's tier identity is
    // fixed once assigned at init.
}

@compute @workgroup_size(64)
fn orb_dynamics(@builtin(global_invocation_id) id: vec3<u32>) {
    let i = id.x;
    if (i >= orb_config.count) { return; }

    var orb = orb_state[i];
    let dt = orb_config.dt;

    // ═══ 1. DOME ROTATION (all rules) ═════════════════════════
    // Rigid rotation of the entire dome. Applied to both position
    // and velocity so local dynamics stay coherent in the rotating
    // frame.
    if (abs(orb_config.rotation_speed) > 0.0001) {
        let rot_axis = normalize(vec3<f32>(
            orb_config.rotation_axis_x,
            orb_config.rotation_axis_y,
            orb_config.rotation_axis_z
        ));
        let rot_angle = orb_config.rotation_speed * dt;
        orb.pos = rodrigues(orb.pos, rot_axis, rot_angle);
        orb.vel = rodrigues(orb.vel, rot_axis, rot_angle);
    }

    // Per-orb tier gains — one lookup, shared by the rule branches
    // and the color tail. Legacy path (tier_count==0) leaves them at 1.
    var noise_gain_t: f32 = 1.0;
    var force_gain_t: f32 = 1.0;
    var color_gain_t: f32 = 1.0;
    if (orb_config.tier_count > 0u) {
        let ti = orb.tier_idx;
        noise_gain_t = orb_tier_noise_gain(ti);
        force_gain_t = orb_tier_force_gain(ti);
        color_gain_t = orb_tier_color_gain(ti);
    }

    // ═══ 2. RULE DISPATCH ═════════════════════════════════════
    // Uniform branch — every invocation in the workgroup takes
    // the same path, no FXC divergence penalty.
    if (orb_config.motion_rule == 0u) {
        // ── BROWNIAN (gesture-modulated) ─────────────────────
        // drift / gather / rise / gust / tide / swell
        orb.vel = orb.vel * exp(-orb.drag * orb_config.rule_drag_brownian * dt);

        if (orb_config.noise_amp > 0.0) {
            let t_seed = bitcast<u32>(orb_config.t_seconds * 1000.0);
            var noise_seed: u32;
            if (orb_config.brownian_coherence > 0.5) {
                noise_seed = orb_coherent_noise_seed(orb.pos, t_seed);
            } else {
                noise_seed = t_seed ^ (i * 2654435761u);
            }

            let nx     = (hash_property(noise_seed, 1u) - 0.5) * 2.0;
            let ny_raw = (hash_property(noise_seed, 2u) - 0.5) * 2.0;
            let nz     = (hash_property(noise_seed, 3u) - 0.5) * 2.0;

            // Vertical bias: 0 = isotropic; 1 = upward-only (abs folds
            // negatives to positives, embers rising instead of
            // drifting symmetrically).
            let ny = mix(ny_raw, abs(ny_raw), orb_config.brownian_vert_bias);

            orb.vel += vec3<f32>(nx, ny, nz)
                * orb_config.noise_amp * orb_config.speed_mult
                * sqrt(dt) * noise_gain_t;
        }

        if (abs(orb_config.force_radial) > 0.001) {
            let radial_dir = normalize(orb.pos);
            // Radial sign: +1 expands, -1 contracts (gather/tide).
            orb.vel += radial_dir * orb_config.force_radial
                * orb_config.brownian_radial_sign
                * dt * force_gain_t / orb.mass;
        }

    } else if (orb_config.motion_rule == 1u) {
        // ── ORBITAL (gesture-modulated) ──────────────────────
        // scatter / parallel / mirror / shear
        // vel stays perturbation-only; drag damps deviations, not
        // the orbital motion.

        let orb_seed = orb_config.seed ^ (i * 2654435761u);

        // Axis depends on alignment_mode:
        //  0 scatter  — random per orb (legacy behaviour)
        //  1 parallel — shared Y-up for the whole shell
        //  2 mirror   — Y-up with a seed-parity sign flip
        var orbital_axis: vec3<f32>;
        let mode = orb_config.orbital_alignment_mode;
        if (mode < 0.5) {
            let ax = hash_property(orb_seed, 20u) - 0.5;
            let ay = hash_property(orb_seed, 21u) - 0.5;
            let az = hash_property(orb_seed, 22u) - 0.5;
            orbital_axis = normalize(vec3<f32>(ax, ay, az));
        } else if (mode < 1.5) {
            orbital_axis = vec3<f32>(0.0, 1.0, 0.0);
        } else {
            let parity = hash_property(orb_seed, 24u);
            let sgn = select(-1.0, 1.0, parity > 0.5);
            orbital_axis = vec3<f32>(0.0, sgn, 0.0);
        }

        // Speed variance: 1.0 + (hash-0.5) * speed_var_mult so
        // speed_var_mult=1 reproduces legacy 0.5..1.5 spread,
        // 0 collapses to unified speed, >1 spreads into sheets.
        let raw_var = hash_property(orb_seed, 23u) - 0.5;
        let speed_factor  = 1.0 + raw_var * orb_config.orbital_speed_var_mult;
        let orbital_speed = orb_config.orbital_base_speed
                          * speed_factor
                          * orb_config.speed_mult;

        orb.pos = rodrigues(orb.pos, orbital_axis, orbital_speed * dt);

        orb.vel = orb.vel * exp(-orb.drag * orb_config.rule_drag_orbital * dt);

        if (abs(orb_config.force_radial) > 0.001) {
            let radial_dir = normalize(orb.pos);
            orb.vel += radial_dir * orb_config.force_radial
                * dt * force_gain_t / orb.mass;
        }

    } else if (orb_config.motion_rule == 3u) {
        // ── FLOCKING (Boids) ─────────────────────────────────
        // Neighbor-based dynamics. Reads orb_state_prev (the
        // previous-frame snapshot) to avoid in-pass feedback.
        // O(N²) inner loop — MAX_ORBS = 256 keeps this tractable
        // without a spatial hash.

        orb.vel = orb.vel * exp(-orb.drag * orb_config.rule_drag_flocking * dt);

        let sep_r2 = orb_config.flock_sep_radius   * orb_config.flock_sep_radius;
        let ali_r2 = orb_config.flock_align_radius * orb_config.flock_align_radius;
        let coh_r2 = orb_config.flock_coh_radius   * orb_config.flock_coh_radius;

        var sep_sum   = vec3<f32>(0.0, 0.0, 0.0);
        var ali_sum   = vec3<f32>(0.0, 0.0, 0.0);
        var coh_sum   = vec3<f32>(0.0, 0.0, 0.0);
        var sep_count = 0.0;
        var ali_count = 0.0;
        var coh_count = 0.0;

        let n = orb_config.count;
        for (var j: u32 = 0u; j < n; j = j + 1u) {
            if (j == i) { continue; }
            let other = orb_state_prev[j];
            let diff  = orb.pos - other.pos;
            let d2    = dot(diff, diff);

            if (d2 < sep_r2 && d2 > 0.001) {
                sep_sum = sep_sum + diff / d2;
                sep_count = sep_count + 1.0;
            }
            if (d2 < ali_r2) {
                ali_sum = ali_sum + other.vel;
                ali_count = ali_count + 1.0;
            }
            if (d2 < coh_r2) {
                coh_sum = coh_sum + other.pos;
                coh_count = coh_count + 1.0;
            }
        }

        // Separation: direction away from the distance-weighted sum.
        var sep_force = vec3<f32>(0.0, 0.0, 0.0);
        if (sep_count > 0.0) {
            let sl = length(sep_sum);
            if (sl > 0.001) { sep_force = sep_sum / sl; }
        }

        // Alignment: steer toward the average velocity.
        var ali_force = vec3<f32>(0.0, 0.0, 0.0);
        if (ali_count > 0.0) {
            let avg_vel = ali_sum / ali_count;
            ali_force = avg_vel - orb.vel;
        }

        // Cohesion: unit vector pointing to center of mass.
        var coh_force = vec3<f32>(0.0, 0.0, 0.0);
        if (coh_count > 0.0) {
            let center = coh_sum / coh_count;
            let d = center - orb.pos;
            let dl = length(d);
            if (dl > 0.001) { coh_force = d / dl; }
        }

        // Per-tier flocking gains (1.0 when no tier set).
        var sep_g: f32 = 1.0;
        var ali_g: f32 = 1.0;
        var coh_g: f32 = 1.0;
        if (orb_config.tier_count > 0u) {
            sep_g = orb_tier_flock_sep_gain(orb.tier_idx);
            ali_g = orb_tier_flock_align_gain(orb.tier_idx);
            coh_g = orb_tier_flock_coh_gain(orb.tier_idx);
        }

        // Polyphony modulation: louder music tightens the flock.
        let k = orb_config.flock_coupling_intensity;
        let sep_mod = 1.0 - k * 0.5;
        let ali_mod = 1.0 + k * 0.5;
        let coh_mod = 1.0 + k;

        // Independent sign per force — compound gestures (swirl,
        // orbit, huddle, etc.) live in the combinations of these
        // three bits, cycled at runtime by the player.
        let sep_s = orb_config.flock_sep_sign;
        let ali_s = orb_config.flock_align_sign;
        let coh_s = orb_config.flock_coh_sign;
        orb.vel = orb.vel
            + sep_force * orb_config.flock_sep_weight   * sep_g * sep_mod * sep_s * dt
            + ali_force * orb_config.flock_align_weight * ali_g * ali_mod * ali_s * dt
            + coh_force * orb_config.flock_coh_weight   * coh_g * coh_mod * coh_s * dt;

        // Speed clamp — prevent runaway velocity from force accumulation.
        // Scaled by speed_mult so the steady-state flock speed follows
        // the population-wide attractor.
        let eff_max = orb_config.flock_max_speed * orb_config.speed_mult;
        let speed2 = dot(orb.vel, orb.vel);
        let max_s2 = eff_max * eff_max;
        if (speed2 > max_s2 && speed2 > 0.0) {
            orb.vel = orb.vel * (eff_max / sqrt(speed2));
        }

        if (abs(orb_config.force_radial) > 0.001) {
            let radial_dir = normalize(orb.pos);
            orb.vel += radial_dir * orb_config.force_radial
                * dt * force_gain_t / orb.mass;
        }

    } else {
        // ── FROZEN (rule 2 / unknown) ────────────────────────
        // Only dome rotation moves orbs. Drag bleeds velocity
        // to zero.
        orb.vel = orb.vel * exp(-orb.drag * orb_config.rule_drag_frozen * dt);
    }

    // ═══ 3. COMMON TAIL ═══════════════════════════════════════

    // Integrate perturbation velocity into position.
    orb.pos = orb.pos + orb.vel * dt;

    // Project back onto dome shell so float drift doesn't accumulate.
    let r = length(orb.pos);
    if (r > 0.001) {
        orb.pos = orb.pos * (orb_config.dome_radius / r);
    }

    // Snap negligible velocity to zero (lower threshold so slow
    // floor-noise drift isn't killed between impulses).
    if (dot(orb.vel, orb.vel) < 1e-8) {
        orb.vel = vec3<f32>(0.0, 0.0, 0.0);
    }

    // ═══ 4. COLOR DYNAMICS (common tail) ══════════════════════
    // Three independent trajectories composed on top of base_color:
    // converge rotates hue toward a shared target so music pulls
    // the sky into unity; surge pushes saturation up; pulse lifts
    // brightness. Order matters — convergence first so the hue is
    // settled before saturation/brightness amplify it.
    var color = orb.base_color;

    let converge_amount = orb_config.color_converge * color_gain_t;
    if (converge_amount > 0.001) {
        let hsv = orb_rgb_to_hsv(color);
        let target_h = orb_config.hue_converge_target;
        var dh = target_h - hsv.x;
        if (dh > 0.5)  { dh = dh - 1.0; }
        if (dh < -0.5) { dh = dh + 1.0; }
        let new_h = fract(hsv.x + dh * converge_amount);
        color = orb_hsv_to_rgb(vec3<f32>(new_h, hsv.y, hsv.z));
    }

    let surge_amount = orb_config.color_surge * color_gain_t;
    if (surge_amount > 0.001) {
        let hsv = orb_rgb_to_hsv(color);
        let boosted_s = clamp(hsv.y + (1.0 - hsv.y) * surge_amount * 0.5,
                              0.0, 1.0);
        color = orb_hsv_to_rgb(vec3<f32>(hsv.x, boosted_s, hsv.z));
    }

    let pulse_mult = 1.0 + orb_config.color_pulse * color_gain_t * 0.6;
    color = color * pulse_mult;

    orb.current_color = color;

    // Twinkle: subtle per-orb brightness oscillation, de-synced by phase.
    let twinkle = 0.85 + 0.15 * sin(orb.twinkle_phase + orb_config.t_seconds * 1.5);
    orb.brightness = orb_config.brightness * twinkle;

    orb_state[i] = orb;
}

struct OrbVSOut {
    @builtin(position) clip_pos: vec4<f32>,
    @location(0) uv: vec2<f32>,
    @location(1) color: vec3<f32>,
    @location(2) brightness: f32,
}

@vertex
fn orb_vs(
    @location(0) quad_pos: vec2<f32>,
    @builtin(instance_index) instance: u32
) -> OrbVSOut {
    let orb = render_orb_state[instance];

    // Build world-space camera basis from azimuth/elevation — matches
    // build_view_projection_matrix conventions exactly.
    let az = render_camera.azimuth;
    let el = render_camera.elevation;
    let cos_el = cos(el);
    let sin_el = sin(el);
    let cos_az = cos(az);
    let sin_az = sin(az);

    let orbital = vec3<f32>(cos_el * sin_az, sin_el, cos_el * cos_az);
    let cam_right = vec3<f32>(cos_az, 0.0, -sin_az);
    let cam_up = cross(orbital, cam_right);

    // orb.pos is dome-local; the dome is a SKYBOX (Jean's
    // ruling): centered on the camera EYE, all three axes, every
    // frame — the sky rises when you fly and never parallaxes away.
    // render_camera is already in this VS (the billboard basis above);
    // the old orb_config.dome_center_* wire is dead (bytes retained
    // for ABI; the CPU anchor machinery retired with it).
    let dome_center = render_camera.pos;
    let world_pos = dome_center + orb.pos
        + cam_right * (quad_pos.x * orb.size)
        + cam_up    * (quad_pos.y * orb.size);

    var out: OrbVSOut;
    out.clip_pos = render_vp.m * vec4<f32>(world_pos, 1.0);
    out.uv = quad_pos;
    out.color = orb.current_color;
    out.brightness = orb.brightness;
    return out;
}

@fragment
fn orb_fs(in: OrbVSOut) -> @location(0) vec4<f32> {
    // Soft radial falloff — circle with smooth edge.
    let r = length(in.uv);
    if (r > 1.0) { discard; }

    let alpha = smoothstep(1.0, 0.3, r);
    let intensity = alpha * in.brightness;

    // Premultiplied alpha — pipeline uses additive blending.
    return vec4<f32>(in.color * intensity, intensity);
}

// END OF SCROLL
