# OPEN — the register of open state
One line per item: what · origin (sha or doc) · what unblocks it.
This file is the ONLY home of open/parked state. When an item closes, its line dies.

## THE iOS BLACK SCREEN — CLOSED 2026-08-28 (IOS_5)

**A render bundle with `colorFormatCount = 0` — depth-only, a
depth-stencil format, executed in a depth-only pass — is not handled
correctly by WebKit's WebGPU.** The shadow map came out wrong and every
iOS device rendered the world black. The sun's bundle is deleted on all
browsers; the pass encodes direct. See L48, L49.

THE INTERSECTION THAT NAMED IT, on the iPad, four reloads:

| switch | what it removes | iPad |
|---|---|---|
| `?sunpass=0` | the sun pass's DRAW LIST (pass opens and clears) | **renders** — with the main bundle still executing |
| `?bundles=0` | `ExecuteBundles` in both passes; encode direct | **renders** — with the sun's draws still issued |
| `?bake=0`, `?card=0`, defaults | — | black |

The only thing both remove is `ExecuteBundles(shadowSunBundle_)`. No
`[gpu]` and no `[lost]` appeared: not refused at validation, and the
device did not die. It executed, and the depth was wrong. Black rather
than bright is a map ending near 0.0, which the PCF compare reads as
everything occluded.

THE BOOT CARD SETTLED THE REST BY OBSERVATION, verbatim:

    seed 1984484084 (drawn)
    mood 0 open_sunset open
    patches allocated 225 baked 225 landed 225

So: the seed varies, the world builds COMPLETELY, and an outdoor roll
stays outdoors. **There is no OPEN→ROOM fallback** — the "indoors" boots
were honest indoor rolls of the mood, and they render correctly. IOS_4's
R-1, R-2 and R-3 are answered by observation and closed; R-4 is moot.
The recon that predicted a fully-formed world (225/225/225) and a black
screen meaning "the pass did not run" was right on the first count and
wrong on the second — it ran, and produced bad depth.

WHAT STANDS FROM BUNDLE_1, confirmed good on WebKit by this same
evidence: the draw ledger, the indirect draws, the encoder-generic verbs
and the MAIN bundle. Only the depth-only recording is retired.

OWED — THE WEBKIT BUG IS NOT YET FILED. A draft report is at
`docs/reference/WEBKIT_BUNDLE_BUG.md`; filing it is Jean's (it is an
outward-facing post under the project's name). Link it from renderer.hpp's
banner once it has a URL. Unblocked by the filing.

## THE FIELD DIAGNOSTIC SET — PERMANENT (IOS_5, extended by AUBADE)

Seven switches, in EVERY build, forever. They found the iOS black screen
in four reloads on a device with no console, and they are the only
instrument that works in a gallery: no inspector, no log, no cable — a
picture and a photograph of it.

| switch | removes | the picture it must produce |
|---|---|---|
| `?bootinfo=1` | nothing — opens the card | the identity block, and `patches: allocated / baked / landed` once a second |
| `?bake=0` | the heightfield bake dispatch (cells still run) | **flat but VISIBLE** — the texture's zero-init is a flat world, not garbage |
| `?card=0` | `write_live_card` | no zone lift, no live delta |
| `?sunpass=0` | the sun pass's DRAW LIST — **never the pass** | the world with **no sun shadows** |
| `?bundles=0` | `ExecuteBundles` on the main bundle; encode direct | **identical** |
| `?adopt=0` | the page's pre-created device; C++ asks for its own | **identical**, and slower to first light (AUBADE U2) |
| `?async=0` | asynchronous pipeline creation; all sixty compile before the loop runs | **identical**, and the old boot's long dark (AUBADE U3) |

THE LAST TWO REMOVE A STAGE OF THE BOOT rather than a stage of the
frame, and they answer the same rule for the same reason: both are
mechanisms a browser can get quietly wrong, and neither produces a
picture of its own. What they are for is bisecting a boot that never
arrives — on the device with no console, which is the only place this
question has ever actually been asked.

THE PICTURE IS WHAT MAKES A SWITCH READABLE, and `?sunpass=0` is the
example of what a switch must never be. Skipping the whole pass would
leave the shadow map at its zero-init, which the PCF compare reads as
everything occluded — a world entirely in shadow, nearly black, and
therefore INDISTINGUISHABLE FROM THE FAULT WE WERE CHASING. A diagnostic
that can be mistaken for the disease is worse than none. It skips the
draws and keeps the clear, and that is why IOS_5's diagnosis was
unambiguous on the first reading instead of costing another round.

Only an explicit `0` removes a stage; absent or malformed leaves the
piece entire, and the `[Params]` line names any switch that is off, so
silence means the piece whole and a diagnosis stays reproducible.

## AUBADE — first light — CLOSED 2026-08-28, merged at `fcbed3c4`

Boot rearrangement: nothing new was built, existing work was resequenced
so the screen lights early and the world assembles in view. Seven units,
`0ee6bc90` … `fcbed3c4`, merged to master by fast-forward on Jean's word;
the branch is retired per ATTIC LAW. **The follow-on is AUBADE_1, below.**

| unit | what moved |
|---|---|
| U1 | the waterfall and the attribution probe — the program's FIRST first-present witness |
| U2 | the device is asked for at HTML parse, adopted by the wasm, verified by the same census |
| U3 | every pipeline creates asynchronously; first light gates the loop, rows skip until ready, shadows land as one event |
| U4 | the veil hands off at first present, not at a substring of a log line |
| U5b/c | the paintings' bytes leave at manifest scan; decode, pad and upload park until first light, then one a frame |
| U6 | the four versioned artifacts are immutable for a year |
| U7 | the package holds the shader and nothing else; three preloads start first light at HTML parse |

### U5a — STRUCK AT BUILD (ruled 28 Aug 2026)

U5a asked for the world's hung set to resolve at manifest scan as a pure
function of (seed, record), ordered by the guaranteed door and then by
gallery spawn distance. It is **struck**, on three facts checked in the
tree rather than on a difficulty:

- Galleries are not a world-level quantity. They are spawned per TILE,
  through `compose_spawn_chance(c, gx, gz, PopFamily::GALLERY)`, as tiles
  stream. A world's gallery set does not exist at scan, does not exist at
  population, and keeps growing while the visitor walks.
- The guaranteed door is `force_spawn_door_fallback` (mood.hpp),
  conditional on the ROLLED ARCHES — it reads `entities_state_.arches`
  and stands down if one already falls within `DOOR_NEAR_RADIUS`. It runs
  at population, after the device, and it is a portal rather than a
  gallery.
- There is no gallery-to-painting map at all. A room hangs from whatever
  the staging pool holds when it is built; `disk_index` comes from the
  fill's linear walk of the manifest.

Ordering the fetch would therefore mean **authoring a seed-determined
exhibition** — which painting hangs in which room. That is a change to
what the piece IS, not a resequencing of when its bytes move, and the
campaign's frame is that nothing new is built.

**AND THE GOAL ARRIVES WITHOUT IT, BY A BETTER ROUTE.** Because rooms
dress from the POOL, *any* six staged are the right six: no byte needs to
know its wall, because the walls take what has arrived. With the bytes
leaving at scan (U5b) and the valve filling fast (U5c), the first
reachable room is dressed by construction. The boot fetch count was
already invariant — the fill has asked `min(manifest, 32)` since
OVERTURE_0, so a catalogue of 57 or 500 sends the same 32.

If a curated exhibition ever appeals artistically, it is its own future
campaign and not a boot unit. **Not open, and not owed.**

### SUPERSEDED — OVERTURE_0 U9's SIX-PAINTING FLOOR

Recorded once, here, so OVERTURE_0's history stays one fact in one home.

U9 put six staged paintings in front of the veil so the visitor met a
furnished world. **U4 moves the veil to first present, which is EARLIER —
ahead of that floor.** The floor's promise is kept by the valve instead:
six stage within frames of present, rooms dress at spawn, and no room is
reachable before they do. **READY stops being a count and becomes a fact:
the first reachable room is real.**

Two OVERTURE_0 items die with it:

- *"The READY offer holds a gallery-less build for the full timeout."*
  U9's floor could never rise with `ROSTER.gallery` off, so such a build
  always paid the 5 s. It costs the visitor nothing now — the veil is
  already up, and the only thing the timeout still governs is when the
  `Controls:` line arms U4's belt.
- The 5 s timeout as a live path. It becomes the rare one.

The `Controls:` line is NOT retired. It no longer lifts the veil; it arms
a 20 s belt, so a browser that renders but never resolves
`onSubmittedWorkDone` still gets its world — and the log names which path
lifted it, present or belt. Its text stays load-bearing, one notch
weaker.

### THE PASTE — TAKEN, AND IT CLOSED R2

`fcbed3c4`, cold:

    [AUBADE] waterfall net=14 wasm=82 device=158 init=326 firstlight=5062
             present=5403 authored6=- ready=5403
    [AUBADE] window ticks=7 cpu=38526ms stb=0ms fade=0
    [AUBADE] pipelines firstlight=7 ready@4747 rest@5077 total=57 failed=0

**P1 CONFIRMED. R2 CLOSED, by instrument, with no trace read by hand.**
The occupant of the dark is pipeline compilation — 4.4 s of it, resolving
roughly serially in issue order. `stb=0` and `fade=0` acquit the other
two suspects by name: no painting decode, no authored darkness. U3's own
side-repair is what proved it, because the table now prints
resolve-minus-call: `pawn: 4743 ms compile, call at 3 ms`.

**preview-A IS STRUCK FROM OWED.** The paste in hand closed R2 on its
own; the tag was never pushed (CC's proxy 403s tag pushes) and is not
needed. This paste stands as AUBADE's after-photo and AUBADE_1's
before-photo.

The warm paste — U6's proof — is still owed, and moves to AUBADE_1's OWED
with the rest of the sitting.

### PARKED, WITH THE CONDITION THAT REOPENS IT

- **Off-thread browser decode** (`createImageBitmap` ->
  `copyExternalImageToTexture`). It moves fetch and pixel ownership
  across the wasm boundary and into the port's texture interop —
  machinery unjustified for now. **Reopens if** the post-present trace
  shows U5c's valve still hitching.
- **Music prefetch-next.** The mechanism is deliberately not built: one
  tune exists (`samsara.mp3`, 4.0 MiB) and machinery for a library must
  not precede the library. The design, for the one-commit follow-up: on
  the gesture fetch tune 1, decode, bind the ears; during tune k prefetch
  tune k+1, logged `[AUBADE] tune k+1 resident`, so a change never waits
  on the network. **Reopens when** tune 2 lands. R12 stands for CHORD:
  analysis is not live, so a whole-tune sidecar riding with its audio
  gives the Playhead unbounded look-ahead — the pre-modulation hint's
  natural home.

### REJECTED — recorded so they stay rejected

- **Shell cache-warm.** 8.8 MiB of exhibition is not fat, and U5b already
  starts true fetches at scan; no second mechanism for one fact. RUL-D.
- **Atlas packing.** Fights aspect variety; HTTP/2 multiplexes already.
- **Thumbnail LQIP.** Galleries dress after first present and beyond
  arm's reach; blur-to-sharp is a material change the piece doesn't
  believe in.
- **BC-compressed paintings.** `texture-compression-bc` sits vaulted by
  the feature wallet; the wallet doesn't reopen for 40 MiB of RGBA that
  already fits.
- **Splitting `world.wgsl`.** Once compile leaves the critical path (U3),
  its duration stops mattering; the single-module sha discipline stays.
- **Service worker.** U6 covers repeat visits; heavier machinery buys
  nothing here.

### WHAT AUBADE LEFT BEHIND

Everything else it owed now sits in AUBADE_1's OWED, below, because that
is where the sitting is. Two items carry across unchanged:

- **The iOS witness (L48).** AUBADE touched pipeline construction and the
  render path; AUBADE_1 touches the same. It is taken on PRODUCTION now,
  post-merge, on Jean's authority — see below.
- **The WebKit bug is drafted, not filed** (`docs/reference/
  WEBKIT_BUNDLE_BUG.md`). Outward-facing, under the project's name, so
  Jean's. Its URL goes into that file and into renderer.hpp's
  `main_bundle_ready` banner.

## AUBADE_1 — first light first (on master, awaiting one production paste)

One reorder and two repairs. AUBADE built the gate correctly and queued
it behind everything it was meant to skip; this campaign issues first
light first. No visual gate: the first-light SET is unchanged; what
changes is when the rest arrives.

| unit | what moved |
|---|---|
| M0 | the merge — `claude/aubade` fast-forwarded to master, branch retired |
| F1 | the seven first-light creates are issued before the other fifty |
| F2 | `cpu` sums deltas, `firstlight` is one mark, a late mark prints its own line |
| F3 | a `pending` page is awaited (2000 ms) instead of read as a refusal |

### THE FINDING F1 REPAIRS

The seven first-light pipelines resolved at queue positions
**{1, 5, 6, 10, 28, 29, 30}** — the gate waited behind ~23 pipelines it
does not need. The assembly-order ruling was Claude's: it ordered the
dawn and accidentally ordered the gate last. `rest@5077 < present@5403`
says it plainly — the world arrived COMPLETE BEFORE THE FIRST FRAME,
which is the exact opposite of assembling in view.

Creation order carries no semantics, so the repair is an ordering and not
a restructuring: the creators are called twice and each issuer answers
only its own pass. Membership, ready bits, layouts and the shadow
conjunction are untouched.

### THE THREE THINGS THE PROBE WAS SAYING WRONG (F2)

- `cpu=38526ms` inside a 5.4 s window — it was summing `now() - s_frame0`
  where `s_frame0` is only assigned under `if constexpr (frame_meter)`,
  and the meter is off in the shipped build. Absolute timestamps, summed;
  the number grew with the machine's uptime.
- `ticks=7` in that same window — not in the handoff, found while
  fixing the above, and the same lie wearing the other shoe. U3's gate
  returns before the frame body and the tick was counted after it, so
  4.4 s of compile counted seven turns instead of some three hundred. The
  tick is now counted ABOVE the gate (a turn the gate sent home is still
  a turn) and `cpu` below every early return (a turn that did no work
  adds none). That asymmetry is the whole reading.
- `firstlight` printed twice, 4747 and 5062 — one instant, two clocks.
  The site that takes the mark now prints the value the mark hands back.
- `authored6=-` — the report fires at `ready` and the valve stages
  nothing until after first present. A missing mark prints `pending`, and
  any mark arriving after the report prints its own line.

### THE FINDING F3 REPAIRS

`[Device] the page has no device to hand over (state=pending)`. U2's
overlap was real and C++ simply arrived first: the shell's promise was
unresolved when C++ asked, adoption fell to the fallback — gracefully,
census intact — and the boot paid two adapter requests instead of one,
about 150 ms. `pending` was being read as a refusal. It is now awaited,
2000 ms, in the boot's own pumped state machine.

### THE PERCEPTUAL CONSEQUENCE, SAID HONESTLY

In the `fcbed3c4` build the world arrived complete before present
(`rest@5077 < present@5403`), so nothing was ever seen to assemble. After
F1, present precedes the rest and **the assembly becomes visible for the
first time** — which is the original brief: a few frames for everything
to sit in place beats a few seconds of nothing. One production look
confirms the taste. If the dawn ever reads as damage, the rest's assembly
order is the tuning knob, and that is a stanza for a future round rather
than a fault.

### OWED — Jean, minutes, all against production

The tip has still never met emscripten; every witness both campaigns ran
was headless, so the build is the first true gate and a compile failure
is a paste to CC, not a fault.

1. Build master → `python tools\web_dist.py` →
   `npx wrangler pages deploy dist --project-name=7t` (production, no
   `--branch`).
2. Open production cold — new hashes make it cold by construction — and
   paste the `[AUBADE]` lines. F1/F3's witness and the campaign's
   after-photo.
3. Reload; paste warm. **U6's proof, finally taken.**
4. Open production once on the iOS device. **L48's witness** — the one
   law-item still outstanding — completed on Jean's authority
   post-merge; one line back to this file. If anything looks wrong the
   bisect is two switches, `?adopt=0` and `?async=0`, from the
   seven-switch set above.
5. File the drafted WebKit bug when convenient.

**CC could not delete the remote branch.** `git push origin --delete
claude/aubade` is refused by the same proxy that 403s tag pushes; the
local branch is gone and master carries every commit. One command from
Jean's side closes ATTIC LAW's second half:
`git push origin --delete claude/aubade`.

## THE OPTIMIZATION ARC — CLOSED 2026-08-28 (PURSE_0)

LATTICE_0-5, SPINE_2, BUNDLE_1, PURSE_0. Both bookend captures are on file
(pre-campaign and post-campaign, both devices, `docs/reference/`), which is
what lets this be a close and not a claim.

| device | purse, before | purse, after | fps |
|---|---|---|---|
| Pixel (floor device) | +1.8 | **+4.4** | ribbon flight presents a wall of 1x |
| Laptop (2012 Kepler, 689x607) | -3.0 … -5.4 | **-0.45 … -0.84** | 38-43 -> 50-53 |

The floor holds. THE LAPTOP'S RESIDUAL STUTTER IS THREE THINGS, AND THE ROUND
TOOK TWO OF THEM: (1) a baseline purse just under zero — NOT CLOSED, and
PURSE_1 ruled it will not be: the three dials cost definition, not fat, and a
machine one refresh short is not a machine the work is reshaped for (see THE
SETTINGS ARE THE SHIPPED DEFAULTS below); (2) the photographer's +10 ms capture
every 1-2 s — TAKEN by PURSE_0 B, which defers a capture to a frame the
presentation law says made its refresh, with a 4 s ceiling so a slow machine
still fills its pool; (3) portal transitions at 60-75 ms CPU — the PRINT half
taken by PURSE_0 C, the rest standing.

WHAT THE ARC RETIRED, AND WHAT IT DID NOT:

- **LATTICE_5, the meter bookend — TAKEN.** Both captures exist. LATTICE_1's
  arithmetic ("~3x fewer ground evaluations, 15.5x less heightfield memory,
  ~20x less derivation work") stopped being a prediction here.
- **LOOM, the Kepler two-canvas split — PRICED, NOT TAKEN.** ~9 ms of the
  laptop's main pass is PIXEL-INDEPENDENT: canvas 689x607 -> 1366x607 (+98%
  pixels) moved `main_pass` 11.74 -> 10.3 at 0.83 vs 0.42 Mpx. It is the
  vertex/state wall, not fill. The preset serves that machine for free, so the
  split is not bought.
- **PIXEL — FALSIFIED ON KEPLER by the same split.** The lever was resolution;
  the numbers say resolution is not what costs.
- **The eight-pass compute fusion — DEFERRED, and now priced.** SPINE_2 B
  established the shape (card+compute fused; placement+cull and the six-pass
  run between the witness copies and placement still available). Pass
  boundaries are pennies against both devices' rows, and every one of them
  costs a meter row. Not worth measurement resolution.
- **CELL_LIVE — STILL FILED as the music precondition.**
- **UMBRA — STILL HORIZON.**

THE DANGLING FXC CITATIONS ARE CLEANED (`banner rule 2` / `banner rules 2, 3`
at `field_sum` and the occupier loop). The shapes they described are kept and
now say they are INHERITED; the rules were struck at PIVOT_0 and live in
`docs/FXC_LAWS_RECORD.md`, whose first line is "Do not honor these as live
constraints."

### THE SETTINGS ARE THE SHIPPED DEFAULTS (PURSE_1)

**No `kepler` preset. The shelf stays clean.** The dial session is HELD, not
skipped — it was run to the point of knowing what the three dials cost, and
what they cost is DEFINITION, not fat:

| dial | value, unchanged | why it is not turned |
|---|---|---|
| `veil_ring` | **325** | It sets where the world ends. Shrinking it moves the horizon inward, and the subject of the piece is an ever-expanding board — a nearer edge is a smaller world, visible to everyone and not only to the machine that needed it. |
| `lod0_radius` | **175** | It sets where the mesh halves its density. At 120 the seam sits inside the walking eye's range on a machine with poor vertex throughput, so the trade lands exactly where it shows. |
| `shadow_pcf_taps` | **16** | 16 -> 4 narrows every penumbra everywhere. The sun's softness is authored, not incidental. |

A 2012 Kepler at -0.6 ms is A MACHINE ONE REFRESH SHORT, not a machine the
work should be reshaped for. The floor device holds at +4.4 with clean 1x
presents, and the exhibition machines are chosen. A preset committed to the
shelf now would be a permanent answer to a temporary machine.

**THE DIALS ARE THE FIELD INSTRUMENT, AND THAT IS WHY THEY STAY LIVE.** They
remain enrolled, exported and importable as an installation tool: if a
gallery's hardware misses budget on the day, the operator opens `?organ=1`,
turns `veil_ring` / `lod0_radius` / `shadow_pcf_taps` one at a time, and reads
the purse off the `[METER]` window's budget line — one dial, one window, one
reading. All three round-trip through the shelf (verified at PURSE_0 R-D), so
a settled answer CAN be exported and named on the day it is needed, by someone
standing in the room. It is not committed here because nobody is standing in
that room yet.

The audience build ships from the `the-board-web` preset through
`web_dist.py` with no `--lab`, and that artifact is the only one on
everexpandingboard.com.

### THE CENSUS TABLES ARE TRANSITION WITNESSES (PURSE_1, the PURSE_0 residue)

The bucket is **P6 transition witness**, not "already behind the dial" — and
only PART of the census answers to the dial. Four call sites, three of them
UNGATED in the shipped build:

| trigger | site | gated? |
|---|---|---|
| `boot` | `Cartridge::initialize` — agent + entity | no |
| `mood-transition` | `phase_transition_machine` — agent + entity | **no** |
| `born` | `stream_patches`, the fullRegen arm | **no** |
| `periodic` | `phase_census_dumps` | YES — `INSTRUMENTS.census_entity_dump` |

A PORTAL CROSSING PAYS THREE PRINTS, NOT ONE, and the third is the one a
reader would miss: `reset_surface` sets `last_center_x = INT32_MAX`, which
re-arms `fullRegen` on the next frame, which fires the `born` census. So a
crossing prints the mood-transition agent census, the mood-transition entity
census, and then the born entity census — all of them in the audience build.

THIS IS NOT A DEFECT TO FIX BY GATING. P6 protects them and the tree's own
banner rules it: "The census at 'boot' and 'mood-transition' stays; only the
PERIODIC one answers to the dial." The mood-transition entity census is
load-bearing beyond witness — its own comment calls it "a
teardown-completeness assertion, not an observation": both columns must read
0 for all twelve, and a silent teardown leak is exactly what it catches.

SO THE THIRD STUTTER STAYS OPEN AND UNPRICED. Portal transitions at 60-75 ms
CPU; PURSE_0 C took the STEADY-STATE print half and this is not that half.
The tree's only measured census number is a BOOT reading in a `full` build
(`census_dumps` max 1051 ms, 2026-08-13, instruments.hpp) — it does not price
a crossing, and no reading of a crossing exists. Unblocked by one, and the
instrument is already built: the meter clocks the phases that pay, so
`[METER] U transition_machine` and `[METER] R stream_patches` on a `meter`
build across a keyed crossing give the number directly. Until then the honest
statement is that the shipped build pays three census tables per crossing and
nobody has measured what that costs.

## STILL OPEN OUT OF PURSE_0

- W4 IS A `full` WITNESS NOW, NOT A `meter` ONE. PURSE_0 C gated the
  `[Photographer] … pool=N/32` line on `stream_witness`, which the meter
  column drops by the dial's own doctrine (per-event blocking writes stay out
  of the build that measures frames). Watch the pool climb on
  `-DT7_INSTRUMENTS=full`; read the purse on `meter`. Unblocked by Jean ruling
  otherwise, in which case the line moves to `frame_meter`.
- THE THIRD STUTTER IS UNTAKEN: portal transitions at 60-75 ms CPU. PURSE_0 C
  removed the STEADY-STATE print half and this is not that half — a crossing
  still prints three census tables in the shipped build, and the teardown /
  respawn work under them is untouched. The mechanism and what would price it
  are in "THE CENSUS TABLES ARE TRANSITION WITNESSES" above; this line is the
  register's pointer at it, not a second copy. Origin: the post-campaign
  laptop capture.

- THE FRAME'S COMPUTE IS TWO PASSES, NOT ONE, AND THE REST OF THE RUN IS
  UNRULED. SPINE_2 B asked for four passes in one — card, placement, compute,
  cull — and the tree refused the shape: R11 WitnessCapture encodes three
  CopyBufferToBuffer between R10 and R16, a copy cannot live inside a pass,
  and O-2 pins the capture after the compute by static_assert. What landed is
  the card as the FIRST DISPATCH of the compute pass (a pass orders its
  dispatches and makes an earlier write visible to a later read — that is the
  same guarantee the boundary was bought for). A NEW PER-FRAME KERNEL JOINS
  THAT PASS IN DEPENDENCY ORDER; A NEW PASS IS A RULING, NOT A DEFAULT.
  What is still open, both Jean's, both priced: (1) placement + cull, adjacent
  and fusable — the cull reads the STORAGE face (`fc_vp`, g2:240, alias of
  `vp_data`) so no frame-R sync copy stands between them — but the boundary it
  removes exists only on placement-dirty frames and the fusion costs
  `meter_row::PlacementCorrection` permanently. (2) The SIX passes between the
  witness copies and placement — zone sync, zone evolve, pawn aura, and the
  four orb passes — sit in one uninterrupted run separated only by queue
  writes, which are not pass boundaries. That is the larger fusion, up to eight
  passes into one, and every one of them retires a meter row. Both trade
  measurement resolution for boundaries, one round before the meter round.
  Origin: SPINE_2 B. Unblocked by the meter's own numbers, or by Jean ruling
  the rows expendable.
- `update_player_agent` IS THE LONGEST LANE. The single-thread step's loop
  structure — the ground resolve taps, the zone / cube / agent contact loops —
  is the next thing to read if a measurement asks for the serial prefix. A
  workgroup-per-agent form is the priced answer, and it is not a trim: it is a
  rewrite of the kernel's shape. Origin: SPINE_2. Unblocked by a meter reading
  that names this lane.
- THE DOOR TEACHES; THE WORLD STILL DOES NOT. ANSWERED, NOT CLOSED. HINT_0
  deleted the `#hint` overlay and ATTIC_ATRIUM D1 deleted the controls poster,
  leaving no statement anywhere of what the keys, the mouse or the doors do.
  The remedy's first half has landed: ENTRANCE_0 gave the entrance its own
  `assets/entrance/`, put the controls diagram on the veil, and E3/E5 freed it
  from the 2:3 slot so it arrives whole. PHASE M reads it on the Pixel — AURA,
  ZOOM, SWAP, DIRECTION, ROTATION all legible at 263 CSS px, neither too small
  nor soft. (That reading was the FLOOR only, and "no cap moved" beside it was
  a claim about a range nobody had read: E6 read the ceiling, and it failed —
  the caps are POSTER_WIDTH now, 640 on both axes, and landscape gets a row.
  The floor reading itself stands to the decimal.) The visitor is therefore
  shown the gestures
  immediately before entering, and the veil is dismissed by a dedicated
  "tap to enter" button rather than by performing one (E3/E4 addendum R4,
  settled). WHAT REMAINS OPEN is the world's own half: once inside there is
  still no teaching, and the portal and current-image images of the named
  remedy have not shipped. Origin: ATTIC_ATRIUM D1, answered at ENTRANCE_0 E5.
  The deploy hold was written against the loading page landing and that
  condition is now met on the door — whether it lifts is Jean's, and
  ATTIC_ATRIUM must be in the same `dist` when it does.
- THE CARD SPILLS ITS COLUMN BETWEEN 400 AND 600 CSS PX OF HEIGHT, and
  ENTRANCE_0 E6 deepened it. `.layer` is a fixed-height centred column with no
  overflow rule, so an over-tall column has always spilled at both ends — the
  poster loses its top, `details` falls off the bottom. E6's 60vh cap sizes the
  poster without knowing the card's stack is 280px where the veil's is 138, so
  the band got worse before the row rescues it below 400: 915x412 went 41 -> 115
  px spilled, 1280x401 48 -> 120, 1280x500 0 -> 80, while 915x384 and 1280x399
  went 57 -> 0 and 49 -> 0. THE VEIL NEVER SPILLS, at any of E6's 26 readings.
  Two cures, both dimensions and so both Jean's: a card-only height cap, or the
  row threshold moved to the card's own arithmetic (229.6 + 280.2 + 32 = 542),
  which costs the veil a row between 400 and 542 where its column still works.
  Origin: ENTRANCE_0 E6. Unblocked by Jean picking one, or by ruling that a
  failure card in a short window may spill.
- THE HARNESS SELECTS ITSELF FROM ITSELF. `src/the_board.cpp` is a "Render
  Cartridge Development Harness" for a cartridge set of one, chosen by
  `T7_RENDER_CARTRIDGE`, a dial with one legal value — and after NAME_0 the
  harness, the dial's only value and the program all spell `the_board`. Whether
  the harness and the selector survive as expressible concepts, or whether the
  program is always the whole program, is the question. NAME_0 asked that this
  be filed beside the entry DEMO removal already holds open on piece-presence,
  to be answered together or not at all — THAT ENTRY IS NOT IN THIS FILE. No
  `INCUBATE`, no "whole program", no piece-presence row: the register does not
  carry it, so this one stands alone and carries the pairing instruction
  instead. Origin: NAME_0, filed not acted on. Unblocked by the piece-presence
  ruling, whenever it is written down.
- THE WORDMARK IS `the_board`, LOWERCASE AND SNAKE, on the veil and on the
  fallback card, and `<title>the_board</title>` with it. Whether it becomes
  "The Board" is a typographic decision on the face of the piece, not a rename,
  and NAME_0's orthography does not decide it: prose is "The Board", artifacts
  are `the_board`, and a wordmark is arguably both. JEAN'S STAMP.
  Recommendation, carried from NAME_0: leave it — the machine spelling is the
  tone. Origin: NAME_0.
- ONE NAME_0 ROW COULD NOT BE CLASSIFIED. `tools/command_census.py` writes the
  ledger sentence "The frame's one submit rides the pawn's render tick". Read
  as the harness it is a program row and should now say `the_board`; read as
  the figure it is the frozen entity noun. The entity has no render tick of its
  own, which argues program — but that is an argument, not a classification, so
  the row was logged and left. It reaches `audit/COMMAND_LEDGER.md`, so
  whichever way it is ruled the fix is one string in the tool and one
  regeneration. Origin: NAME_0 recon.
- NAME_1 IS PARKED ON JEAN'S HAND. CC cannot rename a GitHub repository. Until
  `jeanklein1/7T-Pawns` becomes `jeanklein1/7T-Board`, four R rows stand
  deliberately unchanged: the CONTACT link in `web/index.html`
  (`…/7T-Pawns/issues`), the Pages URL printed by `tools/web_dist.py`
  (`jeanklein1.github.io/7T-Pawns/`), the section comment in `.gitignore`, and
  the git remote itself. GitHub keeps a redirect at the moment of the rename,
  so nothing breaks when it happens; the sweep is `git remote set-url origin`,
  one `git fetch` to verify, then those three files. Origin: NAME_0 / NAME_1.
  Unblocked by Jean stamping the repository name.
- MOOD_ATRIUM STILL EXISTS, AND ONLY BECAUSE A PRESET REMEMBERS ITS ID.
  ATTIC_ATRIUM deleted the entrance's every mechanism — the hang, the
  partition, the arc, the arrival orbit, the colours — and stopped at the ROW.
  R1's gate: `organ_panel.js` exports definitions keyed `"<mood_id>/<param>"`
  into a file the operator keeps, and import reads it back, so a preset saved
  before the deletion carries id 6. Dropping MOOD_COUNT to 6 would have those
  keys land somewhere. `mood_def` now REFUSES an out-of-range id loudly
  instead of aliasing it to mood 0 (D6a), which is the cure — what is left is
  a ruling. Boot no longer goes there: `?mood=6` is the only way in, and what
  it opens is a room with no poster, no arc, no composed camera and the flat
  room's palette. Origin: ATTIC_ATRIUM D6, halted at its own gate. Unblocked
  by Jean ruling whether old presets are migrated (rewrite the key on import,
  or version the blob) or simply re-exported.
- NO VERSION KEY ON EXHIBITION ASSETS. `dist/_headers` marks only `/` and
  `/index.html` `no-cache`, and neither `exhibition.json` nor any painting,
  track carries a `?v=` — so a returning visitor may hold a stale manifest,
  and a REPLACED painting may be served from cache on a redeploy. Jean's
  cure today is a hard
  reload. The real one is `BUILD_ID_PLACEHOLDER`'s idiom extended to the
  exhibition — the wasm's own hash already names every deploy — which is its
  own campaign, not a line. Origin: ATRIUM_10, flagged; it outlived the
  entrance (ATTIC_ATRIUM) because it was never the entrance's. Unblocked by that
  campaign, or by Jean accepting the hard reload as the standing answer.
- The doorway's shell: CLOSED for agents, OPEN for the ribbon. An arch is two
  leg sources half_span apart in the field, and at the social slack (3.0) their
  shells meet across every opening — a barrier with its crest in front of the
  door. `FIELD_ARCH_SLACK` (1.25) answers it in the GPU dialect
  (`world.wgsl` `field_sum`'s occupier_amg loop). The ribbon head's CPU sum
  (`bodies/ribbon.hpp`) is a separate dialect with its own per-family occupier
  dials and still wears the social slack over arch legs — untouched, and it
  will read the same closure the moment a ribbon aims at a doorway. Origin:
  ATRIUM_7 A7.2. Unblocked by a ribbon that meets an arch, or by Jean ruling
  the two dialects into one.
- Doorways flush with walls: the rooms' wall doors stand with their SPAN
  perpendicular to the wall they sit on, so a visitor meets the arch edge-on
  rather than through its opening. An arch's rotation is its span
  (`arch_rotation_from_facing`, contracts/spawn_services.hpp); the two wall
  candidate tables in `direction/mood.hpp` carry authored rotations and would
  each turn a quarter — a one-table change, no new mechanism. Origin:
  ATRIUM_5's reading of `amg_gen_shell`, gated as it stands by Jean at
  ATRIUM_6. Jean's word opens it.
- Sphere repulsion excluded from the witness's presence (motor-sovereign):
  a sphere subscribes to no behavior force and no presence push — it EMITS
  the point's push (`row_sphere_push`) and has never been a subscriber to
  one — so the eye sheds cubes and leaves spheres alone. Origin: KITE_1 C6.
  Jean's word re-opens it.
- The witness's carve follows the PHOTOGRAPHER's eye inside the snapshot
  pass: `patch_terrain_vs` draws there through `framePhotographerGroup_`,
  whose `frame_r.camera` is the photographer's own, so a gallery painting is
  carved by the lens that took it rather than by the player's. Ridden
  deliberately — no distinguisher exists in the shader, and minting one
  would either move a byte-pinned FrameR offset or cost a pipeline variant.
  Origin: KITE_1 C4. Jean's word gates it off.
- The witness carve's height scale is `ZONE_SUPPRESS_OUTER` used as the
  vertical half of the same reach, because the tree has NO cell-lift
  ceiling to be the scale instead: `config.indoor_height_cap` is indoor-only
  with 0 as its disable sentinel, and `GOL_HEIGHT_FACTOR_MAX` is a per-cell
  multiplier. Origin: KITE_1 C4 (RECON F4). Unblocked by a ruling on what
  the ceiling should be, or by leaving the reach as the answer.
- PIPE_0: PARKED per Jean's 2026-08-07 directive. Origin sha of
  docs/HANDOFFS/WEB/PIPE_0_DECISION.md (path retired to git; content at its last sha).
- STREAM_0: PARKED per Jean's 2026-08-07 directive. Origin sha of
  docs/HANDOFFS/WEB/STREAM_0_DECISION.md (path retired to git; content at its last sha).
- DAWN_REFERENCE web-era rewrite: docs/reference/DAWN_REFERENCE.md is
  archival-with-named-drift (its own CANON stamp); the rewrite awaits Jean's
  reference round. Origin: the stamp itself.
- GUARD DEBT in realization/renderer.hpp: four __EMSCRIPTEN__ guards survive
  SUNSET_1 (census `git grep -n "__EMSCRIPTEN__" -- src`). Coverage is
  SPLIT, not blind: console_gate defines __EMSCRIPTEN__ (fidelity with emcc)
  and so type-checks the shipping arms; glaw1 does not, and so type-checks
  the dead ones. The sitting decides two things: collapse the guards
  (SUNSET_1's stated intent; may cost stub declarations), and whether
  glaw1's macro set should match the build's. Origin: RECENSION_1 FLAG-7,
  corrected by RECENSION_4 when the gates were first read whole. Unblocked
  by the gates sitting.
- L26 docket — fields marked dead, awaiting the sitting that next opens their
  struct: `OrbConfig`'s driverless gen-1 block, `Instruments.watcher_ticks` (driver
  went with the FileWatcher at SUNSET_1; `the_board.cpp:60` still names the dial),
  `OrbMoodConfig.base_hue` / `.hue_variance` (dead BY CONSTRUCTION — every
  ORB_PALETTES row carries count ≥ 1, so `pack_palette_` never leaves
  `palette_count` at 0 and the kernel's legacy single-hue arm is
  unreachable; ORGAN_4 P1b), and `OrbMoodConfig.motion_rule` (dead TREE-WIDE
  — `configure_orbs` writes the player's `os.current_motion_rule` and never
  reads the config field; ORGAN_4 C2). The three orb fields stay put rather
  than dying now because `ORB_MOOD_TABLE` is positionally brace-initialised
  (D3: the braces outrank the broom), and the twin rooms — table and struct
  — must move in one commit. Each dies in the commit that reopens its struct
  (L26, L3: twin rooms, one commit). Origin: L26, moved here by RECENSION_2;
  grown by ORGAN_4 P1b. Unblocked by any campaign that relayouts one of the
  three structs named.
- Dead boot write, awaiting the boot block's next sitting: state.hpp's
  `config_.aura_enabled = 1.0f` boot seed is overwritten within one frame by
  tick_pawn_couplings (the driven window's only runtime author since
  ORGAN_2a). Origin: ORGAN_2a D3. Dies in the commit that next opens the
  boot-config block.
- CENSUS_1b, the exhaustive walk of the four realization giants (state.hpp,
  world.wgsl, renderer.hpp, cartridge.hpp) and the ten bodies/** files, line
  by line. CENSUS_1 covered them by sweep and declared that edge; the refuter
  breached it immediately (gol_zones.hpp's "Upload all 7 slots" against a
  5-slot stride). Origin: PROCESS_LAWS SCHEDULING RECORD, moved by
  RECENSION_3 when its dated owner (the control-panel campaign) ran without
  collecting it. Unblocked by any campaign that enumerates those files.
- the_board.cpp IS UNGATED: no gate compiles the TU that owns main(), the rAF
  driver and the boot sequence — glaw1 compiles tu.cpp, console_gate compiles
  cartridge.hpp and console.hpp. One missing declaration is the whole reason:
  tools/gates/console_gate/stubs/emscripten.h stubs
  emscripten_set_main_loop_arg but not emscripten_set_main_loop, the form
  the_board.cpp calls. CC verified everything else in the TU compiles clean under
  the existing gate flags. Cure is one stub line; adding to the pinned stub
  set is a gates decision. Origin: RECENSION_2 FLAG-U5c. Unblocked by the
  gates sitting.
- FIREFOX STAGING RATCHET (bounded; Firefox-side; the cheap cure landed, its
  witness owed): Firefox on Windows compiles the module with no diagnostics,
  boots, runs 30–56 fps on the Kepler, and loses the device with
  `[Device] LOST reason=1 : Out of memory` after several world transitions —
  rapid keyed transitions reach it in under a minute, a calm session in many
  minutes or never. about:memory, GPU process, across ONE keyed transition:
  gpu-committed 1003 → 1308 MiB; private write-combined commit 858 → 1089 MiB
  in 37 → 43 segments; a second transition +15 MiB, 46 segments; flat for
  minutes on either side. The program's own GPU budget is 251 MiB throughout
  and creates nothing after boot. Reading: Firefox's wgpu serves
  WriteBuffer/WriteTexture from upload-heap blocks it sub-allocates; a
  transition's upload burst — the paintings re-staged at teardown, 1 MiB
  WriteTextures landing several to a frame — opens blocks the per-frame trickle
  then keeps from emptying. Chrome's Dawn and WebKit recycle differently: an
  iPad (Safari 26) ran 10+ min with transitions. ORGAN_8 P3 paced the paintings
  to one lane; OVERTURE_0 R-E RAISED IT TO FOUR (`AUTHORED_FETCH_INFLIGHT_CAP
  = 4`), sized for the boot rather than for a browser that cannot run the
  piece. The about:memory before/after pair is therefore MOOT while Firefox is
  HELD at the fallback card, and it is re-owed — against four lanes, not one —
  the day Firefox returns. If the step survives:
  the structural cure is that the program owns its staging — a fixed ring of
  MAP_WRITE buffers made at boot, mapped, filled, unmapped and copied from, so
  the browser allocates no staging at all — priced at one round over
  GPUState's upload doors, not built. Origin: COMPAT_1's witness runs,
  2026-08-21.
- In-place same-shape atmosphere transition (sunset → night without a
  teardown, the sky re-drawn over a standing world): priced at ATMOS_1,
  not built — the spawns' population row is per mood, so a "same shape"
  transition is not yet a no-op below the sky. Origin: ATMOS_1 §7 — the
  campaign's handoff, held by Jean rather than filed in the tree; the
  campaign opens at 71619c0. Unblocked by a visual need (the recording's
  long takes) or the indoor atmospheres' rework.
- The regime's second subscriber owes the flag (REGIME_1): the four weight
  rows raise the MOOD definition flag only, so a weight edit re-rolls and
  re-speaks the sky and nothing else. The first family to carry a regime
  column (the orb mood bank is eligible today) must make the weight rows
  raise its definition flag too — a per-field mask or a "regime law"
  hook at the boundary — or a regime change leaves it behind. Origin:
  REGIME_1 §0.5.
- A per-parameter "mood-wide" flag for the regimes (ATMOS_2 §0.5): today a
  parameter wanted the same in every regime is set equal in every regime,
  four dials that agree. The flag would make it one dial and restore
  "one fact, one home" at the tuning level; it costs a selector row per
  parameter and a draw that reads regime 0 when the flag is set. Also
  priced, not built: a second independent roll (axes), should the
  combinatorics of independent light and fog be wanted back; and moving
  the sun's bearing into the regime row. LENS_1's ALL position writes
  every regime at a stroke, which is the gesture form of the same relief;
  the flag remains the one-home form. Unblocked by Jean's tuning asking
  for any of them. Origin: ATMOS_2 §0.5.
- naga is installed per session in CC's container with `cargo install
  naga-cli` (minutes), and `tools/wgsl_gate.py` then runs in-container on the
  raw module. Origin: ATMOS_1 report FLAG 12, answered at COMPAT_1.

## PERFORMANCE — the one home (three campaigns, one register)

PANORAMA_0, PANORAMA_1 and WRAP_0 ran as one arc and are recorded as one
section; their handoffs die at this close (L31). The next session starts from
this and the tree.

### LANDED

Compute lanes (`update_cube` 64×4 threads; `update_other_agents` one lane per
field subscriber). `SPAWN_QUEUE_MAX = PopFamily::COUNT` with the queue drained
per patch — the old bound overflowed at every boot and portal and dropped the
tail of PLACEMENT_ORDER, galleries first. PCF early-outs at the caller
(bit-identical: `ndotl == 0` already zeroed the product). The column ceiling
gate — the re-raise fired on every corrected frame outdoors for a rebake that
could not change a byte. The settle: one regeneration per 0.133 s outside a
world's birth, `world_young` the birth bypass, stamped at first sight. The
mesh-gen firing counter. `draw_mask` / `shadow_mask` / `shadow_pcf_taps`
(config 704 → 720). The photograph at LOD1. `?pace=1|2`, armed from inside the
loop. The refresh pinned to the 5th percentile. The window's self-description
and the terrain slot line. The deploy scan. And `src/the_board.cpp` under the
TU gate, which nothing had ever compiled.

### MEASURED (laptop Kepler, meter build)

Walking, first world: envelope 17.95 ms, from 20.4. Rides 19.5–21.5. Compute
1.3 flat. Firings 38–130 a window, from ~500. Photograph max 2.5–12, from
16–55. **Pixel: not captured since the compute lanes landed — owed, and it is
the floor device that decides what comes next.**

### FINDING — the main pass is geometry, not fill

Canvas 689×607 → 1366×607 (+98% pixels) moved `main_pass` 11.86 → 13.6 ms over
16 windows (+15%); `shadow_pass` 6.13 → 5.82 on a fixed-size target, the
control. Fragment is ≈ 1.6 of 12 ms; ~10 ms is vertex and submission. Patches
are already frustum-culled through the draw plan. Levers, in aesthetic-price
order: `patch_terrain_vs`'s per-vertex cost and its dependent fetches;
curtain granularity (a zone overlap promotes a whole patch from cap-only to
the full IB, and the eight-zone world ran ~3.5 ms hotter than the one-zone
world); the LOD0 radius and `PATCH_MESH_N`, which are Jean's. (The
heightfield-resolution lever that stood here is SPENT — LATTICE_1 R1 made it
law: `PATCH_HEIGHTFIELD_N = PATCH_MESH_N + 1`.)

### FINDINGS, mechanical

- The PRESENT estimator had ADAPTED to the judder and hidden it — an EWMA over
  frame times cannot tell a slow display from a slow world. Fixed at WRAP_0 U1;
  the period is now the 5th percentile of 300 deltas and may only decrease.
- `?pace=` was INERT: the timing call sat after `emscripten_set_main_loop`,
  which never returns. Fixed at WRAP_0 U2.
- GPU SAMPLING COLLAPSES UNDER BACK-PRESSURE, and the fix as specified does not
  reach its own witness. The meter allows one readback in flight, so the sample
  rate IS the map-completion rate: 126–235 sampled frames of ~1,300 at 689×607
  (1 per 5.5–10 frames) against 3–13 at 1366×607 (1 per 100–433). A 3-deep ring
  divides that by three — 1 per 33–144 — where the witness wants 1 per ≤ 11. The
  depth that would reach it is 9–39, which says the ring is not the whole
  answer and MAP-COMPLETION LATENCY is. Unblocked by measuring that latency
  directly (stamp at MapAsync, stamp in the callback, mean per window) — an
  instrument that does not exist. The ring was attempted and reverted: the
  binding gate's RESOURCES scanner reads a scalar buffer declaration and not an
  array, so even the partial fix needs either three named members or a change to
  the gate's parser.
- At full canvas the CPU blocks 13.7 ms in `finish_submit` — GPU back-pressure,
  the saturation signature, and the same fact the sampling collapse reports.

### LATTICE_1 — landed, and what it filed

Origin: LATTICE_1 (five commits on master, base `06faef85`). The heightfield is
the lattice: `PATCH_HEIGHTFIELD_N = PATCH_MESH_N + 1`, one texel per mesh
vertex. The bake is one fused kernel over the frame's whole batch, reading its
params from a read-only storage array indexed by `workgroup_id.z`; the 512 KB
scratch buffer, the second pass, the dynamic uniform seat and its 256-byte ring
are all retired. Both terrain VSes `textureLoad` at the lattice texel instead of
bilinear-sampling toward it. And the bake derives each lattice node ONCE per
16×16 tile into workgroup memory rather than once per (tap, node) pair.

NOT MEASURED. The arithmetic says ~3× fewer ground evaluations, 15.5× less
heightfield memory, and ~20× less derivation work; no meter reading confirms
any of it. The campaign's meter is its bookend, run once before LATTICE_1 and
once after LATTICE_4 — so LATTICE_1's own numbers are predictions until then.
Jean's witnesses W1–W5 (the ~7.6 MB budget line, the patch-border seam in
raking light, the fine band's shading, the ribbon hitch at full throttle, the
pawn on a pyramid face) are what stand between the arithmetic and belief.

- WALKER TILT NOW READS THE LATTICE SURFACE. `query_ground_walker_pair`'s base
  is `sample_terrain_y_at`, which samples the baked heightfield — so
  `terrain_normal_at`'s 0.5-wu finite difference and `slope_passable` now see
  bilinear between lattice points 0.78 wu apart where they used to see a
  0.196-wu texel grid. A 0.5-wu difference straddles at most one lattice cell
  now, so it reads a plane rather than sub-texel jitter.
  `PAWN_SLOPE_NOISE_FLOOR` exists to reject exactly that jitter ("under it, the
  tilt query's own finite-diff jitter dominates dh") and may be dead after
  LATTICE_1. Not touched; witness first. Origin: LATTICE_1, filed not acted on.
  Unblocked by Jean walking a slope and reporting whether the tilt reads calmer.
- THE WORLD CAN RE-BAKE IN A FRAME. With the batch shape, marking all 225
  patches `NEEDS_REGEN` is one dispatch, not 225 pass pairs. That makes the
  terrain design constants — `TERRAIN_BANDS`, `BAKE_STENCIL_EPS`, the tile
  modifiers — ORGAN-eligible as definition-mode dials that trigger a full
  regen. Filed for ORGAN, not done here. Origin: LATTICE_1. Unblocked by an
  ORGAN round that wants them.
- `compute_entity_placement` AND `zone_derive_params` REMAIN `@workgroup_size(1)`
  LOOP KERNELS. PANORAMA_0 measured both trivial, so the aphorism ("an
  invocation is one output") is not enforced on them. Recorded so the next
  reader does not rediscover them as a finding. Origin: PANORAMA_0, restated at
  LATTICE_1.
- `BAKE_STENCIL_EPS` IS A DIAL NOBODY HAS TURNED. It is `PATCH_EXTENT / 255` —
  the old heightfield's texel step, kept so the gradients carry the same
  smoothing they always did. Larger softens the fine band's shading, smaller
  sharpens it. If W3 reads the ripple as harsher, this is the number to move,
  and it should be moved with a reading beside it, not blind.

### LATTICE_2 / LATTICE_3 — landed, and what they filed

Origin: LATTICE_2 + LATTICE_3 (seven commits on master, base `d1f98eb3`).
The five mesh-gen kernels stopped being single-threaded: one workgroup per
slot, 64 lanes, outermost loop strided by lane, inner loops verbatim. And
the three patch index buffers became uint16.

NOT MEASURED, same as LATTICE_1. The five kernels' output is byte-identical
BY CONSTRUCTION and that much is checked — both address-generation forms
were simulated against each other over 38,781 parameter configurations in
all (arch 4,704, column 1,977, palm 17,424, cactus 12,960, blade 1,716),
comparing full address → value maps and asserting no address is written
twice. What is NOT checked is that it is FASTER: no meter reading exists,
and the campaign's meter is still its bookend.

- MESH GEN'S INNER LOOPS ARE SERIAL PER LANE. The stride law strides the
  OUTERMOST loop only, which takes each kernel from one lane to tens. The
  ideal — "an invocation is one output" — would stride the inner loops
  too, and is the horizon, not the landed state. Two things stand in its
  way and both are real: the cactus arm's `apx/apy/apz` path walk is
  loop-carried down the arm (R3), and the column's per-disc emission is a
  fan-or-strip whose base is a prefix over earlier discs. Filed for a
  measurement that asks; not done. Origin: LATTICE_2 R4.
- `MESHGEN_LANES` IS 64 FOR ALL FIVE, and nothing has compiled them under
  Tint/DXC yet. The column carries ~200 floats of private arrays per lane
  (five 32-entry profile arrays, seven 12-entry disc arrays, two 29-entry
  trig tables); at 64 lanes that is 64× the serial kernel's register
  pressure. naga validates the module, but naga does not allocate
  registers. If Jean's build (G-LAW 1) refuses one of them, the handoff's
  ruling is a per-kernel override to 32 lanes for that kernel only —
  a one-line change, and this entry is the record that it was foreseen.
  Origin: LATTICE_2 R-D. Unblocked by a build.
- THE WORKGROUP-STORAGE FLOOR IS A LITERAL IN TWO HOMES. The schema's
  NEEDS row says 4912 (LATTICE_4 moved it from the bake's 3,744 to the
  card's sum, which is larger) and the module's `CARD_WORKGROUP_BYTES`
  computes it; witness R-3 resolves only `Dim::<SYM>` sources against
  state.hpp, so it cannot hold this one. The module's const_asserts guard
  the ceiling (16,384) and hold the card's sum at or above the bake's, so
  the row always quotes the larger of the two — but nothing guards the
  schema literal against the module's value drifting apart. Closing it
  means teaching R-3 a `world.wgsl <CONST>` source form — an instrument
  commit. THIS ROUND MOVED THE NUMBER BY HAND, which is the gap doing
  exactly what the entry says it does. Origin: LATTICE_2 R5, re-paid at
  LATTICE_4 R6. Unblocked by someone wanting the instrument.

### LATTICE_4 — landed, and what it filed

Origin: LATTICE_4 (three commits on master, base `d7bde104`). The card
writer fused to one kernel and one dispatch; its band sum reads a per-tile
node table; the scratch buffer, one pipeline and one dispatch retired. The
fade overlay's gate moved onto the value the shader reads, at the exact
quantization bound. NOT MEASURED — the campaign's meter is still its
bookend.

- THE CARD'S WINDOW WAS NOT SHRUNK, and the reason is a correction of
  record. LATTICE_4 R1 proposed 640/1000 → 528/825 on the yardstick
  "the farthest reader is 403.125 wu, the allocation window plus the
  snap". That is where an entity's ANCHOR can be. Every entity VS samples
  the card at its VERTEX position, and `arch_vs` has no ring gate — so an
  arch at the window's edge reads out to anchor + half_span + half_depth.
  With `cpu_sample_gaussian` clamping z to ±3, MONUMENTAL reaches 53.00 wu
  and STANDARD 51.95, making the farthest card read 456.125 wu. R1's
  window guarantees 409.375. Held at 640/1000, which guarantees 496.875
  (slack 40.75). THE COVENANT IN state.hpp NOW CARRIES THIS: shrink the
  card against 456.125, never against 403.125.
  A shrink is still available on the true yardstick — SIZE must satisfy
  EXTENT = 1.5625·SIZE and SIZE % 16 == 0, so 608/950 guarantees 471.875
  (slack 15.75, texels 409,600 → 369,664) and 592/925 guarantees 459.375
  (slack 0.75, too tight to want). That is a dial with a visual
  consequence if it is ever wrong, so it is Jean's, not a follow-up.
- `true_band_delta_contribution` IS KERNEL-LOCAL BY CONSTRUCTION. It reads
  `card_nodes` / `card_origin`, workgroup storage only `write_live_card`
  fills, and must run after that kernel's table barrier. naga cannot check
  it; its SINGLE CALLER is the guard. A second caller must restore the
  derive path — `derive_wave_node(node, lattice_node_seed(seed, node,
  band_idx), band)` is exactly what the table holds — and give it its own
  home. Its banner says so.
- `shadow_pcf_taps` SITS IN THE "Measure" GROUP, not a lighting one.
  LATTICE_4 R5 asked for it in "Lighting · Sun"; the dial already existed
  (PANORAMA_1 U4) beside `draw_mask` and `shadow_mask`, the three
  subtraction dials that were born together, and no "Lighting · Sun" group
  exists. Moving it would split that trio to create a one-row group. Panel
  layout is Jean's gate. Origin: LATTICE_4 R5, flagged not acted on.

### BUNDLE_1 — landed, and what it filed

Origin: BUNDLE_1 (five commits on master, base `c1ad23de`). The frame's
draw list is spoken once: a draw ledger of eleven indirect records, the
draw verbs and the drawable table templated over the encoder type, and the
main pass's opaque list and the sun shadow pass's each recorded into a
render bundle executed with one call. NOT MEASURED — no reading exists,
and this round's whole claim is a CPU one, so the browser's own Performance
track is the witness of record (LATTICE_4 §5's last row).

- A DRAWABLE IS A ROW AND A RECORD. Adding one is one `DRAWABLES` row and,
  if its count can move, one `DrawRecord` staged in `stage_draw_ledger`.
  There is no third site. If its count CANNOT move it stays a literal in
  the bundle — and if it needs a non-zero `firstInstance` it must stay a
  literal, because core WebGPU forbids one in an indirect draw without the
  `indirect-first-instance` feature and the wallet does not request it.
  The monolith is the standing example.
- BUNDLES CAPTURE OBJECTS, NOT VALUES. A bundle holds the bind groups and
  buffers it was recorded with, so a re-record is needed when one of those
  OBJECTS is recreated. R-B found ZERO post-boot recreation sites today
  (`galleryTexturesGroup_` is the one rebuildable group and only boot calls
  its rebuild), so `bundlesDirty_` fires at boot and on a mask turn and
  never otherwise. A NEW recreation site must raise it. Nothing enforces
  that — it is not a type error, not a validation error, and not a wrong
  pixel, it is a stale draw list — so it is written down here.
- AN ENCODER-TIME SKIP CANNOT LIVE IN A BUNDLE. Every `if (count == 0)
  return;` in a draw verb was one, and a bundle recorded while a family was
  empty would omit it forever. They are records now. The rule for the next
  campaign: a guard that decides WHETHER to draw belongs in the number, not
  in the verb — unless it is a dial, in which case it belongs in the verb
  AND its setter raises `bundlesDirty_`.
- THE INDOOR ATLAS IS DIRECT BY NECESSITY. Viewport, scissor and the
  per-light group-1 rebind are pass-encoder-only; a bundle can carry none
  of them. A WebGPU that let bundles carry a viewport would let the atlas
  bundle too — priced at one bundle per tile, which buys nothing today
  because the tile loop is two lights, not twenty.
- THE BUNDLE/PASS FORMAT EQUALITY IS TEXTUAL. C-8a/C-8b hold that every
  bundle states its three format facts and that its colour declaration is
  self-consistent; they do NOT prove the formats equal the executing pass's,
  because a pass names TextureViews whose formats come from the swapchain
  and console.hpp. Dawn will reject a real mismatch at ExecuteBundles —
  which means the first witness of a drift is a boot failure on a device,
  not a gate. Closing it means teaching the census the swapchain's format.

### NEXT — TERRAIN_0

Opens on two readings and is not written until they exist: the three terrain
mask bits, one window each, both devices; and the slot line. Then the levers
above in order. PRESENT_0's remainder — the governor (PANORAMA_1 U6b) — after
Jean's eye at `?pace=2`, now that the pace is real and the histogram honest.

### PRICED, NOT BUILT

F3 proper (per-slot mesh regeneration; workgroup-per-mesh) — the settle may
have emptied it, so re-read the Pixel's firing count first. E-2, the
static/dynamic shadow split — a design round, after the `shadow_mask` reading.
D-2, the photographer's own cull window and the two-frame composite. C, the
runway (bake by rows) — likely unnecessary after the heightfield lever. I, the
live card by halves. F, the cell-colour bake — cold until the analyser socket
binds (`12 sources unbound`), so its cost has never been on screen. BOOT_0:
async pipelines with a first-frame set, the warm-up pass, the compressed
bundle. Render scale on the ribbon (taste).

### OWED, Jean's

The mask table (U3 makes each window self-describing). A Pixel console, walking
and riding. The pace by eye at `?pace=1` and `?pace=2`. A shadow edge at 16 and
4 taps. Two photographs, LOD0 and LOD1. The ring's edge while riding, for the
settle. And OVERTURE_0's own visual gate at spawn, never recorded.

### CORRECTIONS OF RECORD (this session)

The cohort order — `gallery.hpp` precedes `patch_system.hpp`, not the reverse.
Eviction precedes the budgeted spawn in `stream_patches`. The plan slots are A
full-IB / B cap-only / C LOD1, and the mask bits follow the code. F5's guard
belongs at `calc_directional_light`, where `ndotl == 0` already zeroes the
product, not inside the PCF. F14's READY clock was armed at the first frame —
the early fire was a slow exhibition, not a bug. R-F's reach test is the open
world's only: `MIN_FROM_ORIGIN` is a floor, and a room's far wall stands at
142–192 wu, so a bare reach test would force a second door into a correctly
doored room. `update_cube` makes two `manifold_position` calls, not three. And
the arithmetic that read the main pass as fill-bound, refuted by the pass
itself at two canvas sizes.

## OVERTURE_0 — the first seconds; what the campaign priced and did not build

Origin: OVERTURE_0 (eleven commits on master, base `c17569ae`). The boot world
was born before its exhibition arrived and nothing let a born patch be dressed
later; the deferred hang, the pool resolution and the flag split landed. What
follows is what the campaign measured and left.

- GALLERY BIOGRAPHY IS FROZEN AT `581693e3`. Two edits moved it: OVERTURE_0's
  U1/U2 (the outdoor proportions, and the roll resolving against the pool),
  and the fifty-fifty ruling that followed. `?seed=N` no longer reproduces
  galleries pinned before the sha above — which is the LATEST freeze, and the
  only one that matters, since anything pinned before it is before both. The
  PORTAL_3 E1/E2 precedent: the number is recorded here and not in a comment
  (L34).
- PER-EVICTION REFETCH, priced not built: a released, hung record is refetched
  from the manifest cursor, so a two-hour walk sees all 57 paintings rather
  than the staged 32. U6 released the record; nothing yet re-asks for a new
  picture inside one world. Unblocked by the recording asking for it.
- ROOM RE-HANG ON A LATE EXHIBITION, priced not built: `place_wall_paintings`
  is idempotent (clear, then hang from the seed), so a room deferred bit is
  the same shape as U3's patch bit. Reachable only from an indoor `?mood=`
  boot, which is a dev door — every other room is entered through a portal,
  thousands of conductor frames after the fill. Unblocked by a measurement.
- DENSITY: `MIN_GALLERY_DISTANCE` 110 -> exclusion disc pi*110^2 ~= 38,000 wu^2
  ~= 15 patches; ~= 2.5 galleries inside LOD0 (175), ~= 9.7 inside the veil ring
  (342), ~= 29 of 40 exhibition layers at `PAINTINGS_MEAN` 3. The layer floor:
  40/3 ~= 13.3 galleries in the ring -> exclusion >= sqrt(pi*342^2/(13.3*pi)) ~= 94
  wu. So 95 buys 1.34x density and 80 overdraws the exhibition. Held as a taste
  gate, not ruled. Unblocked by Jean's eye after this campaign's witness.
- THE RESERVATION GAP: place reserves STAGING layers, not EXHIBITION layers.
  Above ~13 dressed galleries `find_free_exhibition_layer` fails at commit and
  a fan ends short. Inert at `MIN_GALLERY_DISTANCE` 110; binding below ~94, so
  it is the density dial's companion and must move with it.
- U9's SIX-PAINTING FLOOR IS SUPERSEDED by AUBADE U4/U5c — see the AUBADE
  stanza above, which is its one home. The gallery-less-build item that sat
  here dies with it: the veil no longer waits on the floor, so the 5 s costs
  the visitor nothing.

## PORTAL_3 — the even field (E1/E2 landed at `ee90a85`, E3 open)

- E3 DEFERRED, AWAITING ONE NUMBER — WHICH IS NOW PRINTABLE. The gate is the
  arch row `live` count on `[CENSUS ... trigger=born]` in an open world. It has
  to be the BORN dump, not the boot one: `boot` prints from `initialize()`,
  before the first `stream_patches`, so its arch and gall rows read zero by
  construction and the number this ruling wants has never been on screen.
  OVERTURE_0 U8 added the born census at the end of the fullRegen arm, where
  the priority window has been spawned, generated and doored. `ArchConfig::
  SPAWN_CHANCE` stands at `0.050f`; the ruling is unchanged: `<= 8` -> `0.100f`;
  `9-12` -> `0.075f`; `>= 13` -> no edit, the array is the constraint and
  capacity is a separate campaign. Unblocked by the census reading.
- MORE ARCHES IN GENERAL IS WANTED (Jean, Aug 2026). E3 raises every arch
  tier, not DOORWAY alone. That side-effect is a BENEFIT, not a cost — the
  taste gate ARCH_2's "buy portals with the chance" implies is ANSWERED. Do
  not re-open it, and do not price the big arches as a loss.
- COUNT BELONGS IN `SPAWN_CHANCE`, NOT IN `THEMES`. `spawn_weight[ARCH]` and
  `ArchConfig::SPAWN_CHANCE` are the same multiplier; 1.34x in either is
  identical. The THEMES column now states a SHAPE — flat, at the mean — and
  buying count there re-introduces the spread E1 removed. One fact, one home.
- E1 REDISTRIBUTED, IT DID NOT ADD. barren/transition 0.3 -> 0.75 (2.5x more
  arches); monumental/colonnade 1.0 -> 0.75 (-25%). Global mean held. A dense
  theme reading as thinned is this, not a defect.
- THREE ROW BANNERS DESCRIBE ARCH CHARACTER THE NUMBERS NO LONGER CARRY
  (`population_themes.hpp` THEMES rows 1, 2, 4: "varied arches", "moderate
  arches", "near-empty"). HELD DELIBERATELY, not overlooked: if E3 returns
  `>= 13` the tree enters a capacity campaign, and if portals ever become a
  population separate from arches the banners become true again with no edit.
  Rewriting prose that may be re-truthed shortly is churn. Unblocked by E3's
  verdict.
- PRE-CHANGE BIOGRAPHY IS AT `503f90f`. E1/E2 are frozen-biography edits:
  every world ever born changed, and `?seed=N` no longer reproduces anything
  pinned before `551325c`.

## WALLS_3 — claimed and shown, split; what R0 could not confirm

Origin: WALLS_3 (one commit on master, base `815f427c`). `AuthoredStagingRecord`
now carries `shown_disk_index` beside `disk_index`, and the three consequences
of the overload are closed: the rotation's `disk_in_use` protects the claim AND
the picture, `authored_fetch_release_slot` falls its claim back to what is
shown instead of blanking a good image, and the three numeric-order readers
sort by what is on the wall rather than by what is in flight.

- R0's SECOND CONFIRMATION FAILS, AND THE CAMPAIGN ASSUMED OTHERWISE.
  `disk_in_use` is `bool disk_in_use[256]` — a FIXED CAP, not a
  `std::vector<bool>` sized to the manifest. It is documented in place as THE
  CAP FAILS OPEN: every read is guarded `disk_idx < 256`, so an index past the
  array short-circuits to "not in use" and the no-duplicates rule quietly
  stops applying to the overflow. `tools/web_dist.py` mirrors the number as
  `MANIFEST_DEDUPE_CAP = 256` and warns loudly when a dist exceeds it. Inert
  at a manifest of 57 and the two constants must move together. WALLS_3 kept
  the `< 256` guard on its new mark for that reason. Unblocked by a manifest
  that approaches 256, or by a round that makes the array the manifest's
  length.
- THE DEDUP IS TIGHTER NOW, BY DESIGN, AND HERE IS THE MARGIN. A picture is
  unavailable if any slot claims it or shows it, so a rotation can no longer
  reissue anything currently held in staging. Against a 57-image manifest and
  32 staging slots at least 25 pictures are always free; after a heavy world
  of roughly 28 consumed slots showing 28 distinct pictures, 29 remain for 28
  rotations — it fits, but not by much. Total lockout would need 32 shown plus
  26 distinct pending claims. VISUAL GATE 3 is the reading: if
  `[Authored] Rotated N slot(s)` stalls, this margin is where it went, and a
  slot that fails to rotate now KEEPS its picture rather than going blank, so
  a stall costs cycling and not walls. Unblocked by a larger manifest if the
  eye ever sees it.

### What recon found that the campaign did not expect

- R0, ON ITS MAIN QUESTION: THE PREMISE SURVIVED FALSIFICATION. `disk_in_use`
  in `rotate_authored_staging` is the only dedup on disk identity anywhere in
  the tree — there is no `disk_index ==` or `!=` comparison in `src` at all.
  The boot fill dedupes by CONSTRUCTION (slot i takes disk i, once) rather
  than by comparison, and `load_authored_image_to_staging`'s `if (rec.pending)
  return;` guards the slot, not the picture. One site, and E1.3 edited it.
- R1 REPORTS THREE WRITERS, NOT THE THREE EXPECTED. The rotation's claim and
  the boot fill's claim are ONE line — both callers reach
  `load_authored_image_to_staging`. The third expected writer, the failure
  release, is there. The one the campaign did not list is the ARRIVAL:
  `authored_stage_decoded_image` writes `rec.disk_index = disk_index` beside
  the `valid = true` that E1.2 anchors on, which is why E1.2's
  `shown = rec.disk_index` is exact there and would not have been elsewhere in
  that function.
- R2 REPORTS THREE READERS WANTING SHOWN, NOT ONE, AND ALL THREE WERE
  REPOINTED. `pick_authored_staging` was the expected one. The other two are
  `commit_gallery`'s authored fallback scan and `place_wall_paintings`'s
  selection — both order by lowest `disk_index`, both choose what to PUT ON A
  WALL, and both carried the same silent ordering regression WALLS_2 caused.
  Leaving either on the claim would have left the campaign's own gate 1
  failing on the outdoor monuments and on the walls themselves. The readers
  wanting CLAIM — the rotation's cursor logic and the fetch-context plumbing
  — were left alone.
- THE GUARDS COST NO ELIGIBILITY, PROVED FROM THE WRITERS. Authored `valid` is
  written in exactly two places now: the arrival, which sets
  `shown_disk_index` in the same breath, and the failure release, which sets
  `valid = (shown != UINT32_MAX)`. So `valid == true` implies
  `shown_disk_index != UINT32_MAX`, and the `!= UINT32_MAX` skips E1.4 added
  cannot exclude a record that would previously have been picked.

## WALLS_2 — the rooms hang (its open half closed by WALLS_3)

Origin: WALLS_2 (one commit on master, base `1191f60a`). `rotate_authored_staging`
no longer throws a picture away at REQUEST time: `load_authored_image_to_staging`
sets `pending` and the new `disk_index` and queues the fetch, but leaves `valid`
standing, so a record keeps its image until `onsuccess` overwrites it. R0 was
proved by reading rather than inferred — `drain_gallery_promotions` calls
`promote_to_exhibition`, which issues `encoder.CopyTextureToTexture` from the
staging array into `exhibitionTexture_`, an independent texture. A hung frame
holds a copy and cannot flicker when its source is replaced.

The two items this section opened — the duplicate window and the failure-path
desync — were both CLOSED by WALLS_3 (`b5c4a44b`), and their lines have died.
What remains below is the recon that campaign was built on.

- THE INVALIDATE WAS NOT WHERE THE CAMPAIGN PLACED IT, and the anchor it gave
  matched nothing. The line lives in `load_authored_image_to_staging`, the
  shared helper, not in `rotate_authored_staging`; at the stated eight-space
  indent it had zero matches and at four spaces two (the other being
  `authored_fetch_release_slot`). It was disambiguated by the comment block
  above it, which is unique. That helper has a second caller — the boot fill —
  which skips any slot already `valid || pending`, so it only ever loads into
  records whose `valid` is already false and the deletion is a no-op there.
- R1 AND R2, ANSWERED. Three writers of authored `valid`: the request site
  (this commit's), `onsuccess` (arrival, `valid = true`), and
  `authored_fetch_release_slot` (failure). No fourth. Readers run beyond the
  five the campaign listed — the boot fill and `rotate_authored_staging`
  itself also read `valid` — and those two DO reach a staging-texture write,
  but each writes only into records that are not valid (boot) or are consumed
  and not pending (rotation), so neither overwrites a texture another reader
  is relying on. No reader assumes the texture is stable across frames; the
  only assumption WALLS_2 breaks is `disk_index` ↔ picture identity, which is
  the first item above.

## WALLS_1 — the dial landed, the fix did not: what recon disproved

Origin: WALLS_1 (COMMIT 1 on master, base `41bda81`). COMMIT 1 landed. COMMIT 2
was STOPPED and COMMIT 3 was not written, per R3's standing instruction. The
campaign's diagnosis did not survive its own recon, and the fix as specified
collided with the hazard R4 was written to catch.

SUPERSEDED IN ITS OPERATIVE HALF BY WALLS_2 (`4b147afe`), which hung the rooms
without touching `consumed` at all — so the trade this section describes was
never paid. THE FLAG SPLIT IS RULED AND LANDED (OVERTURE_0 U6, ruling R-G):
`consumed` means "on a wall now" and is released when the painting leaves the
world, `hung_this_world` is the rotation's own flag, and the outdoor leak this
section named is closed. The freeze hazard R4 caught cannot arise from it —
the rotation reads the flag it was given. The findings below are kept because
they are the reasoning WALLS_2 and the split were built on.

- THE STATED DEFECT CANNOT HAPPEN: A MOOD CHANGE IS ALWAYS A PORTAL CROSSING.
  `place_wall_paintings` ← `apply_mood_indoor_shell` ← `apply_mood`, and
  `apply_mood` has exactly two call sites — `cartridge.hpp:685` (boot) and
  `cartridge.hpp:1355`. The second sits in the destination block that calls
  `teardown_gallery` at `cartridge.hpp:1308`, forty-seven lines of
  straight-line code earlier, whose last line clears `consumed` on every
  authored record. `pendingDestination_` is written in one place
  (`cartridge.hpp:1590`, from an arch's destination), so there is no
  mood-change path that is not a portal crossing. Every indoor room is
  therefore hung against a fully released `consumed` array, and the stated
  cascade — 28 of 32 in the first room, four in the second, zero in the third
  — cannot occur.
- THE SYMPTOM IS REAL AND THE ARITHMETIC IS RIGHT; THE MECHANISM IS ROTATION,
  NOT ACCUMULATION. `teardown_gallery` calls `rotate_authored_staging` BETWEEN
  freeing the layers and clearing `consumed`, and the rotation's selector is
  `consumed`: for every consumed slot it calls
  `load_authored_image_to_staging`, which sets `valid = false, pending = true`
  and queues a network fetch. `AUTHORED_FETCH_INFLIGHT_CAP` is 1, so the
  refetches run one at a time over a round trip, while `place_wall_paintings`
  runs in the SAME FRAME and selects on `valid && !consumed`. The room is
  hung only from the slots rotation did NOT touch — the ones the previous
  world did not exhibit. A four-wall room at `per_wall_cap` 7 consumes up to
  28 of 32, so the NEXT room opens with four valid records. The handoff's
  numbers are correct; they land one world later, through `valid`, not
  through `consumed`. Recovery is gradual as fetches land, which is the
  intermittency.
- SO COMMIT 2 WOULD WORK, BY THE OPPOSITE MECHANISM — AND WOULD FREEZE THE
  LIBRARY (R4). Clearing `consumed` at layer release leaves few slots consumed
  at teardown, so rotation invalidates few and the rooms fill. But rotation
  then has almost nothing to revisit: `rotate_authored_staging` skips every
  slot with `if (!consumed) continue`, so the 57-image library stops cycling
  through the 32 staging slots and every world hangs the same 32 pictures.
  E2.3 applied to `teardown_gallery`'s loop as the handoff directs makes the
  freeze total — that loop runs immediately before the rotation.
- R3, ANSWERED (b), WITH ITS CONSEQUENCE REVERSED. Entering an indoor mood
  does NOT evict resident outdoor patches: `evict_paintings_for_patch` is
  reached only from `evict_gallery`, which is the patch system's
  radius-driven evictor (`EVICT_BUDGET_PER_FRAME` 4), and nothing on
  `apply_mood`'s indoor path calls it. But the contention the question fears
  does not exist, because `teardown_gallery` frees ALL forty exhibition
  layers at `cartridge.hpp:1308` before `apply_mood` runs. The walls meet an
  empty array, not a full one. No ruling is owed on whether the world keeps
  its sand galleries across an indoor visit — it already does not.
- R2 AND R4, CLEAN. Four claim sites (`commit_gallery` snapshot + authored,
  `place_wall_paintings` snapshot + authored) and three release sites
  (`evict_paintings_for_patch`, `clear_wall_paintings`, `teardown_gallery`)
  — the counts the campaign required. `rotate_authored_staging` has exactly
  one caller, `teardown_gallery`. No fifth claim site exists.
- THE DUPLICATE QUESTION, CLOSED: E2.3 could not have shown one image twice
  in one room. `place_wall_paintings` releases nothing between walls — its
  only `clear_wall_paintings` call is at its head, before any wall is
  planned — so `consumed` accumulates monotonically across the walls of one
  room exactly as it does today, and it is `consumed`, not the per-wall
  claim masks, that later walls read. The rider's STOP does not fire.
- THE WITNESS, UNCHANGED AND EXACT (R1). The console line carries a trailing
  site-type field the handoff's version omits:
  `[WallPainting] Placed N painting(s) + M snapshot(s) across K walls (TYPE)`.
  No commit here touched it.

### The readings WALLS_1 is not closed without

COMMIT 1 alone is stampable now: (4) THREE OR FOUR — `across K walls` never
reads 1 or 2, and OVERTURE_0 U7 pins the two thresholds with a `static_assert`
so no dial can reopen the shape.

READING (1), THE STARVATION REPRODUCED — **CLOSED BY REPEAT_0 U5, NOT
OBSERVED.** It asked Jean to enter several indoor moods in one session and
watch `Placed N + M` fall while `across K walls` held at 3 or 4, as the
witness for the ROTATION account of the starvation rather than the
accumulation one. **The mechanism it was to witness is deleted.**
`rotate_authored_staging` ran between freeing the exhibition layers and
clearing the claims, and its refresh set `valid = false, pending = true` on
every record the last world hung — while `place_wall_paintings` hung the next
room in the same frame off `valid`. REPEAT_0 deletes the rotation whole and
moves refill to the pop, where the vacated layer keeps its picture until its
own fetch lands (WALLS_2). There is no starved room left to reproduce and no
`[Authored] Rotated` line left to time the recovery against; the `consumed`
overload ruling the reading awaited is superseded by that field's deletion.
Its replacement is REPEAT_0's VISUAL GATE 2, which reads the opposite
outcome — the next room hangs FULL immediately. `[WallPainting] BARE WALL <w>`
survives and now reports `authored ready` rather than `authored hangable`.

## SAND_2 — one sizing law, and the one thing it does not cover

Origin: SAND_2 (one commit on master, base `88c7a66`). `commit_gallery`'s two
branches now read the same law — `size_mult = max(0.5f, gallery_size_mean *
(1.0f + jitter))` into an area under a square root — so `gallery_size_mean`
is an AREA dial at both sites and `PAINTING_SIZE_SIGMA` the same fraction at
both. `AUTHORED_AREA` 48 against `PAINTING_AREA`'s 18–30 holds an authored
work at 2.4x a MEDIUM snapshot's canvas at every size and every aspect.
COMMIT 2 of that handoff was SKIPPED on its own instruction: no comment in
the tree states or implies `fill_slot_wall_frame`'s `base_height` is an
authored rather than a derived height, and `state.hpp`'s `scale_x`/`scale_y`
("width/height in world units") stayed true across the change.

- THE SIZING CENSUS, SETTLED (SAND_2's R0, run against the tree at
  `26117317` when the campaign was re-issued with a falsification item). Four
  campaigns in a row returned a different site count than their handoff
  predicted, so this is the census rather than a prediction. EXACTLY THREE
  sites write a painting slot's `scale_x`/`scale_y` in the whole tree, all in
  `bodies/gallery.hpp`: `fill_slot_wall_frame` (`scale_x = base_height *
  aspect`, `scale_y = base_height`), and `commit_gallery`'s snapshot branch
  writing both inline. No compound assignment, no post-hoc clamp or
  Y-correction mutates a slot's scale after the fill, and `world.wgsl` only
  READS the two fields — the shader never resizes. `ContentSource::AUTHORED`
  appears at exactly TWO call sites, both through `fill_slot_wall_frame`:
  `commit_gallery`'s authored branch, which SAND_2 rewrote, and
  `place_wall_paintings`'s, which passes an INDOOR `WallArtScaleBucket`
  height and is a separate sizing authority SAND_2 correctly never reached.
  So the premise held: `fill_slot_wall_frame` is the only consumer of the
  outdoor authored height. Recorded so the next campaign does not re-ask.
- THE ROW BUDGET IS AN ASPECT CEILING FOR BOTH CONTENT KINDS, not an authored
  defect. Width goes as `sqrt(area * aspect)` on both paths now, so a wide
  enough painting overruns `ROW_SPACING` 26 whatever it holds: the crossing
  aspect at the top of range and jitter is 1.94. Measured over the 57 files
  in `assets/paintings` (aspect 0.449–2.530), ONE asset can exceed the row,
  and only at the top of both range and jitter, reaching 29.7 wu; at a
  mid-range site none does. For scale, the deployed pre-SAND_2 tree put 20 of
  57 over the row at a mid-range site and 45 of 57 at the top, worst case
  91.7 wu. Unblocked by `ROW_SPACING`, if it ever bites.
- `AUTHORED_AREA` 48 IS A READING, NOT A RULING — it reproduces what the
  mid-range site held before SAND_1 (92.6 wu² x the 2.18x the snapshots got
  = 201.6 = 48 x 4.2). Whether Jean's paintings should preside more or less
  than 2.4x a MEDIUM snapshot is his eye's call, and it is now one number:
  20 hangs them as equals, 70 makes them dominate. Unblocked by VISUAL GATE 4.
- THE OLD RATIO CANNOT BE PRESERVED AND WAS NOT: it was never constant —
  1.06x at the smallest sites to 3.75x at the largest, times each painting's
  own aspect, because authored area carried an `x aspect` the snapshot area
  does not. "The same factor" is therefore honoured exactly at one point,
  fixed here at the mid-size aspect-neutral value. Small sites' paintings
  come out smaller than the old law gave and large sites' notably smaller;
  that is the quadratic being removed, not a loss. Recorded so a later round
  does not read the shrink as a regression.

### The five readings SAND_2 is not closed without

`?seed=N` pinned. (1) NO OVERLAP — walk a MIXED site, find a landscape-format
painting, its frame clears its neighbours; this was the live defect.
(2) CONSTANT RELATIONSHIP — in a small gallery and a large one, a painting
stands the SAME amount larger than the snapshot beside it; if the large site
still exaggerates, E1.2 did not land. (3) ASPECT NEUTRAL — a portrait and a
landscape at one site carry visibly similar amounts of canvas, one taller,
one wider. (4) THE PREMIUM ITSELF — stand before a pair and say whether the
paintings should preside more or less; see `AUTHORED_AREA` above.
(5) SNAPSHOTS UNTOUCHED — a snapshot-only gallery looks exactly as it did
after SAND_1; nothing in this campaign may reach that path.

## SAND_1 — what the campaign parked

Origin: SAND_1 (five commits on master, base `9d6250d`). All five landed. The
finding this section was opened for — the authored branch reading
`gallery_size_mean` as a linear dial — was ruled on and CLOSED by SAND_2
(`0715e39a`), and its line has died accordingly.

- `PAINTING_AREA`'s block comment still names the retired multiplier
  ("before the right-skewed multiplier [0.85, 3.0]", gallery.hpp above the
  table). False since E4.3; outside COMMIT 5's stated scope, which was the
  `OUTDOOR_SLOT_RESERVE` block only. Dies in the commit that next opens the
  shot-parameter block.
- `PAINTINGS_MAX_BY_ARCHETYPE` IS INERT (R3): at mean 3 σ 1 the raw count
  spans [1.5, 4.5) and the smallest table entry is 8, so the `std::min`
  never binds. Kept because a dead table and a behaviour change do not share
  a commit. Unblocked by any commit that is not itself a behaviour change.
- BOTH `size_mult` `0.5f` FLOORS ARE DEAD GUARDS (SAND_1 E4.4 rider, joined
  by SAND_2 E1.2 which gave the authored branch the same form): `1.0f +
  jitter` bottoms at 0.55 and `GALLERY_SIZE_LO` is 3.4, so the product
  bottoms at 1.87 at either site. SAND_2 also retired the authored branch's
  `2.0f` height floor by superseding it — measured over the library the new
  height bottoms at 5.96 wu, so it too was unreachable. The two survivors are
  left standing; deleting them is its own ruling. They die in the commit that
  reopens the sites.
- `PHOTO_PACE_BY_ARCHETYPE` IS A FLAT TABLE, hence a dead mechanism, and the
  row says so itself. If the even distribution reads right at VISUAL GATE 2,
  the 0.6 folds into `TRIGGER_DISTANCE_MEAN` and the table goes. Unblocked by
  Jean's stamp on that gate.
- `TRIGGER_DISTANCE_FLOOR` HAS BECOME A SHAPER, NOT A GUARD (COMMIT 1
  rider): at `TRIGGER_DISTANCE_MEAN * 0.6 = 30` against σ 8, the 20 wu floor
  clamps 10.6% of draws, where the old fastest tier clamped 3.0% and sand
  0.0%. Reported, not changed — lowering the floor is Jean's. Unblocked by
  his word.
- HELD FOR A LATER ROUND, ON JEAN'S WORD: `MIN_GALLERY_DISTANCE` below 110,
  and `EXHIBITION_LAYERS` above 40 at +24 MiB GPU (gated separately against
  the Firefox staging ratchet above and the Pixel 8 Pro floor). VISUAL GATE
  4 and GATE 3 respectively are the readings that decide whether either is
  needed.

### The eight readings SAND_1 is not closed without

Jean's eye, no gate can see them. Deploy with `?seed=N` pinned so two walks
are comparable. (1) COMMON — galleries arrive across sand without being
hunted; count over ~1000 wu against the same seed before the campaign.
(2) EVEN — they appear on mountain and varied ground too; this is COMMIT 2's
whole subject and the one thing the flat table can be falsified by.
(3) FED — `[Gallery]` stdout reads `paintings=N/N`; a run of `paintings=1/3`
means supply still lags and COMMIT 1's pace must go below 0.6. (4) SPACED BY
THE CONSTANT, NOT THE REGISTRY — no two centres closer than 110 wu and the
typical gap near 110, not well above it. (5) LARGER — the smallest works at
roughly twice their former width, felt on approach at pawn height.
(6) STILL VARIED — works within one gallery visibly different sizes.
(7) NO OVERLAP — no canvas intersects its neighbour at the top of the range;
a panoramic pair on a large-mean site is the case to look for. SAND_2 landed
the authored half of this reading; see its own gate 1.
(8) INDOOR UNCHANGED — wall art hangs exactly as before; nothing in this
campaign may reach it.

## RIBBON_1 — the witnesses Jean owes the campaign

No gate can see any of these; they are the eye's, and the campaign is not
closed until they are looked at. Origin: RIBBON_1 (three commits on
`claude/ribbon-1`, base `fd53316`). Three were looked at, and RIBBON_2's
charter is what came back; the finding is noted under each. None is closed
here — the sign-off is Jean's, and RIBBON_2 has its own list below.

- THE FLOWN BODY FOLLOWS ITS TRACK. The spine became a SPACE law: one
  sample per chord of flight, so the body is drawn where the head has been.
  Tier 0 at full throttle and every parked ribbon are unchanged; tiers 1
  and 2 and every wanderer are visibly different from `fd53316` — where the
  body used to whip, laying rings at `cube_size` regardless of how far the
  head actually travelled, it now trails the path. This is the ruling to
  overturn if the old whip was the beauty.
  LOOKED AT (RIBBON_2 §0.3): the space law stands — the whip was not asked
  back. What was owed was the settling the time law's delay line used to
  give when the hands went still, which a space law has no version of. THE
  SWEEP is that version; witness (2) below.
- THE SKY RULE, both readers. The head should bank away from a shaft and
  climb a roofline before it reaches one (COLOSSAL antennas are the test:
  125 m of post with drums 20 m wide at the top). The body should bulge
  around what it meets and flow through the bulge — the bulge stays where
  the thing is — and lift over a pyramid rather than enter it.
  LOOKED AT (RIBBON_2 §0.1), the BODY's arm only: it still entered things,
  and the reason was structural, not a tuning — a critically damped string
  settles INSIDE a shell that only pushes it. The shell became advice and
  stands wider; THE WALL became law; witness (1) below. The HEAD's arm —
  banking away from a shaft and climbing a roofline before it reaches one
  — is still owed as written.
- THE SEAT. `R` boards and lands on an ease, never a teleport. The saddle
  sits on ring 0's top face through roll and pitch; the nose faces where it
  swims and the tube does not shear at the neck.
  LOOKED AT (RIBBON_2 §0.5): the ease and the saddle drew no finding. What
  came back was the camera, which kept the orbit the pawn had left it in
  instead of turning behind the rider; THE CHASE answers it, witness (4)
  below. The seat itself stays owed a verdict.
- THE DIALS MOVE THE FLIGHT LIVE. Sixteen `Ribbon · Head` / `Ribbon · Sky
  Rule` / `Ribbon · Wander` rows on the organ (twelve at RIBBON_1, four
  more at RIBBON_2) reach the kernels through `config.ribbon_*`; turning
  one mid-flight should change the flight without a respawn.
- FIREFOX. Three `WriteBuffer`s a frame per ribbon, where fifteen stood,
  and 6.4 KB of ring poses no longer uploaded twice per frame. The staging
  ratchet above reads against this.

### Priced at RIBBON_1, not built

- THE GESTURE RING. `[SEAM:ribbon-displacement]` (world.wgsl §6.5) now names
  its own shape: to let music drive the head's displacement, record the
  head's lateral/vertical into a GESTURE ring beside the spine —
  time-cadenced where the spine is chord-cadenced — and read the delayed
  samples in `ribbon_displacement_at`. Unblocked by the music-coupling
  campaign.
- THE SKY RULE'S COST. `sky_push` walks 344 emitters (32 shafts, 16 arches,
  32 walkers, 264 floaters) per reader, and `ribbon_body` runs one reader
  per ring — up to 400 — plus `ribbon_ground`'s analytic terrain per ring.
  No measurement exists; `the-board-web-meter`'s `ribbon_body` row is the
  first one to read. The cures if it bites, in order of cheapness: an
  EMIT_STRIDE-style stride on the body's rule reads, a broad-phase cull by
  the ribbon's own bounding box, or a shared per-frame shortlist the head
  builds once.
  RIBBON_2 RAISED IT, and the price is stated rather than measured, since
  no meter row exists to measure with: `sky_self` adds up to 200 capsule
  tests per reader on top of the 344 emitters, `sky_wall` adds a second
  walk of the standing things per ring, and the head — still ONE thread —
  now also sweeps the spine, ≤ 401 serial steps a frame. The sweep is the
  one of the three that is structurally parallelizable (each chord reads
  only its predecessor, so it is a scan), and it is the first thing the
  optimization sitting should look at.
  RIBBON_4 CLAIMED TO MOVE THE FRAME and RIBBON_6 WITHDREW THE CLAIM. It
  priced its slicing without a meter; the meter, when it was finally read,
  said streaming's worst frame was 2 ms of GPU and had never cost a frame at
  all. The slicing, the urgency margin and the backlog ladder are gone, so
  the dials that paragraph named no longer exist and it is not kept as a
  record of them (L30). What stands in their place is one number:
  **the streaming frame is one whole bake, ~2.4 ms of GPU, every frame the
  same** — and 15 frames to a grid crossing against 18.75 available at the
  top of the speed dial. Evenness by construction, adequacy by arithmetic.
  If terrain is ever seen ARRIVING at the edge of the ring rather than being
  there, `BAKE_BUDGET_PER_FRAME` is the one dial that buys it back.

  RIBBON_3 MOVED IT BOTH WAYS, still unmeasured. Up: an arch costs 8 rib
  capsules plus 2 piers per arch per reader where it cost one disc — 16
  arches, so up to 160 capsule tests added to every `sky_push` and every
  `sky_wall` call. Down, and by more: the head now hears 8 floating slots
  instead of 264, which removes up to 256 sphere tests from the head's
  reader every frame, and `sky_roof` lost its arch loop entirely. The head
  is still ONE thread. The body's reader is where the arch rib is actually
  paid, once per ring.

## RIBBON_2 — the witnesses Jean owes the campaign

Six, in Jean's own order from RIBBON_2 §0. No gate can see any of them.
Origin: RIBBON_2 (three commits on `claude/ribbon-1`, base `a76bbed`).

- THE WALL. No ring center stands inside a drum, a pyramid, a walker or a
  floater — nor under ground plus half a tube. (An arch is no longer a solid
  in this law: RIBBON_3 made it a doorway, so the wall keeps rings out of
  its rib and its piers and leaves the SPAN open.) The shell is
  advice now and stands wider (`clear_head` 25 → 40, `clear_body` 8 → 16);
  `sky_wall` runs AFTER the leash, because law outranks leash, and it kills
  only the velocity that points into the wall. The test case is a COLOSSAL
  antenna: 125 m of post with a drum 20 m wide at the top, which the old
  soft push let a ring settle 18 wu inside.
- THE SWEEP. Hands still → the body settles straight behind the head, and
  the settling FRONT should be seen travelling tailward at `propagation_
  speed`, not the whole body straightening at once. At full throttle the
  track is kept near the head instead. RULING TO OVERTURN WITH ONE WORD:
  the sweep runs at 1.0 × P idle and 0.15 × P at full throttle
  (`RIBBON_SPINE_RELAX_IDLE` / `_FLY`, world.wgsl §6.5).
- THE BODY IS A THING. A tight turn → the head goes OVER its own body and
  the body bulges off itself, both through the same Sky Rule that reads the
  world. The body it reads is last frame's emit half, minus a
  `RIBBON_SELF_NECK` of 24 rings so the neck does not fight itself. RULING
  TO OVERTURN WITH ONE WORD: over, not under, unless the body is clearly
  above.
- THE CHASE. `R` turns the camera behind the rider over the boarding ease,
  looking along the flight. OVERTURNED BY JEAN, and landed at RIBBON_3 P1:
  it does NOT re-center afterwards — the pose is taken once, at boarding,
  and the mouse owns the camera from then on (`RIBBON_CHASE_TAU` 0.0; > 0
  restores the idle-mouse settling). The elevation is his too:
  `RIBBON_CHASE_ELEVATION` 0.25 → 0.6, about 35° to the ribbon's surface.
  What is still owed is the look of that one pose — witness (4) below.
  EXTENDED BY KITE_1 (W2, and the standing ruling is untouched — pose taken
  once at boarding, mouse owns azimuth, `RIBBON_CHASE_TAU` still 0.0): the
  chase now holds the flight instead of trailing it. Board, full throttle,
  straight — the head should HOLD the screen placement the boarding gave
  it, sustained, where it used to lead by v·tau (12 wu at 40 wu/s). Hard
  turns must keep the kite's give, and the sway must stay eased.
  `Camera · Chase / feed-forward` at 0 restores today's trail, which is the
  proof the mechanism is the one described. Origin: KITE_1 C2/C3.
- THE WANDERER STEERS ITSELF. A wanderer should cross its anchor's disc
  target to target and come back, not drift away and never return. The
  brain is the head kernel's now — the target is drawn from the ribbon's
  own seed on a `roam_radius` disc, steered toward through the same yaw cap
  a rider's hands pass through, re-drawn inside `wander_arrive`. Four dials
  (`Ribbon · Wander`) move it live.
- IF THE MOTION STILL READS STEPPED. The polyline is gone (the spine is a
  C1 Hermite arc on each sample's own tangent), so if it still steps the
  cause is one of the three cadence facts, which are recorded here so the
  next sitting does not re-derive them:
  (a) `signal.dt` WAS a `std::chrono::high_resolution_clock` difference
  across `begin_frame()` (`console.hpp`), clamped to `[0, 0.1]` s, with no
  smoothing; `signal.t_seconds` is the running sum of that same clamped dt
  (`beat_clock.hpp`, `BeatClock::update`) — one clock, one integration.
  Under Emscripten that clock is `performance.now()`, which the browser
  coarsens. ANSWERED (RIBBON_3 P2): the measurement is made the same way and
  is still the only clock, but the value handed on is THE STEADY CLOCK's.
  Witness (5) below is what to check if judder survives it.
  (b) `dispatch_compute` is an unconditional row of `RENDER_SPINE`
  (`cartridge.hpp`), so it is recorded on EVERY rendered frame. But
  `update()` runs on frames that are never rendered — `the_board.cpp` returns
  before the encoder when `acquire_surface_texture()` fails — and
  `phase_fill_signal` has already written that frame's dt, which the next
  update overwrites. A dropped acquire therefore does not stretch the
  head's step; it DELETES one. LANDED (RIBBON_3 P2): the ruling went the
  carry's way. `phase_fill_signal` accumulates into `dtPending_` and the
  host clears it only once the frame's command buffer is submitted, so an
  updated-but-unrendered frame now stretches the next rendered step instead
  of deleting itself. The sum carries the same 100 ms ceiling the raw
  measurement does — a stretch is a stretch, a teleport is not.
  (c) The web main loop is `requestAnimationFrame`
  (`emscripten_set_main_loop(frame, 0, true)`, `the_board.cpp`).
  The gesture clock is NOT a suspect: `ribbon_frame_tick` advances
  `par.phase += beat_rate × (60 / reference_bpm) × dt` every frame — a
  smooth per-frame float, never a beat-quantized tick — which is why §3.5
  took its first branch and the clock did not come home.

## RIBBON_6 — ONE BAKE A FRAME, AND THE PRESENTATION LAW

Origin: RIBBON_6 (two commits, base `3493a05`). This round WITHDREW a premise
two earlier rounds were built on, so read this entry before theirs.

- **WHAT THE METER SAID ALL ALONG**, recorded so no future round re-derives
  the streaming hypothesis: across three steady windows of Jean's own
  recording, `stream_patches` never exceeded **2.00 ms of GPU** and its means
  were 0.01–0.02 ms; `frame_total` never exceeded **2.21 ms of CPU**; fps
  59.9 / 60.0 / 60.0. **Streaming never cost a frame.** RIBBON_4's charter
  held that the conductor's bursts were the blocks along the way; they were
  not, and everything built on that has been withdrawn. Origin: RIBBON_6 §0.
- ONE BAKE A FRAME. Fly straight at the top of the speed dial for a minute.
  The world should keep up without holes, and no frame should carry more than
  one bake. In a meter build `[STREAM]` shows `young=0` throughout — **if
  `young` ever flickers to 1 during ordinary flight, this round's diagnosis is
  wrong and that flicker is the whole finding.**
- THE PRESENTATION LAW. `[PRESENT]`, riding and walking, at the exhibition
  canvas. A near-pure `1x` column with the ride reading smooth closes the
  campaign. A fat `2x` column names the cost as GPU-side and hands the
  optimization sitting its target — and the meter's own first window already
  says where the budget goes: `main_pass` 7.4–11.7 ms and `shadow_pass`
  2.2–5.2 ms of GPU. The `[METER]` line now carries the canvas and an `over`
  count beside them, because a GPU budget read against an unknown resolution
  is not a reading.
- TWO RULINGS, each one word: `BAKE_BUDGET_PER_FRAME` (1 — the law; raising it
  buys catch-up and spends evenness) and the youth threshold (three quarters
  of the window, cleared once and never re-armed by anything the player does).

## The ledgers' provenance stamp can never name its own commit

- STRUCTURAL, PRE-EXISTING, and recorded so no reviewer chases it: the ledger
  tools stamp the last COMMITTED commit that touched a scanned file, so a
  commit that both edits a scanned file and regenerates its ledger cannot
  record itself — it records its predecessor. Verified across seven commits
  in this campaign (`32d47d0`→`2f6bd5f`, `2f6bd5f`→`a516038`,
  `a516038`→`d3bfe2d`, `1fce093`→`2e7c948`). Every census row and every file
  `sha256` inside the ledgers is byte-identical on a rebuild; only the
  provenance stanza moves, a four-line diff. CLAUDE.md's "delete the five
  files and the tree is byte-identical again" is therefore literally true
  only at a docs-only tip. The cure, if one is ever wanted, is for the tools
  to stamp the WORKING TREE's hashes rather than a commit id; not built.
  Origin: the RIBBON_6 deletion-safety audit, re-run adversarially.

## Found by the RIBBON_5 audit, still open

- **THE POINT MIRROR CAN FREEZE, and nothing says so.** Every streaming
  consumer reads `c->point_` — the window centre, the eviction sort, the alloc
  box, the bake ordering, the draw band. TEARDOWN authors it and bumps
  `world_gen`, dropping every in-flight readback callback. If
  `pawnReadbackState_` ever wedges in MAPPING, `point_` never moves again:
  `gridChanged` stays false, the box raiser never fires, and `stream_patches`
  encodes nothing — a true 0.00 ms with a visibly moving player, which is the
  one mechanism that produces that reading. **The test is already in the tree
  and costs nothing to run:** in `[STREAM]`, `center=(cx,cz)` must equal
  `floor(point/50)`. If they ever disagree, the readback is the fault and no
  amount of conductor work will help. Unblocked by a meter-build session.
- The readback's own recovery has no witness either: nothing prints if a
  `MapAsync` never completes. Priced at one line beside the state machine,
  not built. Origin: the RIBBON_5 conservation audit, re-run adversarially.

## RIBBON_5 — THE WORLD COMES BACK

Origin: RIBBON_5 (two commits, base `b4fe1fb`). This one comes first: until
the world rebuilds, no other witness in this file can be read at all.

- THE WORLD COMES BACK. Walk through the door-fallback arch ON PURPOSE — it
  stands ~60 wu from spawn by design, so it is a short walk. The world fades,
  reseeds, and must **rebuild whole around the pawn within a few seconds**,
  then keep streaming as he walks. In a meter build the `[STREAM]` line (1 Hz)
  is the reading: `free` must return toward `MAX − active`, `young` must clear
  as the window fills, and `ALLOC`/`SPAWN`/`GEN` must move every second. The
  one-patch world is impossible while the conservation witness stays silent —
  and that witness runs in EVERY build, not just a meter one, so if it ever
  prints, that line is the whole diagnosis.
- FOR JEAN, two lines that are not defects:
  **The "mutation" moment was a door, not a bug.** Doors are arches; the
  fallback arch stands near spawn by design; crossing one is a world
  transition, and the patch underfoot changing IS the new seed's terrain
  arriving. That part was the program working. What was broken is only that
  the world never finished rebuilding afterwards.
  **RIBBON_4's circle-vs-straight witness is RETIRED**, not owed: RIBBON_6
  withdrew the premise it was written to test, and it was never reachable
  anyway.
- ONE RULING LEFT FROM THIS ROUND: whether the conservation witness stays
  always-on. It costs one O(225) walk a second and is the only thing standing
  between a layer leak and a silent one-patch world — the recommendation is
  that it never moves behind a dial. (The youth threshold and the young
  budgets moved to RIBBON_6's entry, which rewrote both.)
- WHAT THE AUDIT FOUND — and it found a real break, on the second pass. The
  first pass asked "does every site that clears `valid` return its layer?"
  The answer is yes, and the answer was useless, because the break runs the
  other way: **the continuous-allocation block checked pool capacity while
  COLLECTING candidates and never again while spending them.** The scan
  decrements nothing, so with `free == 2` and 15 vacant cells all 15 became
  candidates, `allocThisFrame = min(15, ALLOC_BUDGET_PER_FRAME) = 4`, and
  iterations 2 and 3 called `alloc_layer` against an empty pool — which
  silently returned **layer 0**, already owned by a live patch, and still
  wrote a valid record and incremented the count past the pool. Two records
  sharing one heightfield layer is one patch's terrain mutating under
  another, and `active_patch_count` past 225 writes `patches_[225]` — which
  is `freeLayerStack_[0]`, the member immediately after it. The bookkeeping
  error becomes memory corruption of the free list on the first frame it
  fires.
  **The trigger is ordinary and the regression is RIBBON_4's**: a built-out
  world, the player crosses one patch boundary, 15 cells go vacant.
  Before RIBBON_4, `EVICT` and `ALLOC` were both 4, so eviction always
  supplied what allocation could spend. RIBBON_4 lowered `EVICT` to 2 for
  the steady cadence and left `ALLOC` at 4 — and that gap is the hole. So the
  handoff's second defect was right in substance and wrong in mechanism: the
  pool did wedge and layer 0 did go to every comer, but by over-taking, not
  by leaking.
  Fixed at the site (`alloc_layer` refuses and says so; both loops honour the
  refusal; both write sites carry an independent array-bound guard), and the
  1 Hz conservation witness now proves it rather than assuming it. The
  zero-headroom equality remains (`MAX_ACTIVE_PATCHES == 225 == the 15x15
  window`): the pool has no slack by construction, which is exactly why a
  comment saying "this shouldn't happen" was never enough.

## RIBBON_3 — the witnesses Jean owes the campaign

Five. No gate can see any of them. Origin: RIBBON_3 (two commits on
`claude/ribbon-1`, base `92d58c6`), answering Jean's witness from the
saddle: *the flight accelerates almost smoothly, then breaks — like hitting
many blocks along the way, a few times a second, worse with speed.*

- NO BLOCKS. From the saddle, at speed, near things: the flight should read
  as ONE curve. Two causes were closed for it. THE STEADY CLOCK (P2) makes
  the frame's dt the display's cadence rather than the callback's arrival —
  the running mean while the measurement stays within ±20% of it, the
  measurement itself when it does not. ONE COMMAND, C2 (P1) low-passes the
  Sky Rule's lateral word (`RIBBON_RULE_TAU` 0.35 s) and slew-limits the
  total command (`RIBBON_YAW_SLEW` 1.5/s), so the heading's RATE is
  continuous where it used to step every time the probe crossed a shell.
  The head also stopped listening to the 256 cubes, each of which was a
  42-wu bubble at `clear_head` 40; it hears the standing things, the 32
  walkers and the 8 spheres, at `RIBBON_CLEAR_MOVER` 20. The cubes are the
  body's business.
- A DODGE IS A CURVE. An antenna is the test Jean already named as the one
  that looked RIGHT: it should now begin and end as a curve rather than
  snapping into and out of the avoidance. If it now reads too lazy instead,
  the two numbers are `RIBBON_RULE_TAU` (lower = quicker to hear a thing)
  and `RIBBON_YAW_SLEW` (higher = quicker to act on it).
- AN ARCH IS A DOORWAY. Jean's word was that the arch dodge looked STRANGE.
  It was: `sky_shell` gave every arch a disc of `half_span + max(thickness,
  depth)` and a top at the apex — for a MONUMENTAL a 70-wu drum 88 wu tall,
  which the head had to skirt or climb entire and the body was thrown out
  of. The tree's own law said the opposite all along (`occupier_contact`:
  *the SPAN stays open — walking through the doorway is the arch's whole
  meaning; only the legs push*). The ribbon should now pass UNDER, over or
  around, and the body should never be thrown out of a drum that is not
  there. The roofline is shafts-only for the same reason: a roof says the
  sky is closed at this xz, and an arch never closes it.
- THE CHASE POSE, ONCE. `R` → the camera takes its pose over the boarding
  ease, about 35° above the ribbon's surface, and then the mouse owns it —
  no drift back, ever. Both numbers are Jean's (`RIBBON_CHASE_ELEVATION`
  0.6, `RIBBON_CHASE_TAU` 0.0).
- IF JUDDER SURVIVES THE STEADY CLOCK, the next fact is not in this tree: it
  is the resolution of `performance.now()` on the machine in question, which
  is what `high_resolution_clock` becomes under Emscripten. Browsers coarsen
  it deliberately — commonly to 1 ms without cross-origin isolation, and
  Firefox with `privacy.resistFingerprinting` set coarsens it all the way to
  100 ms, which would quantize every frame's dt to a multiple far larger
  than a frame. That is a line for the optimization sitting, and it is
  measured in the browser, not read out of the source. THE IN-BAND SHARE, on
  the other hand, is readable here: at 60 Hz with ±2 ms of jitter the band
  is ±20% of a 16.67 ms mean, i.e. ±3.33 ms, so EVERY frame is in band and
  every frame is served the mean — the steady clock is fully engaged, not
  half of it.

## Harvested at WINNOW-2 W2 (from files that died this campaign)

- HARVEST: PREGEN-8 CONTINGENCY — rig-triggered storage weld (225→289 layers,
  TILE_GRID 17→19, MAX_ACTIVE_PATCHES) sleeps until the rig shows rim-pops under fast
  flight. (from audit/past reports/LADDER.md P-3, eafd9ec)
- HARVEST: TERRAIN-2 b3, the finite collapse — the radius-bounding choice (raise
  pregen / lower finite_radius_max / keep origin-pin degenerate) is undecided until b3
  lands; finite_radius_max is still live in gallery.hpp.
  (from audit/past reports/LADDER.md P-6, eafd9ec)
- HARVEST: the terrain word on the pinned seed — three rounds outstanding, and the last
  open item of PROBATE_I. Wrong looks like "the whole terrain is the same hill repeated"
  or "everything outside the first patch went flat". (from GATEHOUSE_REPORT.md, a5e84bf)
- HARVEST: GF6 — whether to pay for renaming tools/gates/console_gate/ to match what it
  now is; ruled HOLD, the directory rename stays unpaid.
  (from docs/audit/PROBATE_CLOSE.md + GATEHOUSE_REPORT.md, 606924f)
- HARVEST: SPAWN_3 parked — F6 newborns seat inside render radius on move.
  (from audit/past reports/FIELD_BRIDGE.md, 4c1a804)
- HARVEST: the mood-seam crack — neighbours disagree at a shared edge; remediation
  directions recorded but NOT executed, parked per ruling.
  (from audit/past reports/INVESTIGATION_mood_seam.md, eafd9ec)
- HARVEST: TEX_C0 — parked behind the feature wallet and Jean's eye on ASTC banding
  since PROBATE_F. As originally sketched it is **impossible**, not merely expensive;
  priced and shelved with its bill attached, not refused and not scheduled. The texture
  constraint belongs beside docs/FXC_LAWS_RECORD.md.
  (from docs/audit/TEX_C0_PRICE.md, de1d5db)

## Added at CANON C2

- VOICE bus (terrain live-modulation): alias-table design — VOICE_LAYOUT[] shaped
  like PARAM_LAYOUT[], one set_voice(channel, span) door, [VOICE] boot witness
  enumerating every channel with rest + address; sketch in terrain_program_charter
  §C5 (attic since CANON, last at a8f4580); unblocked by the music-coupling campaign.
- SALON Stage B (slot count): STILL OPEN, land-gated. PAINTING_MAX_SLOTS is still 288
  (state.hpp:271) and was last touched by LOOM_1 U3, not by a SALON re-spec; the
  re-spec §0.1/§0.2/§0.3 is unpaid. The witness the stage relies on DOES exist —
  tPipe (renderer.hpp:170). Origin: SALON_1 (attic, 3b931ba).
- SALON Stage D (weld): STILL OPEN, and sharper than the ledger recorded. The named
  slot-filter defect IS fixed — clear_wall_paintings now filters on the sentinel
  patch pair as well as form type (gallery.hpp:2481-2484). What remains: that
  function resets `gs.wall_frame_count = 0` unconditionally (gallery.hpp:2493),
  OUTSIDE the patch-filtered loop, while evict_paintings_for_patch decrements the
  same uint32_t unguarded (gallery.hpp:1423). An outdoor WALL_FRAME slot that
  survives a clear leaves the counter at 0, and the next evict on that patch
  underflows it; the value is consumed as a draw count (render_passes.hpp:490,614).
  Origin: SALON_1 (attic, 3b931ba).
- SALON Stage E (salon hang): STILL OPEN — visual gate, Jean's eye. Prerequisite
  findings §0.5 (vault-crown coupling) and §0.9 (live floor breach) stand.
  Origin: SALON_1 (attic, 3b931ba).
- SALON HELD (packing): STILL OPEN — never built: floor_margin and target_coverage
  both have 0 hits in src/. Note the ledger's cost figure assumed EXHIBITION_LAYERS
  32; the tree now reads 40 (state.hpp:294), so "32 -> 256 = +896 MiB" wants
  recomputing before it is spent. Origin: SALON_1 (attic, 3b931ba).

## GOL_RULES_1 — two facts the campaign's witness turned up

- `pack_cell_tag`'s tier field is 3 bits and ten tiers no longer fit.
  `world.wgsl` packs `tag = mode | ((tier & 0x7u) << 4u) | (height << 7u)`
  into an `rgba8unorm` alpha, so Conway tiers 8 and 9 (Day & night,
  HighLife) alias to 0 and 1. INERT ONLY while nothing reads the tier
  payload — `unpack_cell_tag_mode` takes bits 0-3 and stops, and the height
  bit is unread too — so the first reader of either republishes this as a
  live defect rather than finding it. It cannot be widened in place:
  4 bits of tier collide with the height bit at 7, and an 8-bit alpha has
  no spare. The encoding is Jean's to gate. Origin: the line pre-dates the
  campaign (`b84eb5f`, DEMO-1 s0); GOL_RULES_1 C4 is what made its premise
  false, by taking GOL_TIER_COUNT past 8. Unblocked by a ruling on the tag
  encoding — widen the alpha's budget, move the height bit, or retire the
  payload.

## GOL_TEMPO_2 — what the ladder opened and did not close

- **The ladder's grid has no musical origin.** `GOL_TICK_LADDER` makes every
  tick period a note value, and every rung divides 96 beats, so the whole
  board realigns every 24 bars of 4/4 — against a grid whose beat 0 is BOOT.
  That is correct while nothing else keeps musical time: the board is its own
  transport and the origin is arbitrary. The day the audio socket fills, it
  stops being arbitrary — every zone on the board would be quantized to a
  grid offset from the music by the page-load time, which is a uniformly
  random phase error, not a small one. The ladder cannot fix this from where
  it sits: `t_beats` is handed to the shader already accumulated, so the
  origin needs a home before a transport can move it. Origin: opened by
  GOL_TEMPO_2 U1, which built the grid and deliberately did not build the
  transport (§6 out of scope). Unblocked by a ruling on where musical time
  starts — a transport whose origin the board reads, or an explicit statement
  that boot IS the downbeat.
- **`mode_gol_tick_scale` must stay 1.0, or quantize its product.** The dial
  multiplies `tick_period` inside `pulse_cell_target` and nowhere else, so it
  is not an author of the tick and the one-author law is intact — but it is
  the one live seam through which a Pulse cell's rate can leave the ladder.
  At any value but 1.0 a Pulse zone's cells oscillate off-grid while that same
  zone's tick gate and transit stay on it, which is the duty desync the
  quantizer exists to prevent, arrived at by hand instead of by backend. The
  dial is held at neutral by the boot block (`cartridge.hpp`, the only code
  caller, passes 1.0f) and the shader's own comment calls it DRIVERLESS, so
  today only the organ panel can move it. Origin: found by GOL_TEMPO_2's R5
  recon, which confirmed no ORGAN row writes `tick_period` or either tier
  table — this dial is the whole of the exposure. Unblocked by either pinning
  the ORGAN row read-only, or snapping the product
  `tick_period * mode_gol_tick_scale` to the ladder at both read sites.

## REPEAT_0 — THE CONVEYOR · Phase 0 recon (read-only)

The campaign replaces authored SELECTION with a FIFO playlist: one sequence
(the manifest), one window (the 32 staging layers as a ring), and one verb —
to exhibit is to pop the head and append the cursor in the same act. This
section is the recon that had to land before any edit. Seven gates, every
count with its recipe, symbols and never line numbers.

**Tree at recon:** `f4e2d0dc`, non-shallow (`git rev-parse
--is-shallow-repository` → `false`), working tree clean. The order itself is
not in the tree — `docs/HANDOFFS/` does not exist, which per CLAUDE.md is
health — so every claim below was checked against the order as relayed, not
diffed against a tracked artifact.

### THE DRIFT REGISTER — where the order and the tree disagree

The order was written from a snapshot. Nine of its statements do not
describe `f4e2d0dc`. None is fatal; all are recorded here because the units
below were re-scoped against the tree, not against the order.

| # | the order says | the tree says | weight |
|---|---|---|---|
| D1 | R9: `AUTHORED_FETCH_INFLIGHT_CAP` **stays 1** — "one lane is what makes validity contiguous from the head" | the constant is **4**. OVERTURE_0 R-E raised it, and this file's FIREFOX STAGING RATCHET entry records the raise. R9's premise is false | **flag — the conclusion survives, see R9-RESCUED below** |
| D2 | U1/U2: a **paintings range** of the manifest (`range.lo`, `range_size`) | there is no range. `ATTIC_ATRIUM` D2 deleted the partition; `rotate_authored_staging` says so verbatim: "what is left is one collection, walked by one cursor". `exhibition_manifest_onsuccess` parses the `"paintings"` key alone | **flag — the model simplifies: range = the whole manifest** |
| D3 | U2: `load_authored_image_to_staging(gs, gpu, queue, layer, disk, url)` | the live signature is `(GalleryState&, uint32_t staging_layer, uint32_t disk_index, const char* path)` — **no GPU, no queue**. AUBADE U5b took the device off this path on purpose | **flag — `authored_pop` needs neither `GPUState&` nor `wgpu::Queue&`** |
| D4 | R8/U6: `authored_record_for_disk` and `shown_disk_index` **stay — the atrium reads them** | the atrium does not exist. `authored_record_for_disk` has **zero callers and zero declarations** — `ATTIC_ATRIUM` D1 deleted its only caller and left the definition orphaned. Recipe: `grep -rn "authored_record_for_disk" . --exclude-dir=.git` → 1 hit, its own definition | **flag — see G4** |
| D5 | U5 anchor: `for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) gs.authored_staging[i].consumed = false;` (one line) | the live loop is braced and clears **two** fields — `consumed` and `exhibition_layer` | **flag — site unmistakable, quote stale (P15)** |
| D6 | G2's recipe `grep -n "authored_staging\[" src/ -r \| grep consumed` enumerates the census | it returns **9** hits against a live census of **15** expressions. Six are reached through an alias binding and are invisible to it — including the writer in `authored_release_layer` (P16: a recipe must name what it cannot see) | **flag — census redone by hand, below** |
| D7 | G6: the `found N paintings` sentence is web_dist.py's | it is **C++**: `exhibition_manifest_onsuccess` emits it. `tools/web_dist.py` writes `exhibition.json` and never says "found" | **cosmetic — the coupling R10 protects is real, its home is not where the order put it** |
| D8 | U5: "the exhibition-layer freeing loop and **its ATRIUM_7 exception**" | there is no exception. `teardown_gallery` frees all forty layers unconditionally; the comment claiming otherwise is orphaned prose, and ATRIUM_7 in the live tree means arch-shell geometry | **cosmetic — nothing to preserve; comment left for a sweep** |
| D9 | (unstated) `gallery.hpp` is free to edit | it is an **enforced sha256 pin** in `audit/BINDING_LEDGER.md` and `audit/COMMAND_LEDGER.md` (`sha256:b77b6c07…`, matching live). Any byte change reds both until the generators re-run, and `MIRROR_LEDGER` pins `BINDING_LEDGER`, so the cascade is three tools in order: `binding_ledger.py` → `command_census.py` → `mirror_census.py` | **flag — a ledger commit is owed at the campaign's end** |

**R9-RESCUED — the ruling holds on a better argument than the one it gave.**
R9 wanted one lane so that "arrivals are FIFO because departures are." With
four lanes arrivals can land out of order, so that sentence is false. The
design survives anyway, because **pops are strictly ordered even when
arrivals are not**: `authored_pop` takes the head and advances, so the
pending records are always the k most recently popped, which in ring order
from the head are the k LAST positions. Validity is therefore contiguous
from the head by construction of the pop, not by construction of the fetch.
Out-of-order arrival can only make `authored_ready_depth` report FEWER ready
layers than are actually ready — never more — and a conservative depth
under-hangs a row, which is the already-legal thin-room case. The cap is not
touched by this campaign; re-pricing it stays REPEAT_1's business.

### G1 — THE RECORD STRUCT · FLAG

**`authored_staging` and `snapshot_staging` are arrays of TWO DIFFERENT
structs** — `AuthoredStagingRecord` and `SnapshotStagingRecord`, declared
back to back, each owning an independent `consumed` bool. Recipe:
`grep -n "struct .*StagingRecord" src/cartridges/the_board/bodies/gallery.hpp`
→ 2. **So R7's fork resolves to its second arm: the field dies, not only its
uses** — subject to proving no reader survives, which G2 does.

`AuthoredStagingRecord` carries nine members. Their fate under the playlist,
each with the reader that decides it:

| member | who reads it after REPEAT_0 | verdict |
|---|---|---|
| `disk_index` | `authored_fetch_release_slot` (the fallback), the fetch context | **lives** |
| `shown_disk_index` | `authored_fetch_release_slot`, and U8's pop line — the log must name the picture the texture HOLDS, not the claim (WALLS_3) | **lives** |
| `aspect_ratio` | the plan's peek (row-width trim) and both slot fills | **lives** |
| `uv_scale_x` / `uv_scale_y` | both slot fills | **live** |
| `valid` | the ready predicate | **lives** |
| `pending` | the ready predicate | **lives** |
| `consumed` | nothing — R7 | **dies** |
| `hung_this_world` | `rotate_authored_staging` only, which dies at U5 | **dies by cascade** |
| `exhibition_layer` | `authored_release_layer` only, which exists only to clear `consumed` | **dies by cascade** |

**THE TWO CASCADE DEATHS ARE THE GATE'S REAL FINDING, and no unit named
them.** Every claim site writes a TRIAD in one breath under the in-tree
comment `THE CLAIM, WHOLE (OVERTURE_0)` — `consumed`, `hung_this_world`,
`exhibition_layer`. R7 deletes one leg; the other two have no reader once
the rotation and the release are gone. Under P7 the minimal coherent form is
to take all three, and U5/U6 do.

Stale, and worth one line: both struct banners still read `(circular buffer,
16 layers)` while `Dim::STAGING_LAYERS = 32`. The authored one becomes an
actual circular buffer at U1, so its banner is corrected there; the snapshot
one is out of scope and left.

### G2 — `consumed` CENSUS, AUTHORED SIDE · STOP RAISED, RESOLVED IN PLACE

**The order's recipe is blind to 40% of the census (P16).** It sees 9 of 15
expressions; the six it misses are every one reached through an alias
binding — `authored_stage_decoded_image` (`auto& rec`),
`load_authored_textures` (`const auto& r` and `auto& rec`),
`authored_hangable` (`const auto& r`), `authored_record_for_disk`
(`const auto& r`), and `authored_release_layer` (`auto& r`). The reaching
recipe:

    grep -n "consumed" src/cartridges/the_board/bodies/gallery.hpp

then read every hit for its binding. Fifteen expressions: **five writers,
ten readers.**

WRITERS (five, not the four the order expected):

| symbol | expression | unit |
|---|---|---|
| `place_wall_paintings` | `gs.authored_staging[f.record].consumed = true;` | U3 |
| `commit_gallery` | `gs.authored_staging[auth_stg].consumed = true;` | U4 |
| `teardown_gallery` | the clearing loop | U5 |
| `authored_stage_decoded_image` | `rec.consumed = false;` | U6 |
| **`authored_release_layer`** | **`r.consumed = false;`** | **named by no unit — this is the STOP** |

READERS (ten). Six die with the selection the order names. The other four
are the gate's finding, and each dies with its own host rather than needing
a home:

- `load_authored_textures` × 2 — the `disk_in_use` book and the "a picture
  already here is not re-asked for" skip. **Both die at U1**, which rewrites
  the fill: the ring fills every layer unconditionally and there is no
  dedupe book.
- `rotate_authored_staging` — the dedupe book. **Dies at U5** with the
  function.
- `authored_record_for_disk` — **dead code, zero callers** (G4). **Dies at
  U6.**

**THE STOP, AND WHY IT IS A FLAG.** `authored_release_layer` is the mid-world
release keyed on `exhibition_layer`, called from `evict_paintings_for_patch`
and `clear_wall_paintings`. Under the playlist there is no `consumed` to
release: supply comes from the cursor, not from records handed back. The
function's whole body is the release, and its other field —
`exhibition_layer` — has no other reader. **So it does not lose a home; it
loses its subject, which is what dying is.** The order's own STOP condition
names the default ("if the struct is not shared, delete the field too and
prove no reader survives"), and that proof is the table above. P17: default
taken, flagged, continued. HALT is reserved for an unreachable subject,
stale authority landing edits on the wrong tree, or an irreversible act —
this is none of the three.

**AND ONE THING U7 DOES NOT DO.** `authored_fetch_release_slot` contains
**zero** `consumed` reads or writes; its only two mentions are comment lines
RULING that it must not touch the field. U7 has nothing to remove here.

### G3 — CALLER CENSUS OF THE DYING · FLAG

- `rotate_authored_staging` — **one call site, `teardown_gallery`**, as
  expected. Recipe: `grep -rn "rotate_authored_staging(gs, c, queue);" src/`
  → 1. It is ~100 lines, the largest body in this census.
- `authored_write_cursor` — **already dead, unconditionally**: one writer
  (`load_authored_textures`), **zero readers** anywhere in `src/ tools/ web/
  docs/ audit/`. The order's conditional ("dies if the boot fill is its only
  writer") is answered stronger than it asked.
- `authored_staged_count` — **the reader outside `gallery.hpp` is real and
  the order understates it.** `offer_controls_when_ready` in
  `src/the_board.cpp` gates `OVERTURE_READY_FLOOR`; the `Controls:` line it
  emits is what arms `web/index.html`'s 20 s watchdog belt. It is a tally of
  `valid` and the playlist does not change what `valid` means, so it and
  `recount_authored_staged` **both live, untouched.**
- `recount_authored_staged` — **five** call sites, not "six-ish":
  `authored_fetch_finish`, `pump_authored_valve` ×2, `load_authored_textures`,
  `rotate_authored_staging`. The last dies with its host; the rest live.
- **THREE independent lowest-`shown_disk_index` selectors exist, not one.**
  `pick_authored_staging`, plus two hand-rolled inline loops that call it
  never and copy its order by comment: `commit_gallery`'s authored fallback
  (masked by `usedAuthored[]`) and `place_wall_paintings`' wall selection
  (masked by `authClaimed[]`). Replacing only the function would fork the
  ordering three ways. All three die.
- `authored_hangable` — its callers are **not** selection scans, and one of
  them the order never names: `gallery_available_staging` (which feeds
  `place_gallery_from_selection`'s reservation arithmetic and the fan
  radius), plus `commit_gallery`'s `have_authored` / `max_available` /
  `AUTHORED_ONLY` branch and the `BARE WALL` log. **All repoint to
  `authored_ready_depth`.**
- `disk_in_use` — not a symbol with callers: a stack-local `bool
  disk_in_use[256]` declared **twice**, in `load_authored_textures` and in
  `rotate_authored_staging`, as two unrelated automatic arrays. Both go.

### G4 — ATRIUM COHABITATION · CLEAR OF THE CAMPAIGN STOP

**The atrium does not exist.** `ATTIC_ATRIUM` D1 (`0f138fed`, "the entrance's
hang, deleted whole") removed 479 lines from `gallery.hpp`; D2 (`bd1e444b`,
"the partition, deleted; one manifest, one collection") removed 139 more.
Every one of the 26 case-insensitive `atrium` matches surviving in
`gallery.hpp` is a **comment**. Recipe: `grep -n -i "atrium"
src/cartridges/the_board/bodies/gallery.hpp` — read each hit for its line
kind.

The gate's question was: can the ring own all 32 layers with the atrium as a
transient borrower, or does the atrium hold staging residency? **Neither.
There is no atrium to partition against.** The ring owns all 32 layers
outright, no tail reserved, and R8 ("the atrium is out-of-band") is true in
the strongest available sense.

Evidence, each checked:

- `authored_record_for_disk` — **zero callers, zero declarations.** Its
  historical caller lived in the deleted poster stage; the definition was
  left orphaned. It is absent even from the impl-internal forward-declaration
  block.
- `DiskRange`, `authored_range_for`, `authored_cursor_for`, `atrium_first`,
  `atrium_disk_cursor`, `authored_loaded_lo/hi` — **zero hits** outside
  `docs/reference/ATTIC.md`.
- The manifest has no atrium key: `web_dist.py` writes
  `{"paintings": …, "music": …}` and `exhibition_manifest_onsuccess` parses
  `"paintings"` alone. The `+ M atrium` clause the order asks about is not in
  the sentence; `web/index.html` matches `/found\s+(\d+)\s+paintings/`.
- `place_atrium_poster`, `place_atrium_walls`, `rehang_atrium_memory`,
  `reseat_atrium_poster` — **zero hits** in `src/ tools/ web/`. They survive
  only as orphan cross-references inside comments.

**D4 RULED, and it is the one place this recon overrides the order.** U6 says
to keep `authored_record_for_disk` "because the atrium reads them". Nothing
reads it. Keeping it would also mean inventing a replacement predicate for
its `consumed` test — a larger deviation than deleting a function with no
callers. Under P7 the minimal form is deletion, and U6 takes it. Resurrect:
`git checkout f4e2d0dc -- src/cartridges/the_board/bodies/gallery.hpp`.
`shown_disk_index` **stays** and gains a live reader at U8.

Three orphans found and deliberately NOT touched (out of the campaign's
reach, recorded for a sweep): `gallery.hpp`'s `#include <iomanip>` whose
`std::fixed`/`std::setprecision` witness died with the entrance; the
`═══ THE ATRIUM'S HANG (ATRIUM_3) ═══` banner and its fourteen lines of prose,
which now introduce `clear_wall_paintings` alone; and
`src/console/organ_params.inc`'s last line, an `Atrium` banner with no rows
beneath it.

### G5 — THE RESERVATION FLOW · FLAG (maps onto a counter)

**No site reserves a named record.** `staging_reserved` is a plain count:
`place_gallery_from_selection` does `gs.staging_reserved += reserved`, and
`commit_gallery` releases the same value first thing, so every early return
releases it too. `GalleryPlacement::reserved_count` carries the number; no
DTO carries a staging index; commit re-derives its records from scratch. **So
the mapping the order wants is exact, and U4 lands.**

Four qualifications, none blocking:

- The counter is **one number over two pools** (snapshot + authored), netted
  in `gallery_available_staging`. Repointing `authored_hangable` to
  `authored_ready_depth` inside that function is the whole of U4's
  reservation change.
- `place_wall_paintings` **spends the pool without reading the counter** —
  it always did; the indoor path is not reserved for. Unchanged by the
  playlist.
- `GalleryState`'s comment claims up to `SPAWN_BUDGET_PER_FRAME` galleries
  can be outstanding. `spawn_selected_patches` drains place-then-commit
  inside one loop iteration, so **at most one** reservation is ever live.
  Stale prose, harmless, left.
- "Balanced by construction and cannot drift" has one guarded-unreachable
  leak: `place_entity_queue` tests `placementCount_ >= SPAWN_QUEUE_MAX`
  after `try_place` returned true, so a dropped `PlacementEntry` would strand
  its reservation until `teardown_gallery` zeroes it. Pre-existing, not this
  campaign's, recorded because the gate walked the path.

### G6 — THE MIRROR · FLAG

`MANIFEST_DEDUPE_CAP = 256` and its warning block are quoted whole in the U6
commit before removal; both die there with the array they mirror. The two
prose blocks in `web_dist.py` that name `rotate_authored_staging` and
`disk_idx < 256 && disk_in_use[disk_idx]` by symbol go with them — a warning
about a function that no longer exists is worse than no warning.

**R10's couplings, exactly:**

- `web/index.html` matches `/found\s+(\d+)\s+paintings/` against the C++
  sentence `[Authored] Scanned exhibition.json — found N paintings`, emitted
  by `exhibition_manifest_onsuccess`. **Untouched by every unit.**
- `classify()` also fires on `Controls:`, which `offer_controls_when_ready`
  emits behind `authored_staged_count >= OVERTURE_READY_FLOOR`. **Untouched.**
- `[Authored] Staged N/M images` keeps its exact shape at U1.
- Nothing outside C++ parses `[Authored] Rotated`, `[Authored] Pop`,
  `[WallPainting] …` or `[Gallery] …`.

**And the finding no gate was asked for (D9):** `gallery.hpp` is an enforced
sha256 pin in two ledgers. The campaign owes a regeneration commit —
`binding_ledger.py` → `command_census.py` → `mirror_census.py`, in that
order, because `MIRROR_LEDGER` pins `BINDING_LEDGER`'s output.

### G7 — PRICING REPEAT_1 · CLEAR (informational)

`GPUPaintingSlot` is declared once, in `realization/state.hpp`, `alignas(16)`
with a bare `static_assert(sizeof(GPUPaintingSlot) == 128, …)` — no growth-law
clause of the kind the mesh-params structs carry. Its WGSL twin is named
`UnifiedPaintingSlot`, and **the name asymmetry is why `mirror_census.py`
records the pair as UNRESOLVED**: the 128 is held on the WGSL side by prose
alone, unwitnessed by the layout calculator.

**THE PADDING, COUNTED. Twenty-four free bytes — six f32 slots — with zero
implicit compiler padding:**

| pad | offset | bytes |
|---|---|---|
| `float _pad0` | 92 | 4 |
| `float _pad1` | 108 | 4 |
| `float _pad2[4]` | 112 | 16 |
| | | **24 = 6 × f32** |

All six are offset-neutral: `_pad0` at 92 leaves `frame_color` at its
16-aligned 96; `_pad1` at 108 leaves `_pad2` at 112; `_pad2` is the tail and
splits into four scalars with the 128 stride intact under both C++ and WGSL
layout rules. `offsetof(GPUPaintingSlot, is_active) == 44` — the address
`deactivate_painting_slot` writes — is untouched by any carve. No site names
a pad field; three sites write all pads by whole-struct value-init
(`fill_slot_wall_frame`'s opening `s = {}` and the two teardown clears), so
a field carved from pad inherits zero-init free.

**A crossfade needs two — a source layer and a mix factor. It fits three
times over, with no growth past 128 and no field moved.**

**THE ONE THING THAT CHANGES THE IMPLEMENTATION, not the budget.** The slot
buffer is bound `@group(2) @binding(80) var<storage, read_write>
photo_painting_slots` and **the GPU writes it** —
`photo_painting_slots[i].position.y = ground + lift;`. `upload_painting_slot`
issues a full 128-byte write, so a CPU-driven ramp refreshed through it would
clobber the GPU's Y-correction every frame. REPEAT_1's crossfade must
therefore be GPU-driven, or drive its ramp through a partial
`queue.WriteBuffer` at the new field's `offsetof` — the idiom
`deactivate_painting_slot` already uses. Priced here so the campaign does not
discover it at the keyboard.

### CLOSED BY REPEAT_0

- **THE STARVATION READING (WALLS_1, "the readings WALLS_1 is not closed
  without", item 1).** It asked Jean to enter several indoor moods and watch
  `Placed N + M` fall while `across K walls` holds — the witness for the
  ROTATION account of the starvation. **The mechanism it was to witness is
  deleted at U5.** `rotate_authored_staging` invalidated consumed slots at
  teardown while `place_wall_paintings` hung the next room in the same frame;
  with refill moved to the pop, the vacated layer keeps its picture until its
  fetch lands (the WALLS_2 guarantee) and the next room hangs full
  immediately. There is nothing left to reproduce, and the `consumed`
  overload ruling the reading awaited is superseded by the field's deletion.
  Its replacement is VISUAL GATE 2 below, which reads the opposite outcome.

### WITNESSED AT THE CAMPAIGN'S TIP

- **THE RING'S ARITHMETIC, RUN RATHER THAN REASONED.** The pop, the ready
  depth, the ring size, the boot fill and the failure heal were transcribed
  character-for-character into a standalone model and exercised: 280 pops over
  a 57-painting manifest hang disk indices that ascend by exactly one and wrap
  at 57 (**visual gates 2 and 3, proved before Jean reaches the keyboard**);
  every manifest size from 1 to 33 fills the ring DENSE, `ready == ring`, and
  advances 200 pops with no stall; an empty manifest survives fill, depth, pop
  and slip with no division by zero; a total origin outage followed by
  recovery heals the ring to full; one bad file heals in one hop; and a
  pending head reads depth 0 while the pop declines without touching a deeper
  record (R3). The model is not the program — it cannot be, the real functions
  need a device — but every line of arithmetic in it is a copy of its
  original. It is what turned U1's ring-size decision from a judgement into a
  measurement: at the order's literal "fill layers 0..31", a five-painting
  manifest leaves 27 holes and the playlist stalls after five paintings.

- **L33'S STANDING WITNESS HOLDS.** The five files in `audit/` deleted, the
  five tools re-run, all five byte-identical.

- **AND THE WITNESS HAS A TRAP, PAID FOR ON THE FIRST RUN.** P18 says to
  restore a dirtied file from a sidecar taken at the moment of dirtying. It
  does not say WHERE the sidecar goes, and `audit/` is the wrong answer:
  `mirror_census.py` counts mentions across that directory, so five
  `*.md.bak` files sitting in it took a census row from 121 to 394 and the
  rebuild read as a genuine DIFFERS. The sidecar must live outside every
  directory the generators scan. Recorded rather than ruled — it is P18's
  letter working exactly as written and landing wrong, which is the shape a
  process law gets its next clause from.

### WHAT THE REFUTER FOUND (P3, and it earned its keep)

Six independent lenses over the landed diff — liveness, bounds, the indoor
plan/place contract, the outdoor commit and reservation, the deletions, and
the fetch lifecycle — each finding then handed to a separate skeptic told to
refute it. Eighteen findings raised, most refuted. **Four survived, all four
real, all four this campaign's own:**

- **`exhibition_layer` WAS NOT DELETED.** U6 announced the death of all three
  claim-triad fields and delivered two. The field stood as the struct's last
  member, with zero writers and zero readers, directly beneath its own
  obituary — the U6 diff shows it as an unchanged CONTEXT line between two
  deletions. All six lenses found it independently. A comment that lies about
  the line under it is worse than a leftover, and this is exactly the shape
  CLAUDE.md's "living matter only" exists to forbid. Fixed at U7c.
- **THE CURSOR'S READ-MODIFY-WRITE STRADDLED A CALL THAT ADVANCES IT.** Both
  `authored_pop` and `load_authored_textures` computed a cursor value, called
  into `load_authored_image_to_staging`, and only then wrote the cursor back.
  That call can reach `authored_fetch_release_slot` synchronously — a fetch
  that refuses to start unwinds through it — and the release advances the
  cursor to heal the layer. The write-back would discard that advance,
  handing one manifest index to two ring layers and rewinding the playlist.
  Fixed at U7c: the cursor advances before the call in both.
- **THE REFILL WAS A NO-OP RE-DOWNLOAD FOR ANY EXHIBITION OF 32 OR FEWER.**
  Where the manifest is no larger than the ring, head and cursor advance in
  lockstep, so the cursor always named the picture the popped layer was
  already showing: every hang re-fetched an image byte for byte to replace
  itself, and marked the layer `pending` for a round trip while doing it,
  shortening every subsequent row for nothing. Fixed at U7c by skipping the
  append when `shown_disk_index` already equals the cursor's index. A
  twelve-image exhibition now costs no network at all after its boot lap.
- **THE HEALING APPEND CLOSED AN UNBOUNDED RETRY CYCLE.** Recorded here as
  accepted when U7 landed; the refuter was right that accepting it was wrong.
  One request per failure against an origin that has gone away runs at the
  lane cap for the life of the session. Fixed at U7c with a budget that is
  only safe because `authored_pop` now heals a hole it trips over: the eager
  heal stands down after one lap of unbroken failure, and retries become
  demand-driven — bounded by the visitor rather than by the network — with no
  invalid head left for nothing to ask about.

Refuted, and worth recording because each was raised more than once: that a
room's own pops re-create the WALLS_2 starvation (they do not — the pops
behind the head are the LAST positions from it, so a 28-painting room leaves
depth 4, not 0); that the ring can index out of bounds or divide by zero on
an empty manifest (every path is guarded, and the model exercises it); and
that the pump's new ceiling can strand the queue (the next pop pumps, and the
pop now always has a reason to).

### STILL OPEN OUT OF REPEAT_0

- **`shown_disk_index` is a REPEAT_1 deletion candidate — and the atrium
  ruling it was pending is moot (G4).** After this campaign its readers are
  `authored_fetch_release_slot`'s claim fallback and U8's pop line. If
  REPEAT_1's crossfade logs the source layer instead, the field's last
  independent reader is the failure path, and `disk_index` alone may serve.
  Unblocked by REPEAT_1 opening.
- **`AUTHORED_FETCH_INFLIGHT_CAP` is 4 and its two regimes are unpriced
  (R9, D1).** The playlist makes a boot-time case for four lanes and a
  steady-state case for fewer, and the Firefox-ratchet justification for one
  lane is recorded in this file as suspended, not deleted. Unblocked by
  REPEAT_1, or by Firefox returning.
- **AFTER A TOTAL ORIGIN OUTAGE THE RING REFILLS ONE LAYER PER HANG ATTEMPT,
  and that is the accepted cost of U7b's storm bound.** The eager heal stands
  down after a lap of unbroken failure and the pop's own heal carries the ring
  from there, so a ring of holes comes back at roughly one layer per room
  rather than all at once: the first room after recovery hangs no authored
  paintings, the next one, and so on until the ring is dense again. It is a
  rare degraded mode, it is graceful, and it always completes. The cheap cure,
  if the eye ever meets it: a hole scrubber in `pump_authored_valve` — one
  frame verb, one hole re-asked per frame whenever the fetch queue is empty —
  which would refill the ring in thirty-two frames instead of thirty-two
  rooms. Priced at one round, not built. Unblocked by anyone actually
  watching an exhibition come back mid-session.
- **R3 MAKES BOOT ORDER VISIBLE, AND THIS IS THE ONE THING TO WATCH AT VISUAL
  GATE 1.** Ready depth is contiguous from the head, so a room can hang fewer
  authored paintings than there are ready records: if layer 0's fetch is the
  last of the boot's thirty-two to land, the depth reads 0 while thirty-one
  pictures sit in the ring. Selection could not have this problem, because it
  scanned. The exposure is bounded and small — the fill queues layers 0..31 in
  order, four lanes deep, so arrivals track start order closely, and the
  valve's holding pen drains FIFO — and it is confined to the first seconds,
  since after the first lap every layer has landed at least once. It is a
  DIRECT consequence of R3 ("never skip past a pending head to a deeper
  record"), which is a stamped ruling, so it was implemented as ruled rather
  than worked around. If gate 1 shows a thin first room, the fix is a ruling
  and not a patch: allow the pop to step over a pending head, and accept that
  the sequence is then a preference rather than a law. Unblocked by Jean's
  reading of gate 1.

- **The blocking question for REPEAT_1 is unchanged and is Jean's: the aspect
  fork** — letterbox, re-derive, or fixed cells with pictures breathing
  inside them. No handoff is written until that word is given.
