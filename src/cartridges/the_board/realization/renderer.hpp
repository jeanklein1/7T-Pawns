#pragma once

// THE_BOARD CARTRIDGE -- Renderer (Rasterized)
// ==================================================

#include "cartridges/the_board/realization/state.hpp"
#include <webgpu/webgpu_cpp.h>
#include <string>
#include <fstream>
#include <sstream>
#include <array>
#include <chrono>
#include <iostream>
#include <vector>
#include <functional>
#include <algorithm>
#include <iomanip>

namespace t7 {
    namespace the_board {

        namespace Entry {
            // Compute — split world update (ordered by dependency)
            constexpr const char* UPDATE_PLAYER_AGENT = "update_player_agent";        // 0D (1 thread, possessed slot)
            constexpr const char* UPDATE_OTHER_AGENTS = "update_other_agents";        // 1D (32 threads, non-possessed slots)
            constexpr const char* UPDATE_CAMERA = "update_camera";                  // 0D
            constexpr const char* UPDATE_SPHERE = "update_sphere";                  // 0D
            constexpr const char* UPDATE_CUBE = "update_cube";                      // 0D
            constexpr const char* COMPUTE_VP = "compute_vp";                    // 0D

            // Init-only
            constexpr const char* GENERATE_TERRAIN_INDICES = "generate_terrain_indices";  // 2D -- one-shot

            // On-demand compute
            constexpr const char* GENERATE_PATCH_HEIGHTS = "generate_patch_heights";          // 2D -- per-patch, pass 1
            constexpr const char* GENERATE_PATCH_GRADIENTS = "generate_patch_gradients";      // 2D -- per-patch, pass 2
            constexpr const char* GENERATE_PATCH_CELLS = "generate_patch_cells";              // 2D -- per-patch
            constexpr const char* COMPUTE_RIBBON_RINGS = "compute_ribbon_rings";              // 1D -- per ring
            constexpr const char* COMPUTE_PAWN_AURA = "compute_pawn_aura";                  // 2D -- toroidal grid
            constexpr const char* WRITE_LIVE_CARD_HEIGHTS = "write_live_card_heights";      // 2D -- card pass 1 (TRUEBAND_CONTACT_1)
            constexpr const char* WRITE_LIVE_CARD_RESOLVE = "write_live_card_resolve";      // 2D -- card pass 2 (gradients + store)
            constexpr const char* ZONE_SEED_MASK = "zone_seed_mask";                        // 2D -- the vocabulary mask (UNIFIED_GROUND_1)

            // Render
            constexpr const char* PATCH_TERRAIN_VS = "patch_terrain_vs";
            constexpr const char* PATCH_TERRAIN_FS = "patch_terrain_fs";
            constexpr const char* SHADOW_PATCH_TERRAIN_VS = "shadow_patch_terrain_vs";
            constexpr const char* PAWN_VS = "pawn_vs";
            constexpr const char* SPHERE_VS = "sphere_vs";
            constexpr const char* MONOLITH_VS = "monolith_vs";
            constexpr const char* ENTITY_FS = "entity_fs";
            constexpr const char* RIBBON_FS = "ribbon_fs";   // the veil's ruled exemption: entity shading, veil_scale 0
            constexpr const char* SHADOW_PAWN_VS = "shadow_pawn_vs";
            constexpr const char* SHADOW_SPHERE_VS = "shadow_sphere_vs";
            constexpr const char* SHADOW_MONOLITH_VS = "shadow_monolith_vs";
            constexpr const char* RIBBON_VS = "ribbon_vs";
            constexpr const char* SHADOW_RIBBON_VS = "shadow_ribbon_vs";
            constexpr const char* ARCH_VS = "arch_vs";
            constexpr const char* SHADOW_ARCH_VS = "shadow_arch_vs";
            constexpr const char* COLUMN_VS = "column_vs";
            constexpr const char* SHADOW_COLUMN_VS = "shadow_column_vs";
            // PYRAMID_VS / SHADOW_PYRAMID_VS CUT — pyramid mesh never drawn
            constexpr const char* SHELL_VS = "shell_vs";
            constexpr const char* SHADOW_SHELL_VS = "shadow_shell_vs";

            // Gallery (self-portrait painting frames)
            constexpr const char* GALLERY_FRAME_VS = "gallery_frame_vs";
            constexpr const char* GALLERY_FRAME_FS = "gallery_frame_fs";
            // SHADOW_GALLERY_FRAME_VS CUT — caller-free shadow

            // Wall-mounted framed paintings (indoor)
            constexpr const char* WALL_PAINTING_VS        = "wall_painting_vs";
            constexpr const char* WALL_PAINTING_CANVAS_FS = "wall_painting_canvas_fs";
            constexpr const char* WALL_PAINTING_FRAME_FS  = "wall_painting_frame_fs";
            // SHADOW_WALL_PAINTING_VS CUT — caller-free shadow

            // Photographer compute (GPU-coupled snapshot camera)
            constexpr const char* COMPUTE_PHOTOGRAPHER_VP = "compute_photographer_vp";

            // Entity placement Y-correction (decoupled from photographer)
            constexpr const char* COMPUTE_ENTITY_PLACEMENT = "compute_entity_placement";

            // GPU frustum culling (every frame, after compute_vp)
            constexpr const char* FRUSTUM_CULL_PATCHES = "frustum_cull_patches";

            // GoL zone compute (zone-local automaton)
            constexpr const char* ZONE_GOL_SYNC = "zone_gol_sync";
            constexpr const char* ZONE_GOL_EVOLVE = "zone_gol_evolve";
            constexpr const char* ZONE_DERIVE_PARAMS = "zone_derive_params";

            // Zone extrusion rendering

            // GPU Entity Mesh Gen (Phase 2: Arches, Phase 3: Columns — pyramid mesh-gen CUT)
            constexpr const char* ARCH_MESH_GEN = "arch_mesh_gen";
            constexpr const char* COLUMN_MESH_GEN = "column_mesh_gen";
            constexpr const char* PALM_MESH_GEN = "palm_mesh_gen";
            constexpr const char* PALM_VS = "palm_vs";
            constexpr const char* SHADOW_PALM_VS = "shadow_palm_vs";
            constexpr const char* CACTUS_MESH_GEN = "cactus_mesh_gen";
            constexpr const char* CACTUS_VS = "cactus_vs";
            constexpr const char* SHADOW_CACTUS_VS = "shadow_cactus_vs";
            constexpr const char* BLADE_MESH_GEN = "blade_cluster_mesh_gen";
            constexpr const char* BLADE_VS = "blade_cluster_vs";
            constexpr const char* SHADOW_BLADE_VS = "shadow_blade_cluster_vs";

            // Fade overlay (fullscreen transition)
            constexpr const char* FADE_OVERLAY_VS = "fade_overlay_vs";
            constexpr const char* FADE_OVERLAY_FS = "fade_overlay_fs";

            // Orb sky layer (luminous points on a dome)
            constexpr const char* ORB_INIT           = "orb_init";             // 1D compute
            constexpr const char* ORB_DYNAMICS       = "orb_dynamics";         // 1D compute
            constexpr const char* ORB_RECOLOR        = "orb_recolor";          // 1D compute
            constexpr const char* ORB_STATE_PREV_COPY = "orb_state_prev_copy"; // 1D compute (Pass 9)
            constexpr const char* ORB_VS             = "orb_vs";
            constexpr const char* ORB_FS             = "orb_fs";
        }

        class Renderer {

            wgpu::Device device_;
            wgpu::BindGroupLayout computeEntityLayout_;
            wgpu::BindGroupLayout computeTextureLayout_;   // Group 1 for live-contributor compute (sphere/cube)
            wgpu::BindGroupLayout terrainIndexGenLayout_;
            wgpu::BindGroupLayout patchGenLayout_;
            wgpu::BindGroupLayout renderEntityLayout_;
            wgpu::BindGroupLayout renderTextureLayout_;
            wgpu::BindGroupLayout shadowTextureLayout_;
            wgpu::BindGroupLayout ribbonComputeLayout_;
            wgpu::BindGroupLayout galleryEntityLayout_;
            wgpu::BindGroupLayout galleryTextureLayout_;
            wgpu::BindGroupLayout meshGenEntityLayout_;  // binding 1 only (config) — reused by fade overlay
            wgpu::BindGroupLayout photographerComputeLayout_;
            wgpu::BindGroupLayout pawnAuraComputeLayout_;
            wgpu::BindGroupLayout liveCardWriterLayout_;   // GROUND_CARD_1
            wgpu::BindGroupLayout zoneMaskLayout_;         // UNIFIED_GROUND_1 U5
            wgpu::BindGroupLayout zoneGolComputeLayout_;
            wgpu::BindGroupLayout archMeshGenLayout_;
            wgpu::BindGroupLayout columnMeshGenLayout_;
            wgpu::BindGroupLayout palmMeshGenLayout_;
            wgpu::BindGroupLayout cactusMeshGenLayout_;
            wgpu::BindGroupLayout bladeMeshGenLayout_;
            wgpu::TextureFormat colorFormat_;
            wgpu::TextureFormat depthFormat_;

            wgpu::ShaderModule shaderModule_;
            std::string shaderSource_;
            std::string shaderPath_;

            struct PipelineTiming { std::string label; long long ms; };
            std::vector<PipelineTiming> pipelineTimings_;

            template <typename F>
            bool tPipe(const char* label, F&& fn) {
                auto t0 = std::chrono::high_resolution_clock::now();
                bool ok = fn();
                auto t1 = std::chrono::high_resolution_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
                std::cout << "  [Pipeline] " << label << ": " << ms << " ms\n";
                pipelineTimings_.push_back({label, ms});
                return ok;
            }

            // THE SHARED BUILDERS (cable management): the two collapses every compute pipeline shared.
            // computeLayoutFor wraps a single bind-group layout into a pipeline layout
            // (the ~24 dedicated compute pipelines each repeated this 4-line boilerplate).
            // makeComputePipeline is the uniform creation ALL 30 compute pipelines shared —
            // a pure (entry-point, pipeline-layout) pair over the one shaderModule_; the
            // descriptor carried no other per-pipeline state. The FORKS stay at the call
            // site: which layout, which entry string (passed VERBATIM — the sole C++->shader
            // link), which member, and the ROSTER gate. (The layout-build moves just outside
            // the tPipe timing block; the boot-leaderboard ms now excludes the trivial layout
            // creation — no behavior/pixel effect.)
            wgpu::PipelineLayout computeLayoutFor(wgpu::BindGroupLayout bgl) {
                std::array<wgpu::BindGroupLayout, 1> a = { bgl };
                wgpu::PipelineLayoutDescriptor d{};
                d.bindGroupLayoutCount = a.size();
                d.bindGroupLayouts = a.data();
                return device_.CreatePipelineLayout(&d);
            }
            bool makeComputePipeline(const char* label, const char* dbgLabel,
                                     wgpu::PipelineLayout layout, const char* entry,
                                     wgpu::ComputePipeline& out) {
                return tPipe(label, [&]() {
                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = dbgLabel;
                    desc.layout = layout;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = entry;
                    out = device_.CreateComputePipeline(&desc);
                    return out != nullptr;
                });
            }

            // Compute pipelines -- per-frame (split world update)
            wgpu::ComputePipeline updatePlayerAgentPipeline_;    // 0D (1 thread, possessed slot)
            wgpu::ComputePipeline updateOtherAgentsPipeline_;    // 1D (32 threads, non-possessed)
            wgpu::ComputePipeline updateCameraPipeline_;         // 0D
            wgpu::ComputePipeline updateSpherePipeline_;         // 0D
            wgpu::ComputePipeline updateCubePipeline_;           // 0D
            wgpu::ComputePipeline computeVPPipeline_;         // 0D

            // Compute pipelines -- terrain index generation (one-shot at init)
            wgpu::ComputePipeline generateTerrainIndicesPipeline_;  // 2D

            // Compute pipelines -- patch heightfield generation (per-patch, two-pass)
            wgpu::ComputePipeline generatePatchHeightsPipeline_;     // 2D -- pass 1: heights only
            wgpu::ComputePipeline generatePatchGradientsPipeline_;   // 2D -- pass 2: gradients + complexity
            wgpu::ComputePipeline generatePatchCellsPipeline_;        // 2D
            wgpu::ComputePipeline ribbonRingPipeline_;                    // 1D -- ribbon ring transforms

            // Render pipelines
            wgpu::RenderPipeline pawnPipeline_;          // Chess pawn entity
            wgpu::RenderPipeline spherePipeline_;        // Sphere entity
            wgpu::RenderPipeline monolithPipeline_;      // Monolith entity
            wgpu::RenderPipeline ribbonPipeline_;        // Sky ribbon entity
            wgpu::RenderPipeline archPipeline_;          // Catenary arch entity
            wgpu::RenderPipeline columnPipeline_;        // Generative column entity
            wgpu::RenderPipeline palmPipeline_;          // Palm tree entity
            wgpu::RenderPipeline cactusPipeline_;         // Cactus entity
            wgpu::RenderPipeline bladePipeline_;          // Blade cluster entity
            // pyramidPipeline_ CUT — pyramid mesh never drawn
            wgpu::RenderPipeline shellPipeline_;         // Indoor shell (ceiling + walls)

            // Shadow pass pipelines (depth-only, no fragment shader)
            wgpu::RenderPipeline shadowPawnPipeline_;
            wgpu::RenderPipeline shadowSpherePipeline_;
            wgpu::RenderPipeline shadowMonolithPipeline_;
            wgpu::RenderPipeline shadowRibbonPipeline_;
            wgpu::RenderPipeline shadowArchPipeline_;
            wgpu::RenderPipeline shadowColumnPipeline_;
            wgpu::RenderPipeline shadowPalmPipeline_;
            wgpu::RenderPipeline shadowCactusPipeline_;
            wgpu::RenderPipeline shadowBladePipeline_;
            // shadowPyramidPipeline_ CUT
            wgpu::RenderPipeline shadowShellPipeline_;

            // Patch terrain pipelines (instanced rendering)
            wgpu::RenderPipeline patchTerrainPipeline_;          // direct draw; all FS features compiled in
            wgpu::RenderPipeline patchTerrainIndirectPipeline_;  // same + USE_PATCH_INDIRECTION=true (for frustum cull)
            bool useIndirectTerrainPipeline_ = false;
            wgpu::RenderPipeline shadowPatchTerrainPipeline_;

            // Gallery frame pipeline (painting quads in the world)
            wgpu::RenderPipeline galleryFramePipeline_;
            // shadowGalleryFramePipeline_ CUT — caller-free shadow

            // Wall-mounted framed paintings (indoor) — uses galleryEntity + galleryTexture layouts
            wgpu::RenderPipeline wallPaintingCanvasPipeline_;
            wgpu::RenderPipeline wallPaintingFramePipeline_;
            // shadowWallPaintingPipeline_ CUT — caller-free shadow

            // Photographer VP compute pipeline (0D, GPU-coupled camera)
            wgpu::ComputePipeline photographerVPPipeline_;
            // Entity placement Y-correction pipeline (0D, decoupled from photographer)
            wgpu::ComputePipeline entityPlacementPipeline_;
            wgpu::ComputePipeline frustumCullPipeline_;
            wgpu::BindGroupLayout frustumCullLayout_;
            wgpu::BindGroupLayout entityPlacementComputeLayout_;
            wgpu::ComputePipeline pawnAuraPipeline_;
            wgpu::ComputePipeline liveCardHeightsPipeline_;  // TRUEBAND_CONTACT_1 (two-pass writer)
            wgpu::ComputePipeline liveCardResolvePipeline_;
            wgpu::ComputePipeline zoneSeedMaskPipeline_;     // UNIFIED_GROUND_1 U5

            // Orb sky layer pipelines
            wgpu::BindGroupLayout orbComputeLayout_;
            wgpu::BindGroupLayout orbCopyLayout_;
            wgpu::ComputePipeline orbInitPipeline_;
            wgpu::ComputePipeline orbDynamicsPipeline_;
            wgpu::ComputePipeline orbRecolorPipeline_;
            wgpu::ComputePipeline orbCopyPrevPipeline_;
            wgpu::RenderPipeline  orbRenderPipeline_;

            // GoL zone compute pipelines (dedicated layout, z-dispatched per zone)
            wgpu::ComputePipeline zoneGolSyncPipeline_;
            wgpu::ComputePipeline zoneGolEvolvePipeline_;

            // Zone mesh gen (two-group: compute entity + mesh gen)
            wgpu::ComputePipeline zoneDeriveParamsPipeline_;

            // Zone extrusion render

            // Fade overlay (fullscreen alpha-blended triangle)
            wgpu::RenderPipeline fadeOverlayPipeline_;

            // GPU entity mesh gen (Phase 2: arches, Phase 3: columns — pyramid mesh-gen CUT)
            wgpu::ComputePipeline archMeshGenPipeline_;
            wgpu::ComputePipeline columnMeshGenPipeline_;
            wgpu::ComputePipeline palmMeshGenPipeline_;
            wgpu::ComputePipeline cactusMeshGenPipeline_;
            wgpu::ComputePipeline bladeMeshGenPipeline_;

        public:

            Renderer() = default;
            Renderer(const Renderer&) = delete;
            Renderer& operator=(const Renderer&) = delete;

            bool init(
                wgpu::Device device,
                const GPUState& gpuState,
                wgpu::TextureFormat colorFormat,
                wgpu::TextureFormat depthFormat
            ) {
                device_ = device;
                computeEntityLayout_ = gpuState.compute_entity_layout();
                computeTextureLayout_ = gpuState.compute_texture_layout();
                terrainIndexGenLayout_ = gpuState.terrain_index_gen_layout();
                patchGenLayout_ = gpuState.patch_gen_layout();
                renderEntityLayout_ = gpuState.render_entity_layout();
                renderTextureLayout_ = gpuState.render_texture_layout();
                shadowTextureLayout_ = gpuState.shadow_texture_layout();
                ribbonComputeLayout_ = gpuState.ribbon_compute_layout();
                galleryEntityLayout_ = gpuState.gallery_entity_layout();
                galleryTextureLayout_ = gpuState.gallery_texture_layout();
                meshGenEntityLayout_ = gpuState.mesh_gen_entity_layout();
                photographerComputeLayout_ = gpuState.photographer_compute_layout();
                entityPlacementComputeLayout_ = gpuState.entity_placement_compute_layout();
                frustumCullLayout_ = gpuState.frustum_cull_layout();
                pawnAuraComputeLayout_ = gpuState.pawn_aura_compute_layout();
                liveCardWriterLayout_ = gpuState.live_card_writer_layout();
                zoneMaskLayout_ = gpuState.zone_mask_layout();
                orbComputeLayout_ = gpuState.orb_compute_layout();
                orbCopyLayout_    = gpuState.orb_copy_layout();
                zoneGolComputeLayout_ = gpuState.zone_gol_compute_layout();
                archMeshGenLayout_ = gpuState.arch_mesh_gen_layout();
                columnMeshGenLayout_ = gpuState.column_mesh_gen_layout();
                palmMeshGenLayout_ = gpuState.palm_mesh_gen_layout();
                cactusMeshGenLayout_ = gpuState.cactus_mesh_gen_layout();
                bladeMeshGenLayout_ = gpuState.blade_mesh_gen_layout();
                colorFormat_ = colorFormat;
                depthFormat_ = depthFormat;

                if (!loadShader()) return false;

                auto t0 = std::chrono::high_resolution_clock::now();
                if (!createComputePipelines()) return false;
                auto t1 = std::chrono::high_resolution_clock::now();
                if (!createRenderPipelines()) return false;
                auto t2 = std::chrono::high_resolution_clock::now();

                // Sorted bottleneck leaderboard
                {
                    auto sorted = pipelineTimings_;
                    std::sort(sorted.begin(), sorted.end(),
                        [](const PipelineTiming& a, const PipelineTiming& b) { return a.ms > b.ms; });
                    std::cout << "\n[Renderer] Pipelines by compile time (descending):\n";
                    for (const auto& t : sorted) {
                        std::cout << "  " << std::setw(8) << t.ms << " ms  " << t.label << "\n";
                    }
                    std::cout << "\n";
                }

                std::cout << "[Renderer] Compute pipelines: "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms\n";
                std::cout << "[Renderer] Render pipelines:  "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " ms\n";
                std::cout << "[Renderer] Total pipelines:   "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t0).count() << " ms\n";

                return true;
            }


            // RAYMARCH/SDF EXCAVATION: dispatch_update_terrain_config removed
            // (dead TerrainState writer).

            void dispatch_update_player_agent(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup
            ) {
                pass.SetPipeline(updatePlayerAgentPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);   // live-contributor textures (POLICY_WALKER)
                pass.DispatchWorkgroups(1, 1, 1);         // 1 workgroup × 1 thread = the possessed slot
            }

            void dispatch_update_other_agents(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup
            ) {
                if constexpr (!(ROSTER.wanderers)) return;  // ROSTER-GATE wanderers (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(updateOtherAgentsPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);   // live-contributor textures (POLICY_WALKER_AGENT aura)
                pass.DispatchWorkgroups(1, 1, 1);         // 1 workgroup × 32 threads = all non-player slots
            }

            void dispatch_update_camera(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup
            ) {
                pass.SetPipeline(updateCameraPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);   // live-contributor textures (POLICY_FLYER)
                pass.DispatchWorkgroups(1, 1, 1);
            }

            void dispatch_update_sphere(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup
            ) {
                if constexpr (!(ROSTER.sphere)) return;  // ROSTER-GATE sphere (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(updateSpherePipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);   // live-contributor textures (aura)
                pass.DispatchWorkgroups(1, 1, 1);
            }

            void dispatch_update_cube(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup
            ) {
                if constexpr (!(ROSTER.cube)) return;  // ROSTER-GATE cube (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(updateCubePipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);   // live-contributor textures (aura)
                pass.DispatchWorkgroups(1, 1, 1);
            }

            void dispatch_compute_vp(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup entityBindGroup
            ) {
                pass.SetPipeline(computeVPPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.DispatchWorkgroups(1, 1, 1);  // 0D: single invocation
            }

            // Terrain index generation -- one-shot at init.
            // Fills the terrain index buffer on the GPU. Called once, never again.

            void dispatch_generate_terrain_indices(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup terrainIndexGenBindGroup,
                uint32_t workgroups
            ) {
                pass.SetPipeline(generateTerrainIndicesPipeline_);
                pass.SetBindGroup(0, terrainIndexGenBindGroup);
                pass.DispatchWorkgroups(workgroups, workgroups, 1);
            }

            // Pass 1: evaluate ground_formed() per texel, store height only.
            void dispatch_generate_patch_heights(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup patchGenBindGroup,
                uint32_t workgroups
            ) {
                pass.SetPipeline(generatePatchHeightsPipeline_);
                pass.SetBindGroup(0, patchGenBindGroup);
                pass.DispatchWorkgroups(workgroups, workgroups, 1);
            }

            // Pass 2: read stored heights from neighbors, compute gradients + complexity.
            void dispatch_generate_patch_gradients(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup patchGenBindGroup,
                uint32_t workgroups
            ) {
                pass.SetPipeline(generatePatchGradientsPipeline_);
                pass.SetBindGroup(0, patchGenBindGroup);
                pass.DispatchWorkgroups(workgroups, workgroups, 1);
            }

            void dispatch_generate_patch_cells(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup patchGenBindGroup,
                uint32_t workgroups
            ) {
                pass.SetPipeline(generatePatchCellsPipeline_);
                pass.SetBindGroup(0, patchGenBindGroup);
                pass.DispatchWorkgroups(workgroups, workgroups, 1);
            }

            void dispatch_compute_ribbon_rings(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup ribbonComputeBindGroup,
                uint32_t workgroups
            ) {
                if constexpr (!(ROSTER.ribbon)) return;  // ROSTER-GATE ribbon (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(ribbonRingPipeline_);
                pass.SetBindGroup(0, ribbonComputeBindGroup);
                pass.DispatchWorkgroups(workgroups, 1, 1);
            }

            void dispatch_compute_photographer_vp(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup photographerComputeBindGroup
            ) {
                if constexpr (!(ROSTER.gallery)) return;  // ROSTER-GATE gallery (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(photographerVPPipeline_);
                pass.SetBindGroup(0, photographerComputeBindGroup);
                pass.DispatchWorkgroups(1, 1, 1);
            }

            void dispatch_entity_placement(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup entityPlacementBindGroup
            ) {
                pass.SetPipeline(entityPlacementPipeline_);
                pass.SetBindGroup(0, entityPlacementBindGroup);
                pass.DispatchWorkgroups(1, 1, 1);
            }

            void dispatch_frustum_cull(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup frustumCullBindGroup
            ) {
                pass.SetPipeline(frustumCullPipeline_);
                pass.SetBindGroup(0, frustumCullBindGroup);
                // ceil(MAX_ACTIVE_PATCHES / 64) — derived, never hardcoded again
                // (was hardcoded 4 = 256 threads vs 289 slots: slots 256–288 were
                //  never culled at full window — audit CC-8a).
                pass.DispatchWorkgroups((Dim::MAX_ACTIVE_PATCHES + 63u) / 64u, 1, 1);
            }

            void dispatch_compute_pawn_aura(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup auraComputeBindGroup,
                uint32_t workgroups
            ) {
                if constexpr (!(ROSTER.pawn_aura)) return;  // ROSTER-GATE pawn_aura (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(pawnAuraPipeline_);
                pass.SetBindGroup(0, auraComputeBindGroup);
                pass.DispatchWorkgroups(workgroups, workgroups, 1);
            }

            void dispatch_live_card_write(wgpu::ComputePassEncoder& pass,
                                          wgpu::BindGroup group) {
                // Two-pass writer (TRUEBAND_CONTACT_1): heights → scratch,
                // then resolve (gradients + store). Sequential dispatches in
                // ONE pass — storage-buffer visibility between dispatches is
                // guaranteed (the U5a same-pass law).
                pass.SetPipeline(liveCardHeightsPipeline_);
                pass.SetBindGroup(0, group);
                pass.DispatchWorkgroups(Dim::LIVE_CARD_SIZE / 8u,
                                        Dim::LIVE_CARD_SIZE / 8u, 1);
                pass.SetPipeline(liveCardResolvePipeline_);
                pass.DispatchWorkgroups(Dim::LIVE_CARD_SIZE / 16u,
                                        Dim::LIVE_CARD_SIZE / 16u, 1);
            }

            void dispatch_orb_init(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup orbComputeGroup,
                uint32_t workgroups
            ) {
                if constexpr (!(ROSTER.orbs)) return;  // ROSTER-GATE orbs (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(orbInitPipeline_);
                pass.SetBindGroup(0, orbComputeGroup);
                pass.DispatchWorkgroups(workgroups, 1, 1);
            }

            void dispatch_orb_dynamics(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup orbComputeGroup,
                uint32_t workgroups
            ) {
                if constexpr (!(ROSTER.orbs)) return;  // ROSTER-GATE orbs (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(orbDynamicsPipeline_);
                pass.SetBindGroup(0, orbComputeGroup);
                pass.DispatchWorkgroups(workgroups, 1, 1);
            }

            void dispatch_orb_recolor(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup orbComputeGroup,
                uint32_t workgroups
            ) {
                if constexpr (!(ROSTER.orbs)) return;  // ROSTER-GATE orbs (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(orbRecolorPipeline_);
                pass.SetBindGroup(0, orbComputeGroup);
                pass.DispatchWorkgroups(workgroups, 1, 1);
            }

            void dispatch_orb_copy_prev(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup orbCopyGroup,
                uint32_t workgroups
            ) {
                if constexpr (!(ROSTER.orbs)) return;  // ROSTER-GATE orbs (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(orbCopyPrevPipeline_);
                pass.SetBindGroup(0, orbCopyGroup);
                pass.DispatchWorkgroups(workgroups, 1, 1);
            }

            void draw_orbs(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer quadVB,
                wgpu::Buffer quadIB,
                uint32_t orbCount
            ) {
                if constexpr (!(ROSTER.orbs)) return;  // ROSTER-GATE orbs (a') — pipeline never created; the holder tolerates
                if (orbCount == 0) return;
                pass.SetPipeline(orbRenderPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, quadVB);
                pass.SetIndexBuffer(quadIB, wgpu::IndexFormat::Uint16);
                pass.DrawIndexed(6, orbCount, 0, 0, 0);
            }

            void dispatch_zone_gol_sync(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup zoneComputeBindGroup,
                uint32_t zone_count
            ) {
                if constexpr (!(ROSTER.gol)) return;  // ROSTER-GATE gol (a') — pipeline never created; the holder tolerates
                if (zone_count == 0) return;
                pass.SetPipeline(zoneGolSyncPipeline_);
                pass.SetBindGroup(0, zoneComputeBindGroup);
                pass.DispatchWorkgroups(4, 4, zone_count);  // 32/8=4 per axis, z=zones
            }

            void dispatch_zone_gol_evolve(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup zoneComputeBindGroup,
                uint32_t zone_count
            ) {
                if constexpr (!(ROSTER.gol)) return;  // ROSTER-GATE gol (a') — pipeline never created; the holder tolerates
                if (zone_count == 0) return;
                pass.SetPipeline(zoneGolEvolvePipeline_);
                pass.SetBindGroup(0, zoneComputeBindGroup);
                pass.DispatchWorkgroups(4, 4, zone_count);
            }

            // Zone mesh gen (single group — same layout as sync/evolve)

            // Zone parameter derivation (GPU-authoritative tier selection + Gaussian sampling)
            void dispatch_zone_derive_params(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup zoneGroup,
                uint32_t request_count
            ) {
                if constexpr (!(ROSTER.gol)) return;  // ROSTER-GATE gol (a') — pipeline never created; the holder tolerates
                if (request_count == 0) return;
                pass.SetPipeline(zoneDeriveParamsPipeline_);
                pass.SetBindGroup(0, zoneGroup);
                pass.DispatchWorkgroups(request_count, 1, 1);
            }

            void dispatch_zone_seed_mask(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup maskGroup,
                uint32_t request_count
            ) {
                if constexpr (!(ROSTER.gol)) return;  // ROSTER-GATE gol (a') — pipeline never created; the holder tolerates
                if (request_count == 0) return;
                pass.SetPipeline(zoneSeedMaskPipeline_);
                pass.SetBindGroup(0, maskGroup);
                pass.DispatchWorkgroups(4, 4, request_count);
            }

            // dispatch_pyramid_mesh_gen CUT — mesh never drawn;
            // the FAMILY_DISPATCH pyramid mesh hook now routes to the none-fork.

            // GPU arch mesh gen — generates all 16 slots × 4 sub-meshes.
            // gid.x = slot (0..15), gid.y = sub-mesh (outer shell, inner shell, front cap, back cap).
            void dispatch_arch_mesh_gen(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup meshGenGroup
            ) {
                if constexpr (!(ROSTER.arch)) return;  // ROSTER-GATE arch (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(archMeshGenPipeline_);
                pass.SetBindGroup(0, meshGenGroup);
                pass.DispatchWorkgroups(Dim::MAX_ARCH_INSTANCES, 4, 1);
            }

            // GPU column mesh gen — generates all 32 slots in one dispatch.
            void dispatch_column_mesh_gen(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup meshGenGroup
            ) {
                if constexpr (!(ROSTER.column || ROSTER.antenna)) return;  // ROSTER-GATE column+antenna (shared pipelines) (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(columnMeshGenPipeline_);
                pass.SetBindGroup(0, meshGenGroup);
                pass.DispatchWorkgroups(Dim::MAX_COLUMN_INSTANCES, 1, 1);
            }

            void dispatch_palm_mesh_gen(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup meshGenGroup
            ) {
                if constexpr (!(ROSTER.palm)) return;  // ROSTER-GATE palm (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(palmMeshGenPipeline_);
                pass.SetBindGroup(0, meshGenGroup);
                pass.DispatchWorkgroups(Dim::MAX_PALM_INSTANCES, 1, 1);
            }

            void dispatch_cactus_mesh_gen(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup meshGenGroup
            ) {
                if constexpr (!(ROSTER.cactus)) return;  // ROSTER-GATE cactus (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(cactusMeshGenPipeline_);
                pass.SetBindGroup(0, meshGenGroup);
                pass.DispatchWorkgroups(Dim::MAX_CACTUS_INSTANCES, 1, 1);
            }

            void dispatch_blade_mesh_gen(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup group
            ) {
                if constexpr (!(ROSTER.blade)) return;  // ROSTER-GATE blade (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(bladeMeshGenPipeline_);
                pass.SetBindGroup(0, group);
                pass.DispatchWorkgroups(Dim::MAX_BLADE_INSTANCES, 1, 1);
            }

            // Zone extrusion rendering


            void draw_patch_terrain_lod0_indirect(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer indexBufferLOD0,
                wgpu::Buffer indirectLOD0
            ) {
                pass.SetPipeline(patchTerrainIndirectPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetIndexBuffer(indexBufferLOD0, wgpu::IndexFormat::Uint32);
                pass.DrawIndexedIndirect(indirectLOD0, 0);
            }

            // Direct terrain draw — uses non-indirect pipeline (outdoor or indoor variant).
            // For LOD1 outdoor, LOD0+LOD1 indoor, snapshot pass, etc.
            void draw_patch_terrain_direct(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount,
                uint32_t instanceCount,
                uint32_t firstInstance = 0
            ) {
                pass.SetPipeline(patchTerrainPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount, instanceCount, 0, 0, firstInstance);
            }

            // Frustum cull activation — typically driven by world type (walled vs open).
            // Walled worlds (small, finite) benefit less from culling; open worlds do.
            void set_frustum_cull_active(bool active) { useIndirectTerrainPipeline_ = active; }
            bool use_indirect_terrain() const { return useIndirectTerrainPipeline_; }

            void draw_pawn(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                uint32_t vertexCount
            ) {
                pass.SetPipeline(pawnPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                // One instance per agent slot. Inactive slots collapse via a
                // zero-scale local mesh in pawn_vs (see is_active branch).
                pass.Draw(vertexCount, /*instanceCount=*/ Dim::MAX_AGENTS);
            }

            void draw_sphere(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.sphere)) return;  // ROSTER-GATE sphere (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(spherePipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount, Dim::MAX_SPHERE_INSTANCES);
            }

            void draw_monolith(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.cube)) return;  // ROSTER-GATE cube (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(monolithPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount, Dim::MAX_CUBE_INSTANCES, 0, 0, Dim::CUBE_SLOT_OFFSET);
            }

            void draw_ribbon(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                uint32_t vertexCount
            ) {
                if constexpr (!(ROSTER.ribbon)) return;  // ROSTER-GATE ribbon (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(ribbonPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.Draw(vertexCount);
            }

            void draw_arch(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.arch)) return;  // ROSTER-GATE arch (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(archPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount);
            }

            void draw_column(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.column || ROSTER.antenna)) return;  // ROSTER-GATE column+antenna (shared pipelines) (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(columnPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount);
            }

            void draw_palm(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.palm)) return;  // ROSTER-GATE palm (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(palmPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount);
            }

            void draw_cactus(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.cactus)) return;  // ROSTER-GATE cactus (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(cactusPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount);
            }

            void draw_blade(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.blade)) return;  // ROSTER-GATE blade (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(bladePipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount);
            }

            // draw_pyramid CUT — caller-free; pyramid mesh never drawn

            void draw_shell(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.indoor_shell)) return;  // ROSTER-GATE indoor_shell (a') — pipeline never created; the holder tolerates
                if (indexCount == 0) return;
                pass.SetPipeline(shellPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount);
            }

            void draw_gallery_frames(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup galleryEntityBindGroup,
                wgpu::BindGroup galleryTextureBindGroup,
                uint32_t activePaintingCount
            ) {
                if constexpr (!(ROSTER.gallery)) return;  // ROSTER-GATE gallery (a') — pipeline never created; the holder tolerates
                if (activePaintingCount == 0) return;
                pass.SetPipeline(galleryFramePipeline_);
                pass.SetBindGroup(0, galleryEntityBindGroup);
                pass.SetBindGroup(1, galleryTextureBindGroup);
                pass.Draw(Dim::PAINTING_QUAD_VERTS, Dim::PAINTING_MAX_SLOTS);
            }

            void draw_wall_paintings(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup galleryEntityBindGroup,
                wgpu::BindGroup galleryTextureBindGroup,
                uint32_t wallFrameCount
            ) {
                if constexpr (!(ROSTER.gallery)) return;  // ROSTER-GATE gallery (a') — pipeline never created; the holder tolerates
                if (wallFrameCount == 0) return;

                // Canvas pass (textured surface)
                pass.SetPipeline(wallPaintingCanvasPipeline_);
                pass.SetBindGroup(0, galleryEntityBindGroup);
                pass.SetBindGroup(1, galleryTextureBindGroup);
                pass.Draw(Dim::PAINTING_FRAME_VERTEX_COUNT);

                // Frame pass (solid color)
                pass.SetPipeline(wallPaintingFramePipeline_);
                pass.SetBindGroup(0, galleryEntityBindGroup);
                pass.SetBindGroup(1, galleryTextureBindGroup);
                pass.Draw(Dim::PAINTING_FRAME_VERTEX_COUNT);
            }

            void draw_fade_overlay(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup configBindGroup,
                float fadeAlpha
            ) {
                if constexpr (!(ROSTER.transitions)) return;  // ROSTER-GATE transitions (a') — pipeline never created; the holder tolerates
                if (fadeAlpha < 0.001f) return;
                pass.SetPipeline(fadeOverlayPipeline_);
                pass.SetBindGroup(0, configBindGroup);
                pass.Draw(3);  // fullscreen triangle from vertex ID
            }


            // Shared helper for all "indexed mesh" shadow draws. Per-family
            // wrappers below differ only in pipeline + (rarely) instance count.
            void draw_shadow_indexed_mesh(
                wgpu::RenderPassEncoder& pass,
                wgpu::RenderPipeline pipeline,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount,
                uint32_t instanceCount = 1,
                uint32_t firstInstance = 0
            ) {
                if (indexCount == 0) return;
                pass.SetPipeline(pipeline);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount, instanceCount, 0, 0, firstInstance);
            }

            void draw_shadow_patch_terrain(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount,
                uint32_t instanceCount
            ) {
                pass.SetPipeline(shadowPatchTerrainPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount, instanceCount);
            }

            void draw_shadow_pawn(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                uint32_t vertexCount
            ) {
                pass.SetPipeline(shadowPawnPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.Draw(vertexCount, /*instanceCount=*/ Dim::MAX_AGENTS);
            }

            void draw_shadow_sphere(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.sphere)) return;  // ROSTER-GATE sphere (a') — pipeline never created; the holder tolerates
                draw_shadow_indexed_mesh(pass, shadowSpherePipeline_,
                    entityBindGroup, textureBindGroup,
                    vertexBuffer, indexBuffer, indexCount,
                    Dim::MAX_SPHERE_INSTANCES);
            }

            void draw_shadow_monolith(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.cube)) return;  // ROSTER-GATE cube (a') — pipeline never created; the holder tolerates
                draw_shadow_indexed_mesh(pass, shadowMonolithPipeline_,
                    entityBindGroup, textureBindGroup,
                    vertexBuffer, indexBuffer, indexCount,
                    Dim::MAX_CUBE_INSTANCES, Dim::CUBE_SLOT_OFFSET);
            }

            void draw_shadow_ribbon(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                uint32_t vertexCount
            ) {
                if constexpr (!(ROSTER.ribbon)) return;  // ROSTER-GATE ribbon (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(shadowRibbonPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.Draw(vertexCount);
            }

            void draw_shadow_arch(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.arch)) return;  // ROSTER-GATE arch (a') — pipeline never created; the holder tolerates
                draw_shadow_indexed_mesh(pass, shadowArchPipeline_,
                    entityBindGroup, textureBindGroup,
                    vertexBuffer, indexBuffer, indexCount);
            }

            void draw_shadow_column(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.column || ROSTER.antenna)) return;  // ROSTER-GATE column+antenna (shared pipelines) (a') — pipeline never created; the holder tolerates
                draw_shadow_indexed_mesh(pass, shadowColumnPipeline_,
                    entityBindGroup, textureBindGroup,
                    vertexBuffer, indexBuffer, indexCount);
            }

            void draw_shadow_palm(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.palm)) return;  // ROSTER-GATE palm (a') — pipeline never created; the holder tolerates
                draw_shadow_indexed_mesh(pass, shadowPalmPipeline_,
                    entityBindGroup, textureBindGroup,
                    vertexBuffer, indexBuffer, indexCount);
            }

            void draw_shadow_cactus(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.cactus)) return;  // ROSTER-GATE cactus (a') — pipeline never created; the holder tolerates
                draw_shadow_indexed_mesh(pass, shadowCactusPipeline_,
                    entityBindGroup, textureBindGroup,
                    vertexBuffer, indexBuffer, indexCount);
            }

            void draw_shadow_blade(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.blade)) return;  // ROSTER-GATE blade (a') — pipeline never created; the holder tolerates
                draw_shadow_indexed_mesh(pass, shadowBladePipeline_,
                    entityBindGroup, textureBindGroup,
                    vertexBuffer, indexBuffer, indexCount);
            }

            // draw_shadow_pyramid CUT — caller-free

            void draw_shadow_shell(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.indoor_shell)) return;  // ROSTER-GATE indoor_shell (a') — pipeline never created; the holder tolerates
                // Helper's `if (indexCount == 0) return;` covers the early-out
                // that the original draw_shadow_shell had explicitly.
                draw_shadow_indexed_mesh(pass, shadowShellPipeline_,
                    entityBindGroup, textureBindGroup,
                    vertexBuffer, indexBuffer, indexCount);
            }

            // draw_shadow_wall_paintings + draw_shadow_gallery_frames CUT
            // — both caller-free (frames/paintings are drawn
            // in the color pass but never cast a mesh-shadow).

            // Gate (a'): compile-time count of pipelines the
            // selected demo skips — the boot summary's number.
            static constexpr uint32_t pipelines_skipped() {
                uint32_t n = 0;
                if (!(ROSTER.sphere)) n += 3;
                if (!(ROSTER.cube)) n += 3;
                if (!(ROSTER.ribbon)) n += 3;
                if (!(ROSTER.arch)) n += 3;
                if (!(ROSTER.column || ROSTER.antenna)) n += 3;
                if (!(ROSTER.palm)) n += 3;
                if (!(ROSTER.cactus)) n += 3;
                if (!(ROSTER.blade)) n += 3;
                // pyramid: 0 pipelines (mesh-gen + render + shadow all cut)
                if (!(ROSTER.gol)) n += 7;
                if (!(ROSTER.gallery)) n += 4;  // was 6; shadow_gallery_frame + shadow_wall_painting cut
                if (!(ROSTER.orbs)) n += 5;
                if (!(ROSTER.pawn_aura)) n += 1;
                if (!(ROSTER.indoor_shell)) n += 2;
                if (!(ROSTER.wanderers)) n += 1;
                if (!(ROSTER.transitions)) n += 1;
                return n;
            }

            bool reload() {
                if (!loadShader()) return false;
                if (!createComputePipelines()) return false;
                if (!createRenderPipelines()) return false;
                std::cout << "[Hot Reload] Shader reloaded successfully\n";
                return true;
            }

            const std::string& shader_path() const { return shaderPath_; }

        private:

            //
            // Reads world.wgsl from disk by searching a small list of known
            // relative paths (see the search loop below) — next to the executable
            // or one directory up. If the path moves, update the search list
            // rather than relying on cwd. The lookup logic is the contract.

            bool loadShader() {
                // On reload, use the already-known path instead of searching
                if (!shaderPath_.empty()) {
                    std::ifstream file(shaderPath_);
                    if (file.is_open()) {
                        std::stringstream buffer;
                        buffer << file.rdbuf();
                        shaderSource_ = buffer.str();
                        std::cout << "Reloaded shader from: " << shaderPath_ << "\n";
                    }
                    else {
                        std::cerr << "ERROR: Could not reload shader from: " << shaderPath_ << "\n";
                        return false;
                    }
                }
                else {
                    // First load: search for the shader. Array is sized to its
                    // contents — trailing nullptr slots would reach
                    // ifstream(nullptr) (a CRT assert dialog, not a readable
                    // error) exactly when the file is missing.
                    std::array<const char*, 2> paths = {
                        "../../../src/cartridges/the_board/realization/world.wgsl",
                        "src/cartridges/the_board/realization/world.wgsl",
                    };

                    const char* loadedPath = nullptr;
                    for (const char* path : paths) {
                        std::ifstream file(path);
                        if (file.is_open()) {
                            std::stringstream buffer;
                            buffer << file.rdbuf();
                            shaderSource_ = buffer.str();
                            shaderPath_ = path;
                            loadedPath = path;
                            break;
                        }
                    }

                    if (shaderSource_.empty()) {
                        std::cerr << "ERROR: Could not find shader. Tried:\n";
                        for (const char* path : paths) {
                            std::cerr << "  - " << path << "\n";
                        }
                        return false;
                    }

                    std::cout << "Loaded shader from: " << loadedPath << "\n";
                }

                wgpu::ShaderSourceWGSL wgslSource{};
                wgslSource.code = shaderSource_.c_str();

                wgpu::ShaderModuleDescriptor desc{};
                desc.nextInChain = &wgslSource;
                desc.label = "world.wgsl (The_Board Cartridge)";

                auto tShader0 = std::chrono::high_resolution_clock::now();
                shaderModule_ = device_.CreateShaderModule(&desc);
                auto tShader1 = std::chrono::high_resolution_clock::now();
                std::cout << "[Renderer] Shader compile:    "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(tShader1 - tShader0).count()
                    << " ms\n";
                return shaderModule_ != nullptr;
            }

            bool createComputePipelines() {
                // Shared pipeline layout for all standard compute passes (Group 0 only)
                std::array<wgpu::BindGroupLayout, 1> computeLayouts = {
                    computeEntityLayout_
                };

                wgpu::PipelineLayoutDescriptor layoutDesc{};
                layoutDesc.bindGroupLayoutCount = computeLayouts.size();
                layoutDesc.bindGroupLayouts = computeLayouts.data();
                wgpu::PipelineLayout computeLayout = device_.CreatePipelineLayout(&layoutDesc);
                if (!computeLayout) return false;

                // Shared pipeline layout for compute pipelines that evaluate
                // query_ground_flyer / query_ground_walker. Group 0 is the
                // same compute-entity layout; Group 1 adds the aura texture +
                // sampler needed by sample_pawn_aura on the compute path.
                // Used by update_sphere, update_cube (POLICY_FLYER),
                // update_player_agent (POLICY_WALKER), and update_other_agents
                // (POLICY_WALKER_AGENT — same texture binding for sample_pawn_aura).
                // Created here (before any pipeline that needs it) so the kernel
                // can reach it during behavior dispatch.
                std::array<wgpu::BindGroupLayout, 2> liveContribLayouts = {
                    computeEntityLayout_,
                    computeTextureLayout_
                };
                wgpu::PipelineLayoutDescriptor liveContribLayoutDesc{};
                liveContribLayoutDesc.bindGroupLayoutCount = liveContribLayouts.size();
                liveContribLayoutDesc.bindGroupLayouts = liveContribLayouts.data();
                wgpu::PipelineLayout liveContribComputeLayout =
                    device_.CreatePipelineLayout(&liveContribLayoutDesc);
                if (!liveContribComputeLayout) return false;

                // RAYMARCH/SDF EXCAVATION: update_terrain_config pipeline
                // removed (dead TerrainState writer kernel).

                // Pipeline 1b: update_player_agent (0D, 1 thread — possessed slot only)
                // Live-contributor layout — pawn_ground_resolve, terrain_normal_at
                // call query_ground_walker → contrib_pawn_aura_at → sample_pawn_aura.
                // The walker-policy heavy path inlines once, for one slot.
                if (!makeComputePipeline("update_player_agent", "Update Player Agent (0D, 1 thread)",
                    liveContribComputeLayout, Entry::UPDATE_PLAYER_AGENT, updatePlayerAgentPipeline_)) return false;

                // Pipeline 1c: update_other_agents (1D, 32 threads — non-possessed slots)
                // Live-contributor layout — query_ground_walker_agent reads aura
                // grid via contrib_pawn_aura_at_external → sample_pawn_aura.
                // The walker-policy heavy path is NOT inlined here; algorithmic
                // behaviors only.
                if constexpr (ROSTER.wanderers) {  // ROSTER-GATE wanderers (a') — FXC skipped when disabled
                if (!makeComputePipeline("update_other_agents", "Update Other Agents (1D, 32 threads)",
                    liveContribComputeLayout, Entry::UPDATE_OTHER_AGENTS, updateOtherAgentsPipeline_)) return false;
                }

                // Pipeline 1c: update_camera (0D)
                // Live-contributor layout — the camera clamp uses a walker-style
                // policy that reads the aura texture (sample_pawn_aura).
                if (!makeComputePipeline("update_camera", "Update Camera (0D)",
                    liveContribComputeLayout, Entry::UPDATE_CAMERA, updateCameraPipeline_)) return false;

                // Pipeline 1d: update_sphere (0D)
                // Uses the live-contributor layout so coupling_terrain_to_sphere_orbit_height
                // can call query_ground_flyer (→ contrib_pawn_aura_at → sample_pawn_aura).
                if constexpr (ROSTER.sphere) {  // ROSTER-GATE sphere (a') — FXC skipped when disabled
                if (!makeComputePipeline("update_sphere", "Update Sphere (0D)",
                    liveContribComputeLayout, Entry::UPDATE_SPHERE, updateSpherePipeline_)) return false;
                }

                // Pipeline 1e: update_cube (0D)
                // Same live-contributor layout — update_cube calls
                // query_ground_flyer directly for hover-base clearance.
                if constexpr (ROSTER.cube) {  // ROSTER-GATE cube (a') — FXC skipped when disabled
                if (!makeComputePipeline("update_cube", "Update Cube (0D)",
                    liveContribComputeLayout, Entry::UPDATE_CUBE, updateCubePipeline_)) return false;
                }

                // Pipeline 2: compute_vp (0D)
                if (!makeComputePipeline("compute_vp", "Compute VP Matrix (0D)",
                    computeLayout, Entry::COMPUTE_VP, computeVPPipeline_)) return false;

                // Pipeline 10: generate_terrain_indices (2D, one-shot)
                {
                    wgpu::PipelineLayout pl = computeLayoutFor(terrainIndexGenLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("gen_terrain_indices", "Generate Terrain Indices (2D, one-shot)",
                        pl, Entry::GENERATE_TERRAIN_INDICES, generateTerrainIndicesPipeline_)) return false;
                }

                // Pipeline 11a: generate_patch_heights (2D, pass 1 — heights only)
                {
                    wgpu::PipelineLayout pl = computeLayoutFor(patchGenLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("gen_patch_heights", "Generate Patch Heights (2D, pass 1)",
                        pl, Entry::GENERATE_PATCH_HEIGHTS, generatePatchHeightsPipeline_)) return false;
                }

                // Pipeline 11b: generate_patch_gradients (2D, pass 2 — gradients + complexity)
                {
                    wgpu::PipelineLayout pl = computeLayoutFor(patchGenLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("gen_patch_gradients", "Generate Patch Gradients (2D, pass 2)",
                        pl, Entry::GENERATE_PATCH_GRADIENTS, generatePatchGradientsPipeline_)) return false;
                }

                // Pipeline 12: generate_patch_cells (2D, on demand)
                {
                    wgpu::PipelineLayout pl = computeLayoutFor(patchGenLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("gen_patch_cells", "Generate Patch Cells (2D, on demand)",
                        pl, Entry::GENERATE_PATCH_CELLS, generatePatchCellsPipeline_)) return false;
                }

                // Pipeline 13: compute_ribbon_rings (1D, per frame when ribbon active)
                if constexpr (ROSTER.ribbon) {  // ROSTER-GATE ribbon (a') — FXC skipped when disabled
                    wgpu::PipelineLayout pl = computeLayoutFor(ribbonComputeLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("compute_ribbon_rings", "Compute Ribbon Rings (1D, per frame)",
                        pl, Entry::COMPUTE_RIBBON_RINGS, ribbonRingPipeline_)) return false;
                }

                // Photographer VP compute pipeline (0D, reads pawn → writes VP)
                if constexpr (ROSTER.gallery) {  // ROSTER-GATE gallery (a') — FXC skipped when disabled
                    wgpu::PipelineLayout pl = computeLayoutFor(photographerComputeLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("compute_photographer_vp", "Compute Photographer VP (0D)",
                        pl, Entry::COMPUTE_PHOTOGRAPHER_VP, photographerVPPipeline_)) return false;
                }

                // Entity placement Y-correction pipeline (0D, decoupled from photographer)
                {
                    // Placement gains Group 1 (compute textures): the card's
                    // cell-exact GoL fetch (GROUND_CARD_1 H5). Shared @group(1)
                    // declarations serve; unused group members are legal — the
                    // layout must cover the shader, not vice versa.
                    std::array<wgpu::BindGroupLayout, 2> placementLayouts = {
                        entityPlacementComputeLayout_,
                        computeTextureLayout_
                    };
                    wgpu::PipelineLayoutDescriptor pld{};
                    pld.bindGroupLayoutCount = placementLayouts.size();
                    pld.bindGroupLayouts = placementLayouts.data();
                    wgpu::PipelineLayout pl = device_.CreatePipelineLayout(&pld);
                    if (!pl) return false;
                    if (!makeComputePipeline("compute_entity_placement", "Compute Entity Placement (0D)",
                        pl, Entry::COMPUTE_ENTITY_PLACEMENT, entityPlacementPipeline_)) return false;
                }

                // GPU frustum cull pipeline (dedicated layout)
                {
                    wgpu::PipelineLayout pl = computeLayoutFor(frustumCullLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("frustum_cull_patches", "Frustum Cull Patches",
                        pl, Entry::FRUSTUM_CULL_PATCHES, frustumCullPipeline_)) return false;
                }

                // Pawn aura compute pipeline (dedicated layout)
                if constexpr (ROSTER.pawn_aura) {  // ROSTER-GATE pawn_aura (a') — FXC skipped when disabled
                    wgpu::PipelineLayout pl = computeLayoutFor(pawnAuraComputeLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("compute_pawn_aura", "Compute Pawn Aura (2D)",
                        pl, Entry::COMPUTE_PAWN_AURA, pawnAuraPipeline_)) return false;
                }

                // Live card writer pipelines (two-pass — TRUEBAND_CONTACT_1;
                // the patch-gen dispatch-pair shape at card size)
                {
                    wgpu::PipelineLayout pl = computeLayoutFor(liveCardWriterLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("write_live_card_heights", "Live Card Heights (2D)",
                        pl, Entry::WRITE_LIVE_CARD_HEIGHTS, liveCardHeightsPipeline_)) return false;
                    if (!makeComputePipeline("write_live_card_resolve", "Live Card Resolve (2D)",
                        pl, Entry::WRITE_LIVE_CARD_RESOLVE, liveCardResolvePipeline_)) return false;
                }

                // Orb compute pipelines (init + dynamics + recolor share the dedicated orb layout)
                if constexpr (ROSTER.orbs) {  // ROSTER-GATE orbs (a') — FXC skipped when disabled
                    wgpu::PipelineLayout pl = computeLayoutFor(orbComputeLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("orb_init", "Orb Init", pl, Entry::ORB_INIT, orbInitPipeline_)) return false;
                    if (!makeComputePipeline("orb_dynamics", "Orb Dynamics", pl, Entry::ORB_DYNAMICS, orbDynamicsPipeline_)) return false;
                    if (!makeComputePipeline("orb_recolor", "Orb Recolor", pl, Entry::ORB_RECOLOR, orbRecolorPipeline_)) return false;
                }

                // Orb copy-prev pipeline (Pass 9) — dedicated layout because
                // it flips the access modes on orb_state / orb_state_prev.
                if constexpr (ROSTER.orbs) {  // ROSTER-GATE orbs (a') — FXC skipped when disabled
                    wgpu::PipelineLayout pl = computeLayoutFor(orbCopyLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("orb_state_prev_copy", "Orb State Prev Copy",
                        pl, Entry::ORB_STATE_PREV_COPY, orbCopyPrevPipeline_)) return false;
                }

                // GoL zone compute pipelines (dedicated layout, z-dispatched)
                if constexpr (ROSTER.gol) {  // ROSTER-GATE gol (a') — FXC skipped when disabled
                    wgpu::PipelineLayout pl = computeLayoutFor(zoneGolComputeLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("zone_gol_sync", "GoL Zone Sync", pl, Entry::ZONE_GOL_SYNC, zoneGolSyncPipeline_)) return false;
                    if (!makeComputePipeline("zone_gol_evolve", "GoL Zone Evolve", pl, Entry::ZONE_GOL_EVOLVE, zoneGolEvolvePipeline_)) return false;
                }

                // Zone derive pipeline (shared GoL layout; mesh pair retired — UNIFIED_GROUND_1 U4)
                if constexpr (ROSTER.gol) {  // ROSTER-GATE gol (a') — FXC skipped when disabled
                    wgpu::PipelineLayout pl = computeLayoutFor(zoneGolComputeLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("zone_derive_params", "Zone Derive Params", pl, Entry::ZONE_DERIVE_PARAMS, zoneDeriveParamsPipeline_)) return false;
                }

                // Zone mask pipeline (dedicated layout — UNIFIED_GROUND_1 U5)
                if constexpr (ROSTER.gol) {  // ROSTER-GATE gol (a') — FXC skipped when disabled
                    wgpu::PipelineLayout pl = computeLayoutFor(zoneMaskLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("zone_seed_mask", "Zone Seed Mask (2D)",
                        pl, Entry::ZONE_SEED_MASK, zoneSeedMaskPipeline_)) return false;
                }

                // Mesh-gen compute pipelines — one dedicated single-group layout each,
                // per-family ROSTER gate; identical creation modulo (layout, entry, member).
                // pyramid mesh-gen pipeline CUT — mesh never drawn.

                if constexpr (ROSTER.arch) {  // ROSTER-GATE arch (a') — FXC skipped when disabled
                    wgpu::PipelineLayout pl = computeLayoutFor(archMeshGenLayout_);   // bindings 193-195
                    if (!pl) return false;
                    if (!makeComputePipeline("arch_mesh_gen", "Arch Mesh Gen",
                        pl, Entry::ARCH_MESH_GEN, archMeshGenPipeline_)) return false;
                }

                if constexpr (ROSTER.column || ROSTER.antenna) {  // ROSTER-GATE column+antenna (shared pipelines) (a') — FXC skipped when disabled
                    wgpu::PipelineLayout pl = computeLayoutFor(columnMeshGenLayout_);  // bindings 196-198
                    if (!pl) return false;
                    if (!makeComputePipeline("column_mesh_gen", "Column Mesh Gen",
                        pl, Entry::COLUMN_MESH_GEN, columnMeshGenPipeline_)) return false;
                }

                if constexpr (ROSTER.palm) {  // ROSTER-GATE palm (a') — FXC skipped when disabled
                    wgpu::PipelineLayout pl = computeLayoutFor(palmMeshGenLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("palm_mesh_gen", "Palm Mesh Gen",
                        pl, Entry::PALM_MESH_GEN, palmMeshGenPipeline_)) return false;
                }

                if constexpr (ROSTER.cactus) {  // ROSTER-GATE cactus (a') — FXC skipped when disabled
                    wgpu::PipelineLayout pl = computeLayoutFor(cactusMeshGenLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("cactus_mesh_gen", "Cactus Mesh Gen",
                        pl, Entry::CACTUS_MESH_GEN, cactusMeshGenPipeline_)) return false;
                }

                if constexpr (ROSTER.blade) {  // ROSTER-GATE blade (a') — FXC skipped when disabled
                    wgpu::PipelineLayout pl = computeLayoutFor(bladeMeshGenLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("blade_cluster_mesh_gen", "Blade Mesh Gen",
                        pl, Entry::BLADE_MESH_GEN, bladeMeshGenPipeline_)) return false;
                }

                return true;
            }

            bool createRenderPipelines() {
                // Shadow pipeline layout (entity + textures WITHOUT shadow map)
                std::array<wgpu::BindGroupLayout, 2> shadowLayouts = {
                    renderEntityLayout_,
                    shadowTextureLayout_
                };

                wgpu::PipelineLayoutDescriptor shadowLayoutDesc{};
                shadowLayoutDesc.bindGroupLayoutCount = shadowLayouts.size();
                shadowLayoutDesc.bindGroupLayouts = shadowLayouts.data();
                wgpu::PipelineLayout shadowRenderLayout = device_.CreatePipelineLayout(&shadowLayoutDesc);
                if (!shadowRenderLayout) return false;

                // Main render pipeline layout (entity + textures WITH shadow map)
                std::array<wgpu::BindGroupLayout, 2> renderLayouts = {
                    renderEntityLayout_,
                    renderTextureLayout_
                };

                wgpu::PipelineLayoutDescriptor layoutDesc{};
                layoutDesc.bindGroupLayoutCount = renderLayouts.size();
                layoutDesc.bindGroupLayouts = renderLayouts.data();
                wgpu::PipelineLayout renderLayout = device_.CreatePipelineLayout(&layoutDesc);
                if (!renderLayout) return false;

                // Shared depth stencil state
                wgpu::DepthStencilState depthStencil{};
                depthStencil.format = depthFormat_;
                depthStencil.depthWriteEnabled = true;
                depthStencil.depthCompare = wgpu::CompareFunction::Less;

                // Shared color target
                wgpu::ColorTargetState colorTarget{};
                colorTarget.format = colorFormat_;
                colorTarget.writeMask = wgpu::ColorWriteMask::All;

                // THE SHARED BUILDER (entity category): the builder every ENTITY_FS pipeline shares —
                // renderLayout + depthStencil + colorTarget + ENTITY_FS + TriangleList + CCW.
                // The genuine forks are parameters: the VS entry (passed VERBATIM), the
                // vertex-buffer layout (nullptr = bufferless, GPU-generated from vertex_index),
                // and cullMode — a REAL per-pipeline field, NOT noise: single-sided frond/
                // blade/column quads disable backface cull (None), solids keep Back. Same
                // shared desc the originals mutated in place, rebuilt fresh per call
                // (byte-identical result). Captures renderLayout/depthStencil/colorTarget.
                auto makeEntity = [&](const char* label, const char* dbgLabel, const char* vsEntry,
                                      const wgpu::VertexBufferLayout* vbl, wgpu::CullMode cull,
                                      wgpu::RenderPipeline& out) -> bool {
                    wgpu::FragmentState fragment{};
                    fragment.module = shaderModule_;
                    fragment.entryPoint = Entry::ENTITY_FS;
                    fragment.targetCount = 1;
                    fragment.targets = &colorTarget;

                    wgpu::RenderPipelineDescriptor desc{};
                    desc.label = dbgLabel;
                    desc.layout = renderLayout;
                    desc.vertex.module = shaderModule_;
                    desc.vertex.entryPoint = vsEntry;
                    desc.vertex.bufferCount = vbl ? 1u : 0u;
                    desc.vertex.buffers = vbl;
                    desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                    desc.primitive.cullMode = cull;
                    desc.primitive.frontFace = wgpu::FrontFace::CCW;
                    desc.depthStencil = &depthStencil;
                    desc.fragment = &fragment;
                    return tPipe(label, [&]() {
                        out = device_.CreateRenderPipeline(&desc);
                        return out != nullptr;
                    });
                };

                // Patch terrain pipeline -- instanced, no vertex buffer
                {
                    wgpu::FragmentState fragment{};
                    fragment.module = shaderModule_;
                    fragment.entryPoint = Entry::PATCH_TERRAIN_FS;
                    fragment.targetCount = 1;
                    fragment.targets = &colorTarget;

                    wgpu::RenderPipelineDescriptor desc{};
                    desc.label = "Patch Terrain (instanced)";
                    desc.layout = renderLayout;
                    desc.vertex.module = shaderModule_;
                    desc.vertex.entryPoint = Entry::PATCH_TERRAIN_VS;
                    desc.vertex.bufferCount = 0;
                    desc.vertex.buffers = nullptr;
                    desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                    desc.primitive.cullMode = wgpu::CullMode::Back;
                    desc.primitive.frontFace = wgpu::FrontFace::CCW;
                    desc.depthStencil = &depthStencil;
                    desc.fragment = &fragment;

                    if (!tPipe("patch_terrain", [&]() {
                        patchTerrainPipeline_ = device_.CreateRenderPipeline(&desc);
                        return patchTerrainPipeline_ != nullptr;
                    })) return false;
                }

                // Indirect terrain variant — USE_PATCH_INDIRECTION=true.
                // VS reads patch_instances[visible_patch_indices[instance_index]].
                {
                    wgpu::ConstantEntry overrides[1]{};
                    overrides[0].key = "USE_PATCH_INDIRECTION"; overrides[0].value = 1.0;

                    wgpu::FragmentState fragment{};
                    fragment.module = shaderModule_;
                    fragment.entryPoint = Entry::PATCH_TERRAIN_FS;
                    fragment.targetCount = 1;
                    fragment.targets = &colorTarget;

                    wgpu::RenderPipelineDescriptor desc{};
                    desc.label = "Patch Terrain Indirect (VS indirection)";
                    desc.layout = renderLayout;
                    desc.vertex.module = shaderModule_;
                    desc.vertex.entryPoint = Entry::PATCH_TERRAIN_VS;
                    desc.vertex.constantCount = 1;
                    desc.vertex.constants = overrides;
                    desc.vertex.bufferCount = 0;
                    desc.vertex.buffers = nullptr;
                    desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                    desc.primitive.cullMode = wgpu::CullMode::Back;
                    desc.primitive.frontFace = wgpu::FrontFace::CCW;
                    desc.depthStencil = &depthStencil;
                    desc.fragment = &fragment;

                    if (!tPipe("patch_terrain_indirect", [&]() {
                        patchTerrainIndirectPipeline_ = device_.CreateRenderPipeline(&desc);
                        return patchTerrainIndirectPipeline_ != nullptr;
                    })) return false;
                }

                // Zone cell extrusion pipeline (GoL alive cells with height)
                // (zone extrusion render pipeline RETIRED — UNIFIED_GROUND_1 U4)

                // Pawn pipeline -- chess pawn, GPU-generated from vertex_index (bufferless, cull None)
                if (!makeEntity("pawn", "Pawn Entity (Chess Pawn)", Entry::PAWN_VS,
                    nullptr, wgpu::CullMode::None, pawnPipeline_)) return false;

                // Sphere pipeline -- sphere entity, MeshVertex (pos+normal)
                {
                    std::array<wgpu::VertexAttribute, 2> meshAttrs{};
                    meshAttrs[0].format = wgpu::VertexFormat::Float32x3;
                    meshAttrs[0].offset = 0;
                    meshAttrs[0].shaderLocation = 0;
                    meshAttrs[1].format = wgpu::VertexFormat::Float32x3;
                    meshAttrs[1].offset = 12;
                    meshAttrs[1].shaderLocation = 1;

                    wgpu::VertexBufferLayout meshVBL{};
                    meshVBL.arrayStride = 24;
                    meshVBL.stepMode = wgpu::VertexStepMode::Vertex;
                    meshVBL.attributeCount = meshAttrs.size();
                    meshVBL.attributes = meshAttrs.data();

                    // Sphere + Monolith — same MeshVertex format, Back cull, differ only by VS.
                    if constexpr (ROSTER.sphere) {  // ROSTER-GATE sphere (a') — FXC skipped when disabled
                    if (!makeEntity("sphere", "Sphere Entity (Rasterized)", Entry::SPHERE_VS,
                        &meshVBL, wgpu::CullMode::Back, spherePipeline_)) return false;
                    }
                    if constexpr (ROSTER.cube) {  // ROSTER-GATE cube (a') — FXC skipped when disabled
                    if (!makeEntity("monolith", "Monolith Entity (Rasterized)", Entry::MONOLITH_VS,
                        &meshVBL, wgpu::CullMode::Back, monolithPipeline_)) return false;
                    }
                }

                // Arch pipeline -- catenary arch, ArchVertex (pos+normal+color+arch_index), static world-space
                {
                    std::array<wgpu::VertexAttribute, 4> archAttrs{};
                    archAttrs[0].format = wgpu::VertexFormat::Float32x3;
                    archAttrs[0].offset = 0;
                    archAttrs[0].shaderLocation = 0;
                    archAttrs[1].format = wgpu::VertexFormat::Float32x3;
                    archAttrs[1].offset = 12;
                    archAttrs[1].shaderLocation = 1;
                    archAttrs[2].format = wgpu::VertexFormat::Float32x3;
                    archAttrs[2].offset = 24;
                    archAttrs[2].shaderLocation = 2;
                    archAttrs[3].format = wgpu::VertexFormat::Float32;
                    archAttrs[3].offset = 36;
                    archAttrs[3].shaderLocation = 3;

                    wgpu::VertexBufferLayout archVBL{};
                    archVBL.arrayStride = 40;
                    archVBL.stepMode = wgpu::VertexStepMode::Vertex;
                    archVBL.attributeCount = archAttrs.size();
                    archVBL.attributes = archAttrs.data();

                    // Arch/column/palm/cactus/blade/pyramid — same ArchVertex format; differ by
                    // VS + cull. Single-sided column/palm/cactus/blade quads disable backface
                    // cull (None); arch + pyramid are solids (Back). (cullMode is a real fork.)
                    if constexpr (ROSTER.arch) {  // ROSTER-GATE arch (a') — FXC skipped when disabled
                    if (!makeEntity("arch", "Catenary Arch (Rasterized)", Entry::ARCH_VS,
                        &archVBL, wgpu::CullMode::Back, archPipeline_)) return false;
                    }
                    if constexpr (ROSTER.column || ROSTER.antenna) {  // ROSTER-GATE column+antenna (shared pipelines) (a') — FXC skipped when disabled
                    if (!makeEntity("column", "Generative Column (Rasterized)", Entry::COLUMN_VS,
                        &archVBL, wgpu::CullMode::None, columnPipeline_)) return false;
                    }
                    if constexpr (ROSTER.palm) {  // ROSTER-GATE palm (a') — FXC skipped when disabled
                    if (!makeEntity("palm", "Palm Tree (Rasterized)", Entry::PALM_VS,
                        &archVBL, wgpu::CullMode::None, palmPipeline_)) return false;
                    }
                    if constexpr (ROSTER.cactus) {  // ROSTER-GATE cactus (a') — FXC skipped when disabled
                    if (!makeEntity("cactus", "Cactus (Rasterized)", Entry::CACTUS_VS,
                        &archVBL, wgpu::CullMode::None, cactusPipeline_)) return false;
                    }
                    if constexpr (ROSTER.blade) {  // ROSTER-GATE blade (a') — FXC skipped when disabled
                    if (!makeEntity("blade", "Blade Cluster (Rasterized)", Entry::BLADE_VS,
                        &archVBL, wgpu::CullMode::None, bladePipeline_)) return false;
                    }
                    // pyramid render pipeline CUT — mesh never drawn
                }

                // Shell pipeline -- indoor ceiling + walls, ShellVertex (pos+normal+color), static world-space
                {
                    std::array<wgpu::VertexAttribute, 3> shellAttrs{};
                    shellAttrs[0].format = wgpu::VertexFormat::Float32x3;
                    shellAttrs[0].offset = 0;
                    shellAttrs[0].shaderLocation = 0;  // pos
                    shellAttrs[1].format = wgpu::VertexFormat::Float32x3;
                    shellAttrs[1].offset = 12;
                    shellAttrs[1].shaderLocation = 1;  // normal
                    shellAttrs[2].format = wgpu::VertexFormat::Float32x3;
                    shellAttrs[2].offset = 24;
                    shellAttrs[2].shaderLocation = 2;  // color

                    wgpu::VertexBufferLayout shellVBL{};
                    shellVBL.arrayStride = 36;
                    shellVBL.stepMode = wgpu::VertexStepMode::Vertex;
                    shellVBL.attributeCount = shellAttrs.size();
                    shellVBL.attributes = shellAttrs.data();

                    // ROSTER-GATE indoor_shell (a) — SEPARABLE: skip the shell
                    // pipeline creation when disabled. draw_shell self-gates on
                    // shell_index_count==0 (stays 0 — apply_mood_indoor_shell is
                    // (b)-gated), so the null pipeline is never bound.
                    // cull None: shell normals face inward (ceiling) + outward (walls).
                    if constexpr (ROSTER.indoor_shell) {  // ROSTER-GATE indoor_shell (a') — FXC skipped when disabled
                    if (!makeEntity("shell", "Indoor Shell (Ceiling + Walls)", Entry::SHELL_VS,
                        &shellVBL, wgpu::CullMode::None, shellPipeline_)) return false;
                    }
                }

                // Ribbon pipeline -- sky ribbon, GPU-generated cubes from vertex_index.
                // RIBBON_FS = entity shading with veil_scale 0 (the ruled fork: a
                // flown structure stays whole beyond the band).
                {
                    wgpu::FragmentState fragment{};
                    fragment.module = shaderModule_;
                    fragment.entryPoint = Entry::RIBBON_FS;
                    fragment.targetCount = 1;
                    fragment.targets = &colorTarget;

                    wgpu::RenderPipelineDescriptor desc{};
                    desc.label = "Sky Ribbon Entity";
                    desc.layout = renderLayout;
                    desc.vertex.module = shaderModule_;
                    desc.vertex.entryPoint = Entry::RIBBON_VS;
                    desc.vertex.bufferCount = 0;
                    desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                    desc.primitive.cullMode = wgpu::CullMode::None;
                    desc.primitive.frontFace = wgpu::FrontFace::CCW;
                    desc.depthStencil = &depthStencil;
                    desc.fragment = &fragment;

                    if constexpr (ROSTER.ribbon) {  // ROSTER-GATE ribbon (a') — FXC skipped when disabled
                    if (!tPipe("ribbon", [&]() {
                        ribbonPipeline_ = device_.CreateRenderPipeline(&desc);
                        return ribbonPipeline_ != nullptr;
                    })) return false;
                    }
                }

                // Orb pipeline -- billboarded glowing sprites, additive blended,
                // depth-tested but not depth-writing (transparent, stack safely).
                {
                    wgpu::VertexAttribute orbAttr{};
                    orbAttr.format = wgpu::VertexFormat::Float32x2;
                    orbAttr.offset = 0;
                    orbAttr.shaderLocation = 0;

                    wgpu::VertexBufferLayout orbVBL{};
                    orbVBL.arrayStride = 8;  // 2 × f32
                    orbVBL.stepMode = wgpu::VertexStepMode::Vertex;
                    orbVBL.attributeCount = 1;
                    orbVBL.attributes = &orbAttr;

                    // Additive blend (premultiplied alpha in FS: out.rgb = color*intensity).
                    wgpu::BlendState orbBlend{};
                    orbBlend.color.srcFactor = wgpu::BlendFactor::One;
                    orbBlend.color.dstFactor = wgpu::BlendFactor::One;
                    orbBlend.color.operation = wgpu::BlendOperation::Add;
                    orbBlend.alpha.srcFactor = wgpu::BlendFactor::One;
                    orbBlend.alpha.dstFactor = wgpu::BlendFactor::One;
                    orbBlend.alpha.operation = wgpu::BlendOperation::Add;

                    wgpu::ColorTargetState orbColorTarget{};
                    orbColorTarget.format = colorFormat_;
                    orbColorTarget.blend = &orbBlend;
                    orbColorTarget.writeMask = wgpu::ColorWriteMask::All;

                    wgpu::FragmentState fragment{};
                    fragment.module = shaderModule_;
                    fragment.entryPoint = Entry::ORB_FS;
                    fragment.targetCount = 1;
                    fragment.targets = &orbColorTarget;

                    // Depth: test yes, write no — orbs don't occlude each other
                    // or geometry behind them.
                    wgpu::DepthStencilState orbDepth{};
                    orbDepth.format = depthFormat_;
                    orbDepth.depthCompare = wgpu::CompareFunction::Less;
                    orbDepth.depthWriteEnabled = false;

                    wgpu::RenderPipelineDescriptor desc{};
                    desc.label = "Orb Sky Layer";
                    desc.layout = renderLayout;
                    desc.vertex.module = shaderModule_;
                    desc.vertex.entryPoint = Entry::ORB_VS;
                    desc.vertex.bufferCount = 1;
                    desc.vertex.buffers = &orbVBL;
                    desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                    desc.primitive.cullMode = wgpu::CullMode::None;  // billboards face camera
                    desc.primitive.frontFace = wgpu::FrontFace::CCW;
                    desc.depthStencil = &orbDepth;
                    desc.fragment = &fragment;

                    if constexpr (ROSTER.orbs) {  // ROSTER-GATE orbs (a') — FXC skipped when disabled
                    if (!tPipe("orb", [&]() {
                        orbRenderPipeline_ = device_.CreateRenderPipeline(&desc);
                        return orbRenderPipeline_ != nullptr;
                    })) return false;
                    }
                }

                // ─── Gallery Frame Pipeline ──────────────────────────────────────
                // Instanced subdivided quads textured with painting snapshots.
                // Dedicated pipeline layout (galleryEntity + galleryTexture).
                {
                    wgpu::PipelineLayoutDescriptor pld{};
                    std::array<wgpu::BindGroupLayout, 2> galleryGroups = {
                        galleryEntityLayout_, galleryTextureLayout_
                    };
                    pld.bindGroupLayoutCount = galleryGroups.size();
                    pld.bindGroupLayouts = galleryGroups.data();
                    wgpu::PipelineLayout galleryLayout = device_.CreatePipelineLayout(&pld);

                    wgpu::ColorTargetState colorTarget{};
                    colorTarget.format = colorFormat_;
                    colorTarget.writeMask = wgpu::ColorWriteMask::All;

                    wgpu::BlendState blend{};
                    blend.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
                    blend.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
                    blend.alpha.srcFactor = wgpu::BlendFactor::One;
                    blend.alpha.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
                    colorTarget.blend = &blend;

                    wgpu::FragmentState frag{};
                    frag.module = shaderModule_;
                    frag.entryPoint = Entry::GALLERY_FRAME_FS;
                    frag.targetCount = 1;
                    frag.targets = &colorTarget;

                    wgpu::DepthStencilState galleryDepth{};
                    galleryDepth.format = depthFormat_;
                    galleryDepth.depthWriteEnabled = true;
                    galleryDepth.depthCompare = wgpu::CompareFunction::Less;

                    wgpu::RenderPipelineDescriptor desc{};
                    desc.label = "Gallery Frame";
                    desc.layout = galleryLayout;
                    desc.vertex.module = shaderModule_;
                    desc.vertex.entryPoint = Entry::GALLERY_FRAME_VS;
                    desc.vertex.bufferCount = 0;
                    desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                    desc.primitive.cullMode = wgpu::CullMode::None;
                    desc.primitive.frontFace = wgpu::FrontFace::CCW;
                    desc.depthStencil = &galleryDepth;
                    desc.fragment = &frag;

                    if constexpr (ROSTER.gallery) {  // ROSTER-GATE gallery (a') — FXC skipped when disabled
                    if (!tPipe("gallery_frame", [&]() {
                        galleryFramePipeline_ = device_.CreateRenderPipeline(&desc);
                        return galleryFramePipeline_ != nullptr;
                    })) return false;
                    }

                    // Shadow Gallery Frame pipeline CUT — caller-free
                }

                // ─── Wall Painting Pipelines (framed paintings on indoor walls) ──
                // Uses same bind group layouts as gallery frames (galleryEntity + galleryTexture)
                {
                    wgpu::PipelineLayoutDescriptor pld{};
                    std::array<wgpu::BindGroupLayout, 2> wpGroups = {
                        galleryEntityLayout_, galleryTextureLayout_
                    };
                    pld.bindGroupLayoutCount = wpGroups.size();
                    pld.bindGroupLayouts = wpGroups.data();
                    wgpu::PipelineLayout wpLayout = device_.CreatePipelineLayout(&pld);

                    wgpu::ColorTargetState colorTarget{};
                    colorTarget.format = colorFormat_;
                    colorTarget.writeMask = wgpu::ColorWriteMask::All;

                    wgpu::DepthStencilState wpDepth{};
                    wpDepth.format = depthFormat_;
                    wpDepth.depthWriteEnabled = true;
                    wpDepth.depthCompare = wgpu::CompareFunction::Less;

                    // Canvas pipeline (textured)
                    {
                        wgpu::FragmentState frag{};
                        frag.module = shaderModule_;
                        frag.entryPoint = Entry::WALL_PAINTING_CANVAS_FS;
                        frag.targetCount = 1;
                        frag.targets = &colorTarget;

                        wgpu::RenderPipelineDescriptor desc{};
                        desc.label = "Wall Painting Canvas";
                        desc.layout = wpLayout;
                        desc.vertex.module = shaderModule_;
                        desc.vertex.entryPoint = Entry::WALL_PAINTING_VS;
                        desc.vertex.bufferCount = 0;
                        desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                        desc.primitive.cullMode = wgpu::CullMode::None;  // visible from both sides (outdoor monuments)
                        desc.primitive.frontFace = wgpu::FrontFace::CCW;
                        desc.depthStencil = &wpDepth;
                        desc.fragment = &frag;

                        if constexpr (ROSTER.gallery) {  // ROSTER-GATE gallery (a') — FXC skipped when disabled
                        if (!tPipe("wall_painting_canvas", [&]() {
                            wallPaintingCanvasPipeline_ = device_.CreateRenderPipeline(&desc);
                            return wallPaintingCanvasPipeline_ != nullptr;
                        })) return false;
                        }
                    }

                    // Frame pipeline (solid color)
                    {
                        wgpu::FragmentState frag{};
                        frag.module = shaderModule_;
                        frag.entryPoint = Entry::WALL_PAINTING_FRAME_FS;
                        frag.targetCount = 1;
                        frag.targets = &colorTarget;

                        wgpu::RenderPipelineDescriptor desc{};
                        desc.label = "Wall Painting Frame";
                        desc.layout = wpLayout;
                        desc.vertex.module = shaderModule_;
                        desc.vertex.entryPoint = Entry::WALL_PAINTING_VS;
                        desc.vertex.bufferCount = 0;
                        desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                        desc.primitive.cullMode = wgpu::CullMode::None;  // visible from both sides (outdoor monuments)
                        desc.primitive.frontFace = wgpu::FrontFace::CCW;
                        desc.depthStencil = &wpDepth;
                        desc.fragment = &frag;

                        if constexpr (ROSTER.gallery) {  // ROSTER-GATE gallery (a') — FXC skipped when disabled
                        if (!tPipe("wall_painting_frame", [&]() {
                            wallPaintingFramePipeline_ = device_.CreateRenderPipeline(&desc);
                            return wallPaintingFramePipeline_ != nullptr;
                        })) return false;
                        }
                    }

                    // Shadow Wall Painting pipeline CUT — caller-free
                }

                // ─── Shadow Pipelines (depth-only, Depth32Float) ─────────────────
                // Same bind group layouts as main render, but no fragment shader,
                // no color target, and shadow map depth format.
                {
                    wgpu::DepthStencilState shadowDepth{};
                    shadowDepth.format = wgpu::TextureFormat::Depth32Float;
                    shadowDepth.depthWriteEnabled = true;
                    shadowDepth.depthCompare = wgpu::CompareFunction::Less;

                    // THE SHARED BUILDER (shadow/depth category): the builder every shadow pipeline
                    // shares — shadowRenderLayout + shadowDepth (Depth32Float shadow map) +
                    // NO fragment (depth-only) + TriangleList + CCW. It is a SEPARATE builder
                    // from makeEntity (not one with an isShadow flag): color-vs-depth is a
                    // real category boundary (different layout, different depth state, no FS).
                    // Forks are parameters: shadow-VS (verbatim), VBL, cullMode — same
                    // Back/None split as the entity family (single-sided column/palm/cactus/
                    // blade + pawn/ribbon/shell → None; solids → Back).
                    auto makeShadow = [&](const char* label, const char* dbgLabel, const char* vsEntry,
                                          const wgpu::VertexBufferLayout* vbl, wgpu::CullMode cull,
                                          wgpu::RenderPipeline& out) -> bool {
                        wgpu::RenderPipelineDescriptor desc{};
                        desc.label = dbgLabel;
                        desc.layout = shadowRenderLayout;
                        desc.vertex.module = shaderModule_;
                        desc.vertex.entryPoint = vsEntry;
                        desc.vertex.bufferCount = vbl ? 1u : 0u;
                        desc.vertex.buffers = vbl;
                        desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                        desc.primitive.cullMode = cull;
                        desc.primitive.frontFace = wgpu::FrontFace::CCW;
                        desc.depthStencil = &shadowDepth;
                        desc.fragment = nullptr;
                        return tPipe(label, [&]() {
                            out = device_.CreateRenderPipeline(&desc);
                            return out != nullptr;
                        });
                    };

                    // MeshVertex layout (pos+normal) for sphere shadow
                    std::array<wgpu::VertexAttribute, 2> shadowMeshAttrs{};
                    shadowMeshAttrs[0].format = wgpu::VertexFormat::Float32x3;
                    shadowMeshAttrs[0].offset = 0;
                    shadowMeshAttrs[0].shaderLocation = 0;
                    shadowMeshAttrs[1].format = wgpu::VertexFormat::Float32x3;
                    shadowMeshAttrs[1].offset = 12;
                    shadowMeshAttrs[1].shaderLocation = 1;

                    wgpu::VertexBufferLayout shadowMeshVBL{};
                    shadowMeshVBL.arrayStride = 24;
                    shadowMeshVBL.stepMode = wgpu::VertexStepMode::Vertex;
                    shadowMeshVBL.attributeCount = shadowMeshAttrs.size();
                    shadowMeshVBL.attributes = shadowMeshAttrs.data();

                    // Shadow patch-terrain (bufferless, Back) + pawn (bufferless, None).
                    if (!makeShadow("shadow_patch_terrain", "Shadow Patch Terrain", Entry::SHADOW_PATCH_TERRAIN_VS,
                        nullptr, wgpu::CullMode::Back, shadowPatchTerrainPipeline_)) return false;
                    if (!makeShadow("shadow_pawn", "Shadow Pawn", Entry::SHADOW_PAWN_VS,
                        nullptr, wgpu::CullMode::None, shadowPawnPipeline_)) return false;

                    // Shadow sphere + monolith (MeshVertex, Back).
                    if constexpr (ROSTER.sphere) {  // ROSTER-GATE sphere (a') — FXC skipped when disabled
                    if (!makeShadow("shadow_sphere", "Shadow Sphere", Entry::SHADOW_SPHERE_VS,
                        &shadowMeshVBL, wgpu::CullMode::Back, shadowSpherePipeline_)) return false;
                    }
                    if constexpr (ROSTER.cube) {  // ROSTER-GATE cube (a') — FXC skipped when disabled
                    if (!makeShadow("shadow_monolith", "Shadow Monolith", Entry::SHADOW_MONOLITH_VS,
                        &shadowMeshVBL, wgpu::CullMode::Back, shadowMonolithPipeline_)) return false;
                    }

                    // Shadow Arch (ArchVertex buffer: pos+normal+color+arch_index, stride 40)
                    {
                        std::array<wgpu::VertexAttribute, 4> shadowArchAttrs{};
                        shadowArchAttrs[0].format = wgpu::VertexFormat::Float32x3;
                        shadowArchAttrs[0].offset = 0;
                        shadowArchAttrs[0].shaderLocation = 0;
                        shadowArchAttrs[1].format = wgpu::VertexFormat::Float32x3;
                        shadowArchAttrs[1].offset = 12;
                        shadowArchAttrs[1].shaderLocation = 1;
                        shadowArchAttrs[2].format = wgpu::VertexFormat::Float32x3;
                        shadowArchAttrs[2].offset = 24;
                        shadowArchAttrs[2].shaderLocation = 2;
                        shadowArchAttrs[3].format = wgpu::VertexFormat::Float32;
                        shadowArchAttrs[3].offset = 36;
                        shadowArchAttrs[3].shaderLocation = 3;

                        wgpu::VertexBufferLayout shadowArchVBL{};
                        shadowArchVBL.arrayStride = 40;
                        shadowArchVBL.stepMode = wgpu::VertexStepMode::Vertex;
                        shadowArchVBL.attributeCount = shadowArchAttrs.size();
                        shadowArchVBL.attributes = shadowArchAttrs.data();

                        // arch/column/palm/cactus/blade shadows — same ArchVertex
                        // format; cull matches the color pass (arch Back, the
                        // single-sided column/palm/cactus/blade None). pyramid shadow cut.
                        if constexpr (ROSTER.arch) {  // ROSTER-GATE arch (a') — FXC skipped when disabled
                        if (!makeShadow("shadow_arch", "Shadow Catenary Arch", Entry::SHADOW_ARCH_VS,
                            &shadowArchVBL, wgpu::CullMode::Back, shadowArchPipeline_)) return false;
                        }
                        if constexpr (ROSTER.column || ROSTER.antenna) {  // ROSTER-GATE column+antenna (shared pipelines) (a') — FXC skipped when disabled
                        if (!makeShadow("shadow_column", "Shadow Generative Column", Entry::SHADOW_COLUMN_VS,
                            &shadowArchVBL, wgpu::CullMode::None, shadowColumnPipeline_)) return false;
                        }
                        if constexpr (ROSTER.palm) {  // ROSTER-GATE palm (a') — FXC skipped when disabled
                        if (!makeShadow("shadow_palm", "Shadow Palm Tree", Entry::SHADOW_PALM_VS,
                            &shadowArchVBL, wgpu::CullMode::None, shadowPalmPipeline_)) return false;
                        }
                        if constexpr (ROSTER.cactus) {  // ROSTER-GATE cactus (a') — FXC skipped when disabled
                        if (!makeShadow("shadow_cactus", "Shadow Cactus", Entry::SHADOW_CACTUS_VS,
                            &shadowArchVBL, wgpu::CullMode::None, shadowCactusPipeline_)) return false;
                        }
                        if constexpr (ROSTER.blade) {  // ROSTER-GATE blade (a') — FXC skipped when disabled
                        if (!makeShadow("shadow_blade", "Shadow Blade Cluster", Entry::SHADOW_BLADE_VS,
                            &shadowArchVBL, wgpu::CullMode::None, shadowBladePipeline_)) return false;
                        }
                        // shadow_pyramid pipeline CUT — mesh never drawn
                    }

                    // Shadow Shell (ShellVertex: pos+normal+color, stride 36)
                    {
                        std::array<wgpu::VertexAttribute, 3> shadowShellAttrs{};
                        shadowShellAttrs[0].format = wgpu::VertexFormat::Float32x3;
                        shadowShellAttrs[0].offset = 0;
                        shadowShellAttrs[0].shaderLocation = 0;
                        shadowShellAttrs[1].format = wgpu::VertexFormat::Float32x3;
                        shadowShellAttrs[1].offset = 12;
                        shadowShellAttrs[1].shaderLocation = 1;
                        shadowShellAttrs[2].format = wgpu::VertexFormat::Float32x3;
                        shadowShellAttrs[2].offset = 24;
                        shadowShellAttrs[2].shaderLocation = 2;

                        wgpu::VertexBufferLayout shadowShellVBL{};
                        shadowShellVBL.arrayStride = 36;
                        shadowShellVBL.stepMode = wgpu::VertexStepMode::Vertex;
                        shadowShellVBL.attributeCount = shadowShellAttrs.size();
                        shadowShellVBL.attributes = shadowShellAttrs.data();

                        // ROSTER-GATE indoor_shell (a) — SEPARABLE: skip the
                        // shadow-shell pipeline too. draw_shadow_shell self-gates
                        // on count==0 (shared helper's early-out).
                        if constexpr (ROSTER.indoor_shell) {  // ROSTER-GATE indoor_shell (a') — FXC skipped when disabled
                        if (!makeShadow("shadow_shell", "Shadow Indoor Shell", Entry::SHADOW_SHELL_VS,
                            &shadowShellVBL, wgpu::CullMode::None, shadowShellPipeline_)) return false;
                        }
                    }

                    // Shadow ribbon (bufferless, GPU-generated from vertex_index; None).
                    if constexpr (ROSTER.ribbon) {  // ROSTER-GATE ribbon (a') — FXC skipped when disabled
                    if (!makeShadow("shadow_ribbon", "Shadow Sky Ribbon", Entry::SHADOW_RIBBON_VS,
                        nullptr, wgpu::CullMode::None, shadowRibbonPipeline_)) return false;
                    }

                    // Shadow Zone Extrusion (CellMeshVertex buffer, GoL zones)
                    {
                        std::array<wgpu::VertexAttribute, 4> attrs{};
                        attrs[0].format = wgpu::VertexFormat::Float32x3;
                        attrs[0].offset = 0;
                        attrs[0].shaderLocation = 0;   // pos
                        attrs[1].format = wgpu::VertexFormat::Float32x3;
                        attrs[1].offset = 12;
                        attrs[1].shaderLocation = 1;   // normal
                        attrs[2].format = wgpu::VertexFormat::Float32x2;
                        attrs[2].offset = 24;
                        attrs[2].shaderLocation = 2;   // uv
                        attrs[3].format = wgpu::VertexFormat::Float32x3;
                        attrs[3].offset = 32;
                        attrs[3].shaderLocation = 3;   // color

                        wgpu::VertexBufferLayout vbl{};
                        vbl.arrayStride = 44;
                        vbl.stepMode = wgpu::VertexStepMode::Vertex;
                        vbl.attributeCount = attrs.size();
                        vbl.attributes = attrs.data();

                        // (shadow zone extrusion RETIRED — A2_P2; vbl left unused)
                        (void)vbl;
                    }
                }

                // ─── Fade Overlay Pipeline ───────────────────────────────────────
                // Fullscreen triangle, alpha blending, no depth write.
                // Uses meshGenEntityLayout_ (binding 1 = config only).
                {
                    wgpu::PipelineLayoutDescriptor pld{};
                    std::array<wgpu::BindGroupLayout, 1> groups = { meshGenEntityLayout_ };
                    pld.bindGroupLayoutCount = groups.size();
                    pld.bindGroupLayouts = groups.data();
                    wgpu::PipelineLayout layout = device_.CreatePipelineLayout(&pld);

                    wgpu::ColorTargetState colorTarget{};
                    colorTarget.format = colorFormat_;
                    colorTarget.writeMask = wgpu::ColorWriteMask::All;

                    wgpu::BlendState blend{};
                    blend.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
                    blend.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
                    blend.alpha.srcFactor = wgpu::BlendFactor::One;
                    blend.alpha.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
                    colorTarget.blend = &blend;

                    wgpu::FragmentState frag{};
                    frag.module = shaderModule_;
                    frag.entryPoint = Entry::FADE_OVERLAY_FS;
                    frag.targetCount = 1;
                    frag.targets = &colorTarget;

                    wgpu::RenderPipelineDescriptor desc{};
                    desc.label = "Fade Overlay";
                    desc.layout = layout;
                    desc.vertex.module = shaderModule_;
                    desc.vertex.entryPoint = Entry::FADE_OVERLAY_VS;
                    desc.vertex.bufferCount = 0;
                    desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                    desc.fragment = &frag;

                    wgpu::DepthStencilState fadeDepth{};
                    fadeDepth.format = depthFormat_;
                    fadeDepth.depthWriteEnabled = false;
                    fadeDepth.depthCompare = wgpu::CompareFunction::Always;
                    desc.depthStencil = &fadeDepth;

                    if constexpr (ROSTER.transitions) {  // ROSTER-GATE transitions (a') — FXC skipped when disabled
                    if (!tPipe("fade_overlay", [&]() {
                        fadeOverlayPipeline_ = device_.CreateRenderPipeline(&desc);
                        return fadeOverlayPipeline_ != nullptr;
                    })) return false;
                    }
                }

                return true;
            }
        };

    } // namespace the_board
} // namespace t7