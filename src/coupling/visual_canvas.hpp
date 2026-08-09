#pragma once

// ─── coupling/visual_canvas.hpp ──────────────────────────────────────────────
//
// The visual canvas — the coupling layer's binding surface, dual of the
// musical Canvas. It consumes the analysis contract and writes the visual
// parameter bank; it touches no GPU. Where the musical Canvas reads MIDI,
// composes readings, and publishes AnalysisSignal, this consumes that signal,
// runs each coupling, and drives VisualParams. Entities read the bank and
// upload what moved — they never learn what drove them.
//
// THREE REGIONS
//   couplings            — the decode tuning each coupling reads (tables,
//       spans). The decode itself is inline simple-math in tick(), not a goal
//       object: AffineGoal serves the affine cases, custom decodes stay inline.
//   master control panel — PARAM_LAYOUT: every exposed pipe, its slot, width,
//       and rest. Slots are laid by hand in this one table, so no collisions.
//   the canvas           — bind() resolves every source and target by name
//       once; tick() runs the couplings each frame. Resolve once, never per
//       frame.
//
// FIRST COUPLING — fog. The held field (a one-based rank, 0 = none) selects an
// absolute fog density from FOG_BY_FIELD and an atmospheric tint from
// FOG_COLOR_BY_FIELD; Segments carry both, so density and color drift across a
// modulation instead of snapping. The source, "all.field", is already
// published, so the analysis side is untouched.
//
// WIRING (live). The cartridge owns a VisualCanvas, binds it once in
// bind_signal_layout with the analysis layout, ticks it each frame in
// update() after the signal, and flushes fog — density and color —
// from params() to set_fog. Fog has one driver: the field.
//
// CHECKER-REBUILD — THE PITCH-CLASS COLOR FIELD (the terrain's checker
// voice). The voice's WINDOW pc-LENGTH vector — pc_length(playhead,
// wagon(0)): the Playhead + Wagon compound, duration-weighted, dressed
// to D — is read every CHECKER_READ_SPAN beats. NO DFT, no interval
// math: each ABSOLUTE pitch class (index 0 = D) has an authored RGB
// (PC_COLOR), and the decode is the length-weighted average color,
// resultant = (Σ length_i · PC_COLOR[i]) / Σ length_i. Two enveloped
// scalars ride with it: music_amount (presence [0,1]) and music_variance
// (distinct-pc count). The ENVELOPE is Jean's: LINEAR 2-beat attack to
// the new 4-beat target, LINEAR 8-beat release to rest — a return to
// SEED (music_amount → 0, which the GPU maps to the cell's own seed
// color), not to gray. The cartridge flushes terrain.checker_* through
// set_checker_color_field in U4; the GPU (discrete_cell_color) pulls
// each discrete cell toward the resultant, wanders each region around
// it (re-rolled per window), and widens each region's own spread by the
// count — applied DIRECTLY to the checker cells. Console witness:
// [CHECKER] per read.
//
// USAGE
//   visual_canvas_.bind(analysis_layout);          // startup
//   visual_canvas_.tick(signal);                   // per frame, after analysis
//   ...
//   auto d = visual_canvas_.layout().resolve("fog.density");   // entity, once
//   auto k = visual_canvas_.layout().resolve("fog.color");     // base..base+2
//   float density = visual_canvas_.params().get(d.base);       // entity flush
//
// Depends on: coupling/visual_params.hpp, coupling/trajectory.hpp,
//             musical/signal_layout.hpp, analysis/analysis_signal.hpp,
//             <string>, <cmath>, <algorithm>.

#include "coupling/visual_params.hpp"
#include "coupling/trajectory.hpp"
#include "core/instruments.hpp"   // THE INSTRUMENTS DIAL: INSTRUMENTS.checker_witness gates the [CHECKER] line
#include "musical/signal_layout.hpp"
#include "analysis/analysis_signal.hpp"
#include <string>    // casting-sheet name composition ("<voice>.present_count")
#include <array>     // the hue unit-vector table (OIL_1 U5)
#include <cstddef>   // size_t — the table's index casts (OIL_1 U5)
#include <cmath>     // std::floor / cos / sin / sqrt / atan2 — decode math
#include <algorithm> // std::min/std::max — decode clamps
#include <cstdio>    // std::fprintf — the [CHECKER] witness line

namespace t7 {

    // ═══ COUPLINGS ═══════════════════════════════════════════════════════════════
    // Each coupling's tuning sits with the coupling; the decode runs inline in
    // tick(). Whether a coupling has an idle depends on its source: a source that
    // can fall quiet (a count, a magnitude) has a rest the value returns to; a
    // held source never quiets, so its coupling goes value-to-value, no idle.

    // Fog — the held field selects an absolute density. The field is a held source:
    // once a scale is established it persists through silence, so fog never returns
    // to a rest — it moves from one field's density to the next. FIELD 1 IS THE
    // ANCHOR: the atmosphere the open outdoor world wears, which is also the value
    // index 0 carries. Music is continuous, so that atmosphere is not a rest the
    // world visits between fields — it is field 1's own look, and every other field
    // is a deviation from it. Fields 5/6 sit in the dense band, 2/3/4 in the light.
    // Index 0 is "no field yet" — the value at boot, before any scale is held, not
    // an idle. Tunable.
    inline constexpr int   FOG_FIELD_COUNT = 7;          // index 0 = none, 1..6 fields
    // THE ANCHOR — one home for both rows that wear it. Twinned by the boot
    // config in realization/state.hpp (config_.fog_density / fog_color).
    inline constexpr float FOG_DENSITY_NONE  = 0.0030f;
    inline constexpr float FOG_COLOR_NONE[3] = { 0.85f, 0.78f, 0.72f };
    inline constexpr float FOG_BY_FIELD[FOG_FIELD_COUNT] = {
        FOG_DENSITY_NONE,   // 0  none   — no field yet (boot)
        FOG_DENSITY_NONE,   // 1  anchor — the open outdoor atmosphere
        0.0022f,            // 2  light
        0.0026f,            // 3  light
        0.0020f,            // 4  light
        0.0050f,            // 5  dense
        0.0058f,            // 6  dense
    };

    // Fog color — the same held field selects an atmospheric tint, carried per
    // channel so the hue drifts with the density. Tiers 0 and 1 both wear the
    // anchor; the rest are shifts away from it. Same held source, so the same
    // value-to-value behavior — no idle. Tunable.
    //                                                  R       G       B
    inline constexpr float FOG_COLOR_BY_FIELD[FOG_FIELD_COUNT][3] = {
        { FOG_COLOR_NONE[0], FOG_COLOR_NONE[1], FOG_COLOR_NONE[2] },   // 0  none   — no field yet
        { FOG_COLOR_NONE[0], FOG_COLOR_NONE[1], FOG_COLOR_NONE[2] },   // 1  anchor — the open outdoor atmosphere
        { 0.78f, 0.80f, 0.82f },   // 2  cool pale
        { 0.80f, 0.82f, 0.76f },   // 3  faint sage
        { 0.74f, 0.78f, 0.86f },   // 4  soft blue
        { 0.92f, 0.72f, 0.55f },   // 5  warm amber
        { 0.70f, 0.68f, 0.80f },   // 6  muted violet
    };

    // One span carries both fog pipes across a field change; split into a second
    // constant if color should lead or lag density.
    inline constexpr float FOG_SPAN = 2.0f;   // beats — glide into the new field

    // ── Casting (the avatar principle) ── one voice per entity; the set
    // of these is the CASTING SHEET. The ribbon is the chordal piano.
    inline constexpr const char* RIBBON_VOICE = "ch1";   // live prefix verified: chN (canvas_1 NAME_* tables)

    // ── The zoetrope's ears ── a listener SET, not a voice: bit N =
    // wire chN listens. DIAGNOSTIC WIDE: the screen hears the whole
    // composition. Narrow to a set once the pipe is proven — {ch6}
    // (0b0100'0000u) was the ruling; wire = Ableton − 1.
    inline constexpr uint32_t ZOETROPE_EARS = 0b0111'1111u;

    // ── The mode fold ── pc → screen row, bottom = tonic. Seven rows =
    // E Phrygian dominant {E F G# A B C D} = pc {4 5 8 9 11 0 2}.
    // Out-of-mode pcs borrow the nearest degree's row, ties downward.
    inline constexpr uint8_t ZOETROPE_ROW_OF_PC[12] =
        { 5, 5, 6, 6, 0, 1, 1, 2, 2, 3, 3, 4 };

    // ── Sustain swell (movement) ── PURE ADDITIVE: the dance is the seed
    // idle PLUS the chord's contribution. goal = 1 + (CEILING−1)·t where
    // t ramps over the hold; silence gives 1 from the formula itself —
    // no branch, identity by construction. Music only ever gives;
    // idleness is inviolate. RULED: ceiling 2× idle at 8 beats.
    inline constexpr float RIBBON_SWELL_CEILING = 2.00f;  // × idle (ruled)
    inline constexpr float RIBBON_SWELL_RAMP = 8.0f;   // beats (ruled)
    // Envelope: the swell's goal is continuous during the ramp, so ATTACK
    // engages only at discontinuities (rare); RELEASE governs the breath
    // on re-articulation and the let-go after silence. Fast catch, slow
    // let-go. (A separate BREATH span for re-articulation is one line if
    // the dip wants independence from the final release — say the word.)
    inline constexpr float RIBBON_SWELL_ATTACK = 0.35f;  // beats
    inline constexpr float RIBBON_SWELL_RELEASE = 2.0f;   // beats

    // PITCH_VEC_ORIGIN survives the compass redesign: the tint's angle
    // law and the swappable seating live on it.
    inline constexpr float PITCH_VEC_ORIGIN = 0.0f;    // radians — rotates the hue seating

    // ── Line tint (color gen-2) ── the melody paints the ribbon: the
    // line's degree sets a hue by the SAME 30°-per-semitone law as the
    // compass (shared ORIGIN ⇒ cross-channel equivariance). The stimulus
    // is a TINTING VOICE at authored luma/chroma, mixed over the spawn
    // color; mix rises while the line sounds, releases to 0 in silence —
    // rest = the seed-drawn ribbon exactly. Compositional dials.
    inline constexpr float TINT_LUMA = 0.55f;
    inline constexpr float TINT_CHROMA = 0.35f;
    inline constexpr float TINT_MIX_MAX = 0.85f;
    // Envelope: the mix catches the room quickly and fades long on its
    // last hue; the hue itself re-aims between actives on one span.
    inline constexpr float TINT_MIX_ATTACK = 0.5f;   // beats
    inline constexpr float TINT_MIX_RELEASE = 3.0f;   // beats
    inline constexpr float TINT_HUE_SPAN = 2.0f;   // beats
    // Rodrigues basis about the gray axis (canvas-side twin of the skin's):
    inline constexpr float TINT_D1[3] = { 0.8165f, -0.4082f, -0.4082f };
    inline constexpr float TINT_D2[3] = { 0.0f,     0.7071f, -0.7071f };

    // ── DOOR AXES (Movement 1 harvest) ── signed coverage spans from the
    // door algebra at the tested ±0.80 sweep; the Movement-2 coupling
    // maps goals into these. Dials — nudge by taste.
    inline constexpr float TIDE_SHIFT_MIN   = -0.65f;  // zones fully closed
    inline constexpr float TIDE_SHIFT_MAX   =  0.75f;  // full flood
    inline constexpr float RAIN_SCATTER_MIN = -0.80f;  // countryside extinct (densest clusters linger)
    inline constexpr float RAIN_SCATTER_MAX =  0.25f;  // storm saturation

    // ── CHECKER-REBUILD — the pitch-class color field (the checker voice) ──
    // Source: the voice's WINDOW pc-LENGTH vector (window_length, 12-wide,
    // duration-weighted, dressed to D — the Playhead + Wagon compound,
    // pc_length(playhead, wagon(0))). NO DFT, no interval math: each PITCH
    // CLASS (ABSOLUTE, index 0 = D after the dress) has an authored RGB, and
    // the decode is the length-weighted average color —
    //     resultant = ( Σ_i length_i · PC_COLOR[i] ) / max(Σ length_i, eps)
    // Read every CHECKER_READ_SPAN beats. ENVELOPE (per Jean): LINEAR 2-beat
    // attack to the new target; LINEAR 8-beat release to rest (music_amount →
    // 0, which the GPU maps to each cell's OWN seed color — a return to seed,
    // not to gray). Two enveloped scalars ride alongside the resultant:
    //   music_amount  = presence [0,1] — the GPU's S1 pull + S2 wander scale;
    //   music_variance = distinct-pc count — the GPU's S3 within-patch spread.
    // The GPU (world.wgsl discrete_cell_color) owns pull / wander / spread;
    // this side ships the resultant + the two scalars through terrain.checker_*.
    // PC_COLOR is JEAN'S — twelve hues, one per pitch class. Tune it here.
    inline constexpr const char* CHECKER_VOICE = "ch1";   // the chordal piano; chN = wire = Ableton − 1
    inline constexpr float CHECKER_READ_SPAN = 4.0f;      // beats — read cadence
    inline constexpr float CHECKER_ATTACK    = 2.0f;      // beats — LINEAR rise to the new target
    inline constexpr float CHECKER_RELEASE   = 8.0f;      // beats — LINEAR fall to rest (→ seed color)
    //                          pc (dressed, 0 = D)      R      G      B    — Jean's twelve hues
    inline constexpr float PC_COLOR[12][3] = {
        /*  0  D  */ { 0.85f, 0.20f, 0.20f },   // red
        /*  1  D# */ { 0.85f, 0.45f, 0.15f },   // orange
        /*  2  E  */ { 0.90f, 0.80f, 0.20f },   // yellow
        /*  3  F  */ { 0.55f, 0.80f, 0.25f },   // yellow-green
        /*  4  F# */ { 0.25f, 0.75f, 0.30f },   // green
        /*  5  G  */ { 0.20f, 0.75f, 0.65f },   // teal
        /*  6  G# */ { 0.20f, 0.65f, 0.85f },   // cyan
        /*  7  A  */ { 0.25f, 0.40f, 0.85f },   // blue
        /*  8  A# */ { 0.45f, 0.30f, 0.85f },   // indigo
        /*  9  B  */ { 0.65f, 0.25f, 0.85f },   // violet
        /* 10  C  */ { 0.85f, 0.25f, 0.70f },   // magenta
        /* 11  C# */ { 0.85f, 0.25f, 0.45f },   // crimson
    };
    // Within-patch spread (S3): this side ships the ENVELOPED distinct-pc
    // count surplus (max(0, n-1), glided). The per-note gain and the ceiling
    // live GPU-side (world.wgsl §2.2 ROW 5: CHECKER_VAR_PER_NOTE /
    // CHECKER_VAR_MAX) so they hot-reload. 0 or 1 distinct → 0 extra spread.

    // ═══ MASTER CONTROL PANEL ════════════════════════════════════════════════════
    // The one place every exposed pipe is declared — name, slot, width, and the
    // value it rests at. Slots are assigned here, by hand, in this single table, so
    // there are no collisions across entities. Read it as a register map; every
    // coupling and every entity flush resolves against it by name. (A vector's rest
    // is one value across its channels; for fog.color the bind() seed sets the true
    // per-channel start, so the rest is only the pre-first-tick placeholder.)
    //
    //                          name           base count   rest
    inline constexpr ParamSlot PARAM_LAYOUT[] = {
        { "fog.density",          0,    1,    FOG_DENSITY_NONE },
        { "fog.color",            1,    3,    0.80f            },
        // ── ribbon (pitch compass) ── deviations composed over the seed
        // draws at the entity flush; rest = identity (1 = the seed's dance).
        { "ribbon.amp_lateral_mult",  4, 1, 1.0f },
        { "ribbon.amp_vertical_mult", 5, 1, 1.0f },
        { "ribbon.color_stim", 6, 3, 0.0f },
        { "ribbon.color_mix",  9, 1, 0.0f },
        // ── terrain (CHECKER-REBUILD, the pc-color field) ── checker_mean
        // now carries the resultant COLOR (rgb); checker_var widens to TWO —
        // [0] = music_amount (presence), [1] = music_variance (distinct-pc
        // count). All rest at 0 (amount 0 → the GPU shows each cell's seed
        // color; the_board's authored rests: terrain_looks ROW 2 REST_CHECKER_*).
        { "terrain.checker_mean", 10, 3, 0.0f },
        { "terrain.checker_var",  13, 2, 0.0f },
    };
    inline constexpr uint32_t PARAM_LAYOUT_COUNT =
        sizeof(PARAM_LAYOUT) / sizeof(PARAM_LAYOUT[0]);

    // WITNESS — the register map's teeth: every pipe within the bank,
    // no two pipes overlapping. Hand-laying stays; a collision is now a
    // build error, not a silent cross-write.
    static_assert([] {
        for (uint32_t i = 0; i < PARAM_LAYOUT_COUNT; ++i) {
            const ParamSlot& a = PARAM_LAYOUT[i];
            if (a.count < 1) return false;
            if (a.base < 0 || a.base + a.count > VISUAL_PARAM_SLOTS) return false;
            for (uint32_t j = i + 1; j < PARAM_LAYOUT_COUNT; ++j) {
                const ParamSlot& b = PARAM_LAYOUT[j];
                if (a.base < b.base + b.count && b.base < a.base + a.count) return false;
            }
        }
        return true;
        }(), "PARAM_LAYOUT: a pipe leaves the bank or two pipes overlap");

    // ═══ VISUAL CANVAS ═══════════════════════════════════════════════════════════

    class VisualCanvas {
    public:
        // Startup wiring: publish the control panel, lay the bank to its rests,
        // adopt the analysis layout, and resolve every coupling's source and target
        // once. tick() then never resolves.
        void bind(StatLayoutView analysis_layout) {
            param_layout_.bind(ParamLayoutView{ PARAM_LAYOUT, PARAM_LAYOUT_COUNT });
            param_layout_.reset(params_);

            signal_layout_.bind(analysis_layout);

            // fog: the held field → an absolute density and an atmospheric tint
            fog_field_ = signal_layout_.resolve("all.field");
            fog_density_ = param_layout_.resolve("fog.density");
            fog_color_ = param_layout_.resolve("fog.color");
            fog_seg_ = Segment{ FOG_DENSITY_NONE, FOG_DENSITY_NONE, 0.0f, 0.0f };
            for (int c = 0; c < 3; ++c) {
                fog_color_seg_[c] = Segment{ FOG_COLOR_BY_FIELD[0][c],
                                             FOG_COLOR_BY_FIELD[0][c], 0.0f, 0.0f };
            }

            // ribbon sources (the casting sheet): the voice's Playhead drives
            // the sustain swell; the room's Wagon aims the tint's hue; the
            // room's Playhead gates the tint's mix.
            {
                std::string v(RIBBON_VOICE);
                voice_playhead_ = signal_layout_.resolve((v + ".present_count").c_str());
            }
            room_wagon_ = signal_layout_.resolve("all.window_length");
            room_playhead_ = signal_layout_.resolve("all.present_count");
            amp_lat_ = param_layout_.resolve("ribbon.amp_lateral_mult");
            amp_vert_ = param_layout_.resolve("ribbon.amp_vertical_mult");
            amp_lat_seg_ = Segment{ 1.0f, 1.0f, 0.0f, 0.0f };
            amp_vert_seg_ = Segment{ 1.0f, 1.0f, 0.0f, 0.0f };
            tint_stim_ = param_layout_.resolve("ribbon.color_stim");
            tint_mix_ = param_layout_.resolve("ribbon.color_mix");
            for (int c2 = 0; c2 < 3; ++c2)
                tint_stim_seg_[c2] = Segment{ 0.0f, 0.0f, 0.0f, 0.0f };
            tint_mix_seg_ = Segment{ 0.0f, 0.0f, 0.0f, 0.0f };

            // CHECKER-REBUILD source + targets (the terrain's checker voice):
            // the voice's WINDOW pc-length vector becomes the resultant color;
            // presence + distinct-pc count envelope the pull and the spread.
            {
                std::string v(CHECKER_VOICE);
                checker_win_ = signal_layout_.resolve((v + ".window_length").c_str());
            }
            checker_mean_ = param_layout_.resolve("terrain.checker_mean");   // 3: resultant rgb
            checker_var_  = param_layout_.resolve("terrain.checker_var");    // 2: amount, variance
            for (int c2 = 0; c2 < 3; ++c2) {
                checker_res_goal_[c2] = 0.0f;                     // resultant color, held between reads
                checker_res_seg_[c2] = Segment{ 0.0f, 0.0f, 0.0f, 0.0f };
            }
            checker_amount_goal_ = 0.0f;                          // presence (rest 0 → seed)
            checker_amount_seg_  = Segment{ 0.0f, 0.0f, 0.0f, 0.0f };
            checker_var_goal_ = 0.0f;                             // distinct-pc spread (rest 0)
            checker_var_seg_  = Segment{ 0.0f, 0.0f, 0.0f, 0.0f };
            checker_next_read_ = 0.0f;   // first frame reads, then grid-locks

            // zoetrope ears (the listener set): one "chN.onset" resolve per
            // set bit of ZOETROPE_EARS. A miss warns and disables that ear —
            // the resolver's own semantics; the deaf ear simply never sums.
            zoetrope_ear_count_ = 0;
            for (int ch = 0; ch < 8; ++ch) {
                if (!(ZOETROPE_EARS & (1u << ch))) continue;
                std::string v("ch" + std::to_string(ch));
                zoetrope_ears_[zoetrope_ear_count_++] =
                    signal_layout_.resolve((v + ".onset").c_str());
            }
            for (int r = 0; r < 7; ++r) zoetrope_rows_[r] = 0.0f;
            // Boot witness — doctrine, not measurement (P6): one line,
            // always, so a deaf zoetrope names its fault at the seam.
            {
                int bound = 0;
                for (int e = 0; e < zoetrope_ear_count_; ++e)
                    if (zoetrope_ears_[e].valid) ++bound;
                std::fprintf(stderr, "[Zoetrope] ears bound: %d of %d (mask 0x%02X)\n",
                    bound, zoetrope_ear_count_, ZOETROPE_EARS);
            }

            // PORT_4c — THE SOCKET, in one line. Every signal-side
            // resolve above happens here, and with the BeatClock's empty
            // layout (CUT_1c) every one of them misses. The release twin
            // prints this summary; the debug twin has already printed
            // each source by name. Placed last, after the resolves it
            // counts, beside the Zoetrope witness it deliberately does
            // not replace — that line reports a different fact.
            if (signal_layout_.misses() > 0) {
                std::fprintf(stderr,
                    "[SignalLayout] %u sources unbound (no audio source)\n",
                    signal_layout_.misses());
            }
        }

        // One frame: run every coupling — read its source, decode inline, carry the
        // value on its Segment, write the bank. No GPU.
        void tick(const AnalysisSignal& signal) {
            const float beat = signal.t_beats;

            // ── fog ──────────────────────────────────────────────────────────────
            // The held field selects an absolute density and an atmospheric tint;
            // Segments carry both so they drift across a modulation rather than
            // snapping. One source, two pipes. Decode is a table index — inline,
            // not a goal object.
            if (fog_field_.valid) {
                const int f = (int)signal.stat(fog_field_.channel, fog_field_.base);
                const int idx = (f >= 0 && f < FOG_FIELD_COUNT) ? f : 0;

                if (fog_density_.valid) {
                    params_.set(fog_density_.base,
                        trajectory_release(fog_seg_, FOG_BY_FIELD[idx], beat, FOG_SPAN));
                }
                if (fog_color_.valid) {
                    for (int c = 0; c < 3; ++c) {
                        params_.set(fog_color_.base + c,
                            trajectory_release(fog_color_seg_[c],
                                FOG_COLOR_BY_FIELD[idx][c], beat, FOG_SPAN));
                    }
                }
            }

            // ── sustain swell (movement = TIME, the ribbon's voice) ─────
            // The dance swells with how long the current chord has held,
            // uninterrupted, on ch1's Playhead. Any change to the sounding
            // SET re-articulates: breathe to baseline (1), regrow.
            // Ruled: 1→2× over 8 beats. Silence ⇒ 1 ⇒ the seed dance.
            if (voice_playhead_.valid && amp_lat_.valid && amp_vert_.valid) {
                uint32_t mask = 0u;
                for (int i = 0; i < 12; ++i)
                    if (signal.stat(voice_playhead_.channel,
                        voice_playhead_.base + i) > 0.0f)
                        mask |= (1u << i);
                const float dbeats = beat - last_beat_;
                if (mask == 0u || mask != hold_mask_) hold_beats_ = 0.0f;
                else if (dbeats > 0.0f)               hold_beats_ += dbeats;
                hold_mask_ = mask;

                // One expression: hold==0 (silence or fresh chord) gives
                // goal 1 by itself. Re-articulation breathes to BASELINE.
                const float t = (hold_beats_ < RIBBON_SWELL_RAMP)
                    ? hold_beats_ / RIBBON_SWELL_RAMP : 1.0f;
                const float goal = 1.0f + (RIBBON_SWELL_CEILING - 1.0f) * t;
                params_.set(amp_lat_.base,
                    trajectory_release(amp_lat_seg_, goal, beat,
                        (goal == 1.0f ? RIBBON_SWELL_RELEASE : RIBBON_SWELL_ATTACK)));
                params_.set(amp_vert_.base,
                    trajectory_release(amp_vert_seg_, goal, beat,
                        (goal == 1.0f ? RIBBON_SWELL_RELEASE : RIBBON_SWELL_ATTACK)));
            }

            // ── room tint (color = the room) ────────────────────────────
            // The Wagon AIMS the hue (remembered center of mass — no argmax
            // flicker on chords); the Playhead GATES the mix (sounding ⇒
            // worn; silence ⇒ fades on its last hue).
            if (room_wagon_.valid && tint_stim_.valid && tint_mix_.valid) {
                // Unit-vector seating: the SWAPPABLE TABLE (one line).
                // Chromatic today (i·30°); circle of fifths ((7i mod 12)
                // ·30°) or an authored ordering are one-line futures —
                // the circle rework is PARKED with Jean's name on it; the
                // swappable line is now the th expression in the fill.
                // OIL_1 U5 (ledger: U4 hue loop, C4): the 12 angles are
                // compile-time-stable, so the vectors are seated ONCE — a
                // function-local static filled by the SAME std::cos/std::sin
                // expressions (identical bits by construction; cos/sin are
                // not constexpr in C++20). The per-frame loop reads the
                // table; only the weights vary.
                static const std::array<std::array<float, 2>, 12> PITCH_VECS = [] {
                    std::array<std::array<float, 2>, 12> t{};
                    for (int j = 0; j < 12; ++j) {
                        const float th = PITCH_VEC_ORIGIN + (float)j * 0.523598776f;
                        t[(size_t)j] = { std::cos(th), std::sin(th) };
                    }
                    return t;
                }();
                float vx = 0.0f, vy = 0.0f, energy = 0.0f;
                for (int i = 0; i < 12; ++i) {
                    const float w = signal.stat(room_wagon_.channel, room_wagon_.base + i);
                    if (w <= 0.0f) continue;
                    vx += w * PITCH_VECS[(size_t)i][0]; vy += w * PITCH_VECS[(size_t)i][1];
                    energy += w;
                }
                const float len = std::sqrt(vx * vx + vy * vy);
                if (len > 1e-4f) {
                    const float ca = vx / len, sa = vy / len;   // hue direction, no atan needed
                    for (int c2 = 0; c2 < 3; ++c2) {
                        const float v = TINT_LUMA
                            + (TINT_D1[c2] * ca + TINT_D2[c2] * sa) * TINT_CHROMA;
                        params_.set(tint_stim_.base + c2,
                            trajectory_release(tint_stim_seg_[c2], v, beat, TINT_HUE_SPAN));
                    }
                }
                else {
                    // window drained: stim segments hold their last hue; the
                    // MIX below is what releases — fade, not gray-out.
                    for (int c2 = 0; c2 < 3; ++c2)
                        params_.set(tint_stim_.base + c2,
                            trajectory_release(tint_stim_seg_[c2],
                                tint_stim_seg_[c2].to, beat, TINT_HUE_SPAN));
                }

                float room_sounding = 0.0f;
                if (room_playhead_.valid)
                    for (int i = 0; i < 12; ++i)
                        room_sounding += signal.stat(room_playhead_.channel,
                            room_playhead_.base + i);
                const float mix_goal = (room_sounding > 0.0f) ? TINT_MIX_MAX : 0.0f;
                params_.set(tint_mix_.base,
                    trajectory_release(tint_mix_seg_, mix_goal, beat,
                        (mix_goal == 0.0f ? TINT_MIX_RELEASE : TINT_MIX_ATTACK)));
            }

            // ── CHECKER-REBUILD (the window pc-lengths → a resultant color) ──
            // SAMPLE-AND-HOLD on the absolute beat grid: at each crossing,
            // read the voice's 12-pc WINDOW LENGTH vector, form the length-
            // weighted average color over Jean's PC_COLOR table (NO DFT, no
            // interval math — absolute pitch class → hue). Presence sets the
            // pull; distinct-pc count sets the spread. The ENVELOPE is on the
            // OUTPUT (below): each component glides LINEAR 2-beat attack to the
            // new target, and amount/variance release LINEAR over 8 beats to
            // rest — which the GPU maps to each cell's seed color. [CHECKER]
            // prints one witness line per read.
            if (checker_win_.valid && checker_mean_.valid && checker_var_.valid) {
                // CADENCE: re-anchor on a BACKWARD beat jump (a transport loop
                // back below next_read would otherwise freeze the reader).
                if (beat < checker_next_read_ - CHECKER_READ_SPAN)
                    checker_next_read_ = std::floor(beat / CHECKER_READ_SPAN) * CHECKER_READ_SPAN;
                if (beat >= checker_next_read_) {
                    float acc[3] = { 0.0f, 0.0f, 0.0f };
                    float total = 0.0f;
                    int   n = 0;
                    for (int i = 0; i < 12; ++i) {
                        const float w = signal.stat(checker_win_.channel,
                            checker_win_.base + i);
                        if (w <= 0.0f) continue;
                        acc[0] += w * PC_COLOR[i][0];
                        acc[1] += w * PC_COLOR[i][1];
                        acc[2] += w * PC_COLOR[i][2];
                        total += w;
                        ++n;
                    }
                    const bool present = (total > 1e-6f);
                    if (present) {
                        // The length-weighted average color. On silence we hold
                        // the last resultant (amount fades it to seed anyway).
                        checker_res_goal_[0] = acc[0] / total;
                        checker_res_goal_[1] = acc[1] / total;
                        checker_res_goal_[2] = acc[2] / total;
                    }
                    // Presence drives the pull; distinct-pc count surplus the
                    // spread (raw — the GPU scales + clamps it, hot-reloadable).
                    // 0 or 1 distinct → 0 (a lone note keeps the seed spread).
                    checker_amount_goal_ = present ? 1.0f : 0.0f;
                    checker_var_goal_ = present ? (float)std::max(0, n - 1) : 0.0f;
                    // THE WITNESS: one line per read, upstream of the GPU.
                    // On the instruments dial (core/instruments.hpp): a read
                    // lands every CHECKER_READ_SPAN beats — at 120 BPM that
                    // is an UNBUFFERED stderr write every two seconds, on the
                    // beat, which is exactly where a hitch is most visible.
                    if constexpr (INSTRUMENTS.checker_witness) {
                        std::fprintf(stderr,
                            "[CHECKER] n=%d total=%.2f resultant=(%.2f %.2f %.2f) distinct-1=%.0f\n",
                            n, total,
                            checker_res_goal_[0], checker_res_goal_[1], checker_res_goal_[2],
                            checker_var_goal_);
                    }
                    checker_next_read_ =
                        (std::floor(beat / CHECKER_READ_SPAN) + 1.0f) * CHECKER_READ_SPAN;
                }
                // ENVELOPE. Resultant glides on the 2-beat attack span (its goal
                // holds through silence, so no re-aim there). Amount + variance
                // rise on 2 beats and release to rest on 8 — the return-to-seed.
                for (int c2 = 0; c2 < 3; ++c2)
                    params_.set(checker_mean_.base + c2,
                        trajectory_release(checker_res_seg_[c2],
                            checker_res_goal_[c2], beat, CHECKER_ATTACK));
                params_.set(checker_var_.base,
                    trajectory_release(checker_amount_seg_, checker_amount_goal_, beat,
                        (checker_amount_goal_ > 0.0f ? CHECKER_ATTACK : CHECKER_RELEASE)));
                params_.set(checker_var_.base + 1,
                    trajectory_release(checker_var_seg_, checker_var_goal_, beat,
                        (checker_var_goal_ > 0.0f ? CHECKER_ATTACK : CHECKER_RELEASE)));
            }

            // ── the zoetrope's ears (row impulses) ──────────────────────
            // Sum the resolved ears' onset vectors into pc impulses and fold
            // them through the mode table. Published vectors ship DRESSED to
            // D (index 0 = D — the canvas contract the checker's PC_COLOR
            // table already binds); ZOETROPE_ROW_OF_PC is authored by raw
            // pitch class (0 = C), so the fold un-dresses: pc = (i + 2) % 12.
            // Overwritten every tick — impulses, not an accumulator; the
            // lattice integrates, this side only hears.
            for (int r = 0; r < 7; ++r) zoetrope_rows_[r] = 0.0f;
            for (int e = 0; e < zoetrope_ear_count_; ++e) {
                const SourceBinding& ear = zoetrope_ears_[e];
                if (!ear.valid) continue;
                for (int i = 0; i < 12; ++i) {
                    const float w = signal.stat(ear.channel, ear.base + i);
                    if (w <= 0.0f) continue;
                    zoetrope_rows_[ZOETROPE_ROW_OF_PC[(i + 2) % 12]] += w;
                }
            }

            last_beat_ = beat;   // single write, shared by the swell's hold clock
        }

        // Consumers read the bank (and resolve their pipe once through layout()).
        const VisualParams& params() const { return params_; }
        const ParamLayout& layout() const { return param_layout_; }

        // The zoetrope's row impulses — seven floats, bottom row = tonic,
        // overwritten each tick. The lattice (the_board) strikes from these.
        const float* zoetrope_rows() const { return zoetrope_rows_; }

    private:
        VisualParams params_;
        ParamLayout  param_layout_;
        SignalLayout signal_layout_;

        // ── fog coupling state ───────────────────────────────────────────────────
        SourceBinding fog_field_{};
        TargetBinding fog_density_{};
        TargetBinding fog_color_{};
        Segment       fog_seg_{};
        Segment       fog_color_seg_[3]{};

        // ── ribbon coupling state (sustain swell + room tint) ───────────────────
        SourceBinding voice_playhead_{};   // "<RIBBON_VOICE>.present_count" — the chord's sounding set
        SourceBinding room_wagon_{};       // "all.window_length" — the room's remembered chroma (aims the hue)
        SourceBinding room_playhead_{};    // "all.present_count" — the room sounding (gates the mix)
        uint32_t hold_mask_ = 0u;          // sustain state: the chord's set signature
        float    hold_beats_ = 0.0f;       //   and how long it has held, in beats
        float    last_beat_ = 0.0f;
        TargetBinding amp_lat_{};
        TargetBinding amp_vert_{};
        Segment       amp_lat_seg_{};
        Segment       amp_vert_seg_{};
        TargetBinding tint_stim_{};
        TargetBinding tint_mix_{};
        Segment       tint_stim_seg_[3]{};
        Segment       tint_mix_seg_{};

        // ── checker coupling state (CHECKER-REBUILD: the pc-color field) ─
        SourceBinding checker_win_{};         // "<CHECKER_VOICE>.window_length" — the 12-pc length vector
        float    checker_next_read_ = 0.0f;   // next absolute grid beat (sample-and-hold cursor)
        float    checker_res_goal_[3] = {};   // resultant color goal, held between reads
        float    checker_amount_goal_ = 0.0f; // presence goal [0,1]
        float    checker_var_goal_ = 0.0f;    // distinct-pc spread goal
        TargetBinding checker_mean_{};        // "terrain.checker_mean" (3): resultant rgb
        TargetBinding checker_var_{};         // "terrain.checker_var"  (2): [0]=amount [1]=variance
        Segment       checker_res_seg_[3]{};
        Segment       checker_amount_seg_{};
        Segment       checker_var_seg_{};

        // ── zoetrope coupling state (the ears + the fold) ────────────────
        SourceBinding zoetrope_ears_[8]{};    // "chN.onset" per set bit of ZOETROPE_EARS
        int           zoetrope_ear_count_ = 0;
        float         zoetrope_rows_[7] = {}; // row impulses, overwritten each tick
    };

} // namespace t7