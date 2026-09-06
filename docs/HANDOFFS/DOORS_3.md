# DOORS_3 — THE LAYOUT ROUND

*Handoff · authored 2026-09-06 · Claude → CC · Jean holds build, visual, merge, deploy.*
*Filed to `docs/HANDOFFS/DOORS_3.md` while open. Lands after DOORS_2 on master.*

## JEAN'S BRIEF, AS RULINGS

1. **The veil says two names: *The Board* and *About*.** The sentence leaves; *details* leaves.
   No GPU talk anywhere a visitor reads before choosing — the noscript paragraph included.
2. **The fallback speaks Jean's words**, verbatim: *Welcome. The board is a living,
   ever-expanding world that hangs Jean Klein's paintings. Meanwhile the collection is right
   here.* — "the collection" is the link, and the card's static door goes to the collection.
   Floor, lost and watchdog are different failures and keep their own words.
3. **Every item on the engine's menu opens a pane.** The Board (Jean's messages and hints),
   Controls, The collection, Text, About, Write me, Follow the work. Each pane has one door
   out. *The website* route dies — About is the website.
4. **Controls is its own pane with two platforms side by side**, Smartphone | Desktop, one
   shown at a time. Six rows, exhaustive: Directions, Rotation, Pulse, Aura, Leap, FPV. The
   default is the visitor's own device (coarse pointer → Smartphone) — one tap to the other;
   Jean can pin the default to Smartphone by one constant. The live row (Take a photo,
   Fullscreen, Sound) moves here from The Board: they are controls.
5. **The photographer is parked.** The roll leaves the menu (markup and its JS); the C ABI and
   the pilot stay in the tree, reachable from the console; git holds the pane.
6. **Follow the work** is the website's bottom menu — a footer nav on every site page — with
   five links, one home: `web/follow.json`. The engine's *Follow the work* pane carries the
   same five, injected at build, with a door to the section. About's *elsewhere* band dies.
7. **Text is its own page**, `/text/`, like the collection. The doctrine moves there from the
   about page; the about page gets a *Text* door beside the other two and a short statement
   slot (Jean's copy) where the doctrine stood. The engine's Text pane reads the text page.
8. **The world door says *Navigate the world*** and nothing under it. The WebGPU apology is
   the fallback card's job.
9. **Authors leaves.** The band, its injection, its builder.

Words are placeholders except where Jean gave them; the controls facts are the tree's.

## AUTHORITY

Base: master after DOORS_2 (report HEAD at U0; the engine has moved since — FRUSTUM_0,
FOURWALLS_0, FENCE_0 — none touch `web/` or the dist scripts). Lands on master. Counts 1
unless stated; boundaries are symbols (P2); DEFAULT-AND-FLAG (P17); LF only.

Files: `web/index.html`, `web/about/index.html`, `web/collection/index.html`,
`web/text/index.html` (new), `web/routes.json`, `web/follow.json` (new), `web/menu.css`,
`web/shared.css`, `tools/routes.py`, `tools/about_dist.py`, `tools/collection_dist.py`,
`tools/web_dist.py`, `docs/OPEN.md`. No `src/` file moves this round.

## UNITS

| unit | what | commit |
| --- | --- | --- |
| U0 | preflight; the controls facts | none |
| U1 | the veil, the noscript, the fallback | 1 |
| U2 | Follow the work: `follow.json`, the footers, the bands die | 1 |
| U3 | the text page; the about page's doors and statement | 1 |
| U4 | routes and panes: every item a pane; Controls; the photographer parked | 1 |
| U5 | OPEN.md | 1 |

---

## U0 — PREFLIGHT AND THE CONTROLS FACTS

```
git fetch origin master && git rev-parse HEAD
git ls-tree HEAD web/index.html web/about/index.html web/collection/index.html web/routes.json web/menu.css web/shared.css tools/routes.py tools/about_dist.py tools/collection_dist.py tools/web_dist.py docs/OPEN.md
ls web/text web/follow.json 2>&1         # both absent
```
The controls facts, from the tree (DOORS_2 U0 already found: R bound to nothing; no Esc key;
the pointer door is Numpad ✱; ribbon boarding is the leap on a summit). Read
`console.hpp`'s `on_touch_lift` commentary (the leap / aura / swap block) and
`toggle_fpv_mode`'s callers, and fill the six rows below for both platforms. A verb with no
mouth on a platform reads *not on a phone* / *not on a desktop* — never invented.

| verb | desktop (expected) | smartphone (expected) |
| --- | --- | --- |
| Directions | W A S D | drag the left half |
| Rotation | drag with the mouse held · wheel to zoom | drag the right half · two fingers to zoom |
| Pulse | Caps Lock | two fingers, one clean tap on the right half |
| Aura | 3 lights it · 2 raises it | a second finger on the left half |
| Leap | Space | one clean tap on the right half |
| FPV | Ctrl | (find the touch mouth, or *not on a phone*) |

Report the six rows as the tree says them; U4 uses them verbatim.

---

## U1 — THE VEIL, THE NOSCRIPT, THE FALLBACK (`web/index.html`)

### U1.1 — the stack

FIND
```
      <!-- DOORS_0 — THE SENTENCE. PLACEHOLDER COPY: Jean's words replace
           it. One line, first person (the convention about/ names). -->
      <p class="line">A living world drawn by your GPU, hung with the paintings.</p>
      <div id="status">Waking the world</div>
```
REPLACE
```
      <div id="status">Waking the world</div>
```

FIND
```
        <button id="enter" class="btn" type="button">Enter the world</button>
        <a class="btn alt" href="/about/">Visit the website</a>
      </div>
      <button id="logToggle" class="logToggle" type="button" aria-controls="log" aria-expanded="false">details</button>
    </div>
    <pre id="log" class="log" aria-live="off"></pre>
  </div>
```
REPLACE
```
        <button id="enter" class="btn" type="button">The Board</button>
        <a class="btn alt" href="/about/">About</a>
      </div>
    </div>
  </div>
```
(DOORS_3: the veil's *details* and its log pane leave; the card keeps its own. The labels are
Jean's — the two right names.)

FIND
```
    #veil .line { margin: 0; color: var(--dim); font-size: 15px; max-width: 30em; text-align: center; }
```
REPLACE with nothing.

By symbol — the script side of the veil's log: delete `wireToggle('logToggle',  'log');`
(count 1) and make every remaining read of `logEl` null-tolerant. The one known site:

FIND
```
      var pane = logEl2.classList.contains('open') ? logEl2
               : logEl.classList.contains('open')  ? logEl : null;
```
REPLACE
```
      var pane = logEl2.classList.contains('open') ? logEl2
               : (logEl && logEl.classList.contains('open')) ? logEl : null;   // DOORS_3 — the veil's pane is gone
```
Then `grep -n "logEl\b" web/index.html` — every remaining line must tolerate `null`
(`record()` and the toggle wiring are the suspects; the declaration `var logEl = …` may stay
and read `null`). The comment in the stack that names both labels as PLACEHOLDER COPY is
rewritten in one clause: the labels are Jean's.

### U1.2 — the noscript paragraph

FIND
```
      the_board is a living world drawn by your GPU, and it needs
      JavaScript to begin. The paintings it hangs are in
      <a style="color:#e8e6e0" href="/collection/">the collection</a>;
      <a style="color:#e8e6e0" href="/about/">about</a> says what this is.
```
REPLACE
```
      The board needs JavaScript to begin. Meanwhile
      <a style="color:#e8e6e0" href="/collection/">the collection</a> is right here,
      and <a style="color:#e8e6e0" href="/about/">About</a> says what this is.
```

### U1.3 — the fallback's words, and the card's door

FIND
```
      showCard(
        'This piece needs WebGPU, and this browser cannot give it one.',
        'The board is a living, ever-expanding world that hangs Jean ' +
        'Klein\'s paintings and moves to its own music. It runs today in ' +
        'Chrome and Edge on Android and desktop, and in Safari on iOS 26. ' +
        'The paintings themselves are in <a href="/collection/">the ' +
        'collection</a>, and <a href="/about/">about</a> says what this ' +
        'is. If you want to see it on the machine you have, ' + CONTACT + '.',
        false
      );
```
REPLACE
```
      // DOORS_3 — JEAN'S WORDS, verbatim. The technical reason is in the
      // log pane beside the card and in record(); the visitor gets the
      // welcome and the collection.
      showCard(
        'Welcome.',
        'The board is a living, ever-expanding world that hangs Jean ' +
        'Klein\'s paintings. Meanwhile <a href="/collection/">the ' +
        'collection</a> is right here.',
        false
      );
```
If `CONTACT` becomes unreferenced by this edit — it is still used by floor, lost and the
watchdog, so it stays.

FIND
```
      <a class="btn alt" href="/about/">Visit the website</a>
      <button id="logToggle2" class="logToggle" type="button" aria-controls="log2" aria-expanded="false">details</button>
```
REPLACE
```
      <a class="btn alt" href="/collection/">The collection</a>
      <button id="logToggle2" class="logToggle" type="button" aria-controls="log2" aria-expanded="false">details</button>
```

Witness:
```
grep -c 'id="logToggle"\|id="log"' web/index.html      # 0
grep -c "GPU" web/index.html | cat                      # report; none of the survivors may be visitor-facing text (comments and record() lines are fine)
grep -c "Welcome." web/index.html                        # 1
grep -c "__BUILD_ID__" web/index.html                    # 1
node -e "const t=require('fs').readFileSync('web/index.html','utf8');for(const s of t.match(/<script>([\s\S]*?)<\/script>/g)||[]){new Function(s.replace(/^<script>|<\/script>$/g,''))}console.log('parses')"
```

Commit: `DOORS_3 U1 — the veil says The Board and About, the sentence and details leave; the fallback welcomes and points to the collection; no GPU talk before the door`

---

## U2 — FOLLOW THE WORK

### U2.1 — new file `web/follow.json`
```json
[
  { "label": "Instagram", "href": "https://www.instagram.com/jeanklein1" },
  { "label": "TikTok",    "href": "https://www.tiktok.com/@kleinjean1" },
  { "label": "X",         "href": "https://www.x.com/kleinsouza" },
  { "label": "YouTube",   "href": "https://www.youtube.com/@jeanklein1" },
  { "label": "Spotify",   "href": "https://open.spotify.com/artist/6ujFyfbvYOAiH9FU977L5F" }
]
```
(TikTok's canonical profile path is `/@handle`; Jean wrote it without the `@`. If it does not
resolve, use his form and flag.)

### U2.2 — the renderer (`tools/routes.py`)

By symbol: beside `MENU_CSS = …` add `FOLLOW = os.path.join(WEB, "follow.json")`; after
`menu_css()` add:
```python
def follow_html(indent="    "):
    """Follow the work — the website's bottom menu, one home (web/follow.json).
    External links, so the collection gate exempts them (https:)."""
    with open(FOLLOW, encoding="utf-8") as fh:
        links = json.load(fh)
    items = []
    for l in links:
        if not l.get("label") or not str(l.get("href", "")).startswith("https://"):
            raise SystemExit("REFUSE  web/follow.json: every entry needs a label and an https href")
        items.append('<a href="%s" rel="me noopener" target="_blank">%s</a>' % (esc(l["href"]), esc(l["label"])))
    return ("\n" + indent).join(items)
```
Add `#   follow  web/follow.json — the five external links, rendered by follow_html()` to the
header comment.

### U2.3 — the site footers, one marker each

`web/about/index.html` FIND
```
<footer>
  <span>the ever expanding board</span>
  <span>set in Newsreader · no trackers</span>
</footer>
```
REPLACE
```
<footer>
  <!-- DOORS_3 — FOLLOW THE WORK: the website's bottom menu, from
       web/follow.json through tools/routes.py. -->
  <nav class="follow" id="follow" aria-label="Follow the work">
    <span class="band-label">Follow the work</span>
    <!-- __FOLLOW__ -->
  </nav>
  <span>the ever expanding board</span>
  <span>set in Newsreader · no trackers</span>
</footer>
```

`web/collection/index.html` FIND
```
<footer>
  <a href="../">the ever expanding board</a>
</footer>
```
REPLACE
```
<footer>
  <nav class="follow" id="follow" aria-label="Follow the work">
    <span class="band-label">Follow the work</span>
    <!-- __FOLLOW__ -->
  </nav>
  <a href="../">the ever expanding board</a>
</footer>
```
(The text page, U3, ships with the same footer from birth.)

`web/shared.css` — append:
```

/* ── DOORS_3 — Follow the work: the footer's own menu ──────────────── */
footer .follow { display: flex; flex-wrap: wrap; gap: var(--s2) var(--s3); align-items: baseline; width: 100%; margin-bottom: var(--s3); }
footer .follow .band-label { margin: 0; }
footer .follow a { text-decoration: none; color: var(--dim); }
footer .follow a:hover, footer .follow a:focus-visible { color: var(--ink); }
```
(If `footer` is a flex row in shared.css, the nav needs `flex-basis: 100%` — read the footer
rule and match it; flag.)

### U2.4 — the injections

`tools/about_dist.py`: add `"FOLLOW": routes.follow_html(indent="    "),` to the `fill(template, {…})`
dict; add `"__FOLLOW__"` to `fill()`'s refusal tuple. `tools/collection_dist.py`: in `fill()`,
`out = out.replace("<!-- __FOLLOW__ -->", routes.follow_html(indent="    "))` beside the
routes line, and `"__FOLLOW__"` in its refusal tuple. `tools/web_dist.py`: beside the two
menu markers, a third — `"<!-- __FOLLOW__ -->"` joins the refusal loop (exactly once in the
shell) and `shell_out = shell_out.replace("<!-- __FOLLOW__ -->", routes.follow_html(indent="        "))`
joins the two replacements. (The shell's marker lands in U4's Follow pane; land U4 before
building, or this refusal fires — expected, and the reason U2 and U4 are one deploy.)

### U2.5 — the bands die (`web/about/index.html`, `tools/about_dist.py`)

By symbol: delete `<section class="band" id="authors">` through its `</section>` and
`<section class="band" id="links">` through its `</section>`. In `about_dist.py`: delete
`build_authors` and `build_links` whole, their two dict entries, and `"__AUTHORS__", "__LINKS__"`
from the refusal tuple. `site.json` keeps its `authors` and `links` keys unread; say so in the
report for Jean to prune (the file is his).

Witness:
```
python3 -c "import sys;sys.path.insert(0,'tools');import routes;print(routes.follow_html())"   # five anchors, all https
grep -c "<!-- __FOLLOW__ -->" web/about/index.html web/collection/index.html   # 1 each
grep -c 'id="authors"\|id="links"\|__AUTHORS__\|__LINKS__' web/about/index.html tools/about_dist.py   # 0 each
python3 tools/collection_dist.py && python3 tools/gates/collection_gate.py     # PASS (https: refs are exempt)
```

Commit: `DOORS_3 U2 — Follow the work: web/follow.json, one renderer, a footer nav on every site page; authors and elsewhere leave`

---

## U3 — THE TEXT PAGE; THE ABOUT PAGE'S DOORS AND STATEMENT

### U3.1 — new template `web/text/index.html`

Model it on `web/collection/index.html`'s head and chrome (the `<head>` with
`<link rel="stylesheet" href="../shared.css">`, the `<style>` block ending in
`/* __MENU_CSS__ */`, the `.top` masthead with the sandwich and `<!-- __ROUTES__ -->`, the
footer from U2.3) and one body section:
```
<article class="text" id="text">
  <!-- PLACEHOLDER COPY — Jean's words replace all of this. The doctrine
       moved here from about/ at DOORS_3; this page is its one home, and
       about_dist writes text.json from it for the engine's Text pane. -->
  <h1>The pawn is a vessel for projected intent.</h1>
  <p>It stands alone, or it stands beside someone. Whether it is friend or
     antagonist, whether you are reading it or it is reading you — that is
     not settled here, and it will not be.</p>
</article>
```
Styles in its `<style>`: `.text { max-width: 34em; margin: var(--s5) auto; padding: 0 var(--edge); }`,
`.text h1 { font-size: clamp(27px, 3.4vw, 46px); font-weight: 430; }`, `.text p { font-size: 18px; line-height: 1.55; }`
— shared.css's rhythm variables, no new clamp for vertical spacing (shared.css's stated law).
`<title>text — the ever expanding board</title>`. No JavaScript.

### U3.2 — `about_dist.py` builds it

By symbol: after the about page is written, before the fonts copy:
```python
    # DOORS_3 — THE TEXT PAGE, built here because this script already
    # owns the site's chrome (routes, menu css, follow, fonts). Its one
    # home is web/text/index.html; text.json is the engine's read of it.
    text_tpl = os.path.join(WEB, "text", "index.html")
    with open(text_tpl, encoding="utf-8") as fh:
        text_page = fill(fh.read(), {
            "ROUTES": routes.nav_html("site", "/text/", indent="      "),
            "MENU_CSS": routes.menu_css(),
            "FOLLOW": routes.follow_html(indent="    "),
        })
    text_dist = os.path.join(DIST_ROOT, "text")
    os.makedirs(text_dist, exist_ok=True)
    with open(os.path.join(text_dist, "index.html"), "w", encoding="utf-8") as fh:
        fh.write(text_page)
    m = re.search(r'<article class="text" id="text">(.*?)</article>', text_page, re.S)
    if not m:
        say("REFUSE  the text page has no <article class=\"text\"> — the engine's Text pane would have nothing to show")
        sys.exit(1)
    with open(os.path.join(text_dist, "text.json"), "w", encoding="utf-8") as fh:
        json.dump({"html": m.group(1).strip()}, fh)
    say("dist/text/index.html written; text.json beside it")
```
`fill()` must tolerate a template that lacks some markers: its refusal tuple checks only what
remains in the OUTPUT, so a template without `__HERO__` passes — verify by reading `fill()`;
if it refuses on absent markers, make the check per-template (only markers the template
contained). `WEB` — if `about_dist.py` has no such constant, derive it beside `HERE`.

The about.json emission changes: the doctrine leaves the about page, so the `re.search` for
`<header class="doctrine" id="text">` is replaced by the statement's, and the hero joins:

FIND
```
    m = re.search(r'<header class="doctrine" id="text">(.*?)</header>', page, re.S)
    if not m:
        say("REFUSE  the doctrine header is not in the page — the engine's Text pane would have nothing to show")
        sys.exit(1)
    with open(os.path.join(DIST, "about.json"), "w", encoding="utf-8") as fh:
        json.dump({"html": m.group(1).strip(), "email": site["email"]}, fh)
```
REPLACE
```
    # DOORS_3 — the about page in miniature: the statement, today's hero
    # (the same day-indexed pick the page makes), and the address.
    m = re.search(r'<header class="statement" id="statement">(.*?)</header>', page, re.S)
    if not m:
        say("REFUSE  the statement header is not in the page — the engine's About pane would have nothing to show")
        sys.exit(1)
    with open(os.path.join(DIST, "about.json"), "w", encoding="utf-8") as fh:
        json.dump({"statement": m.group(1).strip(),
                   "hero": hero_data,          # what build_hero returned: the day-indexed list the page itself rotates
                   "email": site["email"]}, fh)
```
Read `build_hero` to confirm `hero_data` is JSON-serialisable and carries src + title per entry
(it is what the template's `/* __HERO_DATA__ */` receives, so it is). The engine pane picks
the same day index the page does: `Math.floor(Date.now()/864e5)` — copy the page's own
expression, and its tall/wide choice by the pane's own aspect (`max-aspect-ratio: 4/5` →
tall).

### U3.3 — the about page

FIND
```
<!-- PLACEHOLDER COPY — Jean's words replace all of this.
     Convention: the front door speaks in the FIRST PERSON. What is here
     now is third-person curatorial language, which reads like wall text
     rather than the artist talking. It holds the shape and the length;
     it is not the voice. -->
<header class="doctrine" id="text">
  <h1>The pawn is a vessel for projected intent.</h1>
  <p>It stands alone, or it stands beside someone. Whether it is friend or
     antagonist, whether you are reading it or it is reading you — that is
     not settled here, and it will not be.</p>
</header>
<div class="doors">
  <!-- PLACEHOLDER: door blurbs, ~20 words each -->
  <a class="door" href="../">
    <h2><span class="go">the world</span></h2>
    <p>A procedurally generated place built from the paintings' logic —
       terrain, ribbon, music, and the pawn, running live.</p>
    <small>Runs on WebGPU: a recent Chrome or Edge. Everyone else,
       the collection is the door.</small>
  </a>
  <a class="door" href="../collection/">
    <h2><span class="go">the collection</span></h2>
    <p>Oils, watercolours and pen, in sets. Every work, one page.</p>
    <div class="strip">
      <!-- __STRIP__ -->
    </div>
  </a>
</div>
```
REPLACE
```
<!-- PLACEHOLDER COPY — Jean's statement. The doctrine moved to /text/ at
     DOORS_3; what stands here says what this is, in his voice. -->
<header class="statement" id="statement">
  <p>Paintings, and a world that hangs them.</p>
</header>
<div class="doors">
  <a class="door" href="../">
    <h2><span class="go">Navigate the world</span></h2>
  </a>
  <a class="door" href="../collection/">
    <h2><span class="go">the collection</span></h2>
    <p>Oils, watercolours and pen, in sets. Every work, one page.</p>
    <div class="strip">
      <!-- __STRIP__ -->
    </div>
  </a>
  <a class="door" href="../text/">
    <h2><span class="go">text</span></h2>
    <!-- PLACEHOLDER: one line -->
    <p>The pawn is a vessel for projected intent.</p>
  </a>
</div>
```
The `.doors` grid was two-up; read its rule and make three fit (a third column, or the text
door full-width beneath — the executor's call, flagged). Rename the CSS for `.doctrine` to
`.statement` if the rule exists (count and report). The `id="text"` anchor is gone from this
page: the `text` route now points at `/text/` (U4).

### U3.4 — `web/_redirects` — nothing. `dist/text/` is a new directory web_dist ships as it
ships `about/` (verify `web_dist.py` copies `dist/about` untouched — it "deletes only the
engine's own names" — so `dist/text` survives its rmtree of engine names; if web_dist's rmtree
is of the whole dist before copying, `about_dist` runs before it in `dist.py` and would be
deleted: READ the rmtree and report; DOORS_0 U7's cascade established about/ survives, so text/
does too by the same rule).

Witness:
```
python3 tools/about_dist.py --preview /tmp/about.html      # if assets/about is present
ls dist/text/index.html dist/text/text.json                # after a full about_dist run
grep -c '<a class="door" href="../text/">' web/about/index.html   # 1
grep -c "Navigate the world" web/about/index.html           # 1
grep -c "WebGPU" web/about/index.html                        # 0
```

Commit: `DOORS_3 U3 — text is its own page (about_dist builds it and text.json); the about page gets a statement, a text door, and Navigate the world with nothing under it`

---

## U4 — ROUTES AND PANES: EVERY ITEM A PANE

### U4.1 — `web/routes.json`, rewritten whole
```json
[
  { "id": "board",      "label": "The Board",       "engine": "pane" },
  { "id": "controls",   "label": "Controls",        "engine": "pane" },
  { "id": "collection", "label": "The collection",  "href": "/collection/",   "engine": "pane" },
  { "id": "text",       "label": "Text",            "href": "/text/",         "engine": "pane" },
  { "id": "about",      "label": "About",           "href": "/about/",        "engine": "pane" },
  { "id": "write",      "label": "Write me",        "href": "/about/#write",  "engine": "pane" },
  { "id": "follow",     "label": "Follow the work", "href": "/about/#follow", "engine": "pane" },
  { "id": "world",      "label": "The world",       "href": "/",  "side": "site", "return": true }
]
```
(The site pages render the collection, text, about, write, follow and world links, dropping
the page they are on.)

### U4.2 — the panes (`web/index.html`)

By symbol: the Board pane (`<div class="pane" data-pane="board" hidden>` through its closing
`</div>`) is replaced whole; the Text pane's door-out href changes; two panes are added after
the Write pane, before `</details>`.

The Board, replaced:
```
    <div class="pane" data-pane="board" hidden>
      <button type="button" class="back" data-back>← menu</button>
      <p class="pane-head">The Board</p>
      <!-- PLACEHOLDER COPY — Jean's messages to the audience, and hints.
           Paragraphs; no controls here (Controls is its own pane). -->
      <p class="pane-note">Welcome. Take your time; the world keeps going whether you move or not.</p>
      <p class="pane-note">Stand by a wall of pictures and wait for the next one to be hung. Find an arch and walk through it.</p>
    </div>
    <div class="pane" data-pane="controls" hidden>
      <button type="button" class="back" data-back>← menu</button>
      <p class="pane-head">Controls</p>
      <!-- TWO PLATFORMS, SIDE BY SIDE, ONE SHOWN. The facts in both lists
           are the tree's (DOORS_3 U0); the words are placeholders. -->
      <div class="seg" role="tablist" aria-label="platform">
        <button type="button" role="tab" data-platform="phone">Smartphone</button>
        <button type="button" role="tab" data-platform="desktop">Desktop</button>
      </div>
      <dl class="ctl" data-platform="phone" hidden>
        <dt>Directions</dt><dd>drag the left half of the screen</dd>
        <dt>Rotation</dt><dd>drag the right half · two fingers to zoom</dd>
        <dt>Pulse</dt><dd>two fingers, one clean tap on the right half</dd>
        <dt>Aura</dt><dd>a second finger on the left half</dd>
        <dt>Leap</dt><dd>one clean tap on the right half</dd>
        <dt>FPV</dt><dd>U0's answer, or: not on a phone</dd>
      </dl>
      <dl class="ctl" data-platform="desktop" hidden>
        <dt>Directions</dt><dd>W A S D</dd>
        <dt>Rotation</dt><dd>drag with the mouse held · the wheel to zoom</dd>
        <dt>Pulse</dt><dd>Caps Lock</dd>
        <dt>Aura</dt><dd>3 lights it · 2 raises it</dd>
        <dt>Leap</dt><dd>Space</dd>
        <dt>FPV</dt><dd>Ctrl</dd>
      </dl>
      <div class="row">
        <button type="button" id="ctlPhoto">Take a photo</button>
        <button type="button" id="ctlFull">Fullscreen</button>
        <button type="button" id="ctlSound">Sound: on</button>
      </div>
    </div>
```
(The roll — `#roll`, `#rollState`, "Its own photographs" — is gone with the old Board pane.)

The Text pane's door: FIND
```
      <a class="out" data-out target="_blank" rel="opener" href="about/#text">Read it on the website →</a>
```
REPLACE
```
      <a class="out" data-out target="_blank" rel="opener" href="text/">Read it on the website →</a>
```

Two panes, inserted after the Write pane's closing `</div>`:
```
    <div class="pane" data-pane="about" hidden>
      <button type="button" class="back" data-back>← menu</button>
      <p class="pane-head">About</p>
      <!-- the day's painting, as the about page picks it, and the statement -->
      <a class="mini-hero" id="miniHero" target="_blank" rel="opener" href="collection/"></a>
      <div class="text" id="miniStatement"></div>
      <a class="out" data-out target="_blank" rel="opener" href="about/">About, on the website →</a>
    </div>
    <div class="pane" data-pane="follow" hidden>
      <button type="button" class="back" data-back>← menu</button>
      <p class="pane-head">Follow the work</p>
      <nav class="follow-list" aria-label="Follow the work">
        <!-- __FOLLOW__ -->
      </nav>
      <a class="out" data-out target="_blank" rel="opener" href="about/#follow">On the website →</a>
    </div>
```

### U4.3 — the behaviours (`web/index.html`)

By symbol, in the sandwich's IIFE:

1. **The roll leaves.** Delete the VISIT_0 roll block (`var rollEl …` through `stopRoll`),
   the `if (id === 'board') startRoll(); else stopRoll();` line in `showPane`, and the
   `button[data-visit]` branch in the click handler. `abi()` keeps `roll`/`visit`/`visiting`
   wrappers only if something still reads them — nothing will; delete the three lines and
   the comment that names them. `grep -c "gallery_roll\|startRoll\|rollEl" web/index.html` → 0.
2. **`showPane`** gains: `if (id === 'about') loadAbout();` (the `text` case now loads the
   text page's json — see 3) and `if (id === 'controls') showPlatform(platform);`.
3. **Two fetches, two homes.** `loadAbout()` now fetches `about/about.json` and fills
   `#miniStatement` with `d.statement` and `#miniHero` with the day's image:
   ```js
   var day = Math.floor(Date.now() / 864e5);              // the about page's own expression
   var tall = window.matchMedia && window.matchMedia('(max-aspect-ratio: 4/5)').matches;
   var list = (d.hero && (tall ? d.hero.tall : d.hero.wide)) || (d.hero && d.hero.wide) || [];
   var pick = list.length ? list[day % list.length] : null;
   if (pick && miniHero) { miniHero.innerHTML = ''; var img = document.createElement('img'); img.src = 'about/' + pick.src; img.alt = pick.title || ''; img.loading = 'lazy'; img.decoding = 'async'; miniHero.appendChild(img); }
   ```
   — the shape of `pick.src`/`pick.title` and the tall/wide keys are whatever `build_hero`
   emits (U3.2 read it); match its names, flag. `loadText()` is new and fetches
   `text/text.json` into `#miniText`; the `write` case still loads about (for the email).
4. **The platform toggle.**
   ```js
   var CONTROLS_DEFAULT = null;   // 'phone' | 'desktop' | null = the visitor's own device (Jean may pin it)
   var platform = CONTROLS_DEFAULT || ((window.matchMedia && window.matchMedia('(pointer: coarse)').matches) ? 'phone' : 'desktop');
   var seg = menu.querySelectorAll('.seg [data-platform]');
   var ctls = menu.querySelectorAll('.ctl[data-platform]');
   function showPlatform(p) {
     platform = p;
     seg.forEach(function (b) { b.setAttribute('aria-selected', b.dataset.platform === p ? 'true' : 'false'); });
     ctls.forEach(function (l) { l.hidden = (l.dataset.platform !== p); });
   }
   ```
   and in the click handler: `var pb = t.closest('.seg [data-platform]'); if (pb) { showPlatform(pb.dataset.platform); return; }`.
5. The Follow pane needs no script (its links are injected at build).

### U4.4 — the rules (`web/menu.css`)

Append:
```

/* ── DOORS_3 — Controls' two platforms; the About pane's hero; Follow ── */
.pane .seg { display: flex; gap: 6px; margin: 0 0 10px; }
.pane .seg button { font: inherit; color: var(--dim); background: none; border: 1px solid #33323c; border-radius: 2px; padding: 6px 12px; cursor: pointer; }
.pane .seg button[aria-selected="true"] { color: var(--ink); border-color: var(--dim); }
.pane .mini-hero { display: block; margin: 0 0 10px; background: var(--mat, #16120d); }
.pane .mini-hero img { display: block; width: 100%; height: auto; max-height: 40vh; object-fit: cover; }
.pane .follow-list a { display: block; padding: 6px 0; color: var(--dim); text-decoration: none; }
.pane .follow-list a:hover, .pane .follow-list a:focus-visible { color: var(--ink); }
```
And delete the `.roll*` rules (DOORS_1/VISIT_0's) — nothing renders them now.

Witness:
```
grep -c 'data-pane="' web/index.html                      # 7
python3 tools/routes.py | grep -c 'data-pane="'            # 7 (engine)
grep -c "<!-- __FOLLOW__ -->" web/index.html               # 1
grep -c "gallery_roll\|startRoll\|rollEl\|data-visit" web/index.html   # 0
grep -c "text/text.json\|about/about.json" web/index.html  # 2
grep -c "__BUILD_ID__" web/index.html                      # 1
node -e "const t=require('fs').readFileSync('web/index.html','utf8');for(const s of t.match(/<script>([\s\S]*?)<\/script>/g)||[]){new Function(s.replace(/^<script>|<\/script>$/g,''))}console.log('parses')"
python3 - <<'EOF'
import io,sys; sys.path.insert(0,'tools'); import web_dist
t=io.open('web/index.html',encoding='utf-8',newline='').read(); assert '\r' not in t
print('boot-set violations:', web_dist.boot_set_violations(t) or 'none')
EOF
```

Commit: `DOORS_3 U4 — every route a pane: The Board is Jean's, Controls has two platforms, About and Follow the work arrive, Text reads its own page; the photographer is parked`

---

## U5 — THE REGISTER (`docs/OPEN.md`)

Close DOORS_3 with the nine rulings; register: the photographer parked (VISIT_0's shell half
removed at U4; the C ABI, the provenance and the pilot stay; the roll pane is in git at the
DOORS_2 commit); `site.json`'s `authors` and `links` unread (Jean prunes); the controls FPV
row for phones as U0 found it; the `.doors` three-up layout as the executor made it (a visual
row for Jean); CLAUDE.md's gate table row for the collection gate, still owed. Then the full
gate set: `score`, `shell_gate`, `console_gate`, the three ledger `--check`s (nothing in `src/`
moved; all green or flag).

Commit: `DOORS_3 U5 — OPEN.md: the layout round closed; the photographer parked with its exits named`

---

## JEAN'S GATE, AFTER `python tools\dist.py` AND THE DEPLOY

- The veil: wordmark, *Waking the world*, two buttons — *The Board*, *About*. No sentence, no
  *details*.
- Disable WebGPU (chrome://flags or a browser without it): *Welcome.* and Jean's two sentences;
  *the collection* is a link; the button says *The collection*.
- The menu: seven items, each a pane. Controls opens on your own device's list; the other
  is one tap away. Take a photo, Fullscreen and Sound live there now.
- About shows today's painting (the same one the about page shows today) and the statement.
- Follow the work: five links in the pane; five in every site page's footer.
- /text/: the doctrine on its own page, with the menu and the footer; the about page's third
  door leads there; *Navigate the world* has nothing under it; *authors* and *elsewhere* are gone.

## WHAT THIS DOCUMENT CANNOT SEE (P16)

`assets/about/site.json` and `build_hero`'s output shape (the About pane's hero and the
tall/wide keys are matched by the executor to what the page already receives). Whether the
phone has an FPV mouth (U0). Whether `fill()` refuses a template that lacks a marker, and
whether `web_dist.py`'s rmtree spares `dist/text/` as it spares `dist/about/` (U3.4). The
`.doors` grid rule and the footer's flex rule, both unread; the executor reads them and flags
its layout choice for Jean's eye.
