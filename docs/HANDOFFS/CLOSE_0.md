# CLOSE_0 — THE CAMPAIGN CLOSES ON A WITNESS

*Handoff · authored 2026-09-08 · Claude → CC · Jean holds build, visual, merge, deploy.*
*Filed to `docs/HANDOFFS/CLOSE_0.md` while open; the directory dies with the campaign.*

## THE IDEA, IN ONE PARAGRAPH

The campaign's last lesson was that three rounds of witnesses parsed the dist tool, grepped
the dist tool, and never RAN the dist tool on a tree that had never run it — the one check
that would have caught SETTLE_0's blocker before it committed. CC's critic said so; it was
right. This round makes that check a gate: `tools/gates/dist_gate/run.py` copies the tree to
scratch (no `dist/`), fabricates the products the way the audit agents did (the package IS
the shader, the glue declares its size), runs `web_dist.py` and `--check`, and requires
exit 0, WROTE, and the first-visit block above it. Run against the tool as SETTLE_0 left it,
the gate is RED with six failures; against the tree now, GREEN. It joins CLAUDE.md's table.
Alongside: CC's four cosmetic findings retire (the products' banner that re-introduced the
membership drift it was written to fix; three stale "four"s; a pre-existing miscount in the
JPEG walk), and P19 names who deletes the scratch ref — the agent's surface could push but
not delete, so Jean does, at the merge, and the report says so until it is gone.

## AUTHORITY

Base: **`origin/claude/important-handoffs-9v1wus` at `4513257c`** (SETTLE_1 landed). Lands
as follow-up commits on that branch; the branch dies on Jean's merge. HALT (P17, scoped
P15): unreachable file; stale authority. Else DEFAULT-AND-FLAG. Count **1** everywhere. LF
only. P19 governs the push.

| file | blob at base |
| --- | --- |
| `tools/web_dist.py` | `19fce45bac01706b16f653dd75d3c9a837622b7c` |
| `docs/PROCESS_LAWS.md` | `991aca016a5e1c2c55e03ce2cb598e35c7f16730` |
| `CLAUDE.md` | `d1f622d6b880744465148668aead3a243d947f0a` |
| `tools/gates/dist_gate/run.py` | NEW — must not exist |

**REHEARSED.** All eight FINDs at count 1; the tool parses; products gate GREEN; the dist
gate GREEN on the tree and RED (six failures) on SETTLE_0's tool restored as a sidecar
(P18) — a gate that can lose. The tree is untouched by the gate: it works in scratch
directories and deletes them.

## THE RULINGS

1. **The dist tool has a witness that runs it.** Fresh tree, fabricated products, end to end;
   `--check` writes nothing and records nothing. A few seconds per cascade.
2. **The gate fabricates what the tool's own checks demand:** the package byte-identical
   to `world.wgsl` (the purity refusal), a glue that declares `remote_package_size` (the
   PAIR_0 witness), noise for the wasms. Nothing else is special-cased.
3. **The membership banner is exact:** `index.html`, `organ_panel.js` and
   `darkroom_worker.js` are tracked source; the rest are build output. (the one `grep -i "the four"` hit left, line 965, is
   DOORS_0's "THE FOURTH REFUSAL" — a refusal, not a file.)
4. **P19 names who deletes the scratch ref.** Whoever holds the permission; the report
   names the ref as outstanding until it is gone. Today: `wip/important-handoffs-9v1wus`
   is Jean's, one click, at the merge.

## UNITS

| unit | what | files | commit |
| --- | --- | --- | --- |
| U1 | the prose | `tools/web_dist.py` | 1 |
| U2 | P19 | `docs/PROCESS_LAWS.md` | 1 |
| U3 | the gate; its row | `tools/gates/dist_gate/run.py` (new), `CLAUDE.md` | 1 |
| U4 | OPEN.md: the campaign closes; push per P19 | `docs/OPEN.md` | 1 |

## U0 — PREFLIGHT
```
git fetch origin && git checkout claude/important-handoffs-9v1wus && git pull --ff-only
git ls-tree HEAD tools/web_dist.py docs/PROCESS_LAWS.md CLAUDE.md      # the three blobs
test ! -e tools/gates/dist_gate && echo absent
```

## U1 — THE PROSE

### U1.1 — EXHIBIT_0's banner stops counting (`tools/web_dist.py`)

FIND
```
# EXHIBIT_0 U1 — THE EXHIBITION LEAVES THE BUNDLE. The four build files
# above are THE PROGRAM: rebuilt only when the program changes. The
```
REPLACE
```
# EXHIBIT_0 U1 — THE EXHIBITION LEAVES THE BUNDLE. The build files
# above (the program's and the darkroom's) are THE PROGRAM: rebuilt only
# when the program changes. The
```

### U1.2 — the products' banner names the membership correctly (CC: organ_panel.js is tracked source too) (`tools/web_dist.py`)

FIND
```
# index.html is SOURCE (tracked); the rest are build output (.gitignore'd)
# or the darkroom's worker script (tracked). All ship — but index.html is the only one that is
```
REPLACE
```
# index.html, organ_panel.js and darkroom_worker.js are SOURCE (tracked);
# the rest are build output (.gitignore'd). All ship — but index.html is the only one that is
```

### U1.3 — a pre-existing miscount in the JPEG walk's docstring (`tools/web_dist.py`)

FIND
```
    except the four that are not frame headers (DHT C4, JPG C8, DAC CC)
```
REPLACE
```
    except the three that are not frame headers (DHT C4, JPG C8, DAC CC)
```

### U1.4 — the exhibition's banner stops counting (`tools/web_dist.py`)

FIND
```
    # These bytes are not in the four files above and never will be
```
REPLACE
```
    # These bytes are not in the program's files above and never will be
```

### U1.5 — the cache rule's banner names the list, not a number (`tools/web_dist.py`)

FIND
```
    # index always names fresh keys — and the four versioned artifacts
    # gain `immutable` beneath it.
```
REPLACE
```
    # index always names fresh keys — and the versioned artifacts
    # (IMMUTABLE_PATHS) gain `immutable` beneath it.
```

### U1.6 — and its second count, with the darkroom's fetches named (`tools/web_dist.py`)

FIND
```
    # included. Every one of these four is fetched as `<path>?v=<build
    # id>` (index.html's script tags for the two .js, Module.locateFile
    # for the .wasm and the .data), and the build id is
```
REPLACE
```
    # included. Every one of these is fetched as `<path>?v=<build
    # id>` (index.html's script tags for the program's .js and the
    # panel's, Module.locateFile for the .wasm and the .data, the worker's
    # URL, importScripts and locateFile for the darkroom's three), and the
    # build id is
```


### WITNESS — U1
```
python3 -c "import ast; ast.parse(open('tools/web_dist.py').read())"
grep -c -i "the four" tools/web_dist.py    # 1 — "THE FOURTH REFUSAL", DOORS_0, not a file count
```
Commit: `CLOSE_0 U1 — the last counts retire; the membership banner is exact`

## U2 — P19

### U2.1 — P19 names who deletes the scratch ref (`docs/PROCESS_LAWS.md`)

FIND
```
itself waits for the verdict, and the scratch ref is deleted when the real
push lands. Work safe, history clean, the hook's nagging is the price, and it
is said out loud in the report rather than reasoned around.
```
REPLACE
```
itself waits for the verdict, and the scratch ref is deleted when the real
push lands — by whoever holds the permission: the agent if the remote lets
it, else Jean at the merge (CLOSE_0: the agent's surface could push but not
delete). Until it is gone, the report names it as outstanding. Work safe,
history clean, the hook's nagging is the price, and it is said out loud in
the report rather than reasoned around.
```


Commit: `CLOSE_0 U2 — P19 names who deletes the scratch ref`

## U3 — THE GATE

### NEW FILE `tools/gates/dist_gate/run.py` — verbatim
```
#!/usr/bin/env python3
"""
DIST GATE (CLOSE_0) — the dist tool runs, end to end, on a tree that has
never run it.

SETTLE_0 gave web_dist.py a check that ran two hundred lines before the
thing it checked existed; on every fresh clone the tool refused before it
could create dist/. Three campaigns' witnesses parsed the tool, grepped the
tool, and never RAN the tool on a tree without dist/ — the one thing that
would have caught it. This gate does that thing, every cascade:

  1. copies the tree (minus .git, out/, dist/, node_modules) to a scratch
     directory, so dist/ does not exist there;
  2. fabricates the build products the way the audit agents did — the
     package IS the shader (the tool's purity check demands it), the glue
     declares the package size (the PAIR_0 check reads it), the wasms are
     noise;
  3. runs `web_dist.py`: exit 0, "WROTE", and the FIRST VISIT block
     printed above WROTE, all required;
  4. runs `web_dist.py --check` on a second fresh copy: exit 0 and NO
     first-visit line (--check writes nothing, so it records nothing);
  5. deletes the scratch directories, whatever happened.

The tree is never touched. Pillow's presence changes the tool's arm, not
its exit; the tool says which arm it took.
"""
import os, re, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
SHADER = os.path.join("src", "cartridges", "the_board", "realization", "world.wgsl")
SKIP = {".git", "out", "dist", "node_modules", "__pycache__"}

def scratch_tree():
    tmp = tempfile.mkdtemp(prefix="t7_dist_gate_")
    dst = os.path.join(tmp, "tree")
    shutil.copytree(ROOT, dst, ignore=lambda d, names: [n for n in names if n in SKIP])
    web = os.path.join(dst, "web")
    shader = open(os.path.join(dst, SHADER), "rb").read()
    with open(os.path.join(web, "the_board.data"), "wb") as fh:
        fh.write(shader)                                    # the package is the shader, byte for byte
    with open(os.path.join(web, "the_board.js"), "w", encoding="utf-8") as fh:
        fh.write("var REMOTE_PACKAGE_SIZE = %d;\nvar remote_package_size = %d;\n" % (len(shader), len(shader)))
    for noise in ("the_board.wasm", "darkroom.js", "darkroom.wasm"):
        with open(os.path.join(web, noise), "wb") as fh:
            fh.write(os.urandom(4096))
    return tmp, dst

def run_tool(dst, *args):
    r = subprocess.run([sys.executable, os.path.join("tools", "web_dist.py"), *args],
                       cwd=dst, capture_output=True, text=True)
    return r.returncode, r.stdout + r.stderr

def main():
    fails = 0
    def row(ok, what):
        nonlocal fails
        print("  [%s] %s" % ("PASS" if ok else "FAIL", what))
        if not ok:
            fails += 1

    tmp, dst = scratch_tree()
    try:
        row(not os.path.exists(os.path.join(dst, "dist")), "the scratch tree has no dist/ — a fresh clone")
        code, out = run_tool(dst)
        row(code == 0, "web_dist.py exits 0 on a fresh tree (exit %d)" % code)
        wrote = out.find("WROTE ")
        first = out.find("FIRST VISIT (the boot set, as written)")
        row(wrote >= 0, "the tool reports WROTE")
        row(first >= 0, "the first-visit block is printed")
        row(0 <= first < wrote, "the first-visit block sits above WROTE (dist is complete when it reads)")
        m = re.search(r"WROTE .*\((\d+) files\)", out)
        row(m is not None and int(m.group(1)) > 0, "WROTE names a file count (%s)" % (m.group(1) if m else "none"))
        if code != 0:
            print("  --- the tool's last lines ---")
            print("\n".join("  " + l for l in out.strip().splitlines()[-8:]))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    tmp, dst = scratch_tree()
    try:
        code, out = run_tool(dst, "--check")
        row(code == 0, "web_dist.py --check exits 0 on a fresh tree (exit %d)" % code)
        row("FIRST VISIT" not in out, "--check prints no first-visit line (it writes nothing)")
        row(not os.path.exists(os.path.join(dst, "dist")), "--check left no dist/ behind")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    print("DIST GATE: %s" % ("GREEN — the dist tool runs on a tree that has never run it" if fails == 0 else "RED — %d failure(s)" % fails))
    return 0 if fails == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
```

### U3.1 — the dist gate joins the table (`CLAUDE.md`)

FIND
```
| products gate | `python3 tools/gates/products_gate/run.py` | every home that names a build product — `.gitignore`, the build presets, `web_dist.py`, `collection_gate.py` — agrees with `CMakeLists.txt` | GREEN |
```
REPLACE
```
| products gate | `python3 tools/gates/products_gate/run.py` | every home that names a build product — `.gitignore`, the build presets, `web_dist.py`, `collection_gate.py` — agrees with `CMakeLists.txt` | GREEN |
| dist gate | `python3 tools/gates/dist_gate/run.py` | `web_dist.py` runs end to end on a scratch copy of the tree that has never run it (no `dist/`, fabricated products): exit 0, WROTE, the first-visit block above it; `--check` writes nothing | GREEN |
```


### WITNESS — U3
```
python3 tools/gates/dist_gate/run.py       # GREEN — nine PASS rows
# and that it can lose (P18 sidecar, never HEAD):
git show 280143b0:tools/web_dist.py > /tmp/settle0_tool.py && cp tools/web_dist.py /tmp/keep.py && cp /tmp/settle0_tool.py tools/web_dist.py
python3 tools/gates/dist_gate/run.py       # RED — 6 failure(s)
cp /tmp/keep.py tools/web_dist.py && git status --short   # clean
```
Commit: `CLOSE_0 U3 — the dist gate: web_dist.py runs end to end on a tree that has never run it; joins the table`

## U4 — THE CAMPAIGN CLOSES
`docs/OPEN.md`, top of the register:
```
## CLOSE_0 — THE CAMPAIGN CLOSES ON A WITNESS (on `claude/important-handoffs-9v1wus`)

The dist gate runs web_dist.py end to end on a fresh scratch tree every cascade — the
check three campaigns' witnesses never ran. The last counts retire; P19 names who deletes
the scratch ref (Jean, at the merge: `wip/important-handoffs-9v1wus` is outstanding).

The performance campaign (POSTCARD_0, PLATE_0, RAZOR_0, MIP_0, GATHER_0, SHUTTER_0,
DARKROOM_0/1, RIG_0, PRODUCTS_0, ATMOS_1, SETTLE_0/1, CLOSE_0) is closed. What it leaves
open, priced, in their own entries: the scrolling shadow cache, instance culling for the
table, the two-walls split and compressed textures, the browser-decode road (CLOSED in the
program's own language — no external-image copy in the vendored surface), the meter's
three windows on the laptop, and the hang's split if the [METER] darkroom row asks.
```
Commit: `CLOSE_0 U4 — the campaign closes`. Push per P19: `wip/` first if the hook asks, the
branch after the verdict.

## JEAN'S GATES
1. Merge `claude/important-handoffs-9v1wus` to master when the verdict is CLEAR; delete
   `wip/important-handoffs-9v1wus` in the GitHub UI — the one P19 step the agent could not
   perform.
2. `python tools\gates\dist_gate\run.py` once on your machine: GREEN, nine rows, a few
   seconds — the check that would have saved SETTLE_0, now standing.
