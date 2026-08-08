#pragma once
#include <cstdint>

// ─── mood_constants.hpp ──────────────────────────────────────────
// History: audit/LADDER.md
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

inline constexpr uint32_t MOOD_COUNT = 4;

// ─── Mood IDs ───────────────────────────────────────────────────
// One outdoor world open, one outdoor world walled, two rooms.
inline constexpr uint32_t MOOD_OPEN_SUNSET = 0;
inline constexpr uint32_t MOOD_INDOOR_FLAT = 1;
inline constexpr uint32_t MOOD_INDOOR_VAULT = 2;
inline constexpr uint32_t MOOD_FINITE_OUTDOOR = 3;

struct PortalDestination {
    uint32_t seed = 0;
    bool finite = false;
    uint32_t finite_radius = 2;
    uint32_t mood = 0;               // destination mood id (MOOD_* above)
};

// Portal color by mood (indexed by destination.mood)
inline constexpr float PORTAL_COLORS[MOOD_COUNT][3] = {
    { 0.72f, 0.45f, 0.85f },  // mood 0  open_sunset     — lilac
    { 0.95f, 0.55f, 0.15f },  // mood 1  indoor_flat     — orange
    { 0.95f, 0.80f, 0.20f },  // mood 2  indoor_vault    — yellow
    { 0.85f, 0.20f, 0.15f },  // mood 3  finite_outdoor  — red
};
inline constexpr float PORTAL_COLOR_BACK[3] = { 0.35f, 0.55f, 0.90f };  // back-portal — blue
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
inline const float* portal_color_for(const PortalDestination& dest, bool is_back) {
    return is_back ? PORTAL_COLOR_BACK : PORTAL_COLORS[dest.mood % MOOD_COUNT];
}

} // namespace the_board
} // namespace t7
