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
//   mood_state_.transition_timer, pendingDestination_, the back-portal
//   pending state, and the
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
#include "core/instruments.hpp"                                    // THE INSTRUMENTS DIAL: INSTRUMENTS.frame_meter / .periodic_census gate the recurring self-measurement (compile-time, T7_INSTRUMENTS; default off)
#include "cartridges/the_board/contracts/roster.hpp"
#include "cartridges/the_board/demos/demo.hpp"             // THE SELECTED SENTENCE: DEMO + ROSTER (compile-time, INCUBATE_DEMO; default full)
#include "cartridges/the_board/primitives/seed_utils.hpp"           // hash/gaussian/tier-select helpers (pure-math leaf)
#include "cartridges/the_board/contracts/ground_architecture.hpp"  // ground contributor/policy tables + compile-time DAG checks
#include "cartridges/the_board/contracts/entity_types.hpp"         // THE CONTRACT HOME: pipeline contracts + boundary DTOs + queue unions + dispatch row/table decl
#include "cartridges/the_board/contracts/indoor_module.hpp"        // THE INDOOR MODULE: mood's insert on the spawn chain — one policy table + three dials; consumers ride the cohort (grounded/floaters/ribbon/the machine)
#include "cartridges/the_board/contracts/spawn_services.hpp"      // THE MACHINE'S DECL TIER: spawn/pipeline service decls + boundary DTOs + arch vocabulary + MIN_SEPARATION (bodies ride the merged machine headers at the cohort tail)
#include "cartridges/the_board/contracts/mood_constants.hpp"       // MOOD_COUNT + the Mood IDs + PortalDestination
#include "cartridges/the_board/contracts/spine_state.hpp"          // TimeState + PlayerState + TransitionPhase + InputState + MoodState/MoodProfile/MOOD_TABLE + the request door decl (spine organ TYPES; instances stay at the root)
#include "cartridges/the_board/contracts/point.hpp"                // THE POINT: the parent of the player system — host enum + the bubble decl; instance at the root
#include "cartridges/the_board/contracts/control_panel.hpp"        // THE PANEL: the field's dials + the beacon rests — one home, every room
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
#include "cartridges/the_board/bodies/ribbon.hpp"               // RibbonState + RibbonDeps + impl — MERGED; after visual_canvas for the coupling face; after agents/cubes/spheres for the FIELD_2 mirror deps
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
            CameraControls camera_;   // the panel: look_sensitivity is live (KP_+/KP_-)

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

            // Sun + atmosphere — authored solely by apply_mood, at boot and at
            // every transition. No boot literals: MOOD_TABLE is the one source.
            float sunDirection_[3] = {};
            float sunColor_[3] = {};
            float clearColor_[3] = {};

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
            //   vocabulary + appliers + seven doors and owns NO instance;
            //   struct MoodState's TYPE lives in contracts/spine_state.hpp.
            //   Constitution §2 carries the K4 line.
            // SEAM[spine:portal-system] consumed by the mood module
            //   (force_spawn_* functions read pendingDestination_), direction/input.hpp
            //   (keypress mood transitions request via mood.hpp's
            //   request_mood_transition), render() (readback callback drives
            //   portal trigger detection). PORTAL_COLORS lives in
            //   contracts/mood_constants.hpp, beside PortalDestination —
            //   a portal's colour is a fact about its destination, so the
            //   palette sits with the type that names it (PORTAL_1 C5) and
            //   every channel derives. The machine keeps the pending state
            //   and the trigger hooks.

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

            // OPT_1a: true while the live card holds a clean rest field
            // (skip the writer); boots false so the first frame writes.
            bool liveCardRestClean_ = false;
            enum class FloaterReadbackState { IDLE, COPIED, MAPPING };
            FloaterReadbackState floaterReadbackState_ = FloaterReadbackState::IDLE;
            // The point readback (option A): runs ONLY in
            // camera-host — the camera's GPU position IS the point's, so
            // it must reach the CPU for the viewpoint set (streaming,
            // LOD, cull, orb). Pawn-host never arms this machine.
            // THE FRAME METER's timestamp readback rides the same P5
            // grammar (skip-if-busy: at most one in flight; unsampled
            // frames still write timestamps, they just aren't resolved).
            // Timing is world-agnostic — no world_gen capture needed.
            enum class MeterReadbackState { IDLE, COPIED, MAPPING };
            MeterReadbackState meterReadbackState_ = MeterReadbackState::IDLE;
            bool meter_gpu_ = false;   // device carries timestamp-query (set at initialize)

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
                                time_state_, player_, point_, gpuState_, renderer_ }
                , tile_world_deps_{ world_state_, mood_state_, gpuState_ }
                , sphere_deps_{ time_state_ }
                , pawn_deps_{ player_, time_state_, gpuState_, renderer_ }
                , orbs_deps_{ gpuState_, renderer_, player_, time_state_, world_state_ }
                , agents_deps_{ gpuState_, player_, point_, transitionPhase_, world_state_, time_state_ }
                , cube_deps_{ gpuState_, time_state_, player_, point_, mood_state_ }
                , gol_deps_{ gpuState_, renderer_, device_, time_state_ }
                , ribbon_deps_{ gpuState_, time_state_, tile_world_state_, player_, point_, inputState_, world_state_, mood_state_, visual_canvas_, ribbon_amp_lat_dst_, ribbon_amp_vert_dst_, ribbon_tint_stim_dst_, ribbon_tint_mix_dst_ }
                , gallery_deps_{ gpuState_, renderer_, world_state_, tile_world_state_, ribbon_state_, player_, point_, mood_state_, sunDirection_, clearColor_ }
                , input_deps_{ inputState_, keys_, mouse_, player_, world_state_, ribbon_state_, gpuState_, device_, point_, camera_ }
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

                // THE FRAME METER — GPU half arms only on timestamp-query
                // (the console requests it when the adapter has it).
                // Absent → degrade loudly; CPU rows are unaffected.
                // Behind the instruments dial: off, the probe never runs and
                // meter_gpu_ stays false, so the harvest/resolve block folds
                // out with it (core/instruments.hpp).
                if constexpr (INSTRUMENTS.frame_meter) {
                    meter_gpu_ = device_.HasFeature(wgpu::FeatureName::TimestampQuery);
                    if (!meter_gpu_)
                        std::cout << "[METER] timestamp-query unavailable on this adapter — CPU rows only\n";
                }

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
                    // Tilt lag rest = the pawn's response (CLOSURE_PAWN [6]).
                    // Matches zero-init; stated here so the rest lives with the
                    // other rest pins rather than in the struct. U1 re-authors
                    // it from the possessed figure every frame.
                    gpuState_.set_pawn_tilt_tau(PAWN_FIGURES[0].tilt_tau);
                    // FPV eye rest = the conventional figure's eye (TUNE_1 A3).
                    // Same shape as the tilt pin above; U1 re-authors it from
                    // the possessed figure every frame. U1 runs before the
                    // first upload (UPDATE_SPINE[0] vs [6]), so a zero could
                    // not actually reach the GPU — this declares the rest
                    // value where the other rest pins live (L10), it is not a
                    // guard against a reachable frame-1 hazard.
                    gpuState_.config().fpv_eye_height =
                        FPV_EYE_RATIO * PAWN_FIGURES[0].height;
                    gpuState_.mark_config_dirty();
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

                // ═══ MOVEMENT: BOOT — S2 THE SURFACE ════════════════════════
                // The same door the transition machine uses. reset_surface opens
                // with init_patch_system, so boot's order is unchanged; what boot
                // gains is the rest of the reset, which it previously received
                // only as in-struct defaults that HAPPENED to match.
                {
                    wgpu::Queue q = device_.GetQueue();
                    reset_surface(&machine_ctx_, q, tile_world_state_, themes_state_);
                }

                // ═══ MOVEMENT: BOOT — PER-PIECE BOOT VERBS (part one) ═══════
                // Order is today's, preserved byte-for-byte (PRIME INVARIANT);
                // one conductor call per piece, presence constexpr-gated.
                // BOOT IS A TRANSITION FROM NOTHING. The world has one way to come
                // into being; the only difference between boot and a mood change is
                // what came before. apply_mood is that one way — it subsumes the
                // frustum-cull row, the orb one-shot, and every atmospheric value
                // boot used to hand-copy from MOOD_TABLE[0].
                {
                    wgpu::Queue q = device_.GetQueue();
                    apply_mood(&mood_deps_, mood_state_.active, q,
                        machine_ctx_,
                        orbs_state_, orbs_deps_, gallery_state_, gallery_deps_,
                        pawn_state_);
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
                    dump_entity_census(&machine_ctx_, "boot");
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
                std::cout << "[Cartridge] Patch system:     "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t1).count() << " ms\n";
                std::cout << "[Cartridge] Total init:       "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t0).count() << " ms\n";

                // PORT_4b — THE BUDGET, once, after the LAST allocation.
                // Every GPU maker has now run: GPUState::init's five
                // creators, initOffscreenResources (the three painting
                // arrays — the 416 MiB family the old call site missed
                // entirely), and load_authored_textures' staging above.
                // Placed after the timings so it reads beneath "Total
                // init", and BEFORE the ROSTER + [Ground] block so that
                // block's claim to be the cartridge's last init line
                // stays true.
                gpuState_.report_gpu_budget();

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

                // P6 COROLLARY — a transition witness prints its state ONCE
                // at boot, so that silence afterwards means "no transition",
                // not "no witness". The Release boot carried no [Ground]
                // line at all and the two causes were indistinguishable;
                // this is that defect's cure. Same form as the transition,
                // marked (boot). Placed as the cartridge's LAST init line —
                // after the timings and the ROSTER block, before the meter
                // restamp — so it reports the state the first frame will
                // actually see.
                {
                    zoneRectsInCorePrev_ = zone_rects_in_core();
                    std::cout << "[Ground] zone rects in core: "
                              << zoneRectsInCorePrev_ << " (boot)\n";
                    // OPT_1e's switch and OPT_1a's landed with transition
                    // logs but no boot line — the exact half of P6 the
                    // Release boot paid for. Both seeded here from the same
                    // functions their gates read, so frame 1 can only print
                    // a REAL transition.
                    zonesActiveAnywherePrev_ = zones_active_anywhere();
                    std::cout << "[Ground] zones active anywhere: "
                              << zonesActiveAnywherePrev_ << " (boot)\n";
                    liveCardLivePrev_ = live_card_is_live();
                    std::cout << "[Card] live-card field: "
                              << live_card_state_label(liveCardLivePrev_)
                              << " (boot)\n";
                }

                // THE FRAME METER: boot ends here — restamp the window so
                // the first census fps excludes init wall-time.
                if constexpr (INSTRUMENTS.frame_meter) meter_.reset();

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
                WitnessHarvest, PortalTrigger, StreamPatches, RespawnAgents,
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

                gpuSignal.move_x = inputState_.move_x;
                gpuSignal.move_z = inputState_.move_z;
                gpuSignal.look_az_delta = inputState_.look_az_delta;
                gpuSignal.look_el_delta = inputState_.look_el_delta;
                gpuSignal.zoom_delta = inputState_.zoom_delta;
                gpuSignal.pan_x_delta = inputState_.pan_x_delta;
                gpuSignal.pan_y_delta = inputState_.pan_y_delta;
                gpuSignal.dt_beats = signal.t_beats - time_state_.prev_beats;  // beats since last frame -> step_trigger

                // Possessed body's tilt lag rides the config's slow-dial cadence
                // (CLOSURE_PAWN [6]). Idempotent: set_pawn_tilt_tau only dirties on a
                // real change, so the per-frame call costs nothing while the figure
                // stays put.
                //
                // NOT A GATED BLOCK — the braces are scope, not a condition, and
                // no enclosing one exists. All three fields below are authored on
                // every frame the update spine runs: this is UPDATE_SPINE[0]
                // (FillSignal) behind a literal-true roster gate, upload_config
                // rides UPDATE_SPINE[6] (StageFadeUpload) behind another, and the
                // agent kernels dispatch later still from the RENDER spine. No
                // dispatch can therefore read a value this block did not author,
                // and config_{}'s zero-init is unreachable by the GPU — the same
                // ordering the fpv_eye_height rest pin in initialize() already
                // states. Recorded here because for pawn_body_radius a zero is
                // not a degraded margin, it IS HEM_0's defect, and the next
                // reader should not have to re-derive that it cannot happen.
                {
                    const uint32_t sid = agent_state_.slots[player_.possessed_slot].skin_id;
                    gpuState_.set_pawn_tilt_tau(
                        sid < PAWN_FIGURE_COUNT ? PAWN_FIGURES[sid].tilt_tau : 0.0f);

                    // The possessed figure's own radius, on the same wire and
                    // the same guard (HEM_0). The boundary clamp insets the
                    // legal box by this, so the body — not its centre — stops
                    // at the wall, and the sample never lands on bmax, the
                    // EXCLUSIVE edge of the patch set. Derived here for the
                    // same reason the eye height below is: the compute stage
                    // cannot see agent_figure_profiles.
                    // The out-of-range arm reads figure 0's radius rather than a
                    // literal: pawn_figures.hpp's static_assert proves every ROW
                    // of the table positive, and a bare 0.0f would sit outside
                    // that proof's reach — the guard belongs inside the thing
                    // that makes the law true.
                    gpuState_.set_pawn_body_radius(
                        sid < PAWN_FIGURE_COUNT ? PAWN_FIGURES[sid].radius
                                                : PAWN_FIGURES[0].radius);

                    // FPV eye height follows the possessed figure (TUNE_1 A3).
                    // Derived here and not in the shader because
                    // agent_figure_profiles (binding 112) is a render-VS
                    // uniform — update_camera's compute layout does not carry
                    // it, and giving it one would be a new binding. Same
                    // out-of-range fallback as the tilt above, so an unknown
                    // skin lands on the conventional figure rather than at
                    // ground level. Guarded like set_pawn_tilt_tau: the config
                    // only dirties when the possessed figure actually changes.
                    const float eye = FPV_EYE_RATIO
                        * (sid < PAWN_FIGURE_COUNT ? PAWN_FIGURES[sid].height
                                                   : PAWN_FIGURES[0].height);
                    if (gpuState_.config().fpv_eye_height != eye) {
                        gpuState_.config().fpv_eye_height = eye;
                        gpuState_.mark_config_dirty();
                    }
                }
            }

            // The sky block is NOT part of the signal drain: upload_signal
            // skips the trailing 32 bytes, so its SOLE author is
            // resync_sky_head (the ribbon tick's tail), with a boot-neutral
            // covering the ribbon-off case. One writer, one disjoint region.

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
            // ── FIELD_4: THE BEACON (the first authored emitter) ──
            // The four dials moved to the panel
            // (contracts/control_panel.hpp), where the S < FIELD_K
            // self-spacing ruling compiles as a static_assert beside
            // them — the writer below reads them from there (FIELD_2a).

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

                // ZOETROPE (C4/C5): the lattice hears the canvas's row
                // impulses, ticks on the musical clock, and projects cells
                // to cube color through the partial-write door. Same member
                // plumbing as the flushes above; the queue is the phase's.
                zoetrope_strike(cube_behaviors_state_, gpuState_, c.queue,
                    world_state_.active_seed, visual_canvas_.zoetrope_rows(), signal.t_beats);
                zoetrope_service(cube_behaviors_state_, gpuState_, c.queue,
                    world_state_.active_seed, signal.t_beats, signal.dt,
                    point_.x, point_.z);   // the point mirror — the reseat watch (G4)

                // ── THE BEACON (FIELD_4): row 0, rewritten hot each
                // frame — the point moved. point y is DERIVED (the
                // point's house carries no y): host-routed — pawn
                // mirror y / ribbon head y / ground under the point
                // (the camera has no CPU y mirror; the harvest
                // discards cam pos[1]).
                {
                    GPUFieldAuthored fa{};
                    const float coord = gpuState_.config().floater_coordination;
                    float py;
                    if (point_.host == PointHost::PAWN) {
                        py = agent_state_.slots[player_.possessed_slot].pos_y;
                    } else if (point_.host == PointHost::RIBBON) {
                        py = ribbon_state_.head.pos[1];
                    } else {
                        py = estimate_terrain_height(tile_world_state_, point_.x, point_.z);
                    }
                    fa.count = 1u;
                    fa.rows[0][0] = point_.x;
                    fa.rows[0][1] = py + FIELD_BEACON_LIFT;
                    fa.rows[0][2] = point_.z;
                    fa.rows[0][3] = FIELD_BEACON_S * coord;
                    fa.rows[1][0] = FIELD_BEACON_R0;
                    fa.rows[1][1] = FIELD_BEACON_R;
                    fa.rows[1][2] = (coord > 0.0f) ? 1.0f : 0.0f;
                    gpuState_.upload_field_authored(c.queue, fa);
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
                        // OPT_1a: the new world's rest field must be written
                        // once even if no zone ever goes live there.
                        liveCardRestClean_ = false;
                        // THE FIRST-CAPTURE GATE (POINT_1, the measured seam):
                        // the harvest closures bind their gen at MAP time, so a
                        // copy STAGED in the old world (state COPIED) and
                        // mapped after this ++ would deliver old bytes under
                        // the fresh gen, passing the guard — the far first
                        // arrivals in Jean's log. Cancel stale staged copies
                        // here; a MAPPING machine is left alone: its callback
                        // bound the OLD gen and drops itself, and forcing it
                        // IDLE would let the poll double-map an in-flight
                        // buffer.
                        if (pawnReadbackState_ == PawnReadbackState::COPIED)
                            pawnReadbackState_ = PawnReadbackState::IDLE;
                        if (floaterReadbackState_ == FloaterReadbackState::COPIED)
                            floaterReadbackState_ = FloaterReadbackState::IDLE;

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
                        reset_surface(&machine_ctx_, queue, tile_world_state_, themes_state_);
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

                        point_.portal_trigger = -1;
                        // THE AUTHORED PRESENT (POINT_1): at a teleport the
                        // CPU is the author of the new present — the same
                        // position reset_player_agent / reseed_player_body
                        // write below. Idle::PAWN_POS is (0,0) today, so the
                        // old zero reset was this value BY LUCK; naming it
                        // makes the equality enforced (the reset_surface
                        // precedent), and every streaming consumer that runs
                        // before the first fresh harvest reads the true point.
                        point_.x = Idle::PAWN_POS_X;
                        point_.z = Idle::PAWN_POS_Z;
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
                            machine_ctx_,
                            orbs_state_, orbs_deps_, gallery_state_, gallery_deps_,
                            pawn_state_);
                        // ROSTER-GATE wanderers (c) — transition population (slots 1+); slot 0 preserved above.
                        if constexpr (ROSTER.wanderers)
                            spawn_population_for_mood(agent_state_, &agents_deps_, pendingDestination_.mood, world_state_.active_seed,
                                Idle::PAWN_POS_X, Idle::PAWN_POS_Z, queue);
                        dump_agent_census(agent_state_, &agents_deps_, "mood-transition");
                        // Fires AFTER reset_surface (:901) and every teardown
                        // verb above, and before stream_patches (a RENDER_SPINE
                        // row) can re-stream. Both columns must therefore read
                        // 0 for all twelve — a teardown-completeness assertion,
                        // not an observation.
                        dump_entity_census(&machine_ctx_, "mood-transition");
                        // ROSTER-GATE ribbon (c) — finite-mode release, owner
                        // verb. Zero effect when ribbon is off (active_count
                        // stays 0). ORDER (O-3): after apply_mood set
                        // mood_state_.active.
                        if constexpr (ROSTER.ribbon)
                            release_finite_ribbons(ribbon_state_, &ribbon_deps_, queue);
                        // Schedule guaranteed back-portal in finite worlds
                        mood_state_.back_portal_pending = world_state_.finite_mode;
                        // Re-arm the door guarantee for the new world (one
                        // shot, consumed by its first fullRegen).
                        mood_state_.door_fallback_pending = true;

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
                for (const URow& row : UPDATE_SPINE) {
                    if (!row.enabled) continue;   // gated-off rows are never timed
                    // The clock pair is the METER's, not the conductor's: with
                    // the instrument off the loop is the bare dispatch it was
                    // before the meter existed (core/instruments.hpp).
                    if constexpr (INSTRUMENTS.frame_meter) {
                        auto t0 = std::chrono::steady_clock::now();
                        (this->*row.fn)(ctx);
                        auto t1 = std::chrono::steady_clock::now();
                        float ms = std::chrono::duration<float, std::milli>(t1 - t0).count();
                        auto& s = meter_.u_rows[(size_t)row.id];
                        s.sum_ms += ms; if (ms > s.max_ms) s.max_ms = ms;
                    }
                    else {
                        (this->*row.fn)(ctx);
                    }
                }
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
                                        // THE POINT: point_.x/z is the
                                        // POINT's position — the body authors it
                                        // whenever the body hosts (PAWN, or
                                        // RIBBON — the possessed body rides the
                                        // seat); the camera harvest below authors
                                        // it in camera-host. The portal trigger is
                                        // the point's BUBBLE sensor, riding the
                                        // possessed slot's wire in every host.
                                        if (point_.host != PointHost::CAMERA) {
                                            point_.x = p.pos_x;
                                            point_.z = p.pos_z;
                                        }
                                        point_.portal_trigger = p.portal_trigger;
                                    }
                                }
                                gpuState_.agent_state_readback_staging().Unmap();
                            }
                            pawnReadbackState_ = PawnReadbackState::IDLE;
                        });
                }

                //
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
                                    }
                                }
                                gpuState_.floating_entity_readback_staging().Unmap();
                            }
                            floaterReadbackState_ = FloaterReadbackState::IDLE;
                        });
                }
            }

            // R2 — PORTAL TRIGGER (algo; GPU event). ENTRY door #2: a GPU-
            // reported trigger arms a transition (consumed next frame by U7 —
            // recon E-9). ROSTER-GATE transitions — guarded at the call site.
            void phase_portal_trigger(RenderCtx&) {
                if (point_.portal_trigger >= 0 && transitionPhase_ == TransitionPhase::IDLE) {
                    uint32_t arch_idx = static_cast<uint32_t>(point_.portal_trigger);
                    point_.portal_trigger = -1;
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
            // and the stream's bubble center reads point_.x/z refreshed at
            // HARVEST — no data edge. ROSTER-GATE wanderers — call site.
            void phase_respawn_agents(RenderCtx& c) {
                auto& queue = c.queue;
                respawn_evicted_agents(agent_state_, &agents_deps_, mood_state_.active, world_state_.active_seed, queue);
            }

            // R6 — CENSUS DUMPS (wall-clock interval, diagnostic). GoL residue
            // proof (G3, constexpr-gated intra-movement) + entity census.
            // Autonomous stdout (constitution §5).
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

                // THE DIAL (core/instruments.hpp). Everything below this line
                // is the PERIODIC instrument — the recurring dump and the
                // meter table that rides its cadence — and a print is a
                // blocking console write on the render thread, so ~50 lines
                // every 30 s IS the long frame every 30 s. Off, the row above
                // (the residue proof, a correctness witness) still runs and
                // this returns. The "boot" and "mood-transition" dumps are
                // NOT on the dial: those are P6 transition witnesses, and
                // their frames are already long.
                if constexpr (!INSTRUMENTS.periodic_census) return;

                // Periodic entity census dump — its own interval, its own
                // gate. THE INSTRUMENT the batch witnesses read; untouched.
                if (time_state_.seconds - spawn_engine_state_.lastCensusDump_ >= CENSUS_DUMP_INTERVAL) {
                    dump_entity_census(&machine_ctx_, "periodic");
                    spawn_engine_state_.lastCensusDump_ = time_state_.seconds;

                    // THE FRAME METER — the timing census rides the same
                    // cadence. Print ALL enabled rows (completeness feeds
                    // the suspect table; Jean pastes this block back
                    // verbatim). mean = sum/frames; fps = frames/wall.
                    // The dial conjunct is for ELIMINATION, not correctness:
                    // with the meter off nothing increments window_frames, so
                    // the block is already inert — the constant lets the
                    // compiler drop the formatting with it.
                    if (INSTRUMENTS.frame_meter && meter_.window_frames > 0) {
                        char line[160];
                        const float wall_s = std::chrono::duration<float>(
                            std::chrono::steady_clock::now() - meter_.window_start).count();
                        const float fps = wall_s > 0.0f
                            ? (float)meter_.window_frames / wall_s : 0.0f;
                        if (meter_gpu_)
                            std::snprintf(line, sizeof line,
                                "[METER] window %uf  fps %.1f  gpu sampled %uf | budget %.1f ms\n",
                                meter_.window_frames, fps, meter_.gpu_sampled_frames,
                                FrameMeter::FRAME_BUDGET_MS);
                        else
                            std::snprintf(line, sizeof line,
                                "[METER] window %uf  fps %.1f | budget %.1f ms\n",
                                meter_.window_frames, fps, FrameMeter::FRAME_BUDGET_MS);
                        std::cout << line;
                        double u_sum = 0.0, r_sum = 0.0;
                        for (const URow& row : UPDATE_SPINE) {
                            if (!row.enabled) continue;
                            const auto& s = meter_.u_rows[(size_t)row.id];
                            const double mean = s.sum_ms / meter_.window_frames;
                            u_sum += mean;
                            std::snprintf(line, sizeof line,
                                "[METER] U %-22s  mean %.2f  max %.2f\n",
                                row.name, mean, (double)s.max_ms);
                            std::cout << line;
                        }
                        for (const RRow& row : RENDER_SPINE) {
                            if (!row.enabled) continue;
                            const auto& s = meter_.r_rows[(size_t)row.id];
                            const double mean = s.sum_ms / meter_.window_frames;
                            r_sum += mean;
                            // A row line gains GPU columns when samples exist.
                            const auto& g = meter_.r_gpu[(size_t)row.id];
                            if (meter_.gpu_sampled_frames > 0 &&
                                (g.sum_ms > 0.0 || g.max_ms > 0.0f)) {
                                const double gmean = g.sum_ms / meter_.gpu_sampled_frames;
                                std::snprintf(line, sizeof line,
                                    "[METER] R %-22s  cpu %.2f/%.2f  gpu %.2f/%.2f\n",
                                    row.name, mean, (double)s.max_ms, gmean, (double)g.max_ms);
                            }
                            else {
                                std::snprintf(line, sizeof line,
                                    "[METER] R %-22s  mean %.2f  max %.2f\n",
                                    row.name, mean, (double)s.max_ms);
                            }
                            std::cout << line;
                        }
                        std::snprintf(line, sizeof line,
                            "[METER] U_SUM %.2f   R_SUM %.2f\n", u_sum, r_sum);
                        std::cout << line;
                        meter_.reset();
                    }
                }
            }

            // R7 — RIBBON TICK (music+wall-clock). One call; the conductor lives
            // in bodies/ribbon.hpp. Its tail is the sky resync — the SOLE author of
            // the sky_* block (E-3 mechanized: the drain skips those 32 bytes,
            // and initialize() boot-neutrals them for the ribbon-off case).
            // ROSTER-GATE ribbon — guarded at the call site.
            void phase_ribbon_tick(RenderCtx& c) {
                auto& queue = c.queue;
                // The sky-exit death first — it releases the ground, so it
                // takes the machine face the tick below does not carry.
                release_sky_exit_ribbon(&machine_ctx_, queue);
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
                    cpd.timestampWrites = gpuState_.meter_arm_compute((uint32_t)RPhase::EntityMeshGen);
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
            //
            // OPT_1a — THE REST SKIP: both dispatches (819,200 invocations)
            // are skipped while the card's field is at rest. The full
            // three-conjunct rest law, evaluated CPU-side and conservative
            // (any doubt => write): no GoL zone live, pulse ring empty,
            // terrain_time <= 0. Post-CUT_1 the last two are structurally
            // pinned at rest (O0-d: the ring's only writer is the boot
            // zero-pin; terrain_time's only writers pin 0.0) — checked
            // anyway so a future re-arming of either conjunct wakes the
            // writer without an edit here. On the transition into rest, ONE
            // final write runs so consumers never read stale non-zero
            // texels; the flag resets at world teardown so a fresh world's
            // rest field is written once too.
            void phase_live_card_write(RenderCtx& c) {
                auto& encoder = c.encoder;

                const bool card_live = live_card_is_live();

                // PROCESS P6 — every switch has a witness. This is the arm
                // ECONOMY_1 E1 taught the campaign to distrust: a rest skip
                // that never fires and a rest skip that works produce the
                // SAME silent log, and the difference is the entire unit.
                // Witness the INPUT that drives it (the zone_rects_in_core
                // form), on change of state only, never per frame; the boot
                // state prints once at init (P6 corollary), so silence here
                // means "no transition", not "no witness".
                if (card_live != liveCardLivePrev_) {
                    std::cout << "[Card] live-card field: "
                              << live_card_state_label(card_live) << "\n";
                    liveCardLivePrev_ = card_live;
                }

                if (card_live) {
                    liveCardRestClean_ = false;      // live: write every frame
                } else if (liveCardRestClean_) {
                    return;                          // at rest, card clean: skip
                } else {
                    liveCardRestClean_ = true;       // entering rest: one clearing write
                }

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

            uint32_t zoneRectsInCorePrev_ = 0;   // P6 witness memory (transitions only)
            uint32_t zonesActiveAnywherePrev_ = 0;   // OPT_1e witness memory
            bool     liveCardLivePrev_ = false;      // OPT_1a witness memory

            // OPT_1a — THE REST LAW, one home. The dispatch gate and the P6
            // witness read the SAME function, so the log can never disagree
            // with the skip (the zone_rects_in_core precedent). The three
            // conjuncts in occurrence order, short-circuiting: the two config
            // reads are free, the zone scan only runs if they clear.
            // Conservative by construction — any doubt is LIVE, and LIVE
            // writes.
            bool live_card_is_live() const {
                if (gpuState_.config().pulse_count > 0) return true;
                if (gpuState_.config().terrain_time > 0.0f) return true;
                for (uint32_t i = 0; i < Dim::MAX_GOL_ZONES; i++)
                    if (gol_state_.zones[i].active) return true;
                return false;
            }

            // The witness's words, one home, so the boot line and the
            // transition line cannot describe the same state differently.
            static const char* live_card_state_label(bool live) {
                return live ? "LIVE — writer runs every frame"
                            : "AT REST — one clearing write, then skipped";
            }

            // THE COUNT, one home — the draw plan's classifier input and the
            // P6 witness read the SAME function, so the log can never
            // disagree with the plan. Each active zone's world AABB
            // (persisted at commit_gol) inflated by one patch, tested
            // against the disc of the LIVE lod0_radius around the point —
            // the same radius patch_system.hpp bands patches by.
            uint32_t zone_rects_in_core() const {
                const float px = point_.x, pz = point_.z;
                const float r  = gpuState_.lod0_radius();
                uint32_t n = 0;
                for (uint32_t i = 0; i < Dim::MAX_GOL_ZONES; i++) {
                    const auto& z = gol_state_.zones[i];
                    if (!z.active) continue;
                    const float minx = z.corner_x - Dim::PATCH_EXTENT;
                    const float minz = z.corner_z - Dim::PATCH_EXTENT;
                    const float maxx = z.corner_x + z.extent_x + Dim::PATCH_EXTENT;
                    const float maxz = z.corner_z + z.extent_z + Dim::PATCH_EXTENT;
                    const float dx = std::max(0.0f, std::max(minx - px, px - maxx));
                    const float dz = std::max(0.0f, std::max(minz - pz, pz - maxz));
                    if (dx * dx + dz * dz <= r * r) n++;
                }
                return n;
            }

            // OPT_1e — THE GLOBAL COUNT. The LOD1 ring's two counts (clean
            // prefix / zoned) select on "any zone active ANYWHERE",
            // deliberately NOT geometric: zones outlive the LOD0 core
            // (EXIST reaches past the veil ring) and the curtain tail is
            // the only thing sealing lifted slab walls in the 175–325 wu
            // annulus, so any distance-scoped predicate would open them.
            // Conservative by construction: the prefix draws only at true
            // rest, when no cell anywhere can lift.
            uint32_t zones_active_anywhere() const {
                uint32_t n = 0;
                for (uint32_t i = 0; i < Dim::MAX_GOL_ZONES; i++)
                    if (gol_state_.zones[i].active) n++;
                return n;
            }

            // R17 — FRUSTUM CULL (algo; O-7 tail). Cull before the draw passes —
            // the indirect draws consume the cull output (recon E-5).
            void phase_frustum_cull(RenderCtx& c) {
                auto& encoder = c.encoder;
                auto& queue = c.queue;
                // ECONOMY_1 E1 rev2 — the LOD0-SCOPED curtain switch, staged
                // once per frame before every LOD0 carrier (R17 precedes the
                // main and snapshot passes; the shadow pass draws LOD1 since
                // E2 and has no curtains to switch).
                //
                // SCOPE: curtains exist ONLY in the LOD0 index buffer. The
                // cap-only choice is correct on a clean LOD0 patch because no
                // cell there lifts — not because a lift without a curtain is
                // harmless. In the LOD1 ring cells lift and own no curtain;
                // what seals those seams is the rim curtain (WALL_1 — skirt
                // ring copies stand on unlifted ground). The rev1 flag asked
                // "any zone anywhere" and was therefore inert: zones are alive
                // globally almost always, so it never released.
                //
                // Conservative by one patch: each zone's world AABB is
                // inflated by PATCH_EXTENT before the disc test, and the
                // radius is the LIVE lod0_radius — the same value
                // patch_system.hpp uses to sort patches into the LOD0 band,
                // so the flag and the band can never disagree.
                {
                    const uint32_t in_core = zone_rects_in_core();
                    // PROCESS P6 — every switch has a witness. The draw
                    // plan made the selection per-patch, so the witness
                    // reports the INPUT that drives it: the rect count in
                    // the core, on change of N only, never per frame. The
                    // boot state is printed once at init (P6 corollary), so
                    // silence here means "no transition", not "no witness".
                    if (in_core != zoneRectsInCorePrev_) {
                        std::cout << "[Ground] zone rects in core: " << in_core << "\n";
                        zoneRectsInCorePrev_ = in_core;
                    }
                    // The flag survives for the SNAPSHOT pass only (R6):
                    // the photographer culls against its own frustum and
                    // cannot read the plan; it keeps the flag-locked pair.
                    gpuState_.set_curtains_active(in_core > 0);
                }
                // OPT_1e — the LOD1 count switch, staged beside the curtain
                // switch: this dispatch's slot-C reset reads it, and so do
                // the sun's two LOD1 draws (R18 follows R17, so the flag is
                // fresh for both same-frame). P6: witness the INPUT that
                // drives the switch, on change of N only.
                {
                    const uint32_t anywhere = zones_active_anywhere();
                    if (anywhere != zonesActiveAnywherePrev_) {
                        std::cout << "[Ground] zones active anywhere: " << anywhere << "\n";
                        zonesActiveAnywherePrev_ = anywhere;
                    }
                    gpuState_.set_zones_active_anywhere(anywhere > 0);
                }
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

            // ═══ THE FRAME METER ════════════════════════════════════════════
            // A view of the spine — it lives beside the tables it reads (a
            // module home waits for a second consumer). CPU ms per executed
            // row (sum+max) over a census window; census_dumps prints the
            // table against FRAME_BUDGET_MS, then reset(). Gated-off rows
            // are never timed.
            //
            // THE DIAL: the whole instrument answers to INSTRUMENTS.frame_meter
            // (core/instruments.hpp), OFF by default. The state below still
            // EXISTS in the off build — a few hundred bytes of zeroed rows —
            // but nothing reads or writes it: no clock pair around a row, no
            // armed timestamp pair, no resolve, no readback, no table. The
            // structure is kept whole so a measurement session is one define
            // (T7_INSTRUMENTS=meter) and a rebuild, never a re-authoring.
            struct FrameMeter {
                static constexpr float FRAME_BUDGET_MS = 16.6f;   // the named budget
                struct RowStat { double sum_ms = 0.0; float max_ms = 0.0f; };
                RowStat u_rows[(size_t)UPhase::COUNT];
                RowStat r_rows[(size_t)RPhase::COUNT];
                uint32_t window_frames = 0;
                std::chrono::steady_clock::time_point window_start =
                    std::chrono::steady_clock::now();   // for fps
                // GPU half (M2): per-row pass ms from timestamp queries,
                // merged into the census when samples exist.
                RowStat r_gpu[(size_t)RPhase::COUNT];
                uint32_t gpu_sampled_frames = 0;
                // Snapshot of the armed pair table, taken at frame close
                // and consumed by the mapped callback. NOT zeroed by
                // reset() — a readback may be in flight across a window.
                MeterPair snap_pairs[GPUState::meter_max_pairs()] = {};
                uint32_t snap_pair_count = 0;
                void reset() {   // zero rows + frames, restamp window_start
                    for (auto& s : u_rows) s = RowStat{};
                    for (auto& s : r_rows) s = RowStat{};
                    for (auto& s : r_gpu) s = RowStat{};
                    window_frames = 0;
                    gpu_sampled_frames = 0;
                    window_start = std::chrono::steady_clock::now();
                }
            };
            FrameMeter meter_;

            // The meter_row registry (state.hpp — the GPU half's raw row
            // ids) is pinned to RPhase HERE, at the enum's home. Drift
            // fails glaw1.
            static_assert(meter_row::StreamPatches       == (uint32_t)RPhase::StreamPatches,       "meter_row drift: StreamPatches");
            static_assert(meter_row::EntityMeshGen       == (uint32_t)RPhase::EntityMeshGen,       "meter_row drift: EntityMeshGen");
            static_assert(meter_row::LiveCardWrite       == (uint32_t)RPhase::LiveCardWrite,       "meter_row drift: LiveCardWrite");
            static_assert(meter_row::DispatchCompute     == (uint32_t)RPhase::DispatchCompute,     "meter_row drift: DispatchCompute");
            static_assert(meter_row::GolDeriveFlush      == (uint32_t)RPhase::GolDeriveFlush,      "meter_row drift: GolDeriveFlush");
            static_assert(meter_row::GolZoneCompute      == (uint32_t)RPhase::GolZoneCompute,      "meter_row drift: GolZoneCompute");
            static_assert(meter_row::PawnAura            == (uint32_t)RPhase::PawnAura,            "meter_row drift: PawnAura");
            static_assert(meter_row::OrbSky              == (uint32_t)RPhase::OrbSky,              "meter_row drift: OrbSky");
            static_assert(meter_row::PlacementCorrection == (uint32_t)RPhase::PlacementCorrection, "meter_row drift: PlacementCorrection");
            static_assert(meter_row::FrustumCull         == (uint32_t)RPhase::FrustumCull,         "meter_row drift: FrustumCull");
            static_assert(meter_row::ShadowPass          == (uint32_t)RPhase::ShadowPass,          "meter_row drift: ShadowPass");
            static_assert(meter_row::MainPass            == (uint32_t)RPhase::MainPass,            "meter_row drift: MainPass");
            static_assert(meter_row::SnapshotPass        == (uint32_t)RPhase::SnapshotPass,        "meter_row drift: SnapshotPass");

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
            static_assert((uint32_t)RPhase::GroundEntries < (uint32_t)RPhase::PlacementCorrection, "O-4: ground entries (raises placement_dirty) before placement correction");
            static_assert((uint32_t)RPhase::FrustumCull < (uint32_t)RPhase::ShadowPass, "O-7: frustum cull precedes the shadow pass (ordering pin)");
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
                if constexpr (INSTRUMENTS.frame_meter) meter_.window_frames++;

                // THE FRAME METER — GPU half, harvest side. Mirrors the
                // floater readback grammar exactly (COPIED → MapAsync →
                // MAPPING; callback accumulates, Unmaps, → IDLE;
                // AllowSpontaneous). No world_gen capture: timing rows are
                // world-agnostic. dt discards per the spec's counter-reset
                // note: end ≤ begin, or dt > 100 ms.
                // The whole GPU half — harvest, arming table, frame-close
                // resolve — rides the instruments dial. Off, the frame issues
                // no ResolveQuerySet, no staging copy, and no MapAsync, and
                // keeps no readback in flight (core/instruments.hpp).
                if constexpr (INSTRUMENTS.frame_meter) {
                    if (meterReadbackState_ == MeterReadbackState::COPIED) {
                        meterReadbackState_ = MeterReadbackState::MAPPING;
                        gpuState_.meter_readback_staging().MapAsync(
                            wgpu::MapMode::Read, 0, GPUState::meter_readback_size(),
                            wgpu::CallbackMode::AllowSpontaneous,
                            [this](wgpu::MapAsyncStatus status, wgpu::StringView) {
                                if (status == wgpu::MapAsyncStatus::Success) {
                                    const auto* ts = static_cast<const uint64_t*>(
                                        gpuState_.meter_readback_staging().GetConstMappedRange(
                                            0, GPUState::meter_readback_size()));
                                    if (ts) {
                                        // METER_1.1: group pair dts by row into a
                                        // frame-local total FIRST, then fold sum/max
                                        // from the per-frame totals — multi-pass rows
                                        // (orb quartet, shadow atlas, patch batches)
                                        // otherwise printed a per-pass max below their
                                        // per-frame mean.
                                        double frame_ms[(size_t)RPhase::COUNT] = {};
                                        for (uint32_t p = 0; p < meter_.snap_pair_count; p++) {
                                            const uint64_t t0 = ts[meter_.snap_pairs[p].begin_idx];
                                            const uint64_t t1 = ts[meter_.snap_pairs[p].begin_idx + 1];
                                            if (t1 <= t0) continue;               // counter-reset garbage
                                            const double ms = (double)(t1 - t0) * 1e-6;   // u64 ns per index
                                            if (ms > 100.0) continue;             // same discard law
                                            frame_ms[meter_.snap_pairs[p].row] += ms;
                                        }
                                        for (size_t r = 0; r < (size_t)RPhase::COUNT; r++) {
                                            if (frame_ms[r] <= 0.0) continue;
                                            auto& s = meter_.r_gpu[r];
                                            s.sum_ms += frame_ms[r];
                                            if ((float)frame_ms[r] > s.max_ms) s.max_ms = (float)frame_ms[r];
                                        }
                                        meter_.gpu_sampled_frames++;
                                    }
                                    gpuState_.meter_readback_staging().Unmap();
                                }
                                meterReadbackState_ = MeterReadbackState::IDLE;
                            });
                    }
                    gpuState_.meter_frame_begin();   // rebuild this frame's arming table
                }

                for (const RRow& row : RENDER_SPINE) {
                    if (!row.enabled) continue;   // gated-off rows are never timed
                    if constexpr (INSTRUMENTS.frame_meter) {
                        auto t0 = std::chrono::steady_clock::now();
                        (this->*row.fn)(ctx);
                        auto t1 = std::chrono::steady_clock::now();
                        float ms = std::chrono::duration<float, std::milli>(t1 - t0).count();
                        auto& s = meter_.r_rows[(size_t)row.id];
                        s.sum_ms += ms; if (ms > s.max_ms) s.max_ms = ms;
                    }
                    else {
                        (this->*row.fn)(ctx);
                    }
                }

                // Frame close (after the last spine row, before the host's
                // Finish/Submit): resolve this frame's armed pairs, stage
                // the copy, snapshot the pair table. SKIP-IF-BUSY is the
                // law — at most one readback in flight.
                if constexpr (INSTRUMENTS.frame_meter) {
                    if (meter_gpu_ && gpuState_.meter_pair_count() > 0 &&
                        meterReadbackState_ == MeterReadbackState::IDLE) {
                        const uint32_t n = 2 * gpuState_.meter_pair_count();
                        encoder.ResolveQuerySet(gpuState_.meter_query_set(), 0, n,
                            gpuState_.meter_resolve_buffer(), 0);
                        encoder.CopyBufferToBuffer(
                            gpuState_.meter_resolve_buffer(), 0,
                            gpuState_.meter_readback_staging(), 0,
                            n * sizeof(uint64_t));
                        meter_.snap_pair_count = gpuState_.meter_pair_count();
                        for (uint32_t p = 0; p < meter_.snap_pair_count; p++)
                            meter_.snap_pairs[p] = gpuState_.meter_pairs()[p];
                        meterReadbackState_ = MeterReadbackState::COPIED;
                    }
                }
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

        // ─── Census: the per-family active_count + slot_census rows ────────
        //
        // THE COUNT IS A `.active` SCAN. NEVER A STORED FIELD. A stored
        // counter would be a number no consumer has ever validated; the
        // scan is ground truth. (The write-only per-family counters the
        // modules once carried were cut once the scan became the census.)
        // ARCH_2 extends the rule, it does not bend it: slot_census's
        // high-water is scanned too — it is the reach AT THAT SCAN, not a
        // running maximum, and nothing below stores state between dumps.
        //
        // The bound is DEDUCED from the array, never written. That is not
        // brevity — it makes three standing traps structurally unreachable:
        //   · MAX_RIBBON_INSTANCES / MAX_GALLERIES are t7::the_board namespace
        //     constants, NOT Dim:: members. Nothing here has to know that.
        //   · Dim::ANTENNA_SLOT_OFFSET (16) / Dim::CUBE_SLOT_OFFSET (8) are
        //     GPU-side only; both CPU arrays are 0-based. No offset can leak
        //     in, because no index arithmetic is written.
        //   · gol_state_.active_slot_count and cpu_pyramids.count are
        //     HIGH-WATER MARKS (highest active slot + 1), not populations.
        //     Both are live-read, which is what makes them tempting; neither
        //     is reachable from here. ARCH_2 makes this bullet sharper, not
        //     stale: slot_census now PRINTS a high-water, so the temptation
        //     is no longer "close enough" but "the same word". It is still
        //     the wrong number — those two fields are maintained for their
        //     own consumers, on their own cadence, for two families out of
        //     twelve. The census scans; it does not borrow.
        //
        // GALLERY counts gallery_centers, not painting_slots[32] — the latter
        // is shared with the indoor_shell feature via form_type == WALL_FRAME
        // and would mix outdoor paintings with indoor wall frames.
        //
        // ROSTER-disabled families are NOT special-cased: a disabled family is
        // never selected, so its array stays empty and it reads zero on both
        // sides. That agreement is itself a check.

        // One implementation, twelve callers (the P11 shape).
        template<typename T, size_t N>
        inline uint32_t census_scan_active(const T (&arr)[N]) {
            uint32_t n = 0;
            for (size_t i = 0; i < N; i++) if (arr[i].active) n++;
            return n;
        }

        // The occupancy triple (ARCH_2), same shape and same deduced bound.
        // N is the capacity: that is the ONLY way the ceiling reaches the
        // census without a constant being named here, which is what keeps the
        // three traps above unreachable — the ribbon/gallery bounds are not
        // Dim:: members, and the antenna/cube slot offsets are GPU-side.
        //
        // Deliberately a SECOND pass over the array, not a merge with
        // census_scan_active. The two numbers must be able to disagree: `live`
        // feeds the delta column that catches ground/body leaks, and a
        // diagnostic that silently re-derived it would put the leak check
        // downstream of itself. Twelve arrays at ≤288 entries, once per
        // census dump — the cost is not measurable beside the print.
        template<typename T, size_t N>
        inline SlotCensus census_scan_slots(const T (&arr)[N]) {
            SlotCensus s{ 0u, 0u, static_cast<uint32_t>(N) };
            for (size_t i = 0; i < N; i++) {
                if (!arr[i].active) continue;
                s.live++;
                s.high_water = static_cast<uint32_t>(i) + 1u;   // ascending scan: last write wins
            }
            return s;
        }

        inline uint32_t active_count_pyramid(const MachineCtx* c) { return census_scan_active(c->entities_state_.pyramids); }
        inline uint32_t active_count_arch   (const MachineCtx* c) { return census_scan_active(c->entities_state_.arches); }
        inline uint32_t active_count_column (const MachineCtx* c) { return census_scan_active(c->entities_state_.columns); }
        inline uint32_t active_count_antenna(const MachineCtx* c) { return census_scan_active(c->entities_state_.antennas); }
        inline uint32_t active_count_palm   (const MachineCtx* c) { return census_scan_active(c->entities_state_.palms); }
        inline uint32_t active_count_cactus (const MachineCtx* c) { return census_scan_active(c->entities_state_.cacti); }
        inline uint32_t active_count_blade  (const MachineCtx* c) { return census_scan_active(c->entities_state_.blades); }
        inline uint32_t active_count_sphere (const MachineCtx* c) { return census_scan_active(c->sphere_state_.activeSpheres_); }
        inline uint32_t active_count_ribbon (const MachineCtx* c) { return census_scan_active(c->ribbon_state_.active); }
        inline uint32_t active_count_cube   (const MachineCtx* c) { return census_scan_active(c->cube_behaviors_state_.activeCubes_); }
        inline uint32_t active_count_gol    (const MachineCtx* c) { return census_scan_active(c->gol_state_.zones); }
        inline uint32_t active_count_gallery(const MachineCtx* c) { return census_scan_active(c->gallery_state_.gallery_centers); }

        // The slot_census row — the SAME twelve arrays, named once more so the
        // capacity travels with the population. Any divergence between these
        // two lists is a family reporting its live count off one array and its
        // ceiling off another, so they are kept adjacent on purpose.
        inline SlotCensus slot_census_pyramid(const MachineCtx* c) { return census_scan_slots(c->entities_state_.pyramids); }
        inline SlotCensus slot_census_arch   (const MachineCtx* c) { return census_scan_slots(c->entities_state_.arches); }
        inline SlotCensus slot_census_column (const MachineCtx* c) { return census_scan_slots(c->entities_state_.columns); }
        inline SlotCensus slot_census_antenna(const MachineCtx* c) { return census_scan_slots(c->entities_state_.antennas); }
        inline SlotCensus slot_census_palm   (const MachineCtx* c) { return census_scan_slots(c->entities_state_.palms); }
        inline SlotCensus slot_census_cactus (const MachineCtx* c) { return census_scan_slots(c->entities_state_.cacti); }
        inline SlotCensus slot_census_blade  (const MachineCtx* c) { return census_scan_slots(c->entities_state_.blades); }
        inline SlotCensus slot_census_sphere (const MachineCtx* c) { return census_scan_slots(c->sphere_state_.activeSpheres_); }
        inline SlotCensus slot_census_ribbon (const MachineCtx* c) { return census_scan_slots(c->ribbon_state_.active); }
        inline SlotCensus slot_census_cube   (const MachineCtx* c) { return census_scan_slots(c->cube_behaviors_state_.activeCubes_); }
        inline SlotCensus slot_census_gol    (const MachineCtx* c) { return census_scan_slots(c->gol_state_.zones); }
        inline SlotCensus slot_census_gallery(const MachineCtx* c) { return census_scan_slots(c->gallery_state_.gallery_centers); }

        // ─── The table ─────────────────────────────────────────────────────
        // AXES: one row per family, POSITIONAL in PopFamily order (PYRAMID=0,
        //   ARCH, COLUMN, ANTENNA, PALM, CACTUS, BLADE, SPHERE, RIBBON, CUBE,
        //   GOL, GALLERY=11) — the enum values are pinned at roster.hpp (F-1)
        //   and every row's trailing name string is boot-checked against
        //   family_short_name by validate_spine (F-2), so a row swap fails
        //   LOUD. Row columns (FamilyDispatch, entity_types.hpp):
        //     { try_select, try_place, try_commit, evict_slot,
        //       prepare_mesh, dispatch_mesh, active_count, slot_census,
        //       grounded, name }
        // CONSUMERS: the machine tail walks select/place/commit per queue
        //   entry; eviction routes through evict_slot; the mesh pair feeds the
        //   RENDER_UPDATE mesh phases (none-fork = family has no mesh).
        inline const FamilyDispatch FAMILY_DISPATCH[PopFamily::COUNT] = {
            { dispatch_select_pyramid_generic, dispatch_place_pyramid_generic, dispatch_commit_pyramid_generic,
              evict_pyramid, dispatch_prepare_mesh_none, dispatch_mesh_gen_none,   // mesh hook → none-fork: pyramid mesh dead-by-design; placement feeds the heightfield
              active_count_pyramid,
              slot_census_pyramid,
              PYRAMID_TRAITS.grounded,
              "pyr" },
            { dispatch_select_arch_generic, dispatch_place_arch_generic, dispatch_commit_arch_generic,
              evict_arch,    Cartridge::dispatch_prepare_mesh_arch,    Cartridge::dispatch_mesh_gen_arch,
              active_count_arch,
              slot_census_arch,
              ARCH_TRAITS.grounded,
              "arch" },
            { dispatch_select_column_generic, dispatch_place_column_generic, dispatch_commit_column_generic,
              evict_column,  Cartridge::dispatch_prepare_mesh_column,  Cartridge::dispatch_mesh_gen_column,
              active_count_column,
              slot_census_column,
              COLUMN_TRAITS.grounded,
              "col" },
            { dispatch_select_antenna_generic, dispatch_place_antenna_generic, dispatch_commit_antenna_generic,
              evict_antenna, Cartridge::dispatch_prepare_mesh_column,  Cartridge::dispatch_mesh_gen_column,
              active_count_antenna,
              slot_census_antenna,
              ANTENNA_TRAITS.grounded,
              "ant" },
            { dispatch_select_palm_generic, dispatch_place_palm_generic, dispatch_commit_palm_generic,
              evict_palm,   Cartridge::dispatch_prepare_mesh_palm,   Cartridge::dispatch_mesh_gen_palm,
              active_count_palm,
              slot_census_palm,
              PALM_TRAITS.grounded,
              "palm" },
            { dispatch_select_cactus_generic, dispatch_place_cactus_generic, dispatch_commit_cactus_generic,
              evict_cactus, Cartridge::dispatch_prepare_mesh_cactus, Cartridge::dispatch_mesh_gen_cactus,
              active_count_cactus,
              slot_census_cactus,
              CACTUS_TRAITS.grounded,
              "cact" },
            { dispatch_select_blade_generic, dispatch_place_blade_generic, dispatch_commit_blade_generic,
              evict_blade, Cartridge::dispatch_prepare_mesh_blade, Cartridge::dispatch_mesh_gen_blade,
              active_count_blade,
              slot_census_blade,
              BLADE_TRAITS.grounded,
              "blad" },
            { dispatch_select_sphere_generic, dispatch_place_sphere_generic, dispatch_commit_sphere_generic,
              evict_sphere, dispatch_prepare_mesh_none, dispatch_mesh_gen_none,
              active_count_sphere,
              slot_census_sphere,
              SPHERE_TRAITS.grounded,   // false — orbits an anchor, claims no ground
              "sph" },   // no CPU mesh gen — GPU compute handles update_sphere
            { dispatch_select_ribbon, dispatch_place_ribbon, dispatch_commit_ribbon,
              evict_ribbon, dispatch_prepare_mesh_none, dispatch_mesh_gen_none,
              active_count_ribbon,
              slot_census_ribbon,
              true,   // anchored: the tips touch ground (no TRAITS object)
              "ribn" },  // no CPU mesh gen — GPU compute handles ribbon rendering
            { dispatch_select_cube_generic, dispatch_place_cube_generic, dispatch_commit_cube_generic,
              evict_cube, dispatch_prepare_mesh_none, dispatch_mesh_gen_none,
              active_count_cube,
              slot_census_cube,
              CUBE_TRAITS.grounded,      // false — hovers and drifts, claims no ground
              "cube" },  // no CPU mesh gen — GPU compute handles update_cube
            { dispatch_select_gol, dispatch_place_gol, dispatch_commit_gol,
              evict_gol, dispatch_prepare_mesh_none, dispatch_mesh_gen_none,
              active_count_gol,
              slot_census_gol,
              true,   // registers directly, gol_zones.hpp (no TRAITS object)
              "gol" },   // mesh hook → none-fork: GoL has no mesh — the zone IS
                         // the ground (UNIFIED_GROUND_1); the lift rides the card's .a
            { dispatch_select_gallery, dispatch_place_gallery, dispatch_commit_gallery,
              evict_gallery, dispatch_prepare_mesh_none, dispatch_mesh_gen_none,
              active_count_gallery,
              slot_census_gallery,
              true,   // registers directly, gallery.hpp (no TRAITS object)
              "gall" },
        };
    } // namespace the_board
} // namespace t7