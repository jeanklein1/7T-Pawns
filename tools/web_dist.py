#!/usr/bin/env python3
# ─── tools/web_dist.py ───────────────────────────────────────────
#
# SHIP_0 U4 — assemble the deployable folder and decide the host BY THE
# NUMBERS, not by preference.
#
# CC never builds, so this script is the executable half of U4: Jean
# builds, runs this, and it reports the inventory, applies the host
# rule, and writes dist/. It reads sizes off disk — nothing here is a
# remembered number.
#
#   python tools/web_dist.py            # inventory + verdict + write dist/
#   python tools/web_dist.py --check    # inventory + verdict only
#
# EXHIBIT_0 U1 — THE EXHIBITION LEAVES THE BUNDLE. The four build files
# above are THE PROGRAM: rebuilt only when the program changes. The
# paintings, the soundtrack, and exhibition.json are THE EXHIBITION:
# plain files beside the program, deployed alone when the exhibition
# changes. Daily curation must never wake the compiler — which is why
# they are assembled here, by this script, and not by --preload-file.
#
# THE HOST RULE (SHIP_0 U4, in its own words): every file <= 25 MiB ->
# Cloudflare Pages; else -> GitHub Pages (~100 MiB/file). If neither
# fits, that is a RESOLVE — the script says so and stops rather than
# repackaging on its own authority.
#
# WebGPU requires a secure context. HTTPS is mandatory and a LAN IP is
# not it; both hosts below serve HTTPS by default, which is the whole
# reason they are the candidates.

import argparse
import hashlib
import json
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
WEB = os.path.join(ROOT, "web")
DIST = os.path.join(ROOT, "dist")

SRC_PAINTINGS = os.path.join(ROOT, "assets", "paintings")
# ENTRANCE_0 — THE ENTRANCE'S OWN FOLDER. Read by resolve_poster_sources
# and by nothing else; list_paintings never looks here, which is the whole
# mechanism. Absent is legal — see the resolver.
SRC_ENTRANCE = os.path.join(ROOT, "assets", "entrance")
SRC_MUSIC = os.path.join(ROOT, "assets", "music")
DIST_PAINTINGS = os.path.join(DIST, "paintings")
DIST_MUSIC = os.path.join(DIST, "music")

# ── ORGAN_4 P6 — THE PRESET SHELF ────────────────────────────────
# web/presets/ holds index.json and one file per scene, fetched BY THE
# PANEL at boot (`?preset=<name>`) and by its select. They are SOURCE, not
# build output, and they are tracked — a scene is a file, and dropping one
# in the folder plus one line in the index is the whole publishing act.
#
# They ship the same way the exhibition does: copied verbatim, weighed by
# the same per-file cap, and COUNTED, so the file count printed at the end
# is not a lie (the poster precedent).
SRC_PRESETS = os.path.join(WEB, "presets")
DIST_PRESETS = os.path.join(DIST, "presets")


def preset_files():
    """Every .json on the shelf, sorted. Absent folder -> no shelf, no error:
    presets are a LAYER, and the panel hides its select when the index 404s."""
    if not os.path.isdir(SRC_PRESETS):
        return []
    return sorted(f for f in os.listdir(SRC_PRESETS) if f.endswith(".json"))

# index.html is SOURCE (tracked); the other three are build output
# (.gitignore'd). All four ship — but index.html is the only one that is
# TRANSFORMED on the way (BUILDID_0), not copied.
ARTIFACTS = ["index.html", "organ_panel.js", "the_board.js", "the_board.wasm", "the_board.data"]

# ── BUILDID_0 — THE STALE PAIR ───────────────────────────────────
# The three build files always move together on disk, but their URLs
# were constant across every deploy. A browser holding one from a
# previous visit could therefore pair it with a fresh sibling — old glue
# against new wasm, imports landing undefined. Cloudflare's
# must-revalidate + ETag makes that rare, not impossible: a 304 on one
# file and a 200 on another is the whole failure, and it lands on a
# stranger's phone weeks later with no way to reproduce it.
#
# The id is the wasm's own hash, TRUNCATED ONLY FOR THE URL — so it
# changes exactly when the program changes, and cannot drift from it.
# A date stamp would have been a second fact about the same thing, and
# would have busted every cache on a rebuild that changed nothing.
BUILD_ID_PLACEHOLDER = "__BUILD_ID__"
BUILD_ID_LEN = 12

# ── PROBATE_SEAL2 — THE SERVE WITNESS ────────────────────────────
# The Pixel was served a world.wgsl cut mid-token 665 bytes from its
# end, and every gate in the tree was green — correctly, because the
# tree was clean and this script copies the shader's package byte for
# byte. Nothing stood between dist/ and the device, so the defect
# announced itself as a syntax error in a file that has none.
#
# The shader's digest is computed here, from the same bytes the link
# packages, and baked into the page. At boot the loader hashes what it
# actually received and compares (renderer.hpp, loadShader). One fact,
# two ends, compared where the audience stands.
#
# THE SOURCE FILE IS THE SUBJECT, deliberately. --preload-file packs
# this exact path into the_board.data (CMakeLists, T7_WEB_SHADER), so a
# digest taken here also catches a STALE .data — a build whose link did
# not rerun ships a shader older than its source, and that failure looks
# identical to a truncation from the audience's side. One witness, both
# corridors.
#
# Eight hex: the short form the console prints, long enough to name a
# corruption and short enough to read off a phone at arm's length.
SHADER_SHA_PLACEHOLDER = "__SHADER_SHA__"

SHADER_SHA_LEN = 8
SHADER_SRC = os.path.join(ROOT, "src", "cartridges", "the_board",
                          "realization", "world.wgsl")

# THE SAME NUMBER THE PROGRAM STAGES AT. The authored loader scales every
# painting to fit Dim::PAINTING_RESOLUTION (src/cartridges/the_board/
# realization/state.hpp) and pads to that square before upload, and it
# never scales UP (`if (scale > 1.0f) scale = 1.0f`). A long side above
# this value is therefore bytes over the wire that the GPU throws away.
# Capping here is the only place that saves them. If PAINTING_RESOLUTION
# moves, this moves with it.
PAINTING_CAP = 512
PAINTING_QUALITY = 82

# THE SECOND NUMBER DIED WITH ITS MECHANISM (REPEAT_0 U6). MANIFEST_DEDUPE_CAP
# echoed `bool disk_in_use[256]` in rotate_authored_staging — a dedupe that
# failed OPEN past 256, so a large manifest could hang one canvas twice, in
# silence. The rotation is deleted and the array with it: the program no longer
# dedupes by comparison at all. It hands each manifest index out exactly once
# per lap of a cursor, and a repeat past the lap is what "on repeat" means
# (REPEAT_0 R5). A catalogue of any size is now correct by arithmetic, so
# there is no ceiling left to state and nothing here to keep in step.

# The scan the program does, restated: name starts PAINTING_, extension
# .jpg or .jpeg case-insensitively (gallery.hpp, scan_paintings_folder).
PAINTING_EXTS = (".jpg", ".jpeg")


# POSTER_0 — THE SHELL KNOWS ROLES, THIS KNOWS PAINTINGS. web/index.html
# names veil_poster.jpg and card_poster.jpg and nothing else; re-picking
# a painting is one edit here and a redeploy, with no page change.
POSTERS = {
    # ENTRANCE_0 E2 — BOTH ROLES LEAVE THE EXHIBITION. These stems live in
    # assets/entrance/, which list_paintings() cannot reach, so neither
    # image can also hang on a wall. Jean's stamp, Aug 2026.
    #
    # THE CONTROLS IMAGE TAKES THE BOOT'S CRITICAL PATH, and that is the
    # whole reason the pair is ordered this way: ATTIC_ATRIUM D1 deleted the
    # controls poster and HINT_0 deleted #hint before it, so the piece
    # currently teaches nothing about its own keys. The veil is the first
    # thing a visitor sees and the thing they see again at READY; the card
    # shows only when something has gone wrong, which is the wrong moment
    # to explain how to play.
    "veil_poster.jpg": "ENTRANCE_CONTROLS",   # STATE 1 LOADING + STATE 4 READY
    "card_poster.jpg": "ENTRANCE_FIELD",      # STATE 2 FALLBACK + STATE 3 LOST
}
# The .poster box is at most 340 CSS px wide. 640 covers it at DPR ~1.9
# and at the widths a phone actually gives it under the 74vw and 34vh
# caps. It is the one dial: 768 costs ~+52 KB on the veil, which is the
# only poster on the boot's critical path.
POSTER_WIDTH   = 640
POSTER_QUALITY = 78

# exhibition.json, NOT manifest.json — the root manifest.json name stays
# reserved for the PWA web manifest.
EXHIBITION_JSON = "exhibition.json"

# ── AUBADE U6 — THE VERSIONED SET, AND ONLY IT ──────────────────────
#
# Every path here is fetched with `?v=<build id>` appended, so its URL —
# which is what the HTTP cache keys on — changes whenever its bytes do.
# index.html is NOT in this list and must never be: it is the page that
# hands out the keys, and it keeps the `no-cache` rule the header
# writer's banner argues for. Nor are paintings/ or music/, which are
# named without a version and can change under a constant name.
IMMUTABLE_PATHS = ["the_board.js", "the_board.wasm", "the_board.data",
                   "organ_panel.js"]
IMMUTABLE_RULE = "public, max-age=31536000, immutable"

# ── AUBADE U7/U8 (RUL-E) — THE MUSIC LAW IS A SIZE LAW ──────────────
#
# Ruled 28 Aug 2026: codec and bitrate stay as authored — no re-encode,
# no listening tests. What is law is the SIZE, and it is one number: a
# tune ships as ONE file under this ceiling, and a tune projected past it
# splits at a movement boundary at export time.
#
# 20 MiB against Cloudflare's 25 MiB per-file limit (CF_LIMIT below), so
# the margin is five whole MiB rather than a rounding error — the point
# of a soft ceiling is that hitting it is a conversation and not a failed
# deploy. R12: one tune exists, samsara.mp3 at 4.0 MiB, so tripling the
# library still clears it.
AUDIO_CEILING = 20 * 1024 * 1024

# ── AUBADE U7 — THE BOOT SET, NAMED ─────────────────────────────────
#
# What a visitor fetches before the world is on screen: the page, the
# glue, the wasm, the package, the veil's poster, and the exhibition
# manifest. Nothing sized O(catalogue) and nothing sized O(library) is in
# it, and the asserts below are what keep it that way — a claim nobody
# checks is a claim that stops being true on the commit that breaks it.
BOOT_SET = ["index.html", "the_board.js", "the_board.wasm", "the_board.data",
            "veil_poster.jpg", EXHIBITION_JSON]

CF_LIMIT = 25 * 1024 * 1024        # Cloudflare Pages per-file
GH_LIMIT = 100 * 1024 * 1024       # GitHub Pages per-file (soft, ~100 MiB)


def mib(n):
    return n / (1024.0 * 1024.0)


def extract_number(name):
    """The C++ sort key, restated in Python.

    gallery.hpp takes the stem, finds the FIRST '_', and hands the rest to
    std::stoi — which reads leading digits and stops, and whose throw is
    caught and turned into 0. `int(...)` would reject "1_v2" where stoi
    returns 1, so the leading-digit match is the faithful form.
    """
    stem = os.path.splitext(name)[0]
    pos = stem.find("_")
    if pos == -1 or pos + 1 >= len(stem):
        return 0
    m = re.match(r"\s*([+-]?\d+)", stem[pos + 1:])
    return int(m.group(1)) if m else 0


def list_paintings():
    """Source paintings in the program's numeric order (PAINTING_2 < PAINTING_10)."""
    if not os.path.isdir(SRC_PAINTINGS):
        return []
    names = [
        f for f in os.listdir(SRC_PAINTINGS)
        if os.path.isfile(os.path.join(SRC_PAINTINGS, f))
        and f.startswith("PAINTING_")
        and os.path.splitext(f)[1].lower() in PAINTING_EXTS
    ]
    # Name is the tiebreak so the order is total, not merely numeric —
    # std::sort is unstable and the browser re-sorts on arrival anyway,
    # but a manifest that reorders between runs is noise in a diff.
    names.sort(key=lambda f: (extract_number(f), f))
    return names


# ═══ AUBADE U5c — THE LONG EDGE, MEASURED AT DIST TIME ═══════════════
#
# The runtime decodes a painting on the main thread (there is no browser
# decode anywhere in the program) and pads it to Dim::PAINTING_RESOLUTION.
# PAINTING_CAP is the number that keeps that decode small — but it was
# only ever ENFORCED by write_paintings' Pillow arm, and that arm has a
# documented fallback: no Pillow, copy verbatim, uncapped. A deploy from a
# machine without Pillow therefore shipped full-size sources and the
# runtime decoded texels it would immediately throw away, once per
# painting, on the main thread, in the frames right after first light.
#
# So the cap is asserted on what actually LANDED IN dist/, on every path,
# by reading the JPEG's own frame header. No dependency: a JPEG's size is
# five bytes into any SOFn marker, and the marker walk is twenty lines.
#
# It REFUSES rather than warns, for the same reason write_posters
# refuses: a painting past the cap is a cost the visitor pays in the one
# window this campaign is here to empty.
def jpeg_dimensions(path):
    """(width, height) from a JPEG's SOFn marker, or None if unreadable.

    The walk: SOI, then a chain of length-prefixed segments. Any SOFn
    except the four that are not frame headers (DHT C4, JPG C8, DAC CC)
    carries height and width as big-endian u16 at offsets 3 and 5 of its
    payload. Entropy-coded data begins at SOS (DA) and no SOF follows it."""
    try:
        with open(path, "rb") as fh:
            b = fh.read()
    except OSError:
        return None
    if len(b) < 4 or b[0] != 0xFF or b[1] != 0xD8:
        return None
    i = 2
    while i + 3 < len(b):
        if b[i] != 0xFF:
            i += 1
            continue
        m = b[i + 1]
        if m in (0xD8, 0x01) or 0xD0 <= m <= 0xD7:   # no payload
            i += 2
            continue
        if m == 0xDA:                                 # scan: no SOF after this
            return None
        seg = (b[i + 2] << 8) | b[i + 3]
        if seg < 2:
            return None
        if 0xC0 <= m <= 0xCF and m not in (0xC4, 0xC8, 0xCC):
            if i + 9 >= len(b):
                return None
            h = (b[i + 5] << 8) | b[i + 6]
            w = (b[i + 7] << 8) | b[i + 8]
            return (w, h)
        i += 2 + seg
    return None


def assert_painting_cap(paths):
    """Every shipped painting is at or under PAINTING_CAP on its long edge.

    Returns a list of (path, reason) — empty is the pass."""
    bad = []
    for p in paths:
        dims = jpeg_dimensions(p)
        if dims is None:
            bad.append((p, "not a readable JPEG frame header"))
            continue
        w, h = dims
        if max(w, h) > PAINTING_CAP:
            bad.append((p, "%dx%d — long edge %d over the %d cap"
                        % (w, h, max(w, h), PAINTING_CAP)))
    return bad


def assert_exhibition_written(names):
    """Every name exhibition.json will carry has a real file beside it.

    Returns a list of (name, reason) — empty is the pass.

    REPEAT_0a U0b. exhibition.json is a list of PROMISES, and nothing
    checked that dist/ keeps them. The one time it mattered — 57 names, 24
    pictures, HTTP 0 on the other 33, every lap, for a month — the console
    could say the program was asking and nothing anywhere could say whether
    the files had ever been WRITTEN. That left the fault equally on this
    script, the deploy and the host, and clearing this script's name cost a
    campaign. THIS is the line that should have done it.

    It cannot fail on a healthy run: write_paintings is handed the very list
    the manifest is written from, so the two agree by construction. That is
    exactly why it earns its place — a run where it DOES fail has written a
    manifest that lies, and the whole point of the exhibition being plain
    files beside the program is that such a lie is invisible until a
    visitor's console fills with HTTP 0.

    AND THE NEGATIVE IS THE HALF THAT PAYS. When this passes and the
    deployed site still cannot serve a painting, the loss is in the deploy
    or the host, and nobody has to spend a campaign finding that out."""
    bad = []
    if not os.path.isdir(DIST_PAINTINGS):
        return [(DIST_PAINTINGS, "no paintings folder in dist at all")]
    on_disk = set(os.listdir(DIST_PAINTINGS))
    for f in names:
        if f not in on_disk:
            bad.append((f, "named by exhibition.json, no file in dist/paintings"))
        elif os.path.getsize(os.path.join(DIST_PAINTINGS, f)) == 0:
            bad.append((f, "named by exhibition.json, zero bytes in dist/paintings"))
    for f in sorted(on_disk - set(names)):
        bad.append((f, "in dist/paintings, not named by exhibition.json"))
    return bad


# ═══ AUBADE U7 — THE PACKAGE HOLDS THE SHADER AND NOTHING ELSE ═══════
#
# R11 found the package already lawful: one --preload-file, world.wgsl
# only (CMakeLists, T7_WEB_SHADER). So this unit does not evict anything
# — it makes the fact CHECKED, because "already lawful" is a property of
# today's link line and not of the repository.
#
# HOW, WITHOUT KNOWING EMSCRIPTEN'S METADATA FORMAT. file_packager writes
# the packed files back-to-back into the .data as raw bytes and keeps the
# offsets in the generated JS; with exactly one file and no compression
# (neither is requested), the_board.data IS world.wgsl, byte for byte.
# So the assert is a digest comparison against bytes this script already
# reads for the boot's shader witness — no format parsing, nothing to
# drift when the toolchain changes its metadata shape, and it catches
# EVERY intruder at once: a second preload-file, an audio file, a
# painting, a stray asset. Any of them moves the digest.
def assert_package_is_shader_only(data_path, shader_bytes):
    """Returns a reason string, or None when the package is exactly the shader."""
    try:
        with open(data_path, "rb") as fh:
            packed = fh.read()
    except OSError as e:
        return "cannot be read (%s)" % e
    if len(packed) != len(shader_bytes):
        return ("%d bytes packed against a %d-byte world.wgsl — the package "
                "holds something besides the shader" % (len(packed), len(shader_bytes)))
    if hashlib.sha256(packed).hexdigest() != hashlib.sha256(shader_bytes).hexdigest():
        return ("same length as world.wgsl but a different digest — the package "
                "holds the wrong bytes (a stale link, or a substitution)")
    return None


# ═══ AUBADE U7/U8 — NOTHING THE PAGE CAN FETCH IS THE EXHIBITION ═════
#
# THE LAW IS ABOUT FETCHING, NOT NAMING, and the distinction is the whole
# function. The page DOES name a tune —
#
#     <audio id="music" loop preload="none" data-src="music/samsara.mp3">
#
# — and that is lawful and deliberate: `data-src` is not a source, and
# `preload="none"` tells the browser to fetch nothing until something
# asks. The audio element is armed by the entry gesture and not before.
# A guard that refused on the STRING would refuse the shape the law
# actually wants, and the first person to hit it would delete the guard.
#
# So the two rules are stated as what they are:
#
#   (a) no src= or href= anywhere in the page resolves under paintings/
#       or music/ — those are the attributes a browser acts on;
#   (b) every <audio> and <video> carries preload="none", so an element
#       that gains a real src later still fetches nothing at boot.
#
# Rule (b) is the one that matters for the future: (a) is a fact about
# today's page, (b) is the property that keeps it true.
AUDIO_PRELOAD_RULE = 'preload="none"'


def boot_set_violations(shell_text):
    """Reasons the shipped page would fetch exhibition bytes at boot."""
    bad = []
    # (a) THE ATTRIBUTES A BROWSER ACTS ON, over the WHOLE page — a URL
    #     built inside a script string is as real a fetch as one in
    #     markup. The lookbehind is what keeps `data-src` lawful: it is a
    #     dataset key, not a source, and the audio element uses it
    #     precisely so nothing is fetched until the gesture.
    for attr in ("src", "href"):
        for m in re.finditer(r'(?<![-\w])' + attr + r'\s*=\s*"([^"]*)"', shell_text):
            url = m.group(1).strip()
            while url.startswith("./"):
                url = url[2:]
            for folder, what in (("paintings/", "a painting"),
                                 ("music/", "an audio file")):
                if url.startswith(folder):
                    bad.append('index.html has %s="%s" — %s, fetched at boot'
                               % (attr, m.group(1), what))
    # (b) THE MEDIA ELEMENTS, over the MARKUP ONLY. The page's own
    #     comments discuss `<video>` in prose twice, inside the stylesheet,
    #     and a guard that cannot tell an element from a sentence about one
    #     is a guard someone deletes. Style blocks and HTML comments come
    #     out first; script blocks stay, because a media element built
    #     there would be a real one.
    markup = re.sub(r"<style\b[^>]*>.*?</style>", "", shell_text, flags=re.S | re.I)
    markup = re.sub(r"<!--.*?-->", "", markup, flags=re.S)
    for m in re.finditer(r"<(audio|video)(\s[^>]*)?>", markup, re.I):
        if AUDIO_PRELOAD_RULE not in (m.group(2) or ""):
            bad.append("index.html has a <%s> without %s — it may fetch at boot"
                       % (m.group(1).lower(), AUDIO_PRELOAD_RULE))
    return bad


def list_music():
    if not os.path.isdir(SRC_MUSIC):
        return []
    names = [
        f for f in os.listdir(SRC_MUSIC)
        if os.path.isfile(os.path.join(SRC_MUSIC, f))
        and os.path.splitext(f)[1].lower() == ".mp3"
    ]
    names.sort()
    return names


def dir_bytes(paths):
    return sum(os.path.getsize(p) for p in paths if os.path.isfile(p))


def write_paintings(names):
    """Re-encode into dist/paintings/, capped and requantized.

    THE OPTIONAL MUST NOT BE ABLE TO FAIL THE DIST. Pillow is a
    convenience — without it every painting still ships, just at its
    authored size. The hint prints once, the copy happens anyway.
    """
    os.makedirs(DIST_PAINTINGS, exist_ok=True)
    try:
        from PIL import Image
    except ImportError:
        print("  (Pillow absent — copying paintings verbatim, uncapped.")
        print("   pip install pillow   for the %dpx / q%d re-encode.)"
              % (PAINTING_CAP, PAINTING_QUALITY))
        for f in names:
            shutil.copy2(os.path.join(SRC_PAINTINGS, f), os.path.join(DIST_PAINTINGS, f))
        return [os.path.join(DIST_PAINTINGS, f) for f in names]

    # The ORIGINAL filename is kept, .jpeg and .jpg alike: exhibition.json
    # names the file that exists in dist/, the program sniffs content not
    # extension (stb_image), and both extensions serve as image/jpeg.
    for f in names:
        src = os.path.join(SRC_PAINTINGS, f)
        dst = os.path.join(DIST_PAINTINGS, f)
        try:
            with Image.open(src) as im:
                im = im.convert("RGB")
                w, h = im.size
                if max(w, h) > PAINTING_CAP:
                    im.thumbnail((PAINTING_CAP, PAINTING_CAP), Image.LANCZOS)
                im.save(dst, "JPEG", quality=PAINTING_QUALITY, optimize=True)
        except Exception as e:
            # One unreadable file is not a reason to ship no exhibition.
            print("  (re-encode failed for %s: %s — copied verbatim)" % (f, e))
            shutil.copy2(src, dst)
    return [os.path.join(DIST_PAINTINGS, f) for f in names]


def resolve_poster_sources():
    """Turn each POSTERS role into a real file under assets/entrance/ or
    assets/paintings/.

    ENTRANCE_0 — THE ENTRANCE'S IMAGES ARE NOT THE SHOW'S. A poster the page
    fetches at boot and a canvas that hangs on a wall are two jobs, and until
    this campaign one folder held both: the page's posters resolved out of
    assets/paintings/, so the image greeting a visitor was also one they would
    meet again on a wall. assets/entrance/ is the answer, and its whole
    strength is structural rather than clerical — list_paintings() reads
    SRC_PAINTINGS and only SRC_PAINTINGS, so the exhibition scan CANNOT REACH
    the entrance folder. Nothing has to remember a rule.

    THE TRAP THIS REPLACES, SAID OUT LOUD. Nothing filters on the PAINTING_
    prefix here — any stem in either folder resolves — so the separation the
    tree relied on before this was an accident of naming that no line claimed
    was deliberate. Point a POSTERS role back at a stem in assets/paintings/
    and you put a hung canvas on the boot's critical path, exactly as
    write_posters' banner warns; the folder is what makes that a choice
    somebody has to make rather than a default.

    AN ABSENT assets/entrance/ IS NOT AN ERROR, on preset_files' precedent: a
    folder that is not there yields nothing and says nothing. The only refusal
    is the caller's existing one — a role that resolves to NOTHING refuses the
    dist at exit 5 — and that is what turns a missing entrance image into a
    stopped deploy rather than a hole on the page.

    THE ENTRANCE WINS A TIE. Two folders can hold the same stem; the entrance
    is the more specific home and the one a role is likelier to have meant.

    THE EXTENSION IS READ, NEVER ASSUMED. Either folder may mix .jpg and .jpeg
    because the program's scan takes both, so the table names STEMS and
    this finds whichever one is actually there. The stem match is exact
    and case-sensitive: Cloudflare's URLs are, and a poster that resolves
    here but 404s in production is the worst of both.

    Returns (resolved, missing) — resolved maps output name -> source
    path, missing lists the (output name, role) pairs with nothing behind
    them. Neither writes anything; the refusal is the caller's.
    """
    # STEM -> FULL PATH, not stem -> filename: with two source folders a bare
    # name is ambiguous and a caller that re-joined one of them would read the
    # wrong directory in silence. The path is carried the whole way; nothing
    # downstream joins a source folder again.
    by_stem = {}
    for folder in (SRC_PAINTINGS, SRC_ENTRANCE):   # entrance second, so it wins
        if not os.path.isdir(folder):
            continue
        # Sorted, so a folder holding both NAME.jpg and NAME.jpeg resolves the
        # same way on every run rather than following whatever order listdir
        # happens to give.
        for f in sorted(os.listdir(folder)):
            stem, ext = os.path.splitext(f)
            full = os.path.join(folder, f)
            if ext.lower() in PAINTING_EXTS and os.path.isfile(full):
                if folder is SRC_ENTRANCE:
                    by_stem[stem] = full            # the entrance overrules
                else:
                    by_stem.setdefault(stem, full)

    resolved = {}
    missing = []
    for out_name in sorted(POSTERS):
        role = POSTERS[out_name]
        found = by_stem.get(role)
        if found is None:
            missing.append((out_name, role))
        else:
            resolved[out_name] = found
    return resolved, missing


def write_posters(resolved):
    """Bake the page's two posters into dist/, beside index.html.

    NOT THE EXHIBITION, AND DELIBERATELY DUPLICATED. These never enter
    exhibition.json, and PAINTING_50 and PAINTING_200 keep shipping
    full-size under paintings/ as hung canvases — two consumers, two
    sizes, on purpose. Anyone later "fixing" the duplication by pointing
    the page at paintings/PAINTING_50.jpeg puts a half-megabyte
    exhibition JPEG on the boot's critical path to fill a 340px box.

    NATIVE ASPECT, NOT 2:3. The crop is a CSS decision (.poster in
    index.html) and stays one, so it can change without re-running this
    script or re-uploading anything.

    NO BUILD-ID SUFFIX. BUILDID_0 exists because glue and wasm can pair
    mismatched — a correctness failure. A stale poster is one old
    painting for one revalidation. The reason does not extend, so the
    mechanism does not.

    Returns (paths, failures).
    """
    paths = []
    failures = []
    try:
        from PIL import Image
    except ImportError:
        Image = None
        print("  (Pillow absent — posters copied verbatim at their authored size.")
        print("   pip install pillow   for the %dpx / q%d bake.)"
              % (POSTER_WIDTH, POSTER_QUALITY))

    for out_name in sorted(resolved):
        role = POSTERS[out_name]
        src = resolved[out_name]
        dst = os.path.join(DIST, out_name)
        try:
            if Image is None:
                # THE OPTIONAL STILL MUST NOT FAIL THE DIST — write_paintings
                # rules the same way on the same library. A poster at its
                # authored size is a heavier download, not a broken page, so
                # it ships. Pillow does not become mandatory because POSTER_0
                # arrived.
                shutil.copy2(src, dst)
            else:
                with Image.open(src) as im:
                    im = im.convert("RGB")
                    w, h = im.size
                    if w > POSTER_WIDTH:
                        im = im.resize(
                            (POSTER_WIDTH, max(1, int(round(h * POSTER_WIDTH / float(w))))),
                            Image.LANCZOS)
                    # Never scales UP, for the reason PAINTING_CAP already
                    # gives: invented pixels are bytes over the wire that
                    # carry no detail. A source narrower than the target
                    # ships at the width it was authored at.
                    im.save(dst, "JPEG", quality=POSTER_QUALITY,
                            optimize=True, progressive=True)
        except Exception as e:
            # UNLIKE A PAINTING, A POSTER THAT FAILS IS A HOLE IN THE PAGE.
            # write_paintings copies verbatim and moves on because one
            # unreadable canvas is not a reason to ship no exhibition. Here
            # the same fallback would push bytes that just refused to open
            # into the slot whose entire job is looking intact.
            failures.append((out_name, role, src, "%s: %s" % (type(e).__name__, e)))
            continue

        if not os.path.isfile(dst) or os.path.getsize(dst) == 0:
            failures.append((out_name, role, src, "wrote 0 bytes"))
            continue
        paths.append(dst)

    return paths, failures


# THE MARKERS. Each is a format-string literal that exists only inside an
# `if constexpr (INSTRUMENTS.frame_meter)` block, so a release build emits
# none of them. Two rooms — the cartridge's window table and the console's
# present histogram — because a marker set drawn from one file is a marker
# set one refactor can empty.
INSTRUMENT_MARKERS = [
    b"[METER] mesh-gen firings",   # cartridge.hpp, the window's firing line
    b"-> purse ",                  # cartridge.hpp, the window header's envelope
    b"[PRESENT] refresh ",         # console.hpp, the present histogram
]


def instruments_verdict():
    """(is_instrumented, markers_found) — or (None, []) if the wasm is absent.

    A byte scan, deliberately: the strings live in the wasm's data segment and
    asking the artifact is the one question a build cannot answer wrongly."""
    wasm = os.path.join(WEB, "the_board.wasm")
    if not os.path.isfile(wasm):
        return None, []
    with open(wasm, "rb") as fh:
        blob = fh.read()
    found = [m.decode("ascii") for m in INSTRUMENT_MARKERS if m in blob]
    return (len(found) > 0), found


def main():
    ap = argparse.ArgumentParser(description="SHIP_0 U4 — web dist assembly + host verdict")
    ap.add_argument("--check", action="store_true", help="inventory and verdict only; write nothing")
    ap.add_argument("--lab", action="store_true",
                    help="permit writing dist/ from an instrumented (meter) build — a capture, not a deploy")
    args = ap.parse_args()

    missing = [f for f in ARTIFACTS if not os.path.isfile(os.path.join(WEB, f))]
    present = [f for f in ARTIFACTS if f not in missing]

    print("web output inventory  (%s)" % WEB)
    print("  %-18s %14s  %9s" % ("file", "bytes", "MiB"))
    total = 0
    sizes = {}
    for f in present:
        n = os.path.getsize(os.path.join(WEB, f))
        sizes[f] = n
        total += n
        print("  %-18s %14d  %9.2f" % (f, n, mib(n)))
    for f in missing:
        print("  %-18s %14s  %9s" % (f, "MISSING", "-"))
    print("  %-18s %14d  %9.2f" % ("TOTAL", total, mib(total)))

    # THE EXHIBITION, COUNTED SEPARATELY BECAUSE IT SHIPS SEPARATELY.
    # These bytes are not in the four files above and never will be
    # again — they are fetched by URL at runtime. Source sizes here;
    # the re-encoded dist figures print after the write.
    paintings = list_paintings()
    music = list_music()
    # POSTER_0 — resolved here, refused later. This reads the directory and
    # writes nothing, so it is safe this early and its answer is needed by
    # the verdict below.
    poster_src, poster_missing = resolve_poster_sources()
    paintings_src_bytes = dir_bytes([os.path.join(SRC_PAINTINGS, f) for f in paintings])
    music_src_bytes = dir_bytes([os.path.join(SRC_MUSIC, f) for f in music])
    print("")
    print("exhibition inventory  (%s)" % os.path.join(ROOT, "assets"))
    print("  %-18s %14s  %9s  %7s" % ("kind", "bytes", "MiB", "files"))
    print("  %-18s %14d  %9.2f  %7d"
          % ("paintings (src)", paintings_src_bytes, mib(paintings_src_bytes), len(paintings)))
    print("  %-18s %14d  %9.2f  %7d"
          % ("music", music_src_bytes, mib(music_src_bytes), len(music)))
    print("  %-18s %14d  %9.2f  %7d"
          % ("TOTAL", paintings_src_bytes + music_src_bytes,
             mib(paintings_src_bytes + music_src_bytes),
             len(paintings) + len(music)))
    if not paintings:
        print("  (no PAINTING_*.jpg|.jpeg under assets/paintings — the dist will")
        print("   carry an empty exhibition, which the program reads as no paintings.)")

    if missing:
        print("")
        print("BUILD FIRST — %d artifact(s) absent." % len(missing))
        print("  cmake --preset the-board-web && cmake --build --preset the-board-web")
        print("(the three build outputs land in web/ beside the tracked index.html;")
        print(" .gitignore keeps them out of the tree on purpose.)")
        return 2

    # THE DOWNLOAD COST, called out on its own line because it is the
    # mobile number that matters and U4 asks for it explicitly. Recorded,
    # NOT optimized — Jean's directive parks that until the phone verdict.
    data = sizes["the_board.data"]
    print("")
    print("MOBILE DOWNLOAD COST")
    print("  the_board.data   %d bytes  (%.2f MiB)   <- world.wgsl (the program's shader)" % (data, mib(data)))
    print("  wasm + js        %d bytes  (%.2f MiB)" % (
        sizes["the_board.wasm"] + sizes["the_board.js"],
        mib(sizes["the_board.wasm"] + sizes["the_board.js"])))
    print("  first visit      %d bytes  (%.2f MiB) uncompressed" % (total, mib(total)))
    print("  exhibition       %d bytes  (%.2f MiB) source, fetched AFTER first paint"
          % (paintings_src_bytes + music_src_bytes, mib(paintings_src_bytes + music_src_bytes)))
    print("  Both hosts serve br/gzip for js/html; .wasm and .data compress well over the")
    print("  wire, so the number a phone actually pays is lower than the figure above.")
    print("  RECORD IT. Do not optimize it (SHIP_0 U4).")

    # THE RULE SAYS EVERY FILE, AND dist/ IS NO LONGER FOUR FILES. The
    # exhibition ships in the same folder under the same per-file cap, so
    # it is weighed by the same three-way branch — not printed beside it
    # as a remark the verdict ignores. Source sizes, because the verdict
    # runs before the write and must also hold under --check: music is
    # copied verbatim so its source size IS its dist size, and paintings
    # only ever shrink, so a source size is a safe upper bound.
    verdict_sizes = dict(sizes)
    for f in preset_files():
        verdict_sizes["presets/" + f] = os.path.getsize(os.path.join(SRC_PRESETS, f))
    for f in paintings:
        verdict_sizes["paintings/" + f] = os.path.getsize(os.path.join(SRC_PAINTINGS, f))
    for f in music:
        verdict_sizes["music/" + f] = os.path.getsize(os.path.join(SRC_MUSIC, f))
    # POSTER_0 — the posters are files in dist/, and the rule says every
    # file. Source size again, and an upper bound for the same reason: a
    # poster only ever shrinks from the painting behind it, so a source
    # that clears the cap guarantees the written poster does. At a few
    # hundred KB they cannot move this verdict — they are counted so the
    # file COUNT printed below is not a lie.
    for out_name, src in poster_src.items():
        verdict_sizes[out_name] = os.path.getsize(src)

    biggest = max(verdict_sizes, key=lambda k: verdict_sizes[k])
    biggest_n = verdict_sizes[biggest]
    print("")
    print("HOST VERDICT (by the numbers)")
    print("  largest single file: %s at %.2f MiB   (of %d files in dist/)"
          % (biggest, mib(biggest_n), len(verdict_sizes) + 1))
    if biggest_n <= CF_LIMIT:
        print("  -> CLOUDFLARE PAGES. Every file is within its 25 MiB per-file limit.")
        host = "cloudflare"
    elif biggest_n <= GH_LIMIT:
        print("  -> GITHUB PAGES. %s exceeds Cloudflare's 25 MiB; GitHub's ~100 MiB holds."
              % biggest)
        host = "github"
    else:
        print("  -> RESOLVE. %s exceeds BOTH limits (%.2f MiB)." % (biggest, mib(biggest_n)))
        print("     Not repackaging on my own authority. Options, for Jean's ruling:")
        print("       a. split the preload — ship a starter asset set, fetch the rest at runtime")
        print("       b. drop the preload and fetch assets over HTTP (needs a loader path)")
        print("       c. a host with no per-file cap (S3/R2 + CDN)")
        print("     If the offender is an exhibition file, a fourth option exists that the")
        print("     other three did not have: re-encode or split THAT FILE. It is not")
        print("     welded into the bundle any more.")
        return 3

    if args.check:
        print("")
        print("(--check: nothing written)")
        return 0

    # THE REFUSAL COMES FIRST, before rmtree — a shell that cannot be
    # versioned must not cost the previous dist. Without this check a
    # future edit to index.html silently restores the stale-pair defect
    # and the symptom reappears weeks later on someone else's phone.
    shell_src_path = os.path.join(WEB, "index.html")
    with open(shell_src_path, "r", encoding="utf-8", newline="") as fh:
        shell_src = fh.read()
    if BUILD_ID_PLACEHOLDER not in shell_src:
        print("")
        print("REFUSING TO SHIP AN UNVERSIONED SHELL.")
        print("  %s carries no %s placeholder." % (shell_src_path, BUILD_ID_PLACEHOLDER))
        print("  Without it the .js/.wasm/.data URLs are constant across deploys and a")
        print("  browser can pair a cached file with a fresh sibling — old glue against")
        print("  new wasm. Restore `var BUILD = '%s';` in the shell." % BUILD_ID_PLACEHOLDER)
        return 4

    # PROBATE_SEAL2 — THE THIRD REFUSAL, and for the reason the first two
    # come before rmtree. A shell with no digest slot ships a page whose
    # serve nothing witnesses, which is precisely the state the Pixel
    # incident found the program in. Shipping that silently would retire
    # the witness by omission — the one failure mode a witness must not
    # ── WRAP_0 U7 — THE AUDIENCE DOES NOT GET THE LAB'S BUILD ────────────
    #
    # `[METER]`, `[PRESENT]` and `[STREAM]` were printing from
    # everexpandingboard.com, because the two presets differ only by a CMake
    # cache variable and nothing downstream could tell them apart. The meter
    # is not free where it is most expensive: its timestamp writes at every
    # pass boundary serialize passes a tiler would otherwise overlap — which
    # is the floor device — and its census prints block the main thread for
    # tens of milliseconds a firing.
    #
    # THE ARTIFACT IS ASKED, NOT THE BUILD SYSTEM. A stamp written by CMake
    # is a stamp a build can forget; the wasm either contains the
    # instruments' format strings or it does not, because the whole meter
    # rides `if constexpr (INSTRUMENTS.frame_meter)` and a false constexpr
    # emits no literal. Three markers rather than one, from two rooms, so a
    # single reworded line cannot silently disarm the check.
    if not args.check:
        verdict, found = instruments_verdict()
        print("")
        if verdict is None:
            print("BUILD PRESET: UNREADABLE — %s is missing; cannot tell lab from audience." % ARTIFACTS[3])
        elif verdict:
            print("BUILD PRESET: THE-BOARD-WEB-METER (instrumented).")
            for m in found:
                print("    marker present: %s" % m)
            if not args.lab:
                print("")
                print("REFUSING TO SHIP THE LAB'S BUILD.")
                print("  This wasm carries the frame meter. Its timestamp writes serialize")
                print("  passes a tile-based GPU would overlap, and its census prints block")
                print("  the main thread — both worst on the floor device.")
                print("  Build the audience's preset:")
                print("    cmake --preset the-board-web && cmake --build --preset the-board-web")
                print("  Or pass --lab if this dist IS a capture and you mean it.")
                return 7
            print("  --lab given: writing an instrumented dist deliberately.")
        else:
            print("BUILD PRESET: the-board-web (audience). No instrument markers in the wasm.")

    # have.
    if SHADER_SHA_PLACEHOLDER not in shell_src:
        print("")
        print("REFUSING TO SHIP AN UNWITNESSED SERVE.")
        print("  %s carries no %s placeholder." % (shell_src_path, SHADER_SHA_PLACEHOLDER))
        print("  Without it the boot cannot compare the shader it received against the")
        print("  shader this build packaged, and a truncated or stale world.wgsl reaches")
        print("  the audience as a syntax error in a file that has none (PROBATE_SEAL).")
        print("  Restore `var SHADER_SHA = '%s';` in the shell." % SHADER_SHA_PLACEHOLDER)
        return 6
    if not os.path.isfile(SHADER_SRC):
        print("")
        print("REFUSING TO SHIP: %s is missing — nothing to witness." % SHADER_SRC)
        return 6

    # POSTER_0 — THE SECOND REFUSAL, and before rmtree for the reason the
    # first one is: a dist that cannot be completed must not cost the
    # previous one. A renamed PAINTING_200 fails the build loudly here
    # rather than shipping a hole on the page whose entire job is behaving
    # when things are broken.
    if poster_missing:
        print("")
        print("REFUSING TO SHIP A POSTER WITH NOTHING BEHIND IT.")
        for out_name, role in poster_missing:
            print("  %-16s  role %s — no %s{%s} under %s"
                  % (out_name, role, role, ",".join(PAINTING_EXTS), SRC_PAINTINGS))
        print("  The page hard-codes these filenames and knows nothing else; POSTERS in")
        print("  this script is the only place that says which painting each one is.")
        print("  Restore the file, or re-point the role in POSTERS and redeploy — either")
        print("  way index.html does not change.")
        return 5

    # ── WEBSITE_1 — THE ENGINE OWNS NAMES, NOT THE FOLDER ───────────
    # dist/ is shared ground: about/, collection/, fonts/ and shared.css
    # are the site's, written by their own pipelines (MAIN/MERGE.md).
    # This script deletes exactly what it writes — the names below, every
    # one already a constant in this file — and treats anything else as a
    # tenant it does not know. That is the whole agnosticism: the site's
    # names never enter this script, so the site can grow, rename or
    # vanish without it hearing.
    owned = ARTIFACTS + list(POSTERS) + [
        EXHIBITION_JSON, "_headers", "_redirects",
        os.path.basename(DIST_PRESETS),
        os.path.basename(DIST_PAINTINGS),
        os.path.basename(DIST_MUSIC),
    ]
    for name in owned:
        p = os.path.join(DIST, name)
        if os.path.isdir(p):
            shutil.rmtree(p)
        elif os.path.isfile(p):
            os.remove(p)
    os.makedirs(DIST, exist_ok=True)
    for f in ARTIFACTS:
        shutil.copy2(os.path.join(WEB, f), os.path.join(DIST, f))

    # ORGAN_4 P6 — the preset shelf, copied verbatim. The panel fetches
    # `presets/index.json` relative to the page, so the folder has to be
    # beside index.html in dist/ or the layer simply is not there.
    presets = preset_files()
    if presets:
        os.makedirs(DIST_PRESETS, exist_ok=True)
        for f in presets:
            shutil.copy2(os.path.join(SRC_PRESETS, f), os.path.join(DIST_PRESETS, f))

    # dist/index.html is GENERATED from here on, not copied. The hash is
    # taken from the file that actually shipped, so the id names the
    # bytes a visitor will run.
    with open(os.path.join(DIST, "the_board.wasm"), "rb") as fh:
        build_id = hashlib.sha256(fh.read()).hexdigest()[:BUILD_ID_LEN]
    # PROBATE_SEAL2 — read as BYTES, hashed whole. No decode, no newline
    # translation, no substitution: the digest is of the file as it sits
    # on disk, which is the only thing --preload-file packs. Text mode
    # here would make the two ends disagree on every CRLF host, and a
    # witness that reports MISMATCH on a good serve gets switched off.
    with open(SHADER_SRC, "rb") as fh:
        shader_bytes = fh.read()
    shader_sha_full = hashlib.sha256(shader_bytes).hexdigest()
    shader_sha = shader_sha_full[:SHADER_SHA_LEN]
    shell_out = shell_src.replace(BUILD_ID_PLACEHOLDER, build_id)
    shell_out = shell_out.replace(SHADER_SHA_PLACEHOLDER, shader_sha)

    # ── AUBADE U7 — FIRST LIGHT STARTS AT HTML PARSE ────────────────
    #
    # The glue is appended to <body> by the inline boot script, and the
    # wasm and the package are fetched by the glue after IT has parsed.
    # So the browser's preload scanner — which runs ahead of the parser
    # and exists precisely to start long fetches early — never saw any of
    # them. Three <link rel=preload> in the head and it does.
    #
    # INJECTED HERE, NOT WRITTEN INTO web/index.html, and the reason is a
    # law two blocks up: the build-id placeholder appears EXACTLY ONCE in
    # the source page so the substitution has one target and the
    # refusal-to-ship check has one thing to count. Two of these hrefs
    # carry the id, so they cannot live in a file that holds the token
    # once. (web/index.html does not boot anyway — it is a source file.)
    #
    # THE URLS MUST MATCH THE REAL REQUESTS EXACTLY, query included, or
    # the browser warns and fetches twice. They are built from the same
    # build_id the page is built with, in the same instant, which is the
    # only way to be sure.
    #
    # THE VEIL'S POSTER IS FIRST because it is the only one of the three
    # a visitor can SEE. It is unversioned (POSTER_0: the page names
    # veil_poster.jpg and nothing else), so no id here.
    #
    # THE PAGE IS READ WITH newline="" — no translation — so a CRLF
    # checkout arrives with CRLF and an injection hard-coded to \n would
    # both miss its anchor and leave one mixed line behind. The line
    # ending is taken from the file itself.
    head_at = shell_out.find("<head>")
    if head_at < 0:
        print("")
        print("REFUSING TO SHIP A PAGE WITH NOWHERE TO PUT THE PRELOADS.")
        print("  web/index.html has no `<head>`, so the injection point this")
        print("  build depends on is gone. Restore it, or move the preload block")
        print("  to wherever the head now opens.")
        return 7
    eol_at = shell_out.find("\n", head_at)
    if eol_at < 0:
        print("")
        print("REFUSING: `<head>` is on the page's last line — nowhere to inject.")
        return 7
    eol = "\r\n" if shell_out[max(0, eol_at - 1)] == "\r" else "\n"
    preloads = (
        '  <link rel="preload" as="image" href="veil_poster.jpg">' + eol +
        '  <link rel="preload" as="script" href="the_board.js?v=%s">' % build_id + eol +
        '  <link rel="preload" as="fetch" crossorigin href="the_board.wasm?v=%s">' % build_id + eol)
    shell_out = shell_out[:eol_at + 1] + preloads + shell_out[eol_at + 1:]
    with open(os.path.join(DIST, "index.html"), "w", encoding="utf-8", newline="") as fh:
        fh.write(shell_out)

    # POSTER_0 — written here, beside index.html, because that is what they
    # belong to. The exhibition is assembled after them and separately.
    print("")
    print("POSTERS — baking %d page asset(s) at %dpx / q%d"
          % (len(poster_src), POSTER_WIDTH, POSTER_QUALITY))
    poster_paths, poster_failures = write_posters(poster_src)
    if poster_failures:
        print("")
        print("REFUSING TO SHIP A BROKEN POSTER.")
        for out_name, role, src, why in poster_failures:
            print("  %-16s  role %s" % (out_name, role))
            print("      source  %s" % src)
            print("      %s" % why)
        print("  dist/ is part-written and NOT deployable: the page would show a hole")
        print("  where a painting belongs. Fix the source, or re-point the role in")
        print("  POSTERS. A poster is not a painting — write_paintings survives one bad")
        print("  file because the wall has others to hang; this slot has exactly one.")
        return 5

    print("")
    print("EXHIBITION — assembling %d painting(s), %d track(s)"
          % (len(paintings), len(music)))
    painting_paths = write_paintings(paintings)

    # ── AUBADE U7 — THE THREE ASSERTS, BEFORE ANYTHING ELSE IS WRITTEN ──
    #
    # THE PACKAGE. One --preload-file, world.wgsl, nothing beside it.
    why = assert_package_is_shader_only(
        os.path.join(DIST, "the_board.data"), shader_bytes)
    if why:
        print("")
        print("REFUSING TO SHIP A POLLUTED PACKAGE.")
        print("  the_board.data %s" % why)
        print("  The package is fetched BEFORE first light, in full, by every")
        print("  visitor on every cold load. It carries the shader because the")
        print("  shader is first light; anything else in it is a byte the dawn")
        print("  pays for. Check CMakeLists' --preload-file list (T7_WEB_SHADER).")
        print("  dist/ is part-written and NOT deployable.")
        return 7

    # THE BOOT SET. No painting, no audio, anywhere the page can FETCH
    # from before the world is on screen. index.html is the one that can
    # go wrong quietly — a poster re-pointed at a hung canvas, an <audio
    # src> added for a splash — so it is read, not assumed.
    boot_bad = boot_set_violations(shell_out)
    if boot_bad:
        print("")
        print("REFUSING TO SHIP A BOOT SET WITH THE EXHIBITION IN IT.")
        for b in boot_bad:
            print("  %s" % b)
        print("  The boot payload is O(first light) and must stay invariant as the")
        print("  catalogue and the library grow. A painting or a tune named by the")
        print("  page is fetched before the world, by everyone, forever.")
        print("  dist/ is part-written and NOT deployable.")
        return 7

    # THE MUSIC CEILING (RUL-E). One file per tune, under AUDIO_CEILING.
    over_ceiling = [(f, os.path.getsize(os.path.join(SRC_MUSIC, f)))
                    for f in music
                    if os.path.getsize(os.path.join(SRC_MUSIC, f)) > AUDIO_CEILING]
    if over_ceiling:
        print("")
        print("REFUSING TO SHIP A TUNE OVER THE CEILING.")
        for f, b in over_ceiling:
            print("  music/%-32s %d bytes (%.2f MiB) over the %.0f MiB ceiling"
                  % (f, b, mib(b), mib(AUDIO_CEILING)))
        print("  RUL-E: the music law is a SIZE law, not a codec migration. Codec")
        print("  and bitrate stay as authored; a tune projected past the ceiling")
        print("  SPLITS at a movement boundary at export time. The ceiling sits")
        print("  five MiB under Cloudflare's per-file limit so hitting it is a")
        print("  conversation, not a failed deploy.")
        print("  dist/ is part-written and NOT deployable.")
        return 7

    # AUBADE U5c — the cap, on what landed, on every path.
    over_cap = assert_painting_cap(painting_paths)
    if over_cap:
        print("")
        print("REFUSING TO SHIP A PAINTING OVER THE DECODE CAP.")
        for p, why in over_cap:
            print("  %-44s %s" % (os.path.relpath(p, DIST), why))
        print("  The runtime decodes each of these on the MAIN THREAD and pads the")
        print("  result to PAINTING_RESOLUTION, so every texel over the cap is paid")
        print("  for and thrown away — in the frames right after first light, which")
        print("  is the window this build is trying to keep clear. Install Pillow so")
        print("  write_paintings can requantize, or cap the sources by hand.")
        print("  dist/ is part-written and NOT deployable.")
        return 6

    os.makedirs(DIST_MUSIC, exist_ok=True)
    for f in music:
        shutil.copy2(os.path.join(SRC_MUSIC, f), os.path.join(DIST_MUSIC, f))
    music_paths = [os.path.join(DIST_MUSIC, f) for f in music]

    # THE MANIFEST IS FILENAMES, NOT PATHS. The program joins the folder
    # itself (paintings/<name>, music/<name>), so a path here would be a
    # second place that decides the layout.
    # ensure_ascii=False, and it is load-bearing. The default escapes any
    # non-ASCII character as \uXXXX, and the program's manifest parse is
    # a by-hand string scan with no unescaping — it would hand
    # "PAINTING_café.jpeg" to the fetch verbatim and take a 404 for
    # a file that is sitting right there. The handle is already utf-8 and
    # the C++ side reads raw bytes.
    with open(os.path.join(DIST, EXHIBITION_JSON), "w", encoding="utf-8", newline="\n") as fh:
        json.dump({"paintings": paintings, "music": music},
                  fh, indent=2, ensure_ascii=False)
        fh.write("\n")

    # ── REPEAT_0a U0b — THE DIST PROVES ITS OWN EXHIBITION ───────────
    # The reasoning lives on assert_exhibition_written. It refuses rather
    # than warns: a manifest that names a file dist/ does not have is a lie
    # this script is the last thing in the chain able to see.
    unkept = assert_exhibition_written(paintings)
    if unkept:
        print("")
        print("REFUSING TO SHIP AN EXHIBITION THAT DOES NOT EXIST.")
        print("  %d promise(s) in %s that dist/ does not keep:"
              % (len(unkept), EXHIBITION_JSON))
        for name, why in unkept[:12]:
            print("    %-28s %s" % (name, why))
        if len(unkept) > 12:
            print("    ... and %d more" % (len(unkept) - 12))
        print("")
        print("  The program fetches exactly what exhibition.json names, one index at")
        print("  a time, and a name with nothing behind it costs a failed round trip")
        print("  it can never get back. write_paintings and the manifest are written")
        print("  from ONE list (list_paintings), so this cannot happen by accident —")
        print("  if it fired, something edited dist/ between the two, or a write")
        print("  failed silently. Do not deploy this tree.")
        return 8

    paintings_dist_bytes = dir_bytes(painting_paths)
    music_dist_bytes = dir_bytes(music_paths)
    poster_dist_bytes = dir_bytes(poster_paths)
    # ── GATEHOUSE_G4 — THE CACHE LAW, THE CORRIDOR'S LAST DOOR ───────
    #
    # The Pixel replayed yesterday's build, and the versioning was not
    # the problem — BUILDID_0 works. The problem is one layer up: the
    # `?v=` keys are only as fresh as THE PAGE THAT NAMES THEM. A cached
    # index.html hands out yesterday's keys, and every asset behind them
    # is then correctly, obediently stale. The stale-pair defect
    # BUILDID_0 closed for siblings reopens for the whole build the
    # moment the index itself is the cached object.
    #
    # Cloudflare Pages reads dist/_headers. The index is no-cache — it
    # must be revalidated on every load, so a plain reload always fetches
    # the page that names the current keys. NOTHING ELSE IS LISTED: the
    # versioned assets keep default caching precisely BECAUSE a fresh
    # index always names fresh keys, and busting them too would throw
    # away the whole point of the id.
    #
    # `no-cache` and not `no-store`: revalidate, do not refuse to keep.
    # A 304 on an unchanged index is free and correct; what must never
    # happen is the browser answering from its own copy without asking.
    #
    # Written HERE, after everything else, on the refusal-safe path —
    # like the rest of dist/, so a run that cannot complete never costs
    # the previous deploy.
    # ── AUBADE U6 — AND THE OTHER HALF: THE SECOND DAWN IS FREE ──────
    #
    # The rule above ADDS; it does not move. The index keeps `no-cache`
    # verbatim, for exactly the reason its own banner gives — a fresh
    # index always names fresh keys — and the four versioned artifacts
    # gain `immutable` beneath it.
    #
    # WHY THIS IS SAFE, and it is the one thing worth checking before
    # believing it: the HTTP cache is keyed on the FULL URL, query string
    # included. Every one of these four is fetched as `<path>?v=<build
    # id>` (index.html's script tags for the two .js, Module.locateFile
    # for the .wasm and the .data), and the build id is
    # sha256(the_board.wasm)[:12]. A new build is a new key, so
    # `immutable` can never pin a stale artifact: it pins a URL that will
    # never be asked for again.
    #
    # WHAT IT BUYS. A returning visitor pays no network for the glue, the
    # wasm or the package — and, more than that, keeps what the browser
    # built FROM them: Chrome's wasm code cache and the implementation's
    # own pipeline cache both key off unchanged bytes, and a
    # revalidation, even a 304, is a chance to lose that. `immutable`
    # says do not even ask.
    #
    # WHAT IS DELIBERATELY ABSENT. paintings/ and music/ are NOT versioned
    # — the manifest names bare filenames and the exhibition changes
    # without the build id moving. `immutable` there would pin a painting
    # for a year past its replacement. They keep default caching, which is
    # revalidation, which is correct for content that can change under a
    # constant name.
    with open(os.path.join(DIST, "_headers"), "w",
              encoding="utf-8", newline="\n") as fh:
        fh.write("/\n  Cache-Control: no-cache\n"
                 "/index.html\n  Cache-Control: no-cache\n")
        for versioned in IMMUTABLE_PATHS:
            fh.write("/%s\n  Cache-Control: %s\n" % (versioned, IMMUTABLE_RULE))
        # ── WEBSITE_1 — THE ROOT FILE HAS ONE WRITER ────────────────
        # Cloudflare reads exactly one _headers, at the deployment root,
        # so this writer is it — for the engine's rules above and the
        # site's below. Site rules are emitted only when their paths
        # exist in dist/, so an engine-only deploy states no law about a
        # folder that is not there. Fonts ship under hashed-stable names
        # and never change in place; the about page changes under a
        # constant name and must revalidate, exactly as / does.
        if os.path.isdir(os.path.join(DIST, "fonts")):
            fh.write("/fonts/*\n  Cache-Control: %s\n" % IMMUTABLE_RULE)
        if os.path.isdir(os.path.join(DIST, "about")):
            fh.write("/about/\n  Cache-Control: no-cache\n")
        # The collection pipeline knows its filenames carry a content
        # hash; its cache law arrives as the fragment it wrote beside
        # its own output, folded here verbatim. Absent fragment, absent
        # rule. Run this script LAST on a full refresh — the fold and
        # the two conditionals above read what the site pipelines wrote.
        frag = os.path.join(DIST, "collection", "_headers.fragment")
        if os.path.isfile(frag):
            with open(frag, "r", encoding="utf-8") as fr:
                txt = fr.read()
            fh.write(txt if txt.endswith("\n") else txt + "\n")

    file_count = (len(ARTIFACTS) + len(painting_paths) + len(music_paths)
                  + len(poster_paths) + len(presets) + 2)

    print("  %-18s %14d  %9.2f  %7d" % ("paintings (dist)", paintings_dist_bytes,
                                        mib(paintings_dist_bytes), len(painting_paths)))
    print("  %-18s %14d  %9.2f  %7d" % ("music (dist)", music_dist_bytes,
                                        mib(music_dist_bytes), len(music_paths)))
    if paintings_src_bytes and paintings_dist_bytes:
        print("  paintings re-encode: %.2f MiB -> %.2f MiB  (%.0f%% of source)"
              % (mib(paintings_src_bytes), mib(paintings_dist_bytes),
                 100.0 * paintings_dist_bytes / paintings_src_bytes))
    # POSTER_0 — page assets, listed with the dist figures and named as not
    # being the show. The per-file lines are here because the veil poster
    # is the one image on the boot's critical path, and a number you have
    # to go and measure is a number nobody measures.
    print("  %-18s %14d  %9.2f  %7d   <- page assets, NOT in %s"
          % ("posters (dist)", poster_dist_bytes, mib(poster_dist_bytes),
             len(poster_paths), EXHIBITION_JSON))
    for p in poster_paths:
        out_name = os.path.basename(p)
        n = os.path.getsize(p)
        # ENTRANCE_0 — the SOURCE FOLDER is named, not just the stem: with two
        # folders resolvable, "which one did this come from" is the question a
        # reader now has, and the answer is one os.path.relpath away.
        print("    %-16s %14d  %9.2f   <- %s (%s)"
              % (out_name, n, mib(n), POSTERS[out_name],
                 os.path.relpath(os.path.dirname(poster_src[out_name]), ROOT)))
    print("  %s: %d painting(s), %d track(s)"
          % (EXHIBITION_JSON, len(paintings), len(music)))
    if presets:
        print("  %-18s %14d  %9.2f  %7d   <- the preset shelf (ORGAN_4 P6); "
              "?preset=<name> picks one at boot"
              % ("presets (dist)",
                 sum(os.path.getsize(os.path.join(DIST_PRESETS, f)) for f in presets),
                 mib(sum(os.path.getsize(os.path.join(DIST_PRESETS, f)) for f in presets)),
                 len(presets)))
    else:
        print("  presets: none — web/presets/ is absent, and the panel hides its select")
    print("  %-18s %s   <- sha256(the_board.wasm)[:%d]; the .js/.wasm/.data query"
          % ("build id", build_id, BUILD_ID_LEN))
    print("  %-18s %s" % ("", "Deploy twice without rebuilding and this must not change."))
    # ORGAN_5 P5a — THE PANEL IS IN THE SAME BOAT, and this line says so
    # because "is the panel versioned too?" is a question the report was
    # letting the reader answer by reading index.html. organ_panel.js
    # carries the same ?v= key, and index.html hands the id to the panel
    # as window.T7_BUILD_ID so the footer and the pill can name the build
    # a phone is actually running. Two devices showing different ids are
    # not the same program.
    print("  %-18s %s"
          % ("", "organ_panel.js?v=%s too; the panel's footer prints `build %s`."
                 % (build_id, build_id)))
    print("  %-18s %s   <- sha256(world.wgsl)[:%d] over %d bytes; the boot compares"
          % ("shader sha", shader_sha, SHADER_SHA_LEN, len(shader_bytes)))
    print("  %-18s %s" % ("", "Pixel console must read: [Dist] world.wgsl sha=%s expected=%s MATCH"
                          % (shader_sha, shader_sha)))

    # The verdict above already weighed these, from source sizes. This is
    # the confirmation on the bytes actually written — and it is a hard
    # stop, not a remark: a dist/ that cannot be deployed must not print
    # deploy instructions.
    exhibition_paths = painting_paths + music_paths
    if exhibition_paths:
        biggest_ex = max(exhibition_paths, key=os.path.getsize)
        ex_n = os.path.getsize(biggest_ex)
        host_limit = CF_LIMIT if host == "cloudflare" else GH_LIMIT
        print("  largest exhibition file (written): %s at %.2f MiB"
              % (os.path.basename(biggest_ex), mib(ex_n)))
        if ex_n > host_limit:
            print("")
            print("  -> RESOLVE. %s is %.2f MiB, past the %.0f MiB per-file limit of the"
                  % (os.path.basename(biggest_ex), mib(ex_n), mib(host_limit)))
            print("     host this run chose. The verdict above weighed SOURCE sizes; this")
            print("     is the written byte count, and it disagrees.")
            print("     dist/ is written but NOT deployable as-is. Re-encode or split")
            print("     that file — it is an exhibition file, not the bundle, so that")
            print("     costs a copy and not a compile.")
            return 3

    print("")
    print("WROTE %s  (%d files)" % (DIST, file_count))
    # REPEAT_0a U0b — the promise, kept, said out loud. The negative is the
    # useful half: if the deployed site still cannot serve a painting, this
    # line has already ruled out everything upstream of the deploy.
    print("  exhibition          %d painting(s) named, %d written, every name has its file"
          % (len(paintings), len(paintings)))
    print("  _headers           index is no-cache; a plain reload now fetches the current build")
    print("                     %d versioned path(s) immutable for a year — the second dawn is free"
          % len(IMMUTABLE_PATHS))

    print("")
    print("DEPLOY — exact commands")
    print("")
    if host == "cloudflare":
        # TIDY_0c-i: the project is named `7t`, not `the-board`. These
        # commands were copy-pasteable and WRONG — `--project-name
        # the-board` names a project that does not exist, so the first
        # one would have created a second, empty one. One home for the
        # deploy truth, and this is it.
        print("  Cloudflare Pages, first time:")
        print("    npm install -g wrangler")
        print("    wrangler login")
        print("    wrangler pages project create 7t --production-branch main")
        print("    wrangler pages deploy dist --project-name 7t")
        print("")
        print("  Every time after:")
        print("    wrangler pages deploy dist --project-name 7t")
        print("")
        print("  Or without the CLI: dash.cloudflare.com -> Workers & Pages -> Create ->")
        print("  Pages -> Upload assets -> drag the dist/ FOLDER in.")
        print("")
        print("  THE DESTINATION: https://everexpandingboard.com is canonical and is")
        print("  the QR destination. https://7t.pages.dev is the raw Pages host —")
        print("  the fallback to check when the custom domain is the suspect.")
    else:
        print("  GitHub Pages, gh-pages branch convention: create an orphan branch that")
        print("  holds ONLY the deployable files, push it, and point Pages at its root")
        print("  (Settings -> Pages -> Source: Deploy from a branch -> gh-pages / (root)).")
        print("  From a clean tree:")
        print("    git switch --orphan gh-pages")
        print("    git rm -rf . >NUL 2>&1")
        print("    cp -r dist/. .         # or: xcopy dist\\* . /E /Y  on cmd")
        print("                           # -r / /E: dist/ has paintings\\ and music\\ now")
        print("    git add -A && git commit -m \"web build\"")
        print("    git push -u origin gh-pages")
        print("    git switch -")
        print("  The URL is https://jeanklein1.github.io/7T-Pawns/ .")
    print("")
    print("  HEADERS: none. The build is single-threaded, so no COOP/COEP is needed")
    print("  and adding them would only risk breaking the load.")
    print("")
    print("  HTTPS is not optional — WebGPU needs a secure context. Both hosts give it;")
    print("  a LAN IP does not, which is why 'just serve it locally' is not a phone test.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
