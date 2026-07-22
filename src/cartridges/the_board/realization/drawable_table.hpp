#pragma once
// ═══════════════════════════════════════════════════════════════════════
// THE DRAWABLE TABLE — one row per drawable; the
// three render passes (shadow / main / snapshot) iterate it FILTERED by
// membership. Kills the triplication (a drawable was enumerated once per
// list, ~3 hand-synced sites) and the ribbon's ordinal DRIFT (the lists had
// ribbon at different positions) — there is now ONE canonical order.
// A new drawable is ONE row instead of a 3-site edit.
//
// PIXEL-SAFETY. Every drawable IN THIS TABLE is OPAQUE (depth-tested,
// depth-write, no blend — or an alpha=1.0 output that makes SrcAlpha a
// no-op): terrain(fork), zone, pawn, sphere, monolith, ribbon, arch,
// column, palm, cactus, blade, shell, and the gallery/wall FORKS. Draw
// order among OPAQUE geometry is immaterial — the depth test resolves
// visibility identically regardless of order — so the ONE canonical order
// (the shadow order) reproduces every pass pixel-for-pixel, and the ribbon
// drift dies for free. Verified opaque: ribbon uses ENTITY_FS + the shared
// depthStencil/colorTarget (no blend); gallery_frame_fs / wall_painting_*_fs
// return alpha=1.0 with depth-write. The ORDER-SENSITIVE draws are FORKS,
// kept last and in order (orbs additive, fade alpha/no-depth).
//
// FORKS (Discipline 2 — flagged, NOT forced into a uniform row):
//   - terrain: three different per-pass codes (shadow: LOD0 + manual LOD1;
//     main: LOD0 indirect/direct + LOD1 direct; snapshot: one direct draw).
//   - wall_paintings / gallery_frames: their OWN gallery bind groups.
//   - orbs / fade: blended, ORDER-SENSITIVE, kept LAST in the main pass.
// These stay as explicit code in their pass function; only the uniform
// opaque entity draws live in the table.
// ═══════════════════════════════════════════════════════════════════════

#include "cartridges/the_board/realization/state.hpp"      // GPUState + wgpu
#include "cartridges/the_board/realization/renderer.hpp"   // Renderer (the draw_* verbs)

namespace t7 {
namespace the_board {

enum DrawPass : uint32_t { DRAW_SHADOW = 1u, DRAW_MAIN = 2u, DRAW_SNAPSHOT = 4u };

// Per-pass binding context: the entity group differs by pass (render vs
// photographer), the texture group by pass (shadow vs render), and the two
// runtime data-guards ride here (precomputed by the pass) so the thunks stay
// ctx-agnostic — MachineCtx (shadow/main) and GalleryDeps (snapshot) both
// expose Renderer&/GPUState&, nothing more is needed.
struct DrawBind {
    wgpu::BindGroup entity;     // render_entity_group | photographer_render_entity_group
    wgpu::BindGroup texture;    // shadow_texture_group | render_texture_group
    bool shadow;                // shadow pass -> draw_shadow_X ; else draw_X
    bool ribbon_active;         // ribbon_state_.rendered_slot != UINT32_MAX
};

struct Drawable {
    const char* name;
    uint32_t    passes;         // bit-OR of DrawPass
    void (*draw)(Renderer&, GPUState&, wgpu::RenderPassEncoder&, const DrawBind&);
};

// ── The thunks: each knows its buffers and picks draw_X vs draw_shadow_X ──
inline void dt_pawn(Renderer& r, GPUState& g, wgpu::RenderPassEncoder& p, const DrawBind& b) {
    (void)g;
    if (b.shadow) r.draw_shadow_pawn(p, b.entity, b.texture, GPUState::pawn_vertex_count());
    else          r.draw_pawn       (p, b.entity, b.texture, GPUState::pawn_vertex_count());
}
inline void dt_sphere(Renderer& r, GPUState& g, wgpu::RenderPassEncoder& p, const DrawBind& b) {
    if (b.shadow) r.draw_shadow_sphere(p, b.entity, b.texture, g.sphere_vertex_buffer(), g.sphere_index_buffer(), g.sphere_index_count());
    else          r.draw_sphere       (p, b.entity, b.texture, g.sphere_vertex_buffer(), g.sphere_index_buffer(), g.sphere_index_count());
}
inline void dt_monolith(Renderer& r, GPUState& g, wgpu::RenderPassEncoder& p, const DrawBind& b) {
    if (b.shadow) r.draw_shadow_monolith(p, b.entity, b.texture, g.monolith_vertex_buffer(), g.monolith_index_buffer(), g.monolith_index_count());
    else          r.draw_monolith       (p, b.entity, b.texture, g.monolith_vertex_buffer(), g.monolith_index_buffer(), g.monolith_index_count());
}
inline void dt_ribbon(Renderer& r, GPUState& g, wgpu::RenderPassEncoder& p, const DrawBind& b) {
    (void)g;
    if (!b.ribbon_active) return;
    if (b.shadow) r.draw_shadow_ribbon(p, b.entity, b.texture, GPUState::ribbon_vertex_count());
    else          r.draw_ribbon       (p, b.entity, b.texture, GPUState::ribbon_vertex_count());
}
inline void dt_arch(Renderer& r, GPUState& g, wgpu::RenderPassEncoder& p, const DrawBind& b) {
    if (b.shadow) r.draw_shadow_arch(p, b.entity, b.texture, g.arch_vertex_buffer(), g.arch_index_buffer(), g.arch_index_count());
    else          r.draw_arch       (p, b.entity, b.texture, g.arch_vertex_buffer(), g.arch_index_buffer(), g.arch_index_count());
}
inline void dt_column(Renderer& r, GPUState& g, wgpu::RenderPassEncoder& p, const DrawBind& b) {
    if (b.shadow) r.draw_shadow_column(p, b.entity, b.texture, g.column_vertex_buffer(), g.column_index_buffer(), g.column_index_count());
    else          r.draw_column       (p, b.entity, b.texture, g.column_vertex_buffer(), g.column_index_buffer(), g.column_index_count());
}
inline void dt_palm(Renderer& r, GPUState& g, wgpu::RenderPassEncoder& p, const DrawBind& b) {
    if (b.shadow) r.draw_shadow_palm(p, b.entity, b.texture, g.palm_vertex_buffer(), g.palm_index_buffer(), g.palm_index_count());
    else          r.draw_palm       (p, b.entity, b.texture, g.palm_vertex_buffer(), g.palm_index_buffer(), g.palm_index_count());
}
inline void dt_cactus(Renderer& r, GPUState& g, wgpu::RenderPassEncoder& p, const DrawBind& b) {
    if (b.shadow) r.draw_shadow_cactus(p, b.entity, b.texture, g.cactus_vertex_buffer(), g.cactus_index_buffer(), g.cactus_index_count());
    else          r.draw_cactus       (p, b.entity, b.texture, g.cactus_vertex_buffer(), g.cactus_index_buffer(), g.cactus_index_count());
}
inline void dt_blade(Renderer& r, GPUState& g, wgpu::RenderPassEncoder& p, const DrawBind& b) {
    if (b.shadow) r.draw_shadow_blade(p, b.entity, b.texture, g.blade_vertex_buffer(), g.blade_index_buffer(), g.blade_index_count());
    else          r.draw_blade       (p, b.entity, b.texture, g.blade_vertex_buffer(), g.blade_index_buffer(), g.blade_index_count());
}
inline void dt_shell(Renderer& r, GPUState& g, wgpu::RenderPassEncoder& p, const DrawBind& b) {
    if (b.shadow) r.draw_shadow_shell(p, b.entity, b.texture, g.shell_vertex_buffer(), g.shell_index_buffer(), g.shell_index_count());
    else          r.draw_shell       (p, b.entity, b.texture, g.shell_vertex_buffer(), g.shell_index_buffer(), g.shell_index_count());
}

// THE CANONICAL ORDER (== the shadow order). Membership is which passes a
// drawable belongs to; snapshot is the photographer's subset.
inline const Drawable DRAWABLES[] = {
    { "pawn",     DRAW_SHADOW | DRAW_MAIN | DRAW_SNAPSHOT, dt_pawn    },
    { "sphere",   DRAW_SHADOW | DRAW_MAIN | DRAW_SNAPSHOT, dt_sphere  },
    { "monolith", DRAW_SHADOW | DRAW_MAIN,                dt_monolith },
    { "ribbon",   DRAW_SHADOW | DRAW_MAIN | DRAW_SNAPSHOT, dt_ribbon  },
    { "arch",     DRAW_SHADOW | DRAW_MAIN | DRAW_SNAPSHOT, dt_arch    },
    { "column",   DRAW_SHADOW | DRAW_MAIN | DRAW_SNAPSHOT, dt_column  },
    { "palm",     DRAW_SHADOW | DRAW_MAIN,                dt_palm     },
    { "cactus",   DRAW_SHADOW | DRAW_MAIN,                dt_cactus   },
    { "blade",    DRAW_SHADOW | DRAW_MAIN,                dt_blade    },
    { "shell",    DRAW_SHADOW | DRAW_MAIN | DRAW_SNAPSHOT, dt_shell   },
};

// Iterate the table for one pass, in canonical order, filtered by membership.
inline void draw_table(Renderer& r, GPUState& g, wgpu::RenderPassEncoder& p,
                       const DrawBind& b, uint32_t passbit) {
    for (const Drawable& d : DRAWABLES)
        if (d.passes & passbit) d.draw(r, g, p, b);
}

} // namespace the_board
} // namespace t7
