# PORT_0 — THE SEAM CENSUS LEDGER

**The platform boundary, drawn before any edit: what needs a seam (PORT_1), what dies (CUT_1), what is already portable (SAFE).**

READING ONLY. No edits, no builds, no probes. Every claim carries file + line
from HEAD `f475ffc` (working tree clean). Inferences are marked `[INFERRED]`.
Absences are stated as findings.

**METHOD.** Every hypothesis was censused by a dedicated reading pass, then put
through an adversarial second pass instructed to refute it — re-running every
count under its stated boundary, spot-checking every cited line, and hunting for
missed sites under alternate spellings. All eight verdicts survived
verification; the second pass overturned or adjusted ~25 counts (none
verdict-bearing) and surfaced 12 additional sites. Corrections are folded into
the table below and flagged where they changed a number. Per THE COUNT LAW,
every count states its disambiguating boundary; bare substring counts are not
evidence.

**DOC HOME.** The handoff asked for `src/docs/PORT_0_SEAM_LEDGER.md` "beside
LEDGER_1's product; if that lives elsewhere, co-locate and report the path." It
lives elsewhere: LEDGER_1's product is `audit/LEDGER_1_REPORT.md`, and every
read-only campaign report of recent commits lands in `audit/`. This document
therefore lands at **`audit/PORT_0_SEAM_LEDGER.md`**.

**SCOPE.** The translation units the current build compiles — `glaw1` is the
tree's name for the compile gate itself (`cartridge.hpp:5-8`), realized as CMake
target `incubator_dual`, preset `the-board-full`: `src/incubator_dual.cpp`
(harness), `src/external/stb_image.cpp`, `src/external/RtMidi.cpp`
(`CMakeLists.txt:541-547`), and their 75-file project include closure
(cartridge `src/cartridges/the_board/**` 42 files + `world.wgsl`,
`src/console/console.hpp`, `src/analysis/canvas_1/canvas.hpp` + 2 interface
headers, 16 × `src/musical/*.hpp`, 3 × `src/coupling/*.hpp`, 3 ×
`src/sources/*.hpp`, `src/core/{cartridge_ids,input_event,instruments}.hpp`,
`src/render/render_cartridge.hpp`). `world.wgsl` (runtime-loaded, never
`#include`d) is in scope: 619,975 B / 13,380 lines. Excluded per handoff:
`src/tools/*.jsx` (no live call sites; dies in CUT_1), plus `the_lab.cpp`,
`main.cpp`, probe/check targets (separate CMake targets, not this build).
Build defines in force: `INCUBATE_RENDER=the_board INCUBATE_ANALYSIS=canvas_1
INCUBATE_DEMO=full T7_INSTRUMENTS=off NOMINMAX WIN32_LEAN_AND_MEAN
_CRT_SECURE_NO_WARNINGS __WINDOWS_MM__` (`CMakeLists.txt:508,559-565`).
RtMidi.cpp compiles only its `__WINDOWS_MM__` branch (lines 2644-3209); its
ALSA/JACK/CoreMIDI/UWP/AMIDI/Web-MIDI branches are textual only — every RtMidi
count below states compiled-vs-textual.

**PRIOR ART.** A hand-mirrored JS web port existed and was deleted
(`audit/past reports/WEB_PORT_LEDGER.md`); its surviving lesson is L7 — THE
BINDING-CLOSURE LAW (`src/docs/LAWS.md:136-152`: "Any future port wants
**generated** layouts, not transcribed ones"). This port carries the shader
verbatim under Emscripten instead of mirroring it; this census is the recon
that strategy binds to.

---

## THE VERDICTS

| H | Hypothesis | Verdict |
|---|---|---|
| H1 | TIME — one frame loop, extractable `frame()` | **CONFIRMED** (with one purity amendment) |
| H2 | ASYNC — floater grammar only, outside boot | **CONFIRMED** (boot is synchronous-native — itself the seam) |
| H3 | SKIN — no Windows headers in the cartridge | **CONFIRMED** (skin lives in console.hpp, not the nominal harness) |
| H4 | NATIVE DAWN — one place, nothing per-frame | **CONFIRMED** (expected toggles do not exist at all) |
| H5 | LIMITS — short exceedance list | **CONFIRMED** — the list has exactly 2 entries |
| H6 | THREADS — single-threaded except MIDI intake | **CONFIRMED** |
| H7 | I/O — all reads at boot | **FALSIFIED** — three independent legs |
| H8 | CUT — clean organ edges | **CONFIRMED** (with amendments: one organ is already gone, one is misnamed) |

---

## H1 — TIME: CONFIRMED

**The claim held.** The compiled build contains exactly one top-level frame
loop: `while (console.running())` at `incubator_dual.cpp:205-265`, in the
harness's `main()`. Boundary: grep `glfwWindowShouldClose|running\(\)` across
`src/` — qualified-call matches only console.hpp:414-415 (definition),
console.hpp:22/371 (comments), incubator_dual.cpp:205, and the_lab.cpp:572
(outside this build's TU set). Every other loop in the closure is bounded
(insertion sorts, ring drains, beat catch-ups, stb decode loops) or textual
RtMidi; the one compiled RtMidi `while` (RtMidi.cpp:3187, `Sleep(1)` retry) is
on the MIDI-**out** path this program never constructs (`RtMidiIn` only,
midi_port.hpp:44). `world.wgsl` contains zero `while`/`loop{}` constructs
(grep `while\s*\(` = 0, `\bloop\s*\{` = 0) — no GPU-side unbounded loop.

**Loop-carried state — 6 `main()` locals** (declaration line, role):
`console` (162, platform shell), `analysis` (170, canvas_1), `render` (180,
the_board), `watcher` (196, FileWatcher: path_ + lastWriteTime_),
`reload_frame_counter` (201), `queue` (202, cached handle). Per-frame-only:
`dt`, `event`, `encDesc/encoder`, `cmdDesc/commands`.

**Every call the loop makes besides the frame body:** `console.running()` 205;
`console.begin_frame()` 206 — the ONLY OS event pump in the closure
(`glfwPollEvents` console.hpp:375), resize check (`glfwGetFramebufferSize` 379
→ `surface_.Configure` 388 + depth recreate 389), and the frame clock
(`std::chrono::high_resolution_clock::now()` 393-395 against `lastTime_` 618;
**no `glfwGetTime` exists anywhere in the closure** — NAME LAW);
`watcher.check()` 218 — one `std::filesystem::last_write_time` stat per 30
frames (103); `std::cout` 216 (compile-time off: `INSTRUMENTS.watcher_ticks`
false at `T7_INSTRUMENTS=off`, instruments.hpp:68-83) and 219 (on-change);
`console.acquire_surface_texture()` 251 (`GetCurrentTexture` + status check,
`continue` on failure); `CreateCommandEncoder` 256; `encoder.Finish` 261;
`queue.Submit` 262; `console.present()` 264 → `surface_.Present()`
(console.hpp:411). **There is no sleep anywhere** — pacing is entirely the Fifo
present mode (console.hpp:349); in the browser, rAF replaces it.

**Extractability — one amendment (adversarial pass).** The harness rind is
pure: every platform call funnels through four Console methods + the watcher
stat. But `frame(dt)` is **not platform-pure inside**: `render.update()`
performs synchronous disk I/O on gallery-commit and world-transition frames
(gallery.hpp:1135 → directory scan 1569/1604 + `stbi_load` 1507/1511; teardown
path cartridge.hpp:1022 → `rotate_authored_staging` gallery.hpp:1646 →
stbi_load per rotated slot), plus shader re-read on hot-reload
(renderer.hpp:1232/1256) and mid-frame `std::cout` witnesses (e.g.
gallery.hpp:1691). The browser `frame()` needs an FS story for these (see H7).
Also: **no dt clamp exists in the closure** — `begin_frame` returns the raw
clock delta; a dormant `core/clock.hpp:34` has the exact clamp the rAF world
will want (tab-suspend delivers huge dt), but it is dead code, `#include`d
nowhere (grep `core/clock` over src = 0).

**Cross-thread state the loop reads each frame** (belongs to TIME because it is
the musical clock): the MIDI ring (midi_port.hpp:153-154 atomics, drained at
canvas.hpp:155) **and** the transport atomics `pulses_/playing_/bpm_/synced_`
(transport.hpp:86-89, read via `port_.beats()` canvas.hpp:153). Two channels,
one producer thread. Both die in CUT_1; a Web-MIDI replacement must carry the
transport clock, not just note events.

---

## H2 — ASYNC: CONFIRMED

**The floater grammar, from the tree.** True state names: `IDLE / COPIED /
MAPPING` (cartridge.hpp:280-295; the expected "IN_FLIGHT" does not exist).
Sequence: copy-to-staging in the frame encoder guarded `state==IDLE`
(skip-if-busy; sites 1551, 1559, 1571, 2041) → submit → `MapAsync(MapMode::Read,
CallbackMode::AllowSpontaneous, gen-captured lambda)` → on Success: `world_gen`
check, `GetConstMappedRange` + memcpy, `Unmap` unconditionally → `IDLE`.
Teardown cancels COPIED and deliberately leaves MAPPING in flight
(cartridge.hpp:976-992).

**The complete wait/async census.** `\.MapAsync\(` call-with-paren over src/ =
**4 sites, all cartridge.hpp**: agent 1178, floater 1221, camera 1256 (gated
`point_.host==CAMERA`, 1569), meter 1976 (under
`if constexpr (INSTRUMENTS.frame_meter)` — folded out of the default build; its
comment at 1963-1972 names the grammar). All 4 conform. `GetConstMappedRange(`
= 4, `\.Unmap\(` = 4, `MapAsyncStatus` = 8 — all in the same file. NAME LAW:
the expected spelling `GetMappedRange(` scores **zero** in the tree; all mapped
reads are const.

**Zero pump exists.** `ProcessEvents(` = 0, `\.Tick\(` = 0, `WaitAny` = 0,
`SubmittedWorkDone` = 0, `wgpu::Future|\.Poll\(` = 0 across src/. Dawn native
fires AllowSpontaneous callbacks from submit internals; the loop never ticks
the device. On the web, map callbacks resolve on event-loop turns — the rAF
restructure delivers them with no pump to invent, but AllowSpontaneous
semantics under emdawnwebgpu is the one assumption PORT_1 must re-verify.

**Boot has no wait because boot never goes async** — the deeper finding. No
spin, no future, no WaitAny anywhere: console.hpp uses dawn::native
**synchronous** `EnumerateAdapters()` (170) and `adapter.CreateDevice()` (312);
`RequestAdapter/RequestDevice` appear nowhere. Browser boot is mandatory-async
— this "absence of waiting" is itself the largest single seam. Also at boot:
`SetUncapturedErrorCallback` (257, the only other async callback registration
in the closure — maps to `onuncapturederror`), sync `GetLimits`/`GetFeatures`/
`HasFeature` adapter queries (267, 298-299, 306/308).

**Other traffic (not waits):** all uploads fire-and-forget — `\.WriteBuffer\(`
= 79 (state.hpp) + 1 (render_passes.hpp:139), `\.WriteTexture\(` = 1
(state.hpp:2385); `mappedAtCreation|MapMode::Write` = 0 src-wide. GPU→GPU
copies only (render_passes.hpp:271/289; `CopyTextureToTexture` state.hpp:3037,
gallery.hpp:1498); **no texture readback to CPU exists**
(`CopyTextureToBuffer` = 0). One extra submit: gol_zones.hpp:686, fire-and-
forget. All pipeline creation is synchronous — `CreateRenderPipelineAsync`
absent repo-wide (the tree says so itself, renderer.hpp:2240-2242) — legal in
browser WebGPU, but it blocks the thread on compile at boot and on hot-reload.
Dead readback: `ribbonReadbackStaging_` (state.hpp:3318-3324) is allocated
every boot, never copied into, never mapped — tree marks it LATENT (3273);
CUT_1 inventory.

---

## H3 — SKIN: CONFIRMED

**No Windows header reaches the cartridge or the shared layers** —
`src/cartridges/**`, `src/musical/**`, `src/analysis/**`, `src/coupling/**`,
`src/sources/**`, `src/core/**`, `src/render/**` contain zero OS-header
includes and zero Win32 tokens (boundary: include-directive regex over the
closure for `windows.h|winsock|mmsystem.h|timeapi.h|winrt/|wrl|dsound|xinput|
dinput|shellapi|shlobj|commdlg`; token `\b(HWND|HINSTANCE|DWORD|WINAPI|
GetModuleHandle)\b`). The handoff's nominal harness holds none of the skin
either: `incubator_dual.cpp`'s `#include <GLFW/glfw3.h>` (74) is **vestigial**
— zero `glfw*(` calls, zero `GLFW_` tokens in the file. The skin lives in
**`src/console/console.hpp`** and the vendored externals:

- **Compiled OS-header includes: exactly 2** — RtMidi.cpp:2651-2652
  (`<windows.h>`, `<mmsystem.h>`, inside `__WINDOWS_MM__`). The 8 UWP includes
  (3233-3240) are textual. `windows.h` reaches console.hpp only transitively
  via `glfw3native.h` (48) under `GLFW_EXPOSE_NATIVE_WIN32` (42).
- **The surface seam** (console.hpp:324-354): `wgpu::SurfaceSourceWindowsHWND{
  hwnd=glfwGetWin32Window(window_), hinstance=GetModuleHandle(nullptr)}` →
  `CreateSurface`; format from `caps.formats[0]` (343), `PresentMode::Fifo` +
  `Opaque` (349-350). `GetModuleHandle` (329) is the **only Win32 call outside
  RtMidi.cpp**. Emscripten twin: `SurfaceSourceCanvasHTMLSelector`.
- **GLFW census**: 27 code call sites / 21 distinct functions in console.hpp
  (boundary: call-with-paren `glfw[A-Z]\w*\(`; 19 compiled — the two
  `glfwGetX11*` sit in the dead `__linux__` branch). The message pump is one
  call: `glfwPollEvents()` (375). No `PeekMessage/GetMessage/DispatchMessage`
  anywhere. Input: 5 callback registrations (112-159) → 5 `inject_*` producers
  → `std::vector<InputEvent>`; ESC and KP_MULTIPLY are consumed at console
  level and never reach cartridges (117-128). Cursor grab = `GLFW_CURSOR_
  DISABLED` + `GLFW_RAW_MOUSE_MOTION` (536-543) → Pointer Lock.
- **The one GLFW reach into the cartridge**: `direction/input.hpp:10` includes
  glfw3.h for **keycode constants only** — 63 `GLFW_*` tokens, 27 distinct, 0
  function calls. Keycodes survive under Emscripten-GLFW or become an enum in
  `core/input_event.hpp` (already the platform-free DTO).
- **Timers**: frame clock is `std::chrono::high_resolution_clock`
  (console.hpp:88, 393-395, 618). `glfwGetTime(` = 0 in the closure — NAME
  LAW: the expected timer name does not exist. `QueryPerformanceCounter` only
  in RtMidi's textual UWP region (3572). `timeGetTime/timeBeginPeriod/
  CreateEvent/WaitForSingleObject` = 0 in the whole file — WinMM timestamps
  come from the driver's callback DWORD parameter (RtMidi.cpp:2702→2715→2777).
- **Compiled Win32 call census in RtMidi (`__WINDOWS_MM__`)**: 18 `midiIn*(` /
  12 `midiOut*(` / 6 CriticalSection sites / 1 `Sleep(` — full line lists in
  the table. All CUT with MIDI intake.
- **Raw filesystem path strings, every one**: renderer.hpp:1250-1251 (the
   2-entry world.wgsl search list — see H7); gallery.hpp:1510 `"7t/"`, 1574
  `"assets/paintings"`, 1575 `"7t/assets/paintings"`, 1594 `"PAINTING_"`, 1597
  `".jpg"/".jpeg"`; incubator_dual.cpp:171 `"assets"` (DISCARDED by receiver —
  canvas.hpp:104 `(void)asset_path`). Drive-letter literals: **0** (boundary:
  `"[A-Za-z]:[/\\]` with leading quote). canvas.hpp:145 `"loopMIDI"` is a MIDI
  port name, not a path.
- stb_image compiles the portable-CRT path: `fopen_s` (stb_image.h:1357, MSVC
  branch; the `STBI_WINDOWS_UTF8` Win32 branch with its 7 decl/call sites
  1325-1354 is never enabled — the macro is defined nowhere in the repo).

---

## H4 — NATIVE DAWN: CONFIRMED

**One place.** Every `dawn::native::` token in the closure sits in
console.hpp: 165 (`dawnProcSetProcs(&dawn::native::GetProcs())`), 170
(`EnumerateAdapters`), 251 (`dawn::native::Adapter&`), 600 (member
`std::optional<dawn::native::Instance>`; constructed by bare `emplace()` at
168 — no `InstanceDescriptor` exists in the tree). Dawn includes: 31/32/34,
console.hpp only. Adapter-side `.Get()` bridges: **3** (225, 242, 252 —
census said 2; corrected). Backend selection = the adapter-pick scoring
(238-250: DiscreteGPU+2, D3D12+1) — log + pick only. `dawn::kDawnVersion`
banner (182) is self-guarded by `__has_include` (33-38). All BOOT; shutdown has
no native site (optional's destructor).

**The expected toggles DO NOT EXIST** — NAME LAW finding, not a stop:
`DawnTogglesDescriptor` = 0 hits in src/; no toggle name strings; no
`use_dxc`; `DeviceDescriptor.nextInChain` never assigned; the only chained
structs in the closure are the two surface sources (console.hpp:330, textual
335) and the portable `ShaderSourceWGSL` chain (renderer.hpp:1278-1286). The
FXC discipline is carried **structurally** in comments and shader shape — 56
comment hits (41 renderer.hpp, 13 world.wgsl, 2 state.hpp/seed_utils.hpp;
boundary: word-bounded case-sensitive `\bFXC\b`), **zero code hits**. There is
no FXC-workaround configuration to port.

**Nothing native-only executes per-frame.** The frame path is portable
spelling throughout: `begin_frame`/`GetCurrentTexture`/`Configure`-on-resize/
`CreateCommandEncoder`/`Submit`. The single per-frame native-**pattern** call
is `surface_.Present()` (console.hpp:411, the only `.Present(` in src/) —
portable header spelling, but the browser has no explicit present; it needs a
no-op/rAF seam, and `PresentMode::Fifo`/`CompositeAlphaMode::Opaque` (349-350)
move to canvas-context configuration. The readback pump needs no native call
at all (H2). `wgpu::FeatureName::MultiDrawIndirect` (306) is a Dawn-extension
enum used for a boot log probe only — never requested, never drawn with; the
tree's actual indirect draw (`DrawIndexedIndirect` renderer.hpp:723) is core
WebGPU and needs no feature (args reset with firstInstance=0 each frame,
state.hpp:2805-2816; cull kernel touches only instanceCount slots,
world.wgsl:10069-10081).

---

## H5 — LIMITS: CONFIRMED, and the short list has exactly two entries

**The exceedance list (vs. WebGPU defaults):**

1. **`maxStorageBuffersPerShaderStage`: 9 needed, default 8.** The room-family
   compute pipelines (`update_player_agent/update_other_agents/update_sphere/
   update_cube`, renderer.hpp:1349-1385) stack layouts Compute Entity (5
   storage, state.hpp:4243-4283) + Compute Texture (0) + The Room (4,
   state.hpp:5110-5124) = **9 storage-buffer layout entries visible to the
   compute stage**. WebGPU validates the LAYOUT, not shader usage — these four
   pipelines fail at creation on a default-limits device. The native build
   masks this by requesting **full adapter limits** (console.hpp:266-269,
   comment: "The default maxStorageBuffersPerShaderStage (8) is too tight");
   LAWS.md L2:46 pins the working caps at 10 storage / 12 uniform (FXC/D3D12
   working limits, not WebGPU defaults). CUT_1 interaction: if the FIELD trio
   in g2 dies, the count drops to ≤ 8 — recount after the cut.
2. **`maxTextureArrayLayers`: 289 needed, default 256** (adversarial-pass
   find). The two patch arrays — heightfield (state.hpp:4110, 256×256×289
   RGBA16Float) and cell color (4129, 16×16×289 RGBA8Unorm), layer count
   `Dim::MAX_ACTIVE_PATCHES = 17² = 289` (81-82), views 4119/4138 — exceed the
   default at `CreateTexture`, independent of finding 1. `requestDevice` must
   raise both limits (and clamp against `adapter.limits`, or copy the
   request-adapter-max pattern).

**Features.** Exactly one feature is ever requested:
`wgpu::FeatureName::TimestampQuery` (console.hpp:282-286) — requested whenever
the adapter has it, **unconditional w.r.t. `T7_INSTRUMENTS`** (the comment at
273-281 declares this deliberate). With the default dial the feature arms
nothing: QuerySet creation is double-gated (`INSTRUMENTS.frame_meter &&
HasFeature`, state.hpp:3332-3333), and the ~22 unconditional `timestampWrites`
arming call sites across 8 files all no-op through the centralized constexpr
fold `meter_arm_alloc` (state.hpp:2956 `if constexpr (!INSTRUMENTS.frame_meter)
return UINT32_MAX;`). The web build may drop the request; METER_1 rides
browser timestamp-query (quantized) when wanted.

**Texture formats — all core.** Distinct set (boundary: qualified token
`wgpu::TextureFormat::\w+`, 22 in-scope tokens; 12 in-scope `CreateTexture(`
sites): surface format from capabilities (console.hpp:343 — never hard-coded
in a pipeline; the handoff to the render core is one call,
incubator_dual.cpp:183), Depth24Plus (swap depth), Depth32Float (4096² sun +
spot shadow, **comparison-sampled only** — sun 16-tap `textureSampleCompareLevel`
world.wgsl:3868-3883, spot `textureSampleCompare` 4122-4136; the WebGPU ban on
filtering depth32float is not violated), R32Float ×2 (textureLoad/NonFiltering
only — no float32-filterable needed), RGBA16Float, RGBA8Unorm, BGRA8Unorm
(compare-only tokens; the CPU R↔B swap probes state.hpp:2357/2391 make painting
upload byte-order agnostic for either web surface format). Storage textures: 6
declarations, all `write`-only, formats rgba16float/r32float/rgba8unorm — all
core-guaranteed (no bgra8unorm-storage, no read_write). Samplers: 4, no
anisotropy, no mipmaps; binding types Filtering ×8, NonFiltering ×3,
Comparison ×1.

**Buffers (H5B; boundary: `CreateBuffer\(` + `makeBuffer\(` call-with-paren —
all creation lives in state.hpp: 6 + 73 sites = 78 objects, 76 in the default
build).** Largest by class: STORAGE Live-Card Scratch 3,276,800 B (:3365);
VERTEX Column VB 1,920,000 B (:3785); INDEX Column IB 768,000 B (:3788);
UNIFORM-usage Floating-Entity Array 54,912 B (:3270); MapRead staging 54,912 B
(:3309); QueryResolve 512 B; Indirect 60 B. Total resident ≈ **13.89 MB**
(corrected sum; census said 15.5) — 1.2% of default `maxBufferSize`. **Near-
limit flags:** the Floating-Entity Array uniform binding is 54,912 B = **83.8%
of default `maxUniformBufferBindingSize`** (264 × 208 B; growth past 248
B/slot or 315 slots crosses the default); VS and FS storage in the main render
family are both **7 of 8** (the tree knows: state.hpp:3263 "the render VS
storage cap is full"; 4386-4398 two tables demoted to Uniform to duck the cap;
4409-4410); compat mode is ruled out as-is.

**Bind groups — the three-group law holds in code, not in LAWS.md.** LAWS.md
contains no "three groups" sentence (L6 governs binding NUMBERS,
LAWS.md:107-134, and names only g0/g1 as examples) — the fact is authored in
`binding_registry.hpp` (namespaces `g0` 33-132 / `g1` 139-154 / `g2` 162-175;
97 constants: 77/14/6; max index 501) and mirrored exactly in world.wgsl:
`@group(0)` ×80, `@group(1)` ×14, `@group(2)` ×6, `@group(3)` ×**0** (100
declarations over 97 slots — 3 fc_* aliases, no entry point uses both members
of a pair). `SetBindGroup(` = **82** call sites in renderer.hpp (corrected;
census said 57) + 1 in render_passes.hpp:150; indices 0/1/2 only; index 2 only
at the four room-family kernels (370/384/409/423). 25 `CreateBindGroupLayout(`
/ 27 `CreateBindGroup(` sites (state.hpp); 29 compute + 30 render pipelines
from 11 textual creation sites (builders: `makeComputePipeline` ×29 invocations,
`computeLayoutFor` ×18, `makeEntity` ×9 — corrected from 11, `makeShadow`
×13); max 3 bind groups per pipeline layout. Dynamic offsets: **0** repo-wide;
frustum-cull segment offsets are static 0/512/1024 over one 2,048 B buffer
(state.hpp:1542-1547), 256-aligned. Vertex state: ≤ 1 vertex buffer, ≤ 4
attributes, strides pinned by static_asserts (state.hpp:1671-1678); 14
pipelines bufferless.

**WGSL (H5C) — the crown jewels are core-clean.** 65 entry points (28 vertex /
8 fragment / 29 compute). `@workgroup_size` ×29, all literal:
64 ×6, 1 ×9, (1,1,1) ×4, (8,8,1) ×5, (8,8) ×1, 32 ×1, (16,16) ×3 — max
product exactly **256 = the default cap** (8722/8743/9337; zero headroom,
still legal). `var<workgroup>` ×2 (8741, 9335), 1,600 B each vs 16,384
default. Extension surface: **zero** — no `enable`/`requires`, no f16, no
subgroups, no push constants, no `ptr<storage>` params, no deprecated syntax
(boundaries in table). One `override` constant (`USE_PATCH_INDIRECTION`, :129,
set once from C++ renderer.hpp:1662). Per-entry-point static binding use maxes:
8 storage (exactly at default, `patch_terrain_fs` L4585 and
`update_other_agents` L7920), 6 uniforms, 6 sampled textures, 3 samplers, 1
storage texture; per-LAYOUT storage textures max 2 (Patch Gen) vs default 4;
inter-stage variables max 6 vs default 16. Fragment-only builtins verified
reachable only from fragment entries. One shader-module creation site in the
whole closure (renderer.hpp:1286, fed by disk `ifstream` — the delivery seam);
zero embedded WGSL in headers.

---

## H6 — THREADS: CONFIRMED

The program creates **zero threads of its own** under five boundaries run
separately over src/: qualified `std::thread`, `std::jthread`,
`CreateThread|_beginthreadex|_beginthread`, `std::async`, `timeSetEvent(` —
all zero compiled hits (RtMidi's 3 `pthread_create` sites are textual
ALSA/Android). The verifier's wider battery (thread pools, futures,
condition_variables, semaphores, Interlocked, `#include <thread>`) also all
zero in the closure.

**The one extra execution context is OS-owned**: `midiInOpen(...,
CALLBACK_FUNCTION)` (RtMidi.cpp:2857-2861, compiled) registers
`midiInputCallback` (2698-2791) on the winmm **driver** thread. What it
services: MIDI intake only. The crossings to the main thread, exhaustively:
(1) the project's SPSC ring — midi_port.hpp:152-154 (`ring_[256]`, 2 atomics,
correct acquire/release; producer `push` 189-195 drop-on-full; consumer `poll`
121-133 on the frame thread via canvas.hpp:155); (2) the transport atomics —
transport.hpp:86-89 (`pulses_/playing_/bpm_/synced_`; producer `feed/on_clock`
on the driver thread via midi_port.hpp:167; consumer `beats()` per frame,
canvas.hpp:153); (3) RtMidi-internal: one compiled `CRITICAL_SECTION`
(RtMidi.cpp:2690, sysex-buffer requeue vs closePort) and the **unsynchronized**
internal MidiQueue (RtMidi.h:599-611, plain front/back) — written only in the
open→setCallback window (midi_port.hpp:75-76 opens before registering; ≤ 100
messages, never drained; formally a data race, practically bounded dead
weight). Fragile edge worth naming: `MidiTransport::reset()` writes non-atomic
`acc_/ema_` from the main thread during `close()` (midi_port.hpp:97-106) —
safe only because `cancelCallback+closePort` quiesce the driver thread first.

**Everything above dies in CUT_1 with MIDI intake.** GLFW callbacks run inside
`glfwPollEvents` on the main thread (not threads). Dawn's internal threads are
Dawn's own; project code configures none. stb_image: TLS flags only. The
`thread` word-sweep (case-insensitive, closure minus RtMidi) = 48 lines: 44
comments/terminology + 4 WGSL code lines (`thread_id` workgroup-stride locals,
world.wgsl:8756/8760/9346/9350) — all GPU-side, no CPU concurrency. RtMidi's
textual `__WEB_MIDI_API__` branch (4522-4833) is main-thread Web MIDI — the
intake thread would not exist in a browser build even if intake survived.

---

## H7 — I/O: FALSIFIED (three independent legs)

**Leg 1 — per-frame-adjacent, unconditional:** the FileWatcher stats
world.wgsl's mtime every 30 frames inside the frame loop, forever
(incubator_dual.cpp:209 gate → :218 `watcher.check()` → :103
`std::filesystem::last_write_time`, error_code overload; ~2×/s). Only the
progress dot is INSTRUMENTS-gated — the stat is not. Dev machinery; dies in
the browser.

**Leg 2 — on-event, dev:** a detected change re-reads world.wgsl
(renderer.hpp:1232-1242) and rebuilds **all** pipelines (1211-1217) —
compile-scale cost, via incubator_dual.cpp:220 → cartridge.hpp:2112.

**Leg 3 — on-event, PRODUCTION:** world/mood transitions read disk after boot:
teardown (cartridge.hpp:1022, inside the TEARDOWN block 964-1088) →
`teardown_gallery` (gallery.hpp:2149) → `rotate_authored_staging` (1646,
guard 1662) → up to 32 `stbi_load` decodes on the frame thread. The rotation
round-robins `authored_disk_cursor` through **all 57** paintings; **25 of the
57 are reachable ONLY through this post-boot path**. A fetch manifest built
from "what boot loads" silently drops 3,439,629 B of reachable content. This
leg falsifies H7 even if hot-reload is classed dev-only.

**Boot loads (the rest of the story).** world.wgsl: 619,975 B, read at boot
via `std::ifstream` (renderer.hpp:1256; the only 2 ifstream sites in the
closure are 1232/1256). **Path contradiction with the handoff**: the search
list (renderer.hpp:1249-1252) is exactly
`{"../../../src/cartridges/the_board/realization/world.wgsl",
"src/cartridges/the_board/realization/world.wgsl"}` — CWD-relative into the
**source tree**. The `cartridges/` POST_BUILD copy beside the exe
(CMakeLists.txt:568-573) is **never on the search list — dead for the compiled
loader** (that is also what makes editing the source trigger hot reload). The
`assets/` copy (575-580) IS live. Paintings: `scan_paintings_folder`
(gallery.hpp:1568-1618) enumerates `{"assets/paintings","7t/assets/paintings"}`
at boot, filters `PAINTING_*.jpg/.jpeg`, numeric-sorts; boot eager-load
(cartridge.hpp:570-577, `if constexpr (ROSTER.gallery)` — true in demo=full)
decodes the first **32** (= `Dim::STAGING_LAYERS`, state.hpp:270) of 57.
canvas_1 reads no files (asset_path discarded, canvas.hpp:104); the .mid
parser (`sources/midi_file.hpp`) is **out of closure, included by nothing**.
Compiled file-open chain for paintings: `stbi_load` → `fopen_s`
(stb_image.h:1357, MSVC branch — corrected from the census's `_wfopen` claim).
Writes: **zero** in the closure (`stbi_write|ofstream|fwrite` = 0).

**THE FETCH MANIFEST (PORT_1 bundle budget).**

| Class | Files | Bytes | Load moment |
|---|---|---|---|
| world.wgsl | 1 | 619,975 | boot (+ hot-reload re-read, dev) |
| paintings, boot-critical | 32 (PAINTING_1..109 numeric order) | 5,633,069 | boot eager-load |
| paintings, rotation-only | 25 (PAINTING_110..1002) | 3,439,629 | world transitions (production) |
| `.mid` clips + README | 4 | 896 | **never read by compiled code — exclude** |
| **Total fetch budget** | **58** | **9,692,673 (~9.24 MiB)** | boot-critical 6,253,044 |

GPU-side note for the same budget: authored staging is 32 × 1024×1024×4 B
regardless of fetch count (state.hpp:263/270), and the 289-layer patch arrays
+ two 4096² depth maps dominate texture memory (~285 MB class) — a PORT_1
mobile-browser note, no WebGPU limit formally exceeded.

**Console traffic** (seam-adjacent, not disk): 171 `std::cout|printf|fprintf`
tokens in the closure; **zero unconditional per-frame prints in the default
build** — every recurring print is INSTRUMENTS-gated (watcher dot 215-216,
[ONSET] canvas.hpp:469, [ZOETROPE] cube_behaviors.hpp:1067, [CHECKER]
visual_canvas.hpp:503, [CENSUS]/[METER] cartridge.hpp:1353/1369); the rest are
boot banner and on-event witnesses, including mid-frame gallery writes
(gallery.hpp:1616/1642/1691 — 1691 self-describes as "Autonomous stdout —
exhibition-guard candidate, still open") and ~13 `std::cerr` sites (list in
table; console.hpp:259's WebGPU error callback can spam unbounded under a
broken pipeline — worth a PORT_1 guard).

---

## H8 — THE CUT BOUNDARY: CONFIRMED (with two amendments)

**Organ A — MIDI/analysis intake. Edge: clean and narrow.**
Dies: `external/RtMidi.{cpp,h}` (sole in-closure consumer midi_port.hpp:26),
`sources/{midi_event,midi_port,transport}.hpp`, `analysis/
analysis_cartridge.hpp`, `analysis/canvas_1/canvas.hpp`, and **15 of 16**
`musical/*.hpp` (count corrected: 16 files, not 15). **Survives — the type
spine**: `analysis/analysis_signal.hpp` (6 in-closure includers, corrected
from 5: render_cartridge.hpp:29, state.hpp:30 — vestigial, zero tokens used,
sole cartridge token use is cartridge.hpp:636 —, visual_canvas.hpp:67,
signal_layout.hpp:25, analysis_cartridge.hpp:41, canvas.hpp:83),
`musical/signal_layout.hpp` (the render side's name resolver), and all 3
`coupling/*.hpp` — **amendment: the hypothesis listed coupling/ as intake; it
is the surviving render side's coupling engine** (cartridge.hpp:77, member
:177).
Harness anchors (the snip line): incubator_dual.cpp **171**
(`analysis.initialize`), **193** (`render.bind_signal_layout(
analysis.stat_layout())` — note the comment at 192 misstates "constexpr"; the
layout is runtime-built in initialize, safe only by call order), **229-231**
(`analysis.on_input` — dead: `is_music_key` always false, and
`Canvas::on_input` is a no-op), **243** (`analysis.update(dt)`), **248**
(`render.update(analysis.output(), ...)`).
Render-side reads (the stub contract): `bind_signal_layout`
cartridge.hpp:636-654 (8 param-target resolves); `ctx.signal` read in 6 update
phases (U1 767-787 — incl. the stats[0..63] GPU copy 776-778 whose WGSL mirror
`GPUFrameSignal.stats` (world.wgsl:770) has **zero live readers** since M1-C:
dead weight crossing the seam, layout pinned 336 B by static_asserts; U3
824-834 tempo follower; U4 846-889 `visual_canvas_.tick` + zoetrope; U7
953-956); `visual_canvas.hpp::bind` 277-351 resolves **12** distinct source
names (corrected from 10): `all.field`, `ch1.present_count`,
`all.window_length`, `all.present_count`, `ch1.window_length`, and
`ch0.onset`..`ch6.onset`. Every miss degrades gracefully (signal_layout.hpp:
55-58, warn + disable). Post-cut stub must provide: the `AnalysisSignal`
struct + a `StatLayoutView` resolving those 12 names + **advancing
t_beats/t_seconds/dt** (frozen time freezes all music-coupled motion; crashes
nothing).

**Organ B — F-key dynamics.** Single home: `direction/input.hpp` — 15
`GLFW_KEY_F[0-9]+` regex hits (10 fallback defines 201-215 + 5 case labels
272-276); nothing elsewhere in the closure. F4→`cycle_cube_behavior_override`,
F5→`cycle_floater_coordination`, F6→`reveal_zoetrope`,
F7→`toggle_cube_kite_mode`, F8→`possess(RIBBON)` (gate 282). Clean snip = 5
case labels + 10 defines; the target machinery in cube_behaviors.hpp then
freezes at safe defaults. **Two rulings needed**: (a) F8 is live navigation
(shares `possess()` with surviving key 4; named in the harness controls print,
incubator_dual.cpp:199) — its cut belongs to a ribbon decision, not a
diagnostics one; (b) F5 entanglement: `config.floater_coordination` boots 0.0
(state.hpp:1785) and only F5 raises it — cutting F5 leaves the beacon writer
(cartridge.hpp:891-917) and the WGSL cube-synchrony arm permanently inert
(readers cartridge.hpp:899, world.wgsl:1602/8547): cut them together or re-pin
the boot value.

**Organ C — retired camera/ribbon modes: ALREADY GONE.** Amendment: 53
case-insensitive `retired` hits across the_board (count corrected from 36; 40
in .hpp + 13 in world.wgsl) are **all** memorial comments, static_assert
strings, or byte-layout ballast (`_pad_pier_retired` state.hpp:500; always-0
pier fields 1005-1017; zeroed self-sep rows spawn_services.hpp:108/110). No
live retired-mode machinery exists — no camera-mode enum (`camera_mode|
CameraMode|orbit_mode` = 0 code hits), PointHost and RibbonColorMode all-live.
The ballast **must stay** (GPU struct offsets pinned by static_asserts,
state.hpp:1660-1666). CUT_1 has nothing to delete here.

**Organ D — ribbon-avoidance: real, but the name is wrong.** NAME LAW:
case-insensitive `avoid` yields zero machinery hits in C++ — the machinery is
**THE FIELD** (FIELD_2). CPU dialect (the ribbon's read): ribbon.hpp:902-1071
inside `ribbon_frame_tick` (:824), consts 122-141, panel dials
control_panel.hpp:80-82. Clean cut: delete that block + consts + dials; the
rest of `ribbon_frame_tick` (phase clock, head mover, sky resync) stands. Sole
surviving-code anchors: cartridge.hpp:1434-1435. **The GPU dialect survives**:
`field_sum/field_pair` (world.wgsl:2230-2330, `FIELD_SUBSCRIBERS=296` :2246)
drives surviving cube/agent/sphere behavior — it must not ride a ribbon cut;
the beacon writer (cartridge.hpp:891-917) is shared with it. Amendment: the
hypothesized span across cube_behaviors/agents/spawn_engine is **shared**
readback/tier/occupier infrastructure that avoidance only reads
(agents.hpp:177-189; spawn_engine.hpp:445/526; mirror reconcilers
spheres.hpp:253-271, cube_behaviors.hpp:1296-1316) — do not touch it in the
cut.

**Organ E — JSX-facing hooks: no runtime channel exists.** Exhaustive hunt
(boundaries: `designer|paste|jsx|tools/` case-insensitive across
the_board/console/harness; `import` in the_board = 0; runtime-channel battery
`clipboard|socket|localhost|CreateNamedPipe|recv(|listen(` = metaphor comments
only; file-I/O hunt = shader ifstream + paintings scan, neither serving
tools/): the designers' output enters the tree **only as frozen pasted
constants** — e.g. pawn_figures.hpp:21-23 "transcribed verbatim from
7t_pawn_designer.jsx" (WGSL twins world.wgsl:12081-12114) — which survive as
data (SAFE). control_panel.hpp is constexpr rests ("still a rebuild", :35-37);
demo.hpp/matrix.hpp are compile-time token-paste doors pinned by golden
static_asserts. `src/tools/*.jsx` can die with **zero in-tree snips**.

---

## DEVIATIONS FROM THE HANDOFF (reported, per the STOP protocol)

1. **Doc home**: LEDGER_1's product is `audit/LEDGER_1_REPORT.md`, not in
   `src/docs/` — this ledger co-locates at `audit/PORT_0_SEAM_LEDGER.md` (the
   handoff invited exactly this correction).
2. **`glaw1`** names the compile gate (cartridge.hpp:5-8), not a CMake target;
   the build is target `incubator_dual`, preset `the-board-full`.
3. **Expected names that do not exist in the tree** (NAME LAW, reported not
   bound): FXC-workaround toggles (none — H4), `GetMappedRange` (tree:
   `GetConstMappedRange`), readback state `IN_FLIGHT` (tree: `MAPPING`),
   `glfwGetTime` (tree: `std::chrono`), `RequestAdapter/RequestDevice` (tree:
   synchronous dawn::native), avoidance-named machinery (tree: THE FIELD),
   `RequiredLimits` type (tree: `wgpu::Limits` + `requiredLimits` member).
4. **"Bind-group law says three groups"**: LAWS.md contains no such sentence;
   the three-group fact is authored in binding_registry.hpp and mirrored in
   world.wgsl — confirmed in code (H5).
5. **Exe-relative runtime paths**: true for `assets/`, **false for the
   shader** — the compiled loader reads world.wgsl from the source tree; the
   `cartridges/` POST_BUILD copy is dead for it (H7).
6. **H7 as stated is false** — verdict FALSIFIED with the three legs above;
   the production leg (transition-time painting rotation) reshapes PORT_1's
   fetch manifest.

## RULINGS PORT_1 / CUT_1 NEED FROM JEAN

- **F8 / PointHost::RIBBON**: cut with F-keys, or survive as navigation?
- **FIELD scope**: CUT_1 cuts the ribbon-CPU dialect only, or more? The GPU
  dialect feeds surviving entities either way.
- **F5 / beacon**: cut together, re-pin `floater_coordination`, or leave
  permanently-inert code?
- **Camera-host readback**: if retired camera modes are already gone but
  `PointHost::CAMERA` lives, the third readback machine survives — confirm.
- **Paintings fetch strategy**: prefetch all 57 (9.07 MB) vs boot-32 + lazy
  fetch at transitions.
- **Hot-reload affordance in the browser**: delete, or dev-server push?
- **Post-cut stub shape**: null AnalysisSignal (all couplings disable) vs a
  browser-side source serving the 12 names (+ advancing beat clock).

---

## THE FINDINGS TABLE

Every finding, tagged. TAG: **SEAM** = platform edge PORT_1 must re-stitch;
**CUT** = dies in CUT_1; **SAFE** = carries as-is. Counts state their
boundaries inline; line numbers are HEAD `f475ffc`. Rows grouped by
hypothesis; adversarial-pass corrections are folded in and marked *(corrected)*
where a census number changed.

| FILE | SITE | COUNT (with boundary) | TAG | NOTE |
|---|---|---|---|---|
| **H1 — TIME** — verdict **CONFIRMED** | | | | |
| src/incubator_dual.cpp | main() frame loop, while (console.running()), lines 205-265 | 1 top-level frame loop / 6 loop-carried locals | grep 'glfwWindowShouldClose\|running\(\)' across src: qualified-call matches only console.hpp:414-415 (definition), console.hpp:22/371 (comments), incubator_dual.cpp:205, the_lab.cpp:572 (out of scope); locals counted = objects declared in main() before line 205 and read or mutated inside 205-265 | SEAM | The only frame loop in the compiled build. Loop-carried locals: console (162), analysis (170), render (180), watcher (196), reload_frame_counter (201), queue (202). Per-frame-only: dt (206), event (225), encDesc/encoder (255/256), cmdDesc/commands (260/261). Body extracts as frame(dt) with that 6-item context; in browser the loop shell becomes requestAnimationFrame. *(corrected: frame body is not platform-pure — render.update performs disk I/O on gallery-commit/transition frames; see verify-add row.)* |
| src/console/console.hpp | Console::begin_frame(), 374-398 | 1 OS event pump / 1 clock read / 1 resize check per frame | grep 'glfwPollEvents\|glfwWaitEvents\|glfwGetTime\|sleep_for\|Sleep\s*\(\|usleep\|WaitForSingleObject\|std::this_thread' across src, in-closure call-sites only | SEAM | glfwPollEvents at 375 is the ONLY OS event pump in the entire closure. dt from std::chrono::high_resolution_clock::now() at 393-394 against lastTime_ (member 618, seeded init:88). Resize: glfwGetFramebufferSize 379 -> surface_.Configure 388 + createDepthBuffer 389. No glfwGetTime, no sleep anywhere in the frame path. Browser: rAF timestamp replaces the clock, DOM/ResizeObserver replaces poll+resize. |
| src/console/console.hpp | Console::present() 410-412 + presentMode Fifo 349 | 1 present call per frame | exact-token 'surface_.Present()' — single occurrence in closure | SEAM | surface_.Present(); pacing is the Fifo vsync block (349) — the loop has NO sleep; frame throttling comes entirely from Present. Browser: present is implicit at rAF return, pacing from rAF itself. |
| src/console/console.hpp | Console::running() 414-416, acquire_surface_texture() 400-408, accessors 553-566 | running=1 def; acquire=1 def | read of full file; exact member-function definitions | SEAM | running(): window_ && !glfwWindowShouldClose(window_). acquire: GetCurrentTexture + status check (SuccessOptimal/SuccessSuboptimal) + backbuffer_=CreateView; main() skips the frame (continue, incubator_dual.cpp:252) on failure. device() 553, queue() 554, backbuffer() 555, depth_view() 556, color_format() 557, depth_format() 558, aspect_ratio() 563-566. All map 1:1 onto browser WebGPU (getCurrentTexture per frame). |
| src/console/console.hpp | GLFW input producers: callbacks 112-159, inject_*/feed_cursor 430-497, cursor grab 519-546 | 5 GLFW callbacks installed; 4 inject_* producers; 3 cursor-state members | grep 'glfwSet.*Callback' within console.hpp (key 112, cursorPos 133, windowFocus 142, mouseButton 149, scroll 155); producer methods counted by definition | SEAM | Producer side of input. ESC->request_close (117-120) and KP_MULTIPLY->toggle_cursor_grab (125-128) are consumed at console level, never reach cartridges. feed_cursor (460-471) turns absolute positions into deltas via lastCursorX_/lastCursorY_/cursorPrimed_ (626-628); Pointer Lock gives deltas natively in browser. Cursor grab: glfwSetInputMode GLFW_CURSOR_DISABLED + RAW_MOUSE_MOTION (536-544) -> Pointer Lock API. |
| src/incubator_dual.cpp | hot-reload block 208-222 + FileWatcher 83-117 + watch wiring 196-198 + reload_frame_counter 201 | 1 filesystem stat per 30 frames; 2 cout sites (216 gated off by default, 219 on-change) | exact lines; stat = std::filesystem::last_write_time(path_, ec) at line 103, reached via watcher.check() at 218; gate = if constexpr (t7::INSTRUMENTS.watcher_ticks) at 215 | CUT | Dev machinery: mtime-stat trigger has no browser analog; the 30-frame counter exists only to rate-limit the stat; the flushed progress dot is already compile-time off (T7_INSTRUMENTS default off -> instruments.hpp:68 all-false, folded at 82-83). render.reload_shaders() (callee, 220) is a cartridge method and survives — only the harness trigger dies. |
| src/incubator_dual.cpp | input-routing block 225-240 + is_music_key 143-146 | 1 range-for over events; 1 dead branch (analysis.on_input, 230) | exact lines; deadness by is_music_key body '(void)key; return false;' at 144-145 | SEAM | Consumption side is platform-agnostic already (InputEvent, input_event.hpp:18-37, no GLFW types) and ports verbatim; the seam is the producer (see console.hpp findings). Routing degenerates to everything->render.on_input; analysis.on_input edge is dead code. Esc/pointer-door semantics move to browser chrome (Esc exits Pointer Lock by browser fiat). |
| src/cartridges/the_board/cartridge.hpp | MapAsync readbacks 1178, 1221, 1256, 1976 with CallbackMode::AllowSpontaneous 1180, 1223, 1258, 1978 | 4 MapAsync sites / 0 device tick or ProcessEvents calls in closure | grep 'ProcessEvents\|WaitAny\|\.Tick\(\|MapAsync' and 'CallbackMode::' across src, in-closure hits only | SEAM | The loop never ticks the device; Dawn native fires AllowSpontaneous callbacks from internal threads. Browser WebGPU resolves map callbacks on event-loop turns — the rAF frame() yields naturally so no pump must be invented, but AllowSpontaneous availability/semantics under Emscripten must be verified in PORT_1. TIME-relevant: async completion timing shifts from mid-frame-possible to between-frames. |
| src/external/RtMidi.cpp | MidiOutWinMM sysex unprepare retry, line 3187 | 1 compiled while-loop (of 12 total in file) | grep 'while\s*\(' in RtMidi.cpp = 12 sites; __WINDOWS_MM__ region delimited by '#if defined(__WINDOWS_MM__)' at 2644 and '#endif  // __WINDOWS_MM__' at 3209; only 3187 falls inside | CUT | COMPILED: 'while (MIDIERR_STILLPLAYING == midiOutUnprepareHeader(...)) Sleep(1);' — blocking sleep-retry, but on the MIDI-OUT sysex path; harness constructs only RtMidiIn (midi_port.hpp:44) so it is unreachable at runtime. Dies with MIDI intake in CUT_1 regardless. |
| src/external/RtMidi.cpp | textual-only loops: 1069, 1694 (CoreMIDI); 1821 (ALSA while(data->doInput) thread pump), 2081, 2087, 2599 (ALSA); 4197, 4284, 4420, 4505 (JACK, 4505 sched_yield spin); 5115 (Android, usleep(2000) at 5117) | 11 textual while-loops; 3 textual pthread_create sites (2238, 2300 ALSA; 4958 Android) | same 12-site while grep minus the 1 site inside the __WINDOWS_MM__ region 2644-3209; pthread grep 'pthread_create\|CreateThread\|std::thread\|_beginthread' in RtMidi.cpp | CUT | NOT COMPILED under __WINDOWS_MM__-only build — no thread-creating or pumping code from these branches exists in the binary. The compiled WinMM input path creates no thread itself: events arrive on the winmm driver callback thread via midiInOpen(..., CALLBACK_FUNCTION) at 2857-2861. |
| src/sources/midi_port.hpp | MidiPort ring drain poll() 121-133; producer push() 189-195 on RtMidi callback thread 158-187 | 1 bounded drain loop (while at 125); ring 256 slots, SPSC atomics 153-154 | grep 'while\s*\(' in midi_port.hpp = 1 site at 125; bound = 'read != write && count < max_out' | CUT | The build's only cross-thread producer/consumer pair: WinMM callback thread pushes, frame thread drains via canvas.hpp:155 'port_.poll(beat, ev, 256)' (MidiPort owned at canvas.hpp:668). Not a frame loop. Dies with MIDI intake in CUT_1; Web MIDI (main-thread events) would replace both sides. *(corrected: not the only crossing — transport.hpp:86-89 atomics carry the DAW beat clock; see H6.)* |
| src/cartridges/the_board/surface/patch_system.hpp | insertion-sort shifts 281, 286, 676 | 3 | grep 'while\s*\(' in file = 3 sites, all of form 'while (j > 0 && ...)' bounded by loop index | SAFE | Per-frame inner sorts; no platform calls, no blocking. |
| src/cartridges/the_board/surface/tile_world.hpp | evict_distant_tiles cache walk, 228 | 1 | grep 'while\s*\(' in file = 1; 'while (it != tw.tileCache_.end())' bounded by container size | SAFE | Bounded container iteration with erase; portable. |
| src/cartridges/the_board/bodies/ribbon.hpp | history catch-up, 782 | 1 | grep 'while\s*\(' in file = 1; 'while (t - hd.hist_time >= RibbonHead::HIST_DT)' bounded by elapsed sim time | SAFE | Fixed-cadence catch-up over sim time; portable. (File is otherwise in ribbon-machinery CUT territory, but the loop form itself is benign — CUT_1 scoping of ribbon.hpp is a different port's question.) |
| src/cartridges/the_board/bodies/cube_behaviors.hpp | zoetrope beat-tick catch-up, 1143 | 1 | grep 'while\s*\(' in file = 1; 'while (cbs.last_tick_beat + ZOETROPE_TICK_BEATS <= t_beats)' bounded by beat progress | SAFE | Sim-time catch-up loop; portable. |
| src/cartridges/the_board/bodies/gallery.hpp | disk-image cursor retry, 1666 | 1 | grep 'while\s*\(' in file = 1; 'while (attempts < manifest_size)' explicit attempt bound | SAFE | Bounded retry over manifest; portable. |
| src/cartridges/the_board/machine/spawn_engine.hpp | census insertion-sort shift, 845 | 1 | grep 'while\s*\(' in file = 1; 'while (j > 0 && ...)' index-bounded | SAFE | Inner sort; portable. |
| src/musical/stream_data.hpp | completed-ring prune_before, 278 | 1 | grep 'while\s*\(' in file = 1; 'while (count > 0)' with break, bounded by ring count | SAFE | Bounded ring prune; loop form portable (whether stream_data itself survives CUT_1 intake scoping is outside H1). |
| src/external/stb_image.h | decode loops throughout: for(;;) at 4312, 5095, 6718, 6842, 7178, 7305, 7550; do{ at 2077, 2231, 2309, 2364, 4204, 4489, 6397, 6983, 7452; 45 while sites | 45 while / 7 for(;;) / 9 do-while | grep 'while\s*\(' count per file; grep 'for\s*\(\s*;\s*;' content; grep 'do\s*\{' content — all in stb_image.h | SAFE | All data-bounded parse/decode loops in a load-time image library; no OS pump, no sleep. stb_image.cpp is 2 lines (STB_IMAGE_IMPLEMENTATION + include). Compiles under Emscripten unchanged. |
| src/console/console.hpp | timing state: lastTime_ member 618, seeded init:88, read/written begin_frame 393-395 | 1 loop-control clock (high_resolution_clock); in-frame-body clocks are measurement-only | grep 'high_resolution_clock\|steady_clock\|system_clock\|chrono' across src, in-closure call-sites; loop-control = feeds dt; measurement = cartridge.hpp 1140-2022 steady_clock meter (INSTRUMENTS-gated, default off) + one-shot boot/reload prints cartridge.hpp 420-586, renderer.hpp 150-356, 1285-1289 | SEAM | The single clock the loop depends on. std::chrono works under Emscripten, but dt should come from the rAF timestamp at the seam; cartridge-internal chrono measurement sites are SAFE and mostly compile-time off. |
| src/the_lab.cpp | second frame loop at 572 (while (console.running())) | 1 — OUT OF SCOPE | grep 'glfwWindowShouldClose\|running\(\)' across src; the_lab.cpp explicitly excluded from incubator_dual TU set | SAFE | Recorded only to close the census: it exists textually in the repo but is not compiled into the glaw1 target, so H1's 'exactly one' claim holds for the build. |
| src/cartridges/the_board/bodies/gallery.hpp | mid-frame disk I/O: lazy load 1135 → scan 1569/1604 + stbi_load 1507/1511; transition path cartridge.hpp:1022 → rotate_authored_staging 1646 → stbi_load | up to 32 image decodes on the frame thread per gallery-commit / world transition | verify add: grep rotate_authored_staging\|load_authored_image_to_staging\|stbi_load\|std::ifstream\|fopen\s*\( and std::filesystem over src minus external/docs/tools | SEAM | The frame body hits the real filesystem on gallery-commit and world-transition frames. frame(dt) extraction needs an FS seam (fetch/preload) for these reads. Cross-listed under H7 leg 3. |
| src/cartridges/the_board/realization/world.wgsl | loop-construct sweep (whole file) | 0 while / 0 loop{} constructs | verify add: grep while\s*\( = 0 and \bloop\s*\{ = 0 over world.wgsl; all iteration is bounded for( | SAFE | No GPU-side unbounded loop; no shader-side time source beyond CPU-fed uniforms (dt at world.wgsl:8657). |
| src/core/clock.hpp | dt clamp at :34 — DEAD CODE, in no closure | 0 include sites | verify add: grep #include.*clock\.hpp\|core/clock over src = 0; grep std::clamp.*dt\|MAX_DT over closure = 0 | SAFE | No dt clamp exists anywhere in the compiled build — begin_frame returns the raw chrono delta. The dormant Clock has the exact clamp the rAF world wants (tab-suspend delivers huge dt). PORT_1 note. |
| **H2 — ASYNC** — verdict **CONFIRMED** | | | | |
| src/cartridges/the_board/cartridge.hpp | phase_witness_harvest R1: agent/floater/camera readback MapAsync, 1176-1280; state enums 280-295 | 3 of 4 total | "\.MapAsync\(" call-with-paren over src/, 4 hits total, all this file (1178, 1221, 1256, +1976 meter) | SAFE | RUNTIME per-frame. Canonical floater grammar: IDLE->CopyBufferToBuffer(COPIED)->MapAsync(MAPPING, MapMode::Read, AllowSpontaneous, gen-captured lambda)->Success: gen==world_gen ? GetConstMappedRange+consume : drop; Unmap unconditionally; ->IDLE. True state names IDLE/COPIED/MAPPING (not IN_FLIGHT). Grammar itself is web-shaped; depends only on main-loop yield for callback delivery (see incubator_dual.cpp SEAM). |
| src/cartridges/the_board/cartridge.hpp | render() frame meter readback: MapAsync 1976, frame-close resolve/copy 2035-2050, comment 1963-1972 | 1 of 4 MapAsync; 1 ResolveQuerySet (2039) | "\.MapAsync\(" call-with-paren (hit 1976); "ResolveQuerySet" appears only here in closure | SAFE | RUNTIME, grammar-variant self-declared at 1964: 'Mirrors the floater readback grammar exactly... No world_gen capture: timing rows are world-agnostic.' Entire block under if constexpr(INSTRUMENTS.frame_meter) — T7_INSTRUMENTS=off default folds it out of glaw1; buffers also not created (state.hpp:3332-3348). SKIP-IF-BUSY law quoted at 2033-2034. |
| src/cartridges/the_board/cartridge.hpp | phase_witness_capture R11 staging copies 1550-1576; meter copy 2041; TEARDOWN stale-cancel gate 976-992 | 4 CopyBufferToBuffer to MapRead staging (1551, 1559, 1571, 2041) | "CopyBufferToBuffer" token in this file, code lines only | SAFE | Copy-arm half of the grammar; each guarded state==IDLE (skip-if-busy). Camera copy additionally gated point_.host==CAMERA (1569). world_gen++ at 976 plus COPIED->IDLE cancel at 987-992 is the P5 stale-callback guard; MAPPING deliberately left in flight (983-986). |
| src/cartridges/the_board/cartridge.hpp | callback consume sites: GetConstMappedRange 1191/1230/1264/1982; Unmap 1212/1241/1275/2008; MapAsyncStatus 1181-1182/1224-1225/1259-1260/1979-1980 | 4 / 4 / 8 | "GetConstMappedRange\(" call-with-paren; "\.Unmap\(" call-with-paren; "MapAsyncStatus" token — src-wide, all hits this file | SAFE | NAME correction: hypothesis boundary "GetMappedRange\(" scores 0 — the tree exclusively uses GetConstMappedRange (read-only). Exactly one Unmap and one Success-test pair per readback; no stray consumer. |
| src/console/console.hpp | initWebGPU()/initSurface() boot: dawnProcSetProcs 165, EnumerateAdapters 170, CreateDevice 312, GetCapabilities 341, Configure 351 | 0 wait loops; 4 synchronous native entry points | "EnumerateAdapters\|CreateDevice\(\|dawnProcSetProcs\|dawn::native" token; "RequestAdapter\|RequestDevice" token = 0 code hits; "WaitAny" = 0; "ProcessEvents" = 0 | SEAM | BOOT. No spin/future/WaitAny exists because boot never goes async at all — dawn::native synchronous APIs. STRANGER to browser WebGPU: PORT_1 must replace with async requestAdapter/requestDevice + surface-from-canvas. surface_.Present() (411) also has no web equivalent (implicit present); GetCurrentTexture (401) and Configure-on-resize (388) map to canvas context. |
| src/incubator_dual.cpp | main() frame loop 205-265; submit 262; hot-reload check 209-222 | 1 Submit (262); 0 waits | "\.Submit\(" call-with-paren src-wide = 3 (this, gol_zones.hpp:686, excluded the_lab.cpp:655); "glfwWaitEvents" = 0 | SEAM | RUNTIME. Loop never blocks on GPU (glfwPollEvents only, fire-and-forget Submit, Present). But while(console.running()) never yields to a browser event loop — with AllowSpontaneous the native build needs no pump, on web MapAsync callbacks resolve only between frames, so PORT_1 must restructure to emscripten main-loop callback. That restructure alone makes all four readbacks deliver. |
| src/cartridges/the_board/bodies/gol_zones.hpp | flush_zone_derive_requests, 660-689; hidden submit 686 | 1 Submit | "Submit\(" call-with-paren within src/cartridges/the_board | SAFE | RUNTIME. Own encoder + queue.Submit before host submit; pure fire-and-forget, no wait, no readback. Web-compatible as-is (multiple submits per frame are legal). |
| src/cartridges/the_board/realization/renderer.hpp | reload() 1211-1217; loadShader 1229-1291; makeComputePipeline funnel 176-188 (CreateComputePipeline 185); CreateRenderPipeline sites 1626, 1653, 1686, 1824, 1886, 1940, 1991, 2019, 2269, 2475; sync declaration 2240-2242 | 10 CreateRenderPipeline / 1 CreateComputePipeline call sites; 0 async variants | "CreateRenderPipeline\(" and "CreateComputePipeline\(" call-with-paren in this file; "CreateRenderPipelineAsync\|CreateComputePipelineAsync\|GetCompilationInfo" token = 0 code hits src-wide | SEAM | BOOT + RUNTIME(watcher-triggered reload via incubator_dual.cpp:218-221 -> cartridge.hpp:2112). All pipeline creation synchronous ('CreateRenderPipelineAsync is absent repo-wide' — tree's own comment, 2242) — blocks calling thread on compile, but synchronous-returning create* also exists in browser WebGPU, so it carries. The real seam is loadShader's blocking std::ifstream of world.wgsl (1231-1252) — becomes fetch/preload on web. Not a GPU fence wait. |
| src/cartridges/the_board/realization/state.hpp | createBuffers readback stagings 3297-3348; accessors 2916-2988; upload funnel (WriteBuffer/WriteTexture) | 79 .WriteBuffer( / 1 .WriteTexture( (2385); 4 MapRead stagings + 1 meter-gated | "\.WriteBuffer\(" and "\.WriteTexture\(" call-with-paren per file; "mappedAtCreation\|MapMode::Write" token = 0 src-wide | SAFE | All uploads are fire-and-forget queue writes — fully web-compatible; zero map-for-write, zero mappedAtCreation. Stagings are CopyDst\|MapRead only. Gallery painting upload = WriteTexture 2385 + CopyTextureToTexture snapshot 3037 (and gallery.hpp:1498) — GPU->GPU, never read back. |
| src/cartridges/the_board/realization/render_passes.hpp | frustum indirect copy 271, spot VP copy 289, WriteBuffer 139 | 2 CopyBufferToBuffer (GPU->GPU) / 1 .WriteBuffer( | "CopyBufferToBuffer" token; "\.WriteBuffer\(" call-with-paren | SAFE | RUNTIME. Both copies are GPU->GPU (compute->indirect because 'Dawn D3D12 can't share Storage\|Indirect'; staging->vp slot per light) — not readbacks, no waits. patch_system.hpp:187 is the same species (staging->params). |
| src/external/RtMidi.cpp | MidiOutWinMM::sendMessage sysex unprepare loop, 3187 (region __WINDOWS_MM__ 2644-3209 = COMPILED) | 1 compiled busy-wait | "Sleep\|usleep\|nanosleep\|sched_yield\|pthread_cond\|sem_wait\|WaitFor" over the file: 3187 compiled; 4506 (JACK) and 5117 (Android) textual-only | CUT | while (MIDIERR_STILLPLAYING == midiOutUnprepareHeader(...)) Sleep(1); — the ONLY busy-wait in the compiled closure, and it waits on WinMM MIDI-out, not Dawn. MIDI intake dies in CUT_1. Textual-only extras: UWP FindAllAsync .get() 3407 + async.wait_for(3s) 3491 (region 3225-3971, not compiled); UWP std::mutex sites 3341-3923 not compiled; compiled WinMM sync is CRITICAL_SECTION 2690/2765/2920 (short locks, no waits). |
| src/cartridges/the_board/realization/state.hpp | ribbonReadbackStaging_ — decl 1818, create 3318-3324, accessor 2937, validity 3419, LATENT note 3273 | 0 copies into it, 0 MapAsync on it | "ribbon_readback_staging\|ribbonReadbackStaging_" token src-wide (4 hits, all creation/accessor); cross-checked against the 4-hit MapAsync census | CUT | Dead readback: MapRead staging allocated every boot, never armed, never mapped. Tree marks it droppable (LATENT[gate-a-shared] ribbon, 3273). Ribbon machinery is CUT_1 inventory — this buffer goes with it. |
| src/core/instruments.hpp | INSTRUMENTS dial 46-94; cost note 15-23 | frame_meter=false at default T7_INSTRUMENTS=off (73-83) | exact-line read; build fact T7_INSTRUMENTS=off | SAFE | Confirms the meter readback ('MapAsync, one readback in flight forever', line 18) is a dial cost that folds out of the default glaw1 build via if constexpr. Web port of the meter additionally rides timestamp-query feature availability (console.hpp:283, state.hpp:3332-3333) — degrades to CPU rows, already handled. |
| src/console/console.hpp | SetUncapturedErrorCallback, 257-261 (initWebGPU) | 1 — the only async callback registration outside the 4 MapAsync sites | verify add: token PopErrorScope\|PushErrorScope\|UncapturedError\|DeviceLost\|LoggingCallback, non-external = 1 hit | SEAM | BOOT. Maps to device.onuncapturederror / the Emscripten descriptor callback. Prints per error with no rate limit (see H7 console note). |
| src/console/console.hpp | synchronous adapter info queries: GetLimits 267, GetFeatures 298-299, HasFeature 306/308 | 4 call sites | verify add: read of initWebGPU 255-322 | SEAM | BOOT. Same boot-stranger class as EnumerateAdapters/CreateDevice; trivial on web (adapter.limits/features are sync properties). |
| **H3 — SKIN** — verdict **CONFIRMED** | | | | |
| src/external/RtMidi.cpp | WinMM OS-header includes, lines 2651-2652 (region #if defined(__WINDOWS_MM__) 2644 → #endif 3209) | 2 | include-directive regex ^\s*#\s*include\s*[<"](windows\.h\|winsock\|mmsystem\.h\|timeapi\.h\|winrt/\|wrl\|dsound\|xinput\|dinput\|shellapi\|shlobj\|commdlg), case-insensitive | CUT | <windows.h> + <mmsystem.h>; COMPILED (build defines __WINDOWS_MM__). The only compiled OS-header includes in the entire closure. MIDI intake = CUT_1; if intake survived, this would be the RtMidi platform seam. |
| src/external/RtMidi.cpp | UWP OS-header includes, lines 3233-3240 (region __WINDOWS_UWP__ 3225-3971) | 8 | same include-directive regex as above | SAFE | windows.h + 7 winrt/* headers; TEXTUAL ONLY, branch not compiled under this build. Dead weight for the port. |
| src/console/console.hpp | GLFW includes + native-expose defines, lines 39-48 | 2 includes / 3 defines | include-directive ^\s*#\s*include\s*[<"]GLFW/glfw3(native)?\.h and token GLFW_EXPOSE_NATIVE_\w+ (-o) | SEAM | glfw3.h @39; GLFW_EXPOSE_NATIVE_WIN32 @42 (#if _WIN32), _X11 @44, _COCOA @46; glfw3native.h @48. windows.h reaches console.hpp TRANSITIVELY through glfw3native.h — no direct windows.h include anywhere in project code. |
| src/console/console.hpp | initSurface() HWND→wgpu Surface, lines 324-354 | 1 Win32 call (GetModuleHandle @329) / 1 glfwGetWin32Window @328 | token \bGetModuleHandle\b and token glfwGetWin32Window | SEAM | THE surface-creation seam: wgpu::SurfaceSourceWindowsHWND{hwnd=glfwGetWin32Window(window_), hinstance=GetModuleHandle(nullptr)} → surfaceDesc.nextInChain → CreateSurface @338; format=caps.formats[0] @343, Fifo+Opaque configure @345-351. Emscripten replaces with SurfaceSourceCanvasHTMLSelector. Linux branch @331-335 not compiled under MSVC. |
| src/console/console.hpp | GLFW call sites, whole file (94-584) | 27 code call sites, 21 distinct fns (19 compiled; glfwGetX11Display/Window in dead __linux__ branch); +1 comment hit @422 | call-with-paren glfw[A-Z]\w*\( (-o per line), comment at 422 excluded by inspection | SEAM | Distinct: glfwInit, WindowHint, CreateWindow, Terminate(x2), SetWindowUserPointer, SetKeyCallback, GetWindowUserPointer(x5), SetCursorPosCallback, SetWindowFocusCallback, SetMouseButtonCallback, SetScrollCallback, GetWin32Window, [GetX11Display, GetX11Window], PollEvents, GetFramebufferSize, WindowShouldClose, SetInputMode(x2), RawMouseMotionSupported, DestroyWindow, SetWindowShouldClose. The complete window/input/pump skin. |
| src/console/console.hpp | message pump: begin_frame(), line 375 | 1 | exact call glfwPollEvents() in code (line 422 duplicate is a comment) | SEAM | The only OS message pump in the closure. No PeekMessage/GetMessage/DispatchMessage/TranslateMessage anywhere in closure (token \b(PeekMessage\|GetMessage\|DispatchMessage\|TranslateMessage)\w*\b = 0 in-closure). |
| src/console/console.hpp | frame timer, lines 88, 393-395, 618 | 4 | qualified token std::chrono:: (count mode) | SEAM | Frame dt = std::chrono::high_resolution_clock (NOT glfwGetTime — zero glfwGetTime( sites in closure). Portable as-is; seam only in that Emscripten frame pacing moves to requestAnimationFrame. |
| src/console/console.hpp | input→InputEvent producers/consumer, lines 112-159 (callbacks) + 430-507 (inject_*/input_events) + 519-546 (cursor grab) | 5 callback registrations / 5 inject_ producers | call-with-paren glfwSet\w*Callback\( + method-name inspection | SEAM | GLFW callbacks → inject_key_event/feed_cursor(delta-ification)/inject_mouse_move/inject_mouse_button/inject_scroll → std::vector<InputEvent> @621; ESC and KP_MULTIPLY handled inside console (never reach cartridges). Cursor grab: glfwSetInputMode GLFW_CURSOR_DISABLED + GLFW_RAW_MOUSE_MOTION @536-543. |
| src/incubator_dual.cpp | #include <GLFW/glfw3.h>, line 74 | 0 uses | call-with-paren glfw[A-Z]\w*\( and token \bGLFW_\w+ — both zero matches in file | SAFE | VESTIGIAL include. The nominal harness holds no GLFW/Win32 skin at all; delete the include, nothing changes. |
| src/incubator_dual.cpp | FileWatcher hot reload, lines 83-117 + wiring 196-198, 209-222 | 3 std::filesystem calls (87, 88, 103) | qualified call std::filesystem::\w+\s*\( | SEAM | Hot-reload stat of render.shader_path() every ~30 frames. std::filesystem on Emscripten needs a virtual FS or the feature dies; likely replaced by dev-server push or removed for the browser. |
| src/incubator_dual.cpp | analysis.initialize("assets"), line 171 | 1 literal | string-literal grep "assets (exact-line match at 171) | SAFE | RUNTIME call but the literal is DISCARDED by the receiver: canvas.hpp:103-104 '(void)asset_path; // no asset to load; the DAW is the source'. No filesystem touch. |
| src/incubator_dual.cpp | main loop, lines 205-265 | 0 platform tokens | token \b(HWND\|HINSTANCE\|DWORD\|WINAPI\|GetModuleHandle)\b = 0; glfw calls = 0 | SAFE | Pure console-API loop (begin_frame/input_events/update/render/present) — ports verbatim once Console has an Emscripten twin; while(running()) becomes the rAF callback body. |
| src/cartridges/the_board/direction/input.hpp | #include <GLFW/glfw3.h> @10 + GLFW_KEY_* uses, lines 168-294 | 1 include / 63 tokens / 27 distinct constants | include-directive regex + token \bGLFW_\w+ (-o, count mode = 63); zero glfw[A-Z]\w*\( calls | SEAM | The ONLY GLFW reach into src/cartridges/**: keycode CONSTANTS only (comment: 'the key codes (unpapered — c6)'). Not a Windows header. GLFW keycodes survive under Emscripten-GLFW, or the seam is a keycode enum in input_event.hpp. Distinct: KP_8, KP_DECIMAL, LEFT/RIGHT_CONTROL, CAPS_LOCK, 0,2,3,4,5,6,7,8, W,A,S,D,V, F4-F8, LEFT/RIGHT_BRACKET, KP_ADD, KP_SUBTRACT. F4-F8 rows = F-key dynamics = CUT_1. |
| src/cartridges/the_board/contracts/wgpu_fwd.hpp | whole file, lines 1-17 | 0 includes of any kind | file read in full; include-directive regex = 0 hits | SAFE | Pure forward declarations (namespace wgpu { class Queue; Device; CommandEncoder; ComputePassEncoder; RenderPassEncoder; }). Does NOT include GLFW — hypothesis-text suspicion cleared. |
| src/core/input_event.hpp | whole file, lines 1-40 | 0 platform includes | file read in full | SAFE | The platform boundary DTO: enum Type {KeyDown,KeyUp,MouseMove,MouseButton,Scroll} + int key ('Platform key code (e.g., GLFW_KEY_A)') + char/x/y/button/pressed. Already the seam's currency. |
| src/cartridges/the_board/realization/renderer.hpp | loadShader() path search, lines 1229-1276 (literals @1250, 1251; ifstream @1232, 1256; shader_path() @1219) | 2 path literals / 2 ifstream sites | regex "[^"]*\.(wgsl\|png\|jpe?g\|mid\|midi\|txt\|json\|bin)" + token std::(i\|o)?fstream | SEAM | RUNTIME: std::array<const char*,2> paths = {"../../../src/cartridges/the_board/realization/world.wgsl", "src/cartridges/the_board/realization/world.wgsl"}; winner cached in shaderPath_ → shader_path() → cartridge.hpp:2113 → incubator_dual watcher. Browser twin = fetch/preload of world.wgsl; the search-list contract is the seam. |
| src/cartridges/the_board/realization/renderer.hpp | std::chrono instrumentation, lines 150-153, 333-356, 1285-1290 | 12 | qualified token std::chrono:: (count mode) | SAFE | Boot/reload ms logging (init phases, shader compile). Portable. |
| src/cartridges/the_board/cartridge.hpp | std::chrono boot timing + frame meter, lines 420-424, 502-586, 1140-1143, 1371-1372, 1821-1838, 2019-2022 | 20 | qualified token std::chrono:: (count mode) | SAFE | Boot-phase ms logs (high_resolution_clock) + fps/section meter (steady_clock). Portable. |
| src/cartridges/the_board/bodies/gallery.hpp | authored-painting load + folder scan, lines 1505-1611 (literals @1510 "7t/", 1574 "assets/paintings", 1575 "7t/assets/paintings", 1594 "PAINTING_", 1597 ".jpg"/".jpeg"; stbi_load @1507,1511; fs::exists/is_directory @1580, directory_iterator @1590) | 5 path/filename literals / 2 stbi_load sites / 3 fs call sites | regexes "(assets\|cartridges\|src)[/\\], "7t/, "[^"]*\.(jpe?g\|...)" + call-with-paren stbi_load\w*\( and std::filesystem inspection | SEAM | ALL RUNTIME: scan_paintings_folder (called @1625) walks SEARCH_DIRS, filters PAINTING_*.jpg/.jpeg, manifest → load_authored_image_to_staging (@1636,1674) → stbi_load with "7t/"+path fallback (@1510-1511). The only image I/O and only directory iteration in the closure. Browser: preloaded FS or fetch; stb_image decode itself is portable (see stb_image finding). |
| src/external/stb_image.h | Win32 UTF8 branch, lines 1325-1353 (MultiByteToWideChar extern @1326, calls @1343,1346; _wfopen_s @1350, _wfopen @1353) vs portable fopen @1360 | 7 Win32 decl/call sites (1326,1327,1333,1343,1346,1350,1353), all inert *(corrected)* | token MultiByteToWideChar\|_wfopen\|CreateFileW\|STBI_WINDOWS_UTF8; guard = defined(_WIN32) && defined(STBI_WINDOWS_UTF8) | SAFE | STBI_WINDOWS_UTF8 defined nowhere in the repo (stb_image.cpp = 2 lines: STB_IMAGE_IMPLEMENTATION + include) → build compiles fopen_s @1357 (MSVC branch) *(corrected; fopen @1360 is the non-MSVC fallback Emscripten would compile)*. No OS-header include in stb_image.h (only <emmintrin.h> @726, SSE2 intrinsic). |
| src/external/RtMidi.cpp | compiled WinMM Win32 API calls, region 2644-3209 | 18 midiIn* / 12 midiOut* / 6 CriticalSection / 1 Sleep | call-with-paren \b(midiIn\w*\|midiOut\w*\|timeGetTime\|CreateEvent\w*\|WaitForSingleObject\|EnterCriticalSection\|LeaveCriticalSection\|InitializeCriticalSection\w*\|DeleteCriticalSection)\s*\( (-o), midiInputCallback definitions (994 textual CoreMIDI, 2698 compiled WinMM) excluded as RtMidi's own functions | CUT | midiIn*: 2766,2815,2841,2857,2877,2879,2887,2889,2897,2899,2921,2922,2925,2929,2937,2962,2968,2978. midiOut*: 3018,3031,3037,3047,3071,3087,3107,3109,3169,3178,3187,3201. CritSec: 2765,2767,2805,2827,2920,2940. Sleep(1) @3187. NO timeGetTime/CreateEvent/WaitForSingleObject anywhere in file; timestamps = driver DWORD callback param (2702→2715→2777). DWORD tokens @2687,2702,3089,3090,3193 all compiled-region. MIDI intake dies in CUT_1; textual __WEB_MIDI_API__ branch exists @4522-4833 if ever wanted. |
| src/external/RtMidi.cpp | UWP timer/chrono, lines 3369-3629 (chrono x6; QueryPerformanceFrequency @3517, QueryPerformanceCounter @3572) | 6 chrono / 2 QPC | qualified token std::chrono:: + token \bQueryPerformance\w+\b; region __WINDOWS_UWP__ 3225-3971 | SAFE | TEXTUAL ONLY — the only QueryPerformanceCounter/Frequency in the entire tree, and it does not compile. timeBeginPeriod/timeGetDevCaps: zero anywhere in src/. |
| src/external/RtMidi.h | _WIN32 use, line 47 | 1 | token \b_WIN32\b (count mode over src/: console.hpp=2 @41,326; stb_image.h=3 @1325,1330,1340; RtMidi.h=1; rest imgui/implot out of scope) | SAFE | Selects RTMIDI_DLL_PUBLIC __declspec decoration only; RtMidi.h contains NO OS-header includes — safe for sources/midi_port.hpp:26 to include. |
| src/sources/midi_port.hpp | #include "external/RtMidi.h" @26; RtMidiIn usage @44-146 | 1 include | grep RtMidi over src/sources (content mode) | CUT | The MIDI intake face (RtMidiIn behind MidiPort; callback on RtMidi's thread @156). No OS headers/calls of its own. Dies with intake in CUT_1. |
| src/analysis/canvas_1/canvas.hpp | initialize(), lines 103-148; port literal @145 | 1 non-path literal ("loopMIDI") | string-literal inspection; not matched by any of the three path regexes (not a filesystem path) | CUT | port_.open_by_name("loopMIDI") — DAW virtual MIDI port name, RUNTIME. asset_path param DISCARDED @104. Zero OS/GLFW/chrono tokens in canvas.hpp. |
| src/cartridges/the_board/realization/world.wgsl | whole file (runtime-loaded) | 0 | regex \.(wgsl\|png\|jpe?g\|mid\|midi\|txt\|json\|bin)\|assets\|[A-Za-z]:/ over the file — sole match is the line-1 banner comment | SAFE | No path strings, no OS references. Carries verbatim; its filesystem identity lives in renderer.hpp:1250-1251. |
| src/musical + src/coupling + src/core + src/render + src/analysis (all other closure files) | entire trees | 0 / 0 / 0 | OS-header include regex = 0; token \b(HWND\|HINSTANCE\|DWORD\|WINAPI\|GetModuleHandle)\b = 0; glfw[A-Z]\w*\( = 0; GLFW include regex = 0 (all run over whole src/, closure-filtered) | SAFE | musical/*, coupling/*, core/cartridge_ids+instruments, render/render_cartridge.hpp, analysis/analysis_cartridge+analysis_signal, sources/midi_event+transport: zero platform tokens of any kind. Already portable. |
| src/cartridges/the_board/cartridge.hpp | #include <filesystem> at :94 — vestigial | 0 std::filesystem/fs:: uses in the file | verify add: grep fs::\|std::filesystem in cartridge.hpp hits only the include line | SAFE | Same class as the vestigial GLFW include in incubator_dual.cpp:74. Deletable. |
| **H4 — NATIVE DAWN** — verdict **CONFIRMED** | | | | |
| src/console/console.hpp | dawn header includes, lines 31-38 | 3 includes | include-directive grep `#include\s*[<"](dawn/native/DawnNative\.h\|dawn/dawn_proc\.h\|dawn/common/Version_autogen\.h)[">]`; sweep `#include\s*[<"]dawn` confirms no others in closure | SEAM | DawnNative.h (31), dawn_proc.h (32), Version_autogen.h (34, wrapped in __has_include guard at 33 with T7_DAWN_VERSION fallback 35/37). Only dawn includes in the whole closure. BOOT. |
| src/console/console.hpp | initWebGPU() proc install, line 165 | 1 call (also the only GetProcs hit) | call-with-paren `dawnProcSetProcs\(`; token `GetProcs` | SEAM | dawnProcSetProcs(&dawn::native::GetProcs()) — proc-table install, native-only, BOOT. No browser equivalent. |
| src/console/console.hpp | instance construction + member, lines 168 and 600 | 4 instance_ usages total (168 emplace, 170 arrow, 338 Get, 600 decl) | token `instance_` in scope; qualified `dawn::native::` gives the 4 closure hits 165/170/251/600 | SEAM | std::optional<dawn::native::Instance> instance_ (600) built by bare emplace() (168) — no InstanceDescriptor, no instance toggles/features. Hypothesis's '~600 instance creation' is the DECLARATION; creation is 168. All usages BOOT; zero per-frame. |
| src/console/console.hpp | adapter enumeration, line 170 | 1 | token `EnumerateAdapters` (in-scope) | SEAM | instance_->EnumerateAdapters() with NO options — native-only synchronous multi-adapter API. Browser: single async requestAdapter. BOOT. |
| src/console/console.hpp | Dawn revision banner, lines 178-190 (kDawnVersion at 182) | 1 kDawnVersion use | token `kDawnVersion` | SAFE | Native-only symbol but self-guarded: __has_include (33) + #if T7_DAWN_VERSION (178) already compiles a portable else-branch (188-189) when the header is absent. BOOT. |
| src/console/console.hpp | adapter log + pick scoring, lines 197-253 (BackendType uses 204-215, 247; .Get() bridges 225, 252; nativeAdapter ref 251) | 9 wgpu::BackendType hits; 3 .Get() bridges in this block (225, 242, 252) *(corrected)*; 1 dawn::native::Adapter& ref | qualified token `wgpu::BackendType`; qualified `dawn::native::`; read-verified .Get() sites | SEAM | Multi-adapter scoring (DiscreteGPU+2, D3D12+1, fallback idx 0) is native-only backend selection; wgpu::BackendType spelling is portable-header but only used here. BOOT. Whole block collapses to nothing on web. |
| src/console/console.hpp | device creation, lines 255-322 (CreateDevice 312, MultiDrawIndirect probe 306, limits passthrough 266-269, TimestampQuery 282-286) | 1 synchronous CreateDevice; 1 Dawn-extension enum probe; 0 toggles chained | read of full block; token `DawnTogglesDescriptor` = 0 hits in src/ | SEAM | Synchronous adapter.CreateDevice is native-only (browser: async RequestDevice). wgpu::FeatureName::MultiDrawIndirect (306) is a Dawn native extension enum — query/log only, never requested. requiredLimits = full adapter limits passthrough (269) is portable spelling, browser-policy question for PORT_1. DeviceDescriptor.nextInChain never touched. BOOT. |
| src/console/console.hpp | initSurface() surface chain + CreateSurface, lines 324-338 | 2 nextInChain assignments (330 compiled Win32, 335 textual X11); 1 instance_->Get() bridge (338) | grep `chainedStruct\|nextInChain` -i in console.hpp; read-verified #if branches | SEAM | wgpu::SurfaceSourceWindowsHWND + glfwGetWin32Window + GetModuleHandle (327-330) is the compiled native-only surface source; browser needs canvas-selector source. CreateSurface goes through wgpu::Instance(instance_->Get()). These are the ONLY chained structs in console.hpp — no toggle chaining exists. BOOT. |
| src/console/console.hpp | surface caps/configure + depth buffer, lines 340-364 | 1 GetCapabilities, 1 Configure, caps.formats[0] pick | read of block | SAFE | Portable spellings; only PORT_1 nuance is caps.formats[0] vs getPreferredCanvasFormat. Depth24Plus depth buffer fully portable. BOOT. |
| src/console/console.hpp | present(), line 411 (called from incubator_dual.cpp:264) | 1 — the only .Present( in src/ | call-with-paren `\.Present\(` over src/ | SEAM | PER-FRAME. Exact spelling: surface_.Present() i.e. wgpu::Surface::Present — portable-header spelling, but browser has no explicit present (implicit at rAF return). The single per-frame native-pattern call; needs no-op/rAF seam. Does not contradict 'all CONFIGURATION at boot' but must be seamed. |
| src/console/console.hpp | begin_frame()/acquire, lines 374-408 (Configure 388, GetCurrentTexture 401) | 0 native-only tokens | read of block; `ProcessEvents\(`, `\.Tick\(` = 0 hits in src/ | SAFE | PER-FRAME path is portable webgpu_cpp spelling throughout (glfwPollEvents at 375 is a GLFW seam, out of H4's Dawn scope). Resize reconfigure + surface texture acquire both have direct browser equivalents. |
| src/cartridges/the_board/cartridge.hpp | readback pump: phase_witness_harvest 1175-1280 (MapAsync 1178, 1221, 1256) + frame-meter harvest in render() 1973-2014 (MapAsync 1976) | 4 MapAsync sites, 4/4 with wgpu::CallbackMode::AllowSpontaneous (1180, 1223, 1258, 1978); 0 ProcessEvents, 0 .Tick(, 0 WaitAny in src/ | grep `MapAsync\|WaitAny\|GetMappedRange\|PopErrorScope\|Unmap\(` in cartridge.hpp; grep `CallbackMode` over src/; call-with-paren `ProcessEvents\(` and `\.Tick\(` over src/ = 0 | SAFE | PER-FRAME but portable webgpu.h API. AllowSpontaneous is why NO native pump exists anywhere — Dawn fires callbacks from submit internals; on web the event loop fires them. No per-frame native-only execution in the readback path. |
| src/incubator_dual.cpp | main() frame loop, lines 205-265 | 0 dawn tokens in file | grep `dawn` -i per-file; read of full file (269 lines) | SAFE | Loop is pure portable spelling: begin_frame 206, acquire 251, CreateCommandEncoder 256, Finish 261, queue.Submit 262, console.present() 264 (routes into the console.hpp:411 seam). Boot calls console.init at 163. |
| src/console/console.hpp | shutdown(), lines 574-580 | 0 native Dawn teardown calls | read of block | SAFE | SHUTDOWN is GLFW-only (glfwDestroyWindow/glfwTerminate). dawn::native::Instance dies implicitly via the optional member's destructor — no explicit native site to seam. |
| src/cartridges/the_board/realization/renderer.hpp | FXC comment census, 41 lines (1331; 1358-2473 list in evidence) | 41 — all comments, 0 code | word-bounded case-sensitive `\bFXC\b`; complementary `\bDXC\b\|use_dxc\|\bfxc\b` = 0 in scope; case-insensitive pattern rejected (false-positives on IdxCount) | SAFE | Every hit is a trailing // comment on an if constexpr(ROSTER.x) gate ('FXC skipped when disabled') or prose (1331). No FXC/DXC configuration code exists anywhere in the closure. |
| src/cartridges/the_board/realization/world.wgsl | FXC comments: 14, 15, 2467, 3023, 3117, 3837, 6771, 6831, 7563, 7567, 12790, 12842, 13063 | 13 — all // comments | word-bounded case-sensitive `\bFXC\b` | SAFE | FXC BANNER (14-15) and shader-structure rationale. The WGSL carries the FXC discipline structurally (comments + shape), not via any toggle; the shader text itself ports verbatim. |
| src/cartridges/the_board/realization/state.hpp | FXC comment at 6262; seed_utils.hpp comment at 12 | 1 + 1 — comments | word-bounded case-sensitive `\bFXC\b` (state.hpp 2907/2909/2910 'idxCount' hits from the case-insensitive run were false positives and are excluded) | SAFE | Prose only. Total in-scope FXC census: 56 comment hits across 4 files, zero code hits. |
| src/console/console.hpp | toggle census (whole tree) | 0 DawnTogglesDescriptor, 0 toggle name strings, 0 chained toggle descriptors | token `DawnTogglesDescriptor` over src/ = 0; `use_dxc` = 0; `toggle` -i in console.hpp hits only cursor-grab 126/516/524; nextInChain census shows only surface sources 330/335 | SAFE | NAME LAW finding: the hypothesis's expected FXC-workaround toggles DO NOT EXIST. Nothing to port; also nothing hidden to miss. Toggle-descriptor machinery is absent from instance AND device creation. |
| src/cartridges/the_board/realization/renderer.hpp | wgpu::ShaderSourceWGSL chained via desc.nextInChain, 1278-1286 (CreateShaderModule 1286) | 1 — the only in-scope nextInChain site outside console.hpp | verify add: grep nextInChain\|ChainedStruct -i over src, in-closure | SAFE | Portable webgpu_cpp WGSL shader-source chain — standard WebGPU, carries as-is. Completes the closure-wide chained-struct census. |
| **H5A — LIMITS: device / features / formats** — verdict **CONFIRMED** | | | | |
| src/console/console.hpp | initWebGPU() device descriptor, 255-286 (requiredLimits :269, requiredFeatures :282-286) | 1 feature requested / limits = full adapter (NOT default) | token 'requiredFeatures\|requiredFeatureCount\|requiredLimits\|RequiredLimits' content-grep over src/ minus docs — 4 hits, all these lines; feature spelling wgpu::FeatureName::TimestampQuery | SEAM | wgpu::Limits adapterLimits fetched via adapter.GetLimits then deviceDesc.requiredLimits = &adapterLimits — the browser port must replace this with an explicit limit request. NAME LAW: no wgpu::RequiredLimits type exists; it is wgpu::Limits + member requiredLimits. |
| src/console/console.hpp | TimestampQuery request gate, 282-286 + comment 273-281 | requested unconditionally w.r.t. T7_INSTRUMENTS (gated only on adapter.HasFeature) | qualified token 'wgpu::FeatureName::\w+' -o over src/ minus docs: 6 in-scope sites, 2 distinct names | SEAM | ANSWER to Q2: with T7_INSTRUMENTS=off the feature IS still requested at device creation. Safe for the web build to drop: every consumer re-probes HasFeature and every meter use-site is if-constexpr(INSTRUMENTS.frame_meter)-folded. |
| src/console/console.hpp | MultiDrawIndirect probe, 306 | 1 site, HasFeature log only | qualified token wgpu::FeatureName::MultiDrawIndirect, call-with-paren HasFeature( | SEAM | Never requested, never used for draws; part of the Dawn-native PROBE_1 boot log (adapter enumeration 224-253, feature dump 297-310) that dies with the console seam. |
| src/cartridges/the_board/realization/state.hpp | meter GPU half: members 1820-1835, arming 2966-2984, creation 3332-3348 | 1 CreateQuerySet site; QuerySet count 64 | token 'QuerySet\|Timestamp' case-sensitive over src/ minus docs; creation guarded by meterEnabled_ = INSTRUMENTS.frame_meter && device_.HasFeature(TimestampQuery) at :3332-3333 | SAFE | Compile-folded off in the default build (T7_INSTRUMENTS=off). MeterComputeTsW/MeterRenderTsW derived by decltype from the pass descriptors' timestampWrites members (1753-1756) — portable across Dawn/emdawnwebgpu header generations by construction. |
| src/cartridges/the_board/cartridge.hpp | meter_gpu_ probe 433-437; ResolveQuerySet harvest 2035-2049 | both blocks under if constexpr (INSTRUMENTS.frame_meter) | token 'ResolveQuerySet\|HasFeature' + if-constexpr guard read in context | SAFE | Default build folds these out entirely; no timestamp API reaches the encoder. |
| src/console/console.hpp | surface format acquisition, initSurface() 340-351; accessors 557-558 | colorFormat_ = caps.formats[0] (1 site); PresentMode::Fifo, CompositeAlphaMode::Opaque | token 'caps.formats' exact-line; token 'color_format\(\)\|depth_format\(\)' over src/ minus docs (in-scope consumers: incubator_dual.cpp:183 only) | SEAM | Surface format comes from surface CAPABILITIES, not a constant — on web this becomes navigator.gpu.getPreferredCanvasFormat()/canvas configure. No pipeline hard-codes it. |
| src/incubator_dual.cpp | format handoff, 183 | 1 call: render.init_renderer(console.color_format(), console.depth_format()) | call-with-paren init_renderer( | SAFE | The single conduit for surface color+depth format into the render core (cartridge.hpp:493-511 → renderer.hpp:299-329 + state.hpp initOffscreenResources:2405). |
| src/cartridges/the_board/realization/renderer.hpp | color targets 1593,1854,1905,1961,2442; depth states 1587,1867,1922,1965,2468; shadow depth 2034 | 5 color targets all colorFormat_; 5 depth states all depthFormat_; 1 hard-coded format (Depth32Float shadow) | token 'colorFormat\|depthFormat' content-grep in renderer.hpp; qualified token wgpu::TextureFormat::Depth32Float at :2034 | SAFE | The only hard-coded pipeline format is the shadow pass's Depth32Float — core WebGPU. Everything else follows the surface. |
| src/cartridges/the_board/realization/state.hpp | colorFormat_ default + swap probes: 1788, 2357, 2391; initOffscreenResources 2405-2472 | 3 BGRA8Unorm tokens (0 creations); paintings/staging/exhibition/offscreen-color created in surface format; offscreen depth Depth24Plus :2467 | qualified token wgpu::TextureFormat::BGRA8Unorm -o; creation sites via CreateTexture( grep | SAFE | :1788 is a member default overwritten at :2406; :2357/:2391 are CPU R<->B swap probes that make the painting upload path byte-order agnostic (works for web bgra8unorm or rgba8unorm surfaces). |
| src/cartridges/the_board/realization/state.hpp | texture creations: 3974 (R32F zone life 32x32x8), 4069 (RGBA16F aura 64x64), 4083 (RGBA16F live card), 4096 (R32F ground atlas 256x1), 4112 (RGBA16F patch heightfield 256x256x289), 4131 (RGBA8U cell color 16x16x289), 4153/4170 (D32F shadow+spot 4096x4096) | 22 qualified wgpu::TextureFormat:: tokens in-scope total; 6 distinct format names + runtime surface format | qualified token 'wgpu::TextureFormat::\w+' -o -n over src/ minus docs; creation sites cross-checked with call 'CreateTexture\(' (12 in-scope sites *(corrected)*; 13th hit is imgui, out of closure) | SAFE | Distinct formats: surface(caps), Depth24Plus, Depth32Float, R32Float, RGBA16Float, RGBA8Unorm, BGRA8Unorm(compare-only). All core WebGPU; no sRGB, no stencil, no compressed, no rg11b10. |
| src/cartridges/the_board/realization/state.hpp | storage-texture layout entries 4573-4577, 4583-4587, 4791-4795, 4866-4870, 4906-4910, 4950-4954 | 6 entries: RGBA16Float x3, R32Float x2, RGBA8Unorm x1 — all StorageTextureAccess::WriteOnly | qualified token wgpu::TextureFormat within 'storageTexture.format =' lines; access lines read in context | SAFE | All three storage formats are core storage-capable; no bgra8unorm-storage anywhere (the BGRA-capable painting array is never a storage binding). |
| src/cartridges/the_board/realization/world.wgsl | storage declarations 6140, 6142, 6407, 6419, 6420, 9621 | 6 declarations: rgba16float x3, r32float x2, rgba8unorm x1, all 'write' | regex 'texture_storage' content-grep (NOTE: the tasked regex texture_storage_2d<\w+ misses the 3 _2d_array declarations — finds only 3 of 6) | SAFE | Exact 1:1 match with the six C++ layout entries; no read or read_write storage textures. |
| src/cartridges/the_board/realization/world.wgsl | depth sampling: decls 6129-6131; PCF 3868-3883 (sun 16-tap), 4123/4135 (spot) | 2 texture_depth_2d + 1 sampler_comparison; sun reads textureSampleCompareLevel (16 taps), spot reads textureSampleCompare (4122-4124, 4134-4136) *(corrected)* | regex 'texture_depth_2d\|sampler_comparison' + token textureSampleCompareLevel | SAFE | Depth32Float is comparison-sampled ONLY (sampler compare=Less with Linear filters = hardware PCF, core-legal). WebGPU's ban on FILTERING depth32float is not violated — no non-comparison depth sampling exists. |
| src/cartridges/the_board/realization/state.hpp | UnfilterableFloat bindings 4366 (entity_ground_atlas, VS) and 4500 (zone_life_read, FS); Depth bindings 4474/4484; Comparison sampler binding 4479 | 2 UnfilterableFloat, 2 Depth, 1 Comparison; 13 Float sampleType entries *(corrected: + 4494, 4668)* | token 'sampleType\|UnfilterableFloat' content-grep in state.hpp | SAFE | R32Float consumed only via textureLoad (world.wgsl:5433 et al.) or textureSampleLevel with the NonFiltering nearest sampler (world.wgsl:4729-4731) — float32-filterable feature NOT needed. RGBA16Float filtered sampling is core. |
| src/cartridges/the_board/realization/state.hpp | createSamplers() 4180-4227 | 4 samplers, 0 with maxAnisotropy set | token 'maxAnisotropy' over src/ minus docs: 1 hit total, imgui_impl_wgpu.cpp:855 — NOT in the incubator_dual closure; in-scope count 0 | SAFE | Bilinear/Nearest/Shadow(comparison)/Painting; all ClampToEdge, default aniso=1, no mipmaps. Nothing exotic for the web. |
| src/cartridges/the_board/realization/state.hpp | patch array layer count: Dim::MAX_ACTIVE_PATCHES 81-82; textures 4109-4123, 4128-4142 | 289 layers (= 17^2) on 2 texture arrays | exact-line 'MAX_ACTIVE_PATCHES = PATCH_PREGEN_SIDE * PATCH_PREGEN_SIDE' + desc.size third component at :4110/:4129 + view arrayLayerCount :4119/:4138 | SEAM | EXCEEDANCE 1: 289 > WebGPU default maxTextureArrayLayers (256). Browser device must request >= 289 (adapters commonly grant 2048). |
| src/cartridges/the_board/realization/renderer.hpp | roomComputeLayout pipelines: layout 1332-1342; pipelines 1350 (update_player_agent), 1360 (update_other_agents), 1375 (update_sphere), 1384 (update_cube) | 9 compute-stage storage buffers (g0: 5 = vp_data/agent_state/camera_state/floating_entities Storage + patch_grid RO, state.hpp:4243-4283; g1: 0; g2: 4 = occupier_cmg/occupier_amg/field_head_poses RO + field_forces Storage, state.hpp:5110-5124) | count of BindGroupLayoutEntry with buffer.type in {Storage, ReadOnlyStorage} and Compute visibility, summed over the exact bindGroupLayouts array of the pipeline layout | SEAM | EXCEEDANCE 2: 9 > WebGPU default maxStorageBuffersPerShaderStage (8). Corroborated by state.hpp:4285-4290 ('pushed compute storage buffer count past the 10-per-stage limit'). CUT interaction: g2's field_head_poses/field_forces/field_ribbon are ribbon-field machinery — if ribbon-avoidance dies in CUT_1 this drops to <= 8; recount after CUT_1. |
| src/cartridges/the_board/realization/state.hpp | Render Entity Layout VS storage entries 4319, 4323, 4327, 4347, 4355, 4372, 4377 | 7 VS storage buffers (<= 8 default) / 7 FS storage buffers | count of (RO)Storage entries with Vertex (resp. Fragment) visibility across renderEntityLayout_ + renderTextureLayout_ (the main render pipeline layout, renderer.hpp:1574-1583) | SAFE | At-limit pressure, not over: comments :4386-4388/:4393-4395 call the VS storage cap 'full' (my census says 7 of 8) — one slot of headroom at most; do not add VS storage bindings in the port. |
| src/cartridges/the_board/realization/world.wgsl | @workgroup_size census (29 entry points, 5704-13026) | max 256 invocations ((16,16) at 8722, 8743, 9337); values used: 64 x6, 1 x9, (1,1,1) x4, (8,8,1) x5, (8,8) x1, 32 x1, (16,16) x3 *(corrected multiset)* | regex '@workgroup_size\([^)]*\)' -o | SAFE | Exactly at the 256 default maxComputeInvocationsPerWorkgroup ceiling; within maxComputeWorkgroupSize{X,Y} defaults (256,256). |
| src/cartridges/the_board/realization/world.wgsl | binding index census; max @binding(501) at 9952-9953; groups g0/g1/g2 only | max binding 501 < 1000 default; 3 bind groups <= 4 default | regex '@binding\((\d\d\d)\)' -o for 3-digit indices; regex '@group\((\d)\)' spot-checked (no group >= 3) | SAFE | Sparse-but-legal binding numbering (100s-500s). Uniform sizes all < 64KiB (largest GPUTileGrid 16400 B, state.hpp:710); no dynamic offsets (0 hasDynamicOffset matches); no MSAA (only sampleCount=1 at state.hpp:4100); vertex attrs <= 4; 1 color attachment. |
| src/external/RtMidi.cpp | Timestamp hits: 3582 (__WINDOWS_UWP__ block 3225-<3982) and 4670-4672 (__WEB_MIDI_API__ block 4522-<4843) | 0 compiled / 3 textual lines | token 'Timestamp' case-sensitive; block membership by '^#if defined(' grep (compiled branch __WINDOWS_MM__ = 2644-3209 per build define) | CUT | Both hits are in branches the glaw1 build never compiles. NAME LAW: hypothesis's 'WinRT' branch is spelled __WINDOWS_UWP__ here; the browser-MIDI branch is __WEB_MIDI_API__ (not __EMSCRIPTEN__). MIDI intake dies in CUT_1 regardless. |
| src/console/console.hpp | depth buffer 356-364, member 613; render_cartridge.hpp:155-157; cartridge.hpp:2104-2106 | 3 Depth24Plus declaration sites, 1 creation (console) | qualified token wgpu::TextureFormat::Depth24Plus -o (4 in-scope tokens incl. state.hpp:2467) | SAFE | Console owns the swap-depth (Depth24Plus, RenderAttachment); cartridge's depth_format() override agrees; incubator_dual passes console.depth_format() so the pair cannot diverge in this target. |
| src/cartridges/the_board (8 files) | timestampWrites arming call sites: render_passes.hpp 146/162/176/262/310/337/464; gol_zones.hpp 671/756/766; orbs.hpp 747/765/777/793; gallery.hpp 1413/1444; pawn.hpp 197; patch_system.hpp 196/206; cartridge.hpp 1504 | 22 unconditional arming sites; fold centralized at state.hpp:2956 | verify add: grep timestampWrites\|CreateQuerySet\|QueryType\|ResolveQuerySet over src (lowercase reaches what the census pattern could not) | SAFE | All no-op via meter_arm_alloc constexpr fold (if constexpr (!INSTRUMENTS.frame_meter) return UINT32_MAX). This is the full timestamp seam surface for PORT_1/METER_1. |
| src/cartridges/the_board/realization/renderer.hpp | DrawIndexedIndirect at :723 (draw_patch_terrain_plan_slot), invoked render_passes.hpp:479/484/489 at offsets 0/20/40 | 1 indirect draw site | verify add: grep MultiDraw\|DrawIndirect\|DrawIndexedIndirect over src | SAFE | CORE WebGPU — needs no feature: args reset each frame with firstInstance=0 (state.hpp:2805-2816); cull kernel touches only instanceCount slots (world.wgsl:10069-10081). |
| src/cartridges/the_board/realization/state.hpp | storage-buffer binding sizes, all 74 makeBuffer sites; largest CMG vertex pool ~2 MB (:292, :3786, entry :6114) | 0 exceedances of maxStorageBufferBindingSize (128 MiB) / maxBufferSize (256 MiB) | verify add: makeBuffer size sweep, all sites | SAFE | Every buffer is single-digit MB at most. |
| **H5B — LIMITS: buffers / bind groups** — verdict **CONFIRMED** | | | | |
| src/docs/LAWS.md | L6 THE BINDING-NUMBER LAW, lines 107-134; L7 lines 136-152; L2 item 4 line 46 | 0 occurrences of a 'three groups' statement | grep -i 'three\|group 0\|group 1\|group 2\|bind group' on the file; exact-line reading of L2/L6/L7 | SAFE | L6 names g0/g1 only as its example and states no group count; L2:46 states the working caps 'Storage buffers per stage = 10. Uniform buffers per stage = 12' (FXC/D3D12 working limits, NOT WebGPU defaults). L7 mandates binding-closure resync and says 'Any future port wants generated layouts, not transcribed ones' (152). |
| src/cartridges/the_board/realization/binding_registry.hpp | full file, g0 lines 33-132, g1 139-154, g2 162-175, witnesses 183-186 | 3 group namespaces; 97 named constants (g0=77, g1=14, g2=6); max binding index 501 | qualified-token count of 'inline constexpr uint32_t' declarations per namespace, full read | SAFE | THREE GROUPS CONFIRMED here (registry is L6's single source of truth). g2 = THE ROOM, private to agent/floater compute kernels. Binding index 501 (g0::fc_indirect) = 50.1% of maxBindingsPerBindGroup default 1000. |
| src/cartridges/the_board/realization/world.wgsl | whole file (runtime-loaded shader) | @group(0) x80, @group(1) x14, @group(2) x6, @group(3) x0 | grep -o '@group([0-9])' \| sort \| uniq -c (exact-token) | SAFE | 100 declarations over 97 slots (3 fc_ aliases share slots) — matches registry header comment line 16. No fourth group exists in the shader mirror. |
| src/cartridges/the_board/realization/state.hpp | makeBuffer wrapper 3236-3239; createBuffers() 3241-3421; createMeshBuffers()+helpers 3423-4060 | 6 CreateBuffer( sites + 73 makeBuffer( call sites = 78 buffer objects (76 in default build: meter pair off) | call-with-paren 'CreateBuffer\(' and 'makeBuffer\(' across src, imgui/docs excluded (out of closure) | SAFE | ALL buffer creation lives in state.hpp. Full 78-row size-sorted table in evidence with every constant cited. Largest per class: STORAGE Live Card Scratch 3,276,800 B (:3365); VERTEX Column VB 1,920,000 B (:3785); INDEX Column IB 768,000 B (:3788); UNIFORM-usage Floating Entity Array 54,912 B (:3270); MapRead staging Floating Entity Readback 54,912 B (:3309); QueryResolve 512 B (:3340); Indirect 60 B (:3396). Total 13,889,856 B (13,888,832 with meter pair off) *(corrected sum)* — 1.22% of maxBufferSize at worst. |
| src/console/console.hpp | initWebGPU device acquisition, lines 263-292 | 1 requiredLimits request (= full adapter limits); 0 CreateBuffer sites in file | grep 'RequiredLimits\|requiredLimits\|GetLimits' + 'CreateBuffer\(' per-file | SEAM | deviceDesc.requiredLimits = &adapterLimits with comment 'The default maxStorageBuffersPerShaderStage (8) is too tight'. The browser device-request seam MUST carry a limits request of at least maxStorageBuffersPerShaderStage >= 9 or the four room-layout compute pipelines fail at creation. TimestampQuery feature requested if present (:282-286). |
| src/cartridges/the_board/realization/renderer.hpp | room pipeline layout 1332-1342; pipelines update_player_agent 1349, update_other_agents 1359, update_sphere 1374, update_cube 1383 | 9 storage buffers / 7 uniform buffers visible to Compute stage | sum of buffer-typed BindGroupLayoutEntry with Compute visibility across the 3 stacked layouts (Compute Entity 5S/5U state.hpp:4233-4305 + Compute Texture 0/0 :4530-4556 + The Room 4S/2U :5107-5139) | SEAM | THE ONE EXCEEDANCE: 9 > WebGPU conservative default maxStorageBuffersPerShaderStage=8 (within LAWS L2's working cap of 10). WebGPU validates this against the pipeline LAYOUT, so it fails at pipeline creation regardless of shader usage. Port must request the higher limit (see console.hpp seam). |
| src/cartridges/the_board/realization/state.hpp | Render Entity Layout 4311-4406 + Render Texture Layout 4462-4525 (main-render family layout pair, renderer.hpp:1574-1583) | VS 7 storage / 7 uniform; FS 7 storage / 3 uniform | sum of buffer-typed entries by ShaderStage visibility flag across the two stacked layouts | SAFE | NEAR-LIMIT: 7/8 storage = 87.5% in BOTH VS and FS of all 13 main-family render pipelines; 11 shadow pipelines have the same VS 7/7 (no FS). The tree knows the cap is full: state.hpp:3263 'the render VS storage cap is full', :4386-4391/:4393-4398 tier-gains + figure-profiles forced to Uniform to duck the cap, :4409-4410 mesh-gen layout kept to 1 entry 'Avoids exceeding the 8 storage buffer per-stage limit'. Zero headroom for new storage bindings in either stage at default limits. |
| src/cartridges/the_board/realization/state.hpp | render_floating layout entry 4329-4331 (BufferBindingType::Uniform, Vertex\|Fragment); bind group entry 5338-5340 | 54,912-byte uniform binding (264 x 208) | exact entry size expression Dim::TOTAL_FLOATING_SLOTS * sizeof(GPUFloatingEntityState); constants state.hpp:341, :1659 | SAFE | NEAR-LIMIT: 83.8% of default maxUniformBufferBindingSize 65,536 — the largest uniform binding in the program. GPUFloatingEntityState growth past 248 B (or slot count past 315) breaches the WebGPU default. Buffer also carries Storage\|CopySrc usage (:3270-3272). Next-largest uniform binding: Tile Grid 16,400 B (25%). |
| src/cartridges/the_board/realization/state.hpp | createBindGroups() 4229-6210: 25 CreateBindGroupLayout sites (4303-5237), 27 CreateBindGroup sites (2494, 5308-6200) | 25 layouts / 27 bind-group creations | call-with-paren 'CreateBindGroupLayout\(' and 'CreateBindGroup\(' per-file; full site lists in evidence | SAFE | All 25 layouts enumerated in evidence with per-entry type+visibility. Group-index assignment: 20 layouts are group-0 shapes, 3 are group-1 (Shadow/Render/Compute Texture + Gallery Texture makes 4 g1-shaped), 1 is group-2 (The Room). Render entity group built 3x (plans A/B/C) differing only in entries[13] window (5412-5414). |
| src/cartridges/the_board/realization/renderer.hpp | createComputePipelines() 1294-1558; createRenderPipelines() 1560-2482 | 29 compute + 30 render pipelines (builders: makeComputePipeline x29, computeLayoutFor x18, makeEntity x9 *(corrected from 11)*, makeShadow x13; 8 direct render sites); 10 CreatePipelineLayout sites + computeLayoutFor helper (:174, 18 invocations) | call-with-paren 'CreateRenderPipeline\(\|CreateComputePipeline\(' (1 + 10 textual sites, both shared builders counted once) + invocation-site enumeration of makeComputePipeline/makeEntity/makeShadow | SAFE | Full name list + per-pipeline layout stack + per-stage S/U counts in evidence. Max bind groups per pipeline = 3 (room family) — 75% of maxBindGroups 4. All pipelines compile in demo=full (Roster all_enabled, contracts/roster.hpp:138-185). |
| src/cartridges/the_board/realization/renderer.hpp | SetBindGroup dispatch/draw helpers 368-1184 (57 sites); group index 2 only at 370, 384, 409, 423 | 82 SetBindGroup sites in renderer.hpp *(corrected)* + 1 in render_passes.hpp:150; indices used: 0,1,2 only | call-with-paren 'SetBindGroup\(' across src, imgui/docs excluded | SAFE | Group 2 is set only by the four room-family compute kernels. render_passes.hpp's single site attaches the compute-texture group (index 1) for the placement pass. No index 3 anywhere — three-group law holds at the call sites too. |
| src/cartridges/the_board/realization/state.hpp | FC_SEG constants 1542-1547; build_render_entity_group 5313-5414 | 0 dynamic offsets repo-wide; 3 static bind groups over one 2,048 B buffer at offsets 0/512/1024 | grep 'hasDynamicOffset\|dynamicOffset\|DynamicOffset' across src (zero matches); exact-line read of FC_SEG_* | SAFE | maxDynamicUniformBuffersPerPipelineLayout is untouched (0 of 8). Static offsets are 256-aligned, satisfying minStorageBufferOffsetAlignment default 256. |
| src/cartridges/the_board/realization/renderer.hpp | vertex states: MeshVertex 1697-1709, ArchVertex 1724-1742, ShellVertex 1772-1787, orb quad 1833-1842, shadow twins 2275-2287/2314-2332/2362-2377 | max 1 vertex buffer / max 4 attributes per pipeline | per-pipeline VertexBufferLayout enumeration (bufferCount is 0 or 1 at :1618/:2261; attributeCount per VBL) | SAFE | Well under maxVertexBuffers 8 and maxVertexAttributes 16. Strides 24/36/40/8 pinned by static_asserts state.hpp:1671-1678 against WGSL input structs. 14 pipelines are fully bufferless (VS generates from vertex_index). |
| src/cartridges/the_board/realization/state.hpp | frame meter block 3325-3348; METER_QUERY_COUNT 1825; console feature request console.hpp:282-286 | 1 CreateQuerySet site, 64 timestamp queries, 512 B resolve + 512 B staging | call-with-paren 'CreateQuerySet\(' (single site 3339); exact-line METER_QUERY_COUNT = 64 | SAFE | Double-gated: INSTRUMENTS.frame_meter (default OFF — T7_INSTRUMENTS=off, core/instruments.hpp:73-74) AND device.HasFeature(TimestampQuery). Pattern degrades loudly to CPU rows; browser timestamp-query quantization is a port note, not a blocker. |
| src/cartridges/the_board/realization/state.hpp | 5 MapRead staging buffers: 3302 (agent), 3309 (floating), 3316 (camera), 3323 (ribbon ring), 3347 (meter) | 5 CopyDst\|MapRead buffers, largest 54,912 B | direct CreateBuffer( sites with usage CopyDst\|MapRead | SAFE | GPU->CPU readback channel (point/camera-host + probes). mapAsync exists in browser WebGPU; whether the WAIT pattern around these maps is synchronous is a different hypothesis's seam — buffer objects themselves are portable. |
| src/cartridges/the_board/realization/state.hpp | ribbon LATENT tags 3273 (buffers), 3323 (readback); gallery LATENT 3379; orbs LATENT 4023; gol LATENT 3953 | 13 LATENT[gate-a-shared] tags in state.hpp *(corrected)*: ribbon 3273, spot_lights 3291, gallery 3379, sphere 3633, cube 3662, arch 3739, column 3784, palm 3823, cactus 3851, blade 3877, gol 3953, pawn_aura 4001, orbs 4023 | exact-comment-line reads of the LATENT[gate-a-shared] annotations | CUT | Per the tree's own annotations, CUT_1-adjacent pieces in this census: ribbonRing pipeline + ribbonReadbackStaging_ 'droppable', BUT ribbonBuffer_/ringTransformsBuffer_ are load-bearing in Render Entity + Photographer groups ('Retire = re-section those groups' — an L7 binding-closure edit, not a deletion). Same shape for gallery (paintingSlotsBuffer_ exclusive-in-Compute-Entity + Entity-Placement) and orbs (orbStateBuffer_/orbConfigBuffer_ in Render Entity). Cutting any of these families forces bind-group re-sectioning. |
| src/external/imgui/backends/imgui_impl_wgpu.cpp | 504, 528, 722 (buffers); 389, 765-771, 845, 875-877 (groups/pipelines); 451-589 (SetBindGroup) | 3 buffer + 7 group/pipeline + 5 SetBindGroup textual sites — 0 compiled | same call-with-paren greps; file is NOT in the 75-file PORT0_SCOPE closure | SAFE | Textual-only: imgui backend is not a TU of incubator_dual (it belongs to the excluded the_lab). Every in-closure GPU-resource count in this report excludes it. |
| src/cartridges/the_board/realization/state.hpp | per-stage sampled-texture/sampler/storage-texture maxima across all 25 layouts | FS sampled textures 6 (of 16); samplers <= 4 (of 16); compute storage textures <= 2 (of 4) | verify add: mechanical awk over all CreateBindGroupLayout blocks | SAFE | All comfortably inside WebGPU defaults; completes the per-stage limits table beyond buffer classes. |
| **H5C — LIMITS: WGSL** — verdict **CONFIRMED** | | | | |
| src/cartridges/the_board/realization/world.wgsl | whole module: entry points | 65 (28 vertex / 8 fragment / 29 compute) | attribute-token regex "@compute\|@vertex\|@fragment" line-anchored; fn name from following line; verified against brace-matched parse (65 entries) | SAFE | Full name+line list in evidence sec.1; all entry-point machinery core WGSL. |
| src/cartridges/the_board/realization/world.wgsl | 29 @workgroup_size sites (5704-13026) | 29; max product 256 (16,16 at 8722, 8743, 9337); max dim 64; 13 kernels at size 1 | regex "@workgroup_size\("; all arguments literal integers, none override-sized | SAFE | No product exceeds default maxComputeInvocationsPerWorkgroup=256; three sites sit exactly AT 256 (zero headroom, still legal). Size-1 kernels are a perf smell, not a portability failure. |
| src/cartridges/the_board/realization/world.wgsl | binding table (module scope, L878-12744) | 100 declarations / 97 slots; groups 0,1,2 only; g0=80 (19u/30 s,rw/22 s,r/6 stex/2 tex/1 smp, max binding 501), g1=14 (4 smp/8 tex/2 s,r), g2=6 (3 s,r/1 s,rw/2 u) | regex "@group\(\d+\) @binding\(\d+\)" grep -cE=100; address space/type/name parsed by census script; "@group\([3-9]" = 0 hits | SAFE | Three-groups law CONFIRMED (<= default maxBindGroups 4). Max binding number 501 < default maxBindingsPerBindGroup 1000. Full table in evidence sec.3; matches binding_registry.hpp mirror. |
| src/cartridges/the_board/realization/world.wgsl | duplicate slots (0,1) L6013/L9949, (0,2) L6014/L9950, (0,340) L6146/L9951 | 3 alias pairs | uniq -c over grep -oE '@group\(\d+\) @binding\(\d+\)'; per-entry conflict check via static-use closure | SAFE | config/fc_config, vp_data/fc_vp, patch_instances/fc_patches. fc_* used only by frustum_cull_patches; no entry point statically uses both members of a pair — valid per-entry-point WGSL aliasing, carries verbatim. |
| src/cartridges/the_board/realization/world.wgsl | per-entry-point static binding use (all 65 entries) | maxima: 8 storage buffers (patch_terrain_fs L4585; update_other_agents L7920), 6 uniforms, 6 sampled textures, 3 samplers, 1 storage texture | census script: comment-strip + brace-matched fn bodies + \b-token resource-name match + transitive call graph (292 fns); overcount possible only if a local shadows a resource name (none observed) | SAFE | Static use everywhere <= core defaults; two entries exactly at 8 storage buffers. NOTE: WebGPU validates LAYOUT entries per stage, not static use — see the state.hpp/renderer.hpp findings. |
| src/cartridges/the_board/realization/renderer.hpp | room pipeline layout, 1332-1342; consumers 1349, 1359, 1374, 1383 | 9 storage-buffer layout entries visible to Compute (5 from computeEntityLayout + 4 from roomLayout) | awk extraction of .binding/.visibility/.buffer.type over state.hpp:4229-5260 layout blocks; pipeline-layout composition read from renderer.hpp:1294-1385 | SEAM | HARD LIMITS FINDING: 9 > WebGPU default maxStorageBuffersPerShaderStage (8). Pipelines update_player_agent, update_other_agents, update_sphere, update_cube fail on a default-limits device; browser port must request elevated limits at requestDevice (adapter-dependent) and cannot run in compat mode. |
| src/console/console.hpp | device creation, 263-269 (log at 288-292) | 1 requiredLimits site: adapter.GetLimits() passed wholesale as deviceDesc.requiredLimits | grep -rnE 'RequiredLimits\|requiredLimits\|maxStorageBuffer\|GetLimits' over closure | SEAM | In-tree comment: "The default maxStorageBuffersPerShaderStage (8) is too tight". The request-adapter-max pattern must be reproduced in the browser (navigator.gpu requestDevice with requiredLimits from adapter.limits); state.hpp:4285-4290 additionally cites a "10-per-stage limit" the native adapter grants. |
| src/cartridges/the_board/realization/state.hpp | createBindGroups(), 4229-5260 (25 CreateBindGroupLayout sites) | render pipeline {renderEntity+renderTexture}: 7 vertex-stage + 7 fragment-stage storage-buffer entries | same awk extraction; renderLayouts composition at renderer.hpp:1574-1580 | SAFE | Within core default 8 but ZERO-to-ONE headroom (comments at 4386-4398: "the VS storage-buffer cap is full" — two tables demoted to uniform). Breaks WebGPU compat-mode stage limits; core-only target required. |
| src/cartridges/the_board/realization/world.wgsl | var<workgroup> at 8741 (sh_height) and 9335 (sh_card_h) | 2 vars, each array<f32,400> = 1600 B; max 1600 B per entry point | regex "var<workgroup>"; usage mapped by static-use census (generate_patch_gradients, write_live_card_resolve respectively) | SAFE | 1600 B << 16384 default maxComputeWorkgroupStorageSize. 2 workgroupBarrier() calls, core. |
| src/cartridges/the_board/realization/world.wgsl | extension surface (whole module) | 0 enable/requires; 0 f16; 0 subgroup; 0 push_constant/immediate; 0 ptr<storage/workgroup/uniform> params; 0 deprecated [[ ]]/@stage/chromium tokens | regexes: '^enable \|^requires '; '\bf16\b'; 'subgroup' -i; 'push_constant\|immediate' (3 hits all prose comments L3133/L3183/L8619); 'ptr<(storage\|workgroup\|uniform)'; '\[\[\|@stage\(\|chromium' | SAFE | The WGSL text is pure core WebGPU; carries verbatim to any browser WGSL front end. |
| src/cartridges/the_board/realization/world.wgsl | override USE_PATCH_INDIRECTION, line 129 | 1 override constant; set once from C++ (renderer.hpp:1662, constantCount=1 at 1675) | regex 'override ' | SAFE | Core pipeline-overridable constant (bool, default false; =1.0 for the indirect terrain pipeline). Portable. |
| src/cartridges/the_board/realization/world.wgsl | storage textures L6140, L6142, L6407, L6419, L6420, L9621 | 6, all access=write; formats rgba16float x3, rgba8unorm x1, r32float x2 | grep -oE 'texture_storage_[a-z0-9_]+<[^>]+>' uniq -c | SAFE | All write-only in core-guaranteed storage formats; no read_write storage textures (would have been non-core). |
| src/cartridges/the_board/realization/world.wgsl | sampling builtins: textureSampleCompare L4122/L4134 (in sample_spot_shadow_pcf, fn at 4016); textureSample L10340/L10555; textureSampleCompareLevel x16 (sample_shadow_pcf, fn at 3776); textureSampleLevel x8; textureLoad x11; textureStore x11 | 2 + 2 fragment-only builtin call sites; 16+8+11+11 stage-free | grep -oE 'textureSample[A-Za-z]*\(' / 'textureLoad\(\|textureStore\(' uniq -c; stage safety via census (no non-fragment entry statically reaches shadow_map/spot_shadow_map/shadow_sampler) | SAFE | Fragment-only builtins confirmed reachable only from fragment entries (patch_terrain_fs, entity_fs, ribbon_fs, gallery_frame_fs, wall_painting_canvas_fs). Builtin value census all-core (global_invocation_id, vertex_index, instance_index, position, local_invocation_id, workgroup_id). |
| src/cartridges/the_board/realization/renderer.hpp | loadShader(), 1229-1292; CreateShaderModule at 1286 | 1 shader-module creation site in the entire closure; 2-entry disk path search list (1249-1252); 1 hot-reload branch (1231-1243) | grep -rnE 'ShaderModuleWGSLDescriptor\|ShaderSourceWGSL\|CreateShaderModule\(' over src/ *.hpp/*.cpp minus excluded targets; '@vertex\|@fragment\|@compute' in *.hpp = 0 hits (zero embedded WGSL in headers) | SEAM | world.wgsl arrives via std::ifstream cwd-relative path search + reload-from-remembered-path. Browser needs a fetch/preload-FS seam for source delivery and for the hot-reload path. wgpu::ShaderSourceWGSL chain itself matches emdawnwebgpu naming. |
| src/external/imgui/backends/imgui_impl_wgpu.cpp | 326-374 (embedded WGSL + wgpuDeviceCreateShaderModule) | 1 embedded-WGSL module site — OUT OF CLOSURE | same closure-wide grep; file is not one of the 3 incubator_dual TUs and absent from the 75-file PORT0_SCOPE closure | SAFE | Recorded only so the synthesizer knows the grep hit is accounted for; not compiled by glaw1. |
| src/cartridges/the_board/realization/world.wgsl | C++/WGSL mirrored constants: PATCH_CELL_SIZE 230, LIVE_CARD_SIZE/EXTENT 233-234, TILE_GRID_CAPACITY 1078, FIELD_SUBSCRIBERS 2246 (=296u), SHADOW_MAP_SIZE 3763 (=4096.0), PAINTING_MAX_SLOTS 10124 (=288) | 6 cross-room constant twins (state.hpp mirror comments at 76, 90, 135, 220, 326, 364-371) | exact-token grep 'const NAME' in world.wgsl + 'world.wgsl' token grep in closure | SAFE | Carry verbatim with the file; they are a maintenance contract, not a porting blocker. state.hpp:364-371 carries a static_assert witness on FIELD_SUBSCRIBER_CAP. |
| src/cartridges/the_board/realization/state.hpp | Patch Gen Layout (CreateBindGroupLayout :4601): heightfield write + cell color write | 2 storage-texture entries in one compute-stage layout — the layout-level max | verify add: awk storageTexture.access count per layout block | SAFE | 2 <= default maxStorageTexturesPerShaderStage (4). WebGPU validates layout entries per stage; per-entry-point static use max is 1. |
| src/cartridges/the_board/realization/world.wgsl | @location census across the 6 Varying structs | max 6 inter-stage variables per pipeline (PatchTerrainVarying) | verify add: awk struct-body @location count | SAFE | vs default maxInterStageShaderVariables 16 — wide headroom. |
| **H6 — THREADS** — verdict **CONFIRMED** | | | | |
| src | closure-wide thread-creation census (all 75 closure files + 3 TUs) | 0 / 0 / 0 / 0 / 0 | five separate greps over src/: qualified token `std::thread` (does not match std::jthread); token `std::jthread`; token `CreateThread\|_beginthreadex\|_beginthread`; token `std::async`; call-with-paren `timeSetEvent\(` — all zero matches | SAFE | The program creates no threads of its own under any boundary. The only execution context beyond main is OS-owned (winmm driver callback thread). |
| src/external/RtMidi.cpp | pthread_create sites: MidiInAlsa::openPort 2238 and 2300 (__LINUX_ALSA__ block 1740-2632); MidiInAndroid::openPort 4958 (__AMIDI__ block 4843-5275) | 3, all textual | token `pthread_create` over src/; compiled-vs-textual resolved against the #if guard map (grep `^#if defined\(\|^#endif\|^#elif\|^#else` with -n); build defines only __WINDOWS_MM__ (branch 2644-3209) | CUT | NOT COMPILED. The MSVC build compiles only the __WINDOWS_MM__ branch, which contains zero thread-creation calls. Whole file dies with MIDI intake in CUT_1. |
| src/external/RtMidi.cpp | MidiInWinMM::openPort midiInOpen registration, 2857-2861 (midiInStart 2897); compiled __WINDOWS_MM__ | 1 registration site | call-with-paren `midiInOpen(` in compiled branch; CALLBACK_FUNCTION flag at 2861 | CUT | THE one extra execution context: winmm invokes midiInputCallback on a driver-owned thread. Hypothesis's expected names verified: midiInOpen at 2857 (expected ~2857), midiInputCallback at 2698 (expected ~2698). |
| src/external/RtMidi.cpp | midiInputCallback, 2698-2791 (compiled); handoff fork at 2779-2787 | 1 callback; touches 6 RtMidiInData fields + 5 WinMidiData fields | function read in full; fields enumerated by line: firstMessage 2711-2714, ignoreFlags 2729/2734/2738/2749/2771, usingCallback/userCallback/userData 2779-2781, queue 2785; WinMidiData message/lastTime/sysexBuffer/inHandle/_mutex 2712-2777 | CUT | Runs on the winmm driver thread. Project registers a user callback (midi_port.hpp:76), so the usingCallback branch (2780-2781) runs at steady state; the internal-queue branch (2783-2787) runs only in the openPort-to-setCallback window. |
| src/external/RtMidi.cpp | WinMidiData CRITICAL_SECTION _mutex: decl 2690; Enter/Leave in callback 2765/2767; Init 2827; Delete 2805; Enter/Leave in closePort 2920/2940 (compiled). Textual UWP mutexes: 3341, 3343, 3657, 3725, 3770, 3817, 3885, 3923 | compiled: 1 decl + 6 call sites (+1 error-string mention at 2828); textual UWP: 2 std::mutex + 6 std::lock_guard lines | pattern `std::mutex\|std::lock_guard\|std::unique_lock\|CriticalSection` over src/ (case-sensitive) + separate all-caps pattern `CRITICAL_SECTION` (1 hit, 2690); compiled-vs-textual per guard map | CUT | The only lock in the compiled program. Serializes the callback's sysex-buffer requeue (midiInAddBuffer) against closePort teardown. Project closure files: 0 mutex hits. |
| src/external/RtMidi.h | MidiQueue 599-611 (plain `unsigned int front; unsigned int back;`) + RtMidiInData 615-636; impl RtMidi.cpp size/push/pop 876-931; drain API getMessage RtMidi.cpp:853-868 | 0 atomics, 0 locks in MidiQueue; 0 project getMessage call sites | qualified token `std::atomic` per file (0 in RtMidi.h); token `getMessage` over src/ — hits only inside RtMidi.cpp (284, 853, 3814, 3819) and RtMidi.h (130, 254, 349, 395, 583, 664), none in project code | CUT | RtMidi's internal ring is an unsynchronized SPSC queue (formally a C++ data race), but the project never drains it — it is only written during the open()-window before setCallback (default limit 100 messages, RtMidi.h:303), then bypassed forever. |
| src/sources/midi_port.hpp | MidiPort SPSC ring: ring_/write_idx_/read_idx_ 152-154; producer push 189-195 (driver thread, drop-on-full 192); consumer poll 121-133 (main thread); pending_count 135-139; callback entry on_rtmidi_callback 158-163 -> handle_message 165-187 | 2 std::atomic (153, 154); 1 ring of 256 MidiEvent; RING_SIZE=256 (142) | qualified token `std::atomic` in file = 2; memory orders read from source: push relaxed-load-write/acquire-load-read/release-store, poll acquire-load-write/relaxed-load-read/release-store | CUT | The project's actual cross-thread crossing for note events: lock-free single-producer(single-consumer) ring, correct acquire/release pairing. Producer = winmm driver thread via RtMidi user-callback; consumer = main thread. |
| src/sources/midi_port.hpp | MidiPort::open ordering, 75-78: openPort (75) precedes setCallback (76); ignoreTypes(true,false,true) 78 lets clock through | 1 ordering window | exact-line reading of open(); RtMidi else-branch RtMidi.cpp:2783-2787 is the destination during the window | CUT | Messages arriving between midiInStart and setCallback go to RtMidi's internal MidiQueue and are never drained (no getMessage caller). Bounded (limit 100), no steady-state effect. |
| src/sources/transport.hpp | MidiTransport: atomics 86-89; producer feed 35-63 + on_clock 95-107 (MIDI thread); consumers playing/pulses/beats/bpm/ever_synced 67-74 (main thread); non-atomic scratch acc_/ema_ 92-93; reset 76-83 | 4 std::atomic (86, 87, 88, 89) | qualified token `std::atomic` in file = 4; producer/consumer split per the file's own contract comment at 16-18 | CUT | DAW transport state crossing: pulses_ (fetch_add acq_rel), playing_, synced_ (release stores), bpm_ (relaxed). reset() is called from MidiPort::close() (midi_port.hpp:105) on the MAIN thread and writes non-atomic acc_/ema_ — safe only because cancelCallback+closePort quiesce the driver thread first; ordering-dependent, not lock-protected. |
| src/analysis/canvas_1/canvas.hpp | Main-thread drain: Canvas::update 150-159 (beats 153, poll 155, route 157, advance 158); transport telemetry reads 490-491 in the onset aperture-shut diagnostic 483-493; owner `MidiPort port_` 668; open_by_name("loopMIDI") 145 | 1 poll site / 1 beats site / 3 telemetry accessor reads / 1 owner | pattern `MidiPort\|midi_port\|\.poll\(\|->poll\(` and pattern `playing\(\)\|ever_synced\(\)\|\.bpm\(\)\|pulses\(\)\|\.beats\(\)` over src/, filtered to closure | CUT | The ONLY consumer of the MIDI thread's shared state, and the only RtMidiIn owner in the closure. Everything runs inside analysis.update on the main thread. |
| src/incubator_dual.cpp | main() frame loop 205-265; drain edge analysis.update(dt) at 243; input routing 225-240; watcher check 209-222 | 1 loop, 0 thread creations in TU | full-file read (269 lines); thread census patterns from finding 1 all zero here | CUT | The frame loop itself is portable, but line 243 is the subtraction edge: it is the sole main-thread entry into the MIDI-fed analysis chain (update -> canvas_1 -> poll). Tagged CUT for the edge, not the loop. |
| src/console/console.hpp | GLFW callbacks 112-155 (key/cursor/focus/mousebutton/scroll); glfwPollEvents 375 in begin_frame 374-398; producer inject_* 430-469 into plain std::vector inputEvents_; producer/consumer contract comment 421-425 | 5 callback registrations, 0 threads, 0 sync primitives | pattern `glfwPollEvents\|ProcessEvents\|CreateInstance\|Instance\|WaitAny\|Tick\|...callback` (-i) over file; mutex boundary pattern = 0 hits in file | SAFE | GLFW callbacks execute synchronously inside glfwPollEvents on the main thread — not a thread. Unsynchronized event vector is correct under single-threaded use. (Its GLFW-to-browser seam is another hypothesis's ledger entry; threading-wise it is safe.) |
| src/console/console.hpp | Dawn setup: dawn::native::Instance member 600, emplace 168 *(corrected)*, EnumerateAdapters 170, SetUncapturedErrorCallback 257, CreateSurface 338 | 0 thread-configuration sites | pattern `Toggle\|dawn::platform\|WorkerTask\|thread` (-i) over file — only cursor-grab toggle hits (126, 516, 524) | SAFE | Project code does not configure Dawn's internal threading. Dawn-internal threads are Dawn's own — out of scope per instructions. |
| src/external/stb_image.h | STBI_THREAD_LOCAL machinery: define 623-636 (MSVC `__declspec(thread)` 629), uses 967-968, 1121-1135, 5006-5031; comments 57, 487, 520-525, 5246, 7783, 7899-7905 | 0 thread creations; thread-local storage only | case-insensitive substring `thread` in file; all hits are TLS macros or changelog comments | SAFE | 523-525 are TLS-flag setter API declarations, not comments *(corrected)*. Thread-local failure/flip flags; single-threaded use here. No concurrency to port. |
| src/external/RtMidi.cpp | std::atomic in RtMidi: single hit at 395 (`std::atomic<bool> reading`, MidiInAndroid, __AMIDI__ declarations block 362-421) | 1, textual | qualified token `std::atomic` in file; branch resolved by reading 362-398 | CUT | Android-only, never compiled. Zero atomics in the compiled RtMidi branch — the compiled winmm path relies on the CRITICAL_SECTION plus the unsynchronized MidiQueue. |
| src/external/RtMidi.cpp | __WEB_MIDI_API__ branch 4522-4833: MAIN_THREAD_EM_ASM proxying at 4552, 4571, 4613, 4664, 4692, 4722, 4797, 4812 | 8 proxy-macro sites, textual | case-insensitive `thread` sweep, guard map 4522-4833 | CUT | 4571 is MAIN_THREAD_ASYNC_EM_ASM *(corrected)*. Port-relevant observation: RtMidi's emscripten backend is main-thread Web MIDI — the intake thread does not exist in a browser build even if RtMidi survived. Moot because MIDI intake is CUT_1. |
| src/sources/transport.hpp | documentation-evidence hits of the case-insensitive `thread` sweep across non-RtMidi closure files: transport.hpp 16/17/65/91, midi_port.hpp 156, instruments.hpp 21, incubator_dual.cpp 93, stream_data.hpp 447, cartridge.hpp 1347, state.hpp 337, control_panel.hpp 78 (English verb), spawn_engine.hpp 624 (English verb), renderer.hpp 24/25/191/192/271/371/385/505/1344/1349/1352/1359 (GPU invocations), world.wgsl 4369 (English)/7551/7554/7568/7573/7576/7623/7727/7927 (GPU invocations) | 48 lines *(corrected)*: 44 comments/terminology + 4 WGSL code lines (thread_id workgroup-stride locals, world.wgsl:8756/8760/9346/9350) — all GPU-side | case-insensitive substring `thread` over src/, filtered to the 75-file closure minus RtMidi; each hit classified comment vs GPU-invocation vs English-verb by reading context | SAFE | No unexpected CPU concurrency anywhere. `#include <thread>` appears in zero closure files (only excluded probe_canvas.cpp:35). GPU "threads" are WGSL invocations, unrelated to CPU threading. |
| **H7 — I/O** — verdict **FALSIFIED** | | | | |
| src/incubator_dual.cpp | FileWatcher::check(), frame loop; class 83-117, stat at 103, loop gate 209-222 | 1 stat per 30 frames (~2x/s), unconditional | qualified token "std::filesystem::" — 4 hits all in this file: exists :87, last_write_time :88 (boot watch) and :103 (error_code overload, the per-check stat), file_time_type :116 | SEAM | Disk metadata traffic INSIDE the frame loop, forever, not gated by INSTRUMENTS (only the progress dot is). Dev-only hot-reload machinery; dies in browser — PORT_1 deletes or replaces with a dev-server push. This alone falsifies H7's letter. |
| src/cartridges/the_board/realization/renderer.hpp | loadShader() 1229-1292: boot read 1256-1264 (search list 1249-1252), reload read 1232-1242; reload() 1211-1217; shader_path() 1219; boot caller initialize :331 | 2 ifstream sites, both world.wgsl | token "std::ifstream\|std::ofstream\|std::fstream" — 2 closure hits (:1232, :1256); 0 ofstream/fstream in closure | SEAM | world.wgsl (619,975 B) read at BOOT (init_renderer -> initialize:331) and ON-EVENT (hot reload via incubator_dual.cpp:220 -> cartridge.hpp:2112). Search paths are CWD-relative into the SOURCE tree: "../../../src/cartridges/the_board/realization/world.wgsl" then "src/cartridges/the_board/realization/world.wgsl" — becomes a fetch in PORT_1. Missing file at boot = init failure (exit). |
| src/cartridges/the_board/bodies/gallery.hpp | load_authored_image_to_staging 1505-1564; stbi_load at 1507 and 1511 ("7t/" fallback) | 2 project call sites (the only stbi_load callers in the closure) | call-with-paren "stbi_load\(" — 4 raw closure hits: stb_image.h:141 comment, :1366 definition, gallery.hpp:1507/:1511 calls | SEAM | Decodes JPEG to RGBA, bilinear-resamples into 1024x1024 (Dim::PAINTING_RESOLUTION, state.hpp:263), uploads to authored staging array. Missing file -> stderr + slot stays invalid, no crash. Underlying open chain: stbi__fopen :1337 → fopen_s :1357 (compiled MSVC branch) *(corrected; the _wfopen UTF8 branch is never enabled)*. In PORT_1: fetch + stbi_load_from_memory (or browser decode); the fopen path dies. |
| src/cartridges/the_board/bodies/gallery.hpp | scan_paintings_folder 1568-1618 | 1 directory enumeration; fs uses: exists+is_directory :1580, directory_iterator :1590, is_regular_file :1591, path ops :1578/:1592/:1595/:1605 | alias-inclusive grep "fs::\|std::filesystem" — 6 line hits in this file (1569,1578,1580,1590,1604,1605) | SEAM | Enumerates SEARCH_DIRS {"assets/paintings","7t/assets/paintings"} (CWD-relative), filters PAINTING_*.jpg/.jpeg case-insensitive, numeric-sorts by suffix. No directory listing in the browser — PORT_1 needs a baked manifest (the 57-file list in evidence IS that manifest). Missing folder = graceful (SNAPSHOT_ONLY fallback). |
| src/cartridges/the_board/cartridge.hpp | init_renderer boot eager-load, 570-577 | 32 stbi_load calls at boot (min(57, Dim::STAGING_LAYERS=32), state.hpp:270) | grep "load_authored_textures" callers — cartridge.hpp:576 (boot, if constexpr ROSTER.gallery), gallery.hpp:1135 (commit_gallery lazy no-op), gallery.hpp:1727 (place_wall_paintings lazy no-op) | SEAM | BOOT load moment for paintings: first 32 in numeric order (PAINTING_1..109), 5,633,069 B. ROSTER.gallery=true in default demo=full (matrix.hpp:90, static_assert :151). Flag authored_textures_loaded (gallery.hpp:1621/:1628/:1641) makes the two lazy call sites no-ops in this build. |
| src/cartridges/the_board/bodies/gallery.hpp | rotate_authored_staging 1646-1696, called from teardown_gallery :2175, called from cartridge.hpp:1022 (TEARDOWN case block 964-1088 *(corrected)*) | up to 32 stbi_load disk reads per world transition (one per consumed slot, guard :1662) | grep "rotate_authored_staging" — definition :1646, sole call :2175 | SEAM | PRODUCTION on-event disk reads AFTER boot — not dev machinery. Round-robins authored_disk_cursor through all 57 paintings; the 25 rotation-only files (3,439,629 B) are reachable ONLY via this path. Falsifies H7 even if the watcher is classed dev-only. PORT_1: pre-fetch all 57 or fetch-on-transition. |
| src/analysis/canvas_1/canvas.hpp | Canvas::initialize, 103-148 | 0 file reads; asset_path discarded at :104 | exact line: `(void)asset_path;   // no asset to load; the DAW is the source` | SAFE | The "assets" string passed at incubator_dual.cpp:171 is unused. canvas_1's only I/O is the loopMIDI port open (:145, RtMidi WinMM). No .mid read: the sole .mid parser src/sources/midi_file.hpp (ifstream :84) is OUT of closure and #included by nothing (grep "midi_file" over *.{cpp,hpp,h}: 0 include directives). |
| assets/ | SWEET_CLIP_1.mid (153 B), SWEET_CLIP_2.mid (307 B), SWEET_CLIP_3_3.mid (186 B), README.md (250 B) | 4 files, 896 B, 0 readers in compiled code | grep "SWEET_CLIP\|\.mid\"" over src/*.{hpp,cpp,h} — 0 closure hits | SAFE | Dead weight on disk — copied beside the exe by CMakeLists.txt:575-580 but never opened. EXCLUDE from the PORT_1 fetch bundle. |
| assets/paintings/ | 57 files: 32 boot-loaded (PAINTING_1..109 numeric order) + 25 rotation-only (PAINTING_110..1002) | 57 files, 9,072,698 B total; boot 5,633,069 B; rotation 3,439,629 B | ls + stat byte sums; numeric-sort matches gallery.hpp:1601-1614 ordering; per-file sizes in evidence | SEAM | THE fetch manifest. All 57 reachable by compiled code (32 at boot, 25 via transition rotation). Missing files degrade gracefully. Total fetch budget with world.wgsl: 9,692,673 B (~9.24 MiB); boot-critical: 6,253,044 B (~5.96 MiB). |
| src/cartridges/the_board/realization/world.wgsl | whole file (runtime-loaded, not #included) | 619,975 B / 13,380 lines *(corrected)* | stat -c %s | SEAM | Loaded from the SOURCE tree (not the POST_BUILD cartridges/ copy) at boot and on hot-reload. In PORT_1 this is a fetch (or inlined string) + the watcher/reload machinery dies. |
| CMakeLists.txt | POST_BUILD copies 568-573 (src/cartridges -> <exedir>/cartridges) and 575-580 (assets -> <exedir>/assets) | 2 copy_directory commands for incubator_dual | grep "copy_directory\|POST_BUILD\|assets" over CMakeLists.txt | SAFE | assets copy is live (gallery's "assets/paintings" resolves against it when CWD=exe dir). The cartridges copy is DEAD for the compiled shader loader — see stops. |
| src/external/RtMidi.cpp | whole TU; compiled branch __WINDOWS_MM__ | 0 disk I/O compiled; 2 cout hits (:1856,:1863) both ALSA + __RTMIDI_DEBUG__ = textual only; compiled stderr error sites :641,:720,:781,:789,:2769,:2786 *(corrected: :785 is __RTMIDI_DEBUG__-guarded, textual)* (on-event errors only) | token greps "fopen\(\|std::ifstream\|ReadFile\|CreateFile" (0 hits in file) and "std::cout\|printf\|fprintf" (2 textual); snd_seq/jack/AMidi opens are device opens in non-compiled branches | SAFE | SAFE for H7 specifically: RtMidi touches no disk. (RtMidi itself is a platform seam for other hypotheses, not this one.) |
| closure-wide | write census | 0 disk writes (stbi_write/ofstream/fwrite: 0 closure hits; all matches in imgui/implot, out of closure) | token "stbi_write\|ofstream\|fwrite" over src/ | SAFE | No screenshots, no logs, no config persistence. Nothing to port on the write side. |
| closure-wide | console traffic census (21 files) | 171 token occurrences (2 in comments); 0 unconditional per-frame prints in default build | token "std::cout\|printf\|fprintf" count-mode per file: console.hpp 11, RtMidi.cpp 2, incubator_dual.cpp 14, visual_canvas.hpp 3, visual_params.hpp 1, signal_layout.hpp 1, canvas.hpp 4, spawn_engine.hpp 21, cartridge.hpp 25, agents.hpp 15, state.hpp 6, ribbon.hpp 3, cube_behaviors.hpp 8, renderer.hpp 11, orbs.hpp 14, gallery.hpp 10, pawn.hpp 2, input.hpp 5, mood.hpp 13, gol_zones.hpp 1, surface_services.hpp 1 | SAFE | Every recurring print is compile-gated by INSTRUMENTS (default off, instruments.hpp:73-74): watcher dot incubator_dual.cpp:215-216, [ONSET] canvas.hpp:469, [ZOETROPE] cube_behaviors.hpp:1067, [CHECKER] visual_canvas.hpp:503, [CENSUS]/[METER] cartridge.hpp:1353/:1369. Rest = boot banner or on-event witnesses (full classification in evidence). Browser seam is mild: boot burst + transition bursts of console.log; supplementary std::cerr sites incl. console.hpp:259 WebGPU error callback (could spam under a broken pipeline). |
| src/cartridges/the_board/realization/state.hpp | Dim constants: PAINTING_MAX_SLOTS=288 :258, PAINTING_RESOLUTION=1024 :263, STAGING_LAYERS=32 :270 | 32 staging layers cap the boot painting load; 1024x1024 RGBA per painting on GPU | exact-line grep "STAGING_LAYERS\s*=\|PAINTING_RESOLUTION\s*=\|PAINTING_MAX_SLOTS\s*=" | SAFE | Sizing facts for the PORT_1 memory budget: authored staging = 32 x 1024x1024x4 B = 128 MiB-class texture array regardless of how many files are fetched. |
| closure (5 files) | std::cerr sites the supplementary sweep missed: tile_world.hpp:488; cartridge.hpp:512/1333/1925/1943; spawn_engine.hpp:614/972/993; RtMidi.h:124 (printMessage — dormant, never called: midi_port.hpp swallows RtMidiError at :45/:64/:83/:102) | 9 sites | verify add: token std::cerr over src, closure-filtered | SAFE | All boot-assert or on-event witnesses; consistent with the mild-console classification. Add to the PORT_1 console-seam list. |
| **H8 — THE CUT BOUNDARY** — verdict **CONFIRMED** | | | | |
| src/incubator_dual.cpp | harness analysis anchors: 52/65/68 (includes), 123/126/130 (types), 156/170-171/177 (init), 193 (bind), 229-231 (input route), 243 (update), 248 (output read) | 5 runtime call anchors + 6 declaration/include sites | exact-line reads of the file; 'analysis\.' qualified-token grep within incubator_dual.cpp | SEAM | The ONLY surviving-code touches of Organ A. is_music_key (143-146) always returns false, so analysis.on_input (230) is dead at runtime; Canvas::on_input (canvas.hpp:161) is a no-op anyway. Line 192 comment 'STAT_LAYOUT is constexpr' misstates: layout is runtime-built in initialize(); safe only because 171 precedes 193. |
| src/external/RtMidi.cpp | WinMM branch 2644-3209 (compiled); guards 92/228/516/605/685 | 7 bare-token __WINDOWS_MM__ occurrences (92, 228, 516, 605, 685, 2644, 3209) *(corrected)*; 47 defined() API-family tests *(corrected)*; 8 API families textually present, 1 compiled | bare token __WINDOWS_MM__ grep (count mode); 'defined\(__X__\)' alternation for the 8 API tests | CUT | Whole file + RtMidi.h die with intake. Only the __WINDOWS_MM__ branch compiles (build fact); ALSA/JACK/CoreMIDI/UWP/AMIDI/WebMIDI textual only. Sole in-closure consumer: sources/midi_port.hpp:26. |
| src/sources/midi_port.hpp | MidiPort class 40-214; entry points open_by_name:88, poll:121, beats:114, callback:76/158 | 1 in-closure consumer (canvas.hpp:82) | include-directive grep '#include "sources/' across closure | CUT | With midi_event.hpp and transport.hpp (included at 24-25). NOTE: sources/keyboard_midi.hpp and midi_file.hpp exist on disk but are NOT in the closure. |
| src/analysis/canvas_1/canvas.hpp | Canvas: initialize:103, update:150, on_input:161 (no-op), output:272, stat_layout:274-276; published names 122-143 | includes 8 musical/ + 2 sources/ + 2 analysis/ headers (73-84) | include-directive grep; override-method reads | CUT | Publishes per-voice chN.{current_pc,present_count,window_length,distance,dft_mag,dft_phase,onset} + all.{field,current_pc,present_count,window_length,dft_mag,dft_phase}. Opens loopMIDI at 145. analysis_cartridge.hpp (interface) dies with it. |
| src/musical | 16 headers on disk = 16 in closure (ls verified) | 16, not 15 (hypothesis) | ls src/musical vs PORT0_SCOPE.md lines 62-77 | CUT | 15 of 16 are analysis-only. EXCEPTION: signal_layout.hpp SURVIVES — included by coupling/visual_canvas.hpp:66; it is the render side's name resolver (SignalLayout, resolve-miss = warn + disable, :56). |
| src/analysis/analysis_signal.hpp | AnalysisSignal 72-108, StatGroup 120-126, StatLayoutView 133-136 | 6 including files *(corrected: + canvas.hpp:83, the dying side)*: render_cartridge.hpp:29, state.hpp:30 (vestigial), visual_canvas.hpp:67, signal_layout.hpp:25, analysis_cartridge.hpp:41 | include-directive grep '#include "analysis/' across closure | SAFE | SHARED TYPE SPINE — survives the cut; the post-cut stub must produce this struct + a StatLayoutView. state.hpp:30's include uses zero tokens from it (grep of its 6 token names across the_board = 1 hit at cartridge.hpp:636). |
| src/coupling/visual_canvas.hpp | bind 277-351 (10 source-name resolves: all.field:284, ch1.present_count:298, all.window_length:300, all.present_count:301, ch1.window_length:317, ch0-ch6.onset:339), tick(const AnalysisSignal&) 355ff, zoetrope_rows:556 | 12 resolved source names *(corrected)*: all.field, ch1.present_count, all.window_length, all.present_count, ch1.window_length, ch0..ch6.onset; RIBBON_VOICE=CHECKER_VOICE="ch1" (:126,:201), ZOETROPE_EARS=0x7F (:132) | quoted-string grep 'all\.field' + read of bind(); resolve() call-with-paren sites | SAFE | STOP vs hypothesis: coupling/*.hpp (with visual_params, trajectory) is the RENDER side's coupling engine (cartridge.hpp:77), not intake. These 10 names ARE the stub layout contract; every miss degrades gracefully. |
| src/cartridges/the_board/cartridge.hpp | signal consumption: bind_signal_layout 636-654; UpdateCtx 698-703; U1 767-787 (stats[0..63] copy 776-778); U3 824-834; U4 846-889 (visual_canvas_.tick:848, zoetrope 885-889); U7 953-956; update() 1129-1150 | 6 phases read ctx.signal; 8 param-target resolves at bind | qualified-token grep 'ctx\.signal\|\.signal\b\|GPUFrameSignal\|upload_signal' in the_board + region reads | SEAM | The render side's complete analysis-read surface. Stub must feed advancing t_beats/t_seconds/dt (tempo follower U3 drives ribbon phase + zoetrope + Segments) — frozen time freezes all music-coupled motion but crashes nothing. |
| src/cartridges/the_board/realization/world.wgsl | GPUFrameSignal.stats decl :770; retired-coupling comment :2678 | 0 live GPU readers of stats | grep 'signal\.stats' (1 comment hit) + bare 'stats' (decl + comment) in world.wgsl | SAFE | The 64-float per-frame stats mirror (U1 copy + array<vec4<f32>,16>) is consumer-free since M1-C retired the terrain coupling. Dead weight crossing the seam; layout is load-bearing (GPUFrameSignal 336 B, sky block at offset 304, state.hpp:1601/2089). |
| src/cartridges/the_board/direction/input.hpp | F-key organ: fallback defines 201-215 (10 hits), case labels 272-276 (5 hits); F8 gate 282 | 15 regex hits — matches prior count exactly; single in-closure home | regex GLFW_KEY_F[0-9]+ count-mode per file across src/ (imgui backend hits at imgui_impl_glfw.cpp:435-458 are OUT of closure) | CUT | F4 cycle_cube_behavior_override, F5 cycle_floater_coordination, F6 reveal_zoetrope, F7 toggle_cube_kite_mode, F8 possess(RIBBON). Clean snip = 5 case labels + 10 defines. F8 is NOT diagnostics: shares possess() with surviving key 4; harness print incubator_dual.cpp:199 names it. |
| src/cartridges/the_board/bodies/cube_behaviors.hpp | F-key targets: cycle_floater_coordination:398, apply/cycle_cube_behavior_override:405/412, reveal_zoetrope:496, set_cube_kite:579, toggle_cube_kite_mode:591; state decls 245-247; downstream reads 824, 842, 994-995, 1021, 1030, 1095-1104, 1189-1282 | 4 toggle fns; 3 state words + Formation enum (:254) | call-with-paren grep of the 4 function names across src/; token grep 'behavior_override\|kite_mode\|coordination_step\|\.formation' | SEAM | Cutting the F-key cases leaves this machinery frozen at defaults (formation=ROAM early-outs, kite off, override STATIONARY, coordination 0.0). zoetrope_strike/service stay called per-frame from surviving U4 (cartridge.hpp:885-889) — they are analysis-coupling consumers, not F-key-private. |
| src/cartridges/the_board/realization/state.hpp | F5 wire: config.floater_coordination :536, stage :2737, zero-init :1785; read cartridge.hpp:899 + world.wgsl:1602/8547 | boots 0.0; 2 surviving readers | token grep 'floater_coordination' across the_board | SEAM | ENTANGLEMENT: the beacon (FIELD_4) and WGSL cube synchrony activate only when F5 raises this off 0.0. Cutting F5 makes the beacon writer (cartridge.hpp:891-917, rows[1][2]=coord>0) and the ribbon lure arm permanently inert — cut them together or re-pin the boot value. |
| src/cartridges/the_board | Organ C census: 36 'retired' hits — orbs.hpp:382/734, input.hpp:260, cartridge.hpp:1114 (dome-anchor camera cmds, gone); ribbon.hpp:36/1074 (gen-1 coupling, gone); state.hpp:324/500/1005-1017/1660-1666 + world.wgsl:1571 (byte-layout ballast); matrix.hpp:143-150 + roster.hpp:178 (retired demo headers, pinned by goldens); spawn_services.hpp:96-141, ground_architecture.hpp:24, seed_utils.hpp:80, binding_registry.hpp:40, entity_pipeline.hpp:300, spawn_engine.hpp:41/196, render_passes.hpp:67/232/472, renderer.hpp:2117, state.hpp:643/2779/4164/4750, cube_behaviors.hpp:33/857, world.wgsl 12 comment sites | 53 hits (40 .hpp + 13 world.wgsl) *(corrected)*, 0 live machinery | grep -rin 'retired' across all the_board files incl. world.wgsl, every hit classified comment-vs-code | SAFE | STOP: no live retired camera/ribbon mode machinery exists — earlier campaigns already excised it. No CameraMode/ribbon-mode enum (grep 'camera_mode\|CameraMode\|orbit_mode' = 0 code hits; PointHost all-live; RibbonColorMode all-live in raffle :1198-1209). The pier/pad residues are load-bearing byte-contract ballast: deleting them breaks GPU struct offsets (static_asserts state.hpp:1660-1666). |
| src/cartridges/the_board/bodies/ribbon.hpp | Organ D private machinery: the field read 902-1071 inside ribbon_frame_tick (:824); consts 122-141; deps 72-82; FIELD_B0 floor 698-704 | 1 block, 1 surviving call anchor (cartridge.hpp:1434-1435), 3 private consts + 5 panel dials | grep -i 'avoid' (0 machinery hits in C++ — NAME LAW: true name is FIELD/FIELD_2); grep 'field\|lure' in ribbon.hpp; call-with-paren 'ribbon_frame_tick' across the_board | CUT | Clean cut: delete 902-1071 + RIBBON_FIELD_* consts (122-141) + control_panel.hpp:80-82; the rest of ribbon_frame_tick (phase clock 826-861, head mover, sky resync :1158) stands. Emitter sources read: agents :943-949 (AGENT_TIER_GAINS contact_radius), spheres :950-954, cubes :955-959, occupiers :960-1016, lure :1017-1049 (field_authored_stage, state.hpp:2292). |
| src/cartridges/the_board/cartridge.hpp | Organ D anchors in surviving code: R7 phase_ribbon_tick 1430-1436 (release_sky_exit_ribbon:1434 + ribbon_frame_tick:1435); beacon writer 891-917 (upload_field_authored:916) | 2 anchors | call-with-paren grep 'ribbon_frame_tick\|release_sky_exit_ribbon\|resync_sky_head' | SEAM | The beacon writer is SHARED with the GPU field (cubes gather) — it is not ribbon-avoidance-private. The GPU dialect (world.wgsl field_sum/field_pair :2230-2330, FIELD_SUBSCRIBERS=296 :2246) is SURVIVING entity behavior and must not ride a ribbon cut. |
| src/cartridges/the_board/bodies/spheres.hpp | shared mirror infra: reconcile_sphere_mirror 253-271 (live_pos/live_body_radius); twin reconcile_cube_mirror cube_behaviors.hpp:1296-1316; fields floaters.hpp:88-92/152-157 | 2 reconcilers, dual-consumer (avoidance + spawn/evict) | token grep 'live_pos\|live_body_radius' across the_board *.hpp | SAFE | STOP vs hypothesis span-claim: the avoidance 'machinery in cube_behaviors/agents/spawn_engine' is actually SHARED readback/tier/occupier infrastructure (agents.hpp:177-189 tier table; spawn_engine.hpp:445/526 occupier mesh+separation) — avoidance only READS it. Cutting avoidance must not touch these. |
| src/cartridges/the_board/bodies/pawn_figures.hpp | Organ E paste-site: banner 21-23 'transcribed verbatim from 7t_pawn_designer.jsx'; PawnShape must match STACKED_SHAPES 37-49 | 1 of 1 designer paste-site file in closure | grep -i 'designer\|paste\|jsx\|tools/' across the_board; -i 'import' across the_board *.hpp = 0 | SAFE | Frozen pasted constants — survives as data. WGSL twin comments world.wgsl:12081-12114/:1806. NO runtime designer channel exists anywhere in closure: control_panel.hpp is constexpr rests ('still a rebuild', :35-37); demo.hpp/matrix.hpp are compile-time token-paste doors with golden static_asserts; cartridge.hpp:1363 is Jean pasting the METER printout OUT (stdout, INSTRUMENTS-gated), not an import; console.hpp clean. Runtime file I/O found = shader ifstream (renderer.hpp:1232/1256, hot-reload SEAM) + paintings scan (gallery.hpp:1568-1644, asset intake) — neither serves src/tools/*.jsx. |
| src/console/console.hpp | include block 28-55 | 0 includes into musical/coupling/analysis/sources; 0 designer tokens | include-directive grep in console.hpp; grep -i 'canvas_1\|analysis\|designer\|paste\|import\|rtmidi' | SAFE | Console is fully clean of both organs A and E — only core/input_event.hpp + Dawn/GLFW/std. |

---

*198 rows. Census and adversarial verification: 20 reading passes over
the 75-file closure + world.wgsl at HEAD `f475ffc`; zero builds, zero source
edits. Product of PORT_0; CUT_1 binds to the H8 anchors, PORT_1 to the SEAM
rows and the fetch manifest.*
