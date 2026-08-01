#pragma once
#include <cstdint>
#include <cstddef>                                        // size_t (cap_to_ceiling's array-reference template)
#include "cartridges/the_board/contracts/roster.hpp"        // PopFamily — the table's axis (F-1 pinned)
#include "cartridges/the_board/contracts/entity_types.hpp"  // EntityInstance — cap_to_ceiling scales its params

// ─── indoor_module.hpp ───────────────────────────────────────────
// History: audit/LADDER.md
//
// ═══ THE INDOOR MODULE — mood's insert on the spawn chain ═══════
// One table + three dials govern the whole indoor treatment.
// Families keep only their param lists; the module modulates.
//
// HOME: this is mood policy, but its consumers (grounded.hpp,
// spheres.hpp, cube_behaviors.hpp, ribbon.hpp) precede mood.hpp in
// the cohort — so the TYPE + TABLE + dials ride the contracts tier
// (the second-consumer law) and the mood panel keeps banner +
// pointer (direction/mood.hpp, INDOOR ENTITY PLACEMENT).
//
// SIZES (Jean's law): indoors, everything exists at outdoor
// distribution. NATURAL keeps outdoor size; EXACT snaps to the
// ceiling (column — architectural); CAP scales down to the fraction
// of the ceiling only when taller — cap-only, nothing ever inflates.
// (The ribbon's extra RIBBON_INDOOR_SCALE pre-scale is unreachable
// since SWEEP_1 T9 — see its row and the dial below.)
// BOUNDS go live at the margin site (spawn_engine's indoor
// clamp): MARGIN = the standing wall-margin clamp; FULL = wholly
// inside (margin + the family's containment extent); FREE = may
// straddle walls (no clamp).

namespace t7 {
namespace the_board {

enum class IndoorSize   : uint32_t { NATURAL, CAP, EXACT };
enum class IndoorBounds : uint32_t { MARGIN, FULL, FREE };
struct IndoorTreatment { IndoorSize size; IndoorBounds bounds; };

inline constexpr float INDOOR_HEIGHT_CAP_FRACTION = 0.75f; // Jean's law
// STATUS: LATENT[ribbon:indoor-miniature] — READER-FREE since SWEEP_1 T9.
// Ribbons never spawn indoors now, so select_ribbon_for_patch returns
// before the pre-scale it fed could run. Kept for one reading; retiring a
// named dial is its own pass.
inline constexpr float RIBBON_INDOOR_SCALE        = 0.15f; // Jean's dial — tune on sight

// Clamp (not reject) is intentional: rejection-based logic
// would silently drop entities anchored to corner patches,
// because their seed-determined position keeps landing in the
// wall margin and never recovers. Clamping shifts the candidate
// to the legal-box edge and lets the existing footprint-overlap
// check handle any pile-ups that result.
// Margin doubled (was 10) so that paintings and snapshots
// mounted on indoor walls are visibly separated from spawning
// entities — the entity's footprint now reads as distinct from
// the wall surface and any artwork on it.
inline constexpr float INDOOR_ENTITY_WALL_MARGIN  = 20.0f; // existing, re-homed here

// The ceiling-fit floor (COLUMN CEILING FIT): indoors the cmg kernel
// derives effective column height = ceiling_height − ground_y(center)
// — the capital sits flush with the ceiling plane across topography —
// but never below this floor. PINNED PAIR: world.wgsl declares the
// same value as const COLUMN_MIN_INDOOR_HEIGHT beside the cmg kernel
// bindings (authored in both rooms, named the same, never
// dial-derived — the TILE_GRID_CAPACITY pattern).
inline constexpr float COLUMN_MIN_INDOOR_HEIGHT   = 1.0f;  // extreme-terrain floor: a column never collapses below this

// AXES: PopFamily order, PINNED by F-1. gol size is NATURAL here —
// its height cap is GPU-side (zone_derive_params, once per zone birth),
// noted on the row.
inline constexpr IndoorTreatment INDOOR_TREATMENT[PopFamily::COUNT] = {
    /* pyramid */ { IndoorSize::CAP,     IndoorBounds::MARGIN },
    /* arch    */ { IndoorSize::CAP,     IndoorBounds::MARGIN },
    /* column  */ { IndoorSize::EXACT,   IndoorBounds::MARGIN },  // architectural: touches the ceiling
    /* antenna */ { IndoorSize::CAP,     IndoorBounds::MARGIN },
    /* palm    */ { IndoorSize::CAP,     IndoorBounds::MARGIN },
    // SWEEP_1 T8 — NO PLANT EXCEEDS ¾ OF ITS ROOM'S CEILING. Both were
    // NATURAL ("Jean: keeps size"); that ruling is superseded. Cactus is
    // why: CANDELABRA is authored at 20.0 ± 4.0 wu against a 20 wu flat
    // ceiling. Blade never reaches the cap at its authored tiers and takes
    // it anyway — the law is about plants, not about tall plants, and the
    // cap only ever scales down.
    /* cactus  */ { IndoorSize::CAP,     IndoorBounds::MARGIN },
    /* blade   */ { IndoorSize::CAP,     IndoorBounds::MARGIN },
    /* sphere  */ { IndoorSize::CAP,     IndoorBounds::MARGIN },
    // The ribbon row is UNREACHABLE since SWEEP_1 T9 — no ribbon is ever
    // born indoors, so neither column is consulted. Row kept: the table's
    // axis is PopFamily and F-1 pins it dense.
    /* ribbon  */ { IndoorSize::CAP,     IndoorBounds::FULL   },  // (was: pre-scaled by RIBBON_INDOOR_SCALE; stays inside)
    /* cube    */ { IndoorSize::CAP,     IndoorBounds::MARGIN },
    /* gol     */ { IndoorSize::NATURAL, IndoorBounds::FREE   },  // may straddle; lift capped at derive
    /* gallery */ { IndoorSize::NATURAL, IndoorBounds::FULL   },  // sand-standing exhibits wholly inside
};

static_assert(PopFamily::PYRAMID == 0 && PopFamily::ARCH    == 1
           && PopFamily::COLUMN  == 2 && PopFamily::ANTENNA == 3
           && PopFamily::PALM    == 4 && PopFamily::CACTUS  == 5
           && PopFamily::BLADE   == 6 && PopFamily::SPHERE  == 7
           && PopFamily::RIBBON  == 8 && PopFamily::CUBE    == 9
           && PopFamily::GOL     == 10 && PopFamily::GALLERY == 11
           && PopFamily::COUNT   == 12,
    "INDOOR_TREATMENT rows ride F-1's PopFamily order — pyramid, arch, "
    "column, antenna, palm, cactus, blade, sphere, ribbon, cube, gol, "
    "gallery: re-row this table in lockstep before renumbering any family");

// ─── THE CAP LAW (shared) ────────────────────────────────────────
// If the family's current vertical extent already fits under
// cap_fraction × ceiling, nothing happens — nothing ever inflates.
// Otherwise every listed param scales by the one ratio: miniatures,
// not squashes. The per-family param-index lists (which indices are
// LENGTHS — never ratios, counts, angles, or rates) stay with the
// families; the module holds only the law.
template<size_t N>
inline void cap_to_ceiling(EntityInstance& inst, float ceiling_h,
    float cap_fraction, float current_h,
    const uint32_t (&params_to_scale)[N]) {
    if (current_h <= 1e-3f) return;
    const float cap_h = cap_fraction * ceiling_h;
    if (current_h <= cap_h) return;
    const float scale = cap_h / current_h;
    for (size_t i = 0; i < N; i++) inst.params[params_to_scale[i]] *= scale;
}

} // namespace the_board
} // namespace t7
