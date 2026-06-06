// ─── spawn_engine.inl ────────────────────────────────────────────
//
// How and when things appear: shared spawn helpers, footprint
// registry, proximity affinity, mesh gen prep, distance culling,
// census, plus the dispatch loops that drive both generic and
// bespoke families through select → place → commit.
//
// ┌─── Public surface (called from outside this file) ──────────────┐
// │                                                                  │
// │  Shared spawn helpers (used by every family — generic + bespoke):│
// │    run_spawn_preamble<ActiveT>(...)   — gate + slot reservation  │
// │    negotiate_position(...)            — jitter + footprint check │
// │    record_placement_bookkeeping(...)  — population observation   │
// │    evaluate_spawn_gate(...)           — seed + flat probability  │
// │    jittered_position(...)             — patch-relative position  │
// │                                                                  │
// │  Footprint registry:                                             │
// │    check_position(...)                — separation check         │
// │    register_footprint(...)            — claim spatial slot       │
// │    unregister_footprints_for_patch(...) — release on eviction    │
// │                                                                  │
// │  Proximity affinity:                                             │
// │    proximity_affinity_boost(cx, cz, family) — spawn-chance boost │
// │    mark_patches_for_regen(...)        — heightfield invalidation │
// │                                                                  │
// │  Census (diagnostics):                                           │
// │    dump_entity_census(trigger)        — full snapshot to stdout  │
// │                                                                  │
// │  Mesh gen preparers + culling:                                   │
// │    prepare_arch_mesh_gen(queue)                                  │
// │    prepare_column_mesh_gen(queue)     — covers Antenna too       │
// │    prepare_pyramid_mesh_gen(queue)                               │
// │    update_entity_draw_visibility(queue) — distance culling       │
// │    write_pier(...), clear_pier(...), flush_pier_count(...)       │
// │                                                                  │
// │  Entity dispatch pipeline:                                       │
// │    select_entities_for_patch(gx, gz)                             │
// │    place_entity_queue()                                          │
// │    commit_entity_queue(queue)                                    │
// │                                                                  │
// │  Bespoke families' selection/placement structs (consumed by      │
// │  ribbon.inl, gol_zones.inl, gallery.inl, mood.inl):              │
// │    RibbonSelection, RibbonPlacement                              │
// │    GoLSelection, GoLPlacement                                    │
// │    GallerySelection, GalleryPlacement                            │
// │                                                                  │
// │  Cross-module reads:                                             │
// │    PopFamily indices    — every spawn-aware module               │
// │    GLOBAL_ENTITY_DENSITY                                         │
// │    footprints_[], MAX_FOOTPRINTS                                 │
// │    estimate_terrain_height(wx, wz)                               │
// │    solve_catenary_a(...)  — used by build_arch_mesh_params       │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// Included inside the Cartridge class body.
// Depends on: entities.inl, terrain_cpu.inl, seed_utils.inl;
//             entity_types.inl is included MID-FILE — see
//             SEAM[spawn_engine:structural] below.
//
// SEAM[spawn_engine:P11] home of pattern P11 (templated active-array
//   helper) — run_spawn_preamble<ActiveT> is the canonical instance.
//   One implementation, ten callers. Same family as P10's per-family
//   vocabulary block at the algorithm level.
// SEAM[spawn_engine:structural] mid-file `#include "modules/entity_types.inl"`
//   is load-bearing: EntityQueueEntry has a union member of type
//   EntityInstance, and C++ requires the union member's type to be
//   defined before the union itself. Keep one file with the include
//   rather than splitting into pre/post files; tag-only awareness.
// SEAM[spawn_engine:L1] latent diagnostic — DIAG_ENTITY_LIFECYCLE is
//   compile-time guarded (#define commented out below). Same family
//   as the [DIAG:*] stdout pattern noted across the codebase.
//   Document alongside any other diagnostic switches when the
//   exhibition-guard discussion happens.
// ─────────────────────────────────────────────────────────────────

// #define DIAG_ENTITY_LIFECYCLE   // uncomment to enable spawn/evict diagnostics


// ═══ SHARED SPAWN HELPERS ════════════════════════════════════════
//
// Three helpers used by every family — generic-pipeline ones
// through their adapter's run_gate, bespoke ones (ribbon, GoL,
// gallery) directly. The shape of the spawn lifecycle these
// embody (gate + slot reserve → jitter + footprint → bookkeeping)
// is the through-line of the whole engine.

// ── Helper 1: SpawnGatePreamble ──────────────────────────────
//
// Idempotency-through-slot-reservation gate.
// Templated on the active-slot array type so it works with
// all Active* structs that share .active, .patch_gx, .patch_gz.

struct SpawnGatePreambleResult {
    uint32_t seed;          // from evaluate_spawn_gate
    uint32_t slot;          // reserved slot index
    uint32_t theme_idx;     // active_theme_idx_ at evaluation time
    bool ok;                // false = early exit (idempotency, gate, no slot)
};

template<typename ActiveT>
SpawnGatePreambleResult run_spawn_preamble(
    int32_t gx, int32_t gz,
    ActiveT* active_arr, uint32_t max_instances,
    uint32_t spawn_roll_prop, float spawn_chance,
    const float* mood_mult,
    uint32_t family, const char* diag_name)
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

    // 2-6. Spawn modifier chain
    float adj_mod = mood_mult[mood_state_.active];
    adj_mod *= population_type_affinity(family);
    adj_mod *= GLOBAL_ENTITY_DENSITY;
    r.theme_idx = active_theme_idx_;
    {
        auto dit = tileCache_.find({ gx, gz });
        if (dit != tileCache_.end()) {
            adj_mod *= dit->second.entity_density;
            adj_mod *= dit->second.theme_spawn[family];
        }
    }

    // 6b. Proximity affinity boost (nearby entities attract)
    {
        float pcx = (gx + 0.5f) * PATCH_EXTENT;
        float pcz = (gz + 0.5f) * PATCH_EXTENT;
        adj_mod *= proximity_affinity_boost(pcx, pcz, family);
    }

    // 7. Spawn gate
    auto ctx = evaluate_spawn_gate(gx, gz, spawn_roll_prop,
        spawn_chance, adj_mod);
    if (!ctx.passed) return r;

    // 8-9. Find and reserve slot
    uint32_t slot = UINT32_MAX;
    for (uint32_t i = 0; i < max_instances; i++) {
        if (!active_arr[i].active) { slot = i; break; }
    }
    if (slot == UINT32_MAX) return r;
    active_arr[slot].active = true;

#ifdef DIAG_ENTITY_LIFECYCLE
    std::cout << "[DIAG:SEL] " << diag_name << " slot=" << slot
        << " patch=(" << gx << "," << gz << ")\n";
#endif

    r.seed = ctx.seed;
    r.slot = slot;
    r.ok = true;
    return r;
}

// ── Helper 2: NegotiatePosition ─────────────────────────────
//
// Jittered position → separation + footprint check →
// host patch + footprint registration.  Returns the
// accepted position or failure.

struct PositionResult {
    float cx, cz, rotation;
    int32_t host_gx, host_gz;
    bool ok;
};

PositionResult negotiate_position(
    uint32_t seed, int32_t trigger_gx, int32_t trigger_gz,
    uint32_t pos_x_prop, uint32_t pos_z_prop, float jitter,
    uint32_t rotation_seed_prop,
    float footprint_r, uint32_t family, uint32_t tier = 0)
{
    PositionResult r{};
    r.ok = false;

    // 1. Jittered position
    jittered_position(seed, trigger_gx, trigger_gz,
        pos_x_prop, pos_z_prop, jitter, r.cx, r.cz);
    r.rotation = cpu_hash_f(seed, rotation_seed_prop) * 6.283185f;

    // 1b. Indoor wall-margin clamp ─────────────────────────
    //
    // In finite indoor worlds, push the candidate inward so the
    // entity's footprint stays at least INDOOR_ENTITY_WALL_MARGIN
    // from every wall. We clamp instead of rejecting because
    // rejection would silently drop entities anchored to corner
    // patches (their seed-determined position keeps landing in
    // the wall margin and never recovers). Clamping shifts the
    // candidate to the boundary of the legal box, then the
    // existing footprint-overlap check handles any pile-ups.
    //
    // If the room is too small for the entity plus margins on
    // both sides (lo > hi), we clamp to the room center —
    // shouldn't happen for typical indoor entities (max
    // footprint at radius=1 is 65; rescaled entities are well
    // under that).
    if (world_state_.finite_mode && MOOD_TABLE[mood_state_.active].indoor) {
        float bmin = -(float)world_state_.finite_radius * PATCH_EXTENT;
        float bmax = ((float)world_state_.finite_radius + 1.0f) * PATCH_EXTENT;
        float clearance = INDOOR_ENTITY_WALL_MARGIN + footprint_r;
        float lo = bmin + clearance;
        float hi = bmax - clearance;
        if (lo > hi) {
            // Entity too big for room — collapse to center;
            // the footprint check below will still reject it
            // if it overlaps something.
            float center = (bmin + bmax) * 0.5f;
            r.cx = center;
            r.cz = center;
        }
        else {
            if (r.cx < lo) r.cx = lo;
            else if (r.cx > hi) r.cx = hi;
            if (r.cz < lo) r.cz = lo;
            else if (r.cz > hi) r.cz = hi;
        }
    }

    // 2. Separation + footprint check (single pass)
    if (!check_position(r.cx, r.cz, footprint_r, family))
        return r;

    // 3. Host patch + footprint registration
    r.host_gx = (int32_t)std::floor(r.cx / PATCH_EXTENT);
    r.host_gz = (int32_t)std::floor(r.cz / PATCH_EXTENT);
    if (register_footprint(r.cx, r.cz, footprint_r,
        r.host_gx, r.host_gz, family, tier) == UINT32_MAX) return r;

    r.ok = true;
    return r;
}

// ── Helper 3: record_placement_bookkeeping ──────────────────
//
// Tail bookkeeping shared by all three families.

void record_placement_bookkeeping(uint32_t family, uint32_t tier_idx)
{
    record_population_observation(family, tier_idx);
}

// ═══ MESH GEN PREPARERS + CULLING ════════════════════════════════
//
// CPU-side prep for GPU mesh-gen compute dispatches: scan active
// arrays, compute draw range, upload index counts. Plus per-frame
// distance culling with hysteresis. Plus pier write helpers
// (piers are derived state of arches and columns, written through
// here so cpu/gpu mirrors stay in sync).

// ─── Column / Arch / Pyramid mesh-gen preparers ───────────────
// (Moved to entities.inl during migration #10 — they now live
//  alongside the palm/cactus/blade preparers as static functions
//  taking EntitiesState& explicitly. The remainder of this file
//  retains pier writers, culling, and footprint registry, which
//  are spawn-machinery rather than per-family state.)

// ─── Pier Write Helper ───────────────────────────────────────────
//
// Writes a pier to both CPU mirror and GPU buffer (48 bytes per slot).
// Maintains pier_count in config for bounded GPU iteration.
// Inactive pier: default-constructed GPUPierInstance (is_active=0).

void write_pier(wgpu::Queue& queue, uint32_t slot, const GPUPierInstance& pier) {
    cpuPiers_[slot] = pier;
    gpuState_.upload_pier_slot(queue, slot, pier);
    world_state_.pier_count_dirty = true;
    world_state_.ground_entries_dirty = true;
}

void clear_pier(wgpu::Queue& queue, uint32_t slot) {
    GPUPierInstance empty{};
    cpuPiers_[slot] = empty;
    gpuState_.upload_pier_slot(queue, slot, empty);
    world_state_.pier_count_dirty = true;
    world_state_.ground_entries_dirty = true;
}

void recompute_and_upload_pier_count(wgpu::Queue& queue) {
    uint32_t highest = 0;
    for (uint32_t i = 0; i < Dim::PIER_TOTAL; i++) {
        if (cpuPiers_[i].is_active) highest = i + 1;
    }
    gpuState_.config().pier_count = highest;
    gpuState_.upload_pier_count(queue);
}

void flush_pier_count(wgpu::Queue& queue) {
    if (!world_state_.pier_count_dirty) return;
    world_state_.pier_count_dirty = false;
    recompute_and_upload_pier_count(queue);
}

// ─── Entity Distance Culling ─────────────────────────────────────
//
// Entities beyond a size-proportional distance from the pawn are
// temporarily hidden by zeroing their GPU mesh params (producing
// degenerate triangles the rasterizer discards for free).
//
// IMPORTANT: the cull floor is set to the PREGEN radius so that
// every entity within the allocation window always has its mesh
// ready. Entities are spawned at allocation time; their meshes
// must be built before the visible circle can reach them.
// The system becomes active only when the world is extended
// beyond the current pregen ring (future LOD work).
//
// Hysteresis prevents oscillation near the threshold.

static constexpr float ENTITY_CULL_BASE = (float)Dim::PATCH_PREGEN_RADIUS * Dim::PATCH_EXTENT;  // = 350: never cull inside pregen
static constexpr float ENTITY_CULL_ARCH_SCALE = 2.5f;   // per unit of max(span, total_height)
static constexpr float ENTITY_CULL_COL_SCALE = 3.0f;   // per unit of column height
static constexpr float ENTITY_CULL_HYSTERESIS = 50.0f;  // band width: show at far-hyst, hide at far

// Rebuild GPUArchMeshParams from cached ActiveArch data.
GPUArchMeshParams build_arch_mesh_params(uint32_t slot) const {
    const auto& a = entities_state_.arches[slot];
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
    // Portal color override (mirrors spawn logic)
    if (a.is_portal) {
        const float* pc = a.is_back_portal
            ? PORTAL_COLOR_BACK
            : PORTAL_COLORS[a.destination.mood % MOOD_COUNT];
        p.color_r = pc[0]; p.color_g = pc[1]; p.color_b = pc[2];
    }
    else {
        p.color_r = a.col_r; p.color_g = a.col_g; p.color_b = a.col_b;
    }
    p.is_active = 1;
    return p;
}

// Rebuild GPUColumnMeshParams from cached ActiveColumn data.
static GPUColumnMeshParams build_column_mesh_params_from(const ActiveColumn& c) {
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

GPUColumnMeshParams build_column_mesh_params(uint32_t slot) const {
    return build_column_mesh_params_from(entities_state_.columns[slot]);
}

// Scan all active entities, toggle draw_visible with hysteresis,
// and upload mesh param changes. Returns count of currently hidden entities.
uint32_t update_entity_draw_visibility(wgpu::Queue& queue) {
    uint32_t culled = 0;

    // Arches
    for (uint32_t i = 0; i < Dim::MAX_ARCH_INSTANCES; i++) {
        if (!entities_state_.arches[i].active) continue;
        const auto& a = entities_state_.arches[i];
        float dx = a.world_x - player_.readback_x;
        float dz = a.world_z - player_.readback_z;
        float dist = std::sqrt(dx * dx + dz * dz);

        float entity_size = std::max(a.half_span * 2.0f, a.total_height);
        float cull_far = ENTITY_CULL_BASE + entity_size * ENTITY_CULL_ARCH_SCALE;
        float cull_near = cull_far - ENTITY_CULL_HYSTERESIS;

        bool should_show = a.draw_visible
            ? (dist <= cull_far)          // currently visible: hide when exceeding far
            : (dist <= cull_near);        // currently hidden:  show when inside near

        if (should_show != a.draw_visible) {
            entities_state_.arches[i].draw_visible = should_show;
            if (should_show) {
                gpuState_.upload_arch_mesh_params_slot(queue, i, build_arch_mesh_params(i));
            }
            else {
                GPUArchMeshParams empty{};
                gpuState_.upload_arch_mesh_params_slot(queue, i, empty);
            }
            entities_state_.arch_mesh_gen_pending = true;
        }

        if (!entities_state_.arches[i].draw_visible) culled++;
    }

    // Columns
    for (uint32_t i = 0; i < Dim::MAX_COLUMN_ONLY; i++) {
        if (!entities_state_.columns[i].active) continue;
        const auto& c = entities_state_.columns[i];
        float dx = c.world_x - player_.readback_x;
        float dz = c.world_z - player_.readback_z;
        float dist = std::sqrt(dx * dx + dz * dz);

        float cull_far = ENTITY_CULL_BASE + c.height * ENTITY_CULL_COL_SCALE;
        float cull_near = cull_far - ENTITY_CULL_HYSTERESIS;

        bool should_show = c.draw_visible
            ? (dist <= cull_far)
            : (dist <= cull_near);

        if (should_show != c.draw_visible) {
            entities_state_.columns[i].draw_visible = should_show;
            if (should_show) {
                gpuState_.upload_column_mesh_params_slot(queue, i, build_column_mesh_params(i));
            }
            else {
                GPUColumnMeshParams empty{};
                gpuState_.upload_column_mesh_params_slot(queue, i, empty);
            }
            entities_state_.column_mesh_gen_pending = true;
        }

        if (!entities_state_.columns[i].draw_visible) culled++;
    }

    // Antennas
    for (uint32_t i = 0; i < Dim::MAX_ANTENNA_ONLY; i++) {
        if (!entities_state_.antennas[i].active) continue;
        const auto& c = entities_state_.antennas[i];
        float dx = c.world_x - player_.readback_x;
        float dz = c.world_z - player_.readback_z;
        float dist = std::sqrt(dx * dx + dz * dz);
        uint32_t gpu_slot = i + Dim::ANTENNA_SLOT_OFFSET;

        float cull_far = ENTITY_CULL_BASE + c.height * ENTITY_CULL_COL_SCALE;
        float cull_near = cull_far - ENTITY_CULL_HYSTERESIS;

        bool should_show = c.draw_visible
            ? (dist <= cull_far)
            : (dist <= cull_near);

        if (should_show != c.draw_visible) {
            entities_state_.antennas[i].draw_visible = should_show;
            if (should_show) {
                gpuState_.upload_column_mesh_params_slot(queue, gpu_slot, build_column_mesh_params_from(c));
            }
            else {
                GPUColumnMeshParams empty{};
                gpuState_.upload_column_mesh_params_slot(queue, gpu_slot, empty);
            }
            entities_state_.column_mesh_gen_pending = true;
        }

        if (!entities_state_.antennas[i].draw_visible) culled++;
    }

    return culled;
}

// ═══ FOOTPRINT REGISTRY ══════════════════════════════════════════
//
// Prevents grounded entities (pyramids, arches, columns, galleries)
// from overlapping. Each entity registers a circular exclusion zone
// at spawn time. Subsequent spawns check against the registry.
//
// Priority order (enforced by spawn call sequence):
//   1. Pyramids  — rarest, largest footprint
//   2. Arches    — moderate, two pier footprints
//   3. Columns   — common, small
//   4. Galleries — common, moderate spread
//
// Linear scan is sufficient: max ~88 active footprints.

struct GroundFootprint {
    float x = 0.0f, z = 0.0f;
    float radius = 0.0f;
    int32_t patch_gx = 0, patch_gz = 0;
    uint32_t family = UINT32_MAX;  // PopFamily index
    uint32_t tier = 0;             // tier index within family
    float spawn_time = 0.0f;       // time_state_.seconds at registration
    bool active = false;
};

static constexpr uint32_t MAX_FOOTPRINTS = 128;
GroundFootprint footprints_[MAX_FOOTPRINTS]{};

// Single-pass spatial check: physical overlap + aesthetic separation.
// For entity footprints (family < COUNT): effective_min = gap + both radii.
// Gap is reduced when proximity affinity exists between the pair.
bool check_position(float px, float pz, float placing_radius,
    uint32_t placing_family) const {
    for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
        if (!footprints_[i].active) continue;
        float dx = px - footprints_[i].x;
        float dz = pz - footprints_[i].z;
        float effective_min = placing_radius + footprints_[i].radius;
        if (footprints_[i].family < PopFamily::COUNT) {
            float min_gap = MIN_SEPARATION[placing_family][footprints_[i].family];
            if (min_gap > 0.0f) {
                float aff = PROXIMITY_AFFINITY[placing_family][footprints_[i].family];
                if (aff > 0.0f) min_gap *= (1.0f - aff * PROXIMITY_GAP_REDUCTION[placing_family]);
                effective_min += min_gap;
            }
        }
        if (dx * dx + dz * dz < effective_min * effective_min) return false;
    }
    return true;
}

uint32_t register_footprint(float x, float z, float radius,
    int32_t gx, int32_t gz, uint32_t family = UINT32_MAX,
    uint32_t tier = 0) {
    for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
        if (!footprints_[i].active) {
            footprints_[i] = { x, z, radius, gx, gz, family, tier, time_state_.seconds, true };
            return i;
        }
    }
    return UINT32_MAX;  // full — entity should not spawn
}

void unregister_footprints_for_patch(int32_t gx, int32_t gz) {
    for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
        if (footprints_[i].active &&
            footprints_[i].patch_gx == gx && footprints_[i].patch_gz == gz) {
            footprints_[i].active = false;
        }
    }
}

// ═══ ENTITY CENSUS ═══════════════════════════════════════════════
//
// Complete snapshot of all active entities via the footprint registry.
// Printed to console periodically and on theme transitions.
// Enables determinism verification: same seed + pawn path → same census.

float lastCensusDump_ = -999.0f;
static constexpr float CENSUS_DUMP_INTERVAL = 30.0f;

static const char* family_short_name(uint32_t family) {
    static const char* NAMES[] = { "pyr", "arch", "col", "ant", "palm", "cact", "blad", "sph", "ribn", "cube", "gol", "gall" };
    return (family < PopFamily::COUNT) ? NAMES[family] : "???";
}

static const char* theme_short_name(uint32_t theme) {
    static const char* NAMES[] = { "transition", "monumental", "colonnade", "antenna", "barren" };
    return (theme < THEME_COUNT) ? NAMES[theme] : "???";
}

void dump_entity_census(const char* trigger) const {
    uint32_t count = 0;
    uint32_t by_family[PopFamily::COUNT] = {};
    for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
        if (!footprints_[i].active) continue;
        if (footprints_[i].family >= PopFamily::COUNT) continue;
        count++;
        by_family[footprints_[i].family]++;
    }

    std::cout << "[CENSUS t=" << std::fixed << std::setprecision(1) << time_state_.seconds
        << " trigger=" << trigger << "] " << count << " entities (";
    for (uint32_t f = 0; f < PopFamily::COUNT; f++) {
        if (f > 0) std::cout << " ";
        std::cout << family_short_name(f) << ":" << by_family[f];
    }
    std::cout << ")\n";

    // Per-entity detail, sorted by family then spawn_time
    struct CensusEntry { uint32_t fp_idx; uint32_t family; uint32_t tier; float spawn_time; };
    CensusEntry entries[MAX_FOOTPRINTS];
    uint32_t n = 0;
    for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
        if (!footprints_[i].active || footprints_[i].family >= PopFamily::COUNT) continue;
        entries[n++] = { i, footprints_[i].family, footprints_[i].tier, footprints_[i].spawn_time };
    }
    // Insertion sort by (family, spawn_time)
    for (uint32_t i = 1; i < n; i++) {
        CensusEntry key = entries[i]; uint32_t j = i;
        while (j > 0 && (entries[j - 1].family > key.family ||
            (entries[j - 1].family == key.family && entries[j - 1].spawn_time > key.spawn_time))) {
            entries[j] = entries[j - 1]; j--;
        }
        entries[j] = key;
    }
    for (uint32_t i = 0; i < n; i++) {
        const auto& fp = footprints_[entries[i].fp_idx];
        std::cout << "  " << family_short_name(fp.family)
            << " t" << fp.tier
            << " (" << std::setw(8) << std::setprecision(1) << fp.x
            << "," << std::setw(8) << fp.z << ")"
            << " p(" << std::setw(3) << fp.patch_gx << "," << std::setw(3) << fp.patch_gz << ")"
            << " age=" << std::setprecision(1) << (time_state_.seconds - fp.spawn_time)
            << "\n";
    }
    std::cout << std::flush;
}

// ═══ SPAWN UTILITIES ═════════════════════════════════════════════
//
// The spawn lifecycle's smallest building blocks: gate evaluation,
// jittered position, the family enum, the global density dial,
// and two load-bearing tag tables (Spawn Configuration Summary,
// Property Index Registry) that document the contracts every family
// participates in.

// ─── Spawn gate ──────────────────────────────────────────────────
//
// Shared preamble for entity spawn functions. Extracted from the
// common skeleton of spawn_{arches,columns,pyramids}_for_patch.
//
// Usage:
//   auto ctx = evaluate_spawn_gate(gx, gz, Prop::SPAWN_ROLL, Config::SPAWN_CHANCE);
//   if (!ctx.passed) return;
//   uint32_t tier = select_tier(ctx.seed, Prop::TIER, weights, count);
//   jittered_position(ctx.seed, gx, gz, Prop::POSITION_X, Prop::POSITION_Z, jitter, cx, cz);

struct SpawnPreamble {
    uint32_t seed;          // tile_seed(world_state_.active_seed, gx, gz)
    uint32_t archetype;     // 0=mountainous, 1=varied, 2=basin, 3=pool
    bool passed;            // false if spawn gate failed
};

// Evaluate the spawn gate: seed + flat probability check.
// adjacency_mod is a multiplier from the full spawn cascade.
SpawnPreamble evaluate_spawn_gate(int32_t gx, int32_t gz,
    uint32_t spawn_roll_prop,
    float spawn_chance,
    float adjacency_mod = 1.0f) const {
    SpawnPreamble result{};
    result.archetype = 1;
    auto tile_it = tileCache_.find({ gx, gz });
    if (tile_it != tileCache_.end()) result.archetype = tile_it->second.archetype;

    result.seed = tile_seed(world_state_.active_seed, gx, gz);
    float chance = std::min(spawn_chance * adjacency_mod, 1.0f);
    result.passed = cpu_hash_f(result.seed, spawn_roll_prop) < chance;
    return result;
}

// Jittered world position within a patch.
static void jittered_position(uint32_t seed, int32_t gx, int32_t gz,
    uint32_t prop_x, uint32_t prop_z, float jitter,
    float& out_x, float& out_z) {
    out_x = (gx + 0.5f) * PATCH_EXTENT + (cpu_hash_f(seed, prop_x) - 0.5f) * PATCH_EXTENT * jitter;
    out_z = (gz + 0.5f) * PATCH_EXTENT + (cpu_hash_f(seed, prop_z) - 0.5f) * PATCH_EXTENT * jitter;
}

// Entity families for observation indexing
struct PopFamily {
    static constexpr uint32_t PYRAMID = 0;
    static constexpr uint32_t ARCH = 1;
    static constexpr uint32_t COLUMN = 2;
    static constexpr uint32_t ANTENNA = 3;
    static constexpr uint32_t PALM = 4;
    static constexpr uint32_t CACTUS = 5;
    static constexpr uint32_t BLADE = 6;
    static constexpr uint32_t SPHERE = 7;    // orbital spheres
    static constexpr uint32_t RIBBON = 8;
    static constexpr uint32_t CUBE = 9;      // hover-bob monoliths (split from legacy FLOATING)
    static constexpr uint32_t GOL = 10;       // Game of Life / Pulse automaton zones
    static constexpr uint32_t GALLERY = 11;   // outdoor art exhibitions (composite: 1 center → N paintings)
    static constexpr uint32_t COUNT = 12;
};

// ─── Spawn Configuration Summary ────────────────────────────────
//
// Single-glance view of all entity spawn parameters.
// Authoritative values live in each entity's Config struct.
//
//  ┌──────────┬──────────┬───────────────────────────────────────┬────────┐
//  │ Family   │ CHANCE   │ MOOD_MULTIPLIER                       │ JITTER │
//  │          │          │ open  sunset flat  vault  fin   finR  │        │
//  ├──────────┼──────────┼───────────────────────────────────────┼────────┤
//  │ Pyramid  │  0.030   │ 1.0   1.0    1.0   1.0    1.0   0.0  │  0.25  │
//  │ Arch     │  0.030   │ 1.0   1.0    1.0   1.0    1.0   1.0  │  0.35  │
//  │ Column   │  0.030   │ 1.0   1.0    1.0   1.0    1.0   0.0  │  0.35  │
//  │ Antenna  │  0.025   │ 1.0   1.0    1.0   1.0    1.0   0.0  │  0.35  │
//  │ Palm     │  0.200   │ 1.0   1.0    1.0   1.0    1.0   0.0  │  0.45  │
//  │ Cactus   │  0.100   │ 1.0   1.0    1.0   1.0    1.0   0.0  │  0.35  │
//  │ Blade    │  0.025   │ 1.0   1.0    1.0   1.0    1.0   0.0  │  0.30  │
//  │ Sphere   │  0.015   │ 1.0   1.0    0.0   0.0    1.0   0.0  │  0.40  │
//  │ Ribbon   │  0.400   │ 1.0   1.0    0.0   0.0    1.0   0.0  │  0.30  │
//  │ Cube     │  0.060   │ 1.0   1.0    0.0   0.0    1.0   0.0  │  0.40  │
//  │ GoL      │  0.150   │ 1.0   1.0    0.0   0.0    1.0   0.0  │  (lattice) │
//  │ Gallery  │  varies  │ 1.0   1.0    1.0   1.0    1.0   0.0  │  0.30  │
//  └──────────┴──────────┴───────────────────────────────────────┴────────┘
//
// Spawn chance is flat — archetype/terrain no longer gates spawning.
// Spatial variation comes from theme lattice and density field.
// Themes further multiply spawn_chance via PopulationTheme::spawn_weight[family].
// Moods gate entire families (0.0 = suppressed in that mood).
// Jitter is fraction of PATCH_EXTENT for position randomization.

// ─── Global Entity Density ──────────────────────────────────────
// Master multiplier applied to ALL entity spawn chances.
// 1.0 = normal, <1.0 = sparser world, >1.0 = denser world.
// Stacks with per-theme density_mult and per-tile entity_density.
static constexpr float GLOBAL_ENTITY_DENSITY = 1.0f;

// ─── Property Index Registry ────────────────────────────────────
//
// Master allocation map for cpu_hash_f(seed, prop) indices.
// Each family claims a non-overlapping range within its seed source.
// Collisions between families that share a seed source produce
// correlated rolls — a subtle bug. Check this table before
// allocating indices for new entities.
//
// SEED SOURCE: tile_seed(world_state_.active_seed, gx, gz)
//  ┌───────────┬───────────┬────────────────────────────────────┐
//  │ Range     │ Family    │ Struct                             │
//  ├───────────┼───────────┼────────────────────────────────────┤
//  │   1       │ Heightfld │ (inline)                           │
//  │ 200 – 208 │ Terrain   │ CPU_WAVE_* constants               │
//  │ 220 – 221 │ Activity  │ CPU_ACT_PROP_* constants           │
//  │ 370       │ Theme     │ select_theme_at_node (inline)      │
//  │ 400 – 443 │ Ribbon    │ RibbonProp                         │
//  │ 500 – 540 │ Gallery   │ select_gallery_for_patch           │
//  │ 600 – 623 │ Arch      │ ArchProp                           │
//  │ 700 – 743 │ Column    │ ColumnProp                         │
//  │ 800 – 823 │ Pyramid   │ PyramidProp                        │
//  │ 900 – 943 │ Antenna   │ AntennaProp                        │
//  │ 950 – 993 │ Palm      │ PalmProp                           │
//  │1000 –1033 │ Cactus    │ CactusProp                         │
//  │1100 –1122 │ Blade     │ BladeProp                          │
//  │1200+      │ (free)    │ next entity family starts here     │
//  └───────────┴───────────┴────────────────────────────────────┘
//
// SEED SOURCE: tile_seed (shared with all families)
//  ┌───────────┬───────────┬────────────────────────────────────┐
//  │ Range     │ Family    │ Struct                             │
//  ├───────────┼───────────┼────────────────────────────────────┤
//  │ 100 – 126 │ Floater   │ FloatingEntityProp                 │
//  └───────────┴───────────┴────────────────────────────────────┘
//
// SEED SOURCE: lattice_node_seed (band 250) — GoL zones
//  ┌───────────┬───────────┬────────────────────────────────────┐
//  │ Range     │ Family    │ Struct                             │
//  ├───────────┼───────────┼────────────────────────────────────┤
//  │ 920 – 938 │ GoL Zone  │ GoLZoneProp                        │
//  │ 950 – 954 │ Pulse     │ PulseZoneProp                      │
//  └───────────┴───────────┴────────────────────────────────────┘
//  NOTE: GoL/Pulse indices overlap Antenna and Palm numerically,
//  but are safe — they use a different seed source (lattice band
//  250 vs tile_seed). Do NOT move GoL props into the tile_seed
//  range without resolving the collision.


// ═══ PROXIMITY AFFINITY ══════════════════════════════════════════
//
// Distance-based spawn boost: "how much does a nearby entity of
// family B increase the spawn chance of family A?"
//
// PROXIMITY_AFFINITY[spawning][nearby] — boost per entity within radius.
// 0.0 = no effect. Positive = attraction. The scan accumulates a
// weighted count and returns a multiplier capped at max boost.
//
// Per-family parameters control the search geometry:
//   PROXIMITY_RADIUS     — search distance (0 = disabled)
//   PROXIMITY_MAX_BOOST  — cap on multiplier (1.0 = no boost possible)
//   PROXIMITY_THRESHOLD  — min nearby count before boost activates
//   PROXIMITY_GAP_REDUCTION — how much affinity softens separation
//
// Precomputed participation masks enable zero-cost early-out for
// families with no affinities defined.

//                              Pyr    Arch   Col    Ant    Palm   Cact   Blad   Sph    Ribn   Cube   GoL    Gall
static constexpr float    PROXIMITY_RADIUS[PopFamily::COUNT] = { 0.0f,  0.0f, 60.0f,  0.0f,150.0f,120.0f,120.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f };
static constexpr float    PROXIMITY_MAX_BOOST[PopFamily::COUNT] = { 1.0f,  1.0f,  2.0f,  1.0f,  3.0f,  3.0f,  3.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f };
static constexpr uint32_t PROXIMITY_THRESHOLD[PopFamily::COUNT] = { 0,     0,     2,     0,     1,     1,     1,     0,     0,     0,     0,     0 };
static constexpr float    PROXIMITY_GAP_REDUCTION[PopFamily::COUNT] = { 0.0f, 0.0f, 0.3f, 0.0f, 0.6f, 0.6f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

static constexpr float PROXIMITY_AFFINITY[PopFamily::COUNT][PopFamily::COUNT] = {
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
static constexpr bool proximity_row_active(uint32_t family) {
    for (uint32_t f = 0; f < PopFamily::COUNT; f++)
        if (PROXIMITY_AFFINITY[family][f] > 0.0f) return true;
    return false;
}

float proximity_affinity_boost(float cx, float cz, uint32_t family) const {
    if (!proximity_row_active(family)) return 1.0f;
    float radius = PROXIMITY_RADIUS[family];
    if (radius <= 0.0f) return 1.0f;
    float r2 = radius * radius;
    float weighted = 0.0f;
    uint32_t count = 0;
    for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
        if (!footprints_[i].active) continue;
        if (footprints_[i].family >= PopFamily::COUNT) continue;
        float aff = PROXIMITY_AFFINITY[family][footprints_[i].family];
        if (aff <= 0.0f) continue;
        float dx = cx - footprints_[i].x;
        float dz = cz - footprints_[i].z;
        if (dx * dx + dz * dz < r2) {
            weighted += aff;
            count++;
        }
    }
    if (count < PROXIMITY_THRESHOLD[family]) return 1.0f;
    return std::min(1.0f + weighted, PROXIMITY_MAX_BOOST[family]);
}

// Mark already-generated patches that overlap a world-space AABB
// for regeneration. Used by arches, pyramids, and any entity whose
// collision geometry must be baked into the heightfield.
void mark_patches_for_regen(float min_wx, float min_wz,
    float max_wx, float max_wz,
    int32_t home_gx, int32_t home_gz) {
    int32_t pg_x0 = (int32_t)std::floor(min_wx / PATCH_EXTENT);
    int32_t pg_x1 = (int32_t)std::floor(max_wx / PATCH_EXTENT);
    int32_t pg_z0 = (int32_t)std::floor(min_wz / PATCH_EXTENT);
    int32_t pg_z1 = (int32_t)std::floor(max_wz / PATCH_EXTENT);

    for (uint32_t p = 0; p < world_state_.active_patch_count; p++) {
        if (patches_[p].phase != PatchPhase::GENERATED) continue;
        if (patches_[p].grid_x == home_gx && patches_[p].grid_z == home_gz) continue;
        if (patches_[p].grid_x >= pg_x0 && patches_[p].grid_x <= pg_x1 &&
            patches_[p].grid_z >= pg_z0 && patches_[p].grid_z <= pg_z1) {
            patches_[p].phase = PatchPhase::NEEDS_REGEN;
        }
    }
}


// (generate_arch_mesh removed — replaced by GPU compute: arch_mesh_gen)

// Precompute catenary parameter 'a' from (half_span, rise).
// 50-iteration bisection, passed to GPU in ArchMeshParams.
static float solve_catenary_a(float half_span, float target_h) {
    float a_lo = 0.1f, a_hi = std::max(half_span * 10.0f, 5.0f);
    float a = half_span;
    for (int iter = 0; iter < 50; iter++) {
        a = 0.5f * (a_lo + a_hi);
        float val = a * (std::cosh(half_span / a) - 1.0f);
        if (val > target_h) a_lo = a; else a_hi = a;
    }
    return a;
}

// ═══ BESPOKE-FAMILY SELECTION/PLACEMENT STRUCTS ══════════════════
//
// Three bespoke families (GoL, Gallery, Ribbon) don't fit the
// generic pipeline's EntityInstance shape — their selection
// records carry family-specific fields (lattice node, painting
// count, wave parameters). These structs are the type-tagged
// payloads in EntityQueueEntry / PlacementEntry below.
//
// The structural mid-file include of entity_types.inl lands here
// because EntityQueueEntry has a union member of type
// EntityInstance (defined in entity_types.inl), and C++ requires
// the union member's type to be defined before the union itself.
// See SEAM[spawn_engine:structural] in the file header.

// ─── GoL Zone Selection / Placement ──────────────────────────────

struct GoLSelection {
    uint32_t seed;
    int32_t  trigger_gx, trigger_gz;
    uint32_t slot;
    int32_t  zone_nx, zone_nz;     // lattice node
    float    corner_x, corner_z;   // zone corner (cell-grid-snapped)
    uint32_t algorithm;            // AlgorithmType::CONWAY or PULSE
    uint32_t tier_idx;             // compound: Conway 0–6, Pulse 7–9
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

// ─── Gallery Selection / Placement ───────────────────────────────
// (Outdoor art exhibitions — composite: 1 center → N paintings)

struct GallerySelection {
    uint32_t seed;
    int32_t  trigger_gx, trigger_gz;
    uint32_t slot;              // gallery center slot
    float    cx, cz;            // gallery center (jittered)
    float    footprint_r;       // gallery spatial envelope
    uint32_t archetype;         // 0–3 (terrain type, used as tier_idx)
    uint32_t painting_count;
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
    float    facing_angle;
    float    gallery_size_mean;
    uint32_t site_type;
};

// ─── Ribbon Selection / Placement ────────────────────────────

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
    float lateral_amp, lateral_cycles, lateral_speed;
    float vertical_amp, vertical_cycles, vertical_speed;
    float twist_amp, twist_cycles, twist_speed;
    // Color
    uint32_t color_mode;
    float color[3];
    // Footprint
    float footprint_r;
};

struct RibbonPlacement {
    uint32_t slot;
    int32_t  trigger_gx, trigger_gz;
    int32_t  host_gx, host_gz;
    uint32_t tier_idx;
    float cx, cz, rotation;
    // Geometry (copied from selection)
    uint32_t cube_count;
    float cube_size, height, orientation;
    float lateral_amp, lateral_cycles, lateral_speed;
    float vertical_amp, vertical_cycles, vertical_speed;
    float twist_amp, twist_cycles, twist_speed;
    uint32_t color_mode;
    float color[3];
};

// Mid-file include — see SEAM[spawn_engine:structural] above.
#include "modules/entity_types.inl"

// ═══ ENTITY DISPATCH PIPELINE ════════════════════════════════════
//
// The three-phase machinery that drives every family — generic and
// bespoke — through select → place → commit. FAMILY_DISPATCH (in
// cartridge.hpp) is the function-pointer table; this file holds
// the queues and the loops that walk them.

// ─── Entity Selection Queue ─────────────────────────────────────
//
// Lightweight tagged entry holding one family's selection.
// Produced by select_entities_for_patch, consumed by
// drain_entity_queue. The queue decouples WHAT exists from
// WHERE it goes — selections are position-independent.

struct EntityQueueEntry {
    uint32_t family;    // PopFamily index
    int32_t  gx, gz;    // trigger patch (for commit bookkeeping)
    union {
        RibbonSelection ribbon;
        GoLSelection    gol;
        GallerySelection gallery;
        EntityInstance   generic;    // used by all 9 migrated families
    };
    EntityQueueEntry() : family(0), gx(0), gz(0) { std::memset(&generic, 0, sizeof(generic)); }
};

std::vector<EntityQueueEntry> entityQueue_;

// ─── Placement Results ──────────────────────────────────────────
//
// Output of place_entity_queue: entities that passed spatial
// negotiation and are ready for GPU commit. Tagged union mirrors
// EntityQueueEntry but holds Placement structs instead of Selections.

struct PlacementEntry {
    uint32_t family;
    int32_t  gx, gz;
    union {
        RibbonPlacement ribbon;
        GoLPlacement    gol;
        GalleryPlacement gallery;
        EntityInstance   generic;    // used by all 9 migrated families
    };
    PlacementEntry() : family(0), gx(0), gz(0) { std::memset(&generic, 0, sizeof(generic)); }
};

std::vector<PlacementEntry> placementResults_;

// ─── Select / Place / Commit dispatch loops ─────────────────────

void select_entities_for_patch(int32_t gx, int32_t gz) {
    for (uint32_t f = 0; f < PopFamily::COUNT; f++) {
        EntityQueueEntry e{};
        e.family = f;
        e.gx = gx; e.gz = gz;
        if (FAMILY_DISPATCH[f].try_select(this, gx, gz, e))
            entityQueue_.push_back(e);
    }
}

// ─── Place: spatial negotiation (no GPU writes) ──────────────
//
// Processes entityQueue_ in FIFO order via FAMILY_DISPATCH table.
// Each selection goes through separation, footprint. Successful
// placements are pushed to placementResults_. Failed placements
// unreserve the slot and are dropped.
//
// Mutates: footprints_, spawn records, population batch.
// Does NOT touch: GPU queue, GPU buffers, Active* arrays.

void place_entity_queue() {
    for (auto& e : entityQueue_) {
        PlacementEntry pe{};
        if (FAMILY_DISPATCH[e.family].try_place(this, e, pe))
            placementResults_.push_back(pe);
    }
    entityQueue_.clear();
}

// ─── Commit: GPU writes from placement results ──────────────
//
// Iterates placementResults_ via FAMILY_DISPATCH table, writes
// GPU state for each entity. Clears placementResults_ when done.
//
// Mutates: GPU buffers via queue, Active* arrays, pier mirrors,
// portal array, mesh gen flags — all render-layer state.
// Does NOT touch: footprints, spawn records.

void commit_entity_queue(wgpu::Queue& queue) {
    for (auto& pe : placementResults_)
        FAMILY_DISPATCH[pe.family].try_commit(this, pe, queue);
    placementResults_.clear();
}



// Estimate terrain height from tile cache (rough CPU-side approximation).
//
// NOT a ground policy query. Deliberately kept as the CPU fast
// path per ground_hierarchy_design.md §6.2: the CPU stays on an
// approximate tile-cache lookup rather than growing a parallel
// query_ground_* system. Callers that need accurate height must
// either (a) pick up the GPU-baked heightfield via readback
// (POLICY_BAKED_HEIGHTFIELD texture) or (b) defer the decision
// to a GPU compute pass.
float estimate_terrain_height(float wx, float wz) const {
    int32_t tx = (int32_t)std::floor(wx / PATCH_EXTENT);
    int32_t tz = (int32_t)std::floor(wz / PATCH_EXTENT);
    auto it = tileCache_.find({ tx, tz });
    if (it != tileCache_.end())
        return it->second.height_bias + it->second.amp_scale * 5.0f;
    return 0.0f;
}


