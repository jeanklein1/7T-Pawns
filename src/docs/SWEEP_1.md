# SWEEP_1 — TEN RULINGS, ONE ROUND

Per task: site, anchor bound, computed constants, branch fired, and
landed / held / dead. Commit order is the ORDER the sweep authored, so
`git bisect` walks it as written.

**Census scope.** `src/cartridges/the_board/**` and `src/incubator_dual.cpp`.
JSX designer text is not a live call site and was not counted. Counts below
are boundary-delimited; no claim rests on a bare substring.

**Gates.** `audit/tools/glaw1/run.sh` (the real cartridge TU, standalone,
`-fsyntax-only`) is GREEN at every commit in this sweep.
`audit/tools/glaw2/run.py` is RED at every commit **and was already RED at
`d785fd9`** with the identical three findings (`array`, `atomicSub`, and
`generate_terrain_indices`) — pre-existing, untouched, and verified by
running it on the unmodified tree. No new glaw2 finding was introduced.
The visual gates are Jean's.

---

## SUMMARY

| task | verdict | commit |
|---|---|---|
| T2 — one clock | **LANDED** (override landed too) | `SWEEP_1 T2` |
| T1 — cube faces | **LANDED** | `SWEEP_1 T1` |
| T3 — FPV eye height | **LANDED** | `SWEEP_1 T3` |
| T4 — under-pawn glow | **LANDED — PROMOTE branch** | `SWEEP_1 T4` |
| T5 — terrain immediacy | **LANDED** | `SWEEP_1 T5` |
| T6 — indoor light count | **LANDED** | `SWEEP_1 T6` |
| T7 — sun indoors | **HELD — the named anchor does not exist** | (no commit) |
| T8 — plant clamp | **LANDED** | `SWEEP_1 T8` |
| T9 — no indoor ribbons | **LANDED** | `SWEEP_1 T9` |
| T10 — GoL period skew | **LANDED** | `SWEEP_1 T10` |

Nine landed, one held. The held one is held because its anchor did not
verify, not because it was hard.

---

## T2 — ONE CLOCK · **LANDED**

**Site.** `contracts/spine_state.hpp` (`TimeState`) · `cartridge.hpp`
(`phase_advance_clock`, `phase_fill_signal`, `phase_transition_machine`,
the O-5a witness, two spine face tags).

**RECON — frame-dt production sites.** ONE upstream, three spellings in
scope. `incubator_dual.cpp` takes `console.begin_frame()` and hands it to
`analysis.update(dt)`; canvas_1 keeps it as `AnalysisSignal.dt`. Inside the
census scope that one number was then written three times:

| site | phase | was |
|---|---|---|
| `cartridge.hpp` `gpuSignal.dt = signal.dt` | U1 FillSignal | the GPU's delta |
| `cartridge.hpp` `time_state_.dt = signal.dt` | U2 AdvanceClock | the CPU's delta |
| `cartridge.hpp` `transition_timer += signal.dt` | U7 TransitionMachine | a third read |

**All three merged** to the transport's one derivation.

**RECON — dt_wall's surviving homes.** Exactly two, as ruled. The transport
(to derive) and THE FRAME METER, whose `std::chrono::steady_clock` pair
(`cartridge.hpp`, `FrameMeter`) measures real milliseconds against a real
16.6 ms budget and never touched `signal.dt` at all.

**RECON — existing tempo / beat / transport / MIDI-clock symbols.**
`TimeState{beats, seconds, dt, beat_rate, prev_beats}` on the spine;
`FrameSignal.t_beats` / `.dt_beats` in both GPU rooms; `MidiTransport`
(`src/sources/transport.hpp`, out of scope); and `src/core/clock.hpp`'s
`Clock` class — a complete bpm/beats/seconds transport with **zero
consumers anywhere in the tree**. Reported, not revived: it is out of scope
and the spine already had the struct to extend.

**RECON — the tempo fact the MIDI intake yields.** BOTH, and only one of
them crosses into scope. `MidiTransport` counts raw 24-ppq `0xF8` pulses
(`pulses()`, `beats() = pulses / MIDI_CLOCK_PPQN`) **and** computes an
EMA-smoothed `bpm()` = `60 / (ema · 24)` — the ruling's fallback formula,
already written, with an exponential mean instead of a flat 24-tick one.
That `bpm()` is **never plumbed to the render side**. What reaches the
census scope is the beat COUNT: canvas_1 reads `port_.beats()` into
`AnalysisSignal.t_beats`. So the override binds from a beat count, which is
the "tempo value" case one derivative away — no tick-averaging fallback was
needed.

**BIND.** `TimeState` extended in place (no new struct): `bpm = BPM_REFERENCE`,
`beat_phase`, `bar`, beside the `beats` it already had.

```
dt         = dt_wall × bpm / BPM_REFERENCE      ← the single source
beat_phase += dt × beat_rate                    ≡ dt_wall × bpm / 60
beats       = beat_phase
bar         = beat_phase / BEATS_PER_BAR
seconds    += dt
bpm         = 60 × Δt_beats(DAW) / dt_wall      ← held-last through silence
```

**Computed constants.** `BPM_REFERENCE = 100.0` (dt scale is exactly 1 at
boot — witnessed by a `static_assert` on `TimeState{}.bpm`).
`BEATS_PER_BAR = 4.0`. `beat_rate = BPM_REFERENCE / 60 = 1.666667` beats per
PROGRAM second — constant now, since tempo lives in `dt`.

**The one identity that matters.** `dt × beat_rate ≡ dt_wall × bpm / 60`.
It is why the ribbon's sway (`phase += beat_rate × (60/RIBBON_REFERENCE_BPM)
× dt`) is unchanged at the anchor by construction rather than by luck, and
why nothing needed rescaling.

**O-5a rephrased, not retired.** It pinned `FillSignal < AdvanceClock`
because `dt_beats` straddled the clock's own advance. The transport authors
`dt_beats` from its own delta, so the end that needs pinning is the other:
`AdvanceClock < StageFadeUpload` — the transport must land before the drain.

**Reported, out of scope.** `coupling/visual_canvas.hpp:323` reads
`signal.t_beats` — the DAW's beat count — directly. It is not a dt source,
so `dt_wall`'s two-home law is intact, but it IS a second beat source and it
lives outside the census scope. One line, when the scope next includes it.

**GATE.** No-DAW motion identical to today (bpm rests at 100, scale exactly
1); DAW at 120 runs the world 1.2×.

---

## T1 — CUBE FACES · **LANDED**

**Site.** `world.wgsl` `monolith_vs` (the shader) · `bodies/cube_behaviors.hpp`
`cube_write_gpu` (the host-side base color, verbatim:
`fe.base_color[i] = fe.color[i] = inst.colors[i]`).

**RECON — is a face signal derivable?** YES, with no new vertex data and no
new binding: `in.normal` is the cube mesh's own per-face normal and the
retired code already argmax'd it into six ids.

**RECON — the cube's per-entity seed.** `fe.entity_seed = Dim::CUBE_SLOT_OFFSET
+ inst.slot` — a slot index, decorrelated by the hash.

**RECON — helper census (hash / colour-space), before adding anything.**

| helper | home | verdict |
|---|---|---|
| `hash_property(seed, prop)` | `world.wgsl` §1.5 | REUSED — this is `hash01` |
| `cpu_hash_f` | `primitives/seed_utils.hpp` | its L3 mirror |
| `hue_rotate(c, a)` | `world.wgsl` (Rodrigues, about the RGB gray axis, branchless, identity at 0) | **REUSED — the tree's one standard rotate-about-gray-axis helper. Nothing added.** |
| `rgb2hsv` / `hsv2rgb` | `world.wgsl` | present, not needed |
| `orb_rgb_to_hsv` / `orb_hsv_to_rgb` | `world.wgsl` §12 | a THIRD colour-space pair — reported as residue, untouched |

`hue_rotate` is declared *below* `monolith_vs`. That is legal and already
relied upon: WGSL module scope is order-independent, and `live_card_origin()`
reads the `config` binding thousands of lines before its declaration.

**BIND.** `face_id = 2·argmax_axis(|n|) + (n[axis] < 0)` by `step`/`select`,
no branch (ties fall to the higher axis, which axis-aligned face normals
never reach). `t = hash01(seed, 506)^0.35`.
`turns = (hash01(seed, 500 + face_id)·2 − 1) · 0.22 · t`, applied as
`hue_rotate(fe.color, turns · 2π)`.

**Computed constants.** `FACE_HUE_MAX = 0.22` turns (≈79° each way, so a
fully tempered cube spans ≈158° of the wheel) and `TEMPERAMENT_POW = 0.35`,
both beside `monolith_vs`. The exponent IS the "minority": `t = u^0.35` has
mean ≈0.74 and puts ≈3.4% of cubes below `t = 0.3`.

**What it replaced, and why replaced rather than added.** The old
`face_delta = (hash − 0.5)·2·fe.face_variance` pushed `fe.color` along
`(1.0, 0.7, 0.5)` — a per-face BRIGHTNESS swing, at a spread sampled ≈0.40
for all four cube tiers. Left live it would have made "a minority
near-uniform" unreachable: every cube would still have carried a ±0.4
value spread. The ruling's gate decided this.

**REPORTED — residue this created.** `FloatingEntityState.face_variance`
(offset 76, both rooms) is still WRITTEN every cube spawn from
`CUBE_TIERS` column 9 and is now READ BY NOTHING. The byte stays — the
struct is an L3 byte-for-byte mirror and cutting it is a two-room ABI edit
plus four tier rows. Tagged `STATUS: LATENT[cube:face-variance]` in both
rooms. **Follow-on: cut the field, the `CubeIdx::FACE_VARIANCE` param, and
the four tier columns together.**

**GATE.** Most cubes visibly multi-hued; occasional near-uniform survivors.

---

## T3 — FPV EYE HEIGHT · **LANDED**

**Site.** The hardcode: `world.wgsl`, `const FPV_EYE_HEIGHT: f32 =
PAWN_HEIGHT + 0.2` — **value 1.7 wu** (`PAWN_HEIGHT = 1.5`), one reader:
`camera.pos = pawn_pos + vec3(0.0, FPV_EYE_HEIGHT, 0.0)` in `update_camera`.

**RECON — where per-pawn height lives.** `PAWN_FIGURES[].height`
(`bodies/pawn_figures.hpp`), mirrored to the GPU as
`agent_figure_profiles[].height` (`PawnFigure`, `@group(0) @binding(112)`).

**RECON — archetype census, and the most common pawn.** 14 figures in three
families. `FIGURE_SHARES` gives regular 70% / smooth 15% / heraldic 15%,
equal within a family — so **figure 0 "Pawn" (1.50 wu) alone is 70%**,
against 2.50% each for the six smooth and 2.14% each for the seven
heraldic. Heights run 1.45 (Star) to 3.80 (Spire).

**Computed constant.** `EYE_RATIO = old_eye / conventional_height =
(1.50 + 0.20) / 1.50 = **1.1333333**`. Derived from figure 0's own height
rather than transcribed, and witnessed: an assert pins
`PAWN_FIGURES[0].height × EYE_RATIO` back onto 1.7.

**BIND, and why it is CPU-side.** `agent_figure_profiles` is binding 112,
**render-VS only**. The Compute Entity layout — where `update_camera` lives
— does not carry it and is FULL at 12 entries. This sweep adds no binding,
so the value crosses the way the tree already crosses this exact gap:
`pawn_tilt_tau` (CLOSURE_PAWN [6]) reads `PAWN_FIGURES[skin]` for the
possessed slot each frame and ships a scalar in the config. `pawn_eye_height`
follows it, three lines away.

GROWTH LAW: it took `_pad592_0`, a declared tail pad. Both rooms, same
commit, same position; **sizeof stays 592 and the witness is unmoved**. No
boot pin, following `pawn_tilt_tau`: the per-frame writer is unconditional,
so a zero rest fails loud rather than hiding behind a plausible number.

The skin clamp now mirrors the shader's (`select(skin_id, 0u, skin_id >=
PAWN_FIGURE_COUNT_WGSL)`) in both rooms; for `tilt_tau` that is
byte-identical to the old `0.0f` fallback, since figure 0 rests at
`TILT_LAG_NONE`.

**GATE.** Common-pawn FPV unchanged to the pixel; Colossal (3.50) and Star
(1.45) now proportional.

---

## T4 — UNDER-PAWN GLOW · **LANDED — PROMOTE**

**BRANCH FIRED: PROMOTE.** Promotion was a guard edit, so the glow lives
fully rather than dying whole.

**RECON — the mechanism, verbatim.** `world.wgsl`, `patch_terrain_fs`. The
trigger was a nest of **seven** conditions:

1. `if (tag_alpha > 0.001)` — a nonzero cell behaviour tag
2. `if ((mode & CELL_ANIM_GOL) != 0u)` — the cell is GoL-animated
3. `if (fade > 0.01)`, `fade = 1 − smoothstep(GOL_FADE_NEAR, GOL_FADE_FAR, cam_dist)`
4. `if (zp.transition_fraction <= 0.0) { continue; }` — a live zone
5. the `local_cell` bounds test — this fragment inside that zone's grid
6. `if (color_val > 0.01)` — the cell's spring life
7. `if (pawn_ff > 0.01)`, with `radius = mix(6.0, 2.0, speed_factor)` — the
   **stopped** condition: 6 wu standing still, 2 wu at full walk

**RECON — colour source.** `ZONE_PAWN_TINT = vec3(0.4, 0.2, 0.5)` (purple),
`ZONE_PAWN_TINT_STRENGTH = 0.6`, weight also multiplied by `color_val`.

**RECON — spatial extent, GoL cells vs. pawn range.** GoL zones exist only
where a discrete-mode lattice node rolls one (`GOL_ZONE_SPAWN_CHANCE = 0.60`
of discrete zones) and only in moods with `allow_gol_zones`. The pawn walks
the whole world. So the glow was **mostly absent**, and where it existed it
flickered at every zone edge and every stop.

**RECON — every reader and writer.** ONE reader: the expression above.
**ZERO writers, zero dedicated state, zero buffers** — it was a pure shader
expression the entire time, derived from `render_pawn_pos()`,
`render_pawn_vel_xz()`, `zone_params` and `zone_life_read`. That is why
promotion is a guard edit at all.

**The three criteria, checked.**

- *Colour well-defined wherever pawns stand* — a const tint over the
  terrain's own `base_color`. YES.
- *Zero new bindings, buffers, branches* — `render_pawn_pos()` was ALREADY
  read unconditionally in this same shader, twelve lines below, by the aura.
  YES.
- *No new branch in an FXC-named chain* — this is the terrain FRAGMENT
  stage, not the collision/ground chain, and the edit removes branches
  rather than adding any. YES.

**BIND.** Hoisted out of the whole nest to an unconditional site. The mix
weight lost `color_val` with the nest — the GoL cell's spring life is
undefined outside a zone and was the only zone-dependent term. Branchless:
at range the smoothstep is 1, the weight is 0, and `mix` returns
`base_color` exactly, so the `> 0.01` guard is not needed and is not
replaced.

**REMOVAL LEDGER.** `PAWN_FORCEFIELD_RADIUS_MOVING` (2.0) and
`PAWN_FORCEFIELD_SPEED_SCALE` (1.0) — gone with the speed lerp.
`_STATIONARY` became the one `PAWN_FORCEFIELD_RADIUS` (6.0 wu).
`render_pawn_vel_xz()` — **cut**; the speed lerp was its only reader.
`zone_pawn_ff` → `pawn_forcefield_at`: it stopped being a zone function
when it stopped living in the zone nest. The SPHERE tint is untouched and
still zone-scoped — only the pawn pair left the family.
`PAWN_FORCEFIELD_ENABLED` (compile-time, `true`) is left alone: it is a real
kill switch and it is not "flag off".

**GATE.** Glow always under the pawn; no corner-case flicker.

---

## T5 — TERRAIN IMMEDIACY · **LANDED**

**Banner re-read before the edit** (`world.wgsl` head + LAWS.md L2). It
forbids new runtime BRANCHING in the collision/ground chain, texture-array
stamps near it, and per-stage binding growth. It does **not** forbid
arithmetic growth. **No clause trips.**

**RECON — the octave table.** `TERRAIN_BANDS`, six bands. `freq_mean` is
cycles per world unit, so wavelength = 1/`freq_mean`:

| band | name | lattice spacing | freq_μ | **wavelength** | amp_μ | amp_σ |
|---|---|---|---|---|---|---|
| 5 | tectonic | 500 | 0.012 | **83.33 wu** | 15.0 | 6.0 |
| 0 | continental | 200 | 0.030 | **33.33 wu** | 8.0 | 3.0 |
| 1 | regional | 80 | 0.080 | **12.50 wu** | 3.0 | 1.5 |
| 2 | local | 30 | 0.200 | **5.00 wu** | 1.2 | 0.5 |
| 3 | detail | 12 | 0.500 | **2.00 wu** | 0.4 | 0.2 |
| 4 | fine | 5 | 1.200 | **0.833 wu** | 0.12 | 0.05 |

**RECON — patch extent.** `PATCH_EXTENT = 50.0` wu ⇒ patch/2 = **25.0**,
patch/16 = **3.125**.

**RECON — card texel density.** `LIVE_CARD_EXTENT / LIVE_CARD_SIZE =
1000.0 / 640 = **1.5625 wu per texel**`; 4 texels = **6.25 wu**.

**THE CLASSIFICATION, reported before binding.**

| class | bands | gain |
|---|---|---|
| λ ≥ patch/2 (25) | 5 tectonic (83.33), 0 continental (33.33) | ×1.0 |
| λ ∈ [patch/16, patch/2) | **1 regional (12.50), 2 local (5.00)** | **×1.7** |
| λ < patch/16 (3.125) | 3 detail (2.00), 4 fine (0.833) | ×1.0 |

Two bands are the hill band. μ AND σ take the gain, in the table, as
const-expressions — the whole per-node distribution moves rather than
gaining a raised floor — and the authored pre-scale numbers stay written
beside them. Patch-scale octaves are untouched, so nothing lifts a patch
whole.

**Computed constants — the ridged term.**

- `f_r` = 2 × the highest amplified octave's frequency = 2 × 0.200 (band 2,
  local) = **0.400**, wavelength 2.50 wu. Then clamped to λ ≥ 4 card texels
  = 6.25 wu ⇒ `f_r` = 1/6.25 = **0.16 cycles/wu**. **THE CLAMP BINDS** — at
  0.400 the riser would alias on the card and the floor would stop agreeing
  with the ground under the walker.
- `amp_r` = 0.35 × that octave's **PRE-SCALE** `amp_mean` (1.2) = **0.42 wu**.

**The field is two summed unit-axis sines, and that is the constraints
speaking.** `terrain_height_and_complexity` inlines ~8× into the ground
chain through `pawn_ground_resolve`, so the term is arithmetic only: a
hashed value noise would put four hash calls behind each of those eight,
and a branch would break L2 outright. Two SUMMED sines carry exactly `f_r`
and nothing above it, so the 4-texel clamp is exact — a sine PRODUCT would
have smuggled in a 1.78 × `f_r` sum term and broken its own clamp. The
second axis sits 54° off +x so the creases cross obliquely rather than
lying on the world grid. Per-world phase from `master_seed`, arithmetic.

**Floor and card move together, structurally.** The riser carries no
`t_beats`, and the card carries only (moving − frozen) per band
(`true_band_delta_contribution`), so a time-invariant term adds the same
constant to both halves: the delta is untouched and the baked base gains
the relief. **The call graph is unchanged** — no new callee — so LEDGER_1's
findings stand.

**One decision the ruling did not pre-make, taken and stated.** The indoor
amp ceiling (`config.terrain_amp_ceiling`) clamps every other wave in this
evaluator per node. The riser answers to it too, by `select` rather than by
branch — a room does not get 0.42 wu of relief the bands around it were
capped out of.

**GATE.** Fewer dead-flat stretches; sharp risers appear; walk one —
collision follows, because it is the same baked evaluator.

---

## T6 — INDOOR LIGHT COUNT · **LANDED**

**Site.** `direction/mood.hpp`, `derive_indoor_lights` — where the count has
never been drawn directly. It arrives as `LIGHT_SCHEMES[scheme].slot_count`.

**RECON — the RNG in reach.** `select_tier(seed, prop, weights, count)` →
`select_weighted(cpu_hash_f(seed, prop), …)` — already a **cumulative**
draw, seeded per destination. Nothing added.

**The bijection.** The scheme table is one-to-one onto the counts:
Cathedral 3, Quartet 4, Gallery 2, Sanctum 1. So the ruled COUNT
distribution IS the scheme weight row, re-ordered onto the schemes that
carry those counts.

| count | scheme | was | **now** |
|---|---|---|---|
| 4 | Quartet | 35% | **70%** |
| 3 | Cathedral | 35% | **20%** |
| 2 | Gallery | 15% | **7%** |
| 1 | Sanctum | 15% | **3%** |

Weights live beside the table they select from. Three witnesses land with
them, because the bijection is load-bearing and invisible: re-slot a scheme
or add a fifth and the count distribution silently stops being the ruled
one, with nothing else to catch it.

**NOTE, eyes open, as ruled.** LEDGER_1 F-3: the indoor shadow list is drawn
once per spot light with terrain included. This makes the 4-light maximum
the common case rather than a third of rooms. Layer E owns the multiplier;
this sweep does not touch it.

**GATE.** Counts over many rooms track the weights.

---

## T7 — SUN INDOORS · **HELD**

**The named anchor does not exist. Nothing landed, and nothing was
improvised in its place.**

**RECON — the masking site, verbatim.** There is **no indoor mask on the
directional term.** `calc_directional_light` is four lines of arithmetic and
one conditional, and that conditional is on the SHADOW:

```wgsl
fn calc_directional_light(world_pos: vec3<f32>, normal: vec3<f32>, geo_normal: vec3<f32>) -> vec3<f32> {
    let light_dir = -render_light.direction;  // toward light
    let ndotl = max(dot(normal, light_dir), 0.0);

    // Shadow: skip when spot lights are active — light_vp is being used
    // for spot atlas tiles, so the directional PCF would sample wrong.
    var shadow = 1.0;
    if (render_spot_lights.count == 0u) {
        shadow = sample_shadow_pcf(world_pos, geo_normal);
    }

    return render_light.color * render_light.intensity * ndotl * shadow;
}
```

`render_light` has exactly **four** readers in the whole shader — the
normal-offset direction, this function's `light_dir`, this return, and
`shade_lit`'s ambient. `shade_lit` adds `sun` unconditionally. Nothing
attenuates the diffuse term indoors.

**So the sun already shines indoors.** `upload_lights` ships
`MOOD_TABLE[mood].sun_intensity`, and the indoor rows carry **0.35** (not 0)
with direction (0.20, −0.90, 0.00), near-vertical. Indoors `shadow` is
pinned to 1.0 by the skip above, so an indoor floor (normal 0,1,0) receives
`0.35 × 0.977` of unshadowed sun plus 0.35 ambient. The ruling's premise is
already true in the tree.

**What IS indoor-conditional and touches the sun** — reported so Jean can
rule, not acted on:

1. `mood.hpp` `apply_mood_spot_lights`: `if (m.indoor) { set_mute_coupling(
   Coupling::PAWN_TO_SUN_VP, true); … }`, gating `world.wgsl`'s
   `if (coupling_active(COUPLING_PAWN_TO_SUN_VP))` around the sun VP build.
   Deleting this mute would compute a sun VP that the spot pass overwrites
   per tile. It changes nothing visible and makes the sun no brighter.
2. The shadow skip above. It cannot be deleted under "no new shadow
   machinery": indoors the sun map **IS** the spot atlas (lights 0–1) and
   `light_vp` is overwritten per spot tile, so `sample_shadow_pcf` would
   sample a spot tile with a spot's matrix. That is not a mask you delete —
   it is a resource the sun no longer owns.
3. `MOOD_TABLE`'s indoor rows (0.35 / 0.35 vs. outdoor 0.90 / 0.20). That is
   mood tuning, and "delete, not flag off" has nothing to delete in a table
   value.

**RECON — is interior geometry drawn into the sun cascade?** **YES.**
`drawable_table.hpp`: the `shell` row carries `DRAW_SHADOW | DRAW_MAIN |
DRAW_SNAPSHOT`, so ceiling and walls cast into whatever shadow pass runs.
**This decides WINDOW-LIGHT, not wash** — if the sun cascade ever ran
indoors, the shell would occlude it and light would fall through openings.
Reported only; unchanged, as instructed.

**What a follow-on needs.** For the sun to cast indoors, the cascade needs
its own depth target: today it lends `shadow_map` to spot lights 0–1
(`render_passes.hpp`, `use_sun_map = (li < 2)`). Freeing it means a third
shadow texture or a re-tiled atlas — new shadow machinery, which T7
forbids. So this is a ruling to re-cut, not a mask to delete.

---

## T8 — PLANT CLAMP · **LANDED**

**Site.** `contracts/indoor_module.hpp` (the treatment table) ·
`bodies/grounded.hpp` (`cactus_apply_indoor_rescale`,
`blade_apply_indoor_rescale`, both adapters).

**RECON — the plant spawn site.** The generic pipeline's single indoor
sizing hook: `machine/entity_pipeline.hpp`, `if (MOOD_TABLE[active].indoor
&& INDOOR_TREATMENT[family].size != NATURAL && adapter.apply_indoor_rescale)`.
Palm was already CAP. **Cactus and blade were NATURAL — no clamp at all.**

**RECON — is growth dynamic?** **NO.** Flora params are drawn once at
select, baked into the GPU mesh params at commit, and the mesh generators
are one-shot behind their pending flags. Nothing mutates a plant's height
per frame, so spawn is the only site and there is no growth tick to also
clamp.

**RECON — the ceiling-height fact's one home.** `MOOD_TABLE[mood]
.ceiling_height` (`contracts/spine_state.hpp`), read at the one hook above.
FLAT 20.0, VAULT 25.0. Single home confirmed — no STOP.

**The law was already in the tree and is reused, not re-spelled.**
`cap_to_ceiling` with `INDOOR_HEIGHT_CAP_FRACTION = 0.75f // Jean's law` is
exactly `s = min(1, 0.75 · ceiling / natural_height)`, uniform across every
listed param, no-op when it already fits.

**Why cactus is the reason the ruling exists.**

| tier | authored height | ¾ × 20 (FLAT) | verdict |
|---|---|---|---|
| CANDELABRA | 20.0 ± 4.0 wu | 15.0 | pierced routinely |
| SAGUARO | 13.0 ± 2.5 wu | 15.0 | pierced at the top of its spread |
| FINGER | 9.0 ± 1.0 wu | 15.0 | fit |

Blade never reaches the cap at its authored tiers (THICKET 5.20 ± 1.20, and
3σ × 1.45 = 12.8 against 15) and takes the clamp anyway — the law is about
plants, not about tall plants, and the cap only ever scales down.

**Natural height, per family.** Cactus: `max(trunk, trunk·ARM_HEIGHT +
ARM_LENGTH)` — arms fork partway up and curve toward vertical, so that bound
is conservative (`arm_curve < 1` means they never rise the full length),
which is the safe direction for a cap. Blade: `BLADE_H · (1 + BLADE_H_VAR)`,
the tallest blade the cluster can grow, not the mean. In both, the unscaled
term is a ratio, so the capped extent scales by exactly the same factor and
the cap is exact.

**Only LENGTHS scale.** `RIB_DEPTH` is excluded despite sitting in the
table's length row — the kernel reads it as `rib_mod = 1 + cos(..)·rib_depth`,
a ratio on the radius. `SPLAY` likewise: the blade kernel reads it through
`cos`/`sin`. `ARM_HEIGHT` is a fraction of the trunk (`fork_frac`).

**Stale comment corrected in passing.** The table's "Jean: keeps size" on
both rows is superseded and says so.

**GATE.** No plant pierces or crowds a ceiling.

---

## T9 — NO INDOOR RIBBONS · **LANDED**

**Site.** `bodies/ribbon.hpp` `select_ribbon_for_patch` (spawn) ·
`direction/input.hpp` F8 (the transfer).

**RECON — the ribbon spawn decision site.** `select_ribbon_for_patch` is the
family's ONE birth path: `dispatch_select_ribbon` is its only caller, and
unlike the arch there is **no force-spawn door**. So the exclusion is
complete, not partial.

**RECON — the key-transfer candidate enumeration, and its scope.** **There
was none.** F8 called `possess(host == RIBBON ? PAWN : RIBBON)` behind
nothing but a compile-time `if constexpr (ROSTER.ribbon)`. The thing being
mounted is `RibbonState.rendered_slot` (RESIDUE_3) — elected each tick in
`ribbon_frame_tick` as the **nearest ACTIVE ribbon to the point**, and
parked at `UINT32_MAX` when the world holds none, at which point an empty
`GPURibbonState` is uploaded. So the candidate set is
`{i : rs.active[i].active}` over `MAX_RIBBON_INSTANCES`, its scope is the
ribbons resident in the live world, and `rendered_slot` is its elected
member. Mounting an empty set handed the pawn kernel's mount gate a zeroed
sky head — the exact failure the ROSTER gate's own comment describes for a
ribbon-less demo. Indoors, after this commit, that stops being a
configuration and becomes the normal case, which is why the fallback had to
land in the same round as the exclusion.

**BIND.** Indoor test as the first line of `select_ribbon_for_patch`, before
the seed is drawn. F8 reads the set it targets; an empty one falls back to
the pawn. **Both arms go through `possess()`** — the one transaction, no
side channel — and `possess()` returns on `cur == next`, so the fallback is
a silent no-op when the pawn already hosts and a real release when the
camera does.

**Dead code removed with its condition.** The INDOOR MINIATURE
(`RIBBON_INDOOR_SCALE` pre-scale + the ribbon-shaped cap) is unreachable by
construction now and is gone rather than left under a condition that can
never be true.

**REPORTED — residue this created.** `RIBBON_INDOOR_SCALE = 0.15f` and
`INDOOR_TREATMENT`'s ribbon row are both reader-free and say so in place,
tagged `STATUS: LATENT[ribbon:indoor-miniature]`. The row stays because the
table's axis is `PopFamily` and F-1 pins it dense. **Follow-on: retire the
dial.**

**GATE.** Indoors, mood keys land on the pawn; outdoors unchanged.

---

## T10 — GoL PERIOD SKEW · **LANDED** (after T2)

**Site.** `bodies/gol_zones.hpp` (the CPU arm) · `world.wgsl`
`zone_derive_params` (the GPU arm). **Both rooms, same commit.**

**RECON — both drivers of the GoL grid.** CONWAY (the neighbour rule) and
PULSE (periodic breathing of cell colour/height, no neighbour rules), split
at spawn by `GOL_PULSE_ALGORITHM_CHANCE = 0.35`.

**RECON — the current period source and its unit.** Both drew `tick_period`
from their tier's Gaussian (μ, σ), and the unit is **BEATS** — `world.wgsl`
says so at the field (`tick_period_mean: f32, // beats between generations`)
and the CPU tick gate is `floor(time_state_.beats / effective_period)`.

| CONWAY tier | tick_μ (beats) | | PULSE tier | tick_μ (beats) |
|---|---|---|---|---|
| Pillars | 8.0 ± 2.0 | | Breathe | 2.0 ± 0.5 |
| Sparse | 2.0 ± 0.5 | | Sparkle | 0.5 ± 0.15 |
| Moderate | 1.0 ± 0.3 | | Drift | 4.0 ± 1.0 |
| Dense | 0.5 ± 0.15 | | | |
| Flash | 0.25 ± 0.05 | | | |
| Monolith | 12.0 ± 3.0 | | | |
| Glacier | 4.0 ± 1.0 | | | |

**The PULSE table is reported for this ledger and NOT edited**, as ruled.

**BIND.** The rule path draws from `{¼, ½, 1, 2, 4}` beats at
`{5, 10, 20, 30, 35}%`. **Four beats is the floor of speed**, with a
`static_assert` to say so: Pillars (8) and Monolith (12) went with the
Gaussian and nothing slower can be drawn. 65% of grids land at 2 beats or
slower; 5% at a quarter.

**Why both rooms, and why bit-identical.** This period is drawn TWICE — the
CPU computes the per-frame tick MASK from it, and `zone_derive_params`
derives the zone's spring transition from its own copy. A divergence would
run the rule and its visual on different clocks. Same salt (**931** in both:
`GoLZoneProp::TICK_PERIOD` / `ZONE_PROP_TICK_PERIOD`), same cumulative walk,
same table order; `hash_property` ≡ `cpu_hash_f` is already pinned by L3
§1.5. The Gaussian's `max(0.1, …)` floor is dropped in both — the table's
own minimum is 0.25.

The WGSL walk is the verbatim shape of the Conway TIER selection fifteen
lines above it in the same kernel, runtime-indexed const array included:
`zone_derive_params` is a compute kernel, not the collision/ground chain,
and that pattern is already what lives there.

**Periods read the transport grid, with no further wiring.** After T2 both
`time_state_.beats` and `signal.t_beats` ARE the program's own clock, so a
grid keeps musical time with no DAW listening and follows the tempo when one
is. That is the whole of the T2 dependency, and it is why T10 came after.

**REPORTED.** `GOL_TIERS`' `tick_μ/σ` columns are reader-free on the Conway
path and say so in both rooms; they stay because the row shape is the L3
mirror.

**GATE.** GoL mostly slow, occasionally quick; slowest visibly 4 beats; the
other driver unchanged.

---

## RESIDUE REGISTER (created by this sweep, reported not chased)

| item | home | why it survives |
|---|---|---|
| `face_variance` (field + `CubeIdx` param + 4 tier columns) | `state.hpp` / `world.wgsl` / `cube_behaviors.hpp` | L3 byte-for-byte mirror; a two-room ABI cut |
| `RIBBON_INDOOR_SCALE` | `contracts/indoor_module.hpp` | retiring a named dial is its own pass |
| `INDOOR_TREATMENT`'s ribbon row | `contracts/indoor_module.hpp` | the table's axis is F-1-pinned dense |
| `GOL_TIERS.tick_period_mean/sigma` | both rooms | the row shape is the mirror |

## RESIDUE FOUND (pre-existing, reported not chased)

| item | home | note |
|---|---|---|
| `Clock` — a complete bpm/beats/seconds transport | `src/core/clock.hpp` | **zero consumers anywhere.** Out of scope; T2 extended the spine's `TimeState` instead |
| `orb_rgb_to_hsv` / `orb_hsv_to_rgb` | `world.wgsl` §12 | a THIRD colour-space pair beside `rgb2hsv`/`hsv2rgb` and `hue_rotate` |
| `signal.t_beats` read direct | `coupling/visual_canvas.hpp:323` | a second beat source, outside the census scope |
