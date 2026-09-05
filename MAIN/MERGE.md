# Merging the website into 7T-Pawns

Everything here is **new files only**. No file already in the repo is
modified by this merge, so nothing you have can break. `git status` on my
working copy shows zero changes to tracked files.

## 1. Land the files

```
cd 7T-Pawns
git checkout master
git pull
git checkout -b WEBSITE_0
```

Unzip `web-merge.zip` over the repo root. The paths inside already match
the repo, so it drops straight in and creates the folders it needs:

```
web/shared.css                          one stylesheet, both pages
web/about/index.html                    the about page, source
web/collection/index.html               the collection page, source
web/fonts/                              Newsreader, subset, + OFL licence
tools/about_dist.py                     builds dist/about/
tools/collection_dist.py                builds dist/collection/
tools/gates/collection_gate.py          engine-agnosticism gate
functions/api/message.js                the message box endpoint
assets/about/site.json                  hero pool, authors, links, email
assets/collection/<set>/set.json         one per set
```

Nothing lands in `src/`, `web/index.html`, `tools/web_dist.py` or
`assets/paintings/`. The engine is untouched.

## 2. Keep images out of git

Before any real masters arrive:

```
printf '\n# website assets — masters live outside version control\nassets/about/*.jpg\nassets/about/*.jpeg\nassets/collection/**/*.jpg\nassets/collection/**/*.jpeg\nassets/collection/**/*.png\n' >> .gitignore
```

The `set.json` and `site.json` files stay tracked; the paintings do not.
`dist/` is already ignored by the existing rule.

## 3. Put images in

The five folders under `assets/collection/` are the number blocks from
your filenames, as placeholders:

```
a_1-14  b_32-92  c_100-115  d_200-214  e_unfiled
```

Rename them to what those sets actually are, and set `"label"` in each
`set.json` to match. Then drop the masters in, named `PAINTING_<n>.jpg` —
the number is both the sort order and the work's URL.

`assets/about/` needs the hero paintings that `site.json` names. Right now
it names `PAINTING_90`, `92`, `112` (wide, for desktop) and `50`, `109`
(tall, for phones). Copy those from `assets/paintings/`, or pick others
and edit the file.

## 4. Build and look

```
pip install pillow
python tools/collection_dist.py
python tools/about_dist.py
python tools/gates/collection_gate.py
cd dist && python -m http.server 8000
```

`http://localhost:8000/about/` and `/collection/`. Order matters — the
about page's four-image strip points into the collection's output and
refuses to build without it.

## 5. Commit

```
git add web tools functions assets/about assets/collection .gitignore
git commit -m "WEBSITE_0: about and collection pages, two pipelines, one gate"
git push -u origin WEBSITE_0
```

## What CC still owns

Four items on the engine's side. None blocks the two pages, and all four
are small.

**Route the engine's output.** `web_dist.py` currently assembles into
`dist/`. That is still correct — the engine keeps `/`. But it must not
delete `dist/about/`, `dist/collection/`, `dist/fonts/` or
`dist/shared.css` when it cleans. Check whether it wipes the folder.

**Merge the headers.** `collection_dist.py` writes
`dist/collection/_headers.fragment`. Cloudflare reads exactly one
`_headers`, at the deployment root, so `web_dist.py`'s writer should fold
that fragment in. The full set wants:

```
/collection/*   Cache-Control: public, max-age=31536000, immutable
/fonts/*        Cache-Control: public, max-age=31536000, immutable
/               Cache-Control: no-cache
/about/         Cache-Control: no-cache
```

Collection derivatives carry a content hash in the filename, so immutable
is safe there — replacing a master changes the URL.

**Nav on the engine, outside the WebGPU path.** A small overlay linking
`collection` and `about`, rendered whether or not the adapter request
succeeds. Today a visitor whose browser cannot run the world has no way
to reach the rest of the site.

**The fallback card becomes a door.** It currently explains a failure. For
a Firefox visitor that card is the entire site, so it should say what the
board is and link to `/collection/`. Same edit adds a `meta description`,
OG tags and a `noscript` paragraph to `web/index.html` — right now a
`<canvas>` is what search engines and link previews find at the domain.

## Deploying

```
npm install -g wrangler
wrangler login
wrangler pages deploy dist
```

Direct upload, so images never pass through git and Cloudflare never runs
a build. For the message box, set `RESEND_API_KEY` and `MESSAGE_TO` in
Pages → Settings → Environment variables. Until then the form shows your
email address instead of sending.
