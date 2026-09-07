# SHUTTER_0 — THE SPACING FLOOR

*Handoff · authored 2026-09-08 · Claude → CC · Jean holds build, visual, merge, deploy.*
*Filed to `docs/HANDOFFS/SHUTTER_0.md` while open; the directory dies with the campaign.*

## THE IDEA, IN ONE PARAGRAPH

The stutter every few seconds is the photographer, and the tree has known it since PURSE_0
recorded "+10 ms every 1–2 s" at 512² — PLATE_0 then quadrupled each shot. PURSE_0's
headroom gate schedules the cost onto frames that made their refresh, but on a machine that
mostly makes refresh the gate is almost always open, and its own banner says the limit out
loud: *the rule schedules the cost, it does not shrink it.* Two facts from the scheduler
finish the diagnosis: the trigger is DISTANCE — deliberately, "the travelogue" — so a ribbon
ride crossing 50 wu in one or two seconds requests shots at several times a walk's rate; and
one trigger can burst up to four captures twelve frames apart, four hitches inside a second.
The fix Jean asked for, composed to keep the travelogue: a wall-clock floor BETWEEN
CAPTURES. A walk (~8 s between triggers) never meets a 5-second floor and is untouched; a
ride meets it immediately, and bursts dissolve into spaced singles. Same shots, same wanted
places, a calmer clock. Nothing about any picture changes — this is when, not what.

## AUTHORITY

Base: `master` at HEAD `89df9510` (GATHER_0 settled), fetched 2026-09-08. Rides a **held
branch `claude/shutter-0`** (P12: pacing of a world behavior is a felt gate — Jean's ride).

| file | blob at base |
| --- | --- |
| `src/cartridges/the_board/bodies/gallery.hpp` | `30f3ebcb8be53dd84ccf1ac931c76d1ef0567d6a` |

**Pinned:** `gallery.hpp` (BINDING + COMMAND); red from U1 until U2 regenerates. HALT (P17,
scoped P15): unreachable file; stale authority. Else DEFAULT-AND-FLAG. Count **1**
everywhere. LF only.

**REHEARSED.** All four FINDs at count 1 on the base; G-LAW 1 GREEN, TU gate zero
diagnostics; cascade and every gate green after U2 (three ledgers, pins and cites only).

## THE RULINGS THIS CAMPAIGN LANDS

1. **The floor is between captures, not triggers.** Distance stays the trigger (the
   travelogue is intent, stated in the config's own banner); the floor only spreads what
   distance requests. Bursts survive as bursts of INTENT — `pending_shots` still counts
   them — they just fire 5 s apart instead of 12 frames apart.
2. **Spacing sits before the defer clock arms.** A shot inside the floor is not "waiting
   for headroom"; it is not due. So `defer_since` starts only once spacing is satisfied,
   and PURSE_0's bounded-starvation ceiling still measures exactly what it measured —
   headroom starvation — and can never be starved into overriding the floor.
3. **One number, Jean's.** `MIN_CAPTURE_SPACING_S = 5.0` — from the felt "every few
   seconds" and the measured every-1–2-s. Walking cadence (mean 50 wu ≈ 8 s) is above it
   by design, so on foot the program is byte-for-byte the same schedule. NAMED CONSTANT,
   NOT A DIAL, same reasoning as PURSE_0's: nobody sees a photograph being taken.
4. **The cost per shot is untouched** — 1024², the resolution Jean stamped at PLATE_0. The
   512-into-the-layer arm stays registered (PLATE_0 residuals) as the weak-device lever if
   a floor phone ever asks; this round does not spend it.

## THE ARITHMETIC (the witness's expectations)

Walk ~6 wu/s: triggers every ~8.3 s; floor never binds; unchanged. Ride ~40 wu/s: triggers
every ~1.25 s, bursts of 1–4 — the old worst case approached 3–4 captures/s; with the floor,
exactly ≤ 0.2 captures/s, each on a headroom frame when one exists. Pool fill during a long
ride slows to 5 s/shot — the tide and the outdoor frames keep cycling what the pool holds,
and the statue test never depended on captures (a stationary point accrues no distance).

## UNITS, IN ORDER

| unit | what | files | commit |
| --- | --- | --- | --- |
| U0 | preflight | — | none |
| U1 | the floor: constant, state, gate, stamp | `gallery.hpp` | 1 |
| U2 | ledgers; OPEN.md; push | `audit/*`, `docs/OPEN.md` | 1 |

---

## U0 — PREFLIGHT (no commit)

```
git fetch origin master && git checkout -b claude/shutter-0 origin/master
git ls-tree HEAD src/cartridges/the_board/bodies/gallery.hpp        # 30f3ebcb…
grep -c "PHOTO_DEFER_MAX_S = 4.0f" src/cartridges/the_board/bodies/gallery.hpp   # 1
sh tools/gates/glaw1/run.sh && python3 tools/gates/console_gate/run.py
```

---

## U1 — THE FLOOR

### U1.1 — the floor, beside the headroom gate it composes with (`src/cartridges/the_board/bodies/gallery.hpp`)

FIND
```
    static constexpr uint32_t PHOTO_HEADROOM_K = 1u;
    static constexpr float    PHOTO_DEFER_MAX_S = 4.0f;
```
REPLACE
```
    static constexpr uint32_t PHOTO_HEADROOM_K = 1u;
    static constexpr float    PHOTO_DEFER_MAX_S = 4.0f;
    // SHUTTER_0 — THE SPACING FLOOR, wall-clock seconds between CAPTURES
    // (not triggers). The trigger is distance — "the travelogue", and that
    // stays: a walk crosses 50 wu in ~8 s and never meets this floor, a
    // ride crosses it in ~1-2 s and a burst of four fired inside a second.
    // Nobody watches a photograph being taken; everybody feels four
    // dropped frames in a row. The floor spreads what distance requests:
    // same shots, same places wanted, a calmer clock. It composes with
    // PURSE_0's headroom gate above (spacing first, then headroom), and
    // the defer ceiling starts counting only once spacing is satisfied,
    // so bounded starvation still means what it meant. Jean's number.
    static constexpr float    MIN_CAPTURE_SPACING_S = 5.0f;
```

### U1.2 — the state remembers its last fire (`src/cartridges/the_board/bodies/gallery.hpp`)

FIND
```
struct PhotographerState {
    float cumulative_distance = 0.0f;
```
REPLACE
```
struct PhotographerState {
    float cumulative_distance = 0.0f;
    // SHUTTER_0 — when the last capture FIRED, in TimeState seconds; the
    // spacing floor measures from here. Very negative: the first shot of a
    // session owes no spacing.
    float last_capture_s = -1.0e9f;
```

### U1.3 — the floor, before the defer clock arms (`src/cartridges/the_board/bodies/gallery.hpp`)

FIND
```
        const double now = c->time_state_.seconds;
        if (gs.photographer.defer_since < 0.0f) gs.photographer.defer_since = now;
```
REPLACE
```
        const double now = c->time_state_.seconds;
        // SHUTTER_0 — the spacing floor, BEFORE the defer clock arms: a
        // shot still inside the floor is not "waiting for headroom", it is
        // not due yet, so the ceiling cannot be starved into overriding
        // the floor. Distance keeps accruing; nothing else advances.
        if (now - gs.photographer.last_capture_s
            < PhotographerCaptureConfig::MIN_CAPTURE_SPACING_S) return;
        if (gs.photographer.defer_since < 0.0f) gs.photographer.defer_since = now;
```

### U1.4 — the fire stamps the clock (`src/cartridges/the_board/bodies/gallery.hpp`)

FIND
```
        capture_snapshot(gs, c, px, pz, queue);
        gs.photographer.pending_shots--;
```
REPLACE
```
        capture_snapshot(gs, c, px, pz, queue);
        gs.photographer.last_capture_s = (float)now;   // SHUTTER_0 — the floor measures from the fire
        gs.photographer.pending_shots--;
```


### WITNESS — U1
```
grep -c "MIN_CAPTURE_SPACING_S" src/cartridges/the_board/bodies/gallery.hpp   # 2 — the constant and the gate that reads it
grep -c "last_capture_s"        src/cartridges/the_board/bodies/gallery.hpp   # 3 — the member, the gate, the stamp
sh tools/gates/glaw1/run.sh && python3 tools/gates/console_gate/run.py        # GREEN / PASS
```
Commit: `SHUTTER_0 U1 — the spacing floor: wall-clock seconds between captures; the travelogue's distance trigger stays, a ride's clusters spread to singles; composes after PURSE_0's headroom gate, before the defer clock arms`

---

## U2 — LEDGERS, THE REGISTER, THE PUSH

```
python3 tools/binding_ledger.py && python3 tools/command_census.py && python3 tools/mirror_census.py && python3 tools/organ_ledger.py
python3 tools/binding_ledger.py --check && python3 tools/command_census.py --check && python3 tools/mirror_census.py --check && python3 tools/organ_ledger.py --check
python3 tools/gates/score/run.py && python3 tools/gates/shell_gate/run.py && python3 tools/gates/glaw2/run.py && python3 tools/gates/sha256_gate/run.py && python3 tools/wgsl_gate.py && python3 tools/binding_gen.py --check
```
Run the provenance-stamp step (PLATE_0 U4d). Rehearsed diffstat: ledgers only, pins and
cites. `docs/OPEN.md`, top of the register:
```
## SHUTTER_0 — THE SPACING FLOOR (on `claude/shutter-0`; Jean's gates open)

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
```
Commit: `SHUTTER_0 U2 — ledgers regenerated (gallery pin); OPEN.md: SHUTTER_0 open on claude/shutter-0`
Push: `git push -u origin claude/shutter-0`.

---

## JEAN'S GATES

1. **The ride, felt.** The same flight that stuttered: the every-few-seconds rhythm is
   gone. What remains, if anything, is a rarer single hitch no closer than 5 s to the
   last — or none, when headroom frames absorb it.
2. **The walk, unchanged.** On foot, the console's capture lines land at the old cadence
   (~8 s and slower in sand); the floor never binds there.
3. **The world still photographs itself.** After a long ride, land and walk: the pool
   resumes at walking cadence; outdoor frames keep turning over.
4. **The number.** 5.0 is a proposal. If the ride wants calmer, 8; if the travelogue
   thins too much for your eye, 3. One constant, your stamp.
