# LODESTAR_0 — a door within reach

**Repo**: `jeanklein1/7T-Pawns`, master. **Trunk rule**: CC commits directly to
master; nothing here is land-gated, so no held branch. One commit per unit, in
unit order, messages word-for-word as given. FLAG-AND-FINISH within a unit;
STOP only on FIND mismatch inside the unit being edited (scoped to that unit —
flag it, land the rest, report). Cite symbols, not line numbers. Place this
file at `docs/HANDOFFS/LODESTAR_0.md` in the U1 commit; U5 retires it.

**Preflight**: `git rev-parse --is-shallow-repository` → unshallow if true.
USHER_0 may land before or after this campaign — its edits are DISJOINT from
every FIND below, so blob drift on `entity_pipeline.hpp` caused by USHER's own
units is expected and is NOT a stale-authority STOP. The STOP condition here is
a FIND mismatch, nothing else.

**Anchors (blob SHAs at authoring time, HEAD `cf0d71e`):**

| file | blob |
| --- | --- |
| `src/cartridges/the_board/contracts/spawn_services.hpp` | `e96baad5` |
| `src/cartridges/the_board/machine/spawn_engine.hpp` | `de99769c` |
| `src/cartridges/the_board/machine/entity_pipeline.hpp` | `9d0fa2ea` |
| `src/cartridges/the_board/bodies/grounded.hpp` | `ef27e09f` |
| `docs/OPEN.md` | `30963803` |

**What this campaign is.** The open field's only door guarantee is at
population (`force_spawn_door_fallback`: one shot, 120 wu around ORIGIN, "it
does not re-run"). After the first step the walk is raw dice — doorway rate
0.05 × 0.75 × 0.74 ≈ 0.028/patch, one roll, one placement candidate — and the
dice lose twice: Poisson gaps (~4% of points >300 wu from any portal) and the
60 wu pyramid/antenna separation shadows that eat up to half the ground in the
hot themes. LODESTAR adds a floor without touching a wall: one patch per
8×8-patch cell (400 wu) carries a LOADED arch roll — spawn forced, tier pinned
DOORWAY, five placement candidates instead of one. Everything downstream
(portal upgrade at density 1.0, commit, detection, colors, eviction,
re-entry determinism) is the existing pipeline, untouched. Pure function of
(seed, patch): streaming-order free, `?seed=`-pinned, no state, no panel row,
no GPU change — 0 mirrors / 0 bindings / 0 WGSL / 0 enrollment.

**What it guarantees, honestly.** Nearest-guaranteed-door ceiling ≈ one cell
diagonal (~400 wu) wherever the designated door PLACES. Two priced residuals:
(1) the 16-slot arch pool (`MAX_ARCH_INSTANCES` — a GPU-buffer dimension, not
a free dial) can still be full — expected standing demand rises from ~5 to ~7
against 16, overflow ≈ 0.1% per window-refresh — and that wall now SPEAKS
(U2's witness) instead of eating doors silently; (2) a designated door can
lose all five candidates deep in a shadow field (a few % of hot-theme cells) —
the neighbor cell then holds the ceiling at ~800 wu. Both are registered in
U5. The random rolls keep the texture; the lattice removes their veto over
absence.

Every unit is **pixel-affecting** (world structure); Jean gates build
(glaw1 + `cmake --build --preset the-board-web`) and visual once at the end.

---

## U1 — the lattice named

**Files**: `contracts/spawn_services.hpp`, `machine/spawn_engine.hpp`,
`bodies/grounded.hpp`.

**a — the constants + decl** (`spawn_services.hpp`, after
`GLOBAL_ENTITY_DENSITY`). FIND (expect 1):

```
inline constexpr float GLOBAL_ENTITY_DENSITY = 1.0f;
```

REPLACE:

```
inline constexpr float GLOBAL_ENTITY_DENSITY = 1.0f;

// ─── LODESTAR — the guaranteed-door lattice (LODESTAR_0) ─────────
// One patch per CELL×CELL cell of the open field carries a LOADED
// arch roll: spawn forced, tier pinned DOORWAY — a portal, since
// portal_density is 1.0. The random rolls keep the texture; this
// lattice removes their veto over ABSENCE. CELL is THE dial:
// 8 patches = 400 wu cells → nearest-guaranteed-door ceiling ≈ one
// cell diagonal (~400 wu). Smaller cells buy a lower ceiling with
// more doors and press the 16-slot arch pool — priced in OPEN.md
// (LODESTAR_0/P) before turning.
inline constexpr uint32_t LODESTAR_CELL      = 8u;    // patches per cell side
inline constexpr uint32_t LODESTAR_SEED_BAND = 190u;  // lattice band (themes 170, GoL zones 250)
inline constexpr uint32_t LODESTAR_TRIES     = 5u;    // designated placement candidates; ordinary rolls keep 1
bool arch_lodestar_designated(uint32_t world_seed, int32_t gx, int32_t gz);
```

**b — the decl rides the service block** (`spawn_services.hpp`). FIND (1):

```
bool check_position(MachineCtx* c, float px, float pz, float placing_radius,
    uint32_t placing_family);
```

REPLACE:

```
bool check_position(MachineCtx* c, float px, float pz, float placing_radius,
    uint32_t placing_family);
// LODESTAR_0 — the designation predicate (constants above; defined in
// spawn_engine.hpp beside the gate it loads).
```

(The standalone decl already landed in **a**; this comment is the block's
cross-reference. If the FIND mismatches, FLAG and skip — the decl in **a**
carries the campaign.)

**c — the definition** (`spawn_engine.hpp`, deriver country). FIND (1):

```
// Evaluate the spawn gate: seed + flat probability check.
```

REPLACE:

```
// LODESTAR_0 — THE DESIGNATED DOORWAY. One patch per LODESTAR_CELL²
// cell carries the loaded roll (see the constants' banner,
// spawn_services.hpp). Pure on (world seed, patch): streaming-order
// free, re-entry identical, ?seed=-pinned. The designated patch is
// hashed into the cell's CENTRAL 3×3 so the ~one-diagonal ceiling
// binds tight without the doors reading as a grid. Floor division by
// hand: C++ '/' truncates toward zero and the field is signed.
inline bool arch_lodestar_designated(uint32_t world_seed, int32_t gx, int32_t gz) {
    constexpr int32_t L = (int32_t)LODESTAR_CELL;
    int32_t qx = gx / L; if (gx % L < 0) qx -= 1;
    int32_t qz = gz / L; if (gz % L < 0) qz -= 1;
    uint32_t cell_seed = cpu_lattice_node_seed(world_seed, qx, qz, LODESTAR_SEED_BAND);
    constexpr int32_t core = (L - 3) / 2;   // central 3×3 origin (locals core..core+2)
    int32_t dx = core + (int32_t)(cpu_hash(cell_seed, 1u) % 3u);
    int32_t dz = core + (int32_t)(cpu_hash(cell_seed, 2u) % 3u);
    return (gx - qx * L) == dx && (gz - qz * L) == dz;
}

// Evaluate the spawn gate: seed + flat probability check.
```

**d — the retry props** (`grounded.hpp`, `struct ArchProp`). FIND (1):

```
    static constexpr uint32_t COLOR_SPREAD = 652u;  // MOSAIC_2: how far THIS body sits from its median
```

REPLACE:

```
    static constexpr uint32_t COLOR_SPREAD = 652u;  // MOSAIC_2: how far THIS body sits from its median
    static constexpr uint32_t LODESTAR_POS_X = 630u; // LODESTAR_0: retry-k position pair = (630+2(k-1), 631+2(k-1)), k=1..4; try 0 keeps POSITION_X/Z
```

Commit: `LODESTAR_0 U1 — the lattice named: one designated patch per cell`

---

## U2 — the loaded gate, and the pool wall speaks

**File**: `machine/spawn_engine.hpp` · symbols `run_spawn_preamble`,
`gate_from_traits`. Three edits.

**a — the signature grows a tail** (default keeps every other caller
bit-identical). FIND (1):

```
    uint32_t family)
```

REPLACE:

```
    uint32_t family,
    bool force_spawn = false)
```

**b — the dice lose their veto; the pool keeps its wall, out loud.**
The seed is derived BEFORE the roll (`evaluate_spawn_gate` computes it
unconditionally), so a door the dice would have built anyway is built
identically — force changes presence, never identity. FIND (1):

```
    if (!ctx.passed) return r;
```

REPLACE:

```
    if (!ctx.passed && !force_spawn) return r;   // LODESTAR_0: the designated door ignores the dice — never the pool (below)
```

Then FIND (1):

```
    if (slot == UINT32_MAX) return r;
```

REPLACE:

```
    if (slot == UINT32_MAX) {
        // LODESTAR_0 — THE POOL WALL, OUT LOUD (P6). MAX_ARCH_INSTANCES
        // is a GPU-buffer dimension, so the wall stands; a wall that can
        // eat a guarantee must not be silent. Arch-scoped: other
        // families' saturation may be their design.
        if (family == PopFamily::ARCH)
            std::cout << "[SPAWN] arch pool FULL — patch (" << gx << "," << gz
                      << ") dropped" << (force_spawn ? " (LODESTAR door lost)" : "") << "\n";
        return r;
    }
```

**c — the one seam where the lattice loads the dice** (`gate_from_traits`:
ARCH, open field, designated patch — every family flows through here, arch
alone is loaded). FIND (1):

```
    auto gate = run_spawn_preamble(c, gx, gz,
        active_arr, t.max_instances,
        t.spawn_roll_prop, t.spawn_chance,
        t.mood_multiplier, t.family_id);
```

REPLACE:

```
    // LODESTAR_0 — ARCH + open field + designated patch = the loaded
    // roll. Finite worlds are excluded whole: their doors are the
    // triad's, and a forced DOORWAY indoors opens nowhere.
    const bool lodestar = t.family_id == PopFamily::ARCH
        && !c->world_state_.finite_mode
        && arch_lodestar_designated(c->world_state_.active_seed, gx, gz);
    auto gate = run_spawn_preamble(c, gx, gz,
        active_arr, t.max_instances,
        t.spawn_roll_prop, t.spawn_chance,
        t.mood_multiplier, t.family_id,
        lodestar);
```

Commit: `LODESTAR_0 U2 — the loaded gate: the designated door ignores the dice, never the pool; the pool wall speaks`

---

## U3 — the designated arch is a DOORWAY

**File**: `machine/entity_pipeline.hpp` · symbol `generic_select`. FIND (1):

```
    uint32_t tier = select_tier(gate.seed, traits.tier_prop,
        weights, traits.tier_count);
    const auto& profile = adapter.get_tier_profile(tier);
```

REPLACE:

```
    uint32_t tier = select_tier(gate.seed, traits.tier_prop,
        weights, traits.tier_count);
    // LODESTAR_0 — the designated patch's arch IS the door: tier pinned
    // DOORWAY; the commit's portal upgrade (portal_density 1.0) does the
    // rest. Open field only — pinned indoors it would skew a room's arch
    // mix for a door that opens nowhere.
    if (traits.family_id == PopFamily::ARCH
        && !c->world_state_.finite_mode
        && arch_lodestar_designated(c->world_state_.active_seed, gx, gz))
        tier = static_cast<uint32_t>(ArchTier::DOORWAY);
    const auto& profile = adapter.get_tier_profile(tier);
```

Commit: `LODESTAR_0 U3 — the designated arch is a DOORWAY`

---

## U4 — the door bargains for ground

**File**: `machine/entity_pipeline.hpp` · symbol `generic_place`. The 60 wu
pyramid/antenna shadows eat up to ~half the ground in the hot themes; one
candidate was losing the guarantee exactly where the field is busiest. The
designated door alone gets LODESTAR_TRIES candidates — try 0 is bit-identical
to today's single draw (same props, same jitter); retries re-draw POSITION
only, at full-patch jitter, on the fresh 630-series pair. Ordinary rolls keep
one try: the texture is theirs. FIND (1):

```
    auto pos = negotiate_position(c, inst.seed,
        inst.trigger_gx, inst.trigger_gz,
        traits.pos_x_prop, traits.pos_z_prop,
        traits.position_jitter,
        traits.rotation_prop,
        traits.grounded,   // ruling 21: the ground-claim policy, from the family's own record
        inst.solid_half, /*containment_r*/ inst.solid_half, traits.family_id, inst.slot, inst.tier_idx);
    if (!pos.ok) return false;
```

REPLACE:

```
    // LODESTAR_0 — the designated door alone bargains for ground (see
    // the unit note in the campaign log). Try 0 is the standing single
    // draw, bit-identical; retries re-jitter position only.
    const bool lodestar = traits.family_id == PopFamily::ARCH
        && !c->world_state_.finite_mode
        && arch_lodestar_designated(c->world_state_.active_seed, inst.trigger_gx, inst.trigger_gz);
    const uint32_t tries = lodestar ? LODESTAR_TRIES : 1u;
    PositionResult pos{};
    for (uint32_t k = 0; k < tries; ++k) {
        const uint32_t px  = (k == 0u) ? traits.pos_x_prop : ArchProp::LODESTAR_POS_X + 2u * (k - 1u);
        const uint32_t pz  = (k == 0u) ? traits.pos_z_prop : ArchProp::LODESTAR_POS_X + 2u * (k - 1u) + 1u;
        const float    jit = (k == 0u) ? traits.position_jitter : 1.0f;
        pos = negotiate_position(c, inst.seed,
            inst.trigger_gx, inst.trigger_gz,
            px, pz,
            jit,
            traits.rotation_prop,
            traits.grounded,   // ruling 21: the ground-claim policy, from the family's own record
            inst.solid_half, /*containment_r*/ inst.solid_half, traits.family_id, inst.slot, inst.tier_idx);
        if (pos.ok) break;
    }
    if (!pos.ok) return false;
```

Commit: `LODESTAR_0 U4 — the door bargains for ground: five candidates against the shadows`

---

## U5 — the register written, the order retired

Append to `docs/OPEN.md` (one section, its lines die when the items close):

```
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
```

Then delete `docs/HANDOFFS/LODESTAR_0.md` — the directory exists only while
work is open; absence is health.

Commit: `LODESTAR_0 U5 — the register written, the order retired`

---

## Jean's gate — what the walk should say

- Boot an open world, walk any direction for several minutes: doorway arches
  now arrive roughly one per 23 patches (was ~1 per 36) — the extra ones ARE
  portals, in the usual destination colours; no stretch should exceed
  ~400 wu without one, hot themes included.
- The console stays quiet: `[SPAWN] arch pool FULL` should be absent or a
  rare single line per long session. If it repeats, that is the L=6 pricing
  item firing early — flag it, don't turn dials mid-gate.
- `?seed=` reloads reproduce the same doors in the same places; approaching
  and crossing a lodestar door behaves exactly like any rolled portal
  (it IS one — same commit path, same detection, same palette).
- Finite worlds and the atrium are untouched: triad + back only, no extra
  doorways indoors.
