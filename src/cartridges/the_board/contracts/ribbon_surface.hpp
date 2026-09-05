#pragma once
#include <cstdint>

// ─── contracts/ribbon_surface.hpp ──────────────────────────────────
//
// THE RIBBON'S HEAD LAW, GRADUATED (ORGAN_3 w2, C2). bodies/ribbon.hpp
// says of these constants, in its own banner: "All control-panel
// material." This is that panel.
//
// RIBBON_TABLE is the DESIGN; RIBBON_LIVE is what the boot reads. Values
// carried verbatim from the module's tuning console, which keeps its
// banner and loses its numbers — one fact, one home.
//
// TWO TEMPERAMENTS SINCE RIBBON_1. The FLIGHT rows (yaw_rate … clear_body)
// are BOOT RESTS: the head is a kernel, the boot pins them into
// config.ribbon_*, and THAT is where the organ turns them — exactly what
// FIELD_SLACK and its siblings are to the field (contracts/control_panel.hpp).
// The rows below them — reference_bpm, the four wander_* and the mount's two
// eases — are read on the CPU every frame and are enrolled HERE. A row's
// enrollment names its live home; there is never a dial on both.
//
// THE STEERING LAW IS A `min`, NOT AN ASSERT, and that is why both of
// its terms may go live. ribbon.hpp states it:
//
//     available yaw rate is min(RIBBON_LIVE.yaw_rate, speed / RIBBON_LIVE.r_min)
//
// so the heading can only change while moving and the flown path can
// never be tighter than the minimum turn radius. A `min` stays honest
// at every value either dial can reach — unlike the beacon's
// static_assert, which is why THAT pair stayed authored. Same shape of
// question, opposite answer, because the mechanism differs.
//
// R_MIN'S FLOOR IS 1, NOT 0. It is a divisor; zero would divide the
// turn radius to nothing and hand the head an unbounded yaw rate. The
// enrollment line carries that floor.
//
// HERE SINCE ORGAN_4 P3d, IN A SECOND BANK: the spawn-rolled wander
// policy (chance, cruise, legs, retarget, hatch leg) and the colour
// vocabulary. They are DESTRUCTIVE-temperament facts and they edit the
// NEXT spawn rather than this one, which is why they are a second bank
// with a second temperament and not four more fields on the first.
//
// THE FLIGHT DIALS REACH THE GPU THROUGH `config` (RIBBON_1). The head and
// the body are kernels; they read config.ribbon_* and nothing else. The
// boot pins this table into GPUDesignConfig's tail (GROWTH LAW, state.hpp)
// and the organ edits it there — one home authors, one transport carries,
// and the rooms cannot drift. The wander brain's dead reckoning reads the
// same config for the same reason: a plan flown under yesterday's boot
// rests would diverge from the head for a reason that is not the Sky Rule.
//
// THE FRAME-LAW MIRRORS ARE GONE, not withheld. RIBBON_1 deleted the CPU
// head that carried this room's half of them: the frame law is the body
// kernel's alone, so there is no second half to keep in lockstep and no
// hazard to keep a dial out of. RIBBON_BANK_GAIN / RIBBON_BANK_MAX stay
// WGSL consts — hot-reloadable by save, tuned on screen — until a round
// asks for them on the panel.
// ────────────────────────────────────────────────────────────────────

namespace t7 {
namespace the_board {

struct RibbonSurface {
    // ── Head control law ─────────────────────────────────────────
    float yaw_rate;         // rad/s cap at full deflection
    float max_speed;        // world units/s at full throttle
    float r_min;            // minimum turn radius (units) — a DIVISOR
    float climb_rate;       // u/s cap on the pen's vertical velocity
    float floor_margin;     // guaranteed gap over tall ground
    float alt_smooth_dist;  // units of travel over which the altitude target relaxes
    float alt_stiff;        // (rad/s)^2 — the pen's stiffness
    float mount_setback;    // pawn seat setback toward the tail
    float sky_yaw_tau;      // s — first-order ease on the player's yaw hand
    float reference_bpm;    // the tempo at which the tiers' sway is DEFINED
    // ── The Sky Rule (RIBBON_1) — self-preservation, two readers ──
    float lookahead;        // wu ahead of the nose where the head reads the rule
    float clear_head;       // wu of clearance the HEAD keeps from a shell
    float clear_body;       // wu of clearance the BODY keeps from a shell
    float roam_radius;      // wu — the disc around the anchor the wanderer's targets are drawn on
    // ── The mount's two eases (RIBBON_1) ── panel-side, not config: the
    // CPU integrates the phase, the GPU only reads how far along it is.
    float board_seconds;    // s — the ease onto the saddle
    float land_seconds;     // s — the ease off it onto the walked pose
    // ── Wander steering (RIBBON_2: the brain came home to the head
    //    kernel, so these are BOOT RESTS for config.ribbon_wander_* —
    //    the same shape the flight dials took at RIBBON_1). The ease that
    //    used to smooth the brain's own output is gone: its command now
    //    passes through the hands' own tau, which is the hand it is.
    float wander_soft;          // rad of heading error at which the brain asks full yaw
    float wander_yaw_max;       // the brain's share of the hands' cap, [0, 1]
    float wander_arrive;        // wu — a target is reached here; the next is drawn
};

inline constexpr RibbonSurface RIBBON_TABLE = {
    1.0f,     // yaw_rate
    40.0f,    // max_speed — halved; full-throttle turns bottom out at r_min
    40.0f,    // r_min
    15.0f,    // climb_rate
    25.0f,    // floor_margin
    180.0f,   // alt_smooth_dist — the head reads the LANDSCAPE, not the texture
    0.36f,    // alt_stiff — damping = 2*sqrt(stiffness), critically damped
    1.5f,     // mount_setback
    0.6f,     // sky_yaw_tau — short tau keeps the yaw hand immediate
    100.0f,   // reference_bpm
    120.0f,   // lookahead — three seconds of flight at full throttle
    40.0f,    // clear_head — RIBBON_2: wider, because the shell now only advises
    16.0f,    // clear_body — RIBBON_2: the shell is advice now, so it stands wider; the wall is what holds
    400.0f,   // roam_radius — the anchor's disc; a wanderer crosses it target to target and comes back
    1.2f,     // board_seconds
    1.5f,     // land_seconds
    0.5f,     // wander_soft
    0.15f,    // wander_yaw_max — the brain asks at most this much of the hands' cap
    120.0f,   // wander_arrive — inside this the bearing chase degenerates; draw the next
};

inline RibbonSurface RIBBON_LIVE = RIBBON_TABLE;
static_assert(RIBBON_TABLE.roam_radius > RIBBON_TABLE.wander_arrive,
    "a roam disc smaller than the arrival radius would count every target "
    "reached at the instant it is drawn, and the brain would spin the hash");
static_assert(RIBBON_TABLE.r_min > 0.0f,
    "r_min is a divisor in the steering law's min(); the design value must "
    "be positive and the enrollment line must floor the dial above zero");


// ═══ THE SPAWN BANK, GRADUATED (ORGAN_4 P3d) ═════════════════════
//
// C3 DESTRUCTIVE, every row, and that is why it is a SECOND bank rather
// than more fields on the first. RIBBON_LIVE is read every frame by the
// head mover; everything below is read ONCE, as a ribbon is drawn:
// `select_ribbon_for_patch` rolls the spawn chance and the colour mode,
// `fill_ribbon_selection_geometry` draws the colour, `commit_ribbon`
// rolls the wander policy, and none of them looks again. Re-speaking a
// destructive author means tearing down the ribbons that already exist
// to apply a slider, so this bank has NO BOUNDARY WIRING (D5) and its
// enrollment rows wear the GEN chip: the edit lands on the next ribbon.
//
// SIZE COUNTS THAT ARE NOT DIALS STAY IN THE MODULE.
// RIBBON_SMOOTH_PALETTE_COUNT sizes the palette row here and the draw
// there; RibbonColorMode's three ids are a vocabulary, not a range.
inline constexpr uint32_t RIBBON_SMOOTH_PALETTE_COUNT = 4;
inline constexpr uint32_t RIBBON_COLOR_MODE_COUNT = 3;

struct RibbonSpawnSurface {
    // ── The spawn rolls ──────────────────────────────────────────
    float spawn_chance;          // per-patch: does a ribbon happen at all
    float position_jitter;       // anchor scatter inside the trigger patch
    float wander_chance;         // per-spawn: is this one an idle wanderer
    float wander_cruise_base;    // gaussian mean, as a fraction of max_speed
    float wander_cruise_sigma;   // gaussian sigma
    float wander_cruise_min;     // the draw's clamp, low
    float wander_cruise_max;     // the draw's clamp, high
    // THE WAYPOINT ROLLS LEFT WITH THE BRAIN (RIBBON_2). Six dials described
    // a walk the CPU no longer takes: a leg of 200-500 units on a bearing
    // spread around the current motion, re-drawn on a timer, opened by a
    // hatch stride. The head kernel draws a uniform point on the anchor's
    // disc and goes — config.ribbon_roam_radius is the disc, and
    // config.ribbon_wander_arrive is when the next one is drawn. There is no
    // leg, no spread, no timer and no hatch to dial.
    // ── The colour vocabulary ────────────────────────────────────
    float color_weights[RIBBON_COLOR_MODE_COUNT];   // SMOOTH / TINTED / CONTRAST
    float smooth_palette[RIBBON_SMOOTH_PALETTE_COUNT][3];
    float smooth_var_range;      // var = hash * RANGE + BIAS
    float smooth_var_bias;
    float smooth_var_g_scale;    // green gets var * G_SCALE
    float smooth_var_b_scale;    // blue  gets var * B_SCALE
    float tinted_range[3];       // per-channel hash * range + base
    float tinted_base[3];
};

inline constexpr RibbonSpawnSurface RIBBON_SPAWN_TABLE = {
    0.900f,   // spawn_chance    — the module's authored value
    0.3f,     // position_jitter
    0.30f,    // wander_chance
    0.35f,    // wander_cruise_base
    0.15f,    // wander_cruise_sigma
    0.15f,    // wander_cruise_min
    0.80f,    // wander_cruise_max
    { 0.03f, 0.02f, 0.95f },   // color_weights — SMOOTH / TINTED / CONTRAST
    {
        { 0.82f, 0.75f, 0.62f },   // warm sandstone
        { 0.55f, 0.65f, 0.78f },   // sky blue
        { 0.85f, 0.78f, 0.58f },   // golden
        { 0.50f, 0.68f, 0.55f },   // sage green
    },
    0.10f,    // smooth_var_range
    -0.05f,   // smooth_var_bias
    0.8f,     // smooth_var_g_scale
    0.6f,     // smooth_var_b_scale
    { 0.45f, 0.40f, 0.45f },   // tinted_range
    { 0.40f, 0.35f, 0.35f },   // tinted_base
};

inline RibbonSpawnSurface RIBBON_SPAWN_LIVE = RIBBON_SPAWN_TABLE;
static_assert(sizeof(RibbonSpawnSurface)
              == (7 + RIBBON_COLOR_MODE_COUNT
                     + RIBBON_SMOOTH_PALETTE_COUNT * 3 + 4 + 3 + 3) * sizeof(float),
    "RIBBON_SPAWN_LIVE is a whole-struct copy of the design row: a field "
    "added to one is added to the other by construction");
static_assert(RIBBON_SPAWN_TABLE.wander_cruise_min
              <= RIBBON_SPAWN_TABLE.wander_cruise_max,
    "the cruise draw clamps low-then-high; an inverted pair would pin every "
    "wanderer to the low bound and read as a dead gaussian");

} // namespace the_board
} // namespace t7
