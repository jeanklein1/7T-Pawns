#pragma once
#include <cstdint>
#include "cartridges/the_board/realization/state.hpp"                    // Dim::*, GPUZoneDeriveRequestArray, wgpu
#include "cartridges/the_board/contracts/mood_constants.hpp"   // MOOD_COUNT (sizes the mood gate)
#include "cartridges/the_board/contracts/wgpu_fwd.hpp"   // wgpu handle fwds (lockstep insurance)
#include "cartridges/the_board/contracts/entity_types.hpp"     // GoLSelection/GoLPlacement (the boundary DTOs) + queue types

// ─── gol_zones.hpp (HEADER: vocabulary + state + decls) ──────────
//
// Zone-local Game of Life + Pulse automata.
//
// The impl additionally reaches the spawn-engine services and
// GLOBAL_ENTITY_DENSITY (contracts/spawn_services.hpp), and
// Dim::PATCH_EXTENT (patch_system.hpp); PopFamily is roster.hpp
// vocabulary.
//
// SEAM[gol_zones:complete-subsystem] complete bespoke pipeline in one
//   module — vocabulary + state + lifecycle + dispatch all together.
//   Distinguishable from the cockpit pattern (multiple decoupled
//   commands); this is single-lifecycle bespoke. Same family as
//   gallery and ribbon.
// SEAM[gol_zones:dual-algorithm] this module houses two algorithms —
//   Conway (GoLTierProfile, GOL_TIERS[]) and Pulse (GolPulseTierProfile,
//   GOL_PULSE_TIERS[]) — gated by GOL_PULSE_ALGORITHM_CHANCE. The shared
//   infrastructure (zone state, seeding, dispatch) is single-track;
//   only the parameter sampling and life initialization branch.
// ─────────────────────────────────────────────────────────────────

#include <cmath>      // std::floor, std::hypot (the footprint radius)   // (impl, merged)
#include <algorithm>  // std::max, std::min   // (impl, merged)
#include <iostream>   // the spawn log   // (impl, merged)
#include "core/instruments.hpp"   // PURSE_0 R3 — INSTRUMENTS.stream_witness gates the spawn log
#include <vector>     // life / height-factor staging   // (impl, merged)

namespace t7 {
namespace the_board {

// ═══ MODULE DEPS (S5) ═══════════════════════════════════════════════
// The GoL score-verbs' requirements face. device_ is the DECLARED
// handover (stamp S5): flush_zone_derive_requests submits its derive
// pass on its OWN encoder, MID-RENDER, and it MUST execute before the
// same frame's agent kernels — SEAM[gol:derive-submit]. Declaring the
// device changes access, never submission order; the refactor (folding
// into the frame encoder) stays FORBIDDEN.
class Renderer;
struct GolDeps {
    GPUState&        gpuState_;
    Renderer&        renderer_;
    wgpu::Device&    device_;   // SEAM[gol:derive-submit] — immediate mid-render submit; never folds into the frame encoder
    const TimeState& time_state_;  // upload_gol_zone_config reads beats/dt for the header
};

// ═══ TUNING CONSOLE ══════════════════════════════════════════════

// ── Spatial constants ────────────────────────────────────────────
// MUST match world.wgsl's MODE_LATTICE_SPACING (TUNING SURFACE
// DIRECTORY: "MODE_LATTICE_SPACING 120 wu — smooth/discrete
// clusters"). Hardware mirror — when tuning, change both sides.
inline constexpr float MODE_LATTICE_SPACING = 120.0f;

// ── Algorithm gate ───────────────────────────────────────────────
inline constexpr float GOL_PULSE_ALGORITHM_CHANCE = 0.35f;

// ═══ ALGORITHM TYPES (shared) ════════════════════════════════════

struct AlgorithmType {
    static constexpr uint32_t CONWAY = 0;
    static constexpr uint32_t PULSE = 1;
};

struct BoundaryMode {
    static constexpr uint32_t REFLECT = 0;
    static constexpr uint32_t WRAP = 1;
};

// The Pulse rows' field function: which spatial law writes the per-cell
// target. BREATH is the per-cell sinusoid the algorithm shipped with.
// L3 MIRROR: world.wgsl PULSE_FIELD_*.
struct PulseField {
    static constexpr uint32_t BREATH = 0;   // what ships today
    static constexpr uint32_t SPIRAL = 1;
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
    static constexpr float SPAWN_CHANCE = 0.60f;  // fraction of checkerboard zones
    // Fraction of zones that get extrusion. The roll refuses no zone; the
    // only flat zones left are the three tiers whose identity is flatness
    // (Conway Flash, Pulse Sparkle and Pulse Spiral set force_no_height),
    // so the delivered rate is 0.8335. Recipe: each family's weights sum
    // to 1, and GOL_PULSE_ALGORITHM_CHANCE = 0.35 splits them, so the flat
    // weight is 0.65 x Flash 0.03 + 0.35 x (Sparkle 0.24 + Spiral 0.18)
    // = 0.0195 + 0.147 = 0.1665, and 1.0 - 0.1665 = 0.8335. See the tier
    // tables below.
    // (TUNE_1 A10 moved Flash 0.17 -> 0.03, which is an INPUT to this
    //  recipe. As of A10 flat zones went 0.2155 -> 0.1245 — about one in
    //  eight, not rare — and Pulse Sparkle, untouched by A10, owned 84%
    //  of what flatness remained.
    //  GOL_RULES_1 is this recipe's SECOND input: it reweighted both
    //  families and added Spiral, a second force_no_height Pulse row.
    //  Flat zones are now 0.1665 — about one in six — and the flat weight
    //  splits Flash 12% / Sparkle 50% / Spiral 38%.)
    static constexpr float HEIGHT_CHANCE = 1.00f;
    static constexpr float MODE_THRESHOLD = 0.50f;  // min interpolated mode for eligibility
    // Per-cell height factor seeding (Gaussian draw per cell)
    static constexpr float HEIGHT_FACTOR_MEAN = 1.0f;
    static constexpr float HEIGHT_FACTOR_SIGMA = 0.15f;
    static constexpr float HEIGHT_FACTOR_CLAMP_LO = 0.6f;
    // L3 MIRROR: world.wgsl GOL_HEIGHT_FACTOR_MAX. This is the upper bound on
    // the per-cell multiplier, and the indoor height cap divides by it at
    // zone_derive_params so the capped lift is exact. Change both rooms.
    static constexpr float HEIGHT_FACTOR_CLAMP_HI = 1.4f;
    // Lens target color range: color = hash * RANGE + LO
    static constexpr float LENS_TARGET_LO = 0.2f;
    static constexpr float LENS_TARGET_RANGE = 0.6f;
    // SEAM[gol_zones:P4] hygiene rows pattern (P4): the gol mood row
    //   lives in MOOD_SPAWN_MULT (population_themes.hpp — the GOL
    //   column). That column is all 1.0 — the mood term rests at
    //   identity and suppresses nothing today; the veto path
    //   (veto_on_zero_mood) is the live mechanism awaiting a value.
    //   Same family as the cube populations' hygiene rows
    //   (cube_behaviors.hpp). Defensive declaration.
};

// ── Color Modes ──────────────────────────────────────────────────
struct GoLColorMode {
    static constexpr uint32_t NEUTRAL = 0;  // no color change (height-only extrusion)
    static constexpr uint32_t LENS = 1;  // shift toward per-zone target color
    static constexpr uint32_t BLACKISH = 2;  // darken toward near-black
    static constexpr uint32_t COUNT = 3;

    static constexpr float WEIGHTS_HEIGHT[COUNT] = { 0.30f, 0.40f, 0.30f };
    static constexpr float WEIGHTS_NO_HEIGHT[COUNT] = { 0.00f, 0.55f, 0.45f };
};

// ═══ THE TICK LADDER (GOL_TEMPO_2) ═══════════════════════════════
//
// Every GoL tick period is a NOTE VALUE. Binary note values and their
// dotted forms, in beats: twelve rungs, {1, 1.5} x 2^n. Fully
// commensurate — every rung divides 96 beats, so the whole board
// realigns every 24 bars of 4/4. The bottom rung is the hard floor of
// the world; the top caps Monolith's +3σ tail at 8 bars.
//
// This governs the tick_period column of BOTH tier tables below.
inline constexpr float GOL_TICK_LADDER[12] =
    { 0.75f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f,
      12.0f, 16.0f, 24.0f, 32.0f };

// CPU-ONLY — THE ONE-AUTHOR LAW. The GPU receives the quantized value
// on the derive request and draws nothing. sample_gaussian runs log and
// cos, and the WGSL spec licenses per-backend accuracy for both, so a
// GPU twin of this snapper would put one draw on two different rungs
// either side of a boundary — a permanent duty desync, different per
// backend and unreproducible across machines. One draw, one number.
//
// Nearest rung in LOG space: the boundary between lad[i] and lad[i+1]
// is their geometric mean, compared as x*x < lad[i]*lad[i+1] — two f32
// products, no transcendentals. Every boundary product (0.75, 1.5, 3,
// 6, 12, 24, 48, 96, 192, 384, 768) is exactly representable, so the
// comparison is exact and the census's Python port cannot disagree
// about membership. Below the bottom rung and above the top the ladder
// clamps — it IS the floor that retired max(0.1f, ...).
inline float quantize_tick_period(float x) {
    for (int i = 0; i < 11; i++)
        if (x * x < GOL_TICK_LADDER[i] * GOL_TICK_LADDER[i + 1])
            return GOL_TICK_LADDER[i];
    return GOL_TICK_LADDER[11];
}


// ═══ CONWAY ALGORITHM ════════════════════════════════════════════

// ── Tier Profile (mean+sigma, matches ColumnTierParams pattern) ──
inline constexpr uint32_t GOL_TIER_COUNT = 10;

struct GoLTierProfile {
    // ─── Rule ────────────────────────────────────────────────
    // Conway B/S as a bitset: bit n = birth on n neighbours, bit 9+n =
    // survival on n. B3/S23 is 0x1808u. L3 MIRROR: world.wgsl GOL_TIERS.
    uint32_t rule_mask;

    // ─── Initial conditions ──────────────────────────────────
    float density_mean, density_sigma;

    // ─── Temporal ────────────────────────────────────────────
    float tick_period_mean, tick_period_sigma;

    // ─── Visual transition ───────────────────────────────────
    float transition_fraction_mean, transition_fraction_sigma;

    // ─── Height ──────────────────────────────────────────────
    float alive_height_mean, alive_height_sigma;

    // ─── Per-cell variation ──────────────────────────────────
    float spring_variance;     // [0,1] per-cell spring speed scatter

    // ─── Selection ───────────────────────────────────────────
    float weight;
    bool  force_no_height;

    // ─── Size (UNIFIED_GROUND_1 U5; cells, not world units) ──
    uint32_t grid_cells;       // zone side in cells ∈ {8..64} (capacity: Dim::GOL_ZONE_GRID)
};

// MUST match world.wgsl's GOL_TIERS cells column. Hardware mirror — when
// tuning, change both sides.
// PROVENANCE, no longer an invariant: the cells column was authored by
// UNIFIED_GROUND_1 U5 as "defaults by weight order thirds, 32/24/16", and
// at that time weight-descending order gave 32,32,24,24,16,16,16. TUNE_1
// A10 re-ranked the weights without re-authoring the cells (cells were not
// in its bind), so the order now reads 24,32,16,16,32,16,24. The column is
// Jean-tunable per row and its VALUES are unchanged; only the descending-
// rank pattern is gone. Re-author the cells if the pattern is wanted back.
// (GOL_RULES_1 reweighted the seven again and appended three rows, so the
//  seven-value list above describes the original rows only.)
// GOL_TEMPO_2 quantized the CPU draw to GOL_TICK_LADDER and re-authored
// four means onto rungs; A MEAN OFF THE LADDER AUTHORS A COIN, NOT A
// TEMPO — a mean sitting in a boundary band splits its zones ~50/50
// across two rungs, so the row has no tempo, it has two. Every mean in
// both tables is now a rung. Flash 0.5 -> 1.0, HighLife 1.2 -> 1.0,
// Cauldron 5.0 -> 4.0 (both rooms), Pulse Sparkle 1.0 -> 1.5.
//                                     rule       dens_μ   σ    tick_μ  σ    trans_μ  σ     ht_μ    σ    sv    wt    no_h   cells
inline constexpr GoLTierProfile GOL_TIERS[GOL_TIER_COUNT] = {
    /* 0: Pillars  */ { 0x1808u,  0.30f, 0.05f,  16.0f, 4.0f,   0.05f, 0.01f,  30.0f, 9.0f,  0.30f,  0.11f, false, 16u },
    /* 1: Sparse   */ { 0x1808u,  0.15f, 0.05f,   4.0f, 1.0f,   0.12f, 0.03f,  18.0f, 6.0f,  0.20f,  0.17f, false, 32u },
    /* 2: Moderate */ { 0x1808u,  0.30f, 0.08f,   2.0f, 0.6f,   0.15f, 0.03f,   9.0f, 3.0f,  0.15f,  0.09f, false, 32u },
    /* 3: Dense    */ { 0x1808u,  0.45f, 0.10f,   1.0f,  0.3f,   0.25f, 0.05f,   6.0f, 1.5f,  0.10f,  0.03f, false, 16u },
    /* 4: Flash    */ { 0x1808u,  0.35f, 0.10f,   1.0f,  0.2f,   0.30f, 0.05f,   0.0f, 0.0f,  0.40f,  0.03f, true,  24u },
    /* 5: Monolith */ { 0x1808u,  0.20f, 0.03f,  24.0f, 6.0f,  0.03f, 0.01f,  42.0f, 12.f,  0.05f,  0.12f, false, 16u },
    /* 6: Glacier  */ { 0x1808u,  0.12f, 0.03f,   8.0f, 2.0f,   0.08f, 0.02f,  24.0f, 7.5f,  0.25f,  0.21f, false, 24u },
    // GOL_TEMPO_1 doubled tick_period_mean and _sigma on EVERY row of both
    // tables — the whole board at half rate. It is one column because
    // transition_fraction is a FRACTION: the extrusion lasts
    // transition_fraction x tick_period, so doubling the period doubles
    // the update interval and the rise-and-sink time together and leaves
    // every row's duty cycle where its author put it. Do not reach for
    // config.mode_gol_tick_scale to do this: that dial is read only by
    // pulse_cell_target, so it would slow the Pulse fields and leave both
    // Conway's tick gate and every spring at speed.
    //
    // GOL_RULES_1 authored these three rules. GOL_ROWS_1 re-authored two
    // of the row VALUES from the headless witness, which overturned the
    // belief they had been tuned around: Cauldron never terminates and
    // Day & night did — the opposite of the assumption. GOL_ROWS_2 then
    // retimed Cauldron for a boil, and GOL_ROWS_3 replaced Day & night's
    // MASK outright: it reached one uniform extreme or the other too
    // often for the slot, and a majority rule fills that slot without the
    // failure mode. Every number below comes from tools/gol_census.py.
    // Read them as intent:
    //  · Plateau is the majority rule (Vote, B5678/S45678): a cell takes
    //    the state most of its neighbourhood is already in. What matters
    //    here is that it PINS at its interfaces rather than coarsening
    //    all the way to consensus — across a straight edge the live cell
    //    sees exactly 5 and survives, the dead cell sees 3 and is not
    //    born, so the boundary is itself a fixed point. Domains smooth
    //    their edges and then stop, and neither extreme is reachable from
    //    a 0.50 seed. Censused at these values over 32 zone seeds: 0/32
    //    dark, 0/32 saturated, 32/32 structured, 32/32 reaching a true
    //    fixed point, ~43% live; over 512 seeds the dark rate is 0.4%.
    //    Dark is the failure the census watches — such a zone renders
    //    nothing (no height, and the tint's color_val > 0.01 fails) while
    //    still holding its footprint against every other zone. This slot
    //    held Day & night twice before, at 0.50 and again biased to 0.58,
    //    and reached one uniform extreme or the other about a third and a
    //    fifth of the time. Those numbers, and the next candidate's, come
    //    from tools/gol_census.py. The treatment was authored for a
    //    terminal, structured, tall row, and this is the first mask that
    //    is one: a slow tick with a crisp rise inside it
    //    (omega = 3 / (0.10 x 8.0) = 3.75) and low variance. Plateaus that
    //    commit and then hold.
    //  · Cauldron, once "Walled cities", builds no walls at any reachable
    //    size: 0 of 32 seeds reached a fixed point in 4000 generations and
    //    not one cell was static, at every size 24..128 and every density
    //    0.25..0.65. It is a dense boil at ~53% live, and it is named for
    //    what it does. Mask, density and cells are the rule's own
    //    requirement and stay. LOW height is what makes a permanently
    //    churning field a textured surface instead of a strobing forest.
    //    The transition 0.40 is the load-bearing number: the cell spends
    //    about two fifths of its tick in transit, so the field shimmers
    //    continuously instead of stepping. THE TICK HAS BEEN BOTH WAYS.
    //    GOL_ROWS_2 took it 6.0 -> 2.5 because at 120bpm 6.0 was three
    //    seconds a generation, slower than a boil can be seen to be;
    //    GOL_TEMPO_1 then halved the whole board's rate and carried this
    //    row with it, 2.5 -> 5.0. GOL_TEMPO_2 put it on the ladder at 4.0
    //    (omega = 3 / (0.40 x 4.0) = 1.875), which is 2.4 seconds a
    //    generation at 100bpm. THE SNAP WAS NOT NEAREST: 5.0 sat almost
    //    exactly ON the 4/6 boundary (sqrt(24) = 4.899), so the row was a
    //    coin, and nearest would have returned it to the 6.0 the argument
    //    above rejected. The row's own history argues downward, and 4.0 is
    //    the rung that keeps it. If the boil stops reading as one, this row
    //    is the first place to look and 3.0 is the rung below.
    //  · HighLife must be 32 cells or the replicator has no room and the
    //    row reads as thin Conway. Brisk tick so replication is visible —
    //    and GOL_TEMPO_2 made brisk mean THE QUARTER NOTE. The old 1.2 sat
    //    a hair under the 1.0/1.5 boundary (sqrt(1.5) = 1.2247), close
    //    enough that the row's own sigma made the rung a coin flip; 1.0 is
    //    the rung it was always reaching for.
    //    Untouched by GOL_ROWS_1 — the witness ran Hickerson's replicator
    //    on this row's own grid and watched one 12-cell seed become two.
    /* 7: Plateau  */ { 0x3E1E0u, 0.50f, 0.06f,   8.0f, 2.0f,   0.10f, 0.02f,  30.0f, 8.0f,  0.08f,  0.09f, false, 32u },
    /* 8: Cauldron */ { 0x79F0u,  0.50f, 0.05f,   4.0f, 1.0f,   0.40f, 0.08f,   5.0f, 1.5f,  0.15f,  0.08f, false, 24u },
    /* 9: HighLife */ { 0x1848u,  0.30f, 0.05f,   1.0f,  0.3f,   0.20f, 0.04f,  10.0f, 3.0f,  0.22f,  0.07f, false, 32u },
};

inline constexpr const char* GOL_TIER_NAMES[] = {
    "Pillars", "Sparse", "Moderate", "Dense",
    "Flash", "Monolith", "Glacier",
    "Plateau", "Cauldron", "HighLife"
};

inline constexpr const char* GOL_COLOR_NAMES[] = {
    "neutral", "lens", "blackish"
};

// ═══ PULSE ALGORITHM ═════════════════════════════════════════════

// ── Property Indices for Pulse-specific parameters ───────────────
struct PulseZoneProp {
    static constexpr uint32_t ALGORITHM_ROLL = 950u;
    static constexpr uint32_t PULSE_TIER = 951u;
    static constexpr uint32_t PHASE_RANDOM = 952u;
    static constexpr uint32_t WANDER = 953u;
    static constexpr uint32_t TEMPO_RANDOM = 954u;
};

// ── Pulse Tier Profile ───────────────────────────────────────────
inline constexpr uint32_t GOL_PULSE_TIER_COUNT = 4;

struct GolPulseTierProfile {
    // ─── Field ───────────────────────────────────────────────
    uint32_t field_fn;         // PulseField:: — which law writes the target

    // ─── Temporal ────────────────────────────────────────────
    float tick_period_mean, tick_period_sigma;

    // ─── Visual transition ───────────────────────────────────
    float transition_fraction_mean, transition_fraction_sigma;

    // ─── Phase scatter ───────────────────────────────────────
    float phase_randomness_mean, phase_randomness_sigma;

    // ─── Tempo scatter ───────────────────────────────────────
    // GOL_TEMPO_2 U3 ZEROED THIS COLUMN ON EVERY ROW. It is the one
    // variance that breaks musical time PER CELL: tempo scatter is a
    // per-cell FREQUENCY multiplier, so each cell drifts off the ladder
    // continuously and its phase error integrates in t_beats without
    // bound. The Spiral row measured it and won the argument (see its
    // comment): 0.99 correlation against a scatter-free field at 20
    // beats, 0.66 at 75, 0.02 by 150 — coherence gone inside a minute.
    // A ladder that every zone leaves per cell is not a ladder.
    // The column is still LIVE and still read (pulse_cell_target's
    // tempo_jitter) — it is zero by ruling, not dead. phase_randomness
    // beside it is untouched: a bounded static offset is texture, not
    // broken time, which is why Sparkle keeps its 0.90 and stays a
    // sparkle. Cost, knowingly paid: Sparkle and Drift lose their
    // per-cell tempo shimmer.
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

    // ─── Size (UNIFIED_GROUND_1 U5; cells, not world units) ──
    uint32_t grid_cells;       // zone side in cells ∈ {8..64} (capacity: Dim::GOL_ZONE_GRID)
};

// MUST match world.wgsl's GOL_PULSE_TIERS cells column
// (UNIFIED_GROUND_1 U5 — "32/16/8 by weight order"). Hardware
// mirror — when tuning, change both sides.
// GOL_TEMPO_2 quantized the CPU draw to GOL_TICK_LADDER and re-authored
// four means onto rungs; A MEAN OFF THE LADDER AUTHORS A COIN, NOT A
// TEMPO — a mean sitting in a boundary band splits its zones ~50/50
// across two rungs, so the row has no tempo, it has two. Every mean in
// both tables is now a rung. Flash 0.5 -> 1.0, HighLife 1.2 -> 1.0,
// Cauldron 5.0 -> 4.0 (both rooms), Pulse Sparkle 1.0 -> 1.5.
//                                     field                  tick_μ   σ    trans_μ  σ    phase_μ  σ    tempo_μ σ    ht_μ   σ    wand_μ  σ    sv    wt    no_h  bnd                    cells
inline constexpr GolPulseTierProfile GOL_PULSE_TIERS[GOL_PULSE_TIER_COUNT] = {
    /* 0: Breathe  */ { PulseField::BREATH,  4.0f, 1.0f,   0.20f, 0.05f,   0.15f, 0.05f,    0.0f,  0.0f,   2.0f, 0.8f,  10.0f, 3.0f,   0.20f,  0.38f, false, BoundaryMode::REFLECT, 32u },
    /* 1: Sparkle  */ { PulseField::BREATH,  1.5f,  0.4f,   0.25f, 0.05f,   0.90f, 0.10f,    0.0f,  0.0f,   0.0f, 0.0f,   5.0f, 2.0f,   0.50f,  0.24f, true,  BoundaryMode::REFLECT, 16u },
    /* 2: Drift    */ { PulseField::BREATH,  8.0f, 2.0f,   0.10f, 0.03f,   0.50f, 0.15f,    0.0f,  0.0f,   4.0f, 1.5f,  25.0f, 8.0f,   0.35f,  0.20f, false, BoundaryMode::WRAP, 8u },
    // GOL_RULES_1. The first Pulse row that is not BREATH, and the first
    // whose target is continuous rather than binary. Read the values as
    // intent:
    //  · the continuous target is TRACKED rather than smeared by
    //    omega = 3 / (0.30 x 6.0) = 1.67. transition_fraction x
    //    tick_period IS the spring, so GOL_TEMPO_1's doubling of the tick
    //    halved the rotation AND halved the rise in one edit — which is
    //    why the fraction was the column that must not move. The
    //    spring_stiffness column that used to sit beside it was written
    //    and never read; GOL_TEMPO_2 U5 collected that docket and deleted
    //    it from all three rooms, so the fraction is now the only spring
    //    dial the row has;
    //  · phase scatter stays at 0.03; TEMPO SCATTER IS ZERO (GOL_ROWS_1).
    //    The two are not the same kind of term: tempo is a per-cell
    //    FREQUENCY multiplier, so its phase error integrates in t_beats
    //    and never saturates, while phase is a bounded static offset. At
    //    tempo 0.02 the arms measured 0.99 correlation against a
    //    scatter-free spiral at 20 beats, 0.66 at 75, and 0.02 by 150 —
    //    gone inside a minute at 120bpm. (Those beat counts were measured
    //    at the then-current 3.0 tick; the decoherence scales with the
    //    period, so at 6.0 the same collapse takes twice as long. The
    //    value is 0 and none of it happens.) Coherence is the point, and
    //    only one of the two scatters could ever keep it;
    //  · wander_radius 0 — the spiral centre IS the zone centre and does
    //    not move; a wandering centre smears the arms;
    //  · 32 cells, because a spiral does not read at 16;
    //  · force_no_height, which is also the safety of the fractional
    //    target: select_gol_zone forces height_enabled = false for such
    //    rows, so height never reads a fractional visual. Tint does, and
    //    that is the intent.
    /* 3: Spiral   */ { PulseField::SPIRAL,  6.0f, 1.6f,   0.30f, 0.06f,   0.03f, 0.01f,    0.0f, 0.0f,   0.0f, 0.0f,   0.0f, 0.0f,   0.10f,  0.18f, true,  BoundaryMode::WRAP, 32u },
};

inline constexpr const char* GOL_PULSE_TIER_NAMES[] = {
    "Breathe", "Sparkle", "Drift", "Spiral"
};

// ── The tier's zone size (UNIFIED_GROUND_1 U5) ───────────────────
// tier_idx is the COMPOUND index select_gol_zone packs: 0..
// GOL_TIER_COUNT-1 are Conway rows, GOL_TIER_COUNT.. are Pulse. This
// is the only place that decode lives.
//
// Two values for one square: cells_x == cells_z until S3 splits the
// grid, and writing it as a pair now means S3 touches nothing here.
// The GPU derives the same number in zone_derive_params from the same
// tables — this is the CPU half of that twin, not a second opinion.
inline void gol_tier_cells(uint32_t tier_idx, uint32_t& cells_x, uint32_t& cells_z) {
    const uint32_t n = (tier_idx < GOL_TIER_COUNT)
        ? GOL_TIERS[tier_idx].grid_cells
        : GOL_PULSE_TIERS[tier_idx - GOL_TIER_COUNT].grid_cells;
    cells_x = n;
    cells_z = n;
}

// The tier's NAME, off the same compound decode and living beside it so
// there is still one decode home, not two. GOL_TIER_NAMES and
// GOL_PULSE_TIER_NAMES were declared-and-unread until the [GoL] spawn
// log took them; this is the reader that makes them live.
inline const char* gol_tier_name(uint32_t tier_idx) {
    return (tier_idx < GOL_TIER_COUNT)
        ? GOL_TIER_NAMES[tier_idx]
        : GOL_PULSE_TIER_NAMES[tier_idx - GOL_TIER_COUNT];
}

inline void gol_tier_extent(uint32_t tier_idx, float& extent_x, float& extent_z) {
    uint32_t cx = 0u, cz = 0u;
    gol_tier_cells(tier_idx, cx, cz);
    extent_x = (float)cx * Dim::PATCH_CELL_SIZE;
    extent_z = (float)cz * Dim::PATCH_CELL_SIZE;
}

// ═══ SPAWN PAYLOADS — AT THE CONTRACT HOME ═══════════════════════
//
// The GoL Selection/Placement DTOs live in entity_types.hpp,
// beside the EntityQueueEntry / PlacementEntry unions that are their
// reason to exist: a DTO that exists to cross a boundary belongs to
// the boundary's contract, not to either side.

// ═══ RUNTIME CPU STATE ═══════════════════════════════════════════

// ── Per-instance zone state ──────────────────────────────────────
struct GoLZoneState {
    int32_t zone_nx = 0, zone_nz = 0;
    int32_t host_gx = 0, host_gz = 0;   // host patch (for entity_refs eviction)
    // ECONOMY_1 E1 rev2 — the zone's WORLD FOOTPRINT, persisted at
    // commit so the CPU can answer "does this zone reach the LOD0
    // core?" without the GPU. Authored once from the same corner +
    // tier extent the derive request carries; the GPU's
    // zone_derive_params re-derives the identical rectangle.
    float corner_x = 0.0f, corner_z = 0.0f;
    float extent_x = 0.0f, extent_z = 0.0f;
    bool active = false;
    uint32_t algorithm = AlgorithmType::CONWAY;
    // THE CPU AUTHORS THIS, for both rooms (GOL_TEMPO_2 U1). It is not
    // a copy that happens to match the GPU's — it is the number, drawn
    // and snapped to GOL_TICK_LADDER once, read here for the tick mask
    // and carried on the derive request for omega and the pulse phase.
    float tick_period = 1.0f;
    float initial_density = 0.3f;    // CPU needs this for life buffer seeding
    int32_t last_tick_index = -1;
};

// ── GoL module state ──────────────────────────────────────────
struct GoLState {
    GoLZoneState zones[Dim::MAX_GOL_ZONES]{};
    uint32_t     zone_count = 0;
    uint32_t     active_slot_count = 0;     // highest active slot + 1 (for dispatch sizing)

    bool         mood_allowed = true;

    // Derive request queue: accumulated during patch gen, flushed once
    // per frame as a single GPU compute dispatch (zone_derive_params).
    GPUZoneDeriveRequestArray pending_derive_requests{};
};

// ═══ MODULE FUNCTIONS — DECLARATIONS ═════════════════════════════

// Lifecycle (three-phase + helper)
bool select_gol_for_patch(GoLState& gs, MachineCtx* c,
    int32_t gx, int32_t gz, GoLSelection& sel);
bool place_gol_from_selection(MachineCtx* c,
    const GoLSelection& sel, GoLPlacement& plan);
void commit_gol(GoLState& gs, MachineCtx* c,
    const GoLPlacement& plan,
    int32_t trigger_gx, int32_t trigger_gz, wgpu::Queue& queue);
// The evictor — MachineCtx-shaped
// to match the FAMILY_DISPATCH evict slot (table in cartridge.hpp, post-class)
void evict_gol(MachineCtx* self, uint32_t slot, wgpu::Queue& queue);
// Dispatch funnels (table-shaped; the FAMILY_DISPATCH rows point here)
bool dispatch_select_gol(MachineCtx* self, int32_t gx, int32_t gz, EntityQueueEntry& e);
bool dispatch_place_gol(MachineCtx* self, EntityQueueEntry& e, PlacementEntry& pe);
void dispatch_commit_gol(MachineCtx* self, PlacementEntry& pe, wgpu::Queue& queue);
void seed_gol_zone(GoLState& gs, MachineCtx* c,
    uint32_t slot, wgpu::Queue& queue);
// Per-frame
void upload_gol_zone_config(GoLState& gs, GolDeps* c, wgpu::Queue& queue);
void flush_zone_derive_requests(GoLState& gs, GolDeps* c, wgpu::Queue& queue);
void teardown_gol(GoLState& gs, GolDeps* c, wgpu::Queue& queue);
void dispatch_zone_sync(GoLState& gs, GolDeps* c, wgpu::CommandEncoder& encoder);
void dispatch_zone_evolve(GoLState& gs, GolDeps* c, wgpu::CommandEncoder& encoder);

// ═══ IMPL:
// rows deref gol_state_(own) + mood/world/time + tile faces via MachineCtx;
// score-verbs deref gpu/renderer/device/time via GolDeps (S5 device).
// COHORT: after renderer (Renderer) + entity_pipeline/spawn_engine (funnels,
// footprints) + patch_system (find_patch) + tile_world (faces) + state.

// ═══ LIFECYCLE — three-phase + helper ════════════════════════════

// ─── select_gol_for_patch ─────────────────────────────────────

inline bool select_gol_for_patch(GoLState& gs, MachineCtx* c,
    int32_t gx, int32_t gz, GoLSelection& sel) {
    // THE COMPOSITION LAW: the shared stack — mood (explicit veto)
    // → global → tile (F3); proximity OFF (GoL's affinity row is zero);
    // clamp [0,1]. The per-lattice-node roll stays below (its own seed
    // domain, cpu_lattice_node_seed — a consumer fact, not the law's).
    auto composed = compose_spawn_chance(c, gx, gz, PopFamily::GOL,
        GoLZoneSpawnConfig::SPAWN_CHANCE, mood_mult_for(PopFamily::GOL),
        /*use_proximity=*/false, /*veto_on_zero_mood=*/true,
        SpawnClamp::RANGE01);
    if (composed.vetoed) return false;

    // Scan lattice nodes overlapping this patch
    float wx0 = gx * Dim::PATCH_EXTENT;
    float wx1 = (gx + 1) * Dim::PATCH_EXTENT;
    float wz0 = gz * Dim::PATCH_EXTENT;
    float wz1 = (gz + 1) * Dim::PATCH_EXTENT;

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
            int32_t auth_gx = (int32_t)std::floor(raw_cx / Dim::PATCH_EXTENT);
            int32_t auth_gz = (int32_t)std::floor(raw_cz / Dim::PATCH_EXTENT);
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

            // Spawn roll (the chance arrived composed — loop-invariant)
            uint32_t seed = cpu_lattice_node_seed(c->world_state_.active_seed, nx, nz, GoLZoneProp::SEED_BAND);
            float roll = cpu_hash_f(seed, GoLZoneProp::SPAWN_ROLL);
            if (roll >= composed.chance) continue;

            // Find free slot
            uint32_t slot = UINT32_MAX;
            for (uint32_t i = 0; i < Dim::MAX_GOL_ZONES; i++) {
                if (!gs.zones[i].active) { slot = i; break; }
            }
            if (slot == UINT32_MAX) continue;

            // Reserve slot
            gs.zones[slot].active = true;

            // (The corner snap moved BELOW tier selection — U5 made the
            //  extent tier-derived, so the corner cannot be known until
            //  the tier is. zone_derive_params made the same move on the
            //  GPU side; this is the CPU catching up.)

            // Algorithm selection
            float algo_roll = cpu_hash_f(seed, PulseZoneProp::ALGORITHM_ROLL);
            uint32_t algorithm = (algo_roll < GOL_PULSE_ALGORITHM_CHANCE)
                ? AlgorithmType::PULSE : AlgorithmType::CONWAY;

            // Height enabled
            float height_roll = cpu_hash_f(seed, GoLZoneProp::HEIGHT_ROLL);
            bool height_enabled = (height_roll < GoLZoneSpawnConfig::HEIGHT_CHANCE);

            // Tier + CPU-side params
            float tick_period = 1.0f;
            float initial_density = 0.0f;
            uint32_t tier_idx = 0;

            if (algorithm == AlgorithmType::CONWAY) {
                float w[GOL_TIER_COUNT];
                for (uint32_t t = 0; t < GOL_TIER_COUNT; t++) w[t] = GOL_TIERS[t].weight;
                uint32_t tier = select_tier(seed, GoLZoneProp::TIER, w, GOL_TIER_COUNT);
                const auto& tp = GOL_TIERS[tier];
                if (tp.force_no_height) height_enabled = false;
                tick_period = quantize_tick_period(
                    cpu_sample_gaussian(seed, GoLZoneProp::TICK_PERIOD,
                        tp.tick_period_mean, tp.tick_period_sigma));
                initial_density = std::max(0.05f, std::min(0.9f,
                    cpu_sample_gaussian(seed, GoLZoneProp::DENSITY,
                        tp.density_mean, tp.density_sigma)));
                tier_idx = tier;  // Conway: 0 .. GOL_TIER_COUNT - 1
            }
            else {
                float w[GOL_PULSE_TIER_COUNT];
                for (uint32_t t = 0; t < GOL_PULSE_TIER_COUNT; t++) w[t] = GOL_PULSE_TIERS[t].weight;
                uint32_t tier = select_tier(seed, PulseZoneProp::PULSE_TIER, w, GOL_PULSE_TIER_COUNT);
                const auto& pp = GOL_PULSE_TIERS[tier];
                if (pp.force_no_height) height_enabled = false;
                tick_period = quantize_tick_period(
                    cpu_sample_gaussian(seed, GoLZoneProp::TICK_PERIOD,
                        pp.tick_period_mean, pp.tick_period_sigma));
                initial_density = 0.0f;
                // Compound index: GOL_TIER_COUNT .. GOL_TIER_COUNT +
                // GOL_PULSE_TIER_COUNT - 1. Named, never a literal — the
                // Conway count moves and every Pulse index moves with it.
                tier_idx = GOL_TIER_COUNT + tier;
            }

            // Zone extent + corner (cell-grid-snapped), from the tier's
            // own size. Snapping subtracts an exact multiple of
            // Dim::PATCH_CELL_SIZE, so corner + extent/2 returns the snapped
            // raw centre for every tier — the same identity the GPU's
            // zone_derive_params relies on, which is why the centre was
            // right even while the extent was a fixed 100.
            float extent_x = 0.0f, extent_z = 0.0f;
            gol_tier_extent(tier_idx, extent_x, extent_z);

            float corner_x = std::floor(
                (raw_cx - extent_x * 0.5f) / Dim::PATCH_CELL_SIZE) * Dim::PATCH_CELL_SIZE;
            float corner_z = std::floor(
                (raw_cz - extent_z * 0.5f) / Dim::PATCH_CELL_SIZE) * Dim::PATCH_CELL_SIZE;

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
            // The CIRCUMSCRIBED radius of the tier's rectangle, not the
            // inscribed one. The registry must never PERMIT an overlap:
            // two GoL zones that overlap would let contrib_gol_zones_at
            // (returns on its first covering zone) and the tint (breaks
            // on its first) disagree about which zone owns a cell, with
            // no defined order and unequal filters. Conservative here
            // costs a little density; inscribed would allow corner
            // overlap, and the old fixed 50 allowed 80 wu of it at 64
            // cells.
            sel.footprint_r = 0.5f * std::hypot(extent_x, extent_z);

            return true;  // at most one zone per patch
        }
    }
    return false;
}

// ─── place_gol_from_selection ─────────────────────────────────

inline bool place_gol_from_selection(MachineCtx* c,
    const GoLSelection& sel, GoLPlacement& plan) {
    float extent_x = 0.0f, extent_z = 0.0f;
    gol_tier_extent(sel.tier_idx, extent_x, extent_z);
    float cx = sel.corner_x + extent_x * 0.5f;
    float cz = sel.corner_z + extent_z * 0.5f;

    if (!check_position(c, cx, cz, sel.footprint_r, PopFamily::GOL))
        return false;

    int32_t host_gx = (int32_t)std::floor(cx / Dim::PATCH_EXTENT);
    int32_t host_gz = (int32_t)std::floor(cz / Dim::PATCH_EXTENT);

    if (register_footprint(c, cx, cz, sel.footprint_r,
        host_gx, host_gz, PopFamily::GOL, sel.slot, sel.tier_idx) == UINT32_MAX)
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

    return true;
}

// ─── commit_gol ──────────────────────────────────────────────

inline void commit_gol(GoLState& gs, MachineCtx* c,
    const GoLPlacement& plan,
    int32_t trigger_gx, int32_t trigger_gz, wgpu::Queue& queue)
{
    (void)trigger_gx; (void)trigger_gz;
    auto& zone = gs.zones[plan.slot];
    zone.zone_nx = plan.zone_nx;
    zone.zone_nz = plan.zone_nz;
    zone.host_gx = plan.host_gx;
    zone.host_gz = plan.host_gz;
    zone.corner_x = plan.corner_x;
    zone.corner_z = plan.corner_z;
    gol_tier_extent(plan.tier_idx, zone.extent_x, zone.extent_z);
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
        // THE ONE-AUTHOR LAW: the tick crosses the seam as a VALUE,
        // already drawn and already snapped to GOL_TICK_LADDER. The
        // GPU no longer re-derives it, so gate, omega and pulse phase
        // read literally the same number.
        req.tick_period = plan.tick_period;
    }

    // A BIRTH ANNOUNCEMENT ON THE SPAWN PATH (PURSE_0 R3). Unconditional
    // tail of commit_gol: no change detector, no error guard — it narrates
    // a SUCCESS, which means it prints when everything is right, which is
    // the opposite standing from the correctness witnesses the dial keeps.
    //
    // It is reachable in steady state, and that is the whole finding: the
    // caller chain is commit_gol <- dispatch_commit_gol <- commit_entity_queue
    // <- spawn_selected_patches <- the per-frame distance-driven spawn block,
    // which drains up to SPAWN_BUDGET_PER_FRAME allocated patches EVERY
    // frame. On an ever-expanding board patches are continuously allocated,
    // so this is a rider's chatter, not only a birth. Same flag and same
    // reason as `[Gallery] slot=` and `[Ribbon] SPAWN/EVICT`.
    if constexpr (t7::INSTRUMENTS.stream_witness) {
        std::cout << "[GoL] "
            << (plan.algorithm == AlgorithmType::PULSE ? "Pulse" : "Conway")
            << " tier=" << gol_tier_name(plan.tier_idx)
            << " slot=" << plan.slot
            << " node=(" << plan.zone_nx << "," << plan.zone_nz << ")"
            << " corner=(" << plan.corner_x << "," << plan.corner_z << ")"
            << " host=(" << plan.host_gx << "," << plan.host_gz << ")"
            << (plan.height_enabled ? " HEIGHT" : "")
            << " period=" << plan.tick_period
            << "\n";
    }
}

// ─── seed_gol_zone ───────────────────────────────────────────

inline void seed_gol_zone(GoLState& gs, MachineCtx* c,
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

    // Generate per-cell height factors: Gaussian draw, clamped.
    // (UNIFIED_GROUND_1 U5: the GPU mask multiplies at birth
    //  (zone_seed_mask) — smooth ground does not extrude, lift, or
    //  carry walker height; STATIC at birth (the dynamic, tide-
    //  following form is Layer E — campaign v2 §9).)
    std::vector<float> height_factors(Dim::GOL_ZONE_CELLS);
    for (uint32_t i = 0; i < Dim::GOL_ZONE_CELLS; i++) {
        float hf = cpu_sample_gaussian(seed + i, GoLZoneProp::HEIGHT_FACTOR,
            GoLZoneSpawnConfig::HEIGHT_FACTOR_MEAN, GoLZoneSpawnConfig::HEIGHT_FACTOR_SIGMA);
        height_factors[i] = std::max(GoLZoneSpawnConfig::HEIGHT_FACTOR_CLAMP_LO,
            std::min(GoLZoneSpawnConfig::HEIGHT_FACTOR_CLAMP_HI, hf));
    }

    // Upload all life slots
    c->gpuState_.upload_zone_life(queue, slot, life.data(), height_factors.data(), Dim::GOL_ZONE_CELLS);
}

// ═══ PER-FRAME UPLOAD ════════════════════════════════════════════

inline void upload_gol_zone_config(GoLState& gs, GolDeps* c, wgpu::Queue& queue) {
    uint32_t count = 0;
    uint32_t tick_mask = 0;

    for (uint32_t i = 0; i < Dim::MAX_GOL_ZONES; i++) {
        if (!gs.zones[i].active) continue;

        // Conway tick gating: exactly one tick per period.
        // No floor on the divisor. The bound is STRUCTURAL: the only
        // author of this field snaps it to GOL_TICK_LADDER, whose
        // bottom rung is 0.75 beats, so the divisor cannot approach
        // zero and the guard that used to say so had nothing left to
        // guard (GOL_TEMPO_2 U1).
        int32_t current_tick = (int32_t)std::floor(
            c->time_state_.beats / gs.zones[i].tick_period);
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
inline void flush_zone_derive_requests(GoLState& gs, GolDeps* c, wgpu::Queue& queue) {
    if (gs.pending_derive_requests.count == 0) return;

    c->gpuState_.upload_zone_derive_requests(queue, gs.pending_derive_requests);

    // DOMESDAY_1 A9 (label law): labels are emitted where objects are
    // created, from the creating function's name.
    wgpu::CommandEncoderDescriptor encDesc{};
    encDesc.label = "flush_zone_derive_requests";
    wgpu::CommandEncoder encoder = c->device_.CreateCommandEncoder(&encDesc);
    wgpu::ComputePassDescriptor desc{};
    desc.label = "Zone Derive Params";
    // The hidden submit's own pass still meters: this encoder submits
    // BEFORE the host encoder, so its writes land ahead of the frame-close
    // resolve in queue order.
    desc.timestampWrites = c->gpuState_.meter_arm_compute(meter_row::GolDeriveFlush);
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&desc);
    // LOOM_2 pass head: WORLD + FRAME are every pipeline's strata 0/1.
    { pass.SetBindGroup(0, c->gpuState_.world_group());
      pass.SetBindGroup(1, c->gpuState_.frame_c_group()); }
    c->renderer_.dispatch_zone_derive_params(
        pass,
        c->gpuState_.zones_state_group(), c->gpuState_.zones_textures_group(),
        gs.pending_derive_requests.count);
    // ORDERING LAW: derive writes zone_config[slot]; the mask reads it —
    // sequential dispatches in one pass suffice (storage-buffer
    // visibility between dispatches is guaranteed). (UNIFIED_GROUND_1 U5)
    c->renderer_.dispatch_zone_seed_mask(
        pass,
        c->gpuState_.zones_state_group(), c->gpuState_.zones_textures_group(),
        gs.pending_derive_requests.count);
    pass.End();
    wgpu::CommandBuffer cmd = encoder.Finish();
    queue.Submit(1, &cmd);

    gs.pending_derive_requests.count = 0;
}

// ═══ DISPATCH FUNNELS (table-shaped; declared in entity_types.hpp) ═

inline bool dispatch_select_gol(MachineCtx* self,
    int32_t gx, int32_t gz, EntityQueueEntry& e) {
    if (!self->gol_state_.mood_allowed) { return false; }   // mood gate — no new zones
    return select_gol_for_patch(self->gol_state_, self, gx, gz, e.gol);
}

inline bool dispatch_place_gol(MachineCtx* self,
    EntityQueueEntry& e, PlacementEntry& pe) {
    pe.family = e.family; pe.gx = e.gx; pe.gz = e.gz;
    if (place_gol_from_selection(self, e.gol, pe.gol)) {
        return true;
    }
    else {
        self->gol_state_.zones[e.gol.slot].active = false;
        return false;
    }
}

inline void dispatch_commit_gol(MachineCtx* self,
    PlacementEntry& pe, wgpu::Queue& queue) {
    auto* host = find_patch(self, pe.gol.host_gx, pe.gol.host_gz);
    if (host) {
        commit_gol(self->gol_state_, self, pe.gol, pe.gx, pe.gz, queue);
        host->record_entity(PopFamily::GOL, pe.gol.slot);
    }
    else {
        // Host patch gone — release by owner (the patch key can never match).
        unregister_footprint_for(self, PopFamily::GOL, pe.gol.slot);
        self->gol_state_.zones[pe.gol.slot].active = false;
    }
}

// ═══ THE EVICTOR ══════════════════════════════════════════════════

inline void evict_gol(MachineCtx* self,
    uint32_t slot, wgpu::Queue& queue) {
    unregister_footprint_for(self, PopFamily::GOL, slot);   // the hand that claims is the hand that frees
    self->gpuState_.deactivate_zone_slot(queue, slot);
    self->gol_state_.zones[slot].active = false;
    self->gol_state_.zone_count--;
}


// ─── Teardown (owner verb) ────────────────────────────────────────
inline void teardown_gol(GoLState& gs, GolDeps* c, wgpu::Queue& queue) {
    // GoL zones (gs is the own organ, explicit; c is the external face)
    for (uint32_t i = 0; i < Dim::MAX_GOL_ZONES; i++) {
        gs.zones[i] = GoLZoneState{};
    }
    gs.zone_count = 0;
    gs.active_slot_count = 0;
    gs.pending_derive_requests.count = 0;
    GPUGoLZoneArray emptyZones{};
    c->gpuState_.upload_zone_config(queue, emptyZones);
}

// ─── Zone compute passes (owner verbs) ─
// derive params + sync + evolve + mesh, SEPARATE passes
// for the GPU barrier (O-6a). Callers order them sync -> evolve ->
// mesh after flush_zone_derive_requests + upload_gol_zone_config.
inline void dispatch_zone_sync(GoLState& gs, GolDeps* c, wgpu::CommandEncoder& encoder) {
    wgpu::ComputePassDescriptor cpd{};
    cpd.label = "GoL Zone Sync";
    cpd.timestampWrites = c->gpuState_.meter_arm_compute(meter_row::GolZoneCompute);
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&cpd);
    // LOOM_2 pass head: WORLD + FRAME are every pipeline's strata 0/1.
    { pass.SetBindGroup(0, c->gpuState_.world_group());
      pass.SetBindGroup(1, c->gpuState_.frame_c_group()); }
    c->renderer_.dispatch_zone_gol_sync(pass,
        c->gpuState_.zones_state_group(), c->gpuState_.zones_textures_group(), gs.active_slot_count);
    pass.End();
}

inline void dispatch_zone_evolve(GoLState& gs, GolDeps* c, wgpu::CommandEncoder& encoder) {
    wgpu::ComputePassDescriptor cpd{};
    cpd.label = "GoL Zone Evolve";
    cpd.timestampWrites = c->gpuState_.meter_arm_compute(meter_row::GolZoneCompute);
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&cpd);
    // LOOM_2 pass head: WORLD + FRAME are every pipeline's strata 0/1.
    { pass.SetBindGroup(0, c->gpuState_.world_group());
      pass.SetBindGroup(1, c->gpuState_.frame_c_group()); }
    c->renderer_.dispatch_zone_gol_evolve(pass,
        c->gpuState_.zones_state_group(), c->gpuState_.zones_textures_group(), gs.active_slot_count);
    pass.End();
}


} // namespace the_board
} // namespace t7
