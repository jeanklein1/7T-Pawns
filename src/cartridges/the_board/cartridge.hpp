#pragma once

// ─── cartridge.hpp ───────────────────────────────────────────────
// glaw1 — the compile gate: the C++ compiler as witness-runner. Every
// static_assert in the tree is a glaw1 check; "glaw1 catches X" means
// the build fails loud. WGSL sits outside its reach — the two-rooms
// mirror rule and the boot rig are the nets there.
//
// THE_BOARD — Generative world engine.
//
// ONE REGIME (L38, the composition law): every module is a file-scope pair around the class;
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
//   previous worlds via the issue-time generation recorded at the
//   machine (pawnReadbackGen_ / floaterReadbackGen_ — OIL_1c moved it
//   out of the closure). Genuinely spine-owned, not a leak.
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
#include "core/boot_params.hpp"                                    // DOMESDAY_1 B9 — ?seed= / ?mood= boot overrides (ctor, the one authoring site)
#include "core/boot_card.hpp"                                      // IOS_3 B2 — the world, the switches and the patch counters reach the page
#include "core/aubade.hpp"                                        // AUBADE U1 — the waterfall's marks and the first-present latch
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
#include "cartridges/the_board/contracts/driver_surface.hpp"       // THE DRIVERS' ROOM: rests and gains at the seams; phase_motion_drivers reads DRIVER_LIVE.fog
#include "cartridges/the_board/contracts/floaters.hpp"   // floater TYPES (ActiveSphere/ActiveCube), file scope
#include "cartridges/the_board/realization/state.hpp"
#include "console/organ_registry.hpp"   // the compiled dial registry + its C ABI (needs the home types above)
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

        // ═══ THE BOOT DRAW (DRAW_0) ══════════════════════════════════
        //
        // THE WORLD'S MASTER SEED IS CHOSEN AT BOOT, NOT AUTHORED AS A
        // CONSTANT. Each visit is a draw from the same latent space, the
        // way each painting is. One number changes and the generator
        // does the rest: terrain, the activity lattice, the waves, GoL
        // zone seeding, agent spawn and every entity placement all hang
        // off world_state_.active_seed and follow it without a further
        // edit anywhere.
        //
        // THE PIN (T7_WORLD_SEED, CMakeLists.txt) is empty by default,
        // so this macro is UNDEFINED and the seed is DRAWN. A number
        // defines it and the seed IS that number — the world becomes
        // exactly reproducible. -DT7_WORLD_SEED=42 is the authored world
        // (DEMO_SEED, demos/matrix.hpp) typed back in: the pin's default
        // value, and the acceptance test for this campaign. The dial is
        // a RESTORE, not a revert — there is nothing to undo.
        //
        // A DRAWN WORLD STAYS REPORTABLE because of the pin and the
        // witness line the root prints beside this call. A visitor's
        // "the ribbon speared something" is reproducible by reading the
        // seed off the console and pinning it.
        inline uint32_t boot_seed() {
#ifdef T7_WORLD_SEED
            return static_cast<uint32_t>(T7_WORLD_SEED);
#else
            // WALL TIME, AND THE REASON IS NOT STYLE. std::chrono::
            // steady_clock is performance.now() on Emscripten, and that
            // starts near ZERO at every page load — a steady_clock draw
            // would hand nearly every visitor the same world, silently,
            // and the only place the defect could surface is a user
            // report. system_clock is Date.now() there and the real wall
            // clock here: distinct per visit and per visitor.
            const uint64_t t = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count());

            // THE MIX. The raw clock's low bits are ADJACENT between two
            // visitors who arrive in the same second, and adjacent
            // numbers are not different worlds: tile_seed and
            // cpu_lattice_node_seed take the master seed through one xor
            // before their finalizer, so near inputs stay near through
            // the first round. cpu_hash (primitives/seed_utils.hpp) is
            // the tree's existing two-input scalar mixer — the same
            // multiply/xorshift idiom lattice_node_seed uses, three
            // rounds — and is shaped for exactly this: fold the 64-bit
            // clock's two halves into one avalanched u32. No new
            // mechanism was invented here.
            //
            // DEMO.seed rides the fold, and that is what keeps it live:
            // two demo columns with different authored seeds draw from
            // different streams, and the column's seed stays the number
            // the pin restores. XOR with a constant is a bijection, so
            // it costs the draw no entropy.
            return cpu_hash(static_cast<uint32_t>(t),
                            static_cast<uint32_t>(t >> 32) ^ DEMO.seed);
#endif
        }

        // The witness's second word. Boot-only; no frame reads it.
#ifdef T7_WORLD_SEED
        inline constexpr const char* BOOT_SEED_ORIGIN = "pinned";
#else
        inline constexpr const char* BOOT_SEED_ORIGIN = "drawn";
#endif

        class Cartridge : public RenderCartridge {

            // COMPOSITION ROOT — ORGANS ARE PUBLIC: sight is free; writes pass
            // through declared seams; the census enforces the seam law, not
            // access control.
        public:

            wgpu::Device device_;
            // OIL_1 U1: the queue, cached once at initialize() — the same
            // singleton object device_.GetQueue() returns; render() reads
            // this instead of re-fetching per frame (update() already
            // rides the harness-cached queue).
            wgpu::Queue queue_;
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
            TouchMoveState touch_;   // SHIP_1 — the stick's organ; never written on native
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
            // The mount's edge and its ease (RIBBON_1) — beside the point,
            // because a host change is what raises it. possess() writes it;
            // FillSignal ships and advances it; nothing reads it back.
            MountState mount_{};
            // THE PENDING dt (RIBBON_3) — the frame's dt as the GPU will see
            // it: the sum of every update since the last submitted frame,
            // capped at the same 100 ms the measurement is. Written by
            // phase_fill_signal, cleared by frame_submitted(); nothing else
            // touches it. time_state_.dt keeps the per-update value, because
            // the CPU's own integrators run on every update, rendered or not.
            float dtPending_ = 0.0f;

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
            //   portal trigger detection). The portal palette lives in
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
            //   from previous worlds via the issue-time generation recorded
            //   at the machine (the *ReadbackGen_ members below — OIL_1c
            //   moved it out of the closure, where nothing could address it).
            //   Genuinely spine-owned, not a leak.

            enum class PawnReadbackState { IDLE, COPIED, MAPPING };
            PawnReadbackState pawnReadbackState_ = PawnReadbackState::IDLE;
            // OIL_1c — THE GENERATION BELONGS TO THE MACHINE, not to a
            // closure. Written at ISSUE time (the COPIED→MAPPING arm),
            // read in the callback: the same fact the capture carried,
            // now somewhere addressable, beside the state it qualifies.
            // EQUIVALENT TO THE CAPTURE ONLY BECAUSE AT MOST ONE READBACK
            // PER MACHINE IS EVER IN FLIGHT — only IDLE arms a copy, only
            // COPIED issues a map, and the callback restores IDLE on its
            // way out, so no second issue can overwrite this while a
            // callback is pending. If that ever became two-in-flight, the
            // second issue would hand its gen to the first callback and
            // the P5 stale-world guard would pass where it must drop.
            uint32_t pawnReadbackGen_ = 0;

            // OPT_1a: true while the live card holds a clean rest field
            // (skip the writer); boots false so the first frame writes.
            bool liveCardRestClean_ = false;
            // SPINE_2 B: R8 decides, R10 encodes. The card's write is the
            // first dispatch of the compute pass now, so the rest law's
            // verdict travels the one row between them as a bool. R10
            // consumes it (sets it false), which is what keeps a frame that
            // never reached R10 from leaking a stale true into the next.
            bool liveCardWritePending_ = false;
            enum class FloaterReadbackState { IDLE, COPIED, MAPPING };
            FloaterReadbackState floaterReadbackState_ = FloaterReadbackState::IDLE;
            uint32_t floaterReadbackGen_ = 0;   // OIL_1c — same grammar as pawnReadbackGen_ above
            // ATRIUM_11 — THE CAMERA WITNESS'S MACHINE, the pawn's and the
            // floaters' grammar exactly: skip-if-busy, the issue-time
            // generation, the stale callback dropped. It exists because the
            // orbit has NO CPU mirror — compose_camera_position_from_orbit is
            // WGSL and the CHORD_3 block copy is GPU-to-GPU — so a pose made
            // with the mouse is unreadable from here any other way. Every
            // line of it, and the buffer behind it, is `if constexpr`-gated
            // on INSTRUMENTS.camera_witness, so an unarmed build has no third
            // readback at all.
            enum class CameraReadbackState { IDLE, COPIED, MAPPING };
            CameraReadbackState cameraReadbackState_ = CameraReadbackState::IDLE;
            uint32_t cameraReadbackGen_ = 0;
            // PLUMB_0 C1 (RULING-1) — THE POSE IS A FACT NOW, NOT A PRINT.
            //
            // The readback above already ran in EVERY instrument column
            // including `off` — camera_witness is one of the two arms true in
            // the shipped frame — and the 48 bytes it brought back were handed
            // to dump_camera_orbit and dropped. So the camera's pose has been
            // crossing to the CPU every frame all along, unusable only because
            // nothing kept it.
            //
            // THIS LEGALIZES A FACT; IT DOES NOT ADD A READBACK. The copy and
            // the map move out from under the `if constexpr` because they were
            // never conditional in practice; what stays under it is
            // dump_camera_orbit, which is the printer, and which keeps its
            // retirement warrant ("RETIRE IT once the arrival row is settled")
            // without taking the pose down with it.
            //
            // ONE FRAME STALE, BY THE SAME LAW AS point_. The copy is encoded
            // at R11 and mapped at R1 of the following frame, so a reader gets
            // the pose the world had last frame — the staleness class the
            // streamer has always worked in.
            //
            // The forward vector, for whoever wants one:
            //   -(cos(el)*sin(az), sin(el), cos(el)*cos(az))
            // the convention build_view_projection_matrix,
            // compose_camera_position_from_orbit and the camera-host fly
            // branch already share, so there is no per-mode fork.
            CameraPose camera_pose_{};
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
            // WRAP_0 U4 — THE SLOT LINE'S SAMPLE. The draw plan's three
            // instance counters live only on the GPU, so the terrain's
            // milliseconds could never be divided by its geometry. Same
            // grammar as the meter's own readback and the same SKIP-IF-BUSY,
            // which is right here rather than wrong: one sample is wanted per
            // window, not per frame, and a count of visible patches is
            // geometry — it moves at walking pace, so a sample a second or two
            // old is still a true reading, where a timing sample would not be.
            MeterReadbackState slotReadbackState_ = MeterReadbackState::IDLE;
            uint32_t slotInstances_[3] = {};
            bool     slotSampleValid_ = false;
            bool meter_gpu_ = false;   // device carries timestamp-query (set at initialize)

            // ROSTER-RESIDUE gol (2e) instrumentation: count of frames the GoL
            // zone-compute block ran (the sole writer of the zone GPU buffers),
            // and the residue-report cadence timer. Read only by the residue
            // check when ROSTER.gol is disabled (proves the buffers pristine).
            uint64_t rosterGolZoneRuns_ = 0;
            double   rosterGolResidueDump_ = 0.0;   // PLUMB_0 B1 — differenced against TimeState::seconds

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
                self->renderer_.dispatch_arch_mesh_gen(pass, self->gpuState_.meshgen_state_group(), self->gpuState_.empty_group());
            }

            static bool dispatch_prepare_mesh_column(MachineCtx* self, wgpu::Queue& queue) {
                return prepare_column_mesh_gen(self->entities_state_, self, queue);
            }
            static void dispatch_mesh_gen_column(MachineCtx* self, wgpu::ComputePassEncoder& pass) {
                self->renderer_.dispatch_column_mesh_gen(pass, self->gpuState_.meshgen_state_column_group(), self->gpuState_.empty_group());
            }

            // ── Mesh gen dispatch wrappers (palm) ──

            static bool dispatch_prepare_mesh_palm(MachineCtx* self, wgpu::Queue& queue) {
                return prepare_palm_mesh_gen(self->entities_state_, self, queue);
            }
            static void dispatch_mesh_gen_palm(MachineCtx* self, wgpu::ComputePassEncoder& pass) {
                self->renderer_.dispatch_palm_mesh_gen(pass, self->gpuState_.meshgen_state_palm_group(), self->gpuState_.empty_group());
            }

            // ── Mesh gen dispatch wrappers (cactus) ──

            static bool dispatch_prepare_mesh_cactus(MachineCtx* self, wgpu::Queue& queue) {
                return prepare_cactus_mesh_gen(self->entities_state_, self, queue);
            }
            static void dispatch_mesh_gen_cactus(MachineCtx* self, wgpu::ComputePassEncoder& pass) {
                self->renderer_.dispatch_cactus_mesh_gen(pass, self->gpuState_.meshgen_state_cactus_group(), self->gpuState_.empty_group());
            }

            static bool dispatch_prepare_mesh_blade(MachineCtx* self, wgpu::Queue& queue) {
                return prepare_blade_mesh_gen(self->entities_state_, self, queue);
            }
            static void dispatch_mesh_gen_blade(MachineCtx* self, wgpu::ComputePassEncoder& pass) {
                self->renderer_.dispatch_blade_mesh_gen(pass, self->gpuState_.meshgen_state_blade_group(), self->gpuState_.empty_group());
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
                                time_state_, player_, point_, camera_pose_,
                                gpuState_, renderer_ }
                , tile_world_deps_{ world_state_, mood_state_, gpuState_ }
                , sphere_deps_{ time_state_ }
                , pawn_deps_{ player_, time_state_, gpuState_, renderer_ }
                , orbs_deps_{ gpuState_, renderer_, player_, time_state_, world_state_ }
                , agents_deps_{ gpuState_, player_, point_, transitionPhase_, world_state_, time_state_ }
                , cube_deps_{ gpuState_, time_state_, player_, point_, mood_state_ }
                , gol_deps_{ gpuState_, renderer_, device_, time_state_ }
                , ribbon_deps_{ gpuState_, time_state_, tile_world_state_, player_, point_, inputState_, world_state_, mood_state_, visual_canvas_, ribbon_amp_lat_dst_, ribbon_amp_vert_dst_, ribbon_tint_stim_dst_, ribbon_tint_mix_dst_ }
                , gallery_deps_{ gpuState_, renderer_, world_state_, tile_world_state_, ribbon_state_, player_, point_, mood_state_, time_state_, sunDirection_, clearColor_ }
                , input_deps_{ inputState_, keys_, mouse_, touch_, player_, world_state_, ribbon_state_, gpuState_, device_, point_, mount_, camera_ }
                , mood_deps_{ mood_state_, world_state_, gpuState_, renderer_, gol_state_, entities_state_, sunDirection_, sunColor_, clearColor_, cpuSpotLights_, cpuPortalArray_, backPortalPosition_ } {
                // THE ROOT AUTHORS THE BOOT VALUES (the demo sentence lands
                // here, not via in-struct defaults — no include-order cable).
                // DRAW_0: the seed is DRAWN, not authored — boot_seed()
                // (above this class) decides, and DEMO.seed is what the pin
                // restores. This line is the campaign's whole edit site.
                world_state_.active_seed = boot_seed();
                // DOMESDAY_1 B9 — the parameter surface: a seed present at
                // boot (?seed= / --seed=) overrides the draw at this one
                // authoring site. Measurement first; boot-read; no mid-run
                // mutation.
                const char* seed_origin = BOOT_SEED_ORIGIN;
                if (boot_params().has_seed) {
                    world_state_.active_seed = boot_params().seed;
                    seed_origin = "param";
                }
                // THE WITNESS (P6). One line, at boot, immediately after the
                // choice and before any consumer — zero frame cost. It prints
                // on both twins; on the web it reaches the DETAILS panel
                // through Module.print (web/index.html routes every stdout
                // line into the log), which is how Jean reads it. This line is
                // what keeps a randomized world reportable.
                std::cout << "[World] Boot seed=" << world_state_.active_seed
                          << " (" << seed_origin << ")\n";
                // IOS_3 B2 — and onto the page. The switches ride this line
                // because a diagnosis is reproducible only if the photograph
                // says which switches were set when it was taken.
                {
                    const auto& bp = t7::boot_params();
                    t7::card_fact("world  seed=" + std::to_string(world_state_.active_seed)
                                  + " (" + seed_origin + ")");
                    t7::card_fact(std::string("switch bake=") + (bp.bake ? "1" : "0")
                                  + " card=" + (bp.card ? "1" : "0")
                                  + " sunpass=" + (bp.sunpass ? "1" : "0")
                                  + " bundles=" + (bp.bundles ? "1" : "0"));
                }
                mood_state_.active = DEMO.boot_mood;
                // B9 — a mood present at boot (?mood= / --mood=) forces the
                // boot mood at this one authoring site; an out-of-range
                // index is refused OUT LOUD (P6 — a switch that half-fired
                // must not look fired).
                if (boot_params().has_mood) {
                    if (boot_params().mood < MOOD_COUNT) {
                        mood_state_.active = boot_params().mood;
                    } else {
                        std::cout << "[Params] mood=" << boot_params().mood
                                  << " out of range (MOOD_COUNT="
                                  << MOOD_COUNT << ") — ignored\n";
                    }
                }


                // BOOT IS A TRANSITION FROM NOTHING — IN FACT (ATRIUM_0).
                // The seed and the mood are settled above; the world's other
                // two destination facts were in-struct defaults until now,
                // right only while the boot mood was open. The radius is
                // DERIVED from the seed, never authored: DemoConfig does not
                // grow, and its parked D2 axis stays parked.
                {
                    const auto& bm = mood_def(mood_state_.active);
                    const PortalDestination boot{ world_state_.active_seed, bm.shape.finite,
                        derive_finite_radius(world_state_.active_seed, bm), mood_state_.active };
                    become_destination(boot);
                }
                // ATRIUM_0 — a boot world has no back (nothing precedes), but
                // its forwards no longer hang off the return.
                mood_state_.forward_portals_pending = world_state_.finite_mode;

                // EXHIBIT_0 — THE EXHIBITION IS A FETCH, AND IT STARTS HERE.
                // This is the earliest instant a GalleryState exists to fill,
                // and on the web twin it runs inside main() BEFORE
                // console.init asks the browser for an adapter — so the
                // manifest travels while the device request is still
                // outstanding, over rAF turns that pump nothing else. It is
                // therefore normally parsed before the conductor's first
                // fill (the deferred hang's head, OVERTURE_0), and always
                // before the first gallery. A manifest that is still in flight is an empty
                // manifest, which is the already-legal no-paintings state.
                // No device, no queue, no GPU touched: this fetch fills a
                // vector of names and nothing more.
                kick_exhibition_manifest_fetch(gallery_state_);
            }

            Cartridge(const Cartridge&) = delete;
            Cartridge& operator=(const Cartridge&) = delete;

            void initialize(wgpu::Device device) override {
                device_ = device;
                queue_ = device_.GetQueue();   // OIL_1 U1 — one fetch, one home
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
                    gpuState_.config().possessed_height =
                        PAWN_FIGURES[0].height;
                    gpuState_.mark_config_dirty();
                }

                // RIBBON_1: the mount block rides the whole-struct signal drain
                // like every other word, so there is no boot-neutral to write —
                // MountState rests at phase 1, kind 0 (arrived, nothing in
                // flight) and the first frame ships that.
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

                // Agent registries — behaviors from AGENT_BEHAVIORS
                // (bodies/agents.hpp), tiers from the world's definition
                // bank TIER_LIVE (contracts/agent_tiers.hpp), uploaded to
                // GPU storage buffers at bindings 110 + 111. Behaviors are
                // constexpr-equivalent, so for them this stays a one-shot
                // write at boot; the bank is a live surface, and the frame
                // boundary re-speaks this same author whenever the panel
                // edits it.
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

                auto t3 = std::chrono::high_resolution_clock::now();

                std::cout << "[Cartridge] Renderer init:    "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms\n";
                std::cout << "[Cartridge] Patch system:     "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t1).count() << " ms\n";
                // AUBADE U1 — init RETURNS here. The 75 ms this line has
                // always printed is the innocent part; every mark after it
                // is the part nobody had measured.
                t7::aubade_mark("init");
                std::cout << "[Cartridge] Total init:       "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t0).count() << " ms\n";

                // PORT_4b — THE BUDGET, once, after the LAST allocation.
                // Every GPU maker has now run: GPUState::init's five
                // creators and initOffscreenResources (the three painting
                // arrays — the 416 MiB family the old call site missed
                // entirely). The authored fill makes nothing: it queues
                // fetches, and an arrival writes into a texture
                // initOffscreenResources already created.
                // Placed after the timings so it reads beneath "Total
                // init", and BEFORE the ROSTER + [Ground] block so that
                // block's claim to be the cartridge's last init line
                // stays true.
                gpuState_.report_gpu_budget();
                // ORGAN — the homes exist by now, so bind them; the ABI is
                // inert until this runs. The mood organ is BORROWED, so the
                // panel cannot name a state the program has left.
                t7::organ::bind_home(&gpuState_);
                t7::organ::bind_mood(&mood_state_);
                t7::organ::bind_point(&point_);   // RIBBON_1 — the panel's host row

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
            //   E-3 (sky write-order) — DEAD WITH ITS SUBJECT (RIBBON_1). It
            //     was a three-writer relay over a POSE the ribbon tick had to
            //     re-write after the drain; the split drain that mechanized it
            //     was the second cure. The pose is the GPU's now
            //     (ribbon_body_read.saddle) and the trailing words carry only
            //     the mount EDGE, which the signal's one author fills like any
            //     other word. One writer, one whole-struct write, no ordering to
            //     preserve and nothing left to say.
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
                wgpu::TextureView     msaaColor;   // B10: null at msaa=1
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
                // PLUMB_0 B1 — THE WALL CLOCK ADVANCES HERE, IN DOUBLE, AND
                // BEFORE ANYTHING READS IT. U1 is the first update row and is
                // enabled by a literal true, so this runs on every frame that
                // updates at all; the O-5a static_assert already pins U1
                // before U3, which is why the accumulation moved up from
                // phase_advance_clock rather than staying beside the beats.
                // Accumulating from signal.dt rather than copying
                // signal.t_seconds is the whole unit: the copy inherited a
                // float accumulator that stops after 4.8 days.
                time_state_.seconds += (double)signal.dt;
                // PLUMB_0 B2 — the shader gets the young number (RULING-3).
                gpuSignal.t_seconds = time_state_.gpu_seconds();
                gpuSignal.t_beats = signal.t_beats;
                // RIBBON_2 P0 1.2b — THE PENDING dt (RIBBON_3). The signal is
                // written every update; the compute pass that consumes it is
                // encoded only on a RENDERED frame. Writing signal.dt here
                // meant a dropped acquire's dt was overwritten by the next
                // update and DELETED from the GPU's integrators. It
                // accumulates instead, and is cleared by frame_submitted().
                // The same 100 ms ceiling the raw measurement carries applies
                // to the sum: a stretch is a stretch, a teleport is not.
                dtPending_ = std::min(dtPending_ + signal.dt, 0.1f);
                gpuSignal.dt = dtPending_;
                gpuSignal.aspect_ratio = aspect_ratio;

                gpuSignal.move_x = inputState_.move_x;
                gpuSignal.move_z = inputState_.move_z;
                gpuSignal.look_az_delta = inputState_.look_az_delta;
                gpuSignal.look_el_delta = inputState_.look_el_delta;
                gpuSignal.zoom_delta = inputState_.zoom_delta;
                gpuSignal.pan_x_delta = inputState_.pan_x_delta;
                gpuSignal.pan_y_delta = inputState_.pan_y_delta;
                gpuSignal.dt_beats = signal.t_beats - time_state_.prev_beats;  // beats since last frame -> step_trigger

                // THE MOUNT BLOCK (RIBBON_1) — ship the edge, then advance the
                // ease. Shipping FIRST is the point: the frame the host changed
                // on must reach the GPU at phase 0, or the body starts the
                // trajectory already partway along it. possess() authored the
                // edge; this is its only carrier; nothing reads it back.
                gpuSignal.mount_phase        = mount_.phase;
                gpuSignal.mount_kind         = mount_.kind;
                gpuSignal.mount_from[0]      = mount_.from[0];
                gpuSignal.mount_from[1]      = mount_.from[1];
                gpuSignal.mount_from[2]      = mount_.from[2];
                gpuSignal.mount_from_heading = mount_.from_heading;
                if (mount_.kind != 0u) {
                    const float secs = (mount_.kind == 1u) ? RIBBON_LIVE.board_seconds
                                                           : RIBBON_LIVE.land_seconds;
                    mount_.phase += signal.dt / (secs > 1e-3f ? secs : 1e-3f);
                    if (mount_.phase >= 1.0f) { mount_.phase = 1.0f; mount_.kind = 0u; }
                }

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
                    // cannot see scene_constants.figure_profiles.
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
                    // scene_constants.figure_profiles rides a render-VS-only
                    // uniform block (g2:200) — update_camera_vp's compute layout
                    // does not carry it, and giving it one would be a new
                    // binding. Same
                    // out-of-range fallback as the tilt above, so an unknown
                    // skin lands on the conventional figure rather than at
                    // ground level. Guarded like set_pawn_tilt_tau: the config
                    // only dirties when the possessed figure actually changes.
                    const float fig_h =
                        (sid < PAWN_FIGURE_COUNT ? PAWN_FIGURES[sid].height
                                                 : PAWN_FIGURES[0].height);
                    const float eye = FPV_EYE_RATIO * fig_h;
                    if (gpuState_.config().fpv_eye_height != eye) {
                        gpuState_.config().fpv_eye_height = eye;
                        gpuState_.mark_config_dirty();
                    }
                    // STATURE_0 — THE SAME FIGURE'S HEIGHT, UNSCALED. The
                    // photographer frames the subject at its own fractions
                    // (compute_photographer_vp), so it needs the fact and not
                    // the FPV camera's ratio of it. One selection now serves
                    // both, on the same guard and the same out-of-range arm:
                    // an unknown skin frames as the conventional figure rather
                    // than at ground level.
                    if (gpuState_.config().possessed_height != fig_h) {
                        gpuState_.config().possessed_height = fig_h;
                        gpuState_.mark_config_dirty();
                    }
                }
            }

            // U3 — ADVANCE CLOCK (music+wall-clock). The tempo follower; bumps
            // prev_beats (the O-5a partner of U1's dt_beats read).
            void phase_advance_clock(UpdateCtx& c) {
                auto& signal = c.signal;
                time_state_.beats = signal.t_beats;
                // PLUMB_0 B1 — `seconds` is NOT copied here any more. It is
                // accumulated at U1 from signal.dt, one row earlier, so that
                // U1's own GPU write already sees this frame's value rather
                // than the previous frame's. AnalysisSignal::t_seconds stays
                // the music contract's field at offset 0; it is no longer the
                // wall clock of record and no longer has a reader in this
                // file.
                time_state_.dt = signal.dt;
                // The frame's third clock (PANORAMA_1) — advanced HERE, with
                // the other two, so there is one place a frame begins.
                world_state_.frame_index++;
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
                // ORGAN — the drivers' room sits at this seam:
                // out = rest + gain·deviation. The REST is the mood's,
                // drawn per world into mood_state_.fog_rest_* by
                // apply_mood_lighting; the DEVIATION is the canvas's,
                // measured from its anchor row. Gain 1 is the coupling
                // verbatim, gain 0 is the mood's own fog, and with no
                // bindings the rest alone speaks — so the dial works
                // headless too.
                //
                // set_fog GUARDS — it compares all four lanes and dirties
                // only on a change — so both arms call it unconditionally
                // and the silent case costs no dirty.
                {
                    const auto& drv = DRIVER_LIVE.fog;
                    const auto& ms  = mood_state_;
                    if (fog_density_dst_.valid && fog_color_dst_.valid) {
                        const VisualParams& fp = visual_canvas_.params();
                        gpuState_.set_fog(
                            std::max(0.0f, ms.fog_rest_density + drv.gain * fp.get(fog_density_dst_.base)),
                            std::clamp(ms.fog_rest_color[0] + drv.gain * fp.get(fog_color_dst_.base + 0), 0.0f, 1.0f),
                            std::clamp(ms.fog_rest_color[1] + drv.gain * fp.get(fog_color_dst_.base + 1), 0.0f, 1.0f),
                            std::clamp(ms.fog_rest_color[2] + drv.gain * fp.get(fog_color_dst_.base + 2), 0.0f, 1.0f));
                    } else {
                        gpuState_.set_fog(ms.fog_rest_density, ms.fog_rest_color[0],
                                          ms.fog_rest_color[1], ms.fog_rest_color[2]);
                    }
                }
                // CHECKER-REBUILD: the pc-color field's flush — one setter,
                // the fan (resultant rgb + music amount + music variance).
                // ORGAN — the drivers' room sits at this seam too:
                // out = rest + gain·(driven − rest), the fog recipe verbatim.
                // Gain 1 is the coupling byte-for-byte; gain 0 is the rest,
                // which terrain_looks calls law — amount 0 returns each cell
                // to its SEED colour, not to gray. With no bindings the rest
                // alone speaks, so the dial works headless.
                //
                // The resultant is a run of three, so it blends per lane
                // against rest_resultant[] rather than against one scalar.
                if (checker_mean_dst_.valid && checker_var_dst_.valid) {
                    const VisualParams& cp = visual_canvas_.params();
                    const auto& ck = DRIVER_LIVE.checker;
                    const float* mean = cp.run(checker_mean_dst_.base);
                    const float blended[3] = {
                        ck.rest_resultant[0] + ck.gain * (mean[0] - ck.rest_resultant[0]),
                        ck.rest_resultant[1] + ck.gain * (mean[1] - ck.rest_resultant[1]),
                        ck.rest_resultant[2] + ck.gain * (mean[2] - ck.rest_resultant[2]),
                    };
                    gpuState_.set_checker_color_field(blended,
                        ck.rest_amount   + ck.gain * (cp.get(checker_var_dst_.base)     - ck.rest_amount),
                        ck.rest_variance + ck.gain * (cp.get(checker_var_dst_.base + 1) - ck.rest_variance));
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
                } else {
                    // No bindings: the rest alone speaks, so the dials still
                    // reach the picture with the music silent — the fog
                    // seam's headless arm again.
                    // set_checker_color_field guards, so this costs no dirty.
                    const auto& ck = DRIVER_LIVE.checker;
                    gpuState_.set_checker_color_field(ck.rest_resultant,
                                                      ck.rest_amount, ck.rest_variance);
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
                // mirror y (PAWN and RIBBON alike — the possessed body IS
                // where the point is in both, riding the seat in one and
                // walking in the other) / ground under the point in
                // camera-host (the camera has no CPU y mirror; the harvest
                // discards cam pos[1]).
                {
                    GPUFieldAuthored fa{};
                    const float coord = gpuState_.config().floater_coordination;
                    float py;
                    if (point_.host != PointHost::CAMERA) {
                        py = agent_state_.slots[player_.possessed_slot].pos_y;
                    } else {
                        py = estimate_terrain_height(tile_world_state_, point_.x, point_.z);
                    }
                    // ORGAN — the beacon reads its BANK, PANEL_LIVE, and
                    // not the design table: a bank nothing reads is a dial
                    // that writes nothing.
                    const auto& bcn = PANEL_LIVE.beacon;
                    // THE RING SELF-SPACES, AT RUNTIME TOO.
                    // control_panel.hpp's static_assert proves the AUTHORED
                    // pair; this clamp guards the DIALED one, reading
                    // config's LIVE field_k rather than the constexpr
                    // because field_k is a dial too and lowering it breaks
                    // the same ruling from the other end. It sits at the
                    // writer, not at the panel, so every author is
                    // guarded by it.
                    const float ceiling = gpuState_.config().field_k - 1.0f;
                    float s = bcn.s;
                    if (s > ceiling) s = ceiling;
                    if (s < 0.0f)    s = 0.0f;
                    fa.count = 1u;
                    fa.rows[0][0] = point_.x;
                    fa.rows[0][1] = py + bcn.lift;
                    fa.rows[0][2] = point_.z;
                    fa.rows[0][3] = s * coord;
                    fa.rows[1][0] = bcn.r0;
                    fa.rows[1][1] = bcn.r;
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

            // L10 — a destination becomes the world through ONE door. The
            // teardown and the ctor both walk it; boot is a transition from
            // nothing in fact, not in doctrine (ATRIUM_0: finite_mode and
            // finite_radius were in-struct defaults at boot, correct by luck
            // while the boot mood was open).
            void become_destination(const PortalDestination& d) {
                // PLUMB_0 B2 (RULING-3) — THE GPU'S ZERO MOVES WITH THE WORLD,
                // and this is the ONE door every world enters by: the boot call
                // and the TEARDOWN call both land here. Stamped in the arm
                // alone it missed the boot world entirely, which is the session
                // shape the rebase most exists for.
                //
                // Every GPU-resident time is re-stamped after this: the ribbon
                // table and the orb state are cleared by their teardown verbs
                // in the same arm, and the frame signal is written fresh every
                // frame, so the rebase cannot tear a live animation.
                time_state_.world_epoch = time_state_.seconds;
                // PULSE_1 — THE FOURTH SEAM, NOW THAT IT IS ARMED. B-G1's
                // census found three GPU-resident times that cross a rebase
                // and named the ring a fourth "but set_pulse_data is only
                // ever called with zeros; it is inert" (TimeState's own
                // comment, corrected there). It is not inert any more: an
                // onset stamped in the world we are leaving would be
                // differenced against the new world's epoch. The ribbon table
                // and the orb state are cleared by their teardown verbs in
                // this same arm; the ring is cleared HERE, at the one door
                // every world enters by, because that is where the epoch
                // moves — so boot is covered by the same line.
                clear_pulse_ring();
                world_state_.active_seed   = d.seed;
                world_state_.finite_mode   = d.finite;
                world_state_.finite_radius = d.finite_radius;
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
                        // PLUMB_0 B2's epoch stamp MOVED to become_destination
                        // (closing refuter): stamped here it never fired on the
                        // BOOT world, so a session that never changed world ran
                        // with epoch 0 and the rebase was inert — exactly the
                        // long-session case it exists for. become_destination
                        // is called at boot AND from this arm, so it is the one
                        // door every world enters by.
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
                        // ATRIUM_11 — same cancel. UNGATED (PLUMB_0 refuter):
                        // C1 promoted the copy, the map and the buffer and left
                        // this cancel under the instrument. With the witness
                        // off, a copy staged before the boundary would survive
                        // it and the next map would read the OLD world's camera
                        // — half a promotion again, in the same commit that
                        // named the phrase.
                        if (cameraReadbackState_ == CameraReadbackState::COPIED)
                            cameraReadbackState_ = CameraReadbackState::IDLE;
                        // AND THE POSE ITSELF IS NO LONGER TRUE. `valid` was
                        // set once and never cleared, so the tide's "no eye
                        // yet" guard could not fire again and the first sweep
                        // of a new world read the last world's camera. The
                        // CameraPose banner states this rule; nothing enforced
                        // it until now.
                        camera_pose_ = CameraPose{};

                        // Capture return seed + mood + radius before overwrite
                        mood_state_.back_portal_return_seed = world_state_.active_seed;
                        mood_state_.back_portal_return_mood = mood_state_.active;
                        mood_state_.back_portal_return_radius = world_state_.finite_radius;

                        become_destination(pendingDestination_);   // L10 — the one door (ATRIUM_0)

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
                        // THE WHOLE POSE, NOT HALF OF IT (RIBBON_5). POINT_1
                        // authored x and z here; RIBBON_1 added y and heading
                        // to the mirror and this block never grew to match, so
                        // a rebirth carried the DEAD world's altitude and
                        // bearing into the new one until the first live
                        // readback. Heading is not cosmetic: RIBBON_4's
                        // look-ahead reads it — gen_cx = x - cos(heading) x
                        // look — so a stale bearing aims the first frames'
                        // streaming up to PATCH_LOOK_AHEAD wu into a direction
                        // the body is not facing. Both are the spawn pose now,
                        // by the same argument POINT_1 made for x and z.
                        point_.y = 0.0f;
                        point_.heading = 0.0f;
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
                        mood_state_.back_portal_pending     = world_state_.finite_mode;
                        mood_state_.forward_portals_pending = world_state_.finite_mode;
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
                // AUBADE U1 — the piece's own darkness, published for the
                // probe. One store a frame. It reads 0 at boot (no
                // transition runs), and that ZERO IS THE FINDING: the black
                // between init and present is unauthored, so no amount of
                // fade tuning addresses it.
                t7::aubade_fade() = mood_state_.transition_fade_alpha;
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
                // ATRIUM_5 — THE DEFERRED HANG'S ONE PLACEMENT PASS. The
                // atrium's sand image is placed out of band, behind a network
                // fetch, so it can arrive after the patch set has settled and
                // there is nothing left to raise placement_dirty. The gallery
                // holds WorldState const and raises a request instead; this is
                // where the world is writable. U9 is before R16, so the pass
                // it asks for runs THIS frame.
            }

            // U10 — DRIVER BOOKKEEPING (O-5e, dead-last): U1's signal fill
            // consumed the deltas.
            void phase_clear_input_deltas(UpdateCtx&) {
                clear_input_deltas(&input_deps_);
            }

            // ORGAN — the frame boundary: doors, definition re-speaks, the masks,
            // the rule window, the flush. Member functions, in their own file.
            #include "cartridges/the_board/organ_boundary.inc"

            // ── THE CONDUCTOR (update) — a LOOP over UPDATE_SPINE (§1a) ─────
            void update(const AnalysisSignal& signal,
                float aspect_ratio,
                wgpu::Queue& queue) override {
                GPUFrameSignal gpuSignal{};   // the mount block is filled below, with the rest of the signal — one author, one write
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
                    pawnReadbackGen_ = world_state_.world_gen;   // OIL_1c — the issue-time generation, at its machine
                    // OIL_1c: CAPTURELESS by requirement, not by taste. The
                    // wrapper's typed-userdata overload converts the callback
                    // with unary + to a plain function pointer, so a capture
                    // would fail the conversion — loudly, at compile time.
                    // `this` rides the trailing userdata slot; the lambda is
                    // written inside a member function, so it keeps the
                    // class's access rights and reaches privates through
                    // `self` with no friend declaration.
                    gpuState_.agent_state_readback_staging().MapAsync(
                        wgpu::MapMode::Read, 0, GPUState::agent_state_buffer_size(),
                        wgpu::CallbackMode::AllowSpontaneous,
                        [](wgpu::MapAsyncStatus status, wgpu::StringView, Cartridge* self) {
                            if (status == wgpu::MapAsyncStatus::Success) {
                                // Drop stale callbacks from a previous world: the
                                // generation recorded at issue time differs from
                                // world_state_.world_gen if a teardown happened in
                                // between. Buffer is still successfully mapped
                                // though, so we Unmap unconditionally (mapping
                                // contract is independent of whether we read).
                                if (self->pawnReadbackGen_ == self->world_state_.world_gen) {
                                    const auto* data = static_cast<const GPUAgentState*>(
                                        self->gpuState_.agent_state_readback_staging().GetConstMappedRange(
                                            0, GPUState::agent_state_buffer_size()));
                                    if (data) {
                                        std::memcpy(self->agent_state_.slots, data,
                                            GPUState::agent_state_buffer_size());
                                        const auto& p = self->agent_state_.slots[self->player_.possessed_slot];
                                        // THE POINT: point_.x/z is the
                                        // POINT's position — the body authors it
                                        // whenever the body hosts (PAWN, or
                                        // RIBBON — the possessed body rides the
                                        // seat). IN CAMERA-HOST NOTHING AUTHORS IT
                                        // (RIBBON_6): this comment named a "camera
                                        // harvest below" that does not exist
                                        // anywhere in src/, and x/z are simply
                                        // HELD-LAST while the witness hosts —
                                        // which is what PointState's own contract
                                        // says. The two writers are this block,
                                        // gated off for CAMERA, and the teardown's
                                        // authored present. The portal trigger is
                                        // the point's BUBBLE sensor, riding the
                                        // possessed slot's wire in every host.
                                        if (self->point_.host != PointHost::CAMERA) {
                                            self->point_.x = p.pos_x;
                                            self->point_.z = p.pos_z;
                                            // RIBBON_1: y and heading join the
                                            // mirror. possess() captures the EDGE
                                            // from them — where the body was when
                                            // the host changed — and the GPU eases
                                            // the trajectory from there.
                                            self->point_.y = p.pos_y;
                                            self->point_.heading = p.heading;
                                        }
                                        self->point_.portal_trigger = p.portal_trigger;
                                        // ATRIUM_5 — THE PASSER WITNESS, here
                                        // because here is where the route state
                                        // becomes readable: it is written on the
                                        // GPU and this memcpy is the only thing
                                        // that brings it back. One line every
                                        // four seconds, in the atrium only.
                                        // Function-local static — the [Atmos]
                                        // witness's idiom, and the [FLUSH]
                                        // one-shot's before it.
                                        if constexpr (INSTRUMENTS.passer_witness) {
                                            if (self->mood_state_.active == MOOD_ATRIUM) {
                                                static float last_passer_print = -1e9f;
                                                const double now = self->time_state_.seconds;   // PLUMB_0 B1
                                                if (now - last_passer_print >= 4.0f) {
                                                    last_passer_print = now;
                                                    dump_passer_census(self->agent_state_, &self->agents_deps_);
                                                }
                                            }
                                        }
                                    }
                                }
                                self->gpuState_.agent_state_readback_staging().Unmap();
                            }
                            self->pawnReadbackState_ = PawnReadbackState::IDLE;
                        },
                        this);
                }

                //
                if (floaterReadbackState_ == FloaterReadbackState::COPIED) {
                    floaterReadbackState_ = FloaterReadbackState::MAPPING;
                    floaterReadbackGen_ = world_state_.world_gen;   // OIL_1c — see the pawn arm above
                    gpuState_.floating_entity_readback_staging().MapAsync(
                        wgpu::MapMode::Read, 0, GPUState::floating_entity_buffer_size(),
                        wgpu::CallbackMode::AllowSpontaneous,
                        [](wgpu::MapAsyncStatus status, wgpu::StringView, Cartridge* self) {
                            if (status == wgpu::MapAsyncStatus::Success) {
                                // Drop stale callbacks from a previous world.
                                // Buffer is still mapped, so Unmap unconditionally.
                                if (self->floaterReadbackGen_ == self->world_state_.world_gen) {
                                    const auto* data = static_cast<const GPUFloatingEntityState*>(
                                        self->gpuState_.floating_entity_readback_staging().GetConstMappedRange(
                                            0, GPUState::floating_entity_buffer_size()));
                                    if (data) {
                                        // Owner mirror reconciliation (funnels
                                        // live with spheres / cube_behaviors).
                                        if constexpr (ROSTER.sphere)  // ROSTER-GATE sphere (b) — no spheres, no mirror to release
                                            reconcile_sphere_mirror(self->sphere_state_, &self->sphere_deps_, data);
                                        if constexpr (ROSTER.cube)    // ROSTER-GATE cube (b)
                                            reconcile_cube_mirror(self->cube_behaviors_state_, &self->cube_deps_, data);
                                    }
                                }
                                self->gpuState_.floating_entity_readback_staging().Unmap();
                            }
                            self->floaterReadbackState_ = FloaterReadbackState::IDLE;
                        },
                        this);
                }

                // ATRIUM_11 — THE CAMERA WITNESS. Third arm, the two above
                // in grammar and in every guard: the issue-time generation,
                // the stale callback dropped, Unmap unconditional on a
                // successful map. The whole arm compiles out when the
                // witness is unarmed.
                //
                // THE BASE IS Idle::PAWN_HEADING, not point_.heading: the
                // arrival row's azimuth is an offset on the ARRIVAL gaze,
                // which is a constant. A printed offset against the LIVE
                // heading would be a different number from the one the panel
                // takes, which is the one way this instrument could lie.
                //
                // PLUMB_0 C3 — this said "and apply_mood_arrival adds it to
                // exactly this", of a function that was declared and never
                // defined. The base is Idle::PAWN_HEADING because that is the
                // arrival gaze, full stop; there was never an applier reading
                // it back.
                // PLUMB_0 C1 — the map is unconditional; the PRINT is not.
                // The pose lands in camera_pose_ first, so a build with the
                // witness off still has the fact and merely says nothing.
                if (cameraReadbackState_ == CameraReadbackState::COPIED) {
                    cameraReadbackState_ = CameraReadbackState::MAPPING;
                    cameraReadbackGen_ = world_state_.world_gen;
                    gpuState_.camera_readback_staging().MapAsync(
                        wgpu::MapMode::Read, 0, GPUState::camera_state_buffer_size(),
                        wgpu::CallbackMode::AllowSpontaneous,
                        [](wgpu::MapAsyncStatus status, wgpu::StringView, Cartridge* self) {
                            if (status == wgpu::MapAsyncStatus::Success) {
                                // THE STALE-CALLBACK GUARD STAYS ON THE STORE,
                                // not just on the print: a pose mapped for the
                                // world before last is not this world's pose,
                                // and storing it would hand a reader a camera
                                // that no longer exists.
                                if (self->cameraReadbackGen_ == self->world_state_.world_gen) {
                                    const auto* cam = static_cast<const GPUCameraState*>(
                                        self->gpuState_.camera_readback_staging().GetConstMappedRange(
                                            0, GPUState::camera_state_buffer_size()));
                                    if (cam) {
                                        // The four numbers worth keeping; the
                                        // raw struct dies with the mapping,
                                        // as it always did.
                                        self->camera_pose_.eye[0]   = cam->pos[0];
                                        self->camera_pose_.eye[1]   = cam->pos[1];
                                        self->camera_pose_.eye[2]   = cam->pos[2];
                                        self->camera_pose_.azimuth  = cam->azimuth;
                                        self->camera_pose_.elevation = cam->elevation;
                                        self->camera_pose_.valid    = true;
                                        if constexpr (INSTRUMENTS.camera_witness)
                                            dump_camera_orbit(*cam, Idle::PAWN_HEADING,
                                                              (float)self->time_state_.seconds);
                                    }
                                }
                                self->gpuState_.camera_readback_staging().Unmap();
                            }
                            self->cameraReadbackState_ = CameraReadbackState::IDLE;
                        },
                        this);
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
                stream_patches(&machine_ctx_, encoder, queue, tile_world_state_, themes_state_, tile_world_deps_, mood_deps_);
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
            // Autonomous stdout (P6: every switch has a witness).
            // IOS_3 B2 — THE LINE JEAN'S LEAD ASKS FOR, and the only one
            // that can answer it. The hypothesis is that on WebKit the
            // outdoor world never FORMS — no patch ever lands — and a black
            // canvas cannot be told apart from a world that formed and
            // failed to draw. These three counters tell them apart:
            //
            //   allocated  a layer was assigned (the CPU got that far)
            //   baked      the heightfield dispatch ran for it
            //   landed     it is drawable
            //
            // allocated > 0 with baked == 0 says the bake is where it stops.
            // All three healthy on a black screen says the world formed and
            // the RENDER is at fault. Nothing else in the program tells
            // those two apart on a device with no console.
            //
            // ONE EM_ASM PER SECOND, and only with ?bootinfo=1 — it rides
            // the census phase's cadence rather than growing one, and
            // card_live rewrites a single div rather than appending.
            double lastCardTick_ = -1.0;   // PLUMB_0 B1 — differenced against TimeState::seconds

            void card_patch_tick_() {
                if (!t7::boot_params().bootinfo) return;
                if (time_state_.seconds - lastCardTick_ < 1.0f) return;
                lastCardTick_ = time_state_.seconds;
                uint32_t baked = 0, landed = 0;
                const uint32_t n = world_state_.active_patch_count;
                for (uint32_t i = 0; i < n; i++) {
                    const auto& p_ = patch_system_state_.patches_[i];
                    if (!p_.valid) continue;
                    if (p_.phase == PatchPhase::GENERATED) { baked++; landed++; }
                    else if (p_.phase == PatchPhase::SPAWNED) { baked++; }
                }
                t7::card_live("patches: allocated " + std::to_string(n)
                              + "  baked " + std::to_string(baked)
                              + "  landed " + std::to_string(landed));
            }

            void phase_census_dumps(RenderCtx&) {
                card_patch_tick_();
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
                    // TIDY_0c-ii — IS THIS THE FIRST DUMP? lastCensusDump_ is
                    // seeded NEGATIVE (spawn_engine.hpp) so the first census
                    // fires immediately rather than 30 s in. time_state_
                    // .seconds is monotonic and non-negative, so a negative
                    // value here is the sentinel and can be nothing else.
                    const bool first_dump = spawn_engine_state_.lastCensusDump_ < 0.0f;
                    // HEADROOM_0 U3 — THE ENTITY TEXT IS ON ITS OWN DIAL.
                    // ~50 blocking std::cout writes, and the 2026-08-13 boot
                    // read census_dumps max 1051 ms: an instrument spending
                    // over a second inside frames it exists to measure.
                    //
                    // It could not simply be gated off with periodic_census,
                    // because instruments.hpp asserts frame_meter REQUIRES
                    // periodic_census — the [METER] table below rides this
                    // same cadence, so turning the dial off would turn off
                    // the meter. The dial is split instead: the cadence and
                    // the table stay on periodic_census, the TEXT answers to
                    // census_entity_dump. `meter` drops it; `full` keeps it.
                    //
                    // Silence rather than buffering: flushing the text
                    // "outside the frame" moves the same blocking write to
                    // the same thread microseconds later. Not writing it is
                    // both cheaper and a smaller edit.
                    //
                    // The cadence bookkeeping below is NOT gated — it must
                    // advance whether or not the text prints, or the meter
                    // window it drives would never close.
                    if constexpr (INSTRUMENTS.census_entity_dump) {
                        dump_entity_census(&machine_ctx_, "periodic");
                    }
                    spawn_engine_state_.lastCensusDump_ = time_state_.seconds;

                    // THE FRAME METER — the timing census rides the same
                    // cadence. Print ALL enabled rows (completeness feeds
                    // the suspect table; Jean pastes this block back
                    // verbatim). mean = sum/frames; fps = frames/wall.
                    // The dial conjunct is for ELIMINATION, not correctness:
                    // with the meter off nothing increments window_frames, so
                    // the block is already inert — the constant lets the
                    // compiler drop the formatting with it.
                    // TIDY_0c-ii — THE FIRST WINDOW IS NOT A MEASUREMENT.
                    // The seeded lastCensusDump_ fires this dump on frame 1,
                    // and render() increments window_frames at its head, so
                    // the guard below sees 1 and prints a ONE-FRAME table
                    // whose wall clock runs from FrameMeter's construction —
                    // i.e. across the whole boot, pipeline compilation
                    // included. That window reads fps ~0.0, means taken from
                    // a single cold frame, and a residue computed from it.
                    // Pasted into an A/B it is not noise, it is a wrong
                    // number wearing the right format.
                    //
                    // SKIPPED, NOT SILENT (P6): the skip prints its own line,
                    // so a missing first window is never confused with a
                    // meter that failed to arm. reset() restamps
                    // window_start, so the NEXT window is a true 30 s span.
                    if (INSTRUMENTS.frame_meter && first_dump) {
                        char line[160];
                        std::snprintf(line, sizeof line,
                            "[METER] first window SKIPPED — %u frame(s), wall clock spans "
                            "the boot; window starts now\n", meter_.window_frames);
                        std::cout << line;
                        meter_.reset();
                    }
                    else if (INSTRUMENTS.frame_meter && meter_.window_frames > 0) {
                        // WIT_2b — 256, not 160. Pulling dropped_submits out of
                        // this header brought the realistic line back to 149 of
                        // 160, and a 10-byte margin is precisely how the last
                        // one was lost: a hostile window (seven-digit frame
                        // counts, four-digit fps, a wide envelope) still renders
                        // 169. Sizing for the worst case retires the CLASS of
                        // defect instead of this instance of it. A char[320] on
                        // the stack, once per 30 s window, costs nothing worth
                        // counting.
                        //
                        // RIBBON_6 widened this line by two columns (canvas WxH
                        // and the over count) — ~24 more characters at ordinary
                        // values, ~40 at hostile ones. 256 would still have fit;
                        // 320 is taken by the paragraph above's own argument,
                        // whose whole point is not to re-audit the margin every
                        // time a column is added.
                        char line[320];
                        const float wall_s = std::chrono::duration<float>(
                            std::chrono::steady_clock::now() - meter_.window_start).count();
                        const float fps = wall_s > 0.0f
                            ? (float)meter_.window_frames / wall_s : 0.0f;
                        if (meter_gpu_)
                            std::snprintf(line, sizeof line,
                                // TIDY_0d: the gpu columns say what they are.
                                // Both are folded from the PER-FRAME SUM over a
                                // row's pass pairs, not from a single pair — so
                                // a multi-pass row's max can exceed any one pass
                                // it contains. The accumulation site carries the
                                // full note.
                                // WRAP_0 U3 — THE WINDOW DESCRIBES ITSELF. A
                                // capture used to say what it measured and not
                                // what it measured it UNDER, so a mask table
                                // had to be transcribed by hand beside the log
                                // and trusted. These four are the dials that
                                // change what a window means.
                                "[METER] window %uf  fps %.1f  canvas %ux%u  gpu sampled %uf"
                                " | draw=0x%02X shadow=0x%X pcf=%u pace=%u"
                                " | budget %.1f ms | over %uf"
                                " | envelope mean %.2f max %.2f ms -> purse %.2f ms"
                                " | gpu mean/max (per-frame sum)\n",
                                meter_.window_frames, fps,
                                // RIBBON_6: a GPU budget read against an unknown
                                // resolution is not a reading, and the canvas was
                                // the one variable that moved silently between
                                // windows in the recording that opened the round.
                                t7::g_canvas_w, t7::g_canvas_h,
                                meter_.gpu_sampled_frames,
                                gpuState_.config().draw_mask,
                                gpuState_.config().shadow_mask,
                                gpuState_.config().shadow_pcf_taps,
                                t7::g_present_pace,
                                FrameMeter::FRAME_BUDGET_MS,
                                meter_.gpu_over_budget_frames,
                                // HEADROOM_0 U1 — the purse. The envelope is a
                                // per-frame SPAN, so its mean is over sampled
                                // frames, not over the window's frames; and the
                                // purse quotes the MEAN, because a budget spent
                                // against the worst frame is not a budget.
                                meter_.gpu_sampled_frames > 0
                                    ? meter_.gpu_envelope.sum_ms / meter_.gpu_sampled_frames : 0.0,
                                (double)meter_.gpu_envelope.max_ms,
                                FrameMeter::FRAME_BUDGET_MS -
                                    (meter_.gpu_sampled_frames > 0
                                        ? meter_.gpu_envelope.sum_ms / meter_.gpu_sampled_frames : 0.0));
                        else
                            std::snprintf(line, sizeof line,
                                "[METER] window %uf  fps %.1f | budget %.1f ms\n",
                                meter_.window_frames, fps, FrameMeter::FRAME_BUDGET_MS);
                        std::cout << line;
                        // WIT_2b — alone, and unconditional. Never appended to
                        // a formatted line again.
                        t7::print_dropped_submits("window");
                        // PANORAMA_0 §5.5 — THE MESH-GEN FIRING COUNT. One
                        // line, only when something fired: on a still world it
                        // is silent, and on a ride it is the number the
                        // EntityMeshGen row cannot give — how many times a
                        // family regenerated EVERY slot because one slot
                        // changed. Names only the families that fired.
                        {
                            uint32_t fired = 0;
                            for (uint32_t f = 0; f < PopFamily::COUNT; f++)
                                fired += meter_.mesh_gen_firings[f];
                            if (fired > 0) {
                                std::string mg = "[METER] mesh-gen firings";
                                for (uint32_t f = 0; f < PopFamily::COUNT; f++) {
                                    if (meter_.mesh_gen_firings[f] == 0) continue;
                                    mg += "  ";
                                    mg += family_short_name(f);
                                    mg += " ";
                                    mg += std::to_string(meter_.mesh_gen_firings[f]);
                                }
                                mg += "  | total ";
                                mg += std::to_string(fired);
                                mg += " over ";
                                mg += std::to_string(meter_.window_frames);
                                mg += "f\n";
                                std::cout << mg;
                            }
                        }
                        // WRAP_0 U4 — THE SLOT LINE: milliseconds divided by
                        // geometry. `main_pass` is 11-12 ms on Kepler for a
                        // vertex and pixel count that does not explain it, and
                        // the three terrain plan slots are where the vertices
                        // are. n is the VISIBLE INSTANCE count the cull kernel
                        // wrote (its three atomics); I is the slot's index
                        // count — so n x I is the slot's triangles x 3, and the
                        // three products are what the pass is actually drawing.
                        // TERRAIN_0 opens on this line and the mask table.
                        if (slotSampleValid_) {
                            char sl[160];
                            std::snprintf(sl, sizeof sl,
                                "[METER] terrain  A %ux%u  B %ux%u  C %ux%u\n",
                                slotInstances_[0], gpuState_.patch_index_count(),
                                slotInstances_[1], gpuState_.patch_index_count_cap_only(),
                                slotInstances_[2], gpuState_.patch_index_count_lod1_live());
                            std::cout << sl;
                        }
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
                        // THE S BLOCK (OIL_1a) — the host rows, HostRow
                        // order. An S row names where a wait SURFACES,
                        // not where the cost lives (the timer law at the
                        // enum). A frame that failed the acquire noted
                        // nothing, so these means stay consistent with
                        // window_frames (rendered frames only).
                        static constexpr const char* S_NAMES[(size_t)HostRow::COUNT] = {
                            "begin_frame", "acquire", "finish_submit", "present", "frame_total"
                        };
                        double s_partials = 0.0, s_frame_total = 0.0;
                        for (size_t i = 0; i < (size_t)HostRow::COUNT; i++) {
                            const auto& s = meter_.s_rows[i];
                            const double mean = s.sum_ms / meter_.window_frames;
                            if (i == (size_t)HostRow::FrameTotal) s_frame_total = mean;
                            else                                  s_partials += mean;
                            std::snprintf(line, sizeof line,
                                "[METER] S %-22s  mean %.2f  max %.2f\n",
                                S_NAMES[i], mean, (double)s.max_ms);
                            std::cout << line;
                        }
                        std::snprintf(line, sizeof line,
                            "[METER] U_SUM %.2f   R_SUM %.2f\n", u_sum, r_sum);
                        std::cout << line;
                        // The residue — the previously unattributable gap,
                        // now named: what frame_total carries that no U, R,
                        // or S bracket does (input drain, encoder create,
                        // glue). Native backpressure surfaced in acquire/
                        // present above; on the web twin the fps line's
                        // remainder beyond frame_total is the rAF interval.
                        std::snprintf(line, sizeof line,
                            "[METER] residue %.2f  (frame_total %.2f - U_SUM - R_SUM - S_partials %.2f)\n",
                            s_frame_total - (u_sum + r_sum + s_partials),
                            s_frame_total, s_partials);
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
                ribbon_on_dismount(&machine_ctx_, queue);
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
                    // LOOM_2 pass head: WORLD + FRAME are every pipeline's strata 0/1.
                                    { pass.SetBindGroup(0, gpuState_.world_group());
                      pass.SetBindGroup(1, gpuState_.frame_c_group()); }
                    // dispatch skips disabled families structurally:
                    // dirty[f] stays false for a disabled family (never
                    // set above), so this branches on dirty-ness, not on
                    // the enable bit.
                    for (uint32_t f = 0; f < PopFamily::COUNT; f++) {
                        if (!dirty[f]) continue;
                        // PANORAMA_0 §5.5 — one firing, counted where it is
                        // known. Folds to nothing with the dial off.
                        if constexpr (INSTRUMENTS.frame_meter) meter_.mesh_gen_firings[f]++;
                        FAMILY_DISPATCH[f].dispatch_mesh(&machine_ctx_, pass);
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
            //
            // SPINE_2 B — THE DECISION STAYS, THE PASS LEAVES. The write is
            // now the FIRST DISPATCH of R10's pass, not a pass of its own
            // (render_passes.hpp, dispatch_compute). This row still owns the
            // rest law and the witness; what it hands on is a bool, and the
            // consumers still read a written card because dispatch order
            // inside a pass is a visibility rule.
            void phase_live_card_write(RenderCtx& c) {
                (void)c;

                // PULSE_1 — THE RING IS DRAINED AND RETIRED BEFORE THE LAW
                // READS IT, so the rest law sees this frame's truth rather
                // than last frame's. The tap arrives as an intent on the
                // input organ (InputState::pulse_pending) and is spent here,
                // where the clock and the point are both in hand — the drain
                // idiom the analog deltas already use, not a mid-event write.
                if (inputState_.pulse_pending) {
                    inputState_.pulse_pending = false;
                    issue_pulse_from_point();
                }
                retire_aged_pulses();

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
                    liveCardWritePending_ = false;   // at rest, card clean: skip
                    return;
                } else {
                    liveCardRestClean_ = true;       // entering rest: one clearing write
                }

                liveCardWritePending_ = true;
            }

            // R10 — DISPATCH COMPUTE (music+input+algo). The per-frame world-
            // update compute pass (7 dispatches; render_passes.hpp). O-1 by
            // construction: R7's resync writes before this reads (submission
            // order).
            void phase_dispatch_compute(RenderCtx& c) {
                auto& encoder = c.encoder;
                // R8's decision rides in as an argument — the card is this
                // pass's first dispatch when the rest law asks for it
                // (SPINE_2 B). Consumed here, so a frame that never reaches
                // R10 cannot leak the flag into the next one.
                const bool write_card = liveCardWritePending_;
                liveCardWritePending_ = false;
                dispatch_compute(&machine_ctx_, encoder, write_card);
            }

            // R11 — WITNESS CAPTURE (O-2: staging copies AFTER compute; feeds
            // next frame's HARVEST).
            //
            // PLUMB_0 C2 — THE BANNER SAID "the camera copy is CAMERA-HOST
            // ONLY (the pawn-host frame encodes no camera copy; that path
            // stays byte-untouched)" AND THE CODE BELOW HAS NO HOST TEST. It
            // never had one: the only guard was `if constexpr
            // (INSTRUMENTS.camera_witness)`, and that arm is true in every
            // column. The copy runs in every host and always has. Corrected
            // rather than implemented — a host test would now break C1's
            // promise that the pose is available whoever holds the point.
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

                // ATRIUM_11 — the camera witness's 48 bytes, on the same
                // encoder and after the same dispatches. update_camera_vp has
                // written camera_state by now, exactly as it has written the
                // agent buffer above.
                // PLUMB_0 C1 — no longer instrument-gated: the pose is a
                // spine fact and the shipped column already paid for it.
                if (cameraReadbackState_ == CameraReadbackState::IDLE) {
                    encoder.CopyBufferToBuffer(
                        gpuState_.camera_buffer(), 0,
                        gpuState_.camera_readback_staging(), 0,
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
                    //
                    // AND ONLY WHERE A CEILING READS IT (PANORAMA_1). The fit
                    // is what makes ground_y an INPUT to the mesh, and it is
                    // the only thing that does: the kernel's height is
                    //   select(p.height, max(ceiling - ground_y, MIN),
                    //          ceiling_height > 0.0 && tier < ANTENNA)
                    // so outdoors, where set_ceiling_height writes 0, the
                    // false arm keeps p.height and every output byte of the
                    // rebake equals the last bake's. The re-raise was firing
                    // on every corrected frame in an open world and paying a
                    // whole-family regeneration for a result it could not
                    // change — 378 of 498 firings in one 1,300-frame window,
                    // which is what the mesh-gen count was built to see.
                    //
                    // The condition is the SAME FIELD the kernel selects on,
                    // read from the CPU side of the same uniform, so the gate
                    // and the select cannot disagree.
                    if (gpuState_.config().ceiling_height > 0.0f) {
                        for (uint32_t i = 0; i < Dim::MAX_COLUMN_ONLY; i++) {
                            if (entities_state_.columns[i].active) {
                                entities_state_.column_mesh_gen_pending = true;
                                break;
                            }
                        }
                    }
                }
            }

            // ═══ THE PULSE BUS (PULSE_1) ═════════════════════════════
            //
            // ONE RING, MANY WRITERS. contrib_radial_pulses_at never asks who
            // stamped a slot: a thumb-tap and a note-on are the same event to
            // it — a position, a time, an amplitude. The writer today is the
            // player (the glass tap, the SPACE key). A soundtrack onset
            // writer joins by calling emit_radial_pulse and needs nothing
            // else: no second home, no struct change, no shader change.
            //
            // NO DEBOUNCE HERE. Rate-limiting belongs to the input that has a
            // bounce problem, which is the finger, not the bus — a musician
            // firing two notes 40 ms apart is entitled to both, and a bus
            // that swallowed one would be a defect authored on purpose.
            //
            // THE RING IS THE SPINE'S because stamping an onset needs the
            // clock (time_state_.gpu_seconds — the SAME number the shader
            // differences against, PLUMB_0 B2) and the point; the numbers it
            // is shaped by live in terrain_looks, beside the rest pin.
            float    pulseRing_[terrain_looks::PULSE_RING_FLOATS] = {};
            uint32_t pulseWriteIdx_ = 0;
            uint32_t pulseCount_    = 0;   // the count the GPU reads; a rest-law conjunct

            // THE ORIGIN IS THE POINT, ONE FRAME STALE (law E-4). On a
            // wavefront expanding at PULSE_SPEED (30 wu/s) one frame of drift
            // at walking pace is sub-pixel, so no GPU-side stamp and no
            // second readback is bought for it.
            void emit_radial_pulse(float world_x, float world_z, float amplitude) {
                const uint32_t slot = pulseWriteIdx_ % terrain_looks::PULSE_RING_SLOTS;
                const uint32_t base = slot * 4u;
                pulseRing_[base + 0] = world_x;
                pulseRing_[base + 1] = world_z;
                pulseRing_[base + 2] = time_state_.gpu_seconds();
                pulseRing_[base + 3] = amplitude;
                pulseWriteIdx_++;
                // Slots fill 0..N-1 from a zeroed ring, so the count CLIMBS
                // to the ring and stays — never a live-scan, because the
                // shader's own early-exit skips dead slots and
                // retire_aged_pulses returns the whole ring to rest at once.
                // Climbing rather than deriving it from pulseWriteIdx_ is
                // deliberate: the index is unbounded and the count must not
                // follow it back down through a wrap.
                if (pulseCount_ < terrain_looks::PULSE_RING_SLOTS) pulseCount_++;
                // ONE FRAME TO THE GPU. This dirties the config, and
                // upload_config runs at U8 — an UPDATE row, so the write
                // lands at the NEXT frame's upload rather than this one's.
                // The ring is therefore seen ~16 ms after the tap, on top of
                // the point's own frame of staleness (law E-4). Both are
                // imperceptible on a wavefront and neither is worth a second
                // upload path; the CPU-side count is current immediately,
                // which is what the rest law below reads.
                gpuState_.set_pulse_data(pulseCount_, pulseRing_);

                // THE PULSE IS A TEMPORAL CHANGE, so its witness reads a
                // TIMELINE: a grep that finds this call proves the call
                // exists, not that a wavefront ever left the point. Tap in
                // rhythm and the slots must advance and wrap at
                // PULSE_RING_SLOTS, with t rising by the gaps you played.
                //
                // stream_witness, which is the per-EVENT arm: a blocking
                // console write per tap is exactly what `meter` drops by the
                // dial's own doctrine, and the shipped frame is silent.
                if constexpr (t7::INSTRUMENTS.stream_witness) {
                    std::cout << "[Pulse] emit"
                              << " x=" << pulseRing_[base + 0]
                              << " z=" << pulseRing_[base + 1]
                              << " amp=" << pulseRing_[base + 3]
                              << " slot=" << slot
                              << " count=" << pulseCount_
                              << " t=" << pulseRing_[base + 2] << "\n";
                }
            }

            // The verb, as the glass and the keyboard issue it.
            void issue_pulse_from_point() {
                emit_radial_pulse(point_.x, point_.z, PANEL_LIVE.pulse.amplitude);
            }

            // THE COUNT MUST RETIRE, and this is the whole reason:
            // pulse_count > 0 is the FIRST conjunct of live_card_is_live, so
            // a ring that never falls back to rest holds the live-card
            // writer — 819,200 invocations a frame — awake for the rest of
            // the session over a world that has stopped drawing rings. One
            // tap would have cost OPT_1a's rest skip permanently.
            //
            // The liveness test is the shader's, exactly (age < 0 is dead
            // there too, which is the rebase net below saying the same thing
            // twice). One survivor is enough to keep the ring.
            void retire_aged_pulses() {
                if (pulseCount_ == 0u) return;
                const float now = time_state_.gpu_seconds();
                for (uint32_t i = 0; i < pulseCount_; i++) {
                    const float age = now - pulseRing_[i * 4u + 2];
                    if (age >= 0.0f && age <= terrain_looks::PULSE_MAX_AGE
                        && pulseRing_[i * 4u + 3] >= terrain_looks::PULSE_MIN_AMPLITUDE) return;
                }
                clear_pulse_ring();
            }

            // Back to the rest the boot pin writes — one config write on the
            // transition into rest, the live card's own idiom.
            void clear_pulse_ring() {
                if (pulseCount_ == 0u && pulseWriteIdx_ == 0u) return;
                pulseWriteIdx_ = 0;
                pulseCount_    = 0;
                for (float& f : pulseRing_) f = 0.0f;
                gpuState_.set_pulse_data(terrain_looks::REST_PULSE_COUNT, pulseRing_);
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
                render_main_pass(&machine_ctx_, encoder, backbuffer, c.msaaColor, depth, clearColor_, orbs_state_, orbs_deps_);
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
            // foundational spine work). The census (tools/gates/score) audits
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
            // ═══ THE HOST S ROWS (OIL_1a; ledger: S0 host tail, C10) ═══
            // The harness's stations — the frame's previously unmetered
            // tail. The host feeds the meter through ONE narrow
            // dial-gated door (meter_note_host below): one meter, one
            // table, one printer.
            // TIMER LAW: an S row names where a wait SURFACES, not where
            // the cost lives — Begin carries the event pump, Acquire and
            // Present carry swapchain backpressure, FinishSubmit carries
            // command-buffer validation; FrameTotal brackets the whole
            // frame() body, so the census's residue line is the gap no
            // row carries.
            enum class HostRow : uint32_t {
                Begin, Acquire, FinishSubmit, Present, FrameTotal, COUNT
            };

            struct FrameMeter {
                static constexpr float FRAME_BUDGET_MS = 16.6f;   // the named budget
                struct RowStat { double sum_ms = 0.0; float max_ms = 0.0f; };
                RowStat u_rows[(size_t)UPhase::COUNT];
                RowStat r_rows[(size_t)RPhase::COUNT];
                RowStat s_rows[(size_t)HostRow::COUNT];   // the host S rows (OIL_1a)
                uint32_t window_frames = 0;
                std::chrono::steady_clock::time_point window_start =
                    std::chrono::steady_clock::now();   // for fps
                // GPU half (M2): per-row pass ms from timestamp queries,
                // merged into the census when samples exist.
                RowStat r_gpu[(size_t)RPhase::COUNT];
                uint32_t gpu_sampled_frames = 0;
                // HEADROOM_0 U1 — THE ENVELOPE. Per frame,
                // max(pair end) - min(pair begin) over every resolved pair.
                // NOT a row: it is not a phase, and folding it into r_gpu
                // would put it in a table whose column header says
                // "per-frame sum", which is the one thing it is not.
                //
                // WHY IT IS THE PURSE. §4b forbids summing brackets, and it
                // is right to: pairs overlap, so a sum over-counts and a
                // single row under-counts. The envelope does neither. It is
                // the wall-span of the frame's GPU work, and it BOUNDS
                // occupancy from above regardless of how the brackets lie
                // inside it — additive-safe by construction, because it
                // never adds anything. budget 16.6 - envelope is the
                // headroom the coupling era has to spend.
                //
                // Pure arithmetic over timestamps already resolved for the
                // rows. Zero new queries, zero new GPU cost.
                RowStat gpu_envelope;
                uint32_t gpu_over_budget_frames = 0;   // RIBBON_6
                // Snapshot of the armed pair table, taken at frame close
                // and consumed by the mapped callback. NOT zeroed by
                // reset() — a readback may be in flight across a window.
                MeterPair snap_pairs[GPUState::meter_max_pairs()] = {};
                uint32_t snap_pair_count = 0;
                // PANORAMA_0 §5.5 — HOW OFTEN A FAMILY REGENERATES ITS MESH.
                // The EntityMeshGen row already says what a firing COSTS; it
                // cannot say how many there were, and the two questions have
                // different answers on a ride, where spawns and evictions keep
                // the dispatch firing. This is the instrument that gates the
                // per-slot regeneration campaign (F3) and proves it after: the
                // count should fall to the number of slots that actually
                // changed, and the row's ms with it.
                uint32_t mesh_gen_firings[PopFamily::COUNT] = {};
                void reset() {   // zero rows + frames, restamp window_start
                    for (auto& s : u_rows) s = RowStat{};
                    for (auto& s : r_rows) s = RowStat{};
                    for (auto& s : s_rows) s = RowStat{};
                    for (auto& s : r_gpu) s = RowStat{};
                    gpu_envelope = RowStat{};
                    gpu_over_budget_frames = 0;
                    window_frames = 0;
                    gpu_sampled_frames = 0;
                    for (auto& n : mesh_gen_firings) n = 0;
                    window_start = std::chrono::steady_clock::now();
                }
            };
            FrameMeter meter_;

            // The host door (OIL_1a): the harness clocks its own brackets
            // and notes the ms here. Dial off: the body folds to an empty
            // inline — the same zero-fold standard the ledger verified
            // for the conductors' clock pairs (X meter-off fold, CLEAN).
            void meter_note_host(HostRow row, float ms) {
                if constexpr (!INSTRUMENTS.frame_meter) { (void)row; (void)ms; return; }
                auto& s = meter_.s_rows[(size_t)row];
                s.sum_ms += ms; if (ms > s.max_ms) s.max_ms = ms;
            }

            // The meter_row registry (state.hpp — the GPU half's raw row
            // ids) is pinned to RPhase HERE, at the enum's home. Drift
            // fails glaw1.
            static_assert(meter_row::StreamPatches       == (uint32_t)RPhase::StreamPatches,       "meter_row drift: StreamPatches");
            static_assert(meter_row::EntityMeshGen       == (uint32_t)RPhase::EntityMeshGen,       "meter_row drift: EntityMeshGen");
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
            // E-3 (sky write-order) died with the sky block (RIBBON_1): the
            // signal has one author and one whole-struct write again.
            static_assert((uint32_t)UPhase::ClearInputDeltas + 1 == (uint32_t)UPhase::COUNT, "O-5e: clear_input_deltas is dead-last");
            // render laws:
            static_assert((uint32_t)RPhase::RibbonTick < (uint32_t)RPhase::DispatchCompute, "O-1: the ribbon's state write precedes the compute that reads it");
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
                wgpu::TextureView msaaColor,
                wgpu::TextureView depth) override {
                RenderCtx ctx{ encoder, queue_, backbuffer, msaaColor, depth };   // OIL_1 U1: the cached queue — no per-frame GetQueue
                if constexpr (INSTRUMENTS.frame_meter) meter_.window_frames++;

                // THE DRAW LEDGER'S FRAME BOUNDARY (BUNDLE_1). Every count
                // the CPU authors is staged here and flushed before a single
                // pass is encoded — the update spine has run, so the numbers
                // are this frame's, and a WriteBuffer issued now lands ahead
                // of the command buffer on the queue timeline. flush writes
                // only what moved; a steady frame writes nothing.
                stage_draw_ledger(&machine_ctx_, orbs_state_);
                gpuState_.flush_draw_ledger(queue_);

                // THE BUNDLES (BUNDLE_1) — recorded here, before the passes
                // that execute them, and only when something a bundle
                // CAPTURED moved: a recreated bind group or buffer (R-B
                // found none post-boot) or a subtraction-mask dial. Not per
                // frame — that is the whole point. The ledger is flushed
                // first because a bundle must capture a buffer that exists;
                // its CONTENTS are read at execution, not at recording.
                //
                // AUBADE U3 — A BUNDLE IS IMMUTABLE, SO AN ARRIVAL DIRTIES
                // IT. Pipelines resolve asynchronously; a bundle recorded
                // while a row's pipeline was still null has that row
                // missing FOREVER, because Finish() seals the recording.
                // take_pipeline_arrival is one integer compare a frame and
                // returns true once per successful arrival, so the bundle
                // is re-recorded exactly as often as the world gains a row
                // and not once more.
                if (renderer_.take_pipeline_arrival())
                    gpuState_.raise_bundles_dirty();
                if (gpuState_.bundles_dirty())
                    record_bundles(&machine_ctx_, orbs_state_, orbs_deps_);

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
                    if (slotReadbackState_ == MeterReadbackState::COPIED) {
                        slotReadbackState_ = MeterReadbackState::MAPPING;
                        gpuState_.frustum_count_readback().MapAsync(
                            wgpu::MapMode::Read, 0, GPUState::frustum_indirect_size(),
                            wgpu::CallbackMode::AllowSpontaneous,
                            [](wgpu::MapAsyncStatus status, wgpu::StringView, Cartridge* self) {
                                if (status == wgpu::MapAsyncStatus::Success) {
                                    const auto* a = static_cast<const uint32_t*>(
                                        self->gpuState_.frustum_count_readback().GetConstMappedRange(
                                            0, GPUState::frustum_indirect_size()));
                                    if (a) {
                                        // instanceCounts at 1 / 6 / 11 — the three
                                        // 5-u32 draw-arg slots (state.hpp's
                                        // reset_frustum_indirect names the layout).
                                        self->slotInstances_[0] = a[1];
                                        self->slotInstances_[1] = a[6];
                                        self->slotInstances_[2] = a[11];
                                        self->slotSampleValid_ = true;
                                    }
                                    self->gpuState_.frustum_count_readback().Unmap();
                                }
                                self->slotReadbackState_ = MeterReadbackState::IDLE;
                            }, this);
                    }
                    if (meterReadbackState_ == MeterReadbackState::COPIED) {
                        meterReadbackState_ = MeterReadbackState::MAPPING;
                        gpuState_.meter_readback_staging().MapAsync(
                            wgpu::MapMode::Read, 0, GPUState::meter_readback_size(),
                            wgpu::CallbackMode::AllowSpontaneous,
                            // OIL_1c: captureless, `this` on the userdata slot.
                            // No generation member here — timing rows are
                            // world-agnostic, so this machine never carried one.
                            [](wgpu::MapAsyncStatus status, wgpu::StringView, Cartridge* self) {
                                if (status == wgpu::MapAsyncStatus::Success) {
                                    const auto* ts = static_cast<const uint64_t*>(
                                        self->gpuState_.meter_readback_staging().GetConstMappedRange(
                                            0, GPUState::meter_readback_size()));
                                    if (ts) {
                                        // METER_1.1: group pair dts by row into a
                                        // frame-local total FIRST, then fold sum/max
                                        // from the per-frame totals — multi-pass rows
                                        // (orb quartet, shadow atlas, patch batches)
                                        // otherwise printed a per-pass max below their
                                        // per-frame mean.
                                        double frame_ms[(size_t)RPhase::COUNT] = {};
                                        // HEADROOM_0 U1 — the envelope's extremes,
                                        // gathered in the SAME loop and under the
                                        // SAME discard law, so a pair rejected from
                                        // the rows cannot widen the envelope either.
                                        uint64_t env_lo = 0, env_hi = 0;
                                        bool env_any = false;
                                        for (uint32_t p = 0; p < self->meter_.snap_pair_count; p++) {
                                            const uint64_t t0 = ts[self->meter_.snap_pairs[p].begin_idx];
                                            const uint64_t t1 = ts[self->meter_.snap_pairs[p].begin_idx + 1];
                                            if (t1 <= t0) continue;               // counter-reset garbage
                                            const double ms = (double)(t1 - t0) * 1e-6;   // u64 ns per index
                                            if (ms > 100.0) continue;             // same discard law
                                            frame_ms[self->meter_.snap_pairs[p].row] += ms;
                                            if (!env_any) { env_lo = t0; env_hi = t1; env_any = true; }
                                            else {
                                                if (t0 < env_lo) env_lo = t0;
                                                if (t1 > env_hi) env_hi = t1;
                                            }
                                        }
                                        if (env_any) {
                                            // One number per frame: the span from the
                                            // earliest begin to the latest end. Folded
                                            // like a row so the window reports it with
                                            // the same mean/max grammar, but it is a
                                            // SPAN, not a sum -- and the print says so.
                                            const double env_ms = (double)(env_hi - env_lo) * 1e-6;
                                            auto& e = self->meter_.gpu_envelope;
                                            e.sum_ms += env_ms;
                                            if ((float)env_ms > e.max_ms) e.max_ms = (float)env_ms;
                                            // RIBBON_6: the direct measure of what
                                            // [PRESENT] sees downstream. A frame
                                            // whose GPU span exceeded the refresh
                                            // budget is a frame the panel may have
                                            // shown twice; the mean and max cannot
                                            // say how MANY, and the count can.
                                            if (env_ms > FrameMeter::FRAME_BUDGET_MS)
                                                self->meter_.gpu_over_budget_frames++;
                                        }
                                        // THE MAX ASYMMETRY (TIDY_0d, present
                                        // behavior). The discard above is
                                        // PER-PAIR; the max folded below is over
                                        // the PER-FRAME SUM. So a frame arming
                                        // many pairs on one row can post a max
                                        // that no individual pair could produce,
                                        // and no discard applies to the sum at
                                        // any size. WEB_METER_0 saw exactly this
                                        // twice — stream_patches gpu max 1323.04
                                        // and 1198.06, both in boot-adjacent
                                        // frames that upload many patches. The
                                        // asymmetry is deliberate: the per-frame
                                        // sum is the honest per-frame cost of a
                                        // multi-pass row (METER_1.1 above), and
                                        // the per-pair discard is what keeps
                                        // counter-reset garbage out of it. The
                                        // window header names the column so the
                                        // number is not misleading on its face.
                                        for (size_t r = 0; r < (size_t)RPhase::COUNT; r++) {
                                            if (frame_ms[r] <= 0.0) continue;
                                            auto& s = self->meter_.r_gpu[r];
                                            s.sum_ms += frame_ms[r];
                                            if ((float)frame_ms[r] > s.max_ms) s.max_ms = (float)frame_ms[r];
                                        }
                                        self->meter_.gpu_sampled_frames++;
                                    }
                                    self->gpuState_.meter_readback_staging().Unmap();
                                }
                                self->meterReadbackState_ = MeterReadbackState::IDLE;
                            },
                            this);
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
                    // U4 — the plan's counters, this frame's. The cull pass
                    // reset and rewrote them earlier in this same encoder, so
                    // the copy takes the frame it was encoded in.
                    if (meter_gpu_ && slotReadbackState_ == MeterReadbackState::IDLE) {
                        encoder.CopyBufferToBuffer(
                            gpuState_.frustum_compute_buffer(), 0,
                            gpuState_.frustum_count_readback(), 0,
                            GPUState::frustum_indirect_size());
                        slotReadbackState_ = MeterReadbackState::COPIED;
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
            // (SEAM[spine:transitions], L38 — assembly only; this is declared orchestration). The force-spawn
            // mutation belongs to the arch's owner: entities'
            // force_spawn_portal_arch (the ROSTER portal door lives
            // there). The lighting-scheme tables stay impl-side. See §1.

        public:

            // RIBBON_2 P0 1.2b: an updated-but-unrendered frame adds its dt to
            // the next rendered one — a dropped acquire stretches a step,
            // never deletes it. The host calls this once the frame's command
            // buffer is submitted, which is the only moment the GPU is known
            // to have been given the time this accumulator was holding.
            void frame_submitted() { dtPending_ = 0.0f; }

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
                // SHIP_1 — the touch doors. Each lands on the organ its
                // mouse/key sibling lands on; the console already
                // resolved which gesture this was.
                case InputEvent::Type::TouchMove:
                    on_touch_move(&input_deps_, event.x, event.y);
                    break;
                case InputEvent::Type::TouchLook:
                    on_touch_look(&input_deps_, event.x, event.y);
                    break;
                case InputEvent::Type::TouchZoom:
                    on_touch_zoom(&input_deps_, event.y);
                    break;
                case InputEvent::Type::TouchTapLeft:
                    on_touch_tap_left(&input_deps_, pawn_state_, pawn_deps_);
                    break;
                case InputEvent::Type::TouchTapRight:
                    on_touch_tap_right(&input_deps_, agent_state_, agents_deps_);
                    break;
                case InputEvent::Type::TouchTapPulse:
                    on_touch_tap_pulse(&input_deps_);
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
// contracts. What remains below is the
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
