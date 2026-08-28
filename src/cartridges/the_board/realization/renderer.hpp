#pragma once

// THE_BOARD CARTRIDGE -- Renderer (Rasterized)
// ==================================================

#include "cartridges/the_board/realization/state.hpp"
#include "core/sha256.hpp"   // PROBATE_SEAL2 — the serve witness's digest
#include "core/boot_card.hpp"   // IOS_3 B2 — the build stamp and shader sha reach the page
#include <webgpu/webgpu_cpp.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>      // EM_ASM — reads the baked shader digest
#endif
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
            constexpr const char* UPDATE_OTHER_AGENTS = "update_other_agents";        // 1D (296 field lanes; slots 0-31 also walk)
            constexpr const char* UPDATE_CAMERA_VP = "update_camera_vp";            // 0D -- camera + VP, one lane (SPINE_2)
            constexpr const char* UPDATE_SPHERE = "update_sphere";                  // 0D
            constexpr const char* UPDATE_CUBE = "update_cube";                      // 1D (256 threads, one per cube slot)

            // On-demand compute
            constexpr const char* BAKE_PATCH_HEIGHTFIELD = "bake_patch_heightfield";         // 2D x batch -- the fused bake
            constexpr const char* GENERATE_PATCH_CELLS = "generate_patch_cells";              // 2D -- per-patch
            constexpr const char* RIBBON_HEAD = "ribbon_head";                                // 0D -- one thread: intent, the Sky Rule, flight, the spine
            constexpr const char* RIBBON_BODY = "ribbon_body";                                // 1D -- per ring: rest, gesture, tension, frame, motor
            constexpr const char* COMPUTE_PAWN_AURA = "compute_pawn_aura";                  // 2D -- toroidal grid
            constexpr const char* WRITE_LIVE_CARD = "write_live_card";                      // 2D -- the card, one fused pass (LATTICE_4)
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

            // GPU frustum culling (every frame, after update_camera_vp)
            constexpr const char* FRUSTUM_CULL_PATCHES = "frustum_cull_patches";

            // GoL zone compute (zone-local automaton)
            constexpr const char* ZONE_GOL_SYNC = "zone_gol_sync";
            constexpr const char* ZONE_GOL_EVOLVE = "zone_gol_evolve";
            constexpr const char* ZONE_DERIVE_PARAMS = "zone_derive_params";

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
            wgpu::BindGroupLayout agentsStateLayout_;
            wgpu::BindGroupLayout agentsTexturesLayout_;
            wgpu::BindGroupLayout auraStateLayout_;
            wgpu::BindGroupLayout auraTexturesLayout_;
            wgpu::BindGroupLayout cullStateLayout_;
            wgpu::BindGroupLayout emptyLayout_;
            wgpu::BindGroupLayout frameKStateLayout_;
            wgpu::BindGroupLayout frameKTexturesLayout_;
            wgpu::BindGroupLayout frameRLayout_;
            wgpu::BindGroupLayout frameCLayout_;
            wgpu::BindGroupLayout galleryStateLayout_;
            wgpu::BindGroupLayout galleryTexturesLayout_;
            wgpu::BindGroupLayout photoKStateLayout_;
            wgpu::BindGroupLayout photoKTexturesLayout_;
            wgpu::BindGroupLayout meshgenStateLayout_;
            wgpu::BindGroupLayout orbsAStateLayout_;
            wgpu::BindGroupLayout orbsBStateLayout_;
            wgpu::BindGroupLayout patchgenStateLayout_;
            wgpu::BindGroupLayout patchgenTexturesLayout_;
            wgpu::BindGroupLayout placeStateLayout_;
            wgpu::BindGroupLayout placeTexturesLayout_;
            wgpu::BindGroupLayout ribbonStateLayout_;
            wgpu::BindGroupLayout ribbonTexturesLayout_;   // SPINE_2 — the room reads the baked ground
            wgpu::BindGroupLayout sceneStateLayout_;
            wgpu::BindGroupLayout sceneTexturesLayout_;
            wgpu::BindGroupLayout shadowStateLayout_;
            wgpu::BindGroupLayout shadowTexturesLayout_;
            wgpu::BindGroupLayout worldLayout_;
            wgpu::BindGroupLayout zonesStateLayout_;
            wgpu::BindGroupLayout zonesTexturesLayout_;
            // ATLAS_1revB D3" — the offset every non-shadow set of the
            // render-entity group passes. A dynamic-offset layout requires an
            // offset at EVERY set; only the shadow tile loop varies it.
            static constexpr uint32_t kShadowSlotZero = 0;
            wgpu::TextureFormat colorFormat_;
            wgpu::TextureFormat depthFormat_;

            // ═══ THE BUNDLES (BUNDLE_1) — storage; the API is public below.
            wgpu::RenderBundle mainBundle_;

            wgpu::ShaderModule shaderModule_;
            std::string shaderSource_;
            std::string shaderPath_;
            // PROBATE_SEAL2/3 — what the serve witness saw, kept so the
            // floor can state it once and the report can quote it.
            std::string shaderSha_;
            size_t shaderBytes_ = 0;
            bool serveWitnessed_ = false;

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
            // strataLayoutFor wraps the four LOOM_2 strata (WORLD fixed, then frame /
            // state / textures, EMPTY where a stratum is unused) into a pipeline layout.
            // makeComputePipeline is the uniform creation ALL 30 compute pipelines shared —
            // a pure (entry-point, pipeline-layout) pair over the one shaderModule_; the
            // descriptor carried no other per-pipeline state. The FORKS stay at the call
            // site: which layout, which entry string (passed VERBATIM — the sole C++->shader
            // link), which member, and the ROSTER gate. (The layout-build moves just outside
            // the tPipe timing block; the boot-leaderboard ms now excludes the trivial layout
            // creation — no behavior/pixel effect.)
            // DOMESDAY_2 F2-b1 (label law, second enforcement): every
            // caller names its layout — the error log's own
            // '[Invalid PipelineLayout (unlabeled)]' was the evidence.
            wgpu::PipelineLayout strataLayoutFor(const char* label,
                                                 wgpu::BindGroupLayout frame,
                                                 wgpu::BindGroupLayout state,
                                                 wgpu::BindGroupLayout tex) {
                std::array<wgpu::BindGroupLayout, 4> sa = { worldLayout_, frame, state, tex };
                wgpu::PipelineLayoutDescriptor d{};
                d.label = label;
                d.bindGroupLayoutCount = sa.size();
                d.bindGroupLayouts = sa.data();
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
            wgpu::ComputePipeline updateOtherAgentsPipeline_;    // 1D (one thread per field lane)
            wgpu::ComputePipeline cameraVPPipeline_;             // 0D -- camera + VP, fused at SPINE_2
            wgpu::ComputePipeline updateSpherePipeline_;         // 0D
            wgpu::ComputePipeline updateCubePipeline_;           // 0D

            // Compute pipelines -- patch heightfield generation (batched, one pass)
            wgpu::ComputePipeline bakePatchPipeline_;                // 2D x batch -- heights + gradients, one pass
            wgpu::ComputePipeline generatePatchCellsPipeline_;        // 2D
            wgpu::ComputePipeline ribbonHeadPipeline_;                    // 0D -- the ribbon's head: intent, the Sky Rule, flight, the spine
            wgpu::ComputePipeline ribbonBodyPipeline_;                    // 1D -- the ribbon's body: one thread per ring

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
            wgpu::ComputePipeline pawnAuraPipeline_;
            wgpu::ComputePipeline liveCardPipeline_;         // TRUEBAND_CONTACT_1, fused at LATTICE_4
            wgpu::ComputePipeline zoneSeedMaskPipeline_;     // UNIFIED_GROUND_1 U5

            // Orb sky layer pipelines
            wgpu::ComputePipeline orbInitPipeline_;
            wgpu::ComputePipeline orbDynamicsPipeline_;
            wgpu::ComputePipeline orbRecolorPipeline_;
            wgpu::ComputePipeline orbCopyPrevPipeline_;
            wgpu::RenderPipeline  orbRenderPipeline_;

            // GoL zone compute pipelines (dedicated layout, z-dispatched per zone)
            // ZONE_GRID_WG: workgroups per axis for the three 8×8 zone kernels
            // (zone_gol_sync / zone_gol_evolve / zone_seed_mask). DERIVED from
            // the capacity constant — the one-spelling law. Kernel-side the
            // bound is the zone's own grid_size, so this over-dispatches a
            // sub-32 tier and the guard retires the excess threads.
            static constexpr uint32_t ZONE_GRID_WG = (Dim::GOL_ZONE_GRID + 7u) / 8u;
            wgpu::ComputePipeline zoneGolSyncPipeline_;
            wgpu::ComputePipeline zoneGolEvolvePipeline_;

            // Zone parameter derivation (shares the GoL compute layout; one
            // workgroup per pending derive request)
            wgpu::ComputePipeline zoneDeriveParamsPipeline_;

            // Fade overlay (fullscreen alpha-blended triangle)
            wgpu::RenderPipeline fadeOverlayPipeline_;

            // GPU entity mesh gen (Phase 2: arches, Phase 3: columns — pyramid mesh-gen CUT)
            wgpu::ComputePipeline archMeshGenPipeline_;
            wgpu::ComputePipeline columnMeshGenPipeline_;
            wgpu::ComputePipeline palmMeshGenPipeline_;
            wgpu::ComputePipeline cactusMeshGenPipeline_;
            wgpu::ComputePipeline bladeMeshGenPipeline_;

        public:
            // ═══ THE BUNDLES (BUNDLE_1) ═══════════════════════════════════
            //
            // The main pass's opaque list and the sun shadow pass's are each
            // recorded ONCE into a GPURenderBundle and executed with one call
            // per frame. The browser's per-command validation moves from
            // every frame to the recording; the GPU work is unchanged by
            // construction, because the same pipelines, groups, buffers and
            // counts are issued in the same order.
            //
            // A BUNDLE CAPTURES OBJECTS, NOT VALUES. It holds the bind groups
            // and buffers it was recorded with, so a re-record is needed only
            // when one of those OBJECTS is recreated — R-B found exactly zero
            // post-boot recreation sites today (galleryTexturesGroup_ is the
            // one rebuildable group and nothing calls its rebuild after boot).
            // What it does NOT capture is the ledger's CONTENTS: an indirect
            // draw reads its count at execution, which is the whole reason
            // Commit A exists.
            //
            // The descriptor states the pass's formats. colorFormat_,
            // depthFormat_ and effective_msaa() are all boot-read and cannot
            // move afterwards (the surface reconfigure keeps the format), so
            // no format change can invalidate a bundle without a reboot.
            // ═══ ONE BUNDLE, NOT TWO (IOS_5) ════════════════════════════
            //
            // THE SUN'S BUNDLE IS GONE, AND IT IS NOT COMING BACK WITHOUT AN
            // iPad IN THE LOOP. It was a DEPTH-ONLY bundle —
            // colorFormatCount = 0, a depth-stencil format, executed in a
            // depth-only pass — and WebKit's WebGPU does not handle that
            // construct correctly. The shadow map came out wrong; the world
            // rendered black on every iOS device.
            //
            // HOW WE KNOW, and it is the intersection of two switches rather
            // than a guess. On the iPad: `?sunpass=0` (the sun pass opens and
            // clears but issues no draws) RENDERED — with the main bundle
            // still executing, so bundles as such are fine. `?bundles=0` (both
            // passes encode direct) RENDERED — with the sun's draws still
            // issued, so the draws, the pipelines, the indirect records and
            // the uint16 index buffers are all fine. The ONLY thing both
            // switches remove is ExecuteBundles on this bundle. No [gpu] error
            // and no [lost] line appeared: it is not refused at validation and
            // the device does not die. It executes, and the depth is wrong.
            //
            // DELETED ON EVERY BROWSER, no per-browser fork and no capability
            // probe. This bundle carried about a dozen calls where the main
            // one carries sixty; a dozen CPU calls do not buy a second code
            // path that exactly one implementation would ever exercise.
            //
            // The main bundle STANDS, and this same evidence confirms it good
            // on WebKit — along with the draw ledger, the indirect draws and
            // the encoder-generic verbs. BUNDLE_1's other half is intact.
            //
            // THE REPORT: docs/reference/WEBKIT_BUNDLE_BUG.md — drafted, and
            // Jean files it (an outward-facing post under the project's
            // name). Its URL is recorded there once it exists. L48 and L49
            // are the laws this bought.
            bool main_bundle_ready() const { return mainBundle_ != nullptr; }
            wgpu::RenderBundle main_bundle() const { return mainBundle_; }

            // The encoder, made where the formats live. The pass that
            // executes a bundle must agree with this exactly — the census
            // holds that (R5), and Dawn rejects a mismatch at ExecuteBundles.
            wgpu::RenderBundleEncoder make_main_bundle_encoder() {
                wgpu::RenderBundleEncoderDescriptor d{};
                d.label = "Main Bundle";
                d.colorFormatCount = 1;
                d.colorFormats = &colorFormat_;
                d.depthStencilFormat = depthFormat_;
                d.sampleCount = effective_msaa();
                return device_.CreateRenderBundleEncoder(&d);
            }
            void set_main_bundle(wgpu::RenderBundle b) { mainBundle_ = b; }


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
                agentsStateLayout_ = gpuState.agents_state_layout();
                agentsTexturesLayout_ = gpuState.agents_textures_layout();
                auraStateLayout_ = gpuState.aura_state_layout();
                auraTexturesLayout_ = gpuState.aura_textures_layout();
                cullStateLayout_ = gpuState.cull_state_layout();
                emptyLayout_ = gpuState.empty_layout();
                frameKStateLayout_ = gpuState.frame_k_state_layout();
                frameKTexturesLayout_ = gpuState.frame_k_textures_layout();
                frameRLayout_ = gpuState.frame_r_layout();
                frameCLayout_ = gpuState.frame_c_layout();
                galleryStateLayout_ = gpuState.gallery_state_layout();
                galleryTexturesLayout_ = gpuState.gallery_textures_layout();
                photoKStateLayout_ = gpuState.photo_k_state_layout();
                photoKTexturesLayout_ = gpuState.photo_k_textures_layout();
                meshgenStateLayout_ = gpuState.meshgen_state_layout();
                orbsAStateLayout_ = gpuState.orbs_a_state_layout();
                orbsBStateLayout_ = gpuState.orbs_b_state_layout();
                patchgenStateLayout_ = gpuState.patchgen_state_layout();
                patchgenTexturesLayout_ = gpuState.patchgen_textures_layout();
                placeStateLayout_ = gpuState.place_state_layout();
                placeTexturesLayout_ = gpuState.place_textures_layout();
                ribbonStateLayout_ = gpuState.ribbon_state_layout();
                ribbonTexturesLayout_ = gpuState.ribbon_textures_layout();
                sceneStateLayout_ = gpuState.scene_state_layout();
                sceneTexturesLayout_ = gpuState.scene_textures_layout();
                shadowStateLayout_ = gpuState.shadow_state_layout();
                shadowTexturesLayout_ = gpuState.shadow_textures_layout();
                worldLayout_ = gpuState.world_layout();
                zonesStateLayout_ = gpuState.zones_state_layout();
                zonesTexturesLayout_ = gpuState.zones_textures_layout();
                colorFormat_ = colorFormat;
                depthFormat_ = depthFormat;

                if (!loadShader()) return false;
                requestShaderVerdict();   // PROBATE_SEAL3 — see the note above it

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

            // ═══ OIL_1 U11 (ledger: R10, C7) — THE PASS-HEAD CONTRACT ═══
            // The six kernel helpers below ride binds set ONCE by
            // dispatch_compute at the pass head: group0 = the compute
            // entity group, group1 = the compute texture group, group2 =
            // the room — identical for every kernel (verified: each had
            // exactly one caller, and all passed these same three
            // objects). Bind-group state is sticky within a pass, and
            // WebGPU permits bound groups a pipeline's layout does not
            // use (camera reads 0/1, VP reads 0 — the extra binds are
            // ignored, not faulted). Each helper keeps SetPipeline +
            // dispatch. The ribbon-ring dispatch is NOT under this
            // contract — it binds its own group0 and runs BEFORE the
            // pass-head binds.
            void dispatch_update_player_agent(wgpu::ComputePassEncoder& pass) {
                pass.SetPipeline(updatePlayerAgentPipeline_);
                pass.DispatchWorkgroups(1, 1, 1);         // 1 workgroup × 1 thread = the possessed slot
            }

            void dispatch_update_other_agents(wgpu::ComputePassEncoder& pass) {
                if constexpr (!(ROSTER.wanderers)) return;  // ROSTER-GATE wanderers (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(updateOtherAgentsPipeline_);
                // ONE THREAD PER FIELD LANE (PANORAMA_0 RIDE_0): 5 x 64 = 320
                // covers FIELD_SUBSCRIBER_CAP 296. Lanes 0..31 are the agent
                // slots and run the walker below their own lane's field sum;
                // the rest write a field lane and end.
                pass.DispatchWorkgroups(GPUState::field_lane_workgroups(), 1, 1);
            }

            // ONE LANE FOR BOTH (SPINE_2). update_camera and compute_vp were two
            // 0D dispatches with a strict dependency — the second reads the
            // camera_state the first writes — and the two floater kernels sat
            // between them by history alone. One dispatch, one pipeline.
            void dispatch_update_camera_vp(wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup) {
                // FRAME_K (A3): the rw faces ride this family's own strata.
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.SetPipeline(cameraVPPipeline_);
                pass.DispatchWorkgroups(1, 1, 1);
            }

            void dispatch_update_sphere(wgpu::ComputePassEncoder& pass) {
                if constexpr (!(ROSTER.sphere)) return;  // ROSTER-GATE sphere (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(updateSpherePipeline_);
                pass.DispatchWorkgroups(1, 1, 1);
            }

            void dispatch_update_cube(wgpu::ComputePassEncoder& pass) {
                if constexpr (!(ROSTER.cube)) return;  // ROSTER-GATE cube (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(updateCubePipeline_);
                // ONE LANE PER CUBE (PANORAMA_0 RIDE_0): 4 x 64 = 256 =
                // Dim::CUBE_SLOT_COUNT exactly. Derived, not written as 4, so
                // a slot-count edit moves the dispatch with it; the kernel
                // guards gid.x >= CUBE_SLOT_COUNT either way.
                pass.DispatchWorkgroups(GPUState::cube_workgroups(), 1, 1);
            }

            // THE ONE BAKE PASS: height and both gradients per lattice point,
            // five terrain evaluations, one store.
            // LATTICE_1 — THE BATCH IS THE DISPATCH. `count` patches ride the z
            // axis; the kernel reads patch_params_batch[workgroup_id.z]. No
            // dynamic offset: the seat is a read-only storage ARRAY now, so the
            // whole batch is addressable from one bind.
            void dispatch_bake_patches(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup,
                uint32_t workgroups,
                uint32_t count
            ) {
                pass.SetPipeline(bakePatchPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.DispatchWorkgroups(workgroups, workgroups, count);
            }

            void dispatch_generate_patch_cells(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup,
                uint32_t workgroups,
                uint32_t count
            ) {
                pass.SetPipeline(generatePatchCellsPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.DispatchWorkgroups(workgroups, workgroups, count);
            }

            // THE RIBBON ROOM, both kernels, one pair of binds (RIBBON_1). The
            // head runs first and alone: the body reads the spine and the head
            // state it just wrote, and dispatches within a pass are ordered.
            void dispatch_ribbon(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup,
                uint32_t body_workgroups
            ) {
                if constexpr (!(ROSTER.ribbon)) return;  // ROSTER-GATE ribbon (a') — pipelines never created; the holder tolerates
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.SetPipeline(ribbonHeadPipeline_);
                pass.DispatchWorkgroups(1, 1, 1);
                pass.SetPipeline(ribbonBodyPipeline_);
                pass.DispatchWorkgroups(body_workgroups, 1, 1);
            }

            void dispatch_compute_photographer_vp(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup
            ) {
                if constexpr (!(ROSTER.gallery)) return;  // ROSTER-GATE gallery (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(photographerVPPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.DispatchWorkgroups(1, 1, 1);
            }

            void dispatch_entity_placement(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup
            ) {
                pass.SetPipeline(entityPlacementPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.DispatchWorkgroups(1, 1, 1);
            }

            void dispatch_frustum_cull(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup
            ) {
                pass.SetPipeline(frustumCullPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                // ceil(MAX_ACTIVE_PATCHES / 64) — derived, never hardcoded again
                // (was hardcoded 4 = 256 threads vs 289 slots: slots 256–288 were
                //  never culled at full window — audit CC-8a).
                pass.DispatchWorkgroups((Dim::MAX_ACTIVE_PATCHES + 63u) / 64u, 1, 1);
            }

            void dispatch_compute_pawn_aura(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup,
                uint32_t workgroups
            ) {
                if constexpr (!(ROSTER.pawn_aura)) return;  // ROSTER-GATE pawn_aura (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(pawnAuraPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.DispatchWorkgroups(workgroups, workgroups, 1);
            }

            void dispatch_live_card_write(wgpu::ComputePassEncoder& pass,
                                          wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup) {
                // ONE PASS, ONE DISPATCH (LATTICE_4). It was a pair: heights
                // into a scratch buffer, then a resolve that read the
                // neighbourhood back. Each workgroup now evaluates its own
                // 20x20 halo tile in workgroup memory, so there is nothing to
                // hand between dispatches and no visibility rule to lean on.
                pass.SetPipeline(liveCardPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.DispatchWorkgroups(Dim::LIVE_CARD_SIZE / 16u,
                                        Dim::LIVE_CARD_SIZE / 16u, 1);
            }

            void dispatch_orb_init(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup,
                uint32_t workgroups
            ) {
                if constexpr (!(ROSTER.orbs)) return;  // ROSTER-GATE orbs (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(orbInitPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.DispatchWorkgroups(workgroups, 1, 1);
            }

            void dispatch_orb_dynamics(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup,
                uint32_t workgroups
            ) {
                if constexpr (!(ROSTER.orbs)) return;  // ROSTER-GATE orbs (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(orbDynamicsPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.DispatchWorkgroups(workgroups, 1, 1);
            }

            void dispatch_orb_recolor(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup,
                uint32_t workgroups
            ) {
                if constexpr (!(ROSTER.orbs)) return;  // ROSTER-GATE orbs (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(orbRecolorPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.DispatchWorkgroups(workgroups, 1, 1);
            }

            void dispatch_orb_copy_prev(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup,
                uint32_t workgroups
            ) {
                if constexpr (!(ROSTER.orbs)) return;  // ROSTER-GATE orbs (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(orbCopyPrevPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.DispatchWorkgroups(workgroups, 1, 1);
            }

            template <class Enc>
            void draw_orbs(
                Enc& pass,
                wgpu::Buffer quadVB,
                wgpu::Buffer quadIB,
                wgpu::Buffer orbStateVB,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                if constexpr (!(ROSTER.orbs)) return;  // ROSTER-GATE orbs (a') — pipeline never created; the holder tolerates
                // The count and the `os.active` guard are the record's
                // (BUNDLE_1): zero instances draw nothing.
                pass.SetPipeline(orbRenderPipeline_);
                // LOOM_2: the scene strata (0-3) ride the pass-head binds —
                // the orb sky layer is SCENE family and rebinds nothing.
                pass.SetVertexBuffer(0, quadVB);
                // ORB_V: slot 1 is the instance-step OrbState stream that
                // replaced the binding-400 storage read in orb_vs.
                pass.SetVertexBuffer(1, orbStateVB);
                pass.SetIndexBuffer(quadIB, wgpu::IndexFormat::Uint16);
                pass.DrawIndexedIndirect(ledger, ledgerOffset);
            }

            void dispatch_zone_gol_sync(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup,
                uint32_t zone_count
            ) {
                if constexpr (!(ROSTER.gol)) return;  // ROSTER-GATE gol (a') — pipeline never created; the holder tolerates
                if (zone_count == 0) return;
                pass.SetPipeline(zoneGolSyncPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                // CAPACITY-shaped dispatch, SIZE-bounded kernel: the grid is
                // derived from Dim::GOL_ZONE_GRID over the 8×8 workgroup (was
                // a hard 4 with a "32/8=4" comment — the one-spelling law).
                // The kernel early-outs on cell >= z.grid_size, so dispatching
                // to capacity is correct for every tier; only the guard costs.
                pass.DispatchWorkgroups(ZONE_GRID_WG, ZONE_GRID_WG, zone_count);
            }

            void dispatch_zone_gol_evolve(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup,
                uint32_t zone_count
            ) {
                if constexpr (!(ROSTER.gol)) return;  // ROSTER-GATE gol (a') — pipeline never created; the holder tolerates
                if (zone_count == 0) return;
                pass.SetPipeline(zoneGolEvolvePipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.DispatchWorkgroups(ZONE_GRID_WG, ZONE_GRID_WG, zone_count);
            }

            // Zone parameter derivation (GPU-authoritative tier selection + Gaussian sampling)
            void dispatch_zone_derive_params(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup,
                uint32_t request_count
            ) {
                if constexpr (!(ROSTER.gol)) return;  // ROSTER-GATE gol (a') — pipeline never created; the holder tolerates
                if (request_count == 0) return;
                pass.SetPipeline(zoneDeriveParamsPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.DispatchWorkgroups(request_count, 1, 1);
            }

            void dispatch_zone_seed_mask(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup,
                uint32_t request_count
            ) {
                if constexpr (!(ROSTER.gol)) return;  // ROSTER-GATE gol (a') — pipeline never created; the holder tolerates
                if (request_count == 0) return;
                pass.SetPipeline(zoneSeedMaskPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.DispatchWorkgroups(ZONE_GRID_WG, ZONE_GRID_WG, request_count);
            }

            // dispatch_pyramid_mesh_gen CUT — mesh never drawn;
            // the FAMILY_DISPATCH pyramid mesh hook now routes to the none-fork.

            // GPU arch mesh gen — generates all 16 slots × 4 sub-meshes.
            // The shape is unchanged by LATTICE_2; what it counts is not.
            // workgroup_id.x = slot (0..15), workgroup_id.y = sub-mesh
            // (outer shell, inner shell, front cap, back cap), and
            // MESHGEN_LANES lanes inside each.
            void dispatch_arch_mesh_gen(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup
            ) {
                if constexpr (!(ROSTER.arch)) return;  // ROSTER-GATE arch (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(archMeshGenPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.DispatchWorkgroups(Dim::MAX_ARCH_INSTANCES, 4, 1);
            }

            // GPU column mesh gen — all 32 slots in one dispatch, one
            // workgroup each (LATTICE_2).
            void dispatch_column_mesh_gen(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup
            ) {
                if constexpr (!(ROSTER.column || ROSTER.antenna)) return;  // ROSTER-GATE column+antenna (shared pipelines) (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(columnMeshGenPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.DispatchWorkgroups(Dim::MAX_COLUMN_INSTANCES, 1, 1);
            }

            void dispatch_palm_mesh_gen(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup
            ) {
                if constexpr (!(ROSTER.palm)) return;  // ROSTER-GATE palm (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(palmMeshGenPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.DispatchWorkgroups(Dim::MAX_PALM_INSTANCES, 1, 1);
            }

            void dispatch_cactus_mesh_gen(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup
            ) {
                if constexpr (!(ROSTER.cactus)) return;  // ROSTER-GATE cactus (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(cactusMeshGenPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.DispatchWorkgroups(Dim::MAX_CACTUS_INSTANCES, 1, 1);
            }

            void dispatch_blade_mesh_gen(
                wgpu::ComputePassEncoder& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup
            ) {
                if constexpr (!(ROSTER.blade)) return;  // ROSTER-GATE blade (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(bladeMeshGenPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.DispatchWorkgroups(Dim::MAX_BLADE_INSTANCES, 1, 1);
            }

            // THE DRAW PLAN: one helper, three invocations — the args slot
            // rides the offset (0 / 20 / 40 bytes into the 3 x 5-u32 args
            // buffer). OIL_1 U13 (ledger: R19, C7): the three plan slots
            // shared one pipeline and one texture group and re-set both —
            // the pipeline is now set ONCE by begin_patch_terrain_plan and
            // group1 rides the pass head.
            // DOMESDAY_0 B3: the per-slot LIST WINDOW moved from the bind
            // group (the retired g2:62 seat's A/B/C offset windows) to the
            // SetVertexBuffer offset below — same segments, same bytes,
            // fixed-function fetch. The plan A/B/C groups became identical
            // when the seat left the layout; they keep their three names
            // pending a collapse ruling, and group 2 still re-binds per
            // slot as before.
            template <class Enc>
            void begin_patch_terrain_plan(Enc& pass) {
                pass.SetPipeline(patchTerrainIndirectPipeline_);
            }
            template <class Enc>
            void draw_patch_terrain_plan_slot(
                Enc& pass,
                wgpu::BindGroup stateGroup,
                wgpu::Buffer visibleList,
                uint64_t visibleOffset,
                uint64_t visibleBytes,
                wgpu::Buffer indexBuffer,
                wgpu::Buffer indirectArgs,
                uint64_t indirectOffset
            ) {
                pass.SetBindGroup(2, stateGroup);
                pass.SetVertexBuffer(0, visibleList, visibleOffset, visibleBytes);
                // LATTICE_3: the three patch IBs are uint16. The indirect args
                // carry indexCount / firstIndex as COUNTS, not bytes, so
                // reset_frustum_indirect is untouched by the width change.
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint16);
                pass.DrawIndexedIndirect(indirectArgs, indirectOffset);
            }

            // Direct terrain draw — uses non-indirect pipeline (outdoor or indoor variant).
            // For LOD1 outdoor, LOD0+LOD1 indoor, snapshot pass, etc.
            // DOMESDAY_0 B3: the pipeline's vertex state requires slot 0
            // bound even though this variant never reads the attribute
            // (USE_PATCH_INDIRECTION=false). The full list (512 entries)
            // covers every direct instance range the program draws
            // (MAX_ACTIVE_PATCHES = 225).
            template <class Enc>
            void draw_patch_terrain_direct(
                Enc& pass,
                wgpu::BindGroup stateGroup,
                wgpu::BindGroup texGroup,
                wgpu::Buffer visibleList,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount,
                uint32_t instanceCount,
                uint32_t firstInstance = 0
            ) {
                pass.SetPipeline(patchTerrainPipeline_);
                pass.SetBindGroup(2, stateGroup);
                pass.SetBindGroup(3, texGroup);
                pass.SetVertexBuffer(0, visibleList);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint16);   // LATTICE_3
                pass.DrawIndexed(indexCount, instanceCount, 0, 0, firstInstance);
            }

            // STATUS: LATENT[mood_cull_opt_out] — the flag is WRITTEN every
            // mood change (mood.hpp apply_mood, from MoodProfile::
            // allow_frustum_cull) and READ BY NOBODY. Its one reader was
            // render_passes.hpp's `if (!use_indirect_terrain()) return;`
            // early-out, retired with the direct indoor path when the draw
            // plan took every mood ("the kernel runs in EVERY mood now",
            // dispatch_frustum_cull). So the two indoor MOOD_TABLE rows say
            // allow_frustum_cull = false and their terrain is culled anyway
            // — the table reads as a knob and is not one. Found by OPT_1's
            // O0-f recensus; the cut (this pair, the MoodProfile column, its
            // two drift asserts, and the apply_mood poke) is a positional-
            // table edit and wants a build, so it is Jean's ruling, not a
            // residue sweep's. Revive-or-rewire when this region is worked.
            void set_frustum_cull_active(bool active) { useIndirectTerrainPipeline_ = active; }
            bool use_indirect_terrain() const { return useIndirectTerrainPipeline_; }

            // ═══ OIL_1 U13 (ledger: R19, C7) — THE COLOR PASS-HEAD CONTRACT
            // The entity draw helpers below ride group0 (the pass's entity
            // window — the FRAME group in the main pass, the
            // photographer's in the snapshot pass) and group1 (the render
            // texture group) bound ONCE by the caller before draw_table.
            // They were identical at every call within a pass and every
            // helper re-set them. All these pipelines share ONE layout
            // (renderLayout), so every draw sees the groups it saw before.
            // The gallery draws keep their own layout and bind their own
            // pair, once, at their call site.

            // Shared helper for all "indexed mesh" COLOR draws — the twin
            // of draw_shadow_indexed_mesh below, same signature and same
            // body. Per-family wrappers differ only in pipeline and
            // (rarely) instance count / first instance.
            //
            // TIDY_0d: `indexCount == 0` returns before SetPipeline. A
            // species whose mesh is empty draws nothing either way — the
            // guard changes no pixel — but Dawn logs "Draw with an index
            // count of 0 is unusual" for the submitted zero-count draw,
            // and on the web twin that warning reaches the audience's
            // browser console. Every species submits its high-water prefix
            // rather than a live count, so a family with nothing live still
            // arrives here with 0. Replacing the prefix with a live count
            // is the ARENA-era fix; this only stops the warning.
            // THE INDIRECT SIBLING (BUNDLE_1). Same shape, but the count
            // comes from a draw-ledger record instead of an argument — so
            // the draw can be RECORDED ONCE into a bundle and replayed while
            // its number keeps moving. The `indexCount == 0` early-out below
            // has no twin here and needs none: a record of zeros draws
            // nothing, and it is the record that carries the guard now.
            // (TIDY_0d's reason for that early-out — Dawn logging "Draw with
            // an index count of 0 is unusual" for a submitted zero-count draw
            // — does not arise: the count is not known at encode time.)
            template <class Enc>
            void draw_indexed_mesh_indirect(
                Enc& pass,
                wgpu::RenderPipeline pipeline,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                pass.SetPipeline(pipeline);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexedIndirect(ledger, ledgerOffset);
            }

            template <class Enc>
            void draw_indexed_mesh(
                Enc& pass,
                wgpu::RenderPipeline pipeline,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount,
                uint32_t instanceCount = 1,
                uint32_t firstInstance = 0
            ) {
                if (indexCount == 0) return;
                pass.SetPipeline(pipeline);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount, instanceCount, 0, 0, firstInstance);
            }

            template <class Enc>
            void draw_pawn(
                Enc& pass,
                uint32_t vertexCount
            ) {
                pass.SetPipeline(pawnPipeline_);
                // One instance per agent slot. Inactive slots collapse via a
                // zero-scale local mesh in pawn_vs (see is_active branch).
                pass.Draw(vertexCount, /*instanceCount=*/ Dim::MAX_AGENTS);
            }

            template <class Enc>
            void draw_sphere(
                Enc& pass,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.sphere)) return;  // ROSTER-GATE sphere (a') — pipeline never created; the holder tolerates
                draw_indexed_mesh(pass, spherePipeline_,
                    vertexBuffer, indexBuffer, indexCount,
                    Dim::MAX_SPHERE_INSTANCES);
            }

            template <class Enc>
            void draw_monolith(
                Enc& pass,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.cube)) return;  // ROSTER-GATE cube (a') — pipeline never created; the holder tolerates
                draw_indexed_mesh(pass, monolithPipeline_,
                    vertexBuffer, indexBuffer, indexCount,
                    Dim::MAX_CUBE_INSTANCES, Dim::CUBE_SLOT_OFFSET);
            }

            // RIBBON_1's LIVE count, now a record (BUNDLE_1). The
            // `ribbon_active` guard the drawable table used to carry went
            // with it: an inactive ribbon stages zero vertices, which draws
            // nothing — and unlike an encoder-time skip, that survives being
            // recorded into a bundle.
            template <class Enc>
            void draw_ribbon(
                Enc& pass,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                if constexpr (!(ROSTER.ribbon)) return;  // ROSTER-GATE ribbon (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(ribbonPipeline_);
                pass.DrawIndirect(ledger, ledgerOffset);
            }

            template <class Enc>
            void draw_arch(
                Enc& pass,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                if constexpr (!(ROSTER.arch)) return;  // ROSTER-GATE arch (a') — pipeline never created; the holder tolerates
                draw_indexed_mesh_indirect(pass, archPipeline_,
                    vertexBuffer, indexBuffer, ledger, ledgerOffset);
            }

            template <class Enc>
            void draw_column(
                Enc& pass,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                if constexpr (!(ROSTER.column || ROSTER.antenna)) return;  // ROSTER-GATE column+antenna (shared pipelines) (a') — pipeline never created; the holder tolerates
                draw_indexed_mesh_indirect(pass, columnPipeline_,
                    vertexBuffer, indexBuffer, ledger, ledgerOffset);
            }

            template <class Enc>
            void draw_palm(
                Enc& pass,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                if constexpr (!(ROSTER.palm)) return;  // ROSTER-GATE palm (a') — pipeline never created; the holder tolerates
                draw_indexed_mesh_indirect(pass, palmPipeline_,
                    vertexBuffer, indexBuffer, ledger, ledgerOffset);
            }

            template <class Enc>
            void draw_cactus(
                Enc& pass,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                if constexpr (!(ROSTER.cactus)) return;  // ROSTER-GATE cactus (a') — pipeline never created; the holder tolerates
                draw_indexed_mesh_indirect(pass, cactusPipeline_,
                    vertexBuffer, indexBuffer, ledger, ledgerOffset);
            }

            template <class Enc>
            void draw_blade(
                Enc& pass,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                if constexpr (!(ROSTER.blade)) return;  // ROSTER-GATE blade (a') — pipeline never created; the holder tolerates
                draw_indexed_mesh_indirect(pass, bladePipeline_,
                    vertexBuffer, indexBuffer, ledger, ledgerOffset);
            }

            // draw_pyramid CUT — caller-free; pyramid mesh never drawn

            template <class Enc>
            void draw_shell(
                Enc& pass,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                if constexpr (!(ROSTER.indoor_shell)) return;  // ROSTER-GATE indoor_shell (a') — pipeline never created; the holder tolerates
                // The zero early-out went with the count: the shell's record
                // carries the guard now (BUNDLE_1), and the shadow twin says
                // the same at draw_shadow_shell.
                draw_indexed_mesh_indirect(pass, shellPipeline_,
                    vertexBuffer, indexBuffer, ledger, ledgerOffset);
            }

            // OIL_1 U13: the gallery pair is bound ONCE by the caller
            // before the wall/frame draws (they share the gallery layout).
            // The instance count is the live mark, not Dim::PAINTING_MAX_SLOTS.
            // The shader culls per slot either way, so the constant meant
            // paying for the ceiling every frame; gallery_frame_vs's bounds
            // guard (B1) makes drawing fewer safe by construction. Both the
            // mark and the `activePaintingCount == 0` guard are in the record
            // now (BUNDLE_1) — zero instances draw nothing.
            template <class Enc>
            void draw_gallery_frames(
                Enc& pass,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                if constexpr (!(ROSTER.gallery)) return;  // ROSTER-GATE gallery (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(galleryFramePipeline_);
                pass.DrawIndirect(ledger, ledgerOffset);
            }

            // Both passes walk vid/PAINTING_FRAME_VERTS_PER as a slot index,
            // so the vertex count is the live mark x the per-frame stride
            // rather than Dim::PAINTING_FRAME_VERTEX_COUNT's ceiling.
            // wall_painting_vs already guards the decoded index (world.wgsl).
            //
            // TWO DRAWS, ONE RECORD: the canvas and the frame walk the same
            // vertices with different pipelines, so their count is one number
            // and gets one home. The `wallFrameCount == 0` guard is in it.
            template <class Enc>
            void draw_wall_paintings(
                Enc& pass,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                if constexpr (!(ROSTER.gallery)) return;  // ROSTER-GATE gallery (a') — pipeline never created; the holder tolerates
                // Canvas pass (textured surface)
                pass.SetPipeline(wallPaintingCanvasPipeline_);
                pass.DrawIndirect(ledger, ledgerOffset);

                // Frame pass (solid color) — same pair, already bound.
                pass.SetPipeline(wallPaintingFramePipeline_);
                pass.DrawIndirect(ledger, ledgerOffset);
            }

            // HALF AN LSB IS THE BOUND (LATTICE_4 R4). The overlay is a
            // fullscreen alpha blend over an 8-bit target: at
            // fadeAlpha < 0.5/255 every channel's blend quantizes back to
            // the destination, so the skipped draw is PIXEL-IDENTICAL to the
            // drawn one — not approximately, exactly. The 0.001 that stood
            // here was an arbitrary epsilon that happened to be smaller;
            // this is the number the argument actually rests on.
            //
            // The caller passes config().fade_alpha — the same value
            // fade_overlay_fs reads — so the gate and the shader cannot
            // disagree about whether the frame is at rest.
            template <class Enc>
            void draw_fade_overlay(
                Enc& pass,
                float fadeAlpha
            ) {
                if constexpr (!(ROSTER.transitions)) return;  // ROSTER-GATE transitions (a') — pipeline never created; the holder tolerates
                if (fadeAlpha < 0.5f / 255.0f) return;
                pass.SetPipeline(fadeOverlayPipeline_);
                pass.Draw(3);  // fullscreen triangle from vertex ID
            }


            // Shared helper for all "indexed mesh" shadow draws. Per-family
            // wrappers below differ only in pipeline + (rarely) instance count.
            // ═══ OIL_1 U12 (ledger: R18, C7) — THE SHADOW PASS-HEAD CONTRACT
            // Every draw_shadow_* below rides binds set ONCE at the pass
            // head by render_shadow_pass: group0 = the render entity
            // group, group1 = the shadow texture group. They were
            // identical at every call (the DrawBind pair), so each
            // helper's own pair was a redundant re-set of unchanged
            // state — ~18-22 per pass, and per atlas tile indoors.
            // Bind-group state is sticky within a pass and all these
            // pipelines share ONE layout (shadowRenderLayout), so every
            // draw sees exactly the groups it saw before. The two
            // gallery draws are NOT under this contract: they carry
            // their own layout and bind their own pair, once, at the
            // tail of draw_shadow_all.
            // The shadow twin of draw_indexed_mesh_indirect. Same record —
            // a family's index count is one number, not one per pass.
            template <class Enc>
            void draw_shadow_indexed_mesh_indirect(
                Enc& pass,
                wgpu::RenderPipeline pipeline,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                pass.SetPipeline(pipeline);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexedIndirect(ledger, ledgerOffset);
            }

            template <class Enc>
            void draw_shadow_indexed_mesh(
                Enc& pass,
                wgpu::RenderPipeline pipeline,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount,
                uint32_t instanceCount = 1,
                uint32_t firstInstance = 0
            ) {
                if (indexCount == 0) return;
                pass.SetPipeline(pipeline);
                pass.SetVertexBuffer(0, vertexBuffer);
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
                pass.DrawIndexed(indexCount, instanceCount, 0, 0, firstInstance);
            }

            // ONE DRAW WHERE THERE WERE TWO (BUNDLE_1, R-G). The two shadow
            // terrain bands shared this pipeline AND this index buffer and
            // differed only in their instance RANGE — [0, lod0) here, then
            // [lod0, render) in a raw DrawIndexed at the call site. Their
            // union is [0, render), which is one draw of render_patch_count
            // instances: the shader sees the same instance_index set either
            // way. Folding them also removes the only non-zero firstInstance
            // on this path, which an indirect draw could not have carried
            // without the `indirect-first-instance` feature.
            template <class Enc>
            void draw_shadow_patch_terrain(
                Enc& pass,
                wgpu::Buffer indexBuffer,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                pass.SetPipeline(shadowPatchTerrainPipeline_);
                // LATTICE_3 — the LOD1 ring IB, uint16 since the patch IBs
                // crossed together.
                pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint16);
                pass.DrawIndexedIndirect(ledger, ledgerOffset);
            }

            template <class Enc>
            void draw_shadow_pawn(
                Enc& pass,
                uint32_t vertexCount
            ) {
                pass.SetPipeline(shadowPawnPipeline_);
                pass.Draw(vertexCount, /*instanceCount=*/ Dim::MAX_AGENTS);
            }

            template <class Enc>
            void draw_shadow_sphere(
                Enc& pass,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.sphere)) return;  // ROSTER-GATE sphere (a') — pipeline never created; the holder tolerates
                draw_shadow_indexed_mesh(pass, shadowSpherePipeline_,
                    vertexBuffer, indexBuffer, indexCount,
                    Dim::MAX_SPHERE_INSTANCES);
            }

            template <class Enc>
            void draw_shadow_monolith(
                Enc& pass,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                uint32_t indexCount
            ) {
                if constexpr (!(ROSTER.cube)) return;  // ROSTER-GATE cube (a') — pipeline never created; the holder tolerates
                draw_shadow_indexed_mesh(pass, shadowMonolithPipeline_,
                    vertexBuffer, indexBuffer, indexCount,
                    Dim::MAX_CUBE_INSTANCES, Dim::CUBE_SLOT_OFFSET);
            }

            template <class Enc>
            void draw_shadow_ribbon(
                Enc& pass,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                if constexpr (!(ROSTER.ribbon)) return;  // ROSTER-GATE ribbon (a') — pipeline never created; the holder tolerates
                pass.SetPipeline(shadowRibbonPipeline_);
                pass.DrawIndirect(ledger, ledgerOffset);
            }

            template <class Enc>
            void draw_shadow_arch(
                Enc& pass,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                if constexpr (!(ROSTER.arch)) return;  // ROSTER-GATE arch (a') — pipeline never created; the holder tolerates
                draw_shadow_indexed_mesh_indirect(pass, shadowArchPipeline_,
                    vertexBuffer, indexBuffer, ledger, ledgerOffset);
            }

            template <class Enc>
            void draw_shadow_column(
                Enc& pass,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                if constexpr (!(ROSTER.column || ROSTER.antenna)) return;  // ROSTER-GATE column+antenna (shared pipelines) (a') — pipeline never created; the holder tolerates
                draw_shadow_indexed_mesh_indirect(pass, shadowColumnPipeline_,
                    vertexBuffer, indexBuffer, ledger, ledgerOffset);
            }

            template <class Enc>
            void draw_shadow_palm(
                Enc& pass,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                if constexpr (!(ROSTER.palm)) return;  // ROSTER-GATE palm (a') — pipeline never created; the holder tolerates
                draw_shadow_indexed_mesh_indirect(pass, shadowPalmPipeline_,
                    vertexBuffer, indexBuffer, ledger, ledgerOffset);
            }

            template <class Enc>
            void draw_shadow_cactus(
                Enc& pass,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                if constexpr (!(ROSTER.cactus)) return;  // ROSTER-GATE cactus (a') — pipeline never created; the holder tolerates
                draw_shadow_indexed_mesh_indirect(pass, shadowCactusPipeline_,
                    vertexBuffer, indexBuffer, ledger, ledgerOffset);
            }

            template <class Enc>
            void draw_shadow_blade(
                Enc& pass,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                if constexpr (!(ROSTER.blade)) return;  // ROSTER-GATE blade (a') — pipeline never created; the holder tolerates
                draw_shadow_indexed_mesh_indirect(pass, shadowBladePipeline_,
                    vertexBuffer, indexBuffer, ledger, ledgerOffset);
            }

            // draw_shadow_pyramid CUT — caller-free

            template <class Enc>
            void draw_shadow_shell(
                Enc& pass,
                wgpu::Buffer vertexBuffer,
                wgpu::Buffer indexBuffer,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                if constexpr (!(ROSTER.indoor_shell)) return;  // ROSTER-GATE indoor_shell (a') — pipeline never created; the holder tolerates
                // Record-guarded, as the colour twin is (BUNDLE_1).
                draw_shadow_indexed_mesh_indirect(pass, shadowShellPipeline_,
                    vertexBuffer, indexBuffer, ledger, ledgerOffset);
            }

            // OIL_1 U12: the gallery pair is bound ONCE by the caller
            // before these two draws (they share galleryShadowLayout and
            // sit at the tail of draw_shadow_all).
            template <class Enc>
            void draw_shadow_gallery_frames(
                Enc& pass,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                if constexpr (!(ROSTER.gallery)) return;
                pass.SetPipeline(shadowGalleryFramePipeline_);
                pass.DrawIndirect(ledger, ledgerOffset);
            }

            template <class Enc>
            void draw_shadow_wall_paintings(
                Enc& pass,
                wgpu::Buffer ledger,
                uint64_t ledgerOffset
            ) {
                if constexpr (!(ROSTER.gallery)) return;
                // ONE draw where the color pass needs two: with no fragment
                // stage the canvas/frame split has nothing to distinguish.
                // Same record as the colour pass — one vertex count.
                pass.SetPipeline(shadowWallPaintingPipeline_);
                pass.DrawIndirect(ledger, ledgerOffset);
            }

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
                if (!(ROSTER.gallery)) n += 6;
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

                // ═══ PROBATE_SEAL2 — THE SERVE WITNESS ═══════════════
                //
                // BEFORE the module is created, compare the shader this
                // program RECEIVED against the shader the build SHIPPED.
                //
                // The Pixel was served a world.wgsl cut mid-token 665
                // bytes from its end and reported `:13639:28 expected
                // ')'` inside orb_vs — a syntax error in a file that has
                // none. The tree was clean, --check was green, the WGSL
                // gate was green, and the packaging copies the bundle
                // byte for byte. Every witness the program owned was
                // looking at the right thing and none of them was
                // standing in the corridor where the bytes actually go
                // wrong: build -> dist -> upload -> CDN -> device.
                //
                // web_dist.py bakes sha256(world.wgsl)[:8] into the page
                // at deploy; this hashes what MEMFS actually handed us.
                // Truncation, a stale the_board.data, a half-written CDN
                // object and a cache serving last week's bytes all land
                // here as one legible line instead of as a parse error
                // three hundred lines from the real problem.
                //
                // NATIVE/OFFLINE: no page, no digest, nothing to
                // compare. `expected` is then empty and the line says
                // UNWITNESSED — a fact, not a failure. The floor treats
                // it as satisfied, because a shader read off local disk
                // never crossed the corridor this witness watches.
                const std::string got = t7::sha256_short(shaderSource_);
                std::string expected;
#ifdef __EMSCRIPTEN__
                {
                    // One EM_ASM, bytes into a stack buffer through
                    // HEAPU8 — boot_params.hpp's pattern exactly (it
                    // writes doubles through HEAPF64). Deliberately NOT
                    // stringToNewUTF8/EM_ASM_PTR: those need a runtime
                    // helper to be present in the shipped glue, and F5F
                    // is this program's standing lesson about betting on
                    // an API the payload may not carry. A digest is
                    // 8 ASCII hex characters; a 16-byte buffer and a
                    // hand-written copy need nothing but the heap view.
                    // DOUBLE QUOTES, AND THEY ARE NOT A STYLE CHOICE.
                    // The preprocessor lexes an EM_ASM body as pp-tokens
                    // before any JS engine ever sees it, and `''` is an
                    // EMPTY CHARACTER CONSTANT — invalid, warned by
                    // -Winvalid-pp-token, and it warned twice here from
                    // the day SEAL wrote it. JS reads "" identically.
                    // ('string' and '__' stay: multi-character constants
                    // are valid pp-tokens, and the minimal diff is law.)
                    char buf[16] = {};
                    EM_ASM({
                        var s = (typeof SHADER_SHA === 'string') ? SHADER_SHA : "";
                        // An unsubstituted placeholder is NOT a digest —
                        // it means someone opened web/index.html directly
                        // instead of the dist/ copy web_dist generates.
                        // Report absent rather than mismatched.
                        if (s.indexOf('__') === 0) s = "";
                        var n = Math.min(s.length, $1 - 1);
                        for (var i = 0; i < n; i++)
                            HEAPU8[$0 + i] = s.charCodeAt(i) & 0x7f;
                        HEAPU8[$0 + n] = 0;
                    }, buf, static_cast<int>(sizeof(buf)));
                    buf[sizeof(buf) - 1] = '\0';
                    expected = buf;
                }
#endif
                shaderBytes_ = shaderSource_.size();
                shaderSha_ = got;

                // PURSE_0 R2 — THE TREE, BESIDE THE ARTIFACT. `[Dist]` below
                // answers "are these the bytes this build shipped"; this
                // answers "which tree was that build". Neither implies the
                // other: a matching digest proves the browser got what the
                // build produced and says nothing about WHICH tree produced
                // it, and a git sha says nothing about a cached predecessor.
                // Printed HERE so the two provenance lines are adjacent in a
                // pasted log — scattered provenance is provenance nobody
                // reads. `unknown` where the stamp did not generate: the one
                // thing this line may never do is lie.
                std::cout << "[Build] " << t7::BUILD_STAMP << "\n";
                // IOS_3 B2 — the same two provenance facts onto the page.
                // A photograph of a broken iPad is the only instrument in
                // that room, and it is worthless if it cannot name the
                // build it photographed.
                t7::card_fact(std::string("build  ") + t7::BUILD_STAMP);
                t7::card_fact("shader " + got + "  ("
                              + std::to_string(shaderSource_.size()) + " bytes)");

                if (expected.empty()) {
                    // A SKIP THAT SAYS ITS NAME. Serving web/ directly —
                    // the dev path — means no web_dist run has baked a
                    // digest, so the placeholder is still sitting in the
                    // page and there is nothing to compare against. That
                    // is not a match and it is not a mismatch: it is a
                    // question that was never asked, and the line says
                    // so. Never a silent pass (which would let a real
                    // corruption through on the day someone forgot to
                    // run web_dist), and never a false MISMATCH (which
                    // would stop every dev serve and get the witness
                    // switched off within a week). The floor does NOT
                    // stop here — the dev path never crossed the
                    // corridor this witness watches.
#ifdef __EMSCRIPTEN__
                    std::cout << "[Dist] world.wgsl sha=" << got
                        << " expected=none (dev serve — placeholder unsubstituted)"
                           " SKIPPED\n";
#else
                    std::cout << "[Dist] world.wgsl sha=" << got
                        << " expected=none (native — read off disk, no serve to"
                           " witness) SKIPPED\n";
#endif
                    serveWitnessed_ = true;
                }
                else if (expected == got) {
                    std::cout << "[Dist] world.wgsl sha=" << got
                        << " expected=" << expected << " MATCH ("
                        << shaderSource_.size() << " bytes)\n";
                    serveWitnessed_ = true;
                }
                else {
                    // THE STOP, not a warning. A module built from bytes
                    // the build did not ship is a module whose every
                    // downstream error is misdirection.
                    std::cout << "[Dist] world.wgsl sha=" << got
                        << " expected=" << expected << " MISMATCH ("
                        << shaderSource_.size() << " bytes received)\n";
                    std::cout << "[Floor] STOP — the shader this device received is not the"
                                 " shader this build shipped. No module is created; no"
                                 " pipeline is compiled; no frame runs. Hard-refresh first"
                                 " (a truncated fetch is the cheapest explanation), then"
                                 " redeploy. PROBATE_SEAL2.\n";
                    serveWitnessed_ = false;
                    return false;
                }

                wgpu::ShaderSourceWGSL wgslSource{};
                wgslSource.code = shaderSource_.c_str();

                wgpu::ShaderModuleDescriptor desc{};
                desc.nextInChain = &wgslSource;
                desc.label = "world.wgsl (The_Board Cartridge)";

                auto tShader0 = std::chrono::high_resolution_clock::now();
                shaderModule_ = device_.CreateShaderModule(&desc);
                auto tShader1 = std::chrono::high_resolution_clock::now();
                std::cout << "[Renderer] Shader compile (create call):    "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(tShader1 - tShader0).count()
                    << " ms\n";
                return shaderModule_ != nullptr;
            }

            // ═══ PROBATE_SEAL3 — THE TIMER TELLS THE TRUTH ═══════════
            //
            // CreateShaderModule is ASYNCHRONOUS about errors: it hands
            // back a non-null module immediately and reports the parse
            // result later, through GetCompilationInfo. So on the Pixel
            // — with a shader cut mid-token — every one of these came
            // out looking like a healthy boot:
            //
            //   [Renderer] Shader compile: 3 ms      (a create call)
            //   [Pipeline] gen_patch_heights: 0 ms   (never compiled)
            //   … fifty-eight more zeroes …
            //
            // Zero milliseconds is what a pipeline built on a broken
            // module costs, and printed in a column of successes it
            // reads as "fast", not as "absent". The timing line already
            // says "create call" now; this is the other half — the
            // module's own verdict, waited for, BEFORE any pipeline is
            // built on it. Nothing downstream gets to print a number
            // about a module that did not compile.
            //
            // The wait is a spin on the device queue, which is what this
            // boot path already is (synchronous, pre-frame-loop). On the
            // web that is the browser's own event loop turning; there is
            // no frame in flight to stall.
            // IT CANNOT BE AWAITED ON THIS TWIN, and pretending otherwise
            // would be the worse defect. GetCompilationInfo resolves on
            // the browser's event loop; the boot path is synchronous and
            // the build carries no -sASYNCIFY (CMakeLists), so a spin
            // waiting for the verdict would turn a wrong shader into a
            // hung tab. WaitAny with a timeout needs the same event loop
            // and TimedWaitAny is not offered on this surface.
            //
            // So the request is FIRED here and the verdict lands when it
            // lands — after pipeline creation, in the common case. What
            // that buys is still the whole of the Pixel's confusion:
            // the real errors, with their real line numbers, printed
            // once, next to a `[Dist]` line that says whether the bytes
            // were even the right bytes. And an error raises `[Floor]
            // STOP`, so the visitor gets the card rather than a black
            // canvas under a scrolling log.
            //
            // The corruption case — the one that actually happened — is
            // already stopped SYNCHRONOUSLY and earlier, by the digest
            // check in loadShader(): no module is created at all. This
            // arm is the net under everything else a module can fail at,
            // and it is honest about being a net rather than a gate.
            void requestShaderVerdict() {
                if (!shaderModule_) return;
                shaderModule_.GetCompilationInfo(
                    wgpu::CallbackMode::AllowSpontaneous,
                    [](wgpu::CompilationInfoRequestStatus status,
                       wgpu::CompilationInfo const* info) {
                        if (status != wgpu::CompilationInfoRequestStatus::Success) {
                            // The QUESTION failed, not the shader. An
                            // unanswered question is not a verdict, and
                            // must not be reported as one.
                            std::cout << "[Shader] compilation info unavailable —"
                                         " the module is unwitnessed\n";
                            return;
                        }
                        if (!info) return;
                        uint32_t errors = 0, warnings = 0;
                        for (size_t i = 0; i < info->messageCount; i++) {
                            const auto& m = info->messages[i];
                            const std::string text(m.message.data, m.message.length);
                            if (m.type == wgpu::CompilationMessageType::Error) {
                                errors++;
                                std::cout << "[Shader] ERROR " << m.lineNum << ":"
                                    << m.linePos << " " << text << "\n";
                            } else if (m.type == wgpu::CompilationMessageType::Warning) {
                                warnings++;
                                std::cout << "[Shader] warning " << m.lineNum << ":"
                                    << m.linePos << " " << text << "\n";
                            }
                        }
                        std::cout << "[Shader] world.wgsl compiled: " << errors
                            << " error(s), " << warnings << " warning(s)\n";
                        if (errors) {
                            std::cout << "[Floor] STOP — world.wgsl did not compile."
                                         " Every [Pipeline] number in this log is a"
                                         " create call on a module that never compiled,"
                                         " not a compile time. If the errors above name"
                                         " a line near the END of the file, read the"
                                         " [Dist] line first: a truncated serve reads"
                                         " exactly like a syntax error."
                                         " PROBATE_SEAL3.\n";
                        }
                    });
            }

            bool createComputePipelines() {
                // The FRAME_K pipeline layout: WORLD + FRAME_C + the frame-k
                // pair. Serves update_camera_vp — the one kernel that writes
                // the frame's vp/camera state (two, before SPINE_2 fused them).
                wgpu::PipelineLayout frameKComputeLayout = strataLayoutFor("frameKComputeLayout", frameCLayout_, frameKStateLayout_, frameKTexturesLayout_);
                if (!frameKComputeLayout) return false;

                // THE ROOM (Option B, Batch F; FIELD_2 amendment): the two
                // agent kernels AND the two floater kernels carry group 2
                // on top of the shared live-contributor pair. The camera
                // keeps the two-group layout untouched, so tenant-side
                // binding growth (the occupier windows, the field pair)
                // never widens its compile surface.
                wgpu::PipelineLayout roomComputeLayout = strataLayoutFor("roomComputeLayout", frameCLayout_, agentsStateLayout_, agentsTexturesLayout_);
                if (!roomComputeLayout) return false;

                // Pipeline: update_player_agent (0D, 1 thread — possessed slot only)
                // Room layout (live-contributor pair + the room) —
                // pawn_ground_resolve, terrain_normal_at
                // call query_ground_walker → contrib_pawn_aura_at → sample_pawn_aura.
                // The walker-policy heavy path inlines once, for one slot.
                if (!makeComputePipeline("update_player_agent", "Update Player Agent (0D, 1 thread)",
                    roomComputeLayout, Entry::UPDATE_PLAYER_AGENT, updatePlayerAgentPipeline_)) return false;

                // Pipeline: update_other_agents (1D, one thread per field lane)
                // Room layout (live-contributor pair + the room) —
                // query_ground_walker_agent reads aura
                // grid via contrib_pawn_aura_at_external → sample_pawn_aura.
                // The walker-policy heavy path is NOT inlined here; algorithmic
                // behaviors only.
                if constexpr (ROSTER.wanderers) {  // ROSTER-GATE wanderers (a') — shader compile skipped when disabled
                if (!makeComputePipeline("update_other_agents", "Update Other Agents (1D, one thread per field lane)",
                    roomComputeLayout, Entry::UPDATE_OTHER_AGENTS, updateOtherAgentsPipeline_)) return false;
                }

                // Pipeline: update_camera_vp (0D) — SPINE_2 fused the pair.
                // Live-contributor layout — the camera clamp uses a walker-style
                // policy that reads the aura texture (sample_pawn_aura); the VP
                // half needs the same strata, which is why they fuse cleanly.
                if (!makeComputePipeline("update_camera_vp", "Update Camera + VP (0D, one lane)",
                    frameKComputeLayout, Entry::UPDATE_CAMERA_VP, cameraVPPipeline_)) return false;

                // Pipeline: update_sphere (0D)
                // Room layout (FIELD_2 tenancy) — coupling_terrain_to_sphere_orbit_height
                // still calls query_ground_flyer (→ contrib_pawn_aura_at → sample_pawn_aura);
                // group 2 adds the field bindings. Unused group members are legal.
                if constexpr (ROSTER.sphere) {  // ROSTER-GATE sphere (a') — shader compile skipped when disabled
                if (!makeComputePipeline("update_sphere", "Update Sphere (0D)",
                    roomComputeLayout, Entry::UPDATE_SPHERE, updateSpherePipeline_)) return false;
                }

                // Pipeline: update_cube (0D)
                // Room layout (FIELD_2 tenancy) — update_cube calls
                // query_ground_flyer directly for hover-base clearance;
                // group 2 adds the field bindings.
                if constexpr (ROSTER.cube) {  // ROSTER-GATE cube (a') — shader compile skipped when disabled
                if (!makeComputePipeline("update_cube", "Update Cube (0D)",
                    roomComputeLayout, Entry::UPDATE_CUBE, updateCubePipeline_)) return false;
                }

                // Pipeline: bake_patch_heightfield (LATTICE_1 — one kernel where
                // two stood; workgroup_id.z selects the patch in the batch)
                {
                    wgpu::PipelineLayout pl = strataLayoutFor("patchgenComputeLayout", frameCLayout_, patchgenStateLayout_, patchgenTexturesLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("bake_patch", "Patch Bake (fused, batched)",
                        pl, Entry::BAKE_PATCH_HEIGHTFIELD, bakePatchPipeline_)) return false;
                }

                // Pipeline: generate_patch_cells (2D, on demand)
                {
                    wgpu::PipelineLayout pl = strataLayoutFor("patchgenComputeLayout", frameCLayout_, patchgenStateLayout_, patchgenTexturesLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("gen_patch_cells", "Generate Patch Cells (2D, on demand)",
                        pl, Entry::GENERATE_PATCH_CELLS, generatePatchCellsPipeline_)) return false;
                }

                // Pipelines: the ribbon room's two kernels (RIBBON_1) — the head
                // (one thread) then the body (one per ring), on ONE layout.
                if constexpr (ROSTER.ribbon) {  // ROSTER-GATE ribbon (a') — shader compile skipped when disabled
                    wgpu::PipelineLayout pl = strataLayoutFor("ribbonComputeLayout", frameCLayout_, ribbonStateLayout_, ribbonTexturesLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("ribbon_head", "Ribbon Head (0D, 1 thread, per frame)",
                        pl, Entry::RIBBON_HEAD, ribbonHeadPipeline_)) return false;
                    if (!makeComputePipeline("ribbon_body", "Ribbon Body (1D, per ring, per frame)",
                        pl, Entry::RIBBON_BODY, ribbonBodyPipeline_)) return false;
                }

                // Photographer VP compute pipeline (0D, reads pawn → writes VP)
                // A7: the PHOTO_K strata — the photographer's compute working set,
                // split from GALLERY so the render stratum stays read-only (L23).
                if constexpr (ROSTER.gallery) {  // ROSTER-GATE gallery (a') — shader compile skipped when disabled
                    wgpu::PipelineLayout pl = strataLayoutFor("photoKComputeLayout", frameCLayout_, photoKStateLayout_, photoKTexturesLayout_);
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
                    wgpu::PipelineLayout pl = strataLayoutFor("placeComputeLayout", frameCLayout_, placeStateLayout_, placeTexturesLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("compute_entity_placement", "Compute Entity Placement (0D)",
                        pl, Entry::COMPUTE_ENTITY_PLACEMENT, entityPlacementPipeline_)) return false;
                }

                // GPU frustum cull pipeline (dedicated layout)
                {
                    wgpu::PipelineLayout pl = strataLayoutFor("cullComputeLayout", frameCLayout_, cullStateLayout_, emptyLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("frustum_cull_patches", "Frustum Cull Patches",
                        pl, Entry::FRUSTUM_CULL_PATCHES, frustumCullPipeline_)) return false;
                }

                // Pawn aura compute pipeline (dedicated layout)
                if constexpr (ROSTER.pawn_aura) {  // ROSTER-GATE pawn_aura (a') — shader compile skipped when disabled
                    wgpu::PipelineLayout pl = strataLayoutFor("auraComputeLayout", frameCLayout_, auraStateLayout_, auraTexturesLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("compute_pawn_aura", "Compute Pawn Aura (2D)",
                        pl, Entry::COMPUTE_PAWN_AURA, pawnAuraPipeline_)) return false;
                }

                // The live card writer — ONE pipeline since LATTICE_4 fused
                // the pair (the patch-gen fusion, at card size)
                {
                    wgpu::PipelineLayout pl = strataLayoutFor("zonesComputeLayout", frameCLayout_, zonesStateLayout_, zonesTexturesLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("write_live_card", "Live Card Write (2D, fused)",
                        pl, Entry::WRITE_LIVE_CARD, liveCardPipeline_)) return false;
                }

                // Orb compute pipelines (init + dynamics + recolor share the A face set)
                if constexpr (ROSTER.orbs) {  // ROSTER-GATE orbs (a') — shader compile skipped when disabled
                    wgpu::PipelineLayout pl = strataLayoutFor("orbsAComputeLayout", frameCLayout_, orbsAStateLayout_, emptyLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("orb_init", "Orb Init", pl, Entry::ORB_INIT, orbInitPipeline_)) return false;
                    if (!makeComputePipeline("orb_dynamics", "Orb Dynamics", pl, Entry::ORB_DYNAMICS, orbDynamicsPipeline_)) return false;
                    if (!makeComputePipeline("orb_recolor", "Orb Recolor", pl, Entry::ORB_RECOLOR, orbRecolorPipeline_)) return false;
                }

                // Orb copy-prev pipeline (Pass 9) — the B face set: it flips the
                // access modes on orb_state / orb_state_prev (A8b restores the
                // partition the recut collapsed).
                if constexpr (ROSTER.orbs) {  // ROSTER-GATE orbs (a') — shader compile skipped when disabled
                    wgpu::PipelineLayout pl = strataLayoutFor("orbsBComputeLayout", frameCLayout_, orbsBStateLayout_, emptyLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("orb_state_prev_copy", "Orb State Prev Copy",
                        pl, Entry::ORB_STATE_PREV_COPY, orbCopyPrevPipeline_)) return false;
                }

                // GoL zone compute pipelines (dedicated layout, z-dispatched)
                if constexpr (ROSTER.gol) {  // ROSTER-GATE gol (a') — shader compile skipped when disabled
                    wgpu::PipelineLayout pl = strataLayoutFor("zonesComputeLayout", frameCLayout_, zonesStateLayout_, zonesTexturesLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("zone_gol_sync", "GoL Zone Sync", pl, Entry::ZONE_GOL_SYNC, zoneGolSyncPipeline_)) return false;
                    if (!makeComputePipeline("zone_gol_evolve", "GoL Zone Evolve", pl, Entry::ZONE_GOL_EVOLVE, zoneGolEvolvePipeline_)) return false;
                }

                // Zone derive pipeline (shared GoL layout)
                if constexpr (ROSTER.gol) {  // ROSTER-GATE gol (a') — shader compile skipped when disabled
                    wgpu::PipelineLayout pl = strataLayoutFor("zonesComputeLayout", frameCLayout_, zonesStateLayout_, zonesTexturesLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("zone_derive_params", "Zone Derive Params", pl, Entry::ZONE_DERIVE_PARAMS, zoneDeriveParamsPipeline_)) return false;
                }

                // Zone mask pipeline (dedicated layout — UNIFIED_GROUND_1 U5)
                if constexpr (ROSTER.gol) {  // ROSTER-GATE gol (a') — shader compile skipped when disabled
                    wgpu::PipelineLayout pl = strataLayoutFor("zonesComputeLayout", frameCLayout_, zonesStateLayout_, zonesTexturesLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("zone_seed_mask", "Zone Seed Mask (2D)",
                        pl, Entry::ZONE_SEED_MASK, zoneSeedMaskPipeline_)) return false;
                }

                // Mesh-gen compute pipelines — one dedicated single-group layout each,
                // per-family ROSTER gate; identical creation modulo (layout, entry, member).
                // pyramid mesh-gen pipeline CUT — mesh never drawn.

                if constexpr (ROSTER.arch) {  // ROSTER-GATE arch (a') — shader compile skipped when disabled
                    wgpu::PipelineLayout pl = strataLayoutFor("meshgenComputeLayout", frameCLayout_, meshgenStateLayout_, emptyLayout_);   // bindings 193-195
                    if (!pl) return false;
                    if (!makeComputePipeline("arch_mesh_gen", "Arch Mesh Gen",
                        pl, Entry::ARCH_MESH_GEN, archMeshGenPipeline_)) return false;
                }

                if constexpr (ROSTER.column || ROSTER.antenna) {  // ROSTER-GATE column+antenna (shared pipelines) (a') — shader compile skipped when disabled
                    wgpu::PipelineLayout pl = strataLayoutFor("meshgenComputeLayout", frameCLayout_, meshgenStateLayout_, emptyLayout_);  // bindings 196-198
                    if (!pl) return false;
                    if (!makeComputePipeline("column_mesh_gen", "Column Mesh Gen",
                        pl, Entry::COLUMN_MESH_GEN, columnMeshGenPipeline_)) return false;
                }

                if constexpr (ROSTER.palm) {  // ROSTER-GATE palm (a') — shader compile skipped when disabled
                    wgpu::PipelineLayout pl = strataLayoutFor("meshgenComputeLayout", frameCLayout_, meshgenStateLayout_, emptyLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("palm_mesh_gen", "Palm Mesh Gen",
                        pl, Entry::PALM_MESH_GEN, palmMeshGenPipeline_)) return false;
                }

                if constexpr (ROSTER.cactus) {  // ROSTER-GATE cactus (a') — shader compile skipped when disabled
                    wgpu::PipelineLayout pl = strataLayoutFor("meshgenComputeLayout", frameCLayout_, meshgenStateLayout_, emptyLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("cactus_mesh_gen", "Cactus Mesh Gen",
                        pl, Entry::CACTUS_MESH_GEN, cactusMeshGenPipeline_)) return false;
                }

                if constexpr (ROSTER.blade) {  // ROSTER-GATE blade (a') — shader compile skipped when disabled
                    wgpu::PipelineLayout pl = strataLayoutFor("meshgenComputeLayout", frameCLayout_, meshgenStateLayout_, emptyLayout_);
                    if (!pl) return false;
                    if (!makeComputePipeline("blade_cluster_mesh_gen", "Blade Mesh Gen",
                        pl, Entry::BLADE_MESH_GEN, bladeMeshGenPipeline_)) return false;
                }

                return true;
            }

            bool createRenderPipelines() {
                // Shadow pipeline layout (entity + textures WITHOUT shadow map)
                wgpu::PipelineLayout shadowRenderLayout = strataLayoutFor("shadowRenderLayout", frameRLayout_, shadowStateLayout_, shadowTexturesLayout_);
                if (!shadowRenderLayout) return false;

                // Main render pipeline layout (entity + textures WITH shadow map)
                wgpu::PipelineLayout renderLayout = strataLayoutFor("renderLayout", frameRLayout_, sceneStateLayout_, sceneTexturesLayout_);
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
                    desc.multisample.count = effective_msaa();   // B10: 1 = the default, byte-identical descriptor
                    return tPipe(label, [&]() {
                        out = device_.CreateRenderPipeline(&desc);
                        return out != nullptr;
                    });
                };

                // Patch terrain pipeline -- instanced. DOMESDAY_0 B3: one
                // instance-step vertex buffer (the visible list, u32 per
                // instance). The direct variant leaves the attribute unread
                // (USE_PATCH_INDIRECTION=false) but the entry point declares
                // it, so every variant's vertex state must supply location 0.
                {
                    std::array<wgpu::VertexAttribute, 1> visAttrs{};
                    visAttrs[0].format = wgpu::VertexFormat::Uint32;
                    visAttrs[0].offset = 0;
                    visAttrs[0].shaderLocation = 0;

                    wgpu::VertexBufferLayout visVBL{};
                    visVBL.arrayStride = 4;
                    visVBL.stepMode = wgpu::VertexStepMode::Instance;
                    visVBL.attributeCount = visAttrs.size();
                    visVBL.attributes = visAttrs.data();

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
                    desc.vertex.bufferCount = 1;
                    desc.vertex.buffers = &visVBL;
                    desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                    desc.primitive.cullMode = wgpu::CullMode::Back;
                    desc.primitive.frontFace = wgpu::FrontFace::CCW;
                    desc.depthStencil = &depthStencil;
                    desc.fragment = &fragment;
                    desc.multisample.count = effective_msaa();   // B10: 1 = the default, byte-identical descriptor

                    if (!tPipe("patch_terrain", [&]() {
                        patchTerrainPipeline_ = device_.CreateRenderPipeline(&desc);
                        return patchTerrainPipeline_ != nullptr;
                    })) return false;
                }

                // Indirect terrain variant — USE_PATCH_INDIRECTION=true.
                // DOMESDAY_0 B3: VS reads patch_instances[visible_id], the
                // visible id arriving as the instance-step attribute below
                // (the g2:62 storage seat is retired; the per-slot segment
                // window rides the SetVertexBuffer offset).
                {
                    std::array<wgpu::VertexAttribute, 1> visAttrs{};
                    visAttrs[0].format = wgpu::VertexFormat::Uint32;
                    visAttrs[0].offset = 0;
                    visAttrs[0].shaderLocation = 0;

                    wgpu::VertexBufferLayout visVBL{};
                    visVBL.arrayStride = 4;
                    visVBL.stepMode = wgpu::VertexStepMode::Instance;
                    visVBL.attributeCount = visAttrs.size();
                    visVBL.attributes = visAttrs.data();

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
                    desc.vertex.bufferCount = 1;
                    desc.vertex.buffers = &visVBL;
                    desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                    desc.primitive.cullMode = wgpu::CullMode::Back;
                    desc.primitive.frontFace = wgpu::FrontFace::CCW;
                    desc.depthStencil = &depthStencil;
                    desc.fragment = &fragment;
                    desc.multisample.count = effective_msaa();   // B10: 1 = the default, byte-identical descriptor

                    if (!tPipe("patch_terrain_indirect", [&]() {
                        patchTerrainIndirectPipeline_ = device_.CreateRenderPipeline(&desc);
                        return patchTerrainIndirectPipeline_ != nullptr;
                    })) return false;
                }

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
                    if constexpr (ROSTER.sphere) {  // ROSTER-GATE sphere (a') — shader compile skipped when disabled
                    if (!makeEntity("sphere", "Sphere Entity (Rasterized)", Entry::SPHERE_VS,
                        &meshVBL, wgpu::CullMode::Back, spherePipeline_)) return false;
                    }
                    if constexpr (ROSTER.cube) {  // ROSTER-GATE cube (a') — shader compile skipped when disabled
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
                    if constexpr (ROSTER.arch) {  // ROSTER-GATE arch (a') — shader compile skipped when disabled
                    if (!makeEntity("arch", "Catenary Arch (Rasterized)", Entry::ARCH_VS,
                        &archVBL, wgpu::CullMode::Back, archPipeline_)) return false;
                    }
                    if constexpr (ROSTER.column || ROSTER.antenna) {  // ROSTER-GATE column+antenna (shared pipelines) (a') — shader compile skipped when disabled
                    if (!makeEntity("column", "Generative Column (Rasterized)", Entry::COLUMN_VS,
                        &archVBL, wgpu::CullMode::None, columnPipeline_)) return false;
                    }
                    if constexpr (ROSTER.palm) {  // ROSTER-GATE palm (a') — shader compile skipped when disabled
                    if (!makeEntity("palm", "Palm Tree (Rasterized)", Entry::PALM_VS,
                        &archVBL, wgpu::CullMode::None, palmPipeline_)) return false;
                    }
                    if constexpr (ROSTER.cactus) {  // ROSTER-GATE cactus (a') — shader compile skipped when disabled
                    if (!makeEntity("cactus", "Cactus (Rasterized)", Entry::CACTUS_VS,
                        &archVBL, wgpu::CullMode::None, cactusPipeline_)) return false;
                    }
                    if constexpr (ROSTER.blade) {  // ROSTER-GATE blade (a') — shader compile skipped when disabled
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
                    if constexpr (ROSTER.indoor_shell) {  // ROSTER-GATE indoor_shell (a') — shader compile skipped when disabled
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
                    desc.multisample.count = effective_msaa();   // B10: 1 = the default, byte-identical descriptor

                    if constexpr (ROSTER.ribbon) {  // ROSTER-GATE ribbon (a') — shader compile skipped when disabled
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

                    // ORB_V: OrbState arrives as an INSTANCE-STEP vertex buffer
                    // rather than a VS storage binding — one attribute per
                    // OrbState field, offsets mirroring the struct (world.wgsl
                    // `struct OrbState` / `GPUOrbState`, state.hpp, 80 B), so
                    // orb_vs reconstructs the value it used to fetch. Frees the
                    // vertex stage's binding 400 seat family-wide; see
                    // BINDING_LEDGER Table F.
                    std::array<wgpu::VertexAttribute, 12> orbStateAttrs{};
                    orbStateAttrs[0].format = wgpu::VertexFormat::Float32x3;  // pos
                    orbStateAttrs[0].offset = 0;
                    orbStateAttrs[0].shaderLocation = 1;
                    orbStateAttrs[1].format = wgpu::VertexFormat::Float32;    // _pad0
                    orbStateAttrs[1].offset = 12;
                    orbStateAttrs[1].shaderLocation = 2;
                    orbStateAttrs[2].format = wgpu::VertexFormat::Float32x3;  // vel
                    orbStateAttrs[2].offset = 16;
                    orbStateAttrs[2].shaderLocation = 3;
                    orbStateAttrs[3].format = wgpu::VertexFormat::Float32;    // _pad1
                    orbStateAttrs[3].offset = 28;
                    orbStateAttrs[3].shaderLocation = 4;
                    orbStateAttrs[4].format = wgpu::VertexFormat::Float32x3;  // base_color
                    orbStateAttrs[4].offset = 32;
                    orbStateAttrs[4].shaderLocation = 5;
                    orbStateAttrs[5].format = wgpu::VertexFormat::Float32;    // brightness
                    orbStateAttrs[5].offset = 44;
                    orbStateAttrs[5].shaderLocation = 6;
                    orbStateAttrs[6].format = wgpu::VertexFormat::Float32x3;  // current_color
                    orbStateAttrs[6].offset = 48;
                    orbStateAttrs[6].shaderLocation = 7;
                    orbStateAttrs[7].format = wgpu::VertexFormat::Float32;    // twinkle_phase
                    orbStateAttrs[7].offset = 60;
                    orbStateAttrs[7].shaderLocation = 8;
                    orbStateAttrs[8].format = wgpu::VertexFormat::Float32;    // size
                    orbStateAttrs[8].offset = 64;
                    orbStateAttrs[8].shaderLocation = 9;
                    orbStateAttrs[9].format = wgpu::VertexFormat::Float32;    // mass
                    orbStateAttrs[9].offset = 68;
                    orbStateAttrs[9].shaderLocation = 10;
                    orbStateAttrs[10].format = wgpu::VertexFormat::Float32;   // drag
                    orbStateAttrs[10].offset = 72;
                    orbStateAttrs[10].shaderLocation = 11;
                    orbStateAttrs[11].format = wgpu::VertexFormat::Uint32;    // tier_idx
                    orbStateAttrs[11].offset = 76;
                    orbStateAttrs[11].shaderLocation = 12;

                    wgpu::VertexBufferLayout orbStateVBL{};
                    orbStateVBL.arrayStride = 80;  // sizeof(GPUOrbState)
                    orbStateVBL.stepMode = wgpu::VertexStepMode::Instance;
                    orbStateVBL.attributeCount = orbStateAttrs.size();
                    orbStateVBL.attributes = orbStateAttrs.data();

                    std::array<wgpu::VertexBufferLayout, 2> orbVBLs{{ orbVBL, orbStateVBL }};

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
                    desc.vertex.bufferCount = orbVBLs.size();
                    desc.vertex.buffers = orbVBLs.data();
                    desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                    desc.primitive.cullMode = wgpu::CullMode::None;  // billboards face camera
                    desc.primitive.frontFace = wgpu::FrontFace::CCW;
                    desc.depthStencil = &orbDepth;
                    desc.fragment = &fragment;
                    desc.multisample.count = effective_msaa();   // B10: 1 = the default, byte-identical descriptor

                    if constexpr (ROSTER.orbs) {  // ROSTER-GATE orbs (a') — shader compile skipped when disabled
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
                    wgpu::PipelineLayout galleryLayout = strataLayoutFor("galleryLayout", frameRLayout_, galleryStateLayout_, galleryTexturesLayout_);

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
                    desc.multisample.count = effective_msaa();   // B10: 1 = the default, byte-identical descriptor

                    if constexpr (ROSTER.gallery) {  // ROSTER-GATE gallery (a') — shader compile skipped when disabled
                    if (!tPipe("gallery_frame", [&]() {
                        galleryFramePipeline_ = device_.CreateRenderPipeline(&desc);
                        return galleryFramePipeline_ != nullptr;
                    })) return false;
                    }

                    // Shadow Gallery Frame lives in the shadow block below, on a
                    // third build of this same layout pair (UMBRA_9).
                }

                // ─── Wall Painting Pipelines (framed paintings on indoor walls) ──
                // Uses same bind group layouts as gallery frames (galleryEntity + galleryTexture)
                {
                    wgpu::PipelineLayoutDescriptor pld{};
                    wgpu::PipelineLayout wpLayout = strataLayoutFor("wpLayout", frameRLayout_, galleryStateLayout_, galleryTexturesLayout_);

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
                        desc.multisample.count = effective_msaa();   // B10: 1 = the default, byte-identical descriptor

                        if constexpr (ROSTER.gallery) {  // ROSTER-GATE gallery (a') — shader compile skipped when disabled
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
                        desc.multisample.count = effective_msaa();   // B10: 1 = the default, byte-identical descriptor

                        if constexpr (ROSTER.gallery) {  // ROSTER-GATE gallery (a') — shader compile skipped when disabled
                        if (!tPipe("wall_painting_frame", [&]() {
                            wallPaintingFramePipeline_ = device_.CreateRenderPipeline(&desc);
                            return wallPaintingFramePipeline_ != nullptr;
                        })) return false;
                        }
                    }

                    // Shadow Wall Painting lives in the shadow block below — ONE
                    // pipeline where the color side needs two (UMBRA_9).
                }

                // ─── Shadow Pipelines (depth-only, kShadowDepthFormat) ───────────
                // Same bind group layouts as main render, but no fragment shader,
                // no color target, and shadow map depth format.
                {
                    wgpu::DepthStencilState shadowDepth{};
                    shadowDepth.format = kShadowDepthFormat;   // FORMAT_1 D1 — the one authority (state.hpp)
                    shadowDepth.depthWriteEnabled = true;
                    shadowDepth.depthCompare = wgpu::CompareFunction::Less;

                    // BIAS HAS ONE HOME, AND IT IS HERE (UMBRA_6). Every
                    // shadow pipeline below shares this state, so these lines are
                    // the whole of the program's shadow DEPTH bias. (Not the whole
                    // of its bias: PENUMBRA_1 P3 and P5 put a normal offset back
                    // in both samplers. Depth bias and normal offset are
                    // different instruments — see the closing paragraph.)
                    //
                    // Slope-scale does most of the work. It multiplies the
                    // ACTUAL window-space depth gradient of the primitive —
                    // applied after the perspective divide, so it is exact for
                    // the spot path's projection too, not an approximation of
                    // it — and it costs no fragment work at all.
                    //
                    // WHERE THE TWO CURVES AGREE AND WHERE THEY DO NOT. Against
                    // the mix(MAX, MIN, cos) this replaces, the slope term
                    // tracks well through the mid-range and diverges at both
                    // ends (incidence: new/old):
                    //     0deg 0.00 · 30deg 1.11 · 45deg 1.04 · 60deg 1.13
                    //          80deg 2.32 · 85deg 4.26 · 88deg 10.13 · ->inf
                    // At NORMAL incidence the new bias is exactly zero where
                    // the old had a 1.0e-4 floor. At GRAZING it RAN unbounded,
                    // where the old was capped at SHADOW_BIAS_MAX by
                    // construction. Both ends were named because "45 degrees
                    // matches" is true and is not the same claim as "this is
                    // equivalent".
                    //
                    // BOTH ENDS ARE NOW CLOSED. The grazing end by the
                    // depthBiasClamp set below (PENUMBRA_1 P2 — 0.0 had meant
                    // NO clamp, not clamp-at-zero); the normal-incidence end by
                    // the normal offset's FLOOR in sample_shadow_pcf
                    // (PENUMBRA_1 P3). The table above is history that
                    // motivated both, not a description of today.
                    //
                    // FORMAT_1 D3 — THE JUSTIFICATION BELOW HAS INVERTED, AND
                    // THE CONCLUSION SURVIVES ANYWAY. The arithmetic in this
                    // block is Depth32Float's: depthBias as a ULP multiple of
                    // the primitive's max depth, where the exponent is
                    // everything. Under Depth16Unorm it is no longer a ULP
                    // multiple of anything — a unorm format quantises the
                    // range uniformly, so one bias unit is a FIXED fraction:
                    // 1/65535 of the depth range, which on the 599.9-deep sun
                    // frustum is ~0.00915 world units. Against a post-UMBRA_5
                    // texel of 0.2051 wu that is a fine step, not the useless
                    // 3.6e-5 the float format made of it.
                    //
                    // So depthBias is now a dial, where P2 correctly said it
                    // was not. It is nonetheless LANDING AT ZERO, because the
                    // flip is meant to change bytes and not texels, and a bias
                    // invented without eyes on the surface is a number chosen
                    // to feel safe. If the gate shows acne, the follow-up is
                    // one line with the step above; slopeScale, depthBiasClamp
                    // and the normal offset remain the live instruments either
                    // way.
                    //
                    // What follows is PENUMBRA_1 P2's reasoning, kept verbatim
                    // as the record of why the constant went — and now also as
                    // the record of what the format change inverted.
                    //
                    // THE CONSTANT — HISTORY, kept because the arithmetic is the
                    // reason it is gone and a future editor will otherwise re-add
                    // it. depthBias was DELETED by PENUMBRA_1 P2. Depth32Float
                    // makes depthBias a ULP multiple of the primitive's max
                    // depth, so the exponent matters: the sun frustum is 599.9
                    // deep and the light sits SUN_ALTITUDE = 250 above the
                    // ground, so stored depths cluster near z = 0.417, not near
                    // 1. One ULP there is 2^-25 = 2.98e-8, so depthBias = 2
                    // buys 6.0e-8 NDC = 3.6e-5 world units. (Reading the ULP at
                    // z = 1 would overstate it 4x — the easy error here.)
                    //
                    // The scale of that: restoring the old floor (1.0e-4 NDC)
                    // would have meant depthBias ~= 3355, and the literal
                    // value-at-deletion of SHADOW_BIAS_MAX (2.0e-3, the
                    // resolution-only carry) ~67100. It was never a dial in
                    // steps of one, which is why P2 removed it rather than
                    // tuning it.
                    //
                    // DIRECTION, because it is easy to get backwards: bias
                    // pushes the STORED caster depth away from the light, so
                    // more bias = more lit = weaker shadow. ACNE wants it UP.
                    // DETACHMENT (peter-panning, shadow pulling off contact)
                    // wants it DOWN.
                    //
                    // THE CEILING (PENUMBRA_1 P2). The old shader term did TWO
                    // jobs — a floor AND a ceiling — and UMBRA_6 replaced only
                    // the floor. mix(MAX, MIN, cos) could never exceed
                    // SHADOW_BIAS_MAX by construction; slope-scale with no
                    // clamp WAS unbounded, running 4.3x the old term at 85 deg
                    // and 10.1x at 88 deg. Seven of the eleven shadow pipelines
                    // are CullMode::None, and an unbounded bias on a body whose
                    // faces reach grazing could push a caster clean out of the
                    // depth range so it stopped occluding at all.
                    //
                    // (This sentence used to call those seven "zero-thickness
                    // sheets". It is false and it was mine: PENUMBRA_3 C1 read
                    // the builders and found the RIBBON is a CLOSED capped tube
                    // — 4 faces x 6 verts plus 2 caps x 6 — and the PAWN a closed
                    // solid of revolution with a bottom cap fan. CullMode::None
                    // is a draw decision, not a thickness claim. The correct
                    // discriminator is at the BiasProfile enum below.)
                    //
                    // 2.8e-3 is the old SHADOW_BIAS_MAX carried across
                    // UMBRA_5 CORRECTLY: 8.192/2048 = 4.0e-3 NDC at the old
                    // 0.29297 wu texel is 0.013653 NDC per wu of texel, and
                    // 0.013653 x 0.20508 = 2.8e-3 at today's texel. Note this
                    // is NOT what the retired per-texel form would have given:
                    // 8.192/4096 = 2.0e-3, which is 1.40x short, because that
                    // form tracked RESOLUTION and UMBRA_5 also changed RADIUS.
                    // ECONOMY_1 E6's "carries its bias for free" held for a
                    // resolution ruling alone; UMBRA_5 was not one.
                    //
                    // WHERE SLOPE-SCALE CANNOT REACH, named because the
                    // campaign's own caster diet created it: the shadow pass
                    // draws terrain with the LOD1 index buffer while the main
                    // pass draws near terrain with a LOD0 one, so caster and
                    // receiver are DIFFERENT tessellations of the same
                    // heightfield — the LOD1 mesh is a chord over a 1.5625 wu
                    // span of a surface sampled at 0.78125. In a concave dip
                    // the chord rides above the true surface and the receiver
                    // reads as self-shadowed. Slope-scale cannot compensate
                    // it: it corrects a primitive's own depth gradient, not a
                    // difference between two meshes — and the error is largest
                    // exactly where the slope, and therefore the slope term,
                    // goes to zero. That gap is now covered by the normal
                    // offset's FLOOR (sample_shadow_pcf, PENUMBRA_1 P3), in
                    // texel units, which is the correct unit for it.
                    //
                    // depthBias is DELETED, not set to zero. Under
                    // Depth32Float it is a ULP multiple of the primitive's max
                    // depth: at this scene's z = 0.417 one ULP is 2^-25, so
                    // the old value of 2 bought 6.0e-8 NDC — about 3,355x
                    // short of the floor it was nominally replacing. It is not
                    // a dial in steps of one, and a line that reads like a dial
                    // and is not one costs more than it buys. THE LIVE
                    // INSTRUMENTS ARE slopeScale, depthBiasClamp, AND the normal
                    // offset — whose floor and ceiling are separate rungs.
                    shadowDepth.depthBiasSlopeScale = 2.0f;
                    // THE CEILING IS A WORLD QUANTITY WEARING NDC CLOTHES
                    // (PENUMBRA_2 N2). depthBiasClamp is in depth-buffer
                    // units, so its meaning scales with the sun's depth
                    // range — and N2 widened that range from 599.9 wu to
                    // 1100.0 wu so the caster set fits along the light axis
                    // in every mood. Carrying 2.8e-3 across unchanged would
                    // have silently widened the ceiling from 1.680 wu to
                    // 3.080 wu, 1.83x, with nothing in the diff to show it.
                    //
                    // THE WORLD NUMBER IS THE FACT. Re-derived, not carried:
                    //
                    //     1.680 wu / 1100.0 wu = 1.527e-3 NDC
                    //
                    // where 1.680 wu is the ceiling PENUMBRA_1 P2 arrived at
                    // (SHADOW_BIAS_MAX carried across UMBRA_5 by texel ratio)
                    // and 1100.0 is SUN_FAR - SUN_NEAR in world.wgsl.
                    //
                    // CROSS-ROOM: the divisor lives in WGSL and the quotient
                    // in C++, with nothing but this comment holding them
                    // together — the same shape as the SHADOW_MAP_SIZE twin,
                    // and filed with it in the report's HORIZON. If SUN_NEAR
                    // or SUN_FAR moves, this number is wrong and silent.
                    shadowDepth.depthBiasClamp      = 1.527e-3f;   // = 1.680 wu / 1100.0 wu

                    // THE SHARED BUILDER (shadow/depth category): the builder every shadow pipeline
                    // shares — shadowRenderLayout + shadowDepth (Depth32Float shadow map) +
                    // NO fragment (depth-only) + TriangleList + CCW. It is a SEPARATE builder
                    // from makeEntity (not one with an isShadow flag): color-vs-depth is a
                    // real category boundary (different layout, different depth state, no FS).
                    // Forks are parameters: shadow-VS (verbatim), VBL, cullMode — same
                    // Back/None split as the entity family (single-sided column/palm/cactus/
                    // blade + pawn/ribbon/shell → None; solids → Back).
                    // TWO BIAS PROFILES (PENUMBRA_3 C2). The profile rides the
                    // existing cullMode fork as a DEFAULTED 7th parameter, so the
                    // ten call sites that keep SOLID stay byte-identical. It must
                    // sit after `out` and not beside `cull`: default arguments are
                    // trailing, and `out` has none.
                    //
                    // WHY TWO. depthBiasClamp bounds how far slope-scale may push a
                    // caster. On terrain that bound is never reached — cell caps are
                    // gently sloped, so the slope term stays far below it. On a body
                    // whose faces reach GRAZING it is reached constantly, and it
                    // RELEASES as the face turns away, so the shadow edge steps in
                    // and out along the body. That is the serration the gate reports
                    // on cubes and the ribbon, and it is why terrain improved while
                    // they did not: they are the bodies that reach the ceiling.
                    //
                    // A tighter ceiling is the right direction for them and not a
                    // compromise. A body that reaches grazing gains nothing from a
                    // large bias — the receiver-side normal offset in
                    // sample_shadow_pcf already covers its self-shadowing — while it
                    // pays the full displacement.
                    //
                    // THE CLASSIFICATION IS NOT cullMode, AND NOT sheet-vs-solid.
                    // Both proxies were tried and both are wrong here (C1):
                    //   · cullMode == None misses the cube entirely — shadow_monolith
                    //     is Back, because a cube IS closed.
                    //   · "thin sheet" is false for the two loudest cases: the ribbon
                    //     is a CLOSED capped tube and the pawn a closed solid of
                    //     revolution, both drawn None.
                    // The real discriminator C1 found is that the serrating bodies are
                    // built from NON-PLANAR QUADS SPLIT INTO TWO TRIANGLES — the cube
                    // by independent corner jitter (J = 0.06, giving up to 5.04
                    // degrees between the two triangles of one face), the ribbon by
                    // twist. Slope-scale is computed PER TRIANGLE, so the two halves
                    // of one visual face disagree, and the ceiling bounds how far that
                    // disagreement can travel. Hence: terrain keeps the loose ceiling
                    // because it is the one body with a large gently-sloped area that
                    // never reaches it; everything else takes the tight one.
                    enum class BiasProfile { SOLID, GRAZING };

                    // THE LAYOUT FORK (UMBRA_9). Trailing and defaulted, so the ten
                    // call sites that take shadowRenderLayout stay byte-identical;
                    // the gallery's two pass their own. It rides BEHIND `profile`
                    // for the same reason `profile` rides behind `out`: default
                    // arguments are trailing.
                    //
                    // NULL IS THE SENTINEL FOR shadowRenderLayout, and it must be —
                    // the layout cannot be spelled as the default argument itself.
                    // shadowRenderLayout is a local of the enclosing function, and a
                    // lambda's default argument sits in the lambda's PARAMETER scope
                    // rather than its block, where an enclosing local is not
                    // odr-usable ([basic.def.odr]/10). Both GCC and Clang reject
                    // `= shadowRenderLayout` outright; MSVC accepts it only outside
                    // /permissive-. The resolution is one line, at desc.layout below.
                    auto makeShadow = [&](const char* label, const char* dbgLabel, const char* vsEntry,
                                          const wgpu::VertexBufferLayout* vbl, wgpu::CullMode cull,
                                          wgpu::RenderPipeline& out,
                                          BiasProfile profile = BiasProfile::GRAZING,
                                          wgpu::PipelineLayout layout = nullptr) -> bool {
                        // Body-local, so its address is valid for exactly as long as
                        // `desc`'s is — and CreateRenderPipeline is SYNCHRONOUS here
                        // (tPipe invokes its closure immediately;
                        // CreateRenderPipelineAsync is absent repo-wide), so nothing
                        // outlives this call. Starts from the shared state, so the
                        // format/write/compare triple has one home still.
                        wgpu::DepthStencilState ds = shadowDepth;
                        if (profile == BiasProfile::GRAZING) {
                            // 0.300 wu / 1100.0 wu. World units are the fact; the NDC
                            // number is derived against SUN_FAR - SUN_NEAR, exactly as
                            // the SOLID ceiling is. 0.300 wu is 37% of the 0.820 wu
                            // visible penumbra, so what shift survives stays inside
                            // the blur meant to hide it.
                            ds.depthBiasSlopeScale = 0.5f;
                            ds.depthBiasClamp      = 2.727e-4f;
                        }

                        wgpu::RenderPipelineDescriptor desc{};
                        desc.label = dbgLabel;
                        desc.layout = layout ? layout : shadowRenderLayout;
                        desc.vertex.module = shaderModule_;
                        desc.vertex.entryPoint = vsEntry;
                        desc.vertex.bufferCount = vbl ? 1u : 0u;
                        desc.vertex.buffers = vbl;
                        desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
                        desc.primitive.cullMode = cull;
                        desc.primitive.frontFace = wgpu::FrontFace::CCW;
                        desc.depthStencil = &ds;
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
                    // The one SOLID: terrain's cell caps are gently sloped and never
                    // reach the ceiling, and it is the body whose gate already passed.
                    // GoL slabs, pyramids and the 8 wu patch skirt ride this pipeline
                    // too and DO present vertical faces (C1) — flagged, not split,
                    // because no measurement has asked and one pipeline cannot carry
                    // two profiles.
                    if (!makeShadow("shadow_patch_terrain", "Shadow Patch Terrain", Entry::SHADOW_PATCH_TERRAIN_VS,
                        nullptr, wgpu::CullMode::Back, shadowPatchTerrainPipeline_,
                        BiasProfile::SOLID)) return false;
                    if (!makeShadow("shadow_pawn", "Shadow Pawn", Entry::SHADOW_PAWN_VS,
                        nullptr, wgpu::CullMode::None, shadowPawnPipeline_)) return false;

                    // Shadow sphere + monolith (MeshVertex, Back).
                    if constexpr (ROSTER.sphere) {  // ROSTER-GATE sphere (a') — shader compile skipped when disabled
                    if (!makeShadow("shadow_sphere", "Shadow Sphere", Entry::SHADOW_SPHERE_VS,
                        &shadowMeshVBL, wgpu::CullMode::Back, shadowSpherePipeline_)) return false;
                    }
                    if constexpr (ROSTER.cube) {  // ROSTER-GATE cube (a') — shader compile skipped when disabled
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
                        if constexpr (ROSTER.arch) {  // ROSTER-GATE arch (a') — shader compile skipped when disabled
                        if (!makeShadow("shadow_arch", "Shadow Catenary Arch", Entry::SHADOW_ARCH_VS,
                            &shadowArchVBL, wgpu::CullMode::Back, shadowArchPipeline_)) return false;
                        }
                        if constexpr (ROSTER.column || ROSTER.antenna) {  // ROSTER-GATE column+antenna (shared pipelines) (a') — shader compile skipped when disabled
                        if (!makeShadow("shadow_column", "Shadow Generative Column", Entry::SHADOW_COLUMN_VS,
                            &shadowArchVBL, wgpu::CullMode::None, shadowColumnPipeline_)) return false;
                        }
                        if constexpr (ROSTER.palm) {  // ROSTER-GATE palm (a') — shader compile skipped when disabled
                        if (!makeShadow("shadow_palm", "Shadow Palm Tree", Entry::SHADOW_PALM_VS,
                            &shadowArchVBL, wgpu::CullMode::None, shadowPalmPipeline_)) return false;
                        }
                        if constexpr (ROSTER.cactus) {  // ROSTER-GATE cactus (a') — shader compile skipped when disabled
                        if (!makeShadow("shadow_cactus", "Shadow Cactus", Entry::SHADOW_CACTUS_VS,
                            &shadowArchVBL, wgpu::CullMode::None, shadowCactusPipeline_)) return false;
                        }
                        if constexpr (ROSTER.blade) {  // ROSTER-GATE blade (a') — shader compile skipped when disabled
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
                        if constexpr (ROSTER.indoor_shell) {  // ROSTER-GATE indoor_shell (a') — shader compile skipped when disabled
                        if (!makeShadow("shadow_shell", "Shadow Indoor Shell", Entry::SHADOW_SHELL_VS,
                            &shadowShellVBL, wgpu::CullMode::None, shadowShellPipeline_)) return false;
                        }
                    }

                    // Shadow ribbon (bufferless, GPU-generated from vertex_index; None).
                    if constexpr (ROSTER.ribbon) {  // ROSTER-GATE ribbon (a') — shader compile skipped when disabled
                    if (!makeShadow("shadow_ribbon", "Shadow Sky Ribbon", Entry::SHADOW_RIBBON_VS,
                        nullptr, wgpu::CullMode::None, shadowRibbonPipeline_)) return false;
                    }

                    // Shadow gallery frame + wall painting (both bufferless, None —
                    // the color side's mode, so the caster silhouette is the drawn
                    // body's). A body that is DRAWN is a body that CASTS; there was
                    // never an artwork exception, only an artwork omission.
                    //
                    // THE GALLERY PIPELINE LAYOUT, THIRD INSTANCE. galleryLayout and
                    // wpLayout are locals of their own pipeline blocks above and do
                    // not reach here, so this rebuilds the SAME pair of bind group
                    // layouts — layout-compatible with the two color families, and
                    // the whole reason this campaign is small: the gallery entity
                    // group already binds the frame-R block at sizeof(GPUFrameR),
                    // so frame_r.vp.light_vp is already reachable, and the gallery
                    // texture group binds no shadow map, so it is already legal
                    // inside a depth-only pass. ZERO new bindings, ZERO new
                    // bind-group layouts. Do not grow either one.
                    if constexpr (ROSTER.gallery) {  // ROSTER-GATE gallery (a') — shader compile skipped when disabled
                    // ATLAS_1revB G2 — group 0 is the RENDER-ENTITY layout here,
                    // not the gallery entity layout. Under D2' these two
                    // shadow VSes call shadow_light_vp(), which reads
                    // frame_r.lighting (and, since B6, the shadow_slot
                    // IMMEDIATE — a pipeline-layout fact, not a group
                    // member); the gallery entity layout carries no
                    // frame_r block, so Dawn would reject both pipelines
                    // at creation. It is a strict subset for everything they DO
                    // use — config (Uniform) and frame_r.vp / frame_r.camera,
                    // which CHORD_3 folded into that one block — so nothing is
                    // lost by the swap, and
                    // draw_shadow_all sheds a bind per light because group 0 no
                    // longer changes mid-tile. Group 1 is untouched: the
                    // painting slots and array still come from the gallery
                    // texture layout. The COLOUR gallery pipelines keep the
                    // gallery entity layout; only the shadow pair moves.
                    wgpu::PipelineLayout galleryShadowLayout = strataLayoutFor("galleryShadowLayout", frameRLayout_, shadowStateLayout_, shadowTexturesLayout_);
                    if (!galleryShadowLayout) return false;

                    if (!makeShadow("shadow_gallery_frame", "Shadow Gallery Frame",
                        Entry::SHADOW_GALLERY_FRAME_VS, nullptr,
                        wgpu::CullMode::None, shadowGalleryFramePipeline_,
                        BiasProfile::GRAZING, galleryShadowLayout)) return false;
                    if (!makeShadow("shadow_wall_painting", "Shadow Wall Painting",
                        Entry::SHADOW_WALL_PAINTING_VS, nullptr,
                        wgpu::CullMode::None, shadowWallPaintingPipeline_,
                        BiasProfile::GRAZING, galleryShadowLayout)) return false;
                    }

                }

                // ─── Fade Overlay Pipeline ───────────────────────────────────────
                // Fullscreen triangle, alpha blending, no depth write.
                // Binds WORLD only (config) — the fade overlay's whole surface.
                {
                    wgpu::PipelineLayoutDescriptor pld{};
                    wgpu::PipelineLayout layout = strataLayoutFor("fadeOverlayLayout", emptyLayout_, emptyLayout_, emptyLayout_);

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
                    desc.multisample.count = effective_msaa();   // B10: 1 = the default, byte-identical descriptor

                    wgpu::DepthStencilState fadeDepth{};
                    fadeDepth.format = depthFormat_;
                    fadeDepth.depthWriteEnabled = false;
                    fadeDepth.depthCompare = wgpu::CompareFunction::Always;
                    desc.depthStencil = &fadeDepth;

                    if constexpr (ROSTER.transitions) {  // ROSTER-GATE transitions (a') — shader compile skipped when disabled
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
