#!/usr/bin/env python3
# ═══════════════════════════════════════════════════════════════════════
# G-LAW 2 — THE WGSL STRUCTURAL WITNESS (stand-in)
#
# The law wants naga/Tint. Neither is installed in this container and
# `npx @webgpu/naga-cli` cannot reach a registry, so this is a STAND-IN
# and its limits are stated up front rather than discovered later.
#
# WHAT IT PROVES
#   1. delimiters balance ({} () []) across the whole file;
#   2. every function-call site resolves to a declared `fn` or to a name
#      in the BASELINE-CALIBRATED builtin set;
#   3. every reference to a module-scope symbol resolves to a declaration;
#   4. the entry-point set is exactly the recorded baseline;
#   5. nothing references a symbol the baseline had and this tree lost.
#
# WHAT IT DOES NOT PROVE
#   types, address spaces, override-expression legality, uniform layout
#   rules, or anything else a real front end checks. A GREEN here means
#   "no dangling name and no structural break" — which is precisely the
#   failure mode a DELETION can cause, and precisely why it is enough of
#   a witness for a pruning campaign and not enough for a feature.
#
# SELF-CALIBRATION. WGSL builtins are not enumerated here. The baseline
# file supplies them: every called-but-undeclared name at baseline IS a
# builtin (the tree parses on Dawn today, so it cannot be anything else).
# Any called-but-undeclared name that appears AFTER the baseline is a
# regression by construction.
#
# ...WITH ONE HOLE, and PREDECLARED is the patch. The inference above
# reads "builtin" off USE, so a builtin the shader never happened to call
# at baseline is absent from the set, and the first legitimate use of it
# afterwards is reported as a call to an undeclared name. That is a false
# positive about the LANGUAGE, not a finding about the tree. Language
# facts do not belong in a snapshot, so they live here instead — and this
# set is deliberately not a full builtin list, only the names that have
# actually tripped it.
#
#   python3 tools/gates/glaw2/run.py --record   # freeze the baseline
#   python3 tools/gates/glaw2/run.py            # check against it
# ═══════════════════════════════════════════════════════════════════════
"""G-LAW 2 stand-in: no dangling name, no structural break in world.wgsl."""

import io
import json
import os
import re
import sys

WGSL = "src/cartridges/the_board/realization/world.wgsl"
BASE = "tools/gates/glaw2/baseline.json"

# WGSL predeclared names the baseline could not have captured, because the
# shader did not call them at record time. See SELF-CALIBRATION above.
#   array     — predeclared type-generator; `array<T, N>(...)` is a
#               constructor, so CALL matches it like a function.
#   atomicSub — predeclared builtin function (atomicAdd was already in the
#               recorded set; its sibling simply had no use yet).
#   dpdx, dpdy, textureSampleGrad  predeclared builtin functions (MIP_0's
#               sample_exhibition is their first use: the derivatives are
#               taken in uniform control flow and handed to the explicit-
#               gradient sample; textureSample and textureSampleLevel were
#               in the recorded set).
#   textureGatherCompare  predeclared builtin function (the GATHER_0 pcf
#               kernels are its first use).
PREDECLARED = {"array", "atomicSub", "dpdx", "dpdy", "textureSampleGrad", "textureGatherCompare"}

IDENT = re.compile(r"[A-Za-z_]\w*")
CALL = re.compile(r"\b([A-Za-z_]\w*)\s*\(")
FN = re.compile(r"\bfn\s+([A-Za-z_]\w*)\s*\(")
CONST = re.compile(r"^const\s+([A-Za-z_]\w*)", re.M)
OVERRIDE = re.compile(r"^override\s+([A-Za-z_]\w*)", re.M)
STRUCT = re.compile(r"^struct\s+([A-Za-z_]\w*)", re.M)
BINDING = re.compile(
    r"@group\s*\(\s*\d+\s*\)\s*@binding\s*\(\s*\d+\s*\)\s*var\s*(?:<[^>]*>)?\s*([A-Za-z_]\w*)")
ENTRY = re.compile(r"@(compute|vertex|fragment)\b[^\n]*\n\s*fn\s+([A-Za-z_]\w*)")
# `let`/`var`/`const` inside a body, plus struct members and parameters, are
# locals; they shadow nothing at module scope and must not be resolved here.
LOCAL = re.compile(r"\b(?:let|var|const)\s+([A-Za-z_]\w*)")


def blank(m):
    return re.sub(r"[^\n]", " ", m.group(0))


def strip(s):
    s = re.sub(r"/\*.*?\*/", blank, s, flags=re.S)
    return re.sub(r"//[^\n]*", blank, s)


def lineno(t, p):
    return t.count("\n", 0, p) + 1


def parse(path):
    raw = io.open(path, encoding="utf-8").read()
    t = strip(raw)
    return {
        "text": t,
        "fns": sorted(set(FN.findall(t))),
        "consts": sorted(set(CONST.findall(t))),
        "overrides": sorted(set(OVERRIDE.findall(t))),
        "structs": sorted(set(STRUCT.findall(t))),
        "bindings": sorted(set(BINDING.findall(t))),
        "entries": sorted({n for _s, n in ENTRY.findall(t)}),
    }


def balance(t):
    """Delimiter balance with the offending line, not just a boolean."""
    pairs = {")": "(", "}": "{", "]": "["}
    stack = []
    for i, ch in enumerate(t):
        if ch in "({[":
            stack.append((ch, i))
        elif ch in ")}]":
            if not stack or stack[-1][0] != pairs[ch]:
                return "unmatched `%s` at line %d" % (ch, lineno(t, i))
            stack.pop()
    if stack:
        ch, i = stack[-1]
        return "unclosed `%s` opened at line %d" % (ch, lineno(t, i))
    return None


def main():
    if not os.path.exists(WGSL):
        sys.stderr.write("glaw2: run from the repo root\n")
        return 2
    cur = parse(WGSL)
    record = "--record" in sys.argv

    bad = balance(cur["text"])
    if bad:
        sys.stderr.write("G-LAW 2: RED — %s\n" % bad)
        return 1

    declared = set(cur["fns"]) | set(cur["consts"]) | set(cur["structs"]) \
        | set(cur["bindings"]) | set(cur["overrides"])
    called = set(CALL.findall(cur["text"]))
    unresolved_calls = called - set(cur["fns"]) - declared

    if record:
        os.makedirs(os.path.dirname(BASE), exist_ok=True)
        with io.open(BASE, "w", encoding="utf-8", newline="\n") as f:
            json.dump({
                "note": "builtins = called-but-undeclared at the baseline; the "
                        "tree parsed on Dawn at this commit, so they cannot be "
                        "anything else",
                "builtins": sorted(unresolved_calls),
                "entries": cur["entries"],
                "declared": sorted(declared),
            }, f, indent=1, sort_keys=True)
        print("G-LAW 2: baseline recorded — %d builtins, %d entry points, "
              "%d module symbols" % (len(unresolved_calls), len(cur["entries"]),
                                     len(declared)))
        return 0

    if not os.path.exists(BASE):
        sys.stderr.write("glaw2: no baseline; run --record first\n")
        return 2
    base = json.load(io.open(BASE, encoding="utf-8"))
    builtins = set(base["builtins"]) | PREDECLARED

    errs = []
    new_unresolved = sorted(unresolved_calls - builtins)
    for n in new_unresolved:
        m = re.search(r"\b" + re.escape(n) + r"\s*\(", cur["text"])
        errs.append("call to undeclared `%s` (first at line %d)"
                    % (n, lineno(cur["text"], m.start()) if m else 0))

    # A reference to a module symbol the baseline had and this tree lost.
    lost = set(base["declared"]) - declared
    locals_ = set(LOCAL.findall(cur["text"]))
    for n in sorted(lost):
        if n in locals_:
            continue          # re-declared as a local somewhere; not dangling
        for m in re.finditer(r"\b" + re.escape(n) + r"\b", cur["text"]):
            errs.append("deleted symbol `%s` still referenced at line %d"
                        % (n, lineno(cur["text"], m.start())))
            break

    ent_lost = sorted(set(base["entries"]) - set(cur["entries"]))
    ent_new = sorted(set(cur["entries"]) - set(base["entries"]))
    for n in ent_lost:
        errs.append("entry point REMOVED: `%s`" % n)
    for n in ent_new:
        errs.append("entry point ADDED: `%s`" % n)

    # ── THE POLICY MIRROR ─────────────────────────────────────────────
    # The C++ `enum PolicyId` and the WGSL `const POLICY_* : u32` are two
    # independent literal lists, and the enum value doubles as the row index
    # into POLICIES[] (ASSERT_POLICY_DAG_CLOSED indexes with it). Naming the
    # switch selectors binds arm to constant WITHIN the shader; nothing binds
    # the shader's numbers to the header's. A renumber that updates one room
    # and not the other is silent in both. This is that check.
    GA = "src/cartridges/the_board/contracts/ground_architecture.hpp"
    if os.path.exists(GA):
        hpp = io.open(GA, encoding="utf-8").read()
        em = re.search(r"enum PolicyId\s*:\s*uint32_t\s*\{(.*?)\}", hpp, re.S)
        cpp_pol = {}
        if em:
            for n, v in re.findall(r"(POLICY_\w+)\s*=\s*(\d+)", em.group(1)):
                if n != "POLICY_COUNT":
                    cpp_pol[n] = int(v)
        wgsl_pol = {n: int(v) for n, v in
                    re.findall(r"^const\s+(POLICY_\w+?)\s*:\s*u32\s*=\s*(\d+)u;",
                               cur["text"], re.M)
                    if not n.endswith("_MASK")}
        for n in sorted(set(cpp_pol) | set(wgsl_pol)):
            c, w = cpp_pol.get(n), wgsl_pol.get(n)
            if c is None:
                errs.append("policy mirror: `%s` = %d in WGSL, absent from the "
                            "C++ enum" % (n, w))
            elif w is None:
                # a C++ policy with no WGSL constant is legal — it is realized
                # by another mechanism and has no arm (R8 class B)
                pass
            elif c != w:
                errs.append("policy mirror: `%s` is %d in C++ and %d in WGSL"
                            % (n, c, w))
        # The contributor enum is the same species: its values are shift
        # counts (`1u << CONTRIB_X`), so a one-room renumber silently
        # re-points every mask bit.
        ec = re.search(r"enum ContributorId\s*:\s*uint32_t\s*\{(.*?)\}", hpp, re.S)
        cpp_c = {}
        if ec:
            for n, v in re.findall(r"(CONTRIB_\w+)\s*=\s*(\d+)", ec.group(1)):
                cpp_c[n] = int(v)
        wgsl_c = {n: int(v) for n, v in
                  re.findall(r"^const\s+(CONTRIB_\w+)\s*:\s*u32\s*=\s*(\d+)u;",
                             cur["text"], re.M)}
        for n in sorted(set(cpp_c) & set(wgsl_c)):
            if cpp_c[n] != wgsl_c[n]:
                errs.append("contributor mirror: `%s` is %d in C++ and %d in WGSL"
                            % (n, cpp_c[n], wgsl_c[n]))
        # One-room existence is legal for contributors as for policies: the
        # WGSL copy was an unreferenced documentation mirror and PRUNING_1 P1
        # 5b deleted it, leaving the C++ table — which the DAG assert actually
        # validates — as the single authority. Only a VALUE disagreement
        # between two live copies is an error.

        rows = len(re.findall(r"^    \{ POLICY_", hpp, re.M))
        cnt = re.search(r"POLICY_COUNT\s*=\s*(\d+)", hpp)
        if cnt and rows and int(cnt.group(1)) != rows:
            errs.append("policy mirror: POLICY_COUNT = %s but POLICIES[] has %d "
                        "rows (the id is the row index)" % (cnt.group(1), rows))

    if errs:
        sys.stderr.write("G-LAW 2: RED\n")
        for e in errs[:40]:
            sys.stderr.write("  · %s\n" % e)
        if len(errs) > 40:
            sys.stderr.write("  … and %d more\n" % (len(errs) - 40))
        return 1

    print("G-LAW 2: GREEN — %d fn, %d const, %d struct, %d binding, "
          "%d entry points; %d symbols retired cleanly"
          % (len(cur["fns"]), len(cur["consts"]), len(cur["structs"]),
             len(cur["bindings"]), len(cur["entries"]), len(lost)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
