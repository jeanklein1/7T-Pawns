#pragma once
// ═══ THE POSTCARD FACE — A PICTURE LEAVES THE WALL (POSTCARD_0) ═════════
//
// Two calls across the one seam, the ride face's grammar (ride_face.hpp):
// the shell owns the look, an absent global is a silent no-op, and it ships
// in every build so the teacher exists in the gallery and not only in the
// lab.
//
// postcard_face(mode) — ONE call per EDGE, never per frame: 0 hides the
// badge, 1 says a painting is before you, 2 says a photograph is.
//
// postcard_deliver(...) — the picture itself, once per take. `bytes` is the
// mapped readback: RES x RES texels, four bytes each, rows packed
// (bytesPerRow == RES * 4, 256-aligned by construction). It is read HERE,
// synchronously, into a JS array, because the range dies at Unmap. The
// shell receives RGBA with alpha forced opaque: the texel order (BGRA on
// Chrome's preferred surface format) is the engine's fact and stops at
// this seam, and a photograph's sky may carry a clear alpha the JPEG
// would otherwise paint black. `name` is a filename stem, ASCII, read
// from the heap by bytes: no runtime string helper is exported
// (console.hpp's rule — ccall/cwrap only), so nothing here calls one.
//
// EM_ASM BODIES: double quotes only, and no comma outside parentheses —
// the preprocessor splits the macro on it (console.hpp's own lesson).

#include <emscripten.h>
#include <cstdint>

namespace t7 {

    inline void postcard_face(uint32_t mode) {
        EM_ASM({
            if (typeof window !== "undefined" && window.T7_POSTCARD)
                window.T7_POSTCARD.set($0);
        }, mode);
    }

    inline void postcard_deliver(const void* bytes, uint32_t res, uint32_t bgra,
                                 uint32_t crop_w, uint32_t crop_h, double aspect,
                                 uint32_t kind, const char* name) {
        EM_ASM({
            if (typeof window === "undefined" || !window.T7_POSTCARD) return;
            var p = $0;
            var n = $1 * $1 * 4;
            var src = HEAPU8.subarray(p, p + n);
            var rgba = new Uint8ClampedArray(n);
            if ($2) {
                for (var i = 0; i < n; i += 4) {
                    rgba[i] = src[i + 2]; rgba[i + 1] = src[i + 1]; rgba[i + 2] = src[i]; rgba[i + 3] = 255;
                }
            } else {
                for (var j = 0; j < n; j += 4) {
                    rgba[j] = src[j]; rgba[j + 1] = src[j + 1]; rgba[j + 2] = src[j + 2]; rgba[j + 3] = 255;
                }
            }
            var s = "";
            var q = $7;
            while (HEAPU8[q]) { s += String.fromCharCode(HEAPU8[q] & 0x7f); q++; }
            window.T7_POSTCARD.deliver(rgba, $1, $3, $4, $5, $6, s);
        }, bytes, res, bgra, crop_w, crop_h, aspect, kind, name);
    }

} // namespace t7
