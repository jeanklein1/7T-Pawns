#pragma once
#include <cstdint>
#include "cartridges/the_board/realization/state.hpp"                    // Dim::*, GPURibbonState, wgpu
#include "cartridges/the_board/contracts/mood_constants.hpp"   // MOOD_COUNT (sizes the mood gate)
#include "cartridges/the_board/contracts/wgpu_fwd.hpp"   // wgpu handle fwds (lockstep insurance)
#include "cartridges/the_board/contracts/entity_types.hpp"     // RibbonSelection/RibbonPlacement (the boundary DTOs) + queue types
#include "cartridges/the_board/contracts/control_panel.hpp"    // FIELD_SLACK/K/FMAX + the two emitter mutes — the one home
#include "cartridges/the_board/contracts/ribbon_surface.hpp"   // ORGAN_3 w2 — RIBBON_LIVE: the head law + wander steering
#include "cartridges/the_board/contracts/driver_surface.hpp"   // ORGAN_4 P2 — DRIVER_LIVE.ribbon: the four pipes' rests + the seam's gain

// ─── ribbon.hpp (HEADER: console + vocabulary + state + decls) ───
//
// Sky Ribbon: the CPU half of one entity whose head, spine, body and
// frame live in the ribbon room on the GPU (world.wgsl §6.5).
//
// THE CPU AUTHORS INTENT AND READS NOTHING BACK (RIBBON_1). What this
// module says, once per frame, is a single 112-byte RibbonState: the
// spawn draw, the tier, the colors, the wave amplitudes the canvas
// drives, and — when the ribbon is a wanderer — one standing fact and
// one number, is_wander and the cruise it was drawn with. The head
// integrates them; the body is drawn where the head has been. There is
// no CPU head, no propagation history, and no readback: the GPU's
// answer never comes home, so nothing here can disagree with it.
//
// THE WANDERER STEERS ITSELF, IN THE ROOM WHERE IT FLIES (RIBBON_2).
// The wander brain used to live here and could not see the head it
// commanded, so it kept a private copy of the flight law to steer
// against. It is the head kernel's now: the waypoint is drawn from the
// ribbon's own seed, held in RibbonHeadState, and compared against the
// position the kernel just integrated. The disc it roams and the
// softness of its turn are config.ribbon_* dials, so the brain reads
// the same rests a rider's hands are eased by, and the Sky Rule bends
// a heading that was aimed at a target the brain can still see.
//
// The impl additionally reaches the spawn-engine services
// (run_spawn_preamble, negotiate_position — spawn_engine.hpp),
// seed_utils.hpp, cartridge core (time_state_.seconds/dt/beat_rate,
// THEMES / Dim::PATCH_EXTENT (file-scope vocabulary), the four ribbon
// canvas bindings ride RibbonDeps; the sky release is OWN state), and the
// GPU wires (upload_ribbon — the one per-frame write — and
// reset_ribbon_body, the one word that says "you are unseeded").
//
// SEAM[ribbon:complete-subsystem] complete bespoke pipeline in one
//   module — vocabulary + state + machinery + lifecycle. Same family
//   as gol_zones (Ch. 12.B) and gallery (Ch. 12.E), and the reference
//   instance of the entity-module pattern: tuning console → registry →
//   tiers → runtime state → author seats → lifecycle. (surface = the
//   bank rows; the in-module section retired with its last legacy
//   consumer; the head laws graduated to the GPU at RIBBON_1.)
// ─────────────────────────────────────────────────────────────────

#include <algorithm>   // std::max, std::min   // (impl, merged)
#include <cfloat>      // FLT_MAX (nearest-slot adoption)   // (impl, merged)
#include <cmath>       // std::sin, std::cos, std::atan2, std::exp, std::floor, std::remainder   // (impl, merged)
#include <cstdint>   // (impl, merged)
#include <cstring>     // std::memcpy (placement color copy)   // (impl, merged)
#include <iostream>    // the spawn log   // (impl, merged)
#include "core/instruments.hpp"   // RIBBON_4 — INSTRUMENTS.stream_witness gates the steady path's witness lines

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
// RibbonConfig::SPAWN_CHANCE / POSITION_JITTER graduated to
// contracts/ribbon_surface.hpp (ORGAN_4 P3d) as the first two fields of
// RIBBON_SPAWN_LIVE — the DESTRUCTIVE bank, read once as a ribbon is
// drawn and never again. The struct dies with them: two static members
// were its whole body.
//
// SEAM[ribbon:P4] hygiene rows pattern — the ribbon column of
//   MOOD_SPAWN_MULT (surface/population_themes.hpp) is
//   { 1, 1, 1, 1 }, all identity: the mood term suppresses no
//   row today. (It previously read as carrying indoor zeros; the
//   live table never had them.) Same family as gol_zones:P4 and
//   the cube populations' hygiene rows.

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
// min(RIBBON_LIVE.yaw_rate, speed / RIBBON_LIVE.r_min), so the heading can only
// change while moving and the flown path can never be tighter than
// the minimum turn radius. Consumed by the HEAD KERNEL (world.wgsl
// ribbon_head) since RIBBON_1 — for the rider's hands and, since
// RIBBON_2, for the wanderer's alike. All control-panel material — and since ORGAN_3 w2 it IS a
// control panel: the values graduated to contracts/ribbon_surface.hpp,
// where RIBBON_TABLE is the design and RIBBON_LIVE is what this module
// reads. The boot pins the same rests into config.ribbon_* — the
// kernels' only road to them (state.hpp).

// ── The frame law is the BODY KERNEL'S (RIBBON_1) ──────────────────
// Three hand-kept constants used to sit here, LOCKSTEP MIRRORS of the
// shader's, because the saddle was computed in this room and the ring
// frame in that one. Both are the body kernel's now — the rider sits on
// the ring-0 motor the tube is drawn with — so there is no second half
// to keep and the mirror is gone, not moved. The tangent-align dial went
// with them: the frame IS the drawn tangent, which is what align = 1 was
// asking for all along.

// ── Wander policy ─────────────────────────────────────────────────
// The steering channel's IDLE SCRIPT — the shape of autonomous drift.
// Constants: control-panel material. The four PER-FRAME steering dials
// (steer_soft, yaw_max, yaw_tau, arrive_radius) graduated to
// contracts/ribbon_surface.hpp at ORGAN_3 w2; the per-SPAWN rolls below
// stay authored until w3 gives them a destructive-temperament bank.
// The per-SPAWN rolls graduated to contracts/ribbon_surface.hpp
// (ORGAN_4 P3d) as RIBBON_SPAWN_LIVE's wander_* fields — chance, cruise
// base/sigma/min/max, leg min/max, spread, retarget min/var, hatch leg.
// w3's sentence above said they were waiting on "a destructive-temperament
// bank"; that bank exists now, and it is a second bank beside RIBBON_LIVE
// precisely because its temperament differs from the head law's.

// ═══ COLOR VOCABULARY ════════════════════════════════════════════

struct RibbonColorMode {
    static constexpr uint32_t SMOOTH = 0;  // terrain-derived monochrome
    static constexpr uint32_t TINTED = 1;  // warm/cool hue shift
    static constexpr uint32_t CONTRAST = 2;  // cell skin: per-cell coloring — pair-contrast species (two medians, parity) or median-field species (one median, terrain-patch texture); see fill
    static constexpr uint32_t COUNT = 3;
    //   The three selection probabilities (SMOOTH/TINTED/CONTRAST, sum
    //   1.0) graduated to RIBBON_SPAWN_LIVE.color_weights (ORGAN_4 P3d);
    //   the three IDS stay — a vocabulary is not a range.
};
static_assert(RibbonColorMode::COUNT == RIBBON_COLOR_MODE_COUNT,
    "the bank's color_weights row is sized by the vocabulary above; a "
    "fourth mode needs its weight in contracts/ribbon_surface.hpp too");

// The SMOOTH palette and the whole colour character block graduated to
// contracts/ribbon_surface.hpp (ORGAN_4 P3d) — RIBBON_SPAWN_LIVE's
// smooth_palette, smooth_var_range/bias/g_scale/b_scale, tinted_range and
// tinted_base. RIBBON_SMOOTH_PALETTE_COUNT went with them because it SIZES
// the bank's palette row; the draw below still reads it unqualified.

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
    static constexpr uint32_t WANDER_CRUISE = 451u;     // gaussian draw: cruise fraction of config.ribbon_max_speed
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
    // THE WANDERER'S ONE WORD (RIBBON_2). The brain came home to the head
    // kernel — target, bearing, cap and cruise all live there now — so what
    // the CPU still says about a wanderer is the throttle it was drawn with.
    // The waypoint, its retarget clock, its xorshift and the eased steering
    // output left with the brain; so did the private copy of the flight law
    // it steered against, which stood in for a head the CPU could not read.
    float    wander_cruise = 0.0f;      // throttle fraction of config.ribbon_max_speed
};

// ── Ribbon module state ──────────────────────────────────────────
struct RibbonState {
    ActiveRibbon   active[MAX_RIBBON_INSTANCES]{};
    uint32_t       active_count = 0;

    GPURibbonState gpu[MAX_RIBBON_INSTANCES]{};
    uint32_t       rendered_slot = UINT32_MAX;

    // ── Sky-flight fixture: the rider state, re-homed from PlayerState —
    //    the mount was always ribbon-owned. ROUTING left this record
    //    (RESIDUE_3): the host machine (point_.host == PointHost::RIBBON)
    //    routes all consumption. RIBBON_1 left ONE field: the eased yaw
    //    went to the head kernel (RIBBON_HANDS_TAU), where the hand it
    //    eases is read. What stays is the one-shot release request
    //    possess() stages for the owning verb. ──
    struct SkyFlight {
        bool release_pending = false; // possess() staged a RIBBON release; ribbon_on_dismount consumes it
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
// Frame conductor — the module's ONE per-frame verb (RIBBON_1)
void ribbon_frame_tick(RibbonState& rs, RibbonDeps* c, wgpu::Queue& queue);

// THE DRAW IS THE LIVE COUNT (RIBBON_1). The ribbon draws the rings it has,
// not the rings it could have: at the tiers' means that is ~1 700 vertices,
// where the 400-ring ceiling was 9 588 and the VS early-out retired four
// fifths of them — twice a pass, main and shadow. 0 when nothing is
// rendered, which is why BUNDLE_1 could retire dt_ribbon's liveness guard
// into the DR_RIBBON record: the two could never have disagreed, and a
// number survives being recorded into a bundle where a skip does not.
inline uint32_t ribbon_draw_verts(const RibbonState& rs) {
    if (rs.rendered_slot == UINT32_MAX) return 0u;
    const uint32_t n = std::min(rs.gpu[rs.rendered_slot].cube_count,
                                Dim::RIBBON_MAX_RINGS);
    if (n < 2u) return 0u;
    return Dim::ribbon_vertex_count_for(n);
}
void teardown_ribbon(RibbonState& rs, RibbonDeps* c, wgpu::Queue& queue);
void release_finite_ribbons(RibbonState& rs, RibbonDeps* c, wgpu::Queue& queue);
// The dismount — machine-faced; RIBBON_1 made it a handover, not a death.
void ribbon_on_dismount(MachineCtx* self, wgpu::Queue& queue);
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
// The steering integrator lives in the head kernel, and so, since
// RIBBON_2, does every seat at it: the PLAYER (the rider's hands, through
// signal.move_x/move_z), the WANDERER (a target on the anchor's disc,
// drawn by hash and steered toward through the hands' own cap), and one
// EMPTY SEAT reserved for the musical canvas. One control law, many
// authors, and none of them on this side of the wire — what leaves this
// room is the standing fact of WHO authors, and the wanderer's cruise.

// ═══ FRAME ORCHESTRATION ═════════════════════════════════════════
//
inline void ribbon_frame_tick(RibbonState& rs, RibbonDeps* c, wgpu::Queue& queue) {

    // Phase clock on all CPU mirrors — MUSICAL TIME: the
    // sway integrates at the tempo follower's rate, scaled
    // so 100 BPM reproduces wall seconds exactly (the
    // calibration anchor). Held-last: transport stops, the
    // world keeps the pulse.
    const float phase_rate = c->time_state_.beat_rate
                           * (60.0f / RIBBON_LIVE.reference_bpm);
    for (uint32_t i = 0; i < MAX_RIBBON_INSTANCES; i++) {
        auto& par = rs.active[i];
        if (!par.active) continue;
        par.phase += phase_rate * c->time_state_.dt;
        rs.gpu[i].time = par.phase;
        {
            const VisualParams& vp = c->visual_canvas_.params();
            // ORGAN_4 P2 — THE SEAM, the 2a recipe verbatim:
            // out = rest + gain·(driven − rest). The rests below ARE the
            // fallbacks this block hardcoded before the room existed, so
            // at the shipped seeds (gain 1, rests 1/1/{0,0,0}/0) the
            // arithmetic is byte-stable: rest + 1·(d − rest) folds back
            // to d for every float, and with the pipe unbound the driven
            // value IS the rest and the blend is the rest exactly.
            const auto& R = DRIVER_LIVE.ribbon;
            const float ml_raw = c->ribbon_amp_lat_dst_.valid
                ? vp.get(c->ribbon_amp_lat_dst_.base)  : R.rest_amp_lat;
            const float ml = R.rest_amp_lat + R.gain * (ml_raw - R.rest_amp_lat);
            const float mv_raw = c->ribbon_amp_vert_dst_.valid
                ? vp.get(c->ribbon_amp_vert_dst_.base) : R.rest_amp_vert;
            const float mv = R.rest_amp_vert + R.gain * (mv_raw - R.rest_amp_vert);
            rs.gpu[i].lateral_amp  = rs.active[i].spawn_lateral_amp  * ml;
            rs.gpu[i].vertical_amp = rs.active[i].spawn_vertical_amp * mv;

            // Line tint (color gen-2): gpu.color = lerp(spawn, stim, mix).
            // Rest = mix 0 = the seed-drawn color exactly; the tick's one
            // upload_ribbon ships it unchanged.
            const float mix_raw = c->ribbon_tint_mix_dst_.valid
                ? vp.get(c->ribbon_tint_mix_dst_.base) : R.rest_tint_mix;
            const float mix = R.rest_tint_mix + R.gain * (mix_raw - R.rest_tint_mix);
            const float* st = c->ribbon_tint_stim_dst_.valid
                ? vp.run(c->ribbon_tint_stim_dst_.base) : nullptr;
            for (int c2 = 0; c2 < 3; ++c2) {
                // THE NULL BRANCH'S REST SHAPE, READ RATHER THAN GUESSED:
                // downstream was `st ? st[c2] : 0.0f`, so the shape the
                // code already assumed is {0,0,0} — PARAM_LAYOUT's rest
                // column for ribbon.color_stim, verbatim.
                const float s_raw = st ? st[c2] : R.rest_tint_stim[c2];
                const float s = R.rest_tint_stim[c2]
                              + R.gain * (s_raw - R.rest_tint_stim[c2]);
                rs.gpu[i].color[c2] =
                    par.spawn_color[c2]
                    + (s - par.spawn_color[c2]) * mix;
            }
        }
    }

    // (The dismount ran at the head of this phase — it hands the ribbon back
    // to the wander brain and needs the machine face this tick does not
    // carry: ribbon_on_dismount.)

    // Render one ribbon: hold the current slot until it's evicted,
    // then pick the nearest active ribbon as the new rendered slot.
    bool current_alive = rs.rendered_slot != UINT32_MAX
        && rs.active[rs.rendered_slot].active;

    if (!current_alive) {
        // Current slot is gone — find nearest active ribbon.
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
            rs.rendered_slot = nearest;
            current_alive = true;
            // A slot change is a new body: the head kernel re-seeds and the
            // body kernel drops the old ribbon's bulges on its first tick.
            c->gpuState_.reset_ribbon_body(queue);
        } else if (rs.rendered_slot != UINT32_MAX) {
            GPURibbonState empty{};
            c->gpuState_.upload_ribbon(queue, empty);
            rs.rendered_slot = UINT32_MAX;
        }
    }

    if (current_alive) {
        auto& g  = rs.gpu[rs.rendered_slot];
        auto& ar = rs.active[rs.rendered_slot];
        // THE WANDERER'S ONE WORD, into the state the kernel reads. Neither
        // the rider's hands nor the brain's steering come through here: the
        // head kernel reads signal.move_x/move_z itself, gated on
        // config.point_host, and draws its own target when nobody rides. A
        // wanderer under a rider is still marked is_wander — the flag says
        // WHO AUTHORS WHEN NOBODY RIDES, and the ride outranks it.
        g.is_wander       = ar.wander ? 1u : 0u;
        g.wander_throttle = ar.wander ? ar.wander_cruise : 0.0f;
        // THE ONE WRITE (RIBBON_1). The whole 112-byte state, three windows,
        // once a frame. It carries the phase clock, the canvas-driven wave
        // amplitudes and the line tint the flush loop above computed, and the
        // brain's two numbers — everything the CPU has to say.
        c->gpuState_.upload_ribbon(queue, g);
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
    // from seed; the ground joins once, on the head kernel's seed tick.
    sel.height = std::max(MIN_ADDED_HEIGHT,
        cpu_sample_gaussian(seed, RibbonProp::HEIGHT, tp.height_mean, tp.height_sigma));

    sel.orientation = cpu_hash_f(seed, RibbonProp::ORIENTATION) * 6.2831853f;

    sel.lateral_amp = std::max(0.1f, cpu_sample_gaussian(seed, RibbonProp::LATERAL_AMP, tp.lateral_amp_mean, tp.lateral_amp_sigma));
    sel.lateral_cycles = std::max(0.1f, cpu_sample_gaussian(seed, RibbonProp::LATERAL_CYCLES, tp.lateral_cycles_mean, tp.lateral_cycles_sigma));

    sel.vertical_amp = std::max(0.1f, cpu_sample_gaussian(seed, RibbonProp::VERTICAL_AMP, tp.vertical_amp_mean, tp.vertical_amp_sigma));

    // Color
    sel.color_mode = select_tier(seed, RibbonProp::COLOR_ROLL,
        RIBBON_SPAWN_LIVE.color_weights, RibbonColorMode::COUNT);

    if (sel.color_mode == RibbonColorMode::SMOOTH) {
        uint32_t pal_idx = (uint32_t)(cpu_hash_f(seed, RibbonProp::PALETTE_IDX) * RIBBON_SMOOTH_PALETTE_COUNT);
        if (pal_idx >= RIBBON_SMOOTH_PALETTE_COUNT) pal_idx = RIBBON_SMOOTH_PALETTE_COUNT - 1;
        const auto& S = RIBBON_SPAWN_LIVE;
        float var = cpu_hash_f(seed, RibbonProp::COLOR_R) * S.smooth_var_range + S.smooth_var_bias;
        sel.color[0] = S.smooth_palette[pal_idx][0] + var;
        sel.color[1] = S.smooth_palette[pal_idx][1] + var * S.smooth_var_g_scale;
        sel.color[2] = S.smooth_palette[pal_idx][2] + var * S.smooth_var_b_scale;
    }
    else if (sel.color_mode == RibbonColorMode::TINTED) {
        const auto& S = RIBBON_SPAWN_LIVE;
        sel.color[0] = cpu_hash_f(seed, RibbonProp::COLOR_R) * S.tinted_range[0] + S.tinted_base[0];
        sel.color[1] = cpu_hash_f(seed, RibbonProp::COLOR_G) * S.tinted_range[1] + S.tinted_base[1];
        sel.color[2] = cpu_hash_f(seed, RibbonProp::COLOR_B) * S.tinted_range[2] + S.tinted_base[2];
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
        RibbonProp::SPAWN_ROLL, RIBBON_SPAWN_LIVE.spawn_chance,
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
    // by INDOOR_LIVE.ribbon_scale ("incredibly diminished"), then the cap
    // law — belt and braces; the cap never bites at 0.15. The
    // sampler's floors (MIN_CUBE_SIZE / MIN_ADDED_HEIGHT / 0.1 amps)
    // still hold.
    if (mood_def(c->mood_state_.active).shape.indoor) {
        sel.cube_size    = std::max(MIN_CUBE_SIZE,    sel.cube_size    * INDOOR_LIVE.ribbon_scale);
        sel.height       = std::max(MIN_ADDED_HEIGHT, sel.height       * INDOOR_LIVE.ribbon_scale);
        sel.lateral_amp  = std::max(0.1f,             sel.lateral_amp  * INDOOR_LIVE.ribbon_scale);
        sel.vertical_amp = std::max(0.1f,             sel.vertical_amp * INDOOR_LIVE.ribbon_scale);
        // The cap law, ribbon-shaped: extent = clearance + vertical
        // wave + half a cube; all four dimensions ride one ratio.
        const float cap_h  = INDOOR_LIVE.height_cap_fraction
                           * mood_def(c->mood_state_.active).shape.wall_height;
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
        RIBBON_SPAWN_LIVE.position_jitter,
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
    const bool ar_wander_roll =
        cpu_hash_f(plan.seed, RibbonProp::WANDER_ROLL) < RIBBON_SPAWN_LIVE.wander_chance;

    GPURibbonState r{};
    r.anchor[0] = plan.cx;
    r.anchor[1] = 0.0f;
    r.anchor[2] = plan.cz;
    // PLUMB_0 B2, CORRECTED BY THE CLOSING REFUTER. B-G1 called this a BIRTH
    // STAMP the shader differences against signal.t_seconds. It is neither.
    // It is the SEED of ActiveRibbon::phase, which ribbon_frame_tick
    // accumulates (`par.phase += phase_rate * dt`) and re-uploads into
    // rs.gpu[i].time EVERY FRAME — so the GPU field is a live phase, not a
    // birth time, and nothing differences it against the frame clock.
    //
    // TAKING IT FROM gpu_seconds() IS STILL RIGHT, for a smaller reason than
    // the one first given: the seed only has to reproduce the 100 BPM anchor,
    // and a young number seeds a float accumulator better than an old one.
    // There is no cross-world promise here to break — teardown_ribbon clears
    // the table in the same arm that re-stamps the epoch.
    r.time = c->time_state_.gpu_seconds();
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
    // Who authors the head when nobody rides: the brain, or nobody. The
    // per-frame numbers ride ribbon_frame_tick; this is the standing fact.
    r.is_wander = ar_wander_roll ? 1u : 0u;

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

    ar.wander = ar_wander_roll;
    {
        float cr = cpu_sample_gaussian(plan.seed, RibbonProp::WANDER_CRUISE,
                                       RIBBON_SPAWN_LIVE.wander_cruise_base,
                                       RIBBON_SPAWN_LIVE.wander_cruise_sigma);
        ar.wander_cruise = (cr < RIBBON_SPAWN_LIVE.wander_cruise_min) ? RIBBON_SPAWN_LIVE.wander_cruise_min
                         : (cr > RIBBON_SPAWN_LIVE.wander_cruise_max) ? RIBBON_SPAWN_LIVE.wander_cruise_max : cr;
    }
    // THE CPU'S ONE WORD TO THE BODY: you are unseeded. The head kernel
    // lays the spawn arc from this state on its next tick and seeds itself;
    // the body kernel drops the previous ribbon's bulges on the tick after.
    c->gpuState_.reset_ribbon_body(queue);

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
    // SEAM[ribbon:L1] CLOSED at RIBBON_4: this fires whenever a patch
    //   spawns, which under a rider is several times a second, and a
    //   blocking console write inside those frames is one of the blocks
    //   the steady world exists to remove. It rides the dial now.
    if constexpr (t7::INSTRUMENTS.stream_witness) {
        std::cout << "[Ribbon] SPAWN slot=" << s << " at (" << plan.cx << ", " << plan.cz
            << ") tier=" << plan.tier_idx
            << " len=" << total_length
            << " near=(" << ar.near_tip_gx << "," << ar.near_tip_gz
            << ") far=(" << ar.far_tip_gx << "," << ar.far_tip_gz << ")\n";
    }
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
        if constexpr (t7::INSTRUMENTS.stream_witness) {
            std::cout << "[Ribbon] REJECT slot=" << slot
                << " — no tip patches alive\n";
        }
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
    // the mounted, rendered ribbon. A rendered WANDERER is pinned the same
    // way: it drifts freely off its spawn patch, and with one slot the
    // world's ribbon persists — a contemplative object should.
    // RIBBON_1: the two pins are now ONE object's whole life. A dismount
    // sets ar.wander (ribbon_on_dismount), so the ribbon crosses from the
    // first pin to the second without ever passing through the ref_count
    // decrement below — which is exactly why the old dismount had to free
    // it by hand, and exactly why this one does not.
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
        // Successor ribbons reuse this slot — force re-seed.
        self->gpuState_.reset_ribbon_body(queue);
    }
    if constexpr (t7::INSTRUMENTS.stream_witness) {
        std::cout << "[Ribbon] EVICT slot=" << slot << "\n";
    }
}

// ─── The dismount (owner verb) ────────────────────────────────────
// THE RIBBON FLIES ON (RIBBON_1's ruling). The rider steps off; the ribbon
// does not die with the ride. It is handed to the WANDER BRAIN — which is
// the same seat the rider just left, filled by the idle script instead of a
// pair of hands — so a flight the player abandoned becomes a flight the
// world continues, and the ribbon the player rode is still there to be
// looked at, and to be ridden again.
//
// WHY THIS IS NOT A DEATH ANY MORE. The verb it replaces freed the ribbon:
// footprint, mirror, count, render slot. That existed because a dismounted
// ribbon was an anchor-less object nothing would evict — the flown pin in
// evict_ribbon spares the rendered slot while the RIBBON hosts, and that
// pin also spares a rendered WANDERER. Making the dismount a handover
// lands the object under the pin that already exists, so nothing leaks and
// nothing has to be freed: the ground it registered is still the ground it
// stands on.
//
// It keeps the MACHINE FACE it needed for unregister_footprint_for, because
// the brain's re-seed reads the anchor from the machine's own record and
// because a future ruling that DOES free here should not have to re-plumb.
// The dismount EDGE lives in possess(): the transaction stages
// sky.release_pending when the RIBBON host is released; this verb is the
// request's sole consumer.
inline void ribbon_on_dismount(MachineCtx* self, wgpu::Queue& queue) {
    (void)queue;
    auto& rs = self->ribbon_state_;
    if (!rs.sky.release_pending) return;
    rs.sky.release_pending = false;
    const uint32_t s = rs.rendered_slot;
    if (s == UINT32_MAX || !rs.active[s].active) return;

    auto& ar = rs.active[s];
    // The brain takes the seat. Its cruise is drawn the way commit_ribbon
    // draws it — the same seed, the same clamp, so a ribbon dismounted twice
    // cruises the same both times. Its TARGET is the head kernel's own:
    // wander_seq is already past 0 for a ribbon that ever wandered, and a
    // ribbon ridden from birth draws its first target on the tick the flag
    // comes back — from the head's true position, which the CPU never knew.
    ar.wander = true;
    {
        float cr = cpu_sample_gaussian(rs.gpu[s].seed, RibbonProp::WANDER_CRUISE,
                                       RIBBON_SPAWN_LIVE.wander_cruise_base,
                                       RIBBON_SPAWN_LIVE.wander_cruise_sigma);
        ar.wander_cruise = (cr < RIBBON_SPAWN_LIVE.wander_cruise_min) ? RIBBON_SPAWN_LIVE.wander_cruise_min
                         : (cr > RIBBON_SPAWN_LIVE.wander_cruise_max) ? RIBBON_SPAWN_LIVE.wander_cruise_max : cr;
    }
    std::cout << "[Ribbon] DISMOUNT slot=" << s << " -> wander\n";
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
