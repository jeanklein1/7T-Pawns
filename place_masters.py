#!/usr/bin/env python3
# place_masters.py — ONE-SHOT BOOTSTRAP. Drop this file at the repo root
# (C:\dev\7t\place_masters.py) and run it once, BEFORE renaming the set
# folders:
#
#   python place_masters.py
#
# It copies the tracked masters in assets/paintings/ into the five
# assets/collection/<set>/ folders by the number range each folder's name
# declares (a_1-14 -> 1..14, ... , e_unfiled takes whatever no range
# claims), and copies the heroes assets/about/site.json names into
# assets/about/. Copies, never moves: the engine's exhibition keeps every
# file it had. Masters are gitignored, so nothing here reaches git.
# Already-placed files are skipped, so running it twice changes nothing.
#
# The number rule is the exhibition's own (tools/web_dist.py,
# extract_number): the stem after the first '_', leading digits only.

import json
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PAINTINGS = os.path.join(HERE, "assets", "paintings")
COLLECTION = os.path.join(HERE, "assets", "collection")
ABOUT = os.path.join(HERE, "assets", "about")
EXTS = (".jpg", ".jpeg")


def number(name):
    stem = os.path.splitext(name)[0]
    pos = stem.find("_")
    if pos == -1:
        return None
    m = re.match(r"\s*([+-]?\d+)", stem[pos + 1:])
    return int(m.group(1)) if m else None


def main():
    for folder in (PAINTINGS, COLLECTION, ABOUT):
        if not os.path.isdir(folder):
            print("REFUSE  %s is not a folder — run this from the repo root" % folder)
            return 1

    ranges, catchall = [], None
    for d in sorted(os.listdir(COLLECTION)):
        if not os.path.isdir(os.path.join(COLLECTION, d)):
            continue
        m = re.search(r"_(\d+)-(\d+)$", d)
        if m:
            ranges.append((d, int(m.group(1)), int(m.group(2))))
        elif d.endswith("_unfiled"):
            catchall = d
    if not ranges or catchall is None:
        print("REFUSE  the set folders no longer carry number ranges — this")
        print("        bootstrap only knows the shape a_1-14 ... e_unfiled.")
        return 1

    placed = {d: 0 for d, _, _ in ranges}
    placed[catchall] = 0
    skipped = 0
    for f in sorted(os.listdir(PAINTINGS)):
        if not f.lower().endswith(EXTS):
            continue
        n = number(f)
        if n is None:
            continue
        dest = catchall
        for d, lo, hi in ranges:
            if lo <= n <= hi:
                dest = d
                break
        target = os.path.join(COLLECTION, dest, f)
        if os.path.exists(target):
            skipped += 1
            continue
        shutil.copy2(os.path.join(PAINTINGS, f), target)
        placed[dest] += 1

    print("collection masters placed")
    for d in sorted(placed):
        print("  %-14s %3d" % (d, placed[d]))
    if skipped:
        print("  (%d already in place, skipped)" % skipped)

    with open(os.path.join(ABOUT, "site.json"), "r", encoding="utf-8") as fh:
        site = json.load(fh)
    heroes = [e["file"] for k in ("wide", "tall") for e in site["hero"][k]]
    print("heroes site.json names")
    missing = 0
    for h in heroes:
        src = os.path.join(PAINTINGS, h)
        dst = os.path.join(ABOUT, h)
        if os.path.exists(dst):
            print("  %-22s already in assets/about" % h)
        elif os.path.isfile(src):
            shutil.copy2(src, dst)
            print("  %-22s copied" % h)
        else:
            print("  %-22s MISSING in assets/paintings — pick another in site.json" % h)
            missing += 1

    print("")
    print("next, in this order:")
    print("  python tools/collection_dist.py")
    print("  python tools/about_dist.py")
    print("  python tools/gates/collection_gate.py")
    print("  python tools/web_dist.py")
    print("  wrangler pages deploy dist --project-name 7t")
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
