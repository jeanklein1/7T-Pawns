// ─── gol_zones.inl ──────────────────────────────────────────────
//
// Zone-local Game of Life + Pulse automata. Each zone is a 32×32
// grid anchored to a mode lattice node. Conway zones evolve by
// neighbor rules; Pulse zones breathe periodically.
//
// Architecture (mirrors the Column entity pattern):
//   GoLZoneProp        — property index registry (seed-based rolls)
//   GoLZoneSpawnConfig — spawn chances and spatial constants
//   GoLTierProfile     — mean+sigma tier matrix (Gaussian sampling)
//   PulseTierProfile   — analogous matrix for Pulse zones
//   GoLColorMode       — color tier weights (declarative)
//   GoLZoneState       — per-instance runtime state
//
// ┌─── Public surface (called from outside this file) ──────────────┐
// │                                                                  │
// │  Module functions are static, take GoLState& explicitly.         │
// │  This makes ownership language-visible and cross-cutting         │
// │  dependencies explicit in function signatures.                   │
// │                                                                  │
// │  Lifecycle (three-phase + helper):                               │
// │    select_gol_for_patch(gs, c, gx, gz, sel)   — Phase 1: roll    │
// │    place_gol_from_selection(c, sel, plan)     — Phase 2: place   │
// │      (note: takes no GoLState — only mediates between sel and    │
// │       spawn-engine helpers; not part of GoL's data)              │
// │    commit_gol(gs, c, plan, gx, gz, queue)     — Phase 3: state   │
// │    seed_gol_zone(gs, c, slot, queue)          — life buffer init │
// │                                                                  │
// │  Per-frame:                                                      │
// │    upload_gol_zone_config(gs, c, queue)       — config+tick mask │
// │    flush_zone_derive_requests(gs, c, queue)   — GPU dispatch     │
// │                                                                  │
// │  Cross-module reads (consumed by other modules):                 │
// │    gol_state_.zones[], gol_state_.zone_count   — read by spine   │
// │    gol_state_.mood_allowed                     — read by spine   │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// Included inside the Cartridge class body.
// Depends on: seed_utils.inl (cpu_hash, cpu_lattice_node_seed,
//             cpu_sample_gaussian), spawn_engine.inl (footprint
//             registry, population biases), musical.inl
//             (GOL_TICK_GAIN, MMODE_GOL_TEMPO).
//
// SEAM[gol_zones:complete-subsystem] complete bespoke pipeline in one
//   block — vocabulary + state + lifecycle + dispatch all together.
//   Distinguishable from the cockpit pattern (multiple decoupled
//   commands); this is single-lifecycle bespoke. Same family as
//   gallery (Ch. 12.E) and ribbon (Ch. 13.C).
// SEAM[gol_zones:dual-algorithm] this module houses two algorithms —
//   Conway (GoLTierProfile, GOL_TIERS[]) and Pulse (PulseTierProfile,
//   PULSE_TIERS[]) — gated by PULSE_ALGORITHM_CHANCE. The shared
//   infrastructure (zone state, seeding, dispatch) is single-track;
//   only the parameter sampling and life initialization branch.
// ─────────────────────────────────────────────────────────────────


// ═══ TUNING CONSOLE ══════════════════════════════════════════════
//
// System-level dials for the GoL zone subsystem. Per-tier values
// (the actual Gaussian means/sigmas that shape each tier's feel)
// live in GOL_TIERS and PULSE_TIERS below — those are the
// per-tier consoles. Everything here applies across all tiers.

// ── Spatial constants ────────────────────────────────────────────
// DONE[gol_zones:L1] MUST match world.wgsl's MODE_LATTICE_SPACING
//   (in TUNING SURFACE DIRECTORY: "MODE_LATTICE_SPACING 120 wu —
//   smooth/discrete clusters"). Hardware mirror — same family as
//   agents:L2 / sample_gaussian / lattice_node_seed.
//   When tuning, change both sides.
static constexpr float MODE_LATTICE_SPACING = 120.0f;
static constexpr float PATCH_CELL_SIZE = (float)Dim::PATCH_EXTENT / 16.0f;  // 3.125

// ── Algorithm gate ───────────────────────────────────────────────
// Probability that a freshly-spawned zone is Pulse rather than
// Conway. The dial that controls the dual-algorithm balance —
// 0.0 = pure Conway, 1.0 = pure Pulse, 0.35 = current mix.
static constexpr float PULSE_ALGORITHM_CHANCE = 0.35f;


// ═══ ALGORITHM TYPES (shared) ════════════════════════════════════

struct AlgorithmType {
    static constexpr uint32_t CONWAY = 0;
    static constexpr uint32_t PULSE = 1;
};

struct BoundaryMode {
    static constexpr uint32_t REFLECT = 0;
    static constexpr uint32_t WRAP = 1;
};


// ═══ ZONE-LEVEL VOCABULARY (shared across both algorithms) ═══════

// ── Property Index Registry (seed band 250, indices 920–939) ─────
struct GoLZoneProp {
    static constexpr uint32_t SEED_BAND = 250u;
    // Zone-level decisions
    static constexpr uint32_t SPAWN_ROLL = 920u;
    static constexpr uint32_t TIER = 921u;
    static constexpr uint32_t HEIGHT_ROLL = 922u;
    static constexpr uint32_t COLOR_ROLL = 923u;
    // Per-zone continuous parameters (Gaussian draws)
    static constexpr uint32_t DENSITY = 930u;
    static constexpr uint32_t TICK_PERIOD = 931u;
    static constexpr uint32_t SPRING = 932u;
    static constexpr uint32_t HEIGHT = 933u;
    static constexpr uint32_t TRANSITION = 934u;
    // Per-zone color target
    static constexpr uint32_t TARGET_R = 935u;
    static constexpr uint32_t TARGET_G = 936u;
    static constexpr uint32_t TARGET_B = 937u;
    // Per-cell seeding
    static constexpr uint32_t HEIGHT_FACTOR = 938u;
};

// ── Spawn Configuration ──────────────────────────────────────────
struct GoLZoneSpawnConfig {
    static constexpr float SPAWN_CHANCE = 0.15f;  // fraction of checkerboard zones
    static constexpr float HEIGHT_CHANCE = 0.30f;  // fraction of zones that get extrusion
    static constexpr float ZONE_EXTENT = 100.0f; // 32 × 3.125 = cell-aligned
    static constexpr float MODE_THRESHOLD = 0.50f;  // min interpolated mode for eligibility
    // Per-cell height factor seeding (Gaussian draw per cell)
    static constexpr float HEIGHT_FACTOR_MEAN = 1.0f;
    static constexpr float HEIGHT_FACTOR_SIGMA = 0.15f;
    static constexpr float HEIGHT_FACTOR_CLAMP_LO = 0.6f;
    static constexpr float HEIGHT_FACTOR_CLAMP_HI = 1.4f;
    // Lens target color range: color = hash * RANGE + LO
    static constexpr float LENS_TARGET_LO = 0.2f;
    static constexpr float LENS_TARGET_RANGE = 0.6f;
    // Footprint: inscribed circle of 100×100 zone
    static constexpr float FOOTPRINT_RADIUS = 50.0f;
    // SEAM[gol_zones:P4] hygiene rows pattern (P4): MOOD_MULTIPLIER
    //   is { open, sunset, [indoor_flat=0], [indoor_vault=0],
    //   [finite_outdoor=1], [finite_outdoor_ref=0] }. The 0
    //   entries are reachable via mood IDs but the gate
    //   intentionally suppresses them. Same family as
    //   floaters:P4 (cube populations). Defensive declaration.
    // Mood gate (suppressed in flat/vault/finR — same as spheres/cubes)
    static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
};

// ── Color Modes ──────────────────────────────────────────────────
struct GoLColorMode {
    static constexpr uint32_t NEUTRAL = 0;  // no color change (height-only extrusion)
    static constexpr uint32_t LENS = 1;  // shift toward per-zone target color
    static constexpr uint32_t BLACKISH = 2;  // darken toward near-black
    static constexpr uint32_t COUNT = 3;

    // Weight matrix: color_mode selection weights
    // Index 0 = NEUTRAL (only available if height_enabled)
    // Index 1 = LENS
    // Index 2 = BLACKISH
    static constexpr float WEIGHTS_HEIGHT[COUNT] = { 0.30f, 0.40f, 0.30f };
    static constexpr float WEIGHTS_NO_HEIGHT[COUNT] = { 0.00f, 0.55f, 0.45f };
};


// ═══ CONWAY ALGORITHM ════════════════════════════════════════════

// ── Tier Profile (mean+sigma, matches ColumnTierParams pattern) ──
static constexpr uint32_t GOL_TIER_COUNT = 7;

struct GoLTierProfile {
    // ─── Initial conditions ──────────────────────────────────
    float density_mean, density_sigma;

    // ─── Temporal ────────────────────────────────────────────
    float tick_period_mean, tick_period_sigma;

    // ─── Visual transition ───────────────────────────────────
    float spring_stiffness_mean, spring_stiffness_sigma;
    float transition_fraction_mean, transition_fraction_sigma;

    // ─── Height ──────────────────────────────────────────────
    float alive_height_mean, alive_height_sigma;

    // ─── Per-cell variation ──────────────────────────────────
    float spring_variance;     // [0,1] per-cell spring speed scatter

    // ─── Selection ───────────────────────────────────────────
    float weight;
    bool  force_no_height;
};

//                                          dens_μ   σ    tick_μ  σ    spring_μ σ    trans_μ  σ     ht_μ    σ    sv    wt    no_h
static constexpr GoLTierProfile GOL_TIERS[GOL_TIER_COUNT] = {
    /* 0: Pillars  */ { 0.30f, 0.05f,   8.0f, 2.0f,   0.5f, 0.1f,   0.05f, 0.01f,  30.0f, 9.0f,  0.30f,  0.10f, false },
    /* 1: Sparse   */ { 0.15f, 0.05f,   2.0f, 0.5f,   4.0f, 1.0f,   0.12f, 0.03f,  18.0f, 6.0f,  0.20f,  0.20f, false },
    /* 2: Moderate */ { 0.30f, 0.08f,   1.0f, 0.3f,   8.0f, 2.0f,   0.15f, 0.03f,   9.0f, 3.0f,  0.15f,  0.18f, false },
    /* 3: Dense    */ { 0.45f, 0.10f,   0.5f, 0.15f, 12.0f, 3.0f,   0.25f, 0.05f,   6.0f, 1.5f,  0.10f,  0.10f, false },
    /* 4: Flash    */ { 0.35f, 0.10f,  0.25f, 0.05f, 20.0f, 5.0f,   0.30f, 0.05f,   0.0f, 0.0f,  0.40f,  0.17f, true  },
    /* 5: Monolith */ { 0.20f, 0.03f,  12.0f, 3.0f,   0.3f, 0.05f,  0.03f, 0.01f,  42.0f, 12.f,  0.05f,  0.12f, false },
    /* 6: Glacier  */ { 0.12f, 0.03f,   4.0f, 1.0f,   2.0f, 0.5f,   0.08f, 0.02f,  24.0f, 7.5f,  0.25f,  0.13f, false },
};

static constexpr const char* GOL_TIER_NAMES[] = {
    "Pillars", "Sparse", "Moderate", "Dense",
    "Flash", "Monolith", "Glacier"
};

static constexpr const char* GOL_COLOR_NAMES[] = {
    "neutral", "lens", "blackish"
};


// ═══ PULSE ALGORITHM ═════════════════════════════════════════════
//
// Pulse zones: periodic breathing of cell color/height, no neighbor
// rules. Each cell oscillates between terrain base and a displaced
// target.

// ── Property Indices for Pulse-specific parameters ───────────────
struct PulseZoneProp {
    static constexpr uint32_t ALGORITHM_ROLL = 950u;
    static constexpr uint32_t PULSE_TIER = 951u;
    static constexpr uint32_t PHASE_RANDOM = 952u;
    static constexpr uint32_t WANDER = 953u;
    static constexpr uint32_t TEMPO_RANDOM = 954u;
};

// ── Pulse Tier Profile ───────────────────────────────────────────
static constexpr uint32_t PULSE_TIER_COUNT = 3;

struct PulseTierProfile {
    // ─── Temporal ────────────────────────────────────────────
    float tick_period_mean, tick_period_sigma;

    // ─── Visual transition ───────────────────────────────────
    float spring_stiffness_mean, spring_stiffness_sigma;
    float transition_fraction_mean, transition_fraction_sigma;

    // ─── Phase scatter ───────────────────────────────────────
    float phase_randomness_mean, phase_randomness_sigma;

    // ─── Tempo scatter ───────────────────────────────────────
    float tempo_randomness_mean, tempo_randomness_sigma;

    // ─── Height ──────────────────────────────────────────────
    float alive_height_mean, alive_height_sigma;

    // ─── Wander ──────────────────────────────────────────────
    float wander_radius_mean, wander_radius_sigma;

    // ─── Per-cell variation ──────────────────────────────────
    float spring_variance;

    // ─── Selection ───────────────────────────────────────────
    float weight;
    bool  force_no_height;
    uint32_t boundary_mode;
};

//                                              tick_μ   σ    spring_μ σ    trans_μ  σ    phase_μ  σ    tempo_μ σ    ht_μ   σ    wand_μ  σ    sv    wt    no_h  bnd
static constexpr PulseTierProfile PULSE_TIERS[PULSE_TIER_COUNT] = {
    /* 0: Breathe  */ { 2.0f, 0.5f,   4.0f, 1.0f,   0.20f, 0.05f,   0.15f, 0.05f,   0.10f, 0.03f,   2.0f, 0.8f,  10.0f, 3.0f,   0.20f,  0.45f, false, BoundaryMode::REFLECT },
    /* 1: Sparkle  */ { 0.5f, 0.15f, 12.0f, 3.0f,   0.25f, 0.05f,   0.90f, 0.10f,   0.60f, 0.15f,   0.0f, 0.0f,   5.0f, 2.0f,   0.50f,  0.30f, true,  BoundaryMode::REFLECT },
    /* 2: Drift    */ { 4.0f, 1.0f,   1.5f, 0.4f,   0.10f, 0.03f,   0.50f, 0.15f,   0.40f, 0.10f,   4.0f, 1.5f,  25.0f, 8.0f,   0.35f,  0.25f, false, BoundaryMode::WRAP    },
};

static constexpr const char* PULSE_TIER_NAMES[] = {
    "Breathe", "Sparkle", "Drift"
};


// ═══ RUNTIME CPU STATE ═══════════════════════════════════════════
//
// Everything the CPU tracks for the GoL zone subsystem while
// running. Sub-grouped by role.

// ── Per-instance zone state ──────────────────────────────────────
// CPU retains only what's needed for: tick mask computation, life
// seeding, and zone slot lifecycle. All visual/spring/color
// parameters are GPU-derived.
struct GoLZoneState {
    int32_t zone_nx = 0, zone_nz = 0;
    int32_t host_gx = 0, host_gz = 0;   // host patch (for entity_refs eviction)
    bool active = false;
    uint32_t algorithm = AlgorithmType::CONWAY;
    float tick_period = 1.0f;        // CPU derives this for tick mask (matches GPU)
    float initial_density = 0.3f;    // CPU needs this for life buffer seeding
    int32_t last_tick_index = -1;
};

// ── GoL module state (Scope B migration #2) ──────────────────────
// All GoL-zone-owned state lives in this struct, accessed via
// gol_state_ on the Cartridge. Module functions take `GoLState& gs`
// explicitly rather than reaching via Cartridge*, making ownership
// language-visible and dependencies explicit in signatures.
struct GoLState {
    GoLZoneState zones[Dim::MAX_GOL_ZONES]{};
    uint32_t     zone_count = 0;
    uint32_t     active_slot_count = 0;     // highest active slot + 1 (for dispatch sizing)

    // Mood gate: set by apply_mood from MoodProfile flags. Default = on.
    // Read by the spine's spawn-detection callback to skip GoL spawning
    // entirely in moods that don't allow zones.
    bool         mood_allowed = true;

    // Derive request queue: accumulated during patch gen, flushed once
    // per frame as a single GPU compute dispatch (zone_derive_params).
    GPUZoneDeriveRequestArray pending_derive_requests{};
};
GoLState gol_state_;


// ═══ LIFECYCLE — three-phase + helper ════════════════════════════

// ─── select_gol_for_patch ─────────────────────────────────────
//
// Phase 1: lattice-node-gated selection. Scans which MODE_LATTICE
// nodes overlap this patch, runs spawn roll, algorithm + tier
// selection, parameter sampling. At most one zone per patch.

static bool select_gol_for_patch(GoLState& gs, Cartridge* c,
    int32_t gx, int32_t gz, GoLSelection& sel) {
    // Mood gate
    float adj_mod = GoLZoneSpawnConfig::MOOD_MULTIPLIER[c->mood_state_.active];
    if (adj_mod <= 0.0f) return false;

    // Density + theme modifiers
    adj_mod *= GLOBAL_ENTITY_DENSITY;
    adj_mod *= c->population_type_affinity(PopFamily::GOL);
    {
        auto dit = c->tileCache_.find({ gx, gz });
        if (dit != c->tileCache_.end()) {
            adj_mod *= dit->second.entity_density;
            adj_mod *= dit->second.theme_spawn[PopFamily::GOL];
        }
    }

    // Scan lattice nodes overlapping this patch
    float wx0 = gx * PATCH_EXTENT;
    float wx1 = (gx + 1) * PATCH_EXTENT;
    float wz0 = gz * PATCH_EXTENT;
    float wz1 = (gz + 1) * PATCH_EXTENT;

    int32_t nx0 = (int32_t)std::floor(wx0 / MODE_LATTICE_SPACING);
    int32_t nx1 = (int32_t)std::floor(wx1 / MODE_LATTICE_SPACING);
    int32_t nz0 = (int32_t)std::floor(wz0 / MODE_LATTICE_SPACING);
    int32_t nz1 = (int32_t)std::floor(wz1 / MODE_LATTICE_SPACING);

    for (int32_t nz = nz0; nz <= nz1; nz++) {
        for (int32_t nx = nx0; nx <= nx1; nx++) {
            // Zone center from lattice node
            float raw_cx = (nx + 0.5f) * MODE_LATTICE_SPACING;
            float raw_cz = (nz + 0.5f) * MODE_LATTICE_SPACING;

            // Authoritative patch: only the patch containing the center owns this node
            int32_t auth_gx = (int32_t)std::floor(raw_cx / PATCH_EXTENT);
            int32_t auth_gz = (int32_t)std::floor(raw_cz / PATCH_EXTENT);
            if (auth_gx != gx || auth_gz != gz) continue;

            // Idempotency: already active at this node?
            bool exists = false;
            for (uint32_t i = 0; i < Dim::MAX_GOL_ZONES; i++) {
                if (gs.zones[i].active &&
                    gs.zones[i].zone_nx == nx && gs.zones[i].zone_nz == nz) {
                    exists = true; break;
                }
            }
            if (exists) continue;

            // Spawn roll with modifiers
            uint32_t seed = cpu_lattice_node_seed(c->world_state_.active_seed, nx, nz, GoLZoneProp::SEED_BAND);
            float roll = cpu_hash_f(seed, GoLZoneProp::SPAWN_ROLL);
            float chance = GoLZoneSpawnConfig::SPAWN_CHANCE * adj_mod;
            chance += c->population_automata_bias();
            chance = std::max(0.0f, std::min(1.0f, chance));
            if (roll >= chance) continue;

            // Find free slot
            uint32_t slot = UINT32_MAX;
            for (uint32_t i = 0; i < Dim::MAX_GOL_ZONES; i++) {
                if (!gs.zones[i].active) { slot = i; break; }
            }
            if (slot == UINT32_MAX) continue;

            // Reserve slot
            gs.zones[slot].active = true;

            // Zone corner (cell-grid-snapped)
            float corner_x = std::floor(
                (raw_cx - GoLZoneSpawnConfig::ZONE_EXTENT * 0.5f) / PATCH_CELL_SIZE) * PATCH_CELL_SIZE;
            float corner_z = std::floor(
                (raw_cz - GoLZoneSpawnConfig::ZONE_EXTENT * 0.5f) / PATCH_CELL_SIZE) * PATCH_CELL_SIZE;

            // Algorithm selection
            float algo_roll = cpu_hash_f(seed, PulseZoneProp::ALGORITHM_ROLL);
            uint32_t algorithm = (algo_roll < PULSE_ALGORITHM_CHANCE)
                ? AlgorithmType::PULSE : AlgorithmType::CONWAY;

            // Height enabled
            float height_roll = cpu_hash_f(seed, GoLZoneProp::HEIGHT_ROLL);
            bool height_enabled = (height_roll < GoLZoneSpawnConfig::HEIGHT_CHANCE);

            // Tier + CPU-side params
            float tick_period = 1.0f;
            float initial_density = 0.0f;
            uint32_t tier_idx = 0;

            if (algorithm == AlgorithmType::CONWAY) {
                float tier_roll = cpu_hash_f(seed, GoLZoneProp::TIER);
                uint32_t tier = GOL_TIER_COUNT - 1;
                float cumul = 0.0f;
                for (uint32_t t = 0; t < GOL_TIER_COUNT; t++) {
                    cumul += GOL_TIERS[t].weight;
                    if (tier_roll < cumul) { tier = t; break; }
                }
                const auto& tp = GOL_TIERS[tier];
                if (tp.force_no_height) height_enabled = false;
                tick_period = std::max(0.1f,
                    cpu_sample_gaussian(seed, GoLZoneProp::TICK_PERIOD,
                        tp.tick_period_mean, tp.tick_period_sigma));
                initial_density = std::max(0.05f, std::min(0.9f,
                    cpu_sample_gaussian(seed, GoLZoneProp::DENSITY,
                        tp.density_mean, tp.density_sigma)));
                tier_idx = tier;  // Conway: 0–6
            }
            else {
                float tier_roll = cpu_hash_f(seed, PulseZoneProp::PULSE_TIER);
                uint32_t tier = PULSE_TIER_COUNT - 1;
                float cumul = 0.0f;
                for (uint32_t t = 0; t < PULSE_TIER_COUNT; t++) {
                    cumul += PULSE_TIERS[t].weight;
                    if (tier_roll < cumul) { tier = t; break; }
                }
                const auto& pp = PULSE_TIERS[tier];
                if (pp.force_no_height) height_enabled = false;
                tick_period = std::max(0.1f,
                    cpu_sample_gaussian(seed, GoLZoneProp::TICK_PERIOD,
                        pp.tick_period_mean, pp.tick_period_sigma));
                initial_density = 0.0f;
                tier_idx = GOL_TIER_COUNT + tier;  // Pulse: 7–9 (compound index)
            }

            // Fill selection
            sel.seed = seed;
            sel.trigger_gx = gx;
            sel.trigger_gz = gz;
            sel.slot = slot;
            sel.zone_nx = nx;
            sel.zone_nz = nz;
            sel.corner_x = corner_x;
            sel.corner_z = corner_z;
            sel.algorithm = algorithm;
            sel.tier_idx = tier_idx;
            sel.tick_period = tick_period;
            sel.initial_density = initial_density;
            sel.height_enabled = height_enabled;
            sel.footprint_r = GoLZoneSpawnConfig::FOOTPRINT_RADIUS;

            return true;  // at most one zone per patch
        }
    }
    return false;
}

// ─── place_gol_from_selection ─────────────────────────────────
//
// Phase 2: footprint check + registration. Position is lattice-
// determined (no jitter), so we bypass negotiate_position and
// call check_position + register_footprint directly.
//
// Note: this function takes no GoLState — like ribbon's place
// function, it only mediates between selection and spawn-engine
// helpers. Doesn't touch GoL's data.

static bool place_gol_from_selection(Cartridge* c,
    const GoLSelection& sel, GoLPlacement& plan) {
    float cx = sel.corner_x + GoLZoneSpawnConfig::ZONE_EXTENT * 0.5f;
    float cz = sel.corner_z + GoLZoneSpawnConfig::ZONE_EXTENT * 0.5f;

    if (!c->check_position(cx, cz, sel.footprint_r, PopFamily::GOL))
        return false;

    int32_t host_gx = (int32_t)std::floor(cx / PATCH_EXTENT);
    int32_t host_gz = (int32_t)std::floor(cz / PATCH_EXTENT);

    if (c->register_footprint(cx, cz, sel.footprint_r,
        host_gx, host_gz, PopFamily::GOL, sel.tier_idx) == UINT32_MAX)
        return false;

    plan = GoLPlacement{};
    plan.slot = sel.slot;
    plan.trigger_gx = sel.trigger_gx;
    plan.trigger_gz = sel.trigger_gz;
    plan.host_gx = host_gx;
    plan.host_gz = host_gz;
    plan.tier_idx = sel.tier_idx;
    plan.cx = cx;
    plan.cz = cz;
    plan.zone_nx = sel.zone_nx;
    plan.zone_nz = sel.zone_nz;
    plan.corner_x = sel.corner_x;
    plan.corner_z = sel.corner_z;
    plan.algorithm = sel.algorithm;
    plan.tick_period = sel.tick_period;
    plan.initial_density = sel.initial_density;
    plan.height_enabled = sel.height_enabled;

    c->record_placement_bookkeeping(PopFamily::GOL, plan.tier_idx);
    return true;
}

// ─── commit_gol ──────────────────────────────────────────────
//
// Phase 3: CPU state + life buffer seeding + GPU derive request.

static void commit_gol(GoLState& gs, Cartridge* c,
    const GoLPlacement& plan,
    int32_t trigger_gx, int32_t trigger_gz, wgpu::Queue& queue)
{
    auto& zone = gs.zones[plan.slot];
    zone.zone_nx = plan.zone_nx;
    zone.zone_nz = plan.zone_nz;
    zone.host_gx = plan.host_gx;
    zone.host_gz = plan.host_gz;
    zone.active = true;
    zone.algorithm = plan.algorithm;
    zone.tick_period = plan.tick_period;
    zone.initial_density = plan.initial_density;
    zone.last_tick_index = -1;
    gs.zone_count++;

    seed_gol_zone(gs, c, plan.slot, queue);

    if (gs.pending_derive_requests.count < Dim::MAX_GOL_ZONES) {
        auto& req = gs.pending_derive_requests.requests[gs.pending_derive_requests.count++];
        req.slot = plan.slot;
        req.nx = plan.zone_nx;
        req.nz = plan.zone_nz;
        req.algorithm = plan.algorithm;
        req.height_enabled = plan.height_enabled ? 1u : 0u;
        req.world_seed = c->world_state_.active_seed;
    }

    std::cout << "[GoL] "
        << (plan.algorithm == AlgorithmType::PULSE ? "Pulse" : "Conway")
        << " slot=" << plan.slot
        << " node=(" << plan.zone_nx << "," << plan.zone_nz << ")"
        << " corner=(" << plan.corner_x << "," << plan.corner_z << ")"
        << " host=(" << plan.host_gx << "," << plan.host_gz << ")"
        << (plan.height_enabled ? " HEIGHT" : "")
        << " period=" << plan.tick_period
        << "\n";
}

// ─── seed_gol_zone ───────────────────────────────────────────
//
// Helper called from commit_gol. Generates the initial life pattern
// (random alive/dead for Conway, all zeros for Pulse) and the per-cell
// height factors (Gaussian draws). Uploads both to the GPU zone slot.

static void seed_gol_zone(GoLState& gs, Cartridge* c,
    uint32_t slot, wgpu::Queue& queue) {
    auto& zone = gs.zones[slot];
    uint32_t seed = cpu_lattice_node_seed(c->world_state_.active_seed, zone.zone_nx, zone.zone_nz, GoLZoneProp::SEED_BAND);

    // Generate initial pattern
    std::vector<float> life(Dim::GOL_ZONE_CELLS, 0.0f);
    if (zone.algorithm == AlgorithmType::CONWAY) {
        // Conway: random alive/dead from density
        for (uint32_t i = 0; i < Dim::GOL_ZONE_CELLS; i++) {
            float roll = cpu_hash_f(seed + i, GoLZoneProp::DENSITY);
            life[i] = (roll < zone.initial_density) ? 1.0f : 0.0f;
        }
    }
    // Pulse: all zeros — oscillation builds from silence

    // Generate per-cell height factors: Gaussian draw, clamped
    std::vector<float> height_factors(Dim::GOL_ZONE_CELLS);
    for (uint32_t i = 0; i < Dim::GOL_ZONE_CELLS; i++) {
        float hf = cpu_sample_gaussian(seed + i, GoLZoneProp::HEIGHT_FACTOR,
            GoLZoneSpawnConfig::HEIGHT_FACTOR_MEAN, GoLZoneSpawnConfig::HEIGHT_FACTOR_SIGMA);
        height_factors[i] = std::max(GoLZoneSpawnConfig::HEIGHT_FACTOR_CLAMP_LO,
            std::min(GoLZoneSpawnConfig::HEIGHT_FACTOR_CLAMP_HI, hf));
    }

    // Upload all 7 slots
    c->gpuState_.upload_zone_life(queue, slot, life.data(), height_factors.data(), Dim::GOL_ZONE_CELLS);
}


// ═══ PER-FRAME UPLOAD ════════════════════════════════════════════

// upload_gol_zone_config: per-frame header-only upload.
// Per-zone config is GPU-derived via zone_derive_params — we only
// write count, t_beats, dt, tick_mask. Slot deactivation happens
// at eviction time (via dispatch_evict_gol through entity_refs).
static void upload_gol_zone_config(GoLState& gs, Cartridge* c, wgpu::Queue& queue) {
    uint32_t count = 0;
    uint32_t tick_mask = 0;

    // GoL tempo mode: scale tick period (> 1 = slower, inverse of polyphony).
    // GAIN constant lives in musical.inl alongside the coupling that drives it.
    float gol_tick_scale = 1.0f + c->player_.mmode_intensities[MMODE_GOL_TEMPO] * GOL_TICK_GAIN;

    for (uint32_t i = 0; i < Dim::MAX_GOL_ZONES; i++) {
        if (!gs.zones[i].active) continue;

        // Conway tick gating: exactly one tick per period
        // Scale period by musical mode (affects both Conway and Pulse in sync)
        float effective_period = std::max(gs.zones[i].tick_period * gol_tick_scale, 0.01f);
        int32_t current_tick = (int32_t)std::floor(c->time_state_.beats / effective_period);
        if (current_tick != gs.zones[i].last_tick_index) {
            tick_mask |= (1u << i);
            gs.zones[i].last_tick_index = current_tick;
        }

        count = i + 1;
    }
    c->gpuState_.upload_zone_config_header(queue, count, c->time_state_.beats, c->time_state_.dt, tick_mask);
    gs.active_slot_count = count;
}

// Flush pending zone derive requests as a GPU compute dispatch.
// Called once per frame after all patch generation is complete.
static void flush_zone_derive_requests(GoLState& gs, Cartridge* c, wgpu::Queue& queue) {
    if (gs.pending_derive_requests.count == 0) return;

    c->gpuState_.upload_zone_derive_requests(queue, gs.pending_derive_requests);

    wgpu::CommandEncoder encoder = c->device_.CreateCommandEncoder();
    wgpu::ComputePassDescriptor desc{};
    desc.label = "Zone Derive Params";
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&desc);
    c->renderer_.dispatch_zone_derive_params(
        pass,
        c->gpuState_.zone_gol_compute_group(),
        gs.pending_derive_requests.count);
    pass.End();
    wgpu::CommandBuffer cmd = encoder.Finish();
    queue.Submit(1, &cmd);

    gs.pending_derive_requests.count = 0;
}
