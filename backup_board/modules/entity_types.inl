// ─── entity_types.inl ────────────────────────────────────────────
//
// Type definitions for the generic entity pipeline. Header-style
// file: pure declarations, no functions, no class-body coupling
// beyond the Cartridge forward reference in adapter signatures.
//
// ┌─── Public surface (consumed by other files) ────────────────────┐
// │                                                                  │
// │  Pipeline contracts:                                             │
// │    MAX_ENTITY_PARAMS       — params[] array length               │
// │    MAX_COLOR_CHANNELS      — colors[] array length               │
// │    ParamDist (enum)        — sampling distribution               │
// │    TierParamDef            — one param's contract                │
// │    TierMuSigma, TierProfile — Gaussian sampling input            │
// │    ColorPartDef            — one color part's spec               │
// │                                                                  │
// │  Family description:                                             │
// │    EntityFamilyTraits      — declarative family metadata         │
// │                                                                  │
// │  Per-instance + adapter:                                         │
// │    EntityInstance          — one rolled instance, pipeline state │
// │    SpawnGateOutput         — gate result                         │
// │    EntityFamilyAdapter     — function-pointer table per family   │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// Included inside the Cartridge class body, mid-block from
// spawn_engine.inl (see SEAM[spawn_engine:structural] for the C++
// union-member-ordering reason). Do not move this include site
// without resolving the union ordering constraint.
//
// SEAM[entity_types:P9] this file is the canonical home of pattern
//   P9 (type definitions extracted to header-style file). Pure
//   declarations; the implementations they describe live in
//   entity_pipeline.inl (generic functions, family data, adapters,
//   dispatch wrappers). Same family as seed_utils.inl (P9 instance
//   for hashing primitives).
// DONE[entity_types:K1] tier sampling profile + extras moved off
//   EntityFamilyTraits and into per-family TierRow structs in
//   entity_pipeline.inl, reached via adapter.get_tier_profile.
//   Cleared the breadcrumb from the struct definitions; the migration
//   is the convention.
// ─────────────────────────────────────────────────────────────────


// ═══ PIPELINE CONTRACTS ══════════════════════════════════════════
//
// Numerical contracts and sampling primitives shared across every
// generic-pipeline family. These define the shape of the pipeline
// interface — touch only with intent.

// ── Array bounds ─────────────────────────────────────────────────
static constexpr uint32_t MAX_ENTITY_PARAMS = 32;
static constexpr uint32_t MAX_COLOR_CHANNELS = 12;

// ── Sampling distributions ───────────────────────────────────────
// Determines how a TierParamDef's `prop` is rolled. GAUSSIAN draws
// from the per-tier (mean, sigma); UNIFORM_01 returns hash(seed,
// prop) directly; UNIFORM_TAU returns the same scaled to [0, 2π).
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
// One row per coloured "part" of the family (body / aged / trunk /
// frond / etc.). Variance is rolled from prop_base + prop_offset
// triplet.
//
// The `variance` field is the per-part fallback. If the active
// TierProfile carries a nonzero `color_var`, that overrides this
// per-part value uniformly across all parts. Families that need
// per-part-per-tier variance (Palm) keep their own override.
struct ColorPartDef {
    float    base[3];
    float    variance;
    uint32_t prop_base;
    uint32_t prop_offset;
};


// ═══ FAMILY DESCRIPTION ══════════════════════════════════════════
//
// Declarative metadata describing a generic-pipeline family. One
// of these is constructed per family in entity_pipeline.inl as a
// `static constexpr <FAMILY>_TRAITS` and passed to generic_select
// / generic_place / generic_commit.

struct EntityFamilyTraits {
    uint32_t    family_id;
    const char* short_name;
    uint32_t    max_instances;
    bool        grounded;
    bool        creates_ground;
    uint32_t    piers_per_entity;
    bool        has_footprint;
    float       cull_base;
    float       cull_height_scale;
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
    bool        gpu_ground_y;       // true = GPU compute corrects ground_y (CPU uploads offset only)
    uint32_t    color_part_count;
    const ColorPartDef* color_parts;
};


// ═══ PER-INSTANCE + ADAPTER ══════════════════════════════════════
//
// What flows through the three-phase pipeline (EntityInstance) and
// how each family customizes the generic steps (EntityFamilyAdapter).

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
// One of these per spawning entity. Populated incrementally:
//   generic_select  → family_id, seed, trigger_*, slot, tier_idx,
//                     theme_idx, params[], colors[], solid_half
//   generic_place   → cx, cz, rotation, host_*
//   generic_commit  → consumed by adapter.write_active / write_gpu
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
};

// ── Per-family adapter ───────────────────────────────────────────
// Function-pointer table. Each family in entity_pipeline.inl
// constructs one as a `static constexpr <FAMILY>_ADAPTER`.
//
// get_tier_profile is per-family because the TierProfile lives
// embedded in each family's per-family TierRow struct (one source
// of truth — there's no generic table on traits to index).
struct EntityFamilyAdapter {
    SpawnGateOutput(*run_gate)(Cartridge* c, int32_t gx, int32_t gz);
    const float* (*get_theme_tier_weights)(uint32_t theme_idx);
    void (*apply_indoor_rescale)(EntityInstance& inst, float ceiling_h);
    void (*compute_solid_half)(EntityInstance& inst, const TierProfile& tier);
    void (*compute_colors)(EntityInstance& inst, const EntityFamilyTraits& traits, const TierProfile& tier);
    void (*write_active)(Cartridge* c, const EntityInstance& inst);
    void (*write_gpu)(Cartridge* c, const EntityInstance& inst, wgpu::Queue& queue);
    void (*post_commit)(Cartridge* c, const EntityInstance& inst, wgpu::Queue& queue);
    const TierProfile& (*get_tier_profile)(uint32_t tier_idx);
};

// ═══ END entity_types.inl ════════════════════════════════════════
