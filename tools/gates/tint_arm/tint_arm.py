#!/usr/bin/env python3
# ═══ THE TINT ARM — CHROMIUM'S OWN DAWN COMPILES THE MODULE ═════════════
#
# GATHER_0 ruling 6: the Tint arm is the shader gate from now on. naga
# proves parse, scope and type; it performs NO derivative-uniformity
# analysis, and MIP_0's report measured that — a textbook violation
# (`textureSample` under a non-uniform `if`) validates successfully under
# naga-cli 30.0.1. Tint is what Chrome compiles with, and Tint enforces it.
#
# THIS TOOL NEEDS NO DAWN CHECKOUT. Chromium already contains Dawn and
# Tint, and WebGPU's `createShaderModule` + `getCompilationInfo()` is
# Tint's own verdict on the real module. The browser is driven over the
# DevTools protocol with a WebSocket client written here in the standard
# library, so the tool's whole dependency list is: python3, and a
# Chromium. No node, no Playwright, no display.
#
# ── THE CONTRACT (tools/wgsl_gate.py's T7_TINT arm) ────────────────────
#   tint_arm.py <file.wgsl>
#     exit 0 and PRINT NOTHING           the module is accepted
#     exit 1 and print Tint's messages   the module is rejected
#     exit 2 and print one line          the rig could not run
#   The gate reads any output containing "error" as a failure, which is
#   why the accepting path is silent.
#
# ── THE TRAPS, WRITTEN DOWN RATHER THAN REMEMBERED ─────────────────────
#   1. `navigator.gpu` IS ABSENT ON about:blank AND PRESENT ON file://.
#      This is the one that costs an afternoon. WebGPU wants a secure
#      context; an opaque origin is not one. The page below is written to
#      a temp file and loaded over file://.
#   2. `chrome://gpu` MAY SAY WEBGPU IS BLOCKLISTED WHILE IT WORKS. On a
#      machine with no GPU it reports "WebGPU has been disabled via
#      blocklist or the command line" and a device is still obtained from
#      a secure context with --enable-unsafe-swiftshader. Do not trust
#      that page over an actual requestAdapter.
#   3. SWIFTSHADER'S VULKAN ICD IS BUNDLED BESIDE THE BINARY and must be
#      pointed at explicitly where no GPU exists: vk_swiftshader_icd.json,
#      via VK_ICD_FILENAMES / VK_DRIVER_FILES. Done below when the file is
#      found next to the browser.
#   4. HEADLESS IS FINE — and this is a correction to the recipe GATHER_0
#      registered, which used a display. Both --headless=new and the old
#      headless obtain a device on a file:// page; the display was never
#      the blocker, trap 1 was.
#   5. --dump-dom CANNOT BE USED. --virtual-time-budget does not advance
#      real GPU work, so the page is still PENDING when the DOM is dumped.
#      The protocol, with awaitPromise, is the only reliable read.
#
# ── THE BROWSER IS NAMED, NEVER GUESSED ────────────────────────────────
#   T7_CHROMIUM must name the browser binary. An unrunnable gate is a
#   failed gate (tools/wgsl_gate.py's own ruling), so absence is exit 2
#   with the variable named, never a silent pass.
#
# ── PROVING IT LOSES (P18: a sidecar, never HEAD) ──────────────────────
#   Two perturbations, both of which naga blesses. See README.md.

import base64
import hashlib
import json
import os
import shutil
import socket
import struct
import subprocess
import sys
import tempfile
import time
import urllib.request

TIMEOUT_S = 180


def _fail(msg, code=2):
    print(msg)
    sys.exit(code)


# ── A WebSocket client, standard library only ──────────────────────────
# CDP is JSON over WebSocket. The client half of RFC 6455 is a handshake
# and a masked frame; both are short enough to own rather than depend on.
class WS:
    def __init__(self, url, timeout=TIMEOUT_S):
        if not url.startswith("ws://"):
            raise ValueError("only ws:// is spoken here: %r" % url)
        rest = url[len("ws://"):]
        hostport, _, path = rest.partition("/")
        host, _, port = hostport.partition(":")
        self.sock = socket.create_connection((host, int(port or 80)), timeout=timeout)
        self.sock.settimeout(timeout)
        key = base64.b64encode(os.urandom(16)).decode()
        req = (
            "GET /%s HTTP/1.1\r\nHost: %s\r\nUpgrade: websocket\r\n"
            "Connection: Upgrade\r\nSec-WebSocket-Key: %s\r\n"
            "Sec-WebSocket-Version: 13\r\n\r\n" % (path, hostport, key)
        )
        self.sock.sendall(req.encode())
        buf = b""
        while b"\r\n\r\n" not in buf:
            chunk = self.sock.recv(4096)
            if not chunk:
                raise IOError("the browser closed the protocol socket during the handshake")
            buf += chunk
        head = buf.split(b"\r\n\r\n", 1)[0].decode("latin-1")
        if "101" not in head.split("\r\n")[0]:
            raise IOError("the protocol handshake was refused: %s" % head.split("\r\n")[0])
        accept = hashlib.sha1((key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").encode()).digest()
        if base64.b64encode(accept).decode().lower() not in head.lower():
            raise IOError("the protocol handshake's accept key did not verify")
        self._rest = buf.split(b"\r\n\r\n", 1)[1]

    def _read(self, n):
        while len(self._rest) < n:
            chunk = self.sock.recv(65536)
            if not chunk:
                raise IOError("the browser closed the protocol socket")
            self._rest += chunk
        out, self._rest = self._rest[:n], self._rest[n:]
        return out

    def send(self, obj):
        payload = json.dumps(obj).encode("utf-8")
        n = len(payload)
        head = bytearray([0x81])           # FIN + text
        if n < 126:
            head.append(0x80 | n)
        elif n < (1 << 16):
            head.append(0x80 | 126)
            head += struct.pack(">H", n)
        else:
            head.append(0x80 | 127)
            head += struct.pack(">Q", n)
        mask = os.urandom(4)               # a client frame must be masked
        head += mask
        masked = bytes(b ^ mask[i & 3] for i, b in enumerate(payload))
        self.sock.sendall(bytes(head) + masked)

    def recv(self):
        data = b""
        while True:
            b0, b1 = self._read(2)
            fin, opcode = b0 & 0x80, b0 & 0x0F
            ln = b1 & 0x7F
            if ln == 126:
                ln = struct.unpack(">H", self._read(2))[0]
            elif ln == 127:
                ln = struct.unpack(">Q", self._read(8))[0]
            if b1 & 0x80:                  # a server frame is never masked
                self._read(4)
            data += self._read(ln)
            if opcode == 0x8:
                raise IOError("the browser closed the protocol connection")
            if fin:
                return json.loads(data.decode("utf-8"))

    def close(self):
        try:
            self.sock.close()
        except Exception:
            pass


class Rig:
    """A headless Chromium with a page on a file:// origin, over CDP."""

    def __init__(self, page_html):
        self.binary = os.environ.get("T7_CHROMIUM", "")
        if not self.binary or not os.path.isfile(self.binary):
            _fail("tint-arm: FAIL — set T7_CHROMIUM to a Chromium/Chrome binary "
                  "(an unrunnable gate is a failed gate).")
        self.tmp = tempfile.mkdtemp(prefix="t7-tint-arm-")
        self.page = os.path.join(self.tmp, "page.html")
        with open(self.page, "w", encoding="utf-8", newline="\n") as f:
            f.write(page_html)
        env = dict(os.environ)
        icd = os.path.join(os.path.dirname(self.binary), "vk_swiftshader_icd.json")
        if os.path.isfile(icd):            # trap 3
            env["VK_ICD_FILENAMES"] = icd
            env["VK_DRIVER_FILES"] = icd
        profile = os.path.join(self.tmp, "profile")
        self.proc = subprocess.Popen(
            [self.binary, "--headless=new", "--remote-debugging-port=0",
             "--user-data-dir=" + profile, "--no-first-run", "--no-default-browser-check",
             "--no-sandbox", "--disable-gpu-sandbox",
             "--enable-unsafe-webgpu", "--enable-unsafe-swiftshader",
             "--use-vulkan=swiftshader", "--use-angle=swiftshader",
             "about:blank"],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=env)
        self.port = self._await_port(profile)
        self.ws = WS(self._page_target())

    def _await_port(self, profile):
        f = os.path.join(profile, "DevToolsActivePort")
        for _ in range(600):
            if os.path.isfile(f):
                try:
                    first = open(f, encoding="utf-8").read().split("\n")[0].strip()
                    if first:
                        return int(first)
                except Exception:
                    pass
            if self.proc.poll() is not None:
                self.stop()
                _fail("tint-arm: FAIL — the browser exited before the protocol port opened.")
            time.sleep(0.05)
        self.stop()
        _fail("tint-arm: FAIL — the browser never opened a protocol port.")

    def _get(self, path):
        # The loopback must not go through any configured proxy.
        opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
        with opener.open("http://127.0.0.1:%d%s" % (self.port, path), timeout=30) as r:
            return json.loads(r.read().decode("utf-8"))

    def _page_target(self):
        for _ in range(200):
            try:
                for t in self._get("/json/list"):
                    if t.get("type") == "page" and t.get("webSocketDebuggerUrl"):
                        return t["webSocketDebuggerUrl"]
            except Exception:
                pass
            time.sleep(0.05)
        self.stop()
        _fail("tint-arm: FAIL — the browser exposed no page target.")

    def _call(self, method, params=None, ident=[0]):
        ident[0] += 1
        me = ident[0]
        self.ws.send({"id": me, "method": method, "params": params or {}})
        deadline = time.time() + TIMEOUT_S
        while time.time() < deadline:
            msg = self.ws.recv()
            if msg.get("id") == me:
                if "error" in msg:
                    raise IOError("%s: %s" % (method, msg["error"]))
                return msg.get("result", {})
        raise IOError("%s: the browser did not answer within %ds" % (method, TIMEOUT_S))

    def run(self, expression):
        """Navigate to the file:// page (trap 1) and evaluate, awaiting the promise."""
        self._call("Page.enable")
        self._call("Page.navigate", {"url": "file://" + self.page.replace(os.sep, "/")})
        time.sleep(0.4)
        r = self._call("Runtime.evaluate", {
            "expression": expression, "awaitPromise": True,
            "returnByValue": True, "timeout": TIMEOUT_S * 1000})
        if "exceptionDetails" in r:
            raise IOError("the page threw: %s" % json.dumps(r["exceptionDetails"])[:400])
        return r.get("result", {}).get("value")

    def stop(self):
        try:
            self.ws.close()
        except Exception:
            pass
        try:
            self.proc.terminate()
            self.proc.wait(timeout=20)
        except Exception:
            try:
                self.proc.kill()
            except Exception:
                pass
        shutil.rmtree(self.tmp, ignore_errors=True)


PAGE = "<!doctype html><meta charset=utf-8><title>tint arm</title><body></body>"

COMPILE_JS = """
(async () => {
  if (!navigator.gpu) return { ok:false, why:"navigator.gpu absent (is the page on a file:// origin?)" };
  let a = null;
  try { a = await navigator.gpu.requestAdapter(); } catch (e) { return { ok:false, why:"requestAdapter: "+e }; }
  if (!a) { try { a = await navigator.gpu.requestAdapter({ forceFallbackAdapter:true }); } catch (e) {} }
  if (!a) return { ok:false, why:"no adapter, and no fallback adapter" };
  let d = null;
  try { d = await a.requestDevice(); } catch (e) { return { ok:false, why:"requestDevice: "+e }; }
  if (!d) return { ok:false, why:"no device" };
  d.pushErrorScope("validation");
  const m = d.createShaderModule({ code: %s });
  const info = await m.getCompilationInfo();
  const scoped = await d.popErrorScope();
  return { ok:true,
           messages: Array.from(info.messages).map(x => ({ type:x.type, line:x.lineNum, pos:x.linePos, message:x.message })),
           scoped: scoped ? String(scoped.message) : null };
})()
"""


def main():
    if len(sys.argv) != 2:
        _fail("usage: tint_arm.py <file.wgsl>")
    path = sys.argv[1]
    if not os.path.isfile(path):
        _fail("tint-arm: FAIL — no such file: %s" % path)
    src = open(path, encoding="utf-8").read()

    rig = Rig(PAGE)
    try:
        out = rig.run(COMPILE_JS % json.dumps(src))
    except Exception as e:
        rig.stop()
        _fail("tint-arm: FAIL — the rig could not run: %s" % e)
    rig.stop()

    if not isinstance(out, dict) or not out.get("ok"):
        why = (out or {}).get("why", "no result") if isinstance(out, dict) else "no result"
        _fail("tint-arm: FAIL — no WebGPU device: %s" % why)

    msgs = out.get("messages") or []
    bad = [m for m in msgs if m.get("type") == "error"]
    if bad or out.get("scoped"):
        print("tint rejected %s" % path)
        for m in msgs:
            print("  [%s] %s:%s  %s" % (m.get("type"), m.get("line"), m.get("pos"),
                                        str(m.get("message", "")).strip()))
        if out.get("scoped"):
            print("  scoped: %s" % str(out["scoped"]).strip().split("\n")[0])
        sys.exit(1)
    sys.exit(0)      # accepted: silent, by the contract


if __name__ == "__main__":
    main()
