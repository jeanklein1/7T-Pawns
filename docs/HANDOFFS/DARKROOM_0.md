# DARKROOM_0 — THE PICTURES DEVELOP OFF THE FRAME

*Handoff · authored 2026-09-07 · Claude → CC · Jean holds build, visual, merge, deploy.*
*Filed to `docs/HANDOFFS/DARKROOM_0.md` while open; the directory dies with the campaign.*

## THE INVESTIGATION, IN ONE PARAGRAPH

Jean asked how much of the stutter is scheduling. The frame's bursty events, read one by
one: pipelines are created asynchronously at boot (`CreateRenderPipelineAsync`), so none is
built mid-run; bundles re-record on `bundlesDirty_` and cost a few dozen calls; mesh
generation, the patch bake and the zone flush are GPU work, dirty-gated, and the meter
measured the bake at ≤ 2 ms; the readbacks are 32 agents; the photographer is now spaced
(SHUTTER_0). What remains is one event, and it is the one every campaign of this week made
heavier: **a painting's arrival.** AUBADE U5c already valved it — bytes are held on arrival
and `pump_authored_valve` develops ONE painting per frame — but the unit it develops is a
1024² JPEG decoded by stb on the main thread, then a four-tap float bilinear over a million
texels, then the edge replication, the BGRA swap, the eleven-level chain, and a 5.3 MiB
`WriteTexture`. On a weak CPU in WASM that is on the order of 80–150 ms, and the pen fills
in batches whenever a placement or the tide hangs pictures: a storm of stalls, one per
frame, each longer than a frame. *Scheduling cannot fix a unit larger than a frame; only
shrinking it or moving it off the thread can.* This round shrinks it by its second-largest
share, byte for byte, and registers the move with the recipe to prove it.

## AUTHORITY

Base: `master` at HEAD `85b0b5c6` (SHUTTER_0 settled), fetched 2026-09-07. Rides a held
branch `claude/darkroom-0` (P12: a felt gate — the arrival storm). Pinned: `gallery.hpp`
(BINDING + COMMAND); red from U1 until U2. HALT (P17, scoped P15): unreachable file; stale
authority. Else DEFAULT-AND-FLAG. Count **1** everywhere. LF only.

| file | blob at base |
| --- | --- |
| `src/cartridges/the_board/bodies/gallery.hpp` | `554cc65e10cd625e5d649733b94cde176675502f` |

**REHEARSED.** Both FINDs at count 1 on the base; G-LAW 1 GREEN, TU gate zero diagnostics;
cascade and every gate green after U2.

## THE RULINGS THIS CAMPAIGN LANDS

1. **At scale 1 the bilinear is a copy, byte for byte.** Since PLATE_0 the dist caps every
   shipped painting at RES on its long side, and the loader never scales up, so `scale` is
   exactly 1.0 for every file that can reach the pad. Then `src_xf == dx` exactly, the
   fractions are exact zeros, `v == data[i00]` exactly, and `(uint8_t)(v + 0.5f)` returns
   `data[i00]` for every value 0..255. The loop was computing a memcpy with sixteen float
   multiplies per texel. It is now a memcpy per row; the old arm stays behind `scale < 1`
   for a master past the cap that reached dist by hand.
2. **The stall is named and sized, per painting, on a weak CPU in WASM** (estimates, not
   measurements — the meter's stb accumulator sums only before first present): decode
   25–60 ms · pad 30–60 ms → ~1 ms after this round · swap ~3 · chain 10–20 · WriteTexture
   5–10. The decode is now the majority and the floor of the main-thread route.
3. **The decode leaves the thread next, and the tree's own rig proves the road before it
   is written.** Two routes, priced in the register below. Not built here.

## UNITS

| unit | what | files | commit |
| --- | --- | --- | --- |
| U0 | preflight | — | none |
| U1 | the copy at scale 1 | `gallery.hpp` | 1 |
| U2 | ledgers; OPEN.md; push | `audit/*`, `docs/OPEN.md` | 1 |

## U0 — PREFLIGHT (no commit)
```
git fetch origin master && git checkout -b claude/darkroom-0 origin/master
git ls-tree HEAD src/cartridges/the_board/bodies/gallery.hpp     # 554cc65e…
grep -c "float src_yf = (float)dy / scale;" src/cartridges/the_board/bodies/gallery.hpp   # 1
sh tools/gates/glaw1/run.sh && python3 tools/gates/console_gate/run.py
```

## U1 — THE COPY AT SCALE 1

### U1.1 — the copy at scale 1, and the old loop becomes the scale < 1 arm (`src/cartridges/the_board/bodies/gallery.hpp`)

FIND
```
    for (uint32_t dy = 0; dy < dst_h; ++dy) {
        float src_yf = (float)dy / scale;
        uint32_t sy0 = (uint32_t)src_yf;
```
REPLACE
```
    // DARKROOM_0 — AT SCALE 1 THE BILINEAR IS A COPY, BYTE FOR BYTE. Since
    // PLATE_0 the dist caps every shipped painting at RES on its long side
    // and this loader never scales UP, so `scale` is exactly 1.0 for every
    // file that can reach here: src_xf == dx and src_yf == dy exactly, the
    // fractions are exact zeros, v == data[i00] exactly, and (uint8_t)(v +
    // 0.5f) is data[i00] again for every value 0..255. The four-tap float
    // loop below was therefore computing a memcpy at ~1M texels x 4
    // channels x 4 taps, on the main thread, once per painting, inside the
    // frame — the largest single share of the arrival stall after the
    // decode itself. The arm below it stays for the one case that can
    // still need it: a master past the cap that reached dist by hand.
    if (scale >= 1.0f) {
        for (uint32_t dy = 0; dy < dst_h; ++dy)
            std::memcpy(&padded[(size_t)dy * RES * 4],
                        &data[(size_t)dy * (size_t)width * 4],
                        (size_t)dst_w * 4);
    } else {
    for (uint32_t dy = 0; dy < dst_h; ++dy) {
        float src_yf = (float)dy / scale;
        uint32_t sy0 = (uint32_t)src_yf;
```

### U1.2 — the arm closes (`src/cartridges/the_board/bodies/gallery.hpp`)

FIND
```
                padded[di + c] = (uint8_t)(v + 0.5f);
            }
        }
    }

    // MIP_0 — THE PAD IS THE EDGE, REPLICATED.```
REPLACE
```
                padded[di + c] = (uint8_t)(v + 0.5f);
            }
        }
    }
    }   // DARKROOM_0 — the scale < 1 arm ends here

    // MIP_0 — THE PAD IS THE EDGE, REPLICATED.```


### WITNESS — U1
```
grep -c "DARKROOM_0" src/cartridges/the_board/bodies/gallery.hpp   # 2 — the banner and the arm's close
grep -c "if (scale >= 1.0f) {" src/cartridges/the_board/bodies/gallery.hpp   # 1
sh tools/gates/glaw1/run.sh && python3 tools/gates/console_gate/run.py       # GREEN / PASS
```
Byte-identity, if a witness is wanted beyond the arithmetic: on the base, stage any
painting and dump `padded` (the sandbox can't run the wasm; on the machine, a temporary
`fwrite` behind a sidecar — P18, never HEAD); on the branch, the same bytes.

Commit: `DARKROOM_0 U1 — at scale 1 the pad is a copy, byte for byte: the four-tap float loop over a million texels per painting becomes a memcpy per row; the bilinear arm stays for a master past the cap`

## U2 — LEDGERS, THE REGISTER, THE PUSH
```
python3 tools/binding_ledger.py && python3 tools/command_census.py && python3 tools/mirror_census.py && python3 tools/organ_ledger.py
python3 tools/binding_ledger.py --check && python3 tools/command_census.py --check && python3 tools/mirror_census.py --check && python3 tools/organ_ledger.py --check
python3 tools/gates/score/run.py && python3 tools/gates/shell_gate/run.py && python3 tools/gates/glaw2/run.py && python3 tools/gates/sha256_gate/run.py && python3 tools/wgsl_gate.py && python3 tools/binding_gen.py --check
```
Run the provenance-stamp step. `docs/OPEN.md`, top of the register:
```
## DARKROOM_0 — THE PICTURES DEVELOP OFF THE FRAME (on `claude/darkroom-0`; Jean's gates open)

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
```
Commit: `DARKROOM_0 U2 — ledgers regenerated (gallery pin); OPEN.md: DARKROOM_0 open, the road priced`
Push: `git push -u origin claude/darkroom-0`.

## JEAN'S GATES
1. **The storm, felt.** Fly over galleries as before. The stutter is shorter per event — the
   pad's share gone — but not gone: the decode remains. If it feels *unchanged*, the pad was
   not the share I estimated and the decode dominates outright; that only strengthens the
   road.
2. **DevTools › Performance, one recording of a flight.** The long tasks during a batch: on
   master ~80–150 ms each; on the branch, shorter by the pad's share. This is the
   measurement this round could not take from here, and it costs one minute.
3. **Every painting looks exactly as it did** — byte identity is the ruling; a difference is
   a finding.
