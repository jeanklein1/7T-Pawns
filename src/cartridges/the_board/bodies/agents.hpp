#pragma once
#include <cstdint>
#include <array>
#include "cartridges/the_board/realization/state.hpp"                    // Dim::MAX_AGENTS, GPUAgentState, GPU_AGENT_*_COUNT, wgpu
#include "cartridges/the_board/bodies/pawn_figures.hpp"        // PAWN_FIGURES, FIGURE_SHARES, family spans (H1) — this TU names them directly
#include "cartridges/the_board/contracts/mood_constants.hpp"   // MOOD_COUNT + the Mood IDs
#include "cartridges/the_board/contracts/agent_tiers.hpp"      // Tier vocabulary graduated to contracts/agent_tiers.hpp (ORGAN_2b) — the bank TIER_LIVE is the world's definition; the translator below reads it.
#include "cartridges/the_board/contracts/wgpu_fwd.hpp"   // wgpu handle fwds (lockstep insurance)
#include "cartridges/the_board/contracts/control_panel.hpp"   // ORGAN_4 P3b — PANEL_LIVE.possession.radius: the reach, graduated out of this file's console

// ─── agents.hpp (HEADER: registries + console + state + decls) ───
//
// Unified entity registry: the control panel for the agent system.
//
// The impl additionally reads COLUMN_PALETTE (grounded.hpp).
// ─────────────────────────────────────────────────────────────────

#include <cmath>      // std::sqrt, std::cos, std::sin   // (impl, merged)
#include <algorithm>  // std::min   // (impl, merged)
#include <iostream>   // census + event logs   // (impl, merged)
#include <iomanip>    // std::fixed, std::setprecision   // (impl, merged)
#include "core/instruments.hpp"   // RIBBON_4 — INSTRUMENTS.stream_witness gates the steady path's witness lines

namespace t7 {
namespace the_board {

// ═══ MODULE DEPS ════════════════════════════════════════════════════
// The agent population's requirements face. player_ is NON-const —
// possession re-anchors (agents door, v3 §9 Act III); the rest is
// read-only. (fwds: spine_state / patch_system types follow in the
// cohort.)
struct PlayerState; struct WorldState; struct TimeState;
enum class TransitionPhase;
class GPUState;
struct AgentsDeps {
    GPUState&              gpuState_;
    PlayerState&           player_;         // non-const: possession door
    const PointState&      point_;          // the point's house (position mirror — respawn ring, possession search)
    const TransitionPhase& transitionPhase_;
    const WorldState&      world_state_;
    const TimeState&       time_state_;
};

// ═══ BEHAVIOR IDS ════════════════════════════════════════════════
//
// Stable indices into AGENT_BEHAVIORS. The compute kernel's behavior
// switch dispatches on these values. Names below are also exported as
// AGENT_BEHAVIOR_NAMES[] for diagnostics — keep the two in lockstep.

// AgentBehaviorId graduated to contracts/agent_tiers.hpp with the
// table its rows name (ORGAN_3 w3).

// ═══ TIER IDS ════════════════════════════════════════════════════
// AgentTierId (AGENT_TIER_COUNT included) graduated to
// contracts/agent_tiers.hpp with the table whose rows name it
// (ORGAN_2b). The cross-check below stays: it stands on a
// realization constant, and a contract may not include one.

// Cross-check: GPU-side count constants in state.hpp must match the
// authoritative enums above. If you add a behavior or a tier, bump
// the GPU constant in state.hpp at the same time. The compiler will
// catch any drift here.
static_assert(AGENT_BEHAVIOR_COUNT == GPU_AGENT_BEHAVIOR_COUNT,
    "AGENT_BEHAVIOR_COUNT must match GPU_AGENT_BEHAVIOR_COUNT in state.hpp");
static_assert(AGENT_TIER_COUNT == GPU_AGENT_TIER_COUNT,
    "AGENT_TIER_COUNT must match GPU_AGENT_TIER_COUNT in state.hpp");

// Display names for diagnostics (census output, error messages).
// Order MUST match the enums above — index by the enum value.
// Mirrors the ORB_PAL_NAMES / ORB_TIERSET_NAMES pattern from orbs.hpp.
inline constexpr const char* AGENT_BEHAVIOR_NAMES[AGENT_BEHAVIOR_COUNT] = {
    "player",        //  0  PLAYER_CONTROLLED
    "random_walk",   //  1  RANDOM_WALK
    "biased_walk",   //  2  BIASED_WALK
    "wanderer",      //  3  WANDERER
    "home_seeker",   //  4  HOME_SEEKER
    "slow_patrol",   //  5  SLOW_PATROL
    "pursuit",       //  6  PURSUIT
    "flee",          //  7  FLEE
    "flock2d",       //  8  FLOCK2D
    "levy_flight",   //  9  LEVY_FLIGHT
    "passer",        // 10  PASSER
};

inline constexpr const char* AGENT_TIER_NAMES[AGENT_TIER_COUNT] = {
    "worker",        //  0
    "scout",         //  1
    "sentinel",      //  2
    "leader",        //  3
};

// ═══ TUNING CONSOLE ══════════════════════════════════════════════

inline constexpr uint32_t PLAYER_SLOT = 0;

// POSSESSION_RADIUS graduated to contracts/control_panel.hpp (ORGAN_4
// P3b) — PANEL_LIVE.possession.radius is what this module reads, and
// POSSESSION_RADIUS_SQ is retired outright. It was a second constant
// derived at declaration, so a dialled radius and a frozen square would
// have disagreed silently; the one read site squares the LIVE value now,
// which is why the pair could stop being a pair.

//
// SEAM[agents:L2] hardware mirror — AGENT_EVICTION_RADIUS must agree
//   with world.wgsl's identically-named const. The compiler cannot
//   catch drift; the prose below is the contract.
// THE VEIL CHAIN (ruled, V1): grounded existence = Dim::EXIST_RADIUS
//   (350, the pregen edge) — was 360, overshooting patch residency.
//
inline constexpr float AGENT_EVICTION_RADIUS    = 350.0f;
inline constexpr float AGENT_EVICTION_RADIUS_SQ = AGENT_EVICTION_RADIUS * AGENT_EVICTION_RADIUS;
static_assert(AGENT_EVICTION_RADIUS == Dim::EXIST_RADIUS,
    "VEIL CHAIN: grounded existence eviction sits ON the EXIST ring");

// AGENT_CENSUS_INTERVAL — wall-clock period (seconds). The periodic
// agent census died (BATCH C); the surviving consumer is the ROSTER
// gol-residue proof cadence (phase_census_dumps). The on-demand agent
// census remains at "boot" and "mood-transition".
inline constexpr float AGENT_CENSUS_INTERVAL = 30.0f;

// ═══ REGISTRY: BEHAVIORS ═════════════════════════════════════════

// AgentBehaviorDef, AGENT_BEHAVIORS and its row-count assert
// graduated to contracts/agent_tiers.hpp (ORGAN_3 w3), where
// BEHAVIOR_LIVE stands beside them as the live surface. They ride
// with the tier bank because they ride the same author: the
// translator below reads both, and one boundary re-speaks both.

// ═══ REGISTRY: TIER GAINS ════════════════════════════════════════
// AgentTierDef, AGENT_TIER_GAINS and its row-count assert graduated
// to contracts/agent_tiers.hpp (ORGAN_2b), where TIER_LIVE stands
// beside them as the world's definition. The table keeps its two
// jobs — seeding the bank and standing under the asserts — and the
// translator below reads the bank, not the table.

// ═══ REGISTRY: POPULATIONS ═══════════════════════════════════════

struct AgentPopulationDef {
    uint32_t mood_id;
    uint32_t count;                                          // 0..Dim::MAX_AGENTS-1
    std::array<float, AGENT_BEHAVIOR_COUNT> behavior_weights;
    std::array<float, AGENT_TIER_COUNT>     tier_weights;
    float    spawn_inner_radius;                             // world units (annulus inner)
    float    spawn_radius;                                   // world units (annulus outer)
    float    spawn_center_forward;                           // ATRIUM_9 — world units the annulus'
                                                             // CENTRE rides along the arrival gaze
    float    home_seeding_radius;                            // world units from spawn point
};

// ─── Why no constexpr helper builders ───────────────────────────

//
inline constexpr AgentPopulationDef AGENT_POPULATIONS[MOOD_COUNT] = {
    /* MOOD_OPEN_SUNSET — Scout-heavy travelers (BiasedWalk) */
    { /*mood_id=*/ MOOD_OPEN_SUNSET, /*count=*/ 17,
      //                       player rwalk  bwalk wandr hseek slowp pursu  flee flock  levy passr
      /*behavior_weights=*/ {    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
      //                     worker scout sentl leadr
      /*tier_weights=*/     {  1.0f, 3.0f, 0.0f, 0.0f },
      /*spawn_inner_radius=*/ 200.0f,
      /*spawn_radius=*/       340.0f,
      /*spawn_center_forward=*/ 0.0f,
      /*home_seeding_radius=*/ 8.0f },
    /* MOOD_INDOOR_FLAT — gallery walkers (SlowPatrol) */
    { /*mood_id=*/ MOOD_INDOOR_FLAT, /*count=*/ 4,
      //                       player rwalk  bwalk wandr hseek slowp pursu  flee flock  levy passr
      /*behavior_weights=*/ {    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
      //                     worker scout sentl leadr
      /*tier_weights=*/     {  2.0f, 0.0f, 2.0f, 1.0f },
      /*spawn_inner_radius=*/ 0.0f,
      /*spawn_radius=*/       60.0f,
      /*spawn_center_forward=*/ 0.0f,
      /*home_seeding_radius=*/ 30.0f },
    /* MOOD_INDOOR_VAULT — gallery walkers (SlowPatrol) */
    { /*mood_id=*/ MOOD_INDOOR_VAULT, /*count=*/ 4,
      //                       player rwalk  bwalk wandr hseek slowp pursu  flee flock  levy passr
      /*behavior_weights=*/ {    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
      //                     worker scout sentl leadr
      /*tier_weights=*/     {  2.0f, 0.0f, 2.0f, 1.0f },
      /*spawn_inner_radius=*/ 0.0f,
      /*spawn_radius=*/       60.0f,
      /*spawn_center_forward=*/ 0.0f,
      /*home_seeding_radius=*/ 30.0f },
    /* MOOD_FINITE_OUTDOOR — unpopulated */
    { /*mood_id=*/ MOOD_FINITE_OUTDOOR, /*count=*/ 0,
      /*behavior_weights=*/ {},
      /*tier_weights=*/     {},
      /*spawn_inner_radius=*/ 0.0f,
      /*spawn_radius=*/       0.0f,
      /*spawn_center_forward=*/ 0.0f,
      /*home_seeding_radius=*/ 0.0f },
    /* MOOD_OPEN_NIGHT — the open field's travelers, seventeen (OPEN_POP_0).
       The hour no longer thins them. Night, noon and sunset now carry one
       population, and night is the reason: a quiet night read as an empty
       one. The ring is unchanged, so this is density, not proximity. */
    { /*mood_id=*/ MOOD_OPEN_NIGHT, /*count=*/ 17,
      //                       player rwalk  bwalk wandr hseek slowp pursu  flee flock  levy passr
      /*behavior_weights=*/ {    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
      //                     worker scout sentl leadr
      /*tier_weights=*/     {  1.0f, 3.0f, 0.0f, 0.0f },
      /*spawn_inner_radius=*/ 200.0f,
      /*spawn_radius=*/       340.0f,
      /*spawn_center_forward=*/ 0.0f,
      /*home_seeding_radius=*/ 8.0f },
    /* MOOD_OPEN_NOON — the open field's travelers, seventeen (OPEN_POP_0). */
    { /*mood_id=*/ MOOD_OPEN_NOON, /*count=*/ 17,
      //                       player rwalk  bwalk wandr hseek slowp pursu  flee flock  levy passr
      /*behavior_weights=*/ {    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
      //                     worker scout sentl leadr
      /*tier_weights=*/     {  1.0f, 3.0f, 0.0f, 0.0f },
      /*spawn_inner_radius=*/ 200.0f,
      /*spawn_radius=*/       340.0f,
      /*spawn_center_forward=*/ 0.0f,
      /*home_seeding_radius=*/ 8.0f },
    /* MOOD_ATRIUM — THREE FIGURES (ATRIUM_8). Three read as a passage; five
       read as traffic, and thirty-one before them read as a crowd repelling
       itself. Jean backtracked on five.

       THE RING BEGINS AT THE ARC'S CENTRE (ATRIUM_9). It used to be drawn
       around the VISITOR, and a ring around the visitor is a ring the
       visitor is inside: a third of it stood behind them, unseen, and the
       rest arrived from the sides. spawn_center_forward walks the centre 40
       wu along the arrival gaze — ATRIUM_TABLE.arc_center_offset exactly,
       the point every door already faces — so the three begin where they
       are already going: in front, in the frame, walking a door.

       THE POSTER IS STILL CLEAR. sand[0] sits at 15 (A8.3); the ring's
       nearest point to the arrival stands at 40 - 22 = 18, so nothing
       spawns in the frame the visitor is reading. The outer 22 keeps the
       ring inside the shell with room to spare — at finite_radius 1 the
       centre lands within 5 wu of the room's own middle.

       tier_weights stays worker-only, and that is a decision rather than the
       oversight it now looks like beside the rolled figures: the tier
       multiplies speed_cap, so a rolled tier puts back exactly the frolic
       A8.1 took out (scout is 1.4x). One line to overturn when the pace is
       settled. */
    { /*mood_id=*/ MOOD_ATRIUM, /*count=*/ 3u,
      //                       player rwalk  bwalk wandr hseek slowp pursu  flee flock  levy passr
      /*behavior_weights=*/ {    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f },
      //                     worker scout sentl leadr
      /*tier_weights=*/     {  1.0f, 0.0f, 0.0f, 0.0f },
      /*spawn_inner_radius=*/ 6.0f,
      /*spawn_radius=*/       22.0f,
      /*spawn_center_forward=*/ 40.0f,
      /*home_seeding_radius=*/ 0.0f },
};

static_assert(sizeof(AGENT_POPULATIONS) / sizeof(AGENT_POPULATIONS[0]) == MOOD_COUNT,
              "AGENT_POPULATIONS must declare one row per mood");

// OIL_1 U6 (ledger: R4, C4): the per-mood weight sums, summed at COMPILE
// TIME in the SAME ascending order as the runtime loops they replace —
// identical float values by construction. One home, beside the table
// they summarize; the per-frame respawn reads these instead of
// re-summing a constexpr row every frame.
// TWO DERIVATIONS, ONE VALUE: spawn_population_for_mood still sums its
// own denominators inline (boot/transition cadence — outside the ledger
// row this unit answers, so it was left alone). They must agree; both
// feed the same normalization in populate_agent_slot_. If either the
// table or one summation is ever edited, edit the other.
inline constexpr std::array<float, MOOD_COUNT> AGENT_BEH_SUMS = [] {
    std::array<float, MOOD_COUNT> s{};
    for (uint32_t m = 0; m < MOOD_COUNT; m++)
        for (uint32_t b = 0; b < AGENT_BEHAVIOR_COUNT; b++)
            s[m] += AGENT_POPULATIONS[m].behavior_weights[b];
    return s;
}();
inline constexpr std::array<float, MOOD_COUNT> AGENT_TIER_SUMS = [] {
    std::array<float, MOOD_COUNT> s{};
    for (uint32_t m = 0; m < MOOD_COUNT; m++)
        for (uint32_t t = 0; t < AGENT_TIER_COUNT; t++)
            s[m] += AGENT_POPULATIONS[m].tier_weights[t];
    return s;
}();

// Row order must match the mood ids in MOOD_TABLE (mood.hpp).
// Unfolded rather than a constexpr loop — the restyle is a named
// later stage.
static_assert(AGENT_POPULATIONS[MOOD_OPEN_SUNSET   ].mood_id == MOOD_OPEN_SUNSET,    "AGENT_POPULATIONS row 0 must be MOOD_OPEN_SUNSET");
static_assert(AGENT_POPULATIONS[MOOD_INDOOR_FLAT   ].mood_id == MOOD_INDOOR_FLAT,    "AGENT_POPULATIONS row 1 must be MOOD_INDOOR_FLAT");
static_assert(AGENT_POPULATIONS[MOOD_INDOOR_VAULT  ].mood_id == MOOD_INDOOR_VAULT,   "AGENT_POPULATIONS row 2 must be MOOD_INDOOR_VAULT");
static_assert(AGENT_POPULATIONS[MOOD_FINITE_OUTDOOR].mood_id == MOOD_FINITE_OUTDOOR, "AGENT_POPULATIONS row 3 must be MOOD_FINITE_OUTDOOR");
static_assert(AGENT_POPULATIONS[MOOD_OPEN_NIGHT     ].mood_id == MOOD_OPEN_NIGHT,     "AGENT_POPULATIONS row 4 must be MOOD_OPEN_NIGHT");
static_assert(AGENT_POPULATIONS[MOOD_OPEN_NOON      ].mood_id == MOOD_OPEN_NOON,      "AGENT_POPULATIONS row 5 must be MOOD_OPEN_NOON");
static_assert(AGENT_POPULATIONS[MOOD_ATRIUM         ].mood_id == MOOD_ATRIUM,         "AGENT_POPULATIONS row 6 must be MOOD_ATRIUM");

// ═══ AGENT MODULE STATE ══════════════════════════════════════════

struct AgentState {
    GPUAgentState slots[Dim::MAX_AGENTS]            = {};
    uint32_t      respawn_counters[Dim::MAX_AGENTS] = {};
};

// ═══ MODULE FUNCTIONS — DECLARATIONS ═════════════════════════════

// Lifecycle
void upload_agent_registries_to_gpu(AgentsDeps* c, wgpu::Queue& queue);
void spawn_population_for_mood(AgentState& as, AgentsDeps* c,
                               uint32_t mood_id,
                               uint32_t seed,
                               float center_x, float center_z,
                               wgpu::Queue& queue);
void respawn_evicted_agents(AgentState& as, AgentsDeps* c,
                            uint32_t mood_id,
                            uint32_t world_seed,
                            wgpu::Queue& queue);
// Player commands
void try_possess_nearest(AgentState& as, AgentsDeps* c, wgpu::Queue& queue);
void seed_player_body(AgentState& as, AgentsDeps* c);
void reseed_player_body(AgentState& as, AgentsDeps* c, uint32_t preserved_tier,
                        float preserved_color_r, float preserved_color_g, float preserved_color_b,
                        uint32_t preserved_skin);
// Logging
void dump_agent_census(const AgentState& as, const AgentsDeps* c, const char* trigger);
void dump_passer_census(const AgentState& as, const AgentsDeps* c);   // ATRIUM_5 — the route state, read from the mirror

// ═══ IMPL:
// bodies deref agent_state_(own) + gpu/player/transitionPhase/world/time
// via AgentsDeps; read COLUMN_PALETTE (entities). COHORT: after entities
// (COLUMN_PALETTE) + patch_system (WorldState) + spine/state. No machine.

// ═══ REGISTRY UPLOAD (CPU table → GPU buffer, once at world-init) ═
//
// Both registries' truth is now a LIVE BANK — BEHAVIOR_LIVE and
// TIER_LIVE, the world's
// definition bank (contracts/agent_tiers.hpp), seeded at load from
// the authored AGENT_TIER_GAINS — so a panel edit to the bank
// outlives this author, because this author reads it every time it
// speaks. The compute kernels read both from GPU uniform buffers
// (bindings 110 / 111), uploaded at world-init by this helper and
// again at the frame boundary whenever the bank changes (ORGAN_2b).

// upload_agent_registries_to_gpu: takes Cartridge* for gpuState_
// access. No agent state needed — uploads constexpr registries only.
inline void upload_agent_registries_to_gpu(AgentsDeps* c, wgpu::Queue& queue) {
    GPUAgentBehaviorDef gpu_behaviors[AGENT_BEHAVIOR_COUNT] = {};
    for (uint32_t i = 0; i < AGENT_BEHAVIOR_COUNT; i++) {
        const auto& src = BEHAVIOR_LIVE.b[i];   // ORGAN_3 w3 — the world's definition, not the design table
        gpu_behaviors[i].step_rate       = src.step_rate;
        gpu_behaviors[i].step_size       = src.step_size;
        gpu_behaviors[i].persistence     = src.persistence;
        gpu_behaviors[i].drag            = src.drag;
        gpu_behaviors[i].home_pull       = src.home_pull;
        gpu_behaviors[i].neighbor_radius = src.neighbor_radius;
        gpu_behaviors[i].speed_cap       = src.speed_cap;
        gpu_behaviors[i].aux             = src.aux;   // ATRIUM_4 — the column travels
    }

    GPUAgentTierDef gpu_tiers[AGENT_TIER_COUNT] = {};
    for (uint32_t i = 0; i < AGENT_TIER_COUNT; i++) {
        const auto& src = TIER_LIVE.t[i];   // ORGAN_2b — the world's definition, not the design table
        gpu_tiers[i].step_gain     = src.step_gain;
        gpu_tiers[i].persist_gain  = src.persist_gain;
        gpu_tiers[i].speed_gain    = src.speed_gain;
        gpu_tiers[i].color_r       = src.color_r;
        gpu_tiers[i].color_g       = src.color_g;
        gpu_tiers[i].color_b       = src.color_b;
        gpu_tiers[i].contact_radius = src.contact_radius;   // TRUEBAND_CONTACT_1
        gpu_tiers[i].contact_mass   = src.contact_mass;
        gpu_tiers[i].personal_radius  = src.personal_radius;  // CONTACT_2
        gpu_tiers[i].flee_gain_player = src.flee_gain_player;
        gpu_tiers[i]._pad0 = 0.0f;
        gpu_tiers[i]._pad1 = 0.0f;
    }

    c->gpuState_.upload_agent_registries(queue,
        gpu_behaviors, AGENT_BEHAVIOR_COUNT,
        gpu_tiers,     AGENT_TIER_COUNT);

    c->gpuState_.upload_pawn_figures(queue);   // one-shot; packs PAWN_FIGURES -> GPU (H2)
}

// ═══ SHARED POPULATION HELPER ════════════════════════════════════

inline void populate_agent_slot_(const AgentState& as,
                          GPUAgentState& out,
                          const AgentPopulationDef& pop,
                          uint32_t agent_seed,
                          float beh_sum, float tier_sum,
                          float center_x, float center_z) {
    // ── Roll behavior ─────────────────────────────────────────────
    float w_beh[AGENT_BEHAVIOR_COUNT];
    for (uint32_t b = 0; b < AGENT_BEHAVIOR_COUNT; b++)
        w_beh[b] = pop.behavior_weights[b] / beh_sum;
    uint32_t behavior_id = select_tier(agent_seed, 1u, w_beh, AGENT_BEHAVIOR_COUNT);

    // ── Roll tier ─────────────────────────────────────────────────
    float w_tier[AGENT_TIER_COUNT];
    for (uint32_t t = 0; t < AGENT_TIER_COUNT; t++)
        w_tier[t] = pop.tier_weights[t] / tier_sum;
    uint32_t tier_idx = select_tier(agent_seed, 2u, w_tier, AGENT_TIER_COUNT);

    // ── Sample annulus position (uniform area distribution) ───────
    // ATRIUM_9 — THE ANNULUS RIDES THE ARRIVAL GAZE. The centre passed in
    // is where the VISITOR is; spawn_center_forward walks it along the
    // gaze before the ring is drawn, so a room can put its figures in
    // front of the viewer instead of all around them. The direction is
    // Idle::PAWN_HEADING's — the ARRIVAL gaze, a constant, not the live
    // heading: the composition is the room's, and it must not swing when
    // the visitor turns. Every mood but the atrium passes 0 here, and at
    // 0 both cosines fall out and the centre is the caller's, unmoved.
    const float two_pi = 6.28318530718f;
    const float gaze = heading_to_bearing(Idle::PAWN_HEADING);
    const float cx = center_x + std::cos(gaze) * pop.spawn_center_forward;
    const float cz = center_z + std::sin(gaze) * pop.spawn_center_forward;
    float theta = cpu_hash_f(agent_seed, 3u) * two_pi;
    const float inner_sq = pop.spawn_inner_radius * pop.spawn_inner_radius;
    const float outer_sq = pop.spawn_radius       * pop.spawn_radius;
    float u = cpu_hash_f(agent_seed, 4u);
    float r = std::sqrt(inner_sq + u * (outer_sq - inner_sq));
    float sx = cx + std::cos(theta) * r;
    float sz = cz + std::sin(theta) * r;

    // ── Sample home offset (uniform on disk around spawn point) ───
    float h_theta = cpu_hash_f(agent_seed, 5u) * two_pi;
    float h_r     = std::sqrt(cpu_hash_f(agent_seed, 6u)) * pop.home_seeding_radius;
    float hx = sx + std::cos(h_theta) * h_r;
    float hz = sz + std::sin(h_theta) * h_r;

    // ── Write the slot ────────────────────────────────────────────
    out.pos_x   = sx;   out.pos_y   = 0.0f; out.pos_z   = sz;
    out.home_x  = hx;   out.route   = 0u;   out.home_z  = hz;   // ATRIUM_4 — route 0 = fresh
    out.heading = 0.0f;
    out.vel_x   = 0.0f; out.vel_y   = 0.0f; out.vel_z   = 0.0f;
    out.orient_x = 0.0f; out.orient_y = 0.0f; out.orient_z = 0.0f; out.orient_w = 1.0f;
    out.seed           = agent_seed;
    out.behavior_id    = behavior_id;
    out.tier_idx       = tier_idx;
    out.is_active      = 1u;
    out.portal_trigger = -1;

    uint32_t ci = cpu_hash(agent_seed, 7u) % COLUMN_PALETTE_COUNT;
    out.color_r = COLUMN_PALETTE[ci][0];
    out.color_g = COLUMN_PALETTE[ci][1];
    out.color_b = COLUMN_PALETTE[ci][2];
    // ── Roll figure (skin_id) — global distribution, deterministic from seed ──
    // Family weighted by FIGURE_SHARES (salt 8u); member uniform within family
    // (salt 9u). Independent of the behavior/tier rolls (distinct salts).
    //
    // No branch — the entrance's figures roll like everyone's (ATRIUM_8).
    {
        float fw[FAM_COUNT];
        float fsum = 0.0f;
        for (uint32_t i = 0; i < FAM_COUNT; ++i) fsum += FIGURE_SHARES[i].share_pct;
        for (uint32_t i = 0; i < FAM_COUNT; ++i) fw[i] = FIGURE_SHARES[i].share_pct / fsum;

        uint32_t fam_i = select_tier(agent_seed, 8u, fw, FAM_COUNT);
        PawnFamilyId fam = FIGURE_SHARES[fam_i].family;   // FIGURE_SHARES is ordered REGULAR,SMOOTH,HERALDIC

        uint32_t base = figure_family_base(fam);
        uint32_t n    = figure_family_member_count(fam);
        if (n <= 1u) {
            out.skin_id = base;                            // single-member family (regular)
        } else {
            float mw[16];                                  // max 7 members; 16 is slack
            for (uint32_t i = 0; i < n; ++i) mw[i] = 1.0f / static_cast<float>(n);
            uint32_t m = select_tier(agent_seed, 9u, mw, n);
            out.skin_id = base + m;
        }
    }
}

// ═══ SPAWN ════════════════════════════════════════════════════════

inline void spawn_population_for_mood(AgentState& as, AgentsDeps* c,
                               uint32_t mood_id,
                               uint32_t seed,
                               float center_x, float center_z,
                               wgpu::Queue& queue) {
    if (mood_id >= MOOD_COUNT) return;
    const auto& pop = AGENT_POPULATIONS[mood_id];

    // Zero every non-player slot before refilling. The player's body
    // (slot PLAYER_SLOT) is preserved across mood transitions.
    for (uint32_t s = PLAYER_SLOT + 1; s < Dim::MAX_AGENTS; s++) {
        as.slots[s] = GPUAgentState{};
    }

    float beh_sum = 0.0f;
    for (uint32_t b = 0; b < AGENT_BEHAVIOR_COUNT; b++) beh_sum += pop.behavior_weights[b];
    float tier_sum = 0.0f;
    for (uint32_t t = 0; t < AGENT_TIER_COUNT; t++) tier_sum += pop.tier_weights[t];

    uint32_t spawned = 0;
    const uint32_t n = std::min(pop.count, Dim::MAX_AGENTS - 1u);
    if (n > 0 && beh_sum > 0.0f && tier_sum > 0.0f) {
        for (uint32_t i = 0; i < n; i++) {
            // Slot 0 is reserved for PLAYER_SLOT; non-player slots
            // pack densely from slot 1 upward.
            uint32_t slot = i + 1u;
            uint32_t agent_seed = cpu_hash(cpu_hash(seed, 0xA6E00000u + mood_id), i + 1u);

            populate_agent_slot_(as, as.slots[slot], pop, agent_seed,
                                 beh_sum, tier_sum,
                                 center_x, center_z);
            spawned++;
        }
    }

    c->gpuState_.upload_agent_state_all(queue, as.slots);
    std::cout << "[Agents] Spawned " << spawned << " for mood " << mood_id
              << " around (" << center_x << "," << center_z << ")\n";
}

// ═══ RESPAWN (per-frame, evicted slots → fresh agents) ════════════
//
// Writes only the changed slots — never the player slot — so the
// GPU's per-frame player update never sees a stale CPU snapshot.
// (respawn_counters lives in the CPU MIRROR section of agents.hpp.)

inline void respawn_evicted_agents(AgentState& as, AgentsDeps* c,
                            uint32_t mood_id,
                            uint32_t world_seed,
                            wgpu::Queue& queue) {
    if (mood_id >= MOOD_COUNT) return;
    const auto& pop = AGENT_POPULATIONS[mood_id];
    if (pop.count == 0) return;

    // OIL_1 U6: the sums are compile-time table facts (AGENT_BEH_SUMS /
    // AGENT_TIER_SUMS beside AGENT_POPULATIONS) — same order, identical
    // values; the per-frame re-sum of a constexpr row retired.
    const float beh_sum = AGENT_BEH_SUMS[mood_id];
    const float tier_sum = AGENT_TIER_SUMS[mood_id];
    if (beh_sum <= 0.0f || tier_sum <= 0.0f) return;

    const uint32_t possessed = c->player_.possessed_slot;
    // THE POINT: fresh agents cluster around the point —
    // point_.x/z, host-authored. Pawn-host value-identical (the
    // slot mirror and the point come from the same P5 harvest
    // snapshot); in free-fly the population spawns in the xz plane
    // around wherever you flew (Jean's ruling — presence follows
    // the point; behaviors unchanged).
    const float px = c->point_.x;
    const float pz = c->point_.z;

    const uint32_t n = std::min(pop.count, Dim::MAX_AGENTS - 1u);
    uint32_t respawned = 0;

    for (uint32_t i = 0; i < n; i++) {
        // Non-player slots pack densely from slot 1 upward.
        uint32_t slot = i + 1u;
        if (slot == possessed) continue;
        if (as.slots[slot].is_active != 0u) continue;

        as.respawn_counters[slot]++;
        uint32_t agent_seed = cpu_hash(
            cpu_hash(world_seed, 0xA6E00000u + mood_id),
            slot * 0x10001u + as.respawn_counters[slot] * 0x100u);

        populate_agent_slot_(as, as.slots[slot], pop, agent_seed,
                             beh_sum, tier_sum,
                             px, pz);

        c->gpuState_.upload_agent_slot(queue, slot, &as.slots[slot]);
        respawned++;
    }

    // RIBBON_4: respawn_evicted_agents is a per-frame spine row, and under a
    // fast point agents are evicted and respawned continuously — steady-state
    // chatter, not a transition witness. The mood-spawn line above stays.
    if constexpr (t7::INSTRUMENTS.stream_witness) {
        if (respawned > 0) {
            std::cout << "[Agents] Respawn " << respawned
                      << " around (" << px << "," << pz << ")\n";
        }
    }
}

// ═══ POSSESSION TRANSFER (Caps Lock) ══════════════════════════════

inline void try_possess_nearest(AgentState& as, AgentsDeps* c, wgpu::Queue& queue) {
    if (c->transitionPhase_ != TransitionPhase::IDLE) {
        std::cout << "[Possess] Blocked (mid-transition)\n";
        return;
    }

    const uint32_t cur = c->player_.possessed_slot;
    // THE POINT: possession reaches from the point — the
    // nearest agent to where you ARE. Pawn-host value-identical (same
    // harvest snapshot as the slot mirror); in free-fly Caps Lock
    // grabs a body wherever you flew (xz-plane reach, per the spawn
    // ruling — the population lives there now, so there is one).
    const float px = c->point_.x;
    const float pz = c->point_.z;

    int best_slot = -1;
    // ORGAN_4 P3b — THE SQUARE IS DERIVED HERE, from the live reach. The
    // retired POSSESSION_RADIUS_SQ was a constexpr twin, and a dialled
    // radius against a frozen square is the disagreement its DEFER row
    // named. One authority, squared where it is used.
    const float reach = PANEL_LIVE.possession.radius;
    float best_d2 = reach * reach;
    for (uint32_t s = 0; s < Dim::MAX_AGENTS; s++) {
        if (s == cur) continue;
        const auto& a = as.slots[s];
        if (a.is_active == 0u) continue;
        if (a.behavior_id == AGENT_BEHAVIOR_PLAYER_CONTROLLED) continue;

        float dx = a.pos_x - px;
        float dz = a.pos_z - pz;
        float d2 = dx * dx + dz * dz;
        if (d2 < best_d2) {
            best_d2 = d2;
            best_slot = (int)s;
        }
    }

    if (best_slot < 0) {
        std::cout << "[Possess] No agent within " << reach
                  << " units of the point at (" << px << "," << pz << ")\n";
        return;
    }

    const uint32_t new_slot = (uint32_t)best_slot;

    as.slots[cur].behavior_id = AGENT_BEHAVIOR_RANDOM_WALK;
    if (as.slots[cur].seed == 0u) {
        as.slots[cur].seed = cpu_hash(c->world_state_.active_seed, cur ^ 0xC11Cu);
    }

    // New slot → player control. Reset velocity + portal trigger so the
    // player's first frame on the new body is clean.
    as.slots[new_slot].behavior_id    = AGENT_BEHAVIOR_PLAYER_CONTROLLED;
    as.slots[new_slot].vel_x          = 0.0f;
    as.slots[new_slot].vel_z          = 0.0f;
    as.slots[new_slot].portal_trigger = -1;

    c->gpuState_.upload_agent_slot(queue, cur, &as.slots[cur]);
    c->gpuState_.upload_agent_slot(queue, new_slot, &as.slots[new_slot]);

    c->player_.possessed_slot = new_slot;
    c->gpuState_.set_possessed_slot(new_slot);

    std::cout << "[Possess] " << cur << " -> " << new_slot
              << " (tier " << as.slots[new_slot].tier_idx
              << ", dist " << std::sqrt(best_d2) << ")\n";
}

// ═══ DIAGNOSTIC: agent census ═════════════════════════════════════

// ═══ DIAGNOSTIC: passer census (ATRIUM_5) ════════════════════════
// The route state lives on the GPU; the readback mirrors it. One line per
// cadence: every passer's leg / phase / current door and its distance to
// the waypoint it is pulled to. Phases that advance say the machine walks;
// distances that never fall under the waypoint radius say where it stalls.
//
// DECODES THE WGSL PACKING (an L2-class mirror — the prose is the contract,
// and the two must move together). behavior_passer writes
//     route = (leg << 12) | (cur << 4) | (phase << 1) | 1
// with cur masked to 8 bits and phase to THREE (0x7u), not two: phase only
// ever holds 0..3 today, so a 2-bit read would agree by accident. The mask
// below is the kernel's, so a fourth-bit phase would show here rather than
// silently fold.
//
// The waypoint IS home_x/home_z (the kernel writes it there and the tether
// pulls to it), so `d` is exactly the distance the advance test measures
// against behaviors[PASSER].step_size — the waypoint radius on that row.
inline void dump_passer_census(const AgentState& as, const AgentsDeps* c) {
    std::cout << "[PASSER t=" << std::fixed << std::setprecision(1) << c->time_state_.seconds << "]";
    for (uint32_t i = PLAYER_SLOT + 1; i < Dim::MAX_AGENTS; i++) {
        const auto& a = as.slots[i];
        if (a.is_active == 0u || a.behavior_id != AGENT_BEHAVIOR_PASSER) continue;
        const uint32_t r = a.route;
        const uint32_t leg   = (r >> 12) & 0xFFFFFu;
        const uint32_t cur   = (r >> 4) & 0xFFu;
        const uint32_t phase = (r >> 1) & 0x7u;
        const uint32_t init  = r & 1u;
        const float dx = a.pos_x - a.home_x, dz = a.pos_z - a.home_z;
        std::cout << " s" << i << ":L" << leg << "P" << phase << "C" << cur << (init ? "" : "!")
                  << "d" << std::setprecision(0) << std::sqrt(dx * dx + dz * dz)
                  << std::setprecision(1);
    }
    std::cout << "\n";
}

inline void dump_agent_census(const AgentState& as, const AgentsDeps* c, const char* trigger) {
    uint32_t active = 0;
    uint32_t by_behavior[AGENT_BEHAVIOR_COUNT] = {};
    uint32_t by_tier[AGENT_TIER_COUNT] = {};

    for (uint32_t i = 0; i < Dim::MAX_AGENTS; i++) {
        const auto& a = as.slots[i];
        if (a.is_active == 0u) continue;
        active++;
        if (a.behavior_id < AGENT_BEHAVIOR_COUNT) by_behavior[a.behavior_id]++;
        if (a.tier_idx     < AGENT_TIER_COUNT)     by_tier[a.tier_idx]++;
    }

    std::cout << "[AGENTS t=" << std::fixed << std::setprecision(1) << c->time_state_.seconds
              << " trigger=" << trigger << "] " << active << "/" << Dim::MAX_AGENTS
              << " active, possessed=" << c->player_.possessed_slot;

    std::cout << " tier:{";
    bool first = true;
    for (uint32_t t = 0; t < AGENT_TIER_COUNT; t++) {
        if (by_tier[t] == 0) continue;
        if (!first) std::cout << " ";
        std::cout << AGENT_TIER_NAMES[t] << "=" << by_tier[t];
        first = false;
    }
    std::cout << "}";

    std::cout << " drv:{";
    first = true;
    for (uint32_t b = 0; b < AGENT_BEHAVIOR_COUNT; b++) {
        if (by_behavior[b] == 0) continue;
        if (!first) std::cout << " ";
        std::cout << AGENT_BEHAVIOR_NAMES[b] << "=" << by_behavior[b];
        first = false;
    }
    std::cout << "}";

    std::cout << "\n";
}


// ─── Player-body seeding (owner verbs) ─
// boot twin: slot 0 at the Idle pose, WORKER tier (the
// GPU-side twin is seeded by GPUState::initializeState).
inline void seed_player_body(AgentState& as, AgentsDeps* c) {
    (void)c;
    as.slots[0].pos_x = Idle::PAWN_POS_X;
    as.slots[0].pos_y = Idle::PAWN_POS_Y;
    as.slots[0].pos_z = Idle::PAWN_POS_Z;
    as.slots[0].heading = Idle::PAWN_HEADING;
    as.slots[0].orient_w = 1.0f;
    as.slots[0].is_active = 1u;
    as.slots[0].behavior_id = AGENT_BEHAVIOR_PLAYER_CONTROLLED;
    as.slots[0].tier_idx = AGENT_TIER_WORKER;
    as.slots[0].skin_id = 0u;   // player is always the regular pawn
    as.slots[0].portal_trigger = -1;
}

// Transition twin: keep the CPU mirror in sync with the GPU reset so
// patch streaming + ribbon + Caps Lock see current state; possession
// re-anchors to slot 0 (the possessed_slot write stays with the
// declared possession door's owner). Tier + colors + figure (skin_id)
// preserved by the caller across the reset — the possessed body's
// appearance set. The twins stay twins — byte-exactness
// outranks unification (PRIME INVARIANT); merging them is later
// material if ever pulled.
inline void reseed_player_body(AgentState& as, AgentsDeps* c, uint32_t preserved_tier,
                               float preserved_color_r, float preserved_color_g, float preserved_color_b,
                               uint32_t preserved_skin) {
    std::memset(as.slots, 0, sizeof(as.slots));
    as.slots[0].pos_x   = Idle::PAWN_POS_X;
    as.slots[0].pos_y   = Idle::PAWN_POS_Y;
    as.slots[0].pos_z   = Idle::PAWN_POS_Z;
    as.slots[0].heading = Idle::PAWN_HEADING;
    as.slots[0].orient_w = 1.0f;
    as.slots[0].is_active = 1u;
    as.slots[0].behavior_id = AGENT_BEHAVIOR_PLAYER_CONTROLLED;
    as.slots[0].tier_idx = preserved_tier;
    as.slots[0].color_r = preserved_color_r;
    as.slots[0].color_g = preserved_color_g;
    as.slots[0].color_b = preserved_color_b;
    as.slots[0].skin_id = preserved_skin;   // the figure rides with tier + color
    as.slots[0].portal_trigger = -1;
    c->player_.possessed_slot = 0;
}

} // namespace the_board
} // namespace t7
