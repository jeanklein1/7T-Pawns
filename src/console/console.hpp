#pragma once

// ─── console.hpp ─────────────────────────────────────────────────
//
// The 7T runtime infrastructure: the platform shell for the visualizer.
// Owns the window, GPU device, surface, depth buffer, input collection,
// and frame timing. Does NOT own cartridges, musical interpretation, or
// visual interpretation.
//
// Organized in lifecycle order:
//   §1  IDENTITY       — Constructor, destructor, copy prevention
//   §2  INITIALIZATION — init() and the helpers it calls, in call order
//   §3  FRAME          — begin_frame(), acquire, present, running
//   §4  INPUT          — Injection (producer) then access (consumer)
//   §5  ACCESSORS      — Handles and properties for external use
//   §6  SHUTDOWN       — shutdown(), request_close()
//   §7  STATE          — Member variables, grouped by responsibility
//
// Usage:
//   Console console;
//   if (!console.init("7T Visualizer", 1280, 720)) return 1;
//   while (console.running()) {
//       float dt = console.begin_frame();
//       // ... update cartridges ...
//       console.present();
//   }

#include "core/input_event.hpp"
#include "core/boot_params.hpp"   // DOMESDAY_1 B9 — ?cap= / --cap= (effective_pixel_cap below)
#include "core/boot_card.hpp"     // IOS_3 B — the program's own surface: facts, and last words
#include "core/aubade.hpp"       // AUBADE U1 — the waterfall's marks
#include "core/instruments.hpp"  // WIT_2 — t7::g_dropped_submits, the frame-validity witness

#include <webgpu/webgpu_cpp.h>

// ── PORT_1b Region 1: platform includes ──────────────────────────
// One program, one set (SUNSET_1). WebGPU arrives through emdawnwebgpu
// and the window through contrib.glfw3; there is no OS handle to expose
// and no native Dawn to link, so nothing here is conditional any more.
#include <GLFW/glfw3.h>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLFW/emscripten_glfw3.h>   // emscripten_glfw_make_canvas_resizable (FRAME_0)

#include <algorithm>
#include <vector>
#include <chrono>
#include <cmath>       // std::sqrt — the stick's magnitude (SHIP_1); std::round/fabs — RIBBON_6's presentation law
#include <cstdio>      // std::printf — RIBBON_6's [PRESENT] histogram (meter builds)
#include <cstdint>     // uint64_t / UINT64_MAX — the touch table's birth counter (SHIP_1)
#include <iostream>
#include <string>
#include <string_view>
#include <optional>

namespace t7 {

    // ═══ WEB PRESENTATION CONSTANTS (PORT_3c) ════════════════════════
    //
    // THE PIXEL CAP — the largest mobile lever this program has, and it
    // is a scalar. A phone at 390x844 CSS px with devicePixelRatio 3
    // renders 1170x2532 = 2.96 M pixels, nearly TRIPLE a 1366x768
    // laptop: a device three times faster can present itself as slower.
    // The ratio multiplies fragment work, the surface backbuffer, and
    // every full-screen attachment together, so it is also a memory
    // dial, not only a speed one.
    //
    // 1.5 keeps edges visibly crisper than 1.0 at 2.25x the pixels
    // rather than 9x. PANEL-ELIGIBLE and deliberately alone up here:
    // this is the number to move when a device cannot keep up, and
    // moving it changes nothing else. 1.0 = CSS-pixel rendering;
    // a very large value = uncapped, the pre-PORT_3c behavior.
    // COMPILE-TIME: this constant is the default, and the ONLY override is
    // the boot parameter read once at startup (effective_pixel_cap, below) —
    // there is no mid-run channel, so nothing can retune it while it runs.
    inline constexpr float MAX_DEVICE_PIXEL_RATIO = 1.5f;

    // ═══ THE FLOORS' ONE HOME (DOMESDAY_2 A12) ═══════════════════════
    // The granted-vs-floor line used to hand-carry its six literals;
    // they live in the schema's NEEDS table now, emitted here. This
    // closes LANTERN's print-vs-enforce hazard: the log reads the
    // wallet's own statement of need on the actual device.
#include "console/limits_floor.gen.inc"

    // ═══ THE FEATURE WALLET'S ONE HOME (PROBATE_F) ═══════════════════
    // The device-request site hand-carried its feature list the same way
    // the floor line once hand-carried its literals. The schema's
    // FEATURES table is its home now: nineteen optional features are
    // offered, one is granted, five are vaulted with a price, and
    // witness F-1 holds the emitted request to the schema's granted set.
    // A grant is a schema edit plus Jean's gate — never an edit here.
#include "console/features_wallet.gen.inc"

    // DOMESDAY_1 B9 — the runtime override: a ?cap= / --cap= present at
    // boot (clamped to [0.5, 3.0] at parse) replaces the constant for
    // THIS RUN; absent, the constant stands unchanged. Boot-read once,
    // no mid-run reread — the soak walk's key, so cap 2.25 can be
    // priced against the purse on the audience device without a
    // rebuild per arm.
    inline float effective_pixel_cap() {
        return boot_params().has_cap ? boot_params().cap
                                     : MAX_DEVICE_PIXEL_RATIO;
    }

    // ═══ THE COMPILER PLAN (PIVOT_0, 2026-08-12) ═════════════════════
    //
    // PIVOT_0 — the native shader-compiler plan. The web twin's
    // compiler is the browser's own; this constant governed
    // native only. world.wgsl is single-source across all values.
    //
    // Why it exists: WALLET_0's occupier cbuffer arrays stalled
    // update_player_agent at 20,227 ms under FXC and then
    // D3DCompiler_47 access-violated on the next room kernel. Jean
    // ruled the floor up rather than the shader down. The audience
    // floor is WebGPU core through modern compilers — Tint→DXC
    // (SM6.0+), Tint→MSL, Tint→SPIR-V, naga.
    //
    // D3D12_Fxc exists for ARCHAEOLOGY ONLY. It reproduces the retired
    // gate so a historical result can be re-run; it is not a supported
    // floor and nothing should be shaped to satisfy it. The laws it
    // used to impose are in docs/FXC_LAWS_RECORD.md.
    //
    // Plan B is one line: if DXC fails on a given driver, set this to
    // Vulkan, rebuild, boot. That IS the fallback, not a failure.
    enum class CompilerPlan { D3D12_Dxc, Vulkan, D3D12_Fxc };
    inline constexpr CompilerPlan kCompilerPlan = CompilerPlan::Vulkan;

    inline constexpr const char* compiler_plan_name(CompilerPlan p) {
        switch (p) {
        case CompilerPlan::D3D12_Dxc: return "DXC";
        case CompilerPlan::Vulkan:    return "VULKAN";
        case CompilerPlan::D3D12_Fxc: return "FXC";
        }
        return "?";
    }

    // ═══ THE FEATURE NAME TABLE (DOMESDAY_1 A8, R6) ══════════════════
    //
    // A switch over wgpu::FeatureName ENUMERATOR IDENTIFIERS — never
    // numeric values, so the compiling header supplies every value and
    // the build gate is the witness that each identifier exists. Names
    // are the WebGPU spec's kebab-case feature strings. An id the
    // switch does not know returns nullptr and the caller prints the
    // number — unknown ids stay honest.
    inline const char* feature_name(wgpu::FeatureName f) {
        switch (f) {
        case wgpu::FeatureName::CoreFeaturesAndLimits:         return "core-features-and-limits";
        case wgpu::FeatureName::DepthClipControl:              return "depth-clip-control";
        case wgpu::FeatureName::Depth32FloatStencil8:          return "depth32float-stencil8";
        case wgpu::FeatureName::TimestampQuery:                return "timestamp-query";
        case wgpu::FeatureName::TextureCompressionBC:          return "texture-compression-bc";
        case wgpu::FeatureName::TextureCompressionBCSliced3D:  return "texture-compression-bc-sliced-3d";
        case wgpu::FeatureName::TextureCompressionETC2:        return "texture-compression-etc2";
        case wgpu::FeatureName::TextureCompressionASTC:        return "texture-compression-astc";
        case wgpu::FeatureName::TextureCompressionASTCSliced3D: return "texture-compression-astc-sliced-3d";
        case wgpu::FeatureName::IndirectFirstInstance:         return "indirect-first-instance";
        case wgpu::FeatureName::ShaderF16:                     return "shader-f16";
        case wgpu::FeatureName::RG11B10UfloatRenderable:       return "rg11b10ufloat-renderable";
        case wgpu::FeatureName::BGRA8UnormStorage:             return "bgra8unorm-storage";
        case wgpu::FeatureName::Float32Filterable:             return "float32-filterable";
        case wgpu::FeatureName::Float32Blendable:              return "float32-blendable";
        case wgpu::FeatureName::ClipDistances:                 return "clip-distances";
        case wgpu::FeatureName::DualSourceBlending:            return "dual-source-blending";
        case wgpu::FeatureName::Subgroups:                     return "subgroups";
        case wgpu::FeatureName::TextureFormatsTier1:           return "texture-formats-tier1";
        case wgpu::FeatureName::TextureFormatsTier2:           return "texture-formats-tier2";
        // DOMESDAY_2 A13 — spec-cited additions, glaw1-pruned (F1-a):
        // SubgroupSizeControl was rejected by the native Dawn header
        // and removed per A8's standing protocol — its id keeps
        // printing as a number on both twins; re-add the case when the
        // Dawn checkout's wgpu::FeatureName learns the identifier. The
        // Pixel's unknown ids 21/22 are expected to resolve to the two
        // below by enum order.
        case wgpu::FeatureName::PrimitiveIndex:                return "primitive-index";
        case wgpu::FeatureName::TextureComponentSwizzle:       return "texture-component-swizzle";
        default:                                               return nullptr;
        }
    }


    // ═══ THE INSTANCE ANCHOR (PORT_4a) ═══════════════════════════════
    //
    // A SECOND external reference to the WebGPU Instance, in static
    // storage, DELIBERATELY NEVER RELEASED. It exists so that no
    // lifetime accident anywhere in the program can take the external
    // count to zero — Dawn destroys every device made from an instance
    // whose last EXTERNAL reference drops, and a device torn out from
    // under a running frame loop is heap corruption a few frames later.
    //
    // This is a BELT, and the reason it is a belt and not a fix is
    // worth recording. PORT_4's census found NO drop in our code:
    // Console::instance_ is assigned once, never released, never moved;
    // Console is non-copyable AND non-movable (a user-declared deleted
    // copy suppresses the implicit move), ~Console() is unreachable on
    // the web path because App is heap-allocated and never deleted, and
    // initWebGPU() has exactly one reachable call. So this anchor
    // should be redundant — and if the "[Device] LOST" line survives
    // it, that is PROOF the loss comes from outside this program and
    // the search moves to the browser.
    //
    // Shape borrowed from Dawn's own cross-platform sample, which holds
    // `static wgpu::Instance instance;` for exactly this reason.
    // Nothing releases it: Emscripten does not run static destructors
    // (EXIT_RUNTIME is off and main never returns — it unwinds).
    inline wgpu::Instance g_instanceAnchor;

    // ═══ WIT_2 — IS THIS ERROR A DROPPED FRAME? ══════════════════════
    //
    // Dawn reports a submit of an invalidated command buffer as a
    // VALIDATION error naming the command buffer as invalid. Matching on
    // both halves — the type AND the two words — keeps the count off every
    // other validation error the program could ever raise, which matters
    // because the number's whole value is that zero means one specific
    // thing. Substring matching is the honest tool here: the message text is
    // Dawn's to word, so a stricter parse would be a guess about a string we
    // do not own, and a looser one would count the wrong frames.
    inline void note_if_dropped_submit(wgpu::ErrorType type, std::string_view msg) {
        if (type != wgpu::ErrorType::Validation) return;
        if (msg.find("CommandBuffer") == std::string_view::npos &&
            msg.find("command buffer") == std::string_view::npos) return;
        if (msg.find("invalid") == std::string_view::npos &&
            msg.find("Invalid") == std::string_view::npos) return;
        ++t7::g_dropped_submits;
    }

    // ═══ SHIP_1 — TOUCH ══════════════════════════════════════════════
    //
    // THE PANEL. CameraControls' form, one module over: one organized
    // block, clear names, editable without hunting. It lives HERE and
    // not beside CameraControls because the gesture machine lives here —
    // the console owns the hand, the cartridge owns what the hand means.
    //
    // EVERY LENGTH IS CSS PIXELS. EmscriptenTouchPoint::targetX/targetY
    // are CSS px (clientX minus the canvas rect), and glfwGetWindowSize
    // reports CSS px too, so the midline and the stick are measured in
    // the same units the finger moves in. Feel therefore survives
    // devicePixelRatio — a 3x phone does not get a 3x-twitchy stick,
    // which is exactly the trap PORT_3c's cap comment warns about from
    // the other side.
    struct TouchControls {
        // The stick's throw: the drag at which the move vector reaches
        // full magnitude. About a thumb's comfortable arc.
        static constexpr float STICK_RADIUS    = 64.0f;
        // Below this the vector is exactly ZERO, not small — a resting
        // thumb must not walk the pawn.
        static constexpr float STICK_DEAD_ZONE = 8.0f;
        // Radians per CSS pixel. SEPARATE from the mouse's
        // CameraControls::look_sensitivity by design: a thumb sweeps a
        // fraction of the arc a mouse does, so one number cannot serve
        // both hands.
        static constexpr float LOOK_SENS_TOUCH = 0.006f;
        // Zoom units per CSS pixel of separation change. Feeds the same
        // zoom_delta channel the scroll wheel feeds.
        static constexpr float PINCH_SENS      = 0.06f;
        // A tap declares itself by a clean quick release; a pinch
        // declares itself by separation change OR by outliving this.
        static constexpr double TAP_MS         = 220.0;
        // Movement past this and the touch was never a tap.
        static constexpr float TAP_SLOP        = 12.0f;
        // Separation change past this declares a pinch immediately,
        // without waiting out TAP_MS.
        static constexpr float PINCH_DECLARE   = 8.0f;
    };

    // ONE TRACKED FINGER. `left` is decided once, at birth, and never
    // again — a thumb that slides across the midline keeps the identity
    // it was born with, so a wide drag cannot silently become a
    // different gesture halfway through.
    struct TouchPoint {
        int      id      = -1;
        bool     active  = false;
        bool     left    = false;
        uint64_t seq     = 0;       // birth order: who is primary, who is second
        float    x = 0.0f,  y = 0.0f;    // current, CSS px, canvas-relative
        float    x0 = 0.0f, y0 = 0.0f;   // where it landed
        double   t0 = 0.0;               // when it landed, ms
        bool     slopped = false;        // has moved past TAP_SLOP since landing
    };

    // The port's default canvas selector (Config.h kDefaultCanvasSelector).
    // lib_emscripten_glfw3.js registers it into specialHTMLTargets at
    // glfwInit, so findEventTarget resolves this exact string to the exact
    // element the port registered its own touch handlers on. That identity
    // is what makes the deregistration below hit its target.
    inline constexpr const char* TOUCH_TARGET = "Module['canvas']";

    class Console {

        // ═══ §1 IDENTITY ═════════════════════════════════════════

    public:
        Console() = default;
        ~Console() { shutdown(); }

        Console(const Console&) = delete;
        Console& operator=(const Console&) = delete;

        // ── PORT_1b: the boot grammar ────────────────────────────
        // Boot is a state machine. Native traversed it synchronously
        // inside init() (RequestingAdapter..Configuring never observed;
        // init() ended at Ready). Web starts an async request chain in
        // init() and the frame gate pumps Configuring → Ready once the
        // device callback lands. Failed is terminal — the cause has
        // already printed.
        enum class BootState { RequestingAdapter, RequestingDevice, Configuring, Ready, Failed };

        BootState boot_state() const { return bootState_; }

        // ── PORT_3a: the device can be lost ──────────────────────
        // On the web, device loss is NORMAL: tab backgrounding, GPU
        // resets, driver updates. A gallery installation running for
        // hours will see it. Once lost, every wgpu object the program
        // holds is dead, and continuing to drive them is what turns a
        // recoverable event into heap corruption. The policy is visible,
        // honest death — no recovery attempt: the frame gate stops
        // issuing GPU work and the reason is on the console verbatim.
        bool device_lost() const { return deviceLost_; }

        // Advance Configuring → Ready: runs the existing surface +
        // depth-buffer path once the device exists, then seeds the
        // frame clock. Native never needed it (init() reached Ready
        // synchronously) but it was callable there harmlessly:
        // every other state is a no-op.
        void pump_boot() {
            if (bootState_ != BootState::Configuring) return;
            if (!initSurface()) { bootState_ = BootState::Failed; return; }
            // ACQ_0: no depth buffer here. It is built at the first acquire,
            // at the size that acquire reports — a placeholder created now
            // could only be a guess, and a guess is what dropped the frames.
            lastTime_ = std::chrono::high_resolution_clock::now();
            bootState_ = BootState::Ready;
        }


        // ═══ §2 INITIALIZATION ═══════════════════════════════════
        //
        // Call order: initGLFW → initWebGPU → initSurface. Each step depends
        // on the previous. If any fails, init returns false. The depth
        // buffer is NOT part of boot any more (ACQ_0) — the first acquire
        // builds it, because the first acquire is the first honest size.

    public:
        bool init(const char* title, uint32_t width, uint32_t height) {
            initialWidth_ = width;
            initialHeight_ = height;
            currentWidth_ = width;
            currentHeight_ = height;

            if (!initGLFW(title))   { bootState_ = BootState::Failed; return false; }
            if (!initWebGPU())      { bootState_ = BootState::Failed; return false; }
            // Web: initWebGPU only STARTED the request chain; the frame
            // gate pumps Configuring → Ready when the device lands.
            return true;
        }

    private:
        bool initGLFW(const char* title) {
            if (!glfwInit()) {
                std::cerr << "Failed to initialize GLFW\n";
                return false;
            }

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            // ═══ CAP_1 — THE PORT STOPS SIZING THE BACKING STORE ═════════
            //
            // contrib.glfw3 is Hi-DPI aware by default (Config.h:46,
            // fScaleFramebuffer{GLFW_TRUE}), and "aware" means it OWNS the
            // canvas backing store: Window::setCanvasSize computes
            // floor(css × devicePixelRatio) and emglfw3w_set_size assigns it
            // straight to canvas.width/height. That is the pre-cap size the
            // Pixel reported. PORT_3c's cap never touched it — the cap only
            // altered the number we passed to Configure, and under the old
            // emdawnwebgpu generation Configure won, so the cap appeared to
            // work. Under the new generation the surface tracks the CANVAS,
            // the canvas is the port's uncapped number, and the cap was
            // silently defeated on the one device it exists for.
            //
            // The port offers this switch and no other: hi-DPI on or off,
            // with no way to supply a scale (Window.h getScale() returns
            // fMonitorScale or 1.0f; there is no setter and no port option).
            // So OFF it goes, and the cap moves wholly into app code where a
            // min() can live. With this FALSE the port writes
            // canvas.width = css and glfwGetFramebufferSize returns CSS px.
            //
            // Writing the backing store ourselves afterwards is SAFE, and the
            // reason is specific: emglfw3w_set_size pins the canvas CSS size
            // separately, via setCSSValue("width", …, "important"). Backing
            // store and layout size are decoupled, so assigning canvas.width
            // cannot change layout and cannot re-enter the port's
            // ResizeObserver. There is no feedback loop to fear.
            glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_FALSE);

            window_ = glfwCreateWindow(initialWidth_, initialHeight_, title, nullptr, nullptr);
            if (!window_) {
                std::cerr << "Failed to create window\n";
                glfwTerminate();
                return false;
            }

            // Set user pointer for callbacks
            glfwSetWindowUserPointer(window_, this);

            // Set callbacks
            glfwSetKeyCallback(window_, [](GLFWwindow* w, int key, int scancode, int action, int mods) {
                (void)scancode; (void)mods;
                auto* console = static_cast<Console*>(glfwGetWindowUserPointer(w));
                if (!console) return;


                // Numpad * — the pointer door. A window command of the same
                // class as ESC: it never reaches the cartridge fan, and the
                // cartridge never gains a window handle to serve it.
                if (key == GLFW_KEY_KP_MULTIPLY && action == GLFW_PRESS) {
                    console->toggle_cursor_grab();
                    return;
                }

                console->inject_key_event(key, action);
                });

            glfwSetCursorPosCallback(window_, [](GLFWwindow* w, double xpos, double ypos) {
                auto* console = static_cast<Console*>(glfwGetWindowUserPointer(w));
                if (!console) return;
                console->feed_cursor(xpos, ypos);
                });

            // A focus change un-applies and re-applies the OS cursor mode
            // beneath us; both edges move the pointer. Unprime so the first
            // sample after the seam is a new origin, not a jump.
            glfwSetWindowFocusCallback(window_, [](GLFWwindow* w, int focused) {
                (void)focused;
                auto* console = static_cast<Console*>(glfwGetWindowUserPointer(w));
                if (!console) return;
                console->unprime_cursor();
                });

            glfwSetMouseButtonCallback(window_, [](GLFWwindow* w, int button, int action, int mods) {
                (void)mods;
                auto* console = static_cast<Console*>(glfwGetWindowUserPointer(w));
                if (console) console->inject_mouse_button(button, action == GLFW_PRESS);
                });

            glfwSetScrollCallback(window_, [](GLFWwindow* w, double xoffset, double yoffset) {
                (void)xoffset;
                auto* console = static_cast<Console*>(glfwGetWindowUserPointer(w));
                if (console) console->inject_scroll(static_cast<float>(yoffset));
                });

            // ═══ FRAME_0 — THE LINK THAT WAS NEVER MADE ══════════════
            //
            // The fluid frame was a contract with a missing middle. The
            // shell tracked visualViewport into --app-h correctly; this
            // file reconfigured the surface on any framebuffer change
            // correctly, PORT_3c-capped. Between them, nothing: the
            // library pins the canvas at glfwCreateWindow's size and
            // enforces it with INLINE width/height
            // (lib_emscripten_glfw3.js, emglfw3w_set_size — its own
            // comment says "this will (on purpose) override any css
            // setting"; it asks for !important and does not get it,
            // because ctx.setCSSValue is a two-parameter arrow that
            // drops the priority — which changes nothing, since a plain
            // inline declaration already outranks a non-important author
            // rule). A stylesheet rule cannot win that, so the
            // canvas never moved, glfwGetFramebufferSize never changed,
            // and the per-frame compare below had nothing to compare.
            // One defect, three symptoms: a phone that fills neither
            // orientation, and an F11 that grows the browser but not
            // the world.
            //
            // This is the whole fix. The library observes #frame and
            // sizes the canvas from it, so the chain finally runs end
            // to end:
            //   #frame  <- CSS, from --app-h (the shell, unchanged)
            //   canvas  <- the library, from #frame
            //   backing <- the library, canvas x devicePixelRatio
            //   surface <- begin_frame's compare, PORT_3c-capped
            //
            // NO DPR HINT IS NEEDED, and that was worth checking rather
            // than assuming: GLFW_SCALE_FRAMEBUFFER already defaults to
            // GLFW_TRUE in the pinned port (Config.h:46), so the
            // framebuffer is floor(css x monitorScale) (Window.cpp:326)
            // and glfwGetWindowSize stays CSS px — exactly the two units
            // apply_pixel_cap's ratio math already assumes. Setting the
            // hint would have been a no-op; NOT having checked would
            // have risked a phone rendering at CSS resolution.
            //
            // Failure is non-fatal by choice: a missing #frame leaves
            // the pre-FRAME_0 behaviour (a fixed canvas), which is a
            // worse frame but still a world.
            if (emscripten_glfw_make_canvas_resizable(window_, "#frame", nullptr)
                != EMSCRIPTEN_RESULT_SUCCESS) {
                std::cerr << "[Frame] #frame not found — the canvas cannot track the page\n";
            }
            else {
                std::cout << "[Frame] Canvas tracks #frame\n";
            }

            // SHIP_1 U1 — after the window exists, because the port
            // registered ITS touch handlers inside glfwCreateWindow.
            // Order against the resizable call above is IMMATERIAL, and
            // the reason is worth stating so nobody preserves a
            // constraint that does not exist: that call only QUEUES a
            // resize request (Window.h, fResizeRequest), applied at the
            // first glfwPollEvents; and the port registers its canvas
            // listeners exactly once, at window creation, so no resize
            // path can land between our deregistration and our claim.
            claim_touch_stream();
            return true;
        }

        // ═══ PORT_5d — THE DEVICE REQUEST, TWICE IF NEEDED ═══════════
        //
        // The web twin asked for the adapter's MAXIMUM limits, the same
        // full passthrough native used. On a desktop that is harmless;
        // on a constrained device it is backwards — it tells the browser
        // to provision every ceiling at once when the program needs one.
        //
        // THE CENSUS behind the modest set (against WebGPU core
        // defaults; full table in this unit's commit body): the largest
        // uniform binding is GPUTileGrid at 16,400 B of 65,536; the
        // largest storage binding is Live Card Scratch at ~3.3 MiB of
        // 128 MiB; the widest workgroup is 16x16 = 256 invocations,
        // exactly the default and not over; texture array layers peak at
        // 225 of 256 (OPT_1b) and 2D dimension at 2048 of 8192
        // (PORT_5a). The last exceedance was
        // maxStorageBuffersPerShaderStage at 9 against a default of 8;
        // C6 (merged) demoted the field's head-pose window from
        // read-only storage to uniform, and TETRIS WALLET_0 demoted the
        // two occupier windows the same way — the room family now
        // counts 6.
        //
        // SO THIS IS NOW A PURE DEFAULTS REQUEST: nothing is named, and
        // a value-initialised wgpu::Limits means "every limit undefined,
        // use the default". If a future piece ever needs a ceiling above
        // a default, name it here — one line, beside this sentence, so
        // the exception is never silent.
        //
        // SAFETY: a mis-censused limit must degrade to today's behavior,
        // never to a black screen. Two nets. (1) If requestDevice fails,
        // the reason prints verbatim and the request is made again with
        // full passthrough. (2) If it SUCCEEDS but comes back below the
        // floor we censused — the failure mode if a value-initialised
        // wgpu::Limits ever meant "zero" rather than "undefined, use
        // default" — the device is discarded and the passthrough request
        // made anyway, because a device whose ceilings are zero fails at
        // pipeline creation later, far from this line and with nothing
        // pointing back here.
        // ═══ AUBADE U2 — THE CENSUS, LIFTED SO BOTH PATHS ANSWER TO IT ══
        //
        // This was the body of the RequestDevice callback's `if
        // (!passthrough)` branch, moved out WORD FOR WORD and called from
        // two places instead of one. That is the whole safety argument for
        // adoption: the page may ASK for a device, but nothing the page
        // produces is kept until this function has said yes to it, against
        // the same emitted floor, in the same order, printing the same
        // line. A second census would be a second opinion about the
        // treasury, and the program has one.
        //
        // Prints ALWAYS — the granted-vs-floor line is on the record
        // whether or not it disagrees. Returns false if any censused
        // ceiling came back below its floor; the CALLER decides what that
        // costs, because the two paths answer it differently (the request
        // path reissues with passthrough; the adopted path lets go and
        // asks from C++).
        bool device_meets_floor_(const wgpu::Device& dev) {
            const wgpu::Device& device = dev;   // the extracted body's name
            wgpu::Limits got{};
            device.GetLimits(&got);
            // PORT_6a (2) — granted vs the censused floor, always
            // printed, so the numbers are on the record whether or
            // not they disagree.
            //
            // LANTERN U1 — three rows added, each one a ceiling
            // this program's design actually stands on, and each
            // silent until now:
            //   maxTextureArrayLayers — the patch heightfield
            //     array is MAX_ACTIVE_PATCHES = 225 layers
            //     (state.hpp: "fits default maxTextureArrayLayers").
            //     225 of a 256 default is the tightest ceiling
            //     the program owns.
            //   maxUniformBuffersPerShaderStage — the LOOM wallet
            //     closed with the agents compute row at 11 of 12
            //     (BINDING_LEDGER Table B). One seat of margin.
            //   maxBindGroups — the LOOM recut spends all four
            //     strata at every pipeline. 4 of 4 is not margin,
            //     it is the design.
            // The DISCARD net below still reads the original
            // three: adding a floor to the discard decision is a
            // behavior change, not an instrument, and this
            // campaign prints only.
            //
            //   maxDynamicUniformBuffersPerPipelineLayout —
            //     ONE dynamic seat in the program: shadow_slot on
            //     frame R. 1 of the 8 default. (LATTICE_1 retired
            //     the second — patch_params left the dynamic seat
            //     for a read-only storage array when the bake
            //     became one batched dispatch.)
            //   maxComputeWorkgroupStorageSize — the CARD writer's
            //     node table + origins + its 20x20 tile, 4,912 B of
            //     the 16,384 default, and the program's largest
            //     per-entry-point sum since LATTICE_4 fused the
            //     card. The module states the number itself
            //     (CARD_WORKGROUP_BYTES) and carries two
            //     const_asserts: under the default, and at or above
            //     the bake's BAKE_WORKGROUP_BYTES (3,744) so this
            //     row always quotes the larger. This print is the
            //     runtime half.
            // The floors read the NEEDS table's emitted
            // constants; no literal lives in the C++.
            // AUBADE U1 — the device is in hand. Marked HERE and
            // not at the request, because the wait this campaign
            // is partitioning is the GRANT, not the asking.
            t7::aubade_mark("device");
            std::cout << "[Device] granted vs floor:"
                << " maxTextureDimension2D=" << got.maxTextureDimension2D
                << "/" << FLOOR_MAX_TEXTURE_DIMENSION_2D
                << " maxTextureArrayLayers=" << got.maxTextureArrayLayers
                << "/" << FLOOR_MAX_TEXTURE_ARRAY_LAYERS
                << " maxStorageBuffersPerShaderStage="
                << got.maxStorageBuffersPerShaderStage
                << "/" << FLOOR_MAX_STORAGE_BUFFERS_PER_SHADER_STAGE
                << " maxUniformBuffersPerShaderStage="
                << got.maxUniformBuffersPerShaderStage
                << "/" << FLOOR_MAX_UNIFORM_BUFFERS_PER_SHADER_STAGE
                << " maxUniformBufferBindingSize="
                << got.maxUniformBufferBindingSize
                << "/" << FLOOR_MAX_UNIFORM_BUFFER_BINDING_SIZE
                << " maxBindGroups=" << got.maxBindGroups
                << "/" << FLOOR_MAX_BIND_GROUPS
                << " maxDynamicUniformBuffersPerPipelineLayout="
                << got.maxDynamicUniformBuffersPerPipelineLayout
                << "/" << FLOOR_MAX_DYNAMIC_UNIFORM_BUFFERS_PER_PIPELINE_LAYOUT
                << " maxComputeWorkgroupStorageSize="
                << got.maxComputeWorkgroupStorageSize
                << "/" << FLOOR_MAX_COMPUTE_WORKGROUP_STORAGE_SIZE
                << " (floor)\n";
            bool below = false;
            if (got.maxTextureDimension2D < FLOOR_MAX_TEXTURE_DIMENSION_2D) {
                std::cerr << "[Device] BELOW FLOOR: maxTextureDimension2D granted "
                    << got.maxTextureDimension2D << ", floor "
                    << FLOOR_MAX_TEXTURE_DIMENSION_2D << "\n";
                below = true;
            }
            if (got.maxStorageBuffersPerShaderStage < FLOOR_MAX_STORAGE_BUFFERS_PER_SHADER_STAGE) {
                std::cerr << "[Device] BELOW FLOOR: maxStorageBuffersPerShaderStage"
                             " granted " << got.maxStorageBuffersPerShaderStage
                    << ", floor " << FLOOR_MAX_STORAGE_BUFFERS_PER_SHADER_STAGE << "\n";
                below = true;
            }
            if (got.maxUniformBufferBindingSize < FLOOR_MAX_UNIFORM_BUFFER_BINDING_SIZE) {
                std::cerr << "[Device] BELOW FLOOR: maxUniformBufferBindingSize"
                             " granted " << got.maxUniformBufferBindingSize
                    << ", floor " << FLOOR_MAX_UNIFORM_BUFFER_BINDING_SIZE << "\n";
                below = true;
            }
            return !below;
        }


        // ═══ AUBADE U2 — THE LINES EVERY ACCEPTED DEVICE PRINTS ═════════
        //
        // Also lifted verbatim, for the same reason: a device the page
        // granted must be described by the same block that describes a
        // device C++ granted, or the two boots are not comparable and the
        // whole overlap is unreadable.
        //
        // ONE THING IS PARAMETERISED, and it is the one thing that
        // genuinely differs. The adapter's OFFER — what silicon was
        // willing to give — is testimony only the holder of the adapter
        // handle can give, and on the adopted path C++ never holds one:
        // the adapter ceases to exist in this realm the moment adoption
        // succeeds. So each path composes that one sentence with what it
        // has (the request path: ids and names from the adapter; the
        // adopted path: the names the page stashed) and hands it in.
        // Everything else here reads device_, which both paths own.
        // ═══ LANTERN U1 — THE GRANTS CENSUS, web half ════════════════
        //
        // The optional-feature treasury (L20), printed at last. The web
        // console named no feature at all, yet the meter produced GPU
        // timings — so timestamp-query was granted and unreported, and
        // its ONLY evidence was the ABSENCE of the cartridge's "[METER]
        // timestamp-query unavailable" line. A grant witnessed by
        // silence is P6's exact complaint: a switch that cannot be seen
        // to have fired is indistinguishable from one that never fired.
        //
        // TWO FACTS, one line. What the adapter OFFERS bounds which
        // optional wings can ever open (L20 governs the request, this
        // governs the possibility) — and it is the half that arrives as
        // an argument, because only the path that held an adapter can
        // testify to it. What the DEVICE carries is what this boot
        // actually got, read here from device_ on both paths; the
        // program requests exactly one optional feature, so the named
        // half is complete, not a sample.
        //
        // DOMESDAY_1 A8 (R6): ids AND spellings — the pair is the
        // census. On the request path the names come from the enumerator
        // switch (feature_name, top of this file), never from numeric
        // values. On the adopted path there are no ids to pair: page JS
        // holds feature SPELLINGS and the port's numbering is internal
        // to the port. The line says so rather than inventing them.
        void report_device_(const std::string& offerIds,
                            const std::string& offerNames,
                            const char* which) {
            std::cout << "[Device] features: adapter offers " << offerIds
                << "; granted timestamp-query="
                << (device_.HasFeature(wgpu::FeatureName::TimestampQuery)
                        ? "YES" : "no")
                << " (the only optional feature this program requests)\n";
            // PROBATE_F — THE WALLET LINE. The line above says
            // what this boot GOT; this one says what the
            // program's treasury ASKS FOR and what it has
            // deliberately left unspent. Both halves come from
            // the schema's FEATURES table, so the testimony
            // cannot drift from the request beside it.
            std::cout << "[Device] feature wallet: granted ";
            for (uint32_t i = 0; i < FEATURE_WALLET_GRANTED_COUNT; i++) {
                std::cout << (i ? " " : "")
                    << FEATURE_WALLET_GRANTED_NAMES[i]
                    << (device_.HasFeature(FEATURE_WALLET_GRANTED[i])
                            ? "" : "(WITHHELD by adapter)");
            }
            std::cout << "; vaulted " << FEATURE_WALLET_VAULTED_COUNT
                << " (schema FEATURES)\n";
            std::cout << "[Device] features named: " << offerNames << "\n";
            // ═══ LANTERN U3 C1 — THE PIXEL CAP, NAMED ════════════
            //
            // The largest lever over the frame's biggest GPU row,
            // and the device block did not mention it. Every purse
            // number this program has printed was measured UNDER
            // this cap, so a device-fact block that hides it
            // overstates the treasury.
            //
            // THREE FACTS, because none of them is useful alone:
            // the cap in force, the ratio the device reports, and
            // that the cap is a COMPILE-TIME CONSTANT. A reader
            // seeing a cap below the device's ratio must know
            // whether that is a setting to change or a rebuild to
            // schedule — it is a rebuild (console.hpp ·
            // MAX_DEVICE_PIXEL_RATIO, consumed by
            // Console::apply_pixel_cap()).
            //
            // emscripten_get_device_pixel_ratio() reads
            // window.devicePixelRatio straight from the browser, so
            // it answers here regardless of canvas layout state —
            // no work is moved to reach it, and the line does not
            // wait for the first frame. The GLFW framebuffer/window
            // ratio would ALSO answer, but only after the library
            // has sized the canvas from #frame (FRAME_0), which has
            // not happened at device adoption.
            //
            // PRINT ONLY. This line does not steer the cap, change
            // its value, or touch apply_pixel_cap().
            std::cout << "[Device] pixel cap: " << effective_pixel_cap()
                << (boot_params().has_cap
                        ? " (boot param — this run only)"
                        : " (compile-time constant, not a setting)")
                << "; device dpr "
                << emscripten_get_device_pixel_ratio() << "\n";
            // PORT_6a (5) — the device the program actually keeps.
            std::cout << "[Device] KEEPING the device from: " << which
                << " (this is the one the frame loop runs on)\n";
        }

        void request_device_web(bool passthrough) {
            wgpu::DeviceDescriptor deviceDesc{};
            deviceDesc.label = "7T Device";
            // Deliberately unguarded — boot wants verbose errors.
            deviceDesc.SetUncapturedErrorCallback(
                [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView msg) {
                    const std::string_view text(msg.data, msg.length);
                    std::cerr << "WebGPU Error (" << static_cast<int>(type) << "): "
                        << text << std::endl;
                    note_if_dropped_submit(type, text);   // WIT_2
                    // IOS_3 B3 — AND ONTO THE PAGE. std::cerr reaches a
                    // console; the device that needs this has none. The
                    // message is the one thing no switch in the ladder can
                    // reproduce — validation NAMES what it refused.
                    card_err("gpu", "(" + std::to_string(static_cast<int>(type))
                                    + ") " + std::string(text));
                });
            // PORT_3a — the loss door. AllowSpontaneous so it fires from
            // the browser event loop without a pump. `this` is safe to
            // capture: App is heap-allocated and never deleted on the web
            // path, so Console outlives every callback.
            deviceDesc.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous,
                [this](const wgpu::Device&, wgpu::DeviceLostReason reason,
                       wgpu::StringView msg) {
                    deviceLost_ = true;
                    std::cerr << "[Device] LOST reason=" << static_cast<int>(reason)
                        << " : " << std::string_view(msg.data, msg.length) << std::endl;
                    // IOS_3 B3 — THE DISTINCTION THE LADDER CANNOT DRAW.
                    // A black screen with [lost] means the device DIED — a
                    // hang or a watchdog. A black screen with [gpu] means
                    // validation refused something. Nothing else we can
                    // build tells those two apart, and they point at
                    // opposite halves of the investigation.
                    card_err("lost", std::to_string(static_cast<int>(reason))
                                     + ": " + std::string(msg.data, msg.length));
                });

            wgpu::Limits limits{};
            if (passthrough) {
                adapter_.GetLimits(&limits);          // the old behavior, kept as the net
            }
            // else: every field stays undefined == every limit at its
            // core default. Post-C6 the program needs no exception.
            //
            // DO NOT "SIMPLIFY" THIS BACK TO PASSTHROUGH (PORT_6c). The
            // ground is COMPATIBILITY: a program that asks only for what
            // it uses runs on the widest set of devices, and the phone is
            // the target that decides. Asking the adapter for its maximum
            // narrows that set and buys nothing — the program never uses
            // the ceilings. L14 carries this as law.
            //
            // SHIP_0 U1 — the 11x timing claim that used to sit here
            // (62,517 vs 5,609 ms, "usable boot vs unusable") is
            // WITHDRAWN. It was one bisect on a machine whose runs vary
            // by an order of magnitude on identical code (native pipeline
            // creation has been observed at 70,459 and 205,527 ms). One
            // run from it is not evidence. Compatibility stands on its
            // own; do not re-argue this line with a number from this
            // laptop.
            //
            // THE REQUEST CARRIES NO EXCEPTION. Every limit this program
            // stands on is a WebGPU core default (NEEDS, limits_floor.gen.inc),
            // so the request asks for core defaults and nothing beside them —
            // and requestDevice, which REJECTS any required limit better than
            // the adapter's own, has nothing here to reject.
            deviceDesc.requiredLimits = &limits;

            // PORT_6a (1) — the request being issued, with its exceptions.
            if (passthrough) {
                std::cout << "[Device] requesting FULL ADAPTER PASSTHROUGH limits"
                             " (fallback path)\n";
            } else {
                std::cout << "[Device] requesting CORE DEFAULTS; exceptions carried: none\n";
            }

            // PROBATE_F — THE REQUEST READS THE WALLET, NOT A LITERAL.
            // The list is FEATURE_WALLET_GRANTED (features_wallet.gen.inc,
            // emitted from the schema's FEATURES table); this site asks
            // for what the adapter actually offers of it and nothing
            // else. A feature the adapter lacks is dropped rather than
            // demanded — requestDevice REJECTS an unofferable required
            // feature, and a rejected modest request reissues as full
            // passthrough (L14), the one shape the design forbids.
            wgpu::FeatureName askedFeatures[FEATURE_WALLET_GRANTED_COUNT] = {};
            size_t askedFeatureCount = 0;
            for (uint32_t i = 0; i < FEATURE_WALLET_GRANTED_COUNT; i++) {
                if (adapter_.HasFeature(FEATURE_WALLET_GRANTED[i])) {
                    askedFeatures[askedFeatureCount++] = FEATURE_WALLET_GRANTED[i];
                }
            }
            if (askedFeatureCount > 0) {
                deviceDesc.requiredFeatures = askedFeatures;
                deviceDesc.requiredFeatureCount = askedFeatureCount;
            }

            // IOS_3 B5 — THE WITNESS FOR THE FAILURE PATH. A last-words
            // surface nobody has seen fail is a surface nobody knows is
            // wired, so ?failboot=1 demands a feature this adapter was
            // just observed NOT to offer. requestDevice rejects an
            // unofferable required feature, which is exactly the shape
            // the iPad may be failing in — and here it is reproducible on
            // a machine that works.
            //
            // IT DELIBERATELY DEFEATS THE FILTER ABOVE. That filter is the
            // program's protection (L14: never reissue as passthrough);
            // this switch is a rehearsal of what happens when protection
            // is not enough, and it exists only behind an explicit flag.
            static wgpu::FeatureName failFeature = wgpu::FeatureName::ShaderF16;
            if (boot_params().failboot && !adapter_.HasFeature(failFeature)) {
                std::cout << "[Params] failboot=1 — demanding an unofferable "
                             "feature so the failure path can be witnessed\n";
                deviceDesc.requiredFeatures = &failFeature;
                deviceDesc.requiredFeatureCount = 1;
            }

            adapter_.RequestDevice(&deviceDesc, wgpu::CallbackMode::AllowSpontaneous,
                [this, passthrough](wgpu::RequestDeviceStatus status, wgpu::Device device,
                       wgpu::StringView message) {
                    const char* which = passthrough
                        ? "full adapter passthrough"
                        : "core defaults + censused exceptions";
                    if (status != wgpu::RequestDeviceStatus::Success) {
                        std::cerr << "RequestDevice failed (" << which << "): "
                            << std::string_view(message.data, message.length) << "\n";
                        // IOS_3 B3 — the earliest black screen there is: no
                        // device, so no callback above will ever fire and
                        // nothing downstream runs to notice.
                        card_err("device", std::string(which) + ": "
                                 + std::string(message.data, message.length));
                        if (!passthrough) {
                            // PORT_6a (4) — the reissue, failure branch.
                            std::cerr << "[Device] REISSUING request with full adapter"
                                         " passthrough (modest request was rejected)\n";
                            request_device_web(true);
                            return;
                        }
                        bootState_ = BootState::Failed;
                        return;
                    }
                    // Net (2) — verify before adopting, while `device` is
                    // still the local (it is moved from just below).
                    //
                    // AUBADE U2 — the census that used to live here now
                    // lives in device_meets_floor_, so the device the
                    // PAGE creates answers to the identical one. The
                    // verdict is the same; only the two paths' answers to
                    // a NO differ, and each says its own.
                    if (!passthrough) {
                        if (!device_meets_floor_(device)) {
                            // PORT_6a (3) — the discard decision, BOTH ways. The
                            // no-discard line is the informative one: it is what
                            // says a [Device] LOST later did not come from here.
                            std::cerr << "[Device] DISCARDING the modest device — its `lost`"
                                         " promise will resolve as a CONSEQUENCE of this"
                                         " discard, not as a failure\n";
                            // PORT_6a (4) — the reissue, discard branch.
                            std::cerr << "[Device] REISSUING request with full adapter"
                                         " passthrough\n";
                            request_device_web(true);
                            return;
                        }
                        std::cout << "[Device] modest device accepted — NO DISCARD\n";
                    }
                    device_ = std::move(device);
                    queue_ = device_.GetQueue();
                    // The adapter's offer, composed by the path that holds
                    // the handle — ids and spellings both, per DOMESDAY_1
                    // A8 (R6). Prints on BOTH request paths: the fallback
                    // passthrough is the path where the numbers matter most.
                    {
                        wgpu::SupportedFeatures feats{};
                        adapter_.GetFeatures(&feats);
                        std::string ids = std::to_string(feats.featureCount) + " (";
                        std::string names;
                        for (size_t i = 0; i < feats.featureCount; i++) {
                            if (i) { ids += " "; names += " "; }
                            ids += std::to_string(
                                static_cast<uint32_t>(feats.features[i]));
                            const char* nm = feature_name(feats.features[i]);
                            names += nm ? std::string(nm)
                                        : std::to_string(
                                              static_cast<uint32_t>(feats.features[i]));
                        }
                        ids += ")";
                        report_device_(ids, names, which);
                    }
                    bootState_ = BootState::Configuring;
                });
        }

        // ── SHIP_0 U2's identity line, lifted (AUBADE U2) ────────────
        //
        // Empty fields print "?" rather than nothing: some builds redact
        // these strings, and a blank field is indistinguishable from a
        // line that never ran. A capture reading all "?" is the RESOLVE
        // case — report it, do not plumb a fallback.
        //
        // Called with the info the adapter gave on the request path, and
        // with the info the DEVICE gives on the adopted one
        // (wgpuDeviceGetAdapterInfo, which the vendored port implements
        // — library_webgpu.js reads device.adapterInfo). Same struct,
        // same line, one home.
        void announce_adapter_(const wgpu::AdapterInfo& info) {
            auto sv = [](wgpu::StringView s) {
                return (s.data && s.length)
                    ? std::string_view(s.data, s.length)
                    : std::string_view("?");
            };
            card_fact("adapter " + std::string(sv(info.vendor))
                      + " | " + std::string(sv(info.architecture)));
            std::cout << "[Device] adapter: " << sv(info.vendor)
                      << " | " << sv(info.architecture)
                      << " | " << sv(info.device)
                      << " | " << sv(info.description) << "\n";
        }

        // ═══ AUBADE U2 — ADOPT THE DEVICE THE PAGE ALREADY ASKED FOR ═════
        //
        // WHAT THIS BUYS. Until this unit the boot was two waits in a row
        // that never needed to be: fetch-and-compile a multi-megabyte
        // wasm, run to main, THEN ask the browser for an adapter and a
        // device. U1's waterfall is what made the seam legible — `device`
        // sat after `wasm` by construction. web/index.html now asks at
        // parse time, in parallel with the fetch, and this function keeps
        // what it finds. The `device` mark moves with it (the page takes
        // it; the mark below is idempotent and no-ops), which is the
        // measurement the unit is judged on.
        //
        // WHY THIS IS NOT A SECOND BOOT PATH. The device is IMPORTED into
        // the vendored port's own object table
        // (emscripten_webgpu_get_device -> WebGPU.importJsDevice) and
        // ref-counted there. From the first line after this function it
        // is an ordinary wgpu::Device and nothing downstream can tell.
        //
        // THE DEVICE IS THE ONLY HANDLE THAT CROSSES. Adoption is
        // device-only: the adapter ceases to exist in this realm when
        // adoption succeeds, and no second port-undefined adoption is
        // attempted for it. Everything the boot used to read from the
        // adapter is read from the device side or not at all —
        //   identity ....... device_.GetAdapterInfo (the port implements it)
        //   granted limits . device_.GetLimits, the same census
        //   granted feature  device_.HasFeature, the same wallet line
        //   the OFFER ...... the page's list, because only the holder of
        //                    an adapter can testify to what silicon was
        //                    willing to give, and C++ never holds one here
        // and initSurface's GetCapabilities takes an adapter it does not
        // read (webgpu.cpp: both parameters unnamed; the answer is
        // navigator.gpu.getPreferredCanvasFormat), so a null adapter_ is
        // legal there and stays legal.
        //
        // ── C++ REMAINS THE AUTHORITY, AND THIS IS WHERE ─────────────
        //
        // Two refusals, and either one costs the overlap and nothing
        // else — the C++ request runs exactly as it did before this unit:
        //
        //   THE FLOOR. device_meets_floor_, the same function the request
        //   path calls, printing the same line against the same emitted
        //   floor.
        //
        //   THE WALLET. For every granted row the page says the adapter
        //   OFFERED, the device must actually carry it. This is what
        //   makes the literal wallet in web/index.html safe: if the page
        //   ever asks for less than features_wallet.gen.inc grants, the
        //   drift lands HERE, as a refusal with a name in it, and never
        //   as a silently feature-poor device that fails at some pipeline
        //   later with nothing pointing back.
        //
        // ── AND TWO SWITCHES STAND IT DOWN ───────────────────────────
        //
        //   ?adopt=0    the bisect: boot exactly as every build before
        //               this unit did. For the device with no console.
        //   ?failboot=1 IOS_3 B5's rehearsal lives in the C++ REQUEST, so
        //               a pre-created device would defeat it. The page
        //               reads this key too and stands aside on its own;
        //               this check is the C++ half of the same decision,
        //               and it holds even if the page's does not.
        //
        // Returns true only when device_ and queue_ are live and the boot
        // may proceed to Configuring.
        bool adopt_page_device_() {
            if (!boot_params().adopt) {
                std::cout << "[Device] adopt=0 — not taking the page's device;"
                             " requesting one from C++\n";
                return false;
            }
            if (boot_params().failboot) {
                std::cout << "[Device] failboot=1 — the failure path lives in the"
                             " C++ request; not adopting\n";
                return false;
            }
            // One EM_ASM, bytes into stack buffers through HEAPU8 —
            // boot_params.hpp's pattern, and deliberately NOT
            // stringToNewUTF8/EM_ASM_PTR: those need a runtime helper to
            // be present in the shipped glue, and F5F is this program's
            // standing lesson about betting on an API the payload may not
            // carry. DOUBLE QUOTES in the body: the preprocessor lexes it
            // as pp-tokens first and '' is an empty character constant.
            char state[24] = {};
            char offers[512] = {};
            EM_ASM({
                function put(p, cap, s) {
                    var n = Math.min(s.length, cap - 1);
                    for (var i = 0; i < n; i++) HEAPU8[p + i] = s.charCodeAt(i) & 0x7f;
                    HEAPU8[p + n] = 0;
                }
                var G = (typeof window !== "undefined") ? window.T7_GPU : null;
                put($0, $1, G ? String(G.state) : "absent");
                put($2, $3, G ? String(G.offers) : "");
            }, state, static_cast<int>(sizeof(state)),
               offers, static_cast<int>(sizeof(offers)));
            state[sizeof(state) - 1] = '\0';
            offers[sizeof(offers) - 1] = '\0';

            const std::string st(state);
            if (st != "ready") {
                // NOT A FAILURE. `pending` is the honest common case on a
                // fast wire — the wasm won the race — and `failed` has
                // already printed its own reason from the page.
                std::cout << "[Device] the page has no device to hand over (state="
                          << st << ") — requesting one from C++\n";
                return false;
            }

            wgpu::Device dev = wgpu::Device::Acquire(emscripten_webgpu_get_device());
            if (!dev) {
                std::cerr << "[Device] the page said ready but the import produced"
                             " nothing — requesting one from C++\n";
                return false;
            }
            std::cout << "[Device] ADOPTING the device the page created at parse time"
                         " (core defaults + the wallet, asked before the wasm arrived)\n";

            // THE SAME CENSUS, and it runs BEFORE anything is kept.
            if (!device_meets_floor_(dev)) {
                std::cerr << "[Device] the page's device is BELOW FLOOR — letting it go"
                             " and requesting one from C++\n";
                return false;   // dev's destructor releases the import
            }

            // THE WALLET HALF. `offers` is the page's comma-joined list of
            // the adapter's feature spellings; the commas on both ends make
            // the membership test exact rather than a prefix match.
            const std::string haystack = "," + std::string(offers) + ",";
            std::string missing;
            for (uint32_t i = 0; i < FEATURE_WALLET_GRANTED_COUNT; i++) {
                const std::string needle =
                    "," + std::string(FEATURE_WALLET_GRANTED_NAMES[i]) + ",";
                const bool offered = haystack.find(needle) != std::string::npos;
                if (offered && !dev.HasFeature(FEATURE_WALLET_GRANTED[i])) {
                    if (!missing.empty()) missing += " ";
                    missing += FEATURE_WALLET_GRANTED_NAMES[i];
                }
            }
            if (!missing.empty()) {
                std::cerr << "[Device] the page's device is missing wallet features the"
                             " adapter offered (" << missing << ") — letting it go and"
                             " requesting one from C++\n";
                return false;
            }

            device_ = std::move(dev);
            queue_ = device_.GetQueue();
            {
                wgpu::AdapterInfo info{};
                if (device_.GetAdapterInfo(&info) == wgpu::Status::Success) {
                    announce_adapter_(info);
                } else {
                    std::cout << "[Device] adapter: (the device declined to name its"
                                 " adapter)\n";
                }
            }
            // The offer, as the page saw it. The COUNT is the half that
            // matters and it survives; the ids do not exist to be
            // reported, and the line says why rather than leaving a
            // reader to wonder whether it ran.
            const std::string offerList(offers);
            uint32_t offerCount = offerList.empty() ? 0u : 1u;
            for (char c : offerList) if (c == ',') offerCount++;
            report_device_(
                std::to_string(offerCount)
                    + " (no ids — the adapter handle does not cross the port"
                      " boundary; these are the page's spellings)",
                offerList.empty() ? std::string("-") : offerList,
                "the page, pre-created at parse time");
            bootState_ = BootState::Configuring;
            return true;
        }

        bool initWebGPU() {
            // ── PORT_1b Region 2 (web): the async boot grammar ────
            // emdawnwebgpu (P0-verified): AllowSpontaneous callbacks fire
            // from the browser event loop between rAF turns — no pump
            // needed for the request chain itself. Adapter::GetLimits
            // exists, so the limits request is the full-adapter
            // passthrough exactly as native. Descriptor locals are
            // serialized during the RequestDevice call, so stack
            // lifetime suffices.
            instance_ = wgpu::CreateInstance(nullptr);
            if (!instance_) {
                std::cerr << "Failed to create WebGPU instance\n";
                return false;
            }
            // PORT_4a — arm the anchor (see its banner above). Copy, not
            // move: the member keeps its own reference and the anchor
            // adds a second one that outlives every object in the
            // program. Both are external references by Dawn's counting.
            g_instanceAnchor = instance_;
            // AUBADE U2 — the page may already have done the waiting.
            // Adoption sets device_, queue_ and bootState_ itself and
            // returns true; every refusal falls through to the request
            // chain below with its reason already printed.
            if (adopt_page_device_()) return true;
            bootState_ = BootState::RequestingAdapter;
            // SHIP_0 U2 — ASK FOR THE REAL GPU. Harmless on single-GPU
            // phones (the only adapter is the only answer); correct for a
            // real-time artwork; on dual-GPU Windows modern Chromium
            // honors it. Stack lifetime suffices for the same reason the
            // device descriptor's does, stated in the banner above: the
            // options are serialized during the call, not held.
            wgpu::RequestAdapterOptions adapterOpts{};
            adapterOpts.powerPreference = wgpu::PowerPreference::HighPerformance;
            instance_.RequestAdapter(&adapterOpts, wgpu::CallbackMode::AllowSpontaneous,
                [this](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter,
                       wgpu::StringView message) {
                    if (status != wgpu::RequestAdapterStatus::Success) {
                        std::cerr << "RequestAdapter failed: "
                            << std::string_view(message.data, message.length) << "\n";
                        bootState_ = BootState::Failed;
                        return;
                    }
                    adapter_ = std::move(adapter);
                    // SHIP_0 U2 — WITNESS IDENTITY, web half. Native
                    // enumerated every adapter and logged its pick
                    // (PROBE_1 C1); the web twin could not name its own
                    // silicon, so "the browser runs the HD 5500" was a
                    // presumption and every web METER number was
                    // uninterpretable without it. Now it is a logged fact
                    // or it is overturned.
                    //
                    // Empty fields print "?" rather than nothing: some
                    // builds redact these strings, and a blank field is
                    // indistinguishable from a line that never ran. A
                    // capture reading all "?" is the RESOLVE case — report
                    // it, do not plumb a fallback.
                    {
                        wgpu::AdapterInfo info{};
                        adapter_.GetInfo(&info);
                        announce_adapter_(info);
                    }
                    bootState_ = BootState::RequestingDevice;
                    // PORT_5d — ask modestly first; the helper owns the
                    // descriptor, the limits census and the one retry.
                    request_device_web(/*passthrough=*/false);
                });
            return true;
        }

        bool initSurface() {
            wgpu::SurfaceDescriptor surfaceDesc{};
            // ── PORT_1b Region 3 (web): the canvas surface ────────
            // P0-verified spelling: wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector
            // (dawn.json "emscripten surface source canvas HTML selector",
            // chained into the surface descriptor; member `selector`).
            wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasSource{};
            canvasSource.selector = "#canvas";
            surfaceDesc.nextInChain = &canvasSource;

            surface_ = instance_.CreateSurface(&surfaceDesc);

            // AUBADE U2 — adapter_ IS NULL ON THE ADOPTED PATH, AND THAT
            // IS FINE HERE. This is the program's last surviving read of
            // the member, and the port does not read it: webgpu.cpp's
            // wgpuSurfaceGetCapabilities takes both the surface and the
            // adapter as UNNAMED parameters and answers from
            // navigator.gpu.getPreferredCanvasFormat(). A null wgpu::Adapter
            // passes nullptr, which the implementation ignores exactly as
            // it ignores a live one. If a future port ever reads it, this
            // line is where that lands — and the fix is not to resurrect a
            // second adoption path for the adapter but to take the format
            // from the page, which already knows it.
            wgpu::SurfaceCapabilities caps;
            surface_.GetCapabilities(adapter_, &caps);

            colorFormat_ = caps.formats[0];

            surfaceConfig_.device = device_;
            surfaceConfig_.format = colorFormat_;
            // CAP_1 — THE FIRST CONFIGURE TAKES TARGET, NOT THE BOOT ARGS.
            // currentWidth_/currentHeight_ were seeded from init()'s
            // arguments, which are glfwCreateWindow's arguments and nothing
            // else. Configuring the surface with them put a number nobody
            // measured into the one place the frame is defined. The same
            // expression the resize path uses answers it here, once, from the
            // canvas that actually exists by now — and the canvas backing
            // store is written from it in the same breath, so boot enters the
            // frame loop with the two already agreeing.
            {
                uint32_t tw = 0, th = 0;
                compute_target_size(tw, th);
                if (tw > 0 && th > 0) {
                    currentWidth_  = tw;
                    currentHeight_ = th;
                    write_canvas_backing_store(tw, th);
                }
            }
            surfaceConfig_.width = currentWidth_;
            surfaceConfig_.height = currentHeight_;
            surfaceConfig_.presentMode = wgpu::PresentMode::Fifo;
            surfaceConfig_.alphaMode = wgpu::CompositeAlphaMode::Opaque;
            surface_.Configure(&surfaceConfig_);

            return true;
        }

        // THE FRAME-SIZED ATTACHMENTS (ACQ_0). Every texture whose size is
        // defined as "the frame's size" is created here and nowhere else, so
        // there is exactly one place that can disagree with the acquired
        // texture — and reconcile_frame_attachments is its only caller.
        //
        // The census that fixed this membership: the depth buffer and, at
        // msaa=4, the MSAA color target. The gallery's offscreen depth/color
        // pair does NOT join — those are sized by painting aspect, not by
        // the surface, and they live in state.hpp with their own lifetime.
        //
        // DOMESDAY_2 B10: the depth buffer carries the boot-read sample
        // count, and the msaa color target rides this same recreate path.
        // With msaa=1 every descriptor below is byte-identical to the
        // pre-B10 shape and no msaa color texture exists.
        void createDepthBuffer(uint32_t w, uint32_t h) {
            // Destroy before recreate: the old textures are a frame size
            // that no longer exists, and holding them is how a stale view
            // finds its way into an encoder.
            if (depthTexture_)     depthTexture_.Destroy();
            if (msaaColorTexture_) msaaColorTexture_.Destroy();
            msaaColorTexture_ = nullptr;
            msaaColorView_    = nullptr;
            wgpu::TextureDescriptor depthDesc{};
            depthDesc.label = "Depth Texture";
            depthDesc.size = { w, h, 1 };
            depthDesc.format = depthFormat_;
            depthDesc.sampleCount = effective_msaa();
            depthDesc.usage = wgpu::TextureUsage::RenderAttachment;
            depthTexture_ = device_.CreateTexture(&depthDesc);
            depthView_ = depthTexture_.CreateView();
            if (effective_msaa() == 4u) {
                wgpu::TextureDescriptor msaaDesc{};
                msaaDesc.label = "MSAA Color Target";
                msaaDesc.size = { w, h, 1 };
                msaaDesc.format = surfaceConfig_.format;
                msaaDesc.sampleCount = 4;
                msaaDesc.usage = wgpu::TextureUsage::RenderAttachment;
                msaaColorTexture_ = device_.CreateTexture(&msaaDesc);
                msaaColorView_ = msaaColorTexture_.CreateView();
            }
        }


        // ═══ §3 FRAME LIFECYCLE ══════════════════════════════════
        //
        // Call order each frame:
        //   begin_frame() → [update cartridges] → acquire_surface_texture()
        //   → [encode & submit] → present() → [back to running()]

    private:
        // PORT_3c — clamp the EFFECTIVE device-pixel ratio, web only.
        //
        // Under contrib.glfw3 the GLFW contract holds: window size is CSS
        // pixels, framebuffer size is CSS x devicePixelRatio. Their ratio
        // IS the DPR, so the cap needs no browser API of its own — and if
        // Hi-DPI is not in play the two are equal, the ratio is 1, and
        // this is a no-op. That is the 1x-display acceptance, structurally.
        //
        // Applied BEFORE the change comparison in begin_frame, which is
        // load-bearing: the capped size is what lands in currentWidth_,
        // so the next frame compares capped against capped and the
        // resize branch stays quiet. Capping after the comparison would
        // reconfigure the surface every single frame.
        //
        // The canvas ELEMENT is sized by the library from #frame
        // (FRAME_0, in initGLFW above) — NOT by a CSS rule on the canvas,
        // which is what this comment used to claim and what the element's
        // own inline style always overrode. That CSS size is independent
        // of the backing-store size this caps. Fewer pixels, same layout.
        // ═══ CAP_1 — THE ONE EXPRESSION ══════════════════════════════
        //
        //     target = css × min(devicePixelRatio, PIXEL_CAP)
        //
        // This is the only place that expression exists. It is BOTH the
        // canvas backing-store size AND the size the surface is configured
        // with, so those two can no longer disagree — which is the whole
        // point, because under the new emdawnwebgpu generation the surface
        // reads the canvas and a disagreement is a dropped frame.
        //
        // It replaces PORT_3c's apply_pixel_cap, which inferred the ratio as
        // framebuffer/window because the port had already applied the DPR.
        // The port does not any more (GLFW_SCALE_FRAMEBUFFER is FALSE, see
        // initGLFW), so the ratio would now be a constant 1 and the cap would
        // silently never engage. The DPR is asked for directly instead.
        //
        // No other site may call glfwGetFramebufferSize or read the device
        // pixel ratio to derive a surface or canvas size. They consume target.
        void compute_target_size(uint32_t& tw, uint32_t& th) const {
            int cssW = 0, cssH = 0;
            glfwGetWindowSize(window_, &cssW, &cssH);
            if (cssW <= 0 || cssH <= 0) { tw = 0; th = 0; return; }
            const double dpr = emscripten_get_device_pixel_ratio();
            const double cap = static_cast<double>(effective_pixel_cap());
            const double s   = (dpr > 0.0 && dpr < cap) ? dpr : cap;
            const int w = static_cast<int>(std::floor(cssW * s));
            const int h = static_cast<int>(std::floor(cssH * s));
            tw = static_cast<uint32_t>(w < 1 ? 1 : w);
            th = static_cast<uint32_t>(h < 1 ? 1 : h);
        }

        // The canvas's own answer. Read back rather than remembered, because
        // what this frame needs to know is not what WE last wrote — it is
        // what the backing store says right now, after everyone has written.
        void read_canvas_backing_store(uint32_t& w, uint32_t& h) const {
            w = static_cast<uint32_t>(EM_ASM_INT({
                var c = (typeof Module !== 'undefined' && Module.canvas)
                        ? Module.canvas : document.getElementById('canvas');
                return c ? c.width : 0;
            }));
            h = static_cast<uint32_t>(EM_ASM_INT({
                var c = (typeof Module !== 'undefined' && Module.canvas)
                        ? Module.canvas : document.getElementById('canvas');
                return c ? c.height : 0;
            }));
        }

        // ═══ CAP_2 — THE APP IS THE LAST WRITER, PROVEN AT THE FRAME BOUNDARY ═══
        //
        // CAP_1 stopped the port SCALING (GLFW_SCALE_FRAMEBUFFER=FALSE) but did
        // not stop it WRITING: Window::setCanvasSize still runs
        // emglfw3w_set_size on every resize observation and assigns
        // canvas.width = css. In this emdawnwebgpu generation the canvas wins
        // over Configure at acquire, so on the Pixel the port's 448 beat the
        // app's 672 and the reference device rendered at dpr 1.0 instead of the
        // capped 1.5. Frames stayed valid — ACQ_0's promise held — but the
        // intent was silently defeated downward, which is the quieter failure.
        //
        // The boot log named the mechanism: the port wrote first, acquire built
        // a texture at the port's number, and the app's write landed after the
        // frame that needed it. The fix is NOT to write earlier. Ordering
        // against another writer's event is an assumption, and the campaign has
        // now paid twice for assumptions about who runs when. A frame boundary
        // is a fact: once per frame, before the acquire that reads the canvas,
        // ask the canvas what it says and make it say target if it does not.
        //
        // Cost in steady state is one comparison and no write. A write fires
        // only on drift, and drift only happens when the port wrote — so
        // reassertions count resize EVENTS, not frames. A per-frame storm would
        // mean the port found a writer path that fires every frame, which is a
        // finding and not a tuning problem: the log line exists to surface it.
        void reassert_canvas_target() {
            uint32_t tw = 0, th = 0;
            compute_target_size(tw, th);
            if (tw == 0 || th == 0) return;
            uint32_t cw = 0, ch = 0;
            read_canvas_backing_store(cw, ch);
            if (cw == tw && ch == th) return;   // the common case: nothing to do
            write_canvas_backing_store(tw, th);
            currentWidth_  = tw;
            currentHeight_ = th;
            surfaceConfig_.width  = tw;
            surfaceConfig_.height = th;
            surface_.Configure(&surfaceConfig_);
            // Loud on purpose: SOAK counts these. Expected ≤1 per resize event
            // and 0 in steady state.
            std::cout << "[CAP] reasserted " << tw << "x" << th
                      << " (canvas drifted to " << cw << "x" << ch << ")\n";
        }

        // THE ONE WRITER of the canvas backing store. Idempotence-guarded the
        // same way the port guards its own write, so a settled size costs
        // nothing. Assigning canvas.width also CLEARS the canvas, which is
        // another reason this rides the settle window rather than every frame.
        void write_canvas_backing_store(uint32_t w, uint32_t h) const {
            EM_ASM({
                var c = (typeof Module !== 'undefined' && Module.canvas)
                        ? Module.canvas : document.getElementById('canvas');
                if (!c) return;
                if (c.width  !== $0) c.width  = $0;
                if (c.height !== $1) c.height = $1;
            }, w, h);
        }

        // ═══ FRAME_1 U0 — TEMPORARY INSTRUMENTATION ══════════════════
        //
        // REMOVABLE. This whole method, the two locals that feed it in
        // begin_frame, and its one call site come out together once the
        // phone numbers have named the defect. Nothing depends on it.
        //
        // It prints through out() — Emscripten's Module.print — and NOT
        // console.log, deliberately: index.html's onLine() feeds
        // Module.print into the "details" panel, so these numbers are
        // readable ON THE PHONE by tapping DETAILS. A devtools-only line
        // would be useless on the device that has the defect.
        //
        // Fires only inside the resize branch, so a steady frame loop
        // prints nothing.
        void frame1_report(int fbPreW, int fbPreH, int fbPostW, int fbPostH) const {
            int winW = 0, winH = 0;
            glfwGetWindowSize(window_, &winW, &winH);
            // CAP_1: the fourth number. acquired is what the GPU actually
            // handed us last frame; in steady state it must equal postCap
            // (= target). If those two ever diverge and stay diverged, the
            // single-writer law has been broken somewhere upstream.
            const int acqW = static_cast<int>(acquiredWidth_);
            const int acqH = static_cast<int>(acquiredHeight_);
            EM_ASM({
                var f  = document.getElementById('frame');
                var c  = document.getElementById('canvas');
                var vv = window.visualViewport;
                var r2 = function (n) { return Math.round(n * 100) / 100; };
                // out() is Emscripten's Module.print. Guarded because this
                // block's only job is to produce numbers on a device that
                // cannot be tested from here — if the symbol is ever
                // absent the line must still reach devtools rather than
                // throw and take the frame with it.
                var say = (typeof out === 'function') ? out : console.log;
                say('[FRAME_1]'
                  + ' glfwWin='   + $0 + 'x' + $1
                  + ' fbPreCap='  + $2 + 'x' + $3
                  + ' fbPostCap=' + $4 + 'x' + $5
                  + ' acquired='  + $6 + 'x' + $7
                  + ' inner='     + window.innerWidth + 'x' + window.innerHeight
                  + ' visualVP='  + (vv ? r2(vv.width) + 'x' + r2(vv.height) : 'absent')
                  + ' dpr='       + window.devicePixelRatio
                  + ' appH='      + getComputedStyle(document.documentElement)
                                      .getPropertyValue('--app-h').trim()
                  + ' frameClient=' + (f ? f.clientWidth + 'x' + f.clientHeight : 'ABSENT')
                  + ' canvasCSS='   + (c ? (c.style.width || '(none)') + 'x'
                                          + (c.style.height || '(none)') : 'ABSENT')
                  + ' canvasBuf='   + (c ? c.width + 'x' + c.height : 'ABSENT'));
            }, winW, winH, fbPreW, fbPreH, fbPostW, fbPostH, acqW, acqH);
        }

    public:
        float begin_frame() {
            glfwPollEvents();
            emit_touch_intents();   // SHIP_1 — the frame tick consumes the gestures

            // Handle resize
            int fbWidth = 0, fbHeight = 0;
            // CAP_1: on this twin the wanted size comes from ONE expression
            // and from nowhere else. The framebuffer query below is read for
            // the FRAME_1 witness alone and never feeds a size — with
            // GLFW_SCALE_FRAMEBUFFER FALSE it is CSS px and should now equal
            // glfwWin, and that equality IS the proof that the port stopped
            // scaling behind us.
            int fbPreCapW = 0, fbPreCapH = 0;
            glfwGetFramebufferSize(window_, &fbPreCapW, &fbPreCapH);
            {
                uint32_t tw = 0, th = 0;
                compute_target_size(tw, th);
                fbWidth  = static_cast<int>(tw);
                fbHeight = static_cast<int>(th);
            }
            // DOMESDAY_1 B7 (R4) — THE SETTLE WINDOW. A new size must hold
            // still for RECONFIGURE_SETTLE_FRAMES consecutive frames before
            // the surface is reconfigured; the accepted cost is ≤100 ms of
            // scale softness while a resize animates.
            //
            // ACQ_0 — ITS JURISDICTION IS CONFIGURE INTENT, AND NOTHING ELSE.
            // This branch decides what the app ASKS the surface for: format,
            // present mode, and the size it would like. It may NEVER again
            // gate the recreation of a frame-sized attachment. It used to
            // call createDepthBuffer, and that was the defect — a debounced
            // attachment is by construction a frame or more behind a surface
            // that resizes on its own clock, and the gap between them is a
            // dropped submit. Attachments follow the acquired texture now
            // (acquire_surface_texture), which cannot lag because it IS the
            // frame. Debouncing intent is still worth doing; debouncing truth
            // never was.
            //
            // CAP_2 — AND IN PRACTICE THIS BRANCH NOW RARELY FIRES. The
            // frame-boundary reassertion reconciles canvas and surface to
            // target as soon as either drifts, and it writes currentWidth_/
            // currentHeight_ when it does, so by the time control reaches
            // here the compare below is usually already equal. The settle
            // window survives as the slower path for a size that changes
            // without the canvas drifting; its ≤100 ms of scale softness is
            // no longer what governs a resize. That is a real change to B7's
            // effect and it is stated rather than discovered: reasserting
            // immediately is the price of the app being the last writer, and
            // the port is already reallocating the backing store on every one
            // of those events anyway.
            //
            // The FRAME_1 print stays — it is this unit's acceptance witness.
            if (fbWidth > 0 && fbHeight > 0 &&
                (static_cast<uint32_t>(fbWidth) != currentWidth_ ||
                    static_cast<uint32_t>(fbHeight) != currentHeight_)) {
                if (static_cast<uint32_t>(fbWidth) == pendingWidth_ &&
                    static_cast<uint32_t>(fbHeight) == pendingHeight_) {
                    if (++stableFrames_ >= RECONFIGURE_SETTLE_FRAMES) {
                        currentWidth_ = static_cast<uint32_t>(fbWidth);
                        currentHeight_ = static_cast<uint32_t>(fbHeight);
                        surfaceConfig_.width = currentWidth_;
                        surfaceConfig_.height = currentHeight_;
                        // CAP_1: the canvas and the surface take the SAME
                        // number, in the same breath. The canvas first, so the
                        // surface is configured against a canvas that already
                        // has the shape being asked for.
                        write_canvas_backing_store(currentWidth_, currentHeight_);
                        surface_.Configure(&surfaceConfig_);
                        // No attachment recreation here — see the banner above.
                        stableFrames_ = 0;
                        frame1_report(fbPreCapW, fbPreCapH, fbWidth, fbHeight);   // FRAME_1 — debounce witness; retire after the soak walk confirms single-fire per settle
                    }
                } else {
                    pendingWidth_ = static_cast<uint32_t>(fbWidth);
                    pendingHeight_ = static_cast<uint32_t>(fbHeight);
                    stableFrames_ = 1;
                }
            } else {
                stableFrames_ = 0;
            }

            // Delta time
            auto currentTime = std::chrono::high_resolution_clock::now();
            float measured = std::chrono::duration<float>(currentTime - lastTime_).count();
            lastTime_ = currentTime;

            // PORT_1b: the dt clamp, lifted verbatim from the dormant
            // core/clock.hpp (retired this commit) — "Clamp dt to avoid
            // spiral of death", cap 0.1f (100 ms). Inert at native frame
            // rates; essential across a browser tab-suspend, where rAF
            // hands back a multi-second gap.
            const float raw = std::clamp(measured, 0.0f, 0.1f);

            // THE PRESENTATION LAW (RIBBON_6). A frame is DISPLAYED for a
            // whole number of refreshes, so it must be INTEGRATED for a whole
            // number of refreshes. The steady clock (RIBBON_3) smoothed the
            // callback's arrival, which is the right cure while every frame
            // makes its refresh and the wrong one the moment a frame misses:
            // a 25 ms frame is shown for two refreshes, and integrating 25 ms
            // of world into 33.3 ms of display is a lurch the eye reads as a
            // block.
            //
            // So: estimate the refresh period from the frames that make it;
            // express the measurement as a multiple of it; serve that multiple
            // exactly. Where nothing is ever dropped this is the steady clock,
            // unchanged — every frame is 1x and the served value is the running
            // mean. Where a frame is dropped it serves what the eye saw. A law
            // that is a no-op when it is not needed.
            //
            // THE PERIOD IS PINNED, NOT TRACKED (WRAP_0 U1). Everything above
            // is still true; what was wrong was WHERE the period came from. It
            // was an EWMA over in-band frames plus a relock that adopted `raw`
            // outright — an estimator that follows the FRAME RATE, and there
            // is nothing in it that can tell "the display is slow" from "the
            // world is slow". So it learned the judder and then hid it: on the
            // laptop it printed 16.66 at boot and 25.4-25.7 for the rest of
            // the session, with every frame labelled 1x. On a 60 Hz panel 25.0
            // is exactly the mean of a 16.67/33.33 alternation — the estimator
            // had adopted the average of the stutter as the refresh, and the
            // histogram, which divides by it, could then only ever read 1x.
            //
            // A PRESENT CAN BE LATE BUT NEVER EARLY. You can miss a vblank;
            // you cannot beat one. So the display period is the SMALLEST
            // stable delta, not the mean: the frames that made their refresh
            // are the ones telling the truth about the panel, and the mean is
            // the judder averaged in. P is the 5th percentile of the trailing
            // PRESENT_FIT_WINDOW deltas — a percentile and not the minimum,
            // because one early timer reading must not redefine the display.
            //
            // AND IT MAY ONLY EVER DECREASE, once seeded. A faster display
            // discovered later is real (a 120 Hz panel whose first seconds
            // were slow); a slower one is a world that has fallen behind, and
            // that is precisely what must NOT be mistaken for a refresh.
            //
            // THE LIMITATION, STATED. A device that never fits a single vblank
            // from its very first frame pins to its slowest honest period, and
            // `refresh` prints 33.3 — which is a recognizable number and not a
            // hidden one. Read it as "this machine never once made 60".
            //
            // THE FLOOR IS NOT DECORATION, and it is the one line that is
            // mine. The period is a DIVISOR now, which the mean never was, so
            // a relock onto a zero measurement is not a degradation but a
            // permanent NaN — and raw == 0 is not hypothetical. Under
            // Emscripten this clock is performance.now(), which browsers
            // coarsen on purpose; Firefox with privacy.resistFingerprinting
            // coarsens it to 100 ms, so at 60 Hz most consecutive calls return
            // the SAME value and the difference is exactly zero. Eight of those
            // in a row is the common case there, not a corner: they would
            // relock the period to 0 and every later frame would divide by it.
            // A refresh period cannot be shorter than PRESENT_MIN_PERIOD.
            // The fit's sample set: deltas only, artefacts dropped. A reading
            // under PRESENT_FIT_FLOOR is a coarsened or duplicated timer value,
            // never a display that refreshes faster than 250 Hz.
            if (raw >= PRESENT_FIT_FLOOR) {
                presentFit_[presentFitCursor_] = raw;
                presentFitCursor_ = (presentFitCursor_ + 1u) % PRESENT_FIT_WINDOW;
                if (presentFitCount_ < PRESENT_FIT_WINDOW) presentFitCount_++;
            }
            // Refit on a cadence, not per frame: a percentile over the window
            // is a selection, and the panel's period does not move between
            // frames. Cheap either way — one 300-float copy, ~1 kB, 1 Hz.
            if (presentFitCount_ >= PRESENT_FIT_MIN
                    && ++presentFitTick_ >= PRESENT_FIT_REFIT) {
                presentFitTick_ = 0;
                float scratch[PRESENT_FIT_WINDOW];
                std::memcpy(scratch, presentFit_, presentFitCount_ * sizeof(float));
                const size_t idx = (size_t)(presentFitCount_ * 5u / 100u);
                std::nth_element(scratch, scratch + idx, scratch + presentFitCount_);
                const float p5 = std::max(scratch[idx], PRESENT_MIN_PERIOD);
                if (!presentFitSeeded_) { refreshPeriod_ = p5; presentFitSeeded_ = true; }
                else if (p5 < refreshPeriod_) { refreshPeriod_ = p5; }
            }

            const float k_raw = std::round(raw / refreshPeriod_);
            const float k = std::clamp(k_raw, 1.0f, PRESENT_MAX_MULTIPLE);
            const float model = k * refreshPeriod_;
            float dt;
            if (std::fabs(raw - model) < PRESENT_BAND * refreshPeriod_) {
                // EVERY in-band frame teaches the period, and it teaches
                // raw / k — the refresh this frame implies, not the frame.
                //
                // THE STUCK MULTIPLE, and it is why this is not `if (k == 1)`.
                // Gating the gain on unit frames leaves a k >= 2 frame with NO
                // feedback path at all: it does not move the period, and the
                // in-band arm clears dtStrangers_, so it never relocks either.
                // A display whose frame time lands anywhere in 29.2-37.5 ms —
                // 26.7 to 34.3 Hz, which is exactly where a battery-saver phone
                // capping rAF sits — then reads as a permanent 2x against a
                // 16.67 ms period and is served 33.3 ms for every frame. At
                // 34 Hz that is the world running 13.4% FAST, forever, with a
                // clean meter. raw / k fixes it without costing anything: a
                // GENUINE 30-on-60 has raw / k == the panel period exactly, so
                // the term is zero and the model is untouched; a false multiple
                // converges until served == raw. A no-op when it is not needed,
                // which is the property the whole law is built on.
                // The period is the percentile's; an in-band frame teaches it
                // nothing any more. What the band still decides is what is
                // SERVED — a frame that fits its multiple is integrated for
                // the multiple, which is the presentation law itself.
                dtStrangers_ = 0;
                dt = model;
            } else {
                // Out of band: a hitch, a tab-resume, a stall. Serve its true
                // time. It no longer relocks the period — a slow frame is not
                // evidence about the display, which is the whole of U1.
                dtStrangers_++;
                dt = raw;
            }
            // THE SERVED VALUE CARRIES THE CLAMP TOO. `model` is k x period and
            // can exceed the 100 ms ceiling `raw` was clamped to: at a 57 ms
            // period a 100 ms measurement reads k=2 in-band and would serve
            // 114.3 ms. The CPU's beat clock would then advance t_seconds by
            // 114 ms while dtPending_ clips the GPU's copy to 100 — two
            // integrators handed different amounts of time for one frame, and
            // the cartridge's own comment about "the same 100 ms ceiling"
            // made false. One line, and the ceiling means what it says.
            dt = std::min(dt, 0.1f);

            // PURSE_0 R1 — THE VERDICT LEAVES THE LAW. k is decided here and
            // nowhere else; publishing it is what lets the photographer's
            // headroom rule read the LAW'S answer instead of growing a
            // second opinion about presentation. Written before the meter
            // block below because it is NOT the meter's: the audience build
            // needs it too (core/instruments.hpp carries the standing).
            //
            // The clamped k, not k_raw: a 5x frame and a 4x frame are the
            // same fact to a rule that only asks "was this frame served at
            // unity", and the clamp is the law's own ceiling.
            t7::g_served_k = (k >= 1.0f && k <= (float)PRESENT_MAX_MULTIPLE)
                           ? (uint32_t)k : 1u;

            // THE PRESENT HISTOGRAM (meter builds, 1 Hz). k IS the reading
            // that settles the class: a 2x or 3x column is a DROPPED FRAME —
            // a stutter with a mechanism outside the simulation — and a
            // near-pure 1x column with judder on screen means the cause is
            // upstream in the integrators, not in presentation.
            if constexpr (t7::INSTRUMENTS.frame_meter) {
                // The index is bounded on its own terms, not on the floor's:
                // k is clamped to [1, 4] above, but an index derived from a
                // float must not depend on that clamp being right (RIBBON_5's
                // lesson — the array bound is stated, never derived).
                const uint32_t bucket = (k >= 1.0f && k <= 4.0f) ? (uint32_t)k : 1u;
                presentBuckets_[bucket <= 4u ? bucket - 1u : 3u]++;
                if (dtStrangers_ != 0u || (dt == raw && k != 1.0f)) presentStrangers_++;
                const float d = std::fabs(dt - raw) * 1000.0f;
                presentDeltaSum_ += d;
                if (d > presentDeltaMax_) presentDeltaMax_ = d;
                if (++presentFrames_ >= PRESENT_REPORT_FRAMES) {
                    std::printf("[PRESENT] refresh %.2f ms | 1x %u  2x %u  3x %u  4x+ %u"
                                "  strangers %u | served-raw |d| mean %.1f max %.1f ms\n",
                                (double)(refreshPeriod_ * 1000.0f),
                                presentBuckets_[0], presentBuckets_[1],
                                presentBuckets_[2], presentBuckets_[3],
                                presentStrangers_,
                                (double)(presentDeltaSum_ / (float)presentFrames_),
                                (double)presentDeltaMax_);
                    presentBuckets_[0] = presentBuckets_[1] = 0;
                    presentBuckets_[2] = presentBuckets_[3] = 0;
                    presentStrangers_ = 0; presentFrames_ = 0;
                    presentDeltaSum_ = 0.0f; presentDeltaMax_ = 0.0f;
                }
            }

            // CAP_2 — the last word, at the frame boundary. glfwPollEvents at
            // the head of this function is where the port applies any queued
            // resize, so by here every other writer for this frame has spoken.
            // Nothing between this line and the acquire touches the canvas:
            // the update phase is CPU state and the render phase has not begun.
            reassert_canvas_target();

            // RIBBON_6: the canvas, published for the meter's window line.
            // Written HERE rather than at the four sites that assign
            // currentWidth_/currentHeight_, because by this point in the frame
            // every one of them has spoken and the reassert above is the last
            // word (CAP_2). One store a frame, no branch.
            t7::g_canvas_w = currentWidth_;
            t7::g_canvas_h = currentHeight_;

            return dt;
        }

        // ═══ ACQ_0 — THE ACQUIRED TEXTURE IS THE ONLY WITNESS OF FRAME SIZE ═══
        //
        // The old emdawnwebgpu generation handed back surface textures at the
        // last CONFIGURED size, so a stale depth buffer and a stale swapchain
        // stayed consistent with each other and the debounce was safe. The new
        // generation's surface tracks the CANVAS's actual size at acquire. Two
        // writers, one of them now moving on its own clock: at boot and around
        // every resize there was a window where the depth attachment and the
        // backbuffer disagreed, Dawn invalidated the whole "frame" encoder, and
        // the ENTIRE submit dropped. Not a dropped draw — a dropped frame. The
        // one-shot GPU work that happened to be encoded in those frames (spawn
        // patch generation, the ground atlas, live-card seeding) was lost
        // silently, and patch caching then preserved the loss. That is the flat
        // black spawn region, and it is the whole of it.
        //
        // So validity may never again depend on two writers agreeing. The size
        // is read off the texture we were just handed, every frame, before any
        // encoding — not from the configure intent, not from the framebuffer
        // query, not from a cached value that was true when it was written.
        //
        // This is also the ONLY creation path for the depth buffer now: the
        // boot placeholder is gone, and the first acquire builds it. There is
        // no frame on which an encoder can be created whose depth size was not
        // read from that frame's own acquired texture, because the one consumer
        // of depth_view() runs after this returns true.
        bool acquire_surface_texture() {
            surface_.GetCurrentTexture(&surfaceTexture_);
            if (surfaceTexture_.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal &&
                surfaceTexture_.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal) {
                // Item 5: no acquire, no encoding. The caller returns before it
                // creates the frame encoder, so a failed acquire costs a frame
                // and nothing else.
                return false;
            }
            const uint32_t aw = surfaceTexture_.texture.GetWidth();
            const uint32_t ah = surfaceTexture_.texture.GetHeight();
            if (aw != acquiredWidth_ || ah != acquiredHeight_ || !depthTexture_) {
                acquiredWidth_  = aw;
                acquiredHeight_ = ah;
                createDepthBuffer(aw, ah);   // synchronous, before any encoding
            }
            backbuffer_ = surfaceTexture_.texture.CreateView();
            return true;
        }

        void present() {
            // ── PORT_1b Region 4 (web): no-op — presentation is implicit
            // at rAF return. P0-verified: emdawnwebgpu's wgpuSurfacePresent
            // exists but ABORTS ("wgpuSurfacePresent is unsupported (use
            // requestAnimationFrame via html5.h instead)"), so it must not
            // be called.
        }

        bool running() const {
            return window_ && !glfwWindowShouldClose(window_);
        }


        // ═══ §4 INPUT ════════════════════════════════════════════
        //
        // Producer: inject_* methods, called by GLFW callbacks during
        //           glfwPollEvents(). These push events into the vector.
        //
        // Consumer: input_events() and clear_input_events(), called by
        //           the main loop after begin_frame().

    public:
        // ── Producer (GLFW callbacks → event vector) ─────────────

        void inject_key_event(int key, int action) {
            InputEvent event{};
            event.type = (action == GLFW_PRESS || action == GLFW_REPEAT)
                ? InputEvent::Type::KeyDown
                : InputEvent::Type::KeyUp;
            event.key = key;

            // Convert to character for printable keys
            if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
                event.character = 'A' + (key - GLFW_KEY_A);
            }
            else if (key == GLFW_KEY_SEMICOLON) {
                event.character = ';';
            }
            else if (key == GLFW_KEY_LEFT_BRACKET) {
                event.character = '[';
            }
            else if (key == GLFW_KEY_RIGHT_BRACKET) {
                event.character = ']';
            }
            else {
                event.character = 0;
            }

            inputEvents_.push_back(event);
        }

        // The one differentiator. GLFW reports absolute positions; the
        // tree consumes only deltas, so the previous position is console
        // state — not a static hiding in a callback body.
        void feed_cursor(double x, double y) {
            // SHIP_1 U1 — THE BACKSTOP. claim_touch_stream() removed the
            // port's touch handlers, so nothing should synthesize a
            // cursor from a finger any more. Should is not a guarantee:
            // the port re-registers its listeners whenever it rebuilds
            // them, and ATMOS_0 made a fullscreen transition part of the
            // normal entry. If that ever resurrects the emulation, this
            // turns a silent double-drive — two consumers fighting over
            // one look delta — into nothing at all. The origin is kept
            // current so a real mouse afterwards does not jump.
            if (any_touch_active()) {
                lastCursorX_ = x;
                lastCursorY_ = y;
                return;
            }
            if (!cursorPrimed_) {
                lastCursorX_ = x;
                lastCursorY_ = y;
                cursorPrimed_ = true;
                return;                 // no event: the first sample is an origin
            }
            inject_mouse_move(static_cast<float>(x - lastCursorX_),
                              static_cast<float>(y - lastCursorY_));
            lastCursorX_ = x;
            lastCursorY_ = y;
        }

        // Declares a seam: the next sample re-origins instead of differencing.
        void unprime_cursor() { cursorPrimed_ = false; }

        void inject_mouse_move(float dx, float dy) {
            InputEvent event{};
            event.type = InputEvent::Type::MouseMove;
            event.x = dx;
            event.y = dy;
            inputEvents_.push_back(event);
        }

        void inject_mouse_button(int button, bool pressed) {
            if (any_touch_active()) return;   // the backstop's other half
            InputEvent event{};
            event.type = InputEvent::Type::MouseButton;
            event.button = button;
            event.pressed = pressed;
            inputEvents_.push_back(event);
        }

        void inject_scroll(float delta) {
            InputEvent event{};
            event.type = InputEvent::Type::Scroll;
            event.y = delta;
            inputEvents_.push_back(event);
        }

        // ═══ SHIP_1 U1 — THE TOUCH STREAM, CLAIMED ═══════════════
        //
        // THE PROBLEM, precisely. contrib.glfw3 registers its own
        // touchstart/move/end/cancel handlers on the canvas inside
        // glfwCreateWindow and converts each one to setCursorPos +
        // mouse-button-left — which is why a drag already rotates the
        // camera on a phone today, by accident. The port exposes NO
        // lever to turn that off: not a port option (disableWarning,
        // disableJoystick, disableMultiWindow, disableWebGL2,
        // optimizationLevel — that is the whole list), not a window
        // hint, not a function in emscripten_glfw3.h.
        //
        // WHAT THIS ACTUALLY REMOVES, corrected against the pinned port
        // (FRAME_0 recon). Window.cpp registers exactly ONE touch
        // listener on the canvas — touchstart — while Context.cpp
        // registers touchstart/move/cancel/end on
        // EMSCRIPTEN_EVENT_TARGET_DOCUMENT. Nulling the canvas target
        // therefore removes one handler and leaves four live, so the
        // port's emulation is NOT gone: what actually prevents the
        // double-drive is the any_touch_active() backstop below, plus
        // the ordering. The port's document listeners are BUBBLE phase
        // (Events.h passes useCapture=false), and ours sit on the canvas
        // — the target — so ours run FIRST and the table is populated
        // before the port's synthesis reaches feed_cursor. Jean's
        // ruling to keep the backstop was not a belt; it is the load-
        // bearing half. Nulling the document target as well is a real
        // option and a separate decision, not a silent one.
        //
        // THE LEVER IS HTML5.H'S OWN. In JSEvents.registerOrRemoveHandler
        // a NON-null callback appends a listener and leaves any existing
        // one in place — so simply registering ours would give two live
        // consumers of one finger, which is the failure this whole unit
        // exists to prevent. A NULL callback takes the other branch and
        // removes every handler matching (target, eventType), unbinding
        // with the useCapture each was stored with. So: null first, ours
        // second.
        //
        // The target string is load-bearing. lib_emscripten_glfw3.js
        // does specialHTMLTargets["Module['canvas']"] = Module.canvas at
        // glfwInit, and findEventTarget checks specialHTMLTargets before
        // querySelector — so this literal resolves to the same element
        // the port used, which is the only reason the removal matches.
        void claim_touch_stream() {
            // 1 — the port's handlers, off.
            emscripten_set_touchstart_callback(TOUCH_TARGET, nullptr, true, nullptr);
            emscripten_set_touchmove_callback(TOUCH_TARGET, nullptr, true, nullptr);
            emscripten_set_touchend_callback(TOUCH_TARGET, nullptr, true, nullptr);
            emscripten_set_touchcancel_callback(TOUCH_TARGET, nullptr, true, nullptr);

            // 2 — ours, on. Every handler returns true, which makes
            // html5.h call preventDefault — and THAT is what suppresses
            // the browser's compatibility mouse events. Without it the
            // emulation would come back through the port's MOUSE door
            // after we closed its touch one.
            emscripten_set_touchstart_callback(TOUCH_TARGET, this, true, &Console::touch_cb);
            emscripten_set_touchmove_callback(TOUCH_TARGET, this, true, &Console::touch_cb);
            emscripten_set_touchend_callback(TOUCH_TARGET, this, true, &Console::touch_cb);
            emscripten_set_touchcancel_callback(TOUCH_TARGET, this, true, &Console::touch_cb);

            std::cout << "[Touch] Claimed the canvas touch stream ("
                << TOUCH_TARGET << ")\n";
        }

        bool any_touch_active() const {
            for (const TouchPoint& t : touches_) if (t.active) return true;
            return false;
        }

    private:
        // ── The table ────────────────────────────────────────────
        // Four slots: two per half is every named gesture, and the
        // vocabulary says extras are ignored rather than queued.
        static constexpr int MAX_TRACKED_TOUCHES = 4;

        TouchPoint* find_touch(int id) {
            for (TouchPoint& t : touches_) if (t.active && t.id == id) return &t;
            return nullptr;
        }

        // Primary = earliest born in that half; secondary = next.
        // Birth order, not slot order: a lifted finger frees its slot and
        // the survivors must not be reshuffled by who happens to sit
        // where.
        TouchPoint* half_touch(bool left, int rank) {
            TouchPoint* out = nullptr;
            uint64_t best = UINT64_MAX;
            uint64_t floor_seq = 0;
            for (int r = 0; r <= rank; r++) {
                out = nullptr; best = UINT64_MAX;
                for (TouchPoint& t : touches_) {
                    if (!t.active || t.left != left) continue;
                    if (t.seq < floor_seq) continue;
                    if (t.seq < best) { best = t.seq; out = &t; }
                }
                if (!out) return nullptr;
                floor_seq = best + 1;
            }
            return out;
        }

        int half_count(bool left) const {
            int n = 0;
            for (const TouchPoint& t : touches_) if (t.active && t.left == left) n++;
            return n;
        }

        // THE MIDLINE, in the same CSS pixels the touch reports. Read
        // per event rather than cached: a rotation changes it, and a
        // stale midline would classify a thumb into the wrong half for
        // one gesture — the exact bug the born-in rule exists to avoid.
        float midline_css() const {
            int w = 0, h = 0;
            glfwGetWindowSize(window_, &w, &h);
            (void)h;
            return w > 0 ? static_cast<float>(w) * 0.5f : 0.0f;
        }

        void clear_all_touches() {
            for (TouchPoint& t : touches_) t = TouchPoint{};
            // Every accumulator too: a cancelled gesture must not leave
            // half a look delta behind to arrive on the next tick.
            touchLookDx_ = 0.0f;
            touchLookDy_ = 0.0f;
            touchZoomAccum_ = 0.0f;
            pinchDeclared_ = false;
            rightTapPending_ = false;
            tapAuraPending_ = false;
            tapPossessPending_ = false;
        }

        // ── U4: the right-half pair ──────────────────────────────
        static float separation(const TouchPoint& a, const TouchPoint& b) {
            const float dx = a.x - b.x, dy = a.y - b.y;
            return std::sqrt(dx * dx + dy * dy);
        }

        static bool touch_cb(int eventType, const EmscriptenTouchEvent* e, void* userData) {
            auto* self = static_cast<Console*>(userData);
            if (self && e) self->on_touch(eventType, *e);
            return true;   // preventDefault — see claim_touch_stream
        }

        void on_touch(int eventType, const EmscriptenTouchEvent& e) {
            if (eventType == EMSCRIPTEN_EVENT_TOUCHCANCEL) {
                // U1's rule, flat: cancel clears EVERYTHING. A cancelled
                // gesture has no ending to interpret, so the only honest
                // reading is that no finger is down.
                clear_all_touches();
                return;
            }

            const float mid = midline_css();

            for (int i = 0; i < e.numTouches; i++) {
                const EmscriptenTouchPoint& p = e.touches[i];
                if (!p.isChanged) continue;

                const float px = static_cast<float>(p.targetX);
                const float py = static_cast<float>(p.targetY);

                if (eventType == EMSCRIPTEN_EVENT_TOUCHSTART) {
                    if (find_touch(p.identifier)) continue;      // already tracked
                    TouchPoint* slot = nullptr;
                    for (TouchPoint& t : touches_) if (!t.active) { slot = &t; break; }
                    if (!slot) continue;                          // extras are ignored
                    slot->id      = p.identifier;
                    slot->active  = true;
                    slot->left    = (px < mid);                   // decided ONCE
                    slot->seq     = nextTouchSeq_++;
                    slot->x = slot->x0 = px;
                    slot->y = slot->y0 = py;
                    slot->t0      = e.timestamp;
                    slot->slopped = false;

                    // U4 — THE PAIR FORMS. Undecided on purpose: two
                    // fingers on the right half are a pinch, a possess
                    // tap, or nothing yet, and which one is not knowable
                    // at the moment they land. The pair is born PENDING
                    // and declares itself later, by separating or by
                    // outliving TAP_MS.
                    if (!slot->left && half_count(false) == 2) {
                        TouchPoint* a = half_touch(false, 0);
                        TouchPoint* b = half_touch(false, 1);
                        rightPairT0_   = e.timestamp;
                        rightPairSep_  = (a && b) ? separation(*a, *b) : 0.0f;
                        pinchDeclared_ = false;
                    }
                    continue;
                }

                TouchPoint* t = find_touch(p.identifier);
                if (!t) continue;

                if (eventType == EMSCRIPTEN_EVENT_TOUCHMOVE) {
                    // The step since this finger's own last report. Taken
                    // BEFORE the position is updated, and per-finger — so
                    // a second touch arriving cannot inject a phantom
                    // jump into the first one's delta.
                    const float step_x = px - t->x;
                    const float step_y = py - t->y;
                    t->x = px;
                    t->y = py;

                    // ── U3: LOOK ────────────────────────────────
                    // RIGHT half, ONE touch. The deltas are RAW here and
                    // scaled at consumption (the ledger's sampling law:
                    // accumulate in the callback, spend once on the frame
                    // tick), so a browser that fires three touchmoves in
                    // one frame turns them into one look, not three.
                    //
                    // The one-touch gate is also the pinch suspension:
                    // while two fingers hold the right half this stops
                    // accumulating, and because the delta is measured
                    // from each finger's OWN previous position, the
                    // survivor of a lift resumes with a small step
                    // instead of a jump. No origin to reset by hand.
                    if (!t->left && half_count(false) == 1 && !rightTapPending_) {
                        touchLookDx_ += step_x;
                        touchLookDy_ += step_y;
                    }
                    // The tap window closes on its own: once the pair
                    // has outlived TAP_MS the survivor is just a look
                    // again. Checked on movement because that is the
                    // only moment the answer can matter.
                    if (rightTapPending_
                        && (e.timestamp - rightPairT0_) > TouchControls::TAP_MS) {
                        rightTapPending_ = false;
                    }

                    // ── U4: PINCH ───────────────────────────────
                    // Only the right half, only as a pair. Separation is
                    // read from the pair as a whole rather than from
                    // either finger's motion, so sliding both fingers
                    // across the glass together zooms nothing.
                    if (!t->left && half_count(false) == 2) {
                        TouchPoint* a = half_touch(false, 0);
                        TouchPoint* b = half_touch(false, 1);
                        if (a && b) {
                            const float sep = separation(*a, *b);
                            if (!pinchDeclared_) {
                                // THE DISAMBIGUATOR. A pinch declares
                                // itself two ways — by moving enough to
                                // mean it, or by lasting longer than a
                                // tap could. Until one of them fires the
                                // pair is still a possible tap, and
                                // nothing zooms.
                                if (std::fabs(sep - rightPairSep_) > TouchControls::PINCH_DECLARE
                                    || (e.timestamp - rightPairT0_) > TouchControls::TAP_MS) {
                                    pinchDeclared_ = true;
                                    // NO RE-BASELINE. The travel that
                                    // proved this was a pinch is real
                                    // pinch travel, and it is spent
                                    // below against the separation the
                                    // pair was BORN with. Discarding it
                                    // would be a dead zone rather than a
                                    // classifier: a browser that
                                    // coalesces a whole fast spread into
                                    // one touchmove would declare the
                                    // pinch and zoom nothing, and the
                                    // faster the gesture the more of it
                                    // would vanish.
                                }
                            }
                            if (pinchDeclared_) {
                                const float dsep = sep - rightPairSep_;
                                rightPairSep_ = sep;
                                // SPREAD = IN. zoom_delta adds to camera
                                // distance and closer IS zoomed in, so
                                // growing separation must go negative.
                                touchZoomAccum_ += -dsep * TouchControls::PINCH_SENS;
                            }
                        }
                    }

                    const float ddx = t->x - t->x0, ddy = t->y - t->y0;
                    if (ddx * ddx + ddy * ddy >
                        TouchControls::TAP_SLOP * TouchControls::TAP_SLOP) {
                        t->slopped = true;   // never a tap again
                    }
                    continue;
                }

                if (eventType == EMSCRIPTEN_EVENT_TOUCHEND) {
                    t->x = px;
                    t->y = py;
                    on_touch_lift(*t, e.timestamp);
                    *t = TouchPoint{};
                    continue;
                }
            }
        }

        // ── U5: THE CLEAN TAPS ───────────────────────────────────
        // Both fire on RELEASE, not on press. A press-fired toggle
        // commits before the gesture has said what it is — and the
        // right-half tap in particular shares its opening frames with a
        // pinch, so there is nothing to commit to yet.
        //
        // Called while `t` is still active, so half_count includes it.
        void on_touch_lift(TouchPoint& t, double now_ms) {
            const bool clean = !t.slopped
                && (now_ms - t.t0) <= TouchControls::TAP_MS;

            if (t.left) {
                // AURA — the SECOND left finger. Never the stick: the
                // primary is the stick whether it is dragging or resting,
                // so a tap is only ever a finger that is not it. That is
                // what "the stick is never disturbed by the tap" means
                // mechanically.
                TouchPoint* primary = half_touch(true, 0);
                if (clean && primary != &t) tapAuraPending_ = true;
                return;
            }

            // POSSESS — both of a pair, clean, within one TAP_MS window
            // measured from when the PAIR formed (not from each finger),
            // and only if the pinch never declared itself.
            if (pinchDeclared_) { rightTapPending_ = false; pinchDeclared_ = false; return; }

            const bool in_window = (now_ms - rightPairT0_) <= TouchControls::TAP_MS;
            if (half_count(false) == 2) {
                // First of the pair. Arm, and hold the survivor's look
                // for the rest of the window so a possess cannot nudge
                // the camera on its way out.
                TouchPoint* a = half_touch(false, 0);
                TouchPoint* b = half_touch(false, 1);
                rightTapPending_ = in_window && a && b && !a->slopped && !b->slopped;
            }
            else if (rightTapPending_) {
                // Second of the pair.
                if (in_window && !t.slopped) tapPossessPending_ = true;
                rightTapPending_ = false;
            }
            pinchDeclared_ = false;
        }

        // ── U2: THE STICK ────────────────────────────────────────
        // FLOATING ORIGIN: the stick is born where the thumb lands, so
        // there is no fixed pad to find and no chrome to draw. The
        // vector is the drag from that birthplace.
        //
        // The dead zone RESCALES rather than truncating: past its edge
        // the magnitude starts at 0 and climbs to 1 at STICK_RADIUS. A
        // plain truncation would make the first pixel past the dead zone
        // jump straight to STICK_DEAD_ZONE/STICK_RADIUS of full speed —
        // a lurch exactly where the thumb is trying to be gentle.
        //
        // Screen y grows downward and W is move_z -= 1, so a thumb
        // pushed UP is forward with no sign flip: the drag IS the
        // vector. Camera-relativity is the kernel's
        // (coupling_input_to_pawn_velocity rotates by camera azimuth),
        // and it does not renormalize — so this magnitude survives all
        // the way to the step.
        void stick_vector(float& out_x, float& out_z) {
            out_x = 0.0f; out_z = 0.0f;
            TouchPoint* s = half_touch(true, 0);
            if (!s) return;
            const float dx = s->x - s->x0;
            const float dy = s->y - s->y0;
            const float len = std::sqrt(dx * dx + dy * dy);
            if (len <= TouchControls::STICK_DEAD_ZONE) return;   // exactly zero, not small
            const float span = TouchControls::STICK_RADIUS - TouchControls::STICK_DEAD_ZONE;
            float m = (len - TouchControls::STICK_DEAD_ZONE) / span;
            if (m > 1.0f) m = 1.0f;                              // clamped at STICK_RADIUS
            const float inv = m / len;
            out_x = dx * inv;
            out_z = dy * inv;
        }

        // Called once per frame from begin_frame, before the main loop
        // drains the queue — the ledger's sampling law: a delta
        // accumulated in a browser callback is consumed exactly once,
        // on the frame tick.
        void emit_touch_intents() {
            // MOVE speaks every frame it is held, PLUS exactly one zero
            // on release. A vector that only spoke when it changed would
            // leave the pawn walking after the thumb left the glass.
            const bool stick_live = (half_touch(true, 0) != nullptr);
            if (stick_live || stickWasLive_) {
                InputEvent e{};
                e.type = InputEvent::Type::TouchMove;
                stick_vector(e.x, e.y);
                inputEvents_.push_back(e);
            }
            stickWasLive_ = stick_live;

            // LOOK — spent once, then zeroed. Silence when there is
            // nothing to say: an unchanged camera needs no event.
            if (touchLookDx_ != 0.0f || touchLookDy_ != 0.0f) {
                InputEvent e{};
                e.type = InputEvent::Type::TouchLook;
                // LOOK_SENS_TOUCH applies HERE, at consumption — one
                // multiply per frame instead of one per browser event,
                // and one place to look when the feel is wrong.
                e.x = touchLookDx_ * TouchControls::LOOK_SENS_TOUCH;
                e.y = touchLookDy_ * TouchControls::LOOK_SENS_TOUCH;
                inputEvents_.push_back(e);
                touchLookDx_ = 0.0f;
                touchLookDy_ = 0.0f;
            }

            // ZOOM — the same channel the scroll wheel feeds, already
            // signed and scaled.
            if (touchZoomAccum_ != 0.0f) {
                InputEvent e{};
                e.type = InputEvent::Type::TouchZoom;
                e.y = touchZoomAccum_;
                inputEvents_.push_back(e);
                touchZoomAccum_ = 0.0f;
            }

            // THE TAPS — one event per recognized tap, then the flag is
            // spent. Edge-fired by construction: the flag is only ever
            // set by a release.
            if (tapAuraPending_) {
                InputEvent e{};
                e.type = InputEvent::Type::TouchTapLeft;
                inputEvents_.push_back(e);
                tapAuraPending_ = false;
            }
            if (tapPossessPending_) {
                InputEvent e{};
                e.type = InputEvent::Type::TouchTapRight;
                inputEvents_.push_back(e);
                tapPossessPending_ = false;
            }
        }

    public:

        // ── Consumer (main loop reads then clears) ───────────────

        const std::vector<InputEvent>& input_events() const {
            return inputEvents_;
        }

        void clear_input_events() {
            inputEvents_.clear();
        }

        // ── Cursor grab ──────────────────────────────────────────
        //
        // Two facts, two homes; the GLFW mode is always DERIVED, never
        // set from anywhere else.
        //   grabPolicy_ — this program grabs the pointer. The board's
        //                 boot declares it; the lab never does (ImGui
        //                 needs a live cursor), so the door is inert there.
        //   grabActive_ — the grab is applied right now. KP_* toggles it,
        //                 and the choice persists across focus changes.

        void set_cursor_grab(bool on) {
            grabPolicy_ = on;
            apply_cursor_mode();
        }

        void toggle_cursor_grab() {
            if (!grabPolicy_) return;
            grabActive_ = !grabActive_;
            apply_cursor_mode();
        }

        bool cursor_grabbed() const { return grabPolicy_ && grabActive_; }

    private:
        void apply_cursor_mode() {
            if (!window_) return;
            const bool grab = grabPolicy_ && grabActive_;
            glfwSetInputMode(window_, GLFW_CURSOR,
                grab ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            // Raw motion is only provided while the cursor is disabled, so
            // it travels with the mode and is never set separately. It
            // removes the OS pointer-ballistics curve from the look path.
            if (glfwRawMouseMotionSupported()) {
                glfwSetInputMode(window_, GLFW_RAW_MOUSE_MOTION,
                    grab ? GLFW_TRUE : GLFW_FALSE);
            }
            unprime_cursor();           // the mode change moves the pointer
        }

    public:

        // ═══ §5 ACCESSORS ════════════════════════════════════════

    public:
        wgpu::Device device() const { return device_; }
        wgpu::Queue queue() { return queue_; }
        wgpu::TextureView backbuffer() const { return backbuffer_; }
        wgpu::TextureView depth_view() const { return depthView_; }
        // B10: null when msaa=1 — the main pass reads the null as
        // "render straight into the backbuffer, exactly as before".
        wgpu::TextureView msaa_color_view() const { return msaaColorView_; }
        wgpu::TextureFormat color_format() const { return colorFormat_; }
        wgpu::TextureFormat depth_format() const { return depthFormat_; }

        uint32_t width() const { return currentWidth_; }
        uint32_t height() const { return currentHeight_; }

        float aspect_ratio() const {
            // ACQ_0: the acquired size is the frame's real shape, so the
            // projection follows it rather than the configure intent. Before
            // the first acquire there is nothing to report and the intent is
            // the best available answer.
            const uint32_t w = acquiredWidth_  ? acquiredWidth_  : currentWidth_;
            const uint32_t h = acquiredHeight_ ? acquiredHeight_ : currentHeight_;
            if (h == 0) return 1.0f;
            return static_cast<float>(w) / static_cast<float>(h);
        }

        GLFWwindow* window() const { return window_; }


        // ═══ §6 SHUTDOWN ═════════════════════════════════════════

    public:
        void shutdown() {
            // WIT_2b — the session's verdict, through the one formatting
            // site (core/instruments.hpp), so teardown and the window close
            // cannot drift into two spellings of one witness.
            t7::print_dropped_submits("session total");
            if (window_) {
                glfwDestroyWindow(window_);
                window_ = nullptr;
            }
            glfwTerminate();
        }

        void request_close() {
            if (window_) {
                glfwSetWindowShouldClose(window_, GLFW_TRUE);
            }
        }


        // ═══ §7 STATE ════════════════════════════════════════════

    private:
        // ── Window ───────────────────────────────────────────────
        GLFWwindow* window_ = nullptr;
        // ── SHIP_1 — the claimed touch stream ────────────────────
        TouchPoint touches_[MAX_TRACKED_TOUCHES]{};
        uint64_t   nextTouchSeq_ = 1;   // 0 stays "never born"
        bool       stickWasLive_ = false;   // so the release emits its zero exactly once
        float      touchLookDx_ = 0.0f;     // raw CSS px, accumulated between ticks
        float      touchLookDy_ = 0.0f;
        float      touchZoomAccum_ = 0.0f;  // already signed + scaled by PINCH_SENS
        double     rightPairT0_ = 0.0;      // when the right-half pair formed
        float      rightPairSep_ = 0.0f;    // its separation at the last reading
        bool       pinchDeclared_ = false;  // pinch, or still a possible tap
        bool       rightTapPending_ = false;// one of a clean pair has lifted; waiting on the other
        bool       tapAuraPending_ = false; // edge-fired verbs, spent on the next tick
        bool       tapPossessPending_ = false;
        uint32_t initialWidth_ = 0;
        uint32_t initialHeight_ = 0;
        // CONFIGURE INTENT — what the app asks the surface for (ACQ_0).
        uint32_t currentWidth_ = 0;
        uint32_t currentHeight_ = 0;
        // FRAME TRUTH — the last acquired texture's own size. Written only by
        // acquire_surface_texture, read by the attachments and the aspect.
        uint32_t acquiredWidth_ = 0;
        uint32_t acquiredHeight_ = 0;
        // DOMESDAY_1 B7 (R4) — the reconfigure settle window: a changed
        // framebuffer size must hold still this many consecutive frames
        // before the surface reconfigures (begin_frame). Boot configures
        // immediately (initSurface); this gates the per-frame path only.
        static constexpr uint32_t RECONFIGURE_SETTLE_FRAMES = 6;
        uint32_t pendingWidth_ = 0;
        uint32_t pendingHeight_ = 0;
        uint32_t stableFrames_ = 0;

        // RIBBON_3 — THE STEADY CLOCK's three numbers and its two words.
        // BAND: how far from the mean a measurement may sit and still be
        // called the same cadence (fraction of the mean). GAIN: how fast
        // the mean follows an in-band measurement. RELOCK: how many
        // consecutive out-of-band frames adopt the new cadence outright.
        // RIBBON_6 — THE PRESENTATION LAW's numbers. BAND: how far from a
        // whole multiple of the refresh a measurement may sit and still be
        // called that multiple (fraction of one period). GAIN: how fast the
        // period follows a UNIT frame. MAX_MULTIPLE: the deepest drop the law
        // will model rather than pass through. MIN_PERIOD: the divisor's
        // floor — 1 ms, far above any real display, and the only thing between
        // a coarsened clock and a permanent NaN. RELOCK keeps RIBBON_3's name
        // and value: it is the same idea, one layer down.
        static constexpr float    PRESENT_BAND         = 0.25f;
        // WRAP_0 U1 — THE FIT. The window is 300 presents (~5 s at 60 Hz):
        // long enough that a burst of dropped frames cannot move the 5th
        // percentile, short enough to seed within the first walk. PRESENT_GAIN
        // and STEADY_CLOCK_RELOCK are retired with the estimator they drove.
        static constexpr uint32_t PRESENT_FIT_WINDOW  = 300;
        static constexpr uint32_t PRESENT_FIT_MIN     = 60;    // seed no earlier
        static constexpr uint32_t PRESENT_FIT_REFIT   = 60;    // refit ~1 Hz
        static constexpr float    PRESENT_FIT_FLOOR   = 0.004f;   // s — 250 Hz
        static constexpr float    PRESENT_MAX_MULTIPLE = 4.0f;
        static constexpr float    PRESENT_MIN_PERIOD   = 0.001f;   // s
        static constexpr uint32_t PRESENT_REPORT_FRAMES = 60;
        float    refreshPeriod_ = 1.0f / 60.0f;   // the panel's period; a divisor, so it is named as one
        float    presentFit_[PRESENT_FIT_WINDOW] = {};   // the trailing deltas the percentile is taken over
        uint32_t presentFitCursor_ = 0;
        uint32_t presentFitCount_  = 0;
        uint32_t presentFitTick_   = 0;
        bool     presentFitSeeded_ = false;
        uint32_t dtStrangers_   = 0;
        // The [PRESENT] histogram's counters (meter builds only; the arithmetic
        // is gated at its site, these are four words that cost nothing).
        uint32_t presentBuckets_[4] = {};
        uint32_t presentStrangers_ = 0;
        uint32_t presentFrames_    = 0;
        float    presentDeltaSum_  = 0.0f;
        float    presentDeltaMax_  = 0.0f;

        // ── Gpu Device ───────────────────────────────────────────
        wgpu::Instance instance_;   // portable handle; owns the async request chain
        wgpu::Adapter adapter_;
        wgpu::Device device_;
        wgpu::Queue queue_;

        // ── Boot (PORT_1b) ───────────────────────────────────────
        BootState bootState_ = BootState::RequestingAdapter;
        bool deviceLost_ = false;   // PORT_3a — set by the loss callback, read by the frame gate

        // ── Surface & Presentation ───────────────────────────────
        wgpu::Surface surface_;
        wgpu::SurfaceConfiguration surfaceConfig_{};
        wgpu::TextureFormat colorFormat_;
        wgpu::SurfaceTexture surfaceTexture_;
        wgpu::TextureView backbuffer_;

        // ── Depth ────────────────────────────────────────────────
        wgpu::TextureFormat depthFormat_ = wgpu::TextureFormat::Depth24Plus;
        wgpu::Texture depthTexture_;
        // B10 — the msaa color target (created only when ?msaa=4; the
        // frame resolves into the backbuffer). Rides createDepthBuffer.
        wgpu::Texture msaaColorTexture_;
        wgpu::TextureView msaaColorView_;
        wgpu::TextureView depthView_;

        // ── Timing ───────────────────────────────────────────────
        std::chrono::high_resolution_clock::time_point lastTime_;

        // ── Input ────────────────────────────────────────────────
        std::vector<InputEvent> inputEvents_;

        // ── Cursor ───────────────────────────────────────────────
        bool   grabPolicy_ = false;   // no program has claimed the pointer
        bool   grabActive_ = true;    // if one does, it begins grabbed
        double lastCursorX_ = 0.0;
        double lastCursorY_ = 0.0;
        bool   cursorPrimed_ = false;
    };

} // namespace t7
