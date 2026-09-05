#pragma once
#include <cstdint>

// ─── mood_constants.hpp ──────────────────────────────────────────
//
// The mood-count constant, the Mood IDs, PortalDestination and the
// portal palette at file scope: the config-bearing module headers size
// their per-mood tables by MOOD_COUNT and name the IDs
// declaration-side; every reference is unqualified and resolves here.
// The palette sits with the destination it describes (PORTAL_1 C5), so
// no module is exempted from deriving by cohort order.
//
// ─────────────────────────────────────────────────────────────────

namespace t7 {
namespace the_board {

inline constexpr uint32_t MOOD_COUNT = 7;

// ─── Mood IDs ───────────────────────────────────────────────────
// Three outdoor worlds open (one sky each), one outdoor world walled,
// two rooms, and the atrium.
inline constexpr uint32_t MOOD_OPEN_SUNSET = 0;
inline constexpr uint32_t MOOD_INDOOR_FLAT = 1;
inline constexpr uint32_t MOOD_INDOOR_VAULT = 2;
inline constexpr uint32_t MOOD_FINITE_OUTDOOR = 3;
inline constexpr uint32_t MOOD_OPEN_NIGHT     = 4;  // ATMOS_1 — the open shape under a night sky
inline constexpr uint32_t MOOD_OPEN_NOON      = 5;  // ATMOS_1 — the open shape under a high sun
inline constexpr uint32_t MOOD_ATRIUM         = 6;  // ATRIUM_1 — the entrance: a small dark room,
                                                    // an arc of every door, the visitor's own figure
                                                    // walking through them. Boot mood; rare afterward.

// The names, positional by id (F-3's kin). mood_name() reads them, the
// registry emits them to the panel's mood select (organ_mood_names), and
// a new mood names itself here in the same commit as its id.
// Sized array: the compiler catches an EXTRA entry past MOOD_COUNT, but
// not a missing one — it zero-fills to nullptr.
inline constexpr const char* MOOD_NAMES[MOOD_COUNT] = {
    "open_sunset", "indoor_flat", "indoor_vault", "finite_outdoor",
    "open_night",  "open_noon",  "atrium",
};

struct PortalDestination {
    uint32_t seed = 0;
    bool finite = false;
    uint32_t finite_radius = 2;
    uint32_t mood = 0;               // destination mood id (MOOD_* above)
};

// ═══ THE WORLD-DRAW BANK, GRADUATED (ORGAN_4 P3d) ════════════════
//
// C3 DESTRUCTIVE, every row. These four facts are read while a WORLD IS
// BEING DRAWN — the portal roll as each DOORWAY arch commits, the mood
// draw as each portal picks its destination, the scheme roll as each
// indoor room derives its lights, the palette as each portal is built.
// None is re-read afterwards, which is exactly why the bank has NO
// BOUNDARY WIRING: re-speaking a destructive author means tearing the
// world down to apply a slider (D5, and the 3b D4 law behind it — a
// wrong re-speak tears down a world). The edit lands at the author's own
// next natural event, and the enrollment rows say so with the GEN chip.
//
// WHY THEY LIVE HERE. `PORTAL_DENSITY`, the destination law (a chance,
// one table since ATMOS_1) and `SCHEME_WEIGHTS` were `inline constexpr`
// in direction/mood.hpp, which
// the ORGAN may not include; the palette was already here, beside the
// PortalDestination it describes (PORTAL_1 C5). One bank rather than
// two: they are one question — what does a fresh world roll?
//
// WORLD_DRAW_TABLE is the DESIGN, two jobs only: seeding the bank and
// standing under its assert. WORLD_DRAW_LIVE is what the rollers read.

inline constexpr uint32_t SCHEME_COUNT = 4;   // indoor light schemes; sizes
                                              // LIGHT_SCHEMES and SCHEME_NAMES
                                              // in direction/mood.hpp
// ATRIUM_13 — SCHEME_ATRIUM IS GONE, and the count is four again. It was a
// fifth row that said "four downlights, straight down, every sigma 0"; the
// entrance points at QUARTET now (A13.2) and nothing pointed at row 4 any
// more. An authored row with no author left is not annotated, it is deleted.
//
// THE ROW THE ENTRANCE POINTS AT NOW. A pinned index must name its row
// (ATRIUM_5's rule, and the reason the palette pin carried a string witness):
// the other three schemes are positional because nothing pins them, and this
// one is not.
inline constexpr uint32_t SCHEME_QUARTET = 1;  // four ceiling lamps at the quarter
                                               // points, aim drawn — the indoor default

struct WorldDrawSurface {
    float portal_density;         // fraction of DOORWAY arches that become portals
    float mood_weights[MOOD_COUNT]; // ATMOS_1 — the open field's destination law:
                                    // one weighted walk over every mood, by id
                                    // (pick_portal_mood). 0 shuts a door without
                                    // unmaking the mood; the walk normalises.
    float scheme_weights[SCHEME_COUNT];        // Cathedral / Quartet / Gallery / Sanctum
    float portal_colors[MOOD_COUNT][3];        // by DESTINATION mood
    float portal_color_back[3];                // the back-portal
};

inline constexpr WorldDrawSurface WORLD_DRAW_TABLE = {
    1.00f,   // portal_density — every Doorway arch is a portal today
    { 0.20f, 0.20f, 0.20f, 0.00f, 0.15f, 0.15f, 0.02f },   // mood_weights by id: sunset, flat, vault,
                                                           // finite, night, noon, ATRIUM — finite
                                                           // outdoor's door SHUT (USHER_0: weight 0,
                                                           // mood kept; the panel dial reopens it);
                                                           // the atrium is the rarest door in the
                                                           // open field (ATRIUM_1) — the walk
                                                           // normalises
    { 0.42f, 0.43f, 0.10f, 0.05f },          // cathedral / quartet / gallery / sanctum (ATRIUM_13 — the
                                             // atrium's weight-0 fifth went with its row)
    {
        { 0.72f, 0.45f, 0.70f },  // mood 0  open_sunset     — lilac, deepened
        { 0.95f, 0.55f, 0.15f },  // mood 1  indoor_flat     — orange
        { 0.95f, 0.80f, 0.20f },  // mood 2  indoor_vault    — yellow
        { 0.85f, 0.20f, 0.15f },  // mood 3  finite_outdoor  — red
        { 0.80f, 0.85f, 0.95f },  // mood 4  open_night      — moon silver (ATMOS_1)
        { 0.20f, 0.85f, 0.85f },  // mood 5  open_noon       — cyan (ATMOS_1)
        { 1.00f, 1.00f, 1.00f },  // mood 6  atrium          — white: the way home (ATRIUM_1)
    },
    { 0.35f, 0.55f, 0.90f },      // back-portal — blue
};

inline WorldDrawSurface WORLD_DRAW_LIVE = WORLD_DRAW_TABLE;
static_assert(sizeof(WorldDrawSurface) == (1 + MOOD_COUNT + SCHEME_COUNT + MOOD_COUNT * 3 + 3) * sizeof(float),
    "WORLD_DRAW_LIVE is a whole-struct copy of the design row: a field "
    "added to one is added to the other by construction");
static_assert(MOOD_COUNT == 7 && SCHEME_COUNT == 4,
    "WORLD_DRAW_TABLE's palette and scheme rows are POSITIONAL — a new "
    "mood or a new scheme needs its row here, in the same commit");

// PORTAL_1 — THE ONE DERIVATION. A portal's colour is a fact about its
// DESTINATION, so it is derived from the destination and never stored twice.
// The palette lives HERE, with the destination it describes: every channel
// derives and none receives. No cohort exempts anyone — this is the contract
// tier, ahead of every module that spawns or draws a portal.
// Two sites call this, and they are the two that DECIDE a portal's
// appearance: arch_write_active (the generic commit) and
// force_spawn_portal_arch (the force-spawn channel). build_arch_mesh_params
// calls nothing — it is a producer of mesh params, and reads
// ActiveArch::col_* like every other arch.
//
// ORGAN_4 P3d — it reads the BANK now, not the table. The derivation is
// unchanged; only where it looks moved, which is what a graduation is.
inline const float* portal_color_for(const PortalDestination& dest, bool is_back) {
    return is_back ? WORLD_DRAW_LIVE.portal_color_back
                   : WORLD_DRAW_LIVE.portal_colors[dest.mood % MOOD_COUNT];
}

} // namespace the_board
} // namespace t7
