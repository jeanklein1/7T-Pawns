# LEDGER_1 — THE TERRAIN COMPUTE LEDGER

**Producers, consumers, rates, and what is paid twice.**

READING ONLY. No edits, no builds, no probes. Every claim carries file + symbol.
Inferences are marked `[INFERRED]`. Absences are stated as findings.

**METHOD.** Every load-bearing claim was read from source directly, then put
through an adversarial second pass instructed to refute it. That pass overturned
four numbers in the first draft — the sun ortho extent (dead code was cited), the
baked texel pitch (fence-post), the mesh:card vertex ratio, and the indoor patch
count — and surfaced three findings the work order did not anticipate (F-4b,
F-4c, F-7b). Corrections are shown in place rather than silently folded in, so
the reasoning that produced the error stays visible.

**DOC HOME.** The work order asked for `src/docs/terrain_compute_ledger.md` and
invited a correction if the tree's home differs. It differs, so this document
lands at **`audit/LEDGER_1_REPORT.md`**. Rationale: `src/docs/` holds long-lived
charters and laws (`LAWS.md`, `7t_program_theory_v3.md`) plus an archival
`old docs/`; every read-only campaign report of the last several commits lands in
`audit/` under `<CAMPAIGN>_REPORT.md` — `POINT_1_REPORT.md` (4ae7635),
`REQUEST_1_REPORT.md` (bf1c459), `BATCH_G_REPORT.md` (0a33733). This document
matches that home and that naming.

**GIT.** The work order said "directly on master". This session is bound by its
harness to the branch `claude/terrain-compute-audit-peuyk7` and may not push to
master. Committed and pushed there; retarget at will.

---

## 0 — FINDINGS, RANKED BY INVOCATIONS × PASSES

Ranked by invocations × passes, per the work order. Geometry-bound today, so a
per-vertex term outranks a per-pixel one.

### F-1 — THE CURTAIN BAND: 48.5% OF LOD0 TRIANGLES, ~ALL DEGENERATE AT REST, NOTHING SUPPRESSES THEM

**Value.** The GoL cell-lift wall between a cell's cap and its unlifted base twin.
**Homes.** Emitted unconditionally into the LOD0 index buffer at
`state.hpp:3192-3210` (CURTAIN QUADS loop). Height driven by `d.lift_scale`
(`world.wgsl:307` `ug_decode` — `1.0` on cap, `0.0` on base band).
**The count.** 256 cells × 16 quads × 6 = **24,576 indices = 8,192 triangles per
patch**, against a 50,688-index / 16,896-triangle LOD0 total — **48.48%**, and
**4,096 base-band vertices per patch (38.1% of LOD0's referenced vertices)**
exist for no other purpose.
**Degeneracy condition (exact).** A curtain quad spans cap perimeter vert
(height + `lift`) to its base twin (height + `0`). Its area is zero ⟺
`ug_cell_lift(...) * (1 - pawn_gol_suppression(...)) == 0` ⟺ the live card's
`.a` at that cell's centre is 0 ⟺ **no GoL zone covers that cell.** At rest that
is *every cell in the world*.
**Suppression mechanism: NONE.** The patch index buffer is built once, on the
CPU, at init (`state.hpp:3166-3243`), uploaded with one `WriteBuffer`, and never
rebuilt — there is no per-frame IB path, no VS early-out, no index rewrite, no
degenerate-skip. Searched: `grep -rn "curtain\|degenerate\|lift_scale\|patchIndexBuffer_"`.
The separate GPU `generate_terrain_indices` kernel (`world.wgsl:7863`) is a plain
grid triangulation for the *legacy* mesh, dispatched once as
`"Terrain Index Gen (one-shot)"` (`cartridge.hpp:505`) — not the patch IB.
**Rate paid.** Outdoor resting: 49 LOD0 patches × 8,192 = 401,408 degenerate
triangles in the eye pass, the same again in the shadow pass — **~802,816
zero-area triangles per frame**, plus **401,408 base-band VS invocations** whose
only output is a collapsed quad.
**What would be lost.** Nothing visually, when lift is zero everywhere. The
curtain is *required* the moment any zone is live, so this is a rate problem, not
a deletion candidate. See lever L-1.

### F-2 — THE SHADOW PASS PAYS FULL EYE DENSITY, UNCULLED, AGAINST A TARGET 4.6× FINER THAN THE MESH

**Value.** The entire terrain surface, re-transformed for the light.
**Homes.** `render_passes.hpp:322-336` `draw_shadow_all` — terrain is drawn with
`c->gpuState_.patch_index_count()` and `c->world_state_.lod0_patch_count`: the
eye's index buffer, the eye's LOD split, verbatim. No coarsening of any kind
exists.
**Uncled.** The eye's LOD0 draw goes through `DrawIndexedIndirect` off the
frustum-cull result (`renderer.hpp:725-737`, `render_passes.hpp:374-382`). The
shadow draw calls plain `DrawIndexed` (`renderer.hpp:999-1016`) and
`shadow_patch_terrain_vs` never reads `visible_patch_indices` — it indexes
`patch_instances[patch_id]` directly (`world.wgsl:4190`). It is culled against
neither the light volume nor the eye's result.
**Resolution mismatch.** `SUN_HALF_EXTENT = 300.0` (`world.wgsl:3245`, consumed
at `:3292`) ⇒ 600 wu across `SHADOW_MAP_SIZE = 4096` (`state.hpp:207`) =
**0.146484 wu/texel**, against a LOD0 vertex spacing of **0.78125 wu**. The
shadow target resolves **5.333× finer per axis (16/3)** than the geometry it
rasterizes — **28.4 shadow texels per LOD0 quad**. Against LOD1 (1.5625 wu) it is
10.67× finer.

> **CORRECTION — and a finding in its own right.** An earlier draft of this
> document took the sun ortho from `render_passes.hpp:569`
> (`const float half_extent = 350.0f;`) and reported 0.1709 wu/texel and a 4.57×
> ratio. **That is dead code.** `compute_sun_matrices` has **zero callers** —
> `grep -rn "compute_sun_matrices" src/` returns only its declaration
> (`render_passes.hpp:50`) and its definition (`:519`). The **live** sun VP is
> built on the GPU: `world.wgsl:7854`,
> `vp_data.light_vp = coupling_pawn_to_sun_vp(...)` inside `compute_vp`, using
> `SUN_HALF_EXTENT = 300.0`. The correct figures are above. A stale 350 in
> unreachable C++ is exactly the failure mode §F-9 catalogues, and it caught this
> audit before the audit caught it.
**Rate paid.** 49 LOD0 + 104 LOD1 patches → 3,202,560 indices / 1,067,520
triangles / ~653,416 VS invocations, *duplicating the eye's terrain work in
full*, and the eye's half of it is at least frustum-culled while this is not.

### F-3 — INDOOR: THE RING GATE IS BYPASSED, CULLING IS OFF, AND THE WHOLE THING RUNS ONCE PER SPOT LIGHT

**Value.** The complete terrain draw, multiplied.
**Homes, three compounding:**
1. `patch_system.hpp:405` — `if (c->world_state_.finite_mode || d2 <= ring_sq)`.
   In finite (indoor) mode the veil-ring draw gate is **bypassed entirely**:
   every resident patch is submitted regardless of distance.
2. `render_passes.hpp:230` — `if (!c->renderer_.use_indirect_terrain()) { return; }
   // indoor: skip`. The frustum-cull compute does not run indoors, and
   `render_passes.hpp:383-392` falls to the direct, uncled draw.
3. `render_passes.hpp:264-297` — the spot loop wraps a **complete render pass**,
   and `draw_shadow_all` (terrain included) sits inside it, up to
   `MAX_SPOT_LIGHTS = 4` (`state.hpp:1363`, `world.wgsl:3555`).
**The count.** `finite_radius` is **seed-derived per destination**, not fixed:
`derive_finite_radius(seed, mood)` picks uniformly in
`[mood.finite_radius_min, finite_radius_max]` (`mood.hpp:1187-1191`,
`entity_pipeline.hpp:983`); the struct default is 2 (`surface_services.hpp:45`,
`mood_constants.hpp:29`). So the resident set is **(2r+1)² = 9 / 25 / 49 / 81
patches** for r ∈ {1,2,3,4}, all inside the 175 wu LOD0 radius ⇒ **all of them
draw at full LOD0 density**. With the eye pass plus one shadow pass per live
spot, the worst case (r=4, 4 spots) is **81 × 5 = 405 full-density patch draws
per frame = 6,842,880 triangles** — materially worse than outdoors.
**Is terrain visible indoors? Yes — it is the floor.** The indoor shell is
*ceiling + walls only* (`state.hpp:210`), capped at `SHELL_MAX_INDICES = 8192`
⇒ ≤2,730 triangles. So the room's own geometry is **~155× smaller than the
terrain floor beneath it**, and a bare indoor room pays the full outdoor terrain
machine. This is the direct explanation for "a bare indoor room still paid it."

### F-4 — THE LIVE CARD IS REWRITTEN WHOLE, EVERY FRAME, WITH NO REST SKIP

**Value.** Waves Δh, pulse Δh, live gradient, GoL lift.
**Home.** `render_passes.hpp:161-170` `dispatch_live_card_write`, called
unconditionally from `cartridge.hpp:1435` via `phase_live_card_write`.
**The count.** `renderer.hpp:538-551` dispatches both passes at full card size,
every frame, with no guard: pass 1 `write_live_card_heights` @8×8 ⇒ (640/8)² =
6,400 workgroups = **409,600 invocations**; pass 2 `write_live_card_resolve`
@16×16 ⇒ (640/16)² = 1,600 workgroups = **409,600 invocations**. **819,200
compute invocations per frame, unconditionally.**
**The rest law zeroes the value, not the work.** The three-conjunct rest law
(`world.wgsl:8458-8470`) is enforced *inside* the kernel —
`if (config.terrain_time > 0.0)` skips the band loop (`world.wgsl:8491`), and
pulses/zones contribute zero — so at rest the card is all zeros and every
consumer adds 0. But both dispatches still run, still write, and pass 2 still
resolves gradients across all 409,600 texels. `[R-RATE]`
**Caveat.** This is the one finding here that is *compute*, not geometry. At
~819K invocations of mostly-early-out work it is unlikely to be the 12.4 ms; it
is listed because it is a pure rate redundancy, cheap to gate, and the campaign
asked for rate ledgers.

### F-4b — THE CARD'S BAND-WAVE TERM IS STRUCTURALLY DEAD IN THE SHIPPING BUILD

`config.terrain_time` gates the entire band loop —
`if (config.terrain_time > 0.0)` at `world.wgsl:8491`. Its **only** writer in the
tree is a boot-pin to rest:

```cpp
gpuState_.set_terrain_time(terrain_looks::REST_TERRAIN_TIME);   // cartridge.hpp:439
inline constexpr float REST_TERRAIN_TIME = 0.0f;   // terrain_looks.hpp:97 — "frozen clock"
```

`grep -rn "set_terrain_time" src/` returns exactly three lines: the setter's
definition (`state.hpp:2409`), that single call site, and a comment
(`terrain_looks.hpp:93`). **There is no other writer**, so `terrain_time` is 0 for
the life of the process and the band loop **never executes**.

Consequence: the card's `.x` carries only `contrib_radial_pulses_at`
(`world.wgsl:8502`), and `.yz` are the gradients of that alone. **The terrain-wave
displacement the card exists to deliver is off by data, not by rest.** The 819,200
invocations of F-4 are producing a field whose dominant term is unreachable.

This sharpens, rather than softens, the oversampling case in H5: the mesh is
carrying 0.78125 wu vertex spacing to resolve a live field that today contributes
nothing but pulses. `[R-RATE]` — and the strongest single argument for L-5.

### F-4c — A THIRD FULL-DENSITY, UNCULLED TERRAIN DRAW LIVES OUTSIDE BOTH PASSES

`render_snapshot_pass` (`gallery.hpp:1230`) issues its own terrain draw at
`gallery.hpp:1272-1277`:

```cpp
c->renderer_.draw_patch_terrain_direct(pass, ...,
    c->gpuState_.patch_index_buffer(),
    c->gpuState_.patch_index_count(),          // 50,688 — LOD0
    c->world_state_.render_patch_count);       // ALL drawn patches, no LOD split
```

Note the combination: the **LOD0** index buffer over **`render_patch_count`**
(all ~153 patches), not `lod0_patch_count`. **This pass draws the far field at
near-field density** — denser than the main pass itself — uncled, with no LOD1
tier. 153 × 16,896 = **2,584,288 triangles** in one pass.

It is gated by `if (!gs.pending_snapshot.active) return;` (`gallery.hpp:1232`), so
it is cadence-driven rather than per-frame, and therefore **does not explain the
steady-state 12.4 ms**. It is reported because it is a third home for the same
geometry, it is the densest of the three, and any lever applied to the other two
must be applied here or the saving leaks. `[R-RATE]`

### F-5 — THE LIVE CARD IS TAPPED TWICE PER VERTEX, IN BOTH VS, BECAUSE TWO SAMPLERS ARE NEEDED

`sample_live_card` (`world.wgsl:8449`) taps `live_card_read` **bilinear**;
`sample_live_card_gol` (`world.wgsl:8453`) taps the **same texture** with the
**nearest** sampler for `.a`. Both fire unconditionally in `patch_terrain_vs`
(lines 3923, 3930) and in `shadow_patch_terrain_vs` (lines 4211, 4215). That is
**2 of the 3 unconditional fetches in each VS spent on one texture**, at
~653,416 vertices × 2 passes ⇒ **~2.6M taps/frame** of a field that is all zeros
at rest. `[R-RATE]`

### F-6 — THE SHADOW PASS IS ALREADY LEAN PER-VERTEX; ITS COST IS PURELY COUNT

Answering D5 directly and in the negative: **the shadow VS computes nothing a
depth-only target cannot use.** `ShadowVarying` declares exactly one member,
`@builtin(position) clip_pos` (`world.wgsl:4632-4634`); the shadow terrain
pipeline binds **no fragment shader at all** (`desc.fragment = nullptr`,
`renderer.hpp:2038`, via the shared `makeShadow` builder instantiated at
`:2061-2062`); and the pass declares zero colour attachments
(`render_passes.hpp:308`). No colour, normal, gradient, fog, checker, tint or AO
is computed and discarded.

Two genuinely dead values, both trivial: `d.wall` is set by `ug_decode`
(`world.wgsl:311, 326, 348`) and read only by the eye VS (`:3942`), never by the
shadow VS; and `let half = pi.extent * 0.5;` (`world.wgsl:3906`) is a dead local
in the **eye** VS, never referenced.

**A correction to an intuition worth recording:** both VS fetch vec4s and use
only some channels (`shadow` uses `.x` of two RGBA16Float taps). This is **not**
a saving opportunity — `textureSampleLevel` returns a vec4 regardless of how many
components are read, and both textures are RGBA16Float (`state.hpp:3763`, `:3734`).
The fetch width is fixed by the format, not by the consumer. **There is no lever
here.** The shadow pass's cost is count, not math and not fetch width.

### F-7 — THE EYE AND SHADOW SURFACES DISAGREE UNDER THE PAWN AURA — STRUCTURALLY

`patch_terrain_vs` adds `aura.r * config.pawn_aura_height` (`world.wgsl:3915-3916`).
`shadow_patch_terrain_vs` **does not** — there is no `sample_pawn_aura` call in
it at all. The shadowed surface is therefore the un-auraed surface.

**This is enforced by the bind group, not merely omitted in code.** The shadow
texture bind group has exactly **4 entries** — `bilinear_sampler`,
`nearest_sampler`, `patch_heightfield_array_read`, `live_card_read`
(`state.hpp:5049-5063`) — and does **not** contain `pawn_aura_read`. The shadow VS
*structurally cannot* sample the aura. No comment anywhere in the tree
acknowledges the divergence (searched `world.wgsl:4178-4222` and the shadow
pipeline block `renderer.hpp:2008-2065`).

Not a performance finding; reported because it is a real divergence, and because
it establishes that **the two VS already run different term sets** — precedent
that they need not agree term-for-term, which lever L-2 leans on.

### F-7b — EVERY VERTEX OF EVERY DRAWN PATCH IS PAID, INCLUDING WHERE EVERY FRAGMENT IS DISCARDED

The veil ring is a **fragment-stage `discard` only** — `world.wgsl:3968-3970` in
`patch_terrain_fs`. There is **no ring/veil vertex kill in the terrain VS**,
unlike flora and zones which do kill per-vertex. Searched: `grep` for
`veil_ring|discard|lod_point` restricted to both VS bodies — zero hits, and the
absence is acknowledged in-code at `world.wgsl:4179-4184`.

Consequence: the draw set is **patch-granular** (nearest-edge ≤ 325 wu) while the
visible region is a **circle**. Every patch straddling the ring — and every patch
in the square corners of the banded set — transforms its full 10,752 (LOD0) or
1,217 (LOD1) vertices, and then discards a large fraction of the resulting
fragments. The geometry is paid in full for pixels that are thrown away. `[R-RATE]`

### F-8 — THE FAR FIELD PAYS NEAR-FIELD DENSITY; THERE IS NO FALLOFF BEYOND ONE STEP

Vertex density falls off exactly once, at `LOD0_RADIUS_DEFAULT = 175`
(`state.hpp:161`), via the `d2 <= lod0_sq` test at `patch_system.hpp:406`. Beyond
that, every patch out to the 325 wu ring pays identical LOD1 density; there is no
second step, no geomorph, no distance-graded tessellation. Searched:
`grep -rn "lod\|LOD\|tess\|morph\|density"` across `patch_system.hpp`,
`state.hpp`, `renderer.hpp`, `world.wgsl`. **Absent — stated plainly as a finding.**
Note the step is also *shallower than it looks*: LOD1 is 6,912 indices against
LOD0's 50,688, a **7.33× drop**, because LOD0 alone carries caps + curtains.

### F-9 — STALE LABEL: THE CARD SAYS 512, THE DESCRIPTOR SAYS 640

**CONFIRMED.** `state.hpp:3732` —
`desc.label = "Live Card (512x512, RGBA16Float — GROUND_CARD_1)"` sits directly
above `desc.size = { Dim::LIVE_CARD_SIZE, Dim::LIVE_CARD_SIZE, 1 }` with
`LIVE_CARD_SIZE = 640` (`state.hpp:95`). Every other 512 site found:
| Site | Text | Kind |
|---|---|---|
| `state.hpp:3732` | `"Live Card (512x512, RGBA16Float — GROUND_CARD_1)"` | **label string** |
| `state.hpp:3728` | `// Live card (512×512 RGBA16Float — GROUND_CARD_1; ...` | comment |
| `state.hpp:1754` | `// Live card (GROUND_CARD_1) — 512×512 RGBA16Float ...` | comment |
| `world.wgsl:8511` | `// ... the bake's pass-2 clone at res 512.` | comment |
| `src/docs/old docs/ground_card_campaign_v2.md:38` | `512² RGBA16F over 800 wu` | archival doc — **two** stale numbers (live: 640, 1000) |

No stale numbers found for `PATCH_MESH_N` or `SHADOW_MAP_SIZE`.

### F-10 — A STALE CONTRACT CLAIM: THE CODE HAS NO ANALYTIC WAVE GRADIENT

`contracts/ground_architecture.hpp:161-162` describes `patch_terrain_vs` as
realizing *"gradients ... via texture `.yz` + the **analytic wave gradient**"*.
**There is no analytic wave gradient in the VS.** `world.wgsl:3937` is
`out.gradients = height_data.yz + live.yz` — **two texture fetches**, no
evaluator. Searched: `awk 'NR>=3875&&NR<=3946{if($0~/contrib_|noise|fbm|octave/)}'`
→ zero hits across the whole VS body.

This matters beyond tidiness: it is the contract file — the document a future
auditor would trust — asserting that the fork is still half-analytic when the
Ground Card retired that path. **It is the written form of exactly the belief
this campaign set out to test, and it is stale.** The same passage's description
of `shadow_patch_terrain_vs` as a "baked + waves subset" is, by contrast,
accurate.

---

## [A] THE FORK — PER QUANTITY

### A note on `patch_terrain_indirect`

There is **no separate WGSL entry point** by that name. `USE_PATCH_INDIRECTION`
is an `override` (`world.wgsl:120`, default `false`) consumed at
`world.wgsl:3881`. The "indirect variant" is the *same* `patch_terrain_vs`
compiled into a second pipeline (`patchTerrainIndirectPipeline_`,
`renderer.hpp:725-737`) with the override flipped. The only difference at vertex
rate is one extra storage load, `visible_patch_indices[patch_id]`.

### `patch_terrain_vs` (`world.wgsl:3875-3946`)

| Quantity | Verdict | Resource / evaluator |
|---|---|---|
| base height | **TEXTURE FETCH** | `patch_heightfield_array_read.x`, written by `generate_patch_heights` (`world.wgsl:7895`) |
| baked gradient | **TEXTURE FETCH** | `patch_heightfield_array_read.yz`, written by `generate_patch_gradients` (`world.wgsl:7917`) |
| wave Δh + pulse Δh | **TEXTURE FETCH** | live card `.x` via `sample_live_card` — one fused channel, not two |
| live gradient | **TEXTURE FETCH** | live card `.yz` via the same tap |
| GoL lift | **TEXTURE FETCH** | live card `.a` via `ug_cell_lift`→`sample_live_card_gol`, nearest, cell-snapped |
| pawn aura | **TEXTURE FETCH** | `pawn_aura_read` via `sample_pawn_aura` (`world.wgsl:5681`) — *guarded* |
| GoL suppression | **ANALYTIC** | `pawn_gol_suppression` (`world.wgsl:2663`) — the only one |
| cell identity | ANALYTIC (index math) | `ug_decode` (`world.wgsl:307`) — pure arithmetic on `vertex_index`, no fetch |
| colour / checker / fog | — | **not computed in the VS at all**; FS-side (see E3) |

**Fetches per vertex — `patch_terrain_vs`: 3 unconditional + 1 guarded = 4 worst case.**
1. `world.wgsl:3900` `patch_heightfield_array_read`, bilinear — unconditional
2. `world.wgsl:3923`→`8450` `live_card_read`, **bilinear** — unconditional
3. `world.wgsl:3930`→`360`→`8455` `live_card_read`, **nearest** — unconditional
4. `world.wgsl:3915`→`5701` `pawn_aura_read`, bilinear — guarded twice:
   `config.aura_enabled < 0.5` returns early, and a footprint test
   `abs(world_xz - pawn_xz) >= half_extent` returns early. Resting case (vertex
   outside the pawn's aura footprint): **not taken**.

**Storage loads per vertex — 4 sites (direct pipeline) / 5 (indirect):**
`patch_instances[actual_id]` (3882); `render_agents[config.possessed_slot]`
**twice** via `render_pawn_pos()` (3915, 3931 — as written; a compiler will
likely CSE the identical address `[INFERRED]`); `render_vp` (3935) — note this is
a **storage** buffer, `@group(0) @binding(201) var<storage, read> render_vp:
VPMatrix` (`world.wgsl:5388`), not a uniform; plus `visible_patch_indices[patch_id]`
(3881) on the indirect pipeline only.
**Uniform reads: exactly one buffer** — `config` (`@group(0) @binding(1)`,
`world.wgsl:5331`), ~8 scalar reads across 5 fields. No other uniform is
reachable from this VS.

**The resting case really is 3 fetches, confirmed CPU-side.** `config.aura_enabled`
is driven to 0 every frame when the pawn's aura presence is at rest —
`c->gpuState_.set_aura_enabled(c->player_.aura_presence > 0.001f)` (`pawn.hpp:128`,
default `aura_enabled = false` at `pawn.hpp:77`) — so the guard at
`world.wgsl:5682` genuinely closes, rather than merely being closeable.

### `shadow_patch_terrain_vs` (`world.wgsl:4186-4222`)

| Quantity | Verdict | Note |
|---|---|---|
| base height | **TEXTURE FETCH** | same texture, same bilinear tap (4204) |
| baked gradient | **FETCHED, UNUSED** | `.yz` discarded |
| wave + pulse Δh | **TEXTURE FETCH** | `sample_live_card(...).x` (4211) |
| live gradient | **FETCHED, UNUSED** | `.yz` discarded |
| GoL lift | **TEXTURE FETCH** | `ug_cell_lift` (4215) |
| GoL suppression | **ANALYTIC** | `pawn_gol_suppression` (4216) |
| pawn aura | **ABSENT** | not sampled — see F-7 |

**Fetches per vertex — `shadow_patch_terrain_vs`: 3, all unconditional.**
Resting cost **equals** worst-case cost: no branch gates any memory access in
this VS (`ug_decode`'s if/else chain selects lanes but every path is fetch-free).
**Storage loads per vertex: 3** — `patch_instances[patch_id]` (4190),
`render_agents[...]` via `render_pawn_pos()` (4216, called **once** here vs twice
in the eye VS), and `render_vp.light_vp` (4220). **No indirection load** — the
shadow pipeline is created with no constant overrides (`renderer.hpp:2061`).

### The one analytic term, and what carrying it would cost

`pawn_gol_suppression` (`world.wgsl:2663-2666`) is, in full:

```wgsl
return 1.0 - smoothstep(ZONE_SUPPRESS_INNER, ZONE_SUPPRESS_OUTER,
                        distance(world_xz, pawn_xz));
```

One `distance`, one `smoothstep` — roughly 5–8 ALU ops, no loop, no noise
octave, no fetch. **Carrying it in a resource would cost more than it saves:** it
would add a 4th/5th texture fetch (~hundreds of cycles of latency) to remove
~5 ALU ops. **H1's fork verdict is right and its lever conclusion is right, but
this specific sub-lever is inverted — do not resource this term.**

---

## [B] THE FIELD LEDGER

Rates use the outdoor-resting census from [D3] (49 LOD0 / 104 LOD1) and the
LOD0 referenced-vertex count from [D1] (10,752/patch).

| FIELD | PRODUCERS (+when) | RESOURCE | RES (wu/texel) | CONSUMERS | RATE PAID | CHANGE RATE |
|---|---|---|---|---|---|---|
| base height | `generate_patch_heights` @16×16, per patch stream-in | `patch_heightfield_array` `.x` | 0.196078 | eye VS 3900; shadow VS 4204 | ~653K vtx × 2 passes | **per patch stream-in** (never after) |
| baked gradient | `generate_patch_gradients` @16×16, per stream-in | same `.yz` | 0.196078 | eye VS 3900 (used); shadow VS 4204 (**discarded**) | same tap | per stream-in |
| wave Δh | `write_live_card_heights`, **every frame** | live card `.x` (fused w/ pulse) | 1.5625 | eye VS 3923; shadow VS 4211; ~26 entity/flora VS | ~653K×2 + entities | per frame *(zero at rest)* |
| pulse Δh | `contrib_radial_pulses_at` in the same kernel, every frame | live card `.x` — **same channel** | 1.5625 | as above | as above | per frame |
| live gradient | `write_live_card_resolve`, every frame | live card `.yz` | 1.5625 | eye VS 3923 (used); shadow VS (**discarded**) | ~653K×2 | per frame |
| GoL lift | `contrib_gol_zones_at` → card `.a`, every frame | live card `.a` | 1.5625 | `ug_cell_lift` in **both** VS; ~10 placement compute sites | ~653K×2 | **per GoL tick** |
| GoL suppression | `pawn_gol_suppression` **analytic only** | **NONE** | — | both VS | ~653K×2 | per frame (pawn move) |
| pawn aura | `dispatch_compute_pawn_aura` | `pawn_aura_read` | `PATCH_CELL_SIZE`=3.125 | eye VS 3915 **only** | guarded; ~0 at rest | per frame |
| cell/region identity | `ug_decode` index math | **NONE** (derived) | — | both VS | ~653K×2 | **never** (pure fn of `vertex_index`) |
| checker value | `animated_cell_color` **analytic**, FS | **NONE** live | — | terrain FS 4043 | per pixel, **gated** `has_mode_bias` | per frame when music on |
| colour | `generate_patch_cells`, per stream-in | `patch_cell_color_array` | 3.125 | terrain FS 4014 `textureLoad` | per pixel | per stream-in |
| fog input | `shade_lit`, analytic FS | **NONE** | — | terrain FS | per pixel | per frame |

**`[R-RATE]` marks** (rate paid exceeds change rate, or exceeds the resource's own
resolution):
- **base height / baked gradient** — resource resolution 0.196078 wu, consumed at
  0.78125 wu vertex spacing. The mesh expresses **one quarter of the baked
  field's detail per axis**; 16 texels back every vertex. Change rate: *never*
  after stream-in. `[R-RATE]`
- **wave/pulse Δh, live gradient, GoL lift** — resource resolution 1.5625 wu,
  consumed at 0.78125 wu ⇒ **4 vertices read every card texel** (2 per axis),
  **twice** (eye + shadow). Change rate for `.a` is per GoL tick, paid per frame
  per vertex per pass. `[R-RATE]`
- **cell identity** — change rate *never*, recomputed per vertex per pass. (Cheap
  index math; listed for completeness, not a lever.)
- **the card itself** — 819,200 producer invocations/frame against a change rate
  that is zero at rest. `[R-RATE]` (F-4)

**`[R-COMPUTE]` marks** (more than one live producer AND a consumer reading the
computed form rather than the stored one):
- **checker/cell colour** — `patch_cell_color_array` holds the gen-time composite,
  but when `has_mode_bias` is true the FS calls `animated_cell_color(cell_center,
  addr_used)` at **`world.wgsl:4043`** and *replaces* the fetched value. The
  comment at 4040-4041 names this deliberately: *"the bake's evaluator, at the
  cell center — one function, two moments, no cache."* One value, two live
  producers, consumer reads the computed one. `[R-COMPUTE]` — see E3.
- Everything else in the terrain VS path is **single-producer**. The Ground Card
  did win that war; H1 is right about the fork.

---

## [C] THE RATIOS

All values verbatim from `state.hpp:73-207`.

| # | Quantity | Expression | Value (wu/texel) |
|---|---|---|---|
| 1 | mesh vertex spacing LOD0 | `PATCH_EXTENT / PATCH_MESH_N` = 50/64 | **0.78125** |
| 2 | mesh vertex spacing LOD1 | `PATCH_EXTENT / PATCH_MESH_N_LOD1` = 50/32 | **1.5625** |
| 3 | cell size | `PATCH_EXTENT / PATCH_CELL_N` = 50/16 | **3.125** |
| 4 | live card texel | `LIVE_CARD_EXTENT / LIVE_CARD_SIZE` = 1000/640 | **1.5625** |
| 5 | baked heightfield texel | `PATCH_EXTENT / (PATCH_HEIGHTFIELD_N − 1)` = 50/**255** | **0.196078** |
| 6 | sun shadow-map texel | (2 × `SUN_HALF_EXTENT`) / `SHADOW_MAP_SIZE` = 600/4096 | **0.146484** |

**Row 4 convention check (this changes the answer by 2×, so it is evidenced).**
`LIVE_CARD_EXTENT = 1000` is the **full** extent, not a half-extent. Three
independent confirmations: (a) `state.hpp:101-102` states the guaranteed
half-extents as `EXTENT/2 − PATCH_CELL_SIZE = 496.875` and `EXTENT/2 = 500.0`;
(b) `live_card_uv` (`world.wgsl:8446`) divides by `LIVE_CARD_EXTENT` with no
factor of 2 and no `*0.5`; (c) the writer steps
`texel = LIVE_CARD_EXTENT / f32(LIVE_CARD_SIZE)` (`world.wgsl:8487`).

**Row 5 is a fence-post, not a division.** The bake samples **inclusively**:
`generate_patch_heights` uses `let uv = vec2<f32>(id.xy) / (res_f - 1.0);`
(`world.wgsl:7902`) with `id ∈ [0,255]` — 256 samples spanning **255** intervals.
So the baked texel pitch is `50/255 = 0.196078`, not `50/256 = 0.1953125`. The VS
remaps to match: `let sample_uv = (uv * (res - 1.0) + 0.5) / res;`
(`world.wgsl:3896`). Small, but it propagates into the mesh:baked ratio below.

**Row 6 verified, not assumed — and the obvious source is dead code.**
`SUN_HALF_EXTENT = 300.0` (`world.wgsl:3245`) feeds a projection with no
perspective row (`world.wgsl:3294-3299`), written live at `world.wgsl:7854`. The
C++ `compute_sun_matrices` (`render_passes.hpp:519`, `half_extent = 350.0f`) has
**no callers** and does not run. See the correction box in F-2.

### The four ratios

| Ratio | Value | Operational meaning |
|---|---|---|
| **mesh : card** | 1.5625 / 0.78125 = **2 per axis** — but see below | Naively 4 vertices per card texel. **The real figure is higher**: the unified ground does not use a shared grid. One cell = 4 card texels and carries `UG_CAP_VERTS_PER_CELL = 25` cap verts ⇒ **6.25 cap vertices per card texel**, or **10.25** counting the 16 curtain-bottom verts, or ~10.5 with skirt copies. Each is tapped twice (bilinear + nearest), in two passes. |
| **mesh : baked** | 0.78125 / 0.196078 = **255/64 = 3.984 per axis** | **15.875 baked texels per LOD0 quad.** The mesh expresses roughly one quarter of the baked field's detail per axis; the rest is inexpressible by LOD0 geometry. |
| **mesh : shadow** | 0.78125 / 0.146484 = **16/3 = 5.333 per axis** | **28.4 shadow texels per LOD0 quad.** The sun's target resolves 5.33× finer than the mesh and cannot benefit from the density it is given. Against LOD1: 10.67×. |
| **cell : card** | 3.125 / 1.5625 = **exactly 2** | One patch cell is exactly two card texels per axis — the relation the nearest `.a` fetch stands on. |

**The cell-exactness `static_assert` still holds.** `state.hpp:119-122`, in
integer arithmetic: `2·1000·16 == 50·640` ⇒ `32000 == 32000`. ✔
Companion float form `state.hpp:125-126`: `2·1000.0 == 3.125·640` ⇒
`2000.0 == 2000.0`. ✔ Dispatch-divisibility assert `state.hpp:133-135`:
640 % 8 == 0 and 640 % 16 == 0. ✔ Band arithmetic assert `state.hpp:193-196`:
`UG_QUADS_PER_CELL==4, UG_CAP_BASE==4481, UG_BASE_BASE==10881,
UG_DECODE_VERTS==14977`. ✔ All four hold.

---

## [D] THE DRAW CENSUS

### D1 — PER TILE

Built at `state.hpp:3166-3243` (LOD0) and `state.hpp:3245-3278` (LOD1).

**LOD0 — `patchIndexCount_` = 50,688 indices = 16,896 triangles**

| Band | Recipe | Quads | Triangles | Indices | Share |
|---|---|---|---|---|---|
| legacy grid | *not emitted at LOD0* | 0 | 0 | 0 | 0% |
| cap | 256 cells × 16 quads (`state.hpp:3179-3191`) | 4,096 | 8,192 | 24,576 | 48.48% |
| **curtain** | 256 cells × 16 quads (`state.hpp:3195-3210`) | 4,096 | 8,192 | 24,576 | **48.48%** |
| skirt ring | `SKIRT_RING`=4×64=256 quads (`state.hpp:3227-3235`) | 256 | 512 | 1,536 | 3.03% |
| **total** | | 8,448 | **16,896** | **50,688** | 100% |

**LOD1 — `patchIndexCountLOD1_` = 6,912 indices = 2,304 triangles**

| Band | Recipe | Triangles | Indices |
|---|---|---|---|
| legacy grid @ step 2 | 32×32 quads (`state.hpp:3250-3259`) | 2,048 | 6,144 |
| skirt ring @ step 2 | 256/2 = 128 quads (`state.hpp:3262-3270`) | 256 | 768 |
| **total** | | **2,304** | **6,912** |

**Unique referenced vertices — the number that is actually paid**

| | LOD0 | LOD1 |
|---|---|---|
| cap band | 256 × 25 = 6,400 | 0 |
| base (curtain-bottom) band | 256 × 16 = **4,096** | 0 |
| skirt ring copies | 256 | 128 |
| legacy grid | **0 — never referenced at LOD0** | 33×33 = 1,089 |
| **unique referenced** | **10,752** | **1,217** |
| `UG_DECODE_VERTS` (decode space) | 14,977 | 14,977 |

**Which is paid.** `UG_DECODE_VERTS = 14,977` (`state.hpp:192`) is the size of the
*decode space* — the domain `ug_decode` can address — **not** an allocated vertex
buffer and **not** the invocation count. These are `DrawIndexed` calls with no
vertex buffer (`renderer.hpp:741-756`); the VS is invoked from the index buffer.
So the ceiling is **50,688 invocations/patch** (one per index, no cache) and the
floor is **10,752** (perfect reuse). Real hardware sits near the floor for the
cap band — a 5×5 cap tile has excellent locality — and near it for curtains too,
since each curtain quad's two cap verts were just emitted. **`[INFERRED]`: the
post-transform cache hit rate is hardware-dependent and not readable from this
tree; [D7] shows the arithmetic both ways.** The load-bearing point does not
depend on it: **the legacy grid's 4,225 vertices are dead weight in the LOD0
decode space**, and the cap band spends **6,400 vertices to carry the 4,225
vertices' worth of surface** the legacy grid used to carry — a **+51% vertex cost
for identical triangle count**, the price of per-cell independent lift.

### D2 — DEGENERATE SHARE

**Condition, precisely.** From `ug_decode` (`world.wgsl:307-378`): cap-band verts
carry `lift_scale = 1.0`; base-band verts carry `lift_scale = 0.0`. Both VS then
apply `world_pos.y += lift * d.lift_scale - d.drop` (`world.wgsl:3932`, `4217`).
A curtain quad joins cap perimeter vert `k` to base twin `k` (`state.hpp:3203-3208`),
so its vertical span is exactly `lift`. **Zero area ⟺ `lift == 0` ⟺
`ug_cell_lift(...) × (1 − pawn_gol_suppression(...)) == 0` ⟺ the card's `.a` is 0
at that cell's centre ⟺ no GoL zone covers that cell.**

> **H2's stated reason is wrong.** H2 says a curtain is degenerate "whenever a
> cell and its neighbour share lift." There is no neighbour comparison anywhere
> in the curtain construction. The curtain is a **per-cell skirt from the cell's
> own cap down to its own unlifted base twin** — it is degenerate when *that
> cell's own* lift is zero, entirely independent of its neighbours. H2's
> *conclusion* (most curtains degenerate most of the time) is nonetheless
> correct, and for a **stronger** reason: at rest, every cell has zero lift, so
> **100%** of curtains are degenerate, not merely those matching a neighbour.

| Case | Condition | Degenerate curtain quads/patch | Degenerate triangles/patch |
|---|---|---|---|
| **resting** (no zone covers the patch) | card `.a` ≡ 0 | 4,096 of 4,096 = **100%** | 8,192 = **48.48% of the tile** |
| **GoL everywhere** (worst case) | every cell lifted, unsuppressed | 0 | 0 |
| **pawn-suppressed core** | within `ZONE_SUPPRESS_INNER` of the pawn | those cells' curtains collapse | — |

**The active case is worse than H2 implies, not better.** H2 reasons that
curtains collapse "whenever a cell and its neighbour share lift". In precisely
that case — two adjacent cells both lifted to the *same* height — each cell still
emits its **own** full-area curtain on the shared edge. The two curtains sit
back-to-back, mutually occluded, and under `cullMode = Back / frontFace = CCW`
(`renderer.hpp:1626-1628`) one of the pair is backface-culled while the other
rasterizes a wall buried inside the terrain. So a uniformly-lifted region pays
**full-area, fully-hidden** curtain geometry — the one case where the waste is
rasterized rather than collapsed at setup.

**Bounding "most of the world most of the time" — supportable, not measured.**
`MAX_GOL_ZONES = 8` (`state.hpp:280`) and a zone spans at most
32 × `PATCH_CELL_SIZE` = 100 wu per side (`gol_zones.hpp:265-278`), so live zones
cover **≤80,000 wu²** against a ~122,500 wu² LOD0 region — and that is the
ceiling, with all eight zones live and maximally sized. The resting claim (zero
zones ⇒ 100% degenerate) is exact; the "most of the time" clause is an upper
bound from constants, not a measurement, and is marked `[INFERRED]`.

**Suppression mechanism: none.** See F-1. The IB is immutable after init.

**A side observation on the slab invariant.** `world.wgsl:353-354` states the
law: *"the card's raw GoL (`.a`, nearest) at the CELL CENTER — every vert of a
cell reads one value; cells lift as slabs."* That holds for the **lift** term,
which is cell-constant. But it is multiplied by
`(1 - pawn_gol_suppression(world_pos.xz, ...))`, which is evaluated **per
vertex** at the vertex's own position (`world.wgsl:3931`, `4216`). Inside the
suppression annulus the product is therefore **not** cell-constant, and cells do
not lift as slabs — they tilt. Not a rate finding; recorded because the invariant
is stated as law two lines above the code that qualifies it, and because it
bears on any lever that assumes per-cell uniformity.

### D3 — TILES PER FRAME PER PASS

Governing constants: `PATCH_PREGEN_RADIUS = 8` (`state.hpp:83`),
`MAX_ACTIVE_PATCHES = 289` (`state.hpp:85`), `LOD0_RADIUS_DEFAULT = 175`
(`state.hpp:161`), `VEIL_RING_DEFAULT = 325` (`state.hpp:162`).

**Recipe.** `patch_system.hpp:389-416` iterates active patches; metric is
**nearest-edge** — `patch_distance_sq(point, origin, half)` with `half = 25`.
`d2 <= ring_sq` admits to the draw set; `d2 <= lod0_sq` selects LOD0. Reproduced
by enumerating the 17×17 pregen window and applying both tests
(`max(0,|dx|−25)² + max(0,|dz|−25)²`), swept over anchor position within a patch:

| Anchor | LOD0 | LOD1 | **Draw set** | pregen-only | Resident |
|---|---|---|---|---|---|
| patch centre | 49 | 104 | **153** | 136 | 289 |
| patch corner | 52 | 112 | **164** | 125 | 289 |
| edge midpoint | 56 | 106 | **162** | 127 | 289 |

**Residency ≠ submission — two different sets.** Residency is the full 17×17 =
**289** patches (`MAX_ACTIVE_PATCHES`); submission is the ring-gated **153–164**.
The remainder are packed into a third `pregen` block
(`patch_system.hpp:414, 422`) — uploaded to `patch_instances` but never drawn.
`lod0_patch_count` and `render_patch_count` are set at
`patch_system.hpp:425-426`; `all_patch_count` (289) at `:427`.

**The frustum-cull workgroup bug is FIXED — the old doc is a fossil.** Verified
against the live tree at `renderer.hpp:515-525`:

```cpp
// ceil(MAX_ACTIVE_PATCHES / 64) — derived, never hardcoded again
// (was hardcoded 4 = 256 threads vs 289 slots: slots 256–288 were
//  never culled at full window — audit CC-8a).
pass.DispatchWorkgroups((Dim::MAX_ACTIVE_PATCHES + 63u) / 64u, 1, 1);
```

ceil(289/64) = **5** workgroups = 320 threads ≥ 289 slots. Correct today.

### D4 — DENSITY FALLOFF

**One step, at 175 wu. Nothing else.** See F-8. Stated plainly: **the far field
paying near-field density is a finding, not a design** — and the step's own
leverage is only 7.33×, not the 4× the "half resolution" naming implies.

### D5 — THE DOUBLE DRAW

`draw_shadow_all` (`render_passes.hpp:322-344`) issues, in order:
1. **Terrain LOD0** — `draw_shadow_patch_terrain(..., patch_index_count(),
   lod0_patch_count)` — the eye's IB, the eye's count, **direct, uncled**.
2. **Terrain LOD1** — an inline `pass.DrawIndexed(patch_index_count_lod1(),
   render_patch_count − lod0_patch_count, 0, 0, lod0_patch_count)`
   (`render_passes.hpp:332-336`).
3. **The drawable table**, shadow-filtered — `draw_table(..., DRAW_SHADOW)`
   (`render_passes.hpp:339-343`), with a `DrawBind` carrying the shadow texture
   group and a ribbon-presence flag.

**Does the shadow VS compute anything depth-only cannot use?** No — see F-6. It
outputs `clip_pos` alone. The only waste is fetch *width* (`.yz` of two vec4
taps, discarded).

**Coarsening: none.** Same index buffers, same density, same LOD split as the
eye, byte for byte.

**Culling: neither.** Not against the light volume, not against the eye's result.
See F-2.

### D6 — THE INDOOR MULTIPLIER

`draw_shadow_all` is issued **once per live spot light**, each inside its own
`BeginRenderPass` (`render_passes.hpp:264-297`), bounded by
`cpuSpotLights_.count` and `MAX_SPOT_LIGHTS = 4`. The atlas: lights 0–1 write the
**sun** map (idle indoors), lights 2–3 the **spot** map; each takes a half-width
viewport `TILE_W = 2048 × TILE_H = 4096` (`render_passes.hpp:261-262, 272-273`),
with `depthLoadOp = Clear` only on the even light (`:279`).

**Live spot count.** Bounded by 4. The per-mood light scheme is built in
`mood.hpp` (`LightSlotDef slots[MAX_SPOT_LIGHTS]`, `:356`; `LightSchemeSlot
slots[4]`, `:301`), with a vault-ceiling branch adding a slot at `mood.hpp:508`
(`if (ceiling_type == CeilingType::VAULT && count < MAX_SPOT_LIGHTS)`). **The
exact per-mood count is table-driven and I did not enumerate every indoor mood's
resolved value from the tables — stated as a gap, not guessed.** The multiplier
is `1 + count` full terrain passes, `count ∈ [1,4]`.

**Is terrain submitted where it cannot be seen?** Terrain **is** the indoor floor
(shell = ceiling + walls only, `state.hpp:210`), so it is visible. But the
**ring gate is bypassed** (`patch_system.hpp:405`), so patches *outside the room's
walls* — invisible behind the shell — are submitted at full LOD0 density too, in
every one of those passes.

**Shell counts.** `SHELL_MAX_VERTICES = 2048`, `SHELL_MAX_INDICES = 8192`
(`state.hpp:211-212`) ⇒ **≤2,730 triangles**, versus 422,400 terrain triangles per
pass: **the floor beneath the bare room is ~155× the room.**

### D7 — ARITHMETIC

Vertex figures given as **cached** (unique referenced verts — the realistic
floor) and **uncached** (one invocation per index — the ceiling).

**Scene 1 — outdoor resting** (49 LOD0 / 104 LOD1, patch-centre anchor)

| | Triangles | VS (cached) | VS (uncached) |
|---|---|---|---|
| eye LOD0 (49 × 16,896 / × 10,752 / × 50,688) | 827,904 | 526,848 | 2,483,712 |
| eye LOD1 (104 × 2,304 / × 1,217 / × 6,912) | 239,616 | 126,568 | 718,848 |
| **eye terrain** | **1,067,520** | **653,416** | **3,202,560** |
| shadow terrain (identical, uncled) | 1,067,520 | 653,416 | 3,202,560 |
| **frame total** | **2,135,040** | **1,306,832** | **6,405,120** |
| *of which degenerate curtain* | *802,816 (37.6%)* | *401,408 base verts* | — |

*Caveat:* the eye's LOD0 draw is frustum-culled indirect, so its 49 is an upper
bound — the surviving count is GPU-side and not readable here `[INFERRED: a
typical camera frustum retains roughly a third to a half]`. The shadow's 49 is
**not** culled and is exact. The LOD1 draws are uncled in both passes.

**Scene 2 — outdoor, GoL active over the LOD0 core**
Triangle and vertex counts are **identical** — the IB is static. What changes is
only that curtain quads acquire area, converting 802,816 zero-area triangles into
rasterizing ones. **Geometry cost unchanged; fill cost rises.** This is why GoL
activity does not move the fixed 12.4 ms: the geometry was always being paid.

**Scene 3 — indoor**, `p = (2r+1)²` patches, all LOD0, `n` live spots.
Per pass: `p × 16,896` triangles, `p × 10,752` VS. Total passes = `1 + n`.

| r | p | Triangles/pass | n=1 total | n=4 total | VS (cached), n=4 |
|---|---|---|---|---|---|
| 1 | 9 | 152,064 | 304,128 | 760,320 | 483,840 |
| **2 (default)** | **25** | **422,400** | **844,800** | **2,112,000** | **1,344,000** |
| 3 | 49 | 827,904 | 1,655,808 | 4,139,520 | 2,634,240 |
| 4 | 81 | 1,368,576 | 2,737,152 | **6,842,880** | 4,354,560 |

Indoor shell across all passes: ≤2,730 × (1+n) ⇒ **≤13,650 at n=4** — against
2.1M–6.8M triangles of floor. The worst indoor case exceeds the outdoor frame
(2,135,040) by **3.2×**.

---

## [E] THE CARD AND THE UNSPENT HEADROOM

### E1 — CHANNELS AND REST

Writer: `write_live_card_heights` (`world.wgsl:8483`) → scratch;
`write_live_card_resolve` (`world.wgsl:8513`) → the texture.

| Ch | Holds | Written by | Rest condition |
|---|---|---|---|
| **R** | Δh: true-band waves **+** radial pulses, fused | pass 1 `dh` (`:8489-8506`) → pass 2 store | `config.terrain_time <= 0` gates the band loop (`:8491`) **AND** the pulse ring is empty |
| **G,B** | ∂x/∂z of the **whole** Δ field (bands *and* pulses) | pass 2, 3-point stencils over a 20×20 cooperative tile | zero when R is zero everywhere |
| **A** | raw GoL lift | pass 1 `contrib_gol_zones_at(p)` (`:8508`) | no zone covers the texel |

**The three-conjunct rest law** is stated at `world.wgsl:8458-8470`: the card is
zero — and every consumer therefore adds 0 — only when **all** of (1)
`config.terrain_time <= 0`, (2) the pulse ring is empty, (3) no zone covers the
texel. Conjuncts 1 and 2 are musical; 3 is not.
**The law governs the value, not the work — see F-4. There is no dispatch-level
rest skip anywhere.**

### E2 — CONSUMERS

Enumerated from every call site of `sample_live_card` / `sample_live_card_gol` /
`live_card_read` in `world.wgsl` (~30 sites). Sampler per
`world.wgsl:8449-8457`: `sample_live_card` = **bilinear**, `sample_live_card_gol`
= **nearest**.

| Consumer | Pass · stage | Sampler | Rate |
|---|---|---|---|
| `patch_terrain_vs:3923` | main · VS | bilinear | **~653K/frame** |
| `ug_cell_lift` ← `patch_terrain_vs:3930` | main · VS | **nearest** | **~653K/frame** |
| `shadow_patch_terrain_vs:4211` | shadow · VS | bilinear | **~653K/frame ×(spots)** |
| `ug_cell_lift` ← `shadow_...:4215` | shadow · VS | **nearest** | **~653K/frame ×(spots)** |
| `patch_terrain_fs:3975` | main · FS | bilinear | per pixel — **`DEBUG_VIEW == 5` only** |
| flora/entity VS `:4757, 4772, 4785, 4800` | main · VS | bilinear | per entity vertex |
| `:9589, 10820, 10841, 11157, 11175, 11393, 11411` | main/shadow · VS | bilinear | per entity vertex |
| manifold/placement `:2960, 2978, 3065-3066, 3094` | compute | both | per query |
| ground-atlas placement `:9027, 9047, 9057, 9068, 9079, 9090-9091` | compute | nearest | per slot, per frame |

The terrain VS pair alone accounts for **~2.6M taps/frame** outdoors (F-5).

### E3 — SITES STILL COMPUTING WHAT A RESOURCE ALREADY HOLDS

**One, and it is per-pixel, not per-vertex.** `patch_terrain_fs:4039-4044`:

```wgsl
} else if (has_mode_bias) {
    let cell_center = (vec2<f32>(addr_used) + 0.5) * PATCH_CELL_SIZE;
    base_color = animated_cell_color(cell_center, addr_used);
}
```

`patch_cell_color_array` already holds the gen-time composite and is fetched two
lines earlier at `:4014` via `textureLoad`; when `has_mode_bias` is true the
fetched value is **discarded and recomputed analytically per pixel**.
`[R-COMPUTE]`

**The checker-LUT retirement** is the mechanism: the comment at `:4040-4041`
names it — *"SAMPLE-POINT + ANALYTIC (charter C8): the bake's evaluator, at the
cell center — one function, two moments, no cache."* The retirement moved
`animated_cell_color` (and the checker field beneath it) **from a lookup into the
live per-pixel path**, gated by `has_mode_bias` (`:4026-4029`), which is true
whenever `mode_color_shift`, `mode_checker_scatter`, `mode_palette_intensity` or
`checker_music_amount` leaves rest — i.e. **whenever music plays**.

**Rate:** per pixel of terrain, main pass only, and **after** the rim `discard`
at `:3968` — so it is paid only on surviving fragments. **Under this campaign's
own ranking rule (invocations × passes, per-pixel-in-a-discarded-fragment is
free) this ranks BELOW every geometry finding above**, and it is absent from the
shadow pass entirely. It is reported because [B] and [E3] asked for it, not
because it is a lever worth pulling while the frame is geometry-bound.

**No other site** recomputes a card-held value: the terrain VS reads Δh,
gradient and lift exclusively from the card. **The Ground Card's "computed in
three places" war is genuinely won.**

### E4 — THE HEADROOM LEDGER

**Device limits.** `console.hpp:170-175` requests the **adapter's full limits**
(`adapter.GetLimits(&adapterLimits); deviceDesc.requiredLimits = &adapterLimits;`)
— the tree takes whatever the adapter offers rather than pinning a floor, and
logs `storageBuffers/stage`, `uniformBuffers/stage`, `bindingsPerGroup`
(`:187-191`). The last recorded witness (`audit/probe_results_post_ug1.json`,
SwiftShader) reports `maxStorageBuffersPerShaderStage = 10`,
`maxUniformBuffersPerShaderStage = 12`, `maxStorageTexturesPerShaderStage = 4`.
**Those are SwiftShader's numbers, not the shipping adapter's** — the tree
contains no witness from the real device.

**Pipelines touching terrain and their groups** (from the same witness plus
`state.hpp` layout construction): the terrain draws bind exactly two groups —
group 0 `render_entity_group()` and group 1 `render_texture_group()` /
`shadow_texture_group()` (`renderer.hpp:725-756, 999-1016`). The card writer
binds one, `"Live Card Writer Layout"`. Terrain-relevant group-1 bindings sit at
`:31` `zone_life_read`, `:32` `zone_params`, `:33` `pawn_aura_read`, `:34`
`live_card_read` (`world.wgsl:5724-5728`); patch storage at `@group(0)` `:340`
`patch_instances`, `:391` `visible_patch_indices` (`world.wgsl:5469-5470`).
**A precise slot-count-against-cap table per stage is the one section of this
audit I could not complete to evidence standard** — the layouts are assembled
across several hundred lines of `state.hpp` entry arrays and I did not enumerate
every entry for every one of the four pipelines. Stated as a gap rather than
estimated.

**What did the reallocation campaign buy, and is any of it spent?**
It bought exactly one thing that reduces invocations: the **frustum-cull compute
+ `DrawIndexedIndirect`** path — `visible_patch_indices` (binding 391),
`frustum_indirect_lod0()`, and the `USE_PATCH_INDIRECTION` override pipeline. It
is spent on **one draw in the entire frame**: the eye's LOD0 terrain
(`render_passes.hpp:374-382`). It is **not** spent on the eye's LOD1 draw (always
direct, `:395`), **not** on any shadow draw (`draw_shadow_all` is entirely
direct), and **not at all indoors** (`render_passes.hpp:230` returns early). So of
the four full-density terrain draws in an outdoor frame, **one** is culled; of
the five-to-eight in an indoor frame, **none**. The headroom is real and the
indirection exists — it is simply pointed at the smallest of the available
targets. **H7's thesis is correct in substance.**

**Multi-draw indirect — the H7 sub-question.**
- The comment is live and appears at three code sites:
  `render_passes.hpp:395` ("Dawn D3D12 limit: only one indirect per pass"),
  `state.hpp:1838`, `state.hpp:3106` — plus `audit/CC_AUDIT_REPORT.md:153` and
  `src/docs/old docs/the_board_seam_map.md:2613`. A *distinct* constraint,
  "Dawn D3D12 can't share Storage|Indirect", forces the extra
  `CopyBufferToBuffer` at `render_passes.hpp:247-252`.
- **`dawn.json` is not in this tree.** Dawn is external:
  `set(DAWN_DIR "C:/dev/dawn" ...)` (`CMakeLists.txt:13`). The work order's claim
  that dawn.json exposes `multi draw indirect` / `multi draw indexed indirect`
  therefore **cannot be verified or refuted from this repository** — stated as an
  absence.
- **Does the device request it? No.** `console.hpp:181-185` requests exactly one
  feature: `wgpu::FeatureName::TimestampQuery`.
- **Does the adapter report it? Never asked.** The only `HasFeature` calls in the
  tree are for `TimestampQuery` (`console.hpp:182`, `cartridge.hpp:427`,
  `state.hpp:3055`). The witness harness `audit/probe_dawn_witness_post_ug1.mjs`
  enumerates `adapterLimits` and `wgslLanguageFeatures` but **never
  `adapter.features`** — and ran against `--use-webgpu-adapter=swiftshader`
  (`:63-64`).
- **Fossil or live constraint?** Undecidable by reading, but what *is* settled:
  **the constraint is self-imposed today.** Multi-draw could not be used even if
  Dawn offered it, because the feature is never requested. Whether it is
  *available* is an F4 probe question.
- **Render bundles: absent.** `grep -rn "RenderBundle"` over `src/` returns
  nothing; neither pass uses them, so the multi-draw-in-bundles question is moot
  here.

---

## [F] RECONCILIATION, FINDINGS, LEVERS

### F1 — DOES THE READING PREDICT THE MEASUREMENT?

**Back of envelope, outdoor:** ~2.14M triangles and ~1.31M VS invocations per
frame across both passes (cached basis, [D7]), each vertex doing 3 unconditional
texture fetches and ~30 ALU ops. Fetch traffic: ~3.9M taps/frame — negligible
bandwidth. Triangle setup: ~2.14M, of which ~800K are zero-area and die at setup
without fill.

**On a mid-range discrete GPU** (≥1 Gtri/s setup, ≥1 Gvert/s with cached
fetches) this predicts roughly **1–3 ms per pass — not 12.4 ms.**
**On a weak integrated part or a software rasterizer** (SwiftShader, ~10–50
Mtri/s) the same counts land at **20–100+ ms**, and 12.4 + 10 ms sits
comfortably inside that band.

**So: I state loudly that the reading does not, by itself, predict 12.4 ms —
unless the adapter is far weaker than a discrete GPU.** The counts are the right
*order* for a software or low-end integrated rasterizer and roughly an order of
magnitude too small for a discrete one. **The tree cannot tell me which adapter
the measurement came from**, and the one witness it contains was deliberately
forced to SwiftShader. That is the single largest gap in this reading.

Two pieces of evidence do corroborate the geometry-bound thesis independently of
the absolute scale, and both are consistent with the counts above:
- **`shadow_pass ≈ 10 ms` against a depth-only target with no fragment work and
  no colour attachment** can only be vertex/setup cost. [D5] shows the shadow VS
  computes nothing but position. 10 ms of pure geometry for ~1.07M triangles is
  the cleanest single datum in the report, and it scales to the main pass's
  12.4 ms fixed almost exactly 1:1 — which is what [D7] predicts, since the two
  passes submit **identical** terrain geometry.
- **Entity count 43→240 barely moving the floor** is exactly what [D7] predicts:
  terrain is ~2.1M triangles; a couple hundred entities are noise against it.
- **A bare indoor room still paying it** is fully explained by F-3 without any
  appeal to adapter speed.

### F2 — FINDINGS

Ranked at the top of this document, §0. Summary of verdicts on the standing
hypotheses:

| | Verdict | Settled by |
|---|---|---|
| **H1** fork = FETCH | **CONFIRMED** (one correction) | `world.wgsl:3875-3946`, `4186-4222`. Fetches/vertex: **eye 3 unconditional + 1 guarded**, **shadow 3 unconditional**. Correction: the lone analytic term `pawn_gol_suppression` is ~5 ALU ops — resourcing it would *cost* a fetch to save nothing. The density-lever conclusion stands. |
| **H2** curtains half, mostly degenerate | **PARTIAL** | `state.hpp:3192-3210`. Index/triangle arithmetic **exactly right** (50,688 / 16,896). Two corrections: curtains are **48.48%**, not "exactly half" (the skirt ring is the remainder); and the degeneracy condition is **the cell's own zero lift**, not "sharing lift with a neighbour" — which makes the resting-case share **100%**, stronger than claimed. Unique-vertex share **38.1%**. **No suppression mechanism exists.** |
| **H3** sun pays eye density | **PARTIAL** | Mechanism **confirmed** (`render_passes.hpp:324-336` — eye's IB + counts verbatim). **Both numbers wrong:** the ±350 comes from `compute_sun_matrices`, which is **dead code with zero callers**; the live ortho is `SUN_HALF_EXTENT = 300.0` (`world.wgsl:3245`, written at `:7854`). Correct: **0.1465 wu/texel**, **5.33× finer**, not 0.171/4.57. The conclusion holds and strengthens. |
| **H4** indoor × spot count | **PARTIAL** | Mechanism **confirmed** (`render_passes.hpp:264, 295`), and two multipliers H4 did not name make it worse: ring gate bypassed (`patch_system.hpp:405`), culling skipped (`render_passes.hpp:230`). But the **explanatory clause is refuted**: the multiplier floor is **1**, and the unconditional cost needs no multiplier to explain it. Also: indoors is **n** passes, not n+1 — the sun and spot arms are a mutually exclusive if/else (`render_passes.hpp:259/299`); and `finite_radius` is seed-derived over {1..4}, so p ∈ {9,25,49,81}, not a fixed 25. |
| **H5** over/under-sampled | **PARTIAL** | Direction right, three numbers wrong. Baked texel is **50/255 = 0.196078** (fence-post bake, `world.wgsl:7902`), so mesh:baked is **3.984**, not 4. "Four vertices per card texel" understates: the unified ground carries 25 cap verts per cell against 4 card texels ⇒ **6.25**, or **10.25** with the curtain band. And the claim that everything above 32 buys only *baked* fidelity is **refuted** — 48.5% of LOD0 indices are the curtain band, whose entire geometry is the **live** GoL scalar. |
| **H6** shadow uncled | **CONFIRMED** | `renderer.hpp:999-1016` (plain `DrawIndexed`); `world.wgsl:4190` (direct `patch_instances[patch_id]`, no `visible_patch_indices`). Neither light-volume nor eye-result culled. |
| **H7** headroom unspent | **PARTIAL** | The **conclusion** holds — indirection serves **one draw per frame**, none indoors, and none of the three shadow/LOD1/snapshot draws. Multi-draw is **neither requested nor queried** (`console.hpp:181-185`); `dawn.json` is **not in this tree** (`CMakeLists.txt:13`). Render bundles absent. But the **causal story is refuted**: freeing a binding was never the blocker. `visible_patch_indices` is **already bound in the shadow pass** — `entries[13]` in both the Render Entity Layout (`state.hpp:4013`) and its BindGroup (`state.hpp:4994-4996`), which the shadow pipeline shares (`renderer.hpp:1542-1544`). The binding is resident and unused; headroom bought nothing because nothing was missing. |

### F3 — CANDIDATE LEVERS (proposals; Jean rules)

**L-1 — Gate the curtain band out of the draw when no zone is live.**
*Spends* F-1. *Evidence:* `state.hpp:3192-3210`; degeneracy condition in [D2].
*Saving:* the curtain band is a **contiguous index range** (caps `[0, 24576)`,
curtains `[24576, 49152)`, skirt `[49152, 50688)`), so a second IB — or two
`DrawIndexed` calls with an offset, skipping the curtain range — removes
**802,816 triangles and 401,408 VS invocations per outdoor frame** with no
resource changes and no shader edit. *Risk:* low; needs a per-patch or global
"any zone live" predicate, and the resting/active transition must not pop.
*Visual gate:* toggle zones on and off at the boundary; the silhouette must be
identical in the resting case (it provably is — the quads have zero area).
*Blocked by:* nothing.

**L-2 — Stop the sun rendering detail it cannot resolve: give the shadow pass a
coarser terrain.** *Spends* F-2. *Evidence:* `render_passes.hpp:324-336`;
mesh:shadow = **5.33** per axis (28.4 shadow texels per LOD0 quad). *Saving:*
drawing the shadow terrain from the **LOD1** IB at all distances cuts shadow
terrain from 1,067,520 to ~352,512 triangles — **~67% of the shadow pass's
terrain geometry** — and the shadow map still resolves **10.67× finer per axis**
than the LOD1 mesh, so the target remains comfortably over-resolved. *Risk:* silhouette fidelity — shadow edges are the one
place LOD1's coarser baked silhouette becomes visible; peter-panning at cell
lifts if the curtain band is dropped too. *Visual gate:* long shadows across a
lifted GoL zone at low sun angle, LOD0 vs LOD1 shadow, A/B. *Blocked by:* nothing
— `patch_index_buffer_lod1()` already exists and the shadow VS already decodes
the legacy space.

**L-3 — Cull the shadow draw.** *Spends* F-2, F-3. *Evidence:* H6 confirmed.
*Saving:* the shadow set is currently the full 153-patch banded set; culling
against the light frustum would remove whatever falls outside ±350 — and indoors,
culling at all would be new. *Risk:* shadow casters outside the view frustum must
**not** be culled against the eye's result (that is the classic bug); it must be
the light's volume.
*Blocked by — and this is sharper than the binding story suggests:* **the
existing cull result is camera-space and cannot be reused.** The cull compute
reads `let vp = fc_vp.m;` (`world.wgsl:9177`) — the `.m` member of `VPMatrix`
(`world.wgsl:3462-3465`), i.e. the **eye** matrix, not `.light_vp`. Feeding it to
the shadow pass would drop exactly the off-screen casters whose shadows must
still fall into view. A shadow cull needs its **own** dispatch per light against
`light_vp`, plus its own indirect buffer per light — so the
one-indirect-draw-per-pass constraint bites four times indoors. Note the binding
is *not* the obstacle: `visible_patch_indices` is already resident in the shadow
pass's bind group (`state.hpp:4013`, `:4994-4996`). A second, smaller blocker
`[INFERRED]`: `state.hpp:2567` pins the indirect args as
`{ patchIndexCount_, 0, 0, 0, 0 }` with `firstInstance = 0`, and the shader
touches only `args[1]` (`world.wgsl:9208`) — LOD1 needs `firstInstance =
lod0_patch_count`, so the args layout would need widening too.

**L-4 — Restore the ring gate indoors, or clamp `finite_radius`'s draw set.**
*Spends* F-3. *Evidence:* `patch_system.hpp:405`. *Saving:* indoors, patches
behind the walls are drawn at full LOD0 in every one of up to 5 passes; gating
them removes a large fraction of 2,112,000 triangles. *Risk:* the bypass exists
for a reason — "Finite mode: all patches visible (walls define boundary, not
fog)" (`patch_system.hpp:404`). A naive re-gate could clip the floor at a room
edge. *Visual gate:* stand at each wall of the largest indoor mood and look down
and outward; no floor may vanish. *Blocked by:* nothing technical; it is a
correctness-of-intent question for Jean.

**L-5 — Skip the card writer when the three-conjunct rest law holds.**
*Spends* F-4. *Evidence:* `render_passes.hpp:161-170`; `renderer.hpp:538-551`.
*Saving:* 819,200 compute invocations/frame at rest. *Risk:* low, but the card
must be **zeroed once** on the rest transition or consumers read stale non-zero
values; and conjunct (3) — zone coverage — is per-texel, so the CPU-side
predicate must be conservative (any zone live anywhere ⇒ write). *Visual gate:*
enter and leave rest with a zone at the card boundary. *Blocked by:* nothing.

**L-6 — Merge the two live-card taps.** *Spends* F-5. *Evidence:*
`world.wgsl:8449-8457`. *Saving:* one of ~2.6M taps/frame. *Risk:* the two taps
use **different samplers by design** — bilinear for smooth Δh, nearest for
cell-exact `.a` (the cell-exactness assert, `state.hpp:119`, exists precisely to
make the nearest fetch legitimate). Merging to a single bilinear tap would
**break cell-exactness and violate the SEAMLESSNESS treaty**; merging to a single
nearest tap would quantise the wave field to 1.5625 wu and produce visible
terracing. **Recommend NOT pulling this lever** — it is listed for completeness
and to record why it is closed.

**L-7 — Spend the headroom: indirect + cull the LOD1 and shadow draws.**
*Spends* F-2, H7. *Blocked by* the one-indirect-draw-per-pass constraint **as
currently believed**. The first move is not a lever but a probe (F4-a): ask the
adapter what it actually supports.

### F4 — WHAT THE READING CANNOT SETTLE

Only these earn a probe build.

1. **Which adapter produced the 12.4/10 ms, and its triangle/vertex throughput.**
   This is the F1 gap and the most important unknown in the report: it decides
   whether the counts here *explain* the measurement or whether something outside
   this reading does. *Probe: log adapter info + a triangle-throughput microbench
   at boot.* (Answers the parked **primitive census in the meter**.)
2. **Whether Dawn/D3D12 on the shipping adapter actually reports
   `multi-draw-indirect` / `multi-draw-indexed-indirect`.** Never requested,
   never queried, and `dawn.json` is outside this tree. *Probe: enumerate
   `adapter.features` — a three-line addition to the existing
   `probe_dawn_witness` harness, which today enumerates limits but not features,
   and must be re-run on the real adapter rather than SwiftShader.*
3. **The post-transform vertex-cache hit rate**, which decides whether LOD0 costs
   10,752 or 50,688 VS invocations per patch — a 4.7× spread that changes every
   estimate in [D7]. *Probe: pipeline statistics query, or the parked
   **primitive census in the meter**.*
4. **The actual resolved live spot-light count per indoor mood**, which sets the
   indoor multiplier between 2× and 5×. Table-driven in `mood.hpp`; resolvable by
   further reading, but I did not complete it. *Probe or further read: log
   `cpuSpotLights_.count` per mood.*
5. **How much of the 6.4 ms fill is terrain versus everything else**, and whether
   the rim `discard` at `world.wgsl:3968` is killing enough fragments to make the
   `has_mode_bias` recompute (E3) irrelevant. *Probe: the parked
   **shadow-terrain-drop probe** and **shadow-map-resolution probe** bound L-2
   directly; the parked **patch-count probe** bounds L-1 and L-4.*

Every parked item from the withdrawn GEOMETRY_1 batch is named above by the
question it answers. None is built by this document.

---

## APPENDIX — RECIPES

**Patch census ([D3]).** Enumerate `gx, gz` over `[c−8, c+8]²`
(`PATCH_PREGEN_RADIUS = 8`); patch origin `((g+0.5)·50, (g+0.5)·50)`; nearest-edge
metric `d² = max(0,|Δx|−25)² + max(0,|Δz|−25)²` (`patch_system.hpp:402`,
`half = PATCH_EXTENT·0.5`); admit to draw if `d² ≤ 325²`, to LOD0 if `d² ≤ 175²`.
Swept over anchor position within a patch. Script:
`scratchpad/counts.py` (not committed — the eight lines above reproduce it).

**Index bands ([D1]).** Read directly off the three emission loops at
`state.hpp:3179-3191` (cap), `:3195-3210` (curtain), `:3227-3235` (skirt);
each `push_back` group of 6 is one quad. LOD1 from `:3250-3270`.

**Unique referenced vertices ([D1]).** Cap loop touches all 25 verts of each
cell's 5×5 tile ⇒ 256×25. Curtain loop adds `base0 + k`, k∈[0,16) ⇒ 256×16 new;
its `cap0 + …` indices were already counted. Skirt loop adds
`SKIRT_GRID_VERTS + k`, k∈[0,256) ⇒ 256 new; its `skirt_cap_index(k)` targets are
cap verts, already counted. The legacy range `[0, 4225)` appears in **no** LOD0
loop.

**Fetch counts ([A]).** Enumerated by walking each VS line by line and following
every callee to its `textureSample*` — `sample_live_card` (`:8449`),
`sample_live_card_gol` (`:8453`), `ug_cell_lift` (`:355`), `sample_pawn_aura`
(`:5681`). Guards recorded at the branch that encloses each.

**Card writer invocations ([F-4]).** `renderer.hpp:544-550`:
`(640/8)² × 8×8 = 409,600` and `(640/16)² × 16×16 = 409,600`.

---

## [R] REPRESENTATION — AMENDMENT A

**Appended, not merged.** This section amends LEDGER_1; it opens no campaign.
Everything above stands as written at `186fd39`. **Line references in THIS
section are read at `cab1a0f`** — the tree has moved 67 commits and +460/−222
lines across `world.wgsl` + `state.hpp` since the body was written, so the
body's references have drifted (see R-7). Where this section and the body cite
the same code, this section's numbers are the live ones.

### R-1 — THE FINDING: LOD0 AND LOD1 SCULPT THE SAME CELL DIFFERENTLY

**Value.** The shape of one lifted GoL cell, as a function of which index buffer
drew its patch.
**Homes.** LOD0 draws per-cell caps (`state.hpp:3274-3287`) plus curtains
(`:3288-3309`) — the cap band decodes at `world.wgsl:320-330`, cell-owned, so a
cell's 25 verts all read one lift and the cell rises as a **slab** with vertical
curtain walls to its neighbour. LOD1 draws the legacy 32×32 grid
(`state.hpp:3368-3377`), decoded through `ug_decode`'s legacy branch
(`world.wgsl:305-310`), where cell ownership is a **clamped division** —
`min(d.vx / UG_QUADS, PATCH_CELL_N - 1u)` — so a vertex on a cell boundary
belongs to one side and the quad spanning the boundary **interpolates between
two lifts**. LOD1 has no curtain band at all: it never had one
(`state.hpp:3363-3396` emits grid + skirt only).
**The shape, exactly.** At LOD1 `step = 2` (`state.hpp:3364`), a 4-quad-wide
cell is drawn as **2 quads**. Of those, the boundary quad straddles two cells,
so a lifted cell renders as **one quad of flat top flanked by ramps** — half the
cell at full height, half spent on the transition. Same apex, half the plateau.
**Evidence (visual, Jean).** A three-frame series on one zone: wholly outside
the LOD0 disc — **uniform taper**; straddling it — a **straight, patch-granular
division** between tapered and slab halves, the division lying on the 50 wu
patch grid; wholly inside — **clean**. The patch-granularity of the division is
the signature: it is the LOD band boundary, not a zone artifact.
**Rate.** Every LOD1 patch overlapping a live zone, every frame, in the eye
pass. And in the **sun** shadow pass, where post-UMBRA both bands draw through
the **LOD1 IB unconditionally** (`render_passes.hpp:393-408`, the density pin
argued at `:372-377`) — so the taper is also what casts. Post-UMBRA
qualification: terrain casts only under the `cast_terrain` arm, true for the sun
pass and **false for every spot atlas tile** (`render_passes.hpp:319`, `:339`,
`:355`). Indoors the taper does not cast because terrain does not cast at all.

### R-2 — RULING (JEAN)

**Cells do not taper. The slab is the truth; the LOD1 ring is what is wrong.**
This closes the question of which representation is canonical. It does not
authorize an edit.

### R-3 — DEAD

| Item | Killed by |
|---|---|
| **The taper/spire branch** — treating the LOD1 taper as an intended soft form to be extended | **Ruling** (R-2). Not a reading result; a decision. |
| **`MAX_GOL_ZONES` truncation** — the suspicion that the draw plan's rect pack silently drops zones | **Census.** `Dim::MAX_GOL_ZONES = 8` (`state.hpp:294`) = the pack's loop bound `n < 8` (`render_passes.hpp:240`) = the WGSL array size `rects: array<vec4<f32>, 8>` (`world.wgsl:9227`). **8 = 8 = 8.** The cap is exact by construction, not a truncation. Struck. |

### R-4 — HELD, DEMOTED: SEGMENT-B RECT CLASSIFICATION HAS NO MARGIN

**Value.** Whether a clean-classified LOD0 patch can own a non-degenerate
curtain and be drawn from the cap-only IB anyway — an open cap.
**Home.** `world.wgsl:9318-9325` — patch rect vs zone rect, raw:
`pi.origin.x + half >= r.x && pi.origin.x - half <= r.x + r.z` (and z). The
frustum AABB one block up takes `CULL_MARGIN_WU = 5.0` (`world.wgsl:9239`, used
at `:9287-9289`); **this test takes none.**
**Why demoted, not dead.** The all-inside frame of the R-1 series shows **no
open caps anywhere**. That is evidence against the mechanism being live, not
proof — the frame does not exercise a zone edge landing inside the 5 wu the
frustum test would have granted. It stays held. **It stops leading.**
**What would settle it.** A zone rect whose edge falls within a few wu of a
patch boundary, inside the LOD0 disc, held across a frame. Not authorized here.

### R-5 — DESIGN, NOT AUTHORIZED: CELL-GRANULAR LOD1

**Shape.** One cap quad per cell; curtains on the **+x and +z planes only**,
each spanning ground to `max(L_here, L_neighbour)`. Every face seals —
including the ones a cell does not own — because the neighbour's curtain takes
the same `max`. The card is world-addressed (`live_card_uv`,
`world.wgsl:8529-8531`, origin from `config.lod_point_*`), so the neighbour
fetch crosses patch boundaries natively; no patch-local fixup.
**Index arithmetic, against the real builders.** Today's LOD1 is
`32×32 = 1024` interior quads (`state.hpp:3368-3377`) + `256/2 = 128` skirt
segments (`:3380-3388`) = `(1024 + 128) × 6` = **6,912 indices / 2,304
triangles** — confirmed. The two candidate bands, arithmetic checked, **not**
read off any builder because none exists:

| Band | Quads | Indices | vs today's LOD1 |
|---|---|---|---|
| today (grid + skirt) | 1024 + 128 | **6,912** | — |
| four-curtain cell band | 256 + 1024 + 64 | **8,064** | **+16.7%** |
| two-curtain cell band (the design) | 256 + 512 + 64 | **4,992** | **−27.8%** |

**Status.** Design only. No builder, no decode branch, no authorization.

### R-6 — OPEN COSTS

**One extra card fetch per curtain vertex.** `max(L_here, L_neighbour)` is two
`sample_live_card_gol` calls where today's `ug_cell_lift`
(`world.wgsl:354-359`) is one. This is a **read-rate increase in the pipeline
this ledger audits** — [A] counts eye at 3 unconditional + 1 guarded fetches per
vertex — so it is a measurement, not an estimate. It must be measured, not
argued.
**New decode branching, against L2.** The design adds branching to a vertex
decode — the exact class L2 constrains. **And the constraint's text cannot
currently be read:** `LAWS.md:35-36` states "the operational home of the
specifics is the world.wgsl FXC banner — the banner owns the constraints"; the
`world.wgsl` header carries only the law index pointing back at L2
(`world.wgsl:12-16`). `world.wgsl:2369` cites "the runtime-indexed const array
**the banner forbids**" — a rule with no home in this tree. **The FXC banner
does not exist.** Costing this design against L2 requires the banner first.
**Behaviour at the card's edge — settled, and it is the benign one.** Read from
the descriptor, not the call site: `nearest_sampler` is
`AddressMode::ClampToEdge` on U and V (`state.hpp:3966-3967`),
`FilterMode::Nearest` (`:3964-3965`). `live_card_uv` applies no clamp of its own
(`world.wgsl:8529-8531`). **A fetch one cell outside the window returns the
clamped edge texel — not wrapped, not zero.** So a rim cell's neighbour fetch
reads the rim's own lift, `max` returns `L_here`, and the curtain collapses to
zero height rather than to a full-height wall against nothing. **Clamp is the
silhouette-safe answer; zero would have reopened what WALL_1 closed.** The
window: `LIVE_CARD_SIZE = 640`, `LIVE_CARD_EXTENT = 1000.0`
(`world.wgsl:223-224`, `state.hpp:91-92`), origin snapped to the 3.125 wu cell
grid — `floor(raw / PATCH_CELL_SIZE) * PATCH_CELL_SIZE` (`world.wgsl:226-231`) —
so texel pitch is 1.5625 wu and a cell is exactly 2 texels.

### R-7 — TWO CORRECTIONS TO THE BODY, FOUND WHILE READING

**F-9 is CLOSED in scope.** The stale 512 label is gone: the descriptor now
reads `"Live Card (RGBA16Float — GROUND_CARD_1)"` (`state.hpp:3850`), and all
four in-scope sites F-9 named (`state.hpp:3732`, `:3728`, `:1754`,
`world.wgsl:8511` in the old numbering) now carry `LIVE_CARD_SIZE` symbolically
or no number at all (`state.hpp:3846`, `:1797`; `world.wgsl:8601`, `:8626`).
The declared dimension is **640**, one home, mirrored. Survivors are archival
only and outside scope: `src/docs/old docs/ground_card_campaign_v2.md`,
`gc_close_census.md`, and past reports.
**The body's line references have drifted, and one cited symbol is gone.** 67
commits since `186fd39`. Spot-checked: F-1's `state.hpp:3192-3210` (CURTAIN
QUADS) now points at draw-plan segment prose — the loop is at `:3288-3309`;
F-1's `world.wgsl:307` (`lift_scale`) now points at a `vx` decode line — the
field is at `:297` and the value is **derived**, not assigned, at `:348`
(WALL_1); and F-1's `world.wgsl:7863 generate_terrain_indices` **no longer
exists** — kernel, IB, pipeline and bindings were retired at `7084f9f`
(CENSUS_2b). The findings themselves are unaffected; their addresses are not.
A full reference re-anchoring was not asked for and is not done here.

### R-8 — ACCEPTANCE TEST, ALREADY IN HAND

**The wholly-outside frame renders like the wholly-inside one.** The R-1 series
is the fixture: same zone, same camera discipline, three positions relative to
the LOD0 disc. The design succeeds when frame 1 stops tapering. The straddling
frame is the sharper instrument — its division must **disappear**, not move.

**Rate context for whatever is measured.** LOD0 full IB **50,688 indices /
16,896 triangles** (cap 24,576 + curtain 24,576 + skirt 1,536); cap-only IB
**26,112 / 8,704**; LOD1 IB **6,912 / 2,304** — derived from the three emission
loops at `state.hpp:3274-3287`, `:3288-3309`, `:3310-3336` and the LOD1 block at
`:3363-3396`. Counts reach the GPU through `reset_frustum_indirect`
(`state.hpp:2630-2641`) as three 5-u32 slots — A full, B cap-only, C LOD1 — and
are drawn at byte offsets **0 / 20 / 40** into the args buffer
(`render_passes.hpp:451-465`), against list windows at byte offsets
**0 / 512 / 1024** (`state.hpp:1418-1423`, TWIN of `world.wgsl:9231-9233`).
