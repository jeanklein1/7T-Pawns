# RIBBON COLOR & CELLS — COUPLING DATASHEET (post-SS-3)
The couplable surface of the ribbon's color system and cell skin, exposed in
contract-datasheet form. Classes per the standing key: L-global (body-wide,
Segment-safe), LH (through the head's history), D (discrete; selection +
state inheritance), C (identity/law; not a live target). Every pipe listed
with its idle, because rest = identity is the safety contract.

Standing rule: this file updates in the SAME COMMIT as any change to the
surface it describes.

## 1. PER-RIBBON PARAMETERS (GPURibbonState; written at commit, static per
##    life today — any per-frame coupling needs the flush seam, noted where)

| field            | meaning                              | class | idle (rest)            | coupling notes |
|------------------|--------------------------------------|-------|------------------------|----------------|
| color[3]         | dark median / uniform color          | L-global | spawn draw (pair.dark, free dark median, or SMOOTH/TINTED draw) | LIVE (coupling #3, T2, RECOMPOSED by SS-2): ribbon.color_stim (rows 6–8) + ribbon.color_mix (row 9); the conductor's flush computes lerp(spawn, stim, mix) per mirror and the held-slot upload ships it; mix rest 0 ⇒ spawn exactly. COLOR = THE ROOM: the Wagon AIMS the hue ("all.window_length" resultant — no argmax flicker on chords); the Playhead GATES the mix ("all.present_count": sounding ⇒ worn, silence ⇒ fades on its last hue). Same 30°-per-semitone seating, shared ORIGIN ⇒ equivariance. ENVELOPE (ENV-1): mix attack 0.5 / release 3.0 beats (catches fast, fades long); hue re-aims on 2.0 |
| color_b[3]       | light median (CONTRAST)              | L-global | pair.light + shared jitter, or free light median; median-field species: ≡ color by construction | opens a NEW musical dimension: drive the medians apart/toward ⇒ "contrast" itself as a coupled quantity. Commit-only today (rides the full-struct upload) |
| checker_scatter  | per-cell lightness texture amplitude | L-global | pair.value_var or free draw | texture-energy idiom; pipe as multiplier, rest 1. Commit-only today |
| hue_spread       | the colorful axis, radians [0, π]    | L-global | pair.hue_var(±sib)·π or free draw [0, π] | THE riot dial; pipe as additive deviation, rest 0; needs per-frame flush seam (commit-only today) — ledger item |
| seed             | GPU hash key                         | C     | spawn seed             | identity; never coupled |
| color_mode       | SMOOTH / TINTED / CONTRAST           | D (birth) | seed roll × weights  | population composition; a re-raffle-on-event coupling is possible but is a REBIRTH-class act |

## 2. PAIR-TABLE + FREE-RAFFLE PARAMETERS (CPU console; design-time, authored)
CHECKER_PAIRS rows {dark, light, value_var, hue_var, weight} — C at runtime,
the curated authoring surface.

FREE RAFFLE region spec (CB-1f) — the generative authoring surface beside
the table: FREE_DARK_LUMA/CHROMA, FREE_LIGHT_LUMA/CHROMA (disjoint luma
bands are the one kept law — the chessboard's legibility contract),
FREE_VALUE_VAR, FREE_HUE_VAR (uncapped [0,1]), and the CHROMA_D1/D2
Rodrigues basis (the CPU twin of the shader's hue machinery). The bounds
ARE the lattice; narrow to tame, widen to liberate.

FREE_PAIR_CHANCE (0 = pure curation, 1 = pure generation) is the population
dial and a spawn-time D-class coupling candidate — section-level shifts
between curated and wild.

MEDIAN-FIELD region spec (CB-1g) — the species above both pair paths:
one median raffled in (luma, chroma, hue), color_b ≡ color so the parity
term dies by algebra and the per-cell machinery carries everything —
the terrain-patch grammar on a tube. MEDIAN_LUMA is broad (no parity to
protect); MEDIAN_VALUE_VAR's floor sits higher so cells read through
texture. CELLS_MEDIAN_CHANCE is the species dial (spawn-time D-class
candidate, same idiom as FREE_PAIR_CHANCE).

Two further future D-couplings live here:
- PAIR RE-RAFFLE: a musical event re-rolls a ribbon's pair (state
  inheritance: the body keeps flying; only the skin's medians re-target
  through Segments — the CA-rule-swap grammar, applied to palette).
- WEIGHT MODULATION: mood/section-level shifts of the raffle distribution
  (spawn-time; changes the flock's composition, not existing ribbons).
- CONTRAST COLLAPSE (ledger, CB-1g): the species boundary is the distance
  between two struct fields, so a coupling driving color_b toward color
  dissolves a chessboard into a field LIVE (and back) — contrast as a
  fader, not a category. L-global; needs the color_b wire when its day
  comes.
- DEGREE→ANGLE SEATING (ledger, T2 rev A): the map is a swappable table,
  one line at the tint decode (and the compass shares the law) —
  chromatic today (θ = d·30°, the identity seating); circle of fifths
  (θ = (7·d mod 12)·30°, harmonic distance as angular distance) and
  authored orderings are one-line futures. Equivariance holds per
  re-seating: any fixed assignment transposes as a rotation. NB: a fifth
  is +7 semitones = 210° under the chromatic seating (a fourth, +5,
  gives 150°).
- ColorSlot RETIRED (T2): the in-module target surface (ColorSlot +
  ribbon_color_targets) deleted with zero consumers since M1 — the bank
  rows are its successor.
- CIRCLE-OF-DIRECTIONS REWORK (ledger, SS-2): PARKED, Jean's ruling
  pending — the seating comment at the tint decode marks the door.
- CHANNEL NAMES (ledger, SS-2): the scaffolding names (abbott/costello/
  louise) are retired; channels are numbered chN. The currency caveat is
  CLOSED definitively — the live publish inventory, read from
  canvas_1/canvas.hpp: per voice (ch0..ch6 active of ch0..ch7 named):
  chN.current_pc, chN.present_count, chN.window_length, chN.distance;
  room-wide: all.field, all.current_pc, all.present_count,
  all.window_length. PresentCount was published on demand by SS-2 (the
  sustain law is its first consumer — readings-on-demand firing on
  schedule); its reserved layout was slot 0, the seat identified as
  gen-1's dead input during the demolition — the oldest empty chair in
  the contract, now filled by its intended occupant.
- COMPASS CONCEPT (ledger, SS-2): the pitch compass now lives in color
  (the Wagon aims the tint); it returns to movement when a directional
  target calls.
- CANON (method): scope claims about unverified surfaces are
  predictions, and verify-first gates are their mandatory counterweight
  — SS-2's "visual_canvas.hpp ONLY" line met readings-on-demand and the
  law won, exactly as designed.
- SIDECHAIN IDIOM RETIRED (ledger, SS-3): presence-ducking punishes
  short gestures — staccato dipped the body BELOW idle, an inverted
  response (the lesson). The 2× ceiling had already made the duck's
  visibility job obsolete; additive-over-identity-rest is the deviation
  law's pure form. Music ADDS to the idle dance; it never suppresses it.
- ENVELOPE IDIOM (ledger, ENV-1): attack ≠ release, chosen at the CALL
  SITE — the caller authored the idle, so it knows its direction
  (goal == idle ⇒ RELEASE, else ATTACK); trajectory.hpp untouched. The
  exact compare is sound only under SS-3's additive law (baseline is
  the unique at-rest goal — the retired duck would have broken it).
  Fog stays symmetric for now (stepwise-rare source); extending it is
  three lines. BREATH constant (independent re-articulation dip span)
  ruling OPEN — one line on the word.
- FRAME LAW (ledger, BNK-1): the ring frames now answer the wave — the
  nose aims along the true instantaneous motion (heading deflected by
  the analytic wave slope) and the body BANKS into the lateral swing.
  Slope ∝ amplitude, so the sustain swell deepens the carve and the
  lean with no extra pipe; identity at gains 0/0. This changes the IDLE
  look deliberately. Hot-loop dials in world.wgsl:
  RIBBON_TANGENT_ALIGN / RIBBON_BANK_GAIN / RIBBON_BANK_MAX. The
  checker skin rotates through the roll — the faces know they dance.
  MOUNT GIMBAL RULING OPEN: the mount and camera still read the CPU
  yaw-only pose — the saddle rides gimbal-level over a banking body,
  keep-or-mirror pending. The diameter law (DM-1) remains parked and
  COMPOSES with this: the diameter would reorient the wave these
  frames answer.

## 3. SHADER CONSTANTS (world.wgsl, hot-reloadable — scene-level dials)
| const                | meaning                         | class | notes |
|----------------------|---------------------------------|-------|-------|
| CHECKER_CHROMA_DIR   | canonical chroma direction      | C     | basis; also the near-gray fallback direction |
| CHECKER_CHROMA_FLOOR | chroma at full spread (CB-1e, live) | L-global (scene) | "palette punch" — a scene pipe candidate; superseded and replaced CHROMA_GAIN |
Scene-level pipes over shader consts require a small uniform hop when ever
coupled; today they are the hot-tuning loop's knobs.

## 4. PER-CELL EXPRESSIONS (the skin pipeline — what the scalars modulate)
cell_id = k·4 + f (segment × face); caps outside the grid (median A).
  p        = (k + f) & 1                      — parity (the chessboard)
  hue_a    = (hash_hue(cell) − .5)·2·hue_spread — per-cell hue angle
  chroma   = reconstruct: rotate(dir(base), hue_a) · max(|chroma|, spread·FLOOR)
  value    = pole-lerp: mix(base, black|white, |hash_v|·checker_scatter)
Every expression is fixed structure; couplings act ONLY through the §1
scalars — the cells never learn what drove them (the categorical boundary,
per-cell edition).

## 5. CELL STATE (CB-2, pending the idiom fork — the D-class grid)
When the living grid lands: per-cell state buffer (binding 40 reserved),
rule id (ParamShape::Discrete; swap inherits state — Jean's CA principle),
stimulus injection at the head (EVENT-class sources: current_pc, distance),
evolution on the Wagon's beat. State modulates §1's medians per cell
(alive→hot median, age→lerp). Idiom fork open: evolve-in-place (GoL feel)
vs travel-at-P (historian feel); layer one above is idiom-independent.

## 6. RELATED SURFACES ALREADY LIVE OR TAGGED
fog.density / fog.color — the played coupling (all.field, held→table).
ribbon.amp_lateral_mult / ribbon.amp_vertical_mult — PARAM_LAYOUT rows
4–5, LIVE (coupling #2, REDESIGNED by SS-2, made ADDITIVE by SS-3:
sustain swell — movement carries TIME): the dance swells with how long
the current chord has held, uninterrupted, on the ribbon's cast voice
("ch1.present_count", the Playhead's sounding set; the casting sheet's
first row). LAW: 1 + contribution (additive; idle inviolate) — music
only ever gives. Any change to the set re-articulates: breathe to
baseline (1.0), regrow. RULED: ceiling 2× idle, reached at 8 beats.
Silence ⇒ 1 from the formula itself (identity by construction, not by
branch). ENVELOPE (ENV-1): attack 0.35 / release 2.0 beats — fast
catch, slow let-go; release governs the re-articulation breath and the
after-silence let-go (span chosen at the call site: goal == idle ⇒
RELEASE). Multipliers composed over the spawn-drawn wave amps at the
conductor's per-frame flush; the pawn mount reads the same mirror —
the rider breathes with the coupled dance for free.
Terrain palette machinery — DRIVERLESS, scene-level future siblings
(band motion, palette drift, mode color shift) — same grammar, one scale up.
sphere/floater color — DRIVERLESS landing sites from the demolition.

## IDLE MAP SUMMARY (what silence looks like, per pipe)
Every color/cell pipe's rest reproduces the seed-drawn, pair-raffled OR
free-raffled skin exactly. A stranger reading only this table can wiggle
each row on the future panel and predict the screen — that is the
datasheet's test.
