#pragma once
// ═══ THE RIDE FACE — TWO WORDS ON THE GLASS (REACH_1 V2) ═════════════
//
// The summit sensor and the ride are invisible law: a door that opens
// underfoot atop a tall pyramid and a gesture that answers it. REACH_0 proved that
// invisible law reads as nothing happening. This is the teacher: ONE
// call per EDGE — never per frame — telling the shell which face to
// wear. 0 hides, 1 says the pulse boards, 2 says the pulse lands.
//
// THE SHELL OWNS THE LOOK. window.T7_RIDE (web/index.html) draws the
// glyph and the words; the artwork slot is Jean's. Absent global =
// silent no-op, the boot card's contract: a dev serving a hand-written
// page still runs. And it ships in every build for the boot card's
// reason too — a teacher that exists only in the lab teaches nobody in
// the gallery.

#include <emscripten.h>

namespace t7 {

    inline void ride_face(uint32_t mode) {
        EM_ASM({
            if (typeof window !== 'undefined' && window.T7_RIDE)
                window.T7_RIDE.set($0);
        }, mode);
    }

} // namespace t7
