# SETTLE_0 — THE FOUR RULINGS

*Handoff · authored 2026-09-07 · Claude → CC · Jean holds build, visual, merge, deploy.*
*Filed to `docs/HANDOFFS/SETTLE_0.md` while open; the directory dies with the campaign.*

## THE IDEA, IN ONE PARAGRAPH

CC's audit of ATMOS_1 and PRODUCTS_0 (193 agents, eight findings survived) asked for four
rulings. Here they are, as edits, on the branch the two campaigns landed on. (1) **The
Sound-toggle regression is real and it was mine:** ATMOS_1 released the media
unconditionally on hide but re-attached it only when sound was wanted, and wrote the attach
idiom a second time instead of using the one `startMusic` already had. The repair is the
one-home form: `attachMusic()` — guarded, idempotent, seek-restoring — called by
`startMusic`, by every return after entry (hoisted above the `musicWasPlaying` gate, since
the release above it is unconditional), and by the Sound toggle before it plays. (2) **The
fast double-hide** keeps the remembered position and drops the seek that could never fire.
(3) **The three stale banners** tell the truth the edits made — including the sharpest one,
which was an instance of its own thesis. (4) **`BOOT_SET` gets the reader it claimed**: the
printed first-visit number sums it (ARTIFACTS carried the ORGAN panel nobody fetches at boot
and omitted the poster and the manifest), with the two checks the banner promised, and
`NOT_AT_BOOT` exists where the products gate and the register say it does.

## AUTHORITY

Base: **`origin/claude/important-handoffs-9v1wus` at `45c383ca`** (ATMOS_1 + PRODUCTS_0
landed), fetched 2026-09-07. Lands as follow-up commits on that branch, as CC offered; the
branch still dies on Jean's merge. HALT (P17, scoped P15): unreachable file; stale
authority. Else DEFAULT-AND-FLAG. Count **1** everywhere. LF only.

| file | blob at base |
| --- | --- |
| `web/index.html` | `333a94fadb84cb1a2cb2dbbbdf1d8e06c37fbe76` |
| `tools/web_dist.py` | `247d02096ced66466cbd64a766a21f1062e70c35` |

**REHEARSED.** All nine FINDs at count 1; the shell gate GREEN; both inline script blocks
parse under node; `web_dist.py` parses; the products gate GREEN. The felt gate is Jean's.

## THE RULINGS

1. **One home for attaching the piece.** `attachMusic()`: no-op if the element has its
   source; otherwise attach, and if a position is remembered, seek to it on
   `loadedmetadata`. Three callers. The second copy ATMOS_1 wrote is gone.
2. **The release is unconditional, so the re-attach is too.** `if (entered) attachMusic();`
   sits above the `musicWasPlaying` gate; play stays gated on the visitor's standing answer
   (ATMOS_0's rule). Attaching costs nothing until play (`preload="none"`).
3. **The seek is a named listener, so a hide can cancel it.** A second hide before
   `loadedmetadata` keeps `musicResumeAt` and removes the pending seek — no restart from
   the top, no orphaned listener.
4. **The boot set sums, checks, and describes.** The first-visit line sums `BOOT_SET`;
   an entry missing from dist or living under `paintings/` or `collection/` is a refusal
   (return 2, the tool's own idiom); `NOT_AT_BOOT = []` is the one place laziness is named.
5. **Prose that counted outputs stops counting.** "the three build outputs", "Delete the
   three build files", and ATMOS_0's "Hidden pauses unconditionally" all now say what is.
6. **On the process note:** yes — gate the push on the verification workflow for every held
   branch from now on. A feature-branch push is reversible; a clean history is the habit.

## UNITS

| unit | what | files | commit |
| --- | --- | --- | --- |
| U1 | the shell: one home, the hoist, the toggle, the seek, the banner | `web/index.html` | 1 |
| U2 | the dist tool: the reader, the checks, `NOT_AT_BOOT`, the prose | `tools/web_dist.py` | 1 |
| U3 | OPEN.md: ATMOS_1 and PRODUCTS_0 entries note the settle; push | `docs/OPEN.md` | 1 |

## U0 — PREFLIGHT
```
git fetch origin && git checkout claude/important-handoffs-9v1wus && git pull --ff-only
git ls-tree HEAD web/index.html tools/web_dist.py       # the two blobs above
grep -c "function attachMusic" web/index.html           # 0 — not yet
```

## U1 — THE SHELL

### U1.1 — one home for attaching the piece; startMusic uses it (`web/index.html`)

FIND
```
    function startMusic() {
      if (!music) { note('[shell] audio: no element'); return; }
      try {
        if (!music.getAttribute('src')) music.setAttribute('src', music.dataset.src || '');
```
REPLACE
```
    // ATMOS_1 R1 — ONE HOME FOR ATTACHING THE PIECE. startMusic, every
    // return from hidden, and the Sound toggle need the same thing: a
    // source on the element, and the remembered position restored once the
    // decoder knows the piece again (currentTime before metadata is
    // ignored). Idempotent: an element that has its source is left alone.
    // ATMOS_1 wrote the attach twice and the toggle not at all, and a
    // visitor who chose silence before pocketing the phone came back to a
    // source-less element under a label reading "Sound: on" (CC's audit,
    // three refuters). One function, three callers, no second copy.
    var musicSeek = null;          // the pending seek's listener, or null
    function attachMusic() {
      if (!music || music.getAttribute('src')) return;
      music.setAttribute('src', music.dataset.src || '');
      if (musicResumeAt > 0) {
        var at = musicResumeAt;
        musicSeek = function () {
          music.removeEventListener('loadedmetadata', musicSeek);
          musicSeek = null;
          try { music.currentTime = at; } catch (err) { /* start over */ }
        };
        music.addEventListener('loadedmetadata', musicSeek);
      }
    }

    function startMusic() {
      if (!music) { note('[shell] audio: no element'); return; }
      try {
        attachMusic();
```

### U1.2 — a hide before the seek keeps the remembered position (`web/index.html`)

FIND
```
            musicResumeAt = music.currentTime || 0;
```
REPLACE
```
            // ATMOS_1 R1 — a second hide before the seek landed would
            // remember 0 (the source is fresh, the piece not yet at its
            // place); keep the position already remembered, and drop the
            // seek that will never fire on an element about to lose its
            // source.
            if (musicSeek) {
              music.removeEventListener('loadedmetadata', musicSeek);
              musicSeek = null;
            } else {
              musicResumeAt = music.currentTime || 0;
            }
```

### U1.3 — the source comes back on EVERY return after entry, wanted or not (`web/index.html`)

FIND
```
      if (music && musicWasPlaying) {
        try {
          // ATMOS_1 — the source comes back, and the position with it once
          // the decoder knows the piece again (currentTime before metadata
          // is ignored). Then the same resume as before.
          if (!music.getAttribute('src')) {
            music.setAttribute('src', music.dataset.src || '');
            var at = musicResumeAt;
            music.addEventListener('loadedmetadata', function seek() {
              music.removeEventListener('loadedmetadata', seek);
              try { music.currentTime = at; } catch (err) { /* start over */ }
            });
          }
          var p = music.play();
```
REPLACE
```
      // ATMOS_1 R1 — the element gets its source back on EVERY return after
      // entry, wanted or not: the release above was unconditional, so the
      // re-attach must be too, or silence chosen before hiding leaves a
      // source-less element behind the Sound toggle. Attaching costs
      // nothing until play (preload="none"); playing stays gated on the
      // visitor's standing answer, as ATMOS_0 ruled.
      if (entered) attachMusic();
      if (music && musicWasPlaying) {
        try {
          var p = music.play();
```

### U1.4 — the Sound toggle attaches before it plays (`web/index.html`)

FIND
```
        if (music.paused) {
          musicWasPlaying = true;                   // "sound is wanted" — the visibility handler's one input
          var p = music.play();
```
REPLACE
```
        if (music.paused) {
          musicWasPlaying = true;                   // "sound is wanted" — the visibility handler's one input
          attachMusic();                            // ATMOS_1 R1 — never play() a source-less element
          var p = music.play();
```

### U1.5 — ATMOS_0's banner tells the truth ATMOS_1 made (`web/index.html`)

FIND
```
    // Hidden pauses unconditionally; visible resumes ONLY if it was
    // playing, so silence chosen (or never granted) stays chosen. The
```
REPLACE
```
    // Hidden RELEASES the media unconditionally (ATMOS_1: a paused piece
    // is still the phone's to resume); visible re-attaches it and resumes
    // ONLY if it was playing, so silence chosen (or never granted) stays
    // chosen. The
```


### WITNESS — U1
```
grep -c "function attachMusic" web/index.html     # 1
grep -c "attachMusic();" web/index.html           # 3 — startMusic, the return, the toggle
grep -c "musicSeek" web/index.html                # 8
grep -c "music.setAttribute('src'" web/index.html # 1 — one home
python3 tools/gates/shell_gate/run.py             # GREEN
```
Commit: `SETTLE_0 U1 — attachMusic is one home with three callers; the source comes back on every return after entry; the seek survives a fast double-hide; ATMOS_0's banner tells ATMOS_1's truth`

## U2 — THE DIST TOOL

### U1.6 — the boot set's banner describes, and names its reader and its asserts (`tools/web_dist.py`)

FIND
```
# What a visitor fetches before the world is on screen: the page, the
# glue, the wasm, the package, the veil's poster, and the exhibition
# manifest. Nothing sized O(catalogue) and nothing sized O(library) is in
# it, and the asserts below are what keep it that way — a claim nobody
# checks is a claim that stops being true on the commit that breaks it.
BOOT_SET = ["index.html", "the_board.js", "the_board.wasm", "the_board.data",
            "darkroom_worker.js", "darkroom.js", "darkroom.wasm",   # PRODUCTS_0 — the darkroom opens beside the program (DARKROOM_1); a first-visit cost, recorded
            "veil_poster.jpg", EXHIBITION_JSON]
```
REPLACE
```
# What a visitor fetches before the world is on screen: the page, the
# program's products, the darkroom's, the veil's poster, and the exhibition
# manifest. Nothing sized O(catalogue) and nothing sized O(library) is in
# it. PRODUCTS_0 R1: this list has a READER now — the first-visit number
# printed below sums it (it used to sum ARTIFACTS, which carries the ORGAN
# panel nobody fetches at boot and omits the poster and the manifest) — and
# the two checks beside that sum are the asserts this banner used to
# promise: every entry exists in dist, none lives under the exhibition or
# the collection. The products gate witnesses every build product in here.
BOOT_SET = ["index.html", "the_board.js", "the_board.wasm", "the_board.data",
            "darkroom_worker.js", "darkroom.js", "darkroom.wasm",   # PRODUCTS_0 — the darkroom opens beside the program (DARKROOM_1); a first-visit cost, recorded
            "veil_poster.jpg", EXHIBITION_JSON]
# A build product that is NOT fetched at boot is excused here, by name, and
# nowhere else (the products gate reads this list). Empty: every product the
# build makes today is a boot fetch.
NOT_AT_BOOT = []
```

### U1.7 — the first-visit number sums the boot set, and checks it (`tools/web_dist.py`)

FIND
```
    print("  first visit      %d bytes  (%.2f MiB) uncompressed" % (total, mib(total)))
```
REPLACE
```
    # PRODUCTS_0 R1 — the boot set's reader, and its two checks.
    boot_total = 0
    for name in BOOT_SET:
        p = os.path.join(DIST, name)
        if not os.path.exists(p):
            print("  BOOT_SET names %s, and dist has no such file — the list is stale, fix the list" % name)
            return 2
        if name.startswith(("paintings/", "collection/")):
            print("  BOOT_SET must never carry the exhibition or the collection: %s" % name)
            return 2
        boot_total += os.path.getsize(p)
    print("  first visit      %d bytes  (%.2f MiB) uncompressed  (the boot set: %d files)" % (boot_total, mib(boot_total), len(BOOT_SET)))
```

### U1.8 — three outputs became five: the prose stops counting (`tools/web_dist.py`)

FIND
```
        print("(the three build outputs land in web/ beside the tracked index.html;")
```
REPLACE
```
        print("(the build outputs land in web/ beside the tracked index.html;")
```

### U1.9 — and the stale-build advice stops counting too (`tools/web_dist.py`)

FIND
```
        print("  web/ holds outputs from two different links. Delete the three")
        print("  build files and build again, and READ THE BUILD'S OUTPUT:")
```
REPLACE
```
        print("  web/ holds outputs from two different links. Delete the build")
        print("  files and build again, and READ THE BUILD'S OUTPUT:")
```


### U1.10 — BUILDID_0's banner, the fourth count, stops counting (`tools/web_dist.py`)

FIND
```
# The three build files always move together on disk, but their URLs
```
REPLACE
```
# The build files always move together on disk (three then; five since
# DARKROOM_1), but their URLs
```

### WITNESS — U2
```
python3 -c "import ast; ast.parse(open('tools/web_dist.py').read())"
grep -c "NOT_AT_BOOT = \[\]" tools/web_dist.py       # 1
grep -c "the boot set:" tools/web_dist.py           # 1
grep -c "three build" tools/web_dist.py             # 0 — four counts retired (two prints, the BOOT_SET banner's six, BUILDID_0's three)
python3 tools/gates/products_gate/run.py            # GREEN
python3 tools/web_dist.py                           # on the machine, with dist built: the first-visit line names the boot set's file count
```
Commit: `SETTLE_0 U2 — BOOT_SET has its reader (the first-visit sum) and its two checks; NOT_AT_BOOT exists; the prose stops counting outputs`

## U3 — THE REGISTER
Under ATMOS_1's entry: *"SETTLE_0: `attachMusic()` is the one home (three callers); the
re-attach is unconditional after entry; the seek survives a double-hide."* Under
PRODUCTS_0's: *"SETTLE_0: BOOT_SET is read by the first-visit sum and checked; NOT_AT_BOOT
exists, empty."* Commit `SETTLE_0 U3 — the register`; verify, then push.

## JEAN'S GATES (phone + headphones)
1. Sound **off** in the menu, background the page, return: the toggle reads "Sound: off";
   tap it — the piece plays from where it was. (The path CC's auditors found.)
2. Sound on, background, headphone play button: silence. Return: the piece resumes where
   it was, or at the next tap.
3. Background twice quickly (app-switch and back and away again), then return: the piece
   resumes where it was, not from the top.
4. `python tools\web_dist.py` after a build: the first-visit line reads "(the boot set:
   9 files)" and a number a little different from before — the ORGAN panel no longer
   counted, the poster and the manifest now are.
