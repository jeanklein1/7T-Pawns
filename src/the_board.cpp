/**
 * THE BOARD -- Render Cartridge Development Harness
 * =================================================
 *
 * Minimal runtime for the render cartridge. Cartridge selection is
 * controlled from CMakeLists.txt:
 *
 *   set(T7_RENDER_CARTRIDGE "the_board")
 *
 * CMake passes it as a compile definition (INCUBATE_RENDER).
 * No need to edit this file to switch cartridges.
 *
 * CONVENTION:
 *   Render cartridges:
 *   | Folder name          | Namespace              | Class     |
 *   |----------------------|------------------------|-----------|
 *   | the_board/           | t7::the_board          | Cartridge |
 *
 * The analysis side is the BeatClock (CUT_1c, ruling R7): advancing
 * clocks at a variable BPM, empty stat layout. All input goes to the
 * render cartridge.
 */

 // =========================================================================
 // TARGET SELECTION -- Provided by CMake, with fallback for manual override
 // =========================================================================

#ifndef INCUBATE_RENDER
#define INCUBATE_RENDER the_board
#endif

// =========================================================================
// MACRO MACHINERY -- Builds include paths and namespaces from defines
// =========================================================================

#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

#define CONCAT(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b

#define RENDER_HEADER(name)   STRINGIFY(cartridges/name/cartridge.hpp)

// =========================================================================
// INCLUDES
// =========================================================================

#include "console/console.hpp"
#include "analysis/beat_clock.hpp"

// IntelliSense cannot resolve macro-expanded #include paths.
// This literal include gives VS navigation (Peek Definition, Go To, etc.).
// The compiler ignores it -- the macro include below pulls in the same file.
#if defined(__INTELLISENSE__)
#include "cartridges/the_board/cartridge.hpp"
#else
#include RENDER_HEADER(INCUBATE_RENDER)
#endif

#include "core/instruments.hpp"   // THE INSTRUMENTS DIAL: INSTRUMENTS.watcher_ticks gates the hot-reload progress dot
#include "core/aubade.hpp"        // AUBADE U1 — the present mark, the tick probe, the first-present latch
#include "core/boot_params.hpp"   // DOMESDAY_1 B9 — parse_boot_params at the top of main
#include <iostream>
#include <chrono>
#include <emscripten.h>   // emscripten_set_main_loop / cancel — the rAF driver

// =========================================================================
// ACTIVE CARTRIDGE TYPES -- Derived from defines
// =========================================================================

namespace render_ns = t7::INCUBATE_RENDER;

using RenderCartridge = render_ns::Cartridge;

// Name for display
constexpr const char* RENDER_NAME = STRINGIFY(INCUBATE_RENDER);

// =========================================================================
// APP -- the loop-carried state (PORT_1a)
// =========================================================================
//
// The six locals that persisted across frame-loop iterations, homed in
// one struct so the loop body could live in frame() and be driven
// either by main()'s while (native) or by the browser's rAF (PORT_1c).
// Member order IS the old construction order; init calls stay in
// main(), verbatim and in sequence.

// ═══ THE READY OFFER'S FLOOR (OVERTURE_0) ════════════════════════════════
//
// "Controls:" was the line the shell dismissed the veil on (AUBADE U4
// moved that to first present; the line now arms the belt) — it either
// presents the world or offers the door, depending on whether the visitor
// has already tapped. The world is LIVE behind that veil, so the line is not
// "the world is ready", it is "you may look now".
//
// It used to print the instant the frame loop went live, which is before any
// painting can have landed: the eager tapper watched the boot ring dress in
// front of them. The offer now waits for a floor of paintings, or for a
// timeout, whichever comes first.
//
// THE TRADE, NAMED: the eager tapper waits at most OVERTURE_READY_TIMEOUT_S
// behind a veil that is already saying "Hanging the paintings (n/57)", and
// nobody sees the ring dress in front of them. A manifest that never lands is
// the timeout's case and needs no third arm.
inline constexpr uint32_t OVERTURE_READY_FLOOR     = 6;     // two galleries of three
inline constexpr float    OVERTURE_READY_TIMEOUT_S = 5.0f;  // a slow phone is not held hostage

struct App {
    t7::Console console;
    t7::BeatClock clock;
    RenderCartridge render;
    wgpu::Queue queue;
    bool world_ready = false;   // PORT_1c: init_world() ran (post-device init)
    // U9's two: when the frame loop went live, and whether the offer is spent.
    // One-shot by construction — once printed, the test is never evaluated again.
    std::chrono::steady_clock::time_point world_live{};
    bool controls_offered = false;
    // PANORAMA_1 U6 — THE METRONOME. rAF callbacks per presented frame.
    // 1 = every vblank; 2 = every second one, a steady 30 on a 60 Hz panel.
    uint32_t pace = 1;
    bool pace_applied = false;   // U2: the timing call is a one-shot from inside the loop
};

static App* app = nullptr;

// =========================================================================
// WORLD INIT -- everything that needs the DEVICE (PORT_1c binding)
// =========================================================================
//
// One home for the post-console init sequence, verbatim from main().
// Native called it from main() exactly where those lines were — same
// calls, same order, same failure handling. Web cannot: the device
// arrives asynchronously, so frame() calls this ONCE when boot reaches
// Ready. (The forced consequence of async boot: render.initialize
// needs console.device(), which does not exist at web main().)

static bool init_world() {
    std::cout << "[Incubator] BeatClock ready (bpm " << app->clock.bpm << ")\n";

    // --- Initialize Render Cartridge ----------------------------------------
    app->render.initialize(app->console.device());

    if (!app->render.init_renderer(app->console.color_format(), app->console.depth_format())) {
        std::cerr << "Failed to initialize " << RENDER_NAME << " renderer\n";
        return false;
    }

    std::cout << "[Incubator] " << RENDER_NAME << " renderer ready\n";

    // Publish the slot map once. The BeatClock's layout is EMPTY by design
    // (CUT_1c): every render-side resolve misses, warns once on stderr, and
    // leaves its coupling disabled — the graceful path in signal_layout.hpp.
    app->render.bind_signal_layout(app->clock.stat_layout());

    // THE OFFER IS NOT MADE HERE ANY MORE (OVERTURE_0). This is the instant
    // the frame loop goes live, which is the instant the wait is measured
    // from — see offer_controls_when_ready(), called from frame().
    app->queue = app->console.queue();
    app->world_live = std::chrono::steady_clock::now();
    app->world_ready = true;
    return true;
}

// ── the metronome, applied from inside the loop (WRAP_0 U2) ────────
//
// emscripten_set_main_loop RESETS the timing mode, and with
// simulate_infinite_loop it never returns — so the pace can only be armed
// from a frame, once, after the registration that would have cleared it.
// Idempotent by the flag: nothing else in this program re-arms the loop
// (one set_main_loop call in the tree, no resize or veil-lift path touches
// it), so one application holds for the session.
static void apply_pace_once() {
    if (app->pace_applied) return;
    app->pace_applied = true;
    t7::g_present_pace = app->pace;   // U3: the meter's window header names it
    if (app->pace == 1u) return;   // the default needs no call
    emscripten_set_main_loop_timing(EM_TIMING_RAF, (int)app->pace);
    std::cout << "[PACE] forced " << (60u / app->pace)
              << " fps target (rAF every " << app->pace << " vblank(s))\n";
}

// ── the offer, once the exhibition has a floor under it ────────────
//
// Called every frame until it fires.
//
// AUBADE U4 — THE VEIL NO LONGER READS THIS LINE. It used to: the shell
// dismissed on a line beginning "Controls:", which made stdout
// load-bearing and, worse, dismissed on a line that never claimed a frame
// had been presented — this is the OVERTURE READY floor (six authored
// paintings staged, or the timeout), and a world can reach it without the
// GPU ever having finished a frame. The veil now waits for first present
// itself (window.t7FirstPresent, armed at frame 1's submit).
//
// THE STRING IS STILL LOAD-BEARING, one notch weaker: the shell arms a
// 20 s belt on it, so a browser that renders but never resolves
// onSubmittedWorkDone still gets its world. Keep the text and its
// position as they are.
static void offer_controls_when_ready() {
    if (app->controls_offered) return;
    const float waited = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - app->world_live).count();
    // ORGANS ARE PUBLIC (the composition root's law): the gallery's own tally
    // is readable here without a face cut for it.
    const bool floor_met =
        app->render.gallery_state_.authored_staged_count >= OVERTURE_READY_FLOOR;
    if (!floor_met && waited < OVERTURE_READY_TIMEOUT_S) return;
    app->controls_offered = true;
    std::cout << "Controls: WASD=move, Mouse=camera, 5-8=moods, Esc=quit\n\n";
}

// =========================================================================
// FRAME -- the loop body, verbatim (the one token change: the acquire
// failure's `continue` is `return` here — same skip-this-frame meaning)
// =========================================================================

static void frame() {
    // --- Boot gate (PORT_1b/1c) ------------------------------------------
    // Web: rAF turns pump the boot until the device lands, then the world
    // initializes once. Native: boot_state() was Ready before the loop
    // ever ran — fell through immediately.
    if (app->console.boot_state() != t7::Console::BootState::Ready) {
        app->console.pump_boot();
        return;
    }

    // --- Device-loss gate (PORT_3a) --------------------------------------
    // Once the device is lost every wgpu object below is dead. Driving
    // them is what turns a normal web event into heap corruption, so the
    // frame stops here — before begin_frame, before any encoder, before
    // any submit. The reason already printed from the loss callback; this
    // gate stays silent so a lost device does not spam the console once
    // per rAF turn. No recovery attempt by design.
    if (app->console.device_lost()) return;
    if (!app->world_ready) {
        if (!init_world()) {
            emscripten_cancel_main_loop();
            return;
        }
    }

    // ══ AUBADE U3 — FIRST LIGHT GATES THE LOOP, AND ONLY FIRST LIGHT ═══
    //
    // Every pipeline is now created asynchronously (renderer.hpp), so
    // init_world returns with sixty compiles still outstanding. Most of
    // them may: a row whose pipeline has not arrived skips, and the world
    // assembles in view over a handful of frames exactly as its entities
    // already do. FIRST LIGHT is the set without which frame 1's shot is
    // visibly incomplete — the terrain and what writes it, and the
    // population at the centre of an open field — and the loop waits for
    // that and nothing else.
    //
    // THE FLAG IS FALSE UNTIL THE RENDERER SETS IT, so this gate is safe
    // before the renderer exists and safe if it never does. A first-light
    // pipeline that FAILS to compile counts as resolved, deliberately: a
    // dead row is a missing body, and a gate that never opens is a black
    // screen — which is the thing this campaign exists to end.
    //
    // RETURNING HERE SKIPS begin_frame FOR A FEW TURNS. Nothing is lost:
    // the veil is up, no input is live, and the pipeline callbacks are
    // AllowSpontaneous — they fire from the browser's event loop between
    // rAF turns and need no pump of ours to arrive.
    //
    // AUBADE_1 F2 — AND THE TICK IS COUNTED ABOVE IT, DELIBERATELY. The
    // probe's whole reading is "many cheap ticks against a late present
    // means the wait was DEVICE-SIDE" — and the gate below silently broke
    // that: it returns before the body, so the 4.4 s of compile counted
    // SEVEN ticks in a 5.4 s window instead of some three hundred. The
    // number proved the gate held, which is true and is not what it was
    // built to say. `ticks` is rAF turns in the init->present window, as
    // its banner has always claimed; `cpu` below is main-thread work in
    // those turns, and a gated turn does none. High ticks with low cpu is
    // the signature, and now it can appear.
    if (!t7::aubade_presented()) t7::aubade_ticks()++;
    if (!t7::g_first_light_ready) return;

    // THE METRONOME (WRAP_0 U2) — armed from inside the loop, once, because
    // the registration that would have cleared it never returns.
    apply_pace_once();

    // THE READY OFFER (OVERTURE_0) — the veil lifts on a world that has its
    // first paintings up, not on a world that has merely started.
    offer_controls_when_ready();

    // ═══ THE FRAME METER — S rows (OIL_1a; ledger: S0 host tail, C10) ══
    // TIMER LAW: these rows name where a wait SURFACES, not where the
    // cost lives — begin_frame carries the event pump, acquire and
    // present carry swapchain backpressure, finish_submit carries
    // command-buffer validation. Every clock pair sits behind the
    // instruments dial (folds to zero off, locals unused) and behind
    // world_ready (this line is below the boot and world-init gates).
    // A frame that fails the acquire notes NOTHING, so the S means stay
    // consistent with window_frames — which counts rendered frames only.
    std::chrono::steady_clock::time_point s_frame0{}, s_t0{};
    float s_begin = 0.0f, s_acquire = 0.0f, s_submit = 0.0f;
    if constexpr (t7::INSTRUMENTS.frame_meter) {
        s_frame0 = std::chrono::steady_clock::now();
        s_t0 = s_frame0;
    }

    // ══ AUBADE_1 F2 — THE PROBE'S OWN STAMP, AND WHY IT NEEDED ONE ═════
    //
    // `cpu` read 38,526 ms inside a 5.4 s window. It was summing s_frame0,
    // which is only ASSIGNED under `if constexpr (INSTRUMENTS.frame_meter)`
    // — and the meter is off in the shipped build, so s_frame0 sat at a
    // default-constructed epoch and `now() - s_frame0` was the absolute
    // steady_clock reading, summed once a frame. Timestamps, not deltas.
    //
    // The probe takes its own stamp, unconditionally, so it cannot inherit
    // the dial's state — and only while it is still counting, so a shipped
    // frame after first present pays one bool and nothing else.
    const bool a_counting = !t7::aubade_presented();
    std::chrono::steady_clock::time_point a_tick0{};
    if (a_counting) a_tick0 = std::chrono::steady_clock::now();

    float dt = app->console.begin_frame();
    // ORGAN — THE DIRTY FLUSH, at the frame boundary and nowhere else.
    // begin_frame has polled events and reconciled, so every writer for this
    // frame has spoken and the panel's edits are bits: this turns them into
    // at most one WriteBuffer per block (docs/ORGAN.md).
    app->render.organ_flush(app->queue);
    if constexpr (t7::INSTRUMENTS.frame_meter)
        s_begin = std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - s_t0).count();


    // --- Input (all of it is the world's) --------------------------------
    for (const auto& event : app->console.input_events()) {
        app->render.on_input(event);
    }
    app->console.clear_input_events();

    // --- Update ---------------------------------------------------------
    app->clock.update(dt);
    app->render.update(app->clock.output(), app->console.aspect_ratio(), app->queue);

    // --- Render ---------------------------------------------------------
    if constexpr (t7::INSTRUMENTS.frame_meter) s_t0 = std::chrono::steady_clock::now();
    if (!app->console.acquire_surface_texture()) {
        return;   // skipped frame — no S notes (see the S-row comment above)
    }
    if constexpr (t7::INSTRUMENTS.frame_meter)
        s_acquire = std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - s_t0).count();

    wgpu::CommandEncoderDescriptor encDesc{};
    encDesc.label = "frame";   // DOMESDAY_1 A9 (label law): named at creation
    wgpu::CommandEncoder encoder = app->console.device().CreateCommandEncoder(&encDesc);

    app->render.render(encoder, app->console.backbuffer(),
        app->console.msaa_color_view(),   // B10: null at msaa=1
        app->console.depth_view());

    wgpu::CommandBufferDescriptor cmdDesc{};
    if constexpr (t7::INSTRUMENTS.frame_meter) s_t0 = std::chrono::steady_clock::now();
    wgpu::CommandBuffer commands = encoder.Finish(&cmdDesc);
    app->queue.Submit(1, &commands);

    // ══ AUBADE U1 — THE FIRST PRESENT, AT LAST WITNESSED ═══════════════
    //
    // The program had NO first-present witness. `[FRAME_1]` was read as one
    // for a whole campaign and is CAP_1's canvas-resize census (R1); the
    // veil dismisses on a log-string sniff before any frame is submitted
    // (R9). So the moment the dark actually ends was, until this line,
    // unobserved by anything.
    //
    // onSubmittedWorkDone ON FRAME 1's SUBMIT is the honest instrument:
    // it resolves when the GPU has finished the work this submit
    // describes, which is the first moment there is a rendered frame at
    // all. Registering it here — after Submit, before present() — is what
    // makes it frame 1's and not some later frame's.
    //
    // ONCE. A callback per frame would be an instrument that costs a
    // readback forever to answer a question asked once.
    if (!t7::aubade_registered()) {
        t7::aubade_registered() = true;
        app->queue.OnSubmittedWorkDone(wgpu::CallbackMode::AllowSpontaneous,
            [](wgpu::QueueWorkDoneStatus, wgpu::StringView) {
                t7::aubade_presented() = true;
                t7::aubade_mark("present");
                // The probe is pushed HERE, in the callback, so its numbers
                // describe the window that just closed. Pushed at submit
                // they would describe one frame.
                t7::aubade_probe();
                // AUBADE U4 — and the veil learns it here, from the same
                // signal, so the handover cannot drift from the mark that
                // measures it.
                EM_ASM({
                    if (typeof window !== 'undefined' && window.t7FirstPresent)
                        window.t7FirstPresent();
                });
            });
    }
    // RIBBON_2 P0 1.2b: an updated-but-unrendered frame adds its dt to the
    // next rendered one — a dropped acquire stretches a step, never deletes
    // it. This is the moment the GPU has actually been handed the time the
    // cartridge was holding, and the only place the accumulator may clear.
    // The early return at acquire_surface_texture() above never reaches it.
    app->render.frame_submitted();
    if constexpr (t7::INSTRUMENTS.frame_meter)
        s_submit = std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - s_t0).count();

    // AUBADE U1 — THE TICK PROBE's other half. Counted only until first
    // present, then never touched again. `ticks` high with `cpu` low is the
    // signature of a DEVICE-SIDE wait — the main thread turning freely
    // while something downstream is not ready — which is what pipeline
    // compilation looks like from up here. `cpu` high is our own work, and
    // `stb` says how much of it was painting decode.
    //
    // AUBADE_1 F2 — THE TWO HALVES SIT AT DIFFERENT HEIGHTS, on purpose.
    // The tick is counted at the top of the frame, above the first-light
    // gate, because a gated turn IS an rAF turn. The cpu is accumulated
    // here, below every early return, because a turn that did no work
    // should add no work. That asymmetry is the whole reading.
    if (a_counting) {
        t7::aubade_cpu() += std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - a_tick0).count();
    }

    if constexpr (t7::INSTRUMENTS.frame_meter) s_t0 = std::chrono::steady_clock::now();
    app->console.present();
    if constexpr (t7::INSTRUMENTS.frame_meter) {
        const auto s_end = std::chrono::steady_clock::now();
        const float s_present = std::chrono::duration<float, std::milli>(s_end - s_t0).count();
        const float s_total = std::chrono::duration<float, std::milli>(s_end - s_frame0).count();
        using HostRow = RenderCartridge::HostRow;
        app->render.meter_note_host(HostRow::Begin, s_begin);
        app->render.meter_note_host(HostRow::Acquire, s_acquire);
        app->render.meter_note_host(HostRow::FinishSubmit, s_submit);
        app->render.meter_note_host(HostRow::Present, s_present);
        app->render.meter_note_host(HostRow::FrameTotal, s_total);
    }
}

// =========================================================================
// MAIN
// =========================================================================

int main(int argc, char* argv[]) {
    // DOMESDAY_1 B9 — the parameter surface, parsed before ANY
    // consumer: the cartridge ctor (inside `new App()` below) reads
    // seed and mood; the console reads cap. One read, never again.
    t7::parse_boot_params(argc, argv);
    std::cout << "\n";
    std::cout << "========================================\n";
    // PORT_2d — the banner states what this build actually has. The
    // FileWatcher class, its member, the watch() call and the per-frame
    // check went with the native arm (SUNSET_1), so there is no hot
    // reload to announce and no longer any twin to contrast against.
    std::cout << "  THE BOARD (web twin — no hot reload)\n";
    std::cout << "  Clock:    BeatClock\n";
    std::cout << "  Render:   " << RENDER_NAME << "\n";
    std::cout << "========================================\n";
    std::cout << "\n";

    // --- Initialize Console -------------------------------------------------
    app = new App();
    if (!app->console.init("The Board", 1280, 720)) {
        std::cerr << "Failed to initialize console\n";
        // EXHIBIT_0 — A REFERENCE OUTLIVES ITS REFERENT (LAWS L15), and
        // here the reference is a fetch. The cartridge constructor ran
        // inside `new App()` above, BEFORE this init — and on this twin
        // it started the exhibition.json request, holding &gallery_state_
        // as its userdata. Returning from main does not tear the browser
        // runtime down, so that request still lands, and freeing App
        // first would make it land in freed memory. The App is
        // deliberately leaked: the page is over, the leak is the last
        // thing that dies, and it is what keeps the callback honest.
        return 1;
    }
    app->console.set_cursor_grab(true);   // the exhibition holds the pointer

    // --- The Clock -----------------------------------------------------------
    // BeatClock needs no initialization: it starts at zero and advances
    // from dt alone. No command-line input either.
    (void)argc; (void)argv;

    // --- The rAF loop (PORT_1c) ----------------------------------------------
    // Boot continues asynchronously from here; frame() pumps the boot state
    // and runs init_world() once the device lands. 0 = rAF-paced; true =
    // this call never returns.
    // ═══ THE METRONOME (PANORAMA_1 U6) ═══════════════════════════════════
    //
    // A vsynced display shows a frame at 16.6, 33.3 or 50 ms — never at 20.
    // A device whose frame costs 20 ms does not run at 50 fps; it alternates
    // between one and two refreshes, and the eye reads that alternation as
    // stutter where it reads a steady 30 as slowness. A film at 24 is smooth;
    // a game at 45 is not. On the laptop a 1.3 ms difference between two
    // worlds flipped it from 48 fps to a 12/24 ms alternation at 40 — which
    // is the whole argument: between one and two vblanks, PACE is the cure
    // and shaving load is chasing a threshold.
    //
    // THE STEADY CLOCK NEEDS NOTHING. It already expresses each frame as an
    // integer multiple k of a MEASURED refresh period and serves k x period
    // exactly (console.hpp, THE PRESENTATION LAW). At pace 2 every frame is
    // k = 2 — well inside PRESENT_MAX_MULTIPLE 4 — so the world is integrated
    // for exactly the time it is displayed, with no term anywhere that
    // assumes 16.7 ms.
    //
    // `?pace=2` forces it, and is here so the taste gate can be taken: the
    // same world, the same ride, at each pace, and Jean says which is the
    // piece. The governor that would choose this by measurement is NOT in
    // this round — see docs/OPEN.md.
    // THE SWITCH IS APPLIED FROM INSIDE THE LOOP, NOT HERE (WRAP_0 U2). The
    // call below takes simulate_infinite_loop = true, which unwinds the stack
    // and NEVER RETURNS — the line above already said so, three lines up, and
    // PANORAMA_1 U6a still put the timing call after it. It was dead code, so
    // `?pace=2` was inert: the param parsed, the world announced it, and the
    // loop ran at one callback per vblank exactly as before.
    //
    // Emscripten also RESETS the timing mode inside set_main_loop, so even a
    // call placed before it would be overwritten. The only correct place is
    // after the loop is registered, which — since the registration does not
    // return — means from within a frame. See apply_pace_once(), called from
    // frame().
    app->pace = t7::boot_params().has_pace ? t7::boot_params().pace : 1u;
    emscripten_set_main_loop(frame, 0, true);
    return 0;
}
