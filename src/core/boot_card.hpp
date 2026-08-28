#pragma once
// ═══ THE BOOT CARD — THE PROGRAM'S OWN SURFACE (IOS_3 B2/B3) ═════════
//
// WHY THIS EXISTS. Every discriminator this campaign built prints to a
// console, and the device that is failing has none. An iPad showing a
// black screen cannot be asked what happened: there is no log to read,
// no inspector to open, and the only instrument in the room is a
// photograph of the screen. So the program writes its own identity and
// its own failures ONTO the screen, where a photograph can carry them.
//
// IT IS NOT AN INSTRUMENT. core/instruments.hpp governs recurring
// self-MEASUREMENT — clocks, timestamps, census tables — and every one
// of those answers to the dial because the shipped frame should not pay
// for readings nobody reads. This is the opposite kind of thing: the
// piece telling the room it is broken. A failure surface that ships only
// in the lab build is absent exactly when it is needed, so this ships in
// EVERY build, at every dial setting.
//
// ── THE TWO CHANNELS, AND WHY THE FACTS ARE BUFFERED ────────────────
//
// A FACT (card_fact) is a line the program already prints at boot — the
// build stamp, the shader sha, the adapter, the world. It is recorded
// ALWAYS and shown only when asked (?bootinfo=1) — an ordinary visitor
// must not meet a diagnostic panel.
//
// AN ERROR (card_err) is shown ALWAYS, because a black screen must speak
// on its own. And when the first one fires it FLUSHES THE FACTS FIRST,
// so a photograph of a broken screen identifies the build that broke.
// That is the whole reason the facts are buffered rather than dropped:
// at the moment the error arrives it is too late to go and collect them.
//
// THE COST, stated: one EM_ASM per fact when bootinfo is set (a dozen at
// boot, none after), one per second for the live line, one per error
// (the page caps the card at 20 lines). At the default — no bootinfo, no
// errors — it is a few string appends into a static buffer and nothing
// crosses into JS at all.
//
// THE DIV IS IN web/index.html, first in <body>, ahead of the canvas and
// the organ panel, so nothing paints over it. Absent div = silent no-op:
// a dev serving a hand-written page still runs.

#include <emscripten.h>
#include <string>

#include "core/boot_params.hpp"   // boot_params().bootinfo — the door

namespace t7 {

    // ── The two doors into the page ─────────────────────────────────
    inline void card_write_(const std::string& line) {
        EM_ASM({
            if (typeof window !== 'undefined' && window.t7card)
                window.t7card(UTF8ToString($0));
        }, line.c_str());
    }

    inline void card_write_err_(const char* tag, const std::string& msg) {
        EM_ASM({
            if (typeof window !== 'undefined' && window.t7err)
                window.t7err(UTF8ToString($0), UTF8ToString($1));
        }, tag, msg.c_str());
    }

    // The identity buffer: every fact, in the order the boot learned it.
    inline std::string& card_facts_() {
        static std::string facts;
        return facts;
    }

    inline bool& card_facts_flushed_() {
        static bool flushed = false;
        return flushed;
    }

    // ── A FACT ──────────────────────────────────────────────────────
    // Recorded always; shown now only if the door is open. Call it
    // beside the std::cout that already states the same thing, so the
    // console and the card cannot drift.
    inline void card_fact(const std::string& line) {
        card_facts_() += line;
        card_facts_() += '\n';
        if (boot_params().bootinfo) {
            card_facts_flushed_() = true;   // shown as it arrives
            card_write_(line);
        }
    }

    // ── AN ERROR ────────────────────────────────────────────────────
    // Always shown. The facts go first, once, so the photograph names
    // the build. A second error does not reprint them — an error storm
    // must not push the first useful message off a phone-sized card.
    inline void card_err(const char* tag, const std::string& msg) {
        if (!card_facts_flushed_()) {
            card_facts_flushed_() = true;
            if (!card_facts_().empty()) card_write_(card_facts_());
        }
        card_write_err_(tag, msg);
    }

    // ── THE LIVE LINE ───────────────────────────────────────────────
    // One line, rewritten in place, for state that changes while the
    // card is up — the patch counters Jean's lead asks for. Gated on the
    // door AND on its own cadence at the call site: once a second, never
    // per frame.
    inline void card_live(const std::string& line) {
        if (!boot_params().bootinfo) return;
        EM_ASM({
            var e = (typeof document !== 'undefined')
                  ? document.getElementById('t7live') : null;
            if (!e) {
                var host = document.getElementById('t7card');
                if (!host) return;
                host.style.display = 'block';
                e = document.createElement('div');
                e.id = 't7live';
                host.appendChild(e);
            }
            e.textContent = UTF8ToString($0);
        }, line.c_str());
    }

} // namespace t7
