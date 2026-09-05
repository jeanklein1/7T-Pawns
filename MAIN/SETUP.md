# Setting this up, and editing the words

## 1. Where each file goes

You already have the repo. Clone it if it isn't on this machine:

```
git clone https://github.com/jeanklein1/7T-Pawns.git
cd 7T-Pawns
git checkout -b WEBSITE_0
```

Then place the files I sent. Create the folders that don't exist yet.

| file I sent | goes to |
|---|---|
| `web-about-index.html` | `web/about/index.html` |
| `web-collection-index.html` | `web/collection/index.html` |
| `about_dist.py` | `tools/about_dist.py` |
| `collection_dist.py` | `tools/collection_dist.py` |
| `collection_gate.py` | `tools/gates/collection_gate.py` |
| `site.json` | `assets/about/site.json` |
| `message.js` | `functions/api/message.js` |

Two things aren't in that list because they're already in your repo or come
out of the zip:

- **Fonts.** From `dist-site.zip`, copy the `fonts/` folder to `web/fonts/`.
  Both pages load from there.
- **Images.** `assets/about/` needs the hero paintings named in `site.json`.
  `assets/collection/<folder>/` needs the works. Right now those are copies
  of `assets/paintings/` I made as a stub — replace with your real masters
  when they're ready.

**The two `*-preview.html` files are throwaways.** They're for looking at,
not editing. Nothing you change in them survives.

## 2. Install what the pipelines need

Python 3 from python.org (tick "Add Python to PATH" during install), then:

```
pip install pillow
```

That's the only dependency. Everything else is standard library.

## 3. Build and look at it

```
python tools/collection_dist.py
python tools/about_dist.py
python tools/gates/collection_gate.py
```

Order matters twice: web_dist.py wipes dist/ whole, so the engine
builds first — and the about page's four-image strip points into the
collection's output, so the collection builds first.

Then serve the result — opening `index.html` by double-clicking will not
work properly, because browsers block fonts and some paths on `file://`:

```
cd dist
python -m http.server 8000
```

Open `http://localhost:8000/about/` and `/collection/`. Leave that window running while you work.
After any edit: rebuild, then hard-refresh the browser (Ctrl+Shift+R).

## 4. Which IDE

**VS Code.** Free, opens a folder rather than a solution file, handles
HTML, CSS, Python and JSON in one window.

Visual Studio (the big one) is built around C++ and C# solutions. It's the
right tool for the engine, wrong tool for a static site — it will want to
create project files this repo doesn't use.

Using both is fine and normal: Visual Studio for `src/`, VS Code for
`web/`, `tools/` and `assets/`. They don't conflict.

In VS Code: **File → Open Folder → 7T-Pawns**. Two extensions worth having,
neither required: *Live Preview* (Microsoft) and *Python* (Microsoft).

## 5. Editing the words

The text lives in two kinds of place, and the difference matters.

### Prose — in `web/about/index.html`

Sentences that are part of the page's argument are written in the HTML.
Every block that still needs your words is marked with an HTML comment
containing `PLACEHOLDER` — search for that word instead of line numbers,
which shift as the file changes. The build prints how many remain:


```
5 PLACEHOLDER markers still in the page — copy is not final.
```


Change the words **between** the tags, never the tags themselves. So:

```html
<h1>The pawn is a vessel for projected intent.</h1>
     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ this part
```

Two characters need escaping if you type them as text: write `&amp;` for
`&` and `&lt;` for `<`. Apostrophes and dashes are fine as-is.

### Data — in `assets/about/site.json`

Anything that's a list rather than a sentence:

```json
{
  "email": "hello@everexpandingboard.com",

  "hero": {
    "wide": [ { "file": "PAINTING_90.jpeg", "n": 90 } ],
    "tall": [ { "file": "PAINTING_50.jpeg", "n": 50 } ]
  },

  "authors": [
    { "name": "Jean Klein",
      "lines": ["First paragraph.", "Second paragraph."] }
  ],

  "links": [
    { "label": "Instagram", "url": "https://instagram.com/yourhandle" }
  ]
}
```

- **hero** — the daily rotation pools. Add as many as you like; each file
  must exist in `assets/about/`. Wide works are used on desktop, tall on
  phones. Curate these by hand.
- **authors** — each `lines` entry becomes a paragraph.
- **links** — any URL containing the word `REPLACE` is silently dropped at
  build time, which is why the section currently reads "links arrive here."
  Put real URLs in and they appear.

JSON is strict: double quotes only, no trailing comma after the last item
in a list. VS Code underlines mistakes in red.

### Set names — in `assets/collection/<folder>/set.json`

```json
{ "label": "Desert Board", "note": "2023–24", "paper": false }
```

- `label` — the heading shown on the collection page.
- `note` — optional short line beside the count.
- `paper` — `true` gives works on paper a warm mat so a white sheet
  doesn't float against the black.

Per-work captions go in the same file:

```json
{
  "label": "Desert Board",
  "works": {
    "107": { "title": "Noon", "meta": "oil on canvas · 2024 · 80 × 60 cm" }
  }
}
```

The key is the number from the filename. Any work you don't name still
ships, titled `no. 107`.

## 6. When it's ready to go live

```
npm install -g wrangler
wrangler login
wrangler pages deploy dist
```

Direct upload, so images never go through git and Cloudflare never runs a
build. Before real masters land, keep them out of version control:

```
See MERGE.md §2 for the exact .gitignore lines.
```

The message box needs two environment variables set in the Cloudflare
dashboard under Pages → your project → Settings → Environment variables:
`RESEND_API_KEY` from resend.com, and `MESSAGE_TO` with your inbox. Until
those exist the form shows your email address instead of sending.
