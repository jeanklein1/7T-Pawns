# COPY — where every visitor-facing word lives

One rule: edit the OWNING file, then `python tools\dist.py` and deploy.
Nothing here is a second home; these are pointers. Written at DOORS_4;
every future round that moves words updates it.

Anchors are SYMBOLS, never line numbers (P2). `PLACEHOLDER` marks copy
Jean has not written yet — `about_dist` counts them and says how many
survive at the end of every build.

## The engine page (everexpandingboard.com) — `web/index.html`

| what a visitor reads | the symbol that owns it |
| --- | --- |
| the wordmark, `the_board` | `#veil .stack .wordmark` (and the card's own) |
| the veil's status line, first words | `#status`'s markup — *Waking the world* |
| the veil's two buttons — *The Board*, *Home* | `#veil .entry` |
| loading words — *Waking your device*, *Reserving memory*, *Compiling shaders*, *Hanging the paintings*, *Growing the terrain*, *Settling the ground*, *Opening the doors* | `classify()`'s `say(...)` calls. NO GPU TALK HERE (DOORS_3 ruling 1) |
| *Ready* | `ENTRY_WORDS` |
| the fallback card — *Welcome.* + Jean's sentences | `fallback()`'s `showCard(...)` |
| the fallback card's button — *Click here* | the `#card` markup, `a.btn.alt` |
| the floor / lost / watchdog cards | their own `showCard(...)` calls; `CONTACT` is the address they offer |
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
  Follow pane instead, and both take their five links from
  `web/follow.json`.

## The social links — `web/follow.json`
Five entries, label + https href. One home, one renderer
(`routes.follow_html`), two places shown: the engine's Follow pane and
the site sandwich's *Follow for more* box. Not in any footer — DOORS_4
ruling 3 took them off the three site footers and they did not go back
there.

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

## The home page — `web/home/index.html`
*(the directory and the route id stay `about` — wiring; only the word a
visitor reads became **Home**)*
- the statement — none since HOUSE_0 (Jean took it off). To write one:
  `header.statement` and its rules, one block back from git
  (`git show 60c2fd3:web/about/index.html`); `about_dist` reads it into
  `home.json` for the engine's Home pane when it is there.
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

## The gallery page — `web/gallery/index.html`
- the lede — `header.lede`'s `h1` and `p`. **STILL PLACEHOLDER COPY**: the
  `h1` is *The pawn is a vessel for projected intent.* — the doctrine that
  opened the about page until DOORS_3, moved to `/text/`, and deleted with
  it at DOORS_4. It is on NEITHER poem (the writings open *The Mirror in
  the Sand*), so this page and two `<meta>` descriptions are now its only
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

## The 404 page — `tools/web_dist.py`'s `NOT_FOUND_PAGE`
Served at every wrong address, so its links are absolute and its menu is
its own — it does NOT read `web/routes.json`. **A route renamed or added
must be renamed or added here by hand.** DOORS_4 found it still saying
*collection* and missing Writings entirely. Wording is Jean-gated, like
the card's.

## Not copy, and not to be edited as copy
`docs/`, `audit/`, every comment in `src/`, and the ledgers. The
`data-route` / `data-pane` / `data-platform` ids are wiring.
