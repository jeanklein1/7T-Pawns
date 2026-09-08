# SETTLE_1 — THE READER MOVES TO WHERE THE THING IT READS EXISTS

*Handoff · authored 2026-09-08 · Claude → CC · Jean holds build, visual, merge, deploy.*
*Filed to `docs/HANDOFFS/SETTLE_1.md` while open; the directory dies with the campaign.*

## THE IDEA, IN ONE PARAGRAPH

SETTLE_0's U1.7 gave `BOOT_SET` the reader its banner promised — and placed it two hundred
lines before `dist/` is created. On a fresh clone the tool refused with *"the list is stale,
fix the list"* before it could make the thing it was checking for; on a machine with a
previous `dist/` it summed last deploy's bytes under a banner that says RECORD IT. CC's five
auditors rated it a blocker, one of them by building scratch trees and running both versions
(SETTLE_0's: exit 2, no dist; the parent's: exit 0, 71 files). The repair is the one they
converged on: the reader moves to just before `WROTE`, where every entry — the copied
products, the transformed page, the baked poster, the written manifest — genuinely exists,
and its two checks become real post-write assertions, in the right order, with a message
that blames the right party. Two more things fold in, both CC's: the ORGAN panel's script IS
a boot fetch (appended to the page unconditionally, 73 KB), so it enters the boot set and
the banner stops excusing it; and ruling 6 becomes a process law with the answer to the
squeeze that broke it. The remaining counts and the ragged reflow are retired.

## AUTHORITY

Base: **`origin/claude/important-handoffs-9v1wus` at `280143b0`** (SETTLE_0 landed). Lands
as follow-up commits on that branch; the branch dies on Jean's merge. HALT (P17, scoped
P15): unreachable file; stale authority. Else DEFAULT-AND-FLAG. Count **1** everywhere. LF
only. **P19 applies to this round's own push.**

| file | blob at base |
| --- | --- |
| `tools/web_dist.py` | `eb27cad3410d66bec84943cd1c8cd035f581b62d` |
| `web/index.html` | `432b1e6fa57462edabef736322d84e563719794f` |
| `docs/PROCESS_LAWS.md` | `52b43239707675a60ef096f46d8db8e165011d94` |

**REHEARSED, THE WAY CC'S AGENT DID IT.** All seven FINDs at count 1; the tool parses;
products gate and shell gate GREEN. Then the tool itself, end to end, in a sandbox with no
`dist/` (a fresh clone) and fabricated build products (the package the real shader, the
glue declaring its size): **exit 0, 71 files written, and the first-visit line printed at
the end from dist as written.** A second run on the existing dist reproduces the number; a
third run after growing the wasm by 500 000 bytes prints a number 500 000 larger — this
run's, not the last one's. The fabrications were removed; nothing of them is in the tree.

## THE RULINGS

1. **A check runs where the thing it checks exists.** The boot-set reader sits just before
   `WROTE`. `--check` writes nothing and prints no first-visit number; that is correct.
2. **The forbidden test precedes the existence test.** A forbidden entry is the worse
   finding, and a missing forbidden entry must not masquerade as merely missing.
3. **The message blames the right party.** At that point dist is complete, so an entry the
   run did not write is the LIST being wrong, and it says so.
4. **The panel is a boot fetch.** `organ_panel.js` is appended to the page on every boot;
   it enters `BOOT_SET`, and SETTLE_0's banner clause excusing it is retired as false.
5. **P19 — the push waits for the verdict, and the work is never the only copy.** The
   scratch ref `wip/<branch>` answers the container squeeze; the campaign branch waits.
6. **Counts retire.** The products' banner and BUILDID_0's twin in the shell stop saying
   "three"; the ATMOS_0 banner is reflowed. (The other "three"s in the tool count markers,
   preloads and options — verified, and left.)

## UNITS

| unit | what | files | commit |
| --- | --- | --- | --- |
| U1 | the reader moves; the panel counts; the counts stop | `tools/web_dist.py`, `web/index.html` | 1 |
| U2 | P19 | `docs/PROCESS_LAWS.md` | 1 |
| U3 | OPEN.md: SETTLE_0's entry notes the blocker and its repair; push per P19 | `docs/OPEN.md` | 1 |

## U0 — PREFLIGHT
```
git fetch origin && git checkout claude/important-handoffs-9v1wus && git pull --ff-only
git ls-tree HEAD tools/web_dist.py web/index.html docs/PROCESS_LAWS.md      # the three blobs
grep -n "the boot set's reader, and its two checks" tools/web_dist.py         # one hit, ~line 825 — the reader in the wrong place
```

## U1 — THE READER MOVES

### U1.1 — the reader leaves the download-cost block: it ran two hundred lines before dist existed (`tools/web_dist.py`)

FIND
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
REPLACE
```
    # SETTLE_1 — the first-visit number is printed at the END of this run,
    # from dist/ as written (the boot set's reader), not here: here dist/
    # does not exist yet, and SETTLE_0's reader refused every fresh clone
    # before the tool could create the thing it was checking for.
```

### U1.2 — the reader lands where every entry exists: just before WROTE (`tools/web_dist.py`)

FIND
```
    print("")
    print("WROTE %s  (%d files)" % (DIST, file_count))
```
REPLACE
```
    # SETTLE_1 — THE BOOT SET, READ FROM WHAT THIS RUN WROTE. dist/ is
    # complete here: the products copied, index.html transformed, the poster
    # baked, the manifest written. The two checks are post-write assertions
    # now — the exhibition/collection test first (a forbidden entry is the
    # worse finding, and a missing forbidden entry must not masquerade as
    # merely missing), then existence: an entry this run did not write is
    # the LIST being wrong, and says so. The sum is the number SHIP_0 U4
    # asked to have recorded; it describes THIS deploy, not the last one.
    boot_total = 0
    for name in BOOT_SET:
        if name.startswith(("paintings/", "collection/")):
            print("  BOOT_SET must never carry the exhibition or the collection: %s" % name)
            return 2
        p = os.path.join(DIST, name)
        if not os.path.exists(p):
            print("  BOOT_SET names %s, but this run wrote no such file — the list is wrong, fix the list" % name)
            return 2
        boot_total += os.path.getsize(p)
    print("")
    print("FIRST VISIT (the boot set, as written)")
    print("  %d bytes  (%.2f MiB) uncompressed, %d files — recorded, not optimized (SHIP_0 U4)"
          % (boot_total, mib(boot_total), len(BOOT_SET)))
    print("")
    print("WROTE %s  (%d files)" % (DIST, file_count))
```

### U1.3 — the ORGAN panel is a boot fetch; the banner says what the list is (`tools/web_dist.py`)

FIND
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
```
REPLACE
```
# What a visitor fetches before the world is on screen: the page, the
# program's products, the darkroom's, the ORGAN panel's script (appended
# to the page on every boot, whether or not ?organ=1 ever shows it), the
# veil's poster, and the exhibition manifest. Nothing sized O(catalogue)
# and nothing sized O(library) is in it. This list has one READER — the
# first-visit number printed at the end of a run sums it from dist/ as
# written — and two checks beside that sum, which are the asserts this
# banner once promised without keeping. The products gate witnesses every
# build product in here. (SETTLE_1: the reader moved to where dist exists,
# and the panel's 73 KB, which SETTLE_0's banner wrongly excused, counts.)
BOOT_SET = ["index.html", "organ_panel.js",
            "the_board.js", "the_board.wasm", "the_board.data",
            "darkroom_worker.js", "darkroom.js", "darkroom.wasm",   # PRODUCTS_0 — the darkroom opens beside the program (DARKROOM_1); a first-visit cost, recorded
            "veil_poster.jpg", EXHIBITION_JSON]
```

### U1.4 — the products' banner stops counting (`tools/web_dist.py`)

FIND
```
# index.html is SOURCE (tracked); the other three are build output
# (.gitignore'd). All four ship — but index.html is the only one that is
```
REPLACE
```
# index.html is SOURCE (tracked); the rest are build output (.gitignore'd)
# or the darkroom's worker script (tracked). All ship — but index.html is the only one that is
```

### U1.5 — BUILDID_0's twin banner stops counting (`web/index.html`)

FIND
```
    // THIS FILE IS A SOURCE FILE AND IT DOES NOT BOOT. The three build
    // artifacts always move together on disk, but their URLs were
```
REPLACE
```
    // THIS FILE IS A SOURCE FILE AND IT DOES NOT BOOT. The build
    // artifacts (three then, five since DARKROOM_1) move together on
    // disk, but their URLs were
```

### U1.6 — ATMOS_0's banner, reflowed (`web/index.html`)

FIND
```
    // Hidden RELEASES the media unconditionally (ATMOS_1: a paused piece
    // is still the phone's to resume); visible re-attaches it and resumes
    // ONLY if it was playing, so silence chosen (or never granted) stays
    // chosen. The
    // wake lock is re-asked here because the browser drops it on hide —
```
REPLACE
```
    // Hidden RELEASES the media unconditionally (ATMOS_1: a paused piece
    // is still the phone's to resume); visible re-attaches it and resumes
    // ONLY if it was playing, so silence chosen (or never granted) stays
    // chosen. The wake lock is re-asked here because the browser drops it
    // on hide —
```


### WITNESS — U1
```
python3 -c "import ast; ast.parse(open('tools/web_dist.py').read())"
grep -n "FIRST VISIT (the boot set, as written)" tools/web_dist.py     # 1 — and it sits above WROTE
grep -c "\"organ_panel.js\"," tools/web_dist.py                       # 3 — ARTIFACTS, IMMUTABLE_PATHS, BOOT_SET
python3 tools/gates/products_gate/run.py && python3 tools/gates/shell_gate/run.py
# THE ROUND'S REAL WITNESS — CC's agent's method, once more, before the commit:
#   a scratch copy of the tree with no dist/, fabricated products (the package = world.wgsl,
#   a glue declaring remote_package_size), then: python3 tools/web_dist.py → exit 0, WROTE,
#   and the FIRST VISIT block printed just above it. Grow the wasm, run again: the number grows.
```
Commit: `SETTLE_1 U1 — the boot set is read where dist exists (just before WROTE), forbidden before missing, the list blamed when the list is wrong; the ORGAN panel counts; the counts stop`

## U2 — P19

### U2.1 — P19: the push waits for the verdict; the work is never the only copy (`docs/PROCESS_LAWS.md`)

FIND
```
## SCHEDULING RECORD
```
REPLACE
```
## P19 — THE PUSH WAITS FOR THE VERDICT

A held branch is pushed only after the verification workflow has returned.
Two campaigns in one day landed with a blocker already on the remote because
the push went first — reversible, since Jean holds merge and deploy, but a
follow-up commit where a clean history was available.

The squeeze that broke it is real: an ephemeral container can be reclaimed
with the only copy of the work, and the stop hook is right to fear that. The
answer is not to choose between the two risks. **The work is pushed to a
scratch ref** — `wip/<branch>` — the moment the hook asks; the campaign branch
itself waits for the verdict, and the scratch ref is deleted when the real
push lands. Work safe, history clean, the hook's nagging is the price, and it
is said out loud in the report rather than reasoned around.

## SCHEDULING RECORD
```


Commit: `SETTLE_1 U2 — P19: the push waits for the verdict; the work is pushed to wip/<branch> when the hook asks`

## U3 — THE REGISTER, THE PUSH
Under SETTLE_0's entry: *"SETTLE_1: U1.7 read dist before dist existed (blocker, CC's
audit, proven by running both tools); the reader now sits before WROTE; organ_panel.js is
a boot fetch and counts; P19 written."* Then, per P19: `git push origin
HEAD:refs/heads/wip/important-handoffs-9v1wus` the moment the hook asks; the branch itself
after the verdict; delete the scratch ref after.

## JEAN'S GATES
1. Pull the branch; `python tools\web_dist.py` after a build prints, just above WROTE,
   `FIRST VISIT (the boot set, as written)` with **10 files** and a number that includes
   the ORGAN panel's 73 KB — the first-visit cost SHIP_0 asked to have recorded, finally
   describing the deploy it ships with.
2. The audio gates from SETTLE_0 are unchanged and still open: sound off → background →
   return → tap; the headphone button; the double-hide.
