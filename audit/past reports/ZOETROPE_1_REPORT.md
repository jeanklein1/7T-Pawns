# ZOETROPE_1 — THE CUBE LATTICE RECON LEDGER

**READ-ONLY. Zero builds. Verified at master `d785fd9`.**

**METHOD.** Every load-bearing claim was read from source directly by the
recon author; a parallel agent sweep (R1–R4) ran as a second pass, and every
anchor cited below was then re-read by hand against the live tree — the
adversarial verify tier of the sweep died on a usage ceiling, so the hand
re-read IS the verification of record. Inferences are marked `[INFERRED]`.
Absences are stated as findings.

**DOC HOME.** The handoff asked for the ledger "beside the terrain compute
ledger — same directory, same naming grammar." The terrain compute ledger is
`audit/LEDGER_1_REPORT.md` (its own title: "LEDGER_1 — THE TERRAIN COMPUTE
LEDGER"), so this document lands at **`audit/ZOETROPE_1_REPORT.md`** under the
same `<CAMPAIGN>_REPORT.md` grammar.

**SCOPE NOTE.** The scoped roots are `src/cartridges/the_board/**` and
`src/incubator_dual.cpp`. R4/R5/R6/R7 name anchors that live outside those
roots by construction — the coupling layer (`src/coupling/`), the analysis
side (`src/analysis/`, `src/musical/`, `src/sources/`) — and those files were
READ because the questions name them, not silently annexed. No source file
was modified. The JSX designer tools were not read.

---

## §0 DESIGN FRAME (copied verbatim from the handoff)

  1. One permanent logical lattice owns the floating cubes. F4/F5/F6 are
     embeddings of the same state; F6 is the embedding where logical
     adjacency becomes physical adjacency (the ring). The automaton never
     stops, including during transit.
  2. Lattice = 7 rows (mode degrees) × floor(N_max/7) columns. The lattice
     is the population ceiling's one home.
  3. Per cell, two fields: fast excitation (diffuses one cell per tick,
     logical neighbors only, fixed-seed asymmetric weights) and slow
     pigment (integrates excitation, long half-life). Diffusion ticks on
     the musical clock; decay is evaluated analytically per frame.
     Deterministic: same MIDI, same screen.
  4. Writer: note-on strikes cell (row = mode degree of pitch, column =
     cell under the continuously sweeping write head); velocity sets
     excitation; channel selects pigment family. Listener set {7} at
     birth, mutable, panel-native, expressed in the tree's native
     channel-designation idiom.
  5. The GPU is a projector only: zero new draws, zero new bindings, zero
     new WGSL branches. Cell state is CPU-resident; per-cube color rides
     the existing instance stream. Any instance-struct growth is a GROWTH
     LAW event: same commit, same position, same type, sizeof witness.
  6. Ghost law: eviction leaves the cell's state evolving unrendered;
     spawn inherits the ghost's accumulated state.
  7. Formation anchors to the pawn's live position (anchor law). All
     transitions are trajectories; nothing teleports.
  8. Every constant this feature owns is born panel-native,
     coupling-eligible, one home.

---

## §1 R1 — F-KEYS

### The handler sites, verbatim

`src/cartridges/the_board/direction/input.hpp:256-259` (the "Diagnostics
(function keys)" arm of `on_key_down`):

```cpp
    case GLFW_KEY_F4: cycle_cube_behavior_override(cube_behaviors_state, &cube_deps, q);   break;
    case GLFW_KEY_F5: cycle_floater_coordination(cube_behaviors_state, &cube_deps);        break;
    case GLFW_KEY_F6: corral_cubes(cube_behaviors_state, &cube_deps, q);                   break;
    case GLFW_KEY_F7: toggle_cube_kite_mode(cube_behaviors_state, &cube_deps, q);          break;
```

F7 is not named by the handoff but is load-bearing for the anchor-law audit
below (it flips the kite/anchor mode the ring's tracking depends on).

**FRAME MISMATCH, stated up front:** the design frame's premise — "F4/F5/F6
are embeddings of the same state" — does not match the tree. Only F6 is a
formation command. F4 and F5 are not spatial embeddings of anything:

- **F4 = `cycle_cube_behavior_override`** (`bodies/cube_behaviors.hpp:250-255`)
  — steps `behavior_override = (behavior_override + 1) % CUBE_BEHAVIOR_COUNT`
  through {`stationary`, `curlfield`, `phasewave`} (IDs at :40-47) and, via
  `apply_cube_behavior_override` (:243-248), uploads the 4-byte
  `behavior_id` into every active cube slot
  (`upload_cube_behavior_id`, `realization/state.hpp:2169-2173`). It selects
  the per-cube FORCE function, not a placement.
- **F5 = `cycle_floater_coordination`** (`bodies/cube_behaviors.hpp:236-241`)
  — steps `coordination_step` 0→1→2 through
  `FLOATER_COORDINATION_STEPS[3] = { 0.0f, 0.5f, 1.0f }` (:66) and stages the
  scalar into the design config:
  `stage_floater_coordination` (`state.hpp:2618`) writes
  `config_.floater_coordination` (field at `state.hpp:507`; WGSL mirror
  `world.wgsl:1591` — "[0,1] cube behavior synchrony knob"). Deliberately NOT
  dirty-flagged — the poke idiom banner `state.hpp:2596-2602`: the value
  rides "the next dirty/dynamic full upload"; the drain is `upload_config`
  (`state.hpp:2059-2063`). Its sole GPU consumer is the behavior-force
  dispatch (`world.wgsl:8313-8314`); inside the two behaviors it lerps
  per-cube decorrelation toward population-wide synchrony
  (curlfield `world.wgsl:8091`, phasewave `:8124`). A synchrony dial, not an
  embedding.
- **F6 = `corral_cubes`** (`bodies/cube_behaviors.hpp:257-298`) — the one
  formation command. Computes, once per press, a ring of radius
  `CUBE_CORRAL_RADIUS = 120.0f` (:62) and writes each active cube a TARGET
  ONLY through `upload_cube_glide_target` (`state.hpp:2186-2191`, an 8-byte
  partial write at struct offsets 200/204). The kernel does all movement:
  `let glide_k = 1.0 - exp(-dt / CUBE_GLIDE_TAU);` (`world.wgsl:8240`,
  `CUBE_GLIDE_TAU = 1.1` at :2010).
- **F7 = `toggle_cube_kite_mode`** (`bodies/cube_behaviors.hpp:302-323`) —
  writes only the `follow_pawn` sentinel (3u kite-capture / 2u kite-release)
  via `upload_cube_follow_pawn` (`state.hpp:2181-2185`); the kernel performs
  the capture/release from the true present (`world.wgsl:8212-8228`).

### Does F6 assign stable per-cube slots, and by which index?

No — not stable. The ring index `k` is a dense counter over active slots in
ascending slot order at press time (`cube_behaviors.hpp:277-293`:
`theta = (float(k) / float(active_count)) * two_pi`, `k` incremented only on
active slots). Any eviction or spawn below a cube's slot shifts its ring
angle at the NEXT press. The underlying buffer slot `i` IS stable for a
cube's whole lifetime — allocation is a first-free linear scan
(`machine/spawn_engine.hpp:237-242`), nothing compacts or moves slots, and
the same index addresses mirror and GPU slot everywhere — but F6 does not
key the ring on it; it keys on active-set enumeration order.

### What F6's placement anchors to (anchor-law audit)

`corral_cubes` reads `c->point_.x / c->point_.z`
(`cube_behaviors.hpp:258-262`) — THE POINT's position mirror
(`contracts/point.hpp:93-104`, "host-authored"). That mirror is refreshed
EVERY FRAME from GPU readback at witness harvest: pawn/ribbon host from the
possessed agent slot (`cartridge.hpp:1124-1137`, `point_.x = p.pos_x;`),
camera host from the camera state (`cartridge.hpp:1198-1201`). It is a live
mirror ~1 frame stale — `surface/patch_system.hpp:379`: "THE POINT (1-frame
stale by law E-4)".

So: **F6 anchors to the live (one-frame-stale) pawn position at press
time.** What happens AFTER the press splits by mode:

- anchor mode (`kite_mode == false`, the default): targets are ABSOLUTE ring
  positions `px + cos(theta)·R` (:287-288) — the ring forms around where the
  point was at the press and does NOT track the pawn afterward.
- kite mode (F7 ON): targets are ring OFFSETS (:284-285) and the kernel's
  kite arm recomputes `home.xz = point_pos().xz + pawn_offset.xz` every
  frame (`world.wgsl:8284-8291`) — the ring tracks the point continuously.

The design frame's "formation anchors to the pawn's live position" is
therefore only continuously true in kite mode; in the default mode it is
press-snapshot-of-a-live-mirror. §11 ruling 5.

### Any cube logic branching per mode beyond target selection?

CPU: `behavior_override` is never branched on (only incremented and
uploaded); `kite_mode` branches select target form at corral (:283) and
spawn-init arm at `cube_write_gpu` (:509-526, "BIRTH INTO THE LIVE MODE").
WGSL (`update_cube`, entry `world.wgsl:8141-8142`, single-thread
`@workgroup_size(1)`): three sites are genuinely beyond target/force
selection —

1. the two sentinel state machines, `fe.follow_pawn == 2u` (:8212-8221) and
   `== 3u` (:8222-8228) — in-kernel mode transitions mutating
   anchor/offset/target/drift;
2. the home computation `if (fe.follow_pawn != 0u)` (:8284-8302) — the two
   arms query GROUND AT DIFFERENT XZ (kite arm at `kite_xz`, anchor arm at
   live `fe.pos.xz` per RULING 1); the comment at :8267-8278 documents the
   resulting vertical step on mid-drift toggles ("that is ANCHOR_2, not this
   edit");
3. the plasticity leak `if (fe.follow_pawn == 0u)` (:8421-8437) — runs in
   anchor mode ONLY ("(3) anchor mode only — in kite mode home tracks the
   point, not anchor, so the anchor is dormant", :8418-8419).

Plus the behavior-force switch `switch (fe.behavior_id)`
(:8132-8139) — force selection. These are ZOETROPE_2's per-mode logic
inventory (→ §9 H6).

---

## §2 R2 — POPULATION

### The constant

`realization/state.hpp:323-325`:

```cpp
            // 256 slots: drone-show populations. update_cube is single-thread
            // (@workgroup_size(1)) — cost scales linearly, ~7.5K ops/frame.
            constexpr uint32_t MAX_CUBE_INSTANCES = 256;
```

**True name `Dim::MAX_CUBE_INSTANCES`, value 256. 256 mod 7 = 4** (7×36 =
252, remainder 4). Companions: `CUBE_SLOT_OFFSET = MAX_SPHERE_INSTANCES`
(= 8, :326) and `TOTAL_FLOATING_SLOTS = MAX_SPHERE_INSTANCES +
MAX_CUBE_INSTANCES;  // 264` (:327).

### Reader census — `\bMAX_CUBE_INSTANCES\b` (live code; docs/`external`/`tools` excluded)

| path:line | role |
|---|---|
| `realization/state.hpp:325` | definition (256) |
| `realization/state.hpp:327` | derives `TOTAL_FLOATING_SLOTS` (264) |
| `bodies/cube_behaviors.hpp:150` | CPU mirror array size `ActiveCube activeCubes_[...]` |
| `bodies/cube_behaviors.hpp:229` | loop bound — `clear_cubes` teardown |
| `bodies/cube_behaviors.hpp:244` | loop bound — `apply_cube_behavior_override` (F4) |
| `bodies/cube_behaviors.hpp:265` | loop bound — `corral_cubes` active-count scan (F6) |
| `bodies/cube_behaviors.hpp:279` | loop bound — `corral_cubes` ring-target loop (F6) |
| `bodies/cube_behaviors.hpp:315` | loop bound — `toggle_cube_kite_mode` (F7) |
| `bodies/cube_behaviors.hpp:422` | `CUBE_TRAITS.max_instances` (spawn slot-scan bound) |
| `bodies/cube_behaviors.hpp:584` | loop bound — `reconcile_cube_mirror` readback reconcile |
| `machine/entity_pipeline.hpp:1152` | `T7_GATE_PIN(CUBE_TRAITS, ...)` static_assert pin |
| `realization/renderer.hpp:790` | main draw instance count (`draw_monolith`) |
| `realization/renderer.hpp:1036` | shadow draw instance count (`draw_shadow_monolith`) |

GPU twins (`world.wgsl:1979-1985`, mirror comment-enforced only — no
static_assert crosses the language gap):

```wgsl
const SPHERE_SLOT_COUNT: u32 = 8u;
const CUBE_SLOT_OFFSET: u32 = 8u;
const CUBE_SLOT_COUNT: u32 = 256u;
```

plus the hard-coded array length `entities: array<FloatingEntityState, 264>`
(`world.wgsl:995`) and the `update_cube` loop range (:8161-8162).
`CUBE_SLOT_OFFSET` readers: `state.hpp:2162/2170/2182/2187` (upload offsets),
`cube_behaviors.hpp:479` (`fe.entity_seed = CUBE_SLOT_OFFSET + inst.slot`),
`:585` (readback index), `renderer.hpp:790/1036` (firstInstance),
`world.wgsl:1984/8161-8162`. `TOTAL_FLOATING_SLOTS` readers (all buffer
sizing): `state.hpp:2801` (`floating_entity_buffer_size()`), `:3138`
(creation), `:3168` (readback staging), `:5098/:5170/:5467` (bind-group entry
sizes — compute, render, photographer), `:6106` (boot zero-init).

### Spawn and evict sites

Spawn: the `FAMILY_DISPATCH` cube row (`cartridge.hpp:2176-2180`):

```cpp
            { dispatch_select_cube_generic, dispatch_place_cube_generic, dispatch_commit_cube_generic,
              evict_cube, dispatch_prepare_mesh_none, dispatch_mesh_gen_none,
              active_count_cube,
              CUBE_TRAITS.grounded,      // false — hovers and drifts, claims no ground
              "cube" },  // no CPU mesh gen — GPU compute handles update_cube
```

Chain: funnels (`cube_behaviors.hpp:558-576`) → `generic_select` →
`cube_run_gate` (:432-434) → `gate_from_traits`
(`spawn_engine.hpp:270-279`) → `run_spawn_preamble` (:203-249): idempotency
= at most ONE cube per trigger patch (:213-220); chance composition
`compose_spawn_chance` (:831-851) = mood × global × tile × proximity, MIN1
clamp — currently mood all 1.0 (`population_themes.hpp:38-43` cube column),
`GLOBAL_ENTITY_DENSITY = 1.0f` (`contracts/spawn_services.hpp:62`), cube
proximity off; free-slot scan first-free (:237-242) — **saturation is a
silent skip** (`if (slot == UINT32_MAX) return r;`, no diagnostic). The live
count is never stored: `active_count_cube` is a `.active` scan
(`cartridge.hpp:2114`, `census_scan_active` :2098-2103).

Evict:
- CPU evictor `evict_cube` (`cube_behaviors.hpp:327-332`); its only
  address-taker is the dispatch row, and the `evict_patch_entities` route
  (`patch_system.hpp:53`) is **dead for cubes in practice** — commit never
  records an entity_ref on the host patch: "Lifecycle Phase 2: cube lifetime
  decoupled from host patch" (`cube_behaviors.hpp:572-573`; rationale
  spelled out at `spheres.hpp:227-233`).
- **The live eviction path is GPU-side**: `update_cube`,
  `world.wgsl:8171-8175` — point-distance test against
  `FLOATER_EVICTION_RADIUS = 800.0` (:7547) sets `is_active = 0u`.
- Mirror release: `reconcile_cube_mirror` (`cube_behaviors.hpp:581-592`)
  after `SPAWN_PROTECTION_S = 0.10f` (`contracts/floaters.hpp:46`); fed by
  the floater readback (`cartridge.hpp:1147-1173`).
- Rollback frees on failed place/commit (`cube_behaviors.hpp:566/:575`);
  teardown `clear_cubes` (:228-234), called at world teardown
  (`cartridge.hpp:946`).

### Can the live count reach the ceiling?

- `CubeConfig::SPAWN_CHANCE = 0.60f` (`contracts/floaters.hpp:100`); one
  cube max per patch; streamed patch supply `MAX_ACTIVE_PATCHES = 289`
  (17×17 pregen, `state.hpp:79-81`).
- Stationary: 289 × 0.60 ≈ **~173 expected live cubes** — under the cap.
- Roaming: cubes outlive their patches (above) and die only 800 wu from the
  point (`world.wgsl:7547`); the eviction disk spans ~16 patch-widths versus
  the 400-wu spawn frontier — the 16:1 ratio is documented in THE SCALE
  LEDGER (`world.wgsl:2152-2166`: "floater eviction ...
  FLOATER_EVICTION_RADIUS 800 wu" beside "patch ... PATCH_EXTENT 50 wu"). A
  roaming player accrues ~0.6 cubes per newly streamed patch while shedding
  none until 800 wu behind. `[INFERRED]` sustained roaming can reach and
  saturate 256, after which spawning silently no-ops.
- Design intent agrees: "256 slots: drone-show populations" (`state.hpp:323`).

This is a ceiling report; nothing binds to it until ZOETROPE_2.

---

## §3 R3 — INSTANCE STREAM (the GROWTH-LAW witness-in-advance)

### The struct

`GPUFloatingEntityState` — `realization/state.hpp:829-881`,
`struct alignas(16)`, **sizeof 208 (13×16)**, pinned by
`static_assert(sizeof(GPUFloatingEntityState) == 208, ...)` and the
target_x/target_z offset asserts (`state.hpp:1580-1582`). Note: the struct
head carries NO growth-law banner of its own — the GROWTH LAW lives at
`src/docs/LAWS.md` L5 and is cited at the `GPUDesignConfig` head
(`state.hpp:435-438`); for THIS struct the discipline is the sizeof witness
plus the byte-for-byte WGSL mirror (L3, `world.wgsl:17-20`).

Full field list (types and byte offsets as written):

```cpp
        struct alignas(16) GPUFloatingEntityState {
            float pos[3];              //   0: world position (computed by GPU)
            float body_radius;         //  12: bounding/visual radius
            float orientation[4];      //  16: quaternion
            float influence_radius;    //  32: zone/terrain influence range
            float t;                   //  36: curve parameter
            float orbit_radius;        //  40: distance from anchor (orbit mode)
            float orbit_speed;         //  44: angular velocity (orbit mode)
            float color[3];            //  48: current appearance (coupling-driven)
            float orbit_height;        //  60: base altitude above terrain
            float anchor[3];           //  64: world anchor point
            float face_variance;       //  76: per-face color spread (monolith)
            float base_color[3];       //  80: seed-derived rest color
            uint32_t geometry_type;    //  92: 0=sphere, 1=monolith
            uint32_t motion_type;      //  96: 0=orbit, 1=hover-bob
            float spin_speed;          // 100: Y-axis rotation rate (hover-bob)
            float bob_amplitude;       // 104: vertical oscillation amplitude
            float bob_period;          // 108: vertical oscillation period
            float spin_tilt_x;         // 112: axis tilt X
            float spin_tilt_z;         // 116: axis tilt Z
            uint32_t entity_seed;      // 120: seed for VS face color hashing
            uint32_t is_active;        // 124: 0=inactive, 1=active
            float aspect_y;            // 128: Y-axis scale multiplier (1.0=cube, >1=tall, <1=flat)
            float aspect_z;            // 132: Z-axis scale multiplier (1.0=cube, <1=thin slab)
            float spring_stiffness;    // 136: pulls drift toward zero (1/s²)
            float drag;                // 140: exponential damping on drift_vel (1/s)
            float drift[3];            // 144: position offset from home (cube)
            uint32_t tier_idx;         // 156: runtime tier lookup for gain tables
            float drift_vel[3];        // 160: drift integrator velocity
            uint32_t behavior_id;      // 172: cube behavior registry index (Phase 3)
            float    pawn_offset[3];   // 176/180/184: cube position relative to pawn
            uint32_t behavior_phase;   // 188: per-slot phase hash for behavior diversity
            uint32_t follow_pawn;      // 192: 0=anchor-relative, 1=pawn-relative
            float plasticity;          // 196 — CONTACT_2 λ (0=elastic; drift→anchor leak). Was _pad0.
            float target_x;            // 200 — glide target x. Was _pad1.
            float target_z;            // 204 — glide target z. Was _pad2.
        };                             // 208 total (13×16)
```

(In-struct comment blocks elided here for table clarity; the anchor-law
block at :873-878 is quoted in §6 — it names "the music couplings" as future
target authors.) WGSL mirror: `FloatingEntityState`, `world.wgsl:946-992`,
field-for-field identical offsets, wrapped by `FloatingEntityArray {
entities: array<FloatingEntityState, 264> }` (:994-996). The pad-retirement
trail (`Was _pad0/_pad1/_pad2` at 196/200/204) means **zero free pad bytes
remain** — any new per-cube field is a 208→224 GROWTH LAW event (same
commit, same position, same type, sizeof witness, both rooms).

### Buffer, upload sites, cadence

Creation (`state.hpp:3137-3139`, `SU = Storage | CopyDst` at :3109):

```cpp
                floatingEntityBuffer_ = makeBuffer("Floating Entity Array",
                    Dim::TOTAL_FLOATING_SLOTS * sizeof(GPUFloatingEntityState),
                    SU | wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopySrc);
```

= 264 × 208 = 54,912 B, usage Storage|CopyDst|Uniform|CopySrc. Write sites
(all `queue.WriteBuffer`, all event-driven, none per-frame):
whole-slot `upload_cube_entity_slot` (`state.hpp:2161-2163` →
`writeSlot` :1987-1990, "Shape B: COMMIT-DRIVEN slot writes") from spawn
commit (`cube_behaviors.hpp:527`), evict (:331), teardown (:231-232);
partial writes `upload_cube_behavior_id` (4 B @172, `state.hpp:2169-2173`),
`upload_cube_follow_pawn` (4 B @192, :2181-2185), `upload_cube_glide_target`
(8 B @200, :2186-2191); one whole-buffer zero-init at boot (:6104-6108).

**Cadence verdict: the stream is GPU-resident.** Per-frame evolution happens
on the GPU (`dispatch_update_sphere` / `dispatch_update_cube`,
`render_passes.hpp:207-217`); the per-frame CPU traffic is the REVERSE
direction — an async readback copy (`cartridge.hpp:1486-1492`) feeding the
mirror reconcile. There is no per-frame CPU upload of this buffer.

### Shader consumers and slots

One buffer, two bindings (registry `realization/binding_registry.hpp:54` and
`:113`; render = compute + 200 band witness :180):

```wgsl
@group(0) @binding(100) var<storage, read_write> floating_entities: FloatingEntityArray;   // world.wgsl:5981
@group(0) @binding(300) var<uniform> render_floating: FloatingEntityArray;                 // world.wgsl:6011
```

Both bound to `floatingEntityBuffer_` (compute entries `state.hpp:5096-5098`;
render :5168-5170; photographer :5465-5467) — zero copies between them.
Consumers:

- binding 100 (compute): `update_sphere` (:7983), `update_cube` (:8142),
  plus read-only touches in `update_player_agent` (:7650),
  `update_other_agents` (:7757), `update_camera` (:7906).
- binding 300 (render): `sphere_vs` (:5149), `monolith_vs` (:5198 — the cube
  VS), `shadow_sphere_vs` (:5323), `shadow_monolith_vs` (:5335), and one
  read in `patch_terrain_fs` (:4695, sphere 0's pos).

Draw: `dt_monolith` (`drawable_table.hpp:67-70`) → `draw_monolith`
(`renderer.hpp:776-791`) —
`pass.DrawIndexed(indexCount, Dim::MAX_CUBE_INSTANCES, 0, 0,
Dim::CUBE_SLOT_OFFSET);` (:790); shadow twin :1024-1037. Always full count;
`firstInstance = 8` makes `@builtin(instance_index)` the floating-slot index
directly; inactive/out-of-ring slots collapse to r = 0 in the VS (:5205).
Per-cube data reaches the VS through the buffer lookup
(`render_floating.entities[inst]`, :5199), NOT a vertex-instance stream —
every VBL in renderer.hpp is `VertexStepMode::Vertex` (e.g. :1662).

### Color capability

**A color-capable field exists today**: `color[3]` @48, comment "current
appearance (coupling-driven)". The cube VS reads `fe.color` (never
`base_color`) and jitters it per face by `entity_seed` hash ×
`face_variance` (`world.wgsl:5223-5225`), then `entity_fs` shades it lit
(:5168-5185). No emissive field exists anywhere in the entity path (no
`emissive` match in the cartridge; `shade_lit` has no self-illumination
term). A CPU write to `color[]` on a live slot reaches pixels with **zero
new draws, zero new bindings, zero new WGSL branches** — the projector
premise holds — with ONE standing fight: the driverless coupling block
rewrites `color` toward `base_color` every frame (§6, §10-1).

---

## §4 R4 — CHANNEL IDIOM

### The mechanism binding ch1 to the ribbon coupling, verbatim

The channel-prefix strings live in the coupling layer, not the cartridge.
`src/coupling/visual_canvas.hpp:124-126`:

```cpp
    // ── Casting (the avatar principle) ── one voice per entity; the set
    // of these is the CASTING SHEET. The ribbon is the chordal piano.
    inline constexpr const char* RIBBON_VOICE = "ch1";   // live prefix verified: chN (canvas_1 NAME_* tables)
```

and its checker twin `:189`:

```cpp
    inline constexpr const char* CHECKER_VOICE = "ch1";   // the chordal piano; chN = wire = Ableton − 1
```

Consumption is compose-then-resolve, once, at bind
(`visual_canvas.hpp:284-287`):

```cpp
            {
                std::string v(RIBBON_VOICE);
                voice_playhead_ = signal_layout_.resolve((v + ".present_count").c_str());
            }
```

(checker twin at :303-306 resolves `(v + ".window_length")`). The resolver
(`src/musical/signal_layout.hpp:48-59`) is a linear name scan over the
published `StatGroup`s; a miss warns on stderr and returns
`{valid = false}` — "callers leave the coupling disabled rather than reading
a wrong slot." Wiring: `incubator_dual.cpp:192`
`render.bind_signal_layout(analysis.stat_layout());` →
`Cartridge::bind_signal_layout` (`cartridge.hpp:624-633`) →
`visual_canvas_.bind(v)`.

### The publish calls, verbatim (the verification surface)

Live composition `Canvas::initialize` (`src/analysis/canvas_1/canvas.hpp`),
7 voices (`constexpr int VOICES = 7;` :109), per-voice loop :120-129:

```cpp
            publish_reading(Reading::CurrentPC,    Source::channel(v), NAME_CURRENT_PC[v]);
            publish_reading(Reading::PresentCount, Source::channel(v), NAME_PRESENT_COUNT[v]);
            publish_reading(Reading::WindowLength, Source::channel(v), NAME_WINDOW_LENGTH[v]);
            publish_reading(Reading::Distance,     Source::channel(v), NAME_DISTANCE[v]);
            publish_reading(Reading::DftMag,       Source::channel(v), NAME_DFT_MAG[v]);
            publish_reading(Reading::DftPhase,     Source::channel(v), NAME_DFT_PHASE[v]);
```

compounds :135-141 (`const Source all = Source::group({0, 1, 2, 3, 4, 5, 6});`):
`"all.field"`, `"all.current_pc"`, `"all.present_count"`,
`"all.window_length"`, `"all.dft_mag"`, `"all.dft_phase"`. The NAME_* tables
(:312-335) enumerate `ch0..ch7` for each reading; **only ch0..ch6 are ever
published** (the tables span MAX_CHANNELS=8; v runs 0..6). The runtime
surface is exactly 48 groups: {ch0..ch6} × {current_pc, present_count,
window_length, distance, dft_mag, dft_phase} + the six `all.*` compounds.
Both live coupling names — `ch1.present_count`, `ch1.window_length` — verify
against this list, which is what the :126 comment records. Channel meaning:
slot v ← MIDI channel v (:110-113); wire = Ableton channel − 1 (:189
comment in visual_canvas.hpp).

### Note-on, pitch, velocity availability per channel

Ingest layer HAS them: `MidiEvent { type, channel, pitch, velocity, beat }`
(`src/sources/midi_event.hpp:19-30`); one `MidiStream` per channel keeps
per-pitch velocity/onset (`musical/midi_stream.hpp:4-5, 74-84`;
`stream_data.hpp:126-155` — `CompletedNote` even carries
`pitch_class()`/`octave()`).

Published surface has NONE of the three raw facts:
- **note-on events**: no event stream crosses the `AnalysisSignal` boundary —
  `publish()` (`canvas.hpp:429-436`) writes only per-frame aggregate
  reading vectors;
- **pitch with octave**: every published vector is folded to 12 pitch
  classes; octave information survives only as the scalar `chN.distance`
  (signed registral interval);
- **velocity**: no published reading reads it — the `Reading` enum
  (`canvas.hpp:175-179`) has no velocity entry; pc_count weighs 1, pc_length
  weighs duration; `musical_ops.hpp:97-100` names velocity weighting as an
  unrealized extractor option. Velocity dies at the stream/context layer.

ZOETROPE's writer (note-on strikes cell, velocity sets excitation) therefore
needs analysis-side publishes that do not exist yet — §11 ruling 4.

### The native channel-designation idiom (listener set)

Three idioms exist, all analysis-side; the coupling side has only
single-voice casting:
1. channel-SET source: `Source::group({...})` over a `uint32_t mask`
   (`canvas.hpp:181-189`; live use :135);
2. slot→channel designation: `ContextSpec.channel`
   (`musical/context_spec.hpp:116-117`), matched by the router
   (`canvas.hpp:244-251`);
3. coupling-side casting: the `*_VOICE` string constants (§ above) — one
   voice per entity, no set form. A "listener set {7}" has no existing
   coupling-side idiom; the nearest native form is (1)'s mask — §11
   ruling 4.

---

## §5 R5 — CLOCKS

The analysis pipeline publishes exactly three time facts, in the signal
header (`src/analysis/analysis_signal.hpp:75-77`):

```cpp
    float t_seconds;        // Wall clock time when computed
    float t_beats;          // Musical time when computed
    float dt;               // Frame delta (seconds)
```

- **t_beats — a real beat clock, published per frame.** Source: the DAW
  transport — `const float beat = static_cast<float>(port_.beats());   //
  the DAW's clock` (`canvas.hpp:151`), written at `publish()`
  (`canvas.hpp:430`). Reaches the board every frame through
  `analysis.output()` (`incubator_dual.cpp:247` →
  `Cartridge::update`).
- Board consumption: `phase_fill_signal` copies it to the GPU signal
  (`cartridge.hpp:760` `gpuSignal.t_beats = signal.t_beats;`) and derives
  `dt_beats` (:775); `phase_advance_clock` maintains
  `time_state_.beats/prev_beats` and the tempo follower
  `time_state_.beat_rate = db / dt` (:795-806).
- GPU availability: `FrameSignal.t_beats` (`world.wgsl:749`) and
  `FrameSignal.dt_beats` (:768, "beat-time delta (currentBeats_ −
  prevBeats_)") — already consumed by agent sway (:6728-6729) and GoL zone
  ticks (:7177).
- Tempo: **not published by analysis.** The board derives it:
  `contracts/spine_state.hpp:31-34` — "Musical tempo follower: beats/sec,
  HELD-LAST through silence and stopped transport; defaults to 100 BPM" —
  `float beat_rate = 100.0f / 60.0f;`. Live consumer precedent:
  `ribbon.hpp:788-793` (sway phase advances at
  `beat_rate * (60 / RIBBON_REFERENCE_BPM)`).
- **Bar: none. Subdivision: none.** No bar/measure/subdivision fact exists
  anywhere in the published surface, the board clock (`TimeState`), or the
  GPU signal (searched `bar|measure|subdiv` across the pipeline).

Consequence for the frame's R5 contingency: the seconds-based fallback is
NOT needed — a beat clock exists and is reachable at the board site
(`signal.t_beats`, `time_state_.beats`, `dt_beats`). Bar-aware behavior
would need new publishes.

---

## §6 R6 — EXISTING COUPLINGS (analysis facts → visuals)

Full live inventory, each a one-home datum:

| # | source (published name) | decode home | flush → GPU | target |
|---|---|---|---|---|
| 1 | `all.field` | `visual_canvas.hpp:330-345` | `set_fog` (`cartridge.hpp:814-820`) | fog density + color |
| 2 | `ch1.present_count` | swell :352-374 | `upload_ribbon_wave_amps` (`ribbon.hpp:873`; `state.hpp:2142-2145`) | ribbon wave amplitudes |
| 3 | `all.window_length` + `all.present_count` | tint :380-421 | `upload_ribbon_color` (`ribbon.hpp:871`; per-frame lerp in the flush loop, :864-868) | ribbon color |
| 4 | `ch1.window_length` | checker :433-493 | `set_checker_color_field` (`cartridge.hpp:823-827`) | terrain checker cells |

All four run in `phase_motion_drivers` (U4, `cartridge.hpp:811-813`
`visual_canvas_.tick(signal);`) or the ribbon tick. **None touches the cube
family.**

GPU-side (the muting registry, `world.wgsl:2538-2556`): the only
signal-to-entity coupling is `[COUPLING:signal.polyphony→sphere:color]`
(`coupling_signal_polyphony_to_sphere_color`, :2601-2617), gated by
`COUPLING_POLYPHONY_TO_SPHERE_COLOR = 1u << 12u` (:2548) — and it is applied
to BOTH floater families: sphere slots (`update_sphere`, :8036-8046) and
**cube slots** (`update_cube` tail, :8448-8458):

```wgsl
        // Floater color — shares the sphere-color capability below.
        if (signal_active() && coupling_active(COUPLING_POLYPHONY_TO_SPHERE_COLOR)) {
            // DRIVERLESS (M1-C): raw signal.stats[0] substituted with the
            // neutral 0.0 — color rests at base_color.
            floating_entities.entities[slot].color = coupling_signal_polyphony_to_sphere_color(
                0.0,
                ...
```

The polyphony argument is hardwired `0.0` (DRIVERLESS since M1-C; the
terrain twin was retired outright, :2593-2597), so the block's LIVE behavior
is an exponential walk of `color → base_color` at
`SPHERE_COLOR_RELEASE_RATE = 2.0` (:1987, :2606-2608). It is ACTIVE at boot:
`config_.mute_signal = 0; config_.mute_couplings = Coupling::NONE;`
(`state.hpp:5983-5985`). **So: no live analysis fact writes cube color,
scale, or motion today — but a driverless color-writing mechanism runs on
every cube slot every frame.** That block is §10's first entry.

Also load-bearing for ZOETROPE: the target door is already named for music.
`state.hpp:873-877` (struct comment): "Goals may leap; values only walk. The
CPU (and later the music couplings) author these targets"; `world.wgsl:
8230-8233`: "the CPU corral (and later the music couplings) write TARGETS
through the target door". The landing site exists; nothing occupies it.

F5's `config.floater_coordination` is fed ONLY by the key (no analysis
author; §1).

---

## §7 R7 — CONSTANTS GRAMMAR

### The named exemplar — anchor mismatch, STOP

The handoff names "the ribbon coupling's goal/stiffness/damping set" as the
exemplar registration. **No such verbatim set exists in the tree.** Per the
register (multiple candidates for a "the" → report all, bind none):

1. `src/coupling/visual_canvas.hpp:133-141` — `RIBBON_SWELL_CEILING /
   RIBBON_SWELL_RAMP / RIBBON_SWELL_ATTACK / RIBBON_SWELL_RELEASE` — the
   ribbon MUSIC coupling's constants; a `goal` is computed per frame
   (:365-367) but the envelope grammar is trajectory Segments with
   attack/release SPANS in beats — no stiffness, no damping.
2. `src/cartridges/the_board/bodies/ribbon.hpp:109` — `RIBBON_ALT_STIFF =
   0.36f;   // (rad/s)^2 — the pen's stiffness; damping = 2*sqrt(stiffness),
   critically damped` — stiffness with DERIVED damping, in the head-control
   panel (:96-112); no goal constant (the altitude target is computed); a
   flight-control law, not a music coupling.
3. `src/cartridges/the_board/bodies/pawn.hpp:36-37/49-50` —
   `attack_stiffness / attack_damping` (the aura panel) — a true
   stiffness/damping pair, but pawn aura, not ribbon.
4. `src/cartridges/the_board/bodies/cube_behaviors.hpp:57-58` —
   `CUBE_DEFAULT_SPRING_STIFFNESS / CUBE_DEFAULT_DRAG` — the cube substrate
   pair, not ribbon, no goal.

Ruling requested — §11-1. The grammar question is answerable regardless:

### How design constants register today

One registration verbatim — the ribbon coupling's swell set
(`visual_canvas.hpp:128-141`, candidate 1):

```cpp
    // ── Sustain swell (movement) ── PURE ADDITIVE: the dance is the seed
    // idle PLUS the chord's contribution. goal = 1 + (CEILING−1)·t where
    // t ramps over the hold; silence gives 1 from the formula itself —
    // no branch, identity by construction. Music only ever gives;
    // idleness is inviolate. RULED: ceiling 2× idle at 8 beats.
    inline constexpr float RIBBON_SWELL_CEILING = 2.00f;  // × idle (ruled)
    inline constexpr float RIBBON_SWELL_RAMP = 8.0f;   // beats (ruled)
    ...
    inline constexpr float RIBBON_SWELL_ATTACK = 0.35f;  // beats
    inline constexpr float RIBBON_SWELL_RELEASE = 2.0f;   // beats
```

The grammar, generalized (each with its live precedent):

- **Per-module C++ panel**: a named `constexpr` block beside the owner —
  `CameraControls` is the declared FORM TEST ("the first panel, deliberately
  MINIMAL", `input.hpp:49-68`); the cube module already has one (`═══ TUNING
  CONSOLE ═══`, `cube_behaviors.hpp:53-74`); `terrain_looks.hpp` is the
  panel constitution (two rooms, one panel, ROW index, REST pins,
  :5-42).
- **Coupling constants + casting**: the COUPLINGS region of
  `coupling/visual_canvas.hpp` (:75-211), with exposed pipes declared in the
  `PARAM_LAYOUT` master control panel (:222-238) whose overlap witness is a
  build error (:245-256).
- **GPU-live values**: fields of `GPUDesignConfig` under the GROWTH LAW (L5)
  and ALIGNMENT LAW (L4) header (`state.hpp:435-438`), written through
  dirty-gated setters (`set_*`) or the poke idiom (`stage_*`,
  :2596-2618); boot rests pinned from the panel (`terrain_looks.hpp` ROW 2
  discipline).
- **WGSL-side kernel constants**: hot-reloadable consts beside their
  consumer — `CUBE_GLIDE_TAU` (:2005-2010, "Jean-tunable") is the cube
  system's own precedent.

### Where the new subsystem's constants would live

Under the same grammar: lattice/diffusion/pigment constants as a panel block
in `bodies/cube_behaviors.hpp` (the cube module's existing console);
listener-set + decode constants beside the casting sheet in
`coupling/visual_canvas.hpp` with `PARAM_LAYOUT` rows for every exposed
pipe; any GPU-live scalar as a `GPUDesignConfig` field (GROWTH/ALIGNMENT
laws); any kernel-side constant beside `update_cube`. Binding numbers, if
ever needed, only through `binding_registry.hpp` (L6) — though the projector
premise (§3) needs none.

---

## §8 R8 — FXC LIVE LAW (world.wgsl banner block, verbatim)

`src/cartridges/the_board/realization/world.wgsl:12-22` — the banner's
governing-laws block; the FXC entry is L2 (lines 14-16):

```
// THE LAWS THAT GOVERN THIS FILE — src/docs/LAWS.md:
//   L1  encoding — BOM-free LF.
//   L2  FXC — the Windows D3D12 backend's hard limits, honored by
//       structure. READ L2 BEFORE adding a branch to the collision/
//       ground chain or a texture-array stamp anywhere near it.
//   L3  mirror — §2.1 structs ↔ state.hpp byte-for-byte; §3.4
//       POLICY_*_MASK ↔ POLICIES[] in contracts/
//       ground_architecture.hpp; §1.5 randomness ↔ primitives/
//       seed_utils.hpp, bit-identical. Both rooms, same commit.
//   L6  binding numbers — realization/binding_registry.hpp is the
//       source; the @binding literals here are its mirror.
```

(The law's full text is `src/docs/LAWS.md` L2; the banner is its in-file
mirror. ZOETROPE's projector premise adds no WGSL branches, so L2 exposure
is nil by design.)

---

## §9 HYPOTHESIS VERDICTS

**H1 — "F6 already places cubes on a pawn-centered ring with stable
slots." VERDICT: FALSE** (the load-bearing half fails). F6 does place all
active cubes on a point-centered ring (radius 120, §1), but ring index k is
active-set enumeration order at press time — not stable per cube — and in
the default (anchor) mode the ring does not track the pawn afterward.
Consolation: the whole target-door machinery (upload_cube_glide_target +
kernel walk + CUBE_GLIDE_TAU) already exists, so ZOETROPE_2's ring targets
are writes through an existing door, not new machinery.

**H2 — "The instance stream is CPU-uploaded per frame and carries a
color-capable field." VERDICT: HALF-FALSE / CONCLUSION HOLDS.** The premise
is false: the stream is GPU-resident with event-driven CPU writes (§3
cadence verdict); no per-frame CPU upload exists. But the color-capable
field exists (`color[3]` @48, read by the cube VS), and per-slot partial
writes are an established idiom (behavior_id/follow_pawn/target precedents)
— so the projector CAN be zero-growth: per-cube color rides the existing
stream via event/frame-rate `WriteBuffer` pokes, zero new draws/bindings/
branches. NOT a GROWTH LAW event for color. Two caveats: (a) the driverless
color walk fights any CPU-authored `color` while unmuted (§10-1); (b) there
are zero free pad bytes — any NEW field is a 208→224 growth event with the
full witness (§3).

**H3 — "The max-count constant is not divisible by 7." VERDICT: TRUE.**
256 mod 7 = 4; floor(256/7) = 36 → lattice 7×36 = 252, four slots outside
the lattice. Every site the clamp decision touches is the §2 census: 13 CPU
readers of `MAX_CUBE_INSTANCES`, the three WGSL twins (`CUBE_SLOT_COUNT
256u`, the `264` array length, the loop range), and the seven
`TOTAL_FLOATING_SLOTS` sizing sites. Two clamp shapes exist for ZOETROPE_2's
ruling (§11-2): change the constant to a multiple of 7 (touches every census
site, both rooms, same commit) or leave 256 and let the lattice own a live
ceiling of 252 (touches only the spawn gate; 4 slots go permanently dark).

**H4 — "A beat or bar clock is published and reachable from the board
site." VERDICT: TRUE for beat; NO bar.** `t_beats` is published per frame
and already rides the GPU signal (`signal.t_beats`, `dt_beats`), with a
held-last tempo follower (`beat_rate`) board-side (§5). The R5 seconds
fallback is unnecessary. Bar/subdivision facts do not exist anywhere.

**H5 — "No existing mechanism writes cube color from MIDI." VERDICT: TRUE
as to live MIDI — WITH a mandatory §10 entry.** No live analysis fact
reaches cube color (§6). But `update_cube`'s tail runs the driverless
polyphony→floater-color block on every cube slot every frame while
`COUPLING_POLYPHONY_TO_SPHERE_COLOR` is unmuted (it is, at boot) — a
color-writing mechanism whose driver was severed, now acting as a
color→base_color restorer. Absorb-or-delete (§10-1).

**H6 — "All F4/F5/F6 differences are embedding-only." VERDICT: FALSE.**
F4/F5/F6 are not three embeddings of one state at all (§1): F4 selects
force programs, F5 sets a synchrony scalar, F6 authors ring targets; and
the kernel carries genuine per-mode physics beyond target selection (the
two sentinels, the per-arm ground-query location, the anchor-only
plasticity leak). ZOETROPE_2's deletion-candidate list, per the frame's
consequence clause: `CUBE_BEHAVIOR_*` registry + `cube_force_curlfield` /
`cube_force_phasewave` + `CUBE_POPULATIONS` behavior weights
(`cube_behaviors.hpp:102-142`) + `FLOATER_COORDINATION_STEPS` /
`config.floater_coordination` + the corral/kite command pair — each either
absorbed into the lattice model or deleted, never duplicated.

---

## §10 ONE-HOME CONFLICTS (absorb or delete, never duplicate)

1. **The driverless floater-color block** (`world.wgsl:8448-8458` on cube
   slots; :8036-8046 sphere twin; fn :2601-2617; bit :2548; unmuted at boot
   `state.hpp:5983-5985`). Writes `color` every frame. Direct collision
   with pigment→color projection. Note the mute bit covers spheres AND
   cubes together — splitting it is part of the ruling (§11-6).
2. **Spawn color authoring** (`cube_compute_colors`,
   `cube_behaviors.hpp:442-446`; written to both `base_color` and `color`
   at `cube_write_gpu` :473-474). The seed-derived rest color. Ghost law
   (frame §0.6) wants spawn to inherit the cell's accumulated state — the
   spawn-time re-init of `color` is the collision point.
3. **The behavior-force motion authors** (F4 registry + forces,
   `world.wgsl:8072-8139`; coordination knob; drift integrator
   :8352-8355). If lattice excitation ever drives motion (not just
   pigment), this is a second author over the same drift substrate.
4. **The corral/kite target authors** (F6/F7, §1) — the same target door
   ZOETROPE_2's ring embedding would write. Two authors, one door: needs
   the one-author ruling, not duplication.
5. **`config.floater_coordination`** (`state.hpp:507`) — key-fed today; if
   the lattice absorbs synchrony semantics, the field and F5 are conflict
   candidates (frame §0.8: every ZOETROPE constant born panel-native, one
   home).
6. **Per-face VS color jitter** (`monolith_vs`, `world.wgsl:5213-5225`) —
   not a conflict but a modulation the pigment rides THROUGH: displayed
   color = `fe.color` ± `entity_seed`/`face_variance` hash. Named so
   ZOETROPE_2's determinism claim (§0.3) accounts for it (it is
   fixed-seed — deterministic).

---

## §11 OPEN RULINGS FOR JEAN

1. **R7 exemplar**: which set is "the ribbon coupling's
   goal/stiffness/damping"? Four candidates reported (§7); none matches
   verbatim. Bind one, or name the swell set (candidate 1) as the intended
   exemplar.
2. **H3 clamp shape**: constant → multiple of 7 (touch every §2 census
   site, both rooms, same commit) vs lattice-owned live ceiling 252 over an
   unchanged 256-slot buffer (4 dark slots). The lattice-owns-ceiling frame
   (§0.2) reads as the second, but the first is the only shape where
   "the lattice IS the population ceiling's one home" is literally true of
   the constant.
3. **Ghost law vs slot reuse**: eviction is GPU-authored
   (point-distance, is_active=0) and slots are reused lowest-first (§1, §2).
   Cell state living CPU-side in the lattice survives eviction naturally —
   but the cube↔cell binding across evict/respawn needs its law: does a
   respawned slot rebind to its old cell (inheriting that ghost), or to the
   write-head's current column (§0.4)? The frame's §0.6 says "spawn inherits
   the ghost's accumulated state" — WHICH ghost, when slot k's next tenant
   spawns far away?
4. **Writer inputs**: note-on events, octave pitch, and velocity are not
   published (§4). ZOETROPE_2 must either add analysis-side publishes (new
   `Reading` forms — velocity weighting is already named as an unrealized
   extractor option, `musical_ops.hpp:97-100`) or derive strikes from
   per-frame deltas of `chN.present_count` (loses velocity, loses
   same-frame retrigger). Which?
5. **Anchor law for F6**: the frame says "formation anchors to the pawn's
   live position"; today that is continuously true only in kite mode (§1).
   Is ZOETROPE's F6 embedding kite-mode-native (offsets, tracks the point)
   — which the kernel already supports — or press-anchored like today's
   default arm?
6. **Mute vocabulary**: absorbing/deleting conflict §10-1 interacts with
   `COUPLING_POLYPHONY_TO_SPHERE_COLOR` covering both floater families
   under one bit. Split the bit (a muting-registry change) or retire the
   block for cubes outright?
7. **Listener-set idiom**: "expressed in the tree's native
   channel-designation idiom" (§0.4) — the native idioms are the analysis
   `Source::group` mask and the coupling-side single-voice casting string
   (§4). A per-coupling channel SET is new form either way. Mask constant
   beside the casting sheet, or a `Source::group`-published compound per
   listener set on the analysis side?

— end of ledger —
