// ─── entity_pipeline.inl ─────────────────────────────────────────
//
// Generic entity lifecycle for the seven cookie-cutter families
// (Blade, Palm, Cactus, Column+Antenna, Pyramid, Sphere, Cube,
// Arch). Three generics drive every spawn:
//
//   generic_select  → tier roll, parameter sample, color compute
//   generic_place   → footprint negotiation, position commit
//   generic_commit  → active-array write, GPU upload, post-commit
//
// Each family contributes a block with the same 10-element template
// (see "Family block template" below). Type definitions live in
// entity_types.inl, which is included before the union in
// spawn_engine.inl. This file is included AFTER the unions.
//
// ┌─── Public surface ──────────────────────────────────────────────┐
// │                                                                  │
// │  Generics (called by FAMILY_DISPATCH wrappers below):            │
// │    generic_select(traits, adapter, gx, gz, inst)                 │
// │    generic_place(traits, inst)                                   │
// │    generic_commit(traits, adapter, inst, queue)                  │
// │                                                                  │
// │  Helpers used by generic_select:                                 │
// │    generic_compute_colors(inst, traits, tier)                    │
// │      — default color path; reads TierProfile.color_var if set,   │
// │        else falls back to ColorPartDef.variance (closes Q24)     │
// │    rescale_to_rolled_target(inst, ceiling_h, lo, hi, current_h,  │
// │                              params_to_scale)                    │
// │      — shared helper for the rolled-target rescale pattern;      │
// │        used by Palm / Pyramid / Arch / Antenna's per-family      │
// │        apply_indoor_rescale functions                            │
// │                                                                  │
// │  Per-family dispatch wrappers (consumed by FAMILY_DISPATCH in    │
// │  cartridge.hpp — three per family, eight families):              │
// │    dispatch_{select,place,commit}_<family>_generic(...)          │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// ┌─── Family block template ───────────────────────────────────────┐
// │                                                                  │
// │  Each family's section follows this 10-element shape:            │
// │                                                                  │
// │    1. <Family>Idx              param indices into params[]       │
// │    2. <FAMILY>_PARAM_DEFS      param contracts (prop, floor,     │
// │                                ceiling, dist)                    │
// │    3. <Family>TierRow          TierProfile + per-family extras   │
// │    4. <FAMILY>_TIERS           tier matrix                       │
// │    5. <family>_get_tier_profile  accessor                        │
// │    6. <FAMILY>_COLOR_PARTS     color part definitions            │
// │    7. <FAMILY>_TRAITS          binds everything for the generic  │
// │                                pipeline                          │
// │    8. Adapter functions        run_gate, get_theme_tier_weights, │
// │                                apply_indoor_rescale (or nullptr  │
// │                                if not eligible for indoor),      │
// │                                compute_solid_half,               │
// │                                compute_colors (or nullptr → use  │
// │                                generic_compute_colors),          │
// │                                write_active, write_gpu,          │
// │                                post_commit (optional)            │
// │    9. <FAMILY>_ADAPTER         function-pointer table            │
// │   10. Dispatch wrappers        three per family                  │
// │                                                                  │
// │  Don't fight the cookie-cutter — it's intentional specificity    │
// │  per family, and the parallel structure makes the eight blocks   │
// │  scannable side-by-side.                                         │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// Included inside the Cartridge class body, AFTER the EntityQueueEntry
// and PlacementEntry unions are declared in spawn_engine.inl.
// Depends on: entity_types.inl (declarations), seed_utils.inl
//             (cpu_hash_f, cpu_sample_gaussian, select_tier_biased),
//             entities.inl (vocabulary: tier enums, prop registries,
//             color palettes, configs).
//
// SEAM[entity_pipeline:K1] tier sampling profile + extras live as a
//   single per-family TierRow struct embedded in this file. Single
//   source of truth — no converters, no derived tables. The legacy
//   *TierParams structs and *_TIERS arrays that used to live in
//   entities.inl are gone (closed: see DONE[entities:K1]).
// ─────────────────────────────────────────────────────────────────


// ═══ GENERIC HELPERS ═════════════════════════════════════════════

// ─── Generic Color Derivation ────────────────────────────────────
//
// Default color computation: for each part, base + hash variance.
// Per-tier color_var (when nonzero) overrides per-part variance —
// closes Q24, retires the Blade/Cactus override pattern.
// Families with exotic color logic (Column, Antenna, Pyramid,
// Sphere, Cube, Arch, Palm) provide their own adapter fn.

static void generic_compute_colors(EntityInstance& inst,
    const EntityFamilyTraits& traits,
    const TierProfile& tier) {
    for (uint32_t p = 0; p < traits.color_part_count; p++) {
        const auto& part = traits.color_parts[p];
        float v = (tier.color_var > 0.0f) ? tier.color_var : part.variance;
        uint32_t ci = p * 3;
        inst.colors[ci + 0] = part.base[0]
            + (cpu_hash_f(inst.seed, part.prop_base + part.prop_offset + 0) - 0.5f) * v;
        inst.colors[ci + 1] = part.base[1]
            + (cpu_hash_f(inst.seed, part.prop_base + part.prop_offset + 1) - 0.5f) * v;
        inst.colors[ci + 2] = part.base[2]
            + (cpu_hash_f(inst.seed, part.prop_base + part.prop_offset + 2) - 0.5f) * v;
    }
}

// ─── Indoor Rescale Helper ───────────────────────────────────────
//
// Indoor moods are walled spaces 20–25 m tall; their largest entity
// budget is the room. Without a rescale, families like Pyramid
// (~28–78 m natural HEIGHT), Antenna (~17–125 m), Palm (~8–35 m)
// would spawn at outdoor scale and punch through the ceiling.
//
// Per-family rescale lives in each family's adapter functions block
// (look for `<family>_apply_indoor_rescale`). Each function defines
// its own policy:
//
//   • Columns: HEIGHT is set to ceiling_height exactly, and every
//     other length param scales by the same ratio so proportions
//     hold. The capital meets the ceiling — the column reads as
//     part of the room's architecture, not a freestanding object.
//
//   • Palms: target rolled in [0.80, 0.95] × ceiling_height. Tighter
//     than the default range — palms read as canopy-defining
//     architectural anchors and look wrong when too small.
//
//   • Pyramid, Arch, Antenna: target rolled in [0.50, 0.95] ×
//     ceiling_height. All length-like params scale by
//     target / current_height. This is the "miniature feeling" —
//     a Royal palm shrunk to 18 m keeps every internal proportion
//     (frond length, taper, bark ring spacing); only the absolute
//     scale changes.
//
// Cactus, Blade, Sphere, Cube are deliberately NOT eligible: their
// natural outdoor heights are below the ceiling, so the rescale
// would scale them UP rather than down — defeating the miniature
// intent. Their adapters have nullptr in the apply_indoor_rescale
// slot and the dispatch site skips the call.
//
// Param indices below are hand-curated per family — only LENGTH
// dimensions get scaled, never ratios (TAPER, ENTASIS, ASPECT...),
// counts (BASE_LAYERS, RIBS, ARM_COUNT...), or angles (LEAN_DIR,
// FROND_DROOP...). Adding a new eligible family means picking which
// pattern it follows, declaring its own
// <family>_apply_indoor_rescale, and registering it in the adapter.
// Property index 7777u is reserved for the rescale-target hash
// (no other family uses it).
//
// SEAM[entity_pipeline:rescale-per-family] DONE — was a free-function
//   switch on family_id; lifted to per-family adapter slot during
//   Pass 7 of the modularity rollout.

// Helper used by Palm / Pyramid / Arch / Antenna for the rolled-
// target rescale pattern. Rolls a target height in [target_lo,
// target_hi] × ceiling_h, computes the scale factor from current_h,
// applies it to every param index in params_to_scale.
//
// Column does NOT use this helper — its policy is "snap to ceiling
// exactly" rather than "roll a target ratio."
//
// The templated array reference avoids needing <initializer_list>;
// each caller passes a constexpr uint32_t array of param indices.
template<size_t N>
static void rescale_to_rolled_target(EntityInstance& inst, float ceiling_h,
    float target_lo, float target_hi, float current_h,
    const uint32_t (&params_to_scale)[N]) {
    if (current_h <= 1e-3f) return;
    constexpr uint32_t RESCALE_TARGET_PROP = 7777u;
    const float t = cpu_hash_f(inst.seed, RESCALE_TARGET_PROP);
    const float target_h = ceiling_h * (target_lo + (target_hi - target_lo) * t);
    const float scale = target_h / current_h;
    for (size_t i = 0; i < N; i++) inst.params[params_to_scale[i]] *= scale;
}

// ═══ GENERIC THREE-PHASE PIPELINE ════════════════════════════════
//
// generic_select → generic_place → generic_commit. These are the
// implementations that all eight family blocks below funnel into
// via their dispatch wrappers.

// ─── Generic Select ──────────────────────────────────────────────
//
// Tier selection + parameter sampling.  Spawn gate is handled by
// the adapter's run_gate, which calls run_spawn_preamble with the
// family's specific active array — exact behavioral parity.
//
// Returns true if entity was successfully selected. On success,
// inst is fully populated except for position (cx, cz, rotation).

bool generic_select(
    const EntityFamilyTraits& traits,
    const EntityFamilyAdapter& adapter,
    int32_t gx, int32_t gz,
    EntityInstance& inst)
{
    // ── Spawn gate (delegates to existing run_spawn_preamble) ──
    auto gate = adapter.run_gate(this, gx, gz);
    if (!gate.ok) return false;

    // ── Tier selection with theme bias ──
    // Tier weights and profiles come from the adapter's per-family
    // accessor; there's no generic table on traits to index.
    float weights[8]{};
    for (uint32_t t = 0; t < traits.tier_count && t < 8; t++)
        weights[t] = adapter.get_tier_profile(t).weight;

    // Apply theme tier weights (per-family array from PopulationTheme)
    const float* theme_tw = adapter.get_theme_tier_weights(gate.theme_idx);
    for (uint32_t t = 0; t < traits.tier_count && t < 8; t++)
        weights[t] *= theme_tw[t];

    uint32_t tier = select_tier_biased(gate.seed, traits.tier_prop,
        weights, traits.tier_count, traits.family_id);
    const auto& profile = adapter.get_tier_profile(tier);

    // ── Sample all parameters from tier profile ──
    for (uint32_t i = 0; i < traits.param_count; i++) {
        const auto& pd = traits.param_defs[i];
        float val = 0.0f;

        switch (pd.dist) {
        case ParamDist::GAUSSIAN:
            val = cpu_sample_gaussian(gate.seed, pd.prop,
                profile.params[i].mean, profile.params[i].sigma);
            break;
        case ParamDist::UNIFORM_01:
            val = cpu_hash_f(gate.seed, pd.prop);
            break;
        case ParamDist::UNIFORM_TAU:
            val = cpu_hash_f(gate.seed, pd.prop) * 6.283185307f;
            break;
        }

        if (pd.do_round) val = std::round(val);
        val = std::max(pd.floor, val);
        if (pd.ceiling < 1e29f) val = std::min(pd.ceiling, val);
        inst.params[i] = val;
    }

    // ── Populate instance header ──
    inst.family_id  = traits.family_id;
    inst.seed       = gate.seed;
    inst.trigger_gx = gx;
    inst.trigger_gz = gz;
    inst.slot       = gate.slot;
    inst.tier_idx   = tier;
    inst.theme_idx  = gate.theme_idx;

    // ── Indoor rescale (must run before compute_solid_half so the
    //    solid extents are derived from the scaled params) ──
    if (MOOD_TABLE[mood_state_.active].indoor && adapter.apply_indoor_rescale) {
        adapter.apply_indoor_rescale(inst, MOOD_TABLE[mood_state_.active].ceiling_height);
    }

    // ── Per-family derived values ──
    adapter.compute_solid_half(inst, profile);
    if (adapter.compute_colors)
        adapter.compute_colors(inst, traits, profile);
    else
        generic_compute_colors(inst, traits, profile);

    return true;
}

// ─── Generic Place ───────────────────────────────────────────────
//
// Position negotiation: jittered position, footprint check,
// host patch resolution. Sets cx, cz, rotation, host_gx/gz.

bool generic_place(
    const EntityFamilyTraits& traits,
    EntityInstance& inst)
{
    auto pos = negotiate_position(inst.seed,
        inst.trigger_gx, inst.trigger_gz,
        traits.pos_x_prop, traits.pos_z_prop,
        traits.position_jitter,
        traits.rotation_prop,
        inst.solid_half, traits.family_id, inst.tier_idx);
    if (!pos.ok) return false;

    inst.host_gx  = pos.host_gx;
    inst.host_gz  = pos.host_gz;
    inst.cx       = pos.cx;
    inst.cz       = pos.cz;
    inst.rotation = pos.rotation;

    // Ground Y: GPU compute shader samples the heightfield at entity
    // positions. The heightfield includes pier contributions, so entities
    // on piers get the correct elevated Y automatically. CPU uploads 0.
    inst.cached_ground_y = 0.0f;

    record_placement_bookkeeping(traits.family_id, inst.tier_idx);
    return true;
}

// ─── Generic Commit ──────────────────────────────────────────────
//
// GPU writes: active tracking, footprint, GPU params upload.

void generic_commit(
    const EntityFamilyTraits& traits,
    const EntityFamilyAdapter& adapter,
    const EntityInstance& inst,
    wgpu::Queue& queue)
{
    // ── Active tracking (per-family array) ──
    adapter.write_active(this, inst);

    // ── GPU mesh params (per-family struct mapping) ──
    adapter.write_gpu(this, inst, queue);

    // ── Post-commit: piers, regen, portals, etc. ──
    if (adapter.post_commit)
        adapter.post_commit(this, inst, queue);

    world_state_.ground_entries_dirty = true;
}


// ═══ FAMILY: BLADE ════════════════════════════════════════════════
//
// Ground-level leaf clusters.
//


// ─── Blade Parameter Index Registry ──────────────────────────────
// These index into EntityInstance::params[], matching the order
// of BLADE_PARAM_DEFS below.

struct BladeIdx {
    static constexpr uint32_t BLADE_COUNT = 0;
    static constexpr uint32_t BLADE_H     = 1;
    static constexpr uint32_t BLADE_H_VAR = 2;
    static constexpr uint32_t BLADE_W     = 3;
    static constexpr uint32_t SPLAY       = 4;
    static constexpr uint32_t CURVE       = 5;
    static constexpr uint32_t TWIST       = 6;
    static constexpr uint32_t TAPER       = 7;
    static constexpr uint32_t COUNT       = 8;
};

// ─── Parameter Definitions ───────────────────────────────────────
// prop must exactly match BladeProp::{name}, floor must match the
// std::max() in select_blade_for_patch.
//
//                                             prop                    floor   round  dist
static constexpr TierParamDef BLADE_PARAM_DEFS[] = {
    { BladeProp::BLADE_COUNT,                  2.0f, 1e30f,  true,  ParamDist::GAUSSIAN },
    { BladeProp::HEIGHT,                       0.5f, 1e30f,  false, ParamDist::GAUSSIAN },
    { BladeProp::HEIGHT_VAR,                   0.0f, 1e30f,  false, ParamDist::GAUSSIAN },
    { BladeProp::WIDTH,                        0.05f, 1e30f, false, ParamDist::GAUSSIAN },
    { BladeProp::SPLAY,                        0.0f, 1e30f,  false, ParamDist::GAUSSIAN },
    { BladeProp::CURVE,                        0.0f, 1e30f,  false, ParamDist::GAUSSIAN },
    { BladeProp::TWIST,                        0.0f, 1e30f,  false, ParamDist::GAUSSIAN },
    { BladeProp::TAPER,                        0.3f, 1e30f,  false, ParamDist::GAUSSIAN },
};
static constexpr uint32_t BLADE_PARAM_COUNT = sizeof(BLADE_PARAM_DEFS) / sizeof(TierParamDef);

// params[] order MUST match BLADE_PARAM_DEFS:
//   [0]BLADE_COUNT [1]BLADE_H [2]BLADE_H_VAR [3]BLADE_W
//   [4]SPLAY [5]CURVE [6]TWIST [7]TAPER
struct BladeTierRow {
    TierProfile profile;
    float       color_over;
    uint32_t    blade_segs;
};

static constexpr BladeTierRow BLADE_TIERS[] = {
    /* SPROUT  */ {
        { 0.50f, 0.06f, { {3.0f, 0.5f}, {1.8f, 0.4f}, {0.35f, 0.08f}, {0.30f, 0.06f},
                   {0.18f, 0.06f}, {0.12f, 0.04f}, {0.05f, 0.02f}, {0.85f, 0.05f} }},
        0.15f, 5
    },
    /* CLUMP   */ {
        { 0.35f, 0.06f, { {5.0f, 1.0f}, {3.2f, 0.6f}, {0.40f, 0.10f}, {0.45f, 0.08f},
                   {0.25f, 0.08f}, {0.18f, 0.06f}, {0.08f, 0.03f}, {0.82f, 0.05f} }},
        0.20f, 6
    },
    /* THICKET */ {
        { 0.15f, 0.08f, { {6.0f, 1.0f}, {5.5f, 1.2f}, {0.45f, 0.10f}, {0.55f, 0.10f},
                   {0.30f, 0.10f}, {0.22f, 0.08f}, {0.10f, 0.04f}, {0.80f, 0.06f} }},
        0.25f, 7
    },
};

static const TierProfile& blade_get_tier_profile(uint32_t tier_idx) {
    return BLADE_TIERS[tier_idx].profile;
}

// ─── Color Parts ─────────────────────────────────────────────────
// Body: BLADE_BODY_BASE, prop=COLOR_VAR_R, offset=0
// Aged: BLADE_AGED_BASE, prop=COLOR_VAR_R, offset=10
// Variance comes from the tier (color_var), applied in the adapter.

static constexpr ColorPartDef BLADE_COLOR_PARTS[] = {
    { { 0.28f, 0.52f, 0.22f },   // BLADE_BODY_BASE
      0.0f,                        // variance set per-tier in adapter
      BladeProp::COLOR_VAR_R, 0 },
    { { 0.48f, 0.45f, 0.28f },   // BLADE_AGED_BASE
      0.0f,
      BladeProp::COLOR_VAR_R, 10 },
};

// ─── Traits Declaration ──────────────────────────────────────────

static constexpr EntityFamilyTraits BLADE_TRAITS = {
    PopFamily::BLADE, "blad",
    Dim::MAX_BLADE_INSTANCES,
    true, false, 0,                                    // grounded, no piers
    true, 100.0f, 1.0f,                               // footprint, cull
    BladeProp::SPAWN_ROLL, BladeClusterConfig::SPAWN_CHANCE,
    BladeClusterConfig::MOOD_MULTIPLIER,
    BladeClusterConfig::POSITION_JITTER,
    BLADE_TIER_COUNT, BladeProp::TIER,
    BLADE_PARAM_DEFS, BLADE_PARAM_COUNT,
    BladeProp::POSITION_X, BladeProp::POSITION_Z, BladeProp::ROTATION, true,
    2, BLADE_COLOR_PARTS,
};

// ─── Blade Adapter Functions ─────────────────────────────────────

static SpawnGateOutput blade_run_gate(Cartridge* c,
    int32_t gx, int32_t gz) {
    auto gate = c->run_spawn_preamble(gx, gz,
        c->entities_state_.blades, Dim::MAX_BLADE_INSTANCES,
        BladeProp::SPAWN_ROLL, BladeClusterConfig::SPAWN_CHANCE,
        BladeClusterConfig::MOOD_MULTIPLIER,
        PopFamily::BLADE, "blad");
    return { gate.ok, gate.seed, gate.slot, gate.theme_idx };
}

static const float* blade_get_theme_tier_weights(uint32_t theme_idx) {
    return THEMES[theme_idx].tier_wt_blade;
}

static void blade_compute_solid_half(EntityInstance& inst,
    const TierProfile& /*tier*/) {
    inst.solid_half = inst.params[BladeIdx::BLADE_W] * 0.5f;
    inst.burial = 0.0f;
}

// blade_compute_colors removed — Blade uses generic_compute_colors with
// TierProfile.color_var from BLADE_TIERS (closes Q24 / Q-closed-12).

static void blade_write_active(Cartridge* c, const EntityInstance& inst) {
    auto& ab = c->entities_state_.blades[inst.slot];
    ab.patch_gx = inst.trigger_gx;
    ab.patch_gz = inst.trigger_gz;
    ab.host_gx  = inst.host_gx;
    ab.host_gz  = inst.host_gz;
    ab.active   = true;
    ab.draw_visible = true;
    ab.world_x  = inst.cx;
    ab.world_z  = inst.cz;
    ab.height   = inst.params[BladeIdx::BLADE_H];
    ab.radius   = inst.params[BladeIdx::BLADE_W] + inst.params[BladeIdx::SPLAY];
    ab.tier_idx = inst.tier_idx;
    ab.cached_ground_y = inst.cached_ground_y;
    c->entities_state_.blade_count++;
}

static void blade_write_gpu(Cartridge* c,
    const EntityInstance& inst, wgpu::Queue& queue) {
    GPUBladeClusterMeshParams mp{};
    mp.center_x    = inst.cx;
    mp.center_z    = inst.cz;
    mp.blade_count = inst.params[BladeIdx::BLADE_COUNT];
    mp.blade_h     = inst.params[BladeIdx::BLADE_H];
    mp.blade_h_var = inst.params[BladeIdx::BLADE_H_VAR];
    mp.blade_w     = inst.params[BladeIdx::BLADE_W];
    mp.splay       = inst.params[BladeIdx::SPLAY];
    mp.curve       = inst.params[BladeIdx::CURVE];
    mp.twist       = inst.params[BladeIdx::TWIST];
    mp.taper       = inst.params[BladeIdx::TAPER];
    mp.blade_r     = inst.colors[0];
    mp.blade_g     = inst.colors[1];
    mp.blade_b     = inst.colors[2];
    mp.aged_r      = inst.colors[3];
    mp.aged_g      = inst.colors[4];
    mp.aged_b      = inst.colors[5];
    mp.blade_segs  = BLADE_TIERS[inst.tier_idx].blade_segs;
    mp.is_active   = 1;
    mp.seed        = inst.seed;
    c->gpuState_.upload_blade_mesh_params_slot(queue, inst.slot, mp);
    c->entities_state_.blade_mesh_gen_pending = true;
}

static constexpr EntityFamilyAdapter BLADE_ADAPTER = {
    blade_run_gate,
    blade_get_theme_tier_weights,
    nullptr,                  // apply_indoor_rescale → not eligible (outdoor < ceiling)
    blade_compute_solid_half,
    nullptr,                  // compute_colors → use generic (Q24)
    blade_write_active,
    blade_write_gpu,
    nullptr,                  // no post_commit (no piers, no portals)
    blade_get_tier_profile,
};

// ── Blade dispatch wrappers ──

static bool dispatch_select_blade_generic(Cartridge* self, int32_t gx, int32_t gz, EntityQueueEntry& e) {
    EntityInstance inst{};
    if (!self->generic_select(BLADE_TRAITS, BLADE_ADAPTER, gx, gz, inst)) return false;
    e.family = PopFamily::BLADE; e.gx = gx; e.gz = gz; e.generic = inst; return true;
}
static bool dispatch_place_blade_generic(Cartridge* self, EntityQueueEntry& e, PlacementEntry& pe) {
    pe.family = e.family; pe.gx = e.gx; pe.gz = e.gz;
    if (self->generic_place(BLADE_TRAITS, e.generic)) { pe.generic = e.generic; return true; }
    self->entities_state_.blades[e.generic.slot].active = false; return false;
}
static void dispatch_commit_blade_generic(Cartridge* self, PlacementEntry& pe, wgpu::Queue& queue) {
    auto* host = self->find_patch(pe.generic.host_gx, pe.generic.host_gz);
    if (host) { self->generic_commit(BLADE_TRAITS, BLADE_ADAPTER, pe.generic, queue); host->record_entity(PopFamily::BLADE, pe.generic.slot); }
    else { self->entities_state_.blades[pe.generic.slot].active = false; }
}

// ═══ FAMILY: PALM ═════════════════════════════════════════════════
//
// Tall trunk + radial fronds. Vegetation-cluster sibling of Cactus and Blade.
//


struct PalmIdx {
    static constexpr uint32_t HEIGHT       = 0;
    static constexpr uint32_t BASE_R       = 1;
    static constexpr uint32_t TOP_R        = 2;
    static constexpr uint32_t LEAN         = 3;
    static constexpr uint32_t LEAN_DIR     = 4;
    static constexpr uint32_t BARK_RINGS   = 5;
    static constexpr uint32_t BARK_DEPTH   = 6;
    static constexpr uint32_t FROND_COUNT  = 7;
    static constexpr uint32_t FROND_LEN    = 8;
    static constexpr uint32_t FROND_WIDTH  = 9;
    static constexpr uint32_t FROND_DROOP  = 10;
    static constexpr uint32_t FROND_ARCH   = 11;
    static constexpr uint32_t CROWN_SPREAD = 12;
    static constexpr uint32_t CROWN_SKIRT  = 13;
    static constexpr uint32_t SOLID_PAD    = 14;
    static constexpr uint32_t EDGE_BLEND   = 15;
    static constexpr uint32_t COUNT        = 16;
};

static constexpr TierParamDef PALM_PARAM_DEFS[] = {
    { PalmProp::HEIGHT,       2.0f, 1e30f,  false, ParamDist::GAUSSIAN },
    { PalmProp::BASE_R,       0.1f, 1e30f,  false, ParamDist::GAUSSIAN },
    { PalmProp::TOP_R,        0.05f, 1e30f, false, ParamDist::GAUSSIAN },
    { PalmProp::LEAN,         0.0f, 1e30f,  false, ParamDist::GAUSSIAN },
    { PalmProp::LEAN_DIR,     0.0f, 1e30f,  false, ParamDist::UNIFORM_TAU },
    { PalmProp::BARK_RINGS,   3.0f, 1e30f,  true,  ParamDist::GAUSSIAN },
    { PalmProp::BARK_DEPTH,   0.0f, 1e30f,  false, ParamDist::GAUSSIAN },
    { PalmProp::FROND_COUNT,  3.0f, 1e30f,  true,  ParamDist::GAUSSIAN },
    { PalmProp::FROND_LEN,    1.0f, 1e30f,  false, ParamDist::GAUSSIAN },
    { PalmProp::FROND_WIDTH,  0.3f, 1e30f,  false, ParamDist::GAUSSIAN },
    { PalmProp::FROND_DROOP,  0.0f, 1e30f,  false, ParamDist::GAUSSIAN },
    { PalmProp::FROND_ARCH,   0.0f, 1e30f,  false, ParamDist::GAUSSIAN },
    { PalmProp::CROWN_SPREAD, 0.1f, 1e30f,  false, ParamDist::GAUSSIAN },
    { PalmProp::CROWN_SKIRT,  0.0f, 1e30f,  false, ParamDist::GAUSSIAN },
    { PalmProp::SOLID_PADDING,0.1f, 1e30f,  false, ParamDist::GAUSSIAN },
    { PalmProp::EDGE_BLEND,   0.1f, 1e30f,  false, ParamDist::GAUSSIAN },
};
static constexpr uint32_t PALM_PARAM_COUNT = sizeof(PALM_PARAM_DEFS) / sizeof(TierParamDef);

// params[] order MUST match PALM_PARAM_DEFS:
//   [0]HEIGHT [1]BASE_R [2]TOP_R [3]LEAN [4]LEAN_DIR(uniform — {0,0})
//   [5]BARK_RINGS [6]BARK_DEPTH [7]FROND_COUNT [8]FROND_LEN
//   [9]FROND_WIDTH [10]FROND_DROOP [11]FROND_ARCH [12]CROWN_SPREAD
//   [13]CROWN_SKIRT [14]SOLID_PAD [15]EDGE_BLEND
//
// LEAN_DIR is UNIFORM_TAU — uniform [0, 2π] has no meaningful mean/sigma,
// the slot is a literal {0, 0} placeholder.
struct PalmTierRow {
    TierProfile profile;
    float       color_over;
    float       burial;
    float       trunk_var;
    float       frond_var;
    uint32_t    trunk_segs;
    uint32_t    frond_segs;
};

static constexpr PalmTierRow PALM_TIERS[] = {
    /* SAPLING */ {
        { 0.50f, 0.0f, { {8.0f, 2.0f},  {0.25f, 0.05f}, {0.12f, 0.02f}, {0.15f, 0.05f}, {0.0f, 0.0f},
                   {8.0f, 2.0f},  {0.03f, 0.01f}, {7.0f, 1.0f},   {3.0f, 0.5f},   {0.8f, 0.15f},
                   {0.3f, 0.1f},  {0.4f, 0.1f},   {0.6f, 0.1f},   {0.3f, 0.1f},   {0.5f, 0.1f},
                   {0.5f, 0.1f} }},
        0.1f, 0.1f, 0.06f, 0.04f, 12, 4
    },
    /* COASTAL */ {
        { 0.35f, 0.0f, { {16.0f, 3.0f}, {0.40f, 0.08f}, {0.15f, 0.03f}, {0.20f, 0.08f}, {0.0f, 0.0f},
                   {12.0f, 3.0f}, {0.04f, 0.01f}, {11.0f, 2.0f},  {5.0f, 0.8f},   {1.2f, 0.2f},
                   {0.5f, 0.15f}, {0.5f, 0.12f},  {0.7f, 0.1f},   {0.4f, 0.1f},   {0.8f, 0.15f},
                   {0.8f, 0.15f} }},
        0.15f, 0.15f, 0.05f, 0.03f, 16, 5
    },
    /* ROYAL   */ {
        { 0.15f, 0.0f, { {28.0f, 5.0f}, {0.55f, 0.10f}, {0.18f, 0.04f}, {0.10f, 0.05f}, {0.0f, 0.0f},
                   {18.0f, 4.0f}, {0.05f, 0.01f}, {15.0f, 2.0f},  {7.0f, 1.0f},   {1.5f, 0.3f},
                   {0.6f, 0.15f}, {0.6f, 0.15f},  {0.8f, 0.1f},   {0.5f, 0.1f},   {1.0f, 0.2f},
                   {1.0f, 0.2f} }},
        0.20f, 0.2f, 0.04f, 0.03f, 20, 6
    },
};

static const TierProfile& palm_get_tier_profile(uint32_t tier_idx) {
    return PALM_TIERS[tier_idx].profile;
}

static constexpr ColorPartDef PALM_COLOR_PARTS[] = {
    { {0.45f,0.35f,0.25f}, 0.0f, PalmProp::COLOR_VAR_R, 0 },   // trunk
    { {0.25f,0.45f,0.20f}, 0.0f, PalmProp::COLOR_VAR_R, 10 },  // frond
    { {0.35f,0.38f,0.18f}, 0.0f, PalmProp::COLOR_VAR_R, 20 },  // aged
};

static constexpr EntityFamilyTraits PALM_TRAITS = {
    PopFamily::PALM, "palm", Dim::MAX_PALM_INSTANCES,
    true, false, 0,
    true, 100.0f, 1.0f,
    PalmProp::SPAWN_ROLL, PalmConfig::SPAWN_CHANCE,
    PalmConfig::MOOD_MULTIPLIER, PalmConfig::POSITION_JITTER,
    PALM_TIER_COUNT, PalmProp::TIER,
    PALM_PARAM_DEFS, PALM_PARAM_COUNT,
    PalmProp::POSITION_X, PalmProp::POSITION_Z, PalmProp::ROTATION, true,
    3, PALM_COLOR_PARTS,
};

// ── Palm adapter functions ──

static SpawnGateOutput palm_run_gate(Cartridge* c, int32_t gx, int32_t gz) {
    auto gate = c->run_spawn_preamble(gx, gz,
        c->entities_state_.palms, Dim::MAX_PALM_INSTANCES,
        PalmProp::SPAWN_ROLL, PalmConfig::SPAWN_CHANCE,
        PalmConfig::MOOD_MULTIPLIER, PopFamily::PALM, "palm");
    return { gate.ok, gate.seed, gate.slot, gate.theme_idx };
}

static const float* palm_get_theme_tier_weights(uint32_t theme_idx) {
    return THEMES[theme_idx].tier_wt_palm;
}

static constexpr uint32_t PALM_INDOOR_RESCALE_PARAMS[] = {
    PalmIdx::HEIGHT, PalmIdx::BASE_R, PalmIdx::TOP_R, PalmIdx::BARK_DEPTH,
    PalmIdx::FROND_LEN, PalmIdx::FROND_WIDTH, PalmIdx::CROWN_SPREAD,
    PalmIdx::CROWN_SKIRT, PalmIdx::SOLID_PAD, PalmIdx::EDGE_BLEND,
    // LEAN/LEAN_DIR (angles), BARK_RINGS/FROND_COUNT (counts),
    // FROND_DROOP/FROND_ARCH (angles) intentionally not scaled.
};

// Palms read as canopy-defining architectural anchors and look wrong
// when too small — tighter target range than other indoor families.
static void palm_apply_indoor_rescale(EntityInstance& inst, float ceiling_h) {
    rescale_to_rolled_target(inst, ceiling_h,
        /*target_lo*/ 0.80f, /*target_hi*/ 0.95f,
        /*current_h*/ inst.params[PalmIdx::HEIGHT],
        PALM_INDOOR_RESCALE_PARAMS);
}

static void palm_compute_solid_half(EntityInstance& inst, const TierProfile&) {
    float base_r = inst.params[PalmIdx::BASE_R];
    float pad    = inst.params[PalmIdx::SOLID_PAD];
    float blend  = inst.params[PalmIdx::EDGE_BLEND];
    inst.solid_half = base_r + pad + blend;
    inst.burial = PALM_TIERS[inst.tier_idx].burial;
}

static void palm_compute_colors(EntityInstance& inst, const EntityFamilyTraits& traits, const TierProfile& /*tier*/) {
    const auto& tp = PALM_TIERS[inst.tier_idx];
    // trunk: trunk_var
    inst.colors[0] = traits.color_parts[0].base[0] + (cpu_hash_f(inst.seed, PalmProp::COLOR_VAR_R) - 0.5f) * tp.trunk_var;
    inst.colors[1] = traits.color_parts[0].base[1] + (cpu_hash_f(inst.seed, PalmProp::COLOR_VAR_G) - 0.5f) * tp.trunk_var;
    inst.colors[2] = traits.color_parts[0].base[2] + (cpu_hash_f(inst.seed, PalmProp::COLOR_VAR_B) - 0.5f) * tp.trunk_var;
    // frond: frond_var
    inst.colors[3] = traits.color_parts[1].base[0] + (cpu_hash_f(inst.seed, PalmProp::COLOR_VAR_R + 10u) - 0.5f) * tp.frond_var;
    inst.colors[4] = traits.color_parts[1].base[1] + (cpu_hash_f(inst.seed, PalmProp::COLOR_VAR_G + 10u) - 0.5f) * tp.frond_var;
    inst.colors[5] = traits.color_parts[1].base[2] + (cpu_hash_f(inst.seed, PalmProp::COLOR_VAR_B + 10u) - 0.5f) * tp.frond_var;
    // aged: frond_var (same variance as frond)
    inst.colors[6] = traits.color_parts[2].base[0] + (cpu_hash_f(inst.seed, PalmProp::COLOR_VAR_R + 20u) - 0.5f) * tp.frond_var;
    inst.colors[7] = traits.color_parts[2].base[1] + (cpu_hash_f(inst.seed, PalmProp::COLOR_VAR_G + 20u) - 0.5f) * tp.frond_var;
    inst.colors[8] = traits.color_parts[2].base[2] + (cpu_hash_f(inst.seed, PalmProp::COLOR_VAR_B + 20u) - 0.5f) * tp.frond_var;
}

static void palm_write_active(Cartridge* c, const EntityInstance& inst) {
    auto& ap = c->entities_state_.palms[inst.slot];
    ap.patch_gx = inst.trigger_gx; ap.patch_gz = inst.trigger_gz;
    ap.host_gx = inst.host_gx; ap.host_gz = inst.host_gz;
    ap.active = true; ap.draw_visible = true;
    ap.world_x = inst.cx; ap.world_z = inst.cz;
    ap.height = inst.params[PalmIdx::HEIGHT];
    ap.base_r = inst.params[PalmIdx::BASE_R];
    ap.tier_idx = inst.tier_idx;
    ap.cached_ground_y = inst.cached_ground_y;
    c->entities_state_.palm_count++;
}

static void palm_write_gpu(Cartridge* c, const EntityInstance& inst, wgpu::Queue& queue) {
    const auto& tp = PALM_TIERS[inst.tier_idx];
    GPUPalmMeshParams mp{};
    mp.center_x     = inst.cx;
    mp.center_z     = inst.cz;
    mp.height       = inst.params[PalmIdx::HEIGHT];
    mp.base_r       = inst.params[PalmIdx::BASE_R];
    mp.top_r        = inst.params[PalmIdx::TOP_R];
    mp.lean         = inst.params[PalmIdx::LEAN];
    mp.lean_dir     = inst.params[PalmIdx::LEAN_DIR];
    mp.bark_rings   = inst.params[PalmIdx::BARK_RINGS];
    mp.bark_depth   = inst.params[PalmIdx::BARK_DEPTH];
    mp.frond_count  = inst.params[PalmIdx::FROND_COUNT];
    mp.frond_len    = inst.params[PalmIdx::FROND_LEN];
    mp.frond_width  = inst.params[PalmIdx::FROND_WIDTH];
    mp.frond_droop  = inst.params[PalmIdx::FROND_DROOP];
    mp.frond_arch   = inst.params[PalmIdx::FROND_ARCH];
    mp.crown_spread = inst.params[PalmIdx::CROWN_SPREAD];
    mp.crown_skirt  = inst.params[PalmIdx::CROWN_SKIRT];
    mp.burial       = inst.burial;
    mp.trunk_r = inst.colors[0]; mp.trunk_g = inst.colors[1]; mp.trunk_b = inst.colors[2];
    mp.frond_r = inst.colors[3]; mp.frond_g = inst.colors[4]; mp.frond_b = inst.colors[5];
    mp.aged_r  = inst.colors[6]; mp.aged_g  = inst.colors[7]; mp.aged_b  = inst.colors[8];
    mp.trunk_segs = tp.trunk_segs;
    mp.frond_segs = tp.frond_segs;
    mp.is_active = 1;
    c->gpuState_.upload_palm_mesh_params_slot(queue, inst.slot, mp);
    c->entities_state_.palm_mesh_gen_pending = true;
}

static constexpr EntityFamilyAdapter PALM_ADAPTER = {
    palm_run_gate, palm_get_theme_tier_weights,
    palm_apply_indoor_rescale,
    palm_compute_solid_half, palm_compute_colors,
    palm_write_active, palm_write_gpu, nullptr,
    palm_get_tier_profile,
};

// ── Palm dispatch wrappers ──

static bool dispatch_select_palm_generic(Cartridge* self, int32_t gx, int32_t gz, EntityQueueEntry& e) {
    EntityInstance inst{};
    if (!self->generic_select(PALM_TRAITS, PALM_ADAPTER, gx, gz, inst)) return false;
    e.family = PopFamily::PALM; e.gx = gx; e.gz = gz; e.generic = inst; return true;
}
static bool dispatch_place_palm_generic(Cartridge* self, EntityQueueEntry& e, PlacementEntry& pe) {
    pe.family = e.family; pe.gx = e.gx; pe.gz = e.gz;
    if (self->generic_place(PALM_TRAITS, e.generic)) { pe.generic = e.generic; return true; }
    self->entities_state_.palms[e.generic.slot].active = false; return false;
}
static void dispatch_commit_palm_generic(Cartridge* self, PlacementEntry& pe, wgpu::Queue& queue) {
    auto* host = self->find_patch(pe.generic.host_gx, pe.generic.host_gz);
    if (host) { self->generic_commit(PALM_TRAITS, PALM_ADAPTER, pe.generic, queue); host->record_entity(PopFamily::PALM, pe.generic.slot); }
    else { self->entities_state_.palms[pe.generic.slot].active = false; }
}


// ═══ FAMILY: CACTUS ═══════════════════════════════════════════════
//
// Ribbed columnar trunk + optional arms. Vegetation-cluster sibling of Palm and Blade.
//


struct CactusIdx {
    static constexpr uint32_t HEIGHT     = 0;
    static constexpr uint32_t RADIUS     = 1;
    static constexpr uint32_t TAPER      = 2;
    static constexpr uint32_t RIBS       = 3;
    static constexpr uint32_t RIB_DEPTH  = 4;
    static constexpr uint32_t LEAN       = 5;
    static constexpr uint32_t LEAN_DIR   = 6;
    static constexpr uint32_t CAP_ROUND  = 7;
    static constexpr uint32_t ARM_COUNT  = 8;
    static constexpr uint32_t ARM_HEIGHT = 9;
    static constexpr uint32_t ARM_LENGTH = 10;
    static constexpr uint32_t ARM_RADIUS = 11;
    static constexpr uint32_t ARM_CURVE  = 12;
    static constexpr uint32_t COUNT      = 13;
};

static constexpr TierParamDef CACTUS_PARAM_DEFS[] = {
    { CactusProp::HEIGHT,     1.0f, 1e30f,  false, ParamDist::GAUSSIAN },
    { CactusProp::RADIUS,     0.05f, 1e30f, false, ParamDist::GAUSSIAN },
    { CactusProp::TAPER,      0.5f, 1e30f,  false, ParamDist::GAUSSIAN },
    { CactusProp::RIBS,       4.0f, 1e30f,  true,  ParamDist::GAUSSIAN },
    { CactusProp::RIB_DEPTH,  0.0f, 1e30f,  false, ParamDist::GAUSSIAN },
    { CactusProp::LEAN,       0.0f, 1e30f,  false, ParamDist::GAUSSIAN },
    { CactusProp::LEAN_DIR,   0.0f, 1e30f,  false, ParamDist::UNIFORM_TAU },
    { CactusProp::CAP_ROUND,  0.0f, 1e30f,  false, ParamDist::GAUSSIAN },
    { CactusProp::ARM_COUNT,  0.0f, 1e30f,  true,  ParamDist::GAUSSIAN },
    { CactusProp::ARM_HEIGHT, 0.1f, 1e30f,  false, ParamDist::GAUSSIAN },
    { CactusProp::ARM_LENGTH, 0.5f, 1e30f,  false, ParamDist::GAUSSIAN },
    { CactusProp::ARM_RADIUS, 0.03f, 1e30f, false, ParamDist::GAUSSIAN },
    { CactusProp::ARM_CURVE,  0.0f, 1e30f,  false, ParamDist::GAUSSIAN },
};
static constexpr uint32_t CACTUS_PARAM_COUNT = sizeof(CACTUS_PARAM_DEFS) / sizeof(TierParamDef);

// params[] order MUST match CACTUS_PARAM_DEFS:
//   [0]HEIGHT [1]RADIUS [2]TAPER [3]RIBS [4]RIB_DEPTH [5]LEAN
//   [6]LEAN_DIR(uniform — {0,0}) [7]CAP_ROUND [8]ARM_COUNT
//   [9]ARM_HEIGHT [10]ARM_LENGTH [11]ARM_RADIUS [12]ARM_CURVE
struct CactusTierRow {
    TierProfile profile;
    float       color_over;
    uint32_t    trunk_segs;
    uint32_t    arm_segs;
};

static constexpr CactusTierRow CACTUS_TIERS[] = {
    /* FINGER     */ {
        { 0.50f, 0.04f, { {3.0f, 1.0f},   {0.15f, 0.03f}, {0.85f, 0.05f}, {8.0f, 1.0f},   {0.04f, 0.01f},
                   {0.1f, 0.05f},  {0.0f, 0.0f},   {0.6f, 0.1f},   {0.0f, 0.0f},
                   {0.5f, 0.1f},   {1.0f, 0.3f},   {0.08f, 0.02f}, {0.7f, 0.1f} }},
        0.1f, 12, 6
    },
    /* SAGUARO    */ {
        { 0.35f, 0.03f, { {8.0f, 2.0f},   {0.35f, 0.06f}, {0.9f, 0.04f},  {12.0f, 2.0f},  {0.05f, 0.01f},
                   {0.08f, 0.04f}, {0.0f, 0.0f},   {0.5f, 0.1f},   {1.5f, 0.7f},
                   {0.45f, 0.1f},  {3.0f, 0.8f},   {0.2f, 0.04f},  {0.6f, 0.15f} }},
        0.15f, 16, 8
    },
    /* CANDELABRA */ {
        { 0.15f, 0.03f, { {14.0f, 3.0f},  {0.45f, 0.08f}, {0.92f, 0.03f}, {16.0f, 2.0f},  {0.06f, 0.01f},
                   {0.05f, 0.03f}, {0.0f, 0.0f},   {0.4f, 0.1f},   {3.0f, 0.8f},
                   {0.4f, 0.1f},   {5.0f, 1.0f},   {0.25f, 0.05f}, {0.5f, 0.15f} }},
        0.2f, 20, 10
    },
};

static const TierProfile& cactus_get_tier_profile(uint32_t tier_idx) {
    return CACTUS_TIERS[tier_idx].profile;
}

static constexpr ColorPartDef CACTUS_COLOR_PARTS[] = {
    { {0.30f,0.45f,0.25f}, 0.0f, CactusProp::COLOR_VAR_R, 0 },   // body
    { {0.35f,0.55f,0.30f}, 0.0f, CactusProp::COLOR_VAR_R, 10 },  // rib
};

static constexpr EntityFamilyTraits CACTUS_TRAITS = {
    PopFamily::CACTUS, "cact", Dim::MAX_CACTUS_INSTANCES,
    true, false, 0,
    true, 100.0f, 1.0f,
    CactusProp::SPAWN_ROLL, CactusConfig::SPAWN_CHANCE,
    CactusConfig::MOOD_MULTIPLIER, CactusConfig::POSITION_JITTER,
    CACTUS_TIER_COUNT, CactusProp::TIER,
    CACTUS_PARAM_DEFS, CACTUS_PARAM_COUNT,
    CactusProp::POSITION_X, CactusProp::POSITION_Z, CactusProp::ROTATION, true,
    2, CACTUS_COLOR_PARTS,
};

// ── Cactus adapter functions ──

static SpawnGateOutput cactus_run_gate(Cartridge* c, int32_t gx, int32_t gz) {
    auto gate = c->run_spawn_preamble(gx, gz,
        c->entities_state_.cacti, Dim::MAX_CACTUS_INSTANCES,
        CactusProp::SPAWN_ROLL, CactusConfig::SPAWN_CHANCE,
        CactusConfig::MOOD_MULTIPLIER, PopFamily::CACTUS, "cact");
    return { gate.ok, gate.seed, gate.slot, gate.theme_idx };
}

static const float* cactus_get_theme_tier_weights(uint32_t theme_idx) {
    return THEMES[theme_idx].tier_wt_cactus;
}

static void cactus_compute_solid_half(EntityInstance& inst, const TierProfile&) {
    inst.solid_half = inst.params[CactusIdx::RADIUS] + 0.5f;
    inst.burial = 0.0f;
}

// cactus_compute_colors removed — Cactus uses generic_compute_colors with
// TierProfile.color_var from CACTUS_TIERS (closes Q24 / Q-closed-12).

static void cactus_write_active(Cartridge* c, const EntityInstance& inst) {
    auto& ac = c->entities_state_.cacti[inst.slot];
    ac.patch_gx = inst.trigger_gx; ac.patch_gz = inst.trigger_gz;
    ac.host_gx = inst.host_gx; ac.host_gz = inst.host_gz;
    ac.active = true; ac.draw_visible = true;
    ac.world_x = inst.cx; ac.world_z = inst.cz;
    ac.height = inst.params[CactusIdx::HEIGHT];
    ac.radius = inst.params[CactusIdx::RADIUS];
    ac.tier_idx = inst.tier_idx;
    ac.cached_ground_y = inst.cached_ground_y;
    c->entities_state_.cactus_count++;
}

static void cactus_write_gpu(Cartridge* c, const EntityInstance& inst, wgpu::Queue& queue) {
    const auto& tp = CACTUS_TIERS[inst.tier_idx];
    GPUCactusMeshParams mp{};
    mp.center_x   = inst.cx;
    mp.center_z   = inst.cz;
    mp.height     = inst.params[CactusIdx::HEIGHT];
    mp.radius     = inst.params[CactusIdx::RADIUS];
    mp.taper      = inst.params[CactusIdx::TAPER];
    mp.ribs       = inst.params[CactusIdx::RIBS];
    mp.rib_depth  = inst.params[CactusIdx::RIB_DEPTH];
    mp.lean       = inst.params[CactusIdx::LEAN];
    mp.lean_dir   = inst.params[CactusIdx::LEAN_DIR];
    mp.cap_round  = inst.params[CactusIdx::CAP_ROUND];
    mp.arm_count  = inst.params[CactusIdx::ARM_COUNT];
    mp.arm_height = inst.params[CactusIdx::ARM_HEIGHT];
    mp.arm_length = inst.params[CactusIdx::ARM_LENGTH];
    mp.arm_radius = inst.params[CactusIdx::ARM_RADIUS];
    mp.arm_curve  = inst.params[CactusIdx::ARM_CURVE];
    mp.body_r = inst.colors[0]; mp.body_g = inst.colors[1]; mp.body_b = inst.colors[2];
    mp.rib_r  = inst.colors[3]; mp.rib_g  = inst.colors[4]; mp.rib_b  = inst.colors[5];
    mp.trunk_segs = tp.trunk_segs;
    mp.arm_segs   = tp.arm_segs;
    mp.is_active  = 1;
    mp.seed       = inst.seed;
    c->gpuState_.upload_cactus_mesh_params_slot(queue, inst.slot, mp);
    c->entities_state_.cactus_mesh_gen_pending = true;
}

static constexpr EntityFamilyAdapter CACTUS_ADAPTER = {
    cactus_run_gate, cactus_get_theme_tier_weights,
    nullptr,                              // apply_indoor_rescale → not eligible
    cactus_compute_solid_half, nullptr,   // compute_colors → use generic (Q24)
    cactus_write_active, cactus_write_gpu, nullptr,
    cactus_get_tier_profile,
};

// ── Cactus dispatch wrappers ──

static bool dispatch_select_cactus_generic(Cartridge* self, int32_t gx, int32_t gz, EntityQueueEntry& e) {
    EntityInstance inst{};
    if (!self->generic_select(CACTUS_TRAITS, CACTUS_ADAPTER, gx, gz, inst)) return false;
    e.family = PopFamily::CACTUS; e.gx = gx; e.gz = gz; e.generic = inst; return true;
}
static bool dispatch_place_cactus_generic(Cartridge* self, EntityQueueEntry& e, PlacementEntry& pe) {
    pe.family = e.family; pe.gx = e.gx; pe.gz = e.gz;
    if (self->generic_place(CACTUS_TRAITS, e.generic)) { pe.generic = e.generic; return true; }
    self->entities_state_.cacti[e.generic.slot].active = false; return false;
}
static void dispatch_commit_cactus_generic(Cartridge* self, PlacementEntry& pe, wgpu::Queue& queue) {
    auto* host = self->find_patch(pe.generic.host_gx, pe.generic.host_gz);
    if (host) { self->generic_commit(CACTUS_TRAITS, CACTUS_ADAPTER, pe.generic, queue); host->record_entity(PopFamily::CACTUS, pe.generic.slot); }
    else { self->entities_state_.cacti[pe.generic.slot].active = false; }
}


// ═══ FAMILY: COLUMN + ANTENNA ═════════════════════════════════════
//
// Pier-creating entities sharing the ColumnTierRow shape. Antenna is a design cell division from Column.
//


// Shared param index layout (both families sample the same 13 params)
struct ColIdx {
    static constexpr uint32_t HEIGHT          = 0;
    static constexpr uint32_t SHAFT_RADIUS    = 1;
    static constexpr uint32_t TAPER           = 2;
    static constexpr uint32_t ENTASIS         = 3;
    static constexpr uint32_t BASE_LAYERS     = 4;
    static constexpr uint32_t BASE_HEIGHT     = 5;
    static constexpr uint32_t BASE_OVERHANG   = 6;
    static constexpr uint32_t CAP_LAYERS      = 7;
    static constexpr uint32_t CAP_HEIGHT      = 8;
    static constexpr uint32_t CAP_OVERHANG    = 9;
    static constexpr uint32_t SOLID_PADDING   = 10;
    static constexpr uint32_t SOLID_HEIGHT    = 11;
    static constexpr uint32_t EDGE_BLEND      = 12;
    static constexpr uint32_t COUNT           = 13;
};

// Column param defs (ColumnProp indices)
//                                                    prop                        floor   ceil   round  dist
static constexpr TierParamDef COLUMN_PARAM_DEFS[] = {
    { ColumnProp::HEIGHT,          1.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { ColumnProp::SHAFT_RADIUS,    0.1f, 1e30f, false, ParamDist::GAUSSIAN },
    { ColumnProp::TAPER,           0.5f, 1.0f,  false, ParamDist::GAUSSIAN },  // ceiling!
    { ColumnProp::ENTASIS,         0.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { ColumnProp::BASE_LAYERS,     0.0f, 1e30f, true,  ParamDist::GAUSSIAN },
    { ColumnProp::BASE_HEIGHT,     0.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { ColumnProp::BASE_OVERHANG,   0.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { ColumnProp::CAPITAL_LAYERS,  0.0f, 1e30f, true,  ParamDist::GAUSSIAN },
    { ColumnProp::CAPITAL_HEIGHT,  0.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { ColumnProp::CAPITAL_OVERHANG,0.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { ColumnProp::SOLID_PADDING,   0.05f,1e30f, false, ParamDist::GAUSSIAN },
    { ColumnProp::SOLID_HEIGHT,    0.6f, 1e30f, false, ParamDist::GAUSSIAN },
    { ColumnProp::EDGE_BLEND,      0.1f, 1e30f, false, ParamDist::GAUSSIAN },
};
static constexpr uint32_t COLUMN_PARAM_COUNT = sizeof(COLUMN_PARAM_DEFS) / sizeof(TierParamDef);

// Antenna param defs (AntennaProp indices, same layout)
static constexpr TierParamDef ANTENNA_PARAM_DEFS[] = {
    { AntennaProp::HEIGHT,          1.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { AntennaProp::SHAFT_RADIUS,    0.1f, 1e30f, false, ParamDist::GAUSSIAN },
    { AntennaProp::TAPER,           0.5f, 1.0f,  false, ParamDist::GAUSSIAN },
    { AntennaProp::ENTASIS,         0.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { AntennaProp::BASE_LAYERS,     0.0f, 1e30f, true,  ParamDist::GAUSSIAN },
    { AntennaProp::BASE_HEIGHT,     0.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { AntennaProp::BASE_OVERHANG,   0.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { AntennaProp::CAPITAL_LAYERS,  0.0f, 1e30f, true,  ParamDist::GAUSSIAN },
    { AntennaProp::CAPITAL_HEIGHT,  0.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { AntennaProp::CAPITAL_OVERHANG,0.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { AntennaProp::SOLID_PADDING,   0.05f,1e30f, false, ParamDist::GAUSSIAN },
    { AntennaProp::SOLID_HEIGHT,    0.6f, 1e30f, false, ParamDist::GAUSSIAN },
    { AntennaProp::EDGE_BLEND,      0.1f, 1e30f, false, ParamDist::GAUSSIAN },
};

// params[] order MUST match COLUMN_PARAM_DEFS / ANTENNA_PARAM_DEFS:
//   [0]HEIGHT [1]SHAFT_R [2]TAPER [3]ENTASIS [4]BASE_LAYERS [5]BASE_H
//   [6]BASE_OH [7]CAP_LAYERS [8]CAP_H [9]CAP_OH [10]SOLID_PAD
//   [11]SOLID_H [12]EDGE_BLEND
//
// Note: name is ColumnTierRow, not ColumnTier — entities.inl declares
// `enum class ColumnTier`, occupying that name.
struct ColumnTierRow {
    TierProfile profile;
    float       color_override;
    float       burial;
    uint32_t    segs_around;
    uint32_t    shaft_rings;
};

static constexpr ColumnTierRow COLUMN_TIERS[] = {
    /* PILLAR */ {
        { 0.05f, 0.0f, { {6.5f, 1.2f}, {1.80f, 0.30f}, {1.00f, 0.0f},  {0.00f, 0.0f},
                   {1.0f, 0.3f}, {0.50f, 0.10f}, {0.20f, 0.05f},
                   {1.0f, 0.3f}, {0.40f, 0.08f}, {0.15f, 0.04f},
                   {0.3f, 0.08f}, {1.5f, 0.3f},  {0.4f, 0.08f} }},
        0.85f, 0.25f, 16, 4
    },
    /* DORIC  */ {
        { 0.20f, 0.0f, { {6.4f, 1.2f}, {0.38f, 0.06f}, {0.85f, 0.03f}, {0.02f, 0.01f},
                   {0.0f, 0.0f}, {0.00f, 0.00f}, {0.00f, 0.00f},
                   {2.0f, 0.5f}, {0.50f, 0.10f}, {0.15f, 0.04f},
                   {0.2f, 0.05f}, {1.0f, 0.2f},  {0.3f, 0.05f} }},
        0.85f, 0.20f, 20, 8
    },
    /* ORNATE */ {
        { 0.18f, 0.0f, { {16.8f, 2.8f}, {1.35f, 0.19f}, {0.82f, 0.03f}, {0.04f, 0.01f},
                   {2.0f, 0.5f},  {1.20f, 0.25f}, {0.30f, 0.08f},
                   {3.0f, 0.5f},  {1.50f, 0.30f}, {0.40f, 0.10f},
                   {0.4f, 0.10f}, {1.5f, 0.3f},   {0.5f, 0.10f} }},
        0.85f, 0.20f, 28, 12
    },
};

static constexpr ColumnTierRow ANTENNA_TIERS[] = {
    /* ANTENNA  */ {
        { 0.10f, 0.0f, { {17.5f, 3.5f}, {0.30f, 0.05f}, {0.85f, 0.05f}, {0.00f, 0.0f},
                   {2.0f, 0.5f},  {2.1f, 0.42f},  {1.5f, 0.3f},
                   {0.0f, 0.0f},  {1.5f, 0.3f},   {0.0f, 0.0f},
                   {0.2f, 0.05f}, {1.0f, 0.2f},   {0.3f, 0.05f} }},
        0.40f, 0.20f, 16, 6
    },
    /* SQUAT    */ {
        { 0.22f, 0.0f, { {32.5f, 6.5f}, {0.90f, 0.15f}, {0.85f, 0.05f}, {0.00f, 0.0f},
                   {2.0f, 0.5f},  {2.0f, 0.4f},   {6.0f, 1.2f},
                   {0.0f, 0.0f},  {1.5f, 0.3f},   {0.0f, 0.0f},
                   {0.4f, 0.10f}, {1.5f, 0.3f},   {0.4f, 0.08f} }},
        0.40f, 0.20f, 16, 6
    },
    /* COLOSSAL */ {
        { 0.13f, 0.0f, { {125.0f, 25.0f}, {3.00f, 0.50f}, {0.85f, 0.05f}, {0.00f, 0.0f},
                   {2.0f, 0.5f},    {7.5f, 1.5f},   {17.5f, 3.5f},
                   {0.0f, 0.0f},    {7.5f, 1.5f},   {0.0f, 0.0f},
                   {1.95f, 0.39f},  {12.0f, 2.4f},  {1.0f, 0.20f} }},
        0.40f, 0.20f, 20, 8
    },
};

static const TierProfile& column_get_tier_profile(uint32_t tier_idx) {
    return COLUMN_TIERS[tier_idx].profile;
}
static const TierProfile& antenna_get_tier_profile(uint32_t tier_idx) {
    return ANTENNA_TIERS[tier_idx].profile;
}

// ── Column traits ──

static constexpr EntityFamilyTraits COLUMN_TRAITS = {
    PopFamily::COLUMN, "col", Dim::MAX_COLUMN_ONLY,
    true, true, 1,                                     // grounded, creates ground, 1 pier
    true, 80.0f, 2.5f,
    ColumnProp::SPAWN_ROLL, ColumnConfig::SPAWN_CHANCE,
    ColumnConfig::MOOD_MULTIPLIER, ColumnConfig::POSITION_JITTER,
    COLUMN_TIER_COUNT, ColumnProp::TIER,
    COLUMN_PARAM_DEFS, COLUMN_PARAM_COUNT,
    ColumnProp::POSITION_X, ColumnProp::POSITION_Z, 355u, true,
    0, nullptr,  // color handled entirely by adapter
};

static constexpr EntityFamilyTraits ANTENNA_TRAITS = {
    PopFamily::ANTENNA, "ant", Dim::MAX_ANTENNA_ONLY,
    true, true, 1,
    true, 80.0f, 2.5f,
    AntennaProp::SPAWN_ROLL, AntennaConfig::SPAWN_CHANCE,
    AntennaConfig::MOOD_MULTIPLIER, AntennaConfig::POSITION_JITTER,
    ANTENNA_TIER_COUNT, AntennaProp::TIER,
    ANTENNA_PARAM_DEFS, COLUMN_PARAM_COUNT,
    AntennaProp::POSITION_X, AntennaProp::POSITION_Z, 355u, true,
    0, nullptr,
};

// ── Column adapter functions ──

static SpawnGateOutput column_run_gate(Cartridge* c, int32_t gx, int32_t gz) {
    auto gate = c->run_spawn_preamble(gx, gz,
        c->entities_state_.columns, Dim::MAX_COLUMN_ONLY,
        ColumnProp::SPAWN_ROLL, ColumnConfig::SPAWN_CHANCE,
        ColumnConfig::MOOD_MULTIPLIER, PopFamily::COLUMN, "col");
    return { gate.ok, gate.seed, gate.slot, gate.theme_idx };
}
static const float* column_get_theme_tier_weights(uint32_t ti) { return THEMES[ti].tier_wt_column; }

// Column shares its rescale param list with Antenna (both use the same
// ColumnTierRow shape and ColIdx layout). Antenna references this same
// array from its own apply_indoor_rescale.
static constexpr uint32_t COLUMN_INDOOR_RESCALE_PARAMS[] = {
    ColIdx::HEIGHT, ColIdx::SHAFT_RADIUS,
    ColIdx::BASE_HEIGHT, ColIdx::BASE_OVERHANG,
    ColIdx::CAP_HEIGHT,  ColIdx::CAP_OVERHANG,
    ColIdx::SOLID_PADDING, ColIdx::SOLID_HEIGHT, ColIdx::EDGE_BLEND,
    // TAPER (ratio), ENTASIS (ratio), BASE_LAYERS / CAP_LAYERS
    // (counts) intentionally not scaled.
};

// Column policy: snap to ceiling exactly. The capital meets the
// ceiling — column reads as part of the room's architecture, not a
// freestanding object. Does NOT use rescale_to_rolled_target — its
// scale factor is ceiling_h/current_h, not a rolled ratio.
static void column_apply_indoor_rescale(EntityInstance& inst, float ceiling_h) {
    const float current_h = inst.params[ColIdx::HEIGHT];
    if (current_h <= 1e-3f) return;
    const float scale = ceiling_h / current_h;
    for (uint32_t pi : COLUMN_INDOOR_RESCALE_PARAMS) inst.params[pi] *= scale;
}

static void column_compute_solid_half(EntityInstance& inst, const TierProfile&) {
    float shaft_r = inst.params[ColIdx::SHAFT_RADIUS];
    float base_oh = inst.params[ColIdx::BASE_OVERHANG];
    float cap_oh  = inst.params[ColIdx::CAP_OVERHANG];
    float pad     = inst.params[ColIdx::SOLID_PADDING];
    float blend   = inst.params[ColIdx::EDGE_BLEND];
    float solid_h = inst.params[ColIdx::SOLID_HEIGHT];
    inst.solid_half = shaft_r + std::max(base_oh, cap_oh) + pad + blend;
    inst.ground_y_offset = solid_h;
    inst.burial = std::max(0.2f, solid_h * COLUMN_TIERS[inst.tier_idx].burial);
}

static void column_compute_colors(EntityInstance& inst, const EntityFamilyTraits&, const TierProfile& /*tier*/) {
    // Sandstone / palette override
    if (cpu_hash_f(inst.seed, ColumnProp::COLOR_OVER) < COLUMN_TIERS[inst.tier_idx].color_override) {
        uint32_t pal_idx = cpu_hash(inst.seed, ColumnProp::COLOR_OVER + 1u) % COLUMN_PALETTE_COUNT;
        inst.colors[0] = COLUMN_PALETTE[pal_idx][0] + (cpu_hash_f(inst.seed, ColumnProp::COLOR_VAR_R) - 0.5f) * 0.06f;
        inst.colors[1] = COLUMN_PALETTE[pal_idx][1] + (cpu_hash_f(inst.seed, ColumnProp::COLOR_VAR_G) - 0.5f) * 0.06f;
        inst.colors[2] = COLUMN_PALETTE[pal_idx][2] + (cpu_hash_f(inst.seed, ColumnProp::COLOR_VAR_B) - 0.5f) * 0.06f;
    } else {
        inst.colors[0] = COLUMN_SANDSTONE_BASE[0] + (cpu_hash_f(inst.seed, ColumnProp::COLOR_VAR_R) - 0.5f) * COLUMN_SANDSTONE_VARIANCE * 2.0f;
        inst.colors[1] = COLUMN_SANDSTONE_BASE[1] + (cpu_hash_f(inst.seed, ColumnProp::COLOR_VAR_G) - 0.5f) * COLUMN_SANDSTONE_VARIANCE * 2.0f;
        inst.colors[2] = COLUMN_SANDSTONE_BASE[2] + (cpu_hash_f(inst.seed, ColumnProp::COLOR_VAR_B) - 0.5f) * COLUMN_SANDSTONE_VARIANCE * 2.0f;
    }
    // No drum colors for classical columns
    for (int i = 3; i < 12; i++) inst.colors[i] = 0.0f;
}

static void column_write_active(Cartridge* c, const EntityInstance& inst) {
    auto& ac = c->entities_state_.columns[inst.slot];
    ac.patch_gx = inst.trigger_gx; ac.patch_gz = inst.trigger_gz;
    ac.host_gx = inst.host_gx; ac.host_gz = inst.host_gz;
    ac.active = true; ac.draw_visible = true;
    ac.world_x = inst.cx; ac.world_z = inst.cz;
    ac.height = inst.params[ColIdx::HEIGHT];
    ac.shaft_radius = inst.params[ColIdx::SHAFT_RADIUS];
    ac.taper = inst.params[ColIdx::TAPER];
    ac.entasis = inst.params[ColIdx::ENTASIS];
    ac.base_layers = (uint32_t)inst.params[ColIdx::BASE_LAYERS];
    ac.base_height = inst.params[ColIdx::BASE_HEIGHT];
    ac.base_overhang = inst.params[ColIdx::BASE_OVERHANG];
    ac.cap_layers = (uint32_t)inst.params[ColIdx::CAP_LAYERS];
    ac.cap_height = inst.params[ColIdx::CAP_HEIGHT];
    ac.cap_overhang = inst.params[ColIdx::CAP_OVERHANG];
    ac.solid_height = inst.params[ColIdx::SOLID_HEIGHT];
    ac.burial = inst.burial;
    ac.segs_around = COLUMN_TIERS[inst.tier_idx].segs_around;
    ac.shaft_rings = COLUMN_TIERS[inst.tier_idx].shaft_rings;
    ac.tier_idx = inst.tier_idx;
    ac.cached_ground_y = inst.cached_ground_y;
    ac.col_r = inst.colors[0]; ac.col_g = inst.colors[1]; ac.col_b = inst.colors[2];
    std::memcpy(ac.drum_colors, &inst.colors[3], 9 * sizeof(float));
    c->entities_state_.column_count++;
}

static void column_write_gpu(Cartridge* c, const EntityInstance& inst, wgpu::Queue& queue) {
    GPUColumnMeshParams mp{};
    mp.center_x = inst.cx; mp.center_z = inst.cz;
    mp.height = inst.params[ColIdx::HEIGHT];
    mp.shaft_radius = inst.params[ColIdx::SHAFT_RADIUS];
    mp.taper = inst.params[ColIdx::TAPER];
    mp.entasis = inst.params[ColIdx::ENTASIS];
    mp.base_height = inst.params[ColIdx::BASE_HEIGHT];
    mp.base_overhang = inst.params[ColIdx::BASE_OVERHANG];
    mp.capital_height = inst.params[ColIdx::CAP_HEIGHT];
    mp.capital_overhang = inst.params[ColIdx::CAP_OVERHANG];
    mp.burial = inst.burial;
    mp.color_r = inst.colors[0]; mp.color_g = inst.colors[1]; mp.color_b = inst.colors[2];
    mp.base_layers = (uint32_t)inst.params[ColIdx::BASE_LAYERS];
    mp.capital_layers = (uint32_t)inst.params[ColIdx::CAP_LAYERS];
    mp.segs_around = COLUMN_TIERS[inst.tier_idx].segs_around;
    mp.shaft_rings = COLUMN_TIERS[inst.tier_idx].shaft_rings;
    mp.is_active = 1;
    mp.tier = inst.tier_idx;
    // drum colors from inst.colors[3..11]
    mp.drum_color_r1 = inst.colors[3]; mp.drum_color_g1 = inst.colors[4]; mp.drum_color_b1 = inst.colors[5];
    mp.drum_color_r2 = inst.colors[6]; mp.drum_color_g2 = inst.colors[7]; mp.drum_color_b2 = inst.colors[8];
    mp.drum_color_r3 = inst.colors[9]; mp.drum_color_g3 = inst.colors[10]; mp.drum_color_b3 = inst.colors[11];
    c->gpuState_.upload_column_mesh_params_slot(queue, inst.slot, mp);
    c->entities_state_.column_mesh_gen_pending = true;
}

static void column_post_commit(Cartridge* c, const EntityInstance& inst, wgpu::Queue& queue) {
    uint32_t pier_slot = Dim::PIER_COLUMN_BASE + inst.slot;
    GPUPierInstance pier{};
    pier.origin[0] = inst.cx;
    pier.origin[1] = inst.cz;
    pier.half_size[0] = inst.solid_half;
    pier.half_size[1] = inst.solid_half;
    pier.height_near = inst.params[ColIdx::SOLID_HEIGHT];
    pier.height_far = inst.params[ColIdx::SOLID_HEIGHT];
    pier.rotation = 0.0f;
    pier.edge_blend = inst.params[ColIdx::EDGE_BLEND];
    pier.tier = PierTier::COL_PILLAR + inst.tier_idx;
    pier.is_active = 1;
    c->write_pier(queue, pier_slot, pier);
}

static constexpr EntityFamilyAdapter COLUMN_ADAPTER = {
    column_run_gate, column_get_theme_tier_weights,
    column_apply_indoor_rescale,
    column_compute_solid_half, column_compute_colors,
    column_write_active, column_write_gpu, column_post_commit,
    column_get_tier_profile,
};

// ── Column dispatch wrappers ──

static bool dispatch_select_column_generic(Cartridge* self, int32_t gx, int32_t gz, EntityQueueEntry& e) {
    EntityInstance inst{};
    if (!self->generic_select(COLUMN_TRAITS, COLUMN_ADAPTER, gx, gz, inst)) return false;
    e.family = PopFamily::COLUMN; e.gx = gx; e.gz = gz; e.generic = inst; return true;
}
static bool dispatch_place_column_generic(Cartridge* self, EntityQueueEntry& e, PlacementEntry& pe) {
    pe.family = e.family; pe.gx = e.gx; pe.gz = e.gz;
    if (self->generic_place(COLUMN_TRAITS, e.generic)) { pe.generic = e.generic; return true; }
    self->entities_state_.columns[e.generic.slot].active = false; return false;
}
static void dispatch_commit_column_generic(Cartridge* self, PlacementEntry& pe, wgpu::Queue& queue) {
    auto* host = self->find_patch(pe.generic.host_gx, pe.generic.host_gz);
    if (host) { self->generic_commit(COLUMN_TRAITS, COLUMN_ADAPTER, pe.generic, queue); host->record_entity(PopFamily::COLUMN, pe.generic.slot); }
    else { self->entities_state_.columns[pe.generic.slot].active = false; }
}


// ── Antenna adapter functions ──

static SpawnGateOutput antenna_run_gate(Cartridge* c, int32_t gx, int32_t gz) {
    auto gate = c->run_spawn_preamble(gx, gz,
        c->entities_state_.antennas, Dim::MAX_ANTENNA_ONLY,
        AntennaProp::SPAWN_ROLL, AntennaConfig::SPAWN_CHANCE,
        AntennaConfig::MOOD_MULTIPLIER, PopFamily::ANTENNA, "ant");
    return { gate.ok, gate.seed, gate.slot, gate.theme_idx };
}
static const float* antenna_get_theme_tier_weights(uint32_t ti) { return THEMES[ti].tier_wt_antenna; }

// Antenna shares the ColumnTierRow / ColIdx layout with Column, so it
// reuses COLUMN_INDOOR_RESCALE_PARAMS. Policy is rolled-target rather
// than snap-to-ceiling — antennas read better as miniaturized than as
// floor-to-ceiling.
static void antenna_apply_indoor_rescale(EntityInstance& inst, float ceiling_h) {
    rescale_to_rolled_target(inst, ceiling_h,
        /*target_lo*/ 0.50f, /*target_hi*/ 0.95f,
        /*current_h*/ inst.params[ColIdx::HEIGHT],
        COLUMN_INDOOR_RESCALE_PARAMS);
}

static void antenna_compute_solid_half(EntityInstance& inst, const TierProfile&) {
    float shaft_r = inst.params[ColIdx::SHAFT_RADIUS];
    float pad     = inst.params[ColIdx::SOLID_PADDING];
    float blend   = inst.params[ColIdx::EDGE_BLEND];
    float solid_h = inst.params[ColIdx::SOLID_HEIGHT];
    inst.solid_half = shaft_r * 2.0f + pad + blend;  // antenna formula: wraps the post
    inst.ground_y_offset = solid_h;
    inst.burial = std::max(0.2f, solid_h * ANTENNA_TIERS[inst.tier_idx].burial);
    // GPU tier offset: antenna tiers map to 3,4,5 on the GPU
    inst.tier_idx = inst.tier_idx + COLUMN_TIER_COUNT;
}

static void antenna_compute_colors(EntityInstance& inst, const EntityFamilyTraits&, const TierProfile& /*tier*/) {
    // Sandstone / palette override (same as column)
    if (cpu_hash_f(inst.seed, AntennaProp::COLOR_OVER) < ANTENNA_TIERS[inst.tier_idx - COLUMN_TIER_COUNT].color_override) {
        uint32_t pal_idx = cpu_hash(inst.seed, AntennaProp::COLOR_OVER + 1u) % COLUMN_PALETTE_COUNT;
        inst.colors[0] = COLUMN_PALETTE[pal_idx][0] + (cpu_hash_f(inst.seed, AntennaProp::COLOR_VAR_R) - 0.5f) * 0.06f;
        inst.colors[1] = COLUMN_PALETTE[pal_idx][1] + (cpu_hash_f(inst.seed, AntennaProp::COLOR_VAR_G) - 0.5f) * 0.06f;
        inst.colors[2] = COLUMN_PALETTE[pal_idx][2] + (cpu_hash_f(inst.seed, AntennaProp::COLOR_VAR_B) - 0.5f) * 0.06f;
    } else {
        inst.colors[0] = COLUMN_SANDSTONE_BASE[0] + (cpu_hash_f(inst.seed, AntennaProp::COLOR_VAR_R) - 0.5f) * COLUMN_SANDSTONE_VARIANCE * 2.0f;
        inst.colors[1] = COLUMN_SANDSTONE_BASE[1] + (cpu_hash_f(inst.seed, AntennaProp::COLOR_VAR_G) - 0.5f) * COLUMN_SANDSTONE_VARIANCE * 2.0f;
        inst.colors[2] = COLUMN_SANDSTONE_BASE[2] + (cpu_hash_f(inst.seed, AntennaProp::COLOR_VAR_B) - 0.5f) * COLUMN_SANDSTONE_VARIANCE * 2.0f;
    }
    // Drum colors: 3 palettes, decorrelated picks
    static constexpr float DRUM_PALETTE[][3] = {
        {0.85f,0.55f,0.35f},{0.45f,0.60f,0.70f},{0.70f,0.65f,0.45f},
        {0.55f,0.70f,0.55f},{0.75f,0.50f,0.55f},{0.60f,0.55f,0.68f},
    };
    static constexpr uint32_t DRUM_PAL_COUNT = 6;
    uint32_t d1 = cpu_hash(inst.seed, 850u) % DRUM_PAL_COUNT;
    uint32_t d2 = (d1 + 1 + cpu_hash(inst.seed, 851u) % (DRUM_PAL_COUNT - 1)) % DRUM_PAL_COUNT;
    uint32_t d3 = (d2 + 1 + cpu_hash(inst.seed, 852u) % (DRUM_PAL_COUNT - 2)) % DRUM_PAL_COUNT;
    float v = 0.04f;
    inst.colors[3]  = DRUM_PALETTE[d1][0] + (cpu_hash_f(inst.seed, 860u) - 0.5f) * v;
    inst.colors[4]  = DRUM_PALETTE[d1][1] + (cpu_hash_f(inst.seed, 861u) - 0.5f) * v;
    inst.colors[5]  = DRUM_PALETTE[d1][2] + (cpu_hash_f(inst.seed, 862u) - 0.5f) * v;
    inst.colors[6]  = DRUM_PALETTE[d2][0] + (cpu_hash_f(inst.seed, 863u) - 0.5f) * v;
    inst.colors[7]  = DRUM_PALETTE[d2][1] + (cpu_hash_f(inst.seed, 864u) - 0.5f) * v;
    inst.colors[8]  = DRUM_PALETTE[d2][2] + (cpu_hash_f(inst.seed, 865u) - 0.5f) * v;
    inst.colors[9]  = DRUM_PALETTE[d3][0] + (cpu_hash_f(inst.seed, 866u) - 0.5f) * v;
    inst.colors[10] = DRUM_PALETTE[d3][1] + (cpu_hash_f(inst.seed, 867u) - 0.5f) * v;
    inst.colors[11] = DRUM_PALETTE[d3][2] + (cpu_hash_f(inst.seed, 868u) - 0.5f) * v;
}

static void antenna_write_active(Cartridge* c, const EntityInstance& inst) {
    auto& ac = c->entities_state_.antennas[inst.slot];
    ac.patch_gx = inst.trigger_gx; ac.patch_gz = inst.trigger_gz;
    ac.host_gx = inst.host_gx; ac.host_gz = inst.host_gz;
    ac.active = true; ac.draw_visible = true;
    ac.world_x = inst.cx; ac.world_z = inst.cz;
    ac.height = inst.params[ColIdx::HEIGHT];
    ac.shaft_radius = inst.params[ColIdx::SHAFT_RADIUS];
    ac.taper = inst.params[ColIdx::TAPER];
    ac.entasis = inst.params[ColIdx::ENTASIS];
    ac.base_layers = (uint32_t)inst.params[ColIdx::BASE_LAYERS];
    ac.base_height = inst.params[ColIdx::BASE_HEIGHT];
    ac.base_overhang = inst.params[ColIdx::BASE_OVERHANG];
    ac.cap_layers = (uint32_t)inst.params[ColIdx::CAP_LAYERS];
    ac.cap_height = inst.params[ColIdx::CAP_HEIGHT];
    ac.cap_overhang = inst.params[ColIdx::CAP_OVERHANG];
    ac.solid_height = inst.params[ColIdx::SOLID_HEIGHT];
    ac.burial = inst.burial;
    ac.segs_around = ANTENNA_TIERS[inst.tier_idx - COLUMN_TIER_COUNT].segs_around;
    ac.shaft_rings = ANTENNA_TIERS[inst.tier_idx - COLUMN_TIER_COUNT].shaft_rings;
    ac.tier_idx = inst.tier_idx;
    ac.cached_ground_y = inst.cached_ground_y;
    ac.col_r = inst.colors[0]; ac.col_g = inst.colors[1]; ac.col_b = inst.colors[2];
    std::memcpy(ac.drum_colors, &inst.colors[3], 9 * sizeof(float));
    c->entities_state_.antenna_count++;
}

static void antenna_write_gpu(Cartridge* c, const EntityInstance& inst, wgpu::Queue& queue) {
    uint32_t gpu_slot = inst.slot + Dim::ANTENNA_SLOT_OFFSET;
    uint32_t raw_tier = inst.tier_idx - COLUMN_TIER_COUNT;
    GPUColumnMeshParams mp{};
    mp.center_x = inst.cx; mp.center_z = inst.cz;
    mp.height = inst.params[ColIdx::HEIGHT];
    mp.shaft_radius = inst.params[ColIdx::SHAFT_RADIUS];
    mp.taper = inst.params[ColIdx::TAPER];
    mp.entasis = inst.params[ColIdx::ENTASIS];
    mp.base_height = inst.params[ColIdx::BASE_HEIGHT];
    mp.base_overhang = inst.params[ColIdx::BASE_OVERHANG];
    mp.capital_height = inst.params[ColIdx::CAP_HEIGHT];
    mp.capital_overhang = inst.params[ColIdx::CAP_OVERHANG];
    mp.burial = inst.burial;
    mp.color_r = inst.colors[0]; mp.color_g = inst.colors[1]; mp.color_b = inst.colors[2];
    mp.base_layers = (uint32_t)inst.params[ColIdx::BASE_LAYERS];
    mp.capital_layers = (uint32_t)inst.params[ColIdx::CAP_LAYERS];
    mp.segs_around = ANTENNA_TIERS[raw_tier].segs_around;
    mp.shaft_rings = ANTENNA_TIERS[raw_tier].shaft_rings;
    mp.is_active = 1;
    mp.tier = inst.tier_idx;  // GPU tier: 3,4,5 for antennas
    mp.drum_color_r1 = inst.colors[3]; mp.drum_color_g1 = inst.colors[4]; mp.drum_color_b1 = inst.colors[5];
    mp.drum_color_r2 = inst.colors[6]; mp.drum_color_g2 = inst.colors[7]; mp.drum_color_b2 = inst.colors[8];
    mp.drum_color_r3 = inst.colors[9]; mp.drum_color_g3 = inst.colors[10]; mp.drum_color_b3 = inst.colors[11];
    c->gpuState_.upload_column_mesh_params_slot(queue, gpu_slot, mp);
    c->entities_state_.column_mesh_gen_pending = true;
}

static void antenna_post_commit(Cartridge* c, const EntityInstance& inst, wgpu::Queue& queue) {
    uint32_t gpu_slot = inst.slot + Dim::ANTENNA_SLOT_OFFSET;
    uint32_t pier_slot = Dim::PIER_COLUMN_BASE + gpu_slot;
    GPUPierInstance pier{};
    pier.origin[0] = inst.cx;
    pier.origin[1] = inst.cz;
    pier.half_size[0] = inst.solid_half;
    pier.half_size[1] = inst.solid_half;
    pier.height_near = inst.params[ColIdx::SOLID_HEIGHT];
    pier.height_far = inst.params[ColIdx::SOLID_HEIGHT];
    pier.rotation = 0.0f;
    pier.edge_blend = inst.params[ColIdx::EDGE_BLEND];
    pier.tier = PierTier::COL_PILLAR + inst.tier_idx;
    pier.is_active = 1;
    c->write_pier(queue, pier_slot, pier);
}

static constexpr EntityFamilyAdapter ANTENNA_ADAPTER = {
    antenna_run_gate, antenna_get_theme_tier_weights,
    antenna_apply_indoor_rescale,
    antenna_compute_solid_half, antenna_compute_colors,
    antenna_write_active, antenna_write_gpu, antenna_post_commit,
    antenna_get_tier_profile,
};

// ── Antenna dispatch wrappers ──

static bool dispatch_select_antenna_generic(Cartridge* self, int32_t gx, int32_t gz, EntityQueueEntry& e) {
    EntityInstance inst{};
    if (!self->generic_select(ANTENNA_TRAITS, ANTENNA_ADAPTER, gx, gz, inst)) return false;
    e.family = PopFamily::ANTENNA; e.gx = gx; e.gz = gz; e.generic = inst; return true;
}
static bool dispatch_place_antenna_generic(Cartridge* self, EntityQueueEntry& e, PlacementEntry& pe) {
    pe.family = e.family; pe.gx = e.gx; pe.gz = e.gz;
    if (self->generic_place(ANTENNA_TRAITS, e.generic)) { pe.generic = e.generic; return true; }
    self->entities_state_.antennas[e.generic.slot].active = false; return false;
}
static void dispatch_commit_antenna_generic(Cartridge* self, PlacementEntry& pe, wgpu::Queue& queue) {
    auto* host = self->find_patch(pe.generic.host_gx, pe.generic.host_gz);
    if (host) { self->generic_commit(ANTENNA_TRAITS, ANTENNA_ADAPTER, pe.generic, queue); host->record_entity(PopFamily::ANTENNA, pe.generic.slot); }
    else { self->entities_state_.antennas[pe.generic.slot].active = false; }
}


// ═══ FAMILY: PYRAMID ══════════════════════════════════════════════
//
// Heightfield-baking entity (GPUPyramidArray + mesh gen).
//


struct PyrIdx {
    static constexpr uint32_t HEIGHT     = 0;
    static constexpr uint32_t BASE_HALF  = 1;
    static constexpr uint32_t ASPECT     = 2;
    static constexpr uint32_t TRUNCATION = 3;
    static constexpr uint32_t EDGE_BLEND = 4;
    static constexpr uint32_t COUNT      = 5;
};

static constexpr TierParamDef PYRAMID_PARAM_DEFS[] = {
    { PyramidProp::HEIGHT,     20.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { PyramidProp::BASE_HALF,  5.0f,  1e30f, false, ParamDist::GAUSSIAN },
    { PyramidProp::ASPECT,     0.5f,  2.0f,  false, ParamDist::GAUSSIAN },  // ceiling 2.0
    { PyramidProp::TRUNCATION, 0.0f,  0.5f,  false, ParamDist::GAUSSIAN },  // ceiling 0.5
    { PyramidProp::EDGE_BLEND, 0.5f,  1e30f, false, ParamDist::GAUSSIAN },
};
static constexpr uint32_t PYRAMID_PARAM_COUNT = sizeof(PYRAMID_PARAM_DEFS) / sizeof(TierParamDef);

// params[] order MUST match PYRAMID_PARAM_DEFS:
//   [0]HEIGHT [1]BASE_HALF [2]ASPECT [3]TRUNCATION [4]EDGE_BLEND
// Note: cannot reuse `PyramidTier` as the struct name — entities.inl
// declares `enum class PyramidTier`, which occupies the same name slot.
// `PyramidTierRow` keeps the new struct distinct without renaming the
// enum (used widely as PyramidTier::OBELISK etc.).
struct PyramidTierRow {
    TierProfile profile;
    float color_override;
    float color_variance;
};

static constexpr PyramidTierRow PYRAMID_TIERS[] = {
    /* OBELISK  */ {
        { 0.50f, 0.0f, { {28.0f, 6.0f},  {16.0f, 3.0f},  {1.0f, 0.15f}, {0.00f, 0.00f}, {1.5f, 0.3f}  }},
        0.10f, 0.04f
    },
    /* TEMPLE   */ {
        { 0.25f, 0.0f, { {45.0f, 8.0f},  {40.0f, 6.0f},  {1.0f, 0.20f}, {0.25f, 0.08f}, {3.0f, 0.75f} }},
        0.15f, 0.04f
    },
    /* COLOSSUS */ {
        { 0.25f, 0.0f, { {78.0f, 14.4f}, {60.0f, 9.6f},  {1.0f, 0.10f}, {0.05f, 0.04f}, {3.6f, 1.0f}  }},
        0.20f, 0.04f
    },
};

static const TierProfile& pyramid_get_tier_profile(uint32_t tier_idx) {
    return PYRAMID_TIERS[tier_idx].profile;
}

static constexpr EntityFamilyTraits PYRAMID_TRAITS = {
    PopFamily::PYRAMID, "pyr", Dim::MAX_PYRAMID_INSTANCES,
    true, false, 0,       // grounded, no piers (bakes into heightfield instead)
    true, 200.0f, 1.0f,
    PyramidProp::SPAWN_ROLL, PyramidConfig::SPAWN_CHANCE,
    PyramidConfig::MOOD_MULTIPLIER, PyramidConfig::POSITION_JITTER,
    3, PyramidProp::TIER,
    PYRAMID_PARAM_DEFS, PYRAMID_PARAM_COUNT,
    PyramidProp::POSITION_X, PyramidProp::POSITION_Z, PyramidProp::ROTATION, true,
    0, nullptr,  // color handled by adapter
};

static SpawnGateOutput pyramid_run_gate(Cartridge* c, int32_t gx, int32_t gz) {
    auto gate = c->run_spawn_preamble(gx, gz,
        c->entities_state_.pyramids, Dim::MAX_PYRAMID_INSTANCES,
        PyramidProp::SPAWN_ROLL, PyramidConfig::SPAWN_CHANCE,
        PyramidConfig::MOOD_MULTIPLIER, PopFamily::PYRAMID, "pyr");
    return { gate.ok, gate.seed, gate.slot, gate.theme_idx };
}
static const float* pyramid_get_theme_tier_weights(uint32_t ti) { return THEMES[ti].tier_wt_pyramid; }

static constexpr uint32_t PYRAMID_INDOOR_RESCALE_PARAMS[] = {
    PyrIdx::HEIGHT, PyrIdx::BASE_HALF, PyrIdx::EDGE_BLEND,
    // ASPECT, TRUNCATION are ratios — not scaled.
};

static void pyramid_apply_indoor_rescale(EntityInstance& inst, float ceiling_h) {
    rescale_to_rolled_target(inst, ceiling_h,
        /*target_lo*/ 0.50f, /*target_hi*/ 0.95f,
        /*current_h*/ inst.params[PyrIdx::HEIGHT],
        PYRAMID_INDOOR_RESCALE_PARAMS);
}

static void pyramid_compute_solid_half(EntityInstance& inst, const TierProfile&) {
    float base_half = inst.params[PyrIdx::BASE_HALF];
    float aspect    = inst.params[PyrIdx::ASPECT];
    float half_x = base_half;
    float half_z = base_half * aspect;

    // Proportion constraint: height ≤ 1.5 × longest base side
    float max_h = 1.5f * 2.0f * std::max(half_x, half_z);
    inst.params[PyrIdx::HEIGHT] = std::min(inst.params[PyrIdx::HEIGHT], max_h);

    inst.solid_half = std::max(half_x, half_z) + inst.params[PyrIdx::EDGE_BLEND];
    inst.burial = 0.0f;
    inst.ground_y_offset = 0.0f;
}

static void pyramid_compute_colors(EntityInstance& inst, const EntityFamilyTraits&, const TierProfile& /*tier*/) {
    // Sandstone only (color_override check is dead code in legacy — both branches identical)
    inst.colors[0] = PYRAMID_SANDSTONE_BASE[0] + (cpu_hash_f(inst.seed, PyramidProp::COLOR_VAR_R) - 0.5f) * PYRAMID_SANDSTONE_VARIANCE * 2.0f;
    inst.colors[1] = PYRAMID_SANDSTONE_BASE[1] + (cpu_hash_f(inst.seed, PyramidProp::COLOR_VAR_G) - 0.5f) * PYRAMID_SANDSTONE_VARIANCE * 2.0f;
    inst.colors[2] = PYRAMID_SANDSTONE_BASE[2] + (cpu_hash_f(inst.seed, PyramidProp::COLOR_VAR_B) - 0.5f) * PYRAMID_SANDSTONE_VARIANCE * 2.0f;
}

static void pyramid_write_active(Cartridge* c, const EntityInstance& inst) {
    auto& ap = c->entities_state_.pyramids[inst.slot];
    ap.patch_gx = inst.trigger_gx; ap.patch_gz = inst.trigger_gz;
    ap.host_gx = inst.host_gx; ap.host_gz = inst.host_gz;
    ap.active = true;
    ap.col_r = inst.colors[0]; ap.col_g = inst.colors[1]; ap.col_b = inst.colors[2];

    // 5-point ground_y: GPU compute shader evaluates the heightfield
    // at center + 4 rotated corners and takes the min. CPU uploads 0.
    ap.cached_ground_y = 0.0f;

    c->entities_state_.pyramid_count++;
}

static void pyramid_write_gpu(Cartridge* c, const EntityInstance& inst, wgpu::Queue& queue) {
    float half_x = inst.params[PyrIdx::BASE_HALF];
    float half_z = inst.params[PyrIdx::BASE_HALF] * inst.params[PyrIdx::ASPECT];

    // Write GPU solid instance (heightfield baking)
    GPUPyramidInstance gpu_inst{};
    gpu_inst.origin[0] = inst.cx;
    gpu_inst.origin[1] = inst.cz;
    gpu_inst.half_size[0] = half_x;
    gpu_inst.half_size[1] = half_z;
    gpu_inst.height = inst.params[PyrIdx::HEIGHT];
    gpu_inst.rotation = inst.rotation;
    gpu_inst.edge_blend = inst.params[PyrIdx::EDGE_BLEND];
    gpu_inst.truncation = inst.params[PyrIdx::TRUNCATION];
    c->entities_state_.cpu_pyramids.instances[inst.slot] = gpu_inst;

    uint32_t max_idx = 0;
    for (uint32_t i = 0; i < Dim::MAX_PYRAMID_INSTANCES; i++) {
        if (i == inst.slot || c->entities_state_.pyramids[i].active) max_idx = i + 1;
    }
    c->entities_state_.cpu_pyramids.count = max_idx;
    c->gpuState_.upload_pyramids(queue, c->entities_state_.cpu_pyramids);

    // Write mesh gen params
    GPUPyramidMeshParams mp{};
    mp.center_x = inst.cx; mp.center_z = inst.cz;
    mp.rotation = inst.rotation;
    mp.half_x = half_x; mp.half_z = half_z;
    mp.height = inst.params[PyrIdx::HEIGHT];
    mp.truncation = inst.params[PyrIdx::TRUNCATION];
    mp.color_r = inst.colors[0]; mp.color_g = inst.colors[1]; mp.color_b = inst.colors[2];
    mp.is_active = 1;
    c->gpuState_.upload_pyramid_mesh_params_slot(queue, inst.slot, mp);
    c->entities_state_.pyramid_mesh_gen_pending = true;
}

static void pyramid_post_commit(Cartridge* c, const EntityInstance& inst, wgpu::Queue&) {
    // Mark heightfield patches for regen (pyramid gets baked in)
    float half_x = inst.params[PyrIdx::BASE_HALF];
    float half_z = inst.params[PyrIdx::BASE_HALF] * inst.params[PyrIdx::ASPECT];
    float blend  = inst.params[PyrIdx::EDGE_BLEND];
    float cr = std::cos(inst.rotation), sr = std::sin(inst.rotation);
    float abs_cr = std::abs(cr), abs_sr = std::abs(sr);
    float ext_x = (half_x + blend) * abs_cr + (half_z + blend) * abs_sr;
    float ext_z = (half_x + blend) * abs_sr + (half_z + blend) * abs_cr;
    c->mark_patches_for_regen(
        inst.cx - ext_x, inst.cz - ext_z,
        inst.cx + ext_x, inst.cz + ext_z,
        inst.host_gx, inst.host_gz);
}

static constexpr EntityFamilyAdapter PYRAMID_ADAPTER = {
    pyramid_run_gate, pyramid_get_theme_tier_weights,
    pyramid_apply_indoor_rescale,
    pyramid_compute_solid_half, pyramid_compute_colors,
    pyramid_write_active, pyramid_write_gpu, pyramid_post_commit,
    pyramid_get_tier_profile,
};

// ── Pyramid dispatch wrappers ──

static bool dispatch_select_pyramid_generic(Cartridge* self, int32_t gx, int32_t gz, EntityQueueEntry& e) {
    EntityInstance inst{};
    if (!self->generic_select(PYRAMID_TRAITS, PYRAMID_ADAPTER, gx, gz, inst)) return false;
    e.family = PopFamily::PYRAMID; e.gx = gx; e.gz = gz; e.generic = inst; return true;
}
static bool dispatch_place_pyramid_generic(Cartridge* self, EntityQueueEntry& e, PlacementEntry& pe) {
    pe.family = e.family; pe.gx = e.gx; pe.gz = e.gz;
    if (self->generic_place(PYRAMID_TRAITS, e.generic)) { pe.generic = e.generic; return true; }
    self->entities_state_.pyramids[e.generic.slot].active = false; return false;
}
static void dispatch_commit_pyramid_generic(Cartridge* self, PlacementEntry& pe, wgpu::Queue& queue) {
    auto* host = self->find_patch(pe.generic.host_gx, pe.generic.host_gz);
    if (host) { self->generic_commit(PYRAMID_TRAITS, PYRAMID_ADAPTER, pe.generic, queue); host->record_entity(PopFamily::PYRAMID, pe.generic.slot); }
    else { self->entities_state_.pyramids[pe.generic.slot].active = false; }
}


// ═══ FAMILY: SPHERE ═══════════════════════════════════════════════
//
// Orbital floating entity. No ground contact.
//


struct SphIdx {
    static constexpr uint32_t BODY_RADIUS      = 0;
    static constexpr uint32_t ORBIT_RADIUS     = 1;
    static constexpr uint32_t ORBIT_HEIGHT     = 2;
    static constexpr uint32_t ORBIT_SPEED      = 3;
    static constexpr uint32_t INFLUENCE_RADIUS = 4;
    static constexpr uint32_t COUNT            = 5;
};

static constexpr TierParamDef SPHERE_PARAM_DEFS[] = {
    { FloatingEntityProp::BODY_RADIUS,      0.5f,  1e30f, false, ParamDist::GAUSSIAN },
    { FloatingEntityProp::ORBIT_RADIUS,     0.0f,  1e30f, false, ParamDist::GAUSSIAN },
    { FloatingEntityProp::ORBIT_HEIGHT,     3.0f,  1e30f, false, ParamDist::GAUSSIAN },
    { FloatingEntityProp::ORBIT_SPEED,      0.05f, 1e30f, false, ParamDist::GAUSSIAN },
    { FloatingEntityProp::INFLUENCE_RADIUS, 3.0f,  1e30f, false, ParamDist::GAUSSIAN },
};
static constexpr uint32_t SPHERE_PARAM_COUNT = sizeof(SPHERE_PARAM_DEFS) / sizeof(TierParamDef);

// params[] order MUST match SPHERE_PARAM_DEFS:
//   [0]BODY_RADIUS [1]ORBIT_RADIUS [2]ORBIT_HEIGHT [3]ORBIT_SPEED
//   [4]INFLUENCE_RADIUS
//
// Note: the original SphereTierProfile carried a number of fields
// (spin_speed, bob_amplitude/period, spin_tilt, aspect_y/z, face_variance,
// geometry_type, motion_type) that the sphere adapter never reads —
// sphere_write_gpu hardcodes those slots in the GPU upload. We preserve
// them here verbatim per the migration spec ("preserve all numeric
// values"); dropping the dead fields would be a separate cleanup.
struct SphereTierRow {
    TierProfile profile;
    // Dead-but-preserved extras (not consumed by sphere adapter).
    float       spin_speed_mean,    spin_speed_sigma;
    float       bob_amplitude_mean, bob_amplitude_sigma;
    float       bob_period_mean,    bob_period_sigma;
    float       spin_tilt_sigma;
    float       aspect_y_mean,      aspect_y_sigma;
    float       aspect_z_mean,      aspect_z_sigma;
    float       face_variance_mean, face_variance_sigma;
    uint32_t    geometry_type;
    uint32_t    motion_type;
};

static constexpr SphereTierRow SPHERE_TIERS[SPHERE_TIER_COUNT] = {
    /* 0: Sentinel */ {
        { 0.65f, 0.0f, { {1.5f, 0.3f}, {12.0f, 3.0f}, {6.0f, 2.0f}, {1.4f, 0.3f}, {8.0f, 2.0f} }},
        0.0f, 0.0f,  0.0f, 0.0f,  0.0f, 0.0f,  0.0f,
        1.0f, 0.0f,  1.0f, 0.0f,  0.0f, 0.0f,  0, 0
    },
    /* 1: Anomaly  */ {
        { 0.35f, 0.0f, { {1.2f, 0.2f}, {8.0f, 2.0f},  {4.0f, 1.5f}, {2.0f, 0.5f}, {6.0f, 1.5f} }},
        0.0f, 0.0f,  0.0f, 0.0f,  0.0f, 0.0f,  0.0f,
        1.0f, 0.0f,  1.0f, 0.0f,  0.0f, 0.0f,  0, 0
    },
};

static const TierProfile& sphere_get_tier_profile(uint32_t tier_idx) {
    return SPHERE_TIERS[tier_idx].profile;
}

static constexpr EntityFamilyTraits SPHERE_TRAITS = {
    PopFamily::SPHERE, "sph", Dim::MAX_SPHERE_INSTANCES,
    false, false, 0,      // not grounded
    true, 200.0f, 0.0f,
    FloatingEntityProp::SPAWN_ROLL, SphereConfig::SPAWN_CHANCE,
    SphereConfig::MOOD_MULTIPLIER, SphereConfig::POSITION_JITTER,
    SPHERE_TIER_COUNT, FloatingEntityProp::TIER,
    SPHERE_PARAM_DEFS, SPHERE_PARAM_COUNT,
    FloatingEntityProp::ANCHOR_X, FloatingEntityProp::ANCHOR_Z, FloatingEntityProp::ROTATION, false,
    0, nullptr,
};

static SpawnGateOutput sphere_run_gate(Cartridge* c, int32_t gx, int32_t gz) {
    auto gate = c->run_spawn_preamble(gx, gz, c->activeFloaters_, Dim::MAX_SPHERE_INSTANCES,
        FloatingEntityProp::SPAWN_ROLL, SphereConfig::SPAWN_CHANCE,
        SphereConfig::MOOD_MULTIPLIER, PopFamily::SPHERE, "sph");
    return { gate.ok, gate.seed, gate.slot, gate.theme_idx };
}
static const float* sphere_get_theme_tier_weights(uint32_t ti) { return THEMES[ti].tier_wt_sphere; }

static void sphere_compute_solid_half(EntityInstance& inst, const TierProfile&) {
    inst.solid_half = inst.params[SphIdx::BODY_RADIUS] + inst.params[SphIdx::ORBIT_RADIUS];
    inst.ground_y_offset = 0.0f;
    inst.burial = 0.0f;
}

static void sphere_compute_colors(EntityInstance& inst, const EntityFamilyTraits&, const TierProfile& /*tier*/) {
    inst.colors[0] = cpu_hash_f(inst.seed, FloatingEntityProp::COLOR_R) * 0.55f + 0.35f;
    inst.colors[1] = cpu_hash_f(inst.seed, FloatingEntityProp::COLOR_G) * 0.50f + 0.30f;
    inst.colors[2] = cpu_hash_f(inst.seed, FloatingEntityProp::COLOR_B) * 0.60f + 0.20f;
}

static void sphere_write_active(Cartridge* c, const EntityInstance& inst) {
    auto& af = c->activeFloaters_[inst.slot];
    af.patch_gx = inst.trigger_gx; af.patch_gz = inst.trigger_gz;
    af.host_gx = inst.host_gx; af.host_gz = inst.host_gz;
    af.last_alloc_time = c->time_state_.seconds;
    af.active = true;
    c->activeFloaterCount_++;
}

static void sphere_write_gpu(Cartridge* c, const EntityInstance& inst, wgpu::Queue& queue) {
    GPUFloatingEntityState fe{};
    fe.anchor[0] = inst.cx; fe.anchor[1] = 0.0f; fe.anchor[2] = inst.cz;
    fe.body_radius = inst.params[SphIdx::BODY_RADIUS];
    fe.orbit_radius = inst.params[SphIdx::ORBIT_RADIUS];
    fe.orbit_height = inst.params[SphIdx::ORBIT_HEIGHT];
    fe.orbit_speed = inst.params[SphIdx::ORBIT_SPEED];
    fe.influence_radius = inst.params[SphIdx::INFLUENCE_RADIUS];
    fe.spin_speed = 0.0f; fe.bob_amplitude = 0.0f; fe.bob_period = 1.0f;
    fe.spin_tilt_x = 0.0f; fe.spin_tilt_z = 0.0f;
    fe.base_color[0] = inst.colors[0]; fe.base_color[1] = inst.colors[1]; fe.base_color[2] = inst.colors[2];
    fe.color[0] = inst.colors[0]; fe.color[1] = inst.colors[1]; fe.color[2] = inst.colors[2];
    fe.aspect_y = 1.0f; fe.aspect_z = 1.0f; fe.face_variance = 0.0f;
    fe.geometry_type = 0; fe.motion_type = 0;
    fe.entity_seed = inst.slot;
    fe.t = 0.0f; fe.orientation[3] = 1.0f;
    fe.pos[0] = inst.cx + fe.orbit_radius; fe.pos[1] = fe.orbit_height; fe.pos[2] = inst.cz;
    fe.is_active = 1;
    c->gpuState_.upload_sphere_entity_slot(queue, inst.slot, fe);
}

static constexpr EntityFamilyAdapter SPHERE_ADAPTER = {
    sphere_run_gate, sphere_get_theme_tier_weights,
    nullptr,                              // apply_indoor_rescale → not eligible (floaters, not grounded)
    sphere_compute_solid_half, sphere_compute_colors,
    sphere_write_active, sphere_write_gpu, nullptr,
    sphere_get_tier_profile,
};

static bool dispatch_select_sphere_generic(Cartridge* self, int32_t gx, int32_t gz, EntityQueueEntry& e) {
    EntityInstance inst{};
    if (!self->generic_select(SPHERE_TRAITS, SPHERE_ADAPTER, gx, gz, inst)) return false;
    e.family = PopFamily::SPHERE; e.gx = gx; e.gz = gz; e.generic = inst; return true;
}
static bool dispatch_place_sphere_generic(Cartridge* self, EntityQueueEntry& e, PlacementEntry& pe) {
    pe.family = e.family; pe.gx = e.gx; pe.gz = e.gz;
    if (self->generic_place(SPHERE_TRAITS, e.generic)) { pe.generic = e.generic; return true; }
    self->activeFloaters_[e.generic.slot].active = false; return false;
}
static void dispatch_commit_sphere_generic(Cartridge* self, PlacementEntry& pe, wgpu::Queue& queue) {
    auto* host = self->find_patch(pe.generic.host_gx, pe.generic.host_gz);
    if (host) {
        self->generic_commit(SPHERE_TRAITS, SPHERE_ADAPTER, pe.generic, queue);
        // Lifecycle Phase 2: sphere lifetime is no longer tied to its
        // host patch. We don't call host->record_entity() here, so
        // evict_patch_entities will never dispatch_evict_sphere on this
        // slot — the GPU-side pawn-distance test in update_sphere is
        // the sole eviction path. The find_patch() lookup is retained
        // because a missing host still means "spawn was invalid"; we
        // just don't link the sphere into the patch's eviction list.
        //
        // CPU's activeFloaters_[slot].active stays true until the next
        // mood transition zeroes the buffer. With 8 slots and a 1.5%
        // spawn chance, allocator pressure from stale CPU bools is
        // unlikely in practice; if it surfaces, add a continuous readback
        // mirroring agent_state_readback_staging (cartridge.hpp ~7990).
    }
    else { self->activeFloaters_[pe.generic.slot].active = false; }
}


// ═══ FAMILY: CUBE ═════════════════════════════════════════════════
//
// Hover-bob monolith. No ground contact.
//


struct CubeIdx {
    static constexpr uint32_t BODY_RADIUS      = 0;
    static constexpr uint32_t ORBIT_HEIGHT     = 1;
    static constexpr uint32_t INFLUENCE_RADIUS = 2;
    static constexpr uint32_t SPIN_SPEED       = 3;
    static constexpr uint32_t BOB_AMPLITUDE    = 4;
    static constexpr uint32_t BOB_PERIOD       = 5;
    static constexpr uint32_t ASPECT_Y         = 6;
    static constexpr uint32_t ASPECT_Z         = 7;
    static constexpr uint32_t FACE_VARIANCE    = 8;
    static constexpr uint32_t COUNT            = 9;
};

// Cube substrate constants (CUBE_DEFAULT_SPRING_STIFFNESS, CUBE_DEFAULT_DRAG)
// and registry helpers (apply_cube_tier_gains, pick_cube_behavior_for_spawn)
// live in modules/cube_behaviors.inl. cube_write_gpu calls into them at spawn time.

static constexpr TierParamDef CUBE_PARAM_DEFS[] = {
    { CubeEntityProp::BODY_RADIUS,      0.5f, 1e30f, false, ParamDist::GAUSSIAN },
    { CubeEntityProp::ORBIT_HEIGHT,     3.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { CubeEntityProp::INFLUENCE_RADIUS, 3.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { CubeEntityProp::SPIN_SPEED,       0.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { CubeEntityProp::BOB_AMPLITUDE,    0.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { CubeEntityProp::BOB_PERIOD,       0.5f, 1e30f, false, ParamDist::GAUSSIAN },
    { CubeEntityProp::ASPECT_Y,         0.2f, 1e30f, false, ParamDist::GAUSSIAN },
    { CubeEntityProp::ASPECT_Z,         0.1f, 1e30f, false, ParamDist::GAUSSIAN },
    { CubeEntityProp::FACE_VARIANCE,    0.0f, 1e30f, false, ParamDist::GAUSSIAN },
};
static constexpr uint32_t CUBE_PARAM_COUNT = sizeof(CUBE_PARAM_DEFS) / sizeof(TierParamDef);

// params[] order MUST match CUBE_PARAM_DEFS:
//   [0]BODY_RADIUS [1]ORBIT_HEIGHT [2]INFLUENCE_RADIUS [3]SPIN_SPEED
//   [4]BOB_AMPLITUDE [5]BOB_PERIOD [6]ASPECT_Y [7]ASPECT_Z [8]FACE_VARIANCE
struct CubeTierRow {
    TierProfile profile;
    float       spin_tilt_sigma;
};

static constexpr CubeTierRow CUBE_TIERS[CUBE_TIER_COUNT] = {
    /* 0: SmallCube */ {
        { 0.40f, 0.0f, { {1.8f, 0.5f}, {25.0f, 20.0f}, {6.0f, 1.5f},  {0.04f, 0.015f},
                   {1.0f, 0.3f}, {5.0f, 1.5f},
                   {1.0f, 0.15f}, {1.0f, 0.15f}, {0.40f, 0.12f} }},
        0.12f
    },
    /* 1: MedCube   */ {
        { 0.32f, 0.0f, { {4.0f, 1.2f}, {45.0f, 30.0f}, {10.0f, 2.0f}, {0.03f, 0.01f},
                   {1.5f, 0.4f}, {6.0f, 2.0f},
                   {1.0f, 0.20f}, {1.0f, 0.20f}, {0.45f, 0.15f} }},
        0.10f
    },
    /* 2: LargeCube */ {
        { 0.20f, 0.0f, { {8.0f, 2.5f}, {75.0f, 45.0f}, {14.0f, 3.0f}, {0.02f, 0.008f},
                   {2.0f, 0.5f}, {8.0f, 2.5f},
                   {1.0f, 0.25f}, {1.0f, 0.25f}, {0.35f, 0.10f} }},
        0.08f
    },
    /* 3: Monolith  */ {
        { 0.08f, 0.0f, { {3.0f, 0.8f}, {12.0f, 8.0f}, {12.0f, 3.0f}, {0.015f, 0.005f},
                   {1.2f, 0.3f}, {6.0f, 2.0f},
                   {5.0f, 1.2f}, {0.15f, 0.03f}, {0.45f, 0.12f} }},
        0.10f
    },
};

static const TierProfile& cube_get_tier_profile(uint32_t tier_idx) {
    return CUBE_TIERS[tier_idx].profile;
}

static constexpr EntityFamilyTraits CUBE_TRAITS = {
    PopFamily::CUBE, "cube", Dim::MAX_CUBE_INSTANCES,
    false, false, 0,
    true, 200.0f, 0.0f,
    CubeEntityProp::SPAWN_ROLL, CubeConfig::SPAWN_CHANCE,
    CubeConfig::MOOD_MULTIPLIER, CubeConfig::POSITION_JITTER,
    CUBE_TIER_COUNT, CubeEntityProp::TIER,
    CUBE_PARAM_DEFS, CUBE_PARAM_COUNT,
    CubeEntityProp::ANCHOR_X, CubeEntityProp::ANCHOR_Z, CubeEntityProp::ROTATION, false,
    0, nullptr,
};

static SpawnGateOutput cube_run_gate(Cartridge* c, int32_t gx, int32_t gz) {
    auto gate = c->run_spawn_preamble(gx, gz, c->activeCubes_, Dim::MAX_CUBE_INSTANCES,
        CubeEntityProp::SPAWN_ROLL, CubeConfig::SPAWN_CHANCE,
        CubeConfig::MOOD_MULTIPLIER, PopFamily::CUBE, "cube");
    return { gate.ok, gate.seed, gate.slot, gate.theme_idx };
}
static const float* cube_get_theme_tier_weights(uint32_t ti) { return THEMES[ti].tier_wt_cube; }

static void cube_compute_solid_half(EntityInstance& inst, const TierProfile&) {
    inst.solid_half = inst.params[CubeIdx::BODY_RADIUS];
    inst.ground_y_offset = 0.0f;
    inst.burial = 0.0f;
}

static void cube_compute_colors(EntityInstance& inst, const EntityFamilyTraits&, const TierProfile& /*tier*/) {
    inst.colors[0] = cpu_hash_f(inst.seed, CubeEntityProp::COLOR_R) * 0.55f + 0.35f;
    inst.colors[1] = cpu_hash_f(inst.seed, CubeEntityProp::COLOR_G) * 0.50f + 0.30f;
    inst.colors[2] = cpu_hash_f(inst.seed, CubeEntityProp::COLOR_B) * 0.60f + 0.20f;
}

static void cube_write_active(Cartridge* c, const EntityInstance& inst) {
    auto& ac = c->activeCubes_[inst.slot];
    ac.patch_gx = inst.trigger_gx; ac.patch_gz = inst.trigger_gz;
    ac.host_gx = inst.host_gx; ac.host_gz = inst.host_gz;
    ac.cx = inst.cx; ac.cz = inst.cz;
    ac.last_alloc_time = c->time_state_.seconds;
    ac.active = true;
    c->activeCubeCount_++;
}

static void cube_write_gpu(Cartridge* c, const EntityInstance& inst, wgpu::Queue& queue) {
    // Spin tilt: custom derivation from tier constant (not a sampled param)
    float tilt_sigma = CUBE_TIERS[inst.tier_idx].spin_tilt_sigma;
    float tilt_x = (cpu_hash_f(inst.seed, CubeEntityProp::SPIN_TILT_X) - 0.5f) * 2.0f * tilt_sigma;
    float tilt_z = (cpu_hash_f(inst.seed, CubeEntityProp::SPIN_TILT_Z) - 0.5f) * 2.0f * tilt_sigma;

    GPUFloatingEntityState fe{};
    fe.anchor[0] = inst.cx; fe.anchor[1] = 0.0f; fe.anchor[2] = inst.cz;
    fe.body_radius = inst.params[CubeIdx::BODY_RADIUS];
    fe.orbit_radius = 0.0f;
    fe.orbit_height = inst.params[CubeIdx::ORBIT_HEIGHT];
    fe.orbit_speed = 0.0f;
    fe.influence_radius = inst.params[CubeIdx::INFLUENCE_RADIUS];
    fe.spin_speed = inst.params[CubeIdx::SPIN_SPEED];
    fe.bob_amplitude = inst.params[CubeIdx::BOB_AMPLITUDE];
    fe.bob_period = inst.params[CubeIdx::BOB_PERIOD];
    fe.spin_tilt_x = tilt_x; fe.spin_tilt_z = tilt_z;
    fe.base_color[0] = inst.colors[0]; fe.base_color[1] = inst.colors[1]; fe.base_color[2] = inst.colors[2];
    fe.color[0] = inst.colors[0]; fe.color[1] = inst.colors[1]; fe.color[2] = inst.colors[2];
    fe.aspect_y = inst.params[CubeIdx::ASPECT_Y];
    fe.aspect_z = inst.params[CubeIdx::ASPECT_Z];
    fe.face_variance = inst.params[CubeIdx::FACE_VARIANCE];
    fe.geometry_type = 1; fe.motion_type = 1;
    fe.entity_seed = Dim::CUBE_SLOT_OFFSET + inst.slot;
    fe.t = 0.0f; fe.orientation[3] = 1.0f;
    fe.pos[0] = inst.cx; fe.pos[1] = fe.orbit_height; fe.pos[2] = inst.cz;
    fe.is_active = 1;
    // Drift-integrator substrate. drift / drift_vel start at zero so the
    // first frame's position equals home; with behavior_force = 0 (default
    // Stationary population), the spring is at rest, drift_vel stays zero,
    // and pos == home — same visual as pre-Phase-3 hover-bob.
    //
    // Spring/drag start at the system defaults defined in cube_behaviors.inl,
    // then pass through apply_cube_tier_gains so each tier gets its own
    // dynamics signature baked in at spawn.
    fe.spring_stiffness = CUBE_DEFAULT_SPRING_STIFFNESS;
    fe.drag             = CUBE_DEFAULT_DRAG;
    fe.tier_idx = inst.tier_idx;
    apply_cube_tier_gains(fe.spring_stiffness, fe.drag, inst.tier_idx);
    fe.drift[0] = 0.0f; fe.drift[1] = 0.0f; fe.drift[2] = 0.0f;
    fe.drift_vel[0] = 0.0f; fe.drift_vel[1] = 0.0f; fe.drift_vel[2] = 0.0f;
    // Behavior assignment. Population picks one of CUBE_BEHAVIOR_* per
    // the active mood's weights (see CUBE_POPULATIONS). behavior_phase
    // is a per-slot u32 used by behaviors that need decorrelated sampling
    // (CurlField's noise origin, PhaseWave's individual offset). Mix
    // constant differs from pick_cube_behavior_for_spawn's so the two
    // derived values are statistically independent.
    fe.behavior_id    = pick_cube_behavior_for_spawn(c->mood_state_.active, inst.seed);
    fe.behavior_phase = cpu_hash(inst.seed, 0xF10A7E70u);
    // Kite mode starts disabled — cube is anchored to its spawn patch
    // until the user explicitly toggles kite mode via cube_behaviors.inl.
    fe.follow_pawn = 0;
    fe.pawn_offset[0] = 0.0f; fe.pawn_offset[1] = 0.0f; fe.pawn_offset[2] = 0.0f;
    c->gpuState_.upload_cube_entity_slot(queue, inst.slot, fe);
}

static constexpr EntityFamilyAdapter CUBE_ADAPTER = {
    cube_run_gate, cube_get_theme_tier_weights,
    nullptr,                              // apply_indoor_rescale → not eligible (floaters, not grounded)
    cube_compute_solid_half, cube_compute_colors,
    cube_write_active, cube_write_gpu, nullptr,
    cube_get_tier_profile,
};

static bool dispatch_select_cube_generic(Cartridge* self, int32_t gx, int32_t gz, EntityQueueEntry& e) {
    EntityInstance inst{};
    if (!self->generic_select(CUBE_TRAITS, CUBE_ADAPTER, gx, gz, inst)) return false;
    e.family = PopFamily::CUBE; e.gx = gx; e.gz = gz; e.generic = inst; return true;
}
static bool dispatch_place_cube_generic(Cartridge* self, EntityQueueEntry& e, PlacementEntry& pe) {
    pe.family = e.family; pe.gx = e.gx; pe.gz = e.gz;
    if (self->generic_place(CUBE_TRAITS, e.generic)) { pe.generic = e.generic; return true; }
    self->activeCubes_[e.generic.slot].active = false; return false;
}
static void dispatch_commit_cube_generic(Cartridge* self, PlacementEntry& pe, wgpu::Queue& queue) {
    auto* host = self->find_patch(pe.generic.host_gx, pe.generic.host_gz);
    if (host) {
        self->generic_commit(CUBE_TRAITS, CUBE_ADAPTER, pe.generic, queue);
        // Lifecycle Phase 2: cube lifetime decoupled from host patch.
        // See dispatch_commit_sphere_generic for the rationale.
    }
    else { self->activeCubes_[pe.generic.slot].active = false; }
}


// ═══ FAMILY: ARCH ═════════════════════════════════════════════════
//
// 2-pier catenary entity with portal detection.
//


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

// Floor for SPAN is 1.0 (raw span); compute_solid_half halves it → half_span ≥ 0.5
static constexpr TierParamDef ARCH_PARAM_DEFS[] = {
    { ArchProp::SPAN,         1.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { ArchProp::RISE,         1.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { ArchProp::DEPTH,        0.5f, 1e30f, false, ParamDist::GAUSSIAN },
    { ArchProp::THICKNESS,    0.1f, 1e30f, false, ParamDist::GAUSSIAN },
    { ArchProp::PIER_HEIGHT,  0.0f, 1e30f, false, ParamDist::GAUSSIAN },
    { ArchProp::PIER_PADDING, 0.1f, 1e30f, false, ParamDist::GAUSSIAN },
    { ArchProp::EDGE_BLEND,   0.1f, 1e30f, false, ParamDist::GAUSSIAN },
};
static constexpr uint32_t ARCH_PARAM_COUNT = sizeof(ARCH_PARAM_DEFS) / sizeof(TierParamDef);

// params[] order MUST match ARCH_PARAM_DEFS:
//   [0]SPAN [1]RISE [2]DEPTH [3]THICKNESS [4]PIER_HEIGHT [5]PIER_PADDING [6]EDGE_BLEND
//
// Note: name is ArchTierRow, not ArchTier — the enum class ArchTier
// (DOORWAY/STANDARD/MONUMENTAL) already occupies that name in entities.inl.
struct ArchTierRow {
    TierProfile profile;
    float       color_override;
    float       burial;
    uint32_t    segs_u;
    uint32_t    segs_v;
};

static constexpr ArchTierRow ARCH_TIERS[] = {
    /* DOORWAY    */ {
        { 0.50f, 0.0f, { {12.0f, 2.4f}, {12.0f, 2.4f}, {4.5f, 0.9f}, {1.2f, 0.18f}, {1.5f, 0.9f}, {0.9f, 0.3f}, {0.9f, 0.15f} }},
        0.15f, 0.20f, 16, 4
    },
    /* STANDARD   */ {
        { 0.15f, 0.0f, { {50.0f, 15.0f}, {42.0f, 7.0f}, {5.6f, 1.1f}, {1.4f, 0.21f}, {5.6f, 2.1f}, {0.7f, 0.3f}, {0.7f, 0.14f} }},
        0.25f, 0.20f, 32, 8
    },
    /* MONUMENTAL */ {
        { 0.15f, 0.0f, { {60.0f, 10.0f}, {80.0f, 12.0f}, {10.0f, 2.0f}, {2.5f, 0.30f}, {8.0f, 2.5f}, {1.0f, 0.3f}, {0.8f, 0.15f} }},
        0.35f, 0.20f, 48, 12
    },
};

static const TierProfile& arch_get_tier_profile(uint32_t tier_idx) {
    return ARCH_TIERS[tier_idx].profile;
}

static constexpr EntityFamilyTraits ARCH_TRAITS = {
    PopFamily::ARCH, "arch", Dim::MAX_ARCH_INSTANCES,
    true, true, 2,        // grounded, creates ground, 2 piers
    true, 120.0f, 1.5f,
    ArchProp::SPAWN_ROLL, ArchConfig::SPAWN_CHANCE,
    ArchConfig::MOOD_MULTIPLIER, ArchConfig::POSITION_JITTER,
    static_cast<uint32_t>(ArchTier::COUNT), ArchProp::TIER,
    ARCH_PARAM_DEFS, ARCH_PARAM_COUNT,
    ArchProp::POSITION_X, ArchProp::POSITION_Z, ArchProp::ROTATION, true,
    0, nullptr,
};

static SpawnGateOutput arch_run_gate(Cartridge* c, int32_t gx, int32_t gz) {
    auto gate = c->run_spawn_preamble(gx, gz, c->entities_state_.arches, Dim::MAX_ARCH_INSTANCES,
        ArchProp::SPAWN_ROLL, ArchConfig::SPAWN_CHANCE,
        ArchConfig::MOOD_MULTIPLIER, PopFamily::ARCH, "arch");
    return { gate.ok, gate.seed, gate.slot, gate.theme_idx };
}
static const float* arch_get_theme_tier_weights(uint32_t ti) { return THEMES[ti].tier_wt_arch; }

static constexpr uint32_t ARCH_INDOOR_RESCALE_PARAMS[] = {
    ArchIdx::SPAN, ArchIdx::RISE, ArchIdx::DEPTH, ArchIdx::THICKNESS,
    ArchIdx::PIER_HEIGHT, ArchIdx::PIER_PADDING, ArchIdx::EDGE_BLEND,
};

// Arch total height = pier_height + rise (catenary apex). Rolled
// target in [0.50, 0.95] × ceiling_h, scale every length param.
static void arch_apply_indoor_rescale(EntityInstance& inst, float ceiling_h) {
    rescale_to_rolled_target(inst, ceiling_h,
        /*target_lo*/ 0.50f, /*target_hi*/ 0.95f,
        /*current_h*/ inst.params[ArchIdx::PIER_HEIGHT] + inst.params[ArchIdx::RISE],
        ARCH_INDOOR_RESCALE_PARAMS);
}

static void arch_compute_solid_half(EntityInstance& inst, const TierProfile&) {
    float half_span    = inst.params[ArchIdx::SPAN] * 0.5f;
    inst.params[ArchIdx::SPAN] = half_span;  // overwrite: SPAN now holds half_span
    float thickness    = inst.params[ArchIdx::THICKNESS];
    float depth        = inst.params[ArchIdx::DEPTH];
    float pier_padding = inst.params[ArchIdx::PIER_PADDING];
    float edge_blend   = inst.params[ArchIdx::EDGE_BLEND];
    float pier_height  = inst.params[ArchIdx::PIER_HEIGHT];

    float pier_half_x = thickness * 0.5f + pier_padding + edge_blend;
    float pier_half_z = depth * 0.5f + pier_padding + edge_blend;
    inst.solid_half = half_span + std::max(pier_half_x, pier_half_z);
    inst.ground_y_offset = pier_height;
    inst.burial = std::max(0.2f, pier_height * ARCH_TIERS[inst.tier_idx].burial);
}

static void arch_compute_colors(EntityInstance& inst, const EntityFamilyTraits&, const TierProfile& /*tier*/) {
    const auto& tp = ARCH_TIERS[inst.tier_idx];
    // Base color: sandstone/palette
    if (cpu_hash_f(inst.seed, ArchProp::COLOR_OVER) < tp.color_override) {
        uint32_t pal_idx = cpu_hash(inst.seed, ArchProp::COLOR_OVER + 1u) % ARCH_PALETTE_COUNT;
        inst.colors[0] = ARCH_PALETTE[pal_idx][0] + (cpu_hash_f(inst.seed, ArchProp::COLOR_VAR_R) - 0.5f) * 0.06f;
        inst.colors[1] = ARCH_PALETTE[pal_idx][1] + (cpu_hash_f(inst.seed, ArchProp::COLOR_VAR_G) - 0.5f) * 0.06f;
        inst.colors[2] = ARCH_PALETTE[pal_idx][2] + (cpu_hash_f(inst.seed, ArchProp::COLOR_VAR_B) - 0.5f) * 0.06f;
    } else {
        inst.colors[0] = ARCH_SANDSTONE_BASE[0] + (cpu_hash_f(inst.seed, ArchProp::COLOR_VAR_R) - 0.5f) * ARCH_SANDSTONE_VARIANCE * 2.0f;
        inst.colors[1] = ARCH_SANDSTONE_BASE[1] + (cpu_hash_f(inst.seed, ArchProp::COLOR_VAR_G) - 0.5f) * ARCH_SANDSTONE_VARIANCE * 2.0f;
        inst.colors[2] = ARCH_SANDSTONE_BASE[2] + (cpu_hash_f(inst.seed, ArchProp::COLOR_VAR_B) - 0.5f) * ARCH_SANDSTONE_VARIANCE * 2.0f;
    }
    // Mesh color defaults to base; portal override applied in write_gpu
    // (needs ActiveArch.is_portal which is set by write_active first)
    inst.colors[3] = inst.colors[0];
    inst.colors[4] = inst.colors[1];
    inst.colors[5] = inst.colors[2];
}

static void arch_write_active(Cartridge* c, const EntityInstance& inst) {
    float half_span = inst.params[ArchIdx::SPAN];  // already halved
    float rise      = inst.params[ArchIdx::RISE];
    float pier_h    = inst.params[ArchIdx::PIER_HEIGHT];

    auto& aa = c->entities_state_.arches[inst.slot];
    aa.patch_gx = inst.trigger_gx; aa.patch_gz = inst.trigger_gz;
    aa.host_gx = inst.host_gx; aa.host_gz = inst.host_gz;
    aa.active = true; aa.draw_visible = true;
    aa.world_x = inst.cx; aa.world_z = inst.cz;
    aa.rotation = inst.rotation;
    aa.half_span = half_span;
    aa.total_height = pier_h + rise;
    aa.tier = static_cast<ArchTier>(inst.tier_idx);
    aa.depth = inst.params[ArchIdx::DEPTH];
    aa.thickness = inst.params[ArchIdx::THICKNESS];
    aa.rise = rise;
    aa.pier_height = pier_h;
    aa.burial = inst.burial;
    aa.segs_u = ARCH_TIERS[inst.tier_idx].segs_u;
    aa.segs_v = ARCH_TIERS[inst.tier_idx].segs_v;
    aa.col_r = inst.colors[0]; aa.col_g = inst.colors[1]; aa.col_b = inst.colors[2];

    // Ground Y: GPU compute shader samples the heightfield at both pier
    // feet (which already includes pier effect) and takes the min. CPU uploads 0.
    aa.cached_ground_y = 0.0f;

    // Portal state: recompute from seed
    aa.is_portal = false;
    aa.is_back_portal = false;
    aa.position_hash = cpu_hash(inst.seed, ArchProp::ROTATION + 100u);
    aa.destination = PortalDestination{};
    if (inst.tier_idx == static_cast<uint32_t>(ArchTier::DOORWAY)) {
        float portal_roll = cpu_hash_f(inst.seed, ArchProp::ROTATION + 200u);
        if (portal_roll < PORTAL_DENSITY) {
            aa.is_portal = true;
            uint32_t dest_seed = cpu_hash(aa.position_hash, 1u);
            uint32_t mood = c->pick_portal_mood(aa.position_hash, 2u);
            const auto& mp = MOOD_TABLE[mood];
            aa.destination.seed = dest_seed;
            aa.destination.mood = mood;
            aa.destination.finite = mp.finite;
            aa.destination.finite_radius = derive_finite_radius(dest_seed, mp);
        }
    }

    c->entities_state_.arch_count++;
    c->mood_state_.portals_dirty = true;
}

static void arch_write_gpu(Cartridge* c, const EntityInstance& inst, wgpu::Queue& queue) {
    float half_span = inst.params[ArchIdx::SPAN];
    float rise      = inst.params[ArchIdx::RISE];

    GPUArchMeshParams mp{};
    mp.center_x   = inst.cx;
    mp.center_z   = inst.cz;
    mp.rotation   = inst.rotation;
    mp.half_span  = half_span;
    mp.rise       = rise;
    mp.depth      = inst.params[ArchIdx::DEPTH];
    mp.thickness  = inst.params[ArchIdx::THICKNESS];
    mp.pier_height = inst.params[ArchIdx::PIER_HEIGHT];
    mp.burial     = inst.burial;
    mp.catenary_a = solve_catenary_a(half_span, rise);
    mp.segs_u     = ARCH_TIERS[inst.tier_idx].segs_u;
    mp.segs_v     = ARCH_TIERS[inst.tier_idx].segs_v;
    mp.color_r    = inst.colors[3]; mp.color_g = inst.colors[4]; mp.color_b = inst.colors[5];
    mp.is_active  = 1;
    c->gpuState_.upload_arch_mesh_params_slot(queue, inst.slot, mp);
    c->entities_state_.arch_mesh_gen_pending = true;
}

static void arch_post_commit(Cartridge* c, const EntityInstance& inst, wgpu::Queue& queue) {
    float half_span    = inst.params[ArchIdx::SPAN];
    float thickness    = inst.params[ArchIdx::THICKNESS];
    float depth        = inst.params[ArchIdx::DEPTH];
    float pier_padding = inst.params[ArchIdx::PIER_PADDING];
    float edge_blend   = inst.params[ArchIdx::EDGE_BLEND];
    float pier_height  = inst.params[ArchIdx::PIER_HEIGHT];

    float pier_half_x = thickness * 0.5f + pier_padding + edge_blend;
    float pier_half_z = depth * 0.5f + pier_padding + edge_blend;

    float cos_r = std::cos(inst.rotation), sin_r = std::sin(inst.rotation);
    float pl_x = inst.cx + (-half_span) * cos_r;
    float pl_z = inst.cz + (-half_span) * sin_r;
    float pr_x = inst.cx + half_span * cos_r;
    float pr_z = inst.cz + half_span * sin_r;

    // Left pier
    uint32_t pier_l_slot = Dim::PIER_ARCH_BASE + inst.slot * 2;
    GPUPierInstance pier_l{};
    pier_l.origin[0] = pl_x; pier_l.origin[1] = pl_z;
    pier_l.half_size[0] = pier_half_x; pier_l.half_size[1] = pier_half_z;
    pier_l.height_near = pier_height; pier_l.height_far = pier_height;
    pier_l.rotation = inst.rotation;
    pier_l.edge_blend = edge_blend;
    pier_l.tier = PierTier::ARCH_DOORWAY + inst.tier_idx;
    pier_l.is_active = 1;
    c->write_pier(queue, pier_l_slot, pier_l);

    // Right pier
    GPUPierInstance pier_r{};
    pier_r.origin[0] = pr_x; pier_r.origin[1] = pr_z;
    pier_r.half_size[0] = pier_half_x; pier_r.half_size[1] = pier_half_z;
    pier_r.height_near = pier_height; pier_r.height_far = pier_height;
    pier_r.rotation = inst.rotation;
    pier_r.edge_blend = edge_blend;
    pier_r.tier = PierTier::ARCH_DOORWAY + inst.tier_idx;
    pier_r.is_active = 1;
    c->write_pier(queue, pier_l_slot + 1, pier_r);

    // Regen AABB
    float reach = std::max(pier_half_x, pier_half_z) + edge_blend;
    c->mark_patches_for_regen(
        std::min(pl_x, pr_x) - reach, std::min(pl_z, pr_z) - reach,
        std::max(pl_x, pr_x) + reach, std::max(pl_z, pr_z) + reach,
        inst.host_gx, inst.host_gz);
}

static constexpr EntityFamilyAdapter ARCH_ADAPTER = {
    arch_run_gate, arch_get_theme_tier_weights,
    arch_apply_indoor_rescale,
    arch_compute_solid_half, arch_compute_colors,
    arch_write_active, arch_write_gpu, arch_post_commit,
    arch_get_tier_profile,
};

static bool dispatch_select_arch_generic(Cartridge* self, int32_t gx, int32_t gz, EntityQueueEntry& e) {
    EntityInstance inst{};
    if (!self->generic_select(ARCH_TRAITS, ARCH_ADAPTER, gx, gz, inst)) return false;
    e.family = PopFamily::ARCH; e.gx = gx; e.gz = gz; e.generic = inst; return true;
}
static bool dispatch_place_arch_generic(Cartridge* self, EntityQueueEntry& e, PlacementEntry& pe) {
    pe.family = e.family; pe.gx = e.gx; pe.gz = e.gz;
    if (self->generic_place(ARCH_TRAITS, e.generic)) { pe.generic = e.generic; return true; }
    self->entities_state_.arches[e.generic.slot].active = false; return false;
}
static void dispatch_commit_arch_generic(Cartridge* self, PlacementEntry& pe, wgpu::Queue& queue) {
    auto* host = self->find_patch(pe.generic.host_gx, pe.generic.host_gz);
    if (host) { self->generic_commit(ARCH_TRAITS, ARCH_ADAPTER, pe.generic, queue); host->record_entity(PopFamily::ARCH, pe.generic.slot); }
    else { self->entities_state_.arches[pe.generic.slot].active = false; }
}


// ─── FAMILY_DISPATCH Integration ─────────────────────────────────
//
// The dispatch wrappers above are entries in FAMILY_DISPATCH (in
// cartridge.hpp). Three per family, eight families: 24 entries
// total. Adding a new family means adding a block matching the
// template in this file's header, plus three entries in
// FAMILY_DISPATCH.

// ═══ END entity_pipeline.inl ═════════════════════════════════════
