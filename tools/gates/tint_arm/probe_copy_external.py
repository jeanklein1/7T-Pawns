#!/usr/bin/env python3
# ═══ THE ROUTE A PROBE (DARKROOM_0's residual, RIG_0 U2) ════════════════
#
# DARKROOM_0 priced two roads off the main thread and made one step the
# prerequisite for Route A's handoff: prove that
# `copyExternalImageToTexture` reaches a real texture in the rig. This is
# that step, in the Tint arm's rig — same Chromium, same file:// origin,
# same standard-library protocol client.
#
# WHAT IT ANSWERS AND WHAT IT DOES NOT
# It answers the BROWSER half: does Chromium's WebGPU take an ImageBitmap
# into an rgba8unorm texture, and do the texels arrive intact? It reads the
# texture back through a buffer and compares against the pattern it drew,
# so OPEN means the pixels were checked, not merely that no error fired.
#
# It cannot answer the PROGRAM half, and that half is answered by reading
# the vendored surface instead: emdawnwebgpu's C binding
# (third_party/emdawnwebgpu/.../webgpu.h) exposes exactly
#   wgpuQueueAddRef / OnSubmittedWorkDone / Release / SetLabel /
#   Submit / WriteBuffer / WriteTexture
# and NO external-image copy at all. So even where this probe reads OPEN,
# a C++ caller in the program cannot make the call; Route A would have to
# do the copy from JS against a texture the program owns, which is a
# larger seam than the blit pipeline the residual priced. That reading is
# the round's finding; this probe is its browser half.
#
# Usage:  T7_CHROMIUM=/path/to/chrome python3 probe_copy_external.py
# Prints OPEN or CLOSED with Chromium's exact message. Exit 0 either way:
# a probe reports, it does not gate.

import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tint_arm import Rig, PAGE            # the same rig, the same traps

PROBE_JS = r"""
(async () => {
  if (!navigator.gpu) return { ok:false, why:"navigator.gpu absent (file:// origin?)" };
  let a = await navigator.gpu.requestAdapter();
  if (!a) a = await navigator.gpu.requestAdapter({ forceFallbackAdapter:true });
  if (!a) return { ok:false, why:"no adapter" };
  const d = await a.requestDevice();
  if (!d) return { ok:false, why:"no device" };

  const N = 64;
  // A canvas with a known pattern: opaque, per-texel, easy to verify.
  const cv = document.createElement("canvas");
  cv.width = N; cv.height = N;
  const g = cv.getContext("2d", { willReadFrequently:true });
  const img = g.createImageData(N, N);
  for (let y = 0; y < N; y++) for (let x = 0; x < N; x++) {
    const i = (y * N + x) * 4;
    img.data[i] = x * 4 & 255; img.data[i+1] = y * 4 & 255; img.data[i+2] = 17; img.data[i+3] = 255;
  }
  g.putImageData(img, 0, 0);
  const bmp = await createImageBitmap(cv);

  const tex = d.createTexture({
    size: { width:N, height:N, depthOrArrayLayers:1 },
    format: "rgba8unorm",
    usage: GPUTextureUsage.COPY_DST | GPUTextureUsage.COPY_SRC | GPUTextureUsage.TEXTURE_BINDING |
           GPUTextureUsage.RENDER_ATTACHMENT });

  d.pushErrorScope("validation");
  try {
    d.queue.copyExternalImageToTexture({ source: bmp }, { texture: tex }, { width:N, height:N });
  } catch (e) {
    await d.popErrorScope();
    return { ok:true, open:false, why:"copyExternalImageToTexture threw: " + e };
  }
  const err = await d.popErrorScope();
  if (err) return { ok:true, open:false, why:"validation: " + err.message };

  // Read it back through a buffer — 256-byte row alignment, as the law wants.
  const bpr = Math.ceil(N * 4 / 256) * 256;
  const buf = d.createBuffer({ size: bpr * N, usage: GPUBufferUsage.COPY_DST | GPUBufferUsage.MAP_READ });
  const enc = d.createCommandEncoder();
  enc.copyTextureToBuffer({ texture: tex }, { buffer: buf, bytesPerRow: bpr, rowsPerImage: N },
                          { width:N, height:N, depthOrArrayLayers:1 });
  d.queue.submit([enc.finish()]);
  await buf.mapAsync(GPUMapMode.READ);
  const got = new Uint8Array(buf.getMappedRange()).slice();
  buf.unmap();

  let bad = 0, first = null;
  for (let y = 0; y < N; y++) for (let x = 0; x < N; x++) {
    const o = y * bpr + x * 4;
    const want = [x * 4 & 255, y * 4 & 255, 17, 255];
    for (let c = 0; c < 4; c++) if (got[o + c] !== want[c]) {
      if (bad === 0) first = { x, y, c, got: got[o + c], want: want[c] };
      bad++;
    }
  }
  return { ok:true, open:true, mismatches:bad, first,
           adapter: { vendor:a.info && a.info.vendor, architecture:a.info && a.info.architecture } };
})()
"""


def main():
    rig = Rig(PAGE)
    try:
        r = rig.run(PROBE_JS)
    except Exception as e:
        rig.stop()
        print("CLOSED — the rig could not run: %s" % e)
        return 0
    rig.stop()

    if not isinstance(r, dict) or not r.get("ok"):
        print("CLOSED — no WebGPU device: %s" % ((r or {}).get("why") if isinstance(r, dict) else "no result"))
        return 0
    if not r.get("open"):
        print("CLOSED — %s" % r.get("why"))
        return 0
    if r.get("mismatches"):
        print("CLOSED — the call succeeded but %d byte(s) came back wrong; first %s"
              % (r["mismatches"], json.dumps(r.get("first"))))
        return 0
    print("OPEN — copyExternalImageToTexture reached the texture and 64x64 texels read back exact "
          "(adapter %s/%s)." % (r.get("adapter", {}).get("vendor"), r.get("adapter", {}).get("architecture")))
    print("NOTE — the browser half only. The vendored emdawnwebgpu C binding exposes no")
    print("       external-image copy on Queue (AddRef/OnSubmittedWorkDone/Release/SetLabel/")
    print("       Submit/WriteBuffer/WriteTexture), so a C++ caller in the program cannot make")
    print("       this call; Route A would have to copy from JS into a texture the program owns.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
