# Merging the website into 7T-Pawns

Verified end-to-end on a fresh clone before writing this: unzip, place
images, build, gate passes, pages render. Everything in the zip is **new
files only** — no file already in the repo is touched, so the engine
cannot break from this merge.

## 1. Land the files

```
cd 7T-Pawns
git checkout master && git pull
git checkout -b WEBSITE_0
```

Unzip `web-merge.zip` over the repo root. Paths inside already match:

```
web/shared.css                      one stylesheet, both pages
web/about/index.html                the about page, source
web/collection/index.html           the collection page, source
web/fonts/                          Newsreader (subset) + OFL licence
tools/about_dist.py                 builds dist/about/
tools/collection_dist.py            builds dist/collection/
tools/gates/collection_gate.py      engine-agnosticism gate
functions/api/message.js            the message box endpoint
assets/about/site.json              hero pool · authors · links · email
assets/collection/<set>/set.json    one per set
```

## 2. Keep images out of git — before adding any

```
printf '\n# website assets — masters live outside version control\nassets/about/*.jpg\nassets/about/*.jpeg\nassets/collection/**/*.jpg\nassets/collection/**/*.jpeg\nassets/collection/**/*.png\n' >> .gitignore
```

## 3. Put images in

Rename the five `assets/collection/` folders (currently the number
blocks: `a_1-14` … `e_unfiled`) to what the sets actually are, and set
`"label"` in each `set.json`. Drop masters in as `PAINTING_<n>.jpg` —
the number is the sort order and the work's URL.

`assets/about/` needs the heroes `site.json` names: wide 90, 92, 112 and
tall 50, 109 today. Copy them from `assets/paintings/` or pick others
and edit the file. Curate these by hand.

## 4. Build order — this is the one rule

**`web_dist.py` deletes all of `dist/` every run** (`shutil.rmtree`,
line ~896). Until CC teaches it to clean only its own files, the engine
always builds first:

```
python tools/web_dist.py           # engine → dist/  (wipes the folder)
python tools/collection_dist.py    # → dist/collection/
python tools/about_dist.py         # → dist/about/ + dist/fonts/ + shared.css
python tools/gates/collection_gate.py
```

Then `cd dist && python -m http.server 8000` and open
`localhost:8000/about/` and `/collection/`. (Without the engine's build
artifacts, `web_dist.py` refuses *before* its rmtree — so on a machine
that can't build the engine, run just the last three lines.)

Both pipelines print warnings while copy or links are placeholder —
currently 5 `PLACEHOLDER` markers and 3 dropped `REPLACE` links. They
stop when the real words and URLs are in.

## 5. Commit

```
git add web tools functions assets/about assets/collection .gitignore
git commit -m "WEBSITE_0: about and collection pages, two pipelines, one gate"
git push -u origin WEBSITE_0
```

## What CC owns next (none of it blocks the merge)

1. **web_dist cleans only its own outputs**, so build order stops
   mattering. Until then: engine first.
2. **Fold `dist/collection/_headers.fragment` into the root `_headers`**
   web_dist writes. Wanted set: `/collection/*` and `/fonts/*` immutable
   (collection filenames carry a content hash, so that's safe);
   `/` and `/about/` no-cache.
3. **Nav on the engine shell, outside the WebGPU path** — `collection` /
   `about` links that render even when the adapter request fails.
4. **The fallback card becomes a door** — for a Firefox visitor it is
   the whole site. Same edit adds `meta description`, OG tags and a
   `noscript` paragraph to `web/index.html`.

## Deploying

```
npm install -g wrangler
wrangler login
wrangler pages deploy dist
```

Message box: set `RESEND_API_KEY` and `MESSAGE_TO` in Pages → Settings →
Environment variables; until then the form shows the email address
instead of sending. One thing to verify against current Cloudflare docs
at deploy time: that direct-upload deploys pick up the root `functions/`
directory from where wrangler runs — Functions behave differently
between git-integrated and direct-upload projects, and the details have
shifted before. If the deploy log doesn't mention compiling Functions,
that's the place to look.

Note: `about-preview.html` and `collection-preview.html` are review
artifacts, not repo files — nothing from them gets merged.
