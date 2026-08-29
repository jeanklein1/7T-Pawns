#pragma once
// ═══ AUBADE U1 — THE WATERFALL'S C++ HALF ════════════════════════════
//
// WHAT THIS IS FOR. Between the navigation and the first present there is
// a stretch of dark nobody had measured, because the program had no
// first-present witness at all — `[FRAME_1]`, the line the campaign began
// by reading, is CAP_1's canvas-resize census and fires in the resize
// branch (R1). This header creates the missing witness and, with it, four
// numbers that say WHERE the dark went without anyone opening a
// Performance panel — which is the whole point, because the device that
// most needs reading has no DevTools.
//
// ONE CLOCK, IN THE PAGE'S REALM. Every mark is taken by
// window.T7_AUBADE from its own performance.now(). The C++ side does not
// keep a second clock: two clocks would need reconciling, and the
// reconciliation is the thing nobody checks.
//
// THE FOUR ATTRIBUTION NUMBERS, and what each one settles:
//
//   ticks  rAF turns between init and present — counted ABOVE the
//          first-light gate (AUBADE_1 F2), because a turn the gate sent
//          home is still a turn. MANY cheap ticks against a late present
//          means the main thread was free and the wait was DEVICE-SIDE —
//          which is what pipeline compilation looks like from here. Few
//          ticks means the main thread was busy.
//   cpu    summed main-thread ms over the turns that ran a FRAME BODY —
//          a subset of `ticks`, and deliberately so: a gated turn does no
//          work and should add none. Each turn contributes its own end
//          minus its own start; it was once a sum of absolute timestamps,
//          which is why a 5.4 s boot once reported 38,526 ms. Read WITH
//          ticks: ticks high + cpu low is a device wait; cpu high is our
//          own work.
//   stb    summed stb_image decode ms before present. The authored path
//          decodes on the main thread (R6), so this separates "the
//          paintings did it" from everything else.
//   fade   the piece's own fade alpha at first present. Authored darkness,
//          named, so it is never mistaken for the unauthored kind.
//
// SHIPS IN EVERY BUILD, like the boot card and for the same reason: an
// instrument that is absent from the build a visitor runs cannot explain
// what that visitor saw. The cost is six EM_ASM calls across a whole boot
// and two integer adds per frame until first present, after which the
// tick accumulator stops being touched.

#include <emscripten.h>

namespace t7 {

    // A mark. Idempotent in the page — the first call for a name wins, so
    // a mark inside a retry loop cannot smear.
    inline void aubade_mark(const char* name) {
        EM_ASM({
            if (typeof window !== 'undefined' && window.T7_AUBADE)
                window.T7_AUBADE.mark(UTF8ToString($0));
        }, name);
    }

    // AUBADE_1 F2 — MARK AND READ IT BACK, IN ONE CROSSING.
    //
    // `firstlight` was printed twice from two clocks: the page's mark, and
    // the renderer's own `pipeT0_` stopwatch, which starts at the first
    // pipeline issue rather than at the page's t0. The two disagreed by the
    // offset between them (4747 against 5062 — about `init`, which is
    // exactly what that offset is). One mark, one home: the site that marks
    // the instant also prints it, and prints THIS number.
    //
    // Returns ms from the page's t0, or -1 where there is no page.
    inline double aubade_mark_ms(const char* name) {
        return EM_ASM_DOUBLE({
            if (typeof window === 'undefined' || !window.T7_AUBADE) return -1.0;
            return window.T7_AUBADE.mark(UTF8ToString($0));
        }, name);
    }

    // The same clock, without leaving a mark — for a line that wants to say
    // WHEN it printed and does not deserve a name in the waterfall.
    inline double aubade_now_ms() {
        return EM_ASM_DOUBLE({
            if (typeof window === 'undefined' || !window.T7_AUBADE) return -1.0;
            return window.T7_AUBADE.now();
        });
    }

    // TWO LATCHES, AND THE DISTINCTION IS THE WHOLE INSTRUMENT.
    //
    //   registered — frame 1's submit has been made and the callback is
    //                armed. Guards against arming it twice.
    //   presented  — the callback has RESOLVED: the GPU has finished the
    //                work frame 1 described, and the dark is over.
    //
    // The window this campaign partitions is init -> PRESENTED, not
    // init -> registered. Collapsing the two would make `ticks` read 1 on
    // every boot and the probe would say nothing at all.
    inline bool& aubade_registered() { static bool r = false; return r; }
    inline bool& aubade_presented()  { static bool p = false; return p; }

    // The piece's own fade alpha, published once a frame by the stage-fade
    // phase. It crosses this way rather than through a new virtual on the
    // cartridge interface for the reason g_canvas_w does (RIBBON_6): one
    // store a frame beats widening a contract for one reading.
    inline float& aubade_fade() { static float f = 0.0f; return f; }

    // The probe's three C++-owned counters, handed to the page at report
    // time. Kept as plain statics because they are written from the frame
    // loop and read once.
    inline uint32_t& aubade_ticks() { static uint32_t t = 0; return t; }
    inline double&   aubade_cpu()   { static double d = 0.0; return d; }
    inline double&   aubade_stb()   { static double d = 0.0; return d; }

    // Push the three counters plus the fade alpha into the page's probe.
    // Called once, at first present — after which ticks and cpu stop
    // meaning anything and stop being counted.
    inline void aubade_probe() {
        EM_ASM({
            if (typeof window === 'undefined' || !window.T7_AUBADE) return;
            var p = window.T7_AUBADE.probe;
            p.ticks = $0;
            p.cpu   = $1;
            p.stb   = $2;
            p.fade  = $3;
        }, aubade_ticks(), aubade_cpu(), aubade_stb(), (double)aubade_fade());
    }

} // namespace t7
