# The website in 7T-Pawns — what remains

The merge is landed. WEBSITE_2 seated the zip's twenty-two files at
their repo paths, and WEBSITE_1 already taught the engine to share:
`web_dist.py` deletes only its own names, folds
`dist/collection/_headers.fragment` into the root `_headers`, ships
`web/_redirects` (`/main` → `/about/`, `/world` → `/`, 302), and the
shell carries two doors, meta/OG tags, a noscript paragraph and a
fallback card that describes the board. Verified end-to-end on a fresh
clone before landing: unzip, place images, build, gate passes, pages
render.

## 1. Put images in

Rename the five `assets/collection/` folders (currently the number
blocks: `a_1-14` … `e_unfiled`) to what the sets actually are, and set
`"label"` in each `set.json`. Drop masters in as `PAINTING_<n>.jpg` —
the number is the sort order and the work's URL. Masters are
gitignored; the json beside them stays tracked.

`assets/about/` needs the heroes `site.json` names: wide 90, 92, 112
and tall 50, 109 today. Copy them from `assets/paintings/` or pick
others and edit the file. Curate these by hand.

## 2. Build — web_dist last

```
python tools/collection_dist.py    # → dist/collection/
python tools/about_dist.py         # → dist/about/ + dist/fonts/ + shared.css
python tools/gates/collection_gate.py
python tools/web_dist.py           # engine → dist/, root _headers + _redirects
```

The collection builds before about (the about strip points into its
output); `web_dist.py` runs last so its fold and its conditional
`/fonts/*` · `/about/` rules read what the site wrote. It no longer
wipes the folder. `cd dist && python -m http.server 8000` to look;
`npx wrangler pages dev dist` to also exercise redirects and headers.

Both pipelines print warnings while copy or links are placeholder —
currently 5 `PLACEHOLDER` markers and 3 dropped `REPLACE` links. They
stop when the real words and URLs are in.

## 3. Deploy

```
wrangler pages deploy dist --project-name 7t
```

Message box: set `RESEND_API_KEY` and `MESSAGE_TO` in Pages → Settings
→ Environment variables; until then the form shows the email address
instead of sending. One thing to verify against current Cloudflare docs
at deploy time: that direct-upload deploys pick up the root `functions/`
directory from where wrangler runs — Functions behave differently
between git-integrated and direct-upload projects, and the details have
shifted before. If the deploy log doesn't mention compiling Functions,
that's the place to look.
