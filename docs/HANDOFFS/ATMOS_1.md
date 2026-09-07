# ATMOS_1 — THE PIECE LEAVES THE PHONE'S HANDS

*Handoff · authored 2026-09-07 · Claude → CC · Jean holds build, visual, merge, deploy.*
*Filed to `docs/HANDOFFS/ATMOS_1.md` while open; the directory dies with the campaign.*

## THE IDEA, IN ONE PARAGRAPH

ATMOS_0 made the piece not sing from a pocket: hidden pauses, visible resumes if it was
wanted. But a paused `<audio>` still holds its media, and a phone keeps a paused piece on
the lock screen and behind the headphone button — leave the page, press play on the
headphones, and the soundtrack plays with the page shut. Pausing is not enough. On hide the
media is RELEASED: the position is remembered, the source removed, the element loaded
empty, and the Media Session's state set to none, so the OS has nothing to resume. On
return the source is re-attached, the position restored once the decoder knows the piece
again, and the same resume path as before runs — including ATMOS_0 R's gesture fallback if
the browser refuses. The shell's grammar unchanged: `musicWasPlaying` is still the visitor's
standing answer; only what "hidden" does to the element changed.

## AUTHORITY

Base: `master` at HEAD `5d655aee`, fetched 2026-09-07. Rides `claude/atmos-1` (P12: a shell
behavior with a felt gate on a phone). HALT (P17, scoped P15): unreachable file; stale
authority. Else DEFAULT-AND-FLAG. Count **1** everywhere. LF only.

| file | blob at base |
| --- | --- |
| `web/index.html` | `9535e83b828a1e894bb039ec53af7d251eb97d94` |

**REHEARSED.** Both FINDs at count 1; the shell gate GREEN after the edit. The behavior
itself needs a phone and headphones — Jean's gate, by design.

## THE RULINGS
1. **Hidden releases, visible re-attaches.** `removeAttribute('src')` + `load()` is the
   documented way to make an element drop its media resource; a source-less element has no
   Media Session for the OS to show or drive.
2. **The position survives the release** (`musicResumeAt`), restored on `loadedmetadata`
   — `currentTime` set before metadata is ignored.
3. **Re-attaching may cost a gesture on iOS** (a fresh source is a fresh permission on some
   WebKit builds). If `play()` refuses, ATMOS_0 R's `armResumeOnGesture` already restores
   the piece at the next touch; nothing new needed.
4. **The mp3 is fetched again on return** — from the browser cache in practice (4 MiB).
   Acceptable; if it ever isn't, `music/` joins IMMUTABLE_PATHS (PRODUCTS_0's list) and
   that is a one-line follow-up.

## U1 — THE RELEASE

### U1.1 — hidden releases the media; visible re-attaches it (`web/index.html`)

FIND
```
      if (document.hidden) {
        // Unconditional: pausing something already paused costs nothing,
        // and musicWasPlaying is not re-derived from element state here
        // — it is the visitor's standing answer, set at the gesture.
        if (music) { try { music.pause(); } catch (err) { /* nothing to pause */ } }
        return;
      }
      if (music && musicWasPlaying) {
        try {
          var p = music.play();
```
REPLACE
```
      if (document.hidden) {
        // ATMOS_1 — THE PIECE LEAVES THE PHONE'S HANDS. A paused element
        // still holds its media, and a phone keeps a paused piece on the
        // lock screen and behind the headphone button: leave the page,
        // press play on the headphones, and the piece sings from a pocket
        // with the page shut. Pausing is not enough; the media is
        // RELEASED — position remembered, source removed, the element
        // loaded empty — so the OS has nothing to resume. Return re-
        // attaches the source and picks up where it left off.
        // musicWasPlaying is not re-derived from element state here — it
        // is the visitor's standing answer, set at the gesture.
        if (music) {
          try {
            musicResumeAt = music.currentTime || 0;
            music.pause();
            music.removeAttribute('src');
            music.load();
          } catch (err) { /* nothing to release */ }
          if (navigator.mediaSession) {
            try { navigator.mediaSession.playbackState = 'none'; } catch (err) { /* no session to drop */ }
          }
        }
        return;
      }
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

### U1.2 — the remembered position (`web/index.html`)

FIND
```
    var ENTRY_WORDS = 'Ready';```
REPLACE
```
    var musicResumeAt = 0;       // ATMOS_1 — where the piece was when the page was hidden
    var ENTRY_WORDS = 'Ready';```


### WITNESS — U1
```
grep -c "ATMOS_1" web/index.html            # 8 — the variable, the release banner, the re-attach (the word recurs inside the banners)
grep -c "musicResumeAt" web/index.html      # 3
python3 tools/gates/shell_gate/run.py       # GREEN
```
Commit: `ATMOS_1 U1 — the piece leaves the phone's hands: hidden releases the media (position kept, source removed, session none), visible re-attaches and seeks`

## U2 — OPEN.md, push
```
## ATMOS_1 — THE PIECE LEAVES THE PHONE'S HANDS (on `claude/atmos-1`; Jean's gates open)
Hidden releases the soundtrack's media instead of pausing it, so a headphone button
cannot resume it with the page shut; visible re-attaches and seeks. ATMOS_0's grammar
stands; ATMOS_0 R's gesture fallback covers a refusal on return.
```
Push `claude/atmos-1`.

## JEAN'S GATES (phone + headphones)
1. Enter, start the piece, close or background the page, press play on the headphones:
   **silence.** The lock screen shows no player for the site.
2. Return to the page: the piece resumes where it was (or at the next tap, with the
   `[shell] audio resume needs a gesture` line in the log panel — that is ATMOS_0 R
   honouring a refusal, not a failure).
3. Desktop unaffected: tab away and back, the piece resumes.
