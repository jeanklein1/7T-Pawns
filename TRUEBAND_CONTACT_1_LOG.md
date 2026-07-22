# TRUEBAND_CONTACT_1 — CAMPAIGN LOG

Campaign: TRUEBAND_CONTACT_1 (campaign v2 Stages 6 + 7, designed on
AUDIT-3; handoffs src/docs/HANDOFFS/T 0-3/). Branch: `TRUEBAND_CONTACT_1`.

---

## T0 — INDEX + PREFLIGHT

### Base

Cut from `732b1a90a051ba1bbfcdd0b1bb3a191dbcc232c1` — the
UNIFIED_GROUND_1_AUDIT3 tip (Jean's designation by push placement: the
T handoffs landed there; src/cartridges identical to the U6 closeout).
Anchor source AUDIT3_REPORT.md §A3-3/§A3-4 in-tree.

### Anchor table

| # | Anchor | Expect | Found | Verdict |
|---|--------|--------|-------|---------|
| a | `fn write_live_card` | 1 | 1 | PASS |
| b | `fn terrain_band_contribution` | 1 def | 1 | PASS |
| c | `fn evaluate_lattice_wave` | 1 def | 1 | PASS |
| d | `PAWN_GOL_GROUND_ENABLED` (the fossil) | count ALL | **exactly 1** — world.wgsl:2086 `const PAWN_GOL_GROUND_ENABLED: bool = false;    // Pawn walks on GoL extrusions` — REFERENCE-FREE ⇒ the T2a tombstone path | PASS |
| e | `const OVERLAY_WAVES` | 1 | 1 | PASS |
| f | `fn get_band_blend` | 1 def | 1 | PASS |
| g | `fn behavior_flock2d` | 1 | 1 | PASS |
| h | agents.hpp `AGENT_TIER_GAINS[AGENT_TIER_COUNT]` | 1 | 1 | PASS |
| i | state.hpp `float _pad[2];         // 24-31` | 1 | 1 | PASS |
| j | `fn update_sphere` / `fn update_cube` | 1 each | 1 / 1 | PASS |

All PASS → T1. Language law noted: `active` reserved (A3-5); new
fields follow the house `is_active` shape.

### Baseline gate

glaw1 at base 732b1a9, before any edit: `G-LAW 1: GREEN`.

---

## T1 — STAGE 6: THE TRUE-BAND SWAP

### [T1a] — 1ffbe62 (the pair refactor)

- CLASS B on the unique `fn evaluate_lattice_wave` def (anchor c):
  the whole body moved VERBATIM into `evaluate_lattice_wave_pair`
  (same params, `-> vec2<f32>`). Both branches (radial / directional)
  return `vec2(val_frozen, val_moving)`; the activation gate returns
  `vec2(0.0, 0.0)` (the original gate returned scalar 0.0 — same
  content, widened). Every expression kept ITS EXISTING FLOAT ORDER —
  the U5a discipline: the refactor moves lines, it does not
  re-associate them. The original became
  `let pair = evaluate_lattice_wave_pair(...); return mix(pair.x,
  pair.y, band_act);` — the same two floats through the same mix the
  body already ended in, so the bake path is bit-exact BY CONSTRUCTION.
- `true_band_delta_contribution(world_xz, seed, t_eff_beats, band_idx,
  raw_activity, beat_freq) -> f32` cloned from
  `terrain_band_contribution`'s pasted node walk (anchor b): same 2×2
  lattice walk, same node seeds, same Hermite weights; per node it sums
  `band_act * (pair.y - pair.x)`; NO complexity accumulation.
  `band_act` via `band_activity_level` — the seeded pools shape where a
  woken band breathes (campaign v2 §6).
- Gates: glaw1 GREEN; Dawn witness ALL FAMILIES GREEN (bake pipelines
  revalidate over the refactor).

### [T1b] — ae6ed29 (the two-pass writer)

- CLASS A registry: `live_card_scratch = 32` after `live_card_write =
  31` (A3-3e certified 32/33 free).
- state.hpp: `liveCardScratchBuffer_` member + creation cloned from the
  patch-scratch pattern, 512·512·2·4 B, Storage; null-check chain
  extended; Live Card Writer layout+group 5→6 entries in the SAME
  commit (the paired-edit law); scratch entry Storage/COMPUTE.
- world.wgsl `@group(0) @binding(32) var<storage, read_write>
  live_card_scratch: array<f32>;` — then `write_live_card` (the
  H3-authored body, anchor a) SPLIT:
  - `write_live_card_heights`: same origin/texel math; band loop
    `b < TERRAIN_BAND_COUNT` skipping `b == 4u` (the fine ripple stays
    bake-only — the Nyquist ruling, campaign v2 §6); `blend <= 0.0`
    skip (the −1 rest sentinel + 0); `t_eff = config.terrain_time -
    get_band_phase_origin(b)`; `dh += clamp(blend,0,1) *
    true_band_delta_contribution(p, config.world_seed, t_eff, b, af.x,
    af.y)`; `dh += contrib_radial_pulses_at(p, signal.t_seconds)` (the
    live signature — H3 deviation-6 precedent); stride-2 scratch write,
    `contrib_gol_zones_at(p)` in channel 1.
  - `write_live_card_resolve`: the bake's pass-2 clone at res 512 —
    16×16 workgroups, 20×20 cooperative `sh_card_h` tile from scratch
    channel 0, the SAME central/one-sided stencils, `eps =
    LIVE_CARD_EXTENT / f32(LIVE_CARD_SIZE)` with the texel-center vs
    bake `(res−1)` mapping difference noted in-kernel;
    `textureStore(live_card_write, texel, vec4(h, gx, gz,
    live_card_scratch[base + 1u]))`.
  - Waking anti-teleport inherited: t_eff = 0 at the origin ⇒ moving ≡
    frozen ⇒ a woken band grows out of the frozen shape.
- renderer.hpp: entry consts `WRITE_LIVE_CARD_HEIGHTS/RESOLVE`;
  pipeline pair; `dispatch_live_card_write` dispatches BOTH sequentially
  in ONE pass (heights /8u, resolve /16u) — the U5a same-pass
  visibility law, cited in-comment.
- Gates: cc6 flags EMPTY (writer 6 entries: 3 storage / 2 uniform /
  1 storage-texture); cc4 closures heights `{0,1,32,160,161}`, resolve
  `{31,32}` — both ⊆ layout; glaw1 GREEN; witness (split-writer
  family) ALL FAMILIES GREEN.

### [T1c] — d2432aa (the overlay retirement + comment truths)

- Retired whole (A3-3d: write_live_card was the sole caller, gone at
  T1b): `terrain_wave_overlay_with_gradient`, `overlay_band_params` +
  `OverlayBandParams`, `OverlayWave` + `OVERLAY_WAVES` +
  `OVERLAY_WAVE_COUNT`, `OVERLAY_PROP_*` consts, the §2.2 ROW 7 overlay
  matrix paragraph. `WAVE_THRESHOLD` + softness STAY — they are the
  true-band pool thresholds. ROW 7's movement third now names the TRUE
  bands; terrain_looks.hpp ROW 2/ROW 7 pointer comments folded in
  lockstep (the C++ room law).
- `get_band_blend` banner: DRIVERLESS → THE TRUE-BAND GATES text
  verbatim per the handoff.
- Comment-stripped closure check: zero overlay references remain.
- Gates: glaw1 GREEN; witness ALL FAMILIES GREEN.

Mid-batch state note (T0 declared): between [T1b] and [T1c] the overlay
tables existed unused — accepted, resolved at [T1c].

---

## T2 — STAGE 7: CONTACT — 4c6e4ae

### DEVIATION (bisection)

The handoff specified three commits [T2a]/[T2b]/[T2c]; the edits were
applied in ONE verified script run over overlapping files and landed as
the single commit `4c6e4ae` carrying all three units. The clean
bisection line the handoff wanted is coarsened by one step here; every
unit's content is delineated below and in the commit message.

### [T2a] — the profile columns + the fossil

- agents.hpp `AgentTierDef` + `AGENT_TIER_GAINS` rows (anchor h) gained
  `contact_radius`/`contact_mass`: worker 1.6/1.0, scout 1.4/0.8,
  sentinel 2.0/1.5, leader 1.8/1.2 — authored defaults, Jean-tunable,
  said in-row.
- state.hpp `GPUAgentTierDef` (anchor i): `float _pad[2]; // 24-31` →
  `contact_radius; // 24` + `contact_mass; // 28`; size stays 32, the
  sync asserts untouched. `upload_agent_registries_to_gpu` fills both
  (was zeroing the pads).
- world.wgsl `AgentTierParams`: `_pad0/_pad1` → the same two names at
  the same offsets.
- Contact consts beside the pawn-forcefield cluster (A3-4e) with the
  population-panel pointer comment: `CONTACT_SPRING 40.0`,
  `CONTACT_IMPULSE_CAP 6.0`, `CONTACT_SPHERE_RADIUS 12.0` (verified =
  `Idle::SPHERE_INFLUENCE_RADIUS` 12.0f — the C++ twin, noted),
  `CONTACT_CUBE_RADIUS 3.0`, `PAWN_CONTACT_MASS_MULT 4.0`.
- THE FOSSIL: `PAWN_GOL_GROUND_ENABLED` was reference-free at T0 (sole
  site = its own declaration, world.wgsl:2086) → tombstoned with the
  epitaph "compile-time gate for the pre-card analytic chain —
  GROUND_CARD_1 retired the chain". No behavior change.

### [T2b] — the walker gathers

- `update_other_agents`, at the A3-4c post-step seam (before the
  point-centered eviction block): the bounded 32-slot pair loop
  (flock2d FXC-shape precedent; A3-5(iii) probe compiled the 33-slot
  form clean on Tint) — tier lookup `min(tier_idx, 3u)`, pawn mass
  mult on `k == config.possessed_slot`, `push = min((r−d) ·
  CONTACT_SPRING · signal.dt, CONTACT_IMPULSE_CAP) ·
  m_other/(m_self+m_other)`; then the sphere loop over
  `SPHERE_SLOT_COUNT` / `floating_entities.entities[sph]`
  (celestial-massive: only self yields).
- `update_player_agent`, same seam shape (before the never-evicted
  write-back): the same pair — with `m_self = g_self.contact_mass ·
  PAWN_CONTACT_MASS_MULT`; the pair weight stays small — a nudge,
  never a shove.
- Impulses land on VELOCITY only (position already ground-resolved this
  frame; the springs realize next frame — soft by construction, said
  in-comment). The racy-neighbor read + the coded dispatch order
  (ribbon → player → others → camera → sphere → cube → vp) are the
  disclosed softness, cited in-comment.

### [T2c] — cube-vs-pawn

- `update_cube` (anchor j), inside the drift-force region: pawn
  repulsion via `agent_state[config.possessed_slot]` +
  `agent_tier_gains` radius, `cr = CONTACT_CUBE_RADIUS +
  pg.contact_radius`, spring-form force (uncapped — a force into the
  damped drift integrator, not a Δv), joins `spring_a + behavior_force`
  in the existing integration line. Zero new state; the drift's own
  spring-to-zero returns the cube. cube-vs-cube stays deferred
  (campaign v2 ruling, said in-comment).

### T2 gates

- glaw1 GREEN.
- Dawn witness (T roster) ALL PIPELINE FAMILIES GREEN.
- cc4 NUANCE (the handoff expected "unchanged binding sets"): the
  agent kernels' static closures GREW, by machine diff (cc4 on the
  pre-T2 vs post-T2 tree, group 0):
  - `update_player_agent` +100 (floating_entities — the new sphere
    loop) and +111 (agent_tier_gains — the player kernel never read
    the tier table before; the gather does)
  - `update_other_agents` +100 only (111 was already in its closure
    via flock2d)
  - `update_cube` +111 only (100 already there — it reads its own
    entity slab)
  - `update_sphere` unchanged.
  Both new numbers were ALREADY BOUND in the CE layout, so the
  handoff's intent — "the gather reads only what's bound" — HOLDS;
  validation proven by the witness. Post-T2 closures (g0; every kernel
  also carries g1 [22,23,33,34], unchanged):
  - `update_player_agent` [0,1,60,62,80,100,111,145,146,152]
  - `update_other_agents` [0,1,60,80,100,110,111,145,146,152]
  - `update_cube`         [0,1,60,80,100,111,145,146,152]
  - `update_sphere`       [0,1,60,80,100,145,146,152]
