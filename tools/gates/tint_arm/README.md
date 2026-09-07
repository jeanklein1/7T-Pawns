# THE TINT ARM — the recipe, so it is a tool and not a session

GATHER_0's ruling 6 made the Tint arm the shader gate. This is the arm, and
the reasons it is shaped the way it is.

## Why it exists

`tools/wgsl_gate.py` runs naga, which parses, scopes and type-checks. It does
**not** analyse derivative uniformity: measured at MIP_0 against naga-cli
30.0.1, a textbook violation — `textureSample` with implicit derivatives
directly under a non-uniform `if` — prints *"Validation successful"*. Tint is
what Chrome compiles with, and Tint enforces the rule. A shader that naga
blesses can still be rejected by every browser that ships it.

## What it needs

`python3` and a Chromium. That is the whole list — no Dawn checkout, no node,
no Playwright, no display. Chromium already contains Dawn and Tint, and
WebGPU's `createShaderModule` + `getCompilationInfo()` is Tint's own verdict
on the real module. The DevTools protocol is driven by a WebSocket client
written in the standard library inside `tint_arm.py`.

## Running it

```
# Linux / macOS
export T7_CHROMIUM=/path/to/chrome            # or .../Chromium.app/Contents/MacOS/Chromium
export T7_TINT="$PWD/tools/gates/tint_arm/tint_arm.py"
python3 tools/wgsl_gate.py                     # "[gate] tint arm PASS (…tint_arm.py)"

# Windows, cmd
set T7_CHROMIUM=C:\Program Files\Google\Chrome\Application\chrome.exe
set T7_TINT=%CD%\tools\gates\tint_arm\tint_arm.py
python tools\wgsl_gate.py

# Windows, PowerShell
$env:T7_CHROMIUM = "C:\Program Files\Google\Chrome\Application\chrome.exe"
$env:T7_TINT     = "$PWD\tools\gates\tint_arm\tint_arm.py"
python tools\wgsl_gate.py
```

`wgsl_gate.py` invokes `T7_TINT` as `<tool> <file.wgsl>` and reads any output
containing "error" as a failure, so the accepting path is **silent, exit 0**.
Rejection prints Tint's messages and exits 1. A rig that cannot run at all
prints one line and exits 2 — an unrunnable gate is a failed gate.

The tool can also be run directly on any `.wgsl`:

```
python3 tools/gates/tint_arm/tint_arm.py src/cartridges/the_board/realization/world.wgsl
```

## The traps, so nobody rediscovers them

1. **`navigator.gpu` is absent on `about:blank` and present on `file://`.**
   This is the one that costs an afternoon. WebGPU wants a secure context and
   an opaque origin is not one. The tool writes its page to a temp file and
   navigates to it over `file://`.
2. **`chrome://gpu` may say WebGPU is blocklisted while it works.** On a
   machine with no GPU that page reports *"WebGPU has been disabled via
   blocklist or the command line"* and a device is still obtained from a
   secure context with `--enable-unsafe-swiftshader`. Trust `requestAdapter`,
   not that page.
3. **SwiftShader's Vulkan ICD is bundled beside the browser binary** as
   `vk_swiftshader_icd.json`, and must be pointed at explicitly where there is
   no GPU. The tool sets `VK_ICD_FILENAMES` / `VK_DRIVER_FILES` when it finds
   the file next to `T7_CHROMIUM`.
4. **Headless is fine.** This corrects the recipe registered at GATHER_0,
   which used a display: both `--headless=new` and the old headless obtain a
   device on a `file://` page. The display was never the blocker — trap 1 was.
5. **`--dump-dom` cannot be used.** `--virtual-time-budget` does not advance
   real GPU work, so the page is still `PENDING` when the DOM is dumped. The
   protocol, with `awaitPromise`, is the only reliable read.

## Proving it loses (P18: a sidecar, never HEAD)

A gate that cannot lose is a report. Both perturbations below are blessed by
naga and rejected by this arm; both were run when it was written.

**One — the textbook.** Put an implicit-derivative sample under a
per-fragment branch, e.g. in `wall_painting_canvas_fs`:

```wgsl
var tex_color = vec4<f32>(0.0);
if (slot.content_source == 0u) { tex_color = textureSample(painting_array, painting_sampler_filt, in.uv, slot.texture_layer); }
else { tex_color = sample_exhibition(in.uv, slot.texture_layer, slot.content_source); }
```

```
naga : Validation successful
tint : [error] 'dpdx' must only be called from uniform control flow
```

**Two — MIP_0's.** In `sample_exhibition`, move `dpdx`/`dpdy` from the top of
the function to inside the `kind == CONTENT_SOURCE_SNAPSHOT` branch.

```
naga : Validation successful
tint : [error] 'dpdx' must only be called from uniform control flow
       [info]  control flow depends on possibly non-uniform value
```

Restore the file and the arm passes again.

## The other tool here

`probe_copy_external.py` answers DARKROOM_0's Route A question —
whether `copyExternalImageToTexture` works — using the same rig. It is a
probe, not a gate: it prints a verdict and is not wired into `wgsl_gate.py`.
