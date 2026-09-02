#pragma once
// ─── entity_types.hpp ────────────────────────────────────────────
//
// Type definitions for the generic entity pipeline: pure declarations,
// no functions, no coupling beyond the MachineCtx face the adapter
// signatures name.
//
// THE CONTRACT HOME: pipeline contracts, the boundary DTOs (a DTO
// that exists to cross a boundary belongs to the boundary's contract,
// not to either side — the program theory), and the dispatch contract (the
// queue-entry unions + row type + table declaration). The DTOs are
// plain aggregates of built-ins, so this header carries no owner
// vocabulary and every owner header may include it.
//
// SEAM[entity_types:P9] this file is the canonical home of pattern
//   P9 (type definitions extracted to header-style file) — a real
//   file-scope header. Pure declarations; the implementations they
//   describe live in machine/entity_pipeline.hpp (generic functions, family
//   data, adapters, dispatch wrappers). Same family as seed_utils (P9
//   instance for hashing primitives).
// Tier sampling profiles + extras live in per-family TierRow structs
//   in machine/entity_pipeline.hpp, reached via adapter.get_tier_profile —
//   not on EntityFamilyTraits.
// ─────────────────────────────────────────────────────────────────

#include <cstdint>
#include <cstring>                                        // std::memset (queue-entry ctors)
#include "cartridges/the_board/contracts/roster.hpp"                // PopFamily (sizes the dispatch table)
#include "cartridges/the_board/contracts/wgpu_fwd.hpp"   // wgpu handle fwds (lockstep insurance)

namespace t7 {
namespace the_board {

// ═══ THE MACHINE FACE ═════════════════════════════════════════════
// ONE declared struct carrying what the machine may hand a family —
// the requirements face made literal at the machine's boundary.
// Members are NAMED AS THE ORGANS: the deduced-C conversion leaves
// every row and verb body byte-identical (the template doorway's
// escape clause, executed). References bound ONCE at the root.
// THE CONST TRIO is the arrow law's compiler teeth at this boundary:
// the machine reads the surface only through the m3b faces, reads
// the clock, reads the witness — and can write none of them.
struct WorldState;            struct TileWorldState;
struct ThemesState;           struct MoodState;
struct PatchSystemState;      struct SpawnEngineState;
struct EntitiesState;         struct SphereState;
struct CubeBehaviorsState;    struct RibbonState;
struct GoLState;              struct GalleryState;
struct TimeState;             struct PlayerState;
struct CameraPose;
struct PointState;
class GPUState;               class Renderer;

struct MachineCtx {
    // S1/S2 — the surface the machine stands on
    WorldState&              world_state_;
    const TileWorldState&    tile_world_state_;    // const: only the m3b faces consume it
    const ThemesState&       themes_state_;        // const: the preamble reads the envelope
    MoodState&               mood_state_;          // active R; portals_dirty W (the arch channel)
    PatchSystemState&        patch_system_state_;
    SpawnEngineState&        spawn_engine_state_;
    // the family organs the rows own
    EntitiesState&           entities_state_;
    SphereState&             sphere_state_;
    CubeBehaviorsState&      cube_behaviors_state_;
    RibbonState&             ribbon_state_;
    GoLState&                gol_state_;
    GalleryState&            gallery_state_;
    // clock + witness (read-only by census)
    const TimeState&         time_state_;
    const PlayerState&       player_;
    const PointState&        point_;         // THE POINT's house (POINT_1): position mirror + bubble sensor
    // PLUMB_0 C1 (RULING-1) — where the eye is and where it looks, one frame
    // stale, filled by the witness harvest. The tide's whole input.
    const CameraPose&        camera_pose_;
    // realization
    GPUState&                gpuState_;
    Renderer&                renderer_;
};


// ═══ PIPELINE CONTRACTS ══════════════════════════════════════════

// ── Array bounds ─────────────────────────────────────────────────
inline constexpr uint32_t MAX_ENTITY_PARAMS = 32;
inline constexpr uint32_t MAX_COLOR_CHANNELS = 12;

// ── Sampling distributions ───────────────────────────────────────
enum class ParamDist : uint32_t {
    GAUSSIAN,
    UNIFORM_01,
    UNIFORM_TAU,
};

// ── Param contract ───────────────────────────────────────────────
// One row per parameter the family rolls. Order in the family's
// PARAM_DEFS[] array must match the family's *Idx struct one-to-one.
struct TierParamDef {
    uint32_t   prop;
    float      floor;
    float      ceiling;    // upper clamp (1e30 = no ceiling)
    bool       do_round;
    ParamDist  dist;
};

// ── Gaussian sampling input ──────────────────────────────────────
// Per-parameter (mean, sigma) pair. TierProfile holds one of these
// for every parameter the family rolls, indexed in lockstep with
// the family's PARAM_DEFS[] array.
struct TierMuSigma {
    float mean, sigma;
};

struct TierProfile {
    float          weight;
    float          color_var;     // per-tier scalar color variance; 0 = use ColorPartDef.variance fallback
    TierMuSigma    params[MAX_ENTITY_PARAMS];
};

// ── Color part spec ──────────────────────────────────────────────
//
struct ColorPartDef {
    float    base[3];
    float    variance;
    uint32_t prop_base;
    uint32_t prop_offset;
};

// ═══ FAMILY DESCRIPTION ══════════════════════════════════════════

struct EntityFamilyTraits {
    uint32_t    family_id;
    uint32_t    max_instances;
    bool        grounded;
    uint32_t    spawn_roll_prop;
    float       spawn_chance;
    const float* mood_multiplier;
    float       position_jitter;
    uint32_t    tier_count;
    uint32_t    tier_prop;
    const TierParamDef* param_defs;
    uint32_t    param_count;
    uint32_t    pos_x_prop;
    uint32_t    pos_z_prop;
    uint32_t    rotation_prop;
    uint32_t    color_part_count;
    const ColorPartDef* color_parts;
};

// ═══ PER-INSTANCE + ADAPTER ══════════════════════════════════════

// ── Spawn gate result ────────────────────────────────────────────
// Returned by the family's run_gate adapter. ok=false → early exit
// from generic_select.
struct SpawnGateOutput {
    bool     ok;
    uint32_t seed;
    uint32_t slot;
    uint32_t theme_idx;
};

// ── Per-instance pipeline state ──────────────────────────────────
struct EntityInstance {
    uint32_t family_id = 0;
    uint32_t seed = 0;
    int32_t  trigger_gx = 0, trigger_gz = 0;
    int32_t  host_gx = 0, host_gz = 0;
    uint32_t slot = 0;
    uint32_t tier_idx = 0;
    uint32_t theme_idx = 0;
    float    cx = 0.0f, cz = 0.0f;
    float    rotation = 0.0f;
    float    params[MAX_ENTITY_PARAMS]{};
    float    solid_half = 0.0f;
    float    cached_ground_y = 0.0f;
    float    ground_y_offset = 0.0f;  // added to terrain Y (e.g. solid_height for pier entities)
    float    burial = 0.0f;
    float    colors[MAX_COLOR_CHANNELS]{};
    uint32_t mosaic_seed = 0;   // MOSAIC_1: 0 = plain; 1..65535 rides the index channel
};

// ── Per-family adapter ───────────────────────────────────────────
//
struct EntityFamilyAdapter {
    SpawnGateOutput(*run_gate)(MachineCtx* c, int32_t gx, int32_t gz);
    // Q5: the per-family get_theme_tier_weights fn-ptr is gone — tier weights
    // now come from the ONE theme_tier_weights(theme_idx, traits.family_id)
    // accessor (population_themes.hpp), keyed on the family the adapter's
    // traits already carry. No per-family plug needed.
    void (*apply_indoor_rescale)(EntityInstance& inst, float ceiling_h);
    void (*compute_solid_half)(EntityInstance& inst, const TierProfile& tier);
    void (*compute_colors)(EntityInstance& inst, const EntityFamilyTraits& traits, const TierProfile& tier);
    void (*write_active)(MachineCtx* c, const EntityInstance& inst);
    void (*write_gpu)(MachineCtx* c, const EntityInstance& inst, wgpu::Queue& queue);
    void (*post_commit)(MachineCtx* c, const EntityInstance& inst, wgpu::Queue& queue);
    const TierProfile& (*get_tier_profile)(uint32_t tier_idx);
};

// ═══ DISPATCH CONTRACT (queue entries + row type) ═════════════════

// ═══ SPAWN PAYLOADS (the boundary DTOs) ═══════════════════════════
//
// The three bespoke families' Selection/Placement DTOs. They exist
// to cross the machine/owner boundary inside EntityQueueEntry /
// PlacementEntry (below) — and a DTO that exists to cross a boundary
// belongs to the boundary's contract, not to either side. Plain
// aggregates of built-ins, by design: the contract home carries no
// owner vocabulary.

// ── GoL (zone lattice; Conway/Pulse) ──────────────────────────────
struct GoLSelection {
    uint32_t seed;
    int32_t  trigger_gx, trigger_gz;
    uint32_t slot;
    int32_t  zone_nx, zone_nz;     // lattice node
    float    corner_x, corner_z;   // zone corner (cell-grid-snapped)
    uint32_t algorithm;            // AlgorithmType::CONWAY or PULSE
    uint32_t tier_idx;             // compound: Conway 0..GOL_TIER_COUNT-1,
                                   // Pulse GOL_TIER_COUNT.. (gol_tier_cells decodes)
    float    tick_period;
    float    initial_density;
    bool     height_enabled;
    float    footprint_r;
};

struct GoLPlacement {
    uint32_t slot;
    int32_t  trigger_gx, trigger_gz;
    int32_t  host_gx, host_gz;
    uint32_t tier_idx;
    float    cx, cz;               // zone center
    int32_t  zone_nx, zone_nz;
    float    corner_x, corner_z;
    uint32_t algorithm;
    float    tick_period;
    float    initial_density;
    bool     height_enabled;
};

// ── Gallery (outdoor art exhibitions — composite: 1 center → N paintings) ──
struct GallerySelection {
    uint32_t seed;
    int32_t  trigger_gx, trigger_gz;
    uint32_t slot;              // gallery center slot
    float    cx, cz;            // gallery center (jittered)
    uint32_t archetype;         // 0–3 (terrain type, used as tier_idx)
    uint32_t painting_count;    // seed-derived WISH; place reserves against content
    float    facing_angle;
    float    gallery_size_mean;
    uint32_t site_type;         // 0=snapshot, 1=mixed, 2=authored
};

struct GalleryPlacement {
    uint32_t slot;
    int32_t  trigger_gx, trigger_gz;
    int32_t  host_gx, host_gz;
    uint32_t tier_idx;          // = archetype
    float    cx, cz;
    float    footprint_r;
    uint32_t archetype;
    uint32_t painting_count;
    // The RESERVATION: how many staging layers place claimed for this gallery.
    // Commit draws from this instead of re-discovering scarcity, and the
    // footprint radius above was computed from it. Plain built-in by design —
    // the contract home carries no owner vocabulary.
    uint32_t reserved_count;
    float    facing_angle;
    float    gallery_size_mean;
    uint32_t site_type;
};

// ── Ribbon ─────────────────────────────────────────────────────────
struct RibbonSelection {
    uint32_t seed;
    int32_t  trigger_gx, trigger_gz;
    uint32_t slot;
    uint32_t tier_idx;
    // Geometry (from select_ribbon_for_patch)
    uint32_t cube_count;
    float cube_size;
    float height;
    float orientation;
    float lateral_amp, lateral_cycles;
    float vertical_amp;
    // Color
    uint32_t color_mode;
    float color[3];
    float color_b[3];        // CONTRAST second median
    float checker_scatter = 0.0f;
    float checker_hue_spread = 0.0f;
    // Footprint
    float footprint_r;
};

struct RibbonPlacement {
    uint32_t slot;
    int32_t  trigger_gx, trigger_gz;
    int32_t  host_gx, host_gz;
    uint32_t tier_idx;
    float cx, cz;
    // Geometry (copied from selection)
    uint32_t cube_count;
    float cube_size, height, orientation;
    float lateral_amp, lateral_cycles;
    float vertical_amp;
    uint32_t color_mode;
    float color[3];
    float color_b[3];        // CONTRAST second median
    float checker_scatter = 0.0f;
    float checker_hue_spread = 0.0f;
    uint32_t seed = 0u;   // spawn seed, carried so commit samples its channels
};

// ─── Entity Selection Queue entry ─────────────────────────────────

struct EntityQueueEntry {
    uint32_t family;    // PopFamily index
    int32_t  gx, gz;    // trigger patch (for commit bookkeeping)
    union {
        RibbonSelection ribbon;
        GoLSelection    gol;
        GallerySelection gallery;
        EntityInstance   generic;    // used by all 9 generic-pipeline families
    };
    EntityQueueEntry() : family(0), gx(0), gz(0) { std::memset(&generic, 0, sizeof(generic)); }
};

// ─── Placement Results entry ───────────────────────────────────────

struct PlacementEntry {
    uint32_t family;
    int32_t  gx, gz;
    union {
        RibbonPlacement ribbon;
        GoLPlacement    gol;
        GalleryPlacement gallery;
        EntityInstance   generic;    // used by all 9 generic-pipeline families
    };
    PlacementEntry() : family(0), gx(0), gz(0) { std::memset(&generic, 0, sizeof(generic)); }
};

// ─── Dispatch row type ─────────────────────────────────────────────
//
// One row per family: the six verbs the spine's dispatch loops call
// through, plus the census accessor. The table itself (FAMILY_DISPATCH)
// is the spine's integration hub (SEAM[spine:owns] at its banner in
// cartridge.hpp).
//
// active_count is the only row member that is not a verb the pipeline
// drives: it is the census's per-family view of "how many slots are
// live", answered by scanning the family's own array. It takes the
// machine face CONST — the census reads and never writes, and the
// signature says so.

// ─── SlotCensus (ARCH_2) — the occupancy triple ────────────────────
//
// WHY A TRIPLE AND NOT A SECOND COUNTER: `live` alone cannot say
// whether a family is SATURATED, because the census has no denominator
// — 16 arches reads the same as 3 until you know the array holds 16.
// That distinction is the whole question the tier weights turn on: at a
// fixed SPAWN_CHANCE the three ArchTiers are zero-sum in ABSOLUTE
// counts, so if the array is already pinned at its ceiling, reweighting
// tiers only decides WHICH bodies win a first-come race for slots — it
// cannot add bodies. Reading a weight change as a population change is
// the error this triple exists to make impossible.
//
// capacity is DEDUCED from the array by the scanning template, never
// written — the same rule the active_count row already obeys, and for
// the same three reasons stated at its banner in cartridge.hpp.
//
// high_water is the ALLOCATOR'S REACH: one past the highest live slot,
// exact per scan, not a running maximum. It is not a session peak and
// must not be read as one — nothing here stores state between dumps.
// Against `live` it separates two saturation stories a single number
// cannot: live == capacity is a full array; high_water == capacity with
// live below it is an array that has BEEN full and now carries holes,
// which is what a population cycling against its ceiling looks like
// when the census happens to sample it mid-breath.
struct SlotCensus {
    uint32_t live;         // bodies with .active set
    uint32_t high_water;   // one past the highest live slot (this scan)
    uint32_t capacity;     // the array's bound — deduced, never written
};

struct FamilyDispatch {
    bool (*try_select)(MachineCtx* self, int32_t gx, int32_t gz, EntityQueueEntry& e);
    bool (*try_place)(MachineCtx* self, EntityQueueEntry& e, PlacementEntry& pe);
    void (*try_commit)(MachineCtx* self, PlacementEntry& pe, wgpu::Queue& queue);
    void (*evict_slot)(MachineCtx* self, uint32_t slot, wgpu::Queue& queue);
    bool (*prepare_mesh)(MachineCtx* self, wgpu::Queue& queue);
    void (*dispatch_mesh)(MachineCtx* self, wgpu::ComputePassEncoder& pass);
    uint32_t (*active_count)(const MachineCtx* self);
    // The occupancy triple, same scan discipline as active_count and the
    // same CONST face. Kept a SEPARATE row rather than folded into
    // active_count: active_count has eleven callers' worth of settled
    // meaning as a plain population, and the census's own delta column is
    // built from it. A diagnostic does not get to change the shape of the
    // number the leak check is computed from.
    SlotCensus (*slot_census)(const MachineCtx* self);
    // Does this family claim ground? (ruling 21 — the campaign law is "a
    // family registers iff its own extent touches the ground plane".) The
    // census needs it for all TWELVE families, and ribbon/gol/gallery have no
    // EntityFamilyTraits at all — so the answer cannot live in the traits
    // alone. The nine generic rows initialise this FROM their own
    // <FAMILY>_TRAITS.grounded, making the row a view of the authored field
    // rather than a second copy of the policy; the three bespoke rows state
    // it here because here is the only place they can.
    bool grounded;
    const char* name;
};

extern const FamilyDispatch FAMILY_DISPATCH[PopFamily::COUNT];

} // namespace the_board
} // namespace t7

// ═══ END entity_types.hpp ════════════════════════════════════════
