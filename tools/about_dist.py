#!/usr/bin/env python3
# ─── tools/about_dist.py ─────────────────────────────────────────
#
# The home page's pipeline (the page a menu calls Home; `about` is its
# wiring — the path, the id, this file). Third sibling: web_dist.py owns
# the world AT THE ROOT, collection_dist.py owns /gallery/, this owns
# /home/ and /writings/. The engine keeps `/` — so this must never write
# dist/index.html, or it would overwrite the engine's own shell.
#
#   python tools/about_dist.py                   # build into dist/home/ + dist/writings/
#   python tools/about_dist.py --preview F       # one self-contained file
#
# Reads assets/about/site.json (HOUSE_0 — paintings are NUMBERS, and the
# files live where the collection keeps them, assets/collection/<set>/):
#   email    the address the engine's write pane falls back to
#   hero     {"wide":[90, 92, 112], "tall":[50, 109]}  hand-curated pools —
#            hand-curated is the rule; an automatic pick once nominated
#            a photograph of a monkey as the front page of the universe.
#            A number with no master is a warning and leaves the pool; an
#            empty wide pool is the one refusal, because the door would be
#            a hole (POSTER_0's law).
#   strip    [2, 5, 7, 11]  works from the collection under the gallery
#            door; dist mode points at the collection's own derivatives,
#            preview embeds. A number with no master is a warning and a
#            shorter strip.
# and assets/about/<door>.jpg (world, writings): the doors' pictures,
# tracked; absent = a door without one.
#
# Writes dist/home/ (index.html, home.json, hero/*, the door pictures),
# dist/writings/ (the scroll, one page per text, writings.json), dist/fonts/
# and dist/shared.css (the fonts and the stylesheet live once, at the root;
# the pages reach up to them). Never touches dist/index.html or
# dist/gallery. Build order is collection first, then this, so the strip
# can find its derivatives.

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
TEMPLATE = os.path.join(WEB, "home", "index.html")
FONTS = os.path.join(ROOT, "web", "fonts")
DIST_ROOT = os.path.join(ROOT, "dist")
DIST = os.path.join(DIST_ROOT, "home")

HERO_EDGES = (1600, 800)
DOOR_EDGE = 1600       # HOUSE_0 — a door's picture (world, writings): the hero's long edge
DOOR_JPEG_Q = 78
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
    # HOUSE_0 — it requires what it reads and nothing else. `authors` and
    # `links` were required and read by nothing since DOORS_4; gone.
    for key in ("email", "hero"):
        if key not in site:
            say("REFUSE  site.json is missing %r" % key)
            sys.exit(1)
    for kind in ("wide", "tall"):
        pool = site["hero"].get(kind, [])
        if not all(isinstance(n, int) for n in pool):
            say("REFUSE  site.json hero.%s must be a list of painting numbers, e.g. [90, 92]" % kind)
            sys.exit(1)
    if not all(isinstance(n, int) for n in site.get("strip", [])):
        say("REFUSE  site.json strip must be a list of painting numbers, e.g. [2, 5, 7]")
        sys.exit(1)
    if not site["hero"].get("wide"):
        say("REFUSE  hero.wide is empty — the door needs at least one work")
        sys.exit(1)
    return site


MASTER_EXTS = (".jpg", ".jpeg", ".png")


def find_master(n):
    """HOUSE_0 — the master of painting n, wherever the collection keeps it:
    assets/collection/<any set>/PAINTING_<n>.<ext>. None if no set has it —
    the caller warns and moves on. The first match by set order if two sets
    hold the number (collection_dist allows that across folders)."""
    root = os.path.join(ROOT, "assets", "collection")
    if not os.path.isdir(root):
        return None
    for folder in sorted(os.listdir(root)):
        for ext in MASTER_EXTS:
            path = os.path.join(root, folder, "PAINTING_%d%s" % (n, ext))
            if os.path.isfile(path):
                return path
    return None


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
        for n in site["hero"].get(kind, []):
            path = find_master(n)
            if path is None:
                say("  warning  hero.%s names %d and no set holds PAINTING_%d — it leaves the pool" % (kind, n, n))
                continue
            with Image.open(path) as im:
                im = im.convert("RGB")
                w, h = im.size
                if preview:
                    src = data_uri(im, 1400, 66)
                else:
                    edge = min(max(w, h), HERO_EDGES[0])
                    step = im.copy()
                    step.thumbnail((edge, edge))
                    name = "%d-%d.jpg" % (n, step.size[0])
                    step.save(os.path.join(out_dir, name), "JPEG",
                              quality=HERO_JPEG_Q, optimize=True, progressive=True)
                    src = "hero/" + name
                    w, h = step.size
                data[kind].append({"n": n, "src": src, "w": w, "h": h})
    if not data["wide"]:
        say("REFUSE  every painting hero.wide names is missing — the door would be a hole; name one that exists")
        sys.exit(1)

    first = data["wide"][0]
    tag = ('<img src="%s" alt="Painting %d" width="%d" height="%d" '
           'fetchpriority="high" decoding="async">'
           % (first["src"], first["n"], first["w"], first["h"]))
    return tag, data


def build_strip(Image, site, preview):
    """The works under the gallery door, by number. Dist mode finds each
    one's 640 rung in dist/gallery/<any set>/ — the derivative names
    carry a content hash and the set's slug is the collection's business,
    so both are found, never guessed — and a number nobody built is a
    warning and a shorter strip, never a refused build (HOUSE_0)."""
    tags = []
    for n in site.get("strip", []):
        if preview:
            path = find_master(n)
            if path is None:
                say("  warning  strip names %d and no set holds PAINTING_%d — skipped" % (n, n))
                continue
            with Image.open(path) as im:
                src = data_uri(im, 560, 62)
                w, h = im.size
        else:
            pattern = os.path.join(DIST_ROOT, "gallery", "*", "%d-640.*.jpg" % n)
            found = sorted(glob.glob(pattern))
            if not found:
                say("  warning  strip names %d and dist/gallery holds no %d-640 — skipped "
                    "(a number no set has, or the collection is not built)" % (n, n))
                continue
            target = found[0]
            src = "../gallery/%s/%s" % (os.path.basename(os.path.dirname(target)),
                                            os.path.basename(target))
            with Image.open(target) as im:
                w, h = im.size
        tags.append('<img src="%s" alt="Painting %d" loading="lazy" '
                    'decoding="async" width="%d" height="%d">'
                    % (src, n, w, h))
    return "\n      ".join(tags)


def build_door_image(Image, preview, name):
    """HOUSE_0 — A DOOR'S PICTURE, by name. Jean drops the file at
    assets/about/<name>.jpg (or .jpeg/.png) — `world` for the board's door,
    `writings` for the writings' — and the door shows it. Absent = one line
    and a door without a picture, NEVER a refused build: a door must not hold
    the site hostage (DOORS_4's ruling for the world door, now for both).
    Shaped like build_hero and build_strip — embeds in preview mode, writes a
    derivative in dist mode — so a preview stays self-contained."""
    for ext in ("jpg", "jpeg", "png"):
        src = os.path.join(SRC, name + "." + ext)
        if not os.path.isfile(src):
            continue
        with Image.open(src) as im:
            if preview:
                uri = data_uri(im, DOOR_EDGE, 66)
                return ('<img class="door-img" src="%s" alt="" '
                        'loading="lazy" decoding="async">' % uri)
            step = im.convert("RGB")
            # THE LONG EDGE, not the width (DOORS_4 R5): thumbnail() caps it
            # on both axes and never upscales, the hero's own tool.
            if max(step.size) > DOOR_EDGE:
                step.thumbnail((DOOR_EDGE, DOOR_EDGE), Image.LANCZOS)
            os.makedirs(DIST, exist_ok=True)
            out = name + ".jpg"
            step.save(os.path.join(DIST, out), "JPEG",
                      quality=DOOR_JPEG_Q, optimize=True, progressive=True)
            return ('<img class="door-img" src="%s" alt="" '
                    'width="%d" height="%d" loading="lazy" decoding="async">'
                    % (out, step.size[0], step.size[1]))
    say("  %s door: assets/about/%s.jpg not found — the door ships without its picture"
        % (name, name))
    return ""


def writing_slug(stem):
    """The URL of a text, from its filename: the stem after the leading
    NN_ (the order, which is not a name), lowercased, non-alphanumerics
    to hyphens. `01_the_mirror_in_the_sand` -> `the-mirror-in-the-sand`.
    Reordering a file (renumbering it) therefore never moves its page;
    renaming the words after the number does, and is Jean's to do."""
    body = re.sub(r"^\d+_", "", stem)
    slug = re.sub(r"[^a-z0-9]+", "-", body.lower()).strip("-")
    return slug


def build_writings():
    """The writings, one home: assets/writings/NN_slug.txt. First line the
    title, then the body; blank lines break stanzas; line breaks inside a
    stanza are kept (white-space: pre-line on the page, <br> nowhere).
    Filename order is page order. Returns the pieces: slug, title, html."""
    src = os.path.join(ROOT, "assets", "writings")
    if not os.path.isdir(src):
        say("REFUSE  assets/writings is missing — the writings have one home and this is it")
        sys.exit(1)
    pieces = []
    for name in sorted(os.listdir(src)):
        if not name.endswith(".txt"):
            continue
        # utf-8-sig, not utf-8: several Windows editors write a BOM by
        # default, `encoding="utf-8"` keeps it, and U+FEFF is not
        # whitespace — `.strip()` cannot remove it, so it would ride at
        # the head of the first title, on the page and in the json both.
        # (DOORS_4 R5. utf-8-sig reads plain UTF-8 unchanged.)
        with open(os.path.join(src, name), encoding="utf-8-sig") as fh:
            raw = fh.read().replace("\r\n", "\n").strip("\n")
        lines = raw.split("\n")
        title = lines[0].strip()
        body = "\n".join(lines[1:]).strip("\n")
        if not title or not body:
            say("REFUSE  %s needs a title line and a body" % name)
            sys.exit(1)
        stanzas = [st.strip("\n") for st in body.split("\n\n") if st.strip()]
        html = "\n".join("<p>%s</p>" % esc(st) for st in stanzas)
        slug = writing_slug(os.path.splitext(name)[0])
        if not slug:
            say("REFUSE  %s has nothing after its number to make a URL from" % name)
            sys.exit(1)
        pieces.append({"slug": slug, "title": title, "html": html})
    if not pieces:
        say("REFUSE  assets/writings holds no .txt — nothing to build")
        sys.exit(1)
    # HOUSE_0 — a slug is a page: two files that share one would give one
    # URL two texts, which is the refusal the collection makes for numbers.
    seen = {}
    for p in pieces:
        if p["slug"] in seen:
            say("REFUSE  writings %s and %s share the URL /writings/%s/" % (seen[p["slug"]], p["title"], p["slug"]))
            sys.exit(1)
        seen[p["slug"]] = p["title"]
    return pieces


def writing_article(p):
    # The slug is a FILENAME, and a filename can hold a quote — escaped like
    # the title and the body always were (DOORS_4 R2).
    return '<article id="%s">\n<h2>%s</h2>\n%s\n</article>' % (esc(p["slug"]), esc(p["title"]), p["html"])


def writings_menu_html(pieces, base, indent="      "):
    """HOUSE_0 — THE WRITINGS MENU, spoken from the page at `base`. On the
    scroll it lists the titles; on a single page the scroll comes first and
    the page's own title is not there — routes.rel() answers "./" for the
    page itself and that entry is dropped, the sandwich's own rule (DOORS_1)."""
    items = [("scroll", "/writings/")] + [(p["title"], "/writings/%s/" % p["slug"]) for p in pieces]
    out = []
    for label, href in items:
        rel = routes.rel(href, base)
        if rel == "./":
            continue
        out.append('<a href="%s">%s</a>' % (esc(rel), esc(label)))
    return ("\n" + indent).join(out)


def write_writings_page(template, pieces, page_pieces, base, title, out_dir):
    """One fill of web/writings/index.html: the scroll (every piece, base
    /writings/) or one text alone (base /writings/<slug>/). The stylesheet
    is the one ../ reference in the template; a page one level down reaches
    it with one more."""
    depth = base.count("/") - 1
    page = fill(template, {
        "TITLE": title,
        "WMENU": writings_menu_html(pieces, base),
        "ROUTES": routes.nav_html("site", base, indent="      "),
        "MENU_CSS": routes.menu_css(),
        "WRITINGS": "\n".join(writing_article(p) for p in page_pieces),
    })
    if depth == 2:
        page = page.replace('href="../shared.css"', 'href="../../shared.css"')
    os.makedirs(out_dir, exist_ok=True)
    with open(os.path.join(out_dir, "index.html"), "w", encoding="utf-8") as fh:
        fh.write(page)


def fill(template, subs):
    out = template
    for key, val in subs.items():
        marker = "<!-- __%s__ -->" % key if key not in ("HERO_DATA", "EMAIL", "TITLE", "MENU_CSS") else None
        if key == "HERO_DATA":
            out = out.replace("/* __HERO_DATA__ */ null", json.dumps(val))
        elif key == "EMAIL":
            out = out.replace("__EMAIL__", val)
        elif key == "TITLE":
            out = out.replace("__TITLE__", esc(val))   # HOUSE_0 — inside <title>, a comment would be text
        elif key == "MENU_CSS":
            out = out.replace("/* __MENU_CSS__ */", val)   # DOORS_0 — the sandwich's rules, one home
        else:
            out = out.replace(marker, val)
    # DOORS_4 — THE TUPLE IS THE ONLY GUARD. fill() checks the OUTPUT, so a
    # token absent from a given template simply never appears and passes;
    # but a token PRESENT and unsubstituted ships silently unless it is
    # named here. __WRITINGS__ and __WORLD__ join for that reason.
    for token in ("__HERO__", "__STRIP__", "__WRITINGS__", "__WORLD__", "__WRITINGS_IMG__",
                  "__WMENU__", "__TITLE__", "__HERO_DATA__", "__EMAIL__", "__ROUTES__", "__MENU_CSS__"):
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
    # DOORS_4 R2 — EVERY READ THAT CAN REFUSE HAPPENS BEFORE THE FIRST
    # DIST WRITE. build_writings() refuses on an empty assets/writings, a
    # missing title line or an empty body; run where it used to sit — after
    # the about page shipped and BEFORE the dist/text/ sweep — such a
    # refusal left a dist carrying a fresh about page, stale fonts, and the
    # live dist/text/index.html the sweep exists to remove. dist.py stops
    # the chain, so that state is never deployed; but a guarantee that
    # depends on the order of the lines above it should not depend on it
    # silently. It is a pure read: it has no reason to be later.
    w_pieces = build_writings()
    # HOUSE_0 — THE WRITER OWNS ITS FOLDER WHOLESALE (collection_dist's own
    # rule), and sweeps it before its first write: a hero taken out of
    # site.json would otherwise stay in dist/home/hero/ under the tenant
    # rule, uploaded and served by every deploy after.
    if not preview and os.path.isdir(DIST):
        shutil.rmtree(DIST)
    hero_tag, hero_data = build_hero(Image, site, preview)

    page = fill(template, {
        "ROUTES": routes.nav_html("site", "/home/", indent="      "),   # DOORS_0 / DOORS_1: spoken from this page
        "MENU_CSS": routes.menu_css(),                        # DOORS_0
        "HERO": hero_tag,
        "STRIP": build_strip(Image, site, preview),
        "WORLD": build_door_image(Image, preview, "world"),          # DOORS_4 — the board's door
        "WRITINGS_IMG": build_door_image(Image, preview, "writings"), # HOUSE_0 — the writings' door
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
    # statement's one home; the writings' is assets/writings/, and their
    # page is web/writings/index.html (DOORS_4).
    # HOUSE_0 — THE STATEMENT IS OPTIONAL. Jean took it off the page; when
    # he writes one, the header comes back and this reads it again. An
    # absent header is an empty statement, not a refused build.
    m = re.search(r'<header class="statement" id="statement">(.*?)</header>', page, re.S)
    with open(os.path.join(DIST, "home.json"), "w", encoding="utf-8") as fh:
        json.dump({"statement": m.group(1).strip() if m else "",
                   "hero": hero_data,          # what build_hero returned: the day-indexed list the page itself rotates
                   "email": site["email"]}, fh)

    # DOORS_4 — THE WRITINGS, plural and growing; HOUSE_0 — AND EACH ONE A
    # PAGE. One home for the texts (assets/writings); one read (taken above,
    # before any dist write) writes the scroll at /writings/, one page per
    # text at /writings/<slug>/, and writings.json for the engine's pane, so
    # none of the three can disagree. A new text is one new file plus a
    # deploy. dist/writings/ is swept first: a text renamed or removed since
    # the last build would otherwise leave its old page live under the
    # tenant rule (web_dist deletes only the engine's own names).
    w_tpl = os.path.join(WEB, "writings", "index.html")
    with open(w_tpl, encoding="utf-8") as fh:
        w_template = fh.read()
    w_dist = os.path.join(DIST_ROOT, "writings")
    if os.path.isdir(w_dist):
        shutil.rmtree(w_dist)
    write_writings_page(w_template, w_pieces, w_pieces, "/writings/", "writings", w_dist)
    for p in w_pieces:
        write_writings_page(w_template, w_pieces, [p], "/writings/%s/" % p["slug"], p["title"],
                            os.path.join(w_dist, p["slug"]))
    with open(os.path.join(w_dist, "writings.json"), "w", encoding="utf-8") as fh:
        json.dump([{"title": p["title"], "html": p["html"]} for p in w_pieces], fh)
    say("dist/writings/: the scroll, %d page(s), writings.json" % len(w_pieces))

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
        say("  Search web/home/index.html.")
        say("")
    say("dist/home/index.html written; hero beside it, fonts at dist/fonts/")
    say("build order: collection_dist, then this, then web_dist LAST —")
    say("since WEBSITE_1 it deletes only the engine's own names, and its")
    say("root _headers folds our fragment and rules only what exists.")


if __name__ == "__main__":
    main()
