/**
 * THE LAB -- Coupling Laboratory
 * ==============================
 *
 * Fast-iteration sandbox for designing musical couplings. Pairs the analysis
 * cartridge of choice with a tiny ImGui+ImPlot dashboard so we can see a
 * parameter move (raw stat, normalized target, and trajectory output) in
 * sub-second iteration loops.
 *
 * No render cartridge. No project WGSL pipelines. FXC compile time is
 * trivial -- only ImGui's internal render pipeline gets compiled.
 *
 * Selection of the analysis cartridge is controlled from CMakeLists.txt
 * via ACTIVE_ANALYSIS_CARTRIDGE (passed as LAB_ANALYSIS).
 *
 * Build:  cmake --build . --target the_lab
 * Run:    ./the_lab.exe [optional_midi_file]
 *
 * CONVENTION
 *   | Folder name        | Namespace            | Class  |
 *   |--------------------|----------------------|--------|
 *   | canvas_1/          | t7::canvas_1         | Canvas |
 */

// =========================================================================
// TARGET SELECTION -- Provided by CMake, with fallback for manual override
// =========================================================================

#ifndef LAB_ANALYSIS
#define LAB_ANALYSIS canvas_1
#endif

#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

#define ANALYSIS_HEADER(name) STRINGIFY(analysis/name/canvas.hpp)

// =========================================================================
// INCLUDES
// =========================================================================

#include "console/console.hpp"
#include "musical/trajectory.hpp"

// IntelliSense cannot resolve macro-expanded #include paths.
// Literal include gives navigation; the macro include below pulls the same file.
#if defined(__INTELLISENSE__)
#include "analysis/canvas_1/canvas.hpp"
#else
#include ANALYSIS_HEADER(LAB_ANALYSIS)
#endif

#include "imgui.h"
#include "imgui_internal.h"   // DockBuilder API (docking branch)
#include "imgui_impl_glfw.h"
#include "imgui_impl_wgpu.h"
#include "implot.h"
#include <GLFW/glfw3.h>       // glfwMaximizeWindow (Console owns the GLFW window)

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <vector>

// =========================================================================
// ACTIVE CARTRIDGE TYPE -- Derived from define
// =========================================================================

namespace analysis_ns = t7::LAB_ANALYSIS;
using AnalysisCartridge = analysis_ns::Canvas;

constexpr const char* ANALYSIS_NAME = STRINGIFY(LAB_ANALYSIS);

// canvas_1 publishes its stat layout at runtime via stat_layout(), not as a
// compile-time static array. Capture a copy once (after initialize()) and read
// it through g_layout[] / layout_count() wherever the old static STAT_LAYOUT was.
static std::vector<t7::StatGroup> g_layout;
static int layout_count() { return static_cast<int>(g_layout.size()); }

// =========================================================================
// SCROLLING BUFFER -- Ring buffer for ImPlot time series
// =========================================================================

struct ScrollingBuffer {
    int max_size;
    int offset;
    std::vector<float> xs;
    std::vector<float> ys;

    explicit ScrollingBuffer(int max = 4000) : max_size(max), offset(0) {
        xs.reserve(max);
        ys.reserve(max);
    }

    void push(float x, float y) {
        if ((int)xs.size() < max_size) {
            xs.push_back(x);
            ys.push_back(y);
        } else {
            xs[offset] = x;
            ys[offset] = y;
            offset = (offset + 1) % max_size;
        }
    }
};

// =========================================================================
// VECTOR HISTORY -- rolling per-bin time series for the line scopes
// =========================================================================
//
// A rolling window of the last `max_cols` frames, each a column of `bins`
// values, with the wall-clock time of each column. Read back per bin as a
// (time, value) series, oldest->newest, for an ImPlot line plot -- value on
// the axis, time on the axis; no color encoding.

struct VectorHistory {
    int bins;
    int max_cols;
    int head = 0;
    int filled = 0;
    std::vector<float> ring;   // max_cols columns, each `bins` long (column-major)
    std::vector<float> times;  // t_seconds of each column

    VectorHistory(int n_bins, int cols = 600)
        : bins(n_bins), max_cols(cols), ring(n_bins * cols, 0.0f), times(cols, 0.0f) {}

    void push(const float* col, float t) {
        for (int b = 0; b < bins; ++b) ring[head * bins + b] = col[b];
        times[head] = t;
        head = (head + 1) % max_cols;
        if (filled < max_cols) ++filled;
    }

    // One bin's series, oldest->newest: the time axis and the values.
    void series(int bin, std::vector<float>& ts, std::vector<float>& ys) const {
        ts.clear(); ys.clear();
        const int oldest = (head - filled + max_cols) % max_cols;
        for (int t = 0; t < filled; ++t) {
            const int src = (oldest + t) % max_cols;
            ts.push_back(times[src]);
            ys.push_back(ring[src * bins + bin]);
        }
    }
};

// =========================================================================
// TEST COUPLING -- One coupling under design; intensity idiom
// =========================================================================
//
// Reads one stat from the AnalysisSignal, normalizes by FULL, ramps a
// Trajectory toward the normalized target with asymmetric attack/release.
// This is the simplest coupling shape (the most common one in musical.inl);
// future iterations of the lab can host rate and event idioms alongside.

struct TestCoupling {
    bool  enabled        = true;
    int   source_group   = 0;     // index into g_layout (group carries its channel)
    int   source_index   = 0;     // within-group slot, 0 .. count-1
    float attack         = 4.0f;  // 1/s
    float release        = 2.5f;  // 1/s
    float full           = 6.0f;  // input value that saturates target at 1.0

    t7::Trajectory trajectory{};

    ScrollingBuffer input_history;   // raw stat over time
    ScrollingBuffer target_history;  // normalized target over time
    ScrollingBuffer output_history;  // trajectory.value over time

    void tick(const t7::AnalysisSignal& signal) {
        // Source addressing derives from the named group: group picks the
        // channel + slot_base, source_index selects within a multi-wide group.
        const t7::StatGroup& g = g_layout[source_group];
        const float input = signal.stat(g.channel, g.slot_base + source_index);
        const float target = enabled ? std::min(input / full, 1.0f) : 0.0f;
        const float rate = (target > trajectory.value) ? attack : release;
        trajectory = t7::trajectory_release(trajectory, target, signal.dt, rate);

        input_history.push(signal.t_seconds, input);
        target_history.push(signal.t_seconds, target);
        output_history.push(signal.t_seconds, trajectory.value);
    }
};

// =========================================================================
// DASHBOARD -- The whole window per frame
// =========================================================================

static void draw_stats_grid(const t7::AnalysisSignal& signal) {
    ImGui::SeparatorText("AnalysisSignal  (slots 0-31 of 128 per channel)");

    if (ImGui::BeginTable("stats", 33,
            ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableSetupColumn("ch");
        for (int s = 0; s < 32; ++s) {
            char header[8];
            std::snprintf(header, sizeof(header), "%d", s);
            ImGui::TableSetupColumn(header);
        }
        ImGui::TableHeadersRow();

        for (int c = 0; c < t7::MAX_CHANNELS; ++c) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("ch%d", c);
            for (int s = 0; s < 32; ++s) {
                ImGui::TableSetColumnIndex(s + 1);
                const float v = signal.stat(c, s);
                if (std::abs(v) > 0.001f) {
                    ImGui::Text("%.2f", v);
                } else {
                    ImGui::TextDisabled(" - ");
                }
            }
        }
        ImGui::EndTable();
    }
}

static void draw_coupling_controls(TestCoupling& tc) {
    ImGui::SeparatorText("Test Coupling  (intensity idiom)");

    // Resolve the selected group up front (also drives the source readout).
    const t7::StatGroup& g0 = g_layout[tc.source_group];

    ImGui::Checkbox("Enabled", &tc.enabled);
    ImGui::SameLine();
    ImGui::Text("  source: %s  -> stat(ch=%d, slot=%d)",
                g0.name, g0.channel, g0.slot_base + tc.source_index);

    // Name-scoped source picker: pick a StatGroup by name, then (for
    // multi-wide groups) an index within it. Same named source a render
    // coupling targets — e.g. "abbott.lowest_pc" + index k is PC-k's one-hot.
    if (ImGui::BeginCombo("Source", g0.name)) {
        for (int i = 0; i < layout_count(); ++i) {
            const bool sel = (i == tc.source_group);
            if (ImGui::Selectable(g_layout[i].name, sel)) {
                tc.source_group = i;
                tc.source_index = 0;   // reset index when group changes
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    const t7::StatGroup& g = g_layout[tc.source_group];
    if (g.count > 1) {
        ImGui::SliderInt("Index", &tc.source_index, 0, g.count - 1);
    }

    ImGui::SliderFloat("Attack rate (1/s)",   &tc.attack,  0.1f, 20.0f, "%.2f");
    ImGui::SliderFloat("Release rate (1/s)",  &tc.release, 0.1f, 20.0f, "%.2f");
    ImGui::SliderFloat("Normalization (FULL)", &tc.full,   1.0f, 16.0f, "%.2f");
}

static void draw_input_scope(const TestCoupling& tc, float now) {
    ImGui::SeparatorText("Input scope  (raw stat over time)");

    if (ImPlot::BeginPlot("##input_scope", ImVec2(-1, 200))) {
        ImPlot::SetupAxes("t (s)", "value", ImPlotAxisFlags_None, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxisLimits(ImAxis_X1, now - 10.0, now, ImPlotCond_Always);

        if (!tc.input_history.xs.empty()) {
            ImPlotSpec spec;
            spec.Offset = tc.input_history.offset;
            ImPlot::PlotLine("input",
                tc.input_history.xs.data(), tc.input_history.ys.data(),
                (int)tc.input_history.xs.size(), spec);
        }
        ImPlot::EndPlot();
    }
}

static void draw_trajectory_scope(const TestCoupling& tc, float now) {
    ImGui::SeparatorText("Trajectory scope  (target vs output, both in [0,1])");

    if (ImPlot::BeginPlot("##trajectory_scope", ImVec2(-1, 240))) {
        ImPlot::SetupAxes("t (s)", "value");
        ImPlot::SetupAxisLimits(ImAxis_X1, now - 10.0, now, ImPlotCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -0.1, 1.2);

        if (!tc.target_history.xs.empty()) {
            ImPlotSpec spec;
            spec.Offset = tc.target_history.offset;
            ImPlot::PlotLine("target (normalized)",
                tc.target_history.xs.data(), tc.target_history.ys.data(),
                (int)tc.target_history.xs.size(), spec);
        }
        if (!tc.output_history.xs.empty()) {
            ImPlotSpec spec;
            spec.Offset = tc.output_history.offset;
            ImPlot::PlotLine("trajectory.value",
                tc.output_history.xs.data(), tc.output_history.ys.data(),
                (int)tc.output_history.xs.size(), spec);
        }
        ImPlot::EndPlot();
    }
}

// ─── Stat panels (descriptor-driven) ──────────────────────────────────────
// Renders each StatGroup in its declared shape: scalars as scrolling lines,
// vectors as bar charts. Iterates the published layout (g_layout), so no slot
// numbers are hardcoded here — new stats appear by publishing them.

static void draw_scalar(const t7::StatGroup& g, const ScrollingBuffer& buf,
                        const t7::AnalysisSignal& signal) {
    const float now = signal.t_seconds;
    ImGui::Text("%.3f", signal.stat(g.channel, g.slot_base));

    char id[64];
    std::snprintf(id, sizeof(id), "##%s", g.name);
    if (ImPlot::BeginPlot(id, ImVec2(-1, 110))) {
        ImPlot::SetupAxes("t (s)", "value",
                          ImPlotAxisFlags_None, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxisLimits(ImAxis_X1, now - 10.0, now, ImPlotCond_Always);
        if (!buf.xs.empty()) {
            ImPlotSpec spec;
            spec.Offset = buf.offset;
            ImPlot::PlotLine(g.name, buf.xs.data(), buf.ys.data(),
                             (int)buf.xs.size(), spec);
        }
        ImPlot::EndPlot();
    }
}

static void draw_vector(const t7::StatGroup& g, const t7::AnalysisSignal& signal) {
    float vals[128];
    for (int i = 0; i < g.count; ++i)
        vals[i] = signal.stat(g.channel, g.slot_base + i);

    char id[64];
    std::snprintf(id, sizeof(id), "##%s", g.name);
    if (ImPlot::BeginPlot(id, ImVec2(-1, 150))) {
        ImPlot::SetupAxes("slot", "value",
                          ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::PlotBars(g.name, vals, g.count);
        ImPlot::EndPlot();
    }
}

// History as time-scaled line scopes: each active bin (its degree above D) is a
// line, value on the Y axis, real time (seconds) on X over the last 10s -- the
// same idiom as the scalar scopes. The numbers are read off the axes; no
// color-encoded heatmap. Bins that never sounded are skipped, to declutter.
static void draw_vector_history(const t7::StatGroup& g, const VectorHistory& hist,
                                const t7::AnalysisSignal& signal) {
    const float now = signal.t_seconds;
    char id[64];
    std::snprintf(id, sizeof(id), "##%s_hist", g.name);
    if (ImPlot::BeginPlot(id, ImVec2(-1, 150))) {
        ImPlot::SetupAxes("t (s)", "value",
                          ImPlotAxisFlags_None, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxisLimits(ImAxis_X1, now - 10.0, now, ImPlotCond_Always);
        static std::vector<float> ts, ys;   // reused scratch; single-threaded UI
        for (int b = 0; b < g.count; ++b) {
            hist.series(b, ts, ys);
            if (ts.empty()) continue;
            bool any = false;
            for (float y : ys) if (y != 0.0f) { any = true; break; }
            if (!any) continue;              // skip pitch classes that never sounded
            char lbl[16];
            std::snprintf(lbl, sizeof(lbl), "%d", b);   // degree above D
            ImPlotSpec spec;
            ImPlot::PlotLine(lbl, ts.data(), ys.data(), (int)ts.size(), spec);
        }
        ImPlot::EndPlot();
    }
}

struct StatScopes {
    std::vector<ScrollingBuffer> scalar_history;
    std::vector<VectorHistory>   vector_history;

    // Stats clustered per channel. Each channel becomes its own dockable
    // window inside the Analysis nested dockspace. Histories stay keyed to
    // declared order via hist_index, independent of how channels are docked.
    struct ChannelGroup {
        int  channel;
        char label[32];          // "abbott" etc., parsed from the name prefix
        std::vector<int> stats;  // g_layout indices, declared order
    };
    std::vector<ChannelGroup> channel_groups;
    std::vector<int> hist_index;  // group -> buffer slot (sized at construction)

    StatScopes() {
        hist_index.resize(layout_count());
        for (int g = 0; g < layout_count(); ++g) {
            const auto& grp = g_layout[g];

            // history buffers, in declared order; remember each group's slot
            if (grp.shape == t7::StatShape::Scalar) {
                hist_index[g] = (int)scalar_history.size();
                scalar_history.emplace_back();
            } else {
                hist_index[g] = (int)vector_history.size();
                vector_history.emplace_back(grp.count);
            }

            // find/create the channel group, append this stat
            ChannelGroup* cg = nullptr;
            for (auto& c : channel_groups)
                if (c.channel == grp.channel) { cg = &c; break; }
            if (!cg) {
                channel_groups.push_back(ChannelGroup{});
                cg = &channel_groups.back();
                cg->channel = grp.channel;
                // label = name up to '.' (manual parse, no <cstring>)
                int k = 0;
                while (grp.name[k] && grp.name[k] != '.' && k < 31) {
                    cg->label[k] = grp.name[k]; ++k;
                }
                cg->label[k] = '\0';
            }
            cg->stats.push_back(g);
        }
    }

    void tick(const t7::AnalysisSignal& signal) {
        for (int g = 0; g < layout_count(); ++g) {
            const auto& grp = g_layout[g];
            if (grp.shape == t7::StatShape::Scalar) {
                scalar_history[hist_index[g]].push(
                    signal.t_seconds, signal.stat(grp.channel, grp.slot_base));
            } else {
                float col[128];
                for (int i = 0; i < grp.count; ++i)
                    col[i] = signal.stat(grp.channel, grp.slot_base + i);
                vector_history[hist_index[g]].push(col, signal.t_seconds);
            }
        }
    }

    void draw(const t7::AnalysisSignal& signal) {
        // (1) Analysis host: a nested dockspace the channel windows dock into.
        ImGui::Begin("Analysis", nullptr, ImGuiWindowFlags_NoCollapse);
        ImGuiID analysis_dock = ImGui::GetID("AnalysisDock");

        static bool inner_inited = false;
        if (!inner_inited) {
            inner_inited = true;
            // Built BEFORE DockSpace() below, and GetID() doesn't create a node,
            // so on a fresh launch this is genuinely null and the default takes;
            // imgui.ini wins on later runs. (Self-correcting if it ever doesn't:
            // docking a channel into Analysis once persists thereafter.)
            if (ImGui::DockBuilderGetNode(analysis_dock) == nullptr) {
                ImGui::DockBuilderRemoveNode(analysis_dock);
                ImGui::DockBuilderAddNode(analysis_dock, ImGuiDockNodeFlags_DockSpace);
                // No DockBuilderSetNodeSize here: this node is never split (all
                // channels tab into it), so it needs no pre-size — and on the
                // first frame GetContentRegionAvail() is still 0, which would
                // trip DockBuilderSetNodeSize's size>0 assert. DockSpace() below
                // sizes the node (clamping to a sane minimum).
                for (auto& cg : channel_groups)
                    ImGui::DockBuilderDockWindow(cg.label, analysis_dock);  // tab here by default
                ImGui::DockBuilderFinish(analysis_dock);
            }
        }
        ImGui::DockSpace(analysis_dock);
        ImGui::End();

        // (2) Channel sub-windows — dock into analysis_dock; drag/tab/split freely.
        for (auto& cg : channel_groups) {
            ImGui::Begin(cg.label);
            for (int g : cg.stats) {
                const auto& grp = g_layout[g];

                // stat name without the channel prefix
                const char* dot = grp.name;
                while (*dot && *dot != '.') ++dot;
                const char* short_name = (*dot == '.') ? dot + 1 : grp.name;

                if (ImGui::CollapsingHeader(short_name)) {  // collapsed by default
                    if (grp.shape == t7::StatShape::Scalar) {
                        draw_scalar(grp, scalar_history[hist_index[g]], signal);
                    } else {
                        ImGui::PushID(g);  // scope the inner "history" header id per stat
                        draw_vector(grp, signal);
                        // Tint the strip's "history" header a deeper blue so it
                        // reads as a sub-section, distinct from the stat headers.
                        ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.10f, 0.22f, 0.45f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.15f, 0.32f, 0.62f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.20f, 0.40f, 0.78f, 1.0f));
                        const bool show_hist = ImGui::CollapsingHeader("history");
                        ImGui::PopStyleColor(3);
                        if (show_hist)  // the line scope toggles on its own
                            draw_vector_history(grp, vector_history[hist_index[g]], signal);
                        ImGui::PopID();
                    }
                }
            }
            ImGui::End();
        }
    }
};

static void draw_coupling_window(const t7::AnalysisSignal& signal,
                                 TestCoupling& tc, AnalysisCartridge& analysis) {
    // Layout is owned by the dockspace; no manual pos/size here.
    ImGui::Begin("Coupling Lab", nullptr, ImGuiWindowFlags_NoCollapse);

    ImGui::Text("t = %.2f s   beat = %.2f   dt = %.4f s   |   analysis: %s",
                signal.t_seconds, signal.t_beats, signal.dt, ANALYSIS_NAME);
    // canvas_1 reads the DAW transport; it has no keyboard-debug source toggle.
    (void)analysis;
    ImGui::Separator();

    draw_coupling_controls(tc);
    draw_input_scope(tc, signal.t_seconds);
    draw_trajectory_scope(tc, signal.t_seconds);

    if (ImGui::CollapsingHeader("raw slot grid (channels x slots 0-15)"))
        draw_stats_grid(signal);
    ImGui::End();
}

// =========================================================================
// MAIN
// =========================================================================

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  THE LAB -- Coupling Laboratory\n";
    std::cout << "  Analysis: " << ANALYSIS_NAME << "\n";
    std::cout << "========================================\n\n";

    // --- Console (window, GPU device, surface, input) -----------------------
    t7::Console console;
    if (!console.init("The Lab", 1400, 900)) {
        std::cerr << "Failed to initialize console\n";
        return 1;
    }

    // Fit the screen on launch: the hardcoded 1400x900 can overflow a smaller
    // monitor (the bottom becoming unreachable). Maximize to the work area;
    // the dockspace below then lays the panels out to fill it.
    glfwMaximizeWindow(console.window());

    // --- Analysis cartridge -------------------------------------------------
    AnalysisCartridge analysis;
    analysis.initialize("assets");

    // canvas_1 reads the DAW transport (loopMIDI), not a MIDI file.
    (void)argc; (void)argv;

    // Capture the published stat layout once, for the descriptor-driven panels.
    {
        t7::StatLayoutView lay = analysis.stat_layout();
        g_layout.assign(lay.groups, lay.groups + lay.count);
    }

    std::cout << "[Lab] " << ANALYSIS_NAME << " analysis ready\n";

    // --- ImGui + ImPlot -----------------------------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // dock within main window
    ImGui::StyleColorsDark();

    // Bind ImGui to Console's GLFW window
    ImGui_ImplGlfw_InitForOther(console.window(), /*install_callbacks=*/true);

    // Bind ImGui to Console's WebGPU device
    ImGui_ImplWGPU_InitInfo wgpu_init{};
    wgpu_init.Device              = console.device().Get();
    wgpu_init.NumFramesInFlight   = 3;
    wgpu_init.RenderTargetFormat  = static_cast<WGPUTextureFormat>(console.color_format());
    wgpu_init.DepthStencilFormat  = WGPUTextureFormat_Undefined;
    ImGui_ImplWGPU_Init(&wgpu_init);

    std::cout << "[Lab] ImGui + WebGPU backend ready\n\n";

    // --- Lab state ----------------------------------------------------------
    TestCoupling test_coupling;
    StatScopes stat_scopes;
    wgpu::Queue queue = console.queue();

    // --- Main loop ----------------------------------------------------------
    while (console.running()) {
        const float dt = console.begin_frame();

        // Music keys must reach the analyzer even when a plot/window has focus.
        // WantCaptureKeyboard is true whenever the mouse is over any widget, so
        // gating on it swallowed keys unless you clicked empty space. Only a
        // live text field should suppress music keys, so gate on WantTextInput
        // (the lab has no text fields yet, so keys always flow for now).
        for (const auto& event : console.input_events()) {
            if (event.type == t7::InputEvent::Type::KeyDown ||
                event.type == t7::InputEvent::Type::KeyUp) {
                if (!io.WantTextInput) {
                    analysis.on_input(event);
                }
            }
        }
        console.clear_input_events();

        // Analysis tick + coupling tick
        analysis.update(dt);
        test_coupling.tick(analysis.output());
        stat_scopes.tick(analysis.output());

        // ImGui frame
        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Main dock space over the viewport: Analysis (audio) | Coupling Lab
        // (visual). First launch builds the default split; imgui.ini wins after.
        ImGuiID main_dock = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
        static bool main_inited = false;
        if (!main_inited) {
            // DockSpaceOverViewport() submits DockSpace(), which creates the node
            // immediately, so a fresh launch yields an EMPTY (not null) node. Guard
            // on null-OR-empty, else the default never builds and the windows float.
            ImGuiDockNode* node = ImGui::DockBuilderGetNode(main_dock);
            const ImVec2 ws = ImGui::GetMainViewport()->WorkSize;
            if (node != nullptr && !node->IsEmpty()) {
                main_inited = true;                  // imgui.ini already restored a layout
            } else if (ws.x > 0.0f && ws.y > 0.0f) { // fresh: build once the size is real
                main_inited = true;
                ImGui::DockBuilderRemoveNode(main_dock);
                ImGui::DockBuilderAddNode(main_dock, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(main_dock, ws);
                ImGuiID right;
                ImGuiID left = ImGui::DockBuilderSplitNode(
                    main_dock, ImGuiDir_Left, 0.60f, nullptr, &right);
                ImGui::DockBuilderDockWindow("Analysis", left);
                ImGui::DockBuilderDockWindow("Coupling Lab", right);
                ImGui::DockBuilderFinish(main_dock);
            }
            // else: viewport size not ready this frame; retry next frame.
        }

        stat_scopes.draw(analysis.output());                               // Analysis host + channel windows
        draw_coupling_window(analysis.output(), test_coupling, analysis);  // Coupling Lab
        ImGui::Render();

        // Render the ImGui draw data via a single render pass
        if (!console.acquire_surface_texture()) {
            continue;
        }

        wgpu::CommandEncoderDescriptor enc_desc{};
        wgpu::CommandEncoder encoder = console.device().CreateCommandEncoder(&enc_desc);

        wgpu::RenderPassColorAttachment color_att{};
        color_att.view       = console.backbuffer();
        color_att.loadOp     = wgpu::LoadOp::Clear;
        color_att.storeOp    = wgpu::StoreOp::Store;
        color_att.clearValue = { 0.07, 0.07, 0.09, 1.0 };

        wgpu::RenderPassDescriptor pass_desc{};
        pass_desc.colorAttachmentCount = 1;
        pass_desc.colorAttachments     = &color_att;

        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&pass_desc);
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass.Get());
        pass.End();

        wgpu::CommandBufferDescriptor cmd_desc{};
        wgpu::CommandBuffer cmds = encoder.Finish(&cmd_desc);
        queue.Submit(1, &cmds);

        console.present();
    }

    // --- Shutdown -----------------------------------------------------------
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    std::cout << "[Lab] Shutdown\n";
    return 0;
}
