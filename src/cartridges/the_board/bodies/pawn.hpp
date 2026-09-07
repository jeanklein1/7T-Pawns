#pragma once
#include <cstdint>
#include <cmath>      // std::exp (the presence ramp)
#include <algorithm>  // std::min (aura config assembly)
#include <iostream>   // command-door logs
#include "cartridges/the_board/contracts/wgpu_fwd.hpp"   // wgpu handle fwds (lockstep insurance)
#include "cartridges/the_board/contracts/driver_surface.hpp"   // ORGAN_2a — THE DRIVERS' ROOM: the presence ramp's intent/attack/release + height gain
#include "cartridges/the_board/contracts/pawn_surface.hpp"     // ORGAN_3 w2 — PAWN_AURA_LIVE: the aura profile's live surface

// ─── pawn.hpp (MERGED single file) ────────────────────────────────
// hpp+inl collapsed: struct + deps + inline impl, one pre-class file.
// The wgpu handle fwds the signatures name ride contracts/wgpu_fwd.hpp.
//
// Player-relative state: aura field (toroidal 64×64 spring grid that
// activates near the pawn and releases when it moves away), presence
// trajectory, per-frame coupling tick.
//
// The aura ramp is a self-contained real-time exponential (std::exp, below).
// ─────────────────────────────────────────────────────────────────

namespace t7 {
namespace the_board {

// ═══ TUNING CONSOLE ══════════════════════════════════════════════

// ── Aura presence ramp ───────────────────────────────────────────
// Ramp rates moved home to the drivers' room (contracts/driver_surface.hpp,
// ORGAN_2a). The authored values they carried — 1.0 attack (~3s to full,
// the spring converges in ~0.5s) and 1.5 release (~2s to zero) — are
// DRIVER_TABLE's, and the tick reads DRIVER_LIVE.aura.

// The aura vocabulary — PawnAuraDeltaMode, PawnAuraProfile,
// PAWN_AURA_DEFAULT — graduated to contracts/pawn_surface.hpp
// (ORGAN_3 w2), where PAWN_AURA_LIVE stands beside the design table as
// the live surface. The readers below read the bank.

// ═══ PAWN MODULE STATE ═══════════════════════════════════════════
//
// Field roles:
//   (the aura profile — lives at PAWN_AURA_LIVE,
//    contracts/pawn_surface.hpp. PawnState's own copy retired at
//    ORGAN_3 w2: the census found no writer for it anywhere, so it was
//    a copy of a constant nothing changed. A landmark that swaps
//    profiles assigns the bank.)
//   (aura intent — lives at DRIVER_LIVE.aura.intent, contracts/driver_surface.hpp;
//    written by Shift's door (SHIFT_0; was key 3), the mood policy door, and the panel. The presence
//    ramp smooths the transition toward it, at the room's own attack/release.)
//   aura_height_enabled — Height-effect gate (key 2, direction/input.hpp);
//     flattens the extrusion. It USED to leave the color tint visible; since
//     TUNE_1 A4 muted tint_strength there is no tint left to leave, so the
//     key is now a height-only toggle.
//   aura_needs_clear — Internal: clear cells next frame after aura disable.
//   aura_cfg_dirty — Internal: full-config upload flag; true at boot; set by
//     external writers on a parameter shift.
//   (aura_presence — lives at player_.aura_presence, SEAM[spine:P8];
//    read as c->player_.aura_presence; written in tick_pawn_couplings.)

struct PawnState {
    bool            aura_height_enabled = true;
    bool            aura_needs_clear    = false;
    bool            aura_cfg_dirty      = true;
};

// ═══ MODULE DEPS ════════════════════════════════════════════════════
// The pawn's requirements face. player_ is NON-const — the P8 door
// (aura_presence is pawn-written, SEAM[spine:P8]); census W allows it
// here. (GPUState/Renderer fwd — state.hpp/renderer.hpp follow this
// header in the cohort; reference members tolerate incomplete types.)
struct PlayerState;
struct TimeState;
class GPUState;
class Renderer;
struct PawnDeps {
    PlayerState&     player_;       // non-const: P8 aura_presence write
    const TimeState& time_state_;
    GPUState&        gpuState_;
    Renderer&        renderer_;
};

// ─── IMPL:
// bodies deref only PawnDeps members; no Cartridge. The merged file
// sits after renderer.hpp in the cohort (Renderer/GPUState complete;
// Dim::PATCH_CELL_SIZE from patch_system.hpp). ───────────────────────────

// ─── Per-frame pawn coupling tick ────────────────────────────────
inline void tick_pawn_couplings(PawnState& ps, PawnDeps* c, wgpu::Queue& queue) {
    (void)queue;
    // Aura presence ramp: smooth 0→1 on enable / 1→0 on disable.
    // (aura_presence lives on player_ — SEAM[spine:P8]; see PlayerState, contracts/spine_state.hpp)
    {
        const auto& drv    = DRIVER_LIVE.aura;   // ORGAN_2a — the ramp's dials
        const float target = drv.intent ? 1.0f : 0.0f;
        const float prev   = c->player_.aura_presence;
        const float rate   = (target > prev) ? drv.attack : drv.release;
        c->player_.aura_presence = prev + (target - prev) * (1.0f - std::exp(-rate * c->time_state_.dt));

        // Snap to endpoints to avoid perpetual drift
        if (c->player_.aura_presence < 0.001f && target == 0.0f) c->player_.aura_presence = 0.0f;
        if (c->player_.aura_presence > 0.999f && target == 1.0f) c->player_.aura_presence = 1.0f;
        if (c->player_.aura_presence != prev) ps.aura_cfg_dirty = true;
    }

    // Pawn aura height: presence × base height. Same value is consumed
    // by terrain VS for extrusion, so pawn and terrain always agree.
    const float effective_aura_height = ps.aura_height_enabled
        ? PAWN_AURA_LIVE.height_scale * DRIVER_LIVE.aura.height_gain
              * c->player_.aura_presence
        : 0.0f;
    c->gpuState_.set_pawn_aura_height(effective_aura_height);
    // Keep compute running while ramping down (so the trail decays cleanly).
    c->gpuState_.set_aura_enabled(c->player_.aura_presence > 0.001f);
}


// ─── Teardown (owner verb) ────────────────────────────────────────
// The pawn half of the world-teardown sweep: schedule a one-frame
// aura clear + full config re-upload for the new world.
inline void teardown_pawn_aura(PawnState& ps) {
    ps.aura_needs_clear = true;
    ps.aura_cfg_dirty = true;
}

// ─── Aura compute (owner verb) ──────────────────────────────────────
// Persistent terrain influence. Runs while presence > 0 (ramping down
// after toggle-off) or clearing; no-op otherwise. DEFERRED-UPLOAD FLAG
// aura_cfg_dirty (O-4): full config upload on change, dt/t_beats-only
// in steady state.
inline void dispatch_pawn_aura(PawnState& ps, PawnDeps* c,
                               wgpu::CommandEncoder& encoder, wgpu::Queue& queue) {
    if (!(c->player_.aura_presence > 0.0f || ps.aura_needs_clear)) return;
        if (ps.aura_cfg_dirty) {
            // Full config upload — profile changed or first frame
            ps.aura_cfg_dirty = false;
            const auto& ap = PAWN_AURA_LIVE;

            // Presence scales all aura params for smooth raise/lower
            float p = c->player_.aura_presence;

            GPUPawnAuraConfig auraCfg{};
            auraCfg.cell_size = Dim::PATCH_CELL_SIZE;
            auraCfg.influence_radius = ap.influence_radius * p;
            auraCfg.attack_stiffness = ap.attack_stiffness;
            auraCfg.attack_damping = ap.attack_damping;
            auraCfg.release_rate = (p > 0.01f) ? ap.release_rate : 999.0f;
            auraCfg.dt = c->time_state_.dt;
            auraCfg.effect_mask = ap.effect_mask;
            auraCfg.aura_n = 64;
            auraCfg.tint_strength = std::min(ap.tint_strength * p, 1.0f);
            auraCfg.tint_r = ap.tint_r;
            auraCfg.tint_g = ap.tint_g;
            auraCfg.tint_b = ap.tint_b;
            auraCfg.delta_mode = ap.delta_mode;
            auraCfg.delta_magnitude = ap.delta_magnitude;
            auraCfg.t_beats = c->time_state_.beats;
            // height_scale gates the compute shader's R channel write (> 0.01 = enabled).
            // Actual terrain extrusion magnitude comes from config.pawn_aura_height in the VS.
            auraCfg.height_scale = (ps.aura_height_enabled && p > 0.01f) ? ap.height_scale : 0.0f;
            c->gpuState_.upload_pawn_aura_config(queue, auraCfg);
        }
        else {
            // Steady state — only dt and t_beats change per frame
            c->gpuState_.upload_pawn_aura_frame(queue, c->time_state_.dt, c->time_state_.beats);
        }

        wgpu::ComputePassDescriptor cpd{};
        cpd.label = "Pawn Aura";
        cpd.timestampWrites = c->gpuState_.meter_arm_compute(meter_row::PawnAura);
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&cpd);
        // LOOM_2 pass head: WORLD + FRAME are every pipeline's strata 0/1.
            { pass.SetBindGroup(0, c->gpuState_.world_group());
          pass.SetBindGroup(1, c->gpuState_.frame_c_group()); }
        c->renderer_.dispatch_compute_pawn_aura(pass,
            c->gpuState_.aura_state_group(), c->gpuState_.aura_textures_group(),
            GPUState::pawn_aura_workgroups());
        pass.End();

        // After one cleanup frame with release_rate=999, all cells are zero
        if (ps.aura_needs_clear) { ps.aura_needs_clear = false; }
}


// ─── Player commands (owner verbs — the input fan's
// pawn pair, matching the orbs/agents/cube command pattern) ────────
inline void toggle_aura_height(PawnState& ps, PawnDeps* c) {
    (void)c;
    ps.aura_height_enabled = !ps.aura_height_enabled;
    ps.aura_cfg_dirty = true;
    std::cout << "[Aura] Height extrusion: " << (ps.aura_height_enabled ? "ON" : "OFF") << "\n";
}

inline void toggle_aura(PawnState& ps, PawnDeps* c) {
    (void)c;
    DRIVER_LIVE.aura.intent = DRIVER_LIVE.aura.intent ? 0u : 1u;   // ORGAN_2a — the intent's home
    ps.aura_cfg_dirty = true;
    std::cout << "[Aura] Field: " << (DRIVER_LIVE.aura.intent ? "ON" : "OFF") << "\n";
}

// ─── Mood policy door: respect player preference when
// permitted, force off when forbidden — the mood driver speaks through
// the pawn's own door instead of writing the organ. Semantics
// byte-identical to the direct write it replaces (disclosure rule).
inline void apply_aura_mood_policy(PawnState& ps, bool allow) {
    (void)ps;
    if (!allow) DRIVER_LIVE.aura.intent = 0u;
}

} // namespace the_board
} // namespace t7
