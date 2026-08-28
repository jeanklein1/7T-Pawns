# WebKit WebGPU — depth-only render bundle produces an incorrect depth attachment

**DRAFT. Not filed.** Filing is Jean's — it is an outward-facing post under
the project's name. When it has a URL, record it here and link it from
`renderer.hpp`'s `main_bundle_ready` banner.

File at: https://bugs.webkit.org — Component: WebGPU

---

## Summary

A `GPURenderBundle` created with **zero color formats** and a
depth-stencil format, executed with `ExecuteBundles` inside a depth-only
render pass, produces an incorrect depth attachment. The bundle is
accepted at validation and executes; the resulting depth texture is
wrong. Re-issuing the identical draw list directly on the pass encoder —
same pipelines, same bind groups, same indirect draws — produces the
correct result.

## Environment

- Safari / WebKit WebGPU, iPad and iPhone 14 (both affected)
- Chrome on Windows (DXC) and Chrome on Pixel (SPIR-V): both correct with
  the same code and the same shader module

## Expected

`ExecuteBundles` on a depth-only bundle writes the same depth values the
equivalent direct encoding writes.

## Actual

The depth attachment ends up such that a subsequent PCF shadow compare
treats every sample as occluded — consistent with depth near 0.0 where
1.0-to-scene-depth was expected. The visible result is a fully black
world. **No `uncapturederror` fires and the device is not lost.**

## Bundle descriptor

```js
device.createRenderBundleEncoder({
  label: "Shadow Sun Bundle",
  colorFormats: [],            // zero color attachments
  depthStencilFormat: "depth32float",
  sampleCount: 1,
});
```

The pass it executes in matches: `colorAttachments: []`, one
depthStencilAttachment, `depthLoadOp: "clear"`, `depthClearValue: 1.0`,
`depthStoreOp: "store"`.

The bundle records four `setBindGroup` calls (one with a dynamic offset),
then a mix of `drawIndexed` and `drawIndexedIndirect` across several
pipelines.

## Isolation

Two independent boot switches were added to the application and each was
toggled alone on the affected device:

| switch | effect | result on iOS |
|---|---|---|
| skip the sun pass's **draw list**, keep the pass and its clear | the depth-only bundle is never executed; the **color** bundle still is | **correct render** |
| encode **both** passes directly instead of via `ExecuteBundles` | the same draws are issued, no bundle is executed | **correct render** |
| neither | — | black |

The intersection of the two is a single `ExecuteBundles` call on the
depth-only bundle. A second render bundle in the same frame — one color
format plus depth, ~60 recorded calls — executes correctly on the same
device, so the fault is specific to the zero-color-format case rather
than to bundles generally.

## Minimal reproduction

Not yet reduced to a standalone page. The isolation above is from the
full application; a minimal repro would be: one depth-only pass, one
depth-only bundle containing a single `drawIndexed` of a triangle,
readback of the depth texture, compared against the same draw encoded
directly.

## Workaround

Encode depth-only passes directly. We deleted the depth-only bundle on
all browsers rather than fork per-implementation — it carried about a
dozen CPU calls, which is not worth a second code path.
