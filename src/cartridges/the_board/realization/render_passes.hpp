#pragma once
#include "core/boot_params.hpp"   // IOS_3 C — the four switches
#include "cartridges/the_board/realization/state.hpp"   // wgpu, GPUSpotLight (the light-VP helper's parameter)
#include "cartridges/the_board/contracts/wgpu_fwd.hpp"   // wgpu handle fwds (lockstep insurance)
#include <algorithm>   // std::max, std::min   // (impl, merged)
#include <cmath>       // std::sqrt, std::abs, std::acos, std::tan   // (impl, merged)
#include <cstdint>   // (impl, merged)

// ─── render_passes.hpp (MERGED: decls + impl) ──────────────────────
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
void stage_draw_ledger(MachineCtx* c, OrbsState& orbs_state_);
void record_bundles(MachineCtx* c, OrbsState& orbs_state_, OrbsDeps& orbs_deps_);
void upload_ground_entries(MachineCtx* c, wgpu::Queue& queue);
void dispatch_placement_correction(MachineCtx* c, wgpu::CommandEncoder& encoder);
// GPU compute dispatch
void dispatch_compute(MachineCtx* c, wgpu::CommandEncoder& encoder);
void dispatch_frustum_cull(MachineCtx* c, wgpu::CommandEncoder& encoder, wgpu::Queue& queue);
// Render passes (the extras outside the machine face ride the call site)
void render_shadow_pass(MachineCtx* c, wgpu::CommandEncoder& encoder,
    const GPUSpotLightArray& cpuSpotLights_);
template <class Enc>
void draw_shadow_all(MachineCtx* c, Enc& pass, bool cast_terrain);
void render_main_pass(MachineCtx* c, wgpu::CommandEncoder& encoder,
    wgpu::TextureView backbuffer, wgpu::TextureView msaaColor,
    wgpu::TextureView depth,
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
    // LOOM_2 pass head: WORLD + FRAME are every pipeline's strata 0/1.
    { compute.SetBindGroup(0, c->gpuState_.world_group());
      compute.SetBindGroup(1, c->gpuState_.frame_c_group()); }
    c->renderer_.dispatch_entity_placement(
        compute, c->gpuState_.place_state_group(), c->gpuState_.place_textures_group()
    );
    compute.End();
}

// ═══ THE DRAW LEDGER'S STAGE (BUNDLE_1) ══════════════════════════
//
// ONE SITE that reads every count the CPU authors and stages it. It runs at
// the frame boundary — after the update spine, so every count is this
// frame's, and before the encoder, so the WriteBuffer lands ahead of the
// command buffer on the queue timeline (the ordering upload_patch_params
// relies on).
//
// WHY ONE SITE AND NOT TWELVE. Each of these numbers already has a home —
// the family setters, gallery_state_, os.count. What did NOT have a home is
// the set of GUARDS: `if (indexCount == 0) return;`, `if (!os.active)
// return;`, `rendered_slot != UINT32_MAX`. Those lived in the draw verbs as
// encoder-time skips, and an encoder-time skip cannot be recorded into a
// bundle — a bundle taken in a frame where a family was empty would omit it
// forever, and every family is empty at boot. A record of zeros draws
// nothing, so each guard becomes its record's number, and this is where
// "what will be drawn this frame" is said once and read once.
//
// flush_draw_ledger writes only what moved, so a steady frame writes zero
// bytes however many times this stages the same numbers.
inline void stage_draw_ledger(MachineCtx* c, OrbsState& orbs_state_) {
    GPUState& g = c->gpuState_;

    // The five generated families + the shell: their index counts are
    // (maxSlot + 1) * MAX_INDICES_PER_SLOT, zero when nothing is active.
    g.stage_draw_indexed(GPUState::DR_ARCH,   g.arch_index_count(),   1u);
    g.stage_draw_indexed(GPUState::DR_COLUMN, g.column_index_count(), 1u);
    g.stage_draw_indexed(GPUState::DR_PALM,   g.palm_index_count(),   1u);
    g.stage_draw_indexed(GPUState::DR_CACTUS, g.cactus_index_count(), 1u);
    g.stage_draw_indexed(GPUState::DR_BLADE,  g.blade_index_count(),  1u);
    g.stage_draw_indexed(GPUState::DR_SHELL,  g.shell_index_count(),  1u);

    // The ribbon: RIBBON_1's live vertex count, and its liveness. A ribbon
    // with no rendered slot stages zero — that IS the old guard.
    const bool ribbon_live = c->ribbon_state_.rendered_slot != UINT32_MAX;
    g.stage_draw_verts(GPUState::DR_RIBBON,
        ribbon_live ? ribbon_draw_verts(c->ribbon_state_) : 0u, 1u);

    // The artworks. slot_high_water is one past the highest ACTIVE slot; the
    // two counts that gated the draws (wall_frame_count,
    // active_painting_count) are folded in as zero-or-not.
    const uint32_t hw = c->gallery_state_.slot_high_water;
    g.stage_draw_verts(GPUState::DR_WALL,
        c->gallery_state_.wall_frame_count == 0u ? 0u : hw * Dim::PAINTING_FRAME_VERTS_PER, 1u);
    g.stage_draw_verts(GPUState::DR_GALLERY_FRAME,
        Dim::PAINTING_QUAD_VERTS,
        c->gallery_state_.active_painting_count == 0u ? 0u : hw);

    // The orbs: six indices of a quad, one instance per orb. `os.active`
    // and a zero count are the same fact to the ledger.
    g.stage_draw_indexed(GPUState::DR_ORBS, 6u,
        orbs_state_.active ? orbs_state_.count : 0u);

    // The sun's terrain: ONE draw over both bands (R-G). The instance range
    // is [0, render_patch_count) — the union of the two the fork used to
    // issue — at the LOD1 ring's live index count.
    g.stage_draw_indexed(GPUState::DR_SHADOW_TERRAIN,
        g.patch_index_count_lod1_live(),
        c->world_state_.render_patch_count);
}

// ═══ GPU COMPUTE DISPATCH ════════════════════════════════════════

// Per-frame compute: the live card, ribbon transforms, agents, camera, VP.
//
// ONE PASS WHERE TWO STOOD (SPINE_2 B). The live card write (GROUND_CARD_1)
// held its own pass for one reason: its writes must be visible to the
// consumers below. A pass boundary is one way to get that; DISPATCH ORDER
// INSIDE A PASS is the other, and it is the cheap one — a compute pass
// orders its dispatches and makes an earlier dispatch's writes visible to
// a later one. So the card is simply the FIRST dispatch here, ahead of the
// ribbon (whose fallback ground reads the card through the GoL suppression
// contributor) and ahead of everything else that reads it.
//
// THE REST-LAW SKIP BECAME A DISPATCH SKIP. `write_live_card` carries R8's
// three-conjunct decision (cartridge.hpp, phase_live_card_write): the pass
// opens either way — an empty dispatch slot is cheaper than a boundary —
// and the card's 819,200 invocations are skipped exactly when they were.
//
// PLACEMENT AND THE CULL DID NOT JOIN. Between this pass and theirs stand
// R11's three CopyBufferToBuffer (the witness capture), and a copy cannot
// be encoded inside a pass; O-2 pins it after the compute. Four passes into
// one was the ask; two into one is what the frame's shape allows.
inline void dispatch_compute(MachineCtx* c, wgpu::CommandEncoder& encoder,
                             bool write_live_card) {
    wgpu::ComputePassDescriptor desc{};
    desc.label = "Compute Phase";
    desc.timestampWrites = c->gpuState_.meter_arm_compute(meter_row::DispatchCompute);
    wgpu::ComputePassEncoder compute = encoder.BeginComputePass(&desc);
    // LOOM_2 pass head: WORLD + FRAME are every pipeline's strata 0/1.
    { compute.SetBindGroup(0, c->gpuState_.world_group());
      compute.SetBindGroup(1, c->gpuState_.frame_c_group()); }

    // THE CARD FIRST — before every consumer, on its own group 2/3 pair
    // (ZONES), which the ribbon's binds below replace.
    // IOS_3 C — ?card=0 rides the same `if` the rest law already owns. A
    // PURE SKIP for the same reason the bake's is: the live card texture is
    // zero-initialised, and zero IS its cleared value — the consumers read
    // no delta and the zones simply do not lift.
    if (write_live_card && t7::boot_params().card) {
        c->renderer_.dispatch_live_card_write(
            compute, c->gpuState_.zones_state_group(), c->gpuState_.zones_textures_group()
        );
    }

    // The ribbon room runs FIRST and on its OWN group 2 (the ribbon state
    // group) — outside the pass-head contract below, which is why it stays
    // ahead of those binds. Head then body, one pair of binds; running first
    // is also what makes the read-only agent/floater windows it binds a
    // one-frame-old read, which is what the Sky Rule wants.
    if (c->ribbon_state_.rendered_slot != UINT32_MAX) {
        c->renderer_.dispatch_ribbon(
            compute,
            c->gpuState_.ribbon_state_group(), c->gpuState_.ribbon_textures_group(),
            GPUState::ribbon_ring_workgroups()
        );
    }

    // LOOM_2 (OIL_1 U11's hoist, restratified): WORLD/FRAME ride the
    // pass head above; the family pairs bind per stratum owner — the
    // AGENTS pair here, FRAME_K inside the camera/VP helpers. The
    // agents pair is restored after the camera for the floaters.
    compute.SetBindGroup(2, c->gpuState_.agents_state_group());
    compute.SetBindGroup(3, c->gpuState_.agents_textures_group());

    c->renderer_.dispatch_update_player_agent(compute);
    c->renderer_.dispatch_update_other_agents(compute);
    c->renderer_.dispatch_update_camera_vp(compute,
        c->gpuState_.frame_k_state_group(), c->gpuState_.frame_k_textures_group());
    compute.SetBindGroup(2, c->gpuState_.agents_state_group());
    compute.SetBindGroup(3, c->gpuState_.agents_textures_group());
    c->renderer_.dispatch_update_sphere(compute);
    c->renderer_.dispatch_update_cube(compute);

    compute.End();

    // CHORD_3 — the GPU truth reaches the render frame. update_camera_vp
    // above is the sovereign writer of camera_state AND vp_data — one
    // kernel since SPINE_2, one lane, both writes; frame_r.camera and
    // frame_r.vp are the render stages' windows onto them. The copy is
    // encoded HERE, after the pass closes, because
    // the pass boundary is the ordering guarantee — and by copy rather
    // than by CPU hand because the readback law forbids the other route.
    c->gpuState_.encode_frame_r_main_sync(encoder);
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
        // LOOM_2 pass head: WORLD + FRAME are every pipeline's strata 0/1.
            { compute.SetBindGroup(0, c->gpuState_.world_group());
          compute.SetBindGroup(1, c->gpuState_.frame_c_group()); }
        c->renderer_.dispatch_frustum_cull(
            compute, c->gpuState_.cull_state_group(), c->gpuState_.empty_group()
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

        // ATLAS_1revB U2" — ONE PASS PER TEXTURE, NOT ONE PER LIGHT.
        //
        // The tiling is unchanged: 1024 x 2048 halves, lights 0-1 on the
        // sun map, 2-3 on the spot atlas, each light scissored to its own
        // half. What changed is the PASS boundary. Before, every tile was
        // its own render pass against a shared texture, so the right-hand
        // tile had to open with LoadOp::Load purely to preserve what the
        // left-hand tile had already stored — and both tiles stored the
        // whole 2048² attachment. At four lights that was 96 MiB/frame of
        // depth traffic, of which 32 MiB was preservation Loads and 32 MiB
        // duplicate Stores.
        //
        // Now each texture is cleared once, drawn for every light it owns
        // under per-light viewports, and stored once: 96 -> 32 MiB at four
        // lights, 48 -> 16 at two. No LoadOp::Load survives in this
        // function. The per-light index arrives as IMMEDIATE DATA since
        // DOMESDAY_1 B6 (it rode a dynamic offset from ATLAS_1revB D3"
        // until then) — either way a value that can change inside a
        // render pass, which is what makes one pass able to serve
        // several lights; a buffer write cannot be recorded inside a
        // render pass, and that copy is exactly what forced the split
        // before.
        //
        // A pass opens only if its texture owns at least one active light.
        const uint32_t live = (cpuSpotLights_.count < MAX_SPOT_LIGHTS)
                            ? cpuSpotLights_.count : MAX_SPOT_LIGHTS;

        // texture 0 = sun map (lights 0-1), texture 1 = spot atlas (2-3).
        for (uint32_t tex = 0; tex < 2; tex++) {
            const uint32_t first = tex * 2;
            if (first >= live) break;          // this texture owns no light

            wgpu::RenderPassDepthStencilAttachment depthAttachment{};
            depthAttachment.view = (tex == 0)
                ? c->gpuState_.shadow_map_view()
                : c->gpuState_.spot_shadow_map_view();
            depthAttachment.depthLoadOp = wgpu::LoadOp::Clear;
            depthAttachment.depthStoreOp = wgpu::StoreOp::Store;
            depthAttachment.depthClearValue = 1.0f;

            wgpu::RenderPassDescriptor desc{};
            desc.label = "Shadow Atlas";
            desc.colorAttachmentCount = 0;
            desc.depthStencilAttachment = &depthAttachment;
            desc.timestampWrites = c->gpuState_.meter_arm_render(meter_row::ShadowPass);

            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&desc);
            // LOOM_2 pass head: WORLD + FRAME + the SHADOW pair, once per
            // pass. FRAME is rebound per light inside the loop below, so
            // the head's offset is the placeholder record 0.
            pass.SetBindGroup(0, c->gpuState_.world_group());
            pass.SetBindGroup(1, c->gpuState_.frame_r_group(), 1, &kFrameSlotZero);
            pass.SetBindGroup(2, c->gpuState_.shadow_state_group());
            pass.SetBindGroup(3, c->gpuState_.shadow_textures_group());

            for (uint32_t li = first; li < first + 2 && li < live; li++) {
                const uint32_t within = li % 2;   // 0 = left half, 1 = right

                // The light index is the one thing that distinguishes
                // this light's draws from the last's: one rebind of group
                // 1 at this light's record is the whole per-light traffic.
                const uint32_t slotOffset = GPUState::shadow_slot_offset(li);
                pass.SetBindGroup(1, c->gpuState_.frame_r_group(), 1, &slotOffset);

                const float vx = static_cast<float>(within * TILE_W);
                pass.SetViewport(vx, 0.0f, static_cast<float>(TILE_W), static_cast<float>(TILE_H), 0.0f, 1.0f);
                pass.SetScissorRect(within * TILE_W, 0, TILE_W, TILE_H);

                // THE SPOT CASTER CUT (UMBRA_4) — terrain does not cast into
                // a spot tile. This is the whole of the edit: one argument,
                // one site, and the revert is this word.
                draw_shadow_all(c, pass, /*cast_terrain=*/false);
            }
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
        // Outdoor draws for the sun, which shadow_light_vp() reads from
        // frame_r.vp.light_vp; shadow_slot is unread on this path
        // (spots.count == 0) and binds record 0 for determinism.
        // BUNDLE_1: the sun's whole draw list is one recorded bundle, head
        // binds included — ExecuteBundles resets pass state, so the bundle
        // carries its own. The direct arm below is not a fallback that can
        // drift: it calls the SAME draw_shadow_all the recorder called.
        // IOS_3 C — ?sunpass=0 SKIPS THE DRAWS, NOT THE PASS, and this is
        // the one switch that could not be a pure pass-skip. FLAGGED.
        //
        // Removing the whole pass would leave the shadow map unwritten —
        // and it is `depthLoadOp = Clear` at depthClearValue 1.0 that
        // clears it. An unwritten map reads its zero-init 0.0, which the
        // PCF compare reads as "everything occluded": a world ENTIRELY in
        // shadow, which is nearly black and therefore AMBIGUOUS WITH THE
        // SYMPTOM WE ARE CHASING. A diagnostic that can be mistaken for
        // the disease is worse than none.
        //
        // So the pass still opens and still clears to 1.0 — nothing
        // occludes — and only the draw list is skipped. The picture is
        // exactly "the world with no sun shadows", which is what the
        // ladder's row expects to see.
        //
        // ?bundles=0 sits beside it: the direct arm below is not a
        // fallback that can drift — it calls the SAME draw_shadow_all the
        // recorder called (BUNDLE_1 R3).
        const bool sun_draws = t7::boot_params().sunpass;
        if (!sun_draws) {
            // the cleared depth is the whole content of the pass
        } else if (c->renderer_.shadow_sun_bundle_ready()
                   && t7::boot_params().bundles) {
            wgpu::RenderBundle b = c->renderer_.shadow_sun_bundle();
            pass.ExecuteBundles(1, &b);
        } else {
            pass.SetBindGroup(0, c->gpuState_.world_group());
            pass.SetBindGroup(1, c->gpuState_.frame_r_group(), 1, &kFrameSlotZero);
            pass.SetBindGroup(2, c->gpuState_.shadow_state_group());
            pass.SetBindGroup(3, c->gpuState_.shadow_textures_group());
            draw_shadow_all(c, pass, /*cast_terrain=*/true);
        }
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
template <class Enc>
inline void draw_shadow_all(MachineCtx* c, Enc& pass, bool cast_terrain) {
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
    // THE SUBTRACTION MASK (PANORAMA_1) — the shadow pass's two halves. Same
    // rule as the main pass: skipped at the encoder, so the pass row reads
    // the absence. `cast_terrain` is the MOOD's word and stays ahead of it.
    const uint32_t smask = c->gpuState_.config().shadow_mask;

    // ONE DRAW, ONE RECORD (BUNDLE_1, R-G). The two bands shared this
    // pipeline and this index buffer and differed only in their instance
    // RANGE: [0, lod0) then [lod0, render). Their union is [0, render) — one
    // draw of render_patch_count instances, staged by stage_draw_ledger.
    if (cast_terrain && (smask & ShadowBit::TERRAIN)) {
        c->renderer_.draw_shadow_patch_terrain(
            pass,
            c->gpuState_.patch_index_buffer_lod1(),
            c->gpuState_.draw_ledger_buffer(),
            GPUState::draw_record_offset(GPUState::DR_SHADOW_TERRAIN)
        );
    }

    // The drawable table — shadow members, canonical order.
    DrawBind b{ /*shadow=*/true, /*ribbon_bit=*/true };
    if (smask & ShadowBit::TABLE)
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
    // ATLAS_1revB G2 — group 0 is NOT rebound here any more. The two shadow
    // artwork pipelines take the RENDER-ENTITY layout at group 0 now (they
    // need frame_r.lighting, which the gallery entity layout does not
    // carry), and the pass head already bound it. Only
    // the texture group changes. One fewer bind per light, and group 0
    // no longer moves mid-tile.
    // LOOM_2: the artwork shadow draws are SHADOW family — their strata
    // are already bound, and the old texture-group fork is retired.
    if (smask & ShadowBit::TABLE) {
    c->renderer_.draw_shadow_wall_paintings(
        pass,
        c->gpuState_.draw_ledger_buffer(),
        GPUState::draw_record_offset(GPUState::DR_WALL)
    );
    c->renderer_.draw_shadow_gallery_frames(
        pass,
        c->gpuState_.draw_ledger_buffer(),
        GPUState::draw_record_offset(GPUState::DR_GALLERY_FRAME)
    );
    }
}

// ═══ THE MAIN PASS'S OPAQUE LIST (BUNDLE_1) ══════════════════════
//
// ONE DRAW LIST, TWO ENCODERS. This is the whole of what the main pass
// draws except the fade — and it is called BOTH by render_main_pass (with
// a wgpu::RenderPassEncoder, exactly as it always ran) and by
// record_bundles (with a wgpu::RenderBundleEncoder). There is no second
// list to keep in step, which is R3 written as code.
//
// THE MASKS ARE READ HERE, so under a bundle they are read AT RECORDING.
// PANORAMA_1's rule — a cleared bit is skipped at the encoder so the pass
// row reads the absence — survives that: the skip is still real, it is
// just taken once per dial-turn instead of once per frame. set_draw_mask
// and set_shadow_mask raise bundlesDirty_ so the turn re-records.
//
// NOT the fade: it is alpha-blended, order-sensitive, and gated on a value
// that moves every frame of a transition. A bundle cannot skip itself, so
// the fade stays a direct draw after ExecuteBundles.
template <class Enc>
inline void encode_main_opaque(MachineCtx* c, Enc& pass,
                               OrbsState& orbs_state_, OrbsDeps& orbs_deps_) {
    // Terrain — THE DRAW PLAN (ECONOMY_1 closing arm): the cull kernel
    // authored three lists; the pass executes them as three indirect
    // draws. Outdoor AND finite/indoor go through the same plan (the
    // kernel sees all bands everywhere). The E1 global-flag selection
    // is RETIRED here — the plan is per-patch; the flag survives only
    // for the snapshot pass (R6), which culls against the
    // photographer's frustum and cannot read this plan.
    // THE SUBTRACTION MASK (PANORAMA_1). Each draw below is skipped AT THE
    // ENCODER when its bit is clear — not culled in the shader, which would
    // still pay the pass's vertex work and leave the meter reading no
    // difference. The mask rests open; a cleared bit is a measurement.
    const uint32_t dmask = c->gpuState_.config().draw_mask;

    c->renderer_.begin_patch_terrain_plan(pass);   // OIL_1 U13: one SetPipeline for the three slots
    // DOMESDAY_0 B3: the per-slot list window rides the vertex-buffer
    // offset now (FC_SEG_A/B/C — the same segments the retired g2:62
    // bind windows carved), delivered to the VS as @location(0).
    if (dmask & DrawBit::TERRAIN_A)
    c->renderer_.draw_patch_terrain_plan_slot(pass,
        c->gpuState_.scene_state_group(),
        c->gpuState_.visible_patch_indices_buffer(), FC_SEG_A_OFF, FC_SEG_A_BYTES,   // plan A window
        c->gpuState_.patch_index_buffer(),           // full IB (zone-overlapped)
        c->gpuState_.frustum_indirect_lod0(), 0);
    if (dmask & DrawBit::TERRAIN_B)
    c->renderer_.draw_patch_terrain_plan_slot(pass,
        c->gpuState_.scene_state_group(),            // B5 (R2): the one scene group
        c->gpuState_.visible_patch_indices_buffer(), FC_SEG_B_OFF, FC_SEG_B_BYTES,   // plan B window
        c->gpuState_.patch_index_buffer_cap_only(),  // cap-only IB (clean LOD0)
        c->gpuState_.frustum_indirect_lod0(), 20);
    if (dmask & DrawBit::TERRAIN_C)
    c->renderer_.draw_patch_terrain_plan_slot(pass,
        c->gpuState_.scene_state_group(),            // B5 (R2): the one scene group
        c->gpuState_.visible_patch_indices_buffer(), FC_SEG_C_OFF, FC_SEG_C_BYTES,   // plan C window
        c->gpuState_.patch_index_buffer_lod1(),      // LOD1 IB (culled at last)
        c->gpuState_.frustum_indirect_lod0(), 40);
    // DOMESDAY_1 B5 (R2): the PlanB/PlanC windows collapsed into the
    // one scene group, so plan C left the RIGHT group bound — the old
    // restore is retired with the groups it restored from.

    // The drawable table — main members, canonical order. All opaque and
    // depth-tested, so order among them is immaterial; this is where the
    // ribbon's ordinal drift dies (it now draws with the entities, not late).
    // The ribbon is a table MEMBER, so its bit is subtracted through the
    // bind rather than by skipping the table it shares.
    DrawBind b{ /*shadow=*/false, /*ribbon_bit=*/(dmask & DrawBit::RIBBON) != 0u };
    if (dmask & DrawBit::TABLE)
        draw_table(c->renderer_, c->gpuState_, pass, b, DRAW_MAIN);

    // FORKS — the specials, kept explicit. Wall paintings + gallery frames use
    // their OWN gallery bind groups (opaque). Then the ORDER-SENSITIVE blended
    // pair, LAST and in order: orbs (additive), fade (alpha, no depth write).

    // Wall-mounted framed paintings (indoor)
    // OIL_1 U13: the gallery pair, bound ONCE for both draws.
    // ROSTER-GATE gallery (a') — matches the consumers' own gate.
    if constexpr (ROSTER.gallery) {
    pass.SetBindGroup(2, c->gpuState_.gallery_state_group());
    pass.SetBindGroup(3, c->gpuState_.gallery_textures_group());
    }
    if (dmask & DrawBit::PAINTINGS) {
    c->renderer_.draw_wall_paintings(
        pass,
        c->gpuState_.draw_ledger_buffer(),
        GPUState::draw_record_offset(GPUState::DR_WALL)
    );

    // Gallery frames (self-portrait paintings on terrain)
    c->renderer_.draw_gallery_frames(
        pass,
        c->gpuState_.draw_ledger_buffer(),
        GPUState::draw_record_offset(GPUState::DR_GALLERY_FRAME)
    );
    }

    // LOOM_2: the gallery fork left its pair at 2/3, and the orb draw
    // is SCENE family (its per-draw binds were hoisted to the strata),
    // so restore the scene pair first — the same restore-after-fork
    // the compute pass does for the agents pair.
    if constexpr (ROSTER.gallery) {
    pass.SetBindGroup(2, c->gpuState_.scene_state_group());
    pass.SetBindGroup(3, c->gpuState_.scene_textures_group());
    }
    if (dmask & DrawBit::ORBS)
        render_orbs(orbs_state_, &orbs_deps_, pass);

}

// ═══ THE BUNDLES, RECORDED (BUNDLE_1) ════════════════════════════
//
// Called at the frame boundary when bundlesDirty_, AFTER stage_draw_ledger
// and its flush — not because the bundle needs the contents (it does not:
// an indirect draw reads its count at execution) but because the ledger
// BUFFER must exist before a bundle can capture it, and because recording
// with the frame's masks already staged keeps the two in step.
//
// The head binds are the pass heads', moved inside: ExecuteBundles resets
// the pass's bind state, so a bundle must carry its own. That is also why
// the fade, which draws after ExecuteBundles, rebinds for itself.
inline void record_bundles(MachineCtx* c, OrbsState& orbs_state_, OrbsDeps& orbs_deps_) {
    {   // MAIN — the opaque canonical order, the same list render_main_pass
        // encodes directly when it has no bundle.
        wgpu::RenderBundleEncoder e = c->renderer_.make_main_bundle_encoder();
        e.SetBindGroup(0, c->gpuState_.world_group());
        e.SetBindGroup(1, c->gpuState_.frame_r_group(), 1, &kFrameSlotZero);
        e.SetBindGroup(3, c->gpuState_.scene_textures_group());
        encode_main_opaque(c, e, orbs_state_, orbs_deps_);
        wgpu::RenderBundleDescriptor bd{};
        bd.label = "Main Bundle";
        c->renderer_.set_main_bundle(e.Finish(&bd));
    }
    {   // SHADOW SUN — cast_terrain = true, which is the outdoor arm's word.
        // The indoor spot atlas is NOT bundled: it sets a viewport and a
        // scissor per tile and rebinds group 1 at each light's record, and
        // a bundle can carry none of those.
        wgpu::RenderBundleEncoder e = c->renderer_.make_shadow_sun_bundle_encoder();
        e.SetBindGroup(0, c->gpuState_.world_group());
        e.SetBindGroup(1, c->gpuState_.frame_r_group(), 1, &kFrameSlotZero);
        e.SetBindGroup(2, c->gpuState_.shadow_state_group());
        e.SetBindGroup(3, c->gpuState_.shadow_textures_group());
        draw_shadow_all(c, e, /*cast_terrain=*/true);
        wgpu::RenderBundleDescriptor bd{};
        bd.label = "Shadow Sun Bundle";
        c->renderer_.set_shadow_sun_bundle(e.Finish(&bd));
    }
    c->gpuState_.clear_bundles_dirty();
}

// ═══ MAIN PASS ═══════════════════════════════════════════════════

inline void render_main_pass(MachineCtx* c, wgpu::CommandEncoder& encoder,
    wgpu::TextureView backbuffer, wgpu::TextureView msaaColor,
    wgpu::TextureView depth,
    const float (&clearColor_)[3], OrbsState& orbs_state_, OrbsDeps& orbs_deps_) {

    // DOMESDAY_2 B10 — the msaa arm: when the boot param created a
    // multisampled color target, the pass renders into it and RESOLVES
    // into the backbuffer; the multisampled contents themselves are
    // discarded (resolve is independent of storeOp — tiler-ideal).
    // msaaColor null (msaa=1) leaves every field byte-identical to the
    // pre-B10 descriptor.
    wgpu::RenderPassColorAttachment colorAttachment{};
    colorAttachment.view = backbuffer;
    colorAttachment.loadOp = wgpu::LoadOp::Clear;
    colorAttachment.storeOp = wgpu::StoreOp::Store;
    colorAttachment.clearValue = { (double)clearColor_[0], (double)clearColor_[1], (double)clearColor_[2], 1.0 };
    if (msaaColor) {
        colorAttachment.view = msaaColor;
        colorAttachment.resolveTarget = backbuffer;
        colorAttachment.storeOp = wgpu::StoreOp::Discard;
    }

    wgpu::RenderPassDepthStencilAttachment depthAttachment{};
    depthAttachment.view = depth;
    depthAttachment.depthLoadOp = wgpu::LoadOp::Clear;
    // DISCARD_0 (PASS_0 F1) — Discard, not Store. The console's depth
    // texture is created with usage RenderAttachment ALONE
    // (console.hpp createDepthBuffer): no TextureBinding, so it cannot
    // enter a bind group and no shader can sample it; no CopySrc, so
    // nothing reads it back. Its contents are unreachable the instant
    // this pass ends, and Store writes the whole attachment to main
    // memory anyway — 4·W·H bytes per frame, every arm, for a resource
    // with no reader. Discard is the op for that case.
    //
    // THE SAFETY PROOF IS THE USAGE MASK, not this comment. If
    // createDepthBuffer ever gains TextureBinding or CopySrc, a reader
    // becomes possible and this line must go back to Store in the same
    // commit that grants it.
    depthAttachment.depthStoreOp = wgpu::StoreOp::Discard;
    depthAttachment.depthClearValue = 1.0f;

    wgpu::RenderPassDescriptor desc{};
    desc.label = "Rasterized Scene";
    desc.colorAttachmentCount = 1;
    desc.colorAttachments = &colorAttachment;
    desc.depthStencilAttachment = &depthAttachment;
    desc.timestampWrites = c->gpuState_.meter_arm_render(meter_row::MainPass);

    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&desc);

    // OIL_1 U13 (ledger: R19, C7) — THE PASS-HEAD BIND, restated for
    // the LOOM_2 numbering and the B5 collapse: groups 0 (WORLD),
    // 1 (FRAME) and 3 (scene textures) are the same for every draw in
    // this pass, so they bind once here. Group 2 is set by the plan
    // slot helper — since B5 all three slots bind the ONE scene group,
    // and the table draws inherit it after slot C.
    // Group 1 carries the shadow_slot dynamic seat, so the bind passes
    // one offset; nothing outside the shadow atlas reads it.
    // BUNDLE_1: the opaque list is one recorded bundle, head binds included
    // — ExecuteBundles resets pass state, so the bundle carries its own and
    // the fade below rebinds for itself. The direct arm cannot drift from
    // the bundle: both call encode_main_opaque.
    // IOS_3 C — ?bundles=0. The cheapest switch in the set, because
    // BUNDLE_1 R3 already built both roads: the direct arm calls
    // encode_main_opaque, which is exactly what the recorder recorded. The
    // picture must be IDENTICAL either way; if it is not, the bundle is
    // the fault and that is the finding.
    if (c->renderer_.main_bundle_ready() && t7::boot_params().bundles) {
        wgpu::RenderBundle mb = c->renderer_.main_bundle();
        pass.ExecuteBundles(1, &mb);
    } else {
        pass.SetBindGroup(0, c->gpuState_.world_group());
        pass.SetBindGroup(1, c->gpuState_.frame_r_group(), 1, &kFrameSlotZero);
        pass.SetBindGroup(3, c->gpuState_.scene_textures_group());
        encode_main_opaque(c, pass, orbs_state_, orbs_deps_);
    }

    // THE FADE REBINDS FOR ITSELF. ExecuteBundles resets every bind, so
    // group 0 — which used to come from the pass head — is restated here
    // with the three EMPTY strata the fade's layout wants.
    pass.SetBindGroup(0, c->gpuState_.world_group());

    // The fade's own bit — the one mask read that stays in the pass,
    // because the fade is the one draw that stays out of the bundle.
    const uint32_t dmask = c->gpuState_.config().draw_mask;

    // Fade overlay (drawn last, alpha blended over everything)
    // LOOM_2: the fade overlay binds WORLD only; its other strata are
    // the shared EMPTY filler (A5).
    pass.SetBindGroup(1, c->gpuState_.empty_group());
    pass.SetBindGroup(2, c->gpuState_.empty_group());
    pass.SetBindGroup(3, c->gpuState_.empty_group());
    // ONE FACT, ONE HOME (LATTICE_4). The gate inside draw_fade_overlay
    // decides whether the blend can move a pixel, and the shader reads
    // config.fade_alpha — so the gate must read the STAGED value too, not
    // the CPU-side mood field it was staged from. They agree today
    // (phase_stage_fade_and_upload runs set_fade every frame), and that is
    // exactly the kind of agreement that quietly stops being true.
    if (dmask & DrawBit::FADE)
    c->renderer_.draw_fade_overlay(
        pass,
        c->gpuState_.config().fade_alpha
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
