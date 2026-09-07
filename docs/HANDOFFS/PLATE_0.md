# PLATE_0 — THE WALL AT 1024

*Handoff · authored 2026-09-07 · Claude → CC · Jean holds build, visual, merge, deploy.*
*Filed to `docs/HANDOFFS/PLATE_0.md` while open; the directory dies with the campaign.*

## THE IDEA, IN ONE PARAGRAPH

Every painting array doubles its edge. `Dim::PAINTING_RESOLUTION` is the one number that
sizes the exhibition, both staging rings, the three offscreen snapshot targets, the CPU
fit box, the padded upload, and the postcard's readback — PORT_5b proved that when it cut
the number to 512 for an old laptop that could not make 681 MiB resident. That laptop has
left the audience; the masters the 512 wall was scaled from are stubs being replaced at
1024; and POSTCARD_0 made the wall the program's public face, because the postcard ships
it. So the number reverses on Jean's stamp, the dist cap and quality follow, comment truth
follows the number everywhere it was written down, and the residency witness moves to
where it belongs: the floor phones. One more thing rides along, from the same review: the
take badge's doorstep pulls in, so the words appear when a visitor is genuinely before a
picture, not across the room from it.

## AUTHORITY

**Base: POSTCARD_0's tip on Jean's machine.** As of 2026-09-07, `origin/master` is still
`e35b8d5d` (HOME_0) and no `claude/postcard-0` exists on origin — POSTCARD_0 lives only
locally, applied by CC and witnessed by Jean. PLATE_0 therefore branches from wherever
that tip lives locally (the held branch if unmerged, master if merged), and U0 proves the
base by CONTENT, not by hash: the blobs below are the tip the POSTCARD_0 handoff produces
when applied verbatim to `e35b8d5d`; a differing blob with every FIND matching at count is
DEFAULT-AND-FLAG (Jean's naming amendments live in files this campaign never touches).

Rides a **held branch `claude/plate-0`** (P12: an estate change, a dist change and a badge
that appears at a different distance are runtime behaviour the visual and residency gates
catch). Jean authorises the merge; the branch dies on merge (ATTIC LAW).

| file | blob at base (expected) |
| --- | --- |
| `src/cartridges/the_board/realization/state.hpp` | `6e67902b85c1a07071de6565b81e32c8b9af0210` |
| `src/cartridges/the_board/cartridge.hpp` | `43c4205e5710444d035b3748619f2d590f204d3b` |
| `src/cartridges/the_board/bodies/gallery.hpp` | `3df8952426356f56c1b4a14251be8d6d2d60ccb0` |
| `tools/web_dist.py` | `f0bf9191a8bb4e8d867a9241c90d625d707e3125` (untouched by POSTCARD_0 — equals base master's) |
| `docs/OPEN.md` | no pin — CC wrote POSTCARD_0's entry with round-found adjustments; U4's anchors are phrases, count-checked |
| `audit/*.md` | no pin — regenerated ledgers carry CC's commit hashes; U4 regenerates again |

**Pinned files.** `state.hpp` (BINDING + MIRROR), `gallery.hpp` (BINDING + COMMAND),
`cartridge.hpp` (COMMAND). The ledger gates are red from U1 until U4 regenerates them.
Expected; say so. `binding_gen.py --check`'s S-6 stays red until the branch is pushed.

**HALT classes (P17), scoped to the unit (P15):** unreachable file; stale authority (blob
differs AND a FIND misses). Everything else DEFAULT-AND-FLAG. Symbols, not ranges (P2).
Expected match count **1** everywhere. LF only, no BOM.

**REHEARSED, NOT RECITED.** Every block below was applied in this order to a clone of the
exact base blobs above. All eight FINDs matched at count 1; G-LAW 1 GREEN and the TU gate
at zero diagnostics after U1 and after U3; `web_dist.py` parses after U2; after U4 the
full cascade and every gate in CLAUDE.md's table passed (naga and the collection gate
excepted — the sandbox has none; `world.wgsl` is untouched, PORT_5b's own audit confirmed
`PAINTING_RESOLUTION` has no WGSL twin, and G-LAW 2 is GREEN). `binding_gen.py --write`
was byte-identical: the schema's size expressions are `Dim::`-relative, no floor in NEEDS
moves (1024 sits under the 2048 `maxTextureDimension2D` floor the shadow atlas already
sets), and MANIFEST carries no byte column. The OPEN.md amendments were count-verified
against the entry as the POSTCARD_0 handoff writes it.

## THE RULINGS THIS CAMPAIGN LANDS

1. **The constant reverses; the history stays.** PORT_5b's paragraph remains above the
   constant (P4 — the tree keeps its reasons), and PLATE_0's paragraph joins it: what
   changed in the world (the laptop gone, the masters replaced, the postcard shipping the
   wall), the five textures' new prices, and where the levers live if a floor phone
   refuses. One fact, one home: the levers are named in OPEN.md, not restated in code.
2. **Comment truth travels with the number.** Every place the old resolution was written
   down as prose — the assert's message, the staging banner's `2048 bytes` and `the
   megabyte`, the copy banner's `2048 B`, the streaming-cost comment's `512-px` /
   `1 MiB` — becomes resolution-relative, so the NEXT move of this constant is one edit
   again.
3. **The dist follows, and quality rises for the postcard's sake.** `PAINTING_CAP` 1024
   (the banner above it already orders this), `PAINTING_QUALITY` 82 → 88: at 1024 the
   wall shows the file near 1:1 and the postcard re-encodes it at JPEG 0.92, so q82's
   blocking would survive two generations. Masters never upscale — the loader's clamp is
   untouched — so an under-1024 master simply occupies less of its layer.
4. **Nothing else moves.** Layer counts, the promotion's full-layer law, the shaders (no
   WGSL twin), the postcard machine (resolution-relative by construction — `deliver`
   takes `res`, the readback sizes derive), the schema (Dim-relative expressions), the
   shell. The static_asserts hold at 1024: bytesPerRow 4096 = 16 × 256; both layer
   floors (≥ 28) unmoved.
5. **The badge's doorstep pulls in** (Jean's eye: shown from afar, the badge is noise):
   reach `max(6 wu, 1.6 ×)` from `max(8, 2.2 ×)`, lateral slack 0.25 from 0.5. The
   invariant is proved in the banner: the pilot's stand-off `max(4, 1.4 ×)` is inside the
   new reach at EVERY span with ≥ 0.75 wu to spare, so a VISIT_0 walk always ends with
   the badge lit. (An earlier sketch said 1.4 ×/5; that would land the pilot exactly ON
   the boundary, one float jitter from a dark badge — the margin is the correction.)
6. **The measurements that gate the next mechanisms are named, not built** (L: measuring
   before building): the `authored6` mark gates the browser-decode road; the
   `Photographer Snapshot` METER row on the Pixel gates the quadrant-capture arm; the
   floor phones' residency gates the diet levers. Each is registered in OPEN.md with its
   priced answer.

## THE MECHANISM AUDIT (L16) — what the tree says, and where

- **One number, five textures + two CPU vectors + one readback.** All schema size
  expressions for the painting family read `Dim::PAINTING_RESOLUTION`; the loader's `RES`,
  fit box, `dst_w/dst_h`, padded vector, `uv_scale` derive from it (PORT_5b's own audit);
  the solid-colour fill and the postcard staging (`postcard_bytes_per_row/size`) derive
  too. `grep -rn "\b512\b" src --include=*.hpp` near the family finds only the constant
  and PROSE — the four comment sites U1 corrects — plus one historical sentence about
  SUPPLY's partial write, which stays (it records a past event).
- **No WGSL twin.** `PAINTING_RESOLUTION` appears nowhere in `world.wgsl`; the shaders
  sample by uv through the array views. PORT_5b checked this and G-LAW 2 re-proves the
  symbol tables each run.
- **The estate at 1024, computed from the schema:** Exhibition 40 → 160 MiB, Authored
  Staging 32 → 128, Snapshot Staging 32 → 128, Offscreen Color 1 → 4, and under MSAA 4
  the Offscreen Depth and MSAA Color 4 → 16 each. **Delta +339.0 MiB (+318.0 where
  `effective_msaa()` is 1** — the MSAA colour target is only created at 4, and the depth
  rides the sample count).
- **The floors hold.** NEEDS' `maxTextureDimension2D` floor is 2048 (the shadow atlas);
  1024 sits under it, so `limits_floor.gen.inc` and the device request are unmoved —
  `--write` proved byte-identical.
- **The boot decode is main-thread stb** (`authored_stage_decoded_image`; the accumulator
  and the `authored6` mark are AUBADE U1's). A 1024² decode+scale is ~4 × the 512 cost;
  the mark is the witness, the READY floor the threshold.
- **The capture is a measured ~10 ms pass at 512** (the gallery's own banner), paced once
  per ceiling; 1024 ≈ 4 ×. The METER row exists; the priced arm (512 into the 1024 layer)
  must ride LoadOp::Clear on the FULL layer because `promote_to_exhibition`'s law forbids
  a partial write without a clear — the arm's price is stated in the residual so nobody
  reinvents the SUPPLY bug.
- **The badge numbers are world-units, resolution-blind.** `TAKE_*` gate on
  `scale_x/scale_y` (wu) and the pilot's `PILOT_STANDOFF_*` (1.4 ×, min 4). Containment at
  every span: span < 2.857 → pilot 4 < reach ≥ 6; 2.857 ≤ span < 3.75 → pilot ≤ 5.25 <
  6; span ≥ 3.75 → pilot 1.4 s < 1.6 s, margin 0.2 s ≥ 0.75 wu. The predicate's `along >
  reach` comparison is strict, so equality would pass — the margin makes equality
  unreachable instead of load-bearing.
- **The dist cap is enforced on what LANDS** (`jpeg_dimensions`' SOFn walk, every path,
  Pillow or not) and its banner already binds it to the constant: "If
  PAINTING_RESOLUTION moves, this moves with it."

## UNITS, IN ORDER — every commit compiles

| unit | what | files | commit |
| --- | --- | --- | --- |
| U0 | preflight: prove the base by content | — | none |
| U1 | the constant and comment truth | `state.hpp`, `cartridge.hpp`, `gallery.hpp` | 1 |
| U2 | the dist follows | `tools/web_dist.py` | 1 |
| U3 | the badge's doorstep | `gallery.hpp` | 1 |
| U4 | ledgers; OPEN.md; push | `audit/*`, `docs/OPEN.md` | 1 |

---

## U0 — PREFLIGHT (no commit)

```
git fetch origin master                                        # P9; expect origin/master = e35b8d5d unless Jean pushed
git rev-parse claude/postcard-0 2>/dev/null; git rev-parse master
# The base is POSTCARD_0's tip: the held branch if it still exists, master if Jean merged.
git checkout -b claude/plate-0 <that tip>
# PROVE THE BASE BY CONTENT (POSTCARD_0 applied, untuned):
grep -c "inline uint32_t pick_picture_before" src/cartridges/the_board/bodies/gallery.hpp    # 1 — 0 means POSTCARD_0 absent: HALT (wrong tree)
grep -c "TAKE_REACH_MULT   = 2.2f" src/cartridges/the_board/bodies/gallery.hpp               # 1 — 0 with pick_picture_before present means Jean retuned: U3 items become DEFAULT-AND-FLAG (report his numbers, skip the value edit, keep the banner edit if its FIND matches)
grep -c "PAINTING_RESOLUTION = 512" src/cartridges/the_board/realization/state.hpp           # 1 — 0 is stale authority: HALT U1
git ls-tree HEAD src/cartridges/the_board/realization/state.hpp src/cartridges/the_board/cartridge.hpp src/cartridges/the_board/bodies/gallery.hpp tools/web_dist.py    # report; compare with AUTHORITY
sh tools/gates/glaw1/run.sh && python3 tools/gates/console_gate/run.py                       # GREEN / PASS before anything moves
```

---

## U1 — THE CONSTANT AND COMMENT TRUTH

### U1.1 — the constant reverses; PORT_5b's reason stays as history (P4) (`src/cartridges/the_board/realization/state.hpp`)

FIND
```
            // PORT_5b — 1024 -> 512 (Jean's stamp). This one number sizes
            // FIVE textures, which is the point of the ruling above and the
            // reason the diet is a single edit: Exhibition (160 -> 40 MiB),
            // Authored Staging (128 -> 32), Snapshot Staging (128 -> 32),
            // and the two offscreen snapshot targets (4 -> 1 each). It also
            // sizes the CPU scale-to-fit box in the authored loader
            // (bodies/gallery.hpp, `RES`), which reads THIS constant — so
            // the fit box and the destination cannot drift apart. Layer
            // counts are untouched; only the per-layer resolution moves.
            constexpr uint32_t PAINTING_RESOLUTION = 512;
```
REPLACE
```
            // PORT_5b — 1024 -> 512 (Jean's stamp). This one number sizes
            // FIVE textures, which is the point of the ruling above and the
            // reason the diet is a single edit: Exhibition (160 -> 40 MiB),
            // Authored Staging (128 -> 32), Snapshot Staging (128 -> 32),
            // and the two offscreen snapshot targets (4 -> 1 each). It also
            // sizes the CPU scale-to-fit box in the authored loader
            // (bodies/gallery.hpp, `RES`), which reads THIS constant — so
            // the fit box and the destination cannot drift apart. Layer
            // counts are untouched; only the per-layer resolution moves.
            //
            // PLATE_0 — 512 -> 1024 (Jean's stamp, reversing PORT_5b).
            // The diet above answered ONE machine — an old laptop that
            // could not make 681 MiB resident — and that machine has left
            // the audience. The masters the 512 wall was scaled from were
            // stubs, being replaced at this size; and the postcard
            // (POSTCARD_0) ships the wall, so the wall's resolution is
            // the program's public face. The same five textures reverse:
            // Exhibition 40 -> 160 MiB, each staging 32 -> 128, offscreen
            // color 1 -> 4, and under MSAA 4 the offscreen depth and MSAA
            // color 4 -> 16 each: +339 MiB (+318 where MSAA runs at 1).
            // Residency's witness is the FLOOR now, not a laptop — the
            // Pixel and the iPhone boot rows are the gate — and the
            // levers if either refuses are registered in OPEN.md
            // (PLATE_0 residuals), not improvised here.
            constexpr uint32_t PAINTING_RESOLUTION = 1024;
```

### U1.2 — the assert's message stops naming a resolution (`src/cartridges/the_board/realization/state.hpp`)

FIND
```
            static_assert((Dim::PAINTING_RESOLUTION * 4u) % 256u == 0u,
                "POSTCARD_0: bytesPerRow must be a multiple of 256 (WebGPU); RES * 4 is, at 512");
```
REPLACE
```
            static_assert((Dim::PAINTING_RESOLUTION * 4u) % 256u == 0u,
                "POSTCARD_0: bytesPerRow must be a multiple of 256 (WebGPU); RES * 4 is, for any RES a multiple of 64");
```

### U1.3 — the staging's banner stops naming a byte count and a megabyte (`src/cartridges/the_board/realization/state.hpp`)

FIND
```
            // POSTCARD_0 — THE POSTCARD'S STAGING, born on the first take and
            // kept for the session: a visitor who never takes a picture never
            // pays the megabyte, and the estate report at boot never names a
            // row it did not spend. One layer of the exhibition, rows packed:
            // RES * 4 = 2048 bytes a row, a multiple of 256 by construction
            // (WebGPU's bytesPerRow law), RES rows.
```
REPLACE
```
            // POSTCARD_0 — THE POSTCARD'S STAGING, born on the first take and
            // kept for the session: a visitor who never takes a picture never
            // pays for it, and the estate report at boot never names a
            // row it did not spend. One layer of the exhibition, rows packed:
            // RES * 4 bytes a row, a multiple of 256 by construction
            // (WebGPU's bytesPerRow law), RES rows.
```

### U1.4 — the copy's banner, resolution-relative (`src/cartridges/the_board/cartridge.hpp`)

FIND
```
                // eye picked; the whole layer is copied (RES x RES, rows packed
                // at 2048 B — the 256-byte law by construction) and the shell
```
REPLACE
```
                // eye picked; the whole layer is copied (RES x RES, rows packed
                // at RES * 4 B — the 256-byte law by construction) and the shell
```

### U1.5 — the streaming-cost comment, resolution-relative (`src/cartridges/the_board/bodies/gallery.hpp`)

FIND
```
// The cost is bounded and small: each arrival is one 512-px JPEG decode
// (PAINTING_CAP) and one 1 MiB WriteTexture, a few ms, and four of them in
```
REPLACE
```
// The cost is bounded and small: each arrival is one PAINTING_CAP-px JPEG
// decode and one RES*RES*4 B WriteTexture, a few ms, and four of them in
```


### WITNESS — U1
```
grep -c "PAINTING_RESOLUTION = 1024" src/cartridges/the_board/realization/state.hpp   # 1
grep -c "PLATE_0 — 512 -> 1024"      src/cartridges/the_board/realization/state.hpp   # 1
grep -rn "\b512\b" src/cartridges/the_board/realization/state.hpp src/cartridges/the_board/cartridge.hpp | grep -i "paint\|postcard\|staging\|exhibition" | grep -v "SUPPLY made snapshots 512"   # EMPTY — no stale prose survives but the historical sentence
sh tools/gates/glaw1/run.sh                        # GREEN (the asserts hold at 1024)
python3 tools/gates/console_gate/run.py            # PASS, zero diagnostics
```
Commit: `PLATE_0 U1 — paintings at 1024²: the constant reverses PORT_5b (Jean's stamp; the diet's laptop left the audience); comment truth follows the number everywhere it was written down`

---

## U2 — THE DIST FOLLOWS

### U2.1 — the cap follows the constant; quality rises for the postcard's second generation (`tools/web_dist.py`)

FIND
```
PAINTING_CAP = 512
PAINTING_QUALITY = 82
```
REPLACE
```
PAINTING_CAP = 1024      # PLATE_0 — follows Dim::PAINTING_RESOLUTION (512 -> 1024), as the banner above orders
PAINTING_QUALITY = 88    # PLATE_0 — 82 -> 88: at 1024 the wall shows the file near 1:1, and the postcard
                         # re-encodes it (JPEG 0.92), so q82's blocking would survive two generations.
```


### WITNESS — U2
```
grep -c "PAINTING_CAP = 1024"     tools/web_dist.py   # 1
grep -c "PAINTING_QUALITY = 88"   tools/web_dist.py   # 1
python3 -c "import ast; ast.parse(open('tools/web_dist.py').read()); print('parses')"
```
Commit: `PLATE_0 U2 — the dist follows: PAINTING_CAP 1024, PAINTING_QUALITY 88 (the postcard re-encodes; q82 would survive two generations)`

---

## U3 — THE BADGE'S DOORSTEP

### U3.1 — the zone's banner: the doorstep pulled in, the pilot still inside (`src/cartridges/the_board/bodies/gallery.hpp`)

FIND
```
//   viewer stands on — begin_visit's standing point is position + forward
//   x stand-off, and the pilot walks there), no deeper than TAKE_REACH_*
//   (the pilot's stand-off is 1.4 x the larger side, min 4 wu, so 2.2 x
//   and min 8 wu holds a visitor who arrived by pilot AND one who wandered
//   up close), and no further sideways than the frame plus TAKE_SLACK_MULT
//   of its width each side. Both gallery forms fall out of the one struct:
```
REPLACE
```
//   viewer stands on — begin_visit's standing point is position + forward
//   x stand-off, and the pilot walks there), no deeper than TAKE_REACH_*
//   (PLATE_0 pulled the doorstep in — Jean's eye: shown from afar, the
//   badge is noise. 1.6 x / min 6 wu still contains the pilot's stand-off,
//   1.4 x / min 4, at EVERY span — a walk ends inside the zone with at
//   least three-quarters of a wu to spare — and a wanderer keeps it),
//   and no further sideways than the frame plus TAKE_SLACK_MULT
//   of its width each side. Both gallery forms fall out of the one struct:
```

### U3.2 — the three numbers (`src/cartridges/the_board/bodies/gallery.hpp`)

FIND
```
inline constexpr float TAKE_REACH_MULT   = 2.2f;     // x the picture's larger side: how deep the zone runs
inline constexpr float TAKE_REACH_MIN_WU = 8.0f;     // and never shallower: a small picture keeps a doorstep
inline constexpr float TAKE_SLACK_MULT   = 0.5f;     // x the picture's width, each side, beyond the frame
```
REPLACE
```
inline constexpr float TAKE_REACH_MULT   = 1.6f;     // x the picture's larger side: how deep the zone runs (PLATE_0: 2.2 -> 1.6, Jean's eye)
inline constexpr float TAKE_REACH_MIN_WU = 6.0f;     // and never shallower: a small picture keeps a doorstep (8 -> 6; still past the pilot's 4)
inline constexpr float TAKE_SLACK_MULT   = 0.25f;    // x the picture's width, each side, beyond the frame (0.5 -> 0.25: before, not beside)
```


### WITNESS — U3
```
grep -c "TAKE_REACH_MULT   = 1.6f"  src/cartridges/the_board/bodies/gallery.hpp   # 1
grep -c "TAKE_REACH_MIN_WU = 6.0f"  src/cartridges/the_board/bodies/gallery.hpp   # 1
grep -c "TAKE_SLACK_MULT   = 0.25f" src/cartridges/the_board/bodies/gallery.hpp   # 1
sh tools/gates/glaw1/run.sh && python3 tools/gates/console_gate/run.py            # GREEN / PASS
```
Commit: `PLATE_0 U3 — the badge's doorstep pulled in: reach 1.6x/min 6 wu (contains the pilot's 1.4x/4 at every span), slack 0.25 (before, not beside)`

---

## U4 — LEDGERS, THE REGISTER, THE PUSH

1. The cascade, in order (D9):
   ```
   python3 tools/binding_gen.py --write        # EXPECT byte-identical (rehearsed): MANIFEST, limits_floor, features_wallet unchanged — flag any diff and include it
   python3 tools/binding_ledger.py
   python3 tools/command_census.py
   python3 tools/mirror_census.py
   python3 tools/organ_ledger.py               # rehearsed: no churn (every gallery edit sits below the cited lines) — regenerating is still the order
   python3 tools/binding_ledger.py --check && python3 tools/command_census.py --check && python3 tools/mirror_census.py --check && python3 tools/organ_ledger.py --check
   python3 tools/organ_readers.py && python3 tools/organ_gap.py --gate
   python3 tools/gates/score/run.py && python3 tools/gates/shell_gate/run.py && python3 tools/gates/glaw2/run.py && python3 tools/gates/console_gate/run.py && python3 tools/gates/sha256_gate/run.py
   python3 tools/wgsl_gate.py                  # PASS — world.wgsl untouched
   python3 tools/binding_gen.py --check        # everything PASS; S-6 red until the push
   ```
   Rehearsed diffstat: `BINDING_LEDGER.md` / `COMMAND_LEDGER.md` / `MIRROR_LEDGER.md` only —
   pins and line-cites, 188 insertions, 188 deletions. List every changed line group.

2. `docs/OPEN.md` — read whole. Three pieces of work, each anchored on a phrase from the
   entry CC wrote at POSTCARD_0 U5 (count 1 expected; a miss means the round-found
   adjustments touched it — DEFAULT-AND-FLAG: apply the same amendment by hand to the
   sentence that says the same thing, and report the wording found):

   **Amendment A — the TAKE residual.**

   FIND
   ```
   - **The four `TAKE_*` numbers are control-panel material**, enrolled nowhere until a
     measurement asks (the pilot's rule). The reach (2.2 × the larger side, min 8 wu)
     was chosen to contain the pilot's standing point (1.4 ×, min 4) with margin; the
     cone (45°) to hold a third-person eye that looks down at the pawn and past it.
     Jean's eye may move any of them; each is one number.
   ```
   REPLACE
   ```
   - **The four `TAKE_*` numbers are control-panel material**, enrolled nowhere until a
     measurement asks (the pilot's rule). PLATE_0 moved three on Jean's eye — reach
     1.6 × the larger side, min 6 wu; slack 0.25 — keeping the invariant: the reach
     contains the pilot's standing point (1.4 ×, min 4) at every span, so a walk
     ends inside the zone. The cone (45°) holds a third-person eye that looks down
     at the pawn and past it. Each is still one number.
   ```

   **Amendment B — the larger-postcard residual.**

   FIND
   ```
   - **A larger postcard for authored work is a dist decision**, not a mechanism: the
     exhibition ships at `PAINTING_CAP` 512 (the Gallery ships 640/1280 for its sets).
     The postcard is the wall's copy, second-generation JPEG. Parked at Jean's gate.
   ```
   REPLACE
   ```
   - **A larger postcard for authored work — RESOLVED at PLATE_0**: the wall itself is
     1024 (`PAINTING_CAP` 1024, q88), and the postcard ships the wall. The road past
     1024 is compressed textures, registered under PLATE_0's residuals.
   ```

   **C — the new entry**, inserted immediately ABOVE the line `## POSTCARD_0 — A PICTURE LEAVES THE WALL` (expected count of that line: 1), verbatim:

   ```
   ## PLATE_0 — THE WALL AT 1024 (on `claude/plate-0`; Jean's gates open)
   
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
   ```


   Commit: `PLATE_0 U4 — ledgers regenerated (state/gallery/cartridge pins); OPEN.md: PLATE_0 open on claude/plate-0, residuals registered; POSTCARD_0's TAKE and larger-postcard residuals amended`

3. Push BOTH branches so origin holds what the machine holds (origin/master was still
   HOME_0 on 2026-09-07): `git push -u origin claude/postcard-0 claude/plate-0` (or
   master, if POSTCARD_0 merged). Then `python3 tools/binding_gen.py --check` once more:
   S-6 green at the pushed tip.

---

## JEAN'S GATES — build, measure, then look

```
cmake --build --preset the-board-web
python tools\web_dist.py            # re-encodes the current stubs at ≤1024 q88; new masters land the same way
npx wrangler pages dev dist
```

1. **The estate, on the boot card.** `[GPU Budget]` textures line: **+339.0 MiB** over
   your last pre-PLATE boot (+318.0 if that device runs MSAA 1). The leaderboard reads
   Exhibition 160.0 with the two stagings at 128.0 behind it. Anything else is a finding.
2. **Residency, on the floor.** The Pixel and the iPhone boot the deployed build with a
   few real tabs open beside it (ecological truth), wander indoors and out, and neither
   drops the device. This is the gate PORT_5b's laptop failed; it is now the phones'
   to pass. If either refuses, stop at this row — the levers are in OPEN.md, in order.
3. **The boot's breath.** Compare the `authored6` mark against a pre-PLATE log. If it
   crosses the READY floor, the browser-decode round opens; nothing else changes today.
4. **The capture's price.** The `Photographer Snapshot` METER row on the Pixel. If the
   frame drop reads as a hitch in the world, the quadrant arm's round opens.
5. **The wall test, PORT_5b's acceptance note inverted.** Stand close: sharper is the
   expected trade, and q88 is judged here. The far wall may shimmer slightly harder —
   that is the mip residual, named, not a regression of this round.
6. **The postcard.** Take a painting: the JPEG's long edge reads 1024 (or the master's
   own, if smaller — never upscaled). Take a photograph: 1024, resampled to its hung
   aspect. Mail it to yourself; look at it at full size. This row is the campaign's
   reason.
7. **The doorstep.** Visit a painting via the roll: the walk ends with the badge lit.
   Step back to roughly 1.6 × its larger side: it dies. On a wall of several, only the
   faced one offers. From across the room: nothing.
8. **L48.** Rows 1, 2, 6 and 7 on Chrome desktop, the Pixel, and the iPhone before the
   round is called landed.

---

## THE ROUND'S REPORT

Per unit — landed / flagged / halted / deferred; witness output verbatim; every default
taken; every count that differed and what the tree said. Cite symbols. Name both branch
tips pushed. Then the gate tables are Jean's.
