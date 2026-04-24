#pragma once

// THE_BOARD CARTRIDGE -- Renderer (Rasterized)
// ==================================================
//
// Pipeline management for patch-streaming rasterized rendering.
//
// COMPUTE PIPELINES:
//   Pipeline                    Dimension    Purpose
//   update_terrain_config       0D (1,1,1)   terrain amplitude from polyphony
//   update_agents               1D (1,1,1)×32 per-slot behavior dispatch (player / random_walk / ...)
//   update_camera               0D (1,1,1)   camera from input + pawn state
//   update_sphere               0D (1,1,1)   sphere orbit + color
//   compute_vp                  0D (1,1,1)   VP matrix from camera state
//   generate_terrain_indices    2D (32x32)   terrain index buffer (ONE-SHOT at init)
//   generate_patch_heights      2D (16x16)   per-patch heightfield pass 1 (heights)
//   generate_patch_gradients    2D (16x16)   per-patch heightfield pass 2 (gradients)
//   generate_patch_cells        2D (8x8)     per-patch cell color texture
//   compute_ribbon_rings        1D (64)      ribbon ring transforms
//   compute_pawn_aura           2D (8x8)     pawn aura spring grid
//   compute_photographer_vp     0D (1,1,1)   gallery camera VP (snapshot frames only)
//   compute_entity_placement    0D (1,1,1)   entity Y-correction (every frame)
//   zone_gol_sync               2D (8x8x8)  GoL zone life state sync
//   zone_gol_evolve             2D (8x8x8)  GoL zone evolution + spring
//   zone_gol_mesh_reset         0D (1,1,1)  zone mesh indirect reset
//   zone_gol_mesh_gen           2D (4x4x8)  zone extrusion mesh generation
//
// RENDER PIPELINES:
//   patch_terrain, zone_extrusion, pawn, sphere, arch, column, pyramid,
//   ribbon, gallery_frame + shadow variants for each
//
// See world.wgsl for the GPU shader (single source of truth).

#include "cartridges/the_board/state.hpp"
#include <webgpu/webgpu_cpp.h>
#include <string>
#include <fstream>
#include <sstream>
#include <array>
#include <chrono>
#include <iostream>

namespace t7 {
    namespace the_board {


        // =====================================================================
        // S1 ENTRY POINTS — Must match world.wgsl §7
        // =====================================================================

        namespace Entry {
            // Compute — split world update (ordered by dependency)
            constexpr const char* UPDATE_TERRAIN_CONFIG = "update_terrain_config";  // 0D
            constexpr const char* UPDATE_AGENTS = "update_agents";                  // 1D (32 threads, one per slot)
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

            // Render
            constexpr const char* PATCH_TERRAIN_VS = "patch_terrain_vs";
            constexpr const char* PATCH_TERRAIN_FS = "patch_terrain_fs";
            constexpr const char* SHADOW_PATCH_TERRAIN_VS = "shadow_patch_terrain_vs";
            constexpr const char* PAWN_VS = "pawn_vs";
            constexpr const char* SPHERE_VS = "sphere_vs";
            constexpr const char* MONOLITH_VS = "monolith_vs";
            constexpr const char* ENTITY_FS = "entity_fs";
            constexpr const char* SHADOW_PAWN_VS = "shadow_pawn_vs";
            constexpr const char* SHADOW_SPHERE_VS = "shadow_sphere_vs";
            constexpr const char* SHADOW_MONOLITH_VS = "shadow_monolith_vs";
            constexpr const char* RIBBON_VS = "ribbon_vs";
            constexpr const char* SHADOW_RIBBON_VS = "shadow_ribbon_vs";
            constexpr const char* ARCH_VS = "arch_vs";
            constexpr const char* SHADOW_ARCH_VS = "shadow_arch_vs";
            constexpr const char* COLUMN_VS = "column_vs";
            constexpr const char* SHADOW_COLUMN_VS = "shadow_column_vs";
            constexpr const char* PYRAMID_VS = "pyramid_vs";
            constexpr const char* SHADOW_PYRAMID_VS = "shadow_pyramid_vs";
            constexpr const char* SHELL_VS = "shell_vs";
            constexpr const char* SHADOW_SHELL_VS = "shadow_shell_vs";

            // Gallery (self-portrait painting frames)
            constexpr const char* GALLERY_FRAME_VS = "gallery_frame_vs";
            constexpr const char* GALLERY_FRAME_FS = "gallery_frame_fs";
            constexpr const char* SHADOW_GALLERY_FRAME_VS = "shadow_gallery_frame_vs";

            // Wall-mounted framed paintings (indoor)
            constexpr const char* WALL_PAINTING_VS        = "wall_painting_vs";
            constexpr const char* WALL_PAINTING_CANVAS_FS = "wall_painting_canvas_fs";
            constexpr const char* WALL_PAINTING_FRAME_FS  = "wall_painting_frame_fs";
            constexpr const char* SHADOW_WALL_PAINTING_VS = "shadow_wall_painting_vs";

            // Photographer compute (GPU-coupled snapshot camera)
            constexpr const char* COMPUTE_PHOTOGRAPHER_VP = "compute_photographer_vp";

            // Entity placement Y-correction (decoupled from photographer)
            constexpr const char* COMPUTE_ENTITY_PLACEMENT = "compute_entity_placement";

            // GPU frustum culling (every frame, after compute_vp)
            constexpr const char* FRUSTUM_CULL_PATCHES = "frustum_cull_patches";

            // GoL zone compute (zone-local automaton)
            constexpr const char* ZONE_GOL_SYNC = "zone_gol_sync";
            constexpr const char* ZONE_GOL_EVOLVE = "zone_gol_evolve";
            constexpr const char* ZONE_GOL_MESH_RESET = "zone_gol_mesh_reset";
            constexpr const char* ZONE_GOL_MESH_GEN = "zone_gol_mesh_gen";
            constexpr const char* ZONE_DERIVE_PARAMS = "zone_derive_params";

            // Zone extrusion rendering
            constexpr const char* ZONE_EXTRUSION_VS = "zone_extrusion_vs";
            constexpr const char* ZONE_EXTRUSION_FS = "zone_extrusion_fs";
            constexpr const char* SHADOW_ZONE_EXTRUSION_VS = "shadow_zone_extrusion_vs";

            // GPU Entity Mesh Gen (Phase 1: Pyramids, Phase 2: Arches, Phase 3: Columns)
            constexpr const char* PYRAMID_MESH_GEN = "pyramid_mesh_gen";
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


        // =====================================================================
        // S2-S7  RENDERER CLASS — Pipeline creation and dispatch
        // =====================================================================

        class Renderer {

            // =================================================================
            // S2 MEMBERS — Pipelines, layouts, shader module
            // =================================================================

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
            wgpu::BindGroupLayout zoneGolComputeLayout_;
            wgpu::BindGroupLayout zoneMeshGenLayout_;
            wgpu::BindGroupLayout pyramidMeshGenLayout_;
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

            // Compute pipelines -- per-frame (split world update)
            wgpu::ComputePipeline updateTerrainConfigPipeline_;  // 0D
            wgpu::ComputePipeline updateAgentsPipeline_;         // 1D (32 threads)
            wgpu::ComputePipeline updateCameraPipeline_;         // 0D
            wgpu::ComputePipeline updateSpherePipeline_;         // 0D
            wgpu::ComputePipeline updateCubePipeline_;           // 0D
            wgpu::ComputePipeline computeVPPipeline_;         // 0D

            // Legacy cell system — compiled but not dispatched in streaming mode.

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
            wgpu::RenderPipeline pyramidPipeline_;       // Generative pyramid entity
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
            wgpu::RenderPipeline shadowPyramidPipeline_;
            wgpu::RenderPipeline shadowShellPipeline_;

            // Patch terrain pipelines (instanced rendering)
            wgpu::RenderPipeline patchTerrainPipeline_;          // direct draw; all FS features compiled in
            wgpu::RenderPipeline patchTerrainIndirectPipeline_;  // same + USE_PATCH_INDIRECTION=true (for frustum cull)
            bool useIndirectTerrainPipeline_ = false;
            wgpu::RenderPipeline shadowPatchTerrainPipeline_;

            // Gallery frame pipeline (painting quads in the world)
            wgpu::RenderPipeline galleryFramePipeline_;
            wgpu::RenderPipeline shadowGalleryFramePipeline_;

            // Wall-mounted framed paintings (indoor) — uses galleryEntity + galleryTexture layouts
            wgpu::RenderPipeline wallPaintingCanvasPipeline_;
            wgpu::RenderPipeline wallPaintingFramePipeline_;
            wgpu::RenderPipeline shadowWallPaintingPipeline_;

            // Photographer VP compute pipeline (0D, GPU-coupled camera)
            wgpu::ComputePipeline photographerVPPipeline_;
            // Entity placement Y-correction pipeline (0D, decoupled from photographer)
            wgpu::ComputePipeline entityPlacementPipeline_;
            wgpu::ComputePipeline frustumCullPipeline_;
            wgpu::BindGroupLayout frustumCullLayout_;
            wgpu::BindGroupLayout entityPlacementComputeLayout_;
            wgpu::ComputePipeline pawnAuraPipeline_;

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
            wgpu::ComputePipeline zoneGolMeshResetPipeline_;
            wgpu::ComputePipeline zoneGolMeshGenPipeline_;
            wgpu::ComputePipeline zoneDeriveParamsPipeline_;

            // Zone extrusion render
            wgpu::RenderPipeline zoneExtrusionPipeline_;
            wgpu::RenderPipeline shadowZoneExtrusionPipeline_;

            // Fade overlay (fullscreen alpha-blended triangle)
            wgpu::RenderPipeline fadeOverlayPipeline_;

            // GPU entity mesh gen (Phase 1: pyramids, Phase 2: arches, Phase 3: columns)
            wgpu::ComputePipeline pyramidMeshGenPipeline_;
            wgpu::ComputePipeline archMeshGenPipeline_;
            wgpu::ComputePipeline columnMeshGenPipeline_;
            wgpu::ComputePipeline palmMeshGenPipeline_;
            wgpu::ComputePipeline cactusMeshGenPipeline_;
            wgpu::ComputePipeline bladeMeshGenPipeline_;


        public:

            // =================================================================
            // S3 IDENTITY — Non-copyable, default-constructible
            // =================================================================

            Renderer() = default;
            Renderer(const Renderer&) = delete;
            Renderer& operator=(const Renderer&) = delete;


            // =================================================================
            // S4 BOOT — Compile shaders and create pipelines from GPUState layouts
            // =================================================================

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
                orbComputeLayout_ = gpuState.orb_compute_layout();
                orbCopyLayout_    = gpuState.orb_copy_layout();
                zoneGolComputeLayout_ = gpuState.zone_gol_compute_layout();
                zoneMeshGenLayout_ = gpuState.zone_mesh_gen_layout();
                pyramidMeshGenLayout_ = gpuState.pyramid_mesh_gen_layout();
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

                std::cout << "[Renderer] Compute pipelines: "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms\n";
                std::cout << "[Renderer] Render pipelines:  "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " ms\n";
                std::cout << "[Renderer] Total pipelines:   "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t0).count() << " ms\n";

                return true;
            }


            // =================================================================
            // S5 COMPUTE DISPATCH — Ordered passes
            // =================================================================
            //
            // ORDER MATTERS: each pass may read what the previous pass wrote.
            //   1. update_terrain_config -- 0D (signal → terrain amplitude)
            //   2. update_agents         -- 1D (32 slots → behavior dispatch)
            //   3. update_camera         -- 0D (input/pawn → camera state)
            //   4. update_sphere         -- 0D (time/signal → sphere + tint)
            //   5. compute_vp            -- 0D VP matrix (reads camera, writes vp_data)

            void dispatch_update_terrain_config(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup entityBindGroup
            ) {
                pass.SetPipeline(updateTerrainConfigPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.DispatchWorkgroups(1, 1, 1);
            }

            void dispatch_update_agents(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup
            ) {
                pass.SetPipeline(updateAgentsPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);   // live-contributor textures (POLICY_WALKER)
                pass.DispatchWorkgroups(1, 1, 1);         // 1 workgroup × 32 threads = 32 slots
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

            // Patch heightfield generation -- on demand when patches enter active set.
            // Group 0 = patchGenBindGroup (params uniform), Group 1 = texture writes.

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
                pass.SetPipeline(ribbonRingPipeline_);
                pass.SetBindGroup(0, ribbonComputeBindGroup);
                pass.DispatchWorkgroups(workgroups, 1, 1);
            }

            void dispatch_compute_photographer_vp(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup photographerComputeBindGroup
            ) {
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
                // ceil(MAX_ACTIVE_PATCHES / 64) = ceil(225/64) = 4
                pass.DispatchWorkgroups(4, 1, 1);
            }

            void dispatch_compute_pawn_aura(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup auraComputeBindGroup,
                uint32_t workgroups
            ) {
                pass.SetPipeline(pawnAuraPipeline_);
                pass.SetBindGroup(0, auraComputeBindGroup);
                pass.DispatchWorkgroups(workgroups, workgroups, 1);
            }

            void dispatch_orb_init(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup orbComputeGroup,
                uint32_t workgroups
            ) {
                pass.SetPipeline(orbInitPipeline_);
                pass.SetBindGroup(0, orbComputeGroup);
                pass.DispatchWorkgroups(workgroups, 1, 1);
            }

            void dispatch_orb_dynamics(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup orbComputeGroup,
                uint32_t workgroups
            ) {
                pass.SetPipeline(orbDynamicsPipeline_);
                pass.SetBindGroup(0, orbComputeGroup);
                pass.DispatchWorkgroups(workgroups, 1, 1);
            }

            void dispatch_orb_recolor(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup orbComputeGroup,
                uint32_t workgroups
            ) {
                pass.SetPipeline(orbRecolorPipeline_);
                pass.SetBindGroup(0, orbComputeGroup);
                pass.DispatchWorkgroups(workgroups, 1, 1);
            }

            void dispatch_orb_copy_prev(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup orbCopyGroup,
                uint32_t workgroups
            ) {
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
                if (zone_count == 0) return;
                pass.SetPipeline(zoneGolEvolvePipeline_);
                pass.SetBindGroup(0, zoneComputeBindGroup);
                pass.DispatchWorkgroups(4, 4, zone_count);
            }

            // Zone mesh gen (single group — same layout as sync/evolve)
            void dispatch_zone_mesh_reset(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup meshGenGroup
            ) {
                pass.SetPipeline(zoneGolMeshResetPipeline_);
                pass.SetBindGroup(0, meshGenGroup);
                pass.DispatchWorkgroups(1, 1, 1);
            }

            void dispatch_zone_mesh_gen(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup meshGenGroup,
                uint32_t zone_count
            ) {
                if (zone_count == 0) return;
                pass.SetPipeline(zoneGolMeshGenPipeline_);
                pass.SetBindGroup(0, meshGenGroup);
                pass.DispatchWorkgroups(4, 4, zone_count);
            }

            // Zone parameter derivation (GPU-authoritative tier selection + Gaussian sampling)
            void dispatch_zone_derive_params(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup zoneGroup,
                uint32_t request_count
            ) {
                if (request_count == 0) return;
                pass.SetPipeline(zoneDeriveParamsPipeline_);
                pass.SetBindGroup(0, zoneGroup);
                pass.DispatchWorkgroups(request_count, 1, 1);
            }

            // GPU pyramid mesh gen — generates all 8 slots in one dispatch.
            // Inactive slots produce degenerate triangles (zero-area, free to rasterize).
            void dispatch_pyramid_mesh_gen(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup meshGenGroup
            ) {
                pass.SetPipeline(pyramidMeshGenPipeline_);
                pass.SetBindGroup(0, meshGenGroup);
                pass.DispatchWorkgroups(Dim::MAX_PYRAMID_INSTANCES, 1, 1);
            }

            // GPU arch mesh gen — generates all 16 slots × 4 sub-meshes.
            // gid.x = slot (0..15), gid.y = sub-mesh (outer shell, inner shell, front cap, back cap).
            void dispatch_arch_mesh_gen(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup meshGenGroup
            ) {
                pass.SetPipeline(archMeshGenPipeline_);
                pass.SetBindGroup(0, meshGenGroup);
                pass.DispatchWorkgroups(Dim::MAX_ARCH_INSTANCES, 4, 1);
            }

            // GPU column mesh gen — generates all 32 slots in one dispatch.
            void dispatch_column_mesh_gen(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup meshGenGroup
            ) {
                pass.SetPipeline(columnMeshGenPipeline_);
                pass.SetBindGroup(0, meshGenGroup);
                pass.DispatchWorkgroups(Dim::MAX_COLUMN_INSTANCES, 1, 1);
            }

            void dispatch_palm_mesh_gen(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup meshGenGroup
            ) {
                pass.SetPipeline(palmMeshGenPipeline_);
                pass.SetBindGroup(0, meshGenGroup);
                pass.DispatchWorkgroups(Dim::MAX_PALM_INSTANCES, 1, 1);
            }

            void dispatch_cactus_mesh_gen(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup meshGenGroup
            ) {
                pass.SetPipeline(cactusMeshGenPipeline_);
                pass.SetBindGroup(0, meshGenGroup);
                pass.DispatchWorkgroups(Dim::MAX_CACTUS_INSTANCES, 1, 1);
            }

            void dispatch_blade_mesh_gen(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup group
            ) {
                pass.SetPipeline(bladeMeshGenPipeline_);
                pass.SetBindGroup(0, group);
                pass.DispatchWorkgroups(Dim::MAX_BLADE_INSTANCES, 1, 1);
            }

            // Zone extrusion rendering
            void draw_zone_extrusion(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                wgpu::Buffer indirectBuffer
            ) {
                pass.SetPipeline(zoneExtrusionPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexedIndirect(indirectBuffer, 0);
            }

            void draw_shadow_zone_extrusion(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                wgpu::Buffer indirectBuffer
            ) {
                pass.SetPipeline(shadowZoneExtrusionPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexedIndirect(indirectBuffer, 0);
            }


            // =================================================================
            // S6 DRAW — Main pass and shadow pass draw calls
            // =================================================================
            //
            // Four-layer rasterized rendering with shared depth buffer.
            // Draw order: terrain -> cell extrusion -> pawn -> sphere


            // GPU frustum-culled LOD0 terrain draw — single DrawIndexedIndirect.
            // (Dawn D3D12 limit: only one indirect draw per render pass.)
            // LOD1 must be drawn separately via draw_patch_terrain_lod1_direct.
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
                pass.Draw(vertexCount);
            }

            void draw_sphere(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
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
                pass.SetPipeline(bladePipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount);
            }

            void draw_pyramid(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                pass.SetPipeline(pyramidPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount);
            }

            void draw_shell(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
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
                if (fadeAlpha < 0.001f) return;
                pass.SetPipeline(fadeOverlayPipeline_);
                pass.SetBindGroup(0, configBindGroup);
                pass.Draw(3);  // fullscreen triangle from vertex ID
            }


            // -----------------------------------------------------------------
            // Shadow pass draw methods (depth-only)
            // -----------------------------------------------------------------


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
                pass.Draw(vertexCount);
            }

            void draw_shadow_sphere(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                pass.SetPipeline(shadowSpherePipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount, Dim::MAX_SPHERE_INSTANCES);
            }

            void draw_shadow_monolith(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                pass.SetPipeline(shadowMonolithPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount, Dim::MAX_CUBE_INSTANCES, 0, 0, Dim::CUBE_SLOT_OFFSET);
            }

            void draw_shadow_ribbon(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                uint32_t vertexCount
            ) {
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
                pass.SetPipeline(shadowArchPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount);
            }

            void draw_shadow_column(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                pass.SetPipeline(shadowColumnPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount);
            }

            void draw_shadow_palm(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                pass.SetPipeline(shadowPalmPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount);
            }

            void draw_shadow_cactus(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                pass.SetPipeline(shadowCactusPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount);
            }

            void draw_shadow_blade(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                pass.SetPipeline(shadowBladePipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount);
            }

            void draw_shadow_pyramid(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                pass.SetPipeline(shadowPyramidPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount);
            }

            void draw_shadow_shell(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup entityBindGroup,
                wgpu::BindGroup textureBindGroup,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if (indexCount == 0) return;
                pass.SetPipeline(shadowShellPipeline_);
                pass.SetBindGroup(0, entityBindGroup);
                pass.SetBindGroup(1, textureBindGroup);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount);
            }

            void draw_shadow_wall_paintings(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup galleryEntityBindGroup,
                wgpu::BindGroup galleryTextureBindGroup,
                uint32_t wallFrameCount
            ) {
                if (wallFrameCount == 0) return;
                pass.SetPipeline(shadowWallPaintingPipeline_);
                pass.SetBindGroup(0, galleryEntityBindGroup);
                pass.SetBindGroup(1, galleryTextureBindGroup);
                pass.Draw(Dim::PAINTING_FRAME_VERTEX_COUNT);
            }

            void draw_shadow_gallery_frames(
                wgpu::RenderPassEncoder& pass,
                wgpu::BindGroup galleryEntityBindGroup,
                wgpu::BindGroup galleryTextureBindGroup,
                uint32_t activePaintingCount
            ) {
                if (activePaintingCount == 0) return;
                pass.SetPipeline(shadowGalleryFramePipeline_);
                pass.SetBindGroup(0, galleryEntityBindGroup);
                pass.SetBindGroup(1, galleryTextureBindGroup);
                pass.Draw(Dim::PAINTING_QUAD_VERTS, Dim::PAINTING_MAX_SLOTS);
            }


            // =================================================================
            // S7 HOT RELOAD — Recompile shaders without restart
            // =================================================================

            bool reload() {
                if (!loadShader()) return false;
                if (!createComputePipelines()) return false;
                if (!createRenderPipelines()) return false;
                std::cout << "[Hot Reload] Shader reloaded successfully\n";
                return true;
            }

            const std::string& shader_path() const { return shaderPath_; }


        private:

            // =================================================================
            // S4 BOOT (continued) — Private pipeline creation
            // =================================================================

            // -----------------------------------------------------------------
            // Shader loading
            // -----------------------------------------------------------------

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
                    // First load: search for the shader
                    std::array<const char*, 6> paths = {
                        "../../../src/cartridges/the_board/world.wgsl",
                        "src/cartridges/the_board/world.wgsl",
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

            // -----------------------------------------------------------------
            // Compute pipeline creation
            // -----------------------------------------------------------------

            bool createComputePipelines() {
                auto tPipe = [](const char* label, auto fn) -> bool {
                    auto t0 = std::chrono::high_resolution_clock::now();
                    bool ok = fn();
                    auto t1 = std::chrono::high_resolution_clock::now();
                    std::cout << "  [Pipeline] " << label << ": "
                        << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count()
                        << " ms\n";
                    return ok;
                    };

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
                // Used by update_sphere, update_cube (POLICY_FLYER) and
                // update_agents (POLICY_WALKER inside behavior_player_controlled).
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

                // Pipeline 1a: update_terrain_config (0D)
                if (!tPipe("update_terrain_config", [&]() {
                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Update Terrain Config (0D)";
                    desc.layout = computeLayout;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::UPDATE_TERRAIN_CONFIG;
                    updateTerrainConfigPipeline_ = device_.CreateComputePipeline(&desc);
                    return updateTerrainConfigPipeline_ != nullptr;
                    })) return false;

                // Pipeline 1b: update_agents (1D, 32 threads — one per agent slot)
                // Live-contributor layout — pawn_ground_resolve, terrain_normal_at
                // call query_ground_walker → contrib_pawn_aura_at → sample_pawn_aura.
                // The PlayerControlled branch runs the heavy walker-policy path;
                // RandomWalk and other behaviors are stubbed in Pass 1.
                if (!tPipe("update_agents", [&]() {
                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Update Agents (1D, 32 threads)";
                    desc.layout = liveContribComputeLayout;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::UPDATE_AGENTS;
                    updateAgentsPipeline_ = device_.CreateComputePipeline(&desc);
                    return updateAgentsPipeline_ != nullptr;
                    })) return false;

                // Pipeline 1c: update_camera (0D)
                // Live-contributor layout — the camera clamp now uses
                // POLICY_FLYER (sphere/cube parity) so it clears
                // aura-lifted and pulse-lifted ground.
                if (!tPipe("update_camera", [&]() {
                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Update Camera (0D)";
                    desc.layout = liveContribComputeLayout;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::UPDATE_CAMERA;
                    updateCameraPipeline_ = device_.CreateComputePipeline(&desc);
                    return updateCameraPipeline_ != nullptr;
                    })) return false;

                // Pipeline 1d: update_sphere (0D)
                // Uses the live-contributor layout so coupling_terrain_to_sphere_orbit_height
                // can call query_ground_flyer (→ contrib_pawn_aura_at → sample_pawn_aura).
                if (!tPipe("update_sphere", [&]() {
                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Update Sphere (0D)";
                    desc.layout = liveContribComputeLayout;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::UPDATE_SPHERE;
                    updateSpherePipeline_ = device_.CreateComputePipeline(&desc);
                    return updateSpherePipeline_ != nullptr;
                    })) return false;

                // Pipeline 1e: update_cube (0D)
                // Same live-contributor layout — update_cube calls
                // query_ground_flyer directly for hover-base clearance.
                if (!tPipe("update_cube", [&]() {
                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Update Cube (0D)";
                    desc.layout = liveContribComputeLayout;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::UPDATE_CUBE;
                    updateCubePipeline_ = device_.CreateComputePipeline(&desc);
                    return updateCubePipeline_ != nullptr;
                    })) return false;

                // Pipeline 2: compute_vp (0D)
                if (!tPipe("compute_vp", [&]() {
                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Compute VP Matrix (0D)";
                    desc.layout = computeLayout;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::COMPUTE_VP;
                    computeVPPipeline_ = device_.CreateComputePipeline(&desc);
                    return computeVPPipeline_ != nullptr;
                    })) return false;



                // Pipeline 10: generate_terrain_indices (2D, one-shot)
                if (!tPipe("gen_terrain_indices", [&]() {
                    std::array<wgpu::BindGroupLayout, 1> tigLayouts = {
                        terrainIndexGenLayout_
                    };

                    wgpu::PipelineLayoutDescriptor tigLayoutDesc{};
                    tigLayoutDesc.bindGroupLayoutCount = tigLayouts.size();
                    tigLayoutDesc.bindGroupLayouts = tigLayouts.data();
                    wgpu::PipelineLayout tigPipelineLayout = device_.CreatePipelineLayout(&tigLayoutDesc);
                    if (!tigPipelineLayout) return false;

                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Generate Terrain Indices (2D, one-shot)";
                    desc.layout = tigPipelineLayout;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::GENERATE_TERRAIN_INDICES;
                    generateTerrainIndicesPipeline_ = device_.CreateComputePipeline(&desc);
                    return generateTerrainIndicesPipeline_ != nullptr;
                    })) return false;

                // Pipeline 11a: generate_patch_heights (2D, pass 1 — heights only)
                if (!tPipe("gen_patch_heights", [&]() {
                    std::array<wgpu::BindGroupLayout, 1> patchLayouts = {
                        patchGenLayout_
                    };

                    wgpu::PipelineLayoutDescriptor patchLayoutDesc{};
                    patchLayoutDesc.bindGroupLayoutCount = patchLayouts.size();
                    patchLayoutDesc.bindGroupLayouts = patchLayouts.data();
                    wgpu::PipelineLayout patchPipelineLayout = device_.CreatePipelineLayout(&patchLayoutDesc);
                    if (!patchPipelineLayout) return false;

                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Generate Patch Heights (2D, pass 1)";
                    desc.layout = patchPipelineLayout;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::GENERATE_PATCH_HEIGHTS;
                    generatePatchHeightsPipeline_ = device_.CreateComputePipeline(&desc);
                    return generatePatchHeightsPipeline_ != nullptr;
                    })) return false;

                // Pipeline 11b: generate_patch_gradients (2D, pass 2 — gradients + complexity)
                if (!tPipe("gen_patch_gradients", [&]() {
                    std::array<wgpu::BindGroupLayout, 1> patchLayouts = {
                        patchGenLayout_
                    };

                    wgpu::PipelineLayoutDescriptor patchLayoutDesc{};
                    patchLayoutDesc.bindGroupLayoutCount = patchLayouts.size();
                    patchLayoutDesc.bindGroupLayouts = patchLayouts.data();
                    wgpu::PipelineLayout patchPipelineLayout = device_.CreatePipelineLayout(&patchLayoutDesc);
                    if (!patchPipelineLayout) return false;

                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Generate Patch Gradients (2D, pass 2)";
                    desc.layout = patchPipelineLayout;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::GENERATE_PATCH_GRADIENTS;
                    generatePatchGradientsPipeline_ = device_.CreateComputePipeline(&desc);
                    return generatePatchGradientsPipeline_ != nullptr;
                    })) return false;

                // Pipeline 12: generate_patch_cells (2D, on demand)
                if (!tPipe("gen_patch_cells", [&]() {
                    std::array<wgpu::BindGroupLayout, 1> patchLayouts = {
                        patchGenLayout_
                    };

                    wgpu::PipelineLayoutDescriptor patchLayoutDesc{};
                    patchLayoutDesc.bindGroupLayoutCount = patchLayouts.size();
                    patchLayoutDesc.bindGroupLayouts = patchLayouts.data();
                    wgpu::PipelineLayout patchPipelineLayout = device_.CreatePipelineLayout(&patchLayoutDesc);
                    if (!patchPipelineLayout) return false;

                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Generate Patch Cells (2D, on demand)";
                    desc.layout = patchPipelineLayout;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::GENERATE_PATCH_CELLS;
                    generatePatchCellsPipeline_ = device_.CreateComputePipeline(&desc);
                    return generatePatchCellsPipeline_ != nullptr;
                    })) return false;

                // Pipeline 13: compute_ribbon_rings (1D, per frame when ribbon active)
                if (!tPipe("compute_ribbon_rings", [&]() {
                    std::array<wgpu::BindGroupLayout, 1> rcLayouts = {
                        ribbonComputeLayout_
                    };

                    wgpu::PipelineLayoutDescriptor rcLayoutDesc{};
                    rcLayoutDesc.bindGroupLayoutCount = rcLayouts.size();
                    rcLayoutDesc.bindGroupLayouts = rcLayouts.data();
                    wgpu::PipelineLayout rcPipelineLayout = device_.CreatePipelineLayout(&rcLayoutDesc);
                    if (!rcPipelineLayout) return false;

                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Compute Ribbon Rings (1D, per frame)";
                    desc.layout = rcPipelineLayout;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::COMPUTE_RIBBON_RINGS;
                    ribbonRingPipeline_ = device_.CreateComputePipeline(&desc);
                    return ribbonRingPipeline_ != nullptr;
                    })) return false;

                // Photographer VP compute pipeline (0D, reads pawn → writes VP)
                if (!tPipe("compute_photographer_vp", [&]() {
                    std::array<wgpu::BindGroupLayout, 1> layouts = { photographerComputeLayout_ };
                    wgpu::PipelineLayoutDescriptor pld{};
                    pld.bindGroupLayoutCount = layouts.size();
                    pld.bindGroupLayouts = layouts.data();
                    wgpu::PipelineLayout pl = device_.CreatePipelineLayout(&pld);
                    if (!pl) return false;

                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Compute Photographer VP (0D)";
                    desc.layout = pl;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::COMPUTE_PHOTOGRAPHER_VP;
                    photographerVPPipeline_ = device_.CreateComputePipeline(&desc);
                    return photographerVPPipeline_ != nullptr;
                    })) return false;

                // Entity placement Y-correction pipeline (0D, decoupled from photographer)
                if (!tPipe("compute_entity_placement", [&]() {
                    std::array<wgpu::BindGroupLayout, 1> layouts = { entityPlacementComputeLayout_ };
                    wgpu::PipelineLayoutDescriptor pld{};
                    pld.bindGroupLayoutCount = layouts.size();
                    pld.bindGroupLayouts = layouts.data();
                    wgpu::PipelineLayout pl = device_.CreatePipelineLayout(&pld);
                    if (!pl) return false;

                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Compute Entity Placement (0D)";
                    desc.layout = pl;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::COMPUTE_ENTITY_PLACEMENT;
                    entityPlacementPipeline_ = device_.CreateComputePipeline(&desc);
                    return entityPlacementPipeline_ != nullptr;
                    })) return false;

                // GPU frustum cull pipeline (dedicated layout)
                {
                    std::array<wgpu::BindGroupLayout, 1> layouts = { frustumCullLayout_ };
                    wgpu::PipelineLayoutDescriptor pld{};
                    pld.bindGroupLayoutCount = layouts.size();
                    pld.bindGroupLayouts = layouts.data();
                    wgpu::PipelineLayout pl = device_.CreatePipelineLayout(&pld);
                    if (!pl) return false;

                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Frustum Cull Patches";
                    desc.layout = pl;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::FRUSTUM_CULL_PATCHES;
                    frustumCullPipeline_ = device_.CreateComputePipeline(&desc);
                    if (!frustumCullPipeline_) return false;
                }

                // Pawn aura compute pipeline (dedicated layout)
                {
                    std::array<wgpu::BindGroupLayout, 1> layouts = { pawnAuraComputeLayout_ };
                    wgpu::PipelineLayoutDescriptor pld{};
                    pld.bindGroupLayoutCount = layouts.size();
                    pld.bindGroupLayouts = layouts.data();
                    wgpu::PipelineLayout pl = device_.CreatePipelineLayout(&pld);
                    if (!pl) return false;

                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Compute Pawn Aura (2D)";
                    desc.layout = pl;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::COMPUTE_PAWN_AURA;
                    pawnAuraPipeline_ = device_.CreateComputePipeline(&desc);
                    if (!pawnAuraPipeline_) return false;
                }

                // Orb compute pipelines (init + dynamics share the dedicated orb layout)
                {
                    std::array<wgpu::BindGroupLayout, 1> layouts = { orbComputeLayout_ };
                    wgpu::PipelineLayoutDescriptor pld{};
                    pld.bindGroupLayoutCount = layouts.size();
                    pld.bindGroupLayouts = layouts.data();
                    wgpu::PipelineLayout pl = device_.CreatePipelineLayout(&pld);
                    if (!pl) return false;

                    wgpu::ComputePipelineDescriptor desc{};
                    desc.layout = pl;
                    desc.compute.module = shaderModule_;

                    desc.label = "Orb Init";
                    desc.compute.entryPoint = Entry::ORB_INIT;
                    orbInitPipeline_ = device_.CreateComputePipeline(&desc);
                    if (!orbInitPipeline_) return false;

                    desc.label = "Orb Dynamics";
                    desc.compute.entryPoint = Entry::ORB_DYNAMICS;
                    orbDynamicsPipeline_ = device_.CreateComputePipeline(&desc);
                    if (!orbDynamicsPipeline_) return false;

                    desc.label = "Orb Recolor";
                    desc.compute.entryPoint = Entry::ORB_RECOLOR;
                    orbRecolorPipeline_ = device_.CreateComputePipeline(&desc);
                    if (!orbRecolorPipeline_) return false;
                }

                // Orb copy-prev pipeline (Pass 9) — dedicated layout because
                // it flips the access modes on orb_state / orb_state_prev.
                {
                    std::array<wgpu::BindGroupLayout, 1> layouts = { orbCopyLayout_ };
                    wgpu::PipelineLayoutDescriptor pld{};
                    pld.bindGroupLayoutCount = layouts.size();
                    pld.bindGroupLayouts = layouts.data();
                    wgpu::PipelineLayout pl = device_.CreatePipelineLayout(&pld);
                    if (!pl) return false;

                    wgpu::ComputePipelineDescriptor desc{};
                    desc.layout = pl;
                    desc.compute.module = shaderModule_;
                    desc.label = "Orb State Prev Copy";
                    desc.compute.entryPoint = Entry::ORB_STATE_PREV_COPY;
                    orbCopyPrevPipeline_ = device_.CreateComputePipeline(&desc);
                    if (!orbCopyPrevPipeline_) return false;
                }

                // GoL zone compute pipelines (dedicated layout, z-dispatched)
                {
                    std::array<wgpu::BindGroupLayout, 1> layouts = { zoneGolComputeLayout_ };
                    wgpu::PipelineLayoutDescriptor pld{};
                    pld.bindGroupLayoutCount = layouts.size();
                    pld.bindGroupLayouts = layouts.data();
                    wgpu::PipelineLayout pl = device_.CreatePipelineLayout(&pld);
                    if (!pl) return false;

                    wgpu::ComputePipelineDescriptor desc{};
                    desc.layout = pl;
                    desc.compute.module = shaderModule_;

                    desc.label = "GoL Zone Sync";
                    desc.compute.entryPoint = Entry::ZONE_GOL_SYNC;
                    zoneGolSyncPipeline_ = device_.CreateComputePipeline(&desc);
                    if (!zoneGolSyncPipeline_) return false;

                    desc.label = "GoL Zone Evolve";
                    desc.compute.entryPoint = Entry::ZONE_GOL_EVOLVE;
                    zoneGolEvolvePipeline_ = device_.CreateComputePipeline(&desc);
                    if (!zoneGolEvolvePipeline_) return false;
                }

                // Zone mesh gen pipelines (dedicated single-group layout with
                // terrain eval + zone data + mesh output bindings)
                {
                    std::array<wgpu::BindGroupLayout, 1> layouts = { zoneMeshGenLayout_ };
                    wgpu::PipelineLayoutDescriptor pld{};
                    pld.bindGroupLayoutCount = layouts.size();
                    pld.bindGroupLayouts = layouts.data();
                    wgpu::PipelineLayout pl = device_.CreatePipelineLayout(&pld);
                    if (!pl) return false;

                    wgpu::ComputePipelineDescriptor desc{};
                    desc.layout = pl;
                    desc.compute.module = shaderModule_;

                    desc.label = "Zone Mesh Reset";
                    desc.compute.entryPoint = Entry::ZONE_GOL_MESH_RESET;
                    zoneGolMeshResetPipeline_ = device_.CreateComputePipeline(&desc);
                    if (!zoneGolMeshResetPipeline_) return false;

                    desc.label = "Zone Mesh Gen";
                    desc.compute.entryPoint = Entry::ZONE_GOL_MESH_GEN;
                    zoneGolMeshGenPipeline_ = device_.CreateComputePipeline(&desc);
                    if (!zoneGolMeshGenPipeline_) return false;

                    desc.label = "Zone Derive Params";
                    desc.compute.entryPoint = Entry::ZONE_DERIVE_PARAMS;
                    zoneDeriveParamsPipeline_ = device_.CreateComputePipeline(&desc);
                    if (!zoneDeriveParamsPipeline_) return false;
                }

                // Pyramid mesh gen pipeline (dedicated layout, isolated from terrain eval)
                {
                    std::array<wgpu::BindGroupLayout, 1> layouts = { pyramidMeshGenLayout_ };
                    wgpu::PipelineLayoutDescriptor pld{};
                    pld.bindGroupLayoutCount = layouts.size();
                    pld.bindGroupLayouts = layouts.data();
                    wgpu::PipelineLayout pl = device_.CreatePipelineLayout(&pld);
                    if (!pl) return false;

                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Pyramid Mesh Gen";
                    desc.layout = pl;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::PYRAMID_MESH_GEN;
                    pyramidMeshGenPipeline_ = device_.CreateComputePipeline(&desc);
                    if (!pyramidMeshGenPipeline_) return false;
                }

                // Arch mesh gen pipeline (dedicated layout — bindings 193-195)
                {
                    std::array<wgpu::BindGroupLayout, 1> layouts = { archMeshGenLayout_ };
                    wgpu::PipelineLayoutDescriptor pld{};
                    pld.bindGroupLayoutCount = layouts.size();
                    pld.bindGroupLayouts = layouts.data();
                    wgpu::PipelineLayout pl = device_.CreatePipelineLayout(&pld);
                    if (!pl) return false;

                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Arch Mesh Gen";
                    desc.layout = pl;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::ARCH_MESH_GEN;
                    archMeshGenPipeline_ = device_.CreateComputePipeline(&desc);
                    if (!archMeshGenPipeline_) return false;
                }

                // Column mesh gen pipeline (dedicated layout — bindings 196-198)
                {
                    std::array<wgpu::BindGroupLayout, 1> layouts = { columnMeshGenLayout_ };
                    wgpu::PipelineLayoutDescriptor pld{};
                    pld.bindGroupLayoutCount = layouts.size();
                    pld.bindGroupLayouts = layouts.data();
                    wgpu::PipelineLayout pl = device_.CreatePipelineLayout(&pld);
                    if (!pl) return false;

                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Column Mesh Gen";
                    desc.layout = pl;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::COLUMN_MESH_GEN;
                    columnMeshGenPipeline_ = device_.CreateComputePipeline(&desc);
                    if (!columnMeshGenPipeline_) return false;
                }

                // Palm mesh gen compute pipeline
                {
                    std::array<wgpu::BindGroupLayout, 1> layouts = { palmMeshGenLayout_ };
                    wgpu::PipelineLayoutDescriptor pld{};
                    pld.bindGroupLayoutCount = layouts.size();
                    pld.bindGroupLayouts = layouts.data();
                    wgpu::PipelineLayout pl = device_.CreatePipelineLayout(&pld);
                    if (!pl) return false;

                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Palm Mesh Gen";
                    desc.layout = pl;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::PALM_MESH_GEN;
                    palmMeshGenPipeline_ = device_.CreateComputePipeline(&desc);
                    if (!palmMeshGenPipeline_) return false;
                }

                // Cactus mesh gen compute pipeline
                {
                    std::array<wgpu::BindGroupLayout, 1> layouts = { cactusMeshGenLayout_ };
                    wgpu::PipelineLayoutDescriptor pld{};
                    pld.bindGroupLayoutCount = layouts.size();
                    pld.bindGroupLayouts = layouts.data();
                    wgpu::PipelineLayout pl = device_.CreatePipelineLayout(&pld);
                    if (!pl) return false;
                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Cactus Mesh Gen";
                    desc.layout = pl;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::CACTUS_MESH_GEN;
                    cactusMeshGenPipeline_ = device_.CreateComputePipeline(&desc);
                    if (!cactusMeshGenPipeline_) return false;
                }

                // Blade mesh gen compute pipeline
                {
                    std::array<wgpu::BindGroupLayout, 1> layouts = { bladeMeshGenLayout_ };
                    wgpu::PipelineLayoutDescriptor pld{};
                    pld.bindGroupLayoutCount = layouts.size();
                    pld.bindGroupLayouts = layouts.data();
                    wgpu::PipelineLayout pl = device_.CreatePipelineLayout(&pld);
                    if (!pl) return false;
                    wgpu::ComputePipelineDescriptor desc{};
                    desc.label = "Blade Mesh Gen";
                    desc.layout = pl;
                    desc.compute.module = shaderModule_;
                    desc.compute.entryPoint = Entry::BLADE_MESH_GEN;
                    bladeMeshGenPipeline_ = device_.CreateComputePipeline(&desc);
                    if (!bladeMeshGenPipeline_) return false;
                }

                return true;
            }

            // -----------------------------------------------------------------
            // Render pipeline creation
            // -----------------------------------------------------------------

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

                    patchTerrainPipeline_ = device_.CreateRenderPipeline(&desc);
                    if (!patchTerrainPipeline_) return false;
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

                    patchTerrainIndirectPipeline_ = device_.CreateRenderPipeline(&desc);
                    if (!patchTerrainIndirectPipeline_) return false;
                }



                // Zone cell extrusion pipeline (GoL alive cells with height)
                {
                    std::array<wgpu::VertexAttribute, 4> zoneAttrs{};
                    zoneAttrs[0].format = wgpu::VertexFormat::Float32x3;
                    zoneAttrs[0].offset = 0;
                    zoneAttrs[0].shaderLocation = 0;   // pos
                    zoneAttrs[1].format = wgpu::VertexFormat::Float32x3;
                    zoneAttrs[1].offset = 12;
                    zoneAttrs[1].shaderLocation = 1;   // normal
                    zoneAttrs[2].format = wgpu::VertexFormat::Float32x2;
                    zoneAttrs[2].offset = 24;
                    zoneAttrs[2].shaderLocation = 2;   // uv
                    zoneAttrs[3].format = wgpu::VertexFormat::Float32x3;
                    zoneAttrs[3].offset = 32;
                    zoneAttrs[3].shaderLocation = 3;   // color (pre-computed)

                    wgpu::VertexBufferLayout zoneVBL{};
                    zoneVBL.arrayStride = 44;   // 11 floats: pos3+normal3+uv2+color3
                    zoneVBL.stepMode = wgpu::VertexStepMode::Vertex;
                    zoneVBL.attributeCount = zoneAttrs.size();
                    zoneVBL.attributes = zoneAttrs.data();

                    wgpu::FragmentState fragment{};
                    fragment.module = shaderModule_;
                    fragment.entryPoint = Entry::ZONE_EXTRUSION_FS;
                    fragment.targetCount = 1;
                    fragment.targets = &colorTarget;

                    wgpu::RenderPipelineDescriptor desc{};
                    desc.label = "Zone Cell Extrusion";
                    desc.layout = renderLayout;
                    desc.vertex.module = shaderModule_;
                    desc.vertex.entryPoint = Entry::ZONE_EXTRUSION_VS;
                    desc.vertex.bufferCount = 1;
                    desc.vertex.buffers = &zoneVBL;
                    desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                    desc.primitive.cullMode = wgpu::CullMode::Back;
                    desc.primitive.frontFace = wgpu::FrontFace::CCW;
                    desc.depthStencil = &depthStencil;
                    desc.fragment = &fragment;

                    zoneExtrusionPipeline_ = device_.CreateRenderPipeline(&desc);
                    if (!zoneExtrusionPipeline_) return false;
                }


                // Pawn pipeline -- chess pawn, GPU-generated from vertex_index
                {
                    wgpu::FragmentState fragment{};
                    fragment.module = shaderModule_;
                    fragment.entryPoint = Entry::ENTITY_FS;
                    fragment.targetCount = 1;
                    fragment.targets = &colorTarget;

                    wgpu::RenderPipelineDescriptor desc{};
                    desc.label = "Pawn Entity (Chess Pawn)";
                    desc.layout = renderLayout;
                    desc.vertex.module = shaderModule_;
                    desc.vertex.entryPoint = Entry::PAWN_VS;
                    desc.vertex.bufferCount = 0;
                    desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                    desc.primitive.cullMode = wgpu::CullMode::None;
                    desc.primitive.frontFace = wgpu::FrontFace::CCW;
                    desc.depthStencil = &depthStencil;
                    desc.fragment = &fragment;

                    pawnPipeline_ = device_.CreateRenderPipeline(&desc);
                    if (!pawnPipeline_) return false;
                }


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

                    wgpu::FragmentState fragment{};
                    fragment.module = shaderModule_;
                    fragment.entryPoint = Entry::ENTITY_FS;
                    fragment.targetCount = 1;
                    fragment.targets = &colorTarget;

                    wgpu::RenderPipelineDescriptor desc{};
                    desc.label = "Sphere Entity (Rasterized)";
                    desc.layout = renderLayout;
                    desc.vertex.module = shaderModule_;
                    desc.vertex.entryPoint = Entry::SPHERE_VS;
                    desc.vertex.bufferCount = 1;
                    desc.vertex.buffers = &meshVBL;
                    desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                    desc.primitive.cullMode = wgpu::CullMode::Back;
                    desc.primitive.frontFace = wgpu::FrontFace::CCW;
                    desc.depthStencil = &depthStencil;
                    desc.fragment = &fragment;

                    spherePipeline_ = device_.CreateRenderPipeline(&desc);
                    if (!spherePipeline_) return false;

                    // Monolith pipeline — same vertex format, different VS
                    desc.label = "Monolith Entity (Rasterized)";
                    desc.vertex.entryPoint = Entry::MONOLITH_VS;
                    monolithPipeline_ = device_.CreateRenderPipeline(&desc);
                    if (!monolithPipeline_) return false;
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

                    wgpu::FragmentState fragment{};
                    fragment.module = shaderModule_;
                    fragment.entryPoint = Entry::ENTITY_FS;
                    fragment.targetCount = 1;
                    fragment.targets = &colorTarget;

                    wgpu::RenderPipelineDescriptor desc{};
                    desc.label = "Catenary Arch (Rasterized)";
                    desc.layout = renderLayout;
                    desc.vertex.module = shaderModule_;
                    desc.vertex.entryPoint = Entry::ARCH_VS;
                    desc.vertex.bufferCount = 1;
                    desc.vertex.buffers = &archVBL;
                    desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                    desc.primitive.cullMode = wgpu::CullMode::Back;
                    desc.primitive.frontFace = wgpu::FrontFace::CCW;
                    desc.depthStencil = &depthStencil;
                    desc.fragment = &fragment;

                    archPipeline_ = device_.CreateRenderPipeline(&desc);
                    if (!archPipeline_) return false;

                    // Column pipeline — same vertex format as arch, different VS.
                    // CullMode::None because the complex profile (walls + discs + shelves)
                    // has many normal-direction transitions that make consistent winding fragile.
                    desc.label = "Generative Column (Rasterized)";
                    desc.vertex.entryPoint = Entry::COLUMN_VS;
                    desc.primitive.cullMode = wgpu::CullMode::None;
                    columnPipeline_ = device_.CreateRenderPipeline(&desc);
                    if (!columnPipeline_) return false;

                    // Palm pipeline — same vertex format, no backface culling (frond quads are single-sided)
                    desc.label = "Palm Tree (Rasterized)";
                    desc.vertex.entryPoint = Entry::PALM_VS;
                    desc.primitive.cullMode = wgpu::CullMode::None;
                    palmPipeline_ = device_.CreateRenderPipeline(&desc);
                    if (!palmPipeline_) return false;

                    // Cactus pipeline — same vertex format, no backface culling
                    desc.label = "Cactus (Rasterized)";
                    desc.vertex.entryPoint = Entry::CACTUS_VS;
                    desc.primitive.cullMode = wgpu::CullMode::None;
                    cactusPipeline_ = device_.CreateRenderPipeline(&desc);
                    if (!cactusPipeline_) return false;

                    // Blade cluster pipeline — no backface culling (flat quads visible from both sides)
                    desc.label = "Blade Cluster (Rasterized)";
                    desc.vertex.entryPoint = Entry::BLADE_VS;
                    desc.primitive.cullMode = wgpu::CullMode::None;
                    bladePipeline_ = device_.CreateRenderPipeline(&desc);
                    if (!bladePipeline_) return false;

                    // Pyramid pipeline — same vertex format as arch/column, Back culling.
                    desc.label = "Generative Pyramid (Rasterized)";
                    desc.vertex.entryPoint = Entry::PYRAMID_VS;
                    desc.primitive.cullMode = wgpu::CullMode::Back;
                    pyramidPipeline_ = device_.CreateRenderPipeline(&desc);
                    if (!pyramidPipeline_) return false;
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

                    wgpu::FragmentState fragment{};
                    fragment.module = shaderModule_;
                    fragment.entryPoint = Entry::ENTITY_FS;
                    fragment.targetCount = 1;
                    fragment.targets = &colorTarget;

                    wgpu::RenderPipelineDescriptor desc{};
                    desc.label = "Indoor Shell (Ceiling + Walls)";
                    desc.layout = renderLayout;
                    desc.vertex.module = shaderModule_;
                    desc.vertex.entryPoint = Entry::SHELL_VS;
                    desc.vertex.bufferCount = 1;
                    desc.vertex.buffers = &shellVBL;
                    desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                    desc.primitive.cullMode = wgpu::CullMode::None;  // normals face inward (ceiling) and outward (walls)
                    desc.primitive.frontFace = wgpu::FrontFace::CCW;
                    desc.depthStencil = &depthStencil;
                    desc.fragment = &fragment;

                    shellPipeline_ = device_.CreateRenderPipeline(&desc);
                    if (!shellPipeline_) return false;
                }

                // Ribbon pipeline -- sky ribbon, GPU-generated cubes from vertex_index
                {
                    wgpu::FragmentState fragment{};
                    fragment.module = shaderModule_;
                    fragment.entryPoint = Entry::ENTITY_FS;
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

                    ribbonPipeline_ = device_.CreateRenderPipeline(&desc);
                    if (!ribbonPipeline_) return false;
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

                    orbRenderPipeline_ = device_.CreateRenderPipeline(&desc);
                    if (!orbRenderPipeline_) return false;
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

                    galleryFramePipeline_ = device_.CreateRenderPipeline(&desc);
                    if (!galleryFramePipeline_) return false;

                    // Shadow Gallery Frame (depth-only, instanced, same layout)
                    {
                        wgpu::DepthStencilState shadowDepth{};
                        shadowDepth.format = wgpu::TextureFormat::Depth32Float;
                        shadowDepth.depthWriteEnabled = true;
                        shadowDepth.depthCompare = wgpu::CompareFunction::Less;

                        wgpu::RenderPipelineDescriptor sdesc{};
                        sdesc.label = "Shadow Gallery Frame";
                        sdesc.layout = galleryLayout;
                        sdesc.vertex.module = shaderModule_;
                        sdesc.vertex.entryPoint = Entry::SHADOW_GALLERY_FRAME_VS;
                        sdesc.vertex.bufferCount = 0;
                        sdesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                        sdesc.primitive.cullMode = wgpu::CullMode::None;
                        sdesc.primitive.frontFace = wgpu::FrontFace::CCW;
                        sdesc.depthStencil = &shadowDepth;
                        sdesc.fragment = nullptr;

                        shadowGalleryFramePipeline_ = device_.CreateRenderPipeline(&sdesc);
                        if (!shadowGalleryFramePipeline_) return false;
                    }
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

                        wallPaintingCanvasPipeline_ = device_.CreateRenderPipeline(&desc);
                        if (!wallPaintingCanvasPipeline_) return false;
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

                        wallPaintingFramePipeline_ = device_.CreateRenderPipeline(&desc);
                        if (!wallPaintingFramePipeline_) return false;
                    }

                    // Shadow wall painting (depth-only, Depth32Float, same gallery layouts)
                    {
                        wgpu::DepthStencilState shadowDepth{};
                        shadowDepth.format = wgpu::TextureFormat::Depth32Float;
                        shadowDepth.depthWriteEnabled = true;
                        shadowDepth.depthCompare = wgpu::CompareFunction::Less;

                        wgpu::RenderPipelineDescriptor desc{};
                        desc.label = "Shadow Wall Painting";
                        desc.layout = wpLayout;
                        desc.vertex.module = shaderModule_;
                        desc.vertex.entryPoint = Entry::SHADOW_WALL_PAINTING_VS;
                        desc.vertex.bufferCount = 0;
                        desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                        desc.primitive.cullMode = wgpu::CullMode::None;
                        desc.primitive.frontFace = wgpu::FrontFace::CCW;
                        desc.depthStencil = &shadowDepth;
                        desc.fragment = nullptr;

                        shadowWallPaintingPipeline_ = device_.CreateRenderPipeline(&desc);
                        if (!shadowWallPaintingPipeline_) return false;
                    }
                }

                // ─── Shadow Pipelines (depth-only, Depth32Float) ─────────────────
                // Same bind group layouts as main render, but no fragment shader,
                // no color target, and shadow map depth format.
                {
                    wgpu::DepthStencilState shadowDepth{};
                    shadowDepth.format = wgpu::TextureFormat::Depth32Float;
                    shadowDepth.depthWriteEnabled = true;
                    shadowDepth.depthCompare = wgpu::CompareFunction::Less;

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


                    // Shadow Patch Terrain (instanced, no vertex buffer)
                    {
                        wgpu::RenderPipelineDescriptor desc{};
                        desc.label = "Shadow Patch Terrain";
                        desc.layout = shadowRenderLayout;
                        desc.vertex.module = shaderModule_;
                        desc.vertex.entryPoint = Entry::SHADOW_PATCH_TERRAIN_VS;
                        desc.vertex.bufferCount = 0;
                        desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                        desc.primitive.cullMode = wgpu::CullMode::Back;
                        desc.primitive.frontFace = wgpu::FrontFace::CCW;
                        desc.depthStencil = &shadowDepth;
                        desc.fragment = nullptr;

                        shadowPatchTerrainPipeline_ = device_.CreateRenderPipeline(&desc);
                        if (!shadowPatchTerrainPipeline_) return false;
                    }

                    // Shadow Pawn (no vertex buffer, vertex_index)
                    {
                        wgpu::RenderPipelineDescriptor desc{};
                        desc.label = "Shadow Pawn";
                        desc.layout = shadowRenderLayout;
                        desc.vertex.module = shaderModule_;
                        desc.vertex.entryPoint = Entry::SHADOW_PAWN_VS;
                        desc.vertex.bufferCount = 0;
                        desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                        desc.primitive.cullMode = wgpu::CullMode::None;
                        desc.primitive.frontFace = wgpu::FrontFace::CCW;
                        desc.depthStencil = &shadowDepth;
                        desc.fragment = nullptr;

                        shadowPawnPipeline_ = device_.CreateRenderPipeline(&desc);
                        if (!shadowPawnPipeline_) return false;
                    }

                    // Shadow Sphere (MeshVertex buffer)
                    {
                        wgpu::RenderPipelineDescriptor desc{};
                        desc.label = "Shadow Sphere";
                        desc.layout = shadowRenderLayout;
                        desc.vertex.module = shaderModule_;
                        desc.vertex.entryPoint = Entry::SHADOW_SPHERE_VS;
                        desc.vertex.bufferCount = 1;
                        desc.vertex.buffers = &shadowMeshVBL;
                        desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                        desc.primitive.cullMode = wgpu::CullMode::Back;
                        desc.primitive.frontFace = wgpu::FrontFace::CCW;
                        desc.depthStencil = &shadowDepth;
                        desc.fragment = nullptr;

                        shadowSpherePipeline_ = device_.CreateRenderPipeline(&desc);
                        if (!shadowSpherePipeline_) return false;
                    }

                    // Shadow Monolith (same MeshVertex buffer, different VS)
                    {
                        wgpu::RenderPipelineDescriptor desc{};
                        desc.label = "Shadow Monolith";
                        desc.layout = shadowRenderLayout;
                        desc.vertex.module = shaderModule_;
                        desc.vertex.entryPoint = Entry::SHADOW_MONOLITH_VS;
                        desc.vertex.bufferCount = 1;
                        desc.vertex.buffers = &shadowMeshVBL;
                        desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                        desc.primitive.cullMode = wgpu::CullMode::Back;
                        desc.primitive.frontFace = wgpu::FrontFace::CCW;
                        desc.depthStencil = &shadowDepth;
                        desc.fragment = nullptr;

                        shadowMonolithPipeline_ = device_.CreateRenderPipeline(&desc);
                        if (!shadowMonolithPipeline_) return false;
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

                        wgpu::RenderPipelineDescriptor desc{};
                        desc.label = "Shadow Catenary Arch";
                        desc.layout = shadowRenderLayout;
                        desc.vertex.module = shaderModule_;
                        desc.vertex.entryPoint = Entry::SHADOW_ARCH_VS;
                        desc.vertex.bufferCount = 1;
                        desc.vertex.buffers = &shadowArchVBL;
                        desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                        desc.primitive.cullMode = wgpu::CullMode::Back;
                        desc.primitive.frontFace = wgpu::FrontFace::CCW;
                        desc.depthStencil = &shadowDepth;
                        desc.fragment = nullptr;

                        shadowArchPipeline_ = device_.CreateRenderPipeline(&desc);
                        if (!shadowArchPipeline_) return false;

                        // Shadow Column — same vertex format, different VS
                        desc.label = "Shadow Generative Column";
                        desc.vertex.entryPoint = Entry::SHADOW_COLUMN_VS;
                        desc.primitive.cullMode = wgpu::CullMode::None;
                        shadowColumnPipeline_ = device_.CreateRenderPipeline(&desc);
                        if (!shadowColumnPipeline_) return false;

                        // Shadow Palm — same vertex format, no culling
                        desc.label = "Shadow Palm Tree";
                        desc.vertex.entryPoint = Entry::SHADOW_PALM_VS;
                        desc.primitive.cullMode = wgpu::CullMode::None;
                        shadowPalmPipeline_ = device_.CreateRenderPipeline(&desc);
                        if (!shadowPalmPipeline_) return false;

                        // Shadow Cactus
                        desc.label = "Shadow Cactus";
                        desc.vertex.entryPoint = Entry::SHADOW_CACTUS_VS;
                        desc.primitive.cullMode = wgpu::CullMode::None;
                        shadowCactusPipeline_ = device_.CreateRenderPipeline(&desc);
                        if (!shadowCactusPipeline_) return false;

                        // Shadow Blade Cluster
                        desc.label = "Shadow Blade Cluster";
                        desc.vertex.entryPoint = Entry::SHADOW_BLADE_VS;
                        desc.primitive.cullMode = wgpu::CullMode::None;
                        shadowBladePipeline_ = device_.CreateRenderPipeline(&desc);
                        if (!shadowBladePipeline_) return false;

                        // Shadow Pyramid — same vertex format, Back culling
                        desc.label = "Shadow Generative Pyramid";
                        desc.vertex.entryPoint = Entry::SHADOW_PYRAMID_VS;
                        desc.primitive.cullMode = wgpu::CullMode::Back;
                        shadowPyramidPipeline_ = device_.CreateRenderPipeline(&desc);
                        if (!shadowPyramidPipeline_) return false;
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

                        wgpu::RenderPipelineDescriptor desc{};
                        desc.label = "Shadow Indoor Shell";
                        desc.layout = shadowRenderLayout;
                        desc.vertex.module = shaderModule_;
                        desc.vertex.entryPoint = Entry::SHADOW_SHELL_VS;
                        desc.vertex.bufferCount = 1;
                        desc.vertex.buffers = &shadowShellVBL;
                        desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                        desc.primitive.cullMode = wgpu::CullMode::None;
                        desc.primitive.frontFace = wgpu::FrontFace::CCW;
                        desc.depthStencil = &shadowDepth;
                        desc.fragment = nullptr;

                        shadowShellPipeline_ = device_.CreateRenderPipeline(&desc);
                        if (!shadowShellPipeline_) return false;
                    }

                    // Shadow Ribbon (no vertex buffer, GPU-generated from vertex_index)
                    {
                        wgpu::RenderPipelineDescriptor desc{};
                        desc.label = "Shadow Sky Ribbon";
                        desc.layout = shadowRenderLayout;
                        desc.vertex.module = shaderModule_;
                        desc.vertex.entryPoint = Entry::SHADOW_RIBBON_VS;
                        desc.vertex.bufferCount = 0;
                        desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                        desc.primitive.cullMode = wgpu::CullMode::None;
                        desc.primitive.frontFace = wgpu::FrontFace::CCW;
                        desc.depthStencil = &shadowDepth;
                        desc.fragment = nullptr;

                        shadowRibbonPipeline_ = device_.CreateRenderPipeline(&desc);
                        if (!shadowRibbonPipeline_) return false;
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

                        wgpu::RenderPipelineDescriptor desc{};
                        desc.label = "Shadow Zone Extrusion";
                        desc.layout = shadowRenderLayout;
                        desc.vertex.module = shaderModule_;
                        desc.vertex.entryPoint = Entry::SHADOW_ZONE_EXTRUSION_VS;
                        desc.vertex.bufferCount = 1;
                        desc.vertex.buffers = &vbl;
                        desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                        desc.primitive.cullMode = wgpu::CullMode::Back;
                        desc.primitive.frontFace = wgpu::FrontFace::CCW;
                        desc.depthStencil = &shadowDepth;
                        desc.fragment = nullptr;

                        shadowZoneExtrusionPipeline_ = device_.CreateRenderPipeline(&desc);
                        if (!shadowZoneExtrusionPipeline_) return false;
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

                    fadeOverlayPipeline_ = device_.CreateRenderPipeline(&desc);
                    if (!fadeOverlayPipeline_) return false;
                }

                return true;
            }
        };

    } // namespace the_board
} // namespace t7