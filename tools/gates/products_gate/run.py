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
