#pragma once
#include <cstdint>
#include <array>      // RibbonHead propagation history
#include "cartridges/the_board/realization/state.hpp"                    // Dim::*, GPURibbonState, wgpu
#include "cartridges/the_board/contracts/mood_constants.hpp"   // MOOD_COUNT (sizes the mood gate)
#include "cartridges/the_board/contracts/wgpu_fwd.hpp"   // wgpu handle fwds (lockstep insurance)
#include "cartridges/the_board/contracts/entity_types.hpp"     // RibbonSelection/RibbonPlacement (the boundary DTOs) + queue types
#include "cartridges/the_board/contracts/control_panel.hpp"    // FIELD_SLACK/K/FMAX + the two emitter mutes — the one home

// ─── ribbon.hpp (HEADER: console + vocabulary + state + decls) ───
// History: audit/LADDER.md
//
// Sky Ribbon: complete subsystem (vocabulary + machinery in one module).
//
// THE HEAD IS THE INSTRUMENT; THE BODY IS THE LAW'S CONSEQUENCE.
// Every author — player, wanderer, and (soon) the musical canvas —
// plays the head through one steering integrator and one altitude
// pen; the propagation law replays the head's past down the body at
// P. Couple the head, and the rest follows.
//
// The impl additionally reaches the spawn-engine services
// (run_spawn_preamble, negotiate_position — spawn_engine.hpp), the
// tile_world surface samplers (estimate_terrain_height / terrain_tile_warm),
// seed_utils.hpp, cartridge core (time_state_.seconds/dt/beat_rate,
// THEMES / Dim::PATCH_EXTENT (file-scope vocabulary), the four ribbon
// canvas bindings ride RibbonDeps; the sky trio is OWN state), and the GPU wires
// (upload_ribbon_time / _color / _wave_amps / _head_poses — the flush +
// head laws write through).
//
// SEAM[ribbon:complete-subsystem] complete bespoke pipeline in one
//   module — vocabulary + state + machinery + lifecycle + head laws.
//   Same family as gol_zones (Ch. 12.B) and gallery (Ch. 12.E), and
//   the reference instance of the entity-module pattern: tuning
//   console → registry → tiers → runtime state → author seats →
//   head laws → lifecycle. (surface = the bank rows; the in-module
//   section retired with its last legacy consumer).
// ─────────────────────────────────────────────────────────────────

#include <algorithm>   // std::max, std::min   // (impl, merged)
#include <array>       // body pose staging   // (impl, merged)
#include <cfloat>      // FLT_MAX (nearest-slot adoption)   // (impl, merged)
#include <cmath>       // std::sin, std::cos, std::atan2, std::exp, std::sqrt, std::fabs, std::floor, std::remainder, std::atan   // (impl, merged)
#include <cstdint>   // (impl, merged)
#include <cstring>     // std::memcpy (placement color copy)   // (impl, merged)
#include <iostream>    // the spawn log   // (impl, merged)

namespace t7 { class VisualCanvas; struct TargetBinding; }  // coupling face (t7 scope)

namespace t7 {
namespace the_board {

// ═══ MODULE DEPS ════════════════════════════════════════════════════
// The ribbon conductor's requirements face — including the coupling
// face made typed (the canvas + its four resolved pipes). All reads
// except the GPU wire. (Organ fwds arrive via entity_types.hpp; the
// coupling types are fwd-declared at t7 scope above.)
struct WorldState; struct TileWorldState; struct InputState;
struct RibbonDeps {
    GPUState&            gpuState_;
    const TimeState&     time_state_;
    const TileWorldState& tile_world_state_;
    const PlayerState&   player_;
    const PointState&    point_;    // the point's house (position mirror — nearest-active adoption)
    const InputState&    inputState_;
    const WorldState&    world_state_;
    const MoodState&     mood_state_;
    const VisualCanvas&  visual_canvas_;
    const TargetBinding& ribbon_amp_lat_dst_;
    const TargetBinding& ribbon_amp_vert_dst_;
    const TargetBinding& ribbon_tint_stim_dst_;
    const TargetBinding& ribbon_tint_mix_dst_;
};

// ═══ TUNING CONSOLE ══════════════════════════════════════════════

// ── Spawn ────────────────────────────────────────────────────────
struct RibbonConfig {
    static constexpr float SPAWN_CHANCE = 0.900f;   // authored value (control-panel constant; the distributions pass revisits it)
    // SEAM[ribbon:P4] hygiene rows pattern — the ribbon column of
    //   MOOD_SPAWN_MULT (surface/population_themes.hpp) is
    //   { 1, 1, 1, 1 }, all identity: the mood term suppresses no
    //   row today. (It previously read as carrying indoor zeros; the
    //   live table never had them.) Same family as gol_zones:P4 and
    //   the cube populations' hygiene rows.
    static constexpr float POSITION_JITTER = 0.3f;
};

// ── Length cap ───────────────────────────────────────────────────

// ── Geometry / placement ─────────────────────────────────────────
inline constexpr float MIN_CUBE_COUNT     = 20.0f;    // floor on Gaussian-sampled cube_count
inline constexpr float MIN_CUBE_SIZE      = 1.0f;     // floor on cube_size
inline constexpr float MIN_ADDED_HEIGHT   = 20.0f;    // floor on the clearance draw
inline constexpr float FOOTPRINT_RADIUS   = 5.0f;     // ribbon spawn footprint radius
inline constexpr float ORIENTATION_SPREAD = 1.0472f;  // ±60° (π/3) around away-from-pawn

// ── Head control law ─────────────────────────────────────────────
// The steering integrator and altitude pen constants — one law, many
// authors. Yaw is STEERING, not free aim: available yaw rate is
// min(RIBBON_YAW_RATE, speed / RIBBON_R_MIN), so the heading can only
// change while moving and the flown path can never be tighter than
// the minimum turn radius. Consumed in ribbon_advance_head. All
// control-panel material.
inline constexpr float RIBBON_YAW_RATE       = 1.0f;    // rad/s cap at full deflection
inline constexpr float RIBBON_MAX_SPEED      = 40.0f;   // world units/s at full throttle (halved; full-throttle turns bottom out at R_MIN)
inline constexpr float RIBBON_R_MIN          = 40.0f;   // minimum turn radius (units)
inline constexpr float RIBBON_CLIMB_RATE     = 15.0f;   // u/s cap on the pen's vertical velocity
inline constexpr float RIBBON_FLOOR_MARGIN   = 25.0f;   // guaranteed gap over tall ground
inline constexpr float RIBBON_ALT_SMOOTH_DIST = 180.0f; // units of travel over which the altitude target relaxes — the head reads the LANDSCAPE, not the terrain texture
inline constexpr float RIBBON_ALT_STIFF      = 0.36f;   // (rad/s)^2 — the pen's stiffness; damping = 2*sqrt(stiffness), critically damped
inline constexpr float RIBBON_MOUNT_SETBACK  = 1.5f;    // pawn seat setback toward the tail (+heading) so the body sits over the tube, not the leading cap
inline constexpr float RIBBON_SKY_YAW_TAU    = 0.6f;    // s; first-order ease on the PLAYER's yaw hand — the body replays the heading history, so bang-bang key input must become curves; short tau keeps it immediate
inline constexpr float RIBBON_REFERENCE_BPM  = 100.0f;  // the tempo at which the tiers' authored sway is DEFINED; phase advances at live-tempo/this (control-panel)

// ── Frame-law mirrors ── LOCKSTEP MIRRORS of world.wgsl's
inline constexpr float MOUNT_TANGENT_ALIGN = 1.0f;
inline constexpr float MOUNT_BANK_GAIN     = 0.9f;
inline constexpr float MOUNT_BANK_MAX      = 0.6f;

// ── Wander policy ─────────────────────────────────────────────────
// The steering channel's IDLE SCRIPT — the shape of autonomous drift.
// Constants: control-panel material.
inline constexpr float WANDER_CHANCE      = 0.30f;   // per-spawn roll
inline constexpr float WANDER_CRUISE_BASE  = 0.35f;   // gaussian mean (fraction of RIBBON_MAX_SPEED)
inline constexpr float WANDER_CRUISE_SIGMA = 0.15f;   // gaussian sigma
inline constexpr float WANDER_CRUISE_MIN  = 0.15f;
inline constexpr float WANDER_CRUISE_MAX  = 0.80f;
inline constexpr float WANDER_LEG_MIN     = 200.0f;  // waypoint leg length (units)
inline constexpr float WANDER_LEG_MAX     = 500.0f;
inline constexpr float WANDER_SPREAD      = 1.0f;    // rad of bearing spread around current motion
inline constexpr float WANDER_RETARGET_MIN = 10.0f;  // seconds between waypoints
inline constexpr float WANDER_RETARGET_VAR = 15.0f;
inline constexpr float WANDER_HATCH_LEG   = 300.0f;  // hatchling's first-waypoint leg (units) — the newborn's opening stride, sized between LEG_MIN/MAX's band; consumed in commit_ribbon's hatchling rule
inline constexpr float WANDER_STEER_SOFT  = 0.5f;    // rad of heading error for full deflection
inline constexpr float WANDER_YAW_MAX     = 0.15f;   // yaw cap: radius >= RIBBON_R_MIN/0.15 (~270 u) — body-scale arcs
inline constexpr float WANDER_YAW_TAU     = 2.0f;    // s; first-order ease on the steering — curvature stays continuous (control-panel)
inline constexpr float WANDER_ARRIVE_RADIUS = 120.0f; // u; arrival = retarget — inside this the bearing chase degenerates (control-panel)

// ═══ COLOR VOCABULARY ════════════════════════════════════════════

struct RibbonColorMode {
    static constexpr uint32_t SMOOTH = 0;  // terrain-derived monochrome
    static constexpr uint32_t TINTED = 1;  // warm/cool hue shift
    static constexpr uint32_t CONTRAST = 2;  // cell skin: per-cell coloring — pair-contrast species (two medians, parity) or median-field species (one median, terrain-patch texture); see fill
    static constexpr uint32_t COUNT = 3;
    //                                     SMOOTH TINTED CONTRAST (selection probabilities, sum 1.0)
    static constexpr float WEIGHTS[COUNT] = { 0.40f, 0.35f, 0.25f };
};

// Smooth color palettes: base colors for SMOOTH mode ribbons
inline constexpr float RIBBON_SMOOTH_PALETTE[][3] = {
    { 0.82f, 0.75f, 0.62f },   // warm sandstone
    { 0.55f, 0.65f, 0.78f },   // sky blue
    { 0.85f, 0.78f, 0.58f },   // golden
    { 0.50f, 0.68f, 0.55f },   // sage green
};
inline constexpr uint32_t RIBBON_SMOOTH_PALETTE_COUNT = 4;

// ── Color character ──────────────────────────────────────────────

// SMOOTH: per-channel variance around the palette base.
//   var = hash * RANGE + BIAS; applied as { +var, +var*G, +var*B }.
inline constexpr float SMOOTH_VAR_RANGE = 0.10f;
inline constexpr float SMOOTH_VAR_BIAS  = -0.05f;
inline constexpr float SMOOTH_VAR_G_SCALE = 0.8f; // green channel gets var * G_SCALE
inline constexpr float SMOOTH_VAR_B_SCALE = 0.6f; // blue  channel gets var * B_SCALE

// TINTED: per-channel hash*range + base (R & B range 0.45, G range 0.40).
inline constexpr float TINTED_RANGE[3] = { 0.45f, 0.40f, 0.45f };
inline constexpr float TINTED_BASE[3]  = { 0.40f, 0.35f, 0.35f };

struct CheckerPair {
    float dark[3];
    float light[3];
    float value_var;
    float hue_var;
    float weight;
};
inline constexpr CheckerPair CHECKER_PAIRS[] = {
    { {0.16f,0.15f,0.17f}, {0.88f,0.86f,0.82f}, 0.05f, 0.05f, 0.30f },  // obsidian / bone   — strict
    { {0.30f,0.12f,0.18f}, {0.92f,0.78f,0.80f}, 0.06f, 0.20f, 0.20f },  // wine / rose       — calm
    { {0.14f,0.16f,0.34f}, {0.87f,0.76f,0.58f}, 0.06f, 0.30f, 0.20f },  // indigo / sand     — lively
    { {0.10f,0.24f,0.16f}, {0.78f,0.90f,0.80f}, 0.07f, 0.55f, 0.15f },  // forest / mint     — wild
    { {0.38f,0.18f,0.10f}, {0.90f,0.85f,0.74f}, 0.06f, 0.35f, 0.10f },  // rust / cream      — lively
    { {0.13f,0.13f,0.13f}, {0.92f,0.80f,0.45f}, 0.05f, 0.75f, 0.05f },  // charcoal / gold   — rare riot
};
inline constexpr uint32_t CHECKER_PAIR_COUNT =
    sizeof(CHECKER_PAIRS) / sizeof(CHECKER_PAIRS[0]);
inline constexpr float CHECKER_PAIR_JITTER = 0.03f;  // shared per-ribbon median offset
inline constexpr float CHECKER_HUE_SIBLING_JITTER = 0.10f;  // per-ribbon ± around the pair's hue_var

// FREE RAFFLE — the terrain's discrete-region grammar for the skin: both
// medians raffled as points in (luma, chroma, hue) space, both variances
// raffled. The bounds below ARE the lattice; narrow them to tame, widen to
// liberate. The one kept law: disjoint luma bands preserve the dark/light
// parity under any hue — the chessboard survives its own liberation.
// All control-panel.
inline constexpr float FREE_PAIR_CHANCE   = 0.50f;  // vs the authored pair table
inline constexpr float FREE_DARK_LUMA[2]  = { 0.10f, 0.35f };
inline constexpr float FREE_DARK_CHROMA[2]= { 0.05f, 0.30f };
inline constexpr float FREE_LIGHT_LUMA[2] = { 0.70f, 0.95f };
inline constexpr float FREE_LIGHT_CHROMA[2]={ 0.02f, 0.22f };
inline constexpr float FREE_VALUE_VAR[2]  = { 0.02f, 0.30f };  // raffled, generous ceiling
inline constexpr float FREE_HUE_VAR[2]    = { 0.00f, 1.00f };  // raffled, UNCAPPED (full axis)
// Rodrigues basis about the gray axis (unit chroma + its quadrature) —
// the CPU twin of the shader's hue machinery.
inline constexpr float CHROMA_D1[3] = { 0.8165f, -0.4082f, -0.4082f };
inline constexpr float CHROMA_D2[3] = { 0.0f,     0.7071f, -0.7071f };

inline constexpr float CELLS_MEDIAN_CHANCE   = 0.35f;  // species roll, above the pair fork
inline constexpr float MEDIAN_LUMA[2]        = { 0.25f, 0.85f };
inline constexpr float MEDIAN_CHROMA[2]      = { 0.04f, 0.30f };
inline constexpr float MEDIAN_VALUE_VAR[2]   = { 0.06f, 0.35f };
inline constexpr float MEDIAN_HUE_VAR[2]     = { 0.00f, 1.00f };

// ═══ PROPERTY INDEX REGISTRY ═════════════════════════════════════

struct RibbonProp {
    static constexpr uint32_t SPAWN_ROLL = 400u;
    static constexpr uint32_t ANCHOR_X = 401u;
    static constexpr uint32_t ANCHOR_Z = 402u;
    static constexpr uint32_t TIER = 403u;
    static constexpr uint32_t COLOR_ROLL = 404u;
    static constexpr uint32_t ORIENTATION = 405u;
    static constexpr uint32_t COLOR_R = 406u;
    static constexpr uint32_t COLOR_G = 407u;
    static constexpr uint32_t COLOR_B = 408u;
    static constexpr uint32_t PALETTE_IDX = 409u;
    // Gaussian draw indices
    static constexpr uint32_t CUBE_COUNT = 410u;
    static constexpr uint32_t CUBE_SIZE = 411u;
    static constexpr uint32_t HEIGHT = 412u;
    static constexpr uint32_t LATERAL_AMP = 420u;
    static constexpr uint32_t LATERAL_CYCLES = 421u;
    static constexpr uint32_t VERTICAL_AMP = 430u;
    static constexpr uint32_t CHECKER_PAIR_ROLL = 440u;   // pair raffle
    static constexpr uint32_t CHECKER_JIT_R     = 441u;   // shared median jitter
    static constexpr uint32_t CHECKER_JIT_G     = 442u;
    static constexpr uint32_t CHECKER_JIT_B     = 443u;
    static constexpr uint32_t CHECKER_HUE_JITTER_ROLL = 444u;  // sibling ± around the pair's hue_var
    static constexpr uint32_t FREE_MODE_ROLL   = 460u;  // free vs authored table
    static constexpr uint32_t FREE_DARK_L      = 461u;
    static constexpr uint32_t FREE_DARK_C      = 462u;
    static constexpr uint32_t FREE_DARK_H      = 463u;
    static constexpr uint32_t FREE_LIGHT_L     = 464u;
    static constexpr uint32_t FREE_LIGHT_C     = 465u;
    static constexpr uint32_t FREE_LIGHT_H     = 466u;
    static constexpr uint32_t FREE_VALUE_ROLL  = 467u;
    static constexpr uint32_t FREE_HUE_ROLL    = 468u;
    static constexpr uint32_t MEDIAN_SPECIES_ROLL = 470u;
    static constexpr uint32_t MEDIAN_L            = 471u;
    static constexpr uint32_t MEDIAN_C            = 472u;
    static constexpr uint32_t MEDIAN_H            = 473u;
    static constexpr uint32_t MEDIAN_VALUE_ROLL   = 474u;
    static constexpr uint32_t MEDIAN_HUE_ROLL     = 475u;
    static constexpr uint32_t WANDER_ROLL = 450u;       // wander yes/no
    static constexpr uint32_t WANDER_CRUISE = 451u;     // gaussian draw: cruise fraction of RIBBON_MAX_SPEED
    static constexpr uint32_t WANDER_RNG = 452u;        // seeds the runtime waypoint stream
};

// ═══ TIER PROFILE + MATRIX ═══════════════════════════════════════

inline constexpr uint32_t RIBBON_TIER_COUNT = 3;
//                                        Serpentine  Helix  Streamer (RIBBON_TIERS row order; theme tier_wt_ribbon multiplies these)
inline constexpr float RIBBON_BASE_TIER_WEIGHTS[RIBBON_TIER_COUNT] = {
    0.45f, 0.30f, 0.25f
};

struct RibbonTierProfile {
    // ─── Geometry ────────────────────────────────────────────
    float cube_count_mean, cube_count_sigma;
    float cube_size_mean, cube_size_sigma;

    // ─── Altitude ────────────────────────────────────────────
    float height_mean, height_sigma;

    // ─── Lateral wave ─────────────────────────────────────
    float lateral_amp_mean, lateral_amp_sigma;
    float lateral_cycles_mean, lateral_cycles_sigma;

    // ─── Vertical wave ────────────────────────────────────
    float vertical_amp_mean, vertical_amp_sigma;

    // ─── Propagation (trail-frame head→tail rate, world units/s) ──
    float propagation_speed;

    // ─── Selection ───────────────────────────────────────────
    float weight;
};

// ─── Geometry ────────────┤                │                │                │
// ─── Altitude ────────────┤                │                │                │
// ─── Lateral wave ────────┤                │                │                │
// ─── Vertical wave ───────┤  cycles = lateral (one wavelength, two amps)     │
// ─── Trail-frame ─────────┤                │                │                │
// ─── Selection ───────────┤                │                │                │
//
inline constexpr RibbonTierProfile RIBBON_TIERS[RIBBON_TIER_COUNT] = {
    // Tier 0: Serpentine — long, massive, slow motion
    {    70.0f,  7.0f,      // cube_count
          8.0f,  1.0f,      // cube_size
         110.0f, 15.0f,      // height
         10.0f,  0.6f,      // lateral_amp
          1.2f,  0.3f,      // lateral_cycles
          5.0f,  0.8f,      // vertical_amp
         40.0f,             // propagation_speed (u/s)
          0.45f },          // weight
    // Tier 1: Helix — tight sway, small cubes, slowest propagation
    {    85.0f, 12.0f,      // cube_count
          5.0f,  0.8f,      // cube_size
         105.0f, 12.0f,      // height
          3.5f,  0.6f,      // lateral_amp
          1.8f,  0.5f,      // lateral_cycles
          2.5f,  0.5f,      // vertical_amp
         24.0f,             // propagation_speed (u/s)
          0.30f },          // weight
    // Tier 2: Streamer — tall vertical form, deep breathing
    {    90.0f,  9.0f,      // cube_count
          6.0f,  0.8f,      // cube_size
         120.0f, 20.0f,      // height
          5.5f,  0.8f,      // lateral_amp
          1.5f,  0.4f,      // lateral_cycles
          8.0f,  1.2f,      // vertical_amp
         48.0f,             // propagation_speed (u/s)
          0.25f },          // weight
};

inline constexpr const char* RIBBON_TIER_NAMES[] = {
    "Serpentine", "Helix", "Streamer"
};
inline constexpr const char* RIBBON_COLOR_NAMES[] = {
    "smooth", "tinted", "contrast"
};

// ═══ SPAWN PAYLOADS — AT THE CONTRACT HOME ═══════════════════════
//
// The ribbon Selection/Placement DTOs live in entity_types.hpp,
// beside the EntityQueueEntry / PlacementEntry unions that are their
// reason to exist: a DTO that exists to cross a boundary belongs to
// the boundary's contract, not to either side.

// ═══ RUNTIME STATE ═══════════════════════════════════════════════

// ── Capacity ─────────────────────────────────────────────────────
inline constexpr uint32_t MAX_RIBBON_INSTANCES = 1;  // single-render; raise when GPU supports multi-ribbon
inline constexpr float    RIBBON_MAX_LENGTH = 700.0f;

// ── Per-instance tracking ────────────────────────────────────────
// Two-tip anchoring: ribbon survives until BOTH its tip patches are
// out of view (cross-patch eviction tracking).
struct ActiveRibbon {
    int32_t patch_gx = 0, patch_gz = 0;   // trigger patch
    int32_t host_gx = 0, host_gz = 0;     // host patch (anchor position)
    float anchor_x = 0.0f, anchor_z = 0.0f;
    int32_t near_tip_gx = 0, near_tip_gz = 0;
    int32_t far_tip_gx = 0, far_tip_gz = 0;
    bool near_tip_registered = false;
    bool far_tip_registered = false;
    uint32_t ref_count = 0;     // patches referencing this ribbon via record_entity
    bool active = false;
    float spawn_color[3] = { 0.0f, 0.0f, 0.0f };   // idle base for the line-tint coupling (gen-2): gpu.color = lerp(spawn, stim, mix)
    float spawn_lateral_amp = 0.0f;   // seed-drawn wave amps — the amp pipes'
    float spawn_vertical_amp = 0.0f;  //   idle bases (gpu = base × pipe mult)
    float phase = 0.0f;   // sway phase clock — integrates at the tempo follower's rate (100 BPM ⇒ wall seconds exactly)

    // ── Wander (autonomous drift) ── rolled at commit; a wanderer authors the
    // same yaw/throttle inputs the player does, through the same steering
    // integrator.
    bool     wander = false;
    float    wander_cruise = 0.0f;      // throttle fraction of RIBBON_MAX_SPEED
    float    wander_tx = 0.0f;          // current waypoint (world XZ)
    float    wander_tz = 0.0f;
    float    wander_retarget = 0.0f;    // seconds until a new waypoint
    uint32_t wander_rng = 1u;           // self-contained xorshift state
    float    wander_yaw_state = 0.0f;   // eased steering output (curvature continuity)
};

// ── The head ──────────────────────────────────────────────────────
struct RibbonHead {
    bool     seeded = false;
    uint32_t slot = UINT32_MAX;
    float    origin[3] = { 0.0f, 0.0f, 0.0f };
    float    alt_target = 0.0f;   // low-passed altitude target (landscape swells, not texture)
    float    y_vel = 0.0f;        // the pen's vertical velocity (critically damped follower)
    bool     alt_baked = false;   // birthright latched? (re-bakes until the ground sample is warm)
    float    heading = 0.0f;               // sky-flight heading (yawed by input)
    float    pos[3] = { 0.0f, 0.0f, 0.0f };  // live integrated head position
    float    mount[3] = { 0.0f, 0.0f, 0.0f }; // visible head-ring center + half-tube (pawn mount point)
    float    mount_yaw_off = 0.0f;  // tangent-align yaw deflection (rad)
    float    mount_pitch   = 0.0f;  // tangent-align pitch (rad)
    float    mount_roll    = 0.0f;  // bank into the lateral swing (rad, clamped)

    // ── Propagation history ── the body is the head's past, replayed at
    static constexpr float    HIST_DT  = 0.05f;  // sample cadence (s)
    static constexpr uint32_t HIST_CAP = 1024u;  // ~51 s of past > max body age (~29 s = RIBBON_MAX_LENGTH / slowest tier P — grow this if the cap grows)
    std::array<float, HIST_CAP> hist_heading{};
    std::array<float, HIST_CAP> hist_y{};
    uint32_t hist_head = 0;    // ring buffer: newest sample index
    float    hist_time = 0.0f; // time of the newest sample
};

// ── Ribbon module state ──────────────────────────────────────────
struct RibbonState {
    ActiveRibbon   active[MAX_RIBBON_INSTANCES]{};
    uint32_t       active_count = 0;

    GPURibbonState gpu[MAX_RIBBON_INSTANCES]{};
    uint32_t       rendered_slot = UINT32_MAX;

    // The head mover — see RibbonHead above.
    RibbonHead     head{};

    // ── Sky-flight fixture (Option A): the rider state,
    //    re-homed from PlayerState — the mount was always ribbon-owned.
    //    ROUTING left this record (RESIDUE_3): the host machine
    //    (point_.host == PointHost::RIBBON) routes all consumption;
    //    what stays is the player's eased pen and the one-shot release
    //    request possess() stages for the owning verb. ──
    struct SkyFlight {
        bool  release_pending = false; // possess() staged a RIBBON release; release_sky_exit_ribbon consumes it
        float yaw_eased = 0.0f;        // player's eased yaw
    };
    SkyFlight      sky{};
};

// ═══ MODULE FUNCTIONS — DECLARATIONS ═════════════════════════════

// Lifecycle (three-phase)
bool select_ribbon_for_patch(RibbonState& rs, MachineCtx* c,
    int32_t gx, int32_t gz, RibbonSelection& sel);
bool place_ribbon_from_selection(MachineCtx* c,
    const RibbonSelection& sel, RibbonPlacement& plan);
void commit_ribbon(RibbonState& rs, MachineCtx* c,
    const RibbonPlacement& plan,
    int32_t trigger_gx, int32_t trigger_gz, wgpu::Queue& queue);
// The evictor — MachineCtx-shaped
// to match the FAMILY_DISPATCH evict slot (table in cartridge.hpp, post-class);
// carries the flown-ribbon pin (host == RIBBON) and ref-count law
void evict_ribbon(MachineCtx* self, uint32_t slot, wgpu::Queue& queue);
// Dispatch funnels (table-shaped; the FAMILY_DISPATCH rows point here)
bool dispatch_select_ribbon(MachineCtx* self, int32_t gx, int32_t gz, EntityQueueEntry& e);
bool dispatch_place_ribbon(MachineCtx* self, EntityQueueEntry& e, PlacementEntry& pe);
void dispatch_commit_ribbon(MachineCtx* self, PlacementEntry& pe, wgpu::Queue& queue);
// Frame conductor
void ribbon_frame_tick(RibbonState& rs, RibbonDeps* c, wgpu::Queue& queue);
// Head mover
// DEPS-FORM PRECEDENT: the head mover and its body
// uploader take GPUState& explicitly — born-converted, callable
// without the complete Cartridge.
void ribbon_advance_head(RibbonState& rs, GPUState& gpuState,
    wgpu::Queue& queue, const GPURibbonState& ribbon,
    uint32_t slot, float t,
    bool flown, float yaw_in, float throttle_in, float dt,
    float ground_y, bool ground_valid);
void ribbon_head_pose(const RibbonState& rs, float& x, float& y, float& z, float& heading);
void ribbon_head_frame(const RibbonState& rs, float& yaw_off, float& pitch, float& roll);
void ribbon_head_pen(const RibbonState& rs, float& x, float& z, float& heading);
void ribbon_invalidate_head(RibbonState& rs);
bool ribbon_head_is(const RibbonState& rs, uint32_t slot);
void teardown_ribbon(RibbonState& rs, RibbonDeps* c, wgpu::Queue& queue);
void release_finite_ribbons(RibbonState& rs, RibbonDeps* c, wgpu::Queue& queue);
// Sky-exit death — machine-faced, because it releases the ground.
void release_sky_exit_ribbon(MachineCtx* self, wgpu::Queue& queue);
struct ActivePatch;  // fwd (patch_system.hpp follows this header in the cohort)
void ribbon_register_tips_at(RibbonState& rs, ActivePatch& host, int32_t gx, int32_t gz);
// Shared geometry helper (single entry: the dispatch path)
void fill_ribbon_selection_geometry(uint32_t seed, uint32_t tier_idx,
    RibbonSelection& sel);

// ═══ IMPL:
// bodies deref rs (RibbonState, own via param) + tile faces/player/input/
// canvas/pipes/world/mood via RibbonDeps. COHORT: after visual_canvas.hpp
// (VisualCanvas + TargetBinding complete) + renderer + patch/tile + machine.

// ═══ AUTHOR SEATS ════════════════════════════════════════════════
//
// The steering integrator has three seats, all writing the same two
// inputs (yaw_in, throttle_in): the PLAYER (cartridge frame block,
// the RIBBON host), the WANDERER below (the idle script), and one
// EMPTY SEAT reserved for the musical canvas. One control law, many
// authors.

inline float wander_rand01(uint32_t& s) {
    // xorshift32 → [0,1)
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    return (float)(s & 0x00FFFFFFu) / 16777216.0f;
}

inline void ribbon_wander_inputs(ActiveRibbon& ar,
                                 float head_x, float head_z, float heading,
                                 float dt, float& yaw_in, float& thr_in)
{
    // Free roam: waypoints are picked AHEAD of the current motion — a bearing
    // spread around where the ribbon is already going, a leg of 200-500 units.
    // No leash: a rendered wanderer is pinned against eviction instead, and
    // the yaw cap below keeps every turn at body scale (radius >= RIBBON_R_MIN /
    // WANDER_YAW_MAX). Gorgeous, contemplative arcs by construction.
    ar.wander_retarget -= dt;
    const float wdx = ar.wander_tx - head_x;
    const float wdz = ar.wander_tz - head_z;
    if (ar.wander_retarget <= 0.0f
        || wdx * wdx + wdz * wdz < WANDER_ARRIVE_RADIUS * WANDER_ARRIVE_RADIUS) {
        const float move_dir = heading + 3.14159265f;           // movement = -heading
        const float spread = (wander_rand01(ar.wander_rng) * 2.0f - 1.0f) * WANDER_SPREAD;
        const float leg = WANDER_LEG_MIN
                        + (WANDER_LEG_MAX - WANDER_LEG_MIN) * wander_rand01(ar.wander_rng);
        const float b = move_dir + spread;
        ar.wander_tx = head_x + leg * std::cos(b);
        ar.wander_tz = head_z + leg * std::sin(b);
        ar.wander_retarget = WANDER_RETARGET_MIN
                           + WANDER_RETARGET_VAR * wander_rand01(ar.wander_rng);
    }

    const float bearing = std::atan2(ar.wander_tz - head_z, ar.wander_tx - head_x);
    const float desired = bearing + 3.14159265f;                // movement = -heading
    const float err = std::remainder(desired - heading, 6.2831853f);
    float cmd = err / WANDER_STEER_SOFT;
    cmd = (cmd >  WANDER_YAW_MAX) ?  WANDER_YAW_MAX :
          (cmd < -WANDER_YAW_MAX) ? -WANDER_YAW_MAX : cmd;
    // Ease the steering: the body replays the heading history, and bang-bang
    // commands print elbows. First-order toward the command keeps curvature
    // continuous — turns enter and exit as curves, never as joints.
    const float ease = 1.0f - std::exp(-dt / WANDER_YAW_TAU);
    ar.wander_yaw_state += (cmd - ar.wander_yaw_state) * ease;
    yaw_in = ar.wander_yaw_state;
    thr_in = ar.wander_cruise;
}

// ═══ HEAD LAWS ═══════════════════════════════════════════════════
//
// The head mover: the steering integrator, the altitude pen, the
// propagation history, and THE LAW that rebuilds the body from it.
// CPU authors intent; the GPU realizes geometry — these laws write
// the GPU exactly once per frame, through
// GPUState::upload_ribbon_head_poses (a dumb wire).

inline void ribbon_history_sample(const RibbonState& rs, float age, float& h, float& y) {
    const RibbonHead& hd = rs.head;
    float fidx = age / RibbonHead::HIST_DT;
    if (fidx < 0.0f) fidx = 0.0f;
    const float fmax = static_cast<float>(RibbonHead::HIST_CAP - 2u);
    if (fidx > fmax) fidx = fmax;
    const uint32_t j0 = static_cast<uint32_t>(fidx);
    const float frac = fidx - static_cast<float>(j0);
    const uint32_t i0 = (hd.hist_head + RibbonHead::HIST_CAP - j0) % RibbonHead::HIST_CAP;
    const uint32_t i1 = (hd.hist_head + RibbonHead::HIST_CAP - (j0 + 1u)) % RibbonHead::HIST_CAP;
    h = hd.hist_heading[i0] + (hd.hist_heading[i1] - hd.hist_heading[i0]) * frac;
    y = hd.hist_y[i0]       + (hd.hist_y[i1]       - hd.hist_y[i0])       * frac;
}

// THE LAW: the body is the head's past, replayed at propagation
// speed. Ring k wears the head's state from age = k·spacing/P
// seconds ago — its delayed heading orients the segment and fills
// the yaw channel (continuous by construction); its delayed y is
// the altitude. XZ integrates the delayed heading TAILWARD
// (+heading) from the live head. A turn made now travels down the
// body at P through space; so does an altitude swell; so will
// every musical gesture at the head. Replaces the spatial trail:
// a bend is motion, not a mark on the floor.
inline void ribbon_rebuild_body_upload(RibbonState& rs, GPUState& gpuState,
                                       wgpu::Queue& queue, const GPURibbonState& ribbon,
                                       float head_x, float head_y, float head_z) {
    const uint32_t n = std::min(ribbon.cube_count, Dim::RIBBON_MAX_RINGS);
    if (n < 2u) return;
    const float spacing = ribbon.cube_size;
    const float inv_p = 1.0f / std::max(ribbon.propagation_speed, 0.001f);

    std::array<float, 4 * Dim::RIBBON_MAX_RINGS> poses{};
    poses[0] = head_x;
    poses[1] = head_y;
    poses[2] = head_z;
    poses[3] = rs.head.heading;   // ring 0: the live head, live heading

    float px = head_x, pz = head_z;
    for (uint32_t k = 1u; k < n; ++k) {
        const float age = (static_cast<float>(k) * spacing) * inv_p;
        float h, y;
        ribbon_history_sample(rs, age, h, y);
        px += spacing * std::cos(h);   // tailward = +heading
        pz += spacing * std::sin(h);
        poses[4u * k + 0u] = px;
        poses[4u * k + 1u] = y;
        poses[4u * k + 2u] = pz;
        poses[4u * k + 3u] = h;        // yaw channel: the delayed heading
    }
    // OIL_1 U3 (ledger: R7 upload width, C8): ship the FILLED prefix,
    // not the 400-ring array — 4n floats is the exact GPU read-set.
    // THE PAIRING INVARIANT this rests on: every GPU-visible RAISE of
    // ribbonBuffer_.cube_count must be followed, in the same queue
    // order, by a pose write at least that wide. It holds because the
    // one live-value upload_ribbon (the slot-switch arm of
    // ribbon_frame_tick) is followed by ribbon_advance_head, whose tail
    // is this writer, and this function has no early return past the
    // n < 2 guard; the other upload_ribbon sites all ship a zeroed
    // state (cube_count 0), which only ever shrinks the read window.
    // The full-width write used to make this self-healing by re-zeroing
    // the tail every frame — it no longer does, so a new upload_ribbon
    // call site owes a pose write beside it.
    // Every reader bounds by the live cube_count: compute_ribbon_rings
    // early-outs at ring_idx >= cube_count, centerline/spine sample
    // i0,i1 <= cube_count-1, and the field's ring loop runs to
    // min(cube_count, 400). Bytes past 4n are unreachable.
    gpuState.upload_ribbon_head_poses(queue, poses.data(),
                                      4ull * n * sizeof(float));
}

inline void ribbon_advance_head(RibbonState& rs, GPUState& gpuState,
                                wgpu::Queue& queue, const GPURibbonState& ribbon,
                                uint32_t slot, float t,
                                bool flown, float yaw_in, float throttle_in, float dt,
                                float ground_y, bool ground_valid) {
    RibbonHead& hd = rs.head;

    if (!hd.seeded || hd.slot != slot) {
        hd.origin[0] = ribbon.anchor[0];
        hd.origin[2] = ribbon.anchor[2];
        hd.heading = ribbon.orientation;
        hd.pos[0] = hd.origin[0];
        hd.pos[2] = hd.origin[2];
        hd.hist_heading.fill(hd.heading);
        hd.hist_head = 0;
        hd.hist_time = t;
        hd.alt_baked = false;   // altitude bakes below, latching on the first warm ground sample
        hd.seeded = true;
        hd.slot = slot;
    }

    // THE BAKE — altitude is a birthright: the ground of the birthplace
    // plus the seed-drawn clearance (ribbon.height), baked ONCE at first
    // truth, never chased. After a mood flip the estimator returns 0 for
    // a still-cold tile — indistinguishable from flat ground by value —
    // so the bake re-runs each frame until the call site reports a WARM
    // sample, then latches. Parked ribbons hold this number forever.
    if (!hd.alt_baked) {
        hd.origin[1]  = ground_y + ribbon.height;   // first truth, once
        hd.pos[1]     = hd.origin[1];
        hd.alt_target = hd.origin[1];
        hd.y_vel      = 0.0f;
        hd.hist_y.fill(hd.pos[1]);   // the body's constant past re-bases with the bake
        hd.alt_baked  = ground_valid;
    }

    float head_x, head_y, head_z;
    if (flown) {
        // Planar flight: yaw the heading, throttle the head along it in
        // the horizontal plane. The head moves along -heading so the
        // straight seed (laid +heading from the anchor) trails behind it.
        // Pitch is deferred (the frame is horizontal-only by
        // construction); altitude is managed by the pen below. Constants
        // live in the tuning console (head control law).
        // Steering model: yaw is STEERING, not free aim. The available
        // yaw rate is min(RIBBON_YAW_RATE, speed / RIBBON_R_MIN): the
        // heading can only change while moving, and the flown path can
        // never be tighter than the minimum turn radius. So the velocity
        // is always the face's outward normal (trajectory orthogonal to
        // the face, by construction), the face can never turn inward
        // against the body, and heading-vs-path divergence stays at a
        // few degrees. Reverse is forbidden (a snake does not burrow
        // into its own body): S (reverse intent) is no thrust — the ribbon's forward grammar.
        const float speed = std::max(throttle_in, 0.0f) * RIBBON_MAX_SPEED;
        const float yaw_avail = std::min(RIBBON_YAW_RATE, speed / RIBBON_R_MIN);
        hd.heading += yaw_in * yaw_avail * dt;
        const float ch = std::cos(hd.heading);
        const float sh = std::sin(hd.heading);
        const float step = speed * dt;
        hd.pos[0] -= ch * step;
        hd.pos[2] -= sh * step;
        // Sky altitude with a terrain FLOOR (not a tether): the target is
        // the ribbon's baked birthright altitude, floored by the smoothed
        // ground plus a constant safety margin. Valleys and mood changes
        // leave it untouched at its sky; only ground tall enough to
        // threaten it lifts it — and it settles back beyond. The low-pass
        // keeps the floor reading the LANDSCAPE (long swells); zero travel
        // (hover) freezes the target; the critically damped PEN below
        // turns every correction into a smooth S-curve — never a
        // constant-rate ramp, never a corner.
        {
            const float floor_y = ground_y + RIBBON_FLOOR_MARGIN;
            const float raw_target = (hd.origin[1] > floor_y)
                                   ? hd.origin[1] : floor_y;
            const float travel = std::fabs(step);   // this frame's distance
            const float alpha = 1.0f - std::exp(-travel / RIBBON_ALT_SMOOTH_DIST);
            hd.alt_target += (raw_target - hd.alt_target) * alpha;
            // FIELD_B0: the floor's guarantee, re-imposed AFTER
            // every field contribution — the lure is lateral by law,
            // but repulsion's fy is not, and a standing ceiling of
            // gathered cubes drove the target under the world
            // (Gate F's second lesson). The constant's own comment
            // says "guaranteed gap"; this line makes it true.
            if (hd.alt_target < floor_y) { hd.alt_target = floor_y; }

            const float damp = 2.0f * std::sqrt(RIBBON_ALT_STIFF);
            hd.y_vel += ((hd.alt_target - hd.pos[1]) * RIBBON_ALT_STIFF
                         - damp * hd.y_vel) * dt;
            hd.y_vel = (hd.y_vel >  RIBBON_CLIMB_RATE) ?  RIBBON_CLIMB_RATE :
                       (hd.y_vel < -RIBBON_CLIMB_RATE) ? -RIBBON_CLIMB_RATE : hd.y_vel;
            hd.pos[1] += hd.y_vel * dt;
        }

        head_x = hd.pos[0];
        head_y = hd.pos[1];
        head_z = hd.pos[2];
    } else {
        hd.heading = ribbon.orientation;
        hd.pos[0] = ribbon.anchor[0];
        hd.pos[1] = hd.origin[1];
        hd.pos[2] = ribbon.anchor[2];
        head_x = hd.pos[0];
        head_y = hd.pos[1];
        head_z = hd.pos[2];
    }

    // Pawn mount point (the RIBBON host): the SEAT — centerline + the wave in
    // the ring frame, sampled at the SADDLE's arc age (s_age), lifted
    // half a tube along the seat frame's up (seat polish) so the pawn's
    // feet ride the top face through roll and pitch. Mirrors
    // ribbon_spine_at at the seat's arc: ring 0's yaw-channel value IS
    // the live heading (head_poses[0].w), so right = (-sin h, 0, cos h),
    // wave vertical on world-up. The pawn, its frame, and the tube
    // beneath the seat share one wave at one age — the saddle's —
    // exactly.
    {
        const float p_spd = std::max(ribbon.propagation_speed, 1e-3f);
        const float s_age = ribbon.time - RIBBON_MOUNT_SETBACK / p_spd;
        const float lat = std::sin(ribbon.lateral_freq  * s_age) * ribbon.lateral_amp;
        const float ver = std::sin(ribbon.vertical_freq * s_age) * ribbon.vertical_amp;
        const float ch  = std::cos(hd.heading);
        const float sh  = std::sin(hd.heading);

        const float sl_lat = std::cos(ribbon.lateral_freq  * s_age)
                           * ribbon.lateral_amp  * ribbon.lateral_freq;
        const float sl_ver = std::cos(ribbon.vertical_freq * s_age)
                           * ribbon.vertical_amp * ribbon.vertical_freq;
        // Negated tangent-align, in LOCKSTEP with the GPU ring motor
        // (the drift-trap resolution: the sweep test flipped the shader
        // side; this mirror flips with it, same commit).
        hd.mount_yaw_off = -MOUNT_TANGENT_ALIGN * std::atan(sl_lat / p_spd);
        hd.mount_pitch   = -MOUNT_TANGENT_ALIGN * std::atan(sl_ver / p_spd);
        const float bank = MOUNT_BANK_GAIN * (sl_lat / p_spd);
        hd.mount_roll = (bank >  MOUNT_BANK_MAX) ?  MOUNT_BANK_MAX :
                        (bank < -MOUNT_BANK_MAX) ? -MOUNT_BANK_MAX : bank;

        // Seat lift along the FRAME's up (seat polish, ruled): the top
        // face tilts with roll/pitch (BNK-1); a world-vertical lift
        // sinks the feet by half·(1/cos tilt − 1) mid-swing. Rotate ŷ
        // through the same roll → pitch → yaw the GPU frames use —
        // closed form verified against the quaternion path to 1e-12.
        // Identity at a level frame: u = world up exactly.
        const float cr = std::cos(hd.mount_roll),  sr = std::sin(hd.mount_roll);
        const float cp = std::cos(hd.mount_pitch), sp = std::sin(hd.mount_pitch);
        const float uxl = -sp * cr;   // local up after roll, then pitch
        const float uyl =  cp * cr;
        const float uzl =  sr;
        const float thy = -hd.heading - hd.mount_yaw_off;
        const float cy = std::cos(thy), sy = std::sin(thy);
        const float ux =  uxl * cy + uzl * sy;   // base yaw into world
        const float uz = -uxl * sy + uzl * cy;
        const float half_t = ribbon.cube_size * 0.5f;

        hd.mount[0] = head_x + lat * (-sh) + RIBBON_MOUNT_SETBACK * ch + half_t * ux;
        hd.mount[1] = head_y + ver                                    + half_t * uyl;
        hd.mount[2] = head_z + lat * ( ch) + RIBBON_MOUNT_SETBACK * sh + half_t * uz;
    }

    // Record the head's state into the propagation history (catch-up
    // at fixed cadence; a frame hitch holds the current state across
    // the gap — the past never has holes).
    while (t - hd.hist_time >= RibbonHead::HIST_DT) {
        hd.hist_head = (hd.hist_head + 1u) % RibbonHead::HIST_CAP;
        hd.hist_heading[hd.hist_head] = hd.heading;
        hd.hist_y[hd.hist_head]       = hd.pos[1];
        hd.hist_time += RibbonHead::HIST_DT;
    }

    ribbon_rebuild_body_upload(rs, gpuState, queue, ribbon, head_x, head_y, head_z);
}

inline void ribbon_invalidate_head(RibbonState& rs) { rs.head.seeded = false; }

inline bool ribbon_head_is(const RibbonState& rs, uint32_t slot) {
    return rs.head.seeded && rs.head.slot == slot;
}

// Last computed ribbon head pose — the SADDLE (mount): pen + wave +
// setback. Read by the pawn mount and camera (the RIBBON host).
inline void ribbon_head_pose(const RibbonState& rs, float& x, float& y, float& z, float& heading) {
    x = rs.head.mount[0];
    y = rs.head.mount[1];
    z = rs.head.mount[2];
    heading = rs.head.heading;
}

inline void ribbon_head_frame(const RibbonState& rs, float& yaw_off, float& pitch, float& roll) {
    yaw_off = rs.head.mount_yaw_off;
    pitch   = rs.head.mount_pitch;
    roll    = rs.head.mount_roll;
}

// The PEN — the true integrated head. Steering reads this; the
// mount above is the SADDLE (pen + wave + setback), for the pawn
// and camera only. Two consumers, two truths, never mixed.
inline void ribbon_head_pen(const RibbonState& rs, float& x, float& z, float& heading) {
    x = rs.head.pos[0];
    z = rs.head.pos[2];
    heading = rs.head.heading;
}

// ═══ FRAME ORCHESTRATION ═════════════════════════════════════════
//
inline void ribbon_frame_tick(RibbonState& rs, RibbonDeps* c, wgpu::Queue& queue) {

    // Phase clock on all CPU mirrors — MUSICAL TIME: the
    // sway integrates at the tempo follower's rate, scaled
    // so 100 BPM reproduces wall seconds exactly (the
    // calibration anchor). Held-last: transport stops, the
    // world keeps the pulse.
    const float phase_rate = c->time_state_.beat_rate
                           * (60.0f / RIBBON_REFERENCE_BPM);
    for (uint32_t i = 0; i < MAX_RIBBON_INSTANCES; i++) {
        auto& par = rs.active[i];
        if (!par.active) continue;
        par.phase += phase_rate * c->time_state_.dt;
        rs.gpu[i].time = par.phase;
        {
            const VisualParams& vp = c->visual_canvas_.params();
            const float ml = c->ribbon_amp_lat_dst_.valid
                ? vp.get(c->ribbon_amp_lat_dst_.base)  : 1.0f;
            const float mv = c->ribbon_amp_vert_dst_.valid
                ? vp.get(c->ribbon_amp_vert_dst_.base) : 1.0f;
            rs.gpu[i].lateral_amp  = rs.active[i].spawn_lateral_amp  * ml;
            rs.gpu[i].vertical_amp = rs.active[i].spawn_vertical_amp * mv;

            // Line tint (color gen-2): gpu.color = lerp(spawn, stim, mix).
            // Rest = mix 0 = the seed-drawn color exactly; the held-slot
            // upload_ribbon_color line ships it unchanged.
            const float mix = c->ribbon_tint_mix_dst_.valid
                ? vp.get(c->ribbon_tint_mix_dst_.base) : 0.0f;
            const float* st = c->ribbon_tint_stim_dst_.valid
                ? vp.run(c->ribbon_tint_stim_dst_.base) : nullptr;
            for (int c2 = 0; c2 < 3; ++c2) {
                const float s = st ? st[c2] : 0.0f;
                rs.gpu[i].color[c2] =
                    par.spawn_color[c2]
                    + (s - par.spawn_color[c2]) * mix;
            }
        }
    }

    // (The sky-exit release ran at the head of this phase — it is a
    // ribbon DEATH and owes the ground back, so it needs the machine
    // face this tick does not carry: release_sky_exit_ribbon.)

    // Render one ribbon: hold the current slot until it's evicted,
    // then pick the nearest active ribbon as the new rendered slot.
    bool current_alive = rs.rendered_slot != UINT32_MAX
        && rs.active[rs.rendered_slot].active;

    // Flight input for the head mover: the player when the RIBBON
    // hosts the point; the wander policy when the rendered ribbon is a
    // wanderer; parked otherwise. One control law, many authors — the
    // gate reads the ONE host value (RESIDUE_3).
    bool  ribbon_flown  = (c->point_.host == PointHost::RIBBON);
    float ribbon_yaw_in = ribbon_flown ?  c->inputState_.move_x : 0.0f;
    float ribbon_thr_in = ribbon_flown ? -c->inputState_.move_z : 0.0f;
    // The player's pen, eased like the wanderer's (RIBBON_SKY_YAW_TAU
    // in the tuning console).
    {
        if (ribbon_flown) {
            const float a = 1.0f - std::exp(-c->time_state_.dt / RIBBON_SKY_YAW_TAU);
            rs.sky.yaw_eased += (ribbon_yaw_in - rs.sky.yaw_eased) * a;
            ribbon_yaw_in = rs.sky.yaw_eased;
        } else {
            rs.sky.yaw_eased = 0.0f;
        }
    }
    if (!ribbon_flown && current_alive
        && ribbon_head_is(rs, rs.rendered_slot)
        && rs.active[rs.rendered_slot].wander) {
        float whx, whz, whh;
        ribbon_head_pen(rs, whx, whz, whh);
        ribbon_wander_inputs(
            rs.active[rs.rendered_slot],
            whx, whz, whh, c->time_state_.dt,
            ribbon_yaw_in, ribbon_thr_in);
        ribbon_flown = true;
    }

    if (current_alive) {
        // Hold — update time + color. The gen-1 §5 coupling retired; the
        // gen-2 line tint (color_stim × color_mix over spawn; see
        // coupling_layer_migration_map.md) now computes the color per
        // frame in the flush loop above, and this upload ships that lerp
        // (rest = mix 0 = the seed-drawn color exactly).
        c->gpuState_.upload_ribbon_time(queue,
            rs.gpu[rs.rendered_slot].time);
        c->gpuState_.upload_ribbon_color(queue,
            rs.gpu[rs.rendered_slot].color);
        c->gpuState_.upload_ribbon_wave_amps(queue,
            rs.gpu[rs.rendered_slot].lateral_amp,
            rs.gpu[rs.rendered_slot].vertical_amp);
        // The head mover must run EVERY frame for the held ribbon,
        // not only on slot eviction. Without this the trail is seeded
        // straight once and never advances, so the ribbon looks
        // stationary despite is_roaming = 1. The mover's init guard
        // (slot unchanged) makes this a pure per-frame advance — no
        // re-seed, no double work with the eviction-branch call below.
        float rib_gnd;
        bool  rib_gnd_valid;
        {
            const auto& rb = rs.gpu[rs.rendered_slot];
            float gx = rb.anchor[0], gz = rb.anchor[2];
            if (ribbon_head_is(rs, rs.rendered_slot)) {
                float hy, hh; ribbon_head_pose(rs, gx, hy, gz, hh);
            }
            rib_gnd = estimate_terrain_height(c->tile_world_state_, gx, gz);
            rib_gnd_valid = terrain_tile_warm(c->tile_world_state_, gx, gz);
        }
        ribbon_advance_head(rs, c->gpuState_, queue,
            rs.gpu[rs.rendered_slot],
            rs.rendered_slot, c->time_state_.seconds,
            ribbon_flown, ribbon_yaw_in, ribbon_thr_in, c->time_state_.dt,
            rib_gnd, rib_gnd_valid);
    }
    else {
        // Current slot is gone — find nearest active ribbon
        uint32_t nearest = UINT32_MAX;
        float nearest_d2 = FLT_MAX;
        for (uint32_t i = 0; i < MAX_RIBBON_INSTANCES; i++) {
            if (!rs.active[i].active) continue;
            float dx = rs.active[i].anchor_x - c->point_.x;
            float dz = rs.active[i].anchor_z - c->point_.z;
            float d2 = dx * dx + dz * dz;
            if (d2 < nearest_d2) { nearest = i; nearest_d2 = d2; }
        }

        if (nearest != UINT32_MAX) {
            c->gpuState_.upload_ribbon(queue, rs.gpu[nearest]);
            float rib_gnd;
            bool  rib_gnd_valid;
            {
                const auto& rb = rs.gpu[nearest];
                float gx = rb.anchor[0], gz = rb.anchor[2];
                if (ribbon_head_is(rs, nearest)) {
                    float hy, hh; ribbon_head_pose(rs, gx, hy, gz, hh);
                }
                rib_gnd = estimate_terrain_height(c->tile_world_state_, gx, gz);
                rib_gnd_valid = terrain_tile_warm(c->tile_world_state_, gx, gz);
            }
            ribbon_advance_head(rs, c->gpuState_, queue, rs.gpu[nearest], nearest, c->time_state_.seconds,
                ribbon_flown, ribbon_yaw_in, ribbon_thr_in, c->time_state_.dt,
                rib_gnd, rib_gnd_valid);  // 2b: head mover
            rs.rendered_slot = nearest;
        }
        else if (rs.rendered_slot != UINT32_MAX) {
            GPURibbonState empty{};
            c->gpuState_.upload_ribbon(queue, empty);
            rs.rendered_slot = UINT32_MAX;
        }
    }

    // ── The sky resync, the tick's tail (O-1 holds BY CONSTRUCTION: the head
    // advanced above, dispatch_compute follows the tick in the score,
    // and queue writes apply in submission order). Re-writes the
    // sky_* signal block so the pawn and the ribbon are sampled at
    // the same frame — the one-frame mount lag disappears, leaving
    // MOUNT_SETBACK as the sole seat offset. update() ships neutral
    // zeros; THIS is the authoritative author.
    {
        float hx, hy, hz, hh;
        ribbon_head_pose(rs, hx, hy, hz, hh);
        float fyaw, fpitch, froll;
        ribbon_head_frame(rs, fyaw, fpitch, froll);
        c->gpuState_.resync_sky_head(queue,
                                     c->point_.host == PointHost::RIBBON ? 1u : 0u,
                                     hx, hy, hz, hh, fyaw, fpitch, froll);
    }
}

// ═══ LIFECYCLE — three-phase + shared helper ═════════════════════

// ─── fill_ribbon_selection_geometry ───────────────────────────
// Geometry + color sampler for the dispatch pipeline.
// Pure: no ribbon state access; all sampling is from `seed`.
inline void fill_ribbon_selection_geometry(
    uint32_t seed, uint32_t tier_idx,
    RibbonSelection& sel)
{
    const auto& tp = RIBBON_TIERS[tier_idx];

    float count_f = std::max(MIN_CUBE_COUNT,
        cpu_sample_gaussian(seed, RibbonProp::CUBE_COUNT, tp.cube_count_mean, tp.cube_count_sigma));
    sel.cube_count = std::min((uint32_t)count_f, Dim::RIBBON_MAX_RINGS);
    sel.cube_size = std::max(MIN_CUBE_SIZE,
        cpu_sample_gaussian(seed, RibbonProp::CUBE_SIZE, tp.cube_size_mean, tp.cube_size_sigma));

    // Length cap — keeps anchor coverage viable (700 u = 14 patches)
    if ((float)sel.cube_count * sel.cube_size > RIBBON_MAX_LENGTH)
        sel.cube_count = (uint32_t)(RIBBON_MAX_LENGTH / sel.cube_size);

    // The seed draw is this ribbon's CLEARANCE above its birthplace — pure
    // from seed; the ground joins once, at head init (ribbon_advance_head).
    sel.height = std::max(MIN_ADDED_HEIGHT,
        cpu_sample_gaussian(seed, RibbonProp::HEIGHT, tp.height_mean, tp.height_sigma));

    sel.orientation = cpu_hash_f(seed, RibbonProp::ORIENTATION) * 6.2831853f;

    sel.lateral_amp = std::max(0.1f, cpu_sample_gaussian(seed, RibbonProp::LATERAL_AMP, tp.lateral_amp_mean, tp.lateral_amp_sigma));
    sel.lateral_cycles = std::max(0.1f, cpu_sample_gaussian(seed, RibbonProp::LATERAL_CYCLES, tp.lateral_cycles_mean, tp.lateral_cycles_sigma));

    sel.vertical_amp = std::max(0.1f, cpu_sample_gaussian(seed, RibbonProp::VERTICAL_AMP, tp.vertical_amp_mean, tp.vertical_amp_sigma));

    // Color
    sel.color_mode = select_tier(seed, RibbonProp::COLOR_ROLL,
        RibbonColorMode::WEIGHTS, RibbonColorMode::COUNT);

    if (sel.color_mode == RibbonColorMode::SMOOTH) {
        uint32_t pal_idx = (uint32_t)(cpu_hash_f(seed, RibbonProp::PALETTE_IDX) * RIBBON_SMOOTH_PALETTE_COUNT);
        if (pal_idx >= RIBBON_SMOOTH_PALETTE_COUNT) pal_idx = RIBBON_SMOOTH_PALETTE_COUNT - 1;
        float var = cpu_hash_f(seed, RibbonProp::COLOR_R) * SMOOTH_VAR_RANGE + SMOOTH_VAR_BIAS;
        sel.color[0] = RIBBON_SMOOTH_PALETTE[pal_idx][0] + var;
        sel.color[1] = RIBBON_SMOOTH_PALETTE[pal_idx][1] + var * SMOOTH_VAR_G_SCALE;
        sel.color[2] = RIBBON_SMOOTH_PALETTE[pal_idx][2] + var * SMOOTH_VAR_B_SCALE;
    }
    else if (sel.color_mode == RibbonColorMode::TINTED) {
        sel.color[0] = cpu_hash_f(seed, RibbonProp::COLOR_R) * TINTED_RANGE[0] + TINTED_BASE[0];
        sel.color[1] = cpu_hash_f(seed, RibbonProp::COLOR_G) * TINTED_RANGE[1] + TINTED_BASE[1];
        sel.color[2] = cpu_hash_f(seed, RibbonProp::COLOR_B) * TINTED_RANGE[2] + TINTED_BASE[2];
    }
    else {
        if (cpu_hash_f(seed, RibbonProp::MEDIAN_SPECIES_ROLL) < CELLS_MEDIAN_CHANCE) {
            // MEDIAN-FIELD: one raffled median; color_b == color kills the
            // parity; texture (value + hue machinery) carries the cells.
            const auto lerpf = [](const float b[2], float t) {
                return b[0] + (b[1] - b[0]) * t; };
            const float ang = cpu_hash_f(seed, RibbonProp::MEDIAN_H) * 6.2831853f;
            const float ca = std::cos(ang), sa = std::sin(ang);
            const float luma   = lerpf(MEDIAN_LUMA,   cpu_hash_f(seed, RibbonProp::MEDIAN_L));
            const float chroma = lerpf(MEDIAN_CHROMA, cpu_hash_f(seed, RibbonProp::MEDIAN_C));
            for (int i = 0; i < 3; ++i) {
                sel.color[i] = luma + (CHROMA_D1[i]*ca + CHROMA_D2[i]*sa) * chroma;
                sel.color_b[i] = sel.color[i];
            }
            sel.checker_scatter = lerpf(MEDIAN_VALUE_VAR,
                cpu_hash_f(seed, RibbonProp::MEDIAN_VALUE_ROLL));
            sel.checker_hue_spread = lerpf(MEDIAN_HUE_VAR,
                cpu_hash_f(seed, RibbonProp::MEDIAN_HUE_ROLL)) * 3.14159265f;
        } else if (cpu_hash_f(seed, RibbonProp::FREE_MODE_ROLL) < FREE_PAIR_CHANCE) {
            // FREE RAFFLE — both medians as raffled (luma, chroma, hue)
            // points; both variances raffled. lerp helper inline.
            const auto lerpf = [](const float b[2], float t) {
                return b[0] + (b[1] - b[0]) * t; };
            const auto median = [&](float luma, float chroma, float ang,
                                    float out[3]) {
                const float ca = std::cos(ang), sa = std::sin(ang);
                for (int i = 0; i < 3; ++i)
                    out[i] = luma + (CHROMA_D1[i]*ca + CHROMA_D2[i]*sa) * chroma;
            };
            median(lerpf(FREE_DARK_LUMA,  cpu_hash_f(seed, RibbonProp::FREE_DARK_L)),
                   lerpf(FREE_DARK_CHROMA,cpu_hash_f(seed, RibbonProp::FREE_DARK_C)),
                   cpu_hash_f(seed, RibbonProp::FREE_DARK_H) * 6.2831853f,
                   sel.color);
            median(lerpf(FREE_LIGHT_LUMA,  cpu_hash_f(seed, RibbonProp::FREE_LIGHT_L)),
                   lerpf(FREE_LIGHT_CHROMA,cpu_hash_f(seed, RibbonProp::FREE_LIGHT_C)),
                   cpu_hash_f(seed, RibbonProp::FREE_LIGHT_H) * 6.2831853f,
                   sel.color_b);
            sel.checker_scatter = lerpf(FREE_VALUE_VAR,
                cpu_hash_f(seed, RibbonProp::FREE_VALUE_ROLL));
            sel.checker_hue_spread = lerpf(FREE_HUE_VAR,
                cpu_hash_f(seed, RibbonProp::FREE_HUE_ROLL)) * 3.14159265f;
        } else {
            // The pair raffle — cumulative-weight pick, the terrain's roll and
            // SMOOTH's pick, one mechanism.
            float w[CHECKER_PAIR_COUNT];
            for (uint32_t i = 0; i < CHECKER_PAIR_COUNT; ++i) w[i] = CHECKER_PAIRS[i].weight;
            uint32_t pick = select_tier(seed, RibbonProp::CHECKER_PAIR_ROLL, w, CHECKER_PAIR_COUNT);
            const CheckerPair& pr = CHECKER_PAIRS[pick];
            // One shared jitter moves both medians together: siblings differ,
            // the pair's designed contrast survives.
            const float jr = (cpu_hash_f(seed, RibbonProp::CHECKER_JIT_R) - 0.5f) * 2.0f * CHECKER_PAIR_JITTER;
            const float jg = (cpu_hash_f(seed, RibbonProp::CHECKER_JIT_G) - 0.5f) * 2.0f * CHECKER_PAIR_JITTER;
            const float jb = (cpu_hash_f(seed, RibbonProp::CHECKER_JIT_B) - 0.5f) * 2.0f * CHECKER_PAIR_JITTER;
            sel.color[0]   = pr.dark[0]  + jr;  sel.color_b[0] = pr.light[0] + jr;
            sel.color[1]   = pr.dark[1]  + jg;  sel.color_b[1] = pr.light[1] + jg;
            sel.color[2]   = pr.dark[2]  + jb;  sel.color_b[2] = pr.light[2] + jb;
            sel.checker_scatter = pr.value_var;
            {
                // hue_spread (radians, [0, pi]) = the pair's authored hue_var,
                // sibling-jittered per ribbon, scaled onto the shader's axis.
                const float sib = (cpu_hash_f(seed, RibbonProp::CHECKER_HUE_JITTER_ROLL) - 0.5f)
                                * 2.0f * CHECKER_HUE_SIBLING_JITTER;
                float hv = pr.hue_var + sib;
                hv = (hv < 0.0f) ? 0.0f : (hv > 1.0f) ? 1.0f : hv;
                sel.checker_hue_spread = hv * 3.14159265f;
            }
        }
    }

    sel.footprint_r = FOOTPRINT_RADIUS;
}

// ─── select_ribbon_for_patch ──────────────────────────────────
//
inline bool select_ribbon_for_patch(RibbonState& rs, MachineCtx* c,
    int32_t gx, int32_t gz, RibbonSelection& sel) {
    // Tip-overlap idempotency: reject if ANY active ribbon's
    // near or far tip falls within this trigger patch.
    for (uint32_t i = 0; i < MAX_RIBBON_INSTANCES; i++) {
        if (!rs.active[i].active) continue;
        if ((rs.active[i].near_tip_gx == gx && rs.active[i].near_tip_gz == gz) ||
            (rs.active[i].far_tip_gx == gx && rs.active[i].far_tip_gz == gz))
            return false;
    }
    auto gate = run_spawn_preamble(c, gx, gz,
        rs.active, MAX_RIBBON_INSTANCES,
        RibbonProp::SPAWN_ROLL, RibbonConfig::SPAWN_CHANCE,
        mood_mult_for(PopFamily::RIBBON),
        PopFamily::RIBBON);
    if (!gate.ok) return false;

    // Tier selection with theme bias
    float tier_weights[RIBBON_TIER_COUNT];
    for (uint32_t t = 0; t < RIBBON_TIER_COUNT; t++)
        tier_weights[t] = RIBBON_BASE_TIER_WEIGHTS[t];
    for (uint32_t t = 0; t < RIBBON_TIER_COUNT; t++)
        tier_weights[t] *= THEMES[gate.theme_idx].tier_wt_ribbon[t];
    uint32_t tier_idx = select_tier(gate.seed, RibbonProp::TIER,
        tier_weights, RIBBON_TIER_COUNT);

    sel.seed = gate.seed;
    sel.trigger_gx = gx;
    sel.trigger_gz = gz;
    sel.slot = gate.slot;
    sel.tier_idx = tier_idx;

    fill_ribbon_selection_geometry(gate.seed, tier_idx, sel);

    // THE MINIATURE (indoor_module): gate-spawned ribbons pre-scale
    // by RIBBON_INDOOR_SCALE ("incredibly diminished"), then the cap
    // law — belt and braces; the cap never bites at 0.15. The
    // sampler's floors (MIN_CUBE_SIZE / MIN_ADDED_HEIGHT / 0.1 amps)
    // still hold.
    if (MOOD_TABLE[c->mood_state_.active].indoor) {
        sel.cube_size    = std::max(MIN_CUBE_SIZE,    sel.cube_size    * RIBBON_INDOOR_SCALE);
        sel.height       = std::max(MIN_ADDED_HEIGHT, sel.height       * RIBBON_INDOOR_SCALE);
        sel.lateral_amp  = std::max(0.1f,             sel.lateral_amp  * RIBBON_INDOOR_SCALE);
        sel.vertical_amp = std::max(0.1f,             sel.vertical_amp * RIBBON_INDOOR_SCALE);
        // The cap law, ribbon-shaped: extent = clearance + vertical
        // wave + half a cube; all four dimensions ride one ratio.
        const float cap_h  = INDOOR_HEIGHT_CAP_FRACTION
                           * MOOD_TABLE[c->mood_state_.active].wall_height;
        const float extent = sel.height + sel.vertical_amp + 0.5f * sel.cube_size;
        if (extent > cap_h) {
            const float s = cap_h / extent;
            sel.cube_size   *= s; sel.height       *= s;
            sel.lateral_amp *= s; sel.vertical_amp *= s;
        }
    }

    {
        float patch_cx = (gx + 0.5f) * Dim::PATCH_EXTENT;
        float patch_cz = (gz + 0.5f) * Dim::PATCH_EXTENT;
        float away_angle = std::atan2(patch_cz - c->point_.z,
            patch_cx - c->point_.x);
        float hash_spread = cpu_hash_f(gate.seed, RibbonProp::ORIENTATION);
        sel.orientation = away_angle + (hash_spread * 2.0f - 1.0f) * ORIENTATION_SPREAD;
    }

    return true;
}

// ─── place_ribbon_from_selection ──────────────────────────────
//
inline bool place_ribbon_from_selection(MachineCtx* c,
    const RibbonSelection& sel, RibbonPlacement& plan) {
    auto pos = negotiate_position(c, sel.seed,
        sel.trigger_gx, sel.trigger_gz,
        RibbonProp::ANCHOR_X, RibbonProp::ANCHOR_Z,
        RibbonConfig::POSITION_JITTER,
        RibbonProp::ORIENTATION,
        /*grounded=*/true,   // the anchor ribbon's tips touch ground; it claims and is blocked
        sel.footprint_r,
        // FULL containment (INDOOR_TREATMENT): the MINIATURE extent —
        // scaled lateral_amp + the scaled cube span (S-4's scale ran
        // at selection, before this clamp: a small ribbon needs only
        // a small room). Outdoors the clamp never fires.
        /*containment_r*/ sel.lateral_amp + (float)sel.cube_count * sel.cube_size,
        PopFamily::RIBBON, sel.slot, sel.tier_idx);
    if (!pos.ok) return false;

    plan = RibbonPlacement{};
    plan.slot = sel.slot;
    plan.seed = sel.seed;
    plan.trigger_gx = sel.trigger_gx;
    plan.trigger_gz = sel.trigger_gz;
    plan.host_gx = pos.host_gx;
    plan.host_gz = pos.host_gz;
    plan.tier_idx = sel.tier_idx;
    plan.cx = pos.cx;
    plan.cz = pos.cz;

    plan.cube_count = sel.cube_count;
    plan.cube_size = sel.cube_size;
    plan.height = sel.height;
    plan.orientation = sel.orientation;
    plan.lateral_amp = sel.lateral_amp;
    plan.lateral_cycles = sel.lateral_cycles;
    plan.vertical_amp = sel.vertical_amp;
    plan.color_mode = sel.color_mode;
    std::memcpy(plan.color, sel.color, sizeof(plan.color));
    std::memcpy(plan.color_b, sel.color_b, sizeof(plan.color_b));
    plan.checker_scatter = sel.checker_scatter;
    plan.checker_hue_spread = sel.checker_hue_spread;

    return true;
}

// ─── commit_ribbon ───────────────────────────────────────────
//
// Single entry: the dispatch path. The second entry left with the
// anchor ribbon it existed for.
inline void commit_ribbon(RibbonState& rs, MachineCtx* c,
    const RibbonPlacement& plan,
    int32_t trigger_gx, int32_t trigger_gz, wgpu::Queue& queue)
{
    GPURibbonState r{};
    r.anchor[0] = plan.cx;
    r.anchor[1] = 0.0f;
    r.anchor[2] = plan.cz;
    r.time = c->time_state_.seconds;
    r.cube_count = plan.cube_count;
    r.cube_size = plan.cube_size;
    r.height = plan.height;
    r.orientation = plan.orientation;
    {
        const float P = RIBBON_TIERS[plan.tier_idx].propagation_speed;
        const float total_length = (float)plan.cube_count * plan.cube_size;
        const float k = 6.2831853f * P / std::max(total_length, 1e-6f);
        r.propagation_speed = P;
        r.lateral_freq  = plan.lateral_cycles * k;
        r.vertical_freq = r.lateral_freq;
    }
    r.lateral_amp = plan.lateral_amp;
    r.vertical_amp = plan.vertical_amp;
    r.color_mode = plan.color_mode;
    r.color[0] = plan.color[0];
    r.color[1] = plan.color[1];
    r.color[2] = plan.color[2];
    r.color_b[0] = plan.color_b[0];
    r.color_b[1] = plan.color_b[1];
    r.color_b[2] = plan.color_b[2];
    r.checker_scatter = plan.checker_scatter;
    r.hue_spread = plan.checker_hue_spread;
    r.seed = plan.seed;
    r.is_visible = 1u;
    r.is_roaming = 1u;   // head-roaming on (player-flown or wandering; parked when neither)

    // Store in CPU mirror (per-frame nearest-selection uploads to GPU)
    uint32_t s = plan.slot;
    rs.gpu[s] = r;

    auto& ar = rs.active[s];
    // Snapshot the spawn color as the idle base — the home the line-tint
    // coupling (gen-2, T2) mixes over: gpu.color = lerp(spawn, stim, mix)
    // in the conductor's flush; mix rests 0 ⇒ this color exactly.
    ar.spawn_color[0] = r.color[0];
    ar.spawn_color[1] = r.color[1];
    ar.spawn_color[2] = r.color[2];
    ar.spawn_lateral_amp  = r.lateral_amp;
    ar.spawn_vertical_amp = r.vertical_amp;
    ar.phase = r.time;    // seed = wall clock, so the 100 BPM anchor reproduces the old clock exactly
    ar.patch_gx = trigger_gx;
    ar.patch_gz = trigger_gz;
    ar.host_gx = plan.host_gx;
    ar.host_gz = plan.host_gz;
    ar.anchor_x = plan.cx;
    ar.anchor_z = plan.cz;

    ar.wander = cpu_hash_f(plan.seed, RibbonProp::WANDER_ROLL) < WANDER_CHANCE;
    {
        float cr = cpu_sample_gaussian(plan.seed, RibbonProp::WANDER_CRUISE,
                                       WANDER_CRUISE_BASE, WANDER_CRUISE_SIGMA);
        ar.wander_cruise = (cr < WANDER_CRUISE_MIN) ? WANDER_CRUISE_MIN
                         : (cr > WANDER_CRUISE_MAX) ? WANDER_CRUISE_MAX : cr;
    }
    ar.wander_rng = 1u + (uint32_t)(cpu_hash_f(plan.seed, RibbonProp::WANDER_RNG)
                                    * 16777215.0f);
    ar.wander_tx = plan.cx - WANDER_HATCH_LEG * std::cos(r.orientation);
    ar.wander_tz = plan.cz - WANDER_HATCH_LEG * std::sin(r.orientation);
    ar.wander_retarget = WANDER_RETARGET_MIN;
    ar.wander_yaw_state = 0.0f;
    if (ribbon_head_is(rs, s))
        ribbon_invalidate_head(rs);

    // Two-tip anchoring: anchor IS the near tip (t=0).
    // Body extends entirely in the orientation direction (away from pawn).
    float total_length = (float)plan.cube_count * plan.cube_size;
    float dir_x = std::cos(plan.orientation);
    float dir_z = std::sin(plan.orientation);

    const float far_x = plan.cx + dir_x * total_length;
    const float far_z = plan.cz + dir_z * total_length;
    ar.near_tip_gx = (int32_t)std::floor(plan.cx / Dim::PATCH_EXTENT);
    ar.near_tip_gz = (int32_t)std::floor(plan.cz / Dim::PATCH_EXTENT);
    ar.far_tip_gx = (int32_t)std::floor(far_x / Dim::PATCH_EXTENT);
    ar.far_tip_gz = (int32_t)std::floor(far_z / Dim::PATCH_EXTENT);

    ar.near_tip_registered = false;
    ar.far_tip_registered = false;
    ar.ref_count = 0;

    ar.active = true;
    rs.active_count++;
    // SEAM[ribbon:L1] unconditional stdout — exhibition guard
    //   candidate, still open. (The DIAG_RIBBON this once named was
    //   never defined anywhere; deleted PRUNING_1 P1 Step 4.)
    std::cout << "[Ribbon] SPAWN slot=" << s << " at (" << plan.cx << ", " << plan.cz
        << ") tier=" << plan.tier_idx
        << " len=" << total_length
        << " near=(" << ar.near_tip_gx << "," << ar.near_tip_gz
        << ") far=(" << ar.far_tip_gx << "," << ar.far_tip_gz << ")\n";
}

// ═══ DISPATCH FUNNELS (table-shaped; declared in entity_types.hpp) ═

inline bool dispatch_select_ribbon(MachineCtx* self,
    int32_t gx, int32_t gz, EntityQueueEntry& e) {
    return select_ribbon_for_patch(self->ribbon_state_, self, gx, gz, e.ribbon);
}

inline bool dispatch_place_ribbon(MachineCtx* self,
    EntityQueueEntry& e, PlacementEntry& pe) {
    pe.family = e.family; pe.gx = e.gx; pe.gz = e.gz;
    if (place_ribbon_from_selection(self, e.ribbon, pe.ribbon)) {
        return true;
    }
    else {
        self->ribbon_state_.active[e.ribbon.slot].active = false;
        return false;
    }
}

inline void dispatch_commit_ribbon(MachineCtx* self,
    PlacementEntry& pe, wgpu::Queue& queue) {
    // Commit the ribbon state (GPU mirror, active record, tip positions)
    commit_ribbon(self->ribbon_state_, self, pe.ribbon, pe.gx, pe.gz, queue);

    uint32_t slot = pe.ribbon.slot;
    auto& ar = self->ribbon_state_.active[slot];

    // Register with tip patches that currently exist.
    // Late registration handles the other tip when its patch is allocated.
    uint32_t refs = 0;
    auto* near_host = find_patch(self, ar.near_tip_gx, ar.near_tip_gz);
    if (near_host) {
        near_host->record_entity(PopFamily::RIBBON, slot);
        ar.near_tip_registered = true;
        refs++;
    }
    auto* far_host = find_patch(self, ar.far_tip_gx, ar.far_tip_gz);
    if (far_host && (ar.far_tip_gx != ar.near_tip_gx || ar.far_tip_gz != ar.near_tip_gz)) {
        far_host->record_entity(PopFamily::RIBBON, slot);
        ar.far_tip_registered = true;
        refs++;
    }

    if (refs == 0) {
        std::cout << "[Ribbon] REJECT slot=" << slot
            << " — no tip patches alive\n";
        // Corollary 3: a rejecting phase releases what earlier phases
        // reserved. place_ribbon_from_selection registered through
        // negotiate_position; nothing here freed it, and the host key it was
        // filed under belongs to a patch that is already gone.
        unregister_footprint_for(self, PopFamily::RIBBON, slot);
        ar = ActiveRibbon{};
        self->ribbon_state_.gpu[slot] = GPURibbonState{};
        self->ribbon_state_.active_count--;
        return;
    }
    ar.ref_count = refs;
}

// ═══ THE EVICTOR ══════════════════════════════════════════════════

inline void evict_ribbon(MachineCtx* self,
    uint32_t slot, wgpu::Queue& queue) {
    auto& ar = self->ribbon_state_.active[slot];
    if (!ar.active) return;

    // The RIBBON host: the flown ribbon is pinned for the flight's
    // duration. Its anchor patches stream out as the player flies away,
    // but the ribbon must persist — skip eviction entirely while it is
    // the mounted, rendered ribbon. release_sky_exit_ribbon (below)
    // frees it on dismount, ground included — and it must, precisely
    // because returning HERE skips the ref_count decrement below, so
    // the refcount protocol cannot finish a flown ribbon's death.
    // A rendered WANDERER is pinned the same way:
    // it drifts freely off its spawn patch, and with one slot the
    // world's ribbon persists — a contemplative object should.
    if (slot == self->ribbon_state_.rendered_slot
        && (self->point_.host == PointHost::RIBBON || ar.wander)) {
        return;
    }

    // Decrement ref count — one anchor patch has been evicted.
    // Only fully evict when all referencing patches are gone.
    if (ar.ref_count > 1) {
        ar.ref_count--;
        return;
    }

    // Final reference gone — full eviction. The release belongs HERE and not
    // at the top: the two early returns above are a live ribbon (the host /
    // wanderer pin, and the refcount still holding). Releasing there would
    // free the ground of a ribbon that is still standing on it.
    unregister_footprint_for(self, PopFamily::RIBBON, slot);
    ar = ActiveRibbon{};
    self->ribbon_state_.gpu[slot] = GPURibbonState{};
    self->ribbon_state_.active_count--;
    if (self->ribbon_state_.rendered_slot == slot) {
        GPURibbonState empty{};
        self->gpuState_.upload_ribbon(queue, empty);
        self->ribbon_state_.rendered_slot = UINT32_MAX;
        // Successor ribbons reuse this slot — force re-init.
        ribbon_invalidate_head(self->ribbon_state_);
    }
    std::cout << "[Ribbon] EVICT slot=" << slot << "\n";
}

// ─── Sky-exit release (owner verb) ─────────────────────────────────
// The RIBBON host was released — free the pinned (now anchor-less)
// ribbon so a fresh one can spawn.
//
// THE HAND THAT CLAIMS IS THE HAND THAT FREES. This is a ribbon DEATH,
// so it owes the ground back: place_ribbon_from_selection registered a
// footprint through negotiate_position (grounded — the anchor ribbon's
// tips touch ground), and nothing here freed it. A dismount is a
// mid-world keypress, not a transition, so no reset_surface sweep
// follows to cover the miss.
//
// NOT routed through evict_ribbon, and the reason is structural: the
// evictor's pin spares a rendered WANDERER, and every anchor-patch
// eviction during the flight returned AT that pin — before the
// ref_count decrement — so ref_count arrives here stale-high and the
// evictor would decrement it instead of releasing. This is the minimal
// owner-release instead: footprint, mirror, count, render slot.
// (Tip refs need nothing: they are patch-side and evict_ribbon does
// not touch them either.)
//
// It takes the MACHINE FACE because unregister_footprint_for does, and
// ribbon_frame_tick's RibbonDeps cannot reach it. The dismount EDGE
// lives in possess() (RESIDUE_3): the transaction stages
// sky.release_pending when the RIBBON host is released; this verb —
// the request's sole consumer — executes today's outcome unchanged.
inline void release_sky_exit_ribbon(MachineCtx* self, wgpu::Queue& queue) {
    auto& rs = self->ribbon_state_;
    if (rs.sky.release_pending) {
        rs.sky.release_pending = false;
        uint32_t s = rs.rendered_slot;
        if (s != UINT32_MAX && rs.active[s].active) {
            unregister_footprint_for(self, PopFamily::RIBBON, s);
            // Release-by-owner completes for RECORDS (REQUEST_1 rider):
            // this death can leave tip records on still-alive patches —
            // the ONE path that can (patch-driven eviction wipes its own
            // records; REJECT records nothing; release_finite follows
            // the registry wipe). A stale {RIBBON, slot} record would
            // later misdirect an eviction at the successor in this slot
            // — one death, one patch-eviction early. Scrub both tips;
            // a dead patch took its record with it. Safe here: this
            // verb runs at phase_ribbon_tick, outside any patch loop.
            auto& ar = rs.active[s];
            if (ar.near_tip_registered) {
                if (auto* p = find_patch(self, ar.near_tip_gx, ar.near_tip_gz))
                    p->unrecord_entity(PopFamily::RIBBON, s);
            }
            if (ar.far_tip_registered) {
                if (auto* p = find_patch(self, ar.far_tip_gx, ar.far_tip_gz))
                    p->unrecord_entity(PopFamily::RIBBON, s);
            }
            rs.active[s] = ActiveRibbon{};
            rs.gpu[s] = GPURibbonState{};
            if (rs.active_count > 0) rs.active_count--;
            GPURibbonState empty{};
            self->gpuState_.upload_ribbon(queue, empty);
            rs.rendered_slot = UINT32_MAX;
            // Successor ribbons reuse this slot — force re-init.
            ribbon_invalidate_head(rs);
        }
    }
}


// ─── Teardown (owner verb) ────────────────────────────────────────
inline void teardown_ribbon(RibbonState& rs, RibbonDeps* c, wgpu::Queue& queue) {
    // Ribbon — clear all slots
    {
        for (uint32_t i = 0; i < MAX_RIBBON_INSTANCES; i++) {
            rs.active[i] = ActiveRibbon{};
            rs.gpu[i] = GPURibbonState{};
        }
        rs.active_count = 0;
        rs.rendered_slot = UINT32_MAX;
        GPURibbonState empty{};
        c->gpuState_.upload_ribbon(queue, empty);
    }
}

// ─── Finite-mode release (owner verb) ─
// deactivate ribbons in finite mode. ORDER (O-3): must run AFTER
// apply_mood set mood_state_.active.
inline void release_finite_ribbons(RibbonState& rs, RibbonDeps* c, wgpu::Queue& queue) {
    if (c->world_state_.finite_mode && rs.active_count > 0) {
        for (uint32_t i = 0; i < MAX_RIBBON_INSTANCES; i++) {
            rs.active[i] = ActiveRibbon{};
            rs.gpu[i] = GPURibbonState{};
        }
        rs.active_count = 0;
        rs.rendered_slot = UINT32_MAX;
        GPURibbonState empty{};
        c->gpuState_.upload_ribbon(queue, empty);
    }
}


// ─── Tip registration (owner verb): called by the
// streaming conductor when a patch spawns — registers whichever of a
// ribbon's two anchor tips lives at (gx,gz) into the host patch and
// takes the reference. The inverse (the ref_count decrement) already
// lives owner-side in evict_ribbon.
inline void ribbon_register_tips_at(RibbonState& rs, ActivePatch& host, int32_t gx, int32_t gz) {
    for (uint32_t r = 0; r < MAX_RIBBON_INSTANCES; r++) {
        auto& ar = rs.active[r];
        if (!ar.active) continue;
        // Check near tip
        if (!ar.near_tip_registered &&
            ar.near_tip_gx == gx && ar.near_tip_gz == gz) {
            host.record_entity(PopFamily::RIBBON, r);
            ar.near_tip_registered = true;
            ar.ref_count++;
        }
        // Check far tip
        if (!ar.far_tip_registered &&
            ar.far_tip_gx == gx && ar.far_tip_gz == gz) {
            host.record_entity(PopFamily::RIBBON, r);
            ar.far_tip_registered = true;
            ar.ref_count++;
        }
    }
}

} // namespace the_board
} // namespace t7
