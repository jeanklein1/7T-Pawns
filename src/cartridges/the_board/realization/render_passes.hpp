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
void draw_shadow_all(MachineCtx* c, wgpu::RenderPassEncoder& pass);
void render_main_pass(MachineCtx* c, wgpu::CommandEncoder& encoder,
    wgpu::TextureView backbuffer, wgpu::TextureView depth,
    const float (&clearColor_)[3], OrbsState& orbs_state_, OrbsDeps& orbs_deps_);
// Light matrix helpers (pure math — no MachineCtx)
void compute_spot_light_vp(const GPUSpotLight& light, float* view_proj_out);
void compute_sun_matrices(const float* direction, float* view_proj_out,
    float center_x = 0.0f, float center_z = 0.0f);


// ═══ MODULE IMPLEMENTATION ════════════════════════════════════════
//
// The dispatch/pass bodies + the two pure light-matrix helpers. The
// bodies reach the machine face (c->gpuState_ / c->renderer_ /
// c->entities_state_ / c->patch_system_state_.cpuPiers_ /
// c->world_state_ / c->gol_state_ / c->ribbon_state_ /
// c->gallery_state_ / c->mood_state_) and the call-site extras
// (cpuSpotLights_ / clearColor_ / the orbs pair).


// ═══ PRE-RENDER DATA PREP ════════════════════════════════════════

inline void upload_ground_entries(MachineCtx* c, wgpu::Queue& queue) {
    // ── Arch ground entries ──
    GPUArchGroundEntry archOrigins[Dim::MAX_ARCH_INSTANCES]{};
    for (uint32_t i = 0; i < Dim::MAX_ARCH_INSTANCES; i++) {
        if (!c->entities_state_.arches[i].active) continue;
        const auto& pl = c->patch_system_state_.cpuPiers_[Dim::PIER_ARCH_BASE + i * 2];
        const auto& pr = c->patch_system_state_.cpuPiers_[Dim::PIER_ARCH_BASE + i * 2 + 1];
        archOrigins[i].pier_left_x = pl.origin[0];
        archOrigins[i].pier_left_z = pl.origin[1];
        archOrigins[i].pier_right_x = pr.origin[0];
        archOrigins[i].pier_right_z = pr.origin[1];
        archOrigins[i].is_active = 1;
        archOrigins[i].ground_y = c->entities_state_.arches[i].cached_ground_y;
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
    wgpu::ComputePassEncoder compute = encoder.BeginComputePass(&desc);

    if (c->ribbon_state_.rendered_slot != UINT32_MAX) {
        c->renderer_.dispatch_compute_ribbon_rings(
            compute,
            c->gpuState_.ribbon_compute_group(),
            GPUState::ribbon_ring_workgroups()
        );
    }

    // RAYMARCH/SDF EXCAVATION: dispatch_update_terrain_config removed (dead
    // writer of the TerrainState buffer — no reader).

    c->renderer_.dispatch_update_player_agent(
        compute,
        c->gpuState_.compute_entity_group(),
        c->gpuState_.compute_texture_group()   // aura + sampler for POLICY_WALKER
    );

    c->renderer_.dispatch_update_other_agents(
        compute,
        c->gpuState_.compute_entity_group(),
        c->gpuState_.compute_texture_group()   // aura + sampler for POLICY_WALKER_AGENT
    );

    c->renderer_.dispatch_update_camera(
        compute,
        c->gpuState_.compute_entity_group(),
        c->gpuState_.compute_texture_group()   // aura + sampler for POLICY_FLYER
    );

    c->renderer_.dispatch_update_sphere(
        compute,
        c->gpuState_.compute_entity_group(),
        c->gpuState_.compute_texture_group()   // aura + sampler for POLICY_FLYER
    );

    c->renderer_.dispatch_update_cube(
        compute,
        c->gpuState_.compute_entity_group(),
        c->gpuState_.compute_texture_group()   // aura + sampler for POLICY_FLYER
    );

    c->renderer_.dispatch_compute_vp(
        compute,
        c->gpuState_.compute_entity_group()
    );

    compute.End();
}

inline void dispatch_frustum_cull(MachineCtx* c, wgpu::CommandEncoder& encoder, wgpu::Queue& queue) {
    if (!c->renderer_.use_indirect_terrain()) { return; }   // indoor: skip

    // 1. Reset compute buffer (constant args + zero instanceCount)
    c->gpuState_.reset_frustum_indirect(queue);

    // 2. Compute pass — frustum cull writes atomics + visible indices
    {
        wgpu::ComputePassDescriptor cpd{};
        cpd.label = "Frustum Cull Patches";
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
        5 * sizeof(uint32_t)
    );
}

// ═══ SHADOW PASS ═════════════════════════════════════════════════

inline void render_shadow_pass(MachineCtx* c, wgpu::CommandEncoder& encoder,
    const GPUSpotLightArray& cpuSpotLights_) {
    if (c->mood_state_.spot_light_active && cpuSpotLights_.count > 0) {
        // ─── Two-texture atlas shadow pass (indoor) ──────────
        static constexpr uint32_t TILE_W = Dim::SHADOW_MAP_SIZE / 2;  // 2048
        static constexpr uint32_t TILE_H = Dim::SHADOW_MAP_SIZE;      // 4096

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

            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&desc);

            float vx = static_cast<float>(within * TILE_W);
            pass.SetViewport(vx, 0.0f, static_cast<float>(TILE_W), static_cast<float>(TILE_H), 0.0f, 1.0f);
            pass.SetScissorRect(within * TILE_W, 0, TILE_W, TILE_H);

            draw_shadow_all(c, pass);
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

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&desc);

        draw_shadow_all(c, pass);
        pass.End();
    }
}

// All shadow draws: terrain (FORK) + the drawable table (shadow filter).
// A shadow pass is DEPTH-ONLY, so draw order is doubly immaterial here.
inline void draw_shadow_all(MachineCtx* c, wgpu::RenderPassEncoder& pass) {
    // FORK — terrain: LOD0 direct + a manual LOD1 DrawIndexed (per-pass shape).
    c->renderer_.draw_shadow_patch_terrain(
        pass,
        c->gpuState_.render_entity_group(),
        c->gpuState_.shadow_texture_group(),
        c->gpuState_.patch_index_buffer(),
        c->gpuState_.patch_index_count(),
        c->world_state_.lod0_patch_count
    );
    if (c->world_state_.render_patch_count > c->world_state_.lod0_patch_count) {
        pass.SetIndexBuffer(c->gpuState_.patch_index_buffer_lod1(), wgpu::IndexFormat::Uint32);
        pass.DrawIndexed(c->gpuState_.patch_index_count_lod1(),
            c->world_state_.render_patch_count - c->world_state_.lod0_patch_count, 0, 0, c->world_state_.lod0_patch_count);
    }

    // The drawable table — shadow members, canonical order.
    DrawBind b{ c->gpuState_.render_entity_group(), c->gpuState_.shadow_texture_group(),
                /*shadow=*/true,
                c->ribbon_state_.rendered_slot != UINT32_MAX };
    draw_table(c->renderer_, c->gpuState_, pass, b, DRAW_SHADOW);
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

    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&desc);

    // Terrain LOD0
    if (c->renderer_.use_indirect_terrain()) {
        // Outdoor: GPU-frustum-culled LOD0 via DrawIndexedIndirect
        c->renderer_.draw_patch_terrain_lod0_indirect(
            pass,
            c->gpuState_.render_entity_group(),
            c->gpuState_.render_texture_group(),
            c->gpuState_.patch_index_buffer(),
            c->gpuState_.frustum_indirect_lod0()
        );
    } else {
        // Indoor: direct draw with CPU count
        c->renderer_.draw_patch_terrain_direct(
            pass,
            c->gpuState_.render_entity_group(),
            c->gpuState_.render_texture_group(),
            c->gpuState_.patch_index_buffer(),
            c->gpuState_.patch_index_count(),
            c->world_state_.lod0_patch_count
        );
    }

    // Terrain LOD1 — always direct (Dawn D3D12 limit: only one indirect per pass)
    if (c->world_state_.render_patch_count > c->world_state_.lod0_patch_count) {
        c->renderer_.draw_patch_terrain_direct(
            pass,
            c->gpuState_.render_entity_group(),
            c->gpuState_.render_texture_group(),
            c->gpuState_.patch_index_buffer_lod1(),
            c->gpuState_.patch_index_count_lod1(),
            c->world_state_.render_patch_count - c->world_state_.lod0_patch_count,
            c->world_state_.lod0_patch_count
        );
    }

    // The drawable table — main members, canonical order. All opaque and
    // depth-tested, so order among them is immaterial; this is where the
    // ribbon's ordinal drift dies (it now draws with the entities, not late).
    DrawBind b{ c->gpuState_.render_entity_group(), c->gpuState_.render_texture_group(),
                /*shadow=*/false,
                c->ribbon_state_.rendered_slot != UINT32_MAX };
    draw_table(c->renderer_, c->gpuState_, pass, b, DRAW_MAIN);

    // FORKS — the specials, kept explicit. Wall paintings + gallery frames use
    // their OWN gallery bind groups (opaque). Then the ORDER-SENSITIVE blended
    // pair, LAST and in order: orbs (additive), fade (alpha, no depth write).

    // Wall-mounted framed paintings (indoor)
    c->renderer_.draw_wall_paintings(
        pass,
        c->gpuState_.gallery_entity_group(),
        c->gpuState_.gallery_texture_group(),
        c->gallery_state_.wall_frame_count
    );

    // Gallery frames (self-portrait paintings on terrain)
    c->renderer_.draw_gallery_frames(
        pass,
        c->gpuState_.gallery_entity_group(),
        c->gpuState_.gallery_texture_group(),
        c->gallery_state_.active_painting_count
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


inline void compute_sun_matrices(const float* direction, float* view_proj_out,
    float center_x, float center_z) {   // defaults live on the header declaration
    //
    const float altitude = 300.0f;
    float light_pos[3] = {
        center_x - direction[0] * altitude,
        -direction[1] * altitude,
        center_z - direction[2] * altitude
    };

    // Up vector (choose one that's not parallel to direction)
    float up[3];
    if (std::abs(direction[1]) > 0.99f) {
        up[0] = 0.0f; up[1] = 0.0f; up[2] = 1.0f;
    }
    else {
        up[0] = 0.0f; up[1] = 1.0f; up[2] = 0.0f;
    }

    // View matrix (look-at from light_pos toward origin)
    float fwd[3] = { direction[0], direction[1], direction[2] };

    // right = normalize(cross(fwd, up))
    float right[3] = {
        fwd[1] * up[2] - fwd[2] * up[1],
        fwd[2] * up[0] - fwd[0] * up[2],
        fwd[0] * up[1] - fwd[1] * up[0]
    };
    float rlen = std::sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
    right[0] /= rlen; right[1] /= rlen; right[2] /= rlen;

    // true_up = cross(right, fwd) — note: not cross(fwd, right)
    float true_up[3] = {
        right[1] * fwd[2] - right[2] * fwd[1],
        right[2] * fwd[0] - right[0] * fwd[2],
        right[0] * fwd[1] - right[1] * fwd[0]
    };

    float tx = -(right[0] * light_pos[0] + right[1] * light_pos[1] + right[2] * light_pos[2]);
    float ty = -(true_up[0] * light_pos[0] + true_up[1] * light_pos[1] + true_up[2] * light_pos[2]);
    float tz = fwd[0] * light_pos[0] + fwd[1] * light_pos[1] + fwd[2] * light_pos[2];

    // Column-major view matrix
    float view[16] = {
        right[0],    right[1],    right[2],    0.0f,
        true_up[0],  true_up[1],  true_up[2],  0.0f,
        -fwd[0],     -fwd[1],     -fwd[2],     0.0f,
        tx,          ty,           tz,          1.0f
    };

    const float half_extent = 350.0f;
    const float near_plane = 0.1f;
    const float far_plane = 800.0f;

    float proj[16] = {
        1.0f / half_extent, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f / half_extent, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f / (far_plane - near_plane), 0.0f,
        0.0f, 0.0f, -near_plane / (far_plane - near_plane), 1.0f
    };

    // Multiply proj * view (column-major)
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
