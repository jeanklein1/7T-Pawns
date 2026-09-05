# GOL_GRID_0 — the giants claim two hundred

**Repo**: `jeanklein1/7T-Pawns`, master. **Trunk rule**: CC commits directly to
master; no held branch. One commit per unit, messages word-for-word.
FLAG-AND-FINISH within a unit; STOP only on FIND mismatch, scoped to that
unit. Cite symbols, not line numbers. Place this file at
`docs/HANDOFFS/GOL_GRID_0.md` in the U1 commit; U3 retires it.

**Preflight**: unshallow if `git rev-parse --is-shallow-repository` says so.
USHER_0 and LODESTAR_0 may land in any order around this campaign — their
files are disjoint from every FIND here except `docs/OPEN.md`, which all
three only APPEND to; blob drift on OPEN.md is expected and not a STOP.

**Anchors (blob SHAs at authoring, HEAD `cf0d71e`):**

| file | blob |
| --- | --- |
| `src/cartridges/the_board/bodies/gol_zones.hpp` | `c24eb0ca` |
| `src/cartridges/the_board/realization/state.hpp` | `e93d24be` |
| `src/cartridges/the_board/realization/world.wgsl` | `fadb3b6e` |
| `docs/OPEN.md` | `30963803` |

**Jean's ruling.** The biggest GoL zones go to **200 wu** (64 cells at
PATCH_CELL_SIZE 3.125). Mechanically that is two moves: the CAPACITY
(`Dim::GOL_ZONE_GRID` and its WGSL twins — a hardware mirror pair) doubles
32 → 64, and every tier row currently AT the old ceiling rides to the new
one — Conway **Sparse, Moderate, Plateau, HighLife** and Pulse **Breathe,
Spiral**, in BOTH twin tables. Rows at 8/16/24 do not move. Everything
else is already capacity-shaped: the kernels early-out on `zp.grid_size`,
`ZONE_GRID_WG` derives from the Dim constant, `upload_zone_life` and the
buffer/texture creation are symbolic, and the separation law
(√2·side + 60 → 343 wu between giants) thins the field on its own. No new
bindings, no lane change; the life buffer grows 160 KB → 640 KB and the
texture to 64×64×8 — orders of magnitude under every WebGPU wall.

U1 is **pixel-identical in intent** (capacity only; no tier uses it yet —
zones render as before, allocations larger). U2 is **pixel-affecting**.
Jean gates build (glaw1 + web preset) and visual once at the end;
`mirror_census` is Jean's gate and should come back green — the pairs move
together inside single commits.

---

## U1 — the capacity doubles (both sides of the mirror, one commit)

**a — the Dim trio** (`realization/state.hpp`). FIND (expect 1):

```
            // CAPACITY — the MAXIMUM cells per zone side, and the side of the
            // life-buffer plane and the life texture. A zone's ACTUAL side is
            // its tier's grid_cells ∈ {8,16,24,32} (bodies/gol_zones.hpp), and
            // every index and bound is derived from THAT. Never mix the two.
            constexpr uint32_t GOL_ZONE_GRID = 32;
            constexpr uint32_t GOL_ZONE_CELLS = GOL_ZONE_GRID * GOL_ZONE_GRID;  // 1024
            constexpr uint32_t GOL_ZONE_LIFE_STRIDE = GOL_ZONE_CELLS * 5;  // 5 slots: visual, velocity, target, next, height_factor
```

REPLACE:

```
            // CAPACITY — the MAXIMUM cells per zone side, and the side of the
            // life-buffer plane and the life texture. A zone's ACTUAL side is
            // its tier's grid_cells ∈ {8..64} (bodies/gol_zones.hpp), and
            // every index and bound is derived from THAT. Never mix the two.
            // GOL_GRID_0 doubled 32 → 64 (biggest zone 100 → 200 wu); the
            // WGSL twins (GOL_ZONE_TEX_N, GOL_ZONE_STRIDE, the GOL_CELL_*
            // slot offsets) moved in the same commit.
            constexpr uint32_t GOL_ZONE_GRID = 64;
            constexpr uint32_t GOL_ZONE_CELLS = GOL_ZONE_GRID * GOL_ZONE_GRID;  // 4096
            constexpr uint32_t GOL_ZONE_LIFE_STRIDE = GOL_ZONE_CELLS * 5;  // 5 slots: visual, velocity, target, next, height_factor
```

**b — the buffer's marginal comment** (`state.hpp`). FIND (1):

```
            wgpu::Buffer zoneLifeBuffer_;          // life state: MAX_ZONES × GOL_ZONE_LIFE_STRIDE (5120) floats
```

REPLACE:

```
            wgpu::Buffer zoneLifeBuffer_;          // life state: MAX_ZONES × GOL_ZONE_LIFE_STRIDE (20480) floats
```

**c — the texture's marginal comment** (`state.hpp`). FIND (1):

```
                // Zone life texture: 32×32 × MAX_ZONES, R32Float (R = the cell's spring visual)
```

REPLACE:

```
                // Zone life texture: 64×64 × MAX_ZONES, R32Float (R = the cell's spring visual)
```

**d — the WGSL twins** (`realization/world.wgsl`). FIND (1):

```
// The zone life texture's side — twin of Dim::GOL_ZONE_GRID
// (state.hpp). FIXED at 32 while zp.grid_size is tier-derived over
// {8..32}: the sim writes texels [0, grid_size)² of a 32² layer, so
// every fetch normalizes by THIS, never by the zone's own grid.
const GOL_ZONE_TEX_N: f32 = 32.0;
const GOL_ZONE_STRIDE: u32 = 5120u;     // floats per zone (5 slots × 1024 cells)
const GOL_CELL_VISUAL: u32 = 0u;        // slot 0: height spring visual [0,1]
const GOL_CELL_VELOCITY: u32 = 1024u;   // slot 1: height spring velocity
const GOL_CELL_TARGET: u32 = 2048u;     // slot 2: current target (binary, Conway reads)
const GOL_CELL_NEXT: u32 = 3072u;       // slot 3: next target (binary, Conway writes)
const GOL_CELL_HEIGHT_FACTOR: u32 = 4096u;  // slot 4: per-cell height multiplier (persistent)
```

REPLACE:

```
// The zone life texture's side — twin of Dim::GOL_ZONE_GRID
// (state.hpp). FIXED at 64 while zp.grid_size is tier-derived over
// {8..64}: the sim writes texels [0, grid_size)² of a 64² layer, so
// every fetch normalizes by THIS, never by the zone's own grid.
// GOL_GRID_0 doubled the capacity; the CPU trio moved in this commit.
const GOL_ZONE_TEX_N: f32 = 64.0;
const GOL_ZONE_STRIDE: u32 = 20480u;    // floats per zone (5 slots × 4096 cells)
const GOL_CELL_VISUAL: u32 = 0u;        // slot 0: height spring visual [0,1]
const GOL_CELL_VELOCITY: u32 = 4096u;   // slot 1: height spring velocity
const GOL_CELL_TARGET: u32 = 8192u;     // slot 2: current target (binary, Conway reads)
const GOL_CELL_NEXT: u32 = 12288u;      // slot 3: next target (binary, Conway writes)
const GOL_CELL_HEIGHT_FACTOR: u32 = 16384u; // slot 4: per-cell height multiplier (persistent)
```

**e — the struct comment keeps the law honest** (`bodies/gol_zones.hpp`).
FIND (1):

```
    uint32_t grid_cells;       // zone side in cells ∈ {8..32}
```

REPLACE:

```
    uint32_t grid_cells;       // zone side in cells ∈ {8..64} (capacity: Dim::GOL_ZONE_GRID)
```

Commit: `GOL_GRID_0 U1 — the capacity doubles: 64-cell planes, both sides of the mirror`

---

## U2 — the giants claim it (both twin tables, one commit)

Six rows ride the ceiling up — the rows AT 32, nothing else. The tables'
own law: "both are authoritative and a tuner must edit both."

**a — CPU Conway rows** (`bodies/gol_zones.hpp`, `GOL_TIERS`). Four FINDs
(expect 1 each); in each REPLACE only the final `32u` becomes `64u`:

```
    /* 1: Sparse   */ { 0x1808u,  0.15f, 0.05f,   4.0f, 1.0f,   0.12f, 0.03f,  18.0f, 6.0f,  0.20f,  0.17f, false, 32u },
```
```
    /* 2: Moderate */ { 0x1808u,  0.30f, 0.08f,   2.0f, 0.6f,   0.15f, 0.03f,   9.0f, 3.0f,  0.15f,  0.09f, false, 32u },
```
```
    /* 7: Plateau  */ { 0x3E1E0u, 0.50f, 0.06f,   8.0f, 2.0f,   0.10f, 0.02f,  30.0f, 8.0f,  0.08f,  0.09f, false, 32u },
```
```
    /* 9: HighLife */ { 0x1848u,  0.30f, 0.05f,   1.0f,  0.3f,   0.20f, 0.04f,  10.0f, 3.0f,  0.22f,  0.07f, false, 32u },
```

**b — CPU Pulse rows** (`GOL_PULSE_TIERS`). Two FINDs (1 each); final
`32u` → `64u`:

```
    /* 0: Breathe  */ { PulseField::BREATH,  4.0f, 1.0f,   0.20f, 0.05f,   0.15f, 0.05f,    0.0f,  0.0f,   2.0f, 0.8f,  10.0f, 3.0f,   0.20f,  0.38f, false, BoundaryMode::REFLECT, 32u },
```
```
    /* 3: Spiral   */ { PulseField::SPIRAL,  6.0f, 1.6f,   0.30f, 0.06f,   0.03f, 0.01f,    0.0f, 0.0f,   0.0f, 0.0f,   0.0f, 0.0f,   0.10f,  0.18f, true,  BoundaryMode::WRAP, 32u },
```

**c — WGSL Conway rows** (`world.wgsl`, `GOL_TIERS`). Four FINDs (1 each);
final `32u` → `64u`:

```
    /* 1: SPARSE   */ GoLTierParams(0x1808u,  0.15, 0.05,   4.0, 1.0,   0.12, 0.03,  18.0, 6.0,  0.20,  0.17, 0u, 32u),
```
```
    /* 2: MODERATE */ GoLTierParams(0x1808u,  0.30, 0.08,   2.0, 0.6,   0.15, 0.03,   9.0, 3.0,  0.15,  0.09, 0u, 32u),
```
```
    /* 7: PLATEAU  */ GoLTierParams(0x3E1E0u, 0.50, 0.06,   8.0, 2.0,   0.10, 0.02,  30.0, 8.0,  0.08,  0.09, 0u, 32u),
```
```
    /* 9: HIGHLIFE */ GoLTierParams(0x1848u,  0.30, 0.05,   1.0,  0.3,   0.20, 0.04,  10.0, 3.0,  0.22,  0.07, 0u, 32u),
```

**d — WGSL Pulse rows** (`GOL_PULSE_TIERS`). Two FINDs (1 each); final
`32u` → `64u`:

```
    /* 0: Breathe  */ GolPulseTierParams( PULSE_FIELD_BREATH,  4.0, 1.0,   0.20, 0.05,   0.15, 0.05,    0.0,  0.0,   2.0, 0.8,  10.0, 3.0,   0.20,  0.38, 0u, 0u, 32u ),
```
```
    /* 3: Spiral   */ GolPulseTierParams( PULSE_FIELD_SPIRAL,  6.0, 1.6,   0.30, 0.06,   0.03, 0.01,    0.0, 0.0,   0.0, 0.0,   0.0, 0.0,   0.10,  0.18, 1u, 1u, 32u ),
```

**e — the census witnesses the promoted rules at their new size.** The tool
parses the edited table live:

```
python tools/gol_census.py --row Sparse --row Moderate --row Plateau --row HighLife
```

(If `--row` wants different spelling, adapt from `--help`; FLAG, don't
STOP.) Paste the dark / saturated / structured / fixed-point rates into the
commit body. The one recorded band: Plateau was 0/32 dark, 0/32 saturated,
32/32 structured at cells=32 (dark 0.4% over 512 seeds) — FLAG if the
64-cell run leaves that neighbourhood; report the other three rows'
numbers as new baselines. Pulse rows are field equations, not automata —
no census.

Commit: `GOL_GRID_0 U2 — six rows ride to 64: the biggest zone is 200 wu`
(census numbers in the body)

---

## U3 — register + retire

Append to `docs/OPEN.md`:

```
## GOL_GRID_0/P — parked residue

- a 64-cell zone shadows arch placement over ~150 wu of radius (Arch–GoL
  separation 0, footprint radii add): a LODESTAR cell whose core falls
  under one loses its designated door to the neighbor cell (~800 wu
  ceiling) · origin: GOL_GRID_0 · unblocks: Jean's walk gate — levers are
  the promoted rows' cells column, or a zone-aware lodestar designation
  (its own campaign).
```

Delete `docs/HANDOFFS/GOL_GRID_0.md`.

Commit: `GOL_GRID_0 U3 — the register written, the order retired`

---

## Jean's gate — what the walk should say

- Giants exist: 200 wu automata for Sparse / Moderate / Plateau / HighLife
  and 200 wu Breathe / Spiral fields — rarer than before and moated (the
  separation law holds giants ≥ ~343 wu apart center-to-center); 8/16/24
  tiers unchanged.
- Addressing is clean: a wrong stride reads as zones wearing each other's
  life — scrambled, flickering cells. Any such corruption is a STOP-grade
  report against U1, not a tuning note.
- Boot witnesses quiet: minBindingSize and the layout asserts recompute
  from the same expressions; `mirror_census` green (the pairs moved
  together).
- The census numbers in U2's body read sane — Plateau still structures and
  still terminates; nothing went dark.
- Near a giant, expect the moat: fewer arches and no portals across its
  disc — the registered LODESTAR interaction, priced, on the walk gate's
  list rather than a defect.
