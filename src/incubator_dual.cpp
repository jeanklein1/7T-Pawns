/**
 * INCUBATOR DUAL -- Analysis + Render Cartridge Development Harness
 * =================================================================
 *
 * Minimal runtime for developing cartridge pairs.
 * Cartridge selection is controlled from CMakeLists.txt:
 *
 *   set(ACTIVE_RENDER_CARTRIDGE "the_board")
 *   set(ACTIVE_ANALYSIS_CARTRIDGE "polyphony_basic")
 *
 * CMake passes these as compile definitions (INCUBATE_RENDER, INCUBATE_ANALYSIS).
 * No need to edit this file to switch cartridges.
 *
 * CONVENTION:
 *   Analysis cartridges:
 *   | Folder name          | Namespace              | Class  |
 *   |----------------------|------------------------|--------|
 *   | polyphony_basic/     | t7::polyphony_basic    | Canvas |
 *
 *   Render cartridges:
 *   | Folder name          | Namespace              | Class     |
 *   |----------------------|------------------------|-----------|
 *   | the_board/     | t7::the_board    | Cartridge |
 *
 * INPUT ROUTING:
 *   - Music keys (A-Z, etc.) -> Analysis cartridge
 *   - Movement/camera        -> Render cartridge
 */

 // =========================================================================
 // TARGET SELECTION -- Provided by CMake, with fallback for manual override
 // =========================================================================

#ifndef INCUBATE_ANALYSIS
#define INCUBATE_ANALYSIS polyphony_basic
#endif

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

#define ANALYSIS_HEADER(name) STRINGIFY(analysis/name/canvas.hpp)
#define RENDER_HEADER(name)   STRINGIFY(cartridges/name/cartridge.hpp)

// =========================================================================
// INCLUDES
// =========================================================================

#include "console/console.hpp"

// IntelliSense cannot resolve macro-expanded #include paths.
// These literal includes give VS navigation (Peek Definition, Go To, etc.).
// The compiler ignores them -- the macro includes below pull in the same files.
#if defined(__INTELLISENSE__)
#include "analysis/polyphony_basic/canvas.hpp"
#include "cartridges/the_board/cartridge.hpp"
#else
#include ANALYSIS_HEADER(INCUBATE_ANALYSIS)
#include RENDER_HEADER(INCUBATE_RENDER)
#endif

#include <iostream>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <chrono>

// =========================================================================
// FILE WATCHER -- Detects shader file changes for hot reload
// =========================================================================

class FileWatcher {
public:
    void watch(const std::string& path) {
        path_ = path;
        if (std::filesystem::exists(path_)) {
            lastWriteTime_ = std::filesystem::last_write_time(path_);
        }
    }

    bool check() {
        if (path_.empty() || !std::filesystem::exists(path_)) {
            return false;
        }

        auto currentTime = std::filesystem::last_write_time(path_);
        if (currentTime != lastWriteTime_) {
            lastWriteTime_ = currentTime;
            return true;
        }
        return false;
    }

private:
    std::string path_;
    std::filesystem::file_time_type lastWriteTime_;
};

// =========================================================================
// ACTIVE CARTRIDGE TYPES -- Derived from defines
// =========================================================================

namespace analysis_ns = t7::INCUBATE_ANALYSIS;
namespace render_ns = t7::INCUBATE_RENDER;

using AnalysisCartridge = analysis_ns::Canvas;
using RenderCartridge = render_ns::Cartridge;

// Names for display
constexpr const char* ANALYSIS_NAME = STRINGIFY(INCUBATE_ANALYSIS);
constexpr const char* RENDER_NAME = STRINGIFY(INCUBATE_RENDER);

// =========================================================================
// INPUT ROUTING -- Music keys go to analysis, rest to render
// =========================================================================

static bool is_music_key(int key) {
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) return true;
    if (key == GLFW_KEY_SEMICOLON) return true;
    if (key == GLFW_KEY_LEFT_BRACKET) return true;
    if (key == GLFW_KEY_RIGHT_BRACKET) return true;
    return false;
}

// =========================================================================
// MAIN
// =========================================================================

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  INCUBATOR DUAL (Hot Reload Enabled)\n";
    std::cout << "  Analysis: " << ANALYSIS_NAME << "\n";
    std::cout << "  Render:   " << RENDER_NAME << "\n";
    std::cout << "========================================\n";
    std::cout << "\n";

    // --- Initialize Console -------------------------------------------------
    t7::Console console;
    if (!console.init("Incubator Dual", 1280, 720)) {
        std::cerr << "Failed to initialize console\n";
        return 1;
    }

    // --- Initialize Analysis Cartridge --------------------------------------
    AnalysisCartridge analysis;
    analysis.initialize("assets");

    // Optional: load MIDI from command line
    // Note: Comment out if your analysis cartridge doesn't support load_midi()
    if (argc > 1) {
        if (analysis.load_midi(argv[1])) {
            std::cout << "[Incubator] Loaded MIDI: " << argv[1] << "\n";
        }
    }

    std::cout << "[Incubator] " << ANALYSIS_NAME << " analysis ready\n";

    // --- Initialize Render Cartridge ----------------------------------------
    RenderCartridge render;
    render.initialize(console.device());

    if (!render.init_renderer(console.color_format(), console.depth_format())) {
        std::cerr << "Failed to initialize " << RENDER_NAME << " renderer\n";
        return 1;
    }

    std::cout << "[Incubator] " << RENDER_NAME << " renderer ready\n";

    // --- Setup File Watcher -------------------------------------------------
    FileWatcher watcher;
    watcher.watch(render.shader_path());
    std::cout << "[Incubator] Hot reload enabled: " << render.shader_path() << "\n\n";
    std::cout << "Controls: Arrows=move, Mouse=camera, A-Z=piano keys\n\n";

    int reload_frame_counter = 0;
    wgpu::Queue queue = console.queue();

    // --- Main Loop ----------------------------------------------------------
    while (console.running()) {
        float dt = console.begin_frame();

        // --- Hot Reload Check (every ~30 frames) ----------------------------
        if (++reload_frame_counter >= 30) {
            reload_frame_counter = 0;
            std::cout << "." << std::flush;  // Print dot every check
            if (watcher.check()) {
                std::cout << "\n[FileWatcher] Change detected!\n";
                render.reload_shaders();
            }
        }

        // --- Input Routing --------------------------------------------------
        for (const auto& event : console.input_events()) {
            if (event.type == t7::InputEvent::Type::KeyDown ||
                event.type == t7::InputEvent::Type::KeyUp) {

                if (is_music_key(event.key)) {
                    analysis.on_input(event);
                }
                else {
                    render.on_input(event);
                }
            }
            else {
                render.on_input(event);
            }
        }
        console.clear_input_events();

        // --- Update ---------------------------------------------------------
        analysis.update(dt);

        // Debug: print abbott/costello polyphony every 0.5s
        static float debug_accum = 0.0f;
        debug_accum += dt;
        if (debug_accum >= 0.5f) {
            debug_accum = 0.0f;
            float a = analysis.output().stat(0, analysis_ns::STAT_POLYPHONY_ABBOTT);
            float c = analysis.output().stat(1, analysis_ns::STAT_POLYPHONY_COSTELLO);
            std::cout << "\n[mc] abbott=" << a
                      << " costello=" << c << "\n";
        }

        render.update(analysis.output(), console.aspect_ratio(), queue);

        // --- Render ---------------------------------------------------------
        if (!console.acquire_surface_texture()) {
            continue;
        }

        wgpu::CommandEncoderDescriptor encDesc{};
        wgpu::CommandEncoder encoder = console.device().CreateCommandEncoder(&encDesc);

        render.render(encoder, console.backbuffer(), console.depth_view());

        wgpu::CommandBufferDescriptor cmdDesc{};
        wgpu::CommandBuffer commands = encoder.Finish(&cmdDesc);
        queue.Submit(1, &commands);

        console.present();
    }

    std::cout << "[Incubator] Shutdown\n";
    return 0;
}