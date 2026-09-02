#pragma once
#include "core/boot_params.hpp"   // IOS_3 C — the four switches
#include <cstdint>
#include "cartridges/the_board/contracts/wgpu_fwd.hpp"   // wgpu handle fwds (lockstep insurance)
#include "cartridges/the_board/contracts/surface_services.hpp"  // the surface's decl tier (this module's own contract)
#include <cmath>          // std::floor, std::sqrt, std::abs   // (impl, merged)
#include <algorithm>      // std::min, std::max   // (impl, merged)
#include <cstring>        // std::memcpy (instance banding)   // (impl, merged)
#include <unordered_set>  // the O(1) existence scan (continuous allocation)   // (impl, merged)
#include <iostream>       // DIAG blocks (lifecycle audit + evict trace)   // (impl, merged)
#include <cstdio>         // std::fprintf — RIBBON_5's conservation witness (always-on, every build)
#include "core/instruments.hpp"   // RIBBON_5 — INSTRUMENTS.frame_meter gates the [STREAM] line

// ─── patch_system.hpp (S2 · MERGED: the active-patch machine) ──────
// The decl tier lives in contracts/surface_services.hpp; this file is the
// machine's bodies whole — the registry lifecycle (allocate → spawn →
// generate → evict), the frame budgets, world teardown, the layer
// allocator, and the streaming conductor. Stands on THE MACHINE FACE
// (the root organs are machine members), the S3 dispatch seam
// (select/place/commit — contracts/spawn_services.hpp), and the GPU
// wire (c->gpuState_ / c->renderer_). The reaches OUTSIDE the face
// ride the call site (the B law): the WRITABLE tile-cache + theme
// organs (the face's views are const — the lifecycle owner mutates
// them through the owner doors), the tile doors' deps, the mood deps
// (the back-portal door), and the driver's intent organ (the movement
// budget read). COHORT: the tail's last — after the machine natives
// (spawn service defs) and mood (the back-portal door's def).
// ─────────────────────────────────────────────────────────────────

namespace t7 {
namespace the_board {


// ── The patch registry ─────────────────────────────────────────────

inline ActivePatch* find_patch(MachineCtx* c, int32_t gx, int32_t gz) {
    for (uint32_t i = 0; i < c->world_state_.active_patch_count; i++) {
        if (c->patch_system_state_.patches_[i].valid && c->patch_system_state_.patches_[i].grid_x == gx && c->patch_system_state_.patches_[i].grid_z == gz)
            return &c->patch_system_state_.patches_[i];
    }
    return nullptr;
}

// Hook: full eviction of a single patch.
// OIL_1 U8/U9 — THE RAISES THIS PRIMITIVE OWES BUT DOES NOT MAKE:
// removing a patch changes the grid set (patch_instances_dirty) and
// frees a layer (alloc_scan_pending). Both raises live in this
// function's ONE caller, the eviction block in stream_patches, which
// batches them after its compaction. A second call site would have to
// carry them itself.
inline void evict_patch(MachineCtx* c, uint32_t pi, wgpu::Queue& queue) {
    free_layer(c, c->patch_system_state_.patches_[pi].layer);
    // Painting eviction now handled by evict_gallery (bodies/gallery.hpp) via entity_refs
    evict_patch_entities(c, c->patch_system_state_.patches_[pi], queue);
    c->patch_system_state_.patches_[pi].valid = false;
}

inline void evict_patch_entities(MachineCtx* c, ActivePatch& patch, wgpu::Queue& queue) {
    for (uint32_t i = 0; i < patch.entity_ref_count; i++) {
        auto& ref = patch.entity_refs[i];
        FAMILY_DISPATCH[ref.family].evict_slot(c, ref.slot, queue);
    }

    patch.entity_ref_count = 0;
}

// ── Dynamic budgets ────────────────────────────────────────────────

// ── World lifecycle ────────────────────────────────────────────────
//
// Root-called owner verb. CALLER: the transition machine
// (root); OWNER: patch_system. The bulk
// sweep over sibling organs dissolved into per-owner teardown verbs
// called by the score's TEARDOWN movement; this core keeps the
// surface's own concerns — patches, tiles, themes, dispatch queues,
// footprints — plus the world-rebirth GPU staging lines.
// ── The recenter door (deps-form) ─────────────────────────────────
inline void request_recenter(WorldState& ws) {
    ws.last_center_x = INT32_MAX;
    ws.last_center_z = INT32_MAX;
    ws.world_young = true;   // RIBBON_6: a re-centred window is a world beginning again
}

// THE ONE SURFACE RESET. Called from BOTH paths — boot and the transition
// machine — because boot is a transition from nothing (LAWS L10). Every line
// below was already true at boot, but by in-struct default rather than by
// call: the equality was asserted by luck and would have parted silently the
// day any default moved. Now it is enforced by call.
inline void reset_surface(MachineCtx* c, wgpu::Queue& queue,
    TileWorldState& tile_world_state, ThemesState& themes_state) {
    // Patches + tile cache
    init_patch_system(c, tile_world_state);
    c->world_state_.last_center_x = INT32_MAX;  // force full regen on next frame
    c->world_state_.last_center_z = INT32_MAX;
    c->world_state_.world_young = true;         // RIBBON_6: a rebirth is a world beginning

    // Terrain tokens — through the owner's door
    reset_terrain_memory(tile_world_state);

    c->spawn_engine_state_.entityQueueCount_ = 0;
    c->spawn_engine_state_.placementCount_ = 0;

    // Theme envelope — through the owner's door
    reset_theme_envelope(themes_state);

    // Footprints
    for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
        c->spawn_engine_state_.footprints_[i] = GroundFootprint{};
    }

    // Indoor shell
    c->gpuState_.set_shell_index_count(0);

    // Lights need re-upload with potentially new config
    c->mood_state_.lights_dirty = true;

    // New world decides its own upload frequency policy
    c->gpuState_.set_config_dynamic(false);
}

// The regen fan-out over the registry (pyramids bake into the
// heightfield; their post-commit is the caller).
inline void mark_patches_for_regen(MachineCtx* c, float min_wx, float min_wz,
    float max_wx, float max_wz,
    int32_t home_gx, int32_t home_gz) {
    int32_t pg_x0 = (int32_t)std::floor(min_wx / Dim::PATCH_EXTENT);
    int32_t pg_x1 = (int32_t)std::floor(max_wx / Dim::PATCH_EXTENT);
    int32_t pg_z0 = (int32_t)std::floor(min_wz / Dim::PATCH_EXTENT);
    int32_t pg_z1 = (int32_t)std::floor(max_wz / Dim::PATCH_EXTENT);

    for (uint32_t p = 0; p < c->world_state_.active_patch_count; p++) {
        if (c->patch_system_state_.patches_[p].phase != PatchPhase::GENERATED) continue;
        if (c->patch_system_state_.patches_[p].grid_x == home_gx && c->patch_system_state_.patches_[p].grid_z == home_gz) continue;
        if (c->patch_system_state_.patches_[p].grid_x >= pg_x0 && c->patch_system_state_.patches_[p].grid_x <= pg_x1 &&
            c->patch_system_state_.patches_[p].grid_z >= pg_z0 && c->patch_system_state_.patches_[p].grid_z <= pg_z1) {
            c->patch_system_state_.patches_[p].phase = PatchPhase::NEEDS_REGEN;
        }
    }
}

// ── Patch subsystem setup ──────────────────────────────────────────

inline void init_patch_system(MachineCtx* c, TileWorldState& tile_world_state) {
    for (uint32_t i = 0; i < Dim::MAX_ACTIVE_PATCHES; i++) {
        c->patch_system_state_.freeLayerStack_[i] = Dim::MAX_ACTIVE_PATCHES - 1 - i;
    }
    c->world_state_.free_layer_count = Dim::MAX_ACTIVE_PATCHES;
    c->world_state_.active_patch_count = 0;
    c->world_state_.render_patch_count = 0;
    c->world_state_.lod0_patch_count = 0;
    c->world_state_.all_patch_count = 0;
    c->gpuState_.stage_placement_patch_count(0);
    reset_tile_cache(tile_world_state);  // owner door
    c->world_state_.ground_entries_dirty = true;
    c->world_state_.patch_instances_dirty = true;
    c->world_state_.placement_dirty = true;
    c->world_state_.alloc_scan_pending = true;   // OIL_1 U9 — a fresh pool is all demand
}

// ── Patch generation ───────────────────────────────────────────────

// The params ride a read-only STORAGE array: one contiguous write for the
// whole batch before the pass is recorded, and workgroup_id.z picks the
// record. ONE BATCH A FRAME (LATTICE_1) — see stream_patches' bake block.
// ONE PASS, TWO DISPATCHES, THE WHOLE BATCH (LATTICE_1). This was a pass PAIR
// per patch: N patches cost 2N passes and 3N dispatches, each pass carrying a
// bind and a boundary that existed to order a scratch buffer which no longer
// exists. The bake reads its patch from patch_params_batch[workgroup_id.z], so
// the batch is the dispatch's z extent and the pass count stops depending on
// the batch size at all.
//
// CELLS RIDE THE SAME PASS. The binding ledger's W1-4 row lists
// {generate_patch_cells, generate_patch_heights} and {generate_patch_cells,
// generate_patch_gradients} as hazard-free in both directions — the pair
// writes disjoint textures — so the boundary between them was never buying
// ordering either.
inline void generate_patch_batch(MachineCtx* c, wgpu::CommandEncoder& encoder, wgpu::Queue& queue,
    const GPUPatchParams* params, uint32_t count) {
    if (count == 0) return;
    count = c->gpuState_.upload_patch_params(queue, params, count);
    if (count == 0) return;

    wgpu::ComputePassDescriptor cpd{};
    cpd.label = "Patch Bake (fused)";
    cpd.timestampWrites = c->gpuState_.meter_arm_compute(meter_row::StreamPatches);
    wgpu::ComputePassEncoder cp = encoder.BeginComputePass(&cpd);
    // LOOM_2 pass head: WORLD + FRAME are every pipeline's strata 0/1.
    { cp.SetBindGroup(0, c->gpuState_.world_group());
      cp.SetBindGroup(1, c->gpuState_.frame_c_group()); }
    // IOS_3 C — ?bake=0. A PURE SKIP: WebGPU zero-initialises a texture,
    // so an unbaked layer reads (0, 0, 0, 0) — height 0, gradients 0 — and
    // the world is FLAT BUT VISIBLE rather than garbage. That is what makes
    // this the discriminator Jean's lead wants: if the iPad renders a flat
    // world, the bake is the cause; if it stays black, the bake is
    // exonerated and the ladder continues.
    //
    // The CELLS still run below, deliberately: this isolates the
    // heightfield dispatch, not the batch shape or the pass.
    if (t7::boot_params().bake) {
        c->renderer_.dispatch_bake_patches(cp,
            c->gpuState_.patchgen_state_group(), c->gpuState_.patchgen_textures_group(),
            GPUState::patch_heightfield_workgroups(), count);
    }
    c->renderer_.dispatch_generate_patch_cells(cp,
        c->gpuState_.patchgen_state_group(), c->gpuState_.patchgen_textures_group(),
        GPUState::patch_cell_workgroups(), count);
    cp.End();
}

inline GPUPatchParams make_patch_params(MachineCtx* c, int32_t gx, int32_t gz, uint32_t layer) {
    GPUPatchParams p{};
    (void)c;   // LATTICE_1: the seed reaches the bake through config.world_seed
    p.origin[0] = (gx + 0.5f) * Dim::PATCH_EXTENT;
    p.origin[1] = (gz + 0.5f) * Dim::PATCH_EXTENT;
    p.layer = layer;
    p._pad0 = 0u;
    return p;
}

// ── Layer allocator ────────────────────────────────────────────────

inline uint32_t alloc_layer(MachineCtx* c) {
    if (c->world_state_.free_layer_count == 0) {
        // CONSERVATION BREAK (RIBBON_5): with MAX_ACTIVE_PATCHES equal to the
        // full 15x15 window there is zero headroom by design, and a caller
        // that reaches an empty pool is a caller the guards above failed.
        // Recycling layer 0 silently is how one mutating patch could chase a
        // player across an empty world — every comer writing the same
        // heightfield layer, endlessly re-baked. Loud, always-on, rate-limited.
        static uint32_t s_starved = 0;
        if ((s_starved++ & 63u) == 0u)
            std::fprintf(stderr, "[Patch] LAYER POOL EMPTY — alloc refused "
                "(active=%u free=%u). A layer leaked or headroom is zero.\n",
                c->world_state_.active_patch_count, c->world_state_.free_layer_count);
        return UINT32_MAX;   // refuse: no patch on a stolen layer
    }
    return c->patch_system_state_.freeLayerStack_[--c->world_state_.free_layer_count];
}

inline void free_layer(MachineCtx* c, uint32_t layer) {
    // CONSERVATION (RIBBON_5), the other end: a layer returned twice, or one
    // returned that was never taken, overflows the stack and corrupts whatever
    // follows it. The pool cannot hold more than it owns.
    if (c->world_state_.free_layer_count >= Dim::MAX_ACTIVE_PATCHES ||
        layer >= Dim::MAX_ACTIVE_PATCHES) {
        std::fprintf(stderr, "[Patch] LAYER POOL OVERFLOW — free(%u) refused "
            "(free=%u of %u). A layer was returned twice.\n",
            layer, c->world_state_.free_layer_count, Dim::MAX_ACTIVE_PATCHES);
        return;
    }
    c->patch_system_state_.freeLayerStack_[c->world_state_.free_layer_count++] = layer;
}

// Check if grid coordinate is within the allocation window (world_state_.active_radius = Dim::PATCH_PREGEN_RADIUS)
inline bool in_render_window(MachineCtx* c, int32_t gx, int32_t gz, int32_t cx, int32_t cz) {
    int32_t r = (int32_t)c->world_state_.active_radius;
    return gx >= cx - r && gx <= cx + r &&
        gz >= cz - r && gz <= cz + r;
}

// ── Visibility cylinder ────────────────────────────────────────────

// Distance² from point (px,pz) to nearest edge of a patch AABB.
// Zero when the point is inside the patch.
inline float patch_distance_sq(float px, float pz,
    float origin_x, float origin_z, float half) {
    float dx = std::max(0.0f, std::abs(px - origin_x) - half);
    float dz = std::max(0.0f, std::abs(pz - origin_z) - half);
    return dx * dx + dz * dz;
}

// ── Patch streaming helpers ────────────────────────────────────────

template<typename Pred>
inline uint32_t collect_sorted_patches(MachineCtx* c, PatchCandidate* out,
    float point_wx, float point_wz, Pred&& pred, bool nearest_first)
{
    float half = Dim::PATCH_EXTENT * 0.5f;
    uint32_t count = 0;
    for (uint32_t i = 0; i < c->world_state_.active_patch_count; i++) {
        if (!c->patch_system_state_.patches_[i].valid) continue;
        if (!pred(c->patch_system_state_.patches_[i])) continue;
        float ox = (c->patch_system_state_.patches_[i].grid_x + 0.5f) * Dim::PATCH_EXTENT;
        float oz = (c->patch_system_state_.patches_[i].grid_z + 0.5f) * Dim::PATCH_EXTENT;
        float d2 = patch_distance_sq(point_wx, point_wz, ox, oz, half);
        out[count++] = { i, d2 };
    }
    for (uint32_t i = 1; i < count; i++) {
        PatchCandidate key = out[i];
        uint32_t j = i;
        if (nearest_first) {
            while (j > 0 && out[j - 1].dist2 > key.dist2) {
                out[j] = out[j - 1]; j--;
            }
        }
        else {
            while (j > 0 && out[j - 1].dist2 < key.dist2) {
                out[j] = out[j - 1]; j--;
            }
        }
        out[j] = key;
    }
    return count;
}

// Check if grid coordinate is within the priority window (Dim::PATCH_GRID_RADIUS)
inline bool in_priority_window(MachineCtx* c, int32_t gx, int32_t gz, int32_t cx, int32_t cz) {
    int32_t r = (int32_t)Dim::PATCH_GRID_RADIUS;
    return gx >= cx - r && gx <= cx + r &&
        gz >= cz - r && gz <= cz + r;
}

// Process entity spawn for pre-collected patch candidates.
//
// ONE PATCH, DRAINED WHOLE (PANORAMA_0 RIDE_1). The three verbs used to run
// as three passes over the whole candidate list: every patch selected, then
// everything placed, then everything committed. That made the queue's
// capacity a function of the CANDIDATE COUNT, and the fullRegen arm hands
// this function all 49 patches of the priority window in one call — so the
// queue overflowed at boot and at every portal, and what it dropped was the
// tail of PLACEMENT_ORDER: galleries first, then arches, cubes, columns,
// GoL. The birth of every world was being truncated by a bound that called
// itself proven.
//
// Draining per patch makes the bound true instead of arguing it: at most one
// patch's selections are ever in flight, so PopFamily::COUNT is the real
// ceiling and no candidate list can reach it.
//
// PLACEMENT ORDER IS UNCHANGED. Select already queued patch-by-patch, and
// footprints register at PLACE — so patch N+1's place sees exactly the ground
// it saw before, whether patch N's place happened one iteration earlier or one
// pass earlier. The one difference is real and is an improvement: a commit
// that RELEASES ground (a gallery that placed no paintings unregisters its
// footprint) now does so before the next patch places, so ground that is
// genuinely free can be used by it.
inline void spawn_selected_patches(MachineCtx* c, const PatchCandidate* candidates, uint32_t count,
    wgpu::Queue& queue,
    ThemesState& themes_state) {
    for (uint32_t s = 0; s < count; s++) {
        uint32_t pi = candidates[s].idx;
        evaluate_theme_envelope(themes_state, c,
            tile_seed(c->world_state_.active_seed, c->patch_system_state_.patches_[pi].grid_x, c->patch_system_state_.patches_[pi].grid_z));
        select_entities_for_patch(c, c->patch_system_state_.patches_[pi].grid_x, c->patch_system_state_.patches_[pi].grid_z);
        c->patch_system_state_.patches_[pi].phase = PatchPhase::SPAWNED;
        place_entity_queue(c);
        commit_entity_queue(c, queue);
    }

    for (uint32_t s = 0; s < count; s++) {
        uint32_t pi = candidates[s].idx;
        int32_t gx = c->patch_system_state_.patches_[pi].grid_x;
        int32_t gz = c->patch_system_state_.patches_[pi].grid_z;
        // Two-tip registration through the owner's door: the
        // ref-count protocol lives whole in bodies/ribbon.hpp now.
        ribbon_register_tips_at(c->ribbon_state_, c->patch_system_state_.patches_[pi], gx, gz);
    }
}

// Process heightfield generation for pre-collected patch candidates.
inline void generate_selected_patches(MachineCtx* c, const PatchCandidate* candidates, uint32_t count,
    wgpu::CommandEncoder& encoder, wgpu::Queue& queue,
    bool& tileGridDirty,
    TileWorldState& tile_world_state, TileWorldDeps& tile_world_deps) {
    if (count == 0) return;
    if (tileGridDirty) {
        upload_tile_grid_now(tile_world_state, &tile_world_deps, queue, c->world_state_.last_center_x, c->world_state_.last_center_z);
        tileGridDirty = false;
    }
    GPUPatchParams batchParams[Dim::MAX_ACTIVE_PATCHES];
    uint32_t batchIdx[Dim::MAX_ACTIVE_PATCHES];
    for (uint32_t i = 0; i < count; i++) {
        uint32_t pi = candidates[i].idx;
        batchParams[i] = make_patch_params(c, 
            c->patch_system_state_.patches_[pi].grid_x, c->patch_system_state_.patches_[pi].grid_z, c->patch_system_state_.patches_[pi].layer);
        batchIdx[i] = pi;
    }
    generate_patch_batch(c, encoder, queue, batchParams, count);
    for (uint32_t b = 0; b < count; b++) {
        uint32_t pi = batchIdx[b];
        c->patch_system_state_.patches_[pi].phase = PatchPhase::GENERATED;
    }
    c->world_state_.patch_instances_dirty = true;
}

// ── band_patches (visibility/LOD → the draw-instance set) ──────────
// One unit of the conductor: walk the GENERATED patches, gate each on the
// world-space visibility cylinder (finite mode = all visible, "walls
// define boundary, not fog"), split LOD0/LOD1/pregen, pack them
// [lod0|lod1|pregen] (the draw-split contract the render reads by count),
// upload the instance buffer + the lod0/render/all counts + the placement
// patch count, and push lod_point so the frustum-cull shader re-bands on
// the SAME point the CPU banded on (the anti-flicker contract).
//   offer-face: GPUPatchInstance[] + lod0/render/all counts + lod_point.
//   requires:   patches_ registry, the point readback, patch_distance_sq,
//               the LIVE veil chain (config veil_ring/lod0_radius),
//               finite_mode; the upload doors.
// Bit-safe: pack order = wire layout, not a draw. Separate from
// build_patch_grid (different consumer/offer-face); they only coincided
// in the same tail block.
inline void band_patches(MachineCtx* c, wgpu::Queue& queue) {
    // OIL_1 U10 (ledger: R3 band_patches, C1): the four arrays carry no
    // value-init — ~14.4 KB of per-frame stack zeroing retired. Safe by
    // the prefix proof: each band array is written [0, count) before the
    // memcpy reads [0, count), instances is written [0, w) by the three
    // memcpys before the upload reads [0, w), and every GPUPatchInstance
    // field is assigned at the pack site (inst carries its own init).
    GPUPatchInstance instances[Dim::MAX_ACTIVE_PATCHES];
    uint32_t lod0Count = 0;
    uint32_t lod1Count = 0;
    uint32_t pregenCount = 0;

    // Temporary arrays for each band
    GPUPatchInstance lod0[Dim::MAX_ACTIVE_PATCHES];
    GPUPatchInstance lod1[Dim::MAX_ACTIVE_PATCHES];
    GPUPatchInstance pregen[Dim::MAX_ACTIVE_PATCHES];

    float point_wx = c->point_.x;   // THE POINT (1-frame stale by law E-4)
    float point_wz = c->point_.z;
    float half = Dim::PATCH_EXTENT * 0.5f;

    // THE VEIL CHAIN, live: THE RING is the draw authority (nothing draws
    // beyond it); lod0 is the full/half-mesh split. The same config values
    // the GPU gate + the fragment icing read — one yardstick everywhere.
    const float ring_sq = c->gpuState_.veil_ring()   * c->gpuState_.veil_ring();
    const float lod0_sq = c->gpuState_.lod0_radius() * c->gpuState_.lod0_radius();

    for (uint32_t i = 0; i < c->world_state_.active_patch_count; i++) {
        if (c->patch_system_state_.patches_[i].phase != PatchPhase::GENERATED &&
            c->patch_system_state_.patches_[i].phase != PatchPhase::NEEDS_REGEN) continue;

        float ox = (c->patch_system_state_.patches_[i].grid_x + 0.5f) * Dim::PATCH_EXTENT;
        float oz = (c->patch_system_state_.patches_[i].grid_z + 0.5f) * Dim::PATCH_EXTENT;

        GPUPatchInstance inst{};
        inst.origin[0] = ox;
        inst.origin[1] = oz;
        inst.extent = Dim::PATCH_EXTENT;
        inst.layer = c->patch_system_state_.patches_[i].layer;

        float d2 = patch_distance_sq(point_wx, point_wz, ox, oz, half);

        // Finite mode: all patches visible (walls define boundary, not fog)
        if (c->world_state_.finite_mode || d2 <= ring_sq) {
            if (d2 <= lod0_sq) {
                lod0[lod0Count++] = inst;
            }
            else {
                lod1[lod1Count++] = inst;
            }
        }
        else {
            pregen[pregenCount++] = inst;
        }
    }

    // Pack: LOD-0, then LOD-1, then pregen
    uint32_t w = 0;
    std::memcpy(instances + w, lod0, lod0Count * sizeof(GPUPatchInstance)); w += lod0Count;
    std::memcpy(instances + w, lod1, lod1Count * sizeof(GPUPatchInstance)); w += lod1Count;
    std::memcpy(instances + w, pregen, pregenCount * sizeof(GPUPatchInstance)); w += pregenCount;

    c->gpuState_.upload_patch_instances(queue, instances, w);
    c->world_state_.lod0_patch_count = lod0Count;
    c->world_state_.render_patch_count = lod0Count + lod1Count;
    c->world_state_.all_patch_count = w;

    // Sync placement_patch_count so compute_entity_placement
    // can sample heightfields from the current frame's patch set.
    c->gpuState_.stage_placement_patch_count(w);
    c->gpuState_.upload_placement_patch_count(queue);

    c->gpuState_.stage_lod_point(point_wx, point_wz);
    c->gpuState_.upload_lod_point(queue);
}

// ── build_patch_grid (the (gx,gz)→layer index the baked sampler reads) ──
// The OTHER conductor-tail unit: build the GPUPatchGrid — the O(1) spatial
// index (origin + side + per-cell layer+1, 0=empty) that
// sample_terrain_y_at hashes into to find a patch's heightfield layer. Its
// OWN walk over the GENERATED patches; a separate consumer and offer-face
// from band_patches — they only coincided in the same tail block, never
// shared the walk.
//   offer-face: GPUPatchGrid (origin_x/z, side, entries[]) uploaded.
//   requires:   patches_ registry (grid_x/grid_z/layer/valid/phase),
//               PATCH_PREGEN_SIDE, Dim::PATCH_EXTENT; upload_patch_grid.
// Bit-safe: index layout = wire layout, not a draw. entries = layer+1
// (0 = empty) is the frozen convention the WGSL sampler decodes.
inline void build_patch_grid(MachineCtx* c, wgpu::Queue& queue) {
    GPUPatchGrid grid{};
    grid.side = Dim::PATCH_PREGEN_SIDE;
    grid.cell_extent = Dim::PATCH_EXTENT;

    int32_t min_gx = INT32_MAX;
    int32_t min_gz = INT32_MAX;
    for (uint32_t i = 0; i < c->world_state_.active_patch_count; i++) {
        if (!c->patch_system_state_.patches_[i].valid) continue;
        if (c->patch_system_state_.patches_[i].phase != PatchPhase::GENERATED &&
            c->patch_system_state_.patches_[i].phase != PatchPhase::NEEDS_REGEN) continue;
        min_gx = std::min(min_gx, c->patch_system_state_.patches_[i].grid_x);
        min_gz = std::min(min_gz, c->patch_system_state_.patches_[i].grid_z);
    }
    if (min_gx == INT32_MAX) { min_gx = 0; min_gz = 0; }
    grid.origin_x = min_gx;
    grid.origin_z = min_gz;

    for (uint32_t i = 0; i < c->world_state_.active_patch_count; i++) {
        if (!c->patch_system_state_.patches_[i].valid) continue;
        if (c->patch_system_state_.patches_[i].phase != PatchPhase::GENERATED &&
            c->patch_system_state_.patches_[i].phase != PatchPhase::NEEDS_REGEN) continue;
        int32_t lx = c->patch_system_state_.patches_[i].grid_x - grid.origin_x;
        int32_t lz = c->patch_system_state_.patches_[i].grid_z - grid.origin_z;
        if (lx < 0 || lz < 0 ||
            lx >= int32_t(grid.side) || lz >= int32_t(grid.side)) continue;
        grid.entries[lz * grid.side + lx] = c->patch_system_state_.patches_[i].layer + 1u;
    }

    c->gpuState_.upload_patch_grid(queue, grid);
}

// ── build_active_patch_set (the (gx,gz) existence set — O(1) lookup) ────
// Rebuilt from the registry each call — a per-scan LOCAL, NOT a maintained
// member (evict/compaction keep patches_ correct; this reads it fresh). It
// enumerates ALL active-count entries (no .valid filter — matching the raw
// scans it replaces, so existence is bit-identical). Shared by the alloc
// window scan AND the fullRegen re-roll (killing that inner scan's O(N^2)).
// find_patch stays the O(N) single-lookup handle — a persistent index for
// one lookup isn't worth an alloc/evict/compaction invariant. Bit-safe:
// existence, not value.
inline std::unordered_set<GridKey, GridKeyHash> build_active_patch_set(MachineCtx* c) {
    std::unordered_set<GridKey, GridKeyHash> set;
    set.reserve(c->world_state_.active_patch_count);
    for (uint32_t i = 0; i < c->world_state_.active_patch_count; i++) {
        set.insert({ c->patch_system_state_.patches_[i].grid_x, c->patch_system_state_.patches_[i].grid_z });
    }
    return set;
}

// --- Patch streaming: determine active 7×7 grid, generate new patches ---
// THE CONDUCTOR: the per-frame step — recenter,
// eviction, allocation, spawn + generation budgets, visibility
// banding, deferred uploads.
// SEAM[patch:spawn-trigger] the S3-trigger calls are the declared
//   seam face: select_entities_for_patch / place_entity_queue /
//   commit_entity_queue (via spawn_selected_patches), plus
//   update_entity_draw_visibility at the frame tail — the surface
//   machine waking the occupier machine.
inline void stream_patches(MachineCtx* c, wgpu::CommandEncoder& encoder, wgpu::Queue& queue,
    TileWorldState& tile_world_state, ThemesState& themes_state,
    TileWorldDeps& tile_world_deps, MoodDeps& mood_deps) {
    // ─── Patch Generation Pipeline ─────────────────────────────────

    int32_t centerX, centerZ;
    // ONE BAKE BATCH A FRAME (LATTICE_1's R-D fold). A LOCAL, and that is
    // the whole mechanism: it is false at the top of every frame by
    // construction, so there is no reset to forget. It replaced a params-
    // ring cursor, which existed because a full regen made two batches into
    // one encoder and the queue orders every WriteBuffer ahead of the whole
    // command buffer — the second batch's write would have landed on the
    // first's records before either dispatch ran. With one batch there is
    // nothing to cursor.
    bool bakedThisFrame = false;
    bool tileGridDirty = false;        // coalesce tile grid uploads to one per frame
    if (c->world_state_.finite_mode) {
        centerX = 0;
        centerZ = 0;
    }
    else {
        centerX = (int32_t)std::floor(c->point_.x / Dim::PATCH_EXTENT);
        centerZ = (int32_t)std::floor(c->point_.z / Dim::PATCH_EXTENT);
    }

    // In finite mode, cap the effective radius
    uint32_t savedRadius = c->world_state_.active_radius;
    if (c->world_state_.finite_mode && c->world_state_.active_radius > c->world_state_.finite_radius) {
        c->world_state_.active_radius = c->world_state_.finite_radius;
    }

    bool gridChanged = (centerX != c->world_state_.last_center_x || centerZ != c->world_state_.last_center_z);

    if (gridChanged) {
        int32_t oldCX = c->world_state_.last_center_x;
        int32_t oldCZ = c->world_state_.last_center_z;
        c->world_state_.last_center_x = centerX;
        c->world_state_.last_center_z = centerZ;
        c->world_state_.alloc_scan_pending = true;   // OIL_1 U9 — the window moved: new cells may be empty

        bool fullRegen = (oldCX == INT32_MAX);  // first frame

        // Lightweight cache maintenance (no GPU buffer writes). CONTAINMENT
        // (FIX_TILE_PATCH_CONTAINMENT): spare every tile a live patch stands on.
        // build_active_patch_set enumerates the registry with no .valid filter
        // and runs BEFORE the budgeted patch-eviction pass below, so it includes
        // patches already out of the render window but not yet drained -- exactly
        // the ones a centre jump would orphan.
        auto liveTiles = build_active_patch_set(c);
        evict_distant_tiles(tile_world_state, centerX, centerZ,
            [&](const GridKey& k) { return liveTiles.count(k) > 0; });

        if (!fullRegen) {
            tileGridDirty = true;
        }

        // ─── FULLREGEN: synchronous bootstrap ────────────────────
        //
        if (fullRegen) {
            int32_t rr = (int32_t)c->world_state_.active_radius;
            static constexpr int32_t TILE_PAD = 1;
            int32_t rp = rr + TILE_PAD;
            for (int32_t gz = centerZ - rp; gz <= centerZ + rp; gz++) {
                for (int32_t gx = centerX - rp; gx <= centerX + rp; gx++) {
                    ensure_tile(tile_world_state, &tile_world_deps, gx, gz);  // owner door
                }
            }

            // NOW spawn portals — tile cache is populated, terrain heights are correct
            if (c->mood_state_.back_portal_pending) {
                force_spawn_back_portal(&mood_deps, queue, *c);
            }
            // ATRIUM_0 — BACK FIRST, THEN THE FORWARDS. The forward roster
            // reads the standing arches to learn which wall the back owns,
            // so its order against the line above is structural, not
            // stylistic. Both stay ahead of force_spawn_door_fallback,
            // whose gate counts standing doors.
            if (c->mood_state_.forward_portals_pending) {
                force_spawn_forward_portals(&mood_deps, queue, *c);
            }
            // Built ONCE before the window loop: bit-identical to the old
            // per-cell O(N) scan (each cell is unique, so a patch added at an
            // earlier cell never matches a later one). Kills the O(N^2).
            auto existingPatches = build_active_patch_set(c);
            for (int32_t gz = centerZ - rr; gz <= centerZ + rr; gz++) {
                for (int32_t gx = centerX - rr; gx <= centerX + rr; gx++) {
                    bool found = existingPatches.count({ gx, gz }) > 0;
                    if (!found && c->world_state_.free_layer_count > 0) {
                        // RIBBON_5: the array bound, stated not derived (see the
                        // continuous loop for why). This loop's own guard is
                        // per-iteration already, so the pool cannot empty under it.
                        if (c->world_state_.active_patch_count >= Dim::MAX_ACTIVE_PATCHES) continue;
                        uint32_t layer = alloc_layer(c);
                        if (layer == UINT32_MAX) continue;   // the pool refused
                        c->patch_system_state_.patches_[c->world_state_.active_patch_count] = ActivePatch{};
                        c->patch_system_state_.patches_[c->world_state_.active_patch_count].grid_x = gx;
                        c->patch_system_state_.patches_[c->world_state_.active_patch_count].grid_z = gz;
                        c->patch_system_state_.patches_[c->world_state_.active_patch_count].layer = layer;
                        c->patch_system_state_.patches_[c->world_state_.active_patch_count].valid = true;
                        c->world_state_.active_patch_count++;
                    }
                }
            }
            tileGridDirty = true;

            // Spawn inner patches
            PatchCandidate spawnCands[Dim::MAX_ACTIVE_PATCHES];
            uint32_t spawnCount = collect_sorted_patches(c, spawnCands,
                c->point_.x, c->point_.z,
                [&](const ActivePatch& p) {
                    return p.phase == PatchPhase::ALLOCATED &&
                        in_priority_window(c, p.grid_x, p.grid_z, centerX, centerZ);
                }, true);
            spawn_selected_patches(c, spawnCands, spawnCount, queue, themes_state);

            // Generate inner patches
            PatchCandidate genCands[Dim::MAX_ACTIVE_PATCHES];
            uint32_t genCount = collect_sorted_patches(c, genCands,
                c->point_.x, c->point_.z,
                [&](const ActivePatch& p) {
                    return p.phase == PatchPhase::SPAWNED &&
                        in_priority_window(c, p.grid_x, p.grid_z, centerX, centerZ);
                }, true);
            generate_selected_patches(c, genCands, genCount,
                encoder, queue, tileGridDirty, tile_world_state, tile_world_deps);
            bakedThisFrame = true;

            // THE DOOR GUARANTEE (U2) — after the synchronous population
            // above, so Channel A's DOORWAY portals are countable: a world
            // that populated doorless gets one forced door; every other
            // world sees a no-op (the count-gate is inside).
            force_spawn_door_fallback(&mood_deps, queue, *c);

            // THE BIRTH CENSUS (OVERTURE_0). `boot` prints before population
            // and reads zero by construction; this is the first count of a
            // world that exists. Always-on, like boot: once per fullRegen,
            // never per frame.
            dump_entity_census(c, "born");
        }
    }

    // ─── A WORLD IS YOUNG ONCE (RIBBON_6) ─────────────────────────
    //
    // RIBBON_5 asked the registry every frame whether the window was mostly
    // unbuilt — but a world that has fallen BEHIND while moving looks exactly
    // like a world being born, so a fast flight was handed a rebirth's burst,
    // cleared it, fell behind, and was handed another. Six whole bakes in one
    // frame is ~14 ms of GPU on an 11 ms base frame: a missed refresh, a few
    // times a second, only while moving, worse the faster you go. RIBBON_5's
    // FLAG-40 saw the oscillation and priced it as harmless. Both arms made
    // progress; one of them cost a frame.
    //
    // Youth is not an observation; it is an AGE. It is set when a world BEGINS
    // — a rebirth, a boot, a radius change — and cleared once, when the window
    // it was given is three quarters built. Nothing the player does sets it
    // again, so the oscillation cannot exist.
    //
    // The count runs only while young: a whole world pays nothing for this.
    if (c->world_state_.world_young) {
        uint32_t built = 0, cells = 2u * c->world_state_.active_radius + 1u;
        cells *= cells;
        for (uint32_t i = 0; i < c->world_state_.active_patch_count; i++) {
            if (!c->patch_system_state_.patches_[i].valid) continue;
            const PatchPhase ph = c->patch_system_state_.patches_[i].phase;
            if (ph == PatchPhase::GENERATED || ph == PatchPhase::NEEDS_REGEN) built++;
        }
        if (built * 4u >= cells * 3u) c->world_state_.world_young = false;
    }
    const bool young = c->world_state_.world_young;

    // ─── CONTINUOUS PATCH EVICTION ────────────────────────────────
    //
    {
        PatchCandidate candidates[Dim::MAX_ACTIVE_PATCHES];
        uint32_t count = collect_sorted_patches(c, candidates,
            c->point_.x, c->point_.z,
            [&](const ActivePatch& p) {
                return !in_render_window(c, p.grid_x, p.grid_z,
                    c->world_state_.last_center_x, c->world_state_.last_center_z);
            }, false);  // farthest first

        // RIBBON_6: EVICT is 4 again and matches ALLOC, so the pool balances
        // by symmetry as well as by refusal (RIBBON_5's FLAG-38 — the spend
        // side was the break). `young` selects the same number either way; the
        // branch is kept because the two are different FACTS, and a later
        // round may move one without the other.
        uint32_t evictThisFrame = std::min(count, young ? 4u : EVICT_BUDGET_PER_FRAME);
        for (uint32_t e = 0; e < evictThisFrame; e++) {
            evict_patch(c, candidates[e].idx, queue);
        }

        if (evictThisFrame > 0) {
            uint32_t write = 0;
            for (uint32_t i = 0; i < c->world_state_.active_patch_count; i++) {
                if (c->patch_system_state_.patches_[i].valid) c->patch_system_state_.patches_[write++] = c->patch_system_state_.patches_[i];
            }
            c->world_state_.active_patch_count = write;
            c->world_state_.patch_instances_dirty = true;
            // OIL_1 U9 — layers were freed: a capacity-blocked vacancy
            // (counted as zero candidates while free_layer_count was 0)
            // can now allocate, so the scan must run again.
            c->world_state_.alloc_scan_pending = true;
        }
    }

    // ─── CONTINUOUS PATCH ALLOCATION ──────────────────────────────
    //
    // OIL_1 U9 (ledger: R3 continuous allocation, C2): the scan — a
    // fresh unordered_set heap build plus the 15x15 window walk — runs
    // only while demand can exist (alloc_scan_pending; raiser census at
    // the flag's home in surface_services.hpp). Steady frames skip it;
    // a budget-capped pass re-arms itself below.
    {
        // The box raiser: the candidate set is box ∩ window, and the box
        // follows the pawn cell — an input no other raiser tracks once
        // the window is pinned (finite mode). Two compares, and the gate
        // stops depending on why the box and the window agree.
        const int32_t scanGX = (int32_t)std::floor(c->point_.x / Dim::PATCH_EXTENT);
        const int32_t scanGZ = (int32_t)std::floor(c->point_.z / Dim::PATCH_EXTENT);
        if (scanGX != c->world_state_.last_alloc_scan_gx ||
            scanGZ != c->world_state_.last_alloc_scan_gz) {
            c->world_state_.last_alloc_scan_gx = scanGX;
            c->world_state_.last_alloc_scan_gz = scanGZ;
            c->world_state_.alloc_scan_pending = true;
        }
    }
    if (c->world_state_.alloc_scan_pending) {
        int32_t pawnGX = (int32_t)std::floor(c->point_.x / Dim::PATCH_EXTENT);
        int32_t pawnGZ = (int32_t)std::floor(c->point_.z / Dim::PATCH_EXTENT);
        int32_t rr = (int32_t)c->world_state_.active_radius;
        float point_wx = c->point_.x;
        float point_wz = c->point_.z;
        float half = Dim::PATCH_EXTENT * 0.5f;

        // O(1) patch existence lookup (replaces O(N) inner scan)
        auto activePatchSet = build_active_patch_set(c);

        struct AllocCandidate { int32_t gx, gz; float dist2; };
        AllocCandidate candidates[Dim::MAX_ACTIVE_PATCHES];
        uint32_t candidateCount = 0;
        uint32_t vacancies_blocked = 0;   // RIBBON_5 — cells that wanted a layer and found none

        for (int32_t gz = pawnGZ - rr; gz <= pawnGZ + rr; gz++) {
            for (int32_t gx = pawnGX - rr; gx <= pawnGX + rr; gx++) {
                // Must be within allocation window of grid center
                if (!in_render_window(c, gx, gz, c->world_state_.last_center_x, c->world_state_.last_center_z)) continue;
                bool found = activePatchSet.count({ gx, gz }) > 0;
                // RIBBON_5: a vacancy the POOL refused is demand that still
                // exists; the re-arm below needs to know it was seen.
                if (!found && c->world_state_.free_layer_count == 0) vacancies_blocked++;
                if (!found && c->world_state_.free_layer_count > 0 && candidateCount < Dim::MAX_ACTIVE_PATCHES) {
                    float ox = (gx + 0.5f) * Dim::PATCH_EXTENT;
                    float oz = (gz + 0.5f) * Dim::PATCH_EXTENT;
                    float d2 = patch_distance_sq(point_wx, point_wz, ox, oz, half);
                    candidates[candidateCount++] = { gx, gz, d2 };
                }
            }
        }

        // Sort by distance (nearest first)
        for (uint32_t i = 1; i < candidateCount; i++) {
            AllocCandidate key = candidates[i];
            uint32_t j = i;
            while (j > 0 && candidates[j - 1].dist2 > key.dist2) {
                candidates[j] = candidates[j - 1];
                j--;
            }
            candidates[j] = key;
        }

        bool allocated_any = false;
        uint32_t allocThisFrame = std::min(candidateCount, ALLOC_BUDGET_PER_FRAME);
        for (uint32_t a = 0; a < allocThisFrame; a++) {
            int32_t gx = candidates[a].gx;
            int32_t gz = candidates[a].gz;
            // Ensure tile cache entry (primary — ticks terrain tokens),
            // then cache neighbors for tile grid padding — both through
            // the owner's doors.
            ensure_tile(tile_world_state, &tile_world_deps, gx, gz);
            for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) {
                ensure_tile_padding(tile_world_state, &tile_world_deps, gx + dx, gz + dz);
            }
            // RIBBON_5 — TWO GUARDS, AND THEY ARE INDEPENDENT ON PURPOSE.
            // The capacity check that mattered used to sit in the CANDIDATE
            // SCAN above, which decrements nothing: with free=2 and 15 vacant
            // cells every cell became a candidate, allocThisFrame was
            // min(15, ALLOC_BUDGET_PER_FRAME)=4, and iterations 2 and 3
            // allocated against an empty pool. The second guard is the array
            // bound, stated rather than derived: patches_ is exactly
            // MAX_ACTIVE_PATCHES long and freeLayerStack_ is the member
            // immediately after it, so an overrun here corrupts the free list
            // itself. It must not rest on the pool arithmetic being right.
            if (c->world_state_.active_patch_count >= Dim::MAX_ACTIVE_PATCHES) break;
            uint32_t layer = alloc_layer(c);
            if (layer == UINT32_MAX) break;   // the pool refused; no patch on a stolen layer
            c->patch_system_state_.patches_[c->world_state_.active_patch_count] = ActivePatch{};
            c->patch_system_state_.patches_[c->world_state_.active_patch_count].grid_x = gx;
            c->patch_system_state_.patches_[c->world_state_.active_patch_count].grid_z = gz;
            c->patch_system_state_.patches_[c->world_state_.active_patch_count].layer = layer;
            c->patch_system_state_.patches_[c->world_state_.active_patch_count].valid = true;
            c->world_state_.active_patch_count++;
            allocated_any = true;
        }

        // Mark tile grid and patch instances dirty whenever new patches were allocated
        if (allocated_any) {
            tileGridDirty = true;
            c->world_state_.patch_instances_dirty = true;
        }

        // OIL_1 U9 — the backlog carry: a pass capped by
        // ALLOC_BUDGET_PER_FRAME leaves live candidates, so the scan
        // stays armed; a drained pass (every candidate allocated, or
        // none found) disarms it. A vacancy blocked by zero capacity
        // counts no candidate and disarms too — the eviction that
        // eventually frees a layer is the raiser that re-arms (see the
        // eviction block above).
        // RIBBON_5 — THE RE-ARMER'S HOLE. A vacancy blocked by ZERO CAPACITY
        // counted no candidate, so the line below disarmed the scan and left
        // the only re-armer an eviction. If the registry ever sits with real
        // vacancies and an empty pool, and nothing is evictable that frame,
        // the scan sleeps and the world stops growing. Capacity is a
        // TEMPORARY blocker, so it must not disarm: stay armed while a
        // vacancy exists that only the pool refused.
        c->world_state_.alloc_scan_pending =
            (candidateCount > allocThisFrame) ||
            (vacancies_blocked > 0 && c->world_state_.free_layer_count == 0);
    }

    // THE LOOK-AHEAD (RIBBON_4): under a rider the point moves at flight speed,
    // and the patches that matter are the ones ahead. The spawn and bake scans
    // order their candidates from a point PATCH_LOOK_AHEAD along the flight —
    // the saddle's heading, from the same readback the point mirror is filled
    // by. Flight is −dir(heading) (the ribbon's law). The allocation window and
    // the eviction stay on the point itself: the window is the authority on
    // what EXISTS, and only the order in which existing work is served moves.
    const float look = (c->point_.host == PointHost::RIBBON) ? PATCH_LOOK_AHEAD : 0.0f;
    const float gen_cx = c->point_.x - std::cos(c->point_.heading) * look;
    const float gen_cz = c->point_.z - std::sin(c->point_.heading) * look;

    // ─── DISTANCE-DRIVEN ENTITY SPAWNING ─────────────────────────
    //
    {
        PatchCandidate candidates[Dim::MAX_ACTIVE_PATCHES];
        uint32_t count = collect_sorted_patches(c, candidates,
            gen_cx, gen_cz,
            [](const ActivePatch& p) {
                return p.phase == PatchPhase::ALLOCATED;
            }, true);
        // RIBBON_5: the burst is the transition's, the one-a-frame is the
        // steady world's.
        spawn_selected_patches(c, candidates,
            std::min(count, young ? 4u : SPAWN_BUDGET_PER_FRAME), queue, themes_state);
    }

    // THE DEFERRED HANG (OVERTURE_0) — the surface machine waking the
    // gallery's owner verb; the verb owns the fill, the pool test and the
    // retry. ROSTER-gated like every gallery consumer.
    if constexpr (ROSTER.gallery) tick_gallery_deferred_hang(c, queue);
    // THE TIDE (REPEAT_2 D2) — the second driver, beside the first. It hangs
    // off the FRAME, not off displacement: this call sits at function-body
    // depth, outside `if (gridChanged)`, so it runs for a motionless visitor
    // and in the finite world where centerX is pinned and no patch ever
    // leaves. That placement is the whole unit.
    if constexpr (ROSTER.gallery) tick_gallery_tide(c, queue);

    // ─── HEIGHTFIELD GENERATION — ONE ARM (RIBBON_6) ─────────────
    //
    // A FRAME BAKES ONE PATCH, UNLESS THE WORLD IS BEING BORN. One list over
    // both pending phases, nearest first from the look-ahead point, and
    // exactly BAKE_BUDGET_PER_FRAME of them baked whole.
    //
    // NEAREST FIRST IS THE WHOLE PRIORITY LAW: a hole beside the eye outranks
    // a pyramid's stale ground three patches away, and neither needs a phase
    // of its own to say so. That is why the two arms RIBBON_4 split and
    // RIBBON_5 kept are one again — the split existed to give the sliced
    // conductor a phase it could own, and there is no sliced conductor.
    {
        PatchCandidate candidates[Dim::MAX_ACTIVE_PATCHES];
        uint32_t count = collect_sorted_patches(c, candidates, gen_cx, gen_cz,
            [](const ActivePatch& p) {
                return p.phase == PatchPhase::SPAWNED ||
                       p.phase == PatchPhase::NEEDS_REGEN;
            }, true);
        // ONE BAKE BATCH A FRAME (LATTICE_1, the R-D fold). The fullRegen arm
        // above already spent this frame's bake — on the whole priority window,
        // which the batch shape makes a single dispatch — and the params buffer
        // is written once per frame at record 0. A second batch in the same
        // encoder would have its write land on the first's records before
        // EITHER dispatch ran, because the queue orders every WriteBuffer ahead
        // of the command buffer. That is what the retired 256-byte ring cursor
        // was for.
        //
        // The cost of folding is that up to SPAWN_BUDGET's worth of patches
        // spawned later in this same frame wait one frame to bake. They are the
        // ones OUTSIDE the priority window — further from the point than
        // anything the fullRegen just baked — and nearest-first already serves
        // them last. Behind the veil at boot and behind the fade at a portal.
        if (!bakedThisFrame) {
            generate_selected_patches(c, candidates,
                std::min(count, young ? BAKE_BUDGET_YOUNG : BAKE_BUDGET_PER_FRAME),
                encoder, queue, tileGridDirty, tile_world_state, tile_world_deps);
        }
    }

    // The conductor tail as a sequence of named units (was one inline block).
    band_patches(c, queue);
    // OIL_1 U8 (ledger: R3 build_patch_grid, C1): the grid reads only the
    // patch registry — no point input — so it rebuilds and re-uploads only
    // when the registry changed. patch_instances_dirty is read HERE, before
    // its consumption-and-clear below, and its raiser set covers every
    // grid-content change (re-verified at edit time): boot/reset
    // (init_patch_system, and the flag boots true), a patch entering the
    // grid set (generation -> GENERATED), and a patch leaving it (eviction
    // compaction). GENERATED -> NEEDS_REGEN keeps both phases inside the
    // set with identical (gx,gz,layer), so the grid bytes cannot move on
    // that transition.
    if (c->world_state_.patch_instances_dirty) build_patch_grid(c, queue);
    // GPU Y-correction is additive (ground_y += terrain), so ground
    // entries must be re-uploaded with offset-only values whenever
    // the heightfield changes. Tie groundEntriesDirty to patch changes.
    c->world_state_.ground_entries_dirty = c->world_state_.ground_entries_dirty || c->world_state_.patch_instances_dirty;
    c->world_state_.placement_dirty = c->world_state_.placement_dirty || c->world_state_.patch_instances_dirty;
    c->world_state_.patch_instances_dirty = false;

    // ─── Entity distance culling ─────────────────────────────
    c->world_state_.entities_culled = update_entity_draw_visibility(c, queue);

    // ─── Deferred uploads (one per frame max) ────────────────
    if (tileGridDirty) upload_tile_grid_now(tile_world_state, &tile_world_deps, queue, c->world_state_.last_center_x, c->world_state_.last_center_z);

    // ─── THE CONSERVATION WITNESS (RIBBON_5) ─────────────────
    //
    // Always-on, O(N), once a second: valid patches + free layers == the pool.
    // A drift here IS the one-patch world — every comer on a recycled layer,
    // one heightfield mutating under a moving player — and it used to say
    // nothing. This is deliberately NOT behind the instruments dial: a broken
    // world must say so in every build, including the shipped one. The
    // [STREAM] line beside it is diagnosis and rides the meter; this is an
    // alarm and rides nothing.
    {
        static uint32_t s_wit = 0;
        if ((s_wit++ % 60u) == 0u) {
            uint32_t valid_count = 0;
            uint32_t ph[4] = {};
            for (uint32_t i = 0; i < c->world_state_.active_patch_count; i++) {
                if (!c->patch_system_state_.patches_[i].valid) continue;
                valid_count++;
                ph[(uint32_t)c->patch_system_state_.patches_[i].phase]++;
            }
            if (valid_count + c->world_state_.free_layer_count != Dim::MAX_ACTIVE_PATCHES) {
                std::fprintf(stderr,
                    "[Patch] CONSERVATION BROKEN — valid=%u + free=%u != %u "
                    "(active=%u). Layers leaked; the world will stop growing.\n",
                    valid_count, c->world_state_.free_layer_count,
                    Dim::MAX_ACTIVE_PATCHES, c->world_state_.active_patch_count);
            }
            if constexpr (t7::INSTRUMENTS.frame_meter) {
                std::fprintf(stderr,
                    "[STREAM] active=%u free=%u | ALLOC=%u SPAWN=%u GEN=%u REGEN=%u | "
                    "young=%d center=(%d,%d) point=(%.0f,%.0f)\n",
                    c->world_state_.active_patch_count, c->world_state_.free_layer_count,
                    ph[(uint32_t)PatchPhase::ALLOCATED], ph[(uint32_t)PatchPhase::SPAWNED],
                    ph[(uint32_t)PatchPhase::GENERATED],
                    ph[(uint32_t)PatchPhase::NEEDS_REGEN],
                    young ? 1 : 0,
                    c->world_state_.last_center_x, c->world_state_.last_center_z,
                    (double)c->point_.x, (double)c->point_.z);
            }
        }
    }

    // Restore radius if we capped it for finite mode
    if (c->world_state_.finite_mode) { c->world_state_.active_radius = savedRadius; }
}

} // namespace the_board
} // namespace t7
