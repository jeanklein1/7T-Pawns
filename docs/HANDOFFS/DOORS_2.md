# DOORS_2 — THE MENU AS THE WEBSITE IN MINIATURE

*Handoff · authored 2026-09-06 · Claude → CC · Jean holds build, visual, merge, deploy.*
*Filed to `docs/HANDOFFS/DOORS_2.md` while open. Lands after DOORS_1.*

## THE RULINGS

1. **One door for the build.** `tools/dist.py` runs the four stages in the one lawful order
   and stops at the first that fails. The halves stay agnostic — the scripts are unchanged
   siblings and `collection_gate.py` still keeps the collection engine-free — but the act of
   putting them together is one command. Deploy stays a hand.
2. **The return path is the browser's own tab.** Every site link from the engine opens a new
   tab with `rel="opener"` (same origin — ours); the world tab is never left. On the site, *The
   world* closes the site tab when an opener exists — the world is still there, exactly where
   it was — and boots a fresh world otherwise. This holds however deep the visitor browses,
   because same-origin navigation keeps the opener. No bfcache reliance.
3. **The sandwich is a glyph.** Three bars, drawn in CSS, `aria-label="menu"`. Familiarity
   buys the strangeness; the word was honest but competes with the piece.
4. **"Controls" becomes "The Board."** One pane about the engine: what it is (Jean's
   sentence), the controls (accurate to `input.hpp` — walk, look, pulse & leap, aura, ride,
   first person, other worlds/arches, free the mouse), things to try (Jean's), the live row,
   and the world's own photographs folded in at the bottom. The *Photographs* route dies.
5. **Text and Write me become panes** — the about page in miniature. `about_dist.py` emits
   `dist/about/about.json` (the doctrine's own markup + the address) from the page it just
   wrote, peek.json's law; the engine fetches it on first open. The Write pane posts to the
   same `api/message` the website posts to (it takes JSON), with a mailto fallback.
6. **The visitor's own camera.** *Take a photo* hands the canvas's last frame to the visitor's
   device — the share sheet on a phone (Messages, Mail, AirDrop, Photos), a download on a
   desktop. This is "send a picture to yourself" in its honest first form: no server, no
   readback, no address to abuse. The world photographs itself for its walls; the visitor
   photographs the world for themselves. HYPOTHESIS (P1): a WebGPU canvas keeps its last
   presented image for `toBlob`. A black picture is the gate row; the fallback is registered.
7. **Miniatures of the world's photographs are not built.** They need the GPU→CPU texture
   readback DOORS_0's recon priced (a fifth readback machine, a channel swap, a megabyte a
   shot). It is registered once, as the one mechanism that would yield both the miniatures
   and "the world's own photographs sent to the visitor" — and not built until a measurement
   asks. The roll stays what it is: a list that takes you there.
8. **The pilot aims only on approach, and proportionally.** The camera oscillated because the
   orbit was steered every frame from a stale, kiting eye. Now the walk leaves the camera
   alone until the last stretch, then eases the orbit toward the picture (exponential, not
   bang-bang). The walk's speed is the pawn's — a second speed for the pilot would break the
   hand's law — and the roll lists nearest first.

Words are placeholders everywhere a sentence appears; the controls FACTS are not.

## AUTHORITY

Base: master after DOORS_1 (report HEAD at U0). Lands on master. Counts are 1 unless stated;
boundaries are symbols (P2); DEFAULT-AND-FLAG (P17); LF only.

| file | note |
| --- | --- |
| `tools/dist.py` | new |
| `tools/routes.py`, `web/routes.json`, `web/menu.css` | post-DOORS_1 |
| `web/index.html`, `web/about/index.html`, `web/collection/index.html` | post-DOORS_1 |
| `tools/about_dist.py` | post-DOORS_1 |
| `src/cartridges/the_board/direction/input.hpp` | post-DOORS_1 (not ledger-pinned) |
| `docs/OPEN.md` | — |

## UNITS

| unit | what | commit |
| --- | --- | --- |
| U0 | preflight | none |
| U1 | `tools/dist.py` — one door | 1 |
| U2 | the return path (renderer + two `rel`s) | 1 |
| U3 | the sandwich glyph | 1 |
| U4 | The Board pane; the Photographs route dies; the camera | 1 |
| U5 | Text and Write me panes; `about.json` | 1 |
| U6 | the pilot's aim | 1 |
| U7 | OPEN.md; checks | 1 |

---

## U0 — PREFLIGHT
```
git fetch origin master && git rev-parse HEAD
git ls-tree HEAD tools/routes.py web/routes.json web/menu.css web/index.html web/about/index.html web/collection/index.html tools/about_dist.py src/cartridges/the_board/direction/input.hpp docs/OPEN.md
ls tools/dist.py 2>&1                                  # must be absent
grep -n "def rel(href, base)" tools/routes.py           # 1 — DOORS_1 landed
grep -rn "possess(" src/cartridges/the_board/direction/input.hpp src/console/console.hpp | grep -v "^.*inline void possess\|^.*void possess" | head   # WHICH KEY presses possess — the Board's "Ride" row must name it (expected R; if not, use the key found and flag)
grep -n "request.json()\|formData()\|\.get(\"name\")\|\.get(\"message\")\|website" functions/api/message.js   # the field names the Write pane must send
```

---

## U1 — ONE DOOR: `tools/dist.py` (new)

```python
#!/usr/bin/env python3
# ─── tools/dist.py — THE ONE DOOR TO dist/ (DOORS_2) ─────────────────
#
# The site is two agnostic halves and one act of putting them together.
# The halves stay siblings — collection_dist.py, about_dist.py and
# web_dist.py know nothing of each other, and collection_gate.py keeps the
# collection engine-free — and the ORDER lives here and nowhere else: the
# collection first (its _headers fragment feeds the root), then about (its
# hero rides beside it), then the gate, then web_dist LAST (it deletes only
# the engine's own names and folds the fragment). Stops at the first stage
# that fails, in that stage's own words.
#
#   cmake --build --preset the-board-web     # the engine, first (Jean)
#   python tools/dist.py                     # everything into dist/
#   npx wrangler pages deploy dist --project-name=7t   # deploy stays a hand
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

STAGES = (
    ("collection", "tools/collection_dist.py"),
    ("about",      "tools/about_dist.py"),
    ("gate",       "tools/gates/collection_gate.py"),
    ("web",        "tools/web_dist.py"),
)


def main():
    for name, script in STAGES:
        print("\n== dist: %s  (%s) ==" % (name, script), flush=True)
        r = subprocess.run([sys.executable, os.path.join(ROOT, script)], cwd=ROOT)
        if r.returncode != 0:
            print("\ndist: STOPPED at %s (exit %d) — nothing after it ran." % (name, r.returncode))
            return r.returncode
    print("\ndist: complete — deploy with: npx wrangler pages deploy dist --project-name=7t")
    return 0


if __name__ == "__main__":
    sys.exit(main())
```
Witness: `python3 tools/dist.py` on a machine with the engine built runs the four in order;
here, without the wasm, it must stop at `web` with web_dist's own refusal and print
`STOPPED at web`. Propose in the report the one-line change to CLAUDE.md's build sequence
(`python tools\web_dist.py` → `python tools\dist.py`); the file is Jean's.

Commit: `DOORS_2 U1 — tools/dist.py: the one door to dist/, the build order's one home; deploy stays a hand`

---

## U2 — THE RETURN PATH

### U2.1 — `web/routes.json`

FIND
```
  { "id": "world",      "label": "The world",      "href": "/",        "side": "site" },
```
REPLACE
```
  { "id": "world",      "label": "The world",      "href": "/",        "side": "site", "return": true },
```

### U2.2 — `tools/routes.py`

FIND
```
#   side    "site" | "engine" | absent (both)
```
REPLACE
```
#   side    "site" | "engine" | absent (both)
#   return  true — on a site page, when this tab was opened by the engine
#           (window.opener, same origin), the link CLOSES the tab: the world
#           is still there, where it was. Without an opener it navigates.
#           The engine's own links open a new tab with rel="opener" for
#           exactly this, whatever the visitor browses in between.
```

FIND
```
        if not href:
            continue
        items.append('<a data-route="%s" href="%s">%s</a>'
                     % (esc(r["id"]), esc(href), esc(r["label"])))
    return ("\n" + indent).join(items)
```
REPLACE
```
        if not href:
            continue
        attrs = ""
        if side == "engine":
            # DOORS_2 — the world tab is never left; rel=opener because
            # _blank implies noopener since 2021 and the return path needs it.
            attrs = ' target="_blank" rel="opener"'
        elif r.get("return"):
            # DOORS_2 — close-or-navigate: close() only counts if the tab
            # actually closed (window.closed), else the link runs.
            attrs = ' onclick="return !(window.opener &amp;&amp; !window.opener.closed &amp;&amp; (window.close(), window.closed))"'
        items.append('<a data-route="%s" href="%s"%s>%s</a>'
                     % (esc(r["id"]), esc(href), attrs, esc(r["label"])))
    return ("\n" + indent).join(items)
```
(`&amp;&amp;` inside an attribute value is `&&` to the parser; the gate's FORBIDDEN list
names no word used here.)

### U2.3 — the two `rel`s in the engine shell (`web/index.html`)

FIND
```
      <a class="out" data-out target="_blank" rel="noopener" href="/collection/">Open the collection in a new tab →</a>
```
REPLACE
```
      <a class="out" data-out target="_blank" rel="opener" href="collection/">Open the collection in a new tab →</a>
```

FIND
```
            a.href = w.href; a.target = '_blank'; a.rel = 'noopener';
```
REPLACE
```
            a.href = w.href; a.target = '_blank'; a.rel = 'opener';   // DOORS_2 — the site tab can find its way back
```

Witness:
```
python3 tools/routes.py | grep -c 'rel="opener"'      # every engine link
python3 tools/routes.py | grep -c 'onclick='          # 2 (world, on /about/ and on /collection/)
grep -c "noopener" web/index.html                     # 0 apart from the CONTACT link, if any — report the lines
```

Commit: `DOORS_2 U2 — the return path: engine links open a tab with rel=opener; The world closes it when an opener exists, boots otherwise`

Gate (Jean): from the engine, open About; browse to the collection; open a work; choose *The
world* in the menu → the site tab closes and the world is where you left it, music resumed.
Open `/about/` directly, choose *The world* → a fresh boot. BREAK: "the tab did not close and
nothing happened" — a browser refusing `close()` on a link-opened tab; the `window.closed`
test falls through to the navigation, so the failure would be a fresh boot, not a dead link.

---

## U3 — THE SANDWICH GLYPH

`web/menu.css` FIND
```
.menu > summary::-webkit-details-marker { display: none; }
```
REPLACE
```
.menu > summary::-webkit-details-marker { display: none; }
/* the glyph: three bars, drawn — a unicode ☰ renders as whatever the
   font has, which over the world is often a box. The accessible name
   is the summary's aria-label; the span is decoration. */
.menu > summary .bars {
  display: inline-block; width: 22px; height: 14px; vertical-align: middle;
  border-top: 2px solid currentColor; border-bottom: 2px solid currentColor;
  position: relative;
}
.menu > summary .bars::after {
  content: ""; position: absolute; left: 0; right: 0; top: 4px;
  border-top: 2px solid currentColor;
}
.menu[open] > summary .bars { opacity: .6; }
```

Three templates, the same line — `web/index.html`, `web/about/index.html`, `web/collection/index.html`:

FIND
```
<summary aria-label="menu">menu</summary>
```
REPLACE
```
<summary aria-label="menu"><span class="bars" aria-hidden="true"></span></summary>
```
(Count 1 per file; leading indentation differs per file — match the line's content.)

Commit: `DOORS_2 U3 — the sandwich is a glyph: three bars in CSS, one home, three summaries`

---

## U4 — THE BOARD

### U4.1 — routes (`web/routes.json`)

FIND
```
  { "id": "controls",   "label": "Controls",       "engine": "pane" },
  { "id": "roll",       "label": "Photographs",    "engine": "pane" },
```
REPLACE
```
  { "id": "board",      "label": "The Board",      "engine": "pane" },
```

### U4.2 — the pane (`web/index.html`)

By symbol: the Controls pane (`<div class="pane" data-pane="controls" hidden>` through its
closing `</div>` — the one before `<div class="pane" data-pane="collection" hidden>`) is
replaced whole; the Photographs pane (`<div class="pane" data-pane="roll" hidden>` through its
closing `</div>`) is deleted whole, its list moving into The Board.

REPLACE the Controls pane with:
```
    <div class="pane" data-pane="board" hidden>
      <button type="button" class="back" data-back>← menu</button>
      <p class="pane-head">The Board</p>
      <!-- PLACEHOLDER COPY — Jean's sentence. -->
      <p class="pane-note">A living world drawn by your GPU. It photographs itself and hangs the pictures; the paintings it hangs are on the website.</p>
      <p class="pane-sub">Controls</p>
      <!-- THE FACTS ARE input.hpp's and console.hpp's (DOORS_2 U0 verified the
           ride key); the WORDS are placeholders. -->
      <dl>
        <dt>Walk</dt><dd>W A S D · on a phone, drag the left half</dd>
        <dt>Look</dt><dd>drag the mouse · drag the right half, two fingers to zoom</dd>
        <dt>Pulse &amp; leap</dt><dd>Space · a clean tap — rings the ground; the body leaps, or somersaults if it is already in the air</dd>
        <dt>Aura</dt><dd>3 lights it · 2 raises it</dd>
        <dt>Ride the ribbon</dt><dd>R · R again to step off</dd>
        <dt>First person</dt><dd>Ctrl</dd>
        <dt>Other worlds</dt><dd>walk through an arch · or 4 to 9 to jump</dd>
        <dt>Free the mouse</dt><dd>Esc</dd>
      </dl>
      <p class="pane-sub">Things to try</p>
      <!-- PLACEHOLDER COPY — Jean's suggestions. -->
      <ul class="try">
        <li>Stand by a wall of pictures and wait for the next one to be hung.</li>
        <li>Press Space among the cubes and watch where they go.</li>
        <li>Find an arch and walk through it.</li>
      </ul>
      <div class="row">
        <button type="button" id="ctlPhoto">Take a photo</button>
        <button type="button" id="ctlFull">Fullscreen</button>
        <button type="button" id="ctlSound">Sound: on</button>
      </div>
      <p class="pane-sub">Its own photographs</p>
      <p class="pane-note">What the world has hung of its own pictures. Choose one and the pawn walks there; touch anything to stop.</p>
      <ol class="roll" id="roll"></ol>
      <p class="roll-state" id="rollState" hidden></p>
    </div>
```

### U4.3 — the behaviours (`web/index.html`)

FIND
```
        if (id === 'roll') startRoll(); else stopRoll();     // VISIT_0
```
REPLACE
```
        if (id === 'board') startRoll(); else stopRoll();    // VISIT_0 / DOORS_2: the roll lives in The Board
```

FIND (the sound control's `if (ctlSound)` block ends the live-controls section; anchor on its last line)
```
        ctlSound.textContent = music.paused ? 'Sound: off' : 'Sound: on';
      });
```
REPLACE
```
        ctlSound.textContent = music.paused ? 'Sound: off' : 'Sound: on';
      });
      // THE VISITOR'S OWN CAMERA (DOORS_2). The world photographs itself
      // for its walls; this is the other camera — what the visitor sees,
      // handed to their own device: the share sheet where there is one
      // (Messages, Mail, AirDrop, Photos), a download where there is not.
      // No server, no readback, no address to abuse. HYPOTHESIS (P1): a
      // WebGPU canvas keeps its last presented image for toBlob; a black
      // picture is the gate row, and the fallback is registered.
      var ctlPhoto = document.getElementById('ctlPhoto');
      var canvasEl = document.getElementById('canvas');
      if (ctlPhoto && canvasEl) ctlPhoto.addEventListener('click', function () {
        try {
          canvasEl.toBlob(function (blob) {
            if (!blob) { note('[shell] photo: the canvas gave no image'); return; }
            var name = 'the_board_' + (window.T7_BUILD_ID || '') + '_'
                     + new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19) + '.png';
            var file = window.File ? new File([blob], name, { type: 'image/png' }) : null;
            if (file && navigator.share && navigator.canShare && navigator.canShare({ files: [file] })) {
              navigator.share({ files: [file], title: 'the_board' })
                .catch(function (err) { if (!err || err.name !== 'AbortError') note('[shell] photo: ' + err); });
              return;
            }
            var url = URL.createObjectURL(blob);
            var a = document.createElement('a');
            a.href = url; a.download = name;
            document.body.appendChild(a); a.click(); a.remove();
            setTimeout(function () { URL.revokeObjectURL(url); }, 1000);
          }, 'image/png');
        } catch (err) { note('[shell] photo: ' + err); }
      });
```

### U4.4 — the rules (`web/menu.css`)

Append:
```

/* ── DOORS_2 — The Board's sections ───────────────────────────────── */
.pane .pane-sub { margin: 12px 0 6px; color: var(--ink); letter-spacing: .06em; font-size: 13px; }
.pane ul.try { margin: 0 0 10px; padding-left: 1.1em; }
.pane ul.try li { margin: 0 0 4px; }
```

Witness:
```
grep -c 'data-pane="board"' web/index.html      # 1
grep -c 'data-pane="controls"\|data-pane="roll"' web/index.html   # 0
grep -c "id=\"ctlPhoto\"" web/index.html         # 1
python3 tools/routes.py | grep -c 'data-pane="board"'   # 1
grep -c "__BUILD_ID__" web/index.html            # 1
node -e "const t=require('fs').readFileSync('web/index.html','utf8');for(const s of t.match(/<script>([\s\S]*?)<\/script>/g)||[]){new Function(s.replace(/^<script>|<\/script>$/g,''))}console.log('parses')"
```

Commit: `DOORS_2 U4 — The Board: what it is, accurate controls, things to try, the visitor's camera; the world's photographs folded in, the Photographs route dies`

Gate (Jean): *Take a photo* on a phone opens the share sheet with a PNG of the world; on a
desktop a PNG downloads. BREAK: "the photo is black" — the hypothesis failed; register and
the fallback is an engine-side copy of the backbuffer into a CopySrc texture, which is the
readback DOORS_0 priced. "The photo shows the cursor orb" — a roadmap item, not this round.

---

## U5 — TEXT AND WRITE ME, IN MINIATURE

### U5.1 — `about.json` (`tools/about_dist.py`)

FIND
```
    with open(os.path.join(DIST, "index.html"), "w", encoding="utf-8") as fh:
        fh.write(page)
```
REPLACE
```
    with open(os.path.join(DIST, "index.html"), "w", encoding="utf-8") as fh:
        fh.write(page)
    # DOORS_2 — THE ABOUT PAGE IN MINIATURE. The doctrine's own markup and
    # the address, for the engine's Text and Write panes; taken from the
    # page just written, so the two cannot disagree (peek.json's law). The
    # template stays the doctrine's one home.
    m = re.search(r'<header class="doctrine" id="text">(.*?)</header>', page, re.S)
    if not m:
        say("REFUSE  the doctrine header is not in the page — the engine's Text pane would have nothing to show")
        sys.exit(1)
    with open(os.path.join(DIST, "about.json"), "w", encoding="utf-8") as fh:
        json.dump({"html": m.group(1).strip(), "email": site["email"]}, fh)
```
By symbol: add `import re` to the stdlib imports if absent (expected: absent; flag either way).

### U5.2 — routes (`web/routes.json`)

FIND
```
  { "id": "text",       "label": "Text",           "href": "/about/#text" },
  { "id": "write",      "label": "Write me",       "href": "/about/#write" },
```
REPLACE
```
  { "id": "text",       "label": "Text",           "href": "/about/#text",  "engine": "pane" },
  { "id": "write",      "label": "Write me",       "href": "/about/#write", "engine": "pane" },
```
(The site pages still render them as links; the engine as panes with the href as door out.)

### U5.3 — the panes (`web/index.html`)

By symbol: immediately after the collection pane's closing `</div>` (the one that follows the
`a.out` line edited in U2.3), before `</details>`, insert:
```
    <!-- DOORS_2 — THE ABOUT PAGE IN MINIATURE. Two panes fed by one fetch of
         about/about.json (gesture-time, never boot): the doctrine as the
         site wrote it, and the same message door the site opens, posting to
         the same function (functions/api/message.js takes JSON). Words are
         placeholders; the address is site.json's. -->
    <div class="pane" data-pane="text" hidden>
      <button type="button" class="back" data-back>← menu</button>
      <p class="pane-head">Text</p>
      <div class="text" id="miniText"></div>
      <a class="out" data-out target="_blank" rel="opener" href="about/#text">Read it on the website →</a>
    </div>
    <div class="pane" data-pane="write" hidden>
      <button type="button" class="back" data-back>← menu</button>
      <p class="pane-head">Write me</p>
      <form id="mini" method="post" action="api/message">
        <label>name<input name="name" autocomplete="name"></label>
        <label>email, if you want an answer<input name="email" type="email" autocomplete="email"></label>
        <label>message<textarea name="message" required rows="4"></textarea></label>
        <label class="hp" aria-hidden="true">leave this empty<input name="website" tabindex="-1" autocomplete="off"></label>
        <div class="row"><button type="submit" id="miniSend">Send</button><span class="said" id="miniSaid"></span></div>
      </form>
      <a class="out" data-out target="_blank" rel="opener" href="about/#write">Write from the website →</a>
    </div>
```

### U5.4 — the behaviours (`web/index.html`)

FIND
```
        if (id === 'collection') loadPeek();
```
REPLACE
```
        if (id === 'collection') loadPeek();
        if (id === 'text' || id === 'write') loadAbout();    // DOORS_2
```

FIND (the peek loader's opening line)
```
      function loadPeek() {
```
REPLACE
```
      // DOORS_2 — ONE FETCH FOR TWO PANES: the doctrine (trusted markup — it
      // is our own template, written by about_dist) and the address the
      // Write pane falls back to.
      var about = null, aboutLoading = false;
      var miniText = document.getElementById('miniText');
      function loadAbout() {
        if (about || aboutLoading) return;
        aboutLoading = true;
        fetch('about/about.json', { cache: 'no-cache' }).then(function (r) {
          if (!r.ok) throw new Error('HTTP ' + r.status);
          return r.json();
        }).then(function (d) {
          about = d;
          if (miniText) miniText.innerHTML = d.html || '';
        }).catch(function (err) {
          aboutLoading = false;                      // retry on the next opening
          if (miniText) miniText.textContent = 'The website did not answer.';
          note('[shell] about: ' + err);
        });
      }
      var mini = document.getElementById('mini');
      var miniSaid = document.getElementById('miniSaid');
      if (mini) mini.addEventListener('submit', function (e) {
        e.preventDefault();
        var data = {};
        ['name', 'email', 'message', 'website'].forEach(function (k) {
          var f = mini.elements[k]; data[k] = f ? f.value : '';
        });
        miniSaid.textContent = 'Sending…';
        fetch('api/message', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(data) })
          .then(function (r) { if (!r.ok) throw new Error('HTTP ' + r.status); miniSaid.textContent = 'Sent. Thank you.'; mini.reset(); })
          .catch(function (err) {
            note('[shell] write: ' + err);
            var mail = about && about.email ? about.email : '';
            miniSaid.innerHTML = 'It did not go through' + (mail ? ' — <a href="mailto:' + mail + '">mail me instead</a>' : ' — try the website') + '.';
          });
      });
      function loadPeek() {
```
Verify against U0's reading of `functions/api/message.js`: the JSON keys it reads must be
`name`, `email`, `message`, `website`; if they differ, use its names and flag.

### U5.5 — the rules (`web/menu.css`)

Append:
```

/* ── DOORS_2 — the about page in miniature ─────────────────────────── */
.pane .text h1 { font-size: 18px; font-weight: 430; margin: 0 0 8px; color: var(--ink); }
.pane .text p { margin: 0 0 8px; }
.pane form label { display: block; margin: 0 0 8px; }
.pane form input, .pane form textarea {
  display: block; width: 100%; box-sizing: border-box; margin-top: 3px;
  font: inherit; color: var(--ink); background: none;
  border: 1px solid #33323c; border-radius: 2px; padding: 6px 8px;
}
.pane form .hp { position: absolute; left: -9999px; }
.pane form .said { align-self: center; }
```

Witness:
```
python3 tools/about_dist.py --preview /tmp/about.html   # if assets/about is present here; else --check-equivalent: python3 -c "import ast;ast.parse(open('tools/about_dist.py').read())"
grep -c 'data-pane="text"\|data-pane="write"' web/index.html   # 2
grep -c "about/about.json" web/index.html          # 1
python3 tools/routes.py | grep -c 'data-pane="text"\|data-pane="write"'   # 2 (engine side)
python3 - <<'EOF'
import io,sys; sys.path.insert(0,'tools'); import web_dist
t=io.open('web/index.html',encoding='utf-8',newline='').read(); assert '\r' not in t
print('boot-set violations:', web_dist.boot_set_violations(t) or 'none')
EOF
```

Commit: `DOORS_2 U5 — Text and Write me as panes: about_dist emits about.json from the page it wrote; the Write pane posts to the site's own function`

Gate (Jean): *Text* shows the doctrine as the about page shows it; *Write me* sends (the Pages
project needs `RESEND_API_KEY` and `MESSAGE_TO` — on `7t-exl` it may not, and the pane then
offers the mailto). BREAK: "the Text pane is blank" — `about.json` 404: about_dist did not
run, or the doctrine header lost its id.

---

## U6 — THE PILOT'S AIM (`input.hpp`)

FIND
```
inline constexpr float  PILOT_STANDOFF_MIN_WU = 4.0f;
```
REPLACE
```
inline constexpr float  PILOT_STANDOFF_MIN_WU = 4.0f;
inline constexpr float  PILOT_AIM_WU          = 24.0f;  // DOORS_2 — the orbit is steered only inside this; the kite follows the walk until then
inline constexpr float  PILOT_AIM_GAIN        = 3.0f;   // DOORS_2 — 1/s: the orbit closes on the picture exponentially, no hunting
```

FIND
```
    const float vx = p.aim_x - c->camera_pose_.eye[0];
    const float vz = p.aim_z - c->camera_pose_.eye[2];
    float want = std::atan2(-vx, -vz) - az;
    while (want >  3.14159265f) want -= 6.28318531f;
    while (want < -3.14159265f) want += 6.28318531f;
    const float step = PILOT_TURN_RATE * (float)dt;
    if (std::fabs(want) > 0.0175f)
        c->inputState_.look_az_delta = std::max(-step, std::min(step, want));
}
```
REPLACE
```
    // THE AIM, ONLY ON APPROACH (DOORS_2). Steering the orbit every frame
    // from a kiting, one-frame-stale eye hunted: the eye moved as the
    // orbit turned and the target angle moved with it. So the walk leaves
    // the camera alone — the kite follows the pawn as it does for a hand —
    // and only inside PILOT_AIM_WU does the orbit ease onto the picture,
    // proportionally (a fraction of the error per second, capped at the
    // turn rate), which converges without overshoot.
    if (d > PILOT_AIM_WU) return;
    const float vx = p.aim_x - c->camera_pose_.eye[0];
    const float vz = p.aim_z - c->camera_pose_.eye[2];
    float want = std::atan2(-vx, -vz) - az;
    while (want >  3.14159265f) want -= 6.28318531f;
    while (want < -3.14159265f) want += 6.28318531f;
    if (std::fabs(want) <= 0.0175f) return;
    const float step = PILOT_TURN_RATE * (float)dt;
    const float ease = want * std::min(1.0f, PILOT_AIM_GAIN * (float)dt);
    c->inputState_.look_az_delta = std::max(-step, std::min(step, ease));
}
```
Witness: `python3 tools/gates/console_gate/run.py` → PASS; `grep -c PILOT_AIM_WU src/cartridges/the_board/direction/input.hpp` → 2.
`input.hpp` is not ledger-pinned; no cascade.

Commit: `DOORS_2 U6 — the pilot aims only on approach, and proportionally: the kite follows the walk, the orbit eases onto the picture without hunting`

Gate (Jean): choose a photograph → the camera stays yours for the walk; in the last stretch it
swings once onto the picture and settles. BREAK: "it still oscillates at the end" — lower
`PILOT_AIM_GAIN` to 2; "it never faces the picture" — raise `PILOT_AIM_WU`.

---

## U7 — THE REGISTER

`docs/OPEN.md` (read whole): close **DOORS_2** with the eight rulings, one line each; register:
- The readback (exhibition texture → CPU) as the one mechanism that would yield miniatures of
  the world's photographs in the menu and "the world's own photographs sent to the visitor";
  priced at DOORS_0's recon; not built until a measurement asks.
- The camera hypothesis's fallback (an engine-side copy of the backbuffer to a CopySrc
  texture) if the gate finds a black picture.
- The cursor orb appears in the visitor's photo — the roadmap's "hidden during recording" item
  now has a second reader.
- The about page's big *the world* door (`.door` and the hero-plate link) boots a fresh world
  even when the tab has an opener; the menu's route returns. Decide whether the doors should
  return too (one attribute each, in the template).
- CLAUDE.md's build line (`python tools\dist.py`) and the collection gate's table row — Jean's.
- Email-to-self of the visitor's photo through `api/message` (Resend takes attachments) is
  possible and is an open relay unless gated; not built.
- Copy: The Board's sentence and suggestions, the pane sentences, the send/fail words.

Then: `python3 tools/gates/score/run.py && python3 tools/gates/shell_gate/run.py && python3 tools/gates/console_gate/run.py && python3 tools/command_census.py --check && python3 tools/mirror_census.py --check && python3 tools/binding_ledger.py --check` — all green (no pinned file moved this round; if one did, run the cascade and flag).

Commit: `DOORS_2 U7 — OPEN.md: DOORS_2 closed, the readback priced once, the camera's fallback and the doors' return registered`

---

## WHAT THIS DOCUMENT CANNOT SEE (P16)

`assets/about/site.json` and the paintings are Jean's, so `about.json` is proven by reading
`about_dist.py`, not by running it. The ride key was not found under `case GLFW_KEY_R` in
`on_key_down` — U0 finds which key presses `possess()` and the Board's row follows it. The
WebGPU-canvas `toBlob` behaviour is a hypothesis with a gate, not a fact of the tree. Whether
`window.close()` closes a link-opened tab after several navigations is browser policy; the
`window.closed` test is the guard, and the failure mode is a fresh boot.
