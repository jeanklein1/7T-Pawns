#!/usr/bin/env python3
# ═══════════════════════════════════════════════════════════════════════
# THE MIRROR CENSUS (LOOM_0) — what one table must contain to regenerate
# the four hand-maintained mirrors of the binding surface byte-identically.
#
# WHAT THIS IS. The binding surface lives in four mirrors kept in lockstep
# by boot-time validation, not by the compiler: binding_registry.hpp,
# world.wgsl's 98 declarations, state.hpp's layout + bind group blocks,
# and renderer.hpp's pipeline-layout lists. LOOM_1 will generate all four
# from one table. This census proves, per mirror, that five relations
# (SLOTS, SEATS, LAYOUTS, GROUPS, PIPELINES) reconstruct it — and lists
# every fact that does not fit. The residue list is the crown of the
# artifact: it is the exact bill of what the generator's table must carry
# BEYOND the five relations, or must leave in place and patch around.
#
# WHAT THIS IS NOT. It is not the generator, and it reconciles no struct
# layouts (M5 counts twins; the panel reconciles them). It touches no
# source file: read-only inputs, one markdown artifact out.
#
# INSTRUMENT REUSE. The parsers are binding_ledger.py's, imported — not
# copied. One parse, two artifacts: a census that re-derived the ledger's
# rows with a second parser would let the two instruments drift apart and
# call the drift a finding. binding_ledger.py is therefore an INPUT, and
# is hashed in the provenance block like the sources it reads.
#
# THE RECONCILIATION GATE (ML-0). Every count this census produces is
# checked against the committed audit/BINDING_LEDGER.md — the values are
# READ from the ledger text, never hardcoded here. A mismatch means the
# ledger is stale or a parser changed underneath us; either way the
# census is not to be trusted and nothing is written.
#
# USAGE
#   python3 tools/mirror_census.py              # witnesses to stdout, write artifact
#   python3 tools/mirror_census.py --check      # witnesses only, write nothing
#   python3 tools/mirror_census.py -o PATH      # write elsewhere
#
# Exit status is 1 if any witness fails. A failing witness is a STOP: the
# site is quoted verbatim in the witness detail and nothing is written.
# ═══════════════════════════════════════════════════════════════════════

import argparse
import importlib.util
import os
import re
import subprocess
import sys

_HERE = os.path.dirname(os.path.abspath(__file__))
_spec = importlib.util.spec_from_file_location(
    "binding_ledger", os.path.join(_HERE, "binding_ledger.py"))
BL = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(BL)

REPO = BL.REPO
REAL = BL.REAL
LEDGER_MD = os.path.join(REPO, "audit", "BINDING_LEDGER.md")
DEFAULT_OUT = os.path.join(REPO, "audit", "MIRROR_LEDGER.md")

# Input order is the handoff's: the four mirrors, then the instrument
# this one leans on, then the ledger it reconciles against.
INPUTS = [BL.REGISTRY_HPP, BL.WORLD_WGSL, BL.STATE_HPP, BL.GEN_INC,
          BL.RENDERER_HPP,
          os.path.join(_HERE, "binding_ledger.py"), LEDGER_MD]

CONTRACTS_DIR = os.path.join(REPO, "src", "cartridges", "the_board", "contracts")

MIRROR_NAMES = {
    "R": "binding_registry.hpp (registry constants)",
    "W": "world.wgsl (binding declarations)",
    "L": "state.hpp (layout entry arrays + descriptors)",
    "G": "state.hpp (bind group creation sites)",
    "P": "renderer.hpp (pipeline-layout lists)",
}


def norm(s):
    """One-space normal form for statement classification and quoting."""
    return " ".join(s.split())


def tight_span(stripped, start, end):
    """Trim a [start, end) span to its first/last non-space byte in the
    comment-stripped text, so the raw slice quotes the statement without
    the comment lines that precede it (comments are blanked to spaces by
    the stripper, offsets preserved)."""
    i, j = start, end
    while i < j and stripped[i] in " \t\r\n":
        i += 1
    while j > i and stripped[j - 1] in " \t\r\n":
        j -= 1
    return i, j


# ═══════════════════════════════════════════════════════════════════════
# THE LEDGER'S OWN NUMBERS (ML-0's authority)
# ═══════════════════════════════════════════════════════════════════════

def parse_ledger(w):
    """Read the expected counts and the Table H membership FROM the
    committed ledger. An unreadable authority is the same defect as a
    wrong one, so a failed match is recorded as an ML-0 failure, not a
    skip (the 0b-1 lesson, one instrument up)."""
    led = BL.read(LEDGER_MD)
    out = {"text": led}
    probes = [
        ("decls_slots",
         r"(\d+) module-scope declarations over (\d+) `\(group, binding\)` slots"),
        ("seats_layouts",
         r"One row per `\(bind group layout, entry index\)`, (\d+) rows over (\d+) layouts"),
        ("groups",
         r"(\d+) bind groups over (\d+) layouts, every one a bijection"),
        ("pipelines",
         r"(\d+) pipelines: \d+ with exactly one call shape"),
    ]
    missing = []
    for key, pat in probes:
        m = re.search(pat, led)
        out[key] = tuple(int(g) for g in m.groups()) if m else None
        if m is None:
            missing.append(key)
    # Table H rows, keyed by kind. The census needs the `wgsl binding`
    # membership for M2's defended column.
    out["tableH_wgsl"] = set(re.findall(r"^\| `(\w+)` \| wgsl binding \|", led, re.M))
    w.record("ML-0r", not missing,
             "the ledger states all four count sentences the gate reads "
             "(declarations/slots, rows/layouts, bind groups, pipelines); "
             "Table H carries %d `wgsl binding` rows" % len(out["tableH_wgsl"])
             if not missing else
             "STOP — the ledger does not state: %s; the gate cannot read its "
             "authority" % ", ".join(missing))
    return out


def ml0(w, ledger, layouts, wgsl, cen, groups):
    """Counts reconcile with the fresh ledger. Expected values come FROM
    the ledger (parse_ledger); the census side is what the imported
    parsers found on the tree."""
    got = {
        "declarations": len(wgsl["decls"]),
        "slots": len(wgsl["slots"]),
        "seats": sum(len(L["entries"]) for L in layouts),
        "layouts": len(layouts),
        "groups": len(groups),
        "pipelines": len(cen["pipelines"]),
    }
    want = {}
    if ledger["decls_slots"]:
        want["declarations"], want["slots"] = ledger["decls_slots"]
    if ledger["seats_layouts"]:
        want["seats"], want["layouts"] = ledger["seats_layouts"]
    if ledger["groups"]:
        want["groups"] = ledger["groups"][0]
    if ledger["pipelines"]:
        want["pipelines"] = ledger["pipelines"][0]
    bad = ["%s: ledger says %s, census finds %s" % (k, want[k], got[k])
           for k in sorted(want) if want[k] != got[k]]
    w.record("ML-0", not bad and len(want) == 6,
             "counts reconcile with audit/BINDING_LEDGER.md, read from the "
             "ledger text: " + ", ".join("%s %d" % (k, got[k]) for k in
                                         ("slots", "declarations", "seats",
                                          "layouts", "groups", "pipelines"))
             if not bad and len(want) == 6 else
             "STOP — " + "; ".join(bad or ["ledger counts unreadable"]))
    return got


# ═══════════════════════════════════════════════════════════════════════
# THE BIND GROUP PARSE (M6's data)
#
# binding_ledger.py's 0a-6 proves the bijection but keeps only the
# binding SET per group. The backing map needs the entries: which member
# expression each seat is fed. Same block delimiters as 0a-6.
# ═══════════════════════════════════════════════════════════════════════

def parse_groups(w):
    raw = BL.read(BL.STATE_HPP)
    src = BL.strip_cpp_comments(raw)
    groups = []
    problems = []
    for m in re.finditer(
            r"std::array<\s*wgpu::BindGroupEntry\s*,\s*(\d+)\s*>\s*(\w+)\s*\{\s*\}\s*;", src):
        declared_n = int(m.group(1))
        arr = re.escape(m.group(2))
        tail = src.find("CreateBindGroup(", m.end())
        if tail < 0:
            raise SystemExit("mirror_census: unterminated bind group block at line %d"
                             % BL.line_of(src, m.start()))
        end = src.find(";", tail)
        block = src[m.end():end + 1]
        base = m.end()

        dv = re.search(r"wgpu::BindGroupDescriptor\s+(\w+)\s*\{\s*\}\s*;", block)
        d = re.escape(dv.group(1)) if dv else r"desc"
        lm = re.search(d + r'\.label\s*=\s*"([^"]*)"\s*;', block)
        lp = re.search(d + r"\.label\s*=\s*(\w+)\s*;", block)
        lo = re.search(d + r"\.layout\s*=\s*(\w+)\s*;", block)
        cm = re.search(r"(\w+)\s*=\s*device_\.CreateBindGroup\(", block)

        rows = {}

        def slot(i):
            return rows.setdefault(i, {})

        for e in re.finditer(arr + r"\[(\d+)\]\.binding\s*=\s*(bind::g\d::\w+)\s*;", block):
            slot(int(e.group(1)))["binding"] = e.group(2)
        for field, kind in (("buffer", "buffer"), ("textureView", "textureView"),
                            ("sampler", "sampler")):
            for e in re.finditer(arr + r"\[(\d+)\]\." + field + r"\s*=\s*([^;]+);", block):
                r = slot(int(e.group(1)))
                r["kind"], r["backing"] = kind, norm(e.group(2))
                r["nback"] = r.get("nback", 0) + 1
        for e in re.finditer(arr + r"\[(\d+)\]\.offset\s*=\s*([^;]+);", block):
            slot(int(e.group(1)))["offset"] = norm(e.group(2))
        for e in re.finditer(arr + r"\[(\d+)\]\.size\s*=\s*([^;]+);", block):
            slot(int(e.group(1)))["size"] = norm(e.group(2))

        for i, r in rows.items():
            if "binding" not in r or "kind" not in r:
                problems.append("entries[%d] at state.hpp:%d lacks %s" %
                                (i, BL.line_of(src, base),
                                 "a .binding" if "binding" not in r else "a backing field"))
            elif r.get("nback", 0) != 1:
                # Two backing assignments on one seat would silently
                # print whichever the loop order preferred — a stale
                # line left behind by a kind change is exactly the
                # defect a backing map exists to catch.
                problems.append("entries[%d] at state.hpp:%d carries %d backing "
                                "assignments" % (i, BL.line_of(src, base),
                                                 r.get("nback", 0)))

        groups.append({
            "label": lm.group(1) if lm else
                     ("(parameter: %s)" % lp.group(1)) if lp else "(unlabeled)",
            "param_label": lm is None,
            "layout_member": lo.group(1) if lo else None,
            "member": cm.group(1) if cm and cm.group(1) != "return" else None,
            "declared_n": declared_n,
            "entries": [dict(rows[i], index=i) for i in sorted(rows)],
            "line": BL.line_of(src, m.start()),
            "desc_var": dv.group(1) if dv else "desc",
        })

    # The builder's three invocations: the one block that mints three
    # groups, its label and index-buffer window arriving as arguments.
    invokes = []
    for m in re.finditer(
            r"(\w+)\s*=\s*build_render_entity_group\(\s*\"([^\"]*)\"\s*,\s*(\w+)\s*,\s*(\w+)\s*\)\s*;",
            src):
        invokes.append({"member": m.group(1), "label": m.group(2),
                        "offset_arg": m.group(3), "size_arg": m.group(4),
                        "line": BL.line_of(src, m.start())})

    incomplete = [g for g in groups
                  if len(g["entries"]) != g["declared_n"]]
    for g in incomplete:
        problems.append("%s (line %d): %d entries parsed against declared %d"
                        % (g["label"], g["line"], len(g["entries"]), g["declared_n"]))
    w.record("M6-0", not problems,
             "%d bind group creation blocks parsed to full entry rows "
             "(every entry has a binding and exactly one backing field); "
             "%d builder invocation(s)" % (len(groups), len(invokes))
             if not problems else "STOP — " + "; ".join(problems))
    return groups, invokes


# ═══════════════════════════════════════════════════════════════════════
# M1 — THE IDIOM CENSUS
#
# The law: every instance inside a surface's boundary matches EXACTLY ONE
# idiom. An unmatched or doubly-matched site is a STOP, quoted verbatim.
# Idioms are code SHAPES; their variable parts are the relations' data,
# their fixed parts are the generator's template, and everything a shape
# cannot say (alignment, prose, order) is residue.
# ═══════════════════════════════════════════════════════════════════════

class Surface:
    def __init__(self, key, idioms):
        self.key = key
        self.idioms = [(iid, desc, re.compile(pat)) for iid, desc, pat in idioms]
        self.counts = {iid: 0 for iid, _, _ in idioms}
        self.exemplar = {}
        self.exemplar_line = {}
        self.stops = []
        self.total = 0

    def feed(self, text, line):
        """Classify one normalized instance; record the STOP on 0 or 2+."""
        self.total += 1
        hits = [iid for iid, _, rx in self.idioms if rx.fullmatch(text)]
        if len(hits) != 1:
            self.stops.append("%s (line %d, matched %d idioms): %s"
                              % (self.key, line, len(hits), text))
            return None
        iid = hits[0]
        self.counts[iid] += 1
        if iid not in self.exemplar:
            self.exemplar[iid] = text
            self.exemplar_line[iid] = line
        return iid

    def rows(self):
        return [(iid, desc, self.counts[iid],
                 self.exemplar.get(iid, "(no instance)"),
                 self.exemplar_line.get(iid))
                for iid, desc, _ in self.idioms]


def census_registry(w):
    """Surface R — one instance per `inline constexpr uint32_t` line."""
    raw = BL.read(BL.REGISTRY_HPP)
    s = Surface("R", [
        ("R-const", "constant, no trailing comment",
         r"inline constexpr uint32_t \w+ = \d+;"),
        ("R-const-c", "constant with trailing // comment",
         r"inline constexpr uint32_t \w+ = \d+; //.*"),
    ])
    ns = None
    per_ns = {}
    ns_order = []
    for i, line in enumerate(raw.splitlines(), 1):
        nm = re.search(r"namespace\s+(g\d)\s*\{", line)
        if nm:
            ns = nm.group(1)
            if ns not in per_ns:
                per_ns[ns] = []
                ns_order.append(ns)
        if "inline constexpr uint32_t" in line:
            s.feed(norm(line), i)
            cm = re.search(r"uint32_t\s+(\w+)\s*=\s*(\d+)\s*;", line)
            if cm and ns:
                per_ns[ns].append((cm.group(1), int(cm.group(2)), i))
    # Ordering: strictly ascending binding number inside each namespace,
    # namespaces in declaration order g0, g1, g2.
    counterexamples = []
    for n, rows_ in per_ns.items():
        for (a, va, la), (b_, vb, lb) in zip(rows_, rows_[1:]):
            if vb <= va:
                counterexamples.append("%s::%s = %d (line %d) follows %s::%s = %d"
                                       % (n, b_, vb, lb, n, a, va))
    asserts = len(re.findall(r"static_assert\s*\(", raw))
    ordering = ("strictly ascending binding number within each namespace "
                "(%s), no counterexample" % ", ".join(ns_order)
                if not counterexamples else
                "ascending-within-namespace VIOLATED: " + "; ".join(counterexamples))
    return s, {"per_ns": per_ns, "ns_order": ns_order, "ordering": ordering,
               "static_asserts": asserts, "counterexamples": counterexamples}


def census_wgsl(w, wgsl):
    """Surface W — one instance per binding declaration, on the raw line."""
    raw = BL.read(BL.WORLD_WGSL)
    rawlines = raw.split("\n")
    # A trailing // comment is attached prose (M4 residue), not a shape
    # of its own; the idioms admit it and the count is reported below.
    tail = r"[^;/]+;( //.*)?"
    s = Surface("W", [
        ("W-u", "`var<uniform>` declaration",
         r"@group\(\d+\) @binding\(\d+\) var<uniform> \w+ ?: " + tail),
        ("W-sr", "`var<storage, read>` declaration",
         r"@group\(\d+\) @binding\(\d+\) var<storage, read> \w+ ?: " + tail),
        ("W-srw", "`var<storage, read_write>` declaration",
         r"@group\(\d+\) @binding\(\d+\) var<storage, read_write> \w+ ?: " + tail),
        ("W-s0", "bare `var<storage>` (access defaulted)",
         r"@group\(\d+\) @binding\(\d+\) var<storage> \w+ ?: " + tail),
        ("W-h", "handle declaration (no address space clause)",
         r"@group\(\d+\) @binding\(\d+\) var \w+ ?: " + tail),
    ])
    padded = 0
    spaced_colon = 0
    trailing = 0
    order_break = None
    prev = None
    for d in sorted(wgsl["decls"], key=lambda d: d.line):
        line = rawlines[d.line - 1]
        s.feed(norm(line), d.line)
        if re.search(r"@binding\(\d+\)\s{2,}var", line):
            padded += 1
        if re.search(r"\b%s\s+:" % re.escape(d.symbol), line):
            spaced_colon += 1
        if re.search(r";\s*//", line):
            trailing += 1
        if prev is not None and (d.group, d.binding) < (prev.group, prev.binding) \
                and order_break is None:
            order_break = ("%s @(%d,%d) at line %d follows %s @(%d,%d) at line %d"
                           % (d.symbol, d.group, d.binding, d.line,
                              prev.symbol, prev.group, prev.binding, prev.line))
        prev = d
    ordering = ("file order is NOT (group, binding) order — first inversion: %s; "
                "the scatter is the fact M2 maps" % order_break
                if order_break else
                "file order equals (group, binding) order")
    return s, {"padded": padded, "spaced_colon": spaced_colon,
               "trailing": trailing, "ordering": ordering,
               "order_break": order_break}


def _block_statements(src, start, end):
    """(tight_start, tight_end) spans of the ';'-terminated statements in
    src[start:end], computed on the comment-stripped text."""
    spans = []
    i = start
    while i < end:
        j = src.find(";", i)
        if j < 0 or j >= end:
            break
        a, b_ = tight_span(src, i, j + 1)
        if a < b_:
            spans.append((a, b_))
        i = j + 1
    return spans


def _extend_through_check(src, end):
    """If the text after `end` is `if (!x) return false;`, include it."""
    m = re.match(r"\s*if\s*\(\s*!\w+\s*\)\s*return\s+false\s*;", src[end:])
    return end + m.end() if m else end


LAYOUT_IDIOMS = [
    ("L-arr", "the entries array declaration",
     r"std::array<wgpu::BindGroupLayoutEntry, \d+> entries\{\};"),
    ("L-bind", "seat -> slot: registry constant, never a literal",
     r"entries\[\d+\]\.binding = bind::g\d::\w+;"),
    ("L-vis", "visibility mask (Vertex/Fragment/Compute joined by |)",
     r"entries\[\d+\]\.visibility = wgpu::ShaderStage::\w+( \| wgpu::ShaderStage::\w+)*;"),
    ("L-buf", "buffer binding type",
     r"entries\[\d+\]\.buffer\.type = wgpu::BufferBindingType::\w+;"),
    ("L-dyn", "dynamic-offset flag",
     r"entries\[\d+\]\.buffer\.hasDynamicOffset = (?:true|false);"),
    ("L-min", "minBindingSize — a named constant or the size of a named "
              "struct (COMPAT_1's two dynamic-offset seats write sizeof)",
     r"entries\[\d+\]\.buffer\.minBindingSize = (?:\w+|sizeof\(\w+\));"),
    ("L-smp", "sampler binding type",
     r"entries\[\d+\]\.sampler\.type = wgpu::SamplerBindingType::\w+;"),
    ("L-txs", "texture sample type",
     r"entries\[\d+\]\.texture\.sampleType = wgpu::TextureSampleType::\w+;"),
    ("L-txd", "texture view dimension",
     r"entries\[\d+\]\.texture\.viewDimension = wgpu::TextureViewDimension::\w+;"),
    ("L-sta", "storage texture access",
     r"entries\[\d+\]\.storageTexture\.access = wgpu::StorageTextureAccess::\w+;"),
    ("L-stf", "storage texture format",
     r"entries\[\d+\]\.storageTexture\.format = wgpu::TextureFormat::\w+;"),
    ("L-std", "storage texture view dimension",
     r"entries\[\d+\]\.storageTexture\.viewDimension = wgpu::TextureViewDimension::\w+;"),
    ("L-desc", "descriptor declaration (always named desc)",
     r"wgpu::BindGroupLayoutDescriptor desc\{\};"),
    ("L-lbl", "layout label, string literal",
     r'desc\.label = "[^"]*";'),
    ("L-cnt", "entryCount from the array, never a literal",
     r"desc\.entryCount = entries\.size\(\);"),
    ("L-ent", "entries pointer",
     r"desc\.entries = entries\.data\(\);"),
    ("L-new", "creation into the state member",
     r"\w+ = device_\.CreateBindGroupLayout\(&desc\);"),
    ("L-chk", "boot check on the member just created",
     r"if \(!\w+\) return false;"),
]

GROUP_IDIOMS = [
    ("G-arr", "the entries array declaration",
     r"std::array<wgpu::BindGroupEntry, \d+> entries\{\};"),
    ("G-bind", "seat -> slot: registry constant, never a literal",
     r"entries\[\d+\]\.binding = bind::g\d::\w+;"),
    ("G-buf", "buffer backing member",
     r"entries\[\d+\]\.buffer = \w+;"),
    ("G-off", "buffer window offset (free expression)",
     r"entries\[\d+\]\.offset = [^;]+;"),
    ("G-siz", "buffer window size (free expression)",
     r"entries\[\d+\]\.size = [^;]+;"),
    ("G-txv", "texture view backing member",
     r"entries\[\d+\]\.textureView = \w+;"),
    ("G-smp", "sampler backing member",
     r"entries\[\d+\]\.sampler = \w+;"),
    ("G-cst", "in-block constant (Entity Placement's plant count)",
     r"static constexpr uint32_t \w+ = [^;]+;"),
    ("G-desc", "descriptor declaration, one per block",
     r"wgpu::BindGroupDescriptor \w+\{\};"),
    ("G-lbl", "group label, string literal",
     r'\w+\.label = "[^"]*";'),
    ("G-lblp", "group label from the builder's parameter",
     r"desc\.label = label;"),
    ("G-lay", "the layout this group instantiates",
     r"\w+\.layout = \w+;"),
    ("G-cnt", "entryCount from the array, never a literal",
     r"\w+\.entryCount = entries\.size\(\);"),
    ("G-ent", "entries pointer",
     r"\w+\.entries = entries\.data\(\);"),
    ("G-new", "creation into the state member",
     r"\w+ = device_\.CreateBindGroup\(&\w+\);"),
    ("G-ret", "creation returned from the builder",
     r"return device_\.CreateBindGroup\(&desc\);"),
    ("G-chk", "boot check, single member",
     r"if \(!\w+\) return false;"),
]

GROUP_EXTRA_IDIOMS = [
    ("G-bldr", "the builder lambda header (label + IB window as parameters)",
     r"auto build_render_entity_group = \[&\]\([^)]*\) -> wgpu::BindGroup \{"),
    ("G-call", "builder invocation (three: plans A / B / C)",
     r'\w+ = build_render_entity_group\("[^"]*", \w+, \w+\);'),
    ("G-chk3", "boot check over the three built groups",
     r"if \(!\w+ \|\| !\w+ \|\| !\w+\) return false;"),
]


def census_state_blocks(w):
    """Surfaces L and G — every ';'-terminated statement inside every
    layout / bind group creation block, plus the builder's out-of-block
    machinery for G."""
    raw = BL.read(BL.STATE_HPP)
    src = BL.strip_cpp_comments(raw)

    def run(kind_re, idioms, key):
        s = Surface(key, idioms)
        blocks = []
        for m in re.finditer(
                r"std::array<\s*wgpu::" + kind_re + r"\s*,\s*(\d+)\s*>\s*(\w+)\s*\{\s*\}\s*;",
                src):
            tail = src.find("CreateBindGroupLayout" if key == "L" else "CreateBindGroup(",
                            m.end())
            end = src.find(";", tail) + 1
            end = _extend_through_check(src, end)
            blocks.append((m.start(), end))
            for a, b_ in _block_statements(src, m.start(), end):
                s.feed(norm(raw[a:b_]), BL.line_of(src, a))
        return s, blocks

    sL, layout_blocks = run(r"BindGroupLayoutEntry", LAYOUT_IDIOMS, "L")
    sG, group_blocks = run(r"BindGroupEntry", GROUP_IDIOMS, "G")

    # The builder's machinery sits BETWEEN blocks: the lambda header, the
    # three invocations, and their compound boot check. The check is
    # anchored to the last invocation — the same `if (!a || !b || !c)`
    # shape appears elsewhere in the file for non-binding resources.
    extra = Surface("G+", GROUP_EXTRA_IDIOMS)
    last_invoke_end = None
    for pat in (r"auto\s+build_render_entity_group\s*=\s*\[&\]\([^)]*\)\s*->\s*wgpu::BindGroup\s*\{",
                r"\w+\s*=\s*build_render_entity_group\(\s*\"[^\"]*\"\s*,\s*\w+\s*,\s*\w+\s*\)\s*;"):
        for m in re.finditer(pat, src):
            extra.feed(norm(raw[m.start():m.end()]), BL.line_of(src, m.start()))
            last_invoke_end = m.end()
    if last_invoke_end is not None:
        cm = re.match(r"\s*(if\s*\(\s*!\w+(?:\s*\|\|\s*!\w+)+\s*\)\s*return\s+false\s*;)",
                      src[last_invoke_end:])
        if cm:
            extra.feed(norm(raw[last_invoke_end + cm.start(1):last_invoke_end + cm.end(1)]),
                       BL.line_of(src, last_invoke_end + cm.start(1)))

    # Ordering facts, computed — the counterexamples are the payload.
    def out_of_order(blocks):
        notes = []
        for start, end in blocks:
            idxs = [int(x) for x in re.findall(r"entries\[(\d+)\]\.binding", src[start:end])]
            if idxs != sorted(idxs):
                notes.append("state.hpp:%d — %s"
                             % (BL.line_of(src, start),
                                ",".join(str(i) for i in idxs)))
        return notes

    vis_rev = [BL.line_of(src, m.start()) for m in re.finditer(
        r"entries\[\d+\]\.visibility\s*=\s*wgpu::ShaderStage::Fragment\s*\|\s*wgpu::ShaderStage::Vertex",
        src)]
    return sL, sG, extra, {
        "ooo_L": out_of_order(layout_blocks),
        "ooo_G": out_of_order(group_blocks),
        "n_L": len(layout_blocks), "n_G": len(group_blocks),
        "vis_reversed": vis_rev,
    }


PIPE_IDIOMS = [
    ("P-help", "strataLayoutFor — the shared four-strata wrapper (LOOM_2: "
               "WORLD implicit, then frame / state / textures; DOMESDAY_2 "
               "F2-b1 named it, so the label leads; its inner statements are "
               "censused as part of this idiom)",
     r"wgpu::PipelineLayout strataLayoutFor\(const char\* \w+, "
     r"wgpu::BindGroupLayout \w+, wgpu::BindGroupLayout \w+, "
     r"wgpu::BindGroupLayout \w+\) \{.*"),
    ("P-arr", "the ordered layout list",
     r"std::array<wgpu::BindGroupLayout, \d+> \w+ = \{[^}]*\};"),
    ("P-desc", "pipeline layout descriptor declaration",
     r"wgpu::PipelineLayoutDescriptor \w+\{\};"),
    ("P-cnt", "bindGroupLayoutCount from the array, never a literal",
     r"\w+\.bindGroupLayoutCount = \w+\.size\(\);"),
    ("P-dat", "bindGroupLayouts pointer",
     r"\w+\.bindGroupLayouts = \w+\.data\(\);"),
    ("P-new", "pipeline layout creation, named local",
     r"wgpu::PipelineLayout \w+ = device_\.CreatePipelineLayout\(&\w+\);"),
    ("P-for", "pipeline layout via the shared wrapper, named at creation",
     r"wgpu::PipelineLayout \w+ = strataLayoutFor\(\"[^\"]*\", \w+, \w+, \w+\);"),
    ("P-prm", "PipelineLayout as a builder parameter (makeComputePipeline; "
              "makeShadow's nullptr-sentinel default)",
     r"wgpu::PipelineLayout \w+( = nullptr\) -> bool \{)?,?"),
]

# `\bPipelineLayout\b` cannot match inside `CreatePipelineLayout` (no
# word boundary after the 'e'), so the create verb is its own token —
# without it an inline `device_.CreatePipelineLayout(&pld)` passed as a
# call argument would carry no token and escape the boundary law.
PIPE_TOKENS = [r"std::array<\s*wgpu::BindGroupLayout\b", r"\bPipelineLayoutDescriptor\b",
               r"\bPipelineLayout\b", r"\bCreatePipelineLayout\b",
               r"\bbindGroupLayoutCount\b",
               r"\bbindGroupLayouts\b", r"\bstrataLayoutFor\b"]


def census_renderer(w):
    """Surface P — instance spans are regex-matched, then the boundary
    law: every occurrence of a pipeline-layout token in the file must
    fall inside exactly one instance span. A token no idiom claims is a
    binding-surface site this census cannot see — a STOP."""
    raw = BL.read(BL.RENDERER_HPP)
    src = BL.strip_cpp_comments(raw)
    s = Surface("P", PIPE_IDIOMS)
    spans = []

    hm = re.search(r"wgpu::PipelineLayout\s+strataLayoutFor\s*\(\s*const\s+char\s*\*\s*\w+\s*,"
                   r"\s*wgpu::BindGroupLayout\s+\w+\s*,\s*wgpu::BindGroupLayout\s+\w+\s*,"
                   r"\s*wgpu::BindGroupLayout\s+\w+\s*\)\s*\{",
                   src)
    helper_span = None
    if hm:
        depth, p = 0, src.find("{", hm.start())
        while p < len(src):
            if src[p] == "{":
                depth += 1
            elif src[p] == "}":
                depth -= 1
                if depth == 0:
                    break
            p += 1
        helper_span = (hm.start(), p + 1)
        first_line = norm(raw[hm.start():src.find("{", hm.start()) + 1])
        s.feed(first_line + " ...", BL.line_of(src, hm.start()))
        spans.append(helper_span)

    def masked(pos):
        return helper_span and helper_span[0] <= pos < helper_span[1]

    for pat in (r"std::array<\s*wgpu::BindGroupLayout\s*,\s*\d+\s*>\s*\w+\s*=\s*\{[^}]*\}\s*;",
                r"wgpu::PipelineLayoutDescriptor\s+\w+\s*\{\s*\}\s*;",
                r"\w+\.bindGroupLayoutCount\s*=\s*[^;]+;",
                r"\w+\.bindGroupLayouts\s*=\s*[^;]+;",
                r"wgpu::PipelineLayout\s+\w+\s*=\s*device_\.CreatePipelineLayout\(\s*&\w+\s*\)\s*;",
                r"wgpu::PipelineLayout\s+\w+\s*=\s*strataLayoutFor\(\s*\"(?:[^\"\\\\]|\\\\.)*\"\s*,"
                r"\s*\w+\s*,\s*\w+\s*,\s*\w+\s*\)\s*;",
                r"wgpu::PipelineLayout\s+\w+\s*(?:=\s*nullptr\s*\)\s*->\s*bool\s*\{|,)"):
        for m in re.finditer(pat, src):
            if masked(m.start()):
                continue
            s.feed(norm(raw[m.start():m.end()]), BL.line_of(src, m.start()))
            spans.append((m.start(), m.end()))

    # The boundary law.
    for tok in PIPE_TOKENS:
        for m in re.finditer(tok, src):
            inside = sum(1 for a, b_ in spans if a <= m.start() < b_)
            if inside != 1:
                line = BL.line_of(src, m.start())
                ls = src.rfind("\n", 0, m.start()) + 1
                le = src.find("\n", m.start())
                s.stops.append("P boundary token %r at renderer.hpp:%d inside %d "
                               "instance spans: %s"
                               % (tok, line, inside, norm(raw[ls:le if le > 0 else len(raw)])))

    # Ordering facts. A descriptor "precedes its array" if the nearest
    # layout-list array declaration sits after it in the file.
    arr_starts = [m.start() for m in re.finditer(
        r"std::array<\s*wgpu::BindGroupLayout\s*,\s*\d+\s*>\s*\w+\s*=", src)
        if not masked(m.start())]
    desc_first = []
    for m in re.finditer(r"wgpu::PipelineLayoutDescriptor\s+(\w+)\s*\{\s*\}\s*;", src):
        if masked(m.start()) or not arr_starts:
            continue
        nearest = min(arr_starts, key=lambda a: abs(a - m.start()))
        if nearest > m.start():
            desc_first.append(BL.line_of(src, m.start()))
    unchecked = []
    for m in re.finditer(r"wgpu::PipelineLayout\s+(\w+)\s*=\s*device_\.CreatePipelineLayout\(\s*&\w+\s*\)\s*;", src):
        if masked(m.start()):
            continue
        if not re.match(r"\s*if\s*\(\s*!%s\s*\)\s*return\s+false\s*;" % re.escape(m.group(1)),
                        src[m.end():]):
            unchecked.append("%s (line %d)" % (m.group(1), BL.line_of(src, m.start())))
    return s, {"desc_before_array": desc_first, "unchecked_creates": unchecked}


def ml1(w, surfaces):
    """Idiom totals sum to instance totals per mirror, and no instance
    fell outside the exactly-one-match law."""
    bad = []
    parts = []
    for s in surfaces:
        sum_ = sum(s.counts.values())
        parts.append("%s %d/%d" % (s.key, sum_, s.total))
        if sum_ != s.total:
            bad.append("%s: %d idiom matches over %d instances" % (s.key, sum_, s.total))
        bad.extend(s.stops)
    w.record("ML-1", not bad,
             "idiom totals sum to instance totals on every surface — " + ", ".join(parts)
             if not bad else "STOP — " + " · ".join(bad))


# ═══════════════════════════════════════════════════════════════════════
# M2 — THE WGSL SCATTER MAP
# ═══════════════════════════════════════════════════════════════════════

def scatter_map(w, wgsl, ledger):
    raw = BL.read(BL.WORLD_WGSL)
    src = wgsl["src"]              # comment-stripped, offsets preserved
    rawlines = raw.split("\n")
    striplines = src.split("\n")
    decls = sorted(wgsl["decls"], key=lambda d: d.line)

    runs = []
    cur = [decls[0]]
    for a, b_ in zip(decls, decls[1:]):
        between = striplines[a.line:b_.line - 1]        # lines strictly between
        if all(not ln.strip() for ln in between):
            cur.append(b_)
        else:
            runs.append(cur)
            cur = [b_]
    runs.append(cur)

    # Attachment — the nearest-preceding rule (Table H's A) decides WHICH
    # block covers a site; taken verbatim it covers every site that has
    # any earlier comment in the file, which would make this column a
    # constant. The question M2 exists to answer is whether prose TRAVELS
    # with the declaration if the generator moves it, so the predicate
    # here is rule A restricted to adjacency: the nearest preceding
    # comment block counts as attached iff nothing but whitespace stands
    # between its end and the declaration. (Deviation, disclosed: the
    # handoff cites rule A; the adjacency restriction is this census's.)
    blocks = BL.comment_blocks(raw)
    attach = {}
    for d in decls:
        # The site is located in the COMMENT-STRIPPED text (offsets are
        # preserved by the stripper), so a banner elsewhere that quotes a
        # declaration header cannot relocate the site into prose — the
        # orb bind-group-topology comment already writes `@binding(410)`
        # lines in exactly that style.
        m = re.search(r"@group\(%d\)\s*@binding\(%d\)\s*var[^;]*?\b%s\b"
                      % (d.group, d.binding, re.escape(d.symbol)), src)
        o = m.start() if m else None
        got = False
        if o is not None:
            prev = None
            for bl_ in blocks:
                if bl_[1] <= o:
                    prev = bl_
                else:
                    break
            if prev is not None and raw[prev[1]:o].strip() == "":
                got = True
        attach[d.symbol] = got
    defended = ledger["tableH_wgsl"]
    w.record("M2-0", len(decls) == sum(len(r) for r in runs),
             "%d declarations partitioned into %d contiguous runs; %d carry an "
             "attached comment block (rule A), %d are Table H defended sites"
             % (len(decls), len(runs), sum(1 for v in attach.values() if v),
                sum(1 for d in decls if d.symbol in defended)))
    return {"runs": runs, "attach": attach, "defended": defended,
            "rawlines": rawlines}


# ═══════════════════════════════════════════════════════════════════════
# M3 — THE FIFTH-HOME GREP (count law: every boundary stated)
# ═══════════════════════════════════════════════════════════════════════

def fifth_home(w, wgsl, out_path):
    findings = {"a": [], "b_literal": [], "b_other": [], "c": []}
    shape_stops = []

    # (a) `@binding(` outside the 98 declarations. Boundary: the
    # comment-stripped code text of every *.wgsl under src/; declaration
    # spans are DECL_RE's matches in world.wgsl. Comment mentions are
    # counted, not sites.
    wgsl_files = []
    for root, dirs, files in os.walk(os.path.join(REPO, "src")):
        dirs.sort()
        for f in sorted(files):
            if f.endswith(".wgsl"):
                wgsl_files.append(os.path.join(root, f))
    a_counts = {}
    for path in wgsl_files:
        raw = BL.read(path)
        stripped = BL.strip_wgsl_comments(raw)
        rel = os.path.relpath(path, REPO)
        spans = []
        if os.path.abspath(path) == os.path.abspath(BL.WORLD_WGSL):
            spans = [(m.start(), m.end()) for m in BL.DECL_RE.finditer(stripped)]
        code_hits = [m.start() for m in re.finditer(r"@binding\s*\(", stripped)]
        outside = [p for p in code_hits
                   if not any(a <= p < b_ for a, b_ in spans)]
        raw_hits = len(re.findall(r"@binding\s*\(", raw))
        a_counts[rel] = (raw_hits, len(code_hits), len(spans), len(outside))
        for p in outside:
            ls = stripped.rfind("\n", 0, p) + 1
            le = stripped.find("\n", p)
            findings["a"].append("%s:%d: %s" % (rel, BL.line_of(stripped, p),
                                                norm(raw[ls:le if le > 0 else len(raw)])))

    # (b) `.binding =` with a non-registry right side. Boundary: the
    # comment-stripped text of every *.hpp under src/ (the handoff's
    # sweep), EXTENDED to .cpp/.h/.cc so a seat assignment in a
    # translation unit cannot hide on the wrong extension — the
    # extension is reported separately so the prescribed sweep stays
    # legible.
    hpp_files = []
    ext_files = []
    for root, dirs, files in os.walk(os.path.join(REPO, "src")):
        dirs.sort()
        for f in sorted(files):
            if f.endswith(".hpp"):
                hpp_files.append(os.path.join(root, f))
            elif f.endswith((".cpp", ".h", ".cc")):
                ext_files.append(os.path.join(root, f))
    b_counts = {"bind": 0, "literal": 0, "other": 0, "ext": 0}
    for path in hpp_files + ext_files:
        stripped = BL.strip_cpp_comments(BL.read(path))
        rel = os.path.relpath(path, REPO)
        extension = path in set(ext_files)
        for m in re.finditer(r"\.binding\s*=\s*([^;]+);", stripped):
            rhs = norm(m.group(1))
            where = "%s:%d: %s" % (rel, BL.line_of(stripped, m.start()),
                                   norm(m.group(0)))
            if extension:
                b_counts["ext"] += 1
            if re.fullmatch(r"bind::g\d::\w+", rhs):
                b_counts["bind"] += 1
            elif re.fullmatch(r"\d+[uU]?", rhs):
                b_counts["literal"] += 1
                findings["b_literal"].append(where)
            else:
                b_counts["other"] += 1
                findings["b_other"].append(where)
                shape_stops.append("(b) RHS is neither a bind:: constant nor an "
                                   "integer literal — " + where)

    # (c) `bind::` outside the homes. Boundary: every file under src/,
    # any extension, AS IT SITS ON DISK (read_raw — the (b) sweep reads
    # state.hpp include-expanded; counting per disk file here keeps one
    # token one count). binding_surface.gen.inc is state.hpp's
    # generated half (LOOM_1 U3) and counts as part of that home.
    # Instruments (tools/) and reports (audit/, docs/) cite the
    # constants rather than bind them; counted below, not listed.
    homes = {os.path.abspath(p) for p in (BL.REGISTRY_HPP, BL.STATE_HPP,
                                          BL.GEN_INC, BL.RENDERER_HPP)}
    c_counts = {}
    for root, dirs, files in os.walk(os.path.join(REPO, "src")):
        dirs.sort()
        for f in sorted(files):
            path = os.path.join(root, f)
            try:
                raw = BL.read_raw(path)
            except (UnicodeDecodeError, OSError):
                continue
            if f.endswith((".hpp", ".cpp", ".h", ".cc", ".inc")):
                text = BL.strip_cpp_comments(raw)
            elif f.endswith(".wgsl"):
                text = BL.strip_wgsl_comments(raw)
            else:
                text = raw
            hits = [m.start() for m in re.finditer(r"\bbind::", text)]
            if not hits:
                continue
            rel = os.path.relpath(path, REPO)
            c_counts[rel] = len(hits)
            if os.path.abspath(path) not in homes:
                for p in hits:
                    ls = text.rfind("\n", 0, p) + 1
                    le = text.find("\n", p)
                    findings["c"].append("%s:%d: %s" % (rel, BL.line_of(text, p),
                                                        norm(raw[ls:le if le > 0 else len(raw)])))
    cite_counts = {}
    skip = {os.path.abspath(out_path), os.path.abspath(DEFAULT_OUT)}
    for d in ("tools", "audit"):
        n = 0
        for root, dirs, files in os.walk(os.path.join(REPO, d)):
            dirs.sort()
            for f in sorted(files):
                path = os.path.join(root, f)
                # The census's own artifact is excluded: a report that
                # counted itself would change its own count on re-run,
                # and G2 requires two runs on one tree to be identical.
                if os.path.abspath(path) in skip:
                    continue
                try:
                    n += len(re.findall(r"\bbind::", BL.read(path)))
                except (UnicodeDecodeError, OSError):
                    continue
        cite_counts[d] = n

    ok = not shape_stops
    w.record("M3-0", ok,
             "every fifth-home hit fits the table shape — (a) %d outside-declaration "
             "site(s), (b) %d literal RHS, %d other RHS, (c) %d code hit(s) outside "
             "the three homes"
             % (len(findings["a"]), len(findings["b_literal"]),
                len(findings["b_other"]), len(findings["c"]))
             if ok else "STOP — " + "; ".join(shape_stops))
    return {"findings": findings, "a_counts": a_counts, "b_counts": b_counts,
            "c_counts": c_counts, "cite_counts": cite_counts,
            "wgsl_files": [os.path.relpath(p, REPO) for p in wgsl_files],
            "hpp_file_count": len(hpp_files), "ext_file_count": len(ext_files)}


# ═══════════════════════════════════════════════════════════════════════
# M5 — THE STRUCT PAIR INVENTORY (counted, not reconciled)
# ═══════════════════════════════════════════════════════════════════════

def struct_pairs(w, wgsl):
    struct_names = set(wgsl["structs"])
    named = {}                     # struct -> [slot symbols naming it]
    for d in wgsl["decls"]:
        t = d.wgsl_type
        m = re.fullmatch(r"(\w+)", t) or re.match(r"array<\s*(\w+)\s*[,>]", t)
        if m and m.group(1) in struct_names:
            named.setdefault(m.group(1), []).append(d.symbol)

    wraw = BL.read(BL.WORLD_WGSL)
    wsrc = BL.strip_wgsl_comments(wraw)
    cpp_files = [BL.STATE_HPP]
    if os.path.isdir(CONTRACTS_DIR):
        for root, dirs, files in os.walk(CONTRACTS_DIR):
            dirs.sort()
            for f in sorted(files):
                if f.endswith((".hpp", ".h")):
                    cpp_files.append(os.path.join(root, f))
    cpp_text = {p: BL.strip_cpp_comments(BL.read(p)) for p in cpp_files}

    def find_asserts(name):
        out = []
        pat = re.compile(r"static_assert\s*\(")
        word = re.compile(r"(?<![\w])(?:GPU)?%s(?![\w])" % re.escape(name))
        for p, text in cpp_text.items():
            for m in pat.finditer(text):
                depth, i = 0, m.end() - 1
                while i < len(text):
                    if text[i] == "(":
                        depth += 1
                    elif text[i] == ")":
                        depth -= 1
                        if depth == 0:
                            break
                    i += 1
                stmt = text[m.start():i + 1]
                if word.search(stmt):
                    out.append("%s:%d: %s" % (os.path.relpath(p, REPO),
                                              BL.line_of(text, m.start()),
                                              norm(stmt)[:220]))
        return out

    rows = []
    for name in sorted(named):
        dm = re.search(r"^struct\s+%s\s*\{" % re.escape(name), wsrc, re.M)
        # The prescribed search: a DEFINITION of GPU<name> or <name> (a
        # bare `struct X;` forward declaration is a name, not a twin —
        # contracts forward-declares an unrelated CPU RibbonState).
        twin, site = None, None
        for cand in ("GPU" + name, name):
            for p, text in cpp_text.items():
                tm = re.search(r"\bstruct\s+(?:alignas\s*\([^)]*\)\s*)?%s\s*\{"
                               % re.escape(cand), text)
                if tm:
                    twin, site = cand, "%s:%d" % (os.path.relpath(p, REPO),
                                                  BL.line_of(text, tm.start()))
                    break
            if twin:
                break
        # Where the prescribed names find nothing, the name search still
        # runs over RAW text (prose included): a comment pinning a
        # differently-named twin is evidence, and it is listed so the
        # finding carries its trail.
        cited = []
        if not twin:
            word = re.compile(r"(?<![\w])%s(?![\w])" % re.escape(name))
            for p in cpp_files:
                rawc = BL.read(p)
                for mm in word.finditer(rawc):
                    cited.append("%s:%d" % (os.path.relpath(p, REPO),
                                            BL.line_of(rawc, mm.start())))
            cited = cited[:4]
        rows.append({
            "name": name,
            "slots": sorted(set(named[name])),
            "wgsl_line": BL.line_of(wsrc, dm.start()) if dm else None,
            "twin": twin,
            "twin_site": site,
            "cited": cited,
            "asserts": find_asserts(name),
        })
    unresolved = [r["name"] for r in rows if not r["twin"]]
    w.record("M5-0", True,
             "%d struct types named by slot store types; %d resolve to a C++ twin, "
             "%d do not (%s) — findings, not STOPs"
             % (len(rows), len(rows) - len(unresolved), len(unresolved),
                ", ".join(unresolved) or "none"))
    return rows


# ═══════════════════════════════════════════════════════════════════════
# THE SLOT-CONSISTENCY PROBE (feeds M4)
#
# SLOTS carries kind / accesses / texture facts per slot. That only
# covers the seats if every seat at a slot states the same facts. Where
# aliases give one slot two access modes, the C++ seat picks ONE of them
# — a fact the SEATS relation as specified has no column for.
# ═══════════════════════════════════════════════════════════════════════

def slot_consistency(layouts):
    per_slot = {}
    for L in layouts:
        for e in L["entries"]:
            per_slot.setdefault(e.binding_const, {}).setdefault(
                (e.kind, e.access), []).append("%s entries[%d]" % (L["label"], e.entry_index))
    varied = {k: v for k, v in per_slot.items() if len(v) > 1}
    return per_slot, varied


def accessor_map():
    """state.hpp layout member -> its gpuState accessor name(s)."""
    state = BL.strip_cpp_comments(BL.read(BL.STATE_HPP))
    inv = {}
    for m in re.finditer(
            r"wgpu::BindGroupLayout\s+(\w+)\s*\(\s*\)\s*const\s*\{\s*return\s+(\w+)\s*;\s*\}", state):
        inv.setdefault(m.group(2), []).append(m.group(1))
    return inv


# ═══════════════════════════════════════════════════════════════════════
# M7 — THE USAGE CENSUS (LOOM_2 U0)
#
# Where the groups are BOUND: every SetBindGroup call in src/, its
# enclosing function, the group index it names, the state member it
# resolves to, and any dynamic-offset arguments. The recut's rewrite
# list is exactly these rows; M7-0 proves each site already binds its
# group at the one index 0c-4 records for the group's layout.
# ═══════════════════════════════════════════════════════════════════════

def group_accessor_map():
    """gpuState bind-group accessor name -> state.hpp member."""
    state = BL.strip_cpp_comments(BL.read(BL.STATE_HPP))
    out = {}
    for m in re.finditer(
            r"wgpu::BindGroup\s+(\w+)\s*\(\s*\)\s*const\s*\{\s*return\s+(\w+)\s*;\s*\}", state):
        out[m.group(1)] = m.group(2)
    return out


def _m7_functions(text):
    """[(name, params, body_start, body_end)] for every void/bool
    function DEFINITION — indented members and column-0 free functions
    alike. Span-keyed, not name-keyed: the bodies forward-declare their
    pass functions, and a name-keyed map hands a declaration the next
    function's body (which is how dispatch_compute once claimed
    upload_ground_entries' span). The `{` guard skips declarations."""
    out = []
    for m in re.finditer(
            r"(?:^|\n)[ \t]*(?:static\s+|inline\s+)*(?:void|bool)\s+(\w+)\s*\(",
            text):
        j = m.end() - 1
        depth, k = 0, j
        while k < len(text):
            if text[k] == "(":
                depth += 1
            elif text[k] == ")":
                depth -= 1
                if depth == 0:
                    break
            k += 1
        after = k + 1
        while after < len(text) and text[after] in " \t\r\n":
            after += 1
        if after >= len(text) or text[after] != "{":
            continue
        depth, p = 0, after
        while p < len(text):
            if text[p] == "{":
                depth += 1
            elif text[p] == "}":
                depth -= 1
                if depth == 0:
                    break
            p += 1
        out.append((m.group(1), text[j + 1:k], after, p))
    return out


def _m7_enclosing(fns, pos):
    """The innermost definition whose body contains pos."""
    best = None
    for name, params, b, e in fns:
        if b <= pos <= e and (best is None or b > best[2]):
            best = (name, params, b, e)
    return best


def usage_census(w, cen, groups, invokes, layouts):
    acc = group_accessor_map()
    # LOOM_2 A5: a zero-entry layout is a stratum FILLER. 0c-4 exempts it
    # from the one-index law (its group stands wherever a pipeline leaves
    # a stratum unused), so the index check here must exempt its group at
    # ANY index — by the same PREDICATE, entryCount == 0, asserted
    # against the parsed layout rather than trusting a name.
    empty_layouts = {L["member"] for L in layouts if len(L["entries"]) == 0}
    member_layout = {}
    builder_layout = None
    for g in groups:
        if g["member"]:
            member_layout[g["member"]] = g["layout_member"]
        else:
            builder_layout = g["layout_member"]
    for iv in invokes:
        member_layout[iv["member"]] = builder_layout

    # Census targets are the files that call SetBindGroup; the CALLER
    # chase for helper parameters must search every C++ file under src/
    # — the pass-recording helpers are invoked from body/surface files
    # that never spell SetBindGroup themselves.
    files = []
    all_files = []
    for root, dirs, fs in os.walk(os.path.join(REPO, "src")):
        dirs.sort()
        for f in sorted(fs):
            if f.endswith((".hpp", ".cpp")):
                path = os.path.join(root, f)
                text = BL.strip_cpp_comments(BL.read_raw(path))
                all_files.append((path, text))
                if "SetBindGroup" in text or "GetBindGroupLayout" in text:
                    files.append((path, text))

    fns_of = {path: _m7_functions(text) for path, text in all_files}

    def resolve_expr(expr, enc, text, depth=0):
        """One group argument -> set of state.hpp bind group members.
        enc is the enclosing definition (name, params, body_start,
        body_end) or None."""
        expr = norm(expr)
        am = re.search(r"(\w+)\s*\(\s*\)\s*$", expr)
        if am and am.group(1) in acc:
            return {acc[am.group(1)]}
        if re.fullmatch(r"\w+", expr) and depth < 3 and enc is not None:
            body = text[enc[2]:enc[3]]
            # a local bound earlier in the enclosing body?
            lm = None
            for a in re.finditer(
                    r"(?:wgpu::BindGroup\s+|auto\s+)?%s\s*=\s*([^;=][^;]*);"
                    % re.escape(expr), body):
                lm = a.group(1)
            if lm:
                return resolve_expr(lm, enc, text, depth + 1)
            # a parameter? chase every call site of the enclosing function.
            params = BL.param_names(enc[1])
            if expr in params:
                pi = params.index(expr)
                out = set()
                for path2, text2 in all_files:
                    for cm in re.finditer(r"[.>\s]%s\s*\(" % re.escape(enc[0]),
                                          text2):
                        j = cm.end() - 1
                        depth2, k = 0, j
                        while k < len(text2):
                            if text2[k] == "(":
                                depth2 += 1
                            elif text2[k] == ")":
                                depth2 -= 1
                                if depth2 == 0:
                                    break
                            k += 1
                        args2 = BL.split_args(text2[j + 1:k])
                        if pi < len(args2):
                            enc2 = _m7_enclosing(fns_of[path2], cm.start())
                            out |= resolve_expr(args2[pi], enc2, text2,
                                                depth + 1)
                return out
        return set()

    rows = []
    problems = []
    get_layout_uses = 0
    for path, text in files:
        rel = os.path.relpath(path, REPO)
        fns = fns_of[path]
        get_layout_uses += len(re.findall(r"\bGetBindGroupLayout\b", text))
        for m in re.finditer(r"(\w+)\.SetBindGroup\s*\(([^;]*)\)\s*;", text):
            args = BL.split_args(m.group(2))
            if len(args) < 2 or not re.fullmatch(r"\d+", args[0]):
                problems.append("%s:%d: unparseable SetBindGroup args: %s"
                                % (rel, BL.line_of(text, m.start()),
                                   norm(m.group(0))))
                continue
            idx = int(args[0])
            enc = _m7_enclosing(fns, m.start())
            fname = enc[0] if enc else "(file scope)"
            members = resolve_expr(args[1], enc, text)
            offsets = ", ".join(args[2:]) if len(args) > 2 else None
            if not members:
                problems.append("%s:%d: group expression %r resolves to no "
                                "state member" % (rel, BL.line_of(text, m.start()),
                                                  norm(args[1])))
            for mem in sorted(members):
                lay = member_layout.get(mem)
                if lay in empty_layouts:
                    continue
                want = cen["group_index"].get(lay)
                if want != idx:
                    problems.append("%s:%d: %s bound at index %d; 0c-4 places "
                                    "its layout %s at %s"
                                    % (rel, BL.line_of(text, m.start()), mem,
                                       idx, lay, want))
            rows.append({"file": rel, "line": BL.line_of(text, m.start()),
                         "fn": fname, "index": idx, "expr": norm(args[1]),
                         "members": sorted(members), "offsets": offsets})
    w.record("M7-0", not problems,
             "%d SetBindGroup sites over %d files, every group expression "
             "resolves to a state member bound at exactly the index 0c-4 "
             "records for its layout; %d GetBindGroupLayout use(s)"
             % (len(rows), len(files), get_layout_uses)
             if not problems else "STOP — " + "; ".join(problems[:12]))
    return {"rows": rows, "files": [os.path.relpath(p, REPO) for p, _ in files],
            "get_layout_uses": get_layout_uses}



# ═══════════════════════════════════════════════════════════════════════
# M4 — THE COVERAGE VERDICT (the residue list is the crown)
# ═══════════════════════════════════════════════════════════════════════

def coverage(reg_info, wgsl_info, state_info, pipe_info, groups, invokes,
             varied_slots, m2, sL):
    """One row per emitted fact per mirror: the relation+column that
    covers it, or RESIDUE with an example site. The fact list is authored
    here — it is the census's reading of what each mirror's text encodes
    — and every example site is computed from the tree, so a row cannot
    outlive its referent silently."""
    ex_reversed = ("state.hpp:%d" % state_info["vis_reversed"][0]
                   if state_info["vis_reversed"] else "(none found)")
    ex_ooo_L = (state_info["ooo_L"][0].split(" — ")[0]
                if state_info["ooo_L"] else "(none found)")
    ex_ooo_G = (state_info["ooo_G"][0].split(" — ")[0]
                if state_info["ooo_G"] else "(none found)")
    ex_defended = next((d for d, v in m2["attach"].items()
                        if v and d in m2["defended"]), "(none)")
    ex_varied = next(iter(sorted(varied_slots)), "(none)")
    ex_invoke = invokes[0] if invokes else None
    ex_param_label = next((g for g in groups if g["param_label"]), None)
    fn_sites = []
    _ssrc = BL.strip_cpp_comments(BL.read(BL.STATE_HPP))
    for fm in re.finditer(r"\bbool\s+(\w+)\s*\(", _ssrc):
        fn_sites.append((BL.line_of(_ssrc, fm.start()), fm.group(1)))

    def _fn_of(line):
        name = None
        for ln, nm in fn_sites:
            if ln > line:
                break
            name = nm
        return name

    _tally = {}
    for g in groups:
        _tally[_fn_of(g["line"])] = _tally.get(_fn_of(g["line"]), 0) + 1
    _boot = max(sorted(_tally), key=lambda nm: _tally[nm])
    ex_rebuild = next((g for g in groups if _fn_of(g["line"]) != _boot), None)
    ex_minsize = "state.hpp:%d" % sL.exemplar_line["L-min"] if sL.counts.get("L-min") else "(none)"

    R, C = "RESIDUE", "covered"
    rows = [
        # ─── binding_registry.hpp ────────────────────────────────────
        ("R", "constant name", C, "SLOTS.symbol", "bind::g0::signal"),
        ("R", "namespace (g0 / g1 / g2)", C, "SLOTS.namespace", "namespace g2"),
        ("R", "binding number", C, "SLOTS key (group, binding)", "g0::signal = 0"),
        ("R", "file banner counts (98 declarations / 95 slots / 3 aliases)", C,
         "SLOTS cardinalities (derived)", "binding_registry.hpp banner"),
        ("R", "trailing comment prose on constants", R,
         "attached prose", "g0::vp_data '// aka fc_vp (frustum-cull alias)'"),
        ("R", "section banners and band prose", R,
         "attached prose", "'GROUP 0 — the everything-group' banner"),
        ("R", "the render = compute + 200 static_assert band (%d asserts)"
              % reg_info["static_asserts"], R,
         "authored witness selection", "static_assert(g0::render_vp == g0::vp_data + 199, ...)"),
        ("R", "subsystem banding and blank-line grouping", R,
         "ordering", "'terrain / patch lattice (20-30)' band"),
        ("R", "column alignment of names and values", R,
         "idiom formatting", "the aligned '= NNN;' column"),
        # ─── world.wgsl ──────────────────────────────────────────────
        ("W", "symbol", C, "SLOTS.symbol / SLOTS.aliases", "fc_config aliasing config"),
        ("W", "group / binding numbers", C, "SLOTS key (group, binding)", "@group(0) @binding(0)"),
        ("W", "address space", C, "SLOTS.address_space", "var<uniform>"),
        ("W", "access mode", C, "SLOTS.accesses", "var<storage, read_write>"),
        ("W", "store type spelling", C, "SLOTS.store type", "array<AgentState, 32>"),
        ("W", "runtime-array flag", C, "SLOTS.runtime-array (derivable from store type)",
         "array<T> with no extent"),
        ("W", "declaration placement among code (%d runs)" % len(m2["runs"]), R,
         "ordering — the M2 scatter map", "runs table below"),
        ("W", "attached comment blocks (%d adjacent) and trailing // comments (%d)"
              % (sum(1 for v in m2["attach"].values() if v), wgsl_info["trailing"]), R,
         "attached prose", "`%s` (Table H site)" % ex_defended),
        ("W", "column-aligned padding (%d decls) and pre-colon spacing (%d decls)"
              % (wgsl_info["padded"], wgsl_info["spaced_colon"]), R,
         "idiom formatting", "@binding(0)   var<uniform>             signal"),
        # ─── state.hpp layouts ───────────────────────────────────────
        ("L", "layout member name", C, "LAYOUTS.member", "computeEntityBindGroupLayout_"),
        ("L", "layout label", C, "LAYOUTS.label", '"Compute Entity Layout"'),
        ("L", "gpuState accessor", C, "LAYOUTS.accessor", "compute_entity_layout()"),
        ("L", "entry index", C, "SEATS key (layout, entry index)", "entries[14]"),
        ("L", "seat -> slot reference", C, "SEATS.slot ref", "bind::g0::shadow_slot"),
        ("L", "visibility mask (member set)", C, "SEATS.visibility", "Vertex | Fragment"),
        ("L", "visibility mask OPERAND ORDER (1 reversed site)", R,
         "idiom formatting", ex_reversed),
        ("L", "hasDynamicOffset", C, "SEATS.hasDynamicOffset", "entries[14] shadow_slot"),
        ("L", "buffer/sampler/texture/storageTexture type facts", C,
         "SLOTS.kind + accesses + texture sample/format/dimension "
         "(per-slot, where seats agree)", "TextureSampleType::Float"),
        ("L", "seat access type where an aliased slot's seats DISAGREE (%d slot%s)"
              % (len(varied_slots), "" if len(varied_slots) == 1 else "s"), R,
         "SEATS needs an access column (or the slot ref must name the alias)",
         ex_varied),
        ("L", "buffer.minBindingSize (%d site%s)"
              % (sL.counts.get("L-min", 0),
                 "" if sL.counts.get("L-min", 0) == 1 else "s"), R,
         "no SEATS column holds it", ex_minsize),
        ("L", "entry statement order (%d block%s out of index order)"
              % (len(state_info["ooo_L"]),
                 "" if len(state_info["ooo_L"]) == 1 else "s"), R,
         "ordering", ex_ooo_L),
        ("L", "descriptor boilerplate, boot checks, blank-line rhythm", R,
         "idiom template", "desc.entryCount = entries.size();"),
        ("L", "attached comment prose (defended seats)", R,
         "attached prose", "Render Entity Layout entries[16]/[17]"),
        ("L", "block order within createBindGroupLayouts", R,
         "ordering", "state.hpp block sequence"),
        # ─── state.hpp bind groups ───────────────────────────────────
        ("G", "group label (string-literal sites)", C, "GROUPS.label",
         '"Compute Entity BindGroup"'),
        ("G", "group label built from a parameter (3 groups, 1 block)", R,
         "irregular label strings",
         "%s at state.hpp:%d" % (ex_param_label["label"], ex_param_label["line"])
         if ex_param_label else "(none)"),
        ("G", "the layout a group instantiates", C, "GROUPS.layout ref",
         "desc.layout = computeEntityBindGroupLayout_"),
        ("G", "per-seat backing resource member", C,
         "GROUPS backing column — the M6 map", "entries[0].buffer = signalBuffer_"),
        ("G", "buffer window .size expressions (one per buffer seat)", R,
         "no GROUPS column holds them; enumerated in M6",
         "entries[3].size = Dim::MAX_AGENTS * sizeof(GPUAgentState)"),
        ("G", "buffer window .offset expressions (3 sites)", R,
         "no GROUPS column holds them; enumerated in M6",
         "entries[11].offset = listOff"),
        ("G", "the builder: one block minting three groups (A/B/C IB windows)", R,
         "creation-site multiplicity",
         "build_render_entity_group at state.hpp:%d" % ex_invoke["line"]
         if ex_invoke else "(none)"),
        ("G", "the exhibition rebuild (a bind group re-made after boot)", R,
         "creation cadence — a bind group re-made after boot",
         "state.hpp:%d %s" % (ex_rebuild["line"], ex_rebuild["label"])
         if ex_rebuild else "(none)"),
        ("G", "in-block constant (PLANT_GROUND_COUNT)", R,
         "idiom template", "Entity Placement Compute BindGroup block"),
        ("G", "entry statement order (%d block%s out of index order)"
              % (len(state_info["ooo_G"]),
                 "" if len(state_info["ooo_G"]) == 1 else "s"), R,
         "ordering", ex_ooo_G),
        ("G", "descriptor boilerplate, boot checks, entry prose", R,
         "idiom template / attached prose", "desc.entries = entries.data();"),
        # ─── renderer.hpp ────────────────────────────────────────────
        ("P", "ordered bind group layout list", C,
         "PIPELINES.ordered layout list", "{ computeEntityLayout_, computeTextureLayout_, roomLayout_ }"),
        ("P", "entry points", C, "PIPELINES.entry points", "Entry::UPDATE_PLAYER_AGENT"),
        ("P", "vertex buffers / attributes", C,
         "PIPELINES.vertex buffers/attributes", "orb pipelines' instance-step buffer"),
        ("P", "pipeline identity (state member)", C, "PIPELINES row key",
         "updatePlayerAgentPipeline_"),
        ("P", "pipeline label", R,
         "no PIPELINES column holds it", '"Update Player Agent (0D, 1 thread)"'),
        ("P", "ROSTER gates around creation blocks", R,
         "no PIPELINES column holds them", "if constexpr (ROSTER.orbs)"),
        ("P", "computeLayoutFor + shared-vs-dedicated list split", R,
         "idiom template / ordering", "renderer.hpp computeLayoutFor"),
        ("P", "layout-list local variable names and rebinding", R,
         "idiom template", "pl rebound per block"),
        ("P", "descriptor-before-array order (%d blocks) and unchecked creates (%d)"
              % (len(pipe_info["desc_before_array"]), len(pipe_info["unchecked_creates"])), R,
         "idiom formatting",
         ", ".join(pipe_info["unchecked_creates"]) or "(none)"),
        ("P", "renderer handle plumbing (field = gpuState.accessor())", C,
         "LAYOUTS.accessor (field name derivable by camel-casing; verified below)",
         "renderEntityLayout_ = gpuState.render_entity_layout();"),
    ]
    return rows


# ═══════════════════════════════════════════════════════════════════════
# PROVENANCE AND THE WRITER
# ═══════════════════════════════════════════════════════════════════════

def provenance():
    """Same law as the binding ledger's: the SHA is the last commit that
    touched any INPUT — not HEAD, which moves when this file lands. The
    content hashes are the authoritative provenance."""
    rel = [os.path.relpath(p, REPO) for p in INPUTS]
    try:
        sha = subprocess.run(["git", "-C", REPO, "log", "-1", "--format=%H", "--"] + rel,
                             capture_output=True, text=True, check=True).stdout.strip()
        subj = subprocess.run(["git", "-C", REPO, "log", "-1", "--format=%s", "--"] + rel,
                              capture_output=True, text=True, check=True).stdout.strip()
    except Exception:
        sha, subj = "(git unavailable)", ""
    return sha, subj, [(r, BL.sha256(p)) for r, p in zip(rel, INPUTS)]


def writer_pins_lf(w):
    """The census's own writer must pin the terminator (the G2-eol law)."""
    src = BL.read(os.path.abspath(__file__))
    m = re.search(r"open\(path,\s*\"w\"[^)]*\)", src)
    call = m.group(0) if m else ""
    ok = 'newline="\\n"' in call and 'encoding="utf-8"' in call
    w.record("ML-2w", ok,
             "artifact writer pins `encoding=\"utf-8\", newline=\"\\n\"`; a byte-level "
             "read-back runs after the write"
             if ok else "artifact writer does not pin the newline: %r" % call)
    return ok


def esc(s):
    return BL.md_escape(s)


def code(s):
    return "`%s`" % str(s).replace("`", "'")


# ═══════════════════════════════════════════════════════════════════════
# THE ARTIFACT
# ═══════════════════════════════════════════════════════════════════════

def emit(path, wb, wm, counts, surfaces, reg_info, wgsl_info, state_info,
         pipe_info, m2, m3, m4rows, m5rows, groups, invokes, varied_slots,
         per_slot, layouts, cen, acc_map, handle_rows, m7):
    sha, subj, hashes = provenance()
    L = []
    A = L.append

    A("# THE MIRROR LEDGER")
    A("")
    A("A read-only census of the four hand-maintained mirrors of the binding")
    A("surface, taken for LOOM_1: what one table must contain to regenerate")
    A("today's tree byte-identically. Generated by `tools/mirror_census.py`;")
    A("do not hand-edit — re-run it.")
    A("")
    A("**The residue list (M4) is the crown.** The five relations cover the")
    A("mirrors' variable data; everything the relations cannot say — prose,")
    A("formatting, ordering, window expressions, creation cadence — is listed")
    A("there, one row per fact, with an example site. LOOM_1's table must")
    A("carry those facts, or leave them in place and patch around them.")
    A("")
    A("## Provenance")
    A("")
    A("| field | value |")
    A("|---|---|")
    A("| source commit | `%s` |" % sha)
    A("| | %s |" % esc(subj))
    for rel, h in hashes:
        A("| `%s` | `sha256:%s` |" % (rel, h))
    A("")
    A("`tools/binding_ledger.py` is an input because its parsers are IMPORTED,")
    A("not copied — one parse, two artifacts, no drift between instruments.")
    A("`audit/BINDING_LEDGER.md` is an input because witness ML-0 reads its")
    A("counts as the reconciliation authority; this census runs against the")
    A("committed ledger, which Step 1 of the campaign verified fresh at HEAD.")
    A("The source commit is the last commit touching any input — not `HEAD`,")
    A("which moves when this file is committed.")
    A("")
    A("The writer pins `encoding=\"utf-8\", newline=\"\\n\"` and a binary")
    A("read-back after every write asserts LF-only, no BOM, exactly one")
    A("trailing newline (witness ML-2); a violation stops the run.")
    A("")
    A("## The coverage target")
    A("")
    A("Five relations. A mirror fact is COVERED if one of these columns")
    A("holds it; everything else is residue.")
    A("")
    A("| relation | key | columns |")
    A("|---|---|---|")
    A("| `SLOTS` | (group, binding) | symbol, aliases, namespace, kind, address space, accesses, store type, runtime-array flag, texture sample/format/dimension |")
    A("| `SEATS` | (layout, entry index) | slot ref, visibility, hasDynamicOffset |")
    A("| `LAYOUTS` | layout | state.hpp member, label, accessor |")
    A("| `GROUPS` | bind group | layout ref, label, per-seat backing resource member |")
    A("| `PIPELINES` | pipeline | ordered layout list, entry points, vertex buffers/attributes |")
    A("")
    A("The relations' row DATA already lives in `audit/BINDING_LEDGER.md`")
    A("(Table A carries the seats; Appendix 1 the slots; Appendix 3 the")
    A("pipelines) and is not duplicated here — one fact, one home. What this")
    A("ledger adds is the GROUPS backing column (M6), the idiom shapes the")
    A("generator must emit (M1), the scatter it must respect (M2), and the")
    A("residue neither ledger's relations can hold (M4).")
    A("")
    A("Census cardinalities, reconciled against the ledger by ML-0:")
    A("")
    A("| relation | rows |")
    A("|---|---|")
    for k in ("slots", "declarations", "seats", "layouts", "groups", "pipelines"):
        A("| %s | %d |" % (k, counts[k]))
    A("")
    A("### Witnesses — LOOM_0")
    A("")
    A("| witness | verdict | detail |")
    A("|---|---|---|")
    for tag, ok, detail in wm.rows:
        A("| `%s` | **%s** | %s |" % (tag, "PASS" if ok else "FAIL", esc(detail)))
    A("")
    A("### Witnesses — inherited from the imported parsers")
    A("")
    A("The binding ledger's own parse witnesses, re-run on this tree by the")
    A("imported code. They are evidence the census's ground truth is the")
    A("ledger's ground truth, not a second opinion.")
    A("")
    A("| witness | verdict | detail |")
    A("|---|---|---|")
    for tag, ok, detail in wb.rows:
        A("| `%s` | **%s** | %s |" % (tag, "PASS" if ok else "FAIL", esc(detail)))
    A("")

    # ─── M1 ──────────────────────────────────────────────────────────
    A("## M1 — the idiom census")
    A("")
    A("Every instance inside each surface's boundary matches exactly one")
    A("idiom (witness ML-1); the exemplar is ONE verbatim instance. The")
    A("idioms' fixed text is the generator's template; the variable parts")
    A("are relation columns; what a shape cannot say is M4 residue.")
    A("")
    surface_blurbs = {
        "R": ("### M1.R — registry constants (`binding_registry.hpp`)",
              "Boundary: every `inline constexpr uint32_t` line. The 4 "
              "static_asserts of the render band and the namespace/banner "
              "prose are M4 residue, not constants."),
        "W": ("### M1.W — WGSL declarations (`world.wgsl`)",
              "Boundary: the 98 module-scope binding declarations (witness "
              "0b-0 proves the boundary is every `@group(` occurrence)."),
        "L": ("### M1.L — layout entry arrays + descriptors (`state.hpp`)",
              "Boundary: every `;`-terminated statement inside the 25 "
              "creation blocks, array declaration through boot check."),
        "G": ("### M1.G — bind group creation sites (`state.hpp`)",
              "Boundary: every `;`-terminated statement inside the 27 "
              "creation blocks, plus the builder machinery between blocks "
              "(surface G+): lambda header, three invocations, compound check."),
        "P": ("### M1.P — pipeline-layout lists (`renderer.hpp`)",
              "Boundary: every occurrence of a pipeline-layout token "
              "(`std::array<wgpu::BindGroupLayout`, `PipelineLayoutDescriptor`, "
              "`PipelineLayout`, `bindGroupLayoutCount`, `bindGroupLayouts`, "
              "`computeLayoutFor`) must fall inside exactly one censused "
              "instance span. Boot checks after creation are shared renderer "
              "idiom, censused as ordering facts, not instances."),
    }
    ooo_L = state_info["ooo_L"]
    ooo_G = state_info["ooo_G"]
    ordering_notes = {
        "R": reg_info["ordering"],
        "W": wgsl_info["ordering"],
        "L": ("entry indices ascend 0..N-1 in %d of %d blocks%s; each index "
              "appears exactly once everywhere. Field order within a seat: "
              "binding, visibility, then type-specific fields. Descriptor "
              "tail: desc, label, entryCount, entries, create, boot check. "
              "One visibility mask reverses operand order%s."
              % (state_info["n_L"] - len(ooo_L), state_info["n_L"],
                 (" (out of order: %s)" % "; ".join(ooo_L)) if ooo_L else "",
                 (" (state.hpp:%s)" % ", ".join(str(x) for x in state_info["vis_reversed"]))
                 if state_info["vis_reversed"] else " — none found")),
        "G": ("entry indices ascend in %d of %d blocks%s. Field order within "
              "a seat: binding, one backing field, then optional offset, "
              "then size; every buffer seat carries a size, no view/sampler "
              "seat does. The builder block's machinery (surface G+) sits "
              "between blocks: header, three invocations, compound check."
              % (state_info["n_G"] - len(ooo_G), state_info["n_G"],
                 (" (out of order: %s)" % "; ".join(ooo_G)) if ooo_G else "")),
        "P": ("array, descriptor, count, layouts-pointer, create, check — "
              "except %d block(s) declare the descriptor before the array%s, "
              "and %d create(s) carry no boot check (%s). Count always "
              "precedes the data pointer. Shared lists are built before any "
              "pipeline; dedicated lists sit inline beside their pipelines."
              % (len(pipe_info["desc_before_array"]),
                 (" (renderer.hpp:%s)" % ", ".join(str(x) for x in pipe_info["desc_before_array"]))
                 if pipe_info["desc_before_array"] else "",
                 len(pipe_info["unchecked_creates"]),
                 ", ".join(pipe_info["unchecked_creates"]) or "none")),
    }
    by_key = {s.key: s for s in surfaces}
    for key in ("R", "W", "L", "G", "P"):
        head, blurb = surface_blurbs[key]
        A(head)
        A("")
        A(blurb)
        A("")
        A("| idiom | what it is | instances | exemplar (verbatim, site is a line hint) |")
        A("|---|---|---|---|")
        emit_surfaces = [by_key[key]] + ([by_key["G+"]] if key == "G" else [])
        total = 0
        idioms = 0
        for s in emit_surfaces:
            total += s.total
            for iid, desc, count, ex, line in s.rows():
                if count:
                    idioms += 1
                where = (" — %s:%d" % ({"R": "binding_registry.hpp",
                                        "W": "world.wgsl", "L": "state.hpp",
                                        "G": "state.hpp", "P": "renderer.hpp"}[key],
                                       line)) if line else ""
                A("| `%s` | %s | %d | %s |"
                  % (iid, esc(desc), count,
                     (code(esc(ex)) + esc(where)) if count else "—"))
        A("")
        A("Instances: %d over %d idioms. Ordering observed: %s"
          % (total, idioms, ordering_notes[key]))
        A("")

    # ─── M2 ──────────────────────────────────────────────────────────
    A("## M2 — the WGSL scatter map")
    A("")
    A("The %d declarations as contiguous runs (two declarations share a run"
      % sum(len(r) for r in m2["runs"]))
    A("iff every line between them is blank or comment). `attached` uses the")
    A("nearest-preceding rule (Table H's A) restricted to ADJACENCY: the")
    A("nearest preceding comment block counts iff nothing but whitespace")
    A("stands between its end and the declaration — rule A verbatim covers")
    A("every site that has any earlier comment and would make this column a")
    A("constant; the adjacent block is the prose that must TRAVEL with the")
    A("declaration if the generator moves it. `defended` is membership in")
    A("the ledger's Table H (`wgsl binding` rows). Facts only, no verdict on")
    A("emit-one-block vs patch-in-place — that call is LOOM_1's.")
    A("")
    A("| run | lines | decls | first symbol | last symbol |")
    A("|---|---|---|---|---|")
    for i, run in enumerate(m2["runs"], 1):
        A("| %d | %d–%d | %d | `%s` | `%s` |"
          % (i, run[0].line, run[-1].line, len(run),
             run[0].symbol, run[-1].symbol))
    A("")
    A("Line numbers are non-authoritative hints; cite symbols.")
    A("")
    A("| wgsl symbol | line (hint) | run | attached comment | Table H defended |")
    A("|---|---|---|---|---|")
    run_of = {}
    for i, run in enumerate(m2["runs"], 1):
        for d in run:
            run_of[d.symbol] = i
    for run in m2["runs"]:
        for d in run:
            A("| `%s` | %d | %d | %s | %s |"
              % (d.symbol, d.line, run_of[d.symbol],
                 "yes" if m2["attach"][d.symbol] else "—",
                 "**yes**" if d.symbol in m2["defended"] else "—"))
    A("")

    # ─── M3 ──────────────────────────────────────────────────────────
    A("## M3 — the fifth-home grep")
    A("")
    A("Count law: each sweep states its boundary. A hit that fits these row")
    A("shapes is recorded; one that does not is a STOP (witness M3-0).")
    A("")
    A("### (a) `@binding(` outside the declarations")
    A("")
    A("Boundary: the comment-stripped code text of every `*.wgsl` under")
    A("`src/` (%s); declaration spans are the parser's %d matches in"
      % (", ".join("`%s`" % f for f in m3["wgsl_files"]), counts["declarations"]))
    A("world.wgsl. Comment mentions are counted, not sites.")
    A("")
    A("| file | raw `@binding(` | in code | declarations | outside code sites |")
    A("|---|---|---|---|---|")
    comment_mentions = 0
    for rel, (rawn, coden, decln, outn) in sorted(m3["a_counts"].items()):
        A("| `%s` | %d | %d | %d | **%d** |" % (rel, rawn, coden, decln, outn))
        comment_mentions += rawn - coden
    A("")
    A("The count law: raw minus code = %d mention(s) inside comments —"
      % comment_mentions)
    A("prose citing binding numbers, not sites. Code sites outside the")
    A("declarations: %d (the handoff expected 0)."
      % sum(outn for _, _, _, outn in m3["a_counts"].values()))
    for f in m3["findings"]["a"]:
        A("- `%s`" % esc(f))
    A("")
    A("### (b) `.binding =` with a non-registry right side")
    A("")
    A("Boundary: the comment-stripped text of all %d `*.hpp` under `src/`"
      % m3["hpp_file_count"])
    A("(the handoff's sweep), extended to the %d `.cpp`/`.h`/`.cc` files"
      % m3["ext_file_count"])
    A("there so a seat assignment in a translation unit cannot hide on the")
    A("wrong extension (%d hit(s) on the extension). state.hpp is read"
      % m3["b_counts"]["ext"])
    A("with its generated include expanded in place. Pattern")
    A("`\\.binding\\s*=`. RHS classes: `bind::` constant %d, integer"
      % m3["b_counts"]["bind"])
    A("literal %d, other %d."
      % (m3["b_counts"]["literal"], m3["b_counts"]["other"]))
    A("")
    if m3["findings"]["b_literal"] or m3["findings"]["b_other"]:
        for f in m3["findings"]["b_literal"] + m3["findings"]["b_other"]:
            A("- `%s`" % esc(f))
    else:
        A("No hit: every `.binding =` right side inside this boundary is a")
        A("`bind::` constant.")
    A("")
    A("### (c) `bind::` outside the homes")
    A("")
    A("Boundary: every file under `src/` (comment-stripped for C++/WGSL, raw")
    A("otherwise), token `\\bbind::`. Instruments and reports cite the")
    A("constants rather than bind them and are counted, not listed:")
    A("`tools/` %d mention(s), `audit/` %d mention(s)."
      % (m3["cite_counts"].get("tools", 0), m3["cite_counts"].get("audit", 0)))
    A("")
    A("| file | `bind::` hits |")
    A("|---|---|")
    for rel, n in sorted(m3["c_counts"].items()):
        A("| `%s` | %d |" % (rel, n))
    A("")
    if m3["findings"]["c"]:
        A("Hits outside the homes:")
        A("")
        for f in m3["findings"]["c"]:
            A("- `%s`" % esc(f))
    else:
        A("No code hit outside `binding_registry.hpp` / `state.hpp` (with its")
        A("generated half, `binding_surface.gen.inc`) / `renderer.hpp`: the")
        A("binding surface has exactly the homes the campaign names (the")
        A("registry, the shader, and the two consumers).")
    A("")

    # ─── M4 ──────────────────────────────────────────────────────────
    A("## M4 — the coverage verdict")
    A("")
    A("One row per emitted fact per mirror: the relation+column that covers")
    A("it, or **RESIDUE**. Example sites are computed from the tree at the")
    A("provenance commit.")
    A("")
    for key in ("R", "W", "L", "G", "P"):
        A("### %s" % MIRROR_NAMES[key])
        A("")
        A("| fact | coverage | example site |")
        A("|---|---|---|")
        for mk, fact, kind, cov, ex in m4rows:
            if mk != key:
                continue
            cell = ("**RESIDUE** — %s" % esc(cov)) if kind == "RESIDUE" else esc(cov)
            A("| %s | %s | %s |" % (esc(fact), cell, code(esc(ex))))
        A("")
    A("### The residue list — the crown")
    A("")
    A("Everything above marked RESIDUE, in one place: the exact bill of what")
    A("LOOM_1's table must carry beyond the five relations, or leave in")
    A("place and patch around.")
    A("")
    A("| # | mirror | fact | example site |")
    A("|---|---|---|---|")
    i = 0
    for mk, fact, kind, cov, ex in m4rows:
        if kind != "RESIDUE":
            continue
        i += 1
        A("| %d | %s | %s | %s |" % (i, mk, esc(fact), code(esc(ex))))
    A("")
    if varied_slots:
        A("The aliased slots whose seats disagree on access — the one place a")
        A("SEATS row cannot be reconstructed from its slot:")
        A("")
        for k in sorted(varied_slots):
            variants = varied_slots[k]
            A("- `%s`: %s" % (k, "; ".join(
                "%s/%s at %s" % (kind, acc, ", ".join(sorted(sites)))
                for (kind, acc), sites in sorted(variants.items()))))
        A("")

    # ─── M5 ──────────────────────────────────────────────────────────
    A("## M5 — the struct pair inventory (counted, not reconciled)")
    A("")
    A("Every struct type named by a slot store type; its WGSL definition")
    A("site; the candidate C++ twin (searched: `state.hpp`, `contracts/`,")
    A("names `X` and `GPUX`); every static_assert naming it. Unresolved")
    A("twins are findings, not STOPs. No layout reconciliation here — that")
    A("is LOOM_1 / panel work.")
    A("")
    A("| wgsl struct | def (line hint) | named by slots | C++ twin | twin site | static_asserts |")
    A("|---|---|---|---|---|---|")
    for r in m5rows:
        twin_cell = ("`%s`" % r["twin"]) if r["twin"] else (
            "**none found** under the prescribed names" +
            ("; name cited at %s" % ", ".join("`%s`" % x for x in r["cited"])
             if r["cited"] else ""))
        A("| `%s` | %s | %s | %s | %s | %d |"
          % (r["name"], r["wgsl_line"] or "—",
             esc(", ".join("`%s`" % x for x in r["slots"])),
             twin_cell,
             ("`%s`" % r["twin_site"]) if r["twin_site"] else "—",
             len(r["asserts"])))
    A("")
    asserted = [r for r in m5rows if r["asserts"]]
    if asserted:
        A("The static_asserts, cited verbatim:")
        A("")
        for r in asserted:
            for a_ in r["asserts"]:
                A("- `%s`: %s" % (r["name"], code(esc(a_))))
        A("")

    # ─── M6 ──────────────────────────────────────────────────────────
    A("## M6 — the backing map")
    A("")
    A("For each bind group creation block in `state.hpp`, per seat: the")
    A("member expression bound. This IS the GROUPS relation's backing")
    A("column; the `.offset` / `.size` window expressions ride along here")
    A("because no relation column holds them (M4 residue).")
    A("")
    for g in groups:
        A("### %s" % (("`%s`" % g["label"]) if not g["param_label"]
                      else "the builder — label is a parameter (state.hpp:%d)" % g["line"]))
        A("")
        A("layout `%s` → member `%s` (state.hpp:%d, descriptor `%s`)"
          % (g["layout_member"], g["member"] or "returned by builder",
             g["line"], g["desc_var"]))
        A("")
        A("| entry | slot | backing | member expression | window |")
        A("|---|---|---|---|---|")
        for e in g["entries"]:
            window = "; ".join(filter(None, [
                ("offset = %s" % e["offset"]) if "offset" in e else None,
                ("size = %s" % e["size"]) if "size" in e else None])) or "—"
            A("| %d | `%s` | %s | `%s` | %s |"
              % (e["index"], e["binding"], e["kind"], esc(e["backing"]),
                 esc(window)))
        A("")
    A("### The builder's three invocations")
    A("")
    A("| member | label | IB window offset | IB window size |")
    A("|---|---|---|---|")
    for iv in invokes:
        A("| `%s` | %s | `%s` | `%s` |"
          % (iv["member"], code(esc(iv["label"])), iv["offset_arg"], iv["size_arg"]))
    A("")

    # ─── M7 ──────────────────────────────────────────────────────────
    A("## M7 — the usage census (LOOM_2 U0)")
    A("")
    A("Where the groups are BOUND: every `SetBindGroup` call under `src/`,")
    A("resolved to the state member it binds — through gpuState accessors,")
    A("locals, and helper parameters (one caller hop). Witness M7-0 proves")
    A("each site binds its group at exactly the index 0c-4 records for the")
    A("group's layout. Dynamic-offset arguments ride verbatim. The recut's")
    A("per-site rewrite list is exactly these rows.")
    A("")
    A("Boundary: every `*.hpp` / `*.cpp` under `src/` naming `SetBindGroup`")
    A("or `GetBindGroupLayout` (%s). `GetBindGroupLayout` uses: %d."
      % (", ".join("`%s`" % f for f in m7["files"]), m7["get_layout_uses"]))
    A("Pipeline-layout LIST sites are M1.P's census (11 arrays, the shared")
    A("wrapper, 18 wrapper calls) and are not recounted here.")
    A("")
    A("| site (line hint) | enclosing function | idx | group member(s) | dynamic offsets |")
    A("|---|---|---|---|---|")
    for r in m7["rows"]:
        A("| `%s:%d` | `%s` | %d | %s | %s |"
          % (r["file"].rsplit("/", 1)[-1], r["line"], r["fn"], r["index"],
             ", ".join("`%s`" % x for x in r["members"]) or "**unresolved**",
             code(esc(r["offsets"])) if r["offsets"] else "—"))
    A("")

    # ─── the handle convention ───────────────────────────────────────
    A("## Appendix — the renderer handle convention")
    A("")
    A("The renderer's private layout handles resolve through `gpuState`")
    A("accessors (LAYOUTS.accessor). The field name is the accessor's name")
    A("camel-cased with a trailing underscore — verified per handle:")
    A("")
    A("| renderer field | accessor | state.hpp member | convention |")
    A("|---|---|---|---|")
    for field, accname, member, ok in handle_rows:
        A("| `%s` | `%s()` | `%s` | %s |"
          % (field, accname, member, "holds" if ok else "**BREAKS**"))
    A("")

    text = "\n".join(L)
    if not text.endswith("\n"):
        text += "\n"
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    return text


# ═══════════════════════════════════════════════════════════════════════

def main():
    ap = argparse.ArgumentParser(description="LOOM_0 — the mirror census.")
    ap.add_argument("--check", action="store_true",
                    help="run the witnesses, write nothing")
    ap.add_argument("-o", "--out", default=DEFAULT_OUT)
    args = ap.parse_args()

    wb = BL.Witnesses()            # inherited parse witnesses
    wm = BL.Witnesses()            # LOOM_0 witnesses

    print("LOOM_0 — the mirror census")
    print("")
    print("PHASE 1 — GROUND TRUTH (imported parsers)")
    registry = BL.parse_registry(wb)
    layouts = BL.parse_layouts(wb, registry)
    b = BL.phase_0b(wb)
    c = BL.phase_0c(wb, layouts, b)
    print("  %d registry constants, %d layouts / %d seats, %d declarations / "
          "%d slots, %d pipelines"
          % (len(registry), len(layouts),
             sum(len(x["entries"]) for x in layouts),
             len(b["decls"]), len(b["slots"]), len(c["pipelines"])))

    groups, invokes = parse_groups(wm)
    print("  %d bind group creation blocks, %d builder invocations"
          % (len(groups), len(invokes)))

    ledger = parse_ledger(wm)
    counts = ml0(wm, ledger, layouts, b, c, groups)

    print("")
    print("PHASE 2 — THE IDIOM CENSUS (M1)")
    sR, reg_info = census_registry(wm)
    sW, wgsl_info = census_wgsl(wm, b)
    sL, sG, sGx, state_info = census_state_blocks(wm)
    sP, pipe_info = census_renderer(wm)
    surfaces = [sR, sW, sL, sG, sGx, sP]
    ml1(wm, surfaces)
    for s in surfaces:
        used = [(iid, n) for iid, n in s.counts.items() if n]
        print("  %-3s %4d instances over %2d idioms" % (s.key, s.total, len(used)))
    if any(s.stops for s in surfaces):
        for s in surfaces:
            for x in s.stops:
                print("  STOP %s" % x)

    print("")
    print("PHASE 3 — SCATTER, FIFTH HOMES, STRUCT PAIRS (M2/M3/M5)")
    m2 = scatter_map(wm, b, ledger)
    print("  M2: %d runs over %d declarations" % (len(m2["runs"]),
                                                  counts["declarations"]))
    m3 = fifth_home(wm, b, args.out)
    print("  M3: (a) %d outside, (b) %d literal / %d other, (c) %d outside homes"
          % (len(m3["findings"]["a"]), len(m3["findings"]["b_literal"]),
             len(m3["findings"]["b_other"]), len(m3["findings"]["c"])))
    m5rows = struct_pairs(wm, b)
    print("  M5: %d struct pairs, %d without a twin"
          % (len(m5rows), sum(1 for r in m5rows if not r["twin"])))
    m7 = usage_census(wm, c, groups, invokes, layouts)
    print("  M7: %d SetBindGroup sites over %d files, %d GetBindGroupLayout uses"
          % (len(m7["rows"]), len(m7["files"]), m7["get_layout_uses"]))

    per_slot, varied = slot_consistency(layouts)
    acc_map = accessor_map()

    # The handle convention: renderer field name == camelCase(accessor)+'_'.
    handle_rows = []
    member_to_acc = {m_: a[0] for m_, a in acc_map.items() if len(a) == 1}
    for field, member in sorted(c["handles"].items()):
        accname = member_to_acc.get(member, "?")
        camel = re.sub(r"_(\w)", lambda m_: m_.group(1).upper(), accname) + "_"
        handle_rows.append((field, accname, member, field == camel))
    broken = [f for f, a, m_, ok in handle_rows if not ok]
    wm.record("M4-h", not broken,
              "renderer handle field names derive from LAYOUTS.accessor by "
              "camel-casing, all %d handles" % len(handle_rows)
              if not broken else
              "handle naming convention breaks at: " + ", ".join(broken))

    m4rows = coverage(reg_info, wgsl_info, state_info, pipe_info, groups,
                      invokes, varied, m2, sL)
    # Tag rows with their kind for emit.
    m4rows = [(mk, fact, "RESIDUE" if cov == "RESIDUE" else "covered",
               where, ex) for (mk, fact, cov, where, ex) in m4rows]

    writer_pins_lf(wm)

    print("")
    print("WITNESSES — inherited")
    wb.report()
    print("WITNESSES — LOOM_0")
    wm.report()

    fails = wb.failures() + wm.failures()
    if fails:
        print("")
        print("STOP — %d witness(es) failed. Nothing written. The census is not "
              "to be trusted until Jean rules which side is wrong." % len(fails))
        return 1

    if args.check:
        print("")
        # GATES_2b — every pin this census writes, including the two only it
        # carries: tools/binding_ledger.py (the instrument, imported not
        # copied) and audit/BINDING_LEDGER.md (ML-0's reconciliation
        # authority). Both were unchecked while the list was hardcoded.
        if BL.report_stale(args.out):
            return 1
        print("--check: all witnesses pass, the ledger's pins match the live "
              "inputs, nothing written.")
        return 0

    text = emit(args.out, wb, wm, counts, surfaces, reg_info, wgsl_info,
                state_info, pipe_info, m2, m3, m4rows, m5rows, groups,
                invokes, varied, per_slot, layouts, c, acc_map, handle_rows,
                m7)
    print("")
    print("PHASE 4 — THE ARTIFACT")
    print("  wrote %s (%d lines, %d bytes)"
          % (os.path.relpath(args.out, REPO), text.count("\n"),
             len(text.encode("utf-8"))))
    bx = BL.verify_bytes(args.out)
    clean = bx["crlf"] == 0 and bx["cr"] == 0 and not bx["bom"] and bx["trailing"]
    wm.record("ML-2", clean,
              "byte read-back: %d CRLF, %d bare CR, %d LF, BOM %s, single "
              "trailing LF %s" % (bx["crlf"], bx["cr"], bx["lf"], bx["bom"],
                                  bx["trailing"]))
    print("  byte check: %d CRLF, %d bare CR, %d LF, BOM %s, single trailing "
          "LF %s -> %s" % (bx["crlf"], bx["cr"], bx["lf"], bx["bom"],
                           bx["trailing"], "LF-CLEAN" if clean else "FAIL"))
    if not clean:
        print("")
        print("STOP — the artifact violates the LF-only encoding law (L1).")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
