# RAZOR_0 — THE BACKGROUND DRAWS LAST

*Handoff · authored 2026-09-08 · Claude → CC · Jean holds build, visual, merge, deploy.*
*Filed to `docs/HANDOFFS/RAZOR_0.md` while open; the directory dies with the campaign.*

## THE IDEA, IN ONE PARAGRAPH

The main pass draws the terrain first — three indirect draws that cover most of the screen
— and then the table (entities, the ribbon, the pawn), the paintings and the frames on top.
Every terrain fragment that one of those will cover is shaded in full (sixteen shadow taps,
the lights, the fog, the icing) and then overwritten. Every draw in the list is opaque and
depth-tested, so the order is immaterial to the picture; it is material to the cost. Moving
the terrain plan to the end of the opaques lets those fragments fail the depth test before
the fragment shader runs. Nothing is removed, nothing moves in the world, no pixel changes.
The gain is the covered fraction of the terrain's shading — largest from the ribbon and in
galleries, where the eye sees the most things standing on the ground.

## AUTHORITY

Base: `master` at HEAD `e2cf155c` (PLATE_0 U4d), fetched 2026-09-08. Rides a **held branch
`claude/razor-0`** (P12: the frame's draw order is runtime behaviour a visual gate can catch
— the gate here is *no change*). Disjoint from MIP_0's files; the two can land in either
order, and the ledger cascade runs once after the last.

| file | blob at base |
| --- | --- |
| `src/cartridges/the_board/realization/render_passes.hpp` | `9c8c013c8bade5cab1460070ac137ba7ce3c5d6b` |

**Pinned:** `render_passes.hpp` (BINDING + COMMAND). Red from U1 until U2 regenerates.

**HALT classes (P17), scoped to the unit (P15):** unreachable file; stale authority (blob
differs AND a FIND misses). Everything else DEFAULT-AND-FLAG. Symbols, not ranges (P2).
Expected match count **1** everywhere. LF only, no BOM.

**REHEARSED.** Both FINDs at count 1 on the base; G-LAW 1 GREEN and the TU gate at zero
diagnostics; `binding_gen.py --check` P-seq PASS — after it FAILED once, which is the
finding below; every ledger and gate green after U2.

## THE RULINGS THIS CAMPAIGN LANDS

1. **Order among opaques is immaterial to the picture and material to the cost.** The
   background — the largest surface — draws last among the opaques; the additive orbs and
   the fade stay where they were, last and in order. One home: `encode_main_opaque` feeds
   both the recorded bundle and the direct arm, so the order cannot drift between them.
2. **A bind the plan used to lend is stated.** The terrain plan bound the scene state at
   group 2, and the table's first draw had been inheriting it. Moved, the plan no longer
   precedes the table, and `binding_gen`'s P-seq simulation caught the inheritance at the
   rehearsal: `DrawIndexedIndirect under archPipeline_ wants (sceneStateLayout_,
   sceneTexturesLayout_) but the last-bound 2/3 pair is (-, sceneTexturesLayout_)`. The
   bind is now written where it is needed. The gate did its job; the ruling records it.

## WHAT WAS EXAMINED AND FOUND ALREADY TIGHT (so nobody re-derives it)

The survey asked one question of every per-frame cost: does this work change a pixel? These
do not need touching — the tree already answers:

- **The sun's sixteen taps are skipped where they cannot show.** `calc_directional_light`
  samples the shadow map only when `ndotl > 0` and `spots.count == 0`; point lights loop
  over `count`, not the cap.
- **The two shadow passes are an either/or.** `render_shadow_pass` renders the spot atlas
  when spots are active and the sun map otherwise — never both — and indoors the sun map
  is the atlas's first tile.
- **The per-frame compute rows are dirty-gated.** Placement correction, mesh generation,
  the aura (`aura_presence > 0 || aura_needs_clear`), the zone derive flush; the readback
  is 32 agents.
- **A fog early-out cannot fire.** The ring is the draw authority at 6.84 × 50 = 342 wu;
  at the densest mood (0.0168) fog reaches 0.9968 at the ring, under any threshold that is
  pixel-identical at 8 bits (≥ 0.9995). Three lines that never run are not a razor.
- **The dial for the sun's kernel already exists.** `config.shadow_pcf_taps` has a live
  setter and a 2×2 kernel beside the 4×4; it is an ORGAN dial (`?organ=1`). It is a quality
  trade, so it is not in these handoffs — but it needs no code to try.

Two things remained: work done on pixels that are then covered (RAZOR_0), and texels
fetched from a 1024² array with no chain (MIP_0).

## UNITS, IN ORDER

| unit | what | files | commit |
| --- | --- | --- | --- |
| U0 | preflight | — | none |
| U1 | the terrain plan moves to the end of the opaques; the stated bind | `render_passes.hpp` | 1 |
| U2 | ledgers; OPEN.md; push | `audit/*`, `docs/OPEN.md` | 1 |

---

## U0 — PREFLIGHT (no commit)

```
git fetch origin master && git checkout -b claude/razor-0 origin/master
git ls-tree HEAD src/cartridges/the_board/realization/render_passes.hpp        # 9c8c013c… expected
grep -c "begin_patch_terrain_plan(pass)" src/cartridges/the_board/realization/render_passes.hpp   # 1
sh tools/gates/glaw1/run.sh && python3 tools/gates/console_gate/run.py && python3 tools/binding_gen.py --check   # green before anything moves (S-6 aside)
```

---

## U1 — THE BACKGROUND DRAWS LAST

### R1.1 — the terrain leaves the head of the opaque list (`src/cartridges/the_board/realization/render_passes.hpp`)

FIND
```
    const uint32_t dmask = c->gpuState_.config().draw_mask;

    c->renderer_.begin_patch_terrain_plan(pass);   // OIL_1 U13: one SetPipeline for the three slots
    // DOMESDAY_0 B3: the per-slot list window rides the vertex-buffer
    // offset now (FC_SEG_A/B/C — the same segments the retired g2:62
    // bind windows carved), delivered to the VS as @location(0).
    if (dmask & DrawBit::TERRAIN_A)
    c->renderer_.draw_patch_terrain_plan_slot(pass,
        c->gpuState_.scene_state_group(),
        c->gpuState_.visible_patch_indices_buffer(), FC_SEG_A_OFF, FC_SEG_A_BYTES,   // plan A window
        c->gpuState_.patch_index_buffer(),           // full IB (zone-overlapped)
        c->gpuState_.frustum_indirect_lod0(), 0);
    if (dmask & DrawBit::TERRAIN_B)
    c->renderer_.draw_patch_terrain_plan_slot(pass,
        c->gpuState_.scene_state_group(),            // B5 (R2): the one scene group
        c->gpuState_.visible_patch_indices_buffer(), FC_SEG_B_OFF, FC_SEG_B_BYTES,   // plan B window
        c->gpuState_.patch_index_buffer_cap_only(),  // cap-only IB (clean LOD0)
        c->gpuState_.frustum_indirect_lod0(), 20);
    if (dmask & DrawBit::TERRAIN_C)
    c->renderer_.draw_patch_terrain_plan_slot(pass,
        c->gpuState_.scene_state_group(),            // B5 (R2): the one scene group
        c->gpuState_.visible_patch_indices_buffer(), FC_SEG_C_OFF, FC_SEG_C_BYTES,   // plan C window
        c->gpuState_.patch_index_buffer_lod1(),      // LOD1 IB (culled at last)
        c->gpuState_.frustum_indirect_lod0(), 40);
    // DOMESDAY_1 B5 (R2): the PlanB/PlanC windows collapsed into the
    // one scene group, so plan C left the RIGHT group bound — the old
    // restore is retired with the groups it restored from.

    // The drawable table — main members, canonical order. All opaque and
    // depth-tested, so order among them is immaterial; this is where the
    // ribbon's ordinal drift dies (it now draws with the entities, not late).
```
REPLACE
```
    const uint32_t dmask = c->gpuState_.config().draw_mask;
    // RAZOR_0 — THE BACKGROUND DRAWS LAST. The terrain plan used to lead
    // this list; it now follows the table and the gallery fork (below).
    // Order among opaques is immaterial to the PICTURE — every draw is
    // depth-tested — and material to the COST: the terrain is the largest
    // surface on screen, and every terrain fragment that an entity, a
    // wall, a painting or the pawn will cover was shaded first (sixteen
    // shadow taps, the lights, the fog) and then overwritten. Drawn after
    // the occluders, those fragments fail the depth test before the
    // fragment shader runs. Pixel-identical by construction; the meter's
    // main_pass row is the witness.
    //
    // The plan used to bind the scene state at group 2 for everything that
    // followed it; now that it follows, the table's first draw needs that
    // bind stated here rather than inherited. (binding_gen's P-seq
    // simulation caught the inheritance at the rehearsal — the gate's job.)
    pass.SetBindGroup(2, c->gpuState_.scene_state_group());
    // The drawable table — main members, canonical order. All opaque and
    // depth-tested, so order among them is immaterial to the picture; this
    // is where the ribbon's ordinal drift dies (it now draws with the
    // entities, not late).
```

### R1.2 — the terrain plan, verbatim, after the scene pair is restored (`src/cartridges/the_board/realization/render_passes.hpp`)

FIND
```
    if constexpr (ROSTER.gallery) {
    pass.SetBindGroup(2, c->gpuState_.scene_state_group());
    pass.SetBindGroup(3, c->gpuState_.scene_textures_group());
    }
    if (dmask & DrawBit::ORBS)
        render_orbs(orbs_state_, &orbs_deps_, pass);
```
REPLACE
```
    if constexpr (ROSTER.gallery) {
    pass.SetBindGroup(2, c->gpuState_.scene_state_group());
    pass.SetBindGroup(3, c->gpuState_.scene_textures_group());
    }
    // RAZOR_0 — THE TERRAIN PLAN, moved here from the head of the list
    // (see the ruling at the top). The three draws and their windows are
    // untouched; only their place in the order moved. They bind their own
    // scene group, so the restore above serves them as it serves the orbs.
    c->renderer_.begin_patch_terrain_plan(pass);   // OIL_1 U13: one SetPipeline for the three slots
    // DOMESDAY_0 B3: the per-slot list window rides the vertex-buffer
    // offset now (FC_SEG_A/B/C — the same segments the retired g2:62
    // bind windows carved), delivered to the VS as @location(0).
    if (dmask & DrawBit::TERRAIN_A)
    c->renderer_.draw_patch_terrain_plan_slot(pass,
        c->gpuState_.scene_state_group(),
        c->gpuState_.visible_patch_indices_buffer(), FC_SEG_A_OFF, FC_SEG_A_BYTES,   // plan A window
        c->gpuState_.patch_index_buffer(),           // full IB (zone-overlapped)
        c->gpuState_.frustum_indirect_lod0(), 0);
    if (dmask & DrawBit::TERRAIN_B)
    c->renderer_.draw_patch_terrain_plan_slot(pass,
        c->gpuState_.scene_state_group(),            // B5 (R2): the one scene group
        c->gpuState_.visible_patch_indices_buffer(), FC_SEG_B_OFF, FC_SEG_B_BYTES,   // plan B window
        c->gpuState_.patch_index_buffer_cap_only(),  // cap-only IB (clean LOD0)
        c->gpuState_.frustum_indirect_lod0(), 20);
    if (dmask & DrawBit::TERRAIN_C)
    c->renderer_.draw_patch_terrain_plan_slot(pass,
        c->gpuState_.scene_state_group(),            // B5 (R2): the one scene group
        c->gpuState_.visible_patch_indices_buffer(), FC_SEG_C_OFF, FC_SEG_C_BYTES,   // plan C window
        c->gpuState_.patch_index_buffer_lod1(),      // LOD1 IB (culled at last)
        c->gpuState_.frustum_indirect_lod0(), 40);
    // DOMESDAY_1 B5 (R2): the PlanB/PlanC windows collapsed into the
    // one scene group, so plan C leaves the RIGHT group bound for the
    // orbs — the old restore is retired with the groups it restored from.
    if (dmask & DrawBit::ORBS)
        render_orbs(orbs_state_, &orbs_deps_, pass);
```


### WITNESS — U1
```
grep -c "RAZOR_0 — THE BACKGROUND DRAWS LAST" src/cartridges/the_board/realization/render_passes.hpp   # 1
grep -c "begin_patch_terrain_plan(pass)"      src/cartridges/the_board/realization/render_passes.hpp   # 1 — the plan exists once, in its new place
grep -n "if (dmask & DrawBit::TERRAIN_A)\|if (dmask & DrawBit::TABLE)\|if (dmask & DrawBit::PAINTINGS)\|if (dmask & DrawBit::ORBS)" src/cartridges/the_board/realization/render_passes.hpp
#   → TABLE, PAINTINGS, TERRAIN_A, ORBS in that line order
sh tools/gates/glaw1/run.sh && python3 tools/gates/console_gate/run.py       # GREEN / PASS
python3 tools/binding_gen.py --check | grep P-seq                           # PASS — the stated bind is why
```
Commit: `RAZOR_0 U1 — the background draws last: the terrain plan follows the table and the gallery fork, so covered terrain fragments fail depth before shading (pixel-identical); the scene-state bind the plan used to lend the table is stated`

---

## U2 — LEDGERS, THE REGISTER, THE PUSH

```
python3 tools/binding_ledger.py && python3 tools/command_census.py && python3 tools/mirror_census.py && python3 tools/organ_ledger.py
python3 tools/binding_ledger.py --check && python3 tools/command_census.py --check && python3 tools/mirror_census.py --check && python3 tools/organ_ledger.py --check
python3 tools/gates/score/run.py && python3 tools/gates/shell_gate/run.py && python3 tools/gates/glaw2/run.py && python3 tools/gates/sha256_gate/run.py && python3 tools/wgsl_gate.py
python3 tools/binding_gen.py --check      # all PASS; S-6 until the push
```
Rehearsed diffstat: three ledgers, pins and line-cites only (23 insertions, 22 deletions).

`docs/OPEN.md`, at the top of the register:
```
## RAZOR_0 — THE BACKGROUND DRAWS LAST (on `claude/razor-0`; Jean's gates open)

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
```
Commit: `RAZOR_0 U2 — ledgers regenerated (render_passes pin); OPEN.md: RAZOR_0 open on claude/razor-0`
Push: `git push -u origin claude/razor-0`.

---

## JEAN'S GATES

1. **Identical pictures.** Same seed, same mood, the pawn standing still: a screenshot on
   master and one on the branch, indoors and outdoors and from the ribbon. Pixel-identical
   (the additive orbs and the fade are untouched; only opaque order moved). Also with
   `?bundles=0` — the direct arm shares the one function.
2. **The meter.** `the-board-web-meter`, the same three windows as before (a gallery on
   foot, a sunset flight, a night flight): `main_pass` on master vs the branch. The gain
   is the covered fraction; report the numbers even if small — they size the pre-pass
   residual.
3. Pixel and iPhone: row 1 on each (L48).
