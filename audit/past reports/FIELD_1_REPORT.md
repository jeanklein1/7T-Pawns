# FIELD_1 — THE AVOIDANCE FIELD RECON LEDGER

**The anchors under the unified avoidance field: who integrates where, what already
repels, where the pack facts live, and where the pass can bind.**

Filed 2026-08-04, read at HEAD `f14af37` (STRAWBERRY FIELDS HANDOFF).
READING ONLY. No edits, no builds, `glaw1` not invoked. Every claim carries
path + line + a verbatim anchor quote. Counts state their search expression and
boundary. Absences state the expressions that were run untruncated. Inferences
are marked `[INFERRED]`.

**METHOD.** Six independent readers, one per hypothesis group (H1+H4, H2, H3+H8,
H5, H6, H7), each instructed to verify its own quotes against the tree before
reporting. Then a second pass by the ledger's author re-verified **every anchor
quoted in this document** — 288 scripted (path, line, quote) checks plus
follow-up reads — and re-ran the
verdict-carrying absence searches untruncated. The second pass overturned two
citations (an off-by-one at `renderer.hpp:1429`→`1430-1431`; a phrase that spans
`world.wgsl:2406-2407`), both corrected in place, and confirmed the rest.

**DOC HOME.** The handoff says "beside LEDGER_1's artifact, mirroring its
convention." LEDGER_1's artifact is `audit/LEDGER_1_REPORT.md`, so this document
lands at **`audit/FIELD_1_REPORT.md`**, matching the `<CAMPAIGN>_REPORT.md` home
and naming.

**GIT.** The handoff orders one commit, straight to master, no `claude/*` branch.
Executed as ordered: this file is the only change in the commit
`FIELD_1: avoidance field recon ledger (read-only)`.

---

## 0 — THE HEADLINES (what redraws the design before any hypothesis detail)

**0.1 — The field would not be the first avoidance mechanism. It would be roughly
the eighth (H2 FALSIFIED).** A mature per-frame entity↔entity influence law —
CONTACT_1–5 — already lives in `world.wgsl`: one shared force body
(`fn influence_response`, `world.wgsl:2343`) applied at **12 call sites** (census
in §H2), driving agent↔agent contact springs and flee-dodges, sphere-pushes-walker,
sphere-pushes-the-point, agents-part-around-the-point, and point-shoves-cubes —
plus an O(N²) boids **separation** force for orbs. "Every entity interpenetrates
freely today" is true only of cube↔cube, sphere↔sphere, orb↔non-orb, and ribbon
rings. The unified field is therefore a **replacement/unification** project, not
an addition; §H2's table is the deletion-target list the handoff asked for.

**0.2 — Integration is GPU-side, not CPU-side, for three of four classes (H1
FALSIFIED).** The CPU spine does not integrate floaters or pawns; `world.wgsl`
kernels do, and the CPU *receives* finished positions via readback. The only
CPU-integrated class is the ribbon (head integration + ring replay). The header
said so all along: `world.wgsl:8-9` — "entity compute, render pipelines, mesh
generation. GPU is / sovereign: CPU dead-reckoning exists only for placement,
picking,". Consequence: the field loop for floaters/pawns is a **WGSL** edit; the
per-frame packed emitter buffer is an **upload** (CPU→GPU), and for GPU-sovereign
emitters its position fact must come from the GPU side (§H3).

**0.3 — The two classes the field wants most have nothing to ride (H7 PARTIAL).**
Cubes, free agents, and orbs each carry per-frame exponential velocity damping a
field force can ride. Sphere floaters and ribbon rings have **no velocity state at
all** — spheres are a closed-form orbit of `t`, rings are a replay of the head's
history — and the possessed pawn's velocity is overwritten from input intent every
frame. For those, "sum a force into velocity" is inapplicable without new state;
the sphere struct already reserves the state it would need (`world.wgsl:970`).

**0.4 — The handoff's banner anchor is a phantom (STOP).** No "FXC banner block"
exists in `world.wgsl`. The file header carries a three-line pointer to L2
(`world.wgsl:14`), and `src/docs/past docs/LAWS.md` §L2 *delegates back* to "the
world.wgsl FXC banner" — a block that is not in the file. Both the law and the
nearest law-like block (the kernel-split banner, `world.wgsl:7487-7510`) are
quoted verbatim in §H5, and the mismatch is recorded rather than papered over.

**0.5 — Populations settle direct summation (H8 CONFIRMED), with one placement
caveat.** Compile-time caps: 8 spheres + 252 living cubes + 32 agents + ≤400
rings = **692** core emitters (realistic ≈ 361); all-in ceiling with orbs,
grounded families, zones and galleries ≈ 1,136, and even 1,136² ≈ 1.29M
pair-tests/frame is one small compute pass. No broadphase. The caveat:
`update_sphere`/`update_cube` are `@workgroup_size(1)` single-threaded kernels
(`world.wgsl:7970`, `:8118`), so a per-floater emitter sum hosted *inside them*
serializes on one thread (§H8).

---

## H1 — INTEGRATION SITE — VERDICT: **FALSIFIED**
### (per class: sphere FALSIFIED · cube FALSIFIED · pawn FALSIFIED · ribbon ring CONFIRMED)

The hypothesis: floater velocity/position integration runs on the CPU spine and
the GPU receives finished positions. The tree says the opposite for floaters and
pawns, and yes for the ribbon.

**The absence that carries the verdict** (re-run by the verifier, untruncated):
`grep -rnE 'pos(_x|_y|_z|\[[012]\])? *[+-]= '` over
`src/cartridges/the_board --include='*.hpp'` and `src/incubator_dual.cpp` returns
exactly three lines, all ribbon-head:

```
bodies/ribbon.hpp:648:        hd.pos[0] -= ch * step;
bodies/ribbon.hpp:649:        hd.pos[2] -= sh * step;
bodies/ribbon.hpp:672:            hd.pos[1] += hd.y_vel * dt;
```

No other CPU position integration exists in scope. The CPU-side motion code is:
the ribbon head mover, spawn-time position writes (`bodies/spheres.hpp:184`,
`bodies/cube_behaviors.hpp:779`, `bodies/agents.hpp:377`), target/scalar walks,
and readback consumption.

**Sphere floater — FALSIFIED, and not integration at all.** Position is authored
on the GPU as a parametric PGA-motor orbit — `world.wgsl:7997`
`let updated = compose_sphere_from_orbit_pga(fe.t, fe);`, angle from
`world.wgsl:3505` `let angle = t * fe.orbit_speed;`, composed at
`world.wgsl:3524` `s.pos = fe.anchor + vec3(local_pos.x, fe.orbit_height,
local_pos.z);` — plus an exponential y-walk toward a floor-respecting target
(`world.wgsl:8013`). There is no `v += a·dt`. The CPU writes position once at
spawn (`bodies/spheres.hpp:184`) and never again.

**Cube floater — FALSIFIED.** True velocity/position integration exists and it is
in the GPU `update_cube` kernel:

```
world.wgsl:8330    fe.drift_vel = fe.drift_vel + (spring_a + behavior_force) * dt + push_impulse;
world.wgsl:8331    fe.drift_vel = fe.drift_vel * exp(-fe.drag * dt);
world.wgsl:8332    fe.drift = fe.drift + fe.drift_vel * dt;
```

The CPU authors targets only — `bodies/cube_behaviors.hpp:370` "The CPU authors
TARGETS only; update_cube walks the live param".

**Pawn — FALSIFIED (GPU-integrated, CPU receives).** Possessed-pawn integration:
`world.wgsl:6898` `agent.pos_x += world_vel.x * speed * dt;` inside
`behavior_player_controlled`. Free agents integrate in `agent_settle`:
`world.wgsl:6793` `a.pos_x += a.vel_x * dt;`. The CPU *reads back* the result —
`cartridge.hpp:1171` `point_.x = p.pos_x;` inside `phase_witness_harvest`
(`cartridge.hpp:1140`).

**Ribbon ring — CONFIRMED.** The head integrates on the CPU (heading
`bodies/ribbon.hpp:644`, xz `:648-649`, y `:672`); ring centerline positions are
rebuilt each frame on the CPU by replaying head history — `bodies/ribbon.hpp:578`
`px += spacing * std::cos(h);` — and uploaded (`bodies/ribbon.hpp:585`
`gpuState.upload_ribbon_head_poses(queue, poses.data(),`, landing in
`state.hpp:2170` `queue.WriteBuffer(headPosesBuffer_, 0, data, bytes);`). The GPU
consumes them read-only — `world.wgsl:6045` `@group(0) @binding(122)
var<storage, read> head_poses: array<vec4<f32>, 400>;` — and composes ring motors
analytically (`world.wgsl:5648`), "no terrain follow" (`world.wgsl:5629`).

**Rendered-position producer: no CPU hop anywhere.** The render bindings are the
same buffers the compute kernels write — `state.hpp:5223-5224`
`entries[2].binding = bind::g0::render_agents;` / `entries[2].buffer =
agentStateBuffer_;`, `state.hpp:5231-5232` (`render_floating` ←
`floatingEntityBuffer_`), `state.hpp:5256-5257` (`render_ring_xforms` ←
`ringTransformsBuffer_`). The VS reads: pawn `world.wgsl:5004/:5099`, sphere
`:5129/:5136`, cube `:5178`, ribbon `:5759`.

### integration_sites

| Class | Where velocity meets position (the field-force seat) | Kernel / function | Rendered position produced by |
|---|---|---|---|
| Sphere floater | **none** — parametric orbit `world.wgsl:3524` + y-ease `:8013` | GPU `update_sphere` (`@compute` at `:7970`) | same kernel → `floatingEntityBuffer_` |
| Cube floater | `world.wgsl:8330` (v += a·dt + impulse), `:8332` (p += v·dt) | GPU `update_cube` (`@compute` at `:8118`) | same kernel (`fe.pos = home + fe.drift;`, `:8374`) |
| Pawn (possessed) | `world.wgsl:6898-6899` (intent), `:7661-7662` (imposed delta) | GPU `update_player_agent` | agent buffer → `pawn_vs` |
| Pawn (free agents) | `world.wgsl:6793-6794` in `agent_settle` | GPU `update_other_agents` (called `:7804`) | agent buffer → `pawn_vs` |
| Ribbon ring | head: CPU `ribbon.hpp:644/:648-649/:672`; rings: CPU replay `:578-579` | CPU RibbonTick | CPU `head_poses` upload → GPU ring motors (`world.wgsl:5648`) |

---

## H2 — THE FIELD IS ADDITIVE — VERDICT: **FALSIFIED**

The hypothesis: no entity↔entity avoidance, repulsion, or separation logic exists
anywhere in scope. It exists, it is systematic, and it is named in the tree.

**The influence law.** One shared force body — `world.wgsl:2343`
`fn influence_response(self_pos: vec3<f32>, self_vel: vec2<f32>,` — with two
response shapes: `world.wgsl:2292` "PRESENCE (the shove)" and `:2294` "APPROACH
(the dodge)", scaled by `world.wgsl:2197` `const CONTACT_SPRING: f32 = 40.0;`.
Profile rows select who feels whom: agent↔agent contact (`world.wgsl:2409`),
agent↔agent flee shell (`:2414`), sphere-pushes (`:2418`),
agents-flee-the-point (`:2426`), point-shoves-cubes (`:2439`). The intent is
literal — `world.wgsl:2243` — `the point -- "cubes avoid you", not "shove them
freely".`

**Call-site census (count law).** Expression: `grep -n 'influence_response'
world.wgsl`, whole file; boundary: excluding the `fn` definition (`:2343`) and
one comment (`:7889`). **12 call sites**: `2488, 2505, 2508` (inside
`occupier_contact`, `:2478`), `7607, 7618, 7641, 7725, 7736, 7752, 7788, 7897,
8325`.

### pairwise_sites — per-frame, the FUTURE DELETION / RECONCILIATION targets

| Pair | Site | Verbatim | What it is |
|---|---|---|---|
| agent ↔ agent contact (possessed pawn included, 4× mass `world.wgsl:7586`) | `world.wgsl:7607` (twin `:7725`) | `let c_r = influence_response(self_p, vec2(0.0), other_p, vec2(0.0),` | pairwise contact spring, gathered over all 32 slots (`:7594`, `:7712`) |
| agent ↔ agent flee/dodge | `world.wgsl:7618` (twin `:7736`) | `let f_r = influence_response(self_p, vec2(agent.vel_x, agent.vel_z),` | approach-rate avoidance |
| agent ← sphere | `world.wgsl:7752` | `let s_r = influence_response(` | sphere body pushes walkers out |
| point/pawn ← sphere | `world.wgsl:7641` (camera twin `:7897`) | `let sp_r = influence_response(` | sphere pushes the player's host |
| agent ← point | `world.wgsl:7788` | `let p_r = influence_response(` | agents part around the point's bubble |
| cube ← point | `world.wgsl:8325` (banner `:8293`) | `let q_r = influence_response(` | the point's presence shoves cubes |
| agent ← occupier (columns/antennas/arch legs) | `world.wgsl:2488/2505/2508` via `occupier_contact` (`:2478`) | `let r = influence_response(self_p, vec2(0.0),` | borderline: immovable spawned entities, functionally terrain — argued both ways, H2 fails on the rows above regardless |
| orb ↔ orb | `world.wgsl:12917` | `sep_sum = sep_sum + diff / d2;` | boids **separation** (loop `:12910`, prev-frame snapshot `:12912`, applied `:12976`), plus alignment/cohesion |
| agent → player flee (behavior 7) | `world.wgsl:7349` | `a.vel_x = a.vel_x + dx * inv * pull * dt;` | per-frame repulsion from the point (away vector `:7338`) |
| agent → player pursuit (behavior 6) | `world.wgsl:7304` | `a.vel_x = a.vel_x + dx * inv * pull * dt;` | pairwise **attraction** — listed because a field must coexist or subsume |
| agent ↔ agent flock cohesion (behaviors 2, 8) | `world.wgsl:7408` | `if (od2 < g.personal_radius * g.personal_radius && od2 > 0.001) {` | cohesion+alignment, no separation term — coupling a field will interact with |

Orb flocking is live, not latent: mood 0 (`open_sunset`) ships 128 orbs on motion
rule 3 (flocking) — `bodies/orbs.hpp:356` — with the prev-frame snapshot buffer
existing solely for it (`state.hpp:1885` `wgpu::Buffer orbStatePrevBuffer_;`).
CPU-side vocabulary agrees: `bodies/agents.hpp:173` `float       personal_radius;`
("social shell"), `:55` `AGENT_BEHAVIOR_PURSUIT           = 6,`.

### spawn_time_checks — placement-time, NOT per-frame avoidance (kept distinct)

| Check | Site | Verbatim |
|---|---|---|
| Family separation matrix | `contracts/spawn_services.hpp:99` | `inline constexpr float MIN_SEPARATION[PopFamily::COUNT][PopFamily::COUNT] = {` |
| Enforcement | `machine/spawn_engine.hpp:595` | `if (dx * dx + dz * dz < effective_min * effective_min) return false;` |
| Proximity affinity (gap SHRINK — attraction) | `machine/spawn_engine.hpp:591` | `if (aff > 0.0f) min_gap *= (1.0f - aff * PROXIMITY_GAP_REDUCTION[placing_family]);` |
| Gallery↔gallery min distance | `bodies/gallery.hpp:959` | `if (dx * dx + dz * dz < min_dist_sq) return false;` |
| Ribbon tip-overlap idempotency (patch-key, not distance) | `bodies/ribbon.hpp:1080` | `Tip-overlap idempotency: reject if ANY active ribbon's` |

**The residual truth inside H2** — pairs that DO interpenetrate freely today:
cube↔cube and sphere↔sphere (verified: within `update_sphere`/`update_cube`
(`world.wgsl:7970-8425`) every `entities[` subscript is `[slot]` — self only),
orb↔non-orb, and ribbon rings. The tree states one of these outright —
`world.wgsl:8302` — "no cube-vs-pawn body-contact row and there cannot be one: a".

Entity-vs-terrain steering exists beside all this (out of H2's pairwise scope):
the gradient "whisper" (`world.wgsl:2206` `const STEER_LOOKAHEAD_WU: f32 = 4.0;`).

---

## H3 — ONE HOME PER PACK FACT — VERDICT: **PARTIAL**

Holds for ribbon rings and pawns; **fails for sphere/cube live position and
sphere size** — those facts live only in the GPU `floating_entities` buffer. The
mitigating machinery already exists: the per-frame floater readback maps the full
struct — position included (`state.hpp:848` `float pos[3];` "world position
(computed by GPU)") — but its two consumers read only liveness
(`cartridge.hpp:1201` `reconcile_sphere_mirror(sphere_state_, &sphere_deps_,
data);`, `:1203` `reconcile_cube_mirror(...)`). A pack-time position harvest is a
funnel extension with **one frame of latency** (R11 capture → next frame R1
harvest), not new plumbing.

### pack_facts

| Class | (a) position home | (b) size/radius home | (c) identity home | Flags |
|---|---|---|---|---|
| Sphere floater | **NO CPU HOME** — GPU `floating_entities.entities[slot].pos` (written `world.wgsl:7999-8019`); CPU mirror `bodies/spheres.hpp:28` `ActiveSphere activeSpheres_[Dim::MAX_SPHERE_INSTANCES]{};` holds patch/host coords + `bool active = false;` (`contracts/floaters.hpp:87`) — no position | **NO CPU STORED HOME** — GPU `body_radius` (`state.hpp:849`) / `influence_radius` (`:851`); re-derivable by seed replay: `machine/spawn_engine.hpp:917` `result.seed = tile_seed(c->world_state_.active_seed, gx, gz);` + `SphereProp::BODY_RADIUS` (`contracts/floaters.hpp:60`) — must replay indoor rescale | slot 0..7; GPU `fe.entity_seed = inst.slot;` (`bodies/spheres.hpp:182`); family `PopFamily::SPHERE = 7` (`contracts/roster.hpp:64`) | NO-HOME (pos, size); readback maps them, consumers discard them |
| Cube floater | **NO CPU HOME** — GPU integrates `drift` and walks even the **anchor** (`state.hpp:893` "update_cube walks" the live param), so the spawn anchor goes stale under corral/kite | `activeCubes_[i].body_radius` = tier prior (`contracts/floaters.hpp:141`, written `bodies/cube_behaviors.hpp:737`); live during formations = `walk_[slot].r` (`:274`); GPU can hold a third value (`ZOETROPE_PIXEL_RADIUS`, `:827`) | slot 0..251; GPU index = `CUBE_SLOT_OFFSET + slot` (`state.hpp:339`, `bodies/cube_behaviors.hpp:777`); family `PopFamily::CUBE = 9` (`contracts/roster.hpp:66`) | NO-HOME (pos); **TWO/THREE-HOME (size)** — pack should read `walk_` when a formation stands, mirror otherwise |
| Pawn (player + agents) | CPU mirror `agent_state_.slots[i].pos_x/y/z` — whole-buffer refresh from readback every harvest (`cartridge.hpp:1159` `std::memcpy(agent_state_.slots, data,`; decl `bodies/agents.hpp:260`); the possessed slot doubled at `point_.x/z` (`contracts/point.hpp:98`, authored `cartridge.hpp:1171-1172`) | figures: `PAWN_FIGURES[skin_id].radius/height` (`bodies/pawn_figures.hpp:240`, row 0 = 1.5/0.5, keyed by mirrored `skin_id`, `state.hpp:697`); collision shell: `AGENT_TIER_GAINS[tier].contact_radius` (`bodies/agents.hpp:171`); WGSL consts `PAWN_HEIGHT/PAWN_RADIUS` (`world.wgsl:1946-1947`) | slot 0..31; `PLAYER_SLOT = 0` (`bodies/agents.hpp:106`) | acceptable two-home: GPU sovereign + CPU mirror, disagreement bounded to 1 frame by the whole-buffer reconciler |
| Ribbon ring | **SINGLE HOME, post-RESIDUE_3 confirmed** — `rs.head` (`bodies/ribbon.hpp:386` `float    pos[3] = { 0.0f, 0.0f, 0.0f };  // live integrated head position` + history), gated on the ONE host value (`:837` `bool  ribbon_flown  = (c->point_.host == PointHost::RIBBON);`); per-ring centerlines derived by one law, `ribbon_rebuild_body_upload` (`:559`), into a transient stack array (`:567`) — capture at upload or recompute | `rs.gpu[slot].cube_size` (`bodies/ribbon.hpp:1206`, seated `:1234`) | `(rendered_slot, k)`; `MAX_RIBBON_INSTANCES = 1` (`:344`); family `RIBBON = 8` (`contracts/roster.hpp:65`) | clean — the one class where the CPU is the author. GPU adds only the sway wave, whose formula the CPU mirrors "LOCKSTEP with the GPU ring motor" (`:709`) — a pack is exact if it replays the wave, centerline±amp if not |

**Strength candidates** (what the pack's `strength` field can draw from):
floaters' `influence_radius` (`state.hpp:851` — an existing seed-drawn per-entity
range), agents' `contact_mass`/`contact_radius`/`personal_radius`
(`bodies/agents.hpp:171-173`), figures' `radius/height`
(`bodies/pawn_figures.hpp:240`), ribbon `cube_size` (`bodies/ribbon.hpp:1206`).

---

## H4 — CONSTRAINT-LAST ORDERING — VERDICT: **PARTIAL**
### (pawn CONFIRMED · cube CONFIRMED · sphere PARTIAL · ribbon FALSIFIED)

**Frame order, proven.** The host loop runs update then render into one submit —
`src/incubator_dual.cpp:248` `render.update(analysis.output(),
console.aspect_ratio(), queue);`, `:258` `render.render(encoder,
console.backbuffer(), console.depth_view());`, `:262` `queue.Submit(1,
&commands);`. Both spines execute in row order (`cartridge.hpp:1099`
`for (const URow& row : UPDATE_SPINE) {`, `:1981` `for (const RRow& row :
RENDER_SPINE) {`). Within the render spine: WitnessHarvest (CPU maps last
frame's copies, `cartridge.hpp:1140`) → RibbonTick (CPU integrates + uploads,
ordering guaranteed "BY CONSTRUCTION", `bodies/ribbon.hpp:936`) → DispatchCompute
(one pass, `render_passes.hpp:173-225`, dispatch order ribbon rings → player
agent → other agents → camera → sphere → cube → vp, disclosed in-shader at
`world.wgsl:7580`) → WitnessCapture (`cartridge.hpp:1516`, `:1524`
`encoder.CopyBufferToBuffer(`) → cull/shadow/main draws from the same buffers.

**Pawn — CONFIRMED, with the two caveats an implementer must hear.** In-kernel
order is integrate (`world.wgsl:6898`) → occupier push → world clamp (`:6938`) →
`pawn_ground_resolve` (`:6949`); a force summed into `world_vel` pre-integration
(the C2b steering precedent, `world.wgsl:6893` `world_vel.x += perp.x * f;`) is
resolved the same frame. Caveats: (1) the contact gather runs AFTER resolve and
its imposed delta "Bypasses this frame's ground resolve, snapped" next frame
(`world.wgsl:7658`, applied `:7661`); (2) stored velocity is overwritten from
intent every frame (`:6907`), so a field force must land on `world_vel` or the
pre-resolve candidate — not on stored `vel_*`. Free agents are cleanest:
behavior + gather shape velocity, then `agent_settle` integrates and snaps last
(`:7804`, `:6793-6798`).

**Cube — CONFIRMED.** Sum (`world.wgsl:8330`) → integrate (`:8332`) → terrain
clamp (`:8368` `if (fe.drift.y < min_drift_y) {`) → compose (`:8374`) → indoor
walls (`:8381`). A field force joins `behavior_force` or `push_impulse` with no
reordering.

**Sphere — PARTIAL.** Resolve does run last (compose `:7997` → floor ease
`:8011-8013` → walls `:8019`), but the premise fails upstream: there is no
velocity to sum into (§H7).

**Ribbon — FALSIFIED.** The terrain floor enters the altitude spring's **target**
before integration — `bodies/ribbon.hpp:660` `const float floor_y = ground_y +
RIBBON_FLOOR_MARGIN;` folds into `alt_target` (`:665`) before `:668-672`
integrate — and nothing re-projects `hd.pos` afterward. The GPU ring pass is
"no terrain follow" (`world.wgsl:5629`). Constraint-last does not describe the
ribbon; a field force there is target-shaping or head-steering, not
force-then-resolve.

**The CPU→GPU boundary shape** (the one CPU-integrated class): what crosses is
`{x, y, z, yaw}` × up to 400 rings, built in `ribbon_rebuild_body_upload`
(`bodies/ribbon.hpp:567-585`), written by `queue.WriteBuffer(headPosesBuffer_,…)`
(`state.hpp:2170`) during RibbonTick, consumed read-only by the ring kernel
(`world.wgsl:6045`, `:5467`) in the same frame's compute pass — queue writes
apply in submission order (`bodies/ribbon.hpp:936-938`).

---

## H5 — ROOM FOR THE PASS — VERDICT: **CONFIRMED**

### Group 2 — THE AGENTS' ROOM, verbatim

WGSL side — the whole census (`grep -n '@group(2)' world.wgsl`, whole file,
re-run by the verifier — exactly two):

```
world.wgsl:2448   @group(2) @binding(0) var<storage, read> occupier_cmg: array<ColumnMeshParams, 32>;
world.wgsl:2449   @group(2) @binding(1) var<storage, read> occupier_amg: array<ArchMeshParams, 16>;
```

Registry charter — `realization/binding_registry.hpp:156-168`:

```
// GROUP 2 — THE AGENTS' ROOM (BATCH F-B, Option B ruling): the
// agent kernels' own bind group. All future agent-side bindings
// (the week's musical couplings included) land HERE, without
// touching the six pipelines sharing the entity layout. Bound
// only by update_player_agent / update_other_agents.
// ─────────────────────────────────────────────────────────────
namespace g2 {
    // Occupier windows — read-only views onto the SAME mesh-param
    // buffers the mesh-gen groups bind (one fact, one home; only
    // reachability topology differs).
    inline constexpr uint32_t occupier_cmg               = 0;   // ColumnMeshParams[32] (columns 0-15, antennas 16-31)
    inline constexpr uint32_t occupier_amg               = 1;   // ArchMeshParams[16]
}
```

CPU layout — declared `realization/renderer.hpp:119` `wgpu::BindGroupLayout
agentOccupierLayout_;    // Group 2, agent kernels only — THE AGENTS' ROOM`;
assembled with **exactly two entries** (`state.hpp:5017`
`std::array<wgpu::BindGroupLayoutEntry, 2> entries{};`), both
Compute-visibility ReadOnlyStorage (`:5019-5025`), under the room rule
(`state.hpp:5014-5015` "one fact, one home. The room grows only when / a named
tenant arrives."). Bind group filled at `state.hpp:5954-5960`
(`columnMeshParamsBuffer_`, `archMeshParamsBuffer_`). Only the two agent
pipelines carry the 3-group layout (`renderer.hpp:1329` `agentLayouts`; the other
live-contributor pipelines keep 2 groups, `:1311` `liveContribLayouts`).

**Free slots in group 2: every binding number ≥ 2.** The packed emitter buffer's
natural home is `g2:2` — it touches no shared layout by construction, which is
precisely the Option B mechanism the room was built for.

### The freed-slot inventory (reallocation campaign, read from the tree)

| Group | Slot(s) | Status | Evidence |
|---|---|---|---|
| g0 | 21, 40 | reserved (freed) | `state.hpp:1750` "(bindings 21, 40 reserved — formerly proximity_field, cell_states)" |
| g0 | 26 | retired (pier_instances) | `state.hpp:4184` "GROUND_CARD_1 [5c] evictions: tile_grid 25, pier_instances 26," — evicted from the compute-entity layout |
| g0 | 192 | unassigned in the mesh-gen band | `binding_registry.hpp:99` "(190–198; 192 unassigned)" |
| g0 | 22 | freed then REUSED — not available | `binding_registry.hpp:40` "Reuses the slot the retired terrain_mesh_indices freed" |
| g2 | ≥ 2 | open by charter | `binding_registry.hpp:157-161` above |

**Per-stage budget and spend.** The law: `src/docs/past docs/LAWS.md:46` — "4.
Storage buffers per stage = 10. Uniform buffers per stage = 12." Live witness in
the tree: `state.hpp:4211` "ReadOnlyStorage but pushed compute storage buffer"
(the 10-per-stage limit, patch_grid moved to make room); the device requests full
adapter limits (`src/console/console.hpp:269` `deviceDesc.requiredLimits =
&adapterLimits;`, motivated at `:264` — the default 8 "is too tight"). The
agent-kernel stage today (verified entry-by-entry, `state.hpp:4157-4215`):
group 0 = 4 Storage + 1 ReadOnlyStorage (`vp_data`, `agent_state`,
`camera_state`, `floating_entities`, `patch_grid`) + 5 uniforms + texture +
sampler; group 1 = 0 storage; group 2 = 2 ReadOnlyStorage. **Spend: 7/10 storage,
5/12 uniform — 3 storage slots unspent.** One read-only emitter buffer fits
without touching any shared layout.

### The host pass

The candidate is **`update_other_agents`** — "1D (32 threads, one per slot).
Algorithmic behaviors + eviction." (`world.wgsl:7480-7481`) — plus
`update_player_agent` for the possessed slot. Both already bind all three groups;
both already run a per-body gather (the CONTACT_5 loop, §H2), which is exactly
the loop shape the field needs. Call sites, verbatim (`render_passes.hpp:187-199`):

```
c->renderer_.dispatch_update_player_agent(
    compute,
    c->gpuState_.compute_entity_group(),
    c->gpuState_.compute_texture_group(),  // aura + sampler for POLICY_WALKER
    c->gpuState_.agent_occupier_group()    // the room: occupier windows
);

c->renderer_.dispatch_update_other_agents(
    compute,
    c->gpuState_.compute_entity_group(),
    c->gpuState_.compute_texture_group(),  // aura + sampler for POLICY_WALKER_AGENT
    c->gpuState_.agent_occupier_group()    // the room: occupier windows
);
```

Dispatch bodies: `renderer.hpp:371` and `:385` — both
`pass.DispatchWorkgroups(1, 1, 1);`. Cost ruling: hosting in the existing agent
kernels is cheaper — zero new pipelines, zero new pass, zero new meter row,
+1 g2 binding. A dedicated pipeline+layout buys FXC isolation (the split
precedent below) at the price of a new layout, compile, dispatch site, and meter
row — and for **floater subscribers** the story differs: `update_sphere`/
`update_cube` bind only two groups today (`render_passes.hpp:207-219` — no
`agent_occupier_group()`), so a floater-side field loop needs either the room
added to their pipeline layout or a dedicated pass (see §H8's serialization
caveat before choosing). WGSL-side, unused group members are legal — the layout
must cover the shader, not vice versa (`renderer.hpp:1430-1431`).

### The FXC banner — STOP: the named anchor does not exist; what exists is quoted

The handoff orders: "Quote the `world.wgsl` FXC banner block verbatim — it is the
law the eventual pass is judged against." **There is no FXC banner block in
`world.wgsl`.** The header carries a three-line pointer (`world.wgsl:14-16`,
"L2  FXC — the Windows D3D12 backend's hard limits, honored by / structure. READ
L2 BEFORE adding a branch to the collision/ / ground chain or a texture-array
stamp anywhere near it."), and the law's own text *delegates back to the
banner*: `src/docs/past docs/LAWS.md:35-36` — "the operational home of the
specifics is the world.wgsl FXC / banner — the banner owns the constraints, this
law owns why they bind." A third pointer cites it as if it stood:
`world.wgsl:2406-2407` "…is not the runtime-indexed const array the / banner
forbids." The referent is absent. Recorded per the register: mismatch noted,
work continued. (Adjacent stale pointer, same family: `world.wgsl:12` and
`binding_registry.hpp:4` name `src/docs/LAWS.md`, which does not exist at HEAD —
the file lives at `src/docs/past docs/LAWS.md`.)

The law the pass will actually be judged against — `src/docs/past docs/LAWS.md`
§L2, verbatim:

```
## L2 — THE FXC LAW

The Windows D3D12 backend compiles through FXC, which has hard limits the
Vulkan/Metal backends do not. The shader honors them **by structure**, so
nothing in it looks like a workaround and everything is one. The law states
the principle; the operational home of the specifics is the world.wgsl FXC
banner — the banner owns the constraints, this law owns why they bind.

1. Instance structs in hot loops stay lean and byte-pinned — the pattern's
   live exemplar is the `GPUSpotLightArray` pin (`static_assert` in
   `state.hpp`: `16 + MAX_SPOT_LIGHTS * 128`).
2. The collision/ground chain admits **no new runtime branching**. The live
   exemplar: the pyramid loop bounds itself by a uniform —
   `min(pyramid_instances.count, MAX_PYRAMID_INSTANCES)` in world.wgsl —
   and dispatch is by uniform function choice, never by branch.
3. Texture-array stamps in the collision chain **hang FXC**. Do not add one.
4. Storage buffers per stage = 10. Uniform buffers per stage = 12.

A violation does not fail on the developer's machine. It fails on Windows, at
pipeline creation, in someone else's hands.
```

And the nearest in-file law-like block — the kernel-split banner,
`world.wgsl:7487-7510`, verbatim (it prices exactly the cliff a hosted field loop
must respect):

```
// ─── Compute kernels ─────────────────────────────────────────────
//
// The agent kernel is split in two for FXC compile-time reasons.
// The original unified kernel placed behavior_player_controlled
// (heavy: walker policy, step-climb, tilt, full contributor chain)
// and behavior_random_walk (light: single agent-policy ground snap)
// in a single switch statement. FXC inlines both branch bodies for
// every one of 32 dispatched threads, producing a pipeline compile
// that landed at 48s. Adding more algorithmic behaviors would
// compound the cost.
//
// Split shape:
//   update_player_agent   — 1 thread. Only the possessed slot, only
//                           behavior_player_controlled. The walker
//                           policy is compiled once, for one slot.
//   update_other_agents   — 32 threads. All non-possessed slots,
//                           behavior switch for algorithmic behaviors.
//                           The walker policy is NOT inlined here.
//
// Dispatch order: player first, then others. THE POINT is
// the eviction reference for this frame's other agents — the
// player's updated position when the pawn hosts, the camera when it
// hosts. Matches the semantic "the point moves, the world adjusts
// around it" (presence follows the point; the ratified rule).
```

### METER_1

The registry census (expression: `grep -rn 'METER_1'` over
`src/cartridges/the_board` and `src/incubator_dual.cpp`, whole scope — 2 hits):
`cartridge.hpp:1950` "METER_1.1: group pair dts by row into a" and
`state.hpp:6118`. The row registry, `state.hpp:1709-1721` verbatim:

```
inline constexpr uint32_t StreamPatches       = 2;
inline constexpr uint32_t EntityMeshGen       = 6;
inline constexpr uint32_t LiveCardWrite       = 8;
inline constexpr uint32_t DispatchCompute     = 9;
inline constexpr uint32_t GolDeriveFlush      = 11;
inline constexpr uint32_t GolZoneCompute      = 12;
inline constexpr uint32_t PawnAura            = 13;
inline constexpr uint32_t OrbSky              = 14;
inline constexpr uint32_t PlacementCorrection = 16;
inline constexpr uint32_t FrustumCull         = 17;
inline constexpr uint32_t ShadowPass          = 18;
inline constexpr uint32_t MainPass            = 19;
inline constexpr uint32_t SnapshotPass        = 20;
```

Each row is pinned to its `RPhase` by a static_assert (`cartridge.hpp:1811`).
**The row that times the field loop: `DispatchCompute` (= 9), existing** — the
whole "Compute Phase" pass is armed once (`render_passes.hpp:176`
`desc.timestampWrites = c->gpuState_.meter_arm_compute(meter_row::DispatchCompute);`),
so a loop hosted in the agent kernels is timed today with zero edits. A dedicated
field pass would need a **new row + new RPhase + its static_assert**; query
headroom exists (`state.hpp:1764` `METER_QUERY_COUNT = 64` — 32 begin/end pairs
against 13 registered rows).

---

## H6 — CHOREOGRAPHY IS ENUMERABLE — VERDICT: **CONFIRMED**

F6/F7 resolve to: two dispatch lines, one self-contained module
(`bodies/cube_behaviors.hpp`), one per-frame seam in `cartridge.hpp`, a family of
partial-write setters that all target `floatingEntityBuffer_`
(`state.hpp:2224-2233` and siblings), and **exactly one WGSL kernel** holding
every mode branch (`update_cube`). The general machine is choreography-blind —
re-verified untruncated: `grep -rni 'kite|formation|zoetrope'` over
`machine/entity_pipeline.hpp`, `machine/spawn_engine.hpp`,
`src/incubator_dual.cpp` → zero hits (exit 1). All executable `follow_pawn`
sites fall inside `update_cube` (`@compute` at `world.wgsl:8118`, next `@compute`
at `:8427`; census: `grep -n 'follow_pawn' world.wgsl` → struct field `:985`,
comments, and executable sites `8189-8398` only).

### f6_f7_sites — every site the two keys reach

| Key | Kind | Site | Verbatim (trimmed) | Motion driven |
|---|---|---|---|---|
| F6 | dispatch | `direction/input.hpp:274` | `case GLFW_KEY_F6: reveal_zoetrope(cube_behaviors_state, &cube_deps, q);` | none directly (stages) |
| F7 | dispatch | `direction/input.hpp:275` | `case GLFW_KEY_F7: toggle_cube_kite_mode(cube_behaviors_state, &cube_deps, q);` | none directly (stages) |
| F6 | handler | `bodies/cube_behaviors.hpp:496` | `inline void reveal_zoetrope(CubeBehaviorsState& cbs, CubeDeps* c, wgpu::Queue& queue) {` | cycles the formation (`switch (cbs.formation) {` `:505`); zero GPU writes in the press frame |
| F6 | state written | `:551` / `:553` | `cbs.formation = next;` / `cbs.stage_wait = true;` | selects the per-frame walk; delays one frame |
| F6 | state written | `:543` | `cbs.walk_[i] = { ac.orbit_height, ac.body_radius, ac.aspect_y, ac.aspect_z };` | seeds the CPU flush-walk |
| F6 | state written | `:560` | `cbs.repaint_all = true;` | one repaint (color/size, no position) |
| F7 | handler | `:591` | `inline void toggle_cube_kite_mode(CubeBehaviorsState& cbs, CubeDeps* c, wgpu::Queue& queue) {` | flips the coordinate frame for all cubes |
| F7 | callee | `:579-585` | `const uint32_t sentinel = on ? 3u : 2u;` … `gpu.upload_cube_follow_pawn(queue, i, sentinel);` | capture/release sentinel per active cube (`cbs.kite_mode = on;` `:580`) |
| F7 | state written | `:603-604` | `cbs.stations_sent = false;` / `cbs.stage_wait    = true;` | re-seats a standing formation |
| both | per-frame reader (CPU) | `cartridge.hpp:880` in `phase_motion_drivers` (`:839`) | `zoetrope_service(cube_behaviors_state_, gpuState_, c.queue,` | the ONE per-frame entry into the choreography (`zoetrope_strike` beside it, `:878`, drives color only in ROAM — formation gate `bodies/cube_behaviors.hpp:1023`) |
| both | per-frame reader (CPU) | `bodies/cube_behaviors.hpp:1099` → `:1104` | `cbs.kite_mode && delta > ZOETROPE_RESEAT_JUMP) {` → `set_cube_kite(cbs, gpu, queue, true);` | reseat on possession jump |
| F6 | per-frame reader (CPU) | `:1191`, `:1217`, `:1265` | `gpu.upload_cube_glide_target(queue, slot, st.off_x, st.off_z);` / `gpu.upload_cube_orbit_height(queue, slot, w.h);` | authors seat targets; walks height/radius/aspects |
| both | per-frame reader (CPU) | `:1281` | `gpu.upload_cube_follow_pawn(queue, slot, cbs.kite_mode ? 3u : 2u);` | hand-back sentinel at settle |
| both | per-spawn reader (CPU) | `:815` / `:833` | `if (c->cube_behaviors_state_.kite_mode) {` | newborn seats into the live formation (reached via `CUBE_ADAPTER`; the pipeline reads no flag) |
| F7 | per-frame reader (WGSL) | `world.wgsl:8189` / `:8199` | `if (fe.follow_pawn == 2u) {` / `if (fe.follow_pawn == 3u) {` | kite release / capture |
| both | per-frame reader (WGSL) | `:8218-8219` | `fe.pawn_offset.x += (fe.target_x - fe.pawn_offset.x) * glide_k;` | the glide walk (`CUBE_GLIDE_TAU`, `world.wgsl:2017`) |
| F7 | per-frame reader (WGSL) | `:8261` → `:8268`/`:8278` | `if (fe.follow_pawn != 0u) {` → point-relative vs anchor home | the home-frame select (the F7 note at `:8244`) |
| F7 | per-frame reader (WGSL) | `:8398` | `if (fe.follow_pawn == 0u) {` | plasticity leak gated to anchor mode |
| — | state carrier | `state.hpp:2224-2227` | `queue.WriteBuffer(floatingEntityBuffer_, base + off, &follow, sizeof(uint32_t));` | all choreography rides `floatingEntityBuffer_` partial writes; no dedicated uniform |

F6 injects **zero** branches into any kernel — it speaks entirely through generic
parameter fields (`orbit_height`, `body_radius`, aspects, `target_x/z`) that
pre-existing readers walk without knowing a formation exists (e.g. the floor
clamp `world.wgsl:8366`, the shove reach gate `:2434`). The judgment call, stated
so a stricter reader can re-rule: F7's five `follow_pawn` branches live *inside*
the general `update_cube` kernel — one contiguous commented block plus three
one-line gates, enumerable, but if the bar were "zero choreography branches in
general kernels," sites `:8218`, `:8261`, `:8398` are the counter-sites. The
formation machine's constants are explicitly not mirrored to WGSL
(`bodies/cube_behaviors.hpp:80` "NONE OF THESE IS MIRRORED TO WGSL.") — no hidden
GPU consumers.

**Re-expression nuance (matters for authored emitters):** F7's per-cube truth is
GPU-side (`follow_pawn`), and its capture/release two-step depends on kernel-only
state (drift) — the CPU deliberately never writes `anchor`/`pawn_offset`
directly. Any redesign that replaces the sentinels must keep or substitute that
in-kernel handshake.

---

## H7 — DAMPING EXISTS — VERDICT: **PARTIAL**

Three subscriber classes ride today; two have nothing to ride; one is a special
case. No class free-orbits under pure repulsion — but "damping exists" as stated
is false for exactly the classes the field wants most.

### damping

| Class | Mechanism | Site | Verbatim | Can a field force ride it? |
|---|---|---|---|---|
| Cube floater (all tiers incl. Monolith — a cube tier, `contracts/floaters.hpp:96`, `bodies/cube_behaviors.hpp:185`) | spring-to-zero + exponential drag on `drift_vel` | `world.wgsl:8331` (spring `:8329`) | `fe.drift_vel = fe.drift_vel * exp(-fe.drag * dt);` | **YES** — join `(spring_a + behavior_force)` or ride `push_impulse` (`:8330`); drag baked nonzero (`CUBE_DEFAULT_DRAG = 1.5f`, `bodies/cube_behaviors.hpp:62`, applied `:783`) |
| Pawn — free agents (behaviors 1–9) | exponential drag on persistent `vel_x/z` in the shared post-step | `world.wgsl:6741-6743` | `let decay = exp(-drag * dt);` / `a.vel_x *= decay;` | **YES** — gather-side velocity adds persist and decay; per-behavior drag 0.6–3.0, uploaded `bodies/agents.hpp:309` |
| Orbs (all 4 motion rules) | exponential drag on `orb.vel` (+ speed clamp in flocking, `:12987`) | `world.wgsl:12810` (et al.) | `orb.vel = orb.vel * exp(-orb.drag * orb_config.rule_drag_brownian * dt);` | **YES tangentially** — dome reprojection (`:13011` `orb.pos = orb.pos * (orb_config.dome_radius / r);`) annihilates radial components; orbs are a 2-DOF subscriber |
| Ribbon head (y only) | critically damped spring on `y_vel` | `bodies/ribbon.hpp:667-669` | `const float damp = 2.0f * std::sqrt(RIBBON_ALT_STIFF);` | **PARTIAL** — a vertical force could ride `y_vel`; horizontal has no velocity state (yaw command is eased, `:522`) |

### flagged — no velocity state / not ridable as-is

| Class | What exists instead | Site | Consequence for FIELD_1 |
|---|---|---|---|
| Sphere floater | closed-form orbit of `fe.t` (`world.wgsl:3505`, `:3524`); y position-lerp (`:8013`); the drift substrate exists in the struct and is deliberately unused — `world.wgsl:970` "Drift-integrator substrate (cube use; spheres leave at zero)." | `update_sphere` body (`:7971-8024`) contains zero `drift/drag/vel` reads — re-verified untruncated | cannot orbit, but cannot ride either. The cheap landing is documented by the tree itself: activate the cube-style integrator the sphere struct already carries (`:973` `spring_stiffness` …) |
| Ribbon rings | deterministic replay of head history at `age = k·spacing/P` (`bodies/ribbon.hpp:578-579`); GPU ring motor is analytic (`world.wgsl:5648`) | — | a force on a ring is meaningless — rings have no independent position. The field can only address the HEAD (heading or `y_vel`) |
| Pawn — possessed (behavior 0) | velocity exists but is overwritten from intent every frame (`world.wgsl:6907`; disclosed `:2471` "(behavior_player_controlled, before pawn_ground_resolve — its vel is / overwritten…") ; drag row 0.0 (`bodies/agents.hpp:145`); post-step never runs for it | — | implicit total damping — no orbit risk, but a velocity-added force survives one frame. The sanctioned wire is the pre-resolve seat (§H4 pawn caveats) |

Two riding caveats the design must carry: the free-agent **speed cap governs
intent, not imposition** — gather-side field impulses are uncapped in their
frame, only drag bounds them (steady-state ride speed = F/drag, and drag runs as
low as 0.6/s); and sustained cube drift **relocates** — the plasticity leak
(`world.wgsl:8398` gate) converts held displacement into permanent anchor
movement, so a standing field gradient will migrate cubes, not just offset them.

---

## H8 — POPULATIONS ARE SMALL — VERDICT: **CONFIRMED**

### populations

| Class | Cap | Anchor |
|---|---|---|
| Sphere floaters | 8 | `state.hpp:335` `constexpr uint32_t MAX_SPHERE_INSTANCES = 8;` |
| Cube floaters | 256 capacity / **252 living** | `state.hpp:338`; `bodies/cube_behaviors.hpp:90` `LATTICE_CELLS = LATTICE_ROWS * LATTICE_COLS;  // 252 — the LIVING ceiling; capacity stays 256` |
| Pawns/agents | 32 cap; live 1 + 10/4/4/0 by mood | `state.hpp:355`; `bodies/agents.hpp:211` `/*count=*/ 10,` (open_sunset row) |
| Ribbon rings | 1 ribbon × ≤400 rings (realistic ~70–90, length-capped) | `bodies/ribbon.hpp:344`; `state.hpp:62` `RIBBON_MAX_RINGS = 400;` |
| Orbs | 256 cap; 128 live in open_sunset | `state.hpp:350`; `bodies/orbs.hpp:356` (mood row: 128, rule 3) |
| Arch / Column+Antenna / Pyramid / Palm / Cactus / Blade | 16 / 32 / 8 / 24 / 20 / 32 | `state.hpp:277`, `:284-286`, `:296`, `:299`, `:306`, `:313` |
| GoL zones | 8 | `state.hpp:332` |
| Galleries / paintings | 48 / 288 | `bodies/gallery.hpp:571`; `state.hpp:257` |

### worst case

Core emitter set (the handoff's three classes + rings at cap):
8 + 252 + 32 + 400 = **692**; realistic ≈ 8 + 252 + 11 + 90 = **361**.
Subscribers (agents + all floaters): 32 + 252 + 8 = **292**.
Worst case 692 × 292 = **202,064** pair-tests/frame; realistic ≈ 105K; the
absolute all-in ceiling (adding orbs, grounded families, zones, galleries on both
sides, ≈1,136 each) is 1,136² ≈ **1.29M** — a single small compute pass at any
plausible rate. **Direct summation is settled; no broadphase question opens.**

The one caveat is placement, not complexity: `update_sphere` and `update_cube`
are single-threaded (`@compute @workgroup_size(1)`, `world.wgsl:7970` / `:8118`;
priced by the tree at `state.hpp:336-337` "update_cube is single-thread /
(@workgroup_size(1)) — cost scales linearly, ~7.5K ops/frame"). A per-cube
emitter sum hosted inside `update_cube` serializes 252 × 692 ≈ **174K iterations
on one thread** — fine as arithmetic, but it is the frame's longest single
thread. The 32-wide `update_other_agents` absorbs its share trivially
(32 × 692 ≈ 22K spread over 32 threads). If floaters subscribe, prefer widening
or a dedicated pass over stuffing the serial kernels.

---

## STOP REGISTER (named anchors that failed, per the handoff's rule)

1. **The `world.wgsl` FXC banner block — ABSENT.** Ordered quoted verbatim as
   "the law the eventual pass is judged against"; no such block exists in the
   file. The header pointer (`world.wgsl:14-16`), the law's delegation
   (`past docs/LAWS.md:35-36`), and a citing comment (`world.wgsl:2406-2407`)
   all reference it. §H5 quotes what actually stands (L2 in full, the
   kernel-split banner in full). Downstream consequence: a field-loop commit
   touching the sanctum has no in-file banner to honor or update — write the
   banner or repoint L2 before that commit.
2. **`src/docs/LAWS.md` — stale path.** Named by `world.wgsl:12` and
   `binding_registry.hpp:4`; the file lives at `src/docs/past docs/LAWS.md` at
   HEAD. An archived-by-folder location for a law the tree still cites as
   governing (the P4 pattern, one layer up).

No other named anchor failed: the RESIDUE_3 quartet (`bodies/ribbon.hpp:414`,
`:836`, `:1426`, `contracts/point.hpp:64`), the F6/F7 dispatch pair, the group-2
seeds, and the METER_1 pair were all found exactly where expected.

---

## APPENDIX — THE SEARCH RECORD (absence-law compliance)

Verdict-carrying absence searches, each run untruncated and re-run by the
verifier; boundary = the full named file set:

| Claim served | Expression | File set | Result |
|---|---|---|---|
| H1: no CPU position integration outside ribbon | `grep -rnE 'pos(_x\|_y\|_z\|\[[012]\])? *[+-]= '` | `the_board/**/*.hpp` + `incubator_dual.cpp` | 3 hits, all `ribbon.hpp` (648/649/672) |
| H2: call-site census | `grep -n 'influence_response'` | `world.wgsl` | 14 lines = 1 defn + 1 comment + **12 call sites** |
| H2 residual: floater kernels read no other floater | `grep -n 'entities\['` over `world.wgsl:7970-8425` | the two kernels | every subscript is `[slot]` |
| H5: group-2 census | `grep -n '@group(2)'` | `world.wgsl` | exactly 2448, 2449 |
| H5: METER_1 census | `grep -rn 'METER_1'` | scope | exactly `cartridge.hpp:1950`, `state.hpp:6118` |
| H6: choreography absence in the general machine | `grep -rni 'kite\|formation\|zoetrope'` | `entity_pipeline.hpp`, `spawn_engine.hpp`, `incubator_dual.cpp` | 0 hits (exit 1) |
| H6: `follow_pawn` containment | `grep -n 'follow_pawn'` | `world.wgsl` | struct `:985` + comments; all executable sites within `update_cube` (`:8118`–`:8427`) |
| H7: sphere kernel has no velocity | `grep -n 'drift\|drag\|vel'` over `world.wgsl:7971-8024` | `update_sphere` body | 0 hits (exit 1) |
| H8: cap census | `grep -n 'constexpr uint32_t MAX_\|LATTICE_CELLS =\|RIBBON_MAX_RINGS'` | `state.hpp`, `cube_behaviors.hpp` | the table in §H8; no other floater/agent caps exist in `Dim` |

The six readers' full expression logs (including the H2 sweep: `avoid|repel|
repuls|separat|steer|flock|boid|crowd|push_apart|min_dist|neighbor|neighbour|
proximity`, `personal|spacing|apart|overlap|penetrat|collision|collide|nearest`,
case-insensitive over the full scope, plus entity-buffer-read and CPU
distance-computation sweeps) ran untruncated with full output inspected;
`src/incubator_dual.cpp` matched none of them (268 lines, read whole: a
cartridge-agnostic host shell).

---

*FIELD_1 does not touch LEDGER_1's terrain scope; per the handoff, the audits
land in either order.*
