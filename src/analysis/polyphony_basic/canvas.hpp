#pragma once

/**
 * POLYPHONY BASIC - Analysis Cartridge
 * =====================================
 *
 * A multi-channel analysis cartridge that tracks polyphony (note count)
 * per named channel.
 *
 * SOURCES
 * -------
 * - MidiPort:    External MIDI input via system port (loopMIDI / DAW)
 * - MidiFile:    Plays a MIDI file in a loop
 * - KeyboardMidi: Computer keyboard as piano
 *
 * CHANNELS
 * --------
 * - "abbott"   on Ableton channel 1 (internal channel 0)
 * - "costello" on Ableton channel 2 (internal channel 1)
 * - "louise"   on Ableton channel 3 (internal channel 2) — live keyboard
 *
 * Each channel owns a MidiStream and a list of attached Trains. Every
 * incoming event is dispatched to the channel matching its MIDI channel
 * byte; events for unregistered channels are silently dropped.
 *
 * ANALYSIS
 * --------
 * - One Train per channel, each with one Playhead, computing polyphony.
 *
 * OUTPUT FORMAT
 * -------------
 * - stat(channel=0, slot=STAT_POLYPHONY_ABBOTT)   = abbott polyphony
 * - stat(channel=1, slot=STAT_POLYPHONY_COSTELLO) = costello polyphony
 * - stat(channel=2, slot=STAT_POLYPHONY_LOUISE)   = louise polyphony
 */

#include "analysis/analysis_cartridge.hpp"
#include "analysis/analysis_signal.hpp"
#include "core/clock.hpp"
#include "sources/midi_event.hpp"
#include "sources/midi_file.hpp"
#include "sources/keyboard_midi.hpp"
#include "sources/midi_port.hpp"
#include "musical/midi_stream.hpp"
#include "musical/playhead.hpp"
#include "musical/train.hpp"

#include <array>
#include <iostream>
#include <string>

namespace t7 {
namespace polyphony_basic {

// =============================================================================
// CONSTANTS
// =============================================================================

constexpr int MAX_NAMED_CHANNELS     = 4;
constexpr int MAX_TRAINS_PER_CHANNEL = 4;

// =============================================================================
// STAT SLOT DEFINITIONS
// =============================================================================
//
// This cartridge's output format. Visualization cartridges must know
// this mapping to interpret the signal.

constexpr int STAT_POLYPHONY_ABBOTT   = 0;
constexpr int STAT_POLYPHONY_COSTELLO = 1;
constexpr int STAT_POLYPHONY_LOUISE   = 2;

// =============================================================================
// CANVAS - The Analysis Cartridge Implementation
// =============================================================================

class Canvas : public AnalysisCartridge {
public:
    Canvas() = default;

    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;

    // ─── LIFECYCLE ──────────────────────────────────────────────────────────

    void initialize(const char* asset_path) override {
        clock_.set_bpm(120.0f);

        // Connect to loopMIDI if available
        if (midi_port_.open_by_name("loopmidi")) {
            std::cout << "Connected to MIDI port: "
                      << midi_port_.port_name() << "\n";
        }

        // Register channels (Ableton 1, 2, 3)
        register_channel("abbott",   1);
        register_channel("costello", 2);
        register_channel("louise",   3);

        // Configure each Train and attach to its channel
        setup_abbott_train();
        setup_costello_train();
        setup_louise_train();
        attach_train("abbott",   &abbott_train_);
        attach_train("costello", &costello_train_);
        attach_train("louise",   &louise_train_);

        // Try to load default MIDI as fallback
        if (asset_path) {
            std::string midi_path = std::string(asset_path) + "/default.mid";
            load_midi(midi_path.c_str());
        }
    }

    void update(float dt) override {
        clock_.tick(dt);
        float beat = clock_.t_beats();

        // Drain sources into channels
        route_midi_port_events(beat);
        route_midi_file_events(beat);
        route_keyboard_events();

        // For each active channel, snapshot the stream and run
        // every attached Train against it.
        for (int i = 0; i < channel_count_; ++i) {
            auto& ch = channels_[i];
            if (!ch.active) continue;

            ch.stream.update(beat);
            StreamSnapshot snap = ch.stream.snapshot();
            const CompletedRing& hist = ch.stream.history();

            for (int t = 0; t < ch.train_count; ++t) {
                ch.trains[t]->update(snap, hist, beat);
            }
        }

        finalize_output();
        prev_beat_ = beat;
    }

    // ─── INPUT ──────────────────────────────────────────────────────────────

    void on_input(const InputEvent& event) override {
        // We only care about key events for musical input
        if (event.type == InputEvent::Type::KeyDown) {
            on_music_key_down(event.character);
        } else if (event.type == InputEvent::Type::KeyUp) {
            on_music_key_up(event.character);
        }
    }

    // ─── OUTPUT ─────────────────────────────────────────────────────────────

    const AnalysisSignal& output() const override {
        return output_;
    }

    // ─── CONFIGURATION ──────────────────────────────────────────────────────

    /**
     * Load a MIDI file as a source.
     */
    bool load_midi(const char* path) {
        if (!midi_file_.load(path)) {
            midi_loaded_ = false;
            return false;
        }
        midi_file_.set_loop(true);
        midi_loaded_ = true;
        prev_beat_ = 0.0f;
        return true;
    }

    /**
     * Set the tempo (BPM).
     */
    void set_bpm(float bpm) {
        clock_.set_bpm(bpm);
    }

    /**
     * Get the clock (for external inspection).
     */
    const Clock& clock() const { return clock_; }

private:
    // ─── CHANNEL INSTANCE ───────────────────────────────────────────────────
    //
    // One per registered name. Owns its MidiStream; holds non-owning
    // pointers to attached Trains (the Trains themselves are owned by
    // the Canvas as direct members).

    struct ChannelInstance {
        std::string name;
        int channel = -1;   // 0-15 internal (Ableton 1-16 minus 1)
        MidiStream stream;
        std::array<Train*, MAX_TRAINS_PER_CHANNEL> trains{};
        int train_count = 0;
        bool active = false;

        void attach(Train* t) {
            if (train_count < MAX_TRAINS_PER_CHANNEL) {
                trains[train_count++] = t;
            }
        }
    };

    // ─── REGISTRATION API ───────────────────────────────────────────────────

    /**
     * Register a named channel. Ableton channels are 1-indexed
     * (matching the Ableton UI). Returns nullptr on failure
     * (capacity reached, or duplicate name).
     */
    ChannelInstance* register_channel(const std::string& name,
                                      int ableton_channel) {
        if (channel_count_ >= MAX_NAMED_CHANNELS) return nullptr;
        if (find_channel(name)) return nullptr;  // duplicate

        auto& ch = channels_[channel_count_];
        ch.name = name;
        ch.channel = ableton_channel - 1;  // Ableton 1-16 -> internal 0-15
        ch.train_count = 0;
        ch.active = true;
        ++channel_count_;
        return &ch;
    }

    /**
     * Attach a Train to a named channel. The Canvas does not own
     * the Train — it must outlive the Canvas (typically a member
     * of the same enclosing scope).
     */
    bool attach_train(const std::string& channel_name, Train* train) {
        auto* ch = find_channel(channel_name);
        if (!ch || !train) return false;
        ch->attach(train);
        return true;
    }

    ChannelInstance* find_channel(const std::string& name) {
        for (int i = 0; i < channel_count_; ++i) {
            if (channels_[i].active && channels_[i].name == name) {
                return &channels_[i];
            }
        }
        return nullptr;
    }

    // ─── TIME ───────────────────────────────────────────────────────────────
    Clock clock_;
    float prev_beat_ = 0.0f;

    // ─── SOURCES ────────────────────────────────────────────────────────────
    MidiFile midi_file_;
    bool midi_loaded_ = false;
    KeyboardMidi keyboard_{ 0, 100 };  // channel 0, max 100 events
    MidiPort midi_port_;

    // ─── CHANNELS + TRAINS ──────────────────────────────────────────────────
    std::array<ChannelInstance, MAX_NAMED_CHANNELS> channels_;
    int channel_count_ = 0;

    Train abbott_train_;
    Train costello_train_;
    Train louise_train_;
    TrainStatId abbott_polyphony_stat_;
    TrainStatId costello_polyphony_stat_;
    TrainStatId louise_polyphony_stat_;

    // ─── OUTPUT ─────────────────────────────────────────────────────────────
    AnalysisSignal output_;

    // ─── TRAIN SETUP ────────────────────────────────────────────────────────

    void setup_abbott_train() {
        int ph = abbott_train_.add_playhead();
        abbott_polyphony_stat_ = abbott_train_.define(
            [ph](const TrainContext& ctx) {
                return float(ctx.playhead(ph).current_count);
            });
    }

    void setup_costello_train() {
        int ph = costello_train_.add_playhead();
        costello_polyphony_stat_ = costello_train_.define(
            [ph](const TrainContext& ctx) {
                return float(ctx.playhead(ph).current_count);
            });
    }

    void setup_louise_train() {
        int ph = louise_train_.add_playhead();
        louise_polyphony_stat_ = louise_train_.define(
            [ph](const TrainContext& ctx) {
                return float(ctx.playhead(ph).current_count);
            });
    }

    // ─── EVENT ROUTING ──────────────────────────────────────────────────────

    void route_midi_port_events(float beat) {
        if (!midi_port_.is_open()) return;

        MidiEvent events[64];
        int count = midi_port_.poll(beat, events, 64);

        for (int i = 0; i < count; ++i) {
            dispatch_event(events[i]);
        }
    }

    void route_midi_file_events(float beat) {
        if (!midi_loaded_) return;

        MidiEvent events[128];
        int count = midi_file_.poll(prev_beat_, beat, events, 128);

        for (int i = 0; i < count; ++i) {
            dispatch_event(events[i]);
        }
    }

    void route_keyboard_events() {
        MidiEvent events[64];
        int count = keyboard_.poll(events, 64);

        for (int i = 0; i < count; ++i) {
            dispatch_event(events[i]);
        }
    }

    /**
     * Find the named channel matching the event's channel and
     * deliver the event to its stream. Events with no matching
     * registered channel are silently dropped.
     */
    void dispatch_event(const MidiEvent& ev) {
        for (int i = 0; i < channel_count_; ++i) {
            auto& ch = channels_[i];
            if (ch.active && ch.channel == ev.channel) {
                ch.stream.receive(ev);
                return;
            }
        }
    }

    // ─── KEYBOARD INPUT ─────────────────────────────────────────────────────

    void on_music_key_down(char key) {
        if (key >= 'A' && key <= 'Z') {
            keyboard_.on_key_press(key, clock_.t_beats());
        } else if (key == ';' || key == '[' || key == ']') {
            keyboard_.on_key_press(key, clock_.t_beats());
        }
    }

    void on_music_key_up(char key) {
        if (key >= 'A' && key <= 'Z') {
            keyboard_.on_key_release(key, clock_.t_beats());
        } else if (key == ';' || key == '[' || key == ']') {
            keyboard_.on_key_release(key, clock_.t_beats());
        }
    }

    // ─── OUTPUT FINALIZATION ────────────────────────────────────────────────

    void finalize_output() {
        output_.t_seconds = clock_.t_seconds();
        output_.t_beats   = clock_.t_beats();
        output_.dt        = clock_.dt();

        output_.set_stat(0, STAT_POLYPHONY_ABBOTT,
                         abbott_train_.get(abbott_polyphony_stat_));
        output_.set_stat(1, STAT_POLYPHONY_COSTELLO,
                         costello_train_.get(costello_polyphony_stat_));
        output_.set_stat(2, STAT_POLYPHONY_LOUISE,
                         louise_train_.get(louise_polyphony_stat_));
    }
};

} // namespace polyphony_basic
} // namespace t7
