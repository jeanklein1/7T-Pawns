#pragma once
#include <cstdint>
#include "cartridges/the_board/contracts/mood_constants.hpp"   // MOOD_COUNT (sizes ORB_MOOD_TABLE)
#include "cartridges/the_board/contracts/orb_surface.hpp"      // ORGAN_3 w2 — ORB_CONSOLE_LIVE: dome / base size / noise floor
#include "cartridges/the_board/contracts/wgpu_fwd.hpp"   // wgpu handle fwds (lockstep insurance)

// ─── orbs.hpp (HEADER: console + registries + state + decls) ─────
//
// Sky orb layer — luminous points on a dome above the world.
//
// The impl additionally needs state.hpp (GPUOrbConfig, Dim::MAX_ORBS)
// and renderer.hpp — both precede it in the TU.
// ─────────────────────────────────────────────────────────────────

#include <cmath>      // std::sqrt (rotation-axis normalization)   // (impl, merged)
#include <iostream>   // operator feedback prints   // (impl, merged)
#include <algorithm>  // std::min   // (impl, merged)

namespace t7 {
namespace the_board {

// ═══ MODULE DEPS ════════════════════════════════════════════════════
// The sky-dome feature's requirements face. const trio: witness +
// clock + world read-only; GPU wire + renderer writable.
struct PlayerState; struct TimeState; struct WorldState;
class GPUState; class Renderer;
struct OrbsDeps {
    GPUState&         gpuState_;
    Renderer&         renderer_;
    const PlayerState& player_;
    const TimeState&  time_state_;
    const WorldState& world_state_;
};

// ═══ TUNING CONSOLE ══════════════════════════════════════════════

// ── Dome geometry ────────────────────────────────────────────────
// ORB_DOME_RADIUS / ORB_BASE_SIZE / ORB_NOISE_FLOOR graduated to
// contracts/orb_surface.hpp (ORGAN_3 w2) — ORB_CONSOLE is the design
// row, ORB_CONSOLE_LIVE the live surface configure_orbs reads.

// ── Noise floor ──────────────────────────────────────────────────
// Barely perceptible drift in silence — the kernel's noise input
// rests here; a gen-2 coupling may lerp it again.

// ── Rule-critical parameter floors ───────────────────────────────
// Graduated to contracts/orb_surface.hpp (ORGAN_3b P3) with
// OrbMoodConfig, whose `drag` initialiser names ORB_DEFAULT_DRAG.

// ═══ REGISTRY: PALETTES ══════════════════════════════════════════

inline constexpr uint32_t MAX_ORB_PALETTE_ENTRIES = 4;

struct OrbPaletteEntry {
    float hue;         // HSV hue center (0..1)
    float hue_var;     // spread around center
    float saturation;  // base saturation
    float weight;      // selection probability
};

struct OrbPalette {
    uint32_t count;
    float    value_variance;   // per-orb HSV value (brightness) spread
    OrbPaletteEntry entries[MAX_ORB_PALETTE_ENTRIES];
};

// Stellar classification — diagnostic baseline with bold equally-
// weighted pockets (O/B hot blue, F/G yellow-white, K orange, M red).
inline constexpr OrbPalette ORB_PALETTE_JWST_DEEP = {
    4, 0.15f,
    {
        //  hue    hue_var  sat    weight
        { 0.60f, 0.03f, 0.90f, 0.25f },   // hot blue
        { 0.12f, 0.04f, 0.60f, 0.25f },   // yellow-white
        { 0.07f, 0.04f, 0.85f, 0.25f },   // orange
        { 0.01f, 0.03f, 0.95f, 0.25f },   // deep red
    }
};

// Hubble SHO (Pillars of Creation): teal and gold with copper.
inline constexpr OrbPalette ORB_PALETTE_PILLARS = {
    4, 0.30f,
    {
        //  hue    hue_var  sat    weight
        { 0.10f, 0.03f, 0.65f, 0.40f },   // gold
        { 0.48f, 0.04f, 0.55f, 0.35f },   // teal
        { 0.02f, 0.02f, 0.70f, 0.15f },   // copper accent
        { 0.55f, 0.02f, 0.35f, 0.10f },   // pale blue
    }
};

// Carina Nebula: rich blues and amber-orange, no greens.
inline constexpr OrbPalette ORB_PALETTE_CARINA = {
    3, 0.30f,
    {
        //  hue    hue_var  sat    weight
        { 0.60f, 0.05f, 0.55f, 0.45f },   // rich blue
        { 0.08f, 0.04f, 0.60f, 0.40f },   // amber-orange
        { 0.55f, 0.03f, 0.30f, 0.15f },   // desaturated blue-white
        { 0.00f, 0.00f, 0.00f, 0.00f },   // (unused)
    }
};

// Single-pocket warm (legacy-equivalent, for testing).
inline constexpr OrbPalette ORB_PALETTE_WARM_MONO = {
    1, 0.20f,
    {
        //  hue    hue_var  sat    weight
        { 0.08f, 0.06f, 0.60f, 1.00f },
        { 0.00f, 0.00f, 0.00f, 0.00f },
        { 0.00f, 0.00f, 0.00f, 0.00f },
        { 0.00f, 0.00f, 0.00f, 0.00f },
    }
};

// The palette ids graduated to contracts/orb_surface.hpp (ORGAN_3b P3)
// with OrbMoodConfig, whose initialisers name them.

inline constexpr OrbPalette ORB_PALETTES[ORB_PAL_COUNT] = {
    ORB_PALETTE_JWST_DEEP,
    ORB_PALETTE_PILLARS,
    ORB_PALETTE_CARINA,
    ORB_PALETTE_WARM_MONO,
};

inline constexpr const char* ORB_PAL_NAMES[ORB_PAL_COUNT] = {
    "jwst_deep", "pillars", "carina", "warm_mono"
};

// ═══ REGISTRY: TIER SETS ═════════════════════════════════════════

inline constexpr uint32_t MAX_ORB_TIERS = 4;

struct OrbTier {
    // Physics (ranges sampled per-orb at init)
    float mass_mult = 1.0f;
    float drag_mult = 1.0f;
    float size_min = 0.7f;   // multiplier on base_size
    float size_max = 1.3f;
    float brightness_min = 0.7f;   // multiplier on palette value
    float brightness_max = 1.0f;
    // Coupling gains (apply each frame in dynamics kernel)
    float noise_gain = 1.0f;   // 0..1+
    float force_gain = 1.0f;
    float color_gain = 1.0f;
    // Selection
    float weight = 0.25f;  // relative selection probability
    // Flocking gains (used when motion_rule == 3)
    float flock_sep_gain = 1.0f;
    float flock_align_gain = 1.0f;
    float flock_coh_gain = 1.0f;
};

struct OrbTierSet {
    uint32_t count;
    OrbTier tiers[MAX_ORB_TIERS];
};

inline constexpr OrbTierSet ORB_TIERSET_JWST_STARS = {
    4,
    {
        //  mass   drag   s_min  s_max  b_min  b_max  n_g    f_g    c_g    w        fs     fa     fc
        {   1.8f,  0.6f,  1.2f,  1.6f,  0.9f,  1.0f,  0.6f,  0.8f,  1.0f,  0.10f,   0.6f,  0.8f,  0.3f  },  // giants
        {   1.0f,  1.0f,  0.8f,  1.1f,  0.6f,  0.9f,  1.0f,  1.0f,  1.0f,  0.60f,   1.0f,  1.0f,  1.0f  },  // main
        {   0.6f,  1.2f,  0.5f,  0.8f,  0.4f,  0.7f,  1.0f,  0.9f,  0.3f,  0.25f,   0.8f,  1.2f,  0.5f  },  // faint
        {   0.3f,  0.8f,  0.4f,  0.6f,  0.5f,  0.8f,  1.8f,  0.5f,  0.8f,  0.05f,   1.8f,  0.5f,  0.0f  },  // flickers
    }
};

inline constexpr OrbTierSet ORB_TIERSET_RESONANT = {
    3,
    {
        //  mass   drag   s_min  s_max  b_min  b_max  n_g    f_g    c_g    w        fs     fa     fc
        {   2.5f,  0.5f,  1.3f,  1.7f,  0.7f,  1.0f,  0.3f,  0.6f,  1.0f,  0.20f,   0.5f,  0.7f,  1.5f  },  // drones
        {   1.0f,  1.0f,  0.8f,  1.2f,  0.6f,  0.9f,  1.0f,  1.0f,  0.7f,  0.50f,   1.0f,  1.0f,  1.0f  },  // voices
        {   0.4f,  0.9f,  0.4f,  0.7f,  0.5f,  0.8f,  1.8f,  1.2f,  0.0f,  0.30f,   1.8f,  0.6f,  0.2f  },  // sparks
        {   0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,    0.0f,  0.0f,  0.0f  },  // (unused)
    }
};

// The tier-set ids graduated to contracts/orb_surface.hpp (ORGAN_3b P3)
// with OrbMoodConfig, whose initialisers name them.

inline constexpr OrbTierSet ORB_TIERSETS[ORB_TIERSET_COUNT] = {
    ORB_TIERSET_JWST_STARS,
    ORB_TIERSET_RESONANT,
};

inline constexpr const char* ORB_TIERSET_NAMES[ORB_TIERSET_COUNT] = {
    "jwst_stars", "resonant"
};

// ═══ REGISTRY: FLOCKING GESTURES ═════════════════════════════════

struct OrbFlockGesture {
    float       sep_sign;
    float       align_sign;
    float       coh_sign;
    const char* name;
};

inline constexpr uint32_t ORB_FLOCK_GESTURE_COUNT = 8;
inline constexpr OrbFlockGesture ORB_FLOCK_GESTURES[ORB_FLOCK_GESTURE_COUNT] = {
    //  sep    align  coh    name
    { +1.0f, +1.0f, +1.0f, "flock"     },  // 0
    { -1.0f, -1.0f, -1.0f, "antiflock" },  // 1
    { +1.0f, -1.0f, +1.0f, "swirl"     },  // 2
    { -1.0f, +1.0f, -1.0f, "orbit"     },  // 3
    { -1.0f, +1.0f, +1.0f, "huddle"    },  // 4
    { +1.0f, +1.0f, -1.0f, "flee"      },  // 5
    { +1.0f, -1.0f, -1.0f, "chaos"     },  // 6
    { -1.0f, -1.0f, +1.0f, "trap"      },  // 7
};

// ═══ REGISTRY: BROWNIAN GESTURES ═════════════════════════════════

struct OrbBrownianGesture {
    float       radial_sign;   // ±1 — polyphony expands or contracts
    float       vert_bias;     // 0 = isotropic, 1 = upward-biased noise
    float       coherence;     // 0 = per-orb, 1 = per-block ("wind gusts")
    const char* name;
};

inline constexpr uint32_t ORB_BROWNIAN_GESTURE_COUNT = 6;
inline constexpr OrbBrownianGesture ORB_BROWNIAN_GESTURES[ORB_BROWNIAN_GESTURE_COUNT] = {
    //  radial  vert   coh    name
    { +1.0f,   0.0f,  0.0f,  "drift"  },  // 0  default wandering diffusion
    { -1.0f,   0.0f,  0.0f,  "gather" },  // 1  music pulls orbs inward
    { +1.0f,   1.0f,  0.0f,  "rise"   },  // 2  upward drift (embers)
    { +1.0f,   0.0f,  1.0f,  "gust"   },  // 3  wind-gust group motion
    { -1.0f,   0.0f,  1.0f,  "tide"   },  // 4  coherent inward pull
    { +1.0f,   1.0f,  1.0f,  "swell"  },  // 5  upward column with coherence
};

// ═══ REGISTRY: ORBITAL GESTURES ══════════════════════════════════

struct OrbOrbitalGesture {
    float       alignment_mode;   // 0 scatter, 1 parallel, 2 mirror
    float       speed_var_mult;   // multiplier on per-orb speed spread
    const char* name;
};

inline constexpr uint32_t ORB_ORBITAL_GESTURE_COUNT = 4;
inline constexpr OrbOrbitalGesture ORB_ORBITAL_GESTURES[ORB_ORBITAL_GESTURE_COUNT] = {
    //  align  var    name
    { 0.0f,   1.0f,  "scatter"  },  // 0  random axes, chaotic rotation
    { 1.0f,   0.0f,  "parallel" },  // 1  unified shell around Y
    { 2.0f,   0.0f,  "mirror"   },  // 2  two counter-rotating streams
    { 1.0f,   3.0f,  "shear"    },  // 3  parallel axis, layered sheets
};

// ═══ MOOD CONFIG (authoring surface) ═════════════════════════════


// ═══ ORBS MODULE STATE ═══════════════════════════════════════════

inline constexpr uint32_t ORB_RULE_BROWNIAN = 0u;
inline constexpr uint32_t ORB_RULE_ORBITAL = 1u;
inline constexpr uint32_t ORB_RULE_FROZEN = 2u;
inline constexpr uint32_t ORB_RULE_FLOCKING = 3u;

struct OrbsState {
    // ── Lifecycle / kernel arming ────────────────────────────────
    bool     active = false;
    uint32_t count = 0;
    bool     init_pending = false;
    bool     recolor_pending = false;
    uint32_t current_palette_id = ORB_PAL_JWST_DEEP;

    // ── Motion rule + flocking gesture ───────────────────────────
    // Motion rule and flock gesture are both player-owned: they persist
    // across mood transitions. The rule seeds to Brownian on first run.
    uint32_t current_motion_rule = 0u;
    bool     motion_rule_initialized = false;  // one-time Brownian seed guard
    uint32_t gesture_idx[4]         = { 0u, 0u, 0u, 0u };
    bool     gesture_initialized[4] = { false, false, false, false };

    // ── Speed ────────────────────────────────────────────────────
    // ORGAN_5 P3a — `speed_mult_current` RETIRED. It was the gen-1
    // coupling's CPU smoother, and that coupling's writer went with it:
    // C6 found the field declared, reset in teardown, read once, and
    // never moved off 1.0 by anything. Its comment still promised
    // "smoothed on the CPU, uploaded via upload_orb_speed_mult only when
    // it moves" — a promise about a smoother that no longer existed.
    // The master motion strength is ORB_CONSOLE_LIVE.speed_mult now: an
    // authored console fact with a dial on it, and the rest a gen-2
    // coupling will claim through a seam.
};

// ═══ MODULE FUNCTIONS — DECLARATIONS ═════════════════════════════

// Lifecycle
// ORGAN_5 P1b — CONFIGURE LEARNS RESTRAINT. `reseed` says whether this
// re-speak must also re-run the init kernel. A mood change does (a new
// world's sky is a new sky); a panel edit that touched only per-frame
// GPU reads does not, and re-seeding for one would suppress the very
// motion texture the drag is meant to show.
// NO DEFAULT, deliberately: there are exactly two callers and each has a
// different answer, so a third that forgets should fail the BUILD rather
// than inherit whichever answer happened to be written here.
void configure_orbs(OrbsState& os, OrbsDeps* c, const OrbMoodConfig& cfg,
    wgpu::Queue& queue, bool reseed);
void teardown_orbs(OrbsState& os, OrbsDeps* c);
// Player commands
void cycle_orb_palette(OrbsState& os, OrbsDeps* c, wgpu::Queue& queue);
void cycle_orb_motion_rule(OrbsState& os, OrbsDeps* c, wgpu::Queue& queue);
void cycle_orb_gesture(OrbsState& os, OrbsDeps* c, wgpu::Queue& queue);
// GPU dispatches
void dispatch_orb_init(OrbsState& os, OrbsDeps* c, wgpu::CommandEncoder& encoder);
void dispatch_orb_recolor(OrbsState& os, OrbsDeps* c, wgpu::CommandEncoder& encoder);
void dispatch_orb_copy_prev(OrbsState& os, OrbsDeps* c, wgpu::CommandEncoder& encoder);
void dispatch_orb_dynamics(OrbsState& os, OrbsDeps* c, wgpu::CommandEncoder& encoder,
    wgpu::Queue& queue);
// Render
template <class Enc>
void render_orbs(OrbsState& os, OrbsDeps* c, Enc& pass);

// The orb mood table + OrbMoodConfig graduated to
// contracts/orb_surface.hpp (ORGAN_3b P3), where ORB_MOOD_LIVE stands
// beside the design table as the world's per-mood definition.

// ═══ IMPL:
// bodies deref orbs_state_(own) + gpu/renderer/player/time/world via OrbsDeps.
// COHORT: after renderer (Renderer) + patch_system (WorldState) + state/spine.

// ═══ GPU LAYOUT HELPERS ══════════════════════════════════════════

inline float* orb_tier_block_ptr(GPUOrbConfig& cfg, uint32_t i) {
    auto* base = reinterpret_cast<char*>(&cfg);
    return reinterpret_cast<float*>(base + 192u + i * 40u);
}

inline float* orb_tier_flock_ptr(GPUOrbConfig& cfg, uint32_t i) {
    auto* base = reinterpret_cast<char*>(&cfg);
    return reinterpret_cast<float*>(base + 416u + i * 16u);
}

// ═══ CONFIGURE HELPERS ═══════════════════════════════════════════

// Apply mood's first-run defaults to player-owned state. The flock
// gesture is "mood seeds once, player wins after." (The anchor seed
// retired — the dome is a skybox, eye-centered always.)
inline void apply_mood_first_run_defaults_(OrbsState& os, const OrbMoodConfig& cfg) {
    if (!os.gesture_initialized[ORB_RULE_BROWNIAN]) {
        os.gesture_idx[ORB_RULE_BROWNIAN] = std::min(
            cfg.flock_gesture_default, ORB_BROWNIAN_GESTURE_COUNT - 1u);
        os.gesture_initialized[ORB_RULE_BROWNIAN] = true;
    }
    if (!os.gesture_initialized[ORB_RULE_ORBITAL]) {
        os.gesture_idx[ORB_RULE_ORBITAL] = std::min(
            cfg.flock_gesture_default, ORB_ORBITAL_GESTURE_COUNT - 1u);
        os.gesture_initialized[ORB_RULE_ORBITAL] = true;
    }
    if (!os.gesture_initialized[ORB_RULE_FLOCKING]) {
        os.gesture_idx[ORB_RULE_FLOCKING] = std::min(
            cfg.flock_gesture_default, ORB_FLOCK_GESTURE_COUNT - 1u);
        os.gesture_initialized[ORB_RULE_FLOCKING] = true;
    }
    // ORB_RULE_FROZEN has no gestures — index stays at 0, unread.
}

// TEMPERAMENT (ORGAN_4 P1e): palette is config-owned — the dial and the
// mood are the durable authors and pack_palette_ re-speaks them; the
// KP palette-cycle is a live gesture that lasts until the next
// configure. Rule and gesture are the OPPOSITE (player-owned, seed
// once): that asymmetry is deliberate — a dial exists for palette_id,
// and a config the applier ignores is a dead dial (the motion_rule
// conviction, ORGAN_4).
//
// AND IT IS WHY base_hue / hue_variance ARE NOT DIALS. Every row of
// ORB_PALETTES carries count >= 1, so this function never leaves
// palette_count at 0, so the kernel's legacy single-hue arm
// (world.wgsl `if palette_count > 0u` … else) is unreachable. Two
// fields the applier faithfully copies into a branch the GPU cannot
// take: the enrollment stated a belief, the reader refused it.
//
// Pack the active palette's per-entry HSV pockets into GPU config.
inline void pack_palette_(OrbsState& os, GPUOrbConfig& gpuCfg, uint32_t palette_id) {
    uint32_t pal_id = std::min(palette_id, ORB_PAL_COUNT - 1u);
    os.current_palette_id = pal_id;
    const auto& pal = ORB_PALETTES[pal_id];

    gpuCfg.palette_count = pal.count;
    gpuCfg.value_variance = pal.value_variance;
    gpuCfg.pal0_hue = pal.entries[0].hue;
    gpuCfg.pal0_hue_var = pal.entries[0].hue_var;
    gpuCfg.pal0_sat = pal.entries[0].saturation;
    gpuCfg.pal0_weight = pal.entries[0].weight;
    gpuCfg.pal1_hue = pal.entries[1].hue;
    gpuCfg.pal1_hue_var = pal.entries[1].hue_var;
    gpuCfg.pal1_sat = pal.entries[1].saturation;
    gpuCfg.pal1_weight = pal.entries[1].weight;
    gpuCfg.pal2_hue = pal.entries[2].hue;
    gpuCfg.pal2_hue_var = pal.entries[2].hue_var;
    gpuCfg.pal2_sat = pal.entries[2].saturation;
    gpuCfg.pal2_weight = pal.entries[2].weight;
    gpuCfg.pal3_hue = pal.entries[3].hue;
    gpuCfg.pal3_hue_var = pal.entries[3].hue_var;
    gpuCfg.pal3_sat = pal.entries[3].saturation;
    gpuCfg.pal3_weight = pal.entries[3].weight;
}

inline void pack_tiers_(GPUOrbConfig& gpuCfg, uint32_t tierset_id) {

    if (tierset_id >= ORB_TIERSET_COUNT) {
        gpuCfg.tier_count = 0;
        for (uint32_t i = 0; i < MAX_ORB_TIERS; i++) {
            float* p = orb_tier_block_ptr(gpuCfg, i);
            for (int k = 0; k < 10; k++) p[k] = 0.0f;
            float* pf = orb_tier_flock_ptr(gpuCfg, i);
            pf[0] = 1.0f; pf[1] = 1.0f; pf[2] = 1.0f;
            if (i != 0u) pf[3] = 0.0f;
        }
        return;
    }

    const auto& ts = ORB_TIERSETS[tierset_id];
    uint32_t n = std::min(ts.count, MAX_ORB_TIERS);
    gpuCfg.tier_count = n;

    float wsum = 0.0f;
    for (uint32_t i = 0; i < n; i++) wsum += ts.tiers[i].weight;
    if (wsum < 1e-6f) wsum = 1.0f;  // pathological: avoid div-by-zero

    float cum = 0.0f;
    for (uint32_t i = 0; i < MAX_ORB_TIERS; i++) {
        float* p = orb_tier_block_ptr(gpuCfg, i);
        float* pf = orb_tier_flock_ptr(gpuCfg, i);

        if (i < n) {
            const auto& t = ts.tiers[i];
            cum += t.weight / wsum;
            if (i == n - 1) cum = 1.0f;   // clamp final bucket to avoid rounding drift
            p[0] = t.mass_mult;
            p[1] = t.drag_mult;
            p[2] = t.size_min;
            p[3] = t.size_max;
            p[4] = t.brightness_min;
            p[5] = t.brightness_max;
            p[6] = t.noise_gain;
            p[7] = t.force_gain;
            p[8] = t.color_gain;
            p[9] = cum;
            pf[0] = t.flock_sep_gain;
            pf[1] = t.flock_align_gain;
            pf[2] = t.flock_coh_gain;
            // pf[3] for tier 0 is orbital_speed_var_mult — don't touch.
            if (i != 0u) pf[3] = 0.0f;
        }
        else {
            // Unused tier slot: zero fields, cumulative = 1.0 so no roll lands here.
            for (int k = 0; k < 10; k++) p[k] = 0.0f;
            p[9] = 1.0f;
            pf[0] = 1.0f; pf[1] = 1.0f; pf[2] = 1.0f;
            if (i != 0u) pf[3] = 0.0f;
        }
    }
}

inline void pack_flocking_(const OrbsState& os, GPUOrbConfig& gpuCfg,
    float sep_r, float align_r, float coh_r,
    float sep_w, float align_w, float coh_w,
    float max_speed,
    float rule_drag_bwn, float rule_drag_orb,
    float rule_drag_frz, float rule_drag_flk) {
    gpuCfg.flock_sep_radius = sep_r;
    gpuCfg.flock_align_radius = align_r;
    gpuCfg.flock_coh_radius = coh_r;
    gpuCfg.flock_sep_weight = sep_w;
    gpuCfg.flock_align_weight = align_w;
    gpuCfg.flock_coh_weight = coh_w;
    gpuCfg.flock_max_speed = max_speed;
    gpuCfg.flock_coupling_intensity = 0.0f;

    {
        const auto& gb = ORB_BROWNIAN_GESTURES[os.gesture_idx[ORB_RULE_BROWNIAN]];
        gpuCfg.brownian_radial_sign = gb.radial_sign;
        gpuCfg.brownian_vert_bias = gb.vert_bias;
        gpuCfg.brownian_coherence = gb.coherence;

        const auto& go = ORB_ORBITAL_GESTURES[os.gesture_idx[ORB_RULE_ORBITAL]];
        gpuCfg.orbital_alignment_mode = go.alignment_mode;
        gpuCfg.orbital_speed_var_mult = go.speed_var_mult;

        const auto& gf = ORB_FLOCK_GESTURES[os.gesture_idx[ORB_RULE_FLOCKING]];
        gpuCfg.flock_sep_sign = gf.sep_sign;
        gpuCfg.flock_align_sign = gf.align_sign;
        gpuCfg.flock_coh_sign = gf.coh_sign;
    }

    // Per-rule drag: zero → 1.0× pass-through (mood has no opinion).
    auto passthrough = [](float authored) {
        return (authored > 0.0f) ? authored : 1.0f;
        };
    gpuCfg.rule_drag_brownian = passthrough(rule_drag_bwn);
    gpuCfg.rule_drag_orbital = passthrough(rule_drag_orb);
    gpuCfg.rule_drag_frozen = passthrough(rule_drag_frz);
    gpuCfg.rule_drag_flocking = passthrough(rule_drag_flk);
    // (speed_mult moved to the config-build block at ORGAN_5 P3a — it is a
    //  console fact for every rule, not a flocking one, and the CPU
    //  smoother it used to read retired with the gen-1 coupling.)
}

inline void log_configure_(const OrbsState& os, const OrbMoodConfig& cfg,
    float eff_drag, float eff_orbital_speed,
    uint32_t palette_id) {
    static const char* RULE_NAMES[] = { "brownian", "orbital", "frozen", "flocking" };

    std::cout << "[Orbs] Configured: count=" << os.count
        << " palette=" << ORB_PAL_NAMES[palette_id]
        << " drag=" << eff_drag
        << " noise=" << ORB_CONSOLE_LIVE.noise_floor
        << " rule=" << RULE_NAMES[std::min(os.current_motion_rule, 3u)]
        << " rot=" << cfg.rotation_speed
        << " orbital=" << eff_orbital_speed
        << " tiers="
        << (cfg.tierset_id < ORB_TIERSET_COUNT
            ? ORB_TIERSET_NAMES[cfg.tierset_id]
            : "legacy")
        << "\n";
}

// ═══ LIFECYCLE ═══════════════════════════════════════════════════

inline void configure_orbs(OrbsState& os, OrbsDeps* c, const OrbMoodConfig& cfg,
    wgpu::Queue& queue, bool reseed) {
    os.active = cfg.enabled != 0u;   // ORGAN_3b P3 — enabled is a u32 now (D2)
    os.count = std::min(cfg.count, (uint32_t)Dim::MAX_ORBS);
    if (!os.active || os.count == 0) return;

    // Effective values: "zero = no opinion, use system default."
    auto eff = [](float authored, float fallback) {
        return (authored > 0.0f) ? authored : fallback;
        };
    const float eff_drag = eff(cfg.drag, ORB_DEFAULT_DRAG);
    const float eff_orbital_speed = eff(cfg.orbital_base_speed, ORB_DEFAULT_ORBITAL_SPEED);
    const float eff_flock_sep_r = eff(cfg.flock_sep_radius, ORB_DEFAULT_FLOCK_SEP_R);
    const float eff_flock_align_r = eff(cfg.flock_align_radius, ORB_DEFAULT_FLOCK_ALIGN_R);
    const float eff_flock_coh_r = eff(cfg.flock_coh_radius, ORB_DEFAULT_FLOCK_COH_R);
    const float eff_flock_sep_w = eff(cfg.flock_sep_weight, ORB_DEFAULT_FLOCK_SEP_W);
    const float eff_flock_align_w = eff(cfg.flock_align_weight, ORB_DEFAULT_FLOCK_ALIGN_W);
    const float eff_flock_coh_w = eff(cfg.flock_coh_weight, ORB_DEFAULT_FLOCK_COH_W);
    const float eff_flock_max_speed = eff(cfg.flock_max_speed, ORB_DEFAULT_FLOCK_MAX_SPEED);

    // First-run mood defaults for player-owned state.
    apply_mood_first_run_defaults_(os, cfg);
    // Motion rule is player-owned (like the flock gesture): seed once to
    // Brownian, then leave it — mood transitions no longer overwrite it.
    if (!os.motion_rule_initialized) {
        os.current_motion_rule = ORB_RULE_BROWNIAN;
        os.motion_rule_initialized = true;
    }

    // Normalize rotation axis on CPU (GPU renormalizes too but this
    // keeps uploaded values unit-length to avoid surprises).
    float rx = cfg.rotation_axis[0];
    float ry = cfg.rotation_axis[1];
    float rz = cfg.rotation_axis[2];
    float rlen = std::sqrt(rx * rx + ry * ry + rz * rz);
    if (rlen > 0.001f) { rx /= rlen; ry /= rlen; rz /= rlen; }
    else { rx = 0.0f; ry = 1.0f; rz = 0.0f; }

    // Build the GPU config in one place.
    GPUOrbConfig gpuCfg{};
    gpuCfg.count = os.count;
    gpuCfg.seed = c->world_state_.active_seed;
    gpuCfg.base_hue = cfg.base_hue;
    gpuCfg.hue_variance = cfg.hue_variance;
    gpuCfg.brightness = cfg.brightness;
    gpuCfg.drag = eff_drag;
    gpuCfg.noise_amp = ORB_CONSOLE_LIVE.noise_floor;   // rests at the floor (driverless since the gen-1 retirement)
    gpuCfg.dome_radius = ORB_CONSOLE_LIVE.dome_radius;
    gpuCfg.base_size = ORB_CONSOLE_LIVE.base_size;
    // ORGAN_5 P3a — the master motion strength, read HERE beside the other
    // three console facts rather than at the tail of pack_flocking_, where
    // it sat behind a smoother that no longer exists. It is not a flocking
    // fact: the kernel scales every rule's energy by it.
    gpuCfg.speed_mult = ORB_CONSOLE_LIVE.speed_mult;
    gpuCfg.dt = 0.0f;
    gpuCfg.t_seconds = 0.0f;
    gpuCfg.force_radial = 0.0f;
    gpuCfg.motion_rule = os.current_motion_rule;
    gpuCfg.rotation_speed = cfg.rotation_speed;
    gpuCfg.rotation_axis_x = rx;
    gpuCfg.rotation_axis_y = ry;
    gpuCfg.rotation_axis_z = rz;
    gpuCfg.orbital_base_speed = eff_orbital_speed;

    pack_palette_(os, gpuCfg, cfg.palette_id);

    gpuCfg.color_pulse = 0.0f;
    gpuCfg.color_converge = 0.0f;
    gpuCfg.color_surge = 0.0f;
    gpuCfg.hue_converge_target = cfg.hue_converge_target;

    // Dome center — DEAD WIRE: the orb VS eye-centers the
    // dome (the skybox); these bytes are zero-filled for the ABI only.
    gpuCfg.dome_center_x = 0.0f;
    gpuCfg.dome_center_y = 0.0f;
    gpuCfg.dome_center_z = 0.0f;
    gpuCfg._pad_anchor = 0.0f;

    pack_tiers_(gpuCfg, cfg.tierset_id);
    pack_flocking_(os, gpuCfg,
        eff_flock_sep_r, eff_flock_align_r, eff_flock_coh_r,
        eff_flock_sep_w, eff_flock_align_w, eff_flock_coh_w,
        eff_flock_max_speed,
        cfg.rule_drag_brownian, cfg.rule_drag_orbital,
        cfg.rule_drag_frozen, cfg.rule_drag_flocking);

    c->gpuState_.upload_orb_config(queue, gpuCfg);
    // ORGAN_5 P1b — NEVER CLEAR A PENDING INIT HERE. A light pass leaves
    // the flag exactly as it found it, so a re-seed already armed by a
    // mood change (or by an earlier heavy edit in the same frame) still
    // fires; only a heavy pass ever raises it.
    //
    // THE UNIFORM'S dt/t ARE ZEROED BY THIS WRITE and that is invisible
    // by construction, not by luck: `gpuCfg` is value-initialised, so a
    // whole-struct upload carries dt = t_seconds = 0 — and organ_flush
    // runs at the HEAD of the frame (pawn.cpp) while phase_orb_sky runs
    // in the RENDER spine, so the same frame's dispatch_orb_dynamics
    // re-authors both before any kernel reads them. With the dome
    // inactive the dispatch early-returns and nothing reads the uniform
    // at all. Every mood change has always taken this same path.
    if (reseed) os.init_pending = true;

    log_configure_(os, cfg, eff_drag, eff_orbital_speed, os.current_palette_id);
}

inline void teardown_orbs(OrbsState& os, OrbsDeps* c) {
    (void)c;
    os.active = false;
    os.count = 0;
    os.init_pending = false;
    os.recolor_pending = false;

    // (The speed multiplier's reset retired with the field at ORGAN_5 P3a:
    //  the master strength is a CONSOLE fact now, and a console fact is
    //  the operator's, so a teardown must not take it back.)
}

// ═══ PLAYER COMMANDS ═════════════════════════════════════════════

inline void cycle_orb_palette(OrbsState& os, OrbsDeps* c, wgpu::Queue& queue) {
    if (!os.active || os.count == 0) {
        std::cout << "[Orbs] Palette cycle ignored (no active dome)\n";
        return;
    }

    os.current_palette_id = (os.current_palette_id + 1u) % ORB_PAL_COUNT;
    const auto& pal = ORB_PALETTES[os.current_palette_id];

    float pal_data[16];
    for (uint32_t i = 0; i < 4; i++) {
        pal_data[i * 4 + 0] = pal.entries[i].hue;
        pal_data[i * 4 + 1] = pal.entries[i].hue_var;
        pal_data[i * 4 + 2] = pal.entries[i].saturation;
        pal_data[i * 4 + 3] = pal.entries[i].weight;
    }
    c->gpuState_.upload_orb_palette(queue, pal.count, pal.value_variance, pal_data);

    // Color-only refresh: positions, velocities, twinkle phase all
    // persist so the sky "holds" and only the hues transition.
    os.recolor_pending = true;

    std::cout << "[Orbs] Palette: " << ORB_PAL_NAMES[os.current_palette_id] << "\n";
}

inline void cycle_orb_motion_rule(OrbsState& os, OrbsDeps* c, wgpu::Queue& queue) {
    if (!os.active || os.count == 0) {
        std::cout << "[Orbs] Motion rule cycle ignored (no active dome)\n";
        return;
    }

    os.current_motion_rule = (os.current_motion_rule + 1u) % 4u;
    c->gpuState_.upload_orb_motion_rule(queue, os.current_motion_rule);

    static const char* RULE_NAMES[] = { "brownian", "orbital", "frozen", "flocking" };
    std::cout << "[Orbs] Motion rule: " << RULE_NAMES[os.current_motion_rule];
    const uint32_t r = os.current_motion_rule;
    const uint32_t gidx = os.gesture_idx[r];
    if (r == ORB_RULE_BROWNIAN && gidx != 0u) {
        std::cout << " (gesture: " << ORB_BROWNIAN_GESTURES[gidx].name << ")";
    }
    else if (r == ORB_RULE_ORBITAL && gidx != 0u) {
        std::cout << " (gesture: " << ORB_ORBITAL_GESTURES[gidx].name << ")";
    }
    else if (r == ORB_RULE_FLOCKING && gidx != 0u) {
        std::cout << " (gesture: " << ORB_FLOCK_GESTURES[gidx].name << ")";
    }
    std::cout << "\n";
}

inline void cycle_orb_gesture(OrbsState& os, OrbsDeps* c, wgpu::Queue& queue) {
    const uint32_t r = os.current_motion_rule;

    if (r == ORB_RULE_BROWNIAN) {
        os.gesture_idx[r] = (os.gesture_idx[r] + 1u) % ORB_BROWNIAN_GESTURE_COUNT;
        const auto& g = ORB_BROWNIAN_GESTURES[os.gesture_idx[r]];
        c->gpuState_.upload_orb_brownian_gesture(queue,
            g.radial_sign, g.vert_bias, g.coherence);
        std::cout << "[Orbs] Brownian gesture: " << g.name << "\n";
        return;
    }
    if (r == ORB_RULE_ORBITAL) {
        os.gesture_idx[r] = (os.gesture_idx[r] + 1u) % ORB_ORBITAL_GESTURE_COUNT;
        const auto& g = ORB_ORBITAL_GESTURES[os.gesture_idx[r]];
        c->gpuState_.upload_orb_orbital_gesture(queue,
            g.alignment_mode, g.speed_var_mult);
        std::cout << "[Orbs] Orbital gesture: " << g.name << "\n";
        return;
    }
    if (r == ORB_RULE_FLOCKING) {
        os.gesture_idx[r] = (os.gesture_idx[r] + 1u) % ORB_FLOCK_GESTURE_COUNT;
        const auto& g = ORB_FLOCK_GESTURES[os.gesture_idx[r]];
        c->gpuState_.upload_orb_flock_signs(queue,
            g.sep_sign, g.align_sign, g.coh_sign);
        std::cout << "[Orbs] Flocking gesture: " << g.name
            << " (sep=" << (g.sep_sign > 0 ? "+" : "-")
            << " align=" << (g.align_sign > 0 ? "+" : "-")
            << " coh=" << (g.coh_sign > 0 ? "+" : "-")
            << ")\n";
        return;
    }

    // ORB_RULE_FROZEN: stillness is the rule's defining property.
    std::cout << "[Orbs] Frozen has no gestures (stillness is the rule).\n";
}

// (The dome anchor commands retired — the dome is a SKYBOX,
// eye-centered in the orb VS every frame; KP_9 freed.)

// ═══ GPU DISPATCHES ══════════════════════════════════════════════

// One-shot seed-to-dome kernel. Fires once per configure_orbs when
// os.init_pending is armed.
inline void dispatch_orb_init(OrbsState& os, OrbsDeps* c, wgpu::CommandEncoder& encoder) {
    if (!os.init_pending) return;
    os.init_pending = false;

    wgpu::ComputePassDescriptor cpd{};
    cpd.label = "Orb Init";
    cpd.timestampWrites = c->gpuState_.meter_arm_compute(meter_row::OrbSky);
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&cpd);
    // LOOM_2 pass head: WORLD + FRAME are every pipeline's strata 0/1.
    { pass.SetBindGroup(0, c->gpuState_.world_group());
      pass.SetBindGroup(1, c->gpuState_.frame_c_group()); }
    uint32_t wgs = (os.count + 63u) / 64u;
    c->renderer_.dispatch_orb_init(pass, c->gpuState_.orbs_a_state_group(), c->gpuState_.empty_group(), wgs);
    pass.End();

    std::cout << "[Orbs] Init dispatched: " << os.count
        << " orbs, " << wgs << " workgroups\n";
}

// Palette re-sample kernel. Fires on cycle_orb_palette — keeps
// positions/velocities/twinkle, only hues shift.
inline void dispatch_orb_recolor(OrbsState& os, OrbsDeps* c, wgpu::CommandEncoder& encoder) {
    if (!os.recolor_pending) return;
    os.recolor_pending = false;

    wgpu::ComputePassDescriptor cpd{};
    cpd.label = "Orb Recolor";
    cpd.timestampWrites = c->gpuState_.meter_arm_compute(meter_row::OrbSky);
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&cpd);
    // LOOM_2 pass head: WORLD + FRAME are every pipeline's strata 0/1.
    { pass.SetBindGroup(0, c->gpuState_.world_group());
      pass.SetBindGroup(1, c->gpuState_.frame_c_group()); }
    uint32_t wgs = (os.count + 63u) / 64u;
    c->renderer_.dispatch_orb_recolor(pass, c->gpuState_.orbs_a_state_group(), c->gpuState_.empty_group(), wgs);
    pass.End();
}

inline void dispatch_orb_copy_prev(OrbsState& os, OrbsDeps* c, wgpu::CommandEncoder& encoder) {
    if (!os.active || os.count == 0) return;

    wgpu::ComputePassDescriptor cpd{};
    cpd.label = "Orb Copy Prev";
    cpd.timestampWrites = c->gpuState_.meter_arm_compute(meter_row::OrbSky);
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&cpd);
    // LOOM_2 pass head: WORLD + FRAME are every pipeline's strata 0/1.
    { pass.SetBindGroup(0, c->gpuState_.world_group());
      pass.SetBindGroup(1, c->gpuState_.frame_c_group()); }
    uint32_t wgs = (os.count + 63u) / 64u;
    c->renderer_.dispatch_orb_copy_prev(pass, c->gpuState_.orbs_b_state_group(), c->gpuState_.empty_group(), wgs);
    pass.End();
}

// Per-frame rule + couplings. Uploads dt/t_seconds, then dispatches.
inline void dispatch_orb_dynamics(OrbsState& os, OrbsDeps* c, wgpu::CommandEncoder& encoder,
    wgpu::Queue& queue) {
    if (!os.active || os.count == 0) return;

    // PLUMB_0 B2 — the orb config's clock is a GPU seam, so it takes the
    // rebased number like the frame signal does. orb_config.t_seconds drives
    // the twinkle phase and salts a per-frame seed; both want a small float.
    c->gpuState_.upload_orb_frame(queue, c->time_state_.dt,
                                  c->time_state_.gpu_seconds());

    wgpu::ComputePassDescriptor cpd{};
    cpd.label = "Orb Dynamics";
    cpd.timestampWrites = c->gpuState_.meter_arm_compute(meter_row::OrbSky);
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&cpd);
    // LOOM_2 pass head: WORLD + FRAME are every pipeline's strata 0/1.
    { pass.SetBindGroup(0, c->gpuState_.world_group());
      pass.SetBindGroup(1, c->gpuState_.frame_c_group()); }
    uint32_t wgs = (os.count + 63u) / 64u;
    c->renderer_.dispatch_orb_dynamics(pass, c->gpuState_.orbs_a_state_group(), c->gpuState_.empty_group(), wgs);
    pass.End();
}

// ═══ RENDER ══════════════════════════════════════════════════════

// Additive billboard draw into the main render pass.
template <class Enc>
inline void render_orbs(OrbsState& os, OrbsDeps* c, Enc& pass) {
    // BUNDLE_1: the `!os.active || os.count == 0` guard is the record's now
    // (stage_draw_ledger stages zero instances for either) — an encoder-time
    // skip would be frozen into the bundle that records this draw.
    (void)os;
    c->renderer_.draw_orbs(pass,
        c->gpuState_.orb_quad_vb(),
        c->gpuState_.orb_quad_ib(),
        c->gpuState_.orb_state_buffer(),
        c->gpuState_.draw_ledger_buffer(),
        GPUState::draw_record_offset(GPUState::DR_ORBS));
}

} // namespace the_board
} // namespace t7
