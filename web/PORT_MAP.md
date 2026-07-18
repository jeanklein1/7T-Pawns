# 7T Web Port — PORT_MAP (Phase 0 recon)

Surveyed on branch `COUPLING_SAGA_MINIATURES` at `3b5fdef`. Active cartridge:
`src/cartridges/the_board/` (`src/cartridges/backup_board/` is a legacy copy and is
ignored throughout). Shader source of truth: `src/cartridges/the_board/realization/world.wgsl`
(11,747 lines). Desktop visualizer binary: **`incubator_dual`** (`src/incubator_dual.cpp` +
the_board render cartridge + canvas_1 analysis cartridge). `the_lab` is a coupling-tuning
dashboard with no render cartridge — not the port source.

> Repo finding (no action taken): the `${PROJECT_NAME}` target in CMakeLists.txt:741 lists
> `src/main.cpp`, which does not exist; the target is inside `if(NOT INCUBATOR_ONLY)` and
> `INCUBATOR_ONLY` defaults to ON, so it never configures. Stale scaffold, desktop build
> unaffected.

> Audit-doc caveat: `audit/FRAME_CONDUCTOR_RECON.md` and `audit/BINDING_REGISTRY_RECON.md`
> predate the phase-spine refactor and two husk removals (`terrain_state`/`render_terrain`
> bindings, pyramid mesh-gen). Everything below was read from current code, not from those docs.

---

## 0. Demo roster (decided)

Cuts exist **only in the JS host** — the host never creates, dispatches, or draws
them. No WGSL deletions (mirror doctrine).

**CUT:** agent machinery (`update_player_agent`, `update_other_agents`, spawn/respawn,
`trajectories`, agent witness readback, possession/fpv) and floating entities
(`update_sphere`, `update_cube`, sphere/monolith draws, corral). **One still pawn
ships instead:** a single frozen `agent_state` slot written at init + one placement
pass + `Draw(pawn_verts, 1)` — no per-frame update.

**LIFTS (behind roster flags):** terrain + streaming + frustum cull; decor mesh-gen
families (arch, column, palm, cactus, blade); ribbon; pawn aura; orbs;
gallery + wall paintings + photographer; GoL (flagged; launch default decided by
boot data); fade. **Indoor/portals stay unlifted.**

**Lift order (one family at a time, page runnable after each step):**
terrain → camera/vp → ribbon → decor mesh-gen → orbs → gallery/wall → fade →
still pawn → (gol, flagged).

**Accepted divergence (Jean-approved):** the ribbon flies **riderless** — desktop's
sky_mode=1 pawn-mounted-on-ribbon state can't occur with possession cut. The sky_*
signal fields remain (written by the ribbon tick), merely unconsumed by a rider.

---

## 1. Shader inventory — 66 entry points (29 compute, 28 vertex, 9 fragment)

Line numbers are `fn` lines in `world.wgsl`. "Gate" = ROSTER/runtime condition that
suppresses the pipeline or dispatch. Every entry point listed has a live pipeline and a
caller in `renderer.hpp` — there are **no orphan entry points**. (The pyramid family was
fully cut; its `pmg_*` helpers remain as a closed dead island with no entry point.)

### 1a. Compute (29)

| name | wg size | line | purpose | gate |
|---|---|---|---|---|
| `compute_ribbon_rings` | 64 | 4798 | ring transforms for the sky ribbon | ribbon slot set |
| `zone_derive_params` | 1 | 5644 | GoL zone tier selection (hidden 2nd submit) | gol && requests>0 |
| `update_player_agent` | 1 | 6644 | possessed/player agent update | dynamics_0d |
| `update_other_agents` | 32 | 6669 | all non-player agent slots | dynamics_0d |
| `update_camera` | 1 | 6714 | camera-state update | dynamics_0d |
| `update_sphere` | 1 | 6829 | floating-sphere update | dynamics_0d |
| `update_cube` | 1 | 6980 | floating-cube/monolith update | dynamics_0d |
| `compute_vp` | 1 | 7161 | view-projection from camera state (last in compute) | — |
| `generate_terrain_indices` | 8,8 | 7184 | terrain index buffer (init-only) | init |
| `generate_patch_heights` | 16,16 | 7219 | patch heightfield pass 1 | streaming |
| `generate_patch_gradients` | 16,16 | 7240 | patch pass 2: gradients | streaming |
| `generate_patch_cells` | 8,8 | 7584 | per-patch cell fields | streaming |
| `zone_gol_sync` | 8,8,1 | 7637 | sync GoL cells per zone | zones>0 |
| `zone_gol_evolve` | 8,8,1 | 7652 | evolve GoL | zones>0 |
| `zone_gol_mesh_reset` | 1 | 7892 | zero zone-mesh indirect counters | gol |
| `zone_gol_mesh_gen` | 8,8,1 | 7902 | extrusion mesh from alive cells | zones>0 |
| `compute_pawn_aura` | 8,8 | 8040 | pawn aura texture (toroidal grid) | pawn_aura |
| `compute_photographer_vp` | 1 | 8332 | gallery photographer VP | gallery |
| `compute_entity_placement` | 1 | 8432 | entity Y-placement correction | placement_dirty |
| `frustum_cull_patches` | 64 | 8602 | patch cull → visible indices + indirect args | outdoor |
| `arch_mesh_gen` | 1 | 9368 | arch mesh, 16 slots × 4 sub-meshes | arch, dirty |
| `column_mesh_gen` | 1 | 9541 | column+antenna mesh, 32 slots | column\|antenna, dirty |
| `palm_mesh_gen` | 1 | 10037 | palm mesh | palm, dirty |
| `cactus_mesh_gen` | 1 | 10369 | cactus mesh | cactus, dirty |
| `blade_cluster_mesh_gen` | 1 | 10699 | grass blade-cluster mesh | blade, dirty |
| `orb_state_prev_copy` | 64 | 11261 | orb state → prev copy | orbs |
| `orb_init` | 64 | 11268 | orb-sky init (one-shot) | orbs |
| `orb_recolor` | 64 | 11355 | orb recolor | orbs |
| `orb_dynamics` | 64 | 11399 | orb flocking dynamics | orbs |

### 1b. Vertex (28)

| name | line | purpose | gate |
|---|---|---|---|
| `patch_terrain_vs` | 3896 | terrain patch (bufferless, instanced; `USE_PATCH_INDIRECTION` override variant) | — |
| `shadow_patch_terrain_vs` | 4096 | terrain shadow | — |
| `pawn_vs` | 4231 | chess pawn (bufferless GPU-gen, instanced per agent slot) | — |
| `sphere_vs` | 4341 | sphere entity | — |
| `monolith_vs` | 4374 | monolith entity | — |
| `shadow_pawn_vs` / `shadow_sphere_vs` / `shadow_monolith_vs` | 4419/4495/4507 | shadow variants | — |
| `arch_vs` / `shadow_arch_vs` | 4528/4543 | catenary arch | arch |
| `column_vs` / `shadow_column_vs` | 4556/4571 | generative column | column\|antenna |
| `shell_vs` / `shadow_shell_vs` | 4598/4608 | indoor shell | indoor_shell |
| `ribbon_vs` / `shadow_ribbon_vs` | 4846/4997 | sky ribbon (bufferless) | ribbon |
| `fade_overlay_vs` | 5089 | fullscreen triangle (transition fade) | — |
| `zone_extrusion_vs` / `shadow_zone_extrusion_vs` | 7924/8009 | GoL cell extrusion (only vertex-buffer draw: stride 44) | gol |
| `gallery_frame_vs` | 8737 | self-portrait paintings on terrain (instanced quads) | gallery |
| `wall_painting_vs` | 9006 | wall-mounted framed paintings (canvas+frame share) | gallery |
| `palm_vs` / `shadow_palm_vs` | 10280/10301 | palm | palm |
| `cactus_vs` / `shadow_cactus_vs` | 10617/10635 | cactus | cactus |
| `blade_cluster_vs` / `shadow_blade_cluster_vs` | 10853/10871 | grass blades | blade |
| `orb_vs` | 11696 | orb billboard (instanced quad VB) | orbs |

### 1c. Fragment (9)

| name | line | purpose |
|---|---|---|
| `patch_terrain_fs` | 3976 | terrain shading incl. RIM kill |
| `entity_fs` | 4360 | shared lit shading — pawn/sphere/monolith/arch/column/palm/cactus/blade/shell |
| `ribbon_fs` | 4368 | entity shading, veil_scale=0 |
| `fade_overlay_fs` | 5099 | fade_color × fade_alpha |
| `zone_extrusion_fs` | 7974 | GoL cell shading |
| `gallery_frame_fs` | 8824 | samples painting array |
| `wall_painting_canvas_fs` / `wall_painting_frame_fs` | 9036/9050 | canvas/frame split (discard-based) |
| `orb_fs` | 11735 | soft radial circle (additive) |

### 1d. Frame graph (per-frame order)

The frame is conducted by two constexpr spine tables (`cartridge.hpp:1428` `UPDATE_SPINE`,
`:1439` `RENDER_SPINE`); ordering is build-asserted. `update()` is CPU + queue writes only
(no encoder). GPU order within `render()`:

| ord | pass | type | entry points / pipelines | shape | conditional |
|---|---|---|---|---|---|
| S | stream_patches | compute | `generate_patch_heights` → `_gradients` → `_cells` (+ `generate_terrain_indices` init) | per-patch dispatch | streaming cadence |
| R8 | entity_mesh_gen | compute | dirty families only: arch/column/palm/cactus/blade `*_mesh_gen` | per-family dispatch | dirty bits |
| R10 | dispatch_compute | compute | `compute_ribbon_rings`? → `update_player_agent` → `update_other_agents` → `update_camera` → `update_sphere` → `update_cube` → `compute_vp` | 7 dispatches, mostly (1,1,1) | ribbon conditional |
| R11 | witness_capture | copy | agent/floater/camera → staging | buffer copies | — |
| R12a | gol_derive_flush | compute | `zone_derive_params` | **hidden 2nd queue.Submit** (own encoder) | gol && zones |
| R12b | gol_zone_compute | compute ×3 passes | `zone_gol_sync` → `zone_gol_evolve` → [`zone_gol_mesh_reset` + `zone_gol_mesh_gen`] | (4,4,zone_count); pass splits are barriers | gol && zones |
| R13 | pawn_aura | compute | `compute_pawn_aura` | toroidal grid | pawn_aura |
| R14 | orb_sky | compute | `orb_init` → `orb_recolor` → `orb_state_prev_copy` → `orb_dynamics` | (orbs/64,1,1) | orbs |
| R15 | ground_entries | queue write | `upload_ground_entries` | CPU→GPU copy | ground_entries_dirty |
| R16 | placement_correction | compute | `compute_entity_placement` | (1,1,1) | placement_dirty |
| R17 | frustum_cull | compute+copy | `frustum_cull_patches`; copy → indirect buffer | (⌈225/64⌉,1,1) | outdoor only |
| R18 | **shadow_pass** | render, depth-only | terrain shadow (LOD0 indirect + LOD1 direct), then drawable table (DRAW_SHADOW): zone, pawn, sphere, monolith, ribbon, arch, column, palm, cactus, blade, shell | as main | two variants: outdoor single map / indoor spot-light atlas loop (2048×4096 tiled) |
| R19 | **main_pass** | render | terrain LOD0 (indirect outdoor / direct indoor) → LOD1 → drawable table (DRAW_MAIN, same canonical order) → forks: wall_paintings → gallery_frames → orbs → fade | below | LOD/roster/runtime gates; fade always last |
| R20 | snapshot_pass | render | photographer subset (pawn/sphere/ribbon/arch/column/shell) | subset | gallery cadence |
| R21 | promotion_drain | copy | painting staging → exhibition layers | copies | gallery \|\| indoor_shell |

Draw shapes (main pass, canonical drawable-table order): zone `DrawIndexedIndirect`;
pawn `Draw(pawn_verts, MAX_AGENTS)` instanced bufferless; sphere/monolith `DrawIndexed`
instanced; ribbon `Draw(ribbon_verts)` single instance bufferless; arch/column/palm/
cactus/blade/shell one `DrawIndexed` per family (all slots baked into one mesh);
wall paintings 2×`Draw` (canvas fs, frame fs); gallery `Draw(quad, 32 slots)`; orbs
`DrawIndexed(6, orbCount)` additive; fade `Draw(3)` alpha, no depth write.

Pipeline↔entry pairing is by verbatim `Entry::` string constants (`renderer.hpp:22-119`);
shadow pipelines are VS-only (no fragment, colorAttachmentCount=0). Gallery/wall have no
shadow variant. One pipeline `override` exists (`USE_PATCH_INDIRECTION`, world.wgsl:154).

---

## 2. Storage buffer census

Ground truth = the C++ bind-group layouts (the WGSL declaration set is a superset; 64
`var<storage>` declarations total, 4 of which are read-only aliases of other slots for
frustum cull). Counts are storage buffers **visible to the stage** summed across the
pipeline's layouts. Desktop grants itself the adapter max (10) — see §5.

| pipeline (stage) | storage bufs | status |
|---|---|---|
| `update_player_agent` / `update_other_agents` / `update_camera` / `update_sphere` / `update_cube` / `compute_vp` — **ComputeEntity layout** (compute) | **9** | ⚠️ over web floor |
| `compute_entity_placement` — **EntityPlacement layout** (compute) | **9** (+1 storage tex) | ⚠️ over web floor |
| all GoL zone entries (shared layout) (compute) | 7 (+1 storage tex) | ok |
| `compute_photographer_vp` (compute) | 6 | ok |
| `frustum_cull_patches` (compute) | 6 | ok |
| `column_mesh_gen` (compute) | 4 | ok |
| ribbon / arch / palm / cactus / blade mesh-gen (compute) | 3 each | ok |
| patch gen ×3 (compute) | 2 (+3 storage tex) | ok |
| pawn aura (compute) | 2 (+1 storage tex) | ok |
| orb compute / orb copy (compute) | 2 | ok |
| terrain index gen (compute) | 1 | ok |
| **main render** (vertex) — RenderEntity+RenderTexture | **8** | at floor, zero headroom |
| **main render** (fragment) | **8** (7 + zone_params) | at floor, zero headroom |
| **shadow** (vertex) — RenderEntity+ShadowTexture | **8** | at floor, zero headroom |
| gallery / wall painting (vertex / fragment) | 2 / 4 | ok |
| fade overlay | 0 | ok |

ComputeEntity's 9: `vp_data`, `agent_state`, `camera_state`, `floating_entities`,
`trajectories`, `pier_instances`(RO), `zone_config`, `zone_life`, `patch_grid`(RO).
EntityPlacement's 9: `agent_state`, `arch_ground`, `column_ground`, `plant_ground`,
`photo_patch_instances`(RO), `patch_grid`(RO), `photo_painting_slots`, `zone_config`,
`zone_life`.

### Proposed coalescing (gets every stage ≤ 8 without stripping)

1. **Merge `vp_data` into `camera_state`** as one `CameraBlock { camera: CameraState,
   vp: VPMatrix }`. The VP is derived from the camera and the two buffers co-travel in
   every layout that binds either. Effect: ComputeEntity **9 → 8**, main-render vertex
   **8 → 7**, frustum-cull 6 → 5, gallery 2 → 1. One struct edit, applied at every
   `vp_data`/`render_vp`/`fc_vp` site.
2. **Merge the three ground-entry buffers** (`arch_ground` 16 + `column_ground` 32 +
   `plant_ground` 76) into one `ground_entries` buffer with fixed sub-ranges —
   `plant_ground` already packs palm/cactus/blade this way, so the precedent and
   packing scheme exist. Effect: EntityPlacement **9 → 7**.
3. Precedent note: `agent_behaviors`/`agent_tier_gains` were already demoted from
   storage to uniform for exactly this limit (state.hpp:3716-3721) — further demotions
   of small read-only tables remain available as follow-up headroom if needed.

Acceptable interim (contract-sanctioned): request
`maxStorageBuffersPerShaderStage: 9` in `requiredLimits` on Chrome with a
`TODO(portability)` — works on desktop adapters (typically ≥10), **fails device/pipeline
creation on adapters capped at 8** (some integrated/mobile). The coalescing above is the
public-build path.

Secondary budgets (all within core floors): uniforms ≤ 7/stage (floor 12); sampled
textures ≤ 6/stage (floor 16); samplers ≤ 3/stage (floor 16); storage textures ≤ 3/stage
(floor 4 — patch-gen sits closest, watch if adding streaming outputs).

### 2b. Census, web edition (method — desktop unions are the wrong number)

The table above counts desktop C++ layout **unions**; the web build authors its own
layouts, so its numbers differ (and the roster cuts shrink them further). Method:

1. **Dev-only WGSL reflection** (single-file reflector vendored under `web/dev/` —
   dev tooling, not a runtime dep) computes per-entry-point static usage,
   **transitive through helper functions**. The census table below (§2c) comes from
   this dump.
2. **Minimal explicit bind group layouts** authored in `state.js` from that table,
   shared across pipelines that genuinely share bind groups (drawable table,
   shadow/main pairs). `layout:'auto'` is not usable for this: GPUBindGroupLayout
   is opaque in the JS API and auto layouts are per-pipeline, non-interchangeable.
3. **Verification is pipeline creation itself**: Tint validates every statically
   used binding against the explicit layout, so the full-roster async create pass
   IS the mechanical census check. Any stage > 8 after this → upstream fix per the
   mirror doctrine (expected holdout: `compute_entity_placement`; expected fix:
   the ground-entries merge). The vp+camera merge happens only if a real count
   demands it — also upstream.

---

## 3. Uniform feed — what the C++ host writes, and its web replacement

Per-frame hot uniforms are exactly two: `signal: FrameSignal` (b0, 336 B, written every
frame by `phase_fill_signal`/`upload_signal`) and `config: DesignConfig` (b1, 560 B,
dirty-gated whole-struct upload plus three targeted offset sub-writes). No push
constants; no dynamic uniform offsets; one pipeline override (§1d).

Mapping legend: **keep** = same value from JS · **BPM** = derive from the web beat clock ·
**audio** = derive from analyser bands · **const** = freeze at a scene value · **drop** =
omit for the demo.

### 3a. `FrameSignal` (per-frame, every frame)

| field | source on desktop | web mapping |
|---|---|---|
| `t_seconds`, `dt` | wall clock | **keep** (performance.now) |
| `t_beats` | MIDI transport, pulses/24 exact | **BPM** → monotonic beat time |
| `dt_beats` | t_beats − prev | **BPM** → `bpm/60 · dt` while running |
| `aspect_ratio` | window | **keep** |
| `stats[64]` | MIDI ch0 analysis slots | **drop** — DRIVERLESS, read by zero shaders (tombstones world.wgsl:1823/6695/6980); 256 B saved |
| `move_x/z`, `look_az/el_delta`, `zoom_delta`, `pan_x/y_delta` | GLFW input | **keep** — fed by demo autopilot and/or user input (open question §8) |
| `sky_mode`, `sky_head_x/y/z`, `sky_heading`, `sky_yaw_off/pitch/roll` | ribbon CPU tick (mount frame) | **keep** if ribbon ships (its CPU tick ports with it), else drop |

### 3b. `DesignConfig` (dirty-gated; grouped — every field listed)

| group (fields) | source | web mapping |
|---|---|---|
| **fog**: `fog_density`, `fog_color` | **the only MIDI-driven config fields** — held harmonic field via `visual_canvas` | **audio** (rms/bass → density, band-derived hue) or **const** per scene |
| transition: `fade_alpha`, `fade_color` | dt-accumulated portal timer | **keep** (or drop transitions) |
| per-frame algo: `veil_strength`, `pier_count`, `placement_patch_count`, `lod_point_x/z`, `possessed_slot` | spawn/streaming/readback | **keep** — written by the ported conductor |
| per-world/mood: `world_seed`, `sun_direction`, `world_bound_min/max`, `terrain_amp_ceiling`, `ceiling_height`, `indoor_height_cap`, `aura_enabled`, `veil_ring`, `veil_icing`, `veil_dither`, `pawn_amp_scale`, `pawn_height_bias`, `pawn_aura_height` | mood/scene setup | **keep** as scene constants |
| input toggles: `fpv_mode`, `freeze_sphere`, `point_host`, `floater_coordination` | keys | **keep/const** (demo decides interactivity) |
| static knobs: `pawn_speed`, `camera_sensitivity`, `active_cell_size`, `wave_time_scale`, `wave_enable_mask`, `wave_freeze_mask`, `wave_frozen_t[3]`, `point_fly_speed`, `lod0_radius`, `mute_*` ×4 | boot config | **const** |
| retired/DRIVERLESS (boot-only REST writes): `terrain_time`, `band_blend_0..5`, `band_phase_origin_0..5`, `mode_color_shift`, `mode_checker_scatter`, `mode_palette_target/intensity`, `mode_discrete_tier`, `mode_gol_tick_scale`, `mode_gol_height_scale`, `pulse_count`, `pulse_data[8]`, `palette_center[4]`, `palette_light[4]`, `palette_weight` | frozen couplings | **const** at rest values — DECIDED. Revival candidates (`terrain_time` ← beat clock, `pulse_data` ← one radial pulse per beat) are recorded as **Phase 3 garnish — not built** |

Port stance: **keep `DesignConfig`'s layout byte-identical to desktop** (shaders reference
`config.*` throughout 11.7k lines); only re-source the writers. `FrameSignal` is replaced
by the final `U` (§7), whose one layout change is dropping driverless `stats[64]`.

### 3c. Secondary uniforms with a per-frame time slice

| uniform | per-frame writes | web mapping |
|---|---|---|
| `RibbonState` (b120/b360) | `.time` phase (integrates at `beat_rate·60/100·dt`), `lateral/vertical_amp`, `.color` | `.time` ← **BPM** (beatPhase-integrated); amps ← **audio** (rms swell); color ← **audio** hue or const (§3d) |
| `PawnAuraConfig` (b170) | `dt`, `t_beats` | clock + **BPM** |
| `OrbConfig` (b411) | `dt`, `t_seconds` (+ gesture writes) | **keep** — wall-clock; `force_radial` "polyphony" coupling is DRIVERLESS (world.wgsl:10952) |
| registries (`agent_behaviors` b110, `agent_tier_gains` b111) | once at init | **keep** — note `step_rate` is **steps/beat**: agents are beat-quantized through `dt_beats` |
| streaming/entity uniforms (`TileGrid`, `PatchParams`, `PortalArray`, `FloatingEntityArray`, `PhotographerConfig`, `PyramidArray`, `ZoneDeriveRequestArray`) | on-demand | **keep** — written by ported conductor |

### 3d. Live MIDI→visual pipes (complete list — there are exactly five)

All MIDI influence flows CPU-side through `coupling/visual_canvas.hpp` (GPU `stats` is
dead). Coupling envelopes are timed in beats, so all five also lean on the beat clock.

| pipe | desktop source | web mapping |
|---|---|---|
| `config.fog_density` | held harmonic-field rank | **audio**: bass or rms → density (Jean tunes) or const |
| `config.fog_color` | same held field | **audio** hue map or const |
| `RibbonState.lateral/vertical_amp` | sustain swell (held-chord duration) | **audio**: rms envelope |
| `RibbonState.color` (stim) | chroma center-of-mass | const, or treble/mid hue — chroma has no direct audio analogue |
| `RibbonState.color` (mix gate) | anything-sounding gate | **audio**: rms threshold |

### 3e. Beat machinery → `bpm`/`beatPhase`

Desktop already runs dual clocks: `t_seconds/dt` wall + `t_beats/dt_beats` musical, with
`TimeState.beat_rate` (beats/sec, tempo-follower, default 100/60) as the live tempo. Web
mapping: `beat_rate ← bpm/60`; `t_beats ← beats` accumulated from AudioContext.currentTime;
`beatPhase = fract(beats)`. **Parity flag:** desktop's calibration anchor is 100 BPM
(`RIBBON_REFERENCE_BPM = 100`); at the contract's default 120 the ribbon sways ~1.2×
its wall-time-calibrated rate. Matching desktop feel may mean starting `bpm` at 100 —
Jean's call (§8).

---

## 4. CPU-side visual logic

**Verdict: nothing justifies WASM.** The desktop is already thin-host/fat-GPU: all motion
integration, terrain heightfields, GoL evolution, orb flocking, mesh generation, frustum
cull, and aura run as `world.wgsl` compute. The C++ per-frame work is conducting +
plumbing. Inventory of everything nontrivial the host computes:

| module | computes | class | port |
|---|---|---|---|
| `bodies/ribbon.hpp` body rebuild | ≤400 ring poses/frame (trig + history interp), 1 ribbon | visual state — the single heaviest CPU visual loop | moderate JS, cheap |
| `bodies/ribbon.hpp` head advance | yaw-rate-limited steering, damped altitude, saddle frame | visual state | moderate JS, tiny |
| `render_passes.hpp` sun/spot VP matrices | few 4×4s per frame | visual state | trivial JS |
| `surface/patch_system.hpp` streaming | evict/alloc/sort ≤289 patches; GPU generates geometry | bookkeeping | moderate JS |
| `surface/tile_world.hpp` tile state | archetype histogram + weighted roll — on tile creation, not per frame | visual params | trivial-moderate JS |
| `machine/spawn_engine.hpp` + `entity_pipeline.hpp` | spawn gates, footprint/separation negotiation — event-driven | mixed | moderate JS |
| readback machines (R1/R11) | 3 × mapAsync staging + mirror reconcile (1-frame lag by design) | plumbing | trivial JS, but async-model rework (§5) |
| signal/config packing, fades, corral easing, photographer cadence, agent respawn | scalar copies + small loops | plumbing | trivial JS |

No per-frame large-array CPU mutation exists anywhere.

---

## 5. Portability flags

### WGSL — core-clean (verified, not assumed)

No `enable`/`requires` directives; no f16; no subgroups/`textureBarrier`/`chromium_*`;
no binding_array; no read_write storage textures (all five storage textures are
write-only in core formats: rgba16float ×3, rgba8unorm, rg32float, r32float); atomics
are all `atomic<u32>` on storage (core); `discard` appears only in fragment stages;
max workgroup size 16×16 = 256 (exactly the core cap, legal); one `var<workgroup>`
array of 1600 B (cap 16 KiB); one pipeline override constant (core). Depth formats
`depth24plus` (main) and `depth32float` (shadow) are core. No MSAA anywhere. Indirect
draws are core and `firstInstance` is always written 0 — the `indirect-first-instance`
feature is **not** needed (preserve that invariant).

### Needs rework at the host boundary

| item | desktop | web replacement |
|---|---|---|
| device acquisition | `dawn::native` instance/adapter enumeration, `requiredLimits = adapter max` verbatim (console.hpp:169-181) | `navigator.gpu.requestAdapter()/requestDevice()` with an **explicit** limits object; ≤8 after §2 coalescing (else request 9 + TODO(portability)) |
| surface/present | `wgpu::Surface` + `Present()`, `presentMode=Fifo`, format = first adapter-preferred | `canvas.getContext('webgpu').configure()`; `getPreferredCanvasFormat()` (renderer already threads `color_format()`, and the painting R/B swap keys on BGRA — wire it through); no presentMode; `alphaMode:'opaque'` |
| readback | mapAsync polled synchronously in-frame | Promise-based mapAsync; keep the existing 1-frame-lag design (it's already async-tolerant, LAW E-4) |
| paintings | `std::filesystem` scan of `assets/paintings/` + `stbi_load` + CPU downscale | manifest JSON + `fetch`/`createImageBitmap` + `copyExternalImageToTexture`. 57 files, **8.8 MB total** — modest, but lazy-load; only 16 staging + 32 exhibition layers are resident anyway |
| error surface | `SetUncapturedErrorCallback` | `device.onuncapturederror` + boot log of `adapter.limits` (contract working-style requirement) |

No required features, no Dawn toggles, no timestamp/occlusion queries, no
`mappedAtCreation`, no dynamic offsets — the device request is limits-only.

### Binding-0 array-stride assertion (first smoke test)

`FrameSignal.stats: array<f32, 64>` (world.wgsl:650) has stride 4 inside a
`var<uniform>` binding; WGSL's uniform address space historically requires 16-byte
array strides. Desktop Dawn and Chrome share the Tint compiler, so behavior should
be identical — but the **first mirror smoke test asserts it**: `createShaderModule`
+ a pipeline using binding 0 must produce zero validation errors. Fallback if
Chrome ever objects: flip binding 0 to read-only storage upstream — the same struct
already binds as storage at binding 200 (`render_signal`), so it is proven
storage-compatible.

### Boot-cost model

- `createShaderModule` = **whole-module parse/validate** — roster-independent; the
  full 66-entry mirror (~514 KB raw, ~138 KB gzipped) is parsed no matter what
  ships.
- Pipeline creation = **per-entry-point backend compile** — roster-scoped, async
  (`create*PipelineAsync` everywhere).
- The boot report times the two **separately**, per family: module create, each
  pipeline create, init dispatches, asset fetches, time-to-first-pixel.

---

## 6. `web/` module layout

Zero-build ES modules, mirroring the desktop's conductor/realization split:

```
web/
  index.html          entry page: palette tokens, canvas, "enter" gate, status/boot report
  harness.html        kept runnable as the reference rig (contract)
  PORT_MAP.md         this file
  shaders/
    world.wgsl        byte-identical MIRROR of the desktop file (never edited here)
    world.wgsl.source sidecar: source commit hash of the current mirror
  dev/
    reflect.mjs       dev-only WGSL reflector (per-entry-point static usage, §2b)
  js/
    boot.js           adapter/device/limits, canvas config, error+limits logging, boot report
    clock.js          wall clock + beat clock (bpm=100/beatPhase; holds 0 pre-gesture)
    audio.js          drone synth → AnalyserNode → bands/rms; soundtrack stub behind same analyser
    palette.js        CSS token reader → color uniforms (audio blends semantic-tier hues)
    uniforms.js       FrameSignal + DesignConfig packing — the single writer
    state.js          buffers/textures + minimal explicit layouts from the §2c census
    frame.js          the two spines as JS arrays — same phase names, same order
    passes/
      compute.js      R10 dispatch block + mesh-gen + gol + aura + orbs + cull
      terrain.js      patch streaming conductor (patch_system.hpp analogue)
      draw.js         drawable table + shadow/main/fade encoding
    readback.js       Promise-based witness machines (camera; agent/floater witnesses cut)
    assets.js         painting manifest fetch, lazy, first-visible-first (gallery roster)
    autopilot.js      attract-camera driver (writes input-delta fields; user look takes over)
```

Shader single-source is handled by the **mirror doctrine + resync ritual**
(CLAUDE.md): desktop file is upstream, `web/shaders/world.wgsl` is a byte-identical
copy with a sidecar recording the source commit; any port-needed WGSL change lands
upstream first.

**Boot staging** (the point of building full): async pipeline creation everywhere;
terrain+sky pipelines → **first pixel** → remaining families created in background
under the attract camera; per-family timings logged; the boot report table is the
data Jean uses to flag families off (§8).

The ROSTER constexpr gates map to a JS build-of-scene object — the same family
list, so the §0 cuts and any post-boot-report cuts never touch pass code.

---

## 7. Final `U` uniform block (FROZEN)

**Revised freeze (supersedes the earlier re-laid proposal): the web `U` IS
`FrameSignal`, verbatim.** Every existing field keeps its desktop name, type, and
offset — **zero `signal.*` rewrites in world.wgsl**, which is what the mirror
doctrine requires. The new web fields live inside the dead `stats[64]` region
(256 B at byte offset 16, DRIVERLESS — read by zero shaders), which the JS host
sub-addresses as follows:

| stats index | byte offset | field | notes |
|---|---|---|---|
| `stats[0]` | 16 | `bpm` | tempo knob; **default 100** (desktop calibration anchor) |
| `stats[1]` | 20 | `beat_phase` | fract(beats), 0..1 within the beat |
| `stats[2]` | 24 | `rms` | audio bands from the analyser |
| `stats[3]` | 28 | `bass` | |
| `stats[4]` | 32 | `mid` | |
| `stats[5]` | 36 | `treble` | |
| `stats[6]` | 40 | `count` | harness agent count; per-scene reuse |
| `stats[7]` | 44 | — | pad (reserved) |
| `stats[8..11]` | 48 | `colA: vec4f` | palette accent (orb-red); 16-aligned ✓ |
| `stats[12..15]` | 64 | `colB: vec4f` | |
| `stats[16..19]` | 80 | `colC: vec4f` | |
| `stats[20..23]` | 96 | `colBg: vec4f` | |
| `stats[24..63]` | 112–271 | — | reserved (160 B headroom) |

Everything else is the desktop field, unchanged: `t_seconds`/`dt` are the wall
clock (performance.now — **always runs**, pre-gesture drift); `t_beats`/`dt_beats`
are the beat clock (AudioContext.currentTime — **hold 0 until the start gesture**;
a suspended context's clock doesn't advance); `aspect_ratio`; the 7 input deltas
(autopilot + user-look takeover); the `sky_*` mount block (written by the ribbon
tick, riderless per §0). `DesignConfig` (b1) ports byte-identical.

Freeze rationale: the mirror never gets edited, so the layout literally cannot
drift from desktop. The stats-region sub-layout above is a **JS-host convention**
(uniforms.js is its single writer); shaders today read none of it, and any future
GPU-side coupling that wants these fields is a desktop-first change that simply
starts consuming the same slots. Extensions use the reserved region — never
reorder, never touch offsets 0–15 or 272–335.

---

## 8. Decisions (formerly open questions — all resolved by Jean)

1. **BPM = 100** — desktop calibration parity (ribbon reference, beat_rate default).
2. **Roster** — per §0: agents/floaters cut, one still pawn, everything else lifts
   behind flags; indoor/portals unlifted; fade lifts.
3. **Input** — camera autopilot with **user-look takeover**.
4. **Audio mappings** — as §3d, under the palette rule: **audio blends
   semantic-tier hues, never invents RGB** (all color motion interpolates between
   palette-token colors fed through uniforms).
5. **Paintings** — lazy, first-visible-first.
6. **Riderless ribbon** — accepted divergence (§0).
7. **GoL launch default and any further cuts** — decided AFTER the boot report,
   by data (§6 boot staging exists to produce exactly that report).
