#pragma once
#include <cstdint>
#include "cartridges/the_board/contracts/demo_config.hpp"       // DemoConfig, Roster

// ─── matrix.hpp (THE DEMO MATRIX: pieces × demos, cells booleans) ──
// Jean's ratified grid.
//
// ONE constexpr grid replaces the hand-written per-demo headers. ROWS
// are PIECES (named down the left, Roster field order); COLUMNS are
// DEMOS (named across the top). A cell is a compile-time bool. The
// grid IS C++ — no generation step, no external format, entirely
// visible; composing a demo is ticking cells, and it compiles the
// instant you save.
//
// COMPLETE CENSUS: the four FOUNDATIONAL pieces — the surface, the sun,
// the point (the witness/camera's parent), the pawn body (slot-0)
// — are ALWAYS ON in every sentence, so they are NOT tickable cells;
// they ride the LOCKED banner below so the census reads whole without
// pretending they can be turned off. (The pawn body is locked-AND-NOTED
// per Jean's stamp: it always exists today, and BECOMES a tickable
// row the day a demo pulls the deferred body-tickable cut — then
// terrain gains a cleaner column with no rework, one cell flipped off.)
//
//   ┌──────────────── LOCKED (always on, every column) ─────────────┐
//   │  THE SURFACE   the stage — terrain streams in every sentence   │
//   │  THE SUN       the atmosphere — the sun-VP computed every frame │
//   │  THE POINT     the parent — the camera is its witness     │
//   │  THE PAWN BODY slot-0 — never evicted; idles in free-fly        │
//   │                (locked-and-noted: tickable when a demo needs    │
//   │                 it gone — the deferred body-tickable cut)       │
//   └────────────────────────────────────────────────────────────────┘
//
// The compile-time boolean FOLD is preserved end to end: column_to_roster
// yields a constexpr Roster, so ROSTER (demos/demo.hpp) folds every
// gate site exactly as the hand-written brace-list did.
// DemoConfig is untouched — the seed is authored onto the root organ
// in the Cartridge ctor (the boot mood is DRAWN from it — USHER_0).
//
// FREE TICKING: the grid encodes NO dependency edges. The roster's one
// legality edge (transitions ⇒ portal) stays the FIRST EDGE static_assert
// in demos/demo.hpp, riding the folded ROSTER — tick an illegal column
// and the build fails there, as ever. The matrix FEEDS the resolver; it
// does not reimplement it.

namespace t7 {
namespace the_board {

// ═══ THE ROWS (piece indices — Roster FIELD ORDER, the one invariant) ═
// These index the grid's rows AND map 1:1 to Roster's declared fields;
// column_to_roster reads them in this exact order. Keep in lock-step
// with struct Roster (contracts/roster.hpp). The static_assert below
// pins the count.
namespace Piece {
enum : uint32_t {
    // 12 families (PopFamily order)
    pyramid, arch, column, antenna, palm, cactus, blade,
    sphere, ribbon, cube, gol, gallery,
    // 7 features
    pawn_aura, orbs, spot_lights, indoor_shell, portal, transitions, wanderers,
    COUNT,
};
}

// ═══ THE COLUMNS (demo names — the valid INCUBATE_DEMO set) ════════
// Adding a demo = one enum value here + one grid column + one seed.
// A bad INCUBATE_DEMO=<name> resolves to DemoCol::<name>
// and fails as an unknown enumerator — a clean compile error.
enum class DemoCol : uint32_t {
    full,      // the golden twin — every tickable ON
    minimal,   // the degenerate proof — every tickable OFF
    COUNT,
};

// ═══ THE GRID (rows = pieces, columns = demos, cells = bool) ═══════
inline constexpr bool GRID[Piece::COUNT][static_cast<uint32_t>(DemoCol::COUNT)] = {
    //                       full  minimal
    /* pyramid        */  {  true,  false },
    /* arch           */  {  true,  false },
    /* column         */  {  true,  false },
    /* antenna        */  {  true,  false },
    /* palm           */  {  true,  false },
    /* cactus         */  {  true,  false },
    /* blade          */  {  true,  false },
    /* sphere         */  {  true,  false },
    /* ribbon         */  {  true,  false },
    /* cube           */  {  true,  false },
    /* gol            */  {  true,  false },
    /* gallery        */  {  true,  false },
    /* pawn_aura      */  {  true,  false },
    /* orbs           */  {  true,  false },
    /* spot_lights    */  {  true,  false },
    /* indoor_shell   */  {  true,  false },
    /* portal         */  {  true,  false },
    /* transitions    */  {  true,  false },
    /* wanderers      */  {  true,  false },
};

// ═══ PER-COLUMN SCALARS (the D2 fields — one per demo) ═════════════
inline constexpr uint32_t DEMO_SEED[static_cast<uint32_t>(DemoCol::COUNT)] = {
    /* full    */ 42,
    /* minimal */ 42,
};

// ═══ THE COLUMN READ (a demo column → a constexpr Roster) ══════════
// Aggregate-inits Roster in field order from the selected column. The
// compiler folds this whole; every field is a constexpr bool from GRID.
constexpr Roster column_to_roster(DemoCol d) {
    const uint32_t c = static_cast<uint32_t>(d);
    return Roster{
        GRID[Piece::pyramid][c], GRID[Piece::arch][c], GRID[Piece::column][c],
        GRID[Piece::antenna][c], GRID[Piece::palm][c], GRID[Piece::cactus][c],
        GRID[Piece::blade][c], GRID[Piece::sphere][c], GRID[Piece::ribbon][c],
        GRID[Piece::cube][c], GRID[Piece::gol][c], GRID[Piece::gallery][c],
        GRID[Piece::pawn_aura][c], GRID[Piece::orbs][c], GRID[Piece::spot_lights][c],
        GRID[Piece::indoor_shell][c], GRID[Piece::portal][c],
        GRID[Piece::transitions][c], GRID[Piece::wanderers][c],
    };
}

// ═══ THE DEMO (a column name → a full DemoConfig) ══════════════════
constexpr DemoConfig demo_config(DemoCol d) {
    return DemoConfig{
        column_to_roster(d),
        DEMO_SEED[static_cast<uint32_t>(d)],
    };
}

// ═══ THE INVARIANTS ════════════════════════════════════════════════
// (1) the row count equals Roster's field count — the field-order
//     mapping in column_to_roster is total. (Add a field to Roster ⇒
//     add a Piece row ⇒ this trips until they agree.)
static_assert(Piece::COUNT == 19,
    "matrix: Piece row count must equal Roster's 19 fields (12 families + 7 features)");

// (2) THE BYTE-EQUIVALENCE GOLDEN (Jean's mandatory gate). The two
//     migrated columns are pinned to the retired headers' exact values,
//     so the retirement is provably lossless — now and forever (the
//     old full.hpp / minimal.hpp are gone; these asserts are what keep
//     their sentences honest).
//     ONE field left the pin entirely: boot_mood left DemoConfig when
//     the boot mood became a DRAW under the destination law (USHER_0)
//     — the seed golden below is now the whole of the pin.
static_assert(demo_config(DemoCol::full).roster.all_enabled(),
    "GOLDEN: demo=full must equal old full.hpp — all 19 tickable bits ON");
static_assert(demo_config(DemoCol::full).seed == 42,
    "GOLDEN: demo=full seed must equal old full.hpp");
static_assert(demo_config(DemoCol::minimal).roster.none_enabled(),
    "GOLDEN: demo=minimal must equal old minimal.hpp — all 19 tickable bits OFF");
static_assert(demo_config(DemoCol::minimal).seed == 42,
    "GOLDEN: demo=minimal seed must equal old minimal.hpp");

} // namespace the_board
} // namespace t7
