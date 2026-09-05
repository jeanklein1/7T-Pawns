#!/usr/bin/env python3
# ─── tools/collection_dist.py ────────────────────────────────────
#
# The collection's own pipeline. Deliberately a SIBLING of web_dist.py,
# not an extension of it: the exhibition path is guarded by asserts
# (assert_exhibition_written refuses strangers in dist/paintings) and
# nothing here may lean on that machinery or weaken it. Two pipelines,
# one folder convention, zero coupling.
#
#   python tools/collection_dist.py               # build dist/collection/
#   python tools/collection_dist.py --check       # inventory only
#   python tools/collection_dist.py --preview F   # one self-contained file
#
# THE SOURCE OF TRUTH is a folder tree:
#
#   assets/collection/<folder>/PAINTING_<n>.jpg   the works
#   assets/collection/<folder>/set.json           optional, all fields optional:
#       { "label": "…", "note": "…", "paper": true,
#         "featured": [90, 112],
#         "works": { "107": { "title": "…", "meta": "oil on canvas · 2024" } } }
#
# "featured" works break the grid and take the full width alone — the
# page's pacing is authored here, one list per set, not derived.
#
# Folders sort by name; works sort by the number in the filename — the
# same key gallery.hpp uses, restated here. A work with no metadata
# still ships, titled "no. <n>": a missing field must never be able to
# drop a painting.
#
# THE PAGE IS WRITTEN, NOT FETCHED. web/collection/index.html is source
# with two placeholder regions; this script fills them with static
# markup and writes the result to dist/collection/index.html. No
# runtime manifest, no client templating: the HTML is the manifest,
# which is also what lets the page work with JavaScript disabled.
#
# DERIVATIVE NAMES CARRY A CONTENT HASH. The _headers rule marks
# /collection/* immutable for a year, which is only safe if replacing a
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

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SRC = os.path.join(ROOT, "assets", "collection")
TEMPLATE = os.path.join(ROOT, "web", "collection", "index.html")
DIST = os.path.join(ROOT, "dist", "collection")

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

        sets.append({
            "folder": folder,
            "slug": slugify(meta.get("label", folder)),
            "label": meta.get("label", folder),
            "note": meta.get("note", ""),
            "paper": bool(meta.get("paper", False)),
            "work_meta": meta.get("works", {}) or {},
            "featured": set(int(x) for x in (meta.get("featured") or [])),
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


def tile_markup(set_slug, rec, meta, featured=False):
    n = rec["n"]
    title = meta.get("title") or ("no. %d" % n)
    meta_text = meta.get("meta", "")
    base = "%s/" % set_slug

    jpg_set = ", ".join("%s%s %dw" % (base, jn, sw)
                        for (_, sw, jn, _) in rec["variants"])
    avf_set = ", ".join("%s%s %dw" % (base, an, sw)
                        for (_, sw, _, an) in rec["variants"])
    full_e, _, full_jpg, _ = rec["variants"][-1]
    mid = next((v for v in rec["variants"] if v[0] >= 1280), rec["variants"][-1])
    first = rec["variants"][0]

    alt = title if not title.startswith("no. ") else "Painting %d" % n

    return (
        '<a class="work%(cls)s" href="%(full)s" style="--r:%(r).4f;--tone:%(tone)s"\n'
        '   data-n="%(n)d" data-set="%(set)s" data-w="%(w)d" data-h="%(h)d"\n'
        '   data-title="%(title)s" data-meta-text="%(meta)s"\n'
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
        "title": esc(title), "meta": esc(meta_text), "alt": esc(alt),
        "avfset": esc(avf_set), "jpgset": esc(jpg_set),
        "src": base + first[2], "sizes": SIZES,
    }


def section_markup(s, tiles):
    count = len(tiles)
    note = ('\n    <span class="note">%s</span>' % esc(s["note"])) if s["note"] else ""
    return (
        '<section id="s-%(slug)s" class="set%(paper)s">\n'
        '  <div class="set-head">\n'
        '    <h2>%(label)s</h2>\n'
        '    <span class="count">%(count)d work%(pl)s</span>%(note)s\n'
        '  </div>\n'
        '  <div class="rows">\n%(tiles)s\n  </div>\n'
        '</section>'
    ) % {
        "slug": s["slug"], "paper": " paper" if s["paper"] else "",
        "label": esc(s["label"]), "count": count,
        "pl": "" if count == 1 else "s", "note": note,
        "tiles": "\n".join(tiles),
    }


def index_markup(sets):
    links = ['<a href="#s-%s">%s</a>' % (s["slug"], esc(s["label"])) for s in sets]
    return '<span>·</span>'.join(links)


def fill(template, index_html, works_html):
    out = template.replace("<!-- __INDEX__ -->", index_html)
    out = out.replace("<!-- __WORKS__ -->", works_html)
    if "__INDEX__" in out or "__WORKS__" in out:
        say("REFUSE  template placeholder did not substitute")
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


def preview_tile(set_slug, rec, meta, uri, featured=False):
    n = rec["n"]
    title = meta.get("title") or ("no. %d" % n)
    alt = title if not title.startswith("no. ") else "Painting %d" % n
    return (
        '<a class="work%(cls)s" href="#w%(n)d" style="--r:%(r).4f;--tone:%(tone)s"\n'
        '   data-n="%(n)d" data-set="%(set)s" data-w="%(w)d" data-h="%(h)d"\n'
        '   data-title="%(title)s" data-meta-text="%(meta)s"\n'
        '   data-mid="%(uri)s" aria-label="%(alt)s">\n'
        '  <div class="fill"></div>\n'
        '  <img src="%(uri)s" loading="lazy" decoding="async" alt="%(alt)s"\n'
        '       width="%(w)d" height="%(h)d">\n'
        '</a>'
    ) % {
        "cls": " full" if featured else "",
        "n": n, "r": rec["w"] / rec["h"], "tone": rec["tone"],
        "set": set_slug, "w": rec["w"], "h": rec["h"],
        "title": esc(title), "meta": esc(meta.get("meta", "")),
        "alt": esc(alt), "uri": uri,
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

    for s in sets:
        out_dir = os.path.join(DIST, s["slug"])
        if not preview:
            os.makedirs(out_dir, exist_ok=True)
        tiles = []
        for f in s["files"]:
            n = extract_number(f)
            src_path = os.path.join(SRC, s["folder"], f)
            with Image.open(src_path) as im:
                if preview:
                    rec = build_work(im, src_path, "", n, write=False)
                    uri = data_uri(im, PREVIEW_EDGE, PREVIEW_Q)
                    tiles.append(preview_tile(
                        s["slug"], rec, s["work_meta"].get(str(n), {}), uri,
                        featured=n in s["featured"]))
                else:
                    rec = build_work(im, src_path, out_dir, n, write=True)
                    tiles.append(tile_markup(
                        s["slug"], rec, s["work_meta"].get(str(n), {}),
                        featured=n in s["featured"]))
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
    # fonts live once, at the deployment root: front_dist.py ships
    # web/fonts/ to dist/fonts/, and this page reaches up to ../fonts/

    # Cloudflare Pages reads ONE _headers, at the deployment root. This
    # fragment is written beside the output and named so it cannot be
    # mistaken for the live file; web_dist.py's writer owns the merge.
    frag = os.path.join(DIST, "_headers.fragment")
    with open(frag, "w", encoding="utf-8") as fh:
        fh.write("/collection/*\n"
                 "  Cache-Control: public, max-age=31536000, immutable\n"
                 "/collection/\n"
                 "  Cache-Control: no-cache\n"
                 "/collection/index.html\n"
                 "  Cache-Control: no-cache\n")

    files = sum(len(fs) for _, _, fs in os.walk(DIST))
    say("\ndist/collection/  %d files" % files)
    say("  jpeg  %6.1f MiB" % (bytes_jpg / 2**20))
    say("  avif  %6.1f MiB" % (bytes_avf / 2**20))
    say("  merge %s into the root _headers at deploy" % os.path.basename(frag))


if __name__ == "__main__":
    main()
