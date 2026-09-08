#!/usr/bin/env python3
# ─── tools/collection_dist.py ────────────────────────────────────
#
# The collection's own pipeline. Deliberately a SIBLING of web_dist.py,
# not an extension of it: the exhibition path is guarded by asserts
# (assert_exhibition_written refuses strangers in dist/paintings) and
# nothing here may lean on that machinery or weaken it. Two pipelines,
# one folder convention, zero coupling.
#
#   python tools/collection_dist.py               # build dist/gallery/
#   python tools/collection_dist.py --check       # inventory only
#   python tools/collection_dist.py --preview F   # one self-contained file
#
# THE SOURCE OF TRUTH is a folder tree:
#
#   assets/collection/<folder>/PAINTING_<n>.jpg   the works
#   assets/collection/<folder>/PAINTING_<n>.txt   optional: ONE LINE, the
#       work's info as Jean writes it — "oil on canvas · 80 × 60 cm · 2024".
#       Shown behind the lightbox's `info` word, nowhere else. It sits
#       beside its master and moves with it (HOUSE_0); the masters are
#       gitignored and these are not, so the info is versioned.
#   assets/collection/<folder>/set.json           optional, all fields optional:
#       { "label": "…", "note": "…", "paper": true, "featured": [90, 112] }
#
# "featured" works break the grid and take the full width alone — the
# page's pacing is authored here, one list per set, not derived.
#
# Folders sort by name; works sort by the number in the filename — the
# same key gallery.hpp uses, restated here. A work with no info still
# ships; a missing sidecar must never be able to drop a painting. THE
# FOLDER IS THE TRUTH: a removed master leaves the page, and anything
# that still names it — a sidecar, a featured number — is a WARNING
# line, never a refused build (HOUSE_0). No number is shown on the page.
#
# THE PAGE IS WRITTEN, NOT FETCHED. web/gallery/index.html is source
# with two placeholder regions; this script fills them with static
# markup and writes the result to dist/gallery/index.html. No
# runtime manifest, no client templating: the HTML is the manifest,
# which is also what lets the page work with JavaScript disabled.
#
# DERIVATIVE NAMES CARRY A CONTENT HASH. The _headers rule marks
# /gallery/* immutable for a year, which is only safe if replacing a
# master changes the URL. Without the hash, re-exporting no. 107 would
# leave every returning visitor looking at last year's scan.
#
# NEVER UPSCALES. Sources today are 1280/1600-edge exports; the ladder
# emits what the source can honestly give and stops. When real masters
# arrive, raising FULL_EDGE is the whole change.

import argparse
import base64
import hashlib
import io
import json
import os
import re
import shutil
import sys
import routes   # DOORS_0 — the sandwich's one renderer

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SRC = os.path.join(ROOT, "assets", "collection")
TEMPLATE = os.path.join(ROOT, "web", "gallery", "index.html")
DIST = os.path.join(ROOT, "dist", "gallery")

WORK_EXTS = (".jpg", ".jpeg", ".png")

# ── the resolution ladder ────────────────────────────────────────
# 640 covers the grid at every sane tile width and pixel ratio; 1280
# covers the opened work on a laptop; FULL_EDGE is the opened work on a
# large high-density display. Sources are re-encoded at their own size
# when they cannot reach a rung — never stretched to it.
LADDER = (640, 1280)
FULL_EDGE = 2000
JPEG_Q = 80
AVIF_Q = 57
AVIF_SPEED = 7

# What the grid tells the browser a tile will roughly occupy, so it can
# pick a rung before layout runs. Approximate on purpose: justified
# rows vary, and the penalty for a miss is one rung, not a wrong image.
SIZES = "(max-width: 620px) 94vw, (max-width: 1500px) 30vw, 24vw"

# DOORS_0 — THE PEEK. How many works the engine's menu shows of the
# collection: featured first, in set order, then the first work of each
# set round-robin. The pacing decision stays where it is authored
# (set.json's "featured"); this only counts.
PEEK_COUNT = 12

PREVIEW_EDGE = 900
PREVIEW_Q = 62


def say(msg=""):
    print(msg)


def extract_number(name):
    """gallery.hpp's sort key, restated (web_dist.py has the same twin)."""
    stem = os.path.splitext(name)[0]
    pos = stem.find("_")
    if pos == -1 or pos + 1 >= len(stem):
        return 0
    m = re.match(r"\s*([+-]?\d+)", stem[pos + 1:])
    return int(m.group(1)) if m else 0


def slugify(name):
    s = re.sub(r"[^a-z0-9]+", "-", name.lower()).strip("-")
    return s or "set"


def esc(s):
    return (str(s).replace("&", "&amp;").replace("<", "&lt;")
            .replace(">", "&gt;").replace('"', "&quot;"))


def load_sets():
    """The folder tree, read once. Refuses only what cannot be shipped
    honestly: a duplicate number inside one folder would give two works
    one URL."""
    if not os.path.isdir(SRC):
        say("no %s — nothing to build" % SRC)
        sys.exit(1)

    sets = []
    for folder in sorted(os.listdir(SRC)):
        path = os.path.join(SRC, folder)
        if not os.path.isdir(path):
            continue

        meta = {}
        meta_path = os.path.join(path, "set.json")
        if os.path.isfile(meta_path):
            try:
                with open(meta_path, encoding="utf-8") as fh:
                    meta = json.load(fh)
            except Exception as e:
                say("REFUSE  %s: set.json unreadable (%s)" % (folder, e))
                sys.exit(1)

        files = sorted(
            (f for f in os.listdir(path)
             if os.path.splitext(f)[1].lower() in WORK_EXTS),
            key=lambda f: (extract_number(f), f))
        if not files:
            continue

        seen = {}
        for f in files:
            n = extract_number(f)
            if n in seen:
                say("REFUSE  %s: %s and %s share number %d — one URL, two works"
                    % (folder, seen[n], f, n))
                sys.exit(1)
            seen[n] = f

        # HOUSE_0 — THE INFO SIDECARS. PAINTING_<n>.txt beside PAINTING_<n>.jpg:
        # one line, read by stem, so a sidecar always finds its own master and
        # only that one. A sidecar with no master is a warning, not a refusal.
        info = {}
        stems = set(os.path.splitext(f)[0] for f in files)
        for f in sorted(os.listdir(path)):
            if not (f.startswith("PAINTING_") and f.lower().endswith(".txt")):
                continue
            stem = os.path.splitext(f)[0]
            with open(os.path.join(path, f), encoding="utf-8-sig") as fh:
                line = " ".join(fh.read().split())
            if stem not in stems:
                say("  warning  %s/%s names no master here — the line is not shown" % (folder, f))
            elif line:
                info[stem] = line

        featured = set(int(x) for x in (meta.get("featured") or []))
        for n in sorted(featured - set(seen)):
            say("  warning  %s: set.json features %d and no PAINTING_%d is here" % (folder, n, n))

        sets.append({
            "folder": folder,
            "slug": slugify(meta.get("label", folder)),
            "label": meta.get("label", folder),
            "note": meta.get("note", ""),
            "paper": bool(meta.get("paper", False)),
            "info": info,
            "featured": featured,
            "files": files,
        })

    slugs = [s["slug"] for s in sets]
    if len(slugs) != len(set(slugs)):
        say("REFUSE  two sets share a slug: %s" % slugs)
        sys.exit(1)
    return sets


def average_hex(im):
    r, g, b = im.resize((1, 1)).getpixel((0, 0))
    return "#%02x%02x%02x" % (r, g, b)


def build_work(im, src_path, out_dir, n, write):
    """Encode one work's ladder. Returns the record the markup needs."""
    im = im.convert("RGB")
    w, h = im.size
    long_edge = max(w, h)

    rungs = [e for e in LADDER if e < long_edge]
    rungs.append(min(long_edge, FULL_EDGE))

    variants = []  # (edge, jpg_name, avif_name)
    for edge in rungs:
        step = im.copy()
        if max(step.size) > edge:
            step.thumbnail((edge, edge))
        sw, sh = step.size
        buf = io.BytesIO()
        step.save(buf, "JPEG", quality=JPEG_Q, optimize=True, progressive=True)
        tag = hashlib.sha256(buf.getvalue()).hexdigest()[:8]
        jpg = "%d-%d.%s.jpg" % (n, edge, tag)
        avf = "%d-%d.%s.avif" % (n, edge, tag)
        if write:
            step.save(os.path.join(out_dir, jpg), "JPEG",
                      quality=JPEG_Q, optimize=True, progressive=True)
            step.save(os.path.join(out_dir, avf), "AVIF",
                      quality=AVIF_Q, speed=AVIF_SPEED)
        variants.append((edge, sw, jpg, avf))

    return {
        "n": n, "w": w, "h": h,
        "tone": average_hex(im),
        "variants": variants,
    }


def tile_markup(set_slug, rec, info, featured=False):
    n = rec["n"]
    base = "%s/" % set_slug

    jpg_set = ", ".join("%s%s %dw" % (base, jn, sw)
                        for (_, sw, jn, _) in rec["variants"])
    avf_set = ", ".join("%s%s %dw" % (base, an, sw)
                        for (_, sw, _, an) in rec["variants"])
    full_e, _, full_jpg, _ = rec["variants"][-1]
    mid = next((v for v in rec["variants"] if v[0] >= 1280), rec["variants"][-1])
    first = rec["variants"][0]

    # The number lives in aria-label, alt and the #w<n> address — wiring
    # and accessibility — and on no visible surface (HOUSE_0).
    alt = "Painting %d" % n

    return (
        '<a class="work%(cls)s" href="%(full)s" style="--r:%(r).4f;--tone:%(tone)s"\n'
        '   data-n="%(n)d" data-set="%(set)s" data-w="%(w)d" data-h="%(h)d"\n'
        '   data-info="%(info)s"\n'
        '   data-mid="%(mid)s" data-full="%(full)s" aria-label="%(alt)s">\n'
        '  <div class="fill"></div>\n'
        '  <picture>\n'
        '    <source type="image/avif" srcset="%(avfset)s" sizes="%(sizes)s">\n'
        '    <img src="%(src)s" srcset="%(jpgset)s" sizes="%(sizes)s"\n'
        '         loading="lazy" decoding="async" alt="%(alt)s"\n'
        '         width="%(w)d" height="%(h)d">\n'
        '  </picture>\n'
        '</a>'
    ) % {
        "cls": " full" if featured else "",
        "full": base + full_jpg, "mid": base + mid[2],
        "r": rec["w"] / rec["h"], "tone": rec["tone"],
        "n": n, "set": set_slug, "w": rec["w"], "h": rec["h"],
        "info": esc(info), "alt": esc(alt),
        "avfset": esc(avf_set), "jpgset": esc(jpg_set),
        "src": base + first[2], "sizes": SIZES,
    }


def section_markup(s, tiles):
    note = ('\n    <span class="note">%s</span>' % esc(s["note"])) if s["note"] else ""
    return (
        '<section id="s-%(slug)s" class="set%(paper)s">\n'
        '  <div class="set-head">\n'
        '    <h2>%(label)s</h2>%(note)s\n'
        '  </div>\n'
        '  <div class="rows">\n%(tiles)s\n  </div>\n'
        '</section>'
    ) % {
        "slug": s["slug"], "paper": " paper" if s["paper"] else "",
        "label": esc(s["label"]), "note": note,
        "tiles": "\n".join(tiles),
    }


def index_markup(sets):
    links = ['<a href="#s-%s">%s</a>' % (s["slug"], esc(s["label"])) for s in sets]
    return '<span>·</span>'.join(links)


def peek_entry(s, rec, featured=False):
    """One work as the engine's menu sees it: the 640 rung, the tone, an
    alt and the page address of the full work. Absolute paths, because
    the engine page lives at the root."""
    first = rec["variants"][0]
    return {
        # DOORS_4 — setLabel left with the sets line that was its only
        # reader (`set` and `featured` still have server-side ones).
        "n": rec["n"], "set": s["slug"],
        "title": "Painting %d" % rec["n"],     # the pane tile's alt; no visible surface (HOUSE_0)
        "tone": rec["tone"], "r": round(rec["w"] / rec["h"], 4),
        "src": "/gallery/%s/%s" % (s["slug"], first[2]),
        "href": "/gallery/#w%d" % rec["n"],
        "featured": bool(featured),
    }


def peek_pick(entries):
    """Featured first, in set order; then the first work of each set,
    round-robin, until PEEK_COUNT. Keyed on (set, n): a number is unique
    within a folder, not across folders (load_sets)."""
    out = [e for e in entries if e["featured"]][:PEEK_COUNT]
    seen = set((e["set"], e["n"]) for e in out)
    lanes = {}
    for e in entries:
        lanes.setdefault(e["set"], []).append(e)
    lanes = list(lanes.values())
    i = 0
    while len(out) < PEEK_COUNT and any(lanes) and i < 10000:
        lane = lanes[i % len(lanes)]
        i += 1
        while lane and (lane[0]["set"], lane[0]["n"]) in seen:
            lane.pop(0)
        if lane:
            e = lane.pop(0)
            seen.add((e["set"], e["n"]))
            out.append(e)
    for e in out:
        e.pop("featured", None)
    return out


def fill(template, index_html, works_html):
    out = template.replace("<!-- __INDEX__ -->", index_html)
    out = out.replace("<!-- __WORKS__ -->", works_html)
    # DOORS_0 — the sandwich: links from web/routes.json, rules from
    # web/menu.css, through tools/routes.py — the renderer the engine
    # shell and home/ also use.
    out = out.replace("<!-- __ROUTES__ -->", routes.nav_html("site", "/gallery/", indent="      "))
    out = out.replace("/* __MENU_CSS__ */", routes.menu_css())
    for token in ("__INDEX__", "__WORKS__", "__ROUTES__", "__MENU_CSS__"):
        if token in out:
            say("REFUSE  template placeholder %s did not substitute" % token)
            sys.exit(1)
    return out


def data_uri(im, edge, q):
    step = im.convert("RGB").copy()
    step.thumbnail((edge, edge))
    buf = io.BytesIO()
    step.save(buf, "WEBP", quality=q, method=6)
    return "data:image/webp;base64," + base64.b64encode(buf.getvalue()).decode()


def font_uri(path):
    with open(path, "rb") as fh:
        return "data:font/woff2;base64," + base64.b64encode(fh.read()).decode()


def preview_tile(set_slug, rec, info, uri, featured=False):
    n = rec["n"]
    alt = "Painting %d" % n
    return (
        '<a class="work%(cls)s" href="#w%(n)d" style="--r:%(r).4f;--tone:%(tone)s"\n'
        '   data-n="%(n)d" data-set="%(set)s" data-w="%(w)d" data-h="%(h)d"\n'
        '   data-info="%(info)s"\n'
        '   data-mid="%(uri)s" aria-label="%(alt)s">\n'
        '  <div class="fill"></div>\n'
        '  <img src="%(uri)s" loading="lazy" decoding="async" alt="%(alt)s"\n'
        '       width="%(w)d" height="%(h)d">\n'
        '</a>'
    ) % {
        "cls": " full" if featured else "",
        "n": n, "r": rec["w"] / rec["h"], "tone": rec["tone"],
        "set": set_slug, "w": rec["w"], "h": rec["h"],
        "info": esc(info), "alt": esc(alt), "uri": uri,
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--preview", metavar="FILE")
    args = ap.parse_args()

    try:
        from PIL import Image
    except ImportError:
        say("REFUSE  Pillow is required here (pip install pillow) — unlike the")
        say("        exhibition, the collection IS its derivatives.")
        sys.exit(1)

    with open(TEMPLATE, encoding="utf-8") as fh:
        template = fh.read()

    sets = load_sets()
    total = sum(len(s["files"]) for s in sets)
    say("collection inventory  (%s)" % SRC)
    for s in sets:
        say("  %-24s %3d works%s" % (s["label"], len(s["files"]),
                                     "   [paper]" if s["paper"] else ""))
    say("  %-24s %3d works" % ("total", total))
    if args.check:
        return

    preview = bool(args.preview)
    if not preview:
        if os.path.isdir(DIST):
            shutil.rmtree(DIST)
        os.makedirs(DIST)

    index_html_parts, sections = [], []
    bytes_jpg = bytes_avf = 0
    peek = []   # DOORS_0 — every work's record, for peek_pick

    for s in sets:
        out_dir = os.path.join(DIST, s["slug"])
        if not preview:
            os.makedirs(out_dir, exist_ok=True)
        tiles = []
        for f in s["files"]:
            n = extract_number(f)
            src_path = os.path.join(SRC, s["folder"], f)
            info = s["info"].get(os.path.splitext(f)[0], "")
            with Image.open(src_path) as im:
                if preview:
                    rec = build_work(im, src_path, "", n, write=False)
                    uri = data_uri(im, PREVIEW_EDGE, PREVIEW_Q)
                    tiles.append(preview_tile(
                        s["slug"], rec, info, uri, featured=n in s["featured"]))
                else:
                    rec = build_work(im, src_path, out_dir, n, write=True)
                    tiles.append(tile_markup(
                        s["slug"], rec, info, featured=n in s["featured"]))
                    peek.append(peek_entry(s, rec, featured=n in s["featured"]))
                    for (_, _, jn, an) in rec["variants"]:
                        bytes_jpg += os.path.getsize(os.path.join(out_dir, jn))
                        bytes_avf += os.path.getsize(os.path.join(out_dir, an))
            say("  %s / %-22s ok" % (s["label"], f))
        sections.append(section_markup(s, tiles))

    index_html = index_markup(sets)
    page = fill(template, index_html, "\n\n".join(sections))

    if preview:
        # one file, openable from anywhere: fonts ride along as data URIs
        with open(os.path.join(ROOT, "web", "shared.css"), encoding="utf-8") as fh:
            shared = fh.read()
        page = page.replace('<link rel="stylesheet" href="../shared.css">',
                            "<style>\n" + shared + "\n</style>")
        fonts_dir = os.path.join(ROOT, "web", "fonts")
        for face in ("Newsreader.woff2", "Newsreader-Italic.woff2"):
            page = page.replace('url("fonts/%s")' % face,
                                'url("%s")' % font_uri(os.path.join(fonts_dir, face)))
        with open(args.preview, "w", encoding="utf-8") as fh:
            fh.write(page)
        say("\nwrote %s  (%.1f MB, self-contained)"
            % (args.preview, os.path.getsize(args.preview) / 1e6))
        return

    with open(os.path.join(DIST, "index.html"), "w", encoding="utf-8") as fh:
        fh.write(page)
    # DOORS_0 — THE PEEK. A dozen works for the engine's menu, drawn from
    # the same records that just wrote the page, so the two cannot
    # disagree. The shell fetches it on the first opening of the menu —
    # a gesture, never boot (web_dist's boot-set law).
    with open(os.path.join(DIST, "peek.json"), "w", encoding="utf-8") as fh:
        # DOORS_4 — ONE FACT, ONE HOME, and this fact's one reader is gone:
        # the pane's sets line died with ruling 5. The set labels still live
        # where they always did, in each assets/collection/*/set.json, and
        # the collection page reads them from there.
        json.dump({"works": peek_pick(peek)}, fh, separators=(",", ":"))
    # fonts live once, at the deployment root: front_dist.py ships
    # web/fonts/ to dist/fonts/, and this page reaches up to ../fonts/

    # Cloudflare Pages reads ONE _headers, at the deployment root. This
    # fragment is written beside the output and named so it cannot be
    # mistaken for the live file; web_dist.py's writer owns the merge.
    frag = os.path.join(DIST, "_headers.fragment")
    with open(frag, "w", encoding="utf-8") as fh:
        fh.write("/gallery/*\n"
                 "  Cache-Control: public, max-age=31536000, immutable\n"
                 "/gallery/\n"
                 "  Cache-Control: no-cache\n"
                 "/gallery/index.html\n"
                 "  Cache-Control: no-cache\n"
                 "/gallery/peek.json\n"
                 "  Cache-Control: no-cache\n")

    files = sum(len(fs) for _, _, fs in os.walk(DIST))
    say("\ndist/gallery/  %d files" % files)
    say("  jpeg  %6.1f MiB" % (bytes_jpg / 2**20))
    say("  avif  %6.1f MiB" % (bytes_avf / 2**20))
    say("  merge %s into the root _headers at deploy" % os.path.basename(frag))


if __name__ == "__main__":
    main()
