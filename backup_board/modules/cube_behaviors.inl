// ─── cube_behaviors.inl ─────────────────────────────────────────
//
// Cube behavior system. Spheres do their own thing (analytical PGA
// orbit, no behavior layer); this module is cube-only by design.
// Floater vocabulary (Sphere + Cube tier counts, configs, prop
// registries, active tracking) lives in floater_vocabulary.inl;
// behavior gains (forces, coordination, kite, corral) live here.
//
// ┌─── Three registries ────────────────────────────────────────────┐
// │                                                                  │
// │  CUBE_BEHAVIORS     id + name table; force functions live in     │
// │                     world.wgsl as cube_force_<n> + a switch      │
// │                     case in cube_behavior_force                  │
// │  CUBE_TIER_GAINS    per-tier multipliers on spring/drag/forces;  │
// │                     applied at spawn so each tier gets its own   │
// │                     baked-in dynamics signature                  │
// │  CUBE_POPULATIONS   per-mood weights across behaviors; consulted │
// │                     at spawn to assign behavior_id stochastically│
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// ┌─── The four-axis vocabulary (from the design doc) ──────────────┐
// │                                                                  │
// │  LATTICE     where cubes can be (anchor xz, pawn-relative)       │
// │  COUPLING    how cubes relate to each other (shared field        │
// │              samples vs independent, shared phase vs random)     │
// │  TRAJECTORY  what each cube does over time (the behaviors)       │
// │  TIER        which "weight class" the cube belongs to            │
// │                                                                  │
// │ Coordination ∈ [0,1] is the system-wide knob that slides each    │
// │ behavior between high-individual and high-coupled regimes.       │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// ┌─── Public surface (called from outside this file) ──────────────┐
// │                                                                  │
// │  Module functions (other than the two pure spawn helpers below)  │
// │  are static, take CubeBehaviorsState& explicitly. Spawn helpers  │
// │  are stateless and stay free.                                    │
// │                                                                  │
// │  Spawn-side (stateless, no struct ref needed):                   │
// │    pick_cube_behavior_for_spawn(mood, seed)  → behavior_id u32   │
// │    apply_cube_tier_gains(spring, drag, tier) → adjusted (sp, dr) │
// │      Both consumed by entity_pipeline.inl's cube_write_gpu.      │
// │                                                                  │
// │  Per-frame:                                                      │
// │    tick_cube_corral_animations(cbs, c, queue)                    │
// │      Called from update(); advances any in-flight glides.        │
// │                                                                  │
// │  Player commands (function keys; chosen to avoid the A-Z piano   │
// │                   range the analysis layer consumes):            │
// │    cycle_cube_behavior_override(cbs, c, q)  F4  next behavior    │
// │    cycle_floater_coordination(cbs, c)       F5  step coordination│
// │    corral_cubes(cbs, c, q)                  F6  ring around pawn │
// │    toggle_cube_kite_mode(cbs, c, q)         F7  follow pawn      │
// │                                                                  │
// │  Cross-module reads (this module reads, doesn't own):            │
// │    activeCubes_[] (floater_vocabulary), agent_state_.slots,      │
// │    player_.possessed_slot                                        │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// Included inside the Cartridge class body. Depends on:
//   state.hpp           GPUFloatingEntityState fields,
//                       GPUDesignConfig::floater_coordination
//   entity_pipeline.inl cube_write_gpu calls into pick_cube_behavior_for_spawn
//                       and apply_cube_tier_gains
//   world.wgsl          force functions and dispatch switch
// ──────────────────────────────────────────────────────────────────


// ═══ BEHAVIOR IDS ════════════════════════════════════════════════
//
// Mirror the WGSL constants in world.wgsl (search "Cube behavior
// registry"). Adding a behavior is four steps:
//   1) Add an id constant here and bump CUBE_BEHAVIOR_COUNT
//   2) Add the name in CUBE_BEHAVIOR_NAMES
//   3) Add a const + force function + switch case in world.wgsl
//   4) Populate CUBE_POPULATIONS rows with a non-zero weight where
//      the behavior should appear

static constexpr uint32_t CUBE_BEHAVIOR_STATIONARY = 0;
static constexpr uint32_t CUBE_BEHAVIOR_CURLFIELD  = 1;
static constexpr uint32_t CUBE_BEHAVIOR_PHASEWAVE  = 2;
// DONE[floaters:L4] CUBE_BEHAVIOR_COUNT_WGSL added on the WGSL side
//   (world.wgsl §7 cube behavior registry). MUST match this value —
//   when adding a behavior, bump both. Mirrors the agents pattern
//   (AGENT_BEHAVIOR_COUNT_WGSL).
static constexpr uint32_t CUBE_BEHAVIOR_COUNT      = 3;

static constexpr const char* CUBE_BEHAVIOR_NAMES[CUBE_BEHAVIOR_COUNT] = {
    "stationary", "curlfield", "phasewave"
};


// ═══ TUNING CONSOLE ══════════════════════════════════════════════
//
// One place for every system-level number that shapes the cube
// system's feel. WGSL kernel constants (force amplitudes, noise
// frequencies, wave wavelengths) cannot be moved here — they're
// shader literals — but they're documented below as the source of
// truth for tuning. If you ever want CPU-side adjustability, promote
// them to GPUDesignConfig fields and read them from the kernel.

// ─ Substrate (drift integrator) ─────────────────────────────────
// Cube tuning constants live here so the file is the one place to
// look when adjusting cube dynamics. Each spawn multiplies these by
// the row's tier gain (see CUBE_TIER_GAINS) so different tiers get
// distinct dynamics signatures from the same defaults.

static constexpr float CUBE_DEFAULT_SPRING_STIFFNESS = 4.0f;   // 1/s², ~0.5s settle
static constexpr float CUBE_DEFAULT_DRAG             = 1.5f;   // 1/s,  gentle damping

// ─ Diagnostics (corral) ─────────────────────────────────────────
// Used by F6 to glide every active cube into a ring around the pawn.
// Kite mode (F7) doesn't change the radius/duration — it just changes
// whether the ring is in world or pawn-relative space.

static constexpr float CUBE_CORRAL_RADIUS   = 120.0f;  // ring radius around pawn (world units)
static constexpr float CUBE_CORRAL_DURATION = 4.0f;    // glide duration (seconds)

// ─ Diagnostics (coordination cycle) ─────────────────────────────
// F5 steps the system-wide coordination knob through these values.
// floater_coordination is uploaded to GPUDesignConfig each frame.

static constexpr float FLOATER_COORDINATION_STEPS[3] = { 0.0f, 0.5f, 1.0f };

// ─ WGSL kernel constants (NOT here, but documented) ─────────────
//
//   cube_force_curlfield (world.wgsl):
//     freq_high   = 0.040    high spatial frequency at coordination=0
//     freq_low    = 0.005    low  spatial frequency at coordination=1
//     amplitude   = 12.0     force magnitude (≈ 10× spring)
//     time_scale  = 0.25     1/s, evolution rate
//
//   cube_force_phasewave (world.wgsl):
//     k_x         = 0.020    wavefront freq in X
//     k_z         = 0.012    wavefront freq in Z (asymmetric)
//     omega       = 1.5      1/s, temporal frequency
//     amplitude   = 30.0     force magnitude (vertical)
//
// To change these, edit world.wgsl. Promote to config if you need
// CPU-side adjustability (~16 bytes of uniform space + 4 lines).


// ═══ REGISTRY: TIER GAINS ════════════════════════════════════════
//
// Per-tier multipliers applied at spawn time. The cube's stored
// spring_stiffness and drag are CUBE_DEFAULT_* × tier gain, so each
// tier gets a baked-in dynamics signature without runtime branching.
//
// behavior_amp_mult is reserved for future use — the kernel applies
// forces uniformly today. To wire it up, kernels would need to read
// tier_idx and multiply force outputs. Defer until a behavior demands
// per-tier amplitude differentiation.
//
// Defaults are all 1.0 (no per-tier differentiation). Character pass
// will populate with real values.

struct CubeTierGain {
    uint32_t tier_idx;
    const char* name;
    float spring_stiffness_mult;
    float drag_mult;
    float behavior_amp_mult;   // reserved; not yet consumed by kernel
};

static constexpr CubeTierGain CUBE_TIER_GAINS[CUBE_TIER_COUNT] = {
    //                            spring  drag   amp
    /* 0 SmallCube */ { 0, "SmallCube", 1.0f, 1.0f, 1.0f },
    /* 1 MedCube   */ { 1, "MedCube",   1.0f, 1.0f, 1.0f },
    /* 2 LargeCube */ { 2, "LargeCube", 1.0f, 1.0f, 1.0f },
    /* 3 Monolith  */ { 3, "Monolith",  1.0f, 1.0f, 1.0f },
};

static_assert(sizeof(CUBE_TIER_GAINS) / sizeof(CUBE_TIER_GAINS[0]) == CUBE_TIER_COUNT,
              "CUBE_TIER_GAINS must declare one row per cube tier");

// Apply tier gains to base substrate values. Called by cube_write_gpu
// during spawn — the result is what gets stored on the cube.
static void apply_cube_tier_gains(float& spring_stiffness, float& drag, uint32_t tier_idx) {
    if (tier_idx >= CUBE_TIER_COUNT) return;  // defensive; tier_idx is bounded at select
    const auto& g = CUBE_TIER_GAINS[tier_idx];
    spring_stiffness *= g.spring_stiffness_mult;
    drag             *= g.drag_mult;
}


// ═══ REGISTRY: POPULATIONS ═══════════════════════════════════════
//
// Per-mood weights across the three behaviors. Each spawn picks a
// behavior stochastically by sampling these weights. Weights need not
// sum to 1; the picker normalizes.
//
// Mood ordering matches MOOD_TABLE in cartridge.hpp. Cubes are gated
// by CubeConfig::MOOD_MULTIPLIER (in floater_vocabulary.inl) which is
// {1, 1, 0, 0, 1, 0} — cubes don't spawn in indoor moods or in
// MOOD_FINITE_OUTDOOR_REF, so those rows here are never consulted in
// practice. We declare them anyway for hygiene; if the spawn gate
// ever changes, the populations will already exist.
//
// Defaults are all-Stationary. Character pass will populate with
// real per-mood character.

struct CubePopulationDef {
    uint32_t mood_id;
    std::array<float, CUBE_BEHAVIOR_COUNT> behavior_weights;
};

static constexpr CubePopulationDef CUBE_POPULATIONS[MOOD_COUNT] = {
    /* MOOD_OPEN_DEFAULT */
    { MOOD_OPEN_DEFAULT,
      //                    stat curl  wave
      /*behavior_weights=*/ { 1.0f, 0.0f, 0.0f } },
    /* MOOD_OPEN_SUNSET */
    { MOOD_OPEN_SUNSET,
      /*behavior_weights=*/ { 1.0f, 0.0f, 0.0f } },
    /* MOOD_INDOOR_FLAT — cubes don't spawn here; row exists for hygiene */
    { MOOD_INDOOR_FLAT,
      /*behavior_weights=*/ { 1.0f, 0.0f, 0.0f } },
    /* MOOD_INDOOR_VAULT — cubes don't spawn here; row exists for hygiene */
    { MOOD_INDOOR_VAULT,
      /*behavior_weights=*/ { 1.0f, 0.0f, 0.0f } },
    /* MOOD_FINITE_OUTDOOR */
    { MOOD_FINITE_OUTDOOR,
      /*behavior_weights=*/ { 1.0f, 0.0f, 0.0f } },
    /* MOOD_FINITE_OUTDOOR_REF — cubes don't spawn here; row exists for hygiene */
    { MOOD_FINITE_OUTDOOR_REF,
      /*behavior_weights=*/ { 1.0f, 0.0f, 0.0f } },
};

static_assert(sizeof(CUBE_POPULATIONS) / sizeof(CUBE_POPULATIONS[0]) == MOOD_COUNT,
              "CUBE_POPULATIONS must declare one row per mood");
static_assert(CUBE_POPULATIONS[0].mood_id == MOOD_OPEN_DEFAULT,        "row 0 must be MOOD_OPEN_DEFAULT");
static_assert(CUBE_POPULATIONS[1].mood_id == MOOD_OPEN_SUNSET,         "row 1 must be MOOD_OPEN_SUNSET");
static_assert(CUBE_POPULATIONS[2].mood_id == MOOD_INDOOR_FLAT,         "row 2 must be MOOD_INDOOR_FLAT");
static_assert(CUBE_POPULATIONS[3].mood_id == MOOD_INDOOR_VAULT,        "row 3 must be MOOD_INDOOR_VAULT");
static_assert(CUBE_POPULATIONS[4].mood_id == MOOD_FINITE_OUTDOOR,      "row 4 must be MOOD_FINITE_OUTDOOR");
static_assert(CUBE_POPULATIONS[5].mood_id == MOOD_FINITE_OUTDOOR_REF,  "row 5 must be MOOD_FINITE_OUTDOOR_REF");

// Stochastic behavior pick. seed is the cube's spawn seed (used as
// the entity_seed in the GPU struct), so behavior assignments are
// deterministic across runs with the same world seed. mood_id is
// the active mood at spawn time. Returns one of CUBE_BEHAVIOR_*.
//
// Mix constant 0xBEEF11A0u is unrelated to behavior_phase's mix
// constant (0xF10A7E70u in entity_pipeline.inl) so the two derived
// values are decorrelated.
static uint32_t pick_cube_behavior_for_spawn(uint32_t mood_id, uint32_t seed) {
    if (mood_id >= MOOD_COUNT) return CUBE_BEHAVIOR_STATIONARY;
    const auto& pop = CUBE_POPULATIONS[mood_id];

    float total = 0.0f;
    for (uint32_t i = 0; i < CUBE_BEHAVIOR_COUNT; i++) total += pop.behavior_weights[i];
    if (total <= 0.0f) return CUBE_BEHAVIOR_STATIONARY;

    uint32_t h = cpu_hash(seed, 0xBEEF11A0u);
    float r = (float(h) / 4294967296.0f) * total;

    float acc = 0.0f;
    for (uint32_t i = 0; i < CUBE_BEHAVIOR_COUNT; i++) {
        acc += pop.behavior_weights[i];
        if (r < acc) return i;
    }
    // Numerical edge case (r == total exactly): return last non-zero.
    for (uint32_t i = CUBE_BEHAVIOR_COUNT; i > 0; i--) {
        if (pop.behavior_weights[i - 1] > 0.0f) return i - 1;
    }
    return CUBE_BEHAVIOR_STATIONARY;  // unreachable; total > 0 was checked
}


// ═══ DIAGNOSTICS ═════════════════════════════════════════════════
//
// Inspection tools, not part of the system's primary expression.
// Keypress-driven; the system runs without them. Anything below
// this point can be commented out and the cubes still spawn,
// populate, and behave correctly per the registries above.


// ─── Coordination cycle ─────────────────────────────────────────────
//
// F5 steps coordination through 0.0 / 0.5 / 1.0 for quick visual
// inspection. Music-driven smooth values are a future concern.
//
// ─── Behavior override ──────────────────────────────────────────────
//
// F4 walks every active cube slot and rewrites behavior_id. No
// "OVERRIDE_NONE" sentinel because the natural state is already
// "whatever the population assigned at spawn"; the cycle just steps
// 0 → 1 → 2 → 0. Useful for visually verifying each behavior's
// look without waiting for a population to favor it.
//
// ─── Corral animation ──────────────────────────────────────────────
//
// F6 smoothly gathers every active cube into a small ring around
// the pawn. Animates anchor (anchor mode) or pawn_offset (kite mode)
// over CUBE_CORRAL_DURATION using smooth-step easing.
//
// Re-pressing mid-glide captures the current interpolated value as
// the new from-position so the formation doesn't snap.
//
// ─── Kite mode ─────────────────────────────────────────────────────
//
// F7 toggles cubes between anchor-relative (default) and pawn-
// relative (kite) anchoring. In kite mode the kernel computes home
// from pawn.pos + pawn_offset, so cubes follow the pawn around the
// world like kites on invisible strings.
//
// Toggle on  — captures each cube's current xz as a pawn-relative
// offset. Cube xz is preserved exactly; y resets to pawn.y +
// orbit_height (small visible jump only if pawn's altitude differs
// from where the cube was hovering).
//
// Toggle off — anchor freezes at the cube's current world xz.
// pawn_offset tracks the per-cube offset so we reconstruct
// world position correctly: cube_xz = pawn.xz + offset.xz.

struct CubeCorralAnim {
    bool   active;
    float  t0;
    float  duration;
    float  ax_from, az_from;
    float  ax_to,   az_to;
};

// ═══ CUBE BEHAVIORS MODULE STATE (Scope B migration #4) ══════════
//
// All cube-behavior-owned state lives in this struct, accessed via
// cube_behaviors_state_ on the Cartridge. Module functions take
// `CubeBehaviorsState& cbs` explicitly rather than reaching via
// Cartridge*, making ownership language-visible and dependencies
// explicit in signatures.

struct CubeBehaviorsState {
    uint32_t       coordination_step  = 0;
    uint32_t       behavior_override  = CUBE_BEHAVIOR_STATIONARY;
    CubeCorralAnim corral_anim[Dim::MAX_CUBE_INSTANCES] = {};
    bool           kite_mode          = false;
    float          pawn_offset[Dim::MAX_CUBE_INSTANCES][2] = {};  // xz only
};
CubeBehaviorsState cube_behaviors_state_;

static float corral_ease_(float t) {
    return t * t * (3.0f - 2.0f * t);  // smoothstep
}

static void cycle_floater_coordination(CubeBehaviorsState& cbs, Cartridge* c) {
    cbs.coordination_step = (cbs.coordination_step + 1) % 3;
    float v = FLOATER_COORDINATION_STEPS[cbs.coordination_step];
    c->gpuState_.config().floater_coordination = v;
    std::cout << "[Floaters] coordination: " << v << "\n";
}


static void apply_cube_behavior_override(CubeBehaviorsState& cbs, Cartridge* c, wgpu::Queue& queue) {
    for (uint32_t i = 0; i < Dim::MAX_CUBE_INSTANCES; i++) {
        if (!c->activeCubes_[i].active) continue;
        c->gpuState_.upload_cube_behavior_id(queue, i, cbs.behavior_override);
    }
}

static void cycle_cube_behavior_override(CubeBehaviorsState& cbs, Cartridge* c, wgpu::Queue& queue) {
    cbs.behavior_override = (cbs.behavior_override + 1) % CUBE_BEHAVIOR_COUNT;
    apply_cube_behavior_override(cbs, c, queue);
    std::cout << "[Floaters] cube behavior: "
              << CUBE_BEHAVIOR_NAMES[cbs.behavior_override] << "\n";
}


static void corral_cubes(CubeBehaviorsState& cbs, Cartridge* c, wgpu::Queue& queue) {
    // DONE[floaters:L1] read the player's pos from the possessed slot
    //   (which is 0 by default but tracks the player on possession
    //   transfer), not from slot 0 directly. Old hardcode was visible
    //   under F6/F7 after Caps Lock possession.
    const float px = c->agent_state_.slots[c->player_.possessed_slot].pos_x;
    const float pz = c->agent_state_.slots[c->player_.possessed_slot].pos_z;

    uint32_t active_count = 0;
    for (uint32_t i = 0; i < Dim::MAX_CUBE_INSTANCES; i++) {
        if (c->activeCubes_[i].active) active_count++;
    }
    if (active_count == 0) {
        std::cout << "[Floaters] corral: no active cubes\n";
        return;
    }

    // Distribute by stable index so successive presses don't shuffle
    // the layout. In kite mode the ring is in pawn-relative offset
    // space; otherwise world space.
    uint32_t armed = 0;
    uint32_t k = 0;
    const float two_pi = 6.28318530718f;
    for (uint32_t i = 0; i < Dim::MAX_CUBE_INSTANCES; i++) {
        if (!c->activeCubes_[i].active) continue;
        float theta = (float(k) / float(active_count)) * two_pi;
        float target_x, target_z;
        if (cbs.kite_mode) {
            target_x = std::cos(theta) * CUBE_CORRAL_RADIUS;
            target_z = std::sin(theta) * CUBE_CORRAL_RADIUS;
        } else {
            target_x = px + std::cos(theta) * CUBE_CORRAL_RADIUS;
            target_z = pz + std::sin(theta) * CUBE_CORRAL_RADIUS;
        }

        // Start position: current interpolated value if a previous
        // animation is in flight, else the CPU mirror.
        float from_x, from_z;
        if (cbs.corral_anim[i].active) {
            float t_norm = (c->time_state_.seconds - cbs.corral_anim[i].t0)
                           / cbs.corral_anim[i].duration;
            t_norm = std::clamp(t_norm, 0.0f, 1.0f);
            float eased = corral_ease_(t_norm);
            from_x = cbs.corral_anim[i].ax_from
                    + (cbs.corral_anim[i].ax_to - cbs.corral_anim[i].ax_from) * eased;
            from_z = cbs.corral_anim[i].az_from
                    + (cbs.corral_anim[i].az_to - cbs.corral_anim[i].az_from) * eased;
        } else if (cbs.kite_mode) {
            from_x = cbs.pawn_offset[i][0];
            from_z = cbs.pawn_offset[i][1];
        } else {
            from_x = c->activeCubes_[i].cx;
            from_z = c->activeCubes_[i].cz;
        }

        cbs.corral_anim[i] = {
            /*active=*/   true,
            /*t0=*/       c->time_state_.seconds,
            /*duration=*/ CUBE_CORRAL_DURATION,
            from_x, from_z,
            target_x, target_z,
        };
        if (cbs.kite_mode) {
            cbs.pawn_offset[i][0] = target_x;
            cbs.pawn_offset[i][1] = target_z;
        } else {
            c->activeCubes_[i].cx = target_x;
            c->activeCubes_[i].cz = target_z;
        }
        armed++;
        k++;
    }

    std::cout << "[Floaters] corral: " << armed << " cube(s) gliding "
              << (cbs.kite_mode ? "to ring offset around pawn" : "to ring")
              << " radius " << CUBE_CORRAL_RADIUS
              << " over " << CUBE_CORRAL_DURATION << "s\n";
}

// Per-frame: advance any active animations and push interpolated
// position to GPU. Skips cleanly when nothing is animating. Called
// from the cartridge's update() path.
static void tick_cube_corral_animations(CubeBehaviorsState& cbs, Cartridge* c, wgpu::Queue& queue) {
    for (uint32_t i = 0; i < Dim::MAX_CUBE_INSTANCES; i++) {
        auto& anim = cbs.corral_anim[i];
        if (!anim.active) continue;
        if (!c->activeCubes_[i].active) { anim.active = false; continue; }

        float t_norm = (c->time_state_.seconds - anim.t0) / anim.duration;
        bool finishing = false;
        if (t_norm >= 1.0f) { t_norm = 1.0f; finishing = true; }
        if (t_norm < 0.0f) t_norm = 0.0f;
        float eased = corral_ease_(t_norm);
        float ax = anim.ax_from + (anim.ax_to - anim.ax_from) * eased;
        float az = anim.az_from + (anim.az_to - anim.az_from) * eased;
        if (cbs.kite_mode) {
            c->gpuState_.upload_cube_pawn_offset(queue, i, ax, 0.0f, az);
        } else {
            c->gpuState_.upload_cube_anchor(queue, i, ax, 0.0f, az);
        }
        if (finishing) anim.active = false;
    }
}


// ─── Kite mode toggle (F7) ──────────────────────────────────────

static void toggle_cube_kite_mode(CubeBehaviorsState& cbs, Cartridge* c, wgpu::Queue& queue) {
    cbs.kite_mode = !cbs.kite_mode;
    // DONE[floaters:L1] possessed_slot for kite mode too — see corral_cubes.
    const float px = c->agent_state_.slots[c->player_.possessed_slot].pos_x;
    const float pz = c->agent_state_.slots[c->player_.possessed_slot].pos_z;

    uint32_t affected = 0;
    if (cbs.kite_mode) {
        for (uint32_t i = 0; i < Dim::MAX_CUBE_INSTANCES; i++) {
            if (!c->activeCubes_[i].active) continue;
            float ox = c->activeCubes_[i].cx - px;
            float oz = c->activeCubes_[i].cz - pz;
            cbs.pawn_offset[i][0] = ox;
            cbs.pawn_offset[i][1] = oz;
            c->gpuState_.upload_cube_pawn_offset(queue, i, ox, 0.0f, oz);
            c->gpuState_.upload_cube_follow_pawn(queue, i, 1u);
            affected++;
        }
    } else {
        // Toggle OFF: rather than write anchor on CPU (which can't see
        // the cube's drift, so anchor would land at home rather than
        // at the cube's visible position), we hand the work to the
        // kernel. follow_pawn = 2u is a "release pending" sentinel:
        // the next update_cube run sees it, sets anchor = current pos,
        // zeroes drift, switches follow_pawn back to 0. Visible
        // position is preserved exactly — no CPU drift estimation
        // needed, no readback latency.
        for (uint32_t i = 0; i < Dim::MAX_CUBE_INSTANCES; i++) {
            if (!c->activeCubes_[i].active) continue;
            // Update CPU mirror cx/cz to a best-effort estimate so a
            // subsequent corral or kite-on doesn't start from a stale
            // value. The kernel's actual anchor write next frame may
            // differ by a few units of drift, but for diagnostic
            // anchoring this is close enough.
            float ax = px + cbs.pawn_offset[i][0];
            float az = pz + cbs.pawn_offset[i][1];
            c->activeCubes_[i].cx = ax;
            c->activeCubes_[i].cz = az;
            c->gpuState_.upload_cube_follow_pawn(queue, i, 2u);
            affected++;
        }
    }

    std::cout << "[Floaters] kite mode: " << (cbs.kite_mode ? "ON" : "OFF")
              << " (" << affected << " cube(s))\n";
}
