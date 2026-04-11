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
// ── Color Palettes (§2.2) ─────────────────────────────────────────
//   PALETTE_CENTER[4]             Sand/salmon/green/grey RGB
//   PALETTE_LIGHT[4]              Light variant per palette
//   PALETTE_VARIANCE[4]           Per-cell noise amplitude
//   PALETTE_WEIGHT[4]             Selection probability
//   COLOR_PAWN                    Pawn entity color
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
// ── Terrain-Mode Coupling (§2.2) ──────────────────────────────────
//   COUPLING_LATTICE_SPACING      250 wu — where coupling is active
//   COUPLING_STRENGTH_EXPONENT    3.0 — cubic: ~50% coupled
//   MODE_COUPLING_MAGNITUDE       0.25 — max mode shift
//   ARCHETYPE_MODE_CHARACTER[4]   Per-archetype coupling direction
//
// ── Terrain Waves (§1.6) ──────────────────────────────────────────
//   WAVE_THRESHOLD[6]             Per-band activity gate
//   ACTIVITY_LATTICE_SPACING      400 wu — activity envelope
//
// ── GoL Zones (§2.2, §7.0b) ──────────────────────────────────────
//   GOL_TIERS[7]                  Tier params (density, tick, spring)
//   PULSE_TIERS[3]                Pulse algorithm params
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
// §1    FOUNDATIONS         PGA algebra, trajectories, utilities, terrain height
// §2    STATE               Structs, constants, muting control
// §3    COUPLINGS           Signal/input/terrain/entity cross-wiring
// §4    DYNAMICS            PGA motor integration (pawn, camera)
// §5    COMPOSITION         0D update split across compute entry points
// §6    RENDERING           Lighting, terrain VS/FS, entity VS/FS, ribbon, shadows
// §7    COMPUTE             Bindings, entry points, GoL zones, pawn aura
// §8    GALLERY             Photographer, terrain paintings, wall paintings
// §9    ENTITY MESH GEN     GPU-sovereign geometry: pyramids, arches, columns
//
// Subsystem-specific bindings live with their consumers (§7, §8, §9).
// Global bindings (signal, config, VP, render mirrors, lights) are in §7.0.

// §1 FOUNDATIONS
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

// (reverse_m removed — re-add from PGA reference when needed)

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

// (verify_motor_norm removed — diagnostic function, never called)


// §1.2 TRAJECTORY PRIMITIVES

struct Trajectory {
    value: f32,
    velocity: f32,
    _pad0: f32,
    _pad1: f32,
}

fn trajectory_release(t: Trajectory, goal: f32, dt: f32, rate: f32) -> Trajectory {
    let new_val = t.value + (goal - t.value) * (1.0 - exp(-rate * dt));
    return Trajectory(new_val, 0.0, 0.0, 0.0);
}


// §1.3 COORDINATE SYSTEMS

// (legacy chart constants removed — CHART_EXTENT, CHART_HEIGHT_RESOLUTION, CHART_SURFACE_COLOR_RESOLUTION)

// Terrain mesh — grid subdivisions along each axis.
// Vertex shader receives deduplicated vertex ID via index buffer (GPU-generated once).
const TERRAIN_MESH_N: u32 = 256u;
const TERRAIN_MESH_STRIDE: u32 = TERRAIN_MESH_N + 1u;  // vertices per row (fence posts)

// Patch system — streaming terrain
const PATCH_HEIGHTFIELD_N: u32 = 256u;  // texels per patch heightfield side
const PATCH_CELL_N: u32 = 16u;          // cell color texture side per patch
const PATCH_EXTENT: f32 = 50.0;         // world units per patch side
const PATCH_MESH_N: u32 = 64u;          // mesh subdivisions per patch (VS bilinear-samples 256-texel heightfield)
const PATCH_MESH_STRIDE: u32 = PATCH_MESH_N + 1u;

// (legacy chart_uv_to_world, chart_height_texel_to_world, chart_surface_color_texel_to_world removed)
// (legacy CELL_GRID_SIZE, MAX_CELL_GRID_SIZE, proximity_field_index removed)


// §1.4 UTILITIES

// (hsv_to_rgb and rgb_to_hsv removed — unused color space conversions)

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

// (legacy cell-based random functions removed: hash_cell_id, random_float,
//  random_float_range, random_vec3, random_vec3_range — replaced by hash_property system)

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


// SPATIAL FIELD MANIFEST
const ACTIVITY_LATTICE_SPACING: f32 = 400.0;    // world units between activity nodes
const ACTIVITY_SEED_BAND: u32       = 50u;      // lattice seed band (separate from terrain)
const ACTIVITY_PROP_LEVEL: u32      = 220u;     // property index: activity intensity
const ACTIVITY_PROP_BEAT_FREQ: u32  = 221u;     // property index: beat frequency
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

fn band_activity_level(raw_activity: f32, band_index: u32) -> f32 {
    let threshold = WAVE_THRESHOLD[band_index];
    return smoothstep(threshold, threshold + WAVE_THRESHOLD_SOFTNESS, raw_activity);
}

// Evaluate the activity field at a world position.
// Returns vec2(activity [0,1], beat_freq [0.25, 2.0] cycles/beat).
fn terrain_activity_at(world_xz: vec2<f32>, master_seed: u32) -> vec2<f32> {
    let lattice_pos = world_xz / ACTIVITY_LATTICE_SPACING;
    let lattice_base = vec2<i32>(floor(lattice_pos));
    let frac = fract(lattice_pos);
    let w = frac * frac * (3.0 - 2.0 * frac);

    var activity: f32 = 0.0;
    var beat_freq: f32 = 0.0;

    for (var dz: i32 = 0; dz <= 1; dz++) {
        for (var dx: i32 = 0; dx <= 1; dx++) {
            let node = lattice_base + vec2<i32>(dx, dz);
            let seed = lattice_node_seed(master_seed, node, ACTIVITY_SEED_BAND);

            let node_activity = hash_property(seed, ACTIVITY_PROP_LEVEL);
            let node_beat_freq = ACTIVITY_BEAT_FREQ_LO
                * pow(ACTIVITY_BEAT_FREQ_HI / ACTIVITY_BEAT_FREQ_LO, hash_property(seed, ACTIVITY_PROP_BEAT_FREQ));

            let wx = select(1.0 - w.x, w.x, dx == 1);
            let wz = select(1.0 - w.y, w.y, dz == 1);
            let weight = wx * wz;

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
fn evaluate_lattice_wave(
    world_xz: vec2<f32>,
    node: vec2<i32>,
    node_seed: u32,
    band: TerrainBand,
    band_act: f32,       // activity level for this band [0,1] (from hierarchy)
    beat_freq: f32,      // cycles per beat from activity field
    t_beats: f32,        // current time in beats
) -> f32 {
    // Activation gate: if draw exceeds band activation, this node is silent.
    // Spatially coherent — a silent node is silent for all query points.
    if (hash_property(node_seed, WAVE_PROP_ACTIVE) > band.activation) {
        return 0.0;
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
        return mix(val_frozen, val_moving, band_act);
    } else {
        let dir_angle = hash_property(node_seed, WAVE_PROP_DIR_ANGLE) * 2.0 * PI;
        let dir = vec2(cos(dir_angle), sin(dir_angle));
        let val_frozen = evaluate_directional_wave(world_xz, node_world, freq, amp, damping, dir, phase_frozen);
        let val_moving = evaluate_directional_wave(world_xz, node_world, freq, amp, damping, dir, phase_moving);
        return mix(val_frozen, val_moving, band_act);
    }
}

// --- Polyphony-driven band motion accessors
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

// --- Total Height
fn terrain_height_at(world_xz: vec2<f32>, master_seed: u32, t_beats: f32) -> f32 {
    let af = terrain_activity_at(world_xz, master_seed);
    let raw_activity = af.x;
    let beat_freq = af.y;

    var total: f32 = 0.0;
    for (var b: u32 = 0u; b < TERRAIN_BAND_COUNT; b++) {
        total += terrain_band_contribution(world_xz, master_seed, t_beats, b, raw_activity, beat_freq).x;
    }
    return total;
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

// (terrain_height_with_gradients removed — replaced by effective_ground_with_gradients)

// (terrain_height_gradients_and_complexity removed — patch gen now uses
//  ground_formed with finite differences directly)


// §2 STATE

// §2.1 STRUCTS

// --- [STATE:signal] FrameSignal

struct FrameSignal {
    t_seconds: f32,
    t_beats: f32,
    dt: f32,
    aspect_ratio: f32,
    stats: array<f32, 64>,
    move_x: f32,
    move_z: f32,
    look_az_delta: f32,
    look_el_delta: f32,
    zoom_delta: f32,
    pan_x_delta: f32,
    pan_y_delta: f32,
    _pad1: f32,
}

// --- [STATE:terrain] TerrainState

struct TerrainState {
    amplitude_scale: f32,
    max_amplitude: f32,
    size: f32,
    lipschitz_factor: f32,
    tint: vec3<f32>,
    _pad: f32,
}

// --- [STATE:pawn] PawnState

struct PawnState {
    pos: vec3<f32>,
    heading: f32,
    orientation: vec4<f32>,
    velocity: vec2<f32>,      // Current XZ velocity (for force field radius)
    portal_trigger: i32,      // -1 = none, >=0 = arch index triggering transition
    _pad1: f32,
}

// --- [STATE:camera] CameraState

struct CameraState {
    pos: vec3<f32>,
    azimuth: f32,
    elevation: f32,
    distance: f32,
    pan_x: f32,
    pan_y: f32,
}

// --- [STATE:sphere] SphereState

struct SphereState {
    pos: vec3<f32>,
    radius: f32,
    orientation: vec4<f32>,
    influence_radius: f32,
    t: f32,
    orbit_radius: f32,
    orbit_speed: f32,
    color: vec3<f32>,
    orbit_height: f32,
    anchor: vec3<f32>,
    color_mode: f32,
    base_color: vec3<f32>,
    _pad0: f32,
}

// --- [STATE:ribbon] RibbonState
struct RibbonState {
    anchor: vec3<f32>,      // world-space center of the ribbon
    time: f32,              // animation time (seconds)
    cube_count: u32,        // number of cross-section rings along the tube
    cube_size: f32,         // cross-section side length (ring spacing = cube_size)
    height: f32,            // base height above terrain
    twist_amp: f32,         // amplitude of corkscrew twist
    color: vec3<f32>,       // ribbon color
    lateral_amp: f32,       // lateral wave amplitude (XZ plane sway)
    lateral_cycles: f32,    // lateral oscillation cycles along ribbon
    lateral_speed: f32,     // lateral time multiplier
    vertical_amp: f32,      // vertical wave amplitude (world Y)
    vertical_cycles: f32,   // vertical oscillation cycles along ribbon
    vertical_speed: f32,    // vertical time multiplier
    twist_cycles: f32,      // corkscrew cycles along ribbon
    twist_speed: f32,       // twist time multiplier
    is_visible: u32,        // 0 = hidden, 1 = flying
    orientation: f32,       // heading angle (radians, 0 = +X axis)
    color_mode: u32,        // 0=smooth, 1=tinted, 2=contrast
    _pad0: f32,
    _pad1: f32,
}

// Pre-computed per-ring transform (compute pass → VS + pawn overlay)
struct RibbonRingTransform {
    motor_p0: vec4<f32>,    // PGA motor rotor part
    motor_p1: vec4<f32>,    // PGA motor translator part
    center: vec3<f32>,      // ring world-space center (extracted from motor)
    terrain_y: f32,         // tile-modified terrain height at center XZ
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

struct TileGrid {
    origin_x: i32,         // grid-space X of entry [0][0]
    origin_z: i32,         // grid-space Z of entry [0][0]
    side: u32,             // grid dimension (up to 17)
    cell_extent: f32,      // world units per cell (50.0)
    entries: array<TileGridEntry, 289>,
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
// The dominant palette gets ~0.80, remainder distributed.
fn palette_weights_at_node(node: vec2<i32>) -> vec4<f32> {
    let seed = color_lattice_seed(node, 0u);
    let roll = hash_property(seed, 500u);

    // Cumulative weight selection from PALETTE_WEIGHT array
    var dominant: u32 = 3u;
    var cumul: f32 = 0.0;
    for (var i: u32 = 0u; i < 4u; i++) {
        cumul += PALETTE_WEIGHT[i];
        if (roll < cumul) { dominant = i; break; }
    }

    // Build weights: dominant gets 0.85, rest share 0.05 each
    var w: vec4<f32>;
    if (dominant == 0u) {
        w = vec4(0.85, 0.05, 0.05, 0.05);
    } else if (dominant == 1u) {
        w = vec4(0.05, 0.85, 0.05, 0.05);
    } else if (dominant == 2u) {
        w = vec4(0.05, 0.05, 0.85, 0.05);
    } else {
        w = vec4(0.05, 0.05, 0.05, 0.85);
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

// Hermite-interpolated mode tendency at a world position. [0,1]
fn mode_field_at(world_xz: vec2<f32>) -> f32 {
    let lc = lattice_coord(world_xz, MODE_LATTICE_SPACING);
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
fn transition_style_at(world_xz: vec2<f32>) -> f32 {
    let lc = lattice_coord(world_xz, TRANSITION_LATTICE_SPACING);
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
    // Base: broad sparse tendency
    let lc_b = lattice_coord(world_xz, SPARSE_BASE_SPACING);
    var base_val: f32 = 0.0;
    for (var dz: i32 = 0; dz <= 1; dz++) {
        for (var dx: i32 = 0; dx <= 1; dx++) {
            base_val += sparse_base_at_node(lc_b.base + vec2(dx, dz)) * lattice_weight(lc_b, dx, dz);
        }
    }

    // Cluster: small-scale density modulation within sparse regions
    let lc_c = lattice_coord(world_xz, SPARSE_CLUSTER_SPACING);
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

// --- Terrain-mode coupling field
//
// Two per-node rolls on a coarse lattice:
//   strength: how much terrain influences mode here (0 = independent, 1 = fully coupled)
//   direction: which way the coupling pushes (-1 = mountains→smooth, +1 = mountains→discrete)
//
// The result is Hermite-interpolated so coupling zones have soft edges.
// Combined with ARCHETYPE_MODE_CHARACTER in evaluate_cell_fields to shift the mode value.

fn coupling_strength_at_node(node: vec2<i32>) -> f32 {
    let seed = color_lattice_seed(node, 15u);
    let raw = hash_property(seed, 530u);
    // Cubic bias: raw^3 — about half the world has meaningful coupling.
    // Tunable via COUPLING_STRENGTH_EXPONENT.
    return pow(raw, COUPLING_STRENGTH_EXPONENT);
}

fn coupling_direction_at_node(node: vec2<i32>) -> f32 {
    let seed = color_lattice_seed(node, 16u);
    // Map [0,1] → [-1,1]. Trimodal snap: commit to a direction per region.
    let raw = hash_property(seed, 531u);
    if (raw < 0.35) { return -1.0; }  // 35% mountains → smooth
    if (raw > 0.65) { return 1.0; }   // 35% mountains → discrete
    return 0.0;                         // 30% no directional coupling (strength still applies as damping)
}

// Returns vec2(strength, direction) at a world position.
fn terrain_coupling_at(world_xz: vec2<f32>) -> vec2<f32> {
    let lc = lattice_coord(world_xz, COUPLING_LATTICE_SPACING);
    var strength: f32 = 0.0;
    var direction: f32 = 0.0;
    for (var dz: i32 = 0; dz <= 1; dz++) {
        for (var dx: i32 = 0; dx <= 1; dx++) {
            let w = lattice_weight(lc, dx, dz);
            strength += coupling_strength_at_node(lc.base + vec2(dx, dz)) * w;
            direction += coupling_direction_at_node(lc.base + vec2(dx, dz)) * w;
        }
    }
    return vec2(strength, direction);
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

// Smooth palette color: weighted blend modulated by complexity only.
// No per-cell noise — produces continuous gradients.
fn palette_color_smooth(weights: vec4<f32>, complexity: f32) -> vec3<f32> {
    var color = vec3(0.0);
    let w = array<f32, 4>(weights.x, weights.y, weights.z, weights.w);
    for (var i: u32 = 0u; i < 4u; i++) {
        color += mix(PALETTE_LIGHT[i], PALETTE_CENTER[i], complexity) * w[i];
    }
    return clamp(color, vec3(0.0), vec3(1.0));
}

// Discrete palette color: weighted blend with per-cell random offset.
// Each cell gets a unique noise contribution for mosaic variety.
fn palette_color(weights: vec4<f32>, complexity: f32, cell_seed: u32) -> vec3<f32> {
    // Per-cell noise direction (shared across all palettes)
    let r0 = hash_property(cell_seed, 600u) - 0.5;  // [-0.5, 0.5]
    let r1 = hash_property(cell_seed, 601u) - 0.5;
    let r2 = hash_property(cell_seed, 602u) - 0.5;
    let noise = vec3(r0, r1, r2);

    var color = vec3(0.0);
    let w = array<f32, 4>(weights.x, weights.y, weights.z, weights.w);
    for (var i: u32 = 0u; i < 4u; i++) {
        let base = mix(PALETTE_LIGHT[i], PALETTE_CENTER[i], complexity);
        color += (base + noise * PALETTE_VARIANCE[i]) * w[i];
    }
    return clamp(color, vec3(0.0), vec3(1.0));
}

// --- Discrete cell color system
// (DISCRETE_*_LATTICE_SPACING defined in Color Field Spatial Config block)

// Per-node: roll RGB mean and variance for a discrete color region.
// Returns vec4(r_mean, g_mean, b_mean, variance).
fn discrete_region_at_node(node: vec2<i32>) -> vec4<f32> {
    let seed = color_lattice_seed(node, 10u);
    let r = hash_property(seed, 800u);
    let g = hash_property(seed, 801u);
    let b = hash_property(seed, 802u);
    // Variance: how much cells spread from the mean. [0.02 .. 0.25]
    let v = 0.02 + hash_property(seed, 803u) * 0.23;
    return vec4(r, g, b, v);
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
fn discrete_region_at(world_xz: vec2<f32>) -> vec4<f32> {
    let gpos = world_xz / DISCRETE_COLOR_LATTICE_SPACING;
    let gbase = vec2<i32>(floor(gpos));
    let frac = fract(gpos);
    let w = frac * frac * (3.0 - 2.0 * frac);

    var result = vec4(0.0);
    for (var dz: i32 = 0; dz <= 1; dz++) {
        for (var dx: i32 = 0; dx <= 1; dx++) {
            let val = discrete_region_at_node(gbase + vec2(dx, dz));
            let wx = select(1.0 - w.x, w.x, dx == 1);
            let wz = select(1.0 - w.y, w.y, dz == 1);
            result += val * wx * wz;
        }
    }
    return result;
}

// Interpolated monochrome tendency at a world position. [0,1]
fn discrete_mono_at(world_xz: vec2<f32>) -> f32 {
    let gpos = world_xz / DISCRETE_MONO_LATTICE_SPACING;
    let gbase = vec2<i32>(floor(gpos));
    let frac = fract(gpos);
    let w = frac * frac * (3.0 - 2.0 * frac);

    var result: f32 = 0.0;
    for (var dz: i32 = 0; dz <= 1; dz++) {
        for (var dx: i32 = 0; dx <= 1; dx++) {
            let val = discrete_mono_at_node(gbase + vec2(dx, dz));
            let wx = select(1.0 - w.x, w.x, dx == 1);
            let wz = select(1.0 - w.y, w.y, dz == 1);
            result += val * wx * wz;
        }
    }
    return result;
}

// Generate a discrete cell color from the region field + per-cell seed.
fn discrete_cell_color(world_xz: vec2<f32>, cell_gx: i32, cell_gz: i32, cell_seed: u32) -> vec3<f32> {
    // --- Chess board tier
    let chess = chess_field_at(world_xz);
    let chess_jitter = (hash_property(cell_seed, 815u) - 0.5) * 0.03;
    if (chess.tendency + chess_jitter > 0.45) {
        let parity = (cell_gx + cell_gz) & 1;
        // Colorful chess: only at the absolute peak of tendency.
        // Even among chess regions, colorful is the exception.
        if (chess.tendency > 0.65) {
            return select(chess.color_a, chess.color_b, parity == 1);
        }
        // Default chess: black and white.
        return select(vec3(0.03), vec3(0.95), parity == 1);
    }

    // --- Monochrome / tinted / full color tiers
    let region = discrete_region_at(world_xz);
    let rgb_mean = region.xyz;
    let variance = region.w;
    let mono = discrete_mono_at(world_xz);

    let mono_jitter = (hash_property(cell_seed, 820u) - 0.5) * 0.15;
    let effective_mono = mono + mono_jitter;

    if (effective_mono > 0.35) {
        // Pure black or white
        let bw_roll = hash_property(cell_seed, 830u);
        return select(vec3(0.02), vec3(0.95), bw_roll > 0.5);
    }

    if (effective_mono > 0.20) {
        // Tinted monochrome
        let bw_roll = hash_property(cell_seed, 830u);
        let base_grey = select(0.12, 0.85, bw_roll > 0.5);
        let tint_strength = 0.15;
        return mix(vec3(base_grey), rgb_mean, tint_strength);
    }

    // Full color
    let nr = (hash_property(cell_seed, 840u) - 0.5) * 2.0;
    let ng = (hash_property(cell_seed, 841u) - 0.5) * 2.0;
    let nb = (hash_property(cell_seed, 842u) - 0.5) * 2.0;
    let color = rgb_mean + vec3(nr, ng, nb) * variance;
    return clamp(color, vec3(0.0), vec3(1.0));
}

// Evaluate a cell's discrete color as if it belonged to a specific tier,
// bypassing the threshold cascade. Each tier uses the cell's own seeds/fields.
//   0 = full color       (rgb_mean + noise)
//   1 = tinted mono      (dark/light grey, tinted toward rgb_mean)
//   2 = pure B&W         (0.02 or 0.95)
//   3 = chess B&W        (0.03 or 0.95 by parity)
//   4 = chess colorful   (chess.color_a/b by parity)
fn discrete_cell_color_at_tier(
    world_xz: vec2<f32>, cell_gx: i32, cell_gz: i32, cell_seed: u32, tier: u32
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
            return mix(vec3(base_grey), region.xyz, 0.15);
        }
        default: {
            let region = discrete_region_at(world_xz);
            let nr = (hash_property(cell_seed, 840u) - 0.5) * 2.0;
            let ng = (hash_property(cell_seed, 841u) - 0.5) * 2.0;
            let nb = (hash_property(cell_seed, 842u) - 0.5) * 2.0;
            return clamp(region.xyz + vec3(nr, ng, nb) * region.w, vec3(0.0), vec3(1.0));
        }
    }
}

// (legacy SceneHit and RayHit structs removed — ray marching system)

// --- [STATE:config] DesignConfig

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
    wave_frozen_t: array<f32, 3>,
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
    // Per-band blend: < 0 = use activity field, [0,1] = direct mix factor.
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
    mode_color_shift: f32,        // [0,~0.6] bias on smooth→discrete mode field
    mode_checker_scatter: f32,    // [0,~0.5] reduction of sparse survival threshold
    mode_palette_target: f32,     // [0,3] target palette (0=sand 1=salmon 2=green 3=grey)
    mode_palette_intensity: f32,  // [0,1] drift strength toward target palette
    mode_discrete_tier: f32,      // [0,4] target discrete tier (0=color 1=tinted 2=BW 3=chessBW 4=chessColor)
    mode_gol_tick_scale: f32,     // tick period multiplier (1.0=normal, <1=faster)
    mode_gol_height_scale: f32,   // alive_height multiplier (1.0=normal, >1=taller)
    _pad_mode_3: f32,
    // ─── Radial pulse ring buffer ────────────────────────────────
    pulse_count: u32,
    _pulse_pad_0: f32,
    _pulse_pad_1: f32,
    _pulse_pad_2: f32,
    pulse_data: array<vec4<f32>, 8>,  // each: (origin_x, origin_z, onset_seconds, amplitude)
}

// §2.2 CONSTANTS

// --- Terrain constants

struct WaveComponent {
    amplitude: f32,
    frequency: f32,
    dir: vec2<f32>,
    period: f32,
}

const WAVES = array<WaveComponent, 3>(
    WaveComponent(1.0, 0.15, vec2(0.7, 0.7),   8.0),
    WaveComponent(0.5, 0.30, vec2(-0.5, 0.8),  6.0),
    WaveComponent(0.3, 0.50, vec2(0.9, -0.3),  4.0),
);
const WAVE_COUNT: i32 = 3;
const HEIGHT_MAX_AMPLITUDE: f32 = 2.0;
const IDLE_AMPLITUDE_SCALE: f32 = 1.0;
const AMPLITUDE_ATTACK_TIME: f32 = 10.0;
const AMPLITUDE_RELEASE_TIME: f32 = 5.0;
const SAND_DUNE_CENTER: vec3<f32> = vec3(0.85, 0.70, 0.50);
const SAND_DUNE_VARIANCE: f32 = 0.35;

// --- Terrain palette system
const PALETTE_CENTER = array<vec3<f32>, 4>(
    vec3(0.85, 0.70, 0.50),   // 0: sand    — warm Mars baseline
    vec3(0.88, 0.58, 0.48),   // 1: salmon  — pink-coral
    vec3(0.45, 0.58, 0.38),   // 2: green   — olive to jade (rare)
    vec3(0.82, 0.55, 0.42),   // 3: warm    — dusty terracotta
);
const PALETTE_LIGHT = array<vec3<f32>, 4>(
    vec3(0.92, 0.82, 0.65),   // 0: sand
    vec3(0.95, 0.72, 0.62),   // 1: salmon
    vec3(0.62, 0.72, 0.52),   // 2: green
    vec3(0.92, 0.72, 0.58),   // 3: warm
);
const PALETTE_VARIANCE = array<f32, 4>(
    0.08,                      // 0: sand    — tight
    0.14,                      // 1: salmon  — moderate
    0.20,                      // 2: green   — wide
    0.12,                      // 3: warm    — moderate
);
const PALETTE_WEIGHT = array<f32, 4>(
    0.42,                      // 0: sand
    0.28,                      // 1: salmon
    0.04,                      // 2: green   — rare
    0.26,                      // 3: warm    — common (was grey 0.07)
);

// --- Color Field Spatial Config
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

// ── Terrain-Mode Coupling ─────────────────────────────────────────
//
// Spatial coupling between terrain archetype and mode field.
// A separate lattice determines WHERE coupling is active.
// Where active, the local terrain archetype shifts the mode value:
//   - positive character + positive direction → more discrete (checkerboard on peaks)
//   - positive character + negative direction → more smooth (smooth peaks)
// The direction is per-region (stochastic), so mountains sometimes get
// checkerboard and sometimes stay smooth — but within a region, it's coherent.
//
// Where coupling strength is near zero, terrain and mode are fully independent
// (identical to the pre-coupling behavior).
//
//  ┌─────────────────────────────┬────────────────────────────────────────────┐
//  │ Constant                    │ Effect                                     │
//  ├─────────────────────────────┼────────────────────────────────────────────┤
//  │ COUPLING_LATTICE_SPACING    │ Zone size: larger = broader coupled areas  │
//  │ COUPLING_STRENGTH_EXPONENT  │ Rarity: higher = less coupling overall     │
//  │ MODE_COUPLING_MAGNITUDE     │ Intensity: max mode shift when coupled     │
//  │ ARCHETYPE_MODE_CHARACTER    │ Direction: which archetypes push how hard  │
//  └─────────────────────────────┴────────────────────────────────────────────┘

const COUPLING_LATTICE_SPACING: f32 = 250.0;      // ~5 patches — broad coupling zones
const COUPLING_STRENGTH_EXPONENT: f32 = 3.0;      // cubic — ~50% of world has meaningful coupling
const MODE_COUPLING_MAGNITUDE: f32 = 0.0;        // DISABLED — zero correlation between terrain and mode

// Per-archetype terrain character for coupling.
// Magnitude = how strongly this archetype pushes. Sign = direction.
//   Positive → when coupling direction > 0, pushes mode UP (toward discrete)
//   Negative → when coupling direction > 0, pushes mode DOWN (toward smooth)
// Varied is zero: neutral terrain never couples regardless of region.
const ARCHETYPE_MODE_CHARACTER = array<f32, 4>(
     1.0,    // 0: mountainous — strong push (direction-dependent)
     0.0,    // 1: varied      — neutral (never couples)
    -0.6,    // 2: basin       — moderate opposing push to mountains
    -0.3,    // 3: pool        — mild opposing push
);

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

// --- Sphere constants (radius, orbit, influence now live in SphereState)

const SPHERE_COLOR_RELEASE_RATE: f32 = 2.0;
const SPHERE_MIN_TERRAIN_CLEARANCE: f32 = 5.0;

// (legacy proximity field constants removed — binding 21 reserved)
// (legacy cell spring/color/random/height constants removed — binding 40 reserved)

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
}

const GOL_TIER_COUNT: u32 = 7u;

//                                                 dens_μ  σ     tick_μ  σ    spring_μ σ    trans_μ  σ     ht_μ    σ    sv    wt    no_h
const GOL_TIERS = array<GoLTierParams, 7>(
    /* 0: PILLARS  */ GoLTierParams(0.30, 0.05,   8.0, 2.0,   0.5, 0.1,   0.05, 0.01,  30.0, 9.0,  0.30,  0.10, 0u),
    /* 1: SPARSE   */ GoLTierParams(0.15, 0.05,   2.0, 0.5,   4.0, 1.0,   0.12, 0.03,  18.0, 6.0,  0.20,  0.20, 0u),
    /* 2: MODERATE */ GoLTierParams(0.30, 0.08,   1.0, 0.3,   8.0, 2.0,   0.15, 0.03,   9.0, 3.0,  0.15,  0.18, 0u),
    /* 3: DENSE    */ GoLTierParams(0.45, 0.10,   0.5, 0.15, 12.0, 3.0,   0.25, 0.05,   6.0, 1.5,  0.10,  0.10, 0u),
    /* 4: FLASH    */ GoLTierParams(0.35, 0.10,  0.25, 0.05, 20.0, 5.0,   0.30, 0.05,   0.0, 0.0,  0.40,  0.17, 1u),
    /* 5: MONOLITH */ GoLTierParams(0.20, 0.03,  12.0, 3.0,   0.3, 0.05,  0.03, 0.01,  42.0, 12.0, 0.05,  0.12, 0u),
    /* 6: GLACIER  */ GoLTierParams(0.12, 0.03,   4.0, 1.0,   2.0, 0.5,   0.08, 0.02,  24.0, 7.5,  0.25,  0.13, 0u),
);

// --- Pulse Algorithm Tier Definitions ────────────────────────────────────
//
// Pulse zones: periodic breathing of cell color/height, no neighbor rules.
// Each cell oscillates between terrain base and a displaced target.
// CPU selects tier and uploads parameters via GoLZoneConfig; these
// definitions are the single source of truth for the tier vocabulary.
//
// Algorithm and boundary mode constants (shared CPU ↔ GPU):
//   GOL_ALGORITHM_CONWAY = 0   GOL_ALGORITHM_PULSE = 1
//   GOL_BOUNDARY_REFLECT = 0   GOL_BOUNDARY_WRAP   = 1
// (defined below in the GoL zone system section)

struct PulseTierParams {
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
}

const PULSE_TIER_COUNT: u32 = 3u;

//                                                       tick_μ   σ    spring_μ σ    trans_μ  σ    phase_μ  σ    tempo_μ σ    ht_μ   σ    wand_μ  σ    sv    wt    no_h  bnd
const PULSE_TIERS = array<PulseTierParams, 3>(
    /* 0: Breathe  */ PulseTierParams( 2.0, 0.5,   4.0, 1.0,   0.20, 0.05,   0.15, 0.05,   0.10, 0.03,   2.0, 0.8,  10.0, 3.0,   0.20,  0.45, 0u, 0u ),
    /* 1: Sparkle  */ PulseTierParams( 0.5, 0.15, 12.0, 3.0,   0.25, 0.05,   0.90, 0.10,   0.60, 0.15,   0.0, 0.0,   5.0, 2.0,   0.50,  0.30, 1u, 0u ),
    /* 2: Drift    */ PulseTierParams( 4.0, 1.0,   1.5, 0.4,   0.10, 0.03,   0.50, 0.15,   0.40, 0.10,   4.0, 1.5,  25.0, 8.0,   0.35,  0.25, 0u, 1u ),
);

// Probability of a zone being Pulse (vs Conway) — must match CPU PULSE_ALGORITHM_CHANCE.
const PULSE_ALGORITHM_CHANCE: f32 = 0.35;

// --- Pawn Safety Force Field
const PAWN_FORCEFIELD_ENABLED: bool = true;

// --- Compile-time feature gates
// These prune heavy dependency chains from update_pawn's pipeline compilation.
// Set to false to cut compile time when iterating on unrelated features.
const PAWN_GOL_GROUND_ENABLED: bool = false;    // Pawn walks on GoL extrusions
// (PAWN_PYRAMID_GROUND_ENABLED removed — pyramids unconditionally in ground_formed)
const PAWN_FORCEFIELD_RADIUS_STATIONARY: f32 = 6.0;  // Radius when not moving
const PAWN_FORCEFIELD_RADIUS_MOVING: f32 = 2.0;      // Radius at max speed
const PAWN_FORCEFIELD_FALLOFF: f32 = 2.0;            // Edge softness (smoothstep width)
const PAWN_FORCEFIELD_SPEED_SCALE: f32 = 1.0;        // How quickly radius shrinks with speed

const COLOR_PAWN: vec3<f32> = vec3(0.8, 0.5, 0.8);

// (legacy raymarcher constants removed: TERRAIN_BASE_COLOR, MAT_TERRAIN/PAWN/SPHERE/SKY,
//  MAX_STEPS, MAX_DIST, SURF_DIST, RAYMARCH_STEP_FACTOR, GOL_TICKS_PER_BEAT)

// §2.3 MUTING CONTROL

// --- Coupling bit flags

const COUPLING_POLYPHONY_TO_AMPLITUDE:       u32 = 1u << 0u;
const COUPLING_TERRAIN_TO_PAWN_Y:            u32 = 1u << 1u;
const COUPLING_TERRAIN_TO_PAWN_TILT:         u32 = 1u << 2u;
const COUPLING_PAWN_TO_CAMERA_TARGET:        u32 = 1u << 3u;
const COUPLING_INPUT_MOVES_PAWN:             u32 = 1u << 4u;
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

// (legacy aliases COUPLING_PAWN_TO_FIELD_COLOR, COUPLING_SPHERE_TO_FIELD_COLOR removed)
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


// §3 COUPLINGS

// §3.1 signal → terrain

// --- [COUPLING:signal.polyphony→terrain:amplitude]

fn coupling_signal_polyphony_to_terrain_amplitude(polyphony: f32, traj: Trajectory, dt: f32) -> Trajectory {
    let goal = IDLE_AMPLITUDE_SCALE * (1.0 + polyphony/4);
    
    if (polyphony > 0.0) {
        // Attack: fast exponential approach
        let rate = 3.0 / AMPLITUDE_ATTACK_TIME;
        let new_val = traj.value + (goal - traj.value) * (1.0 - exp(-rate * dt));
        return Trajectory(new_val, 0.0, 0.0, 0.0);
    } else {
        // Release: slow exponential decay
        let rate = 3.0 / AMPLITUDE_RELEASE_TIME;
        return trajectory_release(traj, IDLE_AMPLITUDE_SCALE, dt, rate);
    }
}

// §3.2 signal → entities

// --- [COUPLING:signal.polyphony→sphere:color]
fn coupling_signal_polyphony_to_sphere_color(polyphony: f32, current: vec3<f32>, dt: f32) -> vec3<f32> {
    let intensity = saturate(polyphony / 8.0);
    
    // --- HYBRID APPROACH
    if (intensity < 0.01) {
        // Silent: smooth return to base color using existing release rate
        return current + (sphere_state.base_color - current) * (1.0 - exp(-SPHERE_COLOR_RELEASE_RATE * dt));
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

// --- [COUPLING:input→camera:orbit]

// (coupling_input_to_camera_orbit removed — logic inlined in update_camera)

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

fn pyramid_height_at(world_xz: vec2<f32>) -> f32 {
    var best: f32 = 0.0;
    let count = min(pyramid_instances.count, MAX_PYRAMID_INSTANCES);
    for (var i = 0u; i < count; i++) {
        let h = evaluate_pyramid(world_xz, pyramid_instances.instances[i]);
        best = max(best, h);
    }
    return best;
}

// GoL zone cell height at a world position.
// Searches active zones for a match, returns visual × alive_height.
fn zone_gol_height_at(world_xz: vec2<f32>) -> f32 {
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
        var h = visual * zp.alive_height * height_factor * config.mode_gol_height_scale;

        // Pawn proximity: suppress GoL extrusion near the pawn.
        // Must match extrusion VS suppression geometry exactly so pawn
        // ground resolve and mesh geometry agree.
        let pawn_dist = distance(world_xz, pawn_state.pos.xz);
        let suppression = 1.0 - smoothstep(ZONE_SUPPRESS_INNER, ZONE_SUPPRESS_OUTER, pawn_dist);
        h *= (1.0 - suppression);

        return h;
    }
    return 0.0;
}

// --- Composable Ground Hierarchy ---
//
// Stratified ground height: each level includes all contributions below it.
// Consumers pick the level they need. Spawning order follows the hierarchy
// (lower layers first, so higher layers see them).
//
//   Level 0  ground_terrain   terrain waves × tile mods + solids (piers, ramps)
//   Level 1  ground_formed    + pyramids (large static landforms)
//   Level 2  effective_ground_y  + GoL zones (dynamic per-frame)
//
// Baked heightfield caches ground_formed (levels 0-1).
// Pawn walks on effective_ground_y (levels 0-2).
//
// Y-correction consumers:
//   Pyramid placement  → ground_terrain  (doesn't see itself or other pyramids)
//   Arch/column placement → ground_formed  (sees pyramids, not GoL)
//   Painting placement → effective_ground_y  (sees everything)
//
// To add a new static landform between pyramids and GoL:
//   insert a new fn between ground_formed and effective_ground_y.
// To add a new dynamic contributor:
//   add it to effective_ground_y.

fn ground_terrain(world_xz: vec2<f32>) -> f32 {
    let raw_h = terrain_height_at(world_xz, config.world_seed, 0.0);
    let mods = tile_modifiers_at(world_xz);
    return raw_h * mods.x + mods.y + structure_height_at(world_xz);
}

fn ground_formed(world_xz: vec2<f32>) -> f32 {
    return ground_terrain(world_xz) + pyramid_height_at(world_xz);
}

// Combined height + complexity — avoids evaluating terrain lattice waves twice.
// terrain_height_and_complexity does the same lattice work as terrain_height_at
// but also accumulates the complexity metric. Used by two-pass heightfield gen.
fn ground_formed_with_complexity(world_xz: vec2<f32>) -> vec2<f32> {
    let hc = terrain_height_and_complexity(world_xz, config.world_seed, 0.0);
    let mods = tile_modifiers_at(world_xz);
    let height = hc.x * mods.x + mods.y + structure_height_at(world_xz) + pyramid_height_at(world_xz);
    return vec2(height, hc.y);
}

fn effective_ground_y(world_xz: vec2<f32>) -> f32 {
    var h = ground_formed(world_xz);
    if (PAWN_GOL_GROUND_ENABLED) {
        h += zone_gol_height_at(world_xz);
    }
    return h;
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
// ─── Overlay Wave Design Matrix ─────────────────────────────────────────
//
//   amp        World-unit displacement at full blend.
//   freq       Spatial frequency (cycles per world unit). Higher = tighter ripples.
//   period     Temporal period in beats. One full sine cycle per this many beats.
//   direction  Propagation angle (radians). 0 = +X, π/2 = +Z. Negative = seed-derived.
//   amp_jit    Amplitude jitter range. Seed scales amp by (1 ± jit/2).
//   freq_jit   Frequency jitter range. Seed scales freq by (1 ± jit/2).

struct OverlayWave {
    amp: f32,
    freq: f32,
    period: f32,
    direction: f32,
    amp_jit: f32,
    freq_jit: f32,
}

const OVERLAY_WAVE_COUNT: u32 = 6u;

//                                amp    freq    period  dir     amp_jit  freq_jit
const OVERLAY_WAVES = array<OverlayWave, 6>(
    OverlayWave(                  0.12,  1.00,   3.0,   -1.0,   0.4,     0.4  ),  // 0: fine ripple
    OverlayWave(                  0.25,  0.50,   4.5,   -1.0,   0.4,     0.4  ),  // 1: detail
    OverlayWave(                  0.50,  0.25,   6.0,   -1.0,   0.4,     0.4  ),  // 2: local swell
    OverlayWave(                  1.00,  0.12,   9.0,   -1.0,   0.4,     0.4  ),  // 3: regional
    OverlayWave(                  2.00,  0.06,  13.0,   -1.0,   0.4,     0.4  ),  // 4: broad
    OverlayWave(                  3.50,  0.03,  18.0,   -1.0,   0.4,     0.4  ),  // 5: tectonic
);

fn terrain_wave_overlay(world_xz: vec2<f32>) -> f32 {
    if (config.terrain_time <= 0.0) { return 0.0; }

    let seed = config.world_seed;
    var h: f32 = 0.0;

    for (var i: u32 = 0u; i < OVERLAY_WAVE_COUNT; i++) {
        let blend = get_band_blend(i);
        if (blend <= 0.0) { continue; }

        let ow = OVERLAY_WAVES[i];
        let origin = get_band_phase_origin(i);
        let t = config.terrain_time - origin;

        // Direction: explicit angle or seed-derived when negative
        var angle: f32;
        if (ow.direction < 0.0) {
            angle = hash_property(seed, 900u + i * 10u) * 2.0 * PI;
        } else {
            angle = ow.direction;
        }
        let dir = vec2(cos(angle), sin(angle));

        // Seed-derived jitter on frequency and amplitude
        let freq = ow.freq * (1.0 + (hash_property(seed, 901u + i * 10u) - 0.5) * ow.freq_jit);
        let amp  = ow.amp  * (1.0 + (hash_property(seed, 902u + i * 10u) - 0.5) * ow.amp_jit);

        let temporal = (2.0 * PI / ow.period) * t;
        let spatial  = freq * dot(dir, world_xz);

        h += blend * amp * sin(spatial + temporal);
    }

    return h;
}

// Gradient of the wave overlay via central finite differences.
// RETAINED for any future consumer that needs gradient without height.
// The terrain VS now uses terrain_wave_overlay_with_gradient instead.
fn terrain_wave_overlay_gradient(world_xz: vec2<f32>) -> vec2<f32> {
    if (config.terrain_time <= 0.0) { return vec2(0.0); }
    let eps = 0.5;
    let h_px = terrain_wave_overlay(world_xz + vec2(eps, 0.0));
    let h_mx = terrain_wave_overlay(world_xz - vec2(eps, 0.0));
    let h_pz = terrain_wave_overlay(world_xz + vec2(0.0, eps));
    let h_mz = terrain_wave_overlay(world_xz - vec2(0.0, eps));
    return vec2((h_px - h_mx) / (2.0 * eps), (h_pz - h_mz) / (2.0 * eps));
}

// Fused height + analytical gradient for the wave overlay.
// Returns vec3(height, dh/dx, dh/dz).
// Replaces the 5x finite-difference approach with 1x loop + analytical derivatives.
// Used by patch_terrain_vs where per-vertex cost dominates frame time.
fn terrain_wave_overlay_with_gradient(world_xz: vec2<f32>) -> vec3<f32> {
    if (config.terrain_time <= 0.0) { return vec3(0.0); }

    let seed = config.world_seed;
    var h: f32 = 0.0;
    var gx: f32 = 0.0;
    var gz: f32 = 0.0;

    for (var i: u32 = 0u; i < OVERLAY_WAVE_COUNT; i++) {
        let blend = get_band_blend(i);
        if (blend <= 0.0) { continue; }

        let ow = OVERLAY_WAVES[i];
        let origin = get_band_phase_origin(i);
        let t = config.terrain_time - origin;

        // Direction: explicit angle or seed-derived when negative
        var angle: f32;
        if (ow.direction < 0.0) {
            angle = hash_property(seed, 900u + i * 10u) * 2.0 * PI;
        } else {
            angle = ow.direction;
        }
        let dir = vec2(cos(angle), sin(angle));

        // Seed-derived jitter on frequency and amplitude
        let freq = ow.freq * (1.0 + (hash_property(seed, 901u + i * 10u) - 0.5) * ow.freq_jit);
        let amp  = ow.amp  * (1.0 + (hash_property(seed, 902u + i * 10u) - 0.5) * ow.amp_jit);

        let temporal = (2.0 * PI / ow.period) * t;
        let phase    = freq * dot(dir, world_xz) + temporal;

        // h  += B * A * sin(phase)
        // dh/dx = B * A * freq * dir.x * cos(phase)
        // dh/dz = B * A * freq * dir.y * cos(phase)
        let ba = blend * amp;
        let s  = sin(phase);
        let c  = cos(phase);

        h  += ba * s;
        gx += ba * freq * dir.x * c;
        gz += ba * freq * dir.y * c;
    }

    return vec3(h, gx, gz);
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

fn evaluate_radial_pulses(world_xz: vec2<f32>, t_seconds: f32) -> f32 {
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

fn effective_ground_with_gradients(world_xz: vec2<f32>, eps: f32) -> vec3<f32> {
    let h    = effective_ground_y(world_xz);
    let h_px = effective_ground_y(world_xz + vec2(eps, 0.0));
    let h_mx = effective_ground_y(world_xz - vec2(eps, 0.0));
    let h_pz = effective_ground_y(world_xz + vec2(0.0, eps));
    let h_mz = effective_ground_y(world_xz - vec2(0.0, eps));
    return vec3(h, (h_px - h_mx) / (2.0 * eps), (h_pz - h_mz) / (2.0 * eps));
}

// Like effective_ground_with_gradients, but clamps discontinuities (walls)
fn effective_ground_with_gradients_walkable(world_xz: vec2<f32>, eps: f32) -> vec3<f32> {
    let h0 = effective_ground_y(world_xz);

    var h_px = effective_ground_y(world_xz + vec2(eps, 0.0));
    var h_mx = effective_ground_y(world_xz - vec2(eps, 0.0));
    var h_pz = effective_ground_y(world_xz + vec2(0.0, eps));
    var h_mz = effective_ground_y(world_xz - vec2(0.0, eps));

    if (abs(h_px - h0) > PAWN_STEP_HEIGHT) { h_px = h0; }
    if (abs(h_mx - h0) > PAWN_STEP_HEIGHT) { h_mx = h0; }
    if (abs(h_pz - h0) > PAWN_STEP_HEIGHT) { h_pz = h0; }
    if (abs(h_mz - h0) > PAWN_STEP_HEIGHT) { h_mz = h0; }

    return vec3(h0, (h_px - h_mx) / (2.0 * eps), (h_pz - h_mz) / (2.0 * eps));
}

fn coupling_effective_ground_to_pawn_orientation(pawn_xz: vec2<f32>, heading: f32) -> vec4<f32> {
    let eg = effective_ground_with_gradients_walkable(pawn_xz, 0.5);
    let normal = normalize(vec3(-eg.y, 1.0, -eg.z));
    let world_up = vec3(0.0, 1.0, 0.0);
    let dot_val = dot(world_up, normal);
    
    var tilt_quat: vec4<f32>;
    if (dot_val < 0.9999) {
        let axis = normalize(cross(world_up, normal));
        let angle = acos(clamp(dot_val, -1.0, 1.0));
        tilt_quat = quat_from_axis_angle(axis, angle);
    } else {
        tilt_quat = vec4(0.0, 0.0, 0.0, 1.0);
    }
    
    let heading_quat = quat_from_axis_angle(vec3(0.0, 1.0, 0.0), heading);
    return quat_multiply(tilt_quat, heading_quat);
}

// --- [COUPLING:terrain→sphere:orbit_height]

fn coupling_terrain_to_sphere_orbit_height(sphere_xz: vec2<f32>, base_height: f32) -> f32 {
    if (!coupling_active(COUPLING_TERRAIN_TO_SPHERE_HEIGHT)) {
        return base_height;
    }
    
    // Sphere clearance → ground_formed level + wave overlay.
    // Sphere flies above pyramids and animated terrain but ignores dynamic GoL blocks.
    let ground = ground_formed(sphere_xz) + terrain_wave_overlay(sphere_xz);
    
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

// --- [COUPLING:sphere→terrain:tint]
fn coupling_sphere_to_terrain_tint(sphere_pos: vec3<f32>) -> vec3<f32> {
    // Normalize position to [-1, 1] based on orbit radius
    let nx = sphere_pos.x / sphere_state.orbit_radius;
    let nz = sphere_pos.z / sphere_state.orbit_radius;
    
    // Map sphere position to RGB offsets within variance bounds
    let offset = vec3(
        nx * SAND_DUNE_VARIANCE,
        nz * SAND_DUNE_VARIANCE * 0.5,
        -nx * SAND_DUNE_VARIANCE * 0.6
    );
    
    return clamp(SAND_DUNE_CENTER + offset, vec3(0.0), vec3(1.0));
}

// §3.7 GoL → evolution

// --- [COUPLING:gol→next_state] Conway's rules
fn coupling_gol_next_state(alive: bool, neighbors: i32) -> f32 {
    if (alive) {
        return select(0.0, 1.0, neighbors == 2 || neighbors == 3);
    } else {
        return select(0.0, 1.0, neighbors == 3);
    }
}

// --- Legacy fixed-wave dynamics (Lipschitz bound still alive) ---

fn wave_enabled(index: i32) -> bool {
    return (config.wave_enable_mask & (1u << u32(index))) != 0u;
}

// (dynamics_terrain_wave_single, dynamics_terrain_wave_eval,
//  wave_frozen, wave_time_for, dynamics_terrain_gradient_single,
//  dynamics_terrain_gradient_eval, dynamics_terrain_normal removed —
//  replaced by lattice-based terrain. Only gradient_max survives
//  for Lipschitz factor computation in update_terrain_config.)

fn dynamics_terrain_gradient_max(amplitude_scale: f32) -> f32 {
    var max_grad: f32 = 0.0;
    for (var i: i32 = 0; i < WAVE_COUNT; i++) {
        if (wave_enabled(i)) {
            max_grad += WAVES[i].amplitude * WAVES[i].frequency;
        }
    }
    return max_grad * amplitude_scale * HEIGHT_MAX_AMPLITUDE * 1.5;
}

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
fn dynamics_sphere_motor_orbit(t: f32) -> SphereState {
    var s: SphereState;

    // 1. DEFINITION
    //    Axis: The vertical line through the origin (Line Y)
    //    Speed: From per-entity state
    let angle = t * sphere_state.orbit_speed;

    // 2. THE MOTOR (The Spell)
    //    Create a rotor that spins around the Y axis.
    let m_orbit = rotor(LINE_Y.d, angle);

    // 3. LIFT (The Input)
    //    Offset from anchor — orbit_radius on X, flat plane.
    //    Height is added after; terrain coupling adjusts it.
    let offset = vec3(sphere_state.orbit_radius, 0.0, 0.0);
    let p_start = point_from_vec3(offset);

    // 4. MOTIVATE (The Transformation)
    //    Apply the rotor to the offset point.
    let p_moved = sw_motor_point(m_orbit, p_start);
    let local_pos = point_to_vec3(p_moved);

    // 5. DROP (The Output)
    //    Anchor translation + base orbit height.
    s.pos = sphere_state.anchor + vec3(local_pos.x, sphere_state.orbit_height, local_pos.z);

    // Bonus: PGA gives us the orientation for free!
    s.orientation = m_orbit.p0;

    s.radius = sphere_state.radius;
    s.influence_radius = sphere_state.influence_radius;
    s.t = t;

    return s;
}

// (dynamics_sphere_orbit_tangent removed — PGA motor provides orientation directly)

// §5 COMPOSITION

// Execution orchestration - where couplings and dynamics are applied.
fn compose_sphere_from_orbit_pga(t: f32) -> SphereState {
    // 1. Get the pure PGA orbit (circular motion via Motor)
    var s = dynamics_sphere_motor_orbit(t);
    
    // 2. Apply terrain coupling (Height adjustment)
    //    This is an "effect" applied after the ideal motion
    let base_height = s.pos.y;  // orbit_height from motor, adjusted by terrain
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

// §5.1 0D COMPOSITION — Now split into 4 entry points (§7.1):
//   update_terrain_config, update_pawn, update_camera, update_sphere
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
    // --- Camera frame (matches raymarch_get_direction convention)
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
fn shade_lit(world_pos: vec3<f32>, normal: vec3<f32>, base_color: vec3<f32>) -> vec3<f32> {
    // Ambient (always present)
    let ambient = base_color * render_light.ambient;

    // Directional light with shadows
    let sun = base_color * calc_directional_light(world_pos, normal);

    // Point lights (diffuse only)
    let points = base_color * calc_point_lights(world_pos, normal);

    // Spot light (cone + distance, indoor moods)
    let spot = base_color * calc_spot_light(world_pos, normal);

    let lit = ambient + sun + points + spot;

    // Fog
    let dist = distance(world_pos, render_camera.pos);
    let fog = 1.0 - exp(-dist * config.fog_density);
    return mix(lit, config.fog_color, fog);
}


// §6.2 PATCH TERRAIN RENDERING
// Instanced rendering of streaming terrain patches. Each instance is one
struct PatchTerrainVarying {
    @builtin(position) clip_pos: vec4<f32>,
    @location(0) world_pos: vec3<f32>,
    @location(1) gradients: vec2<f32>,
    @location(2) complexity: f32,        // local wave interference density [0,1]
    @location(3) patch_uv: vec2<f32>,    // UV within the patch [0,1] for cell sampling
    @location(4) @interpolate(flat) layer: u32,  // heightfield/cell array layer
}

@vertex
fn patch_terrain_vs(
    @builtin(vertex_index) vi: u32,
    @builtin(instance_index) patch_id: u32
) -> PatchTerrainVarying {
    let pi = patch_instances[patch_id];

    // Decode grid position from vertex index (PATCH_MESH_N×PATCH_MESH_N grid)
    let vx = vi % PATCH_MESH_STRIDE;
    let vz = vi / PATCH_MESH_STRIDE;

    // UV within the patch [0, 1]
    let uv = vec2(
        f32(vx) / f32(PATCH_MESH_N),
        f32(vz) / f32(PATCH_MESH_N)
    );

    // Remap UV to align with texel centers in the heightfield.
    let res = f32(PATCH_HEIGHTFIELD_N);
    let sample_uv = (uv * (res - 1.0) + 0.5) / res;

    // Sample heightfield from this patch's array layer
    // .x = height, .yz = gradients, .w = complexity
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
    let aura = sample_pawn_aura(world_pos.xz, render_pawn.pos.xz);
    world_pos.y += aura.r * config.pawn_aura_height;

    // Polyphony-driven wave overlay — fused height + analytical gradient (1 loop, not 5)
    let wave = terrain_wave_overlay_with_gradient(world_pos.xz);
    world_pos.y += wave.x;

    // Radial pulses: expanding ring wavefronts from note onsets
    let pulse_h = evaluate_radial_pulses(world_pos.xz, render_signal.t_seconds);
    world_pos.y += pulse_h;

    var out: PatchTerrainVarying;
    out.clip_pos = render_vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.gradients = height_data.yz + wave.yz;
    out.complexity = height_data.w;
    out.patch_uv = uv;
    out.layer = pi.layer;
    return out;
}

@fragment
fn patch_terrain_fs(in: PatchTerrainVarying) -> @location(0) vec4<f32> {
    var normal = normalize(vec3(-in.gradients.x, 1.0, -in.gradients.y));

    // Color fully composited at gen-time in the cell texture.
    // Alpha carries the cell behavior tag (0.0 = static, nonzero = animated).
    let color_sample = textureSampleLevel(
        patch_cell_color_array_read, nearest_sampler,
        in.patch_uv, i32(in.layer), 0.0
    );

    var base_color = color_sample.rgb;

    // --- Musical animation modes: re-evaluate cell color with biases
    let has_mode_bias = (config.mode_color_shift > 0.001)
                     || (config.mode_checker_scatter > 0.001)
                     || (config.mode_palette_intensity > 0.001);
    if (has_mode_bias) {
        base_color = animated_cell_color(in.world_pos.xz);
    }

    // --- GoL zone visualization
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
                            let pawn_ff = 1.0 - zone_pawn_ff(in.world_pos.xz, render_pawn.pos, render_pawn.velocity);
                            if (pawn_ff > 0.01) {
                                base_color = mix(base_color, ZONE_PAWN_TINT, pawn_ff * ZONE_PAWN_TINT_STRENGTH * color_val);
                            }

                            // Sphere force field: tint zone cells near sphere (render context)
                            let sphere_ff = 1.0 - zone_sphere_ff(in.world_pos.xz, render_sphere.pos);
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
    {
        let aura = sample_pawn_aura(in.world_pos.xz, render_pawn.pos.xz);
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

    return vec4(shade_lit(in.world_pos, normal, base_color), 1.0);
}

// Shadow pass variant — same geometry, light VP instead of camera VP.
@vertex
fn shadow_patch_terrain_vs(
    @builtin(vertex_index) vi: u32,
    @builtin(instance_index) patch_id: u32
) -> ShadowVarying {
    let pi = patch_instances[patch_id];

    let vx = vi % PATCH_MESH_STRIDE;
    let vz = vi / PATCH_MESH_STRIDE;

    let uv = vec2(
        f32(vx) / f32(PATCH_MESH_N),
        f32(vz) / f32(PATCH_MESH_N)
    );

    let res = f32(PATCH_HEIGHTFIELD_N);
    let sample_uv = (uv * (res - 1.0) + 0.5) / res;

    let height_data = textureSampleLevel(
        patch_heightfield_array_read, bilinear_sampler,
        sample_uv, i32(pi.layer), 0.0
    );

    let wx = pi.origin.x + (uv.x - 0.5) * pi.extent;
    let wz = pi.origin.y + (uv.y - 0.5) * pi.extent;
    let world_pos = vec3(wx, height_data.x + terrain_wave_overlay(vec2(wx, wz)), wz);

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
fn pawn_vs(@builtin(vertex_index) vid: u32) -> EntityVarying {
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

    // Transform by pawn orientation and position
    let rotated_pos = quat_rotate(render_pawn.orientation, local_pos);
    let rotated_normal = quat_rotate(render_pawn.orientation, local_normal);

    var out: EntityVarying;
    out.clip_pos = render_vp.m * vec4(rotated_pos + render_pawn.pos, 1.0);
    out.world_pos = rotated_pos + render_pawn.pos;
    out.normal = rotated_normal;
    out.entity_color = COLOR_PAWN;
    return out;
}

@vertex
fn sphere_vs(in: MeshVertexInput) -> EntityVarying {
    // Scale by radius and translate
    let world_pos = in.pos * render_sphere.radius + render_sphere.pos;

    var out: EntityVarying;
    out.clip_pos = render_vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.normal = in.normal;  // Unit sphere normals stay correct after uniform scale
    out.entity_color = render_sphere.color;
    return out;
}

@fragment
fn entity_fs(in: EntityVarying) -> @location(0) vec4<f32> {
    return vec4(shade_lit(in.world_pos, normalize(in.normal), in.entity_color), 1.0);
}


// §6.4 SHADOW PASS VERTEX SHADERS
// Depth-only rendering from the light's point of view.
struct ShadowVarying {
    @builtin(position) clip_pos: vec4<f32>,
}

@vertex
fn shadow_pawn_vs(@builtin(vertex_index) vid: u32) -> ShadowVarying {
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

    let world_pos = quat_rotate(render_pawn.orientation, local_pos) + render_pawn.pos;

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
fn shadow_sphere_vs(in: MeshVertexInput) -> ShadowVarying {
    let world_pos = in.pos * render_sphere.radius + render_sphere.pos;

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
    let ground_y = render_arch_ground[idx].ground_y;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += terrain_wave_overlay(world_pos.xz);

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
    let ground_y = render_arch_ground[idx].ground_y;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += terrain_wave_overlay(world_pos.xz);

    var out: ShadowVarying;
    out.clip_pos = render_vp.light_vp * vec4(world_pos, 1.0);
    return out;
}

// --- Generative Columns
@vertex
fn column_vs(in: ArchVertexInput) -> EntityVarying {
    let idx = u32(in.arch_index);
    let ground_y = render_column_ground[idx].ground_y;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += terrain_wave_overlay(world_pos.xz);

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
    let ground_y = render_column_ground[idx].ground_y;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += terrain_wave_overlay(world_pos.xz);

    var out: ShadowVarying;
    out.clip_pos = render_vp.light_vp * vec4(world_pos, 1.0);
    return out;
}

// --- Generative Pyramids
@vertex
fn pyramid_vs(in: ArchVertexInput) -> EntityVarying {
    let idx = u32(in.arch_index);
    let ground_y = render_pyramid_ground[idx].ground_y;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += terrain_wave_overlay(world_pos.xz);

    var out: EntityVarying;
    out.clip_pos = render_vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.normal = in.normal;
    out.entity_color = in.color;
    return out;
}

@vertex
fn shadow_pyramid_vs(in: ArchVertexInput) -> ShadowVarying {
    let idx = u32(in.arch_index);
    let ground_y = render_pyramid_ground[idx].ground_y;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += terrain_wave_overlay(world_pos.xz);

    var out: ShadowVarying;
    out.clip_pos = render_vp.light_vp * vec4(world_pos, 1.0);
    return out;
}

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
// A continuous square-section tube animated by wave superposition.
fn ribbon_spine_at(t: f32, ribbon: RibbonState) -> vec3<f32> {
    let total_length = f32(ribbon.cube_count) * ribbon.cube_size;
    let time = ribbon.time;

    let along = (t - 0.5) * total_length;
    let lateral = sin(time * ribbon.lateral_speed + t * ribbon.lateral_cycles * 2.0 * PI) * ribbon.lateral_amp;
    let vertical = ribbon.height + sin(time * ribbon.vertical_speed + t * ribbon.vertical_cycles * 2.0 * PI) * ribbon.vertical_amp;

    // Heading rotation (lateral sway only — no twist here)
    let c = cos(ribbon.orientation);
    let s = sin(ribbon.orientation);
    let rotated_along   = along * c - lateral * s;
    let rotated_lateral = along * s + lateral * c;

    // Twist: helical displacement PERPENDICULAR to the sway plane.
    // Depth axis (rotated_lateral) + vertical = corkscrew around the swaying spine.
    // Orthogonal to lateral → no wave interference, no beating.
    let twist_phase = time * ribbon.twist_speed + t * ribbon.twist_cycles * 2.0 * PI;
    let twist_depth = sin(twist_phase) * 0.4 * ribbon.twist_amp;
    let twist_vert  = cos(twist_phase) * 0.3 * ribbon.twist_amp;

    return ribbon.anchor + vec3(rotated_along, vertical + twist_vert, rotated_lateral + twist_depth);
}

// Tangent direction at parameter t (central finite difference).
fn ribbon_tangent_at(t: f32, ribbon: RibbonState) -> vec3<f32> {
    let eps = 0.0005;
    return normalize(ribbon_spine_at(t + eps, ribbon) - ribbon_spine_at(t - eps, ribbon));
}

// Build a PGA motor that places and orients one cross-section ring.
// Composes: translate(center) * correction * heading_rotor.
//
// The orientation is decomposed into two rotors to avoid the
// antiparallel singularity that occurs when orientation ≈ 180°:
//   1. heading_rotor: Y-axis rotation by orientation angle
//      — maps (1,0,0) → (cos(θ),0,sin(θ)), always well-defined
//   2. correction: small-angle rotor from heading direction to tangent
//      — angle is always small because the tangent tracks the heading
fn ribbon_ring_motor(ring_idx: u32, ribbon: RibbonState) -> Motor {
    let t = f32(ring_idx) / f32(max(ribbon.cube_count - 1u, 1u));
    let center = ribbon_spine_at(t, ribbon);
    let tangent = ribbon_tangent_at(t, ribbon);

    // Step 1: Pre-rotate local frame to the ribbon heading.
    let heading = rotor(vec3(0.0, 1.0, 0.0), ribbon.orientation);

    // Step 2: Small correction from heading direction to actual tangent.
    let heading_dir = vec3(cos(ribbon.orientation), 0.0, sin(ribbon.orientation));
    let corr_axis = cross(heading_dir, tangent);
    let corr_cos = dot(heading_dir, tangent);

    var correction: Motor;
    if (length(corr_axis) < 0.001) {
        correction = MOTOR_IDENTITY;
    } else {
        correction = rotor(corr_axis, acos(clamp(corr_cos, -1.0, 1.0)));
    }

    // Compose: translate * correction * heading
    let orient = gp_mm(correction, heading);
    let trans = Motor(vec4(1.0, 0.0, 0.0, 0.0), vec4(-0.5 * center, 0.0));
    return gp_mm(trans, orient);
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

// --- Fallen Ribbon: Terrain Height at World Position
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

    // Compute PGA motor (translate + orient along spine)
    let motor = ribbon_ring_motor(ring_idx, ribbon);
    let center = sw_mp(motor, vec3(0.0));

    let terrain_y: f32 = 0.0;  // Only flying ribbons now; no terrain-following needed.

    ring_xforms[ring_idx].motor_p0 = motor.p0;
    ring_xforms[ring_idx].motor_p1 = motor.p1;
    ring_xforms[ring_idx].center = center;
    ring_xforms[ring_idx].terrain_y = terrain_y;
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
        motor = ribbon_ring_motor(ring_idx, ribbon);
    }
    let orient = Motor(motor.p0, vec4(0.0));

    var world_pos = sw_mp(motor, local_pos);
    let world_normal = sw_mp(orient, local_normal);

    var out: EntityVarying;
    out.clip_pos = render_vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.normal = world_normal;
    out.entity_color = ribbon.color;
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
        motor = ribbon_ring_motor(ring_idx, ribbon);
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
@group(0) @binding(20)  var<storage, read_write> terrain_state: TerrainState;
@group(0) @binding(60)  var<storage, read_write> pawn_state: PawnState;

// Portal proximity array (uploaded by CPU, checked by update_pawn)
struct PortalEntry {
    x: f32,
    z: f32,
    trigger_radius: f32,
    arch_index: u32,
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
@group(0) @binding(100) var<storage, read_write> sphere_state: SphereState;
@group(0) @binding(101) var<storage, read_write> trajectories: array<Trajectory, 16>;
@group(0) @binding(120) var<uniform>             ribbon_state: RibbonState;

// --- [BINDINGS:compute] Group 0 — Render entity mirrors (read-only, +200 offset)
@group(0) @binding(200) var<storage, read> render_signal: FrameSignal;
@group(0) @binding(201) var<storage, read> render_vp: VPMatrix;
@group(0) @binding(220) var<storage, read> render_terrain: TerrainState;
@group(0) @binding(260) var<storage, read> render_pawn: PawnState;
@group(0) @binding(280) var<storage, read> render_camera: CameraState;
@group(0) @binding(300) var<uniform> render_sphere: SphereState;

// --- Ribbon (Group 0: render, binding 360)
@group(0) @binding(360) var<uniform> render_ribbon: RibbonState;
@group(0) @binding(361) var<storage, read> render_ring_xforms: array<RibbonRingTransform, 400>;
@group(0) @binding(380) var<storage, read> render_arch_ground: array<ArchGroundEntry, 16>;
@group(0) @binding(381) var<storage, read> render_column_ground: array<ColumnGroundEntry, 32>;
@group(0) @binding(382) var<storage, read> render_pyramid_ground: array<PyramidGroundEntry, 8>;
@group(0) @binding(383) var<uniform> render_palm_ground: array<PalmGroundEntry, 24>;

struct CactusGroundEntry {
    center_x: f32,   // 1
    center_z: f32,   // 2
    ground_y: f32,   // 3
    is_active: u32,  // 4
    _pad0: f32, _pad1: f32, _pad2: f32, _pad3: f32,  // 5-8 = 32 bytes
}
@group(0) @binding(384) var<uniform> render_cactus_ground: array<CactusGroundEntry, 20>;

struct BladeClusterGroundEntry {
    center_x: f32,
    center_z: f32,
    ground_y: f32,
    is_active: u32,
    _pad0: f32, _pad1: f32, _pad2: f32, _pad3: f32,
}
@group(0) @binding(385) var<uniform> render_blade_ground: array<BladeClusterGroundEntry, 32>;

// --- Ribbon compute (Group 0: binding 121, separate pipeline layout)
// Written by compute_ribbon_rings, read by ribbon VS via render_ring_xforms.
@group(0) @binding(121) var<storage, read_write> ring_xforms: array<RibbonRingTransform, 400>;

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
// (legacy cell mesh gen bindings 40-45 removed — reserved for future use)
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
@group(0) @binding(28) var<storage, read_write> patch_height_scratch: array<f32>;

// --- Patch rendering (Group 0: binding 340, Group 1: bindings 28-29)
@group(0) @binding(340) var<storage, read> patch_instances: array<PatchInstance>;
@group(1) @binding(28) var patch_heightfield_array_read: texture_2d_array<f32>;
@group(1) @binding(29) var patch_cell_color_array_read: texture_2d_array<f32>;

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

    let freq = tempo_jitter / max(tick_period * config.mode_gol_tick_scale, 0.1);
    let phase = t_beats * freq * 2.0 * PI + cell_phase;

    return sin(phase) * 0.5 + 0.5;
}

// --- Terrain cell color at a world position
const GOL_TERRAIN_CELL_SIZE: f32 = 3.125;  // PATCH_EXTENT / 16

fn gol_composite_cell_color(world_xz: vec2<f32>) -> vec3<f32> {
    let cell_gx = i32(floor(world_xz.x / GOL_TERRAIN_CELL_SIZE));
    let cell_gz = i32(floor(world_xz.y / GOL_TERRAIN_CELL_SIZE));
    let cell_seed = lattice_node_seed(config.world_seed, vec2(cell_gx, cell_gz), 200u);

    // Evaluate all fields (same as evaluate_cell_fields, minus archetype)
    var s: CellFieldState;
    s.world_xz = world_xz;
    let palette_w = palette_field_at(world_xz);
    s.smooth_color = palette_color_smooth(palette_w, 0.5);
    s.discrete_color = discrete_cell_color(world_xz, cell_gx, cell_gz, cell_seed);
    s.mode = mode_field_at(world_xz);
    s.style = transition_style_at(world_xz);
    s.sparse = sparse_field_at(world_xz);
    s.cell_roll = hash_property(cell_seed, 900u);
    s.sparse_roll = hash_property(cell_seed, 910u);
    s.archetype = 1u;  // unused by composite_cell_color

    return composite_cell_color(s);
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
fn apply_gol_extrusion_color(world_xz: vec2<f32>, zp: GoLZoneConfig, cx: u32, cy: u32) -> vec3<f32> {
    // Start from the actual terrain cell color at this block's position
    let cell_color = gol_composite_cell_color(world_xz);

    let h = gol_cell_hash(cx, cy);
    let v = gol_cell_variation(h);
    let variation = v * GOL_LENS_VARIATION_RANGE - GOL_LENS_VARIATION_RANGE * 0.5;

    if (zp.color_mode == GOL_COLOR_NEUTRAL) {
        return cell_color * (GOL_NEUTRAL_DARKEN + variation);
    } else if (zp.color_mode == GOL_COLOR_LENS) {
        let tint_color = vec3(zp.target_r, zp.target_g, zp.target_b);
        return mix(cell_color, tint_color, GOL_LENS_BLEND_BASE + variation);
    }
    // BLACKISH
    let dark_factor = GOL_BLACK_DARK_BASE + v * GOL_BLACK_DARK_RANGE;
    let r_shift = f32((h >> 8u) & 0xFFu) / 255.0 * GOL_BLACK_R_SHIFT_RANGE - GOL_BLACK_R_SHIFT_RANGE * 0.5;
    let g_shift = f32((h >> 16u) & 0xFFu) / 255.0 * GOL_BLACK_G_SHIFT_RANGE - GOL_BLACK_G_SHIFT_RANGE * 0.5;
    return clamp(cell_color * dark_factor + vec3(r_shift, g_shift, -r_shift), vec3(0.0), vec3(1.0));
}

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
    return textureSampleLevel(pawn_aura_read, nearest_sampler, aura_uv, 0.0);
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

// --- Pawn Aura compute bindings
// (Group 0: dedicated layout with bindings 60, 170-172)
@group(0) @binding(170) var<uniform> pawn_aura_cfg: PawnAuraConfig;
@group(0) @binding(171) var<storage, read_write> pawn_aura_cells: array<PawnAuraCell>;
@group(0) @binding(172) var pawn_aura_tex_write: texture_storage_2d<rgba16float, write>;

// --- Zone mesh gen output (Group 0: bindings 167-169, same layout as GoL compute)
@group(0) @binding(167) var<storage, read_write> zone_mesh_vertices: array<CellMeshVertex>;
@group(0) @binding(168) var<storage, read_write> zone_mesh_indices: array<u32>;
@group(0) @binding(169) var<storage, read_write> zone_mesh_indirect: array<atomic<u32>, 6>;

// --- Zone heightfield sampling (mesh gen terrain alignment)
@group(0) @binding(163) var zone_heightfield: texture_2d_array<f32>;
@group(0) @binding(164) var zone_hf_sampler: sampler;
@group(0) @binding(165) var<storage, read> zone_patch_instances: array<PatchInstance, 169>;

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
const ZONE_DERIVE_EXTENT: f32         = 100.0;     // zone side length (32 × 3.125)
const ZONE_DERIVE_CELL_SIZE: f32      = 3.125;     // PATCH_EXTENT / PATCH_CELL_N
const ZONE_DERIVE_LENS_LO: f32       = 0.2;       // LENS target color floor
const ZONE_DERIVE_LENS_RANGE: f32     = 0.6;       // LENS target color range

// Color mode weights: [neutral, lens, blackish]
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

    // Zone origin: snap corner to cell grid, then center
    let raw_cx = (f32(req.nx) + 0.5) * MODE_LATTICE_SPACING;
    let raw_cz = (f32(req.nz) + 0.5) * MODE_LATTICE_SPACING;
    let corner_x = floor((raw_cx - ZONE_DERIVE_EXTENT * 0.5) / ZONE_DERIVE_CELL_SIZE) * ZONE_DERIVE_CELL_SIZE;
    let corner_z = floor((raw_cz - ZONE_DERIVE_EXTENT * 0.5) / ZONE_DERIVE_CELL_SIZE) * ZONE_DERIVE_CELL_SIZE;

    var zc: GoLZoneConfig;
    zc.origin = vec2(corner_x + ZONE_DERIVE_EXTENT * 0.5, corner_z + ZONE_DERIVE_EXTENT * 0.5);
    zc.extent = ZONE_DERIVE_EXTENT;
    zc.grid_size = 32u;
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
        var tier_idx: u32 = PULSE_TIER_COUNT - 1u;
        var cumul: f32 = 0.0;
        for (var t: u32 = 0u; t < PULSE_TIER_COUNT; t++) {
            cumul += PULSE_TIERS[t].weight;
            if (tier_roll < cumul) { tier_idx = t; break; }
        }
        let pp = PULSE_TIERS[tier_idx];

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

    zone_config.zones[req.slot] = zc;
}

// Sample baked heightfield at world XZ — exact terrain as rendered.
// Searches patch instances for the covering patch and samples its layer.
fn zone_sample_baked_terrain_y(world_xz: vec2<f32>) -> f32 {
    for (var i = 0u; i < 169u; i++) {
        let pi = zone_patch_instances[i];
        if (pi.extent < 0.01) { continue; }  // empty slot
        let local = world_xz - pi.origin;
        let half = pi.extent * 0.5;
        if (abs(local.x) < half && abs(local.y) < half) {
            let uv = local / pi.extent + 0.5;
            let res = f32(PATCH_HEIGHTFIELD_N);
            let sample_uv = (uv * (res - 1.0) + 0.5) / res;
            let h = textureSampleLevel(zone_heightfield, zone_hf_sampler,
                                       sample_uv, i32(pi.layer), 0.0);
            return h.x;
        }
    }
    // Fallback: analytical evaluation
    return zone_terrain_height(world_xz);
}

const ZONE_MESH_MAX_VERTICES: u32 = 50000u;
const ZONE_MESH_MAX_INDICES: u32 = 75000u;


// §7.1 COMPUTE ENTRY POINTS
// Execution order (critical for correctness):
@compute @workgroup_size(1)
fn update_terrain_config() {
    if (!dynamics_0d_active()) { return; }

    let dt = signal.dt;

    var amplitude_scale = terrain_state.amplitude_scale;

    if (signal_active() && coupling_active(COUPLING_POLYPHONY_TO_AMPLITUDE)) {
        let poly = signal.stats[0];
        let traj = trajectories[0];
        let new_traj = coupling_signal_polyphony_to_terrain_amplitude(poly, traj, dt);
        trajectories[0] = new_traj;
        amplitude_scale = new_traj.value;
    }

    terrain_state.amplitude_scale = amplitude_scale;

    let max_grad = dynamics_terrain_gradient_max(amplitude_scale);
    terrain_state.lipschitz_factor = sqrt(1.0 + max_grad * max_grad);
}

// --- Terrain normal (static, forward-difference)
fn terrain_normal_at(xz: vec2<f32>) -> vec3<f32> {
    let eps = 0.5;
    let h0  = effective_ground_y(xz);
    let h_x = effective_ground_y(xz + vec2(eps, 0.0));
    let h_z = effective_ground_y(xz + vec2(0.0, eps));
    let dx = (h_x - h0) / eps;
    let dz = (h_z - h0) / eps;
    return normalize(vec3(-dx, 1.0, -dz));
}

// --- Pawn ground resolve
fn pawn_ground_resolve(
    new_xz: vec2<f32>, prev_xz: vec2<f32>, prev_y: f32
) -> vec4<f32> {
    let y = effective_ground_y(new_xz);

    // No XZ movement (idle/bootstrap) or passable slope → just snap
    let moved = any(new_xz != prev_xz);
    if (!moved || y - prev_y <= PAWN_STEP_HEIGHT) {
        return vec4(new_xz.x, y, new_xz.y, 1.0);              // happy path
    }

    // Full move blocked — try axis-aligned slides
    let slide_x = vec2(new_xz.x, prev_xz.y);
    let x_y = effective_ground_y(slide_x);
    let x_ok = (x_y - prev_y) <= PAWN_STEP_HEIGHT;

    let slide_z = vec2(prev_xz.x, new_xz.y);
    let z_y = effective_ground_y(slide_z);
    let z_ok = (z_y - prev_y) <= PAWN_STEP_HEIGHT;

    if (x_ok && z_ok) {
        if (abs(new_xz.x - prev_xz.x) >= abs(new_xz.y - prev_xz.y)) {
            return vec4(slide_x.x, x_y, slide_x.y, 1.0);
        }
        return vec4(slide_z.x, z_y, slide_z.y, 1.0);
    }
    if (x_ok) { return vec4(slide_x.x, x_y, slide_x.y, 1.0); }
    if (z_ok) { return vec4(slide_z.x, z_y, slide_z.y, 1.0); }

    // Fully blocked — revert, reuse prev_y (was snapped last frame)
    return vec4(prev_xz.x, prev_y, prev_xz.y, 0.0);
}

@compute @workgroup_size(1)
fn update_pawn() {
    if (!dynamics_0d_active()) { return; }

    let dt = signal.dt;

    var pawn = pawn_state;
    let prev_xz = pawn.pos.xz;
    let prev_y = pawn.pos.y;

    if (coupling_active(COUPLING_INPUT_MOVES_PAWN)) {
        let input_dir = vec2(signal.move_x, signal.move_z);
        let world_vel = coupling_input_to_pawn_velocity(input_dir, camera_state.azimuth);

        let speed = select(PAWN_SPEED, config.pawn_speed, config.pawn_speed > 0.0);
        pawn.pos.x += world_vel.x * speed * dt;
        pawn.pos.z += world_vel.y * speed * dt;

        if (fpv_mode_active()) {
            pawn.heading = camera_state.azimuth;
        } else {
            pawn.heading = coupling_velocity_to_pawn_heading(world_vel, pawn.heading, dt);
        }

        pawn.velocity = world_vel * speed;
    } else {
        pawn.velocity = vec2(0.0);

        if (fpv_mode_active()) {
            pawn.heading = camera_state.azimuth;
        }
    }

    // --- Finite world boundary clamp
    if (config.world_bound_max.x > 0.0) {
        pawn.pos.x = clamp(pawn.pos.x, config.world_bound_min.x, config.world_bound_max.x);
        pawn.pos.z = clamp(pawn.pos.z, config.world_bound_min.y, config.world_bound_max.y);
    }

    // --- Ground resolve: single call, terrain chain behind function boundary
    if (coupling_active(COUPLING_TERRAIN_TO_PAWN_Y)) {
        // Strip dynamic offsets from prev_y: ground_resolve compares frozen base
        // terrain only. Without this, animated overlays create false "steps"
        // that block movement. Cost: 6 sin() + 8 pulse iterations.
        let prev_wave = terrain_wave_overlay(prev_xz);
        let prev_pulse = evaluate_radial_pulses(prev_xz, signal.t_seconds);
        let prev_base_y = prev_y - config.pawn_aura_height - prev_wave - prev_pulse;

        let resolved = pawn_ground_resolve(pawn.pos.xz, prev_xz, prev_base_y);
        pawn.pos.x = resolved.x;
        pawn.pos.y = resolved.y;
        pawn.pos.z = resolved.z;
        if (resolved.w < 0.5) {
            pawn.velocity = vec2(0.0);
        }
        // Pawn rides on top of its own aura extrusion (when enabled)
        pawn.pos.y += config.pawn_aura_height;
        // Polyphony-driven wave overlay (pawn rides animated terrain)
        pawn.pos.y += terrain_wave_overlay(pawn.pos.xz);
        // Radial pulses (pawn rides expanding ring wavefronts)
        pawn.pos.y += evaluate_radial_pulses(pawn.pos.xz, signal.t_seconds);
    }

    // --- Orientation: heading + static terrain tilt
    if (coupling_active(COUPLING_TERRAIN_TO_PAWN_TILT)) {
        let normal = terrain_normal_at(pawn.pos.xz);

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

        let heading_quat = quat_from_axis_angle(vec3(0.0, 1.0, 0.0), pawn.heading);
        pawn.orientation = quat_multiply(tilt_quat, heading_quat);
    }

    // --- Portal proximity detection (GPU-authoritative)
    pawn.portal_trigger = -1;
    for (var pi = 0u; pi < portal_array.count; pi++) {
        let p = portal_array.portals[pi];
        let dx = pawn.pos.x - p.x;
        let dz = pawn.pos.z - p.z;
        let dist_sq = dx * dx + dz * dz;
        if (dist_sq < p.trigger_radius * p.trigger_radius) {
            pawn.portal_trigger = i32(p.arch_index);
            break;
        }
    }

    pawn_state = pawn;
}

@compute @workgroup_size(1)
fn update_camera() {
    if (!dynamics_0d_active()) { return; }

    var camera = camera_state;

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

    if (fpv_mode_active()) {
        camera.pos = pawn_state.pos + vec3(0.0, FPV_EYE_HEIGHT, 0.0);
    } else if (coupling_active(COUPLING_PAWN_TO_CAMERA_TARGET)) {
        camera.pos = coupling_pawn_to_camera_target(pawn_state.pos, camera);
    }

    // ─── Camera terrain clamp: never go underground ──────────────
    {
        let min_clearance = 1.5;  // minimum height above terrain
        let ground_at_cam = effective_ground_y(camera.pos.xz) + terrain_wave_overlay(camera.pos.xz);
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

    if (!sphere_frozen()) {
        var sphere = sphere_state;
        sphere.t = sphere.t + signal.dt;

        let sphere_updated = compose_sphere_from_orbit_pga(sphere.t);
        sphere.pos = sphere_updated.pos;
        sphere.orientation = sphere_updated.orientation;
        sphere_state = sphere;
    }

    if (signal_active() && coupling_active(COUPLING_POLYPHONY_TO_SPHERE_COLOR)) {
        sphere_state.color = coupling_signal_polyphony_to_sphere_color(
            signal.stats[0],
            sphere_state.color,
            dt
        );
    }

    if (coupling_active(COUPLING_SPHERE_TO_TERRAIN_TINT) && sphere_state.orbit_radius > 0.0) {
        terrain_state.tint = coupling_sphere_to_terrain_tint(sphere_state.pos);
    } else {
        terrain_state.tint = vec3(1.0);
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

    // Sun VP: kite coupling — sun orbits pawn at fixed offset
    if (coupling_active(COUPLING_PAWN_TO_SUN_VP)) {
        vp_data.light_vp = coupling_pawn_to_sun_vp(
            pawn_state.pos,
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
//   Evaluates ground_formed_with_complexity() once per texel, stores both
//   height and complexity in the scratch buffer (stride 2).
//   This is the only expensive call — terrain waves + piers + pyramids.
//
// Pass 2: generate_patch_gradients
//   Reads height from 5 scratch neighbors, reads complexity from 1.
//   Pure arithmetic: finite-difference gradients + textureStore.
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
    patch_height_scratch[base]      = hc.x;   // height
    patch_height_scratch[base + 1u] = hc.y;   // complexity
}

@compute @workgroup_size(16, 16)
fn generate_patch_gradients(@builtin(global_invocation_id) id: vec3<u32>) {
    let res = patch_params.resolution;
    if (id.x >= res || id.y >= res) { return; }

    let texel = vec2<i32>(id.xy);
    let layer = i32(patch_params.layer);
    let res_f = f32(res);
    let eps = patch_params.extent / (res_f - 1.0);

    let ix = id.x;
    let iy = id.y;
    let max_i = res - 1u;
    let height     = patch_height_scratch[(iy * res + ix) * 2u];
    let complexity = patch_height_scratch[(iy * res + ix) * 2u + 1u];

    // Gradient computation: central difference in interior,
    // 3-point one-sided stencil at edges for matching O(eps²) accuracy.
    //   Forward:  (-3h[0] + 4h[1] - h[2]) / (2*eps)
    //   Backward: ( 3h[N] - 4h[N-1] + h[N-2]) / (2*eps)

    var grad_x: f32;
    if (ix == 0u) {
        let h0 = height;
        let h1 = patch_height_scratch[(iy * res + 1u) * 2u];
        let h2 = patch_height_scratch[(iy * res + 2u) * 2u];
        grad_x = (-3.0 * h0 + 4.0 * h1 - h2) / (2.0 * eps);
    } else if (ix == max_i) {
        let h0 = height;
        let h1 = patch_height_scratch[(iy * res + max_i - 1u) * 2u];
        let h2 = patch_height_scratch[(iy * res + max_i - 2u) * 2u];
        grad_x = (3.0 * h0 - 4.0 * h1 + h2) / (2.0 * eps);
    } else {
        let h_px = patch_height_scratch[(iy * res + ix + 1u) * 2u];
        let h_mx = patch_height_scratch[(iy * res + ix - 1u) * 2u];
        grad_x = (h_px - h_mx) / (2.0 * eps);
    }

    var grad_z: f32;
    if (iy == 0u) {
        let h0 = height;
        let h1 = patch_height_scratch[(res + ix) * 2u];
        let h2 = patch_height_scratch[(2u * res + ix) * 2u];
        grad_z = (-3.0 * h0 + 4.0 * h1 - h2) / (2.0 * eps);
    } else if (iy == max_i) {
        let h0 = height;
        let h1 = patch_height_scratch[((max_i - 1u) * res + ix) * 2u];
        let h2 = patch_height_scratch[((max_i - 2u) * res + ix) * 2u];
        grad_z = (3.0 * h0 - 4.0 * h1 + h2) / (2.0 * eps);
    } else {
        let h_pz = patch_height_scratch[((iy + 1u) * res + ix) * 2u];
        let h_mz = patch_height_scratch[((iy - 1u) * res + ix) * 2u];
        grad_z = (h_pz - h_mz) / (2.0 * eps);
    }

    textureStore(patch_heightfield_array_write, texel, layer, vec4(height, grad_x, grad_z, complexity));
}

// --- Patch cell color generation (uses patchGen bind group layout)
struct CellFieldState {
    smooth_color: vec3<f32>,       // palette-interpolated base color
    discrete_color: vec3<f32>,     // per-cell flat color (chess + discrete)
    mode: f32,                     // [0,1] smooth → discrete tendency
    style: f32,                    // [0,1] blend → scatter transition style
    sparse: f32,                   // [0,1] sparse scatter field value
    cell_roll: f32,                // per-cell random [0,1] for survival tests
    sparse_roll: f32,              // per-cell random [0,1] for sparse survival
    archetype: u32,                // terrain type (0=mountain, 1=varied, 2=basin, 3=pool)
    world_xz: vec2<f32>,          // world-space position (for zone-level seed derivation)
}

// Stage 1: Evaluate all spatial fields at a cell's world position.
// Pure function — reads only spatial field lattices, no buffers.
fn evaluate_cell_fields(
    world_xz: vec2<f32>,
    cell_gx: i32,
    cell_gz: i32,
    cell_seed: u32
) -> CellFieldState {
    var s: CellFieldState;
    s.world_xz = world_xz;

    // Palette → smooth base color
    let palette_w = palette_field_at(world_xz);
    s.smooth_color = palette_color_smooth(palette_w, 0.5);

    // Discrete per-cell color (chess + color lattice + mono lattice)
    s.discrete_color = discrete_cell_color(world_xz, cell_gx, cell_gz, cell_seed);

    // Spatial fields
    s.mode = mode_field_at(world_xz);
    s.style = transition_style_at(world_xz);
    s.sparse = sparse_field_at(world_xz);

    // Per-cell rolls for survival tests (deterministic from seed)
    s.cell_roll = hash_property(cell_seed, 900u);
    s.sparse_roll = hash_property(cell_seed, 910u);

    // Archetype from tile grid (nearest-cell lookup, not interpolated)
    let cell_extent = tile_grid.cell_extent;
    let tile_gx = i32(floor(world_xz.x / cell_extent));
    let tile_gz = i32(floor(world_xz.y / cell_extent));
    let tile = tile_grid_lookup(tile_gx, tile_gz);
    s.archetype = tile.archetype;

    // ── Terrain-mode coupling ────────────────────────────────────
    // Where coupling is active, the terrain archetype shifts the mode
    // field toward smooth or discrete. The coupling direction is per-region
    // (stochastic), so the same archetype can push either way depending
    // on where in the world you are. Where coupling is near zero,
    // mode and terrain are independent (pre-coupling behavior).
    let coupling = terrain_coupling_at(world_xz);
    let c_strength = coupling.x;
    let c_direction = coupling.y;
    if (c_strength > 0.05) {
        let character = ARCHETYPE_MODE_CHARACTER[s.archetype];
        let shift = character * c_direction * c_strength * MODE_COUPLING_MAGNITUDE;
        s.mode = clamp(s.mode + shift, 0.0, 1.0);
    }

    return s;
}

// Stage 2: Composite final terrain color from evaluated field state.
fn composite_cell_color(s: CellFieldState) -> vec3<f32> {
    // Blend style: smooth → discrete (gradual transition at mode boundary)
    let blend_edge_lo = MODE_DISCRETE_THRESHOLD - 0.15;
    let blend_edge_hi = MODE_DISCRETE_THRESHOLD + 0.05;
    let blend_t = smoothstep(blend_edge_lo, blend_edge_hi, s.mode);
    let blend_color = mix(s.smooth_color, s.discrete_color, blend_t);

    // Scatter style: cell survives or is replaced by smooth background
    let core_edge = MODE_DISCRETE_THRESHOLD + 0.05;
    let scatter_edge = MODE_DISCRETE_THRESHOLD - 0.35;
    let survival = smoothstep(scatter_edge, core_edge, s.mode);
    let cell_visible_scatter = s.cell_roll < survival;
    let scatter_color = select(s.smooth_color, s.discrete_color, cell_visible_scatter);

    // Combine the two transition styles (blend vs scatter) via style field
    let mode_color = mix(blend_color, scatter_color, s.style);

    // Sparse scatter: isolated cells and small clusters outside mode zones
    let sparse_threshold = 0.22;
    let sparse_survival = smoothstep(sparse_threshold, sparse_threshold + 0.35, s.sparse);
    let cell_visible_sparse = s.sparse_roll < sparse_survival;

    let is_in_mode_zone = s.mode > scatter_edge;
    let show_cell = cell_visible_sparse && !is_in_mode_zone;
    return select(mode_color, s.discrete_color, show_cell);
}

// Biased variant: applies musical animation mode shifts to the compositing.
// mode_bias shifts smooth→discrete boundary; sparse_bias lowers scatter threshold.
fn composite_cell_color_biased(s: CellFieldState, mode_bias: f32, sparse_bias: f32) -> vec3<f32> {
    let biased_mode = clamp(s.mode + mode_bias, 0.0, 1.0);

    // Blend style: smooth → discrete (gradual transition at mode boundary)
    let blend_edge_lo = MODE_DISCRETE_THRESHOLD - 0.15;
    let blend_edge_hi = MODE_DISCRETE_THRESHOLD + 0.05;
    let blend_t = smoothstep(blend_edge_lo, blend_edge_hi, biased_mode);
    let blend_color = mix(s.smooth_color, s.discrete_color, blend_t);

    // Scatter style: cell survives or is replaced by smooth background
    let core_edge = MODE_DISCRETE_THRESHOLD + 0.05;
    let scatter_edge = MODE_DISCRETE_THRESHOLD - 0.35;
    let survival = smoothstep(scatter_edge, core_edge, biased_mode);
    let cell_visible_scatter = s.cell_roll < survival;
    let scatter_color = select(s.smooth_color, s.discrete_color, cell_visible_scatter);

    // Combine the two transition styles (blend vs scatter) via style field
    let mode_color = mix(blend_color, scatter_color, s.style);

    // Sparse scatter: threshold lowered by sparse_bias → more cells survive
    let sparse_threshold = max(0.22 - sparse_bias, 0.0);
    let sparse_survival = smoothstep(sparse_threshold, sparse_threshold + 0.35, s.sparse);
    let cell_visible_sparse = s.sparse_roll < sparse_survival;

    let is_in_mode_zone = biased_mode > scatter_edge;
    let show_cell = cell_visible_sparse && !is_in_mode_zone;
    return select(mode_color, s.discrete_color, show_cell);
}

// Target palette color from a continuous index [0,3].
// Integer values select a single palette; fractional values blend neighbors.
fn palette_target_color(palette_idx: f32, complexity: f32) -> vec3<f32> {
    let t = clamp(palette_idx, 0.0, 3.0);
    let lo = u32(floor(t));
    let hi = min(lo + 1u, 3u);
    let frac = t - floor(t);
    let color_lo = mix(PALETTE_LIGHT[lo], PALETTE_CENTER[lo], complexity);
    let color_hi = mix(PALETTE_LIGHT[hi], PALETTE_CENTER[hi], complexity);
    return mix(color_lo, color_hi, frac);
}

// Re-derive cell color at a world position with musical mode biases applied.
// Called from terrain FS when any mode parameter > 0.
//
// Palette drift strategy: composite TWICE with same structural logic.
//   Pass 1: original smooth + discrete colors (structural baseline)
//   Pass 2: smooth_color replaced by smooth target, discrete_color by discrete target
//   Final: mix(pass1, pass2, drift intensity)
// The compositor naturally routes smooth targets to smooth areas and discrete
// targets to discrete areas. No color bleeds across domain boundaries.
fn animated_cell_color(world_xz: vec2<f32>) -> vec3<f32> {
    let cell_size = PATCH_EXTENT / f32(PATCH_CELL_N);
    let cell_gx = i32(floor(world_xz.x / cell_size));
    let cell_gz = i32(floor(world_xz.y / cell_size));
    let cell_seed = lattice_node_seed(config.world_seed, vec2(cell_gx, cell_gz), 200u);

    let fields = evaluate_cell_fields(world_xz, cell_gx, cell_gz, cell_seed);
    let mode_bias = config.mode_color_shift;
    let sparse_bias = config.mode_checker_scatter;

    // Pass 1: original colors, biased thresholds
    let base = composite_cell_color_biased(fields, mode_bias, sparse_bias);

    // Pass 2: palette drift — each system drifts within its own vocabulary
    let drift = config.mode_palette_intensity;
    if (drift > 0.001) {
        var drifted = fields;

        // Smooth areas → target smooth palette tier
        drifted.smooth_color = palette_target_color(config.mode_palette_target, 0.5);

        // Discrete areas → target discrete color tier (chess/mono/color)
        let tier = u32(round(config.mode_discrete_tier));
        drifted.discrete_color = discrete_cell_color_at_tier(
            world_xz, cell_gx, cell_gz, cell_seed, tier);

        let drifted_color = composite_cell_color_biased(drifted, mode_bias, sparse_bias);
        return mix(base, drifted_color, drift * 0.95);
    }

    return base;
}

// Stage 3: Tag cell behavior mode from field state.
fn tag_cell_behavior(s: CellFieldState) -> f32 {
    // Only cells in clearly discrete zones are eligible
    if (s.mode < GOL_ZONE_MODE_THRESHOLD) { return 0.0; }

    // Zone-level seed: all cells in the same mode lattice cell share this
    let zone_node = vec2<i32>(floor(s.world_xz / MODE_LATTICE_SPACING));
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

    // Per-cell unique seed (from world grid position)
    let cell_size = patch_params.extent / f32(cell_n);
    let cell_gx = i32(floor(world_xz.x / cell_size));
    let cell_gz = i32(floor(world_xz.y / cell_size));
    let cell_seed = lattice_node_seed(patch_params.master_seed, vec2(cell_gx, cell_gz), 200u);

    // Stage 1: Evaluate all spatial fields
    let fields = evaluate_cell_fields(world_xz, cell_gx, cell_gz, cell_seed);

    // Stage 2: Composite final color
    let final_color = composite_cell_color(fields);

    // Stage 3: Behavior tag — packed into alpha channel
    // 0.0 = static (no animation), nonzero = animation mode + tier + flags
    let behavior_tag = tag_cell_behavior(fields);

    // Store: RGB = fully composited color, A = behavior tag
    textureStore(patch_cell_color_array_write, texel, layer, vec4(final_color, behavior_tag));
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
        // Pulse: continuous sinusoidal target per cell (no neighbor rules)
        // Writes every frame (not tick-gated) for smooth animation.
        // The target is a continuous [0,1] blend factor.
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







fn zone_emit_quad(
    v0: vec3<f32>, v1: vec3<f32>, v2: vec3<f32>, v3: vec3<f32>,
    n: vec3<f32>, uv: vec2<f32>, color: vec3<f32>
) {
    let vi = atomicAdd(&zone_mesh_indirect[5], 4u);
    let ii = atomicAdd(&zone_mesh_indirect[0], 6u);
    if (vi + 4u > ZONE_MESH_MAX_VERTICES || ii + 6u > ZONE_MESH_MAX_INDICES) { return; }

    zone_mesh_vertices[vi + 0u] = CellMeshVertex(v0.x, v0.y, v0.z, n.x, n.y, n.z, uv.x, uv.y, color.x, color.y, color.z);
    zone_mesh_vertices[vi + 1u] = CellMeshVertex(v1.x, v1.y, v1.z, n.x, n.y, n.z, uv.x, uv.y, color.x, color.y, color.z);
    zone_mesh_vertices[vi + 2u] = CellMeshVertex(v2.x, v2.y, v2.z, n.x, n.y, n.z, uv.x, uv.y, color.x, color.y, color.z);
    zone_mesh_vertices[vi + 3u] = CellMeshVertex(v3.x, v3.y, v3.z, n.x, n.y, n.z, uv.x, uv.y, color.x, color.y, color.z);

    zone_mesh_indices[ii + 0u] = vi;
    zone_mesh_indices[ii + 1u] = vi + 1u;
    zone_mesh_indices[ii + 2u] = vi + 2u;
    zone_mesh_indices[ii + 3u] = vi;
    zone_mesh_indices[ii + 4u] = vi + 2u;
    zone_mesh_indices[ii + 5u] = vi + 3u;
}

// Zone mesh terrain height → ground_formed level (terrain + solids + pyramids).
// GoL extrusions sit on formed ground, not on each other.
fn zone_terrain_height(world_xz: vec2<f32>) -> f32 {
    return ground_formed(world_xz);
}

fn zone_mesh_gen_cell(zone_id: u32, cx: u32, cy: u32) {
    let z = zone_config.zones[zone_id];
    let base = zone_id * GOL_ZONE_STRIDE;
    let idx = cy * z.grid_size + cx;

    // Height from visual value × alive_height × per-cell height factor
    let visual = zone_life[base + GOL_CELL_VISUAL + idx];
    let height_factor = zone_life[base + GOL_CELL_HEIGHT_FACTOR + idx];
    let h = z.alive_height * visual * height_factor * config.mode_gol_height_scale;
    if (h < 0.05) { return; }

    let cell_size = z.extent / f32(z.grid_size);
    let corner_x = z.origin.x - z.extent * 0.5;
    let corner_z = z.origin.y - z.extent * 0.5;

    let x0 = corner_x + f32(cx) * cell_size;
    let x1 = x0 + cell_size;
    let z0 = corner_z + f32(cy) * cell_size;
    let z1 = z0 + cell_size;

    // Terrain height at corners — from baked heightfield for exact alignment
    let th00 = zone_sample_baked_terrain_y(vec2(x0, z0));
    let th10 = zone_sample_baked_terrain_y(vec2(x1, z0));
    let th01 = zone_sample_baked_terrain_y(vec2(x0, z1));
    let th11 = zone_sample_baked_terrain_y(vec2(x1, z1));

    // Pre-compute color ONCE per cell in compute pass (not per pixel in fragment)
    // Evaluates full composite terrain pipeline + applies GoL color mode differential
    let cell_center = vec2(x0 + cell_size * 0.5, z0 + cell_size * 0.5);
    let color = apply_gol_extrusion_color(cell_center, z, cx, cy);

    // Store terrain height at cell center in ux for VS suppression
    let cell_terrain_y = zone_sample_baked_terrain_y(cell_center);
    let vert_uv = vec2(cell_terrain_y, 0.0);  // ux = terrain_y for suppression

    // Top cap — uv.x carries terrain_y for VS suppression
    let top00 = vec3(x0, th00 + h, z0);
    let top10 = vec3(x1, th10 + h, z0);
    let top01 = vec3(x0, th01 + h, z1);
    let top11 = vec3(x1, th11 + h, z1);
    zone_emit_quad(top00, top01, top11, top10, vec3(0.0, 1.0, 0.0), vert_uv, color);

    // Side walls — only where this cell is taller than neighbor
    let gs = z.grid_size;
    // +X wall
    {
        var nh: f32 = 0.0;
        if (cx + 1u < gs) {
            let ni = cy * gs + cx + 1u;
            nh = zone_life[base + GOL_CELL_VISUAL + ni] * z.alive_height * zone_life[base + GOL_CELL_HEIGHT_FACTOR + ni];
        }
        if (nh < h) {
            let bot10 = vec3(x1, select(th10, th10 + nh, nh > 0.05), z0);
            let bot11 = vec3(x1, select(th11, th11 + nh, nh > 0.05), z1);
            zone_emit_quad(top10, top11, bot11, bot10, vec3(1.0, 0.0, 0.0), vert_uv, color);
        }
    }
    // -X wall
    {
        var nh: f32 = 0.0;
        if (cx > 0u) {
            let ni = cy * gs + cx - 1u;
            nh = zone_life[base + GOL_CELL_VISUAL + ni] * z.alive_height * zone_life[base + GOL_CELL_HEIGHT_FACTOR + ni];
        }
        if (nh < h) {
            let bot00 = vec3(x0, select(th00, th00 + nh, nh > 0.05), z0);
            let bot01 = vec3(x0, select(th01, th01 + nh, nh > 0.05), z1);
            zone_emit_quad(top01, top00, bot00, bot01, vec3(-1.0, 0.0, 0.0), vert_uv, color);
        }
    }
    // +Z wall
    {
        var nh: f32 = 0.0;
        if (cy + 1u < gs) {
            let ni = (cy + 1u) * gs + cx;
            nh = zone_life[base + GOL_CELL_VISUAL + ni] * z.alive_height * zone_life[base + GOL_CELL_HEIGHT_FACTOR + ni];
        }
        if (nh < h) {
            let bot01 = vec3(x0, select(th01, th01 + nh, nh > 0.05), z1);
            let bot11 = vec3(x1, select(th11, th11 + nh, nh > 0.05), z1);
            zone_emit_quad(top11, top01, bot01, bot11, vec3(0.0, 0.0, 1.0), vert_uv, color);
        }
    }
    // -Z wall
    {
        var nh: f32 = 0.0;
        if (cy > 0u) {
            let ni = (cy - 1u) * gs + cx;
            nh = zone_life[base + GOL_CELL_VISUAL + ni] * z.alive_height * zone_life[base + GOL_CELL_HEIGHT_FACTOR + ni];
        }
        if (nh < h) {
            let bot00 = vec3(x0, select(th00, th00 + nh, nh > 0.05), z0);
            let bot10 = vec3(x1, select(th10, th10 + nh, nh > 0.05), z0);
            zone_emit_quad(top00, top10, bot10, bot00, vec3(0.0, 0.0, -1.0), vert_uv, color);
        }
    }
}

@compute @workgroup_size(1, 1, 1)
fn zone_gol_mesh_reset() {
    atomicStore(&zone_mesh_indirect[0], 0u);  // indexCount
    atomicStore(&zone_mesh_indirect[1], 1u);  // instanceCount
    atomicStore(&zone_mesh_indirect[2], 0u);  // firstIndex
    atomicStore(&zone_mesh_indirect[3], 0u);  // baseVertex
    atomicStore(&zone_mesh_indirect[4], 0u);  // firstInstance
    atomicStore(&zone_mesh_indirect[5], 0u);  // vertex alloc counter
}

@compute @workgroup_size(8, 8, 1)
fn zone_gol_mesh_gen(@builtin(global_invocation_id) gid: vec3<u32>) {
    let zone_id = gid.z;
    if (zone_id >= zone_config.count) { return; }
    let z = zone_config.zones[zone_id];
    if (z.transition_fraction <= 0.0) { return; }
    if (z.alive_height < 0.01) { return; }  // skip flat zones (Flash, no-height)
    let cell = gid.xy;
    if (cell.x >= z.grid_size || cell.y >= z.grid_size) { return; }

    zone_mesh_gen_cell(zone_id, cell.x, cell.y);
}

// --- Zone extrusion render shaders

struct ZoneExtrusionVarying {
    @builtin(position) clip_pos: vec4<f32>,
    @location(0) world_pos: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) cell_color: vec3<f32>,    // pre-computed in compute pass
}

@vertex
fn zone_extrusion_vs(
    @location(0) pos: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
    @location(3) color: vec3<f32>
) -> ZoneExtrusionVarying {
    var world_pos = pos;

    // uv.x carries terrain_y (base height without extrusion)
    let terrain_y = uv.x;
    let wave_y = terrain_wave_overlay(pos.xz);

    // Suppression target = terrain + aura height + wave overlay
    let aura = sample_pawn_aura(pos.xz, render_pawn.pos.xz);
    let ground_target = terrain_y + aura.r * config.pawn_aura_height + wave_y;

    // Wave overlay: lift entire extrusion mesh with animated terrain
    world_pos.y += wave_y;

    // Pawn proximity suppression: flatten extrusion blocks toward effective ground.
    // Uses same radii as zone_gol_height_at so VS geometry matches pawn Y resolve.
    let pawn_dist = distance(pos.xz, render_pawn.pos.xz);
    let suppression = 1.0 - smoothstep(ZONE_SUPPRESS_INNER, ZONE_SUPPRESS_OUTER, pawn_dist);
    if (suppression > 0.001) {
        world_pos.y = mix(pos.y, ground_target, suppression);
    }

    var out: ZoneExtrusionVarying;
    out.clip_pos = render_vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.normal = normal;
    out.cell_color = color;
    return out;
}

@fragment
fn zone_extrusion_fs(in: ZoneExtrusionVarying) -> @location(0) vec4<f32> {
    let n = normalize(in.normal);
    var block_color = clamp(in.cell_color, vec3(0.0), vec3(1.0));

    // Pawn force field tint on extrusion blocks (render context)
    let pawn_ff = 1.0 - zone_pawn_ff(in.world_pos.xz, render_pawn.pos, render_pawn.velocity);
    if (pawn_ff > 0.01) {
        block_color = mix(block_color, ZONE_PAWN_TINT, pawn_ff * ZONE_PAWN_TINT_STRENGTH);
    }

    // Sphere force field tint on extrusion blocks (render context)
    let sphere_ff = 1.0 - zone_sphere_ff(in.world_pos.xz, render_sphere.pos);
    if (sphere_ff > 0.01) {
        block_color = mix(block_color, ZONE_SPHERE_TINT, sphere_ff * ZONE_SPHERE_TINT_STRENGTH);
    }

    // Pawn aura: persistent tinting from toroidal spring grid
    {
        let aura = sample_pawn_aura(in.world_pos.xz, render_pawn.pos.xz);
        let aura_active = max(aura.r, max(abs(aura.g), max(abs(aura.b), abs(aura.a))));
        if (aura_active > 0.01) {
            block_color = clamp(block_color + aura.gba, vec3(0.0), vec3(1.0));
            let height_boost = aura.r * 0.12;
            block_color = clamp(block_color + vec3(height_boost), vec3(0.0), vec3(1.0));
        }
    }

    // Wall boost: vertical faces get extra ambient
    let is_wall = abs(n.y) < 0.1;
    let wall_boost = select(vec3(0.0), block_color * 0.15, is_wall);

    return vec4(shade_lit(in.world_pos, n, block_color) + wall_boost, 1.0);
}

@vertex
fn shadow_zone_extrusion_vs(
    @location(0) pos: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) uv: vec2<f32>,
    @location(3) color: vec3<f32>
) -> ShadowVarying {
    var world_pos = pos;
    let terrain_y = uv.x;
    let wave_y = terrain_wave_overlay(pos.xz);
    world_pos.y += wave_y;
    let pawn_dist = distance(pos.xz, render_pawn.pos.xz);
    let suppression = 1.0 - smoothstep(ZONE_SUPPRESS_INNER, ZONE_SUPPRESS_OUTER, pawn_dist);
    if (suppression > 0.001) {
        // Shadow doesn't have aura texture — use terrain_y + wave only
        world_pos.y = mix(pos.y + wave_y, terrain_y + wave_y, suppression);
    }
    var out: ShadowVarying;
    out.clip_pos = render_vp.light_vp * vec4(world_pos, 1.0);
    return out;
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

    let pawn_xz = pawn_state.pos.xz;
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
                let heading = pawn_state.heading;
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


// §8 SELF-PORTRAIT GALLERY — Paintings of the pawn distributed over terrain
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
@group(0) @binding(144) var<storage, read> photo_patch_instances: array<PatchInstance>;
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

struct PyramidGroundEntry {
    center_x: f32,
    center_z: f32,
    ground_y: f32,
    own_height: f32,
    is_active: u32,
    half_x: f32,
    half_z: f32,
    rotation: f32,
};
@group(0) @binding(149) var<storage, read_write> pyramid_ground: array<PyramidGroundEntry, 8>;

struct PalmGroundEntry {
    center_x: f32,
    center_z: f32,
    ground_y: f32,
    is_active: u32,
    _pad0: f32, _pad1: f32, _pad2: f32, _pad3: f32,
}
@group(0) @binding(150) var<storage, read_write> palm_ground: array<PalmGroundEntry, 24>;
@group(0) @binding(151) var<storage, read_write> cactus_ground: array<CactusGroundEntry, 20>;

// --- Terrain Height Sampling
fn sample_terrain_y_at(world_xz: vec2<f32>, patch_count: u32) -> f32 {
    for (var i = 0u; i < patch_count; i++) {
        let pi = photo_patch_instances[i];
        let half = pi.extent * 0.5;
        let local = world_xz - pi.origin;
        if (abs(local.x) < half && abs(local.y) < half) {
            let uv = local / pi.extent + 0.5;
            // Remap UV to texel centers (same as patch_terrain_vs)
            let res = f32(PATCH_HEIGHTFIELD_N);
            let sample_uv = (uv * (res - 1.0) + 0.5) / res;
            let h = textureSampleLevel(photo_heightfield, photo_sampler,
                                       sample_uv, i32(pi.layer), 0.0);
            return h.x;
        }
    }
    return 0.0;
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
    let pawn_pos = pawn_state.pos;
    let pc = cfg.patch_count;

    // --- Camera position: spherical offset from pawn
    let cos_el = cos(cfg.elevation);
    let sin_el = sin(cfg.elevation);
    let cos_az = cos(cfg.azimuth);
    let sin_az = sin(cfg.azimuth);

    let eye_raw = pawn_pos + vec3(
        cfg.distance * cos_el * sin_az,
        cfg.distance * sin_el + 1.5,
        cfg.distance * cos_el * cos_az
    );

    // --- Clamp camera above actual terrain
    let terrain_at_cam = sample_terrain_y_at(eye_raw.xz, pc);
    let eye = vec3(eye_raw.x, max(eye_raw.y, terrain_at_cam + 0.1), eye_raw.z);

    // --- Build VP looking at pawn, with frame offset
    let aim_base = pawn_pos + vec3(0.0, 1.0, 0.0);

    // Derive camera frame from initial eye→pawn direction
    let fwd_init = normalize(aim_base - eye);
    var world_up_init = vec3(0.0, 1.0, 0.0);
    if (abs(fwd_init.y) > 0.99) { world_up_init = vec3(0.0, 0.0, 1.0); }
    let right_init = normalize(cross(fwd_init, world_up_init));
    let up_init = cross(right_init, fwd_init);

    // Scale offset by FOV angular extent at the pawn's distance
    let half_fov = cfg.fov_rad * 0.5;
    let cam_to_pawn = length(aim_base - eye);
    let h_extent = tan(half_fov) * cam_to_pawn * cfg.aspect_ratio;
    let v_extent = tan(half_fov) * cam_to_pawn;

    // Shift aim point — camera looks away from pawn, placing pawn off-center
    let aim_pt = aim_base
        - right_init * cfg.frame_offset_x * h_extent
        - up_init * cfg.frame_offset_y * v_extent;

    photographer_vp.m = build_lookat_vp(eye, aim_pt, cfg.fov_rad, cfg.aspect_ratio);
    photographer_vp.light_vp = coupling_pawn_to_sun_vp(pawn_pos, cfg.sun_direction);
    photographer_camera_out.pos = eye;
}

// --- Entity Placement Y-Correction
//
// Arches, columns, pyramids: ground_y is written directly by the CPU
// in upload_ground_entries() using cpu_terrain_base_at + own_pier_height.
// No GPU sampling needed — deterministic, no foreign pier contamination.
//
// Paintings: still need per-frame GPU evaluation because they ride on
// dynamic GoL zone extrusions that only exist on the GPU.
@compute @workgroup_size(1)
fn compute_entity_placement() {
    let pc = config.placement_patch_count;

    // Y-correct all outdoor paintings (terrain quads + wall frame monuments).
    // Indoor wall frames use sentinel patch coords (0x7FFFFFFF) and are skipped.
    for (var i = 0u; i < 32u; i++) {
        if (photo_painting_slots[i].is_active != 0u &&
            photo_painting_slots[i].patch_gx != 0x7FFFFFFF) {
            let slot_xz = vec2(
                photo_painting_slots[i].position.x,
                photo_painting_slots[i].position.z
            );
            let ground = sample_terrain_y_at(slot_xz, pc) + zone_gol_height_at(slot_xz);
            // Terrain quads: center at ground + half-height (bottom at ground)
            // Wall frames: also lift by frame_width so the frame border clears the ground
            var lift = photo_painting_slots[i].scale_y * 0.5;
            if (photo_painting_slots[i].form_type == FORM_WALL_FRAME) {
                lift += photo_painting_slots[i].frame_width;
            }
            photo_painting_slots[i].position.y = ground + lift;
        }
    }

    // Arches, columns, pyramids: ground_y already set by CPU.
    // No action needed here.
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

    // Skip inactive or wall-frame slots (only terrain quads drawn here)
    if (slot.is_active == 0u || slot.form_type != FORM_TERRAIN_QUAD) {
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

// §8.1.1 GALLERY FRAME SHADOW — depth-only pass for terrain-quad paintings
// Same instanced subdivided quad geometry, projected through light_vp.

@vertex
fn shadow_gallery_frame_vs(
    @builtin(vertex_index) vid: u32,
    @builtin(instance_index) iid: u32,
) -> ShadowVarying {
    var out: ShadowVarying;
    let slot = painting_slots[iid];

    if (slot.is_active == 0u || slot.form_type != FORM_TERRAIN_QUAD) {
        out.clip_pos = vec4(0.0, 0.0, 0.0, 1.0);
        return out;
    }

    let N = GALLERY_QUAD_N;
    let tri_in_quad = vid / 3u;
    let vert_in_tri = vid % 3u;
    let quad_idx = tri_in_quad / 2u;
    let second_tri = tri_in_quad % 2u;
    let qx = quad_idx % N;
    let qy = quad_idx / N;

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

    var local = vec3(
        (uv.x - 0.5) * slot.scale_x,
        (uv.y - 0.5) * slot.scale_y,
        0.0
    );
    local = deform_gallery_frame(local, uv, slot.geometry_seed);

    let fwd = normalize(slot.forward);
    let up_raw = normalize(slot.up);
    let right = normalize(cross(up_raw, fwd));
    let up = cross(fwd, right);
    let world = slot.position + right * local.x + up * local.y + fwd * local.z;

    out.clip_pos = render_vp.light_vp * vec4(world, 1.0);
    return out;
}

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
    out.world_pos.y += terrain_wave_overlay(out.world_pos.xz);
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

// §8.3 WALL PAINTING SHADOW — depth-only pass for framed paintings
// Reuses compute_wall_painting_geometry for identical mesh, projects through light_vp.

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

    if (slot.is_active == 0u || slot.form_type != FORM_WALL_FRAME) {
        var out: ShadowVarying;
        out.clip_pos = vec4(0.0);
        return out;
    }

    var geom = compute_wall_painting_geometry(slot, pidx, local_vid);
    geom.world_pos.y += terrain_wave_overlay(geom.world_pos.xz);
    var out: ShadowVarying;
    out.clip_pos = render_vp.light_vp * vec4(geom.world_pos, 1.0);
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
// Three families: pyramids (§9.0), arches (§9.1), columns (§9.2).
//
// Vertex format: matches ArchVertex (pos[3], normal[3], color[3], index:u32)
// = 10 × f32 per vertex = 40 bytes. VB is accessed as array<f32>.

// §9.0 PYRAMID MESH GENERATION

const PMG_MAX_VERTS_PER_SLOT: u32  = 36u;   // truncated: 12 tris × 3 (sides + top + bottom)
const PMG_MAX_INDICES_PER_SLOT: u32 = 36u;  // unindexed triangles (1:1 vert:idx)
const PMG_FLOATS_PER_VERTEX: u32   = 10u;   // pos(3) + normal(3) + color(3) + index(1)
const PMG_MAX_SLOTS: u32           = 8u;

// Total index count for drawIndexed: all slots × max indices per slot.
// Inactive slots have degenerate (index 0) entries → zero-area triangles.
const PMG_TOTAL_INDICES: u32 = 288u;        // 8 × 36

// ── Parameter buffer (CPU writes per-slot on spawn/evict) ─────────────

struct PyramidMeshParams {
    center_x: f32,
    center_z: f32,
    rotation: f32,
    half_x: f32,
    half_z: f32,
    height: f32,
    truncation: f32,
    color_r: f32,
    color_g: f32,
    color_b: f32,
    is_active: u32,
    _pad: u32,
}

// ── Bindings (dedicated layout — isolated from terrain evaluation) ────

@group(0) @binding(190) var<storage, read>       pmg_params: array<PyramidMeshParams, 8>;
@group(0) @binding(191) var<storage, read_write>  pmg_vertices: array<f32>;
@group(0) @binding(192) var<storage, read_write>  pmg_indices: array<u32>;

// ── Vertex writer ─────────────────────────────────────────────────────

fn pmg_write_vertex(
    abs_idx: u32,
    px: f32, py: f32, pz: f32,
    nx: f32, ny: f32, nz: f32,
    cr: f32, cg: f32, cb: f32,
    entity_idx: u32
) {
    let i = abs_idx * PMG_FLOATS_PER_VERTEX;
    pmg_vertices[i + 0u] = px;
    pmg_vertices[i + 1u] = py;
    pmg_vertices[i + 2u] = pz;
    pmg_vertices[i + 3u] = nx;
    pmg_vertices[i + 4u] = ny;
    pmg_vertices[i + 5u] = nz;
    pmg_vertices[i + 6u] = cr;
    pmg_vertices[i + 7u] = cg;
    pmg_vertices[i + 8u] = cb;
    pmg_vertices[i + 9u] = f32(entity_idx);
}

// ── Geometry helpers ──────────────────────────────────────────────────

fn pmg_to_world(lx: f32, ly: f32, lz: f32, cx: f32, cz: f32, co: f32, si: f32) -> vec3<f32> {
    return vec3(cx + lx * co - lz * si, ly, cz + lx * si + lz * co);
}

fn pmg_face_normal(v0: vec3<f32>, v1: vec3<f32>, v2: vec3<f32>) -> vec3<f32> {
    let n = cross(v1 - v0, v2 - v0);
    let l = length(n);
    if (l > 0.0001) { return n / l; }
    return vec3(0.0, 1.0, 0.0);
}

// Emit one triangle: 3 vertices + 3 indices. Returns next cursor value.
fn pmg_emit_tri(
    slot_vb: u32, slot_ib: u32, cursor: u32,
    v0: vec3<f32>, v1: vec3<f32>, v2: vec3<f32>,
    cr: f32, cg: f32, cb: f32, slot: u32
) -> u32 {
    let n = pmg_face_normal(v0, v1, v2);
    let abs_v = slot_vb + cursor;

    pmg_write_vertex(abs_v + 0u, v0.x, v0.y, v0.z, n.x, n.y, n.z, cr, cg, cb, slot);
    pmg_write_vertex(abs_v + 1u, v1.x, v1.y, v1.z, n.x, n.y, n.z, cr, cg, cb, slot);
    pmg_write_vertex(abs_v + 2u, v2.x, v2.y, v2.z, n.x, n.y, n.z, cr, cg, cb, slot);

    let abs_i = slot_ib + cursor;
    pmg_indices[abs_i + 0u] = abs_v;
    pmg_indices[abs_i + 1u] = abs_v + 1u;
    pmg_indices[abs_i + 2u] = abs_v + 2u;

    return cursor + 3u;
}

// ── Compute entry point ───────────────────────────────────────────────
//
// Dispatch: (8, 1, 1) — one invocation per slot.
// Each invocation writes its slot's fixed region of the VB/IB.
// Inactive slots get degenerate indices (all pointing to vertex 0).

@compute @workgroup_size(1, 1, 1)
fn pyramid_mesh_gen(@builtin(global_invocation_id) gid: vec3<u32>) {
    let slot = gid.x;
    if (slot >= PMG_MAX_SLOTS) { return; }

    let p = pmg_params[slot];
    let slot_vb = slot * PMG_MAX_VERTS_PER_SLOT;
    let slot_ib = slot * PMG_MAX_INDICES_PER_SLOT;

    // ── Inactive: write degenerate indices ───────────────────────
    if (p.is_active == 0u) {
        for (var i = 0u; i < PMG_MAX_INDICES_PER_SLOT; i++) {
            pmg_indices[slot_ib + i] = slot_vb;
        }
        return;
    }

    // ── Active: build geometry ────────────────────────────────────
    let co = cos(p.rotation);
    let si = sin(p.rotation);
    let cx = p.center_x;
    let cz = p.center_z;

    // Base corners (Y = 0, ground-relative — VS adds ground_y)
    let b00 = pmg_to_world(-p.half_x, 0.0, -p.half_z, cx, cz, co, si);
    let b10 = pmg_to_world( p.half_x, 0.0, -p.half_z, cx, cz, co, si);
    let b11 = pmg_to_world( p.half_x, 0.0,  p.half_z, cx, cz, co, si);
    let b01 = pmg_to_world(-p.half_x, 0.0,  p.half_z, cx, cz, co, si);

    let cr = p.color_r;
    let cg = p.color_g;
    let cb = p.color_b;

    var vi = 0u;   // vertex/index cursor (relative to slot base)

    if (p.truncation < 0.01) {
        // ── Pointed: 4 triangular faces + bottom cap ─────────────
        let apex = pmg_to_world(0.0, p.height, 0.0, cx, cz, co, si);

        // Winding matches CPU: bases[(face+1)%4], bases[face], apex
        vi = pmg_emit_tri(slot_vb, slot_ib, vi, b10, b00, apex, cr, cg, cb, slot);
        vi = pmg_emit_tri(slot_vb, slot_ib, vi, b11, b10, apex, cr, cg, cb, slot);
        vi = pmg_emit_tri(slot_vb, slot_ib, vi, b01, b11, apex, cr, cg, cb, slot);
        vi = pmg_emit_tri(slot_vb, slot_ib, vi, b00, b01, apex, cr, cg, cb, slot);
        // Bottom cap: b00, b11, b01 + b00, b10, b11 (CW from above → -Y normal)
        vi = pmg_emit_tri(slot_vb, slot_ib, vi, b00, b11, b01, cr, cg, cb, slot);
        vi = pmg_emit_tri(slot_vb, slot_ib, vi, b00, b10, b11, cr, cg, cb, slot);
        // vi = 18

    } else {
        // ── Truncated: 4 side quads + 1 top cap + bottom cap ─────
        let tx = p.half_x * p.truncation;
        let tz = p.half_z * p.truncation;
        let t00 = pmg_to_world(-tx, p.height, -tz, cx, cz, co, si);
        let t10 = pmg_to_world( tx, p.height, -tz, cx, cz, co, si);
        let t11 = pmg_to_world( tx, p.height,  tz, cx, cz, co, si);
        let t01 = pmg_to_world(-tx, p.height,  tz, cx, cz, co, si);

        // Side quads: emit_quad(bases[(f+1)%4], bases[f], tops[f], tops[(f+1)%4])
        // Face 0: b10, b00 → t00, t10
        vi = pmg_emit_tri(slot_vb, slot_ib, vi, b10, b00, t00, cr, cg, cb, slot);
        vi = pmg_emit_tri(slot_vb, slot_ib, vi, b10, t00, t10, cr, cg, cb, slot);
        // Face 1: b11, b10 → t10, t11
        vi = pmg_emit_tri(slot_vb, slot_ib, vi, b11, b10, t10, cr, cg, cb, slot);
        vi = pmg_emit_tri(slot_vb, slot_ib, vi, b11, t10, t11, cr, cg, cb, slot);
        // Face 2: b01, b11 → t11, t01
        vi = pmg_emit_tri(slot_vb, slot_ib, vi, b01, b11, t11, cr, cg, cb, slot);
        vi = pmg_emit_tri(slot_vb, slot_ib, vi, b01, t11, t01, cr, cg, cb, slot);
        // Face 3: b00, b01 → t01, t00
        vi = pmg_emit_tri(slot_vb, slot_ib, vi, b00, b01, t01, cr, cg, cb, slot);
        vi = pmg_emit_tri(slot_vb, slot_ib, vi, b00, t01, t00, cr, cg, cb, slot);
        // Top cap: t00, t01, t11, t10 (CCW from above → +Y normal)
        vi = pmg_emit_tri(slot_vb, slot_ib, vi, t00, t01, t11, cr, cg, cb, slot);
        vi = pmg_emit_tri(slot_vb, slot_ib, vi, t00, t11, t10, cr, cg, cb, slot);
        // Bottom cap: b00, b11, b01 + b00, b10, b11 (CW from above → -Y normal)
        vi = pmg_emit_tri(slot_vb, slot_ib, vi, b00, b11, b01, cr, cg, cb, slot);
        vi = pmg_emit_tri(slot_vb, slot_ib, vi, b00, b10, b11, cr, cg, cb, slot);
        // vi = 36
    }

    // ── Zero remaining indices (pointed: 12→30, truncated: fills 30) ─
    for (var i = vi; i < PMG_MAX_INDICES_PER_SLOT; i++) {
        pmg_indices[slot_ib + i] = slot_vb;
    }
}


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
    let shaft_top_y = p.height - p.capital_height;
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

    const TIER_ANTENNA_FIRST: u32 = 3u;  // ANTENNA=3, ANTENNA_SQUAT=4, ANTENNA_COLOSSAL=5

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
            prof_r[pc] = r_final; prof_y[pc] = p.height - p.burial;
            prof_cr[pc] = base_cr; prof_cg[pc] = base_cg; prof_cb[pc] = base_cb; pc++;
        } else {
            prof_r[pc] = top_radius; prof_y[pc] = p.height - p.burial;
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
            disc_y[dc] = p.height - p.burial; disc_ny[dc] = 1.0;
            disc_cr[dc] = base_cr; disc_cg[dc] = base_cg; disc_cb[dc] = base_cb;
            dc++;
        } else {
            // Simple top cap
            disc_ri[dc] = 0.0; disc_ro[dc] = top_radius;
            disc_y[dc] = p.height - p.burial; disc_ny[dc] = 1.0;
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
    let ground_y = render_palm_ground[idx].ground_y;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += terrain_wave_overlay(world_pos.xz);
    var out: EntityVarying;
    out.clip_pos = render_vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.normal = in.normal;
    out.entity_color = in.color;
    return out;
}

@vertex
fn shadow_palm_vs(in: ArchVertexInput) -> ShadowVarying {
    let idx = u32(in.arch_index);
    let ground_y = render_palm_ground[idx].ground_y;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += terrain_wave_overlay(world_pos.xz);
    var out: ShadowVarying;
    out.clip_pos = render_vp.light_vp * vec4(world_pos, 1.0);
    return out;
}

// ─── §9.4 CACTUS MESH GENERATION (ribbed columnar trunk + forking arms) ──

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
    let ground_y = render_cactus_ground[idx].ground_y;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += terrain_wave_overlay(world_pos.xz);
    var out: EntityVarying;
    out.clip_pos = render_vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.normal = in.normal;
    out.entity_color = in.color;
    return out;
}

@vertex
fn shadow_cactus_vs(in: ArchVertexInput) -> ShadowVarying {
    let idx = u32(in.arch_index);
    let ground_y = render_cactus_ground[idx].ground_y;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += terrain_wave_overlay(world_pos.xz);
    var out: ShadowVarying;
    out.clip_pos = render_vp.light_vp * vec4(world_pos, 1.0);
    return out;
}

// ─── §9.5 BLADE CLUSTER MESH GENERATION ─────────────────────────

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
    let ground_y = render_blade_ground[idx].ground_y;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += terrain_wave_overlay(world_pos.xz);
    var out: EntityVarying;
    out.clip_pos = render_vp.m * vec4(world_pos, 1.0);
    out.world_pos = world_pos;
    out.normal = in.normal;
    out.entity_color = in.color;
    return out;
}

@vertex
fn shadow_blade_cluster_vs(in: ArchVertexInput) -> ShadowVarying {
    let idx = u32(in.arch_index);
    let ground_y = render_blade_ground[idx].ground_y;
    var world_pos = in.pos + vec3(0.0, ground_y, 0.0);
    world_pos.y += terrain_wave_overlay(world_pos.xz);
    var out: ShadowVarying;
    out.clip_pos = render_vp.light_vp * vec4(world_pos, 1.0);
    return out;
}

// END OF SCROLL
