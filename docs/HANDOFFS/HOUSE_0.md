# HOUSE_0 — THE HOUSE THE BOARD STANDS IN

*Handoff · authored 2026-09-08 · Claude → CC · Jean holds build, visual, merge, deploy, naming.*
*Filed to `docs/HANDOFFS/HOUSE_0.md` while open; the directory dies with the campaign.*

## THE IDEA, IN ONE PARAGRAPH

The website is the house the board stands in, and after the 7 September hand edits its
rooms had lost their doors: the sandwich on the Gallery and Writings pages had no way Home,
the home page showed a broken comment where its statement was, the letterbox answered
`501 unconfigured` to every message, and the gallery's pinch worked in a 125-pixel band down
the middle of a phone. This round reconnects the rooms (Home returns to both shells), takes
every number off the gallery and puts a work's one line of *info* behind a word, gives the
writings a menu of their own and a page per text, lets the stage own every gesture with the
pan clamped to the picture, and makes the folders the truth — paintings named by number,
found where the collection keeps them, a missing one a warning and never a refused build.
Site-only: the engine shell loses three dead comments, `web_dist.py` changes one comment,
no `src/` file moves. Everything below was rehearsed on a clone of the base and driven in
headless Chromium; the numbers in the witnesses are the rehearsed tree's.

## AUTHORITY

Base: `master` at HEAD `60c2fd39c78e2f3fe3f579c76918f58caea255e2` (FREECLIMB_0 / SKYGLASS_0 / ZOOM_0).
Rides a **held branch `claude/house-0`** — not for a gate this time but for CONCURRENCY:
engine campaigns keep landing on master while this one runs, and the site's files are
disjoint from theirs but for `web/index.html` (three deleted comments) and `docs/OPEN.md`
(both sides prepend an entry). Jean authorises the merge (U8); the branch dies on merge
(ATTIC LAW).

| file | blob at base | sha256 at the rehearsed tip |
| --- | --- | --- |
| `.gitignore` | `3a3dbb027a8923047878947466328485e7f05708` | `67f6c8558d15317b49d3e12f7f148e74d6beb43d7377e0a8e6e2cb8316d9acd1` |
| `MAIN/MERGE.md` | `ae2239047f989d619d0a12dd8752c3a076c59bed` | `deleted` |
| `MAIN/SETUP.md` | `734c8f83d9ccf5c74f6f43c9a86d3223ca003ab0` | `deleted` |
| `assets/about/README.txt` | `28a2c5496fd7de9bfc048e230cd47d6f8cca70d4` | `ef63f49e6db8347b873e339f17789157a8db9dcee7b95c6282311a7733db5f4c` |
| `assets/about/site.json` | `7255c154c9b04afc83debfa2b648781675b47cb4` | `51033835751b2785ee94935921ca5e9ee1c1424ce0991b923062b2ca438cea13` |
| `docs/COPY.md` | `aea07bca41feb091319e0adbc34f45464b72007e` | `997826772e3ae9879a45ab880d955d44c6ce1654481b88c1dfea8168bb7d0757` |
| `docs/OPEN.md` | `b39145affd7ed22b42942ae8f238099647108706` | `0e45d678b0d3c6128746af30aa28ff07b11e943e2a3531840aea5672c8266441` |
| `functions/api/message.js` | `f1d7974a5c5862d8858c08f7646a529280e35c10` | `cf97a160d52236923e12aa61d54c68e5c841c1c604eab3e6cc620fa4ce35da1d` |
| `place_masters.py` | `2f553333f5dee724715e9e53692f9143c2871875` | `deleted` |
| `tools/about_dist.py` | `2aa7f21b461d619afa6eee1389179169cc5767d2` | `12678518f60cb3b9178cb620a3dfae14c29ccb705541e33e6f38824dbdb4328b` |
| `tools/collection_dist.py` | `52db350b68af5ac0a89f9455947e1166fc547680` | `188ae3fe192bd8bda065b05247fbb4f9c02110e35e0dd76ed2b0b83670043465` |
| `tools/web_dist.py` | `991b2c13c661ac354a2fd8a3f8ec7e4a7c61ff08` | `cd38b345f55fde204d266f8f96a8a4a18687d9c7394c15ae4b9b4efaf955e3ed` |
| `web/about/index.html` | `09f7e09eb854a94fb7374d9c925f1b645a542b74` | `b75390c9b4424bae1dff4efbb8d3d1044e3a250959f80b16d39b4d3e8ca5e3c4` |
| `web/collection/index.html` | `fce93e2bdece45d6a7d6c52848df4d4e5ad4cc97` | `aec7ad8851be152cdeeb9f07317672409a629851eb3724a5860eeb6d110290ff` |
| `web/index.html` | `9535e83b828a1e894bb039ec53af7d251eb97d94` | `6e383499df97a0a4e3887ca75a206a42c978d0d4c92e5ead647b613337b3092b` |
| `web/menu.css` | `8617447c737203235dfcc6bfae4200c550cd8bce` | `b28fb15a00e68ba5947b64340f2704f4c5bfb0671155d1a43a2a6c98e6360586` |
| `web/routes.json` | `cb4651c23c78fef446d809341e58ad0c5d3a587d` | `8fda1bf6a1a7bc3af5b5098e490b4526c5d54e61b35ff5f9e79d7fbe7bb39414` |
| `web/writings/index.html` | `4e01403c6e6ea1c33b7f0b229933bd1af1eca773` | `49e807c63c9bbb094333a482e30ac9f06006a45061771da4a08fe7b824305b40` |

**HALT classes (P17), scoped to the unit (P15):** unreachable file; stale authority (the blob
differs AND a FIND does not match). Everything else DEFAULT-AND-FLAG. Line numbers are hints;
boundaries are symbols (P2). Expected match count **1** for every FIND unless stated. LF only,
no BOM (L35) — the new files too. A newer master tip is fine if every blob above still matches;
if a blob moved, apply that file's blocks by hand against what is there and flag.

**WHOLE-FILE BLOCKS.** Five files are replaced in full (`web/about/index.html`,
`web/writings/index.html`, `tools/about_dist.py`, `assets/about/site.json`,
`assets/about/README.txt`). Replace in full ONLY if the blob matches the pin; a moved blob means
another round touched the file since the rehearsal, and a full replacement would silently drop
its work — then `git diff` the rehearsed text against the tree and apply the difference by
hand, flagging what the other round did.

**REHEARSED, NOT RECITED.** Every edit below was applied to a clone of this base, in this order,
every FIND at count 1, one commit per unit (the rehearsed tip is `6cf6d7eff9f4bde117d531d3738dff66407920c5`). After U6 the site half
built end to end — `collection_dist` → `about_dist` → `collection_gate: PASS (246 local refs,
all present; no engine tokens)` — both `--preview` modes wrote self-contained files, every changed
file is LF-only and BOM-free, and `web/index.html` still carries each of its five markers
(`__ROUTES__`, `__MENU_CSS__`, `__FOLLOW__`, `__BUILD_ID__`, `__SHADER_SHA__`) exactly once.
The two Chromium drives in the appendix passed **28/28** (the gallery, touch at 390 px and mouse at
1280 px) and **45/45** (the home page, every menu on every page, the writings, at both widths).
`web_dist.py` was NOT run — the sandbox has no engine artifacts; it is untouched but for one
comment, and Jean's `python tools\dist.py` is where it runs.

**THE TWO DIAGNOSES, from the live site.** A real POST to `/api/message` answered
`HTTP 501 {"error":"unconfigured"}` — the function is deployed, `RESEND_API_KEY` and
`MESSAGE_TO` are not set; nothing in the code was wrong. And on the ZOOM_0 page in Chromium
at 390 px a pinch whose fingers start at 15% and 85% of the stage's width reads
`zoom.s = 1.000` (dead), the same spread in the middle third reads `1.500`, and the live band
measured 125 px: the step zones were absolutely-positioned SIBLINGS over the stage. On the
HOUSE_0 page the outer pinch reads `1.286` — the spread's own ratio, 0.9/0.7.

## THE RULINGS THIS CAMPAIGN LANDS

Jean's stamps: *Home* is the page's one name; the all-texts page is called **scroll**; every
other item on his judgement call went to the ruling below. Flips are recorded so a later round
can undo one edit rather than rediscover it.

1. **Home, on both shells.** The `about` row returns to `web/routes.json` with no `side`;
   the engine's orphaned Home pane (`data-pane="about"`) lives again. *Flip:* `"side": "site"`
   — and then the pane and `loadAbout`'s hero code are deleted, not reserved.
2. **The gallery page's tab and share card say *gallery*** — one name with the menus
   (HOME_0's residual, closed).
3. **The hero says nothing under it.** `.hero-plate` (number + *the collection →*) dies; the
   hero stays a quiet link into that painting on the gallery page. *Flip:* an inert `<div>`.
4. **The doors carry the menu's names in the page's voice:** *the board · gallery · writings*,
   lowercase. *Flip:* *navigate the world* as the one invitation-style door — copy, one edit.
5. **The writings door takes a picture like the board's:** `assets/about/writings.jpg`;
   `build_world` becomes `build_door_image(name)`. Absent = one build line, never a refusal.
6. **No statement and no footer** — Jean took both out; the build reads a statement only if
   a `header.statement` is there. The block is one `git show` away when he writes one.
7. **Writings: a menu of their own and a page per text.** A second `<details>` at the left of
   the top bar, summary *Writings*; the scroll at `/writings/` lists the titles; each text at
   `/writings/<slug>/` lists *scroll* first and the others, never itself (DOORS_1). One
   template, one read, N+1 pages; `writings.json` unchanged. The slug is the filename after
   the number, so renumbering keeps an address and renaming the words moves it.
8. **ZOOM_1 — the stage owns every gesture.** The two zones die. A tap in an outer third steps
   at 1×; a pair never taps; the pinch zooms about its midpoint; one moving pointer pans while
   zoomed; the wheel zooms; a mouse double-click in the middle third toggles 1 ↔ 2.5 (the outer
   thirds have just stepped twice, as they did under ZOOM_0); arrows step; Esc peels then
   closes. A double-TAP is not a zoom on glass — the pinch is, and its first tap has stepped.
   The pan is clamped to the picture's contained rect, so nothing leaves the stage.
9. **No number on the gallery page** — not *no. N*, not *1 / 13*, not *13 works*. `Painting N`
   survives in `alt`/`aria-label` and the `#wN` address only.
10. **info** — a work's one line, in Jean's words, in `assets/collection/<set>/PAINTING_<n>.txt`
    beside its master; shown behind the word *info*, only for a work that has one; the
    visitor's open/closed choice survives a step. Not the filename (a sort key shared with the
    engine's scan), not `set.json`'s `works` map (it dies: a sidecar moves with its painting
    between set folders, and it is tracked while the master is not). *Flip:* the map.
11. **Closing a work opened from its own address lands on the grid** — the one correction
    made without asking: `history.back()` from a `#w<n>` arrival left the gallery for whatever
    page came before, and the hero now arrives exactly there.
12. **The letterbox's sender is configuration:** `MESSAGE_FROM`, defaulting to Resend's
    unverified sender. Jean's dashboard steps are in the register; nothing here waits on them.
13. **The folders are the truth.** `site.json` names paintings by NUMBER; `find_master` looks in
    `assets/collection/*/`; a number no set holds is a warning and leaves the pool (hero) or the
    strip; only an empty `wide` pool refuses (a door with a hole is POSTER_0's law). `set.json`
    `featured` orphans and orphan sidecars warn. `assets/about/` holds `site.json` and the doors'
    pictures, tracked; `authors`/`links` die; `load_site` requires what it reads.
14. **The bootstrap and `MAIN/` go to the attic** — one-shot and superseded; `docs/COPY.md`
    carries what they said that is still true. *Flip:* `git checkout attic/house-0-main-setup -- MAIN`.
15. **The hand edits are cleaned:** the broken statement comment, the commented-out `<p>` and
    footer, the editor's re-indent (the file is rewritten anyway); the two commented Postcard
    rows and the phone list's orphaned `<dt>Postcard</dt>`, the *SAVING JUST IN CASE* link.
    Jean's Controls copy stands.

## UNITS

| unit | what | commit |
| --- | --- | --- |
| U0 | preflight, the branch, the handoff filed | 1 (the handoff) |
| U1 | the hand edits cleaned; Home back | 1 |
| U2 | the home page, its builder and its inputs (`about_dist.py`, `site.json`, `README.txt`) | 1 |
| U3 | the writings | 1 |
| U4 | the gallery: ZOOM_1, no numbers, info | 1 |
| U5 | the letterbox | 1 |
| U6 | the attic tag and the deletions; `.gitignore`; one comment in `web_dist.py` | 1 (+ tag) |
| U7 | the gallery title; COPY.md; OPEN.md; gates; push | 1 |
| U8 | THE MERGE — only when Jean says so | 1 merge |

---

## U0 — PREFLIGHT

```
git rev-parse --is-shallow-repository        # true → git fetch --unshallow origin
git fetch origin master                      # P9
git checkout -b claude/house-0 origin/master
git rev-parse HEAD                           # 60c2fd39c78e2f3fe3f579c76918f58caea255e2 expected; a newer tip is DEFAULT-AND-FLAG if every blob below still matches
git ls-tree HEAD web/index.html web/routes.json web/menu.css web/about/index.html web/writings/index.html web/collection/index.html tools/about_dist.py tools/collection_dist.py tools/web_dist.py functions/api/message.js assets/about/site.json assets/about/README.txt .gitignore docs/COPY.md docs/OPEN.md place_masters.py MAIN/MERGE.md MAIN/SETUP.md
ls assets/collection/*/PAINTING_*.txt 2>&1   # none expected (a sidecar here is Jean's; keep it)
python3 -c "import PIL; from PIL import features; print(PIL.__version__, features.check('avif'))"   # Pillow with AVIF, for the rehearsal builds
```
Report HEAD and the blobs. Place this document at `docs/HANDOFFS/HOUSE_0.md` and commit it
alone: `HOUSE_0 U0 — the handoff filed`. Every FIND below is a verbatim block; count it before
editing (`grep -cF` on a distinctive line is the quick form; the block is the anchor).

**The masters are not in the clone** (gitignored), so `collection_dist` and `about_dist` build
against an empty collection here. To rehearse the builds as the sandbox did: copy
`assets/paintings/PAINTING_*.jp*g` into the five `assets/collection/<set>/` folders by the
number ranges the folder names carry (`a_1-14` → 1..14 … `e_unfiled` takes the rest); they are
ignored and never reach a commit. On Jean's machine the real masters are already there.

---

## U1 — THE HAND EDITS CLEANED; HOME BACK

### U1.1 — `web/routes.json`
The row Jean's 7 Sep commit deleted. No `side`: both shells. (Ruling 1.)

FIND (block 1 of 1 — count 1)
````json
  { "id": "controls",   "label": "Controls",        "engine": "pane" },
  { "id": "collection", "label": "Gallery",         "href": "/collection/",   "engine": "pane" },
  { "id": "writings",   "label": "Writings",        "href": "/writings/",     "engine": "pane" },
  { "id": "write",      "label": "Write me",        "side": "engine", "engine": "pane" },
  { "id": "follow",     "label": "Follow for more", "side": "engine", "engine": "pane" },
  { "id": "world",      "label": "The Board",       "href": "/",  "side": "site", "return": true }
````
REPLACE
````json
  { "id": "controls",   "label": "Controls",        "engine": "pane" },
  { "id": "collection", "label": "Gallery",         "href": "/collection/",   "engine": "pane" },
  { "id": "writings",   "label": "Writings",        "href": "/writings/",     "engine": "pane" },
  { "id": "about",      "label": "Home",            "href": "/about/",        "engine": "pane" },
  { "id": "write",      "label": "Write me",        "side": "engine", "engine": "pane" },
  { "id": "follow",     "label": "Follow for more", "side": "engine", "engine": "pane" },
  { "id": "world",      "label": "The Board",       "href": "/",  "side": "site", "return": true }
````

### U1.2 — `web/index.html` — three deletions
The phone list's `<dt>Postcard</dt>` had lost its `<dd>` to a comment and rendered as a term
with no definition; the desktop list's pair was commented whole; the Writings pane's door-out
sat in a *SAVING JUST IN CASE* comment. All three are attic. Nothing else in the shell moves.

FIND (block 1 of 3 — count 1)
````html
          <dd>one tap on the right half</dd>
          <dt>FPV</dt>
          <dd>two fingers tap on the left half</dd>
          <dt>Postcard</dt>
          <!--<dd>before a picture, tap the words that appear — it is yours to keep or send</dd>-->
      </dl>
      <dl class="ctl" data-platform="desktop" hidden>
          <dt>Directions</dt>
````
REPLACE
````html
          <dd>one tap on the right half</dd>
          <dt>FPV</dt>
          <dd>two fingers tap on the left half</dd>
      </dl>
      <dl class="ctl" data-platform="desktop" hidden>
          <dt>Directions</dt>
````

FIND (block 2 of 3 — count 1)
````html
          <dd>Space</dd>
          <dt>FPV</dt>
          <dd>Ctrl</dd>
          <!--<dt>Postcard</dt>
    <dd>P, before a picture — or click the words that appear</dd>-->
      </dl>
      <div class="row">
        <button type="button" id="ctlPhoto">Take a photo</button>
````
REPLACE
````html
          <dd>Space</dd>
          <dt>FPV</dt>
          <dd>Ctrl</dd>
      </dl>
      <div class="row">
        <button type="button" id="ctlPhoto">Take a photo</button>
````

FIND (block 3 of 3 — count 1)
````html
            <h2 class="wtitle" id="wTitle"></h2>
            <div class="text" id="wBody"></div>
        </div>
        <!-- <a class="out" data-out target="_blank" rel="opener" href="writings/">Read on the website →</a> ### SAVING JUST IN CASE-->
    </div>
    <div class="pane" data-pane="write" hidden>
      <button type="button" class="back" data-back>← menu</button>
````
REPLACE
````html
            <h2 class="wtitle" id="wTitle"></h2>
            <div class="text" id="wBody"></div>
        </div>
    </div>
    <div class="pane" data-pane="write" hidden>
      <button type="button" class="back" data-back>← menu</button>
````

Witness:
```
grep -c "Postcard" web/index.html              # 0
grep -c "SAVING" web/index.html                # 0
grep -c '"id": "about"' web/routes.json        # 1
for m in '<!-- __ROUTES__ -->' '/\* __MENU_CSS__ \*/' '<!-- __FOLLOW__ -->' __BUILD_ID__ __SHADER_SHA__; do grep -c -- "$m" web/index.html; done   # 1 1 1 1 1
python3 tools/routes.py | grep -c 'data-pane="about"'   # 1 — the engine's Home button renders again
```
Commit: `HOUSE_0 U1 — the hand edits cleaned: Home returns to the routes (both shells), three dead comments leave the shell`

---

## U2 — THE HOME PAGE

### U2.1 — `web/about/index.html`, REPLACED IN FULL (blob check first — see AUTHORITY)
What changed against the base: the plate and `#hero-title` are gone (and the hero's `<a>` stays);
the statement header, its rules and the footer are gone; the doors read *the board · gallery ·
writings* and the writings door gains the `__WRITINGS_IMG__` slot; `html { scroll-behavior }` is
gone (no anchors left); the `<meta name="description">` names Jean and the three rooms; the body
is back at the file's two-space indent. The `__HERO__`, `__STRIP__`, `__WORLD__`, `__ROUTES__`,
`__MENU_CSS__` and `__HERO_DATA__` slots are exactly where `about_dist` expects them.

````html
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="color-scheme" content="dark">
<title>home — the ever expanding board</title>
<meta name="description" content="Jean Klein's paintings, writings, and a procedurally generated world you can walk into.">
<meta property="og:title" content="the ever expanding board">
<meta property="og:type" content="website">
<link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32'%3E%3Cpath fill='%23e8e6e0' d='M16 4a5 5 0 0 0-2.6 9.3c-.5 2.9-1.6 5.2-3.4 7.2h12c-1.8-2-2.9-4.3-3.4-7.2A5 5 0 0 0 16 4zM8 23h16v2.6c0 .8-.6 1.4-1.4 1.4H9.4C8.6 27 8 26.4 8 25.6z'/%3E%3C/svg%3E">
<link rel="stylesheet" href="../shared.css">
<style>
  /* Everything here is home-page-only. Tokens, type, masthead and the
     spacing scale live in shared.css — never redefined here. */

  /* ── the hero ─────────────────────────────────────────────────────
     Capped so the door has one shape. Without the cap the front page
     was 506px tall or 985px tall depending on which work the daily
     rotation happened to pick. It is a quiet door: the picture opens
     itself on the gallery page, and nothing is written under it
     (HOUSE_0 — no number, no caption, no second link). */
  .hero { display: block; }
  .hero img {
    display: block;
    width: 100%;
    height: auto;
    max-height: min(58vh, 560px);
    object-fit: cover;
    object-position: center;
  }

  /* ── the doors — three, lowercase, the menu's own names (HOUSE_0) ── */
  .doors {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(min(340px, 100%), 1fr));
    gap: var(--s5);
    padding: var(--s5) var(--edge) var(--s6);
  }
  .door { text-decoration: none; display: block; }
  .door h2 {
    margin: 0 0 0.35em;
    font-size: clamp(23px, 2.4vw, 31px);
    font-weight: 400;
  }
  .door .go {
    display: inline-block;
    border-bottom: 1px solid var(--faint);
    transition: border-color 200ms ease;
  }
  .door:hover .go, .door:focus-visible .go { border-color: var(--ink); }
  .door p { margin: 0 0 var(--s2); color: var(--dim); max-width: 30em; }
  .door .door-img { display: block; width: 100%; height: auto; margin-top: var(--s2); }

  /* wider than its column on a phone: scrolls inside itself rather
     than handing the whole page a scrollbar */
  .strip {
    display: flex;
    gap: 10px;
    margin-top: var(--s2);
    overflow-x: auto;
    overscroll-behavior-x: contain;
    scrollbar-width: none;
  }
  .strip::-webkit-scrollbar { display: none; }
  .strip img { display: block; height: clamp(70px, 8vw, 112px); width: auto; }

/* __MENU_CSS__ */
</style>
</head>
<body>

<div class="top">
  <!-- DOORS_0 — THE SANDWICH. A native <details>: it opens with no
       script, so this page keeps working with JavaScript off. The links
       are rendered by tools/routes.py from web/routes.json — the one
       home the engine shell and collection/ also read. -->
  <details class="menu">
    <summary aria-label="menu"><span class="bars" aria-hidden="true"></span></summary>
    <nav>
      <!-- __ROUTES__ -->
    </nav>
  </details>
</div>

<a class="hero" id="hero" href="../collection/">
  <!-- __HERO__ -->
</a>

<!-- HOUSE_0 — no statement and no footer: Jean took both out. When the
     statement is written it is one block (header.statement + its rules)
     back from git; about_dist reads it into about.json if it is here. -->

<div class="doors">
  <a class="door" href="../">
    <h2><span class="go">the board</span></h2>
    <!-- __WORLD__ -->
  </a>
  <a class="door" href="../collection/">
    <h2><span class="go">gallery</span></h2>
    <p>Oils, watercolours and pen, in sets. Every work, one page.</p>
    <div class="strip">
      <!-- __STRIP__ -->
    </div>
  </a>
  <a class="door" href="../writings/">
    <h2><span class="go">writings</span></h2>
    <!-- PLACEHOLDER: one line -->
    <p>The pawn is a vessel for projected intent.</p>
    <!-- __WRITINGS_IMG__ -->
  </a>
</div>

<script>
/* ── the hero rotates daily; without JavaScript, today is the first ── */
const HERO = /* __HERO_DATA__ */ null;
if (HERO && HERO.wide.length) {
  const day = Math.floor(Date.now() / 864e5);
  const wide = HERO.wide[day % HERO.wide.length];
  const tall = HERO.tall.length ? HERO.tall[day % HERO.tall.length] : wide;
  const pick = matchMedia("(max-aspect-ratio: 4/5)").matches ? tall : wide;
  const img = document.querySelector("#hero img");
  const cur = new URL(img.currentSrc || img.src, location.href).pathname;
  if (!cur.endsWith(pick.src)) {
    img.src = pick.src;
    img.width = pick.w; img.height = pick.h;
  }
  document.getElementById("hero").href = "../collection/#w" + pick.n;
}
</script>
</body>
</html>
````

### U2.2 — `tools/about_dist.py`, REPLACED IN FULL (blob check first — see AUTHORITY)
The builder and its inputs move together in this unit, because the new builder reads the new
`site.json` (U2.3) and would refuse the old one. What changed against the base, all of it
tested: the header docstring (the new `site.json` shape); `load_site` requiring `email` and
`hero` only and typing the pools as integer lists; `MASTER_EXTS` and `find_master(n)` (the
master of painting n, wherever `assets/collection/<set>/` keeps it); `build_hero` and
`build_strip` by number, a missing one a warning, an empty `wide` pool the one refusal;
`build_world` → `build_door_image(Image, preview, name)`; the writings functions U3 will use
(`writing_slug`, `build_writings` returning pieces, `writing_article`, `writings_menu_html`,
`write_writings_page`); `fill()` taking `TITLE` as a plain token and the four new survivors;
`main` sweeping `dist/about/` after the refusable reads, filling the writings template N+1
times after sweeping `dist/writings/`, and reading a statement only if a header is there.

````python
#!/usr/bin/env python3
# ─── tools/about_dist.py ─────────────────────────────────────────
#
# The home page's pipeline (the page a menu calls Home; `about` is its
# wiring — the path, the id, this file). Third sibling: web_dist.py owns
# the world AT THE ROOT, collection_dist.py owns /collection/, this owns
# /about/ and /writings/. The engine keeps `/` — so this must never write
# dist/index.html, or it would overwrite the engine's own shell.
#
#   python tools/about_dist.py                   # build into dist/about/ + dist/writings/
#   python tools/about_dist.py --preview F       # one self-contained file
#
# Reads assets/about/site.json (HOUSE_0 — paintings are NUMBERS, and the
# files live where the collection keeps them, assets/collection/<set>/):
#   email    the address the engine's write pane falls back to
#   hero     {"wide":[90, 92, 112], "tall":[50, 109]}  hand-curated pools —
#            hand-curated is the rule; an automatic pick once nominated
#            a photograph of a monkey as the front page of the universe.
#            A number with no master is a warning and leaves the pool; an
#            empty wide pool is the one refusal, because the door would be
#            a hole (POSTER_0's law).
#   strip    [2, 5, 7, 11]  works from the collection under the gallery
#            door; dist mode points at the collection's own derivatives,
#            preview embeds. A number with no master is a warning and a
#            shorter strip.
# and assets/about/<door>.jpg (world, writings): the doors' pictures,
# tracked; absent = a door without one.
#
# Writes dist/about/ (index.html, about.json, hero/*, the door pictures),
# dist/writings/ (the scroll, one page per text, writings.json), dist/fonts/
# and dist/shared.css (the fonts and the stylesheet live once, at the root;
# the pages reach up to them). Never touches dist/index.html or
# dist/collection. Build order is collection first, then this, so the strip
# can find its derivatives.

import argparse
import base64
import io
import glob
import json
import routes   # DOORS_0 — web/routes.json + web/menu.css, the sandwich's one renderer
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SRC = os.path.join(ROOT, "assets", "about")
WEB = os.path.join(ROOT, "web")
TEMPLATE = os.path.join(WEB, "about", "index.html")
FONTS = os.path.join(ROOT, "web", "fonts")
DIST_ROOT = os.path.join(ROOT, "dist")
DIST = os.path.join(DIST_ROOT, "about")

HERO_EDGES = (1600, 800)
DOOR_EDGE = 1600       # HOUSE_0 — a door's picture (world, writings): the hero's long edge
DOOR_JPEG_Q = 78
HERO_JPEG_Q = 82
STRIP_H = 240          # strip images are small; one size is enough


def say(m=""):
    print(m)


def esc(s):
    return (str(s).replace("&", "&amp;").replace("<", "&lt;")
            .replace(">", "&gt;").replace('"', "&quot;"))


def load_site():
    path = os.path.join(SRC, "site.json")
    if not os.path.isfile(path):
        say("no %s — nothing to build" % path)
        sys.exit(1)
    with open(path, encoding="utf-8") as fh:
        site = json.load(fh)
    # HOUSE_0 — it requires what it reads and nothing else. `authors` and
    # `links` were required and read by nothing since DOORS_4; gone.
    for key in ("email", "hero"):
        if key not in site:
            say("REFUSE  site.json is missing %r" % key)
            sys.exit(1)
    for kind in ("wide", "tall"):
        pool = site["hero"].get(kind, [])
        if not all(isinstance(n, int) for n in pool):
            say("REFUSE  site.json hero.%s must be a list of painting numbers, e.g. [90, 92]" % kind)
            sys.exit(1)
    if not all(isinstance(n, int) for n in site.get("strip", [])):
        say("REFUSE  site.json strip must be a list of painting numbers, e.g. [2, 5, 7]")
        sys.exit(1)
    if not site["hero"].get("wide"):
        say("REFUSE  hero.wide is empty — the door needs at least one work")
        sys.exit(1)
    return site


MASTER_EXTS = (".jpg", ".jpeg", ".png")


def find_master(n):
    """HOUSE_0 — the master of painting n, wherever the collection keeps it:
    assets/collection/<any set>/PAINTING_<n>.<ext>. None if no set has it —
    the caller warns and moves on. The first match by set order if two sets
    hold the number (collection_dist allows that across folders)."""
    root = os.path.join(ROOT, "assets", "collection")
    if not os.path.isdir(root):
        return None
    for folder in sorted(os.listdir(root)):
        for ext in MASTER_EXTS:
            path = os.path.join(root, folder, "PAINTING_%d%s" % (n, ext))
            if os.path.isfile(path):
                return path
    return None


def data_uri(im, edge, q):
    step = im.convert("RGB").copy()
    step.thumbnail((edge, edge))
    buf = io.BytesIO()
    step.save(buf, "WEBP", quality=q, method=6)
    return "data:image/webp;base64," + base64.b64encode(buf.getvalue()).decode()


def build_hero(Image, site, preview):
    """Returns (first_img_tag, hero_data). Encodes derivatives in dist
    mode; embeds in preview mode. The no-JS visitor gets wide[0]."""
    out_dir = os.path.join(DIST, "hero")
    if not preview:
        os.makedirs(out_dir, exist_ok=True)

    data = {"wide": [], "tall": []}
    for kind in ("wide", "tall"):
        for n in site["hero"].get(kind, []):
            path = find_master(n)
            if path is None:
                say("  warning  hero.%s names %d and no set holds PAINTING_%d — it leaves the pool" % (kind, n, n))
                continue
            with Image.open(path) as im:
                im = im.convert("RGB")
                w, h = im.size
                if preview:
                    src = data_uri(im, 1400, 66)
                else:
                    edge = min(max(w, h), HERO_EDGES[0])
                    step = im.copy()
                    step.thumbnail((edge, edge))
                    name = "%d-%d.jpg" % (n, step.size[0])
                    step.save(os.path.join(out_dir, name), "JPEG",
                              quality=HERO_JPEG_Q, optimize=True, progressive=True)
                    src = "hero/" + name
                    w, h = step.size
                data[kind].append({"n": n, "src": src, "w": w, "h": h})
    if not data["wide"]:
        say("REFUSE  every painting hero.wide names is missing — the door would be a hole; name one that exists")
        sys.exit(1)

    first = data["wide"][0]
    tag = ('<img src="%s" alt="Painting %d" width="%d" height="%d" '
           'fetchpriority="high" decoding="async">'
           % (first["src"], first["n"], first["w"], first["h"]))
    return tag, data


def build_strip(Image, site, preview):
    """The works under the gallery door, by number. Dist mode finds each
    one's 640 rung in dist/collection/<any set>/ — the derivative names
    carry a content hash and the set's slug is the collection's business,
    so both are found, never guessed — and a number nobody built is a
    warning and a shorter strip, never a refused build (HOUSE_0)."""
    tags = []
    for n in site.get("strip", []):
        if preview:
            path = find_master(n)
            if path is None:
                say("  warning  strip names %d and no set holds PAINTING_%d — skipped" % (n, n))
                continue
            with Image.open(path) as im:
                src = data_uri(im, 560, 62)
                w, h = im.size
        else:
            pattern = os.path.join(DIST_ROOT, "collection", "*", "%d-640.*.jpg" % n)
            found = sorted(glob.glob(pattern))
            if not found:
                say("  warning  strip names %d and dist/collection holds no %d-640 — skipped "
                    "(a number no set has, or the collection is not built)" % (n, n))
                continue
            target = found[0]
            src = "../collection/%s/%s" % (os.path.basename(os.path.dirname(target)),
                                            os.path.basename(target))
            with Image.open(target) as im:
                w, h = im.size
        tags.append('<img src="%s" alt="Painting %d" loading="lazy" '
                    'decoding="async" width="%d" height="%d">'
                    % (src, n, w, h))
    return "\n      ".join(tags)


def build_door_image(Image, preview, name):
    """HOUSE_0 — A DOOR'S PICTURE, by name. Jean drops the file at
    assets/about/<name>.jpg (or .jpeg/.png) — `world` for the board's door,
    `writings` for the writings' — and the door shows it. Absent = one line
    and a door without a picture, NEVER a refused build: a door must not hold
    the site hostage (DOORS_4's ruling for the world door, now for both).
    Shaped like build_hero and build_strip — embeds in preview mode, writes a
    derivative in dist mode — so a preview stays self-contained."""
    for ext in ("jpg", "jpeg", "png"):
        src = os.path.join(SRC, name + "." + ext)
        if not os.path.isfile(src):
            continue
        with Image.open(src) as im:
            if preview:
                uri = data_uri(im, DOOR_EDGE, 66)
                return ('<img class="door-img" src="%s" alt="" '
                        'loading="lazy" decoding="async">' % uri)
            step = im.convert("RGB")
            # THE LONG EDGE, not the width (DOORS_4 R5): thumbnail() caps it
            # on both axes and never upscales, the hero's own tool.
            if max(step.size) > DOOR_EDGE:
                step.thumbnail((DOOR_EDGE, DOOR_EDGE), Image.LANCZOS)
            os.makedirs(DIST, exist_ok=True)
            out = name + ".jpg"
            step.save(os.path.join(DIST, out), "JPEG",
                      quality=DOOR_JPEG_Q, optimize=True, progressive=True)
            return ('<img class="door-img" src="%s" alt="" '
                    'width="%d" height="%d" loading="lazy" decoding="async">'
                    % (out, step.size[0], step.size[1]))
    say("  %s door: assets/about/%s.jpg not found — the door ships without its picture"
        % (name, name))
    return ""


def writing_slug(stem):
    """The URL of a text, from its filename: the stem after the leading
    NN_ (the order, which is not a name), lowercased, non-alphanumerics
    to hyphens. `01_the_mirror_in_the_sand` -> `the-mirror-in-the-sand`.
    Reordering a file (renumbering it) therefore never moves its page;
    renaming the words after the number does, and is Jean's to do."""
    body = re.sub(r"^\d+_", "", stem)
    slug = re.sub(r"[^a-z0-9]+", "-", body.lower()).strip("-")
    return slug


def build_writings():
    """The writings, one home: assets/writings/NN_slug.txt. First line the
    title, then the body; blank lines break stanzas; line breaks inside a
    stanza are kept (white-space: pre-line on the page, <br> nowhere).
    Filename order is page order. Returns the pieces: slug, title, html."""
    src = os.path.join(ROOT, "assets", "writings")
    if not os.path.isdir(src):
        say("REFUSE  assets/writings is missing — the writings have one home and this is it")
        sys.exit(1)
    pieces = []
    for name in sorted(os.listdir(src)):
        if not name.endswith(".txt"):
            continue
        # utf-8-sig, not utf-8: several Windows editors write a BOM by
        # default, `encoding="utf-8"` keeps it, and U+FEFF is not
        # whitespace — `.strip()` cannot remove it, so it would ride at
        # the head of the first title, on the page and in the json both.
        # (DOORS_4 R5. utf-8-sig reads plain UTF-8 unchanged.)
        with open(os.path.join(src, name), encoding="utf-8-sig") as fh:
            raw = fh.read().replace("\r\n", "\n").strip("\n")
        lines = raw.split("\n")
        title = lines[0].strip()
        body = "\n".join(lines[1:]).strip("\n")
        if not title or not body:
            say("REFUSE  %s needs a title line and a body" % name)
            sys.exit(1)
        stanzas = [st.strip("\n") for st in body.split("\n\n") if st.strip()]
        html = "\n".join("<p>%s</p>" % esc(st) for st in stanzas)
        slug = writing_slug(os.path.splitext(name)[0])
        if not slug:
            say("REFUSE  %s has nothing after its number to make a URL from" % name)
            sys.exit(1)
        pieces.append({"slug": slug, "title": title, "html": html})
    if not pieces:
        say("REFUSE  assets/writings holds no .txt — nothing to build")
        sys.exit(1)
    # HOUSE_0 — a slug is a page: two files that share one would give one
    # URL two texts, which is the refusal the collection makes for numbers.
    seen = {}
    for p in pieces:
        if p["slug"] in seen:
            say("REFUSE  writings %s and %s share the URL /writings/%s/" % (seen[p["slug"]], p["title"], p["slug"]))
            sys.exit(1)
        seen[p["slug"]] = p["title"]
    return pieces


def writing_article(p):
    # The slug is a FILENAME, and a filename can hold a quote — escaped like
    # the title and the body always were (DOORS_4 R2).
    return '<article id="%s">\n<h2>%s</h2>\n%s\n</article>' % (esc(p["slug"]), esc(p["title"]), p["html"])


def writings_menu_html(pieces, base, indent="      "):
    """HOUSE_0 — THE WRITINGS MENU, spoken from the page at `base`. On the
    scroll it lists the titles; on a single page the scroll comes first and
    the page's own title is not there — routes.rel() answers "./" for the
    page itself and that entry is dropped, the sandwich's own rule (DOORS_1)."""
    items = [("scroll", "/writings/")] + [(p["title"], "/writings/%s/" % p["slug"]) for p in pieces]
    out = []
    for label, href in items:
        rel = routes.rel(href, base)
        if rel == "./":
            continue
        out.append('<a href="%s">%s</a>' % (esc(rel), esc(label)))
    return ("\n" + indent).join(out)


def write_writings_page(template, pieces, page_pieces, base, title, out_dir):
    """One fill of web/writings/index.html: the scroll (every piece, base
    /writings/) or one text alone (base /writings/<slug>/). The stylesheet
    is the one ../ reference in the template; a page one level down reaches
    it with one more."""
    depth = base.count("/") - 1
    page = fill(template, {
        "TITLE": title,
        "WMENU": writings_menu_html(pieces, base),
        "ROUTES": routes.nav_html("site", base, indent="      "),
        "MENU_CSS": routes.menu_css(),
        "WRITINGS": "\n".join(writing_article(p) for p in page_pieces),
    })
    if depth == 2:
        page = page.replace('href="../shared.css"', 'href="../../shared.css"')
    os.makedirs(out_dir, exist_ok=True)
    with open(os.path.join(out_dir, "index.html"), "w", encoding="utf-8") as fh:
        fh.write(page)


def fill(template, subs):
    out = template
    for key, val in subs.items():
        marker = "<!-- __%s__ -->" % key if key not in ("HERO_DATA", "EMAIL", "TITLE", "MENU_CSS") else None
        if key == "HERO_DATA":
            out = out.replace("/* __HERO_DATA__ */ null", json.dumps(val))
        elif key == "EMAIL":
            out = out.replace("__EMAIL__", val)
        elif key == "TITLE":
            out = out.replace("__TITLE__", esc(val))   # HOUSE_0 — inside <title>, a comment would be text
        elif key == "MENU_CSS":
            out = out.replace("/* __MENU_CSS__ */", val)   # DOORS_0 — the sandwich's rules, one home
        else:
            out = out.replace(marker, val)
    # DOORS_4 — THE TUPLE IS THE ONLY GUARD. fill() checks the OUTPUT, so a
    # token absent from a given template simply never appears and passes;
    # but a token PRESENT and unsubstituted ships silently unless it is
    # named here. __WRITINGS__ and __WORLD__ join for that reason.
    for token in ("__HERO__", "__STRIP__", "__WRITINGS__", "__WORLD__", "__WRITINGS_IMG__",
                  "__WMENU__", "__TITLE__", "__HERO_DATA__", "__EMAIL__", "__ROUTES__", "__MENU_CSS__"):
        if token in out:
            say("REFUSE  template placeholder %s did not substitute" % token)
            sys.exit(1)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preview", metavar="FILE")
    args = ap.parse_args()
    preview = bool(args.preview)

    try:
        from PIL import Image
    except ImportError:
        say("REFUSE  Pillow is required (pip install pillow)")
        sys.exit(1)

    with open(TEMPLATE, encoding="utf-8") as fh:
        template = fh.read()

    site = load_site()
    # DOORS_4 R2 — EVERY READ THAT CAN REFUSE HAPPENS BEFORE THE FIRST
    # DIST WRITE. build_writings() refuses on an empty assets/writings, a
    # missing title line or an empty body; run where it used to sit — after
    # the about page shipped and BEFORE the dist/text/ sweep — such a
    # refusal left a dist carrying a fresh about page, stale fonts, and the
    # live dist/text/index.html the sweep exists to remove. dist.py stops
    # the chain, so that state is never deployed; but a guarantee that
    # depends on the order of the lines above it should not depend on it
    # silently. It is a pure read: it has no reason to be later.
    w_pieces = build_writings()
    # HOUSE_0 — THE WRITER OWNS ITS FOLDER WHOLESALE (collection_dist's own
    # rule), and sweeps it before its first write: a hero taken out of
    # site.json would otherwise stay in dist/about/hero/ under the tenant
    # rule, uploaded and served by every deploy after.
    if not preview and os.path.isdir(DIST):
        shutil.rmtree(DIST)
    hero_tag, hero_data = build_hero(Image, site, preview)

    page = fill(template, {
        "ROUTES": routes.nav_html("site", "/about/", indent="      "),   # DOORS_0 / DOORS_1: spoken from this page
        "MENU_CSS": routes.menu_css(),                        # DOORS_0
        "HERO": hero_tag,
        "STRIP": build_strip(Image, site, preview),
        "WORLD": build_door_image(Image, preview, "world"),          # DOORS_4 — the board's door
        "WRITINGS_IMG": build_door_image(Image, preview, "writings"), # HOUSE_0 — the writings' door
        "HERO_DATA": hero_data,
        "EMAIL": site["email"],
    })

    if preview:
        with open(os.path.join(ROOT, "web", "shared.css"), encoding="utf-8") as fh:
            shared = fh.read()
        page = page.replace('<link rel="stylesheet" href="../shared.css">',
                            "<style>\n" + shared + "\n</style>")
        # the @font-face rules now come from shared.css, whose url() is
        # written relative to the deployment root, not to this page
        for face in ("Newsreader.woff2", "Newsreader-Italic.woff2"):
            with open(os.path.join(FONTS, face), "rb") as fh:
                uri = "data:font/woff2;base64," + base64.b64encode(fh.read()).decode()
            page = page.replace('url("fonts/%s")' % face, 'url("%s")' % uri)
        with open(args.preview, "w", encoding="utf-8") as fh:
            fh.write(page)
        say("wrote %s  (%.1f MB, self-contained)"
            % (args.preview, os.path.getsize(args.preview) / 1e6))
        return

    os.makedirs(DIST, exist_ok=True)
    with open(os.path.join(DIST, "index.html"), "w", encoding="utf-8") as fh:
        fh.write(page)
    # DOORS_2/DOORS_3 — THE ABOUT PAGE IN MINIATURE: the statement, today's
    # hero (the same day-indexed pick the page makes) and the address, for
    # the engine's About and Write panes. Taken from the page just written,
    # so the two cannot disagree (peek.json's law). The template stays the
    # statement's one home; the writings' is assets/writings/, and their
    # page is web/writings/index.html (DOORS_4).
    # HOUSE_0 — THE STATEMENT IS OPTIONAL. Jean took it off the page; when
    # he writes one, the header comes back and this reads it again. An
    # absent header is an empty statement, not a refused build.
    m = re.search(r'<header class="statement" id="statement">(.*?)</header>', page, re.S)
    with open(os.path.join(DIST, "about.json"), "w", encoding="utf-8") as fh:
        json.dump({"statement": m.group(1).strip() if m else "",
                   "hero": hero_data,          # what build_hero returned: the day-indexed list the page itself rotates
                   "email": site["email"]}, fh)

    # DOORS_4 — THE WRITINGS, plural and growing; HOUSE_0 — AND EACH ONE A
    # PAGE. One home for the texts (assets/writings); one read (taken above,
    # before any dist write) writes the scroll at /writings/, one page per
    # text at /writings/<slug>/, and writings.json for the engine's pane, so
    # none of the three can disagree. A new text is one new file plus a
    # deploy. dist/writings/ is swept first: a text renamed or removed since
    # the last build would otherwise leave its old page live under the
    # tenant rule (web_dist deletes only the engine's own names).
    w_tpl = os.path.join(WEB, "writings", "index.html")
    with open(w_tpl, encoding="utf-8") as fh:
        w_template = fh.read()
    w_dist = os.path.join(DIST_ROOT, "writings")
    if os.path.isdir(w_dist):
        shutil.rmtree(w_dist)
    write_writings_page(w_template, w_pieces, w_pieces, "/writings/", "writings", w_dist)
    for p in w_pieces:
        write_writings_page(w_template, w_pieces, [p], "/writings/%s/" % p["slug"], p["title"],
                            os.path.join(w_dist, p["slug"]))
    with open(os.path.join(w_dist, "writings.json"), "w", encoding="utf-8") as fh:
        json.dump([{"title": p["title"], "html": p["html"]} for p in w_pieces], fh)
    say("dist/writings/: the scroll, %d page(s), writings.json" % len(w_pieces))

    # DOORS_4 — AND THE OLD PAGE IS SWEPT, because nothing else will.
    # DOORS_3 made this script the writer of dist/text/, and web_dist
    # deletes only "the engine's own names" — text is not among them. So
    # a dist/ from before this round keeps a live dist/text/index.html
    # that the deploy would upload and serve, SHADOWING the 301 in
    # _redirects. The writer that made it is the one that removes it.
    stale_text = os.path.join(DIST_ROOT, "text")
    if os.path.isdir(stale_text):
        shutil.rmtree(stale_text)
        say("dist/text/ swept — /text/ is a 301 to /writings/ now")
    dst_fonts = os.path.join(DIST_ROOT, "fonts")
    if os.path.isdir(dst_fonts):
        shutil.rmtree(dst_fonts)
    shutil.copytree(FONTS, dst_fonts)
    # one stylesheet, at the root; both pages reach up to ../shared.css
    shutil.copy2(os.path.join(ROOT, "web", "shared.css"),
                 os.path.join(DIST_ROOT, "shared.css"))

    holds = page.count("PLACEHOLDER")
    if holds:
        say("")
        say("  %d PLACEHOLDER marker%s still in the page — copy is not final."
            % (holds, "" if holds == 1 else "s"))
        say("  Search web/about/index.html.")
        say("")
    say("dist/about/index.html written; hero beside it, fonts at dist/fonts/")
    say("build order: collection_dist, then this, then web_dist LAST —")
    say("since WEBSITE_1 it deletes only the engine's own names, and its")
    say("root _headers folds our fragment and rules only what exists.")


if __name__ == "__main__":
    main()
````

### U2.3 — `assets/about/site.json`, REPLACED IN FULL
Paintings by number; `authors` and `links` (placeholders read by nothing since DOORS_4) are gone.

````json
{
  "email": "jean@everexpandingboard.com",
  "hero": { "wide": [90, 92, 112], "tall": [50, 109] },
  "strip": [2, 5, 7, 11]
}
````

### U2.4 — `assets/about/README.txt`, REPLACED IN FULL

````
The home page's own folder.
  site.json      the site email; the hero pools and the strip, as painting
                 NUMBERS — the files live in assets/collection/<set>/.
  world.jpg      the board's door picture (jpg, jpeg or png)
  writings.jpg   the writings' door picture (same)
Nothing else lives here. A door with no picture is a door, not a broken build.
````

Witness (with the masters placed as U0 describes; `collection_dist` is still the base's here and
writes the same derivative names):
```
python3 tools/collection_dist.py >/dev/null && python3 tools/about_dist.py 2>&1 | grep -E "warning|REFUSE|door|written|writings/"
   # "writings door: assets/about/writings.jpg not found — the door ships without its picture"
   # "dist/writings/: the scroll, 3 page(s), writings.json"   (the old template, so no left menu yet — U3)
   # "dist/about/index.html written"
grep -c "hero-plate\|hero-title\|<footer" dist/about/index.html      # 0
grep -o '<span class="go">[^<]*' dist/about/index.html                # the board / gallery / writings
grep -c '<img src="../collection/' dist/about/index.html              # 4 — the strip, found by number
ls dist/about/hero                                                    # 109-940.jpg 112-1600.jpg 50-837.jpg 90-1280.jpg 92-1280.jpg
python3 -c "import json;print(repr(json.load(open('dist/about/about.json'))['statement']))"   # ''
python3 - <<'EOF'
import json; p='assets/about/site.json'; d=json.load(open(p)); d['hero']['wide'].append(4242); d['strip'].append(4343); json.dump(d,open(p,'w'),indent=2)
EOF
python3 tools/about_dist.py 2>&1 | grep -E "warning|REFUSE"
   #   warning  hero.wide names 4242 and no set holds PAINTING_4242 — it leaves the pool
   #   warning  strip names 4343 and dist/collection holds no 4343-640 — skipped (a number no set has, or the collection is not built)
git checkout assets/about/site.json
python3 tools/about_dist.py --preview /tmp/about_preview.html | tail -1     # wrote … (self-contained)
```
Commit: `HOUSE_0 U2 — the home page: plate gone, statement and footer gone, doors lowercase with the menu's names, the writings door takes a picture`

---

## U3 — THE WRITINGS

### U3.1 — `web/writings/index.html`, REPLACED IN FULL
One template, N+1 pages. The `TITLE` token is a plain `__TITLE__` inside `<title>` and `og:title`
(a comment there would be text); the `WMENU` marker is the Writings menu. **Neither token is
spelled in full anywhere but its slot** — `fill()` checks the OUTPUT for survivors, and the
rehearsal's first draft refused itself over a CSS comment that named `__WMENU__`.

````html
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="color-scheme" content="dark">
<title>__TITLE__ — the ever expanding board</title>
<meta name="description" content="Writings from the ever expanding board.">
<meta property="og:title" content="__TITLE__ — the ever expanding board">
<meta property="og:type" content="website">
<link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32'%3E%3Cpath fill='%23e8e6e0' d='M16 4a5 5 0 0 0-2.6 9.3c-.5 2.9-1.6 5.2-3.4 7.2h12c-1.8-2-2.9-4.3-3.4-7.2A5 5 0 0 0 16 4zM8 23h16v2.6c0 .8-.6 1.4-1.4 1.4H9.4C8.6 27 8 26.4 8 25.6z'/%3E%3C/svg%3E">
<link rel="stylesheet" href="../shared.css">
<style>
  /* writings-page-only. Tokens, type, masthead and the spacing scale
     live in shared.css — never redefined here. The vertical rhythm uses
     shared.css's own steps: no new clamp for spacing. No .site line and
     no footer (DOORS_4, ruling 8).

     ONE TEMPLATE, N+1 PAGES (HOUSE_0). about_dist fills it once as the
     scroll — every text, filename order — and once per text as that
     text alone at /writings/<slug>/. The TITLE token is the page's own
     name (the tab and the share card); the WMENU marker is the Writings
     menu, whose list is the OTHER pages, never this one. Neither token
     is spelled in full anywhere but its slot: fill() checks the OUTPUT
     for survivors, and a comment that spelled one would be a survivor.
     The stylesheet is the template's one ../ reference; a single page
     sits one level deeper and about_dist rewrites it to ../../. */

  .writings { max-width: 34em; margin: var(--s5) auto; padding: 0 var(--edge); }
  .writings article { margin: 0 0 var(--s5); }
  .writings h2 { font-size: clamp(22px, 2.6vw, 34px); font-weight: 430; margin: 0 0 var(--s2); }
  .writings p { font-size: 18px; line-height: 1.55; margin: 0 0 var(--s2); white-space: pre-line; }
/* __MENU_CSS__ */
</style>
</head>

<body>

<div class="top">
  <!-- HOUSE_0 — THE WRITINGS MENU, on the left, the sandwich's own idiom:
       a native <details> that opens with no script. Its list is written
       by tools/about_dist.py (build_writings) — the titles, and on a
       single page the scroll first. -->
  <details class="menu left">
    <summary>Writings</summary>
    <nav>
      <!-- __WMENU__ -->
    </nav>
  </details>
  <!-- DOORS_0 — THE SANDWICH (see about/). -->
  <details class="menu">
    <summary aria-label="menu"><span class="bars" aria-hidden="true"></span></summary>
    <nav>
      <!-- __ROUTES__ -->
    </nav>
  </details>
</div>

<main class="writings">
  <!-- __WRITINGS__ -->
</main>

</body>
</html>
````

### U3.2 — `web/menu.css`, the left menu's two rules

FIND (block 1 of 1 — count 1)
````css
   does not double it up */
.menu .followbox + .writebox { margin-top: 0; }

/* ── DOORS_4 — HIDDEN MEANS HIDDEN, EVERYWHERE IN THE SANDWICH ───────
   .pane dl sets display:grid, and an AUTHOR display beats the UA's
   [hidden]{display:none} at any specificity, because origin outranks
````
REPLACE
````css
   does not double it up */
.menu .followbox + .writebox { margin-top: 0; }

/* ── HOUSE_0 — A SECOND MENU, ON THE LEFT: the writings' own ──────────
   The same <details> idiom with a word for a summary instead of the bars.
   shared.css keeps the sandwich on the right edge with `.top .menu
   { margin-left: auto }` — a rule that reaches this menu too, and would
   float it into the middle of the bar; this puts it back on the left and
   drops its list from the left edge rather than the right. */
.top .menu.left { margin-left: 0; }
.menu.left > nav { left: 0; right: auto; }

/* ── DOORS_4 — HIDDEN MEANS HIDDEN, EVERYWHERE IN THE SANDWICH ───────
   .pane dl sets display:grid, and an AUTHOR display beats the UA's
   [hidden]{display:none} at any specificity, because origin outranks
````

### U3.3 — `tools/about_dist.py`
Already in place from U2.2 (`writing_slug`, `writings_menu_html`, `write_writings_page`); the
new template is what turns it on.

Witness:
```
python3 tools/about_dist.py 2>&1 | grep writings/          # "dist/writings/: the scroll, 3 page(s), writings.json"
find dist/writings -name index.html | sort                  # /, meaning/, text-page-doctrine/, the-mirror-in-the-sand/
grep -c 'href="../"' dist/writings/meaning/index.html       # ≥ 1 — scroll first on a single page
grep -c "Meaning" dist/writings/meaning/index.html          # the title, the tab, og:title — and NOT in the left menu's list
grep -o '<title>[^<]*' dist/writings/meaning/index.html     # <title>Meaning — the ever expanding board
grep -c '\.\./\.\./shared.css' dist/writings/meaning/index.html   # 1
python3 -c "import json;print([p['title'] for p in json.load(open('dist/writings/writings.json'))])"   # three titles, filename order
```
Commit: `HOUSE_0 U3 — the writings: a Writings menu on the left, the scroll and one page per text from one read, slugs from the filename`

---

## U4 — THE GALLERY: ZOOM_1, NO NUMBERS, INFO

### U4.1 — `web/collection/index.html`
Eleven blocks (the first is the `<head>` — ruling 2, folded in here by the diff). The largest replaces the ZOOM_0 block — from its opening comment
`/* ── ZOOM_0 — wheel, pinch, drag, double-tap` through the two `.zone` click listeners, ending
just before `document.getElementById('shut').addEventListener('click', () => shut(true));` —
with the plate's `plate()`, ZOOM_1 and the `pushed`-aware `shut()`. `stage` is bound above
`restRect` and is REUSED; a second top-level `const stage` is the SyntaxError ZOOM_0's own draft
carried (OPEN.md, ZOOM_0). After the edit: `node --check` on the extracted script.

FIND (block 1 of 11 — count 1)
````html
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="color-scheme" content="dark">
<title>collection — the ever expanding board</title>
<meta name="description" content="Paintings and sketches from the ever expanding board. The pawn is a vessel for projected intent.">
<meta property="og:title" content="collection — the ever expanding board">
<meta property="og:type" content="website">
<link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32'%3E%3Cpath fill='%23e8e6e0' d='M16 4a5 5 0 0 0-2.6 9.3c-.5 2.9-1.6 5.2-3.4 7.2h12c-1.8-2-2.9-4.3-3.4-7.2A5 5 0 0 0 16 4zM8 23h16v2.6c0 .8-.6 1.4-1.4 1.4H9.4C8.6 27 8 26.4 8 25.6z'/%3E%3C/svg%3E">
<link rel="stylesheet" href="../shared.css">
````
REPLACE
````html
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="color-scheme" content="dark">
<title>gallery — the ever expanding board</title>
<meta name="description" content="Jean Klein's paintings and sketches — the gallery of the ever expanding board.">
<meta property="og:title" content="gallery — the ever expanding board">
<meta property="og:type" content="website">
<link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32'%3E%3Cpath fill='%23e8e6e0' d='M16 4a5 5 0 0 0-2.6 9.3c-.5 2.9-1.6 5.2-3.4 7.2h12c-1.8-2-2.9-4.3-3.4-7.2A5 5 0 0 0 16 4zM8 23h16v2.6c0 .8-.6 1.4-1.4 1.4H9.4C8.6 27 8 26.4 8 25.6z'/%3E%3C/svg%3E">
<link rel="stylesheet" href="../shared.css">
````

FIND (block 2 of 11 — count 1)
````html
    font-size: clamp(20px, 1.9vw, 26px);
    font-weight: 400;
    letter-spacing: 0.02em;
  }
  .set-head .count {
    font-size: 14px;
    font-style: italic;
    color: var(--faint);
  }
  .set-head .note {
    font-size: 14px;
````
REPLACE
````html
    font-size: clamp(20px, 1.9vw, 26px);
    font-weight: 400;
    letter-spacing: 0.02em;
  }
  .set-head .note {
    font-size: 14px;
````

FIND (block 3 of 11 — count 1)
````html
  /* the wall takes a breath of the painting's own colour */
  .view { background: color-mix(in srgb, var(--tone, var(--bg)) 9%, var(--bg)); }
  .view[open] { transition: background 800ms ease; }
  .stage, .plate, .zone, .shut { position: relative; z-index: 1; }
  .zone, .shut { position: absolute; }
  .stage img { transition: opacity 260ms ease; }
  .stage img.out { opacity: 0; transition-duration: 140ms; }
  .fly {
````
REPLACE
````html
  /* the wall takes a breath of the painting's own colour */
  .view { background: color-mix(in srgb, var(--tone, var(--bg)) 9%, var(--bg)); }
  .view[open] { transition: background 800ms ease; }
  .stage, .plate, .shut { position: relative; z-index: 1; }
  .shut { position: absolute; }
  .stage img { transition: opacity 260ms ease; }
  .stage img.out { opacity: 0; transition-duration: 140ms; }
  .fly {
````

FIND (block 4 of 11 — count 1)
````html
    object-fit: contain;
  }

  .plate {
    display: flex;
    justify-content: space-between;
    align-items: baseline;
    gap: 24px;
    padding: 0 var(--edge) calc(var(--edge) * 0.6);
    font-size: 14px;
    color: var(--dim);
    font-variant-numeric: oldstyle-nums;
  }
  .plate b { font-weight: 430; color: var(--ink); }
  .plate .pos { color: var(--faint); }

  /* Not buttons. A transparent <button> earns a focus ring the moment
     the arrow keys convince the browser you are a keyboard user, and
     paints a rectangle across a control meant to be invisible. Keyboard
     users get arrows and Escape from the document handler instead. */
  .zone { position: absolute; top: 0; width: 34%; height: 100%; }
  .zone.prev { left: 0; cursor: w-resize; }
  .zone.next { right: 0; cursor: e-resize; }

  .shut {
    position: absolute;
````
REPLACE
````html
    object-fit: contain;
  }

  /* HOUSE_0 — THE PLATE SAYS NOTHING UNTIL ASKED. No number, no count:
     the picture stands alone. `info` shows only for a work that has an
     info line (the PAINTING_<n>.txt beside its master), and opens it. */
  .plate {
    display: flex;
    justify-content: space-between;
    align-items: baseline;
    gap: 24px;
    min-height: 1.6em;
    padding: 0 var(--edge) calc(var(--edge) * 0.6);
    font-size: 14px;
    color: var(--dim);
    font-variant-numeric: oldstyle-nums;
  }
  .plate .info { color: var(--ink); }
  .plate .more {
    margin-left: auto;
    border: 0; padding: 0; background: transparent;
    color: var(--dim); font: inherit; cursor: pointer;
  }
  .plate .more:hover, .plate .more[aria-expanded="true"] { color: var(--ink); }
  .plate .more:focus-visible { outline: 1px solid var(--ink); outline-offset: 4px; }
  /* hidden means hidden, whatever display a rule above gives (DOORS_4's lesson) */
  .view [hidden] { display: none !important; }

  .shut {
    position: absolute;
````

FIND (block 5 of 11 — count 1)
````html
    /* ZOOM_0 — the picture under the fingers. The transform is authored
       by the script below; touch-action none hands the stage's gestures
       to it (the page cannot scroll while the overlay is open anyway). */
    .stage { touch-action: none; overflow: hidden; }
    #big { transform-origin: 0 0; will-change: transform; }
    .view[data-zoomed] .zone { display: none; }   /* edges pan, not step, while zoomed */
    .view[data-zoomed] .stage { cursor: grab; }
  </style>
  <div class="plate">
    <span><b id="plate-title"></b> <span id="plate-meta"></span></span>
    <span class="pos" id="plate-pos"></span>
  </div>
  <div class="zone prev" aria-hidden="true"></div>
  <div class="zone next" aria-hidden="true"></div>
  <button class="shut" id="shut">close</button>
</div>
````
REPLACE
````html
    /* ZOOM_0 — the picture under the fingers. The transform is authored
       by the script below; touch-action none hands the stage's gestures
       to it (the page cannot scroll while the overlay is open anyway). */
    .view[open] { touch-action: none; }         /* ZOOM_1 — nor may a pinch that begins on the plate zoom the page */
    .stage { touch-action: none; overflow: hidden; }
    #big { transform-origin: 0 0; will-change: transform; }
    .stage[data-side="prev"] { cursor: w-resize; }   /* ZOOM_1 — the outer thirds step */
    .stage[data-side="next"] { cursor: e-resize; }
    .view[data-zoomed] .stage { cursor: grab; }      /* ...and pan while zoomed */
  </style>
  <div class="plate">
    <span class="info" id="plate-info" hidden></span>
    <button class="more" id="plate-more" type="button" aria-expanded="false" aria-controls="plate-info" hidden>info</button>
  </div>
  <button class="shut" id="shut">close</button>
</div>
````

FIND (block 6 of 11 — count 1)
````html

const view = document.getElementById('view');
const big = document.getElementById('big');
const pTitle = document.getElementById('plate-title');
const pMeta = document.getElementById('plate-meta');
const pPos = document.getElementById('plate-pos');

let current = -1;   /* index into tiles */
let opener = null;

function bigSrc(t) {
  const w = innerWidth * devicePixelRatio;
````
REPLACE
````html

const view = document.getElementById('view');
const big = document.getElementById('big');
const plateInfo = document.getElementById('plate-info');
const plateMore = document.getElementById('plate-more');
let infoOpen = false;   /* HOUSE_0 — the visitor's choice; it survives a step */

let current = -1;   /* index into tiles */
let opener = null;
let pushed = false; /* HOUSE_0 — did THIS opening push a history entry? A work opened
                       from its own address (#w<n> on arrival — the home page's hero
                       lands here) did not, and closing it must not history.back()
                       out of the gallery altogether: it closes onto the grid. */

function bigSrc(t) {
  const w = innerWidth * devicePixelRatio;
````

FIND (block 7 of 11 — count 1)
````html
  }
  if (opening) fly(t);
  big.alt = t.getAttribute('aria-label') || '';
  pTitle.textContent = t.dataset.title;
  pMeta.textContent = t.dataset.metaText ? '· ' + t.dataset.metaText : '';
  const sibs = bySet.get(t.dataset.set);
  pPos.textContent = (sibs.indexOf(t) + 1) + ' / ' + sibs.length;

  if (push) history.pushState({ w: t.dataset.n }, '', '#w' + t.dataset.n);
  else history.replaceState({ w: t.dataset.n }, '', '#w' + t.dataset.n);

  /* the neighbours are next; have them ready */
  [step_i(1), step_i(-1)].forEach(j => {
````
REPLACE
````html
  }
  if (opening) fly(t);
  big.alt = t.getAttribute('aria-label') || '';
  plate(t);

  if (push) { history.pushState({ w: t.dataset.n }, '', '#w' + t.dataset.n); pushed = true; }
  else { history.replaceState({ w: t.dataset.n }, '', '#w' + t.dataset.n); if (opening) pushed = false; }

  /* the neighbours are next; have them ready */
  [step_i(1), step_i(-1)].forEach(j => {
````

FIND (block 8 of 11 — count 1)
````html
  return tiles.indexOf(sibs[j]);
}

/* ── ZOOM_0 — wheel, pinch, drag, double-tap ─────────────────────────
   One transform on #big: translate(x,y) scale(s), origin 0 0, anchored
   so the point under the cursor (or the pinch midpoint) stays put.
   s=1 is the lightbox exactly as it was — zones step, arrows step; any
   s>1 hides the zones (they would steal the pan) and the drag pans.
   THE ARROWS ALWAYS STEP, zoomed or not, and every show() resets, so an
   arrow out of a zoom lands on the neighbour unzoomed. shut() resets too.
   No library, no state beyond {s,x,y}.

   `stage` is bound ABOVE (restRect reads it) and reused here. A second
   top-level `const stage` is a SyntaxError, and a SyntaxError in this one
   <script> takes the grid and the filters down with the lightbox. */
const zoom = { s: 1, x: 0, y: 0 };
const Z_MAX = 6;
function zoomApply() {
  big.style.transform = zoom.s === 1 ? '' :
    'translate(' + zoom.x + 'px,' + zoom.y + 'px) scale(' + zoom.s + ')';
  if (zoom.s > 1) view.setAttribute('data-zoomed', '');
  else view.removeAttribute('data-zoomed');
}
function zoomReset() { zoom.s = 1; zoom.x = 0; zoom.y = 0; zoomApply(); }
function zoomTo(s2, cx, cy) {                  /* keep (cx,cy) fixed on screen */
````
REPLACE
````html
  return tiles.indexOf(sibs[j]);
}

/* ── HOUSE_0 — the plate: nothing until asked ─────────────────────── */

function plate(t) {
  const info = t.dataset.info || '';
  plateInfo.textContent = info;
  plateMore.hidden = !info;
  plateInfo.hidden = !(info && infoOpen);
  plateMore.setAttribute('aria-expanded', infoOpen ? 'true' : 'false');
}
plateMore.addEventListener('click', () => {
  infoOpen = !infoOpen;
  if (current >= 0) plate(tiles[current]);
});

/* ── ZOOM_1 — the stage owns every gesture ───────────────────────────
   ZOOM_0's two step zones sat OVER the stage as siblings, 34% of the
   width each side, so a finger landing on either never reached the
   stage's pointer handlers: the pinch worked in the middle third only,
   and outside it the browser zoomed the page. The zones are gone. One
   element, one transform on #big — translate(x,y) scale(s), origin 0 0,
   anchored so the point under the cursor or the pinch midpoint stays put
   — and every gesture is read here:
     a tap in an outer third, at s=1     step (a tap is one pointer that
                                          lifts within TAP_MS and TAP_PX)
     two pointers                        pinch about their midpoint
     one pointer moving, s>1             pan
     wheel                               zoom about the cursor
     double-click, middle third (mouse)  1 <-> 2.5
     ArrowLeft / ArrowRight              step, always — every show() resets
     Escape                              peel the zoom first, then close
   A double-TAP is deliberately not a zoom: on glass the pinch is the
   zoom, and a double-tap's first tap has already stepped. THE PAN IS
   CLAMPED TO THE PICTURE — the contained rect inside #big, not the
   element's box, which carries the letterbox — so nothing is ever dragged
   off the stage (ZOOM_0's priced residual). Ceiling 6x. No library, no
   state beyond {s,x,y} and the pointers.

   `stage` is bound ABOVE (restRect reads it) and reused here. A second
   top-level `const stage` is a SyntaxError, and a SyntaxError in this one
   <script> takes the grid and the filters down with the lightbox. */
const zoom = { s: 1, x: 0, y: 0 };
const Z_MAX = 6;
const TAP_MS = 350, TAP_PX = 10;
const SIDE = 0.34;                            /* the outer thirds step */
function zoomApply() {
  big.style.transform = zoom.s === 1 ? '' :
    'translate(' + zoom.x + 'px,' + zoom.y + 'px) scale(' + zoom.s + ')';
  if (zoom.s > 1) view.setAttribute('data-zoomed', '');
  else view.removeAttribute('data-zoomed');
}
/* the picture's own rect inside #big at s=1: object-fit: contain keeps
   the ratio, so the letterbox is on one axis only */
function pictureRect() {
  const W = big.offsetWidth, H = big.offsetHeight;
  const r = (big.naturalWidth && big.naturalHeight) ? big.naturalWidth / big.naturalHeight : W / H;
  let w = W, h = W / r;
  if (h > H) { h = H; w = H * r; }
  return { x: (W - w) / 2, y: (H - h) / 2, w, h };
}
/* one axis: a picture smaller than the stage is centred; a larger one
   may not show the stage past either edge */
function clampAxis(cur, stageN, off, corner, extent) {
  const base = off + corner * zoom.s;        /* the picture's edge, before the translate */
  if (extent <= stageN) return (stageN - extent) / 2 - base;
  return Math.min(-base, Math.max(stageN - extent - base, cur));
}
function zoomClamp() {
  if (zoom.s === 1) { zoom.x = 0; zoom.y = 0; return; }
  const p = pictureRect();
  zoom.x = clampAxis(zoom.x, stage.clientWidth,  big.offsetLeft, p.x, p.w * zoom.s);
  zoom.y = clampAxis(zoom.y, stage.clientHeight, big.offsetTop,  p.y, p.h * zoom.s);
}
function zoomReset() { zoom.s = 1; zoom.x = 0; zoom.y = 0; zoomApply(); }
function zoomTo(s2, cx, cy) {                  /* keep (cx,cy) fixed on screen */
````

FIND (block 9 of 11 — count 1)
````html
  zoom.x = px - ox - ix * s2;
  zoom.y = py - oy - iy * s2;
  zoom.s = s2;
  if (zoom.s === 1) { zoom.x = 0; zoom.y = 0; }
  zoomApply();
}
stage.addEventListener('wheel', e => {
  if (current < 0) return;
  e.preventDefault();
  zoomTo(zoom.s * (e.deltaY < 0 ? 1.2 : 1 / 1.2), e.clientX, e.clientY);
}, { passive: false });
stage.addEventListener('dblclick', e => {
  e.preventDefault();
  zoomTo(zoom.s > 1 ? 1 : 2.5, e.clientX, e.clientY);
});
const pts = new Map();
let pinch0 = 0, pinchS = 1;
stage.addEventListener('pointerdown', e => {
  if (current < 0) return;
  pts.set(e.pointerId, { x: e.clientX, y: e.clientY });
  stage.setPointerCapture(e.pointerId);
  if (pts.size === 2) {
    const [a, b] = [...pts.values()];
    pinch0 = Math.hypot(a.x - b.x, a.y - b.y);
    pinchS = zoom.s;
  }
});
stage.addEventListener('pointermove', e => {
  const p = pts.get(e.pointerId);
  if (!p) return;
  if (pts.size === 2) {
    const q = { x: e.clientX, y: e.clientY };
    pts.set(e.pointerId, q);
    const [a, b] = [...pts.values()];
    const d = Math.hypot(a.x - b.x, a.y - b.y);
    if (pinch0 > 0) zoomTo(pinchS * d / pinch0, (a.x + b.x) / 2, (a.y + b.y) / 2);
  } else if (zoom.s > 1) {
    zoom.x += e.clientX - p.x;
    zoom.y += e.clientY - p.y;
    pts.set(e.pointerId, { x: e.clientX, y: e.clientY });
    zoomApply();
  }
});
['pointerup', 'pointercancel'].forEach(t => stage.addEventListener(t, e => {
  pts.delete(e.pointerId);
  if (pts.size < 2) pinch0 = 0;
}));

function shut(back) {
  if (current < 0) return;
````
REPLACE
````html
  zoom.x = px - ox - ix * s2;
  zoom.y = py - oy - iy * s2;
  zoom.s = s2;
  zoomClamp();
  zoomApply();
}
function sideOf(clientX) {                     /* which third of the stage a point is in */
  const r = stage.getBoundingClientRect();
  const fx = (clientX - r.left) / r.width;
  return fx < SIDE ? 'prev' : fx > 1 - SIDE ? 'next' : '';
}
stage.addEventListener('wheel', e => {
  if (current < 0) return;
  e.preventDefault();
  zoomTo(zoom.s * (e.deltaY < 0 ? 1.2 : 1 / 1.2), e.clientX, e.clientY);
}, { passive: false });
const pts = new Map();                         /* pointerId -> the pointer's own record */
let pinch0 = 0, pinchS = 1, lastType = 'mouse';
stage.addEventListener('pointerdown', e => {
  if (current < 0) return;
  lastType = e.pointerType;
  pts.set(e.pointerId, { x: e.clientX, y: e.clientY, x0: e.clientX, y0: e.clientY,
                         t0: e.timeStamp, moved: false, paired: false });
  stage.setPointerCapture(e.pointerId);
  if (pts.size === 2) {
    const [a, b] = [...pts.values()];
    a.paired = b.paired = true;                /* a pair never taps: two fingers down is not a step */
    pinch0 = Math.hypot(a.x - b.x, a.y - b.y);
    pinchS = zoom.s;
  }
});
stage.addEventListener('pointermove', e => {
  if (e.pointerType === 'mouse') stage.dataset.side = sideOf(e.clientX);   /* the cursor says what a click does */
  const p = pts.get(e.pointerId);
  if (!p) return;
  if (!p.moved && Math.hypot(e.clientX - p.x0, e.clientY - p.y0) > TAP_PX) p.moved = true;
  if (pts.size === 2) {
    p.x = e.clientX; p.y = e.clientY;
    const [a, b] = [...pts.values()];
    const d = Math.hypot(a.x - b.x, a.y - b.y);
    if (pinch0 > 0) zoomTo(pinchS * d / pinch0, (a.x + b.x) / 2, (a.y + b.y) / 2);
  } else if (zoom.s > 1) {
    zoom.x += e.clientX - p.x;
    zoom.y += e.clientY - p.y;
    p.x = e.clientX; p.y = e.clientY;
    zoomClamp();
    zoomApply();
  }
});
['pointerup', 'pointercancel'].forEach(t => stage.addEventListener(t, e => {
  const p = pts.get(e.pointerId);
  pts.delete(e.pointerId);
  if (pts.size < 2) pinch0 = 0;
  if (!p || t !== 'pointerup' || p.moved || p.paired || pts.size) return;
  if (e.timeStamp - p.t0 > TAP_MS || zoom.s !== 1) return;
  const side = sideOf(e.clientX);              /* a tap: the outer thirds step, the middle is quiet */
  if (side === 'prev') show(step_i(-1), false);
  else if (side === 'next') show(step_i(1), false);
}));
stage.addEventListener('dblclick', e => {
  e.preventDefault();
  /* a mouse, in the middle third: the outer thirds have just stepped
     twice, and on glass the pinch is the zoom */
  if (lastType !== 'mouse' || sideOf(e.clientX)) return;
  zoomTo(zoom.s > 1 ? 1 : 2.5, e.clientX, e.clientY);
});

function shut(back) {
  if (current < 0) return;
````

FIND (block 10 of 11 — count 1)
````html
  big.removeAttribute('src');
  zoomReset();                                  /* ZOOM_0 */
  current = -1;
  if (back) history.back();
  else history.replaceState(null, '', location.pathname);
  if (opener) opener.focus();
}
````
REPLACE
````html
  big.removeAttribute('src');
  zoomReset();                                  /* ZOOM_0 */
  current = -1;
  if (back && pushed) history.back();
  else history.replaceState(null, '', location.pathname);
  if (opener) opener.focus();
}
````

FIND (block 11 of 11 — count 1)
````html
  show(i, true);
}));

view.querySelector('.zone.prev').addEventListener('click', () => { if (zoom.s === 1) show(step_i(-1), false); });
view.querySelector('.zone.next').addEventListener('click', () => { if (zoom.s === 1) show(step_i(1), false); });
document.getElementById('shut').addEventListener('click', () => shut(true));

addEventListener('keydown', e => {
````
REPLACE
````html
  show(i, true);
}));

document.getElementById('shut').addEventListener('click', () => shut(true));

addEventListener('keydown', e => {
````

Witness:
```
sed -n '/<script>/,/<\/script>/p' web/collection/index.html | sed '1d;$d' > /tmp/coll.js && node --check /tmp/coll.js && echo parses
grep -c "\.zone\|plate-pos\|plate-title\|metaText" web/collection/index.html   # 0 — the class is gone; the ZOOM_1 banner's prose still names the zones it retired
grep -c "pushed" web/collection/index.html                                    # 4
```

### U4.2 — `tools/collection_dist.py`
The sidecars (`PAINTING_<n>.txt`, read by stem, `utf-8-sig`, whitespace folded to one line),
orphan warnings (a sidecar with no master; a `featured` number with no file), `data-info` in place
of `data-title`/`data-meta-text`, `Painting N` as the only alt, the count gone from
`section_markup`, `peek_entry` and `preview_tile` following. The `works` map is gone.

FIND (block 1 of 9 — count 1)
````python
# THE SOURCE OF TRUTH is a folder tree:
#
#   assets/collection/<folder>/PAINTING_<n>.jpg   the works
#   assets/collection/<folder>/set.json           optional, all fields optional:
#       { "label": "…", "note": "…", "paper": true,
#         "featured": [90, 112],
#         "works": { "107": { "title": "…", "meta": "oil on canvas · 2024" } } }
#
# "featured" works break the grid and take the full width alone — the
# page's pacing is authored here, one list per set, not derived.
#
# Folders sort by name; works sort by the number in the filename — the
# same key gallery.hpp uses, restated here. A work with no metadata
# still ships, titled "no. <n>": a missing field must never be able to
# drop a painting.
#
# THE PAGE IS WRITTEN, NOT FETCHED. web/collection/index.html is source
# with two placeholder regions; this script fills them with static
````
REPLACE
````python
# THE SOURCE OF TRUTH is a folder tree:
#
#   assets/collection/<folder>/PAINTING_<n>.jpg   the works
#   assets/collection/<folder>/PAINTING_<n>.txt   optional: ONE LINE, the
#       work's info as Jean writes it — "oil on canvas · 80 × 60 cm · 2024".
#       Shown behind the lightbox's `info` word, nowhere else. It sits
#       beside its master and moves with it (HOUSE_0); the masters are
#       gitignored and these are not, so the info is versioned.
#   assets/collection/<folder>/set.json           optional, all fields optional:
#       { "label": "…", "note": "…", "paper": true, "featured": [90, 112] }
#
# "featured" works break the grid and take the full width alone — the
# page's pacing is authored here, one list per set, not derived.
#
# Folders sort by name; works sort by the number in the filename — the
# same key gallery.hpp uses, restated here. A work with no info still
# ships; a missing sidecar must never be able to drop a painting. THE
# FOLDER IS THE TRUTH: a removed master leaves the page, and anything
# that still names it — a sidecar, a featured number — is a WARNING
# line, never a refused build (HOUSE_0). No number is shown on the page.
#
# THE PAGE IS WRITTEN, NOT FETCHED. web/collection/index.html is source
# with two placeholder regions; this script fills them with static
````

FIND (block 2 of 9 — count 1)
````python
                sys.exit(1)
            seen[n] = f

        sets.append({
            "folder": folder,
            "slug": slugify(meta.get("label", folder)),
            "label": meta.get("label", folder),
            "note": meta.get("note", ""),
            "paper": bool(meta.get("paper", False)),
            "work_meta": meta.get("works", {}) or {},
            "featured": set(int(x) for x in (meta.get("featured") or [])),
            "files": files,
        })
````
REPLACE
````python
                sys.exit(1)
            seen[n] = f

        # HOUSE_0 — THE INFO SIDECARS. PAINTING_<n>.txt beside PAINTING_<n>.jpg:
        # one line, read by stem, so a sidecar always finds its own master and
        # only that one. A sidecar with no master is a warning, not a refusal.
        info = {}
        stems = set(os.path.splitext(f)[0] for f in files)
        for f in sorted(os.listdir(path)):
            if not (f.startswith("PAINTING_") and f.lower().endswith(".txt")):
                continue
            stem = os.path.splitext(f)[0]
            with open(os.path.join(path, f), encoding="utf-8-sig") as fh:
                line = " ".join(fh.read().split())
            if stem not in stems:
                say("  warning  %s/%s names no master here — the line is not shown" % (folder, f))
            elif line:
                info[stem] = line

        featured = set(int(x) for x in (meta.get("featured") or []))
        for n in sorted(featured - set(seen)):
            say("  warning  %s: set.json features %d and no PAINTING_%d is here" % (folder, n, n))

        sets.append({
            "folder": folder,
            "slug": slugify(meta.get("label", folder)),
            "label": meta.get("label", folder),
            "note": meta.get("note", ""),
            "paper": bool(meta.get("paper", False)),
            "info": info,
            "featured": featured,
            "files": files,
        })
````

FIND (block 3 of 9 — count 1)
````python
    }


def tile_markup(set_slug, rec, meta, featured=False):
    n = rec["n"]
    title = meta.get("title") or ("no. %d" % n)
    meta_text = meta.get("meta", "")
    base = "%s/" % set_slug

    jpg_set = ", ".join("%s%s %dw" % (base, jn, sw)
````
REPLACE
````python
    }


def tile_markup(set_slug, rec, info, featured=False):
    n = rec["n"]
    base = "%s/" % set_slug

    jpg_set = ", ".join("%s%s %dw" % (base, jn, sw)
````

FIND (block 4 of 9 — count 1)
````python
    mid = next((v for v in rec["variants"] if v[0] >= 1280), rec["variants"][-1])
    first = rec["variants"][0]

    alt = title if not title.startswith("no. ") else "Painting %d" % n

    return (
        '<a class="work%(cls)s" href="%(full)s" style="--r:%(r).4f;--tone:%(tone)s"\n'
        '   data-n="%(n)d" data-set="%(set)s" data-w="%(w)d" data-h="%(h)d"\n'
        '   data-title="%(title)s" data-meta-text="%(meta)s"\n'
        '   data-mid="%(mid)s" data-full="%(full)s" aria-label="%(alt)s">\n'
        '  <div class="fill"></div>\n'
        '  <picture>\n'
````
REPLACE
````python
    mid = next((v for v in rec["variants"] if v[0] >= 1280), rec["variants"][-1])
    first = rec["variants"][0]

    # The number lives in aria-label, alt and the #w<n> address — wiring
    # and accessibility — and on no visible surface (HOUSE_0).
    alt = "Painting %d" % n

    return (
        '<a class="work%(cls)s" href="%(full)s" style="--r:%(r).4f;--tone:%(tone)s"\n'
        '   data-n="%(n)d" data-set="%(set)s" data-w="%(w)d" data-h="%(h)d"\n'
        '   data-info="%(info)s"\n'
        '   data-mid="%(mid)s" data-full="%(full)s" aria-label="%(alt)s">\n'
        '  <div class="fill"></div>\n'
        '  <picture>\n'
````

FIND (block 5 of 9 — count 1)
````python
        "full": base + full_jpg, "mid": base + mid[2],
        "r": rec["w"] / rec["h"], "tone": rec["tone"],
        "n": n, "set": set_slug, "w": rec["w"], "h": rec["h"],
        "title": esc(title), "meta": esc(meta_text), "alt": esc(alt),
        "avfset": esc(avf_set), "jpgset": esc(jpg_set),
        "src": base + first[2], "sizes": SIZES,
    }


def section_markup(s, tiles):
    count = len(tiles)
    note = ('\n    <span class="note">%s</span>' % esc(s["note"])) if s["note"] else ""
    return (
        '<section id="s-%(slug)s" class="set%(paper)s">\n'
        '  <div class="set-head">\n'
        '    <h2>%(label)s</h2>\n'
        '    <span class="count">%(count)d work%(pl)s</span>%(note)s\n'
        '  </div>\n'
        '  <div class="rows">\n%(tiles)s\n  </div>\n'
        '</section>'
    ) % {
        "slug": s["slug"], "paper": " paper" if s["paper"] else "",
        "label": esc(s["label"]), "count": count,
        "pl": "" if count == 1 else "s", "note": note,
        "tiles": "\n".join(tiles),
    }
````
REPLACE
````python
        "full": base + full_jpg, "mid": base + mid[2],
        "r": rec["w"] / rec["h"], "tone": rec["tone"],
        "n": n, "set": set_slug, "w": rec["w"], "h": rec["h"],
        "info": esc(info), "alt": esc(alt),
        "avfset": esc(avf_set), "jpgset": esc(jpg_set),
        "src": base + first[2], "sizes": SIZES,
    }


def section_markup(s, tiles):
    note = ('\n    <span class="note">%s</span>' % esc(s["note"])) if s["note"] else ""
    return (
        '<section id="s-%(slug)s" class="set%(paper)s">\n'
        '  <div class="set-head">\n'
        '    <h2>%(label)s</h2>%(note)s\n'
        '  </div>\n'
        '  <div class="rows">\n%(tiles)s\n  </div>\n'
        '</section>'
    ) % {
        "slug": s["slug"], "paper": " paper" if s["paper"] else "",
        "label": esc(s["label"]), "note": note,
        "tiles": "\n".join(tiles),
    }
````

FIND (block 6 of 9 — count 1)
````python
    return '<span>·</span>'.join(links)


def peek_entry(s, rec, meta, featured=False):
    """One work as the engine's menu sees it: the 640 rung, the tone, the
    title and the page address of the full work. Absolute paths, because
    the engine page lives at the root."""
    first = rec["variants"][0]
    return {
        # DOORS_4 — setLabel left with the sets line that was its only
        # reader (`set` and `featured` still have server-side ones).
        "n": rec["n"], "set": s["slug"],
        "title": meta.get("title") or ("no. %d" % rec["n"]),
        "tone": rec["tone"], "r": round(rec["w"] / rec["h"], 4),
        "src": "/collection/%s/%s" % (s["slug"], first[2]),
        "href": "/collection/#w%d" % rec["n"],
````
REPLACE
````python
    return '<span>·</span>'.join(links)


def peek_entry(s, rec, featured=False):
    """One work as the engine's menu sees it: the 640 rung, the tone, an
    alt and the page address of the full work. Absolute paths, because
    the engine page lives at the root."""
    first = rec["variants"][0]
    return {
        # DOORS_4 — setLabel left with the sets line that was its only
        # reader (`set` and `featured` still have server-side ones).
        "n": rec["n"], "set": s["slug"],
        "title": "Painting %d" % rec["n"],     # the pane tile's alt; no visible surface (HOUSE_0)
        "tone": rec["tone"], "r": round(rec["w"] / rec["h"], 4),
        "src": "/collection/%s/%s" % (s["slug"], first[2]),
        "href": "/collection/#w%d" % rec["n"],
````

FIND (block 7 of 9 — count 1)
````python
        return "data:font/woff2;base64," + base64.b64encode(fh.read()).decode()


def preview_tile(set_slug, rec, meta, uri, featured=False):
    n = rec["n"]
    title = meta.get("title") or ("no. %d" % n)
    alt = title if not title.startswith("no. ") else "Painting %d" % n
    return (
        '<a class="work%(cls)s" href="#w%(n)d" style="--r:%(r).4f;--tone:%(tone)s"\n'
        '   data-n="%(n)d" data-set="%(set)s" data-w="%(w)d" data-h="%(h)d"\n'
        '   data-title="%(title)s" data-meta-text="%(meta)s"\n'
        '   data-mid="%(uri)s" aria-label="%(alt)s">\n'
        '  <div class="fill"></div>\n'
        '  <img src="%(uri)s" loading="lazy" decoding="async" alt="%(alt)s"\n'
````
REPLACE
````python
        return "data:font/woff2;base64," + base64.b64encode(fh.read()).decode()


def preview_tile(set_slug, rec, info, uri, featured=False):
    n = rec["n"]
    alt = "Painting %d" % n
    return (
        '<a class="work%(cls)s" href="#w%(n)d" style="--r:%(r).4f;--tone:%(tone)s"\n'
        '   data-n="%(n)d" data-set="%(set)s" data-w="%(w)d" data-h="%(h)d"\n'
        '   data-info="%(info)s"\n'
        '   data-mid="%(uri)s" aria-label="%(alt)s">\n'
        '  <div class="fill"></div>\n'
        '  <img src="%(uri)s" loading="lazy" decoding="async" alt="%(alt)s"\n'
````

FIND (block 8 of 9 — count 1)
````python
        "cls": " full" if featured else "",
        "n": n, "r": rec["w"] / rec["h"], "tone": rec["tone"],
        "set": set_slug, "w": rec["w"], "h": rec["h"],
        "title": esc(title), "meta": esc(meta.get("meta", "")),
        "alt": esc(alt), "uri": uri,
    }
````
REPLACE
````python
        "cls": " full" if featured else "",
        "n": n, "r": rec["w"] / rec["h"], "tone": rec["tone"],
        "set": set_slug, "w": rec["w"], "h": rec["h"],
        "info": esc(info), "alt": esc(alt), "uri": uri,
    }
````

FIND (block 9 of 9 — count 1)
````python
        for f in s["files"]:
            n = extract_number(f)
            src_path = os.path.join(SRC, s["folder"], f)
            with Image.open(src_path) as im:
                if preview:
                    rec = build_work(im, src_path, "", n, write=False)
                    uri = data_uri(im, PREVIEW_EDGE, PREVIEW_Q)
                    tiles.append(preview_tile(
                        s["slug"], rec, s["work_meta"].get(str(n), {}), uri,
                        featured=n in s["featured"]))
                else:
                    rec = build_work(im, src_path, out_dir, n, write=True)
                    tiles.append(tile_markup(
                        s["slug"], rec, s["work_meta"].get(str(n), {}),
                        featured=n in s["featured"]))
                    peek.append(peek_entry(s, rec, s["work_meta"].get(str(n), {}),
                                           featured=n in s["featured"]))
                    for (_, _, jn, an) in rec["variants"]:
                        bytes_jpg += os.path.getsize(os.path.join(out_dir, jn))
                        bytes_avf += os.path.getsize(os.path.join(out_dir, an))
````
REPLACE
````python
        for f in s["files"]:
            n = extract_number(f)
            src_path = os.path.join(SRC, s["folder"], f)
            info = s["info"].get(os.path.splitext(f)[0], "")
            with Image.open(src_path) as im:
                if preview:
                    rec = build_work(im, src_path, "", n, write=False)
                    uri = data_uri(im, PREVIEW_EDGE, PREVIEW_Q)
                    tiles.append(preview_tile(
                        s["slug"], rec, info, uri, featured=n in s["featured"]))
                else:
                    rec = build_work(im, src_path, out_dir, n, write=True)
                    tiles.append(tile_markup(
                        s["slug"], rec, info, featured=n in s["featured"]))
                    peek.append(peek_entry(s, rec, featured=n in s["featured"]))
                    for (_, _, jn, an) in rec["variants"]:
                        bytes_jpg += os.path.getsize(os.path.join(out_dir, jn))
                        bytes_avf += os.path.getsize(os.path.join(out_dir, an))
````

Witness (a sidecar, an orphan with a BOM and CRLF, a featured orphan — all scratch, none committed):
```
printf 'oil on canvas · 80 × 60 cm · 2024\n' > assets/collection/a_1-14/PAINTING_2.txt
printf '\xef\xbb\xbfpen on paper\r\n' > assets/collection/a_1-14/PAINTING_13.txt      # no PAINTING_13 exists
python3 - <<'EOF'
import json; p='assets/collection/e_unfiled/set.json'; d=json.load(open(p)); d['featured']=[777]; json.dump(d,open(p,'w'),indent=2)
EOF
python3 tools/collection_dist.py 2>&1 | grep -i "warning\|REFUSE"
   #   warning  a_1-14/PAINTING_13.txt names no master here — the line is not shown
   #   warning  e_unfiled: set.json features 777 and no PAINTING_777 is here
grep -c 'class="count"' dist/collection/index.html                            # 0
grep -o 'data-info="[^"]*"' dist/collection/index.html | sort | uniq -c        # 53 × "", 1 × the line
grep -c "no\. " dist/collection/index.html                                   # 0
git checkout assets/collection/e_unfiled/set.json && rm assets/collection/a_1-14/PAINTING_13.txt
python3 tools/gates/collection_gate.py                                        # PASS
```
Then the gallery drive (appendix A) against a served `dist/`: **28/28** expected.

Commit: `HOUSE_0 U4 — the gallery: the stage owns every gesture (ZOOM_1), the pan is clamped, no number anywhere, info behind a word, sidecar lines beside the masters`

---

## U5 — THE LETTERBOX (`functions/api/message.js`)

FIND (block 1 of 2 — count 1)
````js
// the repo-root functions/ directory and serves it at /api/message —
// the only dynamic surface on the whole site, and it stays this small.
//
// Configuration (Pages → Settings → Environment variables):
//   RESEND_API_KEY   an api key from resend.com (free tier is plenty)
//   MESSAGE_TO       the inbox that receives the box's messages
//
// Unconfigured, it answers 501 and the page falls back to showing the
// direct address — the box never silently eats a message. The sender
// service is deliberately swappable: everything provider-specific is
// inside sendViaResend, and nothing else knows it exists.

const MAX_LEN = 8000;
````
REPLACE
````js
// the repo-root functions/ directory and serves it at /api/message —
// the only dynamic surface on the whole site, and it stays this small.
//
// Configuration (Pages → Settings → Environment variables, Production;
// a change takes effect on the NEXT deploy):
//   RESEND_API_KEY   an api key from resend.com (free tier is plenty)
//   MESSAGE_TO       the inbox that receives the box's messages
//   MESSAGE_FROM     optional — the sender, "the board <board@everexpandingboard.com>"
//                    once the domain is verified in Resend. Absent, the
//                    default below: Resend's unverified sender, which
//                    delivers ONLY to the Resend account's own address, so
//                    MESSAGE_TO must be that address until the domain is.
//
// Unconfigured, it answers 501 — the box never silently eats a message
// (HOUSE_0 read exactly that from the live site: 501 "unconfigured", the
// two variables unset). The sender service is deliberately swappable:
// everything provider-specific is inside sendViaResend, and nothing
// else knows it exists.

const MAX_LEN = 8000;
````

FIND (block 2 of 2 — count 1)
````js
      "Content-Type": "application/json",
    },
    body: JSON.stringify({
      from: "the board <onboarding@resend.dev>",
      to: env.MESSAGE_TO,
      subject,
      text,
````
REPLACE
````js
      "Content-Type": "application/json",
    },
    body: JSON.stringify({
      from: env.MESSAGE_FROM || "the board <onboarding@resend.dev>",
      to: env.MESSAGE_TO,
      subject,
      text,
````

Witness: `node -e "import('./functions/api/message.js').then(m=>console.log(Object.keys(m)))"` → `[ 'onRequestPost' ]`.

Commit: `HOUSE_0 U5 — the letterbox: the sender is configuration (MESSAGE_FROM), and the comment says what 501 means`

---

## U6 — THE FOLDERS ARE THE TRUTH; ATTIC

### U6.0 — the tag, BEFORE the deletions (ATTIC LAW)
```
git tag attic/house-0-main-setup HEAD      # the last commit that holds place_masters.py and MAIN/
git rm -q place_masters.py MAIN/MERGE.md MAIN/SETUP.md
```
The tag is pushed with the branch at U7.

### U6.1 — `.gitignore`
The two `assets/about/*.jp*g` lines die: the folder holds the doors' pictures now, tracked like `world.png`.

FIND (block 1 of 1 — count 1)
````
.wrangler/
out/

# website assets — masters live outside version control. The json beside
# them (site.json, set.json) stays tracked; the paintings do not.
assets/about/*.jpg
assets/about/*.jpeg
assets/collection/**/*.jpg
assets/collection/**/*.jpeg
assets/collection/**/*.png
````
REPLACE
````
.wrangler/
out/

# website assets — masters live outside version control. The json and the
# one-line info sidecars beside them (set.json, PAINTING_<n>.txt) stay
# tracked; the paintings do not. assets/about/ holds only site.json and the
# doors' pictures, which are tracked.
assets/collection/**/*.jpg
assets/collection/**/*.jpeg
assets/collection/**/*.png
````

### U6.2 — `tools/web_dist.py`, one comment (the only touch to the engine's script)

FIND (block 1 of 1 — count 1)
````python

    # ── WEBSITE_1 — THE ENGINE OWNS NAMES, NOT THE FOLDER ───────────
    # dist/ is shared ground: about/, collection/, fonts/ and shared.css
    # are the site's, written by their own pipelines (MAIN/MERGE.md).
    # This script deletes exactly what it writes — the names below, every
    # one already a constant in this file — and treats anything else as a
    # tenant it does not know. That is the whole agnosticism: the site's
````
REPLACE
````python

    # ── WEBSITE_1 — THE ENGINE OWNS NAMES, NOT THE FOLDER ───────────
    # dist/ is shared ground: about/, collection/, fonts/ and shared.css
    # are the site's, written by their own pipelines (tools/dist.py).
    # This script deletes exactly what it writes — the names below, every
    # one already a constant in this file — and treats anything else as a
    # tenant it does not know. That is the whole agnosticism: the site's
````

Witness:
```
git status --short          # D MAIN/MERGE.md  D MAIN/SETUP.md  D place_masters.py  M .gitignore  M tools/web_dist.py
git tag -l 'attic/*'        # attic/house-0-main-setup
grep -rn "MAIN/\|place_masters" --include=*.py --include=*.html --include=*.json --include=*.txt . | grep -v "^./docs/\|^./dist/"   # nothing
```
Commit: `HOUSE_0 U6 — the folders are the truth: paintings by number, a missing one a warning, assets/about holds doors and site.json only; the bootstrap and MAIN/ to the attic`

---

## U7 — THE GALLERY'S TITLE; COPY.MD; OPEN.MD; GATES; PUSH

### U7.1 — `web/collection/index.html`, the `<head>` (ruling 2)

*(These three lines are already in U4.1's first block — the diff generator folded the `<head>`
hunk into that file's blocks. Nothing further to do here; the witness is below.)*

### U7.2 — `docs/COPY.md`

FIND (block 1 of 4 — count 1)
````markdown
| the noscript paragraph | the `<noscript>` markup |
| pane heads and notes — The Board, Controls, Gallery, Writings, Home, Write me, Follow for more | each `.pane[data-pane="..."]`'s markup |
| The Board's messages | `.pane[data-pane="board"] .pane-note` — **Jean's** |
| the two platform lists | `.pane[data-pane="controls"] dl.ctl` — **words are placeholders, THE FACTS ARE THE TREE'S**: `direction/input.hpp` and `console.hpp`. Change words, never facts |
| the platform chips — *Mobile*, *Desktop* | the `.seg` markup. `CONTROLS_DEFAULT` pins which opens first |
| the live row — *Take a photo*, *Fullscreen*, *Sound: on/off* | `#ctlPhoto` / `#ctlFull` / `#ctlSound` |
| the postcard badge — *take this painting* / *take this photograph* (*· P* on a keyboard) | `window.T7_POSTCARD`'s `set`, in the first script. THE FACT (which picture, and when) is the tree's: `bodies/gallery.hpp` `pick_picture_before`. The *Postcard* row of each platform list is copy too |
| door-out labels — *Open Gallery in a new tab →*, *Read on the website →*, *Home, on the website →* | each pane's `a[data-out]` |
| *The writings did not answer.* / *The collection did not answer.* / *The website did not answer.* | `loadWritings()` / `loadPeek()` / `loadAbout()` catch arms |
| *Sending…* / *Sent. Thank you.* / *It did not go through* | the `#mini` submit handler |

## Menus on every page
- **the labels** — `web/routes.json` (`label`; the `id` is wiring, never
  words — `about` still means the page a menu calls **Home**, and
  `collection` the one it calls **Gallery**)
- **the write box's words** — `tools/routes.py` `write_box_html`
- **the follow box's summary** (*Follow for more*) — `tools/routes.py`
  `follow_box_html`. It shows on the SITE pages only; the engine has the
````
REPLACE
````markdown
| the noscript paragraph | the `<noscript>` markup |
| pane heads and notes — The Board, Controls, Gallery, Writings, Home, Write me, Follow for more | each `.pane[data-pane="..."]`'s markup |
| The Board's messages | `.pane[data-pane="board"] .pane-note` — **Jean's** |
| the two platform lists | `.pane[data-pane="controls"] dl.ctl` — **words are placeholders, THE FACTS ARE THE TREE'S**: `direction/input.hpp` and `console.hpp`. Change words, never facts. The *Postcard* row is off both lists since Jean's 7 Sep edit (HOUSE_0 removed the commented remains; the badge itself still teaches the take) |
| the platform chips — *Mobile*, *Desktop* | the `.seg` markup. `CONTROLS_DEFAULT` pins which opens first |
| the live row — *Take a photo*, *Fullscreen*, *Sound: on/off* | `#ctlPhoto` / `#ctlFull` / `#ctlSound` |
| the postcard badge — *take this painting* / *take this photograph* (*· P* on a keyboard) | `window.T7_POSTCARD`'s `set`, in the first script. THE FACT (which picture, and when) is the tree's: `bodies/gallery.hpp` `pick_picture_before`. The *Postcard* row of each platform list is copy too |
| door-out labels — *Open Gallery in a new tab →*, *Home →* (the Writings pane has none: it is the reader) | each pane's `a[data-out]` |
| *The writings did not answer.* / *The collection did not answer.* / *The website did not answer.* | `loadWritings()` / `loadPeek()` / `loadAbout()` catch arms |
| *Sending…* / *Sent. Thank you.* / *It did not go through* | the `#mini` submit handler |

## Menus on every page
- **the labels** — `web/routes.json` (`label`; the `id` is wiring, never
  words — `about` still means the page a menu calls **Home**, and
  `collection` the one it calls **Gallery**). ONE LIST, TWO SHELLS: a row
  without `side` renders in the engine's sandwich AND on every site page;
  `"side": "site"` or `"engine"` scopes it. Deleting a row deletes it from
  both — which is how Home left every site page for a day (HOUSE_0 put it
  back). A page never lists itself (DOORS_1).
- **the Writings menu** — the second `<details>` at the left of the top
  bar on `/writings/` and each `/writings/<slug>/`: its summary is the
  word *Writings* in `web/writings/index.html`; its list — the titles,
  and *scroll* first on a single page — is `tools/about_dist.py`
  `writings_menu_html`. *scroll* is Jean's word for the all-texts page.
- **the write box's words** — `tools/routes.py` `write_box_html`
- **the follow box's summary** (*Follow for more*) — `tools/routes.py`
  `follow_box_html`. It shows on the SITE pages only; the engine has the
````

FIND (block 2 of 4 — count 1)
````markdown
## The writings — `assets/writings/NN_slug.txt`
First line the title, blank line, then the body. Blank lines break
stanzas; line breaks inside a stanza are kept. **Filename order is page
order.** One new file is one new writing, on the page and in the engine's
pane both. `tools/about_dist.py` `build_writings` is the reader.

## The home page — `web/about/index.html`
*(the directory and the route id stay `about` — wiring; only the word a
visitor reads became **Home**)*
- the statement — `header.statement`
- the three door labels and blurbs — `.doors`
- **the world door's picture**: drop the file at `assets\about\world.jpg`
  (`.jpeg` and `.png` also read). Absent = a warning and a door without a
  picture, never a refused build.
- the day's hero and the site email — `assets/about/site.json`

## The gallery page — `web/collection/index.html`
- the lede — `header.lede`'s `h1` and `p`. **STILL PLACEHOLDER COPY**: the
````
REPLACE
````markdown
## The writings — `assets/writings/NN_slug.txt`
First line the title, blank line, then the body. Blank lines break
stanzas; line breaks inside a stanza are kept. **Filename order is page
order.** One new file is one new writing: on the scroll (`/writings/`),
on its own page (`/writings/<slug>/`) and in the engine's pane, all from
one read — `tools/about_dist.py` `build_writings`. **The slug is the
filename after the number**, hyphenated: `01_the_mirror_in_the_sand.txt`
→ `/writings/the-mirror-in-the-sand/`. Renumbering a file reorders it
and keeps its address; renaming the words after the number moves it.
(`00_text_page_doctrine.txt` therefore lives at `/writings/text-page-doctrine/`
— rename the file if the address should read otherwise.)

## The home page — `web/about/index.html`
*(the directory and the route id stay `about` — wiring; only the word a
visitor reads became **Home**)*
- the statement — none since HOUSE_0 (Jean took it off). To write one:
  `header.statement` and its rules, one block back from git
  (`git show 60c2fd3:web/about/index.html`); `about_dist` reads it into
  `about.json` for the engine's Home pane when it is there.
- the three door labels and blurbs — `.doors`: **the board · gallery ·
  writings**, lowercase, the menu's own names in the page's voice.
- **the doors' pictures**: `assets\about\world.jpg` for the board's,
  `assets\about\writings.jpg` for the writings' (`.jpeg` and `.png` also
  read). Absent = one build line and a door without a picture, never a
  refused build. Tracked.
- **the hero**: a quiet door — no caption, no number, no link under it; the
  picture opens itself on the gallery page. Its pools and the strip under
  the gallery door are `assets/about/site.json`, **as painting numbers**:
  `"hero": { "wide": [90, 92, 112], "tall": [50, 109] }, "strip": [2, 5, 7, 11]`.
  The files live where the collection keeps them (`assets/collection/<set>/`);
  a number no set holds is a warning and leaves the pool; only an empty
  `wide` pool refuses. Curate by hand.
- the site email — `assets/about/site.json` `email`

## The gallery page — `web/collection/index.html`
- the lede — `header.lede`'s `h1` and `p`. **STILL PLACEHOLDER COPY**: the
````

FIND (block 3 of 4 — count 1)
````markdown
  home. Jean's to write, or to move back into `assets/writings/`.
- **the tab, the bookmark and every share card** — `<title>`,
  `<meta property="og:title">` and `<meta name="description">` in that
  page's `<head>`. They still say *collection*. DOORS_4 ruling 5 scoped
  "Gallery" to *everywhere a menu speaks*, and a `<title>` is not a menu,
  so this was left deliberately — **Jean's naming gate**, listed here so
  it is findable rather than forgotten.
- set labels — each `assets/collection/*/set.json`

## The writings page chrome — `web/writings/index.html`
The `<title>` only. The texts come from `assets/writings/`.
````
REPLACE
````markdown
  home. Jean's to write, or to move back into `assets/writings/`.
- **the tab, the bookmark and every share card** — `<title>`,
  `<meta property="og:title">` and `<meta name="description">` in that
  page's `<head>`: *gallery* since HOUSE_0, one name with the menus.
- set labels — each `assets/collection/*/set.json` (`label`, `note`,
  `paper`, `featured`; nothing else is read)
- **a work's info** — ONE LINE in `assets/collection/<set>/PAINTING_<n>.txt`
  beside its master: *oil on canvas · 80 × 60 cm · 2024*, in Jean's own
  words. Shown behind the lightbox's *info* word and nowhere else; a work
  with no sidecar has no *info* word. **No number is on the page** —
  `Painting <n>` survives only in `alt`/`aria-label` and the `#w<n>`
  address. The sidecar moves with its master between set folders; one
  left behind is a build warning.
- the lightbox's words — *info* and *close* in the `.plate` / `.shut` markup

## The writings page chrome — `web/writings/index.html`
The `<title>` only. The texts come from `assets/writings/`.
````

FIND (block 4 of 4 — count 1)
````markdown
*collection* and missing Writings entirely. Wording is Jean-gated, like
the card's.

## The colophon — `web/about/index.html`'s `<footer>`
*set in Newsreader · no trackers*. It is the only footer left on the
site (ruling 8: the name appears once, and its home is the engine's
wordmark).

## Not copy, and not to be edited as copy
`docs/`, `audit/`, every comment in `src/`, and the ledgers. The
`data-route` / `data-pane` / `data-platform` ids are wiring.
````
REPLACE
````markdown
*collection* and missing Writings entirely. Wording is Jean-gated, like
the card's.

## Not copy, and not to be edited as copy
`docs/`, `audit/`, every comment in `src/`, and the ledgers. The
`data-route` / `data-pane` / `data-platform` ids are wiring.
````

### U7.3 — `docs/OPEN.md` — the entry at the top of the register

FIND (block 1 of 1 — count 1)
````markdown
# OPEN — the register of open state
One line per item: what · origin (sha or doc) · what unblocks it.
This file is the ONLY home of open/parked state. When an item closes, its line dies.

## ZOOM_0 — THE COLLECTION PAGE LEARNS THE PINCH (landed on master; Jean's gates open)
````
REPLACE
````markdown
# OPEN — the register of open state
One line per item: what · origin (sha or doc) · what unblocks it.
This file is the ONLY home of open/parked state. When an item closes, its line dies.

## HOUSE_0 — THE HOUSE THE BOARD STANDS IN (on `claude/house-0`; Jean's gates open)

The website round after Jean's 7 Sep hand edits: every room connects, the
letterbox opens, the walls answer *info*. Site-only — the shell loses
three dead comments and nothing else; no `src/` file moves; `web_dist.py`
changes one comment. Rehearsed end to end in a scratch dist and driven in
headless Chromium (touch and mouse); the drives are in the handoff.

| ruling | where it lives now |
|---|---|
| Home is the page's one name, on both shells | `web/routes.json` — the `about` row is back, no `side`; the engine's Home pane lives again |
| The hero says nothing under it; it is a quiet door into its painting | `web/about/index.html` — `.hero-plate` and `#hero-title` are gone; the `<a>` stays |
| The doors carry the menu's names in the page's voice: *the board · gallery · writings* | `.doors` |
| The writings door takes a picture like the board's | `assets/about/writings.jpg` → `about_dist` `build_door_image(name)` (the old `build_world`, by name) |
| No statement, no footer (Jean's edit, honoured) | the template; `about_dist` reads the statement only if a header is there |
| Writings: a *Writings* menu on the left; the scroll and one page per text | `web/writings/index.html` (one template), `about_dist` `writing_slug` / `writings_menu_html` / `write_writings_page`; `web/menu.css` `.menu.left` |
| A text's address is its filename after the number | `writing_slug`: `01_the_mirror_in_the_sand.txt` → `/writings/the-mirror-in-the-sand/` |
| ZOOM_1 — the stage owns every gesture; the pan is clamped to the picture | `web/collection/index.html` — the two `.zone`s are gone; `pictureRect` / `clampAxis` / `zoomClamp` |
| No number on the gallery page: not *no. N*, not *1 / 13*, not *13 works* | `collection_dist` `section_markup` / `tile_markup`; the plate |
| *info* — a work's one line, behind a word, only where there is one | `assets/collection/<set>/PAINTING_<n>.txt` → `load_sets` → `data-info` → `plate()` |
| Closing a work opened from its own address lands on the grid | `pushed` in `show()` / `shut()` |
| The letterbox's sender is configuration | `functions/api/message.js` `MESSAGE_FROM` |
| The folders are the truth: paintings by number, a missing one a warning | `site.json` `hero` / `strip` as numbers; `about_dist` `find_master`; `collection_dist` warns on an orphan sidecar or an orphan `featured` |
| `assets/about/` holds `site.json` and the doors' pictures, nothing else | `.gitignore` (the two about lines gone), `README.txt` |
| `site.json` requires what is read: `email`, `hero`; `strip` optional | `load_site` — `authors` and `links` are gone |
| The gallery page's tab and share card say *gallery* | `<title>`, `og:title`, `description` |
| The bootstrap and `MAIN/` are attic | tag `attic/house-0-main-setup`; `docs/COPY.md` carries what they said that is still true |

**THE TWO DIAGNOSES, READ FROM THE LIVE SITE, NOT GUESSED.**

- **Write me answered `501 {"error":"unconfigured"}`** to a real POST: the
  function is deployed and `RESEND_API_KEY` / `MESSAGE_TO` are not set on
  the Pages project. No code was wrong. Jean's dashboard work (below); the
  page needs a redeploy after the variables land.
- **The pinch was dead outside the middle third.** ZOOM_0's step zones
  were absolutely-positioned SIBLINGS of the stage, 34% each side; a
  finger landing on one never reached the stage's pointer handlers, and
  on the zones the browser zoomed the page. Witnessed on the ZOOM_0 page
  in Chromium at 390 px: a pinch starting at 15%/85% of the width →
  `zoom.s = 1.000`; the same spread in the middle third → `1.500`; the
  live zone was **125 px wide**. On the HOUSE_0 page the outer-third
  pinch reads `1.286` (the spread's own ratio, 0.9/0.7).

**THE ONE THING THE ROUND CORRECTED WITHOUT ASKING.** A work opened from
its own `#w<n>` address — the home page's hero now lands exactly there —
closed with `history.back()`, which left the gallery for whatever page
came before. `pushed` records whether THIS opening pushed an entry; if it
did not, close replaces the state and shows the grid.

### What Jean does (in parallel; nothing in the tree waits on it)
1. resend.com — an account and an API key. Signing up **with**
   `jean@everexpandingboard.com` (Email Routing forwards the verification)
   lets Resend's unverified sender deliver there with no domain work.
   Otherwise verify the domain (three DNS records in Cloudflare; they do
   not collide with Email Routing's MX) and set `MESSAGE_FROM` to
   `the board <board@everexpandingboard.com>`.
2. Pages → `7t` → Settings → Environment variables → Production:
   `RESEND_API_KEY`, `MESSAGE_TO=jean@everexpandingboard.com` (and
   `MESSAGE_FROM` if the domain is verified). Then a deploy.
3. PowerShell: `Invoke-RestMethod -Method Post -Uri https://everexpandingboard.com/api/message -ContentType application/json -Body '{"message":"test"}'`
   → `ok True` and a mail. 501 = variables missing; 502 = Resend refused
   (the unverified sender only delivers to the account's own address).
4. `assets\about\writings.jpg` — whenever; absence costs one build line.
5. `del assets\about\PAINTING_*.jpeg` — the hero copies the retired
   bootstrap placed; the heroes are found in the collection now.
6. One line per painting he wants to say something about:
   `assets\collection\<set>\PAINTING_<n>.txt`.

### Residuals — HOUSE_0
- **Jean's browser is the visual gate**, the phone the real one: the pinch
  from anywhere on the picture; the tap in an outer third steps; the pan
  stops at the picture's edge; *info* opens and follows a step; the
  Writings menu on a phone; the doors' lowercase; the empty air where the
  statement was.
- **`/about/` is the address of the page called Home** (path, directory,
  builder, route id, `about.json`). Priced: ~12 files, one 301, no visible
  change; parked until the word is final. `/main` still forwards there.
- **The doctrine sentence has four homes** — the first writing's title, the
  gallery lede `h1`, the writings door blurb, one `<meta>` on the engine
  page. Copy, Jean's; one home when he chooses it.
- **The site never says his name** except in two `<meta>` descriptions.
  Copy, Jean's.
- **`00_text_page_doctrine.txt` lives at `/writings/text-page-doctrine/`.**
  A rename of the words after `00_` moves it; his call.
- **`/writings/<slug>/` pages carry no explicit `Cache-Control`** — Pages'
  default revalidates them (ETag); `web_dist.py`'s writer rules `/writings/`
  only. Untouched on purpose: that script is the engine's, and this branch
  keeps out of it. One line (`/writings/*`) if the default ever bites.
- **Two sets that hold the same number** would give `find_master` the first
  by set order; `collection_dist` allows the collision across folders and
  the hero would pick without saying which. Unlikely by construction
  (numbers are the works' own); left as authored.
- **The 404 page's menu is hand-kept** (`web_dist.py` `NOT_FOUND_PAGE`); its
  four links still name the four rooms correctly. A route added later must
  be added there by hand — DOORS_4's note stands.

## ZOOM_0 — THE COLLECTION PAGE LEARNS THE PINCH (landed on master; Jean's gates open)
````

### U7.4 — gates, the site drive, the push
```
python3 tools/collection_dist.py >/dev/null && python3 tools/about_dist.py >/dev/null && python3 tools/gates/collection_gate.py   # PASS
grep -o '<title>[^<]*' web/collection/index.html                        # <title>gallery — the ever expanding board
python3 tools/gates/shell_gate/run.py                                     # GREEN — organ_panel.js is untouched
for f in $(git diff --name-only origin/master..HEAD); do [ -f "$f" ] && { grep -c $'\r' "$f" | grep -qv '^0$' && echo "CRLF in $f"; [ "$(head -c3 "$f" | od -An -tx1 | tr -d ' ')" = efbbbf ] && echo "BOM in $f"; }; done; echo "LF-only, no BOM"
```
The engine gates (glaw2, console, score, the censuses) have nothing to look at — no `src/` file
and no `world.wgsl` moved; run any one to show it, not all.

Then appendix B against a served `dist/`: **45/45** expected. Both drives need a browser; if the
sandbox has none, skip them, say so, and Jean's browser is the gate it always was.

```
git push -u origin claude/house-0 --tags
```
Commit (before the push): `HOUSE_0 U7 — the gallery page says gallery; COPY.md current; the register written`

---

## U8 — THE MERGE (ONLY WHEN JEAN SAYS SO)

```
git fetch origin master                                   # P9
git checkout claude/house-0 && git merge origin/master    # master INTO the branch first
```
Expected conflicts: `docs/OPEN.md` (both sides prepend an entry — keep both, HOUSE_0's above the
newer engine entry, the head's four lines once); possibly `web/index.html` (three deleted comments
against whatever the engine changed — keep both intents). Rebuild the site half, `collection_gate`,
re-run the two drives. Then Jean builds `python tools\dist.py` on the branch, looks (his gate below),
and says merge:
```
git checkout master && git merge --ff-only claude/house-0    # or a merge commit if master moved again
git push origin master
git push origin --delete claude/house-0 && git branch -d claude/house-0   # ATTIC LAW
git rm docs/HANDOFFS/HOUSE_0.md                              # the directory exists only while work is open
```
OPEN.md's HOUSE_0 head line becomes *(landed on master; Jean's gates open)*. One deploy.

## JEAN'S GATE, AFTER `python tools\dist.py` AND `npx wrangler pages dev dist`

- Every site page's sandwich: Home from Gallery and Writings; The Board; Gallery/Writings; no page
  lists itself. The engine's sandwich has Home again and its pane shows the day's painting.
- Home page: the picture, nothing under it, click it and it opens on the gallery page; three
  lowercase doors; no statement, no footer; `assets\about\writings.jpg` lights the third door.
- Writings: *Writings* top-left, the sandwich top-right; the list on the scroll; a title opens its
  own page; on it, *scroll* first; the tab reads the text's title.
- Gallery on the phone: pinch anywhere on the picture; drag stops at the picture's edge; tap the
  outer thirds to step; Esc/close; *info* only on a work with a `.txt` beside it, and it stays
  open across a step. No number anywhere.
- After the Pages variables and a deploy: the message door sends (`ok True` from the PowerShell
  line in OPEN.md).
- Then: `del assets\about\PAINTING_*.jpeg` — the retired bootstrap's hero copies.

## WHAT THIS DOCUMENT CANNOT SEE (P16)

The engine's own build and `web_dist.py`'s run (no artifacts in the sandbox; the script is
untouched but for one comment, and its five-marker refusal was checked on the source). Master's
movement since `60c2fd39c78e2f3fe3f579c76918f58caea255e2` (U0's blobs answer it). Whether CC's sandbox has Chromium for the drives.
The look of the doors' column with only one picture, and of the empty air where the statement
was — Jean's eye. iOS Safari's `dblclick` on a double-tap (guarded: only a mouse's double-click
zooms) and its pointer capture in a pinch (the ZOOM_0 residual stands: the phone is the real gate).

---

## APPENDIX A — the gallery drive (playwright; run with `dist/` served at :8765)

````python
from playwright.sync_api import sync_playwright
import json, math, sys
B='http://localhost:8765'
res=[]
def ok(name, cond, detail=''):
    res.append((name, bool(cond), detail))
with sync_playwright() as p:
    b=p.chromium.launch()
    ctx=b.new_context(viewport={'width':390,'height':844}, device_scale_factor=2, has_touch=True, is_mobile=True)
    pg=ctx.new_page()
    errors=[]; pg.on('pageerror', lambda e: errors.append(str(e)))
    pg.goto(B+'/collection/'); pg.wait_for_timeout(500)
    ok('page load: no JS errors', not errors, errors)
    ok('no count spans', pg.locator('.set-head .count').count()==0)
    ok('no visible "no. " anywhere', pg.evaluate("document.body.innerText.includes('no. ')")==False)
    pg.goto('about:blank'); pg.goto(B+'/collection/#w2'); pg.wait_for_timeout(700)
    ok('overlay open', pg.evaluate("document.getElementById('view').hasAttribute('open')"))
    ok('plate shows nothing but the info word', pg.evaluate("document.querySelector('.plate').innerText.trim()")=='info')
    ok('info line hidden until asked', pg.evaluate("document.getElementById('plate-info').hidden"))
    pg.click('#plate-more'); pg.wait_for_timeout(100)
    ok('info opens with the sidecar line', pg.evaluate("document.getElementById('plate-info').hidden==false && document.getElementById('plate-info').textContent")=='oil on canvas · 80 × 60 cm · 2024')
    cdp=ctx.new_cdp_session(pg)
    def touch(kind, pts):
        cdp.send('Input.dispatchTouchEvent', {'type': kind, 'touchPoints': [{'x':x,'y':y,'id':i} for i,(x,y) in enumerate(pts)]})
    st=pg.evaluate("(()=>{const r=document.querySelector('.stage').getBoundingClientRect();return {l:r.left,t:r.top,w:r.width,h:r.height}})()")
    cx, cy = st['l']+st['w']/2, st['t']+st['h']/2
    touch('touchStart', [(st['l']+st['w']*0.9, cy)]); pg.wait_for_timeout(60); touch('touchEnd', [])
    pg.wait_for_timeout(400)
    ok('tap right third steps to the next work', pg.evaluate("location.hash")=='#w3', pg.evaluate("location.hash"))
    ok('a work without info hides the button and the line', pg.evaluate("document.getElementById('plate-more').hidden && document.getElementById('plate-info').hidden"))
    touch('touchStart', [(st['l']+st['w']*0.1, cy)]); pg.wait_for_timeout(60); touch('touchEnd', [])
    pg.wait_for_timeout(400)
    ok('tap left third steps back', pg.evaluate("location.hash")=='#w2')
    ok('info state survived the round trip (open again on 2)', pg.evaluate("!document.getElementById('plate-info').hidden"))
    touch('touchStart', [(cx, cy)]); pg.wait_for_timeout(60); touch('touchEnd', []); pg.wait_for_timeout(400)
    ok('tap in the middle third is quiet', pg.evaluate("location.hash")=='#w2' and pg.evaluate("zoom.s")==1)
    a=(st['l']+st['w']*0.15, cy); bb=(st['l']+st['w']*0.85, cy)
    touch('touchStart', [a, bb]); pg.wait_for_timeout(30)
    a2=(st['l']+st['w']*0.05, cy); b2=(st['l']+st['w']*0.95, cy)
    touch('touchMove', [a2, b2]); pg.wait_for_timeout(30)
    s_mid=pg.evaluate("zoom.s")
    touch('touchEnd', [])
    pg.wait_for_timeout(100)
    ok('pinch that starts in the outer thirds zooms (ZOOM_0 dead zone)', abs(s_mid-0.9/0.7)<0.02, s_mid)
    ok('pinch did not step', pg.evaluate("location.hash")=='#w2')
    touch('touchStart', [(cx-40,cy),(cx+40,cy)]); touch('touchMove', [(cx-140,cy),(cx+140,cy)]); touch('touchEnd', [])
    pg.wait_for_timeout(50)
    touch('touchStart', [(cx,cy)])
    for i in range(1,21): touch('touchMove', [(cx+i*30, cy)])
    touch('touchEnd', [])
    pg.wait_for_timeout(50)
    rect=pg.evaluate("(()=>{const r=big.getBoundingClientRect(), s=document.querySelector('.stage').getBoundingClientRect(); const p=pictureRect(); return {s:zoom.s, picL:r.left + p.x*zoom.s - s.left, picR:r.left + (p.x+p.w)*zoom.s - s.left, sw:s.width}})()")
    ok('pan clamp: the picture cannot leave the stage (left edge ≤ 0)', rect['picL']<=0.5, rect)
    ok('pan clamp: right edge ≥ stage width', rect['picR']>=rect['sw']-0.5, rect)
    ok('a pan did not step', pg.evaluate("location.hash")=='#w2')
    pg.keyboard.press('Escape'); pg.wait_for_timeout(50)
    ok('Esc peels the zoom first', pg.evaluate("zoom.s")==1 and pg.evaluate("document.getElementById('view').hasAttribute('open')"))
    pg.keyboard.press('Escape'); pg.wait_for_timeout(100)
    ok('second Esc closes', not pg.evaluate("document.getElementById('view').hasAttribute('open')"))
    ok('phone: no JS errors', not errors, errors)
    ctx.close()
    ctx=b.new_context(viewport={'width':1280,'height':800})
    pg=ctx.new_page(); errors=[]; pg.on('pageerror', lambda e: errors.append(str(e)))
    pg.goto(B+'/collection/#w2'); pg.wait_for_timeout(700)
    st=pg.evaluate("(()=>{const r=document.querySelector('.stage').getBoundingClientRect();return {l:r.left,t:r.top,w:r.width,h:r.height}})()")
    cx, cy = st['l']+st['w']/2, st['t']+st['h']/2
    pg.mouse.move(cx,cy); pg.mouse.wheel(0,-100); pg.wait_for_timeout(50)
    ok('wheel zooms', abs(pg.evaluate("zoom.s")-1.2)<1e-6)
    pg.keyboard.press('Escape'); pg.wait_for_timeout(50)
    pg.mouse.dblclick(cx,cy); pg.wait_for_timeout(100)
    ok('mouse double-click in the middle third zooms to 2.5', pg.evaluate("zoom.s")==2.5)
    pg.mouse.dblclick(cx,cy); pg.wait_for_timeout(100)
    ok('...and back to 1', pg.evaluate("zoom.s")==1)
    pg.mouse.move(st['l']+st['w']*0.1, cy); pg.wait_for_timeout(30)
    ok('cursor side attribute on the left third', pg.evaluate("document.querySelector('.stage').dataset.side")=='prev')
    pg.mouse.click(st['l']+st['w']*0.9, cy); pg.wait_for_timeout(400)
    ok('mouse click right third steps', pg.evaluate("location.hash")=='#w3')
    pg.mouse.dblclick(st['l']+st['w']*0.9, cy); pg.wait_for_timeout(500)
    ok('double-click on a side: two steps, no zoom', pg.evaluate("location.hash")=='#w5' and pg.evaluate("zoom.s")==1, pg.evaluate("location.hash"))
    pg.keyboard.press('ArrowLeft'); pg.wait_for_timeout(400)
    ok('arrow steps', pg.evaluate("location.hash")=='#w4')
    ok('desktop: no JS errors', not errors, errors)
    ctx.close(); b.close()
for n,c,d in res: print(('PASS ' if c else 'FAIL ')+n+('' if c else '   <- '+str(d)))
print(sum(1 for r in res if r[1]),'/',len(res))
````

## APPENDIX B — the site drive

````python
from playwright.sync_api import sync_playwright
import os, re, urllib.parse
B='http://localhost:8765'; D='dist'
res=[]
def ok(n,c,d=''): res.append((n,bool(c),d))
# static link check over every built site page
pages=[]
for root,_,files in os.walk(D):
    for f in files:
        if f=='index.html' and 'collection' not in root.split(os.sep)[-1:]: pages.append(os.path.join(root,f))
pages=[p for p in pages if '/collection/' not in p.replace(D,'')]  # the gate covers that page
bad=[]
for p in pages:
    html=open(p,encoding='utf-8').read()
    for m in re.finditer(r'(?:href|src)="([^"]+)"', html):
        ref=m.group(1)
        if re.match(r'^(https?:|data:|#|mailto:|\.\./api/|\.\./\.\./api/)',ref) or ref.endswith('api/message'): continue
        path=os.path.normpath(os.path.join(os.path.dirname(p), ref.split('#')[0]))
        if os.path.isdir(path): path=os.path.join(path,'index.html')
        if path==os.path.join(D,'index.html'): continue   # the engine's shell: not in a site-only dist
        if not os.path.exists(path): bad.append((p.replace(D,''),ref))
ok('every local href/src on %d site pages resolves' % len(pages), not bad, bad)
with sync_playwright() as p:
    b=p.chromium.launch()
    for name,vp in (('desk',{'width':1280,'height':800}),('phone',{'width':390,'height':844})):
        ctx=b.new_context(viewport=vp); pg=ctx.new_page(); errs=[]; pg.on('pageerror', lambda e: errs.append(str(e)))
        pg.goto(B+'/about/'); pg.wait_for_timeout(500)
        t=pg.evaluate("document.body.innerText")
        ok(name+': home — no stray comment text', 'DOORS_3' not in t and '-->' not in t)
        ok(name+': home — nothing under the hero (no "no." and no "the collection")', 'no. ' not in t and 'the collection' not in t)
        ok(name+': home — doors read the board / gallery / writings', pg.evaluate("[...document.querySelectorAll('.door .go')].map(e=>e.textContent)")==['the board','gallery','writings'])
        ok(name+': home — hero links into the gallery at a #w', re.match(r'.*/collection/#w\d+$', pg.evaluate("document.getElementById('hero').href")) is not None)
        ok(name+': home — no footer', pg.locator('footer').count()==0)
        menu=pg.evaluate("[...document.querySelectorAll('.top .menu > nav > a')].map(a=>a.textContent)")
        ok(name+': home menu — Gallery, Writings, The Board; no Home (a menu does not list where you are)', menu==['Gallery','Writings','The Board'], menu)
        for path, expect in (('/collection/', ['Writings','Home','The Board']), ('/writings/', ['Gallery','Home','The Board']), ('/writings/meaning/', ['Gallery','Writings','Home','The Board'])):
            pg.goto(B+path); pg.wait_for_timeout(400)
            menu=pg.evaluate("[...document.querySelectorAll('.top .menu:not(.left) > nav > a')].map(a=>a.textContent)")
            ok(name+': %s menu — %s' % (path, ' / '.join(expect)), menu==expect, menu)
            home=pg.evaluate("(()=>{const a=[...document.querySelectorAll('.top .menu:not(.left) > nav > a')].find(a=>a.textContent=='Home'); return a? new URL(a.href).pathname : null})()")
            ok(name+': %s Home points at /about/' % path, home=='/about/', home)
        pg.goto(B+'/writings/'); pg.wait_for_timeout(400)
        wm=pg.evaluate("[...document.querySelectorAll('.menu.left > nav > a')].map(a=>[a.textContent, new URL(a.href).pathname])")
        ok(name+': writings menu on the scroll — three titles, no scroll entry', [x[0] for x in wm]==['The pawn is a vessel for projected intent.','The Mirror in the Sand','Meaning'] and all(x[1].startswith('/writings/') for x in wm), wm)
        ok(name+': the scroll shows all three', pg.locator('main article').count()==3)
        # the left menu sits left, the sandwich right
        pos=pg.evaluate("(()=>{const l=document.querySelector('.menu.left summary').getBoundingClientRect(), r=document.querySelector('.top .menu:not(.left) summary').getBoundingClientRect(); return {l:l.left, lr:l.right, r:r.left, w:innerWidth}})()")
        ok(name+': Writings menu left of centre, sandwich right of centre', pos['lr']<pos['w']/2 and pos['r']>pos['w']/2, pos)
        pg.click('.menu.left summary'); pg.wait_for_timeout(200)
        nav=pg.evaluate("(()=>{const r=document.querySelector('.menu.left > nav').getBoundingClientRect(); return {l:r.left, r:r.right, w:innerWidth}})()")
        ok(name+': the open Writings list drops from the left and stays on screen', nav['l']>=0 and nav['r']<=nav['w'], nav)
        pg.click('.menu.left > nav > a:nth-child(3)'); pg.wait_for_timeout(500)
        ok(name+': a title opens its own page', pg.url.endswith('/writings/meaning/'), pg.url)
        ok(name+': the single page: one article, tab titled by the text', pg.locator('main article').count()==1 and pg.title()=='Meaning — the ever expanding board', pg.title())
        wm=pg.evaluate("[...document.querySelectorAll('.menu.left > nav > a')].map(a=>[a.textContent, new URL(a.href).pathname])")
        ok(name+': on a single page the menu starts with scroll and omits this page', wm[0]==['scroll','/writings/'] and 'Meaning' not in [x[0] for x in wm], wm)
        ok(name+': single page renders with the stylesheet (Newsreader applied)', 'Newsreader' in pg.evaluate("getComputedStyle(document.querySelector('main h2')).fontFamily"))
        pg.click('.menu.left summary'); pg.click('.menu.left > nav > a:first-child'); pg.wait_for_timeout(400)
        ok(name+': scroll takes you back to /writings/', pg.url.endswith('/writings/'))
        ok(name+': no JS errors across the site', not errs, errs)
        ctx.close()
    b.close()
for n,c,d in res: print(('PASS ' if c else 'FAIL ')+n+('' if c else '   <- '+str(d)))
print(sum(1 for r in res if r[1]),'/',len(res))
````

Serve: `cd dist && python3 -m http.server 8765`. Both scripts print PASS/FAIL per row and a total;
the rehearsed totals are 28/28 and 45/45.
