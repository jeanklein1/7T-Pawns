# DARKROOM_1 — THE DARKROOM WORKER

*Handoff · authored 2026-09-07 · Claude → CC · Jean holds build, visual, merge, deploy.*
*Filed to `docs/HANDOFFS/DARKROOM_1.md` while open; the directory dies with the campaign.*

## THE IDEA, IN ONE PARAGRAPH

DARKROOM_0 named the stutter: a painting's arrival develops on the main thread — decode,
pad, swap, chain, upload — one per frame while the pen has content, each unit longer than a
frame. Scheduling cannot fix that; only moving it can. This round moves it, keeping every
byte: the develop (fit, pad, swap, chain) becomes pure functions in ONE header,
`core/develop.hpp`, lifted verbatim from the loader and the uploader; a second tiny wasm —
stb_image plus that header, nothing else — runs in a Web Worker and develops each held file
in arrival order, off the frame; the program hangs what comes back with one `WriteTexture`
per frame, after the world has been seen, exactly as AUBADE U5c's valve always did. No
shared memory, no COOP/COEP, no pthreads: a Worker is universal wherever WebGPU is. Route B′
as stamped. Byte identity by construction (one source, compiled twice) and by witness (the
harness below: 108 cases, 0 bytes differ). If no worker can open, the main-thread arm — the
same header — develops as before.

## AUTHORITY

Base: `master` at HEAD `f54e7310` (DARKROOM_0 settled), fetched 2026-09-07. Rides a held
branch `claude/darkroom-1` (P12: a build change, a shell change, a felt gate). HALT (P17,
scoped P15): unreachable file; stale authority. Else DEFAULT-AND-FLAG. Count **1**
everywhere. LF only. **Fences in this handoff: every FIND/REPLACE block opens and closes on
its own line — one variant, as promised.**

| file | blob at base |
| --- | --- |
| `src/cartridges/the_board/realization/state.hpp` | `fb0320fe88b56d704dd7ed945b0ce24ce2094ee9` |
| `src/cartridges/the_board/bodies/gallery.hpp` | `d8bcef2fed50877f7409cc59ac8e0c294dad1f65` |
| `CMakeLists.txt` | `3c706f551e407794ab1d82d5ad1c313410647471` |
| `tools/web_dist.py` | `a56da23f180f7e0fe747fa3574189a9b8247e4aa` |
| `web/index.html` | `897fd5de02d5f663dee6d9742b97cfd1ad2e6532` |
| `src/core/develop.hpp`, `src/darkroom/darkroom.cpp`, `web/darkroom_worker.js` | NEW — must not exist at base |

**Pinned:** `state.hpp` (BINDING + MIRROR), `gallery.hpp` (BINDING + COMMAND); red from U1
until U5. `binding_schema.py` is untouched: no resource changes hands — `upload_developed`
is a new WriteTexture site on the same authored staging, and R-2 still reaches every row.

**REHEARSED, WITH THE BUILD LEFT TO THE MACHINE.** All twelve FINDs at count 1; G-LAW 1
GREEN and the TU gate at zero diagnostics after U1 and U2 (the header's `EM_ASM`,
`EM_ASM_INT` and `EMSCRIPTEN_KEEPALIVE` type-check against the pinned surface);
`darkroom.cpp` parses against a stub `emscripten.h`; the worker script parses under node;
`web_dist.py` parses; the shell gate GREEN after U4; the cascade and every census green
after U5. **Not rehearsed, because the sandbox has no emsdk and no browser:** the emcc build
of both targets, the runtime, and the felt gate. U3's witness and Jean's gates are where
those are answered; DEFAULT-AND-FLAG applies to emcc's exact flag spellings.

**The byte-identity witness, run here:** the harness compiles the pad/swap/chain code lifted
from base `gallery.hpp` and `state.hpp` (WriteTexture replaced by a capture) beside
`develop.hpp`, and compares level 0 and all ten chain levels: **108 cases** — every
degenerate size CC used at DARKROOM_0, forty random sizes, above-cap controls, each with
and without the BGRA swap — **0 bytes differ.** The harness is reproduced at the end; CC ran
its own at DARKROOM_0 and can run this one the same way.

## THE RULINGS THIS CAMPAIGN LANDS

1. **One home for developing a painting.** `core/develop.hpp`: `fit`, `pad` (DARKROOM_0's
   copy at 1, the bilinear arm, MIP_0's replicated edge), `swap_rb`, `chain`, and the chain's
   geometry (`level_side`, `level_bytes`, `chain_offset`, `chain_bytes`). No wgpu, no `Dim::`,
   no emscripten — the numbers arrive as arguments so the same header compiles into the
   program and into the worker. Nothing was re-derived; it was moved.
2. **The worker develops; the frame hangs.** `src/darkroom/darkroom.cpp` exports one call,
   `t7_develop(bytes, len, res, levels, bgra, out[8])`: bytes in, level 0 and the chain out
   in the texel order the exhibition wants. `web/darkroom_worker.js` runs it one job at a
   time in arrival order and transfers the buffers back. `window.T7_DARKROOM` (index.html)
   opens the worker beside the program, copies replies into the program's heap, and calls
   `t7_darkroom_done`. `pump_authored_valve` hands every held file over the frame it is held
   and hangs one developed painting per frame — after first present, AUBADE's law unchanged.
3. **The uploader is one function.** `upload_developed(queue, layer, level0, chain)` writes
   level 0 and the ten chain levels; the main-thread arm (`upload_authored_painting`: boot's
   solid fill, the no-worker fallback) develops through the header and calls it. The wall
   cannot depend on which arm developed the bytes.
4. **Failure takes the road it always took.** A worker that cannot open leaves `isOpen()`
   false and the pump runs the main-thread arm verbatim; a decode that fails in the worker
   comes back as `width == 0` and releases the slot exactly as the main-thread failure did.
5. **The build exports what the bridge needs and nothing else.** The program adds `HEAPU8`
   to its runtime methods and `_malloc/_free` to its exports (the shell must copy developed
   bytes into the program's heap; `ccall`'s array path would put four megabytes on a 4 MB
   stack). The worker target is a second `add_executable`: `MODULARIZE`, `ENVIRONMENT=worker`,
   no filesystem, no main.
6. **The row DARKROOM_0 asked for.** Meter builds print `[METER] darkroom develop=… (worker)
   hang=… (main)` per painting — the two halves of the unit this round moved, measured.
7. **The dist knows the three artifacts** (`darkroom_worker.js`, `darkroom.js`,
   `darkroom.wasm`), pinned immutable and versioned with `BUILD` like the rest.

## UNITS, IN ORDER — every commit compiles

| unit | what | files | commit |
| --- | --- | --- | --- |
| U0 | preflight | — | none |
| U1 | the one home; the uploader; the loader's arm | `develop.hpp` (new), `state.hpp`, `gallery.hpp` | 1 |
| U2 | the door, the reply, the pump; the worker's C++ and JS | `gallery.hpp`, `darkroom.cpp` (new), `darkroom_worker.js` (new) | 1 |
| U3 | the build and the dist | `CMakeLists.txt`, `tools/web_dist.py` | 1 |
| U4 | the shell's bridge | `web/index.html` | 1 |
| U5 | ledgers; OPEN.md; push | `audit/*`, `docs/OPEN.md` | 1 |

---

## U0 — PREFLIGHT (no commit)
```
git fetch origin master && git checkout -b claude/darkroom-1 origin/master
git ls-tree HEAD src/cartridges/the_board/realization/state.hpp src/cartridges/the_board/bodies/gallery.hpp CMakeLists.txt tools/web_dist.py web/index.html
test ! -e src/core/develop.hpp && test ! -e src/darkroom && test ! -e web/darkroom_worker.js && echo "new files absent, as required"
grep -c "if (scale >= 1.0f) {" src/cartridges/the_board/bodies/gallery.hpp     # 1 — DARKROOM_0 is the base
sh tools/gates/glaw1/run.sh && python3 tools/gates/console_gate/run.py && python3 tools/gates/shell_gate/run.py
```

## U1 — THE ONE HOME

### NEW FILE `src/core/develop.hpp` — create verbatim (LF, no BOM)

```
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
```

### U1.1 — the uploader becomes one function; the main-thread arm develops through core/develop.hpp (`src/cartridges/the_board/realization/state.hpp`)

FIND
```
            void upload_authored_painting(wgpu::Queue& queue, uint32_t layer,
                const uint8_t* rgba_data, uint32_t width, uint32_t height)
            {
                bool need_swap = exhibition_is_bgra();   // POSTCARD_0 — one home for the texel order
                std::vector<uint8_t> swapped;
                const uint8_t* src = rgba_data;

                if (need_swap) {
                    uint32_t n = width * height * 4;
                    swapped.resize(n);
                    for (uint32_t i = 0; i < width * height; ++i) {
                        swapped[i * 4 + 0] = rgba_data[i * 4 + 2]; // B
                        swapped[i * 4 + 1] = rgba_data[i * 4 + 1]; // G
                        swapped[i * 4 + 2] = rgba_data[i * 4 + 0]; // R
                        swapped[i * 4 + 3] = rgba_data[i * 4 + 3]; // A
                    }
                    src = swapped.data();
                }

                wgpu::TexelCopyTextureInfo dest{};
                dest.texture = authoredStagingTexture_;
                dest.mipLevel = 0;
                dest.origin = { 0, 0, layer };
                dest.aspect = wgpu::TextureAspect::All;

                wgpu::TexelCopyBufferLayout layout{};
                layout.offset = 0;
                layout.bytesPerRow = width * 4;
                layout.rowsPerImage = height;

                wgpu::Extent3D extent = { width, height, 1 };
                queue.WriteTexture(&dest, src, width * height * 4, &layout, &extent);

                // MIP_0 — THE CHAIN, built here so every authored upload has
                // one (the loader's padded square and the boot's solid fill
                // alike): each level is the 2x2 box mean of the level above,
                // in the texel order already chosen. WriteTexture's row pitch
                // has no 256-byte law (that is the buffer copies'), so the
                // small levels write as they are. The loader replicates the
                // picture's edge across the pad before calling here, so no
                // level's border blends with black (gallery.hpp).
                std::vector<uint8_t> prev(src, src + (size_t)width * height * 4);
                uint32_t w = width, h = height;
                for (uint32_t level = 1; level < Dim::PAINTING_MIP_LEVELS && w > 1 && h > 1; ++level) {
                    const uint32_t nw = w / 2, nh = h / 2;
                    std::vector<uint8_t> next((size_t)nw * nh * 4);
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
                    dest.mipLevel = level;
                    layout.bytesPerRow = nw * 4;
                    layout.rowsPerImage = nh;
                    wgpu::Extent3D e = { nw, nh, 1 };
                    queue.WriteTexture(&dest, next.data(), (size_t)nw * nh * 4, &layout, &e);
                    prev.swap(next); w = nw; h = nh;
                }
            }

```
REPLACE
```
            // DARKROOM_1 — THE UPLOADER IS ONE FUNCTION: level 0 and the chain,
            // already in the texel order the exhibition wants, arrive as bytes
            // and go up as WriteTextures (level 0 then every chain level at
            // develop::chain_offset). Whoever developed them — the darkroom
            // worker (src/darkroom/) or this thread (upload_authored_painting,
            // below: boot's solid fill and the no-worker fallback) — the bytes
            // came out of core/develop.hpp, so the wall cannot depend on which.
            void upload_developed(wgpu::Queue& queue, uint32_t layer,
                const uint8_t* level0, const uint8_t* chain)
            {
                constexpr uint32_t RES = Dim::PAINTING_RESOLUTION;
                wgpu::TexelCopyTextureInfo dest{};
                dest.texture = authoredStagingTexture_;
                dest.mipLevel = 0;
                dest.origin = { 0, 0, layer };
                dest.aspect = wgpu::TextureAspect::All;
                wgpu::TexelCopyBufferLayout layout{};
                layout.offset = 0;
                layout.bytesPerRow = RES * 4;
                layout.rowsPerImage = RES;
                wgpu::Extent3D extent = { RES, RES, 1 };
                queue.WriteTexture(&dest, level0, (size_t)RES * RES * 4, &layout, &extent);
                // MIP_0's chain: WriteTexture's row pitch has no 256-byte law
                // (that is the buffer copies'), so the small levels write as
                // they are.
                for (uint32_t level = 1; level < Dim::PAINTING_MIP_LEVELS; ++level) {
                    const uint32_t side = t7::develop::level_side(RES, level);
                    if (side == 0) break;
                    dest.mipLevel = level;
                    layout.bytesPerRow = side * 4;
                    layout.rowsPerImage = side;
                    wgpu::Extent3D e = { side, side, 1 };
                    queue.WriteTexture(&dest, chain + t7::develop::chain_offset(RES, level),
                                       t7::develop::level_bytes(RES, level), &layout, &e);
                }
            }
            // The main-thread arm: swap and chain here (core/develop.hpp), then
            // upload. Boot's solid fill and the no-worker fallback use it.
            void upload_authored_painting(wgpu::Queue& queue, uint32_t layer,
                const uint8_t* rgba_data, uint32_t width, uint32_t height)
            {
                constexpr uint32_t RES = Dim::PAINTING_RESOLUTION;
                (void)width; (void)height;   // every caller hands a RES x RES square (the pad, the fill)
                std::vector<uint8_t> level0(rgba_data, rgba_data + (size_t)RES * RES * 4);
                if (exhibition_is_bgra())   // POSTCARD_0 — one home for the texel order
                    t7::develop::swap_rb(level0.data(), (size_t)RES * RES);
                std::vector<uint8_t> chain(t7::develop::chain_bytes(RES, Dim::PAINTING_MIP_LEVELS));
                t7::develop::chain(level0.data(), RES, Dim::PAINTING_MIP_LEVELS, chain.data());
                upload_developed(queue, layer, level0.data(), chain.data());
            }
```

### U1.2 — state.hpp reads the darkroom's home (`src/cartridges/the_board/realization/state.hpp`)

FIND
```
#include "core/boot_params.hpp"                  // DOMESDAY_2 B10: effective_msaa — boot-read sample count for the snapshot targets
```
REPLACE
```
#include "core/boot_params.hpp"                  // DOMESDAY_2 B10: effective_msaa — boot-read sample count for the snapshot targets
#include "core/develop.hpp"                  // DARKROOM_1 — the one home for developing a painting (the chain, the swap)
```

### U1.3 — gallery.hpp reads the darkroom's home (`src/cartridges/the_board/bodies/gallery.hpp`)

FIND
```
#include "external/stb_image.h"                                // stbi_load/stbi_image_free — authored painting loader (dependency named here)
```
REPLACE
```
#include "external/stb_image.h"                                // stbi_load/stbi_image_free — authored painting loader (dependency named here)
#include "core/develop.hpp"                                    // DARKROOM_1 — the one home for developing a painting
#include <cstdlib>                                                // std::free — the darkroom's buffers
```

### U1.4 — the loader's main-thread arm: fit and pad through develop.hpp; the record splits out (`src/cartridges/the_board/bodies/gallery.hpp`)

FIND
```
inline void authored_stage_decoded_image(GalleryState& gs, GPUState& gpu, wgpu::Queue& queue,
    uint32_t staging_layer, uint32_t disk_index,
    const unsigned char* data, int width, int height) {
    // AUBADE U1 — the decode window opens here. The stb decode itself
    // happened in the caller; what this body costs is the SCALE and the
    // PAD, which is main-thread pixel work by any name and belongs in the
    // same number.
    const auto t_decode0 = std::chrono::steady_clock::now();
    constexpr uint32_t RES = Dim::PAINTING_RESOLUTION;
    float scale = std::min((float)RES / width, (float)RES / height);
    if (scale > 1.0f) scale = 1.0f;
    uint32_t dst_w = std::min((uint32_t)(width * scale + 0.5f), RES);
    uint32_t dst_h = std::min((uint32_t)(height * scale + 0.5f), RES);

    std::vector<uint8_t> padded(RES * RES * 4, 0);
    // DARKROOM_0 — AT SCALE 1 THE BILINEAR IS A COPY, BYTE FOR BYTE. Since
    // PLATE_0 the dist caps every shipped painting at RES on its long side
    // and this loader never scales UP, so `scale` is exactly 1.0 for every
    // file that can reach here: src_xf == dx and src_yf == dy exactly, the
    // fractions are exact zeros, v == data[i00] exactly, and (uint8_t)(v +
    // 0.5f) is data[i00] again for every value 0..255. The four-tap float
    // loop below was therefore computing a memcpy at ~1M texels x 4
    // channels x 4 taps, on the main thread, once per painting, inside the
    // frame — the largest single share of the arrival stall after the
    // decode itself. The arm below it stays for the one case that can
    // still need it: a master past the cap that reached dist by hand.
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
    }   // DARKROOM_0 — the scale < 1 arm ends here

    // MIP_0 — THE PAD IS THE EDGE, REPLICATED. The square beyond dst_w x
    // dst_h was zero (transparent black). A chain averages 2x2 blocks, so
    // at every coarser level the picture's border texels would blend with
    // the pad and a dark rim would grow with distance. Replicating the last
    // column and the last row across the pad makes every level's border
    // blend with itself. The quads never sample the pad directly (uv runs 0
    // to uv_scale) and the postcard crops it away (crop_w / crop_h), so
    // nothing else moves.
    if (dst_w > 0 && dst_h > 0) {
        for (uint32_t dy = 0; dy < dst_h; ++dy) {
            const uint8_t* edge = &padded[((size_t)dy * RES + (dst_w - 1)) * 4];
            for (uint32_t dx = dst_w; dx < RES; ++dx)
                std::memcpy(&padded[((size_t)dy * RES + dx) * 4], edge, 4);
        }
        for (uint32_t dy = dst_h; dy < RES; ++dy)
            std::memcpy(&padded[(size_t)dy * RES * 4], &padded[(size_t)(dst_h - 1) * RES * 4], (size_t)RES * 4);
    }
    gpu.upload_authored_painting(queue, staging_layer, padded.data(), RES, RES);

    auto& rec = gs.authored_staging[staging_layer];
    rec.disk_index = disk_index;
    rec.aspect_ratio = (height > 0) ? (float)width / (float)height : 1.0f;
    rec.uv_scale_x = (float)dst_w / RES;
    rec.uv_scale_y = (float)dst_h / RES;
    rec.valid = true;
    // SHOWN CATCHES UP TO CLAIMED, in the same instant `valid` becomes true
    // (WALLS_3). A reader that saw one without the other would see a valid
    // slot showing nothing, or a slot showing a picture it no longer holds.
    rec.shown_disk_index = rec.disk_index;

    // AN ARRIVAL CLEARS THIS INDEX'S SENTENCE AND NOTHING ELSE (REPEAT_0c
    // U-S2). The cooldown is spent — a later failure here earns a fresh full
    // one rather than inheriting a stale stamp.
    //
    // IT DELIBERATELY DOES NOT RE-ARM THE EXHAUSTION LINE. Three versions
    // were measured over ten simulated minutes: re-arming on a successful
    // TAKE printed the sentence 1084 times under total outage (every
    // expiring cooldown read as a fresh episode); re-arming HERE printed it
    // 337 times under a 30% flake, because arrivals kept clearing the floor
    // faster than the floor could bite. Leaving the stamp alone makes the
    // floor absolute — at most one sentence per AUTHORED_DEAD_COOLDOWN_S,
    // measured at 20 lines in ten minutes of total outage and 20 under
    // flake. The cost, stated: a second outage beginning inside 30 s of a
    // reported one is silent, which is the same outage by any honest name.
    if (disk_index < (uint32_t)gs.authored_manifest_dead_until.size())
        gs.authored_manifest_dead_until[disk_index] = -1.0f;

    // AUBADE U1 — HOW MUCH OF THE DARK WAS PAINTING DECODE. R6 found the
    // authored path decodes with stb_image ON THE MAIN THREAD, so this is
    // the number that separates "the paintings did it" from a device-side
    // wait. Summed only before first present; after it, the question has
    // been answered and the add stops.
    if (!t7::aubade_presented()) {
        t7::aubade_stb() += std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - t_decode0).count();
    }

    // AUBADE U1 — THE SIXTH IS THE ONE THAT MATTERS. Six staged images is
    // the READY floor (OVERTURE_0 U9), so `authored6` is the mark that says
    // when the offer could have been made. Counted here, at the moment a
    // slot becomes valid, because that is what "staged" means.
    {
        static uint32_t staged = 0;
        if (++staged == 6u) t7::aubade_mark("authored6");
    }
    std::cout << "[Authored] Scaled → " << dst_w << "x" << dst_h
        << " (aspect " << rec.aspect_ratio << ")\n";
}
```
REPLACE
```
// DARKROOM_1 — the record, written when a staging layer becomes valid, by
// whichever arm developed it. `stb_ms` is the main-thread decode's time
// for AUBADE's accumulator (negative when the darkroom developed it: no
// main-thread decode happened, and the accumulator is a main-thread fact).
inline void authored_record_staged(GalleryState& gs, uint32_t staging_layer, uint32_t disk_index,
                                   uint32_t width, uint32_t height, uint32_t dst_w, uint32_t dst_h,
                                   double stb_ms) {
    constexpr uint32_t RES = Dim::PAINTING_RESOLUTION;
    auto& rec = gs.authored_staging[staging_layer];
    rec.disk_index = disk_index;
    rec.aspect_ratio = (height > 0) ? (float)width / (float)height : 1.0f;
    rec.uv_scale_x = (float)dst_w / RES;
    rec.uv_scale_y = (float)dst_h / RES;
    rec.valid = true;
    // SHOWN CATCHES UP TO CLAIMED, in the same instant `valid` becomes true
    // (WALLS_3). A reader that saw one without the other would see a valid
    // slot showing nothing, or a slot showing a picture it no longer holds.
    rec.shown_disk_index = rec.disk_index;

    // AN ARRIVAL CLEARS THIS INDEX'S SENTENCE AND NOTHING ELSE (REPEAT_0c
    // U-S2). The cooldown is spent — a later failure here earns a fresh full
    // one rather than inheriting a stale stamp.
    //
    // IT DELIBERATELY DOES NOT RE-ARM THE EXHAUSTION LINE. Three versions
    // were measured over ten simulated minutes: re-arming on a successful
    // TAKE printed the sentence 1084 times under total outage (every
    // expiring cooldown read as a fresh episode); re-arming HERE printed it
    // 337 times under a 30% flake, because arrivals kept clearing the floor
    // faster than the floor could bite. Leaving the stamp alone makes the
    // floor absolute — at most one sentence per AUTHORED_DEAD_COOLDOWN_S,
    // measured at 20 lines in ten minutes of total outage and 20 under
    // flake. The cost, stated: a second outage beginning inside 30 s of a
    // reported one is silent, which is the same outage by any honest name.
    if (disk_index < (uint32_t)gs.authored_manifest_dead_until.size())
        gs.authored_manifest_dead_until[disk_index] = -1.0f;

    // AUBADE U1 — HOW MUCH OF THE DARK WAS PAINTING DECODE. R6 found the
    // authored path decodes with stb_image ON THE MAIN THREAD, so this is
    // the number that separates "the paintings did it" from a device-side
    // wait. Summed only before first present; after it, the question has
    // been answered and the add stops.
    if (!t7::aubade_presented() && stb_ms >= 0.0) {
        t7::aubade_stb() += stb_ms;
    }

    // AUBADE U1 — THE SIXTH IS THE ONE THAT MATTERS. Six staged images is
    // the READY floor (OVERTURE_0 U9), so `authored6` is the mark that says
    // when the offer could have been made. Counted here, at the moment a
    // slot becomes valid, because that is what "staged" means.
    {
        static uint32_t staged = 0;
        if (++staged == 6u) t7::aubade_mark("authored6");
    }
    std::cout << "[Authored] Scaled → " << dst_w << "x" << dst_h
        << " (aspect " << rec.aspect_ratio << ")\n";
}
inline void authored_stage_decoded_image(GalleryState& gs, GPUState& gpu, wgpu::Queue& queue,
    uint32_t staging_layer, uint32_t disk_index,
    const unsigned char* data, int width, int height) {
    // DARKROOM_1 — THE MAIN-THREAD ARM. The fit, the pad (DARKROOM_0's copy,
    // MIP_0's replicated edge), the swap and the chain moved to
    // core/develop.hpp, the one home the darkroom worker compiles too; this
    // arm runs only when no worker could be opened. Byte-identical to the
    // loader it replaces — the handoff's native harness is the witness.
    const auto t_decode0 = std::chrono::steady_clock::now();
    constexpr uint32_t RES = Dim::PAINTING_RESOLUTION;
    const t7::develop::Fit f = t7::develop::fit((uint32_t)width, (uint32_t)height, RES);
    std::vector<uint8_t> padded;
    t7::develop::pad(data, (uint32_t)width, (uint32_t)height, RES, f, padded);
    gpu.upload_authored_painting(queue, staging_layer, padded.data(), RES, RES);
    const double ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t_decode0).count();
    authored_record_staged(gs, staging_layer, disk_index, (uint32_t)width, (uint32_t)height, f.dst_w, f.dst_h, ms);
}

```


### WITNESS — U1
```
grep -c "namespace develop" src/core/develop.hpp                                          # 2 — the opening and the closing comment
grep -c "void upload_developed" src/cartridges/the_board/realization/state.hpp           # 1
grep -c "t7::develop::" src/cartridges/the_board/realization/state.hpp                   # 6
grep -c "authored_record_staged" src/cartridges/the_board/bodies/gallery.hpp             # 2 — the definition and the arm's call (U2 makes it 3)
sh tools/gates/glaw1/run.sh && python3 tools/gates/console_gate/run.py                   # GREEN / PASS
```
Then the harness (end of this handoff), natively: `108 cases, 0 fails` — or CC's own from
DARKROOM_0, pointed at `develop.hpp`.

Commit: `DARKROOM_1 U1 — core/develop.hpp: the fit, the pad, the swap and the chain as pure functions, lifted verbatim (native harness: 108 cases, 0 bytes differ); the uploader is one function; the loader's main-thread arm reads the one home`

## U2 — THE DOOR, THE REPLY, THE PUMP; THE WORKER

### NEW FILE `src/darkroom/darkroom.cpp` — create verbatim (LF, no BOM)

```
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
```

### NEW FILE `web/darkroom_worker.js` — create verbatim (LF, no BOM)

```
// ═══ THE DARKROOM WORKER (DARKROOM_1) ══════════════════════════════════════
// Runs darkroom.js (src/darkroom/ built as a Worker module). One job at a
// time, in arrival order: the program's held JPEG bytes come in, level 0 and
// the chain go back as transferred buffers. Everything here is a copy in or
// out of the module's heap; the arithmetic is the module's (core/develop.hpp).
'use strict';
var BUILD = (function () {
  var m = /[?&]v=([^&]+)/.exec(self.location.search);
  return m ? m[1] : '';
})();
function versioned(f) { return f + (BUILD ? '?v=' + BUILD : ''); }
importScripts(versioned('darkroom.js'));

var mod = null;
var pending = [];

createDarkroom({ locateFile: versioned }).then(function (m) {
  mod = m;
  self.postMessage({ ready: true });
  while (pending.length) develop(pending.shift());
});

function develop(job) {
  var t0 = performance.now();
  var n = job.bytes.byteLength;
  var p = mod._malloc(n);
  mod.HEAPU8.set(new Uint8Array(job.bytes), p);
  var out = mod._malloc(8 * 4);
  var ok = mod._t7_develop(p, n, job.res, job.levels, job.bgra ? 1 : 0, out);
  mod._free(p);
  if (!ok) { mod._free(out); self.postMessage({ layer: job.layer, ok: false }); return; }
  // Read after the call: the heap may have grown, and the module's views are fresh.
  var o = out >> 2;
  var w = mod.HEAPU32[o], h = mod.HEAPU32[o + 1], dw = mod.HEAPU32[o + 2], dh = mod.HEAPU32[o + 3];
  var p0 = mod.HEAPU32[o + 4], pc = mod.HEAPU32[o + 5], n0 = mod.HEAPU32[o + 6], nc = mod.HEAPU32[o + 7];
  var level0 = mod.HEAPU8.slice(p0, p0 + n0);   // copies out; the transfer below hands the copy over
  var chain  = mod.HEAPU8.slice(pc, pc + nc);
  mod._free(p0); mod._free(pc); mod._free(out);
  self.postMessage({ layer: job.layer, ok: true, w: w, h: h, dw: dw, dh: dh,
                     level0: level0.buffer, chain: chain.buffer,
                     ms: performance.now() - t0 },
                   [level0.buffer, chain.buffer]);
}

self.onmessage = function (e) {
  if (mod) develop(e.data); else pending.push(e.data);
};
```

### U2.1 — the state knows what the worker holds and what it developed (`src/cartridges/the_board/bodies/gallery.hpp`)

FIND
```
    std::vector<AuthoredHeld> authored_held;
```
REPLACE
```
    std::vector<AuthoredHeld> authored_held;
    // DARKROOM_1 — what the worker holds (by staging layer: a layer is claimed
    // until released, so it is the job's name) and what it has developed,
    // waiting to be hung one per frame. The buffers are this heap's, malloc'd
    // by the shell on the worker's reply; the hang frees them.
    struct AuthoredInflight {
        uint32_t disk_index = 0;
        std::string url;
        bool active = false;
    };
    AuthoredInflight authored_inflight[Dim::STAGING_LAYERS];
    struct AuthoredDeveloped {
        uint32_t staging_layer;
        uint32_t disk_index;
        std::string url;
        uint32_t width, height, dst_w, dst_h;
        uint8_t* level0;
        uint8_t* chain;
        double develop_ms;
    };
    std::vector<AuthoredDeveloped> authored_developed;
```

### U2.2 — the darkroom's door, the worker's reply, and the pump that only hangs (`src/cartridges/the_board/bodies/gallery.hpp`)

FIND
```
inline void pump_authored_valve(GalleryState& gs, GPUState& gpu, wgpu::Queue& queue) {
    if (gs.authored_held.empty()) return;
    // FIRST PRESENT, and the flag is the same latch the waterfall reads
    // (core/aubade.hpp) — one fact about the boot, one home, no second
    // opinion about whether the world has been seen.
    if (!t7::aubade_presented()) return;

    GalleryState::AuthoredHeld h = std::move(gs.authored_held.front());
    gs.authored_held.erase(gs.authored_held.begin());

    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load_from_memory(
        h.bytes.data(), (int)h.bytes.size(), &width, &height, &channels, 4);
    if (!data) {
        std::cerr << "[Authored] Failed to decode: " << h.url << "\n";
        // 0 — the bytes arrived and stb refused them; the failure is not the
        // network's, and the ring is healed the same way regardless.
        authored_fetch_release_slot(gs, h.staging_layer, 0);
        pump_authored_fetches(gs);
        recount_authored_staged(gs);
        return;
    }
    std::cout << "[Authored] Loaded: " << h.url
        << " (" << width << "x" << height << ") → staging " << h.staging_layer << "\n";
    authored_stage_decoded_image(gs, gpu, queue, h.staging_layer, h.disk_index,
                                 data, width, height);
    stbi_image_free(data);
    // The journey is over: the slot holds a picture, so `pending` — which
    // U5c widened to span fetch AND valve — comes down here.
    gs.authored_staging[h.staging_layer].pending = false;
    recount_authored_staged(gs);
}
```
REPLACE
```
// DARKROOM_1 — THE DARKROOM'S DOOR. window.T7_DARKROOM (index.html) owns a
// Worker running src/darkroom/'s wasm; this thread hands it the held bytes
// and later receives level 0 and the chain, developed, as malloc'd buffers
// in this heap (t7_darkroom_done, below). No worker: the main-thread arm.
inline bool darkroom_open() {
#ifdef __EMSCRIPTEN__
    return EM_ASM_INT({ return (window.T7_DARKROOM && window.T7_DARKROOM.isOpen()) ? 1 : 0; }) != 0;
#else
    return false;
#endif
}
inline GalleryState* g_darkroom_gs = nullptr;   // bound by the pump; the worker's reply has no other way in

extern "C" {
// The worker's reply (index.html copies the developed bytes into this heap
// and calls here). width == 0 is a failed decode — the same road the
// main-thread arm's failure takes.
EMSCRIPTEN_KEEPALIVE inline void t7_darkroom_done(uint32_t layer, uint32_t width, uint32_t height,
                                                  uint32_t dst_w, uint32_t dst_h,
                                                  uint8_t* level0, uint8_t* chain, double develop_ms) {
    GalleryState* gsp = g_darkroom_gs;
    if (!gsp || layer >= Dim::STAGING_LAYERS) { std::free(level0); std::free(chain); return; }
    GalleryState& gs = *gsp;
    GalleryState::AuthoredInflight in = gs.authored_inflight[layer];
    gs.authored_inflight[layer] = GalleryState::AuthoredInflight{};
    if (!in.active) { std::free(level0); std::free(chain); return; }
    if (width == 0 || !level0 || !chain) {
        std::cerr << "[Authored] Failed to develop: " << in.url << "\n";
        std::free(level0); std::free(chain);
        authored_fetch_release_slot(gs, layer, 0);
        pump_authored_fetches(gs);
        recount_authored_staged(gs);
        return;
    }
    gs.authored_developed.push_back(GalleryState::AuthoredDeveloped{
        layer, in.disk_index, in.url, width, height, dst_w, dst_h, level0, chain, develop_ms });
}
}

inline void pump_authored_valve(GalleryState& gs, GPUState& gpu, wgpu::Queue& queue) {
    // DARKROOM_1 — THE PEN DEVELOPS IN THE WORKER; THIS THREAD ONLY HANGS.
    // Every held file goes to the darkroom the frame it is held (the worker
    // develops in arrival order, one at a time, off this thread). What comes
    // back is hung ONE PER FRAME, after the world has been seen — AUBADE
    // U5c's law, unchanged; only the decode, the pad and the chain left the
    // frame. The unit this thread pays per painting is now the WriteTexture.
    if (darkroom_open()) {
        g_darkroom_gs = &gs;
        while (!gs.authored_held.empty()) {
            GalleryState::AuthoredHeld h = std::move(gs.authored_held.front());
            gs.authored_held.erase(gs.authored_held.begin());
            gs.authored_inflight[h.staging_layer] = GalleryState::AuthoredInflight{ h.disk_index, h.url, true };
#ifdef __EMSCRIPTEN__
            EM_ASM({ window.T7_DARKROOM.submit($0, $1, $2, $3, $4, $5); },
                   (int)h.staging_layer, h.bytes.data(), (int)h.bytes.size(),
                   (int)Dim::PAINTING_RESOLUTION, (int)Dim::PAINTING_MIP_LEVELS,
                   gpu.exhibition_is_bgra() ? 1 : 0);
#endif
        }
        if (gs.authored_developed.empty()) return;
        if (!t7::aubade_presented()) return;
        GalleryState::AuthoredDeveloped d = std::move(gs.authored_developed.front());
        gs.authored_developed.erase(gs.authored_developed.begin());
        const auto t_up0 = std::chrono::steady_clock::now();
        gpu.upload_developed(queue, d.staging_layer, d.level0, d.chain);
        std::free(d.level0);
        std::free(d.chain);
        const double up_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - t_up0).count();
        std::cout << "[Authored] Developed: " << d.url
            << " (" << d.width << "x" << d.height << ")  staging " << d.staging_layer << "\n";
        if constexpr (t7::INSTRUMENTS.frame_meter) {
            // THE ROW DARKROOM_0 ASKED FOR: the develop, measured in the
            // worker, and the hang, measured here — the two halves of the
            // unit this campaign moved.
            std::cout << "[METER] darkroom  develop=" << d.develop_ms << " ms (worker)"
                      << "  hang=" << up_ms << " ms (main)  staging " << d.staging_layer << "\n";
        }
        authored_record_staged(gs, d.staging_layer, d.disk_index, d.width, d.height, d.dst_w, d.dst_h, -1.0);
        gs.authored_staging[d.staging_layer].pending = false;
        recount_authored_staged(gs);
        return;
    }
    // THE MAIN-THREAD ARM (no darkroom): AUBADE U5c's valve, verbatim.
    if (gs.authored_held.empty()) return;
    // FIRST PRESENT, and the flag is the same latch the waterfall reads
    // (core/aubade.hpp) — one fact about the boot, one home, no second
    // opinion about whether the world has been seen.
    if (!t7::aubade_presented()) return;

    GalleryState::AuthoredHeld h = std::move(gs.authored_held.front());
    gs.authored_held.erase(gs.authored_held.begin());

    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load_from_memory(
        h.bytes.data(), (int)h.bytes.size(), &width, &height, &channels, 4);
    if (!data) {
        std::cerr << "[Authored] Failed to decode: " << h.url << "\n";
        // 0 — the bytes arrived and stb refused them; the failure is not the
        // network's, and the ring is healed the same way regardless.
        authored_fetch_release_slot(gs, h.staging_layer, 0);
        pump_authored_fetches(gs);
        recount_authored_staged(gs);
        return;
    }
    std::cout << "[Authored] Loaded: " << h.url
        << " (" << width << "x" << height << ") → staging " << h.staging_layer << "\n";
    authored_stage_decoded_image(gs, gpu, queue, h.staging_layer, h.disk_index,
                                 data, width, height);
    stbi_image_free(data);
    // The journey is over: the slot holds a picture, so `pending` — which
    // U5c widened to span fetch AND valve — comes down here.
    gs.authored_staging[h.staging_layer].pending = false;
    recount_authored_staged(gs);
}
```


### WITNESS — U2
```
grep -c "t7_darkroom_done" src/cartridges/the_board/bodies/gallery.hpp                   # 2 — the banner's mention and the definition
grep -c "T7_DARKROOM.submit" src/cartridges/the_board/bodies/gallery.hpp                 # 1
grep -c "THE MAIN-THREAD ARM (no darkroom)" src/cartridges/the_board/bodies/gallery.hpp  # 1 — the old valve, verbatim, below the new
grep -c "t7_develop" src/darkroom/darkroom.cpp                                           # 1
node --check web/darkroom_worker.js                                                      # parses
sh tools/gates/glaw1/run.sh && python3 tools/gates/console_gate/run.py                   # GREEN / PASS
```
Commit: `DARKROOM_1 U2 — the darkroom: src/darkroom/ (stb + develop.hpp as a Worker wasm), web/darkroom_worker.js, the door, the worker's reply, and a pump that only hangs (one WriteTexture per frame after the world has been seen)`

## U3 — THE BUILD AND THE DIST

### U3.1 — the program exports the heap and the allocator the shell's bridge needs (`CMakeLists.txt`)

FIND
```
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
        "-sALLOW_MEMORY_GROWTH=1"
```
REPLACE
```
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','HEAPU8']"   # DARKROOM_1 — the shell copies developed bytes into this heap
        "-sEXPORTED_FUNCTIONS=['_main','_malloc','_free']"        # DARKROOM_1 — and allocates them here; KEEPALIVE exports ride alongside
        "-sALLOW_MEMORY_GROWTH=1"
```

### U3.2 — the darkroom worker's wasm, a second target (`CMakeLists.txt`)

FIND
```
        LINK_DEPENDS "${T7_WEB_SHADER}"
    )
endif()
```
REPLACE
```
        LINK_DEPENDS "${T7_WEB_SHADER}"
    )

    # ═══ DARKROOM_1 — THE DARKROOM WORKER'S WASM ═══════════════════════════
    # A second, tiny program: stb_image + core/develop.hpp, nothing else, built
    # for a Worker (ENVIRONMENT=worker, MODULARIZE so the worker script owns
    # its instance). No FETCH, no filesystem, no main. It develops a painting
    # off the main thread; the main program hangs what comes back. The same
    # develop.hpp compiles into both, which is the whole point.
    add_executable(the_board_darkroom
        src/darkroom/darkroom.cpp
        src/external/stb_image.cpp
    )
    target_include_directories(the_board_darkroom PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_compile_options(the_board_darkroom PRIVATE ${T7_WEB_OPT})
    target_link_options(the_board_darkroom PRIVATE
        "-sMODULARIZE=1"
        "-sEXPORT_NAME=createDarkroom"
        "-sENVIRONMENT=worker"
        "-sEXPORTED_FUNCTIONS=['_t7_develop','_malloc','_free']"
        "-sEXPORTED_RUNTIME_METHODS=['HEAPU8','HEAPU32']"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sINITIAL_MEMORY=33554432"
        "-sFILESYSTEM=0"
        "--no-entry"
        ${T7_WEB_OPT}
    )
    set_target_properties(the_board_darkroom PROPERTIES
        OUTPUT_NAME "darkroom"
        SUFFIX ".js"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/web"
    )
endif()
```

### U3.3 — the dist knows the three new artifacts (`tools/web_dist.py`)

FIND
```
ARTIFACTS = ["index.html", "organ_panel.js", "the_board.js", "the_board.wasm", "the_board.data"]
```
REPLACE
```
ARTIFACTS = ["index.html", "organ_panel.js", "the_board.js", "the_board.wasm", "the_board.data",
             "darkroom_worker.js", "darkroom.js", "darkroom.wasm"]   # DARKROOM_1 — the worker and its wasm
```

### U3.4 — and pins them immutable like the rest (`tools/web_dist.py`)

FIND
```
IMMUTABLE_PATHS = ["the_board.js", "the_board.wasm", "the_board.data",
                   "organ_panel.js"]
```
REPLACE
```
IMMUTABLE_PATHS = ["the_board.js", "the_board.wasm", "the_board.data",
                   "organ_panel.js",
                   "darkroom_worker.js", "darkroom.js", "darkroom.wasm"]   # DARKROOM_1
```


### WITNESS — U3 (this is where the sandbox stops and the machine begins)
```
cmake --preset the-board-web && cmake --build --preset the-board-web          # BOTH targets link: web/the_board.js and web/darkroom.js (+ darkroom.wasm)
ls -l web/darkroom.js web/darkroom.wasm web/darkroom_worker.js
python3 tools/web_dist.py                                                      # the inventory lists the three; the boot set's bytes are RECORDED, not optimized (SHIP_0 U4)
```
DEFAULT-AND-FLAG on emcc's spelling of any `-s` flag (the quoting follows the tree's own
lines above it); the outcome that matters is the two link products. If `-sFILESYSTEM=0`
refuses because stb's implementation TU wants stdio, drop that one flag and report it.

Commit: `DARKROOM_1 U3 — the build: the darkroom target; the program exports HEAPU8, malloc and free for the bridge; the dist knows the three artifacts`

## U4 — THE SHELL'S BRIDGE

### U4.1 — the bridge: window.T7_DARKROOM (`web/index.html`)

FIND
```
    window.T7_KEEP = function (blob, name, type, tag) {
```
REPLACE
```
    // ═══ DARKROOM_1 — THE DARKROOM: a Worker develops paintings off the frame ═══
    // The program hands a held JPEG's bytes here (T7_DARKROOM.submit, from
    // pump_authored_valve); the worker decodes, pads and chains them in its
    // own wasm (darkroom.js, the same core/develop.hpp the program compiles);
    // the reply's bytes are copied into the program's heap and handed back
    // through t7_darkroom_done. If the worker cannot open, isOpen() says so
    // and the program develops on its own thread, as it did before.
    window.T7_DARKROOM = (function () {
      var worker = null, ready = false, queue = [];
      var ARGS = ['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number'];
      function post(job) { worker.postMessage(job, [job.bytes.buffer]); }
      function done(m) {
        var M = window.Module;
        if (!m.ok) { M.ccall('t7_darkroom_done', null, ARGS, [m.layer, 0, 0, 0, 0, 0, 0, 0]); return; }
        var l0 = new Uint8Array(m.level0), ch = new Uint8Array(m.chain);
        var p0 = M._malloc(l0.length); M.HEAPU8.set(l0, p0);
        var pc = M._malloc(ch.length); M.HEAPU8.set(ch, pc);
        M.ccall('t7_darkroom_done', null, ARGS, [m.layer, m.w, m.h, m.dw, m.dh, p0, pc, m.ms]);
      }
      return {
        open: function (build) {
          if (typeof Worker === 'undefined') return;
          try {
            worker = new Worker('darkroom_worker.js' + (build ? '?v=' + build : ''));
          } catch (e) { worker = null; return; }
          worker.onmessage = function (e) {
            var m = e.data;
            if (m.ready) { ready = true; while (queue.length) post(queue.shift()); return; }
            done(m);
          };
          worker.onerror = function (e) {
            console.log('[Darkroom] worker failed: ' + (e && e.message ? e.message : 'unknown') + ' — developing on the main thread');
            worker = null; ready = false; queue = [];
          };
        },
        isOpen: function () { return worker !== null; },
        submit: function (layer, ptr, len, res, levels, bgra) {
          var bytes = new Uint8Array(len);
          bytes.set(window.Module.HEAPU8.subarray(ptr, ptr + len));
          var job = { layer: layer, bytes: bytes, res: res, levels: levels, bgra: !!bgra };
          if (ready) post(job); else queue.push(job);
        }
      };
    })();

    window.T7_KEEP = function (blob, name, type, tag) {
```

### U4.2 — the worker starts loading beside the program (`web/index.html`)

FIND
```
    var s = document.createElement('script');
    s.src = 'the_board.js?v=' + BUILD;
```
REPLACE
```
    window.T7_DARKROOM.open(BUILD);   // DARKROOM_1 — the worker starts loading beside the program
    var s = document.createElement('script');
    s.src = 'the_board.js?v=' + BUILD;
```


### WITNESS — U4
```
grep -c "window.T7_DARKROOM" web/index.html          # 2 — the definition and the open() call
python3 tools/gates/shell_gate/run.py                # GREEN
```
Commit: `DARKROOM_1 U4 — the shell's bridge: window.T7_DARKROOM opens the worker beside the program, copies developed bytes into the heap, and calls t7_darkroom_done`

## U5 — LEDGERS, THE REGISTER, THE PUSH
```
python3 tools/binding_ledger.py && python3 tools/command_census.py && python3 tools/mirror_census.py && python3 tools/organ_ledger.py
python3 tools/binding_ledger.py --check && python3 tools/command_census.py --check && python3 tools/mirror_census.py --check && python3 tools/organ_ledger.py --check
python3 tools/gates/score/run.py && python3 tools/gates/shell_gate/run.py && python3 tools/gates/glaw2/run.py && python3 tools/gates/sha256_gate/run.py && python3 tools/wgsl_gate.py && python3 tools/binding_gen.py --check
```
Run the provenance-stamp step. `docs/OPEN.md`, top of the register:
```
## DARKROOM_1 — THE DARKROOM WORKER (on `claude/darkroom-1`; Jean's gates open)

A painting's develop — decode, pad, swap, chain — left the frame: a Web Worker runs a
second tiny wasm built from the same core/develop.hpp the program compiles, and the
program hangs one developed painting per frame with a WriteTexture. Byte-identical by
construction and by harness (108 cases). No shared memory, no headers, no pthreads.

| ruling | where it lives now |
|---|---|
| One home for developing a painting | `src/core/develop.hpp` |
| The worker develops; the frame hangs | `src/darkroom/darkroom.cpp`, `web/darkroom_worker.js`, `window.T7_DARKROOM`, `pump_authored_valve` |
| The uploader is one function | `state.hpp` `upload_developed` |
| Failure takes the road it always took | `t7_darkroom_done` (width 0), the main-thread arm |
| The build exports what the bridge needs | `CMakeLists.txt` (HEAPU8, _malloc, _free; the darkroom target) |
| The measured row | `[METER] darkroom develop=… hang=…` |

### Residuals — DARKROOM_1
- **The hang is still ~5 MiB of WriteTexture in one frame.** If the meter's `hang=` reads
  more than a few ms on the laptop, split it: level 0 one frame, the ten levels the next
  (the layer stays `pending` until the last write). Not built until the row asks.
- **Development now starts on arrival, before first present** (the worker cannot block
  the thread); the hang still waits for first present. If `authored6` moves earlier on
  the boot card, that is this round's gift to AUBADE and worth a line there.
- **Route A stays priced** (browser decode + GPU chain) as the road if a platform ever
  refuses Workers — none does — or if the ±1 LSB ruling changes.
```
Commit: `DARKROOM_1 U5 — ledgers regenerated (state/gallery pins); OPEN.md: DARKROOM_1 open on claude/darkroom-1`
Push: `git push -u origin claude/darkroom-1`.

## JEAN'S GATES
1. **The storm, felt.** The flight over galleries that stuttered every few seconds:
   smooth. The console shows `[Authored] Developed:` lines instead of `Loaded:`; in a meter
   build, `[METER] darkroom develop=… hang=…` per painting.
2. **The wall is the wall.** Every painting exactly as before — the harness is the ruling;
   your eye is the witness that no other stage moved. The postcard of a painting: the same
   JPEG as DARKROOM_0's.
3. **The boot.** The boot card's `authored6` mark, same or earlier. `[GPU Budget]`
   unchanged — no GPU memory moved.
4. **The phones.** Pixel and iPhone: the worker opens (no `[Darkroom] worker failed` line),
   paintings hang, a flight over galleries. L48.
5. **The fallback, once.** Rename `web/darkroom_worker.js` in dist for one run: the console
   says the worker failed, paintings still hang (main-thread arm), the stutter is back —
   proving the road exists, then put the file back.

## THE HARNESS (byte identity, verbatim; g++ -std=c++20 -O2 -I. develop_harness.cpp)

The lifted originals are DARKROOM_0's `authored_stage_decoded_image` body and
`upload_authored_painting` body with `queue.WriteTexture` replaced by a capture; the
harness compares level 0 and every chain level against `develop.hpp` for each case.
```

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <random>
#include "7t/src/core/develop.hpp"
struct Ext { uint32_t w,h,d; };
struct Info { int texture; uint32_t mipLevel; struct { uint32_t x,y,z; } origin; int aspect; };
struct Layout { uint32_t offset, bytesPerRow, rowsPerImage; };
namespace wgpu { enum class TextureAspect { All }; struct Origin { uint32_t x,y,z; }; struct TexelCopyTextureInfo { int texture; uint32_t mipLevel; Origin origin; TextureAspect aspect; }; struct TexelCopyBufferLayout { uint32_t offset, bytesPerRow, rowsPerImage; }; struct Extent3D { uint32_t w,h,d; }; }
static std::vector<std::vector<uint8_t>> CAP;
static void capture(uint32_t level, const uint8_t* p, size_t n) { if (CAP.size() <= level) CAP.resize(level+1); CAP[level].assign(p, p+n); }
// ---- ORIGINAL pad (master) ----
static void orig_pad(const unsigned char* data, int width, int height, uint32_t RES, std::vector<uint8_t>& out_padded, uint32_t& out_w, uint32_t& out_h) {
    float scale = std::min((float)RES / width, (float)RES / height);
    if (scale > 1.0f) scale = 1.0f;
    uint32_t dst_w = std::min((uint32_t)(width * scale + 0.5f), RES);
    uint32_t dst_h = std::min((uint32_t)(height * scale + 0.5f), RES);

    std::vector<uint8_t> padded(RES * RES * 4, 0);
    // DARKROOM_0 — AT SCALE 1 THE BILINEAR IS A COPY, BYTE FOR BYTE. Since
    // PLATE_0 the dist caps every shipped painting at RES on its long side
    // and this loader never scales UP, so `scale` is exactly 1.0 for every
    // file that can reach here: src_xf == dx and src_yf == dy exactly, the
    // fractions are exact zeros, v == data[i00] exactly, and (uint8_t)(v +
    // 0.5f) is data[i00] again for every value 0..255. The four-tap float
    // loop below was therefore computing a memcpy at ~1M texels x 4
    // channels x 4 taps, on the main thread, once per painting, inside the
    // frame — the largest single share of the arrival stall after the
    // decode itself. The arm below it stays for the one case that can
    // still need it: a master past the cap that reached dist by hand.
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
    }   // DARKROOM_0 — the scale < 1 arm ends here

    // MIP_0 — THE PAD IS THE EDGE, REPLICATED. The square beyond dst_w x
    // dst_h was zero (transparent black). A chain averages 2x2 blocks, so
    // at every coarser level the picture's border texels would blend with
    // the pad and a dark rim would grow with distance. Replicating the last
    // column and the last row across the pad makes every level's border
    // blend with itself. The quads never sample the pad directly (uv runs 0
    // to uv_scale) and the postcard crops it away (crop_w / crop_h), so
    // nothing else moves.
    if (dst_w > 0 && dst_h > 0) {
        for (uint32_t dy = 0; dy < dst_h; ++dy) {
            const uint8_t* edge = &padded[((size_t)dy * RES + (dst_w - 1)) * 4];
            for (uint32_t dx = dst_w; dx < RES; ++dx)
                std::memcpy(&padded[((size_t)dy * RES + dx) * 4], edge, 4);
        }
        for (uint32_t dy = dst_h; dy < RES; ++dy)
            std::memcpy(&padded[(size_t)dy * RES * 4], &padded[(size_t)(dst_h - 1) * RES * 4], (size_t)RES * 4);
    }

    out_w = dst_w; out_h = dst_h; out_padded = padded;
}
// ---- ORIGINAL upload (master): swap + level0 + chain ----
static void orig_upload(bool NEED_SWAP, uint32_t LEVELS, const uint8_t* rgba_data, uint32_t width, uint32_t height) {
    const uint32_t layer = 0;
    CAP.clear();
                bool need_swap = NEED_SWAP;   // POSTCARD_0 — one home for the texel order
                std::vector<uint8_t> swapped;
                const uint8_t* src = rgba_data;

                if (need_swap) {
                    uint32_t n = width * height * 4;
                    swapped.resize(n);
                    for (uint32_t i = 0; i < width * height; ++i) {
                        swapped[i * 4 + 0] = rgba_data[i * 4 + 2]; // B
                        swapped[i * 4 + 1] = rgba_data[i * 4 + 1]; // G
                        swapped[i * 4 + 2] = rgba_data[i * 4 + 0]; // R
                        swapped[i * 4 + 3] = rgba_data[i * 4 + 3]; // A
                    }
                    src = swapped.data();
                }

                wgpu::TexelCopyTextureInfo dest{};
                
                dest.mipLevel = 0;
                dest.origin = { 0, 0, layer };
                dest.aspect = wgpu::TextureAspect::All;

                wgpu::TexelCopyBufferLayout layout{};
                layout.offset = 0;
                layout.bytesPerRow = width * 4;
                layout.rowsPerImage = height;

                wgpu::Extent3D extent = { width, height, 1 };
                capture(0, src, (size_t)width*height*4);

                // MIP_0 — THE CHAIN, built here so every authored upload has
                // one (the loader's padded square and the boot's solid fill
                // alike): each level is the 2x2 box mean of the level above,
                // in the texel order already chosen. WriteTexture's row pitch
                // has no 256-byte law (that is the buffer copies'), so the
                // small levels write as they are. The loader replicates the
                // picture's edge across the pad before calling here, so no
                // level's border blends with black (gallery.hpp).
                std::vector<uint8_t> prev(src, src + (size_t)width * height * 4);
                uint32_t w = width, h = height;
                for (uint32_t level = 1; level < LEVELS && w > 1 && h > 1; ++level) {
                    const uint32_t nw = w / 2, nh = h / 2;
                    std::vector<uint8_t> next((size_t)nw * nh * 4);
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
                    dest.mipLevel = level;
                    layout.bytesPerRow = nw * 4;
                    layout.rowsPerImage = nh;
                    wgpu::Extent3D e = { nw, nh, 1 };
                    capture(level, next.data(), (size_t)nw*nh*4);
                    prev.swap(next); w = nw; h = nh;
                }
            }


int main() {
    std::mt19937 rng(7); int fails = 0, cases = 0;
    std::vector<std::pair<int,int>> sizes = {{1024,1024},{1024,683},{683,1024},{1023,1023},{511,997},{1,1},{1,1024},{1024,1},{1024,2},{2,3},{7,1024},{2048,1536},{1025,1024},{300,200}};
    for (int t = 0; t < 40; ++t) sizes.push_back({1 + (int)(rng()%1024), 1 + (int)(rng()%1024)});
    for (auto [w,h] : sizes) for (int swap = 0; swap < 2; ++swap) {
        std::vector<unsigned char> img((size_t)w*h*4); for (auto& b : img) b = (unsigned char)rng();
        const uint32_t RES = 1024, LEVELS = 11;
        std::vector<uint8_t> p1; uint32_t dw1, dh1; orig_pad(img.data(), w, h, RES, p1, dw1, dh1);
        orig_upload(swap, LEVELS, p1.data(), RES, RES);
        auto f = t7::develop::fit(w, h, RES); std::vector<uint8_t> p2; t7::develop::pad(img.data(), w, h, RES, f, p2);
        std::vector<uint8_t> lvl0 = p2; if (swap) t7::develop::swap_rb(lvl0.data(), (size_t)RES*RES);
        std::vector<uint8_t> ch(t7::develop::chain_bytes(RES, LEVELS)); t7::develop::chain(lvl0.data(), RES, LEVELS, ch.data());
        bool ok = (dw1 == f.dst_w && dh1 == f.dst_h) && (CAP[0] == lvl0);
        for (uint32_t k = 1; k < LEVELS; ++k) { size_t off = t7::develop::chain_offset(RES, k), n = t7::develop::level_bytes(RES, k); ok = ok && CAP.size() > k && std::equal(CAP[k].begin(), CAP[k].end(), ch.begin()+off) && CAP[k].size() == n; }
        cases++; if (!ok) { fails++; printf("FAIL %dx%d swap=%d\n", w, h, swap); }
    }
    printf("cases %d  fails %d\n", cases, fails); return fails ? 1 : 0;
}
```
