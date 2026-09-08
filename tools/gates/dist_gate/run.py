#!/usr/bin/env python3
"""
DIST GATE (CLOSE_0) — the dist tool runs, end to end, on a tree that has
never run it.

SETTLE_0 gave web_dist.py a check that ran two hundred lines before the
thing it checked existed; on every fresh clone the tool refused before it
could create dist/. Three campaigns' witnesses parsed the tool, grepped the
tool, and never RAN the tool on a tree without dist/ — the one thing that
would have caught it. This gate does that thing, every cascade:

  1. copies the tree (minus .git, out/, dist/, node_modules) to a scratch
     directory, so dist/ does not exist there;
  2. fabricates the build products the way the audit agents did — the
     package IS the shader (the tool's purity check demands it), the glue
     declares the package size (the PAIR_0 check reads it), the wasms are
     noise;
  3. runs `web_dist.py`: exit 0, "WROTE", and the FIRST VISIT block
     printed above WROTE, all required;
  4. runs `web_dist.py --check` on a second fresh copy: exit 0 and NO
     first-visit line (--check writes nothing, so it records nothing);
  5. deletes the scratch directories, whatever happened.

The tree is never touched. Pillow's presence changes the tool's arm, not
its exit; the tool says which arm it took.
"""
import os, re, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
SHADER = os.path.join("src", "cartridges", "the_board", "realization", "world.wgsl")
SKIP = {".git", "out", "dist", "node_modules", "__pycache__"}

def scratch_tree():
    tmp = tempfile.mkdtemp(prefix="t7_dist_gate_")
    dst = os.path.join(tmp, "tree")
    shutil.copytree(ROOT, dst, ignore=lambda d, names: [n for n in names if n in SKIP])
    web = os.path.join(dst, "web")
    shader = open(os.path.join(dst, SHADER), "rb").read()
    with open(os.path.join(web, "the_board.data"), "wb") as fh:
        fh.write(shader)                                    # the package is the shader, byte for byte
    with open(os.path.join(web, "the_board.js"), "w", encoding="utf-8") as fh:
        fh.write("var REMOTE_PACKAGE_SIZE = %d;\nvar remote_package_size = %d;\n" % (len(shader), len(shader)))
    for noise in ("the_board.wasm", "darkroom.js", "darkroom.wasm"):
        with open(os.path.join(web, noise), "wb") as fh:
            fh.write(os.urandom(4096))
    return tmp, dst

def run_tool(dst, *args):
    r = subprocess.run([sys.executable, os.path.join("tools", "web_dist.py"), *args],
                       cwd=dst, capture_output=True, text=True)
    return r.returncode, r.stdout + r.stderr

def main():
    fails = 0
    def row(ok, what):
        nonlocal fails
        print("  [%s] %s" % ("PASS" if ok else "FAIL", what))
        if not ok:
            fails += 1

    tmp, dst = scratch_tree()
    try:
        row(not os.path.exists(os.path.join(dst, "dist")), "the scratch tree has no dist/ — a fresh clone")
        code, out = run_tool(dst)
        row(code == 0, "web_dist.py exits 0 on a fresh tree (exit %d)" % code)
        wrote = out.find("WROTE ")
        first = out.find("FIRST VISIT (the boot set, as written)")
        row(wrote >= 0, "the tool reports WROTE")
        row(first >= 0, "the first-visit block is printed")
        row(0 <= first < wrote, "the first-visit block sits above WROTE (dist is complete when it reads)")
        m = re.search(r"WROTE .*\((\d+) files\)", out)
        row(m is not None and int(m.group(1)) > 0, "WROTE names a file count (%s)" % (m.group(1) if m else "none"))
        if code != 0:
            print("  --- the tool's last lines ---")
            print("\n".join("  " + l for l in out.strip().splitlines()[-8:]))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    tmp, dst = scratch_tree()
    try:
        code, out = run_tool(dst, "--check")
        row(code == 0, "web_dist.py --check exits 0 on a fresh tree (exit %d)" % code)
        row("FIRST VISIT" not in out, "--check prints no first-visit line (it writes nothing)")
        row(not os.path.exists(os.path.join(dst, "dist")), "--check left no dist/ behind")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    print("DIST GATE: %s" % ("GREEN — the dist tool runs on a tree that has never run it" if fails == 0 else "RED — %d failure(s)" % fails))
    return 0 if fails == 0 else 1

if __name__ == "__main__":
    sys.exit(main())
