# DOORS_1 — THE GATE'S VERDICT, AND FOUR FINDINGS FROM THE REVIEW

*Handoff · authored 2026-09-06 · Claude → CC · Jean holds build, visual, merge, deploy.*
*Filed to `docs/HANDOFFS/DOORS_1.md` while open.*

## WHAT HAPPENED, AND WHY THE GATE WAS RIGHT

Jean's deploy chain runs `tools/gates/collection_gate.py` between `about_dist` and `web_dist`.
DOORS_0's recon never saw it (P16 — it is named nowhere in CLAUDE.md's gate table), and the
chain stopped there. Nothing deployed. The gate's second check resolves every `src/href` the
collection page names against `dist/collection/`, exempting only `https:`, `data:`, `#`,
`mailto:` and **`../`** — which is the site's own convention: every page reaches out with
`../fonts/`, `../shared.css`, `../about/`. DOORS_0 rendered the route list with **absolute**
hrefs (`/about/`), which the gate correctly read as five files the collection does not
contain. The fault is the renderer's, not the gate's: the collection speaks in `../`, and the
route list should have been spoken from each page's own address.

**Fix (U1):** `tools/routes.py` gains `rel(href, base)` — an absolute site path spoken from
the page rendering it — and `nav_html(side, base, indent)`. `/about/#text` is `../about/#text`
from `/collection/`, `#text` from `/about/`, `about/#text` from `/`. A route that names the
page it is on is dropped: a menu does not list where you are. The gate is untouched but for
one hint line. Proven here on the tree: the real `collection_dist.py` + `collection_gate.py`
against the fixed renderer → `collection_gate: PASS`, the nav reading
`../about/ · ../about/#text · ../about/#write · ../`.

**The other four (U2–U5)** are the post-landing review's findings that the executor rightly
left for a ruling. Rulings below, each one line.

## AUTHORITY

Base: `master` at HEAD `44dcf6d` (the merged DOORS_0 + VISIT_0 + post-review). Lands on
**master** (P12: three are corrections inside stated contracts; the teardown release is a
missing release the visual gate has not yet seen — Jean's gate follows on the same build).

| file | blob |
| --- | --- |
| `tools/routes.py` | `c5268f24550c6f48a40724e4ea18805480b02a7b` |
| `tools/about_dist.py`, `tools/collection_dist.py`, `tools/web_dist.py` | (report at U0) |
| `tools/gates/collection_gate.py` | `df6ebb2829c71afb6c17dfc2af6678fcb8964cbb` |
| `src/cartridges/the_board/cartridge.hpp` | `7adc56c05a1ff41c0a70dce3a7a74a5770ecf8e9` |
| `src/cartridges/the_board/direction/input.hpp` | `10b1363eb56b1c1497889698daac86345e78a4d1` |
| `src/console/organ_registry.hpp` | `51f82cf0785a2e3376f44573dc5b974057b20ffa` |
| `web/index.html` | `e9be83e978bd9ba947a010dc7a06bf6e51031e10` |
| `docs/OPEN.md` | `cf753ac97e5e380291610d0f1e262a9416ca6004` |

HALT (P17): unreachable file; stale authority (blob differs AND the FIND misses). Otherwise
DEFAULT-AND-FLAG. Counts are 1 unless stated. LF only; `web/index.html` keeps its bytes.

## UNITS

| unit | what | files | commit |
| --- | --- | --- | --- |
| U0 | preflight | — | none |
| U1 | routes spoken from the page | `tools/routes.py`, the three dist scripts, `collection_gate.py` (one hint) | 1 |
| U2 | the pilot's fifth release — the world changed | `cartridge.hpp` | 1 |
| U3 | the stride's dead floor | `input.hpp` | 1 |
| U4 | the roll keeps the hand's place; two truthful sentences | `web/index.html` | 1 |
| U5 | a non-finite slot cannot poison the list | `organ_registry.hpp` | 1 |
| U6 | ledgers; the register | `audit/*`, `docs/OPEN.md` | 1 |

---

## U0 — PREFLIGHT
```
git fetch origin master && git rev-parse HEAD
git ls-tree HEAD tools/routes.py tools/about_dist.py tools/collection_dist.py tools/web_dist.py tools/gates/collection_gate.py src/cartridges/the_board/cartridge.hpp src/cartridges/the_board/direction/input.hpp src/console/organ_registry.hpp web/index.html docs/OPEN.md
python3 -c "from PIL import features; print('avif', features.check('avif'))"   # the collection build's precondition on this machine
```

---

## U1 — ROUTES SPOKEN FROM THE PAGE

### U1.1 — `tools/routes.py`

FIND
```
#   side    "site" | "engine" | absent (both)
```
REPLACE
```
#   side    "site" | "engine" | absent (both)
#   href    an ABSOLUTE site path ("/about/#text"). The renderer speaks it
#           RELATIVE to the page it is rendering (rel, below): the site's
#           own convention (../fonts/, ../shared.css) and the collection
#           gate's jurisdiction line — a "../" reference is the site's, a
#           bare one is the collection's, and a leading slash is neither.
#           A route that names the page it is on is dropped: a menu does
#           not list where you are. (DOORS_1, after the gate refused.)
```

FIND (the whole function, docstring to `return`)
```
def nav_html(side, indent="    "):
    """The <nav> body for one shell. Absolute hrefs, so the same markup is
    right at /, /about/ and /collection/."""
    items = []
    for r in load():
        if r.get("side") not in (None, side):
            continue
        if side == "engine" and r.get("engine") == "pane":
            href = (' data-href="%s"' % esc(r["href"])) if r.get("href") else ""
            items.append('<button type="button" data-route="%s" data-pane="%s"%s>%s</button>'
                         % (esc(r["id"]), esc(r["id"]), href, esc(r["label"])))
            continue
        if not r.get("href"):
            continue
        items.append('<a data-route="%s" href="%s">%s</a>'
                     % (esc(r["id"]), esc(r["href"]), esc(r["label"])))
    return ("\n" + indent).join(items)
```
REPLACE
```
def rel(href, base):
    """An absolute site path, spoken from a page at `base` (a directory,
    "/collection/"). "/about/#text" is "../about/#text" from /collection/
    and "#text" from /about/; a page's own address is "./"."""
    path, frag = (href.split("#", 1) + [""])[:2]
    frag = ("#" + frag) if frag else ""
    b = [seg for seg in base.split("/") if seg]
    t = [seg for seg in path.split("/") if seg]
    i = 0
    while i < len(b) and i < len(t) and b[i] == t[i]:
        i += 1
    parts = [".."] * (len(b) - i) + t[i:]
    if not parts:
        return frag or "./"
    out = "/".join(parts)
    if path.endswith("/"):
        out += "/"
    return out + frag


def nav_html(side, base, indent="    "):
    """The <nav> body for one shell, spoken from the page at `base`."""
    items = []
    for r in load():
        if r.get("side") not in (None, side):
            continue
        href = rel(r["href"], base) if r.get("href") else ""
        if href == "./":
            continue   # the page itself
        if side == "engine" and r.get("engine") == "pane":
            data = (' data-href="%s"' % esc(href)) if href else ""
            items.append('<button type="button" data-route="%s" data-pane="%s"%s>%s</button>'
                         % (esc(r["id"]), esc(r["id"]), data, esc(r["label"])))
            continue
        if not href:
            continue
        items.append('<a data-route="%s" href="%s">%s</a>'
                     % (esc(r["id"]), esc(href), esc(r["label"])))
    return ("\n" + indent).join(items)
```

FIND
```
if __name__ == "__main__":
    print(nav_html("site"))
    print("---")
    print(nav_html("engine"))
```
REPLACE
```
if __name__ == "__main__":
    for base in ("/about/", "/collection/"):
        print("--- site @ %s" % base)
        print(nav_html("site", base))
    print("--- engine @ /")
    print(nav_html("engine", "/"))
```

### U1.2 — the three call sites

`tools/about_dist.py` FIND
```
        "ROUTES": routes.nav_html("site", indent="      "),   # DOORS_0
```
REPLACE
```
        "ROUTES": routes.nav_html("site", "/about/", indent="      "),   # DOORS_0 / DOORS_1: spoken from this page
```

`tools/collection_dist.py` FIND
```
    out = out.replace("<!-- __ROUTES__ -->", routes.nav_html("site", indent="      "))
```
REPLACE
```
    out = out.replace("<!-- __ROUTES__ -->", routes.nav_html("site", "/collection/", indent="      "))
```

`tools/web_dist.py` FIND
```
    shell_out = shell_out.replace("<!-- __ROUTES__ -->", routes.nav_html("engine", indent="      "))
```
REPLACE
```
    shell_out = shell_out.replace("<!-- __ROUTES__ -->", routes.nav_html("engine", "/", indent="      "))
```

### U1.3 — the gate says why (`tools/gates/collection_gate.py`)

FIND
```
            bad.append("index.html names %s — no such file in dist/collection" % ref)
```
REPLACE
```
            hint = ("  (a leading slash names the SITE root; this page speaks in ../ — "
                    "tools/routes.py rel())") if ref.startswith("/") else ""
            bad.append("index.html names %s — no such file in dist/collection%s" % (ref, hint))
```

### U1 witness
```
python3 tools/routes.py
#   site @ /about/      : ../collection/  #text  #write  ../          (no "about" — the page itself)
#   site @ /collection/ : ../about/  ../about/#text  ../about/#write  ../   (no "collection")
#   engine @ /          : buttons for controls/roll/collection(data-href="collection/"), links about/ about/#text about/#write about/
python3 tools/collection_dist.py && python3 tools/gates/collection_gate.py     # PASS  (N local refs, all present; no engine tokens)
python3 tools/about_dist.py --preview /tmp/about.html && grep -o '<a data-route="[a-z]*" href="[^"]*"' /tmp/about.html
python3 - <<'EOF'
import sys; sys.path.insert(0, 'tools'); from routes import rel
for h, b, want in [("/", "/collection/", "../"), ("/about/#text", "/about/", "#text"), ("/about/", "/", "about/"), ("/collection/", "/collection/", "./")]:
    assert rel(h, b) == want, (h, b, rel(h, b))
print("rel ok")
EOF
```
`peek.json` keeps its absolute `/collection/...` paths: it is consumed by the engine at the
root, and the gate reads only `index.html`'s refs. Unchanged, deliberately.

Commit: `DOORS_1 U1 — routes spoken from the page: rel(href, base), self-links dropped; the collection gate's verdict honoured, its refusal now says why`

---

## U2 — THE PILOT'S FIFTH RELEASE (`cartridge.hpp`)

**Ruling: it speaks**, like the other four (P6). The review's one line, plus the line.

FIND
```
                        camera_pose_ = CameraPose{};
```
REPLACE
```
                        camera_pose_ = CameraPose{};
                        // AND THE PILOT'S MARK IS IN THE OLD WORLD (DOORS_1,
                        // from the post-landing review). The point teleports
                        // to Idle::PAWN_POS below without possess(), the clock
                        // is monotonic and the slots are zeroed, so none of
                        // the pilot's four releases could fire and it walked
                        // the new world toward a dead coordinate. The fifth
                        // release — and it speaks, as the other four do.
                        if (pilot_.active)
                            std::cout << "[Visit] released: the world changed (slot " << pilot_.slot << ")\n";
                        pilot_ = PilotState{};
```
Witness: `grep -c "pilot_ = PilotState{};" src/cartridges/the_board/cartridge.hpp` → 1;
`grep -c "\[Visit\] released" src/cartridges/the_board/direction/input.hpp src/cartridges/the_board/cartridge.hpp` → 3 / 1.
`python3 tools/gates/console_gate.py` … `python3 tools/gates/console_gate/run.py` → PASS.

Commit: `DOORS_1 U2 — the pilot releases at world teardown, and says so: the fifth release beside camera_pose_'s reset`

Gate row (Jean): begin a visit, cross an arch mid-walk → `[Visit] released: the world changed`
in the console and the pawn stands still in the new world. BREAK: "the pawn kept walking on
its own after the arch."

---

## U3 — THE STRIDE'S DEAD FLOOR (`input.hpp`)

**Ruling: delete the clamp, not the comment's promise.** A floor the walk cannot reach is a
line the tree re-reads for nothing; the gain at the door is `ARRIVE/SLOW`, and that is what
the comment now says.

FIND
```
    const float gain = std::min(1.0f, std::max(0.25f, d / PILOT_SLOW_WU));
```
REPLACE
```
    const float gain = std::min(1.0f, d / PILOT_SLOW_WU);   // linear ease; at the door it is ARRIVE/SLOW of a stride
```

FIND
```
inline constexpr float  PILOT_SLOW_WU         = 8.0f;   // inside this the stride eases to a quarter
```
REPLACE
```
inline constexpr float  PILOT_SLOW_WU         = 8.0f;   // inside this the stride eases linearly; at the door it is ARRIVE/SLOW (~0.31) of a stride
```
Witness: `grep -c "std::max(0.25f" src/cartridges/the_board/direction/input.hpp` → 0.

Commit: `DOORS_1 U3 — the pilot's ease is linear: the unreachable quarter floor deleted, the comment tells the truth`

---

## U4 — THE ROLL KEEPS THE HAND'S PLACE (`web/index.html`)

Two things: the rebuild remembers which caption held focus and puts it back (or the first,
if that picture came down); and the two states the guard conflated get their own sentence —
`!c` is "the program is still compiling", `!c.roll` is "not in this build". Words are
placeholders; the split is the structure.

FIND (the whole function)
```
      function refreshRoll() {
        if (!rollEl || !rollState) return;
        var c = abi();
        if (!c || !c.roll) { rollEl.innerHTML = ''; rollState.hidden = false; rollState.textContent = 'Not in this build.'; return; }
        var list;
        try { list = JSON.parse(c.roll()); } catch (err) { note('[shell] roll: ' + err); return; }
        rollEl.innerHTML = '';
        if (!list.length) { rollState.hidden = false; rollState.textContent = 'Nothing hung yet — walk a while.'; return; }
        list.sort(function (a, b) { return a.d - b.d; });
        list.forEach(function (w) {
          var li = document.createElement('li');
          var b = document.createElement('button');
          b.type = 'button'; b.dataset.visit = w.s; b.textContent = caption(w);
          li.appendChild(b); rollEl.appendChild(li);
        });
        var v = c.visiting ? c.visiting() : -1;
        rollState.hidden = v < 0;
        if (v >= 0) rollState.textContent = 'Walking to one — touch anything to stop.';
      }
```
REPLACE
```
      function refreshRoll() {
        if (!rollEl || !rollState) return;
        var c = abi();
        // TWO STATES, TWO SENTENCES (DOORS_1). No Module yet is the common
        // case — the menu is static HTML a visitor can open before the
        // wasm resolves — and it is not "not in this build". Placeholder words.
        if (!c)      { rollEl.innerHTML = ''; rollState.hidden = false; rollState.textContent = 'Still waking — open this again in a moment.'; return; }
        if (!c.roll) { rollEl.innerHTML = ''; rollState.hidden = false; rollState.textContent = 'Not in this build.'; return; }
        var list;
        try { list = JSON.parse(c.roll()); } catch (err) { note('[shell] roll: ' + err); return; }
        // REBUILD, KEEPING THE HAND'S PLACE (DOORS_1, from the review): the
        // visitor may be arrowing down the list when the world rehangs.
        // Remember the focused caption's slot, rebuild, put focus back on
        // that slot — or on the first caption if that picture came down.
        var had = (document.activeElement && rollEl.contains(document.activeElement))
                  ? document.activeElement.dataset.visit : null;
        rollEl.innerHTML = '';
        if (!list.length) { rollState.hidden = false; rollState.textContent = 'Nothing hung yet — walk a while.'; return; }
        list.sort(function (a, b) { return a.d - b.d; });
        list.forEach(function (w) {
          var li = document.createElement('li');
          var b = document.createElement('button');
          b.type = 'button'; b.dataset.visit = w.s; b.textContent = caption(w);
          li.appendChild(b); rollEl.appendChild(li);
        });
        if (had !== null && had !== undefined) {
          var again = rollEl.querySelector('button[data-visit="' + had + '"]') || rollEl.querySelector('button');
          if (again) again.focus();
        }
        var v = c.visiting ? c.visiting() : -1;
        rollState.hidden = v < 0;
        if (v >= 0) rollState.textContent = 'Walking to one — touch anything to stop.';
      }
```
Witness:
```
grep -c "Still waking" web/index.html                   # 1
grep -c "__BUILD_ID__" web/index.html                   # 1
node -e "const t=require('fs').readFileSync('web/index.html','utf8');for(const s of t.match(/<script>([\s\S]*?)<\/script>/g)||[]){new Function(s.replace(/^<script>|<\/script>$/g,''))}console.log('parses')"
```
If headless Chromium is at hand (the review used it): focus a caption, wait one refresh,
`document.activeElement` is still a caption button.

Commit: `DOORS_1 U4 — the roll keeps keyboard focus across its refresh; "still waking" and "not in this build" are two sentences`

---

## U5 — A NON-FINITE SLOT CANNOT POISON THE LIST (`organ_registry.hpp`)

FIND
```
            const float bearing = std::atan2(dx, dz) * 57.2957795f;   // degrees; 0 = +Z, clockwise — the WORLD's frame
```
REPLACE
```
            const float bearing = std::atan2(dx, dz) * 57.2957795f;   // degrees; 0 = +Z, clockwise — the WORLD's frame
            // A non-finite field would print as nan/inf and break the WHOLE
            // list's JSON (the review's fragility). Nothing in the tree is
            // known to hang such a slot; if one appears its row is absent,
            // and the absence is the artifact — no per-frame line (P6).
            if (!std::isfinite(d) || !std::isfinite(bearing)
                || !std::isfinite(s.scale_x) || !std::isfinite(s.scale_y)) continue;
```
`<cmath>` is already included (VISIT_0 U2.1). Witness: `python3 tools/gates/console_gate/run.py` → PASS;
`python3 tools/gates/shell_gate/run.py` → GREEN.

Commit: `DOORS_1 U5 — gallery_roll skips a non-finite slot rather than emitting invalid JSON`

---

## U6 — LEDGERS AND THE REGISTER

1. The cascade (D9): `binding_ledger.py` → `command_census.py` → `mirror_census.py`, then the
   three `--check`s, `organ_ledger.py --check`, `gates/score/run.py`, `gates/shell_gate/run.py`.
   Expected changes: the `cartridge.hpp` pin (U2) in `COMMAND_LEDGER` / `BINDING_LEDGER` /
   `MIRROR_LEDGER`. `input.hpp` and `organ_registry.hpp` are not pinned; if a ledger names
   them, regenerate and flag.
2. `docs/OPEN.md` (read whole): strike as fixed — the teardown release (U2), the stride floor
   (U3), the focus loss and the wording (U4), the nan/inf fragility (U5); keep the cwrap
   fast-path entry open (unverifiable here); add **DOORS_1** as closed with one line on the
   gate's verdict and the relative-route rule; add to the residuals: *`tools/gates/collection_gate.py`
   is a deploy-chain gate absent from CLAUDE.md's gate table — add the row* (Jean's file; CC
   proposes the row text in the report, does not edit CLAUDE.md).

Commit: `DOORS_1 U6 — ledgers regenerated (cartridge.hpp pin); OPEN.md: four review findings struck as fixed, DOORS_1 closed, the collection gate's missing table row registered`

---

## JEAN'S CHAIN, AFTER THIS LANDS

The same command line. The expected new lines: `collection_gate: PASS (… local refs, all
present; no engine tokens)`, then `web_dist.py`'s ordinary output, then the deploy. The first
thing to look at in the browser is the veil — two buttons, the sentence above them — and the
first thing to type after entry is W: DOORS_0's own gate rows and the post-review's key-guard
row are still unwitnessed by anyone but headless Chromium.

## WHAT THIS DOCUMENT CANNOT SEE (P16)

It ran the real `collection_dist.py` + `collection_gate.py` here against an EMPTY asset set
(the paintings are not in git), so it proved the nav refs pass and did not re-prove the
srcset refs — those were passing before DOORS_0 and are untouched. It could not build
`about/` (`assets/about` is Jean's), so `/about/`'s rendered nav is proven by
`python3 tools/routes.py` only. It did not see the wasm, so the engine nav is proven the same way.
