#pragma once
// ═══ THE PARAMETER SURFACE (DOMESDAY_1 B9 — LANTERN's deferred U2) ═══
//
// Measurement first. LANTERN_CENSUS §L2 recorded the gap this fills:
// the four moods were diegetically reachable but not addressable — a
// soak walk could not drive the twin deterministically to a named arm,
// so no capture could be known repeatable. Three values, read ONCE at
// boot before the device request, never reread, never mutated mid-run,
// invisible to ordinary visitors:
//
//   seed — u32, overrides the drawn boot seed (the [World] line then
//          says "(param)" instead of "(drawn)")
//   mood — index into MOOD_TABLE, forces the boot mood at the one
//          site that authors it (the Cartridge ctor; range-checked
//          there against MOOD_COUNT, which this header must not know)
//   cap  — float clamped to [0.5, 3.0] at parse, overriding
//          MAX_DEVICE_PIXEL_RATIO for this run (console.hpp
//          effective_pixel_cap) — the soak walk's key and the frame's
//          largest lever
//
// Channel: `?seed=&mood=&cap=` — ONE URLSearchParams read of
// location.search, in one EM_ASM block. Absent or malformed values
// are silently ignored; anything accepted prints one [Params] line
// at parse time (P6 — a switch that fired is visible). No UI, no
// mid-run reread.

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <emscripten.h>

namespace t7 {

    struct BootParams {
        bool has_seed = false; uint32_t seed = 0;
        bool has_mood = false; uint32_t mood = 0;
        bool has_cap  = false; float    cap  = 1.0f;
        bool has_msaa = false; uint32_t msaa = 1;   // DOMESDAY_2 B10: 1 or 4; anything else -> 1
        // PANORAMA_1 U6 — THE METRONOME, forced. rAF callbacks per presented
        // frame: 1 = every vblank, 2 = every second one (a steady 30 on a
        // 60 Hz panel). Present ONLY to make the taste gate takeable — Jean
        // rides the same world at each and says which is the piece. When the
        // param is present the governor never engages: a forced pace is an
        // instruction, not a starting point.
        bool has_pace = false; uint32_t pace = 1;   // {1, 2}; anything else -> 1

        // ── IOS_3 C — THE FOUR SWITCHES, AND WHY THEY ARE NOT DIALS ──
        //
        // Each removes ONE stage from the frame and nothing else. They
        // exist because the iPad has no console: the only question we can
        // ask a silent device is "does it render with this stage gone",
        // and the answer is a picture, which a photograph carries.
        //
        // DEFAULT 1 — the piece is what it was. A switch at 0 is a
        // DEGRADED picture on purpose, never a fallback and never a
        // preference: `bake=0` is a flat world, not a cheaper one.
        //
        // They are NOT ORGAN dials. A dial is a setting an operator may
        // like; these are surgical removals whose only readers are a
        // diagnosis and this comment.
        bool bake     = true;   // the patch heightfield bake dispatch
        bool card     = true;   // write_live_card
        bool sunpass  = true;   // the outdoor sun shadow pass, bundle included
        bool bundles  = true;   // ExecuteBundles; 0 = encode direct

        // ?bootinfo=1 shows the identity block on the page itself.
        // ?failboot=1 asks the device for a feature that does not exist,
        // so the failure path can be WITNESSED on a machine that works.
        bool bootinfo = false;
        bool failboot = false;
    };

    // Set once by parse_boot_params (main, before any consumer);
    // read-only ever after.
    inline BootParams& boot_params() {
        static BootParams p;
        return p;
    }

    // B10 — the walk's last instrument: multisampling as a boot-read
    // measurement affordance. {1, 4} only; the default stays 1 until
    // the soak walk prices the matrix, and the default flip afterward
    // is one constant. Pipelines are created once with this value —
    // no mid-run mutation, per the surface's law. (F1-b: defined
    // BELOW the accessor it reads — glaw1 caught the original order.)
    inline uint32_t effective_msaa() {
        return boot_params().has_msaa ? boot_params().msaa : 1u;
    }

    inline void boot_params_announce_() {
        BootParams& p = boot_params();
        if (p.has_cap) {
            if (p.cap < 0.5f) p.cap = 0.5f;
            if (p.cap > 3.0f) p.cap = 3.0f;
        }
        if (p.has_msaa && p.msaa != 4u) {
            p.msaa = 1u;   // B10: {1, 4} only; anything else -> 1
        }
        if (p.has_pace && p.pace != 2u) {
            p.pace = 1u;   // U6: {1, 2} only; anything else -> 1
        }
        // IOS_3 C — P6 applies to a switch that is OFF, not one that is
        // on: silence must mean "the piece entire". A stage removed
        // without a line in the log is a diagnosis nobody can reproduce.
        const bool any_switch = !p.bake || !p.card || !p.sunpass || !p.bundles
                                || p.bootinfo || p.failboot;
        if (p.has_seed || p.has_mood || p.has_cap || p.has_msaa || p.has_pace
                || any_switch) {
            std::cout << "[Params]";
            if (p.has_seed) std::cout << " seed=" << p.seed;
            if (p.has_mood) std::cout << " mood=" << p.mood;
            if (p.has_cap)  std::cout << " cap=" << p.cap;
            if (p.has_msaa) std::cout << " msaa=" << p.msaa;
            if (p.has_pace) std::cout << " pace=" << p.pace;
            if (!p.bake)    std::cout << " bake=0";
            if (!p.card)    std::cout << " card=0";
            if (!p.sunpass) std::cout << " sunpass=0";
            if (!p.bundles) std::cout << " bundles=0";
            if (p.bootinfo) std::cout << " bootinfo=1";
            if (p.failboot) std::cout << " failboot=1";
            std::cout << "\n";
        }
    }


    // One location.search read; three typed extractions into HEAPF64.
    // NaN = absent-or-malformed; integer/range checks land on the C++
    // side so the rule has one spelling per twin.
    inline void parse_boot_params(int, char**) {
        double vals[11];
        EM_ASM({
            var q = new URLSearchParams(location.search);
            var o = $0 >> 3;
            function num(k) {
                var v = q.get(k);
                if (v === null || v === "") return NaN;
                var n = Number(v);
                return isFinite(n) ? n : NaN;
            }
            HEAPF64[o]     = num('seed');
            HEAPF64[o + 1] = num('mood');
            HEAPF64[o + 2] = num('cap');
            HEAPF64[o + 3] = num('msaa');
            HEAPF64[o + 4] = num('pace');
            // IOS_3 C — the four switches and the two doors, through the
            // SAME single location.search read. A second read would be a
            // second spelling of the channel.
            HEAPF64[o + 5] = num('bake');
            HEAPF64[o + 6] = num('card');
            HEAPF64[o + 7] = num('sunpass');
            HEAPF64[o + 8] = num('bundles');
            HEAPF64[o + 9] = num('bootinfo');
            HEAPF64[o + 10] = num('failboot');
        }, vals);
        BootParams& p = boot_params();
        if (!std::isnan(vals[0]) && vals[0] >= 0.0 && vals[0] <= 4294967295.0
                && vals[0] == std::floor(vals[0])) {
            p.has_seed = true; p.seed = static_cast<uint32_t>(vals[0]);
        }
        if (!std::isnan(vals[1]) && vals[1] >= 0.0 && vals[1] <= 4294967295.0
                && vals[1] == std::floor(vals[1])) {
            p.has_mood = true; p.mood = static_cast<uint32_t>(vals[1]);
        }
        if (!std::isnan(vals[2])) {
            p.has_cap = true; p.cap = static_cast<float>(vals[2]);
        }
        if (!std::isnan(vals[4]) && vals[4] == std::floor(vals[4])
                && vals[4] >= 0.0 && vals[4] <= 4294967295.0) {
            p.has_pace = true; p.pace = static_cast<uint32_t>(vals[4]);
        }
        if (!std::isnan(vals[3]) && vals[3] == std::floor(vals[3])
                && vals[3] >= 0.0 && vals[3] <= 4294967295.0) {
            p.has_msaa = true; p.msaa = static_cast<uint32_t>(vals[3]);
        }
        // IOS_3 C — ONLY AN EXPLICIT 0 REMOVES A STAGE. Absent, malformed,
        // or any other value leaves the piece entire: a typo must not
        // silently ship a degraded world, and `?bake` with no value is a
        // typo. The two doors take the mirror rule — only an explicit 1
        // opens them.
        p.bake     = !(vals[5] == 0.0);
        p.card     = !(vals[6] == 0.0);
        p.sunpass  = !(vals[7] == 0.0);
        p.bundles  = !(vals[8] == 0.0);
        p.bootinfo = (vals[9] == 1.0);
        p.failboot = (vals[10] == 1.0);
        boot_params_announce_();
    }


} // namespace t7
