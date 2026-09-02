#!/usr/bin/env python3
# ═══════════════════════════════════════════════════════════════════════
# THE READER PROOF — organ_readers.py
#
# AN ENROLLMENT STATES A BELIEF; ONLY THE READER PROVES IT. This asks
# every enrolled row one question — DOES YOUR READER NAME YOU? — by
# mapping each row to the FUNCTION its family is consumed by,
# brace-parsing that function out of the tree, and reporting every row
# whose field name is absent from all of its readers.
#
# EXIT 0 ALWAYS. organ_gap measures the gap between the HOMES and the
# panel; this is the inward question. A suspect here is a QUESTION, not a
# verdict: the reason a row survives one is docs/ORGAN.md's judgement.
#
# USAGE   organ_readers.py · --brief for counts and suspects only
# ═══════════════════════════════════════════════════════════════════════

# ── THE BLIND SPOTS, SAID PLAINLY RATHER THAN DISCOVERED LATER ───────
#   1. HELPER INDIRECTION PRODUCES FALSE POSITIVES. A reader that copies a
#      whole struct and hands the copy on names no field at all. Every
#      function in a family's list is searched, so the cure is to LIST the
#      helper — and the table below is printed on every run.

#   2. MACRO PACKING AND WHOLE-STRUCT COPIES ARE INVISIBLE. `writeStruct`,
#      `memcpy` and an aggregate assignment consume every field and name
#      none. The three GPU rooms are out of scope for that reason, and say
#      so below rather than reporting 130 false suspects.

#   3. GPU-SIDE CONSUMPTION IS OUT OF SCOPE. A field uploaded into a
#      uniform is read in world.wgsl, and proving THAT is the kernel's
#      ledger. Rows whose only reader is the GPU are named as out of
#      scope, never as suspects.

#   4. A LEAF TOKEN CAN COLLIDE. The match is on the field's last
#      identifier, so a leaf like `direction` or `count` can be satisfied
#      by an unrelated mention in the same body. That produces false
#      NEGATIVES — a dead row reading green — which is the worse
#      direction, so a successor should not trust it silently.

#   5. A WITNESS'S QUESTION IS INVERTED. An `_RO` row is a meter and
#      nobody is supposed to read the panel's word for it, because the
#      panel never writes one. What matters for a witness is who AUTHORS
#      it, which is another instrument's question.

# THE MATCH IS HANDLE-QUALIFIED, and that is the whole tool. A bare token
# match would pass a dead row: `configure_orbs` contains the token
# `motion_rule` in a line that reads the PLAYER's rule and never the
# mood's. So the leaf must be reached THROUGH a name that IS the bank.

# THE SECOND PASS makes a suspect actionable: for every row the declared
# readers do not name, the tool searches src/ for the token and prints
# where it IS mentioned.
#   MECHANICAL — named somewhere the table does not list: a stale table.
#   SEMANTIC   — named nowhere but its declaration and its enrollment.
#                The dial writes a fact nothing reads.

import os
import re
import sys

# tools/ is sys.path[0] when a tool runs as a script; the insert is for the
# ledger's subprocess runs and for any caller importing a tool from elsewhere.
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from organ_parse import MACRO, split_args

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INC = os.path.join(ROOT, "src", "console", "organ_params.inc")
SRC = os.path.join(ROOT, "src")
SRC_EXT = (".hpp", ".cpp", ".inc", ".h")

# ─── THE READER TABLE (blind spot 1 — keep it current, it is printed) ──
# family -> (the LIVE bank this family addresses, [(file, function), …]).
# A family is a BLOCK for a plain / GEN row and a DEFINITION KIND for a
# DEF / DEFONLY row: a definition's reader is its applier, not its home's.

# The functions are censused, not guessed — every function body in src/
# that names the family's LIVE symbol. The tool re-derives that census on
# every run and prints any function this table omits, so it cannot go
# stale in silence.
READERS = {
    "DRIVERS": ("DRIVER_LIVE", "DriverSurface", [
        ("src/cartridges/the_board/cartridge.hpp", "phase_motion_drivers"),
        ("src/cartridges/the_board/bodies/pawn.hpp", "tick_pawn_couplings"),
        ("src/cartridges/the_board/bodies/pawn.hpp", "toggle_aura"),
        ("src/cartridges/the_board/bodies/pawn.hpp", "apply_aura_mood_policy"),
        ("src/cartridges/the_board/bodies/ribbon.hpp", "ribbon_frame_tick"),
    ]),
    "PAWN": ("PAWN_AURA_LIVE", "PawnAuraProfile", [
        ("src/cartridges/the_board/bodies/pawn.hpp", "tick_pawn_couplings"),
        ("src/cartridges/the_board/bodies/pawn.hpp", "dispatch_pawn_aura"),
    ]),
    "ORBS": ("ORB_CONSOLE_LIVE", "OrbConsole", [
        ("src/cartridges/the_board/bodies/orbs.hpp", "configure_orbs"),
        ("src/cartridges/the_board/bodies/orbs.hpp", "log_configure_"),
        # the console mask's boundary block: dome, noise and speed reach
        # the GPU through targeted partials from here rather than through
        # configure_orbs.
        ("src/cartridges/the_board/organ_boundary.inc", "organ_flush"),
    ]),
    "PANEL": ("PANEL_LIVE", "PanelSurface", [
        ("src/cartridges/the_board/cartridge.hpp", "phase_motion_drivers"),
        ("src/cartridges/the_board/direction/input.hpp", "nudge_look_sensitivity"),
        ("src/cartridges/the_board/direction/input.hpp", "on_scroll"),
        ("src/cartridges/the_board/bodies/agents.hpp", "try_possess_nearest"),
    ]),
    # RIBBON_1: the head mover left for the GPU, so the bank's reader for the
    # flight dials is the BOOT PIN that carries them into config.ribbon_* —
    # blind spot 3, said at the table instead of reported as eleven suspects.
    "RIBBON": ("RIBBON_LIVE", "RibbonSurface", [
        ("src/cartridges/the_board/realization/state.hpp", "initializeState"),
        ("src/cartridges/the_board/cartridge.hpp", "phase_fill_signal"),
        ("src/cartridges/the_board/bodies/ribbon.hpp", "ribbon_frame_tick"),
    ]),
    "INDOOR": ("INDOOR_LIVE", "IndoorSurface", [
        ("src/cartridges/the_board/direction/mood.hpp", "apply_mood_lighting"),
        ("src/cartridges/the_board/bodies/ribbon.hpp", "select_ribbon_for_patch"),
        ("src/cartridges/the_board/bodies/cube_behaviors.hpp", "cube_apply_indoor_rescale"),
        ("src/cartridges/the_board/bodies/grounded.hpp", "blade_apply_indoor_rescale"),
        ("src/cartridges/the_board/bodies/grounded.hpp", "cactus_apply_indoor_rescale"),
        ("src/cartridges/the_board/bodies/grounded.hpp", "palm_apply_indoor_rescale"),
        ("src/cartridges/the_board/bodies/spheres.hpp", "sphere_apply_indoor_rescale"),
        ("src/cartridges/the_board/machine/entity_pipeline.hpp", "antenna_apply_indoor_rescale"),
        ("src/cartridges/the_board/machine/entity_pipeline.hpp", "arch_apply_indoor_rescale"),
        ("src/cartridges/the_board/machine/entity_pipeline.hpp", "pyramid_apply_indoor_rescale"),
    ]),
    "CANVAS": ("CANVAS_LIVE", "CanvasSurface", [
        ("src/coupling/visual_canvas.hpp", "tick"),
    ]),
    "WORLD": ("WORLD_DRAW_LIVE", "WorldDrawSurface", [
        ("src/cartridges/the_board/contracts/mood_constants.hpp", "portal_color_for"),
        ("src/cartridges/the_board/direction/mood.hpp", "derive_indoor_lights"),
        # pick_portal_mood and pick_open_mood are one-line faces on the
        # weighted walk; mood_weights is named in the walk's body.
        ("src/cartridges/the_board/direction/mood.hpp", "pick_mood_weighted_"),
        ("src/cartridges/the_board/direction/mood.hpp", "pick_portal_mood"),
        ("src/cartridges/the_board/machine/entity_pipeline.hpp", "arch_write_active"),
        ("src/cartridges/the_board/bodies/grounded.hpp", "force_spawn_portal_arch"),
    ]),
    "RIBBON_SPAWN": ("RIBBON_SPAWN_LIVE", "RibbonSpawnSurface", [
        ("src/cartridges/the_board/bodies/ribbon.hpp", "select_ribbon_for_patch"),
        ("src/cartridges/the_board/bodies/ribbon.hpp", "place_ribbon_from_selection"),
        ("src/cartridges/the_board/bodies/ribbon.hpp", "fill_ribbon_selection_geometry"),
        ("src/cartridges/the_board/bodies/ribbon.hpp", "commit_ribbon"),
        # RIBBON_2: the cruise is redrawn when a rider steps off, so the
        # dismount is a reader of the draw's four dials in its own right.
        ("src/cartridges/the_board/bodies/ribbon.hpp", "ribbon_on_dismount"),
    ]),
    # ── the definition kinds: a definition's reader is its APPLIER ────
    # Two handles: the regime law is MoodProfile's own field (read by
    # apply_mood_regime through `m`), the rest is atmos.* (read by
    # draw_atmosphere through `a`).
    "MOOD": ("MOOD_LIVE", ("MoodProfile", "Atmosphere"), [
        ("src/cartridges/the_board/direction/mood.hpp", "apply_mood_regime"),
        ("src/cartridges/the_board/direction/mood.hpp", "draw_atmosphere"),
        ("src/cartridges/the_board/direction/mood.hpp", "apply_mood_lighting"),
    ]),
    "TIER": ("TIER_LIVE", "AgentTierBank", [
        ("src/cartridges/the_board/bodies/agents.hpp", "upload_agent_registries_to_gpu"),
    ]),
    "BEHAVIOR": ("BEHAVIOR_LIVE", "AgentBehaviorBank", [
        ("src/cartridges/the_board/bodies/agents.hpp", "upload_agent_registries_to_gpu"),
    ]),
    "ORB_MOOD": ("ORB_MOOD_LIVE", "OrbMoodConfig", [
        ("src/cartridges/the_board/bodies/orbs.hpp", "configure_orbs"),
        ("src/cartridges/the_board/bodies/orbs.hpp", "apply_mood_first_run_defaults_"),
        ("src/cartridges/the_board/bodies/orbs.hpp", "pack_palette_"),
        ("src/cartridges/the_board/bodies/orbs.hpp", "pack_tiers_"),
        ("src/cartridges/the_board/bodies/orbs.hpp", "pack_flocking_"),
        ("src/cartridges/the_board/bodies/orbs.hpp", "log_configure_"),
    ]),
}

# ─── OUT OF SCOPE, WITH THE REASON (blind spots 2 and 3) ──────────────
# A family here is never reported as a suspect, because this tool cannot
# see its consumption at all — saying nothing is honest, saying "absent"
# would be a lie in the shape of a finding.
OUT_OF_SCOPE = {
    "CONFIG":     "GPU-side: config_ is uploaded whole and read in world.wgsl "
                  "(the kernel's ledger, a future instrument)",
    "LIGHTING":   "GPU-side: lightingStage_ ships through upload_lighting as a "
                  "whole struct and is read in world.wgsl",
    "AGENT_ROOM": "GPU-side: tier_gains and behaviors ship as whole arrays "
                  "through two WriteBuffers and are read in world.wgsl",
}

# ─── THE ENROLLMENT LIST (organ_gap's parser, one column wider) ───────
# Same five macro forms and the same _NS shift. What differs is what this
# tool takes from each line: organ_gap wants (struct, member); this wants
# (family, field), and for a DEF row the family is the KIND and the field
# is the DEFFIELD, because a definition's reader is its applier.


def leaf_of(field):
    """The identifier a reader would actually type.

    `fog.rest_density` -> rest_density · `t[0].color_r` -> color_r ·
    `smooth_palette[0][0]` -> smooth_palette · `sun_ambient` -> sun_ambient.
    """
    tail = field.strip().split(".")[-1]
    return re.split(r"\[", tail, 1)[0].strip()


def rows():
    """Every enrolled row as a dict — the tool's whole input."""
    out = []
    with open(INC, encoding="utf-8") as f:
        for n, raw in enumerate(f, 1):
            m = MACRO.match(raw.strip())
            if not m:
                continue
            form, ns, args = m.group(1), m.group(2), split_args(m.group(3))
            k = 1 if ns else 0
            r = {"line": n, "form": form, "group": None,
                 "family": None, "field": None, "ro": form == "ORGAN_PARAM_RO"}
            if form in ("ORGAN_PARAM", "ORGAN_PARAM_GEN") and len(args) >= 9 + k:
                r["family"] = args[0 + k]
                r["field"] = args[2 + k]
                r["group"] = args[7 + k].strip('"')
            elif form == "ORGAN_PARAM_RO" and len(args) >= 6 + k:
                r["family"] = args[0 + k]
                r["field"] = args[2 + k]
                r["group"] = args[4 + k].strip('"')
            elif form == "ORGAN_PARAM_DEF" and len(args) >= 12 + k:
                r["family"] = args[9 + k]          # DEFKIND
                r["field"] = args[11 + k]          # DEFFIELD
                r["group"] = args[7 + k].strip('"')
            elif form == "ORGAN_PARAM_DEFONLY" and len(args) >= 9 + k:
                r["family"] = args[6 + k]          # DEFKIND
                r["field"] = args[8 + k]           # DEFFIELD
                r["group"] = args[4 + k].strip('"')
            else:
                continue
            r["leaf"] = leaf_of(r["field"])
            out.append(r)
    return out


# ─── THE BODIES ───────────────────────────────────────────────────────
FN_OPEN = re.compile(
    r"^[ \t]*(?:inline\s+|static\s+|constexpr\s+|virtual\s+|template\s*<[^>]*>\s*)*"
    r"(?:const\s+)?[A-Za-z_][\w:<>,\*& ]*?\b(\w+)\s*\([^;{]*\)\s*"
    r"(?:const\s*)?(?:noexcept\s*)?(?:override\s*)?\{", re.M)


def read(rel):
    try:
        with open(os.path.join(ROOT, rel), encoding="utf-8", errors="replace") as f:
            return f.read()
    except OSError:
        return None


def function_spans(text):
    """[(name, signature_start, body_start, body_end)] — brace-matched.

    The signature start rides along because the parameter that IS the bank
    is named there and nowhere else, and it is the handle most readers use
    (`cfg`, `m`, `c`).
    """
    out = []
    for m in FN_OPEN.finditer(text):
        i = text.index("{", m.start())
        depth = 0
        for j in range(i, len(text)):
            if text[j] == "{":
                depth += 1
            elif text[j] == "}":
                depth -= 1
                if depth == 0:
                    out.append((m.group(1), m.start(), i, j))
                    break
    return out


def body_of(rel, fn, cache):
    """The brace-matched body of `fn` in `rel`, or None if it is not there.

    The SIGNATURE rides with it: the parameter that IS the bank is named
    there and nowhere else, and it is the handle most readers use.
    """
    if rel not in cache:
        text = read(rel)
        cache[rel] = (text, function_spans(text) if text else [])
    text, spans = cache[rel]
    if text is None:
        return None
    pieces = [text[sig:b + 1] for name, sig, _a, b in spans if name == fn]
    return "\n".join(pieces) if pieces else None


def owners_of(symbol, cache):
    """{(rel, fn)} — every function body in src/ that names `symbol`.

    The table above is hand-authored; this is the same census re-run, so a
    reader the table forgot is PRINTED rather than silently unsearched.
    """
    found = set()
    for base, _dirs, files in os.walk(SRC):
        for f in sorted(files):
            if not f.endswith((".hpp", ".cpp", ".inc")):
                continue
            rel = os.path.relpath(os.path.join(base, f), ROOT).replace(os.sep, "/")
            if rel.endswith("organ_registry.hpp") or rel.endswith("organ_params.inc"):
                continue
            text = read(rel)
            if not text or symbol not in text:
                continue
            if rel not in cache:
                cache[rel] = (text, function_spans(text))
            _t, spans = cache[rel]
            for m in re.finditer(r"\b" + re.escape(symbol) + r"\b", text):
                for name, _sig, a, b in spans:
                    if a <= m.start() <= b:
                        found.add((rel, name))
                        break
    return found


# ─── THE MATCH IS HANDLE-QUALIFIED, AND THAT IS THE WHOLE TOOL ────────
# A bare token match would pass a dead row: `configure_orbs` contains the
# token `motion_rule` in a line that reads the PLAYER's rule and never the
# mood's. So the match is `<handle>.<…>.<leaf>` — the leaf must be reached
# THROUGH a name that IS the bank.

# A handle is the LIVE symbol itself (WORLD_DRAW_LIVE.portal_density), a
# parameter whose type is the bank's struct (cfg, in configure_orbs), or a
# reference alias bound to either (const auto& S = RIBBON_SPAWN_LIVE),
# resolved to a fixed point because aliases chain.
ALIAS = re.compile(r"(?:const\s+)?auto\s*&\s*(\w+)\s*=\s*([A-Za-z_]\w*)")
PARAM = re.compile(r"[(,]\s*(?:const\s+)?(\w+)\s*[&*]?\s*&?\s*(\w+)\s*(?=[,)])")


def handles_in(sig_and_body, live, struct):
    """Every identifier inside this body that IS the bank.

    `struct` is a name or a tuple of names: a family whose
    leaves live at two depths — MoodProfile.regime_weight beside
    MoodProfile.atmos.* — is held by two handle types.
    """
    structs = struct if isinstance(struct, tuple) else (struct,)
    hs = {live}
    for m in PARAM.finditer(sig_and_body):
        if m.group(1) in structs:
            hs.add(m.group(2))
    for _ in range(4):                      # aliases chain; settle them
        before = len(hs)
        for m in ALIAS.finditer(sig_and_body):
            if m.group(2) in hs:
                hs.add(m.group(1))
        if len(hs) == before:
            break
    return hs


def mentions(text, leaf, handles):
    """`<handle>[…].<…>.<leaf>` — the handoff's `cfg.<field>`, made strict.

    The gap between handle and leaf allows member and index characters
    only, so it cannot cross a comma, a paren or a semicolon: one
    expression, not one line.
    """
    for h in handles:
        pat = (r"(?<![A-Za-z0-9_])" + re.escape(h)
               + r"[ \t]*(?:\[[^\]\n]*\]|\.[ \t]*[A-Za-z_]\w*)*"
                 r"[ \t]*\.[ \t]*" + re.escape(leaf) + r"\b")
        if re.search(pat, text):
            return True
    return False


def tree_mentions(leaf, skip):
    """[(rel, line, text)] — where the token appears anywhere in src/.

    The second pass. `skip` holds the files whose mention is the field's
    own declaration or its enrollment, so what is left is a real reader
    the table did not list.
    """
    hits = []
    for base, _dirs, files in os.walk(SRC):
        for f in sorted(files):
            if not f.endswith(SRC_EXT):
                continue
            rel = os.path.relpath(os.path.join(base, f), ROOT).replace(os.sep, "/")
            if rel in skip:
                continue
            text = read(rel)
            if not text:
                continue
            for m in re.finditer(r"(?<![A-Za-z0-9_])" + re.escape(leaf) + r"\b", text):
                ln = text.count("\n", 0, m.start()) + 1
                hits.append((rel, ln, text.split("\n")[ln - 1].strip()[:78]))
    return hits


def main():
    brief = "--brief" in sys.argv
    cache = {}
    all_rows = rows()

    print("ORGAN READERS — does your reader name you?")
    print("=" * 72)
    print("An enrollment states a belief; only the reader proves it. A map,")
    print("not a gate: exit 0 always. Reasons live in docs/ORGAN.md.")
    print()
    print("BLIND SPOTS this run carries: helper indirection and whole-struct")
    print("copies produce false positives; a colliding leaf token produces a")
    print("false NEGATIVE, which is worse; GPU-side consumption is out of")
    print("scope (that is the kernel's ledger, a future instrument).")
    print()

    print("THE READER TABLE this run trusted (blind spot 1 — printed, so a")
    print("stale row is visible rather than an invisible bug):")
    for fam in sorted(READERS):
        live, struct, fns = READERS[fam]
        print("  %-14s %-20s %s" % (fam, live, struct))
        listed = set()
        for rel, fn in fns:
            ok = body_of(rel, fn, cache) is not None
            print("      %s %-34s %s" % ("·" if ok else "!", fn, rel))
            listed.add((rel, fn))
        extra = sorted(owners_of(live, cache) - listed)
        for rel, fn in extra:
            # Named the bank but is not in the table: usually an accessor
            # or a call site that hands the whole row on (mood_def,
            # organ_flush, apply_mood). Printed anyway — a reader the
            # table forgot looks exactly like this until someone reads it.
            print("      ? census also names the bank here: %-22s %s" % (fn, rel))
    print()
    print("  OUT OF SCOPE, with the reason:")
    for fam in sorted(OUT_OF_SCOPE):
        print("      %-12s %s" % (fam, OUT_OF_SCOPE[fam]))
    print()

    # ── the question, row by row ──────────────────────────────────────
    counted = {"proved": 0, "suspect": 0, "witness": 0, "out": 0, "unmapped": 0}
    suspects = []
    for r in all_rows:
        fam = r["family"]
        if fam in OUT_OF_SCOPE and r["form"] != "ORGAN_PARAM_RO":
            counted["out"] += 1
            continue
        if r["ro"]:
            counted["witness"] += 1
            continue
        if fam not in READERS:
            counted["unmapped"] += 1
            print("  UNMAPPED FAMILY %-14s %s:%d  %s"
                  % (fam, "organ_params.inc", r["line"], r["field"]))
            continue
        live, struct, fns = READERS[fam]
        named = False
        for rel, fn in fns:
            body = body_of(rel, fn, cache)
            if not body:
                continue
            if mentions(body, r["leaf"], handles_in(body, live, struct)):
                named = True
                break
        if named:
            counted["proved"] += 1
        else:
            counted["suspect"] += 1
            suspects.append(r)

    print("THE ANSWER, ROW BY ROW")
    print("-" * 72)
    print("  proved   %4d   a declared reader names the field" % counted["proved"])
    print("  SUSPECT  %4d   no declared reader names it" % counted["suspect"])
    print("  witness  %4d   an _RO meter: the question is inverted (blind spot 5)"
          % counted["witness"])
    print("  scope    %4d   GPU-side or whole-struct (blind spots 2, 3)"
          % counted["out"])
    if counted["unmapped"]:
        print("  UNMAPPED %4d   a family with no row in the reader table"
              % counted["unmapped"])
    print()

    # PLUMB_0 A4 — THE VERDICT IS AN EXIT CODE NOW.
    #
    # Every path here returned 0, including the one that had just printed a
    # list of suspects. CLAUDE.md's gate table calls this tool an assertion —
    # "every enrolled dial's field is named by a declared reader" — so a
    # dial whose reader could not be proved passed the gate that exists to
    # ask exactly that. An UNMAPPED family is red for the same reason and is
    # worse: it means the reader table does not cover the tree it audits.
    #
    # THE PROSE BELOW STANDS UNCHANGED AND IS NOT SOFTENED. "A suspect is a
    # question, not a verdict" is advice about how to FIX one — read the body
    # before deleting a dial — not a claim that it should pass. A question
    # the tree cannot answer is exactly what a gate is for.
    if not suspects and not counted["unmapped"]:
        print("NO SUSPECTS. Every enrolled dial's field is named in the body of")
        print("a function this tool can read.")
        return 0

    if counted["unmapped"] and not suspects:
        print("UNMAPPED FAMILIES: %d. The reader table has no row for a family"
              % counted["unmapped"])
        print("this tool was asked to prove, so the proof is incomplete rather")
        print("than passing. Add the row, or say in the table why it cannot be")
        print("read.")
        return 1

    print("THE SUSPECTS, each with its second pass (mechanical -> fix now,")
    print("semantic -> ledger the anatomy)")
    print("-" * 72)
    for r in suspects:
        live, struct, fns = READERS[r["family"]]
        print("  %-14s %-28s  %s" % (r["family"], r["field"], r["group"]))
        if brief:
            continue
        skip = {"src/console/organ_params.inc"}
        hits = [h for h in tree_mentions(r["leaf"], skip)
                if not (h[0].endswith("organ_readers.py"))]
        # the field's OWN declaration is not a reader — drop it, so what
        # is left is the evidence the mechanical/semantic split turns on.
        declre = re.compile(r"^\s*(?:inline\s+)?(?:static\s+)?(?:constexpr\s+)?"
                            r"[A-Za-z_][\w:]*\s+" + re.escape(r["leaf"]) + r"\b")
        other = [h for h in hits if not declre.search(h[2])]
        if not other:
            print("        SEMANTIC — the token appears nowhere but its own")
            print("        declaration. The dial writes a fact nothing reads.")
        else:
            print("        MECHANICAL? — named outside the declared readers at:")
            for rel, ln, line in other[:6]:
                print("          %s:%d  %s" % (rel, ln, line))
            if len(other) > 6:
                print("          … and %d more" % (len(other) - 6))
    print()
    print("A SUSPECT IS A QUESTION, NOT A VERDICT. Read the body before")
    print("acting: a helper that takes the value by parameter names it at the")
    print("call site, and a reader this table forgot is a stale table rather")
    print("than a dead dial.")
    print()
    print("STOP — %d suspect(s)%s. The gate is red until each is either given"
          % (len(suspects),
             (" and %d unmapped family/ies" % counted["unmapped"])
             if counted["unmapped"] else ""))
    print("its reader, given a READERS row that names one, or retired.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
