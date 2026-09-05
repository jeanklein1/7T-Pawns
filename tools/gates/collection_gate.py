#!/usr/bin/env python3
# ─── tools/gates/collection_gate.py ──────────────────────────────
#
# THE COLLECTION IS ENGINE-AGNOSTIC, BY LAW. A visitor whose browser
# cannot run the world — Firefox held at the fallback card, an iPad on
# a bad day — still gets every painting. That only stays true if no
# engine artifact can creep into the page's dependency closure, so the
# rule is enforced here rather than remembered.
#
#   python tools/gates/collection_gate.py            # gates dist/collection
#
# Two checks:
#   1. No text file under dist/collection mentions an engine artifact
#      or a WebGPU entry point.
#   2. Every local src/href/srcset the page names exists on disk —
#      a manifest that lies is the failure the exhibition path already
#      refuses, restated for this tree.

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
DIST = os.path.join(ROOT, "dist", "collection")

FORBIDDEN = (
    "the_board.js", "the_board.wasm", "the_board.data",
    "organ_panel.js", "navigator.gpu", "requestAdapter",
    "emscripten", "EMSCRIPTEN",
)

TEXT_EXTS = (".html", ".css", ".js", ".json", ".svg", ".txt", ".fragment")


def fail(msgs):
    print("collection_gate: FAIL")
    for m in msgs:
        print("  " + m)
    sys.exit(1)


def main():
    if not os.path.isdir(DIST):
        fail(["no %s — run tools/collection_dist.py first" % DIST])

    bad = []

    # 1 — no engine artifact in the closure
    for dirpath, _, files in os.walk(DIST):
        for f in files:
            if not f.lower().endswith(TEXT_EXTS):
                continue
            path = os.path.join(dirpath, f)
            with open(path, encoding="utf-8", errors="replace") as fh:
                text = fh.read()
            for token in FORBIDDEN:
                if token in text:
                    rel = os.path.relpath(path, DIST)
                    bad.append("%s mentions %r" % (rel, token))

    # 2 — every local reference the page makes is a file that exists
    index = os.path.join(DIST, "index.html")
    with open(index, encoding="utf-8") as fh:
        html = fh.read()

    refs = set()
    for m in re.finditer(r'(?:src|href)="([^"]+)"', html):
        refs.add(m.group(1))
    for m in re.finditer(r'srcset="([^"]+)"', html):
        for part in m.group(1).split(","):
            refs.add(part.strip().split(" ")[0])

    for ref in sorted(refs):
        if re.match(r"^(https?:|data:|#|\.\./|mailto:)", ref):
            continue
        if not os.path.isfile(os.path.join(DIST, ref)):
            bad.append("index.html names %s — no such file in dist/collection" % ref)

    if bad:
        fail(bad)

    print("collection_gate: PASS  (%d local refs, all present; no engine tokens)"
          % len([r for r in refs if not re.match(r"^(https?:|data:|#|\.\./|mailto:)", r)]))


if __name__ == "__main__":
    main()
