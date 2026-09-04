#pragma once
#include <cstdint>

// ─── contracts/orb_conductor.hpp ───────────────────────────────────
//
// THE CONDUCTOR (ORRERY_0). An automated author of the orb sky's motion:
// every 16 beats (flocking: 32) the rule and its parameters change,
// drawn by xorshift on the world seed. The audience is given something
// to wonder about — the changes ride the musical grid and never announce
// themselves.
//
// THE CLAIM. While ORB_CONDUCTOR_ON, the conductor CLAIMS noise_amp and
// speed_mult (pinned to their dial ceilings — Jean: "pin them") and drag
// (absolute per state; the four per-rule multipliers are written to the
// pass-through value so a state looks the same in every sky). The
// ORGAN dials keep authoring the REST values; the conductor authors the
// reign. orb_surface.hpp's speed_mult comment foretold exactly this
// claimant.
//
// THE TABLE IS THE DESIGN — one authored home. Rule, gesture, drag,
// orbital speed and duration live here and nowhere else; the tick in
// bodies/orbs.hpp reads this table and writes the GPU through the same
// targeted seams the player's rule-cycle uses (no reseed).
// ────────────────────────────────────────────────────────────────────

namespace t7 {
namespace the_board {

// Kill switch — mutable so a panel row can claim it later (one macro
// line in organ_params.inc); until then it is a rebuild dial.
inline bool ORB_CONDUCTOR_ON = true;

// Pinned energy while conducting — the dial CEILINGS of
// organ_params.inc's ORBS rows (speed_mult 0..4.0, noise_floor 0..3.0).
// R6 confirmed both ceilings unchanged at this commit.
inline constexpr float ORB_CONDUCTOR_NOISE      = 3.0f;
inline constexpr float ORB_CONDUCTOR_SPEED_MULT = 4.0f;

// ORRERY_0 RECON R2 — THE PASS-THROUGH VALUE IS 1.0, NOT 0.0.
// The zero sentinel is a CPU-SIDE convention only: configure_orbs'
// passthrough() lambda maps an authored 0 to 1.0 before upload
// ("(authored > 0.0f) ? authored : 1.0f"), and world.wgsl's own field
// comment says "Per-rule drag multipliers (1.0 = pass-through,
// sanitized on CPU)". The kernel applies them raw —
// `orb.vel * exp(-orb.drag * rule_drag_X * dt)` with NO sentinel
// branch — so a 0.0 written straight to the buffer through the
// targeted seam means "multiply the damping by zero", i.e. NO DRAG AT
// ALL, in every rule. The conductor bypasses configure_orbs by design,
// so it must speak the GPU's language and neutralize with 1.0.
inline constexpr float ORB_CONDUCTOR_RULE_DRAG_NEUTRAL = 1.0f;

struct OrbConductorStateDef {
    uint32_t    rule;            // ORB_RULE_* (bodies/orbs.hpp)
    uint32_t    gesture;         // index into that rule's registry
    float       drag;            // absolute; kernel value, no sentinel
    float       orbital_speed;   // rad/s; read only when rule==ORBITAL
    float       duration_beats;
    const char* name;
};

// Indices are load-bearing: transitions name them.
inline constexpr uint32_t ORB_CS_BRN_MED    = 0u;
inline constexpr uint32_t ORB_CS_BRN_INT    = 1u;
inline constexpr uint32_t ORB_CS_FLOCK      = 2u;  // boot state — NOT
                                                   // today's sky everywhere:
                                                   // sunset and night boot
                                                   // motion_rule 3u, but
                                                   // finite_outdoor boots 0u
                                                   // (BROWNIAN), so arming
                                                   // changes its rule at once.
inline constexpr uint32_t ORB_CS_ORB_MED    = 3u;
inline constexpr uint32_t ORB_CS_ORB_INT    = 4u;  // frozen's forced successor
inline constexpr uint32_t ORB_CS_FROZEN     = 5u;
inline constexpr uint32_t ORB_CS_COUNT      = 6u;

// Jean's six, verbatim. Brownian moves only the drag (0.6 legible /
// 0.0 intense); orbital moves only the velocity (0.7 / 1.0) — "can't
// go too excited" is speed's job, scatter is the authored gesture.
// Flocking has one mode, gesture 0, and holds twice as long.
inline constexpr OrbConductorStateDef ORB_CONDUCTOR_STATES[ORB_CS_COUNT] = {
    //  rule            gest  drag   orbSpd  beats  name
    {   0u /*BROWNIAN*/, 0u,  0.6f,  0.0f,   16.0f, "brownian-medium"  },
    {   0u /*BROWNIAN*/, 0u,  0.0f,  0.0f,   16.0f, "brownian-intense" },
    {   3u /*FLOCKING*/, 0u,  0.0f,  0.0f,   32.0f, "flocking"         },
    {   1u /*ORBITAL */, 0u,  0.0f,  0.7f,   16.0f, "orbital-medium"   },
    {   1u /*ORBITAL */, 0u,  0.0f,  1.0f,   16.0f, "orbital-intense"  },
    {   2u /*FROZEN  */, 0u,  0.0f,  0.0f,   16.0f, "frozen"           },
};

// FROZEN enters 1-in-8 among eligible draws, and only off brownian or
// orbital. Its exit is not drawn at all: always ORB_CS_ORB_INT.
inline constexpr uint32_t ORB_CONDUCTOR_FROZEN_MASK = 7u;  // (roll & 7)==0

} // namespace the_board
} // namespace t7
