#pragma once
#include <cstdint>
#include "cartridges/the_board/realization/state.hpp"                    // wgpu, GPUSpotLightArray, MAX_SPOT_LIGHTS
#include "cartridges/the_board/contracts/mood_constants.hpp"   // MOOD_COUNT, the Mood IDs, PortalDestination
#include "cartridges/the_board/contracts/spine_state.hpp"      // TransitionPhase (the transition channel — the driver door's param)
#include "cartridges/the_board/contracts/wgpu_fwd.hpp"   // wgpu handle fwds (lockstep insurance)
#include <algorithm>   // std::max, std::min, std::clamp   // (impl, merged)
#include <cmath>       // std::sqrt, std::sin, std::cos, std::cosh, std::floor, std::abs   // (impl, merged)
#include <iostream>    // mood / lighting / shell / portal logs   // (impl, merged)
#include <vector>      // shell mesh staging   // (impl, merged)

// ─── mood.hpp (MERGED: deps + portal/palette vocabulary + impl) ────
// History: audit/LADDER.md
//
// Atmosphere, indoor lighting, shell geometry, portals.
//
// Mood is VOCABULARY + APPLIERS + SIX DOORS. This header owns the
// vocabulary — CeilingType, MoodProfile, MOOD_TABLE, the portal color
// table, the indoor wall palettes (the indoor treatment — sizes,
// bounds, dials — graduated to contracts/indoor_module.hpp) — and
// the DECLARATIONS of the six doors
// (apply_mood, request_mood_transition, force_spawn_back_portal,
// upload_lights, upload_portal_array, mood_name) plus the appliers and
// derivers. The Mood IDs are file-scope vocabulary
// (mood_constants.hpp), consumed here. MOOD OWNS NO INSTANCE: struct
// MoodState's TYPE lives in contracts/spine_state.hpp; the instance
// mood_state and the transition machine
// (transitionPhase / pendingDestination and kin) are SPINE-OWNED
// orchestration (SEAM[spine:transitions] at the machine's banner;
// constitution §2, K4 as amended). The force-spawn
// mutation of the arch belongs to the arch's owner — mood's
// force_spawn_* internals COMPUTE VALUES and call entities'
// force_spawn_portal_arch.
// MERGED at the cohort tail:
// MoodState / CeilingType / MoodProfile / MOOD_TABLE + the request
// door decl live in contracts/spine_state.hpp (the spine's organ
// contract — the demo sentence includes mood_constants, so the
// DEMO-reading MoodState rides the spine tier; tile_world/ribbon/the
// config tables read them early);
// this file keeps the portal + palette vocabulary, MoodDeps, the
// decls, and every definition. COHORT: after ribbon/gallery/input
// (the fan's door owners + ORB_MOOD_TABLE), before the
// machine natives (they call pick_portal_mood / derive_finite_radius
// and read PORTAL_COLORS).
//
// The impl additionally reaches the spine-resident state
// (mood_state / transitionPhase / pendingDestination /
// backPortalPosition_ / cpuSpotLights_ / cpuPortalArray_ / sun + clear
// colors / world_state_ and the feature-gate flags), the converted
// modules' surfaces (entities' force_spawn_portal_arch, gallery's
// wall paintings, orbs' configure,
// render_passes' compute_spot_light_vp), TransitionPhase
// (contracts/spine_state.hpp), ARCH_TIERS / ArchIdx
// (entity_pipeline.hpp), solve_catenary_a (seed_utils.hpp), and
// Dim::PATCH_EXTENT (patch_system.hpp).
//
// SEAM[mood:K1] apply_mood is the single canonical mood entry point.
//   All mood transitions — keyboard, portal crossings, world
//   teardown — funnel through here. Other code paths set
//   pendingDestination / transitionPhase; the actual mood activation
//   happens here. The orchestrator owns only ordering and the
//   activate-mood bookkeeping; the substantive work splits across
//   three named sub-functions (apply_mood_lighting, _spot_lights,
//   _indoor_shell).
// apply_mood orchestrates the named applier helpers; the appliers
//   match the natural seams in the flow, and their per-mood-transition
//   call order is load-bearing.
// request_mood_transition is the single canonical transition entry
//   point. Bails if a transition is already in flight. Lives in the
//   mood module rather than input because portal crossings and other
//   code paths can also drive mood transitions. One door, many keys.
//   DEPS-FORM: the driver world holds no MachineCtx
//   — the door takes the transition channel explicitly (the deps-form
//   precedent class, clear_spheres).
// ─────────────────────────────────────────────────────────────────

namespace t7 {
namespace the_board {

struct WorldState;   // patch_system.hpp — the doors read seeds/bounds (reference members/params; fwd suffices)
// fwd — the deps face's true reaches and the fan's TARGET organs
// (reference members/params; complete types arrive with their owners
// in the cohort). GPUState + the CPU light/portal arrays come complete
// from state.hpp (included above).
class Renderer;
struct GoLState;   struct EntitiesState; struct MachineCtx;
struct OrbsState;   struct OrbsDeps;
struct GalleryState; struct GalleryDeps;
struct PawnState;

// ═══ MOOD STATE + PROFILE VOCABULARY — GRADUATED ═════════════════
// MoodState / CeilingType / MoodProfile / MOOD_TABLE live in
// contracts/spine_state.hpp: the early consumers read the
// contract; the instance stays at the root.

// ═══ PORTAL VOCABULARY ═══════════════════════════════════════════

// ── Portal detection ──
inline constexpr float PORTAL_DENSITY = 1.00f;  // fraction of Doorway arches that become portals

// Portal color by mood (indexed by destination.mood)
inline constexpr float PORTAL_COLORS[MOOD_COUNT][3] = {
    { 0.72f, 0.45f, 0.85f },  // mood 0  open_sunset     — lilac
    { 0.95f, 0.55f, 0.15f },  // mood 1  indoor_flat     — orange
    { 0.95f, 0.80f, 0.20f },  // mood 2  indoor_vault    — yellow
    { 0.85f, 0.20f, 0.15f },  // mood 3  finite_outdoor  — red
};
inline constexpr float PORTAL_COLOR_BACK[3] = { 0.35f, 0.55f, 0.90f };  // back-portal — blue

// ═══ INDOOR WALL PALETTE ═════════════════════════════════════════
//
// SEAM[mood:tuning-data] the palettes are consumed only by
//   apply_mood_indoor_shell; the indoor lighting SCHEMES (the other
//   half of this seam) are consumed only by derive_indoor_lights and
//   stay impl-side (the MODULE IMPLEMENTATION below) — module-internal authoring tables.
struct IndoorPalette {
    const char* name;
    float wall_color[3];
    float ceiling_color[3];
};
inline constexpr IndoorPalette INDOOR_PALETTES[] = {
    { "warm taupe",     { 0.72f, 0.65f, 0.55f }, { 0.65f, 0.58f, 0.50f } },
    { "slate blue",     { 0.58f, 0.62f, 0.68f }, { 0.50f, 0.55f, 0.62f } },
    { "terracotta",     { 0.65f, 0.50f, 0.42f }, { 0.55f, 0.42f, 0.36f } },
    { "sage",           { 0.62f, 0.68f, 0.58f }, { 0.55f, 0.60f, 0.52f } },
    { "pale linen",     { 0.85f, 0.80f, 0.72f }, { 0.78f, 0.73f, 0.65f } },
    { "deep mocha",     { 0.45f, 0.40f, 0.35f }, { 0.38f, 0.34f, 0.30f } },
    { "dusty rose",     { 0.70f, 0.58f, 0.58f }, { 0.62f, 0.50f, 0.50f } },
    { "warm charcoal",  { 0.40f, 0.38f, 0.36f }, { 0.32f, 0.30f, 0.28f } },
};
inline constexpr uint32_t INDOOR_PALETTE_COUNT =
    sizeof(INDOOR_PALETTES) / sizeof(INDOOR_PALETTES[0]);

// ═══ INDOOR ENTITY PLACEMENT → THE INDOOR MODULE ═════════════════
//
// The whole indoor treatment — the size/bounds policy table and
// the three dials (INDOOR_HEIGHT_CAP_FRACTION, RIBBON_INDOOR_SCALE,
// INDOOR_ENTITY_WALL_MARGIN) — lives in contracts/indoor_module.hpp:
// mood's insert on the spawn chain, graduated to the contracts tier
// because its consumers (grounded/spheres/cube_behaviors/ribbon)
// precede this header in the cohort. One table, three dials — the
// tuning session edits rows and dials, never wiring.

// ═══ THE DEPS FACE ═══════════════════════════════════════════════
//
// Mood's own organs plus its true reaches — the atmosphere author's
// face: the mood organ, the sun/clear channel, the CPU light + portal
// staging arrays, the back-portal anchor, the realization pokes
// (GPUState uploads, the frustum-cull flag), the gol mood gate (the
// THE FLAG CHANNEL [mood -> gol]), and a const view of entities (the
// portal-array upload reads arch positions). The fan's TARGET organs
// are deliberately NOT members (the B ruling, input's precedent):
// orbs/gallery/pawn pairs + the machine face ride apply_mood's
// parameters — the spine addresses the fan's bodies at the call site,
// through the owner command doors.
struct MoodDeps {
    MoodState&           mood_state_;
    const WorldState&    world_state_;
    GPUState&            gpuState_;
    Renderer&            renderer_;          // set_frustum_cull_active (per-mood realization poke)
    GoLState&            gol_state_;         // mood_allowed — the flag channel [mood -> gol]
    const EntitiesState& entities_state_;    // upload_portal_array reads arch positions
    float (&sunDirection_)[3];
    float (&sunColor_)[3];
    float (&clearColor_)[3];
    GPUSpotLightArray&   cpuSpotLights_;
    GPUPortalArray&      cpuPortalArray_;
    float (&backPortalPosition_)[2];
};

// ═══ MODULE FUNCTIONS — DECLARATIONS ═════════════════════════════

// Mood lifecycle (doors). The fan's targets ride apply_mood's tail
// parameters — organ-named, addressed by the spine at the call site.
void apply_mood(MoodDeps* c, uint32_t mood, wgpu::Queue& queue,
    MachineCtx& machine_ctx,
    OrbsState& orbs_state, OrbsDeps& orbs_deps,
    GalleryState& gallery_state, GalleryDeps& gallery_deps,
    PawnState& pawn_state);
// request_mood_transition decl GRADUATED to spine_state.hpp (input
// calls it before this file in the cohort); the definition is below.
// Appliers (apply_mood's three named sub-functions; each takes only
// the targets its own fan drives)
void apply_mood_lighting(MoodDeps* c, const MoodProfile& m, wgpu::Queue& queue);
void apply_mood_spot_lights(MoodDeps* c, const MoodProfile& m, wgpu::Queue& queue);
void apply_mood_indoor_shell(MoodDeps* c, const MoodProfile& m, wgpu::Queue& queue,
    GalleryState& gallery_state, GalleryDeps& gallery_deps);
// Indoor support
void derive_indoor_lights(MoodDeps* c, uint32_t seed, float bmin, float bmax,
    float ceiling_height, CeilingType ceiling_type = CeilingType::FLAT);
void generate_indoor_shell(MoodDeps* c, wgpu::Queue& queue, const MoodProfile& m,
    const IndoorPalette& pal,
    GalleryState& gallery_state, GalleryDeps& gallery_deps);
void clear_indoor_shell(MoodDeps* c, wgpu::Queue& queue,
    GalleryState& gallery_state, GalleryDeps& gallery_deps);
// Portals (door; the internals route through entities' force_spawn_portal_arch
// — the arch's owner writes, so the machine face rides the tail param)
void force_spawn_back_portal(MoodDeps* c, wgpu::Queue& queue, MachineCtx& machine_ctx);
// Per-frame uploads (doors)
void upload_lights(MoodDeps* c, wgpu::Queue& queue);
void upload_portal_array(MoodDeps* c, wgpu::Queue& queue);
// Derivers (door + the portal-detection pipeline's shared helpers)
const char* mood_name(uint32_t mood);
uint32_t derive_finite_radius(uint32_t seed, const MoodProfile& mood);
uint32_t pick_portal_mood(MachineCtx* c, uint32_t seed, uint32_t prop);


// ═══ MODULE IMPLEMENTATION ════════════════════════════════════════
//
// The doors + appliers + derivers. The bodies reach the deps face
// (c->mood_state_ / c->world_state_ / c->gpuState_ / c->renderer_ /
// c->gol_state_ / c->entities_state_ / the sun + clear channel / the
// CPU light + portal arrays / the back-portal anchor), the fan's
// TARGET organs (parameters — orbs/gallery/pawn + the machine
// face), TransitionPhase (contracts/spine_state.hpp), ARCH_TIERS /
// ArchIdx (contracts/spawn_services.hpp), solve_catenary_a
// (seed_utils.hpp), and Dim::PATCH_EXTENT (patch_system.hpp).
//
// THE CHANNEL: the force-spawn mutation of the arch belongs to the
// arch's owner. The indoor lighting-scheme tables live impl-side:
// single consumer (derive_indoor_lights); the wall PALETTES — the
// other half of SEAM[mood:tuning-data] — sit above with the deps.


// ── Impl-internal forward declarations ───────────────────────────
// Used before their definitions (which keep their original section
// homes below). Impl-only — not part of the header surface.
inline void force_spawn_finite_portals(MoodDeps* c, wgpu::Queue& queue, MachineCtx& machine_ctx);

// ═══ TUNING DATA (impl-side — internal authoring tables) ═════════
//
// Per-mood indoor lighting schemes. Seed-driven (a per-mood roll picks
// one of the schemes from the table), then per-light parameters are
// sampled from the chosen entry. The indoor wall PALETTES — this
// seam's other half — live declaration-side in mood.hpp
// (SEAM[mood:tuning-data] names the split).

// ── Indoor Lighting Schemes ───────────────────────────────

enum class LightAnchor : uint32_t {
    CEILING, WALL_NORTH, WALL_SOUTH, WALL_EAST, WALL_WEST
};

struct IndoorLightProp {
    static constexpr uint32_t SCHEME = 1100u;
    static constexpr uint32_t WALL_PAIR = 1101u;
    static constexpr uint32_t ANCHOR_PICK = 1102u;
    static constexpr uint32_t SLOT_BASE = 1110u;  // + slot*10 + field
    // Per-slot field offsets
    static constexpr uint32_t LATERAL = 0u;
    static constexpr uint32_t HEIGHT = 1u;
    static constexpr uint32_t INTENSITY = 2u;
    static constexpr uint32_t INNER_CONE = 3u;
    static constexpr uint32_t OUTER_CONE = 4u;
    static constexpr uint32_t WARMTH = 5u;
    static constexpr uint32_t AIM_PITCH = 6u;
    static constexpr uint32_t AIM_YAW = 7u;
};

struct LightSlotDef {
    LightAnchor anchor;
    float intensity_mean, intensity_sigma;
    float inner_mean, inner_sigma;    // inner half-angle (radians)
    float outer_mean, outer_sigma;    // outer half-angle (radians)
    float warmth_mean, warmth_sigma;  // 0 = warm amber, 1 = cool blue
    float aim_pitch_mean, aim_pitch_sigma;  // radians
    float aim_yaw_mean, aim_yaw_sigma;      // radians
    // Anchor-surface position (carried through from LightSchemeSlot).
    float lat_mean, lat_sigma;        // along anchor surface
    float hfrac_mean, hfrac_sigma;    // ceiling: Z; walls: height
};

// ── THE LIGHT COUNT, DRAWN PER ROOM (SWEEP_1 T6) ──────────────────
// Count and scheme are the SAME draw here, because the scheme table is
// a bijection onto the counts — Cathedral 3, Quartet 4, Gallery 2,
// Sanctum 1, one scheme per count. So the ruled count distribution IS
// this weight row, re-ordered onto the schemes that carry those counts:
//
//   4 lights → Quartet    70%   (was 35%)
//   3 lights → Cathedral  20%   (was 35%)
//   2 lights → Gallery     7%   (was 15%)
//   1 light  → Sanctum     3%   (was 15%)
//
// The draw itself is unchanged: select_tier → select_weighted walks
// these cumulatively off cpu_hash_f(seed, SCHEME), the tree's existing
// RNG, seeded per destination.
//
// EYES OPEN (LEDGER_1 F-3): the indoor shadow list is drawn ONCE PER
// SPOT LIGHT, terrain included, and this row makes the 4-light maximum
// the common case rather than a third of rooms. Ruled anyway; the
// multiplier is Layer E's to own, not this sweep's.
inline constexpr float SCHEME_WEIGHTS[] = { 0.20f, 0.70f, 0.07f, 0.03f };
inline constexpr uint32_t SCHEME_COUNT = 4;
inline constexpr const char* SCHEME_NAMES[] = { "Cathedral", "Quartet", "Gallery", "Sanctum" };
inline constexpr const char* ANCHOR_NAMES[] = { "ceiling", "wall_N", "wall_S", "wall_E", "wall_W" };

// ── Lighting Scheme Table ──

enum class AnchorRole : uint32_t {
    CEILING,    // always ceiling
    WALL_A,     // seed-selected wall pair, side A
    WALL_B,     // seed-selected wall pair, side B
    SEED_PICK   // anchor chosen from seed (ceiling or wall)
};

struct LightSchemeSlot {
    AnchorRole role;
    float intensity_mean, intensity_sigma;
    float inner_mean, inner_sigma;
    float outer_mean, outer_sigma;
    float warmth_mean, warmth_sigma;
    float aim_pitch_mean, aim_pitch_sigma;
    float aim_yaw_mean, aim_yaw_sigma;
    float lat_mean, lat_sigma;
    float hfrac_mean, hfrac_sigma;
};

struct LightScheme {
    uint32_t slot_count;
    LightSchemeSlot slots[4];  // MAX_SPOT_LIGHTS
};

// ── Scheme Definitions ──
//                                role            int_μ  int_σ  inn_μ inn_σ out_μ out_σ wrm_μ wrm_σ  pit_μ  pit_σ  yaw_μ  yaw_σ   lat_μ lat_σ  hfrac_μ hfrac_σ
inline constexpr LightScheme LIGHT_SCHEMES[SCHEME_COUNT] = {
    /* 0: Cathedral — ceiling primary + 2 wall sconces */
    { 3, {
        { AnchorRole::CEILING,   8.0f, 2.5f, 0.6f, 0.2f,  1.2f, 0.15f, 0.35f, 0.20f,  0.0f,  0.12f, 0.0f, 0.12f,   0.50f, 0.15f, 0.65f, 0.10f },
        { AnchorRole::WALL_A,    5.0f, 1.5f, 0.4f, 0.15f, 1.0f, 0.15f, 0.20f, 0.15f,  0.60f, 0.40f, 0.0f, 0.30f,   0.50f, 0.15f, 0.65f, 0.10f },
        { AnchorRole::WALL_B,    5.0f, 1.5f, 0.4f, 0.15f, 1.0f, 0.15f, 0.75f, 0.15f,  0.60f, 0.40f, 0.0f, 0.30f,   0.50f, 0.15f, 0.65f, 0.10f },
    }},
    /* 1: Quartet — 4 ceiling lights at quadrant corners.
     *    Tight sigma (0.07) keeps each light pinned in its quadrant;
     *    means at 0.30/0.70 produce a 2x2 layout that fills the
     *    room evenly. Per-light intensity dropped to 4.0±1.0 to
     *    keep total scene brightness comparable to Cathedral
     *    (which has ~8.0 from a single ceiling light + sconces). */
    { 4, {
        { AnchorRole::CEILING,   4.0f, 1.0f, 0.6f, 0.2f,  1.2f, 0.15f, 0.35f, 0.20f,  0.0f,  0.10f, 0.0f, 0.10f,   0.30f, 0.07f, 0.30f, 0.07f },
        { AnchorRole::CEILING,   4.0f, 1.0f, 0.6f, 0.2f,  1.2f, 0.15f, 0.35f, 0.20f,  0.0f,  0.10f, 0.0f, 0.10f,   0.70f, 0.07f, 0.30f, 0.07f },
        { AnchorRole::CEILING,   4.0f, 1.0f, 0.6f, 0.2f,  1.2f, 0.15f, 0.35f, 0.20f,  0.0f,  0.10f, 0.0f, 0.10f,   0.30f, 0.07f, 0.70f, 0.07f },
        { AnchorRole::CEILING,   4.0f, 1.0f, 0.6f, 0.2f,  1.2f, 0.15f, 0.35f, 0.20f,  0.0f,  0.10f, 0.0f, 0.10f,   0.70f, 0.07f, 0.70f, 0.07f },
    }},
    /* 2: Gallery — 2 opposing wall lights, no ceiling */
    { 2, {
        { AnchorRole::WALL_A,    7.0f, 2.0f, 0.4f, 0.15f, 1.1f, 0.15f, 0.25f, 0.20f,  0.55f, 0.45f, 0.0f, 0.35f,   0.50f, 0.15f, 0.65f, 0.10f },
        { AnchorRole::WALL_B,    7.0f, 2.0f, 0.4f, 0.15f, 1.1f, 0.15f, 0.65f, 0.20f,  0.55f, 0.45f, 0.0f, 0.35f,   0.50f, 0.15f, 0.65f, 0.10f },
    }},
    /* 3: Sanctum — single dramatic source */
    { 1, {
        { AnchorRole::SEED_PICK, 10.0f, 2.5f, 0.5f, 0.2f, 1.2f, 0.15f, 0.45f, 0.25f,  0.50f, 0.40f, 0.0f, 0.30f,   0.50f, 0.15f, 0.65f, 0.10f },
    }},
};

// THE COUNT WITNESSES (SWEEP_1 T6). The ruled distribution is over
// COUNTS, and it only reaches the world through SCHEME_WEIGHTS because
// scheme→slot_count is one-to-one. If a scheme is ever re-slotted or a
// fifth is added, that bijection breaks and the count distribution
// silently stops being the ruled one — with no other diagnostic. These
// pin it where the rows live.
static_assert(sizeof(SCHEME_WEIGHTS) / sizeof(SCHEME_WEIGHTS[0]) == SCHEME_COUNT,
    "SCHEME_WEIGHTS must carry one weight per scheme");
static_assert(LIGHT_SCHEMES[0].slot_count == 3 && LIGHT_SCHEMES[1].slot_count == 4
           && LIGHT_SCHEMES[2].slot_count == 2 && LIGHT_SCHEMES[3].slot_count == 1,
    "T6: scheme→count is a bijection (3/4/2/1); SCHEME_WEIGHTS carries the ruled "
    "count distribution 4@70 / 3@20 / 2@7 / 1@3 through it");
static_assert(SCHEME_WEIGHTS[1] == 0.70f && SCHEME_WEIGHTS[0] == 0.20f
           && SCHEME_WEIGHTS[2] == 0.07f && SCHEME_WEIGHTS[3] == 0.03f,
    "T6: the ruled count weights, on the schemes that carry those counts");

// ═══ INDOOR LIGHT DERIVATION ═════════════════════════════════════

inline void derive_indoor_lights(MoodDeps* c, uint32_t seed, float bmin, float bmax,
    float ceiling_height, CeilingType ceiling_type) {
    c->cpuSpotLights_ = GPUSpotLightArray{};

    float room_size = bmax - bmin;
    float room_range = room_size * 0.8f;

    // Scheme selection
    uint32_t scheme = select_tier(seed, IndoorLightProp::SCHEME,
        SCHEME_WEIGHTS, SCHEME_COUNT);

    // Wall pair: N/S or E/W — gives axis variety across seeds
    bool use_ew = cpu_hash_f(seed, IndoorLightProp::WALL_PAIR) > 0.5f;
    LightAnchor wall_a = use_ew ? LightAnchor::WALL_EAST : LightAnchor::WALL_NORTH;
    LightAnchor wall_b = use_ew ? LightAnchor::WALL_WEST : LightAnchor::WALL_SOUTH;

    // Read slot definitions from scheme table
    const auto& sch = LIGHT_SCHEMES[scheme];
    LightSlotDef slots[MAX_SPOT_LIGHTS];
    uint32_t count = sch.slot_count;

    for (uint32_t i = 0; i < count; i++) {
        const auto& src = sch.slots[i];
        // Resolve anchor role to concrete LightAnchor
        LightAnchor anchor;
        switch (src.role) {
        case AnchorRole::CEILING:   anchor = LightAnchor::CEILING; break;
        case AnchorRole::WALL_A:    anchor = wall_a; break;
        case AnchorRole::WALL_B:    anchor = wall_b; break;
        case AnchorRole::SEED_PICK: {
            float anchor_roll = cpu_hash_f(seed, IndoorLightProp::ANCHOR_PICK);
            anchor = (anchor_roll < 0.55f) ? LightAnchor::CEILING
                : (anchor_roll < 0.775f) ? wall_a : wall_b;
            break;
        }
        }
        slots[i] = { anchor,
            src.intensity_mean, src.intensity_sigma,
            src.inner_mean, src.inner_sigma,
            src.outer_mean, src.outer_sigma,
            src.warmth_mean, src.warmth_sigma,
            src.aim_pitch_mean, src.aim_pitch_sigma,
            src.aim_yaw_mean, src.aim_yaw_sigma,
            src.lat_mean, src.lat_sigma,
            src.hfrac_mean, src.hfrac_sigma };
    }

    // Derive each light from its slot spec + seed
    for (uint32_t i = 0; i < count; i++) {
        const auto& s = slots[i];
        uint32_t base = IndoorLightProp::SLOT_BASE + i * 10;
        auto& L = c->cpuSpotLights_.lights[i];

        float lat = std::clamp(
            cpu_sample_gaussian(seed, base + IndoorLightProp::LATERAL,
                s.lat_mean, s.lat_sigma),
            0.1f, 0.9f);
        float hfrac_raw = cpu_sample_gaussian(seed,
            base + IndoorLightProp::HEIGHT, s.hfrac_mean, s.hfrac_sigma);
        float hfrac = (s.anchor == LightAnchor::CEILING)
            ? std::clamp(hfrac_raw, 0.1f, 0.9f)    // ceiling: Z-axis position
            : std::clamp(hfrac_raw, 0.4f, 0.85f);  // walls: sconce height

        float wall_off = 1.0f;
        float px, py, pz;

        // Position: anchor-dependent placement on surface
        switch (s.anchor) {
        case LightAnchor::CEILING:
            px = bmin + lat * room_size;
            py = ceiling_height - 0.5f;
            pz = bmin + hfrac * room_size;
            break;
        case LightAnchor::WALL_NORTH:
            px = bmin + lat * room_size;
            py = ceiling_height * hfrac;
            pz = bmax - wall_off;
            break;
        case LightAnchor::WALL_SOUTH:
            px = bmin + lat * room_size;
            py = ceiling_height * hfrac;
            pz = bmin + wall_off;
            break;
        case LightAnchor::WALL_EAST:
            px = bmax - wall_off;
            py = ceiling_height * hfrac;
            pz = bmin + lat * room_size;
            break;
        case LightAnchor::WALL_WEST:
            px = bmin + wall_off;
            py = ceiling_height * hfrac;
            pz = bmin + lat * room_size;
            break;
        }

        float pitch = cpu_sample_gaussian(seed, base + IndoorLightProp::AIM_PITCH,
            s.aim_pitch_mean, s.aim_pitch_sigma);
        float yaw = cpu_sample_gaussian(seed, base + IndoorLightProp::AIM_YAW,
            s.aim_yaw_mean, s.aim_yaw_sigma);

        float dx, dy, dz;
        if (s.anchor == LightAnchor::CEILING) {
            // pitch=0 → straight down; positive tilts off-vertical
            pitch = std::clamp(pitch, 0.0f, 0.50f);
            yaw = std::clamp(yaw, -0.60f, 0.60f);
            dx = std::sin(pitch) * std::sin(yaw);
            dy = -std::cos(pitch);
            dz = std::sin(pitch) * std::cos(yaw);
        }
        else {
            // pitch=0 → horizontal into room; π/2 → straight down
            // Floor at 0.08 ensures light never aims upward.
            // Ceiling at 1.45 (~83°) allows steep floor pools.
            pitch = std::clamp(pitch, 0.08f, 1.45f);
            yaw = std::clamp(yaw, -0.80f, 0.80f);
            float cp = std::cos(pitch), sp = std::sin(pitch);
            float sy = std::sin(yaw);
            switch (s.anchor) {
            case LightAnchor::WALL_NORTH: dx = sy; dy = -sp; dz = -cp; break;
            case LightAnchor::WALL_SOUTH: dx = sy; dy = -sp; dz = cp; break;
            case LightAnchor::WALL_EAST:  dx = -cp; dy = -sp; dz = sy; break;
            case LightAnchor::WALL_WEST:  dx = cp; dy = -sp; dz = sy; break;
            default: dx = 0; dy = -1; dz = 0; break;  // unreachable
            }
        }

        // Normalize direction
        float dlen = std::sqrt(dx * dx + dy * dy + dz * dz);
        dx /= dlen; dy /= dlen; dz /= dlen;

        // Intensity and cone angles
        float intensity = std::max(1.0f,
            cpu_sample_gaussian(seed, base + IndoorLightProp::INTENSITY,
                s.intensity_mean, s.intensity_sigma));
        float inner_half = std::max(0.2f,
            cpu_sample_gaussian(seed, base + IndoorLightProp::INNER_CONE,
                s.inner_mean, s.inner_sigma));
        float outer_half = std::max(inner_half + 0.3f,
            cpu_sample_gaussian(seed, base + IndoorLightProp::OUTER_CONE,
                s.outer_mean, s.outer_sigma));
        // Hard cap: outer cone must not exceed shadow map FOV capability.
        // At 1.3 rad (75°), shadow FOV = 2×1.3+0.2 = 2.8 rad → coverable.
        static constexpr float MAX_OUTER_HALF = 1.3f;
        outer_half = std::min(outer_half, MAX_OUTER_HALF);
        inner_half = std::min(inner_half, outer_half - 0.1f);

        // Color: warm amber (1.0,0.85,0.65) ↔ cool blue (0.80,0.88,1.0)
        float w = std::clamp(
            cpu_sample_gaussian(seed, base + IndoorLightProp::WARMTH,
                s.warmth_mean, s.warmth_sigma),
            0.0f, 1.0f);
        float cr = 1.00f + w * (0.80f - 1.00f);
        float cg = 0.85f + w * (0.88f - 0.85f);
        float cb = 0.65f + w * (1.00f - 0.65f);

        L.position[0] = px; L.position[1] = py; L.position[2] = pz;
        L.direction[0] = dx; L.direction[1] = dy; L.direction[2] = dz;
        L.color[0] = cr; L.color[1] = cg; L.color[2] = cb;
        L.intensity = intensity;
        L.inner_cone = std::cos(inner_half);
        L.outer_cone = std::cos(outer_half);
        L.range = (s.anchor == LightAnchor::CEILING)
            ? ceiling_height + 30.0f : room_range;

    }

    c->cpuSpotLights_.count = count;

    // ─── Vault Uplight ───────────────────────────────────────
    //
    if (ceiling_type == CeilingType::VAULT && count < MAX_SPOT_LIGHTS) {
        auto& L = c->cpuSpotLights_.lights[count];
        float center = (bmin + bmax) * 0.5f;
        L.position[0] = center;
        L.position[1] = 2.0f;           // near floor level
        L.position[2] = center;
        L.direction[0] = 0.0f;
        L.direction[1] = 1.0f;           // straight up
        L.direction[2] = 0.0f;
        L.color[0] = 0.95f;
        L.color[1] = 0.90f;
        L.color[2] = 0.80f;              // warm ambient
        L.intensity = 2.5f;               // subtle — structural reveal, not flood
        L.inner_cone = std::cos(0.9f);    // ~52° half-angle
        L.outer_cone = std::cos(1.25f);   // ~72° half-angle — within shadow FOV cap
        L.range = ceiling_height + 80.0f; // reach the crown with headroom
        count++;
        c->cpuSpotLights_.count = count;
        std::cout << "[Lighting] Added vault uplight (slot " << (count - 1) << ")\n";
    }

    std::cout << "[Lighting] " << SCHEME_NAMES[scheme]
        << " (" << count << " lights, "
        << (use_ew ? "E/W" : "N/S") << " walls)\n";
}

// ═══ APPLY MOOD ══════════════════════════════════════════════════

// 1) Atmospheric: sun direction/color/intensity, fog, ambient,
//    terrain amp ceiling. Touches GPU directly + a few member fields.
inline void apply_mood_lighting(MoodDeps* c, const MoodProfile& m, wgpu::Queue& /*queue*/) {
    c->sunDirection_[0] = m.sun_direction[0];
    c->sunDirection_[1] = m.sun_direction[1];
    c->sunDirection_[2] = m.sun_direction[2];

    // Push to GPU config so compute_vp builds the shadow VP from the correct direction.
    {
        const float len = std::sqrt(m.sun_direction[0] * m.sun_direction[0] +
                                    m.sun_direction[1] * m.sun_direction[1] +
                                    m.sun_direction[2] * m.sun_direction[2]);
        c->gpuState_.set_sun_direction(m.sun_direction[0] / len,
                                    m.sun_direction[1] / len,
                                    m.sun_direction[2] / len);
    }

    c->sunColor_[0] = m.sun_color[0];
    c->sunColor_[1] = m.sun_color[1];
    c->sunColor_[2] = m.sun_color[2];
    c->mood_state_.sun_intensity = m.sun_intensity;
    c->mood_state_.sun_ambient   = m.sun_ambient;

    c->clearColor_[0] = m.clear_color[0];
    c->clearColor_[1] = m.clear_color[1];
    c->clearColor_[2] = m.clear_color[2];

    c->gpuState_.set_terrain_amp_ceiling(m.terrain_amp_ceiling);
    c->mood_state_.terrain_amp_ceiling = m.terrain_amp_ceiling;
    // The GoL cap rides beside the amp column: indoors the cell lift caps
    // at the module's fraction of the ceiling; 0 disables (the derive
    // kernel's sentinel — outdoor byte-identical). Zones are torn down at
    // every mood transition, so every live zone was derived inside the mood
    // it lives in and the capping is complete, not approximate.
    c->gpuState_.set_indoor_height_cap(
        m.indoor ? INDOOR_HEIGHT_CAP_FRACTION * m.ceiling_height : 0.0f);
    c->mood_state_.lights_dirty = true;
}

inline void apply_mood_spot_lights(MoodDeps* c, const MoodProfile& m, wgpu::Queue& queue) {
    c->cpuSpotLights_ = GPUSpotLightArray{};
    if (m.indoor) {
        c->gpuState_.set_mute_coupling(Coupling::PAWN_TO_SUN_VP, true);

        const float bmin = -(float)c->world_state_.finite_radius * Dim::PATCH_EXTENT;
        const float bmax = ((float)c->world_state_.finite_radius + 1.0f) * Dim::PATCH_EXTENT;
        derive_indoor_lights(c, c->world_state_.active_seed, bmin, bmax, m.ceiling_height, m.ceiling_type);

        for (uint32_t i = 0; i < c->cpuSpotLights_.count; i++) {
            compute_spot_light_vp(c->cpuSpotLights_.lights[i],
                                  c->cpuSpotLights_.lights[i].view_proj);
        }
        c->gpuState_.stage_spot_vps(queue, c->cpuSpotLights_);
        c->mood_state_.spot_light_active = true;
    } else {
        c->gpuState_.set_mute_coupling(Coupling::PAWN_TO_SUN_VP, false);
        c->mood_state_.spot_light_active = false;
    }
}

// ═══ THE CROWN LAW — one spelling of the vault arithmetic ═════════
// Spring (wall top), rise, crown for a VAULT room over the finite
// bounds [bmin, bmax]. The generator and the camera ceiling clamp
// both consume THIS — no bare-literal twin survives at either site.
struct VaultCrown { float spring_h; float rise; float crown_h; };
inline VaultCrown vault_crown(const MoodProfile& m, float bmin, float bmax) {
    static constexpr float VAULT_RISE_FRACTION  = 0.30f;
    static constexpr float SPRING_MARGIN        = 8.0f;
    static constexpr float PAINT_CENTER_FRACTION = 0.45f;
    static constexpr float PAINT_TOP_MARGIN     = 5.5f;
    static constexpr float MIN_RISE_FLOOR       = 5.0f;
    const float ch = m.ceiling_height;
    const float half_span = (bmax - bmin) * 0.5f;
    const float paint_center = ch * PAINT_CENTER_FRACTION;
    const float paint_top = paint_center + PAINT_TOP_MARGIN;
    const float spring_h = paint_top + SPRING_MARGIN;
    const float min_rise = ch - spring_h;
    const float rise = std::max(half_span * VAULT_RISE_FRACTION,
                                std::max(min_rise, MIN_RISE_FLOOR));
    return VaultCrown{ spring_h, rise, spring_h + rise };
}
inline float vault_crown_height(const MoodProfile& m, float bmin, float bmax) {
    return vault_crown(m, bmin, bmax).crown_h;
}

inline void apply_mood_indoor_shell(MoodDeps* c, const MoodProfile& m, wgpu::Queue& queue,
    GalleryState& gallery_state, GalleryDeps& gallery_deps) {
    if (m.indoor && m.ceiling_type != CeilingType::NONE) {
        const uint32_t pal_idx = cpu_hash(c->world_state_.active_seed, 5800u) % INDOOR_PALETTE_COUNT;
        const auto& pal = INDOOR_PALETTES[pal_idx];
        std::cout << "[Mood] Indoor palette: " << pal.name
                  << " (idx=" << pal_idx << ")\n";
        generate_indoor_shell(c, queue, m, pal, gallery_state, gallery_deps);
    } else {
        clear_indoor_shell(c, queue, gallery_state, gallery_deps);
    }

    // Camera ceiling clamp — consumes THE CROWN LAW (vault_crown_height).
    if (m.indoor) {
        float effective_ceiling = m.ceiling_height;
        if (m.ceiling_type == CeilingType::VAULT) {
            const float bmin = -(float)c->world_state_.finite_radius * Dim::PATCH_EXTENT;
            const float bmax = ((float)c->world_state_.finite_radius + 1.0f) * Dim::PATCH_EXTENT;
            effective_ceiling = vault_crown_height(m, bmin, bmax);
        }
        c->gpuState_.set_ceiling_height(effective_ceiling);
    } else {
        c->gpuState_.set_ceiling_height(0.0f);
    }
}

// ── apply_mood (orchestrator) ──
//
inline void apply_mood(MoodDeps* c, uint32_t mood, wgpu::Queue& queue,
    MachineCtx& machine_ctx,
    OrbsState& orbs_state, OrbsDeps& orbs_deps,
    GalleryState& gallery_state, GalleryDeps& gallery_deps,
    PawnState& pawn_state) {
    mood = std::min(mood, MOOD_COUNT - 1);
    c->mood_state_.active = mood;
    const auto& m = MOOD_TABLE[mood];

    // Frustum cull is mood-driven (not tied to indoor/outdoor).
    c->renderer_.set_frustum_cull_active(m.allow_frustum_cull);

    // Per-mood feature gates: GoL zones, aura.
    // Aura policy: respect player preference when permitted, force off when forbidden.
    c->gol_state_.mood_allowed     = m.allow_gol_zones;
    apply_aura_mood_policy(pawn_state, m.allow_pawn_aura);  // the pawn door; byte-identical semantics

    apply_mood_lighting(c, m, queue);          // sun + fog + amp ceiling (foundational — sun is not a piece)
    if constexpr (ROSTER.spot_lights)          // ROSTER-GATE spot_lights (b) — indoor spot array never configured
        apply_mood_spot_lights(c, m, queue);   // indoor only
    if constexpr (ROSTER.indoor_shell)         // ROSTER-GATE indoor_shell (b) — walls/ceiling never generated
        apply_mood_indoor_shell(c, m, queue, gallery_state, gallery_deps);  // shell + camera ceiling clamp
    if constexpr (ROSTER.orbs)                 // ROSTER-GATE orbs (b) — sky dome never configured
        configure_orbs(orbs_state, &orbs_deps, ORB_MOOD_TABLE[mood], queue);

    std::cout << "[Mood] Applied: " << mood_name(mood)
        << " (mood=" << mood
        << (m.indoor ? " INDOOR" : " outdoor")
        << ")\n";
}

// ═══ INDOOR SHELL GENERATION ═════════════════════════════════════

inline void clear_indoor_shell(MoodDeps* c, wgpu::Queue& queue,
    GalleryState& gallery_state, GalleryDeps& gallery_deps) {
    c->gpuState_.set_shell_index_count(0);
    clear_wall_paintings(gallery_state, &gallery_deps, queue);
}

// Helper: push a quad (2 triangles) into vertex/index vectors
inline void push_quad(
    std::vector<ShellVertex>& verts,
    std::vector<uint32_t>& indices,
    float ax, float ay, float az,
    float bx, float by, float bz,
    float cx, float cy, float cz,
    float dx, float dy, float dz,
    float nx, float ny, float nz,
    const float* color
) {
    uint32_t base = static_cast<uint32_t>(verts.size());
    verts.push_back({ { ax, ay, az }, { nx, ny, nz }, { color[0], color[1], color[2] } });
    verts.push_back({ { bx, by, bz }, { nx, ny, nz }, { color[0], color[1], color[2] } });
    verts.push_back({ { cx, cy, cz }, { nx, ny, nz }, { color[0], color[1], color[2] } });
    verts.push_back({ { dx, dy, dz }, { nx, ny, nz }, { color[0], color[1], color[2] } });
    indices.push_back(base); indices.push_back(base + 1); indices.push_back(base + 2);
    indices.push_back(base); indices.push_back(base + 2); indices.push_back(base + 3);
}

inline void generate_indoor_shell(MoodDeps* c, wgpu::Queue& queue, const MoodProfile& m,
    const IndoorPalette& pal,
    GalleryState& gallery_state, GalleryDeps& gallery_deps) {
    float bmin = -(float)c->world_state_.finite_radius * Dim::PATCH_EXTENT;
    float bmax = ((float)c->world_state_.finite_radius + 1.0f) * Dim::PATCH_EXTENT;
    float ch = m.ceiling_height;

    std::vector<ShellVertex> verts;
    std::vector<uint32_t> indices;
    verts.reserve(2400);
    indices.reserve(12000);

    static constexpr float JOINT_OVERLAP = 3.0f;
    static constexpr float WALL_FLOOR = -50.0f;

    // ─── Compute wall height (depends on ceiling type) ───────
    //
    float wall_h = ch;  // flat ceiling: walls go to ceiling
    float crown_h = ch; // effective crown height for log
    float rise = 0.0f;

    if (m.ceiling_type == CeilingType::VAULT) {
        const VaultCrown vc = vault_crown(m, bmin, bmax);   // THE CROWN LAW
        wall_h  = vc.spring_h;
        rise    = vc.rise;
        crown_h = vc.crown_h;
    }

    float wall_top = wall_h + JOINT_OVERLAP;

    // ─── 4 Walls (deep floor to wall_top) ────────────────────
    // South wall (z = bmin, facing +Z into room)
    push_quad(verts, indices,
        bmin, WALL_FLOOR, bmin, bmax, WALL_FLOOR, bmin,
        bmax, wall_top, bmin, bmin, wall_top, bmin,
        0.0f, 0.0f, 1.0f, pal.wall_color);
    // North wall (z = bmax, facing -Z into room)
    push_quad(verts, indices,
        bmax, WALL_FLOOR, bmax, bmin, WALL_FLOOR, bmax,
        bmin, wall_top, bmax, bmax, wall_top, bmax,
        0.0f, 0.0f, -1.0f, pal.wall_color);
    // West wall (x = bmin, facing +X into room)
    push_quad(verts, indices,
        bmin, WALL_FLOOR, bmax, bmin, WALL_FLOOR, bmin,
        bmin, wall_top, bmin, bmin, wall_top, bmax,
        1.0f, 0.0f, 0.0f, pal.wall_color);
    // East wall (x = bmax, facing -X into room)
    push_quad(verts, indices,
        bmax, WALL_FLOOR, bmin, bmax, WALL_FLOOR, bmax,
        bmax, wall_top, bmax, bmax, wall_top, bmin,
        -1.0f, 0.0f, 0.0f, pal.wall_color);

    // ─── Ceiling ─────────────────────────────────────────────
    if (m.ceiling_type == CeilingType::FLAT) {
        push_quad(verts, indices,
            bmin, ch, bmin, bmax, ch, bmin,
            bmax, ch, bmax, bmin, ch, bmax,
            0.0f, -1.0f, 0.0f, pal.ceiling_color);
    }
    else if (m.ceiling_type == CeilingType::VAULT) {
        // ─── Groin vault (cross vault) ───────────────────────

        float half_x = (bmax - bmin) * 0.5f;
        float half_z = (bmax - bmin) * 0.5f;
        float center_x = (bmin + bmax) * 0.5f;
        float center_z = (bmin + bmax) * 0.5f;
        float cat_a_x = solve_catenary_a(half_x, rise);
        float cat_a_z = solve_catenary_a(half_z, rise);

        static constexpr uint32_t VAULT_N = 32;

        auto catenary_y = [&](float dist, float cat_a) -> float {
            return crown_h - cat_a * (std::cosh(dist / cat_a) - 1.0f);
            };

        uint32_t v_base = static_cast<uint32_t>(verts.size());
        for (uint32_t iz = 0; iz <= VAULT_N; iz++) {
            float tz = (float)iz / (float)VAULT_N;
            float z = bmin + tz * (bmax - bmin);
            float dz = z - center_z;

            for (uint32_t ix = 0; ix <= VAULT_N; ix++) {
                float tx = (float)ix / (float)VAULT_N;
                float x = bmin + tx * (bmax - bmin);
                float dx = x - center_x;

                float y_barrel_x = catenary_y(dx, cat_a_x);
                float y_barrel_z = catenary_y(dz, cat_a_z);
                float y = std::min(y_barrel_x, y_barrel_z);

                bool on_edge = (ix == 0 || ix == VAULT_N || iz == 0 || iz == VAULT_N);
                if (on_edge) {
                    y = wall_h - JOINT_OVERLAP;
                }

                // Normal via finite differences
                float eps = (bmax - bmin) / (float)VAULT_N * 0.5f;
                float y_px = std::min(catenary_y(dx + eps, cat_a_x), catenary_y(dz, cat_a_z));
                float y_mx = std::min(catenary_y(dx - eps, cat_a_x), catenary_y(dz, cat_a_z));
                float y_pz = std::min(catenary_y(dx, cat_a_x), catenary_y(dz + eps, cat_a_z));
                float y_mz = std::min(catenary_y(dx, cat_a_x), catenary_y(dz - eps, cat_a_z));

                float ddx = (y_px - y_mx) / (2.0f * eps);
                float ddz = (y_pz - y_mz) / (2.0f * eps);
                float nmag = std::sqrt(ddx * ddx + 1.0f + ddz * ddz);
                float fnx = -ddx / nmag;
                float fny = -1.0f / nmag;
                float fnz = -ddz / nmag;

                if (on_edge) { fnx = 0.0f; fny = -1.0f; fnz = 0.0f; }

                verts.push_back({ { x, y, z }, { fnx, fny, fnz },
                    { pal.ceiling_color[0], pal.ceiling_color[1], pal.ceiling_color[2] } });
            }
        }

        for (uint32_t iz = 0; iz < VAULT_N; iz++) {
            for (uint32_t ix = 0; ix < VAULT_N; ix++) {
                uint32_t a = v_base + iz * (VAULT_N + 1) + ix;
                uint32_t b = a + 1;
                uint32_t c = a + (VAULT_N + 1);
                uint32_t d = c + 1;
                indices.push_back(a); indices.push_back(b); indices.push_back(c);
                indices.push_back(b); indices.push_back(d); indices.push_back(c);
            }
        }
    }

    uint32_t vc = std::min(static_cast<uint32_t>(verts.size()), Dim::SHELL_MAX_VERTICES);
    uint32_t ic = std::min(static_cast<uint32_t>(indices.size()), Dim::SHELL_MAX_INDICES);

    c->gpuState_.upload_shell_mesh(queue, verts.data(), vc, indices.data(), ic);

    place_wall_paintings(gallery_state, &gallery_deps, queue, bmin, bmax, ch);

    std::cout << "[Shell] Generated "
        << (m.ceiling_type == CeilingType::FLAT ? "FLAT" : "GROIN VAULT")
        << ": " << vc << " verts, " << ic << " indices"
        << " bounds=[" << bmin << "," << bmax << "]"
        << " wall_h=" << wall_h << " crown=" << crown_h
        << " rise=" << rise << "\n";
}

// ═══ PORTAL SPAWNING ═════════════════════════════════════════════

// ── force_spawn_portal_at ──
//
inline uint32_t force_spawn_portal_at(MoodDeps* c, wgpu::Queue& queue,
    float cx, float cz, float rotation,
    const PortalDestination& dest, bool is_back_portal,
    MachineCtx& machine_ctx) {
    const float* pc = is_back_portal
        ? PORTAL_COLOR_BACK
        : PORTAL_COLORS[dest.mood % MOOD_COUNT];

    uint32_t slot = force_spawn_portal_arch(machine_ctx.entities_state_, &machine_ctx, queue,
        cx, cz, rotation, dest, is_back_portal, pc);

    if (slot != UINT32_MAX) c->mood_state_.portals_dirty = true;
    return slot;
}

// ── force_spawn_back_portal ──
//
inline void force_spawn_back_portal(MoodDeps* c, wgpu::Queue& queue, MachineCtx& machine_ctx) {
    c->mood_state_.back_portal_pending = false;

    // ─── Seed-driven placement ───────────────────────────────────────
    //
    if (c->world_state_.finite_mode) {
        float bmin = -(float)c->world_state_.finite_radius * Dim::PATCH_EXTENT;
        float bmax = ((float)c->world_state_.finite_radius + 1.0f) * Dim::PATCH_EXTENT;
        float room_center = (bmin + bmax) * 0.5f;
        float room_half = (bmax - bmin) * 0.5f;

        float WALL_MARGIN;
        if (MOOD_TABLE[c->mood_state_.active].indoor) {
            const auto& doorway = ARCH_TIERS[static_cast<uint32_t>(ArchTier::DOORWAY)].profile;
            const float doorway_half_span = doorway.params[ArchIdx::SPAN].mean * 0.5f;
            const float doorway_pier_half = doorway.params[ArchIdx::THICKNESS].mean * 0.5f
                + doorway.params[ArchIdx::PIER_PADDING].mean
                + doorway.params[ArchIdx::EDGE_BLEND].mean;
            WALL_MARGIN = INDOOR_ENTITY_WALL_MARGIN
                + doorway_half_span + doorway_pier_half;
        }
        else {
            WALL_MARGIN = 8.0f;
        }

        constexpr float MIN_FROM_ORIGIN = 30.0f;
        constexpr float MIN_FROM_ORIGIN_SQ = MIN_FROM_ORIGIN * MIN_FROM_ORIGIN;

        struct Spot { float x, z, rotation; };
        Spot candidates[4] = {
            { room_center,        bmin + WALL_MARGIN,  1.5708f  },  // south, faces +Z
            { bmax - WALL_MARGIN, room_center,         3.14159f },  // east,  faces -X
            { room_center,        bmax - WALL_MARGIN, -1.5708f  },  // north, faces -Z
            { bmin + WALL_MARGIN, room_center,         0.0f     },  // west,  faces +X
        };

        // Fisher–Yates on the side order, seeded from the world seed.
        uint32_t order[4] = { 0, 1, 2, 3 };
        for (uint32_t i = 3; i > 0; i--) {
            uint32_t j = cpu_hash(c->world_state_.active_seed, 6600u + i) % (i + 1);
            uint32_t tmp = order[i]; order[i] = order[j]; order[j] = tmp;
        }

        bool placed = false;
        float chosen_rotation = 0.0f;
        for (uint32_t k = 0; k < 4 && !placed; k++) {
            uint32_t side = order[k];
            const auto& cand = candidates[side];
            // Jitter along the wall (perpendicular to its inward normal).
            float jitter = (cpu_hash_f(c->world_state_.active_seed, 6610u + side) - 0.5f) * room_half * 0.4f;
            float x = cand.x, z = cand.z;
            if (side == 0 || side == 2) x += jitter;  // S/N walls run along X
            else                          z += jitter; // E/W walls run along Z

            if (x * x + z * z >= MIN_FROM_ORIGIN_SQ) {
                c->backPortalPosition_[0] = x;
                c->backPortalPosition_[1] = z;
                chosen_rotation = cand.rotation;
                placed = true;
            }
        }
        if (!placed) {
            uint32_t side = order[0];
            c->backPortalPosition_[0] = candidates[side].x;
            c->backPortalPosition_[1] = candidates[side].z;
            chosen_rotation = candidates[side].rotation;
        }

        const auto& retMood = MOOD_TABLE[c->mood_state_.back_portal_return_mood % MOOD_COUNT];
        PortalDestination dest{};
        dest.seed = c->mood_state_.back_portal_return_seed;
        dest.finite = retMood.finite;
        dest.finite_radius = c->mood_state_.back_portal_return_radius;
        dest.mood = c->mood_state_.back_portal_return_mood;

        float cx = c->backPortalPosition_[0];
        float cz = c->backPortalPosition_[1];
        uint32_t slot = force_spawn_portal_at(c, queue, cx, cz, chosen_rotation, dest, true, machine_ctx);

        if (slot != UINT32_MAX) {
            std::cout << "[Portal] Back-portal spawned at (" << cx << "," << cz
                << ") rot=" << chosen_rotation << " slot=" << slot
                << " -> return seed=" << c->mood_state_.back_portal_return_seed
                << " mood=" << mood_name(c->mood_state_.back_portal_return_mood) << "\n";
        }
        else {
            std::cout << "[Portal] WARNING: no free arch slot for back-portal\n";
        }

        // Spawn additional forward portals around the room perimeter
        force_spawn_finite_portals(c, queue, machine_ctx);
        return;
    }

    // ─── Non-finite fallback (open-world back-portal) ───────────────
    // Open worlds don't normally request back-portals, but if they do
    // we keep the legacy fixed-position behavior at backPortalPosition_.
    const auto& retMood = MOOD_TABLE[c->mood_state_.back_portal_return_mood % MOOD_COUNT];
    PortalDestination dest{};
    dest.seed = c->mood_state_.back_portal_return_seed;
    dest.finite = retMood.finite;
    dest.finite_radius = c->mood_state_.back_portal_return_radius;
    dest.mood = c->mood_state_.back_portal_return_mood;

    float cx = c->backPortalPosition_[0];
    float cz = c->backPortalPosition_[1];
    uint32_t slot = force_spawn_portal_at(c, queue, cx, cz, 0.0f, dest, true, machine_ctx);

    if (slot != UINT32_MAX) {
        std::cout << "[Portal] Back-portal spawned at (" << cx << "," << cz
            << ") slot=" << slot
            << " -> return seed=" << c->mood_state_.back_portal_return_seed
            << " mood=" << mood_name(c->mood_state_.back_portal_return_mood) << "\n";
    }
    else {
        std::cout << "[Portal] WARNING: no free arch slot for back-portal\n";
    }

    // Spawn additional forward portals around the room perimeter
    force_spawn_finite_portals(c, queue, machine_ctx);
}

// ── force_spawn_finite_portals ──
//
inline void force_spawn_finite_portals(MoodDeps* c, wgpu::Queue& queue, MachineCtx& machine_ctx) {
    float bmin = -(float)c->world_state_.finite_radius * Dim::PATCH_EXTENT;
    float bmax = ((float)c->world_state_.finite_radius + 1.0f) * Dim::PATCH_EXTENT;
    float room_center = (bmin + bmax) * 0.5f;
    float room_half = (bmax - bmin) * 0.5f;

    float margin;
    if (MOOD_TABLE[c->mood_state_.active].indoor) {
        const auto& doorway = ARCH_TIERS[static_cast<uint32_t>(ArchTier::DOORWAY)].profile;
        const float doorway_half_span = doorway.params[ArchIdx::SPAN].mean * 0.5f;
        const float doorway_pier_half = doorway.params[ArchIdx::THICKNESS].mean * 0.5f
            + doorway.params[ArchIdx::PIER_PADDING].mean
            + doorway.params[ArchIdx::EDGE_BLEND].mean;
        margin = INDOOR_ENTITY_WALL_MARGIN
            + doorway_half_span + doorway_pier_half;
    }
    else {
        margin = 8.0f;
    }

    uint32_t count = 1;
    if (c->world_state_.finite_radius >= 2) count = 2;
    if (c->world_state_.finite_radius >= 3) count = 3;

    // Perimeter positions: distribute along the 4 walls
    struct PortalSpot {
        float x, z, rotation;
    };

    // Fixed candidate positions: one per wall, offset from center
    PortalSpot candidates[] = {
        // South wall, facing +Z (into room)
        { room_center, bmin + margin, 1.5708f },
        // East wall, facing -X (into room)
        { bmax - margin, room_center, 3.14159f },
        // North wall, facing -Z (into room)
        { room_center, bmax - margin, -1.5708f },
        // West wall, facing +X (into room)
        { bmin + margin, room_center, 0.0f },
    };
    uint32_t num_candidates = 4;

    // Shuffle candidates with seed so different rooms use different walls
    for (uint32_t i = num_candidates - 1; i > 0; i--) {
        uint32_t j = cpu_hash(c->world_state_.active_seed, 7700u + i) % (i + 1);
        PortalSpot tmp = candidates[i];
        candidates[i] = candidates[j];
        candidates[j] = tmp;
    }

    uint32_t spawned = 0;
    for (uint32_t i = 0; i < num_candidates && spawned < count; i++) {
        auto& spot = candidates[i];

        // Jitter position along the wall
        float jitter = (cpu_hash_f(c->world_state_.active_seed, 7710u + i) - 0.5f) * room_half * 0.4f;
        // Apply jitter perpendicular to the wall normal
        float jx = spot.x, jz = spot.z;
        if (std::abs(spot.rotation - 1.5708f) < 0.1f || std::abs(spot.rotation + 1.5708f) < 0.1f) {
            jx += jitter;  // south/north wall: jitter along X
        }
        else {
            jz += jitter;  // east/west wall: jitter along Z
        }

        // Don't collide with back-portal (at backPortalPosition_)
        float dbx = jx - c->backPortalPosition_[0];
        float dbz = jz - c->backPortalPosition_[1];
        if (dbx * dbx + dbz * dbz < 10.0f * 10.0f) continue;

        // Generate destination
        uint32_t dest_seed = cpu_hash(c->world_state_.active_seed, 7800u + i);
        uint32_t mood = pick_portal_mood(&machine_ctx, c->world_state_.active_seed, 7900u + i);
        const auto& mp = MOOD_TABLE[mood];
        PortalDestination dest{};
        dest.seed = dest_seed;
        dest.mood = mood;
        dest.finite = mp.finite;
        dest.finite_radius = derive_finite_radius(dest_seed, mp);

        uint32_t slot = force_spawn_portal_at(c, queue, jx, jz, spot.rotation, dest, false, machine_ctx);
        if (slot != UINT32_MAX) {
            std::cout << "[Portal] Forward portal " << (spawned + 1)
                << " at (" << jx << "," << jz
                << ") -> seed=" << dest_seed
                << " mood=" << mood_name(mood)
                << (dest.finite ? " FINITE" : " open") << "\n";
            spawned++;
        }
    }

    std::cout << "[Portal] Finite world: " << spawned << " forward portals + 1 back-portal\n";
}

// ═══ PER-FRAME UPLOAD ════════════════════════════════════════════

// ── upload_portal_array ──
inline void upload_portal_array(MoodDeps* c, wgpu::Queue& queue) {
    if (!c->mood_state_.portals_dirty) return;
    c->mood_state_.portals_dirty = false;

    c->cpuPortalArray_ = GPUPortalArray{};
    uint32_t count = 0;
    for (uint32_t i = 0; i < Dim::MAX_ARCH_INSTANCES && count < MAX_GPU_PORTALS; i++) {
        if (!c->entities_state_.arches[i].active || !c->entities_state_.arches[i].is_portal) continue;
        const auto& aa = c->entities_state_.arches[i];
        auto& entry = c->cpuPortalArray_.portals[count];
        entry.x = aa.world_x;
        entry.z = aa.world_z;
        entry.facing_cos = std::cos(aa.rotation);
        entry.facing_sin = std::sin(aa.rotation);
        float half_depth = aa.depth * 0.5f;
        entry.inv_span_sq = 1.0f / (aa.half_span * aa.half_span);
        entry.inv_depth_sq = 1.0f / (half_depth * half_depth);
        entry.arch_index = i;
        entry._pad = 0;
        count++;
    }
    c->cpuPortalArray_.count = count;
    c->gpuState_.upload_portal_array(queue, c->cpuPortalArray_);
}

// ── upload_lights ──
// (Must precede compute for shadow VP.)
inline void upload_lights(MoodDeps* c, wgpu::Queue& queue) {
    if (!c->mood_state_.lights_dirty) return;
    c->mood_state_.lights_dirty = false;

    GPUDirectionalLight sun{};
    float len = std::sqrt(c->sunDirection_[0] * c->sunDirection_[0] + c->sunDirection_[1] * c->sunDirection_[1] + c->sunDirection_[2] * c->sunDirection_[2]);
    sun.direction[0] = c->sunDirection_[0] / len;
    sun.direction[1] = c->sunDirection_[1] / len;
    sun.direction[2] = c->sunDirection_[2] / len;

    sun.color[0] = c->sunColor_[0];
    sun.color[1] = c->sunColor_[1];
    sun.color[2] = c->sunColor_[2];
    sun.intensity = c->mood_state_.sun_intensity;
    sun.ambient = c->mood_state_.sun_ambient;

    c->gpuState_.upload_directional_light(queue, sun);

    GPUPointLightArray pointLights{};
    pointLights.count = 0;
    c->gpuState_.upload_point_lights(queue, pointLights);

    c->gpuState_.upload_spot_lights(queue, c->cpuSpotLights_);
}

// ═══ MOOD TRANSITION REQUEST ═════════════════════════════════════
//
inline void request_mood_transition(TransitionPhase& phase, PortalDestination& pending,
    MoodState& ms, const WorldState& ws, uint32_t mood) {
    // ROSTER-GATE transitions (b) — ENTRY door #1 (keyboard mood requests).
    // Disabled: the machine stays at IDLE forever; the bit gates requests,
    // nothing structural. Maturity-proof form of the transitions=>portal
    // edge (the v0 form is the manifest static_assert).
    if constexpr (!ROSTER.transitions) { (void)phase; (void)pending; (void)ms; (void)ws; (void)mood; return; }
    if (phase != TransitionPhase::IDLE) return;
    if (mood >= MOOD_COUNT) return;

    const auto& mp = MOOD_TABLE[mood];
    uint32_t dest_seed = cpu_hash(ws.active_seed, 999u);
    uint32_t radius = derive_finite_radius(dest_seed, mp);
    pending = { dest_seed, mp.finite, radius, mood };
    phase = TransitionPhase::FADE_OUT;
    ms.transition_timer = 0.0f;

    if (mp.finite) {
        uint32_t side = 2 * radius + 1;
        std::cout << "[World] Transition (" << mood_name(mood) << " "
            << side << "x" << side << "): seed " << ws.active_seed
            << " -> " << pending.seed << "\n";
    } else {
        std::cout << "[World] Transition (" << mood_name(mood) << "): seed "
            << ws.active_seed << " -> " << pending.seed << "\n";
    }
}

// ═══ DERIVERS ════════════════════════════════════════════════════

inline const char* mood_name(uint32_t mood) {
    // Sized array: the compiler catches an EXTRA entry past
    // MOOD_COUNT, but not a missing one — it zero-fills to nullptr.
    static const char* NAMES[MOOD_COUNT] = {
        "open_sunset", "indoor_flat", "indoor_vault", "finite_outdoor"
    };
    return (mood < MOOD_COUNT) ? NAMES[mood] : "unknown";
}

// Derive finite world radius from seed within mood-defined bounds.
inline uint32_t derive_finite_radius(uint32_t seed, const MoodProfile& mood) {
    if (mood.finite_radius_min >= mood.finite_radius_max) return mood.finite_radius_min;
    uint32_t range = mood.finite_radius_max - mood.finite_radius_min + 1;
    return mood.finite_radius_min + cpu_hash(seed, 77u) % range;
}

inline uint32_t pick_portal_mood(MachineCtx* c, uint32_t seed, uint32_t prop) {
    float roll = cpu_hash_f(seed, prop);
    if (c->world_state_.finite_mode) {
        if (roll < 0.40f) return 0;   // open_sunset    — the way out
        if (roll < 0.55f) return 1;   // indoor_flat
        if (roll < 0.70f) return 2;   // indoor_vault
        return 3;                     // finite_outdoor
    }
    return cpu_hash(seed, prop) % MOOD_COUNT;
}

} // namespace the_board
} // namespace t7
