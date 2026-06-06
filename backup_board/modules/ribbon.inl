// ─── ribbon.inl ──────────────────────────────────────────────────
//
// Sky Ribbon: complete subsystem (vocabulary + machinery in one
// module). Single-instance, bespoke pipeline — runs through the
// 3-phase select/place/commit shape but doesn't share entity_pipeline's
// generic machinery.
//
// Flying ribbons: compound wave functions (lateral + vertical + twist)
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
// │  Shared geometry helper (also called by mood.inl::apply_mood     │
// │  for the mood-5 forced spawn path):                              │
// │    fill_ribbon_selection_geometry(seed, tier, terrain, sel)      │
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
//             pawnReadback_*, THEMES, PATCH_EXTENT, Dim::*).
//
// SEAM[ribbon:complete-subsystem] complete bespoke pipeline in one
//   module — vocabulary + state + machinery + lifecycle. Same family
//   as gol_zones (Ch. 12.B) and gallery (Ch. 12.E).
// SEAM[ribbon:dual-entry] commit_ribbon below has TWO callers:
//   FAMILY_DISPATCH[RIBBON].try_commit during patch streaming, AND
//   mood.inl::apply_mood for mood-5 forced spawn. The dual entry
//   point is owned by mood:K4 (mood-5 reference clone), not by
//   ribbon machinery. Tag-only awareness.
// SEAM[ribbon:P8] CPU mirrors of WGSL ribbon spine/tangent/rotor
//   functions (ribbon_spine_at_cpu, ribbon_tangent_cpu,
//   ribbon_rotor_diag) are authored but not yet called anywhere.
//   Latent infrastructure for future picking / queries / diagnostics
//   that will need to evaluate the ribbon's spine on CPU. Same
//   family as the harmonic-ratio P8 below.
// ─────────────────────────────────────────────────────────────────


// ═══ TUNING CONSOLE ══════════════════════════════════════════════
//
// System-level dials for the ribbon subsystem. Per-tier values
// (the Gaussian means/sigmas that shape each tier's feel) live in
// RIBBON_TIERS below. Everything here applies across all tiers.

// ── Spawn ────────────────────────────────────────────────────────
struct RibbonConfig {
    static constexpr float SPAWN_CHANCE = 0.400f;
    // SEAM[ribbon:P4] hygiene rows pattern — { open, sunset,
    //   [indoor_flat=0], [indoor_vault=0], [finite_outdoor=1],
    //   [finite_outdoor_ref=0] }. Same family as gol_zones:P4
    //   and floaters:P4.
    static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
    static constexpr float POSITION_JITTER = 0.3f;
};

// ── Length cap ───────────────────────────────────────────────────
// Total ribbon length (cube_count × cube_size) is capped here to
// keep anchor coverage viable (~30 patches max). RIBBON_MAX_LENGTH
// itself currently lives in spawn_engine.inl; consumed below in
// fill_ribbon_selection_geometry.


// ═══ COLOR VOCABULARY ════════════════════════════════════════════

struct RibbonColorMode {
    static constexpr uint32_t SMOOTH = 0;  // terrain-derived monochrome
    static constexpr uint32_t TINTED = 1;  // warm/cool hue shift
    static constexpr uint32_t CONTRAST = 2;  // high-contrast alternating segments
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


// ═══ PROPERTY INDEX REGISTRY ═════════════════════════════════════
//
// DONE[ribbon:L1] Stride convention (intentional, do not compact):
//   400      SPAWN_ROLL
//   401-409  per-instance scalar rolls (ANCHOR_X..PALETTE_IDX)
//   410-419  cube-count / size / height       (10-row reserve)
//   420-429  lateral wave  (amp, cycles, speed; rest reserved)
//   430-439  vertical wave (amp, ratio; rest reserved)
//   440-449  twist wave    (amp, ratio; rest reserved)
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
    static constexpr uint32_t LATERAL_SPEED = 422u;
    static constexpr uint32_t VERTICAL_AMP = 430u;
    static constexpr uint32_t VERTICAL_SPEED = 432u;
    static constexpr uint32_t VERTICAL_RATIO = 433u;    // seed roll for vertical harmonic ratio selection
    static constexpr uint32_t TWIST_AMP = 440u;
    static constexpr uint32_t TWIST_SPEED = 442u;
    static constexpr uint32_t TWIST_RATIO = 443u;       // seed roll for twist harmonic ratio selection
};


// ═══ HARMONIC RATIO PALETTES (P8 — latent) ═══════════════════════
//
// SEAM[ribbon:P8] latent infrastructure — the harmonic ratio
//   selector and palettes are authored but not yet wired. The
//   current fill_ribbon_selection_geometry overrides
//   vertical_cycles and twist_cycles to lateral values directly
//   ("overridden = lateral" notes in RIBBON_TIERS). Once the
//   harmonic-ratio system is consumed at runtime, the per-axis
//   ratio palettes below replace the override; until then this
//   block is the artist's note-to-self about what's coming.
//   Same family as gallery:ENVIRONMENTAL and the P8 inventory in
//   the WGSL audit.
//
// Secondary wave cycles (vertical, twist) are derived as simple
// ratios of the lateral fundamental. This eliminates irrational
// beating and gives each ribbon a harmonically coherent form.
//
// Ratios ≤ 1 keep secondary motion slower than lateral sway
// (contemplative, not snaky). Weights favor the middle intervals.

struct HarmonicRatio {
    float ratio;
    float weight;
    const char* name;      // musical interval name for diagnostics
};

static constexpr uint32_t VERTICAL_RATIO_COUNT = 4;
static constexpr HarmonicRatio VERTICAL_RATIOS[VERTICAL_RATIO_COUNT] = {
    { 1.0f / 3.0f,  0.15f, "1:3" },   // breathe at 1/3 of sway
    { 1.0f / 2.0f,  0.35f, "1:2" },   // octave below — one breath per two sways
    { 2.0f / 3.0f,  0.30f, "2:3" },   // fifth below — gentle polyrhythm
    { 1.0f / 1.0f,  0.20f, "1:1" },   // unison — breathe with sway
};

static constexpr uint32_t TWIST_RATIO_COUNT = 4;
static constexpr HarmonicRatio TWIST_RATIOS[TWIST_RATIO_COUNT] = {
    { 1.0f / 4.0f,  0.15f, "1:4" },   // very slow corkscrew
    { 1.0f / 3.0f,  0.30f, "1:3" },   // one turn per three sways
    { 1.0f / 2.0f,  0.35f, "1:2" },   // one turn per two sways
    { 2.0f / 3.0f,  0.20f, "2:3" },   // fifth below
};

static uint32_t select_harmonic_ratio(uint32_t seed, uint32_t prop,
    const HarmonicRatio* palette, uint32_t count) {
    float roll = cpu_hash_f(seed, prop);
    float cumul = 0.0f;
    for (uint32_t i = 0; i < count; i++) {
        cumul += palette[i].weight;
        if (roll < cumul) return i;
    }
    return count - 1;
}


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

    // Cube count: 100–400. Cube size: pawn height (1.5) to 4× (6.0).

    // ─── Altitude ────────────────────────────────────────────
    float height_mean, height_sigma;

    // Flying height: 50–80 units. All wave params independently seeded.

    // ─── Lateral wave ─────────────────────────────────────
    float lateral_amp_mean, lateral_amp_sigma;
    float lateral_cycles_mean, lateral_cycles_sigma;
    float lateral_speed_mean, lateral_speed_sigma;

    // ─── Vertical wave ────────────────────────────────────
    float vertical_amp_mean, vertical_amp_sigma;
    float vertical_cycles_mean, vertical_cycles_sigma;
    float vertical_speed_mean, vertical_speed_sigma;

    // ─── Twist (corkscrew) ───────────────────────────────────
    float twist_amp_mean, twist_amp_sigma;
    float twist_cycles_mean, twist_cycles_sigma;
    float twist_speed_mean, twist_speed_sigma;

    // ─── Selection ───────────────────────────────────────────
    float weight;
};

//                          ┌── Serpentine ──┬──── Helix ─────┬─── Streamer ───┐
//                          │   μ       σ    │   μ       σ    │   μ       σ    │
// ─── Geometry ────────────┤                │                │                │
//   cube_count             │ 350      60    │ 150      40    │ 250      50    │
//   cube_size              │   8.0     2.0  │   5.0     1.2  │   6.0     1.6  │
// ─── Altitude ────────────┤                │                │                │
//   height                 │  60      15    │  55      12    │  70      20    │
// ─── Lateral wave ────────┤                │                │                │
//   lateral_amp             │  10.0     0.6  │   3.5     0.6  │   5.5     0.8  │
//   lateral_cycles          │   1.5     0.4  │   3.0     0.8  │   2.0     0.5  │
//   lateral_speed           │   0.25   0.075 │   0.60   0.175 │   0.40    0.10 │
// ─── Vertical wave ───────┤  cycles + speed = lateral (P8: ratios pending)   │
//   vertical_amp            │   5.0     0.8  │   2.5     0.5  │   8.0     1.2  │
// ─── Twist (corkscrew) ───┤  cycles + speed = lateral (P8: ratios pending)   │
//   twist_amp              │   0.6     0.2  │   6.0     0.8  │   1.6     0.6  │
// ─── Selection ───────────┤                │                │                │
//   weight                 │   0.45         │   0.30         │   0.25         │
//                          └────────────────┴────────────────┴────────────────┘
static constexpr RibbonTierProfile RIBBON_TIERS[RIBBON_TIER_COUNT] = {
    // Tier 0: Serpentine — long, massive, slow motion
    // (Length matched to Streamer (Tier 2): 188 × 8.0 ≈ 1500 units, vs.
    //  Streamer's 250 × 6.0 = 1500. Keeps Serpentine's chunky 8.0 cube_size
    //  that distinguishes its character from the other tiers.)
    {   188.0f, 35.0f,     // cube_count
          8.0f,  2.0f,      // cube_size
         60.0f, 15.0f,      // height
         10.0f,  0.6f,      // lateral_amp
          1.5f,  0.4f,      // lateral_cycles
          0.25f, 0.075f,    // lateral_speed
          5.0f,  0.8f,      // vertical_amp
          0.8f,  0.2f,      // vertical_cycles (overridden = lateral)
          0.20f, 0.06f,     // vertical_speed (overridden = lateral)
          0.6f,  0.2f,      // twist_amp
          0.5f,  0.2f,      // twist_cycles (overridden = lateral)
          0.15f, 0.05f,     // twist_speed (overridden = lateral)
          0.45f },          // weight
    // Tier 1: Helix — tighter cycles, visible corkscrew
    {   150.0f, 40.0f,      // cube_count
          5.0f,  1.2f,      // cube_size
         55.0f, 12.0f,      // height
          3.5f,  0.6f,      // lateral_amp
          3.0f,  0.8f,      // lateral_cycles
          0.60f, 0.175f,    // lateral_speed
          2.5f,  0.5f,      // vertical_amp
          2.5f,  0.6f,      // vertical_cycles (overridden = lateral)
          0.50f, 0.15f,     // vertical_speed (overridden = lateral)
          6.0f,  0.8f,      // twist_amp
          2.0f,  0.5f,      // twist_cycles (overridden = lateral)
          0.20f, 0.05f,     // twist_speed (overridden = lateral)
          0.30f },          // weight
    // Tier 2: Streamer — tall vertical form, deep breathing
    {   250.0f, 50.0f,      // cube_count
          6.0f,  1.6f,      // cube_size
         70.0f, 20.0f,      // height
          5.5f,  0.8f,      // lateral_amp
          2.0f,  0.5f,      // lateral_cycles
          0.40f, 0.10f,     // lateral_speed
          8.0f,  1.2f,      // vertical_amp
          1.2f,  0.3f,      // vertical_cycles (overridden = lateral)
          0.325f, 0.075f,   // vertical_speed (overridden = lateral)
          1.6f,  0.6f,      // twist_amp
          1.5f,  0.4f,      // twist_cycles (overridden = lateral)
          0.25f, 0.08f,     // twist_speed (overridden = lateral)
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
// Per-instance ribbon state and the GPU-state mirrors. Single-render
// today (MAX_RIBBON_INSTANCES = 1), structured to scale when the GPU
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
    float near_tip_x = 0.0f, near_tip_z = 0.0f;
    float far_tip_x = 0.0f, far_tip_z = 0.0f;
    int32_t near_tip_gx = 0, near_tip_gz = 0;
    int32_t far_tip_gx = 0, far_tip_gz = 0;
    bool near_tip_registered = false;
    bool far_tip_registered = false;
    uint32_t ref_count = 0;     // patches referencing this ribbon via record_entity
    bool active = false;
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

    // Mood-5 ribbon anchor offset. Seed-derived position centered on
    // the finite world. Adjust to manually shift the mood-5 anchor XZ
    // (read by mood.inl::apply_mood).
    float          mood_offset[2] = { 0.0f, 0.0f };
};
RibbonState ribbon_state_;


// ═══ LIFECYCLE — three-phase + shared helper ═════════════════════

// ─── fill_ribbon_selection_geometry ───────────────────────────
// Shared geometry + color sampler used by both the dispatch
// pipeline and the mood forced-spawn path (see SEAM[ribbon:dual-entry]).
// Pure: no ribbon state access; all sampling is from `seed`.
static void fill_ribbon_selection_geometry(
    uint32_t seed, uint32_t tier_idx, float terrain_est,
    RibbonSelection& sel)
{
    const auto& tp = RIBBON_TIERS[tier_idx];

    float count_f = std::max(20.0f,
        cpu_sample_gaussian(seed, RibbonProp::CUBE_COUNT, tp.cube_count_mean, tp.cube_count_sigma));
    sel.cube_count = std::min((uint32_t)count_f, Dim::RIBBON_MAX_RINGS);
    sel.cube_size = std::max(1.0f,
        cpu_sample_gaussian(seed, RibbonProp::CUBE_SIZE, tp.cube_size_mean, tp.cube_size_sigma));

    // Length cap — keeps anchor coverage viable (~30 patches max)
    if ((float)sel.cube_count * sel.cube_size > RIBBON_MAX_LENGTH)
        sel.cube_count = (uint32_t)(RIBBON_MAX_LENGTH / sel.cube_size);

    sel.height = terrain_est + std::max(20.0f,
        cpu_sample_gaussian(seed, RibbonProp::HEIGHT, tp.height_mean, tp.height_sigma));

    sel.orientation = cpu_hash_f(seed, RibbonProp::ORIENTATION) * 6.2831853f;

    sel.lateral_amp = std::max(0.1f, cpu_sample_gaussian(seed, RibbonProp::LATERAL_AMP, tp.lateral_amp_mean, tp.lateral_amp_sigma));
    sel.lateral_cycles = std::max(0.1f, cpu_sample_gaussian(seed, RibbonProp::LATERAL_CYCLES, tp.lateral_cycles_mean, tp.lateral_cycles_sigma));
    sel.lateral_speed = std::max(0.005f, cpu_sample_gaussian(seed, RibbonProp::LATERAL_SPEED, tp.lateral_speed_mean, tp.lateral_speed_sigma));

    sel.vertical_amp = std::max(0.1f, cpu_sample_gaussian(seed, RibbonProp::VERTICAL_AMP, tp.vertical_amp_mean, tp.vertical_amp_sigma));
    sel.vertical_cycles = sel.lateral_cycles;
    sel.vertical_speed = sel.lateral_speed;

    sel.twist_amp = std::max(0.0f, cpu_sample_gaussian(seed, RibbonProp::TWIST_AMP, tp.twist_amp_mean, tp.twist_amp_sigma));
    sel.twist_cycles = sel.lateral_cycles;
    sel.twist_speed = sel.lateral_speed;

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
        float var = cpu_hash_f(seed, RibbonProp::COLOR_R) * 0.10f - 0.05f;
        sel.color[0] = RIBBON_SMOOTH_PALETTE[pal_idx][0] + var;
        sel.color[1] = RIBBON_SMOOTH_PALETTE[pal_idx][1] + var * 0.8f;
        sel.color[2] = RIBBON_SMOOTH_PALETTE[pal_idx][2] + var * 0.6f;
    }
    else if (sel.color_mode == RibbonColorMode::TINTED) {
        sel.color[0] = cpu_hash_f(seed, RibbonProp::COLOR_R) * 0.45f + 0.40f;
        sel.color[1] = cpu_hash_f(seed, RibbonProp::COLOR_G) * 0.40f + 0.35f;
        sel.color[2] = cpu_hash_f(seed, RibbonProp::COLOR_B) * 0.45f + 0.35f;
    }
    else {
        float hue = cpu_hash_f(seed, RibbonProp::COLOR_R);
        sel.color[0] = 0.20f + hue * 0.35f;
        sel.color[1] = 0.18f + (1.0f - hue) * 0.30f;
        sel.color[2] = 0.22f + cpu_hash_f(seed, RibbonProp::COLOR_B) * 0.25f;
    }

    sel.footprint_r = 5.0f;
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

    float terrain_est = c->estimate_terrain_height(
        (gx + 0.5f) * PATCH_EXTENT, (gz + 0.5f) * PATCH_EXTENT);

    fill_ribbon_selection_geometry(gate.seed, tier_idx, terrain_est, sel);

    // Constrain orientation: ribbon body extends primarily away
    // from the pawn. The hash provides ±60° of spread around the
    // away direction so ribbons aren't all perfectly radial.
    {
        float patch_cx = (gx + 0.5f) * PATCH_EXTENT;
        float patch_cz = (gz + 0.5f) * PATCH_EXTENT;
        float away_angle = std::atan2(patch_cz - c->player_.readback_z,
            patch_cx - c->player_.readback_x);
        constexpr float SPREAD = 1.0472f; // ±60° = π/3
        float hash_spread = cpu_hash_f(gate.seed, RibbonProp::ORIENTATION);
        sel.orientation = away_angle + (hash_spread * 2.0f - 1.0f) * SPREAD;
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
    plan.trigger_gx = sel.trigger_gx;
    plan.trigger_gz = sel.trigger_gz;
    plan.host_gx = pos.host_gx;
    plan.host_gz = pos.host_gz;
    plan.tier_idx = sel.tier_idx;
    plan.cx = pos.cx;
    plan.cz = pos.cz;
    plan.rotation = pos.rotation;

    plan.cube_count = sel.cube_count;
    plan.cube_size = sel.cube_size;
    plan.height = sel.height;
    plan.orientation = sel.orientation;
    plan.lateral_amp = sel.lateral_amp;
    plan.lateral_cycles = sel.lateral_cycles;
    plan.lateral_speed = sel.lateral_speed;
    plan.vertical_amp = sel.vertical_amp;
    plan.vertical_cycles = sel.vertical_cycles;
    plan.vertical_speed = sel.vertical_speed;
    plan.twist_amp = sel.twist_amp;
    plan.twist_cycles = sel.twist_cycles;
    plan.twist_speed = sel.twist_speed;
    plan.color_mode = sel.color_mode;
    std::memcpy(plan.color, sel.color, sizeof(plan.color));

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
    r.lateral_amp = plan.lateral_amp;
    r.lateral_cycles = plan.lateral_cycles;
    r.lateral_speed = plan.lateral_speed;
    r.vertical_amp = plan.vertical_amp;
    r.vertical_cycles = plan.vertical_cycles;
    r.vertical_speed = plan.vertical_speed;
    r.twist_amp = plan.twist_amp;
    r.twist_cycles = plan.twist_cycles;
    r.twist_speed = plan.twist_speed;
    r.color_mode = plan.color_mode;
    r.color[0] = plan.color[0];
    r.color[1] = plan.color[1];
    r.color[2] = plan.color[2];
    r.is_visible = 1u;

    // Store in CPU mirror (per-frame nearest-selection uploads to GPU)
    uint32_t s = plan.slot;
    rs.gpu[s] = r;

    auto& ar = rs.active[s];
    ar.patch_gx = trigger_gx;
    ar.patch_gz = trigger_gz;
    ar.host_gx = plan.host_gx;
    ar.host_gz = plan.host_gz;
    ar.anchor_x = plan.cx;
    ar.anchor_z = plan.cz;

    // Two-tip anchoring: anchor IS the near tip (t=0).
    // Body extends entirely in the orientation direction (away from pawn).
    float total_length = (float)plan.cube_count * plan.cube_size;
    float dir_x = std::cos(plan.orientation);
    float dir_z = std::sin(plan.orientation);

    ar.near_tip_x = plan.cx;
    ar.near_tip_z = plan.cz;
    ar.far_tip_x = plan.cx + dir_x * total_length;
    ar.far_tip_z = plan.cz + dir_z * total_length;

    ar.near_tip_gx = (int32_t)std::floor(ar.near_tip_x / PATCH_EXTENT);
    ar.near_tip_gz = (int32_t)std::floor(ar.near_tip_z / PATCH_EXTENT);
    ar.far_tip_gx = (int32_t)std::floor(ar.far_tip_x / PATCH_EXTENT);
    ar.far_tip_gz = (int32_t)std::floor(ar.far_tip_z / PATCH_EXTENT);

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


// ═══ CPU MIRRORS (P8 — latent) ═══════════════════════════════════
//
// SEAM[ribbon:P8] CPU mirrors of WGSL ribbon spine/tangent/rotor
//   functions. Authored but not yet called anywhere. Latent
//   infrastructure for future picking / queries / diagnostics
//   that need to evaluate the ribbon's spine on CPU. Keep aligned
//   with the WGSL implementations whenever those change.

// CPU mirror of WGSL ribbon_spine_at — evaluate one ring's world position.
static void ribbon_spine_at_cpu(const GPURibbonState& r, float time, uint32_t ring_idx, float out[3]) {
    constexpr float PI = 3.14159265359f;
    float t = (float)ring_idx / (float)std::max(r.cube_count - 1u, 1u);
    float total_length = (float)r.cube_count * r.cube_size;

    float along = t * total_length;
    float lateral = std::sin(time * r.lateral_speed + t * r.lateral_cycles * 2.0f * PI) * r.lateral_amp;
    float vertical = r.height + std::sin(time * r.vertical_speed + t * r.vertical_cycles * 2.0f * PI) * r.vertical_amp;

    float c = std::cos(r.orientation);
    float s = std::sin(r.orientation);
    float rotated_along = along * c - lateral * s;
    float rotated_lateral = along * s + lateral * c;

    float twist_phase = time * r.twist_speed + t * r.twist_cycles * 2.0f * PI;
    float twist_depth = std::sin(twist_phase) * 0.4f * r.twist_amp;
    float twist_vert = std::cos(twist_phase) * 0.3f * r.twist_amp;

    out[0] = r.anchor[0] + rotated_along;
    out[1] = vertical + twist_vert;
    out[2] = r.anchor[2] + rotated_lateral + twist_depth;
}

// CPU mirror of WGSL ribbon_tangent_at — central finite difference.
static void ribbon_tangent_cpu(const GPURibbonState& r, float time, uint32_t ring_idx, float out[3]) {
    constexpr float eps = 0.0005f;
    float t = (float)ring_idx / (float)std::max(r.cube_count - 1u, 1u);
    // Evaluate spine at t±eps using the raw parametric form
    auto eval = [&](float tp, float p[3]) {
        constexpr float PI = 3.14159265359f;
        float total_length = (float)r.cube_count * r.cube_size;
        float along = tp * total_length;
        float lateral = std::sin(time * r.lateral_speed + tp * r.lateral_cycles * 2.0f * PI) * r.lateral_amp;
        float vertical = r.height + std::sin(time * r.vertical_speed + tp * r.vertical_cycles * 2.0f * PI) * r.vertical_amp;
        float c = std::cos(r.orientation);
        float s = std::sin(r.orientation);
        float rotated_along = along * c - lateral * s;
        float rotated_lateral = along * s + lateral * c;
        float twist_phase = time * r.twist_speed + tp * r.twist_cycles * 2.0f * PI;
        float twist_depth = std::sin(twist_phase) * 0.4f * r.twist_amp;
        float twist_vert = std::cos(twist_phase) * 0.3f * r.twist_amp;
        p[0] = r.anchor[0] + rotated_along;
        p[1] = vertical + twist_vert;
        p[2] = r.anchor[2] + rotated_lateral + twist_depth;
    };
    float a[3], b[3];
    eval(t + eps, a);
    eval(t - eps, b);
    float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    float len = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (len < 1e-8f) { out[0] = 1; out[1] = 0; out[2] = 0; return; }
    out[0] = dx / len; out[1] = dy / len; out[2] = dz / len;
}

// CPU mirror of rotor construction — returns axis, angle, and whether it degenerated.
struct RotorDiag {
    float axis[3];
    float angle_deg;
    float cross_len;   // length of cross(forward, tangent) — near 0 = degenerate
    bool antiparallel; // tangent ≈ -forward
};
static RotorDiag ribbon_rotor_diag(const float tangent[3]) {
    RotorDiag d{};
    // forward = (1,0,0)
    // cross(forward, tangent) = (0*tz - 0*ty, 0*tx - 1*tz, 1*ty - 0*tx)
    //                         = (0, -tz, ty)
    d.axis[0] = 0.0f;
    d.axis[1] = -tangent[2];
    d.axis[2] = tangent[1];
    d.cross_len = std::sqrt(d.axis[1] * d.axis[1] + d.axis[2] * d.axis[2]);
    float dot = tangent[0]; // dot((1,0,0), tangent)
    d.angle_deg = std::acos(std::max(-1.0f, std::min(1.0f, dot))) * 57.2958f;
    d.antiparallel = (d.cross_len < 0.001f && dot < 0.0f);
    return d;
}

