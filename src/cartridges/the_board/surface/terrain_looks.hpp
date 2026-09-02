#pragma once

#include <cstdint>

// ═══════════════════════════════════════════════════════════════════
// THE TERRAIN_LOOKS PANEL — C++ room (surface/terrain_looks.hpp)
// ═══════════════════════════════════════════════════════════════════
//
// The D2 surface-voice authoring surface: geometry, color, movement —
// what the terrain LOOKS like. Every rest value and authored constant
// of the surface voice, consolidated where a designer (or a coupling
// wire) can read it in one sitting. Third foundational panel, after
// CameraControls and population_themes (the population
// panel). THE MAPPING CONVERSATION convenes OVER this file:
// wires get written as rows beside the parameters they drive.
//
// TWO ROOMS, ONE PANEL — the mirror rule (the binding-registry
// ceiling, said honestly): the WGSL room is the §2.2 TERRAIN_LOOKS
// region in realization/world.wgsl. Same rows, same order. No machine
// gate crosses the language gap — glaw1 is WGSL-blind and WGSL cannot
// include this header — so the discipline is: every VALUE lives in
// exactly ONE room, and the other room carries the row as a named
// pointer only (no cross-language number exists to drift). The only
// shared text is the row index below. Nets: the boot rig (the world
// must look identical) for values; Dawn binding validation for
// uniform layout; the C6 registry for binding numbers.
//
// ROW INDEX (identical in both rooms):
//   ROW 1 — THE PALETTE QUARTET        values HERE (REST) → config uniform
//   ROW 2 — MOTION & MODE REST PINS    values HERE → setters at cartridge boot
//   ROW 3 — PALETTE COMPOSITION        values in the WGSL room
//   ROW 4 — FIELD LATTICES             values in the WGSL room
//   ROW 5 — COMPOSITE CUTS & EDGES     values in the WGSL room
//   ROW 7 — THE MOVEMENT THIRD         values in the WGSL room
//   ROW 8 — GOVERNING EXPRESSIONS      text in the WGSL room
//   ROW 9 — THE CONTRIBUTOR ROSTER     pointers, both rooms
//
// STAYS OUT (machinery / other jurisdictions): streaming, the tile
// cache, manifold dispatch, bake kernel bodies, spawn/population
// (population_themes' panel), the veil (visibility's jurisdiction),
// GoL/pulse internals (their own panel — ROW 9 points at the tint
// funnel they share with the surface).

namespace t7 {
namespace the_board {
namespace terrain_looks {

// ── ROW 1 — THE PALETTE QUARTET (REST) ──────────────────────────────
// WHAT: the four terrain palettes' boot values — the pre-graduation
//   WGSL literals, bit-identical by construction. LIVE
//   values ride the config uniform (config.palette_center / _light /
//   _weight, GPUDesignConfig), authored per-frame by set_palette_*;
//   these arrays are what boot writes there (GPUState::initializeState).
// AXES: outer = palette id — 0 sand / 1 salmon / 2 green (rare) /
//   3 warm — shared with the dominant-weight branch in
//   palette_weights_at_node (WGSL ROW 3); inner = r,g,b.
// UNITS: center/light = rgb 0-1; weight = selection probability,
//   component i = palette i (sums 1.0).
// REST ≡ these values (the panel IS the rest column).
// frozen biography (§12) — changing a number changes worlds.
inline constexpr float PALETTE_CENTER_REST[4][3] = {
    { 0.85f, 0.70f, 0.50f },   // 0: sand
    { 0.88f, 0.58f, 0.48f },   // 1: salmon
    { 0.45f, 0.58f, 0.38f },   // 2: green (rare)
    { 0.82f, 0.55f, 0.42f },   // 3: warm
};
inline constexpr float PALETTE_LIGHT_REST[4][3] = {
    { 0.92f, 0.82f, 0.65f },   // 0: sand
    { 0.95f, 0.72f, 0.62f },   // 1: salmon
    { 0.62f, 0.72f, 0.52f },   // 2: green (rare)
    { 0.92f, 0.72f, 0.58f },   // 3: warm
};
inline constexpr float PALETTE_WEIGHT_REST[4] = {
    0.42f,   // 0: sand
    0.28f,   // 1: salmon
    0.04f,   // 2: green — rare
    0.26f,   // 3: warm
};

// ── ROW 2 — MOTION & MODE REST PINS ─────────────────────────────────
// WHAT: the surface voice's silence. The cartridge boot-pin block
//   (Cartridge::initialize) writes exactly these through the setters;
//   nothing else authors them today (the mode trio is DRIVERLESS since
//   gen-1 retirement — driver-ready, held at rest by this row).
// UNITS: terrain_time = beats (≤ 0 freezes the BAND SUM in the card's
//   heights pass — ONE of three rest conjuncts, see REST_PULSE_COUNT
//   below and the full statement at world.wgsl write_live_card_heights);
//   band blend = activation
//   [0,1] per band, -1 = inactive sentinel; band phase origin = beats;
//   mode_color_shift = mode-field bias [−1,1]; mode_checker_scatter =
//   sparse-threshold bias; palette drift = (target palette idx [0,3],
//   intensity [0,1], discrete tier id).
// CONSUMER: cartridge.hpp boot-pin → set_band_motion / set_terrain_time
//   / set_mode_color_shift / set_mode_checker_scatter /
//   set_mode_palette_drift / set_checker_color_field → config uniform
//   → WGSL rows 3/5/7 readers.
inline constexpr float REST_TERRAIN_TIME = 0.0f;                    // frozen clock
inline constexpr float REST_BAND_BLEND[6] =                        // all bands inactive
    { -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f };
inline constexpr float REST_BAND_PHASE_ORIGIN[6] = {};             // zero origins
inline constexpr float REST_MODE_COLOR_SHIFT = 0.0f;               // no mode bias
inline constexpr float REST_MODE_CHECKER_SCATTER = 0.0f;           // no sparse bias
inline constexpr float REST_MODE_PALETTE_DRIFT_TARGET = 0.0f;      // (unread while
inline constexpr float REST_MODE_PALETTE_DRIFT_INTENSITY = 0.0f;   //  intensity 0)
inline constexpr float REST_MODE_PALETTE_DRIFT_TIER = 0.0f;
// CHECKER-REBUILD (THE PITCH-CLASS COLOR FIELD): the checker vocabulary's
// live response — the voice's WINDOW pc-LENGTH vector (Playhead + Wagon
// compound) over Jean's authored PC_COLOR table (absolute pitch class →
// RGB) gives a length-weighted RESULTANT color; presence + distinct-pc
// count ride alongside, enveloped (2-beat attack, 8-beat release). The GPU
// pulls each discrete cell toward the resultant, wanders each region
// around it by a STATIC per-region offset, and widens each region's spread
// by the count; the mode field's own gating decides which cells (between
// smooth sections) show it (world.wgsl discrete_cell_color / _at_tier;
// dials §2.2 ROW 5: CHECKER_WANDER / CHECKER_VAR_PER_NOTE / CHECKER_VAR_MAX
// / DEBUG_VIEW). RESTS are law: amount 0 (the GPU
// maps that to each cell's seed color) and variance 0 — a return to seed,
// not gray. WIRE: <voice>.window_length → PC_COLOR (coupling/visual_
// canvas.hpp, the tunable home) → terrain.checker_* bank pipes →
// set_checker_color_field (U4) → config → WGSL. Read every 4 beats.
inline constexpr float REST_CHECKER_RESULTANT[3] = { 0.0f, 0.0f, 0.0f };
inline constexpr float REST_CHECKER_AMOUNT = 0.0f;
inline constexpr float REST_CHECKER_VARIANCE = 0.0f;
// Pulse ring rest: count 0 with a zeroed ring IS the rest (the boot
// pin sources it from here).
inline constexpr std::uint32_t REST_PULSE_COUNT = 0;

// ── THE PULSE BUS's CPU ROOM (PULSE_1) ──────────────────────────────
// WHAT: the ring's shape, its liveness test, and the tap's impulse, on
//   the CPU side of the seam. The RING ITSELF lives on the spine
//   (Cartridge::pulseRing_) because stamping an onset needs the clock
//   and the point, and both are the spine's; what lives here are the
//   numbers, beside the rest pin they retire to.
// MIRROR: the first four are CPU faces of world.wgsl's
//   `pulse_data: array<vec4<f32>, 8>` and of contrib_radial_pulses_at's
//   own early-exit — `age < 0.0 || age > PULSE_MAX_AGE || p.w < 0.001`.
//   THE TWO ROOMS MUST AGREE OR THE COUNT OUTLIVES THE RINGS IT COUNTS:
//   the CPU retires the ring to REST_PULSE_COUNT using exactly the test
//   the shader draws by, and pulse_count is a conjunct of the live-card
//   rest law (Cartridge::live_card_is_live), so a CPU that thought a
//   pulse still alive would hold the card's writer awake over a world
//   drawing nothing.
// CONSUMER: Cartridge::emit_radial_pulse / retire_aged_pulses.
inline constexpr std::uint32_t PULSE_RING_SLOTS   = 8;    // WGSL: array<vec4<f32>, 8>
inline constexpr std::uint32_t PULSE_RING_FLOATS  = PULSE_RING_SLOTS * 4;
inline constexpr float         PULSE_MAX_AGE      = 8.0f; // WGSL: const PULSE_MAX_AGE
inline constexpr float         PULSE_MIN_AMPLITUDE = 0.001f;  // WGSL: the p.w early-exit
// THE TAP'S IMPULSE IS NOT HERE: it is a DIAL, and the ORGAN may not
// include this file, so it lives in contracts/control_panel.hpp as
// PANEL_LIVE.pulse.amplitude (the possession-reach precedent). The four
// numbers above are shader MIRRORS, not dials, which is why they stay.

// ── ROWS 3-8 — see the WGSL room (world.wgsl §2.2 TERRAIN_LOOKS) ────
//   ROW 3 PALETTE COMPOSITION: PALETTE_DOMINANT_WEIGHT / _MINOR_WEIGHT
//     (the dominant-branch matrix) + PALETTE_COMPLEXITY (the pinned
//     center/light mix dial).
//   ROW 4 FIELD LATTICES: the *_LATTICE_SPACING family.
//   ROW 5 COMPOSITE CUTS & EDGES: mode blend/scatter edges, sparse
//     survival, chess/mono cuts, DISCRETE_TINT_STRENGTH.
//   ROW 7 THE MOVEMENT THIRD: the TRUE bands (TERRAIN_BANDS) +
//     WAVE_THRESHOLD pool gates +
//     activity motion-rate vocabulary (REST pins live in ROW 2 here).
//   ROW 8 GOVERNING EXPRESSIONS: palette_color_smooth (in-room);
//     composite_cell_color (by pointer — Discipline 2).

// ── ROW 9 — THE CONTRIBUTOR ROSTER (pointers; navigable, NOT annexed)─
//   Static landform: CONTRIBUTOR_DAG / POLICIES[] —
//     contracts/ground_architecture.hpp (+ WGSL §3.4 mirror seam).
//   GoL zone tint: gol_composite_cell_color + zone color sites —
//     world.wgsl (GoL keeps its own panel).
//   Pawn aura tint: pawn_aura_cfg.tint_strength (a uniform field, not
//     this panel's pinned DISCRETE_TINT_STRENGTH).
//   Population (what stands on the surface): population_themes.hpp.

}  // namespace terrain_looks
}  // namespace the_board
}  // namespace t7
