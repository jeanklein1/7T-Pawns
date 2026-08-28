# THE LAWS

Repo home: `docs/`. Sibling of `7t_program_theory_v3.md` — that file is
THE LENS (how to think about the program); this one is the LAWS OF PRACTICE
(what breaks if you don't). Created by PRUNING_1 P4 because there was no
live one: the `cartridge_constitution` and the `terrain_program_charter`
are superseded charters, in the attic since CANON (resurrect by sha; L30),
and the theory explicitly holds itself apart from law.

**What belongs here.** A rule that survives every line of code it currently
governs. If deleting the subject deletes the rule, the rule is a comment and
belongs next to its subject. If deleting the subject leaves the rule true and
the next author walking into the same trap, it belongs here.

Rules are NUMBERED and the numbers are permanent. A retired rule is struck,
not renumbered — a citation in a commit message has to keep resolving.

---

## L1 — THE ENCODING LAW

`world.wgsl` is **BOM-free, LF-terminated**, on every platform.

Enforced mechanically by `.gitattributes` (`*.wgsl text eol=lf`), which is the
authority; this rule exists so the reason survives the enforcement. A CRLF
checkout churns every line of a 12,000-line shader diff and buries the real
change. (The original reason — a sha256 sidecar on the deleted web mirror —
is gone (WEB_PORT_LEDGER, attic). The pin stands on its own merit.)

## ~~L2 — THE FXC LAW~~ — **STRUCK 2026-08-12 (PIVOT_0)**

**Struck, not renumbered:** the number is permanent so citations keep
resolving. The full text, its three retired constraints, and what each one
still explains about the shape of `world.wgsl` are preserved verbatim in
**`docs/FXC_LAWS_RECORD.md`**.

**What replaced it.** The audience floor is WebGPU core through modern
compilers — Tint→DXC (SM6.0+), Tint→MSL, Tint→SPIR-V, naga. FXC is
**unsupported**. The native compiler is one constant, `kCompilerPlan` in
`src/console/console.hpp`; the shader's live statement of the floor is the
COMPILER FLOOR block in the `world.wgsl` banner, which is where L2's
operational home used to be.

**Why.** WALLET_0's occupier cbuffer arrays stalled `update_player_agent`
at 20,227 ms under FXC, then `D3DCompiler_47` access-violated on the next
room kernel. Jean ruled the floor up rather than the shader down.

**Do not honor L2's constraints as live.** Code already shaped by them is
not wrong — it is merely no longer required to be that shape, and undoing
any of it needs its own measurement. The agent-kernel split
(`update_player_agent` / `update_other_agents`) is the one with a price on
record: 48 s of FXC compile. Whether DXC prices it the same way is
**unmeasured** — re-witness before merging those kernels back.

**One clause of L2 was never an FXC law and survives it.** Item 4 — storage
buffers 8/stage, uniform buffers 12/stage — is WebGPU **core defaults**,
binding on every backend and every compiler. It lives in **L14**, in the
`world.wgsl` banner's budget line, and as the binding ledger's `gate`
witness. Nothing about it changed.

**The witness protocol survives too**, minus the compiler it named: a
shader-shape change is proven by witnesses, never by argument, and no
witness substitutes for another. `tools/wgsl_gate.py` is the per-commit
module gate (the in-tree transform as its pinned half, behind the
immediate shim); the web build + boot is the witness of record; each
browser gates at its own.

**THE PER-COMMIT GATES, NAMED (GATE_1, 2026-08-16; refreshed RECENSION_4).**
They answer different questions, and their scripts are the authority on
their own invocations and subjects:

| gate | home | subject |
|---|---|---|
| module gate | `tools/wgsl_gate.py` | the WGSL module (naga behind the pinned in-tree immediate shim) |
| TU gate | `tools/gates/console_gate/run.py` | `cartridge.hpp` and `console.hpp` as TUs, warnings read, `-D__EMSCRIPTEN__` (GATEHOUSE_0) |
| standalone compile | `tools/gates/glaw1/run.sh` | the cartridge TU's names, scope and syntax |

The console gate is here because glaw1's translation unit is
`cartridge.hpp`, which does not include `console.hpp` — so for the life of
the tree glaw1 answered GREEN to console edits it had never read. An absent
witness wearing a witness's name is worse than no witness, because it is
counted. It compiles against the emdawnwebgpu payload pinned in
`third_party/emdawnwebgpu/PINNED.md` and never a system or emsdk copy;
its own banner states its boundary (syntax and types, no linking, no
semantics), and the web boot remains the witness of record past it.

*This strike is L15 collecting a debt on the largest referent in the tree —
see L15, and note that the three retired constraints are exactly the kind
of prose that goes on asserting itself long after its subject is gone.*

## L3 — THE MIRROR LAW

Three pairs of rooms hold the same facts in two languages, and nothing but
this rule and a witness keeps them equal:

1. `world.wgsl` §2.1 structs mirror `realization/state.hpp` **byte-for-byte**
   (`FrameSignal`, `AgentState`, `AgentBehaviorParams`, `AgentTierParams`,
   `DesignConfig`). Drift means the GPU reads different fields than the CPU
   wrote — no error, just wrong pixels.
2. `world.wgsl` §3.4 `POLICY_*_MASK` constants mirror `POLICIES[].contributors`
   in `contracts/ground_architecture.hpp`. The `POLICY_*` / `CONTRIB_*` numeric
   constants mirror the C++ enums, whose values double as table indices.
3. `world.wgsl` §1.5 deterministic-randomness helpers must produce
   **bit-identical** results to `primitives/seed_utils.hpp`. Same seed, same
   world, on both sides of the bus.

**Edit both rooms in the same commit.** The C++ room is held by `offsetof` /
`sizeof` static_asserts and `ASSERT_POLICY_DAG_CLOSED`; the WGSL room is held
by nothing the compiler can see, which is why the rule is written down.
`tools/gates/glaw2/run.py` checks the policy and contributor mirrors. **The
struct mirror currently has no automated check.** It was `pruning_census.py`
§3, retired at WINNOW-2 T-j; the check was not re-homed, so until it is, this
law rests on the `sizeof`/`offsetof` asserts in `state.hpp` and on the reader.

## L4 — THE ALIGNMENT LAW

**Every `float[3]` in `GPUDesignConfig` whose WGSL twin is `vec3<f32>` must sit
on a 16-byte boundary.**

WGSL rounds `vec3<f32>` up to align 16; C++ packs `float[3]` at align 4. A
field inserted *before* one of them shifts the C++ offset by 4, 8, or 12 and
the WGSL offset by a full 16. `sizeof` can stay equal while every following
field diverges — so the size witness does not fire, and neither does anything
else. Today the four are `sun_direction`, `fog_color`, `fade_color`,
`checker_resultant`, each already 16-aligned, held by a `static_assert` in
`state.hpp`.

Grow the struct at the **tail**, or insert a matching `_pad` so each `vec3`
keeps its boundary in both rooms. A declared pad is not waste; it is the
mirror.

## L5 — THE GROWTH LAW (how a config knob is born)

Field ORDER is the cross-room contract — `world.wgsl`'s `DesignConfig` mirrors
`GPUDesignConfig` field for field.

1. Prefer re-using a `_pad` slot inside the right `───` system group; else
   append within the group and let padding re-flow.
2. Edit **both rooms in the same commit** — same position, same type (L3).
3. Bump the `sizeof` witness. The number in the assert IS the handshake.
4. Targeted sub-writers carry `offsetof` witnesses; glaw1 re-proves them, so a
   silent shift is impossible.
5. The knob's REST value is authored at the boot block or its panel room,
   **never** in the struct.

New knobs join a cadence: dirty-config for slow dials, a bespoke hot writer
for per-frame voices. And see L4 before choosing the position.

## L6 — THE BINDING-NUMBER LAW

`tools/binding_schema.py` **authors** the GPU binding numbers (L22);
`binding_gen.py --write` emits `realization/binding_registry.hpp` from it.
The registry is the numbers' **one home in C++** and every consumer reads
it — but it is a generated mirror, and a hand edit does not survive
`--check`. This law governs the numbers' shape and their mirrors.

1. Every bind-group layout entry and its matching group entry reference the
   **same named constant**. The "binding integer typed twice" hazard becomes an
   undefined symbol glaw1 catches, not a runtime crash.
2. Numbers are **group-scoped**, not global: `25` is `tile_grid` in
   group 0 and `shadow_map` in group 1. `g0::` and `g1::` are separate
   namespaces because a flat list would fuse distinct slots.
3. Numbers are **authored, not computed**. The `render = compute + 200` band is
   a `static_assert` witness at the foot of the registry: it CHECKS the
   authored literals and is never their source.
4. One constant per **site**, named for the WGSL variable it mirrors — not one
   per buffer. The same buffer wears several names because each name is one
   `(group, slot)`.
5. **A retired number is free.** Numbers are not reserved and comments do not
   reserve them; the registry shows what is taken, git shows what was.
   Precedent: 149 was retired and its neighbors 190/191 were reborn as the cmg
   pair. Reuse is normal.
6. The WGSL `@binding` literals stay a **mirror** — the shader cannot read a
   C++ constant. Lockstep is held by `binding_gen.py --check` (witness S-3:
   the `--write-wgsl` round-trip with unchanged numbers is the identity on
   `world.wgsl`) and by the crash-aware launch gate (bind-group and pipeline
   validation at boot), not by the compiler. The registry names deliberately
   equal the WGSL variable names so the mirror is greppable in both files.
   The token-substituted half of the once-named follow-on is built:
   `binding_gen.py --write-wgsl` renumbers the declarations from the schema
   on demand. The schema authors the numbers (L22); `--write` emits this
   registry; the WGSL text stays the mirror.

## L7 — THE BINDING-CLOSURE LAW

For any host that builds a pipeline against a hand-written bind-group layout,
the test that governs a resync is **binding closure**, not entry-point
existence.

The obvious check — "do the entry points the host names still exist upstream?"
— passes and means nothing. What matters is the set of `@group/@binding` slots
**reachable from each dispatched entry point**, because that is what the layout
must satisfy. An entry point that grows one new reachable binding breaks a host
whose layout is a transcription, and it breaks it at pipeline creation, at
boot.

Proven the expensive way by the deleted web port, which would have shipped a
module its layout could not satisfy while both entry points existed the whole
time — the WEB_PORT_LEDGER (attic). The closure was computed by
`cc4_wgsl_static_usage.py`, a one-shot campaign probe retired at WINNOW-2 T-e;
a future port that needs it again regenerates it. Any future port wants
**generated** layouts, not transcribed ones.

## L8 — THE TOMBSTONE LAW (PRUNING_1)

**Git keeps every word.** A comment whose subject has been deleted carries no
information git does not, and it rots in a way git does not: it accretes,
it is grepped by future readers as if live, and it is indistinguishable from a
description of code that is still there.

The mechanical test, applied to any comment: **if it would still make sense
with the code beneath it deleted, it is prose** — and prose goes, unless it is
a law with no other home, in which case it comes here as a numbered rule and
leaves at most a one-line pointer behind.

What this does NOT license: deleting a comment that describes live code, a
number, an invariant, or a hazard adjacent to the thing it names. The failure
direction is under-reach. A deleted law is invisible until it bites.

## L9 — THE STATUS CONVENTION

A declaration that is not wired says so, in the declaration, in these words:

- `STATUS: REALIZED` — wired and live. **Cite the consumer.** A REALIZED tag
  with no named consumer is a claim, not a status.
- `STATUS: LATENT[name]` — a capability with a plausible future; kept and
  tagged; revive-or-rewire when this region is next worked. The bracketed name
  is the handle, so every site of one capability greps together.
- `STATUS: INTENT` — declared, zero realization yet, kept with the status
  stated. Honest futures, not lies.

Two riders, both learned the hard way:

1. **A tag is not a reprieve.** `STATUS: LATENT` and `STATUS: INTENT` are
   DELETE-AND-RECORD by default — the tag buys one reading, not permanent
   residence. The standing census of them was `pruning_census.py` §4, retired
   at WINNOW-2 T-j and not re-homed.
2. **A tag dies with its subject.** When the declaration goes, the tag goes;
   a status describing something already deleted is a tombstone (L8), and the
   status word makes it read as live.

## L10 — BOOT IS A TRANSITION FROM NOTHING

**The world has exactly one way to come into being; the only difference
between boot and a mood change is what came before.**

Boot is not a special case that happens to resemble a transition. It IS the
transition whose prior state is empty. Wherever a transition path applies a
value, boot must reach that value **by calling the same door** — never by a
literal, an in-struct default, or a hand-copied table row that happens to
agree today.

The failure mode is silence. A hand copy and its source do not diverge with a
build error; they diverge the day someone edits one of them, and the symptom
is a boot frame that is subtly wrong in a way no test names. BOOT_ONE_VOICE
found the whole family at once: an amber sphere the CPU census did not know
existed, a test rig that outlived its own retirement condition, five
transcriptions of `MOOD_TABLE[0]` across three files, and a frustum-cull flag
whose only writer was `apply_mood` — so the world booted in `open_default`
wearing a cull setting that belonged to no mood at all.

The two doors this law currently names:

- `apply_mood` — the atmosphere, the feature gates, the orbs. Called at boot
  and at every transition, with `mood_state_.active`.
- `reset_surface` — patches, tiles, themes, queues, piers, footprints. Called
  at boot and by the transition machine. It was `teardown_surface`; the rename
  is the law made visible in the name.

**What this does NOT license.** Boot legitimately owns things a transition
never touches — buffer creation, pipeline construction, the one-shot index
generation, and the REST values of knobs no mood authors (fog is the live
example: `apply_mood_lighting` does not touch it, so the boot fog values are
correct, not residue). The test is not "did a transition write it" but
"**does a transition path author this value?**" If yes, boot calls that path.
If no, boot is its author and says so.

A corollary worth stating, because it was learned the expensive way: when boot
stops hand-copying a value, give the field a rest value that **fails loud**.
`MoodState::sun_intensity` rests at `0.0f`, not `0.8f` — if the door ever
fails to run, the sun goes out on frame 1 instead of hiding behind the value
the door would have written.

## L11 — THE PAINT ANCHOR LAW

**Evaluate a thing in the frame that owns it.**

Physics is owned by the world, so it reads the live position — the grounded
lift, the card, the terrain under the body this frame. Pigment is owned by the
body, so it reads the mesh frame: `paint_pos = vec3(world_pos.x, in.pos.y,
world_pos.z)` — mesh-authored XZ (the grounded mesh-gen lift is Y-only),
body-relative Y, immune to `ground_y` and the live card. `world_pos` remains
the coordinate of light, fog, and veil.

Either half alone reads as arbitrary — why should paint ignore the ground the
body stands on? — and together they are one principle. The failure the law
prevents is a pattern that swims: a body whose pigment is evaluated in world Y
repaints itself every time the ground under it moves, so its own surface
crawls while the body holds still. The mosaic found this first (MOSAIC_1), but
nothing about it is the mosaic's: any body-owned field — wear, weathering,
inscription — wants the same frame.

Two coordinates, two jobs; neither borrows the other's.

## L12 — DISTANCE TAKES THE GRAIN, NEVER THE MATERIAL

**A body's identity must not be a function of range.**

What fades with distance is the detail the eye can no longer resolve. What
must not fade is what the thing IS. A ceramic body seen from across the valley
is still ceramic — smoother, flatter, its tesserae gone — and if it instead
becomes the stone it would have been had it never been painted, then the
world's inventory changes as the camera moves, and no vantage point tells the
truth about it.

The law has a second half, and it is the half that pays for the first: **the
structure that produces only the faded detail should not be evaluated once it
has faded.** MOSAIC_1 fell into both halves at once — it dissolved a mosaic to
its palette color (identity as a function of range) and it justified that
dissolve as a cost cap it did not deliver (the walk still ran everywhere
inside the band, and a radius caps the mean, never the max). MOSAIC_2 split
them: the passage median is the material and is evaluated always, one hash; the
shard is the grain and its 27-cell walk runs only where the grain is fine
enough to matter. The saving is real precisely because it is structural rather
than a fade.

Where the grain's band comes from is a second question, and the answer is: the
band a body already has. MOSAIC_2 binds grain to `1 − veil_t`, the veil's own
icing smoothstep, so a body materializes at the ring already itself and gains
its detail across exactly the band where it materializes. Two bands measuring
"how far is this body" from two anchors can disagree — a body dithering out at
the ring while its grain insists it is near — and one band cannot.

The test for any distance-driven simplification: **name what the far form IS.**
If the answer is "the same material with a term at zero," the simplification is
lawful. If the answer names a different material, it is not a simplification —
it is a second body wearing the first one's geometry.

## L13 — A BOUNDARY IS A ZONE, NOT A LINE

**Where two regions meet, the meeting has width — and the far form of that
zone is the near form's average.**

A lattice gives you cells, and the naive reading of a cell is that everything
inside it takes the cell's value. That reading puts a hard edge at every face,
and worse, it cuts through whatever unit the content is made of: MOSAIC_2's
recon found passages sampled per fragment, so a single ceramic tile straddling
a passage face was **half one colour and half another** — a thing no tiler has
ever produced, and a defect no amount of tuning the edge would have fixed.

Two rules, and the second is what makes the first safe.

**Sample at the unit, not at the point.** If the content has a grain — tiles,
cells, grains, bodies — the region lookup belongs at the unit's own position,
so the whole unit belongs to one region. The fragment is where you are, not
what you are part of.

**Realize the zone in whatever the range can resolve.** Near, where units are
visible, the zone is *interleaved*: perturb the lookup per unit so units near a
face fall on either side, and the boundary becomes a band of intermixed
regions. Far, where units are not resolvable, the zone is *chromatic*: lerp
between the regions. At the boundary these two are the same function at
different scales — an unresolvable band of alternating blue and white tiles
**is** a blue-white lerp — so they agree in the limit by construction, and that
seam cannot pop or alias no matter how it is tuned.

**Do not extend that identity past where it holds.** MOSAIC_2's first draft of
this law claimed the far form is *always* the near form's average; adversarial
review showed it is not. Within a passage the far field draws one member where
the near field is a mixture of several — the ensemble means coincide, the
per-instance values differ by ~0.2 per channel. The law survives the correction
because the identity was only ever earned at the boundary, which is where it
was derived.

So the general form has two halves, and the second is the one that gets
forgotten: **when a simplification and the thing it simplifies are the same
function at different scales, no seam between them can exist** — and **when
they are not, the seam is real and must be placed where nothing can see it.**
MOSAIC_2 does both: the boundary zone is the same function twice, and the
within-passage seam is parked at `grain ≤ 0.001`, which the veil coupling
makes identical to "this fragment is ≥99.9% fog." A seam hidden by a
coincidence is a bug; a seam placed deliberately, where the placement is a
consequence of the design rather than of tuning, is engineering. The
distinction is whether you can say *why* nothing can see it.

## L14 — THE DEFAULT-LIMITS LAW

The lean build requests no WebGPU limit above core defaults and fits them:
storage buffers 8/stage (C6), texture array layers 225 of 256 (OPT_1b).
Exceeding a default requires Jean's stamp and a recorded reason; the
full-adapter passthrough at boot is a convenience, not a dependency.

The rationale is **compatibility**: a program that asks only for what it
uses runs on the widest set of devices, and the phone is the target that
decides. Requesting the adapter's maximum is not a harmless superset — it
narrows the device set for nothing — so restoring passthrough as a
"simplification" is a regression.

**No timing evidence backs this law, and none is claimed.** A previous
revision cited a single-run bisect on the development laptop (62,517 ms vs
5,609 ms, "an 11× slowdown") as proof that modest limits are also a
PERFORMANCE requirement. That claim is **withdrawn**: timing on that machine
varies by an order of magnitude between runs, so no single-run comparison
from it is evidence. Brackets on identical code:

| measurement | observed range |
|---|---|
| native pipeline creation | 70,459 → 205,527 ms |
| native patch system | 1,223 → 62,000 ms (era-dependent) |
| web total boot | 5.6 → 73.6 s |

The limits choice may or may not affect performance; this machine cannot
answer it, and the law does not need it to. Compatibility is sufficient
ground.

## L15 — A REFERENCE OUTLIVES ITS REFERENT

**A reference outlives its referent. Cite the symbol, not the line; name the
witness, not its value. And when a comment names a symbol, it has taken on a
debt the compiler will never collect.**

Ratified by HEM_1 from two misses in the same campaign — one in a handoff,
one standing in the tree:

| citation | claimed | actual | drift |
|---|---|---|---|
| the `sizeof(GPUDesignConfig)` witness, quoted as a value | 560 | 624 | two campaigns (MOSAIC_0, FIELD_2b) |
| `pawn_profile_normal_2d`, named by two `world.wgsl` comments | a function beside `pawn_profile_radius` | never defined, in any commit | the whole history |

These are one failure, not two. A line number, a quoted `sizeof`, and a symbol
in prose are all references to something that moves or was never there, and
nothing checks any of them. The symbol `GPUDesignConfig` survives every edit
that moves its size, while the number 560 was true once and then silently was
not. `pawn_profile_normal_2d` named a symmetry that was planned and not built,
and went on asserting it long after the code folded that branch into
`eval_profile_normal_2d` behind `is_regular`.

**The debt.** The compiler collects on a renamed function and never on a
renamed function *inside a comment*. Prose that names a symbol is load-bearing
with no test behind it: it must be re-read whenever the symbol moves, and
nothing will remind you. Name symbols anyway — vague prose is worse — but know
that naming one is a commitment, and that grepping the comment corpus before a
rename is the only collection mechanism there is.

**In practice.** In a handoff or a report, cite the symbol and let the reader
read its current value. Carry line numbers as hints, marked as hints, and
verify every one by symbol before editing.

## L16 — THE MECHANISM AUDIT

Every ruled mechanism in a handoff carries its own recon step verifying
the facts it stands on. **A design decision without a mechanism audit
does not land.**

Paid for three times in one session, each time by a mechanism that was
named confidently and was not what the design assumed:

- **`compute_vp`'s sun write.** ATLAS_1's D2 gave the sun and spot
  light 0 one slot, on the belief that the sun VP had a CPU writer at
  mood cadence. It has no CPU writer at all — `compute_vp` writes it on
  the GPU every frame, and the per-tile copy that D4 was retiring was
  also what protected light 0 from it.
- **The occupied instance channel.** D3 proposed `firstInstance` as a
  channel for the light index. `firstInstance` does not add a channel —
  it *biases* `instance_index`, five shadow VSes already index with it,
  six shadow draws are instanced, and the terrain's band 1 was already
  using it as an index base.
- **The gallery layout gap.** D2′ and D3″ were both sound and neither
  could *reach* two of the thirteen shadow VSes, which are drawn on a
  group-0 layout carrying neither binding.

None was catchable by argument; each was one grep from visible. The
audit is the grep, written down before the edit.

## L17 — DELEGATION CROSSES DESIGN FORKS, NEVER MEASUREMENT GATES

CC may rule a design fork under explicit delegation, with the ruling
annotated in the artifact so the choice has one home and a reader can
find who chose and why.

A gate requiring a **boot, a walk, or any physical reading is never
reasoned past.** A design fork has an answer in the tree; a measurement
gate has its answer only in the world. Confidence is not a substitute
for a reading, and an argument that a reading *would* come out a certain
way is the exact shape of the error the gate exists to catch.

## L18 — GATE RESULTS TRAVEL IN WRITING

Walk and boot verdicts are pasted into CC's next session opener or into
the merge commit. **A merge is never recorded "no walk reported" when a
walk occurred.**

The record is the only thing a later session has. A verdict held in a
person's memory is, to the tree, indistinguishable from a verdict that
never happened — and the merge commit is where a bisect looks first.

## L19 — COMPAT MODE, DECLINED AS A DECISION

WebGPU compatibility mode may zero vertex-stage storage bindings, and
the render room stands at **V storage 7 of 8** (`Shadow Gallery Frame` /
`Shadow Wall Painting`, post-ATLAS_1revB G2).

**Declined.** Revisit only if a *measured* audience device requires it.

Recorded as a decision rather than an omission: the constraint is known,
the cost of honouring it is known, and the choice is to spend the
headroom on the work instead. An unrecorded decline is indistinguishable
from an oversight, and the next reader of that 7-of-8 deserves to know
it was seen.

## L20 — OPTIONAL FEATURES

The baseline is **WebGPU core, one shader source.** A feature is adopted
only with all three of: runtime detection, an identical-semantics
fallback, and its own witness at every gate.

**The adopt-list starts empty.**

- `timestamp-query` stays meter-preset-only. It is an instrument, not a
  capability the artwork depends on.
- Texture compression, *if* the paintings are ever unfenced, is
  per-platform transcode — never baseline. The grants census already
  shows the split: ASTC and ETC2 on the Pixel's valhall row, BC absent.

Two shader sources is the failure this forbids. A feature adopted
without a fallback makes the second source inevitable, and the moment
there are two, every witness in the tree is witnessing half a program.

## L21 — A TOGGLE IS CHAINED AT THE STAGE THAT CONSUMES IT

Dawn toggles carry a `ToggleStage` — Instance, Adapter, or Device. A
toggle chained at a descriptor **downstream** of its stage is silently
inert: no error, no warning, no toggle. A toggle chained **upstream**
depends on inheritance carrying it every hop, which is a Dawn
implementation detail this program cannot read and must not assume.

**So chain each toggle at its own stage, and never at a neighbour's.**
`use_dxc` is Adapter-stage and belongs on the instance/adapter path;
`disable_symbol_renaming` is Device-stage and belongs on the device
descriptor. Two toggles at two stages is the correct shape, not
duplication to be tidied away.

Paid for by PIVOT_0a and debt 12. `use_dxc` was chained on the device
descriptor, one stage too late; the boot log said "Compiler plan: DXC"
and FXC compiled anyway. The reverse error — a Device-stage toggle
chained on the instance descriptor and hoping inheritance delivers it —
was never disproved, and under this law never needs to be: the question
only arises for a toggle sited somewhere other than its own stage.

**Debt 12 therefore closes as MOOT, not as branch (a) or (b).** Its
question — does the instance chain propagate two hops? — stops gating
anything once no toggle is ever sited off its own stage. Reopening it
costs one boot: re-arm one Device-stage toggle on `idesc.nextInChain`
alone and read the count. Present → inheritance carries; absent →
branch (b) confirmed at last. Nothing in the queue wants that boot.
`disable_symbol_renaming` at count 9 → 10 on the device chain (Dawn
`f0bf8ab`, 2026-08-13) is the positive half, and the only half proven.

**The corollary is the witness.** A toggle request is not a toggle.
`dawn::native::GetTogglesUsed(device_)` prints the set Dawn actually
enabled, and that line stays in the boot log for exactly this reason: a
switch that cannot be seen to have fired is indistinguishable from one
that never fired (P6).

## L22 — THE SCHEMA LAW

`tools/binding_schema.py` is the single authority for the binding
surface. `binding_registry.hpp` and `binding_surface.gen.inc` are
generated; edit the schema and run `binding_gen.py --write`.
`world.wgsl` declarations are checked mirrors: `binding_gen.py --check`
must pass at every campaign's recon gate and before any commit that
touches the surface.

## L23′ — THE SCOPE LAW (supersedes L23)

Within one synchronization scope, a buffer presents ONE writability.
A render pass is one scope, WHOLE. A compute dispatch is one scope
over its FULL bound groups. Neither is filtered by shader-stage
visibility or by static use — Dawn merges every entry of every bound
group, touched or not (Jean's boot log at the U4 gate is the
evidence; L23's "compute validates per dispatch over what it uses"
was the half of the truth that survived one gate).

So: **mixed-writability faces of one buffer never share a layout and
are never co-bound in one scope.** A stratum serving a scope carries
only the faces that scope may legally see — FRAME split by consumer
mode (FRAME_R render / FRAME_C compute, A8a); ORBS carries its face
partition in two layouts (A8b); GALLERY/PHOTO_K stand from A7.

Witness: `P-scope`, both arms — the render arm per pass span, the
compute arm per dispatch site over the full bound groups, plus the
group-local law (no bind group backs one buffer through entries of
mixed writability). **Pessimism is the law: no relaxation of the rule
may ever be committed on a citation — only on a witnessed Dawn
behavior test.**

Paid for twice: A7's gallery working set in the render passes, then
A8's FRAME ro windows and collapsed orb faces at the compute
dispatches — the same law, learned one scope at a time.

## L24 — THE ROOM GROWS BY TEXTURE OR UNIFORM

The agents' room grows by texture or uniform, never by a new storage
seat; the one named payment, if ever unavoidable, is the `agent_state` +
`field_forces` merge.

The room family — `update_player_agent`, `update_other_agents`,
`update_sphere`, `update_cube` — is the program's tightest storage row.
Every ordered pair among those four already carries a RAW hazard
(Table E), so the seats it holds are coupled as well as counted, and a
new one is paid for by a demotion somewhere in the same rows. The merge
is named here so that it is a DECISION when it happens rather than a
discovery: two RAW-coupled seats become one, authorized by name at
Jean's gate and never as a side effect of a feature.

Priced, not spent: the demotion candidates and what each costs are
Table D's thirteen A2 rows, ruled from `DEMAND_RULINGS` in the schema
(PROBATE_D).

## L25 — ARRAYS BEFORE SEATS

A new compute-written surface prefers a layer in an existing
storage-texture array over a new seat; the lane stands at 2 of 4.

A storage-texture seat is charged per stage like any other, and the
program already owns arrays with spare layers. A layer costs memory,
which the budget prints; a seat costs a slot in a lane with two left.
Reach for the layer first, and name the reason if you do not.

## L26 — MARKED-DEAD DIES ON NEXT OPENING

A field the tree marks dead — "retained until the next relayout" and
kin — dies in the same sitting that next opens its struct; twin rooms,
one commit.

A field kept alive by a note is a fact with two homes: the layout and
the note that says the layout is wrong. The note is cheap to write and
free to ignore, so it accumulates. Binding the removal to the next
opening of that struct makes the cost fall on the campaign that is
already paying the mirror cost (L3: both rooms, same commit) — which is
the only campaign that can remove it for free.

## L27 — THE JOIN-COHERENCE LAW

A schema datum and the emitter that joins on it move in one commit;
splitting a key from its join manufactures a red between commits.

A key is only half a fact — the other half is the code that looks it up.
Land one without the other and every witness that reads the pair goes red
in the gap, which teaches the next reader that red is normal there. This
is the ONE sanctioned form of an instrument and its subject sharing a
commit, and the commit message must name it.

*Paid for twice, one round apart.* PROBATE_I taught `imm_bytes_of` a new
`sizeof()` spelling in the same commit that put that spelling in the
schema — split, `--check` would have compared 32 against a parsed 0 for
as long as the split lasted. PROBATE_E1 re-keyed `DEMAND_RULINGS` to
stable ids in the same commit that taught Table D to join on them —
split, the loud-orphan warning would have fired fourteen times on a tree
that was exactly as correct before as after. Both are the same shape: a
red that reports nothing except that the campaign chose to land two
halves apart.

The corollary is the useful half: a warning path that fires only in the
gap between two commits is not a finding, and a round that never opens
the gap never has to tell its reader to ignore one.

## L28 — MINUTES ARE STAMPED, NEVER REWRITTEN

Audit reports are minutes: stamped, never edited; a known drift is named
on the stamp, not corrected in the text.

A report is a record of what was believed on a date, and its value is
that it does not move. Correcting a number inside one destroys the only
thing it was for — a reader can no longer tell what the campaign
actually thought, and every later citation of it becomes unverifiable.
But an unmarked stale report is worse than a marked one: a reader greps
it, believes it, and is wrong (P4's own test).

So the stamp does both jobs. It says the text is historical, it points
at the living law, and where a specific drift is KNOWN it names that
drift on the stamp — so the reader meets the correction before the
error, and the minutes stay minutes.

*Paid for by:* `SALON_1` (attic), which cites `L2.4`'s ceiling as
a live constraint at a value wrong twice over — the clause survived L2's
striking into **L14**, and the core default is **8**, not the 10 the
report carries. PROBATE_E's EF6 flagged it rather than editing it, and
this law is the ruling that flag asked for. The same stamp settles
`SALON_1_E_C_REPORT.md` beside it.

The sibling rule is P4's: being in an `audit/` or `past docs/` folder is
shelving, not filing. Filing is visible inside the file.

## L29 — HISTORY IS NOT THE WORKING TREE

A shallow clone answers "absent" for everything below its graft, so any
claim about what the repository has ever contained must first verify
depth.

`git log`, `git merge-base`, `git rev-list` and every tool built on them
answer from the objects present, not from the objects that exist. In a
shallow clone the graft boundary is indistinguishable, from inside, from
the beginning of history — a commit below it has no parents, an ancestor
below it is not an ancestor, and a blob below it was never in the
repository. Every one of those answers is *correct about the clone* and
*false about the project*.

The check is one line — `git rev-parse --is-shallow-repository`, or the
presence of `.git/shallow` — and the cure is `git fetch --unshallow`.
Neither is expensive. What is expensive is the report written without
them.

*Paid for by:* PROBATE_SEAL's F15, and collected at the campaign's last
hour. The session's clone carried 82 commits of `master`; `b491115` sat
below the graft. `git merge-base` therefore reported **no common
ancestor**, and `git fetch` reported **`(forced update)`** — so F15
recorded two histories "89 files and ~45k lines apart", an orphan line
of 59 commits, and a force-discard that never happened. Unshallowing
raised `master` from 82 commits to **1684**, and `merge-base b491115
master` returned **`b491115` itself**: a direct ancestor, 142 commits
back, with not one commit missing. The file counts in F15 were real; its
ancestry claim was an artifact of depth.

It is **P11 one layer down**. There the truncation was `head` on a
search's output; here it is the clone's own floor, and it is worse in
one specific way: `head` is visible in the command that used it, while a
graft boundary is invisible from every command that hits it. The tell is
therefore not in the output but in the question — **any sentence
containing "was never", "has always", or "no common ancestor" about a
repository is a depth claim first and a history claim second.**

Corollary, earned in the same hour: the first pass of the verification
that *found* this error had itself reported "20 files never present in
master's history", measured against the same shallow view. A law is not
proof against its own subject; the check is.

## SUNSET_0 (2026-08-16) — the web twin is the program
Native is archived at tag `native-sunset`. Resurrection is
archaeology from the tag, not maintenance. The witness chain is:
`tools/wgsl_gate.py` gates the WGSL module per commit (the in-tree
transform as its pinned half, behind the immediate shim;
CC-runnable); the web build + boot witnesses pipeline-layout
conformance and minBindingSize (the classes naga cannot see —
ATLAS_1revB). The audience floor (WebGPU core defaults) and the
compiler floor (PIVOT_0) are unchanged by this sunset.

SUNSET_1 (2026-08-16): the native arms are deleted from shared files and
the __EMSCRIPTEN__ guards collapsed — everywhere but
`realization/renderer.hpp`, which still carries four (census 2026-08-18).
Gate coverage of those guards is split — console_gate (defining
__EMSCRIPTEN__) checks the shipping arms, glaw1 the dead ones; registered
in `docs/OPEN.md`. The tree BUILDS one program; it does not yet COMPILE
only one. Resurrection remains archaeology from tag `native-sunset`.

SUNSET_2: the presets carry the web flow only; the guard fires first and
names the preset.

## L30 — THE TREE HOLDS LIVING MATTER ONLY

The working tree carries what the program needs today. **Git history is the
attic**, and it is a complete one: nothing committed is ever lost by deletion.

Resurrection is one line:

```
git checkout <sha>^ -- <path>
```

`<sha>` is the commit that deleted it — `git log --all --diff-filter=D -- '*<name>'`
finds it, and every WINNOW-2 tombstone commit lists its own paths in the body
precisely so that search lands.

The failure this prevents is not clutter, it is **false authority**. A document
that describes a subsystem as it stood three campaigns ago reads exactly like a
document that describes it now — same confident tone, same file extension, same
grep hit — and the reader who cannot tell has been actively misled by the tree
itself. Deleting it is not destroying the record; the record is in the attic.

*This is L29 pointed the other way.* L29 says history is not the working tree,
so do not read the tree as history. L30 says the working tree is not history,
so do not keep history in the tree.

## L31 — HANDOFFS DIE AT CLOSE

An executed handoff and its round report are **spent**. They die when the
campaign closes.

`docs/HANDOFFS/` holds **open work orders only** — if it is in there, it isn't
done. The directory being empty is a true and healthy statement, not a gap to
fill.

Campaign scaffolds are **self-consuming**: a census, a plan, a kill list is
authored to be spent, and the last unit of the campaign that created it deletes
it. A scaffold that outlives its campaign becomes a document describing a tree
that no longer exists — L30's exact failure, arrived at by a different road.

## L32 — OPEN STATE HAS ONE HOME

`docs/OPEN.md`, and nowhere else.

One line per item: what · origin (sha or doc) · what unblocks it. When an item
closes, its line dies — the register is never a changelog.

The rule exists because open state left in situ is open state nobody finds. Before
WINNOW-2 it was scattered across a PARKED LEDGER inside a 3,000-line archived
report, two decision briefs marked PARKED in a handoffs folder, a UTF-16LE note at
the repo root, and §11 of a campaign report — each of which was, locally, a
perfectly sensible place to put it.

## L33 — audit/ IS THE MACHINE'S ROOM

`audit/` holds **generated files ONLY**. Every tool lives in `tools/` (gates under
`tools/gates/`).

Humans write in `docs/`. If a file in `audit/` has no generator, it is in the
wrong room — and **everything in `audit/` is deletable and rebuildable by its
generator**, which is the room's standing witness: delete the five, run the
five tools, and the tree is byte-identical again.
The five that live there — `MANIFEST.md`, `BINDING_LEDGER.md`,
`MIRROR_LEDGER.md`, `COMMAND_LEDGER.md`, `ORGAN.md` — each name their producer
in their own header, and each is byte-reproducible from the tree.

They are **law and stay searchable**: a binding search genuinely wants the wallet
lane, so no ignore file hides them.

## L34 — CODE CITES LAW, NEVER TRANSIENTS

A comment may cite a law, a charter, or a durable reference. It may not cite a
campaign report, a recon, a census, or a handoff — those are transients, and the
citation outlives them.

**No `// History:` path breadcrumbs.** `git log --follow <path>` is the history,
it is always correct, and it costs nothing to keep. WINNOW-2 deleted 32 such
breadcrumbs; 31 of them named a file that had moved two weeks earlier, so the
tree carried 31 confident pointers to a path that did not exist.

If a document's rationale is worth keeping next to the code, **fold the sentences
in** and let the document die. The why belongs at the fact's home, not behind a
link.

## L35 — TRACKED PATHS: LF, NO BOM, NO SPACES

Every tracked path: **LF line endings, no BOM, no spaces in the name.**

L1 pins `world.wgsl` specifically and explains why; this generalises it to the
tree. A space in a path breaks every unquoted shell loop, every `git grep -F`
pipeline, and every tool that splits on whitespace — quietly, and usually in the
one script written in a hurry.

Enforced by `.gitattributes` for encoding. The no-spaces half is enforced by
review: WINNOW-2 retired the last three space-bearing paths — a release-console
report, a WGSL spec PDF, and a whole top-level directory — along with the tree's
last UTF-16LE file. They are named in the T-d, T-e and T-i tombstone commits;
this law does not repeat the paths, because a law that quotes dead paths becomes
the very thing L30 deletes.

## L36 — SOAK: ECOLOGICAL TRUTH OVER LAB TRUTH

Averaged readings across many seeds outrank pinned-seed comparisons, and relative
numbers within a session outrank absolutes across sessions.

A pinned seed measures one world, and the one world it measures is the one you
happened to pin. A number carried between sessions crosses a driver version, a
thermal state, and a browser build, none of which are in the notebook.

Measure the ecology, and compare inside the session that produced both halves.

## L37 — THE ONE-GENERATION LAW

The web twin's WebGPU generation is **this pin**; the native checkout
**tracks** it; a reference document without a stated revision is
**RECALLED, not CITED**. The pin, the native revision, and the fact
that they match are recorded in-tree and updated as one act.

**Witnesses:** `third_party/emdawnwebgpu/PINNED.md`; the boot's
`[Console] Dawn revision:` line; the dialect testimony, which names
the instance it measured.

**Paid for by** three arbitration rounds and one retraction. F1 cost
a compile on a spelling read from a document whose revision was not
the compiler's. F2 cost a boot on an enum that does not exist at any
revision. F3 corrected it from a probe of the right header — and was
still reading the *bench's* generation, eight months behind the
audience's. F4 was withdrawn before it ran. The one fact that would
have shortened all four was a pinned generation, written down.

*Numeral note.* DOMESDAY_2 §10 drafted this text and proposed **L24**,
correct when written (the book then ran to L23′). L24 was taken by THE
ROOM GROWS BY TEXTURE OR UNIFORM before the ratification arrived, so
the law lands at the next free numeral. The text is otherwise verbatim
from the draft, resurrected at CANON from `5ff5696`.

## L38 — THE COMPOSITION LAW (from the cartridge constitution)

Modules are real headers that own their state structs and laws; the
cartridge is the COMPOSITION ROOT — member instances, file-scope
includes, conductor calls, assembly, nothing else. Sight is granted by
explicit parameters (state references, the machine-context pointer,
declared services), never by ambient membership. The cartridge remains
ONE translation unit: boundary honesty, not compilation strategy.

**Verified at CANON**, against the tree rather than the charter:
- ONE TU — `tools/gates/glaw1/tu.cpp` is the witness and reaches the
  whole cohort through a single `#include ".../cartridge.hpp"`. Its
  banner states the boundary: *"this harness certifies OUR names,
  scope, and syntax; the SDK surface is vacuous-stubbed, so SDK-call
  correctness remains the rig's jurisdiction."*
- COMPOSITION ROOT — 33 file-scope module includes in `cartridge.hpp`,
  and **zero** class-body includes: the transitional form the charter
  called struck is genuinely struck.
- EXPLICIT SIGHT — service entry points take the context by parameter
  (`compose_spawn_chance(MachineCtx* c, …)`,
  `evaluate_spawn_gate(MachineCtx* c, …)` in
  `contracts/spawn_services.hpp`).

*Vocabulary note.* The charter called that parameter **the keyhole
pointer**. That word has 0 occurrences in `src/` — it survived only in
the two charters, both now in the attic — so this law states the
mechanism in the tree's own name, `MachineCtx*`. The claim verified;
only the word was retired.

Extracted at CANON from the superseded constitution; provenance in the
attic.

## L39 — PATH OWNS THE TOOLCHAIN

CMake's program search on Windows considers `.com`, `.exe`, and the BARE
NAME — not `.bat` — and searches NAMES_PER_DIR: it exhausts one directory
before trying the next. An extension-less POSIX script named `ninja`
earlier on PATH beats a real `ninja.exe` later, and `project()` dies with
"inappropriate file type or format". The same rule resolves `python`,
`git`, and `node` — every bare name in the build and deploy chain.

**depot_tools is not installed on this machine** (removed 2026-08-18,
and the Dawn source checkout with it — native is archived at
`native-sunset`; the web twin builds from the in-tree emdawnwebgpu pin).
If a Dawn source build is ever needed again: clone depot_tools fresh,
prepend it to PATH in that shell only, delete it after. It never enters
the persistent PATH.

**Toolchains live at paths we own**, not inside the IDE:
`C:\Program Files\CMake\bin\cmake.exe` and `C:\dev\bin\ninja.exe`.
`CMAKE_MAKE_PROGRAM` is an absolute path baked into `CMakeCache.txt`
exactly as `CMAKE_ROOT` is; a versioned IDE path dangles at the next
major update and takes the tree's build step with it.

A CACHE HIDES A BAD ANSWER. `find_program` writes its result before the
generator probes `--version`, so a failed configure leaves the bad path
behind and the next configure reads it instead of searching. Repair is
`--fresh`, not reconfigure.

Visual Studio's presets driver bypasses PATH and configures with its
bundled cmake, injecting `CMAKE_MAKE_PROGRAM` on the command line — no
preset or PATH edit defends against it; only removing its authority
does. Its automatic configure is disabled (Options → CMake → "Never run
configure step automatically"); the manual "Delete Cache and
Reconfigure" button still writes. VS edits text; the command line
configures. Fingerprint: the emscripten shared-libraries warning prints
from cmake 4.2–4.3.3 only — on this machine, only the IDE bundles one.
If it prints, the IDE held the pen.

Verify (cmd; in PowerShell `where` is the `Where-Object` alias — call
`where.exe`):

    where ninja & where cmake & where python & where git & where node
    findstr /S /I /C:"CMAKE_ROOT:INTERNAL" /C:"CMAKE_MAKE_PROGRAM:FILEPATH" CMakeCache.txt

## L40 — ARMING DIES WITH THE CHILD (emsdk_env.bat)

A `.bat` invoked from PowerShell runs in a cmd child; its environment
dies with the child — the PowerShell parent is never armed. The web
presets do not depend on the arming: the persistent `EMSDK` user
variable resolves the toolchain file, and emscripten's tools self-locate
from their own config (witness: the-board-web-meter configured, built,
and linked from an unarmed PowerShell parent, 2026-08-18). The
persistent `EMSDK` user variable is therefore load-bearing: deleting it
breaks every web preset at configure.

## L41 — ATTIC LAW

Branches list work in flight only; everything else is master or a tag
under attic/. A branch retires by tagging its tip attic/<name>,
pushing the tag, then deleting the branch on both sides. Recovery: git
branch <name> attic/<name>.

## L42 — FLAG-AND-FINISH

Within a phase, a failed item is logged and skipped and the phase
completes. Halting is reserved for gates where continuing risks
reachability or edits from stale authority.

## L43 — WHERE A TIMER POINTS

*Filed by SHIP_0 U1.*

**A timer names where the wait surfaced, not where the cost lives.**

Web per-pipeline times are **wire-enqueue latency**, not compile cost. The
backend compile executes in-order in the browser's GPU process and lands on
the first phase that waits. Witness, from one capture pair on the same
tree (`docs/reference/RELEASE_CONSOLE.md`):

| phase | web twin | native twin |
|---|---|---|
| `Total pipelines` | **14 ms** | 205,527 ms (Renderer init) |
| `Patch system` | **56,887 ms** | 1,413 ms |
| `Total init` | 56,945 ms | 206,941 ms |

Neither twin is lying and neither is measuring what its label says. One
cost, two attributions: the web's near-zero pipeline times are enqueues
that returned immediately, and its 56.9 s "patch" phase is where the
deferred compile storm came due. The native twin shows the reverse, and its
patch phase — 1.4 s — is the honest cost of patch generation.

**Corollary.** Chromium disk-caches compiled pipelines per origin, so a
fast revisit (5.5 s observed) is expected and is not evidence that the
first visit was mismeasured.

The rule: before attributing a cost to the phase whose timer moved, ask
what that phase is the first thing to WAIT on.

## L44 — THE TEMPERAMENT LAW

An idempotent author — one whose re-speak reproduces the same state from its
definitions — re-speaks at the frame boundary when its bank changes. A
destructive author — one whose re-speak tears down live state — does not: its
definitions take effect at its next natural event, and the bank's group banner
says so. Where one fact has both temperaments, the stricter governs — no
boundary wiring at all.

*Paid for by:* ORGAN_3/3b. `INDOOR_HEIGHT_CAP_FRACTION` has one idempotent
reader and nine destructive ones, so its bank has none.

## L45 — AN ENROLLMENT STATES A BELIEF; ONLY THE READER PROVES IT

A graduation from a design table to a live bank is complete when the table's
only remaining readers are its seed and its asserts. A bank that is built,
enrolled and left unread is worse than no bank: the dial writes, the write
lands, the world does not move, and the instrument reads as broken.
`tools/organ_gap.py --gate` is the standing witness.

*Paid for by:* ORGAN_3 w2 (`PANEL_LIVE`: seven dials over a bank nothing read)
and ORGAN_4 (eight of the sky's twenty-seven rows dead behind a green harness).

## L46 — A RULE RESTATED IN A SECOND LANGUAGE IS A RULE WITH TWO HOMES

Across a two-language seam the side that owns a fact derives it and emits it;
the other side asks. A number copied across the seam drifts, and the copy is
the one that is wrong.

*Paid for by:* ORGAN_6. The shell kept one definition-only sentinel after the
C++ minted a second; nineteen dials died in preview mode for two campaigns
behind a live reject counter.

## L47 — A COUNT IS NOT A DIAGNOSIS

A witness that counts refusals, drops or failures also names the last one —
which subject, and why. A bare count says that something happened and never
what.

*Paid for by:* ORGAN_6. `rejected 19` printed for two campaigns and carried no
information; `organ_last_reject` is the cure.

## L48 — THE THIRD ARM IS NOT WITNESSED

naga validates the module. Chrome/DXC and Chrome/SPIR-V are exercised every
session by the two machines in the loop. **WebKit's WGSL→Metal compiler is
exercised by nothing in the gate set and by no machine in the loop** until
Jean opens the iPad — and it is the strictest of the three.

So a round that touches shader entry points, bind group layouts, pipeline
construction or the render path **is not landed until an iOS witness has
run**. Every gate can be green and the piece can be black on a third of the
devices that matter.

Until that witness is habit, the iPad is not a device this program supports.
It is a device this program hopes about.

*Paid for by:* the optimization arc's iOS black screen. Eleven gates green,
two browsers rendering, one construct — a depth-only render bundle — wrong on
the arm nothing tested, for four rounds.

## L49 — A BUNDLE IS AN OPTIMIZATION, SO IT KEEPS ITS DIRECT PATH

Every bundled pass retains the direct encode AND a boot switch that reaches
it. The two roads call the SAME encode function, so they cannot drift.

**A recording that cannot be turned off is a change that cannot be bisected
on the device where it fails.** That is the whole of the law: not that
bundles are dangerous, but that an optimization which erases its own
alternative erases the only cheap way to test it.

The corollary is what made IOS_5 solvable in four reloads: the switch's
picture must be UNAMBIGUOUS. `?sunpass=0` skips the sun's draws and keeps
the pass's clear, because skipping the pass would leave the shadow map at
its zero-init — a world entirely in shadow, nearly black, and
indistinguishable from the fault being chased. A diagnostic that can be
mistaken for the disease is worse than none.

*Paid for by:* IOS_5. `?bundles=0` and `?sunpass=0` intersected to name a
single `ExecuteBundles` call on a device with no console, no inspector and
no cable.
