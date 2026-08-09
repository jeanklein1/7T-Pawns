#pragma once

// ─── beat_clock.hpp ──────────────────────────────────────────────
//
// CUT_1c: the analysis intake's successor (ruling R7). MIDI/DAW
// analysis left the build; the render side keeps its two contracts —
// an AnalysisSignal each frame and a StatLayoutView once at bind.
// The BeatClock serves both from nothing but dt: advancing clocks at
// a variable BPM (default 100; this struct is the value's ONE home,
// panel-eligible), and an EMPTY layout.
//
// The empty layout is the audio socket. The render side resolves 12
// live source names against it — all.field, ch1.present_count,
// all.window_length, all.present_count, ch1.window_length,
// ch0.onset .. ch6.onset — and every resolve misses and disables its
// coupling via the graceful path (musical/signal_layout.hpp
// resolve(): one stderr warn, valid=false). A future browser-side
// audio source plugs into this socket by publishing exactly those
// names through a real StatLayoutView.

#include "analysis/analysis_signal.hpp"

namespace t7 {

struct BeatClock {
    float bpm = 100.0f;   // variable BPM — Jean's amendment; one home

    void update(float dt) {
        signal_.dt = dt;
        signal_.t_seconds += dt;
        signal_.t_beats += dt * (bpm / 60.0f);
    }

    // Every time-bearing field of the surviving contract, from the
    // clock; everything else (stats, pads) stays at the zero it was
    // value-initialized to ONCE, at construction (OIL_1 U2 — the
    // 4128-byte per-frame re-fill retired; ledger: X BeatClock, C3).
    // The stats plane has NO writer — that is the documented audio
    // socket, and a future soundtrack writer now has a persistent
    // home to fill instead of a per-frame temporary. AnalysisSignal
    // carries no transport flag — nothing to default.
    const AnalysisSignal& output() const { return signal_; }

    StatLayoutView stat_layout() const { return StatLayoutView{ nullptr, 0 }; }

private:
    // THE WRITE WINDOW (OIL_1 U2). signal_ is written BETWEEN frames —
    // update(dt), called once from frame() before render.update() — and
    // never during it. That matters because output() now hands out a
    // reference INTO this member rather than a per-frame copy: the whole
    // update spine reads it live (phase_advance_clock reads t_beats
    // twice), so a future soundtrack writer filling the stats plane must
    // publish on the same between-frames cadence, not from an audio
    // callback landing mid-spine. The empty layout is still the socket;
    // this is the contract that comes with the persistent home.
    AnalysisSignal signal_{};   // zero-filled once; update() writes the 3 live floats
};

} // namespace t7
