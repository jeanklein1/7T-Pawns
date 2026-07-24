#pragma once

// ─── cartridge.hpp ───────────────────────────────────────────────
// History: audit/LADDER.md
// glaw1 — the compile gate: the C++ compiler as witness-runner. Every
// static_assert in the tree is a glaw1 check; "glaw1 catches X" means
// the build fails loud. WGSL sits outside its reach — the two-rooms
// mirror rule and the boot rig are the nets there.
//
// THE_BOARD — Generative world engine.
//
// ONE REGIME (constitution §1): every module is a file-scope pair around the class;
// the cartridge is the composition root alone.
//
// SEAM[spine:owns] FAMILY_DISPATCH is genuinely spine work — the
//   integration hub that ties the 12 families together. Each row's
//   body lives in the family's owning module. Per Ch. 15 of the seam
//   map. Adding a new family means: write select/place/commit/
//   evict/prepare_mesh in the owning module, add wrappers below,
//   add 1 row to FAMILY_DISPATCH.
// SEAM[spine:K2-related] the dispatch_prepare_mesh_*,
//   dispatch_mesh_gen_* wrappers are integration glue, not module
//   work. They live here correctly.
//   NOTE[seam-map] keep wrappers here; they're the integration layer
//   between FAMILY_DISPATCH and per-family modules.
// SEAM[spine:P5] readback state machines + world_state_.world_gen counter are
//   pattern P5 (release-pending sentinel) at the spine level. Pawn +
//   floater readbacks each protect against stale callbacks from
//   previous worlds via world_state_.world_gen capture in the closure. Genuinely
//   spine-owned, not a leak.
// SEAM[spine:P8] PlayerState's commented "Future (deferred)" fields
//   are explicit latent infrastructure: aura_presence is live here;
//   the other deferred fields await the unified entity layer.
//   Pattern P8 visible in source.
// SEAM[spine:portal-system] portal/transition state machine. Owns
//   transitionPhase_ (enum type in contracts/spine_state.hpp),
//   mood_state_.transition_timer, pendingDestination_, the
//   PORTAL_COLORS table, the back-portal pending state, and the
//   trigger-detection hooks called by readback. direction/mood.hpp drives portal
//   spawning (force_spawn_portal_at, force_spawn_back_portal,
//   force_spawn_finite_portals); spine owns the request → activation
//   flow.
// SEAM[spine:family-dispatch] all evict_<family> (owner-side),
//   dispatch_prepare_mesh_<family>, dispatch_mesh_gen_<family>
//   wrapper functions land here — referenced by FAMILY_DISPATCH and
//   by machine/spawn_engine.hpp's commit/evict pipelines.
// ─────────────────────────────────────────────────────────────────

#include "render/render_cartridge.hpp"
#include "core/input_event.hpp"
#include "cartridges/the_board/contracts/roster.hpp"
#include "cartridges/the_board/demos/demo.hpp"             // THE SELECTED SENTENCE: DEMO + ROSTER (compile-time, INCUBATE_DEMO; default full)
#include "cartridges/the_board/primitives/seed_utils.hpp"           // hash/gaussian/tier-select helpers (pure-math leaf)
#include "cartridges/the_board/contracts/ground_architecture.hpp"  // ground contributor/policy tables + compile-time DAG checks
#include "cartridges/the_board/contracts/entity_types.hpp"         // THE CONTRACT HOME: pipeline contracts + boundary DTOs + queue unions + dispatch row/table decl
#include "cartridges/the_board/contracts/indoor_module.hpp"        // THE INDOOR MODULE: mood's insert on the spawn chain — one policy table + three dials; consumers ride the cohort (grounded/floaters/ribbon/the machine)
#include "cartridges/the_board/contracts/spawn_services.hpp"      // THE MACHINE'S DECL TIER: spawn/pipeline service decls + boundary DTOs + arch vocabulary + MIN_SEPARATION (bodies ride the merged machine headers at the cohort tail)
#include "cartridges/the_board/contracts/mood_constants.hpp"       // MOOD_COUNT + the Mood IDs + PortalDestination
#include "cartridges/the_board/contracts/spine_state.hpp"          // TimeState + PlayerState + TransitionPhase + InputState + MoodState/MoodProfile/MOOD_TABLE + the request door decl (spine organ TYPES; instances stay at the root)
#include "cartridges/the_board/contracts/point.hpp"                // THE POINT: the parent of the player system — host enum + terrain rule + the bubble decl; instance at the root
#include "cartridges/the_board/contracts/floaters.hpp"   // floater TYPES (ActiveSphere/ActiveCube), file scope
#include "cartridges/the_board/realization/state.hpp"
#include "cartridges/the_board/surface/population_themes.hpp"  // S2: THEMES + ThemeEnvelope + ThemesState — MERGED single file
#include "cartridges/the_board/contracts/surface_services.hpp"  // THE SURFACE'S DECL TIER: WorldState + the patch registry + budgets/visibility + PatchSystemState + the surface service decls (bodies ride surface/patch_system.hpp at the cohort tail)
#include "cartridges/the_board/surface/tile_world.hpp"          // S2: archetypes + tokens + TileState/cache + TileWorldDeps + impl — MERGED single file; after patch_system for WorldState/Dim::PATCH_EXTENT
#include "cartridges/the_board/bodies/grounded.hpp"             // grounded-family vocabulary + EntitiesState + impl — MERGED; after entity_pipeline for generic_*
#include "cartridges/the_board/bodies/agents.hpp"               // AgentState + AgentsDeps + impl — MERGED; after entities for COLUMN_PALETTE
#include "cartridges/the_board/bodies/cube_behaviors.hpp"       // CubeBehaviorsState + CubeDeps + impl — MERGED; after agents for AgentState
#include "cartridges/the_board/bodies/spheres.hpp"              // SphereState + SphereDeps + impl — MERGED single file; after entity_pipeline for the generic funnels
#include "cartridges/the_board/realization/renderer.hpp"
#include "cartridges/the_board/realization/drawable_table.hpp"  // The drawable table (one row per drawable; the 3 passes iterate it filtered) — after renderer/state, BEFORE gallery + render_passes so both the snapshot pass and shadow/main see it
#include "cartridges/the_board/bodies/pawn.hpp"                 // PawnState + PawnDeps + impl — MERGED single file; after renderer for Renderer/GPUState complete
#include "cartridges/the_board/bodies/orbs.hpp"                 // OrbsState + OrbsDeps + impl — MERGED; after renderer for Renderer
#include "cartridges/the_board/bodies/gol_zones.hpp"            // GoLState + GolDeps (S5 device) + impl — MERGED; after renderer/machine/tile
#include "coupling/visual_canvas.hpp"
#include "cartridges/the_board/bodies/ribbon.hpp"               // RibbonState + RibbonDeps + impl — MERGED; after visual_canvas for the coupling face
#include "cartridges/the_board/bodies/gallery.hpp"              // GalleryState + GalleryDeps + impl — MERGED; after ribbon for RibbonState
#include "cartridges/the_board/direction/input.hpp"             // KeyState/MouseState + InputDeps + impl — MERGED; after ribbon for RibbonState (the sky fixture); InputState graduated to spine_state
#include "cartridges/the_board/realization/render_passes.hpp"   // the pass/dispatch bodies on THE MACHINE FACE + light-VP helpers — MERGED; before mood (compute_spot_light_vp)
#include "cartridges/the_board/direction/mood.hpp"              // MoodDeps + portal/palette vocabulary + impl — MERGED; after ribbon/gallery/input (fan targets), before the machine natives (they call its derivers); MoodState/MoodProfile/MOOD_TABLE graduated to spine_state
#include "cartridges/the_board/machine/spawn_engine.hpp"        // S3: proximity tables + footprints + SpawnEngineState + the preamble template + impl — MERGED; after entities/renderer for complete organs; decl tier in contracts/spawn_services.hpp
#include "cartridges/the_board/machine/entity_pipeline.hpp"     // S3: the three-phase verbs + the welded four — MERGED; after spawn_engine (services) + entities (vocab)
#include "cartridges/the_board/surface/patch_system.hpp"        // S2: the active-patch machine's bodies on THE MACHINE FACE — MERGED; decl tier in contracts/surface_services.hpp
#include <cmath>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <filesystem>
#include <algorithm>
#include <string>
#include <vector>

namespace t7 {
    namespace the_board {

        class Cartridge : public RenderCartridge {

            // COMPOSITION ROOT — ORGANS ARE PUBLIC: sight is free; writes pass
            // through declared seams; the census enforces the seam law, not
            // access control.
        public:

            wgpu::Device device_;
            wgpu::TextureFormat colorFormat_;
            wgpu::TextureFormat depthFormat_;

            GPUState gpuState_;
            Renderer renderer_;

            // ═══ COMPOSITION ROOT — MODULE STATE ════════════════════════
            //
            SphereState sphere_state_;

            //   cube_behaviors_state_ — CubeBehaviorsState:
            //     the cube diagnostics + the cube active-slot mirror.
            CubeBehaviorsState cube_behaviors_state_;

            //   pawn_state_ — PawnState: the pawn aura + presence state.
            PawnState pawn_state_;

            EntitiesState entities_state_;

            //   orbs_state_ — OrbsState: the sky-dome lifecycle +
            //     player-owned anchor/rule/gesture state.
            OrbsState orbs_state_;

            //   gol_state_ — GoLState: the zone slots + counts +
            //     mood gate + derive-request queue.
            GoLState gol_state_;

            //   agent_state_ — AgentState: the 32-slot CPU mirror +
            //     respawn counters + diagnostic overrides.
            AgentState agent_state_;

            GalleryState gallery_state_;

            RibbonState ribbon_state_;

            //   themes_state_ — ThemesState: the theme
            //     envelope machine + the per-patch selection.
            ThemesState themes_state_;

            //   tile_world_state_ — TileWorldState: the tile
            //     cache + the terrain tokens (what the terrain remembers).
            TileWorldState tile_world_state_;

            //   patch_system_state_ — PatchSystemState: the
            //     active-patch registry + the free-layer pool.
            PatchSystemState patch_system_state_;

            //   world_state_ — WorldState: the world seed +
            //     radii + patch counts + dirty flags. ROOT ORGAN: the struct
            //     lives in contracts/surface_services.hpp; the
            //     instance stays here at the root.
            WorldState world_state_;

            //   spawn_engine_state_ — SpawnEngineState: the
            //     two dispatch queues + the footprint registry + the census clock.
            SpawnEngineState spawn_engine_state_;

            InputState inputState_;
            KeyState keys_;
            MouseState mouse_;

            // ═══ TIME STATE ═════════════════════════════════════════════
            // Struct TimeState lives in contracts/spine_state.hpp;
            // the instance stays here.
            TimeState time_state_;

            VisualCanvas  visual_canvas_;
            TargetBinding fog_density_dst_{};   // resolved "fog.density" pipe
            // Ribbon amp pipes (pitch compass) — resolved once at bind.
            TargetBinding ribbon_amp_lat_dst_{};
            TargetBinding ribbon_amp_vert_dst_{};
            TargetBinding ribbon_tint_stim_dst_{};
            TargetBinding ribbon_tint_mix_dst_{};
            TargetBinding fog_color_dst_{};      // resolved "fog.color" pipe (3 wide)
            // Checker pipes (CHECKER-1) — resolved once at bind.
            TargetBinding checker_mean_dst_{};
            TargetBinding checker_var_dst_{};

            // Sun + atmosphere (driven by active mood — see apply_mood)
            float sunDirection_[3] = { 0.69f, -0.71f, -0.14f };
            float sunColor_[3] = { 1.0f, 0.95f, 0.9f };
            float clearColor_[3] = { 0.85f, 0.78f, 0.72f };

            // ═══ MOOD STATE ═════════════════════════════════════════════
            //
            // Struct MoodState lives in contracts/spine_state.hpp.
            // The INSTANCE stays spine-resident because
            // mood-applied values feed every other subsystem
            // (SEAM[spine:transitions], K4).
            MoodState mood_state_;

            // ═══ PLAYER STATE ════════════════════════════════════════════
            //
            // Struct PlayerState lives in contracts/spine_state.hpp
            // — SEAM[spine:P8] rides with the
            // struct; the instance stays here.
            PlayerState player_{};

            // ═══ THE POINT ═══════════════════════════════════════════════
            //
            // The parent of the player system (contracts/point.hpp): the
            // camera is its permanent witness; the pawn is its default
            // host (the kite); free-fly re-hosts it on the camera. The
            // GPU mirror is config.point_host; the toggle is input's
            // point-host command (key 4).
            PointState point_{};

            // ═══ THE MACHINE FACE ═══════════════════════════════════════
            // The one declared context the dispatch contract hands the
            // rows — references bound once, in the constructor, to the
            // organs above (contracts/entity_types.hpp owns the type).
            MachineCtx machine_ctx_;

            // Per-module deps faces — bound once
            // in the ctor, each the module's requirements made literal.
            TileWorldDeps tile_world_deps_;
            SphereDeps    sphere_deps_;
            PawnDeps      pawn_deps_;
            OrbsDeps      orbs_deps_;
            AgentsDeps    agents_deps_;
            CubeDeps      cube_deps_;
            GolDeps       gol_deps_;
            RibbonDeps    ribbon_deps_;
            GalleryDeps   gallery_deps_;
            InputDeps     input_deps_;
            MoodDeps      mood_deps_;

            GPUSpotLightArray cpuSpotLights_{};  // count=0 disables (outdoor)

            // ═══ PORTAL & TRANSITION STATE MACHINE ═══════════════════════
            //
            // SEAM[spine:transitions] (K4, Jean, 2026-07-11): the transition
            //   machine and its working members — transitionPhase_,
            //   pendingDestination_, backPortalPosition_, cpuPortalArray_,
            //   mood_state_ and kin — are DECLARED SPINE-OWNED
            //   ORCHESTRATION per the §2 residency law, the same legitimacy
            //   class as the P5 readbacks. Mood (direction/mood.hpp) supplies
            //   vocabulary + appliers + six doors and owns NO instance;
            //   struct MoodState's TYPE lives in contracts/spine_state.hpp.
            //   Constitution §2 carries the K4 line.
            // SEAM[spine:portal-system] consumed by the mood module
            //   (force_spawn_* functions read pendingDestination_), direction/input.hpp
            //   (keypress mood transitions request via mood.hpp's
            //   request_mood_transition), render() (readback callback drives
            //   portal trigger detection). PORTAL_COLORS lives in mood.hpp —
            //   portal color is mood vocabulary; the machine keeps the
            //   pending state and the trigger hooks.

            // enum TransitionPhase lives in contracts/spine_state.hpp;
            // the machine member stays here.
            TransitionPhase transitionPhase_ = TransitionPhase::IDLE;

            PortalDestination pendingDestination_{};

            GPUPortalArray cpuPortalArray_{};

            // ── Back-portal (guaranteed exit from finite worlds) ──
            // Position is configurable so special-case layouts can relocate it.
            float backPortalPosition_[2] = { 10.0f, 0.0f };   // world XZ

            // ═══ GPU READBACK + WORLDGEN ═════════════════════════════════
            //
            // SEAM[spine:P5] readback state machines + world_state_.world_gen counter are
            //   pattern P5 (release-pending sentinel) at the spine level.
            //   Pawn + floater readbacks each protect against stale callbacks
            //   from previous worlds via world_state_.world_gen capture in the closure.
            //   Genuinely spine-owned, not a leak.

            enum class PawnReadbackState { IDLE, COPIED, MAPPING };
            PawnReadbackState pawnReadbackState_ = PawnReadbackState::IDLE;
            enum class FloaterReadbackState { IDLE, COPIED, MAPPING };
            FloaterReadbackState floaterReadbackState_ = FloaterReadbackState::IDLE;
            // The point readback (option A): runs ONLY in
            // camera-host — the camera's GPU position IS the point's, so
            // it must reach the CPU for the viewpoint set (streaming,
            // LOD, cull, orb). Pawn-host never arms this machine.
            enum class CameraReadbackState { IDLE, COPIED, MAPPING };
            CameraReadbackState cameraReadbackState_ = CameraReadbackState::IDLE;

            // ROSTER-RESIDUE gol (2e) instrumentation: count of frames the GoL
            // zone-compute block ran (the sole writer of the zone GPU buffers),
            // and the residue-report cadence timer. Read only by the residue
            // check when ROSTER.gol is disabled (proves the buffers pristine).
            uint64_t rosterGolZoneRuns_ = 0;
            float    rosterGolResidueDump_ = 0.0f;

            // ═══ FAMILY DISPATCH TABLE ═══════════════════════════════════
            //
            // SEAM[spine:owns] FAMILY_DISPATCH is the integration hub that
            //   ties the 12 families together. Each row's body lives in
            //   the family's owning module.
            // SEAM[spine:K2-related] the five real dispatch_prepare_mesh_* /
            //   dispatch_mesh_gen_* adapter pairs below are integration glue
            //   between FAMILY_DISPATCH and the per-family modules (their
            //   signatures adapt module preparers and renderer dispatches to
            //   the row slots). The bespoke select/place/commit funnels AND
            //   the twelve evictors live with their owners (§5 EVICTION
            //   THUNKS: retirement fulfilled); the no-op mesh adapters are
            //   shared (inlined beside the table, post-class).
            // SEAM[spine:family-dispatch] anchor for cross-file references —
            //   eviction routes through FAMILY_DISPATCH[f].evict_slot to the
            //   owner-side evict_<family> functions.
            //
            // The row type (struct FamilyDispatch) and the queue-entry
            // unions it walks (EntityQueueEntry / PlacementEntry) live in
            // entity_types.hpp — the contract home.

            // ═══ DISPATCH WRAPPERS ═══════════════════════════════════════

            // ── Mesh gen wrappers ──
            // The pyramid has none: it is the first entity whose realization IS
            // the terrain — it keeps its select/place/commit/evict verbs
            // (placement feeds the heightfield) but has no mesh realization of
            // its own; its FAMILY_DISPATCH mesh hook routes to the none-fork.

            static bool dispatch_prepare_mesh_arch(MachineCtx* self, wgpu::Queue& queue) {
                return prepare_arch_mesh_gen(self->entities_state_, self, queue);
            }
            static void dispatch_mesh_gen_arch(MachineCtx* self, wgpu::ComputePassEncoder& pass) {
                self->renderer_.dispatch_arch_mesh_gen(pass, self->gpuState_.arch_mesh_gen_group());
            }

            static bool dispatch_prepare_mesh_column(MachineCtx* self, wgpu::Queue& queue) {
                return prepare_column_mesh_gen(self->entities_state_, self, queue);
            }
            static void dispatch_mesh_gen_column(MachineCtx* self, wgpu::ComputePassEncoder& pass) {
                self->renderer_.dispatch_column_mesh_gen(pass, self->gpuState_.column_mesh_gen_group());
            }

            // ── Mesh gen dispatch wrappers (palm) ──

            static bool dispatch_prepare_mesh_palm(MachineCtx* self, wgpu::Queue& queue) {
                return prepare_palm_mesh_gen(self->entities_state_, self, queue);
            }
            static void dispatch_mesh_gen_palm(MachineCtx* self, wgpu::ComputePassEncoder& pass) {
                self->renderer_.dispatch_palm_mesh_gen(pass, self->gpuState_.palm_mesh_gen_group());
            }

            // ── Mesh gen dispatch wrappers (cactus) ──

            static bool dispatch_prepare_mesh_cactus(MachineCtx* self, wgpu::Queue& queue) {
                return prepare_cactus_mesh_gen(self->entities_state_, self, queue);
            }
            static void dispatch_mesh_gen_cactus(MachineCtx* self, wgpu::ComputePassEncoder& pass) {
                self->renderer_.dispatch_cactus_mesh_gen(pass, self->gpuState_.cactus_mesh_gen_group());
            }

            static bool dispatch_prepare_mesh_blade(MachineCtx* self, wgpu::Queue& queue) {
                return prepare_blade_mesh_gen(self->entities_state_, self, queue);
            }
            static void dispatch_mesh_gen_blade(MachineCtx* self, wgpu::ComputePassEncoder& pass) {
                self->renderer_.dispatch_blade_mesh_gen(pass, self->gpuState_.blade_mesh_gen_group());
            }

            // ── The dispatch table (FAMILY_DISPATCH) is defined at file
            //    scope after the class, beside the shared no-op adapters
            //    (declared in entity_types.hpp). ──
            //
            // The spine-owned piece-enable manifest (struct Roster, the
            // ROSTER constant, the transitions=>portal edge, and the full doc
            // block — RIDER A / MATURITY DIAL / FOUNDATIONAL / LATENT /
            // gate-(a) status column) now lives in
            // cartridges/the_board/contracts/roster.hpp. It met its SECOND CONSUMER —
            // GPUState::init (state.hpp) gates creation on the feature bits —
            // so the reading publishes at the shared header (the standing
            // law). ROSTER is visible here by namespace lookup
            // (t7::the_board::ROSTER); every ROSTER-GATE / ROSTER-RESIDUE
            // consult below is unchanged.

        public:

            // ═══ PUBLIC: CARTRIDGE LIFECYCLE ═════════════════════════════

            Cartridge()
                : machine_ctx_{ world_state_, tile_world_state_, themes_state_,
                                mood_state_, patch_system_state_, spawn_engine_state_,
                                entities_state_, sphere_state_, cube_behaviors_state_,
                                ribbon_state_, gol_state_, gallery_state_,
                                time_state_, player_, gpuState_, renderer_ }
                , tile_world_deps_{ world_state_, mood_state_, gpuState_ }
                , sphere_deps_{ time_state_ }
                , pawn_deps_{ player_, time_state_, gpuState_, renderer_ }
                , orbs_deps_{ gpuState_, renderer_, player_, time_state_, world_state_ }
                , agents_deps_{ gpuState_, player_, transitionPhase_, world_state_, time_state_ }
                , cube_deps_{ gpuState_, time_state_, player_, mood_state_ }
                , gol_deps_{ gpuState_, renderer_, device_, time_state_ }
                , ribbon_deps_{ gpuState_, time_state_, tile_world_state_, player_, inputState_, world_state_, mood_state_, visual_canvas_, ribbon_amp_lat_dst_, ribbon_amp_vert_dst_, ribbon_tint_stim_dst_, ribbon_tint_mix_dst_ }
                , gallery_deps_{ gpuState_, renderer_, world_state_, tile_world_state_, ribbon_state_, player_, mood_state_, sunDirection_, clearColor_ }
                , input_deps_{ inputState_, keys_, mouse_, player_, world_state_, ribbon_state_, gpuState_, device_, point_ }
                , mood_deps_{ mood_state_, world_state_, gpuState_, renderer_, gol_state_, entities_state_, sunDirection_, sunColor_, clearColor_, cpuSpotLights_, cpuPortalArray_, backPortalPosition_ } {
                // THE ROOT AUTHORS THE BOOT VALUES (the demo sentence lands
                // here, not via in-struct defaults — no include-order cable).
                world_state_.active_seed = DEMO.seed;
                mood_state_.active = DEMO.boot_mood;
            }

            Cartridge(const Cartridge&) = delete;
            Cartridge& operator=(const Cartridge&) = delete;

            void initialize(wgpu::Device device) override {
                device_ = device;
                auto tGpu0 = std::chrono::high_resolution_clock::now();
                gpuState_.init(device);
                auto tGpu1 = std::chrono::high_resolution_clock::now();
                std::cout << "[Cartridge] GPUState init:    "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(tGpu1 - tGpu0).count()
                    << " ms\n";

                {
                    // The surface voice's terrain rows read THE
                    // TERRAIN_LOOKS PANEL ROW 2 (surface/
                    // terrain_looks.hpp) — the rest column lives where
                    // the parameters live. Values unchanged: blend -1
                    // = inactive, everything else 0.
                    gpuState_.set_band_motion(terrain_looks::REST_BAND_BLEND,
                        terrain_looks::REST_BAND_PHASE_ORIGIN);
                    gpuState_.set_terrain_time(terrain_looks::REST_TERRAIN_TIME);
                    gpuState_.set_mode_color_shift(terrain_looks::REST_MODE_COLOR_SHIFT);
                    gpuState_.set_mode_checker_scatter(terrain_looks::REST_MODE_CHECKER_SCATTER);
                    gpuState_.set_mode_palette_drift(terrain_looks::REST_MODE_PALETTE_DRIFT_TARGET,
                        terrain_looks::REST_MODE_PALETTE_DRIFT_INTENSITY,
                        terrain_looks::REST_MODE_PALETTE_DRIFT_TIER);
                    gpuState_.set_checker_color_field(terrain_looks::REST_CHECKER_RESULTANT,
                        terrain_looks::REST_CHECKER_AMOUNT,
                        terrain_looks::REST_CHECKER_VARIANCE);
                    gpuState_.set_mode_gol_scales(1.0f, 1.0f);   // GoL's jurisdiction — stays inline (ROW 9 pointer)
                    // Pulse ring rest — the count is a ROW 2 pin; the
                    // zeroed ring is the rest (Phase 1, C4-F1).
                    float zero_pulses[32] = {};
                    gpuState_.set_pulse_data(terrain_looks::REST_PULSE_COUNT, zero_pulses);
                    // The CameraControls panel authors the fly speed
                    // — one dial, one writer, at boot.
                    gpuState_.set_point_fly_speed(CameraControls::MOVE_SPEED);
                }

                // E-3 (mechanized): boot-neutral the sky_* block ONCE. The signal
                // drain no longer carries these words (upload_signal skips the
                // trailing 32 bytes), so their sole per-frame author is
                // resync_sky_head (R7, ribbon on). This one write covers the
                // ribbon-OFF case: the block stays neutral 0 forever, matching the
                // old per-frame sky-neutral placeholder exactly. One writer, one
                // region — the neutral-then-overwrite relay is gone.
                wgpu::Queue q = device_.GetQueue();
                gpuState_.resync_sky_head(q, 0u,
                    0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
            }

            bool init_renderer(
                wgpu::TextureFormat colorFormat,
                wgpu::TextureFormat depthFormat
            ) {
                colorFormat_ = colorFormat;
                depthFormat_ = depthFormat;

                validate_spine();  // BOOT: table-order + O-5b/c face law (the O-#/RC laws are static-asserted)

                auto t0 = std::chrono::high_resolution_clock::now();
                if (!renderer_.init(
                    device_,
                    gpuState_,
                    colorFormat,
                    depthFormat
                )) return false;

                // Create offscreen textures with the actual swapchain format
                if (!gpuState_.initOffscreenResources(colorFormat)) {
                    std::cerr << "[Cartridge] Failed to init offscreen resources\n";
                    return false;
                }

                auto t1 = std::chrono::high_resolution_clock::now();

                // ═══ MOVEMENT: BOOT — REALIZATION (the stage exists first) ══
                // --- One-shot: generate terrain index buffer on GPU -----------------
                {
                    wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
                    wgpu::ComputePassDescriptor desc{};
                    desc.label = "Terrain Index Gen (one-shot)";
                    wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&desc);
                    renderer_.dispatch_generate_terrain_indices(
                        pass,
                        gpuState_.terrain_index_gen_group(),
                        GPUState::terrain_mesh_workgroups()
                    );
                    pass.End();
                    wgpu::CommandBuffer cmd = encoder.Finish();
                    device_.GetQueue().Submit(1, &cmd);
                }
                auto t2 = std::chrono::high_resolution_clock::now();

                // ═══ MOVEMENT: BOOT — S2 THE SURFACE ════════════════════════
                init_patch_system(&machine_ctx_, tile_world_state_);
                setup_test_rig_piers(&machine_ctx_, device_.GetQueue());

                // ═══ MOVEMENT: BOOT — PER-PIECE BOOT VERBS (part one) ═══════
                // Order is today's, preserved byte-for-byte (PRIME INVARIANT);
                // one conductor call per piece, presence constexpr-gated.
                // Sky orbs for the initial mood (apply_mood runs only on transitions).
                if constexpr (ROSTER.orbs) {  // ROSTER-GATE orbs (c) — boot one-shot skipped when disabled
                    wgpu::Queue q = device_.GetQueue();
                    configure_orbs(orbs_state_, &orbs_deps_, ORB_MOOD_TABLE[mood_state_.active], q);
                }

                // Agent registries — single source of truth in bodies/agents.hpp
                // (AGENT_BEHAVIORS / AGENT_TIER_GAINS), uploaded once to GPU
                // storage buffers at bindings 110 + 111. Values are
                // constexpr-equivalent and never change during a session,
                // so this is a one-shot write at boot.
                {
                    wgpu::Queue q = device_.GetQueue();
                    upload_agent_registries_to_gpu(&agents_deps_, q);
                }

                // ═══ MOVEMENT: BOOT — S3 PLACEMENT ══════════════════════════
                {
                    // Slot 0, the pawn — ungated: the player body is
                    // unconditional (owner verb).
                    seed_player_body(agent_state_, &agents_deps_);

                    wgpu::Queue q = device_.GetQueue();
                    // ROSTER-GATE wanderers (c) — boot population (agent slots
                    // 1+). Slot 0 (the pawn, seeded just above) is untouched.
                    if constexpr (ROSTER.wanderers)
                        spawn_population_for_mood(agent_state_, &agents_deps_, mood_state_.active, world_state_.active_seed,
                            Idle::PAWN_POS_X, Idle::PAWN_POS_Z, q);
                    dump_agent_census(agent_state_, &agents_deps_, "boot");
                }

                // ═══ MOVEMENT: BOOT — PER-PIECE BOOT VERBS (part two) ═══════
                // Eager-load authored paintings at boot (avoids mid-frame stall on first gallery).
                // ROSTER-GATE gallery (c) — P2 DIES STRUCTURALLY:
                // disabled, the authored-staging textures stay pristine.
                if constexpr (ROSTER.gallery) {
                    wgpu::Queue q = device_.GetQueue();
                    load_authored_textures(gallery_state_, gpuState_, q);
                }

                auto t3 = std::chrono::high_resolution_clock::now();

                std::cout << "[Cartridge] Renderer init:    "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms\n";
                std::cout << "[Cartridge] Terrain gen:      "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " ms\n";
                std::cout << "[Cartridge] Patch system:     "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count() << " ms\n";
                std::cout << "[Cartridge] Total init:       "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t0).count() << " ms\n";

                if constexpr (!ROSTER.all_enabled()) {
                    std::string off;
                    auto mark = [&](bool enabled, const char* name) {
                        if (!enabled) { if (!off.empty()) off += ", "; off += name; }
                        };
                    mark(ROSTER.pyramid, "pyramid");     mark(ROSTER.arch, "arch");
                    mark(ROSTER.column, "column");       mark(ROSTER.antenna, "antenna");
                    mark(ROSTER.palm, "palm");           mark(ROSTER.cactus, "cactus");
                    mark(ROSTER.blade, "blade");         mark(ROSTER.sphere, "sphere");
                    mark(ROSTER.ribbon, "ribbon");       mark(ROSTER.cube, "cube");
                    mark(ROSTER.gol, "gol");             mark(ROSTER.gallery, "gallery");
                    mark(ROSTER.pawn_aura, "pawn_aura"); mark(ROSTER.orbs, "orbs");
                    mark(ROSTER.spot_lights, "spot_lights");
                    mark(ROSTER.indoor_shell, "indoor_shell");
                    mark(ROSTER.portal, "portal");       mark(ROSTER.transitions, "transitions");
                    mark(ROSTER.wanderers, "wanderers");
                    // Buffer creation: only indoor_shell (SEP) skips in v0;
                    // pipelines gate per piece (gate a').
                    const char* skipped = ROSTER.indoor_shell
                        ? "(none — every disabled piece is SH-shared, created-pristine)"
                        : "indoor_shell (shell VB/IB)";
                    std::cout << "[ROSTER] pieces disabled: " << off
                        << " | buffer creations skipped: " << skipped
                        << " | pipelines skipped: " << Renderer::pipelines_skipped() << "\n";
                }

                return true;
            }

            void bind_signal_layout(StatLayoutView v) {
                visual_canvas_.bind(v);
                fog_density_dst_ = visual_canvas_.layout().resolve("fog.density");
                fog_color_dst_ = visual_canvas_.layout().resolve("fog.color");
                ribbon_amp_lat_dst_ = visual_canvas_.layout().resolve("ribbon.amp_lateral_mult");
                ribbon_amp_vert_dst_ = visual_canvas_.layout().resolve("ribbon.amp_vertical_mult");
                ribbon_tint_stim_dst_ = visual_canvas_.layout().resolve("ribbon.color_stim");
                ribbon_tint_mix_dst_ = visual_canvas_.layout().resolve("ribbon.color_mix");
                checker_mean_dst_ = visual_canvas_.layout().resolve("terrain.checker_mean");
                checker_var_dst_ = visual_canvas_.layout().resolve("terrain.checker_var");
                std::fprintf(stderr,
                    "[the_board] fog.density base=%d valid=%d | fog.color base=%d count=%d valid=%d\n",
                    fog_density_dst_.base, (int)fog_density_dst_.valid,
                    fog_color_dst_.base, fog_color_dst_.count, (int)fog_color_dst_.valid);
                std::fprintf(stderr,
                    "[the_board] terrain.checker_mean base=%d count=%d valid=%d | terrain.checker_var base=%d valid=%d\n",
                    checker_mean_dst_.base, checker_mean_dst_.count, (int)checker_mean_dst_.valid,
                    checker_var_dst_.base, (int)checker_var_dst_.valid);
            }

            // ═══════════════════════════════════════════════════════════════
            // THE FRAME SPINE — the program's
            // temporal dispatch table.
            //
            // The frame is an AUTHORED order, CHECKED by validation, never
            // COMPUTED. update() and render() are LOOPS over two constexpr spine
            // tables (UPDATE_SPINE / RENDER_SPINE, below the phase methods); each
            // row is {phase id, name, member fn, driver (§9), roster gate, face
            // tags}. The row order IS the frame order; the O-# / RC laws are
            // static_asserts over the row indices (see § SPINE VALIDATION). No
            // topo-solver — the deliberate stale-reads and write-order designs
            // are LAWS, declared here (recon §2):
            //
            //   LAW E-4 (witness lag): the readback is 1 frame stale BY DESIGN.
            //     R1 WitnessHarvest consumes the capture R11 WitnessCapture wrote
            //     LAST frame (O-2). Player pos / portal trigger / owner mirrors
            //     all lag one frame — every downstream consumer is written to
            //     eat a one-frame-old point.
            //   LAW E-9 (portal spans a frame): R2 PortalTrigger arms the
            //     transition from a GPU-reported trigger; U7 TransitionMachine
            //     consumes it NEXT frame (update precedes render within a frame).
            //     A portal step is render N arms -> update N+1 advances; the
            //     one-frame readback lag (E-4) stacks on top.
            //   E-3 (sky write-order) — MECHANIZED, NO LONGER A LAW. It was
            //     a three-writer relay: U2 wrote neutral sky words, U8 uploaded
            //     the whole signal, R7's tail (resync_sky_head) overwrote them,
            //     and correctness rode submission order across update()->render().
            //     Now the sky_* words are the TRAILING 32 bytes and upload_signal
            //     skips them, so resync_sky_head is their SOLE author (R7 per
            //     frame; a boot-neutral in initialize() covers ribbon-off). The
            //     drain and the sky author write DISJOINT regions — there is no
            //     ordering to preserve, so there is no law. Structure replaced the
            //     paragraph. (E-1 dies with it on the signal side: a setter can no
            //     longer land in the sky window and be clobbered by the relay.)
            //
            // Gates are ROW COLUMNS: a disabled family's row is skipped at
            // runtime (row.enabled folds from its constexpr ROSTER bit). Runtime
            // data-guards (zone_count>0, dirty flags) live INSIDE their phase.
            // ═══════════════════════════════════════════════════════════════

            // Frame-transient inputs, bundled so every phase has ONE uniform
            // signature (the row's fn type). A phase reads only what it needs.
            struct UpdateCtx {
                const AnalysisSignal& signal;
                float                 aspect_ratio;
                wgpu::Queue& queue;
                GPUFrameSignal& gpuSignal;   // U1 fills it; U8 drains it (sky_* excluded — E-3 mechanized)
            };
            struct RenderCtx {
                wgpu::CommandEncoder& encoder;
                wgpu::Queue& queue;
                wgpu::TextureView     backbuffer;
                wgpu::TextureView     depth;
            };

            // §9 driver law (input.hpp:102 — a driver writes intents through
            // bodies it does not own). None = foundational spine work (no bit).
            enum class Driver : uint8_t { Input, Algo, Music, WallClock, Mixed, None };

            // Coarse face tags — the frame-truth axes a phase touches (recon §3).
            enum FaceTag : uint32_t {
                F_NONE = 0,
                F_SIGNAL = 1u << 0,   // the GPU signal buffer (clock/input/stats)
                F_CONFIG = 1u << 1,   // the GPU config buffer (fog/world/fade/...)
                F_CLOCK = 1u << 2,   // time_state_ (beats/seconds/dt/prev_beats)
                F_WITNESS = 1u << 3,   // the readback record (agent/floater/camera)
                F_GROUND = 1u << 4,   // ground-entries / placement dirty cascade
                F_COMPUTE = 1u << 5,   // encodes a GPU compute pass
                F_DRAW = 1u << 6,   // encodes a GPU render pass
                F_SUBMIT = 1u << 7,   // issues its OWN queue submit (hidden)
                F_TRANSITION = 1u << 8,   // the transition machine / mood
                F_STREAM = 1u << 9,   // patch streaming (S2)
            };

            // Phase ids — DECLARATION ORDER == AUTHORED ORDER == ROW INDEX.
            // (The spine tables are asserted dense + in this order; the O-#/RC
            //  laws are static_asserts over these indices.)
            enum class UPhase : uint32_t {
                FillSignal, AdvanceClock, MotionDrivers, MotionBodies,
                StageWorld, TransitionMachine, StageFadeUpload, WitnessPhotographer,
                ClearInputDeltas, COUNT
            };
            enum class RPhase : uint32_t {
                WitnessHarvest, PortalTrigger, StreamPatches, RespawnAgents, MotionCorral,
                CensusDumps, RibbonTick, EntityMeshGen, UploadPortalLights, LiveCardWrite, DispatchCompute,
                WitnessCapture, GolDeriveFlush, GolZoneCompute, PawnAura, OrbSky,
                GroundEntries, PlacementCorrection, FrustumCull, ShadowPass, MainPass,
                SnapshotPass, PromotionDrain, COUNT
            };

            // Row shapes (the FAMILY_DISPATCH shape, one clock per conductor).
            struct URow {
                UPhase                          id;
                const char* name;
                void (Cartridge::* fn)(UpdateCtx&);
                Driver                          driver;
                bool                            enabled;   // roster gate (constexpr-folded)
                uint32_t                        face;
            };
            struct RRow {
                RPhase                          id;
                const char* name;
                void (Cartridge::* fn)(RenderCtx&);
                Driver                          driver;
                bool                            enabled;
                uint32_t                        face;
            };

            // U1 — SIGNAL FILL (music+input+wall-clock). Build the GPU signal
            // from analysis + input. O-5a: dt_beats reads prev_beats BEFORE the
            // clock advances it at U3. Input deltas were harvested by on_input.
            void phase_fill_signal(UpdateCtx& c) {
                auto& gpuSignal = c.gpuSignal;
                auto& signal = c.signal;
                auto aspect_ratio = c.aspect_ratio;
                gpuSignal.t_seconds = signal.t_seconds;
                gpuSignal.t_beats = signal.t_beats;
                gpuSignal.dt = signal.dt;
                gpuSignal.aspect_ratio = aspect_ratio;

                for (size_t i = 0; i < gpuSignal.stats.size(); ++i) {
                    gpuSignal.stats[i] = signal.stats[i];
                }

                gpuSignal.move_x = inputState_.move_x;
                gpuSignal.move_z = inputState_.move_z;
                gpuSignal.look_az_delta = inputState_.look_az_delta;
                gpuSignal.look_el_delta = inputState_.look_el_delta;
                gpuSignal.zoom_delta = inputState_.zoom_delta;
                gpuSignal.pan_x_delta = inputState_.pan_x_delta;
                gpuSignal.pan_y_delta = inputState_.pan_y_delta;
                gpuSignal.dt_beats = signal.t_beats - time_state_.prev_beats;  // beats since last frame -> step_trigger
            }

            // U2 (sky-neutral) REMOVED — E-3 MECHANIZED. The sky block is no
            // longer part of the signal drain: upload_signal skips the trailing
            // 32 bytes, so the block's SOLE author is resync_sky_head (R7, the
            // ribbon tick's tail), with a boot-neutral (initialize) covering the
            // ribbon-off case. One writer of a disjoint region — no neutral-then-
            // overwrite relay, no submission-order paragraph across two functions.

            // U3 — ADVANCE CLOCK (music+wall-clock). The tempo follower; bumps
            // prev_beats (the O-5a partner of U1's dt_beats read).
            void phase_advance_clock(UpdateCtx& c) {
                auto& signal = c.signal;
                time_state_.beats = signal.t_beats;
                time_state_.seconds = signal.t_seconds;
                time_state_.dt = signal.dt;
                {
                    const float db = signal.t_beats - time_state_.prev_beats;
                    if (db > 1e-6f && time_state_.dt > 1e-6f)
                        time_state_.beat_rate = db / time_state_.dt;
                    time_state_.prev_beats = signal.t_beats;
                }
            }

            // U4 — MOTION DRIVERS (music). The music driver authors params
            // through the canvas; fog is its first staged consumer. (Input was
            // harvested by the on_input callbacks; its deltas rode U1.)
            void phase_motion_drivers(UpdateCtx& c) {
                auto& signal = c.signal;
                visual_canvas_.tick(signal);
                if (fog_density_dst_.valid && fog_color_dst_.valid) {
                    const VisualParams& fp = visual_canvas_.params();
                    gpuState_.set_fog(fp.get(fog_density_dst_.base),
                        fp.get(fog_color_dst_.base + 0),
                        fp.get(fog_color_dst_.base + 1),
                        fp.get(fog_color_dst_.base + 2));
                }
                // CHECKER-REBUILD: the pc-color field's flush — one setter,
                // the fan (resultant rgb + music amount + music variance).
                if (checker_mean_dst_.valid && checker_var_dst_.valid) {
                    const VisualParams& cp = visual_canvas_.params();
                    gpuState_.set_checker_color_field(cp.run(checker_mean_dst_.base),
                        cp.get(checker_var_dst_.base),        // [0] = music_amount
                        cp.get(checker_var_dst_.base + 1));   // [1] = music_variance
                    // [FLUSH] one-shot: fires the first time a live resultant
                    // crosses the CPU->GPU seam. If [CHECKER] is singing in the
                    // console and this line never prints, the bindings above are
                    // invalid or the two params_ objects disagree — name it.
                    static bool checker_flush_seen = false;
                    if (!checker_flush_seen
                        && cp.get(checker_var_dst_.base) > 0.05f) {   // music_amount up
                        std::fprintf(stderr,
                            "[FLUSH] checker -> config: resultant=(%.2f %.2f %.2f) amount=%.2f var=%.2f\n",
                            cp.get(checker_mean_dst_.base),
                            cp.get(checker_mean_dst_.base + 1),
                            cp.get(checker_mean_dst_.base + 2),
                            cp.get(checker_var_dst_.base),
                            cp.get(checker_var_dst_.base + 1));
                        checker_flush_seen = true;
                    }
                }

            }

            // U5 — MOTION BODIES (wall-clock). Pawn presence ramp + aura height
            // (bodies/pawn.hpp real-time exponential tick; closes pawn:K1).
            // ROSTER-GATE pawn_aura (b) — guarded at the call site.
            void phase_motion_bodies(UpdateCtx& c) {
                auto& queue = c.queue;
                tick_pawn_couplings(pawn_state_, &pawn_deps_, queue);
            }

            // U6 — STAGE WORLD (seed + finite bounds, algo). Stays PRE-machine
            // (RC policy): the TEARDOWN case re-stages the seed itself, and
            // moving the bounds after the machine would ship the NEW world's
            // bounds one frame early on the teardown frame.
            void phase_stage_world(UpdateCtx&) {
                gpuState_.set_world_seed(world_state_.active_seed);
                if (world_state_.finite_mode) {
                    float bmin = -(float)world_state_.finite_radius * Dim::PATCH_EXTENT;
                    float bmax = ((float)world_state_.finite_radius + 1.0f) * Dim::PATCH_EXTENT;
                    gpuState_.set_world_bounds(bmin, bmin, bmax, bmax);
                }
                else {
                    gpuState_.set_world_bounds(0.0f, 0.0f, 0.0f, 0.0f);
                }
                // THE VEIL (ruled): OFF in finite/indoor — walls define the
                // boundary there, not fog (the same law that makes all patches
                // visible in finite mode). Dirty-gated; rides the U8 drain.
                gpuState_.set_veil_strength(world_state_.finite_mode ? 0.0f : 1.0f);
            }

            // U7 — THE TRANSITION MACHINE (spine-owned; SEAM[spine:transitions]).
            // ONE phase; internals untouched. FADE_OUT/TEARDOWN/FADE_IN; the
            // TEARDOWN arm owns the worldGen bump (P5 guard), return-state
            // capture, per-owner teardown verbs, agent reset, repopulation.
            void phase_transition_machine(UpdateCtx& c) {
                auto& signal = c.signal;
                auto& queue = c.queue;
                if (transitionPhase_ != TransitionPhase::IDLE) {
                    mood_state_.transition_timer += signal.dt;
                    switch (transitionPhase_) {
                    case TransitionPhase::FADE_OUT:
                        mood_state_.transition_fade_alpha = std::min(1.0f, mood_state_.transition_timer / mood_state_.transition_fade_duration);
                        if (mood_state_.transition_fade_alpha >= 1.0f) {
                            transitionPhase_ = TransitionPhase::TEARDOWN;
                        }
                        break;
                    case TransitionPhase::TEARDOWN:
                    {
                        // ═══ MOVEMENT: TEARDOWN (fixed sequence O-3) ════════
                        // This TEARDOWN block owns the integration concerns:
                        //   worldGen bump (P5 stale-callback guard),
                        //   return-state capture, per-owner teardown verbs
                        //   (the owner-verb pattern that already lived
                        //   inside teardown_world), agent reset, repopulation.
                        // SEAM[spine:P5] world_state_.world_gen++ at top of TEARDOWN is the
                        //   stale-callback guard (P5 family). Genuinely
                        //   spine-owned.

                        world_state_.world_gen++;

                        // Capture return seed + mood + radius before overwrite
                        mood_state_.back_portal_return_seed = world_state_.active_seed;
                        mood_state_.back_portal_return_mood = mood_state_.active;
                        mood_state_.back_portal_return_radius = world_state_.finite_radius;

                        world_state_.active_seed = pendingDestination_.seed;
                        world_state_.finite_mode = pendingDestination_.finite;
                        world_state_.finite_radius = pendingDestination_.finite_radius;

                        // The surface core first, then one teardown verb per
                        // owner. The per-organ clears are independent (each
                        // touches only its organ + its own GPU slots), so the
                        // owner-verb order is free; the new gates eliminate
                        // only zeros-over-pristine GPU writes (disclosed at
                        // the ladder).
                        teardown_surface(&machine_ctx_, queue, tile_world_state_, themes_state_);
                        teardown_entities(&machine_ctx_, queue);
                        if constexpr (ROSTER.gol)      // ROSTER-GATE gol (c) — teardown clear skipped when disabled (organ pristine)
                            teardown_gol(gol_state_, &gol_deps_, queue);
                        if constexpr (ROSTER.ribbon)   // ROSTER-GATE ribbon (c) — same zero-write elimination
                            teardown_ribbon(ribbon_state_, &ribbon_deps_, queue);
                        if constexpr (ROSTER.sphere)   // ROSTER-GATE sphere (c)
                            clear_spheres(sphere_state_, gpuState_, queue);
                        if constexpr (ROSTER.cube)     // ROSTER-GATE cube (c)
                            clear_cubes(cube_behaviors_state_, gpuState_, queue);
                        // The gallery organ is SHARED with indoor_shell (wall
                        // frames live in the same painting slots — form_type).
                        if constexpr (ROSTER.gallery || ROSTER.indoor_shell)  // ROSTER-GATE gallery+indoor_shell (c)
                            teardown_gallery(gallery_state_, &gallery_deps_, queue);
                        if constexpr (ROSTER.pawn_aura)  // ROSTER-GATE pawn_aura (c) — teardown clear skipped when disabled (no aura to clear)
                            teardown_pawn_aura(pawn_state_);
                        // Sky orbs: apply_mood re-enables + re-seeds as needed
                        if constexpr (ROSTER.orbs)  // ROSTER-GATE orbs (c) — teardown one-shot skipped when disabled
                            teardown_orbs(orbs_state_, &orbs_deps_);

                        player_.readback_portal_trigger = -1;
                        player_.readback_x = 0.0f;
                        player_.readback_z = 0.0f;
                        uint32_t preserved_tier = agent_state_.slots[player_.possessed_slot].tier_idx;
                        float preserved_color_r = agent_state_.slots[player_.possessed_slot].color_r;
                        float preserved_color_g = agent_state_.slots[player_.possessed_slot].color_g;
                        float preserved_color_b = agent_state_.slots[player_.possessed_slot].color_b;
                        // The figure travels with the body you inhabit (CLOSURE_PAWN [5]).
                        uint32_t preserved_skin = agent_state_.slots[player_.possessed_slot].skin_id;

                        gpuState_.reset_player_agent(queue, preserved_tier,
                            preserved_color_r, preserved_color_g, preserved_color_b,
                            preserved_skin);
                        gpuState_.set_possessed_slot(0);
                        // CPU mirror reseed rides with its owner (agents).
                        reseed_player_body(agent_state_, &agents_deps_, preserved_tier,
                            preserved_color_r, preserved_color_g, preserved_color_b,
                            preserved_skin);
                        gpuState_.set_world_seed(world_state_.active_seed);
                        apply_mood(&mood_deps_, pendingDestination_.mood, queue,
                            machine_ctx_, ribbon_state_, ribbon_deps_,
                            orbs_state_, orbs_deps_, gallery_state_, gallery_deps_,
                            pawn_state_);
                        // ROSTER-GATE wanderers (c) — transition population (slots 1+); slot 0 preserved above.
                        if constexpr (ROSTER.wanderers)
                            spawn_population_for_mood(agent_state_, &agents_deps_, pendingDestination_.mood, world_state_.active_seed,
                                Idle::PAWN_POS_X, Idle::PAWN_POS_Z, queue);
                        dump_agent_census(agent_state_, &agents_deps_, "mood-transition");
                        // ROSTER-GATE ribbon (c) — finite-mode release, owner
                        // verb. Zero effect
                        // when ribbon is off (active_count stays 0). ORDER
                        // (O-3): after apply_mood set mood_state_.active.
                        if constexpr (ROSTER.ribbon)
                            release_finite_ribbons(ribbon_state_, &ribbon_deps_, queue);
                        // Schedule guaranteed back-portal in finite worlds
                        mood_state_.back_portal_pending = world_state_.finite_mode;

                        transitionPhase_ = TransitionPhase::FADE_IN;
                        mood_state_.transition_timer = 0.0f;
                        uint32_t side = world_state_.finite_mode ? 2 * world_state_.finite_radius + 1 : 0;
                        std::cout << "[World] Teardown complete, seed=" << world_state_.active_seed
                            << " mode=" << (world_state_.finite_mode ? "finite" : "open")
                            << (world_state_.finite_mode ? " " + std::to_string(side) + "x" + std::to_string(side) : "")
                            << "\n";
                    }
                    break;
                    case TransitionPhase::FADE_IN:
                        mood_state_.transition_fade_alpha = std::max(0.0f, 1.0f - mood_state_.transition_timer / mood_state_.transition_fade_duration);
                        if (mood_state_.transition_fade_alpha <= 0.0f) {
                            transitionPhase_ = TransitionPhase::IDLE;
                            mood_state_.transition_fade_alpha = 0.0f;
                        }
                        break;
                    default: break;
                    }
                }
            }

            // U8 — STAGE FADE + THE TWO UPLOADS (O-5b/c). Fade after the machine
            // (alpha is current-frame); upload_signal then upload_config AFTER
            // all staging setters — the O-5b/c face law, enforced by
            // validate_spine at boot.
            void phase_stage_fade_and_upload(UpdateCtx& c) {
                auto& gpuSignal = c.gpuSignal;
                auto& queue = c.queue;
                gpuState_.set_fade(mood_state_.transition_fade_alpha, 0.0f, 0.0f, 0.0f);
                gpuState_.upload_signal(queue, gpuSignal);
                gpuState_.upload_config(queue);
            }

            // U9 — WITNESS: PHOTOGRAPHER (algo). The orb dome anchor movement
            // retired (skybox — eye-centered in the orb VS; no CPU
            // upload). ROSTER-GATE gallery (b) — P1 dies structurally in a
            // gallery-less demo; guarded at the call site.
            void phase_witness_photographer(UpdateCtx& c) {
                auto& queue = c.queue;
                update_photographer(gallery_state_, &gallery_deps_, queue);
            }

            // U10 — DRIVER BOOKKEEPING (O-5e, dead-last): U1's signal fill
            // consumed the deltas.
            void phase_clear_input_deltas(UpdateCtx&) {
                clear_input_deltas(&input_deps_);
            }

            // ── THE CONDUCTOR (update) — a LOOP over UPDATE_SPINE (§1a) ─────
            void update(const AnalysisSignal& signal,
                float aspect_ratio,
                wgpu::Queue& queue) override {
                GPUFrameSignal gpuSignal{};   // sky_* stay zero — upload_signal skips them (E-3); resync_sky_head owns the block
                UpdateCtx ctx{ signal, aspect_ratio, queue, gpuSignal };
                for (const URow& row : UPDATE_SPINE)
                    if (row.enabled) (this->*row.fn)(ctx);
            }

            // SEAM[spine:owns] render() is genuinely spine work: readback state
            //   machines, stale-callback guards, portal trigger handling,
            //   patch streaming, photographer cadence. The K1 observation
            //   doesn't apply to render() the same way it applies to update();
            //   render() mixes orchestration (correct) with smaller per-module
            //   GPU upload calls (each lives in its module already).
            // ═══════════════════════════════════════════════════════════════
            // THE FRAME SPINE — render() phases
            //
            // THE EXTRACTION: the movements R1..R21 are now
            // named phase methods; render() at the tail of this block is a page
            // of calls. PURE LIFT — no reordering, no logic change. A whole-
            // movement `if constexpr(ROSTER.x)` gate stays at the CALL SITE
            // (→ CUT-2 spine-row column); runtime data-guards live inside the
            // phase. R12 splits per the ruling: the HIDDEN SUBMIT (derive flush)
            // is its own named phase, distinct from the zone sync/evolve/mesh.
            // ═══════════════════════════════════════════════════════════════

            // R1 — WITNESS HARVEST (algo; P5 maps; consumes LAST frame's
            // capture). Leads the score: every downstream consumer (stream
            // center, portal door, corral, sorts) eats its output. The CAPTURE
            // half (R11) sits after dispatch_compute (O-2).
            void phase_witness_harvest(RenderCtx&) {
                if (pawnReadbackState_ == PawnReadbackState::COPIED) {
                    pawnReadbackState_ = PawnReadbackState::MAPPING;
                    gpuState_.agent_state_readback_staging().MapAsync(
                        wgpu::MapMode::Read, 0, GPUState::agent_state_buffer_size(),
                        wgpu::CallbackMode::AllowSpontaneous,
                        [this, gen = world_state_.world_gen](wgpu::MapAsyncStatus status, wgpu::StringView) {
                            if (status == wgpu::MapAsyncStatus::Success) {
                                // Drop stale callbacks from a previous world: gen
                                // captured at issue time differs from current
                                // world_state_.world_gen if a teardown happened in between.
                                // Buffer is still successfully mapped though, so
                                // we Unmap unconditionally (mapping contract is
                                // independent of whether we read the data).
                                if (gen == world_state_.world_gen) {
                                    const auto* data = static_cast<const GPUAgentState*>(
                                        gpuState_.agent_state_readback_staging().GetConstMappedRange(
                                            0, GPUState::agent_state_buffer_size()));
                                    if (data) {
                                        std::memcpy(agent_state_.slots, data,
                                            GPUState::agent_state_buffer_size());
                                        const auto& p = agent_state_.slots[player_.possessed_slot];
                                        // THE POINT: readback_x/z is the
                                        // POINT's position — the body authors it
                                        // only when the pawn hosts; the camera
                                        // harvest below authors it in camera-host.
                                        // The portal trigger is the point's BUBBLE
                                        // sensor, riding the possessed
                                        // slot's wire in both hosts.
                                        if (point_.host == PointHost::PAWN) {
                                            player_.readback_x = p.pos_x;
                                            player_.readback_z = p.pos_z;
                                        }
                                        player_.readback_portal_trigger = p.portal_trigger;
                                    }
                                }
                                gpuState_.agent_state_readback_staging().Unmap();
                            }
                            pawnReadbackState_ = PawnReadbackState::IDLE;
                        });
                }

                //
                // ── DIAG_FLOATER_BRIDGE (temporary) ───────────────────────
                static uint32_t dbg_fb_cb   = 0;   // times the readback callback ran
                static uint32_t dbg_fb_gsph = 0;   // GPU-side active spheres, last seen
                static uint32_t dbg_fb_gcub = 0;   // GPU-side active cubes,   last seen
                {
                    static float dbg_fb_last = -1.0f;
                    const float now_s = time_state_.seconds;
                    if (now_s - dbg_fb_last >= 1.0f) {
                        dbg_fb_last = now_s;
                        uint32_t fs = 0, fc = 0;
                        for (uint32_t i = 0; i < Dim::MAX_SPHERE_INSTANCES; i++)
                            if (sphere_state_.activeSpheres_[i].active) fs++;
                        for (uint32_t i = 0; i < Dim::MAX_CUBE_INSTANCES; i++)
                            if (cube_behaviors_state_.activeCubes_[i].active) fc++;
                        std::cout << "[FLOATER] sph n=" << sphere_state_.activeSphereCount_
                                  << " f=" << fs
                                  << " gpu=" << dbg_fb_gsph
                                  << " | cub n=" << cube_behaviors_state_.activeCubeCount_
                                  << " f=" << fc
                                  << " gpu=" << dbg_fb_gcub
                                  << " | cb=" << dbg_fb_cb
                                  << " st=" << static_cast<int>(floaterReadbackState_)
                                  << "\n";
                    }
                }
                // ── end DIAG_FLOATER_BRIDGE ─────────────────────────────
                if (floaterReadbackState_ == FloaterReadbackState::COPIED) {
                    floaterReadbackState_ = FloaterReadbackState::MAPPING;
                    gpuState_.floating_entity_readback_staging().MapAsync(
                        wgpu::MapMode::Read, 0, GPUState::floating_entity_buffer_size(),
                        wgpu::CallbackMode::AllowSpontaneous,
                        [this, gen = world_state_.world_gen](wgpu::MapAsyncStatus status, wgpu::StringView) {
                            if (status == wgpu::MapAsyncStatus::Success) {
                                // Drop stale callbacks from a previous world.
                                // Buffer is still mapped, so Unmap unconditionally.
                                if (gen == world_state_.world_gen) {
                                    const auto* data = static_cast<const GPUFloatingEntityState*>(
                                        gpuState_.floating_entity_readback_staging().GetConstMappedRange(
                                            0, GPUState::floating_entity_buffer_size()));
                                    if (data) {
                                        // Owner mirror reconciliation (funnels
                                        // live with spheres / cube_behaviors).
                                        if constexpr (ROSTER.sphere)  // ROSTER-GATE sphere (b) — no spheres, no mirror to release
                                            reconcile_sphere_mirror(sphere_state_, &sphere_deps_, data);
                                        if constexpr (ROSTER.cube)    // ROSTER-GATE cube (b)
                                            reconcile_cube_mirror(cube_behaviors_state_, &cube_deps_, data);
                                        // ── DIAG_FLOATER_BRIDGE (temporary) ───────────────────────
                                        {
                                            uint32_t gs = 0, gc = 0;
                                            for (uint32_t i = 0; i < Dim::MAX_SPHERE_INSTANCES; i++)
                                                if (data[i].is_active != 0u) gs++;
                                            for (uint32_t i = 0; i < Dim::MAX_CUBE_INSTANCES; i++)
                                                if (data[Dim::CUBE_SLOT_OFFSET + i].is_active != 0u) gc++;
                                            dbg_fb_gsph = gs;
                                            dbg_fb_gcub = gc;
                                            dbg_fb_cb++;
                                        }
                                        // ── end DIAG_FLOATER_BRIDGE ─────────────────────────────
                                    }
                                }
                                gpuState_.floating_entity_readback_staging().Unmap();
                            }
                            floaterReadbackState_ = FloaterReadbackState::IDLE;
                        });
                }

                // THE POINT's camera-host harvest (option A): when
                // the camera hosts the point, its GPU-resident position is
                // the point's position — read it back so the CPU viewpoint
                // set (streaming center, recenter, LOD banding, lod stage,
                // entity cull, orb anchor) follows the point. The pawn-host
                // frame never encodes the copy, so that path stays
                // byte-untouched (the binding pixel gate, by construction).
                if (cameraReadbackState_ == CameraReadbackState::COPIED) {
                    cameraReadbackState_ = CameraReadbackState::MAPPING;
                    gpuState_.camera_state_readback_staging().MapAsync(
                        wgpu::MapMode::Read, 0, GPUState::camera_state_buffer_size(),
                        wgpu::CallbackMode::AllowSpontaneous,
                        [this, gen = world_state_.world_gen](wgpu::MapAsyncStatus status, wgpu::StringView) {
                            if (status == wgpu::MapAsyncStatus::Success) {
                                // Drop stale callbacks from a previous world.
                                if (gen == world_state_.world_gen) {
                                    const auto* cam = static_cast<const GPUCameraState*>(
                                        gpuState_.camera_state_readback_staging().GetConstMappedRange(
                                            0, GPUState::camera_state_buffer_size()));
                                    // Host re-checked at harvest: a toggle
                                    // between copy and map must not let a
                                    // stale camera pos overwrite the pawn's
                                    // authorship.
                                    if (cam && point_.host == PointHost::CAMERA) {
                                        player_.readback_x = cam->pos[0];
                                        player_.readback_z = cam->pos[2];
                                    }
                                }
                                gpuState_.camera_state_readback_staging().Unmap();
                            }
                            cameraReadbackState_ = CameraReadbackState::IDLE;
                        });
                }
            }

            // R2 — PORTAL TRIGGER (algo; GPU event). ENTRY door #2: a GPU-
            // reported trigger arms a transition (consumed next frame by U7 —
            // recon E-9). ROSTER-GATE transitions — guarded at the call site.
            void phase_portal_trigger(RenderCtx&) {
                if (player_.readback_portal_trigger >= 0 && transitionPhase_ == TransitionPhase::IDLE) {
                    uint32_t arch_idx = static_cast<uint32_t>(player_.readback_portal_trigger);
                    player_.readback_portal_trigger = -1;
                    if (arch_idx < Dim::MAX_ARCH_INSTANCES &&
                        entities_state_.arches[arch_idx].active &&
                        entities_state_.arches[arch_idx].is_portal) {
                        pendingDestination_ = entities_state_.arches[arch_idx].destination;
                        transitionPhase_ = TransitionPhase::FADE_OUT;
                        mood_state_.transition_timer = 0.0f;
                        std::cout << "[Portal] GPU trigger: arch " << arch_idx
                            << " -> seed=" << pendingDestination_.seed
                            << " finite=" << pendingDestination_.finite << "\n";
                    }
                }
            }

            // R3 — STREAM PATCHES (S2 surface lifecycle, algo). The streaming
            // conductor; carries the S3-trigger seam (SEAM[patch:spawn-trigger]
            // — select/place/commit fire from the stream's own cadence).
            void phase_stream_patches(RenderCtx& c) {
                auto& encoder = c.encoder;
                auto& queue = c.queue;
                stream_patches(&machine_ctx_, encoder, queue, tile_world_state_, themes_state_, tile_world_deps_, mood_deps_, inputState_);
            }

            // R4 — RESPAWN AGENTS (S3, algo; RC-1: after stream). Refills slots
            // the GPU evicted last frame; slots 1+ only (slot 0 never evicted),
            // and the stream's bubble center reads readback_x/z refreshed at
            // HARVEST — no data edge. ROSTER-GATE wanderers — call site.
            void phase_respawn_agents(RenderCtx& c) {
                auto& queue = c.queue;
                respawn_evicted_agents(agent_state_, &agents_deps_, mood_state_.active, world_state_.active_seed, queue);
            }

            // R5 — MOTION CORRAL (S4, wall-clock; RC-2: after stream). Corral
            // tick + patch eviction touch disjoint cube fields per frame.
            // ROSTER-GATE cube — guarded at the call site.
            void phase_motion_corral(RenderCtx& c) {
                auto& queue = c.queue;
                tick_cube_corral_animations(cube_behaviors_state_, &cube_deps_, queue);
            }

            // R6 — CENSUS DUMPS (wall-clock interval, diagnostic). GoL residue
            // proof (G3, constexpr-gated intra-movement) + periodic agent census
            // + entity census. Autonomous stdout (constitution §5).
            void phase_census_dumps(RenderCtx&) {
                // ROSTER-RESIDUE gol (2e) — residue recipe. When gol is
                // disabled it is never selected (b), so zone_count stays 0 and
                // the sole writer of the zone GPU buffers (the compute block
                // above, guarded by zone_count>0) never runs. Prove it across
                // frames: report pristine, and fail LOUD if either invariant
                // is ever violated. Used at gate G3. Zero effect when enabled.
                if constexpr (!ROSTER.gol) {
                    if (time_state_.seconds - rosterGolResidueDump_ >= AGENT_CENSUS_INTERVAL) {
                        if (gol_state_.zone_count != 0 || rosterGolZoneRuns_ != 0) {
                            std::cerr << "[ROSTER residue] VIOLATION: gol disabled but zone_count="
                                << gol_state_.zone_count << " runs=" << rosterGolZoneRuns_ << "\n";
                        }
                        else {
                            std::cout << "[ROSTER residue] gol disabled: zone buffers pristine"
                                << " (zone_count=0, zone-compute runs this session=0)\n";
                        }
                        rosterGolResidueDump_ = time_state_.seconds;
                    }
                }

                if (time_state_.seconds - agent_state_.last_census_dump >= AGENT_CENSUS_INTERVAL) {
                    dump_agent_census(agent_state_, &agents_deps_, "periodic");
                    const auto& player = agent_state_.slots[0];
                    std::cout << "[Player] pos=(" << std::fixed << std::setprecision(1)
                        << player.pos_x << "," << player.pos_z
                        << ") slot=" << player_.possessed_slot
                        << " behavior=" << player.behavior_id
                        << "\n";
                    agent_state_.last_census_dump = time_state_.seconds;
                }

                // Periodic entity census dump
#ifdef DIAG_ENTITY_CENSUS
                if (time_state_.seconds - spawn_engine_state_.lastCensusDump_ >= CENSUS_DUMP_INTERVAL) {
                    dump_entity_census(&machine_ctx_, "periodic");
                    spawn_engine_state_.lastCensusDump_ = time_state_.seconds;
                }
#endif
            }

            // R7 — RIBBON TICK (music+wall-clock). One call; the conductor lives
            // in bodies/ribbon.hpp. Its tail is the sky resync — the SOLE author of
            // the sky_* block (E-3 mechanized: the drain skips those 32 bytes,
            // and initialize() boot-neutrals them for the ribbon-off case).
            // SEAM[ribbon:sky-mode]. ROSTER-GATE ribbon — guarded at the call site.
            void phase_ribbon_tick(RenderCtx& c) {
                auto& queue = c.queue;
                ribbon_frame_tick(ribbon_state_, &ribbon_deps_, queue);
            }

            // R8 — ENTITY MESH GEN (algo; dirty-driven). Twelve constexpr-gated
            // prepare lines set dirty[]; one compute pass dispatches the dirty
            // families (branches on dirty-ness, not the enable bit). The
            // per-family gates are intra-movement.
            void phase_entity_mesh_gen(RenderCtx& c) {
                auto& encoder = c.encoder;
                auto& queue = c.queue;
                bool dirty[PopFamily::COUNT] = {};
                bool anyDirty = false;
                // Twelve explicit prepare lines, one per family, each
                // presence constexpr-gated — THE SCORE RULING: the typelist
                // fold dissolved into prose. A disabled
                // family's prepare is eliminated at COMPILE TIME (no call,
                // no runtime branch); all-enabled compiles to the same 12
                // calls in the same order.
                if constexpr (ROSTER.pyramid) {   // ROSTER-GATE pyramid (b)
                    dirty[PopFamily::PYRAMID] = FAMILY_DISPATCH[PopFamily::PYRAMID].prepare_mesh(&machine_ctx_, queue);
                    anyDirty = anyDirty || dirty[PopFamily::PYRAMID];
                }
                if constexpr (ROSTER.arch) {      // ROSTER-GATE arch (b)
                    dirty[PopFamily::ARCH] = FAMILY_DISPATCH[PopFamily::ARCH].prepare_mesh(&machine_ctx_, queue);
                    anyDirty = anyDirty || dirty[PopFamily::ARCH];
                }
                if constexpr (ROSTER.column) {    // ROSTER-GATE column (b)
                    dirty[PopFamily::COLUMN] = FAMILY_DISPATCH[PopFamily::COLUMN].prepare_mesh(&machine_ctx_, queue);
                    anyDirty = anyDirty || dirty[PopFamily::COLUMN];
                }
                if constexpr (ROSTER.antenna) {   // ROSTER-GATE antenna (b)
                    dirty[PopFamily::ANTENNA] = FAMILY_DISPATCH[PopFamily::ANTENNA].prepare_mesh(&machine_ctx_, queue);
                    anyDirty = anyDirty || dirty[PopFamily::ANTENNA];
                }
                if constexpr (ROSTER.palm) {      // ROSTER-GATE palm (b)
                    dirty[PopFamily::PALM] = FAMILY_DISPATCH[PopFamily::PALM].prepare_mesh(&machine_ctx_, queue);
                    anyDirty = anyDirty || dirty[PopFamily::PALM];
                }
                if constexpr (ROSTER.cactus) {    // ROSTER-GATE cactus (b)
                    dirty[PopFamily::CACTUS] = FAMILY_DISPATCH[PopFamily::CACTUS].prepare_mesh(&machine_ctx_, queue);
                    anyDirty = anyDirty || dirty[PopFamily::CACTUS];
                }
                if constexpr (ROSTER.blade) {     // ROSTER-GATE blade (b)
                    dirty[PopFamily::BLADE] = FAMILY_DISPATCH[PopFamily::BLADE].prepare_mesh(&machine_ctx_, queue);
                    anyDirty = anyDirty || dirty[PopFamily::BLADE];
                }
                if constexpr (ROSTER.sphere) {    // ROSTER-GATE sphere (b)
                    dirty[PopFamily::SPHERE] = FAMILY_DISPATCH[PopFamily::SPHERE].prepare_mesh(&machine_ctx_, queue);
                    anyDirty = anyDirty || dirty[PopFamily::SPHERE];
                }
                if constexpr (ROSTER.ribbon) {    // ROSTER-GATE ribbon (b)
                    dirty[PopFamily::RIBBON] = FAMILY_DISPATCH[PopFamily::RIBBON].prepare_mesh(&machine_ctx_, queue);
                    anyDirty = anyDirty || dirty[PopFamily::RIBBON];
                }
                if constexpr (ROSTER.cube) {      // ROSTER-GATE cube (b)
                    dirty[PopFamily::CUBE] = FAMILY_DISPATCH[PopFamily::CUBE].prepare_mesh(&machine_ctx_, queue);
                    anyDirty = anyDirty || dirty[PopFamily::CUBE];
                }
                if constexpr (ROSTER.gol) {       // ROSTER-GATE gol (b)
                    dirty[PopFamily::GOL] = FAMILY_DISPATCH[PopFamily::GOL].prepare_mesh(&machine_ctx_, queue);
                    anyDirty = anyDirty || dirty[PopFamily::GOL];
                }
                if constexpr (ROSTER.gallery) {   // ROSTER-GATE gallery (b)
                    dirty[PopFamily::GALLERY] = FAMILY_DISPATCH[PopFamily::GALLERY].prepare_mesh(&machine_ctx_, queue);
                    anyDirty = anyDirty || dirty[PopFamily::GALLERY];
                }
                if (anyDirty) {
                    wgpu::ComputePassDescriptor cpd{};
                    cpd.label = "Entity Mesh Gen";
                    wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&cpd);
                    // dispatch skips disabled families structurally:
                    // dirty[f] stays false for a disabled family (never
                    // set above), so this branches on dirty-ness, not on
                    // the enable bit.
                    for (uint32_t f = 0; f < PopFamily::COUNT; f++) {
                        if (dirty[f]) FAMILY_DISPATCH[f].dispatch_mesh(&machine_ctx_, pass);
                    }
                    pass.End();
                }
            }

            // R9 — PORTAL + LIGHTS UPLOAD (algo).
            void phase_upload_portal_lights(RenderCtx& c) {
                auto& queue = c.queue;
                upload_portal_array(&mood_deps_, queue);
                upload_lights(&mood_deps_, queue);
            }

            // LIVE CARD WRITE (GROUND_CARD_1; between R9 and R10). The per-frame
            // deformation field: the writer calls the existing evaluators at
            // texel centers; every consumer then samples one card. Before
            // DispatchCompute (the consumers) and before PlacementCorrection
            // (reads .a at H5).
            void phase_live_card_write(RenderCtx& c) {
                auto& encoder = c.encoder;
                dispatch_live_card_write(&machine_ctx_, encoder);
            }

            // R10 — DISPATCH COMPUTE (music+input+algo). The per-frame world-
            // update compute pass (7 dispatches; render_passes.hpp). O-1 by
            // construction: R7's resync writes before this reads (submission
            // order).
            void phase_dispatch_compute(RenderCtx& c) {
                auto& encoder = c.encoder;
                dispatch_compute(&machine_ctx_, encoder);
            }

            // R11 — WITNESS CAPTURE (O-2: staging copies AFTER compute; feeds
            // next frame's HARVEST). The camera copy is CAMERA-HOST ONLY (the
            // pawn-host frame encodes no camera copy; that path stays
            // byte-untouched).
            void phase_witness_capture(RenderCtx& c) {
                auto& encoder = c.encoder;
                // Copy full agent buffer from GPU to staging (for readback next frame)
                if (pawnReadbackState_ == PawnReadbackState::IDLE) {
                    encoder.CopyBufferToBuffer(
                        gpuState_.agent_state_buffer(), 0,
                        gpuState_.agent_state_readback_staging(), 0,
                        GPUState::agent_state_buffer_size());
                    pawnReadbackState_ = PawnReadbackState::COPIED;
                }

                if (floaterReadbackState_ == FloaterReadbackState::IDLE) {
                    encoder.CopyBufferToBuffer(
                        gpuState_.floating_entity_buffer(), 0,
                        gpuState_.floating_entity_readback_staging(), 0,
                        GPUState::floating_entity_buffer_size());
                    floaterReadbackState_ = FloaterReadbackState::COPIED;
                }

                // The point readback copy: CAMERA-HOST ONLY — the
                // pawn-host frame encodes no camera copy (option A; the
                // pawn path stays byte-untouched).
                if (point_.host == PointHost::CAMERA &&
                    cameraReadbackState_ == CameraReadbackState::IDLE) {
                    encoder.CopyBufferToBuffer(
                        gpuState_.camera_state_buffer(), 0,
                        gpuState_.camera_state_readback_staging(), 0,
                        GPUState::camera_state_buffer_size());
                    cameraReadbackState_ = CameraReadbackState::COPIED;
                }
            }

            // R12a — GOL DERIVE FLUSH (algo). THE HIDDEN 2nd SUBMIT
            // (gol_zones.hpp): its own encoder + Submit, before the host submit
            // (recon E-8). Its own named phase per the spine ruling. Guarded at
            // the call site (ROSTER.gol + zone_count>0).
            void phase_gol_derive_flush(RenderCtx& c) {
                if (gol_state_.zone_count == 0) return;  // runtime data-guard, moved inside (was the call-site `if (zone_count>0)`)
                rosterGolZoneRuns_++;  // ROSTER-RESIDUE gol (2e) — sole writer marker of the zone GPU buffers; the residue check proves pristine when disabled
                flush_zone_derive_requests(gol_state_, &gol_deps_, c.queue);
            }

            // R12b — GOL ZONE COMPUTE (algo). Config upload + sync/evolve/mesh
            // in SEPARATE passes (O-6a barrier by pass boundary). Guarded at the
            // call site with the derive flush.
            void phase_gol_zone_compute(RenderCtx& c) {
                if (gol_state_.zone_count == 0) return;  // shares R12a's guard; flush_zone_derive_requests does NOT touch zone_count (verified), so the two independent checks are equivalent to the original single guard
                auto& encoder = c.encoder;
                auto& queue = c.queue;
                upload_gol_zone_config(gol_state_, &gol_deps_, queue);
                dispatch_zone_sync(gol_state_, &gol_deps_, encoder);
                dispatch_zone_evolve(gol_state_, &gol_deps_, encoder);
            }

            // R13 — PAWN AURA (wall-clock). Persistent terrain influence; the
            // runtime presence/clearing condition lives inside. ROSTER-GATE
            // pawn_aura — guarded at the call site.
            void phase_pawn_aura(RenderCtx& c) {
                auto& encoder = c.encoder;
                auto& queue = c.queue;
                dispatch_pawn_aura(pawn_state_, &pawn_deps_, encoder, queue);
            }

            // R14 — ORB SKY (algo+music). One-shot init, optional recolor,
            // snapshot-prev for flocking, advance dynamics. ROSTER-GATE orbs —
            // guarded at the call site.
            void phase_orb_sky(RenderCtx& c) {
                auto& encoder = c.encoder;
                auto& queue = c.queue;
                dispatch_orb_init(orbs_state_, &orbs_deps_, encoder);
                dispatch_orb_recolor(orbs_state_, &orbs_deps_, encoder);
                dispatch_orb_copy_prev(orbs_state_, &orbs_deps_, encoder);
                dispatch_orb_dynamics(orbs_state_, &orbs_deps_, encoder, queue);
            }

            // R15 — GROUND ENTRIES (algo; dirty-driven). On ground_entries_dirty:
            // stage per-family ground origins and raise placement_dirty (the E-6
            // same-frame cascade into R16). Runtime guard inside.
            void phase_ground_entries(RenderCtx& c) {
                auto& queue = c.queue;
                if (world_state_.ground_entries_dirty) {
                    world_state_.ground_entries_dirty = false;
                    world_state_.placement_dirty = true;
                    upload_ground_entries(&machine_ctx_, queue);
                }
            }

            // R16 — PLACEMENT CORRECTION (algo; dirty-driven). On placement_dirty:
            // the entity Y-correction compute. Runtime guard inside.
            void phase_placement_correction(RenderCtx& c) {
                auto& encoder = c.encoder;
                if (world_state_.placement_dirty) {
                    world_state_.placement_dirty = false;
                    dispatch_placement_correction(&machine_ctx_, encoder);
                    // THE RE-RAISE (COLUMN CEILING FIT): this frame's
                    // correction rewrites column ground_y; the cmg kernel
                    // bakes ceiling-relative height from it, so a corrected
                    // ground demands ONE rebake — bake raw at N, correct at
                    // N's R16, rebake true at N+1's R8. Gated on an active
                    // column (the population the fit touches; antennas are
                    // tier-gated out in the kernel) and on the SAME dirty
                    // consumption that dispatched the correction — NEVER
                    // unconditional: a perpetual rebake is the failure mode
                    // (idle rig = zero mesh-gen dispatches at steady state).
                    for (uint32_t i = 0; i < Dim::MAX_COLUMN_ONLY; i++) {
                        if (entities_state_.columns[i].active) {
                            entities_state_.column_mesh_gen_pending = true;
                            break;
                        }
                    }
                }
            }

            // R17 — FRUSTUM CULL (algo; O-7 tail). Cull before the draw passes —
            // the indirect draws consume the cull output (recon E-5).
            void phase_frustum_cull(RenderCtx& c) {
                auto& encoder = c.encoder;
                auto& queue = c.queue;
                dispatch_frustum_cull(&machine_ctx_, encoder, queue);
            }

            // R18 — SHADOW PASS. draw_shadow_all into the shadow map(s).
            void phase_shadow_pass(RenderCtx& c) {
                auto& encoder = c.encoder;
                render_shadow_pass(&machine_ctx_, encoder, cpuSpotLights_);
            }

            // R19 — MAIN PASS. The rasterized scene into the backbuffer.
            void phase_main_pass(RenderCtx& c) {
                auto& encoder = c.encoder;
                auto backbuffer = c.backbuffer;
                auto depth = c.depth;
                render_main_pass(&machine_ctx_, encoder, backbuffer, depth, clearColor_, orbs_state_, orbs_deps_);
            }

            // R20 — SNAPSHOT PASS (algo; gallery cadence). The photographer's
            // third draw list.
            void phase_snapshot_pass(RenderCtx& c) {
                auto& encoder = c.encoder;
                render_snapshot_pass(gallery_state_, &gallery_deps_, encoder);
            }

            // R21 — PROMOTION DRAIN (algo). ROSTER-GATE gallery+indoor_shell —
            // guarded at the call site.
            void phase_promotion_drain(RenderCtx& c) {
                auto& encoder = c.encoder;
                drain_gallery_promotions(gallery_state_, &gallery_deps_, encoder);
            }

            // ═══════════════════════════════════════════════════════════════
            // THE SPINE TABLES — the AUTHORED order (row order == frame order).
            // Row = {phase id, name, member fn, driver(§9), roster gate, face}.
            // A gate is a constexpr-folded bool (ROSTER bit or `true` for
            // foundational spine work). The census (audit/tools/score) audits
            // THESE ROWS: manifest = the table, attribution = row membership.
            // ═══════════════════════════════════════════════════════════════
            static constexpr URow UPDATE_SPINE[] = {
                { UPhase::FillSignal,          "fill_signal",           &Cartridge::phase_fill_signal,           Driver::Mixed,     true,             F_SIGNAL | F_CLOCK },
                { UPhase::AdvanceClock,        "advance_clock",         &Cartridge::phase_advance_clock,         Driver::Music,     true,             F_CLOCK },
                { UPhase::MotionDrivers,       "motion_drivers",        &Cartridge::phase_motion_drivers,        Driver::Music,     true,             F_CONFIG },
                { UPhase::MotionBodies,        "motion_bodies",         &Cartridge::phase_motion_bodies,         Driver::WallClock, ROSTER.pawn_aura, F_NONE },
                { UPhase::StageWorld,          "stage_world",           &Cartridge::phase_stage_world,           Driver::Algo,      true,             F_CONFIG },
                { UPhase::TransitionMachine,   "transition_machine",    &Cartridge::phase_transition_machine,    Driver::Mixed,     true,             F_CONFIG | F_TRANSITION },
                { UPhase::StageFadeUpload,     "stage_fade_and_upload", &Cartridge::phase_stage_fade_and_upload, Driver::None,      true,             F_SIGNAL | F_CONFIG },
                { UPhase::WitnessPhotographer, "witness_photographer",  &Cartridge::phase_witness_photographer,  Driver::Algo,      ROSTER.gallery,   F_WITNESS },
                { UPhase::ClearInputDeltas,    "clear_input_deltas",    &Cartridge::phase_clear_input_deltas,    Driver::None,      true,             F_NONE },
            };
            static constexpr RRow RENDER_SPINE[] = {
                { RPhase::WitnessHarvest,      "witness_harvest",       &Cartridge::phase_witness_harvest,       Driver::Algo,      true,                                   F_WITNESS },
                { RPhase::PortalTrigger,       "portal_trigger",        &Cartridge::phase_portal_trigger,        Driver::Algo,      ROSTER.transitions,                     F_WITNESS | F_TRANSITION },
                { RPhase::StreamPatches,       "stream_patches",        &Cartridge::phase_stream_patches,        Driver::Algo,      true,                                   F_STREAM | F_COMPUTE },
                { RPhase::RespawnAgents,       "respawn_agents",        &Cartridge::phase_respawn_agents,        Driver::Algo,      ROSTER.wanderers,                       F_NONE },
                { RPhase::MotionCorral,        "motion_corral",         &Cartridge::phase_motion_corral,         Driver::WallClock, ROSTER.cube,                            F_NONE },
                { RPhase::CensusDumps,         "census_dumps",          &Cartridge::phase_census_dumps,          Driver::WallClock, true,                                   F_NONE },
                { RPhase::RibbonTick,          "ribbon_tick",           &Cartridge::phase_ribbon_tick,           Driver::Mixed,     ROSTER.ribbon,                          F_SIGNAL },
                { RPhase::EntityMeshGen,       "entity_mesh_gen",       &Cartridge::phase_entity_mesh_gen,       Driver::Algo,      true,                                   F_COMPUTE },
                { RPhase::UploadPortalLights,  "upload_portal_lights",  &Cartridge::phase_upload_portal_lights,  Driver::Algo,      true,                                   F_CONFIG },
                { RPhase::LiveCardWrite,       "live_card_write",       &Cartridge::phase_live_card_write,       Driver::Mixed,     true,                                   F_COMPUTE },
                { RPhase::DispatchCompute,     "dispatch_compute",      &Cartridge::phase_dispatch_compute,      Driver::Mixed,     true,                                   F_COMPUTE },
                { RPhase::WitnessCapture,      "witness_capture",       &Cartridge::phase_witness_capture,       Driver::None,      true,                                   F_WITNESS },
                { RPhase::GolDeriveFlush,      "gol_derive_flush",      &Cartridge::phase_gol_derive_flush,      Driver::Algo,      ROSTER.gol,                             F_COMPUTE | F_SUBMIT },
                { RPhase::GolZoneCompute,      "gol_zone_compute",      &Cartridge::phase_gol_zone_compute,      Driver::Algo,      ROSTER.gol,                             F_COMPUTE },
                { RPhase::PawnAura,            "pawn_aura",             &Cartridge::phase_pawn_aura,             Driver::WallClock, ROSTER.pawn_aura,                       F_COMPUTE },
                { RPhase::OrbSky,              "orb_sky",               &Cartridge::phase_orb_sky,               Driver::Mixed,     ROSTER.orbs,                            F_COMPUTE },
                { RPhase::GroundEntries,       "ground_entries",        &Cartridge::phase_ground_entries,        Driver::Algo,      true,                                   F_GROUND },
                { RPhase::PlacementCorrection, "placement_correction",  &Cartridge::phase_placement_correction,  Driver::Algo,      true,                                   F_GROUND | F_COMPUTE },
                { RPhase::FrustumCull,         "frustum_cull",          &Cartridge::phase_frustum_cull,          Driver::Algo,      true,                                   F_COMPUTE },
                { RPhase::ShadowPass,          "shadow_pass",           &Cartridge::phase_shadow_pass,           Driver::None,      true,                                   F_DRAW },
                { RPhase::MainPass,            "main_pass",             &Cartridge::phase_main_pass,             Driver::None,      true,                                   F_DRAW },
                { RPhase::SnapshotPass,        "snapshot_pass",         &Cartridge::phase_snapshot_pass,         Driver::Algo,      true,                                   F_DRAW },
                { RPhase::PromotionDrain,      "promotion_drain",       &Cartridge::phase_promotion_drain,       Driver::Algo,      (ROSTER.gallery || ROSTER.indoor_shell), F_NONE },
            };

            // ═══ SPINE VALIDATION ═══════════════════════
            // Every CROSS-PHASE O-# / RC law the recon named is a static_assert
            // over the row indices: the frame CANNOT be authored out of its
            // lawful order (a reorder fails the BUILD, not the pixel rig). The
            // by-design lags (E-3/E-4/E-9) are declared as law lines in the
            // spine header above. Two laws are INTRA-phase, not row-index laws,
            // enforced by structure inside a single phase: O-3 (the TEARDOWN
            // fixed sequence, inside phase_transition_machine) and O-6a (the
            // zone sync->evolve->mesh barrier = the three SEPARATE compute
            // passes inside phase_gol_zone_compute).
            static constexpr bool spine_ordered_u() {
                for (std::size_t i = 0; i < (std::size_t)UPhase::COUNT; i++)
                    if ((std::size_t)UPDATE_SPINE[i].id != i) return false;
                return true;
            }
            static constexpr bool spine_ordered_r() {
                for (std::size_t i = 0; i < (std::size_t)RPhase::COUNT; i++)
                    if ((std::size_t)RENDER_SPINE[i].id != i) return false;
                return true;
            }
            // O-5b/c (face-based): no SIGNAL/CONFIG staging phase may follow the
            // drain (StageFadeUpload) — a future staging phase placed after it
            // fails to build, not silently drops for the frame.
            static constexpr bool no_staging_after_drain() {
                for (std::size_t i = 0; i < (std::size_t)UPhase::COUNT; i++)
                    if (UPDATE_SPINE[i].id != UPhase::StageFadeUpload &&
                        (UPDATE_SPINE[i].face & (F_SIGNAL | F_CONFIG)) &&
                        i > (std::size_t)UPhase::StageFadeUpload) return false;
                return true;
            }

            static_assert(sizeof(UPDATE_SPINE) / sizeof(URow) == (std::size_t)UPhase::COUNT, "update spine must be dense");
            static_assert(sizeof(RENDER_SPINE) / sizeof(RRow) == (std::size_t)RPhase::COUNT, "render spine must be dense");
            // Table-order integrity + the O-5b/c face law are BOOT asserts
            // (validate_spine, called once at init): a constexpr member fn
            // cannot be static_asserted inside its own incomplete class.
            // update laws:
            static_assert((uint32_t)UPhase::FillSignal < (uint32_t)UPhase::AdvanceClock, "O-5a: dt_beats reads prev_beats before the clock advances it");
            // E-3 (sky write-order) is now MECHANIZED, not an ordering assert:
            // update writes the sky block NOWHERE (upload_signal skips it), so its
            // sole author is resync_sky_head (R7). Structure replaced the paragraph.
            static_assert((uint32_t)UPhase::ClearInputDeltas + 1 == (uint32_t)UPhase::COUNT, "O-5e: clear_input_deltas is dead-last");
            // render laws:
            static_assert((uint32_t)RPhase::RibbonTick < (uint32_t)RPhase::DispatchCompute, "O-1: the sky resync (R7 tail) precedes the compute that reads it");
            static_assert((uint32_t)RPhase::WitnessHarvest < (uint32_t)RPhase::DispatchCompute, "O-2: witness harvest before compute");
            static_assert((uint32_t)RPhase::DispatchCompute < (uint32_t)RPhase::WitnessCapture, "O-2: witness capture after compute (feeds next frame's harvest)");
            static_assert((uint32_t)RPhase::StreamPatches < (uint32_t)RPhase::RespawnAgents, "RC-1: respawn after the stream (S3 after S2)");
            static_assert((uint32_t)RPhase::StreamPatches < (uint32_t)RPhase::MotionCorral, "RC-2: corral after the stream (S4 after S2)");
            static_assert((uint32_t)RPhase::GroundEntries < (uint32_t)RPhase::PlacementCorrection, "O-4: ground entries (raises placement_dirty) before placement correction");
            static_assert((uint32_t)RPhase::FrustumCull < (uint32_t)RPhase::ShadowPass, "O-7: frustum cull before the shadow pass");
            static_assert((uint32_t)RPhase::FrustumCull < (uint32_t)RPhase::MainPass, "O-7: frustum cull before the main pass (indirect draws consume the cull)");
            static_assert((uint32_t)RPhase::LiveCardWrite > (uint32_t)RPhase::UploadPortalLights &&
                          (uint32_t)RPhase::LiveCardWrite < (uint32_t)RPhase::DispatchCompute,
                "GROUND_CARD_1: the card writes before the consumers "
                "(pre-evolve zone read preserved, R10<R13 order intact)");
            static_assert((uint32_t)RPhase::LiveCardWrite < (uint32_t)RPhase::PlacementCorrection,
                "GROUND_CARD_1: the card writes before placement reads .a");
            static_assert((uint32_t)RPhase::GolDeriveFlush < (uint32_t)RPhase::GolZoneCompute, "gol: the derive flush (hidden submit) precedes the zone compute that reads it");
            static_assert((uint32_t)RPhase::ShadowPass < (uint32_t)RPhase::MainPass, "draw: shadow before main");
            static_assert((uint32_t)RPhase::MainPass < (uint32_t)RPhase::SnapshotPass, "draw: main before snapshot");

            // BOOT VALIDATION (always-on): table-order integrity + the O-5b/c
            // face law — the checks a constexpr member fn cannot static_assert
            // inside its own incomplete class. Fails LOUD at boot, never silent.
            void validate_spine() const {
                if (!spine_ordered_u() || !spine_ordered_r() || !no_staging_after_drain()) {
                    std::cerr << "[SPINE] VALIDATION FAILED — row order / O-5b/c face law violated\n";
                    std::abort();
                }
                // F-2: FAMILY_DISPATCH rows are
                // POSITIONAL in PopFamily order and each carries its name —
                // check every row's name against the canonical
                // family_short_name list, so a row swap fails LOUD at boot,
                // never silent (the table is inline const, not constexpr, so
                // this cannot be a static_assert).
                for (uint32_t f = 0; f < PopFamily::COUNT; f++) {
                    const char* have = FAMILY_DISPATCH[f].name;
                    const char* want = family_short_name(f);
                    bool eq = (have != nullptr);
                    for (uint32_t i = 0; eq; i++) {
                        if (have[i] != want[i]) { eq = false; break; }
                        if (have[i] == '\0') break;
                    }
                    if (!eq) {
                        std::cerr << "[SPINE] FAMILY_DISPATCH row " << f << " name '"
                            << (have ? have : "<null>") << "' != PopFamily order '"
                            << want << "' (F-2)\n";
                        std::abort();
                    }
                }
                std::cout << "[SPINE] validated: " << (uint32_t)UPhase::COUNT << " update rows + "
                    << (uint32_t)RPhase::COUNT << " render rows + "
                    << (uint32_t)PopFamily::COUNT << " dispatch rows name-checked; "
                    << "O-#/RC laws static-asserted\n";
            }

            // ── THE CONDUCTOR (render) — a LOOP over RENDER_SPINE (§1b) ─────
            void render(wgpu::CommandEncoder& encoder,
                wgpu::TextureView backbuffer,
                wgpu::TextureView depth) override {
                wgpu::Queue queue = device_.GetQueue();
                RenderCtx ctx{ encoder, queue, backbuffer, depth };
                for (const RRow& row : RENDER_SPINE)
                    if (row.enabled) (this->*row.fn)(ctx);
            }

            // Mood is VOCABULARY + APPLIERS + SIX DOORS: CeilingType /
            // MoodProfile / MOOD_TABLE / portal colors / indoor palettes +
            // the door, applier, and deriver declarations are in mood.hpp
            // (file scope, above the class); the definitions live in the
            // same header's MODULE IMPLEMENTATION zone (the merged file,
            // pre-class in the cohort). MOOD OWNS NO STATE —
            // nothing at the COMPOSITION ROOT; mood_state_ and the
            // transition machine are spine-resident
            // (SEAM[spine:transitions], constitution §2). The force-spawn
            // mutation belongs to the arch's owner: entities'
            // force_spawn_portal_arch (the ROSTER portal door lives
            // there). The lighting-scheme tables stay impl-side. See §1.

        public:

            void on_input(const InputEvent& event) override {
                switch (event.type) {
                case InputEvent::Type::KeyDown:
                    // The command fan's TARGET organs ride the call site:
                    // the root addresses the fan's
                    // bodies per event, through the owner doors — the driver
                    // owns none of them (v3 §9 Act I). The F6 socket stays
                    // RESERVED for a real addressing need.
                    on_key_down(&input_deps_, event.key,
                        pawn_state_, pawn_deps_,
                        orbs_state_, orbs_deps_,
                        agent_state_, agents_deps_,
                        cube_behaviors_state_, cube_deps_,
                        transitionPhase_, pendingDestination_, mood_state_);
                    break;
                case InputEvent::Type::KeyUp:
                    on_key_up(&input_deps_, event.key);
                    break;
                case InputEvent::Type::MouseMove:
                    on_mouse_move(&input_deps_, event.x, event.y);
                    break;
                case InputEvent::Type::MouseButton:
                    on_mouse_button(&input_deps_, event.button, event.pressed);
                    break;
                case InputEvent::Type::Scroll:
                    on_scroll(&input_deps_, event.y);
                    break;
                }
            }

            void get_clear_color(float& r, float& g, float& b) const override {
                r = clearColor_[0];
                g = clearColor_[1];
                b = clearColor_[2];
            }

            wgpu::TextureFormat depth_format() const override {
                return wgpu::TextureFormat::Depth24Plus;
            }

            bool supports_backspace() const override {
                return true;
            }

            bool reload_shaders() override { return renderer_.reload(); }
            const std::string& shader_path() const { return renderer_.shader_path(); }

        };

    } // namespace the_board
} // namespace t7

// ═══ THE POST-CLASS ZONE — EMPTY OF MODULES ═══════════════════════════
//
// Every module rides ONE pre-class header, its decl tier at the
// contracts (history: audit/LADDER.md). What remains below is the
// spine's own table — FAMILY_DISPATCH — which was never a module.
// ═══ THE TABLE — FAMILY_DISPATCH ═══════════════════════════════════
// The definition
// is SEAM[spine:owns] spine work — it takes the Cartridge mesh-wrapper
// static ADDRESSES and the family row addresses, so it lives with its
// owner, the composition root, at the post-class point.
namespace t7 {
    namespace the_board {
        // ─── Shared no-op adapters ────────────────────────────────────────

        inline bool dispatch_prepare_mesh_none(MachineCtx* self, wgpu::Queue& queue) {
            (void)self; (void)queue;
            return false;
        }
        inline void dispatch_mesh_gen_none(MachineCtx* self, wgpu::ComputePassEncoder& pass) {
            (void)self; (void)pass;
        }

        // ─── The table ─────────────────────────────────────────────────────
        // AXES: one row per family, POSITIONAL in PopFamily order (PYRAMID=0,
        //   ARCH, COLUMN, ANTENNA, PALM, CACTUS, BLADE, SPHERE, RIBBON, CUBE,
        //   GOL, GALLERY=11) — the enum values are pinned at roster.hpp (F-1)
        //   and every row's trailing name string is boot-checked against
        //   family_short_name by validate_spine (F-2), so a row swap fails
        //   LOUD. Row columns (FamilyDispatch, entity_types.hpp):
        //     { try_select, try_place, try_commit, evict_slot,
        //       prepare_mesh, dispatch_mesh, name }
        // CONSUMERS: the machine tail walks select/place/commit per queue
        //   entry; eviction routes through evict_slot; the mesh pair feeds the
        //   RENDER_UPDATE mesh phases (none-fork = family has no mesh).
        inline const FamilyDispatch FAMILY_DISPATCH[PopFamily::COUNT] = {
            { dispatch_select_pyramid_generic, dispatch_place_pyramid_generic, dispatch_commit_pyramid_generic,
              evict_pyramid, dispatch_prepare_mesh_none, dispatch_mesh_gen_none,   // mesh hook → none-fork: pyramid mesh dead-by-design; placement feeds the heightfield
              "pyr" },
            { dispatch_select_arch_generic, dispatch_place_arch_generic, dispatch_commit_arch_generic,
              evict_arch,    Cartridge::dispatch_prepare_mesh_arch,    Cartridge::dispatch_mesh_gen_arch,
              "arch" },
            { dispatch_select_column_generic, dispatch_place_column_generic, dispatch_commit_column_generic,
              evict_column,  Cartridge::dispatch_prepare_mesh_column,  Cartridge::dispatch_mesh_gen_column,
              "col" },
            { dispatch_select_antenna_generic, dispatch_place_antenna_generic, dispatch_commit_antenna_generic,
              evict_antenna, Cartridge::dispatch_prepare_mesh_column,  Cartridge::dispatch_mesh_gen_column,
              "ant" },
            { dispatch_select_palm_generic, dispatch_place_palm_generic, dispatch_commit_palm_generic,
              evict_palm,   Cartridge::dispatch_prepare_mesh_palm,   Cartridge::dispatch_mesh_gen_palm,
              "palm" },
            { dispatch_select_cactus_generic, dispatch_place_cactus_generic, dispatch_commit_cactus_generic,
              evict_cactus, Cartridge::dispatch_prepare_mesh_cactus, Cartridge::dispatch_mesh_gen_cactus,
              "cact" },
            { dispatch_select_blade_generic, dispatch_place_blade_generic, dispatch_commit_blade_generic,
              evict_blade, Cartridge::dispatch_prepare_mesh_blade, Cartridge::dispatch_mesh_gen_blade,
              "blad" },
            { dispatch_select_sphere_generic, dispatch_place_sphere_generic, dispatch_commit_sphere_generic,
              evict_sphere, dispatch_prepare_mesh_none, dispatch_mesh_gen_none,
              "sph" },   // no CPU mesh gen — GPU compute handles update_sphere
            { dispatch_select_ribbon, dispatch_place_ribbon, dispatch_commit_ribbon,
              evict_ribbon, dispatch_prepare_mesh_none, dispatch_mesh_gen_none,
              "ribn" },  // no CPU mesh gen — GPU compute handles ribbon rendering
            { dispatch_select_cube_generic, dispatch_place_cube_generic, dispatch_commit_cube_generic,
              evict_cube, dispatch_prepare_mesh_none, dispatch_mesh_gen_none,
              "cube" },  // no CPU mesh gen — GPU compute handles update_cube
            { dispatch_select_gol, dispatch_place_gol, dispatch_commit_gol,
              evict_gol, dispatch_prepare_mesh_none, dispatch_mesh_gen_none,
              "gol" },   // zone mesh gen is a separate compute pass
            { dispatch_select_gallery, dispatch_place_gallery, dispatch_commit_gallery,
              evict_gallery, dispatch_prepare_mesh_none, dispatch_mesh_gen_none,
              "gall" },
        };
    } // namespace the_board
} // namespace t7