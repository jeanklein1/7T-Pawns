#!/usr/bin/env python3
# ═══════════════════════════════════════════════════════════════════════
# THE COMMAND LEDGER (DOMESDAY_0 A4) — a read-only census of the
# program's pass and submit surface, and the instrument that
# reproduces it.
#
# WHAT THIS IS. One row per render/compute PASS the program encodes:
# label, color attachments with loadOp/storeOp, depth-stencil
# attachment with loadOp/storeOp (and depthReadOnly where present),
# and the function that encodes it. Plus the queue-submit sites, and
# the swapchain reconfigure trigger quoted verbatim (the condition
# that feeds the [FRAME_1] print — the debounce ruling's evidence).
#
# WHAT THIS IS NOT. It touches no translation unit, no shader, no
# build graph. It reads source files and writes one markdown artifact.
# Skeleton discipline: tools/binding_ledger.py (provenance hashes of
# every scanned file, LF-pinned writer, witnesses as rows).
#
# THE SCAN SET. The handoff named render_passes.hpp and renderer.hpp;
# the tree places pass encoders more widely — renderer.hpp holds
# pipeline creation and NO Begin*Pass site, while the cartridge, four
# bodies files and the patch surface each encode passes of their own,
# the frame submit lives in the_board.cpp, and the reconfigure
# trigger in console.hpp. The census scans where the facts are and
# says so; witness C-4 pins renderer.hpp's zero so the widening stays
# an observation, not an assumption.
#
# USAGE
#   python3 tools/command_census.py              # witnesses to stdout, write artifact
#   python3 tools/command_census.py --check      # witnesses only, write nothing
#   python3 tools/command_census.py -o PATH      # write elsewhere
#
# Exit status is 1 if any witness fails. A failing witness means the
# parser or the program disagrees with a stated fact; either way the
# ledger is not to be trusted until ruled on.
# ═══════════════════════════════════════════════════════════════════════

import argparse
import hashlib
import os
import re
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BOARD = os.path.join(REPO, "src", "cartridges", "the_board")

# Order is the ledger's row order: the two files the handoff named
# first, then the encoders the tree actually carries, then the two
# files that hold the submit tail and the reconfigure trigger.
# NAMED, NOT POSITIONAL (PLUMB_0 A3). Two censuses read INPUTS[-1] meaning
# "console.hpp" and one read INPUTS[1] meaning "renderer.hpp" — so appending a
# tenth input silently re-aimed both at the new file and two witnesses failed
# with a message about the tree rather than about the list. A scanned file
# that some pass needs BY IDENTITY gets a name; the list is for iteration.
RENDERER_HPP = os.path.join(BOARD, "realization", "renderer.hpp")
CONSOLE_HPP = os.path.join(REPO, "src", "console", "console.hpp")

INPUTS = [
    os.path.join(BOARD, "realization", "render_passes.hpp"),
    RENDERER_HPP,
    os.path.join(BOARD, "cartridge.hpp"),
    os.path.join(BOARD, "surface", "patch_system.hpp"),
    os.path.join(BOARD, "bodies", "gol_zones.hpp"),
    os.path.join(BOARD, "bodies", "pawn.hpp"),
    os.path.join(BOARD, "bodies", "gallery.hpp"),
    os.path.join(BOARD, "bodies", "orbs.hpp"),
    os.path.join(REPO, "src", "the_board.cpp"),
    CONSOLE_HPP,
    # PLUMB_0 A3 — A GENERATOR IS AN INPUT TO ITS OWN ARTIFACT. GATES_2c gave
    # the mirror census this rule and left the command census without it, so
    # COMMAND_LEDGER named `tools/command_census.py` in its own first line and
    # pinned everything except that. An edit to the census's rules changed the
    # artifact with nothing to say the artifact was owed.
    os.path.abspath(__file__),
]

DEFAULT_OUT = os.path.join(REPO, "audit", "COMMAND_LEDGER.md")
BINDING_LEDGER_MD = os.path.join(REPO, "audit", "BINDING_LEDGER.md")


class Witnesses:
    """Every authority-bearing check, recorded with its numbers.

    A witness is not an assertion: it is a row. It records what was
    expected, what was found, and whether they agreed, so the artifact
    can carry the evidence rather than a claim about it.
    """

    def __init__(self):
        self.rows = []

    def record(self, tag, ok, detail):
        self.rows.append((tag, bool(ok), detail))
        return ok

    def failures(self):
        return [r for r in self.rows if not r[1]]

    def report(self, stream=sys.stdout):
        for tag, ok, detail in self.rows:
            stream.write("  [%s] %-10s %s\n" % ("PASS" if ok else "FAIL", tag, detail))


def read_raw(path):
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def verify_bytes(path):
    """Read the artifact back as bytes. LF-only, no BOM, one trailing LF."""
    with open(path, "rb") as f:
        d = f.read()
    return {"crlf": d.count(b"\r\n"), "cr": d.count(b"\r"),
            "lf": d.count(b"\n") - d.count(b"\r\n"),
            "bom": d[:3] == b"\xef\xbb\xbf",
            "trailing": d.endswith(b"\n") and not d.endswith(b"\n\n")}


# PLUMB_0 A1/A3 — A CHECK THAT CANNOT FAIL IS A REPORT.
#
# `--check` ran the witnesses and returned 0. The witnesses ask whether the
# TREE is consistent; nothing asked whether the ARTIFACT was taken from this
# tree. So COMMAND_LEDGER shipped a stale digest through every campaign that
# edited a scanned file — the REPEAT_0a refuter found it, REPEAT_0c re-found
# it, and both times the row was green. GATES_2b gave the binding ledger this
# comparison and it was never carried across; this is the carry.
#
# THE STANZA IS THE LIST, as it is there: whatever the artifact pins is
# checked, so a pin added tomorrow is enforced with no edit here.
_PIN_ROW = re.compile(r"^\|\s*`([^`]+)`\s*\|\s*`sha256:([0-9a-f]{64})`\s*\|\s*$",
                      re.M)

_CITE_RE = re.compile(r"(?<![\w/.-])((?:src|tools|web|audit)/[A-Za-z0-9_./+-]+"
                      r"\.(?:hpp|cpp|h|cc|inc|wgsl|py|js|html|md|json))")


def cited_paths(text):
    """Every repo file the emission NAMES — the pin set beyond INPUTS.

    Existence is the filter, not the regex: prose names directories and
    files that died campaigns ago, and a pin on a path that is not there
    would red the gate for a sentence rather than for a fact.
    """
    out = set()
    for m in _CITE_RE.finditer(text):
        rel = m.group(1)
        if os.path.isfile(os.path.join(REPO, rel.replace("/", os.sep))):
            out.add(rel)
    return sorted(out)


def pinned_inputs(ledger_md):
    """Read back every pin the artifact wrote about its own inputs."""
    try:
        with open(ledger_md, "r", encoding="utf-8") as f:
            text = f.read()
    except OSError:
        return None
    return [(pp.replace("\\", "/"), h) for pp, h in _PIN_ROW.findall(text)]


def report_stale(ledger_md):
    """True when the artifact disagrees with the tree. The caller exits red.

    Same shape and same message form as binding_ledger.report_stale — one
    voice across the ledger family — and the same three verdicts: an absent
    artifact pins nothing, a stanza with no pins claims nothing, and a pin
    that disagrees is stale. The cure names the ORDER, because the three
    tools have one.
    """
    rel_led = os.path.relpath(ledger_md, REPO).replace(os.sep, "/")
    cure = ("Regenerate in input order: binding_ledger, command_census, "
            "then mirror_census.")
    pins = pinned_inputs(ledger_md)
    bad = []
    if pins is None:
        bad.append("STALE: %s is absent, so nothing is pinned. %s"
                   % (rel_led, cure))
    elif not pins:
        bad.append("STALE: %s pins nothing; its provenance stanza carries no "
                   "sha256 row. %s" % (rel_led, cure))
    else:
        for rel, want in pins:
            live_path = os.path.join(REPO, rel.replace("/", os.sep))
            if not os.path.isfile(live_path):
                bad.append("STALE: %s pins %s for %s; that file is not on "
                           "disk. %s" % (rel_led, want[:8], rel, cure))
                continue
            live = sha256(live_path)
            if live != want:
                bad.append("STALE: %s pins %s for %s; live is %s. %s"
                           % (rel_led, want[:8], rel, live[:8], cure))
    for line in bad:
        print(line)
    return bool(bad)


def provenance(extra=()):
    """Commit SHA + content hashes of the scanned files.

    NOT `HEAD` (the G2 pattern, binding_ledger.py): re-running on an
    unchanged tree must reproduce this file byte for byte, and HEAD
    moves the moment the artifact is committed. The SHA recorded is
    the last commit that touched any INPUT; the content hashes are the
    authoritative provenance."""
    rel = [os.path.relpath(p, REPO) for p in INPUTS]
    try:
        sha = subprocess.run(["git", "-C", REPO, "log", "-1", "--format=%H", "--"] + rel,
                             capture_output=True, text=True, check=True).stdout.strip()
        subj = subprocess.run(["git", "-C", REPO, "log", "-1", "--format=%s", "--"] + rel,
                              capture_output=True, text=True, check=True).stdout.strip()
    except Exception:
        sha, subj = "(git unavailable)", ""
    pins = [(r.replace(os.sep, "/"), sha256(p)) for r, p in zip(rel, INPUTS)]
    have = {r for r, _ in pins}
    for r in extra:
        if r not in have:
            pins.append((r, sha256(os.path.join(REPO, r.replace("/", os.sep)))))
    return sha, subj, pins


# ═══════════════════════════════════════════════════════════════════════
# THE PASS CENSUS
# ═══════════════════════════════════════════════════════════════════════

BEGIN_RE = re.compile(r"Begin(Render|Compute)Pass\(&(\w+)\)")
SUBMIT_RE = re.compile(r"(\w+(?:->\w+)*(?:\.\w+)*)\.Submit\(")
STOREOP_RE = re.compile(r"wgpu::StoreOp::(\w+)")
FN_RE = re.compile(
    r"^[ \t]*(?:inline\s+)?(?:static\s+)?"
    r"(?:void|bool|int|float|uint32_t|auto|wgpu::\w+)\s+"
    r"(\w+)\s*\(", re.M)


def line_of(text, pos):
    return text.count("\n", 0, pos) + 1


def enclosing_function(text, pos):
    """Last function-definition head before pos. The scanned files
    define pass encoders as free inline functions or plain member
    functions; both match the head pattern."""
    last = None
    for m in FN_RE.finditer(text, 0, pos):
        last = m
    return (last.group(1), line_of(text, last.start())) if last else ("(none)", 0)


def _grab(region, pat, flags=0):
    ms = list(re.finditer(pat, region, flags))
    return ms[-1] if ms else None


def _norm(expr):
    return re.sub(r"\s+", " ", expr.strip())


def _grab_all(region, pat, flags=0):
    """Every distinct assigned value, in source order — a field assigned
    once reads as before; a field re-assigned in a conditional arm
    (DOMESDAY_2 B10's msaa branch) reads as 'base or arm'."""
    seen, out = set(), []
    for m in re.finditer(pat, region, flags):
        v = _norm(m.group(1))
        if v not in seen:
            seen.add(v)
            out.append(v)
    return " or ".join(out) if out else None


def attachment_facts(region, desc):
    """The descriptor's attachments, read from the assignments that
    precede the Begin call inside this site's region."""
    row = {"label": None, "colors": [], "depth": None}
    m = _grab(region, r"%s\.label = \"([^\"]*)\"" % re.escape(desc))
    row["label"] = m.group(1) if m else "(unlabeled)"
    m = _grab(region, r"%s\.colorAttachments = &(\w+)" % re.escape(desc))
    if m:
        cv = m.group(1)
        color = {"var": cv}
        for key, pat in (("view", r"%s\.view = (.*?);"),
                         ("loadOp", r"%s\.loadOp = wgpu::LoadOp::(\w+)"),
                         ("storeOp", r"%s\.storeOp = wgpu::StoreOp::(\w+)"),
                         ("resolveTarget", r"%s\.resolveTarget = (.*?);")):
            v = _grab_all(region, pat % re.escape(cv), re.S)
            color[key] = v if v is not None else "(unset)"
        if color["resolveTarget"] == "(unset)":
            del color["resolveTarget"]
        row["colors"].append(color)
    m = _grab(region, r"%s\.depthStencilAttachment = &(\w+)" % re.escape(desc))
    if m:
        dv = m.group(1)
        depth = {"var": dv}
        for key, pat in (("view", r"%s\.view = (.*?);"),
                         ("depthLoadOp", r"%s\.depthLoadOp = wgpu::LoadOp::(\w+)"),
                         ("depthStoreOp", r"%s\.depthStoreOp = wgpu::StoreOp::(\w+)"),
                         ("stencilLoadOp", r"%s\.stencilLoadOp = wgpu::LoadOp::(\w+)"),
                         ("stencilStoreOp", r"%s\.stencilStoreOp = wgpu::StoreOp::(\w+)"),
                         ("depthReadOnly", r"%s\.depthReadOnly = (\w+)")):
            g = _grab(region, pat % re.escape(dv), re.S)
            depth[key] = _norm(g.group(1)) if g else None
        row["depth"] = depth
    return row


def census_passes(w):
    """One row per Begin*Pass site, region-scoped: a site's region runs
    from max(enclosing-function head, previous site's Begin) to its own
    Begin call, which is where its descriptor assignments live. StoreOp
    token attribution (C-1) uses the same regions."""
    rows = []
    orphan_tokens = []
    total_tokens = 0
    for path in INPUTS:
        text = read_raw(path)
        relp = os.path.relpath(path, REPO)
        sites = list(BEGIN_RE.finditer(text))
        regions = []
        prev_end = 0
        for m in sites:
            fn, fn_line = enclosing_function(text, m.start())
            fn_head = 0
            for fm in FN_RE.finditer(text, 0, m.start()):
                fn_head = fm.start()
            start = max(fn_head, prev_end)
            region = text[start:m.end()]
            prev_end = m.end()
            regions.append((start, m.end()))
            row = {"file": relp, "line": line_of(text, m.start()),
                   "kind": m.group(1).lower(), "desc": m.group(2),
                   "fn": fn, "fn_line": fn_line}
            row.update(attachment_facts(region, m.group(2)))
            rows.append(row)
        for t in STOREOP_RE.finditer(text):
            total_tokens += 1
            hits = [1 for (a, b) in regions if a <= t.start() < b]
            if len(hits) != 1:
                orphan_tokens.append("%s:%d %s (in %d regions)"
                                     % (relp, line_of(text, t.start()),
                                        t.group(0), len(hits)))
    w.record("C-1", not orphan_tokens,
             "every storeOp token attributed to exactly one pass row: "
             "%d tokens over %d rows%s"
             % (total_tokens, len(rows),
                "" if not orphan_tokens else
                " — ORPHANS: " + "; ".join(orphan_tokens)))
    unresolved = [r for r in rows if r["fn"] == "(none)"]
    w.record("C-2", not unresolved,
             "every pass row names its encoding function (%d rows)%s"
             % (len(rows),
                "" if not unresolved else
                " — UNRESOLVED: " + "; ".join(
                    "%s:%d" % (r["file"], r["line"]) for r in unresolved)))
    return rows


ENCODER_RE = re.compile(r"CreateCommandEncoder\(\s*(?:&(\w+))?\s*\)")
BUNDLE_RE = re.compile(r"CreateRenderBundleEncoder\(&(\w+)\)")


def census_bundles(w, pass_rows):
    """BUNDLE_1 R5 — the census learns the bundle.

    A render bundle is a draw list recorded once and executed by a pass.
    Dawn rejects an ExecuteBundles whose bundle disagrees with the pass on
    colour format, depth format or sample count — so the two descriptors
    are a MIRROR, and a mirror with no witness is the kind of pair that
    drifts silently until a device rejects it at boot on someone else's
    machine.

    THE WITNESS COMPARES THE TWO DESCRIPTORS TEXTUALLY, and says so (R-F's
    fallback). It cannot resolve the pass's formats symbolically: a render
    pass names TextureViews, and their formats come from the swapchain and
    from console.hpp's depth buffer, neither of which this census reads. So
    what it holds is narrower and still worth holding — every bundle
    encoder states all three fields, and the fields it states are the ones
    the executing pass's own descriptor was built from."""
    sites = []
    for path in INPUTS:
        text = read_raw(path)
        relp = os.path.relpath(path, REPO)
        for m in BUNDLE_RE.finditer(text):
            fn, _ = enclosing_function(text, m.start())
            fn_head = 0
            for fm in FN_RE.finditer(text, 0, m.start()):
                fn_head = fm.start()
            region = text[fn_head:m.start()]
            var = re.escape(m.group(1))
            def grab(field):
                g = _grab(region, r"%s\.%s\s*=\s*([^;]+);" % (var, field))
                return g.group(1).strip() if g else None
            sites.append({
                "file": relp, "line": line_of(text, m.start()), "fn": fn,
                "label": (lambda g: g.group(1) if g else None)(
                    _grab(region, r'%s\.label\s*=\s*"([^"]*)"' % var)),
                "colorFormatCount": grab("colorFormatCount"),
                "colorFormats": grab("colorFormats"),
                "depthStencilFormat": grab("depthStencilFormat"),
                "sampleCount": grab("sampleCount"),
            })
    # C-8a: every bundle encoder states all three format facts and a label.
    missing = []
    for st in sites:
        for f in ("colorFormatCount", "depthStencilFormat", "sampleCount"):
            if st[f] is None:
                missing.append("%s:%d %s" % (st["file"], st["line"], f))
        if not st["label"]:
            missing.append("%s:%d (label)" % (st["file"], st["line"]))
    w.record("C-8a", not missing,
             "every render-bundle encoder states colorFormatCount, "
             "depthStencilFormat and sampleCount, and carries a label "
             "(%d site(s))%s"
             % (len(sites),
                "" if not missing else " — MISSING: " + "; ".join(missing)))
    # C-8b: an executed bundle's formats are the pass's own — checked
    # textually, per R-F. A depth-only bundle must declare no colour, and
    # a colour bundle must declare exactly one.
    wrong = []
    for st in sites:
        n = (st["colorFormatCount"] or "").strip()
        if n not in ("0", "1"):
            wrong.append("%s:%d colorFormatCount=%s (expected 0 or 1)"
                         % (st["file"], st["line"], n))
        elif n == "0" and (st["colorFormats"] or "nullptr") != "nullptr":
            wrong.append("%s:%d declares 0 colour formats but a non-null list"
                         % (st["file"], st["line"]))
        elif n == "1" and not st["colorFormats"]:
            wrong.append("%s:%d declares 1 colour format and no list"
                         % (st["file"], st["line"]))
    w.record("C-8b", not wrong,
             "every bundle's colour declaration is internally consistent "
             "(%d site(s); the pass-format equality is textual — a pass "
             "names views, whose formats this census cannot resolve)%s"
             % (len(sites), "" if not wrong else " — " + "; ".join(wrong)))
    return sites


def census_encoders(w, pass_rows):
    """DOMESDAY_1 A9 — the label law's instrument half (witness C-7):
    every encoder-creation and pass-begin site carries a label. Labels
    are emitted where objects are created, from the creating function's
    name — never hand-swept again; C-7 stands at every landing."""
    sites = []
    for path in INPUTS:
        text = read_raw(path)
        relp = os.path.relpath(path, REPO)
        for m in ENCODER_RE.finditer(text):
            fn, _ = enclosing_function(text, m.start())
            label = None
            if m.group(1):
                fn_head = 0
                for fm in FN_RE.finditer(text, 0, m.start()):
                    fn_head = fm.start()
                g = _grab(text[fn_head:m.start()],
                          r"%s\.label = \"([^\"]*)\"" % re.escape(m.group(1)))
                label = g.group(1) if g else None
            sites.append({"file": relp, "line": line_of(text, m.start()),
                          "fn": fn, "label": label})
    bad = (["%s:%d (encoder)" % (s["file"], s["line"])
            for s in sites if not s["label"]]
           + ["%s:%d (pass)" % (r["file"], r["line"])
              for r in pass_rows if r["label"] == "(unlabeled)"])
    w.record("C-7", not bad,
             "label law: every encoder-creation site (%d) and pass-begin "
             "site (%d) carries a label%s"
             % (len(sites), len(pass_rows),
                "" if not bad else " — UNLABELED: " + "; ".join(bad)))
    return sites


def census_submits():
    subs = []
    for path in INPUTS:
        text = read_raw(path)
        relp = os.path.relpath(path, REPO)
        for m in SUBMIT_RE.finditer(text):
            fn, _ = enclosing_function(text, m.start())
            subs.append({"file": relp, "line": line_of(text, m.start()),
                         "recv": m.group(1), "fn": fn})
    return subs


def census_reconfigure(w):
    """The swapchain reconfigure sites, and THE trigger: the resize-path
    condition in Console::begin_frame whose branch reconfigures the
    surface and fires frame1_report — the code that feeds the [FRAME_1]
    print. Quoted verbatim."""
    path = CONSOLE_HPP
    text = read_raw(path)
    relp = os.path.relpath(path, REPO)
    sites = []
    trigger = None
    for m in re.finditer(r"surface_\.Configure\(&surfaceConfig_\);", text):
        fn, _ = enclosing_function(text, m.start())
        sites.append({"file": relp, "line": line_of(text, m.start()), "fn": fn})
        if fn != "begin_frame":
            continue
        # The nearest `if (` above the Configure call opens the branch.
        if_pos = text.rfind("if (", 0, m.start())
        # Take the branch verbatim: from the `if` line through the
        # closing brace at the same indentation as the `if`.
        if_line_start = text.rfind("\n", 0, if_pos) + 1
        indent = text[if_line_start:if_pos]
        close = text.find("\n" + indent + "}", m.start())
        end = text.find("\n", close + 1) if close != -1 else m.end()
        quote = text[if_line_start:end]
        branch_has_frame1 = "frame1_report(" in quote
        trigger = {"file": relp, "line": line_of(text, if_pos),
                   "quote": quote, "anchored": branch_has_frame1}
    w.record("C-3", trigger is not None and trigger["anchored"],
             "the begin_frame reconfigure branch is captured verbatim and "
             "contains the frame1_report call that feeds [FRAME_1]"
             if trigger else
             "NO begin_frame reconfigure site found")
    return sites, trigger


def depth_bindings_from_binding_ledger():
    """Table A's depth-texture rows, read from the committed ledger —
    the cross-cite the D-DISCARD finding names. A Table A row carries
    the declaration symbol in the cell just before its store type."""
    led = read_raw(BINDING_LEDGER_MD)
    a = led.find("## Table A")
    b = led.find("## Table B")
    seen, out = set(), []
    for m in re.finditer(r"^\|.*`(\w+)` \| `texture_depth_2d`.*$",
                         led[a:b], re.M):
        if m.group(1) not in seen:
            seen.add(m.group(1))
            out.append((m.group(1), m.group(0)))
    return out


def finding_d_discard(w, rows):
    """The stated finding: for the MAIN SCENE PASS depth attachment,
    (a) does any later pass in the frame use loadOp Load on the same
    depth texture, and (b) is that texture bound anywhere in
    BINDING_LEDGER Table A."""
    main = [r for r in rows if r["label"] == "Rasterized Scene"]
    if not w.record("C-5", len(main) == 1,
                    "exactly one main scene pass row (label 'Rasterized "
                    "Scene'): found %d" % len(main)):
        return None
    main = main[0]
    # (a) — two facts, both censused: no depth attachment anywhere in
    # the program opens with LoadOp::Load, and no other pass's depth
    # view names the console depth texture (the main pass receives it
    # as the `depth` parameter; every other depth view is a shadow or
    # offscreen view of gpuState_).
    loads = [r for r in rows
             if r["depth"] and r["depth"].get("depthLoadOp") == "Load"]
    other_views = sorted({r["depth"]["view"] for r in rows
                          if r["depth"] and r is not main})
    same_texture = [v for v in other_views if v == main["depth"]["view"]]
    a_no = not loads and not same_texture
    # (b) — Table A's only depth bindings; and the console depth
    # texture's usage mask carries RenderAttachment ALONE, so it cannot
    # enter a bind group at all.
    depth_rows = depth_bindings_from_binding_ledger()
    syms = sorted(s for s, _ in depth_rows)
    console_text = read_raw(CONSOLE_HPP)
    m = re.search(r"depthDesc\.usage = ([^;]+);", console_text)
    usage = _norm(m.group(1)) if m else "(not found)"
    b_no = (syms == ["shadow_map", "spot_shadow_map"]
            and usage == "wgpu::TextureUsage::RenderAttachment")
    w.record("C-6", a_no and b_no,
             "(a) no depth LoadOp::Load anywhere (%d), no other pass names "
             "the main depth view (%s); (b) Table A depth bindings are %s "
             "and the console depth usage is %s"
             % (len(loads), "none" if not same_texture else same_texture,
                "/".join(syms) or "(none)", usage))
    return {"main": main, "a_no": a_no, "b_no": b_no,
            "loads": loads, "other_views": other_views,
            "depth_rows": depth_rows, "usage": usage}


# ═══════════════════════════════════════════════════════════════════════
# EMISSION
# ═══════════════════════════════════════════════════════════════════════

def emit(w, rows, subs, reconf_sites, trigger, dd, encoders, bundles,
         extra_pins=()):
    A = []
    A.append("# COMMAND_LEDGER — the pass census (DOMESDAY_0 A4)")
    A.append("")
    A.append("Generated by `tools/command_census.py` — do not hand-edit.")
    A.append("Read-only: a census of the program's pass and submit surface.")
    A.append("")
    sha, subj, hashes = provenance(extra_pins)
    A.append("## Provenance")
    A.append("")
    A.append("Last commit touching any scanned file: `%s`" % sha)
    if subj:
        A.append("(%s)" % subj)
    A.append("")
    A.append("| file scanned | sha256 |")
    A.append("|---|---|")
    for r, h in hashes:
        A.append("| `%s` | `sha256:%s` |" % (r, h))
    A.append("")
    A.append("The handoff named `render_passes.hpp` and `renderer.hpp`; the")
    A.append("tree places pass encoders more widely, so the census scans the")
    A.append("files above. `renderer.hpp` carries pipeline creation and ZERO")
    A.append("`Begin*Pass` sites (witness C-4 pins that zero); the cartridge,")
    A.append("bodies, and patch surface encode passes of their own; the frame")
    A.append("submit lives in `the_board.cpp`; the reconfigure trigger")
    A.append("in `console.hpp`.")
    A.append("")
    A.append("## §1 — the passes, one row each")
    A.append("")
    A.append("| # | label | kind | encoded by | site | color (load/store) | depth (load/store, readOnly) | stencil |")
    A.append("|---|---|---|---|---|---|---|---|")
    for i, r in enumerate(rows):
        if r["kind"] == "render":
            colors = "; ".join(
                "%s/%s → `%s`%s" % (c["loadOp"], c["storeOp"], c["view"],
                                    (" resolve → `%s`" % c["resolveTarget"])
                                    if c.get("resolveTarget") else "")
                for c in r["colors"]) or "(none: depth-only)"
            d = r["depth"]
            if d:
                ro = d["depthReadOnly"] if d["depthReadOnly"] is not None else "(absent)"
                depth = "%s/%s, readOnly %s → `%s`" % (
                    d["depthLoadOp"], d["depthStoreOp"], ro, d["view"])
                stencil = ("%s/%s" % (d["stencilLoadOp"], d["stencilStoreOp"])
                           if d["stencilLoadOp"] or d["stencilStoreOp"]
                           else "(no stencil aspect)")
            else:
                depth, stencil = "(none)", "(none)"
        else:
            colors, depth, stencil = "—", "—", "—"
        A.append("| %d | %s | %s | `%s` | `%s:%d` | %s | %s | %s |"
                 % (i + 1, r["label"], r["kind"], r["fn"],
                    r["file"], r["line"], colors, depth, stencil))
    n_render = sum(1 for r in rows if r["kind"] == "render")
    A.append("")
    A.append("%d passes: %d render, %d compute."
             % (len(rows), n_render, len(rows) - n_render))
    A.append("")
    A.append("## §2 — submit sites")
    A.append("")
    A.append("| # | receiver | enclosing function | site |")
    A.append("|---|---|---|---|")
    for i, s in enumerate(subs):
        A.append("| %d | `%s.Submit` | `%s` | `%s:%d` |"
                 % (i + 1, s["recv"], s["fn"], s["file"], s["line"]))
    A.append("")
    A.append("%d submit sites. The frame's one submit rides the pawn's"
             % len(subs))
    A.append("render tick; the GoL derive flush issues its own (the cartridge")
    A.append("phase table marks it `F_SUBMIT`, cartridge.hpp).")
    A.append("")
    A.append("### Render bundles (BUNDLE_1)")
    A.append("")
    A.append("A bundle is a draw list recorded ONCE and executed by a pass with")
    A.append("one call. Its descriptor states the formats of the pass that will")
    A.append("execute it — Dawn rejects a mismatch — so the two are a mirror,")
    A.append("held by C-8a/C-8b. The equality is TEXTUAL: a render pass names")
    A.append("TextureViews and their formats come from the swapchain and from")
    A.append("console.hpp's depth buffer, neither of which this census reads.")
    A.append("")
    A.append("| # | label | colour | depth | samples | recorded in | site |")
    A.append("|---|---|---|---|---|---|---|")
    for i, b in enumerate(bundles):
        A.append("| %d | %s | %s x `%s` | `%s` | `%s` | `%s` | `%s:%d` |"
                 % (i + 1,
                    ("`\"%s\"`" % b["label"]) if b["label"] else "**(unlabeled)**",
                    b["colorFormatCount"] or "?", b["colorFormats"] or "-",
                    b["depthStencilFormat"] or "?", b["sampleCount"] or "?",
                    b["fn"], b["file"], b["line"]))
    A.append("")
    A.append("### Encoder-creation sites (the label law, DOMESDAY_1 A9)")
    A.append("")
    A.append("Labels are emitted where objects are created, from the creating")
    A.append("function's name — never hand-swept again; witness C-7 stands at")
    A.append("every landing.")
    A.append("")
    A.append("| # | label | enclosing function | site |")
    A.append("|---|---|---|---|")
    for i, s in enumerate(encoders):
        A.append("| %d | %s | `%s` | `%s:%d` |"
                 % (i + 1,
                    ("`\"%s\"`" % s["label"]) if s["label"] else "**(unlabeled)**",
                    s["fn"], s["file"], s["line"]))
    A.append("")
    A.append("## §3 — the swapchain reconfigure trigger")
    A.append("")
    A.append("| # | enclosing function | site |")
    A.append("|---|---|---|")
    for i, s in enumerate(reconf_sites):
        A.append("| %d | `%s` | `%s:%d` |" % (i + 1, s["fn"], s["file"], s["line"]))
    A.append("")
    A.append("The boot-time site configures the surface once; the per-frame")
    A.append("trigger is the resize branch of `Console::begin_frame`, quoted")
    A.append("verbatim (`%s:%d`) — its branch is what feeds the `[FRAME_1]`"
             % (trigger["file"], trigger["line"]))
    A.append("print. This is the debounce ruling's evidence, and it reads the")
    A.append("other way now: the size test is a bare not-equal on the capped")
    A.append("framebuffer size, but it only ARMS the reconfigure — the branch")
    A.append("below counts `stableFrames_` and acts once the size has held")
    A.append("still for `RECONFIGURE_SETTLE_FRAMES` consecutive frames, so a")
    A.append("size flutter costs a counter rather than a surface reconfigure.")
    A.append("(The old sentence also said the branch recreates the depth")
    A.append("buffer. It does not, and had not since ACQ_0 moved the depth")
    A.append("buffer to the first acquire: the branch writes the canvas")
    A.append("backing store and calls Configure, nothing more. A5 corrected")
    A.append("the timing half and carried the dead half forward; this is")
    A.append("both.) The settling window is quoted")
    A.append("directly below; this paragraph said there was none for as long")
    A.append("as the quote beneath it said otherwise (PLUMB_0 A5).")
    A.append("")
    A.append("```cpp")
    A.append(trigger["quote"])
    A.append("```")
    A.append("")
    A.append("## §4 — the finding the handoff asked this census to compute")
    A.append("")
    main = dd["main"]
    d = main["depth"]
    A.append("For the main scene pass (`%s`, \"%s\") depth attachment:"
             % (main["fn"], main["label"]))
    A.append("")
    A.append("- **(a)** does any later pass in the frame use `loadOp:\"load\"`")
    A.append("  on the same depth texture? **No.** No depth attachment in the")
    A.append("  program opens with `LoadOp::Load` (%d found), and no other"
             % len(dd["loads"]))
    A.append("  pass's depth view names the main depth texture — the other")
    A.append("  depth views are: %s."
             % ", ".join("`%s`" % v for v in dd["other_views"]))
    A.append("- **(b)** is that texture bound anywhere in BINDING_LEDGER")
    A.append("  Table A? **No.** Table A's only depth bindings are:")
    for sym, row in dd["depth_rows"]:
        A.append("  - `%s`" % sym)
    A.append("")
    A.append("  and the main depth's backing (`console.hpp`")
    A.append("  `createDepthBuffer`) is created with usage `%s` alone —"
             % dd["usage"])
    A.append("  no `TextureBinding`, so it cannot enter a bind group at all.")
    A.append("")
    A.append("FINDING D-DISCARD: main depth is single-pass, never sampled — "
             "storeOp discard is mechanical.")
    A.append("")
    A.append("STATUS: **already satisfied in-tree.** The main pass depth")
    A.append("storeOp reads `wgpu::StoreOp::%s` today (DISCARD_0 / PASS_0 F1,"
             % d["depthStoreOp"])
    A.append("`render_passes.hpp` `render_main_pass`), and the snapshot twin")
    A.append("carries the same op (PASS_0 F2, `gallery.hpp`")
    A.append("`render_snapshot_pass`). Unit B4 has no token left to edit.")
    A.append("")
    A.append("## §5 — witnesses")
    A.append("")
    A.append("| witness | verdict | detail |")
    A.append("|---|---|---|")
    for tag, ok, detail in w.rows:
        A.append("| `%s` | **%s** | %s |" % (tag, "PASS" if ok else "FAIL", detail))
    return "\n".join(A) + "\n"


def main():
    ap = argparse.ArgumentParser(description="DOMESDAY_0 A4 — the command ledger census.")
    ap.add_argument("--check", action="store_true",
                    help="run the witnesses, write nothing")
    ap.add_argument("-o", "--out", default=DEFAULT_OUT)
    args = ap.parse_args()

    w = Witnesses()
    rows = census_passes(w)
    subs = census_submits()
    reconf_sites, trigger = census_reconfigure(w)
    renderer_hits = len(BEGIN_RE.findall(read_raw(RENDERER_HPP)))
    w.record("C-4", renderer_hits == 0,
             "renderer.hpp encodes no pass of its own (Begin*Pass sites: %d)"
             % renderer_hits)
    dd = finding_d_discard(w, rows)
    encoders = census_encoders(w, rows)
    bundles = census_bundles(w, rows)

    print("DOMESDAY_0 A4 — the command ledger")
    print("")
    print("  %d pass rows (%d render, %d compute), %d submit sites, "
          "%d reconfigure sites, %d render bundle(s)"
          % (len(rows), sum(1 for r in rows if r["kind"] == "render"),
             sum(1 for r in rows if r["kind"] == "compute"),
             len(subs), len(reconf_sites), len(bundles)))
    print("")
    w.report()
    print("")
    if w.failures():
        print("STOP — %d witness(es) failed; the ledger is not to be trusted "
              "until ruled on." % len(w.failures()))
        return 1
    if args.check:
        # PLUMB_0 A1 — the witnesses passed on the TREE; now ask whether the
        # ARTIFACT was taken from it. This is the comparison that was missing.
        print("")
        if report_stale(args.out):
            return 1
        print("--check: all witnesses pass, the ledger's pins match the live "
              "inputs, nothing written.")
        return 0

    if not args.check:
        # PLUMB_0 A3 — two passes, one fixed point: pass one is read for the
        # paths this census names, pass two pins them. The rows pass two adds
        # name paths already in the set, so a third pass would change nothing.
        draft = emit(w, rows, subs, reconf_sites, trigger, dd, encoders, bundles)
        text = emit(w, rows, subs, reconf_sites, trigger, dd, encoders, bundles,
                    extra_pins=cited_paths(draft))
        with open(args.out, "w", encoding="utf-8", newline="\n") as f:
            f.write(text)
        bx = verify_bytes(args.out)
        if bx["crlf"] or bx["cr"] or bx["bom"] or not bx["trailing"]:
            print("STOP — encoding read-back failed: %r" % bx)
            return 1
        print("wrote %s" % os.path.relpath(args.out, REPO))
    return 0


if __name__ == "__main__":
    sys.exit(main())
