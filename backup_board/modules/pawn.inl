// ─── pawn.inl ────────────────────────────────────────────────────
//
// Player-relative state: aura field (toroidal 64×64 spring grid that
// activates near the pawn and releases when it moves away),
// presence trajectory, per-frame coupling tick.
//
// Architecture:
//   PawnAuraProfile    — declarative parameter table
//   PawnAuraDeltaMode  — color differential strategy
//   GPUPawnAuraConfig  — per-frame GPU config (in state.hpp)
//   GPUPawnAuraCell    — per-cell state (in state.hpp)
//   aura_presence       — Trajectory-style 0→1 ramp on toggle
//
// ┌─── Public surface (called from outside this file) ──────────────┐
// │                                                                  │
// │  Module functions are static, take PawnState& explicitly.        │
// │                                                                  │
// │  Per-frame updates:                                              │
// │    tick_pawn_couplings(ps, c, queue)                             │
// │      — presence ramp + height compute                            │
// │                                                                  │
// │  Cross-module writers (set pawn_state_ flags from outside):      │
// │    input.inl    — toggles aura_enabled, aura_height_enabled;     │
// │                   marks aura_cfg_dirty                           │
// │    mood.inl     — clears aura_enabled when mood disallows aura   │
// │    musical.inl  — marks aura_cfg_dirty when coupling changes     │
// │                                                                  │
// │  Cross-module readers (read pawn_state_ from outside):           │
// │    cartridge.hpp — aura compute dispatch reads all fields per    │
// │                    frame (currently inline; candidate for future │
// │                    cross-file move into pawn.inl)                │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// Included inside the Cartridge class body.
// Depends on: musical/trajectory.hpp (release primitive), musical.inl
//             (player_.mmode_intensities[MMODE_AURA_EXPAND] for the expand
//             coupling).
// ─────────────────────────────────────────────────────────────────


// ═══ TUNING CONSOLE ══════════════════════════════════════════════
//
// System-level dials for the pawn aura. Profile-authored values
// (radius, stiffness, tints, mode) live in PawnAuraProfile below;
// these are dials that apply regardless of which profile is active.

// ── Aura presence ramp ───────────────────────────────────────────
// aura_presence ramps 0→1 on enable and 1→0 on disable. Smooths
// the transition so terrain extrusion and pawn height change
// gradually rather than snapping. Asymmetric: enable feels
// deliberate (~3s), disable feels release-y (~2s).
static constexpr float AURA_PRESENCE_ATTACK  = 1.0f;   // 1/s — ~3s to full (spring converges in ~0.5s)
static constexpr float AURA_PRESENCE_RELEASE = 1.5f;   // 1/s — ~2s to zero

// ── Musical coupling ─────────────────────────────────────────────
// Aura height is gained by player_.mmode_intensities[MMODE_AURA_EXPAND].
// At zero musical intensity, height = base × presence.
// At full intensity,         height = base × presence × (1 + GAIN).
static constexpr float AURA_EXPAND_GAIN = 3.0f;        // 4× baseline at full music


struct PawnAuraDeltaMode {
    static constexpr uint32_t CONVERGENT = 0;  // all cells shift toward signature tint
    static constexpr uint32_t RANDOM = 1;  // each cell gets unique random delta
};

struct PawnAuraProfile {
    float influence_radius;
    float attack_stiffness;
    float attack_damping;
    float release_rate;
    float tint_strength;
    float tint_r, tint_g, tint_b;
    uint32_t delta_mode;
    float delta_magnitude;     // random mode: max offset per channel
    uint32_t effect_mask;      // bit 0=color, bit 1=height
    float height_scale;        // height extrusion in world units
};

static constexpr PawnAuraProfile PAWN_AURA_DEFAULT = {
    20.0f,             // influence_radius
    12.0f,             // attack_stiffness
    0.7f,              // attack_damping
    1.5f,              // release_rate
    0.5f,              // tint_strength
    0.4f, 0.2f, 0.5f, // tint RGB (purple)
    PawnAuraDeltaMode::CONVERGENT,
    0.3f,              // delta_magnitude (used in random mode)
    0x3u,              // effect_mask: color tint + height
    3.0f,              // height_scale
};

// ═══ PAWN MODULE STATE (Scope B migration #5) ═══════════════════
//
// All pawn-owned state lives in this struct, accessed via
// pawn_state_ on the Cartridge. Module functions take
// `PawnState& ps` explicitly rather than reaching via Cartridge*,
// making ownership language-visible and dependencies explicit
// in signatures.
//
// Field roles:
//
//   active_aura_profile — Currently active profile. Starts as
//     PAWN_AURA_DEFAULT and can be swapped by landmarks/commands.
//
//   aura_enabled — On/off intent. Currently toggled by numpad 3
//     (input.inl); the presence ramp below smooths the transition
//     so the toggle never snaps. The toggle key is temporary —
//     see input.inl's binding fluidity note; the function this
//     controls (aura on/off) will remain even when the binding
//     changes.
//
//   aura_height_enabled — Height-effect gate. Currently toggled
//     by key 2 (input.inl); leaves the color tint visible while
//     flattening the terrain extrusion. Same temporary-binding
//     caveat as aura_enabled.
//
//   aura_needs_clear — Internal: marks need to clear cells next
//     frame after aura disable.
//
//   aura_cfg_dirty — Internal: full-config upload flag. true at
//     boot → first frame uploads full config. Set true by external
//     writers (input keys, mood transitions, musical coupling
//     changes) when a parameter shift requires re-upload.
//
//   (aura_presence — migrated to player_.aura_presence in SEAM[spine:P8].
//    Read here as c->player_.aura_presence; written in tick_pawn_aura.)

struct PawnState {
    PawnAuraProfile active_aura_profile = PAWN_AURA_DEFAULT;
    bool            aura_enabled        = false;
    bool            aura_height_enabled = true;
    bool            aura_needs_clear    = false;
    bool            aura_cfg_dirty      = true;
};
PawnState pawn_state_;


// ─── Per-frame pawn coupling tick ────────────────────────────────
//
// DONE[pawn:K1] aura presence ramp + height computation moved out of
//   cartridge.hpp::update() into this single named tick. The presence
//   value uses trajectory_release (mirroring WGSL §1.2). The height
//   computation reads player_.mmode_intensities[MMODE_AURA_EXPAND] from
//   musical.inl — kept the cross-module read explicit since the
//   coupling is genuinely musical→aura, not pawn-internal.
//
// Caller: cartridge.hpp::update() — runs once per frame after the
// signal is built and before world bounds are uploaded.
static void tick_pawn_couplings(PawnState& ps, Cartridge* c, wgpu::Queue& queue) {
    // Aura presence ramp: smooth 0→1 on enable / 1→0 on disable.
    // (aura_presence migrated to player_ in SEAM[spine:P8] — see Cartridge::PlayerState)
    {
        const float target = ps.aura_enabled ? 1.0f : 0.0f;
        const float prev   = c->player_.aura_presence;
        const float rate   = (target > prev) ? AURA_PRESENCE_ATTACK : AURA_PRESENCE_RELEASE;
        Trajectory ap{ prev, 0.0f, 0.0f, 0.0f };
        ap = trajectory_release(ap, target, c->time_state_.dt, rate);
        c->player_.aura_presence = ap.value;

        // Snap to endpoints to avoid perpetual drift
        if (c->player_.aura_presence < 0.001f && target == 0.0f) c->player_.aura_presence = 0.0f;
        if (c->player_.aura_presence > 0.999f && target == 1.0f) c->player_.aura_presence = 1.0f;
        if (c->player_.aura_presence != prev) ps.aura_cfg_dirty = true;
    }

    // Pawn aura height: presence × base height × musical-expand multiplier.
    // Same value is consumed by terrain VS for extrusion, so pawn and
    // terrain always agree.
    const float aura_expand_mult = 1.0f + c->player_.mmode_intensities[MMODE_AURA_EXPAND] * AURA_EXPAND_GAIN;
    const float effective_aura_height = ps.aura_height_enabled
        ? ps.active_aura_profile.height_scale * c->player_.aura_presence * aura_expand_mult
        : 0.0f;
    c->gpuState_.set_pawn_aura_height(effective_aura_height);
    // Keep compute running while ramping down (so the trail decays cleanly).
    c->gpuState_.set_aura_enabled(c->player_.aura_presence > 0.001f);
}
