#pragma once
#include "cartridges/the_board/realization/state.hpp"   // wgpu, GPUSpotLight (the light-VP helper's parameter)
#include "cartridges/the_board/contracts/wgpu_fwd.hpp"   // wgpu handle fwds (lockstep insurance)
#include <algorithm>   // std::max, std::min   // (impl, merged)
#include <cmath>       // std::sqrt, std::abs, std::acos, std::tan   // (impl, merged)
#include <cstdint>   // (impl, merged)

// ─── render_passes.hpp (MERGED: decls + impl) ──────────────────────
// History: audit/LADDER.md
//
// GPU dispatch and draw calls.
//
// THE MACHINE FACE (the B ruling): the realization conductor stands on MachineCtx — its nine
// organ reaches are all machine members, byte-identical through the
// face. The three reaches OUTSIDE the face ride the call site (the B
// law): render_shadow_pass takes the CPU spot-light array (const
// read), render_main_pass takes the clear color (const read) + the
// orbs pair (render_orbs — the one sibling door). The module owns no
// state; the two light-matrix helpers are pure math. COHORT: merged
// at the tail after orbs/ribbon/gallery/input (render_orbs def +
// complete organs), BEFORE merged mood (mood's spot-light applier
// calls compute_spot_light_vp).
// ─────────────────────────────────────────────────────────────────

namespace t7 {
namespace the_board {

// fwd — the machine face + the main pass's orbs pair (reference
// params; complete types precede this file in the cohort).
struct MachineCtx;
struct OrbsState; struct OrbsDeps;

// ═══ MODULE FUNCTIONS — DECLARATIONS ═════════════════════════════

// Pre-render data preparation
void upload_ground_entries(MachineCtx* c, wgpu::Queue& queue);
void dispatch_placement_correction(MachineCtx* c, wgpu::CommandEncoder& encoder);
// GPU compute dispatch
void dispatch_compute(MachineCtx* c, wgpu::CommandEncoder& encoder);
void dispatch_frustum_cull(MachineCtx* c, wgpu::CommandEncoder& encoder, wgpu::Queue& queue);
// Render passes (the extras outside the machine face ride the call site)
void render_shadow_pass(MachineCtx* c, wgpu::CommandEncoder& encoder,
    const GPUSpotLightArray& cpuSpotLights_);
void draw_shadow_all(MachineCtx* c, wgpu::RenderPassEncoder& pass, bool cast_terrain);
void render_main_pass(MachineCtx* c, wgpu::CommandEncoder& encoder,
    wgpu::TextureView backbuffer, wgpu::TextureView depth,
    const float (&clearColor_)[3], OrbsState& orbs_state_, OrbsDeps& orbs_deps_);
// Light matrix helpers (pure math — no MachineCtx)
void compute_spot_light_vp(const GPUSpotLight& light, float* view_proj_out);


// ═══ MODULE IMPLEMENTATION ════════════════════════════════════════
//
// The dispatch/pass bodies + the two pure light-matrix helpers. The
// bodies reach the machine face (c->gpuState_ / c->renderer_ /
// c->entities_state_ /
// c->world_state_ / c->gol_state_ / c->ribbon_state_ /
// c->gallery_state_ / c->mood_state_) and the call-site extras
// (cpuSpotLights_ / clearColor_ / the orbs pair).


// ═══ PRE-RENDER DATA PREP ════════════════════════════════════════

inline void upload_ground_entries(MachineCtx* c, wgpu::Queue& queue) {
    // ── Arch ground entries ──
    // Leg positions from the arch's OWN geometry (world_x/z ± half_span
    // rotated) — the identical values the retired pier writer computed;
    // the pier mirror died in BATCH G and the arch was always the source.
    GPUArchGroundEntry archOrigins[Dim::MAX_ARCH_INSTANCES]{};
    for (uint32_t i = 0; i < Dim::MAX_ARCH_INSTANCES; i++) {
        const auto& ar = c->entities_state_.arches[i];
        if (!ar.active) continue;
        float cr = std::cos(ar.rotation), sr = std::sin(ar.rotation);
        archOrigins[i].pier_left_x = ar.world_x - ar.half_span * cr;
        archOrigins[i].pier_left_z = ar.world_z - ar.half_span * sr;
        archOrigins[i].pier_right_x = ar.world_x + ar.half_span * cr;
        archOrigins[i].pier_right_z = ar.world_z + ar.half_span * sr;
        archOrigins[i].is_active = 1;
        archOrigins[i].ground_y = ar.cached_ground_y;
        archOrigins[i].pier_correction_left = 0.0f;
        archOrigins[i].pier_correction_right = 0.0f;
    }
    c->gpuState_.upload_arch_origins(queue, archOrigins, Dim::MAX_ARCH_INSTANCES);

    // ── Column + Antenna ground entries (shared GPU buffer, split arrays) ──
    GPUColumnGroundEntry columnOrigins[Dim::MAX_COLUMN_INSTANCES]{};
    for (uint32_t i = 0; i < Dim::MAX_COLUMN_ONLY; i++) {
        if (!c->entities_state_.columns[i].active) continue;
        columnOrigins[i].center_x = c->entities_state_.columns[i].world_x;
        columnOrigins[i].center_z = c->entities_state_.columns[i].world_z;
        columnOrigins[i].is_active = 1;
        columnOrigins[i].ground_y = c->entities_state_.columns[i].cached_ground_y;
        columnOrigins[i].pier_correction = 0.0f;
    }
    for (uint32_t i = 0; i < Dim::MAX_ANTENNA_ONLY; i++) {
        if (!c->entities_state_.antennas[i].active) continue;
        uint32_t gpu_slot = i + Dim::ANTENNA_SLOT_OFFSET;
        columnOrigins[gpu_slot].center_x = c->entities_state_.antennas[i].world_x;
        columnOrigins[gpu_slot].center_z = c->entities_state_.antennas[i].world_z;
        columnOrigins[gpu_slot].is_active = 1;
        columnOrigins[gpu_slot].ground_y = c->entities_state_.antennas[i].cached_ground_y;
        columnOrigins[gpu_slot].pier_correction = 0.0f;
    }
    c->gpuState_.upload_column_origins(queue, columnOrigins, Dim::MAX_COLUMN_INSTANCES);

    // ── Plant ground entries (palm + cactus + blade) ──
    // Combined compute buffer: [0..23] palm, [24..43] cactus, [44..75] blade.
    // Individual render uniform buffers kept for VS bindings (383, 384, 385).
    static constexpr uint32_t PALM_OFF = 0;
    static constexpr uint32_t CACT_OFF = Dim::MAX_PALM_INSTANCES;
    static constexpr uint32_t BLAD_OFF = Dim::MAX_PALM_INSTANCES + Dim::MAX_CACTUS_INSTANCES;
    static constexpr uint32_t PLANT_COUNT = BLAD_OFF + Dim::MAX_BLADE_INSTANCES;

    GPUPalmGroundEntry plantOrigins[PLANT_COUNT]{};

    for (uint32_t i = 0; i < Dim::MAX_PALM_INSTANCES; i++) {
        if (!c->entities_state_.palms[i].active) continue;
        plantOrigins[PALM_OFF + i].center_x = c->entities_state_.palms[i].world_x;
        plantOrigins[PALM_OFF + i].center_z = c->entities_state_.palms[i].world_z;
        plantOrigins[PALM_OFF + i].is_active = 1;
        plantOrigins[PALM_OFF + i].ground_y = c->entities_state_.palms[i].cached_ground_y;
    }
    for (uint32_t i = 0; i < Dim::MAX_CACTUS_INSTANCES; i++) {
        if (!c->entities_state_.cacti[i].active) continue;
        plantOrigins[CACT_OFF + i].center_x = c->entities_state_.cacti[i].world_x;
        plantOrigins[CACT_OFF + i].center_z = c->entities_state_.cacti[i].world_z;
        plantOrigins[CACT_OFF + i].is_active = 1;
        plantOrigins[CACT_OFF + i].ground_y = c->entities_state_.cacti[i].cached_ground_y;
    }
    for (uint32_t i = 0; i < Dim::MAX_BLADE_INSTANCES; i++) {
        if (!c->entities_state_.blades[i].active) continue;
        plantOrigins[BLAD_OFF + i].center_x = c->entities_state_.blades[i].world_x;
        plantOrigins[BLAD_OFF + i].center_z = c->entities_state_.blades[i].world_z;
        plantOrigins[BLAD_OFF + i].is_active = 1;
        plantOrigins[BLAD_OFF + i].ground_y = c->entities_state_.blades[i].cached_ground_y;
    }

    // One write to the combined compute storage buffer
    queue.WriteBuffer(c->gpuState_.plant_compute_ground_buffer(), 0,
        plantOrigins, sizeof(plantOrigins));
}

inline void dispatch_placement_correction(MachineCtx* c, wgpu::CommandEncoder& encoder) {
    wgpu::ComputePassDescriptor cpd{};
    cpd.label = "Entity Placement Y Correction";
    cpd.timestampWrites = c->gpuState_.meter_arm_compute(meter_row::PlacementCorrection);
    wgpu::ComputePassEncoder compute = encoder.BeginComputePass(&cpd);
    // Group 1: compute textures — the card's cell-exact GoL fetch (H5).
    // Bound before the dispatch inside; bind-group state is sticky.
    compute.SetBindGroup(1, c->gpuState_.compute_texture_group());
    c->renderer_.dispatch_entity_placement(
        compute, c->gpuState_.entity_placement_compute_group()
    );
    compute.End();
}

// The live card write (GROUND_CARD_1) — its own pass, before the
// consumers (dispatch_compute) and before placement reads .a (H5).
inline void dispatch_live_card_write(MachineCtx* c, wgpu::CommandEncoder& encoder) {
    wgpu::ComputePassDescriptor cpd{};
    cpd.label = "Live Card Write";
    cpd.timestampWrites = c->gpuState_.meter_arm_compute(meter_row::LiveCardWrite);
    wgpu::ComputePassEncoder compute = encoder.BeginComputePass(&cpd);
    c->renderer_.dispatch_live_card_write(
        compute, c->gpuState_.live_card_writer_group()
    );
    compute.End();
}

// ═══ GPU COMPUTE DISPATCH ════════════════════════════════════════

// Per-frame compute: ribbon transforms, agents, camera, VP.
inline void dispatch_compute(MachineCtx* c, wgpu::CommandEncoder& encoder) {
    wgpu::ComputePassDescriptor desc{};
    desc.label = "Compute Phase";
    desc.timestampWrites = c->gpuState_.meter_arm_compute(meter_row::DispatchCompute);
    wgpu::ComputePassEncoder compute = encoder.BeginComputePass(&desc);

    // The ribbon rings run FIRST and on their OWN group0 (the ribbon
    // compute group) — outside the pass-head contract below, which is
    // why it stays ahead of those binds.
    if (c->ribbon_state_.rendered_slot != UINT32_MAX) {
        c->renderer_.dispatch_compute_ribbon_rings(
            compute,
            c->gpuState_.ribbon_compute_group(),
            GPUState::ribbon_ring_workgroups()
        );
    }

    // OIL_1 U11 (ledger: R10, C7) — THE PASS-HEAD BINDS. The six kernels
    // below all took the SAME three groups and each re-bound them: ~12
    // redundant SetBindGroup calls of unchanged state per frame. Bound
    // once here; bind-group state is sticky for the rest of the pass
    // (the codebase's own precedent is declared at the shadow band-1
    // site). Group 2 is bound unconditionally — camera and VP do not
    // read it, and a bound group their layout ignores is legal.
    compute.SetBindGroup(0, c->gpuState_.compute_entity_group());
    compute.SetBindGroup(1, c->gpuState_.compute_texture_group());  // aura + sampler (POLICY_WALKER / POLICY_FLYER)
    compute.SetBindGroup(2, c->gpuState_.room_group());             // the room: occupier windows + field (FIELD_2)

    c->renderer_.dispatch_update_player_agent(compute);
    c->renderer_.dispatch_update_other_agents(compute);
    c->renderer_.dispatch_update_camera(compute);
    c->renderer_.dispatch_update_sphere(compute);
    c->renderer_.dispatch_update_cube(compute);
    c->renderer_.dispatch_compute_vp(compute);

    compute.End();
}

inline void dispatch_frustum_cull(MachineCtx* c, wgpu::CommandEncoder& encoder, wgpu::Queue& queue) {
    // THE DRAW PLAN: the kernel runs in EVERY mood now — the finite/
    // indoor path draws through the plan too, so the old indoor skip
    // is retired with the direct path it fed.

    // 0. The plan's inputs: band counts + the active zone rects
    // (world footprints persisted at commit_gol — E1 rev2's four
    // floats), packed dense. Uploaded beside the frustum reset.
    {
        GPUDrawPlanParams plan{};
        plan.lod0_count   = c->world_state_.lod0_patch_count;
        plan.render_count = c->world_state_.render_patch_count;
        uint32_t n = 0;
        for (uint32_t i = 0; i < Dim::MAX_GOL_ZONES && n < 8; i++) {
            const auto& z = c->gol_state_.zones[i];
            if (!z.active) continue;
            plan.rects[n][0] = z.corner_x;
            plan.rects[n][1] = z.corner_z;
            plan.rects[n][2] = z.extent_x;
            plan.rects[n][3] = z.extent_z;
            n++;
        }
        plan.rect_count = n;
        c->gpuState_.upload_draw_plan(queue, plan);
    }

    // 1. Reset compute buffer (constant args + zero instanceCounts)
    c->gpuState_.reset_frustum_indirect(queue);

    // 2. Compute pass — frustum cull writes atomics + visible indices
    {
        wgpu::ComputePassDescriptor cpd{};
        cpd.label = "Frustum Cull Patches";
        cpd.timestampWrites = c->gpuState_.meter_arm_compute(meter_row::FrustumCull);
        wgpu::ComputePassEncoder compute = encoder.BeginComputePass(&cpd);
        c->renderer_.dispatch_frustum_cull(
            compute, c->gpuState_.frustum_cull_group()
        );
        compute.End();
    }

    // 3. Copy compute buffer → indirect buffer (Dawn D3D12 can't share Storage|Indirect)
    encoder.CopyBufferToBuffer(
        c->gpuState_.frustum_compute_buffer(), 0,
        c->gpuState_.frustum_indirect_lod0(), 0,
        FC_ARGS_BYTES
    );
}

// ═══ SHADOW PASS ═════════════════════════════════════════════════

inline void render_shadow_pass(MachineCtx* c, wgpu::CommandEncoder& encoder,
    const GPUSpotLightArray& cpuSpotLights_) {
    if (c->mood_state_.spot_light_active && cpuSpotLights_.count > 0) {
        // ─── Two-texture atlas shadow pass (indoor) ──────────
        static constexpr uint32_t TILE_W = Dim::SHADOW_MAP_SIZE / 2;  // half width
        static constexpr uint32_t TILE_H = Dim::SHADOW_MAP_SIZE;      // full height

        for (uint32_t li = 0; li < cpuSpotLights_.count && li < MAX_SPOT_LIGHTS; li++) {
            // Copy this light's VP from staging → VP buffer's light_vp slot
            encoder.CopyBufferToBuffer(
                c->gpuState_.spot_vp_staging(), li * 64,
                c->gpuState_.vp_buffer(), GPUState::light_vp_offset(),
                GPUState::light_vp_size());

            // Lights 0-1 → sun map (idle in indoor mode), lights 2-3 → spot map
            bool use_sun_map = (li < 2);
            uint32_t within = li % 2;   // 0=left half, 1=right half

            wgpu::RenderPassDepthStencilAttachment depthAttachment{};
            depthAttachment.view = use_sun_map
                ? c->gpuState_.shadow_map_view()
                : c->gpuState_.spot_shadow_map_view();
            depthAttachment.depthLoadOp = (within == 0) ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load;
            depthAttachment.depthStoreOp = wgpu::StoreOp::Store;
            depthAttachment.depthClearValue = 1.0f;

            wgpu::RenderPassDescriptor desc{};
            desc.label = "Shadow Atlas Tile";
            desc.colorAttachmentCount = 0;
            desc.depthStencilAttachment = &depthAttachment;
            desc.timestampWrites = c->gpuState_.meter_arm_render(meter_row::ShadowPass);

            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&desc);

            // OIL_1 U12 (ledger: R18, C7) — the pass-head binds. Each
            // atlas tile is a FRESH pass, so this is the tile's one real
            // bind; the draws that follow re-set nothing.
            pass.SetBindGroup(0, c->gpuState_.render_entity_group());
            pass.SetBindGroup(1, c->gpuState_.shadow_texture_group());

            float vx = static_cast<float>(within * TILE_W);
            pass.SetViewport(vx, 0.0f, static_cast<float>(TILE_W), static_cast<float>(TILE_H), 0.0f, 1.0f);
            pass.SetScissorRect(within * TILE_W, 0, TILE_W, TILE_H);

            // THE SPOT CASTER CUT (UMBRA_4) — terrain does not cast into a
            // spot tile. This is the whole of the edit: one argument, one
            // site, and the revert is this word.
            draw_shadow_all(c, pass, /*cast_terrain=*/false);
            pass.End();
        }
    }
    else {
        // ─── Standard shadow pass (outdoor) ──────────────────
        wgpu::RenderPassDepthStencilAttachment depthAttachment{};
        depthAttachment.view = c->gpuState_.shadow_map_view();
        depthAttachment.depthLoadOp = wgpu::LoadOp::Clear;
        depthAttachment.depthStoreOp = wgpu::StoreOp::Store;
        depthAttachment.depthClearValue = 1.0f;

        wgpu::RenderPassDescriptor desc{};
        desc.label = "Shadow Pass";
        desc.colorAttachmentCount = 0;
        desc.depthStencilAttachment = &depthAttachment;
        desc.timestampWrites = c->gpuState_.meter_arm_render(meter_row::ShadowPass);

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&desc);

        // OIL_1 U12 — the pass-head binds (see the atlas arm above).
        pass.SetBindGroup(0, c->gpuState_.render_entity_group());
        pass.SetBindGroup(1, c->gpuState_.shadow_texture_group());

        draw_shadow_all(c, pass, /*cast_terrain=*/true);
        pass.End();
    }
}

// All shadow draws: terrain (FORK) + the drawable table (shadow filter).
// A shadow pass is DEPTH-ONLY, so draw order is doubly immaterial here.
//
// cast_terrain — true for the sun pass, false for every spot atlas tile
// (UMBRA_4). An indoor spot sits under a shell with a cone that never
// reaches the horizon, and the pass ran ONCE PER LIGHT: up to
// MAX_SPOT_LIGHTS full terrain redraws per frame, inside the known
// bottleneck, to shadow a surface the cone cannot light. No light-volume
// bounding mechanism is built to decide this — no mechanism until a
// measurement asks. The drawable table is untouched: the indoor scene's
// actual occluders all still cast.
inline void draw_shadow_all(MachineCtx* c, wgpu::RenderPassEncoder& pass, bool cast_terrain) {
    // FORK — terrain, both bands at LOD1 density (ECONOMY_1 E2): the
    // shadow target resolves coarser than even the half mesh, and the
    // decode is patch-agnostic. Both bands take the ring IB at the LIVE
    // LOD1 count (OPT_1e): prefix + curtain tail while any zone is
    // active anywhere — the tail is the slab WALLS, the only thing
    // connecting a lifted cell's shadow to its base (the clean-prefix
    // cut detached them and the caps' shadows floated free) — and the
    // clean prefix at true rest, when no cell anywhere can lift.
    //
    // THE CASTER LOD PIN (UMBRA_3, ruled here rather than re-cut). The
    // ladder is two rungs, both terrain-mesh densities of a 50 wu patch:
    // LOD0 at PATCH_MESH_N = 64 (0.781 wu per quad edge) and the ring's
    // stride-2 cap lattice at 1.5625 wu (CELL_1 rev2). Against a
    // post-UMBRA_5 texel of 0.2051 wu, LOD0 is 3.8 texels per edge —
    // finer than the map can resolve, so pure cost — and the ring is
    // 7.6, already coarser than the target. Neither rung satisfies
    // "edge <= 2 x texelWorld", so that rule selects nothing; the pin is
    // nonetheless already at the ladder's COARSEST rung and there is
    // nothing coarser to move to.
    //
    // SCOPE THAT CLAIM CAREFULLY — it is about DENSITY, not about the SET.
    // Nothing here selects a mesh density by distance: both bands take the
    // LOD1 buffer unconditionally, so a caster's silhouette never
    // re-tessellates as the camera nears it. That is what acquits the
    // second of the two suspects the campaign named for the "shadows
    // compose as we approach" artifact.
    //
    // The instance COUNTS below are a different matter and do move with
    // the eye: band_patches (surface/patch_system.hpp) partitions patches
    // against lod0_radius and the veil ring measured from THE POINT, and
    // in camera-host mode the point IS the eye. So WHICH patches cast
    // tracks the viewer even though HOW FINELY they cast does not.
    //
    // That set boundary is the veil ring (325 wu), and post-UMBRA_5 the
    // sun frustum's half-extent is 420 wu — so the cast set now ends
    // strictly INSIDE the shadow map's coverage, where nothing is drawn to
    // receive anyway (the ring is the draw authority). Before UMBRA_5 the
    // radius was 300 and the relation was inverted: the shadow map ran out
    // 25 wu BEFORE the drawn world did, and that visible edge is what the
    // campaign was chasing. It is now structurally gone, not merely
    // pushed.
    if (cast_terrain) {
        c->renderer_.draw_shadow_patch_terrain(
            pass,
            c->gpuState_.patch_index_buffer_lod1(),
            c->gpuState_.patch_index_count_lod1_live(),
            c->world_state_.lod0_patch_count
        );
        if (c->world_state_.render_patch_count > c->world_state_.lod0_patch_count) {
            // Band 1 — same IB, already bound by the band-0 helper; the
            // redundant re-bind collapsed (trivially adjacent).
            pass.DrawIndexed(c->gpuState_.patch_index_count_lod1_live(),
                c->world_state_.render_patch_count - c->world_state_.lod0_patch_count, 0, 0, c->world_state_.lod0_patch_count);
        }
    }

    // The drawable table — shadow members, canonical order.
    DrawBind b{ /*shadow=*/true,
                c->ribbon_state_.rendered_slot != UINT32_MAX };
    draw_table(c->renderer_, c->gpuState_, pass, b, DRAW_SHADOW);

    // FORKS — the artworks, on their OWN gallery bind groups, as in the
    // main pass. UNCONDITIONAL: no cast_terrain-style argument and no mood
    // test. The mood selects which LIGHT this pass runs for; it does not
    // select what exists. Each form self-culls in its VS, so outdoors the
    // wall draw is near-degenerate and indoors the quad draw is.
    // OIL_1 U12: the gallery pair, bound ONCE for both draws (they
    // share galleryShadowLayout, and nothing draws after them in this
    // pass, so the pass-head pair above is not needed again).
    // ROSTER-GATE gallery (a') — the gate MATCHES its consumers: both
    // draws below open with the same if constexpr, so a gallery-less
    // build must not pay two binds for zero draws, and the pass must
    // not name groups a future creation-side gate could leave null.
    if constexpr (ROSTER.gallery) {
    pass.SetBindGroup(0, c->gpuState_.gallery_entity_group());
    pass.SetBindGroup(1, c->gpuState_.gallery_texture_group());
    }
    c->renderer_.draw_shadow_wall_paintings(
        pass,
        c->gallery_state_.wall_frame_count,
        c->gallery_state_.slot_high_water
    );
    c->renderer_.draw_shadow_gallery_frames(
        pass,
        c->gallery_state_.active_painting_count,
        c->gallery_state_.slot_high_water
    );
}

// ═══ MAIN PASS ═══════════════════════════════════════════════════

inline void render_main_pass(MachineCtx* c, wgpu::CommandEncoder& encoder,
    wgpu::TextureView backbuffer, wgpu::TextureView depth,
    const float (&clearColor_)[3], OrbsState& orbs_state_, OrbsDeps& orbs_deps_) {

    wgpu::RenderPassColorAttachment colorAttachment{};
    colorAttachment.view = backbuffer;
    colorAttachment.loadOp = wgpu::LoadOp::Clear;
    colorAttachment.storeOp = wgpu::StoreOp::Store;
    colorAttachment.clearValue = { (double)clearColor_[0], (double)clearColor_[1], (double)clearColor_[2], 1.0 };

    wgpu::RenderPassDepthStencilAttachment depthAttachment{};
    depthAttachment.view = depth;
    depthAttachment.depthLoadOp = wgpu::LoadOp::Clear;
    depthAttachment.depthStoreOp = wgpu::StoreOp::Store;
    depthAttachment.depthClearValue = 1.0f;

    wgpu::RenderPassDescriptor desc{};
    desc.label = "Rasterized Scene";
    desc.colorAttachmentCount = 1;
    desc.colorAttachments = &colorAttachment;
    desc.depthStencilAttachment = &depthAttachment;
    desc.timestampWrites = c->gpuState_.meter_arm_render(meter_row::MainPass);

    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&desc);

    // OIL_1 U13 (ledger: R19, C7) — THE PASS-HEAD BIND. group1 (the
    // render texture group) is the same for every entity draw in this
    // pass, plan slots included, so it is bound once here and never
    // again. group0 is NOT bound here: it is genuinely per-slot (the
    // plan A/B/C windows are three different groups, each bound by its
    // own slot) and is restored to the entity group after slot C, once,
    // for the table draws.
    pass.SetBindGroup(1, c->gpuState_.render_texture_group());

    // Terrain — THE DRAW PLAN (ECONOMY_1 closing arm): the cull kernel
    // authored three lists; the pass executes them as three indirect
    // draws. Outdoor AND finite/indoor go through the same plan (the
    // kernel sees all bands everywhere). The E1 global-flag selection
    // is RETIRED here — the plan is per-patch; the flag survives only
    // for the snapshot pass (R6), which culls against the
    // photographer's frustum and cannot read this plan.
    c->renderer_.begin_patch_terrain_plan(pass);   // OIL_1 U13: one SetPipeline for the three slots
    c->renderer_.draw_patch_terrain_plan_slot(pass,
        c->gpuState_.render_entity_group(),          // plan A window
        c->gpuState_.patch_index_buffer(),           // full IB (zone-overlapped)
        c->gpuState_.frustum_indirect_lod0(), 0);
    c->renderer_.draw_patch_terrain_plan_slot(pass,
        c->gpuState_.render_entity_group_plan_b(),   // plan B window
        c->gpuState_.patch_index_buffer_cap_only(),  // cap-only IB (clean LOD0)
        c->gpuState_.frustum_indirect_lod0(), 20);
    c->renderer_.draw_patch_terrain_plan_slot(pass,
        c->gpuState_.render_entity_group_plan_c(),   // plan C window
        c->gpuState_.patch_index_buffer_lod1(),      // LOD1 IB (culled at last)
        c->gpuState_.frustum_indirect_lod0(), 40);
    // Plan C left its window bound: restore the entity group the table
    // draws read (group1 is untouched by the slots).
    pass.SetBindGroup(0, c->gpuState_.render_entity_group());

    // The drawable table — main members, canonical order. All opaque and
    // depth-tested, so order among them is immaterial; this is where the
    // ribbon's ordinal drift dies (it now draws with the entities, not late).
    DrawBind b{ /*shadow=*/false,
                c->ribbon_state_.rendered_slot != UINT32_MAX };
    draw_table(c->renderer_, c->gpuState_, pass, b, DRAW_MAIN);

    // FORKS — the specials, kept explicit. Wall paintings + gallery frames use
    // their OWN gallery bind groups (opaque). Then the ORDER-SENSITIVE blended
    // pair, LAST and in order: orbs (additive), fade (alpha, no depth write).

    // Wall-mounted framed paintings (indoor)
    // OIL_1 U13: the gallery pair, bound ONCE for both draws.
    // ROSTER-GATE gallery (a') — matches the consumers' own gate.
    if constexpr (ROSTER.gallery) {
    pass.SetBindGroup(0, c->gpuState_.gallery_entity_group());
    pass.SetBindGroup(1, c->gpuState_.gallery_texture_group());
    }
    c->renderer_.draw_wall_paintings(
        pass,
        c->gallery_state_.wall_frame_count,
        c->gallery_state_.slot_high_water
    );

    // Gallery frames (self-portrait paintings on terrain)
    c->renderer_.draw_gallery_frames(
        pass,
        c->gallery_state_.active_painting_count,
        c->gallery_state_.slot_high_water
    );

    render_orbs(orbs_state_, &orbs_deps_, pass);

    // Fade overlay (drawn last, alpha blended over everything)
    c->renderer_.draw_fade_overlay(
        pass,
        c->gpuState_.mesh_gen_entity_group(),
        c->mood_state_.transition_fade_alpha
    );

    pass.End();
}

// ═══ LIGHT MATRIX COMPUTATION ════════════════════════════════════

inline void compute_spot_light_vp(const GPUSpotLight& light, float* view_proj_out) {
    const float* pos = light.position;
    float ld[3] = { light.direction[0], light.direction[1], light.direction[2] };
    float dlen = std::sqrt(ld[0] * ld[0] + ld[1] * ld[1] + ld[2] * ld[2]);
    ld[0] /= dlen; ld[1] /= dlen; ld[2] /= dlen;

    // Choose an up vector not parallel to the light direction
    float light_up[3];
    if (std::abs(ld[1]) > 0.99f) {
        light_up[0] = 0.0f; light_up[1] = 0.0f; light_up[2] = 1.0f;
    }
    else {
        light_up[0] = 0.0f; light_up[1] = 1.0f; light_up[2] = 0.0f;
    }

    // View matrix (look-at from light position along direction)
    float right[3] = {
        light_up[1] * ld[2] - light_up[2] * ld[1],
        light_up[2] * ld[0] - light_up[0] * ld[2],
        light_up[0] * ld[1] - light_up[1] * ld[0]
    };
    float rlen = std::sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
    right[0] /= rlen; right[1] /= rlen; right[2] /= rlen;

    float up[3] = {
        ld[1] * right[2] - ld[2] * right[1],
        ld[2] * right[0] - ld[0] * right[2],
        ld[0] * right[1] - ld[1] * right[0]
    };

    float tx = -(right[0] * pos[0] + right[1] * pos[1] + right[2] * pos[2]);
    float ty = -(up[0] * pos[0] + up[1] * pos[1] + up[2] * pos[2]);
    float tz = (ld[0] * pos[0] + ld[1] * pos[1] + ld[2] * pos[2]);

    float view[16] = {
        right[0], up[0], -ld[0], 0.0f,
        right[1], up[1], -ld[1], 0.0f,
        right[2], up[2], -ld[2], 0.0f,
        tx, ty, tz, 1.0f
    };

    const float outer_half = std::acos(std::max(light.outer_cone, -0.95f));
    const float fov = std::min(2.0f * outer_half + 0.2f, 2.8f);
    const float near_plane = 1.0f;
    const float far_plane = light.range + 5.0f;
    float f = 1.0f / std::tan(fov * 0.5f);
    float nf = 1.0f / (near_plane - far_plane);

    float proj[16] = {
        f, 0.0f, 0.0f, 0.0f,
        0.0f, f, 0.0f, 0.0f,
        0.0f, 0.0f, far_plane * nf, -1.0f,
        0.0f, 0.0f, far_plane * near_plane * nf, 0.0f
    };

    // proj * view (column-major)
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += proj[k * 4 + row] * view[col * 4 + k];
            }
            view_proj_out[col * 4 + row] = sum;
        }
    }
}



} // namespace the_board
} // namespace t7
