// ─── ribbon.inl ──────────────────────────────────────────────────
//
// Sky Ribbon: complete subsystem (vocabulary + machinery in one
// module). Single-instance, bespoke pipeline — runs through the
// 3-phase select/place/commit shape but doesn't share entity_pipeline's
// generic machinery.
//
// THE HEAD IS THE INSTRUMENT; THE BODY IS THE LAW'S CONSEQUENCE.
// Every author — player, wanderer, and (soon) the musical canvas —
// plays the head through one steering integrator and one altitude
// pen; the propagation law replays the head's past down the body at
// P. Couple the head, and the rest follows.
//
// Flying ribbons: compound wave functions (lateral + vertical sway)
// forming square-tube geometry in the sky. Each ribbon is a tier
// instance with Gaussian-sampled parameters.
//
// ┌─── Public surface (called from outside this file) ──────────────┐
// │                                                                  │
// │  Module functions are static, take RibbonState& explicitly.      │
// │  This makes ribbon's state ownership language-visible and        │
// │  cross-cutting dependencies explicit in function signatures.     │
// │                                                                  │
// │  Lifecycle (three-phase):                                        │
// │    select_ribbon_for_patch(rs, c, gx, gz, sel)  — Phase 1: roll  │
// │    place_ribbon_from_selection(c, sel, plan)    — Phase 2: place │
// │      (note: takes no RibbonState — only mediates between sel     │
// │       and spawn-engine helpers; not part of ribbon's data)       │
// │    commit_ribbon(rs, c, plan, gx, gz, queue)    — Phase 3: state │
// │                                                                  │
// │  Frame conductor (ONE call per frame from update()):             │
// │    ribbon_frame_tick(rs, c, queue)                               │
// │      — author selection, slot hold/adopt, uploads, advance       │
// │                                                                  │
// │  Head mover (called by the conductor):                           │
// │    ribbon_advance_head(rs, gpuState, queue, ribbon, slot, …)     │
// │    ribbon_head_pose(rs, x, y, z, h)   — the SADDLE (mount)       │
// │    ribbon_head_pen(rs, x, z, h)       — the PEN (steering reads) │
// │    ribbon_invalidate_head(rs), ribbon_head_is(rs, slot)          │
// │                                                                  │
// │  Shared geometry helper (also called by mood.inl::apply_mood     │
// │  for the mood-5 forced spawn path):                              │
// │    fill_ribbon_selection_geometry(seed, tier, sel)               │
// │      — pure; no ribbon state needed                              │
// │                                                                  │
// │  Cross-module reads (consumed by spine, mood.inl, render):       │
// │    ribbon_state_.active[], ribbon_state_.gpu[]   — read by spine │
// │    ribbon_state_.active_count                    — read by spine │
// │    ribbon_state_.rendered_slot                   — read by spine │
// │    ribbon_state_.mood_offset                     — read by mood  │
// │    MAX_RIBBON_INSTANCES, RIBBON_MAX_LENGTH                       │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// Included inside the Cartridge class body.
// Depends on: spawn_engine.inl (run_spawn_preamble, negotiate_position,
//             record_placement_bookkeeping, footprint registry,
//             evaluate_spawn_gate, jittered_position),
//             seed_utils.inl, cartridge.hpp core (time_state_.seconds,
//             pawnReadback_*, THEMES, PATCH_EXTENT, Dim::*),
//             state.hpp (GPUState::upload_ribbon_head_poses — the one
//             dumb wire the head laws write through).
//
// SEAM[ribbon:complete-subsystem] complete bespoke pipeline in one
//   module — vocabulary + state + machinery + lifecycle + head laws.
//   Same family as gol_zones (Ch. 12.B) and gallery (Ch. 12.E), and
//   the reference instance of the entity-module pattern: tuning
//   console → registry → tiers → runtime state → author seats →
//   head laws → lifecycle. (surface = the bank rows; the in-module
//   section retired with its last legacy consumer).
// SEAM[ribbon:dual-entry] commit_ribbon below has TWO callers:
//   FAMILY_DISPATCH[RIBBON].try_commit during patch streaming, AND
//   mood.inl::apply_mood for mood-5 forced spawn. The dual entry
//   point is owned by mood:K4 (mood-5 reference clone), not by
//   ribbon machinery. Tag-only awareness.
// ─────────────────────────────────────────────────────────────────


// ═══ TUNING CONSOLE ══════════════════════════════════════════════
//
// System-level dials for the ribbon subsystem — the DESIGN-TIME
// control panel (change and rebuild). Per-tier values (the Gaussian
// means/sigmas that shape each tier's feel) live in RIBBON_TIERS
// below. Everything here applies across all tiers.

// ── Spawn ────────────────────────────────────────────────────────
struct RibbonConfig {
    static constexpr float SPAWN_CHANCE = 0.900f;   // TESTING: was 0.400f -- bumped for ribbon-dev visibility; revert before ship (control-panel constant)
    // SEAM[ribbon:P4] hygiene rows pattern — { open, sunset,
    //   [indoor_flat=0], [indoor_vault=0], [finite_outdoor=1],
    //   [finite_outdoor_ref=0] }. Same family as gol_zones:P4
    //   and floaters:P4.
    static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
    static constexpr float POSITION_JITTER = 0.3f;
};

// ── Length cap ───────────────────────────────────────────────────
// Total ribbon length (cube_count × cube_size) is capped here to
// keep anchor coverage viable (700 u = 14 patches). RIBBON_MAX_LENGTH
// itself is defined below in this file (Capacity section); consumed
// in fill_ribbon_selection_geometry.

// ── Geometry / placement ─────────────────────────────────────────
// Floors on the Gaussian-sampled shape draws plus the fixed spawn
// footprint and orientation spread. Consumed in
// fill_ribbon_selection_geometry / select_ribbon_for_patch.
static constexpr float MIN_CUBE_COUNT     = 20.0f;    // floor on Gaussian-sampled cube_count
static constexpr float MIN_CUBE_SIZE      = 1.0f;     // floor on cube_size
static constexpr float MIN_ADDED_HEIGHT   = 20.0f;    // floor on the clearance draw
static constexpr float FOOTPRINT_RADIUS   = 5.0f;     // ribbon spawn footprint radius
static constexpr float ORIENTATION_SPREAD = 1.0472f;  // ±60° (π/3) around away-from-pawn

// ── Head control law ─────────────────────────────────────────────
// The steering integrator and altitude pen constants — one law, many
// authors. Yaw is STEERING, not free aim: available yaw rate is
// min(RIBBON_YAW_RATE, speed / RIBBON_R_MIN), so the heading can only
// change while moving and the flown path can never be tighter than
// the minimum turn radius. Consumed in ribbon_advance_head. All
// control-panel material.
static constexpr float RIBBON_YAW_RATE       = 1.0f;    // rad/s cap at full deflection
static constexpr float RIBBON_MAX_SPEED      = 40.0f;   // world units/s at full throttle (halved; full-throttle turns bottom out at R_MIN)
static constexpr float RIBBON_R_MIN          = 40.0f;   // minimum turn radius (units)
static constexpr float RIBBON_CLIMB_RATE     = 15.0f;   // u/s cap on the pen's vertical velocity
static constexpr float RIBBON_FLOOR_MARGIN   = 25.0f;   // guaranteed gap over tall ground
static constexpr float RIBBON_ALT_SMOOTH_DIST = 180.0f; // units of travel over which the altitude target relaxes — the head reads the LANDSCAPE, not the terrain texture
static constexpr float RIBBON_ALT_STIFF      = 0.36f;   // (rad/s)^2 — the pen's stiffness; damping = 2*sqrt(stiffness), critically damped
static constexpr float RIBBON_MOUNT_SETBACK  = 1.5f;    // pawn seat setback toward the tail (+heading) so the body sits over the tube, not the leading cap
static constexpr float RIBBON_SKY_YAW_TAU    = 0.6f;    // s; first-order ease on the PLAYER's yaw hand — the body replays the heading history, so bang-bang arrows must become curves; short tau keeps it immediate
static constexpr float RIBBON_REFERENCE_BPM  = 100.0f;  // the tempo at which the tiers' authored sway is DEFINED; phase advances at live-tempo/this (control-panel)

// ── Wander policy ─────────────────────────────────────────────────
// The steering channel's IDLE SCRIPT — the shape of autonomous drift.
// Constants: control-panel material.
static constexpr float WANDER_CHANCE      = 0.30f;   // per-spawn roll
static constexpr float WANDER_CRUISE_BASE  = 0.35f;   // gaussian mean (fraction of RIBBON_MAX_SPEED)
static constexpr float WANDER_CRUISE_SIGMA = 0.15f;   // gaussian sigma
static constexpr float WANDER_CRUISE_MIN  = 0.15f;
static constexpr float WANDER_CRUISE_MAX  = 0.80f;
static constexpr float WANDER_LEG_MIN     = 200.0f;  // waypoint leg length (units)
static constexpr float WANDER_LEG_MAX     = 500.0f;
static constexpr float WANDER_SPREAD      = 1.0f;    // rad of bearing spread around current motion
static constexpr float WANDER_RETARGET_MIN = 10.0f;  // seconds between waypoints
static constexpr float WANDER_RETARGET_VAR = 15.0f;
static constexpr float WANDER_STEER_SOFT  = 0.5f;    // rad of heading error for full deflection
static constexpr float WANDER_YAW_MAX     = 0.15f;   // yaw cap: radius >= RIBBON_R_MIN/0.15 (~270 u) — body-scale arcs
static constexpr float WANDER_YAW_TAU     = 2.0f;    // s; first-order ease on the steering — curvature stays continuous (control-panel)
static constexpr float WANDER_ARRIVE_RADIUS = 120.0f; // u; arrival = retarget — inside this the bearing chase degenerates (control-panel)


// ═══ COLOR VOCABULARY ════════════════════════════════════════════

struct RibbonColorMode {
    static constexpr uint32_t SMOOTH = 0;  // terrain-derived monochrome
    static constexpr uint32_t TINTED = 1;  // warm/cool hue shift
    static constexpr uint32_t CONTRAST = 2;  // cell skin: per-cell coloring — pair-contrast species (two medians, parity) or median-field species (one median, terrain-patch texture); see fill
    static constexpr uint32_t COUNT = 3;
    static constexpr float WEIGHTS[COUNT] = { 0.40f, 0.35f, 0.25f };
};

// Smooth color palettes: base colors for SMOOTH mode ribbons
static constexpr float RIBBON_SMOOTH_PALETTE[][3] = {
    { 0.82f, 0.75f, 0.62f },   // warm sandstone
    { 0.55f, 0.65f, 0.78f },   // sky blue
    { 0.85f, 0.78f, 0.58f },   // golden
    { 0.50f, 0.68f, 0.55f },   // sage green
};
static constexpr uint32_t RIBBON_SMOOTH_PALETTE_COUNT = 4;

// ── Color character ──────────────────────────────────────────────
// Per-mode dials for the color draws in fill_ribbon_selection_geometry.
// Structure (which hash prop feeds which channel, the (1 - hue) on
// CONTRAST green) stays in code; only the magnitudes live here.

// SMOOTH: per-channel variance around the palette base.
//   var = hash * RANGE + BIAS; applied as { +var, +var*G, +var*B }.
static constexpr float SMOOTH_VAR_RANGE = 0.10f;
static constexpr float SMOOTH_VAR_BIAS  = -0.05f;
static constexpr float SMOOTH_VAR_G_SCALE = 0.8f; // green channel gets var * G_SCALE
static constexpr float SMOOTH_VAR_B_SCALE = 0.6f; // blue  channel gets var * B_SCALE

// TINTED: per-channel hash*range + base (R & B range 0.45, G range 0.40).
static constexpr float TINTED_RANGE[3] = { 0.45f, 0.40f, 0.45f };
static constexpr float TINTED_BASE[3]  = { 0.40f, 0.35f, 0.35f };

// CONTRAST — the checker pair raffle, shaped like the terrain's §2.2
// palette system. Each pair authors its colors AND its character:
//   value_var — per-cell lightness texture: fraction of headroom each
//               cell travels toward black/white (clip-free). Legibility
//               guidance: beyond ~0.15 the dark/light parity starts to
//               blur; author past it only on purpose.
//   hue_var   — 0..1, the colorful axis: scales the per-cell hue
//               rotation and chroma injection (world.wgsl skin block).
//               0 = strict chessboard; 1 = full-wheel mosaic on BOTH
//               squares (light cells go pastel at their own lightness).
//   weight    — cumulative-roll probability; rare pairs are events.
// Weights sum to 1. All control-panel; author freely.
struct CheckerPair {
    float dark[3];
    float light[3];
    float value_var;
    float hue_var;
    float weight;
};
static constexpr CheckerPair CHECKER_PAIRS[] = {
    { {0.16f,0.15f,0.17f}, {0.88f,0.86f,0.82f}, 0.05f, 0.05f, 0.30f },  // obsidian / bone   — strict
    { {0.30f,0.12f,0.18f}, {0.92f,0.78f,0.80f}, 0.06f, 0.20f, 0.20f },  // wine / rose       — calm
    { {0.14f,0.16f,0.34f}, {0.87f,0.76f,0.58f}, 0.06f, 0.30f, 0.20f },  // indigo / sand     — lively
    { {0.10f,0.24f,0.16f}, {0.78f,0.90f,0.80f}, 0.07f, 0.55f, 0.15f },  // forest / mint     — wild
    { {0.38f,0.18f,0.10f}, {0.90f,0.85f,0.74f}, 0.06f, 0.35f, 0.10f },  // rust / cream      — lively
    { {0.13f,0.13f,0.13f}, {0.92f,0.80f,0.45f}, 0.05f, 0.75f, 0.05f },  // charcoal / gold   — rare riot
};
static constexpr uint32_t CHECKER_PAIR_COUNT =
    sizeof(CHECKER_PAIRS) / sizeof(CHECKER_PAIRS[0]);
static constexpr float CHECKER_PAIR_JITTER = 0.03f;  // shared per-ribbon median offset
static constexpr float CHECKER_HUE_SIBLING_JITTER = 0.10f;  // per-ribbon ± around the pair's hue_var

// FREE RAFFLE — the terrain's discrete-region grammar for the skin: both
// medians raffled as points in (luma, chroma, hue) space, both variances
// raffled. The bounds below ARE the lattice; narrow them to tame, widen to
// liberate. The one kept law: disjoint luma bands preserve the dark/light
// parity under any hue — the chessboard survives its own liberation.
// All control-panel.
static constexpr float FREE_PAIR_CHANCE   = 0.50f;  // vs the authored pair table
static constexpr float FREE_DARK_LUMA[2]  = { 0.10f, 0.35f };
static constexpr float FREE_DARK_CHROMA[2]= { 0.05f, 0.30f };
static constexpr float FREE_LIGHT_LUMA[2] = { 0.70f, 0.95f };
static constexpr float FREE_LIGHT_CHROMA[2]={ 0.02f, 0.22f };
static constexpr float FREE_VALUE_VAR[2]  = { 0.02f, 0.30f };  // raffled, generous ceiling
static constexpr float FREE_HUE_VAR[2]    = { 0.00f, 1.00f };  // raffled, UNCAPPED (full axis)
// Rodrigues basis about the gray axis (unit chroma + its quadrature) —
// the CPU twin of the shader's hue machinery.
static constexpr float CHROMA_D1[3] = { 0.8165f, -0.4082f, -0.4082f };
static constexpr float CHROMA_D2[3] = { 0.0f,     0.7071f, -0.7071f };

// MEDIAN-FIELD species — some cell-skinned ribbons are not defined by
// contrast at all: ONE median, cells as variations around it — the
// terrain-patch grammar on a tube (color_b == color; the parity term
// vanishes by algebra). Luma band is broad (no parity to protect);
// VALUE_VAR floor sits higher so the cells read through texture, as
// terrain cells do. All control-panel.
static constexpr float CELLS_MEDIAN_CHANCE   = 0.35f;  // species roll, above the pair fork
static constexpr float MEDIAN_LUMA[2]        = { 0.25f, 0.85f };
static constexpr float MEDIAN_CHROMA[2]      = { 0.04f, 0.30f };
static constexpr float MEDIAN_VALUE_VAR[2]   = { 0.06f, 0.35f };
static constexpr float MEDIAN_HUE_VAR[2]     = { 0.00f, 1.00f };


// ═══ PROPERTY INDEX REGISTRY ═════════════════════════════════════
//
// DONE[ribbon:L1] Stride convention (intentional, do not compact):
//   400      SPAWN_ROLL
//   401-409  per-instance scalar rolls (ANCHOR_X..PALETTE_IDX)
//   410-419  cube-count / size / height       (10-row reserve)
//   420-429  lateral wave  (amp, cycles, speed; rest reserved)
//   430-439  vertical wave (amp; rest reserved)
//   440-449  checker skin  (pair roll, median jitter, hue sibling-jitter; rest reserved)
//   450-459  wander        (roll, cruise, rng seed; rest reserved)
//   460-469  checker free raffle (mode, dark l/c/h, light l/c/h, vars; rest reserved)
//   470-479  checker median-field (species roll, luma, chroma, hue, vars; rest reserved)
//   The per-axis stride of 10 leaves room for future per-axis
//   params without renumbering downstream. Same self-documentation
//   discipline used by the WGSL side.

struct RibbonProp {
    static constexpr uint32_t SPAWN_ROLL = 400u;
    static constexpr uint32_t ANCHOR_X = 401u;
    static constexpr uint32_t ANCHOR_Z = 402u;
    static constexpr uint32_t TIER = 403u;
    static constexpr uint32_t COLOR_ROLL = 404u;
    static constexpr uint32_t ORIENTATION = 405u;
    static constexpr uint32_t COLOR_R = 406u;
    static constexpr uint32_t COLOR_G = 407u;
    static constexpr uint32_t COLOR_B = 408u;
    static constexpr uint32_t PALETTE_IDX = 409u;
    // Gaussian draw indices
    static constexpr uint32_t CUBE_COUNT = 410u;
    static constexpr uint32_t CUBE_SIZE = 411u;
    static constexpr uint32_t HEIGHT = 412u;
    static constexpr uint32_t LATERAL_AMP = 420u;
    static constexpr uint32_t LATERAL_CYCLES = 421u;
    static constexpr uint32_t VERTICAL_AMP = 430u;
    static constexpr uint32_t CHECKER_PAIR_ROLL = 440u;   // pair raffle
    static constexpr uint32_t CHECKER_JIT_R     = 441u;   // shared median jitter
    static constexpr uint32_t CHECKER_JIT_G     = 442u;
    static constexpr uint32_t CHECKER_JIT_B     = 443u;
    static constexpr uint32_t CHECKER_HUE_JITTER_ROLL = 444u;  // sibling ± around the pair's hue_var
    static constexpr uint32_t FREE_MODE_ROLL   = 460u;  // free vs authored table
    static constexpr uint32_t FREE_DARK_L      = 461u;
    static constexpr uint32_t FREE_DARK_C      = 462u;
    static constexpr uint32_t FREE_DARK_H      = 463u;
    static constexpr uint32_t FREE_LIGHT_L     = 464u;
    static constexpr uint32_t FREE_LIGHT_C     = 465u;
    static constexpr uint32_t FREE_LIGHT_H     = 466u;
    static constexpr uint32_t FREE_VALUE_ROLL  = 467u;
    static constexpr uint32_t FREE_HUE_ROLL    = 468u;
    static constexpr uint32_t MEDIAN_SPECIES_ROLL = 470u;
    static constexpr uint32_t MEDIAN_L            = 471u;
    static constexpr uint32_t MEDIAN_C            = 472u;
    static constexpr uint32_t MEDIAN_H            = 473u;
    static constexpr uint32_t MEDIAN_VALUE_ROLL   = 474u;
    static constexpr uint32_t MEDIAN_HUE_ROLL     = 475u;
    static constexpr uint32_t WANDER_ROLL = 450u;       // wander yes/no
    static constexpr uint32_t WANDER_CRUISE = 451u;     // gaussian draw: cruise fraction of RIBBON_MAX_SPEED
    static constexpr uint32_t WANDER_RNG = 452u;        // seeds the runtime waypoint stream
};


// ═══ TIER PROFILE + MATRIX ═══════════════════════════════════════
//
// Three tiers — Serpentine, Helix, Streamer — each with mean+sigma
// for every wave/geometry parameter. Pattern matches GoLTierProfile.

static constexpr uint32_t RIBBON_TIER_COUNT = 3;
static constexpr float RIBBON_BASE_TIER_WEIGHTS[RIBBON_TIER_COUNT] = {
    0.45f, 0.30f, 0.25f
};

struct RibbonTierProfile {
    // ─── Geometry ────────────────────────────────────────────
    float cube_count_mean, cube_count_sigma;
    float cube_size_mean, cube_size_sigma;

    // Cube count ~50–115 across tiers; cube size ~3.5–10 (Serpentine chunky).

    // ─── Altitude ────────────────────────────────────────────
    float height_mean, height_sigma;

    // Flying height: 50–80 units. All wave params independently seeded.

    // Trail-frame: tiers author lateral_cycles (aesthetic visible cycles)
    // + amps. Temporal rate is derived at commit: freq = cycles * 2pi *
    // propagation_speed / total_length; vertical shares the lateral cycles
    // (one visible wavelength, two amplitudes).

    // ─── Lateral wave ─────────────────────────────────────
    float lateral_amp_mean, lateral_amp_sigma;
    float lateral_cycles_mean, lateral_cycles_sigma;

    // ─── Vertical wave ────────────────────────────────────
    float vertical_amp_mean, vertical_amp_sigma;

    // ─── Propagation (trail-frame head→tail rate, world units/s) ──
    float propagation_speed;

    // ─── Selection ───────────────────────────────────────────
    float weight;
};

//                          ┌── Serpentine ──┬──── Helix ─────┬─── Streamer ───┐
//                          │   μ       σ    │   μ       σ    │   μ       σ    │
// ─── Geometry ────────────┤                │                │                │
//   cube_count             │  70       7    │  85      12    │  90       9    │
//   cube_size              │   8.0     1.0  │   5.0     0.8  │   6.0     0.8  │
// ─── Altitude ────────────┤                │                │                │
//   height                 │  60      15    │  55      12    │  70      20    │
// ─── Lateral wave ────────┤                │                │                │
//   lateral_amp            │  10.0     0.6  │   3.5     0.6  │   5.5     0.8  │
//   lateral_cycles         │   1.2     0.3  │   1.8     0.5  │   1.5     0.4  │
// ─── Vertical wave ───────┤  cycles = lateral (one wavelength, two amps)     │
//   vertical_amp           │   5.0     0.8  │   2.5     0.5  │   8.0     1.2  │
// ─── Trail-frame ─────────┤                │                │                │
//   propagation_speed      │  40.0          │  24.0          │  48.0          │
// ─── Selection ───────────┤                │                │                │
//   weight                 │   0.45         │   0.30         │   0.25         │
//                          └────────────────┴────────────────┴────────────────┘
//
// LENGTH LAW — the tier means live UNDER the cap; they do not fight it.
//   μ_len = count·size ≈ 0.8 × RIBBON_MAX_LENGTH (560 / 425 / 540 u), with
//   +2σ_len ≈ the cap, so the Gaussian breathes and capping is a rare tail
//   event rather than the norm. lateral_cycles were rescaled by μ_len/700
//   from the capped-era values, preserving each tier's temporal sway rate
//   (freq = cycles·2π·P/L): 0.54 / 0.64 / 0.84 rad/s at the means.
static constexpr RibbonTierProfile RIBBON_TIERS[RIBBON_TIER_COUNT] = {
    // Tier 0: Serpentine — long, massive, slow motion
    {    70.0f,  7.0f,      // cube_count
          8.0f,  1.0f,      // cube_size
         60.0f, 15.0f,      // height
         10.0f,  0.6f,      // lateral_amp
          1.2f,  0.3f,      // lateral_cycles
          5.0f,  0.8f,      // vertical_amp
         40.0f,             // propagation_speed (u/s)
          0.45f },          // weight
    // Tier 1: Helix — tight sway, small cubes, slowest propagation
    {    85.0f, 12.0f,      // cube_count
          5.0f,  0.8f,      // cube_size
         55.0f, 12.0f,      // height
          3.5f,  0.6f,      // lateral_amp
          1.8f,  0.5f,      // lateral_cycles
          2.5f,  0.5f,      // vertical_amp
         24.0f,             // propagation_speed (u/s)
          0.30f },          // weight
    // Tier 2: Streamer — tall vertical form, deep breathing
    {    90.0f,  9.0f,      // cube_count
          6.0f,  0.8f,      // cube_size
         70.0f, 20.0f,      // height
          5.5f,  0.8f,      // lateral_amp
          1.5f,  0.4f,      // lateral_cycles
          8.0f,  1.2f,      // vertical_amp
         48.0f,             // propagation_speed (u/s)
          0.25f },          // weight
};

static constexpr const char* RIBBON_TIER_NAMES[] = {
    "Serpentine", "Helix", "Streamer"
};
static constexpr const char* RIBBON_COLOR_NAMES[] = {
    "smooth", "tinted", "contrast"
};


// ═══ RUNTIME STATE ═══════════════════════════════════════════════
//
// Per-instance ribbon state, the GPU-state mirrors, and the head —
// the one live instrument. Single-render today
// (MAX_RIBBON_INSTANCES = 1), structured to scale when the GPU
// supports multi-ribbon.

// ── Capacity ─────────────────────────────────────────────────────
static constexpr uint32_t MAX_RIBBON_INSTANCES = 1;  // single-render; raise when GPU supports multi-ribbon
static constexpr float    RIBBON_MAX_LENGTH = 700.0f;

// ── Per-instance tracking ────────────────────────────────────────
// Two-tip anchoring: ribbon survives until BOTH its tip patches are
// out of view (cross-patch eviction tracking).
struct ActiveRibbon {
    int32_t patch_gx = 0, patch_gz = 0;   // trigger patch
    int32_t host_gx = 0, host_gz = 0;     // host patch (anchor position)
    float anchor_x = 0.0f, anchor_z = 0.0f;
    int32_t near_tip_gx = 0, near_tip_gz = 0;
    int32_t far_tip_gx = 0, far_tip_gz = 0;
    bool near_tip_registered = false;
    bool far_tip_registered = false;
    uint32_t ref_count = 0;     // patches referencing this ribbon via record_entity
    bool active = false;
    float spawn_color[3] = { 0.0f, 0.0f, 0.0f };   // idle base for the line-tint coupling (gen-2): gpu.color = lerp(spawn, stim, mix)
    float spawn_lateral_amp = 0.0f;   // seed-drawn wave amps — the amp pipes'
    float spawn_vertical_amp = 0.0f;  //   idle bases (gpu = base × pipe mult)
    float phase = 0.0f;   // sway phase clock — integrates at the tempo follower's rate (100 BPM ⇒ wall seconds exactly)

    // ── Wander (autonomous drift) ── rolled at commit; a wanderer authors the
    // same yaw/throttle inputs the player does, through the same steering
    // integrator. SEAM[ribbon:sky-mode].
    bool     wander = false;
    float    wander_cruise = 0.0f;      // throttle fraction of RIBBON_MAX_SPEED
    float    wander_tx = 0.0f;          // current waypoint (world XZ)
    float    wander_tz = 0.0f;
    float    wander_retarget = 0.0f;    // seconds until a new waypoint
    uint32_t wander_rng = 1u;           // self-contained xorshift state
    float    wander_yaw_state = 0.0f;   // eased steering output (curvature continuity)
};

// ── The head ──────────────────────────────────────────────────────
// The live instrument: integrated position, heading, the altitude
// pen, and the propagation history the body is rebuilt from. ONE head
// exists (the rendered ribbon wears it); identity is (seeded, slot),
// and since slots are reused, REBIRTH is signalled by
// ribbon_invalidate_head — commit sends it (see the succession note
// there).
struct RibbonHead {
    bool     seeded = false;
    uint32_t slot = UINT32_MAX;
    float    origin[3] = { 0.0f, 0.0f, 0.0f };
    float    alt_target = 0.0f;   // low-passed altitude target (landscape swells, not texture)
    float    y_vel = 0.0f;        // the pen's vertical velocity (critically damped follower)
    bool     alt_baked = false;   // birthright latched? (re-bakes until the ground sample is warm)
    float    heading = 0.0f;               // sky-flight heading (yawed by input)
    float    pos[3] = { 0.0f, 0.0f, 0.0f };  // live integrated head position
    float    mount[3] = { 0.0f, 0.0f, 0.0f }; // visible head-ring center + half-tube (pawn mount point)

    // ── Propagation history ── the body is the head's past, replayed at
    // propagation speed: ring k wears the head's state from
    // age = k·spacing/P seconds ago. Two channels suffice (heading, y);
    // XZ is reconstructed by integrating the delayed heading tailward.
    static constexpr float    HIST_DT  = 0.05f;  // sample cadence (s)
    static constexpr uint32_t HIST_CAP = 1024u;  // ~51 s of past > max body age (~29 s = RIBBON_MAX_LENGTH / slowest tier P — grow this if the cap grows)
    std::array<float, HIST_CAP> hist_heading{};
    std::array<float, HIST_CAP> hist_y{};
    uint32_t hist_head = 0;    // ring buffer: newest sample index
    float    hist_time = 0.0f; // time of the newest sample
};

// ── Ribbon module state (Scope B migration #1) ────────────────────
// All ribbon-owned state lives in this struct, accessed via
// ribbon_state_ on the Cartridge. Module functions take
// `RibbonState& rs` explicitly rather than reaching via Cartridge*,
// making ownership language-visible and dependencies explicit in
// signatures.
struct RibbonState {
    ActiveRibbon   active[MAX_RIBBON_INSTANCES]{};
    uint32_t       active_count = 0;

    // GPU-state CPU mirror. Per-frame, the spine picks the nearest
    // ribbon to the pawn and uploads its slot to the GPU.
    // rendered_slot is the currently rendered slot (UINT32_MAX = none).
    GPURibbonState gpu[MAX_RIBBON_INSTANCES]{};
    uint32_t       rendered_slot = UINT32_MAX;

    // The head mover — see RibbonHead above.
    RibbonHead     head{};

    // Mood-5 ribbon anchor offset. Seed-derived position centered on
    // the finite world. Adjust to manually shift the mood-5 anchor XZ
    // (read by mood.inl::apply_mood).
    float          mood_offset[2] = { 0.0f, 0.0f };
};
RibbonState ribbon_state_;


// ═══ AUTHOR SEATS ════════════════════════════════════════════════
//
// The steering integrator has three seats, all writing the same two
// inputs (yaw_in, throttle_in): the PLAYER (cartridge frame block,
// sky mode), the WANDERER below (the idle script), and one EMPTY
// SEAT reserved for the musical canvas. One control law, many
// authors.

static inline float wander_rand01(uint32_t& s) {
    // xorshift32 → [0,1)
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    return (float)(s & 0x00FFFFFFu) / 16777216.0f;
}

// Author this frame's flight inputs for a wandering ribbon. Movement is along
// -heading (the flight convention), so the desired heading is the bearing to
// the waypoint plus pi. The steering integrator downstream enforces
// RIBBON_R_MIN, so any waypoint yields a legal, frame-safe path.
static void ribbon_wander_inputs(ActiveRibbon& ar,
                                 float head_x, float head_z, float heading,
                                 float dt, float& yaw_in, float& thr_in)
{
    // Free roam: waypoints are picked AHEAD of the current motion — a bearing
    // spread around where the ribbon is already going, a leg of 200-500 units.
    // No leash: a rendered wanderer is pinned against eviction instead, and
    // the yaw cap below keeps every turn at body scale (radius >= RIBBON_R_MIN /
    // WANDER_YAW_MAX). Gorgeous, contemplative arcs by construction.
    ar.wander_retarget -= dt;
    const float wdx = ar.wander_tx - head_x;
    const float wdz = ar.wander_tz - head_z;
    if (ar.wander_retarget <= 0.0f
        || wdx * wdx + wdz * wdz < WANDER_ARRIVE_RADIUS * WANDER_ARRIVE_RADIUS) {
        const float move_dir = heading + 3.14159265f;           // movement = -heading
        const float spread = (wander_rand01(ar.wander_rng) * 2.0f - 1.0f) * WANDER_SPREAD;
        const float leg = WANDER_LEG_MIN
                        + (WANDER_LEG_MAX - WANDER_LEG_MIN) * wander_rand01(ar.wander_rng);
        const float b = move_dir + spread;
        ar.wander_tx = head_x + leg * std::cos(b);
        ar.wander_tz = head_z + leg * std::sin(b);
        ar.wander_retarget = WANDER_RETARGET_MIN
                           + WANDER_RETARGET_VAR * wander_rand01(ar.wander_rng);
    }

    const float bearing = std::atan2(ar.wander_tz - head_z, ar.wander_tx - head_x);
    const float desired = bearing + 3.14159265f;                // movement = -heading
    const float err = std::remainder(desired - heading, 6.2831853f);
    float cmd = err / WANDER_STEER_SOFT;
    cmd = (cmd >  WANDER_YAW_MAX) ?  WANDER_YAW_MAX :
          (cmd < -WANDER_YAW_MAX) ? -WANDER_YAW_MAX : cmd;
    // Ease the steering: the body replays the heading history, and bang-bang
    // commands print elbows. First-order toward the command keeps curvature
    // continuous — turns enter and exit as curves, never as joints.
    const float ease = 1.0f - std::exp(-dt / WANDER_YAW_TAU);
    ar.wander_yaw_state += (cmd - ar.wander_yaw_state) * ease;
    yaw_in = ar.wander_yaw_state;
    thr_in = ar.wander_cruise;
}


// ═══ HEAD LAWS ═══════════════════════════════════════════════════
//
// The head mover: the steering integrator, the altitude pen, the
// propagation history, and THE LAW that rebuilds the body from it.
// CPU authors intent; the GPU realizes geometry — these laws write
// the GPU exactly once per frame, through
// GPUState::upload_ribbon_head_poses (a dumb wire).

// Sample the propagation history `age` seconds into the past
// (0 = now). Heading is an integrator's output — continuous — so
// plain lerp is safe; y likewise.
static void ribbon_history_sample(const RibbonState& rs, float age, float& h, float& y) {
    const RibbonHead& hd = rs.head;
    float fidx = age / RibbonHead::HIST_DT;
    if (fidx < 0.0f) fidx = 0.0f;
    const float fmax = static_cast<float>(RibbonHead::HIST_CAP - 2u);
    if (fidx > fmax) fidx = fmax;
    const uint32_t j0 = static_cast<uint32_t>(fidx);
    const float frac = fidx - static_cast<float>(j0);
    const uint32_t i0 = (hd.hist_head + RibbonHead::HIST_CAP - j0) % RibbonHead::HIST_CAP;
    const uint32_t i1 = (hd.hist_head + RibbonHead::HIST_CAP - (j0 + 1u)) % RibbonHead::HIST_CAP;
    h = hd.hist_heading[i0] + (hd.hist_heading[i1] - hd.hist_heading[i0]) * frac;
    y = hd.hist_y[i0]       + (hd.hist_y[i1]       - hd.hist_y[i0])       * frac;
}

// THE LAW: the body is the head's past, replayed at propagation
// speed. Ring k wears the head's state from age = k·spacing/P
// seconds ago — its delayed heading orients the segment and fills
// the yaw channel (continuous by construction); its delayed y is
// the altitude. XZ integrates the delayed heading TAILWARD
// (+heading) from the live head. A turn made now travels down the
// body at P through space; so does an altitude swell; so will
// every musical gesture at the head. Replaces the spatial trail:
// a bend is motion, not a mark on the floor.
static void ribbon_rebuild_body_upload(RibbonState& rs, GPUState& gpuState,
                                       wgpu::Queue& queue, const GPURibbonState& ribbon,
                                       float head_x, float head_y, float head_z) {
    const uint32_t n = std::min(ribbon.cube_count, Dim::RIBBON_MAX_RINGS);
    if (n < 2u) return;
    const float spacing = ribbon.cube_size;
    const float inv_p = 1.0f / std::max(ribbon.propagation_speed, 0.001f);

    std::array<float, 4 * Dim::RIBBON_MAX_RINGS> poses{};
    poses[0] = head_x;
    poses[1] = head_y;
    poses[2] = head_z;
    poses[3] = rs.head.heading;   // ring 0: the live head, live heading

    float px = head_x, pz = head_z;
    for (uint32_t k = 1u; k < n; ++k) {
        const float age = (static_cast<float>(k) * spacing) * inv_p;
        float h, y;
        ribbon_history_sample(rs, age, h, y);
        px += spacing * std::cos(h);   // tailward = +heading
        pz += spacing * std::sin(h);
        poses[4u * k + 0u] = px;
        poses[4u * k + 1u] = y;
        poses[4u * k + 2u] = pz;
        poses[4u * k + 3u] = h;        // yaw channel: the delayed heading
    }
    gpuState.upload_ribbon_head_poses(queue, poses.data(),
                                      poses.size() * sizeof(float));
}

// Advance the head one frame — flown (player, wanderer, or a
// future musical author, all through the same integrator) or
// parked (held at the anchor) — record heading + Y into the
// propagation history, rebuild the body. head_poses is ALWAYS
// the rendered body: a parked head has a constant past, which
// reads as the straight spawn arc.
static void ribbon_advance_head(RibbonState& rs, GPUState& gpuState,
                                wgpu::Queue& queue, const GPURibbonState& ribbon,
                                uint32_t slot, float t,
                                bool flown, float yaw_in, float throttle_in, float dt,
                                float ground_y, bool ground_valid) {
    RibbonHead& hd = rs.head;

    if (!hd.seeded || hd.slot != slot) {
        hd.origin[0] = ribbon.anchor[0];
        hd.origin[2] = ribbon.anchor[2];
        hd.heading = ribbon.orientation;
        hd.pos[0] = hd.origin[0];
        hd.pos[2] = hd.origin[2];
        // A newborn body is a constant past: straight along the spawn
        // heading — the seed, restated as time. (The Y channel fills in
        // the bake below.)
        hd.hist_heading.fill(hd.heading);
        hd.hist_head = 0;
        hd.hist_time = t;
        hd.alt_baked = false;   // altitude bakes below, latching on the first warm ground sample
        hd.seeded = true;
        hd.slot = slot;
    }

    // THE BAKE — altitude is a birthright: the ground of the birthplace
    // plus the seed-drawn clearance (ribbon.height), baked ONCE at first
    // truth, never chased. After a mood flip the estimator returns 0 for
    // a still-cold tile — indistinguishable from flat ground by value —
    // so the bake re-runs each frame until the call site reports a WARM
    // sample, then latches. Parked ribbons hold this number forever.
    // SEAM[ribbon:sky-mode].
    if (!hd.alt_baked) {
        hd.origin[1]  = ground_y + ribbon.height;   // first truth, once
        hd.pos[1]     = hd.origin[1];
        hd.alt_target = hd.origin[1];
        hd.y_vel      = 0.0f;
        hd.hist_y.fill(hd.pos[1]);   // the body's constant past re-bases with the bake
        hd.alt_baked  = ground_valid;
    }

    float head_x, head_y, head_z;
    if (flown) {
        // Planar flight: yaw the heading, throttle the head along it at
        // fixed sky altitude. The head moves along -heading so the
        // straight seed (laid +heading from the anchor) trails behind it.
        // Pitch/altitude is deferred (the frame is horizontal-only by
        // construction). Constants live in the tuning console (head
        // control law). SEAM[ribbon:sky-mode].
        // Steering model: yaw is STEERING, not free aim. The available
        // yaw rate is min(RIBBON_YAW_RATE, speed / RIBBON_R_MIN): the
        // heading can only change while moving, and the flown path can
        // never be tighter than the minimum turn radius. So the velocity
        // is always the face's outward normal (trajectory orthogonal to
        // the face, by construction), the face can never turn inward
        // against the body, and heading-vs-path divergence stays at a
        // few degrees. Reverse is forbidden (a snake does not burrow
        // into its own body): down-arrow is no thrust.
        // SEAM[ribbon:sky-mode].
        const float speed = std::max(throttle_in, 0.0f) * RIBBON_MAX_SPEED;
        const float yaw_avail = std::min(RIBBON_YAW_RATE, speed / RIBBON_R_MIN);
        hd.heading += yaw_in * yaw_avail * dt;
        const float ch = std::cos(hd.heading);
        const float sh = std::sin(hd.heading);
        const float step = speed * dt;
        hd.pos[0] -= ch * step;
        hd.pos[2] -= sh * step;
        // Sky altitude with a terrain FLOOR (not a tether): the target is
        // the ribbon's baked birthright altitude, floored by the smoothed
        // ground plus a constant safety margin. Valleys and mood changes
        // leave it untouched at its sky; only ground tall enough to
        // threaten it lifts it — and it settles back beyond. The low-pass
        // keeps the floor reading the LANDSCAPE (long swells); zero travel
        // (hover) freezes the target; the critically damped PEN below
        // turns every correction into a smooth S-curve — never a
        // constant-rate ramp, never a corner.
        {
            const float floor_y = ground_y + RIBBON_FLOOR_MARGIN;
            const float raw_target = (hd.origin[1] > floor_y)
                                   ? hd.origin[1] : floor_y;
            const float travel = std::fabs(step);   // this frame's distance
            const float alpha = 1.0f - std::exp(-travel / RIBBON_ALT_SMOOTH_DIST);
            hd.alt_target += (raw_target - hd.alt_target) * alpha;

            // THE PEN: critically damped vertical follower — stiffness
            // sets the ease, damping = 2*sqrt(stiffness) means no
            // overshoot, the velocity clamp bounds extreme corrections.
            const float damp = 2.0f * std::sqrt(RIBBON_ALT_STIFF);
            hd.y_vel += ((hd.alt_target - hd.pos[1]) * RIBBON_ALT_STIFF
                         - damp * hd.y_vel) * dt;
            hd.y_vel = (hd.y_vel >  RIBBON_CLIMB_RATE) ?  RIBBON_CLIMB_RATE :
                       (hd.y_vel < -RIBBON_CLIMB_RATE) ? -RIBBON_CLIMB_RATE : hd.y_vel;
            hd.pos[1] += hd.y_vel * dt;
        }

        head_x = hd.pos[0];
        head_y = hd.pos[1];
        head_z = hd.pos[2];
    } else {
        // Not flying: head holds at the anchor at its BAKED birthright
        // altitude; heading holds the spawn orientation. The body is the
        // (constant) history — straight. Flight state stays primed.
        hd.heading = ribbon.orientation;
        hd.pos[0] = ribbon.anchor[0];
        hd.pos[1] = hd.origin[1];
        hd.pos[2] = ribbon.anchor[2];
        head_x = hd.pos[0];
        head_y = hd.pos[1];
        head_z = hd.pos[2];
    }

    // Pawn mount point (sky mode): the VISIBLE head-ring center —
    // centerline + the wave in the head ring's frame — lifted half a tube
    // so the pawn's feet sit on top. Mirrors ribbon_spine_at(0): ring 0's
    // yaw-channel value IS the live heading (head_poses[0].w), so
    // right = (-sin h, 0, cos h), up = world-up, phase_age = ribbon.time.
    // The pawn and the head ring share one frame and one wave, exactly.
    // SEAM[ribbon:sky-mode].
    {
        const float ph  = ribbon.time;
        const float lat = std::sin(ribbon.lateral_freq  * ph) * ribbon.lateral_amp;
        const float ver = std::sin(ribbon.vertical_freq * ph) * ribbon.vertical_amp;
        const float ch  = std::cos(hd.heading);
        const float sh  = std::sin(hd.heading);
        // Setback toward the tail (+heading) so the pawn's body sits
        // over the tube instead of straddling the leading cap.
        hd.mount[0] = head_x + lat * (-sh) + RIBBON_MOUNT_SETBACK * ch;
        hd.mount[1] = head_y + ver + ribbon.cube_size * 0.5f;
        hd.mount[2] = head_z + lat * ( ch) + RIBBON_MOUNT_SETBACK * sh;
    }

    // Record the head's state into the propagation history (catch-up
    // at fixed cadence; a frame hitch holds the current state across
    // the gap — the past never has holes).
    while (t - hd.hist_time >= RibbonHead::HIST_DT) {
        hd.hist_head = (hd.hist_head + 1u) % RibbonHead::HIST_CAP;
        hd.hist_heading[hd.hist_head] = hd.heading;
        hd.hist_y[hd.hist_head]       = hd.pos[1];
        hd.hist_time += RibbonHead::HIST_DT;
    }

    ribbon_rebuild_body_upload(rs, gpuState, queue, ribbon, head_x, head_y, head_z);
}

// Invalidate the head/trail state so the next advance re-inits and
// re-seeds. Called on release, eviction, and succession (commit):
// slots are reused, so the slot-change guard alone cannot detect a
// successor ribbon.
static void ribbon_invalidate_head(RibbonState& rs) { rs.head.seeded = false; }

// True when the head state belongs to this slot (post-init). The
// cartridge uses it to sample the ground at the HEAD when ready,
// and at the ANCHOR for the first frame of a life.
static bool ribbon_head_is(const RibbonState& rs, uint32_t slot) {
    return rs.head.seeded && rs.head.slot == slot;
}

// Last computed ribbon head pose — the SADDLE (mount): pen + wave +
// setback. Read by the pawn mount and camera (sky mode).
static void ribbon_head_pose(const RibbonState& rs, float& x, float& y, float& z, float& heading) {
    x = rs.head.mount[0];
    y = rs.head.mount[1];
    z = rs.head.mount[2];
    heading = rs.head.heading;
}

// The PEN — the true integrated head. Steering reads this; the
// mount above is the SADDLE (pen + wave + setback), for the pawn
// and camera only. Two consumers, two truths, never mixed.
static void ribbon_head_pen(const RibbonState& rs, float& x, float& z, float& heading) {
    x = rs.head.pos[0];
    z = rs.head.pos[2];
    heading = rs.head.heading;
}


// ═══ FRAME ORCHESTRATION ═════════════════════════════════════════
//
// The ribbon's per-frame conductor: choose the author (player in sky
// mode, wanderer when the rendered ribbon wanders, parked otherwise),
// advance the head through the laws, keep the rendered slot (hold
// until eviction, then adopt the nearest active ribbon), and flush
// the per-frame GPU uploads (time, color). update() calls this once
// per frame at the ribbon block's old position. External consumers
// of the head pose (camera signal, pawn-mount resync) read
// ribbon_head_pose from their own order-sensitive sites in update()
// — they consume, they do not orchestrate.
static void ribbon_frame_tick(RibbonState& rs, Cartridge* c, wgpu::Queue& queue) {
    // Ribbon eviction is fully event-driven via ref_count in
    // evict_patch_entities — no per-frame scan needed.

    // Phase clock on all CPU mirrors — MUSICAL TIME: the
    // sway integrates at the tempo follower's rate, scaled
    // so 100 BPM reproduces wall seconds exactly (the
    // calibration anchor). Held-last: transport stops, the
    // world keeps the pulse.
    const float phase_rate = c->time_state_.beat_rate
                           * (60.0f / RIBBON_REFERENCE_BPM);
    for (uint32_t i = 0; i < MAX_RIBBON_INSTANCES; i++) {
        auto& par = rs.active[i];
        if (!par.active) continue;
        par.phase += phase_rate * c->time_state_.dt;
        rs.gpu[i].time = par.phase;
        {
            const VisualParams& vp = c->visual_canvas_.params();
            const float ml = c->ribbon_amp_lat_dst_.valid
                ? vp.get(c->ribbon_amp_lat_dst_.base)  : 1.0f;
            const float mv = c->ribbon_amp_vert_dst_.valid
                ? vp.get(c->ribbon_amp_vert_dst_.base) : 1.0f;
            rs.gpu[i].lateral_amp  = rs.active[i].spawn_lateral_amp  * ml;
            rs.gpu[i].vertical_amp = rs.active[i].spawn_vertical_amp * mv;

            // Line tint (color gen-2): gpu.color = lerp(spawn, stim, mix).
            // Rest = mix 0 = the seed-drawn color exactly; the held-slot
            // upload_ribbon_color line ships it unchanged.
            const float mix = c->ribbon_tint_mix_dst_.valid
                ? vp.get(c->ribbon_tint_mix_dst_.base) : 0.0f;
            const float* st = c->ribbon_tint_stim_dst_.valid
                ? vp.run(c->ribbon_tint_stim_dst_.base) : nullptr;
            for (int c2 = 0; c2 < 3; ++c2) {
                const float s = st ? st[c2] : 0.0f;
                rs.gpu[i].color[c2] =
                    par.spawn_color[c2]
                    + (s - par.spawn_color[c2]) * mix;
            }
        }
    }

    // Sky mode just ended — release the pinned (now anchor-less)
    // ribbon so a fresh one can spawn. SEAM[ribbon:sky-mode].
    if (c->player_.sky_mode_prev && !c->player_.sky_mode) {
        uint32_t s = rs.rendered_slot;
        if (s != UINT32_MAX && rs.active[s].active) {
            rs.active[s] = ActiveRibbon{};
            rs.gpu[s] = GPURibbonState{};
            if (rs.active_count > 0) rs.active_count--;
            GPURibbonState empty{};
            c->gpuState_.upload_ribbon(queue, empty);
            rs.rendered_slot = UINT32_MAX;
            // Successor ribbons reuse this slot — force re-init.
            ribbon_invalidate_head(rs);
        }
    }
    c->player_.sky_mode_prev = c->player_.sky_mode;

    // Render one ribbon: hold the current slot until it's evicted,
    // then pick the nearest active ribbon as the new rendered slot.
    bool current_alive = rs.rendered_slot != UINT32_MAX
        && rs.active[rs.rendered_slot].active;

    // Flight input for the head mover: the player when sky mode is
    // on; the wander policy when the rendered ribbon is a wanderer;
    // parked otherwise. One control law, many authors.
    // SEAM[ribbon:sky-mode].
    bool  ribbon_flown  = c->player_.sky_mode;
    float ribbon_yaw_in = ribbon_flown ?  c->inputState_.move_x : 0.0f;
    float ribbon_thr_in = ribbon_flown ? -c->inputState_.move_z : 0.0f;
    // The player's pen, eased like the wanderer's (RIBBON_SKY_YAW_TAU
    // in the tuning console).
    {
        if (c->player_.sky_mode) {
            const float a = 1.0f - std::exp(-c->time_state_.dt / RIBBON_SKY_YAW_TAU);
            c->player_.sky_yaw_eased += (ribbon_yaw_in - c->player_.sky_yaw_eased) * a;
            ribbon_yaw_in = c->player_.sky_yaw_eased;
        } else {
            c->player_.sky_yaw_eased = 0.0f;
        }
    }
    if (!ribbon_flown && current_alive
        && ribbon_head_is(rs, rs.rendered_slot)
        && rs.active[rs.rendered_slot].wander) {
        float whx, whz, whh;
        ribbon_head_pen(rs, whx, whz, whh);
        ribbon_wander_inputs(
            rs.active[rs.rendered_slot],
            whx, whz, whh, c->time_state_.dt,
            ribbon_yaw_in, ribbon_thr_in);
        ribbon_flown = true;
    }

    if (current_alive) {
        // Hold — update time + color. Color is static at the spawn draw
        // since the gen-1 §5 coupling retired; the per-frame color upload
        // stays — it is the seam the gen-2 color coupling (color_stim ×
        // color_mix over spawn; see coupling_layer_migration_map.md) will
        // write through.
        c->gpuState_.upload_ribbon_time(queue,
            rs.gpu[rs.rendered_slot].time);
        c->gpuState_.upload_ribbon_color(queue,
            rs.gpu[rs.rendered_slot].color);
        c->gpuState_.upload_ribbon_wave_amps(queue,
            rs.gpu[rs.rendered_slot].lateral_amp,
            rs.gpu[rs.rendered_slot].vertical_amp);
        // The head mover must run EVERY frame for the held ribbon,
        // not only on slot eviction. Without this the trail is seeded
        // straight once and never advances, so the ribbon looks
        // stationary despite is_roaming = 1. The mover's init guard
        // (slot unchanged) makes this a pure per-frame advance — no
        // re-seed, no double work with the eviction-branch call below.
        float rib_gnd;
        bool  rib_gnd_valid;
        {
            const auto& rb = rs.gpu[rs.rendered_slot];
            float gx = rb.anchor[0], gz = rb.anchor[2];
            if (ribbon_head_is(rs, rs.rendered_slot)) {
                float hy, hh; ribbon_head_pose(rs, gx, hy, gz, hh);
            }
            rib_gnd = c->estimate_terrain_height(gx, gz);
            rib_gnd_valid = c->terrain_tile_warm(gx, gz);
        }
        ribbon_advance_head(rs, c->gpuState_, queue,
            rs.gpu[rs.rendered_slot],
            rs.rendered_slot, c->time_state_.seconds,
            ribbon_flown, ribbon_yaw_in, ribbon_thr_in, c->time_state_.dt,
            rib_gnd, rib_gnd_valid);
    }
    else {
        // Current slot is gone — find nearest active ribbon
        uint32_t nearest = UINT32_MAX;
        float nearest_d2 = FLT_MAX;
        for (uint32_t i = 0; i < MAX_RIBBON_INSTANCES; i++) {
            if (!rs.active[i].active) continue;
            float dx = rs.active[i].anchor_x - c->player_.readback_x;
            float dz = rs.active[i].anchor_z - c->player_.readback_z;
            float d2 = dx * dx + dz * dz;
            if (d2 < nearest_d2) { nearest = i; nearest_d2 = d2; }
        }

        if (nearest != UINT32_MAX) {
            c->gpuState_.upload_ribbon(queue, rs.gpu[nearest]);
            float rib_gnd;
            bool  rib_gnd_valid;
            {
                const auto& rb = rs.gpu[nearest];
                float gx = rb.anchor[0], gz = rb.anchor[2];
                if (ribbon_head_is(rs, nearest)) {
                    float hy, hh; ribbon_head_pose(rs, gx, hy, gz, hh);
                }
                rib_gnd = c->estimate_terrain_height(gx, gz);
                rib_gnd_valid = c->terrain_tile_warm(gx, gz);
            }
            ribbon_advance_head(rs, c->gpuState_, queue, rs.gpu[nearest], nearest, c->time_state_.seconds,
                ribbon_flown, ribbon_yaw_in, ribbon_thr_in, c->time_state_.dt,
                rib_gnd, rib_gnd_valid);  // 2b: head mover
            rs.rendered_slot = nearest;
        }
        else if (rs.rendered_slot != UINT32_MAX) {
            GPURibbonState empty{};
            c->gpuState_.upload_ribbon(queue, empty);
            rs.rendered_slot = UINT32_MAX;
        }
    }
}


// ═══ LIFECYCLE — three-phase + shared helper ═════════════════════

// ─── fill_ribbon_selection_geometry ───────────────────────────
// Shared geometry + color sampler used by both the dispatch
// pipeline and the mood forced-spawn path (see SEAM[ribbon:dual-entry]).
// Pure: no ribbon state access; all sampling is from `seed`.
static void fill_ribbon_selection_geometry(
    uint32_t seed, uint32_t tier_idx,
    RibbonSelection& sel)
{
    const auto& tp = RIBBON_TIERS[tier_idx];

    float count_f = std::max(MIN_CUBE_COUNT,
        cpu_sample_gaussian(seed, RibbonProp::CUBE_COUNT, tp.cube_count_mean, tp.cube_count_sigma));
    sel.cube_count = std::min((uint32_t)count_f, Dim::RIBBON_MAX_RINGS);
    sel.cube_size = std::max(MIN_CUBE_SIZE,
        cpu_sample_gaussian(seed, RibbonProp::CUBE_SIZE, tp.cube_size_mean, tp.cube_size_sigma));

    // Length cap — keeps anchor coverage viable (700 u = 14 patches)
    if ((float)sel.cube_count * sel.cube_size > RIBBON_MAX_LENGTH)
        sel.cube_count = (uint32_t)(RIBBON_MAX_LENGTH / sel.cube_size);

    // The seed draw is this ribbon's CLEARANCE above its birthplace — pure
    // from seed; the ground joins once, at head init (ribbon_advance_head).
    sel.height = std::max(MIN_ADDED_HEIGHT,
        cpu_sample_gaussian(seed, RibbonProp::HEIGHT, tp.height_mean, tp.height_sigma));

    sel.orientation = cpu_hash_f(seed, RibbonProp::ORIENTATION) * 6.2831853f;

    sel.lateral_amp = std::max(0.1f, cpu_sample_gaussian(seed, RibbonProp::LATERAL_AMP, tp.lateral_amp_mean, tp.lateral_amp_sigma));
    sel.lateral_cycles = std::max(0.1f, cpu_sample_gaussian(seed, RibbonProp::LATERAL_CYCLES, tp.lateral_cycles_mean, tp.lateral_cycles_sigma));

    sel.vertical_amp = std::max(0.1f, cpu_sample_gaussian(seed, RibbonProp::VERTICAL_AMP, tp.vertical_amp_mean, tp.vertical_amp_sigma));

    // Color
    float color_roll = cpu_hash_f(seed, RibbonProp::COLOR_ROLL);
    sel.color_mode = RibbonColorMode::COUNT - 1;
    float ccum = 0.0f;
    for (uint32_t c = 0; c < RibbonColorMode::COUNT; c++) {
        ccum += RibbonColorMode::WEIGHTS[c];
        if (color_roll < ccum) { sel.color_mode = c; break; }
    }

    if (sel.color_mode == RibbonColorMode::SMOOTH) {
        uint32_t pal_idx = (uint32_t)(cpu_hash_f(seed, RibbonProp::PALETTE_IDX) * RIBBON_SMOOTH_PALETTE_COUNT);
        if (pal_idx >= RIBBON_SMOOTH_PALETTE_COUNT) pal_idx = RIBBON_SMOOTH_PALETTE_COUNT - 1;
        float var = cpu_hash_f(seed, RibbonProp::COLOR_R) * SMOOTH_VAR_RANGE + SMOOTH_VAR_BIAS;
        sel.color[0] = RIBBON_SMOOTH_PALETTE[pal_idx][0] + var;
        sel.color[1] = RIBBON_SMOOTH_PALETTE[pal_idx][1] + var * SMOOTH_VAR_G_SCALE;
        sel.color[2] = RIBBON_SMOOTH_PALETTE[pal_idx][2] + var * SMOOTH_VAR_B_SCALE;
    }
    else if (sel.color_mode == RibbonColorMode::TINTED) {
        sel.color[0] = cpu_hash_f(seed, RibbonProp::COLOR_R) * TINTED_RANGE[0] + TINTED_BASE[0];
        sel.color[1] = cpu_hash_f(seed, RibbonProp::COLOR_G) * TINTED_RANGE[1] + TINTED_BASE[1];
        sel.color[2] = cpu_hash_f(seed, RibbonProp::COLOR_B) * TINTED_RANGE[2] + TINTED_BASE[2];
    }
    else {
        if (cpu_hash_f(seed, RibbonProp::MEDIAN_SPECIES_ROLL) < CELLS_MEDIAN_CHANCE) {
            // MEDIAN-FIELD: one raffled median; color_b == color kills the
            // parity; texture (value + hue machinery) carries the cells.
            const auto lerpf = [](const float b[2], float t) {
                return b[0] + (b[1] - b[0]) * t; };
            const float ang = cpu_hash_f(seed, RibbonProp::MEDIAN_H) * 6.2831853f;
            const float ca = std::cos(ang), sa = std::sin(ang);
            const float luma   = lerpf(MEDIAN_LUMA,   cpu_hash_f(seed, RibbonProp::MEDIAN_L));
            const float chroma = lerpf(MEDIAN_CHROMA, cpu_hash_f(seed, RibbonProp::MEDIAN_C));
            for (int i = 0; i < 3; ++i) {
                sel.color[i] = luma + (CHROMA_D1[i]*ca + CHROMA_D2[i]*sa) * chroma;
                sel.color_b[i] = sel.color[i];
            }
            sel.checker_scatter = lerpf(MEDIAN_VALUE_VAR,
                cpu_hash_f(seed, RibbonProp::MEDIAN_VALUE_ROLL));
            sel.checker_hue_spread = lerpf(MEDIAN_HUE_VAR,
                cpu_hash_f(seed, RibbonProp::MEDIAN_HUE_ROLL)) * 3.14159265f;
        } else if (cpu_hash_f(seed, RibbonProp::FREE_MODE_ROLL) < FREE_PAIR_CHANCE) {
            // FREE RAFFLE — both medians as raffled (luma, chroma, hue)
            // points; both variances raffled. lerp helper inline.
            const auto lerpf = [](const float b[2], float t) {
                return b[0] + (b[1] - b[0]) * t; };
            const auto median = [&](float luma, float chroma, float ang,
                                    float out[3]) {
                const float ca = std::cos(ang), sa = std::sin(ang);
                for (int i = 0; i < 3; ++i)
                    out[i] = luma + (CHROMA_D1[i]*ca + CHROMA_D2[i]*sa) * chroma;
            };
            median(lerpf(FREE_DARK_LUMA,  cpu_hash_f(seed, RibbonProp::FREE_DARK_L)),
                   lerpf(FREE_DARK_CHROMA,cpu_hash_f(seed, RibbonProp::FREE_DARK_C)),
                   cpu_hash_f(seed, RibbonProp::FREE_DARK_H) * 6.2831853f,
                   sel.color);
            median(lerpf(FREE_LIGHT_LUMA,  cpu_hash_f(seed, RibbonProp::FREE_LIGHT_L)),
                   lerpf(FREE_LIGHT_CHROMA,cpu_hash_f(seed, RibbonProp::FREE_LIGHT_C)),
                   cpu_hash_f(seed, RibbonProp::FREE_LIGHT_H) * 6.2831853f,
                   sel.color_b);
            sel.checker_scatter = lerpf(FREE_VALUE_VAR,
                cpu_hash_f(seed, RibbonProp::FREE_VALUE_ROLL));
            sel.checker_hue_spread = lerpf(FREE_HUE_VAR,
                cpu_hash_f(seed, RibbonProp::FREE_HUE_ROLL)) * 3.14159265f;
        } else {
            // The pair raffle — cumulative-weight pick, the terrain's roll and
            // SMOOTH's pick, one mechanism.
            const float roll = cpu_hash_f(seed, RibbonProp::CHECKER_PAIR_ROLL);
            uint32_t pick = CHECKER_PAIR_COUNT - 1;
            float ccum = 0.0f;
            for (uint32_t i = 0; i < CHECKER_PAIR_COUNT; ++i) {
                ccum += CHECKER_PAIRS[i].weight;
                if (roll < ccum) { pick = i; break; }
            }
            const CheckerPair& pr = CHECKER_PAIRS[pick];
            // One shared jitter moves both medians together: siblings differ,
            // the pair's designed contrast survives.
            const float jr = (cpu_hash_f(seed, RibbonProp::CHECKER_JIT_R) - 0.5f) * 2.0f * CHECKER_PAIR_JITTER;
            const float jg = (cpu_hash_f(seed, RibbonProp::CHECKER_JIT_G) - 0.5f) * 2.0f * CHECKER_PAIR_JITTER;
            const float jb = (cpu_hash_f(seed, RibbonProp::CHECKER_JIT_B) - 0.5f) * 2.0f * CHECKER_PAIR_JITTER;
            sel.color[0]   = pr.dark[0]  + jr;  sel.color_b[0] = pr.light[0] + jr;
            sel.color[1]   = pr.dark[1]  + jg;  sel.color_b[1] = pr.light[1] + jg;
            sel.color[2]   = pr.dark[2]  + jb;  sel.color_b[2] = pr.light[2] + jb;
            sel.checker_scatter = pr.value_var;
            {
                // hue_spread (radians, [0, pi]) = the pair's authored hue_var,
                // sibling-jittered per ribbon, scaled onto the shader's axis.
                const float sib = (cpu_hash_f(seed, RibbonProp::CHECKER_HUE_JITTER_ROLL) - 0.5f)
                                * 2.0f * CHECKER_HUE_SIBLING_JITTER;
                float hv = pr.hue_var + sib;
                hv = (hv < 0.0f) ? 0.0f : (hv > 1.0f) ? 1.0f : hv;
                sel.checker_hue_spread = hv * 3.14159265f;
            }
        }
    }

    sel.footprint_r = FOOTPRINT_RADIUS;
}

// ─── select_ribbon_for_patch ──────────────────────────────────
//
// Phase 1: tip-overlap idempotency check, then standard spawn
// preamble (gate + slot reserve), then tier selection with theme
// bias, then geometry sampling, then orientation-toward-pawn-away
// constraint with ±60° spread.
static bool select_ribbon_for_patch(RibbonState& rs, Cartridge* c,
    int32_t gx, int32_t gz, RibbonSelection& sel) {
    // Tip-overlap idempotency: reject if ANY active ribbon's
    // near or far tip falls within this trigger patch.
    for (uint32_t i = 0; i < MAX_RIBBON_INSTANCES; i++) {
        if (!rs.active[i].active) continue;
        if ((rs.active[i].near_tip_gx == gx && rs.active[i].near_tip_gz == gz) ||
            (rs.active[i].far_tip_gx == gx && rs.active[i].far_tip_gz == gz))
            return false;
    }
    auto gate = c->run_spawn_preamble(gx, gz,
        rs.active, MAX_RIBBON_INSTANCES,
        RibbonProp::SPAWN_ROLL, RibbonConfig::SPAWN_CHANCE,
        RibbonConfig::MOOD_MULTIPLIER,
        PopFamily::RIBBON, "ribn");
    if (!gate.ok) return false;

    // Tier selection with theme bias
    float tier_weights[RIBBON_TIER_COUNT];
    for (uint32_t t = 0; t < RIBBON_TIER_COUNT; t++)
        tier_weights[t] = RIBBON_BASE_TIER_WEIGHTS[t];
    for (uint32_t t = 0; t < RIBBON_TIER_COUNT; t++)
        tier_weights[t] *= THEMES[gate.theme_idx].tier_wt_ribbon[t];
    uint32_t tier_idx = c->select_tier_biased(gate.seed, RibbonProp::TIER,
        tier_weights, RIBBON_TIER_COUNT, PopFamily::RIBBON);

    sel.seed = gate.seed;
    sel.trigger_gx = gx;
    sel.trigger_gz = gz;
    sel.slot = gate.slot;
    sel.tier_idx = tier_idx;

    fill_ribbon_selection_geometry(gate.seed, tier_idx, sel);

    // Constrain orientation: ribbon body extends primarily away
    // from the pawn. The hash provides ±60° of spread around the
    // away direction so ribbons aren't all perfectly radial.
    {
        float patch_cx = (gx + 0.5f) * PATCH_EXTENT;
        float patch_cz = (gz + 0.5f) * PATCH_EXTENT;
        float away_angle = std::atan2(patch_cz - c->player_.readback_z,
            patch_cx - c->player_.readback_x);
        float hash_spread = cpu_hash_f(gate.seed, RibbonProp::ORIENTATION);
        sel.orientation = away_angle + (hash_spread * 2.0f - 1.0f) * ORIENTATION_SPREAD;
    }

    return true;
}

// ─── place_ribbon_from_selection ──────────────────────────────
//
// Phase 2: footprint negotiation with jitter + rotation, copy
// selection fields into the placement record.
//
// Note: this function takes no RibbonState — it only mediates
// between the selection and spawn-engine helpers (negotiate_position,
// record_placement_bookkeeping). It doesn't touch ribbon's data.
static bool place_ribbon_from_selection(Cartridge* c,
    const RibbonSelection& sel, RibbonPlacement& plan) {
    auto pos = c->negotiate_position(sel.seed,
        sel.trigger_gx, sel.trigger_gz,
        RibbonProp::ANCHOR_X, RibbonProp::ANCHOR_Z,
        RibbonConfig::POSITION_JITTER,
        RibbonProp::ORIENTATION,
        sel.footprint_r, PopFamily::RIBBON, sel.tier_idx);
    if (!pos.ok) return false;

    plan = RibbonPlacement{};
    plan.slot = sel.slot;
    plan.seed = sel.seed;
    plan.trigger_gx = sel.trigger_gx;
    plan.trigger_gz = sel.trigger_gz;
    plan.host_gx = pos.host_gx;
    plan.host_gz = pos.host_gz;
    plan.tier_idx = sel.tier_idx;
    plan.cx = pos.cx;
    plan.cz = pos.cz;

    plan.cube_count = sel.cube_count;
    plan.cube_size = sel.cube_size;
    plan.height = sel.height;
    plan.orientation = sel.orientation;
    plan.lateral_amp = sel.lateral_amp;
    plan.lateral_cycles = sel.lateral_cycles;
    plan.vertical_amp = sel.vertical_amp;
    plan.color_mode = sel.color_mode;
    std::memcpy(plan.color, sel.color, sizeof(plan.color));
    std::memcpy(plan.color_b, sel.color_b, sizeof(plan.color_b));
    plan.checker_scatter = sel.checker_scatter;
    plan.checker_hue_spread = sel.checker_hue_spread;

    c->record_placement_bookkeeping(PopFamily::RIBBON, plan.tier_idx);
    return true;
}

// ─── commit_ribbon ───────────────────────────────────────────
//
// Phase 3: write GPU state (CPU mirror, uploaded later by spine),
// register tip patches for cross-patch eviction tracking.
//
// Dual entry: also called from mood.inl::apply_mood for mood-5
// forced spawn (SEAM[ribbon:dual-entry]).
static void commit_ribbon(RibbonState& rs, Cartridge* c,
    const RibbonPlacement& plan,
    int32_t trigger_gx, int32_t trigger_gz, wgpu::Queue& queue)
{
    GPURibbonState r{};
    r.anchor[0] = plan.cx;
    r.anchor[1] = 0.0f;
    r.anchor[2] = plan.cz;
    r.time = c->time_state_.seconds;
    r.cube_count = plan.cube_count;
    r.cube_size = plan.cube_size;
    r.height = plan.height;
    r.orientation = plan.orientation;
    // Trail-frame: derive the temporal freq from authored cycles, the
    // per-tier propagation_speed, and the ribbon length. Frozen-frame
    // visible cycles are preserved; crests propagate head→tail at
    // propagation_speed. freq = cycles * 2pi * P / total_length. Vertical
    // shares the lateral cycles — one visible wavelength, two amplitudes.
    {
        const float P = RIBBON_TIERS[plan.tier_idx].propagation_speed;
        const float total_length = (float)plan.cube_count * plan.cube_size;
        const float k = 6.2831853f * P / std::max(total_length, 1e-6f);
        r.propagation_speed = P;
        r.lateral_freq  = plan.lateral_cycles * k;
        r.vertical_freq = r.lateral_freq;
    }
    r.lateral_amp = plan.lateral_amp;
    r.vertical_amp = plan.vertical_amp;
    r.color_mode = plan.color_mode;
    r.color[0] = plan.color[0];
    r.color[1] = plan.color[1];
    r.color[2] = plan.color[2];
    r.color_b[0] = plan.color_b[0];
    r.color_b[1] = plan.color_b[1];
    r.color_b[2] = plan.color_b[2];
    r.checker_scatter = plan.checker_scatter;
    r.hue_spread = plan.checker_hue_spread;
    r.seed = plan.seed;
    r.is_visible = 1u;
    r.is_roaming = 1u;   // head-roaming on (player-flown or wandering; parked when neither)

    // Store in CPU mirror (per-frame nearest-selection uploads to GPU)
    uint32_t s = plan.slot;
    rs.gpu[s] = r;

    auto& ar = rs.active[s];
    // Snapshot the spawn color as the idle base — the home the line-tint
    // coupling (gen-2, T2) mixes over: gpu.color = lerp(spawn, stim, mix)
    // in the conductor's flush; mix rests 0 ⇒ this color exactly.
    ar.spawn_color[0] = r.color[0];
    ar.spawn_color[1] = r.color[1];
    ar.spawn_color[2] = r.color[2];
    ar.spawn_lateral_amp  = r.lateral_amp;
    ar.spawn_vertical_amp = r.vertical_amp;
    ar.phase = r.time;    // seed = wall clock, so the 100 BPM anchor reproduces the old clock exactly
    ar.patch_gx = trigger_gx;
    ar.patch_gz = trigger_gz;
    ar.host_gx = plan.host_gx;
    ar.host_gz = plan.host_gz;
    ar.anchor_x = plan.cx;
    ar.anchor_z = plan.cz;

    // Wander: seed-driven — every decision a pure channel of the spawn seed
    // (RibbonProp 450-452), like tier, geometry, color, and position. The seed
    // determines the wander decision, the cruise, and (by seeding the runtime
    // stream) the entire waypoint sequence. Channel draws are stateless, so
    // this decade perturbs no existing draw.
    ar.wander = cpu_hash_f(plan.seed, RibbonProp::WANDER_ROLL) < WANDER_CHANCE;
    {
        float cr = cpu_sample_gaussian(plan.seed, RibbonProp::WANDER_CRUISE,
                                       WANDER_CRUISE_BASE, WANDER_CRUISE_SIGMA);
        ar.wander_cruise = (cr < WANDER_CRUISE_MIN) ? WANDER_CRUISE_MIN
                         : (cr > WANDER_CRUISE_MAX) ? WANDER_CRUISE_MAX : cr;
    }
    ar.wander_rng = 1u + (uint32_t)(cpu_hash_f(plan.seed, RibbonProp::WANDER_RNG)
                                    * 16777215.0f);
    // Hatchling rule: a newborn departs along its own body. Movement is
    // -heading = -orientation, so the first waypoint lies straight ahead;
    // the meander begins at the second pick. No pose contradiction at birth.
    ar.wander_tx = plan.cx - 300.0f * std::cos(r.orientation);
    ar.wander_tz = plan.cz - 300.0f * std::sin(r.orientation);
    ar.wander_retarget = WANDER_RETARGET_MIN;
    ar.wander_yaw_state = 0.0f;
    // Succession: a commit into the slot the head is wearing is a REBIRTH.
    // Slots are reused (MAX_RIBBON_INSTANCES = 1), so the slot-equality init
    // guard in ribbon_advance_head cannot see a successor — only this signal
    // can. Every ribbon life passes through commit (dispatch AND mood force-
    // spawn), so the head dies here, and no other site needs to remember.
    if (ribbon_head_is(rs, s))
        ribbon_invalidate_head(rs);

    // Two-tip anchoring: anchor IS the near tip (t=0).
    // Body extends entirely in the orientation direction (away from pawn).
    float total_length = (float)plan.cube_count * plan.cube_size;
    float dir_x = std::cos(plan.orientation);
    float dir_z = std::sin(plan.orientation);

    const float far_x = plan.cx + dir_x * total_length;
    const float far_z = plan.cz + dir_z * total_length;
    ar.near_tip_gx = (int32_t)std::floor(plan.cx / PATCH_EXTENT);
    ar.near_tip_gz = (int32_t)std::floor(plan.cz / PATCH_EXTENT);
    ar.far_tip_gx = (int32_t)std::floor(far_x / PATCH_EXTENT);
    ar.far_tip_gz = (int32_t)std::floor(far_z / PATCH_EXTENT);

    ar.near_tip_registered = false;
    ar.far_tip_registered = false;
    ar.ref_count = 0;

    ar.active = true;
    rs.active_count++;
    // SEAM[ribbon:L1] unconditional stdout — exhibition guard
    //   candidate. Same family as the [DIAG:*] stdout pattern
    //   noted across the codebase. Phase 1+ batch: wrap in
    //   #ifdef DIAG_RIBBON or similar before exhibition.
    std::cout << "[Ribbon] SPAWN slot=" << s << " at (" << plan.cx << ", " << plan.cz
        << ") tier=" << plan.tier_idx
        << " len=" << total_length
        << " near=(" << ar.near_tip_gx << "," << ar.near_tip_gz
        << ") far=(" << ar.far_tip_gx << "," << ar.far_tip_gz << ")\n";
}
