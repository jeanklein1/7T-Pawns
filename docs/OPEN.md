# OPEN — the register of open state
One line per item: what · origin (sha or doc) · what unblocks it.
This file is the ONLY home of open/parked state. When an item closes, its line dies.

## HOUSE_0 — THE HOUSE THE BOARD STANDS IN (on `claude/house-0`; Jean's gates open)

The website round after Jean's 7 Sep hand edits: every room connects, the
letterbox opens, the walls answer *info*. Site-only — the shell loses
three dead comments and nothing else; no `src/` file moves; `web_dist.py`
changes one comment. Rehearsed end to end in a scratch dist and driven in
headless Chromium (touch and mouse); the drives are in the handoff.

| ruling | where it lives now |
|---|---|
| Home is the page's one name, on both shells | `web/routes.json` — the `about` row is back, no `side`; the engine's Home pane lives again |
| The hero says nothing under it; it is a quiet door into its painting | `web/about/index.html` — `.hero-plate` and `#hero-title` are gone; the `<a>` stays |
| The doors carry the menu's names in the page's voice: *the board · gallery · writings* | `.doors` |
| The writings door takes a picture like the board's | `assets/about/writings.jpg` → `about_dist` `build_door_image(name)` (the old `build_world`, by name) |
| No statement, no footer (Jean's edit, honoured) | the template; `about_dist` reads the statement only if a header is there |
| Writings: a *Writings* menu on the left; the scroll and one page per text | `web/writings/index.html` (one template), `about_dist` `writing_slug` / `writings_menu_html` / `write_writings_page`; `web/menu.css` `.menu.left` |
| A text's address is its filename after the number | `writing_slug`: `01_the_mirror_in_the_sand.txt` → `/writings/the-mirror-in-the-sand/` |
| ZOOM_1 — the stage owns every gesture; the pan is clamped to the picture | `web/collection/index.html` — the two `.zone`s are gone; `pictureRect` / `clampAxis` / `zoomClamp` |
| No number on the gallery page: not *no. N*, not *1 / 13*, not *13 works* | `collection_dist` `section_markup` / `tile_markup`; the plate |
| *info* — a work's one line, behind a word, only where there is one | `assets/collection/<set>/PAINTING_<n>.txt` → `load_sets` → `data-info` → `plate()` |
| Closing a work opened from its own address lands on the grid | `pushed` in `show()` / `shut()` |
| The letterbox's sender is configuration | `functions/api/message.js` `MESSAGE_FROM` |
| The folders are the truth: paintings by number, a missing one a warning | `site.json` `hero` / `strip` as numbers; `about_dist` `find_master`; `collection_dist` warns on an orphan sidecar or an orphan `featured` |
| `assets/about/` holds `site.json` and the doors' pictures, nothing else | `.gitignore` (the two about lines gone), `README.txt` |
| `site.json` requires what is read: `email`, `hero`; `strip` optional | `load_site` — `authors` and `links` are gone |
| The gallery page's tab and share card say *gallery* | `<title>`, `og:title`, `description` |
| The bootstrap and `MAIN/` are attic | tag `attic/house-0-main-setup`; `docs/COPY.md` carries what they said that is still true |

**THE TWO DIAGNOSES, READ FROM THE LIVE SITE, NOT GUESSED.**

- **Write me answered `501 {"error":"unconfigured"}`** to a real POST: the
  function is deployed and `RESEND_API_KEY` / `MESSAGE_TO` are not set on
  the Pages project. No code was wrong. Jean's dashboard work (below); the
  page needs a redeploy after the variables land.
- **The pinch was dead outside the middle third.** ZOOM_0's step zones
  were absolutely-positioned SIBLINGS of the stage, 34% each side; a
  finger landing on one never reached the stage's pointer handlers, and
  on the zones the browser zoomed the page. Witnessed on the ZOOM_0 page
  in Chromium at 390 px: a pinch starting at 15%/85% of the width →
  `zoom.s = 1.000`; the same spread in the middle third → `1.500`; the
  live zone was **125 px wide**. On the HOUSE_0 page the outer-third
  pinch reads `1.286` (the spread's own ratio, 0.9/0.7).

**THE ONE THING THE ROUND CORRECTED WITHOUT ASKING.** A work opened from
its own `#w<n>` address — the home page's hero now lands exactly there —
closed with `history.back()`, which left the gallery for whatever page
came before. `pushed` records whether THIS opening pushed an entry; if it
did not, close replaces the state and shows the grid.

### What Jean does (in parallel; nothing in the tree waits on it)
1. resend.com — an account and an API key. Signing up **with**
   `jean@everexpandingboard.com` (Email Routing forwards the verification)
   lets Resend's unverified sender deliver there with no domain work.
   Otherwise verify the domain (three DNS records in Cloudflare; they do
   not collide with Email Routing's MX) and set `MESSAGE_FROM` to
   `the board <board@everexpandingboard.com>`.
2. Pages → `7t` → Settings → Environment variables → Production:
   `RESEND_API_KEY`, `MESSAGE_TO=jean@everexpandingboard.com` (and
   `MESSAGE_FROM` if the domain is verified). Then a deploy.
3. PowerShell: `Invoke-RestMethod -Method Post -Uri https://everexpandingboard.com/api/message -ContentType application/json -Body '{"message":"test"}'`
   → `ok True` and a mail. 501 = variables missing; 502 = Resend refused
   (the unverified sender only delivers to the account's own address).
4. `assets\about\writings.jpg` — whenever; absence costs one build line.
5. `del assets\about\PAINTING_*.jpeg` — the hero copies the retired
   bootstrap placed; the heroes are found in the collection now.
6. One line per painting he wants to say something about:
   `assets\collection\<set>\PAINTING_<n>.txt`.

### Residuals — HOUSE_0
- **Jean's browser is the visual gate**, the phone the real one: the pinch
  from anywhere on the picture; the tap in an outer third steps; the pan
  stops at the picture's edge; *info* opens and follows a step; the
  Writings menu on a phone; the doors' lowercase; the empty air where the
  statement was.
- **`/about/` is the address of the page called Home** (path, directory,
  builder, route id, `about.json`). Priced: ~12 files, one 301, no visible
  change; parked until the word is final. `/main` still forwards there.
- **The doctrine sentence has four homes** — the first writing's title, the
  gallery lede `h1`, the writings door blurb, one `<meta>` on the engine
  page. Copy, Jean's; one home when he chooses it.
- **The site never says his name** except in two `<meta>` descriptions.
  Copy, Jean's.
- **`00_text_page_doctrine.txt` lives at `/writings/text-page-doctrine/`.**
  A rename of the words after `00_` moves it; his call.
- **`/writings/<slug>/` pages carry no explicit `Cache-Control`** — Pages'
  default revalidates them (ETag); `web_dist.py`'s writer rules `/writings/`
  only. Untouched on purpose: that script is the engine's, and this branch
  keeps out of it. One line (`/writings/*`) if the default ever bites.
- **Two sets that hold the same number** would give `find_master` the first
  by set order; `collection_dist` allows the collision across folders and
  the hero would pick without saying which. Unlikely by construction
  (numbers are the works' own); left as authored.
- **The 404 page's menu is hand-kept** (`web_dist.py` `NOT_FOUND_PAGE`); its
  four links still name the four rooms correctly. A route added later must
  be added there by hand — DOORS_4's note stands.

## CLOSE_0 — THE CAMPAIGN CLOSES ON A WITNESS (on `claude/important-handoffs-9v1wus`)

The dist gate runs web_dist.py end to end on a fresh scratch tree every cascade — the
check three campaigns' witnesses never ran. The last counts retire; P19 names who deletes
the scratch ref (Jean, at the merge: `wip/important-handoffs-9v1wus` is outstanding).

The performance campaign (POSTCARD_0, PLATE_0, RAZOR_0, MIP_0, GATHER_0, SHUTTER_0,
DARKROOM_0/1, RIG_0, PRODUCTS_0, ATMOS_1, SETTLE_0/1, CLOSE_0) is closed. What it leaves
open, priced, in their own entries: the scrolling shadow cache, instance culling for the
table, the two-walls split and compressed textures, the browser-decode road (CLOSED in the
program's own language — no external-image copy in the vendored surface), the meter's
three windows on the laptop, and the hang's split if the [METER] darkroom row asks.

## PRODUCTS_0 — THE PRODUCTS GATE (on `claude/important-handoffs-9v1wus`; Jean's gates open)

The build's web products are read from CMakeLists.txt and witnessed in .gitignore, the
build presets, web_dist.py (ARTIFACTS, IMMUTABLE_PATHS, BOOT_SET or NOT_AT_BOOT) and
collection_gate.py. Found RED on the base with the four homes DARKROOM_1 had not visited;
GREEN after. Joins the cascade.

| ruling | where it lives now |
|---|---|
| The build is the authority for its products | `tools/gates/products_gate/run.py` |
| Four homes are witnesses, not authors | the gate's rows |
| Laziness is named once | `NOT_AT_BOOT` in `web_dist.py` (empty today) |

SETTLE_0: BOOT_SET is read by the first-visit sum and checked; NOT_AT_BOOT
exists, empty.
SETTLE_1: U1.7 read dist before dist existed (blocker, CC's audit, proven by
running both tools); the reader now sits before WROTE; organ_panel.js is a boot
fetch and counts; P19 written.

## ATMOS_1 — THE PIECE LEAVES THE PHONE'S HANDS (on `claude/important-handoffs-9v1wus`; Jean's gates open)

Hidden releases the soundtrack's media instead of pausing it, so a headphone button
cannot resume it with the page shut; visible re-attaches and seeks. ATMOS_0's grammar
stands; ATMOS_0 R's gesture fallback covers a refusal on return.

SETTLE_0: `attachMusic()` is the one home (three callers); the re-attach is
unconditional after entry; the seek survives a double-hide.

## ZOOM_0 — THE COLLECTION PAGE LEARNS THE PINCH (landed on master; Jean's gates open)

Five seams of the lightbox the page already had. One transform on `#big` —
`translate(x,y) scale(s)`, origin `0 0` — anchored so the point under the cursor
or the pinch midpoint stays put. Wheel zooms, drag pans, two pointers pinch,
double-tap toggles 1 ↔ 2.5, ceiling 6×. `s=1` is the lightbox exactly as it was;
any `s>1` hides the step zones (they would steal the pan) and Esc peels the zoom
before it closes. Every `show()` and `shut()` resets, so a step always lands
unzoomed. No library, ~80 lines of the page's own vanilla, CSS in a style block
beside the stage. Site-only — no engine artifact, and `collection_gate` PASS.

**THE ORDER'S DRAFT WOULD HAVE KILLED THE PAGE, and this is the one repair CC
made without asking.** Its script block opened with
`const stage = view.querySelector('.stage');` — but `stage` is already a
top-level `const` in that same and only `<script>`, bound above `restRect`, which
reads it. A second `const` at the same scope is not a shadow and not a warning:
it is `SyntaxError: Identifier 'stage' has already been declared`, thrown at
PARSE time, which discards the whole block. The grid, the filters, the lightbox,
the history wiring — all of it, dead, on the page that is the collection. The
landed seam reuses the existing binding and says so in place. Shown both ways
with `node --check` on the extracted block: the draft throws the SyntaxError,
what landed parses.

**Two smaller corrections in the same seam.**
- The draft's banner said that above `s=1` "arrows pan instead". They do not, and
  nothing in the five seams makes them: `ArrowLeft`/`ArrowRight` still call
  `show()`, which resets — which is what the order's own reading list asks for
  ("arrow-stepping always lands unzoomed"). The banner now says what the code
  does. The drag pans; the arrows step.
- `zoomClamp()` was defined and never called, and could not have been used as
  written — it carries `+ (big.offsetLeft ? 0 : 0)`, which is 0 either way, and a
  branch that assigns `zoom.y` to itself. Dead on arrival, in the one file that
  ships to the site, against the tree's living-matter rule. Struck; the attic has
  it. If a clamp is wanted (today nothing stops a pan from carrying the picture
  off the stage), it should be written against the transformed rect and CALLED
  from `zoomTo` and the pan arm — a small, separate piece of work.

**Witnessed in headless Chromium, not reasoned about.** Driven over CDP through
RIG_0's own `Rig`, on the page with three synthetic works injected (the repo
carries no collection assets):

| reading | result |
|---|---|
| page load | 0 errors; `zoomTo`/`zoomReset`/`zoomApply` defined |
| one wheel notch | `s` 1 → 1.2; ceiling holds at exactly 6 |
| anchor invariant | drift **0** in the transform's own frame (`big.offsetLeft`) |
| anchor vs. true layout origin | **0.028 image px = 0.070 on-screen px** at s = 2.488, five notches |
| drag pan | exactly the pointer delta (+40, −25); **0,0 while `s=1`** |
| pinch | span ×2 → `s=2`; ×3 → `s=3`, about the midpoint |
| double-tap | 1 → 2.5 → 1 |
| zones while zoomed | `display: none`; a `.zone.next` click did **not** step |
| Esc | first peels (`s=1`, overlay open, transform cleared), second closes |
| arrow while zoomed at 4.30 | steps, and lands at `s=1` |
| wheeling down | floors at exactly 1 and clears the transform |

The one non-zero number has a named cause: `big.offsetLeft` is an integer and
the stage gutter is `clamp(18px, 3.2vw, 52px)` — 24.953125 px at the tested
width — so the anchor is exact in its own frame and off by that 0.047 px of
rounding against the real one. 0.07 of a CSS pixel at the ceiling is below
anything an eye or a finger can find; left as authored.

### Residuals — ZOOM_0
- **Jean's browser is the visual gate**, and the phone is the real one: pinch,
  pan, double-tap out, the edge zones must not steal a pan, Esc twice.
- **No pan clamp.** A drag can carry the picture off the stage; `zoomReset` on
  every step and close is what limits the damage today. Priced above.
- **`setPointerCapture` throws for a pointer id with no live pointer.** Harmless
  in a browser (ids are real); it is why the witness above stubs it. Worth
  knowing if the seam is ever driven from a test.


## SKYGLASS_0 — A PORTAL WEARS ITS DESTINATION'S SKY (landed on master; Jean's gates open)

The portal palette in `contracts/mood_constants.hpp` stops being seven authored
hues and becomes, for the open worlds, a value-stamp of the sky behind the door.
Outdoor rows carry their mood's drawn-regime `clear_color` CENTRE with the
provenance named in-row; the two rooms are authored olives (no sky to wear); the
atrium keeps white — the way home, not an open world; the back-portal goes blue
to cement.

| row | value | where it came from |
|---|---|---|
| mood 0 open_sunset | `0.95, 0.70, 0.45` | `ATMOS_SUNSET.regime[0].clear_color` |
| mood 1 indoor_flat | `0.42, 0.44, 0.16` | authored olive |
| mood 2 indoor_vault | `0.56, 0.53, 0.11` | authored olive, yellow forward |
| mood 3 finite_outdoor | `0.85, 0.78, 0.72` | `ATMOS_FINITE_DAY.regime[0].clear_color` |
| mood 4 open_night | `0.02, 0.03, 0.06` | `ATMOS_NIGHT.regime[0].clear_color` (the drawn row) |
| mood 5 open_noon | `0.45, 0.68, 0.95` | `ATMOS_NOON.regime[0].clear_color` (the drawn row) |
| mood 6 atrium | `1.00, 1.00, 1.00` | ATRIUM_1, kept |
| back-portal | `0.62, 0.61, 0.57` | cement (was blue) |

**All four stamps were checked against `contracts/spine_state.hpp`, not taken.**
Each of the four outdoor values equals its `ATMOS_*.regime[0].clear_color` triple
exactly, and the named flip for the night door — the fog centre
`{0.11, 0.12, 0.15}` — is `ATMOS_NIGHT.regime[0].fog_color`, also exact. No
`static_assert` in `spine_state.hpp` pins the palette (the drift asserts there
pin mood ids and Atmosphere columns), so nothing but this register and the
in-row provenance holds the twins together.

**The stamp is a centre; two of the four skies draw around it.** Night's
`clear_color_spread` is 0.25 and noon's is 0.08, so the sky a visitor stands
under is a per-seed draw about the value the door wears. Sunset and
finite_outdoor have spread 0 and match exactly. This is what "a twin by stamp"
buys and what it does not: the door names the mood's sky, not the seed's.

**Deriving is not merely forbidden — it is a cycle.** The banner says an
include edge this header's own banner forbids. Stronger: `spine_state.hpp`
already `#include`s `mood_constants.hpp` (for `MOOD_COUNT`, the ids and
`PortalDestination`), so the reverse edge would be circular. The stamp is the
only shape available, which is why the re-stamp instruction sits in the row.

### Residuals — SKYGLASS_0
- **Jean's eye is the gate.** The night portal at `0.02, 0.03, 0.06` against
  night ground is the judgment call; the lift to the fog centre is one line, and
  the row names it.
- **If a sky moves, re-stamp its row.** Nothing enforces this. If it wants
  enforcing, the cheapest witness is a row in the console gate comparing the four
  triples to `MOOD_TABLE[...].atmos.regime[0].clear_color` — a `static_assert` in
  `spine_state.hpp`, where both are already in scope, would cost one line and
  close the gap. Not built: it is a gate, and a gate needs a row in CLAUDE.md and
  a shown perturbation.


## DARKROOM_1 — THE DARKROOM WORKER (landed on master; Jean's gates open)

A painting's develop — decode, pad, swap, chain — left the frame: a Web Worker runs a
second tiny wasm built from the same core/develop.hpp the program compiles, and the
program hangs one developed painting per frame with a WriteTexture. Byte-identical by
construction and by harness (108 cases). No shared memory, no headers, no pthreads.

| ruling | where it lives now |
|---|---|
| One home for developing a painting | `src/core/develop.hpp` |
| The worker develops; the frame hangs | `src/darkroom/darkroom.cpp`, `web/darkroom_worker.js`, `window.T7_DARKROOM`, `pump_authored_valve` |
| The uploader is one function | `state.hpp` `upload_developed` |
| Failure takes the road it always took | `t7_darkroom_done` (width 0), the main-thread arm |
| The build exports what the bridge needs | `CMakeLists.txt` (HEAPU8, _malloc, _free; the darkroom target) |
| The measured row | `[METER] darkroom develop=… hang=…` |

### Residuals — DARKROOM_1
- **The hang is still ~5 MiB of WriteTexture in one frame.** If the meter's `hang=` reads
  more than a few ms on the laptop, split it: level 0 one frame, the ten levels the next
  (the layer stays `pending` until the last write). Not built until the row asks.
- **Development now starts on arrival, before first present** (the worker cannot block
  the thread); the hang still waits for first present. If `authored6` moves earlier on
  the boot card, that is this round's gift to AUBADE and worth a line there.
- **Route A stays priced** (browser decode + GPU chain) as the road if a platform ever
  refuses Workers — none does — or if the ±1 LSB ruling changes.
- **Byte identity witnessed twice, independently.** CC ran its own harness against the
  tree's real `core/develop.hpp` and the base code read out of git at `f9642df6`,
  comparing **level 0 and all ten chain levels, with and without the BGRA swap**:
  **108 cases, 0 bytes differ** — the handoff's own figure, reproduced rather than
  taken. The set: twelve degenerate and exact sizes with ramp pixels (1024x1024,
  1024x683, 683x1024, 1023x1023, 512x512, 511x997, 1024x2, 1024x1, 1x1024, 7x1024,
  2x3, 1x1), forty random sizes ≤ 1024 with random pixels, and the above-cap bilinear
  arm (2048x1536, 4000x3000), each with both texel orders.
- **The developed buffers' lifetime was traced end to end**, since the shell allocates
  them and the program frees them: `_malloc` in `T7_DARKROOM.done`, `std::free` on every
  failure path of `t7_darkroom_done`, and `std::free` in the drain immediately after
  `upload_developed`. No leak on any path the pump runs.
- **A worker reply cannot outlive its world's claim, and the reason is already law.**
  `teardown_gallery` keeps the authored staging array intact across a world boundary by
  design (WALLS_2 — a layer keeps its picture until its own fetch lands), so a reply
  arriving after a teardown writes into a staging layer still claimed by the same fetch.
  This was checked because the three state readbacks all carry generation guards for the
  opposite reason; the darkroom needs none.
- **`-sFILESYSTEM=0` on the darkroom target is the one flag to watch**, as the handoff
  says. `STBI_NO_STDIO` is defined nowhere in the tree, so stb's implementation TU still
  compiles its `FILE*` path (`stb_image.h` names `FILE*`/`fopen` 32 times) even though
  `darkroom.cpp` calls only `stbi_load_from_memory`. If the link refuses, the handoff's
  remedy is to drop the flag; the tighter one is to keep it and add `STBI_NO_STDIO` as a
  compile definition on `the_board_darkroom`, removing the stdio path at the source
  instead of re-admitting the filesystem. Jean's build decides which.

## RIG_0 — THE TINT ARM LANDS, AND THE ROUTE A PROBE (landed on master; Jean's gates open)

GATHER_0's ruling 6 is a tool now: `tools/gates/tint_arm/`, needing python3 and a
Chromium and nothing else. And DARKROOM_0's Route A prerequisite has its verdict —
OPEN in the browser, and closed to the program, which is the more useful half.

| ruling | where it lives now |
|---|---|
| The Tint arm is the shader gate, on any machine with a Chromium | `tools/gates/tint_arm/tint_arm.py`; `T7_TINT` + `T7_CHROMIUM` |
| The traps are written down, not remembered | `tools/gates/tint_arm/README.md` |
| A gate that cannot lose is a report — two perturbations, both blessed by naga | the README's own section, re-run against the landed tool |
| The Route A probe is a probe, not a gate | `tools/gates/tint_arm/probe_copy_external.py` |

### Residuals — RIG_0

- **THE PROBE'S VERDICT, both halves.** *Browser:* **OPEN** —
  `copyExternalImageToTexture` takes an ImageBitmap into an `rgba8unorm` texture in the
  rig, and the probe reads it back through a buffer and compares 64x64 texels against the
  pattern it drew: **0 mismatches**. OPEN means the pixels were checked, not that no error
  fired. *Program:* **CLOSED** — the vendored emdawnwebgpu C binding exposes exactly
  `wgpuQueueAddRef`, `OnSubmittedWorkDone`, `Release`, `SetLabel`, `Submit`, `WriteBuffer`
  and `WriteTexture`, and **no external-image copy of any kind**. So Route A as DARKROOM_0
  sketched it has no C++ caller available: the copy would have to be issued from JS against
  a texture the program owns — a larger seam than the one blit pipeline the residual
  priced, and a fact that belongs in any Route A handoff before it is written.
- **The arm is not wired into `dist.py` or any automatic run**, deliberately. It lights
  only when `T7_TINT` names it, exactly as `wgsl_gate.py` already ruled, so a machine
  without a Chromium keeps the naga arm and an honest DORMANT line.
- **The rig is one Chromium per invocation** (launch, drive, terminate) — a few seconds.
  Fine for a gate; if a future round wants it per-shader in a loop, the `Rig` class in
  `tint_arm.py` is already the reusable half.
- **`T7_CHROMIUM` is never guessed.** No default paths are probed, because a gate that
  silently finds *a* browser is a gate that silently proves something about the wrong one.

## DARKROOM_0 — THE PICTURES DEVELOP OFF THE FRAME (landed on master; Jean's gates open)

The stutter that survived SHUTTER_0 is a painting's arrival: one 1024² decode + pad +
chain + upload per frame on the main thread, in batches, each unit longer than a frame.
This round removes the pad's share byte for byte (a bilinear at scale 1 is a copy).
The decode is the floor of the main-thread route; the next round moves it off.

| ruling | where it lives now |
|---|---|
| At scale 1 the bilinear is a copy, byte for byte | `gallery.hpp` `authored_stage_decoded_image` |
| Scheduling cannot fix a unit larger than a frame | this entry; `pump_authored_valve` stays one per frame |

### Residuals — DARKROOM_0 (the road, priced)
- **Route A — the browser develops the picture.** `createImageBitmap` (off-thread,
  hardware) then `copyExternalImageToTexture` into the authored staging's level 0: no
  CPU pixels on the main thread, no WriteTexture, no swap. The chain and the edge
  replication then move to the GPU: one blit pipeline (fullscreen triangle, level k →
  k+1, clamped to the picture's crop so the pad becomes the replicated edge for free) —
  the same blit MIP_0 priced for photographs, which it would also serve. Cost: a pipeline,
  a layout, seats, a census enrollment. Integrity note: the browser's JPEG decoder is not
  stb; pixels may differ by ±1 LSB where IDCT and chroma upsampling round differently.
  FIRST STEP, before any handoff: prove `copyExternalImageToTexture` reaches a Dawn
  texture from emdawnwebgpu in CC's Chromium rig (the Tint arm's), one afternoon.
- **Route B — a thread develops it.** Emscripten pthreads: stb decode, pad, swap and chain
  in a worker; the main thread keeps only the WriteTexture (level 0 one frame, the ten
  small levels the next, ~4 ms each). Bit-identical to today. Cost: `-pthread` and a
  pool in the build, COOP/COEP headers on Pages (`web_dist.py` prints "HEADERS: none"
  today and knows where they go), an iOS witness (SharedArrayBuffer needs the headers),
  and the memory-growth interplay. Integrity keeps every byte; the price is the build.
- Recommendation: B if the ±1 LSB is a ruling you would refuse; A otherwise, because it
  also retires the CPU pad and the WriteTexture, and it is the WebGPU-native shape.
- A per-pump timing row in the meter build (the `aubade_stb` accumulator stops at first
  present) would make the next round measured rather than estimated; three lines.
- **Ruling 1 is proven, not estimated — the round's one addition.** The handoff offers byte
  identity as arithmetic and says the sandbox cannot witness it ("the sandbox can't run the
  wasm"). It does not need to: the pad is pure CPU code, so both arms were lifted verbatim
  from `authored_stage_decoded_image` into a harness and compiled natively. **414 cases,
  0 bytes differing** — every value 0..255 present at 1024x1024, 1024x683, 683x1024,
  1023x1023, 511x997 and the degenerate 1x1, 1x1024, 1024x1, 1024x2, 2x3, 7x1024; 400
  random sizes <= 1024 with random pixels; and a control ABOVE the cap (2048x1536,
  1025x1024, 4000x3000) where the new arm delegates to the old one and must agree, which it
  does. The `>= 1.0f` test is safe because `if (scale > 1.0f) scale = 1.0f;` sits above it,
  so >= 1 means exactly 1; and at exactly 1, `dst_w == width`, so the row memcpy copies the
  whole source row and nothing past it. Jean's gate 3 (byte identity) is therefore already
  answered for the pad — what his eye is still needed for is that no OTHER stage moved.
- **CC's Chromium rig, which Route A's first step asks for, exists.** GATHER_0 lit the WGSL
  gate's Tint arm through Chromium's own Dawn (WebGPU `createShaderModule` over CDP,
  SwiftShader, the page on a `file://` origin). The same rig reaches a real `GPUDevice`, so
  the Route A probe the handoff prices at "one afternoon" — prove
  `copyExternalImageToTexture` reaches a Dawn texture — is runnable here without a Dawn
  checkout. The rig is not in the tree (GATHER_0's residual explains why); landing it as
  `tools/gates/tint_arm/` would make both this probe and ruling 6 repeatable.

## SHUTTER_0 — THE SPACING FLOOR (landed on master; Jean's gates open)

A wall-clock floor between photographer captures (5 s, Jean's number). The distance
trigger — the travelogue — stays; a walk never meets the floor, a ride stops firing
bursts of 1024² captures inside a second. Composes after PURSE_0's headroom gate and
before its defer clock, so bounded starvation still measures headroom, not spacing.

| ruling | where it lives now |
|---|---|
| The floor is between captures, not triggers | `gallery.hpp` `MIN_CAPTURE_SPACING_S`, the gate in `update_photographer` |
| Spacing before the defer clock arms | the gate's position above `defer_since` |
| Named constant, not a dial (PURSE_0's reasoning) | the config banner |

### Residuals — SHUTTER_0
- The per-shot cost (1024² since PLATE_0) is the other axis; the 512-into-the-layer arm
  stays registered under PLATE_0 for a floor phone that asks. This round spent the clock,
  not the pixels.
- If Jean's ride still stutters with the floor in, the remaining suspects are not the
  photographer: re-open the four-experiment ladder (cap, mask, taps, [PRESENT]) with the
  meter — `python tools/web_dist.py --lab`, then `npx wrangler pages dev dist`.
- **`last_capture_s` landed as a `double`, not the handoff's `float` — the round's one
  deviation, in its own commit (U1b) so it can be dropped alone.** It is differenced
  against `TimeState::seconds`, which is exactly what PLUMB_0 B1 governs, and every other
  such stamp in the tree is a double: `spawn_engine`'s `lastCensusDump_`, `cartridge`'s
  `rosterGolResidueDump_` and `lastCardTick_`, `floaters`' `last_alloc_time` — and
  `defer_since` **two lines below it in the same struct**, whose comment reads "PLUMB_0 B1
  — differenced against TimeState::seconds". The handoff's was the only float one. This is
  not PLUMB_0's float-accumulator stall (a stamp does not accumulate) but the same family:
  a float stamp quantises at the wall clock's magnitude. Measured against the 5 s floor —
  ulp 0.031 s at four days (PLUMB_0's own *"four days is not a hypothetical"*), 0.25 s at
  thirty, 1.0 s at six months, 4.0 s at 485 days, where a five-second floor stops being
  measurable at all. The piece is permanently hosted. Behaviour is identical on any session
  short enough for float to have been fine.
- **The rehearsed diffstat said three ledgers; it is four.** `ORGAN.md` moves too — its four
  `bodies/gallery.hpp` constexpr-derivation cites shift 555→566. That is +11, exactly the
  lines U1.1 adds at `PhotographerCaptureConfig` (line 156), far above the cited region, so
  it is the handoff's own edit and not the deviation above (U1b's lines sit below the
  cites and move nothing). Noted only so the next round's diffstat expectation is right.
- **Ruling 2 was checked against the function, not assumed.** `cumulative_distance` accrues
  *before* the pending-shots block, so the new early return leaves "distance keeps accruing;
  nothing else advances" true. And because `now` only increases while `last_capture_s` is
  fixed, spacing once satisfied can never un-satisfy — so `defer_since` cannot be left armed
  by a frame that was still inside the floor, and the bounded-starvation ceiling still
  measures only headroom. The floor and the ceiling do not race: 5 s floor, 4 s ceiling, and
  the ceiling's clock starts after the floor is met.

## GATHER_0 — THE GATE BEFORE THE TAPS; NINE GATHERS (landed on master; Jean's gates open)

The spot shadow is sampled only where attenuation × cone × facing is non-zero (it
always multiplied the result); both 4×4 PCF kernels read their 6×6 window through nine
gathers instead of sixteen bilinear taps, exact in the model. No pixel leaves.

| ruling | where it lives now |
|---|---|
| The gate before the taps (0 × shadow) | `world.wgsl` `calc_spot_light` |
| A sample under a per-fragment branch takes its level explicitly | `sample_spot_shadow_pcf` (U1), then the gathers (U2) |
| Nine gathers reconstruct sixteen taps: separable tent weights on texel corners | `pcf_window`, `pcf_quad`, both kernels |
| The 4-tap arm stays: nothing to save there | `sample_shadow_pcf`, the `shadow_pcf_taps == 4` arm |
| The Tint arm is the shader gate from now on | `tools/wgsl_gate.py` + `T7_TINT` → the Dawn checkout's tint |

### Residuals — GATHER_0
- **The penumbra may differ by under 1/256** from the fixed-point bilinear the hardware
  used to blend — under one 8-bit step of the lit color. The screenshot diff at a shadow
  edge is the witness; a visible difference is a finding, not a tolerance.
- **The sun's gate could tighten** (e.g. skip the kernel where the receiver is beyond the
  map's fade) now that no implicit derivative is left in it; not built — the meter's
  outdoor window decides whether the fade band is worth a branch.
- **The shadow pass encodes its draws each frame** (the main pass is a recorded bundle;
  the shadow pass is not) — a CPU/JS-bridge cost, not a GPU one. Priced: the sun arm as one
  bundle, the atlas arm as one bundle per tile between its scissors (bundles inherit
  scissor state and may not set it). Worth building only if `[PRESENT]` ever names the
  CPU; the machinery is BUNDLE_1's.
- **The orbs run two compute passes a frame** (copy_prev, dynamics); a ping-pong would
  retire the copy and one pass boundary. Small everywhere; measurable only on glass.
- **THE TINT ARM IS LIT, AND IT IS PORTABLE — this round's finding.** Ruling 6 points at
  a Dawn checkout on Jean's machine. It does not need one: **Chromium already contains
  Dawn and Tint**, and a WebGPU `createShaderModule` + `getCompilationInfo()` is Tint's
  own verdict on the real module. The arm was lit here against Chromium 141 on
  SwiftShader (`--enable-unsafe-swiftshader`, driven over CDP, the page on a `file://`
  origin so `navigator.gpu` is present — on `about:blank` it is not, which costs an hour
  if unknown), wrapped in a two-line shell script that satisfies `T7_TINT`'s contract
  (exit 0 and print no "error" substring when accepted). `python3 tools/wgsl_gate.py`
  then reads `[gate] tint arm PASS` instead of DORMANT.
- **AND IT LOSES WHERE naga CANNOT SEE — twice, proven.** Ruling 2 is not precautionary,
  it is required: with the new gate and the OLD implicit-derivative sample, Tint rejects —
  *"'textureSampleCompare' must only be called from uniform control flow"*, chained
  `sample_spot_shadow_pcf` → `calc_spot_light` → `shade_lit`; with the explicit level it
  accepts. naga passes both. **MIP_0's open question is also closed by the same arm:**
  perturbing `sample_exhibition` so its derivatives sit inside the non-uniform branch,
  Tint rejects with *"'dpdx' must only be called from uniform control flow"* and names
  `parameter 'kind' of 'sample_exhibition'` — while naga prints "Validation successful"
  on the identical file. MIP_0's ruling 3 was load-bearing, and MIP_0 as landed is
  correct. The arm accepts the shipped module.
- **The gather kernels clear Tint too**: `textureGatherCompare` on the depth textures and
  the dynamic indexing of `win.wx`/`win.wy` are accepted, so U2's HALT condition did not
  fire. The model reproduces exactly — max |pcf16 − pcf9| = 3.3306690738754696e-16 over
  the handoff's 20,000 positions, and 0.0 over an adversarial grid of exact texel centres,
  corners, half-texels and far-beyond-edge positions.
- **The wrapper is not in the tree. — CLOSED at RIG_0**, which landed it as
  `tools/gates/tint_arm/` and made it smaller on the way in: no node and no Playwright
  (the DevTools protocol is spoken by a standard-library WebSocket client inside the
  tool), and **no display** — which corrects the recipe below. Both `--headless=new` and
  the old headless obtain a device on a `file://` page; headless was never the blocker,
  `about:blank` was. The two perturbations are in its README and both were re-run against
  the landed tool.

## MIP_0 — THE EXHIBITION'S CHAIN (landed on master; Jean's gates open)

Paintings sample through an eleven-level chain the CPU builds at upload; photographs
sample level 0 as before; the two are told apart by the slot's content_source. No new
pass, pipeline or binding. +96 MiB where the chain lives.

| ruling | where it lives now |
|---|---|
| The chain is built where the texels already pass through the CPU | `state.hpp` `upload_authored_painting`; `Dim::PAINTING_MIP_LEVELS` |
| A photograph has no chain and asks for none — level 0, by kind | `world.wgsl` `sample_exhibition`, `CONTENT_SOURCE_SNAPSHOT` (L3 mirror of `ContentSource::SNAPSHOT`) |
| Derivatives in uniform flow, the sample under the branch | `sample_exhibition` (`dpdx`/`dpdy` → `textureSampleGrad`) |
| The pad is the edge, replicated | `gallery.hpp` `authored_stage_decoded_image` |
| The promotion copies as many levels as its source has | `state.hpp` `promote_to_exhibition(…, levels)`; `gallery.hpp` `drain_gallery_promotions` |
| The estate census reads the lambda's fourth argument | `tools/binding_gen.py` |

### Residuals — MIP_0
- **Photographs still shimmer at a distance** — unchanged from before, by ruling 2. A
  chain for them is one blit pass (fullscreen triangle, level k → k+1, per promoted
  layer): a pipeline, a layout, seats, a census enrollment. Priced; built when the wall's
  seven photographs are worth a mechanism.
- **+96 MiB of estate** (exhibition 213.3, authored staging 170.7). The phones' residency
  row is this round's gate as it was PLATE_0's; the lever if one refuses is the chain on
  the exhibition only, with the levels written at promotion from a CPU copy the staging
  record retains (same bytes, moved from GPU to heap).
- **The CPU decode grows by a third** (the box means); the `authored6` mark is still the
  witness, and the browser-decode road registered at PLATE_0 still answers it.
- **The WGSL gate cannot witness ruling 3, and that is this round's finding. — CLOSED at
  GATHER_0**, which lit the Tint arm and got the verdict this entry could not: perturbed so
  the derivatives sit inside the non-uniform branch, Tint rejects with *"'dpdx' must only be
  called from uniform control flow"*, naming `parameter 'kind' of 'sample_exhibition'`,
  while naga prints "Validation successful" on the identical file. So ruling 3 was
  load-bearing rather than merely careful, and MIP_0 as landed is correct — the arm accepts
  the shipped module. The finding below stands as written, and this is how it ended.** MIP_0
  hands CC the gate as the round's own and rests the uniformity ruling on it. Run both
  ways on naga-cli 30.0.1 — as written, and PERTURBED with the derivatives moved INSIDE
  the non-uniform branch — the gate reads PASS both times. Calibrated with a textbook
  violation in a minimal module (`textureSample` under a non-uniform `if`): naga prints
  "Validation successful". **naga does no derivative-uniformity analysis in this
  configuration**, so a PASS proves parse, scope and type, not ruling 3. The gate's own
  tint arm is what would witness it — "[gate] tint arm DORMANT" — and Tint is Dawn's
  compiler, so Jean's emcc build is this ruling's real gate; no tint binary is vendored
  (only emdawnwebgpu headers). It also narrows gate row 6: naga is Firefox's compiler, so
  Firefox witnesses the syntax and typing of `textureSampleGrad` on an array, not the
  uniformity. THE CODE IS RIGHT REGARDLESS, verified structurally instead: uniformity is a
  property of the CALL CONTEXT, and both sites are uniform — `sample_exhibition` is the
  first statement of `gallery_frame_fs`, and in `wall_painting_canvas_fs` it sits at
  function-body depth after `if (in.is_canvas == 0u) { discard; }`, where `discard`
  demotes to helper without splitting control flow.
- **The sampler's label had a schema row the handoff did not name.** M1.6 renames it to
  *(trilinear, clamp)*; `tools/binding_schema.py` carried *(bilinear, clamp)* and
  `binding_gen --check` read `RESOURCES ... MISMATCH`, falsifying the rehearsal's "every
  census row PASS". `binding_schema.py` appears nowhere in MIP_0. The schema now follows
  the tree — the label is descriptive by house convention and the sampler really is
  trilinear — and the label is user-visible on the boot card's estate leaderboard.

## RAZOR_0 — THE BACKGROUND DRAWS LAST (landed on master; Jean's gates open)

The terrain plan moved from the head of the opaque list to its end, after the table
and the gallery fork: covered terrain fragments now fail the depth test before they
are shaded. Pixel-identical; the meter's `main_pass` row is the witness.

| ruling | where it lives now |
|---|---|
| Order among opaques is immaterial to the picture, material to the cost; the background draws last | `render_passes.hpp` `encode_main_opaque` (one home for the bundle and the direct arm) |
| A bind the plan lent the table is stated, not inherited (P-seq caught it) | `encode_main_opaque`, the `SetBindGroup(2, scene_state_group())` before the table |

### Residuals — RAZOR_0
- The snapshot pass (R6) keeps its own draw list and order; it runs once per ceiling and
  was not touched. If the meter ever prices it, the same ruling applies there.
- A depth pre-pass would finish what this starts (every pixel shaded exactly once) at the
  price of a pass and a pipeline variant per material; not built — the overdraw the
  meter shows after this round decides whether it is worth a mechanism.
- **Ruling 2 was verified, not assumed.** Both call arms of `encode_main_opaque` bind
  groups 0, 1 and 3 and never 2, so the plan really was the table's only source of the
  scene-state bind. Perturbed on a sidecar by removing the stated bind, P-seq FAILS with
  the handoff's own message and names all ten draws of the table — arch, blade, cactus,
  column, monolith, palm, pawn, ribbon, shell, sphere. The bind is load-bearing and the
  gate can lose.

## PLATE_0 — THE WALL AT 1024 (landed on master; Jean's gates open)

Every painting array doubles its edge: the constant that PORT_5b cut to fit an old
laptop reverses on Jean's stamp — the laptop left the audience, the stub masters are
being replaced at this size, and the postcard ships the wall, so the wall's
resolution is the program's public face. One constant, one dist cap, comment truth
everywhere the old number was written down; and the take badge's doorstep pulled in.

| ruling | where it lives now |
|---|---|
| `PAINTING_RESOLUTION` 512 → 1024; PORT_5b's reason kept as history beside it | `state.hpp` (the constant's banner carries both stamps) |
| +339 MiB (+318 at MSAA 1): Exhibition 160, each staging 128, offscreen 4/16/16 | the boot card's `[GPU Budget]` rows — the witness |
| Residency's witness is the floor, not a laptop | Jean's gate rows: Pixel and iPhone boot beside open tabs |
| `PAINTING_CAP` 1024, `PAINTING_QUALITY` 88 (the postcard re-encodes at 0.92; q82 would survive two generations) | `tools/web_dist.py`; the cap gate enforces it on what lands |
| Masters never upscale: an under-1024 master occupies less of its layer | the loader's `scale > 1 → 1` clamp, unchanged |
| The badge's doorstep: reach 1.6 ×/min 6 wu, slack 0.25 — contains the pilot's stand-off at every span | `bodies/gallery.hpp` `TAKE_*` |

### Residuals — PLATE_0

- **No mipmaps on the painting arrays** (`mipLevelCount` 1), so a far painting
  shimmers, and at 1024 it shimmers harder than the 512 softness used to hide.
  The priced fix: the loader builds the chain on the CPU inside the pad/scale
  body it already runs (authored), and one blit pass builds it for a promoted
  photograph. A round of its own; the far-wall look is its gate.
- **The boot decode quadruples** (stb, main thread). The `authored6` aubade mark
  is the gate: if it crosses the READY floor, the road is the web-native one —
  `createImageBitmap` (off-thread, hardware) + `copyExternalImageToTexture`,
  retiring stb, the swap and the CPU pad for authored work. Measure first.
- **The capture quadruples** (~10 → ~40 ms once per ceiling). The `Photographer
  Snapshot` METER row on the Pixel is the gate. The priced arm if it says so: a
  512 capture into the 1024 layer — LoadOp::Clear on the full layer (the
  partial-write law's price, see `promote_to_exhibition`), viewport 512,
  `uv_scale 0.5`. The machinery exists; no new mechanism until the row asks.
- **If a floor phone refuses residency**, the levers in order: staging rings
  32 → 28 (the `WALL_ART` assert's floor; saves 32 MiB — weak), the two-walls
  split (photographs stay at 512 in their own array; returns 224 MiB of the
  delta and the capture cost with it; prerequisite for compression), and
  compressed textures (KTX2/Basis → BC7/ASTC; 1024 at ~1 MiB a layer). None
  built until a phone speaks.
- **`dist/paintings` grows ~4×** on re-encode; paintings stream progressively as
  before, so the boot wire pays only for what hangs. No action.
- **THE CASCADE NEEDS ONE MORE PASS THAN EVERY HANDOFF WRITES, and this is the
  second campaign it caught.** `mirror_census.py` pins `audit/BINDING_LEDGER.md`
  as an input and stamps "the last commit touching any input — not `HEAD`" (its
  own words). Every handoff's D9 cascade generates the ledgers and THEN commits
  them, so at the moment MIRROR is written, the BINDING_LEDGER.md it pins has not
  landed yet and the stamp names the previous commit. POSTCARD_0 hit it (fixed at
  U5c); PLATE_0 hit it again, identically. The tell is CLAUDE.md's own L33 witness
  — delete the five files in `audit/`, run the five tools, expect byte-identical —
  which fails on exactly this one line and passes once the stamp is settled. No
  `--check` catches it: the mirror census does not compare its own provenance
  stamp, so all four ledger checks stay green while the room does not rebuild.
  **The fix is a step, not a patch:** after committing the cascade, run
  `python3 tools/mirror_census.py` once more and commit the one-line result. It
  converges immediately — `BINDING_LEDGER.md`'s last-touching commit stops moving —
  and L33 holds again. The next handoff's D9 block should carry that step.

## POSTCARD_0 — A PICTURE LEAVES THE WALL (landed on master; Jean's gates open)

The visitor before a picture takes it — a key, or a tap on the words the glass
shows — and their own device keeps or sends it. DOORS_0's priced readback, built
because Jean asked; the roll's miniatures now have their machine.

| ruling | where it lives now |
|---|---|
| Before a picture is a CPU predicate: the zone, then the eye | `bodies/gallery.hpp` `pick_picture_before`, `TAKE_*` |
| The badge is the affordance; on glass it is the door; P is the key | `web/index.html` `window.T7_POSTCARD`; `input.hpp` `request_take` |
| One door, two mouths, folded at the boundary, refusals in words | `organ_boundary.inc` THE TAKE DOOR; `organ_registry.hpp` `gallery_take` / `take_take`; `cartridge.hpp` `request_postcard` |
| The fifth readback: on demand, no generation guard (a moment, not a state) | `cartridge.hpp` POSTCARD MACHINE; R11 copy, R1 map |
| The wall carries CopySrc; the staging is born on the first take | `state.hpp` `makeTextureArray("Exhibition"…)`, `ensure_postcard_readback_staging` |
| RGBA and provenance cross once; the texel order stops at the seam | `core/postcard_face.hpp` |
| The postcard is the picture as it hangs (uv crop; a photograph resampled to its hung aspect); JPEG 0.92 | `T7_POSTCARD.deliver` |
| One hand-off for two cameras; the message leaves from the visitor's own mail | `window.T7_KEEP`; `#ctlPhoto` calls it |
| The estate learns `.buffer =` as a copy target | `tools/binding_gen.py` `resource_reach`; `tools/binding_schema.py` `postcardReadbackStaging_` |

### Residuals — POSTCARD_0

- **The four `TAKE_*` numbers are control-panel material**, enrolled nowhere until a
  measurement asks (the pilot's rule). PLATE_0 moved three on Jean's eye — reach
  1.6 × the larger side, min 6 wu; slack 0.25 — keeping the invariant: the reach
  contains the pilot's standing point (1.4 ×, min 4) at every span, so a walk
  ends inside the zone. The cone (45°) holds a third-person eye that looks down
  at the pawn and past it. Each is still one number.
- **The badge's siting is top-centre**, on the argument that the ride face's spot is
  where thumbs land and fall through by charter. A visual row for Jean on both
  platforms; the idiom (inline `cssText`, built on first use) is T7_RIDE's.
- **The badge's words and the key are copy** (`docs/COPY.md`): *take this painting* /
  *take this photograph*, *· P* on a fine pointer; the two *Postcard* rows. Jean's
  naming gate — the facts are the tree's.
- **A delivered photograph's orientation is a visual row.** The readback hands the
  texel array as stored (row 0 = top for a decoded painting and for a render
  target); if a photograph arrives inverted, the fix is one vertical flip per kind
  in `T7_POSTCARD.deliver`, not in the engine.
- **P does nothing while the sandwich is open** — DOORS_2's guard withholds keydown
  inside the open menu; the badge's click is outside `#menu` and still works.
- **A larger postcard for authored work — RESOLVED at PLATE_0**: the wall itself is
  1024 (`PAINTING_CAP` 1024, q88), and the postcard ships the wall. The road past
  1024 is compressed textures, registered under PLATE_0's residuals.
- **The roll's miniatures** (DOORS_2's other reader of this readback) would be one
  more door with a slot parameter through the same machine and the same `deliver`;
  the roll is parked (DOORS_3), so they are not built.
- **The camera's residual stands**: the cursor orb appears in the visitor's PHOTO
  (the frame). It cannot appear in a postcard (a texture).
- **The estate census's `.buffer =` arm is INERT on this tree, and the handoff's
  perturbation for it does not reproduce.** The handoff expected `--check` without
  U2.6 to read `[FAIL] R-2 ... 1 orphan(s) — FLAGGED: postcardReadbackStaging_`.
  Measured on a sidecar at the completed U2 (P18), it reads `PASS ... 96 rows,
  0 orphan(s)` either way. `resource_reach`'s MapAsync arm already reaches the row:
  it scans the whole parenthesised argument text of a `MapAsync(` call, and U2.11's
  callback lambda sits inside those parentheses naming
  `postcard_readback_staging()` at its `GetConstMappedRange`. The rehearsal most
  likely perturbed between U2.10 and U2.11, where `dst.buffer =` is the only
  mention. Measured further: `\.buffer\s*=` also matches the generated
  `binding_surface.gen.inc`, so the arm adds a redundant "copy/write target" reason
  to **56 rows already reached as bind-group entries**, and rescues **none** from
  orphanhood. It is kept — it never removes a reach, and it is the honest spelling
  of a copy target against the next texture->buffer copy that is not also mapped
  here — but it is not load-bearing, and the sentence in its own comment in
  `tools/binding_gen.py` claiming the perturbation is one the tree falsifies.
  Correcting a claim of record is Claude's ruling, so the comment stands verbatim
  and the finding is registered here.

### What the round found — POSTCARD_0

Five things the handoff stated that the tree does not back, or states for the wrong
reason, plus two caveats for whoever edits the seam next. None is a HALT class (no
file unreachable, no blob stale), so all of U1–U5 was applied verbatim and every one
is registered here instead. The gate table is green at the tip either way. A sixth
was raised and then REFUTED by the round's own adversarial pass; it is kept below,
struck, because a register that records only the findings that survived teaches the
next round nothing about the ones that did not.

- **`note()` IS NOT GLOBAL, so all three of the new shell guards are dead — and the
  visitor's camera lost a diagnostic.** `web/index.html` has two inline `<script>`
  blocks. Block 1 is wrapped `(function () { 'use strict'; ... })();` and declares
  `function note(text)` INSIDE that IIFE; nothing assigns `window.note`. Block 0's
  top level is global scope, and that is where `window.T7_KEEP` and
  `window.T7_POSTCARD` live. So `typeof note === 'function'` — written three times
  in the new code (T7_KEEP's `say`, the badge's click fallback, `deliver`'s `say`) —
  is ALWAYS false, and every postcard diagnostic reaches `console.log` and never
  `record()` or the `#t7card` panel. Confirmed by reading and again under node.
  **U4.2's delta, stated in both directions** (the first draft of this line got it
  one-sided, and named the wrong panel; the round's own refuters caught both).
  `note()` reaches the on-device pane through `record()`, which paints `#log2`
  behind the `logToggle2` *details* button — NOT `#t7card`, which is a separate
  identity block fed by `window.t7card`. Against that sink, U4.2 LOSES one branch
  and GAINS another. Lost: a non-`AbortError` `navigator.share` rejection was
  reported by `note(...)` from inside block 1, where it was in scope, and reached
  `#log2`; it now runs inside T7_KEEP in block 0 and reaches only the console —
  which on the phone, where the share sheet actually lives, is the one place nobody
  can read. Gained: in the base tree the handler's outer `try` wrapped only the
  `canvasEl.toBlob(...)` CALL, so a throw from `new File`, `URL.createObjectURL` or
  the anchor append — all inside the async callback — was uncaught and reached
  nothing at all; T7_KEEP's own `try/catch` now catches those. `#ctlPhoto`'s other
  two arms (`toBlob` null, the outer `catch`) still call `note` directly and are
  unaffected. So: a wash on robustness, a loss on one diagnostic branch.
  THE FIX IS ONE LINE, and it is Jean's to take: add `window.note = note;` beside
  the declaration in block 1, after which all three guards start telling the truth
  and the lost branch comes back with them.
- **The estate census's `.buffer =` arm is inert, and its perturbation does not
  reproduce** — the long entry in the residuals above.
- **P arrives untranslated, but not for the reason the MECHANISM AUDIT gives.** It
  reads "`inject_key_event` converts A–Z to characters only", which implies P is
  spared; P (80) is squarely inside `GLFW_KEY_A..GLFW_KEY_Z`, so
  `event.character = 'P'` IS written. What makes the key work is a different fact:
  `event.key = key;` is unconditional, and `.character` is written in five places in
  `console.hpp` and **read nowhere in the tree** — the dispatch routes on
  `event.key` (`on_key_down(&input_deps_, event.key, ...)`). P works; the stated
  reason does not. Recorded so no one repairs a non-problem or mistakes `.character`
  for a usable second channel.
- **`GLFW_KEY_P` is the one key in the switch with no `#ifndef` fallback.**
  `input.hpp`'s GLFW KEY CODE FALLBACKS block guards fourteen codes — KP_8,
  KP_DECIMAL, LEFT/RIGHT_CONTROL, CAPS_LOCK, 4, 9, W, A, S, D, R, V, SPACE — and P
  is not among them. It is an asymmetry, not a defect: `input.hpp` includes
  `<GLFW/glfw3.h>` itself and both resolutions define `GLFW_KEY_P` as 80 (the TU
  gate passes, which proves the stub path). One `#ifndef` restores the symmetry.
- **`Module._gallery_take` is the shell's first direct export access.** Every other
  C-ABI reach in `web/` goes through `cwrap` — `index.html`'s own `abi()` helper and
  `organ_panel.js`'s eighteen sites. There is no `EXPORTED_FUNCTIONS` list to add
  the name to and `EMSCRIPTEN_KEEPALIVE` alone attaches it, so the call is sound;
  but it is a new idiom in this shell rather than the established one, and its
  failure mode is the printed note the handoff wrote for it. The P key is unaffected
  either way — that is what two independent mouths buy.
- ~~**U2.12 overstates `camera_pose_`'s freshness.**~~ **RAISED AND REFUTED.** The
  charge was that "the pose R1 just harvested" reads as fresh while
  `contracts/spine_state.hpp` brands `CameraPose` "ONE FRAME STALE ... encoded at
  R11 and mapped at R1 of the following frame". The refutation stands on three
  things. **"Already home" is the tree's own idiom** for CPU-resident / no new
  readback, used verbatim six lines above in the REACH_1 V2 ride watcher — which
  reads `point_.host` and `point_.bubble`, members the tree brands one frame stale
  in the same breath. It never claimed synchrony. **The lag is common-mode and
  cancels:** the badge watcher and `request_postcard` call the one
  `pick_picture_before` with the same `camera_pose_`, so U2.12's actual claim —
  "the badge and the take can never disagree by more than a frame" — is a
  badge-vs-take claim and is exactly true. **And the staleness is declared at the
  member already.** The fact is real; it is not a fault of this handoff, and the
  picture named at the press is chosen from a pose one frame old with no
  consequence at frame rate.
- **The seam's `$` map is consistent, and it is not the obvious one.** For the
  record, because a reviewer checking it against the prose will otherwise chase a
  phantom: the argument list is `}, bytes, res, bgra, crop_w, crop_h, aspect, kind,
  name)`, so `$5` is the DOUBLE (`aspect`) and `$6` is `kind` — not the other way
  round. The C++ signature, the EM_ASM body's `deliver(rgba, $1, $3, $4, $5, $6, s)`,
  the shell's `deliver: function (rgba, res, cw, ch, aspect, kind, stem)` and
  `cartridge.hpp`'s caller all agree. Mixing a double among integers is precedented:
  `core/aubade.hpp`'s `aubade_probe` already does it, and `console.hpp` already
  passes eight arguments.
- **`HEAPU8.subarray` is safe here only because the body never re-enters wasm.**
  CMakeLists links `-sALLOW_MEMORY_GROWTH=1`, under which a wasm memory growth
  reassigns `HEAPU8` and DETACHES any outstanding view. Between
  `var src = HEAPU8.subarray(p, p + n);` and the last read of `src`, `postcard_deliver`
  does only JS-side work — one `Uint8ClampedArray` allocation and two copy loops — so
  no growth can occur and the view cannot detach. Correct as written; the safety rests
  on that property, not on `subarray` being durable. Anyone adding a call back into
  wasm inside that block breaks it silently.
- **The new file's banner asserts a rule the tree does not keep.** It says "no runtime
  string helper is exported (console.hpp's rule — ccall/cwrap only), so nothing here
  calls one" — true of itself, and it does read the stem byte-by-byte through `HEAPU8`.
  But `core/aubade.hpp` calls `UTF8ToString` in two EM_ASM bodies and `core/boot_card.hpp`
  in three, against a link line that exports only `['ccall','cwrap']`. Five shipping
  sites bet the other way. Nothing in POSTCARD_0 is affected — it is the conservative
  one — but a future seam author who cites those files as counter-precedent will reach
  for a helper on the strength of a rule this banner states more broadly than the tree
  honours.
- **`bodies/gallery.hpp` cannot see `CameraPose` on its own.** Its transitive include
  closure is twenty files and does not reach `contracts/spine_state.hpp`; the only
  `CameraPose` it names for itself is the forward declaration in
  `contracts/entity_types.hpp`. Completeness comes from COHORT ORDER — `cartridge.hpp`
  includes the definition before the body. This is pre-existing (`gallery_unwatched`
  already dereferences a `CameraPose`), and `pick_picture_before` is simply its
  second reader; G-LAW 1 and the TU gate are green. Registered because the file
  cannot be compiled standalone and nothing in it says so.

## HOME_0 — THE PAGE IS CALLED HOME, AND FOLLOW FOR MORE COMES BACK TO THE MENU (landed on master; Jean's gates open)

Two asks from Jean, straight after DOORS_4. No handoff — his words are
the order.

| ruling | where it lives now |
|---|---|
| Everywhere it said *About*, it says **Home** | `web/routes.json`'s label; the veil's second door; the noscript line; the pane head and its door-out; the page's `<title>`; the 404 page |
| *Follow for more* returns to the site menu | `tools/routes.py` `follow_box_html`, appended to every site nav beside the write box |
| One name for one thing | the engine's pane and route label follow his words too — *Follow for more*, not *Follow the work* |

**THE WORD MOVED; THE PATH AND THE ID DID NOT.** `/about/` is still the
address, `about` is still the route id and the pane id, `web/about/` is
still the directory and `about_dist.py` still its builder. That is the
precedent DOORS_4 set when the collection became Gallery: labels are
words, ids are wiring, and a rename that touches the wiring breaks links
that are already printed and already shared.

**FOLLOW CAME BACK TO THE MENU, NOT TO THE FOOTERS.** Jean asked for it
"on the menu", and that is where it went — a nested `<details>` beside
the write box, the same idiom, so the sandwich holds two boxes and not
two ideas. DOORS_4 ruling 8's footers stay gone. `web/follow.json` is
still the one home and `follow_html()` still the one renderer; it now has
two places it is shown instead of one.

**AND THE LABEL IS JEAN'S, EVERYWHERE.** He wrote *Follow for more*; the
tree said *Follow the work*. Rather than leave the program calling one
thing by two names — the exact drift DOORS_4 spent a round removing — his
words won on both sides: the site box's summary, the engine's route label
and pane head, the `aria-label`, the docstring and COPY.md. Zero
occurrences of the old phrase survive. **He confirmed the wording
mid-round.**

### Residuals — HOME_0

- **The Gallery page's `<title>` still says *collection*** while the home
  page's now says *home*. DOORS_4 left it deliberately (ruling 5 scoped
  Gallery to "everywhere a menu speaks", and a `<title>` is not a menu);
  this round changed the about page's title because Jean's ask was
  "across the program". The two pages are now inconsistent with each
  other. **One word, Jean's gate.** `COPY.md` points at both.
- **The home page drops its own menu item**, as every page does
  (DOORS_1: a menu does not list where you are). Worth a look on Jean's
  visual gate — "Home" is the one label a visitor may expect to be
  present everywhere, and the rule that hides it is older than the name.
- **The site menu is now two boxes tall when both are open.** Measured at
  1100, 390 and 320 px: the dropdown fits, scrolls inside its own 80vh
  cap and never gives the page a sideways scrollbar. **A visual row for
  Jean** all the same — it is a longer menu than it was.

**Witnessed:** 45/45 across `/about/`, `/collection/` and `/writings/` in
Chromium — no page says *About*, every other page's menu says *Home* and
points at `../about/`, the follow box sits in the menu closed by default,
its summary reads *Follow for more*, its five links are https with
`rel="me noopener"` opening in a new tab, no footer carries a follow nav,
both boxes open at once, and the dropdown never overflows. Plus the
DOORS_4 drives re-run unchanged: 20/20, 17/17, 13/13, 62/62, 14/14,
12/12, 2/2.

## DOORS_4 — MOBILE AND DESKTOP, WRITINGS, GALLERY, AND THE COPY MAP (landed on master; Jean's gates open)

Seven units. The round opened on a bug Jean could see and no test could:
the Controls toggle did nothing.

| ruling | where it lives now |
|---|---|
| Mobile, not Smartphone, and one platform's list at a time | the `.seg` chip; `.pane [hidden]` is what makes "one at a time" true |
| The Board's second note is *Lose yourself.* | `.pane[data-pane="board"]` |
| The social links live in one place: the engine's sandwich | `write`/`follow` are `side: engine`; the site footers and the Follow pane's door-out are gone |
| Text becomes Writings — plural, browsable, one home | `assets/writings/NN_slug.txt` → `build_writings()` → `/writings/` **and** `writings.json`, from one read |
| The collection is called Gallery everywhere a menu speaks | `web/routes.json`'s label; the pane head; the one door out. The id `collection` stays — wiring |
| The world door gets its picture | `build_world(Image, preview)`; `assets/about/world.jpg` |
| Write me is a small box in the site sandwich | `tools/routes.py` `write_box_html`, appended to every site nav |
| The name appears once | `<p class="site">` gone from both site pages; the engine's wordmark is its home |
| The fallback speaks Jean's words; the button reads *Click here* | `fallback()` and the `#card` markup |
| `docs/COPY.md` — the copy map | new; every visitor-facing string and the one file that owns it |

### THE LESSON, SO THE NEXT PANE ELEMENT DOES NOT RELEARN IT

**An author `display` beats the user agent's `[hidden] { display: none }`,
at any specificity, because origin outranks specificity.** `.pane dl`
set `display: grid`, so both platform lists rendered always; the JS
flipped `.hidden` correctly and the cascade ignored it. Measured in
Chromium before and after: `hidden=true, display=grid, on screen` →
`display=none, off screen`.

The shell had already learned this once — `web/index.html` carries
`.layer[hidden] { display: none; }` for the card, for exactly this
reason. The lesson never reached the panes.

**THE FIRST FIX WAS STILL TOO WEAK, and the review caught it (R1).**
U1 wrote `.pane [hidden]` and claimed in its comment that it beat every
other display rule in the file. Measured, it does not. Four rules declare
`display` at (0,2,1) and outranked it — `.pane a.out`,
`.pane .mini-hero img`, `.pane .follow-list a`, `.pane .wlist button` —
all four rendering with `hidden` set. And a DESCENDANT selector never
matched the panes themselves, which stayed on the UA rule while
`#menu.over > .pane` sits at (1,2,0): one `display` added there and all
six panes render at once, the same bug at six times the scale.

**So the landed rule is `!important`, and refusing it at U1 was my
error.** `hidden` is a user-agent invariant this stylesheet broke by
accident; restoring it is precisely what `!important` is for, which is
why every CSS reset writes this line. Specificity cannot do the job — no
class-based selector outranks an id-based one, so no amount of ordering
discipline would ever have held. The rule is three selectors, scoped to
the sandwich:

```css
.menu [hidden], .pane[hidden], .pane [hidden] { display: none !important; }
```

**So the lesson is two lessons.** The first is that origin beats
specificity. The second is that a fix for a cascade bug must be MEASURED
against every element it claims to cover — reasoning about specificity
numbers in a comment is exactly how the second version shipped broken
too.

### MY WITNESS FAILED, AND THAT IS WHY THE BUG SHIPPED

DOORS_3's Chromium drive asserted `l.hidden` — the PROPERTY the JS sets —
instead of the computed display. It passed 64/64 on a page where nothing
moved. **Asserting the attribute you just set proves only that you set
it.** Every test this round asserts `getClientRects().length` and
`getComputedStyle`, and the same rule now applies to every future round.

### Two things the handoff got wrong, and one it left open

- **The authority paragraph named one campaign; SEVEN had landed.** Base
  was `27c4e466` — twenty commits and `HOLD_0`, `SPREAD_0`, `EASE_1`,
  `CAP_0`, `SPORE_0`, `RISE_0`, `MASSIF_0`. "MASSIF_0 is the tip —
  engine-only, no overlap" was false twice over: `EASE_1` edited
  `web/index.html` (the landing hint 5 s → 8 s). It sits in the
  glass-badge helper, nowhere near any FIND, so nothing was blocked.
  **This is the second consecutive round whose authority paragraph
  under-counted the tree** — DOORS_3's did the same. A handoff's base
  should be read, not recited.
- **`write_box_html`'s docstring claims an email fallback its own code
  cannot give.** `routes.py` renders for every site page and has never
  read `site.json`; only `about_dist`'s `fill()` knows `__EMAIL__`, and
  `collection_dist`'s does not, so a literal `__EMAIL__` in the shared
  nav would ship raw on the Gallery page. **THE SITE THEREFORE LOST ITS
  MAILTO FALLBACK**: the old band offered the address when the endpoint
  refused; the box says *try again in a moment*. The engine's Write pane
  still offers the mailto. The docstring now says so plainly. **Jean's
  call** — the fix is either an about-page-only box, or teaching
  `collection_dist`'s `fill()` the `EMAIL` key.
- **The dropdown WAS too tight**, as the handoff suspected. Measured at
  187 px wide with a 159 px textarea, which is not a message box. Widened
  to `min(92vw, 20em)` as proposed, then re-measured at 1100, 390 and
  320 px on the site pages **and on the engine shell**, which shares the
  rule: everything fits, nothing scrolls sideways. **A visual row for
  Jean either way.**

### Two hazards beyond the document, both fixed here

1. **`fill()`'s refusal tuple is FIXED and checks the OUTPUT.** U0's
   question answered: a token absent from a template never appears and
   passes, but a token PRESENT and unsubstituted **ships silently**
   unless it is named in the tuple — and DOORS_3's text block never had
   to learn this, because it used only tokens already there.
   `__WRITINGS__` and `__WORLD__` now join it. Without that, a marker
   typo deploys a literal HTML comment where the poems belong.
2. **A stale `dist/text/` would have shadowed the 301.** DOORS_3 made
   `about_dist` the writer of `dist/text/`, and `web_dist` deletes only
   "the engine's own names" — `text` is not among them. So a `dist/` from
   before this round keeps a live `dist/text/index.html` that `wrangler`
   uploads and Pages serves, over the redirect. `about_dist` now sweeps
   it: the writer that made it removes it. Verified with a planted
   sentinel. The `/text/` no-cache rule follows the page to `/writings/`.

### Residuals — DOORS_4

- **The site has no mailto fallback** (above). Jean's call.
- **The write box's dropdown width** and **the world door's picture
  proportions** are visual rows for Jean. `assets/about/world.jpg` does
  not exist until he places it; the build says so and ships without it.
- **`peek.json` still carries `href` per work**, read by nothing now that
  the tiles are inert `<span>`s. Unlike `sets` it is one field of a
  record rather than a whole shape, and the handoff named only `sets`, so
  it stands. A later round takes it or keeps it deliberately.
- **The Gallery tiles lost their link role and their `title` tooltip.**
  Ruling 5 asked for exactly that — pictures, not doors — and the work's
  name survives in each `img`'s `alt`. Named here because it is a
  deliberate accessibility trade, not an oversight.
- **The collection page's lede `h1` is still placeholder copy**, and the
  review corrected what I wrote about it: it does NOT repeat the
  writings' opening line. It is *The pawn is a vessel for projected
  intent.* — the doctrine that opened the about page until DOORS_3, moved
  to `/text/`, and deleted with that page at DOORS_4. It is on neither
  poem, so the Gallery page and two `<meta>` descriptions are now the
  only place that sentence lives. Jean's to write, or to move back into
  `assets/writings/`.
- **The Gallery page's `<title>`, `og:title` and `description` still say
  *collection*.** Ruling 5 scoped "Gallery" to *everywhere a menu
  speaks*, and a `<title>` is not a menu — so this is left as written and
  named for **Jean's naming gate**. `COPY.md` now points at it.
- **`docs/COPY.md` exists and must be kept current.** Every future round
  that moves words updates it. Its every pointer was checked against the
  tree at this commit (38 symbol and path assertions).
- **The engine menu has no plain link out**, and now neither Follow nor
  Write has a door to the site — because the sections they pointed at are
  gone. Deliberate; named so a later round does not read it as a loss.

### What the adversarial review found after landing (R1–R3)

Six finders and a completeness critic over the landed diff; every claim
re-measured here before acting. Beyond the `[hidden]` rule above:

- **The Send button was not a button in a row.** `.menu > nav button`
  (0,1,2) makes every button in the dropdown a full-width left-aligned
  block — right for a menu item, wrong for Send, which measured 260 px
  and stacked ABOVE its status line. `.pane .row`'s flex could not help:
  it needs a `.pane` ancestor the site sandwich has not got. **My U5
  witness asked whether the box was typable and whether it posted; it
  never asked what the button looked like.**
- **The dropdown had no height cap** while the engine's pane has carried
  one since DOORS_0. Capped at 80vh; verified on an 844×390 viewport.
- **`build_writings()` could refuse after the about page had shipped and
  before the `dist/text/` sweep**, leaving exactly the half-written dist
  the sweep exists to prevent. It is a pure read; it now runs before any
  write. Reproduced with a broken writing, then disproved.
- **The slug was the one unescaped interpolation** in a function that
  escapes its title and its body. A filename can hold a quote.
- **The 404 page still said *collection* and offered no Writings.** It is
  served at every wrong address and does NOT read `web/routes.json` — its
  links are absolute because it answers from everywhere — so only reading
  it could catch this. `COPY.md` now carries it and says so.
- **`setLabel` and the collection page's three `footer` rules** were dead
  matter, removed.

### Still open after the review

- **`#reload` is protected only by the UA rule.** `web/index.html`'s
  `<button class="btn" id="reload" hidden>` works because `.btn` declares
  no display — but `a.btn { display: inline-block }` sits three lines
  away, and the card's other door is already an anchor. Make `#reload` an
  anchor and it is permanently visible on a card that passes
  `offerReload=false`. Outside the sandwich, so `menu.css`'s rule does not
  reach it; `.layer[hidden]` is the local precedent to follow.
- **The no-JS write path ends on a raw JSON body.**
  `functions/api/message.js` answers `application/json`, so a visitor with
  JavaScript off is navigated to `{"ok":true}` with no way home.
  Pre-existing — the deleted band had it too — but the box put it on all
  three site pages. Fixing it means content negotiation in a function this
  round did not touch.
- **The colophon now appears on one site page in three** (`/about/`
  only). Ruling 8 asked for the name to appear once and said nothing
  about the colophon. A visual row for Jean.
- **The write box is a `<form>` inside a `<nav>` landmark**, so a screen
  reader announces the message form as navigation. Valid HTML, odd
  semantics; the alternative is a second landmark beside the nav.
- **`build_writings` mints article ids nothing links to.** `writings.json`
  carries only `title` and `html`, so neither the page nor the pane can
  deep-link a piece. Registered, not a defect: no words ask for it yet.
- **`/api/message` accepts cross-origin form POSTs — an unauthenticated,
  unrate-limited relay into Jean's inbox.** Any third-party page can host
  a form pointed at it and auto-submit; `application/x-www-form-urlencoded`
  is a CORS-simple request, so the browser sends it without a preflight
  and `parseBody` accepts it. **PRE-EXISTING and untouched by DOORS_4** —
  the deleted band posted to the same function — but the write box put
  that door on all three site pages, so it is named here rather than left
  to a later reader. The honeypot catches a naive bot, not a targeted
  one. A fix is an Origin check in `functions/api/message.js`, plus a
  rate limit; both are that file's business and this round did not open
  it. **For Jean to rule on.**
- **The `<noscript>` paragraph still carries in-text links** to the
  collection and About, while U6's ruling for the card was "no link in
  the text — the button below is the door". Different surface (a visitor
  whose scripts never ran has no button), so it was left alone; named
  because the two now read differently and a later round should decide
  deliberately rather than by accident.

### Where the witnesses could not reach

`assets/about` holds no hero images and the collection holds 0 works in
this checkout, so `about_dist` and `collection_dist` were exercised
against stubbed heroes, a stubbed strip and a planted `world.jpg` in a
scratch dist, and the Gallery pane against a two-work `peek.json`
fixture. On Jean's machine both run for real. The `/text/` → `/writings/`
301 is Cloudflare's to serve; what was proved here is that nothing in
`dist/` will shadow it.

## DOORS_3 — THE LAYOUT ROUND (landed on master; Jean's gates open)

Five units. The engine's menu stopped being a list with one big pane and
became the site itself: seven items, seven panes, one door out of each.

| ruling | where it lives now |
|---|---|
| The veil says two names and nothing else | `#enter` is *The Board*, the anchor is *About*; the sentence, `details` and the veil's log pane are gone |
| No GPU talk anywhere a visitor reads before choosing | the noscript paragraph, and `classify()`'s status line (*Waking your device*) |
| The fallback speaks Jean's words, and the card's door follows them | `fallback()`'s *Welcome.*; the static door went to `/collection/` *(DOORS_4 U6 moved it to `/about/` and the button reads **Click here**)* |
| Every item on the menu opens a pane | `web/routes.json` — seven engine panes; the `website` route died |
| Controls is its own pane, two platforms, one shown | `.pane[data-pane="controls"]`, `showPlatform()`, `CONTROLS_DEFAULT` |
| Follow the work has one home and two readers | `web/follow.json` → `routes.follow_html()` *(one reader since DOORS_4: the engine's Follow pane; the site footers went with ruling 3)* |
| Text is its own page | *(superseded at DOORS_4: `/text/` became `/writings/`, plural, and retired by 301)* |
| The world door says *Navigate the world* and nothing under it | `web/about/index.html`'s first door *(DOORS_4 U5 put Jean's picture under it — no words, still)* |
| Authors and elsewhere leave | the two bands, `build_authors`, `build_links` and their CSS are gone |

**The six controls rows are the tree's, read at U0** — and two had moved
since the handoff was written, so the pane carries the tree's answer:

| verb | desktop | smartphone |
|---|---|---|
| Directions | W A S D | drag the left half |
| Rotation | drag with the mouse held · the wheel zooms | drag the right half · two fingers to zoom |
| Pulse | Caps Lock — on a summit it boards the ribbon, and lands it again | two fingers, one clean tap on the right half — same |
| Aura | 3 lights it · 2 raises it | a second finger on the left half |
| Leap | Space | one clean tap on the right half |
| FPV | Ctrl | two fingers on the left half, landing together and lifting as one |

`FPV_TAP_0` answered the handoff's open question — the phone HAS an FPV
mouth (`on_touch_tap_fpv`), the LEFT pair, told from the aura by the
ARRIVAL window: the aura is a second finger landing on a stick that is
already held. `REWIRE_0` moved the ride off `request_radial_pulse` onto
`request_pulse_swap`, so the gesture the glass teaches (*pulse to fly /
pulse to land*) is the gesture that works; the leap no longer rings. Both
rulings landed from another session while DOORS_3 was being written.

### The handoff's authority paragraph was wrong on two counts

It named `FRUSTUM_0`, `FOURWALLS_0` and `FENCE_0` and said "none touch
`web/` or the dist scripts". Two more campaigns had landed —
`FPV_TAP_0` and `REWIRE_0` — and `REWIRE_0 U1` **did** edit
`web/index.html`: a `landT` clock that hides *pulse to land* after five
seconds. It sits in the glass-badge helper, nowhere near any FIND, so no
unit was blocked; it survives untouched.

**Witnessed here:** the shell driven in headless Chromium at 1280×800 and
at 390×844 with a coarse pointer — 32 assertions each, 64/64. The
platform defaults to the visitor's own device and one tap reaches the
other; the six verbs read in order; About fills from the day's pick with
the `about/` prefix and links to that work; Text filled from `text.json`
(Writings reads `writings.json` since DOORS_4);
Follow shows five https links; and DOORS_2's key blocker stays fixed
(keydown withheld only while the menu is open, keyup never withheld, W
reaches the world once closed, Space does not re-open it). `about_dist`
was run end to end into a scratch dist against stubbed hero images.

### Residuals — DOORS_3

- **The photographer is PARKED, not removed.** The roll pane, its 2 s
  refresh, its focus-preserving rebuild and the three `cwrap` wrappers
  left `web/index.html` at U4. `gallery_roll`, `gallery_visit` and
  `gallery_visiting` are still exported by `organ_registry.hpp`, still
  reachable from the console, and the pilot (`PilotState`, `pilot_tick`,
  the Pilot spine row, the five releases) is untouched. The pane's markup
  and script are in git at `d998022`^ — resurrect with
  `git checkout d998022^ -- web/index.html` and take the block.
- **`assets/about/site.json` keeps `authors` and `links`, now read by
  nothing.** The file is Jean's; he prunes.
- **TikTok's href is written canonically** (`/@kleinjean1`), not in the
  bare form Jean wrote. If it does not resolve, his form is the one to
  keep. `web/follow.json` is the one home.
- **`.doors` was left as it stood** — `repeat(auto-fit, minmax(min(340px,
  100%), 1fr))` already takes a third door without a rule change. Three
  columns where the width allows, wrapping below. **A visual row for
  Jean.**
- ~~The footer nav takes `flex: 0 0 100%`~~ — **CLOSED at DOORS_4**:
  ruling 3 took the follow nav off every site footer, and the
  `footer .follow` block went with its last reader.
- **`.statement h1` matches nothing today.** The statement Jean's
  placeholder leaves is a single `<p>`. The rule was renamed from
  `.doctrine` rather than deleted, because whether the slot grows a
  heading is his copy's call — unlike the band CSS, which was deleted
  because those bands can never return.
- **Two GPU survivors are visitor-facing and were NOT touched**: the
  `<meta name="description">` still ends "WebGPU, in the browser.", and
  it is Jean's copy to give. (The Board pane's "drawn by your GPU" died
  with the old pane.)
- ~~`text.json` carries the article's placeholder HTML comment~~ —
  **CLOSED at DOORS_4**: `text.json` is gone; `writings.json` carries no
  comment, because `build_writings` emits only the stanzas it parsed.
- ~~`/text/` was given the `no-cache` rule `/about/` has~~ — **CLOSED at
  DOORS_4**: the rule followed the page to `/writings/`.
- **The engine menu has no plain link out any more** *(and since DOORS_4
  neither Follow nor Write has a door to the site either)*. The `website`
  route died by ruling, and every remaining engine route is a pane. Each
  pane carried its own `a[data-out]` when this was written; since DOORS_4
  three do — Gallery, Writings, About — and four do not: The Board and
  Controls never had a page to point at, and Follow and Write me lost
  theirs when the sections they named were deleted. Deliberate, and named
  here because it is the kind of thing a later round would otherwise read
  as a loss.

### One discipline gap, closed here

`docs/HANDOFFS/` holds open work orders only, and DOORS_1 and DOORS_2
were never filed there when they landed — their OPEN.md entries say
"Jean's gates open", so the directory contradicted the register for two
campaigns. Both are filed now beside DOORS_3, verbatim as Jean sent them.
All three leave the moment Jean's gates close them.

### Where the witnesses could not reach

`assets/about` holds no hero images in this checkout and the collection
holds 0 works, so a real `python3 tools/about_dist.py` cannot run here:
the text page's build (now the writings page's), `about.json`'s new
shape and `dist/text/` (now `dist/writings/`) were
proven against stubbed heroes and a stubbed strip in a scratch dist, and
the About pane against a hand-written `about.json` of the same shape.
On Jean's machine both run for real. `collection_gate: PASS` again proved
the NAV refs only, for the same reason DOORS_1 recorded.

## DOORS_1 — THE GATE'S VERDICT, AND FOUR FINDINGS FROM THE REVIEW (landed on master; Jean's gates open)

Six units. It arrived AFTER DOORS_2 — its own base was `44dcf6d`, and
DOORS_2 had already landed on top — and it composed anyway.

| ruling | where it lives now |
|---|---|
| The gate was right and the renderer was wrong: a route list must be spoken from the page rendering it | `rel(href, base)`, `nav_html(side, base, indent)` in `tools/routes.py` |
| A menu does not list where you are | the `"./"` drop; `/about/` no longer offers "About" |
| The gate's refusal now says WHY a leading slash is neither the site's nor the collection's | `collection_gate.py`'s hint line |
| The pilot's fifth release — the world changed — and it speaks, as the other four do | `cartridge.hpp`, beside `camera_pose_ = CameraPose{}` |
| The stride's ease is linear; the quarter floor it could never reach is gone | `PILOT_SLOW_WU`'s comment, and the clamp |
| The roll keeps the hand's place across its refresh | `refreshRoll`'s `had`/`again` |
| "Still waking" and "not in this build" are two different sentences | `refreshRoll`'s two guards |
| A non-finite slot loses its row rather than the whole list | `gallery_roll`'s `isfinite` continue |

**`collection_gate: PASS (0 local refs)`** — the state it was in before
DOORS_0 gave the collection page an absolute-href menu. The regression that
opened this campaign is closed.

### It landed out of order, and that cost one merge

DOORS_1 U1.1 quotes `nav_html` as it stood before DOORS_2 U2 added the
return path's `attrs` branch, so its FIND missed and its REPLACE would have
deleted `rel="opener"` and the close-or-navigate `onclick`. **The two
handoffs compose exactly**: DOORS_2 U2.2's own FIND matches DOORS_1's
replacement function verbatim, so the merge was DOORS_1's text with DOORS_2's
text applied on top of it — zero hand-written lines, no third author. Every
other DOORS_1 anchor matched at 1 on the landed tree, and `cartridge.hpp`,
`organ_registry.hpp` and `collection_gate.py` were blob-identical to its own
table.

The rollback was considered and refused: it would have rewritten eight
pushed commits — merged to master expressly so collaborators could see them
— and put at risk work that lives in neither handoff (the corrected controls
table, the ride row reconciled with the glass, the four post-review
corrections).

**Witnessed here:** `rel()` on 13 cases, every output resolving to its
intended URL under `urljoin`; the nav rendered from all three bases; the
focus fix measured in headless Chromium — focus survives the 2 s rebuild on
the same slot, and falls to the first caption when that picture comes down;
both new sentences fired from their own conditions (`Still waking` with no
Module, `Not in this build.` with a Module missing the export).

### Residuals — DOORS_1

- **The collection gate's missing table row — TAKEN.** CLAUDE.md now carries it,
  earned by the file's own standard (a non-zero exit shown on three perturbed
  trees: an engine token in the closure, a ref naming a file that is not there,
  and the leading slash that was DOORS_0's regression). The paragraph under the
  table gained the rule the tree had now learned twice: **a gate with no row is
  a gate nobody runs.** Every gate in the tree is listed; `gol_census.py` is not
  a gate (no verdict, no `--check`) and stays out.
- The collection here holds **0 works**, so `collection_gate: PASS` proved
  the NAV refs and did not re-prove the `srcset` refs — those were passing
  before DOORS_0 and are untouched. On Jean's machine the count will not be 0.
- `assets/about` is Jean's, so `/about/`'s rendered nav is proven by
  `python3 tools/routes.py` and by `about_dist.fill()` on the real template,
  not by a full `about_dist` run.

## DOORS_2 — THE MENU AS THE WEBSITE IN MINIATURE (landed on master; Jean's gates open)

Seven units. The menu stopped being a list of links and became the website
in miniature.

| ruling | where it lives now |
|---|---|
| One door for the build — four stages in the one lawful order, stopping at the first that fails | `tools/dist.py` |
| The return path is the browser's own tab: engine links open with `rel="opener"`, and *The world* closes the site tab when an opener exists | `tools/routes.py` `nav_html`, the `return` flag in `web/routes.json` |
| The sandwich is a glyph — three bars drawn in CSS, `aria-label` carrying the name | `web/menu.css` `.bars`, the three `<summary>`s |
| "Controls" became "The Board": what it is, the controls, things to try, the live row, and the world's own photographs | `.pane[data-pane="board"]`; the Photographs route died |
| Text and Write me became panes — the about page in miniature, one fetch | `about_dist` emits `dist/about/about.json`; `loadAbout()` |
| The visitor's own camera — the canvas's last frame to their device, share sheet or download | `#ctlPhoto` |
| Miniatures of the world's photographs are NOT built — they need the readback | registered below |
| The pilot aims only on approach, and proportionally | `PILOT_AIM_WU`, `PILOT_AIM_GAIN` |

**Witnessed here:** 22 shell behaviours in headless Chromium — the glyph's
three drawn bars, The Board's ten control rows, the roll folded in and still
listing, the Text pane rendering the doctrine out of the real `about.json`
emission, the Write pane's four fields and its honeypot, a 501 falling back
to `mailto:` with the address from `about.json`, and a 200 saying thank you
and resetting. `tools/dist.py`'s loop was exercised on all three paths
(all-pass, mid-fail, last-fail): it stops at the first failure, returns that
stage's own exit code, and runs nothing after it.

### THE CONTROLS TABLE WAS WRONG, AND IS NOW THE TREE'S

DOORS_2 ruled "the controls FACTS are not [placeholders]" and its U0 ordered
the ride key found rather than assumed. Found, and most of the rest with it:

- **`R` IS BOUND TO NOTHING.** `GLFW_KEY_R` occurs twice in the whole tree,
  both in `input.hpp`'s `#ifndef` fallback. There is no case for it in
  `on_key_down`. The ribbon is boarded — and left — by the SAME LEAP
  GESTURE (Space, or a lone clean tap on the right half), gated on
  `point_.bubble.summit`: `request_radial_pulse` calls `possess(RIBBON)`
  from a summit and `possess(PAWN)` from the ribbon. **Two comments still
  name the dead key** — `begin_visit`'s "The R key's own transaction" and
  `organ_boundary.inc`'s "the same transaction key R presses". Stale prose
  over a dead binding; VISIT_0 wrote the first of them.
- **There is no `GLFW_KEY_ESCAPE` in `src/` at all.** The program's pointer
  door is `Numpad ✱` (`console.hpp` intercepts `GLFW_KEY_KP_MULTIPLY` before
  dispatch). Esc freeing a locked pointer is the BROWSER's rule, not this
  tree's — and the one thing Esc verifiably does while a visitor reads the
  pane is close the pane.
- **A bare mouse does not look.** `on_mouse_move` writes the look deltas only
  under `mouse_.left_dragging`. The RIGHT button drag is a PAN, a control the
  table still does not mention.
- **The phone halves are specific**, and the vagueness was the lie: the LEFT
  half walks (a floating stick, born where the thumb lands), the RIGHT half
  looks. A visitor told "one half" who drags the right half looks instead.
- **Zoom was missing entirely** — the wheel, and two fingers on the right half.
- **Space is the LEAP ALONE** since PULSE_SPLIT_0; the ring left with the
  other hand (Caps Lock, or a two-finger clean tap), which also reaches for a
  body as the wave goes.
- **4 to 9 is six worlds**, not seven: noon, sunset, indoor flat, indoor
  vault, finite outdoor, night. `MOOD_ATRIUM` — the boot mood — has no key.
- Bound and still unlisted, deliberately (panel knobs, not visitor controls):
  `V` the rim, `[` `]` the render radius, `0` the orb palette, `KP_+/-` look
  sensitivity, `KP_8` the orb motion rule, `KP_.` the orb gesture.

**And a consequence of the sandwich's own key guard:** while the menu is
open, every key this pane lists is `stopPropagation`'d, so a visitor cannot
try one while reading it. Correct — the keys are the menu's while it is open
— but it means the pane teaches rather than demonstrates.

### DOORS_2 U2 HAS NO COMMIT OF ITS OWN — an executor's slip, recorded

The return path (U2: the `return` flag, `nav_html`'s `attrs`, and the two
`rel="opener"`s in the shell) was applied and witnessed, then swept into
**`c9418a20`**, U4's commit, by a `git add -A` — so that commit carries two
units and its message names only one. One commit per logical unit is the
rule and this round broke it once. The history is pushed to master and is
not being rewritten for a bookkeeping error; the code is all present, and
`git log -S'rel="opener"' -- tools/routes.py` finds it. Named here so no
reader concludes U2 was skipped.

### Residuals — DOORS_2

- **The collection gate red on master — CLOSED at DOORS_1 U1.** The cause was
  DOORS_0's absolute-href menu, not the gate's law; `rel(href, base)` now speaks
  every route from the page rendering it and the gate reads
  `collection_gate: PASS`. `python tools/dist.py` runs all four stages again.
- **DOORS_1 not having landed — CLOSED.** It landed after DOORS_2 and composed with
  it (see the DOORS_1 entry above). Both adaptations are retired: `nav_html` carries
  DOORS_1's `rel()` and DOORS_2's `attrs` together, and the shell's six hand-written
  hrefs are relative, as DOORS_2 specified.
- **The readback (exhibition texture → CPU)** is the one mechanism that would
  yield BOTH miniatures of the world's photographs in the menu AND "the
  world's own photographs sent to the visitor". Priced at DOORS_0's recon (a
  fifth readback machine, a channel swap, a megabyte a shot). BUILT at POSTCARD_0
  (the texture readback, on demand); the roll's miniatures remain unbuilt because
  the roll is parked. Note for whoever prices it again: `surfaceConfig_.usage`
  is **never assigned** in `console.hpp` — it keeps `RenderAttachment` only,
  so there is no `COPY_SRC` on the swapchain texture and the C++-side fallback
  needs that line changed before anything else.
- **The camera's guard cannot catch its own failure mode.** DOORS_2 registers
  a black picture as the gate row and `if (!blob)` as the guard. Measured in
  headless Chromium: `toBlob` on a canvas that HAS a webgpu context returns a
  valid PNG (1572 bytes, correct magic) — **it does not return null**. So the
  guard fires only when there is no context at all; a blank or black frame
  arrives as a perfectly well-formed PNG and the visitor is handed it. Nothing
  in the shell can tell. Jean's eye is the only gate, and the fallback stays
  the engine-side copy above.
- **The cursor orb appears in the visitor's photo** — the roadmap's "hidden
  during recording" item now has a second reader.
- **The pane and the glass use two names for one press.** `window.T7_RIDE`
  draws "pulse to fly" / "pulse to land" on the badge; PULSE_SPLIT_0 took the
  ring off that door, so what it actually does now is leap and board. The
  pane says "Space", and names the badge's word rather than choosing between
  them. One vocabulary, one home — Jean's naming gate.
- **Two comments still name the dead R key**: `begin_visit`'s "The R key's own
  transaction" (VISIT_0 wrote it) and `organ_boundary.inc`'s "the same
  transaction key R presses". Stale prose over a binding that does not exist.
- The about page's big *the world* door (`.door` and the hero-plate link)
  boots a fresh world even when the tab has an opener; only the menu's route
  returns. Decide whether the doors should return too — one attribute each,
  in the template.
- CLAUDE.md's build line — TAKEN: it reads `python tools\dist.py`, with four lines
  saying what the one door does and noting that `web_dist.py` alone still builds
  the engine's half and skips the gate guarding the other one.
- Email-to-self of the visitor's photo through `api/message` (Resend takes
  attachments) is possible and is an open relay unless gated; not built.
  POSTCARD_0 ruling 7: the share sheet is the mail path; the site sends nothing.
- Copy: The Board's sentence and its three suggestions, the pane sentences,
  the send/fail words, and every control row's WORDS (their facts are the
  tree's).

## THE POST-LANDING REVIEW OF DOORS_0 + VISIT_0 — what it found, and what was done

Six reviewers read the landed diff adversarially, one per dimension. The
line taken on every finding: **a defect that breaks a campaign's OWN stated
contract was corrected in commit `POST-REVIEW`; a finding that would EXTEND
a contract is registered below, unapplied, because extending is Claude's and
Jean's.** The corrections ride one commit so they can be reverted as one.

### Corrected — each restored a contract the campaign already stated

- **THE SANDWICH WAS EATING THE WORLD'S KEYS — a blocker.** The three
  `stopPropagation` listeners were bound to `#menu` for the life of the page
  with no `menu.open` test, and `<summary>` KEEPS focus after the menu
  closes. Measured in headless Chromium: open the menu, close it (Escape or
  a second click), and `activeElement` is still `SUMMARY` — W, A, S, D and R
  never reached a bubble-phase document listener again, so **the visitor
  could not move after opening the menu once**, and Space re-opened the
  sandwich instead of firing the pulse. Worse, `keyup` was withheld too, and
  the engine LATCHES key state (`on_key_up` clears `keys_.forward`): a key
  held while focus entered the menu lost its release and the pawn walked for
  ever. DOORS_0's own gate row says "WASD typed **while the menu is open**
  does not move the pawn" — the guard exceeded its own words. It is now
  scoped to `menu.open`, never withholds a `keyup` (a release can only CLEAR
  a flag), and the summary is blurred when the menu closes so focus returns
  to the world. Eight key-routing behaviours measured green.
- **`web_dist.py` refused the menu marker AFTER destroying the previous
  dist.** DOORS_0 U3 placed the check at the `shader_sha` line — past the
  `rmtree` of `owned` and the `ARTIFACTS` copy — so a duplicated or lost
  marker deleted `dist/index.html` and replaced it with the raw template
  (literal `__BUILD_ID__`, no menu) and only THEN refused. The handoff said
  it was refusing "on the build-id law's own precedent"; that law is stated
  in this file as "THE REFUSAL COMES FIRST, before rmtree — a shell that
  cannot be versioned must not cost the previous dist." The count is a
  property of the shell SOURCE, so it now asks `shell_src` up beside the
  other three refusals, before `rmtree`. The substitution stays in the
  `shell_out` pipeline where it belongs.
- **A successful peek retry rendered underneath the failure it disproved.**
  `loadPeek`'s catch writes "The collection did not answer." and clears
  `peekLoaded` so the next opening retries — but the success arm only
  appended and never cleared the box. Reproduced: 503 then 200 left the
  twelve tiles captioned by a stale error for the rest of the session. The
  success arm now clears first.
- **`organ_mood_names` lost its doc comment to VISIT_0's banner.** U2.4
  anchored the roll block on the function's signature, splicing it between
  the comment and the function it describes. The comment is back where it
  belongs; the roll's banner sits above it.

### Registered, not applied — each would EXTEND a stated contract

- The pilot outliving a world teardown (VISIT_0's residuals, first entry) —
  a fifth release condition, and P6 wants it to speak.
- The roll's 2 s refresh dropping keyboard focus (VISIT_0's residuals) —
  focus preservation is an addition to a poll the handoff specified.
- **One disjunct of the roll's "Not in this build." guard may be dead —
  narrower than it first looked.** A reviewer reads emscripten's `$cwrap` as
  taking the throwing fast path only when `numericRet` is true (i.e.
  `returnType !== 'string'`), so `w('gallery_roll', 'string', [])` would
  always return a closure and `abi()` would never note that name's absence;
  the same reading says `-sASSERTIONS` compiles the fast path out entirely,
  so under the Debug preset no name can throw. **UNVERIFIED** — no emsdk in
  this container to read `libccall.js` against.
  **But the guard is NOT dead code, and the first draft of this entry said
  otherwise.** `refreshRoll` asks `if (!c || !c.roll)`, and `abi()` returns
  `null` — WITHOUT caching, so it re-probes — whenever `window.Module` or
  `Module.cwrap` is absent. That is the ordinary state the block's own
  comment names ("the program may still be compiling when the shell runs"),
  and the menu is static HTML a visitor can open before the wasm resolves.
  So the words reach the pane on the common path; only the `!c.roll`
  disjunct is in question, and only on a shell/wasm skew, which
  `web_dist.py` ships against by writing `index.html` and the wasm in one
  pass. Low stakes, then — but the WORDS are wrong on the path that does
  fire: a visitor who opens Photographs while the program is still compiling
  is told "Not in this build." when the truth is "not yet". Worth a
  sentence of Jean's, and one look at `libccall.js` on a machine with an
  emsdk.

## VISIT_0 — THE ROLL AS DIRECTORY (landed on the session branch; Jean's gates open)

The world already photographs itself and already hangs the pictures. The
sandwich gained a pane that lists what is hanging **as facts** — tier,
distance, bearing, age, wall or ground — and choosing one makes the pawn
**walk there**, the camera swinging round to face it. No texture leaves the
GPU; the CPU reads its own slot array. Zero readback, zero new render pass,
zero new GPU word: `world.wgsl` is byte-identical.

| ruling | where it lives now |
|---|---|
| The pawn host WALKS — terrain height is GPU-only, so a camera pilot cannot know where the ground is at the destination | `pilot_tick` / `begin_visit`, `direction/input.hpp` |
| The pilot is a third hand on the same wheel: `move_x/move_z` under the fold's clamp, `look_az_delta` at a bounded rate, nothing else | same |
| Provenance is a CPU side-table, written at the two snapshot fill sites, read only beside `is_active` | `SlotProvenance`, `gs.slot_provenance[]` |
| The roll is a window, not a home | `RollView` + `bind_roll` / `bind_clock` |
| The visit is a door with a parameter, taken once at the frame boundary | `gallery_visit` -> `take_visit` -> `begin_visit` |
| The pilot is a spine row, `Pilot`, first in `UPDATE_SPINE`, gated `true` | `UPhase::Pilot`, O-5f |

**AWAITING JEAN:** `glaw1`; the visual rows of U3 (the walk, the swing, the
hands' release, the stall) and U4 (the pane's captions against a real
world); merge. Nothing below substitutes for a frame on a screen.

### What was resolved here that the handoff could only bound

- **The indoor stand-off's sign is NOT a mystery.** `gallery.hpp`'s wall
  record declares `float nx, ny, nz;    // inward normal`, and
  `fill_slot_wall_frame` copies it straight into `s.forward`. So
  `position + forward * off` already lands IN THE ROOM, in front of the
  picture. No sign flip is owed. Still a visual row — a derivation is not a
  frame — but the gate is now confirming a reading, not choosing one.
- **`PILOT_STALL_S` does outlast the landing ease.** The ease is
  `RIBBON_LIVE.land_seconds = 3.6 s` (`board_seconds = 3.0`), against
  `PILOT_STALL_S = 6.0`. Margin 2.4 s. **But `land_seconds` is a LIVE
  DIAL** — raise it past 6 s on the panel and a visit begun from the ribbon
  dies on its own doorstep, exactly as the handoff feared. Either couple the
  two numbers or leave the pilot's stall clock unstarted until `mount_.kind`
  returns to 0.
- **The provenance invariant is closed, not assumed.** Exactly two sites in
  the tree write `ContentSource::SNAPSHOT` — `gallery.hpp:2268` (outdoor,
  inline) and `:3975` (indoor, through `fill_slot_wall_frame`) — and both
  are the sites U1 patched. `fill_slot_wall_frame` is the only other writer
  of `content_source` and its two remaining calls pass `AUTHORED`. So no
  snapshot can reach a wall without its tier and moment.
- **The spine's boot-only laws were evaluated, not trusted.** `Pilot` is
  index 0 and `StageFadeUpload` index 7, so `no_staging_after_drain()` holds
  for an `F_SIGNAL` row; `spine_ordered_u()` holds at 10 rows against
  `COUNT` 10. The static ones (O-5a, O-5e, O-5f, density) compiled.
- **Every cwrap the shell asks for reaches a real export, at the right
  arity** — `gallery_roll` (0), `gallery_visit` (1), `gallery_visiting` (0),
  `shell_pace` (1). The shell gate cannot see this: it reads
  `organ_panel.js`, not `web/index.html`. Checked directly instead.
- **The pane was driven in headless Chromium** against a stubbed ABI: 27
  behaviours, all passing — captions in the documented shape
  (`Cinematic · 9 wu N · 60 min ago · on a wall`), nearest-first ordering, a
  negative age omitted rather than printed as `-1`, the 2 s poll MEASURED
  running and MEASURED stopping on both leave-the-pane and close-the-menu
  (no leaked interval), the empty world's words, and malformed roll JSON
  landing as a note rather than a failure.

### Residuals — VISIT_0

- **The pilot outliving a world teardown — FIXED at DOORS_1 U2.** `pilot_ = PilotState{};`
  now sits beside `camera_pose_ = CameraPose{}` in the TEARDOWN arm, and it
  speaks: `[Visit] released: the world changed (slot N)`. Jean's gate row:
  begin a visit, cross an arch mid-walk, and the pawn stands still.
- **The stride's unreachable quarter floor — FIXED at DOORS_1 U3.** The clamp is
  deleted and the ease is linear; `PILOT_SLOW_WU`'s comment now says what it does.
- **The roll's refresh dropping keyboard focus — FIXED at DOORS_1 U4.** It remembers
  the focused caption's slot and restores it, falling to the first caption when
  that picture has come down. Both measured in headless Chromium.
- **A non-finite slot poisoning the whole list — FIXED at DOORS_1 U5.** `gallery_roll`
  skips such a row; the absence is the artifact, with no per-frame line (P6).
  Truncation was never reachable: 232 of `buf`'s 256 bytes at FLT_MAX, measured.
- The pilot's seven numbers (`PILOT_ARRIVE_WU`, `PILOT_SLOW_WU`,
  `PILOT_PROGRESS_WU`, `PILOT_TURN_RATE`, `PILOT_STALL_S`,
  `PILOT_STANDOFF_MULT`, `PILOT_STANDOFF_MIN_WU`) are control-panel
  material, enrolled nowhere: no measurement has asked yet.
- `PILOT_STALL_S` against the live `land_seconds` dial — see above.
- The bearing words (`compass`) name +Z "N". A convention this shell
  invented, not a fact of the world; nothing else in the program agrees or
  disagrees with it yet.
- The roll refreshes by POLLING, 2 s, because the registry has no event out.
  One JSON string per two seconds while one pane is open, and the timer is
  cleared on both exits. Revisit if the pane grows.
- **The `Pilot` spine row carries a trailing `//` comment.** Zero of the
  other 31 rows in `UPDATE_SPINE`/`RENDER_SPINE` do; in this table per-row
  prose has always lived in the block comment above it or in the census's
  justification string. Written as the handoff specified, and it parses, but
  it breaks a convention held uniformly across both spines. One edit to
  strip it if Jean wants the table clean.
- `FOUNDATIONAL_PHASES`' new value is the dict's first double-quoted string
  (the justification contains an apostrophe, which would close a
  single-quoted one). No neighbour set a precedent either way.
- **The ROLL item — snapshots as a strip of film — WAS NOT IN THIS
  REGISTER TO STRIKE.** VISIT_0 U5 asked for it to be struck as superseded
  by the directory form; a search of `docs/OPEN.md` finds no such item, and
  `docs/` carries no other copy. Nothing was removed. If it lives somewhere
  this executor cannot see, it is superseded: the pictures never leave the
  GPU and the list takes you to them instead of showing them.

### The branch — READ THIS BEFORE MERGING

VISIT_0's handoff calls for a held branch `claude/visit-0` off master, with
DOORS_0 landing on master first. The session's harness pinned all work to
one branch, `claude/handoff-session-vjt33w`, so **both campaigns are on it,
in order and separated by commit**: DOORS_0 is U1..U7
(`7da09fe5`..`c6bbeadb`) and VISIT_0 is U1..U5 after it. The split the
handoff wanted is still available — merging up to `c6bbeadb` takes DOORS_0
alone and leaves VISIT_0 held. Nothing else about either campaign changed.

## DOORS_0 — THE FRONT DOOR, THE SANDWICH, THE PEEK, THE IDLE (landed; Jean's visual gates open)

Seven units, U1..U7. The engine stays at the root and the veil became the
front door.

| ruling | where it lives now |
|---|---|
| The veil is no longer a tap target; `#enter` is the one entry gesture, and a static anchor is the second door | `web/index.html` — `.entry`, `enterBtn.addEventListener` x4 |
| An eager tap is BANKED: the grants are spent at the tap, the reveal arrives at `showReady` through the same handler | `if (entered) onEntryGesture({ type: 'click' });` |
| The card's way out is a link, not a redirect — the log pane is the diagnostic surface a redirect would steal | the second `a.btn.alt[href="/about/"]` |
| One route list, three shells | `web/routes.json` + `web/menu.css` -> `tools/routes.py`, injected by all three dist scripts |
| The sandwich is a native `<details>` — it opens with no script, so the site pages keep their JavaScript-off law | `.menu` on about/, collection/, and `#menu.over` on the shell |
| Idle, not freeze: menu open -> rAF every 4th vblank + a blur on the frame; the soundtrack never rides rAF | `shell_pace` (`src/the_board.cpp`), `html.idle #frame` |
| The peek is written by the page's own records, fetched at the menu's first opening — a gesture, never boot | `dist/collection/peek.json`, `no-cache` in `_headers.fragment` |

`onEntryGesture` and `carriesActivation` are byte-identical: the
activation-critical path did not move, only its target did.

**Witnessed here, beyond the handoff's asking.** The console gate compiles
`the_board.cpp`, so `shell_pace` has a real compile witness, not only
`glaw1`'s. `peek_pick` was exercised end to end against a synthesised
collection (11 works, 3 sets) and unit-tested at its edges: the
`PEEK_COUNT` cap, an over-full featured list, an empty collection, and the
cross-folder `n` collision its docstring names. The sandwich itself was
driven in headless Chromium against a dist-shaped shell — 28 behaviours,
all passing: pane switching, `#menu.paned`, the back button, the idle class
and its blur reaching `#frame`, the peek's ONE fetch across repeated
openings, `--tone`/`--r` on the tiles, the door-out href taken from
`data-href`, Escape closing and resetting, and a missing `shell_pace`
export surviving as a note. **What that cannot reach** is the wasm: no
build ran here, so `web_dist.py`'s marker refusal and the `[PACE]` lines
are still first exercised on Jean's machine.

### Residuals — DOORS_0

- `onEntryGesture` carries two inert `statusEl.removeAttribute` lines
  (DOORS_0 kept the handler byte-identical). Delete at the next shell sweep.
- A key to open the sandwich under pointer lock (desktop): today Esc frees
  the mouse, then the summary is clickable. Needs the key-map census before
  a key is chosen.
- **The menu's key guard is bubble-phase only.** `stopPropagation` on the
  menu subtree stops any listener on an ancestor in the BUBBLE phase, and
  was measured doing so (W/A/S/D/Space/R: 0 of 6 reached a bubble listener
  on `window`). A CAPTURE-phase listener still sees all six. The world's
  keys arrive through emscripten_glfw's own listener, whose phase is not
  readable from this tree — the glue is not vendored. If the pawn moves
  while the menu is open, this is why, and the fix is a capture-phase guard.
- `web_dist.py`'s new marker refusal returns **7**, which the lab-build
  refusal already returns. Both print a distinct message, so an operator can
  tell them apart, but the exit code no longer identifies which refused.
  Written as the handoff specified; 1 and 8 are free.
- The STATE 4 comment's FIRST paragraph still says the status line "stops
  being a report and becomes a door". The handoff rewrote only its last
  paragraph, which now contradicts it in the same comment. Jean's words.
- `web/about/index.html`: `<a class="hero" id="hero" href="../collection/">`
  wraps the full-bleed image — the tap-anywhere pathology, smaller blast
  radius.
- Collection-as-profile: collapse `.lede`, `.index` as a sticky filter, sets
  as filter states over one flow; touch affordance on `.work`; swipe in the
  viewer.
- PWA manifest + Apple meta for a true fullscreen on iOS; fullscreen
  re-entry beyond the menu button.
- Adaptive GPU pacing (gen-8 Intel reference) — TWO_DOORS Task 3, not scoped
  here.
- Copy: the veil sentence, both door labels, `ENTRY_WORDS`, the Controls
  table (verify against `console.hpp`'s touch halves before the words are
  set), route labels.
- Peek count and pick rule (`PEEK_COUNT`, `peek_pick`) are a first
  authoring, to be looked at, not tuned.
- **VISIT_0** (the roll as directory) is a separate handoff and depends on
  DOORS_0 U3/U6 for its pane; its own entry is below.

## LEAP_0 / LEAP_1 — THE PAWN LEAPS, TUMBLES, AND PASSES ITSELF ALONG (landed; Jean's visual gate open)

The pawn's height was a lookup; it is now a lookup on the ground and an
integration aloft, one clock (`agent.t`) saying which. The door is the
pulse's — the ring fires as ever, the body leaps or, once per flight,
somersaults. Four dials under Interaction · Pawn; two shapes
(`PAWN_AIR_LIP`, `LEAP_FLIP_SECONDS`) in world.wgsl. The slope law is the
walk's law only now; the leap is a ladder (any riser under the apex is a
step). Config 720 -> 736.

**Jean's readings, all against the screen:** the tumble's direction along
the heading (negate `turn` if it reads backward); the apex against the
world's risers (3 wu = two pawn heights); a tap on a summit still boards.

**Priced, not built:** hold-to-extend (keyboard-only — the glass tap
resolves at lift); the ring at touchdown instead of takeoff (needs the
P5 harvest, like `portal_trigger`); a CPU airborne sensor to silence the
somersault's ring (R1 kept every tap ringing).

**LEAP_1 — the hand has two words, and both ring.** One finger on the
right half (SPACE) is the body's own verb: the leap, then the flip,
then the taller flip — 3.0 / 4.5 / 6.5 wu, one clock, band 64 — and on
a summit, the boarding. Two fingers on the right half (CAPS_LOCK) is
the reach: the ring goes out and the swap commits when the wavefront
crosses the target, `dist / config.pulse_speed` (PULSE_SPEED
graduated, 0.67 s at the reach's edge). The left half is the stick's
entire. `try_possess_nearest` retired into `find_possess_target` +
`commit_possession`; `on_key_down` shed two parameters with it. Both
bodies are grounded by the handover. Config 736 -> 752.

**Jean's readings, all against the screen:** drive with the left thumb
and tap the right half with a second finger — the leap arrives without
the walk stopping; the lone left tap does nothing; the three rungs
read clearly taller each; two right fingers near a pawn and the swap
lands ON the ring's edge, not before it; leap first, then swap-tap,
and the swap waits for your feet; a mood change mid-wave cancels the
promise in silence.

**Priced, not built:** a double turn on the third rung (a value in the
tumble line); the swap's own voice on the bus; arming from the air
(R2 holds it to the ground); a visible mark on the armed body; an
instant operator swap (R4 retired it — one line to restore).

**PULSE_SPLIT_0 — the leap stops ringing.** LEAP_0's R1 ("every tap
rings; the ring is the tap's word") is REVERSED. `request_radial_pulse`
raises `jump_pending` alone now: one finger on the right half, and SPACE,
are the body's verb and nothing else. The ring has exactly two producers
left — the two-finger word (`request_pulse_swap`: CAPS_LOCK, the
right-half pair tap) and the musical bus (`emit_radial_pulse`). One line
of behaviour; the rest was comment made true again.

**Priced, not built:** the rename — `request_radial_pulse` no longer
requests a radial pulse, and naming is Jean's gate (`request_leap` /
`request_body_verb` are the candidates; ~6 sites). Moving the ribbon
routing off this door: it still rides the leap's word (LEAP_1 R6), which
is why a dismount tap still fires a leap off the saddle.

**LEAP_2 — the latch was the bug, and it predates the campaigns.**
`TouchPoint::alone` was cleared by any finger anywhere, so the stick's thumb
cancelled the solitude of every tap on the far half: the one-finger word
could not be said while walking, from PULSE_1 through LEAP_1, and LEAP_1's
own reading claimed otherwise. It is `solo` now and scoped to the half — the
two halves were already separate rooms for the stick and the look, and are
separate rooms for the latch. The pulse/leap split (PULSE_SPLIT_0, reverted
at `cde4c47`) is restored unchanged: it was never what was wrong.

**Jean's readings:** walk with the left thumb and tap the right half — the
leap arrives and the walk does not stop (this is the reading LEAP_1 owed and
could not have passed); three taps climb the ladder while walking; two right
fingers ring and swap without leaping; one right finger never rings.

**Priced, not built:** slop-aware company, so a look drag in progress stops
being company and a second right finger may leap mid-look (R3); renaming
`request_radial_pulse`, which raises the leap and not the ring since
PULSE_SPLIT_0 (its own register priced this).

**FPV_TAP_0 — the glass carries a camera verb on each half.** Two left
fingers landing and lifting as one toggle FPV; CTRL is the other mouth
and toggle_fpv_mode is untouched. The gesture mirrors the right-half
pair with one added conjunct — the fingers must ARRIVE together —
because a resting stick is unslopped and every aura tap would
otherwise read as half a pair. The aura is unchanged in every case it
owns. `TouchTapLeft` and `TouchTapPulse`'s enum comments were corrected
in passing: the first is a second-finger tap, and the second gained a
side in LEAP_1.

**Jean's readings:** two left fingers together toggle the eye; hold the
stick and tap a second left finger — still the aura, still no FPV;
drive and tap — still the aura; a left pair that drags does nothing.

**Priced, not built:** an FPV gesture on the right half (there is no
room — the lone tap leaps and the pair rings); a visual hint that the
left half carries two verbs; a longer arrival window if two-finger
taps read as auras on Jean's hand.

**REWIRE_0 — the ride answers the pulse.** The host-flip moved from the
leap door to request_pulse_swap: the gesture T7_RIDE teaches is the
gesture that works, both arrivals announced by a wave again, and a
press the ride claims raises no swap. POINT_SUMMIT_RADIUS 15 -> 5 (the
cap is the door). "pulse to land" shows 5 s — the shell's clock,
the edge gate its metronome. Struck: LEAP_1 R6.

**FRUSTUM_0 — every pyramid a frustum, every frustum a door.** Obelisk
x3 (84/48/4.5); truncation retargeted so every cap ≈ 4.8 wu =
POINT_SUMMIT_RADIUS; spawn chance 0.030 -> 0.050 under the ceiling of
8; the summit height gate struck and POINT_SUMMIT_MIN_HEIGHT deleted
with its one reader. The eye's checks: obelisks read as architecture;
every flat top boards; caps read as one width everywhere.

**FOURWALLS_0 — four, always.** wall_count_t3 0.2775 -> 0.0; every
indoor room hangs all four walls. WALLS_1's three-wall argument
struck by Jean's stamp.

**FENCE_0 — indoors the walls are everyone's.** Agents clamp through
world_box_clamp_xz at the kernel finalize, margin
config.pawn_body_radius, identity outdoors. The eye's check: stand
indoors and watch the walkers — none crosses a wall, none jitters
against it.

**HOLD_0 — the ring waits for the feet.** pulse_pending and
swap_pending are spent only when the air clock reads zero; teardown
lowers both. One ring per landing however many presses.

**SPREAD_0 — the obelisk opens.** Base half 48 -> 84 (flank 60 -> 45
degrees, walkable); truncation 0.10 -> 0.06 (cap 5.04 wu = the door);
aspect sigma tightened.

**EASE_1.** Mount eases 2.55 / 3.06 s (15% faster, ratio kept);
"pulse to land" shows 8 s.

**CAP_0 — the antenna's turning shadow.** Diagnosed: the disc pass
restated the lathe's two flat annuli per drum with inverted normals;
CullMode::None drew both; the dark twin won a view-dependent depth
fight. Struck: both shoulder discs and their loop. Priced: the
classical columns' step discs fight the same way, invisibly.

**SPORE_0 — the sea is never still.** Uniform 3x3 cells roll one coin
per 5x5 block per tick (0.75 spores per zone per tick, beat-clock
entropy) and a hit flips the block — sized to persist under Vote,
boil under Conway. Spawn chance 0.60 -> 0.70. Jean's dials:
GOL_SPORE_PER_ZONE_TICK, GOL_SPORE_BLOCK (world.wgsl).

**RISE_0 — doors across themes.** Pyramid spawn_weight 0.8 / 1.5 /
0.8 / 0.8 / 0.7 (was 0.4 / 1.5 / 0.3 / 0.5 / 0.4); SPAWN_CHANCE holds
at 0.050 against the ceiling of 8. Fresh-world reading.

**MASSIF_0 — the plains recede.** Continental activation 0.70 ->
0.91, tectonic 0.75 -> 0.975 (+30% each); amplitude untouched (the
named flip if the stamp meant height). Fresh-world reading.

**SHIFT_0 — the aura on Shift.** Either Shift, down-edge with a held
guard (REPEAT would flicker under a resting finger); key 3 dark; the
glass tap unchanged; four door-naming comments follow the move — and a
fifth the campaign did not name: `console/organ_params.inc`'s Pawn-Aura
banner carried the twin of the `driver_surface.hpp` sentence, and took
the same correction, so no comment in the tree still points at key 3
except the two that record it as history on purpose.

**ARROWS_0 — arrow-key look.** Four held flags in WASD's grammar;
apply_arrow_look pumps ±1.6 rad/s onto look_az/el at the signal fill;
mouse kept, signs matched, one channel downstream. The signs were
checked against the mouse, not assumed — a rightward drag does
`look_az_delta -= dx` and `look_right` does `az -= 1`; a downward drag
does `look_el_delta += dy` and `look_down` does `el += 1`; the thumb's
look uses the same convention, so all three hands agree. The dt is
`dtPending_`, settled four lines above the pump: the frame's dt as the
GPU will see it, capped at 100 ms, so a stall cannot spin the camera.

**FREECLIMB_0 — the slope law on trial.** The one call site bypasses
`pawn_ground_resolve`; the law stands compiled, witnessed and defended
(W4-2 green, its trigger set unchanged); every grade is a stair for the
trial; the happy path drops from two paired terrain queries to one single.
**Revert = `3edd467f`** — `git revert --no-edit 3edd467f`, rebuild, and the
law resumes byte-identical; nothing else moved in that commit. Priced
follow-ups if the trial convicts: the 5.67 value stamp (three anchors) or
the full strike (twenty-six, tool estate included).

**The order's draft claimed one thing that is not true, and the banner as
landed says the true one instead.** "Driver and driven share
`agent_settle`'s physics for the first time" — they do not. `agent_settle`
snaps through POLICY_WALKER_AGENT (`query_ground_walker_agent`: the full
GoL lift, the aura sampled externally at the body's own XZ). The trial
gives the pawn POLICY_WALKER (`query_ground_walker`: GoL self-suppressed
under `pawn_gol_suppression`, the aura its scalar peak — and
`contrib_pawn_aura_at_self`'s banner says why the pawn must not sample the
grid at its own XZ: the directional bias reads as bobbing). What the trial
takes away is the SLOPE LAW that separated driver from driven, not the two
contributors that still do. Jean's reading 4 stands as written — both climb
any grade now — but the symmetry it asks about is of SHAPE (a snap, not a
resolve), not of policy.

**Nine comments now point at a law nobody calls, and not one was touched.**
`world.wgsl` names `pawn_ground_resolve` in nine comments outside its own
banner: the steering band above `STEER_GAIN`, the occupier row's ONE
consumer, the occupier's inline branchless note, the two "Typical
consumers" rows on POLICY_WALKER and POLICY_WALKER_TILT, the paired-query
note, THE WIRE, the LEAP_0 air-clock banner ("on the ground Y is a lookup
and the wall is a grade"), and the player kernel's roster. The order
counted them and chose ONE probation notice on the resolve's own banner
over nine edits, because nine edits are nine seams the revert must cross.
That is the right trade, recorded here so the next reader need not
rediscover it: for the duration those nine describe a law on probation, and
the notice at the function says so.

## SKIRT_WELD_1/P — THE PERIMETER SKIRT HANGS FROM THE BASE BAND (landed; one seam held)

Sibling of SKIRT_WELD_1 on 7T-Music, landed there at `be0eb28f`. The patch
perimeter skirt hung its top edge on **cap** verts, and cap verts are
cell-owned (UNIFIED_GROUND_1): ring slot `k` and `k+1` straddle a cell seam at
every multiple of `UG_QUADS_PER_CELL`, so one live zone cell turned that quad
into a ramp from `ground + alive_height` down to `ground` — an extra triangle
dragged up by a cell it does not belong to, over a curtain that had already
sealed the same edge. The ring now hangs from the **base** band, whose twins
carry `wall = 1 → lift_scale = 0` (WALL_1) and never lift. **Zero mirrors, zero
bindings, zero organ rows, and the four index counts are byte-identical** — the
emission pushes six indices per quad either way, so only the index VALUES moved.

**1 · THE INVERSE IS ASSERTED, NOT ASSUMED.** `cell_perimeter_slot` (the new
`(lx,lz) → k` inverse of `cell_perimeter`) is `constexpr` and carries a
sixteen-term `static_assert` that is simultaneously a round-trip check and a
bijection proof: the sixteen results are `0..15`, each exactly once. Stride-2
totality comes free — LOD1's locals are the subset `{0,2,4}`, and a function
total over all 16 perimeter positions is total over any subset.

**2 · ALL THREE EMISSION PATHS GOT THE SAME RE-AIM.** `build_lod0_ib` is one
lambda serving the curtained and cap-only buffers, so LOD0's single call-site
pair covers two of them; LOD1's stride-2 ring is the third. The curtain-less
variants are the two lift-conservative switches (`curtainsActive_` off
`zone_rects_in_core()`, `zonesActiveAnywhere_` off `zones_active_anywhere()`),
not dead alternates.

**3 · A STALE CLAIM DIED WITH IT.** `cartridge.hpp` said *"In the LOD1 ring
cells lift and own no curtain; what seals those seams is the rim curtain
(WALL_1 — skirt ring copies stand on unlifted ground)."* False at HEAD: LOD1's
zoned build appends a full per-cell stride-2 curtain tail after the clean
prefix. It was **the one document in the tree saying the skirt ring performs
sealing work**, and a careful reader would have refused this fix on its
authority. Corrected in the same commit as the re-aim. Prose is Jean's gate.

**4 · THE TINT'S VARIATION IS A GAIN NOW, NOT AN OFFSET.** `apply_gol_color`'s
BLACKISH branch added `vec3(r_shift, g_shift, -r_shift)` **after** the
darkening multiply. On near-black ground the base term is ~0, so the offset
became the whole colour and the `clamp` beneath it deleted G and B
asymmetrically — a cell holding max `r_shift` painted saturated red out of
black ground. As a gain the scatter is proportional to what is there. The clamp
is EXCISED: ceiling `0.55 * 1.05 = 0.578` against a base ≤ 1, floor ≥ 0 because
every factor is. Neither `_SHIFT_RANGE` value moved. The edit is inside the
branch that reads neither `zp` nor a per-zone field, which is why it is
byte-portable between the two trees.

**5 · ONE RESIDUAL, AND ONE PREDICTED RESIDUAL THAT DOES NOT OCCUR.**
- `in.skirt` was `0 → 1` across a skirt quad (cap top, ring-copy bottom) and is
  now the constant `1` (both ends carry `wall = 1`). `DEBUG_VIEW = 3u`
  therefore paints the whole skirt quad magenta including its top edge —
  truer as an instrument, and a changed picture in that debug view. Sole
  consumer; the art does not move.
- **The skirt's baked cell colour does NOT change.** The handoff predicted it
  would, on the premise that skirt fragments took `owned_texel` from the
  floored world position (`cell_local.z == 0`) and would now take the flat cell
  index. They never took the floored position: `cell_local` is
  `@interpolate(flat)`, whose provoking vertex is the primitive's first, and
  the emission is `a, b, sa` / `b, sb, sa` — `a` and `b` were cap verts
  (`z = 1`) before and are base verts (`z = 1`) after. Both bands decode the
  same `cellx`/`cellz` for the same ring slot, so `owned_texel` is bit-identical
  across the change. What survives is the per-triangle cell **step** at a seam
  — `a` in cell *c*, `b` in cell *c+1*, split along the quad's diagonal — which
  was true before the re-aim too. The re-aim removed the ramp it was riding on.

**6 · HELD — THE SHADING NORMAL EXCLUDES THE ZONE LIFT.** Both terrain VS sum
`out.gradients = height_data.yz + live.yz`; the lift rides the card's `.w`,
nearest-sampled at the cell centre, and contributes nothing to the gradient.
Every cap and curtain is therefore lit with the flat ground's normal. Real,
second-order, and superseded by SKIRT_WELD_1/P — the picture must be re-shot
with the ramps gone before this is scoped, because the ramps were producing
tone splits of their own. Held identically in 7T-Music.

**7 · THE RECORD RITUAL WAS NOT RUN, DELIBERATELY.** The handoff asks for
`glaw2 --record`. One name retires here — `skirt_cap_index` — and it is
**C++**, which glaw2 does not see; the campaign adds no WGSL entry point and no
WGSL const, and glaw2 is GREEN without a re-record. The tombstone is the
diff's. The handoff also predicted `sha256_gate` would red by construction: it
does not. That gate proves `src/core/sha256.hpp` agrees with `hashlib` — it
uses `world.wgsl` as an input vector, it does not pin a recorded digest — and
it is PASS on the edited shader with nothing to re-record.

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
| D9 | (unstated) `gallery.hpp` is free to edit | it is an **enforced sha256 pin** in `audit/BINDING_LEDGER.md` and `audit/COMMAND_LEDGER.md` (`sha256:b77b6c07…` **as of that recon** — the digest moves with every edit to the file, so compare against `sha256sum`, never against this line). Any byte change reds both until the generators re-run, and `MIRROR_LEDGER` pins `BINDING_LEDGER`, so the cascade is three tools in order: `binding_ledger.py` → `command_census.py` → `mirror_census.py` | **flag — a ledger commit is owed at the campaign's end** |

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

## REPEAT_0a — THE SEQUENCE HEALS, THE WINDOW SPEAKS

### U0 — WHY 33 NAMES HAVE NO FILE · CC's half

**The order's hypothesis is FALSIFIED, and it could not have been true.**
`tools/web_dist.py` writes the manifest and copies the files **from the same
list**: `main` calls `paintings = list_paintings()` once, hands that list to
`write_paintings(paintings)`, and later writes
`json.dump({"paintings": paintings, "music": music}, …)` from the same
variable. There is no second listing, no extension filter on one side only,
no subfolder walk. A name in the manifest with no file beside it in `dist/`
cannot be produced by this script.

**RECIPE, ADAPTED, AND WHY.** The order's recipe reads `dist/`. This container
has no build — `web_dist.py` reaches `BUILD FIRST — 3 artifact(s) absent` and
returns before `shutil.rmtree(DIST)`, so no `dist/` is ever written here. The
script's own functions were driven directly instead, which tests the same
thing more sharply — it exercises the real copy rather than inspecting its
output:

    python3 - <<'EOF'
    import os, importlib.util
    spec = importlib.util.spec_from_file_location("wd", "tools/web_dist.py")
    wd = importlib.util.module_from_spec(spec); spec.loader.exec_module(wd)
    wd.DIST = "<scratch>"; wd.DIST_PAINTINGS = os.path.join(wd.DIST, "paintings")
    names = wd.list_paintings(); wd.write_paintings(names)
    got = sorted(os.listdir(wd.DIST_PAINTINGS))
    print(len(names), len(got), sorted(set(names) - set(got)))
    EOF

Result: **57 names in, 57 files written, zero missing, zero zero-byte, all 57
beginning `ff d8`** (a real JPEG SOI marker). `web_dist.py` is exonerated by
execution, not by reading.

**THE SOURCES ARE ALL THERE, TOO.**

| check | recipe | result |
| --- | --- | --- |
| files present | `ls assets/paintings \| wc -l` | **57** |
| all tracked by git | `git ls-files assets/paintings \| wc -l` | **57**, and `git ls-files --others --exclude-standard assets/paintings` is empty |
| no LFS or filter | `grep -inE "lfs\|filter" .gitattributes` | none — `*.jpeg` is marked `binary`, nothing more |
| all real JPEGs | first two bytes of each == `ff d8` | **57/57** |
| when added | `git log --diff-filter=A -1 -- <each>` | **all 57 in one commit**, `f52d8c16`, 2026-04-04 |
| dist is rebuilt, never patched | `grep -n "rmtree" tools/web_dist.py` | `shutil.rmtree(DIST)` then `os.makedirs(DIST)` — a stale `dist/` is impossible |

**So the loss is DOWNSTREAM of `dist/`: the deploy, the CDN, or the client.**
It is not in this repository.

**THE COUNT, RECONCILED (a FLAG on the order's arithmetic).** The order names
the failing set as `70, 71, 104–115, 200–214, 500, 501, 900, 1001, 1002` and
calls it 33. Read literally that is 34 names — but `PAINTING_204` does not
exist in `assets/paintings`, so `200–214` is fourteen files, not fifteen, and
the set is **exactly 33**. The order's set and the console agree.

**AND THE SHAPE OF THE FAILING SET IS THE FINDING.** In `list_paintings`'
numeric order (index : name):

    0..15   PAINTING_1 … PAINTING_60      LOAD
    16,17   PAINTING_70, PAINTING_71      FAIL
    18..25  PAINTING_72 … PAINTING_103    LOAD
    26..56  PAINTING_104 … PAINTING_1002  FAIL

Sixteen plus eight is the twenty-four that load; two plus thirty-one is the
thirty-three that fail. **The failures are a contiguous TAIL in request order
plus one two-wide notch — a shape that tracks WHEN a request was issued, not
WHICH file it asked for.** Nothing distinguishes `PAINTING_70` from
`PAINTING_72` in the tree: same folder, same extension, same commit, both
valid JPEGs, 165 KB against 188 KB.

That leaves exactly two live hypotheses, and they are not distinguishable
from inside this repository:

- **(A) THE FILES ARE NOT ON THE SERVER.** The deploy dropped or never
  uploaded 33 of 57. Then `HTTP 0` is the CDN answering a missing object in a
  way that is not a 404 — a redirect to an error page on another origin, or a
  connection closed before a status.
- **(B) THE FILES ARE THERE AND THE REQUESTS ARE BEING REFUSED.** A rate
  limit, a WAF rule, or a burst ceiling cuts the client off after roughly two
  dozen image requests. `HTTP 0` is emscripten's verdict for a request that
  never got an HTTP status at all — a reset, a CORS refusal, an abort — which
  is what a refused burst looks like, and which is *not* what a 404 looks
  like. The two-wide notch at 70/71 is what a four-lane race looks like.

**ONE TEST SEPARATES THEM, AND IT IS JEAN'S** (this container's egress policy
blocks `everexpandingboard.com` — `curl` returns `CONNECT tunnel failed,
response 403` — so CC cannot run it):

1. Open `https://everexpandingboard.com/paintings/PAINTING_104.jpeg`
   directly. **200 and a picture ⇒ hypothesis B.** 404, or an error page,
   ⇒ hypothesis A.
2. Open `https://everexpandingboard.com/exhibition.json` and count. It should
   name 57.
3. In DevTools → Network, filter `paintings/`, and read the failed rows'
   status column: `404` is A; `(failed) net::ERR_…` is B.

> **SUPERSEDED 2026-09-02 — REPEAT_0c U-S2 TOOK THE REVERSAL.** Jean's field
> measurement came back (B): the ghosts were a dying dev server, not absences.
> A failed fetch now sleeps for `AUTHORED_DEAD_COOLDOWN_S` (30 s) instead of
> dying for the session, and `authored_manifest_dead` (`std::vector<bool>`) is
> now `authored_manifest_dead_until` (`std::vector<float>`). The paragraph
> below is kept because its reasoning is what licensed the reversal — read it
> as the argument, not as the live rule.

**AND THE ANSWER CHANGES R1, WHICH IS WHY IT MATTERS BEFORE U1 SHIPS.** R1
rules that a failed fetch is dead for the session, priced on "a picture that
is not there this minute is not there in ten." That is exactly right under
(A). **Under (B) it is false**: the pictures are there, the refusal is
transient, and session-scoped death converts a recoverable burst into 33
paintings permanently lost until reload. U1 lands as ruled — it is the
correct degradation for the case the order names, and it is strictly better
than the per-lap retry either way — but if Jean's test returns 200, R1's
one-counter reversal (R4's timestamp-and-reset, priced below) stops being a
horizon item and becomes the next unit.

### U0b — NOT A FILTER FIX; A WITNESS

The order reserves U0b for a mechanical fix to `web_dist.py`'s copy. **There
is no mechanical fault to fix** — proven above. What the script lacks is not
correctness but a *witness*: it prints `WROTE dist (N files)` and never says
whether every name it wrote into `exhibition.json` has a file beside it. Had
it said so, this campaign's whole question would have been answered at dist
time, on Jean's machine, months ago. Instrument before mechanism: U0b adds
the assertion instead of the filter it turns out not to need.

### R4 — OUTAGE RECOVERY — **CLOSED 2026-09-02, BUILT AS REPEAT_0c U-S2/U-S2b**

Was: priced at one counter and one reset, not built, waiting on a field
measurement. The measurement came back (B) and it was built — but not as
priced. A stamp *per index* rather than one session-wide, denominated in wall
seconds rather than in laps, plus `authored_pump_cooled_layers`, which the
pricing did not foresee and which is the half that actually recovers: the
reset the old entry describes has no caller in the state it exists for. See
§ REPEAT_0c / REPEAT_2 below for the measurements and for what is still open.

### THE ≤32 NO-OP REFILL IS UNAFFECTED

REPEAT_0 skips a refill when the cursor names the painting the popped layer
already shows, which fires whenever head and cursor advance in lockstep —
that is, whenever the manifest is no larger than the ring. With **24 live
indices in a 32-layer ring, 32 mod 24 = 8**: each layer's target index shifts
by eight per lap and the two never stay aligned, so the no-op never fires and
every refill is a request. After lap one they are browser cache hits.
Nothing to optimize, and nothing the dead mask breaks.

### WHAT U1, U2 AND U3 CORRECTED IN THE ORDER

- **FOUR CURSOR CONSUMERS, NOT THREE (U1).** The order names the boot fill,
  the pop's append and the healing re-append. The fourth is `authored_pop`'s
  HOLE-HEAL arm, added by REPEAT_0 U7c after the order's snapshot — a distinct
  cursor consumption on a mutually exclusive path inside the same function. A
  patch applied to "the three writers" would have left it handing dead indices
  to the ring forever. The cure was not three patches but one verb:
  `authored_take_next` is now the only read-modify-write of the cursor in the
  tree, so there is no fourth to forget.

- **R2's ARITHMETIC IS WRONG, AND THE DELETION STILL STANDS (U1).** R2 says
  the mask bounds a session to at most `manifest_size` failing fetches, making
  the slip budget unreachable. Measured, an all-dead 57-entry manifest costs
  **88** failing fetches, not 57: an index can be handed to more than one ring
  layer before its first failure marks it dead, and each of those handouts
  fails on its own. The budget is still deleted, because R2's actual test is
  whether it guards a case the mask does not — and it does not. The budget
  bought TERMINATION against an origin that has gone away; the mask buys the
  same thing and buys it better, since it also stops asking for the specific
  things that are not there. Bounded is the property that matters, and bounded
  is measured.

- **THE DEAD MASK, MEASURED (U1).** The pop, the take, the next-live walk, the
  fill and the heal were transcribed character-for-character into a model and
  driven against the real 24/33 split:

  | case | result |
  | --- | --- |
  | the live defect — 57 named, 24 on the server | boot 65 fetches, **exactly 33 failures, one per ghost**; then 420 fetches over 60 rooms with **zero** failures; all 24 live paintings exhibited, never a dead one |
  | every entry dead | 88 fetches ever, exhaustion logged **once**, then not one further fetch for the session — *superseded by U-S2/U-S2b: the sentence now repeats at most once per 30 s cooldown, and the pump does keep asking* |
  | a single ghost | **one** failed request, ever; the other 56 all hang |
  | an honest manifest | zero failures, indices still ascend by one and wrap — REPEAT_0's gates 2 and 3 intact |
  | two live entries of 57 | only the pair is hung, 82 failures, then silence |

- **EXHAUSTION NEEDS NO SPECIAL CASE (U1, R3).** R3 asks that popped layers go
  invalid when the sequence dies. They do by construction: an index is marked
  dead only by a failure, and an index that ever loaded never failed, so total
  exhaustion implies nothing ever arrived and no layer was ever valid. In the
  mixed case — an origin that goes away mid-session — the layers keep the
  pictures they hold (WALLS_2) and the ring keeps cycling them, which is the
  graceful half of the same rule and is better than bare walls.

- **THE `ExhibitionName` STRUCT IS NOT CEREMONY (U2).** The order asks for a
  bare `uint32_t exhibition_disk_index[…]` defaulting to UINT32_MAX. A plain
  member array value-initialises to ZERO, zero is a legal disk index, and C++
  gives no way to fill a member array with a non-zero default — so forty
  layers would have booted claiming to hold painting 0 and `hung` would have
  read 40 before a single painting was up. A one-member struct with a member
  initialiser is the same idiom `AuthoredStagingRecord` already uses, for the
  same reason.

- **`ready` IS A GAUGE, NOT A TALLY (U3).** Read after the pop, it is the
  contiguous depth from the head, so it reads ring−1 while the append that
  refills the vacated layer is in flight and ring once it lands. It oscillates
  just under the ring in a healthy exhibition and only SINKS when the sequence
  is in trouble. A near-constant reading is the good news.

- **THE EVICT BURST IS PER WORLD TRANSITION, NOT PER MOOD KEYPRESS (U3).**
  TEARDOWN has two entry doors — `request_mood_transition` and
  `phase_portal_trigger` — and on everexpandingboard.com there is no keyboard,
  so the portal is the only one a visitor walks through. The ceiling is 40
  lines. The order's comparison holds but its number was low: the rotation era
  spent up to **97** lines on the same boundary, not 64.

- **R10's COUPLING LIST IS SHORT BY ONE.** `web/index.html` parses a THIRD C++
  sentence: `[Authored] Loaded:` drives the veil's
  "Hanging the paintings (n/total)" counter. It is emitted by
  `pump_authored_valve` — inside a function this campaign edits — and was not
  touched. The full guard set is sixteen `indexOf`/`match` tests, and both new
  REPEAT_0a sentences were checked against all sixteen.

- **AND ONE PRE-EXISTING MIS-PARSE, FOUND WHILE CHECKING R10.** The failure
  sentence `[Authored] No paintings folder found (exhibition.json, HTTP 404)`
  satisfies the shell's `indexOf('found') && indexOf('paintings')` test,
  fails the `/found\s+(\d+)\s+paintings/` regex behind it, and still calls
  `say('Hanging the paintings')` and returns. So a manifest that 404s tells
  the visitor the paintings are being hung. Not this campaign's to fix — the
  sentence is one R10 protects — but it is the exact shape of coupling R10
  exists to warn about, and it is one word away from capturing four more
  sentences, since every URL-bearing `[Authored]` line already contains
  "paintings" by way of `EXHIBITION_PAINTINGS_DIR`. Unblocked by any campaign
  that opens the veil's classifier.

### WHAT THE REFUTER FOUND ON REPEAT_0a (P3)

Five lenses over the landed diff — the dead mask and the cursor, liveness and
exhaustion, the exhibition name and its count, ordering and re-entrancy, and
the web_dist witness — each finding handed to an independent skeptic told to
refute it. The web_dist lens returned **zero findings**. Of the rest:

- **CONFIRMED, AND PAID: THE LEDGER DEBT.** Three lenses independently found
  that the campaign edited `gallery.hpp` and had not yet regenerated the
  ledgers that pin it. Paid in the closing commit. **And the verifier found a
  gap in the gate table while checking it:** `tools/binding_ledger.py --check`
  reports `STALE` and exits 1, but `tools/command_census.py --check` **exits 0
  while carrying the same stale digest** — it writes `sha256:` rows and has no
  pin-comparison logic at all. So `COMMAND_LEDGER.md` can be silently wrong
  with every gate row green. That is why D9's three-tool cascade is right and
  the STALE message's two-tool advice is short by one: `command_census` must
  be re-run even though nothing complains when it is not. **Unblocked by a
  gates round that gives command_census the pin check its siblings have.**

- **REFUTED, AND IT SAVED A REGRESSION: THE EVICT LINE STAYS UNGUARDED.** One
  lens found `[Authored] Evict` firing from the patch-streaming path — true —
  and proposed putting it behind `INSTRUMENTS.stream_witness`, which is where
  this file puts steady-path chatter. The refutation holds on three checks:
  `stream_witness` governs a NAMED family (`core/instruments.hpp`:
  "`[Ribbon] SPAWN/REJECT/EVICT`, `[Gallery] slot=` and `[Agents] Respawn`")
  that `[Authored]` has never been in; `[Authored] Pop` is already unguarded
  on the same patch path and carries its own note calling it a standing
  exhibition-guard candidacy; and evict lines are bounded 1:1 by pop lines,
  since the pop is the name's only writer. **Decisively: guarding only the
  evict half would restore the exact asymmetry R6 was written to close** —
  `hung=` climbing on an unguarded line and falling on a silent one. If the
  volume ever wants quieting it is a family-wide ruling taken at both ends at
  once, which is the candidacy the code already records. **Unblocked by Jean
  hearing it and disliking it.**

- **REFUTED, AND WORTH ONE NUMBER: THE 30 s TIMEOUT.** A lens argued the fetch
  timeout is a fifth road into the dead mask, so a slow phone would delete
  paintings that exist. It is not a fifth road — the timeout routes into
  `authored_image_onerror`, which is road one, and REPEAT_0 U7 already said so
  in place. The hazard class it names is the one `authored_manifest_dead`'s own
  comment and R4 above both enumerate. (That symbol is now
  `authored_manifest_dead_until` — REPEAT_0c U-S2.) The useful residue is the
  magnitude:
  for the timeout to fire on a ~500 KB painting the link must sustain under
  ~17 KB/s per lane against four lanes — roughly half a megabit aggregate,
  which is a link on which the exhibition cannot run at all. R4's one-counter
  reversal covers this arm for free if it is ever built. **It was built —
  REPEAT_0c U-S2 — so this arm is covered: a timeout now costs one painting
  for 30 s rather than for the session.**

## REPEAT_0b — THE ROOM THAT WOULDN'T LEAVE

### PHASE 0 — RECON. THE PREMISE DOES NOT SURVIVE THE TREE.

The order's subject is a cross-world leak: hung exhibition layers outliving
their world. **Recon finds no such leak, and finds that none is structurally
possible.** The order says recon decides and the log only accuses; this is
recon deciding.

**G1 — THE CHOREOGRAPHY, FROM CODE. There is ONE boundary of record and its
order is the opposite of the one the log was read as showing.**
`phase_transition_machine`'s `TEARDOWN` arm, in a single frame, in this order:

| # | what runs | what it prints |
| --- | --- | --- |
| 1 | `world_state_.world_gen++` | — |
| 2 | `become_destination(pendingDestination_)` — the new seed | — |
| 3 | `reset_surface`, `teardown_entities`, the per-organ teardown verbs | — |
| 4 | **`teardown_gallery`** — releases ALL `Dim::EXHIBITION_LAYERS` | **the evict burst, `hung` counting to 0** |
| 5 | pose reset, agent reset, `set_world_seed` | — |
| 6 | **`apply_mood`** → `apply_mood_indoor_shell` → `place_wall_paintings` | **the new world's pops** |
| 7 | still inside `apply_mood`, after the hang | `[Mood] Applied: …` |
| 8 | end of the arm | `[World] Teardown complete, seed=…` |

**`[Mood] Applied` is printed AFTER the hang, not before it** — it is the last
statement of `apply_mood`, and `apply_mood_indoor_shell` (which reaches
`place_wall_paintings`) is called twenty lines above it. So **a pop line above
a `[Mood] Applied` line belongs to the world that line announces, not to the
one before it.** The order records this ambiguity as observation (a), at low
confidence, and then reasons from the opposite reading. Every number in the
accusation follows from that inversion:

- line 616 `Pop … hung=28` is the NEW room filling, not the old one lingering;
- line 655 `hung=29` and 711 `hung=33` are that same room plus the outdoor
  galleries dressing as patches stream in;
- line 845 `Evict exh=0 disk=37` is not a late release. `[World] Transition`
  is printed by `request_mood_transition`, which only *requests* the
  transition and sets `FADE_OUT`; teardown happens seconds later, after the
  fade. An evict one line after it is the streaming evictor releasing a
  CURRENT-world gallery, and `disk=37` belongs to the current world's lap.

**G2 — THE SKIPPED-STRUCTURES HYPOTHESIS IS REFUTED.** The order asks whether
`teardown_gallery` walks slots, frames and centres rather than the layer
array. It walks both, and the layer walk is total and unconditional:

    // Free all exhibition layers …
    for (uint32_t i = 0; i < Dim::EXHIBITION_LAYERS; i++)
        release_exhibition_layer(gs, i);

`release_exhibition_layer` clears `exhibition_name[exh].disk_index`
unconditionally. **No layer, hung by any road, can survive that loop** — the
release does not consult the slot, the frame, the centre or the patch. R3
asks that the release pass of record iterate the layers; it already does.

The diff G2 asks for closes to nothing: every writer of the name array is
`authored_pop`, whose single call chain is the two hang roads, and both are
covered by a loop that ignores roads entirely.

**G3 — NO LIGHT PATH.** `request_mood_transition` has no same-mood
short-circuit — its only early returns are `phase != IDLE` and
`mood >= MOOD_COUNT`. Both entry doors (`request_mood_transition`,
`phase_portal_trigger`) set `FADE_OUT` and converge on the single `TEARDOWN`
arm. `teardown_gallery` has exactly ONE caller. Every transition takes the
same teardown, whatever the mood or the size.

**AND THE LEDGER RECONCILES WITH TOTAL EVICTION.** 257 pops − 227 evicts = 30,
which is exactly the `hung` the capture ends on: the final world, still
standing because the capture stopped before its teardown. A partial eviction
is not reachable — the loop is unconditional — so "some transitions evict
partially" cannot describe this tree. The excerpt is non-contiguous (line 616
to 619 to 650 to 655 …), and the evict burst for the boundary in question
sits above its window, before the pops at 616.

**G4 — THE WORLD SERIAL ALREADY EXISTS, AND IT IS ALREADY AT THE BOUNDARY OF
RECORD.** `WorldState::world_gen` (`contracts/surface_services.hpp`) is
incremented exactly once per world rebuild, as the FIRST statement of the
`TEARDOWN` arm, and is described in place as the P5 stale-callback guard. U1
therefore introduces no counter: it carries the one that exists into the
layer. `gallery.hpp` already reads `c->world_state_` at six sites, so both
hang roads can reach it. The choke point for the law is `authored_pop` — the
one supply verb, which every authored hang passes through.

**G5 — THE MOOD MONOTONY IS THE OPERATOR, NOT THE CHOOSER.** `[World]
Transition (…)` is printed only by `request_mood_transition`, the KEYBOARD
door, which takes the mood as an argument — the key pressed. The weighted
chooser `pick_mood_weighted_` is reached only by `pick_portal_mood` /
`pick_open_mood`, for PORTAL destinations, which announce themselves as
`[Portal] GPU trigger` instead. **Eight `[World] Transition (indoor_flat …)`
lines are eight presses of the same key.** The varying 3x3/5x5/7x7/9x9 is
`derive_finite_radius(dest_seed, …)` over a fresh seed each time, exactly as
designed. For the record, the chooser is not pinned:
`mood_weights = { 0.20, 0.20, 0.20, 0.10, 0.15, 0.15, 0.02 }`, so
`indoor_flat` carries ~20% — eight independent picks in a row would be about
five in a million. Nothing to repair; R5 is answered and closed.

**G6 — THE DEAD ATRIUM PROSE IS THERE, AND IT DIES.** Verbatim above the
release loop: `// Free all exhibition layers (staging persists across worlds)
— except // the ones the atrium holds, which outlive every world (ATRIUM_7).`
The code has no exception and the atrium has no holder — `ATTIC_ATRIUM` D1
deleted it. Struck under R1.

### AND ONE REAL FINDING THE ACCUSATION WALKED PAST

**`hung` COUNTS AUTHORED LAYERS ONLY, SO IT UNDER-REPORTS OCCUPANCY — and
that is why the order's visual gate 4 would fire spuriously.** A snapshot
claims an exhibition layer and is never named by `authored_pop`, so it is
occupied and invisible to `hung`. At log line 711, `exh=39` allocating with
`hung=33` is not an allocator fault: it is 33 authored layers plus 7 snapshot
layers making 40 of 40. **Gate 4's rule — "`exh=39` never allocates while
`hung < 35`" — is unsatisfiable by construction and should not be run.**

What IS real underneath it: **the exhibition array can saturate, and when it
does the failure is nearly silent.** `find_free_exhibition_layer` returns
`UINT32_MAX`, the row ends, and only `[WallPainting] BARE WALL` says anything
— and only when a whole wall got nothing. A room thinned by layer exhaustion
rather than by supply reads identically to a small room. The cure is one
derived count on the pop line (`occ=`, the `exhibition_occupied` tally beside
`hung=`), which would have made this distinction free. **Priced at one line,
not built — it is outside this order's units.** Unblocked by REPEAT_1, which
needs the headroom arithmetic (40 − room size = concurrent fades) to be
readable anyway.

### WHAT LANDS, UNDER THE ORDER'S OWN STOP CONDITION

G2 refutes the hypothesis and names no alternative road, so the order's first
stop condition governs: **land U1 and U3, FLAG U2, stop at the instrument.**

- **U1 lands** — and it is the unit that matters, because the campaign was
  misdiagnosed for exactly the want it fills. `w=` on the pop and evict lines
  makes per-world attribution a fact of the log instead of an inference from
  print order.
- **U2 does not land as a mechanism.** There is no structure-walking loop to
  replace; the total layer walk R3 specifies is already the code. Its prose
  half lands: G6's dead atrium clause is struck under R1.
- **U3 lands** — the law, armed. After it, a leak names itself. It should
  never print, and the recon above says why it cannot; if it ever does, that
  capture is the next campaign's first exhibit and the accusation was right
  about something recon could not see.

### STILL OPEN, RE-RECORDED

- `command_census.py --check` exits 0 while carrying a stale digest — no
  pin-comparison logic at all, so `COMMAND_LEDGER.md` can be silently wrong
  with every gate row green. Found by the REPEAT_0a refuter. **This campaign
  does not fix it; it re-records that it exists.** Unblocked by a gates round.
- The wrangler dev-loop fragility (R6): a `Dead`/`Failed` burst under
  `pages dev` is a server symptom before it is a tree symptom. The CDN serves
  57/57; the ghost file is closed.

### U4 — WHAT THE REFUTER FOUND, AND THE CORRECTED VISUAL GATE

Five lenses over the landed diff, the first two aimed at the campaign's own
no-leak conclusion, each finding handed to an independent skeptic. **The
conclusion survived every attack on it**: no lens found a road by which a
layer named by world N is still named when world N+1 begins to hang, and the
boot path (which never runs `teardown_gallery` because nothing is hung yet)
was checked and is clean. Roster gating is symmetric — `teardown_gallery` is
gated on `ROSTER.gallery || ROSTER.indoor_shell`, the UNION of both hang
roads — so no build configuration hangs a painting it cannot tear down.

**Two findings against U3 landed, and one move fixed both (U3b):**

- **SATURATION DISARMED THE LAW.** Living inside `authored_pop`, the law was
  unreachable whenever `find_free_exhibition_layer` returned `UINT32_MAX` —
  because both hang roads `break` on that BEFORE calling the pop. Forty
  occupied layers is precisely what a leak accumulates to, so the witness
  would have gone quiet as the defect got worse. It now runs at the head of
  each hang road, before a layer is asked for.
- **THE LAW WAS THE ONLY RELEASE THAT DID NOT RETIRE THE FRAME.** Four lenses
  found this independently. Freeing a layer while a live frame still samples
  it hands it back to the allocator under that frame, and the next promotion
  paints a new picture into a dead room. The head of a hang road has the
  device, so the relocated law retires every frame that samples a layer it
  frees.

**And the refuter surfaced a synergy the order did not plan.** U1 put `w=` on
the EVICT line as well as the pop line, which means **any road that releases a
stale layer testifies, not just the law's**. That matters because
`clear_wall_paintings` runs at the head of `place_wall_paintings` and would
silently clean a stale INDOOR layer — the law now runs before it, but even if
a future road slipped past, its release would print `w=` below the current
world. The law catches what reaches it; the stamp catches the rest.

**THE VISUAL GATE, CORRECTED.** Three of the order's five checks need
restating before Jean runs them:

| # | as ordered | as it should be read |
| --- | --- | --- |
| 1 | an evict burst to `hung=0` before the incoming world's first pop | **holds, and it always did** — but the burst is ABOVE the incoming world's pops in the log, and `[Mood] Applied` prints AFTER those pops, so do not use that line as the world divider. Use `w=`. |
| 2 | zero `LATE EVICT` lines | **holds — and now extend it:** also zero `Evict` lines whose `w=` is below the current world. That is the wider net U1 bought. Grep `"] Evict "` with the bracket, or `LATE EVICT` will match `Evict` as a substring and the count will be wrong. |
| 3 | pop lines carry `w=`; per-world counts read off the log | **holds, and this is the unit that mattered.** |
| 4 | `exh=39` never allocates while `hung < 35` | **DO NOT RUN — unsatisfiable by construction.** `hung` counts authored layers only; a snapshot claims a layer and is never named. `exh=39` at `hung=33` is 33 authored + 7 snapshot = 40 of 40. |
| 5 | supply unchanged: indices ascend, wrap, zero `Dead` | **holds.** |

One more, worth adding: **a `LATE EVICT` line is always followed by an
ordinary `Evict` line for the same layer** — the accusation and the release
are two lines about one event. The pop-minus-evict reconciliation still works
because only the second is an `] Evict `.

## REPEAT_0c / REPEAT_2 — SUPPLY HARDENING AND THE TIDE

Prologue built (U-S1, U-S2), U1 built, **U2/U3/U4 STOPPED at the order's own
G1 stop condition.** Phase 0's six gates ran read-only, each with an
adversarial refuter over it; what follows is what survived refutation.

> **RESOLVED 2026-09-02 by PLUMB_0 RULING-1 (Jean's), built as Phase C+D.**
> Option B was taken in its honest form: the camera readback — which already
> ran in every instrument column including the shipped `off` — is promoted out
> of `INSTRUMENTS.camera_witness` and its pose kept as a contracts-tier
> `CameraPose`. `dump_camera_orbit` keeps its retirement warrant as a printer.
> The tide reads that pose, so the facing half of R3's test is implementable
> and the distance half measures from the EYE rather than from `point_`. The
> analysis below is kept because it is what the ruling was made on — read it
> as the argument, not as live state.

### The stop condition, and why it fired

The order: *"G1 finds no CPU-visible camera direction **and** distance-only
gating would recycle galleries the viewer plausibly watches at range → land
U1 only, FLAG, report options."* Both clauses hold.

**No CPU-visible camera direction.** The pose is GPU-sovereign:
`update_camera_vp` in `world.wgsl` is the only author of `camera_state`, and
nothing on the CPU retains an azimuth. `PlayerState`'s own banner says it
outright — *"THE CAMERA HAS NO CPU MIRROR — it lives GPU-resident"*.
`CameraControls` is a sensitivity panel, not a pose.

**`PointState::heading` is not the gaze**, though it is the near miss worth
naming. It exists on the CPU and is already consumed trigonometrically
(`patch_system.hpp`'s look-ahead computes `gen_cx` from `cos(point_.heading)`).
But it is the possessed **body's** heading: authored only inside
`phase_witness_harvest`'s `if (host != PointHost::CAMERA)` arm, held-last and
stale in camera-host, and in orbit the eye's azimuth is free of it entirely.
A body walking north while the camera looks south is an ordinary frame.

**No radius satisfies both R1 and R3.** Galleries only exist inside
`Dim::EXIST_RADIUS` (350), so the whole usable band is 0–350. At
`R_legible` = 300 roughly 0.2 galleries qualify at any moment — the tide
would essentially never fire, and R1 ("the playlist advances for a statue")
goes unmet. Lower the radius until it fires and it is recycling pictures in
plain sight: at 150 wu a 10 wu piece subtends ~3.8°, plainly a picture, and
R3 ("a watched gallery never changes") goes. The distance-only fallback is
not a weaker tide; it is a tide that either does nothing or breaks its own
law.

The eye is not the point, either: a distance keyed on `point_` under-measures
by up to `CAMERA_MAX_DISTANCE` (100 wu) of zoom, and by an unbounded amount
under accumulated pan (`coupling_input_to_camera_pan` has no clamp).

### THE OPENING — the pose already crosses the CPU every frame

`GPUCameraState` is 48 bytes (`pos[3]`, `azimuth`, `elevation`, `distance`,
`pan_x`, `pan_y`, `aim_point[3]`). It is copied GPU→staging in
`phase_witness_capture` and mapped in `phase_witness_harvest`, both under
`if constexpr (INSTRUMENTS.camera_witness)` — and `camera_witness` is **true
in every instrument column including `off`**, which is what `the-board-web`
builds (`CMakeLists.txt` caches `T7_INSTRUMENTS "off"`; only
`the-board-web-meter` overrides). The mapped struct is handed to
`dump_camera_orbit`, printed, and dropped.

So the gaze is **one member assignment** from being CPU-visible, at exactly
the staleness class `point_.x/z` already carries by law (one frame). The
forward vector needs no per-mode fork — `forward = -(cos_el·sin_az, sin_el,
cos_el·cos_az)` is the same convention in `build_view_projection_matrix`,
`compose_camera_position_from_orbit` and the camera-host fly branch, and
every branch of `update_camera_vp` writes `camera.azimuth`.

**The objection, and it is Jean's to rule on.** `camera_witness`'s own banner
says *"RETIRE IT once the arrival row is settled — that is the whole of its
warrant."* Building the tide's gaze on it makes an instrument marked for
retirement load-bearing. The honest form is the reverse: promote the pose
into the spine as a first-class fact and let the witness read *that*. Which
is architecture, not execution, so it stopped here.

### The three options, priced

| option | cost | what it buys | what it costs |
| --- | --- | --- | --- |
| **A — the body's heading** | zero new machinery | a real facing test in FPV, where body and eye are coupled | wrong in orbit (azimuth is free of the body), stale in camera-host. Recycles watched galleries whenever the camera is not behind the pawn. |
| **B — promote the pose** | one member assignment onto spine-resident state, plus the ruling above | the exact test R3 asks for, in every camera mode, at one-frame staleness | makes a retirement-marked instrument load-bearing unless the pose is promoted out of it first |
| **C — distance only** | zero | nothing | starves at a safe radius, breaks R3 at a firing one. Measured above. |

### What the other five gates settled

- **G2 — the table.** `GalleryCenter` is CPU-only: zero mentions in
  `world.wgsl`, zero in `tools/binding_schema.py`, no bind-group entry, no
  upload verb. `dressed_at` is not a mirror change; U1 landed on that basis.
  The record is a bare site key — it does **not** know which exhibition
  layers or painting slots it owns; a recycler must scan `painting_slots`
  for the patch pair, exactly as `evict_paintings_for_patch` already does.
- **G2 — THE SENTINEL TRAP, and it is a landmine for whoever builds U3.**
  `GalleryCenter`'s default patch pair is `(INT32_MAX, INT32_MAX)` — the
  *same* sentinel `place_wall_paintings` stamps on indoor wall slots. Any
  age sweep that walks the table and calls `evict_paintings_for_patch` on a
  record without first testing `gc.active` will pass the sentinel pair and
  **destroy the entire indoor wall hang**. Test `active` first, always.
- **G3 — the re-dress road is OPEN, and this is the campaign's good news.**
  The three hang verbs (`select_gallery_for_patch`,
  `place_gallery_from_selection`, `commit_gallery`) read and write **no**
  patch-system state. Only the two funnels do: `dispatch_place_gallery`
  writes `ActivePatch::gallery_deferred`, `dispatch_commit_gallery` writes
  `ActivePatch::entity_refs` via `record_entity`. **G3's stop condition does
  not fire** — no extraction is needed at all, because
  `tick_gallery_deferred_hang` already calls the trio directly for an
  existing patch without touching `entityQueue_` / `placementResults_`.
- **G3 — the one seam has a designed door.** `record_entity` does not
  dedupe (cap `MAX_ENTITY_REFS` = 16, loud drop on overflow), so a gallery
  re-dressed repeatedly would overflow its host patch's refs. The door
  already exists: `ActivePatch::unrecord_entity`, whose banner names its
  intended caller shape exactly — *"owner-side death verbs outside any patch
  loop"*, which is the tide. A recycle must `unrecord_entity` before
  re-dressing.
- **G3 — evict-before-redress is forced, and it is visible.**
  `select_gallery_for_patch` refuses a gallery against itself three ways: a
  scan for any active `painting_slot` with the patch pair, a scan for any
  active `gallery_center` with it, and `check_position`'s separation. So
  there is no road that keeps the old pictures up while the new ones are
  prepared — a recycled site goes bare and refills. Whether that reads as
  breathing or as a glitch is a visual question, not a code one.
- **G3 — the gallery slot is not stable across a cycle.** `select` takes
  the first free `gallery_centers` index, so a recycled gallery generally
  changes slot. Any tide bookkeeping keyed on slot must survive renumbering.
- **G4 — R5 confirmed.** One flat 48-slot table, same trio, finite or open;
  no separate finite-mode boot fill. A table-walking recycler never asks the
  patch grid anything, so the pinned `centerX` is irrelevant to it. And the
  lock is real: in a finite outdoor world `stream_patches` pins
  `centerX = centerZ = 0`, every allocated patch satisfies
  `in_render_window`, `evict_gallery` never fires, and the exhibition holds
  its first fill for the life of the world.
- **G5 — wall seconds, `TimeState::seconds`.** Confirmed monotonic across a
  world transition. Reached from gallery code as `GalleryState::authored_now`,
  which REPEAT_0c added.

### Two corrections to things this session said out loud

- **`GalleryState::frame_counter` is NOT motion-gated.** I said earlier it
  "lives inside `update_photographer` behind the step guard" and so could not
  drive the tide. Wrong: the guard is `if (step > 5.0f) return;` — a
  **teleport** guard, not a "did the pawn move" test. A stationary pawn has
  `step == 0`, passes it, and increments. It is still the wrong dialect for
  the tide (it skips the init frame and every teleport frame, and it is
  `ROSTER.gallery`-gated), but not for the reason given.
- **REPEAT_0c did not land a motion-gated cooldown.**
  `tick_gallery_deferred_hang` is called from `stream_patches` at
  function-body depth — the `if (gridChanged)` block opens and closes well
  above it (braces verified by column scan). `authored_now` therefore
  refreshes every frame, stationary or not, and U-S2's cooldown ticks for a
  statue. This was checked because it would have been a bug shipped in the
  same campaign that names displacement as the enemy.

### Priced, not built

- **G6 — the interval cannot be enrolled in one line.** Not in
  `config_` / `lightingStage_` / `agentRoomStage_`: `organ_readers.py`
  classifies all three as GPU-side and **out of scope to the reader proof**,
  so a CPU-read dial parked there would pass vacuously — an enrollment
  stating a belief no instrument can test. The lawful home is a new
  contracts-tier bank behind a new `ORGAN_BLOCK_GALLERY = 12`: a new header,
  `ORGAN_BLOCK_COUNT` 12→13, a `block_base()` case, an include, HOMES/PAIRS
  rows in `organ_gap.py`, a READERS row in `organ_readers.py`, and the
  `docs/ORGAN.md` tables. Registry surgery, and its own campaign.
- **`gallery.hpp` reads no graduated bank today** — zero `_LIVE` hits, zero
  mentions in `organ_readers.py`. The tide's dial would be the gallery body's
  first runtime read of any ORGAN bank and its first READERS row ever.
- **The tide needs two dials, not one.** The predicate is "aged **and**
  unwatched"; a bank minted for `recycle_interval_s` would need `R_legible`
  immediately. `Dim::EXIST_RADIUS` lives in `realization/state.hpp` and is
  not a bank, so it is not enrollable in place.
- **`ORGAN_PARAM_RO` was never considered and sidesteps a blocker.** A
  witness carries no range, so "the range is Jean's" does not apply to a
  meter. A tide that silently recycles resident galleries is exactly what an
  operator wants a meter for.
- **`RibbonSpawnSurface` is the charter template**, not merely the scope
  precedent: a per-spawn bank for one body, seated in that body's existing
  contracts header, flushed destructively. A re-dress *is* the gallery's
  spawn event.

### Three defects found in passing, none fixed

- **`organ_ledger.py --check` has no teeth.** It writes its text and
  `return 0`; it never compares the emission against `audit/ORGAN.md`.
  `organ_readers.py` likewise returns 0 on both the NO-SUSPECTS branch and
  after printing a suspect list. CLAUDE.md's gate table lists the organ
  ledger as an assertion; today it is a report. (Sibling of the already-open
  `command_census.py --check` finding, which exits 0 carrying a stale digest.)
- ~~`apply_mood_arrival` is declared and never defined or called~~ — **CLOSED
  by PLUMB_0 C3.** Deleted, with all three citations corrected in place. The
  CPU no longer needs to author an azimuth; RULING-1 makes it read one.
- ~~A stale banner in `phase_witness_capture`~~ — **CLOSED by PLUMB_0 C2**,
  and it was exactly the sentence someone pricing RULING-1 would have
  trusted.

### REPEAT_0c — what U5's refuter found and I did NOT fix

Five lenses, read-only, over `b9f476f4` / `ec9aa085` / `cd29f952`. Three
findings were real and landed as `6261bb04` (the cooldown's missing caller,
the two manifest roads into silence, two false banners of my own). These are
the rest — recorded because they are true, not fixed because each is either
outside this campaign's reach or Jean's to price.

- ~~THE WALL CLOCK IS A FLOAT ACCUMULATOR AND IT STOPS~~ — **CLOSED by
  PLUMB_0 B1.** `TimeState::seconds` and BeatClock's accumulators are double.
  The freeze was measured, not estimated: 524288 s = **6.07 days** at 60 fps,
  frame 24,986,956; the double is still advancing after 400 simulated days.
  Every CPU field differenced against the clock widened with it. The GPU seam
  is rebased per world (`world_epoch`, `gpu_seconds()`), so the float a shader
  animates on stays young as well.

- ~~THE COOLDOWN IS DENOMINATED IN RENDERED TIME, THE TIMEOUT IN WALL TIME~~
  — **CLOSED by PLUMB_0 B3 and D4.** Two clocks, and the tree now says which
  is which: `AUTHORED_DEAD_COOLDOWN_S` keeps rendered time on purpose (a retry
  burnt while nobody is watching is a retry wasted; `console.hpp` clamps dt at
  0.1 s and a backgrounded tab does not advance), while the manifest ladder
  moved to `steady_clock`, because a ladder that must beat a boot deadline
  cannot speak a clock that starts at first light.

- **THE RETRY LADDER IS STILL HOSTED ON THE RENDER SPINE — half closed by
  PLUMB_0 D4(a), and the half that remains is named rather than glossed.** The
  order asked for the tick to MOVE off the spine. It did not move; its
  denomination changed. Measured against `steady_clock`, a rung armed at 0.3 s
  is already DUE when the first post-first-light tick runs (~4.4 s) and fires
  on it, ahead of `OVERTURE_READY_TIMEOUT_S`'s 5 s — which is the outcome the
  order wanted. STILL OPEN: a tab whose surface acquire keeps failing, or a
  device-lost session, freezes the tick entirely, because `render()` sits
  below the acquire gate. Moving the call needs a pre-first-light per-frame
  site with the gallery in scope, which today means widening the
  `RenderCartridge` virtual — out of this order's reach and not worth that on
  its own. Unblocked by anything else that wants a pre-light cartridge tick.

- ~~A SECOND GATE BLIND SPOT (mirror_census cites what it does not pin)~~ —
  **CLOSED by PLUMB_0 A2.** The pin set is now INPUTS plus every repo file the
  emission NAMES, computed from the emission in a two-pass fixed point; pins
  went 8 -> 15 and cited-but-unpinned is 0. The command census's twin is closed
  by A1/A3 (`report_stale` wired into `--check`, own source pinned, 10 -> 11).
  Both proved to LOSE on a perturbed tree and to pass restored.

- ~~organ_ledger.py --check and organ_readers.py cannot fail~~ — **CLOSED by
  PLUMB_0 A4.** `organ_ledger --check` compares its emission against
  `audit/ORGAN.md` byte for byte and names the first differing line;
  `organ_readers` exits 1 on any suspect and on an unmapped family. CLAUDE.md's
  gate table was wrong in three places and is rewritten: the organ ledger's row
  carried the READERS' claim, organ_readers had no row, and THE COMMAND CENSUS
  HAD NO ROW AT ALL — which is why A1's stale digest survived two refuters.

- **The dead-mask reads fail-OPEN but writes fail-CLOSED.**
  `authored_next_live` treats an index the mask cannot speak for as live
  (deliberate, and its comment prices it); `authored_fetch_release_slot`
  guards its write on the same bound, so a short mask would give an unbounded
  retry loop rather than the one wasted round trip the comment promises.
  Unreachable today — both are sized in one breath in
  `exhibition_manifest_onsuccess` — and recorded because "unreachable" is the
  word that gets campaigns written.
- **Two guards that cannot be false.** `authored_manifest_attempts >= 1u` in
  `exhibition_manifest_onerror` (the counter increments before the request
  leaves) and `gs != nullptr` in the same condition (`userData` is taken from
  a reference). Both are defence for the `n - 1u` array index rather than live
  flow, and the second is worse than dead: as the first conjunct, a null would
  fall through to the terminal absence line rather than to a diagnostic.
- **`Sleeping disk=` is louder than `Dead disk=` was.** The old line printed
  once per index for the session; the new one prints once per index per
  cooldown — measured at ~100 lines in ten minutes with 5 files genuinely
  absent, where the old code printed 5 and stopped. Correct as information,
  and a candidate for the same floor the exhaustion sentence now has if Jean
  finds it noisy.
- ~~GalleryCenter::dressed_at is write-only~~ — **CLOSED by PLUMB_0 D5.**
  `tick_gallery_tide` differences it against `TIDE_INTERVAL_S` once per
  candidate sweep. The other half of that entry STANDS and is not closed: no
  gate can see an unread C++ member — G-LAW 2 parses `world.wgsl` only, and
  nothing audits dead C++ fields. Unblocked by someone wanting that gate.


## PLUMB_0 — THE INSTRUMENTS STOP LYING, THE TIDE STARTS MOVING

Every unit closed a line already recorded here or in the ledgers; the closes
are marked at their lines above, per L32. What follows is only what this
campaign leaves open or learned.

### The drift register — where the tree disagreed with the order

- **A3 walked into a trap the order did not know about.** Adding the census's
  own source to `INPUTS` broke witnesses C-3 and C-6, because two passes read
  `INPUTS[-1]` meaning "console.hpp" and one read `INPUTS[1]` meaning
  "renderer.hpp". The append silently re-aimed all three at the new file, and
  the failure message talked about the TREE ("NO begin_frame reconfigure site
  found") rather than about the list. `RENDERER_HPP` and `CONSOLE_HPP` are now
  named; no positional index into `INPUTS` survives. A one-line unit found a
  latent bug by stepping on it.
- **A2 could not "pin the full read set" literally.** `mirror_census` walks all
  of `src/` — pinning every file it opens would be a hundred-row stanza that
  churns on any edit anywhere. What closes the actual defect is narrower and
  self-maintaining: pin every file the emission NAMES. The order's own second
  clause ("or stop citing what is not pinned") is what this satisfies.
- **B1 could not widen `AnalysisSignal::t_seconds`.** Its layout is a
  documented 4128-byte contract with `t_seconds` at offset 0, size 4, and it
  crosses a boundary. BeatClock accumulates in double and narrows into the
  contract instead, so the field is a snapshot of an exact number rather than
  a drifting accumulation. `TimeState::seconds` is accumulated separately at
  U1 from the same `dt`; the two agree by construction and neither reads the
  other.
- **B1 moved the accumulation from U3 to U1**, which the order did not
  specify. U1 writes the GPU's clock, so accumulating at U3 would have handed
  the shader the PREVIOUS frame's value. The O-5a static_assert already pins
  U1 before U3.
- **C1 had to promote the BUFFER too.** `cameraReadbackStaging_` was also
  `if constexpr`-gated ("created only when the witness is armed"). Promoting
  the copy alone would have been a `CopyBufferToBuffer` into a null buffer the
  day the witness took the retirement its own banner promises — a crash
  planted by the unit that exists to survive that retirement.
- **C1 projected the pose into contracts rather than carrying `GPUCameraState`
  into `MachineCtx`.** `GPUCameraState` is a realization type; a body reading
  it would invert the tier order. `CameraPose` (eye, azimuth, elevation, valid)
  is the projection; the raw 48 bytes still die with the mapping.
- **D4(a) did not do what it was asked.** The order said move the tick off the
  render spine. It did not move — see the standing entry above for what
  changed instead, what that buys, and what is still open.

### What this campaign leaves open

- **ORGAN_2c** — `ORGAN_BLOCK_GALLERY`, the contracts-tier bank, the
  `RibbonSpawnSurface` charter, the first gallery READERS row, the
  `ORGAN_PARAM_RO` meter. `TIDE_INTERVAL_S`, `TIDE_R_LEGIBLE` and
  `TIDE_CONE_COS` land as named constants per RULING-4; the enrollment debt is
  recorded, not paid.
- **REPEAT_1** — cells ruled (RULING-2), padding counted, ramp idiom named.
  Awaiting only Jean's call.
- **The tide is unproven in the world.** Everything here is gates and models;
  no build, no visual. The statue test, the turn, and the finite world are
  Jean's, and until they run the tide is a machine that compiles.

### PLUMB_0's closing refuter — what it found that is NOT fixed

Fourteen findings were real and landed as `7e616a42`. These are the rest,
recorded because they are true and because each is a ruling or a campaign
rather than an edit.

- **THE PIN-ONLY TOOLS STILL CANNOT SEE A HAND EDIT.** A4 gave
  `organ_ledger --check` a byte-for-byte comparison against its artifact. A1
  and A2 gave their tools the GATES_2b shape instead: `--check` re-derives
  nothing about the artifact's BODY, it reads `_PIN_ROW` out of the stanza and
  hashes the inputs. So a hand edit to a row, a count, or a witness verdict in
  `COMMAND_LEDGER.md` or `MIRROR_LEDGER.md` passes green — L28's "never
  hand-edited" is enforced for `audit/ORGAN.md` alone. The cure is the one A4
  already demonstrates: compare the emission. It was not carried across here
  because both censuses embed a git-derived provenance stamp, and an emission
  comparison would then red the gate for the documented stamp lag rather than
  for a defect. Unblocked by splitting the stamp out of the compared body.
- **`report_stale` READS ITS SCOPE FROM THE ARTIFACT**, so widening a tool's
  `INPUTS` is inert until someone regenerates. That is GATES_2b's deliberate
  design ("the stanza is the list") and it is why the cascade is a step rather
  than a nicety — but it means a tool and its artifact can disagree about what
  is even being checked for exactly one commit.
- **`mirror_census` READS FILES IT NEVER NAMES.** A2 pins what the emission
  NAMES. The census also reads every `.py` under `tools/` and every file under
  `src/` to count `bind::` mentions; a file that is read, contributes to a
  count, and is never cited stays unpinned, so a change to its mention count
  moves the artifact with nothing to say the artifact was owed. Narrower than
  the blind spot A2 closed, and the same shape. Unblocked by a read-recording
  wrapper around the census's own file access.
- **A RED `organ_readers` CAN BE COMMITTED INTO `ORGAN.md`.** The organ
  ledger's emission embeds `tail_of("organ_readers.py", 16)` and `tail_of`
  ignores the return code — which is what makes A4's exit change safe. But it
  also means a run with suspects can be baked into the artifact, after which
  `organ_ledger --check` compares that text and passes. The two gates must be
  read together; only the readers row is the verdict.
- **D4(a) is half done** — see the standing entry above.

### One refuter finding REFUTED

The fan-radius report priced a gallery's reach at ~150 wu from
`PAINTINGS_MAX_BY_ARCHETYPE` `{8,10,12,12}`. Those ceilings are structurally
unreachable: `count_raw = PAINTINGS_MEAN(3) + (h1+h2+h3-1.5)*PAINTINGS_SIGMA(1)`
with each `h` in [0,1] spans [1.5, 4.5] and rounds to at most 4, and the clamp
never binds. The true maximum fan is
`sqrt(41² + 12²) + PAINTING_HALF(13) + FAN_MARGIN(3) = 58.72` wu, which is
what `TIDE_SITE_REACH` is derived from. The finding's SUBSTANCE stood — the
predicate did measure to the centre — and is fixed; only its number was wrong.

## PULSE_1 — THE RING HAS A WRITER AGAIN

`contrib_radial_pulses_at` was whole and DRIVERLESS: every consumer reached it,
and it returned `0.0` at every point in the world because its only writer was
the boot zero-pin. PULSE_1 gave it a PLAYER writer — a lone clean tap on either
half of the glass, and SPACE on the keyboard — and changed nothing in WGSL.

### Pulse bus — one writer

THE RING IS A BUS: `contrib_radial_pulses_at` does not ask who stamped a slot.
A thumb-tap and a note-on are the same event to it — a position, a time, an
amplitude. LIGATURE_1 adds the soundtrack onset writer by calling
`emit_radial_pulse`. It needs no second home, no struct change and no shader
change — only a source of onsets and a point position, both of which the
cartridge already holds. There is deliberately no debounce on the bus: a
musician firing two notes 40 ms apart is entitled to both, and the rate-limit
lives at the finger, which is the input that bounces.

THE ONSET DETECTOR WAS ALREADY GONE when this campaign opened — no
`pulseRing_`, no `pulseWriteIdx_`, no `prevPolyphony_`, no `signal.stats[0]`
block. The order that authorised PULSE_1 expected to delete it and instead
found the deletion done, which is why this entry records a debt rather than a
deletion. The ring PULSE_1 built is new, and it is the first one the player
can write.

### Two things arming the ring made true that were false before

- **THE REST LAW NOW HAS A MOVING CONJUNCT.** `pulse_count > 0` is the first
  test in `live_card_is_live`, and its own comment said the conjunct was
  "structurally pinned at rest (O0-d: the ring's only writer is the boot
  zero-pin)" and was "checked anyway so a future re-arming of either conjunct
  wakes the writer without an edit here". That future is here: the check was
  right and no edit was needed, but the ring must now RETIRE or OPT_1a's rest
  skip — 819,200 invocations a frame — is lost for the session on the first
  tap. `retire_aged_pulses` runs at the head of `phase_live_card_write` using
  the shader's own liveness test.
- **THE FOURTH REBASE SEAM IS LIVE.** `TimeState`'s `world_epoch` comment named
  the ring as a fourth GPU-resident time crossing a world rebase and dismissed
  it — "but `set_pulse_data` is only ever called with zeros; it is inert". It
  is not inert now. The ring is cleared in `become_destination`, the one door
  every world enters by and the line the epoch moves on; the comment is
  corrected where it stands.

### What PULSE_1 did not take

- **The glass's dials cannot be ORGAN rows.** `TAP_MS`, `TAP_SLOP` and
  `PULSE_DEBOUNCE_MS` live in `src/console/console.hpp`, which contains zero
  occurrences of `the_board` — the console is cartridge-blind by construction
  and ORGAN is the cartridge's panel. No `TouchControls` dial is enrolled, so
  the pulse's three are in the same position as every other glass dial and this
  campaign opened no gap. Unblocked by giving the console a panel of its own,
  which is a campaign and not a line.
- **The pulse's SHAPE is unenrolled.** `PULSE_SPEED`, `PULSE_RING_SHARPNESS`,
  `PULSE_DAMPING`, `PULSE_AGE_DECAY` are `world.wgsl` consts with no ORGAN row.
  They are ORGAN_2b's business; PULSE_1 gave the contributor a writer, not a
  face. Only `PANEL.pulse.amplitude` — the thing this campaign introduced — is
  enrolled.
- **The reference sheet has no pulse glyph.** The pulse is the first MOMENTARY
  verb on the glass; aura and possess are toggles, which is why both were drawn
  with double-headed arcs. A strike is not a switch and needs its own mark.
  That is a design sitting with Jean, not a CC round — and it lands in the same
  place as "THE DOOR TEACHES; THE WORLD STILL DOES NOT", above, which this
  campaign adds a third unteachable verb to.
- **Amplitude is flat.** Speed-scaling the impulse to the point's velocity is
  the attractive next move and is refused until a measurement asks for it.
  Jean's word opens it; the dial is already on the panel to look through.

### Owed — Jean, and the campaign is not closed without it

THE VISUAL WITNESS IS THE WHOLE POINT and no gate can stand in for it: tap
once, and the ground should ring outward from the point, the pawn ride its own
wavefront, and the camera clamp lift with it. One tap, whole frame answers.
Everything landed here is gates and a compile — `glaw1` GREEN at every phase —
and the piece has not been seen doing it. `-DT7_INSTRUMENTS=full` prints
`[Pulse] emit x= z= amp= slot= count= t=` once per tap; five taps in rhythm
must show the slots advancing and wrapping at 8, and `t` rising by the gaps
played.

## RETRACT_2 — FOUR ITEMS HALTED, THEIR SUBJECTS UNREACHABLE

RETRACT_2 ordered five commits. Three landed (the cube orbit resolve, L50 +
the FLOOR banner's scope note, the pulse tune). Four items halted under P17 —
unreachable subject, which stops the ITEM and never the round — and they are
recorded here because the order's own close (its 4.3) asked for a RETRACT
entry in this file that has never existed.

THE ORDER WAS WRITTEN AGAINST A TREE THIS REPOSITORY HAS NEVER BEEN. Two of
its named symbols return zero hits from `git log --all -S`: they were never
added and never removed anywhere in history. The cited RETRACT_1 sha
`fc032c9b` is not a valid object here either. Whatever tree RETRACT_1 read,
it was not this one, and that is the finding — not a drift, an absence.

- **The automaton fetch divisors (commit 1).** `AUTO_GRID_MAX` does not
  exist; neither does the two-site divisor pair it was to replace, nor any
  "texel 143 where it wants 79" addressing. The nearest real thing is
  `world_box_clamp_xz`'s documented half-open `floor(bmax / PATCH_EXTENT)`
  hazard, which is guarded by every caller passing a positive margin and is
  not this. Unblocked by Jean naming the real site, or by withdrawing the
  item.
- **The carve fade stamp (commit 2.2).** `gol_carve_fade` does not exist.
  This tree has exactly TWO suppression centres — `pawn_gol_suppression` and
  `witness_gol_suppression` (the eye, KITE_1) — and NO cube-driven carve at
  all. There is no fade reading `fe.orbit_height + fe.bob_amplitude` to stamp.
- **`CUBE_REACH_CEILING` redefined as `2.0 * ZONE_SUPPRESS_OUTER`
  (commit 2.1).** The anchor MATCHES and the value is identical (30.0 both
  ways), so this one halted on judgement rather than on reach, and it is
  Jean's call. In the order's tree the redefinition tied the ceiling to a
  cube carve fade's zero point. Here `CUBE_REACH_CEILING` has ONE reader —
  `row_cube_push`'s `reach_ok`, the shove-eligibility gate — and
  `ZONE_SUPPRESS_OUTER` is the GoL suppression radius the EYE fades over.
  Binding them by definition would mean a future tune of the eye's carve
  reach silently re-classifies which cubes the point can shove, against the
  constant's own comment ("Jean-tunable"). The coincidence at 30 is a
  coincidence. Unblocked by Jean ruling the two reaches one idea.
- **Closing RETRACT in `docs/OPEN.md` (commit 4.3).** There was no RETRACT
  entry to close — `grep -in retract docs/OPEN.md` was empty before this one.
  The deferred question the close names (whether cubes should descend into
  their carve) is answered by the FLOOR banner's new scope note regardless.

### What the landed half found that the order did not predict

- **The Monolith gets its bob back.** Before the resolve the clamp ate it
  whole — `min_drift_y = 6 - bob_y` forced `drift.y` to track `bob_y` exactly
  and the cube sat dead still at ground+18. After, `min_drift_y = -bob_y`
  clamps only the lower half, so it bobs in [ground+18, ground+19.2]. The
  order predicted "positions do not change"; they change by a 1.2 wu
  half-wave that the tier table authored and the clamp had been eating.
  Jean's to accept or refuse — refusing it means clamping `home.y` rather
  than `drift.y`, which is a different edit.
- **All four cube tiers can resolve, not Monolith alone.** At the tier MEANS
  only Monolith fires (12 against a floor of 18). But ORBIT_HEIGHT is a
  Gaussian floored at 3.0 by `CUBE_PARAM_DEFS`, so a low draw puts any tier
  under its own body floor: SmallCube below ~4.8, MedCube below ~7.0,
  LargeCube below ~11.0. No position changes in any of them — the clamp was
  already holding them there — so what fires is the honesty of the number.
- **The retract value is outside the predicted bracket, and the prediction
  was unreachable anyway.** The order predicted fade(19.2) in 0.40–0.75. The
  only fade of that shape in the tree is the EYE's height fade,
  `1 - smoothstep(ZONE_SUPPRESS_OUTER, 2*ZONE_SUPPRESS_OUTER, h)`, which
  gives 0.8087 at 19.2 — and it never reads a cube's altitude. Reported, not
  tuned.

## STATURE_0 — the aim fraction is a constant, not a dial

PHOTO_AIM_FRACTION (world.wgsl, 2/3) and the implicit eye lift of one
subject height are the photographer's framing choices, and unlike the
capture cadence they ARE seen — they are the painting. They sit as a WGSL
const and a bare expression because no measurement has asked for a dial
yet. What unblocks the graduation is ORGAN sovereignty: a panel subject
lives in config_, lightingStage_ or agentRoomStage_, so promoting them
means two more floats on GPUDesignConfig and two rows in
organ_params.inc — worth doing when Jean wants to taste the framing live,
not before. · STATURE_0 · a visual complaint about where shots land.

## ORRERY_0 — THE SKY GAINS A CONDUCTOR (A+B landed; ORRERY_1/A+B, ORRERY_2 and ORRERY_3 landed; visual gate held by Jean)

Six authored states over four rules change the orb sky on the beat grid —
16 beats each, flocking 32 — drawn by xorshift on the world seed. FROZEN
enters 1-in-8, only off brownian/orbital, and is always answered by
orbital-intense. Noise and speed_mult pin to their dial ceilings while the
conductor reigns (the claimant orb_surface.hpp's speed_mult comment
foretold). Kill switch: ORB_CONDUCTOR_ON. Closes on Jean's eye across the
three orb moods.

TWO CORRECTIONS OF RECORD, both found in recon and both against the order
that authored this section. · ORRERY_0 R2 ·

- **The pass-through value is 1.0, not 0.0.** The order had the conductor
  write 0.0 to all four `rule_drag_*` calling that "the sentinel". The zero
  sentinel is a CPU convention only: `configure_orbs`' `passthrough()` maps
  an authored 0 to 1.0 BEFORE upload, and world.wgsl's own field comment
  says "1.0 = pass-through, sanitized on CPU". The kernel applies the
  multiplier raw — `exp(-orb.drag * rule_drag_X * dt)`, no sentinel branch —
  so a 0.0 written through the conductor's targeted seam, which bypasses
  `configure_orbs`, means NO DRAG AT ALL in every rule. The conductor writes
  `ORB_CONDUCTOR_RULE_DRAG_NEUTRAL` (1.0) instead, which is what the order's
  own stated intent — "a state looks the same in every sky" — requires.
- **The boot state is NOT a no-op, and the absolute drag column is inert.**
  The order claimed "boot state is flocking, today's sky, so nothing changes
  until the first fire". Two ways that is false. (a) The arm frame applies
  the boot state immediately, and pinning writes noise 0.3 -> 3.0 (ten times
  the rest floor) and speed_mult 3.33 -> 4.0 the moment the conductor arms.
  (b) `finite_outdoor` is `motion_rule 0u` (BROWNIAN) today, not flocking, so
  arming switches its rule at once. Only sunset and night boot flocking.
- **RESOLVED at ORRERY_1/A — the drag rides `rule_drag_*`, and the dead seam
  is deleted.** The finding stood: `orb.drag` is baked per orb by `orb_init`
  and read from the state buffer by `orb_dynamics`, `GPUOrbConfig::drag` is
  read by `orb_init` ALONE, and speaking it on a reseed frame would have
  baked the conductor's drag into every orb for the life of that world.
  Jean took the first of the three priced routes: the table's `drag` column
  is now the BROWNIAN MULTIPLIER on the baked per-orb drag (0.4 at all three
  open moods), written to the live `rule_drag_brownian` slot — 1.5 gives 0.6
  effective, the legible medium; 0.0 lands raw through the seam, which
  bypasses `configure_orbs`' sanitizer, and is the undamped intense. The two
  brownian states now differ on screen. `upload_orb_drag` is deleted (YAGNI),
  and the other three multiplier slots stay at 1.0 — written, unread, neutral.
- **RESOLVED at ORRERY_2 — the desync dissolves into rows Jean turns himself.**
  The two ORBS console dials (`speed_mult`, `noise_floor`) no longer argue with
  a pinned constant: the conductor carries its OWN `noise` and `speed_mult` per
  state, so the console rows author the REST and the conductor authors the
  REIGN, and each is a dial. Nothing is silently overwritten by a constant any
  more — what the GPU holds while conducting is a row Jean can see and turn.
- **ORRERY_2 — every conductor number is a dial, in block CONDUCTOR.** 44 rows:
  `enabled` and `frost_one_in` on the console, then seven per state (gesture,
  brownian drag x, orbital speed, noise, speed mult, duration, jitter) across
  the six. Seeds are Jean's spec with drag zeroed on his later word ("keeping
  the drag zero"), noise and speed_mult at the old ceilings per state, orbital
  0.7 / 1.0, gesture 0, jitter 0. NOT dials, deliberately: each state's RULE,
  and the ceremony that orders them — identity, not knobs. THE ROW-WATCH means
  an edit to the reigning state's row lands on the sky within one frame,
  mid-reign, with no transition to wait for. Jean tunes from `?organ=1`, and
  export/import carries his numbers by the stable `CONDUCTOR.states[i].field`
  ids.
- **ORRERY_3 — THE FROST FORKS, and that is the fix for a too-frequent flock.**
  The ceremony had one divergence from Jean's spec: frozen ALWAYS opened the
  flock, so the flock was earned by the frost alone. His law is that frozen
  forks — "after frozen we may have either brownians or flocking" — so the
  frozen branch now draws 1-in-`flock_one_in` (a new console dial, seeded 3)
  for the flock and otherwise releases back to the pool on a coin. Rarity
  COMPOUNDS along the chain: the flock sits behind two gates and the wheel
  behind three, because the wheel can only follow the flock.
  MEASURED on the landed graph with Jean's `ca07c0f` durations, frost 8 /
  flock 3, over 1.2M transitions: brownian-or-frozen holds **89.6%** of the
  time (his "most of the time"), and the mean gap between flock entries is
  **278 s — 4.6 minutes** at the 100 BPM rest tempo, not the ~4 the order
  estimated. Occupancy by time: brownian 79.3%, frozen 10.3%, flocking 6.9%,
  orbital 3.5%. An eye that finds the wheel too rare turns `flock 1-in-N`
  down; one that finds the pool too still turns `frost 1-in-N` down. Both are
  dials now.
- **ORRERY_2 needed one thing its order did not name: a READERS row.**
  `tools/organ_readers.py` reds on an UNMAPPED FAMILY, and a new block is one
  until its family is listed there. CONDUCTOR is now listed with both handle
  depths (`OrbConductorConsole`, `OrbConductorState`), the same shape MOOD
  already uses. Related and load-bearing: that tool's ALIAS regex matches
  `auto&` bindings only, so the conductor binds its reigning row with
  `const auto&` — a `const OrbConductorState&` would have left all seven
  per-state leaves unprovable and the gate red.
- **Phase B's comment fix retired TWO stale claims, not one.** The order named
  the `bri 0.95` against the row's `0.605f`. The same sentence also called the
  night "brighter" than the sunset's field when the sunset row holds `0.85f` —
  the night is DIMMER, not brighter. The replacement drops both words, so the
  sentence now claims only what the row gives: fuller and slower.

## USHER_0/P — parked residue

- finite_outdoor is DARK, not cut: `mood_weights[3] = 0` (mood_constants.hpp);
  every other surface of the mood stands, `?mood=3` and the panel dial reach it
  · origin: USHER_0 U1 · unblocks: a finite-outdoor design worth a door (reopen
  = one table cell — portals only; the boot roster is open-sky by shape, U3),
  or a ruling to cut the mood whole — its own campaign (MOOD_COUNT shrinks,
  ids renumber, every per-mood table loses a row).

## LODESTAR_0/P — parked residue

- LODESTAR_CELL stands at 8 (400 wu cells, ~400 wu nearest-door ceiling).
  Turning it to 6 (300 wu ceiling, ~+100% doorways) raises standing arch
  demand toward the 16-slot pool (MAX_ARCH_INSTANCES — a GPU-buffer
  dimension: AMG vertex/index pools, ground buffer, mesh-params buffer, a
  renderer dispatch) · origin: LODESTAR_0 U1 · unblocks: a session showing
  pool headroom at 8 (the U2 witness is the instrument), or a
  MAX_ARCH_INSTANCES resize campaign that re-prices those buffers.
- a designated door can lose all LODESTAR_TRIES candidates deep in a shadow
  field (~few % of monumental/antenna cells); the neighbor cell then holds
  the ceiling at ~800 wu · origin: LODESTAR_0 U4 · unblocks: Jean's walk
  gate — levers are LODESTAR_TRIES or the retry jitter, both one-line.

## WEBSITE_1 — the site moves into dist/; the engine owns names, not the folder

Engine-side half of the website merge; the site-side half is Jean's
(MAIN/MERGE.md). web_dist.py deletes only the names it writes and treats
everything else in dist/ as a tenant; the root _headers keeps one writer,
emits /fonts/* and /about/ rules only when those folders exist, and folds
dist/collection/_headers.fragment verbatim; web/_redirects ships /main →
/about/ and /world → / as 302 aliases; the shell gains two static doors
(collection · about), a noscript paragraph, meta/OG tags, and a fallback
card that says what the board is instead of promising a recording it never
had. Full-refresh order: site pipelines first, `python tools/web_dist.py`
LAST — it is the root assembler, and the fold reads what the site wrote.
Open: · the card's browser list dropped "Firefox on desktop" — the shell
claimed it, the Aug 2026 record holds Firefox LOST/HELD; Jean rules which
fact is stale · card / noscript / meta wording is drafted, Jean gates.
WEBSITE_2 seated the zip's 22 files at their repo paths, gitignored the
masters, and corrected MERGE/SETUP to the web_dist-LAST order. The
fragment item closed on inspection: collection_dist writes exactly
/collection/* immutable plus /collection/ and /collection/index.html
no-cache — zero overlap with the root writer's conditionals, so the
verbatim fold stands as landed. Still Jean's: masters in, placeholder
copy (5 markers, 3 links), pipelines + deploy, and the functions/
pickup check at deploy time.

## GOL_GRID_0/P — parked residue

- a 64-cell zone shadows arch placement over ~150 wu of radius (Arch–GoL
  separation 0, footprint radii add): a LODESTAR cell whose core falls
  under one loses its designated door to the neighbor cell (~800 wu
  ceiling) · origin: GOL_GRID_0 · unblocks: Jean's walk gate — levers are
  the promoted rows' cells column, or a zone-aware lodestar designation
  (its own campaign).

## BUILDID_1 — the ?v= key hashes both halves (closes the 5 Sep pin)

world.wgsl is packed data; a shader-only edit changed .data under an
unchanged v=sha256(wasm), `immutable` pinned the old shader in the
visiting browser, and SEAL2 correctly floored a coherent server
(expected ca1de349 / received 1b2295bc / v=e59631131758 on both ends —
the served data hashes to ca1de349, verified from the wire).
build_id is now sha256(wasm + world.wgsl)[:BUILD_ID_LEN]; the first
deploy after this re-keys every URL and heals any browser pinned today.
Interim cure on an already-pinned machine: hard refresh (Ctrl+Shift+R).

## PAIR_0 — the glue and the package are witnessed as a pair (closed)

The 5 Sep floor outlived BUILDID_1 because it was never the cache. From
the wire: glue `remote_package_size:745343`, package 745414 bytes, wasm
`e59631131758` stamped `native-sunset-722-g0b18b2dd 2026-09-05T05:06Z`
— the morning's build beside that afternoon's package. The glue slices
to the length it was linked against, so every device received the
current shader minus its last 71 bytes (`// END OF SCROLL`), digesting
to 1b2295bc against an expected ca1de349. `web/` had accumulated
outputs from two links; a link writes the .data before it emits the
.js/.wasm, so a failure between the two leaves exactly this mixture.
web_dist now compares the glue's declared package length against the
package it ships and refuses on mismatch; absence of the field prints
UNWITNESSED rather than refusing. Deploy hygiene of record: delete the
three build files before building, so a failed link is an ABSENT
artifact (web_dist's existing BUILD FIRST refusal) rather than a stale
one.
