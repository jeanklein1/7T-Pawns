// ─── world.wgsl ──────────────────────────────────────────────────────
//
// THE_BOARD CARTRIDGE — GPU spine.
//
// Counterpart to cartridge.hpp. The CPU spine authors intent (mood
// transitions, entity lifecycle, dispatch tables, signal preparation);
// this file is what the GPU runs every frame — terrain evaluation,
// entity compute, render pipelines, mesh generation. GPU is
// sovereign: CPU dead-reckoning exists only for placement, picking,
// and step decisions; the visual reality is here.
//
// THE LAWS THAT GOVERN THIS FILE — docs/LAWS.md:
//   L1  encoding — BOM-free LF.
// ═══ COMPILER FLOOR ═══════════════════════════════════════
// The web twin is the program (native archived at tag
// native-sunset). This module is PLAIN WGSL: no `requires`
// directive, no immediate address space, so naga reads it raw
// and tools/wgsl_gate.py is that read, per commit.
// The floor of record is the Tint trio — Tint→DXC (SM6.0+),
// Tint→MSL, Tint→SPIR-V; FXC is unsupported and its retired
// laws live in docs/FXC_LAWS_RECORD.md. Firefox and Safari:
// WITNESSED 2026-08-21. Firefox on Windows compiles the module
// with no diagnostics and runs; Safari 26 on an iPad runs 10+
// min with transitions. Firefox's later device loss is staging
// memory (OPEN.md, FIREFOX STAGING RATCHET), not the compiler:
// the supported set is the Tint trio and naga.
// NAGA WITNESSES THE MODULE ONLY. Pipeline-layout conformance,
// minBindingSize and every dynamic-offset alignment are Dawn's
// checks at pipeline creation and at bind, so the web boot
// witnesses them and naga cannot; the [Pipeline] timer prices
// compile time per kernel.
// Witness protocol: a shader-shape change is proven by
// witnesses, not argument, and no witness substitutes for
// another — each browser gates at its own.
//   Budget = WebGPU core defaults: storage 8 / uniforms 12 per stage;
//   per-row occupancy is MANIFEST.md's lane table — the banner names
//   the witness, not its value (TETRIS WALLET_0; demotions: Table C).
//   This budget is L14, NOT a compiler law: core defaults bind on
//   every backend and survived FXC's retirement unchanged.
//   L3  mirror — §2.1 structs ↔ state.hpp byte-for-byte; §3.4
//       POLICY_*_MASK ↔ POLICIES[] in contracts/
//       ground_architecture.hpp; §1.5 randomness ↔ primitives/
//       seed_utils.hpp, bit-identical. Both rooms, same commit.
//   L6  binding numbers — realization/binding_registry.hpp is the
//       source; the @binding literals here are its mirror.
//
// Navigation: §1-§9 chapter-numbered structure. Section numbers
// reflect file order — search by section number to jump.
// TUNING SURFACE DIRECTORY (below) lists the constants that shape
// terrain, color, and entity behavior. SECTION MAP gives the
// chapter outline.
// ─────────────────────────────────────────────────────────────────────

// THE_BOARD CARTRIDGE — GPU Scroll
//
// ═══════════════════════════════════════════════════════════════════════
// TUNING SURFACE DIRECTORY — GPU-side compositional control
// ═══════════════════════════════════════════════════════════════════════
//
// All constants that shape terrain, color, and cell behavior.
// Change a number, recompile, see the result. No logic edits needed.
// Purely visual constants prefer WGSL-side residence for the
// edit-save-look loop: a number that only changes what the eye sees
// wants the shortest path from edit to sight, and that path is here.
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
//
// ── Spatial Field Lattices (§2.2) ─────────────────────────────────
//   PALETTE_LATTICE_SPACING       300 wu — palette blob size
//   MODE_LATTICE_SPACING          120 wu — smooth/discrete clusters
//   MODE_DISCRETE_THRESHOLD       0.70 — gate for checkerboard
//   MODE_BIAS_EXPONENT            0.72 — the 50/50 checkers/smooth split
//   TRANSITION_LATTICE_SPACING    200 wu — blend/scatter zones
//   SPARSE_BASE_SPACING           160 wu — isolated cell regions
//   SPARSE_CLUSTER_SPACING        40 wu — small dense patches
//   CHESS_LATTICE_SPACING         55 wu — B&W alternation zones
//   DISCRETE_COLOR_LATTICE_SPACING  80 wu — colored cell blobs
//   DISCRETE_MONO_LATTICE_SPACING   250 wu — B&W tendency zones
//
// ── Terrain Waves (→ §2.2 TERRAIN_LOOKS ROW 7) ────────────────────
//   WAVE_THRESHOLD[6]             Per-band activity gate
//   ACTIVITY_LATTICE_SPACING      400 wu — activity envelope
//
// ── GoL Zones (§2.2, §7.0b) ──────────────────────────────────────
//   GOL_TIERS[10]                 Tier params (rule, density, tick, spring)
//   GOL_PULSE_TIERS[4]                Pulse algorithm params (field, ...)
//   GOL_ZONE_SPAWN_CHANCE         0.60 — fraction of discrete zones
//   GOL_ZONE_HEIGHT_CHANCE        1.00 — fraction with extrusion
//   GOL_COLOR_WEIGHTS             Color mode probabilities
//
// ── Pawn (§2.2) ──────────────────────────────────────────────────
//   PAWN_HEIGHT / PAWN_RADIUS     Physical dimensions
//   PAWN_MAX_SLOPE                Max climbable grade (the slope law)
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
// the visible_id instance attribute — the latter is needed for GPU
// frustum-culled draws. DOMESDAY_0 B3: the visible list arrives by
// instance-step vertex fetch now, not through a storage seat).
override USE_PATCH_INDIRECTION: bool = false;  // true = read the visible_id attribute

// §1.1 PROJECTIVE GEOMETRIC ALGEBRA

struct Motor {
    p0: vec4<f32>,  // [s, e23, e31, e12]     — scalar + rotation bivectors
    p1: vec4<f32>,  // [e01, e02, e03, e0123] — translation + pseudoscalar
}

struct Line {
    d: vec3<f32>,   // [e23, e31, e12] — direction/axis
    m: vec3<f32>,   // [e01, e02, e03] — moment
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

// Patch system — streaming terrain
// THE BAKE IS THE READER'S SHADOW (LATTICE_1). Texel i IS lattice point i —
// the grid patch_terrain_vs decodes. Twin of Dim::PATCH_HEIGHTFIELD_N.
const PATCH_HEIGHTFIELD_N: u32 = PATCH_MESH_N + 1u;   // 65
const_assert PATCH_HEIGHTFIELD_N == PATCH_MESH_STRIDE;
// The stencil the normals were tuned under: the pitch of the RETIRED
// 256-texel field. The gradient stays a 5-tap central difference at this
// spacing rather than becoming analytic — analytic would carry the fine
// band's slope ~1.7x stronger, which is a different picture, not a better
// number.
const BAKE_STENCIL_EPS: f32 = PATCH_EXTENT / 255.0;
const PATCH_CELL_N: u32 = 16u;          // cell color texture side per patch
const PATCH_EXTENT: f32 = 50.0;         // world units per patch side
// THE CELL — one spelling, and this is it. The card's window snap, the
// one-address law, the zone extent + corner snap and the aura all read
// THIS name. L3 MIRROR: Dim::PATCH_CELL_SIZE (state.hpp, beside
// PATCH_CELL_N). Change both rooms together.
const PATCH_CELL_SIZE: f32 = PATCH_EXTENT / f32(PATCH_CELL_N);   // 3.125

// ── THE LIVE CARD (GROUND_CARD_1; C++ room: Dim::LIVE_CARD_*) ──
const LIVE_CARD_SIZE: u32 = 640u;
// IOS_3 A — THE GUARD MUST NEVER BECOME LOAD-BEARING. write_live_card is
// @workgroup_size(16, 16) and 640 = 40 x 16 EXACTLY, so its
// `if (ix >= LIVE_CARD_SIZE …) { return; }` never fires and no workgroup
// is partial. The bake is the counter-example that earns this line:
// PATCH_HEIGHTFIELD_N is 65 = 4 x 16 + 1, so its last workgroup row and
// column carry ONE valid lane in sixteen, and its guard sits BELOW both
// barriers for exactly that reason. If this ever stops holding, partial
// workgroups appear here too and the same rule binds: a barrier reached
// by only some lanes is undefined behaviour in WGSL — tolerated by
// permissive desktop compilers, fatal on a strict one.
const_assert LIVE_CARD_SIZE % 16u == 0u;
const LIVE_CARD_EXTENT: f32 = 1000.0;
const SHELL_RING_WIDTH: f32 = 0.35;   // wu, half-width of a ring band (DEBUG_VIEW 6)
fn live_card_origin() -> vec2<f32> {
    let cs = PATCH_CELL_SIZE;
    let raw = vec2(config.lod_point_x, config.lod_point_z)
            - vec2(LIVE_CARD_EXTENT * 0.5);
    return floor(raw / cs) * cs;                          // cell snap
}
const PATCH_MESH_N: u32 = 64u;          // mesh subdivisions per patch (LOD-0)
const PATCH_MESH_STRIDE: u32 = PATCH_MESH_N + 1u;

// THE ONE-ADDRESS LAW (SEAMLESSNESS corollary — charter C8). A cell
// has exactly ONE address: its world cell index. Every consumer —
// hash, roll, noise, color/tag texel, bake write — derives from it. A
// texel is COMPUTED FROM the address; patch_uv never addresses
// anything by itself again.
fn cell_address(world_xz: vec2<f32>) -> vec2<i32> {
    let cs = PATCH_CELL_SIZE;
    return vec2<i32>(floor(world_xz / cs));
}

// ─── Patch skirts (weld #2, SKIRTS) ─────────────────────────────────
// Each patch skirts its FULL perimeter to hide inter-patch cracks
// (precision + LOD/T-junction) with one mechanism: duplicate the edge
// ring, drop the copies below the composited surface, quad-strip
// ring→copy. Skirt verts have vertex_index >= PATCH_GRID_VERT_COUNT; the
// index geometry is appended by the C++ patch-IB gen (state.hpp).
const PATCH_GRID_VERT_COUNT: u32 = PATCH_MESH_STRIDE * PATCH_MESH_STRIDE;  // 65*65 = 4225
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
    cellx: u32,        // owning cell, patch-local. Cap and base decode it
    cellz: u32,        // directly; legacy and skirt derive it from the
                       // min-corner grid vert, which names their quad's cell.
    lift_scale: f32,   // derived: 1 - wall. Walls do not lift (WALL_1).
    drop: f32,         // PATCH_SKIRT_DEPTH on skirt ring copies
    wall: f32,         // 1 on curtain-bottom + skirt copies
}
fn ug_decode(vi: u32) -> UgVert {
    var d: UgVert;
    d.drop = 0.0;
    d.wall = 0.0;
    if (vi < PATCH_GRID_VERT_COUNT) {
        // legacy grid (the LOD1/soft space) — zero-reader since CELL_1;
        // retained as the address-space floor.
        d.vx = vi % PATCH_MESH_STRIDE;
        d.vz = vi / PATCH_MESH_STRIDE;
        d.cellx = min(d.vx / UG_QUADS, PATCH_CELL_N - 1u);
        d.cellz = min(d.vz / UG_QUADS, PATCH_CELL_N - 1u);
    } else if (vi < UG_CAP_BASE) {
        // skirt ring copy — keeps its legacy slot, drops after compositing
        let g = patch_skirt_grid(vi - PATCH_GRID_VERT_COUNT);
        d.vx = g.x;
        d.vz = g.y;
        d.cellx = min(d.vx / UG_QUADS, PATCH_CELL_N - 1u);
        d.cellz = min(d.vz / UG_QUADS, PATCH_CELL_N - 1u);
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
        d.cellx = cell % PATCH_CELL_N;
        d.cellz = cell / PATCH_CELL_N;
    } else {
        // base band: curtain-bottom twin of a cap perimeter vert
        let r = vi - UG_BASE_BASE;
        let cell = r / 16u;          // UG_BASE_VERTS_PER_CELL
        let g = ug_cell_perimeter(r % 16u);
        d.vx = (cell % PATCH_CELL_N) * UG_QUADS + g.x;
        d.vz = (cell / PATCH_CELL_N) * UG_QUADS + g.y;
        d.cellx = cell % PATCH_CELL_N;
        d.cellz = cell / PATCH_CELL_N;
        d.wall = 1.0;
    }
    // WALLS DO NOT LIFT (WALL_1). `wall` names a wall BOTTOM — skirt
    // ring copy or curtain-bottom twin — and a wall bottom stands on the
    // unlifted ground, so the face above it spans the whole discontinuity
    // whatever its size. The skirt row carried lift_scale = 1 while
    // carrying wall = 1: one fact with two homes, and the copies
    // disagreed. Derived once, here.
    d.lift_scale = 1.0 - d.wall;
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
    TerrainBand(     30.0,   0.200,  0.060,  1.2,   1.0,   0.040,  0.020,  0.020,  0.78,  0.20  ),  // 2: local        reach≈150
    TerrainBand(     12.0,   0.500,  0.150,  0.4,   0.45,  0.080,  0.040,  0.040,  0.72,  0.40  ),  // 3: detail       reach≈75
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
// THE NODE, DERIVED (LATTICE_1b). A lattice node's parameters are a pure
// function of (node, node_seed, band, config.terrain_amp_ceiling) — R-F
// verified end to end: `world_xz` enters ONLY the two wave primitives. So
// the derivation — ~11 hash_property calls, three Box-Muller draws and two
// angle transcendentals — can be done ONCE per node and reused by every
// query point that touches it, instead of once per (point, node) pair.
//
// `kind` folds the activation gate in: 0 is a silent node, and nothing
// else on the struct is meaningful for it. `anchor` is node_world for a
// directional node and the offset center for a radial one; `dir` is
// (0,0) for radial and unread.
struct WaveNode {
    freq: f32,
    amp: f32,
    damping: f32,
    phase: f32,          // phase_base — the frozen phase; the caller adds the offset
    anchor: vec2<f32>,   // directional: node_world; radial: center
    dir: vec2<f32>,      // directional only; (0,0) for radial
    kind: u32,           // 0 silent (activation gate), 1 directional, 2 radial
    _p0: u32,
    _p1: u32,
    _p2: u32,
}

// The parameter half of the old evaluate_lattice_wave_pair, expression for
// expression in its float order (the U5a discipline).
fn derive_wave_node(node: vec2<i32>, node_seed: u32, band: TerrainBand) -> WaveNode {
    var n: WaveNode;
    n.freq = 0.0;
    n.amp = 0.0;
    n.damping = 0.0;
    n.phase = 0.0;
    n.anchor = vec2(0.0);
    n.dir = vec2(0.0);
    n.kind = 0u;
    n._p0 = 0u;
    n._p1 = 0u;
    n._p2 = 0u;

    // Activation gate: if draw exceeds band activation, this node is silent.
    // Spatially coherent — a silent node is silent for all query points.
    if (hash_property(node_seed, WAVE_PROP_ACTIVE) > band.activation) {
        return n;
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

    // Node world position: center of this lattice cell
    let node_world = (vec2<f32>(node) + 0.5) * band.spacing;

    n.freq = freq;
    n.amp = amp;
    n.damping = damping;
    n.phase = phase_base;

    if (is_radial) {
        let offset_angle = hash_property(node_seed, WAVE_PROP_DIR_ANGLE) * 2.0 * PI;
        let offset_r = hash_property(node_seed, WAVE_PROP_CENTER_R) * band.spacing * 0.3;
        n.anchor = node_world + vec2(cos(offset_angle), sin(offset_angle)) * offset_r;
        n.kind = 2u;
    } else {
        let dir_angle = hash_property(node_seed, WAVE_PROP_DIR_ANGLE) * 2.0 * PI;
        n.anchor = node_world;
        n.dir = vec2(cos(dir_angle), sin(dir_angle));
        n.kind = 1u;
    }
    return n;
}

// The evaluation half. `phase_offset` is what the caller adds to the frozen
// phase — 0.0 for frozen, the beat term for moving. `x + 0.0 == x` for
// finite x in IEEE, so the frozen call is bit-identical to the old
// phase_frozen = phase_base path.
fn eval_wave_node(world_xz: vec2<f32>, n: WaveNode, phase_offset: f32) -> f32 {
    if (n.kind == 0u) { return 0.0; }
    if (n.kind == 2u) {
        return evaluate_radial_wave(world_xz, n.anchor, n.freq, n.amp, n.damping,
                                    n.phase + phase_offset);
    }
    return evaluate_directional_wave(world_xz, n.anchor, n.freq, n.amp, n.damping,
                                     n.dir, n.phase + phase_offset);
}

fn evaluate_lattice_wave_pair(
    world_xz: vec2<f32>,
    node: vec2<i32>,
    node_seed: u32,
    band: TerrainBand,
    band_act: f32,       // activity level for this band [0,1] (from hierarchy)
    beat_freq: f32,      // cycles per beat from activity field
    t_beats: f32,        // current time in beats
) -> vec2<f32> {
    let n = derive_wave_node(node, node_seed, band);
    if (n.kind == 0u) { return vec2(0.0, 0.0); }

    // Two phases: frozen is the reference shape, moving advances with beats.
    // band.temporal_freq scales the pool's beat_freq per band:
    //   fine bands ripple fast, continental bands swell slowly.
    // The product keeps the old phase_moving's association exactly.
    let moving_offset = t_beats * beat_freq * band.temporal_freq * 2.0 * PI;
    return vec2(eval_wave_node(world_xz, n, 0.0),
                eval_wave_node(world_xz, n, moving_offset));
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
//
// THIS IS THE CARD KERNEL'S FORM (LATTICE_4) and it MUST run after
// write_live_card's table barrier: it reads `card_nodes` / `card_origin`,
// which only that kernel fills. naga cannot check that; its SINGLE
// CALLER is the guard. A second caller must restore the derive path —
// `derive_wave_node(node, lattice_node_seed(seed, node, band_idx), band)`
// is exactly what the table holds — and give it its own home.
fn true_band_delta_contribution(world_xz: vec2<f32>, seed: u32,
    t_eff_beats: f32, band_idx: u32,
    raw_activity: f32, beat_freq: f32) -> f32 {
    let band = TERRAIN_BANDS[band_idx];
    let band_act = band_activity_level(raw_activity, band_idx);

    let lattice_pos = world_xz / band.spacing;
    let lattice_base = vec2<i32>(floor(lattice_pos));
    let frac = fract(lattice_pos);
    let w = frac * frac * (3.0 - 2.0 * frac);

    // THE MOVING PHASE'S OFFSET, hoisted out of the node loop but in the
    // pair's own association: evaluate_lattice_wave_pair computed
    // phase_moving = phase_base + t * beat_freq * temporal_freq * 2 * PI,
    // and this is that product, unchanged.
    let moving_offset = t_eff_beats * beat_freq * band.temporal_freq * 2.0 * PI;

    var delta: f32 = 0.0;
    for (var dz: i32 = 0; dz <= 1; dz++) {
        for (var dx: i32 = 0; dx <= 1; dx++) {
            let node = lattice_base + vec2<i32>(dx, dz);

            let wx = select(1.0 - w.x, w.x, dx == 1);
            let wz = select(1.0 - w.y, w.y, dz == 1);
            let weight = wx * wz;

            // The node, DERIVED ONCE PER TILE (LATTICE_4). The pair returned
            // vec2(frozen, moving) and this took pair.y − pair.x; these are
            // the same two values from the same parameters, so the
            // subtraction is the same subtraction.
            let rel = node - card_origin[band_idx];
            let n = card_nodes[CARD_TABLE_OFF[band_idx]
                             + u32(rel.y) * card_nodes_n(band_idx) + u32(rel.x)];
            delta += band_act * (eval_wave_node(world_xz, n, moving_offset)
                               - eval_wave_node(world_xz, n, 0.0)) * weight;
        }
    }
    return delta;
}

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
    move_x: f32,
    move_z: f32,
    look_az_delta: f32,
    look_el_delta: f32,
    zoom_delta: f32,
    pan_x_delta: f32,
    pan_y_delta: f32,
    dt_beats: f32,        // beat-time delta (currentBeats_ - prevBeats_)
    // THE MOUNT BLOCK (RIBBON_1) — a trajectory, not a pose. The pose is the
    // GPU's (ribbon_body_read.saddle); the CPU authors only the edge: where the
    // body was when the host changed, and how far along the ease it is.
    mount_phase: f32,          // 48  0→1; 1 = arrived
    mount_kind: u32,           // 52  0 none, 1 boarding, 2 landing
    mount_from_heading: f32,   // 56
    _mp0: u32,                 // 60
    mount_from: vec3<f32>,     // 64  the pose the body left
    _mp1: f32,                 // 76
}

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
    route: u32,     // ATRIUM_4. Was home_y — the tether is planar and nothing read it
                    // (R4). PASSER's route state; 0 = fresh. Zero on every other behaviour.
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
    skin_id: u32,   // PawnFigure row (0 = regular). Mirrors GPUAgentState.skin_id (H2).
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
    aux:             f32,  // ATRIUM_4: behaviour-specific scalar (PASSER = the band, wu; 0 elsewhere). Was _pad.
}



struct AgentTierParams {
    step_gain:     f32,
    persist_gain:  f32,
    speed_gain:    f32,
    color_r:       f32,
    color_g:       f32,
    color_b:       f32,
    contact_radius: f32, // 24 — TRUEBAND_CONTACT_1: body radius (wu)
    contact_mass:   f32, // 28 — relative yield authority
    personal_radius: f32,// 32 — CONTACT_2: the social shell (flock sense + flee trigger); seed 30 = flock neighbor_radius
    flee_gain_player: f32,//36 — CONTACT_2: flee response gain vs the point-source
    _pad0: f32,          // 40 — pad AgentTierParams to 48 (uniform array stride)
    _pad1: f32,          // 44
}

const AGENT_TIER_COUNT_WGSL: u32 = 4u;


// --- Pawn figure table (CLOSURE_PAWN) ----------------------------------
// UNIFORM, render VS only — the render entity group's VERTEX stage is at
// the per-stage STORAGE cap and uniform has its own budget. That reason
// is now the whole SceneConstants block's (CHORD_4), which is where this
// table rides. Do not "upgrade" the block to var<storage>; it fails
// CreateBindGroupLayout at launch.
//
// vec4 PACKING IS MANDATORY, not cosmetic: the uniform address space forces a
// 16-byte array stride, so array<f32,32> would pad each float to 16 B and
// desync from the C++ float[32]. array<vec4<f32>,8> is the same 128 bytes.
//
//   prof[] FAM_SMOOTH   -> 16 fields as 4 vec4s, in H1 SmoothProfile order:
//                          prof[0] = (start_r, flare_r, flare_t, peak_r)
//                          prof[1] = (peak_t, base_t, body_start_r, body_t)
//                          prof[2] = (waist_r, neck_t, neck_r, collar_t)
//                          prof[3] = (collar_bulge, head_t, head_base_r, head_sphere_r)
//          FAM_HERALDIC -> segment k IS prof[k] = (height, r_bot, r_top, shape)
//          FAM_REGULAR  -> unused (hardcoded pawn_profile_radius).
//   pal[]  -> stop k IS pal[k] = (t, r, g, b); first t < 0 ends the list;
//            pal[0].x < 0 means "no palette" (regular -> legacy color).
//   drift  -> (hue_deg, sat, val, _pad).
struct PawnFigure {
    family:    u32,
    seg_count: u32,
    height:    f32,
    radius:    f32,
    prof:      array<vec4<f32>, 8>,
    pal:       array<vec4<f32>, 8>,
    drift:     vec4<f32>,
}

const PAWN_FIGURE_COUNT_WGSL: u32 = 14u;

// SCENE CONSTANTS (CHORD_4) — the render room's mood-cadence block:
// the tier-gains window, the figure profiles, the ribbon window.
// Bound by the scene AND shadow layouts, VERTEX only. Mirrors
// GPUSceneConstants in state.hpp BYTE-FOR-BYTE (4336 B). Offsets:
// tier_gains 0, figure_profiles 192, ribbon 4224.
struct SceneConstants {
    tier_gains: array<AgentTierParams, 4>,
    figure_profiles: array<PawnFigure, 14>,
    ribbon: RibbonState,
}
@group(2) @binding(200) var<uniform> scene_constants: SceneConstants;

const FAM_HERALDIC_W: u32 = 2u;

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
    // each frame in update_camera_vp with a soft time constant. This
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
    // Drift-integrator substrate (cube + sphere use since FIELD_2;
    // sphere spawn seeds spring/drag — bodies/spheres.hpp).
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
    plasticity: f32,           // 196: CONTACT_2 λ (0=elastic; drift→anchor leak rate). Was _pad0.
    // The anchor law (ONE_ANCHOR_1): goals may leap; values only walk.
    // update_cube walks the live param (anchor.xz / pawn_offset.xz)
    // toward these each frame. At rest target == param, glide term 0.
    target_x: f32,             // 200: glide target x. Was _pad1.
    target_z: f32,             // 204: glide target z. Was _pad2.
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
    is_wander: u32,         // 1: the wander brain authors intent (the two fields below); 0: parked. Ridden when config.point_host == 2.
    _pad1: f32,             // RIBBON_2: the brain moved to the head kernel; nothing writes a yaw here
    wander_throttle: f32,   // [0, 1]
    _pad3: f32,
    color_b: vec3<f32>,     // second checker median (CONTRAST)
    hue_spread: f32,        // radians — per-cell hue rotation amplitude (CONTRAST skin; 0 = CB-1 look)
}

// Per-ring pose — written by ribbon_body, read by ribbon_vs / shadow_ribbon_vs
// through render_ring_xforms (g2:143). 48 B stride (vec4, vec4, vec3 + alignment).
struct RibbonRingTransform {
    motor_p0: vec4<f32>,    // PGA motor rotor part
    motor_p1: vec4<f32>,    // PGA motor translator part
    center: vec3<f32>,      // ring world-space center
}

// --- [STATE:patch] PatchParams
// LATTICE_1 — what the bake actually reads, and nothing else. `extent` was
// always Dim::PATCH_EXTENT (make_patch_params wrote the constant), `resolution`
// is now PATCH_HEIGHTFIELD_N, `time` had no reader in either kernel, and
// `master_seed` folded into config.world_seed — the two are one fact written
// together at world birth, and the HEIGHTS already baked from the config copy
// while the cells read the struct, so folding also retires a disagreement that
// could survive one frame. 32 B -> 16 B, and the twin is the C++ struct's.
struct PatchParams {
    origin: vec2<f32>,      // world XZ of the patch CENTER
    layer: u32,             // heightfield / cell-color array layer
    _pad0: u32,
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
// the compile gate/Dawn objects.
const TILE_GRID_CAPACITY: u32 = 1024u;

struct TileGrid {
    origin_x: i32,         // grid-space X of entry [0][0]
    origin_z: i32,         // grid-space Z of entry [0][0]
    side: u32,             // grid dimension (runtime; ceiling = state.hpp TILE_GRID_SIDE)
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
// Raw uniform [0,1] → raw^0.72, node mean 0.58 (mean = 1/(1+e)).
// The exponent is SOLVED, not tasted: half the world reads as checkers.
// The NODE split is not the VISIBLE split — composite_cell_color's doors
// are smoothsteps and the scatter door opens at 0.35, so 31% of the field
// above the 0.70 threshold delivers 50% visible checker coverage.
fn mode_tendency_at_node(node: vec2<i32>) -> f32 {
    let seed = color_lattice_seed(node, 1u);
    let raw = hash_property(seed, 501u);
    return pow(raw, MODE_BIAS_EXPONENT);
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

// (palette_color_smooth — the palette's governing expression — lives
//  beside its dials at THE TERRAIN_LOOKS PANEL ROW 8, §2.2.)

// --- Discrete cell color system
// (DISCRETE_*_LATTICE_SPACING defined in Color Field Spatial Config block)

// Per-node: roll RGB mean and variance for a discrete color region —
// the region's anatomy: seed color mean and spread.
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
    let cs = PATCH_CELL_SIZE;
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
    mute_signal: u32,
    mute_couplings: u32,
    pawn_speed: f32,
    freeze_sphere: u32,
    fpv_mode: u32,
    // (pawn_tilt_tau belongs to this Interaction run semantically — it sits in
    //  the struct's trailing pad instead; see the note at the tail. Inserting
    //  here would push sun_direction off its 16-byte boundary in THIS room only.)
    // three scalars, not array<f32,3>: core WGSL rejects stride-4 arrays in
    // uniform address space. Same 12 bytes, same offsets; CPU mirror stays
    // float[3].
    world_seed: u32,              // master seed for terrain/zone generation
    sun_direction: vec3<f32>,
    aura_enabled: f32,            // 0.0 = off, 1.0 = on (guards all aura sampling)
    pawn_aura_height: f32,
    fog_density: f32,             // exponential fog coefficient (default 0.003)
    fog_color: vec3<f32>,         // fog/sky color RGB
    fade_alpha: f32,              // 0.0 = no overlay, 1.0 = fully opaque
    fade_color: vec3<f32>,        // transition overlay RGB
    // (pier_count's slot: retired in BATCH G — vec2 align re-pads here,
    //  offsets unmoved; the C++ twin carries the explicit pad.)
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
    indoor_height_cap: f32,  // indoor cap on the GoL cell lift, 0 = disabled. READER: zone_derive_params, once per zone birth (mirror of GPUDesignConfig — last pulse pad repurposed)
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
    point_bubble_radius: f32,   // CONTACT_2 C3a: the point's bounded awareness (rest 80 = contracts/point.hpp)
    cube_plasticity: f32,       // CONTACT_3 K2c: global λ master (rest 1.0 = Idle::CUBE_PLASTICITY_DEFAULT, raised at CONTACT_5 P2b)
    // CLOSURE_PAWN [6] — possessed body's terrain-tilt lag, seconds (0 =
    // instant). Sits at offset 556 in BOTH rooms. (It WAS the struct's last
    // 4 bytes at 588 when written; MOSAIC_0a appended the mosaic tail behind
    // it and 588 is now _pad592_2. Size stays 592.) It reads as an Interaction knob and is grouped with them
    // in spirit, but it cannot sit there: this room aligns vec3<f32> to 16 and
    // the C++ room aligns float[3] to 4, so a field inserted above
    // sun_direction shifts the two mirrors by different amounts. Grow at the
    // TAIL, or pad each vec3 back onto its boundary. (state.hpp carries the
    // matching static_assert.)
    pawn_tilt_tau: f32,
    // ─── THE MOSAIC (MOSAIC_0/1/2) — trencadís dials ─────────────────
    // Mirror of GPUDesignConfig tail (state.hpp) — GROWTH LAW, same
    // commit, same order; sizeof witness 592, UNMOVED by MOSAIC_2 (six
    // dials + two pads became five + three). Rests: Dim::MOSAIC_*.
    // enable: master gate; rests OPEN (MOSAIC_2). No key binds it.
    // shard_size: wu per shard cell (batch-jittered ±30% per entity).
    // passage_scale: the coarse palette lattice.
    // blend: boundary-zone width, in fractions of passage_scale — the
    //   ONE dial for both halves of "a boundary is a zone": near it
    //   jitters the per-shard passage lookup (interleaved tiles,
    //   reaching blend·passage_scale from a face, linear), far it
    //   widens the chromatic lerp (reaching HALF that — mosaic_far
    //   halves it again — with a smoothstep profile). One dial, two
    //   realizations; the widths are not equal and do not need to be,
    //   since the far branch only runs where the veil has erased it.
    // facet: per-shard plate lean on the SHADING normal (the glitter).
    // (radius/icing RETIRED — grain is 1 − veil_t; the veil owns the
    //  band and owns it once.)
    mosaic_enable: f32,
    mosaic_shard_size: f32,
    mosaic_passage_scale: f32,
    mosaic_blend: f32,
    mosaic_facet: f32,
    // TUNE_1 A3 — possessed figure's eye height in world units, authored
    // CPU-side (FPV_EYE_RATIO x the figure's own height) and read by
    // update_camera_vp. This room cannot derive it: scene_constants.figure_profiles
    // (binding 112) is a render-VS uniform and no compute layout binds it.
    // Reuses the first tail pad in place; sizeof 592 unmoved. Was _pad592_0.
    fpv_eye_height: f32,
    // ─── THE FIELD'S DIALS (FIELD_2b) ────────────────────────────────
    // Mirror of GPUDesignConfig tail (state.hpp) — GROWTH LAW, same
    // commit, same order, same types. These were module consts in this
    // room and hand-copied constants in the ribbon's; both copies are
    // gone. THE PANEL (contracts/control_panel.hpp) authors the rests
    // and the boot pins them here, so the rooms cannot drift — the
    // failure Gate E paid for is now structurally impossible.
    // Two _pad592 floats consumed, six appended, two fresh pads to the
    // boundary: sizeof 592 -> 624 (state.hpp carries the witness).
    field_slack: f32,
    field_k: f32,
    field_fmax: f32,
    field_occupier_gain: f32,
    field_authored_gain: f32,
    field_gain_cube: f32,
    field_gain_sphere: f32,
    field_gain_agent: f32,
    // HEM_0 — the possessed figure's boundary inset (world units). Mirror of
    // GPUDesignConfig.pawn_body_radius (state.hpp) — GROWTH LAW, same commit,
    // same order, same type. Reuses the first FIELD_2b tail pad in place, so
    // sizeof 624 is unmoved. Read by behavior_player_controlled's box clamp.
    // Was _pad624_0.
    pawn_body_radius: f32,
    // ─── THE RIBBON'S DIALS (RIBBON_1) — GROWTH LAW, same commit, same
    // order, same types as GPUDesignConfig (state.hpp). THE PANEL
    // (contracts/ribbon_surface.hpp RIBBON_LIVE) authors the rests; the
    // boot pins them here; the organ edits them. The ribbon's two kernels
    // read these and nothing else — the CPU head that used to read the
    // panel directly is gone, so this is the ONE road from the panel to
    // the flight and the rooms cannot drift.
    // _pad624_1 consumed, eleven appended, one fresh pad to the boundary:
    // sizeof 624 -> 672 (state.hpp carries the witness).
    ribbon_max_speed: f32,
    ribbon_yaw_rate: f32,
    ribbon_r_min: f32,
    ribbon_floor_margin: f32,
    ribbon_alt_smooth_dist: f32,
    ribbon_alt_stiff: f32,
    ribbon_climb_rate: f32,
    ribbon_mount_setback: f32,
    ribbon_lookahead: f32,
    ribbon_clear_head: f32,
    ribbon_clear_body: f32,
    ribbon_hands_tau: f32,
    // RIBBON_2 — the wander brain's dials, GROWTH LAW: _pad672_0 consumed,
    // three appended, one fresh pad: 672 -> 688 (state.hpp carries the witness).
    ribbon_wander_soft: f32,        // 668  rad of heading error at which the brain asks full yaw
    ribbon_wander_yaw_max: f32,     // 672  the brain's share of the hands' cap, [0, 1]
    ribbon_wander_arrive: f32,      // 676  wu — a target is reached here; the next is drawn
    ribbon_roam_radius: f32,        // 680  wu — the anchor's disc the targets are drawn on
    // KITE_1 — THE CHASE'S FEED-FORWARD. Mirror of GPUDesignConfig
    // (state.hpp) — GROWTH LAW, same commit, same position, same type.
    // THE PANEL (contracts/point.hpp CAMERA_CHASE_FF) authors the rest and
    // the boot pins it here. Reuses the tail pad in place: sizeof 688 is
    // UNMOVED, so no size pin nor offsetof witness changes. Read by
    // update_camera_vp, RIBBON host only.
    camera_chase_ff: f32,           // 684  [0,1] — 1 cancels the aim ease's trail; 0 restores it
    // KITE_1 — THE WITNESS'S PRESENCE. Mirror of GPUDesignConfig, GROWTH
    // LAW, same commit, same order. THE PANEL (contracts/point.hpp
    // CAMERA_PUSH_GAIN / _RADIUS) authors the rests. The tail pad was spent
    // by camera_chase_ff, so these two append and two fresh pads carry the
    // struct back to its 16-byte boundary: sizeof 688 -> 704 (state.hpp
    // carries the witness). Read by cube_force_witness.
    camera_push_gain: f32,          // 688  wu/s² of shove at the shell's center
    camera_push_radius: f32,        // 692  wu — the shell; 0 shuts the term off
    // ATRIUM_7 — an arch leg's own shell factor. Mirror of
    // GPUDesignConfig.field_arch_slack (state.hpp) — GROWTH LAW, same
    // commit, same order, same type. Read by field_sum's occupier_amg loop.
    // Was _pad704_0.
    field_arch_slack: f32,          // 696
    // ─── The subtraction dials (PANORAMA_1) ──────────────────────────────
    // Mirror of GPUDesignConfig (state.hpp) — GROWTH LAW, same commit, same
    // order, same types. DECLARED HERE AND READ BY NOTHING IN THIS ROOM: the
    // masks gate DRAW CALLS, which is a CPU-side decision at the encoder, and
    // that is the point — a shader-side cull would still pay the pass's
    // vertex work and the meter would read no difference. The mirror is the
    // GROWTH LAW's, not a reader's. Was _pad704_1, consumed in place.
    draw_mask: u32,                 // 700
    shadow_mask: u32,               // 704
    // THE TAP COUNT (PANORAMA_1). 16 = the 4x4 kernel; 4 = its inner 2x2.
    // Mirror of GPUDesignConfig — GROWTH LAW, same commit, same order, same
    // type. Read by sample_shadow_pcf, and the ONE mask field this room
    // actually reads. Was _pad720_0.
    shadow_pcf_taps: u32,           // 708
    _pad720_1: f32,                 // 712
    _pad720_2: f32,                 // 716
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
// the C++ compile gate is WGSL-blind and WGSL cannot include headers
// — so the discipline is: every VALUE lives in exactly ONE room, and
// the other room carries the row as a named pointer only (no cross-language
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
//   ROW 7 — THE MOVEMENT THIRD         values HERE
//   ROW 8 — GOVERNING EXPRESSIONS      text HERE
//   ROW 9 — THE CONTRIBUTOR ROSTER     pointers, both rooms
//
// STAYS OUT (machinery / other jurisdictions): streaming, the tile
// cache, manifold dispatch, bake kernel bodies, spawn/population
// (population_themes' panel), the veil (visibility's jurisdiction),
// GoL/pulse internals (ROW 9 points at the shared tint funnel).

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

// ── ROW 2 — MOTION & MODE REST PINS (values in the C++ room) ───────
// The surface voice's silence. config.terrain_time / the band
// blend+phase-origin arrays / mode_color_shift / mode_checker_scatter
// / mode_palette_{target,intensity,discrete_tier} rest at
// terrain_looks::REST_* (C++ ROW 2), written once by the cartridge
// boot-pin block; nothing else authors them today. terrain_time ≤ 0
// freezes the BAND SUM inside the card's heights pass (ROW 7's
// consumer; TRUEBAND_CONTACT_1) — one of three rest conjuncts, not the
// whole law; the full statement is at write_live_card_heights (§7.3). The mode trio is DRIVERLESS since the gen-1
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
// it. Dials: ROW 5 CHECKER_WANDER / DEBUG_VIEW. Bake passes
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
const MODE_BIAS_EXPONENT: f32 = 0.72;            // solved for a 50/50 checkers/smooth world
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
// in DEBUG_VIEW 4 — the art shows it only after patch regen.
const MODE_WARP_AMP: f32 = 0.0;      // wu displacement; 0 = identity
const MODE_WARP_SCALE: f32 = 240.0;  // wavelength (~2× mode spacing)

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
// ═══ DEBUG_VIEW — THE INSTRUMENT REGISTRY (one const, hot-reload) ═══
// Every instrument branches on this module const, so at 0 each one
// folds out per entry point and costs nothing at runtime.
//   0  ART             the fold-out — no instrument.
//   1  WHEEL METER     all ground painted by the checker wheel as the
//                      shader receives it (angle → hue on the seat
//                      table's recipe: 0° red · 60° yellow · 240°
//                      blue; length → vividness). Static gray under
//                      music = the CPU→GPU path is cut.
//   2  FIELD COVERAGE  green where the live path runs, gray where the
//                      baked composite stands.
//   3  SKIRT PAINT     skirt fragments magenta over the art — where
//                      the perimeter curtains present as pixels.
//   4  ZONE GEOMETRY   the sculpting room: live post-warp mode field
//                      grayscale + patch border lines + red coastline
//                      isoline. The warp tuning view.
//   5  LIVE CARD EYE   the card itself: R = |Δh|, G = GoL lift. Black
//                      at rest; paints under music / near zones.
//   6  SHELL RINGS     the point's influence radii as rings on the
//                      terrain, blended (not replacing) so scale reads
//                      in context. Zero bindings, zero layout — the
//                      rings use config.lod_point_x/z, already in the
//                      terrain FS.
// Slots 3-6 are permanent. Slots 1-2 replace the old CHECKER_DEBUG_VIEW,
// 3-4 the old TERRAIN_DEBUG_VIEW, 5 LIVE_CARD_DEBUG_VIEW, 6
// CONTACT_SHELL_DEBUG. The archived charter penciled 5-7 as SPARSE
// SURVIVAL / MOTION PHASE / GUEST MASKS; none was built and 5-6 are
// spent here.
const DEBUG_VIEW: u32 = 0u;

// ── ROW 7 — THE MOVEMENT THIRD ──────────────────────────────────────
// The surface voice's motion vocabulary (moved here from §1.6 —
// TERRAIN_LOOKS gather; values unchanged). REST pins live in the C++
// room (ROW 2). terrain_time ≤ 0 freezes the BAND SUM only — it is one
// of three rest conjuncts, not the whole law; the full statement is at
// write_live_card_heights (§7.3). Band blend -1 = inactive.

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

// ─── The Movement Third — THE TRUE BANDS ───────────────────────────
// The terrain animates with its OWN waves: the card writer's heights
// pass sums per-band true_band_delta_contribution (TERRAIN_BANDS,
// §1.5), shaped by the WAVE_THRESHOLD pools above and gated by
// get_band_blend's per-band wires. That is what those thresholds are.

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
// THE SLOPE LAW. What stops the pawn is GRADE, not height: a rise is
// impassable when dh/dxz exceeds this. Slope is geometry, so v·dt
// cancels and the verdict is frame-rate independent — the height form
// this replaced was not. It compared a per-FRAME rise against a fixed
// 0.5, so the same dune was a ramp at 60 fps and a wall at 30, and a
// dune was a pier by accident of frame time. Downhill never blocks:
// the test is one-sided. Jean-tunable, panel-destined.
const PAWN_MAX_SLOPE: f32 = 1.75;           // ≈60° — the climbable limit
// A NUMERICAL noise floor, NOT a step allowance: under it, the tilt
// query's own finite-diff jitter dominates dh and the ratio is noise.
const PAWN_SLOPE_NOISE_FLOOR: f32 = 0.05;
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

// Eye height is NOT a constant: it is a ratio of the POSSESSED FIGURE's
// own height, authored CPU-side into config.fpv_eye_height (TUNE_1 A3).
// The constant this replaced was PAWN_HEIGHT + 0.2, which is what the
// ratio still yields, to the bit, on the conventional figure.
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

const SPHERE_MIN_TERRAIN_CLEARANCE: f32 = 5.0;

// RESIDUE_2 [3b]: the sphere's floor over the LIVE ground (flyer
// policy — raw GoL, one card fetch). update_sphere walks pos.y toward
// max(authored orbit y, floor + SPHERE_CLEARANCE) — the walk, not this
// floor, is the operative change while COUPLING_TERRAIN_TO_SPHERE_HEIGHT
// (its snapped +5 sibling above) is unmuted; this floor binds when that
// coupling is muted. Jean's dial, panel-legible.
const SPHERE_CLEARANCE: f32 = 2.0;

// Cubes bob and now drift via Phase-3 behaviors; the drift integrator
// can pull pos below ground if a behavior pushes drift.y negative
// faster than the spring restores it. update_cube clamps drift.y from
// below to keep the cube's base (center minus half-height) at least this
// far above local ground.
const CUBE_TERRAIN_CLEARANCE: f32 = 3.0;

// The anchor-law glide time constant (ONE_ANCHOR_1). update_cube walks
// the live param toward its target by (1 - exp(-dt / TAU)) per frame —
// exponential approach, no per-cube clocks, no from-fields; a retarget
// mid-flight walks from the present by construction. 1.1 ≈ the retired
// 4 s CPU smoothstep's settle feel. Jean-tunable.
const CUBE_GLIDE_TAU: f32 = 1.1;

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



// --- Game of Life Zone Config
const GOL_ZONE_SEED_BAND: u32      = 250u;      // lattice seed for zone decisions
const GOL_ZONE_PROP_SPAWN: u32     = 920u;      // spawn roll
const GOL_ZONE_PROP_TIER: u32      = 921u;      // tier selection roll
const GOL_ZONE_PROP_HEIGHT: u32    = 922u;      // height factor roll

const GOL_ZONE_SPAWN_CHANCE: f32   = 0.60;      // 60% of checkerboard zones
const GOL_ZONE_HEIGHT_CHANCE: f32  = 1.00;      // every zone the roll sees gets height
const GOL_ZONE_MODE_THRESHOLD: f32 = 0.50;      // min interpolated mode for eligibility
                                                  // (above scatter_edge, well into discrete)
struct GoLTierParams {
    // --- Rule
    // Conway B/S as a bitset: bit n = birth on n neighbours, bit 9+n =
    // survival on n. B3/S23 is 0x1808u.
    // L3 MIRROR: bodies/gol_zones.hpp GoLTierProfile::rule_mask.
    rule_mask: u32,

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

const GOL_TIER_COUNT: u32 = 10u;

// GOL_TEMPO_2 quantized the CPU draw to GOL_TICK_LADDER and re-authored
// four means onto rungs; A MEAN OFF THE LADDER AUTHORS A COIN, NOT A
// TEMPO — a mean sitting in a boundary band splits its zones ~50/50
// across two rungs, so the row has no tempo, it has two. Every mean in
// both tables is now a rung. Flash 0.5 -> 1.0, HighLife 1.2 -> 1.0,
// Cauldron 5.0 -> 4.0 (both rooms), Pulse Sparkle 1.0 -> 1.5.
//                                            rule       dens_μ  σ     tick_μ  σ    spring_μ σ    trans_μ  σ     ht_μ    σ    sv    wt    no_h  cells
// (cells column: authored by UNIFIED_GROUND_1 U5 as "defaults by weight
//  order thirds, 32/24/16"; Jean-tunable per row. That descending-rank
//  pattern held until TUNE_1 A10 re-ranked the weights without touching
//  the cells — the values are unchanged, the pattern is not. See the CPU
//  twin in bodies/gol_zones.hpp for the full note.)
const GOL_TIERS = array<GoLTierParams, GOL_TIER_COUNT>(
    /* 0: PILLARS  */ GoLTierParams(0x1808u,  0.30, 0.05,  16.0, 4.0,   0.5, 0.1,   0.05, 0.01,  30.0, 9.0,  0.30,  0.11, 0u, 16u),
    /* 1: SPARSE   */ GoLTierParams(0x1808u,  0.15, 0.05,   4.0, 1.0,   4.0, 1.0,   0.12, 0.03,  18.0, 6.0,  0.20,  0.17, 0u, 32u),
    /* 2: MODERATE */ GoLTierParams(0x1808u,  0.30, 0.08,   2.0, 0.6,   8.0, 2.0,   0.15, 0.03,   9.0, 3.0,  0.15,  0.09, 0u, 32u),
    /* 3: DENSE    */ GoLTierParams(0x1808u,  0.45, 0.10,   1.0,  0.3, 12.0, 3.0,   0.25, 0.05,   6.0, 1.5,  0.10,  0.03, 0u, 16u),
    /* 4: FLASH    */ GoLTierParams(0x1808u,  0.35, 0.10,   1.0,  0.2, 20.0, 5.0,   0.30, 0.05,   0.0, 0.0,  0.40,  0.03, 1u, 24u),
    /* 5: MONOLITH */ GoLTierParams(0x1808u,  0.20, 0.03,  24.0, 6.0,   0.3, 0.05,  0.03, 0.01,  42.0, 12.0, 0.05,  0.12, 0u, 16u),
    /* 6: GLACIER  */ GoLTierParams(0x1808u,  0.12, 0.03,   8.0, 2.0,   2.0, 0.5,   0.08, 0.02,  24.0, 7.5,  0.25,  0.21, 0u, 24u),
    // GOL_RULES_1 — three rules that are not Conway; GOL_ROWS_1/2/3 then
    // re-authored two of them (Cauldron was named "Walled cities";
    // Plateau was Day & night and took a new mask). Rationale lives with
    // the CPU twin in bodies/gol_zones.hpp; these are the same rows.
    /* 7: PLATEAU  */ GoLTierParams(0x3E1E0u, 0.50, 0.06,   8.0, 2.0,   6.0, 1.5,   0.10, 0.02,  30.0, 8.0,  0.08,  0.09, 0u, 32u),
    /* 8: CAULDRON */ GoLTierParams(0x79F0u,  0.50, 0.05,   4.0, 1.0,   1.2, 0.3,   0.40, 0.08,   5.0, 1.5,  0.15,  0.08, 0u, 24u),
    /* 9: HIGHLIFE */ GoLTierParams(0x1848u,  0.30, 0.05,   1.0,  0.3,  9.0, 2.0,   0.20, 0.04,  10.0, 3.0,  0.22,  0.07, 0u, 32u),
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
    // --- Field
    // Which spatial law writes the per-cell target (PULSE_FIELD_*).
    // L3 MIRROR: bodies/gol_zones.hpp GolPulseTierProfile::field_fn.
    field_fn: u32,

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

const GOL_PULSE_TIER_COUNT: u32 = 4u;

//                                            field                    tick_μ   σ    spring_μ σ    trans_μ  σ    phase_μ  σ    tempo_μ σ    ht_μ   σ    wand_μ  σ    sv    wt    no_h  bnd  cells
// (cells column: UNIFIED_GROUND_1 U5 — 32/16/8 by weight order; Jean-tunable.)
// GOL_TEMPO_2 quantized the CPU draw to GOL_TICK_LADDER and re-authored
// four means onto rungs; A MEAN OFF THE LADDER AUTHORS A COIN, NOT A
// TEMPO — a mean sitting in a boundary band splits its zones ~50/50
// across two rungs, so the row has no tempo, it has two. Every mean in
// both tables is now a rung. Flash 0.5 -> 1.0, HighLife 1.2 -> 1.0,
// Cauldron 5.0 -> 4.0 (both rooms), Pulse Sparkle 1.0 -> 1.5.
const GOL_PULSE_TIERS = array<GolPulseTierParams, GOL_PULSE_TIER_COUNT>(
    /* 0: Breathe  */ GolPulseTierParams( PULSE_FIELD_BREATH,  4.0, 1.0,   4.0, 1.0,   0.20, 0.05,   0.15, 0.05,   0.10, 0.03,   2.0, 0.8,  10.0, 3.0,   0.20,  0.38, 0u, 0u, 32u ),
    /* 1: Sparkle  */ GolPulseTierParams( PULSE_FIELD_BREATH,  1.5,  0.4, 12.0, 3.0,   0.25, 0.05,   0.90, 0.10,   0.60, 0.15,   0.0, 0.0,   5.0, 2.0,   0.50,  0.24, 1u, 0u, 16u ),
    /* 2: Drift    */ GolPulseTierParams( PULSE_FIELD_BREATH,  8.0, 2.0,   1.5, 0.4,   0.10, 0.03,   0.50, 0.15,   0.40, 0.10,   4.0, 1.5,  25.0, 8.0,   0.35,  0.20, 0u, 1u, 8u ),
    // GOL_RULES_1 — the continuous field row. Rationale lives with the CPU
    // twin in bodies/gol_zones.hpp; this is the same row.
    /* 3: Spiral   */ GolPulseTierParams( PULSE_FIELD_SPIRAL,  6.0, 1.6,   8.0, 2.0,   0.30, 0.06,   0.03, 0.01,    0.0, 0.0,   0.0, 0.0,   0.0, 0.0,   0.10,  0.18, 1u, 1u, 32u ),
);


// --- Pawn Safety Force Field
const PAWN_FORCEFIELD_ENABLED: bool = true;

// --- Compile-time feature gates
// These prune heavy dependency chains from update_player_agent's pipeline compilation.
// Set to false to cut compile time when iterating on unrelated features.
// PAWN_FORCEFIELD_RADIUS_* stay as-is — the pawn's own visual forcefield
// expression, distinct from the CONTACT_2 personal-shell family
// (AgentTierParams.personal_radius: the flock + flee social radius).
const PAWN_FORCEFIELD_RADIUS_STATIONARY: f32 = 6.0;  // Radius when not moving
const PAWN_FORCEFIELD_RADIUS_MOVING: f32 = 2.0;      // Radius at max speed
const PAWN_FORCEFIELD_FALLOFF: f32 = 2.0;            // Edge softness (smoothstep width)
const PAWN_FORCEFIELD_SPEED_SCALE: f32 = 1.0;        // How quickly radius shrinks with speed

// === THE SCALE LEDGER (CONTACT_4) ==================================
// Every influence radius, beside the world dimension it derives from.
// A radius with no stated reference is not reviewable -- three below
// were not, and three bodies died of it (the S0 diagnosis).
//
// THE WORLD'S SCALES (the reference column -- cite the owning constant):
//   mosaic cell ........ PATCH_CELL_SIZE            3.125 wu
//   patch .............. PATCH_EXTENT               50 wu
//   pawn body (visual) . PAWN_FIGURES radius        ~0.5 wu
//   cube altitude ...... orbit_height (Gaussian)    indoor <18.75, outdoor 25-75 wu
//   the bubble ......... config.point_bubble_radius 80 wu
//   possess reach ...... POSSESSION_RADIUS          20 wu (agents.hpp)
//   agent eviction ..... AGENT_EVICTION_RADIUS      350 wu
//   floater eviction ... FLOATER_EVICTION_RADIUS    800 wu
//   veil ring / LOD0 ... config.veil_ring / lod0    325 / 175 wu
//   sphere body ........ fe.body_radius (per-inst)  ~1.2-1.5 wu
//
// THE INFLUENCE RADII  (name . value . reference . derivation):
//   CONTACT_CUBE_RADIUS   3.0  . cube alt H<=18.75 . [UNREACHABLE] 3+1.6=4.6 < H (floating cubes)
//   CONTACT_SPHERE_RADIUS 12.0 . sphere body ~1.5  . [WRONG SPACE] influence, not body (S2c retires)
//   CUBE_PART_RADIUS      8.0  . cube alt H<=18.75 . [DEAD] 3D gate: sqrt(64-H^2) imaginary (S2b -> 30)
//   point-source ppr      50   . patch 50 = 1.0P   . [DEAD] personal(30)+bubble(20); uncatchable (S2a -> 20)
//   FLEE_SHELL_FRAC*sum   15   . bubble/poss 20    . body-to-body only (ok)
//   STEER_LOOKAHEAD_WU    4.0  . cell 3.125        . ~1.3 cells of anticipation (a distance, not a gate)
//   BUBBLE_PART_SPEED     4.0  . -- (wu/s)         . camera-host fallback speed, not a radius
//
// THE GATE-FEASIBILITY RULE (the CONTACT_4 lesson): a 3D gate against a
// body at altitude H can only fire if the radius EXCEEDS H. Radius < H
// => dead code, silently. Usable lateral reach = sqrt(R^2 - H^2).
// (S2 removes the flags by fixing the values; these rows stay the record.)
// ===================================================================

// --- Contact collision (TRUEBAND_CONTACT_1; the population panel owns
//     these numbers' biography)
// CONTACT_IMPULSE_CAP is not a radius -- units below. (CONTACT_SPRING retired
// at SHELL_0: row_sphere_push already carried its own 40.0 as SPHERE_PUSH_GAIN,
// and row_occupier -- its last consumer -- now reads OCCUPIER_PUSH_GAIN. One
// name for two unshared facts, which is how the occupier row inherited a
// stiffness sized for somebody else.)
const CONTACT_IMPULSE_CAP: f32 = 6.0;    // max Δv per pair per frame
// dimensionless -- the pawn's emitter authority. Not a radius.
const PAWN_CONTACT_MASS_MULT: f32 = 4.0; // the pawn is heavy: agents yield — consumed by field_sum's emitter scale since FIELD_B2 (the possessed emits, never yields)

// --- The avoidance field (FIELD_2; the phase-B succession landed) ---
// One summation loop (field_sum, hosted in update_other_agents)
// writes field_forces: one vec4 per subscriber. INDEX MAP (mirrors
// Dim::FIELD_SUBSCRIBER_CAP, state.hpp): [0..31] agents by slot ·
// [32..39] spheres · [40..295] cubes — for floaters, lane − 32 is the
// floating_entities index. Body-class presence lives HERE
// (sphere→agent B1, agent↔agent B2, standing geometry B4); the
// point's rows and every flee-dodge stay the influence law's (the
// point arc is undesigned — THE FIELD LAW banner above field_pair
// is the register); the possessed pawn emits to agents only
// (×PAWN_CONTACT_MASS_MULT, B2b) and never subscribes.
// FIELD_SUBSCRIBERS is a DIMENSION, not a dial: it sizes the
// field_forces array, and an array size cannot come from a uniform.
// It stays a const here and is pinned to Dim::FIELD_SUBSCRIBER_CAP by
// a static_assert in state.hpp (the L3 lockstep witness it never had).
// The array bound below reads THIS name — one home in this room.
const FIELD_SUBSCRIBERS: u32 = 296u;   // 32 agents + 8 spheres + 256 cubes
// The dials themselves live at THE PANEL (contracts/control_panel.hpp)
// and arrive as config fields (FIELD_2b): config.field_slack /
// field_k / field_fmax, the two emitter mutes field_occupier_gain /
// field_authored_gain, and the subscriber-class gains
// field_gain_cube / _sphere / _agent — Jean's gate instrument, each
// still zeroing its class independently.
// config.field_occupier_gain scales the standing-geometry terms
//  (shafts + arch legs) before the FMAX clamp — zeroing it mutes
//  standing geometry for EVERY subscriber (FIELD_B4a/b: floaters
//  and free agents alike). The possessed pawn is the exception and
//  always was: it never subscribes, and meets these bodies through
//  occupier_contact in its candidate.
// Authored emitters (FIELD_4): SHELL attractors — spring toward
// radius r0, envelope (1-len/R)^2, zero beyond R; S >= 0;
// config.field_authored_gain is their mute. Tiebreak band: 900+i.

// --- Gradient steering (CONTACT_2 C2a; the whisper before the wall)
// reference: mosaic cell PATCH_CELL_SIZE 3.125 wu -> ~1.3 cells of
// anticipation ahead of the walker. A distance, not a gate.
const STEER_LOOKAHEAD_WU: f32 = 4.0;   // sensing distance ahead of velocity
const STEER_GAIN: f32 = 3.0;           // lateral accel per unit gradient
// The wall is pawn_ground_resolve's SLOPE LAW — and since BATCH E that
// wall is a gradient magnitude too (PAWN_MAX_SLOPE), so these numbers
// and it are finally the same kind of thing. Read them as one band:
// 0.7 no whisper → 1.4 full whisper → 1.75 the wall. Still authored
// here (Jean-tunable), but now deliberately BELOW the wall rather than
// merely unrelated to it:
const STEER_GRAD_LO: f32 = 0.7;        // below: no whisper
const STEER_GRAD_HI: f32 = 1.4;        // above: full whisper (the wall's shadow)

// --- The social split (CONTACT_2 C3a; flee is the point's, shove the body's)
// dimensionless escape gain (body-to-body). < 1 => the wake contracts.
const NONPLAYER_FLEE_GAIN: f32 = 0.8;   // < 1.0: body-to-body flee cascades CONTRACT
// units: wu/s. Camera-host parting fallback (no camera velocity field). Not a radius.
const BUBBLE_PART_SPEED: f32 = 4.0;     // camera-host parting speed (the C3 fallback — no camera velocity field)

// The social shell's FLEE expression is a FRACTION of its SENSING
// expression — sensing (flock) is a long-range perception, fleeing is a
// short-range reflex. 30+30 sensing -> 15 wu flee trigger. Jean-tunable.
// reference: bubble/possess reach 20 wu; 15 < 20 (a shoulder-scale reflex).
// NOTE: this scopes the BODY-TO-BODY gate ONLY; the POINT-source gate is
// separate and was DEAD at 50 wu until S2a.
const FLEE_SHELL_FRAC: f32 = 0.25;   // CONTACT_3 K2a

// Cube PUSH (CONTACT_5 P2b, reformulated TIDY_1 T2b): the point's PRESENCE
// shoves cubes by OCCUPANCY, not approach -- stand under a floating cube and it
// keeps moving until you leave its column. The gate is an INFINITE CYLINDER
// (planar radius, NO vertical window), admitted by a separate REACH test on the
// cube's AUTHORED altitude. A spherical gate would need a radius larger than the
// altitude, dragging the planar reach out with it -- the CONTACT_4 [DEAD]/
// outdoor-caveat trap; the cylinder escapes it. The reach test replaces the old
// CUBE_PUSH_VWINDOW, whose |dy| gate folded ground relief into eligibility
// (audit #7 -- OVERTURNED: authored altitude is terrain-independent).
//   CUBE_PUSH_RADIUS   -- planar reach, shoulder-scale. A TOOL, not a field:
//     small on purpose (~2x the pawn contact shell); a large presence shell
//     plus persistence (lambda=1) would permanently clear a wide disc around
//     the point -- "cubes avoid you", not "shove them freely".
//   CUBE_REACH_CEILING -- altitude eligibility. A cube is shoveable iff its
//     authored mean altitude (orbit_height + bob_amplitude, terrain-independent)
//     is within the ceiling; above it the cube is canopy, left alone. At 30:
//     monoliths (~12) and small cubes (~25) are in reach, medium (~45) and
//     large (~75) are canopy. Raise toward INFLUENCE_PLANAR_ONLY to make every
//     cube shoveable. Jean-tunable.
//   CUBE_PUSH_GAIN     -- presence force per wu overlap (dt-scaled).
const CUBE_PUSH_RADIUS:  f32 = 7.0;   // planar reach (~2x pawn contact shell; a tool, not a field)
const CUBE_REACH_CEILING: f32 = 30.0; // authored-altitude eligibility; a |dy| window would couple terrain relief instead
const CUBE_PUSH_GAIN:    f32 = 25.0;  // presence force per wu overlap
// units: max Δv per frame on the cube's drift. A FRAME-HITCH GUARD, not a
// tuning knob: unreachable at 60 Hz (max 7*25/60 = 2.92); first engages at
// dt = 0.0686 s (14.6 fps). Raising it changes nothing you can see.
const CUBE_PUSH_CAP: f32 = 12.0;
// [TIDY_1 T1e] The hitch-guard claim above, machine-checked: at 60 Hz the max
// cube presence impulse (radius*gain*dt) stays below the cap, so the cap binds
// only on a frame hitch. All three terms are consts, so Tint const-evaluates
// it (a false assertion errors -- verified T1e). The ONE cap that proves
// itself; the other three rows are split across rooms (see the ledger below).
const_assert CUBE_PUSH_RADIUS * CUBE_PUSH_GAIN < CUBE_PUSH_CAP * 60.0;

// Spheres push the POINT (CONTACT_5 P2a). Presence gain 40.0 — the shape the
// retired CONTACT_SPRING carried, so the restored feel matches what Jean liked
// pre-S2c. Not a radius (the shell is fe.influence_radius, per-instance).
const SPHERE_PUSH_GAIN: f32 = 40.0;

// ═══ THE INFLUENCE LAW (CONTACT_5) ══════════════════════════════════
// One body, many callers. The PROFILE selects the shape; the law is
// written once. See the authority table in the P0 banner (CONTACT_5_LOG).
// PRESENCE follows the POINT; EMANATION stays the BODY's (contracts/point.hpp).
//
// THE POINT-CENTERING LEDGER (CONTACT_5 P2c) — every profile below whose
// 'other' is the point reads point_pos() (host-routed: pawn-host -> the
// possessed slot's pos, camera-host -> the camera). The possessed slot is
// read directly ONLY where the term is genuinely the BODY's emanation:
//   point-source flee (others) . other = point_pos()   [PRESENCE -> POINT]
//   cube push (update_cube) ..... other = point_pos()   [PRESENCE -> POINT]
//   sphere push (player/camera) . self  = the point's HOST body; other = the
//                                 sphere (fe.pos)        [emanation of the sphere]
//   agent<->agent flee .......... other = agent_state[k] [BODY pair -- stays;
//                                 the contact half migrated to the field, FIELD_B2]
//   contact mass weight ......... k == possessed_slot -> PAWN_CONTACT_MASS_MULT
//                                 [the pawn BODY's yield authority -- consumed
//                                 by field_sum's emitter scale since FIELD_B2]
// The one remaining possessed-slot read for a point term is the point's
// VELOCITY in the point-source flee's pawn-host branch (the point's velocity
// IS its host's; camera-host uses the BUBBLE_PART_SPEED floor). The deferred
// config.point_vel_x/z would retire even that into a host-agnostic field.
//
// Two response shapes, both already in the tree — the profile selects:
//   PRESENCE (the shove) — force proportional to overlap (r-d); a reaction
//                          to OCCUPANCY; impulse, dt-scaled (the K1 law).
//   APPROACH (the dodge) — force proportional to closing speed v_ap; a
//                          reaction to MOTION; velocity floor, NOT dt-scaled
//                          (K1); the matador tangential split.
//
// dt is a PARAMETER, not a constant: velocity-hosts (agents) pass the real
// dt and add the result straight to velocity; force-integrators (cubes)
// pass dt = 1.0 so PRESENCE returns a raw force, then apply the real x dt
// at their own drift-integration line. This is how one body serves both
// the impulse sites and the acceleration sites (bit-preservation proof in
// CONTACT_5_LOG). CONTACT_5 corrections vs the P1 draft, both proven in the
// log: (1) fall is applied ONCE to the approach v_ap term (the draft
// squared it via min(mag*fall,...) and would have weakened the agent
// dodge — an agent-visible change, forbidden by the P1 gate); (2) the
// matador split is a PROFILE column `tangential` (0.6 flee / 0 radial) so
// the cube parting keeps its straight radial push.
struct InfluenceProfile {
    radius:        f32,   // shell radius (see vwindow)
    vwindow:       f32,   // <= 0 : spherical gate (3D distance)
                          //  > 0 : CYLINDRICAL gate -- planar radius, |dy| < vwindow
    presence_gain: f32,   // overlap term (r-d)*gain, dt-scaled, 0 = off
    approach_gain: f32,   // approach term v_ap*gain, NOT dt-scaled, 0 = off
    falloff_mix:   f32,   // 0 = flat across the shell, 1 = linear (1-d/r)
    cap:           f32,   // max magnitude of the summed response
    yield_share:   f32,   // 0..1 -- how much of it THIS body takes
    tangential:    f32,   // matador split coefficient (0.6 flee, 0 = radial)
    approach_floor: f32,  // isotropic approach-speed the point emanates when its
                          // host has no velocity field (camera-host: BUBBLE_
                          // PART_SPEED); every other row carries 0 (real closing
                          // speed via other_vel).
}

// The uncapped flee rows carry this so the profile table has no empty cell.
const INFLUENCE_NO_CAP: f32 = 1.0e9;
// Sibling sentinel (TIDY_1 T2b): the cube row carries this as vwindow to select
// the CYLINDRICAL gate with an unbounded vertical half-window -- a planar-only
// column. Same 1e9 magnitude as INFLUENCE_NO_CAP, different column (a cap vs a
// vwindow); vwindow <= 0 would flip to the SPHERICAL gate and reinstate the
// CONTACT_4 altitude-vs-reach trap.
const INFLUENCE_PLANAR_ONLY: f32 = 1.0e9;

// Returns the PLANAR response for `self`, in wu/s. The caller adds it to
// velocity (and, where it integrates inline, to position * dt per K1b);
// force-integrators pass dt = 1.0 and scale externally.
// The approach floor is now a PROFILE column (p.approach_floor, T1d): the
// isotropic approach-speed the point emanates when its host has no velocity
// field (camera-host: BUBBLE_PART_SPEED). Every other row carries 0 (the real
// closing speed arrives via other_vel). It is the exact camera-host fallback
// the deferred config.point_vel_x/z would retire; a scalar floor because the
// fallback is direction-agnostic.
fn influence_response(self_pos: vec3<f32>, self_vel: vec2<f32>,
                      other_pos: vec3<f32>, other_vel: vec2<f32>,
                      p: InfluenceProfile, dt: f32) -> vec2<f32> {
    let d3 = self_pos - other_pos;
    // Explicit sum (NOT dot(d3,d3)) so the fma lowering matches the inline
    // sites' `dx*dx + dy*dy + dz*dz` bit-for-bit (CONTACT_5 P1 verify).
    let d2_3d = d3.x * d3.x + d3.y * d3.y + d3.z * d3.z;
    let d2_pl = d3.x * d3.x + d3.z * d3.z;
    // Degenerate coincidence. The test is PLANAR (SHELL_2). What degenerates
    // is `dir`, and `dir` is built from d_pl, so a 3D test misses the case
    // that matters: a body directly ABOVE another. It SUBSUMES the 3D test it
    // replaces -- d2_3d = d2_pl + d3.y*d3.y, so d2_pl <= d2_3d always.
    // What it catches: on a tangential row, dir = (0,0) reached
    // normalize(vec2(0,0)) and returned NaN, uncapped, into a velocity -- and
    // NaN fails every eviction comparison, so the slot never recycled.
    // CONTACT_5 P1 found the neighbourhood ("a cube nearly overhead, |dir| <
    // 1") and ruled for the radial rows: keep the raw dir, let it attenuate
    // to nothing. Returning zero here IS that ruling at the limit.
    if (d2_pl <= 0.0001) { return vec2<f32>(0.0, 0.0); }
    // No max(): past the guard d2_pl > 0.0001 by construction, so the clamp
    // that used to floor this sqrt is provably slack. Consequence worth
    // knowing: |dir| is now exactly 1 on every row. The sub-unit dir P1
    // documented lived only in the 0.01 wu disc this guard now returns from.
    let d_pl = sqrt(d2_pl);
    // THE GATE -- spherical (a ball is a ball) or cylindrical (the column
    // beneath a hovering body: you shove a floating cube by walking under it).
    // Compared in SQUARED space (d2 >= r*r), exactly as the inline sites did
    // (d2 < r*r): comparing sqrt(d2) < r flips a thin f32 band at the shell
    // edge, and on the FLAT flee shell that flip is a full-magnitude impulse
    // (CONTACT_5 P1 verify found this — the reason the gate is squared).
    var d2_gate = d2_3d;
    if (p.vwindow > 0.0) {
        if (abs(d3.y) > p.vwindow) { return vec2<f32>(0.0, 0.0); }
        d2_gate = d2_pl;                                 // cylindrical: planar
    }
    if (d2_gate >= p.radius * p.radius) { return vec2<f32>(0.0, 0.0); }
    let d_gate = sqrt(d2_gate);
    let dir = vec2<f32>(d3.x, d3.z) / d_pl;              // other -> self (RAW; not re-normalized)
    let fall = mix(1.0, clamp(1.0 - d_gate / p.radius, 0.0, 1.0),
                   clamp(p.falloff_mix, 0.0, 1.0));
    // PRESENCE -- occupancy. Impulse: dt-scaled (K1). fall = 1 for the
    // contact rows (falloff_mix 0), so they stay byte-identical.
    var mag = (p.radius - d_gate) * p.presence_gain * dt * fall;
    var esc = dir;
    // APPROACH -- motion. Velocity floor: NOT dt-scaled (K1). fall weights
    // the v_ap term ONLY (once -- the point-flee's proximity, not squared).
    if (p.approach_gain > 0.0) {
        let v_ap = max(p.approach_floor, dot(other_vel, dir));
        if (v_ap > 0.001) {
            let deficit = v_ap * p.approach_gain * fall - dot(self_vel, dir);
            if (deficit > 0.0) {
                mag += deficit;
                // The matador tangential split ONLY when tangential > 0 (the
                // flees, which normalize dir+tang). The cube parting is radial
                // (tangential 0) and uses the RAW dir, never re-normalized --
                // leave esc = dir. (CONTACT_5 P1 verify: normalize() diverged
                // macroscopically for a cube nearly overhead, where |dir| < 1.
                // SHELL_2's planar guard returns from that disc, so |dir| is
                // now always 1 and the divergence is unreachable. The rule
                // stands on its own: a radial row has no direction to split.)
                if (p.tangential > 0.0) {
                    let tang = vec2<f32>(-dir.y, dir.x)
                             * sign(other_vel.x * dir.y - other_vel.y * dir.x + 0.000001);
                    esc = normalize(dir + tang * p.tangential);   // the matador split
                }
            }
        }
    }
    // Scalar-first: fold (min*yield) BEFORE the vec multiply, matching the
    // inline dir*(scalar) grouping (f32 * is non-associative -- CONTACT_5 P1
    // verify: (dir*m)*y vs dir*(m*y) diverged by 1 ULP on ~15% of pairs).
    let s = min(mag, p.cap) * p.yield_share;
    return esc * s;
}

// [TIDY_1 T1c] The profile table AS CODE -- one contiguous block of row_*()
// builders, called from the sites. Dynamic columns (radii, the pair mass
// weight, tier gains) stay parameters; the constant columns live here once.
// STRUCTURE, not values: each row returns the exact struct its site inlined
// (T1c gate: the kernels' backend SPIR-V is unchanged). A fn returning a
// constructed struct is not a runtime-indexed const array -- the shape the
// retired FXC laws forbade (docs/FXC_LAWS_RECORD.md). The banner forbids
// nothing here now; the structure is kept because it is good structure.
fn row_agent_flee(g_self: AgentTierParams, og: AgentTierParams) -> InfluenceProfile {
    return InfluenceProfile((g_self.personal_radius + og.personal_radius) * FLEE_SHELL_FRAC, 0.0,
                            0.0, NONPLAYER_FLEE_GAIN, 0.0, INFLUENCE_NO_CAP, 1.0, 0.6, 0.0);
}
fn row_sphere_push(fe: FloatingEntityState) -> InfluenceProfile {
    return InfluenceProfile(fe.influence_radius, 0.0,
                            SPHERE_PUSH_GAIN, 0.0, 0.0, CONTACT_IMPULSE_CAP, 1.0, 0.0, 0.0);
}
fn row_point_flee(g_self: AgentTierParams, approach_floor: f32) -> InfluenceProfile {
    return InfluenceProfile(config.point_bubble_radius, 0.0,
                            0.0, g_self.flee_gain_player, 1.0, INFLUENCE_NO_CAP, 1.0, 0.6, approach_floor);
}
fn row_cube_push(fe: FloatingEntityState) -> InfluenceProfile {
    // Test A -- REACH: shove only cubes whose AUTHORED mean altitude
    // (orbit_height + bob_amplitude; terrain-independent, NOT the instantaneous
    // bob) is within the ceiling. Folded into radius via select (branchless):
    // out of reach -> radius 0 -> the gate never opens.
    let reach_ok = (fe.orbit_height + fe.bob_amplitude) <= CUBE_REACH_CEILING;
    // Test B -- PLANAR: INFLUENCE_PLANAR_ONLY as vwindow keeps the CYLINDRICAL
    // gate (positive vwindow) with an unbounded vertical half-window, so once
    // Test A admits the cube the gate is purely planar (vwindow <= 0 would flip
    // to the spherical gate and reinstate the CONTACT_4 trap).
    return InfluenceProfile(select(0.0, CUBE_PUSH_RADIUS, reach_ok), INFLUENCE_PLANAR_ONLY,
                            CUBE_PUSH_GAIN, 0.0, 1.0, CUBE_PUSH_CAP, 1.0, 0.0, 0.0);
}

// ── THE OCCUPIER ROWS (BATCH F-B) — the standing bodies' word ──────
// Columns, antennas, and arch legs push walkers as PRESENCE bodies.
// The occupier windows ride agent_room.occupier_cmg / .occupier_amg
// (CHORD_1) — same mesh-param rows the mesh-gen kernels read: one
// authored geometry, one home; the rows and the mesh can never disagree.
// The field (FIELD_2): the ring-pose and ribbon-state windows in, the
// force sum out. Both windows in ride field_bus now (CHORD_2); the
// force sum keeps the storage seat it always had.
@group(2) @binding(10) var<storage, read_write> field_forces : array<vec4<f32>, FIELD_SUBSCRIBERS>;
// The authored table (FIELD_4) — mirrors GPUFieldAuthored in
// state.hpp BYTE-FOR-BYTE (144 B; the static_assert is the
// handshake). Per emitter i: rows[2i] = {x, y, z, S};
// rows[2i+1] = {r0, R, enable, _}.
struct FieldAuthored {
    count: u32,
    _p0: u32,
    _p1: u32,
    _p2: u32,
    rows: array<vec4<f32>, 8>,
}
// THE FIELD BUS (CHORD_2; RIBBON_1 took the rings out — the field reads
// the body's emit table through ribbon_body_read). Mirrors GPUFieldBus in
// state.hpp BYTE-FOR-BYTE (256 B). Offsets: ribbon 0, authored 112.
struct FieldBus {
    ribbon: RibbonState,
    authored: FieldAuthored,
}
@group(2) @binding(9) var<uniform> field_bus: FieldBus;

// THE OCCUPIER SHELL (SHELL_0). The row answers in wu/s and the caller
// integrates once; the body half is the CALLER's own radius, passed in.
// (Retired: OCCUPIER_CONTACT_SKIN 1.6, a population average standing in
// for a body, from when this row also served free agents. FIELD_B4b took
// that consumer to the field; the survivor knows its own radius.)
//
//   OCCUPIER_PUSH_GAIN — PRESENCE, wu/s of separation per wu of overlap.
//     The 0.6 tangential split costs the RADIAL direction cos 31° = 0.857 of
//     every response, so the standoff solves 0.857 × (gain·o + floor) =
//     PAWN_SPEED → o = 0.94 wu of overlap: a body of radius r parks with
//     (r − 0.94) wu of clear air at full walk-in, 0.66 wu at r = 1.6. The
//     FLOOR is (PAWN_SPEED/0.857 − OCCUPIER_DODGE_FLOOR) / r_body ≈ 10.6 at
//     r_body = 1.6; below it the body
//     reaches the shaft and the shell is decorative. Ejection from dead
//     centre runs clamped at OCCUPIER_SPEED_CAP down to o = 1.64, then eases
//     out — ≈0.34 s to FULL exit at R = 3.6 (shaft 2.0 + body 1.6).
//     R-DEPENDENT: a fatter shaft spends longer in the clamped phase.
//     CROSS-ROOM: keyed to PAWN_SPEED, which config.pawn_speed overrides at
//     runtime — raise that and the standoff shrinks in proportion. Nothing
//     but this line holds the two together.
//   OCCUPIER_DODGE_FLOOR — the APPROACH branch's key, and NOTHING MORE than
//     that. A standing body has no velocity field, so v_ap was 0 and the
//     branch could not open. This opens it — but occupier_contact passes
//     BOTH velocity arguments as vec2(0.0), so −dot(self_vel, dir) is
//     identically zero and the term is a CONSTANT 0.5 wu/s outward. It does
//     not read the walk. The 0.6 tangential split likewise has FIXED
//     HANDEDNESS: sign() consults other_vel, which is zero, so it folds to
//     +1 and every shaft is passed on the same rotational side.
//     One thing the floor genuinely earns: it makes full exit FINITE. A pure
//     presence term decays exponentially and only ever approaches the rim;
//     the constant carries the body out.
//     DEFERRED (SHELL_1): a closing-speed dodge costs one argument — pass
//     other_vel = −(walker velocity), putting the column in the walker's rest
//     frame, and magnitude and handedness both fall out with the shared law
//     untouched. It moves the standoff from 0.94 wu of overlap to 0.14, which
//     may read as an invisible wall rather than a dodge. Its own campaign,
//     its own visual gate.
//   OCCUPIER_SPEED_CAP — 2 × PAWN_SPEED. ONE clamp on the SUMMED response
//     (config.field_fmax's pattern), not a per-row cap: columns cluster
//     (PROXIMITY_RADIUS[COLUMN] 60, THRESHOLD 2), so overlapping shells are
//     the ordinary case and N per-row caps are not a speed limit.
const OCCUPIER_PUSH_GAIN:   f32 = 18.0;  // wu/s per wu of overlap
const OCCUPIER_DODGE_FLOOR: f32 = 0.5;   // wu/s — opens the APPROACH branch
const OCCUPIER_SPEED_CAP:   f32 = 30.0;  // = 2 × PAWN_SPEED; clamps the SUM

fn row_occupier(radius: f32) -> InfluenceProfile {
    // The immovable-authority pattern (yield 1.0 on the agent,
    // zero on the occupier; retired row_agent_sphere's shape), with the
    // CYLINDRICAL gate: a column is a vertical body — an agent at any
    // height meets the shaft (the row_cube_push planar precedent).
    // SHELL_0: PRESENCE plus a CONSTANT approach term. The three approach
    // columns were 0.0/0.0/0.0 — the branch could not open, because v_ap is
    // 0 for a body that does not move. These open it, but occupier_contact
    // passes both velocity arguments as zero, so the term is a fixed 0.5 wu/s
    // and the split has fixed handedness (see OCCUPIER_DODGE_FLOOR).
    // Uncapped per row; the SUM is clamped once, in occupier_contact.
    return InfluenceProfile(radius, INFLUENCE_PLANAR_ONLY,
                            OCCUPIER_PUSH_GAIN, 1.0, 0.0,
                            INFLUENCE_NO_CAP, 1.0, 0.6, OCCUPIER_DODGE_FLOOR);
}

// ONE consumer since FIELD_B4b: the possessed pawn's candidate
// (behavior_player_controlled, before the box clamp and before
// pawn_ground_resolve, so the body's word and the ground's word compose
// instead of racing). Free agents met these bodies here once; they meet
// them in the field now. THAT RETIRED CONSUMER IS WHY THE ROW READ
// CONTACT_SPRING × dt — a persistent velocity ACCUMULATES an impulse
// across frames, and the candidate never does, so the survivor was
// paying a second dt for a spring nobody re-derived. SHELL_0 re-derives
// it. dt stays a PARAMETER (the cube's force-integrator precedent): the
// caller passes 1.0 for a wu/s answer and integrates once itself.
// body_radius is the caller's own half of the shell.
fn occupier_contact(self_p: vec3<f32>, body_radius: f32, dt: f32) -> vec2<f32> {
    var dv = vec2<f32>(0.0, 0.0);

    // SHAFTS — 32 slots: columns 0–15, antennas 16–31. One field,
    // one law; is_active gates every test; the loop is bounded by the
    // Dim:: cap the array is declared with.
    // EMITTER y := SUBJECT y (SHELL_1), the field's ruling inherited: a
    // column is a vertical body, so the pair is planar and d3.y is 0. It
    // read 0.0 before, which made d3.y the walker's ALTITUDE — and d3.y's
    // only live consumer on this row is d2_3d, the degenerate-coincidence
    // guard. The guard was testing height above sea level. It fired on flat
    // ground by accident and never fired on a hill; since SHELL_0 opened the
    // unconditional tangential path, not firing means normalize(vec2(0,0)).
    for (var i = 0u; i < 32u; i++) {
        let cm = agent_room.occupier_cmg[i];
        if (cm.is_active == 0u) { continue; }
        let prof = row_occupier(cm.shaft_radius + body_radius);
        let r = influence_response(self_p, vec2(0.0),
                                   vec3(cm.center_x, self_p.y, cm.center_z), vec2(0.0),
                                   prof, dt);
        dv += r;
    }

    // ARCH LEGS — 16 arches × 2 bodies at center ± half_span rotated;
    // radius from the leg cross-section halves (thickness/depth) +
    // skin. The SPAN stays open — walking through the doorway is the
    // arch's whole meaning; only the legs push.
    for (var i = 0u; i < 16u; i++) {
        let am = agent_room.occupier_amg[i];
        if (am.is_active == 0u) { continue; }
        let leg_r = max(am.thickness, am.depth) * 0.5 + body_radius;
        let leg = vec2(cos(am.rotation), sin(am.rotation)) * am.half_span;
        let c0 = vec2(am.center_x, am.center_z);
        let prof = row_occupier(leg_r);
        let r1 = influence_response(self_p, vec2(0.0),
                                    vec3(c0.x + leg.x, self_p.y, c0.y + leg.y), vec2(0.0),
                                    prof, dt);
        let r2 = influence_response(self_p, vec2(0.0),
                                    vec3(c0.x - leg.x, self_p.y, c0.y - leg.y), vec2(0.0),
                                    prof, dt);
        dv += r1 + r2;
    }

    // ONE clamp on the SUM (config.field_fmax's pattern). Per-row caps
    // summed are not a speed limit: columns cluster, so a body can sit
    // inside three shells at once. BRANCHLESS — this function inlines into
    // behavior_player_controlled, one line above the box clamp and two
    // above pawn_ground_resolve; the banner forbids new runtime branching
    // in that chain. At dv = 0 the quotient is 0/1e-4 = 0, so the identity
    // holds without a test.
    let m = length(dv);
    return dv * (min(m, OCCUPIER_SPEED_CAP) / max(m, 1e-4));
}

// [TIDY_1 T1e] THE CAP LEDGER -- what each `cap` column guards, and whether the
// guard is machine-checkable. A cap on a PRESENCE row (dt-scaled) guards against
// a frame hitch inflating one impulse; a cap on an APPROACH row would guard a
// dt-INVARIANT term, which cannot run away -- so the flees carry none.
//   CUBE row     -- FRAME-HITCH GUARD. Max impulse at 60 Hz is radius*gain*dt =
//     CUBE_PUSH_RADIUS*CUBE_PUSH_GAIN/60 = 2.92, below CUBE_PUSH_CAP (12); it
//     engages only on a hitch (~14.6 fps). All three terms are consts, so the
//     claim ships as a `const_assert` at the CUBE_PUSH_CAP definition -- the one
//     row that compiles its own feasibility proof.
//   OCCUPIER row -- SUM-CLAMPED, not per-row capped (SHELL_0). The row carries
//     INFLUENCE_NO_CAP and occupier_contact clamps the summed response once at
//     OCCUPIER_SPEED_CAP -- the field's ceiling pattern, for the same reason:
//     shells overlap, and N per-row caps bound nothing. Supersedes the CONTACT-
//     rows entry: FIELD_B1/B2 had already migrated agent<->agent and
//     agent<-sphere presence to the field, and row_occupier was the last
//     CONTACT-cap consumer standing.
//   SPHERE row   -- LIVE LIMITER, not a guard. Max impulse is
//     fe.influence_radius*SPHERE_PUSH_GAIN*dt; at the typical mu=8 that is 5.33
//     at 60 Hz (under CONTACT_IMPULSE_CAP 6) but crosses it below 53.3 fps, and
//     at mu>=9 it binds at 60 Hz. This cap is a real clamp on the restore speed,
//     engaged in normal play -- labelled, no value change. (Also uniform-gated
//     by influence_radius, hence likewise unassertable.)
//   FLEE rows    -- UNCAPPED (INFLUENCE_NO_CAP 1e9). presence_gain is 0: the
//     flees are pure APPROACH (a velocity floor, NOT dt-scaled), so no hitch can
//     inflate them -- there is nothing for a cap to guard. The 1e9 sentinel is
//     the table's "no cell is empty" placeholder, not a real ceiling.
// No C++ mirror of these WGSL consts: that reintroduces the ungated
// cross-language duplication AUDIT-4 flagged. The const_assert lives with the
// WGSL that uses it; consolidation (contact/sphere) is the enforcement path.


// §2.3 MUTING CONTROL

// --- Coupling bit flags

const COUPLING_TERRAIN_TO_PAWN_Y:            u32 = 1u << 1u;
const COUPLING_TERRAIN_TO_PAWN_TILT:         u32 = 1u << 2u;
const COUPLING_PAWN_TO_CAMERA_TARGET:        u32 = 1u << 3u;
const COUPLING_INPUT_MOVES_PLAYER:           u32 = 1u << 4u;
const COUPLING_INPUT_ORBITS_CAMERA:          u32 = 1u << 5u;
const COUPLING_INPUT_ZOOMS_CAMERA:           u32 = 1u << 6u;
const COUPLING_TERRAIN_TO_SPHERE_HEIGHT:     u32 = 1u << 14u;
const COUPLING_PAWN_TO_SUN_VP:               u32 = 1u << 16u;

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
// true = the CAMERA hosts the point (free-fly); false = a BODY hosts
// (PAWN kite, or RIBBON seat — every body-path read below is
// unchanged: the possessed slot rides the seat when the ribbon hosts).
fn point_camera_hosted() -> bool {
    return config.point_host == 1u;
}

// RESIDUE_3: riding is a host. The mount gate reads the ONE host
// value — the signal's sky block carries POSE only.
fn point_ribbon_hosted() -> bool {
    return config.point_host == 2u;
}


// §3 COUPLINGS

// §3.1 signal → terrain

// --- [COUPLING:signal.polyphony→terrain:amplitude]
// DRIVERLESS since gen-1 retirement (the 8th capability — raw
// signal.stats[0] terrain-amplitude coupling, retired M1-C). Revive
// only as a deliberately designed gen-2 idiom; direct shader reads of
// the signal bypass canvas and bank (sovereignty decision, parked).

// §3.2 signal → entities

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
//
// THE PAWN DOES NOT TINT THE GROUND (TUNE_2 B1). "A colour under the pawn"
// is a PERCEPT, and a percept is only silenced when every producer of it is.
// The whole producer list, so the next reader finds all of it from any one:
//
//   1. This pawn FF tint — ZONE_PAWN_TINT x ZONE_PAWN_TINT_STRENGTH, mixed
//      into living GoL cells in patch_terrain_fs. SILENCED here: strength 0.
//   2. The pawn aura's tint — PAWN_AURA_DEFAULT.tint_strength in
//      bodies/pawn.hpp, which scales the GBA channels the aura kernel
//      writes. SILENCED by TUNE_1 A4 (0.0f), and it stays so.
//   3. zone_extrusion_fs's own copy of (1) — RETIRED with the whole separate
//      extrusion mesh in [U4a]; GoL is the ground now, so there is no second
//      fragment shader to keep in step. This is what closed charter C6-F1.
//
// NOT a tint, and deliberately still live: the aura's height brighten
// (aura.r * 0.15, plus the normal perturb) in patch_terrain_fs. It adds the
// SAME amount to R, G and B, so it moves brightness and not hue, and it is
// part of the height effect this ruling preserves.
//
// COLOURS ARE KEPT, ONLY STRENGTHS GO TO ZERO — the effect returns by
// restoring the number below, with its authored purple intact.
//
// The sphere is NOT ruled on and keeps its gold at 0.5.
const ZONE_PAWN_TINT: vec3<f32> = vec3(0.4, 0.2, 0.5);     // purple shift near pawn (authored; kept)
const ZONE_SPHERE_TINT: vec3<f32> = vec3(0.5, 0.35, 0.0);  // gold shift near sphere
const ZONE_PAWN_TINT_STRENGTH: f32 = 0.0;                  // TUNE_2 B1: was 0.6 — silenced
const ZONE_SPHERE_TINT_STRENGTH: f32 = 0.5;

// --- Pawn GoL suppression radii (shared by the compute policies and the
// --- two patch VS — the three callers of pawn_gol_suppression below)
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

// THE WITNESS'S CARVE (KITE_1) — the eye is a SECOND suppression center.
// Cells duck under the camera as they duck under the pawn, on the same
// radii and the same falloff: the form above IS the law, called at the
// other center. Render-side only (it reads frame_r); the two patch VS are
// its callers, and they max() it into the pawn's factor, which is the
// union of the two footprints — "flat under EITHER" — and leaves the
// factor on its own [0,1] scale.
//
// Two gates make this the WITNESS's and not the point's:
//
//   HOST. Off in free-fly. The revision camera is a ghost and emanates
//     nothing — contracts/point.hpp: presence follows the point, emanation
//     stays the body's. (In that host the eye and the point coincide
//     anyway, so the term would carve twice in one place and nowhere else.)
//   HEIGHT. Faded out as the eye rises, so an aerial camera does not mow
//     the field. The vertical scale is the suppression's own OUTER radius,
//     used as the vertical half of the same reach: a lens more than one
//     reach above the ground is OVER the field rather than beside it, and
//     two reaches up it is gone. At the idle pose that is 15 sin(0.4) ≈
//     5.8 wu of eye height — full carve; zoomed to CAMERA_MAX_DISTANCE it
//     is ≈ 39 — none.
//
// The scale is the reach because there is no cell-lift ceiling in this
// tree to be the scale instead: config.indoor_height_cap is indoor-only
// with 0 as its disable sentinel, and GOL_HEIGHT_FACTOR_MAX is a per-cell
// multiplier, not a height. A reach is a named home and needs no new one.
//
// Returns exactly 0.0 when either gate shuts, so max()-ing it in is
// BIT-IDENTICAL to the pawn's factor alone wherever the witness is silent.
fn witness_gol_suppression(world_xz: vec2<f32>, local_ground_y: f32) -> f32 {
    if (point_camera_hosted()) { return 0.0; }
    let eye = frame_r.camera.pos;
    let fade = 1.0 - smoothstep(ZONE_SUPPRESS_OUTER, 2.0 * ZONE_SUPPRESS_OUTER,
                                eye.y - local_ground_y);
    return pawn_gol_suppression(world_xz, eye.xz) * fade;
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

@group(2) @binding(42) var<uniform> pyramid_instances: PyramidArray;

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


// --- Ground Architecture: contributor and policy ids ---
//
// Mirror of contracts/ground_architecture.hpp. Shader code references
// contributors and policies by these symbols. The canonical ids and
// policy bitmasks live on the C++ side; these consts exist so WGSL
// can refer to the same numeric values. Keep in sync with POLICIES[]
// in that header.


const POLICY_FLYER                : u32 = 1u;
const POLICY_WALKER               : u32 = 2u;
const POLICY_WALKER_TILT          : u32 = 3u;
const POLICY_WALKER_AGENT         : u32 = 4u;
const POLICY_WALKER_WITNESS       : u32 = 6u;





// Combined height + complexity — avoids evaluating terrain lattice waves twice.
// terrain_height_and_complexity does the same lattice work as terrain_height_at
// but also accumulates the complexity metric. Used by the patch bake.
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
    let height = hc.x * mods.x + mods.y + contrib_pyramids_at(world_xz);
    return vec2(height, hc.y);
}

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
//   both render stages and compute stages
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
// side samples in patch_terrain_vs.
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
// its own call site (a compile-time constant choice of function), so a
// specialization carries only its own policy's contributor set and every
// backend drops the rest — ordinary dead-code elimination of a uniform
// branch, not one compiler's kindness. (The pricing this once carried
// was FXC's; docs/FXC_LAWS_RECORD.md §PROBATE.) Runtime policy dispatch
// is deliberately avoided — see contracts/ground_architecture.hpp (POLICIES[]).
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




// --- Baked heightfield: all static, no dynamic, no deformation ---

// POLICY_BAKED_HEIGHTFIELD — what the cached patch heightfield texture caches.
// Contributors: contrib_static_base_at + CONTRIB_PYRAMIDS.
// Typical consumers: any compute that wants the ground-without-dynamics.
//   The texture variant is sample_terrain_y_at.
// Notes: must stay consistent with ground_formed_with_complexity (what
//   the patch bake evaluates) — same contributor set.

// ─── The shared dynamic-overlay stack ───────────────────────────────
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
//
// Implementation: evaluates contrib_gol_zones_at ONCE and applies the
// pawn-centered suppression factor inline — algebraically identical to
//   h += contrib_gol_zones_at(xz);
//   h -= contrib_gol_suppression_at(xz, consumer_pos);
// but avoids a second full pass over the GoL zone loop — one traversal
// of the zone set instead of two, which is a saving on every backend and
// at every unroll factor. (Shaped under FXC — retired, PIVOT_0;
// docs/FXC_LAWS_RECORD.md §PROBATE.) See contrib_gol_suppression_at for
// the standalone subtractive form.
fn query_ground_walker(xz: vec2<f32>, qi: QueryInputs) -> f32 {
    return query_ground_walker_pair(xz, qi).x;
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
    return query_ground_walker_pair(xz, qi).y;
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
// THE WALKER LAW, STATED ONCE. query_ground_walker and
// query_ground_walker_tilt are the .x and .y of this function; the policy
// arms reach the law through them. Before PRUNING_1 P2 the law was written
// out three times — here, and again in each of those two — and the claim
// that the three agreed was held by a comment. Now it is held by there
// being one of them.
//
// .x = walker: the shared world stack + the mover-anchored self-aura, so
//      the pawn stands on its own aura peak without reading the grid.
// .y = tilt:   the same stack WITHOUT the aura — the self field is a
//      constant scalar with zero tilt gradient, excluded for clarity.
//
// GoL carries inline pawn-centered suppression, equivalent to
//   contrib_gol_zones_at(xz) - contrib_gol_suppression_at(xz, consumer_pos)
// but evaluating the zone loop once instead of twice, kept as the SINGLE
// term gol*(1-supp) so the arithmetic is bit-stable. Walker intent: "GoL
// doesn't push me up into the air while I'm standing on it."
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

// POLICY_WALKER_WITNESS — THE CAMERA'S FLOOR (KITE_1).
// Contributors: static_base + CONTRIB_PYRAMIDS + CONTRIB_GOL_ZONES +
//   CONTRIB_TERRAIN_WAVES + CONTRIB_RADIAL_PULSES + CONTRIB_PAWN_AURA
//   (external form) - CONTRIB_GOL_SUPPRESSION (subtractive, centered on
//   qi.consumer_pos — the EYE — and height-faded).
// Typical consumers: update_camera_vp's clearance clamp.
// Notes: contributor for contributor this is POLICY_WALKER. What differs is
//   the REALIZATION, and both halves of the difference say the same thing —
//   the consumer is the WITNESS, not the body.
//
//   THE AURA IS EXTERNAL, not the self scalar. The eye is not standing at
//   the dome's peak; it is passing over the dome's SHAPE, and the shape is
//   the one patch_terrain_vs extrudes. That is what makes the aura a floor
//   exactly as terrain is: the camera rides the visual skin.
//
//   THE SUPPRESSION IS THE EYE'S, under the render carve's own height fade.
//   THE FLOOR IS THE PICTURE: witness_gol_suppression flattens cells under
//   the eye in the VS, and this flattens the same cells in the clamp. If
//   only one of them did it, the eye would either hover a cell's height
//   over a field the picture had already flattened, or sink into one the
//   picture had left standing. The fade's datum is this xz's surface BEFORE
//   the lift — the analytic twin of the VS's world_pos.y at the same point,
//   aura included.
//
//   Only the pawn's center is absent, and it costs nothing where this
//   function is read: the clamp samples at the EYE's own xz, where the
//   eye's factor saturates at 1 inside ZONE_SUPPRESS_INNER and the render's
//   max() of the two returns that same 1. The two rooms agree exactly at
//   the one point the floor is asked about.
//
//   The stack is evaluated twice — once at gol 0 for the datum, once for
//   the answer. This kernel is one invocation a frame; clarity is the right
//   purchase, and the second evaluation keeps manifold_overlay_stack the
//   single authored fold rather than open-coding a rival sum.
fn query_ground_walker_witness(xz: vec2<f32>, qi: QueryInputs) -> f32 {
    let aura   = contrib_pawn_aura_at_external(xz);
    let ground = manifold_overlay_stack(xz, qi, 0.0) + aura;
    let fade   = 1.0 - smoothstep(ZONE_SUPPRESS_OUTER, 2.0 * ZONE_SUPPRESS_OUTER,
                                  qi.consumer_pos.y - ground);
    let supp   = pawn_gol_suppression(xz, qi.consumer_pos.xz) * fade;
    return manifold_overlay_stack(xz, qi, sample_live_card_gol(xz) * (1.0 - supp))
         + aura;
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
// answered; the interface and cast boundary are this banner's
// (TERRAIN2 Stage 1, in the attic).
//
// THE PARTITION — what Stage 3 inherits vs replaces. INTERFACE =
// this signature + the fold declaration (POLICIES[] in
// contracts/ground_architecture.hpp) + the consumers. CAST = the
// projection, the base-shape bodies, and the four welds (the
// rgba16float texel format, the mesh VS, the normal basis, the
// Y-up/XZ movement + spatial index). Stage 3 inherits the
// interface whole and replaces exactly the cast.
// THE SPHERE, PROVED: dir = normalize(query_pos - center);
// r = base_radius + overlay_fold(dir, policy, qi); position =
// center + dir*r; normal = tangent_perturbed(dir, overlay_grad);
// valid = 1u — a closed manifold has no edge, so every direction
// is inside. Same signature, same consumers, different cast.
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
    // Selectors are the NAMED constants, not bare literals. WGSL accepts a
    // module-scope `const` as a case selector, so the arm and the policy id
    // are bound by the compiler: deleting a POLICY_* takes its arm with it
    // as a compile error, instead of silently re-pointing the arm at its
    // neighbour. (PRUNING_1 P1 5a-i — the renumber trap, removed rather
    // than handled.)
    switch policy {
        case POLICY_FLYER:                { return query_ground_flyer(xz, qi); }
        case POLICY_WALKER:               { return query_ground_walker(xz, qi); }
        case POLICY_WALKER_TILT:          { return query_ground_walker_tilt(xz, qi); }
        case POLICY_WALKER_AGENT:         { return query_ground_walker_agent(xz, qi); }
        case POLICY_WALKER_WITNESS:       { return query_ground_walker_witness(xz, qi); }
        // CELESTIAL/RENDER: baked-path fallback (GROUND_CARD_1 — the inline
        // contributor arm rewired per H5). WGSL requires a default arm.
        default:                          { return sample_terrain_y_at(xz); }
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
// THE DEPTH AXIS CONTAINS THE WORLD (PENUMBRA_2 N2). These three are one
// decision, not three numbers: the eye sits SUN_ALTITUDE up-light of THE
// POINT and the ortho keeps [SUN_NEAR, SUN_FAR] along the light axis, so
// a caster survives only if dot(d, P - C) + SUN_ALTITUDE lands inside that
// window. The sun direction's HORIZONTAL component converts lateral
// distance into depth, and at a low sun almost all of it does.
//
// UMBRA_5 fixed this laterally and left the depth axis at its old size.
// The result, found late in PENUMBRA_1: at MOOD_OPEN_SUNSET the near plane
// clipped everything past ~259 wu down-sun while the visible rim ran to
// 325 — a band of visible terrain casting nothing at all.
//
// THE REQUIREMENT, per outdoor mood (the two indoor moods mute the sun
// coupling entirely, so they do not constrain it):
//
//   axial half-need = |d_xz| * R_xz + |d_y| * H
//
//   mood                  |d_xz|  |d_y|    R_xz    half-need
//   MOOD_OPEN_SUNSET       0.966  0.259   395.7 *    398.8
//   MOOD_FINITE_OUTDOOR    0.704  0.710   636.4 †    493.6
//
//   * veil ring 325 + a patch's far corner, 50*sqrt(2) — the caster set is
//     patch-granular, so its supremum is R + 50*sqrt(2) = 395.71 wu.
//   † finite_radius 4 puts the room at +-225 wu; THE POINT may stand in one
//     corner with a caster in the opposite one, so the separation is the
//     room's full diagonal, 450*sqrt(2).
//   H = 64 wu of terrain height half-span: the six TERRAIN_BANDS at
//     mu+3sigma sum to 61.5, and terrain_amp_ceiling is 0 (unlimited)
//     outdoors, so this is a stated worst case rather than a read constant.
//
// Half-range 550 covers the worst mood with 56.4 wu to spare, and the
// point sits at the exact centre so the two clips are symmetric.
// Widening costs nothing but depth precision, and Depth32Float has it to
// spare: one ULP near z = 0.5 is 6.6e-5 wu against a normal offset of
// 0.169-0.513 wu, ~2,600x finer than the smallest thing that reads it.
const SUN_ALTITUDE: f32 = 551.0;   // = SUN_NEAR + 550
// UMBRA_5 bought 4x the texels and split the yield evenly: 1.4x coverage
// here (the composing boundary moves out) and 1.4x crispness at the map
// (SHADOW_TEXEL_WORLD falls to 0.7x its old value). The map centers on
// THE POINT and the camera orbits it, so the uniform sharpness lands
// exactly where the player is looking. Crispness is bought from this
// number, never from tap count — the tuning ladder claws it back at -10%
// per step if the penumbra reads mushy near the camera.
const SUN_HALF_EXTENT: f32 = 420.0;
const SUN_NEAR: f32 = 1.0;
const SUN_FAR: f32 = 1101.0;  // = SUN_NEAR + 2*550 ; range exactly 1100 wu

// One shadow-map texel, in world units, on the sun map. Derived — never
// authored: it is the frustum's width divided by the map's, so it cannot
// drift from either. THE UNIT OF THE SNAP below, and of the normal offset
// in sample_shadow_pcf. (Const-expression, so no uniform reaches for it
// and no bind-group layout grows.)
const SHADOW_TEXEL_WORLD: f32 = 2.0 * SUN_HALF_EXTENT / SHADOW_MAP_SIZE;

// THE FILTER FOOTPRINT — ONE FACT, TWO EXPRESSIONS (PENUMBRA_1 P3).
//
// Tap spacing and normal-offset magnitude are not two decisions. The offset
// exists to walk the sample out of the caster's own surface, so it must cover
// at least half the filter's reach or the filter reaches back in and the
// offset was pointless. Give the footprint one home and derive both from it.
//
// TAP SPACING ABOVE 1 TEXEL IS NEVER AVAILABLE WITH A BILINEAR COMPARISON
// SAMPLER. Width comes from TAP COUNT alone. True of every shipping
// implementation (D3D SampleCmp, Vulkan, Metal all do 4-texel
// bilinear-of-comparisons); the WebGPU spec itself leaves comparison-sampler
// filtering implementation-defined, so this is a fact about hardware rather
// than a guarantee on paper. It does not weaken the rule: spacing <= 1 is
// safe under ANY filter shape, spacing 2 is safe only under a box.
// PENUMBRA_1 P3 shipped a regression by assuming the box:
//
//   A linear-filtered sampler_comparison tap is A TENT, NOT A BOX. The
//   hardware compares the four texels around the sample point and blends the
//   0/1 results bilinearly, so the tap's weight is 1 at its centre and falls
//   linearly to ZERO at +-1 texel. A sum of tents is flat — a partition of
//   unity — if and only if they are spaced <= 1 texel apart.
//
//   At spacing 2 the tents land on one another's zeros. Summed weight at
//   successive integer texels:   1  0  1  0  1
//   Half the map is never read, on a regular alternation, and a comb of
//   period 2 texels projected down a grazing shadow reads as parallel lines.
//
// P3's warrant checked that the taps' SUPPORT INTERVALS touched — [-3,-1],
// [-1,1], [1,3]. They do touch, at points where both tents are zero.
// Touching supports is not coverage: the condition is on the WEIGHTS.
//
// THE WIDTH LADDER, spacing fixed at 1. N taps per axis gives N+1 texels of
// support; at TEXEL_WORLD = 0.20508 wu:
//     3 taps  -1,0,1            4 texels  0.820 wu   ( 9 total)
//     4 taps  -1.5..1.5         5 texels  1.026 wu   (16 total)  <- here
//     5 taps  -2..2             6 texels  1.231 wu   (25 total)  <- N4, held
// Four is the pre-campaign tap count, so its cost is known-shipped rather
// than estimated, and the offsets stay symmetric so UMBRA_8's centring fix
// survives.
//
// Half-texel offsets cannot ride the `offset` parameter — it takes
// const-expression INTEGERS — so they go into the uv, via TEXEL_UV.
const TEXEL_UV: f32 = 1.0 / SHADOW_MAP_SIZE;
const PCF_RADIUS_TEXELS: f32 = 2.5;   // half the 5-texel support; drives the normal offset

fn coupling_pawn_to_sun_vp(pawn_pos: vec3<f32>, direction: vec3<f32>) -> mat4x4<f32> {
    // Sun position: offset from the frustum center opposite to light direction
    let sun_pos = pawn_pos - direction * SUN_ALTITUDE;

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
    //
    // THE SNAP (UMBRA_2). The frustum center is quantized to whole shadow-
    // map texels IN LIGHT SPACE — the only space where the sample grid is
    // axis-aligned. The old snap rounded world XZ to a 2.0-unit grid, which
    // coincides with the light's lattice only when the sun points straight
    // down, and is 7x coarser than a texel even then; between grid lines
    // the sample grid slid freely and every static edge re-rasterized at a
    // shifted threshold each frame. That was the fire.
    //
    // Both {right, true_up} are orthogonal to `direction`, so the center's
    // light-space x/y are dot(right, pawn_pos) and dot(true_up, pawn_pos) —
    // the SUN_ALTITUDE offset contributes nothing to either. Flooring those
    // to texel multiples quantizes the grid exactly: SUN_HALF_EXTENT is
    // SHADOW_MAP_SIZE/2 texels, an integer, so the ortho's own half-extent
    // shift lands on a texel boundary too.
    //
    // z is NOT snapped. Depth is not the sample grid, and caster and
    // receiver ride the same matrix, so a shift there cancels in the
    // compare — leaving it unsnapped keeps SUN_NEAR/SUN_FAR centered on
    // where the point actually is.
    //
    // The GEOMETRY_2 lever, carried forward and re-priced: between texel
    // crossings the light VP is still bit-stable, which WOULD let the CPU
    // reuse static shadow content — no skip is implemented; the pass runs
    // unconditionally every frame. But a texel is now the quantum, not the
    // old 2.0-unit grid, so crossings are ~7x more frequent and the skip
    // buys correspondingly less. Cheaper prize than when it was written.
    let tx = -floor(dot(right,   pawn_pos) / SHADOW_TEXEL_WORLD) * SHADOW_TEXEL_WORLD;
    let ty = -floor(dot(true_up, pawn_pos) / SHADOW_TEXEL_WORLD) * SHADOW_TEXEL_WORLD;
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

// §3.7 GoL → evolution

// --- [COUPLING:gol→next_state] the zone's own birth/survival rule
// rule_mask is a bitset: bit n = birth on n neighbours, bit 9+n =
// survival on n. neighbors is a Moore-neighbourhood count, 0..8 by
// construction, so 9 + n never reaches the shift width.
// B3/S23 — Conway — is 0x1808u, and every row carried that until
// GOL_RULES_1 gave three rows a rule of their own.
fn coupling_gol_next_state(alive: bool, neighbors: i32, rule_mask: u32) -> f32 {
    let n = u32(neighbors);
    let bit = select(n, 9u + n, alive);
    return select(0.0, 1.0, (rule_mask & (1u << bit)) != 0u);
}

// §4 DYNAMICS
// §4.1 PGA MOTOR INTEGRATION
// These functions use Projective Geometric Algebra for elegant transformations.

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
//   update_player_agent, update_other_agents, update_camera_vp, update_sphere
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

// WALLET_1revA — the lighting block. THREE fragment-stage storage
// bindings became ONE uniform: the trio cost three of the entity
// family's eight F-stage storage seats and bought nothing that one
// block does not. The member structs stay defined where they are —
// one fact, one home; only their standalone bindings died.
//
// Uniform-legal by construction: every member is align 16, each
// offset is a 16-multiple, and 848 B is far under the 65,536 B
// uniform binding cap. TWIN: GPULighting in state.hpp (L3 MIRROR,
// static_assert(sizeof(GPULighting) == 848)).
struct Lighting {
    sun    : DirectionalLight,   // offset   0
    points : PointLightArray,    // offset  48
    spots  : SpotLightArray,     // offset 320
}                                // size 848, uniform-legal

// --- Shadow constants

// TWIN: state.hpp Dim::SHADOW_MAP_SIZE (// Lighting) — sizes the
// two depth textures and the atlas tiles. Change BOTH rooms
// together. (L3 MIRROR.)
const SHADOW_MAP_SIZE: f32 = 2048.0;

// SHADOW_BIAS_MIN/MAX deleted (UMBRA_6) — depth bias now lives once, in
// the shared shadow pipeline's depth-stencil state (renderer.hpp). The
// ECONOMY_1 E6 per-texel form these carried did NOT in fact carry UMBRA_5
// for free, though it was credited here with doing so: it tracked
// RESOLUTION only, and UMBRA_5 changed RADIUS too, so it would have landed
// 1.40x short. The full derivation is at the depthBiasClamp assignment in
// renderer.hpp, where PENUMBRA_1 P2 needed it.

// --- Shadow Sampling with 4x4 PCF. Both kernels are now 4x4 at spacing 1
// with half-texel centres; the sun one arrived here second (PENUMBRA_2 N1).

fn sample_shadow_pcf(world_pos: vec3<f32>, normal: vec3<f32>) -> f32 {
    // THE NORMAL OFFSET (UMBRA_7) — what glues the pawn's shadow to its
    // feet. It moves the SAMPLE POSITION along the receiver normal, not
    // the depth: it walks the lookup out of the occluder's own texel
    // instead of lifting the comparison above it, so slope acne goes
    // without contact separation going with it. That is the whole reason
    // it can exist here while the depth nudges could not.
    //
    // Magnitude DERIVES from the filter footprint: it is PCF_RADIUS_TEXELS,
    // half the kernel's 5-texel support, so the offset clears the filter's
    // reach whatever the tap count becomes. Scale is in TEXELS, so UMBRA_5's
    // resize carried it for free and any future one will too.
    //
    // THAT THE STRUCTURE SURVIVED AN ERROR IN ITS CONSTANT IS THE ARGUMENT
    // FOR THE STRUCTURE. PENUMBRA_1 P3 set the radius from a spacing rule
    // that turned out to be wrong; PENUMBRA_2 N1 fixed the kernel and the
    // radius followed as one number, because there is one home for it.
    //
    // THE FLOOR IS THE POINT OF THIS REVISION. UMBRA_7 scaled by
    // (1 - ndotl) alone, which is ZERO on ground facing the sun — and
    // slope-scale is also zero there, so on flat sun-facing ground NOTHING
    // covered the caster/receiver tessellation mismatch (the shadow pass
    // draws terrain at LOD1, the main pass at LOD0, and in a concave dip
    // the coarser chord rides above the finer surface). The 0.33 floor
    // closes that gap, and it closes it in texel units, which is the unit
    // the mismatch actually scales with.
    //
    //   ndotl = 1 (facing sun): 0.83 texels = 0.1692 wu
    //   ndotl = 0 (grazing)   : 2.50 texels = 0.5127 wu
    //
    // `normal` is unit at every caller — the terrain FS normalizes its
    // gradient normal and both entity FS pass normalize(in.normal) — so
    // this scale is exact rather than approximately exact.
    let light_dir = -frame_r.lighting.sun.direction;   // toward the light
    let ndotl     = clamp(dot(normal, light_dir), 0.0, 1.0);
    let offset_w  = normal * (SHADOW_TEXEL_WORLD * PCF_RADIUS_TEXELS
                              * (0.33 + 0.67 * (1.0 - ndotl)));

    // Transform to light clip space
    let light_clip = frame_r.vp.light_vp * vec4(world_pos + offset_w, 1.0);
    let light_ndc = light_clip.xyz / light_clip.w;

    let shadow_uv = vec2(
        light_ndc.x * 0.5 + 0.5,
        -light_ndc.y * 0.5 + 0.5
    );

    // No bias term. It moved to the rasterizer (UMBRA_6) — the shadow
    // pipelines carry depthBiasSlopeScale, which biases against the real
    // depth gradient at write time instead of guessing it here from the
    // receiver normal.
    let current_depth = light_ndc.z;

    // Safety: if somehow outside shadow map, return fully lit.
    //
    // TESTED BEFORE THE TAPS (PANORAMA_0 LIGHT_0). This was the last line of
    // the function — `select(lit, 1.0, out_of_bounds)` — so a fragment outside
    // the map paid all sixteen textureSampleCompareLevel calls to have their
    // result discarded. Every input to the test (shadow_uv, light_ndc.z) is
    // already in hand here, so the early return is bit-identical and simply
    // stops buying an answer nothing reads.
    let out_of_bounds = shadow_uv.x < 0.0 || shadow_uv.x > 1.0 ||
                        shadow_uv.y < 0.0 || shadow_uv.y > 1.0 ||
                        light_ndc.z < 0.0 || light_ndc.z > 1.0;
    if (out_of_bounds) { return 1.0; }

    let clamped_uv = clamp(shadow_uv, vec2(0.001), vec2(0.999));

    // 4x4 PCF — sixteen taps, fully unrolled, hardware bilinear per tap
    // through the comparison sampler. By construction: no array, no loop
    // to unroll, no dynamic index.
    //
    // textureSampleCompareLevel, NOT textureSampleCompare: it takes no
    // implicit derivatives, so no uniformity diagnostic can fire and the
    // call is legal in any stage.
    //
    // OFFSETS RIDE THE UV, NOT THE `offset` PARAMETER. That parameter takes
    // const-expression INTEGERS, and these centres are half-texel — which is
    // the whole point. Centres at +-0.5 and +-1.5 are spaced exactly 1 texel
    // apart, so the tents sum flat (see TEXEL_UV's banner), and they are
    // symmetric about the fragment, so UMBRA_8's centring fix survives: the
    // centroid sits on the fragment, not half a texel up-light of it.
    //
    // SUPPORT is 5 texels (1.025 wu) — which texels get read. The VISIBLE
    // penumbra is narrower: an edge sweeps the response 0->1 over the
    // tap-centre span plus one texel of bilinear ramp, i.e. 4 texels =
    // 0.820 wu. Quote the second number at a visual gate; the first is a
    // bounds fact. (For scale: UMBRA_8's 3x3 was 0.615 wu visible, P3's
    // banded 3x3-at-spacing-2 was 1.025, pre-campaign was 1.171.)
    //
    // Sixteen taps is the pre-campaign COUNT, so the texture-op cost is
    // known-shipped — though not the memory traffic: UMBRA_5 quadrupled
    // the map behind it, so cache behaviour is an estimate, not a
    // measurement. The 5x5-texel footprint is compact, so it should be
    // small.
    //
    // This is now the SAME arrangement sample_spot_shadow_pcf has always
    // used (its `f32(x) + 0.5` term over -2..=1 gives the identical centres).
    // The spot kernel never combed; only the sun kernel did.
    // THE TAP DIAL (PANORAMA_1). The 4-tap arm takes the INNER four —
    // (±0.5, ±0.5) — which are the same centres the 16-tap kernel already
    // uses, so nothing about the arrangement changes: the tents still sum
    // flat, the centroid still sits on the fragment, and UMBRA_8's centring
    // survives. What changes is the support, 5 texels down to 3, because the
    // comparison sampler makes each tap a 2x2 bilinear.
    //
    // NOTHING ELSE MOVES. PCF_RADIUS_TEXELS stays 2.5 for both arms, so the
    // normal offset is identical and the two arms differ ONLY in the taps —
    // which is what makes the A/B a reading of the taps rather than of two
    // different shadow settings. The bounds guard, the edge fade and the return
    // are shared.
    if (config.shadow_pcf_taps == 4u) {
        var s4 = 0.0;
        s4 += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>(-0.5, -0.5) * TEXEL_UV, current_depth);
        s4 += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>( 0.5, -0.5) * TEXEL_UV, current_depth);
        s4 += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>(-0.5,  0.5) * TEXEL_UV, current_depth);
        s4 += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>( 0.5,  0.5) * TEXEL_UV, current_depth);
        let shadow4 = s4 * 0.25;
        let d4    = max(abs(shadow_uv.x * 2.0 - 1.0), abs(shadow_uv.y * 2.0 - 1.0));
        let fade4 = clamp((1.0 - d4) / 0.12, 0.0, 1.0);
        return mix(1.0, shadow4, fade4);
    }

    var s = 0.0;
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>(-1.5, -1.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>(-0.5, -1.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>( 0.5, -1.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>( 1.5, -1.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>(-1.5, -0.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>(-0.5, -0.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>( 0.5, -0.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>( 1.5, -0.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>(-1.5,  0.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>(-0.5,  0.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>( 0.5,  0.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>( 1.5,  0.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>(-1.5,  1.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>(-0.5,  1.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>( 0.5,  1.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>( 1.5,  1.5) * TEXEL_UV, current_depth);
    let shadow = s * (1.0 / 16.0);

    // EDGE FADE. Distant shadows used to materialize at the frustum
    // boundary instead of assembling — a hard line where the map ran out.
    // Pure arithmetic over the outer 12%, no taps, no branch: measure the
    // Chebyshev distance to the edge in NDC and lerp the whole term back
    // to unshadowed. Uses shadow_uv, not clamped_uv, so it measures true
    // distance to the edge rather than saturating at the clamp.
    //
    // AT TODAY'S NUMBERS THIS IS INSURANCE, NOT AN ACTIVE EFFECT, and it
    // is worth knowing which: the band begins at 0.88 * SUN_HALF_EXTENT =
    // 369.6 wu, while the veil ring — the draw authority, past which no
    // terrain exists to receive — is 325 wu. The band never reaches drawn
    // ground. It becomes live if the tuning ladder claws the radius back:
    // 0.88 * R < 325 means R < 369.3, so the FIRST -10% step (420 -> 378)
    // is still clear and the SECOND (-> 340) is where the fade starts
    // doing visible work. Keep it either way — it is a few instructions,
    // and it is what makes that ladder step safe to take.
    let d    = max(abs(shadow_uv.x * 2.0 - 1.0), abs(shadow_uv.y * 2.0 - 1.0));
    let fade = clamp((1.0 - d) / 0.12, 0.0, 1.0);

    // The out-of-bounds guard moved to the head of the function, where its
    // inputs are first known; it still guards the DEPTH range and anything
    // past the frustum outright, and the fade still smooths the uv approach.
    return mix(1.0, shadow, fade);
}

// --- Directional Light

// TWO NORMALS, ON PURPOSE (PENUMBRA_1 P4).
//   normal     — the SHADING normal. Whatever the surface wants to look like.
//   geo_normal — the GEOMETRIC normal. What the surface actually IS.
// They are the same for every entity. They differ on terrain, where the pawn
// aura bends the shading normal up to ~17 degrees to fake raised ground.
// Normal-offset exists to escape a GEOMETRIC self-shadowing surface, so it
// reads the second; ndotl is a shading fact and reads the first.
fn calc_directional_light(world_pos: vec3<f32>, normal: vec3<f32>, geo_normal: vec3<f32>) -> vec3<f32> {
    let light_dir = -frame_r.lighting.sun.direction;  // toward light
    let ndotl = max(dot(normal, light_dir), 0.0);

    // Sun PCF stays off when spots are active: indoors the sun map's
    // CONTENT is spot atlas tiles 0-1 (ATLAS_1revB draws them there), so a
    // sun-matrix sample would read spot depths. light_vp itself is no
    // longer overwritten (the per-tile copy died in ATLAS_1revB).
    // Restoring indoor sun shadow needs its own map content — future ruling.
    //
    // AND IT STAYS OFF ON GROUND THAT FACES AWAY FROM THE SUN (PANORAMA_0
    // LIGHT_0). Surface turned from a 17-degree sun is dark by GEOMETRY, and
    // the return below says so: the product carries ndotl, so at ndotl == 0 it
    // is zero whatever `shadow` holds. Sixteen taps were being issued to
    // multiply by nothing.
    //
    // THE GUARD IS PROVABLY BIT-NEUTRAL, and reads the SHADING normal on
    // purpose. ndotl is max(dot(normal, light_dir), 0.0), so `ndotl > 0.0` is
    // false exactly where the product is zero; sample_shadow_pcf's own
    // geo_normal argument is unchanged and still decides the normal offset
    // wherever the taps do run. Skipping the call cannot move a pixel because
    // the pixel was never reading the call's answer.
    var shadow = 1.0;
    if (ndotl > 0.0 && frame_r.lighting.spots.count == 0u) {
        shadow = sample_shadow_pcf(world_pos, geo_normal);
    }

    return frame_r.lighting.sun.color * frame_r.lighting.sun.intensity * ndotl * shadow;
}

// --- Point Lights (diffuse only, no shadows)

fn calc_point_lights(world_pos: vec3<f32>, normal: vec3<f32>) -> vec3<f32> {
    var total = vec3(0.0);
    let count = min(frame_r.lighting.points.count, MAX_POINT_LIGHTS);

    for (var i: u32 = 0u; i < count; i++) {
        let light = frame_r.lighting.points.lights[i];
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
// Two full-size depth textures, each split left/right into half-width tiles:
//   shadow_map      (repurposed sun map) → lights 0, 1
//   spot_shadow_map                      → lights 2, 3
// Doubles per-tile resolution vs the old single-texture 2×2 grid,
// with zero extra VRAM — the sun map is idle during indoor moods.
//
// Bias strategy: NO DEPTH BIAS HERE — but there IS a normal offset, added
// by PENUMBRA_1 P5 and described at the end of this banner. (UMBRA_6.)
// The hand-rolled pair that used to
// live here — a base bias divided by clip.w to compensate for hyperbolic
// 1/z depth, plus a per-pixel slope term from the radial light direction —
// is deleted. The rasterizer's slope-scaled bias replaces both, and on the
// perspective question it is strictly better: bias is applied to
// window-space z, AFTER the divide and viewport transform, so the
// hyperbolic 1/z distribution is handled exactly where the old /clip.w was
// a hand-rolled approximation of the same thing.
//
// The old term's CEILING went missing for one campaign and came back:
// slope_bias was capped by SPOT_SLOPE_BIAS_MAX, UMBRA_6 left
// depthBiasClamp at 0.0 (which means NO clamp, not clamp-at-zero) so the
// grazing term ran unbounded, and PENUMBRA_1 P2 restored a ceiling. The
// live value lives in renderer.hpp beside the assignment — not quoted
// here, because a cross-room quotation of a C++ field is a label waiting
// to go stale. The spot path is still not independently SIZED: it
// inherits slopeScale from the sun pipelines through a completely
// different projection. Jean's, at the indoor gate.
//
// The spot pass reuses the SAME pipelines as the sun pass, so it inherits
// that bias without a second home.
//
// THERE IS NOW A NORMAL OFFSET HERE (PENUMBRA_1 P5), and the old ruling
// against one is superseded rather than merely overruled. That ruling said
// an offset "breaks contact shadows (disconnects pawn shadow from feet by
// lifting the comparison point above the occluder depth)" — true of a
// DEPTH lift, which is what this path had at the time. A normal offset
// moves the sample POSITION along the surface instead, so it walks out of
// the occluder's texel without lifting the comparison off contact. That is
// the whole distinction UMBRA_7 was built on, and it applies here too.
//
// What genuinely blocked it was that the frustum is perspective, so texel
// world-size is not constant. Derived per fragment below, it is.

// This kernel's own footprint. NOT PCF_RADIUS_TEXELS — that is the sun's,
// and coupling two kernels through one dial is the pattern this campaign
// family exists to break. Derivation: 4x4 at spacing 1 with the +0.5
// centring term puts tap centres at +-0.5 and +-1.5, and each
// textureSampleCompare carries a 2x2 bilinear footprint reaching +-1, so
// the kernel reaches +-2.5 texels.
const SPOT_PCF_RADIUS_TEXELS: f32 = 2.5;

// `normal` is back (PENUMBRA_1 P5) and it is the GEOMETRIC one — see
// calc_directional_light. UMBRA_6 dropped it with the slope term that was
// its only reader; the spot path then ran with no offset, and with a
// constant bias that was INERT rather than absent (depthBias = 2 on a
// float format bought 6.0e-8 NDC) until P2 deleted it outright. That
// window was the campaign's thinnest ice.
fn sample_spot_shadow_pcf(world_pos: vec3<f32>, geo_normal: vec3<f32>, light_index: u32) -> f32 {
    let light = frame_r.lighting.spots.lights[light_index];

    // THE SPOT NORMAL OFFSET. The frustum is perspective, so texel world-size
    // is not constant — which is why UMBRA ruled this path offset-free. Derive
    // it per fragment instead, from data already in hand:
    //
    //   f — the projection's 1/tan(halfFOV). Recovered from the matrix rather
    //       than mirrored from the CPU: view_proj's first ROW is f * right and
    //       right is unit, so its length IS f. (P1-D: the FOV is per-light,
    //       computed CPU-side from light.outer_cone, and never uploaded as a
    //       scalar — there is no constant to quote, and inventing one would
    //       open a new L3 mirror.)
    //   distance — radial, not axial. Slightly over-estimates inside the cone,
    //       which is the conservative direction for an offset.
    //   tile texels — the X axis, 1024 of them (PORT_5a: SHADOW_MAP_SIZE/2).
    //       P1-D found the projection
    //       carries NO aspect term (proj[0] == proj[5] == f) while the tile is
    //       1024 x 2048, so spot texels are non-square by exactly 2x. The
    //       handoff's rule for that case is to take the LARGER texel
    //       world-size, and X is the coarser axis. Over-offsets on one axis
    //       rather than under-offsetting on the other. The aspect itself is a
    //       HORIZON item; it is not fixed here.
    // spot_f is a DIVISOR and it is unguarded here on purpose: it is
    // guarded by an invariant kept in another room. A zeroed view_proj
    // would make it 0, spot_texel_world +Inf, and offset_w NaN on every
    // zero component of geo_normal — a flat floor's (0,1,0) being the
    // commonest indoor receiver. That cannot be reached: apply_mood_lighting
    // (direction/mood.hpp) runs compute_spot_light_vp for every
    // i < cpuSpotLights_.count before uploading, and calc_spot_light below
    // bounds its loop by the same count, so every light this function is
    // ever called for has a computed matrix. fov is clamped to <= 2.8 rad,
    // so f = 1/tan(fov/2) >= 0.17. If that loop bound and that fill ever
    // stop agreeing, this is where it surfaces.
    let spot_f     = length(vec3(light.view_proj[0][0],
                                 light.view_proj[1][0],
                                 light.view_proj[2][0]));
    let light_dist = distance(world_pos, light.position);
    let spot_texel_world = 2.0 * light_dist / (spot_f * SHADOW_MAP_SIZE * 0.5);

    let spot_dir = normalize(light.position - world_pos);
    let ndotl    = clamp(dot(geo_normal, spot_dir), 0.0, 1.0);
    let offset_w = geo_normal * (spot_texel_world * SPOT_PCF_RADIUS_TEXELS
                                 * (0.33 + 0.67 * (1.0 - ndotl)));

    // Transform to light clip space (perspective)
    let light_clip = light.view_proj * vec4(world_pos + offset_w, 1.0);
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

    // No bias term — it moved to the rasterizer (UMBRA_6).
    let current_depth = light_ndc.z;

    let out_of_bounds = raw_uv.x < 0.0 || raw_uv.x > 1.0 ||
                        raw_uv.y < 0.0 || raw_uv.y > 1.0 ||
                        current_depth < 0.0 || current_depth > 1.0;

    let clamped_uv = clamp(shadow_uv,
        tile_offset + vec2(0.001, 0.001),
        tile_offset + vec2(0.499, 0.999));
    // THIS CLAMP IS LOAD-BEARING. It looks like residue of the bias UMBRA_6
    // deleted — PENUMBRA_1 P6 was written to delete it on exactly that
    // reading — and it is not. Two reasons, either one sufficient:
    //
    // 1. IT IS THE MANUAL CLAMP THE FORMAT REQUIRES. Both shadow textures
    //    are Depth32Float, a FLOATING-POINT resource. HLSL's SampleCmp
    //    reference: on a floating-point resource "the comparison value is
    //    not automatically clamped between 0.0 and 1.0. Therefore, a manual
    //    clamp of the comparison value may be necessary for common
    //    shadowing techniques." Unorm depth formats do clamp; float ones do
    //    not, and the difference is observable on D3D12 (gpuweb#4653).
    //
    // 2. IT IS THE ONLY NaN SCRUBBER ON THIS PATH, and out_of_bounds cannot
    //    be one. Every ordered comparison against NaN is false, so a NaN
    //    current_depth makes `< 0.0 || > 1.0` FALSE and the fragment passes
    //    the guard. clamp(NaN, 0, 1) folds to saturate/min-max, and D3D's
    //    rule is that a min/max with one NaN operand returns the other — so
    //    NaN becomes 0.0, the nearest depth, and the fragment reads FULLY
    //    LIT. Without the clamp a NaN reference fails all sixteen compares
    //    (NaN < stored is false) and the fragment reads FULLY BLACK. The
    //    clamp is what makes this path fail open instead of fail black.
    //
    // Note the taps below are NOT gated by out_of_bounds — they run on every
    // fragment and only the RETURNED value is selected, so this argument
    // reaches the sampler on 100% of fragments, not just survivors.
    //
    // Cost of keeping it: zero. On D3D12 clamp(x, 0, 1) folds into the _sat
    // destination modifier of the instruction that produces x.
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

fn calc_spot_light(world_pos: vec3<f32>, normal: vec3<f32>, geo_normal: vec3<f32>) -> vec3<f32> {
    var total = vec3(0.0);
    let count = min(frame_r.lighting.spots.count, MAX_SPOT_LIGHTS);

    for (var i: u32 = 0u; i < count; i++) {
        let light = frame_r.lighting.spots.lights[i];
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
        let shadow = sample_spot_shadow_pcf(world_pos, geo_normal, i);

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

// THE ICING's t — factored (MOSAIC_2) so the veil and the mosaic's
// grain read ONE value. Point-anchored, XZ, exactly as the veil law
// requires: if grain were eye-anchored while the material dissolved
// point-anchored, a body could be dithering out at the ring while its
// grain insisted it was near.
fn veil_t(world_pos: vec3<f32>) -> f32 {
    return smoothstep(config.veil_ring - config.veil_icing, config.veil_ring,
                      distance(world_pos.xz, render_point_pos().xz));
}

// veil_scale: 1.0 = the family joins the veil (terrain + all entity_fs
// users); 0.0 = a ruled exemption (ribbon — a flown structure that shares
// ENTITY_FS but must stay visible at range). NOT an anchor knob — the
// veil always measures from the point.
// geo_normal is the GEOMETRIC normal — see calc_directional_light. Callers
// whose shading normal IS the geometry (every entity) pass it twice; the
// terrain passes its pre-aura normal.
fn shade_lit(world_pos: vec3<f32>, normal: vec3<f32>, geo_normal: vec3<f32>, base_color: vec3<f32>, veil_scale: f32) -> vec3<f32> {
    // Ambient (always present)
    let ambient = base_color * frame_r.lighting.sun.ambient;

    // Directional light with shadows
    let sun = base_color * calc_directional_light(world_pos, normal, geo_normal);

    // Point lights (diffuse only)
    let points = base_color * calc_point_lights(world_pos, normal);

    // Spot light (cone + distance, indoor moods)
    let spot = base_color * calc_spot_light(world_pos, normal, geo_normal);

    let lit = ambient + sun + points + spot;

    // Fog — the EYE-anchored atmospheric term (a view effect; stays).
    let dist = distance(world_pos, frame_r.camera.pos);
    let fog = 1.0 - exp(-dist * config.fog_density);
    let fogged = mix(lit, config.fog_color, fog);

    // THE ICING (re-ruled) — the POINT-anchored fade band AT the ring,
    // composed AFTER the eye-fog. Cosmetic, not concealment: the RING is
    // the draw authority (nothing is drawn beyond it), and this narrow
    // band [ring−δ, ring] is where draw-set joins materialize. Zero
    // inside ring−δ (pixel-identical there); full fog/horizon color at
    // the ring (the rim melts into sky). strength: 0 in finite/indoor.
    let veil = veil_t(world_pos) * config.veil_strength * veil_scale;
    // THE RIM taste knob: veil_dither > 0.5 → the icing band DITHER-
    // dissolves (geometry condenses) instead of tinting to fog.
    if (config.veil_dither > 0.5) {
        if (veil_dither_noise(world_pos.xz) < veil) { discard; }
        return fogged;
    }
    return mix(fogged, config.fog_color, veil);
}

// ═══ THE MOSAIC (MOSAIC_0/1/2) — trencadís for the mesh-gen families ═══
//
// THE PAINT ANCHOR LAW: pigment evaluates in the frame that owns it.
// Physics is the world's → the live position. Paint is the body's →
// paint_pos = (world_pos.x, in.pos.y, world_pos.z): mesh-authored XZ
// (the grounded lift is Y-only), body-relative Y — immune to ground_y
// and the live card. world_pos remains light/fog/veil's coordinate.
//
// Two scales — the terrain's own structure at the body's size:
// PASSAGES (~12 wu) raffle a small palette; SHARDS (~0.3 wu, F1
// Voronoi) pick one member of it and jitter that by the passage's
// variance. The per-shard normal lean is the pressed-plate glitter.
//
// MATERIAL AT EVERY RANGE (MOSAIC_2): distance takes the GRAIN, never
// the material. Grain is 1 − veil_t — the veil's own band — so a body
// materializes at the ring already ceramic and gains its grain across
// exactly the band where it materializes. At grain 0 the body is its
// passage medians at variance zero and the 27-cell walk does not run.
//
// A BOUNDARY IS A ZONE (MOSAIC_2): the passage is sampled at the
// SHARD'S SITE, so a tile is never cut in half by a colour edge, and
// that lookup is jittered per shard so tiles near a boundary fall on
// either side — an interleaved zone, what a real tiler leaves behind.
// Far, with no tiles to interleave, the zone is chromatic: a lerp with
// the nearest neighbouring passage. AT A BOUNDARY the far form is the
// near form's average exactly — an unresolvable band of alternating
// blue and white tiles IS a blue-white lerp — so that seam cannot pop.
//
// WITHIN a passage the two are NOT the same function, and saying so
// was an overstatement worth correcting: far draws ONE member (roll
// 909) where near is a mixture of the passage's members (roll 904 per
// shard). Only their ensemble means coincide; per passage they differ
// by ~0.2 per channel. What makes THAT seam invisible is not an
// identity but the veil coupling — grain ≤ 0.001 is exactly
// veil_t ≥ 0.999, so the far branch is only ever reached where the
// fragment is ≥99.9% fog and the difference lands ~0.05/255, under
// quantization. THE SEAM'S SAFETY IS THE COUPLING: decouple grain
// from veil_t and this jump becomes visible.
//
// Property run 900–921 (hash_property): 900-902 shard site jitter,
// 903 passage K, 904 shard slot roll, 905 member start, 906 member
// stride, 908 passage variance, 909 passage slot roll (far),
// 910-912 shard colour jitter, 913 entity batch, 914-916 shard facet,
// 917 binder gate, 919-921 passage-lookup jitter. Free: 907, 918.
// Cell-folded seeds — disjoint from the CPU entity registries by
// construction.

const MOSAIC_MEDIANS = array(
    vec3(0.16, 0.32, 0.62),   // cobalt
    vec3(0.20, 0.55, 0.58),   // teal
    vec3(0.85, 0.63, 0.25),   // ochre
    vec3(0.42, 0.56, 0.30),   // moss
    vec3(0.72, 0.30, 0.22),   // rust
    vec3(0.88, 0.78, 0.40),   // sun
    vec3(0.35, 0.25, 0.50),   // violet
    vec3(0.60, 0.75, 0.80),   // sky
);
const MOSAIC_WHITE: vec3<f32> = vec3(0.90, 0.88, 0.84);   // the binder
const MOSAIC_VAR_BASE: f32 = 0.03;   // per-shard jitter floor
const MOSAIC_VAR_SPAN: f32 = 0.10;   // + passage-hashed span

// Fold a 3D lattice cell into the hash_property seed space. Spatial-
// hash primes; bitcast keeps negative cells well-mixed. salt
// decorrelates the shard lattice from the passage lattice.
fn mosaic_cell_seed(c: vec3<i32>, salt: u32) -> u32 {
    return (bitcast<u32>(c.x) * 73856093u)
         ^ (bitcast<u32>(c.y) * 19349663u)
         ^ (bitcast<u32>(c.z) * 83492791u)
         ^ (salt * 2654435761u);
}

const MOSAIC_BINDER_CHANCE: f32 = 0.55;   // fraction of passages that seat white at all

struct MosaicShard {
    seed: u32,
    site: vec3<f32>,   // the site's position, in shard-cell units
}

// F1-only 3×3×3 Voronoi. Returns the nearest jittered site's seed AND
// its position — the position is what lets a whole tile belong to one
// passage (see mosaic_sample). No F2: the grout died at design (Güell's
// binder is pale cement; what separates shards is the shards), and F2
// would double the walk's register pressure for a line we do not draw.
fn mosaic_shard(p: vec3<f32>) -> MosaicShard {
    let base = vec3<i32>(floor(p));
    var best_d = 1e9;
    var out: MosaicShard;
    out.seed = 0u;
    out.site = p;
    for (var dz = -1; dz <= 1; dz++) {
        for (var dy = -1; dy <= 1; dy++) {
            for (var dx = -1; dx <= 1; dx++) {
                let cell = base + vec3<i32>(dx, dy, dz);
                let cs = mosaic_cell_seed(cell, 0u);
                let site = vec3<f32>(cell) + vec3(hash_property(cs, 900u),
                                                  hash_property(cs, 901u),
                                                  hash_property(cs, 902u));
                let dv = site - p;
                let d = dot(dv, dv);
                if (d < best_d) { best_d = d; out.seed = cs; out.site = site; }
            }
        }
    }
    return out;
}

struct MosaicPassage {
    median: vec3<f32>,
    variance: f32,
}

fn mosaic_pcell(p: vec3<f32>) -> vec3<i32> {
    return vec3<i32>(floor(p / max(config.mosaic_passage_scale, 1e-3)));
}

// A passage's small palette, and the raffle that picks one member.
// roll_h is the SHARD's roll near (a passage is a mixture at fine
// grain) and the PASSAGE's own roll far (one member, one clean field).
fn mosaic_passage_at(ps: u32, roll_h: f32) -> MosaicPassage {
    let k = 2u + u32(hash_property(ps, 903u) * 2.999);   // 2..4 COLOURED members
    // MEMBERS BY STRIDE, not by independent picks. Four independent
    // draws from an 8-entry table collide 59% of the time, so a
    // "3-member" passage often had two — half of why every body read as
    // white plus one median. A stride coprime to 8 walks the table and
    // cannot repeat.
    let start  = u32(hash_property(ps, 905u) * 7.999);
    let stride = 1u + 2u * u32(hash_property(ps, 906u) * 3.999);   // 1,3,5,7
    // THE BINDER IS A MEMBER, NOT A MAJORITY. MOSAIC_1's "double
    // weight" resolved to a white share of 2/(k+1) — 67% at K=2, 52%
    // averaged. That is the dominant material, not a binder threaded
    // through. Now: one slot among k+1, and only some passages seat it
    // at all. White share ≈ 14%.
    let binder = select(0u, 1u, hash_property(ps, 917u) < MOSAIC_BINDER_CHANCE);
    let slots  = k + binder;
    // hash_property can return exactly 1.0 (one in 2³²); clamp or the
    // pick runs off the end of the slot list.
    let pick = min(u32(roll_h * f32(slots)), slots - 1u);
    var out: MosaicPassage;
    if (binder == 1u && pick == 0u) {
        out.median = MOSAIC_WHITE;
    } else {
        out.median = MOSAIC_MEDIANS[(start + (pick - binder) * stride) & 7u];
    }
    out.variance = MOSAIC_VAR_BASE + hash_property(ps, 908u) * MOSAIC_VAR_SPAN;
    return out;
}

// THE FAR FIELD — no tiles to interleave, so the boundary zone is
// CHROMATIC: lerp with the nearest neighbouring passage. This is
// exactly the near field's average — an unresolvable band of
// alternating blue and white tiles IS a blue-white lerp — so the two
// halves agree in the limit by construction. That is why the
// transition cannot pop and why this cannot alias. t reaches 0.5 at
// the face, so both sides of a boundary agree there.
fn mosaic_far(p: vec3<f32>) -> vec3<f32> {
    let scale = max(config.mosaic_passage_scale, 1e-3);
    let f = fract(p / scale) - 0.5;
    let a = abs(f);
    var d = vec3(0.0, 0.0, 0.0);
    if (a.x >= a.y && a.x >= a.z) { d.x = sign(f.x); }
    else if (a.y >= a.z)          { d.y = sign(f.y); }
    else                          { d.z = sign(f.z); }
    let w = max(config.mosaic_blend, 1e-3) * 0.5;
    let t = smoothstep(0.5 - w, 0.5, max(a.x, max(a.y, a.z))) * 0.5;
    let psa = mosaic_cell_seed(mosaic_pcell(p), 7u);
    let psb = mosaic_cell_seed(mosaic_pcell(p + d * scale), 7u);
    return mix(mosaic_passage_at(psa, hash_property(psa, 909u)).median,
               mosaic_passage_at(psb, hash_property(psb, 909u)).median,
               t);
}

struct MosaicSample {
    color: vec3<f32>,
    facet: vec3<f32>,   // per-shard plate lean, unscaled — the FS scales it
}

fn mosaic_sample(paint_pos: vec3<f32>, entity_seed: u32, grain: f32) -> MosaicSample {
    var s: MosaicSample;
    s.facet = vec3(0.0);
    // Grain 0: the body is still fully itself — its passage medians at
    // variance zero. The shard exists only to carry jitter, lean and a
    // boundary, so with all three at zero the walk has nothing to do
    // and is not evaluated.
    if (grain <= 0.001) {
        s.color = mosaic_far(paint_pos);
        return s;
    }
    let batch = 0.85 + 0.30 * hash_property(entity_seed, 913u);
    let cell = max(config.mosaic_shard_size * batch, 1e-4);
    let sh = mosaic_shard(paint_pos / cell);
    // THE PASSAGE IS SAMPLED AT THE SHARD'S SITE, not at the fragment.
    // A tile is one piece of ceramic and cannot be cut in half by a
    // colour boundary; per-fragment sampling did exactly that. The
    // lookup is then jittered per shard, so tiles near a boundary fall
    // on either side and the boundary becomes an INTERLEAVED ZONE —
    // what a real tiler leaves behind, and what mosaic_far averages.
    let jitp = (vec3(hash_property(sh.seed, 919u),
                     hash_property(sh.seed, 920u),
                     hash_property(sh.seed, 921u)) - 0.5) * 2.0;
    let look = sh.site * cell + jitp * (config.mosaic_blend * config.mosaic_passage_scale);
    let pa = mosaic_passage_at(mosaic_cell_seed(mosaic_pcell(look), 7u),
                               hash_property(sh.seed, 904u));
    let jitc = (vec3(hash_property(sh.seed, 910u),
                     hash_property(sh.seed, 911u),
                     hash_property(sh.seed, 912u)) - 0.5) * 2.0;
    s.color = clamp(pa.median + jitc * (pa.variance * grain), vec3(0.0), vec3(1.0));
    s.facet = (vec3(hash_property(sh.seed, 914u),
                    hash_property(sh.seed, 915u),
                    hash_property(sh.seed, 916u)) - 0.5) * 2.0;
    return s;
}


// §6.2 PATCH TERRAIN RENDERING
// Instanced rendering of streaming terrain patches. Each instance is one
struct PatchTerrainVarying {
    @builtin(position) clip_pos: vec4<f32>,
    @location(0) world_pos: vec3<f32>,
    @location(1) gradients: vec2<f32>,
    @location(2) patch_uv: vec2<f32>,    // UV within the patch [0,1] for cell sampling
    @location(3) @interpolate(flat) layer: u32,  // heightfield/cell array layer
    // TEMPORARY (INCIDENT #2, I3): 1.0 on skirt ring-copy verts, 0.0 on
    // the surface — wall fragments interpolate toward 1. Remove with
    // the instruments after conviction.
    @location(4) skirt: f32,
    // THE CARRIED ADDRESS (ONE-ADDRESS LAW, charter C8). xy = the owning
    // cell, patch-local, decoded in the VS. z = 1 on the cap and base
    // bands, where every vertex of a primitive shares one cell so flat is
    // exact; 0 on legacy and skirt, whose quads straddle cells and whose
    // fragments keep the world floor. A curtain face stands exactly ON
    // the cell boundary, where cell_address floors into the neighbour.
    @location(5) @interpolate(flat) cell_local: vec3<u32>,
}

// patch_terrain_vs — hand-fused POLICY_TERRAIN_RENDER evaluation.
//
// Inlines the contributor sum for per-vertex performance:
//   patch heightfield texture (cached CONTRIB_STATIC_BASE + CONTRIB_PYRAMIDS)
//   + CONTRIB_PAWN_AURA
//   + CONTRIB_TERRAIN_WAVES
//   + CONTRIB_RADIAL_PULSES
//
// Does NOT include CONTRIB_GOL_ZONES — the patch heightfield caches the
// STATIC ground only. The GoL lift rides the live card's .a and is added
// per-vertex in the VS (UNIFIED_GROUND_1), never baked.
//
// Waves + pulses arrive via the live card (GROUND_CARD_1; true-band
// deltas since TRUEBAND_CONTACT_1): the resolve pass computes the
// gradient the fragment normal needs — nothing wave-shaped is
// evaluated in this VS.
//
// Keep consistent with POLICY_TERRAIN_RENDER: if that policy's mask
// gains or loses a contributor, update this function to match — or
// document the divergence at the mask block. The patch VS runs ~15,000
// invocations per patch (the 65×65 lattice plus the skirt, cap and base
// bands), so a function-call-per-contributor dispatch would dominate
// frame time; that's why this stays hand-fused. (LATTICE_1 corrected the
// old "~256×256" here: that was the retired heightfield's texel count,
// never this VS's vertex count.)
//
// See contracts/ground_architecture.hpp (fused inline evaluations).
@vertex
fn patch_terrain_vs(
    @builtin(vertex_index) vi: u32,
    @builtin(instance_index) patch_id: u32,
    @location(0) visible_id: u32
) -> PatchTerrainVarying {
    // Direct or indirect patch lookup (override-controlled per pipeline
    // variant). DOMESDAY_0 B3: the indirect path reads the visible list
    // as an instance-step vertex attribute — same index stream the
    // g2:62 storage seat used to deliver, fixed-function fetch now.
    var actual_id = patch_id;
    if (USE_PATCH_INDIRECTION) { actual_id = visible_id; }
    let pi = patch_instances[actual_id];

    // THE UNIFIED DECODE (UNIFIED_GROUND_1): legacy grid + skirt copies
    // + cap band + curtain-bottom band — one arithmetic split (Dim::UG_*).
    let d = ug_decode(vi);

    // UV within the patch [0, 1]
    let uv = vec2(
        f32(d.vx) / f32(PATCH_MESH_N),
        f32(d.vz) / f32(PATCH_MESH_N)
    );

    // THE LATTICE IS THE TEXEL GRID (LATTICE_1). d.vx / d.vz index the
    // heightfield DIRECTLY — texel i is lattice point i, both in
    // [0, PATCH_MESH_N] — so there is no uv to remap and no filter to
    // straddle: an exact fetch of the value the bake wrote for this
    // vertex. `uv` stays, because it positions the vertex and feeds
    // patch_uv; it just no longer addresses the texture.
    // .x = height, .yz = gradients, .w = unused (was complexity — swept)
    let height_data = textureLoad(patch_heightfield_array_read,
                                  vec2<i32>(i32(d.vx), i32(d.vz)),
                                  i32(pi.layer), 0);

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
    // live.x = Δh (true-band waves + pulses), live.yz = the full-Δ
    // gradient (TRUEBAND_CONTACT_1: the resolve differentiates the
    // whole scratch — normals shade pulses AND bands)
    // (parity with the old fused overlay; pulse shading = Stage 6).
    let live = sample_live_card(world_pos.xz);
    world_pos.y += live.x;

    // THE CELL LIFT (UNIFIED_GROUND_1): GoL rides the ground itself —
    // the card's .a, nearest at the vertex's OWN cell center, suppressed.
    // BASE verts take no lift (that gap IS the curtain); d.drop subsumes
    // the old skirt ring drop.
    //
    // TWO CENTERS (KITE_1): the pawn's and the eye's, unioned by max — a
    // cell is flat where EITHER stands over it. local_ground for the
    // witness's height fade is this vertex's own ground (heightfield +
    // aura + card, the y just above), which is the ground under the eye to
    // within the reach the carve acts over.
    let supp = max(pawn_gol_suppression(world_pos.xz, render_pawn_pos().xz),
                   witness_gol_suppression(world_pos.xz, world_pos.y));
    let lift = ug_cell_lift(pi.origin, pi.extent, d.cellx, d.cellz) * (1.0 - supp);
    world_pos.y += lift * d.lift_scale - d.drop;

    var out: PatchTerrainVarying;
    out.clip_pos = frame_r.vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.gradients = height_data.yz + live.yz;
    out.patch_uv = uv;
    out.cell_local = vec3<u32>(d.cellx, d.cellz,
                               select(0u, 1u, vi >= UG_CAP_BASE));
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

    // DEBUG_VIEW 5 — THE LIVE CARD EYE. After the rim discard, so the
    // eye respects the veil ring.
    if (DEBUG_VIEW == 5u) {
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
    let owned_texel = select(cell_texel, vec2<i32>(in.cell_local.xy),
                             in.cell_local.z == 1u);
    let addr_used = patch_grid * i32(PATCH_CELL_N) + owned_texel;

    // Color fully composited at gen-time in the cell texture.
    // Alpha carries the cell behavior tag (0.0 = static, nonzero = animated).
    // One-address: loaded at the law texel (was a nearest-neighbor SAMPLE
    // by patch_uv — the second addressing that made the seam expressible).
    let color_sample = textureLoad(
        patch_cell_color_array_read, owned_texel, i32(in.layer), 0);

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
    if (DEBUG_VIEW == 1u) {
        // WHEEL METER: the resultant color as the shader receives it,
        // faded by presence (gray at rest). Static gray under music = the
        // CPU->GPU path is cut.
        base_color = mix(vec3(0.5), config.checker_resultant, config.checker_music_amount);
    } else if (DEBUG_VIEW == 2u) {
        // FIELD COVERAGE: green where the live path runs
        // (has_mode_bias), gray where the baked composite stands.
        base_color = select(vec3(0.45), vec3(0.25, 0.7, 0.35), has_mode_bias);
    } else if (has_mode_bias) {
        // SAMPLE-POINT + ANALYTIC (charter C8): the bake's evaluator,
        // at the cell center — one function, two moments, no cache.
        let cell_center = (vec2<f32>(addr_used) + 0.5) * PATCH_CELL_SIZE;
        base_color = animated_cell_color(cell_center, addr_used);
    }

    // ── The terrain slots, painted AFTER the music branch so each view
    //    shows the live-path truth with music playing. Shading and fog
    //    still compose after — legible.
    if (DEBUG_VIEW == 3u) {
        // SKIRT PAINT — skirt fragments magenta over the art: shows
        // where the perimeter curtains present as pixels.
        if (in.skirt > 0.01) {
            base_color = vec3(1.0, 0.0, 1.0);
        }
    } else if (DEBUG_VIEW == 4u) {
        // ZONE GEOMETRY — the sculpting room, computed LIVE in
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
            let cam_dist = distance(frame_r.camera.pos, in.world_pos);
            let fade = 1.0 - smoothstep(GOL_FADE_NEAR, GOL_FADE_FAR, cam_dist);

            if (fade > 0.01) {
                for (var z: u32 = 0u; z < zone_params.count; z++) {
                    let zp = zone_params.zones[z];
                    if (zp.transition_fraction <= 0.0) { continue; }
                    let zone_corner = zp.origin - zp.extent * 0.5;
                    let local_cell = addr_used - cell_address(zone_corner);
                    // COVERAGE, NOT LATTICE (UG_FIELDS_1 S1). The bounds
                    // test IS the membership test. The retired pre-filter
                    // compared lattice nodes, which assumes a zone never
                    // leaves its 120 wu node; a 32-cell zone is already
                    // 100 wu and fragments across the boundary lost it.
                    // This is also the FS's last MODE_LATTICE_SPACING
                    // reader — the zone's SIZE is now free of the lattice
                    // that places it.
                    if (local_cell.x >= 0 && local_cell.x < i32(zp.grid_size) &&
                        local_cell.y >= 0 && local_cell.y < i32(zp.grid_size)) {

                        let uv = (vec2<f32>(local_cell) + 0.5) / GOL_ZONE_TEX_N;
                        let life_sample = textureSampleLevel(
                            zone_life_read, nearest_sampler, uv, i32(z), 0.0
                        );
                        let color_val = life_sample.x;  // R channel = the cell's spring visual

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
                            let sphere_ff = 1.0 - zone_sphere_ff(in.world_pos.xz, frame_r.sphere_pos);
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

    // THE GEOMETRIC NORMAL, held before the aura touches it (PENUMBRA_1 P4).
    // Everything above this line is the real surface: a bilinear heightfield
    // gradient plus the live card's. The aura below is a SHADING fiction —
    // it bends the normal up to fake raised ground it never raised. The
    // shadow offset needs the geometry, so it takes its copy here.
    let geo_normal = normal;

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

    var out_colour = shade_lit(in.world_pos, normal, geo_normal, base_color, 1.0);

    // DEBUG_VIEW 6 — THE SHELL RINGS: each ring is an influence radius
    // the code actually uses; the grey patch ring is the world's own
    // yardstick. If a shell ring sits OUTSIDE the patch ring, ask
    // whether that influence should really reach a whole patch.
    if (DEBUG_VIEW == 6u) {
        let pxz = vec2(config.lod_point_x, config.lod_point_z);
        let d = distance(in.world_pos.xz, pxz);
        var ring = vec3(0.0);
        // the bubble — the point's social shell (the flee trigger)
        ring += vec3(0.2, 0.9, 1.0)
              * step(abs(d - config.point_bubble_radius), SHELL_RING_WIDTH);
        // the cube PUSH reach (planar column footprint -- CONTACT_5 P2b)
        ring += vec3(1.0, 0.7, 0.2)
              * step(abs(d - CUBE_PUSH_RADIUS), SHELL_RING_WIDTH);
        // one patch, for scale
        ring += vec3(0.5, 0.5, 0.5)
              * step(abs(d - PATCH_EXTENT), SHELL_RING_WIDTH);
        out_colour = mix(out_colour, ring, min(1.0, ring.r + ring.g + ring.b) * 0.75);
    }

    return vec4(out_colour, 1.0);
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

    // Same unified decode as patch_terrain_vs — but NOT the same index
    // buffer. The shadow pass binds the ring IB's clean prefix only
    // (ECONOMY_1 E2, CELL_1 rev2): stride-2 cap lattice + skirt copies.
    // The curtain tail is not drawn here, so curtains again do not
    // cast — caps cast as slabs, walls do not. (UMBRA_3's caster diet
    // survives as density: 2×2 quads per cell.)
    let d = ug_decode(vi);

    let uv = vec2(
        f32(d.vx) / f32(PATCH_MESH_N),
        f32(d.vz) / f32(PATCH_MESH_N)
    );

    // The same exact fetch patch_terrain_vs makes (LATTICE_1): texel i is
    // lattice point i, so the caster reads the drawn surface's own value.
    let height_data = textureLoad(patch_heightfield_array_read,
                                  vec2<i32>(i32(d.vx), i32(d.vz)),
                                  i32(pi.layer), 0);

    let wx = pi.origin.x + (uv.x - 0.5) * pi.extent;
    let wz = pi.origin.y + (uv.y - 0.5) * pi.extent;
    var world_pos = vec3(wx, height_data.x + sample_live_card(vec2(wx, wz)).x, wz);
    // THE CELL LIFT (UNIFIED_GROUND_1) — shadow surface = heightfield +
    // the card + the suppressed cell lift; d.drop subsumes the old skirt
    // ring drop. The caster carries the SAME two centers the visible
    // surface carries (KITE_1): a cell carved out of the picture but left
    // standing in the shadow map casts the shadow of geometry nobody drew.
    // This VS has no aura term, so its local_ground is the aura's height
    // lower inside the dome — under the 15→30 fade that is nothing.
    let supp = max(pawn_gol_suppression(world_pos.xz, render_pawn_pos().xz),
                   witness_gol_suppression(world_pos.xz, world_pos.y));
    let lift = ug_cell_lift(pi.origin, pi.extent, d.cellx, d.cellz) * (1.0 - supp);
    world_pos.y += lift * d.lift_scale - d.drop;

    var out: ShadowVarying;
    out.clip_pos = shadow_light_vp() * vec4(world_pos, 1.0);
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
    // THE PAINT ANCHOR (MOSAIC_1): pigment coordinates. paint_y is the
    // mesh-authored body Y (in.pos.y) — immune to ground_y + the live
    // card; XZ reuses world_pos (the grounded mesh-gen lift is Y-only).
    // The FS assembles paint_pos = (world_pos.x, paint_y, world_pos.z).
    // mosaic_seed 0 = unpainted — every zero-init VS opts out for free;
    // only arch_vs / column_vs write these today.
    @location(3) paint_y: f32,
    @location(4) @interpolate(flat) mosaic_seed: u32,
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



// --- Pawn figure helpers (CLOSURE_PAWN) --------------------------------
// Between the primitive they call (pawn_profile_radius — eval_profile_radius
// calls it) and the two entry points that consume them (pawn_vs,
// shadow_pawn_vs). Locality, not a constraint: module-scope declarations are
// order-independent in WGSL, and this file relies on that in ~40 places.

// Segment shape vocabulary — mirrors STACKED_SHAPES / PawnShape (H1).
fn profile_shape(shape: u32, u: f32, rb: f32, rt: f32) -> f32 {
    let line = rb + (rt - rb) * u;
    let mid  = (rb + rt) * 0.5;
    switch (shape) {
        case 0u:  { return line; }                                        // linear
        case 1u:  { return line - 0.25 * mid * sin(u * PI); }             // concave
        case 2u:  { return line + 0.25 * mid * sin(u * PI); }             // convex
        case 3u:  { if (u < 0.85) { return rb; } return rb + (rt - rb) * ((u - 0.85) / 0.15); } // step
        case 4u:  { let r = max(max(rb, rt), 0.05); return line + 0.55 * r * sin(u * PI); }      // bell
        case 5u:  { return rb + (rt - rb) * (u * u); }                    // flare
        case 6u:  { return line + 0.18 * mid * sin(u * PI * 2.0); }       // ogee
        case 7u:  { let R = max(rb, rt); let x = 2.0 * u - 1.0; return R * sqrt(max(0.0, 1.0 - x * x)); } // sphere
        default:  { return line; }
    }
}

// Smooth: parametric (16 fields as 4 vec4s). For PROF_PAWN values this is
// identical to pawn_profile_radius — the regular pawn never routes here, but
// the parity holds.
fn smooth_profile_radius(fig: PawnFigure, t: f32) -> f32 {
    let p0 = fig.prof[0]; let p1 = fig.prof[1];
    let p2 = fig.prof[2]; let p3 = fig.prof[3];
    let start_r = p0.x;  let flare_r = p0.y;  let flare_t = p0.z;
    let peak_r  = p0.w;  let peak_t  = p1.x;
    let base_t  = p1.y;  let body_start_r = p1.z;
    let body_t  = p1.w;  let waist_r = p2.x;
    let neck_t  = p2.y;  let neck_r  = p2.z;
    let collar_t = p2.w; let collar_bulge = p3.x;
    let head_t  = p3.y;  let head_base_r = p3.z; let head_sphere_r = p3.w;
    if (t < flare_t)  { return mix(start_r, flare_r, t / flare_t); }
    if (t < peak_t)   { let u = (t - flare_t) / (peak_t - flare_t); return mix(flare_r, peak_r, sin(u * PI * 0.5)); }
    if (t < base_t)   { let u = (t - peak_t) / (base_t - peak_t); return mix(peak_r, body_start_r, u * u); }
    if (t < body_t)   { let u = (t - base_t) / (body_t - base_t); let e = smoothstep(0.0, 1.0, u); return mix(body_start_r, waist_r, e); }
    if (t < neck_t)   { let u = (t - body_t) / (neck_t - body_t); return mix(waist_r, neck_r, u); }
    if (t < collar_t) { let u = (t - neck_t) / (collar_t - neck_t); return neck_r + collar_bulge * sin(u * PI); }
    if (t < head_t)   { let u = (t - collar_t) / (head_t - collar_t); let y = u * 2.0 - 1.0; let sr = sqrt(max(0.0, 1.0 - y * y)); return head_base_r + head_sphere_r * sr; }
    let u = (t - head_t) / (1.0 - head_t); return mix(head_base_r, 0.0, smoothstep(0.0, 1.0, u));
}

// Heraldic: segment walk (<=7), mirrors heraldicRadiusAt().
// Segment k IS prof[k] = (height, r_bot, r_top, shape) — one vec4, no stride math.
fn heraldic_profile_radius(fig: PawnFigure, t: f32) -> f32 {
    if (t > 1.0) { return 0.0; }
    let n = fig.seg_count;
    if (n == 0u) { return 0.0; }
    var total_h = 0.0;
    for (var k = 0u; k < n; k = k + 1u) { total_h = total_h + fig.prof[k].x; }
    if (total_h < 1e-6) { total_h = 1.0; }
    var acc = 0.0;
    for (var k = 0u; k < n; k = k + 1u) {
        let sg = fig.prof[k];
        let h  = sg.x;
        let rb = sg.y;
        let rt = sg.z;
        let sh = u32(sg.w);
        let seg_frac = h / total_h;
        if (t <= acc + seg_frac || k == n - 1u) {
            let u = select((t - acc) / seg_frac, 0.0, seg_frac < 1e-8);
            return max(0.0, profile_shape(sh, clamp(u, 0.0, 1.0), rb, rt));
        }
        acc = acc + seg_frac;
    }
    return 0.0;
}

// NORMALIZED radius dispatcher (caller multiplies by the figure's radius).
fn eval_profile_radius(fig: PawnFigure, is_regular: bool, t: f32) -> f32 {
    if (is_regular) { return pawn_profile_radius(t); }
    if (fig.family == FAM_HERALDIC_W) { return heraldic_profile_radius(fig, t); }
    return smooth_profile_radius(fig, t);
}

// Profile normal in (t, r) space — central finite difference on
// eval_profile_radius, normalized (aspect-correct normals are a later nicety).
// There is no regular-pawn twin to match: that branch folds in through
// eval_profile_radius's is_regular arm rather than standing as its own function.
fn eval_profile_normal_2d(fig: PawnFigure, is_regular: bool, t: f32) -> vec2<f32> {
    let eps = 0.005;
    let t0 = max(0.0, t - eps);
    let t1 = min(1.0, t + eps);
    let r0 = eval_profile_radius(fig, is_regular, t0);
    let r1 = eval_profile_radius(fig, is_regular, t1);
    let dr = r1 - r0;
    let dh = t1 - t0;
    let len = sqrt(dr * dr + dh * dh);
    if (len < 0.0001) { return vec2(1.0, 0.0); }
    return vec2(dh / len, -dr / len);
}

// --- Color: palette gradient at t + bounded seeded HSV drift --
// Uses the existing hash_property(seed, property) hash (distinct salts per channel).
fn rgb2hsv(c: vec3<f32>) -> vec3<f32> {
    let K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    let p = mix(vec4(c.b, c.g, K.w, K.z), vec4(c.g, c.b, K.x, K.y), step(c.b, c.g));
    let q = mix(vec4(p.x, p.y, p.w, c.r), vec4(c.r, p.y, p.z, p.x), step(p.x, c.r));
    let d = q.x - min(q.w, q.y);
    let e = 1e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}
fn hsv2rgb(c: vec3<f32>) -> vec3<f32> {
    let K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    let p = abs(fract(vec3(c.x, c.x, c.x) + K.xyz) * 6.0 - vec3(K.w, K.w, K.w));
    return c.z * mix(vec3(K.x, K.x, K.x), clamp(p - vec3(K.x, K.x, K.x), vec3(0.0), vec3(1.0)), c.y);
}

// Stop k IS pal[k] = (t, r, g, b).
fn eval_palette(fig: PawnFigure, t: f32) -> vec3<f32> {
    let s0 = fig.pal[0];
    var prev_t = s0.x;
    var prev_c = s0.yzw;
    if (t <= prev_t) { return prev_c; }
    for (var k = 1u; k < 8u; k = k + 1u) {
        let sk = fig.pal[k];
        if (sk.x < 0.0) { return prev_c; }
        if (t <= sk.x) {
            let u = (t - prev_t) / max(sk.x - prev_t, 1e-6);
            return mix(prev_c, sk.yzw, u);
        }
        prev_t = sk.x; prev_c = sk.yzw;
    }
    return prev_c;
}

fn figure_color(fig: PawnFigure, t: f32, seed: u32) -> vec3<f32> {
    let base = eval_palette(fig, t);
    let jh = hash_property(seed, 41u) * 2.0 - 1.0;
    let js = hash_property(seed, 42u) * 2.0 - 1.0;
    let jv = hash_property(seed, 43u) * 2.0 - 1.0;
    var hsv = rgb2hsv(base);
    hsv.x = fract(hsv.x + jh * (fig.drift.x / 360.0));
    hsv.y = clamp(hsv.y * (1.0 + js * fig.drift.y), 0.0, 1.0);
    hsv.z = clamp(hsv.z * (1.0 + jv * fig.drift.z), 0.0, 1.0);
    return hsv2rgb(hsv);
}


// --- Pawn Vertex Shader (chess pawn, GPU-generated)

@vertex
fn pawn_vs(@builtin(vertex_index) vid: u32,
           @builtin(instance_index) inst: u32) -> EntityVarying {
    let agent = render_agents[inst];

    // THE RING (draw authority): agents exist to 350 but DRAW only inside the
    // ring; the pawn is NOT exempt (ruled). Inactive/out-of-ring slots collapse
    // to a degenerate point at the agent's pos (local geometry * active_f = 0).
    let agent_in_ring = distance(vec2(agent.pos_x, agent.pos_z),
                                 vec2(config.lod_point_x, config.lod_point_z))
                        - 5.0 <= config.veil_ring;   // 5 wu: agent body half-extent
    let active_f = f32(agent.is_active) * f32(agent_in_ring);

    // -- Figure selection (0 = regular pawn: hardcoded profile + legacy color) --
    // Uniform arrays are fixed-size: use the count const, NOT arrayLength().
    let sid = select(agent.skin_id, 0u, agent.skin_id >= PAWN_FIGURE_COUNT_WGSL);
    let fig = scene_constants.figure_profiles[sid];
    let is_regular = sid == 0u;
    let scale_r = select(fig.radius, PAWN_RADIUS, is_regular);
    let scale_h = select(fig.height, PAWN_HEIGHT, is_regular);

    var local_pos: vec3<f32>;
    var local_normal: vec3<f32>;
    var vt: f32 = 0.0;   // this vertex's profile parameter (for the palette)

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

        let r_lo = eval_profile_radius(fig, is_regular, t_lo) * scale_r;
        let r_hi = eval_profile_radius(fig, is_regular, t_hi) * scale_r;

        let y_lo = t_lo * scale_h;
        let y_hi = t_hi * scale_h;

        let p00 = vec3(cos(angle0) * r_lo, y_lo, sin(angle0) * r_lo);
        let p10 = vec3(cos(angle1) * r_lo, y_lo, sin(angle1) * r_lo);
        let p01 = vec3(cos(angle0) * r_hi, y_hi, sin(angle0) * r_hi);
        let p11 = vec3(cos(angle1) * r_hi, y_hi, sin(angle1) * r_hi);

        let n2d_lo = eval_profile_normal_2d(fig, is_regular, t_lo);
        let n2d_hi = eval_profile_normal_2d(fig, is_regular, t_hi);

        let n00 = normalize(vec3(n2d_lo.x * cos(angle0), n2d_lo.y, n2d_lo.x * sin(angle0)));
        let n10 = normalize(vec3(n2d_lo.x * cos(angle1), n2d_lo.y, n2d_lo.x * sin(angle1)));
        let n01 = normalize(vec3(n2d_hi.x * cos(angle0), n2d_hi.y, n2d_hi.x * sin(angle0)));
        let n11 = normalize(vec3(n2d_hi.x * cos(angle1), n2d_hi.y, n2d_hi.x * sin(angle1)));

        switch tri_vert {
            case 0u: { local_pos = p00; local_normal = n00; vt = t_lo; }
            case 1u: { local_pos = p10; local_normal = n10; vt = t_lo; }
            case 2u: { local_pos = p01; local_normal = n01; vt = t_hi; }
            case 3u: { local_pos = p01; local_normal = n01; vt = t_hi; }
            case 4u: { local_pos = p10; local_normal = n10; vt = t_lo; }
            case 5u: { local_pos = p11; local_normal = n11; vt = t_hi; }
            default: { local_pos = p00; local_normal = n00; vt = t_lo; }
        }

    } else {
        // Bottom cap: fan triangles
        let cap_vid = vid - PAWN_BODY_VERTICES;
        let seg = cap_vid / 3u;
        let tri_vert = cap_vid % 3u;

        let seg_next = (seg + 1u) % PAWN_SEGMENTS;
        let angle0 = f32(seg) / f32(PAWN_SEGMENTS) * 2.0 * PI;
        let angle1 = f32(seg_next) / f32(PAWN_SEGMENTS) * 2.0 * PI;

        let r = eval_profile_radius(fig, is_regular, 0.0) * scale_r;

        local_normal = vec3(0.0, -1.0, 0.0);
        vt = 0.0;

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

    // -- Body color --
    // Regular (figure 0): legacy per-agent pick + tier fallback (UNCHANGED).
    //   per-tier color is the fallback when a slot carries no per-agent color.
    // Figures 1..13: per-vertex palette gradient + seeded HSV drift.
    let tier = min(agent.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let tg = scene_constants.tier_gains[tier];
    let agent_color = vec3(agent.color_r, agent.color_g, agent.color_b);
    let legacy_color = select(vec3(tg.color_r, tg.color_g, tg.color_b),
                              agent_color, any(agent_color > vec3(0.0)));
    var body_color: vec3<f32>;
    if (is_regular) {
        body_color = legacy_color;
    } else {
        body_color = figure_color(fig, vt, agent.seed);
    }

    var out: EntityVarying;
    out.clip_pos = frame_r.vp.m * vec4(rotated_pos + pawn_p, 1.0);
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
    out.clip_pos = frame_r.vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.normal = in.normal;
    out.entity_color = fe.color;
    return out;
}

@fragment
fn entity_fs(in: EntityVarying) -> @location(0) vec4<f32> {
    var albedo = in.entity_color;   // THE FALLBACK — plain bodies only
    let geo_n = normalize(in.normal);
    var n = geo_n;
    // THE MOSAIC (MOSAIC_2) — TWO POPULATIONS, no leak. A mosaic body IS
    // its mosaic at every range; entity_color is what a body wears when
    // it has NO mosaic and is never consulted here. Distance takes the
    // GRAIN, never the material — and grain is the veil's own band, so
    // a body materializes at the ring already ceramic and gains its
    // grain across exactly the band where it materializes.
    if (in.mosaic_seed != 0u && config.mosaic_enable > 0.5) {
        let grain = 1.0 - veil_t(in.world_pos);
        let paint_pos = vec3(in.world_pos.x, in.paint_y, in.world_pos.z);
        let s = mosaic_sample(paint_pos, in.mosaic_seed, grain);
        albedo = s.color;
        n = normalize(geo_n + s.facet * (config.mosaic_facet * grain));
    }
    return vec4(shade_lit(in.world_pos, n, geo_n, albedo, 1.0), 1.0);
}

// Ribbon FS — same shading as entity_fs but veil-EXEMPT (ruled fork): the
// ribbon is a flown sky structure, meant to be seen/ridden far beyond the
// band; veil_scale 0.0 keeps it whole while everything else condenses.
@fragment
fn ribbon_fs(in: EntityVarying) -> @location(0) vec4<f32> {
    return vec4(shade_lit(in.world_pos, normalize(in.normal), normalize(in.normal), in.entity_color, 0.0), 1.0);
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
    // Per-channel face hue. One hash pushed into three channels at fixed
    // ratios is monochromatic by construction — every face rides one color
    // axis. Three independent hashes give each channel its own axis.
    // Property block 500-517 is ONE purpose (entity-seed face hue), three
    // disjoint 6-wide runs, face_idx in 0..5:
    //   R 500-505   G 506-511   B 512-517
    // R keeps its original hash, so R is bit-identical to before and the
    // change is provably additive on G and B.
    let dr = (hash_property(fe.entity_seed, 500u + face_idx) - 0.5) * 2.0 * fe.face_variance;
    let dg = (hash_property(fe.entity_seed, 506u + face_idx) - 0.5) * 2.0 * fe.face_variance;
    let db = (hash_property(fe.entity_seed, 512u + face_idx) - 0.5) * 2.0 * fe.face_variance;
    let face_color = clamp(fe.color + vec3(dr, dg, db), vec3(0.0), vec3(1.0));

    var out: EntityVarying;
    out.clip_pos = frame_r.vp.m * vec4(world_pos, 1.0);
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

    // Figure lookup — shadow silhouette must track the figure or shadows detach.
    let sid = select(agent.skin_id, 0u, agent.skin_id >= PAWN_FIGURE_COUNT_WGSL);
    let fig = scene_constants.figure_profiles[sid];
    let is_regular = sid == 0u;
    let scale_r = select(fig.radius, PAWN_RADIUS, is_regular);
    let scale_h = select(fig.height, PAWN_HEIGHT, is_regular);

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

        let r_lo = eval_profile_radius(fig, is_regular, t_lo) * scale_r;
        let r_hi = eval_profile_radius(fig, is_regular, t_hi) * scale_r;
        let y_lo = t_lo * scale_h;
        let y_hi = t_hi * scale_h;

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
        let r = eval_profile_radius(fig, is_regular, 0.0) * scale_r;

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

    // The 0.3 world-unit lift is deleted (UMBRA_6). It was depth bias
    // wearing a geometry disguise: it displaced the caster rather than the
    // comparison, so it lifted the pawn's shadow off its own feet to buy
    // separation from terrain. Bias now has one home, in the pipeline.
    var out: ShadowVarying;
    out.clip_pos = shadow_light_vp() * vec4(world_pos, 1.0);
    return out;
}

// Shadow: Sphere (same as sphere_vs, light VP)
@vertex
fn shadow_sphere_vs(@builtin(instance_index) inst: u32, in: MeshVertexInput) -> ShadowVarying {
    let fe = render_floating.entities[inst];
    let r = select(0.0, fe.body_radius, fe.geometry_type == 0u && fe.is_active != 0u);
    let world_pos = in.pos * r + fe.pos;

    var out: ShadowVarying;
    out.clip_pos = shadow_light_vp() * vec4(world_pos, 1.0);
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
    out.clip_pos = shadow_light_vp() * vec4(world_pos, 1.0);
    return out;
}

// --- Catenary Arch
struct ArchVertexInput {
    @location(0) pos: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) color: vec3<f32>,
    // THE INDEX CHANNEL (MOSAIC_1): enc = mosaic_seed·64 + slot, as a
    // float (small-int exact; avoids GPU denorm flush on
    // bitcast<f32>(u32)). slot < 64 (census C-12); seed < 65536 →
    // enc < 2^22, f32-exact. Families that never paint write seed 0 —
    // their bytes are unchanged and their VSes keep the plain u32()
    // read (identity on a bare slot: palm/cactus/blade untouched).
    // Painted families (arch, column) and their shadow twins decode
    // via entity_index_decode below.
    @location(3) arch_index: f32,
};

fn entity_index_decode(v: f32) -> vec2<u32> {
    let enc = u32(v);
    return vec2<u32>(enc & 63u, enc >> 6u);   // (slot, mosaic_seed)
}

@vertex
fn arch_vs(in: ArchVertexInput) -> EntityVarying {
    let dec = entity_index_decode(in.arch_index);
    let ground_y = textureLoad(entity_ground_atlas, vec2<i32>(i32(dec.x) + GROUND_ATLAS_ARCH, 0), 0).r;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += sample_live_card(world_pos.xz).x;

    var out: EntityVarying;
    out.clip_pos = frame_r.vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.normal = in.normal;
    out.entity_color = in.color;
    out.paint_y = in.pos.y;
    out.mosaic_seed = dec.y;
    return out;
}

@vertex
fn shadow_arch_vs(in: ArchVertexInput) -> ShadowVarying {
    let dec = entity_index_decode(in.arch_index);
    let ground_y = textureLoad(entity_ground_atlas, vec2<i32>(i32(dec.x) + GROUND_ATLAS_ARCH, 0), 0).r;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += sample_live_card(world_pos.xz).x;

    var out: ShadowVarying;
    out.clip_pos = shadow_light_vp() * vec4(world_pos, 1.0);
    return out;
}

// --- Generative Columns
@vertex
fn column_vs(in: ArchVertexInput) -> EntityVarying {
    let dec = entity_index_decode(in.arch_index);
    let ground_y = textureLoad(entity_ground_atlas, vec2<i32>(i32(dec.x) + GROUND_ATLAS_COLUMN, 0), 0).r;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += sample_live_card(world_pos.xz).x;

    var out: EntityVarying;
    out.clip_pos = frame_r.vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.normal = in.normal;
    out.entity_color = in.color;
    out.paint_y = in.pos.y;
    out.mosaic_seed = dec.y;
    return out;
}

@vertex
fn shadow_column_vs(in: ArchVertexInput) -> ShadowVarying {
    let dec = entity_index_decode(in.arch_index);
    let ground_y = textureLoad(entity_ground_atlas, vec2<i32>(i32(dec.x) + GROUND_ATLAS_COLUMN, 0), 0).r;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += sample_live_card(world_pos.xz).x;

    var out: ShadowVarying;
    out.clip_pos = shadow_light_vp() * vec4(world_pos, 1.0);
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
    out.clip_pos = frame_r.vp.m * vec4(in.pos, 1.0);
    out.world_pos = in.pos;
    out.normal = in.normal;
    out.entity_color = in.color;
    return out;
}

@vertex
fn shadow_shell_vs(in: ShellVertexInput) -> ShadowVarying {
    var out: ShadowVarying;
    out.clip_pos = shadow_light_vp() * vec4(in.pos, 1.0);
    return out;
}

// §6.5 THE RIBBON ENTITY (RIBBON_1; RIBBON_2 — the wall, the sweep, the body as a thing)
// ONE ENTITY, ONE ROOM: head, spine, body and frame live here. The CPU
// authors INTENT — spawn, tier, colors, wave amplitudes, the wanderer's
// cruise, the rider's hands through the signal — and reads nothing back.
//
// TWO CLOCKS.
//   The SPINE is a SPACE law: a ring of samples the head lays down once
//   per chord (cube_size) of flight, C1 between samples (a Hermite arc on
//   the samples' own tangents). The body is drawn where the head has been.
//   THE SWEEP returns it to rest: each chord turns toward the one before
//   it at up to propagation_speed when the hands are idle, so a turn
//   travels down the body at P and the body settles straight behind the
//   head — the old time law's settling, kept, without the whip.
//   The GESTURE is a TIME law: the transverse wave, analytic at phase_age
//   (ribbon_displacement_at, unchanged).
//
// THE SKY RULE — self-preservation, one law, two readers, three voices.
//   Standing things are cylinders (a disc and a top); moving things are
//   spheres; the ribbon's own body is a chain of capsules minus the neck.
//   THE SHELL is advice: a quadratic push, read ahead by the head (lateral
//   -> yaw through the hands' own cap; vertical -> the altitude target) and
//   at every ring by the body (a string under tension). THE WALL is law:
//   after the string has moved, no ring center stands inside a thing.
//   Pyramids are ground: both readers climb them through the floor.
//
// Hot-reloadable; tune by save. Units: wu, s, rad.
const RIBBON_SPINE_SLOTS: u32 = 402u;        // Dim::RIBBON_SPINE_SLOTS
const RIBBON_EMIT_STRIDE: u32 = 4u;          // Dim::RIBBON_EMIT_STRIDE
const RIBBON_EMIT_SLOTS: u32 = 100u;         // Dim::RIBBON_EMIT_SLOTS — one half; the table is two
const RIBBON_CLEAR_Y: f32 = 20.0;            // vertical clearance above a standing thing's top
const RIBBON_PUSH_OUT: f32 = 1.0;            // weight of the out-of-disc component
const RIBBON_PUSH_UP: f32 = 0.6;             // weight of the over-the-top component
const RIBBON_STEER_GAIN: f32 = 3.0;          // head: lateral push -> yaw command (saturates at |push.lateral| = 1/gain)
const RIBBON_YAW_SIGN: f32 = 1.0;            // the rider's A/D sign; flip if the hands feel mirrored
const RIBBON_DT_MAX: f32 = 0.0333333;        // 1/30 s — the integrators never see a hitch longer than this
const RIBBON_BANK_GAIN: f32 = 0.9;           // BNK-1: roll into the lateral swing
const RIBBON_BANK_MAX: f32 = 0.6;            // rad, clamp
const RIBBON_BODY_K: f32 = 6.0;              // 1/s^2 — the return spring
const RIBBON_BODY_ZETA: f32 = 1.0;           // damping ratio (1 = critical; 0.7 for a hint of overshoot)
const RIBBON_BODY_TENSION: f32 = 60.0;       // 1/s^2 — neighbor coupling; a bulge reaches ~sqrt(TENSION/K) rings past the thing. Keep < 600 (explicit-step stability at DT_MAX)
const RIBBON_BODY_PUSH: f32 = 600.0;         // wu/s^2 at full shell depth (RIBBON_3: the wall carries the guarantee; the shell can be gentle)
const RIBBON_BODY_DMAX: f32 = 60.0;          // wu — the deformation's leash
const RIBBON_BODY_FLOOR: f32 = 12.0;         // wu above local ground the body refuses to go under
const RIBBON_BODY_BANK: f32 = 0.0;           // rad per (wu/s) of lateral deform velocity — roll into the dodge (0 = off)
// RIBBON_2
const RIBBON_SPINE_RELAX_IDLE: f32 = 1.0;    // × propagation_speed — the sweep when the hands are idle
const RIBBON_SPINE_RELAX_FLY: f32 = 0.15;    // × propagation_speed — at full throttle (0 = the pure track)
const RIBBON_WALL_HALF: f32 = 0.75;          // × cube_size — the tube's half-diagonal; the wall's thickness
const RIBBON_SELF_NECK: u32 = 24u;           // rings around a reader that are its own tube, not a thing
const RIBBON_CLEAR_SELF: f32 = 30.0;         // the head's shell against its body
const RIBBON_CLEAR_SELF_BODY: f32 = 10.0;    // a ring's shell against the rest of the body
const RIBBON_LIFT_GAIN: f32 = 60.0;          // wu of altitude bias per unit of vertical push — the body's word and the world's (arches, movers)
const RIBBON_SELF_BODY_W: f32 = 0.5;         // body-on-body push, as a fraction of RIBBON_BODY_PUSH
const RIBBON_CHASE_BOARD_TAU: f32 = 0.35;    // s — the camera turns to the flight over the boarding
const RIBBON_CHASE_ELEVATION: f32 = 0.6;     // rad — the chase pose, ~35° to the ribbon's surface (Jean)
const RIBBON_CHASE_TAU: f32 = 0.0;           // s — set once at boarding; 0 = the mouse owns the camera from then on (Jean). > 0 re-centers on an idle mouse
const RIBBON_CHASE_AZ_OFFSET: f32 = 0.0;     // rad — 3.14159 if the camera lands in front (screen gate)
// RIBBON_3
const RIBBON_RULE_TAU: f32 = 0.35;           // s — the rule's lateral word, low-passed before it steers
const RIBBON_YAW_SLEW: f32 = 1.5;            // command units per second — the heading's RATE never jumps faster than this
const RIBBON_CLEAR_MOVER: f32 = 20.0;        // the head's shell against the big movers (spheres, walkers); cubes are the body's
const RIBBON_ARCH_SEGS: u32 = 8u;            // capsules along an arch's rib

struct RibbonHeadState {          // 64 B — mirrors GPURibbonHeadState
    pos: vec3<f32>,               //  0  the pen
    heading: f32,                 // 12  unbounded; tailward = +dir(heading), flight = -dir(heading)
    y_vel: f32,                   // 16
    alt_target: f32,              // 20
    head_slot: u32,               // 24  spine slot of the newest chord sample
    seeded: u32,                  // 28
    tick: u32,                    // 32  frames since seed; parity selects the deform half and the emit half
    yaw_eased: f32,               // 36
    throttle_eased: f32,          // 40
    yaw_cmd: f32,                 // 44  the one command (hands + rule), slew-limited
    wander_tx: f32,               // 48  the wander brain's target
    wander_tz: f32,               // 52
    wander_seq: u32,              // 56  targets drawn so far (0 = none yet)
    rule_eased: f32,              // 60  the rule's lateral word, low-passed
}
struct RibbonSaddle {             // 32 B — mirrors GPURibbonSaddle
    pos: vec3<f32>,               //  0
    heading: f32,                 // 12
    yaw_off: f32,                 // 16
    pitch: f32,                   // 20
    roll: f32,                    // 24
    _pad0: f32,                   // 28
}
struct RibbonBody {               // 28896 B — mirrors GPURibbonBody
    head: RibbonHeadState,        //     0  written by ribbon_head
    saddle: RibbonSaddle,         //    64  written by ribbon_body (ring 0)
    emit: array<vec4<f32>, 200>,  //    96  [half][slot]: every EMIT_STRIDE-th ring, xyz center, w radius.
                                  //        The body writes half (tick & 1); the head and every ring read
                                  //        the other; the field, after the body, reads the written one.
    deform: array<vec4<f32>, 1600>, // 3296  [half][ring][offset|velocity], ping-pong by tick parity
}

fn wrap_pi(a: f32) -> f32 { return a - 6.2831853 * round(a / 6.2831853); }
fn mix_heading(a: f32, b: f32, t: f32) -> f32 { return a + wrap_pi(b - a) * t; }
fn mount_ease(t: f32) -> f32 { let x = saturate(t); return x * x * (3.0 - 2.0 * x); }

// THE RIBBON READS THE DRAWN GROUND (SPINE_2 R1). It used to evaluate the
// baked heightfield's contributor set ANALYTICALLY — static base + tile
// modifiers + pyramids, every term, every call — for one reason the old
// banner stated plainly: the ribbon room had no textures. It has them now.
//
// The bake already holds that ground at the lattice, and every other ground
// reader in the program samples it there. Three serial evaluations at the
// head and one per ring in the body become fetches, and the ribbon's
// clearance, lift and tips read THE SURFACE THAT IS DRAWN rather than an
// analytic ghost of it — which is the correctness half of this change, not
// only the cost half.
//
// THE FALLBACK IS THE WINDOW'S EDGE and it is not optional: outside the
// active patch grid there is no baked layer, and a ribbon flying past the
// edge must not step. The two forms differ by at most the fine band's
// amplitude (0.12 wu) — the lattice carries every band the analytic form
// does, and the ONE thing it does not carry is band 4's ripple below the
// lattice's own spacing — against a body flying RIBBON_CLEAR_Y above the
// ground. Sub-visible, and stated rather than assumed.
fn ribbon_ground(xz: vec2<f32>) -> f32 {
    let r = sample_terrain_y_found(xz);
    if (r.y == 0.0) { return ground_formed_with_complexity(xz).x; }
    return r.x;
}

// One standing thing: a disc of radius r and a top at top_agl above local
// ground. Inside r + clear and under top + CLEAR_Y it pushes OUT (quadratic
// shell) and UP (fading as the reader clears the top). Zero elsewhere.
fn sky_shell(p: vec3<f32>, agl: f32, c_xz: vec2<f32>, r: f32, top_agl: f32, clear: f32) -> vec3<f32> {
    let shell = r + clear;
    let d = p.xz - c_xz;
    let len = length(d);
    if (len >= shell) { return vec3(0.0); }
    let u = saturate((top_agl + RIBBON_CLEAR_Y - agl) / RIBBON_CLEAR_Y);
    if (u <= 0.0) { return vec3(0.0); }
    var out_dir = vec2(1.0, 0.0);
    if (len > 1e-3) { out_dir = d / len; }
    let s = 1.0 - len / shell;
    let mag = s * s * u;
    return vec3(out_dir.x * mag * RIBBON_PUSH_OUT, mag * RIBBON_PUSH_UP, out_dir.y * mag * RIBBON_PUSH_OUT);
}

// One moving thing: a sphere of radius r. Radial quadratic shell.
fn sky_sphere(p: vec3<f32>, c: vec3<f32>, r: f32, clear: f32) -> vec3<f32> {
    let shell = r + clear;
    let d = p - c;
    let len = length(d);
    if (len >= shell) { return vec3(0.0); }
    var dir = vec3(0.0, 1.0, 0.0);
    if (len > 1e-3) { dir = d / len; }
    let s = 1.0 - len / shell;
    return dir * (s * s);
}

// Closest point on the segment ab to p.
fn seg_closest(p: vec3<f32>, a: vec3<f32>, b: vec3<f32>) -> vec3<f32> {
    let ab = b - a;
    let t = saturate(dot(p - a, ab) / max(dot(ab, ab), 1e-6));
    return a + ab * t;
}

// THE DOORWAY (RIBBON_3): an arch is not a disc. Two piers and a rib — the
// rib a parabola from pier top to apex to pier top (the catenary within a
// few wu; the shell is wider than the error), walked as RIBBON_ARCH_SEGS
// capsules; the piers two vertical capsules. Legs at center ± half_span
// along (cos rotation, sin rotation), radius half the larger cross-section
// — occupier_contact's own law. A reader passes under, over or around.
fn arch_rib_point(am: ArchMeshParams, g: f32, u: f32) -> vec3<f32> {
    let along = vec2(cos(am.rotation), sin(am.rotation)) * (u * am.half_span);
    return vec3(am.center_x + along.x,
                g + am.pier_height - am.burial + am.rise * (1.0 - u * u),
                am.center_z + along.y);
}

fn sky_arch_push(p: vec3<f32>, g: f32, am: ArchMeshParams, clear: f32) -> vec3<f32> {
    var f = vec3(0.0);
    let r = max(am.thickness, am.depth) * 0.5;
    var prev = arch_rib_point(am, g, -1.0);
    for (var s = 1u; s <= RIBBON_ARCH_SEGS; s++) {
        let cur = arch_rib_point(am, g, -1.0 + 2.0 * f32(s) / f32(RIBBON_ARCH_SEGS));
        f += sky_sphere(p, seg_closest(p, prev, cur), r, clear);
        prev = cur;
    }
    let pa = arch_rib_point(am, g, -1.0);
    let pb = arch_rib_point(am, g, 1.0);
    f += sky_sphere(p, seg_closest(p, vec3(pa.x, g, pa.z), pa), r, clear);
    f += sky_sphere(p, seg_closest(p, vec3(pb.x, g, pb.z), pb), r, clear);
    return f;
}

// The wall's arch: the same capsules, projected out of, sequentially.
fn sky_arch_wall(q_in: vec3<f32>, g: f32, am: ArchMeshParams, half: f32) -> vec3<f32> {
    var q = q_in;
    let r = max(am.thickness, am.depth) * 0.5 + half;
    var prev = arch_rib_point(am, g, -1.0);
    for (var s = 1u; s <= RIBBON_ARCH_SEGS + 2u; s++) {
        var cur = prev;
        var a = prev;
        if (s <= RIBBON_ARCH_SEGS) {
            cur = arch_rib_point(am, g, -1.0 + 2.0 * f32(s) / f32(RIBBON_ARCH_SEGS));
        } else if (s == RIBBON_ARCH_SEGS + 1u) {
            a = arch_rib_point(am, g, -1.0); cur = vec3(a.x, g, a.z);
        } else {
            a = arch_rib_point(am, g, 1.0);  cur = vec3(a.x, g, a.z);
        }
        let c = seg_closest(q, a, cur);
        let d = q - c;
        let len = length(d);
        if (len < r) {
            var od = vec3(0.0, 1.0, 0.0);
            if (len > 1e-3) { od = d / len; }
            q += od * (r - len);
        }
        prev = cur;
    }
    return q;
}

// The rule, summed at p. agl = p.y − ribbon_ground(p.xz), computed once by
// the caller. clear is the shell against standing things, clear_mover
// against movers; movers is how many floating slots the reader hears —
// SPHERE_SLOT_COUNT for the head (the big movers), all of them for the body.
// skip_agent: the rider's own body (32u = skip nobody).
fn sky_push(p: vec3<f32>, agl: f32, clear: f32, clear_mover: f32, movers: u32, skip_agent: u32) -> vec3<f32> {
    var f = vec3(0.0);
    let g = p.y - agl;
    // Shafts — columns 0–15, antennas 16–31: a disc and a top. The disc is
    // the widest thing on the post (an antenna's drums live in base_overhang).
    for (var i = 0u; i < 32u; i++) {
        let cm = agent_room.occupier_cmg[i];
        if (cm.is_active == 0u) { continue; }
        f += sky_shell(p, agl, vec2(cm.center_x, cm.center_z),
                       cm.shaft_radius + max(cm.base_overhang, cm.capital_overhang),
                       cm.height - cm.burial, clear);
    }
    // Arches — doorways.
    for (var i = 0u; i < 16u; i++) {
        let am = agent_room.occupier_amg[i];
        if (am.is_active == 0u) { continue; }
        f += sky_arch_push(p, g, am, clear);
    }
    // Walkers — spheres of their tier's contact radius.
    for (var i = 0u; i < 32u; i++) {
        if (i == skip_agent) { continue; }
        let a = render_agents[i];
        if (a.is_active == 0u) { continue; }
        f += sky_sphere(p, vec3(a.pos_x, a.pos_y, a.pos_z),
                        agent_room.tier_gains[min(a.tier_idx, 3u)].contact_radius, clear_mover);
    }
    // Floaters — spheres of their body radius; the head hears the first
    // SPHERE_SLOT_COUNT, the body all of them.
    for (var i = 0u; i < movers; i++) {
        let fe = render_floating.entities[i];
        if (fe.is_active == 0u) { continue; }
        f += sky_sphere(p, fe.pos, fe.body_radius, clear_mover);
    }
    return f;
}

// The roofline under a disc at xz: the highest (top + CLEAR_Y), above local
// ground, of the SHAFTS whose shell covers xz (arches are doorways: their
// word is the push's vertical, not a roof). 0 when the sky is open.
fn sky_roof(xz: vec2<f32>, clear: f32) -> f32 {
    var roof = 0.0;
    for (var i = 0u; i < 32u; i++) {
        let cm = agent_room.occupier_cmg[i];
        if (cm.is_active == 0u) { continue; }
        let r = cm.shaft_radius + max(cm.base_overhang, cm.capital_overhang) + clear;
        if (distance(xz, vec2(cm.center_x, cm.center_z)) < r) {
            roof = max(roof, cm.height - cm.burial + RIBBON_CLEAR_Y);
        }
    }
    return roof;
}

// The spine is C1 between samples: a cubic Hermite on the samples' own
// tailward tangents (.w), scaled to the chord; Y takes the chord's slope so
// a steady climb stays a line.
fn ribbon_spine_arc(a: vec4<f32>, b: vec4<f32>, t: f32) -> vec3<f32> {
    let len = length(b.xyz - a.xyz);
    let dy = b.y - a.y;
    let ta = vec3(cos(a.w) * len, dy, sin(a.w) * len);
    let tb = vec3(cos(b.w) * len, dy, sin(b.w) * len);
    let t2 = t * t;
    let t3 = t2 * t;
    return a.xyz * (2.0 * t3 - 3.0 * t2 + 1.0) + ta * (t3 - 2.0 * t2 + t)
         + b.xyz * (3.0 * t2 - 2.0 * t3)       + tb * (t3 - t2);
}

// THE BODY AS A THING (RIBBON_2): the ribbon's own body enters the rule as
// capsules between emit samples (the half written last frame), minus the
// neck — the NECK rings around the reader, which are its own tube.
fn sky_self(p: vec3<f32>, k_reader: u32, n: u32, cs: f32, emit_half: u32, clear: f32) -> vec3<f32> {
    var f = vec3(0.0);
    let seg_n = (n - 1u) / RIBBON_EMIT_STRIDE;
    for (var i = 0u; i < seg_n; i++) {
        let ring_a = i * RIBBON_EMIT_STRIDE;
        if (ring_a + RIBBON_EMIT_STRIDE + RIBBON_SELF_NECK > k_reader && k_reader + RIBBON_SELF_NECK > ring_a) { continue; }
        let a = ribbon_body_rw.emit[emit_half + i].xyz;
        let ab = ribbon_body_rw.emit[emit_half + i + 1u].xyz - a;
        let t = saturate(dot(p - a, ab) / max(dot(ab, ab), 1e-6));
        f += sky_sphere(p, a + ab * t, cs * 0.5, clear);
    }
    return f;
}

// THE WALL — the shell was advice; this is law. After the string has moved:
// no ring center inside a shaft's disc (widened by half) while under its
// top — the cheaper exit wins, out or up; none inside an arch's capsules;
// none inside a mover's sphere; none under ground + half. Returns the
// displacement that restores the law. Applied after the leash: law
// outranks leash.
fn sky_wall(p: vec3<f32>, g: f32, half: f32, skip_agent: u32) -> vec3<f32> {
    var q = p;
    for (var i = 0u; i < 32u; i++) {
        let cm = agent_room.occupier_cmg[i];
        if (cm.is_active == 0u) { continue; }
        let r = cm.shaft_radius + max(cm.base_overhang, cm.capital_overhang) + half;
        let d = q.xz - vec2(cm.center_x, cm.center_z);
        let len = length(d);
        let top = g + cm.height - cm.burial + half;
        if (len < r && q.y < top) {
            let out = r - len;
            let up = top - q.y;
            if (out <= up) {
                var od = vec2(1.0, 0.0);
                if (len > 1e-3) { od = d / len; }
                q.x += od.x * out;
                q.z += od.y * out;
            } else {
                q.y = top;
            }
        }
    }
    for (var i = 0u; i < 16u; i++) {
        let am = agent_room.occupier_amg[i];
        if (am.is_active == 0u) { continue; }
        q = sky_arch_wall(q, g, am, half);
    }
    for (var i = 0u; i < 32u; i++) {
        if (i == skip_agent) { continue; }
        let a = render_agents[i];
        if (a.is_active == 0u) { continue; }
        let r = agent_room.tier_gains[min(a.tier_idx, 3u)].contact_radius + half;
        let d = q - vec3(a.pos_x, a.pos_y, a.pos_z);
        let len = length(d);
        if (len < r) {
            var od = vec3(0.0, 1.0, 0.0);
            if (len > 1e-3) { od = d / len; }
            q += od * (r - len);
        }
    }
    for (var i = 0u; i < 264u; i++) {
        let fe = render_floating.entities[i];
        if (fe.is_active == 0u) { continue; }
        let r = fe.body_radius + half;
        let d = q - fe.pos;
        let len = length(d);
        if (len < r) {
            var od = vec3(0.0, 1.0, 0.0);
            if (len > 1e-3) { od = d / len; }
            q += od * (r - len);
        }
    }
    q.y = max(q.y, g + half);
    return q - p;
}

// Ring k's place on the spine: k chords behind the head (ring 0 = the head).
// slot (head_slot - j) mod S holds the sample j chords tailward; the head
// sits a fraction f of a chord past the newest sample.
fn ribbon_rest(k: u32, hd: RibbonHeadState, cs: f32) -> vec3<f32> {
    if (k == 0u) { return hd.pos; }
    let last = ribbon_spine[hd.head_slot].xyz;
    let f = length(hd.pos - last) / cs;
    let u = max(f32(k) - f, 0.0);
    let j = u32(floor(u));
    let t = u - f32(j);
    let a = ribbon_spine[(hd.head_slot + RIBBON_SPINE_SLOTS - j) % RIBBON_SPINE_SLOTS];
    let b = ribbon_spine[(hd.head_slot + RIBBON_SPINE_SLOTS - j - 1u) % RIBBON_SPINE_SLOTS];
    return ribbon_spine_arc(a, b, t);
}

// Ring k's drawn rest: the spine's place plus the gesture in the ring's own
// frame — lateral on (-sin w, 0, cos w) of the spine's tailward tangent,
// vertical on world-up. One convention, as before.
fn ribbon_drawn_rest(k: u32, n: u32, hd: RibbonHeadState, ribbon: RibbonState) -> vec3<f32> {
    let cs = max(ribbon.cube_size, 1e-3);
    let km = select(k - 1u, k, k == 0u);
    let kp = min(k + 1u, n - 1u);
    let rest = ribbon_rest(k, hd, cs);
    let tan_xz = ribbon_rest(kp, hd, cs).xz - ribbon_rest(km, hd, cs).xz;   // tailward
    var txz = vec2(cos(hd.heading), sin(hd.heading));
    let tl = length(tan_xz);
    if (tl > 1e-4) { txz = tan_xz / tl; }
    let lateral = vec3(-txz.y, 0.0, txz.x);
    let t = f32(k) / f32(n - 1u);
    let d = ribbon_displacement_at(ribbon_phase_age(t, ribbon), ribbon);
    return rest + lateral * d.x + vec3(0.0, d.y, 0.0);
}

// The head's transverse displacement (the choreography) at echo time
// `phase_age`. Two components -- lateral (sway) and vertical (bob). This is
// the single displacement the body echoes along the ruler. The echo is
// ANALYTIC: the body re-evaluates the head's timetable at a delayed time —
// honest only while the script is a pure function of time.
// [SEAM:ribbon-displacement] To let music drive displacement at the head,
// record the head's lateral/vertical into a GESTURE ring beside the spine
// (time-cadenced, like the spine is chord-cadenced) and read the delayed
// samples here. Coupling amp/freq parameters directly would move
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

// ── THE HEAD ─────────────────────────────────────────────────────────────
// One thread. Seed, intent (the hands or the brain), the Sky Rule at the
// probe (the world, then the body), flight, the pen's altitude, the spine's
// emission, the sweep. The flight law is the CPU's, kept: speed = throttle ×
// max_speed; yaw available = min(yaw_rate, speed / r_min); flight along
// −dir(heading).
@compute @workgroup_size(1)
fn ribbon_head(@builtin(global_invocation_id) gid: vec3<u32>) {
    if (gid.x != 0u) { return; }
    let ribbon = ribbon_state;
    if (ribbon.is_visible == 0u || ribbon.cube_count < 2u) { return; }
    var hd = ribbon_body_rw.head;
    let n = min(ribbon.cube_count, 400u);
    let cs = max(ribbon.cube_size, 1e-3);
    let dt = min(signal.dt, RIBBON_DT_MAX);
    let birth_y = ribbon_ground(ribbon.anchor.xz) + ribbon.height;

    // Seed: the straight spawn arc, laid once, here. Tailward = +dir(heading).
    if (hd.seeded == 0u) {
        hd.pos = vec3(ribbon.anchor.x, birth_y, ribbon.anchor.z);
        hd.heading = ribbon.orientation;
        hd.y_vel = 0.0;
        hd.alt_target = birth_y;
        hd.head_slot = 0u;
        hd.tick = 0u;
        hd.yaw_eased = 0.0;
        hd.throttle_eased = 0.0;
        hd.wander_tx = ribbon.anchor.x;
        hd.wander_tz = ribbon.anchor.z;
        hd.wander_seq = 0u;
        hd.yaw_cmd = 0.0;
        hd.rule_eased = 0.0;
        let tail = vec3(cos(hd.heading), 0.0, sin(hd.heading));
        for (var j = 0u; j < RIBBON_SPINE_SLOTS; j++) {
            let slot = (RIBBON_SPINE_SLOTS - j) % RIBBON_SPINE_SLOTS;
            ribbon_spine[slot] = vec4(hd.pos + tail * (f32(j) * cs), hd.heading);
        }
        hd.seeded = 1u;
    }
    hd.tick += 1u;
    let emit_prev = ((hd.tick + 1u) & 1u) * RIBBON_EMIT_SLOTS;   // the half the body wrote last frame
    let has_body = hd.tick > 1u;                                  // last frame's half exists

    // Intent: the rider's hands, the wander brain, or nothing (parked).
    var yaw_in = 0.0;
    var throttle = 0.0;
    if (point_ribbon_hosted()) {
        yaw_in = clamp(signal.move_x, -1.0, 1.0) * RIBBON_YAW_SIGN;
        throttle = saturate(-signal.move_z);               // W is forward-negative (input.hpp:117)
    } else if (ribbon.is_wander == 1u) {
        // THE WANDER BRAIN (RIBBON_2) — lives with the head it steers. A
        // target on the anchor's disc, drawn by hash; aim at it through the
        // hands' own cap; cruise; a new draw on arrival. No dead reckoning.
        if (hd.wander_seq == 0u
            || distance(vec2(hd.wander_tx, hd.wander_tz), hd.pos.xz) < config.ribbon_wander_arrive) {
            hd.wander_seq += 1u;
            let u1 = hash_property(ribbon.seed, 2000u + 2u * hd.wander_seq);
            let u2 = hash_property(ribbon.seed, 2001u + 2u * hd.wander_seq);
            let r = config.ribbon_roam_radius * sqrt(u1);
            let a = 6.2831853 * u2;
            hd.wander_tx = ribbon.anchor.x + r * cos(a);
            hd.wander_tz = ribbon.anchor.z + r * sin(a);
        }
        let to = vec2(hd.wander_tx, hd.wander_tz) - hd.pos.xz;
        let want = atan2(-to.y, -to.x);                    // flight is −dir(heading): point dir(heading) away from the target
        let err = wrap_pi(want - hd.heading);
        yaw_in = clamp(err / max(config.ribbon_wander_soft, 1e-3), -1.0, 1.0) * config.ribbon_wander_yaw_max;
        throttle = saturate(ribbon.wander_throttle);
    }
    let ease = 1.0 - exp(-dt / max(config.ribbon_hands_tau, 1e-3));
    hd.yaw_eased += (yaw_in - hd.yaw_eased) * ease;
    hd.throttle_eased += (throttle - hd.throttle_eased) * ease;

    // The Sky Rule at the probe: the world at the head's own scale (standing
    // things; the spheres and the walkers — the cubes are the body's
    // business), then the body. lateral = (−sin h, 0, cos h);
    // d(flight)/d(heading) = −lateral, so a push toward +lateral asks for
    // LESS heading. Derived; the screen is the gate.
    let dir_h = vec3(cos(hd.heading), 0.0, sin(hd.heading));
    let lateral = vec3(-dir_h.z, 0.0, dir_h.x);
    let probe = hd.pos - dir_h * config.ribbon_lookahead;
    let ground_here = ribbon_ground(hd.pos.xz);
    let ground_probe = ribbon_ground(probe.xz);
    let skip = select(32u, config.possessed_slot, point_ribbon_hosted());
    let push_world = sky_push(probe, probe.y - ground_probe, config.ribbon_clear_head,
                              RIBBON_CLEAR_MOVER, SPHERE_SLOT_COUNT, skip);
    var push_self = vec3(0.0);
    if (has_body) { push_self = sky_self(probe, 0u, n, cs, emit_prev, RIBBON_CLEAR_SELF); }

    // ONE COMMAND, C2 (RIBBON_3): the rule's word is low-passed, the total —
    // hands and rule — is slew-limited, so the heading's RATE never jumps.
    // A dodge begins and ends as a curve; the rider feels a turn, not a wall.
    let rule_lat = dot(push_world + push_self, lateral);
    hd.rule_eased += (rule_lat - hd.rule_eased) * (1.0 - exp(-dt / RIBBON_RULE_TAU));
    let want = clamp(hd.yaw_eased - RIBBON_STEER_GAIN * hd.rule_eased, -1.0, 1.0);
    hd.yaw_cmd += clamp(want - hd.yaw_cmd, -RIBBON_YAW_SLEW * dt, RIBBON_YAW_SLEW * dt);

    // Over or under: the world's vertical word (a doorway's rib, a mover)
    // and the body's (over unless clearly above), both through the pen.
    var lift_self = push_self.y;
    if (push_self.y >= -0.2) { lift_self = max(push_self.y, length(push_self.xz)); }
    let lift = lift_self + push_world.y;

    // Flight.
    let speed = hd.throttle_eased * config.ribbon_max_speed;
    let yaw_avail = min(config.ribbon_yaw_rate, speed / max(config.ribbon_r_min, 1e-3));
    hd.heading += hd.yaw_cmd * yaw_avail * dt;
    let step = speed * dt;
    hd.pos.x -= cos(hd.heading) * step;
    hd.pos.z -= sin(hd.heading) * step;

    // The pen owns altitude (B0, kept): the birth altitude, biased by the
    // vertical word above — the body's and the world's; never under the
    // floor here or ahead; never under a SHAFT's roofline here or ahead (an
    // arch has no roof: it is a doorway, and its word is in the bias);
    // low-passed by travel; critically damped; climb-capped. The floor is
    // a guarantee.
    let floor_y = max(ground_here, ground_probe) + config.ribbon_floor_margin;
    let roof_y = max(ground_probe + sky_roof(probe.xz, config.ribbon_clear_head),
                     ground_here + sky_roof(hd.pos.xz, config.ribbon_clear_head));
    let raw_target = max(max(birth_y + lift * RIBBON_LIFT_GAIN, floor_y), roof_y);
    let alpha = 1.0 - exp(-step / max(config.ribbon_alt_smooth_dist, 1e-3));
    hd.alt_target += (raw_target - hd.alt_target) * alpha;
    hd.alt_target = max(hd.alt_target, floor_y);
    let damp = 2.0 * sqrt(max(config.ribbon_alt_stiff, 1e-6));
    hd.y_vel += ((hd.alt_target - hd.pos.y) * config.ribbon_alt_stiff - damp * hd.y_vel) * dt;
    hd.y_vel = clamp(hd.y_vel, -config.ribbon_climb_rate, config.ribbon_climb_rate);
    hd.pos.y += hd.y_vel * dt;

    // The spine: one sample per chord, exactly cs apart, along the flight.
    // A sample's .w is its tailward tangent: the newest carries its one
    // chord; the one before it learns the mean of its two.
    var last = ribbon_spine[hd.head_slot].xyz;
    var emitted = 0u;
    loop {
        let dv = hd.pos - last;
        let dl = length(dv);
        if (dl < cs || emitted >= 8u) { break; }
        let tail_w = atan2(-dv.z, -dv.x);
        let prev_slot = hd.head_slot;
        let prev_w = ribbon_spine[prev_slot].w;
        ribbon_spine[prev_slot].w = prev_w + 0.5 * wrap_pi(tail_w - prev_w);
        last = last + dv * (cs / dl);
        hd.head_slot = (hd.head_slot + 1u) % RIBBON_SPINE_SLOTS;
        ribbon_spine[hd.head_slot] = vec4(last, tail_w);
        emitted += 1u;
    }

    // THE SWEEP (RIBBON_2): each chord turns toward the one before it — the
    // head's own heading leads — lengths kept, at relax × P per chord. A
    // turn travels tailward at that speed and the body settles straight
    // behind the head. Only the n + 1 samples the rings read are swept; the
    // tangents (.w) follow, each the mean of its two chords.
    let relax = mix(RIBBON_SPINE_RELAX_IDLE, RIBBON_SPINE_RELAX_FLY, hd.throttle_eased)
              * max(ribbon.propagation_speed, 0.0);
    if (relax > 0.0) {
        let w = min(relax * dt / cs, 1.0);
        var prev_p = hd.pos;      // where the previous sample now IS (swept)
        var prev_old = hd.pos;    // where it WAS — the chord is measured here
        var prev_dir = vec3(cos(hd.heading), 0.0, sin(hd.heading));
        var prev_slot = RIBBON_SPINE_SLOTS;                           // none yet
        for (var j = 0u; j <= n; j++) {
            let slot = (hd.head_slot + RIBBON_SPINE_SLOTS - j) % RIBBON_SPINE_SLOTS;
            let old_p = ribbon_spine[slot].xyz;
            // LENGTHS KEPT means kept: the chord is the one that WAS there,
            // measured old-to-old, and only its DIRECTION relaxes. Measuring
            // it from the swept predecessor instead would let every chord's
            // length wander a little each frame with nothing to restore it,
            // and the body's arc length — n × cube_size, the thing the ring
            // count means — would drift out from under the draw.
            let seg = old_p - prev_old;
            let len = length(seg);
            var dir = prev_dir;
            if (len > 1e-4) { dir = normalize(mix(seg / len, prev_dir, w)); }
            let dir_w = atan2(dir.z, dir.x);
            if (prev_slot < RIBBON_SPINE_SLOTS) {
                let wa = ribbon_spine[prev_slot].w;
                ribbon_spine[prev_slot].w = wa + 0.5 * wrap_pi(dir_w - wa);
            }
            let q = prev_p + dir * len;
            ribbon_spine[slot] = vec4(q, dir_w);                       // provisional: its arriving chord
            prev_old = old_p;
            prev_p = q;
            prev_dir = dir;
            prev_slot = slot;
        }
    }
    ribbon_body_rw.head = hd;
}

// ── THE BODY ─────────────────────────────────────────────────────────────
// One thread per ring. Rest from the spine, gesture from the wave, the Sky
// Rule at the ring (the world, the body, the floor), a string under tension,
// the leash, the wall, the frame from what is drawn, the motor, the field's
// sample, the saddle.
@compute @workgroup_size(64)
fn ribbon_body(@builtin(global_invocation_id) gid: vec3<u32>) {
    let k = gid.x;
    if (k >= 400u) { return; }
    let ribbon = ribbon_state;
    let hd = ribbon_body_rw.head;
    let n = min(ribbon.cube_count, 400u);
    let emit_now = (hd.tick & 1u) * RIBBON_EMIT_SLOTS;              // the half this frame writes
    let emit_prev = RIBBON_EMIT_SLOTS - emit_now;                    // the half everyone reads this frame
    if (k >= n || ribbon.is_visible == 0u || n < 2u || hd.seeded == 0u) {
        ring_xforms[k] = RibbonRingTransform(vec4(1.0, 0.0, 0.0, 0.0), vec4(0.0), vec3(0.0));
        if (k % RIBBON_EMIT_STRIDE == 0u) { ribbon_body_rw.emit[emit_now + k / RIBBON_EMIT_STRIDE] = vec4(0.0); }
        return;
    }
    let cs = max(ribbon.cube_size, 1e-3);
    let dt = min(signal.dt, RIBBON_DT_MAX);
    let km = select(k - 1u, k, k == 0u);
    let kp = min(k + 1u, n - 1u);
    let fresh = hd.tick == 1u;                  // first body tick after a seed: the previous ribbon's bulges die here
    let rd = (hd.tick & 1u) * 400u;             // last frame's deformation
    let wr = 400u - rd;                         // this frame's

    let p_rest = ribbon_drawn_rest(k, n, hd, ribbon);
    let d0 = select(ribbon_body_rw.deform[(rd + k) * 2u].xyz, vec3(0.0), fresh);
    let v0 = select(ribbon_body_rw.deform[(rd + k) * 2u + 1u].xyz, vec3(0.0), fresh);
    let dm = select(ribbon_body_rw.deform[(rd + km) * 2u].xyz, vec3(0.0), fresh);
    let dp = select(ribbon_body_rw.deform[(rd + kp) * 2u].xyz, vec3(0.0), fresh);

    // The Sky Rule at this ring: the world, the body, the floor (ground includes pyramids).
    let p_now = p_rest + d0;
    let g = ribbon_ground(p_now.xz);
    let skip = select(32u, config.possessed_slot, point_ribbon_hosted());
    var force = sky_push(p_now, p_now.y - g, config.ribbon_clear_body, config.ribbon_clear_body, 264u, skip) * RIBBON_BODY_PUSH;
    if (!fresh) {
        force += sky_self(p_now, k, n, cs, emit_prev, RIBBON_CLEAR_SELF_BODY) * (RIBBON_BODY_PUSH * RIBBON_SELF_BODY_W);
    }
    let under = saturate((g + RIBBON_BODY_FLOOR - p_now.y) / RIBBON_BODY_FLOOR);
    force.y += under * under * RIBBON_BODY_PUSH;

    // A string under tension: spring home, damped, coupled to its neighbors, leashed.
    let accel = force
              - RIBBON_BODY_K * d0
              - 2.0 * RIBBON_BODY_ZETA * sqrt(RIBBON_BODY_K) * v0
              + RIBBON_BODY_TENSION * (dm + dp - 2.0 * d0);
    var v1 = v0 + accel * dt;
    var d1 = d0 + v1 * dt;
    let dl = length(d1);
    if (dl > RIBBON_BODY_DMAX) { d1 *= RIBBON_BODY_DMAX / dl; }

    // THE WALL: law outranks leash. The velocity into the wall dies; the rest lives.
    let wall = sky_wall(p_rest + d1, g, cs * RIBBON_WALL_HALF, skip);
    if (dot(wall, wall) > 0.0) {
        d1 += wall;
        let nrm = normalize(wall);
        v1 -= min(dot(v1, nrm), 0.0) * nrm;
    }
    ribbon_body_rw.deform[(wr + k) * 2u]      = vec4(d1, 0.0);
    ribbon_body_rw.deform[(wr + k) * 2u + 1u] = vec4(v1, 0.0);

    // The drawn centerline and its frame: the tangent of what is drawn
    // (spine + gesture + deformation; the neighbors' deformation one frame old).
    let center = p_rest + d1;
    let c_m = select(ribbon_drawn_rest(km, n, hd, ribbon) + dm, center, k == 0u);
    let c_p = select(ribbon_drawn_rest(kp, n, hd, ribbon) + dp, center, k == n - 1u);
    let tangent = c_p - c_m;                                   // tailward
    var tn = vec3(cos(hd.heading), 0.0, sin(hd.heading));
    let tl = length(tangent);
    if (tl > 1e-4) { tn = tangent / tl; }
    let yaw   = atan2(tn.z, tn.x);                             // the tailward heading w' (a wrap is harmless: the rotor double-covers)
    let pitch = atan2(tn.y, max(length(tn.xz), 1e-4));         // elevation of the tailward tangent — BNK-1 passes rotor(Z, elevation)
    let t = f32(k) / f32(n - 1u);
    let slopes = ribbon_wave_slopes(ribbon_phase_age(t, ribbon), ribbon);
    let p_speed = max(ribbon.propagation_speed, 1e-3);
    let lateral = vec3(-tn.z, 0.0, tn.x);
    let roll = clamp(RIBBON_BANK_GAIN * (slopes.x / p_speed) + RIBBON_BODY_BANK * dot(v1, lateral),
                     -RIBBON_BANK_MAX, RIBBON_BANK_MAX);
    // Compose as BNK-1 verified: roll about the tube axis, pitch about the
    // lateral axis, then the base yaw (negated: rotor+sw_mp map +X to
    // (cos θ, −sin θ)), then translate. First argument applies first.
    // The tangent-align law is now structural: the frame IS the drawn tangent.
    let base_yaw = rotor(vec3(0.0, 1.0, 0.0), -yaw);
    let r_pitch  = rotor(vec3(0.0, 0.0, 1.0), pitch);
    let r_roll   = rotor(vec3(1.0, 0.0, 0.0), roll);
    let orient   = gp_mm(gp_mm(r_roll, r_pitch), base_yaw);
    let motor    = gp_mm(orient, Motor(vec4(1.0, 0.0, 0.0, 0.0), vec4(-0.5 * center, 0.0)));
    ring_xforms[k] = RibbonRingTransform(motor.p0, motor.p1, center);

    // The field's view, and the body's view of itself next frame: one sample
    // every EMIT_STRIDE rings into this frame's half.
    if (k % RIBBON_EMIT_STRIDE == 0u) {
        ribbon_body_rw.emit[emit_now + k / RIBBON_EMIT_STRIDE] = vec4(center, cs * 0.5);
    }

    // The saddle (ring 0): set back toward the tail, half a tube up, in the
    // ring's own frame — the same motor the tube is drawn with, so rider and
    // ring cannot disagree.
    if (k == 0u) {
        var sd: RibbonSaddle;
        sd.pos = sw_mp(motor, vec3(config.ribbon_mount_setback, cs * 0.5, 0.0));
        sd.heading = hd.heading;
        sd.yaw_off = wrap_pi(yaw - hd.heading);
        sd.pitch = pitch;
        sd.roll = roll;
        sd._pad0 = 0.0;
        ribbon_body_rw.saddle = sd;
    }
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
    let ribbon = scene_constants.ribbon;
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
        // Pre-compute frame (rare): identity motor. ribbon_body writes the
        // rings before render reads them, so this branch doesn't fire in
        // normal operation.
        motor = Motor(vec4(1.0, 0.0, 0.0, 0.0), vec4(0.0));
    }
    let orient = Motor(motor.p0, vec4(0.0));

    var world_pos = sw_mp(motor, local_pos);
    let world_normal = sw_mp(orient, local_normal);

    var out: EntityVarying;
    out.clip_pos = frame_r.vp.m * vec4(world_pos, 1.0);
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
    let ribbon = scene_constants.ribbon;
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
    out.clip_pos = shadow_light_vp() * vec4(world_pos, 1.0);
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
@group(1) @binding(0)   var<uniform>             signal: FrameSignal;
@group(0) @binding(0)   var<uniform>             config: DesignConfig;
@group(2) @binding(240)   var<storage, read_write> vp_data: VPMatrix;

// Agent system — unified entity buffer. Slot 0 is the player's body;
// slots 1..31 are mood-authored agents. The player's relationship
// to this array is config.possessed_slot. Array size matches
// Dim::MAX_AGENTS (32) in state.hpp — keep in sync.
@group(2) @binding(0)  var<storage, read_write> agent_state: array<AgentState, 32>;

// Portal proximity array (uploaded by CPU, checked in behavior_player_controlled)
struct PortalEntry {
    x: f32,
    z: f32,
    facing_cos: f32,
    facing_sin: f32,
    inv_span_sq: f32,
    inv_depth_sq: f32,
    arch_index: u32,
    kind: u32,   // 0 forward, 1 back (ATRIUM_2: the passers walk forward doors only)
}
struct PortalArray {
    count: u32,
    _pad0: u32,
    _pad1: u32,
    _pad2: u32,
    portals: array<PortalEntry, 32>,
}
// THE AGENTS' ROOM CONSTANTS (CHORD_1) — one cadence, one block.
// Everything here is CPU-authored at world/mood cadence. Mirrors
// GPUAgentRoomConstants in state.hpp BYTE-FOR-BYTE (6960 B; the
// static_asserts are the handshake). Offsets: portals 0,
// behaviors 1040, tier_gains 1392, occupier_cmg 1584,
// occupier_amg 5680. (ATRIUM_4 grew behaviors by one row, +32 B.)
struct AgentRoomConstants {
    portals: PortalArray,
    behaviors: array<AgentBehaviorParams, 11>,
    tier_gains: array<AgentTierParams, 4>,
    occupier_cmg: array<ColumnMeshParams, 32>,
    occupier_amg: array<ArchMeshParams, 16>,
}
@group(2) @binding(1) var<uniform> agent_room: AgentRoomConstants;

@group(2) @binding(241)  var<storage, read_write> camera_state: CameraState;
@group(2) @binding(2) var<storage, read_write> floating_entities: FloatingEntityArray;
@group(2) @binding(140) var<uniform>             ribbon_state: RibbonState;

// Possessed-agent helpers (compute stage). Every kernel that used to
// read pawn_state.pos now goes through these. Extracting here keeps
// the indexing + scalar→vec conversion at one site and the call sites
// read as "the pawn's pos" without repeating the slot lookup.
fn compute_pawn_pos() -> vec3<f32> {
    let a = agent_state[config.possessed_slot];
    return vec3(a.pos_x, a.pos_y, a.pos_z);
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
@group(2) @binding(5) var<storage, read> render_agents: array<AgentState, 32>;
// CHORD_5: promoted back to read-only storage — the uniform ceiling
// (54,912 of 65,536 B) was a wall on entity growth; post-LOOM the
// storage rows afford the seat. Demotion: Table C. Reversal: CHORD.md.
@group(2) @binding(6) var<storage, read> render_floating: FloatingEntityArray;

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
    if (point_camera_hosted()) { return frame_r.camera.pos; }
    return render_pawn_pos();
}
fn render_pawn_vel_xz() -> vec2<f32> {
    let a = render_agents[config.possessed_slot];
    return vec2(a.vel_x, a.vel_z);
}

// --- Ribbon (the render rooms' read of ring_xforms)
@group(2) @binding(143) var<storage, read> render_ring_xforms: array<RibbonRingTransform, 400>;
// Entity ground atlas — VS reads ground_y via textureLoad (r32float, 256×1)
@group(3) @binding(81) var entity_ground_atlas: texture_2d<f32>;

// Atlas slot offsets (must match Dim:: constants in state.hpp)
const GROUND_ATLAS_ARCH: i32     = 0;
const GROUND_ATLAS_COLUMN: i32   = 16;
// slots 48..55: A DOCUMENTED HOLE. Do NOT re-pack — these offsets are
// hand-mirrored with state.hpp Dim::GROUND_ATLAS_*.
const GROUND_ATLAS_PALM: i32     = 56;
const GROUND_ATLAS_CACTUS: i32   = 80;
const GROUND_ATLAS_BLADE: i32    = 100;

// --- The ribbon room (ribbonStateLayout_; RIBBON_1)
// ring_xforms: written by ribbon_body, read by the render rooms as 143.
@group(2) @binding(141) var<storage, read_write> ring_xforms: array<RibbonRingTransform, 400>;
// ribbon_spine: the chord ring. .xyz = a head sample, .w = the heading at emission.
@group(2) @binding(142) var<storage, read_write> ribbon_spine: array<vec4<f32>, 402>;
// ribbon_body_rw: head (ribbon_head writes), saddle + emit + deform (ribbon_body writes).
@group(2) @binding(144) var<storage, read_write> ribbon_body_rw: RibbonBody;
// The agents' room's read of the same buffer: the mount reads .saddle, the field reads .emit.
@group(2) @binding(145) var<storage, read> ribbon_body_read: RibbonBody;

// --- Light system (Group 0: render, bindings 320-339)
// WALLET_1revA: one uniform block, not three storage bindings. 321 and
// 322 are retired; the sun, the point array and the spot array reach
// the fragment stage as `frame_r.lighting.sun` / `.points` / `.spots`.
// FRAME R (CHORD_3) — the render frame's block: lighting (CPU-
// authored) + vp + camera (GPU-sovereign, arriving by encoder copy
// from vp_data / camera_state each frame — the CPU never reads
// them). Two instances back two bind groups over this layout: main
// and photographer. Mirrors GPUFrameR in state.hpp BYTE-FOR-BYTE
// (1040 B). Offsets: lighting 0, vp 848, camera 976, sphere_pos 1024.
//
// THIRD PASSENGER (BEQ_A). sphere slot 0's position — the terrain
// fragment stage's one frame-uniform read of render_floating,
// promoted here under the CHORD_5 law (uniform-read must never sit
// in storage; frame_r is the program's now). GPU-sovereign like vp
// and camera: update_sphere writes it, the frame encoder copies 12
// bytes from Floating Entity Array offset 0 into each instance, and
// the CPU never reads it.
struct FrameR {
    lighting: Lighting,
    vp: VPMatrix,
    camera: CameraState,
    sphere_pos: vec3<f32>,
    _pad_sphere: f32,
}
@group(1) @binding(1) var<uniform> frame_r: FrameR;

// ─── THE SHADOW TILE'S LIGHT INDEX ────────────────────────────────
// One u32 on a DYNAMIC-OFFSET uniform seat: the buffer is four
// 256-byte records holding 0..3, written once at boot, and the shadow
// pass rebinds group 1 with this light's offset. Every other group-1
// render bind carries offset 0.
//
// WHY AN INDEX AND NOT THE MATRIX. A matrix would need the SUN's on
// the outdoor path, and the sun VP's only writer is update_camera_vp — on
// the GPU, every frame; a CPU-pushed matrix would give it two owners
// at two cadences. An index has no owners and no cadence, and its
// failure mode is a validation error rather than a wrong pixel.
@group(1) @binding(2) var<uniform> shadow_slot: u32;

// D2' — the shadow VS's light matrix, from where it already lives.
// Outdoors (no spots) the sun VP is frame_r.vp.light_vp, written by
// update_camera_vp and read here exactly as the 13 shadow VSes read it
// before. Indoors it is the per-light matrix that already rides the
// lighting buffer, the same array sample_spot_shadow_pcf indexes in the
// fragment stage. Nothing is duplicated and nothing new is written.
fn shadow_light_vp() -> mat4x4<f32> {
    if (frame_r.lighting.spots.count == 0u) {
        return frame_r.vp.light_vp;
    }
    return frame_r.lighting.spots.lights[shadow_slot].view_proj;
}

// --- Render textures (Group 1: bindings 22-23, 25-27)
@group(1) @binding(5) var bilinear_sampler: sampler;
@group(1) @binding(6) var nearest_sampler: sampler;
@group(3) @binding(200) var shadow_map: texture_depth_2d;           // sun shadows (outdoor) / spot atlas lights 0-1 (indoor)
@group(3) @binding(201) var shadow_sampler: sampler_comparison;
@group(3) @binding(202) var spot_shadow_map: texture_depth_2d;     // spot atlas lights 2-3 (indoor)

// §7.0a PATCH GENERATION BINDINGS


// --- Patch heightfield generation (Group 0: bindings 23-24)
// Separate pipeline layout. Dispatched once a frame for the whole batch of
// patches that entered the active set. Writes one layer of the patch
// heightfield array per patch.
// THE BATCH, not the patch. A read-only STORAGE array indexed by
// workgroup_id.z: one dispatch bakes every patch in the frame's batch, where
// there used to be one pass pair and one 256-byte dynamic-offset ring slot
// per patch. The dynamic seat retires with it.
@group(2) @binding(40) var<storage, read> patch_params_batch: array<PatchParams>;
// .a = the terrain's reserve field — 28.125 MiB pre-paid, nine consumers wired, write nothing until a campaign names it (LOOM ruling).
@group(3) @binding(40) var patch_heightfield_array_write: texture_storage_2d_array<rgba16float, write>;
@group(0) @binding(1) var<uniform> tile_grid: TileGrid;
@group(3) @binding(41) var patch_cell_color_array_write: texture_storage_2d_array<rgba8unorm, write>;

// --- Patch rendering (Group 0: binding 340, 391; Group 1: bindings 28-29)
@group(2) @binding(61) var<storage, read> patch_instances: array<PatchInstance>;
@group(3) @binding(44) var patch_heightfield_array_read: texture_2d_array<f32>;
@group(3) @binding(45) var patch_cell_color_array_read: texture_2d_array<f32>;

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
    // GOL_RULES_1: the two trailing pad words, renamed in place. Same
    // offsets, same 80 bytes; the CPU twin's static_assert is the witness.
    rule_mask: u32,              // Conway B/S bitset: bit n birth, bit 9+n survival
    field_fn: u32,               // Pulse field function id (PULSE_FIELD_*)
}

// --- GoL Zone Visual Parameters
const GOL_COLOR_NEUTRAL:  u32 = 0u;  // no color change (height-only extrusion)
const GOL_COLOR_LENS:     u32 = 1u;  // shift ground toward per-zone target color

// Algorithm constants (must match CPU AlgorithmType::)
const GOL_ALGORITHM_CONWAY: u32 = 0u;

// Pulse field-function constants (must match CPU PulseField::)
const PULSE_FIELD_BREATH: u32 = 0u;   // what ships today
const PULSE_FIELD_SPIRAL: u32 = 1u;

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
                     tempo_randomness: f32, field_fn: u32,
                     grid_size: u32) -> f32 {
    // Per-cell phase offset from hash
    let h = gol_cell_hash(cell_x, cell_y);
    let cell_phase = gol_cell_variation(h) * phase_randomness * 2.0 * PI;

    // Per-cell frequency jitter: each cell oscillates at a slightly different tempo
    // tempo_randomness=0 → all unison. =1 → ±50% frequency variation.
    let h2 = gol_cell_hash(cell_x + 137u, cell_y + 251u);
    let tempo_jitter = 1.0 + (gol_cell_variation(h2) - 0.5) * tempo_randomness;

    if (field_fn == PULSE_FIELD_SPIRAL) {
        // A two-armed phase spiral about the zone centre, CONTINUOUS where
        // BREATH is binary. Nothing clamps this return: the shared
        // critically damped spring drives visual to whatever it is, and the
        // gradation between neighbouring cells IS the row. Height never
        // reads it — the Spiral tier sets force_no_height, so alive_height
        // was already zeroed at derive; tint does, which is the intent.
        // The two scatter terms above are reused verbatim at the same
        // placement BREATH gives them: tempo on the temporal rate, phase on
        // the accumulated phase — and the rate carries
        // config.mode_gol_tick_scale in the same shape BREATH gives it, so
        // the mode tick dial reaches both Pulse fields alike the day it
        // gets a driver. (No TAU constant exists in this module; 2.0 * PI
        // is the idiom already in this function.)
        let n = f32(grid_size);
        let p = vec2<f32>(f32(cell_x) + 0.5 - n * 0.5, f32(cell_y) + 0.5 - n * 0.5);
        let r = length(p);
        let th = atan2(p.y, p.x) / (2.0 * PI);
        let arms = 2.0;
        let arm_wavelength = n * 0.5;
        let u = th * arms + r / arm_wavelength
              - t_beats * tempo_jitter
                / max(tick_period * config.mode_gol_tick_scale, 0.01);
        return 0.5 + 0.5 * cos(u * 2.0 * PI + cell_phase);
    }

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

// --- Terrain cell color at a world position. cell_address is the only
//     address derivation (THE ONE-ADDRESS LAW).
fn gol_composite_cell_color(world_xz: vec2<f32>) -> vec3<f32> {
    let addr = cell_address(world_xz);
    let cell_gx = addr.x;
    let cell_gz = addr.y;
    let cell_seed = lattice_node_seed(config.world_seed, vec2(cell_gx, cell_gz), 200u);

    // IDENTITY VOICE: GoL keeps its own panel (ROW 9), so the door bias
    // args are 0 here — amount 0 maps each cell to its seed color. When
    // the zones' own coupling pass convenes (Jean's, STATUS: INTENT), it
    // revives by passing config.checker_resultant / _music_amount /
    // _music_variance in place of those zeros.
    let id = evaluate_cell_fields(world_xz, cell_gx, cell_gz, cell_seed, 0.0, 0.0);
    let dcol = discrete_cell_color_at_tier(world_xz, cell_gx, cell_gz, cell_seed,
                                           id.tier, vec3(0.0), 0.0, 0.0);
    return composite_cell_color(id, dcol);
}

// --- Shared color application (the terrain FS is the sole caller)
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

// L3 MIRROR: Dim::MAX_GOL_ZONES (state.hpp). Sizes BOTH zone arrays below.
const MAX_GOL_ZONES: u32 = 8u;

struct GoLZoneArray {
    count: u32,
    t_beats: f32,
    dt: f32,
    tick_mask: u32,              // bit N = zone N should tick Conway this frame
    zones: array<GoLZoneConfig, MAX_GOL_ZONES>,
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
    effect_mask: u32,          // STATUS: INTENT — mirrored, never read (TUNE_1 A4).
    aura_n: u32,               // likewise unread: sample_pawn_aura uses PAWN_AURA_N.
    tint_strength: f32,
    tint_r: f32,
    tint_g: f32,
    tint_b: f32,
    delta_mode: u32,           // 0=convergent (toward tint), 1=random per-cell
    delta_magnitude: f32,      // random mode: max offset per channel
    t_beats: f32,              // current musical time (for oscillation)
    height_scale: f32,         // height contribution scale (world units)
}

const AURA_DELTA_RANDOM: u32 = 1u;

// Helper: sample pawn aura with toroidal lookup and ghost rejection.
// Returns vec4(height_blend, delta_r, delta_g, delta_b) or vec4(0) if ghost/inactive.
//
// Three call sites, all greppable: contrib_pawn_aura_at_external(xz),
// patch_terrain_vs, and patch_terrain_fs. NOT called by the pawn's own Y
// resolve: POLICY_WALKER uses contrib_pawn_aura_at_self() which returns
// the scalar peak directly — the pawn knows it sits at its own aura peak
// without reading the directionally-biased grid. See those functions
// for rationale.
//
// SAMPLER CHOICE — AND THE MECHANISM THE OLD COMMENT CLAIMED, CORRECTED.
// It read: bilinear was chosen over nearest so consumers "reading across
// cell boundaries" get smooth interpolation, because nearest "would
// produce visible banding at cell boundaries (one PATCH_CELL_SIZE)".
// The body defeats that. aura_uv is built from floor(world_xz / aura_cs)
// plus a half-texel — an EXACT texel centre — so bilinear degenerates to
// nearest and the result is constant across a PATCH_CELL_SIZE cell and
// steps at every boundary. The comment described the banding it caused.
//
// Read from the addressing, not from the sampler. The filtering mode is
// real (rgba16float is filterable) and harmless; it is simply not doing
// the job the comment credited it with. Whether the aura SHOULD be
// continuous is a design question this campaign did not open — but until
// it is opened, this function returns a per-cell step, and any consumer
// that needs continuity must know that. PENUMBRA_1 P4 is exactly such a
// consumer: it stopped routing this value into the shadow sample.
fn sample_pawn_aura(world_xz: vec2<f32>, pawn_xz: vec2<f32>) -> vec4<f32> {
    if (config.aura_enabled < 0.5) { return vec4(0.0); }
    // NOT pawn_aura_cfg.*: that uniform is bound ONLY in the Pawn Aura
    // Compute Layout, and this sampler runs in the terrain VS, the terrain FS
    // and the compute-entity policy path — three pipelines that never bind it.
    // The two constants are the only spelling reachable from here, and they
    // are the same numbers the CPU stages into cell_size / aura_n.
    let aura_cs = PATCH_CELL_SIZE;
    let aura_n = PAWN_AURA_N;
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

@group(2) @binding(101) var<storage, read_write> zone_config: GoLZoneArray;
@group(2) @binding(102) var<storage, read_write> zone_life: array<f32>;
@group(3) @binding(101) var zone_life_tex_write: texture_storage_2d_array<r32float, write>;

// --- GoL zone system (Group 1: bindings 31-32, render texture layout)
@group(3) @binding(102) var zone_life_read: texture_2d_array<f32>;
@group(2) @binding(104) var<storage, read> zone_params: GoLZoneArray;
@group(3) @binding(21) var pawn_aura_read: texture_2d<f32>;
@group(3) @binding(103) var live_card_read: texture_2d<f32>;  // GROUND_CARD_1: the live card (sampled; render + compute)

// --- Pawn Aura compute bindings
// (Group 0: dedicated layout with bindings 60, 170-172)
@group(2) @binding(20) var<uniform> pawn_aura_cfg: PawnAuraConfig;
@group(2) @binding(21) var<storage, read_write> pawn_aura_cells: array<PawnAuraCell>;
@group(3) @binding(20) var pawn_aura_tex_write: texture_storage_2d<rgba16float, write>;
@group(3) @binding(100) var live_card_write: texture_storage_2d<rgba16float, write>;  // GROUND_CARD_1: writer kernel

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
    // GOL_TEMPO_2 U1 — THE ONE-AUTHOR LAW. The leading pad word,
    // renamed in place: same offsets, same 32 bytes, and the CPU twin's
    // static_assert in state.hpp is the witness that nothing grew.
    // sample_gaussian runs log and cos, and the spec licenses per-backend
    // accuracy for both — so a GPU re-draw of the tick would land on a
    // different rung of GOL_TICK_LADDER than the CPU's gate whenever the
    // draw fell near a boundary. The CPU draws and snaps; this carries it.
    tick_period: f32,       // beats, already on GOL_TICK_LADDER
    _pad1: u32,
}

struct ZoneDeriveRequestArray {
    count: u32,
    _pad0: u32,
    _pad1: u32,
    _pad2: u32,
    requests: array<ZoneDeriveRequest, MAX_GOL_ZONES>,
}

@group(2) @binding(103) var<uniform> zone_derive_requests: ZoneDeriveRequestArray;

// Constants for zone derivation (must match CPU GoLZoneSpawnConfig / GoLColorMode)
// A zone's extent is tier-derived: grid_cells × PATCH_CELL_SIZE.
// L3 MIRROR: GoLZoneSpawnConfig::HEIGHT_FACTOR_CLAMP_HI (bodies/gol_zones.hpp).
// The per-cell height_factor is a CPU Gaussian clamped to [LO, HI] and then
// multiplied by the birth mask's {0,1}, so this is the true upper bound on
// the per-cell multiplier. The indoor cap below divides by it. Both rooms.
const GOL_HEIGHT_FACTOR_MAX: f32 = 1.4;
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

    // Each row's own slot; the other algorithm's stays 0.
    zc.rule_mask = 0u;
    zc.field_fn  = 0u;

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
        zc.extent    = f32(zc.grid_size) * PATCH_CELL_SIZE;

        let actual_height = height_enabled && (tp.force_no_height == 0u);

        zc.tick_period = req.tick_period;
        zc.spring_stiffness = max(0.1,
            sample_gaussian(seed, ZONE_PROP_SPRING, tp.spring_stiffness_mean, tp.spring_stiffness_sigma));
        zc.transition_fraction = clamp(
            sample_gaussian(seed, ZONE_PROP_TRANSITION, tp.transition_fraction_mean, tp.transition_fraction_sigma),
            0.01, 0.5);
        zc.alive_height = select(0.0,
            max(0.5, sample_gaussian(seed, ZONE_PROP_HEIGHT, tp.alive_height_mean, tp.alive_height_sigma)),
            actual_height);
        zc.spring_variance = tp.spring_variance;
        zc.rule_mask = tp.rule_mask;

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
        zc.extent    = f32(zc.grid_size) * PATCH_CELL_SIZE;

        let actual_height = height_enabled && (pp.force_no_height == 0u);

        zc.tick_period = req.tick_period;
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
        zc.field_fn = pp.field_fn;

        // Pulse zones always use LENS color mode
        zc.color_mode = GOL_COLOR_LENS;
    }

    // THE INDOOR HEIGHT CAP. Pre-Stage-5 a zone that punched a ceiling was a
    // separate extrusion mesh; now GoL IS the ground, so an uncapped indoor
    // zone lifts the LAND through the roof of a room the camera is clamped
    // inside. Applied HERE because derive is the only site that sees
    // alive_height, and it runs ONCE PER ZONE BIRTH — 8 per world — instead
    // of at every card texel, walker, flyer, placement and entity VS.
    // DIVIDED BY THE CLAMP BOUND: the realised lift is
    //   visual · alive_height · height_factor · mode_gol_height_scale
    // with visual ∈ [0,1] (apply_boundary) and height_factor ≤
    // GOL_HEIGHT_FACTOR_MAX, so bounding alive_height by cap/MAX makes the
    // MAXIMUM realised lift exactly the cap. A cell at the top of the mask
    // range reaches exactly INDOOR_HEIGHT_CAP_FRACTION × ceiling; cells below
    // reach less, which is the mask doing its job, not the cap lying.
    // cap == 0 is the disable sentinel — outdoor stays byte-identical.
    if (config.indoor_height_cap > 0.0) {
        zc.alive_height = min(zc.alive_height,
                              config.indoor_height_cap / GOL_HEIGHT_FACTOR_MAX);
    }

    // Zone origin: snap corner to the cell grid with the TIER-DERIVED
    // extent, then center (the same snap formula; extent now varies).
    let corner_x = floor((raw_cx - zc.extent * 0.5) / PATCH_CELL_SIZE) * PATCH_CELL_SIZE;
    let corner_z = floor((raw_cz - zc.extent * 0.5) / PATCH_CELL_SIZE) * PATCH_CELL_SIZE;
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

// §7.1 COMPUTE ENTRY POINTS
// Execution order (critical for correctness):

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
//   POLICY_WALKER_TILT  gives climb-safe heights for the slope-law
//                       comparison. Excludes the two
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
// THE SLOPE LAW, one home for all three candidate tests below.
// dh = the tilt-height rise of the candidate move; dxz = its horizontal
// length. Blocked iff the rise clears the noise floor AND its grade
// exceeds PAWN_MAX_SLOPE — so downhill (dh ≤ 0) always passes.
//
// `|` and not `||`: the non-short-circuiting form. Both operands are
// always evaluated, which keeps this function's branch count exactly
// what the height test had. No living law governs branching in this
// chain — L2 was struck (PIVOT_0; docs/FXC_LAWS_RECORD.md). The shape
// stands until a measurement asks; a reshape's witness is the
// per-browser boot. max(dxz, 1e-4) makes the divide
// total; dxz is 0 only when the candidate did not move on that axis,
// where dh is 0 too and the noise floor already passed it.
fn slope_passable(dh: f32, dxz: f32) -> bool {
    return (dh <= PAWN_SLOPE_NOISE_FLOOR) | (dh / max(dxz, 1e-4) <= PAWN_MAX_SLOPE);
}

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
    if (!moved || slope_passable(y_tilt - prev_y_tilt, distance(new_xz, prev_xz))) {
        return vec4(new_xz.x, y, new_xz.y, 1.0);              // happy path
    }

    // Full move blocked — try axis-aligned slides. Each axis reads
    // both walker (the y the pawn would stand at) and walker_tilt
    // (the step-climb comparison) from a single paired query.
    let slide_x = vec2(new_xz.x, prev_xz.y);
    let x_pair  = query_ground_walker_pair(slide_x, qi);
    // Each slide's dxz is its own axis delta — the other axis is held.
    let x_ok = slope_passable(x_pair.y - prev_y_tilt, abs(new_xz.x - prev_xz.x));

    let slide_z = vec2(prev_xz.x, new_xz.y);
    let z_pair  = query_ground_walker_pair(slide_z, qi);
    let z_ok = slope_passable(z_pair.y - prev_y_tilt, abs(new_xz.y - prev_xz.y));

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
// from velocity. Factored out of each behavior body — good structure on
// its own. (Shaped under the FXC compile-cost law — retired, PIVOT_0;
// docs/FXC_LAWS_RECORD.md §PROBATE.) No living law bars branching in
// this chain; reshape only with a measured reason.

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

// ─── Shared post-step (VELOCITY shaping only) ────────────────────
// agent_post_step applies drag, the speed cap, and the C2b steering
// block — velocity shaping ONLY. Integration + ground snap + heading
// moved to agent_settle (CONTACT_3 K1a), which the KERNEL calls AFTER
// the contact gather: THE SPEED CAP GOVERNS INTENT, NOT IMPOSITION — a
// body chooses how fast it WALKS, not how fast it is DISPLACED by
// another body. Behaviors' call sites are unchanged (same name, same
// params); only the contract narrowed.
//
// Each behavior reads its own (drag, speed_cap) from
// agent_room.behaviors[id], with tier scaling applied via
// agent_room.tier_gains[tier_idx].speed_gain — that's why both the
// raw cap and the tier multiplier come in as parameters.
fn agent_post_step(agent_in: AgentState, drag: f32, speed_cap: f32, speed_gain: f32) -> AgentState {
    var a = agent_in;
    let dt = signal.dt;

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

    // ── CONTACT_2 C2b — potential-field steering (the whisper) ─────
    // Read the ground's own slope ahead and deflect ALONG the level-
    // set — velocity-shaping only, nothing inside the ground-resolve
    // chain (the old FXC sanctum — docs/FXC_LAWS_RECORD.md; the chain
    // is still kept branchless, now by taste rather than by law).
    // Branchless: smoothstep is 0 below LO and
    // min(1,sp2) quiets it at standstill; the max(glen,eps) hardens the
    // handoff's normalize against a flat-ground zero. The walkable
    // cliff-clamp stays the wall (dead-center local minimum is the
    // clamp's case, by design: hard in spirit, soft in essence).
    {
        let sp2s = a.vel_x * a.vel_x + a.vel_z * a.vel_z;
        let inv_sp = inverseSqrt(max(sp2s, 0.0001));
        let ahead = vec2(a.pos_x, a.pos_z)
                  + vec2(a.vel_x, a.vel_z) * inv_sp * STEER_LOOKAHEAD_WU;
        let g = sample_terrain_grad_at(ahead);
        let glen = length(g);
        let steep = smoothstep(STEER_GRAD_LO, STEER_GRAD_HI, glen);
        let side = sign(a.vel_x * g.y - a.vel_z * g.x + 0.000001);
        let perp = (vec2(-g.y, g.x) / max(glen, 0.0001)) * side;
        let f = steep * STEER_GAIN * signal.dt * min(1.0, sp2s);
        a.vel_x += perp.x * f;
        a.vel_z += perp.y * f;
    }

    return a;
}

// agent_settle — INTEGRATION + ground snap + heading, moved out of
// agent_post_step at CONTACT_3 K1a. The KERNEL calls this AFTER the
// contact gather, so the speed cap governs INTENT, not imposition — a
// body does not choose how fast it is displaced by another body. sp2 is
// recomputed from the POST-gather velocity so the integration and the
// heading reflect the imposed motion too.
fn agent_settle(agent_in: AgentState) -> AgentState {
    var a = agent_in;
    let dt = signal.dt;
    let t  = signal.t_seconds;
    let sp2 = a.vel_x * a.vel_x + a.vel_z * a.vel_z;

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
// modifies velocity only — drag, cap, and steering are factored into
// agent_post_step; integration, ground snap, and heading into
// agent_settle (the kernel calls it AFTER the gather). The kernel switch
// dispatches on agent.behavior_id; slot numbers must match the
// AgentBehaviorId enum in bodies/agents.hpp. Behavior parameters
// come from agent_room.behaviors[behavior_id] (uploaded once at world
// init from the C++ AGENT_BEHAVIORS table).

// ─── Behavior: PlayerControlled ──────────────────────────────────
// The body the player is currently inhabiting. Reads input/couplings,
// resolves ground, computes tilt orientation, detects portal triggers.
// Only meaningful for the slot whose behavior_id is PLAYER_CONTROLLED
// — by convention that is the same slot as config.possessed_slot.
fn behavior_player_controlled(agent_in: AgentState) -> AgentState {
    var agent = agent_in;

    // RIBBON host: the pawn rides the saddle the body kernel wrote this pass
    // (ribbon_body_read.saddle — the ring-0 frame the tube is drawn with, so
    // rider and ring cannot disagree). Boarding is a trajectory: from the pose
    // the body left (signal.mount_from, captured at the edge) to the saddle,
    // eased over mount_phase; pitch and roll arrive with the ease.
    if (point_ribbon_hosted()) {
        let sd = ribbon_body_read.saddle;
        var e = 1.0;
        if (signal.mount_kind == 1u) { e = mount_ease(signal.mount_phase); }
        agent.pos_x = mix(signal.mount_from.x, sd.pos.x, e);
        agent.pos_y = mix(signal.mount_from.y, sd.pos.y, e);
        agent.pos_z = mix(signal.mount_from.z, sd.pos.z, e);
        agent.heading = mix_heading(signal.mount_from_heading, sd.heading, e);
        agent.vel_x = 0.0;
        agent.vel_y = 0.0;
        agent.vel_z = 0.0;
        // Composed in the ring motor's verified order — roll, then pitch,
        // then yaw (quat_multiply applies its SECOND argument first). Negated:
        // quat_rotate maps +X to (cos θ, −sin θ); the heading speaks dir(θ).
        let q_yaw   = quat_from_axis_angle(vec3(0.0, 1.0, 0.0), -agent.heading - sd.yaw_off * e);
        let q_pitch = quat_from_axis_angle(vec3(0.0, 0.0, 1.0), sd.pitch * e);
        let q_roll  = quat_from_axis_angle(vec3(1.0, 0.0, 0.0), sd.roll * e);
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
        var world_vel = coupling_input_to_pawn_velocity(input_dir, camera_state.azimuth);

        // ── CONTACT_2 C2b — potential-field steering (the whisper) ─
        // The pawn reads the ground's slope ahead and bends around
        // monuments BEFORE the walkable clamp (the wall) ever fires; a
        // dead-center approach still stops. Steers the input intent
        // (world_vel) before it integrates, so the PATH bends this
        // frame. Branchless (see agent_post_step); world_vel.y is the
        // z-axis component.
        {
            let sp2s = world_vel.x * world_vel.x + world_vel.y * world_vel.y;
            let inv_sp = inverseSqrt(max(sp2s, 0.0001));
            let ahead = vec2(agent.pos_x, agent.pos_z)
                      + world_vel * inv_sp * STEER_LOOKAHEAD_WU;
            let g = sample_terrain_grad_at(ahead);
            let glen = length(g);
            let steep = smoothstep(STEER_GRAD_LO, STEER_GRAD_HI, glen);
            let side = sign(world_vel.x * g.y - world_vel.y * g.x + 0.000001);
            let perp = (vec2(-g.y, g.x) / max(glen, 0.0001)) * side;
            let f = steep * STEER_GAIN * signal.dt * min(1.0, sp2s);
            world_vel.x += perp.x * f;
            world_vel.y += perp.y * f;
        }

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

    // ── THE WIRE (BATCH G1): the possessed pawn consumes the occupier
    // push IN THE CANDIDATE — where its position is actually authored.
    // The push lands before the boundary clamp (so the clamp clamps it)
    // and before pawn_ground_resolve (so the slope law resolves the
    // pushed candidate, and the body's word and the ground's word
    // compose instead of racing). Both arms: an idle pawn is eased out
    // of a shaft the same as a walking one. The gather's occupier call
    // left this kernel with this wire — landing it there too would
    // apply the push twice. Free agents are untouched: their velocity
    // persists, so their path already consumes the rows.
    // A VALUE change on the existing candidate, adding no branch. That
    // is a description of the wire, not a constraint on it: L2 was
    // struck (PIVOT_0; docs/FXC_LAWS_RECORD.md), and no living law
    // governs branching here.
    {
        // SHELL_0: 1.0 for the law's dt — occupier_contact answers in wu/s
        // and the pos-add below integrates it ONCE. signal.dt here made the
        // response overlap × gain × dt², an effective stiffness of gain/60
        // that halved again at 30 fps: the collision was frame-rate
        // dependent and nobody could see it. body_radius is the possessed
        // figure's own (config.pawn_body_radius, HEM_0's wire) — the same
        // radius that insets the world box two lines down now insets every
        // shaft's shell, and a Colossal stands off proportionally.
        let o_push = occupier_contact(
            vec3(agent.pos_x, agent.pos_y, agent.pos_z),
            config.pawn_body_radius, 1.0);
        agent.pos_x += o_push.x * signal.dt;
        agent.pos_z += o_push.y * signal.dt;
    }

    // --- Finite world boundary clamp — THE BODY, NOT THE POINT (HEM_0).
    // The legal box is inset by the possessed figure's own radius
    // (config.pawn_body_radius, CPU-authored from PAWN_FIGURES[skin] on the
    // wire pawn_tilt_tau already rides). The inset is not cosmetic: see
    // world_box_clamp_xz for why bmax is not a coordinate a body may hold.
    {
        let cxz = world_box_clamp_xz(vec2(agent.pos_x, agent.pos_z),
                                     config.pawn_body_radius);
        agent.pos_x = cxz.x;
        agent.pos_z = cxz.y;
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
        // (orient_target, not target: `target` is a WGSL RESERVED WORD — naga
        //  and Tint both reject it as an identifier.)
        let orient_target = quat_multiply(tilt_quat, heading_quat);

        // Per-figure tilt lag (CLOSURE_PAWN [6]). The stored orientation is the
        // state this walks from — see the AgentState comment on why orientation
        // is stored rather than derived. tau = 0 collapses to the previous hard
        // assignment, so the regular pawn is byte-identical. CPU authors tau
        // from the possessed body's figure (config.pawn_tilt_tau).
        var orient = orient_target;
        let tau = config.pawn_tilt_tau;
        if (tau > 0.0001) {
            let cur = vec4(agent.orient_x, agent.orient_y, agent.orient_z, agent.orient_w);
            // Shortest arc: q and -q are the same rotation; pick the near twin.
            let c = select(cur, -cur, dot(cur, orient_target) < 0.0);
            // Frame-rate independent: same settle time at any dt.
            let a = 1.0 - exp(-dt / tau);
            orient = normalize(mix(c, orient_target, a));
        }
        agent.orient_x = orient.x;
        agent.orient_y = orient.y;
        agent.orient_z = orient.z;
        agent.orient_w = orient.w;
    }

    // --- Portal ellipse detection (GPU-authoritative)
    // Ellipse spans the arch opening: lateral = half_span, forward = depth/2.
    //
    // THE POINT'S BUBBLE: the portal is the bubble's FIRST
    // SENSOR — the probe is the body's pos THIS FRAME (agent.pos, the
    // local — byte-identical to the pre-point test; point_pos() is
    // deliberately NOT used here: it reads the storage copy, one frame
    // stale). The trigger stays on the possessed slot's wire, harvested
    // by the same P5 path.
    agent.portal_trigger = -1;
    let probe = vec3(agent.pos_x, agent.pos_y, agent.pos_z);
    for (var pi = 0u; pi < agent_room.portals.count; pi++) {
        let p = agent_room.portals.portals[pi];
        let dx = probe.x - p.x;
        let dz = probe.z - p.z;
        let lat = dx * p.facing_cos + dz * p.facing_sin;
        let fwd = -dx * p.facing_sin + dz * p.facing_cos;
        let e = lat * lat * p.inv_span_sq + fwd * fwd * p.inv_depth_sq;
        if (e < 1.0) {
            agent.portal_trigger = i32(p.arch_index);
            break;
        }
    }

    // LANDING (no-teleportation): after a dismount the body eases from the
    // saddle it left (signal.mount_from) onto the walked pose computed above.
    if (signal.mount_kind == 2u && signal.mount_phase < 1.0) {
        let e = mount_ease(signal.mount_phase);
        agent.pos_x = mix(signal.mount_from.x, agent.pos_x, e);
        agent.pos_y = mix(signal.mount_from.y, agent.pos_y, e);
        agent.pos_z = mix(signal.mount_from.z, agent.pos_z, e);
        agent.heading = mix_heading(signal.mount_from_heading, agent.heading, e);
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

    let b = agent_room.behaviors[1u];
    let tier = min(a.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let g = agent_room.tier_gains[tier];

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

    let b = agent_room.behaviors[2u];
    let tier = min(a.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let g = agent_room.tier_gains[tier];

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

    let b = agent_room.behaviors[5u];
    let tier = min(a.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let g = agent_room.tier_gains[tier];

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

    let b = agent_room.behaviors[3u];
    let tier = min(a.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let g = agent_room.tier_gains[tier];

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

    let b = agent_room.behaviors[4u];
    let tier = min(a.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let g = agent_room.tier_gains[tier];

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

// ─── Behavior: Passer — THE ROUND (ATRIUM_6) ─────────────────────
//
// A BEHAVIOUR, NOT A TRAVERSAL, and in Jean's words: they start from the
// centre of the semicircle, walk out through a door, go back around the
// semicircle, and go at it once again. One crossing per round, always
// outward, always from the centre — so a visitor standing behind the chord
// sees, over and over, exactly the act they are invited to perform.
// Nothing about the world changes when one crosses a door plane: the portal
// trigger rides the POSSESSED slot only (update_player_agent).
//
// THE DOORS are the portal entries with kind == 0, in array order, which is
// arc order (force_spawn_atrium_arc spawns ascending; the back portal, when
// one exists, takes the lowest slot and wears kind == 1).
//
// THE NORMAL, NOT THE SPAN. facing_cos/facing_sin are the arch's SPAN — the
// chord between its feet (amg_gen_shell; arch_rotation_from_facing names the
// convention CPU-side). The opening's normal is (-facing_sin, facing_cos),
// and force_spawn_atrium_arc aims it at the arc's centre, so the normal
// points INTO the room. ATRIUM_4 read the span as the normal, which put
// every waypoint beside a door rather than through it.
//
// THE CENTRE IS RECOVERED, NOT CARRIED. The doors were placed facing it, so
// the inverse recovers it and a re-placed arc is followed with no window
// where the two disagree. Each door constrains C to its own normal LINE;
// the constraint is span . (C - D) = 0, and the least-squares solution over
// ALL the doors is the intersection when they concur. Over all of them and
// not two of them for a reason worth stating: on a 180-degree span the END
// doors' normals are ANTIPARALLEL — their lines coincide, the 2x2 is exactly
// singular, and the pair a two-door derivation would pick is the one pair
// that cannot answer. Six doors over 180 degrees give det(M) ~ 8.75.
//
// THE ROUTE is A4.2's packing, unchanged, and so is the census that reads it:
//   route = (leg << 12) | (cur << 4) | (phase << 1) | 1
//   phase 0 CENTRE  waypoint = C + jitter(seed, leg)   |jitter| <= PASSER_CENTRE_JITTER
//                   arrive -> a = hash(seed, leg*2) % N; cur = a; phase 1
//   phase 1 DOOR    waypoint = outside(a) = D[a] - n_a * band
//                   arrive -> phase 2   (from C the line runs through a's opening)
//   phase 2 BAND    waypoint = outside(cur), door by door along the outside
//                   arrive -> cur == e ? phase 3 : cur += sign(e - cur)
//   phase 3 END     waypoint = end(e), a quarter past the end door on the band
//                   arrive -> leg += 1; phase 0
//   fresh (bit 0 clear): leg 0, phase 0 — the first thing a passer does is
//                   walk to the centre.
//   arrive: |waypoint - pos.xz| < step_size, the row's documented second role.
// `e` is not stored: it is a function of (seed, leg, N) like `a` is, so the
// packing did not have to grow.
//
// MOTION — A WALK, NOT A SPRING. The tether sprinted from afar and crawled
// the last metres, which is why ATRIUM_5's passers stalled short of every
// waypoint and never advanced a phase. Here the speed is CONSTANT and only
// the direction is eased:
//   desired = normalize(waypoint - pos.xz) * speed_cap * tier.speed_gain
//   vel.xz += (desired - vel.xz) * min(1, drag * dt)
// so drag is the blend rate of the turn, not a decay — agent_post_step is
// therefore called with drag 0 (its speed cap and its C2b ground steering
// still apply). home_pull is UNREAD on this arm, and so is step_rate: the
// step kick was never a gait, only noise on the velocity, and noise on a
// constant-speed walk is a shove.
const PASSER_BEHAVIOR: u32 = 10u;
const PASSER_END_MARGIN: f32 = 0.32;     // rad past the end door's bearing — clears its pier by ~R*sin(18deg)
const PASSER_CENTRE_JITTER: f32 = 4.0;   // wu — a handful of bodies do not stack on one point

struct PasserDoor { xz: vec2<f32>, normal: vec2<f32>, ok: bool }

// The d-th forward door, by a linear scan over the portal array (count <= 32).
fn passer_door(d: u32) -> PasserDoor {
    var out: PasserDoor;
    out.xz = vec2(0.0);
    out.normal = vec2(0.0, 1.0);
    out.ok = false;
    var k = 0u;
    for (var pi = 0u; pi < agent_room.portals.count; pi++) {
        let p = agent_room.portals.portals[pi];
        if (p.kind != 0u) { continue; }
        if (k == d) {
            out.xz = vec2(p.x, p.z);
            out.normal = vec2(-p.facing_sin, p.facing_cos);   // the opening, not the chord
            out.ok = true;
            return out;
        }
        k += 1u;
    }
    return out;
}

fn passer_door_count() -> u32 {
    var n = 0u;
    for (var pi = 0u; pi < agent_room.portals.count; pi++) {
        if (agent_room.portals.portals[pi].kind == 0u) { n += 1u; }
    }
    return n;
}

struct PasserArc { c: vec2<f32>, r: f32, ok: bool }

// The arc the doors were placed on, recovered from the doors themselves.
fn passer_arc() -> PasserArc {
    var out: PasserArc;
    out.c = vec2(0.0);
    out.r = 0.0;
    out.ok = false;
    var m00 = 0.0;
    var m01 = 0.0;
    var m11 = 0.0;
    var v = vec2(0.0);
    var first = vec2(0.0);
    var have_first = false;
    for (var pi = 0u; pi < agent_room.portals.count; pi++) {
        let p = agent_room.portals.portals[pi];
        if (p.kind != 0u) { continue; }
        let xz = vec2(p.x, p.z);
        // span . (C - D) = 0 — C lies on this door's normal line.
        let sp = vec2(p.facing_cos, p.facing_sin);
        let q = dot(sp, xz);
        m00 += sp.x * sp.x;
        m01 += sp.x * sp.y;
        m11 += sp.y * sp.y;
        v += sp * q;
        if (!have_first) { first = xz; have_first = true; }
    }
    let det = m00 * m11 - m01 * m01;
    if (!have_first || abs(det) < 0.0001) { return out; }   // every span parallel — no centre
    out.c = vec2(m11 * v.x - m01 * v.y, m00 * v.y - m01 * v.x) / det;
    out.r = length(first - out.c);
    out.ok = out.r > 0.001;
    return out;
}

fn passer_leg_a(seed: u32, leg: u32, n: u32) -> u32 {
    return u32(hash_property(seed, 9600u + leg * 2u) * f32(n)) % n;
}

// The end the leg returns around: the nearer one to the door it went out of.
// An exactly-central door (N odd) is a coin, and the coin is the leg's.
fn passer_leg_end(seed: u32, leg: u32, n: u32) -> u32 {
    let a = passer_leg_a(seed, leg, n);
    let half = n / 2u;
    if (a * 2u == n) {
        return select(0u, n - 1u, (u32(hash_property(seed, 9600u + leg * 2u + 1u) * 2.0) & 1u) == 1u);
    }
    return select(0u, n - 1u, a >= half);
}

// The waypoint this (leg, cur, phase) asks for, in world XZ.
fn passer_waypoint(seed: u32, leg: u32, cur: u32, phase: u32, n: u32,
                   band: f32, arc: PasserArc) -> vec2<f32> {
    if (phase == 0u) {
        let ja = hash_property(seed, 9700u + leg) * 6.28318530718;
        let jr = sqrt(hash_property(seed, 9800u + leg)) * PASSER_CENTRE_JITTER;
        return arc.c + vec2(cos(ja), sin(ja)) * jr;
    }
    if (phase == 3u) {
        // A quarter past the end door, on the outside band — the turn that
        // brings the passer back around the arc's end toward the centre.
        let e = passer_leg_end(seed, leg, n);
        let d0 = passer_door(0u);
        let d1 = passer_door(1u);
        let b0 = atan2(d0.xz.y - arc.c.y, d0.xz.x - arc.c.x);
        let b1 = atan2(d1.xz.y - arc.c.y, d1.xz.x - arc.c.x);
        // Which way the arc runs in index order — read off ADJACENT doors,
        // whose bearings differ by span/(N-1) and so cannot wrap ambiguously
        // the way the two ends can at a half turn.
        let step_sign = select(-1.0, 1.0, wrap_pi(b1 - b0) > 0.0);
        let de = passer_door(e);
        let be = atan2(de.xz.y - arc.c.y, de.xz.x - arc.c.x);
        let s = select(-step_sign, step_sign, e != 0u);
        let bo = be + s * PASSER_END_MARGIN;
        return arc.c + vec2(cos(bo), sin(bo)) * (arc.r + band);
    }
    // phases 1 and 2 — outside(d), through the opening and out.
    let d = select(cur, passer_leg_a(seed, leg, n), phase == 1u);
    let door = passer_door(d);
    if (!door.ok) { return arc.c; }
    return door.xz - door.normal * band;
}

fn behavior_passer(agent_in: AgentState) -> AgentState {
    var a = agent_in;
    let dt = signal.dt;

    let b = agent_room.behaviors[PASSER_BEHAVIOR];
    let tier = min(a.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let g = agent_room.tier_gains[tier];

    // FEWER THAN TWO DOORS IS NOT A ROUND, and neither is an arc whose
    // centre will not resolve — the doors face nowhere in particular, so
    // there is nothing to be the centre OF. The passer patrols instead,
    // which is the nearest honest thing a figure in a room can do. (A kernel
    // has no console; the [PASSER] census is where this shows, as a phase
    // that never leaves 0 and a d that wanders.)
    let n = passer_door_count();
    if (n < 2u) { return behavior_slow_patrol(a); }
    let arc = passer_arc();
    if (!arc.ok) { return behavior_slow_patrol(a); }

    let band = b.aux;
    var leg   = (a.route >> 12u) & 0xFFFFFu;
    var cur   = (a.route >> 4u) & 0xFFu;
    var phase = (a.route >> 1u) & 0x7u;

    if ((a.route & 1u) == 0u) {
        // Fresh: the first thing a passer does is walk to the centre.
        leg = 0u; phase = 0u; cur = 0u;
    } else {
        // ARRIVED? step_size is the WAYPOINT RADIUS on this row.
        let to_home = vec2(a.home_x - a.pos_x, a.home_z - a.pos_z);
        if (length(to_home) < b.step_size) {
            let e = passer_leg_end(a.seed, leg, n);
            if (phase == 0u) { cur = passer_leg_a(a.seed, leg, n); phase = 1u; }
            else if (phase == 1u) { phase = 2u; }
            else if (phase == 2u) {
                if (cur == e) { phase = 3u; }
                else if (e > cur) { cur += 1u; }
                else { cur -= 1u; }
            }
            else { leg += 1u; phase = 0u; cur = 0u; }
        }
    }
    // The waypoint IS home — one target field, and the census's `d` is
    // exactly the distance the arrival test measures.
    let wp = passer_waypoint(a.seed, leg, cur, phase, n, band, arc);
    a.home_x = wp.x;
    a.home_z = wp.y;
    a.route = (leg << 12u) | ((cur & 0xFFu) << 4u) | ((phase & 0x7u) << 1u) | 1u;

    // ── The walk: constant speed, eased direction ─────────────────
    let to = wp - vec2(a.pos_x, a.pos_z);
    let dist = length(to);
    let dir = select(vec2(0.0), to / max(dist, 0.0001), dist > 0.0001);
    let desired = dir * (b.speed_cap * g.speed_gain);
    let blend = min(1.0, b.drag * dt);
    a.vel_x += (desired.x - a.vel_x) * blend;
    a.vel_z += (desired.y - a.vel_z) * blend;

    // drag 0: the blend above IS this arm's drag, and decaying twice would
    // put the walk under its own cap. The cap and the ground steering stay.
    return agent_post_step(a, 0.0, b.speed_cap, g.speed_gain);
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

    let b = agent_room.behaviors[6u];
    let tier = min(a.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let g = agent_room.tier_gains[tier];

    // Steer toward THE POINT (the emitter), not the raw possessed slot:
    // point_pos() is the possessed body in pawn-host (identical target there)
    // and the flown camera-eye in free-fly, so pursuers track what the player
    // controls instead of clustering the idle statue (TIDY_1 T2a).
    let p = point_pos();
    let dx = p.x - a.pos_x;
    let dz = p.z - a.pos_z;
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

    let b = agent_room.behaviors[7u];
    let tier = min(a.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let g = agent_room.tier_gains[tier];

    // Flee THE POINT, not the raw possessed slot (TIDY_1 T2a; see pursuit).
    let p = point_pos();
    let dx = a.pos_x - p.x;  // away vector from the point
    let dz = a.pos_z - p.z;
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

    let b = agent_room.behaviors[8u];
    let tier = min(a.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let g = agent_room.tier_gains[tier];

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
            if (od2 < g.personal_radius * g.personal_radius && od2 > 0.001) {
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

    let b = agent_room.behaviors[9u];
    let tier = min(a.tier_idx, AGENT_TIER_COUNT_WGSL - 1u);
    let g = agent_room.tier_gains[tier];

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
//   update_other_agents  — 1D (one thread per FIELD lane, 296).
//                          Lanes 0-31 are the agent slots and also run
//                          algorithmic behaviors + eviction; the rest
//                          write their field lane and end.
//                          Skips the possessed slot internally.
// Order matters: dispatch player BEFORE other_agents so the player's
// updated position is visible to neighbor-sampling behaviors in the
// same frame.

// ─── Compute kernels ─────────────────────────────────────────────
//
// The agent kernel is split in two, and the reason is MECHANICAL, not
// a compiler's. The original unified kernel placed
// behavior_player_controlled (heavy: walker policy, step-climb, tilt,
// full contributor chain) and behavior_random_walk (light: single
// agent-policy ground snap) in a single switch statement. That shape was
// first bought at a compile-cost bench — the price is history now and it
// is on the record (docs/FXC_LAWS_RECORD.md §PROBATE).
//
// WHAT BARS RE-UNIFICATION TODAY IS TABLE E. Every ordered pair among
// update_player_agent / update_other_agents / update_sphere / update_cube
// carries a RAW hazard — agent_state between the agent kernels,
// floating_entities between the floater kernels, field_forces across the
// divide — so all twelve are BARRED. Merging two kernels deletes the
// implicit inter-dispatch barrier that makes their ordering correct, and
// WGSL has no device-wide barrier to put back. That holds on every
// backend and under every compiler; it would still hold if compile time
// were free. The split is a correctness structure now, not a budget one.
//
// Split shape:
//   update_player_agent   — 1 thread. Only the possessed slot, only
//                           behavior_player_controlled. The walker
//                           policy is compiled once, for one slot.
//   update_other_agents   — one thread per field lane. Lanes 0-31 are
//                           the non-possessed agent slots and carry the
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

// The eviction radius MUST exceed the patch allocation radius, or every
// floater committed at the streaming frontier is evicted the frame it
// spawns — silently, since per-patch spawn is idempotent and never retries.
// Allocation reaches active_radius (<= PATCH_PREGEN_RADIUS 7, OPT_1b) x
// PATCH_EXTENT 50 = 350 wu at the near edge, ~495 at the diagonal corner.
// 800 clears it with margin at the current radius. NOT DERIVED: this is a
// CPU quantity (active_radius x PATCH_EXTENT) and a GPU const in different
// rooms, so the relation cannot be asserted in either. Raising the render
// radius silently breaks this again. Queued: derive it CPU-side and upload
// through config, which puts both values in one room and makes the
// relation assertable — the feasibility corollary.
const FLOATER_EVICTION_RADIUS:    f32 = 800.0;
const FLOATER_EVICTION_RADIUS_SQ: f32 = FLOATER_EVICTION_RADIUS * FLOATER_EVICTION_RADIUS;

// POINT_BUBBLE_RADIUS — the point's bounded awareness (v3 §11; the
// bubble's first field). Sensors: the portal's vertical gate in
// camera-host, AND (CONTACT_2 C3b) the point-source flee trigger.
// REST MIRROR: the value is 20.0, now boot-pinned into
// config.point_bubble_radius from contracts/point.hpp POINT_BUBBLE_RADIUS
// (the source of truth) via set_point_bubble_radius — a runtime upload,
// no longer a compile-time const (CONTACT_2 C3a). DISCLOSE: the bubble
// is ONE thing — coupling its radius later breathes portal sensitivity
// too.

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

    // ── CONTACT_3 K1b: the player owns its own integration ─────────
    // behavior_player_controlled already integrated the voluntary
    // velocity and ground-resolved; snapshot it so the gather's IMPOSED
    // delta can land on position DIRECTLY (acts this frame, never
    // capped). This is the handoff's per-term paired pos-add unified:
    // the sum of imposed deltas * dt (linearity). Snapped next frame.
    let imp_v0 = vec2(agent.vel_x, agent.vel_z);

    // ── CONTACT (TRUEBAND_CONTACT_1): the bounded pair gather ──────────
    // Impulses land on VELOCITY only — position is already ground-
    // resolved this frame; the springs realize next frame (soft by
    // construction). flock2d's racy-neighbor read + the coded dispatch
    // order (ribbon → player → others → camera → sphere → cube → vp) are
    // the disclosed softness: one-frame asymmetries the springs absorb.
    {
        let g_self = agent_room.tier_gains[min(agent.tier_idx, 3u)];
        // (FIELD_B2: the agent↔agent presence row migrated to the
        // field — the possessed EMITS at ×PAWN_CONTACT_MASS_MULT in
        // field_sum and yields to nothing; the flee-dodge stays.)
        for (var k = 0u; k < 32u; k++) {
            if (k == slot) { continue; }
            let other = agent_state[k];
            if (other.is_active == 0u) { continue; }
            let og = agent_room.tier_gains[min(other.tier_idx, 3u)];
            let self_p = vec3(agent.pos_x, agent.pos_y, agent.pos_z);
            let other_p = vec3(other.pos_x, other.pos_y, other.pos_z);
            // CONTACT_5 P1b: body-to-body flee (APPROACH) -- skip the possessed
            // pair. self_vel reads the POST-contact velocity (the same
            // sequential dependency the inline blocks had). falloff_mix 0 = the
            // flat shell the audit found here, kept as a VISIBLE column beside
            // the point row's falloff_mix 1 (Jean's call whether it softens).
            if (k != config.possessed_slot) {
                let f_prof = row_agent_flee(g_self, og);
                let f_r = influence_response(self_p, vec2(agent.vel_x, agent.vel_z),
                                             other_p, vec2(other.vel_x, other.vel_z),
                                             f_prof, signal.dt);
                agent.vel_x += f_r.x;
                agent.vel_z += f_r.y;
            }
        }
        // ── SPHERES PUSH THE POINT (CONTACT_5 P2a) ─────────────────
        // The restore, host-agnostic. A sphere's authority is ABOVE the
        // point's: yield_share 1.0 with no mass weight IS the reversed
        // inequality — the point takes the WHOLE response and the sphere
        // takes none (CONTACT_5 authority table). PRESENCE (occupancy):
        // stand inside the shell and it keeps easing you out until you
        // clear it. Reference = the sphere's OWN influence shell
        // (fe.influence_radius, per-instance ~6-8 wu; a celestial object's
        // influence IS its reach). Applied to the point's HOST body (the
        // pawn here); the camera-host twin is in update_camera_vp. K1b: the
        // impulse lands on velocity and the inline pos-add below carries it
        // to position this frame. (Reverses CONTACT_4 S2c — Jean's ruling.)
        for (var sph = 0u; sph < SPHERE_SLOT_COUNT; sph++) {
            let fe = floating_entities.entities[sph];
            if (fe.is_active == 0u) { continue; }
            let sp_prof = row_sphere_push(fe);
            let sp_r = influence_response(
                vec3(agent.pos_x, agent.pos_y, agent.pos_z), vec2(0.0),
                fe.pos, vec2(0.0), sp_prof, signal.dt);
            agent.vel_x += sp_r.x;
            agent.vel_z += sp_r.y;
        }

        // (The occupier push moved to behavior_player_controlled's
        // CANDIDATE — BATCH G1's wire. Consuming it here as velocity
        // was a one-frame dt² nudge the walk out-paced 8:1: the next
        // frame's intent overwrite erased it, and it never met the
        // resolve. update_other_agents keeps its gather call — a free
        // agent's velocity persists, so that path was always whole.)
    }

    // CONTACT_3 K1b: imposed motion acts THIS frame — the imposed delta
    // (post-gather velocity minus the voluntary snapshot) lands on
    // position directly. Bypasses this frame's ground resolve, snapped
    // next frame; displacements are small (the pawn's 4x mass makes its
    // share of every pair tiny). Named, accepted.
    agent.pos_x += (agent.vel_x - imp_v0.x) * signal.dt;
    agent.pos_z += (agent.vel_z - imp_v0.y) * signal.dt;

    // The player is never evicted — their slot is the reference
    // frame for eviction, not subject to it.
    agent_state[slot] = agent;
}

// ─── Other-agents kernel ─────────────────────────────────────────
// One thread per FIELD lane (PANORAMA_0 RIDE_0); lanes 0-31 are the agent
// slots. Skips the possessed slot (handled by update_player_agent). Runs
// algorithmic behaviors only — the heavy walker-policy path never inlines
// here.
// ─── THE FIELD — ONE PRESENCE LAW (FIELD_2 → FIELD_B) ────────────
// THE ENDPOINT, AS LAW: not one mechanism — ONE PRESENCE LAW. The
// body arc is CLOSED (every body-class presence pair lives here);
// the point arc is OPEN and undesigned. The rulings below bind any
// edit to this block; they were paid for at the phase-B gates.
// (DURABLE HOME: a law lives where its consumers read, which for the
// presence law is this block — every body-class presence pair is
// below it, and an edit to any of them meets these rulings on the way
// past. The siting precedent was
// first set by the L2 banner, struck since — PIVOT_0,
// docs/FXC_LAWS_RECORD.md; the practice outlived the law that
// demonstrated it, which is the whole reason it is restated here in
// its own words.)
//  R1  Presence migrates; APPROACH stays behavioral — the
//      flee/dodge personalities and the boids' desire (alignment,
//      cohesion) are never field terms.
//  R2  The POINT's rows are a separate arc, undesigned:
//      row_cube_push, row_sphere_push (+camera twin),
//      row_point_flee / agents-part. Migrate-vs-permanent is
//      Jean's ruling, not an executor's.
//  R3  The possessed EMITS (×PAWN_CONTACT_MASS_MULT, to agent
//      lanes only — the B2b lane ruling) and NEVER subscribes;
//      floater←possessed stays row_cube_push's until the point
//      arc rules.
//  R4  ONE LAW, TWO TRANSPORTS: the field_forces buffer where
//      classes must hear each other; the direct field_pair call
//      where a subsystem is closed (orb separation is the
//      precedent).
//  R5  The lure (authored→ribbon) is LATERAL ONLY; the pen owns
//      altitude (B0's floor clamp makes the guarantee true).
//  R6  No anti-windup mechanism exists because no standing fy
//      source remains — build one only when a measurement asks.
//
// The shape: one pair law, one summation body — the influence-law
// shape ("one body, many callers") at field scale. field_pair is
// the quadratic shell; field_sum resolves one subscriber lane and
// walks the emitter classes under the FEEL MATRIX (phase A's
// table, amended through B4): body-class presence is all here —
// sphere→agent B1, agent↔agent B2, standing geometry B4
// (occupier_contact survives solely as the possessed candidate's
// row). Authored emitters (FIELD_4): floaters YES; agents no*
// (the point-rows own them); possessed no. Loops are flat and
// constant- or uniform-bounded; no textures; outside every
// collision/ground chain.
//
// THE SHAPE IS KEPT, THE CITATION IS NOT (PURSE_0 R5). Those three
// properties were once FXC law — rules 2 and 3 of a banner struck at
// PIVOT_0, preserved in docs/FXC_LAWS_RECORD.md, whose first line reads
// "Do not honor these as live constraints." The code is not thereby
// wrong; it is merely no longer REQUIRED to be so, and a citation to a
// dead referent is the one thing it must not keep, because a reader who
// chases it finds a rule that has been struck and cannot tell whether
// the shape is load-bearing or merely inherited. It is inherited.

// ATRIUM_7 — THE SHELL FACTOR IS A PARAMETER NOW, because one source class
// does not wear the social shell: an arch's two legs half_span apart close
// the doorway between them at slack 3.0. Every caller but the arch-leg pair
// passes config.field_slack, and field_pair is the door that says so.
fn field_pair_slack(sub_pos: vec3<f32>, emit_pos: vec3<f32>,
                    r_s: f32, r_e: f32, sub_i: u32, emit_i: u32,
                    slack: f32) -> vec3<f32> {
    let dvec = sub_pos - emit_pos;
    let shell = (r_s + r_e) * slack;
    let len = length(dvec);
    if (len >= shell) { return vec3(0.0); }
    // Degenerate overlap: deterministic axis by index parity — no
    // randomness, recordings stay reproducible.
    var dir = vec3(1.0, 0.0, 0.0);
    if (len < 1e-4) {
        if (((sub_i ^ emit_i) & 1u) == 1u) { dir = vec3(0.0, 0.0, 1.0); }
    } else {
        dir = dvec / len;
    }
    let depth = 1.0 - len / shell;
    return dir * (config.field_k * depth * depth);
}

fn field_pair(sub_pos: vec3<f32>, emit_pos: vec3<f32>,
              r_s: f32, r_e: f32, sub_i: u32, emit_i: u32) -> vec3<f32> {
    return field_pair_slack(sub_pos, emit_pos, r_s, r_e, sub_i, emit_i, config.field_slack);
}

fn field_sum(sub_i: u32) -> vec3<f32> {
    // Subscriber resolve — lane map at the FIELD consts. Radii ride
    // the CONTACT vocabulary: agents
    // agent_room.tier_gains[...].contact_radius, floaters fe.body_radius
    // (the S2c ruling: the sphere's OWN body).
    var sub_pos: vec3<f32>;
    var r_s: f32;
    if (sub_i < 32u) {
        let a = agent_state[sub_i];
        if (a.is_active == 0u || sub_i == config.possessed_slot) { return vec3(0.0); }
        sub_pos = vec3(a.pos_x, a.pos_y, a.pos_z);
        r_s = agent_room.tier_gains[min(a.tier_idx, 3u)].contact_radius;
    } else {
        let fe = floating_entities.entities[sub_i - 32u];
        if (fe.is_active == 0u) { return vec3(0.0); }
        sub_pos = fe.pos;
        r_s = fe.body_radius;
    }
    var f = vec3(0.0);
    // Agents emit — EVERY subscriber (FIELD_B2: the agent↔agent
    // presence row migrated here; the flee-dodge personality stays
    // the influence law's). Authority enters the field: each
    // emitter's term scales by its contact_mass, and the possessed
    // emits at og_mass × PAWN_CONTACT_MASS_MULT — to AGENT lanes
    // only (the FIELD_B2b lane ruling, permanent: cube←point stays
    // row_cube_push's until the point arc rules).
    // It still never yields: its subscriber lane stays rest.
    for (var k = 0u; k < 32u; k++) {
        if (sub_i < 32u && k == sub_i) { continue; }   // self
        let a = agent_state[k];
        if (a.is_active == 0u) { continue; }
        let og = agent_room.tier_gains[min(a.tier_idx, 3u)];
        var og_mass = og.contact_mass;
        if (k == config.possessed_slot) {
            og_mass = select(0.0, og_mass * PAWN_CONTACT_MASS_MULT, sub_i < 32u);
        }
        f += field_pair(sub_pos, vec3(a.pos_x, a.pos_y, a.pos_z), r_s, og.contact_radius, sub_i, k) * og_mass;
    }
    // Spheres emit — EVERY subscriber (FIELD_B1: the agent←sphere
    // presence row migrated here; point←sphere stays the influence
    // law's — row_sphere_push, the point's own reflex); skip self.
    for (var k = 0u; k < SPHERE_SLOT_COUNT; k++) {
        if (sub_i >= 32u && sub_i - 32u == k) { continue; }
        let fe = floating_entities.entities[k];
        if (fe.is_active == 0u) { continue; }
        f += field_pair(sub_pos, fe.pos, r_s, fe.body_radius, sub_i, 32u + k);
    }
    // Cubes emit — every subscriber; skip self.
    for (var k = 0u; k < CUBE_SLOT_COUNT; k++) {
        let ei = CUBE_SLOT_OFFSET + k;
        if (sub_i >= 32u && sub_i - 32u == ei) { continue; }
        let fe = floating_entities.entities[ei];
        if (fe.is_active == 0u) { continue; }
        f += field_pair(sub_pos, fe.pos, r_s, fe.body_radius, sub_i, 40u + k);
    }
    // The ribbon emits — every subscriber — as CAPSULES between the body's
    // emit samples (one ring in EMIT_STRIDE; ribbon_body wrote them this
    // pass). Liveness and the count are the ribbon's own words; the radius is
    // the tube's half-extent. 296u + i keeps the tiebreak index.
    if (field_bus.ribbon.is_visible == 1u && field_bus.ribbon.cube_count >= 2u) {
        let seg_n = (min(field_bus.ribbon.cube_count, 400u) - 1u) / RIBBON_EMIT_STRIDE;
        let eh = (ribbon_body_read.head.tick & 1u) * RIBBON_EMIT_SLOTS;   // the half the body wrote this pass
        let rr = field_bus.ribbon.cube_size * 0.5;
        for (var i = 0u; i < seg_n; i++) {
            let a = ribbon_body_read.emit[eh + i].xyz;
            let ab = ribbon_body_read.emit[eh + i + 1u].xyz - a;
            let t = saturate(dot(sub_pos - a, ab) / max(dot(ab, ab), 1e-6));
            f += field_pair(sub_pos, a + ab * t, r_s, rr, sub_i, 296u + i);
        }
    }
    // Standing geometry emits (FIELD_3; FIELD_B4a: EVERY subscriber).
    // The floater-only scope was never the design — it was the
    // anti-double-application guard, holding while agents still met
    // these bodies through occupier_contact's direct row. B4b retires
    // that row; this ungating is what must precede it, or free agents
    // would hear standing geometry from nothing at all.
    // Emitter y := subscriber y makes the pair test PLANAR —
    // row_occupier's cylindrical ruling inherited verbatim ("a column
    // is a vertical body"). Accepted percept v1: shafts are infinite
    // columns to floaters and agents alike; if altitude phantoms ever
    // read wrong, the deferral is a base_y cached at spawn (where
    // ground is queried once), not a manifold query here. True radii,
    // no skin — config.field_slack is the standoff. Flat,
    // const-bounded loops; no textures; outside every
    // collision/ground chain — inherited FXC shape, and the citation
    // that used to name it is struck (PURSE_0 R5; see field_sum's
    // banner and docs/FXC_LAWS_RECORD.md).
    var occ = vec3(0.0);
    for (var i = 0u; i < 32u; i++) {
        let cm = agent_room.occupier_cmg[i];
        if (cm.is_active == 0u) { continue; }
        occ += field_pair(sub_pos,
                          vec3(cm.center_x, sub_pos.y, cm.center_z),
                          r_s, cm.shaft_radius, sub_i, 700u + i);
    }
    for (var i = 0u; i < 16u; i++) {
        let am = agent_room.occupier_amg[i];
        if (am.is_active == 0u) { continue; }
        let leg_r = max(am.thickness, am.depth) * 0.5;
        let leg = vec2(cos(am.rotation), sin(am.rotation)) * am.half_span;
        // ATRIUM_7 — THE DOORWAY OPENS. These two are half_span apart, and
        // at the SOCIAL slack their shells meet across the opening: what
        // stands between the legs is a barrier with its crest in front of
        // the door, which is where the passers stopped. An arch leg wears
        // its own, tighter shell (config.field_arch_slack); the shaft loop
        // above and every other emitter keep the social one.
        occ += field_pair_slack(sub_pos,
                          vec3(am.center_x + leg.x, sub_pos.y, am.center_z + leg.y),
                          r_s, leg_r, sub_i, 740u + 2u * i, config.field_arch_slack);
        occ += field_pair_slack(sub_pos,
                          vec3(am.center_x - leg.x, sub_pos.y, am.center_z - leg.y),
                          r_s, leg_r, sub_i, 741u + 2u * i, config.field_arch_slack);
    }
    f += occ * config.field_occupier_gain;
    if (sub_i >= 32u) {
        // Authored emitters (FIELD_4) — floater subscribers only
        // v1 (agents keep their point-rows; possessed exempt).
        let na = min(field_bus.authored.count, 4u);
        for (var i = 0u; i < na; i++) {
            let a0 = field_bus.authored.rows[2u * i];
            let a1 = field_bus.authored.rows[2u * i + 1u];
            if (a1.z < 0.5) { continue; }
            let advec = sub_pos - a0.xyz;
            let alen = length(advec);
            if (alen >= a1.y) { continue; }
            var adir = vec3(1.0, 0.0, 0.0);
            if (alen >= 1e-4) { adir = advec / alen; }
            else if (((sub_i ^ (900u + i)) & 1u) == 1u) {
                adir = vec3(0.0, 0.0, 1.0);
            }
            let env = 1.0 - alen / a1.y;
            let e = clamp((alen - a1.x) / max(a1.x, 1.0), -1.0, 1.0);
            f += adir * (-(a0.w) * e * env * env) * config.field_authored_gain;
        }
    }
    let fl = length(f);
    if (fl > config.field_fmax) { f = f * (config.field_fmax / fl); }
    var gain = config.field_gain_agent;
    if (sub_i >= 40u) { gain = config.field_gain_cube; }
    else if (sub_i >= 32u) { gain = config.field_gain_sphere; }
    return f * gain;
}

// ONE THREAD PER FIELD LANE (PANORAMA_0 RIDE_0). This kernel used to run 32
// threads, and each of them walked NINE OR TEN field lanes in a strided loop
// (slot, slot+32, …) — every lane a field_sum over ~450 pair evaluations
// (32 agents + 8 spheres + 256 cubes + ribbon segments + 32 columns + 32 arch
// legs), each with a dependent storage load. Ten of those, serially, on one
// thread, while 264 lanes' worth of parallelism sat unused.
//
// The dispatch is now FIELD_SUBSCRIBERS wide and the strided loop is gone:
// thread `i` computes exactly one lane. The agent path below still belongs to
// lanes 0..31 — the agent slots — and it reads `field_forces[slot]`, which is
// THE SAME THREAD'S OWN WRITE. That is why no barrier is needed and none is
// added: nothing here reads a lane it did not write.
//
// The lane-coverage rule the old strided loop existed to protect is now
// structural rather than ordered. It wrote every lane BEFORE the possession
// and liveness returns so that a returned agent thread could not strand its
// sphere and cube lanes; with one thread per lane there is no shared thread to
// strand, and the returns below can only end the thread that took them.
//
// The agent-agent read race in the contact gather is unchanged and disclosed
// where it lives: it was a race between the 32 agent threads before and is the
// same race between the same 32 threads now.
@compute @workgroup_size(64)
fn update_other_agents(@builtin(global_invocation_id) gid: vec3<u32>) {
    if (!dynamics_0d_active()) { return; }

    let slot = gid.x;
    if (slot >= FIELD_SUBSCRIBERS) { return; }

    // ── THE FIELD (FIELD_2): this thread's one lane ────────────────
    // Reads positions only, which nothing writes between here and
    // agent_settle, so the read set equals an after-the-gather
    // placement. Dead/possessed subscribers write rest (vec4(0)).
    field_forces[slot] = vec4(field_sum(slot), 0.0);

    if (slot >= 32u) { return; }                     // the field lanes end here
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
        case 10u: { agent = behavior_passer(agent); }   // ATRIUM_4
        default: { /* unknown behavior — no-op */ }
    }

    // ── CONTACT (TRUEBAND_CONTACT_1): the bounded pair gather ──────────
    // Impulses land on VELOCITY only — position is already ground-
    // resolved this frame; the springs realize next frame (soft by
    // construction). flock2d's racy-neighbor read + the coded dispatch
    // order (ribbon → player → others → camera → sphere → cube → vp) are
    // the disclosed softness: one-frame asymmetries the springs absorb.
    {
        let g_self = agent_room.tier_gains[min(agent.tier_idx, 3u)];
        // (FIELD_B2: the agent↔agent presence row migrated to the
        // field — crowd spacing is field_sum's now, mass-weighted;
        // the flee-dodge personality stays here.)
        for (var k = 0u; k < 32u; k++) {
            if (k == slot) { continue; }
            let other = agent_state[k];
            if (other.is_active == 0u) { continue; }
            let og = agent_room.tier_gains[min(other.tier_idx, 3u)];
            let self_p = vec3(agent.pos_x, agent.pos_y, agent.pos_z);
            let other_p = vec3(other.pos_x, other.pos_y, other.pos_z);
            // CONTACT_5 P1b: body-to-body flee (APPROACH) -- skip the possessed
            // pair. self_vel reads the POST-contact velocity (the same
            // sequential dependency the inline blocks had). falloff_mix 0 = the
            // flat shell the audit found here, kept as a VISIBLE column beside
            // the point row's falloff_mix 1 (Jean's call whether it softens).
            if (k != config.possessed_slot) {
                let f_prof = row_agent_flee(g_self, og);
                let f_r = influence_response(self_p, vec2(agent.vel_x, agent.vel_z),
                                             other_p, vec2(other.vel_x, other.vel_z),
                                             f_prof, signal.dt);
                agent.vel_x += f_r.x;
                agent.vel_z += f_r.y;
            }
        }
        // (FIELD_B1: the agent←sphere presence row migrated to the
        // field — walkers part by field law; point←sphere stays in
        // update_player_agent, the point's own reflex.)

        // (FIELD_B4b: the occupier row migrated to the field — free
        // agents part around shafts and arch legs by field law, summed
        // in field_sum since B4a ungated it. occupier_contact lives on
        // for its OTHER consumer: the possessed pawn's candidate, which
        // never subscribes to the field — ruling 3. One law, two
        // consumers, and now only one of them is a row.)

        // ── THE POINT SOURCE (CONTACT_5 P1b): flee is the point's ──
        // Presence, not a body: agents part around the POINT within the bubble.
        // One body now -- the APPROACH profile with falloff_mix 1 (the S2a
        // proximity reflex, a VISIBLE column beside the flat body-to-body row).
        // Host-routed: pawn-host passes the pawn's velocity; camera-host feeds
        // approach_floor = BUBBLE_PART_SPEED into the point row's 9th column
        // (the isotropic fallback -- no camera velocity field yet; the deferred
        // config.point_vel_x/z retires it). point_pos() is the emitter --
        // PRESENCE FOLLOWS THE POINT.
        // ATRIUM_6 — THE PASSAGE IS IMPERTURBABLE. A passer walking its
        // round is the one thing in the entrance that must not flinch: the
        // visitor is invited to walk up to a door BY WATCHING SOMEONE DO IT,
        // and a figure that backs away as you approach teaches the opposite.
        // Only this term is off — body-to-body contact above still holds, so
        // passers part around each other and around you.
        if (agent.behavior_id != PASSER_BEHAVIOR) {
            var src_vel = vec2(0.0);
            var a_floor = BUBBLE_PART_SPEED;
            if (!point_camera_hosted()) {
                let pawn = agent_state[config.possessed_slot];
                src_vel = vec2(pawn.vel_x, pawn.vel_z);
                a_floor = 0.0;
            }
            let p_prof = row_point_flee(g_self, a_floor);
            let p_r = influence_response(
                vec3(agent.pos_x, agent.pos_y, agent.pos_z),
                vec2(agent.vel_x, agent.vel_z),
                point_pos(), src_vel, p_prof, signal.dt);
            agent.vel_x += p_r.x;
            agent.vel_z += p_r.y;
        }

        // ── THE FIELD (FIELD_2): the ride ──────────────────────────
        // Beside the gather impulses, before settle. This thread's
        // lane was summed above, so the read is same-frame fresh.
        // Per the feel matrix the agent lane carries cube and ring
        // terms only. xz only — agents are grounded.
        {
            agent.vel_x += field_forces[slot].x * signal.dt;
            agent.vel_z += field_forces[slot].z * signal.dt;
        }
    }

    // ── CONTACT_3 K1a: integrate + snap AFTER the gather ───────────
    // Order: behavior (velocity shaped) -> gather (imposed velocity) ->
    // settle (integrate + ground snap) -> evict. The speed cap governs
    // intent, not imposition — imposed motion lands the same frame and
    // is never capped. (The gather read a pre-settle pos = last frame's
    // snapped position for the 3D gate; dy changes slowly, a one-frame-
    // stale y is immaterial to a soft field — named, accepted.)
    agent = agent_settle(agent);

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

// ─── THE WORLD BOX — one spelling of the finite-bounds inset ─────
// [bmin + margin, bmax - margin] on both axes; identity outdoors.
// The vec2's .y IS THE Z AXIS — config.world_bound_min/max are (x, z),
// as the vec3 callers' `bmin.y → p.z` line already was.
//
// THE BOX IS HALF-OPEN. bmax is the EXCLUSIVE edge of the outermost
// patch: bmax = (finite_radius + 1) * PATCH_EXTENT, and patches run to
// finite_radius. floor(bmax / PATCH_EXTENT) indexes a cell that does not
// exist; sample_terrain_y_at answers 0.0 there, the walker stands on it,
// and the slope law then reads the way back as a cliff. A caller passing
// margin = 0 puts a body on that coordinate. Every caller passes a
// positive margin, and that is the law — not a coincidence.
fn world_box_clamp_xz(xz: vec2<f32>, margin: f32) -> vec2<f32> {
    var p = xz;
    let bmin = config.world_bound_min;
    let bmax = config.world_bound_max;
    let has_bounds = (bmin.x != 0.0 || bmin.y != 0.0 || bmax.x != 0.0 || bmax.y != 0.0);
    if (has_bounds) {
        p.x = clamp(p.x, bmin.x + margin, bmax.x - margin);
        p.y = clamp(p.y, bmin.y + margin, bmax.y - margin);
    }
    return p;
}

// ─── Indoor bounds resolve — walls (via the box) + ceiling ───────
// Readers: update_camera_vp, update_sphere, update_cube. The 2.0 is the
// CAMERA'S OWN margin, now stated at the call site instead of inside
// the law — which is what let the pawn pass its own.
fn indoor_bounds_resolve(pos: vec3<f32>) -> vec3<f32> {
    var p = pos;
    let cxz = world_box_clamp_xz(vec2(p.x, p.z), 2.0);  // don't let the body press into walls
    p.x = cxz.x;
    p.z = cxz.y;
    // Ceiling clamp (works for any mood with a ceiling_height > 0)
    if (config.ceiling_height > 0.0) {
        let ceiling_margin = 3.0;
        p.y = min(p.y, config.ceiling_height - ceiling_margin);
    }
    return p;
}

@compute @workgroup_size(1)
fn update_camera_vp() {
    // TWO KERNELS, ONE LANE (SPINE_2). update_camera and compute_vp were two
    // @workgroup_size(1) dispatches with a strict dependency between them —
    // the second reads the camera_state the first writes — and two of the
    // frame's floater kernels sat between them for no reason but history.
    // One entry point, one dispatch, one pipeline.
    //
    // THE LEDGER BARRED THIS PAIR, AND THE BAR WAS A FALSE POSITIVE — read
    // this before fusing anything else. Table E of the binding ledger
    // carried `update_camera -> compute_vp` on `camera_state` as **BARRED**,
    // and its stated reason is right: fusing two dispatches deletes the
    // implicit barrier WebGPU puts between them, and there is no device-wide
    // barrier to put back. But that barrier is a barrier BETWEEN THREADS.
    // Both kernels were @workgroup_size(1) dispatched 1x1x1 — one invocation
    // each — so the fused kernel is ONE INVOCATION running both bodies as
    // straight-line code, and WGSL orders a read after a write to the same
    // memory location within one invocation by program order alone. There
    // was never a second thread, so there was no barrier to delete.
    //
    // The ruling is recorded in the instrument, not only here: Table E now
    // states the single-invocation carve-out and marks such pairs EXEMPT,
    // keyed to witness W3-2's two sets. Recorded because a bar that a
    // violation ERASES — this pair left the table when the pair stopped
    // existing — is the one kind of bar a reader can never check.
    //
    // THE GUARD WRAPS THE CAMERA HALF, NOT THE PAIR, and this is the whole
    // care in the fusion. update_camera opened with
    //     if (!dynamics_0d_active()) { return; }
    // and compute_vp had no such guard: with the mute dial set, the camera
    // froze and the VP kept being rebuilt from the frozen camera. Letting
    // that `return` short-circuit the fused body would freeze the VP MATRIX
    // too — a live ORGAN dial (config.mute_dynamics_0d) that stops the world
    // rendering from its own camera. So the camera half is guarded and the
    // VP half runs unconditionally, which is exactly what the two dispatches
    // did.
    if (dynamics_0d_active()) {

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

            // ── SPHERES PUSH THE POINT (CONTACT_5 P2a, camera-host twin) ──
            // The SAME sphere-row profile, applied to the camera position (the
            // point's other host). self_vel = 0 — the PRESENCE term needs no
            // velocity, which is exactly why this works with no camera-velocity
            // field (the deferred config.point_vel_x/z is NOT needed for it). In
            // free-fly the terrain rule is NONE (the revision camera clips
            // freely), so spheres are the ONLY solid things in the point's world
            // here — a deliberate percept (Jean's ruling). influence_response
            // returns the impulse; the camera integrates it * dt to position
            // (no persistent velocity to accumulate). No later writer authors
            // camera.pos in this branch (verified P0), so it has the last word.
            for (var sph = 0u; sph < SPHERE_SLOT_COUNT; sph++) {
                let fe = floating_entities.entities[sph];
                if (fe.is_active == 0u) { continue; }
                let sp_prof = row_sphere_push(fe);
                let sp_r = influence_response(
                    camera.pos, vec2(0.0), fe.pos, vec2(0.0), sp_prof, signal.dt);
                camera.pos.x += sp_r.x * signal.dt;
                camera.pos.z += sp_r.y * signal.dt;
            }

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

        // ─── THE CHASE (RIBBON_2) ────────────────────────────────────
        // Boarding turns the camera to the flight: azimuth behind the rider,
        // looking along −dir(heading), at a standard elevation, eased over the
        // boarding. While riding, an idle mouse lets the azimuth settle back
        // behind the flight (RIBBON_CHASE_TAU; 0 = never). Derivation: the
        // orbit's forward is (−sin az, ·, −cos az) and its offset sits opposite
        // (compose_camera_position_from_orbit); flight is (−cos h, ·, −sin h);
        // equal when az = π/2 − h. The screen gates the sign (AZ_OFFSET).
        if (point_ribbon_hosted()) {
            let chase_az = 1.5707963 - ribbon_body_read.head.heading + RIBBON_CHASE_AZ_OFFSET;
            if (signal.mount_kind == 1u && signal.mount_phase < 1.0) {
                let w = 1.0 - exp(-signal.dt / RIBBON_CHASE_BOARD_TAU);
                camera.azimuth = mix_heading(camera.azimuth, chase_az, w);
                camera.elevation = mix(camera.elevation, RIBBON_CHASE_ELEVATION, w);
            } else if (RIBBON_CHASE_TAU > 0.0 && abs(signal.look_az_delta) < 1e-6) {
                let w = 1.0 - exp(-signal.dt / RIBBON_CHASE_TAU);
                camera.azimuth = mix_heading(camera.azimuth, chase_az, w);
            }
        }

        // Damped aim point — third-person orbit tracks aim_point rather than
        // the possessed body's raw position. A first-order ease trails a
        // constant-velocity target by v·tau at steady state, so tau IS the trail:
        // 4.5 wu at the pawn's 15 u/s walk (imperceptible in a third-person
        // frame), while a Caps Lock transfer over ~10 units still takes about a
        // second to settle. FPV bypasses this — first-person view requires the
        // camera to be exactly on the pawn each frame.
        //
        // ─── THE KITE LOCK (KITE_1, RIBBON host only) ────────────────
        //
        // Riding, that same trail is 12 wu — 40 wu/s × 0.30 s — and the head
        // flies out of the frame the boarding placed it in. Since the trail is
        // v·tau EXACTLY, adding v·tau back to the target cancels it identically,
        // at any dt, while transients and ring-0's wave sway still pass through
        // the ease and keep their filtering. The camera locks to the FLIGHT, not
        // to the oscillation.
        //
        // v_cmd is the head's commanded motion, recomposed here because the head
        // stores no speed: flight is −dir(heading) at throttle_eased ×
        // config.ribbon_max_speed — ribbon_head's own step, read back through the
        // window the chase above already reads — and y_vel is the pen's vertical
        // rate (realized, the spring's output toward alt_target; it is the only
        // vertical the head stores, and it is what the seat actually climbs at).
        //
        // The ramp is the SEAT's own boarding ease, read exactly as
        // behavior_player_controlled reads it. The seat is what the camera
        // chases, so a feed-forward that outran the seat's arrival would let
        // boarding lead. mount_kind is 1 or 0 in this branch — landing eases onto
        // the walked pose, by which time the point no longer rides.
        //
        // config.camera_chase_ff at 0 restores the plain trail exactly, which
        // makes the dial the proof that this is the mechanism. The PAWN path
        // takes none of it: aim_target is pawn_pos, and the walk kite is
        // byte-identical to the one it was.
        let pawn_pos = compute_pawn_pos();
        {
            let tau = 0.30;
            let alpha = 1.0 - exp(-signal.dt / tau);
            var aim_target = pawn_pos;
            if (point_ribbon_hosted()) {
                let hd = ribbon_body_read.head;
                let speed = hd.throttle_eased * config.ribbon_max_speed;
                let v_cmd = vec3(-cos(hd.heading) * speed, hd.y_vel, -sin(hd.heading) * speed);
                var mount_e = 1.0;
                if (signal.mount_kind == 1u) { mount_e = mount_ease(signal.mount_phase); }
                aim_target += v_cmd * (mount_e * config.camera_chase_ff * tau);
            }
            camera.aim_point = mix(camera.aim_point, aim_target, alpha);
        }

        if (fpv_mode_active()) {
            camera.pos = pawn_pos + vec3(0.0, config.fpv_eye_height, 0.0);
        } else if (coupling_active(COUPLING_PAWN_TO_CAMERA_TARGET)) {
            camera.pos = coupling_pawn_to_camera_target(camera.aim_point, camera);
        }

        // ─── Camera terrain clamp: never go underground ──────────────
        //
        // THE AURA IS A FLOOR EXACTLY AS TERRAIN IS (Jean's ruling): the eye
        // never passes under the visual skin, and the skin includes the pawn's
        // aura dome. POLICY_WALKER_WITNESS is the surface that says so — the
        // shared world stack (static base + pyramids + GoL zones + terrain
        // waves + radial pulses) plus the EXTERNAL aura form, the same grid
        // sample patch_terrain_vs extrudes the ground by, and the eye's own
        // GoL suppression under the same height fade the render carve applies.
        // Floor and picture are then the same surface, term for term.
        //
        // The query is at the COMPOSED EYE's xz (camera.pos, already the orbit
        // eye at this point), and consumer_pos is the eye too — the policy
        // reads it for the suppression center and for the fade's height.
        //
        // The eye still CLIMBS a zone's lift as the pawn does: the suppression
        // is the witness's own, so it flattens what is under the lens and
        // leaves the rest of the field standing to lift the camera over.
        //
        // Free-fly is unaffected — the camera host returns above this point,
        // which IS its TERRAIN RULE = NONE (contracts/point.hpp).
        {
            let min_clearance = 1.5;  // minimum height above the visual skin
            let qi = QueryInputs(camera.pos, signal.t_seconds);  // the eye is the consumer
            let ground_at_cam = manifold_position(camera.pos, POLICY_WALKER_WITNESS, qi).y;
            camera.pos.y = max(camera.pos.y, ground_at_cam + min_clearance);
        }

        // ─── Indoor boundary clamp: stay within walls and below ceiling ──
        // (extracted to indoor_bounds_resolve — the one law, behavior-identical)
        camera.pos = indoor_bounds_resolve(camera.pos);

        camera_state = camera;
    }

    // Build VP matrix from camera state (already updated by update_world)
    vp_data.m = build_view_projection_matrix(
        camera_state.pos,
        camera_state.azimuth,
        camera_state.elevation,
        signal.aspect_ratio
    );

    // Sun VP: kite coupling — the sun orbits THE POINT at fixed
    // offset (was the pawn; the shadow box must cover
    // what the eye sees, so it follows the point's host — identical
    // when the pawn hosts, tracks the camera in free-fly).
    if (coupling_active(COUPLING_PAWN_TO_SUN_VP)) {
        vp_data.light_vp = coupling_pawn_to_sun_vp(
            point_pos(),
            config.sun_direction
        );
    }
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
            let prev_y = fe.pos.y;   // the walking value's present (last frame's settled y)
            fe.pos = updated.pos;
            fe.orientation = updated.orientation;

            // ── THE FIELD (FIELD_2): the sphere yields, then returns ──
            // The reserved substrate wakes — the cube's integrator,
            // spring-to-zero keeping the orbit the attractor (spawn
            // seeds spring/drag, bodies/spheres.hpp). Drift joins the
            // composed position HERE — after orbit compose, before the
            // floor ease and walls: resolve stays last (H4 untouched).
            let ff = field_forces[32u + slot].xyz;  // lane 32 + sphere slot: the sphere band
            let sph_spring_a = -fe.drift * fe.spring_stiffness;
            fe.drift_vel = fe.drift_vel + (sph_spring_a + ff) * dt;
            fe.drift_vel = fe.drift_vel * exp(-fe.drag * dt);
            fe.drift = fe.drift + fe.drift_vel * dt;
            fe.pos = fe.pos + fe.drift;

            // RESIDUE_2 [3b]: the sphere rides the live ground. The goal
            // may leap (authored orbit y, live floor + clearance); the
            // value walks — continuity law, no snap. The sphere lifts
            // over live cells and settles back beyond the zone. The
            // existing flyer evaluator does the fetch (raw GoL, no
            // self-suppression); ease 4.0 is a starting stamp — Jean's
            // dial at the gate.
            let authored_orbit_y = fe.pos.y;
            let floor_qi = QueryInputs(fe.pos, signal.t_seconds);
            let floor_y = manifold_position(fe.pos, POLICY_FLYER, floor_qi).y;
            let y_target = max(authored_orbit_y, floor_y + SPHERE_CLEARANCE);
            fe.pos.y = prev_y + (y_target - prev_y) * (1.0 - exp(-4.0 * dt));

            // RESIDUE_2 [2]: runtime walls — the sphere reads the indoor
            // bounds through the one law (margins v1 are the camera's
            // own). An orbit that crosses a wall slides along it —
            // accepted percept v1. Identity outdoors.
            fe.pos = indoor_bounds_resolve(fe.pos);

            floating_entities.entities[slot] = fe;
        }
    }
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

// ─ Force: the Witness's presence (KITE_1) ────────────────────────
// THE EYE REPELS FLOATERS AS IT PASSES. Radial from the composed eye, on
// the pawn forcefield's falloff shape read the other way round — 1 inside
// the shell, 0 outside, soft over PAWN_FORCEFIELD_FALLOFF at the rim. It
// is not a behavior: it is the room the behaviors happen in, so it is
// added to every cube whatever its behavior_id.
//
// RADIAL IN THREE DIMENSIONS, not planar. The pawn's own shove is a
// column because a walking body reaches a hovering cube only as an
// occupancy underneath it; the eye is not on the ground, and a cube
// directly above the lens is exactly the one that should shed. So the
// distance and the direction are both the full vector.
//
// camera_state.pos DIRECTLY, never point_pos(): the point is the pawn in
// every host but free-fly, and this term is the WITNESS's. Same gate as
// the render carve — off when the camera hosts the point, because the
// revision camera is a ghost and emanates nothing (contracts/point.hpp:
// presence follows the point, emanation stays the body's).
//
// AN ACCELERATION, not an impulse. It joins behavior_force inside
// update_cube's (spring_a + behavior_force + ff) * dt, so the spring and
// the drag bound it exactly as they bound CurlField's 12.0 and
// PhaseWave's 30.0. At the rest gain the steady shed is gain /
// spring_stiffness = 12.5 / 4 ≈ 3 wu — the same visible displacement
// CurlField already produces.
//
// SPHERES TAKE NONE OF IT, by ruling. They subscribe to no behavior force
// at all: update_sphere composes its orbit from motors and adds only the
// field and its own spring. It is the sphere that EMITS the point's push
// (row_sphere_push, read from update_camera_vp's free-fly branch), never a
// subscriber to one. Perturbing a motor is the complicated dynamics the
// ruling excludes.
fn cube_force_witness(fe: FloatingEntityState) -> vec3<f32> {
    if (point_camera_hosted()) { return vec3<f32>(0.0); }
    let r = config.camera_push_radius;
    if (r <= 0.0) { return vec3<f32>(0.0); }
    let away = fe.pos - camera_state.pos;
    let d = length(away);
    if (d < 1e-4) { return vec3<f32>(0.0); }   // degenerate: the lens is inside the body
    let w = 1.0 - smoothstep(r - PAWN_FORCEFIELD_FALLOFF,
                             r + PAWN_FORCEFIELD_FALLOFF, d);
    return (away / d) * (config.camera_push_gain * w);
}

// ─ Dispatch ──────────────────────────────────────────────────────
// Switch by behavior_id. New behaviors land here as additional cases
// alongside their authoring registry rows in bodies/cube_behaviors.hpp.
// The witness's presence is added AFTER the switch, to every cube: it is
// the room, not a behavior, so no behavior_id can opt out of it.
fn cube_behavior_force(fe: FloatingEntityState, t: f32, point_xz: vec2<f32>, coordination: f32) -> vec3<f32> {
    let rest_xz = vec2<f32>(fe.anchor.x, fe.anchor.z);
    var f = vec3<f32>(0.0);
    switch (fe.behavior_id) {
        case 1u: { f = cube_force_curlfield(rest_xz, t, coordination); }
        case 2u: { f = cube_force_phasewave(rest_xz, t, fe.behavior_phase, coordination); }
        default: { f = cube_force_stationary(); }
    }
    return f + cube_force_witness(fe);
}

// ONE LANE PER CUBE (PANORAMA_0 RIDE_0). This kernel was @workgroup_size(1)
// dispatched (1,1,1), walking `for slot in CUBE_SLOT_OFFSET..+256` on a single
// GPU lane — two manifold_position queries per cube (a heightfield fetch
// through the patch grid plus the pyramid/zone/pulse overlay loops), the
// behavior force, the push response and the drift integrator, 256 times,
// serially. It was the `dispatch_compute` mean and all of its spikes.
//
// THE LOOP WAS ALWAYS EMBARRASSINGLY PARALLEL, which is what makes this a
// mapping change and not a design one. Verified rather than assumed: every
// write in the body is to `floating_entities.entities[slot]` — its own slot,
// both the eviction store and the final write-back — and the only reads that
// leave the slot are `point_pos()` (a uniform) and `field_forces[32u + slot]`,
// the cube's OWN lane, written by update_other_agents in an EARLIER dispatch
// of this same pass. No iteration reads another iteration's slot, so the
// arithmetic and the results are identical lane for lane.
//
// GPUState::cube_workgroups() rounds up: 4 x 64 = 256 = CUBE_SLOT_COUNT.
@compute @workgroup_size(64)
fn update_cube(@builtin(global_invocation_id) gid: vec3<u32>) {
    if (!dynamics_0d_active()) { return; }

    if (gid.x >= CUBE_SLOT_COUNT) { return; }
    let slot = CUBE_SLOT_OFFSET + gid.x;

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
    {
        var fe = floating_entities.entities[slot];
        if (fe.is_active == 0u) { return; }

        // Lifecycle: point-distance eviction (was the pawn —
        // floaters follow the point). Cube stays alive as long as its
        // current position (home + drift) is within range of THE
        // POINT. Patch eviction no longer touches cubes — see the
        // matching test in update_sphere for the lifecycle rationale.
        let to_point = fe.pos.xz - point_xz;
        if (dot(to_point, to_point) > FLOATER_EVICTION_RADIUS_SQ) {
            floating_entities.entities[slot].is_active = 0u;
            return;
        }

        if (!sphere_frozen()) {
            fe.t = fe.t + dt;

            // ── Mode-switch sentinels (the anchor law) ────────────
            // Goals may leap; values may only walk; the walk starts
            // from the true present — which only this kernel knows
            // (pos.xz = home.xz + drift.xz, and drift lives only on
            // GPU; a CPU capture would need a readback — one-frame
            // latency, race-prone — or a drift estimate, imperfect:
            // CurlField drifts cubes by ~3 wu, a visible jump).
            //
            // follow_pawn == 2u — kite-RELEASE: freeze the cube's
            // CURRENT world xz as the new anchor, cancel any in-flight
            // glide (target := the captured anchor), switch to anchor
            // mode. xz is preserved bit-exactly (anchor := pos, drift
            // .xz zeroed, so pos.xz = home.xz), and Y WALKS HOME: only
            // the xz components of drift and drift_vel are cleared,
            // and the existing spring/drag settle the vertical the way
            // every other displacement settles. Zeroing drift.y here
            // was a ~10 wu snap under PhaseWave (a vertical force);
            // goals may leap, values may only walk — the same law the
            // xz side already obeys. Bit-neutral whenever drift.y is
            // zero, which is every planar behavior.
            //
            // follow_pawn == 3u — kite-CAPTURE: the offset is taken
            // from the true present WITH drift subtracted, so
            // home.xz + drift.xz reconstructs pos.xz — algebraically
            // exact, a few f32 ULPs in practice — even mid-shove.
            // target := the captured offset cancels any in-flight
            // glide (stated and deliberate: a mode switch retargets
            // to the present). drift is untouched — it carries
            // across the switch.
            //
            // After either sentinel fires, the rest of update_cube
            // runs in the new mode this frame, on consistent state.
            if (fe.follow_pawn == 2u) {
                fe.anchor = vec3(fe.pos.x, 0.0, fe.pos.z);
                fe.target_x = fe.pos.x;
                fe.target_z = fe.pos.z;
                fe.drift.x = 0.0;
                fe.drift.z = 0.0;
                fe.drift_vel.x = 0.0;
                fe.drift_vel.z = 0.0;
                fe.follow_pawn = 0u;
            }
            if (fe.follow_pawn == 3u) {
                let off = fe.pos.xz - point_xz - fe.drift.xz;
                fe.pawn_offset = vec3(off.x, 0.0, off.y);
                fe.target_x = off.x;
                fe.target_z = off.y;
                fe.follow_pawn = 1u;
            }

            // ── THE WALK (the anchor law) ─────────────────────────
            // The one control law, many authors: the CPU corral (and
            // later the music couplings) write TARGETS through the
            // target door; this walk is the only mover of the param.
            // Exponential approach — no per-cube clocks, no
            // from-fields; a retarget mid-flight walks from the
            // present by construction. At rest target == param
            // (spawn init + both sentinels above), the delta is zero,
            // and the walk moves nothing: rest is identity,
            // structurally, not by tuning.
            let glide_k = 1.0 - exp(-dt / CUBE_GLIDE_TAU);
            if (fe.follow_pawn == 1u) {
                fe.pawn_offset.x += (fe.target_x - fe.pawn_offset.x) * glide_k;
                fe.pawn_offset.z += (fe.target_z - fe.pawn_offset.z) * glide_k;
            } else {
                fe.anchor.x += (fe.target_x - fe.anchor.x) * glide_k;
                fe.anchor.z += (fe.target_z - fe.anchor.z) * glide_k;
            }

            // ── Analytical home ───────────────────────────────────
            // Two modes:
            //   follow_pawn = 0 (default): home.xz = anchor.xz; home.y
            //     terrain-relative at home.xz.
            //   follow_pawn = 1 (kite mode): home.xz = POINT.xz +
            //     offset (was the pawn — the kite target is the
            //     point, Jean's ruling; the offset is captured
            //     IN-KERNEL by the sentinel-3 block above, from the
            //     true present). home.y still terrain-relative at
            //     home.xz.
            //
            // Y is *always* terrain-relative in both modes, so cubes
            // feel like balloons leashed to the point — they float at
            // orbit_height above whatever terrain they are over,
            // regardless of the point's current altitude. Under ruling
            // 1 that now holds in BOTH modes; before it, the anchor arm
            // read its BIRTHPLACE's ground and the claim was kite-only.
            //
            // F7 toggle, what is preserved and what is not. pos.xz is
            // continuous BOTH ways, even under drift — the sentinels
            // capture from the true present (release: anchor := pos;
            // capture: offset := pos − point − drift). home.y is
            // continuous only when drift.xz is zero: the anchor arm
            // queries ground at pos.xz (ruling 1) while the kite arm
            // still queries at kite_xz. A cube toggled mid-drift
            // across a slope therefore steps vertically by the ground
            // difference over drift.xz — zero for a cube at rest,
            // visible on a pyramid face under CurlField's ~3 wu.
            // Closing it is ruling 1 applied to the kite arm as well
            // (kite_xz -> pos.xz); that is ANCHOR_2, not this edit.
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
                // RULING 1 (anchor): clearance is a PER-FRAME evaluation, so
                // it evaluates where the body IS — the live xz — not where it
                // was born.  home.xz stays anchor.xz: the spring's rest point
                // is a force-law constant, not a reading of the world, so the
                // cube stays leashed while its clearance follows it.
                let live_xz = fe.pos.xz;
                let qi = QueryInputs(vec3(live_xz.x, 0.0, live_xz.y), signal.t_seconds);
                let ground_a = manifold_position(vec3(live_xz.x, 0.0, live_xz.y), POLICY_FLYER, qi).y;
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

            // ── THE POINT SOURCE push (CONTACT_5 P2b): cubes are SHOVED ──
            // The point's PRESENCE, not its approach: stand under a floating
            // cube and it moves ahead of you until you step out of the column
            // (OCCUPANCY, not motion -- the beach-ball Jean asked for). The
            // gate is an INFINITE CYLINDER (planar CUBE_PUSH_RADIUS, no vertical
            // window), admitted by a REACH test on the cube's authored altitude
            // (row_cube_push): a hovering body's shell is the column beneath
            // it, and only in-reach cubes have a column at all.
            // Radial (tangential 0), falloff_mix 1 (soft at the rim). There is
            // no cube-vs-pawn body-contact row and there cannot be one: a
            // contact shell never reaches a hovering cube (4.6 < H). The
            // presence column is what reaches it.
            // Persistence: config.cube_plasticity is 1.0
            // (Idle::CUBE_PLASTICITY_DEFAULT) so a shove RELOCATES rather than
            // partially returns -- lambda=1 semantics (the leak below).
            //
            // The push is an IMPULSE (PRESENCE is dt-scaled inside -- the K1
            // law), added STRAIGHT to drift_vel exactly as the agent contact
            // adds to velocity; spring_a + behavior_force stay accelerations
            // (x dt). (P1b passed dt=1.0 to bit-preserve the OLD force-parting;
            // P2b's presence push is the impulse K1 names, so it takes signal.dt
            // and a DIRECT add -- now the soft falloff actually bites, and
            // CUBE_PUSH_CAP is a velocity-delta safety, ~never reached at the
            // ~0.95 wu/s typical.) Worked: a small cube (orbit_height ~25 <=
            // ceiling 30 -> in reach), pawn 3 wu planar beneath -> planar
            // overlap 4, fall = 1-3/7 = 0.57 -> impulse = 4*25*(1/60)*0.57
            // ~= 0.95 wu/s of drift velocity per frame, accumulating while you
            // stay under the column, STOPPING the instant you step out (or the
            // cube rises past the ceiling); standing still inside still pushes.
            var push_impulse = vec3(0.0);
            {
                let q_prof = row_cube_push(fe);
                let q_r = influence_response(
                    fe.pos, vec2(0.0), point_pos(), vec2(0.0), q_prof, dt);
                push_impulse = vec3(q_r.x, 0.0, q_r.y);
            }
            let spring_a = -fe.drift * fe.spring_stiffness;
            let ff = field_forces[32u + slot].xyz;  // lane 40 + (slot − CUBE_SLOT_OFFSET): the cube band of the index map
            fe.drift_vel = fe.drift_vel + (spring_a + behavior_force + ff) * dt + push_impulse;
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
            // Since ruling 1 this is a FLOOR, no longer the kernel's
            // only live-xz reader: home.y already tracks local ground,
            // so min_drift_y goes deeply negative and the clamp rests
            // slack in the ordinary case. It still earns its keep — it
            // covers the one frame of lag in home.y (which reads LAST
            // frame's pos.xz, the integrator running below the home
            // block), it covers the kite arm, and it is the hard floor
            // no drift may cross.
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

            // RESIDUE_2 [2]: runtime walls — the cube reads the indoor
            // bounds through the one law (margins v1 are the camera's
            // own). Drift that presses into a wall slides along it;
            // home + drift stay untouched, so the clamp re-resolves
            // each frame. Identity outdoors.
            fe.pos = indoor_bounds_resolve(fe.pos);

            // ── PLASTICITY (CONTACT_2 C1b) ─────────────────────────
            // Displacement leaks from drift (temporary) into anchor
            // (permanent). fe.pos is already composed above, so the
            // transfer moves no pixels this instant — the present
            // becomes the next configuration's initial condition (the
            // continuity law). λ = 0 reproduces the elastic cube
            // bit-exactly. Three refinements make the stated law TRUE on
            // this code (all bit-neutral at λ=0): (1) placed AFTER the
            // compose, since `home` is a local from the PRE-leak anchor
            // — leaking before it would jump pos this frame; (2) XZ-only
            // — home.y is terrain-relative (not anchor.y), so a .y leak
            // would move pixels (contact is planar anyway, C1a); (3)
            // anchor mode only — in kite mode home tracks the point, not
            // anchor, so the anchor is dormant. (GPU anchor mutation
            // precedent: the kite-release freeze.)
            if (fe.follow_pawn == 0u) {
                let lam = fe.plasticity * config.cube_plasticity;   // K2c: per-tier character x live master
                let leak = clamp(lam * dt, 0.0, 1.0);
                fe.anchor.x = fe.anchor.x + fe.drift.x * leak;
                fe.anchor.z = fe.anchor.z + fe.drift.z * leak;
                // The leak is an anchor AUTHOR, so it moves target and
                // anchor as a PAIR (ONE_ANCHOR_1): target − anchor is
                // invariant under it, so a resting cube stays resting
                // (the walk above sees delta 0) and a shove RELOCATES
                // instead of being walked back to a stale goal. A
                // mid-glide shove translates the remaining glide.
                // λ = 0 remains bit-exact.
                fe.target_x = fe.target_x + fe.drift.x * leak;
                fe.target_z = fe.target_z + fe.drift.z * leak;
                fe.drift.x = fe.drift.x - fe.drift.x * leak;
                fe.drift.z = fe.drift.z - fe.drift.z * leak;
            }

            // Spin around tilted Y axis (unchanged)
            let spin_angle = fe.t * fe.spin_speed;
            let axis = normalize(vec3(fe.spin_tilt_x, 1.0, fe.spin_tilt_z));
            let half_a = spin_angle * 0.5;
            fe.orientation = vec4(axis * sin(half_a), cos(half_a));

            floating_entities.entities[slot] = fe;
        }
    }
}


// --- The patch bake (one fused pass, one batched dispatch) ------------
//
// LATTICE_1. This was TWO kernels and a 512 KB scratch buffer: pass 1
// evaluated the terrain per texel and wrote heights to storage; pass 2 read
// five neighbours back and differenced them. That shape existed to pay the
// terrain evaluation once per texel — which mattered when there were 65,536
// texels per patch and the stencil neighbours were texels.
//
// At the lattice there are 4,225, and the stencil is a WORLD offset
// (BAKE_STENCIL_EPS), not a neighbouring texel — so the neighbours a texel
// needs are not values any other texel computed. There is nothing to share,
// the scratch buffer has nothing to carry, and the pass boundary has nothing
// to order. One kernel, five evaluations, one store.
//
// THE STENCIL IS CENTRAL AT THE PATCH EDGE TOO, which the old pass 2 could
// not manage: it read neighbours out of a per-patch scratch buffer, so at a
// patch border it fell back to a one-sided 3-point difference — on BOTH sides
// of every seam, from two different one-sided stencils. ground_formed_with
// _complexity is world-continuous and takes a world position, so the five
// taps here cross the border like any other point. Patch-border normals
// become continuous across neighbours. A consequence, not a goal.
//
// ─── THE NODE TABLE (LATTICE_1b) ────────────────────────────────────
//
// DERIVE EACH NODE ONCE PER TILE, not once per (tap, node). A 16x16
// lattice tile spans 12.11 wu of world; across the six bands it touches at
// most 77 distinct nodes. Its 256 threads take 5 taps each, and each tap
// walks a 2x2 per band — 30,720 derivations of those 77 nodes, every one
// of them ~11 hash_property calls, three Box-Muller draws and two angle
// transcendentals. The table is 400x less derivation for the same values.
//
// THE SPAN, and why the counts are what they are. The tile's 16 lattice
// points are 15 steps of PATCH_EXTENT / PATCH_MESH_N, and the stencil
// reaches BAKE_STENCIL_EPS past each end:
//   15 * (50/64) + 2 * (50/255) = 11.719 + 0.392 = 12.111 wu.
// For spacing s the walk reads node indices floor(X0/s) .. floor(X1/s)+1,
// and floor(X1/s) - floor(X0/s) <= floor(L/s) + 1, so floor(L/s) + 3 nodes
// per axis always suffice. The const_asserts below state exactly that
// inequality against TERRAIN_BANDS — the table cannot fall out of step with
// the band spacings without the shader refusing to compile.
const BAKE_TILE: u32 = 16u;
const BAKE_NODES_0: u32 = 3u;   // spacing 200 — 12.111/200 = 0.06  -> 0 + 3
const BAKE_NODES_1: u32 = 3u;   // spacing  80 — 0.151            -> 0 + 3
const BAKE_NODES_2: u32 = 3u;   // spacing  30 — 0.404            -> 0 + 3
const BAKE_NODES_3: u32 = 4u;   // spacing  12 — 1.009            -> 1 + 3
const BAKE_NODES_4: u32 = 5u;   // spacing   5 — 2.422            -> 2 + 3
const BAKE_NODES_5: u32 = 3u;   // spacing 500 — 0.024            -> 0 + 3

// Prefix sums of n^2 — band b's rectangle occupies [OFF[b], OFF[b+1]).
const BAKE_TABLE_OFF = array<u32, 7>(0u, 9u, 18u, 27u, 43u, 68u, 77u);
const BAKE_TABLE_N: u32 = 77u;
const_assert BAKE_TABLE_OFF[6] == BAKE_TABLE_N;

const BAKE_TILE_SPAN: f32 =
    f32(BAKE_TILE - 1u) * PATCH_EXTENT / f32(PATCH_MESH_N) + 2.0 * BAKE_STENCIL_EPS;
const_assert floor(BAKE_TILE_SPAN / TERRAIN_BANDS[0].spacing) + 3.0 <= f32(BAKE_NODES_0);
const_assert floor(BAKE_TILE_SPAN / TERRAIN_BANDS[1].spacing) + 3.0 <= f32(BAKE_NODES_1);
const_assert floor(BAKE_TILE_SPAN / TERRAIN_BANDS[2].spacing) + 3.0 <= f32(BAKE_NODES_2);
const_assert floor(BAKE_TILE_SPAN / TERRAIN_BANDS[3].spacing) + 3.0 <= f32(BAKE_NODES_3);
const_assert floor(BAKE_TILE_SPAN / TERRAIN_BANDS[4].spacing) + 3.0 <= f32(BAKE_NODES_4);
const_assert floor(BAKE_TILE_SPAN / TERRAIN_BANDS[5].spacing) + 3.0 <= f32(BAKE_NODES_5);

// THE ARITHMETIC ABOVE IS THE BOUNDS GUARD: `rel` in
// bake_ground_at is non-negative and below the band's count BY THAT
// INEQUALITY, never by a clamp. WGSL robustness would clamp a wrong index
// silently and hand back a neighbour's wave, so a wrong table would read
// as a subtly wrong world rather than as a fault. If a band spacing moves,
// the const_assert fires before the shader ever runs.
var<workgroup> bake_nodes: array<WaveNode, BAKE_TABLE_N>;
var<workgroup> bake_origin: array<vec2<i32>, TERRAIN_BAND_COUNT>;   // per-band min node index

// THE WORKGROUP-STORAGE NEED (LATTICE_2 R5). Stated once, checked here
// against the core default; the schema's NEEDS row quotes this constant.
// 48 = the WGSL size of WaveNode (4 f32 + vec2 + vec2 + 4 u32, 8-aligned);
// 8 = vec2<i32>. This is the program's LARGEST per-entry-point sum — the
// only other var<workgroup> in the module is the card resolve's
// `sh_card_h`, 1,600 B in its own entry point.
const BAKE_WORKGROUP_BYTES: u32 = BAKE_TABLE_OFF[6] * 48u + TERRAIN_BAND_COUNT * 8u;   // 3,744
const_assert BAKE_WORKGROUP_BYTES <= 16384u;   // maxComputeWorkgroupStorageSize, core default

// Comparisons only — the blessed shape (no loops, no tables).
fn bake_band_of(k: u32) -> u32 {
    if (k < BAKE_TABLE_OFF[1]) { return 0u; }
    if (k < BAKE_TABLE_OFF[2]) { return 1u; }
    if (k < BAKE_TABLE_OFF[3]) { return 2u; }
    if (k < BAKE_TABLE_OFF[4]) { return 3u; }
    if (k < BAKE_TABLE_OFF[5]) { return 4u; }
    return 5u;
}

fn bake_nodes_n(b: u32) -> u32 {
    switch (b) {
        case 0u: { return BAKE_NODES_0; }
        case 1u: { return BAKE_NODES_1; }
        case 2u: { return BAKE_NODES_2; }
        case 3u: { return BAKE_NODES_3; }
        case 4u: { return BAKE_NODES_4; }
        default: { return BAKE_NODES_5; }
    }
}

// ground_formed_with_complexity's height, read out of the table.
//
// SAME band order, SAME node order (dz outer, dx inner), SAME Hermite
// weights, SAME per-band partial sum, SAME tile modifiers and pyramids —
// so this is the analytic function's value, not an approximation of it.
// Two things are dropped and both are provably inert at t = 0, which is
// the only time the bake evaluates at:
//   · the activity field. beat_freq enters only phase_moving, scaled by
//     t_beats; at t = 0 the moving wave IS the frozen wave, bit for bit.
//   · the mix. evaluate_lattice_wave returns mix(x, x, band_act) once the
//     two are equal, which is x to within one ulp — the one departure
//     (mix is x*(1-a) + x*a, not a no-op in IEEE).
//   · complexity, which had no reader once the .w channel went to 0.
fn bake_ground_at(world_xz: vec2<f32>) -> f32 {
    var height = 0.0;
    for (var b: u32 = 0u; b < TERRAIN_BAND_COUNT; b++) {
        let band = TERRAIN_BANDS[b];
        let n = bake_nodes_n(b);
        let off = BAKE_TABLE_OFF[b];
        let origin = bake_origin[b];

        let lattice_pos = world_xz / band.spacing;
        let lattice_base = vec2<i32>(floor(lattice_pos));
        let frac = fract(lattice_pos);
        let w = frac * frac * (3.0 - 2.0 * frac);

        var band_h = 0.0;
        for (var dz: i32 = 0; dz <= 1; dz++) {
            for (var dx: i32 = 0; dx <= 1; dx++) {
                let node = lattice_base + vec2<i32>(dx, dz);

                let wx = select(1.0 - w.x, w.x, dx == 1);
                let wz = select(1.0 - w.y, w.y, dz == 1);
                let weight = wx * wz;

                let rel = node - origin;
                let wn = bake_nodes[off + u32(rel.y) * n + u32(rel.x)];
                band_h += eval_wave_node(world_xz, wn, 0.0) * weight;
            }
        }
        height += band_h;
    }
    let mods = tile_modifiers_at(world_xz);
    return height * mods.x + mods.y + contrib_pyramids_at(world_xz);
}

// workgroup_id.z selects the patch: one dispatch bakes the whole batch.
@compute @workgroup_size(16, 16)
fn bake_patch_heightfield(
    @builtin(local_invocation_id) lid: vec3<u32>,
    @builtin(workgroup_id) wid: vec3<u32>
) {
    let pp = patch_params_batch[wid.z];
    let t = lid.y * BAKE_TILE + lid.x;

    // The tile's minimum world corner, stencil reach included — the point
    // every band's origin index is floored from.
    let tile_min = pp.origin
        + (vec2(f32(wid.x * BAKE_TILE), f32(wid.y * BAKE_TILE)) / f32(PATCH_MESH_N)
           - vec2(0.5)) * PATCH_EXTENT
        - vec2(BAKE_STENCIL_EPS);

    if (t < TERRAIN_BAND_COUNT) {
        bake_origin[t] = vec2<i32>(floor(tile_min / TERRAIN_BANDS[t].spacing));
    }
    workgroupBarrier();

    // 256 threads fill 77 nodes by stride. EVERY thread runs this and both
    // barriers — the out-of-range bounds check waits until after them.
    for (var k = t; k < BAKE_TABLE_N; k += BAKE_TILE * BAKE_TILE) {
        let b = bake_band_of(k);
        let n = bake_nodes_n(b);
        let r = k - BAKE_TABLE_OFF[b];
        let node = bake_origin[b] + vec2<i32>(i32(r % n), i32(r / n));
        bake_nodes[k] = derive_wave_node(node, lattice_node_seed(config.world_seed, node, b),
                                         TERRAIN_BANDS[b]);
    }
    workgroupBarrier();

    let ix = wid.x * BAKE_TILE + lid.x;
    let iz = wid.y * BAKE_TILE + lid.y;
    if (ix >= PATCH_HEIGHTFIELD_N || iz >= PATCH_HEIGHTFIELD_N) { return; }

    // texel i IS lattice point i: the same uv patch_terrain_vs decodes.
    let uv = vec2(f32(ix), f32(iz)) / f32(PATCH_MESH_N);
    let p  = pp.origin + (uv - vec2(0.5)) * PATCH_EXTENT;

    let e   = BAKE_STENCIL_EPS;
    let h0  = bake_ground_at(p);
    let hx  = bake_ground_at(p + vec2(e, 0.0));
    let hmx = bake_ground_at(p - vec2(e, 0.0));
    let hz  = bake_ground_at(p + vec2(0.0, e));
    let hmz = bake_ground_at(p - vec2(0.0, e));
    let grad_x = (hx - hmx) / (2.0 * e);
    let grad_z = (hz - hmz) / (2.0 * e);

    // The .w channel is unused — stored 0.0. Palette calls read the
    // pinned PALETTE_COMPLEXITY (TERRAIN_LOOKS ROW 3) instead.
    textureStore(patch_heightfield_array_write, vec2<i32>(i32(ix), i32(iz)),
                 i32(pp.layer), vec4(h0, grad_x, grad_z, 0.0));
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
    let zone_seed = lattice_node_seed(config.world_seed, zone_node, GOL_ZONE_SEED_BAND);

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
fn generate_patch_cells(@builtin(global_invocation_id) id: vec3<u32>,
                        @builtin(workgroup_id) wid: vec3<u32>) {
    let cell_n = PATCH_CELL_N;
    if (id.x >= cell_n || id.y >= cell_n) {
        return;
    }

    // LATTICE_1 — the same batch the bake reads; wid.z is the patch.
    let pp = patch_params_batch[wid.z];
    let texel = vec2<i32>(id.xy);
    let layer = i32(pp.layer);

    // Map cell to world-space center position
    let uv = (vec2<f32>(id.xy) + 0.5) / f32(cell_n);
    let world_xz = vec2<f32>(
        pp.origin.x + (uv.x - 0.5) * PATCH_EXTENT,
        pp.origin.y + (uv.y - 0.5) * PATCH_EXTENT
    );

    // THE ONE-ADDRESS LAW, inverted: write texel T colors the cell at
    // address patch_origin_address + T — the SAME derivation the FS
    // uses, run backward. The patch grid index comes from the min
    // corner (origin is the patch CENTER: (g + 0.5) * extent,
    // patch_system.hpp make_patch_params); round() is exact on the
    // aligned grid. world_xz above stays the cell-center FIELD sample
    // point — bit-identical to the pre-law bake.
    let patch_grid = vec2<i32>(round((pp.origin - 0.5 * PATCH_EXTENT) / PATCH_EXTENT));
    let addr = patch_grid * i32(cell_n) + vec2<i32>(id.xy);
    let cell_gx = addr.x;
    let cell_gz = addr.y;
    let cell_seed = lattice_node_seed(config.world_seed, vec2(cell_gx, cell_gz), 200u);

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

}


// §7.2 GOL ZONE COMPUTE — Zone-local Game of Life
// Two compute passes per frame (when zones are active):
// The zone life texture's side — twin of Dim::GOL_ZONE_GRID
// (state.hpp). FIXED at 32 while zp.grid_size is tier-derived over
// {8..32}: the sim writes texels [0, grid_size)² of a 32² layer, so
// every fetch normalizes by THIS, never by the zone's own grid.
const GOL_ZONE_TEX_N: f32 = 32.0;
const GOL_ZONE_STRIDE: u32 = 5120u;     // floats per zone (5 slots × 1024 cells)
const GOL_CELL_VISUAL: u32 = 0u;        // slot 0: height spring visual [0,1]
const GOL_CELL_VELOCITY: u32 = 1024u;   // slot 1: height spring velocity
const GOL_CELL_TARGET: u32 = 2048u;     // slot 2: current target (binary, Conway reads)
const GOL_CELL_NEXT: u32 = 3072u;       // slot 3: next target (binary, Conway writes)
const GOL_CELL_HEIGHT_FACTOR: u32 = 4096u;  // slot 4: per-cell height multiplier (persistent)
// Slots 5-6 were a COLOUR spring. It was provably the height spring: same
// target, same omega/e, same settle thresholds, same apply_boundary and
// select guard, and seeded from the same life_data with the same zero
// velocity — so color_visual == visual for every cell at every frame, and
// always had been. Collapsed; see the commit for the induction.

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
            let next = coupling_gol_next_state(tgt > 0.5, count, z.rule_mask);
            zone_life[base + GOL_CELL_NEXT + idx] = next;
        }
    } else {
        // Pulse: the tier's field function writes the per-cell target, no
        // neighbor rules. BREATH is binary like Conway, so cells plant on
        // the ground / at full height instead of hovering at mid-extension;
        // the shared spring smooths the transitions. A continuous field
        // returns whatever it returns and the same spring tracks it.
        // Written every frame (deterministic from t_beats), not tick-gated.
        let raw_target = pulse_cell_target(
            cell.x, cell.y,
            zone_config.t_beats,
            z.tick_period,
            z.phase_randomness,
            z.tempo_randomness,
            z.field_fn,
            z.grid_size
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


    // --- Write back
    zone_life[base + GOL_CELL_VISUAL + idx] = visual;
    zone_life[base + GOL_CELL_VELOCITY + idx] = velocity;

    // Write to texture: R = the cell's spring visual. The FS tint reads it;
    // the LIFT reads zone_life[VISUAL] from the buffer. One number, two
    // consumers, one channel.
    textureStore(zone_life_tex_write, cell, i32(zone_id), vec4(visual, 0.0, 0.0, 0.0));
}


// ═══ §7.3 THE LIVE CARD (GROUND_CARD_1) ════════════════════════════════
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
// THE CARD WRITER (TRUEBAND_CONTACT_1 T1b, fused at LATTICE_4). ONE
// kernel: each workgroup evaluates its own 20x20 tile (16x16 interior +
// a 2-texel halo) into `sh_card_h`, barriers, and stores
// vec4(h, gx, gz, gol) for the interior.
//
// IT WAS TWO PASSES AND A 3.3 MB SCRATCH BUFFER. Pass 1 evaluated every
// texel and wrote it to storage; pass 2 read a 20x20 neighbourhood back
// and differenced it. That shape paid the evaluation once per texel — the
// bake's model, at card size. The bake stopped needing it at LATTICE_1
// for the same reason the card does not need it here: a halo texel's
// value is a function of a WORLD POSITION, and the field is
// world-continuous, so a lane can evaluate the halo as cheaply as it can
// read it. 400 evaluations per workgroup against 256 stores: the tile
// pays 1.5625 evaluations per output texel and no bus traffic at all.
//
// THE STENCIL IS CENTRAL EVERYWHERE, which the two-pass form could not
// manage: pass 2 clamped its halo reads to the card's edge, so the first
// and last row/column fell back to a one-sided 3-point difference. Here
// the halo is evaluated, not fetched, so texels outside the window carry
// the TRUE field and the edge branches are gone. The window edge lies far
// beyond the veil ring; no pixel could see the difference either way.
//
// THE REST LAW IS A CONJUNCTION, and only two of its three conjuncts
// are MUSICAL. The card is zero — and every consumer therefore adds 0 —
// only when ALL of:
//   (1) config.terrain_time <= 0        the band sum is gated off  [MUSICAL]
//   (2) the pulse ring is empty         contrib_radial_pulses_at is
//       (pulse_count == 0, or every     added OUTSIDE that gate, on its
//       slot aged out / zero-amp)       own clock signal.t_seconds  [MUSICAL]
//   (3) no zone covers the texel        contrib_gol_zones_at feeds .a
//       (zone_config.count == 0, or     with no gate at all, and a zone
//       no covering zone has            runs on ITS OWN tick clock —
//       alive_height >= 0.01 and        beats, not the music's voice.
//       transition_fraction > 0)        [NOT MUSICAL]
// Conjunct (3) is why the one-way "terrain_time <= 0 => zeros" claim was
// tempting and wrong: silence the music and a living zone still lifts.
// Boot pins all three: REST_TERRAIN_TIME and REST_PULSE_COUNT
// (surface/terrain_looks.hpp ROW 2) and an empty zone table. The rest law
// is enforced by the CALLER — phase_live_card_write returns before the
// dispatch when the card is clean (liveCardRestClean_), so this kernel
// never runs at rest and inherits the law unchanged.
//
// Waking anti-teleport is inherited: t_eff = 0 at the origin => moving ==
// frozen => a woken band grows out of the frozen shape.

// ─── THE CARD'S NODE TABLE (LATTICE_4) ──────────────────────────────
//
// The bake's table, at card size — LATTICE_1b's split earning its second
// consumer. A 20x20 card tile spans 20 texels of 1.5625 wu = 31.25 wu of
// world; across the five bands the card sums it touches at most 68
// distinct lattice nodes, and every texel of the tile would otherwise
// re-derive each of them (~11 hash_property calls, three Box-Muller draws,
// two angle transcendentals). Derive each once.
//
// BAND 4 IS ABSENT BY RULING, not by omission: the fine ripple stays
// bake-only (the Nyquist ruling, campaign v2 §6), so its slot is empty and
// CARD_TABLE_OFF[4] == CARD_TABLE_OFF[5].
//
// Nodes per axis = floor(CARD_TILE_SPAN / spacing) + 3, the same
// inequality the bake's table stands on, stated below as const_asserts
// against TERRAIN_BANDS so a band spacing cannot move without the shader
// refusing to compile. The tile's own halo IS the evaluation, so unlike
// the bake there is no stencil margin to add.
const CARD_NODES_0: u32 = 3u;   // spacing 200 — 31.25/200 = 0.16  -> 0 + 3
const CARD_NODES_1: u32 = 3u;   // spacing  80 — 0.39            -> 0 + 3
const CARD_NODES_2: u32 = 4u;   // spacing  30 — 1.04            -> 1 + 3
const CARD_NODES_3: u32 = 5u;   // spacing  12 — 2.60            -> 2 + 3
const CARD_NODES_4: u32 = 0u;   // excluded — the Nyquist ruling
const CARD_NODES_5: u32 = 3u;   // spacing 500 — 0.06            -> 0 + 3

// Prefix sums of n^2 — band b's rectangle occupies [OFF[b], OFF[b+1]).
const CARD_TABLE_OFF = array<u32, 7>(0u, 9u, 18u, 34u, 59u, 59u, 68u);
const CARD_TABLE_N: u32 = 68u;
const_assert CARD_TABLE_OFF[6] == CARD_TABLE_N;

const CARD_TILE_SPAN: f32 = 20.0 * LIVE_CARD_EXTENT / f32(LIVE_CARD_SIZE);
const_assert floor(CARD_TILE_SPAN / TERRAIN_BANDS[0].spacing) + 3.0 <= f32(CARD_NODES_0);
const_assert floor(CARD_TILE_SPAN / TERRAIN_BANDS[1].spacing) + 3.0 <= f32(CARD_NODES_1);
const_assert floor(CARD_TILE_SPAN / TERRAIN_BANDS[2].spacing) + 3.0 <= f32(CARD_NODES_2);
const_assert floor(CARD_TILE_SPAN / TERRAIN_BANDS[3].spacing) + 3.0 <= f32(CARD_NODES_3);
const_assert floor(CARD_TILE_SPAN / TERRAIN_BANDS[5].spacing) + 3.0 <= f32(CARD_NODES_5);

// THE ARITHMETIC ABOVE IS THE BOUNDS GUARD, as it is for the bake: `rel`
// in true_band_delta_contribution is non-negative and below its band's
// count BY THAT INEQUALITY, never by a clamp. WGSL robustness would clamp
// a wrong index silently and hand back a neighbour's wave — a wrong table
// would read as a subtly wrong world, not as a fault.
var<workgroup> card_nodes: array<WaveNode, CARD_TABLE_N>;
var<workgroup> card_origin: array<vec2<i32>, TERRAIN_BAND_COUNT>;   // per-band min node index

// Workgroup shared tile: 20x20 heights (16x16 interior + a 2-texel halo
// for the central stencil).
var<workgroup> sh_card_h: array<f32, 400>;

// THE CARD'S WORKGROUP-STORAGE NEED (LATTICE_2 R5's row, moved here at
// LATTICE_4 — this sum is the program's largest now, past the bake's
// 3,744). 48 = the WGSL size of WaveNode; 8 = vec2<i32>; 4 = f32.
const CARD_WORKGROUP_BYTES: u32 =
    CARD_TABLE_N * 48u + TERRAIN_BAND_COUNT * 8u + 400u * 4u;   // 4,912
const_assert CARD_WORKGROUP_BYTES <= 16384u;   // maxComputeWorkgroupStorageSize, core default
const_assert CARD_WORKGROUP_BYTES >= BAKE_WORKGROUP_BYTES;      // the floor row quotes the larger

// Comparisons only — the blessed shape. Band 4 is empty, so no k ever
// lands in it (OFF[4] == OFF[5]) and this never returns 4.
fn card_band_of(k: u32) -> u32 {
    if (k < CARD_TABLE_OFF[1]) { return 0u; }
    if (k < CARD_TABLE_OFF[2]) { return 1u; }
    if (k < CARD_TABLE_OFF[3]) { return 2u; }
    if (k < CARD_TABLE_OFF[4]) { return 3u; }
    return 5u;
}

fn card_nodes_n(b: u32) -> u32 {
    switch (b) {
        case 0u: { return CARD_NODES_0; }
        case 1u: { return CARD_NODES_1; }
        case 2u: { return CARD_NODES_2; }
        case 3u: { return CARD_NODES_3; }
        default: { return CARD_NODES_5; }
    }
}

@compute @workgroup_size(16, 16)
fn write_live_card(
    @builtin(local_invocation_id) lid: vec3<u32>,
    @builtin(workgroup_id) wid: vec3<u32>
) {
    let texel = LIVE_CARD_EXTENT / f32(LIVE_CARD_SIZE);
    let origin = live_card_origin();
    let t = lid.y * 16u + lid.x;
    let tile_x0 = i32(wid.x * 16u) - 2;
    let tile_y0 = i32(wid.y * 16u) - 2;

    // ── THE NODE TABLE (LATTICE_4) ────────────────────────────────
    // THREE UNCONDITIONAL BARRIERS. The two fills sit inside branches on
    // `bands_awake`, which is a UNIFORM value (config.terrain_time is a
    // uniform read, identical in every lane), and neither branch contains
    // a barrier — so every lane of the workgroup reaches all three.
    let tile_min = origin + vec2<f32>(f32(tile_x0), f32(tile_y0)) * texel;   // the halo's own corner
    let bands_awake = config.terrain_time > 0.0;

    if (bands_awake && t < TERRAIN_BAND_COUNT) {
        card_origin[t] = vec2<i32>(floor(tile_min / TERRAIN_BANDS[t].spacing));
    }
    workgroupBarrier();                                   // 1: origins

    if (bands_awake) {
        for (var k = t; k < CARD_TABLE_N; k += 256u) {
            let b = card_band_of(k);
            let n = card_nodes_n(b);
            let r = k - CARD_TABLE_OFF[b];
            let node = card_origin[b] + vec2<i32>(i32(r % n), i32(r / n));
            card_nodes[k] = derive_wave_node(node,
                lattice_node_seed(config.world_seed, node, b), TERRAIN_BANDS[b]);
        }
    }
    workgroupBarrier();                                   // 2: the table

    // 400 tile texels over 256 threads — the resolve's own load loop,
    // evaluating where it used to fetch.
    for (var k = t; k < 400u; k += 256u) {
        let tx = tile_x0 + i32(k % 20u);
        let ty = tile_y0 + i32(k / 20u);
        // Beyond the window is still the field: no clamp, no edge case.
        let p = origin + (vec2<f32>(f32(tx), f32(ty)) + vec2(0.5)) * texel;
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
        sh_card_h[k] = dh;
    }
    workgroupBarrier();                                   // 3: the tile

    let ix = wid.x * 16u + lid.x;
    let iy = wid.y * 16u + lid.y;
    if (ix >= LIVE_CARD_SIZE || iy >= LIVE_CARD_SIZE) { return; }   // after the barrier

    let cx = lid.x + 2u;
    let cy = lid.y + 2u;
    let height = sh_card_h[cy * 20u + cx];

    // eps = texel-CENTER spacing (extent / res): the card maps texel
    // centers across the window, unlike the bake's corner-pinned
    // (res − 1) grid — the one mapping difference from the model.
    let eps = texel;
    let grad_x = (sh_card_h[cy * 20u + cx + 1u] - sh_card_h[cy * 20u + cx - 1u]) / (2.0 * eps);
    let grad_z = (sh_card_h[(cy + 1u) * 20u + cx] - sh_card_h[(cy - 1u) * 20u + cx]) / (2.0 * eps);

    // .a runs once per INTERIOR texel, as it did — pass 1 wrote it per
    // texel and pass 2 copied it across; the halo never needed it.
    let p_here = origin + (vec2<f32>(f32(ix), f32(iy)) + vec2(0.5)) * texel;
    textureStore(live_card_write, vec2<i32>(i32(ix), i32(iy)),
                 vec4(height, grad_x, grad_z, contrib_gol_zones_at(p_here)));
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

@group(2) @binding(160) var<uniform> photographer_config: PhotographerConfig;
@group(2) @binding(161) var<storage, read_write> photographer_vp: VPMatrix;
@group(2) @binding(162) var<storage, read_write> photographer_camera_out: CameraState;
@group(2) @binding(80) var<storage, read_write> photo_painting_slots: array<UnifiedPaintingSlot, PAINTING_MAX_SLOTS>;
@group(3) @binding(42) var photo_heightfield: texture_2d_array<f32>;
@group(3) @binding(43) var photo_sampler: sampler;

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
@group(2) @binding(81) var<storage, read_write> arch_ground: array<ArchGroundEntry, 16>;

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
@group(2) @binding(82) var<storage, read_write> column_ground: array<ColumnGroundEntry, 32>;

struct PalmGroundEntry {
    center_x: f32,
    center_z: f32,
    ground_y: f32,
    is_active: u32,
    _pad0: f32, _pad1: f32, _pad2: f32, _pad3: f32,
}
// Combined plant ground for compute Y-correction: palm[0..23] + cactus[24..43] + blade[44..75]
@group(2) @binding(83) var<storage, read_write> plant_ground: array<PalmGroundEntry, 76>;

// Entity ground atlas — compute writes corrected ground_y (r32float, 256×1)
@group(3) @binding(80) var entity_ground_atlas_write: texture_storage_2d<r32float, write>;

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
@group(2) @binding(43) var<storage, read> patch_grid: PatchGrid;

// --- Terrain Height Sampling
// O(1) lookup: hash world_xz to patch grid cell, read layer, sample heightfield.
//
// THE WALK HAS ONE HOME (SPINE_2). Two callers want two different things on a
// MISS — sample_terrain_y_at wants 0.0 (the old linear-scan behaviour for
// out-of-range queries, which every agent and placement reader depends on),
// and ribbon_ground wants to fall back to the analytic form. So the walk
// returns (height, found) and each caller keeps its own miss behaviour
// without a second copy of the grid walk or the uv remap.
//
// NOT PARAMETRISED over the texture and sampler, which is how the ribbon room
// was expected to reach it: WGSL forbids a `ptr<uniform, T>` function
// parameter outright (naga: "a pointer of space Uniform ... can't be passed
// into functions"), and passing the grid BY VALUE would copy it per call. It
// needs neither. The ribbon room BORROWS patch_grid, photo_heightfield and
// photo_sampler at their existing numbers — the ratified idiom of the g2
// PATCHGEN band, "a home is a numbering band, not necessarily a seat ...
// readers borrow at its numbers" — so the globals resolve for its pipelines
// too and this function is literally the same code for both rooms.
fn sample_terrain_y_found(world_xz: vec2<f32>) -> vec2<f32> {
    let gx = i32(floor(world_xz.x / patch_grid.cell_extent));
    let gz = i32(floor(world_xz.y / patch_grid.cell_extent));
    let lx = gx - patch_grid.origin_x;
    let lz = gz - patch_grid.origin_z;
    let s = i32(patch_grid.side);
    if (lx < 0 || lz < 0 || lx >= s || lz >= s) { return vec2(0.0, 0.0); }

    let packed = patch_grid.entries[lz * s + lx];
    if (packed == 0u) { return vec2(0.0, 0.0); }
    let layer = i32(packed - 1u);

    // Patch origin is the cell center; UV is local offset normalized to extent.
    let origin = vec2(f32(gx) + 0.5, f32(gz) + 0.5) * patch_grid.cell_extent;
    let local = world_xz - origin;
    let uv = local / patch_grid.cell_extent + 0.5;
    // Remap UV to texel centers (texel i is lattice point i; bilinear
    // between lattice points is the drawn surface)
    let res = f32(PATCH_HEIGHTFIELD_N);
    let sample_uv = (uv * (res - 1.0) + 0.5) / res;
    return vec2(textureSampleLevel(photo_heightfield, photo_sampler,
                                   sample_uv, layer, 0.0).x, 1.0);
}

// Returns 0.0 outside the active patch window or on empty slots (preserves
// the old linear-scan behavior for out-of-range queries).
fn sample_terrain_y_at(world_xz: vec2<f32>) -> f32 {
    return sample_terrain_y_found(world_xz).x;
}

// CONTACT_2 C2a — gradient sibling of sample_terrain_y_at. Same patch_grid
// lookup + uv math; returns the baked .yz slope (out-of-window => vec2(0),
// per the original's convention). The whisper reader (steering); it adds
// no binding — patch_grid/photo_heightfield/photo_sampler already resolve
// in the agent kernels' closures. Living plateaus (card.a) are NOT steered
// this batch — the card carries no GoL gradient (the deferred sibling).
fn sample_terrain_grad_at(world_xz: vec2<f32>) -> vec2<f32> {
    let gx = i32(floor(world_xz.x / patch_grid.cell_extent));
    let gz = i32(floor(world_xz.y / patch_grid.cell_extent));
    let lx = gx - patch_grid.origin_x;
    let lz = gz - patch_grid.origin_z;
    let s = i32(patch_grid.side);
    if (lx < 0 || lz < 0 || lx >= s || lz >= s) { return vec2(0.0); }

    let packed = patch_grid.entries[lz * s + lx];
    if (packed == 0u) { return vec2(0.0); }
    let layer = i32(packed - 1u);

    let origin = vec2(f32(gx) + 0.5, f32(gz) + 0.5) * patch_grid.cell_extent;
    let local = world_xz - origin;
    let uv = local / patch_grid.cell_extent + 0.5;
    let res = f32(PATCH_HEIGHTFIELD_N);
    let sample_uv = (uv * (res - 1.0) + 0.5) / res;
    return textureSampleLevel(photo_heightfield, photo_sampler,
                              sample_uv, layer, 0.0).yz;
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
    // pyramids only). See update_camera_vp for the rationale.
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
// CPU uploads ground_y = 0 (GPU owns the terrain sample).
// This shader samples the heightfield and adds the terrain height.
//
// POLICY_BAKED_HEIGHTFIELD consumer (texture variant).
// sample_terrain_y_at reads the cached patch_heightfield_array_read,
// which bake_patch_heightfield populates. That bake evaluates the
// baked heightfield's contributor set (static_base + pyramids) at each
// lattice point and caches the result.
// This Y-correction pass samples the cache rather than re-evaluating
// contributors analytically — it is *not* a spawn-time placement
// query: CPU spawn decisions stay on estimate_terrain_height (the
// tile-cache proxy in machine/spawn_engine.hpp), and this GPU pass is
// the live Y path via the baked heightfield.
//
// Paintings: terrain + the GoL cell lift (the card's .a).
// Arch: 2-point min at the leg positions + pier_height offset (the legs' visual height).
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
    // Indoor WALL FRAMES use sentinel patch coords (0x7FFFFFFF) and are
    // skipped: their Y is the wall's, measured from the ceiling down, and the
    // ground under them is not their business.
    //
    // A SENTINEL TERRAIN QUAD IS NOT A WALL FRAME (ATRIUM_5). The sentinel
    // says NO PATCH OWNS ME — it is the eviction discriminator
    // (evict_paintings_for_patch, clear_wall_paintings) — and it was reused
    // here as an "is this indoor" test because until ATRIUM_3 no terrain quad
    // had ever carried it. The atrium's sand image does: it stands on the
    // floor of a room, owned by no patch, and its foot wants the ground like
    // every other terrain quad's. So the skip is narrowed to the form it was
    // always about.
    for (var i = 0u; i < PAINTING_MAX_SLOTS; i++) {
        let owned = photo_painting_slots[i].patch_gx != 0x7FFFFFFF;
        let seats = owned || photo_painting_slots[i].form_type == FORM_TERRAIN_QUAD;
        if (photo_painting_slots[i].is_active != 0u && seats) {
            let slot_xz = vec2(
                photo_painting_slots[i].position.x,
                photo_painting_slots[i].position.z
            );
            // Painting Y-correction — hybrid POLICY_PLACEMENT_PAINTING
            // evaluation. The cached heightfield (sample_terrain_y_at)
            // covers static_base + pyramids; GoL rides the card's
            // cell-exact raw lift (sample_live_card_gol — GROUND_CARD_1;
            // was an analytic zone loop). Equivalent in value to
            // the placement-painting query would have been, but cheaper per
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

    // --- Arch: 2-point min at the leg positions (slope straddle).
    for (var i = 0u; i < 16u; i++) {
        if (arch_ground[i].is_active != 0u) {
            let left_xz = vec2(arch_ground[i].pier_left_x, arch_ground[i].pier_left_z);
            let right_xz = vec2(arch_ground[i].pier_right_x, arch_ground[i].pier_right_z);
            // b2b: each leg rides its own local GoL, then min (see column).
            let tl = sample_terrain_y_at(left_xz) + sample_live_card_gol(left_xz);
            let tr = sample_terrain_y_at(right_xz) + sample_live_card_gol(right_xz);
            arch_ground[i].ground_y = min(tl, tr);
            textureStore(entity_ground_atlas_write, vec2<i32>(i32(i) + GROUND_ATLAS_ARCH, 0), vec4<f32>(arch_ground[i].ground_y, 0.0, 0.0, 0.0));
        }
    }
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
// Main pass reads the visible list through the visible_id instance
// attribute (DOMESDAY_0 B3 — pulled from fc_visible's buffer as a
// vertex fetch; the storage seat g2:62 is retired).
// Shadow pass uses direct patch_instances[instance_index] (no frustum cull).

const FRUSTUM_PATCH_Y_MIN: f32 = -50.0;   // widened: terrain amplitude + entity heights
const FRUSTUM_PATCH_Y_MAX: f32 = 200.0;   // widened: tall entities (towers, antennas, ribbons)
// The LOD0 gate reads fc_config.lod0_radius — the SAME config value the
// CPU band reads, so the radius has one spelling in the whole program.

// Frustum cull compute bindings (dedicated bind group)
@group(0) @binding(0)   var<uniform>             fc_config: DesignConfig;
@group(2) @binding(240)   var<storage, read>       fc_vp: VPMatrix;
@group(2) @binding(61) var<storage, read>       fc_patches: array<PatchInstance>;
@group(2) @binding(63) var<storage, read_write> fc_visible: array<u32>;
@group(2) @binding(64) var<storage, read_write> fc_indirect: array<atomic<u32>, 15>;

// THE DRAW PLAN (ECONOMY_1 closing arm) — the kernel authors three
// lists; the main pass executes them as three indirect draws.
//   A: LOD0, zone-overlapped -> full IB    (fc_visible[  0..128))
//   B: LOD0, clean           -> cap-only IB (fc_visible[128..256))
//   C: LOD1, frustum-visible -> LOD1 IB    (fc_visible[256..512))
// Segment BYTE offsets 0/512/1024 — 256-aligned for the render side's
// offset bind groups. TWIN: state.hpp FC segment constants (L3 MIRROR
// — change both rooms together). fc_indirect: 3 x 5 draw-args slots;
// instance counters at indices 1 / 6 / 11.
struct DrawPlanParams {
    lod0_count: u32,     // CPU band boundary — instances [0, lod0) are LOD0
    render_count: u32,   // draw set end — [lod0, render) LOD1; beyond: pregen
    rect_count: u32,     // active zone rects, packed dense
    _pad0: u32,
    rects: array<vec4<f32>, 8>,   // corner.xy, extent.xy (world)
}
@group(2) @binding(60) var<uniform> fc_draw_plan: DrawPlanParams;

const FC_SEG_A_BASE: u32 = 0u;    const FC_SEG_A_CAP: u32 = 128u;
const FC_SEG_B_BASE: u32 = 128u;  const FC_SEG_B_CAP: u32 = 128u;
const FC_SEG_C_BASE: u32 = 256u;  const FC_SEG_C_CAP: u32 = 256u;

// THE HONEST MARGIN (R1): every terrain displacement is Y-ONLY —
// baked heightfield, live-card delta, cell lift, aura, skirt drop all
// add to world_pos.y; XZ is pure lattice. So the planar margin is
// SAFETY, not physics. Hot-tunable; the pop hunt at the gate rules it.
const CULL_MARGIN_WU: f32 = 5.0;

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

    // THE PLAN GATE: only the draw set is classified — instances are
    // packed [LOD0][LOD1][pregen] by the CPU band, so the band is read
    // off the INDEX against the plan's counts (the CPU's own banding;
    // no distance re-derivation, no boundary disagreement).
    if (id.x >= fc_draw_plan.render_count) { return; }

    // AABB with the honest planar margin (R1: displacement is Y-only).
    let half = pi.extent * 0.5;
    let margin = CULL_MARGIN_WU;
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

    // Frustum test rejects out-of-view patches — ALL bands now; this
    // is the first frustum test LOD1 ever had.
    if (!aabb_in_frustum(planes, bmin, bmax)) { return; }

    if (id.x < fc_draw_plan.lod0_count) {
        // LOD0 — split by zone overlap: a curtain can only be
        // non-degenerate where a zone rect reaches the patch, so the
        // clean majority draws the cap-only IB. Patch rect vs zone
        // rect, <=8 rects, early-out on the first hit.
        var zoned = false;
        for (var z = 0u; z < fc_draw_plan.rect_count; z++) {
            let r = fc_draw_plan.rects[z];
            if (pi.origin.x + half >= r.x && pi.origin.x - half <= r.x + r.z &&
                pi.origin.y + half >= r.y && pi.origin.y - half <= r.y + r.w) {
                zoned = true;
                break;
            }
        }
        if (zoned) {
            let slot = atomicAdd(&fc_indirect[1], 1u);
            if (slot < FC_SEG_A_CAP) { fc_visible[FC_SEG_A_BASE + slot] = id.x; }
            else { atomicSub(&fc_indirect[1], 1u); }
        } else {
            let slot = atomicAdd(&fc_indirect[6], 1u);
            if (slot < FC_SEG_B_CAP) { fc_visible[FC_SEG_B_BASE + slot] = id.x; }
            else { atomicSub(&fc_indirect[6], 1u); }
        }
    } else {
        // LOD1 — culled at last.
        let slot = atomicAdd(&fc_indirect[11], 1u);
        if (slot < FC_SEG_C_CAP) { fc_visible[FC_SEG_C_BASE + slot] = id.x; }
        else { atomicSub(&fc_indirect[11], 1u); }
    }
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

// L3 MIRROR — twin of Dim::PAINTING_MAX_SLOTS in state.hpp. Both rooms,
// same commit; nothing the compiler can see holds this pair.
const PAINTING_MAX_SLOTS: u32 = 288u;

// --- Gallery Group 1 bindings (shared by terrain quads + wall frames)

@group(2) @binding(85) var<storage, read> painting_slots: array<UnifiedPaintingSlot, PAINTING_MAX_SLOTS>;
@group(3) @binding(160) var painting_array: texture_2d_array<f32>;
@group(3) @binding(161) var painting_sampler_filt: sampler;

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

// The quad's geometry, shared by the color VS and its shadow twin. Both
// gates live here — the bounds guard and THE RING — so a caster set and a
// draw set cannot drift apart. `valid` false means the slot is not drawn
// by this entry point; each caller emits its own degenerate position.
struct GalleryQuadPoint {
    world:         vec3<f32>,
    uv:            vec2<f32>,
    fwd:           vec3<f32>,
    uv_scale:      vec2<f32>,
    texture_layer: u32,
    valid:         bool,
};

fn compute_gallery_quad_geometry(vid: u32, iid: u32) -> GalleryQuadPoint {
    var out: GalleryQuadPoint;
    out.world = vec3(0.0);
    out.uv = vec2(0.0);
    out.fwd = vec3(0.0, 1.0, 0.0);
    out.uv_scale = vec2(1.0);
    out.texture_layer = 0u;
    out.valid = false;

    // Instance count comes from the CPU (Dim::PAINTING_MAX_SLOTS, at
    // renderer.hpp's draw_gallery_frames) and the array size from this
    // room. They agree today, which is the only reason the read below has
    // stood unguarded. wall_painting_vs already guards its decoded index;
    // this is the same guard, on the other painting entry point.
    if (iid >= PAINTING_MAX_SLOTS) { return out; }

    let slot = painting_slots[iid];

    // THE RING (draw authority): outdoor frames draw only inside the ring
    // (center - extent <= ring; 5 wu covers the frame's half-reach).
    let frame_in_ring = distance(slot.position.xz,
                                 vec2(config.lod_point_x, config.lod_point_z))
                        - 5.0 <= config.veil_ring;
    if (slot.is_active == 0u || slot.form_type != FORM_TERRAIN_QUAD || !frame_in_ring) {
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

    out.world = world;
    out.uv = uv;
    out.fwd = fwd;
    out.uv_scale = vec2(slot.uv_scale_x, slot.uv_scale_y);
    out.texture_layer = slot.texture_layer;
    out.valid = true;
    return out;
}

// --- Vertex Shader
@vertex
fn gallery_frame_vs(
    @builtin(vertex_index) vid: u32,
    @builtin(instance_index) iid: u32,
) -> GalleryVarying {
    var out: GalleryVarying;

    // Both gates — the bounds guard and THE RING — now live in
    // compute_gallery_quad_geometry, shared with the shadow twin. The two
    // early-return blocks this VS used to carry are one block, because
    // they always emitted the same degenerate vertex.
    let g = compute_gallery_quad_geometry(vid, iid);
    if (!g.valid) {
        out.clip_pos = vec4(0.0, 0.0, 0.0, 1.0);
        out.uv = vec2(0.0);
        out.world_pos = vec3(0.0);
        out.world_normal = vec3(0.0, 1.0, 0.0);
        out.texture_layer = 0u;
        return out;
    }

    out.clip_pos = frame_r.vp.m * vec4(g.world, 1.0);
    // THE SLOT'S uv_scale, which this path used to drop on the floor. The wall
    // path has always applied it (compute_wall_painting_geometry); this one
    // emitted raw uv, so a slot occupying part of its layer drew the whole
    // layer — the composite quads, when snapshots were briefly 512.
    //
    // It affects OUTDOOR SNAPSHOTS ONLY, and today they fill their layer, so
    // this line changes nothing that draws. The two entry points partition by
    // form type, and outdoor AUTHORED paintings go through fill_slot_wall_frame
    // into FORM_WALL_FRAME — the path that already scaled — so they never
    // showed load_authored_image_to_staging's letterbox padding. The only
    // writer of FORM_TERRAIN_QUAD is commit_gallery's snapshot branch.
    //
    // It lands anyway: an entry point that silently ignores a field the slot
    // carries is a trap, and it cost this campaign a stage.
    //
    // SCALE AFTER THE FLIP, and the two paths then agree exactly. `uv` runs 0 at
    // the quad's bottom (local.y = (uv.y - 0.5) * scale_y), so the flip sends
    // bottom to v = 1 and top to v = 0; scaling here selects v in [0, s], the
    // ORIGIN-side band, which is the end a partial write lands on. The wall
    // path's corner/uv tables give the same mapping — bottom to s, top to 0.
    // Scaling BEFORE the flip would select v in [1-s, 1], the far end, where
    // nothing is ever written.
    //
    // `uv` itself stays raw above: it drives local position and
    // deform_gallery_frame, which are geometry and must not move with content.
    out.uv = vec2(g.uv.x, 1.0 - g.uv.y) * g.uv_scale;
    out.world_pos = g.world;
    out.world_normal = g.fwd;
    out.texture_layer = g.texture_layer;
    return out;
}

// §8.1.1 GALLERY FRAME SHADOW
@vertex
fn shadow_gallery_frame_vs(
    @builtin(vertex_index) vid: u32,
    @builtin(instance_index) iid: u32,
) -> ShadowVarying {
    var out: ShadowVarying;
    let g = compute_gallery_quad_geometry(vid, iid);
    if (!g.valid) {
        out.clip_pos = vec4(0.0, 0.0, 0.0, 1.0);
        return out;
    }
    out.clip_pos = shadow_light_vp() * vec4(g.world, 1.0);
    return out;
}

// --- Fragment Shader
@fragment
fn gallery_frame_fs(in: GalleryVarying) -> @location(0) vec4<f32> {
    let painting_color = textureSample(painting_array, painting_sampler_filt, in.uv, in.texture_layer);
    if (painting_color.a < 0.01) { discard; }

    var color = painting_color.rgb;

    // Distance fog only (scene consistency — paintings far away dissolve into fog)
    let dist = distance(in.world_pos, frame_r.camera.pos);
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
    out.clip_pos = frame_r.vp.m * vec4(out.world_pos, 1.0);
    return out;
}

@fragment
fn wall_painting_canvas_fs(in: WallPaintingVarying) -> @location(0) vec4<f32> {
    if (in.is_canvas == 0u) { discard; }

    let slot = painting_slots[in.painting_index];
    let tex_color = textureSample(painting_array, painting_sampler_filt, in.uv, slot.texture_layer);
    if (tex_color.a < 0.01) { discard; }

    let lit = tex_color.rgb * 0.9;
    let dist = distance(in.world_pos, frame_r.camera.pos);
    let fog = 1.0 - exp(-dist * config.fog_density);
    return vec4(mix(lit, config.fog_color, fog), 1.0);
}

@fragment
fn wall_painting_frame_fs(in: WallPaintingVarying) -> @location(0) vec4<f32> {
    if (in.is_canvas == 1u) { discard; }

    let lit = in.frame_color * 0.8;
    let dist = distance(in.world_pos, frame_r.camera.pos);
    let fog = 1.0 - exp(-dist * config.fog_density);
    return vec4(mix(lit, config.fog_color, fog), 1.0);
}

// §8.3 WALL PAINTING SHADOW
// The twin of wall_painting_vs: the same decode, the same two guards, the
// same geometry call and the same live-card lift, differing only in the
// matrix. BOTH VERT RANGES PASS THROUGH — canvas [0, 6) and frame [6, 78).
// The frame is an open border with a canvas-shaped hole (§8.2's [54, 78)
// branch), so a frame-only caster would print an outline with a lit hole
// where the picture is. That is why ONE shadow draw replaces the color
// pass's two: with no fragment stage the canvas/frame split has nothing
// left to distinguish.
@vertex
fn shadow_wall_painting_vs(@builtin(vertex_index) vid: u32) -> ShadowVarying {
    let pidx = vid / PAINTING_FRAME_VERTS_PER;
    let local_vid = vid % PAINTING_FRAME_VERTS_PER;

    if (pidx >= PAINTING_MAX_SLOTS) {
        var out: ShadowVarying;
        out.clip_pos = vec4(0.0);
        return out;
    }

    let slot = painting_slots[pidx];

    // Skip inactive or terrain-quad slots
    if (slot.is_active == 0u || slot.form_type != FORM_WALL_FRAME) {
        var out: ShadowVarying;
        out.clip_pos = vec4(0.0);
        return out;
    }

    var g = compute_wall_painting_geometry(slot, pidx, local_vid);
    g.world_pos.y += sample_live_card(g.world_pos.xz).x;
    var out: ShadowVarying;
    out.clip_pos = shadow_light_vp() * vec4(g.world_pos, 1.0);
    return out;
}


// ═══════════════════════════════════════════════════════════════════════
// §9 GPU ENTITY MESH GENERATION
// ═══════════════════════════════════════════════════════════════════════
//
// GPU-sovereign geometry: CPU authors intent (params), GPU realizes mesh.
// Each entity family writes into fixed per-slot regions of pre-allocated
// VB/IB buffers. Inactive slots receive degenerate (zero-area) triangles.
//
// Two families: arches (§9.1), columns (§9.2) — the pyramid's realization
// is the terrain itself: placement feeds the heightfield, there is no mesh.
//
// Vertex format: matches ArchVertex (pos[3], normal[3], color[3], index:u32)
// = 10 × f32 per vertex = 40 bytes. VB is accessed as array<f32>.
//
// ─── THE STRIDE LAW (LATTICE_2) ──────────────────────────────────────
//
// ONE WORKGROUP PER SLOT, MESHGEN_LANES LANES INSIDE IT. Every family's
// kernel used to be @workgroup_size(1): one thread built a whole entity,
// serially, while 63 lanes of the same wave sat idle — mesh gen fires on
// spawn frames, which is exactly where a frame can least afford it.
//
// The transformation is the same five times and it is deliberately narrow:
//
//   · the slot (and the arch's sub-mesh) comes from `workgroup_id`;
//     `global_invocation_id` is no longer the slot. Host dispatch shapes
//     do not change — they were already one workgroup per slot.
//   · in every emission section the OUTERMOST loop is strided by the lane
//     (`for (var o = lane; o < N; o += MESHGEN_LANES)`); every INNER loop
//     stays verbatim.
//   · the `vi++` / `ii++` cursors become closed-form arithmetic — the
//     value the cursor HELD at that iteration, so the write addresses are
//     the same numbers in a different order.
//   · the skeleton before emission (profiles, trig tables, counts,
//     ceilings) is replayed verbatim by EVERY lane. It is pure in the
//     slot's params, so 64 lanes computing it agree by construction.
//     No var<workgroup>, no barrier, nothing to synchronize.
//
// Lanes write DISJOINT addresses: each section's write index is a
// bijection of (outer, inner), and the outer values partition across
// lanes. Output is byte-identical to the serial kernel's.
//
// LOOP-CARRIED STATE STAYS INSIDE A LANE. Where an outer iteration
// accumulates (the cactus arm's `apx/apy/apz` walk), that loop is the
// strided one and a lane walks its whole arm serially, exactly as before.
// This is why the stride is the OUTER loop and never the vertex.
//
// The ideal — an invocation is one output — is the horizon, not this
// round: the outer stride already takes each kernel from one lane to
// tens. If a measurement asks for more, the inner loops are next.
const MESHGEN_LANES: u32 = 64u;


// ─── §9.1 ARCH MESH GENERATION (catenary barrel vault) ───────────────
//
// Four sub-meshes per arch: outer shell, inner shell, front cap, back cap.
// Indexed vertices with shared edges (grid topology). The catenary
// parameter 'a' is precomputed on CPU and passed in params.
//
// Dispatch: (16, 4, 1) — 4 WORKGROUPS per arch slot (LATTICE_2; it was
// 4 threads).
//   workgroup_id.x = slot index (0..15)
//   workgroup_id.y = sub-mesh (0=outer shell, 1=inner shell, 2=front cap,
//                              3=back cap)
//   local_invocation_id.x = the lane, MESHGEN_LANES of them
//
// Each sub-mesh writes to a deterministic offset within the slot's VB/IB
// region, computed from segs_u and segs_v. That partition is what let the
// four sub-meshes run as four threads in the first place; the stride law
// now partitions each sub-mesh again, across its lanes.

// ── Constants ─────────────────────────────────────────────────────────

const AMG_MAX_VERTS_PER_SLOT: u32   = 2000u;
const AMG_MAX_INDICES_PER_SLOT: u32 = 7500u;  // must be divisible by 3 (triangle alignment)
const AMG_FLOATS_PER_VERTEX: u32    = 10u;   // pos(3) + normal(3) + color(3) + index(1)
const AMG_MAX_SLOTS: u32            = 16u;


// ── Parameter buffer ──────────────────────────────────────────────────
//
// MUST match state.hpp::GPUArchMeshParams (size: 80 bytes).
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
    // MOSAIC_1 (GROWTH 64 → 80 with the C++ twin): 0 = plain;
    // enc = mosaic_seed·64 + slot rides the vertex index channel.
    mosaic_seed: u32,
    _pad80_0: u32,
    _pad80_1: u32,
    _pad80_2: u32,
}

// ── Bindings (dedicated layout — different binding numbers from pyramid) ─

@group(2) @binding(180) var<storage, read>       amg_params: array<ArchMeshParams, 16>;
@group(2) @binding(181) var<storage, read_write>  amg_vertices: array<f32>;
@group(2) @binding(182) var<storage, read_write>  amg_indices: array<u32>;

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

// ── Shell generation (one workgroup per sub-mesh, outer loop strided) ──
//
// Writes (su+1)*(sv+1) vertices and su*sv*6 indices at the given offsets.
// offset = +half_t (outer) or -half_t (inner).
// nsign = +1.0 (outer) or -1.0 (inner).
//
// Catenary profile is precomputed into lane-local arrays and reused across
// all v-rows, which eliminates (sv) redundant exp() evaluations per
// u-column (13× reduction for monumental arches). Under the stride law it
// is SKELETON: every lane replays it, because it is pure in (p, offset)
// and both are uniform across the workgroup.
//
// The v-row is the strided loop, the u-column the verbatim inner one.


fn amg_gen_shell(
    p: ArchMeshParams, slot: u32, lane: u32,
    vb_start: u32, ib_start: u32,
    offset: f32, nsign: f32,
    co: f32, si: f32, base_y: f32, a: f32, H: f32
) {
    // THE INDEX CHANNEL (MOSAIC_1): every vertex of this body carries
    // enc — legacy plain-slot meshes decode identically (seed 0).
    let enc = p.mosaic_seed * 64u + slot;
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
    // vi was vb_start + iv * stride + iu; that is the write index now.
    for (var iv = lane; iv <= sv; iv += MESHGEN_LANES) {
        let v = f32(iv) / f32(sv);
        let lz = -half_d + p.depth * v;
        for (var iu = 0u; iu <= su; iu++) {
            let wx = p.center_x + cat_lx[iu] * co - lz * si;
            let wy = cat_ly[iu];
            let wz = p.center_z + cat_lx[iu] * si + lz * co;

            let wnx = (cat_pnx[iu] * nsign) * co;
            let wny = cat_pny[iu] * nsign;
            let wnz = (cat_pnx[iu] * nsign) * si;

            amg_write_vertex(vb_start + iv * stride + iu, wx, wy, wz, wnx, wny, wnz,
                p.color_r, p.color_g, p.color_b, enc);
        }
    }

    // ── Indices ──
    // ii was ib_start + (iv * su + iu) * 6u + j.
    for (var iv = lane; iv < sv; iv += MESHGEN_LANES) {
        for (var iu = 0u; iu < su; iu++) {
            let i00 = vb_start + iv * stride + iu;
            let i10 = i00 + 1u;
            let i01 = i00 + stride;
            let i11 = i01 + 1u;
            let ii = ib_start + (iv * su + iu) * 6u;
            if (nsign > 0.0) {
                amg_indices[ii + 0u] = i00;
                amg_indices[ii + 1u] = i01;
                amg_indices[ii + 2u] = i10;
                amg_indices[ii + 3u] = i10;
                amg_indices[ii + 4u] = i01;
                amg_indices[ii + 5u] = i11;
            } else {
                amg_indices[ii + 0u] = i00;
                amg_indices[ii + 1u] = i10;
                amg_indices[ii + 2u] = i01;
                amg_indices[ii + 3u] = i10;
                amg_indices[ii + 4u] = i11;
                amg_indices[ii + 5u] = i01;
            }
        }
    }
}

// ── Cap generation (one workgroup per sub-mesh, outer loop strided) ────
//
// Writes 2*(su+1) vertices and su*6 indices at the given offsets.
// lz_pos = +half_d (front) or -half_d (back).
// nz_sign = +1.0 (front) or -1.0 (back).
//
// The u-column is the strided loop; it writes an outer/inner PAIR, so the
// pair's base is vb_start + iu * 2u — the two values the cursor held.

fn amg_gen_cap(
    p: ArchMeshParams, slot: u32, lane: u32,
    vb_start: u32, ib_start: u32,
    lz_pos: f32, nz_sign: f32,
    co: f32, si: f32, base_y: f32, a: f32, H: f32
) {
    // THE INDEX CHANNEL (MOSAIC_1): every vertex of this body carries
    // enc — legacy plain-slot meshes decode identically (seed 0).
    let enc = p.mosaic_seed * 64u + slot;
    let su = p.segs_u;
    let half_t = p.thickness * 0.5;

    // Cap normal (rotated into world)
    let cap_nx = -nz_sign * si;
    let cap_ny = 0.0;
    let cap_nz = nz_sign * co;

    // Vertices: outer/inner pairs along catenary profile
    for (var iu = lane; iu <= su; iu += MESHGEN_LANES) {
        let vi = vb_start + iu * 2u;
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
            p.color_r, p.color_g, p.color_b, enc);

        // Inner vertex
        let ilx = t - pnx * half_t;
        let ily = base_y + y - pny * half_t;
        amg_write_vertex(vi + 1u,
            p.center_x + ilx * co - lz_pos * si,
            ily,
            p.center_z + ilx * si + lz_pos * co,
            cap_nx, cap_ny, cap_nz,
            p.color_r, p.color_g, p.color_b, enc);
    }

    // Indices: quad strip between outer/inner
    // ii was ib_start + iu * 6u + j.
    for (var iu = lane; iu < su; iu += MESHGEN_LANES) {
        let o0 = vb_start + iu * 2u;
        let i0 = o0 + 1u;
        let o1 = vb_start + (iu + 1u) * 2u;
        let i1 = o1 + 1u;
        let ii = ib_start + iu * 6u;
        if (nz_sign > 0.0) {
            amg_indices[ii + 0u] = o0;
            amg_indices[ii + 1u] = i0;
            amg_indices[ii + 2u] = o1;
            amg_indices[ii + 3u] = o1;
            amg_indices[ii + 4u] = i0;
            amg_indices[ii + 5u] = i1;
        } else {
            amg_indices[ii + 0u] = o0;
            amg_indices[ii + 1u] = o1;
            amg_indices[ii + 2u] = i0;
            amg_indices[ii + 3u] = o1;
            amg_indices[ii + 4u] = i1;
            amg_indices[ii + 5u] = i0;
        }
    }
}

// ── Compute entry point ───────────────────────────────────────────────
//
// Dispatch: (16, 4, 1) — one workgroup per (slot, sub-mesh)
//   workgroup_id.x = slot, workgroup_id.y = sub-mesh, local_invocation_id.x = lane
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

@compute @workgroup_size(MESHGEN_LANES)
fn arch_mesh_gen(
    @builtin(workgroup_id) wid: vec3<u32>,
    @builtin(local_invocation_id) lid: vec3<u32>
) {
    // THE WORKGROUP IS (slot, sub_mesh) — the dispatch shape is unchanged
    // (MAX_ARCH_INSTANCES, 4, 1); it just names workgroups now, not
    // threads. The guard is uniform across the workgroup: every lane of a
    // dead (slot, sub_mesh) returns together.
    let slot = wid.x;
    let sub_mesh = wid.y;
    let lane = lid.x;
    if (slot >= AMG_MAX_SLOTS || sub_mesh >= 4u) { return; }

    let p = amg_params[slot];
    let slot_vb = slot * AMG_MAX_VERTS_PER_SLOT;
    let slot_ib = slot * AMG_MAX_INDICES_PER_SLOT;

    // ── Inactive: each sub-mesh zeroes its quarter of the index range ─
    if (p.is_active == 0u) {
        let chunk = AMG_MAX_INDICES_PER_SLOT / 4u;
        let start = slot_ib + sub_mesh * chunk;
        for (var i = lane; i < chunk; i += MESHGEN_LANES) {
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
            amg_gen_shell(p, slot, lane,
                slot_vb, slot_ib,
                half_t, 1.0,
                co, si, base_y, a, H);
        }
        case 1u: {
            // Inner shell
            amg_gen_shell(p, slot, lane,
                slot_vb + shell_verts,
                slot_ib + shell_indices,
                -half_t, -1.0,
                co, si, base_y, a, H);
        }
        case 2u: {
            // Front cap
            amg_gen_cap(p, slot, lane,
                slot_vb + 2u * shell_verts,
                slot_ib + 2u * shell_indices,
                half_d, 1.0,
                co, si, base_y, a, H);

            // This SUB-MESH also zeroes unused indices after the caps,
            // strided across its lanes. Total used = 2*shell_indices +
            // 2*cap_indices.
            let total_used = 2u * shell_indices + 2u * cap_indices;
            for (var i = total_used + lane; i < AMG_MAX_INDICES_PER_SLOT; i += MESHGEN_LANES) {
                amg_indices[slot_ib + i] = slot_vb;
            }
        }
        case 3u: {
            // Back cap
            amg_gen_cap(p, slot, lane,
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
// Dispatch: (32, 1, 1) — one WORKGROUP per column slot, MESHGEN_LANES
// lanes inside it (LATTICE_2; it was one invocation).

// ── Constants ─────────────────────────────────────────────────────────

const CMG_MAX_VERTS_PER_SLOT: u32    = 1500u;
const CMG_MAX_INDICES_PER_SLOT: u32  = 6000u; // must be divisible by 3
const CMG_FLOATS_PER_VERTEX: u32     = 10u;   // pos(3) + normal(3) + color(3) + index(1)
const CMG_MAX_SLOTS: u32             = 32u;
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
    // MOSAIC_1: _pad128_0 repurposed (the indoor_height_cap precedent) —
    // 0 = plain; enc = mosaic_seed·64 + slot rides the index channel.
    mosaic_seed: u32, _pad128_1: f32, _pad128_2: f32,
}

// ── Bindings ──────────────────────────────────────────────────────────

@group(2) @binding(180) var<storage, read>       cmg_params: array<ColumnMeshParams, 32>;
@group(2) @binding(181) var<storage, read_write>  cmg_vertices: array<f32>;
@group(2) @binding(182) var<storage, read_write>  cmg_indices: array<u32>;
// COLUMN CEILING FIT: the ceiling gate + the correction pass's ground
// output (read-only view of binding 148's buffer — slot-aligned with
// cmg_params: columns 0.., antennas at ANTENNA_SLOT_OFFSET).
@group(2) @binding(183) var<uniform>             cmg_config: DesignConfig;
@group(2) @binding(84) var<storage, read>       cmg_column_ground: array<ColumnGroundEntry, 32>;

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

@compute @workgroup_size(MESHGEN_LANES)
fn column_mesh_gen(
    @builtin(workgroup_id) wid: vec3<u32>,
    @builtin(local_invocation_id) lid: vec3<u32>
) {
    // ONE WORKGROUP PER SLOT (LATTICE_2). The dispatch shape,
    // (MAX_COLUMN_INSTANCES, 1, 1), is unchanged. The whole skeleton below
    // — the profile polyline, the disc records, the trig table — is pure
    // in (p, eff_h) and replayed by every lane; only emission is strided.
    let slot = wid.x;
    let lane = lid.x;
    if (slot >= CMG_MAX_SLOTS) { return; }

    let p = cmg_params[slot];
    // THE INDEX CHANNEL (MOSAIC_1): every vertex of this body carries
    // enc — legacy plain-slot meshes decode identically (seed 0).
    let enc = p.mosaic_seed * 64u + slot;
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
        for (var i = lane; i < CMG_MAX_INDICES_PER_SLOT; i += MESHGEN_LANES) {
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

    // ── THE SECTION BASES (LATTICE_2) ──────────────────────────
    // The revolution is a rectangle, so its totals are closed forms. The
    // DISCS are not — a disc is a fan or a strip depending on its inner
    // radius — so a lane recovers its own disc's base with a pure prefix
    // loop over the skeleton: the same arithmetic the cursor performed,
    // no emission. pc >= 1 by construction (both profile branches emit at
    // least one point), but the trip count is stated defensively: a u32
    // `pc - 1u` at pc == 0 would be 4 billion, not zero.
    let pc_quads    = select(pc - 1u, 0u, pc == 0u);   // the wall loop's inner trip count
    let rev_verts   = (sa + 1u) * pc;
    let rev_indices = sa * pc_quads * 6u;

    // ── Revolution: rotate profile around Y axis ───────────────
    // Vertices: (sa+1) rings × pc points. vi was slot_vb + si * pc + pi.
    for (var si = lane; si <= sa; si += MESHGEN_LANES) {
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

            cmg_write_vertex(slot_vb + si * pc + pi,
                p.center_x + r * ct, y, p.center_z + r * st,
                nx_local * ct, ny_local, nx_local * st,
                prof_cr[pi], prof_cg[pi], prof_cb[pi], enc);
        }
    }

    // Wall indices: quads between adjacent rings.
    // ii was slot_ib + (si * pc_quads + pi) * 6u + j.
    let rev_vb = slot_vb;
    for (var si = lane; si < sa; si += MESHGEN_LANES) {
        for (var pi = 0u; pi + 1u < pc; pi++) {
            let i00 = rev_vb + si * pc + pi;
            let i10 = i00 + 1u;
            let i01 = rev_vb + (si + 1u) * pc + pi;
            let i11 = i01 + 1u;
            let ii = slot_ib + (si * pc_quads + pi) * 6u;
            cmg_indices[ii + 0u] = i00;
            cmg_indices[ii + 1u] = i01;
            cmg_indices[ii + 2u] = i10;
            cmg_indices[ii + 3u] = i10;
            cmg_indices[ii + 4u] = i01;
            cmg_indices[ii + 5u] = i11;
        }
    }

    // ── Horizontal discs (using precomputed trig table) ────────
    //
    // A LANE OWNS A WHOLE DISC. That is why the body below is verbatim,
    // `vi++` / `ii++` and all: within one disc the cursor is ordinary
    // serial state, and no other lane is writing into this disc's range.
    // Only the disc's STARTING cursor has to be recovered, which the
    // prefix loop does.
    for (var di = lane; di < dc; di += MESHGEN_LANES) {
        // The cursor's value on reaching disc di: the revolution's whole
        // rectangle, plus every earlier disc's own shape. Reads only the
        // skeleton, writes nothing.
        var dv = rev_verts;
        var dii = rev_indices;
        for (var k = 0u; k < di; k++) {
            if (disc_ri[k] < 0.001) { dv += 1u + (sa + 1u); dii += sa * 3u; }
            else                    { dv += 2u * (sa + 1u); dii += sa * 6u; }
        }
        var vi = slot_vb + dv;
        var ii = slot_ib + dii;

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
                d_cr, d_cg, d_cb, enc);
            vi++;

            for (var si = 0u; si <= sa; si++) {
                cmg_write_vertex(vi,
                    p.center_x + d_ro * tbl_cos[si], d_y, p.center_z + d_ro * tbl_sin[si],
                    0.0, d_ny, 0.0,
                    d_cr, d_cg, d_cb, enc);
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
                    d_cr, d_cg, d_cb, enc);
                vi++;

                cmg_write_vertex(vi,
                    p.center_x + d_ro * ct, d_y, p.center_z + d_ro * st,
                    0.0, d_ny, 0.0,
                    d_cr, d_cg, d_cb, enc);
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
    // `used` was ii - slot_ib, which no lane holds any more: the same
    // prefix run to dc instead of to di.
    var used = rev_indices;
    for (var k = 0u; k < dc; k++) {
        if (disc_ri[k] < 0.001) { used += sa * 3u; } else { used += sa * 6u; }
    }
    for (var i = used + lane; i < CMG_MAX_INDICES_PER_SLOT; i += MESHGEN_LANES) {
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

@group(2) @binding(180) var<storage, read>       palmg_params: array<PalmMeshParams, 24>;
@group(2) @binding(181) var<storage, read_write>  palmg_vertices: array<f32>;
@group(2) @binding(182) var<storage, read_write>  palmg_indices: array<u32>;

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

@compute @workgroup_size(MESHGEN_LANES)
fn palm_mesh_gen(
    @builtin(workgroup_id) wid: vec3<u32>,
    @builtin(local_invocation_id) lid: vec3<u32>
) {
    // ONE WORKGROUP PER SLOT (LATTICE_2). Dispatch shape unchanged.
    let slot = wid.x;
    let lane = lid.x;
    if (slot >= PALMG_MAX_SLOTS) { return; }

    let p = palmg_params[slot];
    let vb_base = slot * PALMG_MAX_VERTS_PER_SLOT;
    let ib_base = slot * PALMG_MAX_INDICES_PER_SLOT;

    if (p.is_active == 0u) {
        for (var i = lane; i < PALMG_MAX_INDICES_PER_SLOT; i += MESHGEN_LANES) {
            palmg_indices[ib_base + i] = vb_base;
        }
        return;
    }

    let cx = p.center_x;
    let cz = p.center_z;
    let burial = p.burial;
    let lean_cos = cos(p.lean_dir);
    let lean_sin = sin(p.lean_dir);

    // ── TRUNK: surface of revolution with taper + bark rings + lean ──

    let trunk_rings = min(u32(max(8.0, p.bark_rings)), 40u);
    let trunk_segs = min(p.trunk_segs, 24u);

    // THE SECTION BASES (LATTICE_2). Hoisted from the frond block, where
    // they were already named for the ceiling arithmetic — the crown cap
    // needs them too now, and one number has one home. Every count here is
    // pure in (trunk_rings, trunk_segs), which are uniform per workgroup.
    let cap_tip_vi    = (trunk_rings + 1u) * trunk_segs;   // vi at the crown tip
    let cap_ring_vi   = cap_tip_vi + 1u;                   // vi at the crown ring
    let cap_fan_ii    = trunk_rings * trunk_segs * 6u;     // ii at the crown fan
    let trunk_verts   = cap_ring_vi + trunk_segs;
    let trunk_indices = cap_fan_ii + trunk_segs * 3u;

    // vi was ring * trunk_segs + seg.
    for (var ring = lane; ring <= trunk_rings; ring += MESHGEN_LANES) {
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

            palmg_write_vertex(vb_base + ring * trunk_segs + seg,
                cx + lean_x + ca * r, y, cz + lean_z + sa * r,
                ca, 0.0, sa,
                cr, cg, cb, slot);
        }
    }

    // Trunk indices: quads between consecutive rings.
    // ii was (ring * trunk_segs + seg) * 6u + j.
    for (var ring = lane; ring < trunk_rings; ring += MESHGEN_LANES) {
        for (var seg = 0u; seg < trunk_segs; seg++) {
            let next_seg = (seg + 1u) % trunk_segs;
            let row0 = ring * trunk_segs;
            let row1 = (ring + 1u) * trunk_segs;

            let v00 = vb_base + row0 + seg;
            let v10 = vb_base + row1 + seg;
            let v11 = vb_base + row1 + next_seg;
            let v01 = vb_base + row0 + next_seg;

            let ii = ib_base + (ring * trunk_segs + seg) * 6u;
            palmg_indices[ii + 0u] = v00;
            palmg_indices[ii + 1u] = v10;
            palmg_indices[ii + 2u] = v11;
            palmg_indices[ii + 3u] = v00;
            palmg_indices[ii + 4u] = v11;
            palmg_indices[ii + 5u] = v01;
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

    // ONE VERTEX, ONE LANE: the tip is not a loop, so lane 0 writes it.
    if (lane == 0u) {
        palmg_write_vertex(vb_base + cap_tip_vi,
            cx + crown_lean_x, crown_y + crown_r * 0.6, cz + crown_lean_z,
            0.0, 1.0, 0.0,
            crown_cr, crown_cg, crown_cb, slot);
    }

    for (var seg = lane; seg < trunk_segs; seg += MESHGEN_LANES) {
        let angle = f32(seg) / f32(trunk_segs) * 2.0 * PI;
        palmg_write_vertex(vb_base + cap_ring_vi + seg,
            cx + crown_lean_x + cos(angle) * crown_r,
            crown_y,
            cz + crown_lean_z + sin(angle) * crown_r,
            0.0, 1.0, 0.0,
            crown_cr, crown_cg, crown_cb, slot);
    }

    for (var seg = lane; seg < trunk_segs; seg += MESHGEN_LANES) {
        let next_seg = (seg + 1u) % trunk_segs;
        let ii = ib_base + cap_fan_ii + seg * 3u;
        palmg_indices[ii + 0u] = vb_base + cap_tip_vi;
        palmg_indices[ii + 1u] = vb_base + cap_ring_vi + seg;
        palmg_indices[ii + 2u] = vb_base + cap_ring_vi + next_seg;
    }

    // ── FRONDS: radial quad strips with golden-angle packing ──

    let golden_angle = PI * (3.0 - sqrt(5.0));
    let frond_segs = min(p.frond_segs, 14u);

    // THE SLOT IS THE AUTHORITY. Trunk and crown are already written, so
    // the frond count is whatever the remaining vertex and index budgets
    // afford — never a per-family constant, because the cost is per-tier.
    // The authored floor of 3 yields to the ceiling: a floor that can
    // overrun the slot is not a guard. The two saturating min() calls keep
    // the subtraction total rather than dependent on the ring/seg clamps
    // above staying where they are. (`trunk_verts` / `trunk_indices` are
    // hoisted to the section-base block now; the crown cap needs them too.)
    let verts_left    = PALMG_MAX_VERTS_PER_SLOT   - min(trunk_verts,   PALMG_MAX_VERTS_PER_SLOT);
    let indices_left  = PALMG_MAX_INDICES_PER_SLOT - min(trunk_indices, PALMG_MAX_INDICES_PER_SLOT);
    let frond_ceiling = min(verts_left / ((frond_segs + 1u) * 2u),
                            indices_left / max(frond_segs * 6u, 1u));

    let n_fronds = min(u32(max(3.0, p.frond_count)), frond_ceiling);
    let crown_frond_y = crown_y + crown_r * 0.3;

    // A LANE OWNS A WHOLE FROND, so the body below is verbatim — the
    // per-frond cursor is ordinary serial state inside one lane's range.
    // Only the frond's starting cursor is arithmetic, and every frond is
    // the same size, so no prefix loop is needed.
    for (var f = lane; f < n_fronds; f += MESHGEN_LANES) {
        var vi = trunk_verts + f * (frond_segs + 1u) * 2u;
        var ii = trunk_indices + f * frond_segs * 6u;

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

    // Zero remaining indices (degenerate padding).
    // `used` was the final ii, which no lane holds any more.
    let used = trunk_indices + n_fronds * frond_segs * 6u;
    for (var i = used + lane; i < PALMG_MAX_INDICES_PER_SLOT; i += MESHGEN_LANES) {
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
    out.clip_pos = frame_r.vp.m * vec4(world_pos, 1.0);
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
    out.clip_pos = shadow_light_vp() * vec4(world_pos, 1.0);
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

@group(2) @binding(180) var<storage, read>       cactusg_params: array<CactusMeshParams, 20>;
@group(2) @binding(181) var<storage, read_write>  cactusg_vertices: array<f32>;
@group(2) @binding(182) var<storage, read_write>  cactusg_indices: array<u32>;

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

@compute @workgroup_size(MESHGEN_LANES)
fn cactus_mesh_gen(
    @builtin(workgroup_id) wid: vec3<u32>,
    @builtin(local_invocation_id) lid: vec3<u32>
) {
    // ONE WORKGROUP PER SLOT (LATTICE_2). Dispatch shape unchanged.
    // R3 LIVES HERE: the arm path accumulates (apx/apy/apz) down its
    // length, so the ARM loop is the strided one and a lane walks its whole
    // arm serially, exactly as the single thread did.
    let slot = wid.x;
    let lane = lid.x;
    if (slot >= CACTUSG_MAX_SLOTS) { return; }

    let p = cactusg_params[slot];
    let vb_base = slot * CACTUSG_MAX_VERTS_PER_SLOT;
    let ib_base = slot * CACTUSG_MAX_INDICES_PER_SLOT;

    if (p.is_active == 0u) {
        for (var i = lane; i < CACTUSG_MAX_INDICES_PER_SLOT; i += MESHGEN_LANES) {
            cactusg_indices[ib_base + i] = vb_base;
        }
        return;
    }

    let cx = p.center_x;
    let cz = p.center_z;
    let lean_cos = cos(p.lean_dir);
    let lean_sin = sin(p.lean_dir);

    let ribs = u32(max(4.0, p.ribs));
    let around = min(max(ribs * 2u, 12u), 20u);
    let trunk_steps = min(u32(p.trunk_segs), 20u);

    // THE SECTION BASES (LATTICE_2). Pure in (trunk_steps, around), both
    // uniform per workgroup. `top_ring_vi` was already named below for the
    // cap fan; it moves here so the trunk's totals derive from it.
    let top_ring_vi = trunk_steps * around;        // first vertex of the trunk's last ring
    let cap_tip_vi  = top_ring_vi + around;        // the single tip vertex
    let cap_fan_ii  = trunk_steps * around * 6u;   // ii at the cap fan

    // ── TRUNK: ribbed surface of revolution ──
    // vi was ring * around + seg.

    for (var ring = lane; ring <= trunk_steps; ring += MESHGEN_LANES) {
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

            cactusg_write_vertex(vb_base + ring * around + seg,
                cx + lx + ca * r, y, cz + lz + sa * r,
                ca, 0.0, sa, cr, cg, cb, slot);
        }
    }

    // Trunk indices — ii was (ring * around + seg) * 6u + j.
    for (var ring = lane; ring < trunk_steps; ring += MESHGEN_LANES) {
        for (var seg = 0u; seg < around; seg++) {
            let next_seg = (seg + 1u) % around;
            let row0 = ring * around;
            let row1 = (ring + 1u) * around;

            let ii = ib_base + (ring * around + seg) * 6u;
            cactusg_indices[ii + 0u] = vb_base + row0 + seg;
            cactusg_indices[ii + 1u] = vb_base + row1 + seg;
            cactusg_indices[ii + 2u] = vb_base + row1 + next_seg;
            cactusg_indices[ii + 3u] = vb_base + row0 + seg;
            cactusg_indices[ii + 4u] = vb_base + row1 + next_seg;
            cactusg_indices[ii + 5u] = vb_base + row0 + next_seg;
        }
    }

    // ── TRUNK CAP (stitched to top ring) ──

    let top_lean = p.lean * p.height;
    let cap_cx = cx + top_lean * lean_cos;
    let cap_cz = cz + top_lean * lean_sin;
    let cap_y = p.height;
    let cap_r = p.radius * p.taper * 0.6;
    let cap_col_r = p.body_r * 0.6 + p.rib_r * 0.4;
    let cap_col_g = p.body_g * 0.6 + p.rib_g * 0.4;
    let cap_col_b = p.body_b * 0.6 + p.rib_b * 0.4;

    // Single tip vertex above center — ONE VERTEX, so lane 0 writes it.
    if (lane == 0u) {
        cactusg_write_vertex(vb_base + cap_tip_vi,
            cap_cx, cap_y + cap_r * 0.6, cap_cz,
            0.0, 1.0, 0.0,
            cap_col_r, cap_col_g, cap_col_b, slot);
    }

    // Fan from tip to trunk's existing top ring — no separate cap ring
    for (var seg = lane; seg < around; seg += MESHGEN_LANES) {
        let next = (seg + 1u) % around;
        let ii = ib_base + cap_fan_ii + seg * 3u;
        cactusg_indices[ii + 0u] = vb_base + cap_tip_vi;
        cactusg_indices[ii + 1u] = vb_base + top_ring_vi + seg;
        cactusg_indices[ii + 2u] = vb_base + top_ring_vi + next;
    }

    // ── ARMS: ribbed columns along upward-curving paths ──

    let golden_angle = PI * (3.0 - sqrt(5.0));
    let arm_segs_u = min(u32(p.arm_segs), 12u);
    let arm_ribs = max(4u, ribs - 2u);
    let arm_around = min(max(arm_ribs * 2u, 8u), 12u);

    // THE SLOT IS THE AUTHORITY (mirrors the palm's frond ceiling). Every
    // other quantity feeding the arm loops is clamped — arm_segs_u,
    // arm_around, around, trunk_steps — and the TRIP COUNT was the one
    // thing that was not, while ARM_COUNT carries 1e30f as its parameter
    // ceiling. An unbounded loop writing into a fixed slot is a hole whose
    // probability is a property of the current table, not of the code: a
    // later table edit moves it with no warning.
    //
    // Costs are read from the loops, POST-F5. Trunk: rings are inclusive
    // (ring <= trunk_steps) over `around` segs, plus ONE top-cap tip that
    // fans to the existing top ring rather than emitting its own — which
    // is precisely the pattern the arm tip lacked until F5. Per arm: the
    // body rings are inclusive too, plus the single cap tip.
    // n_arms moves BELOW arm_segs_u and arm_around because the ceiling
    // depends on both. No floor to preserve — zero arms is a valid Finger.
    let trunk_verts   = cap_tip_vi + 1u;              // (trunk_steps+1)*around + the tip
    let trunk_indices = cap_fan_ii + around * 3u;     // the trunk quads + the cap fan
    let verts_per_arm   = (arm_segs_u + 1u) * arm_around + 1u;
    let indices_per_arm = arm_segs_u * arm_around * 6u + arm_around * 3u;
    let arm_verts_left   = CACTUSG_MAX_VERTS_PER_SLOT   - min(trunk_verts,   CACTUSG_MAX_VERTS_PER_SLOT);
    let arm_indices_left = CACTUSG_MAX_INDICES_PER_SLOT - min(trunk_indices, CACTUSG_MAX_INDICES_PER_SLOT);
    let arm_ceiling = min(arm_verts_left / max(verts_per_arm, 1u),
                          arm_indices_left / max(indices_per_arm, 1u));

    let n_arms = min(u32(max(0.0, p.arm_count)), arm_ceiling);

    // A LANE OWNS A WHOLE ARM (R3). The body below is verbatim — the
    // apx/apy/apz path walk accumulates down the arm, and it accumulates
    // inside one lane exactly as it did inside the one thread. Every arm
    // is the same size (arm_segs_u and arm_around are per-slot), so the
    // base is a product, not a prefix.
    for (var a = lane; a < n_arms; a += MESHGEN_LANES) {
        var vi = trunk_verts + a * verts_per_arm;
        var ii = trunk_indices + a * indices_per_arm;

        let arm_az = f32(a) * golden_angle + cactus_hash(p.seed, 1050u + a) * 0.5;
        let fork_frac = p.arm_height + (cactus_hash(p.seed, 1060u + a) - 0.5) * 0.15;
        let fork_y = p.height * fork_frac;
        let arm_len = p.arm_length * (0.8 + cactus_hash(p.seed, 1070u + a) * 0.4);
        let arm_r = p.arm_radius * (0.85 + cactus_hash(p.seed, 1080u + a) * 0.3);

        let lean_at_fork = p.lean * p.height * fork_frac * fork_frac;
        // The fork rides the trunk's FULL lean. The trunk centre at
        // fork_y is displaced by lean_at_fork; taking 0.3 of it left the
        // arm growing out of a point the trunk is not at, and the error
        // scales with lean. The designer's 2D preview applies the whole
        // offset, and the designer is the shape authority.
        let fork_x = cx + cos(arm_az) * p.radius * p.taper * 0.9 + lean_at_fork * lean_cos;
        let fork_z = cz + sin(arm_az) * p.radius * p.taper * 0.9 + lean_at_fork * lean_sin;

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

            // THE ARM'S PLANE IS FIXED. The path lies entirely in the
            // vertical plane spanned by out = (cos az, 0, sin az) and world
            // up, so ONE horizontal perpendicular serves every ring. The
            // runtime branch it replaces flipped the reference axis mid-arm
            // whenever |ndy| passed 0.95 — which arm_curve mu 1.00 now
            // reaches around t ~ 0.75 (the old designer defaults peaked at
            // 0.949, one hundredth below it). out is already unit, so this
            // is too: no re-normalisation.
            let rx = out_z;
            let rz = -out_x;
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

                // The normal takes the SAME basis as the position above.
                // (ca, 0, sa) is the ring's local parameter, not a world
                // direction — correct for the trunk, whose rings are
                // horizontal circles about a vertical axis, and wrong here,
                // where the ring lives in (r, f). r and f are orthonormal by
                // construction (f = r x nd, both unit), so this is unit and
                // needs no normalisation.
                cactusg_write_vertex(vb_base + vi,
                    vx, vy, vz,
                    rx * ca + fx * sa, fy * sa, rz * ca + fz * sa,
                    cr, cg, cb, slot);
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

        // ── Arm cap: the tip fans DIRECTLY to the last body ring ──
        // The cap used to emit its OWN ring at arm_r * 0.6 while the last
        // body ring sat at arm_r * 0.7 with rib modulation — two concentric
        // rings at the same height, unstitched, leaving an open annulus all
        // the way round every arm tip. Fanning to the body ring closes it
        // and deletes the ring's vertices outright.
        //
        // THE FAN USES arm_around, NOT the old min(arm_around, 8u). Those
        // two differ at every tier's mu (arm_around 12 against a cap of 8),
        // so a fan over the cap count would have skipped a third of the
        // ring it is stitching to.
        //
        // Winding is taken from the body's own quad, not guessed: the body
        // writes (row0+seg, row1+seg, row1+next) then (row0+seg, row1+next,
        // row0+next). The tip plays row1, so the first triangle degenerates
        // and the second is what survives — (last+seg, tip, last+next). That
        // is the OPPOSITE cyclic order from the deleted cap fan, which wound
        // against its own separate ring.
        let arm_cap_r = arm_r * 0.6;
        let arm_cap_tip = vi;
        cactusg_write_vertex(vb_base + vi,
            apx, apy + arm_cap_r * 0.6, apz,
            0.0, 1.0, 0.0,
            cap_col_r, cap_col_g, cap_col_b, slot);
        vi++;

        let arm_last_ring = arm_vi_start + arm_segs_u * arm_around;
        for (var seg = 0u; seg < arm_around; seg++) {
            let next = (seg + 1u) % arm_around;
            cactusg_indices[ib_base + ii] = vb_base + arm_last_ring + seg;  ii++;
            cactusg_indices[ib_base + ii] = vb_base + arm_cap_tip;          ii++;
            cactusg_indices[ib_base + ii] = vb_base + arm_last_ring + next; ii++;
        }
    }

    // Zero remaining indices — `used` was the final ii, which no lane holds.
    let used = trunk_indices + n_arms * indices_per_arm;
    for (var i = used + lane; i < CACTUSG_MAX_INDICES_PER_SLOT; i += MESHGEN_LANES) {
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
    out.clip_pos = frame_r.vp.m * vec4(world_pos, 1.0);
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
    out.clip_pos = shadow_light_vp() * vec4(world_pos, 1.0);
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

@group(2) @binding(180) var<storage, read>       bladeg_params: array<BladeClusterMeshParams, 32>;
@group(2) @binding(181) var<storage, read_write>  bladeg_vertices: array<f32>;
@group(2) @binding(182) var<storage, read_write>  bladeg_indices: array<u32>;

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

@compute @workgroup_size(MESHGEN_LANES)
fn blade_cluster_mesh_gen(
    @builtin(workgroup_id) wid: vec3<u32>,
    @builtin(local_invocation_id) lid: vec3<u32>
) {
    // ONE WORKGROUP PER SLOT (LATTICE_2). Dispatch shape unchanged.
    // The simplest of the five: ONE emission section, and every blade is
    // the same size, so a lane owns a blade and the base is a product.
    let slot = wid.x;
    let lane = lid.x;
    if (slot >= BLADEG_MAX_SLOTS) { return; }

    let p = bladeg_params[slot];
    let vb_base = slot * BLADEG_MAX_VERTS_PER_SLOT;
    let ib_base = slot * BLADEG_MAX_INDICES_PER_SLOT;

    if (p.is_active == 0u) {
        for (var i = lane; i < BLADEG_MAX_INDICES_PER_SLOT; i += MESHGEN_LANES) {
            bladeg_indices[ib_base + i] = vb_base;  // NOT 0u!
        }
        return;
    }

    let cx = p.center_x;
    let cz = p.center_z;
    let segs = max(3u, p.blade_segs);

    // THE SLOT IS THE AUTHORITY (mirrors the palm's frond ceiling and the
    // cactus arm's). n_blades was an unbounded trip count writing into a
    // fixed slot, and BLADE_COUNT carries 1e30f as its parameter ceiling,
    // so only the distribution's tail was holding it.
    //
    // Costs read from the loops: the vertex loop is INCLUSIVE (s <= segs)
    // and writes TWO verts per step; the index loop is exclusive and writes
    // six. There is NO base or root cost — vi and ii are still zero when
    // the blade loop opens, so the whole slot is the blade budget.
    //
    // NOTE for a future table edit: `segs` above has a floor but no
    // ceiling. It is safe today only because blade_segs is an authored
    // TIER SCALAR (5/6/7), not a sampled parameter — unlike blade_count.
    // The ceiling below is computed FROM segs, so it stays correct if that
    // ever changes; the guard that would still be missing is on segs itself.
    let verts_per_blade   = (segs + 1u) * 2u;
    let indices_per_blade = segs * 6u;
    let blade_ceiling = min(BLADEG_MAX_VERTS_PER_SLOT   / max(verts_per_blade, 1u),
                            BLADEG_MAX_INDICES_PER_SLOT / max(indices_per_blade, 1u));

    let n_blades = min(u32(max(2.0, p.blade_count)), blade_ceiling);
    let GA = PI * (3.0 - sqrt(5.0));

    // A LANE OWNS A BLADE: the body is verbatim, both inner loops and the
    // per-blade cursor included. `verts_per_blade` / `indices_per_blade`
    // were already named above for the ceiling; they are the stride too.
    for (var b = lane; b < n_blades; b += MESHGEN_LANES) {
        var vi = b * verts_per_blade;
        var ii = b * indices_per_blade;

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

    // Fill remaining indices with vb_base (NOT 0u!).
    // `used` was the final ii, which no lane holds any more.
    let used = n_blades * indices_per_blade;
    for (var i = used + lane; i < BLADEG_MAX_INDICES_PER_SLOT; i += MESHGEN_LANES) {
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
    out.clip_pos = frame_r.vp.m * vec4(world_pos, 1.0);
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
    out.clip_pos = shadow_light_vp() * vec4(world_pos, 1.0);
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
//   Compute (orb_init, orb_dynamics, orb_recolor) — ORBS_A face set (g2):
//     @binding(120) orb_state         storage, read_write
//     @binding(121) orb_config        uniform
//     @binding(122) orb_state_prev    storage, read
//   Compute (orb_state_prev_copy) — ORBS_B face set (g2), inverse access:
//     @binding(123) orb_state_ro      storage, read       (→ orb_state)
//     @binding(124) orb_state_prev_rw storage, read_write (→ orb_state_prev)
//   Render (orb_vs, orb_fs) — FRAME stratum (g1):
//     @binding(3)   frame_r.vp         (already declared)
//     @binding(4)   frame_r.camera     (already declared)
//   ORB_V: the orb state itself is no longer a binding here. It rides an
//   instance-step VERTEX BUFFER (renderer.hpp, orbStateVBL), one attribute
//   per field, and orb_vs rebuilds the struct from its @location inputs.

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

@group(2) @binding(120) var<storage, read_write> orb_state: array<OrbState>;
@group(2) @binding(121) var<uniform> orb_config: OrbConfig;
// Previous-frame snapshot (read-only view in main layout). Written
// by orb_state_prev_copy before each frame's dynamics dispatch so
// flocking can query neighbors against a stable previous frame.
@group(2) @binding(122) var<storage, read> orb_state_prev: array<OrbState>;
// Inverse-access views used only by orb_state_prev_copy. They
// reference the same physical buffers through a dedicated copy
// layout. WebGPU requires each shader declaration to match exactly
// one layout access mode, so 410/412 (bound read_write/read in the
// main layout) can't be re-used here with swapped access modes.
@group(2) @binding(123) var<storage, read>       orb_state_ro:      array<OrbState>;
@group(2) @binding(124) var<storage, read_write> orb_state_prev_rw: array<OrbState>;


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
// structs cheaply; the explicit ifs are a uniform-bounded chain over a
// fixed pocket count.
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
// (tier_count, per-invocation tier_idx). The pattern is mechanical;
// it's kept in this compact form so the obvious repetition doesn't
// dominate the file.

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
// Uses the dedicated copy layout's bindings (123 read, 124 read_write)
// rather than 120/122, which are bound read_write/read in the main
// layout — see the binding-layout comment above.
// Swap-two-groups alternative DECLINED under L23' (DOMESDAY_1 R1).
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
    // the same path, so the branch never diverges.
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

        let ali_r2 = orb_config.flock_align_radius * orb_config.flock_align_radius;
        let coh_r2 = orb_config.flock_coh_radius   * orb_config.flock_coh_radius;

        // FIELD_B3: separation calls the ONE LAW in place (field_pair,
        //  module-scope) — the succession's second transport: the
        //  buffer where classes must hear each other, the direct call
        //  where a subsystem is closed (orb↔orb only; the snapshot
        //  keeps it race-free). Radius mapping keeps the mood dial:
        //  shell = 2r·config.field_slack = flock_sep_radius exactly — and the
        //  shell gives separation a TRUE finite reach where 1/d² had an
        //  infinite tail behind a gate. Magnitude ownership unchanged:
        //  the sum is normalized below, the weight chain governs.
        let sep_pair_r = orb_config.flock_sep_radius / (2.0 * config.field_slack);

        var sep_sum   = vec3<f32>(0.0, 0.0, 0.0);
        var ali_sum   = vec3<f32>(0.0, 0.0, 0.0);
        var coh_sum   = vec3<f32>(0.0, 0.0, 0.0);
        var ali_count = 0.0;
        var coh_count = 0.0;

        let n = orb_config.count;
        for (var j: u32 = 0u; j < n; j = j + 1u) {
            if (j == i) { continue; }
            let other = orb_state_prev[j];
            let diff  = orb.pos - other.pos;
            let d2    = dot(diff, diff);

            sep_sum = sep_sum + field_pair(orb.pos, other.pos,
                                           sep_pair_r, sep_pair_r,
                                           i, j);
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
        let sl = length(sep_sum);
        if (sl > 0.001) { sep_force = sep_sum / sl; }

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
    // ORB_V: OrbState arrives per-instance through an instance-step vertex
    // buffer (renderer.hpp, orbStateVBL — stride 80, offsets mirroring the
    // struct) instead of a VS storage binding. One @location per field, in
    // declaration order, so the value below is the one binding 400 used to
    // hand over. Demotion record: BINDING_LEDGER Table F.
    @location(1)  in_pos:           vec3<f32>,
    @location(2)  in_pad0:          f32,
    @location(3)  in_vel:           vec3<f32>,
    @location(4)  in_pad1:          f32,
    @location(5)  in_base_color:    vec3<f32>,
    @location(6)  in_brightness:    f32,
    @location(7)  in_current_color: vec3<f32>,
    @location(8)  in_twinkle_phase: f32,
    @location(9)  in_size:          f32,
    @location(10) in_mass:          f32,
    @location(11) in_drag:          f32,
    @location(12) in_tier_idx:      u32
) -> OrbVSOut {
    let orb = OrbState(in_pos, in_pad0, in_vel, in_pad1,
                       in_base_color, in_brightness,
                       in_current_color, in_twinkle_phase,
                       in_size, in_mass, in_drag, in_tier_idx);

    // Build world-space camera basis from azimuth/elevation — matches
    // build_view_projection_matrix conventions exactly.
    let az = frame_r.camera.azimuth;
    let el = frame_r.camera.elevation;
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
    // frame_r.camera is already in this VS (the billboard basis above);
    // orb_config.dome_center_* is dead, its bytes retained for ABI.
    let dome_center = frame_r.camera.pos;
    let world_pos = dome_center + orb.pos
        + cam_right * (quad_pos.x * orb.size)
        + cam_up    * (quad_pos.y * orb.size);

    var out: OrbVSOut;
    out.clip_pos = frame_r.vp.m * vec4<f32>(world_pos, 1.0);
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
