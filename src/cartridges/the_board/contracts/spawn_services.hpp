#pragma once
#include <cstdint>
#include <cstddef>                                            // size_t (the rescale template's array extent)
#include <cmath>                                              // std::atan2 (arch_rotation_from_facing)
#include "cartridges/the_board/contracts/roster.hpp"          // PopFamily (sizes MIN_SEPARATION)
#include "cartridges/the_board/contracts/wgpu_fwd.hpp"   // wgpu handle fwds (lockstep insurance)
#include "cartridges/the_board/contracts/entity_types.hpp"    // MachineCtx + EntityInstance + traits/adapter + TierProfile

// ─── spawn_services.hpp (CONTRACT: the machine's decl tier) ───────
// The machine natives (spawn_engine, entity_pipeline) each held a
// two-tier shape — a DECL tier consumed BEFORE entities (the preamble
// + generic + rescale templates, the service decls, the arch
// vocabulary, MIN_SEPARATION) and a BODY tier that needs entities
// COMPLETE. The decl tier graduates HERE so the merged machine
// headers ride the cohort tail whole. The bodies bind by the TU's own
// law: an inline function or template declared before its callers may
// be DEFINED later in the same TU (templates instantiate at
// end-of-TU) — named at the contract.
//
// OWNERS: every DEFINITION lives in machine/spawn_engine.hpp or
// machine/entity_pipeline.hpp (merged, cohort tail). This file owns
// only the boundary — entity_types.hpp's sibling.
// ─────────────────────────────────────────────────────────────────

namespace t7 {
namespace the_board {

// fwd — entities vocabulary named in a declaration only (graduated
// here with the decl that names it); the
// definitions, at the cohort tail, see the complete type.
struct ActiveColumn;
// fwd — state.hpp's GPU mesh-param records (return values below; a
// non-defining declaration tolerates the incomplete type).
struct GPUArchMeshParams;
struct GPUColumnMeshParams;

// ── Shared spawn helper vocabulary ─────────────────────────────────

struct SpawnGatePreambleResult {
    uint32_t seed;          // from evaluate_spawn_gate
    uint32_t slot;          // reserved slot index
    uint32_t theme_idx;     // themes_state_.temporal_flavor at evaluation time
    bool ok;                // false = early exit (idempotency, gate, no slot)
};

struct PositionResult {
    float cx, cz, rotation;
    int32_t host_gx, host_gz;
    bool ok;
};

// ── Spawn gate vocabulary ──────────────────────────────────────────

struct SpawnPreamble {
    uint32_t seed;          // tile_seed(world_state_.active_seed, gx, gz)
    bool passed;            // false if spawn gate failed
};

// ─── Global Entity Density ──────────────────────────────────────
// (Graduated with the decl tier: gol's and gallery's bespoke spawn
// funnels read it before the machine tail.)
inline constexpr float GLOBAL_ENTITY_DENSITY = 1.0f;

// ─── LODESTAR — the guaranteed-door lattice (LODESTAR_0) ─────────
// One patch per CELL×CELL cell of the open field carries a LOADED
// arch roll: spawn forced, tier pinned DOORWAY — a portal, since
// portal_density is 1.0. The random rolls keep the texture; this
// lattice removes their veto over ABSENCE. CELL is THE dial:
// 8 patches = 400 wu cells → nearest-guaranteed-door ceiling ≈ one
// cell diagonal (~400 wu). Smaller cells buy a lower ceiling with
// more doors and press the 16-slot arch pool — priced in OPEN.md
// (LODESTAR_0/P) before turning.
inline constexpr uint32_t LODESTAR_CELL      = 8u;    // patches per cell side
inline constexpr uint32_t LODESTAR_SEED_BAND = 190u;  // lattice band (themes 170, GoL zones 250)
inline constexpr uint32_t LODESTAR_TRIES     = 5u;    // designated placement candidates; ordinary rolls keep 1
bool arch_lodestar_designated(uint32_t world_seed, int32_t gx, int32_t gz);

// ── Minimum Separation Matrix ─────────────────────────────────────
//
// WHAT: the extra edge-to-edge gap a candidate placement must keep from
//   every registered footprint, per ordered family pair.
// AXES: MIN_SEPARATION[placing][existing] — row = family being PLACED,
//   column = the EXISTING footprint's family. ASYMMETRIC: [Antenna][Arch]
//   = 60 (an antenna being placed keeps 60 wu from a standing arch) but
//   [Arch][Antenna] = 8 (an arch being placed tolerates 8 wu to a
//   standing antenna).
// UNITS: world-units, ADDITIVE — the consumer sums the two footprint
//   radii first, then adds this gap on top (and may shrink it by
//   PROXIMITY_GAP_REDUCTION × affinity for clustering families).
// ORDER: rows and columns both follow PopFamily order (PYRAMID=0 …
//   GALLERY=11), PINNED by the F-1 static_assert at roster.hpp —
//   renumbering a family is a compile error, not a silent re-column.
// CONSUMER: check_position(), machine/spawn_engine.hpp (sole reader).
// SENTINEL: 0.0 = no gap constraint for that pair (only the radii sum
//   applies; the consumer skips the gap term entirely).
// Placement determinant — frozen biography (§12): changing a number
// changes which candidate positions survive, i.e. changes worlds.
//
// NON-PARTICIPANTS — SPHERE (7) and CUBE (9), ruling 21/23. Both are
//   unreachable in BOTH directions and no number in either line can change
//   anything:
//     · their ROWS never execute — negotiate_position skips check_position
//       entirely for a non-`grounded` family, so those families never read
//       the table at all;
//     · their COLUMNS never match — they register no footprint, so no
//       sphere or cube entry will ever be found by the scan.
//   The rows and columns REMAIN because F-1 pins this table at 12x12 in
//   PopFamily order and it is one of eight positional tables — deleting a
//   line would re-column all of them. They are held as structural zeros.
//   Their diagonals were 20 (sph) and 15 (cube); ruling 22 retired them with
//   the footprint. If floaters ever claim ground again, the values are in git
//   and this note is what tells you they were deliberate.
inline constexpr float MIN_SEPARATION[PopFamily::COUNT][PopFamily::COUNT] = {
    //                near:  Pyr    Arch   Col    Ant    Palm   Cact   Blad   Sph    Ribn   Cube   GoL    Gall
    /* placing Pyramid  */ { 65.0f, 60.0f,  5.0f, 55.0f,  5.0f,  5.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f },
    /* placing Arch     */ { 60.0f, 20.0f, 10.0f, 60.0f,  8.0f,  5.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f },
    /* placing Column   */ {  5.0f, 10.0f,  8.0f,  6.0f,  5.0f,  5.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f },
    /* placing Antenna  */ { 55.0f, 60.0f,  6.0f, 12.0f,  5.0f,  5.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f },
    /* placing Palm     */ {  5.0f,  8.0f,  5.0f,  5.0f,  8.0f,  5.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f },
    /* placing Cactus   */ {  5.0f,  5.0f,  5.0f,  5.0f,  5.0f,  8.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f },
    /* placing Blade    */ {  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f },
    /* placing Sphere   */ {  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f },   // ruling 22: self-sep retired with the footprint (was 20)
    /* placing Ribbon   */ {  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 40.0f,  0.0f,  0.0f,  0.0f },
    /* placing Cube     */ {  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f },   // ruling 22: self-sep retired with the footprint (was 15)
    /* placing GoL      */ { 10.0f, 10.0f,  5.0f,  5.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 60.0f,  0.0f },
    /* placing Gallery  */ { 10.0f, 10.0f,  5.0f,  5.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 10.0f, 30.0f },
};

// ── Arch vocabulary (graduated with the decl tier: read by entities'
//    recipes and direction/mood.hpp's portal/doorway geometry) ──

// ── AN ARCH'S ROTATION IS ITS SPAN (ATRIUM_6) ────────────────────
// amg_gen_shell (world.wgsl) sweeps the catenary along (cos r, sin r) and
// the depth along (-sin r, cos r); arch_rib_point walks foot to foot along
// the same (cos r, sin r); the crossing ellipse weights that axis by
// inv_span_sq and its perpendicular by inv_depth_sq. Three sites, one
// reading: (cos r, sin r) is the chord between the feet, and the OPENING's
// normal is (-sin r, cos r).
//
// So a door meant to FACE a point takes the bearing of that point less a
// quarter turn, and every CPU site that places a door to face something
// derives it here rather than spelling a sign it can get backwards. Pass
// the vector FROM the door TO the thing it should open onto; the length
// does not matter.
//
// THE WALL CANDIDATE TABLES ARE NOT SUCH SITES. They carry their own
// authored rotations and their own standing reading — a doorway's span
// perpendicular to its wall — which Jean has gated as it stands. They do
// not derive here, and this line is why they do not have to.
inline float arch_rotation_from_facing(float nx, float nz) {
    return std::atan2(-nx, nz);
}

struct ArchIdx {
    static constexpr uint32_t SPAN         = 0;  // full span (halved in compute_solid_half)
    static constexpr uint32_t RISE         = 1;
    static constexpr uint32_t DEPTH        = 2;
    static constexpr uint32_t THICKNESS    = 3;
    static constexpr uint32_t PIER_HEIGHT  = 4;
    static constexpr uint32_t PIER_PADDING = 5;
    static constexpr uint32_t EDGE_BLEND   = 6;
    static constexpr uint32_t COUNT        = 7;
};

// params[] order MUST match ARCH_PARAM_DEFS:
//   [0]SPAN [1]RISE [2]DEPTH [3]THICKNESS [4]PIER_HEIGHT [5]PIER_PADDING [6]EDGE_BLEND
//
// Note: name is ArchTierRow, not ArchTier — the enum class ArchTier
// (DOORWAY/STANDARD/MONUMENTAL) already occupies that name in grounded.hpp.
struct ArchTierRow {
    TierProfile profile;
    float       color_override;
    // MOSAIC_2: is this KIND ceramic? 1.0 = every body of this tier;
    // 0.0 = none. Orthogonal to color_override: what a body's FALLBACK
    // color is, and whether it is ceramic, are two facts.
    // MOSAIC_3 — ALL ROWS 1.0. The contrast ruling (plain stone beside
    // the ceramic) is retired: every arch is ceramic. The portals are
    // the one plain population left, and they are plain for a different
    // reason — a functional marker wears its destination's color. That
    // exception lives at the decision in arch_write_active, NOT here.
    // Do not "restore" DOORWAY to 0.0; that duplicates the guard.
    float       mosaic_chance;
    float       burial;
    uint32_t    segs_u;
    uint32_t    segs_v;
};

// ── Arch tier table ────────────────────────────────────────────────
// WHAT: per-tier arch recipe — tier-selection weight + the Gaussian
//   {mean, sigma} of each geometry parameter + mesh tessellation.
// AXES: row = arch tier, in enum class ArchTier order
//   (0 DOORWAY / 1 STANDARD / 2 MONUMENTAL); inner params[] column =
//   ArchIdx order (the MUST-match note above).
// UNITS: weight = relative tier-selection weight (normalized over rows
//   by select_tier); {μ,σ} = world-units for SPAN/RISE/DEPTH/THICKNESS/
//   PIER_HEIGHT, dimensionless ratio for PIER_PADDING/EDGE_BLEND;
//   burial = fraction of pier_height sunk below ground; segs_u/v =
//   mesh tessellation counts.
// CONSUMERS: entity_pipeline.hpp (arch_tier_profile, burial, segs into
//   mesh params); grounded.hpp force-spawn portal (DOORWAY row);
//   mood.hpp portal/doorway geometry (DOORWAY row).
// SENTINELS: color_var 0 = fall back to ColorPartDef.variance;
//   color_override 0 = no override.
// Biography determinant — frozen biography (§12): weight feeds
// select_tier(gate.seed), {μ,σ} feed cpu_sample_gaussian(gate.seed);
// changing a number changes every arch ever born.
inline constexpr ArchTierRow ARCH_TIERS[] = {
    //   {  weight, color_var, { {μ,σ}: SPAN      RISE        DEPTH       THICKNESS     PIER_HEIGHT  PIER_PAD     EDGE_BLEND } },  col_ovr  mosaic  burial  segs_u  segs_v
    // DOORWAY's weight IS the open world's portal density.
    // WORLD_DRAW_LIVE.portal_density is 1.0 (mood_constants.hpp, the
    // world-draw bank): every doorway arch becomes a portal, so this
    // number and ArchConfig::SPAWN_CHANCE are the only two dials deciding
    // how many ways out the field has.
    // ZERO-SUM (ARCH_2): at a fixed SPAWN_CHANCE these three weights trade
    // against each other in ABSOLUTE counts — the mix is theirs, the count
    // is the chance's. Every portal bought by skewing is a big arch sold.
    // STANDARD and MONUMENTAL are the outdoor silhouette and are held by
    // ruling: buy portals with the chance, never out of them.
    /* DOORWAY    */ {
        { 0.60f, 0.0f, { {12.0f, 2.4f}, {12.0f, 2.4f}, {4.5f, 0.9f}, {1.2f, 0.18f}, {1.5f, 0.9f}, {0.9f, 0.3f}, {0.9f, 0.15f} }},
        0.85f, 1.0f, 0.20f, 16, 4
    },
    /* STANDARD   */ {
        { 0.20f, 0.0f, { {50.0f, 15.0f}, {42.0f, 7.0f}, {5.6f, 1.1f}, {1.4f, 0.21f}, {5.6f, 2.1f}, {0.7f, 0.3f}, {0.7f, 0.14f} }},
        0.85f, 1.0f, 0.20f, 32, 8
    },
    /* MONUMENTAL */ {
        { 0.20f, 0.0f, { {60.0f, 10.0f}, {80.0f, 12.0f}, {10.0f, 2.0f}, {2.5f, 0.30f}, {8.0f, 2.5f}, {1.0f, 0.3f}, {0.8f, 0.15f} }},
        0.85f, 1.0f, 0.20f, 48, 12
    },
};

// ═══ SPAWN SERVICES — DECLARATIONS (spawn_engine) ═════════════════
//
// DEFINED in machine/spawn_engine.hpp (merged, cohort tail): the
// engine reaches the machine face for the root organs (world/time/
// mood/themes/tile state, entities_state_, the GPU wire) and routes
// the twelve families through FAMILY_DISPATCH.

// ═══ THE COMPOSITION LAW — the collapse ═════════════════════
// ONE stack, authored once, called by all three spawn authors (the
// generic preamble, GoL, Gallery). Per-consumer FACTS travel as DATA:
// base-chance authority (scalar, or archetype-indexed — resolved by
// the caller before the call), clamp policy, proximity on/off, and
// the mood-zero veto style. The float multiplication ORDER inside the
// definition is the bit-identity contract:
// mood → GLOBAL_ENTITY_DENSITY → tile (spatial_density) →
// [proximity] → base × adj → clamp. Seed domains
// and the rolls themselves stay with the consumers.

enum class SpawnClamp : uint32_t {
    MIN1,     // min(chance, 1.0)        — the generic preamble
    RANGE01,  // max(0, min(1, chance))  — GoL
    NONE,     // raw base × adj          — Gallery. Sub-ruling: the
              //   absent clamp is CARRIED AS DATA (behavior-identical);
              //   ruling a clamp IN is a separate taste gate.
};

struct SpawnChanceResult {
    float chance;   // the composed probability (post clamp policy)
    bool  vetoed;   // true only under veto_on_zero_mood with mood ≤ 0
};

SpawnChanceResult compose_spawn_chance(MachineCtx* c, int32_t gx, int32_t gz,
    uint32_t family, float base_chance, const float* mood_mult,
    bool use_proximity, bool veto_on_zero_mood, SpawnClamp clamp);

SpawnPreamble evaluate_spawn_gate(MachineCtx* c, int32_t gx, int32_t gz,
    uint32_t spawn_roll_prop,
    float chance);
void jittered_position(uint32_t seed, int32_t gx, int32_t gz,
    uint32_t prop_x, uint32_t prop_z, float jitter,
    float& out_x, float& out_z);
float proximity_affinity_boost(MachineCtx* c, float cx, float cz, uint32_t family);
bool check_position(MachineCtx* c, float px, float pz, float placing_radius,
    uint32_t placing_family);
// LODESTAR_0 — the designation predicate (constants above; defined in
// spawn_engine.hpp beside the gate it loads).
uint32_t register_footprint(MachineCtx* c, float x, float z, float radius,
    int32_t gx, int32_t gz, uint32_t family, uint32_t slot,
    uint32_t tier = 0);
// Release by owner identity (family, slot). No index is stored anywhere — the
// registry is scanned. See the definition for why that is the design.
void unregister_footprint_for(MachineCtx* c, uint32_t family, uint32_t slot);
// The indoor bounds law (INDOOR_TREATMENT.bounds — contracts/
// indoor_module.hpp): MARGIN clamps footprint_r inside the wall
// margin; FULL clamps containment_r (the family's whole extent);
// FREE skips. Returns false when a FULL legal box collapses — the
// caller skips the spawn (the loud line prints in the law).
bool indoor_bounds_clamp(MachineCtx* c, uint32_t family,
    float footprint_r, float containment_r, float& cx, float& cz);
// `grounded` (ruling 21) decides whether footprint_r means anything: a family
// registers iff its own extent touches the ground plane. FALSE skips BOTH the
// check and the registration — the floater is neither blocked by ground nor a
// claimant of it. It does NOT skip indoor_bounds_clamp (containment is a
// different concept) nor the host-patch derivation (eviction bookkeeping,
// which every family needs). Deliberately NOT defaulted: with two call sites,
// an explicit value at each beats a default that would silently register a
// future floater family whose author forgot the flag.
PositionResult negotiate_position(MachineCtx* c,
    uint32_t seed, int32_t trigger_gx, int32_t trigger_gz,
    uint32_t pos_x_prop, uint32_t pos_z_prop, float jitter,
    uint32_t rotation_seed_prop,
    bool grounded,
    float footprint_r, float containment_r, uint32_t family, uint32_t slot,
    uint32_t tier = 0);
GPUArchMeshParams build_arch_mesh_params(MachineCtx* c, uint32_t slot);
GPUColumnMeshParams build_column_mesh_params_from(const ActiveColumn& c);
GPUColumnMeshParams build_column_mesh_params(MachineCtx* c, uint32_t slot);
uint32_t update_entity_draw_visibility(MachineCtx* c, wgpu::Queue& queue);
const char* family_short_name(uint32_t family);
void dump_entity_census(MachineCtx* c, const char* trigger);
void select_entities_for_patch(MachineCtx* c, int32_t gx, int32_t gz);
void place_entity_queue(MachineCtx* c);
void commit_entity_queue(MachineCtx* c, wgpu::Queue& queue);

// The preamble template (SEAM[spawn_engine:P11]) — DECLARATION only;
// the definition rides machine/spawn_engine.hpp (end-of-TU
// instantiation binds every pre-tail caller).
//
// SPAWN_DECL_0 — THE TENTH PARAMETER IS PART OF THE SIGNATURE. LODESTAR_0
// gave the definition `force_spawn`; this declaration kept nine, and a
// template declared with nine parameters is a DIFFERENT template from one
// defined with ten. Every pre-tail caller bound to the nine and wasm-ld
// found no definition — a link that failed after file_packager had already
// refreshed the package, which is how a fresh .data came to ship beside a
// stale .js (PAIR_0). The default lives HERE, on the declaration the
// callers see, and not on the definition: once per scope, where it is
// visible.
template<typename C, typename ActiveT>
SpawnGatePreambleResult run_spawn_preamble(C* c,
    int32_t gx, int32_t gz,
    ActiveT* active_arr, uint32_t max_instances,
    uint32_t spawn_roll_prop, float spawn_chance,
    const float* mood_mult,
    uint32_t family,
    bool force_spawn = false);

// The generic gate — DECLARATION only; defined beside run_spawn_preamble at
// the cohort tail. Same binding law: the nine family run_gates that call it
// (grounded.hpp, spheres.hpp, cube_behaviors.hpp all precede spawn_engine.hpp
// in the cohort) bind by end-of-TU instantiation.
//
// Every per-family constant travels on the traits row; only the ACTIVE ARRAY
// is a parameter, because its type varies and ActiveT deduces from it.
template<typename ActiveT>
SpawnGateOutput gate_from_traits(MachineCtx* c, int32_t gx, int32_t gz,
    const EntityFamilyTraits& t, ActiveT* active_arr);

// ═══ PIPELINE VERBS — DECLARATIONS (entity_pipeline) ══════════════
//
// DEFINED in machine/entity_pipeline.hpp (merged, cohort tail): the
// verbs reach the machine face for c->mood_state_ / c->world_state_
// and route through the spawn services; the family adapters write
// c->entities_state_ and the GPU wire.

bool generic_select(MachineCtx* c,
    const EntityFamilyTraits& traits,
    const EntityFamilyAdapter& adapter,
    int32_t gx, int32_t gz,
    EntityInstance& inst);
bool generic_place(MachineCtx* c,
    const EntityFamilyTraits& traits,
    EntityInstance& inst);
void generic_commit(MachineCtx* c,
    const EntityFamilyTraits& traits,
    const EntityFamilyAdapter& adapter,
    const EntityInstance& inst,
    wgpu::Queue& queue);

} // namespace the_board
} // namespace t7
