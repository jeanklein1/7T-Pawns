#pragma once
#include <cstdint>

// ─── contracts/orb_conductor.hpp ───────────────────────────────────
//
// THE CONDUCTOR'S CONSOLE (ORRERY_2). Every number the conductor
// speaks is a dial now: ORB_CONDUCTOR is the DESIGN — the authored
// seeds, two jobs only: seeding ORB_CONDUCTOR_LIVE and standing under
// its assert. ORB_CONDUCTOR_LIVE is what the tick reads, and the ORGAN
// writes (block CONDUCTOR). Jean adjusts the numbers himself.
//
// THE ROW-WATCH LAW: the tick compares the reigning state's LIVE row
// against a cached copy every frame and re-speaks on any change, so a
// panel edit lands on the sky within one frame, mid-reign.
//
// THE CEREMONY (ORRERY_5) is structural, not dialed: one coin, one
// chain. BROWNIAN and ORBITAL both end at the coin — 75% another
// brownian, with a fresh drag drawn from [drag_min, drag_max];
// `frost_percent` of the time FROZEN, which opens the fixed chain
// flocking → orbital. There are no other edges, so the wheel can follow
// itself — orbital → frozen is legal, one wheel in four at the seed.
// Rules are identity, not knobs.
//
// THE DRAG IS A RANGE, NOT A LEVEL. The two authored brownian levels
// ORRERY_0 shipped are now this row's two ends. `conductor_enter_` draws
// a normalized u once per entry and `conductor_apply_` lerps through it,
// so a panel edit to either end moves the sky continuously mid-reign
// instead of waiting for the next entry.
// ────────────────────────────────────────────────────────────────────

namespace t7 {
namespace the_board {

// One state's dial row. All 4-byte fields, no padding — the tick's
// row-watch memcmp depends on that (asserted below).
//
// WHICH RULE READS WHAT, so a dead cell is never mistaken for a defect:
//   brownian  gesture, drag_min, drag_max, noise, speed_mult
//   flocking  gesture, speed_mult
//   orbital   gesture, orbital_speed, speed_mult
//   frozen    nothing — the kernel's frozen branch reads only the baked
//             per-orb drag; frozen's reign is its whole contribution.
// Every row reads duration_beats and jitter_beats.
struct OrbConductorState {
    uint32_t gesture;         // registry index; clamped per rule (brn 6, orb 4, flk 8)
    float    drag_min;        // BROWNIAN, low end  ×(baked per-orb ~0.4)
    float    drag_max;        // BROWNIAN, high end; drawn uniform on entry
    float    orbital_speed;   // rad/s before speed_mult; read when rule==ORBITAL
    float    noise;           // brownian energy source (noise_amp)
    float    speed_mult;      // master motion strength while this state reigns
    float    duration_beats;  // reign length
    float    jitter_beats;    // ± drawn on each entry; 0 = the metronome
};

struct OrbConductorConsole {
    uint32_t enabled;         // the conductor's own switch
    uint32_t frost_percent;   // THE COIN — brownian and orbital each end
                              // in FROZEN (the wheel) this % of the time
    OrbConductorState states[4];
};

// Indices are load-bearing twice over: the ceremony names them, and they
// are the panel's reading order — which is the wheel's own order.
inline constexpr uint32_t ORB_CS_BROWNIAN = 0u;
inline constexpr uint32_t ORB_CS_FROZEN   = 1u;
inline constexpr uint32_t ORB_CS_FLOCK    = 2u;
inline constexpr uint32_t ORB_CS_ORBITAL  = 3u;
inline constexpr uint32_t ORB_CS_COUNT    = 4u;

// Rule identity — structural, parallel to the rows, not a dial.
inline constexpr uint32_t ORB_CONDUCTOR_RULES[ORB_CS_COUNT] =
    { 0u, 2u, 3u, 1u };  // BRN FRZ FLK ORB
inline constexpr const char* ORB_CONDUCTOR_NAMES[ORB_CS_COUNT] = {
    "brownian", "frozen", "flocking", "orbital",
};

// Neutral for the three rule_drag slots the conductor leaves alone.
inline constexpr float ORB_CONDUCTOR_RULE_DRAG_NEUTRAL = 1.0f;

// THE DESIGN — Jean's seeds.
inline constexpr OrbConductorConsole ORB_CONDUCTOR = {
    1u,   // enabled
    25u,  // frost_percent — "75 to 25", his words
    { //  gest  dragLo dragHi  orbSpd   noise  spdMul  dur    jitter
        { 0u,  0.2f,  0.7f,  0.0010f,  3.0f,  4.0f,   16.0f, 0.0f },  // brownian
        { 0u,  0.0f,  0.0f,  0.0010f,  3.0f,  4.0f,   16.0f, 0.0f },  // frozen — reads none of these
        { 3u,  0.0f,  0.0f,  0.0010f,  3.0f,  4.0f,   32.0f, 0.0f },  // flocking — 4.0 is the dial's ceiling
        { 0u,  0.0f,  0.0f,  0.0010f,  3.0f,  2.0f,   16.0f, 0.0f },  // orbital — gesture 0 (scatter), 0.0010 rad/s = one revolution in 105 min
    },
};

// THE LIVE SURFACE — the panel's block and the tick's read.
inline OrbConductorConsole ORB_CONDUCTOR_LIVE = ORB_CONDUCTOR;

static_assert(sizeof(OrbConductorState) == 8u * 4u,
    "the row-watch memcmp assumes a packed 32-byte row: a field added "
    "here is added to the cache compare by construction, padding is not");
static_assert(sizeof(OrbConductorConsole) ==
    2u * 4u + ORB_CS_COUNT * sizeof(OrbConductorState),
    "ORB_CONDUCTOR_LIVE is a whole-struct copy of the design: a field "
    "added to one is added to the other by construction");

} // namespace the_board
} // namespace t7
