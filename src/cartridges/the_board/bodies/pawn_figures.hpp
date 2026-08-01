#pragma once
#include <cstdint>

// ═══════════════════════════════════════════════════════════════════════════
// THE PAWN FIGURES REGISTRY — the design surface for pawn skins ("figures").
// One row per figure; flat matrices; Pawn (1.5/0.5) is the reference scale.
//
// TWO PROFILE IDIOMS, kept in each family's natural form (ratified in-chat):
//   FAM_REGULAR  — the current pawn. Uses the hardcoded world.wgsl
//                  pawn_profile_radius + the legacy per-agent/tier color path.
//                  Its table row carries scale only; profile/palette are null.
//   FAM_SMOOTH   — parametric profile-of-revolution (SmoothProfile, 16 fields),
//                  the designer's pawnRadius(t,P) form.
//   FAM_HERALDIC — segment list (HeraldicProfile, ≤7 segs), the stacked/heraldic
//                  radiusAt() form, each segment shaped by a PawnShape.
//
// Color: figures 1..13 carry a height-gradient palette + a bounded HSV drift
//        envelope (seeded per agent from AgentState.seed). Figure 0 (regular)
//        keeps the existing color logic and takes no palette (drift = 0).
//
// Data transcribed verbatim from 7t_pawn_designer.jsx (Smooth *Profile()/
// *Stops(); Heraldic *Geom()/*Stops()). Source of truth for H2 (GPU pack) and
// H3 (WGSL mirror). Edit numbers HERE; the pipeline flows down.
// ═══════════════════════════════════════════════════════════════════════════

namespace t7 {
namespace the_board {

// ── Axes ────────────────────────────────────────────────────────────────────
enum PawnFamilyId : uint32_t {
    FAM_REGULAR  = 0,
    FAM_SMOOTH   = 1,
    FAM_HERALDIC = 2,
    FAM_COUNT    = 3,
};

// Segment shape vocabulary — MUST match the WGSL profile_shape() switch (H3)
// and the designer's STACKED_SHAPES keys.
enum PawnShape : uint32_t {
    SHAPE_LINEAR  = 0,
    SHAPE_CONCAVE = 1,
    SHAPE_CONVEX  = 2,
    SHAPE_STEP    = 3,
    SHAPE_BELL    = 4,
    SHAPE_FLARE   = 5,
    SHAPE_OGEE    = 6,
    SHAPE_SPHERE  = 7,
    SHAPE_COUNT   = 8,
};

// ═══ DISTRIBUTION — the tweak surface ═══════════════════════════════════════
// Percent of the population per family; equal within a family. A rarity slope
// later = replace this with per-figure weights (one column), no code change.
struct PawnFamilyShare { PawnFamilyId family; const char* name; float share_pct; };

inline constexpr PawnFamilyShare FIGURE_SHARES[FAM_COUNT] = {
    //  family        name        share   (members → each)
    { FAM_REGULAR,  "regular",  70.0f },  // 1 → 70.00%
    { FAM_SMOOTH,   "smooth",   15.0f },  // 6 →  2.50%
    { FAM_HERALDIC, "heraldic", 15.0f },  // 7 →  2.14%
};

// ═══ SMOOTH PROFILES — parametric (16 fields), mirrors pawnRadius(t,P) ═══════
struct SmoothProfile {
    float start_r, flare_r, flare_t;         // base → flare
    float peak_r, peak_t;                     // peak
    float base_t, body_start_r;               // body onset
    float body_t, waist_r;                    // waist
    float neck_t, neck_r;                     // neck
    float collar_t, collar_bulge;             // collar
    float head_t, head_base_r, head_sphere_r; // head (hemisphere) + tip to 0
};

//                                         start flare flr_t  peak pk_t  base body_r  body waist  neck nk_r  coll bulge  head hd_r  sph_r
inline constexpr SmoothProfile PROF_PAWN     = { 0.75f,0.88f,0.04f, 0.92f,0.10f, 0.14f,0.70f, 0.50f,0.22f, 0.62f,0.18f, 0.70f,0.08f, 0.95f,0.15f,0.35f }; // reference (regular pawn uses hardcoded path)
inline constexpr SmoothProfile PROF_SQUAT    = { 0.85f,0.90f,0.05f, 1.05f,0.12f, 0.18f,0.85f, 0.58f,0.25f, 0.60f,0.40f, 0.67f,0.24f, 0.93f,0.30f,0.20f };
inline constexpr SmoothProfile PROF_COLOSSAL = { 0.78f,0.92f,0.05f, 0.98f,0.12f, 0.16f,0.72f, 0.48f,0.26f, 0.60f,0.14f, 0.68f,0.17f, 0.96f,0.18f,0.42f };
inline constexpr SmoothProfile PROF_ACORN    = { 0.50f,0.90f,0.05f, 0.80f,0.18f, 0.28f,0.50f, 0.50f,0.35f, 0.65f,0.30f, 0.70f,0.25f, 0.93f,0.25f,0.20f };
inline constexpr SmoothProfile PROF_SPIRE    = { 0.50f,0.65f,0.05f, 0.65f,0.15f, 0.20f,0.50f, 0.55f,0.30f, 0.75f,0.20f, 0.85f,0.05f, 0.99f,0.18f,0.19f };
inline constexpr SmoothProfile PROF_IDOL     = { 0.50f,0.62f,0.05f, 0.68f,0.10f, 0.16f,0.55f, 0.45f,0.20f, 0.55f,0.20f, 0.66f,0.30f, 0.92f,0.10f,0.20f };
inline constexpr SmoothProfile PROF_STELE    = { 0.78f,0.82f,0.04f, 0.955f,0.10f, 0.14f,0.78f, 0.60f,0.35f, 0.72f,0.28f, 0.80f,0.03f, 0.95f,0.35f,0.20f };

// ═══ HERALDIC PROFILES — segment lists (≤7), mirrors heraldicRadiusAt() ══════
struct HeraldicSeg     { float height, r_bot, r_top; PawnShape shape; };
struct HeraldicProfile { uint32_t seg_count; HeraldicSeg seg[7]; };  // unused segs zeroed

inline constexpr HeraldicProfile PROF_H_BRONZE = { 5, {
    { 0.070f, 0.76f, 0.64f, SHAPE_CONVEX  },   // foot
    { 0.385f, 0.81f, 0.55f, SHAPE_CONCAVE },   // base (was convex)
    { 0.635f, 0.50f, 0.30f, SHAPE_OGEE    },   // shank (was concave)
    { 0.185f, 0.21f, 0.31f, SHAPE_CONVEX  },   // collar disc
    { 0.650f, 0.00f, 0.40f, SHAPE_SPHERE  },   // head sphere — 28% → 34% of the figure
    {}, {},
}};
inline constexpr HeraldicProfile PROF_H_SILVER = { 5, {
    { 0.005f, 1.00f, 0.92f, SHAPE_CONVEX },    // foot, now a thin lip
    { 0.185f, 0.92f, 0.55f, SHAPE_CONVEX },
    { 0.380f, 0.50f, 0.28f, SHAPE_OGEE   },
    { 0.040f, 0.28f, 0.42f, SHAPE_CONVEX },
    { 0.265f, 0.00f, 0.40f, SHAPE_SPHERE },
    {}, {},
}};
// Gold: rebuilt, 6 → 7 segments. No padding entry remains.
inline constexpr HeraldicProfile PROF_H_GOLD = { 7, {
    { 0.335f, 0.660f, 0.700f, SHAPE_CONVEX  },
    { 0.735f, 0.840f, 0.670f, SHAPE_OGEE    },
    { 0.360f, 0.675f, 0.280f, SHAPE_CONCAVE },
    { 0.040f, 0.135f, 0.175f, SHAPE_BELL    },
    { 0.005f, 0.200f, 0.000f, SHAPE_CONVEX  },
    { 0.210f, 0.285f, 0.145f, SHAPE_CONCAVE },
    { 0.685f, 0.465f, 0.205f, SHAPE_SPHERE  },
}};
inline constexpr HeraldicProfile PROF_H_STEEL = { 6, {
    { 0.04f, 1.05f, 0.95f, SHAPE_CONVEX },
    { 0.10f, 0.95f, 0.58f, SHAPE_CONVEX },
    { 0.42f, 0.52f, 0.24f, SHAPE_OGEE   },
    { 0.05f, 0.24f, 0.24f, SHAPE_OGEE   },   // bead: bell → ogee
    { 0.04f, 0.24f, 0.00f, SHAPE_CONVEX },   // r_top 0.42 → 0
    { 0.42f, 0.00f, 0.52f, SHAPE_SPHERE },   // head r_top 0.42 → 0.52
    {},
}};
inline constexpr HeraldicProfile PROF_H_CRYSTAL = { 7, {
    { 0.04f, 1.05f, 0.95f, SHAPE_CONVEX },
    { 0.10f, 0.69f, 0.58f, SHAPE_CONVEX },   // r_bot 0.95 → 0.69
    { 0.42f, 0.63f, 0.24f, SHAPE_OGEE   },   // r_bot 0.52 → 0.63
    { 0.05f, 0.24f, 0.37f, SHAPE_BELL   },   // r_top 0.24 → 0.37
    { 0.04f, 0.24f, 0.42f, SHAPE_CONVEX },
    { 0.40f, 0.00f, 0.40f, SHAPE_SPHERE },
    { 0.06f, 0.00f, 0.10f, SHAPE_SPHERE },   // finial bud
}};
inline constexpr HeraldicProfile PROF_H_STAR = { 7, {
    { 0.04f, 0.00f, 0.90f, SHAPE_CONVEX },   // foot now springs from a POINT (r_bot 1.00 → 0)
    { 0.09f, 0.57f, 0.52f, SHAPE_CONVEX },
    { 0.48f, 0.45f, 0.20f, SHAPE_OGEE   },
    { 0.04f, 0.20f, 0.20f, SHAPE_BELL   },
    { 0.03f, 0.20f, 0.36f, SHAPE_CONVEX },
    { 0.36f, 0.00f, 0.36f, SHAPE_SPHERE },
    { 0.06f, 0.00f, 0.08f, SHAPE_SPHERE },
}};
inline constexpr HeraldicProfile PROF_H_DIVINE = { 7, {
    { 0.04f, 0.39f, 0.88f, SHAPE_CONVEX },   // r_bot 1.00 → 0.39
    { 0.10f, 0.63f, 0.50f, SHAPE_CONVEX },   // r_bot 0.88 → 0.63
    { 0.50f, 0.46f, 0.18f, SHAPE_OGEE   },
    { 0.07f, 0.20f, 0.20f, SHAPE_BELL   },
    { 0.03f, 0.20f, 0.38f, SHAPE_CONVEX },
    { 0.38f, 0.00f, 0.38f, SHAPE_SPHERE },
    { 0.07f, 0.00f, 0.10f, SHAPE_SPHERE },
}};

// ═══ PALETTES — height gradients (foot→head). t < 0 ends the list. ══════════
struct PawnStop { float t, r, g, b; };

inline constexpr PawnStop PAL_LAVENDER[] = {   // regular pawn reference (unused — legacy color path)
    {0.00f,0.80f,0.50f,0.80f},{0.14f,0.80f,0.50f,0.80f},{0.50f,0.80f,0.50f,0.80f},
    {0.62f,0.80f,0.50f,0.80f},{0.70f,0.80f,0.50f,0.80f},{0.95f,0.80f,0.50f,0.80f},{-1,0,0,0},
};
inline constexpr PawnStop PAL_BRONZE[] = {     // Squat
    {0.00f,0.55f,0.42f,0.30f},{0.18f,0.68f,0.55f,0.40f},{0.55f,0.78f,0.66f,0.50f},
    {0.65f,0.82f,0.71f,0.55f},{0.72f,0.85f,0.74f,0.58f},{0.92f,0.92f,0.84f,0.68f},{-1,0,0,0},
};
inline constexpr PawnStop PAL_BLACKGOLD[] = {  // Colossal
    {0.00f,0.10f,0.09f,0.09f},{0.16f,0.13f,0.11f,0.11f},{0.48f,0.18f,0.15f,0.13f},
    {0.60f,0.55f,0.42f,0.16f},{0.68f,0.78f,0.62f,0.22f},{0.96f,0.92f,0.80f,0.38f},{-1,0,0,0},
};
inline constexpr PawnStop PAL_ACORN[] = {      // Acorn (non-monotonic: brightest at collar, darkens to cap)
    {0.00f,0.30f,0.18f,0.08f},{0.28f,0.48f,0.30f,0.16f},{0.55f,0.55f,0.36f,0.20f},
    {0.65f,0.62f,0.42f,0.24f},{0.72f,0.42f,0.28f,0.14f},{0.88f,0.22f,0.13f,0.06f},{-1,0,0,0},
};
inline constexpr PawnStop PAL_STEELBLUE[] = {  // Spire
    {0.00f,0.18f,0.20f,0.25f},{0.20f,0.30f,0.35f,0.42f},{0.65f,0.45f,0.52f,0.60f},
    {0.75f,0.55f,0.62f,0.70f},{0.82f,0.62f,0.70f,0.78f},{0.92f,0.78f,0.85f,0.92f},{-1,0,0,0},
};
inline constexpr PawnStop PAL_JADE[] = {       // Idol
    {0.00f,0.10f,0.22f,0.20f},{0.16f,0.15f,0.32f,0.28f},{0.45f,0.20f,0.45f,0.40f},
    {0.55f,0.30f,0.55f,0.50f},{0.66f,0.45f,0.70f,0.62f},{0.92f,0.65f,0.85f,0.75f},{-1,0,0,0},
};
inline constexpr PawnStop PAL_LIMESTONE[] = {  // Stele
    {0.00f,0.55f,0.52f,0.48f},{0.14f,0.62f,0.60f,0.55f},{0.60f,0.72f,0.70f,0.66f},
    {0.72f,0.78f,0.76f,0.72f},{0.80f,0.82f,0.80f,0.76f},{0.95f,0.88f,0.86f,0.82f},{-1,0,0,0},
};
inline constexpr PawnStop PAL_H_BRONZE[] = {
    {0.00f,0.32f,0.20f,0.10f},{0.06f,0.45f,0.28f,0.14f},{0.20f,0.55f,0.36f,0.18f},
    {0.55f,0.65f,0.42f,0.22f},{1.00f,0.78f,0.55f,0.28f},{-1,0,0,0},
};
inline constexpr PawnStop PAL_H_SILVER[] = {
    {0.00f,0.55f,0.55f,0.58f},{0.06f,0.68f,0.68f,0.70f},{0.20f,0.78f,0.78f,0.80f},
    {0.55f,0.85f,0.85f,0.86f},{1.00f,0.92f,0.92f,0.94f},{-1,0,0,0},
};
inline constexpr PawnStop PAL_H_GOLD[] = {
    {0.00f,0.52f,0.30f,0.10f},{0.06f,0.70f,0.45f,0.16f},{0.20f,0.78f,0.40f,0.25f},
    {0.50f,0.92f,0.32f,0.38f},{0.58f,0.85f,0.65f,0.44f},{1.00f,0.98f,0.70f,0.58f},{-1,0,0,0},
};
inline constexpr PawnStop PAL_H_STEEL[] = {
    {0.00f,0.22f,0.26f,0.32f},{0.06f,0.35f,0.42f,0.50f},{0.20f,0.50f,0.58f,0.65f},
    {0.55f,0.62f,0.70f,0.75f},{0.62f,0.58f,0.66f,0.72f},{1.00f,0.78f,0.85f,0.90f},{-1,0,0,0},
};
inline constexpr PawnStop PAL_H_CRYSTAL[] = {
    {0.00f,0.30f,0.50f,0.55f},{0.06f,0.45f,0.65f,0.70f},{0.20f,0.60f,0.78f,0.82f},
    {0.55f,0.72f,0.86f,0.88f},{0.62f,0.78f,0.90f,0.92f},{0.92f,0.88f,0.96f,0.98f},{1.00f,0.98f,1.00f,1.00f},{-1,0,0,0},
};
inline constexpr PawnStop PAL_H_STAR[] = {
    {0.00f,0.62f,0.60f,0.72f},{0.06f,0.78f,0.75f,0.85f},{0.20f,0.88f,0.85f,0.92f},
    {0.62f,0.92f,0.88f,0.95f},{0.68f,0.95f,0.92f,0.96f},{0.92f,0.98f,0.96f,1.00f},{1.00f,1.00f,1.00f,1.00f},{-1,0,0,0},
};
inline constexpr PawnStop PAL_H_DIVINE[] = {
    {0.00f,0.85f,0.78f,0.55f},{0.06f,0.92f,0.85f,0.60f},{0.20f,0.96f,0.90f,0.65f},
    {0.62f,1.00f,0.95f,0.70f},{0.70f,1.00f,0.96f,0.75f},{0.92f,1.00f,0.98f,0.85f},{1.00f,1.00f,1.00f,0.95f},{-1,0,0,0},
};

// ═══ COLOR DRIFT — bounded per-agent HSV travel (seed-driven) ════════════════
// Each figure moves only where its design tolerates it. Regular pawn = 0 (legacy).
struct PawnDrift { float hue_deg, sat, val; };

// ═══ TILT LAG — how fast the possessed body follows the ground normal ═══════
// Consumed CPU-side only: the cartridge reads PAWN_FIGURES[skin].tilt_tau for
// the POSSESSED slot each frame and ships it in GPUDesignConfig.pawn_tilt_tau.
// Never packed into GPUPawnFigure.
//
//   tau = 0  → instant snap to the terrain normal (the pawn's response).
//   tau > 0  → first-order lag, alpha = 1 - exp(-dt/tau) per frame.
//              Settles in roughly 3·tau: 0.10 → ~0.3 s, 0.14 → ~0.42 s.
//
// THREE CLASSES, NOT A GRADIENT. A per-figure ramp differing by a few percent
// per row is below the threshold of legibility; classes read. The break follows
// the SILHOUETTE, not head ornament: sway is a distance percept, and at the
// range where sway is legible a finial has long since stopped resolving. By
// aspect (h/r) the roster runs 8.00, 5.00, 4.67, then a gap down to 3.45 and
// below — Spire and Stele are the only figures that stand like sticks, and
// Colossal is the only monolith. Everything else reads as a pawn and responds
// as one.
inline constexpr float TILT_LAG_NONE  = 0.00f;   // reads as a pawn — 11 of 14
inline constexpr float TILT_LAG_STICK = 0.10f;   // slender uprights: Spire, Stele
inline constexpr float TILT_LAG_TALL  = 0.14f;   // the monolith: Colossal

// ═══ THE MASTER MATRIX ══════════════════════════════════════════════════════
// smooth / heraldic / palette are null where not applicable to the family.
struct PawnFigureDef {
    PawnFamilyId            family;
    const char*             name;
    float                   height, radius;   // world units; Pawn = 1.5/0.5 reference
    const SmoothProfile*    smooth;           // set iff FAM_SMOOTH
    const HeraldicProfile*  heraldic;         // set iff FAM_HERALDIC
    const PawnStop*         palette;          // null for FAM_REGULAR (legacy color)
    PawnDrift               drift;
    float                   tilt_tau;         // possessed-body tilt lag, seconds. CPU-only.
};

inline constexpr PawnFigureDef PAWN_FIGURES[] = {
  //  family        name        H      R      smooth          heraldic         palette         drift{hue,sat,val}      tilt_tau
  { FAM_REGULAR,  "Pawn",     1.50f, 0.50f, nullptr,        nullptr,         nullptr,        { 0.0f, 0.00f, 0.00f}, TILT_LAG_NONE  }, // 0  unchanged, as ruled
  { FAM_SMOOTH,   "Squat",    1.90f, 0.55f, &PROF_SQUAT,    nullptr,         PAL_BRONZE,     {10.0f, 0.15f, 0.12f}, TILT_LAG_NONE  }, // 1  scale unchanged
  { FAM_SMOOTH,   "Colossal", 3.50f, 0.75f, &PROF_COLOSSAL, nullptr,         PAL_BLACKGOLD,  { 8.0f, 0.10f, 0.20f}, TILT_LAG_TALL  }, // 2  scale unchanged
  { FAM_SMOOTH,   "Acorn",    2.00f, 0.60f, &PROF_ACORN,    nullptr,         PAL_ACORN,      {12.0f, 0.15f, 0.10f}, TILT_LAG_NONE  }, // 3  was 1.70/0.50
  { FAM_SMOOTH,   "Spire",    3.80f, 1.04f, &PROF_SPIRE,    nullptr,         PAL_STEELBLUE,  {10.0f, 0.12f, 0.12f}, TILT_LAG_TALL  }, // 4  was 2.80/0.35 — now tallest; STICK → TALL
  { FAM_SMOOTH,   "Idol",     2.30f, 0.75f, &PROF_IDOL,     nullptr,         PAL_JADE,       {10.0f, 0.15f, 0.10f}, TILT_LAG_NONE  }, // 5  was 1.80/0.55
  { FAM_SMOOTH,   "Stele",    3.00f, 0.70f, &PROF_STELE,    nullptr,         PAL_LIMESTONE,  { 6.0f, 0.08f, 0.15f}, TILT_LAG_STICK }, // 6  was 2.00/0.40
  { FAM_HERALDIC, "Bronze",   1.50f, 0.76f, nullptr,        &PROF_H_BRONZE,  PAL_H_BRONZE,   {10.0f, 0.15f, 0.12f}, TILT_LAG_NONE  }, // 7  was 1.00/0.50
  { FAM_HERALDIC, "Silver",   1.61f, 0.50f, nullptr,        &PROF_H_SILVER,  PAL_H_SILVER,   { 4.0f, 0.06f, 0.15f}, TILT_LAG_NONE  }, // 8  was 1.10/0.50
  { FAM_HERALDIC, "Gold",     2.26f, 0.74f, nullptr,        &PROF_H_GOLD,    PAL_H_GOLD,     { 6.0f, 0.10f, 0.12f}, TILT_LAG_NONE  }, // 9  was 1.15/0.50
  { FAM_HERALDIC, "Steel",    1.93f, 0.55f, nullptr,        &PROF_H_STEEL,   PAL_H_STEEL,    { 8.0f, 0.10f, 0.12f}, TILT_LAG_NONE  }, // 10 was 1.30/0.55
  { FAM_HERALDIC, "Crystal",  1.75f, 0.59f, nullptr,        &PROF_H_CRYSTAL, PAL_H_CRYSTAL,  { 8.0f, 0.08f, 0.10f}, TILT_LAG_NONE  }, // 11 was 1.35/0.55
  { FAM_HERALDIC, "Star",     1.45f, 0.55f, nullptr,        &PROF_H_STAR,    PAL_H_STAR,     {10.0f, 0.06f, 0.08f}, TILT_LAG_NONE  }, // 12 scale unchanged
  { FAM_HERALDIC, "Divine",   1.88f, 0.55f, nullptr,        &PROF_H_DIVINE,  PAL_H_DIVINE,   { 4.0f, 0.05f, 0.06f}, TILT_LAG_NONE  }, // 13 was 1.55/0.55
};

inline constexpr uint32_t PAWN_FIGURE_COUNT = sizeof(PAWN_FIGURES) / sizeof(PAWN_FIGURES[0]);

// ═══ THE EYE (SWEEP_1 T3) ═══════════════════════════════════════════════════
// FPV eye height is a RATIO of the POSSESSED body's live height, not a number.
// world.wgsl carried `const FPV_EYE_HEIGHT = PAWN_HEIGHT + 0.2` — 1.7 wu for
// every figure alike, so possessing Colossal (3.50) put the eye at its waist
// and Star (1.45) floated a head above itself.
//
// ANCHORED TO THE CONVENTIONAL PAWN, so the common case is preserved to the
// pixel: figure 0 is 70% of the population (FIGURE_SHARES — one member at 70%,
// against 2.50% each for the six smooth and 2.14% each for the seven heraldic),
// and at its 1.50 the ratio reproduces 1.7 exactly. The rise is what the old
// constant authored above the head; the ratio is derived from it and figure 0's
// own height, so a re-scaled reference pawn carries both.
//
// CONSUMED CPU-SIDE, like tilt_tau above and for the same reason: the figure
// table is a RENDER-VS-only uniform (binding 112 is absent from the Compute
// Entity layout, which is full at 12), and the FPV camera is a compute kernel.
// The cartridge reads PAWN_FIGURES[skin].height for the POSSESSED slot each
// frame and ships height x EYE_RATIO in GPUDesignConfig.pawn_eye_height.
inline constexpr float PAWN_EYE_RISE = 0.2f;   // wu above the head, as authored
inline constexpr float EYE_RATIO =
    (PAWN_FIGURES[0].height + PAWN_EYE_RISE) / PAWN_FIGURES[0].height;   // 1.7 / 1.5 = 1.1333333

// The anchor, witnessed: the conventional pawn's eye must land where the
// retired hardcode put it. Tolerance, not equality — EYE_RATIO is a quotient,
// so the round trip through it is float-exact only by luck.
static_assert(PAWN_FIGURES[0].height * EYE_RATIO > 1.69999f
           && PAWN_FIGURES[0].height * EYE_RATIO < 1.70001f,
    "T3: EYE_RATIO must reproduce the retired FPV_EYE_HEIGHT (1.7) on figure 0");

// ── Family spans — derived from PAWN_FIGURES (for the spawn roll) ────────────
// skin_id layout is contiguous per family: [regular | smooth | heraldic].
inline constexpr uint32_t figure_family_member_count(PawnFamilyId fam) {
    uint32_t n = 0;
    for (uint32_t i = 0; i < PAWN_FIGURE_COUNT; ++i)
        if (PAWN_FIGURES[i].family == fam) ++n;
    return n;
}
inline constexpr uint32_t figure_family_base(PawnFamilyId fam) {
    for (uint32_t i = 0; i < PAWN_FIGURE_COUNT; ++i)
        if (PAWN_FIGURES[i].family == fam) return i;
    return 0u;
}

// ── Witnesses ────────────────────────────────────────────────────────────────
static_assert(PAWN_FIGURE_COUNT == 14, "PAWN_FIGURES must declare 14 figures");
static_assert(FIGURE_SHARES[FAM_REGULAR].share_pct
            + FIGURE_SHARES[FAM_SMOOTH].share_pct
            + FIGURE_SHARES[FAM_HERALDIC].share_pct == 100.0f,
            "family shares must sum to 100%");
static_assert(PAWN_FIGURES[0].family == FAM_REGULAR && PAWN_FIGURES[0].palette == nullptr,
            "figure 0 is the regular pawn: legacy path, no palette");
static_assert(figure_family_member_count(FAM_REGULAR)  == 1
           && figure_family_member_count(FAM_SMOOTH)   == 6
           && figure_family_member_count(FAM_HERALDIC) == 7,
           "figure roll assumes 1/6/7 members per family");
static_assert(figure_family_base(FAM_REGULAR) == 0u
           && figure_family_base(FAM_SMOOTH)  == 1u
           && figure_family_base(FAM_HERALDIC) == 7u,
           "figure families must be contiguous in PAWN_FIGURES order");

} // namespace the_board
} // namespace t7
