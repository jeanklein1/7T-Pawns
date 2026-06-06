// ─── musical.inl ─────────────────────────────────────────────────
//
// The coupling layer: connects analysis signal to visual parameters.
// Mode definitions, intensity trajectories, band motion, palette
// drift, radial pulse, per-frame update, teardown reset.
//
// ┌─── Public surface (called from outside this file) ──────────────┐
// │                                                                  │
// │  Lifecycle:                                                      │
// │    tick_musical_couplings(signal, queue)  — every frame          │
// │    reset_musical_couplings(queue)         — mood transitions     │
// │                                                                  │
// │  Module functions are static, take MusicalState& explicitly      │
// │  (or const MusicalState& when read-only).                        │
// │                                                                  │
// │  Per-frame:                                                      │
// │    tick_musical_couplings(ms, c, signal, queue)  — coupling tick │
// │    reset_musical_couplings(ms, c, queue)         — mood reset    │
// │                                                                  │
// │  Player commands (wired in input.inl):                           │
// │    toggle_mmode(ms, c, mode)              — numpad 1–7           │
// │                                                                  │
// │  Queries:                                                        │
// │    is_mmode_on(ms, mode)                  — mood-gated read      │
// │                                                                  │
// │  Cross-module reads (consumed by other modules):                 │
// │    player_.mmode_intensities[GOL_TEMPO]   — gol_zones.inl   │
// │    player_.mmode_intensities[AURA_EXPAND] — pawn.inl, spine │
// │    is_mmode_on(musical_state_, ...)            — mood.inl        │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// Included inside the Cartridge class body.
// Depends on: musical/trajectory.hpp (release primitive), pawn.inl
//             (pawn_state_.aura_cfg_dirty flag set when aura-coupling changes).
// ─────────────────────────────────────────────────────────────────


// ═══ TUNING CONSOLE ══════════════════════════════════════════════
//
// System-level dials for the musical coupling layer. Per-mood
// values (which modes are gated on/off in each mood) live in
// MOOD_TABLE (cartridge.hpp); these are dials that apply across
// every mood.

// ── Band motion ──────────────────────────────────────────────────
// Band activation order: fine(4) → detail(3) → local(2) → regional(1)
// → continental(0) → tectonic(5). First note animates fine ripples,
// full chord reshapes the continent.
static constexpr uint32_t BAND_ACTIVATION_ORDER[6] = { 4, 3, 2, 1, 0, 5 };
static constexpr float BAND_BLEND_ATTACK  = 3.0f;   // 1/s — blend ramp up
static constexpr float BAND_BLEND_RELEASE = 2.0f;   // 1/s — blend ramp down

// ── Mode intensity ramps ─────────────────────────────────────────
// All MModes (except palette drift, which has its own curve) ramp
// 0→1 toward the polyphony-driven target through these rates.
// Asymmetric: attack faster than release so onsets feel punchy.
static constexpr float MMODE_ATTACK  = 4.0f;        // 1/s — intensity ramp up
static constexpr float MMODE_RELEASE = 2.5f;        // 1/s — intensity ramp down

// ── Polyphony normalization ──────────────────────────────────────
// "Full" polyphony point — input above this saturates the mode at
// intensity 1.0. Different modes saturate at different polyphony
// counts so partial chords feel meaningful.
static constexpr float MMODE_POLYPHONY_FULL    = 6.0f;   // most modes: 6-note chord = full
static constexpr float PALETTE_POLYPHONY_FULL  = 3.0f;   // palette drift saturates earlier

// ── Mode-specific gains ──────────────────────────────────────────
// At zero intensity, output = 1.0×; at full intensity, output =
// 1.0× × (1 + GAIN). Gain coefficients, not ceilings — see the
// rollout report's note on the gain-vs-ceiling family.
static constexpr float GOL_TICK_GAIN   = 3.0f;      // up to 4× slower zones at full music
static constexpr float GOL_HEIGHT_GAIN = 2.0f;      // up to 3× taller cells at full music

// ── Mode-specific output scales ──────────────────────────────────
// Pre-multiplier on the [0,1] intensity before it's sent to the
// GPU. Tuned so each mode's visible amplitude feels right.
static constexpr float COLOR_SHIFT_OUTPUT_SCALE     = 0.6f;
static constexpr float CHECKER_SCATTER_OUTPUT_SCALE = 0.5f;

// ── Palette drift target ramp ────────────────────────────────────
// Palette drift target ramps toward desired across discrete steps;
// the rate keeps color from snapping when polyphony jumps.
static constexpr float PALETTE_DRIFT_TARGET_RATE = 2.0f;   // 1/s

// ── Radial pulse ─────────────────────────────────────────────────
// Pulses are event-driven: an onset (polyphony jump > threshold)
// writes a slot in the ring buffer, the kernel reads decaying
// pulses for terrain displacement.
static constexpr uint32_t PULSE_RING_SIZE        = 8;
static constexpr float    PULSE_AMPLITUDE        = 2.5f;   // world units of peak displacement
static constexpr float    PULSE_MAX_AGE          = 8.0f;   // seconds — must match WGSL
static constexpr float    PULSE_ONSET_THRESHOLD  = 0.5f;   // polyphony rise to count as onset
static constexpr float    PULSE_INCREASE_CLAMP   = 3.0f;   // max polyphony jump scaled into amplitude


// ═══ MODE REGISTRY ═══════════════════════════════════════════════
//
// Each MMode is an independently toggleable coupling circuit.
// When on: polyphony drives the mode's intensity through the ramp.
// When off: intensity releases to 0 (idle).
//
// Future: each mode's source can be rewired to any analysis stat.
// Today: all modes read polyphony as their input signal.
//
// Numpad bindings are temporary — see the rollout open-questions
// report on input fluidity. The function each mode controls is
// durable; the binding is placeholder.
//
//   Numpad 1 → MMODE_TERRAIN_WAVES      (band motion — retroactively mode 0)
//   Numpad 2 → MMODE_COLOR_SHIFT        (smooth → discrete mode bias)
//   Numpad 3 → MMODE_CHECKER_SCATTER    (sparse survival threshold bias)
//   Numpad 4 → MMODE_PALETTE_DRIFT      (terrain color drifts toward target)
//   Numpad 5 → MMODE_GOL_TEMPO          (zones slower + taller cells)
//   Numpad 6 → MMODE_AURA_EXPAND        (aura radius + height + tint)
//   Numpad 7 → MMODE_RADIAL_PULSE       (event-driven, separate from intensity array)

static constexpr uint32_t MMODE_TERRAIN_WAVES   = 0;   // band motion (existing)
static constexpr uint32_t MMODE_COLOR_SHIFT     = 1;
static constexpr uint32_t MMODE_CHECKER_SCATTER = 2;
static constexpr uint32_t MMODE_PALETTE_DRIFT   = 3;
static constexpr uint32_t MMODE_GOL_TEMPO       = 4;
static constexpr uint32_t MMODE_AURA_EXPAND     = 5;
static constexpr uint32_t MMODE_COUNT           = 6;   // numpad 1–6 (intensity-driven modes)

// Radial pulse mode: event-driven (no intensity trajectory).
// Toggle gates onset detection; existing pulses decay naturally.
static constexpr uint32_t MMODE_RADIAL_PULSE    = 7;   // numpad 7 (separate from intensity array)

// DONE[musical:L2] mode-name registry promoted to module-level constant.
//   Was previously a hidden static array inside toggle_mmode. Indexed
//   by mode id [0..7]; index 6 is "UNUSED" because MMODE_RADIAL_PULSE
//   skips to 7 (musical:L4 — semantic-kind off-by-one, folds into K1).
static constexpr const char* MMODE_NAMES[] = {
    "terrain_waves", "color_shift", "checker_scatter",
    "palette_drift", "gol_tempo",   "aura_expand",
    "UNUSED",        "radial_pulse"
};


// ═══ PALETTE DRIFT MAPS ══════════════════════════════════════════
//
// Polyphony → palette index lookups, indexed by clamped chord size.
// Authored data, not logic — kept at module scope so they're
// scannable as tables.

// Smooth palette mapping: ordered by contrast from sand baseline.
//   1 note → green(2), 2 notes → grey(3), 3+ notes → salmon(1).
//                                                  poly:  0     1     2     3
static constexpr float SMOOTH_PALETTE_MAP[]   = {        0.0f, 2.0f, 3.0f, 1.0f };

// Discrete tier mapping for drift mode.
//                                                  poly:  0     1     2     3     4
static constexpr float DISCRETE_TIER_MAP[]    = {        0.0f, 1.0f, 4.0f, 2.0f, 3.0f };


// ═══ MUSICAL MODULE STATE (Scope B migration #8) ═════════════════
//
// All musical-coupling state lives in this struct, accessed via
// musical_state_ on the Cartridge. Module functions take
// `MusicalState& ms` explicitly rather than reaching via Cartridge*,
// making ownership language-visible and dependencies explicit.
//
// Sub-grouped by subsystem:
//   • band motion         — polyphony-driven terrain band activation
//   • mode toggles        — per-mode bitmask + smoothed intensity
//   • mood gate           — silence all modes per mood policy
//   • palette drift       — smooth-ramped palette index target
//   • radial pulse        — onset-detection ring buffer
//
// Cross-module reads (consumed by other modules):
//   player_.mmode_intensities[]  — read by gol_zones (GOL_TEMPO),
//                                       pawn (AURA_EXPAND), spine
//   is_mmode_on(ms, mode)             — called by mood.inl

struct MusicalState {
    // ── Band motion ──────────────────────────────────────────────
    bool  band_motion_active        = false;       // true when polyphony drives bands (mood 5 only)
    float band_blend[6]             = { -1.f, -1.f, -1.f, -1.f, -1.f, -1.f };  // per-band blend (-1 = activity field)
    float band_phase_origin[6]      = {};          // t_beats when band was activated
    float band_blend_target[6]      = {};          // 0 or 1, driven by polyphony count

    // ── Mode toggles ─────────────────────────────────────────────
    uint32_t mmode_mask = 0;      // bitfield: which modes are active
    // (mmode_intensity[] migrated to player_.mmode_intensities in
    //  SEAM[spine:P8] — written by tick_musical_couplings, read by
    //  pawn.inl, gol_zones.inl, and the cartridge.hpp aura-config block.)

    // ── Mood gate ────────────────────────────────────────────────
    // Set by apply_mood from MoodProfile flags. Default = on. Read by
    // is_mmode_on to silence all modes in moods that don't allow them.
    bool mood_allows_modes = true;

    // ── Palette drift ────────────────────────────────────────────
    // Target palette index ramps smoothly to avoid color snaps.
    float palette_drift_target  = 0.0f;            // current [0,3] — ramps toward desired
    float palette_drift_desired = 0.0f;            // set by polyphony mapping

    // ── Radial pulse ─────────────────────────────────────────────
    // Ring buffer: 8 slots × 4 floats per slot = (origin_x, origin_z,
    // onset_seconds, amplitude). Circular write.
    float    pulse_ring[32]   = {};                // 8 × 4 floats
    uint32_t pulse_write_idx  = 0;                 // next slot to write (wraps at 8)
    float    prev_polyphony   = 0.0f;              // previous frame's polyphony (for onset detection)
};
MusicalState musical_state_;


// ═══ MODE QUERY + TOGGLE ═════════════════════════════════════════

static bool is_mmode_on(const MusicalState& ms, uint32_t mode) {
    if (!ms.mood_allows_modes) { return false; }   // mood gate — silences all modes
    return (ms.mmode_mask & (1u << mode)) != 0;
}

static void toggle_mmode(MusicalState& ms, Cartridge* c, uint32_t mode) {
    ms.mmode_mask ^= (1u << mode);
    bool on = is_mmode_on(ms, mode);
    // Retroactive: mode 0 controls band_motion_active
    if (mode == MMODE_TERRAIN_WAVES) {
        ms.band_motion_active = on;
        if (ms.band_motion_active) {
            for (int i = 0; i < 6; i++) {
                ms.band_blend[i] = 0.0f;
                ms.band_blend_target[i] = 0.0f;
                ms.band_phase_origin[i] = 0.0f;
            }
            c->gpuState_.set_band_motion(ms.band_blend, ms.band_phase_origin);
            c->gpuState_.set_terrain_time(0.0f);
        }
        else {
            float inactive[6] = { -1.f, -1.f, -1.f, -1.f, -1.f, -1.f };
            float zeros[6] = {};
            c->gpuState_.set_band_motion(inactive, zeros);
            c->gpuState_.set_terrain_time(0.0f);
        }
    }
    // Mode 5 (aura expand): mark config dirty to push updated aura params
    if (mode == MMODE_AURA_EXPAND) {
        c->pawn_state_.aura_cfg_dirty = true;
    }
    std::cout << "[MMode] " << MMODE_NAMES[mode] << ": " << (on ? "ON" : "OFF") << "\n";
}


// ═══ PER-FRAME COUPLING TICK ═════════════════════════════════════
//
// DONE[musical:K2] All polyphony→intensity ramps moved out of
//   cartridge.hpp::update() into this single named tick. The three
//   blocks that used to live there:
//     1) band motion (BAND_BLEND_ATTACK/RELEASE per band)
//     2) musical mode intensities (MMODE_ATTACK/RELEASE per mode)
//     3) palette drift target + intensity ramp
//   ...plus radial pulse onset detection (musical:K3 site) all
//   happen here in one call, in the same order they used to run.
//   Every exponential ramp uses the same Trajectory primitive shape
//   as WGSL §1.2 (trajectory_release).
//
// Caller: cartridge.hpp::update() — single site, runs every frame
// after the signal has been uploaded and before orb couplings.
static void tick_musical_couplings(MusicalState& ms, Cartridge* c, const AnalysisSignal& signal, wgpu::Queue& queue) {
    const float polyphony = signal.stats[0];
    const float dt        = signal.dt;

    // ─── 1. Polyphony-driven band motion ──────────────────────────
    if (ms.band_motion_active) {
        const uint32_t active_count = (uint32_t)std::max(0.0f, std::min(polyphony, 6.0f));

        // Set per-band targets: bands activate fine → tectonic
        for (uint32_t i = 0; i < 6; i++) ms.band_blend_target[i] = 0.0f;
        for (uint32_t i = 0; i < active_count; i++) {
            ms.band_blend_target[BAND_ACTIVATION_ORDER[i]] = 1.0f;
        }

        bool changed = false;
        for (uint32_t i = 0; i < 6; i++) {
            const float prev = ms.band_blend[i];
            const float target = ms.band_blend_target[i];

            // Capture phase origin at the moment a band activates
            if (target > 0.5f && prev < 0.01f) {
                ms.band_phase_origin[i] = c->time_state_.beats;
            }

            const float rate = (target > prev) ? BAND_BLEND_ATTACK : BAND_BLEND_RELEASE;
            Trajectory bb{ prev, 0.0f, 0.0f, 0.0f };
            bb = trajectory_release(bb, target, dt, rate);
            ms.band_blend[i] = bb.value;

            // Snap to endpoints to avoid perpetual drift
            if (ms.band_blend[i] < 0.001f && target == 0.0f) ms.band_blend[i] = 0.0f;
            if (ms.band_blend[i] > 0.999f && target == 1.0f) ms.band_blend[i] = 1.0f;

            if (ms.band_blend[i] != prev) changed = true;
        }

        if (changed) {
            c->gpuState_.set_band_motion(ms.band_blend, ms.band_phase_origin);
        }
        c->gpuState_.set_terrain_time(c->time_state_.beats);
    }

    // ─── 2. Musical mode intensities (per-mode polyphony coupling) ─
    bool any_changed = false;
    for (uint32_t m = 0; m < MMODE_COUNT; m++) {
        // Skip mode 0 (terrain waves) — handled by band motion above
        // Skip mode 3 (palette drift) — has its own steeper curve below
        if (m == MMODE_TERRAIN_WAVES || m == MMODE_PALETTE_DRIFT) continue;

        const bool on = is_mmode_on(ms, m);
        const float target = on ? std::min(polyphony / MMODE_POLYPHONY_FULL, 1.0f) : 0.0f;
        const float prev = c->player_.mmode_intensities[m];
        const float rate = (target > prev) ? MMODE_ATTACK : MMODE_RELEASE;
        Trajectory mi{ prev, 0.0f, 0.0f, 0.0f };
        mi = trajectory_release(mi, target, dt, rate);
        float next = mi.value;

        // Snap to endpoints
        if (next < 0.001f && target == 0.0f) next = 0.0f;
        if (next > 0.999f && target >= 1.0f) next = 1.0f;

        if (next != prev) {
            c->player_.mmode_intensities[m] = next;
            any_changed = true;
        }
    }
    if (any_changed) {
        c->gpuState_.set_mode_color_shift(c->player_.mmode_intensities[MMODE_COLOR_SHIFT] * COLOR_SHIFT_OUTPUT_SCALE);
        c->gpuState_.set_mode_checker_scatter(c->player_.mmode_intensities[MMODE_CHECKER_SCATTER] * CHECKER_SCATTER_OUTPUT_SCALE);

        // Aura expand: intensity scales aura parameters
        if (c->player_.mmode_intensities[MMODE_AURA_EXPAND] > 0.0f || is_mmode_on(ms, MMODE_AURA_EXPAND)) {
            c->pawn_state_.aura_cfg_dirty = true;
        }

        // GoL tempo: more polyphony = slower zones + taller cells.
        // tick_scale > 1 = slower, height_scale > 1 = taller.
        {
            const float gi = c->player_.mmode_intensities[MMODE_GOL_TEMPO];
            const float tick_scale   = 1.0f + gi * GOL_TICK_GAIN;
            const float height_scale = 1.0f + gi * GOL_HEIGHT_GAIN;
            c->gpuState_.set_mode_gol_scales(tick_scale, height_scale);
        }
    }

    // ─── 3. Palette drift (target + intensity, both ramped) ───────
    {
        const bool drift_on = is_mmode_on(ms, MMODE_PALETTE_DRIFT);

        if (drift_on && polyphony >= 1.0f) {
            const uint32_t idx = std::min((uint32_t)polyphony, 3u);
            ms.palette_drift_desired = SMOOTH_PALETTE_MAP[idx];
        }
        if (!drift_on || polyphony < 0.5f) {
            ms.palette_drift_desired = 0.0f;
        }

        // Ramp target smoothly (avoids color snaps)
        const float prev_t = ms.palette_drift_target;
        Trajectory pdt{ prev_t, 0.0f, 0.0f, 0.0f };
        pdt = trajectory_release(pdt, ms.palette_drift_desired, dt, PALETTE_DRIFT_TARGET_RATE);
        ms.palette_drift_target = pdt.value;

        // Intensity: poly/3 partial→full
        const float drift_intensity = drift_on ? std::min(polyphony / PALETTE_POLYPHONY_FULL, 1.0f) : 0.0f;
        const float prev_i = c->player_.mmode_intensities[MMODE_PALETTE_DRIFT];
        const float rate_i = (drift_intensity > prev_i) ? MMODE_ATTACK : MMODE_RELEASE;
        Trajectory pdi{ prev_i, 0.0f, 0.0f, 0.0f };
        pdi = trajectory_release(pdi, drift_intensity, dt, rate_i);
        c->player_.mmode_intensities[MMODE_PALETTE_DRIFT] = pdi.value;
        const float intensity = c->player_.mmode_intensities[MMODE_PALETTE_DRIFT];

        float discrete_tier = 0.0f;
        if (drift_on && polyphony >= 1.0f) {
            const uint32_t tidx = std::min((uint32_t)polyphony, 4u);
            discrete_tier = DISCRETE_TIER_MAP[tidx];
        }

        if (intensity > 0.001f || ms.palette_drift_target != prev_t) {
            c->gpuState_.set_mode_palette_drift(ms.palette_drift_target, intensity, discrete_tier);
        }
    }

    // ─── 4. Radial pulse onset detection (musical:K3 site) ────────
    {
        const bool pulse_on = is_mmode_on(ms, MMODE_RADIAL_PULSE);

        if (pulse_on && polyphony > ms.prev_polyphony + PULSE_ONSET_THRESHOLD) {
            const float increase = polyphony - std::max(ms.prev_polyphony, 0.0f);
            const uint32_t slot = ms.pulse_write_idx % PULSE_RING_SIZE;
            const uint32_t base = slot * 4;
            ms.pulse_ring[base + 0] = c->player_.readback_x;
            ms.pulse_ring[base + 1] = c->player_.readback_z;
            ms.pulse_ring[base + 2] = c->time_state_.seconds;
            ms.pulse_ring[base + 3] = PULSE_AMPLITUDE * std::min(increase, PULSE_INCREASE_CLAMP);
            ms.pulse_write_idx++;
            std::cout << "[Pulse] ONSET slot=" << slot
                << " pos=(" << c->player_.readback_x << "," << c->player_.readback_z << ")"
                << " t=" << c->time_state_.seconds
                << " amp=" << ms.pulse_ring[base + 3]
                << " poly=" << polyphony << " prev=" << ms.prev_polyphony
                << "\n";
        }
        ms.prev_polyphony = polyphony;

        // Count active (non-expired) pulses and upload
        uint32_t active = 0;
        for (uint32_t i = 0; i < PULSE_RING_SIZE; i++) {
            const float onset = ms.pulse_ring[i * 4 + 2];
            const float amp   = ms.pulse_ring[i * 4 + 3];
            if (amp > 0.001f && (c->time_state_.seconds - onset) < PULSE_MAX_AGE) {
                active = std::max(active, i + 1);
            }
        }
        c->gpuState_.set_pulse_data(active, ms.pulse_ring);
    }
}


// ═══ PER-MOOD-TRANSITION RESET ═══════════════════════════════════
//
// DONE[mood:K3] Per-mood-transition musical reset extracted from the
//   two duplicate sites (cartridge.hpp::teardown_world and
//   mood.inl::apply_mood) into this single named function. Both
//   callers now invoke reset_musical_couplings(queue). Mask stays
//   wired (toggles persist across world transitions), values reset.
static void reset_musical_couplings(MusicalState& ms, Cartridge* c, wgpu::Queue& queue) {
    for (uint32_t m = 0; m < MMODE_COUNT; m++) c->player_.mmode_intensities[m] = 0.0f;
    ms.palette_drift_target  = 0.0f;
    ms.palette_drift_desired = 0.0f;
    c->gpuState_.set_mode_color_shift(0.0f);
    c->gpuState_.set_mode_checker_scatter(0.0f);
    c->gpuState_.set_mode_palette_drift(0.0f, 0.0f, 0.0f);
    c->gpuState_.set_mode_gol_scales(1.0f, 1.0f);
    for (int i = 0; i < 32; i++) ms.pulse_ring[i] = 0.0f;
    ms.pulse_write_idx = 0;
    ms.prev_polyphony = 0.0f;
    float zero_pulses[32] = {};
    c->gpuState_.set_pulse_data(0, zero_pulses);
}


// ═══ SUN ORBIT COUPLING ══════════════════════════════════════════
//
// Per-frame coupling that swings the sun's azimuth around the mood's
// authored direction. Polyphony drives the orbital rate; the swing
// returns to the mood baseline when music quiets.
//
// Elevation is held at the mood-authored value — only azimuth moves.
// This keeps the "time of day" feeling intact and avoids vertical
// shadow drift that would feel uncanny.
//
//   effective_azimuth = mood_azimuth + sin(phase) × MAX_SWING
//   phase += dt × ORBITAL_RATE × (polyphony × ORBITAL_GAIN)
//
// Advance is proportional to polyphony only — at full polyphony a
// full half-swing (zero → max) takes about 2.6s; at silence the
// phase holds, so the sun stays put. This tunes to "the sun is
// alive but never flighty."
static constexpr float SUN_ORBITAL_RATE  = 0.3f;   // rad/s baseline phase rate
static constexpr float SUN_ORBITAL_GAIN  = 2.0f;   // multiplier from polyphony
static constexpr float SUN_MAX_SWING     = 0.785f; // ~45° from baseline (in radians)

static void tick_sun_coupling(MoodState& mds, Cartridge* c, float polyphony, float dt) {
    // Baseline mood sun direction (authored in MOOD_TABLE)
    const float* base = MOOD_TABLE[mds.active].sun_direction;

    // Decompose into azimuth (XZ plane) + elevation
    const float horiz_len = std::sqrt(base[0] * base[0] + base[2] * base[2]);
    const float base_azimuth = std::atan2(base[2], base[0]);
    const float base_elevation = std::atan2(base[1], horiz_len);

    // Polyphony in [0,1] — same normalization as MMODE intensities use
    const float poly_norm = std::min(polyphony / MMODE_POLYPHONY_FULL, 1.0f);

    // Accumulate phase
    mds.sun_orbit_phase += dt * SUN_ORBITAL_RATE * (poly_norm * SUN_ORBITAL_GAIN);

    // Wrap to keep float precision
    if (mds.sun_orbit_phase > 2.0f * 3.14159265358979f) mds.sun_orbit_phase -= 2.0f * 3.14159265358979f;

    // Effective azimuth = baseline + bounded swing
    const float swing = std::sin(mds.sun_orbit_phase) * SUN_MAX_SWING;
    const float effective_azimuth = base_azimuth + swing;

    // Reconstruct unit vector from (azimuth, elevation)
    const float ce = std::cos(base_elevation);
    const float dx = std::cos(effective_azimuth) * ce;
    const float dy = std::sin(base_elevation);
    const float dz = std::sin(effective_azimuth) * ce;

    c->gpuState_.set_sun_direction(dx, dy, dz);
}
