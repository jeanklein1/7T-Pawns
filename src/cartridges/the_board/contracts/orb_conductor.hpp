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
// THE CEREMONY (ORRERY_1) is structural, not dialed: the pool breathes
// brownian; only the pool or the wheel may freeze (1-in-`frost_one_in`);
// the frost always releases into the flock; the flock alone opens the
// wheel; the wheel returns to the pool. Rules are identity, not knobs.
//
// SEEDS: Jean's spec, plus his later word — drag 0.0 in every row
// ("keeping the drag zero"); noise and speed_mult at the old ceilings
// PER STATE ("we keep noise and speed mult at max"), his to split;
// orbital 0.7 / 1.0; gesture 0; jitter 0, waiting ("non regular
// periods of time" was the first sentence of this feature).
// ────────────────────────────────────────────────────────────────────

namespace t7 {
namespace the_board {

// One state's dial row. All 4-byte fields, no padding — the tick's
// row-watch memcmp depends on that (asserted below).
struct OrbConductorState {
    uint32_t gesture;         // registry index; clamped per rule (brn 6, orb 4, flk 8)
    float    drag;            // BROWNIAN drag ×(baked per-orb ~0.4); raw seam, 0.0 = undamped
    float    orbital_speed;   // rad/s before speed_mult; read when rule==ORBITAL
    float    noise;           // brownian energy source (noise_amp)
    float    speed_mult;      // master motion strength while this state reigns
    float    duration_beats;  // reign length
    float    jitter_beats;    // ± drawn on each entry; 0 = the metronome
};

struct OrbConductorConsole {
    uint32_t enabled;         // the conductor's own switch
    uint32_t frost_one_in;    // frozen enters 1-in-N from pool/wheel
    OrbConductorState states[6];
};

// Indices are load-bearing: the ceremony names them.
inline constexpr uint32_t ORB_CS_BRN_MED = 0u;
inline constexpr uint32_t ORB_CS_BRN_INT = 1u;
inline constexpr uint32_t ORB_CS_FLOCK   = 2u;
inline constexpr uint32_t ORB_CS_ORB_MED = 3u;
inline constexpr uint32_t ORB_CS_ORB_INT = 4u;
inline constexpr uint32_t ORB_CS_FROZEN  = 5u;
inline constexpr uint32_t ORB_CS_COUNT   = 6u;

// Rule identity — structural, parallel to the rows, not a dial.
inline constexpr uint32_t ORB_CONDUCTOR_RULES[ORB_CS_COUNT] =
    { 0u, 0u, 3u, 1u, 1u, 2u };  // BRN BRN FLK ORB ORB FRZ
inline constexpr const char* ORB_CONDUCTOR_NAMES[ORB_CS_COUNT] = {
    "brownian-medium", "brownian-intense", "flocking",
    "orbital-medium", "orbital-intense", "frozen",
};

// Neutral for the three rule_drag slots the conductor leaves alone.
inline constexpr float ORB_CONDUCTOR_RULE_DRAG_NEUTRAL = 1.0f;

// THE DESIGN — Jean's seeds.
inline constexpr OrbConductorConsole ORB_CONDUCTOR = {
    1u,   // enabled
    8u,   // frost_one_in
    { //  gest  drag   orbSpd  noise  spdMul  dur    jitter
        { 0u,  0.0f,  0.0f,   3.0f,  4.0f,   16.0f, 0.0f },  // brownian-medium
        { 0u,  0.0f,  0.0f,   3.0f,  4.0f,   16.0f, 0.0f },  // brownian-intense
        { 0u,  0.0f,  0.0f,   3.0f,  4.0f,   32.0f, 0.0f },  // flocking
        { 0u,  0.0f,  0.7f,   3.0f,  4.0f,   16.0f, 0.0f },  // orbital-medium
        { 0u,  0.0f,  1.0f,   3.0f,  4.0f,   16.0f, 0.0f },  // orbital-intense
        { 0u,  0.0f,  0.0f,   3.0f,  4.0f,   16.0f, 0.0f },  // frozen — reads none of these
    },
};

// THE LIVE SURFACE — the panel's block and the tick's read.
inline OrbConductorConsole ORB_CONDUCTOR_LIVE = ORB_CONDUCTOR;

static_assert(sizeof(OrbConductorState) == 7u * 4u,
    "the row-watch memcmp assumes a packed 28-byte row: a field added "
    "here is added to the cache compare by construction, padding is not");
static_assert(sizeof(OrbConductorConsole) ==
    2u * 4u + ORB_CS_COUNT * sizeof(OrbConductorState),
    "ORB_CONDUCTOR_LIVE is a whole-struct copy of the design: a field "
    "added to one is added to the other by construction");

} // namespace the_board
} // namespace t7
