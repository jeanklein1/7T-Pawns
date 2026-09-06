#!/usr/bin/env python3
# ─── tools/dist.py — THE ONE DOOR TO dist/ (DOORS_2) ─────────────────
#
# The site is two agnostic halves and one act of putting them together.
# The halves stay siblings — collection_dist.py, about_dist.py and
# web_dist.py know nothing of each other, and collection_gate.py keeps the
# collection engine-free — and the ORDER lives here and nowhere else: the
# collection first (its _headers fragment feeds the root), then about (its
# hero rides beside it), then the gate, then web_dist LAST (it deletes only
# the engine's own names and folds the fragment). Stops at the first stage
# that fails, in that stage's own words.
#
#   cmake --build --preset the-board-web     # the engine, first (Jean)
#   python tools/dist.py                     # everything into dist/
#   npx wrangler pages deploy dist --project-name=7t   # deploy stays a hand
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

STAGES = (
    ("collection", "tools/collection_dist.py"),
    ("about",      "tools/about_dist.py"),
    ("gate",       "tools/gates/collection_gate.py"),
    ("web",        "tools/web_dist.py"),
)


def main():
    for name, script in STAGES:
        print("\n== dist: %s  (%s) ==" % (name, script), flush=True)
        r = subprocess.run([sys.executable, os.path.join(ROOT, script)], cwd=ROOT)
        if r.returncode != 0:
            print("\ndist: STOPPED at %s (exit %d) — nothing after it ran." % (name, r.returncode))
            return r.returncode
    print("\ndist: complete — deploy with: npx wrangler pages deploy dist --project-name=7t")
    return 0


if __name__ == "__main__":
    sys.exit(main())
