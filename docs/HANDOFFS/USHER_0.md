# USHER_0 — the mood at the door

**Repo**: `jeanklein1/7T-Pawns`, master. **Trunk rule**: CC commits directly to
master; nothing here is land-gated, so no held branch. One commit per unit, in
unit order, messages word-for-word as given. FLAG-AND-FINISH within a unit;
STOP only on anchor mismatch inside the unit being edited (STOP is scoped to
that unit — flag it, land the rest, report). Cite symbols, not line numbers.
Place this file at `docs/HANDOFFS/USHER_0.md` in the U1 commit; U5 retires it.

**Preflight** (CLAUDE.md): `git rev-parse --is-shallow-repository` → unshallow
if true. Confirm the anchor blobs below before editing; a moved blob is a
stale-authority STOP for its unit only.

**Anchors (blob SHAs at authoring time, HEAD `cf0d71e`):**

| file | blob |
| --- | --- |
| `src/cartridges/the_board/contracts/mood_constants.hpp` | `93ea6a55` |
| `src/cartridges/the_board/direction/mood.hpp` | `25289600` |
| `src/cartridges/the_board/machine/entity_pipeline.hpp` | `9d0fa2ea` |
| `src/cartridges/the_board/cartridge.hpp` | `3b9fc293` |
| `src/cartridges/the_board/contracts/demo_config.hpp` | `a025e828` |
| `src/cartridges/the_board/demos/matrix.hpp` | `af801376` |
| `src/cartridges/the_board/contracts/spine_state.hpp` | `83e7bcc4` |

**The four rulings, one sentence each:**

1. **Finite outdoor goes dark** — `mood_weights[3] → 0`. The bank's own
   mechanism ("0 shuts a door without unmaking the mood"); every other
   surface of the mood stands, and the panel dial *draw · finite outdoor*
   reopens it the day a design asks.
2. **No door to here** — the destination walk gains `exclude_mood` and every
   forward draw bars the standing world; the triad's deeper-in coin mirrors
   to the other room. The back-portal is exempt by nature: it is a return,
   not a destination draw.
3. **The boot mood is drawn, not authored** — same doctrine as the radius:
   the seed decides, under the destination law, at the one authoring site.
   `DemoConfig.boot_mood` dies; `?mood=` stays as the forcing override;
   `?seed=` now pins the mood with the world it pins.
4. **The rooms come down a rung** — `SHAPE_ROOM_FLAT` / `SHAPE_ROOM_VAULT`
   `finite_radius_max` 4 → 3: largest room 7×7 patches / 350 wu a side
   (was 9×9 / 450). `SHAPE_FINITE` untouched — a dark mood keeps its
   definition whole.

Every unit is **pixel-affecting** (world structure); Jean gates build
(glaw1 + `cmake --build --preset the-board-web`) and visual once at the end.
No mirrors, no bindings, no WGSL, no enrollment rows move: 0 / 0 / 0 / 0.

---

## U1 — finite outdoor's door shut

**File**: `contracts/mood_constants.hpp` · symbol `WORLD_DRAW_TABLE`.

FIND (expect 1 match):

```
    { 0.20f, 0.20f, 0.20f, 0.10f, 0.15f, 0.15f, 0.02f },   // mood_weights by id: sunset, flat, vault,
                                                           // finite, night, noon, ATRIUM — PORTAL_2's
                                                           // 0.10 finite kept; the old even thirds
                                                           // re-cut to seat the two new skies
                                                           // (ATMOS_1); the atrium is the rarest door
                                                           // in the open field (ATRIUM_1) — the walk
                                                           // normalises
```

REPLACE:

```
    { 0.20f, 0.20f, 0.20f, 0.00f, 0.15f, 0.15f, 0.02f },   // mood_weights by id: sunset, flat, vault,
                                                           // finite, night, noon, ATRIUM — finite
                                                           // outdoor's door SHUT (USHER_0: weight 0,
                                                           // mood kept; the panel dial reopens it);
                                                           // the atrium is the rarest door in the
                                                           // open field (ATRIUM_1) — the walk
                                                           // normalises
```

Reachability after U1: audience roads into `MOOD_FINITE_OUTDOOR` are the open
walk (now skips it), the triad (never offered it — flat/vault/open only), and
the fallback (same walk). Dev roads stay: `?mood=3` and the keyboard request.
A panel **import** carrying an old export can reopen the door for that session
— the panel is a hand on the live surface, not a defect.

Commit: `USHER_0 U1 — finite outdoor's door shut: weight 0, mood kept`

---

## U2 — no door to here

**Files**: `direction/mood.hpp`, `machine/entity_pipeline.hpp`.

**a — the declaration** (symbol block above `MODULE IMPLEMENTATION`).
FIND (1):

```
uint32_t pick_portal_mood(uint32_t seed, uint32_t prop);
```

REPLACE:

```
uint32_t pick_portal_mood(uint32_t seed, uint32_t prop, uint32_t exclude_mood);
```

**b — the walk** (symbols `pick_mood_weighted_`, `pick_portal_mood`,
`pick_open_mood`). FIND (1):

```
// open_only restricts the walk to shape_is_open moods: the triad's way
// OUT of a room is an open sky, whichever one the weights favour.
inline uint32_t pick_mood_weighted_(uint32_t seed, uint32_t prop, bool open_only) {
    float sum = 0.0f;
    for (uint32_t m = 0; m < MOOD_COUNT; ++m) {
        if (open_only && !shape_is_open(mood_def(m).shape)) continue;
        sum += std::max(0.0f, WORLD_DRAW_LIVE.mood_weights[m]);
    }
    if (sum <= 0.0f) return MOOD_OPEN_SUNSET;   // every door shut: the home sky
    const float roll = cpu_hash_f(seed, prop) * sum;
    float cumul = 0.0f;
    uint32_t pick = MOOD_OPEN_SUNSET;
    for (uint32_t m = 0; m < MOOD_COUNT; ++m) {
        if (open_only && !shape_is_open(mood_def(m).shape)) continue;
        const float w = std::max(0.0f, WORLD_DRAW_LIVE.mood_weights[m]);
        if (w <= 0.0f) continue;
        cumul += w;
        pick = m;
        if (roll < cumul) break;
    }
    return pick;
}
inline uint32_t pick_portal_mood(uint32_t seed, uint32_t prop) { return pick_mood_weighted_(seed, prop, false); }
inline uint32_t pick_open_mood  (uint32_t seed, uint32_t prop) { return pick_mood_weighted_(seed, prop, true);  }
```

REPLACE:

```
// open_only restricts the walk to shape_is_open moods: the triad's way
// OUT of a room is an open sky, whichever one the weights favour.
// exclude_mood bars ONE row for this walk — the world you stand in is
// not a destination (USHER_0). MOOD_COUNT bars nothing: no row wears
// that id, so the comparison never fires — the boot draw's spelling.
// The every-door-shut fallback stands as written; a degenerate panel
// config earns the home sky, not a missing door.
inline uint32_t pick_mood_weighted_(uint32_t seed, uint32_t prop, bool open_only, uint32_t exclude_mood) {
    float sum = 0.0f;
    for (uint32_t m = 0; m < MOOD_COUNT; ++m) {
        if (m == exclude_mood) continue;
        if (open_only && !shape_is_open(mood_def(m).shape)) continue;
        sum += std::max(0.0f, WORLD_DRAW_LIVE.mood_weights[m]);
    }
    if (sum <= 0.0f) return MOOD_OPEN_SUNSET;   // every door shut: the home sky
    const float roll = cpu_hash_f(seed, prop) * sum;
    float cumul = 0.0f;
    uint32_t pick = MOOD_OPEN_SUNSET;
    for (uint32_t m = 0; m < MOOD_COUNT; ++m) {
        if (m == exclude_mood) continue;
        if (open_only && !shape_is_open(mood_def(m).shape)) continue;
        const float w = std::max(0.0f, WORLD_DRAW_LIVE.mood_weights[m]);
        if (w <= 0.0f) continue;
        cumul += w;
        pick = m;
        if (roll < cumul) break;
    }
    return pick;
}
inline uint32_t pick_portal_mood(uint32_t seed, uint32_t prop, uint32_t exclude_mood) { return pick_mood_weighted_(seed, prop, false, exclude_mood); }
inline uint32_t pick_open_mood  (uint32_t seed, uint32_t prop) { return pick_mood_weighted_(seed, prop, true,  MOOD_COUNT); }
```

`pick_open_mood` keeps its two-arg face: its only caller is the triad, which
runs in finite worlds — the standing mood is never open there, so open-only
already bars self structurally. YAGNI on the third arg.

**c — the triad's coin** (symbol `force_spawn_finite_portals`). FIND (1):

```
    // PORTAL_2 — THE TRIAD. A finite world offers exactly three
    // doors: deeper in (one of the two rooms, seed's coin), out — an
    // open sky, drawn by the destination law's weights among the open
    // moods (ATMOS_1) — and back (already standing). Two forwards here;
    // the radius no longer buys doors.
    constexpr uint32_t count = 2;
    const uint32_t fwd_moods[2] = {
        (cpu_hash_f(c->world_state_.active_seed, 7950u) < 0.5f)
            ? MOOD_INDOOR_FLAT : MOOD_INDOOR_VAULT,
        pick_open_mood(c->world_state_.active_seed, 7951u),   // the way out: an open sky (ATMOS_1)
    };
```

REPLACE:

```
    // PORTAL_2 — THE TRIAD. A finite world offers exactly three
    // doors: deeper in (a room — standing IN a room, always the OTHER
    // room; elsewhere the seed's coin — USHER_0, no door to here), out
    // — an open sky, drawn by the destination law's weights among the
    // open moods (ATMOS_1) — and back (already standing). Two forwards
    // here; the radius no longer buys doors.
    constexpr uint32_t count = 2;
    uint32_t deeper = (cpu_hash_f(c->world_state_.active_seed, 7950u) < 0.5f)
        ? MOOD_INDOOR_FLAT : MOOD_INDOOR_VAULT;
    if (deeper == c->mood_state_.active)
        deeper = (deeper == MOOD_INDOOR_FLAT) ? MOOD_INDOOR_VAULT : MOOD_INDOOR_FLAT;
    const uint32_t fwd_moods[2] = {
        deeper,
        pick_open_mood(c->world_state_.active_seed, 7951u),   // the way out: an open sky (ATMOS_1)
    };
```

A deterministic mirror, not a re-roll: inside a room the coin is moot by the
rule itself, and no loop enters the tree.

**d — the open-field roll** (`entity_pipeline.hpp`, arch-commit portal block;
context line `if (portal_roll < WORLD_DRAW_LIVE.portal_density)`). FIND (1):

```
            uint32_t mood = pick_portal_mood(aa.position_hash, 2u);
```

REPLACE:

```
            uint32_t mood = pick_portal_mood(aa.position_hash, 2u, c->mood_state_.active);
```

**e — the door guarantee** (symbol `force_spawn_door_fallback`). FIND (1):

```
    uint32_t mood = pick_portal_mood(c->world_state_.active_seed, 8900u);
```

REPLACE:

```
    uint32_t mood = pick_portal_mood(c->world_state_.active_seed, 8900u, c->mood_state_.active);
```

**f — FLAGGED stale-comment fold** (skip on mismatch, never STOP). ATTIC_ATRIUM
took the arc to the attic; this sentence still claims a dispatch that no
longer exists and would mislead a careful reader. Symbol
`force_spawn_forward_portals`. FIND (1):

```
    // ATRIUM_2 — the roster is the SHAPE's, not this function's. One
    // property, two rosters; every other finite world keeps the triad.
    force_spawn_finite_portals(c, queue, machine_ctx);
```

REPLACE:

```
    // One roster: the triad. ATRIUM_2's arc went to the attic with the
    // entrance (ATTIC_ATRIUM); every finite world keeps the triad.
    force_spawn_finite_portals(c, queue, machine_ctx);
```

**Gate after U2**: `python3 tools/organ_readers.py`. `pick_mood_weighted_`
remains the reader of `mood_weights`; if the tool goes red on a textual
signature match, update the reader declaration where the tool points and note
it in the report. FLAG, don't improvise beyond that.

Commit: `USHER_0 U2 — no door to here: the walk bars the standing mood`

---

## U3 — the boot mood is drawn, not authored

**Files**: `cartridge.hpp`, `contracts/demo_config.hpp`, `demos/matrix.hpp`,
`contracts/spine_state.hpp`.

**a — the authoring site** (Cartridge ctor; directly under the boot-card
block). Salt `9001u` is fresh — no 9xxx series exists in the tree. FIND (1):

```
                mood_state_.active = DEMO.boot_mood;
                // B9 — a mood present at boot (?mood= / --mood=) forces the
                // boot mood at this one authoring site; an out-of-range
                // index is refused OUT LOUD (P6 — a switch that half-fired
                // must not look fired).
                if (boot_params().has_mood) {
                    if (boot_params().mood < MOOD_COUNT) {
                        mood_state_.active = boot_params().mood;
                    } else {
                        std::cout << "[Params] mood=" << boot_params().mood
                                  << " out of range (MOOD_COUNT="
                                  << MOOD_COUNT << ") — ignored\n";
                    }
                }
```

REPLACE:

```
                // USHER_0 — THE BOOT MOOD IS DRAWN, NOT AUTHORED. Same
                // doctrine as the radius below: the seed decides, under the
                // destination law the panel already dials (mood_weights) —
                // one law for every arrival, portal or boot. MOOD_COUNT
                // bars nothing: a boot has no standing world to exclude.
                // ?seed= therefore pins the mood with the world it pins.
                mood_state_.active = pick_portal_mood(world_state_.active_seed, 9001u, MOOD_COUNT);
                const char* mood_origin = "draw";
                // B9 — a mood present at boot (?mood= / --mood=) forces the
                // boot mood at this one authoring site; an out-of-range
                // index is refused OUT LOUD (P6 — a switch that half-fired
                // must not look fired).
                if (boot_params().has_mood) {
                    if (boot_params().mood < MOOD_COUNT) {
                        mood_state_.active = boot_params().mood;
                        mood_origin = "param";
                    } else {
                        std::cout << "[Params] mood=" << boot_params().mood
                                  << " out of range (MOOD_COUNT="
                                  << MOOD_COUNT << ") — ignored\n";
                    }
                }
                // THE WITNESS (P6) — the drawn mood is reportable like the
                // drawn seed above it, on the same two roads: stdout and
                // the boot card.
                std::cout << "[World] Boot mood=" << mood_name(mood_state_.active)
                          << " (" << mood_origin << ")\n";
                t7::card_fact(std::string("world  mood=") + mood_name(mood_state_.active)
                              + " (" + mood_origin + ")");
```

**b — the contract sheds the field** (`demo_config.hpp`). Three FINDs, 1 each:

```
// A DEMO is a sentence in the roster's grammar: one piece-enable
// manifest + one world seed + one boot mood. v0 carries NOTHING else
```
→
```
// A DEMO is a sentence in the roster's grammar: one piece-enable
// manifest + one world seed. v0 carries NOTHING else
```

```
// AXES THIS TYPE GROWS (D1-D5, the demo contract): D1 piece manifest
// (here), D2 world params (seed + boot mood here; radii/finiteness
// later), D3 design-table overrides, D4 coupling/canvas selection,
```
→
```
// AXES THIS TYPE GROWS (D1-D5, the demo contract): D1 piece manifest
// (here), D2 world params (the seed here; the mood and the radius are
// DRAWN from it — USHER_0), D3 design-table overrides, D4 coupling/canvas selection,
```

```
struct DemoConfig {
    Roster   roster;      // D1 — which pieces exist
    uint32_t seed;        // D2 — the world master seed (WorldState boot)
    uint32_t boot_mood;   // D2 — the mood the world wakes in
};
```
→
```
struct DemoConfig {
    Roster   roster;      // D1 — which pieces exist
    uint32_t seed;        // D2 — the world master seed (WorldState boot)
};
```

**c — the matrix sheds its column** (`matrix.hpp`). Seven FINDs, 1 each:

```
#include "cartridges/the_board/contracts/mood_constants.hpp"    // MOOD_OPEN_SUNSET
```
→ delete the line (its only consumers die below; verify with
`grep -n MOOD_ src/cartridges/the_board/demos/matrix.hpp` → zero hits after).

```
// DemoConfig is untouched — seed + boot_mood are authored onto the
// root organs in the Cartridge ctor.
```
→
```
// DemoConfig is untouched — the seed is authored onto the root organ
// in the Cartridge ctor (the boot mood is DRAWN from it — USHER_0).
```

```
// Adding a demo = one enum value here + one grid column + one seed +
// one boot_mood. A bad INCUBATE_DEMO=<name> resolves to DemoCol::<name>
```
→
```
// Adding a demo = one enum value here + one grid column + one seed.
// A bad INCUBATE_DEMO=<name> resolves to DemoCol::<name>
```

```
inline constexpr uint32_t DEMO_BOOT_MOOD[static_cast<uint32_t>(DemoCol::COUNT)] = {
    /* full    */ MOOD_OPEN_SUNSET,
    /* minimal */ MOOD_OPEN_SUNSET,
};
```
→ delete whole block.

```
    return DemoConfig{
        column_to_roster(d),
        DEMO_SEED[static_cast<uint32_t>(d)],
        DEMO_BOOT_MOOD[static_cast<uint32_t>(d)],
    };
```
→
```
    return DemoConfig{
        column_to_roster(d),
        DEMO_SEED[static_cast<uint32_t>(d)],
    };
```

```
//     ONE field is deliberately no longer byte-equal: boot_mood. The
//     old headers booted into open_default, and the mood cut retired
//     that mood; the two columns then booted into open_sunset, the
//     surviving open outdoor world. Both columns boot into the atrium
//     now (ATRIUM_1) — the entrance is the visitor's first room — and
//     the golden pins the new value.
```
(NOTE: this paragraph is already stale at HEAD — its last sentence describes
the ATRIUM_1 era; ATTIC_ATRIUM re-pinned the asserts without it. It dies with
the field either way.) →
```
//     ONE field left the pin entirely: boot_mood left DemoConfig when
//     the boot mood became a DRAW under the destination law (USHER_0)
//     — the seed golden below is now the whole of the pin.
```

The two golden asserts (both, 1 each):

```
static_assert(demo_config(DemoCol::full).seed == 42 &&
              demo_config(DemoCol::full).boot_mood == MOOD_OPEN_SUNSET,
    "GOLDEN: demo=full seed must equal old full.hpp; boot_mood is the open field (ATTIC_ATRIUM)");
```
→
```
static_assert(demo_config(DemoCol::full).seed == 42,
    "GOLDEN: demo=full seed must equal old full.hpp");
```

```
static_assert(demo_config(DemoCol::minimal).seed == 42 &&
              demo_config(DemoCol::minimal).boot_mood == MOOD_OPEN_SUNSET,
    "GOLDEN: demo=minimal seed must equal old minimal.hpp; boot_mood is the open field (ATTIC_ATRIUM)");
```
→
```
static_assert(demo_config(DemoCol::minimal).seed == 42,
    "GOLDEN: demo=minimal seed must equal old minimal.hpp");
```

**d — the spine's comment** (`spine_state.hpp`, `struct MoodState`). FIND (1):

```
    uint32_t active = 0;  // authored at the composition root (Cartridge ctor) from DEMO.boot_mood
```

REPLACE:

```
    uint32_t active = 0;  // drawn at the composition root (Cartridge ctor) under the destination law (USHER_0)
```

**Gate after U3**: `python3 tools/gates/console_gate/run.py` (the TU gate
type-checks the ctor and the shrunken aggregate).

Commit: `USHER_0 U3 — the boot mood is drawn, not authored; DemoConfig sheds boot_mood`

---

## U4 — the rooms come down a rung

**File**: `contracts/spine_state.hpp` · symbols `SHAPE_ROOM_FLAT`,
`SHAPE_ROOM_VAULT`. The ghost `roster` column label dies under the same brush
(the field left with ATTIC's arc; the label stayed — 13 labels over 12
fields at HEAD). FIND (1):

```
//                                              fin    r_min r_max indoor ceil                wall_h amp_c  zones aura  cull   roster               scheme         palette
inline constexpr WorldShape SHAPE_OPEN       = { false, 2,    2,    false, CeilingType::NONE,  0.0f,  0.0f,  true, true, true,  SCHEME_ROLL,   PALETTE_ROLL };
inline constexpr WorldShape SHAPE_ROOM_FLAT  = { true,  1,    4,    true,  CeilingType::FLAT,  20.0f, 0.5f,  true, true, false, SCHEME_ROLL,   PALETTE_ROLL };
inline constexpr WorldShape SHAPE_ROOM_VAULT = { true,  1,    4,    true,  CeilingType::VAULT, 25.0f, 0.5f,  true, true, false, SCHEME_ROLL,   PALETTE_ROLL };
```

REPLACE:

```
//                                              fin    r_min r_max indoor ceil                wall_h amp_c  zones aura  cull   scheme         palette
inline constexpr WorldShape SHAPE_OPEN       = { false, 2,    2,    false, CeilingType::NONE,  0.0f,  0.0f,  true, true, true,  SCHEME_ROLL,   PALETTE_ROLL };
inline constexpr WorldShape SHAPE_ROOM_FLAT  = { true,  1,    3,    true,  CeilingType::FLAT,  20.0f, 0.5f,  true, true, false, SCHEME_ROLL,   PALETTE_ROLL };
inline constexpr WorldShape SHAPE_ROOM_VAULT = { true,  1,    3,    true,  CeilingType::VAULT, 25.0f, 0.5f,  true, true, false, SCHEME_ROLL,   PALETTE_ROLL };
```

`derive_finite_radius` clamps nothing and asserts nothing on `r_max`; the
column witnesses probe `finite`, `indoor`, `wall_height`,
`allow_frustum_cull` — none moves. `SHAPE_FINITE` and `SHAPE_ATRIUM` stand.
Standing rooms whose destination already rolled radius 4 (a live back-portal's
`back_portal_return_radius`, an already-drawn `PortalDestination`) keep it —
the design governs fresh rolls, and that is the intended edge, not a defect.

Commit: `USHER_0 U4 — the rooms come down a rung: r_max 4 → 3`

---

## U5 — the register written, the order retired

Append to `docs/OPEN.md` (one section, its lines die when the items close):

```
## USHER_0/P — parked residue

- finite_outdoor is DARK, not cut: `mood_weights[3] = 0` (mood_constants.hpp);
  every other surface of the mood stands, `?mood=3` and the panel dial reach it
  · origin: USHER_0 U1 · unblocks: a finite-outdoor design worth a door (reopen
  = one table cell), or a ruling to cut the mood whole — its own campaign
  (MOOD_COUNT shrinks, ids renumber, every per-mood table loses a row).
- the boot draw includes the atrium at its portal weight (0.02): roughly one
  visitor in fifty wakes in the small dark room · origin: USHER_0 U3 ·
  unblocks: Jean's visual gate — if unwanted, the lever is the same weight
  dial (which also shuts its portal) or a one-line exclusion at the boot draw.
```

Then delete `docs/HANDOFFS/USHER_0.md` — the directory exists only while work
is open; absence is health.

Commit: `USHER_0 U5 — the register written, the order retired`

---

## Jean's gate — what the witnesses should say

- Reload without params, several times: `[World] Boot seed=N (draw)` then
  `[World] Boot mood=<name> (draw)`; names vary ≈ sunset/flat/vault 21% each,
  night/noon 16% each, atrium 2%; **never** `finite_outdoor`. The boot card
  carries `world  mood=…`.
- `?seed=` pins seed **and** mood together; `?mood=3` still boots the walled
  outdoor and prints `(param)`.
- Open fields: no red doors anywhere; no door wearing the standing sky's own
  colour (no lilac in sunset, no silver in night, no cyan in noon).
- Rooms: deeper-in door in `indoor_flat` is always yellow (vault) and in
  `indoor_vault` always orange (flat); the shell log's `finite NxN` tops out
  at `7x7`.
