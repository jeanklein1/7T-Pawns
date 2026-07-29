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

#include <webgpu/webgpu_cpp.h>
#include <dawn/native/DawnNative.h>
#include <dawn/dawn_proc.h>
#if __has_include("dawn/common/Version_autogen.h")
#include "dawn/common/Version_autogen.h"
#define T7_DAWN_VERSION 1
#else
#define T7_DAWN_VERSION 0
#endif
#include <GLFW/glfw3.h>

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>

#include <vector>
#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <optional>

namespace t7 {

    class Console {

        // ═══ §1 IDENTITY ═════════════════════════════════════════

    public:
        Console() = default;
        ~Console() { shutdown(); }

        Console(const Console&) = delete;
        Console& operator=(const Console&) = delete;


        // ═══ §2 INITIALIZATION ═══════════════════════════════════
        //
        // Call order: initGLFW → initWebGPU → initSurface → createDepthBuffer.
        // Each step depends on the previous. If any fails, init returns false.

    public:
        bool init(const char* title, uint32_t width, uint32_t height) {
            initialWidth_ = width;
            initialHeight_ = height;
            currentWidth_ = width;
            currentHeight_ = height;

            if (!initGLFW(title)) return false;
            if (!initWebGPU()) return false;
            if (!initSurface()) return false;
            createDepthBuffer(width, height);

            lastTime_ = std::chrono::high_resolution_clock::now();
            return true;
        }

    private:
        bool initGLFW(const char* title) {
            if (!glfwInit()) {
                std::cerr << "Failed to initialize GLFW\n";
                return false;
            }

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

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

                if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                    console->request_close();
                    return;
                }

                console->inject_key_event(key, action);
                });

            glfwSetCursorPosCallback(window_, [](GLFWwindow* w, double xpos, double ypos) {
                auto* console = static_cast<Console*>(glfwGetWindowUserPointer(w));
                if (!console) return;

                static double last_x = xpos, last_y = ypos;
                console->inject_mouse_move(
                    static_cast<float>(xpos - last_x),
                    static_cast<float>(ypos - last_y)
                );
                last_x = xpos;
                last_y = ypos;
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

            return true;
        }

        bool initWebGPU() {
            dawnProcSetProcs(&dawn::native::GetProcs());

            // Construct instance in place (non-copyable, non-movable)
            instance_.emplace();

            std::vector<dawn::native::Adapter> adapters = instance_->EnumerateAdapters();
            if (adapters.empty()) {
                std::cerr << "No WebGPU adapters found\n";
                return false;
            }

            // The platform line: Dawn's own 20-byte SHA1
            // (all-zero when the build is unhashed).
#if T7_DAWN_VERSION
            {
                static constexpr char hexd[] = "0123456789abcdef";
                std::string dawnRev; dawnRev.reserve(40);
                for (uint8_t b : dawn::kDawnVersion) {
                    dawnRev += hexd[b >> 4]; dawnRev += hexd[b & 0x0F];
                }
                std::cout << "[Console] Dawn revision: " << dawnRev << "\n";
            }
#else
            std::cout << "[Console] Dawn revision: unavailable "
                         "(Version_autogen.h not on the include path)\n";
#endif
#ifdef NDEBUG
            std::cout << "[Console] Build: Release\n";
#else
            std::cout << "[Console] Build: Debug\n";
#endif

            // PROBE_1 C1 — the adapter log: every adapter Dawn
            // enumerates, then the pick. The tree records what it
            // runs on; every METER number is uninterpretable
            // without this line.
            auto sv = [](wgpu::StringView s) {
                return std::string_view(s.data, s.length);
            };
            auto backend_name = [](wgpu::BackendType b) {
                switch (b) {
                case wgpu::BackendType::D3D12:    return "D3D12";
                case wgpu::BackendType::D3D11:    return "D3D11";
                case wgpu::BackendType::Vulkan:   return "Vulkan";
                case wgpu::BackendType::Metal:    return "Metal";
                case wgpu::BackendType::OpenGL:   return "OpenGL";
                case wgpu::BackendType::OpenGLES: return "OpenGLES";
                case wgpu::BackendType::Null:     return "Null";
                default:                          return "?";
                }
            };
            auto type_name = [](wgpu::AdapterType t) {
                switch (t) {
                case wgpu::AdapterType::DiscreteGPU:   return "discrete";
                case wgpu::AdapterType::IntegratedGPU: return "integrated";
                case wgpu::AdapterType::CPU:           return "CPU";
                default:                               return "unknown";
                }
            };
            for (size_t i = 0; i < adapters.size(); i++) {
                wgpu::Adapter a = wgpu::Adapter(adapters[i].Get());
                wgpu::AdapterInfo info{};
                a.GetInfo(&info);
                std::cout << "[Console] Adapter " << i << ": "
                    << type_name(info.adapterType) << " / "
                    << backend_name(info.backendType) << " | "
                    << sv(info.device) << " (" << sv(info.description)
                    << ") vendor=" << sv(info.vendor) << "\n";
            }

            // Adapter selection (landed, PROBE_1): DiscreteGPU
            // outranks integrated; D3D12 breaks ties. Falls back to
            // index 0.
            size_t adapterPick = 0;
            {
                int best = -1;
                for (size_t i = 0; i < adapters.size(); i++) {
                    wgpu::Adapter a = wgpu::Adapter(adapters[i].Get());
                    wgpu::AdapterInfo info{};
                    a.GetInfo(&info);
                    int score =
                        (info.adapterType == wgpu::AdapterType::DiscreteGPU ? 2 : 0)
                      + (info.backendType == wgpu::BackendType::Vulkan      ? 1 : 0);
                    if (score > best) { best = score; adapterPick = i; }
                }
            }
            dawn::native::Adapter& nativeAdapter = adapters[adapterPick];
            wgpu::Adapter adapter = wgpu::Adapter(nativeAdapter.Get());
            std::cout << "[Console] Adapter selected: index=" << adapterPick << "\n";

            wgpu::DeviceDescriptor deviceDesc{};
            deviceDesc.label = "7T Device";
            deviceDesc.SetUncapturedErrorCallback(
                [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message) {
                    std::cerr << "WebGPU Error (" << static_cast<int>(type) << "): "
                        << std::string_view(message.data, message.length) << std::endl;
                });

            // Query adapter limits and request the full capacity.
            // The default maxStorageBuffersPerShaderStage (8) is too tight
            // once generative objects each add render bindings.
            wgpu::Limits adapterLimits{};
            adapter.GetLimits(&adapterLimits);

            deviceDesc.requiredLimits = &adapterLimits;

            // THE FRAME METER (timestamp-query): request the feature when
            // the adapter carries it; consumers check device.HasFeature.
            // Absent → no unsafe-API chasing; downstream degrades loudly
            // to CPU rows only.
            wgpu::FeatureName requiredFeatures[1] = { wgpu::FeatureName::TimestampQuery };
            if (adapter.HasFeature(wgpu::FeatureName::TimestampQuery)) {
                deviceDesc.requiredFeatures = requiredFeatures;
                deviceDesc.requiredFeatureCount = 1;
            }

            std::cout << "[Console] Adapter limits:"
                << " storageBuffers/stage=" << adapterLimits.maxStorageBuffersPerShaderStage
                << " uniformBuffers/stage=" << adapterLimits.maxUniformBuffersPerShaderStage
                << " bindingsPerGroup=" << adapterLimits.maxBindingsPerBindGroup
                << "\n";

            // PROBE_1 C1 — the full enumerated feature list (numeric;
            // settles LEDGER_1 F4-2 at zero cost). Nothing is
            // requested here beyond what the tree already requests.
            {
                wgpu::SupportedFeatures feats{};
                adapter.GetFeatures(&feats);
                std::cout << "[Console] Adapter features (" << feats.featureCount << "):";
                for (size_t i = 0; i < feats.featureCount; i++) {
                    std::cout << " " << static_cast<uint32_t>(feats.features[i]);
                }
                std::cout << "\n";
                std::cout << "[Console] feature multi-draw-indirect="
                    << (adapter.HasFeature(wgpu::FeatureName::MultiDrawIndirect) ? "YES" : "no")
                    << " timestamp-query="
                    << (adapter.HasFeature(wgpu::FeatureName::TimestampQuery) ? "YES" : "no")
                    << "\n";
            }

            device_ = adapter.CreateDevice(&deviceDesc);
            if (!device_) {
                std::cerr << "Failed to create WebGPU device\n";
                return false;
            }

            queue_ = device_.GetQueue();
            adapter_ = adapter;

            return true;
        }

        bool initSurface() {
            wgpu::SurfaceDescriptor surfaceDesc{};
#if defined(_WIN32)
            wgpu::SurfaceSourceWindowsHWND hwndSource{};
            hwndSource.hwnd = glfwGetWin32Window(window_);
            hwndSource.hinstance = GetModuleHandle(nullptr);
            surfaceDesc.nextInChain = &hwndSource;
#elif defined(__linux__)
            wgpu::SurfaceSourceXlibWindow x11Source{};
            x11Source.display = glfwGetX11Display();
            x11Source.window = glfwGetX11Window(window_);
            surfaceDesc.nextInChain = &x11Source;
#endif

            surface_ = wgpu::Instance(instance_->Get()).CreateSurface(&surfaceDesc);

            wgpu::SurfaceCapabilities caps;
            surface_.GetCapabilities(adapter_, &caps);

            colorFormat_ = caps.formats[0];

            surfaceConfig_.device = device_;
            surfaceConfig_.format = colorFormat_;
            surfaceConfig_.width = initialWidth_;
            surfaceConfig_.height = initialHeight_;
            surfaceConfig_.presentMode = wgpu::PresentMode::Fifo;
            surfaceConfig_.alphaMode = wgpu::CompositeAlphaMode::Opaque;
            surface_.Configure(&surfaceConfig_);

            return true;
        }

        void createDepthBuffer(uint32_t w, uint32_t h) {
            wgpu::TextureDescriptor depthDesc{};
            depthDesc.label = "Depth Texture";
            depthDesc.size = { w, h, 1 };
            depthDesc.format = depthFormat_;
            depthDesc.usage = wgpu::TextureUsage::RenderAttachment;
            depthTexture_ = device_.CreateTexture(&depthDesc);
            depthView_ = depthTexture_.CreateView();
        }


        // ═══ §3 FRAME LIFECYCLE ══════════════════════════════════
        //
        // Call order each frame:
        //   begin_frame() → [update cartridges] → acquire_surface_texture()
        //   → [encode & submit] → present() → [back to running()]

    public:
        float begin_frame() {
            glfwPollEvents();

            // Handle resize
            int fbWidth, fbHeight;
            glfwGetFramebufferSize(window_, &fbWidth, &fbHeight);
            if (fbWidth > 0 && fbHeight > 0 &&
                (static_cast<uint32_t>(fbWidth) != currentWidth_ ||
                    static_cast<uint32_t>(fbHeight) != currentHeight_)) {

                currentWidth_ = static_cast<uint32_t>(fbWidth);
                currentHeight_ = static_cast<uint32_t>(fbHeight);
                surfaceConfig_.width = currentWidth_;
                surfaceConfig_.height = currentHeight_;
                surface_.Configure(&surfaceConfig_);
                createDepthBuffer(currentWidth_, currentHeight_);
            }

            // Delta time
            auto currentTime = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float>(currentTime - lastTime_).count();
            lastTime_ = currentTime;

            return dt;
        }

        bool acquire_surface_texture() {
            surface_.GetCurrentTexture(&surfaceTexture_);
            if (surfaceTexture_.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal &&
                surfaceTexture_.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal) {
                return false;
            }
            backbuffer_ = surfaceTexture_.texture.CreateView();
            return true;
        }

        void present() {
            surface_.Present();
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

        void inject_mouse_move(float dx, float dy) {
            InputEvent event{};
            event.type = InputEvent::Type::MouseMove;
            event.x = dx;
            event.y = dy;
            inputEvents_.push_back(event);
        }

        void inject_mouse_button(int button, bool pressed) {
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

        // ── Consumer (main loop reads then clears) ───────────────

        const std::vector<InputEvent>& input_events() const {
            return inputEvents_;
        }

        void clear_input_events() {
            inputEvents_.clear();
        }


        // ═══ §5 ACCESSORS ════════════════════════════════════════

    public:
        wgpu::Device device() const { return device_; }
        wgpu::Queue queue() { return queue_; }
        wgpu::TextureView backbuffer() const { return backbuffer_; }
        wgpu::TextureView depth_view() const { return depthView_; }
        wgpu::TextureFormat color_format() const { return colorFormat_; }
        wgpu::TextureFormat depth_format() const { return depthFormat_; }

        uint32_t width() const { return currentWidth_; }
        uint32_t height() const { return currentHeight_; }

        float aspect_ratio() const {
            if (currentHeight_ == 0) return 1.0f;
            return static_cast<float>(currentWidth_) / static_cast<float>(currentHeight_);
        }

        GLFWwindow* window() const { return window_; }


        // ═══ §6 SHUTDOWN ═════════════════════════════════════════

    public:
        void shutdown() {
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
        uint32_t initialWidth_ = 0;
        uint32_t initialHeight_ = 0;
        uint32_t currentWidth_ = 0;
        uint32_t currentHeight_ = 0;

        // ── Gpu Device ───────────────────────────────────────────
        std::optional<dawn::native::Instance> instance_;
        wgpu::Adapter adapter_;
        wgpu::Device device_;
        wgpu::Queue queue_;

        // ── Surface & Presentation ───────────────────────────────
        wgpu::Surface surface_;
        wgpu::SurfaceConfiguration surfaceConfig_{};
        wgpu::TextureFormat colorFormat_;
        wgpu::SurfaceTexture surfaceTexture_;
        wgpu::TextureView backbuffer_;

        // ── Depth ────────────────────────────────────────────────
        wgpu::TextureFormat depthFormat_ = wgpu::TextureFormat::Depth24Plus;
        wgpu::Texture depthTexture_;
        wgpu::TextureView depthView_;

        // ── Timing ───────────────────────────────────────────────
        std::chrono::high_resolution_clock::time_point lastTime_;

        // ── Input ────────────────────────────────────────────────
        std::vector<InputEvent> inputEvents_;
    };

} // namespace t7