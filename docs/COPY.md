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
| the veil's two buttons — *The Board*, *About* | `#veil .entry` |
| loading words — *Waking your device*, *Reserving memory*, *Compiling shaders*, *Hanging the paintings*, *Growing the terrain*, *Settling the ground*, *Opening the doors* | `classify()`'s `say(...)` calls. NO GPU TALK HERE (DOORS_3 ruling 1) |
| *Ready* | `ENTRY_WORDS` |
| the fallback card — *Welcome.* + Jean's sentences | `fallback()`'s `showCard(...)` |
| the fallback card's button — *Click here* | the `#card` markup, `a.btn.alt` |
| the floor / lost / watchdog cards | their own `showCard(...)` calls; `CONTACT` is the address they offer |
| the noscript paragraph | the `<noscript>` markup |
| pane heads and notes — The Board, Controls, Gallery, Writings, About, Write me, Follow the work | each `.pane[data-pane="..."]`'s markup |
| The Board's messages | `.pane[data-pane="board"] .pane-note` — **Jean's** |
| the two platform lists | `.pane[data-pane="controls"] dl.ctl` — **words are placeholders, THE FACTS ARE THE TREE'S**: `direction/input.hpp` and `console.hpp`. Change words, never facts |
| the platform chips — *Mobile*, *Desktop* | the `.seg` markup. `CONTROLS_DEFAULT` pins which opens first |
| the live row — *Take a photo*, *Fullscreen*, *Sound: on/off* | `#ctlPhoto` / `#ctlFull` / `#ctlSound` |
| door-out labels — *Open Gallery in a new tab →*, *Read on the website →*, *About, on the website →* | each pane's `a[data-out]` |
| *The writings did not answer.* / *The collection did not answer.* / *The website did not answer.* | `loadWritings()` / `loadPeek()` / `loadAbout()` catch arms |
| *Sending…* / *Sent. Thank you.* / *It did not go through* | the `#mini` submit handler |

## Menus on every page
- **the labels** — `web/routes.json` (`label`; the `id` is wiring, never words)
- **the write box's words** — `tools/routes.py` `write_box_html`

## The social links — `web/follow.json`
Five entries, label + https href. One home; the engine's Follow pane is
the only reader (DOORS_4 ruling 3).

## The writings — `assets/writings/NN_slug.txt`
First line the title, blank line, then the body. Blank lines break
stanzas; line breaks inside a stanza are kept. **Filename order is page
order.** One new file is one new writing, on the page and in the engine's
pane both. `tools/about_dist.py` `build_writings` is the reader.

## The about page — `web/about/index.html`
- the statement — `header.statement`
- the three door labels and blurbs — `.doors`
- **the world door's picture**: drop the file at `assets\about\world.jpg`
  (`.jpeg` and `.png` also read). Absent = a warning and a door without a
  picture, never a refused build.
- the day's hero and the site email — `assets/about/site.json`

## The gallery page — `web/collection/index.html`
- the lede — `header.lede`'s `h1` and `p`. **STILL PLACEHOLDER COPY**: the
  `h1` repeats the writings' first line, which is the doctrine and not a
  gallery's opening. Jean's to write.
- set labels — each `assets/collection/*/set.json`

## The writings page chrome — `web/writings/index.html`
The `<title>` only. The texts come from `assets/writings/`.

## The 404 page — `tools/web_dist.py`'s `NOT_FOUND_PAGE`
Served at every wrong address, so its links are absolute and its menu is
its own — it does NOT read `web/routes.json`. **A route renamed or added
must be renamed or added here by hand.** DOORS_4 found it still saying
*collection* and missing Writings entirely. Wording is Jean-gated, like
the card's.

## The colophon — `web/about/index.html`'s `<footer>`
*set in Newsreader · no trackers*. It is the only footer left on the
site (ruling 8: the name appears once, and its home is the engine's
wordmark).

## Not copy, and not to be edited as copy
`docs/`, `audit/`, every comment in `src/`, and the ledgers. The
`data-route` / `data-pane` / `data-platform` ids are wiring.
