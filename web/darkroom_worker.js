// ═══ THE DARKROOM WORKER (DARKROOM_1) ══════════════════════════════════════
// Runs darkroom.js (src/darkroom/ built as a Worker module). One job at a
// time, in arrival order: the program's held JPEG bytes come in, level 0 and
// the chain go back as transferred buffers. Everything here is a copy in or
// out of the module's heap; the arithmetic is the module's (core/develop.hpp).
'use strict';
var BUILD = (function () {
  var m = /[?&]v=([^&]+)/.exec(self.location.search);
  return m ? m[1] : '';
})();
function versioned(f) { return f + (BUILD ? '?v=' + BUILD : ''); }
importScripts(versioned('darkroom.js'));

var mod = null;
var pending = [];

createDarkroom({ locateFile: versioned }).then(function (m) {
  mod = m;
  self.postMessage({ ready: true });
  while (pending.length) develop(pending.shift());
});

function develop(job) {
  var t0 = performance.now();
  var n = job.bytes.byteLength;
  var p = mod._malloc(n);
  mod.HEAPU8.set(new Uint8Array(job.bytes), p);
  var out = mod._malloc(8 * 4);
  var ok = mod._t7_develop(p, n, job.res, job.levels, job.bgra ? 1 : 0, out);
  mod._free(p);
  if (!ok) { mod._free(out); self.postMessage({ layer: job.layer, ok: false }); return; }
  // Read after the call: the heap may have grown, and the module's views are fresh.
  var o = out >> 2;
  var w = mod.HEAPU32[o], h = mod.HEAPU32[o + 1], dw = mod.HEAPU32[o + 2], dh = mod.HEAPU32[o + 3];
  var p0 = mod.HEAPU32[o + 4], pc = mod.HEAPU32[o + 5], n0 = mod.HEAPU32[o + 6], nc = mod.HEAPU32[o + 7];
  var level0 = mod.HEAPU8.slice(p0, p0 + n0);   // copies out; the transfer below hands the copy over
  var chain  = mod.HEAPU8.slice(pc, pc + nc);
  mod._free(p0); mod._free(pc); mod._free(out);
  self.postMessage({ layer: job.layer, ok: true, w: w, h: h, dw: dw, dh: dh,
                     level0: level0.buffer, chain: chain.buffer,
                     ms: performance.now() - t0 },
                   [level0.buffer, chain.buffer]);
}

self.onmessage = function (e) {
  if (mod) develop(e.data); else pending.push(e.data);
};
