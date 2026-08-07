# PORTAL_COLOR_1 — FIND THE CEMENT

**The colours a mood cut took with it, the residue that stayed, and the true
sandstone count.**

READING ONLY. No edits, no builds. Every claim carries file + line or a commit
SHA. Absences are stated as findings. Three of the work order's own premises did
not survive contact with the tree; they are reported in place, not worked around.

**METHOD.** `git fetch --unshallow` first — the clone arrived shallow and every
log claim below depends on the full history. Two notes on that unshallow, since
they bear on the record: it also corrected the local `master` ref, which pointed
at `cab1a0f` (an `INSTRUMENTS_1` tip) while `origin/master` was already at
`4fc6ffc`; and the pickaxe query the work order specifies does not find the
commit it was aimed at (F-1).

---

## 0 — FINDINGS

| # | Finding |
|---|---|
| **F-1** | `git log -S "PORTAL_COLORS"` **does not surface the commit that shrank the table.** `-S` counts occurrences of the string; the cut removed two rows and left the identifier count unchanged, so it is invisible. `-G` finds it. |
| **F-2** | **`MOOD_COUNT` was never 5.** `git log -S "MOOD_COUNT = 5"` returns nothing. The count went **6 → 4** in one commit. |
| **F-3** | **Exactly two triples were ever removed**, both in `9e4fe57`. The table was six rows with byte-identical values from the initial commit until that cut, and nothing since has touched it. |
| **F-4** | **`build_arch_mesh_params_from` does not exist.** The real symbol is `build_arch_mesh_params`. The work order's question is still answerable and the answer is unchanged. |
| **F-5** | The `portal_color[3]` parameter **is read**, at `grounded.hpp:767-769`. Not dropped. |
| **F-6** | The sandstone write at `grounded.hpp:724` is a **dead store** — unreachable as a colour for any body that function creates. |
| **F-7** | The sandstone triple has **five** homes, not the three the work order predicted — and the three its regex finds are not the three it names. |

---

## 1 — THE DELETED ROWS

### F-1 — the pickaxe misses its target

The work order specifies:

```
git log -S "PORTAL_COLORS" --oneline
git log -p -S "PORTAL_COLORS" -- '*mood*'
```

Neither lists `9e4fe57`, the commit that shrank the table. `-S` is the pickaxe:
it reports commits where the **number of occurrences** of the string changed.
`9e4fe57` deleted two rows from the table body and rewrote the extent
(`[6]` → `[MOOD_COUNT]`), but the identifier `PORTAL_COLORS` appears the same
number of times before and after. The commit is invisible to `-S` on both the
unfiltered and the `-- '*mood*'` form.

`-G` (regex against every changed line) finds it:

```
git log -G "PORTAL_COLORS" --oneline -- '*mood*'
9e4fe57 mood: six moods become four -- one open outdoor, one walled outdoor, two rooms
```

Recorded because the register's rule is to verify, and a query that returns a
plausible-looking list of twenty-three commits while omitting the one that
matters is the failure mode that rule exists for.

### The cut

**`9e4fe57`** · 2026-07-28 · *mood: six moods become four — one open outdoor, one
walled outdoor, two rooms* · `src/cartridges/the_board/direction/mood.hpp`

```diff
-inline constexpr float PORTAL_COLORS[6][3] = {
-    { 0.90f, 0.45f, 0.70f },  // mood 0  open_default    — pink
-    { 0.72f, 0.45f, 0.85f },  // mood 1  open_sunset     — lilac
-    { 0.95f, 0.55f, 0.15f },  // mood 2  indoor_flat     — orange
-    { 0.95f, 0.80f, 0.20f },  // mood 3  indoor_vault    — yellow
-    { 0.85f, 0.20f, 0.15f },  // mood 4  finite_outdoor  — red
-    { 0.70f, 0.15f, 0.12f },  // mood 5  finite_outdoor_ref — dark red
+inline constexpr float PORTAL_COLORS[MOOD_COUNT][3] = {
+    { 0.72f, 0.45f, 0.85f },  // mood 0  open_sunset     — lilac
+    { 0.95f, 0.55f, 0.15f },  // mood 1  indoor_flat     — orange
+    { 0.95f, 0.80f, 0.20f },  // mood 2  indoor_vault    — yellow
+    { 0.85f, 0.20f, 0.15f },  // mood 3  finite_outdoor  — red
 };
```

### The recovered triples, verbatim

Every removed row, with its comment as written:

```cpp
    { 0.90f, 0.45f, 0.70f },  // mood 0  open_default    — pink
    { 0.70f, 0.15f, 0.12f },  // mood 5  finite_outdoor_ref — dark red
```

| mood id (pre-cut) | name | triple | comment's name for it |
|---|---|---|---|
| 0 | `open_default` | `{ 0.90f, 0.45f, 0.70f }` | pink |
| 5 | `finite_outdoor_ref` | `{ 0.70f, 0.15f, 0.12f }` | dark red |

The four survivors kept their values byte-for-byte and renumbered down.

### F-3 — these are the only two, ever

Walking every commit that `-G "PORTAL_COLORS"` reports, from `d16d157` (initial
project commit, 2026-04-04) forward: the table is six rows with **identical
values** at every point in its life. It moves house repeatedly — `static
constexpr` inside the cartridge class, then out to `direction/mood.hpp` — and is
copied wholesale into `backup_board/` and the retired `the_chord/` sibling, but
no row is ever added, removed, or retuned until `9e4fe57`.

`git log -G "PORTAL_COLORS" 9e4fe57..HEAD` is **empty**. Nothing since the cut
has touched the table.

So the recovered set above is complete: two triples, one commit.

The work order's framing holds — `PORTAL_COLORS` is `[MOOD_COUNT][3]`, sized by
the constant, so no orphan row could have survived the cut. The colour is in the
history and nowhere else.

---

## 2 — THE MOOD CUT ITSELF

### F-2 — MOOD_COUNT was never 5

```
git log -S "MOOD_COUNT = 5"     (empty)
```

The work order's premise is a count that never existed. `MOOD_COUNT` went
**6 → 4** in a single commit — `9e4fe57`, the same commit that took the colours.
There was no intermediate five-mood world, so there is no third cut row to find.

Current home: `contracts/mood_constants.hpp:17` — `inline constexpr uint32_t
MOOD_COUNT = 4;`, pinned by a static_assert at `contracts/spine_state.hpp:214`.

### The moods that existed

| pre-cut id | name | fate | post-cut id |
|---|---|---|---|
| 0 | `open_default` | **CUT** | — |
| 1 | `open_sunset` | survives | 0 |
| 2 | `indoor_flat` | survives | 1 |
| 3 | `indoor_vault` | survives | 2 |
| 4 | `finite_outdoor` | survives | 3 |
| 5 | `finite_outdoor_ref` | **CUT** | — |

The two cut constants are `MOOD_OPEN_DEFAULT` and `MOOD_FINITE_OUTDOOR_REF` —
which is why `git log -S "open_default"` and `git log -S "finite_outdoor_ref"`
both list `9e4fe57` at their head while `-S "MOOD_COUNT = 5"` lists nothing.
Those two queries work because the identifiers genuinely vanished; the
`PORTAL_COLORS` query fails because that identifier did not.

### What `9e4fe57` removed alongside the colour

It is a renumbering, so every per-mood positional table moved together. From the
commit's own account, verified against the diff:

- `MOOD_TABLE`
- **`PORTAL_COLORS`** — the two triples above; the literal `6` became `MOOD_COUNT`
- `mood_name`'s `NAMES` — lost `"open_default"` and `"finite_outdoor_ref"`
- `ORB_MOOD_TABLE`
- `CUBE_POPULATIONS`
- `AGENT_POPULATIONS` (`bodies/agents.hpp`) — a seventh per-mood table the
  commit reports finding outside its handoff's named scope
- `MOOD_SPAWN_MULT` — the cut row was the only one carrying zeros; the table now
  rests at identity
- Key 9 is freed.

**The portal raffle absorbed the cut rather than losing their share.** Each
survivor inherited its cut sibling's weight exactly: `open_sunset` took
`open_default`'s 0.20 (→ 0.40), `finite_outdoor` took `finite_outdoor_ref`'s 0.15
(→ 0.30). Verified live at `mood.hpp` `pick_portal_mood`:

```cpp
if (roll < 0.40f) return 0;   // open_sunset    — the way out
if (roll < 0.55f) return 1;   // indoor_flat
if (roll < 0.70f) return 2;   // indoor_vault
return 3;                     // finite_outdoor
```

0.40 / 0.15 / 0.15 / 0.30 — sums to 1. The claim holds.

---

## 3 — LIVE RESIDUE

Every `PORTAL_COLOR|portal_color` hit in `src/cartridges/the_board/` and
`src/incubator_dual.cpp`, plus a full WGSL sweep.

### The array and its index

| site | what |
|---|---|
| `direction/mood.hpp:109` | `PORTAL_COLORS[MOOD_COUNT][3]` — the table |
| `direction/mood.hpp:881-882` | read at force-spawn: `is_back ? PORTAL_COLOR_BACK : PORTAL_COLORS[dest.mood % MOOD_COUNT]` |
| `machine/spawn_engine.hpp:427-428` | read at mesh-param rebuild, same fork |
| `cartridge.hpp:38`, `cartridge.hpp:255` | comments only — narration pointing at mood.hpp as the home |
| `direction/mood.hpp:44` | comment only |

### Outside the MOOD_COUNT array — the answer to the question asked

**One neutral triple, and it is deliberate:**

- **`direction/mood.hpp:115`** —
  `inline constexpr float PORTAL_COLOR_BACK[3] = { 0.35f, 0.55f, 0.90f };  // back-portal — blue`

  This is *not* an orphaned mood row. It is selected by `is_back_portal`, a flag,
  never by `destination.mood`, so it is outside the array by design and could not
  have been sized by `MOOD_COUNT`. Both readers (`mood.hpp:881`,
  `spawn_engine.hpp:427`) take it on the same branch.

**One sandstone triple in the portal path, and it is dead:**

- **`bodies/grounded.hpp:724`** — `aa.col_r = 0.75f;  aa.col_g = 0.68f;  aa.col_b = 0.60f;`
  inside `force_spawn_portal_arch`. Not a portal colour at all; see §4 Q2.

**One pass-through parameter:**

- `bodies/grounded.hpp:569` (declaration), `:662` (definition), `:678`
  (`(void)portal_color` on the ROSTER-disabled fork), `:767-769` (read).

### WGSL — nothing

`rg -ni "portal" src/ -g '*.wgsl'` returns 19 hits, **all of them geometry and
trigger logic**: `PortalEntry` / `PortalArray` at `world.wgsl:5949-5966`, the
ellipse-detection block at `:7000-7033`, `agent.portal_trigger`, and the bubble
commentary. `rg -n "PORTAL_COLOR|portal_color"` over `*.wgsl` returns **zero**.

Portal colour never reaches a shader as a portal colour. It arrives as
`GPUArchMeshParams.color_r/g/b`, uploaded from the CPU, indistinguishable at that
point from any other arch tint. There is no shader-side portal palette to drift.

---

## 4 — `force_spawn_portal_arch`

`bodies/grounded.hpp:659-775`. One caller: `force_spawn_portal_at`
(`direction/mood.hpp:884`). Declared at `grounded.hpp:566`.

### Q1 — is `const float portal_color[3]` read, or passed and dropped?

**READ.** `grounded.hpp:767-769`:

```cpp
    meshParams.color_r = portal_color[0];
    meshParams.color_g = portal_color[1];
    meshParams.color_b = portal_color[2];
```

It is the value actually uploaded for the slot at force-spawn time
(`upload_arch_mesh_params_slot`, `:771`). Not dropped, and not vestigial.

The single caller computes it as:

```cpp
    const float* pc = is_back_portal
        ? PORTAL_COLOR_BACK
        : PORTAL_COLORS[dest.mood % MOOD_COUNT];
```

— byte-for-byte the expression `build_arch_mesh_params` re-derives on every later
rebuild (`spawn_engine.hpp:426-428`). So the parameter is **redundant but
consistent**: the force-spawn upload and every subsequent rebuild agree on the
colour by two independent routes. That is a duplicated derivation, not a
divergence, and this audit does not touch it.

The one path that ignores it is the ROSTER-disabled fork at `:678`, which
`(void)`-casts every parameter and returns the no-free-slot sentinel — spawning
nothing at all.

### F-4 — `build_arch_mesh_params_from` does not exist

`rg -n "build_arch_mesh_params_from" src/` returns **nothing**. The tree has one
builder for this: **`build_arch_mesh_params(MachineCtx* c, uint32_t slot)`**,
defined `machine/spawn_engine.hpp:409`, declared `contracts/spawn_services.hpp:268`,
called from `spawn_engine.hpp:506`. Its portal fork is what the question is
about, so the question stands as asked with the name corrected.

The `_from` suffix is not invented, which is presumably how it travelled: the
tree has **`build_column_mesh_params_from(const ActiveColumn&)`** at
`spawn_engine.hpp:440`, sitting between the arch builder and
`build_column_mesh_params(MachineCtx*, uint32_t)` at `:475`. The column family
has both a slot-indexed builder and a by-reference one; the arch family has only
the slot-indexed builder. Worth knowing before anyone goes looking for an arch
twin that was never written.

### Q2 — is the sandstone write at `:724` reachable for any portal?

**No. It is a dead store, and this is stronger than "always overridden."**

`build_arch_mesh_params`, `spawn_engine.hpp:425-433`:

```cpp
    if (a.is_portal) {
        const float* pc = a.is_back_portal
            ? PORTAL_COLOR_BACK
            : PORTAL_COLORS[a.destination.mood % MOOD_COUNT];
        p.color_r = pc[0]; p.color_g = pc[1]; p.color_b = pc[2];
    }
    else {
        p.color_r = a.col_r; p.color_g = a.col_g; p.color_b = a.col_b;
    }
```

`ActiveArch::col_r/col_g/col_b` has **exactly one reader in the whole tree** —
line 432, the `else` arm. (`rg -n "col_r" src/cartridges/` returns four other
`col_r` sites: two are `ActiveColumn`/`ActivePyramid` member defaults, one is
`build_column_mesh_params` reading `ActiveColumn`, one is the generic write at
`entity_pipeline.hpp:1015`. None reads an arch's.)

`force_spawn_portal_arch` sets `aa.is_portal = true` at `:732`, unconditionally —
it is the portal channel, every body it authors is a portal. So every body it
creates takes the `if` arm forever, and the value written at `:724` is never
read.

**No window on slot reuse either.** The only way a slot's `is_portal` returns to
`false` is `arch_write_active` (`entity_pipeline.hpp:994`), the generic path, and
it writes the colour *first*:

- `:1015` — `aa.col_r = inst.colors[0]; ...`
- `:1023` — `aa.is_portal = false;`

The stale sandstone is overwritten eight lines before the flag that would make it
readable is cleared. There is no ordering at which `:724`'s value reaches
`p.color_*`.

`evict_arch` (`grounded.hpp:795`) only clears `.active` and uploads an empty
`GPUArchMeshParams`, so it opens no window either.

**Conclusion.** `:724` writes a colour that no code path can observe. It is not a
portal colour being overridden — it is a body colour written into a body that is
definitionally never coloured that way. Reported, not fixed.

---

## 5 — THE SANDSTONE CENSUS

### F-7 — the count is five, and the regex and the prediction disagree with each other

The work order predicts three sites — `ARCH_SANDSTONE_BASE`, `ActiveArch`'s
default member init, and the force-spawn body — and supplies:

```
rg -n "0\.75f, 0\.68f, 0\.60f|0\.75f;\s+.*0\.68f"
```

That regex returns three hits, but **not those three**. It finds
`ARCH_SANDSTONE_BASE`, `COLUMN_SANDSTONE_BASE` and the force-spawn body, and
misses both default member inits — they are written in comma form:

```cpp
    float col_r = 0.75f, col_g = 0.68f, col_b = 0.60f;
```

Neither alternative matches: there is no `{ ... }` list, and `0\.75f;` requires a
semicolon immediately after `0.75f`, where the source has a comma. Widening to
`rg -n "0\.75f|0\.68f|0\.60f"` and filtering for co-occurrence recovers them.

### The true count — 5

| # | site | form |
|---|---|---|
| 1 | `bodies/grounded.hpp:85` | `inline constexpr float ARCH_SANDSTONE_BASE[3] = { 0.75f, 0.68f, 0.60f };` — the named home |
| 2 | `bodies/grounded.hpp:141` | `ActiveArch` default member init — `float col_r = 0.75f, col_g = 0.68f, col_b = 0.60f;` |
| 3 | `bodies/grounded.hpp:179` | `inline constexpr float COLUMN_SANDSTONE_BASE[3] = { 0.75f, 0.68f, 0.60f };` — a **second named constant**, byte-identical |
| 4 | `bodies/grounded.hpp:284` | `ActiveColumn` default member init — same comma form |
| 5 | `bodies/grounded.hpp:724` | `force_spawn_portal_arch` body — the dead store of §4 Q2 |

All five in one file. The only reader of the *named* constant is
`entity_pipeline.hpp:983-985`, which tints it per-seed for the plain arch
population.

**Near-miss, for the record:** `grounded.hpp:499` is `ActivePyramid`'s default —
`float col_r = 0.80f, col_g = 0.72f, col_b = 0.58f;`. A different triple. It is
not sandstone and is not counted above.

### The comment

`bodies/grounded.hpp:72-73`, in the `ARCH_PALETTE` banner:

> `ARCH_SANDSTONE_BASE is deliberately NOT duplicated here — it has one home.`

Read narrowly — "not duplicated *here*", meaning the sandstone value is not
repeated as a row inside `ARCH_PALETTE` — the sentence is **true**, and that is
plainly what it was written to say.

Read as the claim its last clause makes — that the value has one home — it is
**false four times over**: a second named constant with the same three floats
(#3), two struct defaults that hardcode it rather than initialising from either
constant (#2, #4), and one function body that writes it as literals (#5).

The sharpest version of the finding: `ARCH_SANDSTONE_BASE` has one **definition**
and five **occurrences of its value**, and none of the other four reach the
constant by name — so editing the constant moves one of five sites, and the
remaining four would silently keep the old colour. #5 is inert (§4 Q2), but #2,
#3 and #4 are all live.

**Not fixed, per the work order.**

---

## 6 — WHAT WAS NOT DONE

- No source edits. No builds. No probes. This document is the only artifact.
- The three broken premises (F-1, F-2, F-4) are reported, not repaired in the
  register — correcting a work order is not this campaign's licence.
- The five-way sandstone duplication (F-7) and the dead store (F-6) are both
  left standing, as instructed.
- `backup_board/` and the retired `the_chord/` sibling carry their own frozen
  copies of the six-row table. They are noted in §1 for completeness and were
  otherwise excluded: they are archival text, not the live tree.
