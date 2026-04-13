// ─── render_passes.inl ──────────────────────────────────────────
//
// GPU dispatch and draw calls. The speaker at the end of the
// signal chain: reads final state, issues compute and render passes.
//
// Contents:
//   upload_ground_entries        — pack entity positions for GPU
//   dispatch_placement_correction — Y-correct entities on heightfield
//   dispatch_compute             — ribbon, pawn, camera, sphere, VP
//   render_shadow_pass           — depth-only pass (outdoor + indoor atlas)
//   draw_shadow_all              — shared shadow draw list
//   render_main_pass             — full color pass (terrain + all entities)
//   compute_spot_light_vp        — perspective projection for spot shadows
//   compute_sun_matrices         — orthographic projection for sun shadows
//
// Included inside the Cartridge class body.
// Depends on: entities.inl, gallery.inl, gol_zones.inl, pawn_aura.inl
// ─────────────────────────────────────────────────────────────────


// ─── Pre-Render Data Preparation ─────────────────────────────────

            // --- Per-frame ground entry upload: cached pier-top Y ---
            // ground_y is computed once at spawn time and cached in Active* structs.
            // This function is now a pure data-upload — no terrain evaluation.
            void upload_ground_entries(wgpu::Queue& queue) {
                // --- Arch ground entries ---
                GPUArchGroundEntry archOrigins[Dim::MAX_ARCH_INSTANCES]{};
                for (uint32_t i = 0; i < Dim::MAX_ARCH_INSTANCES; i++) {
                    if (!activeArches_[i].active) continue;
                    const auto& pl = cpuPiers_[Dim::PIER_ARCH_BASE + i * 2];
                    const auto& pr = cpuPiers_[Dim::PIER_ARCH_BASE + i * 2 + 1];
                    archOrigins[i].pier_left_x = pl.origin[0];
                    archOrigins[i].pier_left_z = pl.origin[1];
                    archOrigins[i].pier_right_x = pr.origin[0];
                    archOrigins[i].pier_right_z = pr.origin[1];
                    archOrigins[i].is_active = 1;
                    archOrigins[i].ground_y = activeArches_[i].cached_ground_y;
                    archOrigins[i].pier_correction_left = 0.0f;
                    archOrigins[i].pier_correction_right = 0.0f;
                }
                gpuState_.upload_arch_origins(queue, archOrigins, Dim::MAX_ARCH_INSTANCES);

                // --- Column + antenna ground entries (shared GPU buffer, split arrays) ---
                GPUColumnGroundEntry columnOrigins[Dim::MAX_COLUMN_INSTANCES]{};
                for (uint32_t i = 0; i < Dim::MAX_COLUMN_ONLY; i++) {
                    if (!activeColumns_[i].active) continue;
                    columnOrigins[i].center_x = activeColumns_[i].world_x;
                    columnOrigins[i].center_z = activeColumns_[i].world_z;
                    columnOrigins[i].is_active = 1;
                    columnOrigins[i].ground_y = activeColumns_[i].cached_ground_y;
                    columnOrigins[i].pier_correction = 0.0f;
                }
                for (uint32_t i = 0; i < Dim::MAX_ANTENNA_ONLY; i++) {
                    if (!activeAntennas_[i].active) continue;
                    uint32_t gpu_slot = i + Dim::ANTENNA_SLOT_OFFSET;
                    columnOrigins[gpu_slot].center_x = activeAntennas_[i].world_x;
                    columnOrigins[gpu_slot].center_z = activeAntennas_[i].world_z;
                    columnOrigins[gpu_slot].is_active = 1;
                    columnOrigins[gpu_slot].ground_y = activeAntennas_[i].cached_ground_y;
                    columnOrigins[gpu_slot].pier_correction = 0.0f;
                }
                gpuState_.upload_column_origins(queue, columnOrigins, Dim::MAX_COLUMN_INSTANCES);

                // --- Pyramid ground entries ---
                GPUPyramidGroundEntry pyramidOrigins[Dim::MAX_PYRAMID_INSTANCES]{};
                for (uint32_t i = 0; i < Dim::MAX_PYRAMID_INSTANCES; i++) {
                    if (!activePyramids_[i].active) continue;
                    const auto& inst = cpuPyramids_.instances[i];
                    pyramidOrigins[i].center_x = inst.origin[0];
                    pyramidOrigins[i].center_z = inst.origin[1];
                    pyramidOrigins[i].is_active = 1;
                    pyramidOrigins[i].own_height = inst.height;
                    pyramidOrigins[i].half_x = inst.half_size[0];
                    pyramidOrigins[i].half_z = inst.half_size[1];
                    pyramidOrigins[i].rotation = inst.rotation;
                    pyramidOrigins[i].ground_y = activePyramids_[i].cached_ground_y;
                }
                gpuState_.upload_pyramid_origins(queue, pyramidOrigins, Dim::MAX_PYRAMID_INSTANCES);

                // --- Palm ground entries ---
                GPUPalmGroundEntry palmOrigins[Dim::MAX_PALM_INSTANCES]{};
                for (uint32_t i = 0; i < Dim::MAX_PALM_INSTANCES; i++) {
                    if (!activePalms_[i].active) continue;
                    palmOrigins[i].center_x = activePalms_[i].world_x;
                    palmOrigins[i].center_z = activePalms_[i].world_z;
                    palmOrigins[i].is_active = 1;
                    palmOrigins[i].ground_y = activePalms_[i].cached_ground_y;
                }
                gpuState_.upload_palm_origins(queue, palmOrigins, Dim::MAX_PALM_INSTANCES);

                // --- Cactus ground entries ---
                GPUCactusGroundEntry cactusOrigins[Dim::MAX_CACTUS_INSTANCES]{};
                for (uint32_t i = 0; i < Dim::MAX_CACTUS_INSTANCES; i++) {
                    if (!activeCacti_[i].active) continue;
                    cactusOrigins[i].center_x = activeCacti_[i].world_x;
                    cactusOrigins[i].center_z = activeCacti_[i].world_z;
                    cactusOrigins[i].is_active = 1;
                    cactusOrigins[i].ground_y = activeCacti_[i].cached_ground_y;
                }
                gpuState_.upload_cactus_origins(queue, cactusOrigins, Dim::MAX_CACTUS_INSTANCES);

                // --- Blade cluster ground entries ---
                GPUBladeClusterGroundEntry bladeOrigins[Dim::MAX_BLADE_INSTANCES]{};
                for (uint32_t i = 0; i < Dim::MAX_BLADE_INSTANCES; i++) {
                    if (!activeBlades_[i].active) continue;
                    bladeOrigins[i].center_x = activeBlades_[i].world_x;
                    bladeOrigins[i].center_z = activeBlades_[i].world_z;
                    bladeOrigins[i].is_active = 1;
                    bladeOrigins[i].ground_y = activeBlades_[i].cached_ground_y;
                }
                gpuState_.upload_blade_origins(queue, bladeOrigins, Dim::MAX_BLADE_INSTANCES);
            }

            // --- Entity placement Y-correction: heightfield sample - pier correction ---
            // Runs unconditionally every frame, AFTER upload_ground_entries and BEFORE
            // render passes (shadow + main read the corrected ground_y).
            void dispatch_placement_correction(wgpu::CommandEncoder& encoder) {
                wgpu::ComputePassDescriptor cpd{};
                cpd.label = "Entity Placement Y Correction";
                wgpu::ComputePassEncoder compute = encoder.BeginComputePass(&cpd);
                renderer_.dispatch_entity_placement(
                    compute, gpuState_.entity_placement_compute_group()
                );
                compute.End();
            }

// ─── GPU Compute Dispatch ─────────────────────────────────────────

            // --- GPU compute: ribbon transforms, world update, VP matrix ---
            void dispatch_compute(wgpu::CommandEncoder& encoder) {
                wgpu::ComputePassDescriptor desc{};
                desc.label = "Compute Phase";
                wgpu::ComputePassEncoder compute = encoder.BeginComputePass(&desc);

                if (ribbonActive_) {
                    renderer_.dispatch_compute_ribbon_rings(
                        compute,
                        gpuState_.ribbon_compute_group(),
                        GPUState::ribbon_ring_workgroups()
                    );
                }

                renderer_.dispatch_update_terrain_config(
                    compute,
                    gpuState_.compute_entity_group()
                );

                renderer_.dispatch_update_pawn(
                    compute,
                    gpuState_.compute_entity_group()
                );

                renderer_.dispatch_update_camera(
                    compute,
                    gpuState_.compute_entity_group()
                );

                renderer_.dispatch_update_sphere(
                    compute,
                    gpuState_.compute_entity_group()
                );

                renderer_.dispatch_compute_vp(
                    compute,
                    gpuState_.compute_entity_group()
                );

                compute.End();
            }

            // --- Shadow depth pass ---
            //
            // Outdoor: single pass, directional sun VP, full 4096×4096 map.
            // Indoor:  two-texture atlas — lights 0-1 on the sun map (repurposed),
            //          lights 2-3 on the spot map. Each texture is split left/right
            //          into 2048×4096 tiles. Doubles per-tile resolution vs the old
            //          single-texture 2×2 grid, with zero extra memory.

            void render_shadow_pass(wgpu::CommandEncoder& encoder) {
                if (spotLightActive_ && cpuSpotLights_.count > 0) {
                    // ─── Two-texture atlas shadow pass (indoor) ──────────
                    static constexpr uint32_t TILE_W = Dim::SHADOW_MAP_SIZE / 2;  // 2048
                    static constexpr uint32_t TILE_H = Dim::SHADOW_MAP_SIZE;      // 4096

                    for (uint32_t li = 0; li < cpuSpotLights_.count && li < MAX_SPOT_LIGHTS; li++) {
                        // Copy this light's VP from staging → VP buffer's light_vp slot
                        encoder.CopyBufferToBuffer(
                            gpuState_.spot_vp_staging(), li * 64,
                            gpuState_.vp_buffer(), GPUState::light_vp_offset(),
                            GPUState::light_vp_size());

                        // Lights 0-1 → sun map (idle in indoor mode), lights 2-3 → spot map
                        bool use_sun_map = (li < 2);
                        uint32_t within = li % 2;   // 0=left half, 1=right half

                        wgpu::RenderPassDepthStencilAttachment depthAttachment{};
                        depthAttachment.view = use_sun_map
                            ? gpuState_.shadow_map_view()
                            : gpuState_.spot_shadow_map_view();
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

                        draw_shadow_all(pass);
                        pass.End();
                    }
                }
                else {
                    // ─── Standard shadow pass (outdoor) ──────────────────
                    wgpu::RenderPassDepthStencilAttachment depthAttachment{};
                    depthAttachment.view = gpuState_.shadow_map_view();
                    depthAttachment.depthLoadOp = wgpu::LoadOp::Clear;
                    depthAttachment.depthStoreOp = wgpu::StoreOp::Store;
                    depthAttachment.depthClearValue = 1.0f;

                    wgpu::RenderPassDescriptor desc{};
                    desc.label = "Shadow Pass";
                    desc.colorAttachmentCount = 0;
                    desc.depthStencilAttachment = &depthAttachment;

                    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&desc);

                    draw_shadow_all(pass);
                    pass.End();
                }
            }

            // All shadow draws: terrain + entities
            void draw_shadow_all(wgpu::RenderPassEncoder& pass) {
                // Terrain (LOD-0 + LOD-1)
                renderer_.draw_shadow_patch_terrain(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.shadow_texture_group(),
                    gpuState_.patch_index_buffer(),
                    gpuState_.patch_index_count(),
                    lod0PatchCount_
                );
                if (renderPatchCount_ > lod0PatchCount_) {
                    pass.SetIndexBuffer(gpuState_.patch_index_buffer_lod1(), wgpu::IndexFormat::Uint32);
                    pass.DrawIndexed(gpuState_.patch_index_count_lod1(),
                        renderPatchCount_ - lod0PatchCount_, 0, 0, lod0PatchCount_);
                }

                // Entities
                if (golZoneCount_ > 0) {
                    renderer_.draw_shadow_zone_extrusion(
                        pass,
                        gpuState_.render_entity_group(),
                        gpuState_.shadow_texture_group(),
                        gpuState_.zone_mesh_vertex_buffer(),
                        gpuState_.zone_mesh_index_buffer(),
                        gpuState_.zone_mesh_indirect_buffer()
                    );
                }

                renderer_.draw_shadow_pawn(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.shadow_texture_group(),
                    GPUState::pawn_vertex_count()
                );

                renderer_.draw_shadow_sphere(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.shadow_texture_group(),
                    gpuState_.sphere_vertex_buffer(),
                    gpuState_.sphere_index_buffer(),
                    gpuState_.sphere_index_count()
                );

                renderer_.draw_shadow_monolith(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.shadow_texture_group(),
                    gpuState_.monolith_vertex_buffer(),
                    gpuState_.monolith_index_buffer(),
                    gpuState_.monolith_index_count()
                );

                if (ribbonActive_) {
                    renderer_.draw_shadow_ribbon(
                        pass,
                        gpuState_.render_entity_group(),
                        gpuState_.shadow_texture_group(),
                        GPUState::ribbon_vertex_count()
                    );
                }

                renderer_.draw_shadow_arch(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.shadow_texture_group(),
                    gpuState_.arch_vertex_buffer(),
                    gpuState_.arch_index_buffer(),
                    gpuState_.arch_index_count()
                );

                renderer_.draw_shadow_column(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.shadow_texture_group(),
                    gpuState_.column_vertex_buffer(),
                    gpuState_.column_index_buffer(),
                    gpuState_.column_index_count()
                );

                renderer_.draw_shadow_palm(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.shadow_texture_group(),
                    gpuState_.palm_vertex_buffer(),
                    gpuState_.palm_index_buffer(),
                    gpuState_.palm_index_count()
                );

                renderer_.draw_shadow_cactus(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.shadow_texture_group(),
                    gpuState_.cactus_vertex_buffer(),
                    gpuState_.cactus_index_buffer(),
                    gpuState_.cactus_index_count()
                );

                renderer_.draw_shadow_blade(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.shadow_texture_group(),
                    gpuState_.blade_vertex_buffer(),
                    gpuState_.blade_index_buffer(),
                    gpuState_.blade_index_count()
                );

                renderer_.draw_shadow_shell(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.shadow_texture_group(),
                    gpuState_.shell_vertex_buffer(),
                    gpuState_.shell_index_buffer(),
                    gpuState_.shell_index_count()
                );
            }

            // --- Main color pass: terrain, pawn, sphere, ribbon ---
            void render_main_pass(wgpu::CommandEncoder& encoder,
                wgpu::TextureView backbuffer, wgpu::TextureView depth) {

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

                // LOD-0: full 64×64 mesh for near patches [0..lod0PatchCount_)
                renderer_.draw_patch_terrain(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.render_texture_group(),
                    gpuState_.patch_index_buffer(),
                    gpuState_.patch_index_count(),
                    lod0PatchCount_
                );

                // LOD-1: half 32×32 mesh for far patches [lod0PatchCount_..renderPatchCount_)
                // Pipeline and bind groups already set by LOD-0 draw above.
                if (renderPatchCount_ > lod0PatchCount_) {
                    pass.SetIndexBuffer(gpuState_.patch_index_buffer_lod1(), wgpu::IndexFormat::Uint32);
                    pass.DrawIndexed(gpuState_.patch_index_count_lod1(),
                        renderPatchCount_ - lod0PatchCount_, 0, 0, lod0PatchCount_);
                }

                if (golZoneCount_ > 0) {
                    renderer_.draw_zone_extrusion(
                        pass,
                        gpuState_.render_entity_group(),
                        gpuState_.render_texture_group(),
                        gpuState_.zone_mesh_vertex_buffer(),
                        gpuState_.zone_mesh_index_buffer(),
                        gpuState_.zone_mesh_indirect_buffer()
                    );
                }

                renderer_.draw_pawn(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.render_texture_group(),
                    GPUState::pawn_vertex_count()
                );

                renderer_.draw_sphere(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.render_texture_group(),
                    gpuState_.sphere_vertex_buffer(),
                    gpuState_.sphere_index_buffer(),
                    gpuState_.sphere_index_count()
                );

                renderer_.draw_monolith(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.render_texture_group(),
                    gpuState_.monolith_vertex_buffer(),
                    gpuState_.monolith_index_buffer(),
                    gpuState_.monolith_index_count()
                );

                renderer_.draw_arch(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.render_texture_group(),
                    gpuState_.arch_vertex_buffer(),
                    gpuState_.arch_index_buffer(),
                    gpuState_.arch_index_count()
                );

                renderer_.draw_column(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.render_texture_group(),
                    gpuState_.column_vertex_buffer(),
                    gpuState_.column_index_buffer(),
                    gpuState_.column_index_count()
                );

                renderer_.draw_palm(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.render_texture_group(),
                    gpuState_.palm_vertex_buffer(),
                    gpuState_.palm_index_buffer(),
                    gpuState_.palm_index_count()
                );

                renderer_.draw_cactus(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.render_texture_group(),
                    gpuState_.cactus_vertex_buffer(),
                    gpuState_.cactus_index_buffer(),
                    gpuState_.cactus_index_count()
                );

                renderer_.draw_blade(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.render_texture_group(),
                    gpuState_.blade_vertex_buffer(),
                    gpuState_.blade_index_buffer(),
                    gpuState_.blade_index_count()
                );

                renderer_.draw_shell(
                    pass,
                    gpuState_.render_entity_group(),
                    gpuState_.render_texture_group(),
                    gpuState_.shell_vertex_buffer(),
                    gpuState_.shell_index_buffer(),
                    gpuState_.shell_index_count()
                );

                // Wall-mounted framed paintings (indoor)
                renderer_.draw_wall_paintings(
                    pass,
                    gpuState_.gallery_entity_group(),
                    gpuState_.gallery_texture_group(),
                    wallFrameCount_
                );

                // Pyramids: terrain surface IS the pyramid shape (via ground_formed).
                // No separate mesh draw needed.

                if (ribbonActive_) {
                    renderer_.draw_ribbon(
                        pass,
                        gpuState_.render_entity_group(),
                        gpuState_.render_texture_group(),
                        GPUState::ribbon_vertex_count()
                    );
                }

                // Gallery frames (self-portrait paintings on terrain)
                renderer_.draw_gallery_frames(
                    pass,
                    gpuState_.gallery_entity_group(),
                    gpuState_.gallery_texture_group(),
                    activePaintingCount_
                );

                // Fade overlay (drawn last, alpha blended over everything)
                renderer_.draw_fade_overlay(
                    pass,
                    gpuState_.mesh_gen_entity_group(),
                    transitionFadeAlpha_
                );

                pass.End();
            }

// ─── Light Matrix Computation ─────────────────────────────────────

            // ─── Light Matrix Computation ─────────────────────────────────────

            // Perspective projection for spot light shadow map.
            // Physically correct: shadows fan out from the source.
            // Bias is handled in the shader with distance-scaled + normal offset.
            static void compute_spot_light_vp(const GPUSpotLight& light, float* view_proj_out) {
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

                // Perspective projection — FOV matched to outer cone angle.
                // Concentrates shadow texels on the actual lit area instead of
                // wasting resolution on the dark periphery. Small margin ensures
                // the shadow frustum slightly exceeds the lit cone.
                // Capped at 2.8 rad (~160°) to avoid degenerate perspective.
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

            //
            // Orthographic projection for directional light shadow map.
            // Covers a fixed area centered at world origin. The light "looks"
            // along its direction from a high altitude.

            void compute_sun_matrices(const float* direction, float* view_proj_out,
                float center_x = 0.0f, float center_z = 0.0f) {
                // Sun orbits the pawn at a fixed distance.
                // The frustum is sized to cover the 11×11 patch grid (radius 5).
                //
                // Terrain extends ±250 from grid center. The pawn can be up to
                // ~25 units from grid center (half-cell before shift).
                // So worst case: terrain is ±275 from pawn in world space.
                // At the oblique light angle, that projects to ~305 in light space.
                // ±350 covers everything with margin. Yields 700/4096 = 0.17 units/texel.
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

                // Orthographic projection — sized to cover all loaded terrain.
                // With radius 5: ±250 from grid center, ±275 from pawn.
                // At the oblique light angle, worst-case is ~305 in light space.
                // ±350 covers everything with margin.
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
