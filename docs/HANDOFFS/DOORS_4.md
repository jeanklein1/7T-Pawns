# DOORS_4 — MOBILE AND DESKTOP, WRITINGS, GALLERY, AND THE COPY MAP

*Handoff · authored 2026-09-06 · Claude → CC · Jean holds build, visual, merge, deploy.*
*Filed to `docs/HANDOFFS/DOORS_4.md` while open. Lands on master after DOORS_3.*

## THE TOGGLE'S REAL BUG, FOUND

Jean: clicking Smartphone/Desktop yields nothing, and both lists show. The landed JS is
correct — the fault is one CSS rule. `web/menu.css:61` sets `.pane dl { display: grid; … }`,
and an author-origin `display` **defeats the user-agent's `[hidden] { display: none }`**. So
both `<dl hidden>` lists render always, and the toggle's flipping of `hidden` changes
nothing visible. The fix is one rule that restores `hidden`'s meaning inside panes, for every
element, forever. The lesson is registered (U7).

## THE RULINGS

1. **Mobile, not Smartphone**, and the pane shows exactly one platform's list at a time —
   which the hidden fix makes true. `data-platform="phone"` stays (wiring, not words).
2. **The Board's second note becomes "Lose yourself."**
3. **The social links live in one place: the engine's sandwich.** The three site footers lose
   the Follow nav; the engine's Follow pane loses its door-out; the routes for follow and
   write become engine-only.
4. **Text becomes Writings — plural, browsable, one home.** The texts are files:
   `assets/writings/NN_slug.txt` (first line the title, then the body; blank lines break
   stanzas; line breaks are kept — they are poems). `about_dist` builds `/writings/` and
   `writings.json` from the same files, so the engine's pane and the page can never disagree,
   and a new text is one new file plus a deploy. The engine pane is a browser: a list of
   titles; choose one and read it there, world breathing, music on; back to the list.
   `/text/` retires with a 301.
5. **The collection is called Gallery** everywhere a menu speaks (the id `collection` stays —
   wiring). The engine pane keeps only the images, its title, and the link below; the tiles
   are no longer links; the sets line dies, in the pane and in `peek.json`.
6. **The world door gets its picture.** Jean drops the file at **`assets\about\world.jpg`**
   (or `.png`); `about_dist` bakes it beside the hero and the door shows it. Absent file =
   a warning line and a door without a picture, never a refused build.
7. **Write me is a small box in the sandwich** on the site: a nested `<details>` inside the
   menu, the form inside it, posting to the same function — no page band, no `#write`
   anchor. The about page's write band and its script die. The engine keeps its Write pane.
8. **The name appears once.** The `.top` site line dies on all site pages; the about footer
   keeps only *set in Newsreader · no trackers*; the collection footer dies; the writings
   page ships footerless. The engine's wordmark is the name's home.
9. **The fallback speaks Jean's words**, verbatim, and its button reads **Click here** and
   goes to `/about/`.
10. **`docs/COPY.md`** — the copy map: every visitor-facing string and the one file that owns
    it. Written this round, kept current by every future round that moves words.

## AUTHORITY

Base: master (report HEAD at U0; MASSIF_0 is the tip — engine-only, no overlap). Lands on
master. Counts 1 unless stated; boundaries are symbols (P2); DEFAULT-AND-FLAG (P17); LF only.

Files: `web/index.html`, `web/menu.css`, `web/routes.json`, `web/about/index.html`,
`web/collection/index.html`, `web/text/` (dies), `web/writings/index.html` (new),
`web/_redirects`, `web/shared.css`, `assets/writings/` (new, two seeds), `tools/routes.py`,
`tools/about_dist.py`, `tools/collection_dist.py`, `docs/COPY.md` (new), `docs/OPEN.md`.
No `src/` file moves.

## UNITS

| unit | what | commit |
| --- | --- | --- |
| U0 | preflight | none |
| U1 | the hidden fix; Mobile; Lose yourself | 1 |
| U2 | Follow the work: engine-only | 1 |
| U3 | Writings | 1 |
| U4 | Gallery | 1 |
| U5 | the about page: world picture, write box, the quieter name | 1 |
| U6 | the fallback's words and button | 1 |
| U7 | COPY.md; OPEN.md; gates | 1 |

---

## U0 — PREFLIGHT
```
git fetch origin master && git rev-parse HEAD
git ls-tree HEAD web/index.html web/menu.css web/routes.json web/about/index.html web/collection/index.html web/text/index.html web/_redirects web/shared.css tools/routes.py tools/about_dist.py tools/collection_dist.py docs/OPEN.md
ls assets/writings web/writings docs/COPY.md 2>&1     # all absent
sed -n '150,175p' tools/about_dist.py                  # read fill()'s body: does it refuse on tokens absent from the TEMPLATE or only on tokens left in the OUTPUT? U3/U5 depend on the answer; adapt as the text-page block already did.
```

---

## U1 — THE HIDDEN FIX; MOBILE; LOSE YOURSELF

### U1.1 — `web/menu.css`, appended at the end of the file
```

/* ── DOORS_4 — HIDDEN MEANS HIDDEN. .pane dl sets display:grid, and an
   author display beats the UA's [hidden]{display:none} — which is why
   both platform lists showed and the toggle "did nothing". One rule,
   every pane element, forever. */
.pane [hidden] { display: none; }
```

### U1.2 — `web/index.html`

FIND
```
        <button type="button" role="tab" data-platform="phone">Smartphone</button>
```
REPLACE
```
        <button type="button" role="tab" data-platform="phone">Mobile</button>
```

FIND
```
      <p class="pane-note">Stand by a wall of pictures and wait for the next one to be hung. Find an arch and walk through it.</p>
```
REPLACE
```
      <p class="pane-note">Lose yourself.</p>
```

Witness:
```
grep -c "\.pane \[hidden\]" web/menu.css     # 1
grep -c ">Mobile<" web/index.html            # 1
grep -c "Lose yourself." web/index.html      # 1
```

Commit: `DOORS_4 U1 — hidden means hidden in a pane (the toggle's real bug was CSS); Mobile; Lose yourself`

---

## U2 — FOLLOW THE WORK: ENGINE-ONLY

### U2.1 — `web/routes.json`

FIND
```
  { "id": "write",      "label": "Write me",        "href": "/about/#write",  "engine": "pane" },
  { "id": "follow",     "label": "Follow the work", "href": "/about/#follow", "engine": "pane" },
```
REPLACE
```
  { "id": "write",      "label": "Write me",        "side": "engine", "engine": "pane" },
  { "id": "follow",     "label": "Follow the work", "side": "engine", "engine": "pane" },
```

### U2.2 — the engine's Follow pane loses its door-out (`web/index.html`)

By symbol: in the pane `data-pane="follow"`, delete the `<a class="out" …>On the website →</a>`
line (count 1 within that pane; the surrounding `<nav class="follow-list">` and its
`<!-- __FOLLOW__ -->` stay).

### U2.3 — the three site footers

`web/about/index.html` FIND
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
REPLACE
```
<footer>
  <span>set in Newsreader · no trackers</span>
</footer>
```

`web/collection/index.html`: by symbol, delete `<footer>` through `</footer>` whole (it holds
the Follow nav and the site-name link; both die by rulings 3 and 8).

`web/text/index.html` is deleted whole at U3; nothing to edit here.

### U2.4 — the injections follow

`tools/about_dist.py`: delete the `"FOLLOW": routes.follow_html(indent="    "),` dict entry;
remove `"__FOLLOW__"` from any refusal tuple that names it. `tools/collection_dist.py`:
delete the `out = out.replace("<!-- __FOLLOW__ -->", …)` line and `"__FOLLOW__"` from its
refusal tuple. `tools/web_dist.py` keeps its marker and replacement — the engine pane is the
one consumer left. `tools/routes.py` `follow_html` is untouched.

Witness:
```
grep -rc "__FOLLOW__" web/index.html                      # 1 (the engine pane)
grep -rc "__FOLLOW__" web/about/index.html web/collection/index.html tools/about_dist.py tools/collection_dist.py   # 0 each
python3 tools/routes.py                                    # site navs list Gallery/Writings/About/The world only after U3/U4 land; no follow, no write link
python3 tools/collection_dist.py && python3 tools/gates/collection_gate.py    # PASS
```

Commit: `DOORS_4 U2 — the social links live in one place: the engine's Follow pane; the site footers and the door-out leave`

---

## U3 — WRITINGS

### U3.1 — the seeds, `assets/writings/` (new; Jean's mockups verbatim)

`assets/writings/01_the_mirror_in_the_sand.txt`:
```
The Mirror in the Sand

Unmapped grid
Boundaries dead
Shifting sands
The board expands

Humble choices
To be made
A tea spoon of will
In an ocean of fate
Another die rolls
Through the vast landscape
The quiet wind blows
Empty canvas takes shape
```

`assets/writings/02_meaning.txt`:
```
Meaning

The nature of the game
The shape of the rules
The structure of the board
The intent of the player

But who designs
What is to be played with?
```
(First line the title; first blank line ends it; every later blank line breaks a stanza;
line breaks inside a stanza are kept. Filename order is page order.)

### U3.2 — the template: `git rm -r web/text`, new `web/writings/index.html`

Model the head and chrome on the deleted text template (same `<head>`, `<style>` ending in
`/* __MENU_CSS__ */`, the `.top` masthead with the sandwich and `<!-- __ROUTES__ -->` — but
**no** `.site` line, ruling 8, and **no** footer). Body:
```
<main class="writings">
  <!-- __WRITINGS__ -->
</main>
```
Styles, in the page's `<style>` (shared.css rhythm; no new vertical clamp):
```
.writings { max-width: 34em; margin: var(--s5) auto; padding: 0 var(--edge); }
.writings article { margin: 0 0 var(--s5); }
.writings h2 { font-size: clamp(22px, 2.6vw, 34px); font-weight: 430; margin: 0 0 var(--s2); }
.writings p { font-size: 18px; line-height: 1.55; margin: 0 0 var(--s2); white-space: pre-line; }
```
`<title>writings — the ever expanding board</title>`. No JavaScript.

### U3.3 — `tools/about_dist.py`: build_writings and the block swap

By symbol, near the other builders, add:
```python
def build_writings():
    """The writings, one home: assets/writings/NN_slug.txt. First line the
    title, then the body; blank lines break stanzas; line breaks inside a
    stanza are kept (white-space: pre-line on the page, <br> nowhere).
    Filename order is page order. Returns (page_html, json_list)."""
    src = os.path.join(ROOT, "assets", "writings")
    if not os.path.isdir(src):
        say("REFUSE  assets/writings is missing — the writings have one home and this is it")
        sys.exit(1)
    pieces = []
    for name in sorted(os.listdir(src)):
        if not name.endswith(".txt"):
            continue
        with open(os.path.join(src, name), encoding="utf-8") as fh:
            raw = fh.read().replace("\r\n", "\n").strip("\n")
        lines = raw.split("\n")
        title = lines[0].strip()
        body = "\n".join(lines[1:]).strip("\n")
        if not title or not body:
            say("REFUSE  %s needs a title line and a body" % name)
            sys.exit(1)
        stanzas = [s.strip("\n") for s in body.split("\n\n") if s.strip()]
        html = "\n".join("<p>%s</p>" % esc(s) for s in stanzas)
        slug = os.path.splitext(name)[0]
        pieces.append({"slug": slug, "title": title, "html": html})
    if not pieces:
        say("REFUSE  assets/writings holds no .txt — nothing to build")
        sys.exit(1)
    page = "\n".join('<article id="%s">\n<h2>%s</h2>\n%s\n</article>'
                     % (p["slug"], esc(p["title"]), p["html"]) for p in pieces)
    return page, [{"title": p["title"], "html": p["html"]} for p in pieces]
```
`esc` — if `about_dist.py` has no HTML escaper, take `tools/routes.py`'s (`from routes import esc`
is already importable; use it). `ROOT` — derive beside `HERE` if absent.

Then, by symbol, the text-page block (from `text_tpl = os.path.join(WEB, "text", "index.html")`
through its `say("dist/text/index.html written; text.json beside it")`) is replaced whole with:
```python
    # DOORS_4 — THE WRITINGS PAGE, plural and growing. One home for the
    # texts (assets/writings); this page and writings.json are built from
    # the same read, so the engine's pane and the site can never disagree,
    # and a new text is one new file plus a deploy.
    w_page_html, w_json = build_writings()
    w_tpl = os.path.join(WEB, "writings", "index.html")
    with open(w_tpl, encoding="utf-8") as fh:
        w_page = fill(fh.read(), {
            "ROUTES": routes.nav_html("site", "/writings/", indent="      "),
            "MENU_CSS": routes.menu_css(),
            "WRITINGS": w_page_html,
        })
    w_dist = os.path.join(DIST_ROOT, "writings")
    os.makedirs(w_dist, exist_ok=True)
    with open(os.path.join(w_dist, "index.html"), "w", encoding="utf-8") as fh:
        fh.write(w_page)
    with open(os.path.join(w_dist, "writings.json"), "w", encoding="utf-8") as fh:
        json.dump(w_json, fh)
    say("dist/writings/index.html written; writings.json beside it (%d piece(s))" % len(w_json))
```
Adapt `fill()`'s token handling exactly as the deleted text block had (U0 read it):
`__WRITINGS__` joins whatever check pattern the text block used for its tokens.

### U3.4 — `web/routes.json`

FIND
```
  { "id": "text",       "label": "Text",            "href": "/text/",         "engine": "pane" },
```
REPLACE
```
  { "id": "writings",   "label": "Writings",        "href": "/writings/",     "engine": "pane" },
```

### U3.5 — `web/_redirects`, appended
```

# DOORS_4 — /text/ retired into /writings/. 301, unlike the aliases
# above: this path is not moving again, and burning it in is the point.
/text     /writings/   301
/text/    /writings/   301
```

### U3.6 — the engine pane becomes a browser (`web/index.html`)

By symbol: the pane `data-pane="text"` is replaced whole with:
```
    <div class="pane" data-pane="writings" hidden>
      <button type="button" class="back" data-back>← menu</button>
      <p class="pane-head">Writings</p>
      <ol class="wlist" id="wList"></ol>
      <div class="wread" id="wRead" hidden>
        <button type="button" class="back" id="wBack">← writings</button>
        <h2 class="wtitle" id="wTitle"></h2>
        <div class="text" id="wBody"></div>
      </div>
      <a class="out" data-out target="_blank" rel="opener" href="writings/">Read on the website →</a>
    </div>
```
Then, in the script: `showPane`'s `if (id === 'text') loadText();` becomes
`if (id === 'writings') loadWritings();`. The `loadText` function (and its `text`/`textLoading`
vars and `miniText` if nothing else reads it — check; the About pane uses `miniStatement`,
not `miniText`) is replaced whole with:
```js
      // DOORS_4 — WRITINGS, BROWSED IN THE WORLD. A list of titles; choose
      // one and read it here, music on, world breathing behind the blur;
      // back to the list. writings.json is built from the same files as
      // the page, so a new text on the site is a new text here.
      var writings = null, writingsLoading = false;
      var wList = document.getElementById('wList');
      var wRead = document.getElementById('wRead');
      var wTitle = document.getElementById('wTitle');
      var wBody = document.getElementById('wBody');
      var wBack = document.getElementById('wBack');
      function showWriting(i) {
        var p = writings && writings[i];
        if (!p) return;
        wTitle.textContent = p.title;
        wBody.innerHTML = p.html;          // our own build's markup (about_dist), not visitor text
        wList.hidden = true; wRead.hidden = false;
      }
      function loadWritings() {
        if (wRead) { wRead.hidden = true; }
        if (wList) { wList.hidden = false; }
        if (writings || writingsLoading || !wList) return;
        writingsLoading = true;
        fetch('writings/writings.json', { cache: 'no-cache' }).then(function (r) {
          if (!r.ok) throw new Error('HTTP ' + r.status);
          return r.json();
        }).then(function (d) {
          writings = d;
          wList.innerHTML = '';
          d.forEach(function (p, i) {
            var li = document.createElement('li');
            var b = document.createElement('button');
            b.type = 'button'; b.dataset.writing = i; b.textContent = p.title;
            li.appendChild(b); wList.appendChild(li);
          });
        }).catch(function (err) {
          writingsLoading = false;
          wList.innerHTML = ''; 
          var li = document.createElement('li'); li.textContent = 'The writings did not answer.';
          wList.appendChild(li);
          note('[shell] writings: ' + err);
        });
      }
      if (wBack) wBack.addEventListener('click', function () { wRead.hidden = true; wList.hidden = false; });
```
And in the menu click handler, before the `data-pane` branch:
```js
        var wb = t.closest('button[data-writing]');                          // DOORS_4
        if (wb) { showWriting(+wb.dataset.writing); return; }
```

### U3.7 — the rules (`web/menu.css`), appended
```

/* ── DOORS_4 — Writings, browsed in the pane ──────────────────────── */
.pane .wlist { list-style: none; margin: 0 0 10px; padding: 0; }
.pane .wlist li + li { border-top: 1px solid #1f1e26; }
.pane .wlist button { display: block; width: 100%; text-align: left; font: inherit; color: var(--dim); background: none; border: 0; padding: 8px 0; cursor: pointer; }
.pane .wlist button:hover, .pane .wlist button:focus-visible { color: var(--ink); }
.pane .wtitle { font-size: 17px; font-weight: 430; margin: 6px 0 8px; color: var(--ink); }
.pane .wread .text p { white-space: pre-line; }
```

Witness:
```
ls web/text 2>&1                                    # gone
grep -c 'data-pane="writings"' web/index.html       # 1
grep -c "text/text.json\|loadText" web/index.html   # 0
grep -c "writings/writings.json" web/index.html     # 1
python3 tools/about_dist.py --preview /tmp/about.html   # if assets/about present; the writings build runs on the non-preview path — full run on Jean's machine
python3 - <<'EOF'
import sys, os; sys.path.insert(0, 'tools')
import about_dist as ad
page, js = ad.build_writings()
assert len(js) == 2 and js[0]["title"] == "The Mirror in the Sand" and js[1]["title"] == "Meaning"
assert "Unmapped grid\nBoundaries dead" in js[0]["html"], "line breaks must survive"
print("writings ok:", [p["title"] for p in js])
EOF
node -e "const t=require('fs').readFileSync('web/index.html','utf8');for(const s of t.match(/<script>([\s\S]*?)<\/script>/g)||[]){new Function(s.replace(/^<script>|<\/script>$/g,''))}console.log('parses')"
```

Commit: `DOORS_4 U3 — Writings: one home in assets/writings, the page and writings.json from one read, a browser in the pane; /text/ retires by 301`

---

## U4 — GALLERY

### U4.1 — `web/routes.json`

FIND
```
  { "id": "collection", "label": "The collection",  "href": "/collection/",   "engine": "pane" },
```
REPLACE
```
  { "id": "collection", "label": "Gallery",         "href": "/collection/",   "engine": "pane" },
```

### U4.2 — the pane (`web/index.html`)

FIND
```
      <p class="pane-head">The collection</p>
```
REPLACE
```
      <p class="pane-head">Gallery</p>
```
By symbol, in the same pane: the door-out's text becomes `Open Gallery in a new tab →`
(href unchanged).

FIND
```
            var a = document.createElement('a');
            a.href = w.href; a.target = '_blank'; a.rel = 'opener';   // DOORS_2 — the site tab can find its way back
            a.className = 'peek-work'; a.title = w.title;
```
REPLACE
```
            // DOORS_4 — the tiles are pictures, not doors: only the link
            // below opens the page.
            var a = document.createElement('span');
            a.className = 'peek-work';
```
By symbol: the four lines that build and append the `sets` paragraph (`var sets = …` through
`peekEl.appendChild(sets);` — the `.peek-sets` builder) are deleted; the `frag` append stays.

### U4.3 — the sets leave `peek.json` (`tools/collection_dist.py`)

By symbol: the `json.dump({"sets": …, "works": peek_pick(peek)}, …)` becomes
`json.dump({"works": peek_pick(peek)}, …)` — nothing reads `sets` any more (one fact, one
home, and this fact's one reader is gone).

### U4.4 — `web/menu.css`: delete the `.peek-sets` rule; `.peek-work` keeps its rules
(`span` inherits them — the selector is a class).

Witness:
```
grep -c ">Gallery<\|\"Gallery\"" web/index.html web/routes.json    # 1 each
grep -c "peek-sets" web/index.html web/menu.css tools/collection_dist.py   # 0 each
grep -c "Open Gallery in a new tab" web/index.html                 # 1
python3 tools/collection_dist.py && python3 -c "import json; d=json.load(open('dist/collection/peek.json')); assert list(d)==['works']; print('peek ok', len(d['works']))"
python3 tools/gates/collection_gate.py                              # PASS
```

Commit: `DOORS_4 U4 — Gallery: the label everywhere a menu speaks; inert tiles, no sets line, one link out`

---

## U5 — THE ABOUT PAGE: THE WORLD'S PICTURE, THE WRITE BOX, THE QUIETER NAME

### U5.1 — the world door's picture

`web/about/index.html` FIND
```
  <a class="door" href="../">
    <h2><span class="go">Navigate the world</span></h2>
  </a>
```
REPLACE
```
  <a class="door" href="../">
    <h2><span class="go">Navigate the world</span></h2>
    <!-- __WORLD__ -->
  </a>
```
Also, the third door's label follows the round's renames: FIND
```
    <h2><span class="go">text</span></h2>
```
REPLACE
```
    <h2><span class="go">writings</span></h2>
```
and its `href="../text/"` becomes `href="../writings/"` (count 1 each).

`tools/about_dist.py`, by symbol, after the hero build and before `fill(…)` is called, add:
```python
    # DOORS_4 — THE WORLD DOOR'S PICTURE. Jean drops the file at
    # assets/about/world.jpg (or .png); it is baked like a poster and the
    # door shows it. Absent = a warning and a door without a picture —
    # never a refused build (the door must not hold the site hostage).
    world_tag = ""
    for ext in ("jpg", "jpeg", "png"):
        world_src = os.path.join(ROOT, "assets", "about", "world." + ext)
        if os.path.isfile(world_src):
            im = Image.open(world_src).convert("RGB")
            if im.width > 1600:
                im = im.resize((1600, round(im.height * 1600 / im.width)), Image.LANCZOS)
            im.save(os.path.join(DIST, "world.jpg"), "JPEG", quality=78, optimize=True, progressive=True)
            world_tag = '<img class="door-img" src="world.jpg" alt="" loading="lazy" decoding="async">'
            break
    if not world_tag:
        say("  world door: assets/about/world.jpg not found — the door ships without its picture")
```
and `"WORLD": world_tag,` joins the `fill(…)` dict (token handling as the others). In the
preview branch, if `Image` is absent, `world_tag = ""` stands (the loop is inside the
non-preview path or guarded on `Image` — match how `build_strip` guards; flag the choice).

In the about page's `<style>`, by symbol, beside the `.door` rules:
```
.door .door-img { display: block; width: 100%; height: auto; margin-top: var(--s2); }
```

### U5.2 — the write band dies; the write box joins the site sandwich

`web/about/index.html`: by symbol, delete `<section class="band" id="write">` through
`</section>` whole. Then, in the page's script, delete the mform enhancement whole — the
block from `const form = document.getElementById("mform");` through the end of its submit
handler (read the block; it is self-contained around the `fetch(form.action…)`; count of
`mform` after the deletion: 0).

`tools/routes.py`: after `follow_html`, add:
```python
def write_box_html(base, indent="  "):
    """Write me, as a small box inside the site sandwich (DOORS_4): a
    nested <details>, the same fields the engine's pane sends, posting to
    the same function. The inline script is the courtesy layer — without
    it the native POST still lands, and the function answers in JSON.
    The email fallback stays site.json's, on the about page's own path."""
    action = rel("/api/message", base)
    lines = [
        '<details class="writebox">',
        '  <summary>Write me</summary>',
        '  <form method="post" action="%s">' % esc(action),
        '    <label>name<input name="name" autocomplete="name"></label>',
        '    <label>email, if you want an answer<input name="email" type="email" autocomplete="email"></label>',
        '    <label>message<textarea name="message" required rows="4"></textarea></label>',
        '    <label class="hp" aria-hidden="true">leave this empty<input name="website" tabindex="-1" autocomplete="off"></label>',
        '    <div class="row"><button type="submit">Send</button><span class="said" role="status"></span></div>',
        '  </form>',
        '  <script>(function () {',
        '    var d = document.currentScript.closest("details");',
        '    var f = d.querySelector("form"), s = d.querySelector(".said");',
        '    f.addEventListener("submit", function (e) {',
        '      e.preventDefault(); s.textContent = "Sending…";',
        '      var data = {}; ["name", "email", "message", "website"].forEach(function (k) {',
        '        var el = f.elements[k]; data[k] = el ? el.value : "";',
        '      });',
        '      fetch(f.action, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(data) })',
        '        .then(function (r) { if (!r.ok) throw new Error("HTTP " + r.status); s.textContent = "Sent. Thank you."; f.reset(); })',
        '        .catch(function () { s.textContent = "It did not go through — try again in a moment."; });',
        '    });',
        '  })();</script>',
        '</details>',
    ]
    return ("\n" + indent).join(lines)
```
and in `nav_html`, at the end of the site branch's loop (after the last route is appended,
before the `return`), by symbol:
```python
    if side == "site":
        items.append(write_box_html(base, indent=indent))
```

`web/menu.css`, appended:
```

/* ── DOORS_4 — the write box inside the site sandwich ─────────────── */
.menu .writebox { border-top: 1px solid #1f1e26; margin-top: 6px; padding: 4px 14px 10px; }
.menu .writebox > summary { list-style: none; cursor: pointer; color: var(--dim); padding: 7px 0; }
.menu .writebox > summary::-webkit-details-marker { display: none; }
.menu .writebox > summary:hover, .menu .writebox > summary:focus-visible, .menu .writebox[open] > summary { color: var(--ink); }
.menu .writebox label { display: block; margin: 0 0 8px; }
.menu .writebox input, .menu .writebox textarea { display: block; width: 100%; box-sizing: border-box; margin-top: 3px; font: inherit; color: var(--ink); background: none; border: 1px solid #33323c; border-radius: 2px; padding: 6px 8px; }
.menu .writebox .hp { position: absolute; left: -9999px; }
.menu .writebox .row button { font: inherit; color: var(--ink); background: none; border: 1px solid #33323c; border-radius: 2px; padding: 6px 12px; cursor: pointer; }
```
(The dropdown's `min-width` may be tight for a form; if it visibly is, widen the `.menu > nav`
`min-width` to `min(92vw, 20em)` and flag — a visual row for Jean either way.)

### U5.3 — the quieter name

`web/about/index.html` and `web/collection/index.html`: delete the line
`  <p class="site">the ever expanding board</p>` (count 1 each; the writings template never
had one). `web/shared.css`, by symbol, beside the `.top` rules:
```
.top .menu { margin-left: auto; }   /* DOORS_4 — the name left; the sandwich keeps the right edge */
```

Witness:
```
grep -c '<p class="site">' web/about/index.html web/collection/index.html   # 0 each
grep -c '__WORLD__' web/about/index.html          # 1
grep -c 'id="write"\|mform' web/about/index.html  # 0
python3 tools/routes.py | grep -c "writebox"       # 2 (both site bases)
python3 tools/collection_dist.py && python3 tools/gates/collection_gate.py  # PASS (the box's action ../api/message is a ../ ref — the site's, exempt)
```

Commit: `DOORS_4 U5 — the world door gets its picture (assets/about/world.jpg), Write me becomes a box in the site sandwich, the name appears once`

---

## U6 — THE FALLBACK'S WORDS AND BUTTON (`web/index.html`)

By symbol: in `fallback(reason)`'s `showCard(…)` call, the second argument becomes (Jean's
words, verbatim; no links in the text — the button below is the door):
```
        'The board is a living, ever-expanding world that hangs Jean ' +
        'Klein\'s paintings. We couldn\'t run it on your browser yet. ' +
        'Meanwhile here is our web page.',
```
(`'Welcome.'` stays as the first argument.)

FIND
```
      <a class="btn alt" href="/collection/">The collection</a>
```
REPLACE
```
      <a class="btn alt" href="/about/" aria-label="Our web page">Click here</a>
```

Witness: `grep -c "We couldn" web/index.html` → 1; `grep -c ">Click here<" web/index.html` → 1;
the node parse line from U3.

Commit: `DOORS_4 U6 — the fallback speaks Jean's words; the card's button reads Click here and opens the web page`

---

## U7 — COPY.MD, OPEN.MD, GATES

### U7.1 — new file `docs/COPY.md`
A pointer map, not a copy — every visitor-facing string and the ONE file that owns it, in
this shape (fill the anchors from the tree as landed; keep it to facts):

```markdown
# COPY — where every visitor-facing word lives

One rule: edit the OWNING file, then `python tools/dist.py` and deploy.
Nothing here is a second home; these are pointers.

## The engine page (everexpandingboard.com) — web/index.html
- Veil: wordmark, "Waking the world", the two buttons (The Board / About) — the .stack markup.
- Fallback card: "Welcome." + Jean's sentences + "Click here" — fallback() and the card markup.
- Floor / lost / watchdog cards: their showCard() calls.
- The noscript paragraph.
- The sandwich panes: The Board (notes), Controls (both platform lists; facts from
  input.hpp/console.hpp — change words, not facts), Gallery / Writings / About / Write me /
  Follow the work pane heads, notes, buttons, door-out labels.
- Status words (ENTRY_WORDS, "Still waking…", "Not in this build…", send/fail lines) — the
  shell script's string literals.

## Menus on every page — web/routes.json (labels), tools/routes.py (the write box's words)
## The social links — web/follow.json
## The writings — assets/writings/NN_slug.txt (first line the title; filename order is page order)
## The about page — web/about/index.html (statement, door blurbs, form-free)
  - the world door's picture: drop the file at assets/about/world.jpg
- The day's hero + site email — assets/about/site.json
## The gallery page — web/collection/index.html (the lede h1 + line; set labels come from
  assets/collection/*/set.json)
## The writings page chrome — web/writings/index.html (title tag only; the texts come from assets/writings)
```

### U7.2 — `docs/OPEN.md`
Close **DOORS_4** with the ten rulings, one line each; register: the hidden-vs-author-display
lesson (one line, so the next pane element doesn't relearn it); `/text/` retired by 301;
`peek.json` lost its `sets` key (the one reader left); the write box's dropdown width as a
visual row; `docs/COPY.md` exists and every future round that moves words updates it; the
collection page's lede h1 is still placeholder copy (COPY.md points at it).

### U7.3 — gates
```
python3 tools/gates/score/run.py && python3 tools/gates/shell_gate/run.py && python3 tools/gates/console_gate/run.py
python3 tools/command_census.py --check && python3 tools/mirror_census.py --check && python3 tools/binding_ledger.py --check
```
All green (no `src/` file moved; if a ledger reds, run the cascade and flag).

Commit: `DOORS_4 U7 — COPY.md is the copy map; the register written`

---

## JEAN'S GATE, AFTER `python tools\dist.py` AND THE DEPLOY

- Controls: opens on your device's list alone; **Mobile | Desktop** swap the whole window's
  contents, one at a time.
- The Board: your welcome line, then *Lose yourself.*
- Follow the work: five links in the engine's pane, nothing under them; no social links
  anywhere on the site pages.
- Writings: on the site, both poems on one page at /writings/ (old /text/ forwards); in the
  engine, a list of the two titles — choose one and read it over the breathing world.
- Gallery: the pane is pictures + the one link; tiles don't navigate; site menus say Gallery.
- About: the world door shows your picture once `assets\about\world.jpg` is in place; the
  write band is gone; **Write me** is inside the sandwich, a small box; the top-left name is
  gone everywhere but the engine.
- No WebGPU: *Welcome. … Meanwhile here is our web page.* and a **Click here** button.

## WHAT THIS DOCUMENT CANNOT SEE (P16)

`fill()`'s exact token discipline (U0 reads it; two blocks already adapted to it, this round
adapts twice more). The about page's `<style>` internals (the `.door` block is found by
symbol). Whether the sandwich dropdown is wide enough for the write box on a narrow phone —
a visual row. `assets/about/world.jpg` does not exist until Jean places it; the build says so
and ships without it.
