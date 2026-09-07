// ═══ THE DARKROOM (DARKROOM_1) — ONE HOME FOR DEVELOPING A PAINTING ═══════
//
// Everything between "the JPEG's decoded texels" and "bytes ready for the
// GPU" lives here, as pure functions on memory: the fit, the pad (with
// DARKROOM_0's copy at scale 1 and MIP_0's replicated edge), the texel-order
// swap, and MIP_0's chain. No wgpu, no Dim::, no emscripten — the numbers
// arrive as arguments so the SAME header compiles into the program (the
// main-thread arm: boot's solid fill, and the fallback when no worker can be
// opened) and into the darkroom worker's own wasm (src/darkroom/). Two
// builds, one source, so the bytes a worker develops are the bytes the main
// thread would have developed: byte identity by construction, and a native
// harness in the handoff proves it against the code these were lifted from.
//
// The arithmetic is the loader's and the uploader's VERBATIM — the bilinear
// arm, the memcpy arm, the replication, the swap, the (sum + 2) / 4 box
// mean. Nothing here was re-derived; it was moved.
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace t7 {
namespace develop {

// ── The fit: scale-to-fit, never up. Same clamp, same rounding as the loader.
struct Fit {
    float    scale = 1.0f;
    uint32_t dst_w = 0;
    uint32_t dst_h = 0;
};

inline Fit fit(uint32_t width, uint32_t height, uint32_t res) {
    Fit f;
    f.scale = std::min((float)res / width, (float)res / height);
    if (f.scale > 1.0f) f.scale = 1.0f;
    f.dst_w = std::min((uint32_t)(width * f.scale + 0.5f), res);
    f.dst_h = std::min((uint32_t)(height * f.scale + 0.5f), res);
    return f;
}

// ── The pad: a zero square of res x res texels holding the fitted picture
// at its origin, the pad beyond it replicated from the picture's edge.
// `padded` is sized here (res * res * 4).
inline void pad(const unsigned char* data, uint32_t width, uint32_t height,
                uint32_t res, const Fit& f, std::vector<uint8_t>& padded) {
    const uint32_t RES = res;
    const float scale = f.scale;
    const uint32_t dst_w = f.dst_w, dst_h = f.dst_h;
    padded.assign((size_t)RES * RES * 4, 0);

    // DARKROOM_0 — AT SCALE 1 THE BILINEAR IS A COPY, BYTE FOR BYTE.
    if (scale >= 1.0f) {
        for (uint32_t dy = 0; dy < dst_h; ++dy)
            std::memcpy(&padded[(size_t)dy * RES * 4],
                        &data[(size_t)dy * (size_t)width * 4],
                        (size_t)dst_w * 4);
    } else {
    for (uint32_t dy = 0; dy < dst_h; ++dy) {
        float src_yf = (float)dy / scale;
        uint32_t sy0 = (uint32_t)src_yf;
        uint32_t sy1 = std::min(sy0 + 1, (uint32_t)(height - 1));
        float fy = src_yf - sy0;
        for (uint32_t dx = 0; dx < dst_w; ++dx) {
            float src_xf = (float)dx / scale;
            uint32_t sx0 = (uint32_t)src_xf;
            uint32_t sx1 = std::min(sx0 + 1, (uint32_t)(width - 1));
            float fx = src_xf - sx0;
            uint32_t i00 = (sy0 * width + sx0) * 4;
            uint32_t i10 = (sy0 * width + sx1) * 4;
            uint32_t i01 = (sy1 * width + sx0) * 4;
            uint32_t i11 = (sy1 * width + sx1) * 4;
            uint32_t di = (dy * RES + dx) * 4;
            for (int c = 0; c < 4; ++c) {
                float v = (1 - fx) * (1 - fy) * data[i00 + c] + fx * (1 - fy) * data[i10 + c]
                    + (1 - fx) * fy * data[i01 + c] + fx * fy * data[i11 + c];
                padded[di + c] = (uint8_t)(v + 0.5f);
            }
        }
    }
    }   // the scale < 1 arm ends here

    // MIP_0 — THE PAD IS THE EDGE, REPLICATED.
    if (dst_w > 0 && dst_h > 0) {
        for (uint32_t dy = 0; dy < dst_h; ++dy) {
            const uint8_t* edge = &padded[((size_t)dy * RES + (dst_w - 1)) * 4];
            for (uint32_t dx = dst_w; dx < RES; ++dx)
                std::memcpy(&padded[((size_t)dy * RES + dx) * 4], edge, 4);
        }
        for (uint32_t dy = dst_h; dy < RES; ++dy)
            std::memcpy(&padded[(size_t)dy * RES * 4], &padded[(size_t)(dst_h - 1) * RES * 4], (size_t)RES * 4);
    }
}

// ── The texel order: RGBA -> BGRA (or back), in place. The uploader's swap.
inline void swap_rb(uint8_t* px, size_t texels) {
    for (size_t i = 0; i < texels; ++i) {
        const uint8_t r = px[i * 4 + 0];
        px[i * 4 + 0] = px[i * 4 + 2];   // B
        px[i * 4 + 2] = r;               // R
    }
}

// ── The chain's geometry: level k is (res >> k) square; the chain buffer
// holds levels 1..levels-1 back to back, so one WriteTexture per level reads
// it at chain_offset(k). One arithmetic, read by the worker and the uploader.
inline constexpr uint32_t level_side(uint32_t res, uint32_t level) { return res >> level; }
inline constexpr size_t   level_bytes(uint32_t res, uint32_t level) {
    return (size_t)level_side(res, level) * level_side(res, level) * 4u;
}
inline constexpr size_t chain_offset(uint32_t res, uint32_t level) {
    size_t off = 0;
    for (uint32_t k = 1; k < level; ++k) off += level_bytes(res, k);
    return off;
}
inline constexpr size_t chain_bytes(uint32_t res, uint32_t levels) {
    return chain_offset(res, levels);
}

// ── The chain: each level the 2x2 box mean of the level above, in the
// texel order already chosen. MIP_0's loop, writing a flat buffer instead
// of the queue. `out` must hold chain_bytes(res, levels).
inline void chain(const uint8_t* level0, uint32_t res, uint32_t levels, uint8_t* out) {
    std::vector<uint8_t> prev(level0, level0 + (size_t)res * res * 4);
    uint32_t w = res, h = res;
    for (uint32_t level = 1; level < levels && w > 1 && h > 1; ++level) {
        const uint32_t nw = w / 2, nh = h / 2;
        uint8_t* next = out + chain_offset(res, level);
        for (uint32_t y = 0; y < nh; ++y) {
            for (uint32_t x = 0; x < nw; ++x) {
                const size_t a = ((size_t)(2 * y) * w + 2 * x) * 4;
                const size_t b = a + (size_t)w * 4;
                for (uint32_t ch = 0; ch < 4; ++ch) {
                    const uint32_t sum = prev[a + ch] + prev[a + 4 + ch] + prev[b + ch] + prev[b + 4 + ch];
                    next[((size_t)y * nw + x) * 4 + ch] = (uint8_t)((sum + 2u) / 4u);
                }
            }
        }
        prev.assign(next, next + (size_t)nw * nh * 4);
        w = nw; h = nh;
    }
}

} // namespace develop
} // namespace t7
