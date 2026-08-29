#!/usr/bin/env python3
# ═══════════════════════════════════════════════════════════════════════
# THE BINDING LEDGER (BUDGET_0) — a read-only census of the program's
# binding surface, and the instrument that reproduces it.
#
# WHAT THIS IS. WebGPU charges a program in SLOTS, per shader stage,
# summed across a whole pipeline layout. Bytes and slots are unrelated
# currencies; bind GROUPS do not partition the budget (all the bind group
# layouts of a pipeline layout are concatenated, then checked); and a slot
# is charged once per stage named in `visibility`, whether or not that
# stage can reach the binding. This tool counts what the API charges.
#
# WHAT THIS IS NOT. It touches no translation unit, no shader, and no
# build graph. It reads four files and writes one markdown artifact.
# Precedent: tools/web_dist.py.
#
# THE UNIT OF THE LEDGER is one row per (bind group layout, entry index)
# — NOT per buffer. The registry is one constant per SITE: the same
# buffer wears several names because each name is one (group, slot).
# A buffer-keyed census would merge rows the API charges separately.
#
# USAGE
#   python3 tools/binding_ledger.py              # witnesses to stdout, write artifact
#   python3 tools/binding_ledger.py --check      # witnesses only, write nothing
#   python3 tools/binding_ledger.py -o PATH      # write elsewhere
#
# Exit status is 1 if any witness fails or the reconciliation gate fails.
# A failing witness means the parser or the program disagrees with a
# stated fact; either way the ledger is not to be trusted until ruled on.
# ═══════════════════════════════════════════════════════════════════════

import argparse
import hashlib
import os
import re
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REAL = os.path.join(REPO, "src", "cartridges", "the_board", "realization")

STATE_HPP = os.path.join(REAL, "state.hpp")
REGISTRY_HPP = os.path.join(REAL, "binding_registry.hpp")
RENDERER_HPP = os.path.join(REAL, "renderer.hpp")
WORLD_WGSL = os.path.join(REAL, "world.wgsl")

DEFAULT_OUT = os.path.join(REPO, "audit", "BINDING_LEDGER.md")


def _load_schema():
    """binding_schema.py — the one home for facts about the binding surface.

    LOOM_3: witnesses read their control sets from the schema instead of
    carrying hand-written copies, so a rename in the subject never requires
    an edit in the witness. Loaded by path, not by package import, for the
    same reason binding_gen.py does it: the tools directory is not a package
    and the schema is DATA ONLY — no logic, no imports, nothing to cycle on.
    """
    import importlib.util
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                        "binding_schema.py")
    if not os.path.exists(path):
        sys.exit("binding_ledger: tools/binding_schema.py is missing — the "
                 "witnesses read their control sets from it (LOOM_3)")
    spec = importlib.util.spec_from_file_location("binding_schema", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


schema = _load_schema()

# ─── The Core defaults (WebGPU §3.6.2 "limits"). The guaranteed floor on
#     every machine that hands you an adapter, and therefore the web
#     twin's real ceiling. ────────────────────────────────────────────
CORE = {
    "uniform": 12,          # maxUniformBuffersPerShaderStage
    "storage": 8,           # maxStorageBuffersPerShaderStage
    "sampled": 16,          # maxSampledTexturesPerShaderStage
    "samplers": 16,         # maxSamplersPerShaderStage
    "storagetex": 4,        # maxStorageTexturesPerShaderStage
}
CORE_BIND_GROUPS = 4                      # maxBindGroups
CORE_GROUPS_PLUS_VBS = 24                 # maxBindGroupsPlusVertexBuffers
CORE_DYN_UNIFORM = 8                      # maxDynamicUniformBuffersPerPipelineLayout
CORE_DYN_STORAGE = 4                      # maxDynamicStorageBuffersPerPipelineLayout
UNIFORM_BINDING_MAX_BYTES = 65536         # maxUniformBufferBindingSize
UNIFORM_OFFSET_ALIGN = 256                # minUniformBufferOffsetAlignment

STAGES = ("V", "F", "C")


# ═══════════════════════════════════════════════════════════════════════
# WITNESSES
# ═══════════════════════════════════════════════════════════════════════

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


# ═══════════════════════════════════════════════════════════════════════
# C++ SOURCE HANDLING
# ═══════════════════════════════════════════════════════════════════════

GEN_INC = os.path.join(REAL, "binding_surface.gen.inc")
_GEN_INC_LINE = re.compile(r'[ \t]*#include "binding_surface\.gen\.inc"\n')


def read_raw(path):
    """One file, as it sits on disk. The per-file sweeps (and the
    provenance hashes) want this; every parser wants read() below."""
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def read(path):
    """LOOM_1 U3: state.hpp's binding-surface creation blocks live in
    binding_surface.gen.inc, generated from tools/binding_schema.py and
    included at class scope. The census reads the include EXPANDED in
    place, so every parser, witness, and defended-site rule sees one
    text under the state.hpp path — the W4-2 controls stay keyed to the
    file they have always named."""
    text = read_raw(path)
    m = _GEN_INC_LINE.search(text)
    if m:
        text = text[:m.start()] + read_raw(GEN_INC) + text[m.end():]
    return text


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def strip_cpp_comments(src):
    """Blank out // and /* */ comments, preserving byte offsets and lines.

    Offsets are preserved so that every span computed on the stripped text
    indexes the original text identically — the parser can quote source
    without a second coordinate system. String and character literals are
    left intact: `desc.label` is a string literal and is ledger data.
    """
    out = list(src)
    i, n = 0, len(src)
    while i < n:
        c = src[i]
        if c == '"' or c == "'":
            quote = c
            i += 1
            while i < n:
                if src[i] == "\\":
                    i += 2
                    continue
                if src[i] == quote:
                    i += 1
                    break
                i += 1
            continue
        if c == "/" and i + 1 < n and src[i + 1] == "/":
            while i < n and src[i] != "\n":
                out[i] = " "
                i += 1
            continue
        if c == "/" and i + 1 < n and src[i + 1] == "*":
            while i < n and not (src[i] == "*" and i + 1 < n and src[i + 1] == "/"):
                if src[i] != "\n":
                    out[i] = " "
                i += 1
            if i < n:
                out[i] = " "
                out[i + 1] = " "
                i += 2
            continue
        i += 1
    return "".join(out)


def line_of(src, pos):
    return src.count("\n", 0, pos) + 1


def gate_map(src):
    """Offset -> list of enclosing `if constexpr (...)` conditions.

    Returns a list of (start, end, condition) spans, innermost last when
    sorted by start. The program's compile-time gates are all written as
    `if constexpr (COND) {` with the brace on the same line, so a brace
    walk resolves them exactly; a gate whose body is a single unbraced
    statement guards nothing this census reads (they are early `return`s
    in dispatch helpers, not creation sites).
    """
    spans = []
    for m in re.finditer(r"if\s+constexpr\s*\(", src):
        j = m.end() - 1
        depth, k = 0, j
        while k < len(src):
            if src[k] == "(":
                depth += 1
            elif src[k] == ")":
                depth -= 1
                if depth == 0:
                    break
            k += 1
        cond = src[j + 1:k].strip()
        rest = src[k + 1:k + 200]
        brace = re.match(r"\s*\{", rest)
        if not brace:
            continue                       # unbraced gate: guards one statement
        body = k + 1 + brace.end() - 1
        depth, p = 0, body
        while p < len(src):
            if src[p] == "{":
                depth += 1
            elif src[p] == "}":
                depth -= 1
                if depth == 0:
                    break
            p += 1
        spans.append((m.start(), p, cond))
    return spans


def gates_at(spans, pos):
    return [c for (s, e, c) in spans if s <= pos <= e]


def roster_gate_of(spans, pos):
    """The ROSTER condition(s) enclosing an offset, as one string."""
    hits = [c for c in gates_at(spans, pos) if "ROSTER" in c]
    return " && ".join(hits) if hits else ""


# ═══════════════════════════════════════════════════════════════════════
# PHASE 0a — THE LAYOUT CENSUS
# ═══════════════════════════════════════════════════════════════════════

VIS_ABBREV = {"Vertex": "V", "Fragment": "F", "Compute": "C"}


def vis_string(mask_tokens):
    """A ShaderStage mask as a canonical V/F/C string, in pipeline order."""
    s = {VIS_ABBREV[t] for t in mask_tokens if t in VIS_ABBREV}
    return "".join(a for a in STAGES if a in s)


def parse_registry(w):
    """binding_registry.hpp -> {('g0','signal'): 0, ...} plus the reverse map."""
    src = strip_cpp_comments(read(REGISTRY_HPP))
    consts = {}
    ns = None
    depth_at_ns = None
    depth = 0
    for line in src.splitlines():
        m = re.search(r"namespace\s+(g\d)\s*\{", line)
        if m:
            ns = m.group(1)
            depth_at_ns = depth
        depth += line.count("{") - line.count("}")
        if ns is not None and depth <= depth_at_ns:
            ns = None
        c = re.search(r"inline\s+constexpr\s+uint32_t\s+(\w+)\s*=\s*(\d+)\s*;", line)
        if c and ns:
            consts[(ns, c.group(1))] = int(c.group(2))
    w.record("registry", len(consts) > 0,
             "binding_registry.hpp: %d constants over %d namespaces (%s)"
             % (len(consts), len({k[0] for k in consts}),
                ", ".join(sorted({k[0] for k in consts}))))
    return consts


class LayoutEntry:
    __slots__ = ("layout_label", "layout_member", "entry_index", "binding_const",
                 "binding_number", "registry_ns", "kind", "access", "vis_declared",
                 "vis_raw", "has_dynamic_offset", "roster_gated", "src_line")

    def __init__(self, **kw):
        for k in self.__slots__:
            setattr(self, k, kw.get(k))


def parse_layouts(w, registry):
    """state.hpp -> one LayoutEntry per (bind group layout, entry index).

    Each creation block is delimited by its `std::array<...BindGroupLayoutEntry,
    N>` declaration and the `CreateBindGroupLayout` that consumes it, so the
    parse cannot drift into the bind-GROUP blocks that follow (those declare
    `BindGroupEntry`, a different type).
    """
    raw = read(STATE_HPP)
    src = strip_cpp_comments(raw)
    spans = gate_map(src)

    layouts = []          # (label, member, declared_n, [LayoutEntry], roster_gate)
    for m in re.finditer(
            r"std::array<\s*wgpu::BindGroupLayoutEntry\s*,\s*(\d+)\s*>\s*(\w+)\s*\{\s*\}\s*;", src):
        declared_n = int(m.group(1))
        arr = m.group(2)
        tail = src.find("CreateBindGroupLayout", m.end())
        if tail < 0:
            raise SystemExit("binding_ledger: unterminated layout block at line %d"
                             % line_of(src, m.start()))
        end = src.find(";", tail)
        block = src[m.end():end]
        base = m.end()

        label = None
        lm = re.search(r'desc\.label\s*=\s*"([^"]*)"\s*;', block)
        if lm:
            label = lm.group(1)
        mem = re.search(r"(\w+)\s*=\s*device_\.CreateBindGroupLayout", block)
        member = mem.group(1) if mem else None

        # entryCount must be the array's own size, not an independent literal.
        ec = re.search(r"desc\.entryCount\s*=\s*(.+?)\s*;", block)
        ec_expr = ec.group(1).strip() if ec else "<absent>"

        rows = {}

        def slot(i):
            return rows.setdefault(i, {"vis": [], "dyn": False})

        pat = re.escape(arr)
        for e in re.finditer(pat + r"\[(\d+)\]\.binding\s*=\s*bind::(g\d)::(\w+)\s*;", block):
            r = slot(int(e.group(1)))
            r["ns"], r["const"] = e.group(2), e.group(3)
            r["line"] = line_of(src, base + e.start())
        for e in re.finditer(pat + r"\[(\d+)\]\.visibility\s*=\s*([^;]+);", block):
            r = slot(int(e.group(1)))
            r["vis"] = re.findall(r"wgpu::ShaderStage::(\w+)", e.group(2))
            r["vis_raw"] = e.group(2).strip()
        for e in re.finditer(pat + r"\[(\d+)\]\.buffer\.type\s*=\s*wgpu::BufferBindingType::(\w+)\s*;", block):
            r = slot(int(e.group(1)))
            r["kind"], r["access"] = "buffer", e.group(2)
        for e in re.finditer(pat + r"\[(\d+)\]\.buffer\.hasDynamicOffset\s*=\s*(true|false)\s*;", block):
            slot(int(e.group(1)))["dyn"] = (e.group(2) == "true")
        for e in re.finditer(pat + r"\[(\d+)\]\.sampler\.type\s*=\s*wgpu::SamplerBindingType::(\w+)\s*;", block):
            r = slot(int(e.group(1)))
            r["kind"], r["access"] = "sampler", e.group(2)
        for e in re.finditer(pat + r"\[(\d+)\]\.texture\.sampleType\s*=\s*wgpu::TextureSampleType::(\w+)\s*;", block):
            r = slot(int(e.group(1)))
            r["kind"] = "texture"
            r["sample"] = e.group(2)
        for e in re.finditer(pat + r"\[(\d+)\]\.texture\.viewDimension\s*=\s*wgpu::TextureViewDimension::(\w+)\s*;", block):
            slot(int(e.group(1)))["viewdim"] = e.group(2)
        for e in re.finditer(pat + r"\[(\d+)\]\.storageTexture\.access\s*=\s*wgpu::StorageTextureAccess::(\w+)\s*;", block):
            r = slot(int(e.group(1)))
            r["kind"] = "storageTexture"
            r["staccess"] = e.group(2)
        for e in re.finditer(pat + r"\[(\d+)\]\.storageTexture\.format\s*=\s*wgpu::TextureFormat::(\w+)\s*;", block):
            slot(int(e.group(1)))["stformat"] = e.group(2)
        for e in re.finditer(pat + r"\[(\d+)\]\.storageTexture\.viewDimension\s*=\s*wgpu::TextureViewDimension::(\w+)\s*;", block):
            slot(int(e.group(1)))["stviewdim"] = e.group(2)

        entries = []
        for i in sorted(rows):
            r = rows[i]
            kind = r.get("kind")
            if kind == "texture":
                access = "%s/%s" % (r.get("sample", "?"), r.get("viewdim", "?"))
            elif kind == "storageTexture":
                access = "%s/%s/%s" % (r.get("staccess", "?"), r.get("stformat", "?"),
                                       r.get("stviewdim", "?"))
            else:
                access = r.get("access", "?")
            entries.append(LayoutEntry(
                layout_label=label,
                layout_member=member,
                entry_index=i,
                binding_const="bind::%s::%s" % (r.get("ns"), r.get("const")),
                binding_number=registry.get((r.get("ns"), r.get("const"))),
                registry_ns=r.get("ns"),
                kind=kind,
                access=access,
                vis_declared=vis_string(r["vis"]),
                vis_raw=r.get("vis_raw", ""),
                has_dynamic_offset=r["dyn"],
                roster_gated=roster_gate_of(spans, m.start()),
                src_line=r.get("line"),
            ))
        layouts.append({
            "label": label, "member": member, "declared_n": declared_n,
            "entries": entries, "entry_count_expr": ec_expr,
            "roster_gated": roster_gate_of(spans, m.start()),
            "line": line_of(src, m.start()),
        })

    # ─── WITNESS 0a-1 — the array's declared size IS the row count.
    #     A partially populated `entries` array is either a defect or a
    #     parser failure and the census cannot tell which.
    bad = [(L["label"], L["declared_n"], len(L["entries"])) for L in layouts
           if L["declared_n"] != len(L["entries"])]
    w.record("0a-1", not bad,
             "%d layouts, every row count == std::array<…, N>"
             % len(layouts) if not bad else
             "row-count mismatch: " + "; ".join("%s declared %d, parsed %d" % b for b in bad))

    # entryCount must be derived from the array, never an independent literal.
    lit = [(L["label"], L["entry_count_expr"]) for L in layouts
           if not L["entry_count_expr"].endswith(".size()")]
    w.record("0a-1b", not lit,
             "every desc.entryCount is <array>.size()" if not lit else
             "literal entryCount: " + "; ".join("%s = %s" % x for x in lit))

    # ─── WITNESS 0a-2 — every binding_const resolves in the registry.
    unresolved = ["%s (%s entry %d)" % (e.binding_const, e.layout_label, e.entry_index)
                  for L in layouts for e in L["entries"] if e.binding_number is None]
    total_rows = sum(len(L["entries"]) for L in layouts)
    w.record("0a-2", not unresolved,
             "%d rows, every bind:: symbol resolves in binding_registry.hpp" % total_rows
             if not unresolved else "unresolvable: " + ", ".join(unresolved))

    # ─── WITNESS 0a-3 — binding numbers are unique within one layout.
    dups = []
    for L in layouts:
        seen = {}
        for e in L["entries"]:
            if e.binding_number in seen:
                dups.append("%s: binding %s at entries[%d] and entries[%d]"
                            % (L["label"], e.binding_number, seen[e.binding_number],
                               e.entry_index))
            seen[e.binding_number] = e.entry_index
    w.record("0a-3", not dups,
             "no duplicate binding number inside any layout" if not dups
             else "; ".join(dups))

    # Every row must name a kind — an entry with none is an unparsed construct.
    kindless = ["%s entries[%d]" % (e.layout_label, e.entry_index)
                for L in layouts for e in L["entries"] if not e.kind]
    w.record("0a-4", not kindless,
             "every row carries a resolved kind (buffer/sampler/texture/storageTexture)"
             if not kindless else "kindless rows: " + ", ".join(kindless))

    # Every row must name at least one stage — a zero mask binds nothing —
    # and every stage token must be one of the three the census can count.
    # `ShaderStage::None` or an unnamed mask would silently under-count.
    visless = ["%s entries[%d] (%r)" % (e.layout_label, e.entry_index, e.vis_raw)
               for L in layouts for e in L["entries"] if not e.vis_declared]
    unknown = sorted({t for L in layouts for e in L["entries"]
                      for t in re.findall(r"wgpu::ShaderStage::(\w+)", e.vis_raw)
                      if t not in VIS_ABBREV})
    w.record("0a-5", not visless and not unknown,
             "every row names at least one of Vertex/Fragment/Compute, and no other stage token appears"
             if not visless and not unknown else
             "; ".join(filter(None, [
                 ("empty visibility: " + ", ".join(visless)) if visless else "",
                 ("unknown stage token(s): " + ", ".join(unknown)) if unknown else ""])))

    # ─── WITNESS 0a-6 — BIND GROUP ≡ LAYOUT BIJECTION (TIDY_0a) ──────
    #
    # createBindGroup requires a BIJECTION: exactly one bind group entry
    # per layout entry, same binding numbers. Nothing above checks it.
    # 0a-1 compares a LAYOUT to its own std::array; 0a-1b compares a
    # layout's entryCount to that same array. Both are true of a 4-entry
    # bind group with a 4-entry array whose layout has 3 — which is a
    # boot failure at CreateBindGroup and was invisible to all 40
    # witnesses.
    #
    # PAID FOR BY: BUDGET_1 rows 6 and 7. Each removed one layout seat
    # and updated the bind group beside it. A LAYOUT MAY BACK MORE THAN
    # ONE BIND GROUP — the UMBRA_9 photographer duplicates — and neither
    # row followed the seat there. Row 6's own commit body said it
    # changed the array "in BOTH", meaning the layout and the bind group;
    # there were three places. Both defects rode master until TETRIS
    # WALLET_1's census tripped over one of them, and the witness output
    # that row 6 published read "layout and bind group corresponding" in
    # the SINGULAR, having checked one of the two groups that layout
    # carries. This witness exists so the next such row fails at its own
    # gate instead of at Jean's boot.
    groups = []
    for m in re.finditer(
            r"std::array<\s*wgpu::BindGroupEntry\s*,\s*(\d+)\s*>\s*(\w+)\s*\{\s*\}\s*;", src):
        declared_n = int(m.group(1))
        arr = m.group(2)
        tail = src.find("CreateBindGroup(", m.end())
        if tail < 0:
            raise SystemExit("binding_ledger: unterminated bind group block at line %d"
                             % line_of(src, m.start()))
        end = src.find(";", tail)
        block = src[m.end():end]

        # The descriptor variable is NOT always named `desc` — the gallery
        # texture group rebuilt for the exhibition view calls it `bd`, and
        # a reader hardcoding `desc` skips that group silently while
        # reporting a total that looks complete. Derive the name.
        dv = re.search(r"wgpu::BindGroupDescriptor\s+(\w+)\s*\{\s*\}\s*;", block)
        d = re.escape(dv.group(1)) if dv else r"desc"
        lm = re.search(d + r'\.label\s*=\s*"([^"]*)"\s*;', block)
        lo = re.search(d + r"\.layout\s*=\s*(\w+)\s*;", block)
        ec = re.search(d + r"\.entryCount\s*=\s*(.+?)\s*;", block)
        binds = set(re.findall(
            re.escape(arr) + r"\[\d+\]\.binding\s*=\s*(bind::\w+::\w+)\s*;", block))
        groups.append({
            # `build_render_entity_group` takes its label as a PARAMETER and
            # builds three groups from one block, so there is no literal to
            # read. Name it by where it is rather than pretending it is
            # anonymous — the line is what a reader needs either way.
            "label": lm.group(1) if lm else
                     "(label is a parameter, built at state.hpp:%d)"
                     % line_of(src, m.start()),
            "layout_member": lo.group(1) if lo else None,
            "declared_n": declared_n,
            "bindings": binds,
            "entry_count_expr": ec.group(1).strip() if ec else "<absent>",
            "line": line_of(src, m.start()),
        })

    by_member = {L["member"]: L for L in layouts if L["member"]}
    bg_bad = []
    for g in groups:
        lay = by_member.get(g["layout_member"])
        if lay is None:
            # Not a skip: a bind group whose layout the census cannot
            # resolve is a parse gap, and a witness that quietly ignores
            # it is the same failure one layer up.
            bg_bad.append("%s (line %d): layout %r does not resolve to a "
                          "parsed layout member" % (g["label"], g["line"],
                                                    g["layout_member"]))
            continue
        if g["declared_n"] != lay["declared_n"]:
            bg_bad.append("%s (line %d): %d entries against %s's %d"
                          % (g["label"], g["line"], g["declared_n"],
                             lay["label"], lay["declared_n"]))
        lay_binds = {e.binding_const for e in lay["entries"]}
        extra = g["bindings"] - lay_binds
        missing = lay_binds - g["bindings"]
        if extra:
            bg_bad.append("%s (line %d): binds %s, absent from %s"
                          % (g["label"], g["line"], ", ".join(sorted(extra)),
                             lay["label"]))
        if missing:
            bg_bad.append("%s (line %d): does NOT bind %s, required by %s"
                          % (g["label"], g["line"], ", ".join(sorted(missing)),
                             lay["label"]))
        if not g["entry_count_expr"].endswith(".size()"):
            bg_bad.append("%s (line %d): literal entryCount = %s"
                          % (g["label"], g["line"], g["entry_count_expr"]))

    shared = {}
    for g in groups:
        shared.setdefault(g["layout_member"], []).append(g["label"])
    multi = {k: v for k, v in shared.items() if len(v) > 1}
    w.record("0a-6", not bg_bad,
             "%d bind groups over %d layouts, every one a bijection with its "
             "layout; %d layout(s) back more than one group%s"
             % (len(groups), len(layouts), len(multi),
                (" — " + "; ".join("%s: %s" % (k, ", ".join(v))
                                   for k, v in sorted(multi.items()))) if multi else "")
             if not bg_bad else "; ".join(bg_bad))

    return layouts


# ═══════════════════════════════════════════════════════════════════════
# PHASE 0b — THE REACHABILITY CENSUS (WGSL)
#
# INSTRUMENT RULING (handoff): the call graph is built TEXTUALLY over the
# one module. Not naga IR reflection — naga reflects per-module, so the
# per-entry-point set still needs the closure, and an IR dependency buys
# nothing the text does not already give.
# ═══════════════════════════════════════════════════════════════════════

def strip_wgsl_comments(src):
    """Blank out // and (nestable) /* */ comments, preserving offsets."""
    out = list(src)
    i, n, depth = 0, len(src), 0
    while i < n:
        if depth == 0 and src.startswith("//", i):
            while i < n and src[i] != "\n":
                out[i] = " "
                i += 1
            continue
        if src.startswith("/*", i):
            depth += 1
            out[i] = out[i + 1] = " "
            i += 2
            continue
        if depth and src.startswith("*/", i):
            depth -= 1
            out[i] = out[i + 1] = " "
            i += 2
            continue
        if depth and src[i] != "\n":
            out[i] = " "
        i += 1
    return "".join(out)


# ─── The const environment. Array counts are spelled with named
#     constants (PAINTING_MAX_SLOTS, FIELD_SUBSCRIBERS), so a size
#     calculator that cannot fold them cannot size the bindings that
#     matter. Only integer-valued constants are folded; anything else
#     is simply absent, and a type that needs it reports unresolved
#     rather than guessing. ──────────────────────────────────────────

def parse_wgsl_consts(src):
    env = {}
    for m in re.finditer(r"^const\s+(\w+)\s*(?::\s*([\w<>]+)\s*)?=\s*([^;]+);", src, re.M):
        name, expr = m.group(1), m.group(3).strip()
        v = eval_const(expr, env)
        if v is not None:
            env[name] = v
    return env


def eval_const(expr, env):
    """Fold an integer-valued WGSL const expression, or return None.

    Deliberately narrow: literals with u/i suffixes, the four arithmetic
    operators, parentheses, u32()/i32() casts, and references to
    already-folded constants. Anything else — floats, struct values,
    array constructors, function calls — folds to None and the caller
    reports the size as unresolved.
    """
    e = expr.strip()
    if not e:
        return None
    e = re.sub(r"\b(?:u32|i32)\s*\(", "(", e)
    e = re.sub(r"\b(\d+)[uif]\b", r"\1", e)
    if not re.fullmatch(r"[\w\s+\-*/%()]+", e):
        return None
    if re.search(r"\b\d+\.\d*|\.\d+\b", e):
        return None
    for name in sorted(set(re.findall(r"[A-Za-z_]\w*", e)), key=len, reverse=True):
        if name not in env:
            return None
        e = re.sub(r"\b%s\b" % re.escape(name), str(env[name]), e)
    try:
        v = eval(e, {"__builtins__": {}}, {})     # noqa: S307 — arithmetic only
    except Exception:
        return None
    return int(v) if isinstance(v, (int, float)) and float(v).is_integer() else None


def parse_wgsl_structs(src):
    """name -> [(member_name, member_type, align_override, size_override)]"""
    structs = {}
    for m in re.finditer(r"^struct\s+(\w+)\s*\{", src, re.M):
        depth, p = 0, m.end() - 1
        while p < len(src):
            if src[p] == "{":
                depth += 1
            elif src[p] == "}":
                depth -= 1
                if depth == 0:
                    break
            p += 1
        body = src[m.end():p]
        members = []
        for line in split_top_level(body, ","):
            line = line.strip()
            if not line:
                continue
            al = re.search(r"@align\s*\(\s*(\d+)\s*\)", line)
            sz = re.search(r"@size\s*\(\s*(\d+)\s*\)", line)
            line = re.sub(r"@\w+\s*(\([^)]*\))?", "", line).strip()
            mm = re.match(r"(\w+)\s*:\s*(.+)$", line, re.S)
            if not mm:
                continue
            members.append((mm.group(1), " ".join(mm.group(2).split()),
                            int(al.group(1)) if al else None,
                            int(sz.group(1)) if sz else None))
        structs[m.group(1)] = members
    return structs


def split_top_level(text, sep, angle=True):
    """Split on `sep` at bracket depth zero.

    `angle` counts `<` and `>` as nesting, which is what WGSL type
    spellings need (`array<T, N>`) and what C++ argument lists must NOT
    have — `c->world_state_` would unbalance the depth and swallow every
    following comma.
    """
    opens, closes = ("<([{", ">)]}") if angle else ("([{", ")]}")
    out, depth, cur = [], 0, []
    for ch in text:
        if ch in opens:
            depth += 1
        elif ch in closes:
            depth -= 1
        if ch == sep and depth == 0:
            out.append("".join(cur))
            cur = []
        else:
            cur.append(ch)
    out.append("".join(cur))
    return out


def round_up(k, n):
    return ((n + k - 1) // k) * k


SCALARS = {"f32": (4, 4), "i32": (4, 4), "u32": (4, 4), "f16": (2, 2)}


class WgslType:
    """A resolved WGSL store type: its layout, and whether uniform admits it.

    `size` is None for anything containing a runtime-sized array or an
    unfoldable element count — the census reports unresolved rather than
    inventing a number.
    """

    def __init__(self, spelling):
        self.spelling = spelling
        self.align = None
        self.size = None
        self.runtime = False
        self.unresolved = None            # why the size is unknown
        self.uniform_blockers = []        # concrete reasons uniform refuses it
        self.array_elem = None            # (elem WgslType, count, stride)


def layout_of(spelling, structs, consts, cache=None, stack=()):
    """Compute AlignOf/SizeOf and uniform-address-space legality for a type.

    Rules are WGSL §"Memory Layout" and §"Address Space Layout Constraints":
    array stride is roundUp(AlignOf(E), SizeOf(E)); a struct's size rounds up
    to its own alignment; and in the UNIFORM address space

      · an array's element STRIDE must be a multiple of 16,
      · every struct member's offset must be a multiple of
        RequiredAlignOf(member, uniform) — which is roundUp(16, AlignOf) when
        the member is itself a struct or an array, and AlignOf otherwise,
      · a struct-or-array member must be followed by roundUp(16, SizeOf) of
        space before the next member,
      · atomics and runtime-sized arrays are storage-only.

    This is the predicate that actually decides a storage→uniform demotion,
    and it is where a naive census goes wrong. Note what it does and does
    NOT forbid: array<f32, N> (stride 4) and array<vec2<f32>, N> (stride 8)
    are ILLEGAL; array<vec3<f32>, N> is LEGAL, because AlignOf(vec3<f32>) is
    16 and the stride therefore rounds to 16 — it merely wastes 4 B per
    element. A type's own alignment being under 16 is NOT a blocker: a
    struct of eight f32 members has AlignOf 4, stride 32, and is perfectly
    legal in uniform.
    """
    cache = {} if cache is None else cache
    t = " ".join(spelling.split())
    if t in cache:
        return cache[t]
    r = WgslType(t)

    if t in SCALARS:
        r.align, r.size = SCALARS[t]
    elif re.fullmatch(r"vec([234])<(\w+)>", t):
        n, base = re.fullmatch(r"vec([234])<(\w+)>", t).groups()
        n = int(n)
        if base not in SCALARS:
            r.unresolved = "non-scalar vector element %s" % base
        else:
            bs = SCALARS[base][1]
            r.align = (2 if n == 2 else 4) * bs
            r.size = n * bs
    elif re.fullmatch(r"mat([234])x([234])<(\w+)>", t):
        c, rw, base = re.fullmatch(r"mat([234])x([234])<(\w+)>", t).groups()
        col = layout_of("vec%s<%s>" % (rw, base), structs, consts, cache, stack)
        if col.size is None:
            r.unresolved = "column type unresolved"
        else:
            r.align = col.align
            r.size = int(c) * round_up(col.align, col.size)
    elif re.fullmatch(r"atomic<(\w+)>", t):
        inner = layout_of(re.fullmatch(r"atomic<(\w+)>", t).group(1),
                          structs, consts, cache, stack)
        r.align, r.size, r.unresolved = inner.align, inner.size, inner.unresolved
        r.uniform_blockers.append("atomic<> is not permitted in the uniform address space")
    elif t.startswith("array<"):
        inner = t[len("array<"):]
        assert inner.endswith(">")
        parts = [p.strip() for p in split_top_level(inner[:-1], ",")]
        elem = layout_of(parts[0], structs, consts, cache, stack)
        count = None
        if len(parts) > 1:
            count = eval_const(parts[1], consts)
        else:
            r.runtime = True
        r.align = elem.align
        if elem.size is None:
            r.unresolved = "element type unresolved: %s" % (elem.unresolved or parts[0])
        else:
            stride = round_up(elem.align, elem.size)
            r.array_elem = (elem, count, stride)
            if r.runtime:
                r.unresolved = "runtime-sized array"
                r.uniform_blockers.append(
                    "runtime-sized array is not permitted in the uniform address space")
            elif count is None:
                r.unresolved = "unfoldable element count %r" % parts[1]
            else:
                r.size = count * stride
            if stride % 16 != 0:
                r.uniform_blockers.append(
                    "array element stride %d B is not a multiple of 16 "
                    "(element %s: align %d, size %d)"
                    % (stride, elem.spelling, elem.align, elem.size))
        r.uniform_blockers += ["in element: " + b for b in elem.uniform_blockers]
    elif t in structs:
        if t in stack:
            r.unresolved = "recursive struct %s" % t
        else:
            off, maxalign, blockers = 0, 1, []
            members = structs[t]
            for i, (mname, mtype, aov, sov) in enumerate(members):
                mt = layout_of(mtype, structs, consts, cache, stack + (t,))
                if mt.align is None:
                    r.unresolved = "member %s: %s" % (mname, mt.unresolved or "unresolved")
                    break
                malign = aov or mt.align
                msize = sov if sov is not None else mt.size
                off = round_up(malign, off)
                maxalign = max(maxalign, malign)
                # UNIFORM: every member offset is a multiple of
                # roundUp(16, align) for struct/array members.
                req = round_up(16, malign) if (mt.array_elem is not None
                                               or mtype in structs) else malign
                if off % req:
                    blockers.append("member %s.%s at offset %d is not a multiple of %d"
                                    % (t, mname, off, req))
                blockers += ["in %s.%s: %s" % (t, mname, b) for b in mt.uniform_blockers]
                if msize is None:
                    if i == len(members) - 1 and mt.runtime:
                        r.runtime = True
                        r.unresolved = "trailing runtime-sized array in %s.%s" % (t, mname)
                    else:
                        r.unresolved = "member %s: %s" % (mname, mt.unresolved or "unresolved")
                    break
                # UNIFORM: a struct/array member must be followed by
                # roundUp(16, SizeOf) of space.
                if i < len(members) - 1 and (mt.array_elem is not None or mtype in structs):
                    nxt = members[i + 1]
                    nt = layout_of(nxt[1], structs, consts, cache, stack + (t,))
                    if nt.align is not None:
                        nalign = nxt[2] or nt.align
                        if round_up(nalign, off + msize) < off + round_up(16, msize):
                            blockers.append(
                                "member %s.%s (a %s) is followed by %s at offset %d; "
                                "uniform requires the next member at %d or later"
                                % (t, mname, "array" if mt.array_elem else "struct",
                                   nxt[0], round_up(nalign, off + msize),
                                   off + round_up(16, msize)))
                off += msize
            else:
                r.align = maxalign
                r.size = round_up(maxalign, off)
            if r.align is None and r.runtime:
                r.align = maxalign
            r.uniform_blockers = blockers
    elif t.startswith("texture_") or t.startswith("sampler"):
        r.unresolved = "handle type (no host-shareable layout)"
    else:
        r.unresolved = "unknown type %s" % t

    cache[t] = r
    return r


def uniform_element_note(ty):
    """What a storage→uniform demotion of this type would COST in element type.

    Table C's A2 rows carry this so the cost lands in the ledger rather than
    in a later surprise. Where nothing is needed, say so — "none needed" is
    the answer for most rows and it is worth stating.
    """
    if not ty.uniform_blockers:
        return "none needed"
    stride_hit = [b for b in ty.uniform_blockers if b.startswith("array element stride")]
    if stride_hit and ty.array_elem:
        elem, count, stride = ty.array_elem
        need = round_up(16, stride)
        if elem.spelling in SCALARS:
            return ("element %s has stride %d B; uniform needs a multiple of 16 — "
                    "widen to vec4<%s> (÷4 the count) or wrap in a @size(16) struct"
                    % (elem.spelling, stride, elem.spelling))
        if re.fullmatch(r"vec2<(\w+)>", elem.spelling):
            base = re.fullmatch(r"vec2<(\w+)>", elem.spelling).group(1)
            return ("element %s has stride %d B; uniform needs a multiple of 16 — "
                    "widen to vec4<%s> (pack two per element) or pad to %d B"
                    % (elem.spelling, stride, base, need))
        return ("element %s has stride %d B; uniform needs a multiple of 16 — "
                "pad the element to %d B (@size(%d) or an explicit tail member)"
                % (elem.spelling, stride, need, need))
    return "; ".join(ty.uniform_blockers)


class WgslDecl:
    __slots__ = ("symbol", "group", "binding", "address_space", "wgsl_access",
                 "wgsl_type", "has_runtime_array", "line", "layout")

    def __init__(self, **kw):
        for k in self.__slots__:
            setattr(self, k, kw.get(k))


DECL_RE = re.compile(
    r"@group\((\d+)\)\s*@binding\((\d+)\)\s*var\s*"
    r"(?:<\s*([^>]*?)\s*>)?\s*(\w+)\s*:\s*([^;]+);")


def parse_wgsl_decls(w, src, structs, consts):
    decls = []
    for m in DECL_RE.finditer(src):
        space_clause = (m.group(3) or "").strip()
        parts = [p.strip() for p in space_clause.split(",")] if space_clause else []
        space = parts[0] if parts else ""
        access = parts[1] if len(parts) > 1 else ""
        if space == "storage" and not access:
            access = "read"                # WGSL default for var<storage>
        spelling = " ".join(m.group(5).split())
        lay = layout_of(spelling, structs, consts)
        decls.append(WgslDecl(
            symbol=m.group(4),
            group=int(m.group(1)),
            binding=int(m.group(2)),
            address_space=space or "handle",
            wgsl_access=access or "n/a",
            wgsl_type=spelling,
            has_runtime_array=bool(lay.runtime),
            line=line_of(src, m.start()),
            layout=lay,
        ))

    # Every `@group` occurrence in the file must have produced a row — a
    # declaration the regex skipped is a hole the census would never show.
    raw_groups = len(re.findall(r"@group\s*\(", src))
    w.record("0b-0", raw_groups == len(decls),
             "%d @group( occurrences, %d declarations parsed" % (raw_groups, len(decls)))

    slots = {}
    for d in decls:
        slots.setdefault((d.group, d.binding), []).append(d)
    aliases = sorted(s for k, v in slots.items() for s in [x.symbol for x in v][1:])

    # ─── WITNESS 0b-1 — the registry banner's stated declaration and slot
    #     counts, PARSED from binding_registry.hpp and compared to this
    #     census. Plus the alias roster: the three frustum-cull faces
    #     (fc_config / fc_patches / fc_vp) and, since the LOOM_2 recut,
    #     the twelve MESHGEN convergence aliases — five scratch trios
    #     share one params / one vertices / one indices slot, so four
    #     trios alias the representative's numbers.
    #
    #     IT USED TO BE A LITERAL HERE, and the literal tolled three
    #     campaigns running: 100/97 (BUDGET_1) -> 99/96 (TETRIS ORB_V) ->
    #     97/94 (WALLET_1revA). Three homes held one fact — the banner
    #     states the counts, the census finds them, and this line hardcoded
    #     what the banner says — so every subject campaign that moved a
    #     declaration had to edit an INSTRUMENT to stay green, which
    #     standing order 3 forbids from riding the same commit. The result
    #     was a red witness on master between the two, every time.
    #
    #     The banner is the single authority now. A subject campaign edits
    #     one caption; this witness still does its whole job, which was
    #     never "know the number" but "catch the banner disagreeing with
    #     reality". A malformed or missing banner is a FAILURE, not a skip
    #     — an unparseable authority is the same defect as a wrong one,
    #     and skipping would make the witness silently inert.
    #     LOOM_3: the alias list is DERIVED, not typed. An alias is exactly a
    #     DECLS row carrying alias_of, so the schema already knows the whole
    #     set and knew it before this line was ever written. Fifteen decl
    #     names sat here until now, and any rename among them would have
    #     forced an instrument commit to keep 0b-1 green — the same defect
    #     0b-1's own banner describes, still standing in 0b-1's own body.
    expect_aliases = sorted(k for k, v in schema.DECLS.items() if v.get("alias_of"))
    bm = re.search(r"world\.wgsl\s*\((\d+)\s+declarations\s+over\s+(\d+)\s+slots",
                   read(REGISTRY_HPP))
    if bm is None:
        w.record("0b-1", False,
                 "binding_registry.hpp's banner does not state "
                 "'<N> declarations over <M> slots' — the witness cannot read its "
                 "own authority; census found %d over %d"
                 % (len(decls), len(slots)))
    else:
        said_decls, said_slots = int(bm.group(1)), int(bm.group(2))
        ok = (len(decls) == said_decls and len(slots) == said_slots
              and aliases == expect_aliases)
        w.record("0b-1", ok,
                 "banner reproduced: %d declarations over %d slots; aliases %s"
                 % (len(decls), len(slots), ", ".join(aliases)) if ok else
                 "banner says %d declarations over %d slots with aliases %s; census found "
                 "%d over %d with aliases %s"
                 % (said_decls, said_slots, ", ".join(expect_aliases),
                    len(decls), len(slots), ", ".join(aliases) or "none"))

    # ─── The layout calculator, checked against the PROGRAM's own prose.
    #     If the calculator cannot reproduce the byte counts the people who
    #     sized these structs wrote down, no A2 row it produces is worth
    #     reading.
    #
    #     ═══ LOOM_3 — THE MARKER IS THE REGISTRATION ═══════════════════
    #
    #     A RENAME IN THE SUBJECT MUST NEVER REQUIRE AN EDIT IN THE WITNESS.
    #     This control used to be a hand-written list of declaration names,
    #     and it was edited twice in one campaign: CHORD_2 merged two of its
    #     three symbols into a block and CHORD_4 merged the third, so each
    #     time the subject moved, an INSTRUMENT commit had to move with it —
    #     which standing order 3 forbids from riding the same commit, so the
    #     witness went red on master in between. That is 0b-1's defect, one
    #     witness over, and the cure is the same one 0b-1 got: stop keeping a
    #     list, and read the program's own convention instead.
    #
    #     THE CONVENTION IS ALREADY THERE. L3 makes every mirrored struct
    #     announce itself in world.wgsl with `BYTE-FOR-BYTE (N B`, because a
    #     mirror without its size written down is not a handshake. So the
    #     marker enrolls the struct: every one found is checked, and the
    #     struct checked is the next one declared after it.
    #
    #     WRITING THE MARKER IS HOW YOU JOIN. An author who adds a mirrored
    #     struct and states its size in the banner — which L3 already asks
    #     for — has registered it here, with no edit to this file and nothing
    #     to remember. An author who renames one has done nothing at all,
    #     because no name is written down here to go stale.
    #
    #     It also widened the control for free: the list held two structs, the
    #     convention finds every mirrored struct in the module, including the
    #     two that FELL OUT of the old list when CHORD turned them from
    #     declarations into members.
    raw_for_markers = read(WORLD_WGSL)
    marker_re = re.compile(r"BYTE-FOR-BYTE[^)]*?\(\s*(\d+)\s*B\b", re.S)
    struct_re = re.compile(r"^\s*struct\s+(\w+)\s*\{", re.M)
    stated, off = [], []
    for m in marker_re.finditer(raw_for_markers):
        want = int(m.group(1))
        nxt = struct_re.search(raw_for_markers, m.end())
        line = raw_for_markers[:m.start()].count("\n") + 1
        if nxt is None:
            off.append("marker at world.wgsl:%d states %d B but no struct "
                       "declaration follows it" % (line, want))
            continue
        name = nxt.group(1)
        lay = layout_of(name, structs, consts)
        got = lay.size if lay else None
        stated.append((name, want))
        if got != want:
            off.append("%s (world.wgsl:%d): prose says %s, calculator says %s"
                       % (name, line, want, got))
    if not stated and not off:
        off.append("no BYTE-FOR-BYTE markers found in world.wgsl — the "
                   "registration convention has gone missing, and an empty "
                   "control is not a passing one")
    w.record("0b-4", not off,
             "WGSL layout calculator reproduces every byte count the module's "
             "BYTE-FOR-BYTE markers state (%d struct(s), marker-registered): %s"
             % (len(stated), ", ".join("%s %d B" % s for s in stated))
             if not off else "; ".join(off))

    # ─── The uniform-legality predicate, checked against the program.
    #     Every declaration the program ALREADY places in the uniform
    #     address space compiles today, so the predicate MUST judge every
    #     one of them legal. A false positive here would let an A2 row
    #     propose a demotion the compiler refuses — the exact failure G4
    #     names.
    already = [d for d in decls if d.address_space == "uniform"]
    wrong = ["%s (%s): %s" % (d.symbol, d.wgsl_type, d.layout.uniform_blockers[0])
             for d in already if d.layout.uniform_blockers]
    w.record("0b-5", not wrong,
             "the uniform-legality predicate clears all %d declarations the program already "
             "places in the uniform address space" % len(already)
             if not wrong else
             "predicate rejects %d live uniform declaration(s): %s"
             % (len(wrong), "; ".join(wrong)))
    return decls, slots


# ─── 0b-ii — the reachability closure ────────────────────────────────

class WgslFn:
    __slots__ = ("name", "stage", "workgroup_size", "body", "params", "start", "line",
                 "calls", "refs", "uses", "sites", "shadowed")

    def __init__(self, **kw):
        for k in self.__slots__:
            setattr(self, k, kw.get(k))


STAGE_OF_ATTR = {"vertex": "V", "fragment": "F", "compute": "C"}

# One combined scanner so braces, local declarations and identifiers are
# seen in a single offset-ordered pass.
SCOPE_TOK = re.compile(
    r"(?P<open>\{)|(?P<close>\})"
    r"|\b(?:let|var|const)\b\s*(?:<[^>]*>)?\s*(?P<decl>[A-Za-z_]\w*)"
    r"|(?<![\w.])(?P<id>[A-Za-z_]\w*)")


def scoped_idents(fn, interesting):
    """Yield (identifier, is_module_scope) for every occurrence in a body.

    A name declared by `let`/`var`/`const`, or bound as a parameter,
    shadows the module scope for the rest of its block. Without this, a
    local named after a binding reads as a reference to that binding —
    which is how `patch_terrain_fs` appeared to reach `patch_grid`.
    """
    scopes = [set(re.findall(r"(?<![\w.])(\w+)\s*:", fn.params or ""))]
    for m in SCOPE_TOK.finditer(fn.body):
        if m.group("open"):
            scopes.append(set())
        elif m.group("close"):
            if len(scopes) > 1:
                scopes.pop()
        elif m.group("decl"):
            name = m.group("decl")
            scopes[-1].add(name)
            if name in interesting:
                yield name, False, m.start("decl"), m.end("decl")
        else:
            tok = m.group("id")
            if tok not in interesting:
                continue
            yield (tok, not any(tok in sc for sc in scopes),
                   m.start("id"), m.end("id"))


def parse_wgsl_functions(w, src, decl_names):
    fns = {}
    for m in re.finditer(r"\bfn\s+(\w+)\s*\(", src):
        name = m.group(1)
        p = m.end() - 1
        depth = 0
        while p < len(src):
            if src[p] == "(":
                depth += 1
            elif src[p] == ")":
                depth -= 1
                if depth == 0:
                    break
            p += 1
        brace = src.find("{", p)
        if brace < 0:
            continue
        depth, q = 0, brace
        while q < len(src):
            if src[q] == "{":
                depth += 1
            elif src[q] == "}":
                depth -= 1
                if depth == 0:
                    break
            q += 1
        body = src[brace:q + 1]

        # Attributes precede `fn`, on the same line or the lines above,
        # separated only by whitespace.
        head_start = m.start()
        k = head_start - 1
        while k >= 0:
            back = src[:k + 1]
            am = re.search(r"@\w+\s*(\([^)]*\))?\s*$", back)
            if not am:
                break
            k = am.start() - 1
        head = src[k + 1:head_start]
        stage = None
        for a, s in STAGE_OF_ATTR.items():
            if re.search(r"@%s\b" % a, head):
                stage = s
        wg = re.search(r"@workgroup_size\s*\(([^)]*)\)", head)
        fns[name] = WgslFn(name=name, stage=stage, params=src[m.end():p],
                           workgroup_size=("(%s)" % " ".join(wg.group(1).split())) if wg else "",
                           body=body, start=brace, line=line_of(src, m.start()),
                           calls=set(), refs=set())

    # Reference extraction. An identifier preceded by `.` is member
    # access, not a module-scope name; the `fn` keyword itself is a
    # definition, not a call. Everything else that matches a function
    # name or a binding symbol counts as a reference — UNLESS a local
    # declaration or parameter shadows it in scope. BUDGET_0b noted the
    # shadowing hazard and accepted it as one-directional; extension 1's
    # W1-1 then caught it live (`let patch_grid = vec2<i32>(…)` in two
    # functions), so the closure now resolves scope properly rather than
    # over-approximating.
    for fn in fns.values():
        fn.shadowed = set()
        for tok, is_ref, _s, _e in scoped_idents(fn, decl_names | set(fns)):
            if not is_ref:
                fn.shadowed.add(tok)
                continue
            if tok in fns:
                fn.calls.add(tok)
            elif tok in decl_names:
                fn.refs.add(tok)

    entries = {n: f for n, f in fns.items() if f.stage}
    w.record("0b-2", len(entries) > 0,
             "%d functions, %d entry points (%d vertex, %d fragment, %d compute)"
             % (len(fns), len(entries),
                sum(1 for f in entries.values() if f.stage == "V"),
                sum(1 for f in entries.values() if f.stage == "F"),
                sum(1 for f in entries.values() if f.stage == "C")))
    missing_wg = [n for n, f in entries.items() if f.stage == "C" and not f.workgroup_size]
    w.record("0b-3", not missing_wg,
             "every @compute entry point carries a @workgroup_size" if not missing_wg
             else "no workgroup_size on: " + ", ".join(sorted(missing_wg)))
    return fns, entries


def reachability(fns, entries):
    """entry point -> (reached function set, reached binding symbols)."""
    out = {}
    for name, fn in entries.items():
        seen, stack = set(), [name]
        while stack:
            cur = stack.pop()
            if cur in seen:
                continue
            seen.add(cur)
            stack.extend(fns[cur].calls - seen)
        refs = set()
        for f in seen:
            refs |= fns[f].refs
        out[name] = (seen, refs)
    return out


# ═══════════════════════════════════════════════════════════════════════
# EXTENSION 1 — READ/WRITE SEPARATION
#
# BUDGET_0 recorded that a binding is REFERENCED. This records what the
# reference DOES. Two things turn on it: whether two dispatches can be
# fused (a fused kernel deletes the inter-dispatch barrier that is making
# the sequence correct today), and whether a `read_write` declaration
# claims a capability it never exercises.
# ═══════════════════════════════════════════════════════════════════════

# Atomic builtins, by what they do to the atomic they are handed.
ATOMIC_RW = ("atomicAdd", "atomicSub", "atomicMax", "atomicMin", "atomicAnd",
             "atomicOr", "atomicXor", "atomicExchange", "atomicCompareExchangeWeak")
ATOMIC_W = ("atomicStore",)
ATOMIC_R = ("atomicLoad",)

COMPOUND_ASSIGN = ("<<=", ">>=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=")


def _skip_ws(s, i, step=1):
    while 0 <= i < len(s) and s[i] in " \t\r\n":
        i += step
    return i


def _consume_access_chain(s, i):
    """From just past a symbol, consume `[…]` and `.ident` to the chain's end."""
    while True:
        j = _skip_ws(s, i)
        if j < len(s) and s[j] == "[":
            depth, k = 0, j
            while k < len(s):
                if s[k] == "[":
                    depth += 1
                elif s[k] == "]":
                    depth -= 1
                    if depth == 0:
                        break
                k += 1
            i = k + 1
            continue
        if j < len(s) and s[j] == "." and j + 1 < len(s) and (s[j + 1].isalpha() or s[j + 1] == "_"):
            k = j + 1
            while k < len(s) and (s[k].isalnum() or s[k] == "_"):
                k += 1
            i = k
            continue
        return i


def classify_use(body, start, end):
    """What does this reference to a binding DO? -> 'r' | 'w' | 'rw'.

    Lexical and deliberately biased: an ambiguous site is called a WRITE.
    Both consumers are safe in that direction — a spurious write adds a RAW
    pair (a false bar on fusing, not a false blessing) and removes an A4
    flag (a missed correction, not a wrong one).
    """
    # A builtin call wrapping the reference: textureStore(tex, …),
    # atomicAdd(&buf[i], …). Look back past whitespace and an optional `&`.
    b = _skip_ws(body, start - 1, -1)
    if b >= 0 and body[b] == "&":
        b = _skip_ws(body, b - 1, -1)
    if b >= 0 and body[b] == "(":
        k = _skip_ws(body, b - 1, -1)
        e = k + 1
        while k >= 0 and (body[k].isalnum() or body[k] == "_"):
            k -= 1
        callee = body[k + 1:e]
        if callee == "textureStore":
            return "w"
        if callee in ATOMIC_W:
            return "w"
        if callee in ATOMIC_R:
            return "r"
        if callee in ATOMIC_RW:
            return "rw"

    # Otherwise: is the reference the target of an assignment?
    i = _consume_access_chain(body, end)
    j = _skip_ws(body, i)
    tail = body[j:j + 3]
    for op in COMPOUND_ASSIGN:
        if tail.startswith(op):
            return "rw"
    if tail.startswith("++") or tail.startswith("--"):
        return "rw"
    if tail.startswith("=") and not tail.startswith("=="):
        return "w"
    return "r"


def annotate_uses(w, src, fns, decls):
    """Attach per-function {symbol: 'r'|'w'|'rw'} to every function.

    Also records each access SITE (offset, index expression, enclosing
    override gate) for extension 2, which asks a different question of the
    same occurrences.
    """
    names = {d.symbol for d in decls}
    for fn in fns.values():
        fn.uses = {}
        fn.sites = []
        for tok, is_ref, st, en in scoped_idents(fn, names):
            if not is_ref:
                continue
            kind = classify_use(fn.body, st, en)
            prev = fn.uses.get(tok)
            fn.uses[tok] = "rw" if (prev and prev != kind) or kind == "rw" else kind
            fn.sites.append({"symbol": tok, "off": st, "end": en, "use": kind})

    # ─── W1-0. The write detector is TEXTUAL: it sees an assignment or a
    #     builtin call at the reference. A write reached through a pointer
    #     parameter would be invisible to it. world.wgsl must therefore
    #     contain no pointer types at all, or the detector is unsound.
    ptrs = sorted(set(re.findall(r"\bptr\s*<[^>]*>", src)))
    w.record("W1-0", not ptrs,
             "world.wgsl declares no `ptr<…>` anywhere, so no write can reach a "
             "binding except through an assignment or a builtin at the reference — "
             "which is exactly what the detector sees"
             if not ptrs else "pointer types present, write detection is unsound: "
             + ", ".join(ptrs))
    return fns


def rw_closure(fns, entries, reach):
    """entry point -> {symbol: 'r'|'w'|'rw'} over its whole call closure."""
    out = {}
    for name in entries:
        acc = {}
        for f in reach[name][0]:
            for sym, kind in fns[f].uses.items():
                prev = acc.get(sym)
                acc[sym] = "rw" if (prev and prev != kind) or kind == "rw" else kind
        out[name] = acc
    return out


def reads(u):
    return {s for s, k in u.items() if k in ("r", "rw")}


def writes(u):
    return {s for s, k in u.items() if k in ("w", "rw")}


def phase_ext1(w, wgsl, cen, layouts):
    fns, entries, reach = wgsl["fns"], wgsl["entries"], wgsl["reach"]
    ep_use = rw_closure(fns, entries, reach)
    wgsl["ep_use"] = ep_use

    # ─── W1-1 — a binding the WGSL declares non-writable must have an
    #     empty write-set, and a write-only storage texture must have an
    #     empty read-set. A violation is a detector bug, not a program
    #     defect: the shader would not compile.
    bad = []
    for d in wgsl["decls"]:
        writable = (d.address_space == "storage" and d.wgsl_access == "read_write") \
                   or d.wgsl_type.startswith("texture_storage")
        readable = not (d.wgsl_type.startswith("texture_storage")
                        and re.search(r",\s*write\s*>", d.wgsl_type))
        for ep, u in ep_use.items():
            k = u.get(d.symbol)
            if k is None:
                continue
            if not writable and k in ("w", "rw"):
                bad.append("%s: write recorded in %s but declared %s %s"
                           % (d.symbol, ep, d.address_space, d.wgsl_access))
            if not readable and k in ("r", "rw"):
                bad.append("%s: read recorded in %s but declared write-only"
                           % (d.symbol, ep))
    w.record("W1-1", not bad,
             "no write recorded on any binding the WGSL declares non-writable, and no "
             "read on any write-only storage texture, across all %d entry points"
             % len(entries) if not bad else "; ".join(sorted(set(bad))[:6]))

    # ─── 1a — the RAW table. Restricted to FUSION-ELIGIBLE pairs: a fused
    #     kernel has ONE pipeline layout, so only compute entry points whose
    #     pipelines carry identical group layouts could ever be fused. The
    #     unrestricted matrix is counted too, so the restriction hides
    #     nothing.
    cs = [p for p in cen["pipelines"] if p.kind == "compute"]
    ep_of = {}
    for p in cs:
        ep_of.setdefault(p.cs_entry and cen["entry_const"].get(p.cs_entry), []).append(p)
    comp_eps = [e for e in ep_of if e]

    raw_rows, all_pairs, all_nonempty = [], 0, 0
    for a in sorted(comp_eps):
        for b_ep in sorted(comp_eps):
            if a == b_ep:
                continue
            all_pairs += 1
            hazard = writes(ep_use[a]) & reads(ep_use[b_ep])
            if hazard:
                all_nonempty += 1
            same = (ep_of[a][0].group_layouts == ep_of[b_ep][0].group_layouts)
            if same:
                raw_rows.append({"a": a, "b": b_ep, "hazard": sorted(hazard),
                                 "layouts": ep_of[a][0].group_layouts})

    # ─── W1-3 — the table exists whether or not it has rows, and its pair
    #     count is stated. Silence and absence must be distinguishable.
    w.record("W1-3",
             len(raw_rows) == sum(1 for a in comp_eps for b_ep in comp_eps
                                  if a != b_ep
                                  and ep_of[a][0].group_layouts == ep_of[b_ep][0].group_layouts),
             "RAW table: %d fusion-eligible ordered pairs (same pipeline layout) of %d "
             "compute entry points; %d carry a hazard. Unrestricted matrix: %d ordered "
             "pairs, %d with a non-empty write∩read."
             % (len(raw_rows), len(comp_eps),
                sum(1 for r in raw_rows if r["hazard"]), all_pairs, all_nonempty))

    # ─── 1a-ii — THE UNORDERED CLOSURE. Table E's ordered rows answer
    #     "is this sequence safe as written". Fusion asks a different
    #     question: a fused kernel has NO ordering — its threads run
    #     concurrently inside one dispatch — so a pair is fusable only if
    #     BOTH orderings are hazard-free. Taking the closure is what turns
    #     a hazard table into a fusability answer.
    seen_pairs, unordered = set(), []
    for r in raw_rows:
        key = tuple(sorted((r["a"], r["b"])))
        if key in seen_pairs:
            continue
        seen_pairs.add(key)
        fwd = next((x for x in raw_rows if x["a"] == key[0] and x["b"] == key[1]), None)
        rev = next((x for x in raw_rows if x["a"] == key[1] and x["b"] == key[0]), None)
        dirs = []
        if fwd and fwd["hazard"]:
            dirs.append(("%s -> %s" % (key[0], key[1]), fwd["hazard"]))
        if rev and rev["hazard"]:
            dirs.append(("%s -> %s" % (key[1], key[0]), rev["hazard"]))
        unordered.append({"pair": key, "clean": not dirs, "barred_dirs": dirs})
    unordered.sort(key=lambda x: (not x["clean"], x["pair"]))

    clean_pairs = [u for u in unordered if u["clean"]]
    w.record("W1-4", True,
             "unordered closure: %d fusion-eligible unordered pairs; %d are hazard-free "
             "in BOTH directions%s. RAW-clean is necessary, not sufficient — cadence is "
             "the second gate and it lives in the dispatch schedule, not the shader."
             % (len(unordered), len(clean_pairs),
                (" — " + ", ".join("{%s, %s}" % u["pair"] for u in clean_pairs))
                if clean_pairs else ""))

    # ─── 1b — A4, OVER-PRIVILEGED. Declared read_write, never written.
    a4 = []
    for d in wgsl["decls"]:
        if not (d.address_space == "storage" and d.wgsl_access == "read_write"):
            continue
        touch = {ep: u[d.symbol] for ep, u in ep_use.items() if d.symbol in u}
        if touch and all(k == "r" for k in touch.values()):
            a4.append({"decl": d, "readers": sorted(touch),
                       "layout_rows": [], "vertex_hazard": None})
    return {"ep_use": ep_use, "raw": raw_rows, "a4": a4, "unordered": unordered,
            "raw_all_pairs": all_pairs, "raw_all_nonempty": all_nonempty,
            "compute_eps": sorted(comp_eps)}


def phase_0b(w):
    raw = read(WORLD_WGSL)
    src = strip_wgsl_comments(raw)
    consts = parse_wgsl_consts(src)
    structs = parse_wgsl_structs(src)
    decls, slots = parse_wgsl_decls(w, src, structs, consts)
    fns, entries = parse_wgsl_functions(w, src, {d.symbol for d in decls})
    annotate_uses(w, src, fns, decls)
    reach = reachability(fns, entries)
    return {"src": src, "consts": consts, "structs": structs, "decls": decls,
            "slots": slots, "fns": fns, "entries": entries, "reach": reach}


# ═══════════════════════════════════════════════════════════════════════
# EXTENSION 2 — ACCESS-PATTERN CLASSIFICATION
#
# The ledger records a binding's TYPE. Whether it can leave the storage
# wallet for a vertex buffer turns on how it is INDEXED, which is a
# different question — and the one the logistics survey got backwards in
# both directions on the same pair of bindings.
# ═══════════════════════════════════════════════════════════════════════

SEQ_BUILTINS = {"instance_index": "instance", "vertex_index": "vertex"}
OTHER_BUILTINS = ("global_invocation_id", "local_invocation_id", "workgroup_id",
                  "num_workgroups", "local_invocation_index", "front_facing",
                  "position", "sample_index", "sample_mask", "frag_depth",
                  "subgroup_invocation_id", "subgroup_size")

# Least-eligible wins when a variable is assigned more than once.
CLASS_RANK = {"builtin_sequential": 0, "scalar": 1, "builtin_derived": 2,
              "other": 3, "indirected": 4}


class IdxClass:
    __slots__ = ("cls", "source")

    def __init__(self, cls, source=""):
        self.cls, self.source = cls, source

    def __repr__(self):
        return "%s(%s)" % (self.cls, self.source) if self.source else self.cls


def join_class(a, b):
    if a is None:
        return b
    if b is None:
        return a
    return a if CLASS_RANK[a.cls] >= CLASS_RANK[b.cls] else b


AFFINE_OK = re.compile(r"^[\w\s\+\-\*\(\)\.]*$")


def classify_expr(expr, env, bindings, consts, overrides, uniforms, params=()):
    """Classify an index expression into the five-class taxonomy."""
    e = " ".join(expr.split())
    if not e:
        return IdxClass("scalar", "(no index)")

    # An index that is itself a load from another binding is an
    # INDIRECTION, whatever it is indexed by. This test comes first
    # because an index like the pre-B3 `visible_patch_indices[patch_id]`
    # is sequential in patch_id and still makes its consumer
    # random-access (the rule outlives that example — DOMESDAY_0 B3
    # migrated it to a vertex attribute).
    for m in re.finditer(r"(?<![\w.])([A-Za-z_]\w*)\s*\[", e):
        if m.group(1) in bindings:
            return IdxClass("indirected", m.group(1))

    # `select`, `min`, `f32`… are CALLEES, not data sources — the
    # arguments are. `true`/`false` are literals, not identifiers.
    idents = [t for t in re.findall(r"(?<![\w.])([A-Za-z_]\w*)(?!\s*\()", e)
              if t not in ("true", "false")]
    tainted = [(t, env[t]) for t in idents if t in env and env[t] is not None]
    if tainted:
        worst = None
        for _, k in tainted:
            worst = join_class(worst, k)
        if worst.cls == "builtin_sequential":
            # Affine only: no division, modulo, member access, call or
            # select. Anything else is many elements per primitive.
            body = e
            for t, _ in tainted:
                body = re.sub(r"(?<![\w.])%s(?![\w])" % re.escape(t), " ", body)
            if ("/" in e or "%" in e or "." in e or "select" in e
                    or re.search(r"\w\s*\(", e) or not AFFINE_OK.match(e)):
                return IdxClass("builtin_derived", worst.source)
            return IdxClass("builtin_sequential", worst.source)
        return worst
    if any(t in uniforms for t in idents):
        return IdxClass("scalar", next(t for t in idents if t in uniforms))
    if all(t in consts or t in overrides or t in SCALARS for t in idents) or not idents:
        src = ", ".join(t for t in idents if t in overrides) or "const"
        return IdxClass("scalar", src)
    unknown = [t for t in idents if t not in consts and t not in overrides]
    if params and all(t in params for t in unknown):
        # The index comes from this function's own parameters. The closure
        # is not interprocedural, so say so rather than inventing a class.
        return IdxClass("other", "callee parameter: " + ", ".join(sorted(set(unknown))))
    return IdxClass("other", e[:60])


ASSIGN_RE = re.compile(
    r"(?:\b(?:let|var|const)\b\s*(?:<[^>]*>)?\s*(?P<decl>\w+)\s*(?::[^=;]+)?=\s*(?P<dexpr>[^;]+);)"
    r"|(?:(?:^|(?<=[{};]))\s*(?P<lhs>[A-Za-z_]\w*)\s*(?P<op>[+\-*/%&|^]?=)(?!=)"
    r"\s*(?P<aexpr>[^;]+);)", re.M)


def taint_env(fn, bindings, consts, overrides, uniforms):
    """Local dataflow: which locals carry a builtin, an indirection, or neither."""
    env = {}
    pnames = set(re.findall(r"(?<![\w.])(\w+)\s*:", fn.params or ""))
    for pm in re.finditer(r"@builtin\s*\(\s*(\w+)\s*\)\s*(\w+)\s*:", fn.params or ""):
        b, name = pm.group(1), pm.group(2)
        if b in SEQ_BUILTINS:
            env[name] = IdxClass("builtin_sequential", SEQ_BUILTINS[b])
        else:
            env[name] = IdxClass("builtin_derived", b)
    # DOMESDAY_0 B3: an entry-point @location parameter is a VERTEX
    # ATTRIBUTE input — per-instance or per-vertex data the fixed
    # function fetched. As an index provenance it is arbitrary data
    # (the attribute can carry anything), so it ranks with `other`,
    # and its source names the location so the ledger can say where
    # the index came from.
    for pm in re.finditer(r"@location\s*\(\s*(\d+)\s*\)\s*(?:@[\w()]+\s*)*(\w+)\s*:",
                          fn.params or ""):
        env[pm.group(2)] = IdxClass("other",
                                    "vertex attribute @location(%s)" % pm.group(1))
    for om in re.finditer(r"@builtin\s*\(\s*(\w+)\s*\)", fn.body[:0] or ""):
        pass
    for m in ASSIGN_RE.finditer(fn.body):
        name = m.group("decl") or m.group("lhs")
        expr = m.group("dexpr") or m.group("aexpr")
        if not name or name in bindings:
            continue
        env[name] = join_class(env.get(name),
                               classify_expr(expr, env, bindings, consts, overrides,
                                             uniforms, pnames))
        if m.group("op") and m.group("op") != "=":
            env[name] = join_class(env[name], IdxClass("builtin_derived", "compound assign"))
    return env


def if_spans_wgsl(body):
    """(start, end, condition) for every braced `if (...)` in a body."""
    spans = []
    for m in re.finditer(r"\bif\s*\(", body):
        j = m.end() - 1
        depth, k = 0, j
        while k < len(body):
            if body[k] == "(":
                depth += 1
            elif body[k] == ")":
                depth -= 1
                if depth == 0:
                    break
            k += 1
        cond = body[j + 1:k]
        rest = re.match(r"\s*\{", body[k + 1:k + 40])
        if not rest:
            continue
        b = k + 1 + rest.end() - 1
        depth, p = 0, b
        while p < len(body):
            if body[p] == "{":
                depth += 1
            elif body[p] == "}":
                depth -= 1
                if depth == 0:
                    break
            p += 1
        spans.append((m.start(), p, " ".join(cond.split())))
    return spans


def phase_ext2(w, wgsl):
    decls = wgsl["decls"]
    bindings = {d.symbol for d in decls}
    uniforms = {d.symbol for d in decls if d.address_space == "uniform"}
    consts = set(wgsl["consts"])
    overrides = set(re.findall(r"^override\s+(\w+)", wgsl["src"], re.M))

    sites = []
    for fn in wgsl["fns"].values():
        env = taint_env(fn, bindings, consts, overrides, uniforms)
        ifs = if_spans_wgsl(fn.body)
        for s in fn.sites:
            i = _skip_ws(fn.body, s["end"])
            if i < len(fn.body) and fn.body[i] == "[":
                depth, k = 0, i
                while k < len(fn.body):
                    if fn.body[k] == "[":
                        depth += 1
                    elif fn.body[k] == "]":
                        depth -= 1
                        if depth == 0:
                            break
                    k += 1
                idx = fn.body[i + 1:k]
            else:
                idx = ""
            cls = classify_expr(idx, env, bindings, consts, overrides, uniforms,
                                set(re.findall(r"(?<![\w.])(\w+)\s*:", fn.params or "")))
            gates = [c for (a, b_, c) in ifs if a <= s["off"] <= b_
                     and any(o in c for o in overrides)]
            sites.append({"fn": fn.name, "symbol": s["symbol"], "use": s["use"],
                          "index": " ".join(idx.split()), "cls": cls,
                          "line": fn.line + fn.body.count("\n", 0, s["off"]),
                          "override_gate": "; ".join(gates)})

    # ─── W2-1 — every classified access resolves to a Table A binding.
    stray = sorted({s["symbol"] for s in sites if s["symbol"] not in bindings})
    w.record("W2-1", not stray,
             "%d access sites classified, every one on a declared binding" % len(sites)
             if not stray else "unknown symbols: " + ", ".join(stray))

    # ─── W2-2 — classification is total, and `other` rows are listed
    #     individually with their expression. Never counted in aggregate.
    others = [s for s in sites if s["cls"].cls == "other"]
    counts = {}
    for s in sites:
        counts[s["cls"].cls] = counts.get(s["cls"].cls, 0) + 1
    w.record("W2-2", all(s["cls"].cls in CLASS_RANK for s in sites),
             "classification total: " + ", ".join("%s %d" % (k, counts[k])
                                                  for k in sorted(counts))
             + ("; `other` rows enumerated below" if others else "; no `other` rows"))

    # ─── W2-3 — the POSITIVE CONTROL, re-anchored by DOMESDAY_0 B3.
    #     The original control held visible_patch_indices[patch_id]
    #     sequential and patch_instances[actual_id] indirected. B3
    #     spent that row: the visible list arrives as an instance-step
    #     vertex attribute and the binding is retired. The known truth
    #     the classifier must now reproduce, in patch_terrain_vs: the
    #     patch_instances index joins the direct path's instance
    #     builtin with the indirect path's attribute input, so it must
    #     NOT read as pure builtin_sequential, and its source must
    #     name the vertex attribute. If it does not, the classifier is
    #     wrong in the exact way the survey was.
    #     LOOM_3: the control's identity comes from the schema
    #     (CLASSIFIER_CONTROL), not from two names typed here.
    cc = schema.CLASSIFIER_CONTROL
    pc = next((s for s in sites
               if s["fn"] == cc["entry_point"] and s["symbol"] == cc["decl"]), None)
    ok = (pc is not None
          and pc["cls"].cls != "builtin_sequential"
          and cc["expect_source_contains"] in pc["cls"].source)
    w.record("W2-3", ok,
             "positive control: patch_terrain_vs reads patch_instances[%s] as %s "
             "(direct-path sequential joined with the B3 vertex-attribute input)"
             % (pc["index"], pc["cls"])
             if ok else "control FAILED: %s"
             % {k: (v["index"], str(v["cls"])) for k, v in ctl.items()})

    # ─── The vertex-buffer eligibility predicate. Every access in every
    #     entry point that reaches the binding must be builtin_sequential
    #     on the same builtin, and the element stride must fit
    #     maxVertexBufferArrayStride. An override gate does NOT rescue a
    #     mixed classification: an override selects a branch inside ONE
    #     entry point, and one entry point has ONE signature.
    by_symbol = {}
    for s in sites:
        by_symbol.setdefault(s["symbol"], []).append(s)
    elig = []
    for d in decls:
        ss = by_symbol.get(d.symbol, [])
        if not ss:
            continue
        classes = {(s["cls"].cls, s["cls"].source) for s in ss}
        stride = d.layout.array_elem[2] if d.layout.array_elem else None
        blockers = []
        kind = None
        if len(classes) > 1:
            blockers.append("mixed classification across %d access sites: %s"
                            % (len(ss), ", ".join(sorted("%s(%s)" % c for c in classes))))
        only = next(iter(classes)) if len(classes) == 1 else None
        if only and only[0] != "builtin_sequential":
            blockers.append("indexed %s(%s), not sequential in a builtin" % only)
        if only and only[0] == "builtin_sequential":
            kind = only[1]
        if stride is None:
            blockers.append("no array element stride (not an array binding)")
        elif stride > 2048:
            blockers.append("element stride %d B exceeds maxVertexBufferArrayStride 2048"
                            % stride)
        gated = sorted({s["override_gate"] for s in ss if s["override_gate"]})
        elig.append({"decl": d, "sites": ss, "classes": sorted(classes), "stride": stride,
                     "blockers": blockers, "step": kind, "override_gates": gated,
                     "eligible": not blockers})
    return {"sites": sites, "eligibility": elig, "others": others,
            "overrides": sorted(overrides)}


# ═══════════════════════════════════════════════════════════════════════
# PHASE 0c — THE PIPELINE CENSUS
#
# The shared builders (makeEntity, makeComputePipeline, strataLayoutFor,
# makeShadow) are RESOLVED, not recorded: a pipeline built through a
# shared builder inherits that builder's pipeline layout, and the ledger
# wants the layout, not the builder's name. strataLayoutFor(frame, state,
# tex) is the LOOM_2 successor of computeLayoutFor(single): it wraps the
# four strata — WORLD is implicit and always first — so its call sites
# resolve to a four-layout list.
# ═══════════════════════════════════════════════════════════════════════

class Pipeline:
    __slots__ = ("label", "member", "kind", "vs_entry", "fs_entry", "cs_entry",
                 "group_layouts", "vertex_buffer_count", "vertex_attribute_count",
                 "color_target_count", "roster_gate", "line", "off")

    def __init__(self, **kw):
        for k in self.__slots__:
            setattr(self, k, kw.get(k))

    def entries(self):
        return [(e, s) for e, s in ((self.vs_entry, "V"), (self.fs_entry, "F"),
                                    (self.cs_entry, "C")) if e]


def parse_layout_handles(w):
    """renderer field name -> state.hpp bind group layout member.

    Two hops: state.hpp's accessors (`x_layout() const { return xLayout_; }`)
    and the renderer's init() assignments (`xLayout_ = gpuState.x_layout();`).
    Resolving both means Table C can name the LAYOUT a pipeline binds, not
    the renderer's private handle for it.
    """
    state = strip_cpp_comments(read(STATE_HPP))
    acc = {}
    for m in re.finditer(
            r"wgpu::BindGroupLayout\s+(\w+)\s*\(\s*\)\s*const\s*\{\s*return\s+(\w+)\s*;\s*\}", state):
        acc[m.group(1)] = m.group(2)
    rend = strip_cpp_comments(read(RENDERER_HPP))
    field = {}
    for m in re.finditer(r"(\w+)\s*=\s*gpuState\.(\w+)\s*\(\s*\)\s*;", rend):
        if m.group(2) in acc:
            field[m.group(1)] = acc[m.group(2)]
    w.record("0c-0", len(field) > 0,
             "%d renderer layout handles resolve to state.hpp layout members "
             "(via %d gpuState accessors)" % (len(field), len(acc)))
    return field


def parse_pipelines(w, src, spans, handles, layouts_by_member):
    """Resolve every pipeline creation to (label, member, stages, group layouts)."""
    # ─── Pipeline layouts, resolved in source order. Every variable is
    #     reassigned inside its own block, so a running map with
    #     last-write-wins is exact for this file's shape.
    arrays, plds, pls = {}, {}, {}
    events = []
    for m in re.finditer(
            r"std::array<\s*wgpu::BindGroupLayout\s*,\s*(\d+)\s*>\s*(\w+)\s*=\s*\{([^}]*)\}", src):
        items = [x.strip() for x in m.group(3).split(",") if x.strip()]
        events.append((m.start(), "array", m.group(2), items, int(m.group(1))))
    for m in re.finditer(r"(\w+)\.bindGroupLayouts\s*=\s*(\w+)\.data\(\)", src):
        events.append((m.start(), "pld", m.group(1), m.group(2), None))
    for m in re.finditer(
            r"wgpu::PipelineLayout\s+(\w+)\s*=\s*device_\.CreatePipelineLayout\(\s*&(\w+)\s*\)", src):
        events.append((m.start(), "create", m.group(1), m.group(2), None))
    # DOMESDAY_2 F2-b2: the label law gave the builder a LEADING
    # string argument (F2-b1) — the layouts are arguments two through
    # four. The label itself is display prose; the layout list is still
    # these three plus the implicit WORLD.
    for m in re.finditer(
            r"wgpu::PipelineLayout\s+(\w+)\s*=\s*strataLayoutFor\(\s*"
            r"\"[^\"]*\"\s*,\s*"
            r"(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*\)", src):
        events.append((m.start(), "for", m.group(1),
                       ["worldLayout_", m.group(2), m.group(3), m.group(4)],
                       None))

    bad_counts = []
    for pos, kind, a, b, n in sorted(events):
        if kind == "array":
            arrays[a] = b
            if n != len(b):
                bad_counts.append("%s declares %d, lists %d" % (a, n, len(b)))
        elif kind == "pld":
            plds[a] = b
        elif kind == "create":
            pls[a] = list(arrays.get(plds.get(b, ""), []))
        elif kind == "for":
            pls[a] = list(b)
    w.record("0c-0b", not bad_counts,
             "every std::array<BindGroupLayout, N> lists exactly N members"
             if not bad_counts else "; ".join(bad_counts))

    def resolve(plvar, at):
        return [handles.get(h, h) for h in pls.get(plvar, [])]

    # Snapshot the pipeline-layout map at each use site: the same variable
    # name (`pl`, `layout`) is rebound in later blocks.
    snapshots = []
    for pos, kind, a, b, n in sorted(events):
        if kind in ("create", "for"):
            snapshots.append((pos, a,
                              list(pls_at(events, pos, handles).get(a, []))))

    def layouts_at(plvar, pos):
        best = None
        for p, name, val in snapshots:
            if name == plvar and p <= pos:
                best = val
        return best if best is not None else []

    pipelines = []

    # ─── Compute: every one goes through makeComputePipeline. ─────────
    #     AUBADE U3 added an OPTIONAL sixth argument — the PipeGroup that
    #     says whether the row gates the frame loop. The tail is therefore
    #     `member)` or `member, PipeGroup::X)`, and the census reads both:
    #     four rows carry the tag today and a silent regex miss would drop
    #     them from the ledger without failing anything until ML-0.
    for m in re.finditer(
            r"makeComputePipeline\(\s*\"([^\"]*)\"\s*,\s*\"([^\"]*)\"\s*,\s*"
            r"(\w+)\s*,\s*Entry::(\w+)\s*,\s*(\w+)\s*(?:,\s*PipeGroup::\w+\s*)?\)",
            src, re.S):
        pipelines.append(Pipeline(
            label=m.group(2), member=m.group(5), kind="compute",
            cs_entry=m.group(4), group_layouts=layouts_at(m.group(3), m.start()),
            vertex_buffer_count=0, vertex_attribute_count=0, color_target_count=0,
            roster_gate=roster_gate_of(spans, m.start()), line=line_of(src, m.start()),
            off=m.start()))

    # ─── Vertex buffer layouts: attribute counts by VBL variable. ─────
    attrs = {}
    for m in re.finditer(
            r"std::array<\s*wgpu::VertexAttribute\s*,\s*(\d+)\s*>\s*(\w+)", src):
        attrs[m.group(2)] = int(m.group(1))
    vbl_attrs = {}
    for m in re.finditer(r"(\w+)\.attributeCount\s*=\s*(?:(\w+)\.size\(\)|(\d+))", src):
        vbl_attrs[m.group(1)] = attrs.get(m.group(2), 0) if m.group(2) else int(m.group(3))

    # ─── Pipelines carrying MORE THAN ONE vertex buffer pass an ARRAY of
    #     layouts rather than the address of a single one. Until TETRIS
    #     ORB_V every render pipeline in the program had exactly one, so
    #     the reader below only understood `&someVBL`; a `.data()` token
    #     resolved to nothing and the pipeline silently counted 0 buffers
    #     and 0 attributes. Map the array's NAME to (buffer count, summed
    #     attributes) so both shapes read.
    vbl_arrays = {}
    for m in re.finditer(
            r"std::array<\s*wgpu::VertexBufferLayout\s*,\s*(\d+)\s*>\s*(\w+)\s*\{\{([^}]*)\}\}",
            src, re.S):
        members = [x.strip() for x in m.group(3).split(",") if x.strip()]
        vbl_arrays[m.group(2)] = (int(m.group(1)),
                                  sum(vbl_attrs.get(x, 0) for x in members))

    def vbl_of(tok):
        tok = tok.strip()
        if tok == "nullptr":
            return 0, 0
        if tok.endswith(".data()"):
            n, va = vbl_arrays.get(tok[:-len(".data()")].strip(), (0, None))
            return n, va
        name = tok.lstrip("&")
        return 1, vbl_attrs.get(name, None)

    # ─── Render, through the two shared builders. makeEntity always uses
    #     `renderLayout` and Entry::ENTITY_FS; makeShadow uses
    #     `shadowRenderLayout` unless its trailing 8th argument overrides.
    for m in re.finditer(r"makeEntity\(([^;]*?)\)\)\s*return\s+false", src, re.S):
        a = [x.strip() for x in split_top_level(m.group(1), ",")]
        a[0] = a[0].lstrip("!")
        vb, va = vbl_of(a[3])
        pipelines.append(Pipeline(
            label=a[1].strip('"'), member=a[5], kind="render",
            vs_entry=entry_name(a[2]), fs_entry="ENTITY_FS",
            group_layouts=layouts_at("renderLayout", m.start()),
            vertex_buffer_count=vb, vertex_attribute_count=va, color_target_count=1,
            roster_gate=roster_gate_of(spans, m.start()), line=line_of(src, m.start()),
            off=m.start()))

    for m in re.finditer(r"makeShadow\(([^;]*?)\)\)\s*return\s+false", src, re.S):
        a = [x.strip() for x in split_top_level(m.group(1), ",")]
        a[0] = a[0].lstrip("!")
        vb, va = vbl_of(a[3])
        plvar = a[7] if len(a) > 7 else "shadowRenderLayout"
        pipelines.append(Pipeline(
            label=a[1].strip('"'), member=a[5], kind="render",
            vs_entry=entry_name(a[2]), fs_entry=None,       # depth-only: no FS
            group_layouts=layouts_at(plvar, m.start()),
            vertex_buffer_count=vb, vertex_attribute_count=va, color_target_count=0,
            roster_gate=roster_gate_of(spans, m.start()), line=line_of(src, m.start()),
            off=m.start()))

    # ─── Render, spelled out. Each block is delimited by its own
    #     RenderPipelineDescriptor; the two inside the shared builders are
    #     skipped, since their call sites are already recorded above.
    builder_spans = []
    for m in re.finditer(r"auto\s+(makeEntity|makeShadow)\s*=\s*\[&\]", src):
        b = src.find("{", m.end())
        depth, p = 0, b
        while p < len(src):
            if src[p] == "{":
                depth += 1
            elif src[p] == "}":
                depth -= 1
                if depth == 0:
                    break
            p += 1
        builder_spans.append((m.start(), p))

    for m in re.finditer(r"wgpu::RenderPipelineDescriptor\s+(\w+)\s*\{\s*\}\s*;", src):
        if any(s <= m.start() <= e for s, e in builder_spans):
            continue
        d = m.group(1)
        nxt = re.search(r"wgpu::(?:Render|Compute)PipelineDescriptor\s+\w+\s*\{\s*\}\s*;",
                        src[m.end():])
        stop = m.end() + (nxt.start() if nxt else len(src) - m.end())
        # AUBADE U3 — THE SPELLING CHANGED AND THE MEMBER MOVED. It was
        # `member = device_.CreateRenderPipeline(&desc)`; every render
        # pipeline in the program is now issued asynchronously through
        # `issue_render_("label", desc, member, PipeGroup::X)`. The member
        # is still the thing this arm is here to learn — it is now the
        # third argument rather than the assignment's left side. The
        # descriptor is passed BY REFERENCE, so there is no `&`.
        cm = re.search(r"issue_render_\(\s*\"[^\"]*\"\s*,\s*%s\s*,\s*(\w+)\s*,"
                       % re.escape(d), src[m.end():stop], re.S)
        if not cm:
            continue
        # The FragmentState, ColorTargetState and VertexBufferLayout a
        # descriptor points at are declared BEFORE it, in the same braced
        # scope — so the block to read is the enclosing scope, not the
        # descriptor's own tail.
        blk = src[enclosing_block_start(src, m.start()):m.end() + cm.end()]
        get = lambda f: (re.search(re.escape(d) + r"\." + f + r"\s*=\s*([^;]+);", blk) or [None, None])[1]
        fsvar = get(r"fragment")
        fs = None
        if fsvar:
            fm = re.search(re.escape(fsvar.strip().lstrip("&")) +
                           r"\.entryPoint\s*=\s*Entry::(\w+)\s*;", blk)
            fs = fm.group(1) if fm else None
        tc = None
        if fsvar:
            tm = re.search(re.escape(fsvar.strip().lstrip("&")) +
                           r"\.targetCount\s*=\s*(\d+)\s*;", blk)
            tc = int(tm.group(1)) if tm else None
        vbc = get(r"vertex\.bufferCount")
        vbuf = get(r"vertex\.buffers")
        va = 0
        vbn = 0
        if vbuf and vbuf.strip() not in ("nullptr",):
            vbn, va = vbl_of(vbuf.strip())
        # `bufferCount` is a literal on the single-buffer pipelines and
        # `<array>.size()` on the multi-buffer ones; the layout token is the
        # authority either way, so fall back to what vbl_of resolved.
        if vbc and vbc.strip().isdigit():
            vbn = int(vbc)
        pipelines.append(Pipeline(
            label=(get("label") or "").strip().strip('"'), member=cm.group(1), kind="render",
            vs_entry=entry_name(get(r"vertex\.entryPoint") or ""), fs_entry=fs,
            group_layouts=layouts_at((get("layout") or "").strip(), m.start()),
            vertex_buffer_count=vbn,
            vertex_attribute_count=va, color_target_count=tc if tc is not None else 0,
            roster_gate=roster_gate_of(spans, m.start()), line=line_of(src, m.start()),
            off=m.start()))

    pipelines.sort(key=lambda p: p.line)
    return pipelines


def enclosing_block_start(src, pos):
    """Offset just inside the innermost `{` enclosing `pos`."""
    depth, p = 0, pos
    while p > 0:
        p -= 1
        if src[p] == "}":
            depth += 1
        elif src[p] == "{":
            if depth == 0:
                return p + 1
            depth -= 1
    return 0


def pls_at(events, pos, handles):
    """Replay the pipeline-layout events up to `pos`. Exact, and cheap enough."""
    arrays, plds, pls = {}, {}, {}
    for p, kind, a, b, n in sorted(events):
        if p > pos:
            break
        if kind == "array":
            arrays[a] = b
        elif kind == "pld":
            plds[a] = b
        elif kind == "create":
            pls[a] = [handles.get(h, h) for h in arrays.get(plds.get(b, ""), [])]
        elif kind == "for":
            pls[a] = [handles.get(h, h) for h in b]
    return pls


def entry_name(tok):
    m = re.search(r"Entry::(\w+)", tok or "")
    return m.group(1) if m else None


def phase_0c(w, layouts, wgsl):
    src = strip_cpp_comments(read(RENDERER_HPP))
    spans = gate_map(src)
    handles = parse_layout_handles(w)
    by_member = {L["member"]: L for L in layouts}
    pipelines = parse_pipelines(w, src, spans, handles, by_member)

    # Entry:: constants -> the WGSL entry point names they carry verbatim.
    entry_const = {}
    for m in re.finditer(r'constexpr\s+const\s+char\*\s+(\w+)\s*=\s*"([^"]*)"\s*;', src):
        entry_const[m.group(1)] = m.group(2)

    # Every pipeline must resolve to at least one real bind group layout,
    # and every named layout must be one state.hpp actually creates.
    unresolved = ["%s -> %s" % (p.label, g) for p in pipelines
                  for g in p.group_layouts if g not in by_member]
    empty = [p.label for p in pipelines if not p.group_layouts]
    w.record("0c-0c", not unresolved and not empty,
             "every pipeline resolves to bind group layouts state.hpp creates"
             if not unresolved and not empty else
             "; ".join(filter(None, [
                 ("unknown layout: " + ", ".join(unresolved)) if unresolved else "",
                 ("no layouts resolved: " + ", ".join(empty)) if empty else ""])))

    # ─── WITNESS 0c-1 — at most 4 bind groups per pipeline layout.
    over = ["%s: %d" % (p.label, len(p.group_layouts)) for p in pipelines
            if len(p.group_layouts) > CORE_BIND_GROUPS]
    w.record("0c-1", not over,
             "max bind groups per pipeline layout: %d of %d"
             % (max((len(p.group_layouts) for p in pipelines), default=0), CORE_BIND_GROUPS)
             if not over else "over maxBindGroups: " + ", ".join(over))

    # ─── WITNESS 0c-2 — groups + vertex buffers ≤ 24.
    worst = max(((len(p.group_layouts) + p.vertex_buffer_count, p.label)
                 for p in pipelines), default=(0, "-"))
    over2 = ["%s: %d" % (p.label, len(p.group_layouts) + p.vertex_buffer_count)
             for p in pipelines
             if len(p.group_layouts) + p.vertex_buffer_count > CORE_GROUPS_PLUS_VBS]
    w.record("0c-2", not over2,
             "max bindGroups+vertexBuffers: %d of %d (%s)"
             % (worst[0], CORE_GROUPS_PLUS_VBS, worst[1])
             if not over2 else "over maxBindGroupsPlusVertexBuffers: " + ", ".join(over2))

    # ─── WITNESS 0c-3 — every Entry:: constant used is a real entry point
    #     in the 0b census, with a matching stage.
    problems = []
    for p in pipelines:
        for const, stage in p.entries():
            name = entry_const.get(const)
            if name is None:
                problems.append("%s: Entry::%s is not declared" % (p.label, const))
                continue
            fn = wgsl["entries"].get(name)
            if fn is None:
                problems.append("%s: Entry::%s -> \"%s\" is not an entry point in world.wgsl"
                                % (p.label, const, name))
            elif fn.stage != stage:
                problems.append("%s: \"%s\" used as %s but declared %s"
                                % (p.label, name, stage, fn.stage))
    used = {c for p in pipelines for c, _ in p.entries()}
    w.record("0c-3", not problems,
             "all %d Entry:: constants used by pipelines resolve to world.wgsl entry points "
             "with a matching stage" % len(used)
             if not problems else "; ".join(problems))

    # A layout used at two different group indices would break the
    # (group, binding) join in 0d outright.
    #
    # LOOM_2 A5 exemption: the EMPTY layout is a stratum FILLER — it
    # carries zero entries, so there is no (group, binding) join for a
    # second index to break. It stands wherever a pipeline leaves a
    # stratum unused, which is indices 1, 2 and 3 today. The exemption
    # is by PREDICATE, not by name: only entryCount == 0 qualifies, so
    # the witness ASSERTS the exempted layout's parsed row count rather
    # than trusting its label, and the exempted indices are reported.
    exempt = {L["member"] for L in layouts if len(L["entries"]) == 0}
    idx = {}
    clash = []
    exempt_seen = {}
    for p in pipelines:
        for i, g in enumerate(p.group_layouts):
            if g in exempt:
                exempt_seen.setdefault(g, set()).add(i)
                continue
            if idx.setdefault(g, i) != i:
                clash.append("%s at index %d and %d" % (g, idx[g], i))
    w.record("0c-4", not clash,
             "every bind group layout with entries is bound at ONE group index "
             "across all pipelines"
             + "".join("; %s exempt per A5 (0 entries) at indices %s"
                       % (g, ",".join(str(i) for i in sorted(ix)))
                       for g, ix in sorted(exempt_seen.items()))
             if not clash else "; ".join(sorted(set(clash))))

    return {"pipelines": pipelines, "entry_const": entry_const,
            "group_index": idx, "handles": handles}


# ═══════════════════════════════════════════════════════════════════════
# EXTENSION 3 — CALL-SHAPE CENSUS
#
# BUDGET_0 censused what the SHADER declares. This censuses what the HOST
# issues. They are different facts and they disagree: nine entry points
# carry @workgroup_size(1); seven dispatches issue a single workgroup.
# And a pipeline drawn with instanceCount 1 cannot host an instance-step
# vertex buffer whatever its access pattern says.
# ═══════════════════════════════════════════════════════════════════════

RENDER_PASSES_HPP = os.path.join(REAL, "render_passes.hpp")
DRAWABLE_TABLE_HPP = os.path.join(REAL, "drawable_table.hpp")


def caller_files():
    """Every file that invokes a renderer verb, discovered not hand-listed.

    The pass files are not the whole story: `draw_orbs` is called from
    bodies/orbs.hpp and `draw_patch_terrain_direct` from bodies/gallery.hpp.
    A hand-listed input set would have left both instance counts reading as
    a parameter name. Sorted, so the scan is deterministic.
    """
    out = []
    for root, _dirs, files in os.walk(os.path.join(REPO, "src")):
        for f in files:
            if not f.endswith((".hpp", ".cpp", ".h")):
                continue
            path = os.path.join(root, f)
            if os.path.abspath(path) == os.path.abspath(RENDERER_HPP):
                continue
            try:
                t = read(path)
            except Exception:
                continue
            if "renderer_." in t or "renderer_->" in t:
                out.append(path)
    return sorted(out)

DRAW_VARIANTS = ("DrawIndexedIndirect", "DrawIndexed", "DrawIndirect", "Draw")


def split_args(s):
    return [" ".join(a.split()) for a in split_top_level(s, ",", angle=False) if a.strip()]


def cpp_functions(src):
    """name -> (params, body_start, body_end) for `void`/`bool` members."""
    out = {}
    for m in re.finditer(r"\n\s+(?:void|bool)\s+(\w+)\s*\(", src):
        j = m.end() - 1
        depth, k = 0, j
        while k < len(src):
            if src[k] == "(":
                depth += 1
            elif src[k] == ")":
                depth -= 1
                if depth == 0:
                    break
            k += 1
        params = src[j + 1:k]
        b = src.find("{", k)
        if b < 0:
            continue
        depth, p = 0, b
        while p < len(src):
            if src[p] == "{":
                depth += 1
            elif src[p] == "}":
                depth -= 1
                if depth == 0:
                    break
            p += 1
        out[m.group(1)] = (params, b, p, j)
    return out


def param_names(params):
    return [re.sub(r".*?(\w+)\s*(?:=.*)?$", r"\1", a.strip())
            for a in split_args(params) if a.strip()]


def phase_ext3(w, cen, wgsl):
    rend = strip_cpp_comments(read(RENDERER_HPP))
    callers = caller_files()
    caller_src = {p: strip_cpp_comments(read(p)) for p in callers}
    passes = caller_src.get(RENDER_PASSES_HPP, "")
    table = caller_src.get(DRAWABLE_TABLE_HPP, "")
    fns = cpp_functions(rend)

    # ─── Per renderer function: the pipelines it sets and the calls it
    #     issues, in source order. A call takes the most recent
    #     SetPipeline in the same function.
    ev = re.compile(r"SetPipeline\(\s*([\w:]+)\s*\)"
                    r"|\.(" + "|".join(DRAW_VARIANTS) + r")\("
                    r"|DispatchWorkgroups\(")
    info = {}
    for name, (params, b, e, _decl) in fns.items():
        body = rend[b:e]
        pnames = set(param_names(params))
        cur, sets, calls = None, [], []
        for m in ev.finditer(body):
            if m.group(1):
                cur = m.group(1)
                sets.append(cur)
                continue
            args = split_args(balanced(body, m.end() - 1))
            calls.append({"kind": m.group(2) or "DispatchWorkgroups",
                          "args": args, "pipeline": cur,
                          "param_pipeline": cur in pnames if cur else False})
        info[name] = {"params": params, "pnames": pnames, "sets": sets,
                      "calls": calls, "body": body}

    # ─── W3-0 — every Draw*/Dispatch call site in renderer.hpp lands
    #     inside a function the parser recognised. A call outside one is
    #     a call shape the census would silently lose.
    total = len(re.findall(r"\.(?:" + "|".join(DRAW_VARIANTS) + r")\(", rend)) \
        + len(re.findall(r"DispatchWorkgroups\(", rend))
    seen = sum(len(i["calls"]) for i in info.values())
    w.record("W3-0", total == seen,
             "renderer.hpp: %d Draw*/DispatchWorkgroups call sites, all inside a parsed "
             "function" % total if total == seen else
             "%d call sites, only %d inside a parsed function" % (total, seen))

    # ─── A count spelled as a parameter name is not a count. Resolve it
    #     to the distinct arguments the callers pass — that IS "its
    #     source", and it is what decides whether a pipeline is ever
    #     drawn instanced.
    everywhere = rend + "".join("\n" + caller_src[p] for p in callers)

    def sig(fnname):
        raw = split_args(fns[fnname][0])
        names, defaults = [], []
        for a in raw:
            d = a.split("=", 1)
            names.append(re.sub(r"^.*?(\w+)$", r"\1", d[0].strip()))
            defaults.append(d[1].strip() if len(d) > 1 else None)
        return names, defaults

    def bind_args(fnname, args):
        """Helper parameter name -> the expression THIS caller passes."""
        names, defaults = sig(fnname)
        out = {}
        for i, n in enumerate(names):
            out[n] = args[i] if i < len(args) else (defaults[i] or "1")
        return out

    def resolve_param(fnname, pname):
        if fnname not in fns:
            return None
        raw = split_args(fns[fnname][0])
        names, defaults = [], []
        for a in raw:
            d = a.split("=", 1)
            names.append(re.sub(r".*?(\w+)$", r"\1", d[0].strip()))
            defaults.append(d[1].strip() if len(d) > 1 else None)
        if pname not in names:
            return None
        idx = names.index(pname)
        decl = fns[fnname][3]
        found = set()
        for cm in re.finditer(r"(?<![\w])%s\s*\(" % re.escape(fnname), everywhere):
            if cm.end() - 1 == decl:
                continue                      # the definition, not a call
            args = split_args(balanced(everywhere, cm.end() - 1))
            found.add(args[idx] if len(args) > idx else (defaults[idx] or "1"))
        found.discard(pname)
        return sorted(found) or None

    # ─── Resolve the two cross-function shapes.
    #     (a) a helper that sets a pipeline given as a PARAMETER — resolve
    #         at its call sites.
    #     (b) a function that draws without setting — the pipeline was set
    #         by a sibling call at the pass head. Replay render_passes.hpp
    #         linearly to recover it.
    shapes = {}                      # pipeline member -> [shape dicts]

    def add(pipeline, c, where, extra="", fn=""):
        shapes.setdefault(pipeline, []).append(
            {"variant": c["kind"], "args": c["args"], "where": where,
             "note": extra, "fn": fn, "bound": c.get("bound")})

    for name, i in info.items():
        for c in i["calls"]:
            if c["pipeline"] and not c["param_pipeline"]:
                add(c["pipeline"], c, name, fn=name)
            elif c["param_pipeline"]:
                # (a) every caller supplies the pipeline.
                pidx = param_names(i["params"]).index(c["pipeline"])
                for cm in re.finditer(r"\b%s\s*\(" % re.escape(name), rend):
                    if b_in_fn(fns, cm.start()) == name:
                        continue
                    a = split_args(balanced(rend, cm.end() - 1))
                    if len(a) > pidx:
                        sh = dict(c)
                        sh["bound"] = bind_args(name, a)
                        add(a[pidx], sh, "%s via %s" % (b_in_fn(fns, cm.start()), name),
                            "pipeline and counts supplied by caller", fn=name)

    # (b) replay the pass files for draws whose pipeline was set elsewhere.
    setters = {n: i["sets"][-1] for n, i in info.items() if i["sets"] and not i["calls"]}
    drawless = {n for n, i in info.items() if i["calls"] and not i["sets"]
                and not any(c["param_pipeline"] for c in i["calls"])}
    for path in callers:
        text, label = caller_src[path], os.path.relpath(path, REPO)
        cur = None
        pat = re.compile(r"(?:renderer_?\.|r\.)(\w+)\s*\(|pass\.SetPipeline\(\s*([\w:]+)"
                         r"|pass\.(" + "|".join(DRAW_VARIANTS) + r")\(")
        for m in pat.finditer(text):
            if m.group(2):
                cur = m.group(2)
            elif m.group(3):
                if cur:
                    add(cur, {"kind": m.group(3),
                              "args": split_args(balanced(text, m.end() - 1))},
                        label, "raw draw at the pass head", fn="")
            else:
                fn = m.group(1)
                if fn in setters:
                    cur = setters[fn]
                elif fn in drawless:
                    if cur:
                        for c in info[fn]["calls"]:
                            add(cur, c, "%s via %s" % (label, fn),
                                "pipeline set by the pass head", fn=fn)
                elif fn in info and info[fn]["sets"]:
                    cur = info[fn]["sets"][-1]

    # ─── Resolve instanceCount / workgroup counts, and their source.
    def count_of(sh, fnname):
        v = sh["variant"]
        if v in ("DrawIndirect", "DrawIndexedIndirect"):
            return "indirect", "from the indirect args buffer"
        idx = 1 if v in ("Draw", "DrawIndexed") else None
        if idx is None or len(sh["args"]) <= idx:
            return "1", "default (argument omitted)"
        raw = sh["args"][idx]
        raw = re.sub(r"/\*.*?\*/", "", raw).strip()
        if sh.get("bound") and raw in sh["bound"]:
            return sh["bound"][raw], "argument at the call site"
        if re.fullmatch(r"\w+", raw) and sh.get("fn"):
            res = resolve_param(sh["fn"], raw)
            if res:
                return " | ".join(res), "parameter `%s` of %s, resolved at its call sites" % (
                    raw, sh["fn"])
        return raw, ""

    rows = []
    for p in cen["pipelines"]:
        got = shapes.get(p.member, [])
        seen_shapes = []
        for sh in got:
            if p.kind == "compute":
                cnt = " x ".join(sh["args"]) if sh["args"] else "?"
                src = ""
            else:
                cnt, src = count_of(sh, p.member)
            key = (sh["variant"], cnt)
            if key not in [(x["variant"], x["count"]) for x in seen_shapes]:
                seen_shapes.append({"variant": sh["variant"], "count": cnt,
                                    "src": src, "where": sh["where"], "note": sh["note"]})
        rows.append({"pipeline": p, "shapes": seen_shapes})

    # ─── W3-1 — every pipeline is ACCOUNTED FOR: exactly one call shape,
    #     or recorded as never invoked, or recorded with all its shapes.
    #     Picking one and hiding the rest is the failure this forbids.
    none_ = [r["pipeline"].label for r in rows if not r["shapes"]]
    many = [(r["pipeline"].label, r["shapes"]) for r in rows if len(r["shapes"]) > 1]
    w.record("W3-1", True,
             "%d pipelines: %d with exactly one call shape, %d with several (all listed), "
             "%d never invoked%s"
             % (len(rows), sum(1 for r in rows if len(r["shapes"]) == 1), len(many),
                len(none_), (" — " + ", ".join(none_)) if none_ else ""))

    # ─── W3-3 — no instance count is left reading as a PARAMETER NAME.
    #     A count spelled `instanceCount` is the census failing to find the
    #     call site, not a fact about the program — and it is exactly what
    #     a hand-listed input set produced before the caller scan went wide.
    unresolved = []
    for r in rows:
        p_ = r["pipeline"]
        if p_.kind != "render":
            continue
        for sh in r["shapes"]:
            fnm = sh.get("fn") or ""
            if fnm in fns and re.fullmatch(r"\w+", sh["count"]) \
                    and sh["count"] in sig(fnm)[0]:
                unresolved.append("%s: %s" % (p_.label, sh["count"]))
    w.record("W3-3", not unresolved,
             "every render pipeline's instanceCount resolves to a literal, a named "
             "constant or a call-site expression — none is left as a parameter name "
             "(%d caller files scanned)" % len(callers)
             if not unresolved else "unresolved counts: " + "; ".join(unresolved))

    # ─── W3-2 — the two single-thread counts, and the per-row
    #     reconciliation between them.
    wg1 = sorted(n for n, f in wgsl["entries"].items()
                 if f.stage == "C" and re.fullmatch(r"\(\s*1\s*(,\s*1\s*)*\)",
                                                    f.workgroup_size or ""))
    single = []
    for r in rows:
        p = r["pipeline"]
        if p.kind != "compute":
            continue
        ep = cen["entry_const"].get(p.cs_entry)
        for sh in r["shapes"]:
            if re.fullmatch(r"1( x 1)*", sh["count"]):
                single.append(ep)
    single = sorted(set(single))
    recon = []
    for ep in sorted(set(wg1) | set(single)):
        recon.append((ep, ep in wg1, ep in single))
    w.record("W3-2", True,
             "@workgroup_size(1) entry points: %d (%s). Dispatches issuing ONE workgroup: "
             "%d (%s). The %d that differ: %s."
             % (len(wg1), ", ".join(wg1), len(single), ", ".join(single),
                len([r for r in recon if r[1] != r[2]]),
                ", ".join("%s (wg1=%s, single-dispatch=%s)" % (a, b_, c_)
                          for a, b_, c_ in recon if b_ != c_) or "none"))

    return {"rows": rows, "wg1": wg1, "single_dispatch": single, "recon": recon,
            "never_invoked": none_, "multi": many, "callers": callers}


def balanced(s, open_pos):
    """Text inside the parenthesis group that opens at `open_pos`."""
    depth, k = 0, open_pos
    while k < len(s):
        if s[k] == "(":
            depth += 1
        elif s[k] == ")":
            depth -= 1
            if depth == 0:
                return s[open_pos + 1:k]
        k += 1
    return ""


def b_in_fn(fns, pos):
    for n, (_, b, e, _d) in fns.items():
        if b <= pos <= e:
            return n
    return ""


# ═══════════════════════════════════════════════════════════════════════
# PHASE 0d — THE JOIN, THE STAGE BUDGET, THE TIER A PREDICATES
# ═══════════════════════════════════════════════════════════════════════

def demo_column(w):
    """Which demo column this census is taken under.

    The binding surface turns out not to vary with it (no bind group layout
    creation is ROSTER-gated), but the PIPELINE set does, and every A3 row
    has to say which column it was censused under or the flag could be a
    false positive from a pipeline that column never creates.
    """
    demo = read(os.path.join(REPO, "src", "cartridges", "the_board", "demos", "demo.hpp"))
    m = re.search(r"#\s*define\s+INCUBATE_DEMO\s+(\w+)", demo)
    default = m.group(1) if m else "?"
    cml = read(os.path.join(REPO, "CMakeLists.txt"))
    cm = re.search(r'set\(THE_BOARD_DEMO\s+"(\w+)"', cml)
    cmake = cm.group(1) if cm else "?"
    w.record("0d-0", default == cmake,
             "demo column: demo.hpp default `%s`, CMake THE_BOARD_DEMO default `%s`"
             % (default, cmake))
    return default


def compatible(entry, decl):
    """Can this WGSL declaration satisfy this layout entry?

    WebGPU requires each shader declaration's access mode to match its
    layout entry exactly — the reason orbCopyLayout_ exists at all. This
    matters on the three ALIASED slots, where two declarations share one
    (group, binding) and only one of them is legal against a given entry:
    (0, 2) carries `vp_data` as read_write and `fc_vp` as read.
    """
    if entry.kind == "buffer":
        if entry.access == "Uniform":
            return decl.address_space == "uniform"
        want = "read_write" if entry.access == "Storage" else "read"
        return decl.address_space == "storage" and decl.wgsl_access == want
    if entry.kind == "sampler":
        return decl.wgsl_type.startswith("sampler")
    if entry.kind == "storageTexture":
        return decl.wgsl_type.startswith("texture_storage")
    if entry.kind == "texture":
        return (decl.wgsl_type.startswith("texture_")
                and not decl.wgsl_type.startswith("texture_storage"))
    return False


def slot_category(row):
    """Which of the five per-stage budgets this row spends."""
    if row.kind == "buffer":
        return "uniform" if row.access == "Uniform" else "storage"
    if row.kind == "texture":
        return "sampled"
    if row.kind == "sampler":
        return "samplers"
    if row.kind == "storageTexture":
        return "storagetex"
    return None


def phase_0d(w, layouts, wgsl, cen):
    pipelines = cen["pipelines"]
    gindex = cen["group_index"]
    entry_const = cen["entry_const"]
    by_member = {L["member"]: L for L in layouts}

    # (group, binding) -> the WGSL declarations at that slot.
    by_slot = {}
    for d in wgsl["decls"]:
        by_slot.setdefault((d.group, d.binding), []).append(d)

    # Which pipelines bind each layout, and at which index.
    binders = {}
    for p in pipelines:
        for i, g in enumerate(p.group_layouts):
            binders.setdefault(g, []).append((p, i))

    # ─── 0d-i — resolve vis_actual, row by row. ───────────────────────
    rows = []
    for L in layouts:
        gi = gindex.get(L["member"])
        for e in L["entries"]:
            at_slot = by_slot.get((gi, e.binding_number), [])
            decls = [x for x in at_slot if compatible(e, x)]
            syms = {d.symbol for d in decls}
            actual, reached_by = set(), []
            for p, _ in binders.get(L["member"], []):
                for const, stage in p.entries():
                    ep = entry_const.get(const)
                    fn_reach = wgsl["reach"].get(ep)
                    if not fn_reach:
                        continue
                    if syms & fn_reach[1]:
                        actual.add(stage)
                        reached_by.append("%s/%s" % (p.label, ep))
            vis_actual = "".join(s for s in STAGES if s in actual)
            vis_delta = "".join(s for s in e.vis_declared if s not in actual)
            # The registry states that its names deliberately equal the
            # WGSL names, so a matched pair whose names differ is a STALE
            # MIRROR — a finding, not a match failure.
            const_name = e.binding_const.rsplit("::", 1)[1]
            slot_syms = {x.symbol for x in at_slot}
            mirror = "ok" if const_name in slot_syms else (
                "no WGSL declaration" if not slot_syms else
                "registry says %s, WGSL says %s" % (const_name, "/".join(sorted(slot_syms))))
            rows.append({
                "e": e, "layout": L, "group": gi,
                "decls": decls, "at_slot": at_slot, "symbols": sorted(syms),
                # On an aliased slot both names are the same slot; show the
                # one the registry constant is spelled with.
                "primary": (const_name if const_name in syms
                            else (sorted(syms)[0] if syms else "—")),
                "vis_actual": vis_actual, "vis_delta": vis_delta,
                "mirror": mirror, "reached_by": sorted(set(reached_by)),
                "binders": [p.label for p, _ in binders.get(L["member"], [])],
            })

    # REPORT — a layout entry whose (group, binding) has no WGSL declaration.
    orphan_rows = [r for r in rows if not r["at_slot"]]

    # A layout entry with declarations at its slot but none the entry can
    # legally satisfy would be rejected at pipeline creation. Finding one
    # means the join is wrong, not the program.
    mismatch = ["%s entries[%d] %s wants %s/%s; slot (@group(%s), @binding(%s)) declares %s"
                % (r["layout"]["label"], r["e"].entry_index, r["e"].binding_const,
                   r["e"].kind, r["e"].access, r["group"], r["e"].binding_number,
                   ", ".join("%s: %s %s" % (x.symbol, x.address_space, x.wgsl_access)
                             for x in r["at_slot"]))
                for r in rows if r["at_slot"] and not r["decls"]]
    w.record("0d-3", not mismatch,
             "every layout entry has at least one access-compatible WGSL declaration at its "
             "(group, binding)" if not mismatch else "; ".join(mismatch))
    # REPORT — a WGSL declaration with no layout entry in any pipeline that
    # binds its group.
    covered = {(r["group"], r["e"].binding_number) for r in rows}
    orphan_decls = [d for d in wgsl["decls"] if (d.group, d.binding) not in covered]
    # REPORT — a registry constant with no WGSL mirror.
    ns_group = {"g0": 0, "g1": 1, "g2": 2, "g3": 3}
    registry = parse_registry(Witnesses())
    orphan_consts = [(ns, n, v) for (ns, n), v in sorted(registry.items())
                     if (ns_group[ns], v) not in by_slot]

    # ─── 0d-ii — the stage budget. ────────────────────────────────────
    row_of = {}
    for r in rows:
        row_of.setdefault(r["layout"]["member"], []).append(r)

    budget = []
    for p in pipelines:
        stages = sorted({s for _, s in p.entries()}, key=STAGES.index)
        reach_here = set()
        for const, _ in p.entries():
            ep = entry_const.get(const)
            if ep in wgsl["reach"]:
                reach_here |= wgsl["reach"][ep][1]
        per_stage_reach = {}
        for const, stage in p.entries():
            ep = entry_const.get(const)
            per_stage_reach.setdefault(stage, set())
            if ep in wgsl["reach"]:
                per_stage_reach[stage] |= wgsl["reach"][ep][1]
        for stage in stages:
            dec = dict.fromkeys(CORE, 0)
            act = dict.fromkeys(CORE, 0)
            hit = dict.fromkeys(CORE, 0)
            detail = {k: [] for k in CORE}
            for g in p.group_layouts:
                for r in row_of.get(g, []):
                    cat = slot_category(r["e"])
                    if cat is None:
                        continue
                    if stage in r["e"].vis_declared:
                        dec[cat] += 1
                        detail[cat].append("%s[%d] %s" % (r["layout"]["label"],
                                                          r["e"].entry_index, r["primary"]))
                    if stage in r["vis_actual"]:
                        act[cat] += 1
                    if set(r["symbols"]) & per_stage_reach.get(stage, set()):
                        hit[cat] += 1
            budget.append({"pipeline": p, "stage": stage, "declared": dec,
                           "actual": act, "reached": hit, "detail": detail,
                           "dyn_uniform": sum(1 for g in p.group_layouts
                                              for r in row_of.get(g, [])
                                              if r["e"].has_dynamic_offset
                                              and r["e"].access == "Uniform"),
                           "dyn_storage": sum(1 for g in p.group_layouts
                                              for r in row_of.get(g, [])
                                              if r["e"].has_dynamic_offset
                                              and r["e"].access != "Uniform")})

    # ─── THE RECONCILIATION GATE. The web twin boots on a pure-defaults
    #     device request (PORT_5d), which is a live external witness that
    #     the real program fits inside the Core defaults. So every
    #     (pipeline, stage) row counted against vis_declared MUST fit. If
    #     one does not, the ledger is wrong — not the program.
    over = []
    for b in budget:
        for cat, limit in CORE.items():
            if b["declared"][cat] > limit:
                over.append("%s/%s %s %d>%d [%s]"
                            % (b["pipeline"].label, b["stage"], cat,
                               b["declared"][cat], limit, "; ".join(b["detail"][cat])))
    tight = max(budget, key=lambda b: max(b["declared"][c] / CORE[c] for c in CORE))
    tight_cat = max(CORE, key=lambda c: tight["declared"][c] / CORE[c])
    w.record("gate", not over,
             "every (pipeline, stage) row fits the Core defaults; tightest is "
             "%s / %s at %s %d of %d"
             % (tight["pipeline"].label, tight["stage"], tight_cat,
                tight["declared"][tight_cat], CORE[tight_cat])
             if not over else "OVER Core defaults: " + " | ".join(over))

    over_dyn = ["%s dyn_uniform %d>%d" % (b["pipeline"].label, b["dyn_uniform"], CORE_DYN_UNIFORM)
                for b in budget if b["dyn_uniform"] > CORE_DYN_UNIFORM]
    over_dyn += ["%s dyn_storage %d>%d" % (b["pipeline"].label, b["dyn_storage"], CORE_DYN_STORAGE)
                 for b in budget if b["dyn_storage"] > CORE_DYN_STORAGE]
    w.record("0d-1", not over_dyn,
             "dynamic-offset bindings: %d of %d uniform, %d of %d storage, program-wide"
             % (max((b["dyn_uniform"] for b in budget), default=0), CORE_DYN_UNIFORM,
                max((b["dyn_storage"] for b in budget), default=0), CORE_DYN_STORAGE)
             if not over_dyn else "; ".join(over_dyn))

    # A row's vis_actual can never exceed its vis_declared: a stage that
    # reaches a binding its layout does not expose to that stage would be a
    # pipeline-creation error, and finding one means the join is wrong.
    impossible = ["%s[%d] declares %s but reaches %s"
                  % (r["layout"]["label"], r["e"].entry_index,
                     r["e"].vis_declared or "-", r["vis_actual"])
                  for r in rows if set(r["vis_actual"]) - set(r["e"].vis_declared)]
    w.record("0d-2", not impossible,
             "no row is reached by a stage its visibility mask excludes"
             if not impossible else "; ".join(impossible))

    # ─── 0d-iii — the Tier A predicates. Flags, not a plan. ───────────
    a1 = [r for r in rows if r["vis_delta"]]

    a2 = []
    for r in rows:
        e = r["e"]
        if e.access != "ReadOnlyStorage":
            continue
        d = r["decls"][0] if r["decls"] else None
        ev = {
            "access": e.access == "ReadOnlyStorage",
            "wgsl_access": bool(d) and d.wgsl_access == "read",
            "not_runtime": bool(d) and not d.has_runtime_array,
            "bytes": bool(d) and d.layout.size is not None
                     and d.layout.size <= UNIFORM_BINDING_MAX_BYTES,
            "uniform_legal": bool(d) and not d.layout.uniform_blockers,
        }
        a2.append({"row": r, "decl": d, "ev": ev, "pass": all(ev.values()),
                   "element_note": uniform_element_note(d.layout) if d else "no WGSL declaration",
                   "size": d.layout.size if d else None})

    reached_any = set()
    for _, refs in wgsl["reach"].values():
        reached_any |= refs
    a3 = []
    for ns, n, v in orphan_consts:
        a3.append(("registry constant with no WGSL mirror",
                   "bind::%s::%s = %d" % (ns, n, v), ""))
    for d in wgsl["decls"]:
        if d.symbol not in reached_any:
            a3.append(("WGSL declaration no entry point reaches",
                       "%s @group(%d) @binding(%d)" % (d.symbol, d.group, d.binding), ""))
    for d in orphan_decls:
        a3.append(("WGSL declaration with no layout entry in any pipeline binding its group",
                   "%s @group(%d) @binding(%d)" % (d.symbol, d.group, d.binding), ""))
    for r in orphan_rows:
        a3.append(("layout entry whose (group, binding) has no WGSL declaration",
                   "%s entries[%d] %s -> (@group(%s), @binding(%s))"
                   % (r["layout"]["label"], r["e"].entry_index, r["e"].binding_const,
                      r["group"], r["e"].binding_number), ""))
    for r in rows:
        if not r["vis_actual"] and r["decls"]:
            a3.append(("layout entry no reachable code touches in any pipeline that binds it",
                       "%s entries[%d] %s (declares %s)"
                       % (r["layout"]["label"], r["e"].entry_index,
                          r["symbols"][0] if r["symbols"] else "?", r["e"].vis_declared), ""))

    stale = [r for r in rows if r["mirror"] not in ("ok",) and r["decls"]]

    return {"rows": rows, "budget": budget, "a1": a1, "a2": a2, "a3": a3,
            "stale_mirror": stale, "orphan_rows": orphan_rows,
            "orphan_decls": orphan_decls, "orphan_consts": orphan_consts,
            "tightest": (tight, tight_cat)}


# ═══════════════════════════════════════════════════════════════════════
# EXTENSION 4 — THE DEFENDED-SITE INDEX
#
# The program's expensive lessons live in prose, because prose is where a
# measurement's REASON can live. A census cannot hold reasons. It can hold
# a pointer to where they are.
#
# Emit the SYMBOL and the matched trigger tokens — never the prose. A
# reference outlives its referent; a copy goes stale and gives one fact
# two homes.
# ═══════════════════════════════════════════════════════════════════════

# W4-1: this list is the predicate, and it is emitted verbatim into the
# artifact so it can be audited rather than trusted. Prefer FALSE
# POSITIVES — over-flagging costs a glance, under-flagging costs a
# 48-second lesson relearned.
TRIGGERS = [
    ("time-cost",
     r"\b\d+(?:\.\d+)?\s*(?:ms|s|sec|secs|second|seconds|min|minute|minutes)\b"),
    ("FXC", r"\bFXC\b"),
    ("law-ref", r"\bL\d+\b|\b[A-Z_]{3,} LAW\b|docs/LAWS\.md"),
    ("measured", r"\bmeasured\b|\bmeasurement\b"),
    ("witness", r"\bwitness\b"),
    ("hangs", r"\bhangs\b"),
    ("compile-time", r"\bcompile[ -]time\b"),
    ("landed-at", r"\blanded at\b"),
    ("regressed", r"\bregressed\b"),
    ("budget", r"\bbudget\b"),
    # ─── ADDED beyond the handoff's list, and reported as such. Without
    #     these two, W4-2's positive control MISSES the state.hpp
    #     entries[16]/[17] comments: they say "per-stage storage buffer
    #     cap" and "the VS storage-buffer cap is full", neither of which
    #     is the literal string "per-stage limit".
    ("per-stage", r"\bper[- ]stage\b"),
    ("slot-cap", r"\b(?:storage|uniform|binding|slot)[- ]?(?:buffer[- ]?)?(?:cap|limit)\b"),
]


def comment_blocks(src, wgsl=False):
    """Contiguous runs of `//` lines, and `/* … */` blocks, with spans."""
    blocks, i, n = [], 0, len(src)
    while i < n:
        if src.startswith("//", i):
            start = i
            while i < n:
                eol = src.find("\n", i)
                if eol < 0:
                    eol = n
                i = eol + 1
                nxt = i
                while nxt < n and src[nxt] in " \t":
                    nxt += 1
                if not src.startswith("//", nxt):
                    break
                i = nxt
            blocks.append((start, i, src[start:i]))
            continue
        if src.startswith("/*", i):
            e = src.find("*/", i + 2)
            e = n if e < 0 else e + 2
            blocks.append((i, e, src[i:e]))
            i = e
            continue
        j = src.find("//", i)
        k = src.find("/*", i)
        cand = [x for x in (j, k) if x >= 0]
        if not cand:
            break
        i = min(cand)
    return blocks


def trigger_hits(text):
    return sorted({name for name, pat in TRIGGERS
                   if re.search(pat, text, re.I if name != "law-ref" else 0)})


class Defended:
    __slots__ = ("symbol", "key", "kind", "file", "line", "triggers", "rules")

    def __init__(self, **kw):
        for k in self.__slots__:
            setattr(self, k, kw.get(k))


def attach_and_scan(sites, blocks, src, path):
    """Attach comment blocks to declaration sites, then trigger-match.

    Two rules, both conservative:
      A — PROXIMITY. The nearest preceding block with no other block
          between it and the site. A block therefore covers the RUN of
          declarations that follows it, which is what recovers
          `pawn_ground_resolve` — its banner sits above `slope_passable`,
          a small function that was inserted between the two.
      B — NAMING. Any block anywhere that carries a trigger AND names the
          symbol as a whole word. This is what recovers
          `update_player_agent` and `update_other_agents`: the 48-second
          kernel-split banner names both, several declarations upstream
          and behind three intervening comment blocks.
    """
    starts = [b[0] for b in blocks]
    named = [(b, trigger_hits(b[2])) for b in blocks]
    out = []
    for s in sites:
        o = s["off"]
        text, rules = [], []
        prev = None
        for b in blocks:
            if b[1] <= o:
                prev = b
            else:
                break
        if prev is not None and not any(prev[1] < st < o for st in starts):
            text.append(prev[2])
            rules.append("A:proximity")
        for b, hits in named:
            if hits and re.search(r"(?<![\w])%s(?![\w])" % re.escape(s["symbol"]), b[2]):
                if b[2] not in text:
                    text.append(b[2])
                    rules.append("B:named")
        for lo, hi in s.get("inner", []):
            for b in blocks:
                if lo <= b[0] and b[1] <= hi and b[2] not in text:
                    text.append(b[2])
                    rules.append("C:body")
        hits = trigger_hits("\n".join(text))
        if hits:
            out.append(Defended(symbol=s["symbol"], key=s.get("key", s["symbol"]),
                                kind=s["kind"], file=os.path.relpath(path, REPO),
                                line=line_of(src, o), triggers=hits,
                                rules=sorted(set(rules))))
    return out


def phase_ext4(w, wgsl, layouts, cen):
    found = []

    # ─── The four hashed inputs, plus their file banners.
    for path in (WORLD_WGSL, STATE_HPP, RENDERER_HPP, REGISTRY_HPP):
        raw = read(path)
        blocks = comment_blocks(raw)
        if blocks and blocks[0][0] < 400:
            hits = trigger_hits(blocks[0][2])
            if hits:
                found.append(Defended(symbol="(file banner)", key="(file banner)",
                                      kind="file", file=os.path.relpath(path, REPO),
                                      line=1, triggers=hits, rules=["banner"]))

    # ─── world.wgsl: binding declarations and every function.
    wraw = read(WORLD_WGSL)
    wblocks = comment_blocks(wraw)
    sites = []
    for d in wgsl["decls"]:
        m = re.search(r"@group\(%d\)\s*@binding\(%d\)\s*var[^;]*?\b%s\b"
                      % (d.group, d.binding, re.escape(d.symbol)), wraw)
        if m:
            sites.append({"symbol": d.symbol, "kind": "wgsl binding", "off": m.start()})
    for name, fn in wgsl["fns"].items():
        m = re.search(r"\bfn\s+%s\s*\(" % re.escape(name), wraw)
        if not m:
            continue
        b = wraw.find("{", m.end())
        depth, p = 0, b
        while p < len(wraw) and b >= 0:
            if wraw[p] == "{":
                depth += 1
            elif wraw[p] == "}":
                depth -= 1
                if depth == 0:
                    break
            p += 1
        sites.append({"symbol": name,
                      "kind": "wgsl entry point" if fn.stage else "wgsl function",
                      "off": m.start(), "inner": [(b, p)] if b >= 0 else []})
    found += attach_and_scan(sites, wblocks, wraw, WORLD_WGSL)

    # ─── state.hpp: each layout creation block, and each layout ENTRY.
    #     Entry granularity is required — the entries[16]/[17] comments
    #     sit inside a block, attached to individual entries.
    sraw = read(STATE_HPP)
    sblocks = comment_blocks(sraw)
    sites = []
    for L in layouts:
        m = re.search(r'desc\.label\s*=\s*"%s"' % re.escape(L["label"]), sraw)
        if m:
            sites.append({"symbol": L["label"], "kind": "layout", "off": m.start()})
        for e in L["entries"]:
            em = re.search(r"entries\[%d\]\.binding\s*=\s*%s\s*;"
                           % (e.entry_index, re.escape(e.binding_const)), sraw)
            if em:
                sites.append({"symbol": "%s entries[%d]" % (L["label"], e.entry_index),
                              "key": "%s in %s" % (e.binding_const, L["label"]),
                              "kind": "layout entry", "off": em.start()})
    found += attach_and_scan(sites, sblocks, sraw, STATE_HPP)

    # ─── binding_registry.hpp: each constant.
    graw = read(REGISTRY_HPP)
    gblocks = comment_blocks(graw)
    gsites = []
    for gm in re.finditer(r"inline\s+constexpr\s+uint32_t\s+(\w+)\s*=", graw):
        gsites.append({"symbol": gm.group(1), "kind": "registry constant",
                       "off": gm.start()})
    found += attach_and_scan(gsites, gblocks, graw, REGISTRY_HPP)

    # ─── renderer.hpp: each pipeline creation site.
    rraw = read(RENDERER_HPP)
    rblocks = comment_blocks(rraw)
    sites = []
    for p in cen["pipelines"]:
        sites.append({"symbol": p.member, "kind": "pipeline", "off": p.off})
    found += attach_and_scan(sites, rblocks, rraw, RENDERER_HPP)

    # ─── W4-1 — the trigger list is emitted verbatim, so the predicate is
    #     auditable rather than trusted.
    w.record("W4-1", True,
             "%d trigger tokens, emitted verbatim into the artifact: %s"
             % (len(TRIGGERS), ", ".join(n for n, _ in TRIGGERS)))

    # The control set, defined ONCE — and NOT here (LOOM_3). W4-3's overfit
    # guard and W4-2's positive control both read it from
    # binding_schema.py's DEFENDED_SITES, which is the schema's job: the
    # one home for facts about the binding surface (L22). Three campaigns
    # re-keyed this list while it lived in the instrument, each forcing an
    # instrument commit to keep a witness green; a key that lives in the
    # schema is edited by the campaign that moves its subject, in the file
    # that campaign already has open. The schema's banner carries the
    # history and the reasoning.
    want = [(row["key"], row["file"]) for row in schema.DEFENDED_SITES]

    # ─── W4-3 — the guard on W4-2. Two triggers were ADDED to make the
    #     control pass, which is what a control is for — but an instrument
    #     that can grant the variable under test is not an instrument.
    #     Per-token site counts make overfitting visible rather than
    #     inferred: a token contributing exactly one site, that site being
    #     a control, is a token written to pass the test.
    per_token = {}
    for f in found:
        for t in f.triggers:
            per_token.setdefault(t, []).append(f)
    controls = {k for k, _ in want}
    overfit = []
    for t, fs in per_token.items():
        if len(fs) == 1 and fs[0].key in controls:
            overfit.append("%s: 1 site, and it is a control (%s)" % (t, fs[0].symbol))
    w.record("W4-3", not overfit,
             "no trigger is overfitted to the control — site counts: "
             + ", ".join("%s %d (sole trigger at %d)"
                         % (t, len(per_token.get(t, [])),
                            sum(1 for f in per_token.get(t, []) if f.triggers == [t]))
                         for t, _ in TRIGGERS)
             if not overfit else "; ".join(overfit))

    # ─── W4-2 — the POSITIVE CONTROL. An instrument that cannot find what
    #     we already know is there is not an instrument.
    # BUDGET_1 re-key: the two state.hpp sites were named
    # `Render Entity Layout entries[16]` and `entries[17]`. Row 7 removed
    # entries[15] and renumbered the survivors, and the control failed —
    # not because the instrument stopped finding the prose, but because
    # the CONTROL was itself a reference that outlived its referent. An
    # index is a position; a binding is an identity. Keyed by binding, the
    # control asserts what it always meant: that the defended prose
    # attached to these two seats is still found, with its triggers intact.
    hit = {}
    for k, f in want:
        hit[k] = next((x for x in found
                       if x.key == k and f in x.file and x.triggers), None)
    missing = [k for k, _ in want if hit[k] is None]
    w.record("W4-2", not missing,
             "positive control, keyed by symbol and by binding: all %d known-defended "
             "sites found with a non-empty trigger set — %s"
             % (len(want), "; ".join("%s [%s]" % (k.split(" in ")[0], ", ".join(hit[k].triggers))
                                     for k, _ in want))
             if not missing else "MISSED: " + ", ".join(missing))

    found.sort(key=lambda f: (f.file, f.line))
    return {"sites": found, "triggers": TRIGGERS, "per_token": per_token}


# ═══════════════════════════════════════════════════════════════════════
# PHASE 0e — THE ARTIFACT
#
# Table A's `purpose` column and the whole of Table D are JEAN'S. They are
# left empty on purpose: they are the two things a census cannot produce.
# ═══════════════════════════════════════════════════════════════════════

INPUTS = [STATE_HPP, GEN_INC, REGISTRY_HPP, RENDERER_HPP, WORLD_WGSL]


def writer_pins_lf(w):
    """Assert the artifact writer cannot let the host choose line endings.

    G2 says two runs on an unchanged tree are byte-identical. Compared on
    ONE host that holds trivially even if the write is host-translated —
    Python's text mode rewrites "\n" to the platform terminator unless
    `newline` is pinned. So the determinism claim is only cross-host if the
    WRITER is pinned, and that is a static property worth asserting rather
    than assuming.
    """
    src = read(os.path.abspath(__file__))
    m = re.search(r"open\(path,\s*\"w\"[^)]*\)", src)
    call = m.group(0) if m else ""
    ok = 'newline="\\n"' in call and 'encoding="utf-8"' in call
    w.record("G2-eol", ok,
             "artifact writer pins `encoding=\"utf-8\", newline=\"\\n\"`, so no host can "
             "translate the terminator; a byte-level read-back runs after the write"
             if ok else "artifact writer does not pin the newline: %r" % call)
    return ok


def verify_bytes(path):
    """Read the artifact back as bytes. LF-only, no BOM, one trailing LF."""
    with open(path, "rb") as f:
        d = f.read()
    return {"crlf": d.count(b"\r\n"), "cr": d.count(b"\r"),
            "lf": d.count(b"\n") - d.count(b"\r\n"),
            "bom": d[:3] == b"\xef\xbb\xbf",
            "trailing": d.endswith(b"\n") and not d.endswith(b"\n\n")}


def provenance():
    """Commit SHA + content hashes of the four inputs.

    NOT `HEAD`. G2 requires that re-running on an unchanged tree reproduces
    this file byte for byte, and HEAD moves the moment the artifact is
    committed. The SHA recorded here is the last commit that touched any
    INPUT, which is stable across commits that do not — including the one
    that lands this file. The content hashes are the authoritative
    provenance; the SHA is the human-readable handle for them.
    """
    rel = [os.path.relpath(p, REPO) for p in INPUTS]
    try:
        sha = subprocess.run(["git", "-C", REPO, "log", "-1", "--format=%H", "--"] + rel,
                             capture_output=True, text=True, check=True).stdout.strip()
        subj = subprocess.run(["git", "-C", REPO, "log", "-1", "--format=%s", "--"] + rel,
                              capture_output=True, text=True, check=True).stdout.strip()
    except Exception:
        sha, subj = "(git unavailable)", ""
    return sha, subj, [(r, sha256(p)) for r, p in zip(rel, INPUTS)]


_PIN_ROW = re.compile(
    r"^\|\s*`([^`]+)`\s*\|\s*`sha256:([0-9a-f]{64})`\s*\|\s*$", re.M)


def pinned_inputs(ledger_md):
    """Read back the pins a ledger wrote about ITS OWN inputs.

    The inverse of provenance(): that function stamps the hashes, this one
    recovers them. Keyed on the `sha256:` prefix, which is exactly what
    separates the provenance stanza from BUDGET_0f's caller table, whose
    hashes are bare. Separators are normalised because the stamp is
    os.path.relpath and the two hosts disagree about the slash.

    Returns None when the ledger is not on disk — an absent artifact
    pins nothing, which the caller reports rather than passes.
    """
    try:
        text = read_raw(ledger_md)
    except OSError:
        return None
    return {p.replace("\\", "/"): h for p, h in _PIN_ROW.findall(text)}


def report_stale(ledger_md, required):
    """GATES_2 — does the ARTIFACT still describe the tree it censused?

    `--check` proves the witnesses hold against the LIVE files. It never
    proved the ledger on disk was taken FROM those files, so an edit to a
    scanned input that skipped regeneration left every witness green and
    the artifact silently wrong — the whole battery included, since no row
    read the pins. PALE_0 shipped one file where three were due and no
    gate said a word. This is that missing comparison, one line per pin.

    `required` is the paths this ledger must pin; a pin that is absent is
    as red as a pin that disagrees. Returns True when stale, and the
    caller exits nonzero — the message names the CURE, not just the
    disease, because the two tools have a load-bearing order.
    """
    rel_led = os.path.relpath(ledger_md, REPO).replace(os.sep, "/")
    cure = ("Regenerate in input order: binding_ledger, then mirror_census.")
    pins = pinned_inputs(ledger_md)
    bad = []
    for p in required:
        rel = os.path.relpath(p, REPO).replace(os.sep, "/")
        if pins is None:
            bad.append("STALE: %s is absent, so nothing pins %s. %s"
                       % (rel_led, rel, cure))
            continue
        want = pins.get(rel)
        if want is None:
            bad.append("STALE: %s pins no hash for %s. %s"
                       % (rel_led, rel, cure))
            continue
        live = sha256(p)
        if live != want:
            bad.append("STALE: %s pins %s for %s; live is %s. %s"
                       % (rel_led, want[:8], rel, live[:8], cure))
    for line in bad:
        print(line)
    return bool(bad)


def shallow_note():
    """L29 — the environment announces its own depth.

    S-6 asks a question about history (HEAD vs the pushed tip), and the
    provenance header names a source commit; both read the objects
    present, not the objects that exist. In a shallow clone that is a
    different question wearing the same words. Printed, never judged —
    binding_gen imports this module and calls it too, so the pair speaks
    with one voice.
    """
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    if os.path.exists(os.path.join(root, ".git", "shallow")):
        print("  [gate] NOTE: shallow clone — history-derived claims are "
              "unsafe until git fetch --unshallow")


def md_escape(s):
    return str(s).replace("|", "\\|")


# Comments in state.hpp that this census can check a number against. Each
# is quoted only if it is STILL THERE, so the finding cannot outlive the
# comment it is about. The census reports; it never corrects a comment.
CHECKED_COMMENTS = [
    ("the VS storage-buffer cap is full",
     "state.hpp, Render Entity Layout entries[16]/[17] — the two uniform "
     "bindings that exist because the vertex storage budget was said to be full"),
    ("avoids exceeding the 8 storage buffer per-stage limit",
     "state.hpp, Mesh Gen Entity Layout — the one-entry layout said to exist "
     "to stay under the storage cap"),
    ("past the 10-per-stage limit",
     "state.hpp, Compute Entity Layout entries[10]/[11] — the two agent "
     "registries said to have been moved to uniform to clear a 10-per-stage cap"),
]


def check_comments():
    src = read(STATE_HPP)
    low = src.lower()
    return [(frag, where, frag.lower() in low) for frag, where in CHECKED_COMMENTS]


# ═══ THE ROOM FAMILY, BY IDENTITY (PROBATE_E1) ═══════════════════════
#
# The room family is FOUR ENTRY POINTS. It is not a layout name, and
# every attempt to name it by one has rotted: ATLAS_1revB's fix keyed the
# derivation to `roomLayout_`, LOOM_2 renamed that layout to
# `agentsStateLayout_`, the filter silently matched nothing, and
# `max(..., default=0)` printed a confident **0 of 8** for a family
# standing at 5 of 8 — in TWO places (finding 4 and Table D's room row),
# for two campaigns, until PROBATE's report caught it.
#
# An entry point is an identity: renaming one is a shader edit `--check`
# already sees, and a name this list carries that no pipeline reaches is
# a FAILED WITNESS below, never a silent zero. That is the whole
# difference between this derivation and the two it replaces.
#
# IT RUNS IN THE WITNESS PHASE, not inside emit(). Witnesses recorded
# during emission are swallowed — `w.report()` has already run and the
# gate has already passed — so a check placed there fires into nothing
# and looks exactly like a check that passed (P6, and ECONOMY_1 E1's
# inert arm). Verified by negative control: typo an entry-point name and
# this witness FAILS the run.
#
# ONE HOME, TWO READERS: finding 4 and Table D's room row both read what
# this returns. A third body of the defect is not available.
ROOM_ENTRY_POINTS = ("update_player_agent", "update_other_agents",
                     "update_sphere", "update_cube")


def room_family_census(budget, cen, join, w):
    want = set(ROOM_ENTRY_POINTS)
    found, rows, members = set(), [], set()
    for b in budget:
        eps = {cen["entry_const"].get(c, c) for c, _ in b["pipeline"].entries()}
        if eps & want:
            rows.append(b)
            found |= eps & want
            members |= set(b["pipeline"].group_layouts)
    missing = sorted(want - found)
    w.record("E1-identity", not missing,
             ("the room family resolves to %d (pipeline, stage) rows over %d "
              "layouts, from all %d entry points by name"
              % (len(rows), len(members), len(want))) if not missing else
             ("the room family names entry point(s) NO pipeline carries: %s — "
              "fix the name here or in the module, but never let this "
              "derivation resolve to nothing and print a zero"
              % ", ".join(missing)))
    # DEMOTABLE = a seat of this family that Table C already carries as an
    # A2 CANDIDATE. Not "could be demoted in principle" — the A2 verdict,
    # joined by the seat's own layout member and slot category.
    return {
        "storage": max((b["declared"]["storage"] for b in rows), default=0),
        "demotable": sum(1 for x in join["a2"] if x["pass"]
                         and x["row"]["layout"]["member"] in members
                         and slot_category(x["row"]["e"]) == "storage"),
        "rows": rows, "members": members,
    }


def emit(path, w, column, layouts, wgsl, cen, join, e1, e2, e3, e4, room):
    sha, subj, hashes = provenance()
    rows, budget = join["rows"], join["budget"]
    o = []
    A = o.append

    # ─── 1. PROVENANCE ────────────────────────────────────────────────
    A("# THE BINDING LEDGER")
    A("")
    A("A read-only census of every declaration that spends a WebGPU slot, and of")
    A("what the program can actually reach through it. Generated by")
    A("`tools/binding_ledger.py`; do not hand-edit — re-run it.")
    A("")
    A("**One row per `(bind group layout, entry index)`, not per buffer.** The")
    A("registry is one constant per SITE: the same buffer wears several names")
    A("because each name is one `(group, slot)`. A buffer-keyed census would")
    A("merge rows the API charges separately.")
    A("")
    A("## Provenance")
    A("")
    A("| field | value |")
    A("|---|---|")
    A("| demo column censused | `%s` |" % column)
    A("| source commit | `%s` |" % sha)
    A("| | %s |" % md_escape(subj))
    for r, h in hashes:
        A("| `%s` | `sha256:%s` |" % (r, h))
    A("")
    A("")
    A("BUDGET_0f's call-shape census reads further files, for INVOCATION SITES")
    A("only — `draw_orbs` is called from `bodies/orbs.hpp`, not from")
    A("`realization/`. They are read-only and hashed here too:")
    A("")
    A("| caller file scanned | sha256 |")
    A("|---|---|")
    for cp in e3["callers"]:
        A("| `%s` | `%s` |" % (os.path.relpath(cp, REPO), sha256(cp)))
    A("")
    A("The source commit is the last commit touching any of the four primary")
    A("inputs — not `HEAD`, which moves when this file is committed. The content")
    A("hashes are the authoritative provenance.")
    A("")
    A("**Determinism is asserted at byte level, not by comparing two local runs.**")
    A("Two runs on one host are byte-identical even if the write is")
    A("host-translated, because both are translated the same way — Python's text")
    A("mode rewrites `\\n` to the platform terminator unless `newline` is pinned. So")
    A("the writer pins `encoding=\"utf-8\", newline=\"\\n\"` (witness `G2-eol`), and a")
    A("binary read-back after every write asserts LF-only, no BOM, exactly one")
    A("trailing newline. A violation stops the run instead of shipping a file that")
    A("breaks the L1 encoding law on one platform and not another.")
    A("")
    A("### The law this serves")
    A("")
    A("A slot is charged **once per stage named in `visibility`**, whether or not")
    A("that stage can reach the binding. Bind **groups do not partition the")
    A("budget** — every bind group layout in a pipeline layout is concatenated,")
    A("then checked. **Bytes and slots are unrelated currencies.** Core defaults,")
    A("per shader stage: uniform %d · storage %d · sampled %d · samplers %d ·"
      % (CORE["uniform"], CORE["storage"], CORE["sampled"], CORE["samplers"]))
    A("storage textures %d. Per pipeline layout: bind groups %d, dynamic-offset"
      % (CORE["storagetex"], CORE_BIND_GROUPS))
    A("uniform %d, dynamic-offset storage %d. Per binding: uniform %d B, storage"
      % (CORE_DYN_UNIFORM, CORE_DYN_STORAGE, UNIFORM_BINDING_MAX_BYTES))
    A("134217728 B. Uniform bindings also need %d B offset alignment, which"
      % UNIFORM_OFFSET_ALIGN)
    A("matters only where a binding is a window onto a shared buffer.")
    A("")
    A("### Witnesses")
    A("")
    A("| witness | result | numbers |")
    A("|---|---|---|")
    for tag, ok, detail in w.rows:
        A("| `%s` | %s | %s |" % (tag, "**PASS**" if ok else "**FAIL**", md_escape(detail)))
    A("")
    A("`gate` is the RECONCILIATION GATE. The web twin boots on a pure-defaults")
    A("device request (PORT_5d — a value-initialised `wgpu::Limits`, nothing")
    A("named), which is a live external witness that the real program fits")
    A("inside the Core defaults. If a Table B row counted against `vis_declared`")
    A("exceeded a limit, the ledger would be wrong, not the program.")
    A("")

    # ─── Findings. The handoff's REPORT items need a home Jean will read,
    #     and it is not a commit message. Every number below is computed;
    #     every quoted comment is checked to still exist before it is
    #     quoted. Reported, not acted on: no program file is touched.
    tight, tcat = join["tightest"]
    main_v = next((r for r in budget if r["pipeline"].member == "pawnPipeline_"
                   and r["stage"] == "V"), None)
    gated = [L for L in layouts if L["roster_gated"]]
    reached_any = set()
    for _, refs in wgsl["reach"].values():
        reached_any |= refs
    dead_decls = sorted(d.symbol for d in wgsl["decls"] if d.symbol not in reached_any)

    A("### Findings — reported, not acted on")
    A("")
    A("**1. The binding surface does not vary with the demo column.** %s"
      % ("No bind group layout creation site sits inside a `if constexpr (ROSTER.…)` "
         "gate — all %d are created unconditionally. So slot pressure exists for "
         "pipelines a given build never creates: the PIPELINE set is ROSTER-gated "
         "(%d of %d pipelines carry a gate), the LAYOUT set is not."
         % (len(layouts), sum(1 for p in cen["pipelines"] if p.roster_gate),
            len(cen["pipelines"]))
         if not gated else
         "%d layout(s) ARE ROSTER-gated: %s."
         % (len(gated), ", ".join(L["label"] for L in gated))))
    A("")
    A("**2. No dead surface in the shader.** %s"
      % ("All %d module-scope declarations are reached by at least one of the %d "
         "entry points." % (len(wgsl["decls"]), len(wgsl["entries"]))
         if not dead_decls else
         "%d declaration(s) reached by ZERO entry points: %s."
         % (len(dead_decls), ", ".join(dead_decls))))
    A("")
    # LOOM_CLOSE C1b: this finding REGENERATES from the parsed rows —
    # the hand-carried "zero dynamic bindings" sentence outlived
    # ATLAS_1revB's shadow slot and contradicted witness 0d-1 in the
    # same artifact. A finding the parser can compute, the parser
    # states.
    dyn_seats = [(L["label"], e.binding_const) for L in layouts
                 for e in L["entries"] if e.has_dynamic_offset]
    if not dyn_seats:
        A("**3. Zero dynamic-offset bindings.** `hasDynamicOffset` is never set")
        A("anywhere in `state.hpp`; the two dynamic-offset limits stand at 0/%d"
          % CORE_DYN_UNIFORM)
        A("and 0/%d program-wide. Computed each run; witness `0d-1` is the gate"
          % CORE_DYN_STORAGE)
        A("this figure answers to.")
    else:
        A("**3. Dynamic-offset bindings: %d.** %s. The limits stand at %d/%d"
          % (len(dyn_seats),
             "; ".join("`%s` in %s" % (c_, l_) for l_, c_ in dyn_seats),
             max((b["dyn_uniform"] for b in budget), default=0),
             CORE_DYN_UNIFORM))
        A("uniform and %d/%d storage at the worst pipeline. Computed each run;"
          % (max((b["dyn_storage"] for b in budget), default=0),
             CORE_DYN_STORAGE))
        A("witness `0d-1` is the gate this figure answers to.")
    A("")
    room_family_storage = room["storage"]
    room_demotable = room["demotable"]
    A("**4. The tightest row is not where the handoff expected it (as of BUDGET_1).** BUDGET_0's")
    A("handoff expected the main render family's VERTEX stage at storage 8/8. It")
    if main_v:
        A("reads **storage %d of %d**. The tightest row in the program is"
          % (main_v["declared"]["storage"], CORE["storage"]))
    # ATLAS_1revB found this paragraph asserting "the ROOM FAMILY" and naming
    # a layout regardless of which row was actually tightest. That was
    # silently true while the room family held the row and false the moment it
    # did not. The family is now DERIVED from the tightest row's own pipeline
    # layout, so the sentence cannot outlive its subject.
    #
    # PROBATE_E1: and the family it is compared against is derived from
    # ENTRY POINTS (see the block above), not from a layout name. The
    # original form of this guard named `roomLayout_` — which is how a
    # fix written against a rename became the rename's next victim.
    A("**%s / %s at %s %d of %d** — over the concatenated groups %s,"
      % (md_escape(tight["pipeline"].label), tight["stage"], tcat,
         tight["declared"][tcat], CORE[tcat],
         " + ".join("`%s`" % g for g in tight["pipeline"].group_layouts)))
    A("shared by %s."
      % ", ".join("`%s`" % cen["entry_const"].get(c, c)
                  for p in cen["pipelines"] if p.group_layouts == tight["pipeline"].group_layouts
                  for c, _ in p.entries()))
    # Third stale sentence in this block, pre-dating ATLAS_1revB: it quoted the
    # banner as saying "8/8 storage" when the banner says 6/8 and PIVOT_0
    # renamed it from FXC to COMPILER FLOOR. A verbatim quote of text that no
    # longer exists is worse than no quote, so the banner is cited, not quoted,
    # and the room family's own figure is measured rather than remembered.
    A("`world.wgsl`'s COMPILER FLOOR banner carries the standing demotion rule for")
    A("the room family, which stands at %d of %d storage with %d demotable seats. And"
      % (room_family_storage, CORE["storage"], room_demotable))
    A("its `actual` column reads %d too: **an A1 sweep buys nothing on the one row"
      % tight["actual"][tcat])
    A("with no margin.**")
    A("")
    A("The handoff asked which comment has outlived its referent if the render")
    A("family did not reproduce 8/8. Three have, to different degrees. Each is")
    A("quoted only because it is still present in `state.hpp` today:")
    A("")
    live = {frag: ok for frag, _, ok in check_comments()}
    if live.get("the VS storage-buffer cap is full") and main_v:
        A("- *\"the VS storage-buffer cap is full\"* — **off by one.** The vertex")
        A("  stage stands at storage %d of %d, so there is exactly ONE free vertex"
          % (main_v["declared"]["storage"], CORE["storage"]))
        A("  storage slot. Not enough for both `agent_tier_gains` and")
        A("  `agent_figure_profiles` — putting both back as storage would be %d —"
          % (main_v["declared"]["storage"] + 2))
        A("  but \"full\" is no longer literally true. The decision the comment")
        A("  defends still holds; the number in it does not.")
    if live.get("avoids exceeding the 8 storage buffer per-stage limit"):
        mg = [p for p in cen["pipelines"]
              if "meshGenEntityBindGroupLayout_" in p.group_layouts]
        A("- *\"Avoids exceeding the 8 storage buffer per-stage limit\"* on the Mesh")
        A("  Gen Entity Layout — **outlived its referent outright.** That layout is")
        A("  bound by %s in the whole program: %s. %sEvery mesh-gen"
          % ("exactly ONE pipeline" if len(mg) == 1 else "%d pipelines" % len(mg),
             ", ".join(md_escape(p.label) for p in mg) or "none",
             "A render pipeline with one uniform, zero storage buffers, and no "
             "compute stage at all. " if len(mg) == 1 else ""))
        A("  pipeline now carries its own dedicated single-group layout")
        A("  (`archMeshGenLayout_` and its four siblings). Corroborating it: this")
        A("  layout's one entry declares `VFC` and only `F` reaches it — the `C` in")
        A("  that mask is a fossil of the mesh-gen era, and it is an A1 row below.")
    if live.get("past the 10-per-stage limit"):
        A("- *\"pushed compute storage buffer count past the 10-per-stage limit\"* —")
        A("  **the Core default is %d, not 10.** The constraint is real and TIGHTER"
          % CORE["storage"])
        A("  than the comment states: with those two registries as storage the room")
        A("  family would stand at %d of %d. The number cited is not a Core default."
          % (tight["declared"]["storage"] + 2, CORE["storage"]))
    A("")
    A("**5. One correction to the handoff's A2 rule, and the ledger follows the")
    A("spec (as of BUDGET_1).** The handoff states that in the uniform address space")
    A("`array<f32, N>` *and* `array<vec3<f32>, N>` are illegal. The first is")
    A("right; the second is not. The rule is that the array's **element stride**")
    A("— `roundUp(AlignOf(E), SizeOf(E))` — must be a multiple of 16.")
    A("`AlignOf(vec3<f32>)` is 16, so the stride is `roundUp(16, 12)` = 16 and the")
    A("array is legal; it merely wastes 4 B per element. `array<vec2<f32>, N>`")
    A("(stride 8) *is* illegal, as is any scalar element. A type's own alignment")
    A("being under 16 is not a blocker either: `array<AgentBehaviorParams, 10>`")
    A("has `AlignOf` 4 and stride 32, and the program binds it as a uniform")
    A("today. Witness `0b-5` is the guard — the predicate clears all %d"
      % sum(1 for d in wgsl["decls"] if d.address_space == "uniform"))
    A("declarations the program already places in uniform.")
    A("")
    A("**6. The C6 note on `g2::field_head_poses` did not cost an element type (as of BUDGET_1).**")
    A("The handoff attributes an element-type widening to that demotion. At")
    A("`ecfbc32`, the commit that introduced the binding, both `g0:122")
    A("head_poses` and `g2:2 field_head_poses` already read")
    A("`array<vec4<f32>, 400>`. The demotion changed the address space over an")
    A("element type that was already vec4-strided. Table C still records the")
    A("required element type per A2 row, because for other candidates it is a")
    A("real cost — it just was not one there.")
    A("")
    A("**7. A4 is empty, so BUDGET_1 is the A1 sweep alone (as of BUDGET_1).** Every one of the")
    A("%d `var<storage, read_write>` declarations is written by at least one entry"
      % sum(1 for dd in wgsl["decls"] if dd.address_space == "storage"
            and dd.wgsl_access == "read_write"))
    A("point that reaches it. No binding claims write access it never exercises.")
    A("")
    A("**8. The room family cannot be fused, and the reason is mechanical (as of BUDGET_1).**")
    barred_room = [r for r in e1["raw"]
                   if r["a"].startswith("update_") and r["b"].startswith("update_")
                   and r["hazard"]]
    A("All %d ordered pairs among `update_player_agent`, `update_other_agents`,"
      % len(barred_room))
    A("`update_sphere` and `update_cube` carry a RAW hazard — `agent_state`")
    A("between the agent kernels, `floating_entities` between the floater")
    A("kernels, and `field_forces` across the divide. Fusing deletes the implicit")
    A("inter-dispatch barrier that makes the ordering correct, and WGSL has no")
    A("device-wide barrier to put back. Table E has the pairs.")
    A("")
    A("**8b. The program has ZERO fusable pairs, and the ordered table alone")
    A("does not say so (as of BUDGET_1).** A fused kernel has no ordering — its threads run")
    A("concurrently inside one dispatch — so fusability needs the UNORDERED")
    A("closure, not the ordered hazard list. Of %d unordered fusion-eligible"
      % len(e1["unordered"]))
    A("pairs, **%d survive RAW in both directions**, and both couple an on-demand"
      % len([u for u in e1["unordered"] if u["clean"]]))
    A("pass with a per-frame one — fusing either would change WHEN work runs, not")
    A("just how it is dispatched. RAW-clean is necessary, not sufficient: cadence")
    A("is the second gate and it lives in the dispatch schedule, not the shader.")
    A("")
    A("**9. The vertex-buffer candidates are not the ones that were proposed (as of BUDGET_1; the eligible row spent by DOMESDAY_0 B3).**")
    A("`visible_patch_indices` WAS eligible — one site, `[patch_id]`, sequential in")
    A("`instance_index`, stride 4 B — and DOMESDAY_0 B3 spent that row: the")
    A("visible list now arrives in `patch_terrain_vs` as an instance-step vertex")
    A("attribute (`@location(0) visible_id`) and the binding is retired.")
    A("`patch_instances` remains BLOCKED, now by the attribute join inside ONE")
    A("entry point: `patch_terrain_vs` reads it sequentially on the direct path")
    A("and through the `visible_id` attribute under the `USE_PATCH_INDIRECTION`")
    A("override. `render_ring_xforms` is blocked")
    A("at `builtin_derived(vertex)` — segment arithmetic, many vertices per ring,")
    A("and its pipeline draws with instanceCount 1 besides. A third candidate")
    A("nobody named: `render_orb_state`, eligible on access pattern AND drawn")
    A("instanced, and a runtime-sized array — the class A2 cannot touch.")
    A("")
    A("*TETRIS ORB_V took that third candidate; DOMESDAY_0 B3 took the first.*")
    A("The two blocked rows stay blocked for the same reasons as at BUDGET_1.")
    A("")
    A("**10. The single-thread count is three numbers, not one (as of BUDGET_1).** %d entry points"
      % len(e3["wg1"]))
    A("declare `@workgroup_size(1)`; %d dispatches issue one workgroup; the"
      % len(e3["single_dispatch"]))
    A("intersection is %d. `update_other_agents` dispatches ONE workgroup at"
      % len(set(e3["wg1"]) & set(e3["single_dispatch"])))
    A("`@workgroup_size(32)` — which is exactly the shape the kernel-split banner")
    A("prices at 48 seconds of FXC.")
    A("")
    A("**11. %d defended sites.** Table H marks where the program already paid to"
      % len(e4["sites"]))
    A("learn something. It cites and does not quote, so it cannot go stale against")
    A("the prose it points at.")
    A("")
    A("**12. The census corrected its own instrument, and the lesson is the")
    A("campaign's (as of BUDGET_1; the control re-keyed at LOOM_2).** BUDGET_1 removed two layout seats and renumbered the")
    A("survivors, and `W4-2` failed — not because the sweep was wrong, and not")
    A("because the instrument stopped finding the defended prose. It found all")
    A("%d sites with identical trigger sets. What broke was the CONTROL'S KEY:"
      % len(e4["sites"]))
    A("it named two defended sites `Render Entity Layout entries[16]` and")
    A("`entries[17]`, and a campaign whose job is removing layout entries")
    A("renumbers the survivors.")
    A("")
    A("**An index is a position; a binding is an identity.** The control was")
    A("itself a reference that outlived its referent — precisely what Table H")
    A("says about line numbers, one layer up and pointed at the instrument. It is")
    A("now keyed by the binding each seat carries, `bind::g2::agent_tier_gains`")
    A("and `bind::g2::agent_figure_profiles` in the Scene State Layout (their")
    A("LOOM_2 home; the identity followed the seats through the recut), which")
    A("survive renumbering. The displayed `entries[N]` column stays, because it")
    A("is regenerated every run and a reader wants it.")
    A("")
    A("Recorded here as a finding rather than only in a commit body, because a")
    A("positive control that can be made to pass by whoever it is testing is not")
    A("a control. The re-key was authorised before the run that had to pass it.")
    A("")

    # ─── 2. TABLE A ───────────────────────────────────────────────────
    A("## Table A — the ledger")
    A("")
    A("One row per `(bind group layout, entry index)`, %d rows over %d layouts."
      % (len(rows), len(layouts)))
    A("Columns through `roster_gated` are the layout census (0a); `group` onward")
    A("are joined from the WGSL census (0b) and the pipeline census (0c).")
    A("`bytes` is the size of the WGSL **store type**, computed by WGSL layout")
    A("rules; `—` means a runtime-sized array or a handle type, which has none.")
    A("**`purpose` is Jean's and is left empty.**")
    A("")
    hdr = ("| layout_label | layout_member | # | binding_const | binding | ns | "
           "kind | access | vis_declared | dyn | roster_gated | group | wgsl_symbol | "
           "wgsl_type | wgsl_access | runtime_array | bytes | slot | vis_actual | "
           "vis_delta | purpose |")
    A(hdr)
    A("|" + "---|" * (hdr.count("|") - 1))
    for r in sorted(rows, key=lambda r: (r["layout"]["label"], r["e"].entry_index)):
        e = r["e"]
        d = r["decls"][0] if r["decls"] else None
        alias = ""
        if len(r["at_slot"]) > 1:
            alias = " *(slot also spelled %s)*" % ", ".join(
                "`%s`" % x.symbol for x in r["at_slot"] if x.symbol != r["primary"])
        A("| %s | `%s` | %d | `%s` | %s | `%s` | %s | %s | `%s` | %s | %s | %s | `%s`%s | "
          "`%s` | %s | %s | %s | %s | `%s` | `%s` |  |"
          % (md_escape(e.layout_label), e.layout_member, e.entry_index,
             md_escape(e.binding_const), e.binding_number, e.registry_ns,
             e.kind, md_escape(e.access), e.vis_declared,
             "yes" if e.has_dynamic_offset else "no",
             e.roster_gated or "—", r["group"],
             r["primary"], alias,
             md_escape(d.wgsl_type) if d else "—",
             d.wgsl_access if d else "—",
             ("yes" if d.has_runtime_array else "no") if d else "—",
             (d.layout.size if d and d.layout.size is not None else "—"),
             slot_category(e) or "—",
             r["vis_actual"] or "-", r["vis_delta"] or "-"))
    A("")

    # ─── 3. TABLE B ───────────────────────────────────────────────────
    A("## Table B — the stage budget")
    A("")
    A("One row per `(pipeline, stage)`, counted across **all** the pipeline's")
    A("group layouts concatenated. Each cell reads `declared / actual / reached`")
    A("against the Core limit in the header.")
    A("")
    A("- **declared** — counted against `vis_declared`. *This is what the API")
    A("  charges.* Every one of these must fit, and the gate above says they do.")
    A("- **actual** — counted against `vis_actual`: the same rows, minus those no")
    A("  reachable code touches through this layout in any pipeline that binds")
    A("  it. This is what the budget would read after an A1 sweep. **The")
    A("  difference between these two columns is the entire prize of BUDGET_1.**")
    A("- **reached** — counted against what *this pipeline's* stage actually")
    A("  touches. A derived third column, narrower than `actual`, because a")
    A("  visibility mask is shared by every pipeline binding the layout and can")
    A("  only be trimmed to the union. It bounds what any per-pipeline split")
    A("  could ever buy.")
    A("")
    A("| pipeline | member | stage | entry point | uniform /%d | storage /%d | "
      "sampled /%d | samplers /%d | storagetex /%d | bind groups /%d | dyn_u /%d | dyn_s /%d |"
      % (CORE["uniform"], CORE["storage"], CORE["sampled"], CORE["samplers"],
         CORE["storagetex"], CORE_BIND_GROUPS, CORE_DYN_UNIFORM, CORE_DYN_STORAGE))
    A("|---|---|---|---|---|---|---|---|---|---|---|---|")
    for bx in budget:
        p = bx["pipeline"]
        ep = ", ".join(cen["entry_const"].get(c, c) for c, s in p.entries() if s == bx["stage"])
        cells = []
        for k in ("uniform", "storage", "sampled", "samplers", "storagetex"):
            cell = "%d / %d / %d" % (bx["declared"][k], bx["actual"][k], bx["reached"][k])
            if bx["declared"][k] == CORE[k]:
                cell = "**%s**" % cell
            cells.append(cell)
        A("| %s | `%s` | %s | `%s` | %s | %d | %d | %d |"
          % (md_escape(p.label), p.member, bx["stage"], ep, " | ".join(cells),
             len(p.group_layouts), bx["dyn_uniform"], bx["dyn_storage"]))
    A("")
    tight, tcat = join["tightest"]
    A("Tightest row: **%s / %s** at %s **%d of %d**, and `actual` reads %d there too —"
      % (md_escape(tight["pipeline"].label), tight["stage"], tcat,
         tight["declared"][tcat], CORE[tcat], tight["actual"][tcat]))
    A("an A1 sweep buys nothing on the row that has no margin.")
    A("")
    A("The rows in bold are at their limit exactly. Groups bound per pipeline")
    A("are shown because they are a per-pipeline-layout limit of their own;")
    A("`bind groups + vertex buffers` is checked by witness `0c-2` and is")
    A("nowhere near its 24.")
    A("")

    # ─── 4. TABLE C ───────────────────────────────────────────────────
    A("## Table C — Tier A candidates")
    A("")
    A("**A flag list, not a plan.** Rows are flagged; nothing is acted on.")
    A("")
    A("### A1 — OVER-VISIBLE (`vis_delta` ≠ ∅)")
    A("")
    A("The declaration claims a stage that cannot reach the binding. Correcting")
    A("one is not an optimization: it is the removal of a false statement that")
    A("happened to cost a slot. No shader edit, no data movement, no behaviour")
    A("change.")
    A("")
    A("| layout | # | symbol | kind | slot | vis_declared | vis_actual | **vis_delta** | "
      "slots freed, per stage | reached by |")
    A("|---|---|---|---|---|---|---|---|---|---|")
    for r in sorted(join["a1"], key=lambda r: (r["layout"]["label"], r["e"].entry_index)):
        A("| %s | %d | `%s` | %s | %s | `%s` | `%s` | `%s` | %s | %s |"
          % (md_escape(r["layout"]["label"]), r["e"].entry_index, r["primary"],
             r["e"].kind, slot_category(r["e"]), r["e"].vis_declared,
             r["vis_actual"] or "-", r["vis_delta"],
             ", ".join("%s: 1 %s" % (s, slot_category(r["e"])) for s in r["vis_delta"]),
             md_escape(", ".join(r["reached_by"]) or "— nothing")))
    A("")
    A("### A2 — DEMOTABLE (storage → uniform)")
    A("")
    A("All five predicates must hold. The one that actually decides it is")
    A("`uniform_legal`: in the uniform address space an array's **element")
    A("stride** must be a multiple of 16, every struct member offset must be a")
    A("multiple of `roundUp(16, …)` for struct and array members, and a struct or")
    A("array member must be followed by `roundUp(16, SizeOf)` of space.")
    A("`element_type_required` names what a demotion would cost, or says none is")
    A("needed. Uniform bindings also carry a %d B offset alignment, which binds"
      % UNIFORM_OFFSET_ALIGN)
    A("only where the binding is a window onto a shared buffer.")
    A("")
    A("| layout | # | symbol | wgsl_type | bytes ≤ %d | ReadOnlyStorage | wgsl read | "
      "no runtime array | uniform_legal | **verdict** | element_type_required | "
      "same slot bound elsewhere as |" % UNIFORM_BINDING_MAX_BYTES)
    A("|---|---|---|---|---|---|---|---|---|---|---|---|")
    tick = lambda b: "yes" if b else "**no**"
    for x in sorted(join["a2"], key=lambda x: (not x["pass"], x["row"]["layout"]["label"],
                                               x["row"]["e"].entry_index)):
        r, ev = x["row"], x["ev"]
        # A slot demoted in one layout is still the same buffer the OTHER
        # layouts bind. If another layout binds it Storage, the buffer needs
        # both usages and the demotion is not local to one layout.
        elsewhere = sorted({"%s in %s" % (o["e"].access, o["layout"]["label"])
                            for o in rows
                            if (o["group"], o["e"].binding_number)
                            == (r["group"], r["e"].binding_number)
                            and o is not r and o["e"].access != r["e"].access})
        A("| %s | %d | `%s` | `%s` | %s | %s | %s | %s | %s | %s | %s | %s |"
          % (md_escape(r["layout"]["label"]), r["e"].entry_index, r["primary"],
             md_escape(x["decl"].wgsl_type) if x["decl"] else "—",
             ("%s (%s)" % (tick(ev["bytes"]), x["size"] if x["size"] is not None else "—")),
             tick(ev["access"]), tick(ev["wgsl_access"]), tick(ev["not_runtime"]),
             tick(ev["uniform_legal"]),
             "**CANDIDATE**" if x["pass"] else "no",
             md_escape(x["element_note"]),
             md_escape("; ".join(elsewhere)) if elsewhere else "—"))
    A("")
    A("A demotion also **spends a uniform slot to save a storage one**, so an A2")
    A("row only helps where the same `(pipeline, stage)` has uniform margin. Read")
    A("each candidate against its Table B rows before costing it. And the last")
    A("column matters: where the same `(group, binding)` is bound with a different")
    A("access by another layout, a demotion is not local to one layout — the")
    A("buffer carries both usages, and the WGSL declaration for the demoted")
    A("layout has to be a separate `var<uniform>` alias, which spends a registry")
    A("site of its own. That is the `orb_state` / `orb_state_ro` pattern.")
    A("")
    A("### A3 — ORPHANED")
    A("")
    A("Dead binding surface. **Every row states the demo column it was censused")
    A("under**, because a flag on a binding reached only under a column not")
    A("censused would be a false positive.")
    A("")
    if join["a3"]:
        A("| kind | what | demo column | evidence |")
        A("|---|---|---|---|")
        for kind, what, _ in join["a3"]:
            ev = ""
            for r in rows:
                if what.startswith(r["layout"]["label"]) and ("entries[%d]" % r["e"].entry_index) in what:
                    ev = ("bound by %d pipeline(s): %s; reached by none of their entry points"
                          % (len(set(r["binders"])), ", ".join(sorted(set(r["binders"])))))
            A("| %s | %s | `%s` | %s |" % (md_escape(kind), md_escape(what), column, md_escape(ev)))
    else:
        A("None.")
    A("")
    A("No registry constant lacks a WGSL mirror — %d constants, %d slots, one to"
      % (len(parse_registry(Witnesses())), len(wgsl["slots"])))
    A("one. No WGSL declaration goes unreached by every entry point. No bind group")
    A("layout goes unbound by every pipeline. Every layout entry's")
    A("`(group, binding)` resolves to an access-compatible WGSL declaration.")
    A("")

    A("### A4 — OVER-PRIVILEGED (declared `read_write`, never written)")
    A("")
    A("A binding whose WGSL declaration claims write access no entry point that")
    A("reaches it ever exercises. Same species as A1: not an optimization, the")
    A("removal of a false statement. `read_write` storage is also **illegal in")
    A("the vertex stage**, so an over-privileged binding is a portability hazard")
    A("the moment anything wants it vertex-visible — and A2 requires")
    A("`ReadOnlyStorage`, so an A4 correction can unlock an A2 candidate.")
    A("")
    if e1["a4"]:
        A("| symbol | wgsl_type | read by | unlocks A2 |")
        A("|---|---|---|---|")
        for x in e1["a4"]:
            A("| `%s` | `%s` | %s |  |" % (x["decl"].symbol, md_escape(x["decl"].wgsl_type),
                                           ", ".join("`%s`" % r for r in x["readers"])))
    else:
        A("**Empty.** All %d `var<storage, read_write>` declarations in the program"
          % sum(1 for dd in wgsl["decls"] if dd.address_space == "storage"
                and dd.wgsl_access == "read_write"))
        A("are written by at least one entry point that reaches them. There is no")
        A("over-privileged binding, so BUDGET_1 does not become an A1+A4 sweep —")
        A("it stays the A1 sweep, seven rows.")
    A("")

    # ─── 5. TABLE D ───────────────────────────────────────────────────
    A("## Table D — DEMAND")
    A("")
    A("**Jean's, and their home is the schema.** The four judgment cells —")
    A("demand, what it buys, what it costs, ruling — are joined in from")
    A("`DEMAND_RULINGS` in `tools/binding_schema.py` (PROBATE_D) by each queued")
    A("item's STABLE ID (PROBATE_E1) — `a2:{binding_const}@{layout_member}`, or")
    A("`room-wall` for the wall — never by the prose printed in the first")
    A("column, which is free to be rewritten. Nothing in Tier A needs a")
    A("demand to justify it — a declaration that claims a stage it cannot reach")
    A("was never true. These are the items that DO.")
    A("")
    A("Three columns are CENSUS FACTS, not judgments — 0f is what makes them")
    A("computable. A ruling is a schema edit and Jean-gated like any other; an")
    A("id this emitter builds and the schema does not carry leaves its cells")
    A("empty and prints a warning at the console, so a rename can orphan a")
    A("ruling loudly but never silently.")
    A("")
    A("| queued item | fusion barred? | can leave the storage wallet? | site defended? | demand | what it buys | what it costs | ruling |")
    A("|---|---|---|---|---|---|---|---|")
    # PROBATE_E1 — EVERY QUEUED ITEM CARRIES A STABLE ID. The join to
    # DEMAND_RULINGS runs on the id, never on the printed prose, so a
    # sentence may be rewritten (as this round rewrites the room row's)
    # without orphaning the ruling attached to it. An id is built from
    # identities only — a binding constant and a layout member, or a
    # fixed name for the wall.
    queued = []
    for x in join["a2"]:
        if x["pass"]:
            queued.append(("A2 demotion — `%s` @group(%s) @binding(%s) in %s (%s B)"
                           % (x["row"]["primary"], x["row"]["group"],
                              x["row"]["e"].binding_number, x["row"]["layout"]["label"],
                              x["size"]), x["row"]["primary"],
                           "a2:%s@%s" % (x["row"]["e"].binding_const,
                                         x["row"]["layout"]["member"])))
    for kind, what, _ in join["a3"]:
        queued.append(("A3 removal — %s" % what,
                       what.split()[-2] if " " in what else what,
                       "a3:%s" % what))
    for x in e2["eligibility"]:
        if x["eligible"]:
            queued.append(("Vertex-buffer move — `%s` (%s-step, stride %s B); the only "
                           "wallet that can hold a runtime-sized array"
                           % (x["decl"].symbol, x["step"], x["stride"]),
                           x["decl"].symbol, "vb:%s" % x["decl"].symbol))
    # ATLAS_1revB: this row printed the PROGRAM-WIDE tightest row's storage and
    # attributed it to the room family. Coincidentally right until the tightest
    # row moved off the room family; then it asserted a number the room family
    # does not hold. Measure the room family itself.
    # PROBATE_E1: the sentence is COMPUTED, from the identity-derived room
    # family above. Both of its numbers are measured each run — the stand
    # and the number of its seats Table C already calls demotable — and
    # the row's join id (`room-wall`) is independent of every word of it.
    queued.append(("The room family's storage lane stands at %d of %d with %d "
                   "demotable seats (%d = its seats holding an A2 CANDIDATE "
                   "verdict). Any new storage binding reachable from "
                   "update_player_agent / update_other_agents / update_sphere / "
                   "update_cube needs a demotion to pay for it, and none is for "
                   "sale."
                   % (room_family_storage, CORE["storage"],
                      room_demotable, room_demotable),
                   "update_player_agent", "room-wall"))
    defended_syms = {f.symbol for f in e4["sites"]}
    elig_syms = {x["decl"].symbol for x in e2["eligibility"] if x["eligible"]}
    # PROBATE_D — THE JUDGMENT CELLS COME FROM THE SCHEMA. The join key is
    # the queued-item string exactly as printed. An unmatched key emits
    # empty cells and warns: a future rename must not silently orphan a
    # ruling, and an empty cell that looks deliberate is worse than a
    # missing one that says so.
    rulings = getattr(schema, "DEMAND_RULINGS", {})
    matched = set()
    for q, sym, qid in queued:
        barred = "n/a"
        if sym in {r["a"] for r in e1["raw"]} | {r["b"] for r in e1["raw"]}:
            hz = sorted({h for r in e1["raw"] if sym in (r["a"], r["b"])
                         for h in r["hazard"]})
            barred = ("**yes** — " + ", ".join("`%s`" % h for h in hz)) if hz else "no"
        r = rulings.get(qid)
        if r is None:
            print("  [WARN] Table D: no DEMAND_RULINGS row for id %r — four "
                  "judgment cells emitted empty (PROBATE_E1 join)" % qid)
            judgment = ("", "", "", "")
        else:
            matched.add(qid)
            judgment = (r["demand"], r["buys"], r["costs"], r["ruling"])
        A("| %s | %s | %s | %s | %s | %s | %s | %s |"
          % (md_escape(q), barred,
             ("**yes**, vertex buffer" if sym in elig_syms else "no"),
             ("**yes** — see Table H" if sym in defended_syms else "no"),
             *(md_escape(c) for c in judgment)))
    for orphan in sorted(set(rulings) - matched):
        print("  [WARN] Table D: DEMAND_RULINGS carries a ruling for id %r "
              "that this table no longer prints (PROBATE_E1 join)" % orphan)
    A("")

    # ─── 6. TABLE E — the RAW hazard table (BUDGET_0f-1) ──────────────
    A("## Table E — RAW hazards, and what they bar")
    A("")
    A("**Why this is a hard bar.** WebGPU inserts an implicit memory barrier")
    A("between dispatches in a compute pass. WGSL's own barriers are")
    A("workgroup-scoped only. **Fusing two dispatches deletes the barrier that is")
    A("currently making the sequence correct** — and there is no device-wide")
    A("barrier to put back. A non-empty `write(A) ∩ read(B)` is therefore a bar on")
    A("fusing that pair, regardless of any other argument.")
    A("")
    A("**ONE CARVE-OUT, AND IT IS NOT AN EXCEPTION TO THE RULE — IT IS THE RULE")
    A("READ PROPERLY (SPINE_2).** The barrier the bar protects is a barrier BETWEEN")
    A("THREADS. Where both entry points are `@workgroup_size(1)` AND both dispatch")
    A("exactly one workgroup, the fused kernel is a SINGLE INVOCATION: the two")
    A("bodies run as straight-line code in one thread, and WGSL orders a read")
    A("after a write to the same memory location within one invocation by program")
    A("order alone. There is no barrier to delete because there was never a second")
    A("thread. Such pairs are marked EXEMPT rather than BARRED.")
    A("")
    A("The evidence is already in this ledger and is not re-derived here: witness")
    A("`W3-2` publishes both sets — the `@workgroup_size(1)` entry points and the")
    A("dispatches issuing ONE workgroup — and the carve-out is exactly their")
    A("intersection. The bar itself is unchanged for every other pair, and EXEMPT")
    A("is NOT a recommendation: cadence and shape are separate gates this census")
    A("cannot decide.")
    A("")
    A("Restricted to FUSION-ELIGIBLE ordered pairs: a fused kernel has ONE")
    A("pipeline layout, so only compute entry points whose pipelines carry")
    A("identical group layouts could ever be fused. %d such ordered pairs, of %d"
      % (len(e1["raw"]), len(e1["compute_eps"])))
    A("compute entry points. The unrestricted matrix is %d ordered pairs with %d"
      % (e1["raw_all_pairs"], e1["raw_all_nonempty"]))
    A("non-empty intersections; the restriction hides nothing, it removes pairs")
    A("that could not be fused for a different reason.")
    A("")
    # THE CARVE-OUT'S SET: one invocation, total. W3-2 owns both halves.
    solo = set(e3["wg1"]) & set(e3["single_dispatch"])
    exempt_rows = [r for r in e1["raw"]
                   if r["hazard"] and r["a"] in solo and r["b"] in solo]
    A("| write(A) | read(B) | bindings in write(A) ∩ read(B) | verdict |")
    A("|---|---|---|---|")
    for r in e1["raw"]:
        if not r["hazard"]:
            verdict = "no hazard on this ordering"
        elif r["a"] in solo and r["b"] in solo:
            verdict = "**EXEMPT** — one invocation, no barrier to delete"
        else:
            verdict = "**BARRED**"
        A("| `%s` | `%s` | %s | %s |"
          % (r["a"], r["b"], ", ".join("`%s`" % x for x in r["hazard"]) or "—",
             verdict))
    A("")
    A("%d ordered row(s) carry a hazard the single-invocation carve-out exempts%s."
      % (len(exempt_rows),
         (": " + ", ".join("`%s` → `%s`" % (r["a"], r["b"])
                           for r in exempt_rows)) if exempt_rows else ""))
    A("")
    A("### The unordered closure — and why the ordered table alone is the wrong question")
    A("")
    A("**A fused kernel has no ordering.** Its threads run concurrently inside one")
    A("dispatch. The table above answers *\"is this sequence safe as written\"*;")
    A("fusion asks *\"is this pair safe in either direction\"*, and those are")
    A("different questions. A pair is fusable only if BOTH orderings are")
    A("hazard-free, so the ordered table has to be asked twice and closed.")
    A("")
    clean = [u for u in e1["unordered"] if u["clean"]]
    A("| unordered pair | fusable on RAW? | barred direction(s) |")
    A("|---|---|---|")
    solo_pairs = [u for u in e1["unordered"]
                  if not u["clean"] and u["pair"][0] in solo
                  and u["pair"][1] in solo]
    for u in e1["unordered"]:
        both_solo = u["pair"][0] in solo and u["pair"][1] in solo
        if u["clean"]:
            fus = "**yes**"
        elif both_solo:
            fus = "**yes** — one invocation"
        else:
            fus = "no"
        A("| {`%s`, `%s`} | %s | %s |"
          % (u["pair"][0], u["pair"][1], fus,
             md_escape("; ".join("`%s` on %s" % (d, ", ".join(h))
                                 for d, h in u["barred_dirs"])) or "—"))
    A("")
    A("%d unordered pairs; **%d survive RAW in both directions**%s."
      % (len(e1["unordered"]), len(clean),
         (", and **%d more %s exempt as %s single-invocation pair%s** (%s) —"
          " RAW-barred on paper, unbarrable in fact"
          % (len(solo_pairs),
             "is" if len(solo_pairs) == 1 else "are",
             "a" if len(solo_pairs) == 1 else "",
             "" if len(solo_pairs) == 1 else "s",
             ", ".join("{`%s`, `%s`}" % u["pair"] for u in solo_pairs)))
         if solo_pairs else ""))
    if clean:
        A("")
        A("### The second gate: cadence")
        A("")
        A("**RAW-clean is necessary, not sufficient.** The second gate is CADENCE,")
        A("and it lives in the dispatch schedule, not the shader — which means the")
        A("census cannot decide it. It can only put the evidence next to the")
        A("survivors:")
        A("")
        A("| survivor pair | pipeline labels |")
        A("|---|---|")
        lbl = {}
        for p_ in cen["pipelines"]:
            if p_.kind == "compute":
                lbl[cen["entry_const"].get(p_.cs_entry)] = p_.label
        for u in clean:
            A("| {`%s`, `%s`} | %s |"
              % (u["pair"][0], u["pair"][1],
                 " / ".join(md_escape(lbl.get(x, "?")) for x in u["pair"])))
        A("")
        A("Every surviving pair couples an **on-demand** pass with a **per-frame**")
        A("one. Fusing them would make the on-demand pass run every frame — a")
        A("behaviour change, not a barrier change, and outside anything Tier A")
        A("can authorise.")
        A("")
        A("And the three patch-gen passes cannot all merge regardless: the pair")
        A("`{generate_patch_gradients, generate_patch_heights}` is barred on")
        A("`patch_height_scratch`, so the maximal RAW-clean set is a pair, never")
        A("the triple.")
        A("")
        A("**Net: the program has zero fusable pairs.** Not one that survives both")
        A("gates.")
        A("")

    # ─── 7. TABLE F — vertex-buffer eligibility (BUDGET_0f-2) ─────────
    A("## Table F — vertex-buffer eligibility")
    A("")
    A("Vertex buffers are the one wallet that can absorb a RUNTIME-SIZED ARRAY,")
    A("which is the class A2 structurally cannot touch. Eligibility is decided by")
    A("ACCESS PATTERN, not type: every access in every entry point that reaches")
    A("the binding must be `builtin_sequential` on the same builtin, and the")
    A("element stride must fit `maxVertexBufferArrayStride` (2048 B).")
    A("")
    A("**An `override` gate does not rescue a mixed classification.** An override")
    A("selects a branch inside ONE entry point, and one entry point has ONE")
    A("signature. A binding read both sequentially and indirected under an")
    A("override is blocked; moving it needs a second entry point, which is")
    A("another FXC compile. That cost is in the row, not omitted from it.")
    A("")
    A("A pipeline drawn with `instanceCount` 1 cannot host an instance-step")
    A("buffer whatever its access pattern says — Table G carries the counts.")
    A("")
    A("An `override gate` on an ELIGIBLE row is not a blocker but is a cost:")
    A("the access happens in only one pipeline variant, and the variants share")
    A("the entry point and therefore the vertex signature. Moving such a binding")
    A("obliges the sibling variant to bind the slot too, or splits the entry")
    A("point. `visible_patch_indices` was the row this applied to — DOMESDAY_0")
    A("B3 moved it and paid exactly that cost: the direct variant binds the")
    A("slot and leaves the attribute unread.")
    A("")
    A("| symbol | wgsl_type | stride | access classes | override gate | **verdict** | blockers |")
    A("|---|---|---|---|---|---|---|")
    for x in sorted(e2["eligibility"], key=lambda y: (not y["eligible"], y["decl"].symbol)):
        A("| `%s` | `%s` | %s | %s | %s | %s | %s |"
          % (x["decl"].symbol, md_escape(x["decl"].wgsl_type),
             "%s B" % x["stride"] if x["stride"] else "—",
             ", ".join("`%s(%s)`" % c for c in x["classes"]),
             md_escape("; ".join(x["override_gates"])) if x["override_gates"] else "—",
             "**ELIGIBLE (%s-step)**" % x["step"] if x["eligible"] else "blocked",
             md_escape("; ".join(x["blockers"])) or "—"))
    A("")
    A("### What an eligible move costs")
    A("")
    A("Eligibility is not freeness. A vertex buffer removes a bind-group binding")
    A("and spends vertex-buffer and vertex-attribute slots instead, and the")
    A("underlying buffer needs `BufferUsage::Vertex` alongside `Storage` — the")
    A("same class of program edit that A2 needs `BufferUsage::Uniform` for, and")
    A("the reason neither is free. Attribute count assumes vec4 packing,")
    A("`ceil(stride / 16)`.")
    A("")
    A("| symbol | pipelines reaching it | vertex buffers | vertex attributes | buffer usage |")
    A("|---|---|---|---|---|")
    prog_vb = max(p.vertex_buffer_count for p in cen["pipelines"])
    prog_va = max(p.vertex_attribute_count or 0 for p in cen["pipelines"])
    for x in e2["eligibility"]:
        if not x["eligible"]:
            continue
        sym = x["decl"].symbol
        touch = [p for p in cen["pipelines"]
                 for cc, _ in p.entries()
                 if sym in wgsl["reach"].get(cen["entry_const"].get(cc), (set(), set()))[1]]
        touch = sorted({p.label: p for p in touch}.items())
        need = -(-x["stride"] // 16)
        vb = ["%s %d→%d" % (lbl, p.vertex_buffer_count, p.vertex_buffer_count + 1)
              for lbl, p in touch]
        va = ["%s %s→%s" % (lbl, p.vertex_attribute_count,
                            (p.vertex_attribute_count or 0) + need) for lbl, p in touch]
        A("| `%s` | %s | %s | %s | `Storage` + **`Vertex`** |"
          % (sym, ", ".join(md_escape(lbl) for lbl, _ in touch) or "—",
             md_escape("; ".join(vb)), md_escape("; ".join(va))))
    A("")
    post_vb, post_va = prog_vb, prog_va
    for x in e2["eligibility"]:
        if not x["eligible"]:
            continue
        sym = x["decl"].symbol
        need = -(-x["stride"] // 16)
        for p in cen["pipelines"]:
            if any(sym in wgsl["reach"].get(cen["entry_const"].get(cc), (set(), set()))[1]
                   for cc, _ in p.entries()):
                post_vb = max(post_vb, p.vertex_buffer_count + 1)
                post_va = max(post_va, (p.vertex_attribute_count or 0) + need)
    # This passage used to be written around `render_orb_state` by name and
    # around a fixed count of "both" eligible moves. TETRIS ORB_V took that
    # move, which emptied the filter the attribute arithmetic reduced over
    # and left the prose describing a binding the program no longer has —
    # finding 12's lesson (a reference outliving its referent) landing in
    # the WRITER this time rather than in a witness key. Both are derived
    # from the eligibility set now, so neither can go stale again.
    n_elig = sum(1 for x in e2["eligibility"] if x["eligible"])
    orb_row = [x for x in e2["eligibility"]
               if x["eligible"] and x["decl"].symbol == "render_orb_state"]
    A("Program-wide today: **%d of 8** vertex buffers, **%d of 16** vertex"
      % (prog_vb, prog_va))
    if n_elig:
        A("attributes. With the %d remaining eligible move%s taken, the maxima would"
          % (n_elig, "" if n_elig == 1 else "s"))
        A("read **%d of 8** and **%d of 16**." % (post_vb, post_va))
    else:
        A("attributes. NO eligible move remains: every Table F candidate above is")
        A("blocked, so these maxima are where the program stands rather than a")
        A("floor under a reserve.")
    A("")
    if orb_row:
        A("Two facts in `render_orb_state`'s favour that belong in its row. There is")
        A("no Shadow Orb pipeline, so exactly one pipeline in the program reaches it —")
        A("a move touches one vertex signature, not a family. And its slot is one of")
        A("the sites the registry banner names as sharing a buffer under several")
        A("names (`orb_state` / `render_orb_state` / `orb_state_ro`), so retiring it")
        A("retires a registry site rather than relocating one.")
    else:
        A("`render_orb_state` is absent from the table above because TETRIS ORB_V")
        A("TOOK the move. The orb state reaches `orb_vs` through an instance-step")
        A("vertex buffer — one attribute per `OrbState` field, offsets mirroring the")
        A("struct — and the binding-400 seat is gone from the Render Entity Layout,")
        A("from `world.wgsl` and from the registry, where the trio of names on that")
        A("buffer is now the duo `orb_state` / `orb_state_ro`. The Orb Sky Layer is")
        A("the pipeline that paid: it carries the program's vertex-attribute maximum")
        A("of %d now, where the arch family's 4 stood before." % prog_va)
    A("")

    # ─── 8. TABLE G — call shapes (BUDGET_0f-3) ───────────────────────
    A("## Table G — call shapes")
    A("")
    A("What the HOST issues, against what the shader declares. Counts are")
    A("resolved to a literal, a named constant, or the expression at the call")
    A("site; a count spelled as a parameter name is the census failing to find")
    A("the caller, and witness `W3-3` forbids it.")
    A("")
    A("| pipeline | kind | entry point(s) | call | count | @workgroup_size | source |")
    A("|---|---|---|---|---|---|---|")
    for r in e3["rows"]:
        p_ = r["pipeline"]
        eps = ", ".join("`%s`" % cen["entry_const"].get(cc, cc) for cc, _ in p_.entries())
        wgz = ""
        if p_.kind == "compute":
            fnx = wgsl["entries"].get(cen["entry_const"].get(p_.cs_entry))
            wgz = fnx.workgroup_size if fnx else ""
        if not r["shapes"]:
            A("| %s | %s | %s | **NEVER INVOKED** | — | %s | — |"
              % (md_escape(p_.label), p_.kind, eps, wgz or "—"))
        for sh in r["shapes"]:
            A("| %s | %s | %s | `%s` | `%s` | %s | %s |"
              % (md_escape(p_.label), p_.kind, eps, sh["variant"],
                 md_escape(sh["count"]), wgz or "—",
                 md_escape(sh["src"] or sh["note"] or sh["where"])))
    A("")
    A("### The single-thread reconciliation (G3)")
    A("")
    A("Two different facts that a single census column would have merged:")
    A("`@workgroup_size(1)` is what the SHADER declares; a one-workgroup dispatch")
    A("is what the HOST issues.")
    A("")
    A("| entry point | @workgroup_size(1) | dispatches ONE workgroup |")
    A("|---|---|---|")
    for ep, a_, b_ in e3["recon"]:
        A("| `%s` | %s | %s |" % (ep, "yes" if a_ else "no", "yes" if b_ else "no"))
    A("")
    A("%d entry points declare `@workgroup_size(1)`; %d dispatches issue one"
      % (len(e3["wg1"]), len(e3["single_dispatch"])))
    A("workgroup; the intersection is %d. `update_other_agents` is the row that"
      % len(set(e3["wg1"]) & set(e3["single_dispatch"])))
    A("goes the other way — ONE workgroup at `@workgroup_size(32)`, which is why")
    A("the kernel-split banner says FXC \"inlines both branch bodies for every one")
    A("of 32 dispatched threads\".")
    A("")

    # ─── 9. TABLE H — the defended-site index (BUDGET_0f-4) ───────────
    A("## Table H — the defended-site index")
    A("")
    A("Where the program already paid to learn something. **This table cites; it")
    A("does not quote.** A reference outlives its referent, so the ledger carries")
    A("the symbol and the matched trigger tokens, and the reader greps. Copying")
    A("the prose in would give one fact two homes and guarantee the copy goes")
    A("stale.")
    A("")
    A("A proposal that touches a row here is a proposal that must read the prose")
    A("at that site before it argues with it.")
    A("")
    # PROBATE_E2 (P5, one home): this paragraph used to NARRATE the
    # 110 → 74 rebase and call 74 "the ruling of record". A hand-carried
    # number in a regenerated file is a number with two homes, and this
    # one went stale by six sites across two campaigns while still
    # calling itself the record. The count is computed now; the history
    # has a home that does not regenerate.
    A("%d sites at this run. The index's history — every rebase, with causes"
      % len(e4["sites"]))
    A("— lives in docs/FXC_LAWS_RECORD.md §index-history; a regenerated")
    A("artifact carries no hand-carried number (P5, one home).")
    A("")
    A("**The predicate, verbatim (W4-1).** A site is DEFENDED if its attached")
    A("comment matches any of:")
    A("")
    A("| trigger | pattern | sites | sole trigger at |")
    A("|---|---|---|---|")
    for name, pat in e4["triggers"]:
        fs = e4["per_token"].get(name, [])
        sole = sum(1 for f in fs if f.triggers == [name])
        A("| `%s` | `%s` | %s | %d |"
          % (name, md_escape(pat),
             "%d" % len(fs) if fs else "0 — **prospective**", sole))
    A("")
    A("Site counts are the guard on this predicate (`W4-3`). Two triggers were")
    A("ADDED to make the positive control pass, which is what a control is for —")
    A("but an instrument that can grant the variable under test is not an")
    A("instrument. A token contributing exactly one site, that site being a")
    A("control, would be a token written to pass the test; the witness fails on")
    A("it. Both added tokens are general vocabulary and neither is close.")
    A("")
    A("**A zero-site trigger is PROSPECTIVE, not dead.** A trigger list is a")
    A("predicate over FUTURE text, not a description of present data. Deleting a")
    A("term that matches nothing today would not make the predicate leaner, only")
    A("narrower — and this extension's whole posture is that over-flagging costs")
    A("a glance while under-flagging costs a 48-second lesson relearned. They")
    A("stay, marked, so the zero reads as intended rather than as a defect.")
    A("")
    A("Two readings worth keeping in view. `FXC` contributes %d sites and is the"
      % len(e4["per_token"].get("FXC", [])))
    A("sole trigger at %d of them — that is where this program's expensive lessons"
      % sum(1 for f in e4["per_token"].get("FXC", []) if f.triggers == ["FXC"]))
    A("actually cluster, and it is a map of the real risk surface. `time-cost`")
    A("contributes %d sites and is the sole trigger at %d, meaning every comment"
      % (len(e4["per_token"].get("time-cost", [])),
         sum(1 for f in e4["per_token"].get("time-cost", [])
             if f.triggers == ["time-cost"])))
    A("that records a measured duration also carries another trigger — the prose")
    A("conventions are internally consistent.")
    A("")
    A("Attachment: **A** the nearest preceding comment block with no other block")
    A("between it and the site (so a block covers the run that follows it);")
    A("**B** any block anywhere carrying a trigger AND naming the symbol; **C**")
    A("body comments, for WGSL functions. Both A and B are load-bearing — A")
    A("recovers `pawn_ground_resolve`, whose banner sits above an intervening")
    A("function; B recovers the two agent kernels, whose 48-second banner sits")
    A("several declarations upstream.")
    A("")
    A("The `line` column is **non-authoritative**: it is true for the commit in")
    A("the provenance header and stale for any other. Cite the SYMBOL. The line is")
    A("a convenience for finding it, not a reference to it — this table cites and")
    A("does not quote precisely so it cannot go stale, and a line number is the")
    A("one column that can.")
    A("")
    A("| symbol | kind | file | line (non-authoritative) | triggers | matched via |")
    A("|---|---|---|---|---|---|")
    for f in e4["sites"]:
        A("| `%s` | %s | `%s` | %d | %s | %s |"
          % (f.symbol, f.kind, f.file, f.line,
             ", ".join("`%s`" % t for t in f.triggers), ", ".join(f.rules)))
    A("")

    # ─── Appendices: 0b's two products, which A–D consume but do not show.
    A("## Appendix 1 — the WGSL declaration table (0b-i)")
    A("")
    A("%d module-scope declarations over %d `(group, binding)` slots. The three"
      % (len(wgsl["decls"]), len(wgsl["slots"])))
    A("aliases share slots with an earlier declaration and are marked.")
    A("")
    A("| wgsl_symbol | group | binding | address_space | wgsl_access | wgsl_type | "
      "runtime_array | bytes | align | uniform_legal | alias of |")
    A("|---|---|---|---|---|---|---|---|---|---|---|")
    first = {}
    for d in wgsl["decls"]:
        first.setdefault((d.group, d.binding), d.symbol)
    for d in sorted(wgsl["decls"], key=lambda d: (d.group, d.binding, d.line)):
        A("| `%s` | %d | %d | %s | %s | `%s` | %s | %s | %s | %s | %s |"
          % (d.symbol, d.group, d.binding, d.address_space, d.wgsl_access,
             md_escape(d.wgsl_type), "yes" if d.has_runtime_array else "no",
             d.layout.size if d.layout.size is not None else "—",
             d.layout.align if d.layout.align is not None else "—",
             "yes" if not d.layout.uniform_blockers else "no",
             ("`%s`" % first[(d.group, d.binding)])
             if first[(d.group, d.binding)] != d.symbol else "—"))
    A("")
    A("## Appendix 2 — the reachability closure (0b-ii)")
    A("")
    A("Per entry point: the transitive closure of function calls, then every")
    A("binding declaration referenced anywhere in that closure. Built textually")
    A("over the one module. The residual error is one-directional — a local")
    A("shadowing a binding name would ADD a reference, never remove one — so")
    A("every `vis_actual` here is generous and every A1 flag is conservative.")
    A("")
    A("Each binding carries what the closure DOES with it (BUDGET_0f-1):")
    A("`r` read, `w` written, `rw` both. Scope is resolved, so a local shadowing")
    A("a binding name is not counted as a reference to it.")
    A("")
    A("| entry_point | stage | workgroup_size | functions reached | bindings reached (use) |")
    A("|---|---|---|---|---|")
    for name in sorted(wgsl["entries"]):
        fn = wgsl["entries"][name]
        fns_reached, refs = wgsl["reach"][name]
        use = wgsl.get("ep_use", {}).get(name, {})
        A("| `%s` | %s | %s | %d | %s |"
          % (name, fn.stage, fn.workgroup_size or "—", len(fns_reached),
             ", ".join("`%s`(%s)" % (s, use.get(s, "?")) for s in sorted(refs))
             or "— none"))
    A("")
    A("## Appendix 4 — access-site classification (0f-2)")
    A("")
    A("Every access site whose index is NOT a plain scalar, plus every `other`")
    A("row individually — W2-2 forbids counting those in aggregate. The %d"
      % sum(1 for x in e2["sites"] if x["cls"].cls == "scalar"))
    A("`scalar` sites (constant, override, or a value from a uniform) are")
    A("summarised by binding rather than listed.")
    A("")
    A("| function | binding | index expression | class | use | override gate | line |")
    A("|---|---|---|---|---|---|---|")
    for x in sorted(e2["sites"], key=lambda y: (y["fn"], y["symbol"], y["line"])):
        if x["cls"].cls == "scalar":
            continue
        A("| `%s` | `%s` | `%s` | `%s` | %s | %s | %d |"
          % (x["fn"], x["symbol"], md_escape(x["index"]) or "(no index)",
             x["cls"], x["use"], md_escape(x["override_gate"]) or "—", x["line"]))
    A("")
    scal = {}
    for x in e2["sites"]:
        if x["cls"].cls == "scalar":
            scal[x["symbol"]] = scal.get(x["symbol"], 0) + 1
    A("Scalar-indexed sites by binding: %s."
      % ", ".join("`%s` %d" % (k, v) for k, v in sorted(scal.items())))
    A("")
    A("## Appendix 3 — pipelines and their group layouts (0c)")
    A("")
    A("Index position in `bindGroupLayouts` **is** the `@group` number. Shared")
    A("builders are resolved, not recorded.")
    A("")
    A("| pipeline | member | kind | vs | fs | cs | group_layouts (ordered) | vbufs | "
      "vattrs | color targets | roster_gate |")
    A("|---|---|---|---|---|---|---|---|---|---|---|")
    for p in cen["pipelines"]:
        ec = cen["entry_const"]
        A("| %s | `%s` | %s | %s | %s | %s | %s | %d | %s | %d | %s |"
          % (md_escape(p.label), p.member, p.kind,
             "`%s`" % ec.get(p.vs_entry, p.vs_entry) if p.vs_entry else "—",
             "`%s`" % ec.get(p.fs_entry, p.fs_entry) if p.fs_entry else "—",
             "`%s`" % ec.get(p.cs_entry, p.cs_entry) if p.cs_entry else "—",
             " → ".join("`%s`" % g for g in p.group_layouts),
             p.vertex_buffer_count,
             "—" if p.vertex_attribute_count is None else p.vertex_attribute_count,
             p.color_target_count, "`%s`" % p.roster_gate if p.roster_gate else "—"))
    A("")

    while o and not o[-1].strip():
        o.pop()                      # exactly one trailing LF, no blank tail
    text = "\n".join(o) + "\n"
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    return text


# ═══════════════════════════════════════════════════════════════════════
# REPORT
# ═══════════════════════════════════════════════════════════════════════

def main():
    ap = argparse.ArgumentParser(description="BUDGET_0 — the binding ledger census.")
    ap.add_argument("--check", action="store_true",
                    help="run the witnesses, write nothing")
    ap.add_argument("-o", "--out", default=DEFAULT_OUT)
    args = ap.parse_args()

    w = Witnesses()
    registry = parse_registry(w)
    layouts = parse_layouts(w, registry)

    print("BUDGET_0 — the binding ledger")
    print("")
    print("PHASE 0a — THE LAYOUT CENSUS")
    print("  %d bind group layouts, %d rows"
          % (len(layouts), sum(len(L["entries"]) for L in layouts)))
    for L in layouts:
        print("    %-34s %-32s %2d entr%s%s"
              % (L["label"], L["member"], len(L["entries"]),
                 "y" if len(L["entries"]) == 1 else "ies",
                 ("  [gate: %s]" % L["roster_gated"]) if L["roster_gated"] else ""))
    gated = [L for L in layouts if L["roster_gated"]]
    print("")
    print("  FINDING: %s"
          % ("no bind group layout creation sits inside a ROSTER gate — the "
             "binding surface is identical across every demo column"
             if not gated else
             "%d layout(s) are ROSTER-gated: %s"
             % (len(gated), ", ".join(L["label"] for L in gated))))
    b = phase_0b(w)
    print("")
    print("PHASE 0b — THE REACHABILITY CENSUS")
    print("  %d module-scope binding declarations over %d (group, binding) slots"
          % (len(b["decls"]), len(b["slots"])))
    print("  %d functions, %d entry points"
          % (len(b["fns"]), len(b["entries"])))
    sized = [d for d in b["decls"] if d.layout.size is not None]
    print("  %d declarations have a computable store size; %d do not "
          "(runtime-sized arrays and handle types)"
          % (len(sized), len(b["decls"]) - len(sized)))

    reached_any = set()
    for _, refs in b["reach"].values():
        reached_any |= refs
    dead = sorted(d.symbol for d in b["decls"] if d.symbol not in reached_any)
    print("")
    print("  REPORT: %s"
          % ("every module-scope binding declaration is reached by at least one entry point"
             if not dead else
             "%d declaration(s) reached by ZERO entry points — dead binding surface: %s"
             % (len(dead), ", ".join(dead))))

    c = phase_0c(w, layouts, b)
    print("")
    print("PHASE 0c — THE PIPELINE CENSUS")
    print("  %d pipelines (%d render, %d compute)"
          % (len(c["pipelines"]),
             sum(1 for p in c["pipelines"] if p.kind == "render"),
             sum(1 for p in c["pipelines"] if p.kind == "compute")))
    for p in c["pipelines"]:
        print("    %-30s %-28s %-7s %-46s vb=%d/%s ct=%d%s"
              % (p.label, p.member, p.kind,
                 "+".join(e for e, _ in p.entries()),
                 p.vertex_buffer_count,
                 "-" if p.vertex_attribute_count is None else p.vertex_attribute_count,
                 p.color_target_count,
                 ("  [gate: %s]" % p.roster_gate) if p.roster_gate else ""))
    print("")
    print("  group index of each bind group layout:")
    for g, i in sorted(c["group_index"].items(), key=lambda kv: (kv[1], kv[0])):
        print("    @group(%d)  %s" % (i, g))

    column = demo_column(w)
    d = phase_0d(w, layouts, b, c)
    e1 = phase_ext1(w, b, c, layouts)
    e2 = phase_ext2(w, b)
    e3 = phase_ext3(w, c, b)
    e4 = phase_ext4(w, b, layouts, c)
    writer_pins_lf(w)

    print("")
    print("PHASE 0d — THE JOIN, THE STAGE BUDGET, TIER A")
    print("  demo column censused: %s" % column)
    print("")
    print("  the four tightest (pipeline, stage) rows, declared / actual / limit:")
    order = sorted(d["budget"], key=lambda r: -max(r["declared"][k] / CORE[k] for k in CORE))
    for r in order[:4]:
        print("    %-32s %s   %s" % (r["pipeline"].label, r["stage"],
                                     "  ".join("%s %d/%d/%d" % (k, r["declared"][k],
                                                                r["actual"][k], CORE[k])
                                               for k in ("uniform", "storage", "sampled",
                                                         "samplers", "storagetex"))))
    main_v = next((r for r in d["budget"]
                   if r["pipeline"].member == "pawnPipeline_" and r["stage"] == "V"), None)
    if main_v:
        print("")
        print("  the main render family's VERTEX stage (renderEntityLayout_ + "
              "renderTextureLayout_): storage %d of %d"
              % (main_v["declared"]["storage"], CORE["storage"]))
        for x in main_v["detail"]["storage"]:
            print("      %s" % x)

    print("")
    print("  TIER A: %d A1 (over-visible), %d A2 candidates (%d pass all predicates), %d A3"
          % (len(d["a1"]), len(d["a2"]), sum(1 for x in d["a2"] if x["pass"]), len(d["a3"])))
    for r in d["a1"]:
        print("    A1  %-30s [%2d] %-22s declared %-3s  actual %-3s  delta %s"
              % (r["layout"]["label"], r["e"].entry_index, r["primary"],
                 r["e"].vis_declared, r["vis_actual"] or "-", r["vis_delta"]))
    for kind, what, _ in d["a3"]:
        print("    A3  %s — %s" % (kind, what))
    print("  stale mirror rows (registry name != WGSL name): %d" % len(d["stale_mirror"]))

    print("")
    print("EXTENSION 1 — READ/WRITE SEPARATION")
    shadow = {}
    for fn in b["fns"].values():
        for s in getattr(fn, "shadowed", ()) or ():
            shadow.setdefault(s, []).append(fn.name)
    print("  scope resolution: %d module-scope name(s) shadowed by a local — %s"
          % (len(shadow), "; ".join("%s in %s" % (k, ", ".join(sorted(v)))
                                    for k, v in sorted(shadow.items())) or "none"))
    print("  RAW hazards, fusion-eligible ordered pairs (same pipeline layout):")
    for r in e1["raw"]:
        print("    %-24s -> %-24s %s"
              % (r["a"], r["b"], ", ".join(r["hazard"]) or "EMPTY — no hazard"))
    print("  A4 (OVER-PRIVILEGED, declared read_write and never written): %d"
          % len(e1["a4"]))
    for x in e1["a4"]:
        print("    %-26s readers: %s" % (x["decl"].symbol, ", ".join(x["readers"])))

    print("")
    print("EXTENSION 2 — ACCESS-PATTERN CLASSIFICATION")
    print("  overrides in world.wgsl: %s" % (", ".join(e2["overrides"]) or "none"))
    elig = [x for x in e2["eligibility"] if x["eligible"]]
    print("  vertex-buffer eligible bindings: %d" % len(elig))
    for x in elig:
        print("    %-24s %s-step, stride %s B, %d access site(s)"
              % (x["decl"].symbol, x["step"], x["stride"], len(x["sites"])))
    print("  the bindings the survey named (visible_patch_indices spent by "
          "DOMESDAY_0 B3 — absent rows print nothing):")
    for name in ("patch_instances", "render_ring_xforms", "visible_patch_indices"):
        x = next((y for y in e2["eligibility"] if y["decl"].symbol == name), None)
        if x:
            print("    %-22s %-9s %s" % (name, "ELIGIBLE" if x["eligible"] else "blocked",
                                         "; ".join(x["blockers"]) or "-"))
    if e2["others"]:
        grp = {}
        for s_ in e2["others"]:
            grp.setdefault((s_["symbol"], s_["cls"].source), []).append(s_["fn"])
        print("  `other` access sites: %d, over %d (binding, provenance) groups — every"
              " one enumerated individually in the artifact:" % (len(e2["others"]), len(grp)))
        for (sym, src), fs in sorted(grp.items()):
            print("    %-22s %-34s in %s" % (sym, src, ", ".join(sorted(set(fs)))[:60]))

    print("")
    print("EXTENSION 3 — CALL-SHAPE CENSUS")
    print("  caller files scanned for invocation sites: %s"
          % ", ".join(os.path.relpath(x, REPO) for x in e3["callers"]))
    for r in e3["rows"]:
        p_ = r["pipeline"]
        if not r["shapes"]:
            print("    %-38s %s" % (p_.label, "NEVER INVOKED — finding"))
        for sh in r["shapes"]:
            print("    %-38s %-20s count=%-28s %s"
                  % (p_.label, sh["variant"], sh["count"],
                     sh["note"] or sh["where"]))
    print("")
    print("EXTENSION 4 — THE DEFENDED-SITE INDEX")
    print("  %d defended sites over %d files" % (len(e4["sites"]),
          len({f.file for f in e4["sites"]})))
    byfile = {}
    for f in e4["sites"]:
        byfile.setdefault(f.file, []).append(f)
    for fl, items in sorted(byfile.items()):
        print("    %s: %d" % (fl, len(items)))
    # LOOM_3: read from the schema, and note what this line used to be — it
    # still said "six" and still named `Render Entity Layout entries[16]/[17]`,
    # keys W4-2's control abandoned two re-keys before this sweep found them.
    # A stale console line is a small lie, but it is the same lie.
    _ctl_keys = {row["key"] for row in schema.DEFENDED_SITES}
    print("  the %d positive-control sites:" % len(_ctl_keys))
    for f in e4["sites"]:
        if f.symbol in _ctl_keys:
            print("    %-36s %-16s %s:%d  [%s]  via %s"
                  % (f.symbol, f.kind, f.file, f.line, ", ".join(f.triggers),
                     ", ".join(f.rules)))

    # PROBATE_E1: derived BEFORE the gate so E1-identity can fail the run.
    room = room_family_census(d["budget"], c, d, w)

    print("")
    print("WITNESSES")
    shallow_note()   # L29
    w.report()

    if w.failures():
        print("")
        print("STOP — %d witness(es) failed. Nothing written. The ledger is not "
              "trustworthy until Jean rules which side is wrong." % len(w.failures()))
        return 1

    if args.check:
        print("")
        # GATES_2 — the witnesses pass on the live tree; now ask whether the
        # ledger on disk was taken from it.
        if report_stale(args.out, [WORLD_WGSL]):
            return 1
        print("--check: all witnesses pass, the ledger's pins match the live "
              "inputs, nothing written.")
        return 0

    text = emit(args.out, w, column, layouts, b, c, d, e1, e2, e3, e4, room)
    print("")
    print("PHASE 0e — THE ARTIFACT")
    print("  wrote %s (%d lines, %d bytes)"
          % (os.path.relpath(args.out, REPO), text.count("\n"), len(text.encode("utf-8"))))
    bx = verify_bytes(args.out)
    clean = bx["crlf"] == 0 and bx["cr"] == 0 and not bx["bom"] and bx["trailing"]
    print("  byte check: %d CRLF, %d bare CR, %d LF, BOM %s, single trailing LF %s -> %s"
          % (bx["crlf"], bx["cr"], bx["lf"], bx["bom"], bx["trailing"],
             "LF-CLEAN" if clean else "FAIL"))
    if not clean:
        print("")
        print("STOP — the artifact violates the LF-only encoding law (L1). "
              "The file on disk is not what the tool intended to write.")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
