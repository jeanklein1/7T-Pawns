# RIG_0 — THE TINT ARM LANDS, AND THE ROUTE A PROBE

*Handoff · authored 2026-09-07 · Claude → CC · Jean holds build, visual, merge, deploy.*
*Filed to `docs/HANDOFFS/RIG_0.md` while open; the directory dies with the campaign.*

## THE IDEA, IN ONE PARAGRAPH

GATHER_0's ruling 6 says the Tint arm is the shader gate from now on, and CC lit it — from
Chromium's own Dawn/Tint over CDP — in one session, where it caught what naga had blessed,
twice. A gate that lives in a session is a ruling on paper. This round lands CC's recipe as
a tool in the tree, `tools/gates/tint_arm/`, so any machine with a Chromium can run it and
`T7_TINT` names a file the tree owns; and it runs the one probe DARKROOM_0 priced — whether
`copyExternalImageToTexture` reaches a Dawn texture from emdawnwebgpu — in the same rig, so
Route A is a witnessed road rather than a priced one. No program code moves.

## AUTHORITY

Base: `master` at HEAD `f54e7310`, fetched 2026-09-07. Rides `claude/rig-0` (P12: a tool).
**CC authors from its own recipe** (registered in OPEN.md at GATHER_0): this handoff fixes
the tool's contract and witnesses, not its bytes — the bytes exist only where the rig ran.
Disjoint from DARKROOM_1's files; either order.

| file | state at base |
| --- | --- |
| `tools/gates/tint_arm/` | must not exist |
| `tools/wgsl_gate.py` | read-only — its `T7_TINT` contract is the tool's contract |

## THE CONTRACT

1. **`tools/gates/tint_arm/tint_arm.py <file.wgsl>`** — exits 0 and prints nothing on a
   module Tint accepts; on rejection prints Tint's messages (they contain "error") and exits
   non-zero. That is exactly what `wgsl_gate.py`'s arm expects of `T7_TINT`.
2. **The Chromium is named by environment, never guessed:** `T7_CHROMIUM` = the browser
   binary; absent, the tool prints one line naming the variable and exits 2 (an unrunnable
   gate is a fail, as `wgsl_gate.py` already rules).
3. **The traps are written in the tool's banner**, not remembered: `navigator.gpu` is absent
   on `about:blank` and present on a `file://` page; `chrome://gpu` may say WebGPU is
   blocklisted while it works from a secure context; SwiftShader's Vulkan ICD is bundled and
   must be pointed at explicitly when no GPU exists.
4. **`tools/gates/tint_arm/README.md`** — the recipe: the flags, the CDP driving, the two
   perturbations that prove the arm loses (the textbook `textureSample` under a non-uniform
   `if`; DARKROOM's `dpdx` moved inside the branch), and how to set `T7_TINT` on Windows,
   macOS and Linux.
5. **`tools/gates/tint_arm/probe_copy_external.py`** — the Route A probe: a page with a
   canvas, an ImageBitmap, a Dawn device from the program's own emdawnwebgpu build, and one
   `copyExternalImageToTexture` into an `rgba8unorm` texture read back through a buffer.
   Prints `OPEN` or `CLOSED` with Chromium's exact message. Its verdict is the round's
   report; nothing in the program changes on either answer.

## UNITS

| unit | what | commit |
| --- | --- | --- |
| U1 | `tint_arm.py` + README; `T7_TINT` lights on the base; the two perturbations lose | 1 |
| U2 | `probe_copy_external.py`; the verdict | 1 |
| U3 | OPEN.md: GATHER_0's "rig not in tree" note closes; RIG_0's entry; push | 1 |

### WITNESS — U1
```
export T7_TINT=$PWD/tools/gates/tint_arm/tint_arm.py
python3 tools/wgsl_gate.py                       # "[gate] tint arm PASS (…tint_arm.py)"
# then each perturbation as a sidecar (P18): the arm FAILS with Tint's uniformity message; restored, PASS again
```
### WITNESS — U2
```
python3 tools/gates/tint_arm/probe_copy_external.py      # OPEN or CLOSED, with the message
```

## JEAN'S GATES
1. On your machine: `set T7_TINT=…\tint_arm.py` and `set T7_CHROMIUM=…\chrome.exe`, then
   `python tools\wgsl_gate.py` reads "tint arm PASS". If it cannot, the README's Windows
   section is the finding.
2. The probe's verdict in the report — read it once, file it under DARKROOM_1's residuals.
