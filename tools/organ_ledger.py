#!/usr/bin/env python3
# ═══════════════════════════════════════════════════════════════════════
# THE ORGAN'S OWN BOOK — organ_ledger.py
#
# It emits audit/ORGAN.md: one committed markdown table over every
# enrolled row, the tallies that summarise it, and the tails of both check
# tools underneath. A book and not only the tools, for two reasons:
#
#   · SEARCHABILITY. "What is the range on the cohesion radius" should be
#     answerable by grepping the tree, not by opening a panel.
#   · THE COUPLING MENU. A coupling is a parameter set into trajectory
#     over time, so EVERY ROW WITH AN AUTHORED RANGE IS A TRAJECTORY
#     DOMAIN: this table is the music campaign's target map.
#
# USAGE   organ_ledger.py · --check to print · -o PATH to write elsewhere
# ═══════════════════════════════════════════════════════════════════════

# THE PARSER IS tools/organ_parse.py, the one organ_gap.py and
# organ_readers.py import, because three copies of one reading can drift.
# What is added here is the DERIVATION the C++ does: `derived_cadence()`
# restated once, against the rules organ_registry.hpp states in C++, so
# the book and the manifest cannot disagree about what a row means. If
# they ever do, this file is wrong.

# THE DOOR TABLE is parsed out of organ_registry.hpp's `kOrganDoors`
# rather than restated, for the same reason. LF, no BOM, single trailing
# newline — pinned by the writer and proved by a byte-level read-back.

import os
import re
import subprocess
import sys

# tools/ is sys.path[0] when a tool runs as a script; the insert is for the
# ledger's subprocess runs and for any caller importing a tool from elsewhere.
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from organ_parse import MACRO, split_args

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INC = os.path.join(ROOT, "src", "console", "organ_params.inc")
REG = os.path.join(ROOT, "src", "console", "organ_registry.hpp")
OUT = os.path.join(ROOT, "audit", "ORGAN.md")

# ─── THE ENROLLMENT LIST (the shared parser) ──────────────────────────

# The sentinel a definition-only family lands on. Mirrors
# ORGAN_DEFONLY_BLOCK_##DEFKIND in organ_registry.hpp; the convention
# DESCENDS from 255 and a third family adds one line there and one here.
DEFONLY_BLOCK = {"MOOD": "NONE (255)", "ORB_MOOD": "NONE_ORB (254)"}


def num(tok):
    """`0.005f` -> `0.005` · `the_board::FIELD_BEACON_S_MAX` -> itself.

    A range bound that is a NAMED CONSTANT stays named: the beacon's
    ceiling is `FIELD_K − 1` and printing 299 would hide the law it
    carries.
    """
    t = tok.strip()
    m = re.fullmatch(r"(-?[0-9]*\.?[0-9]+)f?", t)
    if not m:
        return t.replace("the_board::", "")
    v = float(m.group(1))
    return ("%g" % v)


def derived_cadence(row):
    """organ_registry.hpp's `derived_cadence()`, restated once.

    Order matters, exactly as it does there: a witness is DRIVEN even if
    its block has a boundary, because the row is a meter and a meter's
    cadence is its author's.
    """
    if row["ro"]:
        return "driven"
    if row["defkind"] != "NONE" or row["defonly"] or row["block"] == "ORBS":
        return "boundary"
    return row["cad"]


def rows():
    out = []
    with open(INC, encoding="utf-8") as f:
        for n, raw in enumerate(f, 1):
            m = MACRO.match(raw.strip())
            if not m:
                continue
            form, ns, args = m.group(1), m.group(2), split_args(m.group(3))
            k = 1 if ns else 0
            r = {"line": n, "form": form, "ns": (args[0] if ns else "the_board"),
                 "ro": False, "defonly": False, "defkind": "NONE",
                 "cad": "live", "minv": "", "maxv": "", "step": ""}
            if form in ("ORGAN_PARAM", "ORGAN_PARAM_GEN") and len(args) >= 9 + k:
                r.update(block=args[0 + k], struct=args[1 + k], field=args[2 + k],
                         type=args[3 + k].replace("ORGAN_", ""),
                         minv=num(args[4 + k]), maxv=num(args[5 + k]),
                         step=num(args[6 + k]),
                         group=args[7 + k].strip('"'), label=args[8 + k].strip('"'))
                if form == "ORGAN_PARAM_GEN":
                    r["cad"] = "gen"
            elif form == "ORGAN_PARAM_RO" and len(args) >= 6 + k:
                r.update(block=args[0 + k], struct=args[1 + k], field=args[2 + k],
                         type=args[3 + k].replace("ORGAN_", ""),
                         group=args[4 + k].strip('"'), label=args[5 + k].strip('"'),
                         ro=True)
            elif form == "ORGAN_PARAM_DEF" and len(args) >= 12 + k:
                r.update(block=args[0 + k], struct=args[1 + k], field=args[2 + k],
                         type=args[3 + k].replace("ORGAN_", ""),
                         minv=num(args[4 + k]), maxv=num(args[5 + k]),
                         step=num(args[6 + k]),
                         group=args[7 + k].strip('"'), label=args[8 + k].strip('"'),
                         defkind=args[9 + k],
                         defstruct=args[10 + k], deffield=args[11 + k])
            elif form == "ORGAN_PARAM_DEFONLY" and len(args) >= 9 + k:
                # TYPE MIN MAX STEP GROUP LABEL DEFKIND DEFSTRUCT DEFFIELD —
                # no BLOCK/STRUCT/FIELD at all, because there is no instance.
                r.update(block=DEFONLY_BLOCK.get(args[6 + k], "NONE?"),
                         struct=args[7 + k], field=args[8 + k],
                         type=args[0 + k].replace("ORGAN_", ""),
                         minv=num(args[1 + k]), maxv=num(args[2 + k]),
                         step=num(args[3 + k]),
                         group=args[4 + k].strip('"'), label=args[5 + k].strip('"'),
                         defkind=args[6 + k], defstruct=args[7 + k],
                         deffield=args[8 + k], defonly=True)
            else:
                continue
            # the id, exactly as organ_manifest builds it
            r["id"] = ("%s.%s" % (r["struct"], r["field"])) if r["defonly"] \
                else ("%s.%s" % (r["block"].split(" ")[0], r["field"]))
            r["section"] = r["group"].split(" · ")[0]
            r["cadence"] = derived_cadence(r)
            out.append(r)
    return out


# ─── THE DOORS (parsed, not restated) ─────────────────────────────────
def doors():
    text = open(REG, encoding="utf-8").read()
    m = re.search(r"kOrganDoors\[\]\s*=\s*\{(.*?)\n\};", text, re.S)
    if not m:
        return []
    return re.findall(r"\{\s*(ORGAN_DOOR_\w+)\s*,\s*\"([^\"]*)\"\s*\}", m.group(1))


def blocks_used(rs):
    return sorted({r["block"] for r in rs})


def tail_of(tool, keep):
    """The last `keep` lines of a check tool's stdout, verbatim."""
    try:
        p = subprocess.run([sys.executable, os.path.join(ROOT, "tools", tool)],
                           capture_output=True, text=True, timeout=600)
    except (OSError, subprocess.SubprocessError) as e:
        return ["(%s did not run: %s)" % (tool, e)]
    lines = (p.stdout or "").rstrip("\n").split("\n")
    return lines[-keep:] if len(lines) > keep else lines


def tally(rs, key):
    counts = {}
    for r in rs:
        counts[r[key]] = counts.get(r[key], 0) + 1
    return counts


def emit():
    rs = rows()
    L = []
    A = L.append

    A("# ORGAN — the organ's own book")
    A("")
    A("<!-- GENERATED by tools/organ_ledger.py from src/console/organ_params.inc")
    A("     and src/console/organ_registry.hpp — do not hand-edit.")
    A("     Regenerate: python3 tools/organ_ledger.py -->")
    A("")
    A("The audit family's fifth member. BINDING, COMMAND, MANIFEST and MIRROR")
    A("each keep a book about one of the program's rooms; this is the panel's.")
    A("")
    A("**A coupling is a parameter set into trajectory over time**, so every")
    A("row below with an authored range is a TRAJECTORY DOMAIN. This table is")
    A("the music campaign's target map, not panel decoration: the range column")
    A("is the domain a trajectory would play over, and the cadence column says")
    A("whether playing it would be heard now, at the frame boundary, or at the")
    A("author's next natural event.")
    A("")
    A("Cadence is DERIVED, never stored — the rule lives once in")
    A("`organ_registry.hpp::derived_cadence()` and is restated once in this")
    A("generator, so the book and the manifest cannot disagree. `driven` is an")
    A("`_RO` witness: the panel meters it and `organ_set` refuses to write it.")
    A("")

    A("## Every enrolled row")
    A("")
    A("| section | label | id | block / family | type | range | step | cadence | def-kind | ro |")
    A("| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |")
    for r in rs:
        rng = "—" if r["ro"] else "%s … %s" % (r["minv"], r["maxv"])
        stp = "—" if r["ro"] else r["step"]
        group = r["group"].split(" · ")
        grp = group[1] if len(group) > 1 else ""
        A("| %s · %s | %s | `%s` | %s | %s | %s | %s | %s | %s | %s |"
          % (r["section"], grp, r["label"], r["id"], r["block"], r["type"],
             rng, stp, r["cadence"], r["defkind"].lower(),
             "•" if r["ro"] else ""))
    A("")

    A("## The tallies")
    A("")
    A("| | |")
    A("| --- | --- |")
    A("| entries | **%d** |" % len(rs))
    A("| by section | %s |" % " · ".join(
        "%s %d" % (k, v) for k, v in sorted(tally(rs, "section").items(),
                                            key=lambda kv: -kv[1])))
    A("| by cadence | %s |" % " · ".join(
        "%s %d" % (k, v) for k, v in sorted(tally(rs, "cadence").items())))
    A("| by macro form | %s |" % " · ".join(
        "%s %d" % (k.replace("ORGAN_PARAM", "PARAM"), v)
        for k, v in sorted(tally(rs, "form").items())))
    A("| definition kinds | %s |" % " · ".join(
        "%s %d" % (k, v) for k, v in sorted(tally(rs, "defkind").items())))
    A("| witnesses (`ro`) | %d |" % sum(1 for r in rs if r["ro"]))
    A("| blocks and sentinels used | %s |" % ", ".join(blocks_used(rs)))
    A("| namespaces | %s |" % " · ".join(
        "%s %d" % (k, v) for k, v in sorted(tally(rs, "ns").items())))
    A("")
    A("### Doors")
    A("")
    A("A door is the panel pressing the program's OWN machinery: it raises")
    A("flags the frame boundary already consumes and adds no author. Parsed")
    A("from `kOrganDoors`, never restated.")
    A("")
    A("| id | label |")
    A("| --- | --- |")
    for i, (sym, label) in enumerate(doors()):
        A("| %d `%s` | %s |" % (i, sym, label))
    A("")

    A("## THE GAP")
    A("")
    A("`tools/organ_gap.py` — members of the enrolled homes the panel does")
    A("NOT name, and the reader witness over every graduated pair. The tail")
    A("of its run, verbatim:")
    A("")
    A("```")
    for line in tail_of("organ_gap.py", 28):
        A(line)
    A("```")
    A("")

    A("## THE READERS")
    A("")
    A("`tools/organ_readers.py` — does your reader name you? An enrollment")
    A("states a belief; only the reader proves it. The tail of its run,")
    A("verbatim:")
    A("")
    A("```")
    for line in tail_of("organ_readers.py", 16):
        A(line)
    A("```")
    # exactly one trailing newline, so the byte check below can prove it
    return "\n".join(L).rstrip("\n") + "\n"


def main():
    """PLUMB_0 A4 — `--check` compares; it used to narrate.

    It wrote the emission to stdout and returned 0. That is a REPORT: it
    proved the tool still runs, and nothing else. CLAUDE.md's gate table
    listed it as an assertion, so audit/ORGAN.md could drift from the
    enrollment list for a whole campaign with the row green — the same
    disease A1 cured in the command census, in the same family of tools.

    The comparison is against the ARTIFACT, byte for byte, because for this
    tool the emission IS the claim: there is no separate pin stanza to
    check. A mismatch names the first differing line, which is the one thing
    a reader needs to tell a stale regeneration from a hand edit (L28).
    """
    text = emit()
    out = OUT
    if "-o" in sys.argv:
        out = sys.argv[sys.argv.index("-o") + 1]
    rel = os.path.relpath(out, ROOT).replace(os.sep, "/")
    if "--check" in sys.argv:
        try:
            with open(out, "r", encoding="utf-8", newline="") as f:
                live = f.read()
        except OSError:
            print("STALE: %s is absent, so it asserts nothing. "
                  "Regenerate: python3 tools/organ_ledger.py" % rel)
            return 1
        if live != text:
            want = text.split("\n")
            have = live.split("\n")
            n = 0
            while n < len(want) and n < len(have) and want[n] == have[n]:
                n += 1
            print("STALE: %s disagrees with this tool's emission on the live "
                  "tree, from line %d." % (rel, n + 1))
            print("  committed: %s"
                  % (have[n].rstrip() if n < len(have) else "(end of file)"))
            print("  emitted:   %s"
                  % (want[n].rstrip() if n < len(want) else "(end of file)"))
            print("  Regenerate: python3 tools/organ_ledger.py "
                  "(and never hand-edit audit/ — L28).")
            return 1
        print("--check: %s matches this tool's emission on the live tree, "
              "%d lines, nothing written." % (rel, text.count("\n")))
        return 0
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    # G2-eol, the binding_ledger precedent: the writer pins the terminator
    # and a byte-level read-back proves it, so no host can translate it.
    raw = open(out, "rb").read()
    print("wrote %s (%d lines, %d bytes)"
          % (os.path.relpath(out, ROOT).replace(os.sep, "/"),
             raw.count(b"\n"), len(raw)))
    print("  byte check: %d CRLF, %d bare CR, %d LF, BOM %s, single trailing LF %s -> %s"
          % (raw.count(b"\r\n"), raw.count(b"\r") - raw.count(b"\r\n"),
             raw.count(b"\n"), raw[:3] == b"\xef\xbb\xbf",
             raw.endswith(b"\n") and not raw.endswith(b"\n\n"),
             "LF-CLEAN" if b"\r" not in raw and raw[:3] != b"\xef\xbb\xbf"
             else "DIRTY"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
