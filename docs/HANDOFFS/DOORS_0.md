# DOORS_0 — THE FRONT DOOR, THE SANDWICH, THE PEEK, THE IDLE

*Handoff · authored 2026-09-06 · Claude → CC · Jean holds build, visual, merge, deploy.*
*Filed to `docs/HANDOFFS/DOORS_0.md` while open; the directory dies with the campaign.*

## AUTHORITY

Base: `master` at HEAD `dcbd4a900464add6d297b9351cbe31da2b40af67`.
Lands on **master** (P12: the shell ships only through Jean's deploy gate — `dist/` is the
deploy target and only Jean writes it — so master is already the held state for `web/`;
the one `src/` touch, U5, is an export nothing calls until the shell does).

Gate on **blob SHA**, never commit SHA (the anchor law). Every FIND below is verbatim
against these blobs; expected match count is **1** unless stated.

| file | blob |
| --- | --- |
| `web/index.html` | `e25c99f9aa45b7bcbc6cab7e73cf4ecab4fd91e3` |
| `web/about/index.html` | `5c2b7009eaaa5d667fd06b66d3008436864a600e` |
| `web/collection/index.html` | `49db685ac0eefe2f5137b954df537ca35f4aaceb` |
| `web/shared.css` | `59b55735363b73b6317fcfeb039da6daee8bb1ce` |
| `tools/web_dist.py` | `f636a9c70917a8b116f446c13ff82c51dd6b5674` |
| `tools/about_dist.py` | `60f1ad11010b2649ca34824ae4af15a7d7d5b437` |
| `tools/collection_dist.py` | `4389b0729ca984bb94c1b63fc625e6a85052d4ef` |
| `src/the_board.cpp` | `d81ff01d340067ef490d5df57e289bc9f06649a5` |
| `docs/OPEN.md` | `86a95324e0dd359e72c2dff58639bcd12291c8bb` |

**HALT classes (P17), scoped to the unit (P15):** (a) a named file unreachable; (b) the
file's blob differs from the table AND the unit's FIND does not match — stale authority.
Everything else is DEFAULT-AND-FLAG: take the default written beside the clause, log it in
the report, finish the round. Line numbers in this document are hints; boundaries are
**symbols** (P2), and the executor recomputes them against the tree in front of it.

Encoding law: LF only, no BOM, whole tree. `web/index.html` is read by `web_dist.py` with
`newline=""`; do not let an editor translate its line endings.

## THE RULINGS THIS CAMPAIGN LANDS

1. **The engine stays at `everexpandingboard.com`. The veil is the front door.** Two
   buttons on the veil: `#enter` (the engine) and a static anchor to `/about/` (the
   website). The veil is no longer a tap target. `onEntryGesture` and `carriesActivation`
   are **byte-identical** — the activation-critical path does not move; only its target does.
2. **Eager tap is banked.** A visitor who taps `#enter` at 40 % loaded spends the grants
   now (as today) and the world reveals itself the instant it is ready, through the same
   handler. No second tap.
3. **Fallback: a link, not a redirect.** The card's log pane is the diagnostic surface a
   redirect would steal. `#reload` stays gated by `offerReload` exactly as today.
4. **One route list, three shells.** `web/routes.json` is the home; `tools/routes.py` the
   one renderer; every dist script injects. The site pages' hand-written mastheads die.
5. **The sandwich is a native `<details>`.** It opens with no script, so the site pages keep
   their JavaScript-off law; the engine shell hooks its `toggle` event for idle.
6. **Idle, not freeze.** Menu open → rAF every 4th vblank + a blur on the frame; music is an
   HTML element and never rides rAF. Menu closed → the boot pace. This is the second
   arm site WRAP_0 U2's banner said did not exist; the banner is revised to say so.
7. **The peek** is `dist/collection/peek.json`, written by `collection_dist.py` from the
   same records that write the page; fetched by the shell at the menu's first opening — a
   gesture, never boot (web_dist's boot-set law, unchanged and re-witnessed).

Words are placeholders throughout (the sentence, both labels, the controls table). They
are marked `PLACEHOLDER COPY` in the tree, one home each, and are Jean's.

## UNITS, IN ORDER

| unit | what | files | commit |
| --- | --- | --- | --- |
| U0 | reachability preflight | — | none |
| U1 | the veil is the front door | `web/index.html` | 1 |
| U2 | the card's way out | `web/index.html` | 1 |
| U3 | routes: one home, three shells | `web/routes.json` (new), `web/menu.css` (new), `tools/routes.py` (new), the three dist scripts, the three templates, `web/shared.css` | 1 |
| U4 | the peek | `tools/collection_dist.py` | 1 |
| U5 | the shell's hand on the metronome | `src/the_board.cpp` | 1 |
| U6 | the engine sandwich: panes and behaviours | `web/index.html` | 1 |
| U7 | the ledgers and the register | `audit/*` (generated), `docs/OPEN.md` | 1 |

---

## U0 — PREFLIGHT (no commit)

```
git rev-parse --is-shallow-repository        # true → git fetch --unshallow origin
git fetch origin master                      # P9 — before any claim about master
git rev-parse HEAD
git ls-tree HEAD web/index.html web/about/index.html web/collection/index.html web/shared.css tools/web_dist.py tools/about_dist.py tools/collection_dist.py src/the_board.cpp docs/OPEN.md
ls web/routes.json web/menu.css tools/routes.py 2>&1   # all three must be ABSENT
python3 tools/routes.py 2>&1 | head -1                  # must fail: the file does not exist yet
```

Report the HEAD and the nine blobs. A blob that differs is not a halt by itself — the FIND
decides (P15). The three new files existing is a halt for U3: something landed the
campaign already.

---

## U1 — THE VEIL IS THE FRONT DOOR

Symbol boundaries: the veil markup (`<div id="veil" class="layer">` … its closing `</div>`);
the STATE 4 stylesheet block; the `#doors` stylesheet block; the ATMOS_1 binding block (the
three `veil.addEventListener` lines through the keyboard listener's closing `});`); the
`#doors` nav markup; `present()`, `showReady()`, `showCard()`; the shell's `var` block.

### U1.1 — markup: two doors in the stack

FIND
```
    <div class="stack">
      <div class="wordmark">the_board</div>
      <div id="status">Waking the world</div>
      <button id="logToggle" class="logToggle" type="button" aria-controls="log" aria-expanded="false">details</button>
    </div>
```
REPLACE
```
    <div class="stack">
      <div class="wordmark">the_board</div>
      <!-- DOORS_0 — THE SENTENCE. PLACEHOLDER COPY: Jean's words replace
           it. One line, first person (the convention about/ names). -->
      <p class="line">A living world drawn by your GPU, hung with the paintings.</p>
      <div id="status">Waking the world</div>
      <!-- DOORS_0 — TWO DOORS, STATIC MARKUP, PRESENT FROM FIRST PAINT.
           #enter is the ONLY entry-gesture target: ATMOS_1's three
           listeners bind to it (below), not to the veil, so a stray tap
           is a stray tap and a tap meant for the second door is not
           spent on the first. #enter is live during the load: an eager
           tap is BANKED — the grants are spent now, the reveal follows
           the moment the world is ready (showReady). The second door is
           an anchor: it works when no script of ours finishes. Both
           labels are PLACEHOLDER COPY. -->
      <div class="entry">
        <button id="enter" class="btn" type="button">Enter the world</button>
        <a class="btn alt" href="/about/">Visit the website</a>
      </div>
      <button id="logToggle" class="logToggle" type="button" aria-controls="log" aria-expanded="false">details</button>
    </div>
```

### U1.2 — stylesheet: READY lights the button, not the veil

FIND (the eight rule lines of the STATE 4 block)
```
    #veil.ready { cursor: pointer; }
    #veil.ready #status {
      color: var(--ink);
      border: 1px solid #33323c; border-radius: 2px;
      padding: 11px 22px;
      letter-spacing: .06em;
    }
    #veil.ready #status:focus-visible { border-color: var(--dim); outline: none; }
```
REPLACE
```
    #veil .line { margin: 0; color: var(--dim); font-size: 15px; max-width: 30em; text-align: center; }
    #veil .entry { display: flex; gap: 10px; flex-wrap: wrap; justify-content: center; }
    a.btn { text-decoration: none; display: inline-block; }
    .btn.alt { color: var(--dim); }
    .btn.alt:hover, .btn.alt:focus-visible { color: var(--ink); }
    #enter { opacity: .55; }
    #enter.armed { opacity: .8; }
    #veil.ready #enter { opacity: 1; }
```
Then, by symbol: the comment paragraph directly above those rules (it opens
`/* ── STATE 4 — READY` and ends `The log toggle and its pane opt out in JS. */`) describes
a veil that is the target. Rewrite its LAST paragraph (the one beginning `The whole veil is
the target`) to:
```
       DOORS_0: the veil is no longer the target. The doors are the two
       buttons in .entry, present from first paint; READY lights #enter
       and changes the status line's report, nothing more. */
```
Comments describe present behaviour only.

### U1.3 — stylesheet: `#doors` dies

By symbol: delete the comment paragraph directly above `#doors {` (it describes the doors
"gone while the world runs: present() hides them, showCard() returns them"), and these
rules:

FIND
```
    #doors {
      position: fixed;
      left:   calc(12px + env(safe-area-inset-left));
      bottom: calc(10px + env(safe-area-inset-bottom));
      z-index: 30;
      pointer-events: none;
      font-size: 12px; letter-spacing: 0.04em;
    }
    #doors a {
      pointer-events: auto;
      color: var(--faint);
      text-decoration: none;
      padding: 10px 8px;              /* the target outgrows the word */
    }
    #doors a:hover, #doors a:focus-visible { color: var(--ink); }
```
REPLACE with nothing.

### U1.4 — markup: the `#doors` nav dies; the noscript paragraph stays

By symbol: the WEBSITE_1 comment (`<!-- ── WEBSITE_1 — THE DOORS OUT` through `-->`) and
the `<nav id="doors" aria-label="site">` element through its `</nav>`. Replace both with:
```
  <!-- ── WEBSITE_1 / DOORS_0 — THE NOSCRIPT PAGE ───────────────────
       The doors out are static markup in the veil (.entry) and on the
       card, so they render on the visit where the adapter request fails
       and no script of ours finishes. This paragraph is the page for a
       visitor whose scripts never ran at all; its styles are inline on
       t7card's precedent — the failure that stopped the scripts may not
       have spared anything else. -->
```
The `<noscript>` block that follows is untouched.

### U1.5 — script: the door is `#enter`

FIND
```
    var doors   = document.getElementById('doors');
```
REPLACE
```
    var enterBtn = document.getElementById('enter');   // DOORS_0 — the one entry target
```

FIND (the binding block)
```
    veil.addEventListener('pointerdown', onEntryGesture);
    veil.addEventListener('pointerup',   onEntryGesture);
    veil.addEventListener('click',       onEntryGesture);
    // The keyboard's way in. A gesture is a gesture — this unlocks audio
    // and fullscreen exactly as the finger does.
    statusEl.addEventListener('keydown', function (e) {
      if (e.key === 'Enter' || e.key === ' ' || e.key === 'Spacebar') {
        e.preventDefault();
        onEntryGesture(e);
      }
    });
```
REPLACE
```
    // DOORS_0 — THE DOOR IS #enter, NOT THE VEIL. Same three listeners,
    // same handler, one target: a full-screen target made every stray
    // tap an entry, and a tap meant for the second door spent the
    // gesture on the first. A <button> fires `click` on Enter and Space
    // by itself, so the keyboard path that used to live on #status is
    // the third line. The `.armed` mark is feedback for the banked tap
    // (showReady spends it); it is cosmetic and rides its own listener
    // so the handler above stays byte-identical.
    enterBtn.addEventListener('pointerdown', onEntryGesture);
    enterBtn.addEventListener('pointerup',   onEntryGesture);
    enterBtn.addEventListener('click',       onEntryGesture);
    enterBtn.addEventListener('click', function () { if (entered && !ready) enterBtn.classList.add('armed'); });
```
The comment above the binding block ("Both pointer halves are bound and carriesActivation
picks whichever one can actually pay…") remains true and stays.

FIND (in `present()`)
```
      shown = true;
      // WEBSITE_3 — the doors leave with the veil: over the running
      // world nothing of the page remains. Any card brings them back,
      // and the belt (handoff -> present) rides this same line.
      if (doors) doors.hidden = true;
      veil.classList.add('lifting');
```
REPLACE
```
      shown = true;
      veil.classList.add('lifting');
```

FIND (in `showCard()`)
```
      veil.hidden = true;
      // WEBSITE_3 — a card on screen means no world on screen: the
      // doors return, for fallback, floor, lost and watchdog alike.
      if (doors) doors.hidden = false;
      // The door is gone with the veil that carried it. Clearing the
```
REPLACE
```
      veil.hidden = true;
      // The door is gone with the veil that carried it. Clearing the
```

FIND (the tail of `showReady()`)
```
      statusEl.textContent = ENTRY_WORDS;
      statusEl.setAttribute('role', 'button');
      statusEl.setAttribute('tabindex', '0');
      veil.classList.add('ready');
    }
```
REPLACE
```
      statusEl.textContent = ENTRY_WORDS;
      veil.classList.add('ready');
      // DOORS_0 — THE BANKED TAP. An eager visitor already spent the
      // gesture on the grants (`entered`); the reveal they asked for
      // arrives now, through the same handler, as a click — which
      // carriesActivation accepts and `entered` makes grant-free.
      if (entered) onEntryGesture({ type: 'click' });
    }
```

FIND
```
    var ENTRY_WORDS = 'tap to enter';   // placeholder; the aesthetic pass owns the words
```
REPLACE
```
    var ENTRY_WORDS = 'Ready';   // DOORS_0 — the status line's READY report; the door is #enter. Placeholder words.
```

`onEntryGesture` still carries `statusEl.removeAttribute('role')` and
`statusEl.removeAttribute('tabindex')`; both are now inert (nothing sets them). They stay,
because the handler is byte-identical by ruling; U7 registers them as a residual.

### U1 witness (P11: absence over the whole file, no `head`)
```
grep -c "veil.addEventListener"                      web/index.html   # 0
grep -c "enterBtn.addEventListener"                  web/index.html   # 4
grep -n "doors\.hidden\|getElementById('doors')\|#doors\|id=\"doors\"" web/index.html   # no lines
grep -c "__BUILD_ID__"  web/index.html               # 1  (the one-placeholder law)
grep -c "__SHADER_SHA__" web/index.html              # 1
grep -c "statusEl.setAttribute"                      web/index.html   # 0
grep -c "if (entered) onEntryGesture({ type: 'click' });" web/index.html   # 1
python3 - <<'EOF'
import re,io
t=io.open('web/index.html',encoding='utf-8',newline='').read()
assert '\r' not in t, 'CRLF crept in'
import sys; sys.path.insert(0,'tools'); import web_dist
print('boot-set violations:', web_dist.boot_set_violations(t) or 'none')
EOF
```
The last block must print `none`.

Commit: `DOORS_0 U1 — the veil is the front door: #enter takes the gesture, the second door is static, the eager tap is banked`

### U1 gate (Jean; P10/P14 — what to look for)
| should FIX | could BREAK (what you would say out loud) |
| --- | --- |
| Tapping the poster, the wordmark or empty veil does nothing; only *Enter the world* enters. | "I tapped Enter and nothing happened" — a lost activation. Check the console for `NotAllowedError`. |
| Tapping *Visit the website* mid-load goes to /about/ with no music started. | "The music started when I tapped the website button." |
| Tap Enter at 40 %: music starts, Enter dims to "armed"; when the boot finishes the world appears without a second tap. | "It said Ready and sat there" — the banked call did not fire. |
| On an iPhone the soundtrack starts on the Enter tap. | "Silent on iPhone" — activation fell off the button. |
| — | "The two buttons sit on top of the status line" / "the buttons fall off the bottom on a phone held sideways" — the SHORT VIEWPORTS block never saw a third row. |
| — | "The details toggle opened the world." |

---

## U2 — THE CARD'S WAY OUT

FIND
```
      <button class="btn" id="reload" type="button" hidden>Open it again</button>
```
REPLACE
```
      <button class="btn" id="reload" type="button" hidden>Open it again</button>
      <!-- DOORS_0 — THE WAY OUT IS STATIC. A card means no world, and
           the website is the whole piece this visitor receives. An
           anchor, so it works when nothing else does. Placeholder words.
           RULING: a link, not a redirect — the log pane beside it is the
           diagnostic surface a redirect would take away. -->
      <a class="btn alt" href="/about/">Visit the website</a>
```

Witness: `grep -c 'class="btn alt" href="/about/"' web/index.html` → **2** (veil + card).

Commit: `DOORS_0 U2 — the card's way out is a static link to the website`

Gate: with `?forcecard=1` (or whatever the shell's card rehearsal is; if none, disable WebGPU
in the browser) — the card shows the poster, the words, *Visit the website*; *Open it again*
appears only where it did before.

---

## U3 — ROUTES: ONE HOME, THREE SHELLS

### U3.1 — new file `web/routes.json`
```json
[
  { "id": "controls",   "label": "Controls",       "engine": "pane" },
  { "id": "about",      "label": "About",          "href": "/about/" },
  { "id": "collection", "label": "The collection", "href": "/collection/", "engine": "pane" },
  { "id": "text",       "label": "Text",           "href": "/about/#text" },
  { "id": "write",      "label": "Write me",       "href": "/about/#write" },
  { "id": "world",      "label": "The world",      "href": "/",        "side": "site" },
  { "id": "website",    "label": "The website",    "href": "/about/",  "side": "engine" }
]
```
Grammar (also stated in `tools/routes.py`): `side` absent = both shells; `engine: "pane"` =
on the engine shell the route is a pane button, its `href` (if any) the pane's door out; a
site page renders such a route as a link if it has an `href`, else skips it. Labels are
placeholder words. Machine-parsed: no stamp (P4 form note).

### U3.2 — new file `web/menu.css`
```css
/* ── web/menu.css — THE SANDWICH, ONE HOME (DOORS_0) ──────────────────
   Inlined into all three pages by tools/routes.py at build time; never
   shipped as a file. A native <details>: it opens with no script. The
   site pages place it in .top; the engine shell fixes it over the
   canvas (#menu.over). Colours are the palette variables every page
   declares (--ink, --dim, --bg); the font is inherited. */
.menu { position: relative; }
.menu > summary {
  list-style: none; cursor: pointer; color: var(--dim);
  letter-spacing: 0.06em; padding: 8px 10px; user-select: none;
}
.menu > summary::-webkit-details-marker { display: none; }
.menu > summary:hover, .menu > summary:focus-visible, .menu[open] > summary { color: var(--ink); }
.menu > nav {
  position: absolute; right: 0; top: 100%; z-index: 20;
  min-width: 13em; padding: 6px 0;
  background: var(--bg); border: 1px solid #33323c; border-radius: 2px;
}
.menu > nav a, .menu > nav button {
  display: block; width: 100%; box-sizing: border-box; text-align: left;
  padding: 7px 14px; margin: 0; background: none; border: 0;
  font: inherit; color: var(--dim); text-decoration: none; cursor: pointer;
}
.menu > nav a:hover, .menu > nav a:focus-visible,
.menu > nav button:hover, .menu > nav button:focus-visible { color: var(--ink); }

/* ── the engine shell only: fixed over the world, top-right; z 30 sits
   above the layers (10) and below the photo flash (40) and the organ
   panel. A pane replaces the nav (#menu.paned) so the two never overlap. */
#menu.over {
  position: fixed;
  right: calc(12px + env(safe-area-inset-right));
  top:   calc(10px + env(safe-area-inset-top));
  z-index: 30;
}
#menu.over > .pane {
  position: absolute; right: 0; top: 100%; z-index: 21;
  width: min(92vw, 34em); max-height: 70vh; overflow: auto;
  padding: 10px 14px; box-sizing: border-box;
  background: var(--bg); border: 1px solid #33323c; border-radius: 2px;
  color: var(--dim);
}
#menu.paned > nav { display: none; }
.pane .back { font: inherit; color: var(--dim); background: none; border: 0; padding: 0 0 8px; cursor: pointer; }
.pane .back:hover, .pane .back:focus-visible { color: var(--ink); }
.pane .pane-head { margin: 0 0 8px; color: var(--ink); letter-spacing: .06em; }
.pane .pane-note { margin: 0 0 10px; }
.pane dl { display: grid; grid-template-columns: max-content 1fr; gap: 4px 12px; margin: 0 0 10px; }
.pane dt { color: var(--ink); }
.pane dd { margin: 0; }
.pane .row { display: flex; gap: 8px; flex-wrap: wrap; }
.pane .row button { font: inherit; color: var(--ink); background: none; border: 1px solid #33323c; border-radius: 2px; padding: 7px 12px; cursor: pointer; }
.pane a.out { display: block; margin-top: 10px; color: var(--ink); }
.peek { display: flex; flex-wrap: wrap; gap: 6px; }
.peek-work { display: block; flex: 1 1 auto; height: 72px; aspect-ratio: var(--r, 1.3); background: var(--tone, #16120d); }
.peek-work img { width: 100%; height: 100%; object-fit: cover; display: block; }
.peek-sets { width: 100%; margin: 6px 0 0; font-size: 13px; }
```
The `.pane`, `.peek` and `.paned` rules are consumed by U6; landing them here keeps the
CSS in one file and one unit.

### U3.3 — new file `tools/routes.py`
```python
#!/usr/bin/env python3
# ─── tools/routes.py ─────────────────────────────────────────────
#
# THE ROUTE LIST HAS ONE HOME: web/routes.json. Three pages carry the
# same sandwich menu — the engine shell, about/, collection/ — and this
# is the one renderer all three dist scripts call, so a route added to
# the JSON appears on every page with no markup edit anywhere. The
# menu's rules have one home too, web/menu.css, inlined by menu_css().
# Neither file ships; both are build-time only.
#
#   side    "site" | "engine" | absent (both)
#   engine  "pane" — on the engine shell the route opens a pane inside
#           the menu (the shell owns the pane's content); its href, if
#           any, is the pane's door out. A site page renders it as a
#           link if it has an href and skips it otherwise.
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
WEB = os.path.join(os.path.dirname(HERE), "web")
ROUTES = os.path.join(WEB, "routes.json")
MENU_CSS = os.path.join(WEB, "menu.css")


def esc(s):
    return (str(s).replace("&", "&amp;").replace("<", "&lt;")
            .replace(">", "&gt;").replace('"', "&quot;"))


def load():
    with open(ROUTES, encoding="utf-8") as fh:
        routes = json.load(fh)
    for r in routes:
        if "id" not in r or "label" not in r:
            raise SystemExit("REFUSE  web/routes.json: every route needs id and label")
        if r.get("side") not in (None, "site", "engine"):
            raise SystemExit("REFUSE  web/routes.json: side must be site, engine or absent (%s)" % r["id"])
    return routes


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


def menu_css():
    with open(MENU_CSS, encoding="utf-8") as fh:
        return fh.read().rstrip("\n")


if __name__ == "__main__":
    print(nav_html("site"))
    print("---")
    print(nav_html("engine"))
```

### U3.4 — the three dist scripts inject

`tools/about_dist.py`:

FIND
```
import json
```
REPLACE
```
import json
import routes   # DOORS_0 — web/routes.json + web/menu.css, the sandwich's one renderer
```

FIND
```
def fill(template, subs):
    out = template
    for key, val in subs.items():
        marker = "<!-- __%s__ -->" % key if key not in ("HERO_DATA", "EMAIL") else None
        if key == "HERO_DATA":
            out = out.replace("/* __HERO_DATA__ */ null", json.dumps(val))
        elif key == "EMAIL":
            out = out.replace("__EMAIL__", val)
        else:
            out = out.replace(marker, val)
    for token in ("__HERO__", "__STRIP__", "__AUTHORS__", "__LINKS__",
                  "__HERO_DATA__", "__EMAIL__"):
```
REPLACE
```
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
    for token in ("__HERO__", "__STRIP__", "__AUTHORS__", "__LINKS__",
                  "__HERO_DATA__", "__EMAIL__", "__ROUTES__", "__MENU_CSS__"):
```

FIND
```
    page = fill(template, {
        "HERO": hero_tag,
```
REPLACE
```
    page = fill(template, {
        "ROUTES": routes.nav_html("site", indent="      "),   # DOORS_0
        "MENU_CSS": routes.menu_css(),                        # DOORS_0
        "HERO": hero_tag,
```

`tools/collection_dist.py`:

By symbol: add `import routes   # DOORS_0 — the sandwich's one renderer` immediately after
the last line of the stdlib import group (the line `import sys`; expected count 1).

FIND
```
def fill(template, index_html, works_html):
    out = template.replace("<!-- __INDEX__ -->", index_html)
    out = out.replace("<!-- __WORKS__ -->", works_html)
    if "__INDEX__" in out or "__WORKS__" in out:
        say("REFUSE  template placeholder did not substitute")
        sys.exit(1)
    return out
```
REPLACE
```
def fill(template, index_html, works_html):
    out = template.replace("<!-- __INDEX__ -->", index_html)
    out = out.replace("<!-- __WORKS__ -->", works_html)
    # DOORS_0 — the sandwich: links from web/routes.json, rules from
    # web/menu.css, through tools/routes.py — the renderer the engine
    # shell and about/ also use.
    out = out.replace("<!-- __ROUTES__ -->", routes.nav_html("site", indent="      "))
    out = out.replace("/* __MENU_CSS__ */", routes.menu_css())
    for token in ("__INDEX__", "__WORKS__", "__ROUTES__", "__MENU_CSS__"):
        if token in out:
            say("REFUSE  template placeholder %s did not substitute" % token)
            sys.exit(1)
    return out
```

`tools/web_dist.py`:

By symbol: add `import routes   # DOORS_0 — the sandwich's one renderer` immediately after
the stdlib import group that contains `import hashlib` (expected count of `import hashlib`: 1).

FIND
```
    shell_out = shell_out.replace(SHADER_SHA_PLACEHOLDER, shader_sha)
```
REPLACE
```
    shell_out = shell_out.replace(SHADER_SHA_PLACEHOLDER, shader_sha)

    # ── DOORS_0 — THE SANDWICH ───────────────────────────────────────
    # The route list and the menu's rules have one home each
    # (web/routes.json, web/menu.css) and one renderer (tools/routes.py),
    # the same one about_dist and collection_dist call. The shell holds
    # two markers; the build fills them. Refuse rather than ship a page
    # with no menu, on the build-id law's own precedent.
    for marker in ("<!-- __ROUTES__ -->", "/* __MENU_CSS__ */"):
        if shell_out.count(marker) != 1:
            print("")
            print("REFUSING TO SHIP A SHELL WITHOUT ITS MENU MARKER.")
            print("  web/index.html must carry %s exactly once (found %d)."
                  % (marker, shell_out.count(marker)))
            return 7
    shell_out = shell_out.replace("<!-- __ROUTES__ -->", routes.nav_html("engine", indent="      "))
    shell_out = shell_out.replace("/* __MENU_CSS__ */", routes.menu_css())
```

### U3.5 — the site templates

`web/about/index.html`:

FIND
```
<div class="top">
  <p class="site">the ever expanding board</p>
  <nav>
    <a href="../">world</a>
    <a href="../collection/">collection</a>
    <a href="#authors">authors</a>
    <a href="#write">write</a>
  </nav>
</div>
```
REPLACE
```
<div class="top">
  <p class="site">the ever expanding board</p>
  <!-- DOORS_0 — THE SANDWICH. A native <details>: it opens with no
       script, so this page keeps working with JavaScript off. The links
       are rendered by tools/routes.py from web/routes.json — the one
       home the engine shell and collection/ also read. -->
  <details class="menu">
    <summary aria-label="menu">menu</summary>
    <nav>
      <!-- __ROUTES__ -->
    </nav>
  </details>
</div>
```

FIND
```
<header class="doctrine">
```
REPLACE
```
<header class="doctrine" id="text">
```
(The `text` route lands here. The `#authors` band keeps its id; it left the menu by Jean's
route list and is reached by scrolling.)

By symbol: in the page's one `<style>` block (count of `</style>`: 1), insert this line
immediately before `</style>`:
```
/* __MENU_CSS__ */
```

`web/collection/index.html`:

FIND
```
  <nav><a href="../">the world</a> <a href="../about/">about</a></nav>
```
REPLACE
```
  <!-- DOORS_0 — THE SANDWICH (see about/). -->
  <details class="menu">
    <summary aria-label="menu">menu</summary>
    <nav>
      <!-- __ROUTES__ -->
    </nav>
  </details>
```

By symbol: in the page's one `<style>` block (count of `</style>`: 1), insert
`/* __MENU_CSS__ */` on its own line immediately before `</style>`.

`web/shared.css`:

FIND
```
.top nav { font-size: 14.5px; color: var(--dim); }
.top nav a { text-decoration: none; margin-left: 1.1em; }
.top nav a:first-child { margin-left: 0; }
.top nav a:hover, .top nav a:focus-visible { color: var(--ink); }
```
REPLACE
```
/* the masthead's own links died at DOORS_0; the sandwich (web/menu.css,
   inlined per page) stands where they stood */
.top .menu { font-size: 14.5px; }
```

### U3.6 — the engine shell's skeleton (markup + markers; behaviours are U6)

`web/index.html`:

FIND
```
    @media (prefers-reduced-motion: reduce) { #veil.lifting { transition: none; } }
```
REPLACE
```
    @media (prefers-reduced-motion: reduce) { #veil.lifting { transition: none; } }

    /* ── DOORS_0 — THE SANDWICH'S RULES land on the next line at build
       time (web_dist.py, from web/menu.css). The marker must appear
       exactly once: web_dist counts it, as it counts the build id. */
    /* __MENU_CSS__ */
```

FIND
```
  </noscript>
```
REPLACE
```
  </noscript>

  <!-- ── DOORS_0 — THE SANDWICH OVER THE WORLD ──────────────────────
       Hidden until present() and hidden again by any card: the veil
       carries its own doors. A native <details>, as on the site pages.
       Its links are web/routes.json rendered by web_dist.py at the
       marker below; its rules are web/menu.css inlined at the stylesheet
       marker above. Engine-only routes arrive as pane buttons
       (data-pane); the panes and their behaviours are the shell's own
       (DOORS_0 U6). -->
  <details id="menu" class="menu over" hidden>
    <summary aria-label="menu">menu</summary>
    <nav>
      <!-- __ROUTES__ -->
    </nav>
  </details>
```

FIND
```
    var enterBtn = document.getElementById('enter');   // DOORS_0 — the one entry target
```
REPLACE
```
    var enterBtn = document.getElementById('enter');   // DOORS_0 — the one entry target
    var menu     = document.getElementById('menu');    // DOORS_0 — the sandwich over the world
```

FIND (in `present()`, post-U1)
```
      shown = true;
      veil.classList.add('lifting');
```
REPLACE
```
      shown = true;
      if (menu) menu.hidden = false;        // DOORS_0 — the sandwich arrives with the world
      veil.classList.add('lifting');
```

FIND (in `showCard()`, post-U1)
```
      veil.hidden = true;
      // The door is gone with the veil that carried it. Clearing the
```
REPLACE
```
      veil.hidden = true;
      if (menu) { menu.hidden = true; menu.open = false; }   // DOORS_0 — no world, no menu over it
      // The door is gone with the veil that carried it. Clearing the
```

### U3 witness
```
python3 tools/routes.py                                   # prints two nav bodies; site has no data-pane; engine has three buttons/links pattern per routes.json
grep -c "<!-- __ROUTES__ -->"  web/index.html web/about/index.html web/collection/index.html   # 1 each
grep -c "/\* __MENU_CSS__ \*/" web/index.html web/about/index.html web/collection/index.html   # 1 each
grep -n "__MENU_CSS__" web/index.html    # exactly one line, the marker; NONE inside a comment (web_dist replaces every occurrence)
grep -n 'href="../collection/">collection\|href="#authors"\|href="../about/">about' web/about/index.html web/collection/index.html   # no lines (the old links are gone; the hero-plate's "the collection →" link and the door blurbs are NOT masthead links and stay)
grep -c "^\.top nav" web/shared.css      # 0
python3 tools/about_dist.py --preview /tmp/about.html && grep -c 'data-route="world"' /tmp/about.html    # 1
python3 tools/collection_dist.py --check                  # inventory prints (Pillow needed for a full build; --check does not)
```
`web_dist.py` cannot be run to completion without a build (it hashes the wasm); the marker
count law is exercised by U7's full-build gate on Jean's machine.

Commit: `DOORS_0 U3 — one route list, three shells: web/routes.json + web/menu.css through tools/routes.py; the mastheads' own links die`

### U3 gate
| should FIX | could BREAK |
| --- | --- |
| about/ and collection/ show *menu* top-right; it opens a list (About, The collection, Text, Write me, The world) with JavaScript disabled. | "The menu opens but the page jumps" (a `<summary>` inside a flex `.top` reflowing). "The dropdown is cut off at the right edge" on a phone. |
| *Text* lands on the doctrine; *Write me* on the message form. | "Text scrolls to nothing" — the `id="text"` edit missed. |
| The engine page shows nothing new yet (menu hidden until U6 lands the panes). | `web_dist.py` refuses with the marker message — a marker was duplicated or lost. |

---

## U4 — THE PEEK (`tools/collection_dist.py`)

By symbol: after `SIZES = …` (the module constants) add:
```
# DOORS_0 — THE PEEK. How many works the engine's menu shows of the
# collection: featured first, in set order, then the first work of each
# set round-robin. The pacing decision stays where it is authored
# (set.json's "featured"); this only counts.
PEEK_COUNT = 12
```

By symbol: after `def index_markup(sets):` (the whole function) add:
```
def peek_entry(s, rec, meta, featured=False):
    """One work as the engine's menu sees it: the 640 rung, the tone, the
    title and the page address of the full work. Absolute paths, because
    the engine page lives at the root."""
    first = rec["variants"][0]
    return {
        "n": rec["n"], "set": s["slug"], "setLabel": s["label"],
        "title": meta.get("title") or ("no. %d" % rec["n"]),
        "tone": rec["tone"], "r": round(rec["w"] / rec["h"], 4),
        "src": "/collection/%s/%s" % (s["slug"], first[2]),
        "href": "/collection/#w%d" % rec["n"],
        "featured": bool(featured),
    }


def peek_pick(entries):
    """Featured first, in set order; then the first work of each set,
    round-robin, until PEEK_COUNT. Keyed on (set, n): a number is unique
    within a folder, not across folders (load_sets)."""
    out = [e for e in entries if e["featured"]][:PEEK_COUNT]
    seen = set((e["set"], e["n"]) for e in out)
    lanes = {}
    for e in entries:
        lanes.setdefault(e["set"], []).append(e)
    lanes = list(lanes.values())
    i = 0
    while len(out) < PEEK_COUNT and any(lanes) and i < 10000:
        lane = lanes[i % len(lanes)]
        i += 1
        while lane and (lane[0]["set"], lane[0]["n"]) in seen:
            lane.pop(0)
        if lane:
            e = lane.pop(0)
            seen.add((e["set"], e["n"]))
            out.append(e)
    for e in out:
        e.pop("featured", None)
    return out
```

FIND (in `main()`, the loop's head)
```
    index_html_parts, sections = [], []
    bytes_jpg = bytes_avf = 0
```
REPLACE
```
    index_html_parts, sections = [], []
    bytes_jpg = bytes_avf = 0
    peek = []   # DOORS_0 — every work's record, for peek_pick
```
(Expected count 1; if the two lines are not adjacent in the tree, add `peek = []` beside
`sections`' initialisation and flag.)

FIND (the non-preview branch of the work loop)
```
                else:
                    rec = build_work(im, src_path, out_dir, n, write=True)
                    tiles.append(tile_markup(
                        s["slug"], rec, s["work_meta"].get(str(n), {}),
                        featured=n in s["featured"]))
```
REPLACE
```
                else:
                    rec = build_work(im, src_path, out_dir, n, write=True)
                    tiles.append(tile_markup(
                        s["slug"], rec, s["work_meta"].get(str(n), {}),
                        featured=n in s["featured"]))
                    peek.append(peek_entry(s, rec, s["work_meta"].get(str(n), {}),
                                           featured=n in s["featured"]))
```

FIND (the page write)
```
    with open(os.path.join(DIST, "index.html"), "w", encoding="utf-8") as fh:
        fh.write(page)
```
REPLACE
```
    with open(os.path.join(DIST, "index.html"), "w", encoding="utf-8") as fh:
        fh.write(page)
    # DOORS_0 — THE PEEK. A dozen works for the engine's menu, drawn from
    # the same records that just wrote the page, so the two cannot
    # disagree. The shell fetches it on the first opening of the menu —
    # a gesture, never boot (web_dist's boot-set law).
    with open(os.path.join(DIST, "peek.json"), "w", encoding="utf-8") as fh:
        json.dump({"sets": [{"slug": s["slug"], "label": s["label"],
                             "count": len(s["files"])} for s in sets],
                   "works": peek_pick(peek)}, fh, separators=(",", ":"))
```
(Expected count 1. `with open(os.path.join(DIST, "index.html")` also appears in the preview
branch only as `args.preview`; the DIST form is unique.)

FIND (the headers fragment)
```
                 "/collection/index.html\n"
                 "  Cache-Control: no-cache\n")
```
REPLACE
```
                 "/collection/index.html\n"
                 "  Cache-Control: no-cache\n"
                 "/collection/peek.json\n"
                 "  Cache-Control: no-cache\n")
```
Without this the `/collection/*` rule above it marks the peek immutable for a year.

Witness (needs Pillow; if absent, `--check` + a `python3 -c "import ast,sys; ast.parse(open('tools/collection_dist.py').read())"` and flag):
```
python3 tools/collection_dist.py && python3 -c "import json; d=json.load(open('dist/collection/peek.json')); print(len(d['works']), [w['href'] for w in d['works']][:3])"
grep -c "peek.json" dist/collection/_headers.fragment    # 1
```

Commit: `DOORS_0 U4 — the peek: collection_dist writes dist/collection/peek.json from the page's own records, no-cache`

---

## U5 — THE SHELL'S HAND ON THE METRONOME (`src/the_board.cpp`)

FIND
```
// Idempotent by the flag: nothing else in this program re-arms the loop
// (one set_main_loop call in the tree, no resize or veil-lift path touches
// it), so one application holds for the session.
static void apply_pace_once() {
```
REPLACE
```
// Idempotent by the flag. ONE other site retimes the loop — shell_pace,
// below, the menu's idle (DOORS_0) — and it reads the pace this one
// applied; no resize or veil-lift path touches the timing. (The older
// claim here, "nothing else re-arms the loop", was true until DOORS_0.)
static void apply_pace_once() {
```

By symbol: immediately after the closing brace of `apply_pace_once()` add:
```

// ── DOORS_0 — THE SHELL'S HAND ON THE METRONOME ──────────────────────
//
// The sandwich over the canvas asks the loop to slow while it is open —
// IDLE, NOT FREEZE: the world keeps stepping (one update per presented
// frame, as ever), the soundtrack is an HTML element rAF never touches,
// and closing the menu restores the boot pace. This is the second arm
// site, and the only other one. Transitions are witnessed (P6); the
// boot state is apply_pace_once's line. Reachable from JS through
// cwrap; the shell resolves it lazily and tolerates its absence.
static constexpr uint32_t SHELL_IDLE_PACE = 4u;   // rAF every 4th vblank: ~15 fps on a 60 Hz panel
extern "C" EMSCRIPTEN_KEEPALIVE void shell_pace(int idle) {
    if (!app || !app->pace_applied) return;   // before the loop is armed there is nothing to slow
    const uint32_t want = idle ? (app->pace > SHELL_IDLE_PACE ? app->pace : SHELL_IDLE_PACE)
                               : app->pace;
    if (want == t7::g_present_pace) return;
    t7::g_present_pace = want;
    emscripten_set_main_loop_timing(EM_TIMING_RAF, (int)want);
    std::cout << "[PACE] shell " << (idle ? "idle" : "live")
              << " -> rAF every " << want << " vblank(s)\n";
}
```
`EMSCRIPTEN_KEEPALIVE` comes from `<emscripten.h>`, already included at the head of this
file (the rAF driver). `t7::g_present_pace` is `uint32_t` (core/instruments.hpp). No new
include.

Witness:
```
grep -c "shell_pace" src/the_board.cpp        # 3 (banner, definition, the [PACE] line's neighbourhood counts as text)  — report the number, expect >= 2
grep -c "nothing else in this program re-arms" src/the_board.cpp   # 0
```
This file is sha256-pinned in `audit/COMMAND_LEDGER.md`; `command_census.py --check` is
red from this commit until U7 regenerates. Expected; say so in the report.

Commit: `DOORS_0 U5 — shell_pace: the menu's idle retimes the rAF loop; WRAP_0 U2's one-site claim revised`

Compile gate: Jean's `glaw1` / the web build. Nothing calls this yet.

---

## U6 — THE ENGINE SANDWICH: PANES AND BEHAVIOURS (`web/index.html`)

Three behaviours and no more: which pane a button opens, the peek's one fetch, and the
loop's pace while the menu is open. Plus two live controls — the tiny features.

### U6.1 — markup: the panes

FIND (post-U3)
```
    <nav>
      <!-- __ROUTES__ -->
    </nav>
  </details>
```
REPLACE
```
    <nav>
      <!-- __ROUTES__ -->
    </nav>
    <!-- THE PANES. One per engine-only route (data-pane matches the
         route id). A pane replaces the nav while open (#menu.paned) and
         carries its own way back. Controls: PLACEHOLDER COPY — the key
         words are Jean's, and their truth is console.hpp's. -->
    <div class="pane" data-pane="controls" hidden>
      <button type="button" class="back" data-back>← menu</button>
      <p class="pane-head">Controls</p>
      <dl>
        <dt>Move</dt><dd>W A S D · on a phone, drag one half of the screen</dd>
        <dt>Look</dt><dd>the mouse · drag the other half</dd>
        <dt>Ride / release</dt><dd>R</dd>
        <dt>Pulse</dt><dd>Space · a clean tap</dd>
        <dt>Free the mouse</dt><dd>Esc</dd>
      </dl>
      <div class="row">
        <button type="button" id="ctlFull">Fullscreen</button>
        <button type="button" id="ctlSound">Sound: on</button>
      </div>
    </div>
    <div class="pane" data-pane="collection" hidden>
      <button type="button" class="back" data-back>← menu</button>
      <p class="pane-head">The collection</p>
      <div class="peek" id="peek" aria-live="polite"></div>
      <a class="out" data-out target="_blank" rel="noopener" href="/collection/">Open the collection in a new tab →</a>
    </div>
  </details>
```

### U6.2 — stylesheet: the idle blur

FIND
```
    /* __MENU_CSS__ */
```
REPLACE
```
    /* __MENU_CSS__ */
    /* DOORS_0 — IDLE, NOT FREEZE. While the sandwich is open the frame
       blurs and the loop slows (shell_pace); the world keeps breathing
       behind the panel and the soundtrack keeps playing. */
    #frame { transition: filter .35s; }
    html.idle #frame { filter: blur(6px) saturate(.85); }
```
(The marker line itself is unchanged — still exactly once.)

### U6.3 — script: `goFullscreen(force)`

FIND
```
    // 2 — FULLSCREEN, coarse pointers only. A desktop visitor's entry is
    // already the pointer-lock gesture, and taking their window away
    // would be a second, louder answer to a question they did not ask.
    function goFullscreen() {
      try {
        if (!window.matchMedia || !window.matchMedia('(pointer: coarse)').matches) return;
```
REPLACE
```
    // 2 — FULLSCREEN, coarse pointers only at entry. A desktop visitor's
    // entry is already the pointer-lock gesture, and taking their window
    // away would be a second, louder answer to a question they did not
    // ask. The menu's button (DOORS_0) asks that question in words, so it
    // passes `force` and gets fullscreen on any pointer.
    function goFullscreen(force) {
      try {
        if (!force && (!window.matchMedia || !window.matchMedia('(pointer: coarse)').matches)) return;
```
The entry call `goFullscreen();` inside `onEntryGesture` is untouched (`force` undefined).

### U6.4 — script: the behaviours

FIND (post-U1)
```
    enterBtn.addEventListener('click', function () { if (entered && !ready) enterBtn.classList.add('armed'); });
```
REPLACE
```
    enterBtn.addEventListener('click', function () { if (entered && !ready) enterBtn.classList.add('armed'); });

    // ═══ DOORS_0 — THE SANDWICH'S BEHAVIOURS ═════════════════════════
    //
    // Three things: which pane a route button opens, the peek's one
    // fetch (gesture-time, never boot), and the loop's pace while the
    // menu is open — idle, not freeze (shell_pace, the_board.cpp). The
    // C ABI is resolved lazily, organ_panel.js's way: the program may
    // still be compiling when the shell runs, and a missing export is a
    // note, never a failure. Keys typed at the menu are the menu's.
    (function () {
      if (!menu) return;
      var panes = menu.querySelectorAll('.pane');
      var peekEl = document.getElementById('peek');
      var peekLoaded = false;
      var C = null;
      function abi() {
        if (C) return C;
        var M = window.Module;
        if (!M || typeof M.cwrap !== 'function') return null;
        function w(name, ret, args) {
          try { return M.cwrap(name, ret, args); }
          catch (err) { note('[shell] menu: ' + name + ' not in this build'); return null; }
        }
        C = {
          pace: w('shell_pace', null, ['number'])
        };
        return C;
      }
      function idle(on) {
        document.documentElement.classList.toggle('idle', !!on);
        var c = abi();
        if (c && c.pace) c.pace(on ? 1 : 0);
      }
      function showPane(id) {
        panes.forEach(function (p) { p.hidden = (p.dataset.pane !== id); });
        menu.classList.toggle('paned', !!id);
        if (id === 'collection') loadPeek();
      }
      function loadPeek() {
        if (peekLoaded || !peekEl) return;
        peekLoaded = true;
        fetch('/collection/peek.json', { cache: 'no-cache' }).then(function (r) {
          if (!r.ok) throw new Error('HTTP ' + r.status);
          return r.json();
        }).then(function (d) {
          var frag = document.createDocumentFragment();
          (d.works || []).forEach(function (w) {
            var a = document.createElement('a');
            a.href = w.href; a.target = '_blank'; a.rel = 'noopener';
            a.className = 'peek-work'; a.title = w.title;
            a.style.setProperty('--tone', w.tone);
            a.style.setProperty('--r', w.r);
            var img = document.createElement('img');
            img.src = w.src; img.alt = w.title; img.loading = 'lazy'; img.decoding = 'async';
            a.appendChild(img);
            frag.appendChild(a);
          });
          var sets = document.createElement('p');
          sets.className = 'peek-sets';
          sets.textContent = (d.sets || []).map(function (s) { return s.label + ' (' + s.count + ')'; }).join(' · ');
          peekEl.appendChild(frag);
          peekEl.appendChild(sets);
        }).catch(function (err) {
          peekLoaded = false;                       // try again on the next opening
          peekEl.textContent = 'The collection did not answer.';
          note('[shell] peek: ' + err);
        });
      }
      menu.addEventListener('click', function (e) {
        var t = e.target;
        if (!t || !t.closest) return;
        if (t.closest('[data-back]')) { showPane(null); return; }
        var b = t.closest('button[data-pane]');
        if (!b) return;
        showPane(b.dataset.pane);
        var out = menu.querySelector('.pane[data-pane="' + b.dataset.pane + '"] a[data-out]');
        if (out && b.dataset.href) out.href = b.dataset.href;
      });
      menu.addEventListener('toggle', function () {
        idle(menu.open);
        if (!menu.open) showPane(null);
      });
      window.addEventListener('keydown', function (e) {
        if (e.key === 'Escape' && menu.open) { menu.open = false; e.preventDefault(); }
      });
      ['keydown', 'keyup', 'keypress'].forEach(function (t) {
        menu.addEventListener(t, function (e) { if (e.key !== 'Escape') e.stopPropagation(); });
      });
      // THE TWO LIVE CONTROLS — the tiny features.
      var ctlFull = document.getElementById('ctlFull');
      var ctlSound = document.getElementById('ctlSound');
      if (ctlFull) ctlFull.addEventListener('click', function () { goFullscreen(true); });
      if (ctlSound) ctlSound.addEventListener('click', function () {
        if (!music) return;
        if (music.paused) {
          musicWasPlaying = true;                   // "sound is wanted" — the visibility handler's one input
          var p = music.play();
          if (p && p.catch) p.catch(function (err) { note('[shell] sound: ' + err); });
        } else {
          musicWasPlaying = false;                  // a visitor who chose silence stays in silence
          music.pause();
        }
        ctlSound.textContent = music.paused ? 'Sound: off' : 'Sound: on';
      });
    })();
```
`note`, `goFullscreen` are function declarations (hoisted); `menu`, `music`,
`musicWasPlaying` are the shell's own vars declared above this point. If any of the four is
not in scope where this lands, the insertion point is wrong — move the block later in the
same IIFE (after the ATMOS_0 R block) and flag; do not duplicate a declaration.

### U6 witness
```
grep -c "data-pane=\"controls\"\|data-pane=\"collection\"" web/index.html   # 2
grep -c "/\* __MENU_CSS__ \*/"  web/index.html    # 1
grep -c "<!-- __ROUTES__ -->"   web/index.html    # 1
grep -c "__BUILD_ID__"          web/index.html    # 1
grep -c "function goFullscreen(force)" web/index.html   # 1
grep -c "peek.json" web/index.html                 # 1
python3 - <<'EOF'
import io,sys; sys.path.insert(0,'tools'); import web_dist
t=io.open('web/index.html',encoding='utf-8',newline='').read()
assert '\r' not in t
print('boot-set violations:', web_dist.boot_set_violations(t) or 'none')
EOF
node -e "
const fs=require('fs');const t=fs.readFileSync('web/index.html','utf8');
const m=t.match(/<script>([\s\S]*?)<\/script>/g)||[];
for(const s of m){try{new Function(s.replace(/^<script>|<\/script>$/g,''));}catch(e){console.log('SYNTAX',e.message);process.exit(1)}}
console.log('script blocks parse:',m.length);"
```
If `node` is absent, skip the last block and flag; the browser is the gate.

Commit: `DOORS_0 U6 — the engine sandwich: controls and peek panes, idle on toggle, fullscreen and sound as live controls`

### U6 gate (Jean, `python tools\web_dist.py` then `npx wrangler pages dev dist`)
| should FIX | could BREAK |
| --- | --- |
| After entry, *menu* sits top-right; opening it blurs the world and the console prints `[PACE] shell idle -> rAF every 4 vblank(s)`; closing prints `live` and the blur lifts. Music never stops. | "The world froze" (pace not restored — a second `[PACE]` line missing). "The blur stays after closing." |
| *Controls* lists the keys and offers Fullscreen and Sound; Sound toggles the soundtrack and the label. | "Sound: on but silent" — `music.paused` read before play settled; check the console note. |
| *The collection* shows a dozen tone-coloured tiles then their images; each opens the work in a new tab; the door-out link opens `/collection/` in a new tab; the world is still running when you come back. | "The tiles are grey and never fill" — peek.json 404 (U4 not built into dist) or a src path off by one folder. "The world was gone when I came back" — the link opened in the same tab. |
| Desktop: Esc frees the mouse, then the menu is clickable; Esc again closes it. | "I can't click the menu" — pointer lock holds; the Controls copy names Esc, but a key to open the menu under lock is a residual (U7). |
| WASD typed while the menu is open does not move the pawn. | "The pawn walked while I was reading the controls." |
| — | "The menu appeared over the loading poster" — `hidden` lost on the `<details>`. |

---

## U7 — THE LEDGERS AND THE REGISTER

1. Regenerate, in this order (D9's cascade):
   ```
   python3 tools/binding_ledger.py
   python3 tools/command_census.py
   python3 tools/mirror_census.py
   python3 tools/command_census.py --check && python3 tools/mirror_census.py --check && python3 tools/binding_ledger.py --check
   ```
   Expected: only `audit/COMMAND_LEDGER.md` changes (the `src/the_board.cpp` pin). If
   `BINDING_LEDGER` or `MIRROR_LEDGER` also change, commit them and flag: something else
   moved.

2. `docs/OPEN.md` — read the file whole (P11 corollary: the head and the tail), then in the
   form its neighbours use:
   - close **DOORS_0** with one paragraph: the seven rulings above, one line each;
   - register these residuals as open items (words are Jean's; each is a fact about the tree):
     - `onEntryGesture` carries two inert `statusEl.removeAttribute` lines (DOORS_0 kept the
       handler byte-identical); delete at the next shell sweep.
     - A key to open the sandwich under pointer lock (desktop): today Esc frees the mouse,
       then the summary is clickable. Needs the key-map census before a key is chosen.
     - `web/about/index.html`: `<a class="hero" id="hero" href="../collection/">` wraps the
       full-bleed image — the tap-anywhere pathology, smaller blast radius.
     - Collection-as-profile: collapse `.lede`, `.index` as a sticky filter, sets as filter
       states over one flow; touch affordance on `.work`; swipe in the viewer.
     - PWA manifest + Apple meta for a true fullscreen on iOS; fullscreen re-entry beyond
       the menu button.
     - Adaptive GPU pacing (gen-8 Intel reference) — TWO_DOORS Task 3, not scoped here.
     - Copy: the veil sentence, both door labels, `ENTRY_WORDS`, the Controls table (verify
       against `console.hpp`'s touch halves before the words are set), route labels.
     - Peek count and pick rule (`PEEK_COUNT`, `peek_pick`) are a first authoring, to be
       looked at, not tuned.
   - note that **VISIT_0** (the roll as directory) is a separate held-branch handoff and
     depends on DOORS_0 U3/U6 for its pane.

Commit: `DOORS_0 U7 — ledgers regenerated (the_board.cpp pin); OPEN.md: DOORS_0 closed, residuals registered`

---

## THE ROUND'S REPORT

One report at the end, the usual shape: per unit — landed / flagged / halted, the witness
outputs verbatim, every default taken under DEFAULT-AND-FLAG, every count that differed
from this document and what the tree said instead. Cite symbols. Then the gate tables above
are Jean's; the shell gate (`tools/gates/shell_gate/run.py`) is unaffected by design (it
reads `organ_panel.js`, not the shell) — say so rather than claiming it proves the menu.

## WHAT THIS DOCUMENT CANNOT SEE (P16)

It read `web/index.html` in the regions it quotes and around them, not whole; the SHORT
VIEWPORTS media block and the `.stack`/`#veil` layout rules were not read, so the two-button
row's fit on a sideways phone is a gate row, not a claim. It did not read `console.hpp`'s
touch-half assignment, so the Controls copy is placeholder in fact as well as in name. It
did not run `web_dist.py` end to end (no wasm here), so the marker law's refusal path is
exercised first on Jean's machine.
