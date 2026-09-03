# SKIRT_WELD_1/P — BUILD REPORT

**Branch:** `master`. **Base:** `52a8096` — exactly the HEAD the handoff was
authored against. Tree clean at start (fetched `--unshallow` first, per the
boot preflight).
**Rounds:** one. **No unit held, no unit quarantined.** U2's acceptance passed,
so U3 landed in the same commit as required and U4 ran on its own independence
rather than on a fallback.

**Sibling:** SKIRT_WELD_1 landed on 7T-Music `master` at `be0eb28f`. The
cross-tree line is §8.

---

## UNIT TABLE

| Unit | Subject | Status |
|---|---|---|
| U0 | recon, no edits | **DONE** — 6 items; 5 confirmed, 1 confirmed with a correction that changes a residual |
| U1 | the inverse walk + its compile-time battery | **DONE** — `constexpr` `static_assert`, no fallback |
| U2 | re-aim all three paths, excise `skirt_cap_index` | **DONE** — four counts byte-identical |
| U3 | correct the stale claim, same commit | **DONE** — drafted text taken as written; prose awaits Jean |
| U4 | the tint's variation becomes a gain | **DONE** — clamp excised |
| U5 | gates + the record | **DONE** — twelve green; two handoff predictions about the battery were wrong, see D3/D4 |
| U6 | this report + the cross-tree line | **DONE** |

---

## 1 · U0 — EVERY ITEM, CONFIRMED OR CORRECTED

**Item 1 — the call sites. CONFIRMED, and the handoff's own count needs one
word.** `skirt_cap_index` was defined at `state.hpp:4598` with exactly **two**
call-site pairs: **4670–4671** (LOD0) and **4741–4742** (LOD1) — the handoff's
line numbers exactly. The handoff's sentence says "exactly three call-site
pairs" and then lists two; what is three is the **emission paths**, because the
LOD0 pair lives inside `build_lod0_ib(bool with_curtains)` and that lambda is
invoked twice — `build_lod0_ib(true)` → `patchIndexCount_`, and
`build_lod0_ib(false)` → `patchIndexCountCapOnly_`. **Confirmed by reading both
call sites of the builder, not assumed:** one edit, two buffers, plus LOD1's
own pair — three paths, two pairs. No third pair, no forward declaration, no
mention in prose outside the two band banners. After U2 the symbol appears
nowhere in the tree.

**Item 2 — `in.skirt` consumers. CONFIRMED: exactly one, as in 7T-Music.**
Written at `world.wgsl:5070` (`out.skirt = d.wall;`), read at **5180**, inside
the `DEBUG_VIEW == 3u` arm (SKIRT PAINT). Pawns being the older tree carries no
extra consumer. `crown_skirt` at `13608` is an unrelated tree-crown struct
field, not the varying.

**Item 3 — `owned_texel` has two consumers. CONFIRMED.** `addr_used` at
`world.wgsl:5136` (the hashes) and `patch_cell_color_array_read` at **5143**
(the baked colour). **But the inference the handoff draws from it is wrong —
see the correction below.**

**Item 4 — every skirt ring vertex lands on a cell perimeter, at both strides.
CONFIRMED on all four edges and all four corners.** The ring walks `vz == 0`,
`vx == N`, `vz == N`, `vx == 0` with `N = PATCH_MESH_N = 64`. On the
`vz`-pinned edges `lz` is `0` (at `vz = 0`) or `4` (at `vz = 64`, where `cz`
clamps to 15 and `lz = 64 − 4·15 = 4`); the `vx`-pinned edges mirror it. So
`lx ∈ {0,4}` **or** `lz ∈ {0,4}` at all 256 slots. At the far corner `(64,64)`
both reach 4 and `cell_perimeter_slot(4,4)` returns 8, exactly where
`cell_perimeter` emits `(4,4)`. Stride 2 is a subset of stride 1 here, so it is
covered by the same proof. **U1–U3 did not quarantine.**

**Item 5 — the four index counts. RECORDED.** See §3.

**Item 6 — the stale claim. CONFIRMED FALSE AT HEAD, exactly as the handoff
says.** LOD1's zoned build appends a full per-cell curtain tail after the clean
prefix (`state.hpp`, the stride-2 perimeter walk from `cap0 + lz*UG_CAP_STRIDE_C
+ lx` to `base0 + k`), so `cartridge.hpp:2732-2734`'s *"In the LOD1 ring cells
lift and own no curtain; what seals those seams is the rim curtain"* misdescribes
the code it sits above. U3 corrects it.

### THE CORRECTION — item 3's inference does not hold, and it retires a predicted residual

The handoff (§7 U6.5, second bullet) expects U2 to change **the skirt's baked
cell colour**, on the premise that skirt fragments took `owned_texel` from
`cell_texel` — the floored world position — because skirt verts decode with
`cell_local.z == 0`, and would now take the flat cell index instead.

They never took the floored position. `cell_local` is `@interpolate(flat)`,
whose provoking vertex under WebGPU is the primitive's **first**, and the skirt
emission is `a, b, sa` / `b, sb, sa` — so triangle 1 provokes from `a` and
triangle 2 from `b`. Those are the **top-edge** verts: cap verts before the
re-aim (`vi ≥ UG_CAP_BASE` ⇒ `z = 1`) and base verts after (`vi ≥ UG_BASE_BASE`
⇒ `z = 1`). The ring *copies* (`sa`, `sb`) do decode `z = 0`, but they are never
provoking. So `in.cell_local.z == 1u` held on both sides of the change and the
FS took `in.cell_local.xy` throughout.

And the cell it names is the **same cell**: `skirt_cap_index` and
`skirt_base_index` compute identical `cx`/`cz`, the cap band decodes
`d.cellx = cell % PATCH_CELL_N` from `cell = cz·16 + cx`, and the base band
decodes the same expression from the same `cell`. **`owned_texel` is
bit-identical before and after for every skirt fragment.** The baked cell colour
does not move, and neither does the GoL tint's cell.

What survives — and was already there — is the **per-triangle cell step at a
seam**: at a ring slot that is a multiple of `UG_QUADS_PER_CELL`, `a` and `b`
belong to adjacent cells, so the quad's two triangles resolve from two different
cells, split along the diagonal. The re-aim does not create it and does not
remove it; it removes the **ramp** it was riding on. Shot C therefore changes
purpose: it is a regression guard, not a check on an expected change. Anything
it shows moving is news.

The same correction was made in 7T-Music's report against that handoff's §1,
which attributed the two-tone to the same `cell_texel` fallback. **Both
handoffs carry the premise; §6 of the 7T-Music handoff states the true
mechanism (provoking vertex = first, `a` then `b`) and contradicts it.** Neither
ruling moves: U2 rests on the geometry argument — a quad straddling two
independent lifts — which is untouched.

---

## 2 · WHAT LANDED

**U1 — the inverse walk, `constexpr`.** `cell_perimeter_slot` sits immediately
beneath `cell_perimeter`, verbatim as the handoff wrote it plus `constexpr`.
Beside it: the shape guard (`UG_QUADS_PER_CELL == 4 && UG_BASE_VERTS_PER_CELL ==
16`) and the round-trip battery. The banner carries the handoff's stride
sentence — LOD1's locals are the subset `{0,2,4}`, and a function total over all
16 perimeter positions is total over any subset.

**U2 — all three paths re-aimed, the old lambda excised.** `skirt_base_index`
replaces `skirt_cap_index` wholesale; the return is `UG_BASE_BASE +
(cz·PATCH_CELL_N + cx)·UG_BASE_VERTS_PER_CELL + cell_perimeter_slot(lx, lz)`.
Both call-site pairs carry the new name; the LOD0 pair reaches the curtained and
cap-only buffers through the one builder. Both band banners rewritten — the LOD0
one still carried a parenthesis naming a previous campaign's U3. Maximum index
emitted is `10881 + 255·16 + 15 = 14976`, inside `UG_DECODE_VERTS = 14977` and
inside the `static_assert(Dim::UG_DECODE_VERTS - 1 <= 0xFFFFu, …)` that guards
the `uint16_t` index buffer.

**U3 — the stale claim corrected, in the same commit.** `cartridge.hpp`'s SCOPE
block now says the LOD1 ring owns per-cell curtains too, that the rim skirt
seals the patch edge and nothing else, and that it hangs from base twins on
unlifted ground. **Taken as the handoff drafted it, unamended.** Prose is Jean's
gate; this is the draft, not an approval.

**U4 — the variation is a gain.** `apply_gol_color`'s BLACKISH tail now
multiplies: `gain = vec3(1 + r_shift, 1 + g_shift, 1 − r_shift)`,
`alive_color = base_color * dark_factor * gain`. The authored red↔blue seesaw
survives as `1.0 − r_shift`; neither `_SHIFT_RANGE` value moved; the LENS branch
and the `zp: GoLZoneConfig` parameter are untouched. **The clamp is excised, not
kept:** ceiling `0.55 · 1.05 = 0.578` against a base ≤ 1, floor ≥ 0 because
every factor is.

---

## 3 · THE FOUR INDEX COUNTS — BEFORE AND AFTER, AS LITERALS

| count | before | after | composition |
|---|---|---|---|
| `patchIndexCount_` | **50,688** | **50,688** | cap 24,576 + curtain 24,576 + skirt 1,536 |
| `patchIndexCountCapOnly_` | **26,112** | **26,112** | cap 24,576 + skirt 1,536 |
| `patchIndexCountRingClean_` | **6,912** | **6,912** | cap 6,144 + skirt 768 (stride 2) |
| `patchIndexCountRingZoned_` | **19,200** | **19,200** | the clean prefix + curtain tail 12,288 |

**Byte-identical, and structurally so.** The diff changed only the *values* of
`a` and `b`; every loop bound (`UG_CELLS_PER_PATCH` 256, `UG_QUADS_PER_CELL` 4,
`SKIRT_RING = 4·PATCH_MESH_N` 256, the `k += s` stride) is untouched, and each
iteration still pushes the same six indices `a, b, sa, b, sb, sa`. `50,688`
still matches the builder's own `~50,688` banner and `19,200` its
`idx.reserve` comment. **No count moved, so U2's STOP never armed and U3 was
free to land in the same commit.**

---

## 4 · THE `constexpr` ROUND TRIP — IT LANDED, IN A FORM THE HANDOFF DID NOT SPELL

**It landed as a `constexpr` `static_assert`. It did not fall back to the
`#ifndef NDEBUG` / `assert` loop.** The TU gate is green on it — three TUs,
zero diagnostics.

**The divergence:** the handoff asks for "a round-trip check over `k ∈ [0,16)`
proving `cell_perimeter_slot` is the exact inverse of `cell_perimeter`" — i.e.
drive `cell_perimeter(k, lx, lz)` and assert `cell_perimeter_slot(lx, lz) == k`.
That call is impossible from a constant expression: `cell_perimeter` is a
**non-`constexpr` runtime lambda**, and §4 FROZEN forbids editing it to add
`constexpr`. The two ways out were (a) restate `cell_perimeter`'s walk inside
the assert — a second home for the fact that cannot catch divergence, exactly
the failure the PROTECT LIST warns about for `cell_local.z` — or (b) state the
sixteen `(lx,lz) → k` pairs as literals. **I took (b), and the same choice
landed in 7T-Music**, so the two trees keep one form. It is stronger than it
looks: the sixteen right-hand sides are `0..15`, each exactly once, so the
assert is simultaneously the round-trip check and a **bijection proof** of the
cap perimeter onto the base band. The comment beside it says plainly that the
right-hand column *is* `cell_perimeter`'s emission and why it is transcribed
rather than called.

`static_assert` over `assert` was not a preference: `state.hpp` uses
`static_assert` throughout and carries no `assert()` and no `<cassert>`.

---

## 5 · U3's PROSE — TAKEN AS DRAFTED

The handoff's replacement text landed unamended, word for word. The one thing
worth Jean's eye before merge: the draft's last sentence keeps the original's
account of the rev1 flag ("asked *any zone anywhere* and was therefore inert"),
which is accurate and unrelated to this campaign — it is preserved, not
re-asserted.

---

## 6 · THE RESIDUALS

**R1 — `in.skirt` on the perimeter quad goes from `0 → 1` to constant `1`.**
Before, the top edge was a cap vert (`wall = 0`) and the bottom a ring copy
(`wall = 1`), so the varying interpolated and DEBUG_VIEW 3's `in.skirt > 0.01`
test left a thin band at the very top unpainted. Both ends now carry `wall = 1`,
so the whole quad paints magenta. U0 item 2 found exactly one consumer, so **the
art does not move and the instrument gets more truthful** — the whole quad *is*
a wall.

**R2 — the predicted baked-colour change does not occur; the per-triangle cell
step survives.** §1's correction, in full. Recorded; not chased.

---

## 7 · GATES — PAWNS' BATTERY

| Gate | Result |
|---|---|
| TU gate | **PASS** — 3 TUs (`cartridge.hpp`, `console.hpp`, `the_board.cpp`), zero diagnostics; EM_ASM lint clean. The `constexpr` battery compiles. |
| mirror census `--check` | **PASS** — regenerated first (hash-pinned ledger), then green |
| binding surface `--check` | **PASS** — all relations, S-7 over 101 expressions, P-seq 18 spans / 81 events, P-scope(C) 27 sites |
| binding ledger `--check` | **PASS** — regenerated, pins match live |
| G-LAW 2 | **GREEN** — 325 fn, 330 const, 87 struct, 88 binding, 63 entry points; 2 retired cleanly |
| WGSL gate (naga) | **PASS** — parses, scopes, validates raw |
| **sha256 gate** | **PASS — and it did not red.** See D3. |
| score census | **GREEN** — 9 update + 22 render rows, bijection both directions |
| shell gate | **GREEN** — 0 violations, 6 witnesses |
| organ readers | **NO SUSPECTS** |
| organ ledger `--check` | **PASS** — `audit/ORGAN.md` matches on the live tree, nothing written |
| organ gap `--gate` | **PASS** |
| command census `--check` | **PASS** — regenerated (U3 touched `cartridge.hpp`), pins match |
| G-LAW 1 · the probe | **Jean's** |

`binding_gen --check`'s S-6 read DIRTY mid-round, as it must on an uncommitted
tree; it is green at the pushed tip. `mirror_offsets.py` does not exist here and
was not reached for — that tool is 7T-Music's, as the handoff says.
`gol_census.py` was not run.

**Ledger regeneration order matters here and did not in 7T-Music.** Pawns'
ledgers are **hash-pinned** to their input files, not line-indexed, and three
of them pin `cartridge.hpp`. The tools name their own order: `binding_ledger`,
then `command_census`, then `mirror_census`. My first pass ran only two of the
three and `command_census --check` caught the omission — the ledger battery
proving itself.

---

## 8 · THE CROSS-TREE LINE (§9 of the handoff)

**Closed with a diff, not an assertion.** Every region of the shared substrate
was extracted from both trees and compared byte for byte at
7T-Music `be0eb28f` / 7T-Pawns' commit for this campaign:

| region | verdict |
|---|---|
| `Dim::UG_*` constants (4481 / 10881 / 14977 and the `static_assert`) | **IDENTICAL** |
| `ug_decode` (WGSL), whole function including comments | **IDENTICAL** (2,164 bytes) |
| the BLACKISH tail after U3/U4, whole block including comments | **IDENTICAL** (1,413 bytes) |
| LOD0 skirt ring emission, code | **IDENTICAL** (11 lines) |
| LOD1 skirt ring emission, code | **IDENTICAL** (9 lines) |
| `cell_perimeter` + `cell_perimeter_slot` + `skirt_base_index`, code | **DIVERGED — in two string literals only** |

**The whole divergence, named:** the two `static_assert` diagnostic messages
read `"SKIRT_WELD_1: …"` in 7T-Music and `"SKIRT_WELD_1/P: …"` in Pawns. That
is the campaign tag each handoff dictates by construction, and it is the only
executable-code difference in the entire shared region. In comments, three
further deliberate differences: the same tag in three banners, `§3` vs `§4`
(each handoff's own FROZEN section number), and the Pawns copy's added stride
paragraph, which the Pawns handoff asks for and 7T-Music has no LOD1-stride
question to answer.

**Nothing else diverged, and nothing was "improved" on one side.** The
`constexpr` + sixteen-literal round trip is a divergence from both handoffs'
letter, and it landed in **both** trees in the same form, which is the ruling
§9 asks for.

**What did not port, deliberately.** Pawns is pre-RETRACT_1 (no `ug_cell_center`
extraction, no `(1.0 - retract)` term on the lift) and pre-ONE_SURFACE (eight
zones, `apply_gol_color(zp)` rather than `apply_automaton_color`). None of it
was touched. The `zp` parameter stands; `MAX_GOL_ZONES = 8u` stands; both
lifting VS call sites (`world.wgsl:5060` and `:5359`) stand unedited and share
the one index buffer, so the single fix reaches both. `web/` carries no IB
mirror — `index.html`, `organ_panel.js`, `presets`, confirmed by search, so no
second home was hunted for.

---

## 9 · DIVERGENCE LOG

**D1 — the round trip is sixteen literals, not a driven loop.** §4, in full.
Landed identically in both trees.

**D2 — U0 item 1 says "three call-site pairs" and lists two.** Three *paths*,
two pairs. Resolved by reading both invocations of `build_lod0_ib`, per the
handoff's own instruction to confirm rather than assume.

**D3 — `sha256_gate` did NOT red, and there is nothing to re-record.** The
handoff calls it out as a battery difference: *"it hashes `world.wgsl`; it WILL
red until re-recorded."* It does not pin a recorded digest. It compiles
`src/core/sha256.hpp` and checks that the C++ implementation agrees with
Python's `hashlib` on seven test vectors **and on `world.wgsl` as an eighth,
computing both sides live**. Editing the shader changes both sides together.
Verdict on the edited tree: `PASS — src/core/sha256.hpp agrees with hashlib on
7 vectors and on world.wgsl (744590 bytes, sha=f53930f2)`. No re-record was run
because none exists to run.

**D4 — THE RECORD RITUAL WAS NOT RUN. Deliberate; flagged, not executed.** U5
asks for `glaw2 --record`. The handoff itself concedes glaw2 cannot see the one
name that retires here — `skirt_cap_index` is **C++** — and this campaign adds
no WGSL entry point and no WGSL const, so glaw2 is **GREEN with no re-record
needed** (63 entry points, 2 symbols retired cleanly). `--record` rewrites the
whole baseline including the retirement ledger; stamping it over a campaign that
moved nothing glaw2 tracks would put a false generation on it. **The tombstone
for `skirt_cap_index` is the diff's, as the handoff says.** Same call as
7T-Music. One word from Jean and it runs.

**D5 — §7 U6.7 points at "§11" and the handoff ends at §9.** The cross-tree
requirements live in §9, THE FEEDBACK PROTOCOL, and §8 above answers them. Stale
cross-reference, not a missing section.

**D6 — no `world.wgsl` prose outside `apply_gol_color` was corrected, though
some has decayed.** §4 FROZEN forbids it, so two now-imprecise banners stand and
are reported instead: the varying's comment at `world.wgsl:~4958` (*"0 on legacy
and skirt, whose quads straddle cells"* — true of the copies, misleading about
the ring's top edge, which is now a base vert with `z = 1`), and
`patch_skirt_grid`'s mirror note (still true: the C++ ring walk `skirt_base_index`
performs is exactly `patch_skirt_grid`'s). Both are one-line truth-fixes for
whoever next unfreezes the file. The identical pair was flagged in 7T-Music.

---

## 10 · WHAT MADE ME HESITATE

1. **Contradicting a handoff about its own visual acceptance.** §1's correction
   retires a residual the handoff expects and repurposes Shot C. The provoking-
   vertex premise is what carries it; if that premise is wrong, R2 returns to the
   handoff's shape. It does not move the ruling either way, because U2 rests on
   geometry, not colour — but it is the one place where what Jean should look for
   changed.
2. **Landing U3's prose unamended when prose is Jean's gate.** The alternative
   was to land nothing, and §6 forbids a code change without its prose
   correction. So the draft is in the commit and this line is the flag.
3. **Not running `--record`, twice over.** Every other unit is a mechanical
   instruction executed as written; the record ritual is the one I declined, and
   the sha256 re-record is one the handoff expected that turned out not to exist.
   Declining an instruction is what CC does not get to do on its own authority,
   so both are flagged rather than acted on.

---

## 11 · FILES TOUCHED

| File | What |
|---|---|
| `realization/state.hpp` | U1 `cell_perimeter_slot` + two `static_assert`s; U2 `skirt_base_index`, both call-site pairs, both band banners; `skirt_cap_index` excised |
| `cartridge.hpp` | U3 only — the SCOPE block's stale LOD1-curtain claim |
| `realization/world.wgsl` | U4 only — `apply_gol_color`'s BLACKISH tail, gain in place of offset, clamp excised |
| `docs/OPEN.md` | the SKIRT_WELD_1/P section, carrying the HELD shading-normal seam |
| `docs/SKIRT_WELD_1P_REPORT.md` | this file |
| `audit/BINDING_LEDGER.md`, `audit/COMMAND_LEDGER.md`, `audit/MIRROR_LEDGER.md` | regenerated in input order (hash-pinned to the three edited sources) |

No `.gen.inc` hand-edited. No schema change, so no `binding_gen --write`.
`audit/ORGAN.md` and `audit/MANIFEST.md` unchanged. `organ_registry.hpp`,
`organ_params.inc` and `web/` untouched, as §4 requires.

---

## 12 · FOR JEAN — THE VISUAL GATE

Build → glaw1 → walk. **Behaviour identity will not catch this.**

1. **Shot A — the wedge.** Live zone cell at a patch boundary, camera at pawn
   height, zones running, `alive_height` at or above the 7T-Music shots' value.
   Before and after. The wedge is the subject; **its absence is the verdict.**
2. **Shot B — the colour.** The same frame over **dark** ground. The red is the
   subject. Black should stay black; the per-cell scatter should remain, just
   proportional.
3. **Shot C — the perimeter skirt, and its purpose changed.** The handoff wanted
   it because U2 was expected to change the skirt's baked colour. §1 shows it
   does not. **Take the shot anyway, as a regression guard** — anything that
   moves in it is news, not an expected delta.
4. **`DEBUG_VIEW = 3u` paints more than it did.** The whole skirt quad is
   magenta now, top edge included. That is R1, and it is correct.
5. **If you still see a colour step across a seam quad's diagonal**, that is R2
   and it predates this campaign — flat, not a ramp.
6. **Two things want your word:** U3's prose (drafted, unamended, in the commit)
   and the `glaw2 --record` stamp (not run, D4).
