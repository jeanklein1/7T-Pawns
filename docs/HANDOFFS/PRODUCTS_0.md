# PRODUCTS_0 — EVERY HOME THAT NAMES A BUILD PRODUCT AGREES WITH THE BUILD

*Handoff · authored 2026-09-07 · Claude → CC · Jean holds build, visual, merge, deploy.*
*Filed to `docs/HANDOFFS/PRODUCTS_0.md` while open; the directory dies with the campaign.*

## THE IDEA, IN ONE PARAGRAPH

DARKROOM_1 added a target whose products land in `web/`, and in one week three homes that
enumerated the old products by name were found stale one at a time: `CMakePresets.json`
built half the site (U3b), `.gitignore` let 104 KB of build output into the tree (CC's
catch), and — this round's own census found — `web_dist.py`'s BOOT_SET stopped recording a
first-visit cost and `collection_gate.py`'s FORBIDDEN no longer named everything that is the
engine's. Lists that name one fact in many homes drift on the commit that changes the fact.
The tree's answer to every other enumeration is a census with witnesses; this round gives
the products one. `CMakeLists.txt` is the authority (every target linking into `web/`);
`tools/gates/products_gate/run.py` reads it and witnesses each home. Run on the base it reads
RED with exactly the four misses named above — the two CC fixed already pass — and GREEN
after U1.

## AUTHORITY

Base: `master` at HEAD `5d655aee`, fetched 2026-09-07. Rides `claude/products-0` (P12: a
gate and three lists; no runtime change). HALT (P17, scoped P15): unreachable file; stale
authority. Else DEFAULT-AND-FLAG. Count **1** everywhere. LF only. Disjoint from ATMOS_1
except `web_dist.py`'s neighbour edits are here and index.html is there; either order.

| file | blob at base |
| --- | --- |
| `tools/web_dist.py` | `991b2c13c661ac354a2fd8a3f8ec7e4a7c61ff08` |
| `tools/gates/collection_gate.py` | `4de856530d270e641a6975f45bae7af5026dbd09` |
| `tools/gates/products_gate/run.py` | NEW — must not exist |

**REHEARSED.** The gate ran on the base (RED, four misses) and on the branch (GREEN); both
tools parse; the shell gate GREEN (untouched here).

## THE RULINGS

1. **The build is the authority for what the build produces.** A target with
   `RUNTIME_OUTPUT_DIRECTORY web/` and `SUFFIX .js` yields `<name>.js` and `<name>.wasm`,
   plus `<name>.data` when its link carries `--preload-file`. The gate parses that; no list
   of products is written by hand anywhere as truth.
2. **Four homes are witnesses, not authors:** `.gitignore` (`web/<product>`),
   `CMakePresets.json` (every web build preset names every web target), `web_dist.py`
   (ARTIFACTS, IMMUTABLE_PATHS, and BOOT_SET unless `NOT_AT_BOOT` excuses a lazy product —
   the one place laziness is named), `collection_gate.py` (FORBIDDEN).
3. **The gate joins CLAUDE.md's table** and runs in every U-last cascade from now on.
4. **`darkroom_worker.js` is source, tracked, and a boot fetch** — in BOOT_SET and
   FORBIDDEN by hand (it is not a product, so the gate does not require it); the products
   the gate requires are the two the build makes.

## UNITS

| unit | what | files | commit |
| --- | --- | --- | --- |
| U1 | the gate; the four homes | `products_gate/run.py` (new), `web_dist.py`, `collection_gate.py` | 1 |
| U2 | CLAUDE.md's gate table; OPEN.md; push | `CLAUDE.md`, `docs/OPEN.md` | 1 |

## U0 — PREFLIGHT
```
git fetch origin master && git checkout -b claude/products-0 origin/master
test ! -e tools/gates/products_gate && echo absent
git ls-tree HEAD tools/web_dist.py tools/gates/collection_gate.py
```

## U1 — THE GATE AND THE FOUR HOMES

### NEW FILE `tools/gates/products_gate/run.py` — verbatim
```
#!/usr/bin/env python3
"""
PRODUCTS GATE (PRODUCTS_0) — every home that names a build product agrees
with the build.

The rule this enforces was learned three times in one week: a new target
whose products land in web/ was added (DARKROOM_1), and .gitignore, the
build presets and the dist tool each still enumerated the OLD products by
name — build output got committed, a preset built half the site, a first-
visit cost stopped being recorded. Lists that name the same fact in many
homes drift the moment the fact changes; the fix is not a checklist in a
document, it is a census with witnesses (CLAUDE.md's rule for every other
enumeration in this tree).

THE AUTHORITY IS CMakeLists.txt. Every target with RUNTIME_OUTPUT_DIRECTORY
web/ and SUFFIX .js produces <name>.js and <name>.wasm, plus <name>.data if
its link options carry --preload-file. The gate reads those targets and
then witnesses each HOME:

  .gitignore              web/<product> present for every product
  CMakePresets.json       every build preset whose configurePreset is a
                          web preset names every web target
  tools/web_dist.py       ARTIFACTS, IMMUTABLE_PATHS contain every product;
                          BOOT_SET contains every product unless it is in
                          NOT_AT_BOOT (the one place a lazy product is named)
  tools/gates/collection_gate.py
                          FORBIDDEN names every product (the collection page
                          must never reference the engine's files)

Prints one row per witness. Exits 1 on any miss. No arguments.
"""
import json, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))   # tools/gates/products_gate -> the tree

def read(p):
    with open(os.path.join(ROOT, p), encoding="utf-8") as fh:
        return fh.read()

def web_targets(cmake):
    """{target: [products]} for every target that links into web/."""
    out = {}
    for m in re.finditer(r'set_target_properties\((\w+)\s+PROPERTIES(.*?)\)\s*\n', cmake, re.S):
        target, body = m.group(1), m.group(2)
        if '/web"' not in body and "/web'" not in body:
            continue
        name = re.search(r'OUTPUT_NAME\s+"([^"]+)"', body)
        suffix = re.search(r'SUFFIX\s+"([^"]+)"', body)
        if not name or not suffix or suffix.group(1) != ".js":
            continue
        n = name.group(1)
        products = [n + ".js", n + ".wasm"]
        # a target's link options sit in its own target_link_options block
        lo = re.search(r'target_link_options\(' + re.escape(target) + r'\s+PRIVATE(.*?)\n\s*\)', cmake, re.S)
        if lo and "--preload-file" in lo.group(1):
            products.append(n + ".data")
        out[target] = products
    return out

def py_list(src, name):
    m = re.search(r'^' + name + r'\s*=\s*\[(.*?)\]', src, re.S | re.M)
    if not m:
        return None
    return re.findall(r'"([^"]+)"', m.group(1))

def py_tuple(src, name):
    m = re.search(r'^' + name + r'\s*=\s*\((.*?)\)', src, re.S | re.M)
    if not m:
        return None
    return re.findall(r'"([^"]+)"', m.group(1))

def main():
    cmake = read("CMakeLists.txt")
    targets = web_targets(cmake)
    if not targets:
        print("PRODUCTS GATE: STOP — no web target found in CMakeLists.txt (the parser is stale, not the tree)")
        return 1
    products = [p for ps in targets.values() for p in ps]
    print("PRODUCTS GATE — the build's web products: " + ", ".join(
        "%s -> %s" % (t, " ".join(ps)) for t, ps in targets.items()))
    misses = 0
    def row(ok, what):
        nonlocal misses
        print("  [%s] %s" % ("PASS" if ok else "MISS", what))
        if not ok:
            misses += 1

    gi = read(".gitignore").splitlines()
    for p in products:
        row(("web/" + p) in gi, ".gitignore names web/%s" % p)

    presets = json.load(open(os.path.join(ROOT, "CMakePresets.json"), encoding="utf-8"))
    web_cfg = {c["name"] for c in presets.get("configurePresets", []) if c["name"].startswith("the-board-web")}
    for b in presets.get("buildPresets", []):
        if b.get("configurePreset") not in web_cfg:
            continue
        named = set(b.get("targets") or [])
        for t in targets:
            row(t in named, "CMakePresets.json build preset %s names target %s" % (b["name"], t))

    wd = read("tools/web_dist.py")
    for lst in ("ARTIFACTS", "IMMUTABLE_PATHS"):
        got = py_list(wd, lst)
        for p in products:
            row(got is not None and p in got, "web_dist.py %s names %s" % (lst, p))
    boot = py_list(wd, "BOOT_SET") or []
    lazy = py_list(wd, "NOT_AT_BOOT") or []
    for p in products:
        row(p in boot or p in lazy, "web_dist.py BOOT_SET names %s (or NOT_AT_BOOT excuses it)" % p)

    cg = read("tools/gates/collection_gate.py")
    forb = py_tuple(cg, "FORBIDDEN") or []
    for p in products:
        row(p in forb, "collection_gate.py FORBIDDEN names %s" % p)

    print("PRODUCTS GATE: %s" % ("GREEN — every home agrees with the build" if misses == 0 else "RED — %d miss(es)" % misses))
    return 0 if misses == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
```

### U1.1 — the first-visit cost records the darkroom (`tools/web_dist.py`)

FIND
```
BOOT_SET = ["index.html", "the_board.js", "the_board.wasm", "the_board.data",
            "veil_poster.jpg", EXHIBITION_JSON]
```
REPLACE
```
BOOT_SET = ["index.html", "the_board.js", "the_board.wasm", "the_board.data",
            "darkroom_worker.js", "darkroom.js", "darkroom.wasm",   # PRODUCTS_0 — the darkroom opens beside the program (DARKROOM_1); a first-visit cost, recorded
            "veil_poster.jpg", EXHIBITION_JSON]
```

### U1.2 — the stale-build advice names every product (`tools/web_dist.py`)

FIND
```
        print("    del web\\the_board.js web\\the_board.wasm web\\the_board.data")
```
REPLACE
```
        print("    del web\\the_board.js web\\the_board.wasm web\\the_board.data web\\darkroom.js web\\darkroom.wasm")
```

### U1.3 — the collection may not reference the darkroom either (`tools/gates/collection_gate.py`)

FIND
```
FORBIDDEN = (
    "the_board.js", "the_board.wasm", "the_board.data",
```
REPLACE
```
FORBIDDEN = (
    "the_board.js", "the_board.wasm", "the_board.data",
    "darkroom.js", "darkroom.wasm", "darkroom_worker.js",   # PRODUCTS_0 — the darkroom is the engine's too
```


### WITNESS — U1
```
python3 tools/gates/products_gate/run.py      # GREEN — every home agrees with the build (RED on the base with the four misses, if you want the before)
python3 -c "import ast; ast.parse(open('tools/web_dist.py').read()); ast.parse(open('tools/gates/collection_gate.py').read())"
python3 tools/gates/collection_gate.py         # PASS (needs dist/collection; if absent, the parse above is the witness)
```
Commit: `PRODUCTS_0 U1 — the products gate: CMakeLists is the authority, four homes are witnesses; BOOT_SET records the darkroom, FORBIDDEN names it`

## U2 — THE TABLE, THE REGISTER, THE PUSH
Add `python3 tools/gates/products_gate/run.py` to CLAUDE.md's gate table beside the shell
gate (one row, same shape as its neighbours). `docs/OPEN.md`, top of the register:
```
## PRODUCTS_0 — THE PRODUCTS GATE (on `claude/products-0`; Jean's gates open)

The build's web products are read from CMakeLists.txt and witnessed in .gitignore, the
build presets, web_dist.py (ARTIFACTS, IMMUTABLE_PATHS, BOOT_SET or NOT_AT_BOOT) and
collection_gate.py. Found RED on the base with the four homes DARKROOM_1 had not visited;
GREEN after. Joins the cascade.

| ruling | where it lives now |
|---|---|
| The build is the authority for its products | `tools/gates/products_gate/run.py` |
| Four homes are witnesses, not authors | the gate's rows |
| Laziness is named once | `NOT_AT_BOOT` in `web_dist.py` (empty today) |
```
Commit: `PRODUCTS_0 U2 — the gate joins the table; OPEN.md`. Push `claude/products-0`.

## JEAN'S GATES
1. Your deploy recipe's first line grows: `del /q web\the_board.js web\the_board.wasm
   web\the_board.data web\darkroom.js web\darkroom.wasm` — a failed darkroom link must
   never ship last week's darkroom. (The dist tool's own advice line now says the same.)
2. `python tools\web_dist.py` prints the first-visit cost with the darkroom's three files
   in it — recorded, as SHIP_0 U4 orders, not optimized.
