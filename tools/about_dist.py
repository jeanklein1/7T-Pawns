#!/usr/bin/env python3
# ─── tools/about_dist.py ─────────────────────────────────────────
#
# The about page's pipeline. Third sibling: web_dist.py owns the world
# AT THE ROOT, collection_dist.py owns /collection/, this owns /about/.
# The engine keeps `/` — so this must never write dist/index.html, or it
# would overwrite the engine's own shell. Each section
# reads its own assets root — assets/front/ here — per the convention
# that sections never share a source folder.
#
#   python tools/about_dist.py                   # build into dist/about/
#   python tools/about_dist.py --preview F       # one self-contained file
#
# Reads assets/about/site.json:
#   email    the address the message box falls back to
#   hero     {"wide":[{"file","n"}…], "tall":[…]}  hand-curated pools —
#            hand-curated is the rule; an automatic pick once nominated
#            a photograph of a monkey as the front page of the universe
#   strip    [{"n","dir","source"}…]  four works from the collection;
#            dist mode points at collection derivatives, preview embeds
#   authors  [{"name","lines":[…]}…]
#   links    [{"label","url"}…]  entries whose url contains REPLACE are
#            dropped at build rather than shipped as dead links
#
# Writes dist/about/index.html, dist/about/hero/*, and dist/fonts/* (the
# fonts live once at the root; both pages reach up to ../fonts/). Never
# touches dist/index.html or dist/collection. Build order is collection
# first, then this, so the strip can verify its targets exist.

import argparse
import base64
import io
import glob
import json
import routes   # DOORS_0 — web/routes.json + web/menu.css, the sandwich's one renderer
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SRC = os.path.join(ROOT, "assets", "about")
WEB = os.path.join(ROOT, "web")
TEMPLATE = os.path.join(WEB, "about", "index.html")
FONTS = os.path.join(ROOT, "web", "fonts")
DIST_ROOT = os.path.join(ROOT, "dist")
DIST = os.path.join(DIST_ROOT, "about")

HERO_EDGES = (1600, 800)
HERO_JPEG_Q = 82
STRIP_H = 240          # strip images are small; one size is enough


def say(m=""):
    print(m)


def esc(s):
    return (str(s).replace("&", "&amp;").replace("<", "&lt;")
            .replace(">", "&gt;").replace('"', "&quot;"))


def load_site():
    path = os.path.join(SRC, "site.json")
    if not os.path.isfile(path):
        say("no %s — nothing to build" % path)
        sys.exit(1)
    with open(path, encoding="utf-8") as fh:
        site = json.load(fh)
    for key in ("email", "hero", "strip", "authors", "links"):
        if key not in site:
            say("REFUSE  site.json is missing %r" % key)
            sys.exit(1)
    if not site["hero"].get("wide"):
        say("REFUSE  hero.wide is empty — the door needs at least one work")
        sys.exit(1)
    return site


def data_uri(im, edge, q):
    step = im.convert("RGB").copy()
    step.thumbnail((edge, edge))
    buf = io.BytesIO()
    step.save(buf, "WEBP", quality=q, method=6)
    return "data:image/webp;base64," + base64.b64encode(buf.getvalue()).decode()


def build_hero(Image, site, preview):
    """Returns (first_img_tag, hero_data). Encodes derivatives in dist
    mode; embeds in preview mode. The no-JS visitor gets wide[0]."""
    out_dir = os.path.join(DIST, "hero")
    if not preview:
        os.makedirs(out_dir, exist_ok=True)

    data = {"wide": [], "tall": []}
    for kind in ("wide", "tall"):
        for entry in site["hero"].get(kind, []):
            path = os.path.join(SRC, entry["file"])
            if not os.path.isfile(path):
                say("REFUSE  hero names %s — not in assets/about" % entry["file"])
                sys.exit(1)
            with Image.open(path) as im:
                im = im.convert("RGB")
                w, h = im.size
                if preview:
                    src = data_uri(im, 1400, 66)
                else:
                    edge = min(max(w, h), HERO_EDGES[0])
                    step = im.copy()
                    step.thumbnail((edge, edge))
                    name = "%d-%d.jpg" % (entry["n"], step.size[0])
                    step.save(os.path.join(out_dir, name), "JPEG",
                              quality=HERO_JPEG_Q, optimize=True, progressive=True)
                    src = "hero/" + name
                    w, h = step.size
                data[kind].append({"n": entry["n"], "src": src, "w": w, "h": h})

    first = data["wide"][0]
    tag = ('<img src="%s" alt="Painting %d" width="%d" height="%d" '
           'fetchpriority="high" decoding="async">'
           % (first["src"], first["n"], first["w"], first["h"]))
    return tag, data


def build_strip(Image, site, preview):
    tags = []
    for entry in site["strip"]:
        if preview:
            path = os.path.join(ROOT, entry["source"])
            with Image.open(path) as im:
                src = data_uri(im, 560, 62)
                w, h = im.size
        else:
            # derivative names carry a content hash, so find rather than guess
            pattern = os.path.join(DIST_ROOT, "collection", entry["dir"],
                                   "%d-640.*.jpg" % entry["n"])
            found = sorted(glob.glob(pattern))
            if not found:
                say("REFUSE  strip wants %s/%d — build the collection first"
                    % (entry["dir"], entry["n"]))
                sys.exit(1)
            target = found[0]
            src = "../collection/%s/%s" % (entry["dir"], os.path.basename(target))
            with Image.open(target) as im:
                w, h = im.size
        tags.append('<img src="%s" alt="Painting %d" loading="lazy" '
                    'decoding="async" width="%d" height="%d">'
                    % (src, entry["n"], w, h))
    return "\n      ".join(tags)


def build_writings():
    """The writings, one home: assets/writings/NN_slug.txt. First line the
    title, then the body; blank lines break stanzas; line breaks inside a
    stanza are kept (white-space: pre-line on the page, <br> nowhere).
    Filename order is page order. Returns (page_html, json_list)."""
    src = os.path.join(ROOT, "assets", "writings")
    if not os.path.isdir(src):
        say("REFUSE  assets/writings is missing — the writings have one home and this is it")
        sys.exit(1)
    pieces = []
    for name in sorted(os.listdir(src)):
        if not name.endswith(".txt"):
            continue
        with open(os.path.join(src, name), encoding="utf-8") as fh:
            raw = fh.read().replace("\r\n", "\n").strip("\n")
        lines = raw.split("\n")
        title = lines[0].strip()
        body = "\n".join(lines[1:]).strip("\n")
        if not title or not body:
            say("REFUSE  %s needs a title line and a body" % name)
            sys.exit(1)
        stanzas = [s.strip("\n") for s in body.split("\n\n") if s.strip()]
        html = "\n".join("<p>%s</p>" % esc(s) for s in stanzas)
        slug = os.path.splitext(name)[0]
        pieces.append({"slug": slug, "title": title, "html": html})
    if not pieces:
        say("REFUSE  assets/writings holds no .txt — nothing to build")
        sys.exit(1)
    page = "\n".join('<article id="%s">\n<h2>%s</h2>\n%s\n</article>'
                     % (p["slug"], esc(p["title"]), p["html"]) for p in pieces)
    return page, [{"title": p["title"], "html": p["html"]} for p in pieces]


def fill(template, subs):
    out = template
    for key, val in subs.items():
        marker = "<!-- __%s__ -->" % key if key not in ("HERO_DATA", "EMAIL", "MENU_CSS") else None
        if key == "HERO_DATA":
            out = out.replace("/* __HERO_DATA__ */ null", json.dumps(val))
        elif key == "EMAIL":
            out = out.replace("__EMAIL__", val)
        elif key == "MENU_CSS":
            out = out.replace("/* __MENU_CSS__ */", val)   # DOORS_0 — the sandwich's rules, one home
        else:
            out = out.replace(marker, val)
    # DOORS_4 — THE TUPLE IS THE ONLY GUARD. fill() checks the OUTPUT, so a
    # token absent from a given template simply never appears and passes;
    # but a token PRESENT and unsubstituted ships silently unless it is
    # named here. __WRITINGS__ and __WORLD__ join for that reason.
    for token in ("__HERO__", "__STRIP__", "__WRITINGS__", "__WORLD__",
                  "__HERO_DATA__", "__EMAIL__", "__ROUTES__", "__MENU_CSS__"):
        if token in out:
            say("REFUSE  template placeholder %s did not substitute" % token)
            sys.exit(1)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preview", metavar="FILE")
    args = ap.parse_args()
    preview = bool(args.preview)

    try:
        from PIL import Image
    except ImportError:
        say("REFUSE  Pillow is required (pip install pillow)")
        sys.exit(1)

    with open(TEMPLATE, encoding="utf-8") as fh:
        template = fh.read()

    site = load_site()
    hero_tag, hero_data = build_hero(Image, site, preview)

    page = fill(template, {
        "ROUTES": routes.nav_html("site", "/about/", indent="      "),   # DOORS_0 / DOORS_1: spoken from this page
        "MENU_CSS": routes.menu_css(),                        # DOORS_0
        "HERO": hero_tag,
        "STRIP": build_strip(Image, site, preview),
        "HERO_DATA": hero_data,
        "EMAIL": site["email"],
    })

    if preview:
        with open(os.path.join(ROOT, "web", "shared.css"), encoding="utf-8") as fh:
            shared = fh.read()
        page = page.replace('<link rel="stylesheet" href="../shared.css">',
                            "<style>\n" + shared + "\n</style>")
        # the @font-face rules now come from shared.css, whose url() is
        # written relative to the deployment root, not to this page
        for face in ("Newsreader.woff2", "Newsreader-Italic.woff2"):
            with open(os.path.join(FONTS, face), "rb") as fh:
                uri = "data:font/woff2;base64," + base64.b64encode(fh.read()).decode()
            page = page.replace('url("fonts/%s")' % face, 'url("%s")' % uri)
        with open(args.preview, "w", encoding="utf-8") as fh:
            fh.write(page)
        say("wrote %s  (%.1f MB, self-contained)"
            % (args.preview, os.path.getsize(args.preview) / 1e6))
        return

    os.makedirs(DIST, exist_ok=True)
    with open(os.path.join(DIST, "index.html"), "w", encoding="utf-8") as fh:
        fh.write(page)
    # DOORS_2/DOORS_3 — THE ABOUT PAGE IN MINIATURE: the statement, today's
    # hero (the same day-indexed pick the page makes) and the address, for
    # the engine's About and Write panes. Taken from the page just written,
    # so the two cannot disagree (peek.json's law). The template stays the
    # statement's one home; the doctrine's is web/text/index.html now.
    m = re.search(r'<header class="statement" id="statement">(.*?)</header>', page, re.S)
    if not m:
        say("REFUSE  the statement header is not in the page — the engine's About pane would have nothing to show")
        sys.exit(1)
    with open(os.path.join(DIST, "about.json"), "w", encoding="utf-8") as fh:
        json.dump({"statement": m.group(1).strip(),
                   "hero": hero_data,          # what build_hero returned: the day-indexed list the page itself rotates
                   "email": site["email"]}, fh)

    # DOORS_4 — THE WRITINGS PAGE, plural and growing. One home for the
    # texts (assets/writings); this page and writings.json are built from
    # the same read, so the engine's pane and the site can never disagree,
    # and a new text is one new file plus a deploy.
    w_page_html, w_json = build_writings()
    w_tpl = os.path.join(WEB, "writings", "index.html")
    with open(w_tpl, encoding="utf-8") as fh:
        w_page = fill(fh.read(), {
            "ROUTES": routes.nav_html("site", "/writings/", indent="      "),
            "MENU_CSS": routes.menu_css(),
            "WRITINGS": w_page_html,
        })
    w_dist = os.path.join(DIST_ROOT, "writings")
    os.makedirs(w_dist, exist_ok=True)
    with open(os.path.join(w_dist, "index.html"), "w", encoding="utf-8") as fh:
        fh.write(w_page)
    with open(os.path.join(w_dist, "writings.json"), "w", encoding="utf-8") as fh:
        json.dump(w_json, fh)
    say("dist/writings/index.html written; writings.json beside it (%d piece(s))" % len(w_json))

    # DOORS_4 — AND THE OLD PAGE IS SWEPT, because nothing else will.
    # DOORS_3 made this script the writer of dist/text/, and web_dist
    # deletes only "the engine's own names" — text is not among them. So
    # a dist/ from before this round keeps a live dist/text/index.html
    # that the deploy would upload and serve, SHADOWING the 301 in
    # _redirects. The writer that made it is the one that removes it.
    stale_text = os.path.join(DIST_ROOT, "text")
    if os.path.isdir(stale_text):
        shutil.rmtree(stale_text)
        say("dist/text/ swept — /text/ is a 301 to /writings/ now")
    dst_fonts = os.path.join(DIST_ROOT, "fonts")
    if os.path.isdir(dst_fonts):
        shutil.rmtree(dst_fonts)
    shutil.copytree(FONTS, dst_fonts)
    # one stylesheet, at the root; both pages reach up to ../shared.css
    shutil.copy2(os.path.join(ROOT, "web", "shared.css"),
                 os.path.join(DIST_ROOT, "shared.css"))

    holds = page.count("PLACEHOLDER")
    if holds:
        say("")
        say("  %d PLACEHOLDER marker%s still in the page — copy is not final."
            % (holds, "" if holds == 1 else "s"))
        say("  Search web/about/index.html and assets/about/site.json.")
        say("")
    say("dist/about/index.html written; hero beside it, fonts at dist/fonts/")
    say("build order: collection_dist, then this, then web_dist LAST —")
    say("since WEBSITE_1 it deletes only the engine's own names, and its")
    say("root _headers folds our fragment and rules only what exists.")


if __name__ == "__main__":
    main()
