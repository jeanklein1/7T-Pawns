#pragma once
#include <cstdint>
#include <cstddef>                                                   // offsetof (the F-5 positional pin below)
#include <cmath>                                                     // std::floor (theme lattice, Q6b)
#include "cartridges/the_board/contracts/roster.hpp"                // PopFamily (spawn_weight indexing)
#include "cartridges/the_board/contracts/mood_constants.hpp"        // MOOD_COUNT (the mood axis of MOOD_SPAWN_MULT)
#include "cartridges/the_board/primitives/seed_utils.hpp"    // cpu_hash_f (theme rolls)
// Nothing here names Cartridge; MachineCtx arrives from
// contracts/entity_types.hpp earlier in the cohort.

// ─── population_themes.hpp (S2 · HEADER: vocabulary + state + decls) ─
//
// Compositional intent per region: what spawns, how densely, in which
// tier mix — THEMES rows selected per patch by the envelope machine.
//
// The impl's only machine reach is the flag-gated census dump (MachineCtx).

namespace t7 {
namespace the_board {

inline constexpr float THEME_LATTICE_SPACING = 500.0f;
inline constexpr uint32_t THEME_SEED_BAND = 170u;
inline constexpr uint32_t THEME_COUNT = 5;
inline constexpr float THEME_BASE_WEIGHT = 10.0f;

// ═══ MOOD × FAMILY SPAWN MULTIPLIERS ═══════════════════════════════
// WHAT: the mood term of the composition law — presence × proportion
//   per family per mood. A live control surface: 0 means absent. The
//   veto path (veto_on_zero_mood) is live but the generic preamble
//   declines it and multiplies through instead — MIN1 clamps the top
//   only, so a zero survives the stack to the gate.
// AXES: row = mood id (mood_constants order, F-3 kin); column =
//   PopFamily order, PINNED by F-1 (roster.hpp).
// Frozen biography: values feed the spawn gates.
//
// RIBBON × the two indoor moods is this table's only departure from
// identity, and it is HELD, not ruled. Two faults, one cause: the
// miniature never sat inside the mood's clamps, and propagation_speed
// is authored in outdoor units/s, so shrinking the body raised the
// head's oscillation rate by 1/scale. The indoor treatment in
// bodies/ribbon.hpp stays, dormant behind these two cells. Raise them
// and both faults return with them.
//                              pyr   arch  col   ant   palm  cact  blade sph   rib   cube  gol   gal
inline constexpr float MOOD_SPAWN_MULT[MOOD_COUNT][PopFamily::COUNT] = {
    /* MOOD_OPEN_SUNSET     */ { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
    /* MOOD_INDOOR_FLAT     */ { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    /* MOOD_INDOOR_VAULT    */ { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    /* MOOD_FINITE_OUTDOOR  */ { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
    /* MOOD_OPEN_NIGHT      */ { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
    /* MOOD_OPEN_NOON       */ { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
    // THE ATRIUM IS AUTHORED, NOT ROLLED (ATRIUM_1). Every family's column
    // is 0: nothing the composition law would scatter belongs in the
    // entrance. Every one of the twelve reaches compose_spawn_chance
    // through mood_mult_for, and adj_mod = 0 survives the whole stack —
    // the clamps bound the top only — so this row silences all of them.
    /* MOOD_ATRIUM          */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
};

// The per-family column view, contiguous for the const float* funnels
// (the spawn gates index by mood id): a compile-time transpose — the
// matrix above stays the ONE authored home.
namespace detail {
struct MoodMultTransposed { float v[PopFamily::COUNT][MOOD_COUNT]; };
inline constexpr MoodMultTransposed build_mood_mult_transposed() {
    MoodMultTransposed r{};
    for (uint32_t fam = 0; fam < PopFamily::COUNT; fam++)
        for (uint32_t mood = 0; mood < MOOD_COUNT; mood++)
            r.v[fam][mood] = MOOD_SPAWN_MULT[mood][fam];
    return r;
}
inline constexpr MoodMultTransposed MOOD_MULT_T = build_mood_mult_transposed();
}  // namespace detail
inline constexpr const float* mood_mult_for(uint32_t family) {
    return detail::MOOD_MULT_T.v[family];
}

struct ThemeEnvelope {
    int32_t  active = -1;              // theme index, or -1 (no bias)
    uint32_t elapsed = 0;               // patches since this theme fired
    uint32_t cooldowns[THEME_COUNT]{};  // per-theme remaining cooldown
};

struct PopulationTheme {
    float spawn_weight[PopFamily::COUNT];          // multiplier on base spawn chance per family
    float tier_wt_pyramid[3];                      // multiplier on pyramid tier base weights
    float tier_wt_arch[3];                         // multiplier on arch tier base weights
    float tier_wt_column[3];                       // multiplier on column tier base weights (Pillar, Doric, Ornate)
    float tier_wt_antenna[3];                      // multiplier on antenna tier base weights (Antenna, Squat, Colossal)
    float tier_wt_palm[3];                         // multiplier on palm tier base weights (Sapling, Coastal, Royal)
    float tier_wt_cactus[3];                       // multiplier on cactus tier base weights (Finger, Saguaro, Candelabra)
    float tier_wt_blade[3];                        // multiplier on blade tier base weights (Sprout, Clump, Thicket)
    float tier_wt_sphere[2];                       // multiplier on sphere tier base weights (Sentinel, Anomaly)
    float tier_wt_ribbon[3];                       // multiplier on ribbon tier base weights (Serpentine, Helix, Streamer)
    float tier_wt_cube[4];                         // multiplier on cube tier base weights (SmCube..Monolith)

    // Envelope parameters (replace lattice weight for theme selection)
    float    spike;
    uint32_t sustain;       // patches at full spike
    uint32_t decay;         // patches for linear decay to base
    uint32_t cooldown;      // patches before re-eligible after expiry

    // Lattice node weight (spatial distribution of themes)
    float weight;
};

// F-5: the positional pin. THEMES rows below are positional aggregates;
// density_mult was CUT from between tier_wt_cube and spike, and a field
// silently reintroduced there would reshuffle every row's envelope
// numbers. The neighbors are pinned adjacent, and weight is pinned as
// the tail.
static_assert(offsetof(PopulationTheme, spike) ==
              offsetof(PopulationTheme, tier_wt_cube) + 4 * sizeof(float),
    "F-5: spike must directly follow tier_wt_cube — no field between them");
static_assert(offsetof(PopulationTheme, weight) ==
              sizeof(PopulationTheme) - sizeof(float),
    "F-5: weight must be PopulationTheme's tail field");

// THE S2/S3 BOUNDARY FACE: THEMES is read across the boundary by the
// theme_tier_weights accessor (the interface trio's vocabulary member —
// the program theory; Q5 unified the per-family plugs).
//
// ── The theme table ────────────────────────────────────────────────
// AXES: row = theme id (0 TRANSITION / 1 MONUMENTAL / 2 COLONNADE /
//   3 ANTENNA / 4 BARREN — theme_short_name order); row 0 below carries
//   the full per-line column legend; rows 1-4 read by that legend.
//   spawn_weight's inner axis is PopFamily order (PYRAMID=0 …
//   GALLERY=11, pinned by the F-1 static_assert at roster.hpp); each
//   tier_* inner axis is that family's own tier order (member names in
//   the struct above).
// UNITS: spawn_weight / tier_* = multipliers (1.0 = neutral);
//   spike = envelope weight at full intensity; sustain / decay /
//   cooldown = PATCH COUNTS (envelope lifetime; cooldown 0 =
//   immediately re-eligible); weight = lattice-node selection weight.
// CONSUMERS: generate_tile_population (spawn_weight → spatial_density);
//   select_theme_at_node (weight);
//   theme_envelope_weight / evaluate_theme_envelope (spike/sustain/
//   decay/cooldown); theme_tier_weights → the generic pipeline's tier
//   selection (ribbon reads tier_wt_ribbon directly).
// Biography determinant — frozen biography (§12): every number here
// shifts spawn rates or tier draws; changing one changes worlds.
inline constexpr PopulationTheme THEMES[THEME_COUNT] = {
    // ── 0: TRANSITION — sparse connective tissue ─────────────────
    // RISE_0 — the pyramid column, all rows: a pyramid is a DOOR now
    // (FRUSTUM_0 promoted every summit to a fly base), and a door the
    // themes suppress is a function the visitor meets by luck. The
    // sub-1.0 cells rise — 0.4/0.3/0.5/0.4 -> 0.8/0.8/0.8/0.7 — so every
    // theme carries doors; MONUMENTAL keeps its 1.5 crown and BARREN
    // stays the leanest. PyramidConfig::SPAWN_CHANCE holds at 0.050
    // (FRUSTUM_0): the instance ceiling is 8, and the remaining
    // suppressor was this column, not the roll.
    {   { 0.8f, 0.75f, 0.7f, 0.3f, 0.3f, 0.3f, 0.5f, 0.3f, 1.0f, 0.5f, 0.5f, 0.5f },  // spawn_weight [pyr..sph, ribn, cube, gol, gall]
        { 1.0f, 1.0f, 1.0f },                                       // tier_pyr
        { 1.233f, 0.3f, 1.0f },                                     // tier_arch
        { 0.1f, 0.2f, 0.3f },                                       // tier_col
        { 0.1f, 2.0f, 0.7f },                                       // tier_ant
        { 1.0f, 1.0f, 1.0f },                                       // tier_palm
        { 1.0f, 1.0f, 1.0f },                                       // tier_cactus
        { 1.0f, 1.0f, 1.0f },                                       // tier_blade
        { 1.0f, 1.0f },                                              // tier_sphere (neutral)
        { 1.0f, 1.0f, 1.0f },                                       // tier_ribbon (neutral)
        { 1.0f, 1.0f, 1.0f, 1.0f },                                 // tier_cube (neutral)
        150.0f, 20u, 3u, 0u,                                          // spike, sustain, decay, cooldown
        0.21f                                                         // weight
    },
    // ── 1: MONUMENTAL — big pyramids, varied arches, heavy columns
    {   { 1.5f, 0.75f, 1.0f, 0.5f, 0.2f, 0.2f, 0.5f, 0.3f, 1.0f, 0.3f, 0.3f, 0.3f },
        { 0.2f, 0.5f, 3.0f },
        { 2.941f, 0.1f, 3.0f },
        { 0.01f, 0.01f, 1.0f },
        { 0.5f, 1.5f, 0.5f },
        { 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f, 1.0f },
        150.0f, 10u, 10u, 8u,
        0.30f
    },
    // ── 2: COLONNADE — dense columns, moderate arches ────────────
    {   { 0.8f, 0.75f, 4.0f, 0.5f, 0.3f, 0.3f, 0.5f, 0.3f, 1.0f, 0.3f, 0.5f, 0.4f },  // RISE_0: pyr 0.3 -> 0.8 (the largest map share starved the doors hardest)
        { 1.0f, 1.0f, 1.0f },
        { 1.423f, 0.5f, 1.0f },
        { 0.3f, 3.0f, 5.0f },
        { 0.2f, 0.1f, 0.1f },
        { 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f, 1.0f },
        150.0f, 15u, 6u, 6u,
        0.31f
    },
    // ── 3: ANTENNA — antenna-dominant corridor ───────────────────
    {   { 0.8f, 0.75f, 1.0f, 4.0f, 0.5f, 0.5f, 0.5f, 0.3f, 1.0f, 0.3f, 0.3f, 0.3f },  // RISE_0: pyr 0.5 -> 0.8
        { 1.0f, 0.05f, 2.0f },
        { 0.949f, 0.2f, 0.8f },
        { 0.1f, 0.3f, 0.3f },
        { 0.5f, 3.5f, 1.0f },
        { 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f, 1.0f },
        180.0f, 10u, 5u, 5u,
        0.18f
    },
    // ── 4: BARREN — near-empty ───────────────────────────────────
    {   { 0.7f, 0.75f, 0.5f, 0.3f, 0.2f, 0.2f, 0.1f, 0.3f, 1.0f, 0.1f, 0.2f, 0.6f },  // RISE_0: pyr 0.4 -> 0.7 — barren keeps its lean, but even the desert has doors
        { 2.0f, 0.5f, 0.2f },
        { 1.897f, 1.0f, 1.0f },
        { 0.2f, 0.5f, 0.5f },
        { 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f, 1.0f },
        100.0f, 12u, 3u, 4u,
        0.04f
    },
};

// ═══ MODULE STATE ══════════════════════════════════════════════════

// The envelope machine's working state + the per-patch selection.
// Instance (themes_state_) lives at the composition root.
struct ThemesState {
    ThemeEnvelope envelope_{};
    uint32_t temporal_flavor = 0;   // Q7 TEMPORAL axis: the drifting theme index (drives tier weights), set per-patch by evaluate_theme_envelope. Independent of spatial_density — a different axis, not a duplicate.
};

// ═══ MODULE FUNCTIONS ══════════════════════════════════════════════

// Select a theme at a lattice node from cumulative weights
// Q5: the ONE tier-weight accessor over the family→member map. Replaces the
// per-family *_get_theme_tier_weights plugs — the generic pipeline calls it
// with traits.family_id (a PopFamily). Tables untouched; bit-safe (returns
// the same const float* into the PopulationTheme row the old plug did).
inline const float* theme_tier_weights(uint32_t theme_idx, uint32_t family_id) {
    const PopulationTheme& th = THEMES[theme_idx];
    switch (family_id) {
        case PopFamily::PYRAMID: return th.tier_wt_pyramid;
        case PopFamily::ARCH:    return th.tier_wt_arch;
        case PopFamily::COLUMN:  return th.tier_wt_column;
        case PopFamily::ANTENNA: return th.tier_wt_antenna;
        case PopFamily::PALM:    return th.tier_wt_palm;
        case PopFamily::CACTUS:  return th.tier_wt_cactus;
        case PopFamily::BLADE:   return th.tier_wt_blade;
        case PopFamily::SPHERE:  return th.tier_wt_sphere;
        case PopFamily::RIBBON:  return th.tier_wt_ribbon;
        case PopFamily::CUBE:    return th.tier_wt_cube;
        default:                 return th.tier_wt_column;
    }
}

// Q4: the SPATIAL theme selector. Its bucket walk stays SEPARATE from
// select_weighted (seed_utils) BY DESIGN — it normalizes inline against the
// live theme weight-sum (select_weighted takes pre-normalized weights), and
// together with evaluate_theme_envelope (the temporal, stateful-sequenced
// selector) it forms the theme-sampler family — a different shape from the
// stateless entity bucket walk. Documented-not-migrated (Q4 ruling).
inline uint32_t select_theme_at_node(uint32_t node_seed) {
    float roll = cpu_hash_f(node_seed, 370u);
    float cumul = 0.0f;
    float total = 0.0f;
    for (uint32_t t = 0; t < THEME_COUNT; t++) total += THEMES[t].weight;
    for (uint32_t t = 0; t < THEME_COUNT; t++) {
        cumul += THEMES[t].weight / total;
        if (roll < cumul) return t;
    }
    return THEME_COUNT - 1;
}

// THE POPULATION HALF — spawn density + per-family theme weights (NOT
// terrain shape; the population/themes concern). Read by tile_apply_
// spawn_mult (F3, tile_world.hpp). Q6b relocated it here to sit with the
// authoring that fills it (generate_tile_population, below) and the theme
// vocabulary (THEMES) it samples. TileState (tile_world.hpp) carries it as
// `pop`; population_themes precedes tile_world in the cohort, so the type
// is complete at the TileState member.
struct TilePopulation {
    // Theme: evaluated from theme lattice at tile generation time
    float spatial_density[PopFamily::COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f }; // Q7 SPATIAL axis: per-family (PopFamily order), position-locked density multiplier (applied by F3 tile_apply_spawn_mult; 1.0 = neutral default). Independent of temporal_flavor — a different axis, not a duplicate.
};

// The population-half authoring (Q6b: relocated verbatim from generate_
// tile_state's two population blocks). BIT-IDENTITY LIVE — hash draws off
// the theme seed band, disjoint from the shape draws (tile_seed
// props) that stay in tile_world; the caller passes active_seed so this
// stays free of WorldState (later in the cohort). Dim::PATCH_EXTENT is read as
// Dim::PATCH_EXTENT (same constexpr 50.0f; state.hpp precedes this header).
inline TilePopulation generate_tile_population(uint32_t active_seed, int32_t gx, int32_t gz) {
    TilePopulation pop;

    // ── Theme field (coarse compositional character) ─────────
    {
        float patch_cx = (gx + 0.5f) * Dim::PATCH_EXTENT;
        float patch_cz = (gz + 0.5f) * Dim::PATCH_EXTENT;
        float tlx = patch_cx / THEME_LATTICE_SPACING;
        float tlz = patch_cz / THEME_LATTICE_SPACING;
        int32_t tbx = (int32_t)std::floor(tlx);
        int32_t tbz = (int32_t)std::floor(tlz);
        float tfx = tlx - tbx, tfz = tlz - tbz;
        float twx = tfx * tfx * (3.0f - 2.0f * tfx);
        float twz = tfz * tfz * (3.0f - 2.0f * tfz);

        // Blend spawn weights across 4 lattice nodes.
        float blended_spawn[PopFamily::COUNT] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

        for (int dz = 0; dz <= 1; dz++) for (int dx = 0; dx <= 1; dx++) {
            uint32_t ns = cpu_lattice_node_seed(active_seed, tbx + dx, tbz + dz, THEME_SEED_BAND);
            uint32_t tidx = select_theme_at_node(ns);
            const auto& theme = THEMES[tidx];
            float w = ((dx == 1) ? twx : (1.0f - twx)) * ((dz == 1) ? twz : (1.0f - twz));
            for (uint32_t f = 0; f < PopFamily::COUNT; f++) {
                blended_spawn[f] += theme.spawn_weight[f] * w;
            }
        }

        for (uint32_t f = 0; f < PopFamily::COUNT; f++)
            pop.spatial_density[f] = blended_spawn[f];
        // (pop.theme_idx dead write CUT: the spatial
        //  dominant-theme was authored and read nowhere; the LIVE theme axis
        //  is the temporal one, evaluate_theme_envelope → temporal_flavor.)
    }

    return pop;
}

inline float theme_envelope_weight(const PopulationTheme& theme, uint32_t elapsed) {
    if (elapsed < theme.sustain) return theme.spike;
    if (elapsed < theme.sustain + theme.decay) {
        float t = (float)(elapsed - theme.sustain) / (float)theme.decay;
        return theme.spike + (THEME_BASE_WEIGHT - theme.spike) * t;
    }
    return THEME_BASE_WEIGHT;
}

// Called ONCE per patch, inside the spawn loop, BEFORE per-family gates.
// Returns the theme index to use for this patch. DEFINED below
// (the flag-gated census dump reaches the machine via MachineCtx).
uint32_t evaluate_theme_envelope(ThemesState& ts, MachineCtx* c, uint32_t tile_seed_value);  // stores its own result ; takes the face
void reset_theme_envelope(ThemesState& ts);


// ═══ IMPL (every body takes the machine face; no dispatch
// rows). COHORT PROOF: every callee is
// declared earlier in the cohort — cpu_hash_f (seed_utils, contracts
// side), THEMES/theme_envelope_weight (above), EXCEPT the DIAG-gated
// census dump, fwd-declared here under its own flag (spawn_engine.hpp
// follows this header in the cohort; disclosed per the cohort law).
// Nothing here names Cartridge. MachineCtx's type precedes via contracts/entity_types.hpp
// (the patch_system.hpp cohort precedent). ═══════════════════════════


// Census home: sole consumer is the census dump below;
// the names are this module's vocabulary.
inline const char* theme_short_name(uint32_t theme) {
    static const char* NAMES[] = { "transition", "monumental", "colonnade", "antenna", "barren" };
    return (theme < THEME_COUNT) ? NAMES[theme] : "???";
}

// ═══ THE JOURNEY LAW (ruled, R4) ═══════════════════════════════════
// This sampler is STATEFUL and advances once per SPAWNED PATCH, in
// streaming order (nearest-first, budget-paced) — so the player's PATH
// through the world writes the tier biography: which patches spawn
// first determines every tile's temporal_flavor, and with it which
// TIERS its entities draw. DELIBERATE — keep-and-declare: do NOT
// determinize this by keying it to (gx,gz); the journey is the author.
// (Spawn PROBABILITY stays path-independent — the gates roll on
// tile_seed; only the tier axis rides the journey.)
inline uint32_t evaluate_theme_envelope(ThemesState& ts, MachineCtx* c, uint32_t tile_seed_value) {
    (void)c;
    auto& env = ts.envelope_;

    // Build effective weights
    float weights[THEME_COUNT];
    float total = 0.0f;
    for (uint32_t i = 0; i < THEME_COUNT; i++) {
        if (env.cooldowns[i] > 0) {
            weights[i] = 0.0f;
        }
        else if ((int32_t)i == env.active) {
            weights[i] = theme_envelope_weight(THEMES[i], env.elapsed);
        }
        else {
            weights[i] = THEME_BASE_WEIGHT;
        }
        total += weights[i];
    }
    if (total < 0.001f) total = 1.0f;

    // Roll from weights
    float roll = cpu_hash_f(tile_seed_value, 370u);
    uint32_t selected = THEME_COUNT - 1;
    float cumul = 0.0f;
    for (uint32_t i = 0; i < THEME_COUNT; i++) {
        cumul += weights[i] / total;
        if (roll < cumul) { selected = i; break; }
    }

    // State transitions
    if ((int32_t)selected != env.active) {
        if (env.active >= 0) {
            env.cooldowns[env.active] = THEMES[env.active].cooldown;
        }
        env.active = (int32_t)selected;
        env.elapsed = 0;

        // Census dump on theme transition
    }
    else {
        env.elapsed++;
    }

    // Check expiry
    if (env.active >= 0) {
        const auto& th = THEMES[env.active];
        if (env.elapsed >= th.sustain + th.decay) {
            env.cooldowns[env.active] = th.cooldown;
            env.active = -1;
            env.elapsed = 0;
        }
    }

    // Tick cooldowns
    for (uint32_t i = 0; i < THEME_COUNT; i++) {
        if (env.cooldowns[i] > 0) env.cooldowns[i]--;
    }

    ts.temporal_flavor = selected;  // stores its own result — the caller no longer writes the organ
    return selected;
}

// ─── Teardown reset (owner verb) ──────────────────────────────────
inline void reset_theme_envelope(ThemesState& ts) {
    ts = ThemesState{};
}


} // namespace the_board
} // namespace t7
