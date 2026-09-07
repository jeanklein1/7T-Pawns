// ═══ THE DARKROOM (DARKROOM_1) — a painting develops off the frame ═══════
//
// A second, tiny wasm, built for a Worker: stb_image and core/develop.hpp
// and nothing else. The program's own thread used to decode, pad, swap and
// chain every arriving painting inside a frame (80-150 ms on a weak CPU, in
// batches — the stutter DARKROOM_0 named). Now the worker does exactly the
// same arithmetic — the same header, compiled twice — and the program hangs
// the result with one WriteTexture per frame.
//
// The contract, one call: bytes in, level 0 and the chain out, both malloc'd
// in this module's heap in the texel order the exhibition wants (bgra says
// which), sized by res and levels the caller passes (the program's Dim::
// values — no second home for them here). out[8]: w, h, dst_w, dst_h,
// level0 ptr, chain ptr, level0 bytes, chain bytes. Returns 0 on a failed
// decode.
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <emscripten/emscripten.h>

#include "external/stb_image.h"
#include "core/develop.hpp"

extern "C" {

EMSCRIPTEN_KEEPALIVE int t7_develop(const uint8_t* bytes, int len, uint32_t res, uint32_t levels,
                                    int bgra, uint32_t* out) {
    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load_from_memory(bytes, len, &width, &height, &channels, 4);
    if (!data) return 0;
    const t7::develop::Fit f = t7::develop::fit((uint32_t)width, (uint32_t)height, res);
    std::vector<uint8_t> padded;
    t7::develop::pad(data, (uint32_t)width, (uint32_t)height, res, f, padded);
    stbi_image_free(data);
    if (bgra) t7::develop::swap_rb(padded.data(), (size_t)res * res);
    const size_t l0_bytes = (size_t)res * res * 4;
    const size_t ch_bytes = t7::develop::chain_bytes(res, levels);
    uint8_t* level0 = (uint8_t*)std::malloc(l0_bytes);
    uint8_t* chain  = (uint8_t*)std::malloc(ch_bytes);
    if (!level0 || !chain) { std::free(level0); std::free(chain); return 0; }
    std::memcpy(level0, padded.data(), l0_bytes);
    t7::develop::chain(level0, res, levels, chain);
    out[0] = (uint32_t)width;  out[1] = (uint32_t)height;
    out[2] = f.dst_w;          out[3] = f.dst_h;
    out[4] = (uint32_t)(uintptr_t)level0;
    out[5] = (uint32_t)(uintptr_t)chain;
    out[6] = (uint32_t)l0_bytes;
    out[7] = (uint32_t)ch_bytes;
    return 1;
}

}
