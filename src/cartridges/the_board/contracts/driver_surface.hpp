#pragma once
#include <cstdint>

// ─── contracts/driver_surface.hpp — THE DRIVERS' ROOM ──────────────
// A driven parameter wears no dial on its value; it wears dials on its
// DRIVER. This room holds those dials' homes and nothing else: no module
// facts live here, no GPU block reads it, and the seams read it once per
// tick. docs/ORGAN.md, "The drivers' room".
//   rest — what a seam holds when the driver's authority is dialled away.
//          The fog's rest is the MOOD's (MoodState.fog_rest_*), so this
//          room keeps only the fog's gain.
//   gain — the blend: out = rest + gain·(driven − rest). 1 is the coupling
//          verbatim, 0 is full manual.
//   aura intent — the presence ramp's rest TARGET; Shift's door (SHIFT_0), the mood
//          policy door (force-off) and the panel write it.

namespace t7 {
namespace the_board {

struct DriverSurface {
    struct Fog {
        float    gain;            // 0 manual … 1 coupling verbatim
    } fog;
    struct Aura {
        uint32_t intent;          // the ramp's rest target: 0 off, 1 on
        float    attack;          // 1/s — presence rise rate
        float    release;         // 1/s — presence fall rate
        float    height_gain;     // × profile.height_scale at the tick
    } aura;
    // THE CHECKER FIELD'S SEAM. phase_motion_drivers flushes the
    // pitch-class colour field through set_checker_color_field every
    // frame; same shape as fog, same recipe. The rests are law: "amount 0
    // (the GPU maps that to each cell's seed color) and variance 0 — a
    // return to seed, not gray" (surface/terrain_looks.hpp).
    struct Checker {
        float rest_resultant[3];  // the music colour at gain 0
        float rest_amount;        // enveloped presence at gain 0
        float rest_variance;      // enveloped distinct-pc count at gain 0
        float gain;               // 0 manual … 1 coupling verbatim
    } checker;
    // THE RIBBON'S FOUR PIPES. ribbon_frame_tick reads four seams every
    // frame, and the rests are that seam's own fallbacks. color_stim's
    // fallback is a NULL POINTER: downstream is
    // `const float s = st ? st[c2] : 0.0f`, so its rest SHAPE is {0,0,0}.
    // ONE GAIN, THE SEAM'S VOLUME — the four pipes are one gesture, the
    // canvas moving a ribbon, so they share a gain as fog's terms do.
    struct Ribbon {
        float rest_amp_lat;       // 1.0 — identity: the seed's dance
        float rest_amp_vert;      // 1.0
        float rest_tint_stim[3];  // {0,0,0} — the null branch's shape
        float rest_tint_mix;      // 0.0
        float gain;               // one gain, the seam's volume
    } ribbon;
};

// The authored design — the code panel. The fog row carries the gain
// alone: its rests live on the REGIME (Regime.fog_density / .fog_color,
// contracts/spine_state.hpp).
inline constexpr DriverSurface DRIVER_TABLE = {
    { 0.63f },                  // fog: the desk dialled the coupling back —
                                // 0.63 driven, the rest held at the regime's rest
    { 0u, 1.0f, 1.5f, 1.0f },   // aura: intent off, the authored rates.
                                // (It rested ON for one commit, d3b1f6d;
                                // the next desk export put it back off.)
    { { 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, 1.0f },   // checker: a return to seed
    { 1.0f, 1.0f, { 0.0f, 0.0f, 0.0f }, 0.0f, 1.0f },   // ribbon: the seam's own fallbacks
};

// The live surface — the panel's fourth block and the seams' read.
inline DriverSurface DRIVER_LIVE = DRIVER_TABLE;
static_assert(sizeof(DriverSurface) == 18 * sizeof(float),
    "DRIVER_LIVE is a whole-struct copy of the design row: a field added "
    "to one is added to the other by construction. 18 words — fog 1, "
    "aura 4, checker 6, ribbon 7");

} // namespace the_board
} // namespace t7
