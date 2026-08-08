#pragma once
#include <cstdint>
#include <iostream>       // census + the indoor-skip line
#include <cmath>      // std::floor, std::sqrt, std::min/max companions   // (impl, merged)
#include <algorithm>  // std::min, std::max   // (impl, merged)
#include <iomanip>    // census column formatting   // (impl, merged)
#include "cartridges/the_board/contracts/wgpu_fwd.hpp"   // wgpu handle fwds (lockstep insurance)

// ─── spawn_engine.hpp (S3 · MERGED: vocabulary + state + impl) ─────
// History: audit/LADDER.md
//
// How and when things appear: shared spawn helpers, footprint
// registry, proximity affinity, mesh-param rebuilds, distance
// culling, census, plus the dispatch loops that drive both generic
// and bespoke families through select → place → commit.
//
// SEAM[spawn_engine:P11] home of pattern P11 (templated active-array
//   helper) — run_spawn_preamble<C, ActiveT> is the canonical
//   instance. One implementation, ten callers.
// The EntityQueueEntry / PlacementEntry unions and every type they
//   embed live in entity_types.hpp (the contract home); this module
//   holds only the queues and loops. spawn_engine stays ONE pair.
//
// Depends on cohort include order: roster.hpp (PopFamily),
// entity_types.hpp (queue unions), state.hpp (GPU mesh params),
// grounded.hpp (ActiveColumn/EntitiesState — COMPLETE, the merged
// bodies deref them), patch_system.hpp (Dim::PATCH_EXTENT — the preamble
// template reads it at definition), renderer.hpp. MERGED at the
// cohort tail (the B ruling): the decl tier
// lives in contracts/spawn_services.hpp; every pre-tail caller binds
// by same-TU late definition (templates at end-of-TU).

namespace t7 {
namespace the_board {

// ─── Entity Distance Culling — THE RING (re-ruled) ─────────────────
//
// THE RING is the draw authority (chain, state.hpp Dim; live value =
// config veil_ring): an entity is IN the draw set iff any part of it
// reaches inside the ring — center-distance MINUS its horizontal extent
// ≤ ring (the "center±extent" metric; replaces the retired per-size
// inset). Hysteresis sits OUTSIDE the ring, in the fully-iced zone:
//   show when (dist − extent) ≤ ring        (entering fragments are at
//                                            icing = 1 → invisible join)
//   hide when (dist − extent) > ring + HYST (fragments fully iced for
//                                            the whole band → invisible exit)
// Both toggle edges are behind the icing — materialize inside the fade.
inline constexpr float ENTITY_CULL_HYSTERESIS     = 40.0f;   // toggle band, wholly beyond the ring
inline constexpr float ENTITY_THIN_EXTENT         = 5.0f;    // columns/antennas: conservative horizontal half-reach

// ── Footprint registry vocabulary ──────────────────────────────────

struct GroundFootprint {
    float x = 0.0f, z = 0.0f;
    float radius = 0.0f;
    int32_t patch_gx = 0, patch_gz = 0;
    uint32_t family = UINT32_MAX;  // PopFamily index
    uint32_t slot = UINT32_MAX;    // slot within that family — (family, slot) IS the owner
    uint32_t tier = 0;             // tier index within family
    float spawn_time = 0.0f;       // time_state_.seconds at registration
    bool active = false;
};

inline constexpr uint32_t MAX_FOOTPRINTS = 128;
inline constexpr float CENSUS_DUMP_INTERVAL = 30.0f;
// Hard ceiling on the census detail listing. The arrivals filter should
// already bound it well under this; the cap is what guarantees the print
// can never cost a frame regardless of what the world does. An instrument
// must be cheap enough not to become the phenomenon.
inline constexpr uint32_t CENSUS_LISTING_MAX = 12;

// ── Proximity affinity ─────────────────────────────────────────────
//
// WHAT: the clustering system — five tables, one mechanism. When a
//   family with a non-zero AFFINITY row is being placed, nearby
//   attractor footprints (a) MULTIPLY its spawn chance (boost) and
//   (b) SHRINK its required separation gap (gap reduction).
// AXES: the four vectors are indexed by the PLACING family; the matrix
//   is PROXIMITY_AFFINITY[placing][existing-neighbor] — e.g.
//   [Palm][Palm]=0.65 means a palm being placed is strongly attracted
//   to standing palms; [Palm][Cactus]=0.3 a milder pull.
// UNITS: RADIUS = wu (neighbor-scan distance around the candidate);
//   MAX_BOOST = multiplier ceiling on the spawn-chance boost;
//   THRESHOLD = count (minimum qualifying neighbors before any boost);
//   GAP_REDUCTION = fraction 0-1 of MIN_SEPARATION removed, scaled by
//   the pair's affinity; AFFINITY = dimensionless weight 0-1, summed
//   over neighbors into boost = min(1 + Σaff, MAX_BOOST).
// ORDER: every axis follows PopFamily order (PYRAMID=0 … GALLERY=11),
//   PINNED by the F-1 static_assert at roster.hpp.
// CONSUMERS: proximity_affinity_boost() below (RADIUS/MAX_BOOST/
//   THRESHOLD/AFFINITY → the adj_mod spawn multiplier);
//   check_position() (AFFINITY × GAP_REDUCTION → the effective gap);
//   proximity_row_active() (constexpr row precheck).
// SENTINELS: RADIUS 0 = family never scans (boost hard-disabled);
//   MAX_BOOST 1.0 = no boost possible; THRESHOLD 0 = no minimum;
//   GAP_REDUCTION 0 = gap never shrinks; an all-zero AFFINITY row
//   short-circuits the whole mechanism for that family.
// Spawn + placement determinant — frozen biography (§12): these numbers
// shape both the rate and the geometry of every cluster ever born.
// Only COLUMN and the flora trio (PALM/CACTUS/BLADE) cluster today.

//                              Pyr    Arch   Col    Ant    Palm   Cact   Blad   Sph    Ribn   Cube   GoL    Gall
inline constexpr float    PROXIMITY_RADIUS[PopFamily::COUNT] = { 0.0f,  0.0f, 60.0f,  0.0f,150.0f,120.0f,120.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f };   // wu; 0 = never scans
inline constexpr float    PROXIMITY_MAX_BOOST[PopFamily::COUNT] = { 1.0f,  1.0f,  2.0f,  1.0f,  3.0f,  3.0f,  3.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f };   // ×ceiling; 1 = no boost
inline constexpr uint32_t PROXIMITY_THRESHOLD[PopFamily::COUNT] = { 0,     0,     2,     0,     1,     1,     1,     0,     0,     0,     0,     0 };   // min neighbors; 0 = none
inline constexpr float    PROXIMITY_GAP_REDUCTION[PopFamily::COUNT] = { 0.0f, 0.0f, 0.3f, 0.0f, 0.6f, 0.6f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };   // fraction of MIN_SEPARATION; 0 = keep full gap

// AFFINITY[placing][existing]: rows follow PopFamily; only Col + the
// flora trio have non-zero rows (all others never cluster).
inline constexpr float PROXIMITY_AFFINITY[PopFamily::COUNT][PopFamily::COUNT] = {
    //           near: Pyr   Arch  Col   Ant   Palm  Cact  Blad  Sph   Ribn  Cube  GoL   Gall
    /* Pyr   */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    /* Arch  */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    /* Col   */ { 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    /* Ant   */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    /* Palm  */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.65f, 0.3f, 0.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    /* Cact  */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.5f, 0.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    /* Blad  */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.3f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    /* Sph   */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    /* Ribn  */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    /* Cube  */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    /* GoL   */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    /* Gall  */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
};

// Precomputed: does this family have any non-zero affinity?
inline constexpr bool proximity_row_active(uint32_t family) {
    for (uint32_t f = 0; f < PopFamily::COUNT; f++)
        if (PROXIMITY_AFFINITY[family][f] > 0.0f) return true;
    return false;
}

// ═══ MODULE STATE ══════════════════════════════════════════════════

// ═══ BESPOKE-FAMILY SELECTION/PLACEMENT PAYLOADS ═════════════════
//
// Three bespoke families (GoL, Gallery, Ribbon) don't fit the
// generic pipeline's EntityInstance shape — their selection
// records carry family-specific fields (lattice node, painting
// count, wave parameters). The payload DTOs AND the tagged unions
// that carry them (EntityQueueEntry / PlacementEntry) live together
// in entity_types.hpp — the contract home; a DTO that exists to
// cross a boundary belongs to the boundary's contract. See
// SEAM[spawn_engine:structural] in the file header.

// ─── The queues (machine state) ──────────────────────────────────
//
// EntityQueueEntry / PlacementEntry are contract vocabulary
// (entity_types.hpp); the QUEUES they fill are spine state and live
// here. entityQueue_ decouples WHAT exists from WHERE it goes;
// placementResults_ holds entities past spatial negotiation, ready
// for GPU commit.

// Instance (spawn_engine_state_) lives at the composition root.
// THE BOUND, PROVEN — not estimated. spawn_selected_patches
// (surface/patch_system.hpp) is the SOLE caller of all three queue verbs and
// runs them as one straight line, within a single frame:
//
//   fill    select_entities_for_patch, once per patch, for at most
//           SPAWN_BUDGET_PER_FRAME patches; each pushes at most one entry per
//           family, so at most PopFamily::COUNT per patch
//   drain   place_entity_queue  — unconditional full-range loop, then an
//           unconditional reset of the count
//   drain   commit_entity_queue — same shape
//
// There is NO partial drain and no early exit in either drain, so nothing can
// carry into the next frame's fill. That is what makes the bound a bound.
// placementResults_ takes at most one push per entityQueue_ entry, so it
// shares the ceiling and cannot exceed it.
//
// Derived symbolically rather than written as 48: raising the spawn budget or
// adding a family moves the capacity with it, instead of silently outgrowing a
// literal.
inline constexpr uint32_t SPAWN_QUEUE_MAX = SPAWN_BUDGET_PER_FRAME * PopFamily::COUNT;

struct SpawnEngineState {
    EntityQueueEntry entityQueue_[SPAWN_QUEUE_MAX]{};
    uint32_t         entityQueueCount_ = 0;
    PlacementEntry   placementResults_[SPAWN_QUEUE_MAX]{};
    uint32_t         placementCount_ = 0;
    GroundFootprint  footprints_[MAX_FOOTPRINTS]{};
    float lastCensusDump_ = -999.0f;
};

// ═══ MODULE FUNCTIONS ══════════════════════════════════════════════
//
// DECLARATIONS live in contracts/spawn_services.hpp (the
// machine's decl tier) with the boundary DTOs
// (SpawnGatePreambleResult / PositionResult / SpawnPreamble), the
// ActiveColumn fwd, MIN_SEPARATION, and GLOBAL_ENTITY_DENSITY (gol +
// gallery read it pre-tail). Definitions are all below.

// ── Helper 1: SpawnGatePreamble ──────────────────────────────

// SEAM[spawn_engine:P11] the canonical templated active-array helper.
// THE TEMPLATE KEYHOLE, retired to a doorway:
// every instantiation now deduces C = MachineCtx (the machine face);
// the DECLARATION lives in contracts/spawn_services.hpp so the ten
// pre-tail callers bind here at end-of-TU instantiation. The typename
// C stays — one implementation, ten callers, the active-array type
// still varies per family (ActiveT).
template<typename C, typename ActiveT>
SpawnGatePreambleResult run_spawn_preamble(C* c,
    int32_t gx, int32_t gz,
    ActiveT* active_arr, uint32_t max_instances,
    uint32_t spawn_roll_prop, float spawn_chance,
    const float* mood_mult,
    uint32_t family)
{
    SpawnGatePreambleResult r{};
    r.ok = false;

    // 1. Idempotency
    for (uint32_t i = 0; i < max_instances; i++) {
        if (active_arr[i].active &&
            active_arr[i].patch_gx == gx &&
            active_arr[i].patch_gz == gz) {
            return r;
        }
    }

    // 2-6. THE COMPOSITION LAW: the stack, authored once —
    // mood → global → tile (F3) → proximity → base × adj → min(·,1).
    // Generic semantics: multiply-through on mood zero (no veto flag),
    // proximity ON, MIN1 clamp.
    r.theme_idx = c->themes_state_.temporal_flavor;
    auto composed = compose_spawn_chance(c, gx, gz, family,
        spawn_chance, mood_mult,
        /*use_proximity=*/true, /*veto_on_zero_mood=*/false,
        SpawnClamp::MIN1);

    // 7. Spawn gate (seed + roll; the chance arrives composed)
    auto ctx = evaluate_spawn_gate(c, gx, gz, spawn_roll_prop, composed.chance);
    if (!ctx.passed) return r;

    // 8-9. Find and reserve slot
    uint32_t slot = UINT32_MAX;
    for (uint32_t i = 0; i < max_instances; i++) {
        if (!active_arr[i].active) { slot = i; break; }
    }
    if (slot == UINT32_MAX) return r;
    active_arr[slot].active = true;


    r.seed = ctx.seed;
    r.slot = slot;
    r.ok = true;
    return r;
}

// ── The generic gate: one law, one per-family fact ─────────────────
//
// Nine families ran identical bodies here, each restating five constants that
// its own TRAITS row already declares — max_instances, spawn_roll_prop,
// spawn_chance, mood_multiplier, family_id — around one call. The restating
// was why four of those fields read as DEAD: the row was the right home and
// nobody read it, so the cut was about to remove the home and keep the nine
// duplicates.
//
// The only genuinely per-family fact is the ACTIVE ARRAY. Its type varies, so
// it stays a parameter and the template deduces ActiveT from it; everything
// else travels as data on the traits row.
//
// BIT-IDENTITY: same callee, same arguments, same order. run_spawn_preamble is
// untouched. The SpawnGatePreambleResult -> SpawnGateOutput conversion moves
// from nine copies to one; note the two structs order their fields
// DIFFERENTLY (preamble: seed, slot, theme_idx, ok — output: ok, seed, slot,
// theme_idx), so this is a real field-by-field reorder and must stay written
// out rather than becoming a cast or a copy.
template<typename ActiveT>
inline SpawnGateOutput gate_from_traits(MachineCtx* c, int32_t gx, int32_t gz,
    const EntityFamilyTraits& t, ActiveT* active_arr)
{
    auto gate = run_spawn_preamble(c, gx, gz,
        active_arr, t.max_instances,
        t.spawn_roll_prop, t.spawn_chance,
        t.mood_multiplier, t.family_id);
    return { gate.ok, gate.seed, gate.slot, gate.theme_idx };
}


// ═══ MODULE IMPLEMENTATION ════════════════════════════════════════
//
// The engine's verbs: position negotiation, the footprint registry,
// mesh-param rebuilds + distance culling, the census, gate
// evaluation, proximity affinity, and the select → place → commit
// dispatch loops. Reaches the machine face for the root organs
// (c->world_state_ / c->time_state_ / c->mood_state_ /
// c->themes_state_ / c->tile_world_state_ / c->entities_state_ /
// c->player_) and the GPU wire (c->gpuState_); the loops route
// through FAMILY_DISPATCH with the machine face as the row argument.


// ── Helper 1b: the indoor bounds law ────────────────────────
//
// One law for every placement site (negotiate_position + the
// gallery's own site). In finite indoor worlds, push the
// candidate inward so the clamped radius stays at least
// INDOOR_ENTITY_WALL_MARGIN from every wall. We clamp instead of
// rejecting because rejection would silently drop entities
// anchored to corner patches (their seed-determined position
// keeps landing in the wall margin and never recovers). Clamping
// shifts the candidate to the boundary of the legal box, then
// the existing footprint-overlap check handles any pile-ups.
//
// MARGIN clamps footprint_r (today's law; a collapsed box falls
// back to the room center — max footprint at radius=1 is 65,
// capped entities are well under that). FULL clamps
// containment_r — the family's WHOLE extent stays inside
// (ribbon: scaled lateral_amp + the scaled cube span; gallery:
// the fan formula) — and a collapsed box SKIPS the spawn with
// one loud line: cramming is worse than absence. FREE never
// clamps (gol may straddle).
inline bool indoor_bounds_clamp(MachineCtx* c, uint32_t family,
    float footprint_r, float containment_r, float& cx, float& cz)
{
    if (!(c->world_state_.finite_mode && MOOD_TABLE[c->mood_state_.active].indoor))
        return true;
    const IndoorBounds bounds = INDOOR_TREATMENT[family].bounds;
    if (bounds == IndoorBounds::FREE) return true;

    float bmin = -(float)c->world_state_.finite_radius * Dim::PATCH_EXTENT;
    float bmax = ((float)c->world_state_.finite_radius + 1.0f) * Dim::PATCH_EXTENT;
    float clearance = INDOOR_ENTITY_WALL_MARGIN
        + (bounds == IndoorBounds::FULL ? containment_r : footprint_r);
    float lo = bmin + clearance;
    float hi = bmax - clearance;
    if (lo > hi) {
        if (bounds == IndoorBounds::FULL) {
            std::cout << "[DIAG:INDOOR-SKIP] " << family_short_name(family)
                      << " containment_r=" << containment_r
                      << " exceeds the room — spawn skipped\n";
            return false;
        }
        float center = (bmin + bmax) * 0.5f;
        cx = center;
        cz = center;
        return true;
    }
    if (cx < lo) cx = lo;
    else if (cx > hi) cx = hi;
    if (cz < lo) cz = lo;
    else if (cz > hi) cz = hi;
    return true;
}

// ── Helper 2: NegotiatePosition ─────────────────────────────

inline PositionResult negotiate_position(MachineCtx* c,
    uint32_t seed, int32_t trigger_gx, int32_t trigger_gz,
    uint32_t pos_x_prop, uint32_t pos_z_prop, float jitter,
    uint32_t rotation_seed_prop,
    bool grounded,
    float footprint_r, float containment_r, uint32_t family, uint32_t slot, uint32_t tier)
{
    PositionResult r{};
    r.ok = false;

    // 1. Jittered position
    jittered_position(seed, trigger_gx, trigger_gz,
        pos_x_prop, pos_z_prop, jitter, r.cx, r.cz);
    r.rotation = cpu_hash_f(seed, rotation_seed_prop) * 6.283185f;

    // The indoor bounds law rides INDOOR_TREATMENT (contracts/
    // indoor_module.hpp): MARGIN keeps the standing wall-margin
    // clamp; FULL (ribbon on this path) clamps the caller-supplied
    // containment extent so the whole family stays inside; FREE
    // skips (gol never crosses this negotiation — it places by
    // patch cell). A collapsed FULL box skips the spawn.
    if (!indoor_bounds_clamp(c, family, footprint_r, containment_r, r.cx, r.cz))
        return r;

    // 2. Separation + footprint check — GROUND CLAIM, and only for bodies
    // that touch the ground. A non-grounded family (sphere, cube) is not
    // blocked by a standing footprint: a pyramid beneath a hovering cube is a
    // composition, not a collision. Ruling 21 — the campaign law is "a family
    // registers iff its own extent touches the ground plane", which is what
    // `grounded` already means and says.
    if (grounded && !check_position(c, r.cx, r.cz, footprint_r, family))
        return r;

    // 3. Host patch. NOT part of the ground claim — this is the entity's
    // address for eviction bookkeeping and EVERY family needs it, floaters
    // included: dispatch_commit_sphere_generic (spheres.hpp) and
    // dispatch_commit_cube_generic (cube_behaviors.hpp) both gate their commit
    // on find_patch(host_gx, host_gz). Step 3 used to fuse this with
    // registration as "one key derivation"; the fusion is why the two look
    // like one concept. They are not, and skipping this alongside the
    // footprint would leave every floater addressed to patch (0,0) — silently
    // dropping any that spawned away from the world origin.
    auto hk = tile_key(r.cx, r.cz);
    r.host_gx = hk.x; r.host_gz = hk.z;

    // 4. Footprint registration — the claim itself. Skipped for the same
    // families, in the same direction: they claim no ground. Both steps
    // conditional, or the ruling is half-applied.
    if (grounded && register_footprint(c, r.cx, r.cz, footprint_r,
        r.host_gx, r.host_gz, family, slot, tier) == UINT32_MAX) return r;

    r.ok = true;
    return r;
}

// ═══ MESH GEN PREPARERS + CULLING ════════════════════════════════

// ─── Column / Arch / Pyramid mesh-gen preparers ───────────────

// Rebuild GPUArchMeshParams from cached ActiveArch data.
inline GPUArchMeshParams build_arch_mesh_params(MachineCtx* c, uint32_t slot) {
    const auto& a = c->entities_state_.arches[slot];
    GPUArchMeshParams p{};
    p.center_x = a.world_x;
    p.center_z = a.world_z;
    p.rotation = a.rotation;
    p.half_span = a.half_span;
    p.rise = a.rise;
    p.depth = a.depth;
    p.thickness = a.thickness;
    p.pier_height = a.pier_height;
    p.burial = a.burial;
    p.catenary_a = solve_catenary_a(a.half_span, a.rise);
    p.segs_u = a.segs_u;
    p.segs_v = a.segs_v;
    // PORTAL_1: no branch. col_* carries the portal override from the decision.
    p.color_r = a.col_r; p.color_g = a.col_g; p.color_b = a.col_b;
    p.mosaic_seed = a.mosaic_seed;   // portals zeroed at the decision — MOSAIC_2b
    p.is_active = 1;
    return p;
}

// Rebuild GPUColumnMeshParams from cached ActiveColumn data.
inline GPUColumnMeshParams build_column_mesh_params_from(const ActiveColumn& c) {
    GPUColumnMeshParams p{};
    p.center_x = c.world_x;
    p.center_z = c.world_z;
    p.height = c.height;
    p.shaft_radius = c.shaft_radius;
    p.taper = c.taper;
    p.entasis = c.entasis;
    p.base_height = c.base_height;
    p.base_overhang = c.base_overhang;
    p.capital_height = c.cap_height;
    p.capital_overhang = c.cap_overhang;
    p.burial = c.burial;
    p.color_r = c.col_r;
    p.color_g = c.col_g;
    p.color_b = c.col_b;
    p.mosaic_seed = c.mosaic_seed;   // MOSAIC_1 — the Q3 one-producer: commit + reupload/cull both ride this; antennas arrive plain (zeroed ActiveColumn)
    p.base_layers = c.base_layers;
    p.capital_layers = c.cap_layers;
    p.segs_around = c.segs_around;
    p.shaft_rings = c.shaft_rings;
    p.is_active = 1;
    p.tier = c.tier_idx;
    p.drum_color_r1 = c.drum_colors[0];
    p.drum_color_g1 = c.drum_colors[1];
    p.drum_color_b1 = c.drum_colors[2];
    p.drum_color_r2 = c.drum_colors[3];
    p.drum_color_g2 = c.drum_colors[4];
    p.drum_color_b2 = c.drum_colors[5];
    p.drum_color_r3 = c.drum_colors[6];
    p.drum_color_g3 = c.drum_colors[7];
    p.drum_color_b3 = c.drum_colors[8];
    return p;
}

inline GPUColumnMeshParams build_column_mesh_params(MachineCtx* c, uint32_t slot) {
    return build_column_mesh_params_from(c->entities_state_.columns[slot]);
}

// Scan all active entities, toggle draw_visible with hysteresis,
// and upload mesh param changes. Returns count of currently hidden entities.
// THE RING is the correctness gate (re-ruled): draw membership = any part
// of the entity inside the live ring (center − extent ≤ ring). Anchor: the
// point (readback — the same yardstick as the terrain band). Both toggle
// edges sit at/beyond the ring where the icing is already 1 — invisible.
inline uint32_t update_entity_draw_visibility(MachineCtx* c, wgpu::Queue& queue) {
    uint32_t culled = 0;

    const float ring = c->gpuState_.veil_ring();   // live chain value — the draw authority

    // Arches
    for (uint32_t i = 0; i < Dim::MAX_ARCH_INSTANCES; i++) {
        if (!c->entities_state_.arches[i].active) continue;
        const auto& a = c->entities_state_.arches[i];
        float dx = a.world_x - c->point_.x;
        float dz = a.world_z - c->point_.z;
        float dist = std::sqrt(dx * dx + dz * dz);

        float nearest = dist - a.half_span;            // the arch's closest reach (center − extent)
        bool should_show = a.draw_visible
            ? (nearest <= ring + ENTITY_CULL_HYSTERESIS)  // visible: hide once fully iced past the band
            : (nearest <= ring);                          // hidden:  show as fragments enter the icing

        if (should_show != a.draw_visible) {
            c->entities_state_.arches[i].draw_visible = should_show;
            if (should_show) {
                c->gpuState_.upload_arch_mesh_params_slot(queue, i, build_arch_mesh_params(c, i));
            }
            else {
                GPUArchMeshParams empty{};
                c->gpuState_.upload_arch_mesh_params_slot(queue, i, empty);
            }
            c->entities_state_.arch_mesh_gen_pending = true;
        }

        if (!c->entities_state_.arches[i].draw_visible) culled++;
    }

    // Columns
    for (uint32_t i = 0; i < Dim::MAX_COLUMN_ONLY; i++) {
        if (!c->entities_state_.columns[i].active) continue;
        const auto& col = c->entities_state_.columns[i];
        float dx = col.world_x - c->point_.x;
        float dz = col.world_z - c->point_.z;
        float dist = std::sqrt(dx * dx + dz * dz);

        float nearest = dist - std::max(col.shaft_radius, ENTITY_THIN_EXTENT);
        bool should_show = col.draw_visible
            ? (nearest <= ring + ENTITY_CULL_HYSTERESIS)
            : (nearest <= ring);

        if (should_show != col.draw_visible) {
            c->entities_state_.columns[i].draw_visible = should_show;
            if (should_show) {
                c->gpuState_.upload_column_mesh_params_slot(queue, i, build_column_mesh_params(c, i));
            }
            else {
                GPUColumnMeshParams empty{};
                c->gpuState_.upload_column_mesh_params_slot(queue, i, empty);
            }
            c->entities_state_.column_mesh_gen_pending = true;
        }

        if (!c->entities_state_.columns[i].draw_visible) culled++;
    }

    // Antennas
    for (uint32_t i = 0; i < Dim::MAX_ANTENNA_ONLY; i++) {
        if (!c->entities_state_.antennas[i].active) continue;
        const auto& ant = c->entities_state_.antennas[i];
        float dx = ant.world_x - c->point_.x;
        float dz = ant.world_z - c->point_.z;
        float dist = std::sqrt(dx * dx + dz * dz);
        uint32_t gpu_slot = i + Dim::ANTENNA_SLOT_OFFSET;

        float nearest = dist - ENTITY_THIN_EXTENT;   // antennas are thin masts
        bool should_show = ant.draw_visible
            ? (nearest <= ring + ENTITY_CULL_HYSTERESIS)
            : (nearest <= ring);

        if (should_show != ant.draw_visible) {
            c->entities_state_.antennas[i].draw_visible = should_show;
            if (should_show) {
                c->gpuState_.upload_column_mesh_params_slot(queue, gpu_slot, build_column_mesh_params_from(ant));
            }
            else {
                GPUColumnMeshParams empty{};
                c->gpuState_.upload_column_mesh_params_slot(queue, gpu_slot, empty);
            }
            c->entities_state_.column_mesh_gen_pending = true;
        }

        if (!c->entities_state_.antennas[i].draw_visible) culled++;
    }

    return culled;
}

// ═══ FOOTPRINT REGISTRY ══════════════════════════════════════════

inline bool check_position(MachineCtx* c, float px, float pz, float placing_radius,
    uint32_t placing_family) {
    for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
        if (!c->spawn_engine_state_.footprints_[i].active) continue;
        float dx = px - c->spawn_engine_state_.footprints_[i].x;
        float dz = pz - c->spawn_engine_state_.footprints_[i].z;
        float effective_min = placing_radius + c->spawn_engine_state_.footprints_[i].radius;
        if (c->spawn_engine_state_.footprints_[i].family < PopFamily::COUNT) {
            float min_gap = MIN_SEPARATION[placing_family][c->spawn_engine_state_.footprints_[i].family];
            if (min_gap > 0.0f) {
                float aff = PROXIMITY_AFFINITY[placing_family][c->spawn_engine_state_.footprints_[i].family];
                if (aff > 0.0f) min_gap *= (1.0f - aff * PROXIMITY_GAP_REDUCTION[placing_family]);
                effective_min += min_gap;
            }
        }
        if (dx * dx + dz * dz < effective_min * effective_min) return false;
    }
    return true;
}

inline uint32_t register_footprint(MachineCtx* c, float x, float z, float radius,
    int32_t gx, int32_t gz, uint32_t family,
    uint32_t slot, uint32_t tier) {
    for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
        if (!c->spawn_engine_state_.footprints_[i].active) {
            c->spawn_engine_state_.footprints_[i] = { x, z, radius, gx, gz, family, slot, tier, c->time_state_.seconds, true };
            return i;
        }
    }
    // SATURATION WAS SILENT, and which family lost was decided by PopFamily
    // order — the tail (cube, gol, gallery) simply stopped appearing, with no
    // symptom anywhere. Capacity stays 128 (ruling 8: post-SPAWN_2 occupancy
    // peaked at 65% and settles near 45%), so if this ever prints, the leak it
    // names is the thing to fix, not the number.
    std::cerr << "[SPAWN] footprint registry FULL (" << MAX_FOOTPRINTS
              << ") — dropping " << family_short_name(family)
              << " slot " << slot << "; spawn silently denied\n";
    return UINT32_MAX;
}

// Release by OWNER. The (family, slot) pair is the identity; nothing stores the
// registry index anywhere, deliberately.
//
// A stored index would be a second copy of a fact the registry already holds,
// and it would have to be threaded through PositionResult, the placement DTOs
// and every family's active record to reach the evictor. This campaign has now
// found EIGHT defects whose shape was two copies of one fact. A 128-slot scan
// costs nothing at eviction cadence (EVICT_BUDGET_PER_FRAME is 4) and cannot
// drift, because there is nothing to drift from.
inline void unregister_footprint_for(MachineCtx* c, uint32_t family, uint32_t slot) {
    for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
        auto& fp = c->spawn_engine_state_.footprints_[i];
        if (fp.active && fp.family == family && fp.slot == slot) {
            fp.active = false;
            return;   // one footprint per owner
        }
    }
}

// ═══ ENTITY CENSUS ═══════════════════════════════════════════════

inline const char* family_short_name(uint32_t family) {
    static const char* NAMES[] = { "pyr", "arch", "col", "ant", "palm", "cact", "blad", "sph", "ribn", "cube", "gol", "gall" };
    return (family < PopFamily::COUNT) ? NAMES[family] : "???";
}

// TWO INDEPENDENT REGISTRIES, ONE LINE. `active` is the family's own
// array, scanned through FAMILY_DISPATCH[f].active_count. `claimed` is
// the footprints keyed to that family. They measure different things
// and MUST agree: delta = claimed − active, and a nonzero delta is a
// leak that names its family — a body with no ground, or ground with
// no body.
//
// The old print walked footprints_[] alone and called the result
// "entities". It reported a proxy: it could not see a family that had
// spawned without registering, and it counted ground that outlived its
// owner as though the owner were alive.
// The delta column: explicit sign, except zero — which has none. Zero is
// the resting state and reads as noise with a sign glued to it.
inline void census_put_delta(int32_t d) {
    if (d == 0) std::cout << std::setw(8) << 0;
    else        std::cout << std::setw(8) << std::showpos << d << std::noshowpos;
}

// NON-PARTICIPANT, not zero. A family that claims no ground (ruling 21) has
// nothing to report in the footprint-derived columns, and `0` would be a
// MEASUREMENT — it would read as "registered nothing", which is a different
// claim from "does not register". The distinction is load-bearing: after
// SPAWN_2 the floaters hold a live `active` against no footprint forever, and
// a row that disagrees permanently and CORRECTLY teaches the reader to
// discount the delta column — the one thing here that catches real leaks.
// Do not "repair" these back to 0.
//
// Padded by hand because the em-dash is three UTF-8 bytes and one display
// column, and setw counts bytes.
inline void census_put_dash(int width) {
    for (int i = 1; i < width; i++) std::cout << ' ';
    std::cout << "—";
}

inline void dump_entity_census(MachineCtx* c, const char* trigger) {
    // The claimed side: footprints, keyed by family. `arrived` (the `new`
    // column) rides this same pass — one scan, not two.
    //
    // NAME IT HONESTLY: `new` is FOOTPRINT-derived, so it counts claimed
    // arrivals, not active ones. It is consistent with the listing beneath
    // it, which is also footprint data. A family that spawned WITHOUT
    // registering shows 0 here while its `active` count still moves — and
    // that disagreement is exactly what the delta column exists to catch.
    uint32_t claimed[PopFamily::COUNT] = {};
    uint32_t arrived[PopFamily::COUNT] = {};
    uint32_t claimed_total = 0;   // sum over the twelve families
    uint32_t arrived_total = 0;
    uint32_t occupancy = 0;       // every live slot, family or not
    for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
        const auto& fp = c->spawn_engine_state_.footprints_[i];
        if (!fp.active) continue;
        occupancy++;
        if (fp.family >= PopFamily::COUNT) continue;
        claimed[fp.family]++;
        claimed_total++;
        if (c->time_state_.seconds - fp.spawn_time < CENSUS_DUMP_INTERVAL) {
            arrived[fp.family]++;
            arrived_total++;
        }
    }

    std::cout << "[CENSUS t=" << std::fixed << std::setprecision(1) << std::setw(7)
        << c->time_state_.seconds << " trigger=" << trigger << "]\n"
        << "  fam    active  claimed   delta     new\n";

    uint32_t active_total = 0;            // all twelve — reports what EXISTS
    uint32_t active_grounded_total = 0;   // registrants only — feeds the delta
    for (uint32_t f = 0; f < PopFamily::COUNT; f++) {
        const uint32_t a = FAMILY_DISPATCH[f].active_count(c);
        active_total += a;
        std::cout << "  " << std::left << std::setw(7) << family_short_name(f) << std::right
            << std::setw(6) << a;
        if (FAMILY_DISPATCH[f].grounded) {
            active_grounded_total += a;
            std::cout << std::setw(9) << claimed[f];
            census_put_delta((int32_t)claimed[f] - (int32_t)a);
            // `new` is an unsigned count: no sign, and a plain 0 at rest.
            std::cout << std::setw(8) << arrived[f];
        }
        else {
            // claimed / delta / new are all footprint-derived — see
            // census_put_dash. This family does not participate.
            census_put_dash(9); census_put_dash(8); census_put_dash(8);
        }
        std::cout << "\n";
    }

    // TOTAL's columns answer different questions, deliberately. `active` sums
    // all twelve, because it reports what exists. `claimed` sums only
    // registrants, because only registrants can have footprints. The delta
    // must therefore be the sum of the PRINTED deltas — measured against
    // active_grounded_total — or TOTAL would report a permanent leak equal to
    // the live floater population, which is not a leak at all.
    std::cout << "  " << std::left << std::setw(7) << "TOTAL" << std::right
        << std::setw(6) << active_total
        << std::setw(9) << claimed_total;
    census_put_delta((int32_t)claimed_total - (int32_t)active_grounded_total);
    std::cout << std::setw(8) << arrived_total
        << "    footprints " << occupancy << "/" << MAX_FOOTPRINTS << "\n";

    // An unfamilied live footprint is unreachable through the three
    // register sites (all pass a real PopFamily), so this never fires
    // today. It is here because occupancy is what saturates, and the
    // per-family sum is what the delta column is built from: if those
    // two ever part company, every delta above is understated.
    if (occupancy != claimed_total) {
        std::cout << "  [CENSUS] WARNING: " << (occupancy - claimed_total)
            << " live footprint(s) carry no family — deltas above are understated\n";
    }

    // ─── SLOT OCCUPANCY (ARCH_2) ──────────────────────────────────────
    //
    // THE TABLE ABOVE HAS NO DENOMINATOR. It answers "how many stand" and
    // "does ground agree with body"; it cannot answer "could there have
    // been more", because nothing in it names a ceiling. This block adds
    // the ceiling and the reach, and it is the number the arch tier
    // weights have to be read against.
    //
    // WHY IT DECIDES: ArchConfig::SPAWN_CHANCE sets how many arches are
    // ATTEMPTED; ARCH_TIERS' three weights only choose WHICH tier each
    // attempt becomes. They are zero-sum in absolute counts. So a tier
    // reweight adds big arches only while the array has room — if live is
    // already sitting at capacity, the same reweight adds nothing and
    // merely reshuffles which bodies win a first-come race for the
    // sixteen slots. Same table, opposite conclusion, and the only thing
    // that tells them apart is printed here.
    //
    // ALL TWELVE ROWS, not the grounded ten. The dash convention above is
    // for FOOTPRINT-derived columns, and a family that claims no ground
    // genuinely has nothing to report there. These columns are
    // ARRAY-derived: every family has an instance array with a bound, so
    // every family has an honest answer, floaters included. The one dash
    // here is `portal`, below.
    //
    // hi-wtr is one past the highest live slot AT THIS SCAN — the
    // allocator's reach, not a session peak. Nothing is stored between
    // dumps. live == cap is a full array; hi-wtr == cap with live under it
    // is an array that has been full and now carries holes, which is what
    // a population cycling against its ceiling looks like when the census
    // samples it mid-breath. Both readings mean "the ceiling is binding".
    //
    // portal is arch-only and DASHED elsewhere — not zeroed. A palm has no
    // portal count to be zero; PORTAL_DENSITY applies to the DOORWAY arch
    // tier alone (mood.hpp), so `0` on a palm row would be a measurement
    // of something that does not exist. Same law as census_put_dash.
    // It is a SUBSET of the arch row's live, never a separate population:
    // portals are force-spawned into the same sixteen slots every rolled
    // arch competes for (force_spawn_portal_arch, grounded.hpp), which is
    // precisely why a portal-heavy tier skew costs big arches.
    uint32_t portal_arches = 0;
    for (uint32_t i = 0; i < Dim::MAX_ARCH_INSTANCES; i++) {
        const auto& a = c->entities_state_.arches[i];
        if (a.active && a.is_portal) portal_arches++;
    }

    std::cout << "  fam      live   hi-wtr     cap  portal\n";
    for (uint32_t f = 0; f < PopFamily::COUNT; f++) {
        const SlotCensus s = FAMILY_DISPATCH[f].slot_census(c);
        std::cout << "  " << std::left << std::setw(7) << family_short_name(f) << std::right
            << std::setw(6) << s.live
            << std::setw(9) << s.high_water
            << std::setw(8) << s.capacity;
        if (f == PopFamily::ARCH) std::cout << std::setw(8) << portal_arches;
        else                      census_put_dash(8);
        std::cout << "\n";
    }

    // ARRIVALS ONLY — the detail listing shows what appeared since the last
    // census, not everything that stands.
    //
    // A snapshot re-printed on a timer cannot tell a world at rest from a
    // world in motion: it emits the same ~140 lines either way. That is how a
    // static list came to read as a spawn sequence, how a frozen world looked
    // busy, and how the print's own cost read as a spawn burst. Filtering to
    // arrivals makes stillness print nothing, which is the point.
    //
    // THE WINDOW IS THE INTERVAL CONSTANT, not a delta against
    // lastCensusDump_. Boot and mood-transition never write that field (they
    // mirror the agent census, which does not either), so a delta would
    // measure the wrong span immediately after a transition; a fixed window
    // means the same thing at every trigger. Sub-frame slop is expected and
    // deliberately untreated — the periodic gate fires at 30.0-30.02s, so an
    // arrival may miss its window by a frame. No epsilon.
    struct CensusEntry { uint32_t fp_idx; float spawn_time; };
    CensusEntry entries[MAX_FOOTPRINTS];
    uint32_t n = 0;
    for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
        const auto& fp = c->spawn_engine_state_.footprints_[i];
        if (!fp.active || fp.family >= PopFamily::COUNT) continue;
        if (c->time_state_.seconds - fp.spawn_time >= CENSUS_DUMP_INTERVAL) continue;
        entries[n++] = { i, fp.spawn_time };
    }
    // Insertion sort by spawn_time ASCENDING, and by nothing else. The family
    // term is gone: family-major ordering is precisely what made a static
    // listing read as arrival order. Sorted by age alone, a burst shows up as
    // a run of near-equal ages — which is how the patch-row cadence was
    // diagnosed by hand in the first place.
    for (uint32_t i = 1; i < n; i++) {
        CensusEntry key = entries[i]; uint32_t j = i;
        while (j > 0 && entries[j - 1].spawn_time > key.spawn_time) {
            entries[j] = entries[j - 1]; j--;
        }
        entries[j] = key;
    }
    // CLAIMED GROUND, not entities. XZ, host patch and age live in the
    // registry, so this listing can only ever describe footprints — the
    // owning body may already be gone. Only the CONTENTS narrowed to
    // arrivals; the claim the label makes is unchanged.
    if (n > 0) {
        // Oldest-first, so the tail that gets dropped is the most recent. A
        // later census will not show them: a known, accepted loss for a
        // diagnostic that must not cost a frame.
        const uint32_t shown = (n < CENSUS_LISTING_MAX) ? n : CENSUS_LISTING_MAX;
        std::cout << "  claimed ground — arrivals (" << n << "):\n";
        for (uint32_t i = 0; i < shown; i++) {
            const auto& fp = c->spawn_engine_state_.footprints_[entries[i].fp_idx];
            std::cout << "  " << family_short_name(fp.family)
                << " t" << fp.tier
                << " (" << std::setw(8) << std::setprecision(1) << fp.x
                << "," << std::setw(8) << fp.z << ")"
                << " p(" << std::setw(3) << fp.patch_gx << "," << std::setw(3) << fp.patch_gz << ")"
                << " age=" << std::setprecision(1) << (c->time_state_.seconds - fp.spawn_time)
                << "\n";
        }
        if (n > shown) std::cout << "    ... +" << (n - shown) << " more\n";
    }
    std::cout << std::flush;
}

// ═══ SPAWN UTILITIES ═════════════════════════════════════════════
//
// The spawn lifecycle's smallest building blocks: the composition
// law, gate evaluation, jittered position, and the proximity
// affinity boost.

// ─── Spawn gate ──────────────────────────────────────────────────

// ═══ THE COMPOSITION LAW — definition (decl: spawn_services.hpp) ═══
// The ONE place the spawn-probability stack is authored. The
// float multiplication ORDER below is the bit-identity contract
// — do not reorder a multiply, do not move a
// clamp. Exact argument orders of min/max preserved per policy.
inline SpawnChanceResult compose_spawn_chance(MachineCtx* c, int32_t gx, int32_t gz,
    uint32_t family, float base_chance, const float* mood_mult,
    bool use_proximity, bool veto_on_zero_mood, SpawnClamp clamp) {
    float adj_mod = mood_mult[c->mood_state_.active];
    if (veto_on_zero_mood && adj_mod <= 0.0f) return { 0.0f, true };
    adj_mod *= GLOBAL_ENTITY_DENSITY;
    tile_apply_spawn_mult(c->tile_world_state_, gx, gz, family, adj_mod);  // F3: the S2 boundary face
    if (use_proximity) {
        float pcx = (gx + 0.5f) * Dim::PATCH_EXTENT;
        float pcz = (gz + 0.5f) * Dim::PATCH_EXTENT;
        adj_mod *= proximity_affinity_boost(c, pcx, pcz, family);
    }
    float chance = base_chance * adj_mod;
    switch (clamp) {
        case SpawnClamp::MIN1:    chance = std::min(chance, 1.0f); break;
        case SpawnClamp::RANGE01: chance = std::max(0.0f, std::min(1.0f, chance)); break;
        case SpawnClamp::NONE:    break;
    }
    return { chance, false };
}

// Evaluate the spawn gate: seed + flat probability check.
inline SpawnPreamble evaluate_spawn_gate(MachineCtx* c, int32_t gx, int32_t gz,
    uint32_t spawn_roll_prop,
    float chance) {
    SpawnPreamble result{};
    // (per-gate archetype lookup CUT: computed for
    //  every generic gate, read by nobody; the sole archetype consumer
    //  (gallery) calls tile_archetype itself in its bespoke funnel.)
    result.seed = tile_seed(c->world_state_.active_seed, gx, gz);
    result.passed = cpu_hash_f(result.seed, spawn_roll_prop) < chance;
    return result;
}

// Jittered world position within a patch.
inline void jittered_position(uint32_t seed, int32_t gx, int32_t gz,
    uint32_t prop_x, uint32_t prop_z, float jitter,
    float& out_x, float& out_z) {
    out_x = (gx + 0.5f) * Dim::PATCH_EXTENT + (cpu_hash_f(seed, prop_x) - 0.5f) * Dim::PATCH_EXTENT * jitter;
    out_z = (gz + 0.5f) * Dim::PATCH_EXTENT + (cpu_hash_f(seed, prop_z) - 0.5f) * Dim::PATCH_EXTENT * jitter;
}

inline float proximity_affinity_boost(MachineCtx* c, float cx, float cz, uint32_t family) {
    if (!proximity_row_active(family)) return 1.0f;
    float radius = PROXIMITY_RADIUS[family];
    if (radius <= 0.0f) return 1.0f;
    float r2 = radius * radius;
    float weighted = 0.0f;
    uint32_t count = 0;
    for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
        if (!c->spawn_engine_state_.footprints_[i].active) continue;
        if (c->spawn_engine_state_.footprints_[i].family >= PopFamily::COUNT) continue;
        float aff = PROXIMITY_AFFINITY[family][c->spawn_engine_state_.footprints_[i].family];
        if (aff <= 0.0f) continue;
        float dx = cx - c->spawn_engine_state_.footprints_[i].x;
        float dz = cz - c->spawn_engine_state_.footprints_[i].z;
        if (dx * dx + dz * dz < r2) {
            weighted += aff;
            count++;
        }
    }
    if (count < PROXIMITY_THRESHOLD[family]) return 1.0f;
    return std::min(1.0f + weighted, PROXIMITY_MAX_BOOST[family]);
}

// ─── Select / Place / Commit dispatch loops ─────────────────────

inline void select_entities_for_patch(MachineCtx* c, int32_t gx, int32_t gz) {
    // PLACEMENT_ORDER (roster.hpp, F-6) is the priority, not the loop counter:
    // push order IS placement order, so whoever comes first claims contested
    // ground first. Identity today, so this is bit-identical to the old
    // `f = 0..COUNT`.
    for (uint32_t i = 0; i < PopFamily::COUNT; i++) {
        const uint32_t f = PLACEMENT_ORDER[i];
        if (!ROSTER.family_enabled(f)) continue;  // ROSTER-GATE family (b) — disabled family never selected -> never placed/committed/meshed/drawn. Budgeted stream path, not the per-frame hot path.
        EntityQueueEntry e{};
        e.family = f;
        e.gx = gx; e.gz = gz;
        if (!FAMILY_DISPATCH[f].try_select(c, gx, gz, e)) continue;
        auto& st = c->spawn_engine_state_;
        if (st.entityQueueCount_ >= SPAWN_QUEUE_MAX) {
            // Unreachable if the bound above holds. LOUD rather than silent:
            // a dropped selection is an entity that never existed, with no
            // other symptom anywhere.
            std::cerr << "[SPAWN] entityQueue_ OVERFLOW at " << SPAWN_QUEUE_MAX
                      << " — dropping " << family_short_name(f)
                      << " and the rest of this patch. The proven bound"
                         " (SPAWN_BUDGET_PER_FRAME x PopFamily::COUNT) is wrong.\n";
            break;
        }
        st.entityQueue_[st.entityQueueCount_++] = e;
    }
}

// ─── Place: spatial negotiation (no GPU writes) ──────────────

inline void place_entity_queue(MachineCtx* c) {
    auto& st = c->spawn_engine_state_;
    for (uint32_t i = 0; i < st.entityQueueCount_; i++) {
        PlacementEntry pe{};
        if (!FAMILY_DISPATCH[st.entityQueue_[i].family].try_place(c, st.entityQueue_[i], pe))
            continue;
        if (st.placementCount_ >= SPAWN_QUEUE_MAX) {
            // Structurally unreachable — at most one push per queue entry, and
            // the queue shares this ceiling. Guarded anyway, and loudly.
            std::cerr << "[SPAWN] placementResults_ OVERFLOW at " << SPAWN_QUEUE_MAX
                      << " — dropping a placed entity\n";
            break;
        }
        st.placementResults_[st.placementCount_++] = pe;
    }
    st.entityQueueCount_ = 0;
}

// ─── Commit: GPU writes from placement results ──────────────

inline void commit_entity_queue(MachineCtx* c, wgpu::Queue& queue) {
    auto& st = c->spawn_engine_state_;
    for (uint32_t i = 0; i < st.placementCount_; i++)
        FAMILY_DISPATCH[st.placementResults_[i].family].try_commit(c, st.placementResults_[i], queue);
    st.placementCount_ = 0;
}

} // namespace the_board
} // namespace t7
