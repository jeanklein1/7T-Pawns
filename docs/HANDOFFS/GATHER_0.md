# GATHER_0 — THE GATE BEFORE THE TAPS, AND NINE GATHERS WHERE SIXTEEN WERE

*Handoff · authored 2026-09-08 · Claude → CC · Jean holds build, visual, merge, deploy.*
*Filed to `docs/HANDOFFS/GATHER_0.md` while open; the directory dies with the campaign.*

## THE IDEA, IN ONE PARAGRAPH

Two razors in the shadow term, the largest per-fragment cost the meter can see. First:
`calc_spot_light` samples sixteen shadow taps for every fragment and every active light and
only then multiplies by attenuation × cone × facing — a product that is zero over most of a
room for most of its lights. Testing the product first and skipping the taps is 0 × anything:
pixel-identical, and indoors it removes the majority of the spot shadow work. Second: the
4×4 PCF kernel, in both the sun and the spot, reads a 6×6 texel window through sixteen
bilinear compare taps — 64 texel reads, most twice. The same weighted sum is reconstructed
exactly from nine `textureGatherCompare` fetches (36 reads): the model below agrees with the
old kernel to 3e-16 over twenty thousand positions, beyond-edge and clamped ones included.
Nothing leaves the scene; the shadows are the shadows.

## AUTHORITY

Base: `master` at HEAD `dff54963` (MIP_0 / RAZOR_0 settled), fetched 2026-09-08. Rides a
**held branch `claude/gather-0`** (P12: a shader; the visual gate is a screenshot diff at
shadow edges).

| file | blob at base |
| --- | --- |
| `src/cartridges/the_board/realization/world.wgsl` | `2887d0eba49726c014f8d186022bdde08d06bfc7` |
| `tools/gates/glaw2/run.py` | `032a192aaa93113c33f24fa1dca8d15509a863be` |
| `tools/wgsl_gate.py` | `4804c4c0c2cb708edeb66c81188099449b7c718c` (read only — the tint arm is an environment variable) |

**Pinned:** `world.wgsl` (BINDING + MIRROR). Red from U1 until U3 regenerates. HALT classes
(P17) scoped to the unit (P15): unreachable file; stale authority. Everything else
DEFAULT-AND-FLAG. Expected count **1** unless stated. LF only.

**REHEARSED, WITH THE ONE GATE THAT MATTERS LEFT TO U0.** Every FIND at its count on the base
(U2.3 anchors on U1's text, so it reads 0 before U1 and 1 after — by design). G-LAW 2 GREEN
after each unit (330 fn, 335 const, 88 struct — the window struct, `pcf_window`, `pcf_quad`);
`binding_gen` S-3 round-trip identity; the cascade and every gate green after U3. **naga
proves parse and type only** (MIP_0's finding: it performs no derivative-uniformity analysis).
This round's shader edit *depends* on the uniformity rule — a spot sample now sits under a
per-fragment branch — so U0 lights the WGSL gate's dormant Tint arm from your own Dawn
checkout before U1 moves, and proves it loses on a violation. Tint is what Chrome compiles
with; after U0 the gate is the compiler.

## THE RULINGS THIS CAMPAIGN LANDS

1. **The gate before the taps.** `attenuation_sq * cone_falloff * ndotl <= 0` → `continue`,
   before `sample_spot_shadow_pcf`. Bit-identical: the product multiplied the shadow, and
   zero times anything is zero. The sun's kernel has always been gated this way
   (`calc_directional_light`, on `ndotl`); the spot's never was.
2. **A sample under a per-fragment branch takes its level explicitly.** `textureSampleCompare`
   (implicit derivatives) may not sit in non-uniform control flow; `textureSampleCompareLevel`
   may, and on a one-level map returns the same value. The two loops rename their call in U1
   so U1 stands legal on its own; U2 then replaces the loops.
3. **Nine gathers reconstruct sixteen taps exactly.** Each texel's weight in the old kernel
   is the sum of four unit tents — one per tap row and column — so the 6×6 window is
   separable, `Wx[6] × Wy[6]`; gathers on texel corners tile it in 2×2 quads. Proven in the
   model below. The hardware's own bilinear weights are fixed-point, so a penumbra texel may
   differ from the old sum by under 1/256 — below one 8-bit step of the lit color.
4. **The 4-tap arm stays.** Four bilinear taps read sixteen texels; four gathers would read
   sixteen. Nothing to save, nothing touched.
5. **No implicit derivatives remain in either kernel** — `grep -c "textureSampleCompare("
   world.wgsl` reads 0 — so both may be gated per fragment, and the sun's gate could tighten
   later without a uniformity question.
6. **The Tint arm is the shader gate from now on.** `tools/wgsl_gate.py` lights it when
   `T7_TINT` names a tint executable; the Dawn checkout at `C:\dev\dawn` (pin `56f332d7`)
   builds one. Registered as a standing rule: a WGSL handoff's U0 runs the arm and shows it
   lose on a perturbation once.

## THE MODEL (the proof, verbatim; runs anywhere python3 does)

```python
import random, math
random.seed(7); N = 64
def cmp(D, cx, cy, ref):
    cx = min(max(cx, 0), N-1); cy = min(max(cy, 0), N-1)          # ClampToEdge
    return 1.0 if ref <= D[cy][cx] else 0.0
def bilinear_compare(D, qx, qy, ref):                             # a hardware PCF tap: 4 compares, fractional weights
    tx, ty = qx-0.5, qy-0.5; ix, iy = math.floor(tx), math.floor(ty); fx, fy = tx-ix, ty-iy
    return ((1-fx)*(1-fy)*cmp(D,ix,iy,ref) + fx*(1-fy)*cmp(D,ix+1,iy,ref) + (1-fx)*fy*cmp(D,ix,iy+1,ref) + fx*fy*cmp(D,ix+1,iy+1,ref))
def pcf16(D, ux, uy, ref):                                        # the kernel as it was
    return sum(bilinear_compare(D, ux+ox, uy+oy, ref) for oy in (-1.5,-0.5,0.5,1.5) for ox in (-1.5,-0.5,0.5,1.5)) / 16.0
def gather4(D, cornerx, cornery, ref):                            # textureGatherCompare at a texel corner: the quad {c-1, c}
    bx, by = cornerx-1, cornery-1
    return [cmp(D,bx,by+1,ref), cmp(D,bx+1,by+1,ref), cmp(D,bx+1,by,ref), cmp(D,bx,by,ref)]   # x, y, z, w — WGSL's order
def tent(x): return max(0.0, 1.0-abs(x))
def pcf9(D, ux, uy, ref):                                         # the kernel as it is
    px, py = ux-0.5, uy-0.5; ix, iy = math.floor(px), math.floor(py); fx, fy = px-ix, py-iy
    bx, by = ix-2, iy-2
    Wx = [sum(tent((bx+k) - px - o) for o in (-1.5,-0.5,0.5,1.5)) for k in range(6)]
    Wy = [sum(tent((by+k) - py - o) for o in (-1.5,-0.5,0.5,1.5)) for k in range(6)]
    s = 0.0
    for gy in range(3):
        for gx in range(3):
            q = gather4(D, bx+2*gx+1, by+2*gy+1, ref); x0, y0 = 2*gx, 2*gy
            s += Wx[x0]*Wy[y0+1]*q[0] + Wx[x0+1]*Wy[y0+1]*q[1] + Wx[x0+1]*Wy[y0]*q[2] + Wx[x0]*Wy[y0]*q[3]
    return s / 16.0
worst = 0.0
for _ in range(400):
    D = [[random.random() for _ in range(N)] for _ in range(N)]
    for _ in range(50):
        ux, uy, ref = random.uniform(-3, N+3), random.uniform(-3, N+3), random.random()
        worst = max(worst, abs(pcf16(D,ux,uy,ref) - pcf9(D,ux,uy,ref)))
print("max |pcf16 - pcf9| over 20000 positions:", worst)      # 3.3e-16 at the rehearsal
```

## UNITS, IN ORDER

| unit | what | files | commit |
| --- | --- | --- | --- |
| U0 | preflight; light the Tint arm; prove it loses | — | none |
| U1 | the gate; the explicit level | `world.wgsl` | 1 |
| U2 | nine gathers, both kernels; G-LAW 2 admits the builtin | `world.wgsl`, `tools/gates/glaw2/run.py` | 1 |
| U3 | ledgers; OPEN.md; push | `audit/*`, `docs/OPEN.md` | 1 |

---

## U0 — PREFLIGHT, AND THE ARM (no commit)

```
git fetch origin master && git checkout -b claude/gather-0 origin/master
git ls-tree HEAD src/cartridges/the_board/realization/world.wgsl tools/gates/glaw2/run.py   # 2887d0eb… / 032a192a…
grep -c "textureSampleCompare("  src/cartridges/the_board/realization/world.wgsl   # 2 — the spot's two loops, before U1
grep -c "textureGatherCompare"   src/cartridges/the_board/realization/world.wgsl   # 0
```
**The Tint arm.** Build or find `tint` in the Dawn checkout (`C:\dev\dawn`, pin `56f332d7`; the
`tint` target of that build), then:
```
set T7_TINT=C:\dev\dawn\out\<config>\tint.exe          # PowerShell: $env:T7_TINT = "..."
python3 tools/wgsl_gate.py                                 # "[gate] tint arm PASS (…)" on the base — not DORMANT
```
Then prove the arm sees what naga cannot (P18: a sidecar, never HEAD):
```
cp src/cartridges/the_board/realization/world.wgsl world.wgsl.bak
# In calc_spot_light, wrap the existing sample in a per-fragment branch WITHOUT the level rename:
#   if (ndotl > 0.0) { shadow_probe = sample_spot_shadow_pcf(world_pos, geo_normal, i); }   (any use is fine)
python3 tools/wgsl_gate.py                                 # EXPECT the tint arm to FAIL: derivative uniformity (textureSampleCompare under a non-uniform branch)
mv world.wgsl.bak src/cartridges/the_board/realization/world.wgsl
python3 tools/wgsl_gate.py                                 # PASS again
```
If no tint can be built or found in this round: DEFAULT-AND-FLAG — proceed, and state in the
report that rulings 2 and 5 were witnessed by Chrome's own compile at Jean's build instead.
Report the arm's state either way.

---

## U1 — THE GATE, AND THE EXPLICIT LEVEL

### U1.1 — the gate before the spot taps (`src/cartridges/the_board/realization/world.wgsl`)

FIND
```
        // Diffuse
        let ndotl = max(dot(normal, light_dir), 0.0);

        // Per-light shadow from atlas tile
        let shadow = sample_spot_shadow_pcf(world_pos, geo_normal, i);
```
REPLACE
```
        // Diffuse
        let ndotl = max(dot(normal, light_dir), 0.0);

        // GATHER_0 — THE GATE BEFORE THE TAPS. Attenuation, cone and facing
        // multiply the shadow below; where their product is zero the
        // sixteen taps were bought for nothing, and indoors that is most
        // of the room for most of the lights. Skipping them is 0 x
        // anything: pixel-identical. The sun's kernel was always gated this
        // way (calc_directional_light, ndotl). The sample under this branch
        // is non-uniform control flow, so the spot kernel takes its level
        // explicitly — Tint enforces that rule; naga does not (MIP_0's
        // report), which is why the gate's tint arm is lit at U0.
        if (attenuation_sq * cone_falloff * ndotl <= 0.0) { continue; }

        // Per-light shadow from atlas tile
        let shadow = sample_spot_shadow_pcf(world_pos, geo_normal, i);
```

### U1.2 — the spot kernel takes its level explicitly (the map has one level, so the result is identical) (`src/cartridges/the_board/realization/world.wgsl`)

Expected count **2** — replace every match.

FIND
```
                shadow += textureSampleCompare(
```
REPLACE
```
                shadow += textureSampleCompareLevel(
```


### WITNESS — U1
```
grep -c "GATHER_0 — THE GATE BEFORE THE TAPS" src/cartridges/the_board/realization/world.wgsl   # 1
grep -c "textureSampleCompare("               src/cartridges/the_board/realization/world.wgsl   # 0
grep -c "textureSampleCompareLevel("          src/cartridges/the_board/realization/world.wgsl   # 22 (the sun's twenty, the spot's two)
python3 tools/wgsl_gate.py                    # PASS, tint arm PASS
python3 tools/gates/glaw2/run.py              # GREEN — 328 fn, 335 const
```
Commit: `GATHER_0 U1 — the gate before the spot taps (pixel-identical: 0 x shadow); the spot kernel samples at its explicit level, as the uniformity rule requires under a per-fragment branch`

---

## U2 — NINE GATHERS, BOTH KERNELS

### U2.1 — the window and the quad, before the sun kernel (`src/cartridges/the_board/realization/world.wgsl`)

FIND
```
fn sample_shadow_pcf(world_pos: vec3<f32>, normal: vec3<f32>) -> f32 {
```
REPLACE
```
// ═══ GATHER_0 — THE 4x4 PCF: NINE GATHERS WHERE SIXTEEN TAPS WERE ═══════
//
// Sixteen bilinear compare taps at unit spacing read a 6x6 texel window
// (64 texel reads, most of them twice). Each texel's total weight is the
// sum of four unit tents, one per tap in its row and one per tap in its
// column — separable, so the window is Wx[6] x Wy[6] — and that weighted
// sum is reconstructed EXACTLY from nine textureGatherCompare fetches (36
// texel reads): each gather returns the four compares of a 2x2 quad, and
// the quads tile the window when the gathers land on texel CORNERS
// (corner c+1 selects texels {c, c+1}: floor(c + 1 - 0.5) = c, half a
// texel from any boundary). A CPU model of the two kernels agreed to 3e-16
// over 20,000 positions, beyond-edge positions with clamp included; the
// handoff carries the model. The hardware's own bilinear weights are
// fixed-point, so a penumbra texel may differ from the old sum by less
// than 1/256 — under one 8-bit step of the lit color. The 4-tap arm
// (shadow_pcf_taps == 4) is untouched: 4 bilinear taps read 16 texels and
// 4 gathers would read 16, nothing to save.
//
// p-space: texel c's centre sits at c (p = uv * size - 0.5), the taps at
// p + {-1.5, -0.5, 0.5, 1.5}; the window starts at floor(p) - 2. No
// implicit derivatives anywhere here, so the kernel may sit under a
// per-fragment branch (calc_spot_light gates it).
struct PcfWindow {
    base_uv: vec2<f32>,        // the first gather's corner (i0 - 1, in texels), as uv
    wx: array<f32, 6>,
    wy: array<f32, 6>,
};
fn pcf_window(uv: vec2<f32>, texel: f32) -> PcfWindow {
    var w: PcfWindow;
    let p  = uv / texel - 0.5;
    let i0 = floor(p);
    let f  = p - i0;
    w.base_uv = (i0 - 1.0) * texel;
    for (var k: u32 = 0u; k < 6u; k++) {
        let c = f32(k) - 2.0 - f;   // texel (i0 - 2 + k), relative to p, per axis
        var sx = 0.0;
        var sy = 0.0;
        for (var t: u32 = 0u; t < 4u; t++) {
            let o = f32(t) - 1.5;
            sx += max(0.0, 1.0 - abs(c.x - o));
            sy += max(0.0, 1.0 - abs(c.y - o));
        }
        w.wx[k] = sx;
        w.wy[k] = sy;
    }
    return w;
}
// One gathered quad, weighted. WGSL gather order (GLSL's): x = (i, j+1),
// y = (i+1, j+1), z = (i+1, j), w = (i, j), j growing with v.
fn pcf_quad(q: vec4<f32>, wx0: f32, wx1: f32, wy0: f32, wy1: f32) -> f32 {
    return wx0 * wy1 * q.x + wx1 * wy1 * q.y + wx1 * wy0 * q.z + wx0 * wy0 * q.w;
}

fn sample_shadow_pcf(world_pos: vec3<f32>, normal: vec3<f32>) -> f32 {
```

### U2.2 — the sun kernel: sixteen taps become nine gathers (`src/cartridges/the_board/realization/world.wgsl`)

FIND
```
    var s = 0.0;
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>(-1.5, -1.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>(-0.5, -1.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>( 0.5, -1.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>( 1.5, -1.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>(-1.5, -0.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>(-0.5, -0.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>( 0.5, -0.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>( 1.5, -0.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>(-1.5,  0.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>(-0.5,  0.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>( 0.5,  0.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>( 1.5,  0.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>(-1.5,  1.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>(-0.5,  1.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>( 0.5,  1.5) * TEXEL_UV, current_depth);
    s += textureSampleCompareLevel(shadow_map, shadow_sampler, clamped_uv + vec2<f32>( 1.5,  1.5) * TEXEL_UV, current_depth);
    let shadow = s * (1.0 / 16.0);
```
REPLACE
```
    // GATHER_0 — the sixteen taps, as nine gathers (see pcf_window).
    var win = pcf_window(clamped_uv, TEXEL_UV);
    var s = 0.0;
    for (var gy: u32 = 0u; gy < 3u; gy++) {
        for (var gx: u32 = 0u; gx < 3u; gx++) {
            let at = win.base_uv + vec2<f32>(f32(2u * gx), f32(2u * gy)) * TEXEL_UV;
            let q  = textureGatherCompare(shadow_map, shadow_sampler, at, current_depth);
            let x0 = 2u * gx;
            let y0 = 2u * gy;
            s += pcf_quad(q, win.wx[x0], win.wx[x0 + 1u], win.wy[y0], win.wy[y0 + 1u]);
        }
    }
    let shadow = s * (1.0 / 16.0);
```

### U2.3 — the spot kernel: the two loops become nine gathers (`src/cartridges/the_board/realization/world.wgsl`)

FIND
```
    if (light_index < 2u) {
        for (var y: i32 = -2; y <= 1; y++) {
            for (var x: i32 = -2; x <= 1; x++) {
                let offset = vec2(f32(x) + 0.5, f32(y) + 0.5) * texel_size;
                shadow += textureSampleCompareLevel(
                    shadow_map,
                    shadow_sampler,
                    clamped_uv + offset,
                    clamped_depth
                );
            }
        }
    } else {
        for (var y: i32 = -2; y <= 1; y++) {
            for (var x: i32 = -2; x <= 1; x++) {
                let offset = vec2(f32(x) + 0.5, f32(y) + 0.5) * texel_size;
                shadow += textureSampleCompareLevel(
                    spot_shadow_map,
                    shadow_sampler,
                    clamped_uv + offset,
                    clamped_depth
                );
            }
        }
    }
    ```
REPLACE
```
    // GATHER_0 — the sixteen taps, as nine gathers (see pcf_window); the
    // texture still branches by light index, the weights do not.
    var win = pcf_window(clamped_uv, texel_size);
    for (var gy: u32 = 0u; gy < 3u; gy++) {
        for (var gx: u32 = 0u; gx < 3u; gx++) {
            let at = win.base_uv + vec2<f32>(f32(2u * gx), f32(2u * gy)) * texel_size;
            var q: vec4<f32>;
            if (light_index < 2u) {
                q = textureGatherCompare(shadow_map, shadow_sampler, at, clamped_depth);
            } else {
                q = textureGatherCompare(spot_shadow_map, shadow_sampler, at, clamped_depth);
            }
            let x0 = 2u * gx;
            let y0 = 2u * gy;
            shadow += pcf_quad(q, win.wx[x0], win.wx[x0 + 1u], win.wy[y0], win.wy[y0 + 1u]);
        }
    }
    ```

### U2.4 — G-LAW 2 admits the gather (`tools/gates/glaw2/run.py`)

FIND
```
PREDECLARED = {"array", "atomicSub", "dpdx", "dpdy", "textureSampleGrad"}
```
REPLACE
```
#   textureGatherCompare  predeclared builtin function (the GATHER_0 pcf
#               kernels are its first use).
PREDECLARED = {"array", "atomicSub", "dpdx", "dpdy", "textureSampleGrad", "textureGatherCompare"}
```


### WITNESS — U2
```
python3 tools/wgsl_gate.py                    # PASS, tint arm PASS — textureGatherCompare on texture_depth_2d, the loops, the struct
python3 tools/gates/glaw2/run.py              # GREEN — 330 fn, 335 const, 88 struct
grep -c "textureGatherCompare"      src/cartridges/the_board/realization/world.wgsl   # 4 (the banner's mention, the sun's, the spot's two)
grep -c "fn pcf_window\|fn pcf_quad" src/cartridges/the_board/realization/world.wgsl   # 2
grep -c "textureSampleCompareLevel(" src/cartridges/the_board/realization/world.wgsl   # 4 — the 4-tap arm only
python3 tools/binding_gen.py --check | grep S-3    # PASS
```
If the tint arm rejects `textureGatherCompare` or the dynamic indexing of `win.wx` (both are
core WGSL; neither should), HALT U2 and report the exact line — U1 stands on its own.

Commit: `GATHER_0 U2 — the 4x4 PCF as nine gathers (sun and spot), exact in the model; G-LAW 2 admits textureGatherCompare`

---

## U3 — LEDGERS, THE REGISTER, THE PUSH

```
python3 tools/binding_ledger.py && python3 tools/command_census.py && python3 tools/mirror_census.py && python3 tools/organ_ledger.py
python3 tools/binding_ledger.py --check && python3 tools/command_census.py --check && python3 tools/mirror_census.py --check && python3 tools/organ_ledger.py --check
python3 tools/organ_readers.py && python3 tools/organ_gap.py --gate
python3 tools/gates/score/run.py && python3 tools/gates/shell_gate/run.py && python3 tools/gates/glaw2/run.py && python3 tools/gates/sha256_gate/run.py && python3 tools/gates/console_gate/run.py && python3 tools/wgsl_gate.py && sh tools/gates/glaw1/run.sh
python3 tools/binding_gen.py --check      # all PASS; S-6 until the push
```
Rehearsed diffstat: two ledgers, 427/427, pins and line-cites. Run the provenance-stamp
step PLATE_0 U4d registered.

`docs/OPEN.md`, at the top of the register:
```
## GATHER_0 — THE GATE BEFORE THE TAPS; NINE GATHERS (on `claude/gather-0`; Jean's gates open)

The spot shadow is sampled only where attenuation × cone × facing is non-zero (it
always multiplied the result); both 4×4 PCF kernels read their 6×6 window through nine
gathers instead of sixteen bilinear taps, exact in the model. No pixel leaves.

| ruling | where it lives now |
|---|---|
| The gate before the taps (0 × shadow) | `world.wgsl` `calc_spot_light` |
| A sample under a per-fragment branch takes its level explicitly | `sample_spot_shadow_pcf` (U1), then the gathers (U2) |
| Nine gathers reconstruct sixteen taps: separable tent weights on texel corners | `pcf_window`, `pcf_quad`, both kernels |
| The 4-tap arm stays: nothing to save there | `sample_shadow_pcf`, the `shadow_pcf_taps == 4` arm |
| The Tint arm is the shader gate from now on | `tools/wgsl_gate.py` + `T7_TINT` → the Dawn checkout's tint |

### Residuals — GATHER_0
- **The penumbra may differ by under 1/256** from the fixed-point bilinear the hardware
  used to blend — under one 8-bit step of the lit color. The screenshot diff at a shadow
  edge is the witness; a visible difference is a finding, not a tolerance.
- **The sun's gate could tighten** (e.g. skip the kernel where the receiver is beyond the
  map's fade) now that no implicit derivative is left in it; not built — the meter's
  outdoor window decides whether the fade band is worth a branch.
- **The shadow pass encodes its draws each frame** (the main pass is a recorded bundle;
  the shadow pass is not) — a CPU/JS-bridge cost, not a GPU one. Priced: the sun arm as one
  bundle, the atlas arm as one bundle per tile between its scissors (bundles inherit
  scissor state and may not set it). Worth building only if `[PRESENT]` ever names the
  CPU; the machinery is BUNDLE_1's.
- **The orbs run two compute passes a frame** (copy_prev, dynamics); a ping-pong would
  retire the copy and one pass boundary. Small everywhere; measurable only on glass.
```
Commit: `GATHER_0 U3 — ledgers regenerated (world.wgsl pin); OPEN.md: GATHER_0 open on claude/gather-0`
Push: `git push -u origin claude/gather-0`.

---

## JEAN'S GATES

1. **The shadows are the shadows.** Same seed, same mood, the pawn still: screenshots on
   master and the branch at a sunlit edge outdoors, and at a spot-lit edge in a gallery.
   Identical, or different by a hair inside penumbrae only — anything else is a finding.
2. **The meter, indoors first.** A gallery with its spots on: `main_pass` on master vs the
   branch. This is where ruling 1 pays; expect the largest drop of any round so far.
3. **The meter, outdoors and from the ribbon.** `main_pass` again: ruling 3's share.
4. **Firefox** parses `textureGatherCompare` on a depth array through naga — a syntax and
   typing witness, as MIP_0's report narrowed it.
5. The phones: rows 1 and 2 (L48).
