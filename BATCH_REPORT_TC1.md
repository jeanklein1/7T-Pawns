# TRUEBAND_CONTACT_1 — BATCH REPORT

Campaign: TRUEBAND_CONTACT_1 (campaign v2 Stages 6 + 7, designed on
AUDIT-3; handoffs `src/docs/HANDOFFS/T 0-3/`). This is the batch's
witness stand (T3).

## Base + final

- Base: `732b1a90a051ba1bbfcdd0b1bb3a191dbcc232c1` (UNIFIED_GROUND_1_AUDIT3
  tip — Jean's designation by push placement: the T handoffs landed there;
  `src/cartridges` identical to the U6 closeout).
- Commit list:

| Commit | Hash | Intent |
|--------|------|--------|
| [T0]   | 2d58ce0 | Preflight: anchors a–j all PASS; the fossil is reference-free; baseline glaw1 GREEN |
| [T1a]  | 1ffbe62 | The pair refactor: `evaluate_lattice_wave_pair` + `true_band_delta_contribution` |
| [T1b]  | ae6ed29 | The two-pass writer: true-band deltas → scratch (g0:32) → resolve |
| [T1c]  | d2432aa | The overlay retirement + the gate banner truth |
| [T2]   | 4c6e4ae | Contact: profile columns + walker gathers + cube-vs-pawn ([T2a]+[T2b]+[T2c] — see Deviations) |
| [T-log]| 7d83e8a | Campaign log: T1 + T2 sections |
| [T3a]  | da7bfd5 | Closeout comment truths: six stale overlay references folded |
| [T3b]  | 2b10505 | Instruments → `_post_tc1` + the split-writer Dawn witness |

## Recount vs expectations (cc6/cc7/cc4 — `_post_tc1` outputs)

| Check | Expectation (T3) | Result | Verdict |
|---|---|---|---|
| cc6 flags | EMPTY | EMPTY | **MATCH** |
| cc6 layouts | unchanged everywhere except the Writer | machine-diffed vs `_post_ug1` with `state_hpp_line` masked: **Live Card Writer Layout is the only substantive diff** | **MATCH** |
| Live Card Writer layout | +scratch storage | 5 → 6 entries; entry 5 = g0:32 storage (Compute); per-stage 3 storage / 2 uniform / 1 storage-texture | **MATCH** |
| cc7 declarations | +1 (g0:32) − the overlay set | **95** (94 + `live_card_scratch`); the overlay retirement is invisible to the census — the retired set were module-scope consts/fns, not resource declarations | **MATCH** (with that note) |
| cc7 mirror | tombstones parked; zero orphans | 89 matched by number+name; zero orphans in BOTH directions; only the three documented `fc_` aliases surface as name mismatches | **MATCH** |
| cc4 entry points | write_live_card → the split pair | 63 → 64 EPs (`write_live_card_heights` + `write_live_card_resolve`); heights closure {0,1,32,160,161}, resolve {31,32} — both ⊆ the Writer layout | **MATCH** |
| cc4 agent sets | "unchanged binding sets expected" | **GREW — see Deviations D2** (all growth already bound in the CE layout; the handoff's intent holds) | DISCLOSED |
| Dawn witness (U roster + split-writer family) | ALL GREEN | 19 families / 30 entry points, **ALL PIPELINE FAMILIES GREEN**, zero module messages | **MATCH** |
| Final glaw1 | GREEN | `G-LAW 1: GREEN` at [T3a] (comment-only tree = the pushed tree) | **MATCH** |

## The [T1a] bit-exactness argument

The refactor is bit-exact for the bake path BY CONSTRUCTION, not by
tolerance:

1. `evaluate_lattice_wave`'s body moved VERBATIM into
   `evaluate_lattice_wave_pair` — every expression in its existing
   float order (the U5a discipline: the refactor moves lines, it does
   not re-associate them). The original function's ending was already
   `mix(val_frozen, val_moving, band_act)`; the pair returns those same
   two floats and the wrapper applies the same `mix`.
2. Therefore every call site that previously computed
   `mix(frozen, moving, band_act)` inline now computes the SAME two
   IEEE values through the SAME operation — no reordering, no
   re-association, no extra rounding step (vec2 construction and
   component extraction are value-preserving).
3. The gate branch widened from scalar `0.0` to `vec2(0.0, 0.0)`;
   `mix(0.0, 0.0, x) == 0.0` exactly.
4. At rest (`terrain_time ≤ 0`) the writer's band loop contributes
   nothing (the −1 sentinel skips every band), the card's Δh is
   pulses-only exactly as pre-batch, and the stills are unchanged —
   REST IDENTITY is Jean's bitwise gate below.

`true_band_delta_contribution` is NEW code (the delta form), used only
by the new heights pass — it cannot perturb any pre-existing path.

## The overlay retirement census

Retired at [T1c] (A3-3d certified `write_live_card` — gone at [T1b] —
was the sole caller):

- `fn terrain_wave_overlay_with_gradient`
- `fn overlay_band_params` + `struct OverlayBandParams`
- `struct OverlayWave` + `OVERLAY_WAVES` table + `OVERLAY_WAVE_COUNT`
- `OVERLAY_PROP_*` consts
- §2.2 ROW 7's overlay design-matrix paragraph (room text)

Kept deliberately: `WAVE_THRESHOLD` + softness — they are the
true-band pool thresholds (said at ROW 7). Post-retirement census over
`src/cartridges` (comment-inclusive, post-[T3a]): every remaining
mention of an overlay symbol is a tombstone or a historical epitaph;
zero code references (the code-only closure check at [T1c] was already
zero). cc7 confirms no resource declaration was involved.

## The fossil resolution (T2a)

`PAWN_GOL_GROUND_ENABLED` — T0 census found **exactly one** reference
in the tree: its own declaration
(`world.wgsl:2086: const PAWN_GOL_GROUND_ENABLED: bool = false;`).
Reference-free ⇒ the handoff's tombstone path (not the gate-removal
path): the const is replaced by the epitaph "compile-time gate for the
pre-card analytic chain — GROUND_CARD_1 retired the chain." No
behavior change; the walker's card-lift reality was never gated by it
in this tree.

## The normals disclosure (supersedes the batch-1 waves-only parity note)

Since [T1b] the resolve pass differentiates the WHOLE scratch Δh —
true-band waves + radial pulses — so the card's `.gb` gradients (and
therefore the terrain fragment normals) now shade under BOTH pulses
and woken bands. This is the campaign v2 §7.1 upgrade arriving here,
superseding GROUND_CARD_1's waves-only-gradient parity note. The VS
code is unchanged — `.gb` just got richer. Rest identity is
unaffected: zeros differentiate to zeros. This is a motif-review item
for Jean (gate list below), not a bit-review item.

## Deviations

- **D1 — [T2] landed as one commit, not three.** The handoff specified
  [T2a]/[T2b]/[T2c]; the edits were applied in one verified script run
  over overlapping files and committed together as `4c6e4ae`. The
  bisection line is coarsened by one step; each unit's content is
  delineated in the commit message and the campaign log.
- **D2 — cc4 agent binding sets grew** (the handoff expected
  "unchanged"). Machine-diffed (g0): `update_player_agent` +100
  (floating_entities — the sphere loop) +111 (agent_tier_gains — the
  player kernel never read the tier table before);
  `update_other_agents` +100 (111 already present via flock2d);
  `update_cube` +111; `update_sphere` unchanged. Every new number was
  ALREADY BOUND in the Compute Entity layout — the handoff's intent
  ("the gather reads only what's bound") holds, and the witness proves
  validation. The literal expectation was written against the
  gather-only reading; the sphere loops and the player's first tier
  read are what grew the sets.
- **D3 — [T3a] closeout comment truths** (out-of-spec, disclosed). The
  T3 census — comment-inclusive, unlike [T1c]'s code-only check —
  surfaced six comment sites still speaking the retired overlay's name
  as live truth (stale-since-earlier-eras claims among them: the
  patch-VS header's "uses terrain_wave_overlay_with_gradient", refuted
  by A3-3d; the live-card sample comment's "waves-only gradient",
  superseded at [T1b]). Folded to truth in a comment-only commit,
  U1b-stale-six precedent; glaw1 GREEN over it.
- **D4 — pulses-call signature** ([T1b], anticipated by the handoff):
  `contrib_radial_pulses_at(p, signal.t_seconds)` per its live form —
  the H3 deviation-6 precedent.

## Encoding sweep

All six touched files (`world.wgsl`, `state.hpp`,
`binding_registry.hpp`, `renderer.hpp`, `bodies/agents.hpp`,
`surface/terrain_looks.hpp`): **no BOM, LF-only**. The FXC constraints
banner (world.wgsl §SEAM fxc-constraints) is byte-untouched.

Note: `web/shaders/world.wgsl` (the browser mirror) predates this
campaign and the U/H batches; the resync ritual (CLAUDE.md) is Jean's
standing call and was not part of any batch closeout — flagged, not
touched.

## JEAN'S GATE LIST

- [ ] **Windows build + boot.** FXC watch: the split writer pair (the
      bake's own proven two-pass shape at card size), the two 33-slot
      gathers (probe-blessed on Tint; FXC's word lands here), and the
      refactored `evaluate_lattice_wave` under the bake.
- [ ] **REST IDENTITY, bitwise** — the pair refactor is the risk this
      time; the stills are its judge.
- [ ] **THE BREATH:** locally pin (terrain_looks ROW 2, revert after)
      `terrain_time` to the beat clock and `band_blend = 1.0` for
      bands {5, 0, 1} — watch the pools: continental swells in the
      world's own seeded geography, waking bands growing out of the
      frozen shape (no teleport), band 4's fine ripple properly still.
      This is the gen-1 wires' first true sound; the gen-2 couplings
      that drive them for real are the coupling campaign's opening act.
- [ ] **THE CONTACT WALK:** walk into an agent cluster (they yield,
      you barely feel it); watch a crowd breathe apart; stand in the
      sphere's path (it pushes you); shoulder a cube (it gives, then
      returns on its drift spring).
- [ ] **Normals under music:** pulses and bands now shade (the
      disclosed upgrade) — motif review, not bit review.

## Seeds

The stamped arc of campaign v2 is COMPLETE at this batch's green.
What follows is the coupling campaign on the unified substrate — the
drift/morph/breath rulings, Layer E, the band wires' gen-2 drivers,
the presence and paint cards — and the control panel that plays it
all.
