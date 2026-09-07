# POSTCARD_0 — A PICTURE LEAVES THE WALL

*Handoff · authored 2026-09-06 · Claude → CC · Jean holds build, visual, merge, deploy, naming.*
*Filed to `docs/HANDOFFS/POSTCARD_0.md` while open; the directory dies with the campaign.*

## THE IDEA, IN ONE PARAGRAPH

A visitor stands before a picture — a painting the exhibition hung or a photograph the world
took of itself, on a wall indoors or a fan outdoors — and the glass says so, in words. A key
(P) or a tap on the words takes it: one layer of the exhibition array leaves the GPU, the
shell turns it into a JPEG cropped and shaped **as the picture hangs**, and the visitor's own
device is asked to keep it — the share sheet where there is one (Mail with the picture
attached and the address field theirs; Messages; AirDrop; Photos), a download where there is
not. No server, no readback until the press, nothing the site sends on anyone's behalf. This
is the mechanism OPEN.md priced at DOORS_0 — *"a fifth readback machine, a channel swap, a
megabyte a shot. Not built until a measurement asks"* — and Jean's ask is the measurement.
The roll's miniatures (the same residual's other reader) now have their machine too.

## AUTHORITY

Base: `master` at HEAD `e35b8d5d8a44e54bb23b9558ed9047dac2754423` (HOME_0).
Rides a **held branch `claude/postcard-0`** (P12: a texture usage, a buffer, a copy in the
frame encoder, a key, a badge and a picture are runtime behaviour a visual gate can catch).
Jean authorises the merge; the branch dies on merge (ATTIC LAW).

| file | blob at base |
| --- | --- |
| `src/cartridges/the_board/bodies/gallery.hpp` | `3ccc80cbf5605ed7ff80aec7fef91a224a380efc` |
| `src/cartridges/the_board/realization/state.hpp` | `72fff9ff83a18f7df2c39267316eee91bc01c651` |
| `src/cartridges/the_board/cartridge.hpp` | `9c2e0bcf465381185e1bb152065a1ba5adcb0187` |
| `src/cartridges/the_board/contracts/spine_state.hpp` | `0b5a3e07385bda9e58accbde9d4c16ecc4664506` |
| `src/cartridges/the_board/direction/input.hpp` | `37a71e89f390510aec49bf63872b8851544fde46` |
| `src/cartridges/the_board/organ_boundary.inc` | `30b3aa18d38cfc108f40836101735f76da789d33` |
| `src/console/organ_registry.hpp` | `6cdf0e7bec8143ae451971dcb4eb4a04f993806a` |
| `tools/binding_schema.py` | `3e5529701c043107dfe74ff05f3e7e95be1f044d` |
| `tools/binding_gen.py` | `82848c93b4fca3b603a7ae22e7dfc264ca48fbaf` |
| `web/index.html` | `37066acff139cfb7f3abcf48468feb96562bd862` |
| `docs/COPY.md` | `4bd9f993cff3a094f06e2b8e29365b29e3b2ecec` |
| `docs/OPEN.md` | `4066dc4e31ae3ed486170b6d5d36f44aa1e4c410` |
| `src/core/ride_face.hpp` | `0423e5c94036ac75354c9f5b6485e3123b60563c` (read only — the idiom) |
| `src/core/postcard_face.hpp` | NEW at U2 (`3cb95eaaf5f025413a95de46ca3b841b5dc83274` when written verbatim) |

**Pinned files.** `state.hpp` (BINDING + MIRROR), `gallery.hpp` (BINDING + COMMAND),
`cartridge.hpp` (COMMAND), `spine_state.hpp` (MIRROR); `audit/ORGAN.md` carries
`gallery.hpp` line numbers. The ledger gates are red from U1 until U5 regenerates them in
order. Expected; say so. `binding_gen.py --check`'s S-6 is red until the branch is pushed
(it wants a clean tree at a pushed tip); every other row of that census is green from U2 on.

**HALT classes (P17), scoped to the unit (P15):** unreachable file; stale authority (blob
differs AND the FIND does not match). Everything else DEFAULT-AND-FLAG. Line numbers are
hints; boundaries are symbols (P2). Expected match count **1** unless stated. LF only, no
BOM (L35) — the new file too.

**REHEARSED, NOT RECITED.** Every edit below was applied to a clone of this base, in this
order, every FIND at its stated count. At each unit's commit G-LAW 1 was GREEN and the TU
gate PASS with zero diagnostics; after U5 every census check and every gate in CLAUDE.md's
table passed, except the two the rehearsal sandbox cannot run: the WGSL gate (naga absent —
`world.wgsl` is untouched and its sha256 pin unchanged, G-LAW 2 GREEN) and the collection
gate (wants a built `dist/`). The counts in each WITNESS are the rehearsed tree's. The
shell's two scripts parse under `node --check`; the EM_ASM body and the badge's `deliver`
were exercised under stubs (BGRA in → RGBA out, alpha 255, the name read from the heap, the
three shapes a picture can take, the hand-off's download arm reached with the right name).

## THE RULINGS THIS CAMPAIGN LANDS

1. **Before a picture is a CPU predicate** (`pick_picture_before`, `gallery.hpp`), computed
   from what is already home: the slot array the CPU authors, the pose R1 harvests, the
   point. Two questions in order — the ZONE (the picture's own footprint pushed forward,
   L13's zone, both gallery forms through the one struct) and the EYE (the candidate
   nearest the line of sight, inside a 45° cone; looking away clears it). The GPU learns no
   word. The point must be the pawn; the caller answers for that.
2. **The affordance is a badge, and on glass the badge IS the door.** The touch grammar is
   full (both halves, taps, pairs, pinch); a right-half hold past `TAP_MS` is unassigned
   today, but would fire under a finger resting before a look-drag. The badge takes pointer
   events (unlike T7_RIDE), sits top-centre where no thumb rests, and takes none while
   hidden. On a keyboard P is the door and the click is P's second mouth.
3. **One door, two mouths, folded at the boundary.** P raises `InputState::take_pending`
   (`request_take`, guarded by `KeyState::take_held` — KeyDown fires on autorepeat and a take
   is a strike); the badge raises the registry's `g_take_pending` (`gallery_take`, the visit
   door's shape without a parameter). `organ_flush` spends both, every frame, whether or not
   a picture answers, and `request_postcard` refuses in words (P6) or arms the machine.
4. **The postcard machine is the readback grammar with two named departures.** IDLE →
   COPIED (R11, `CopyTextureToBuffer` of the chosen layer, same encoder as the three buffer
   copies) → MAPPING (R1, captureless `MapAsync`) → IDLE. On demand, never per frame. And no
   generation guard: a state readback drops a callback from a world that ended because its
   bytes would become the new world's state; a postcard's bytes are a moment, and a picture
   pressed in a world that ended is still the picture pressed. Teardown clears only an
   arming not yet spent.
5. **The seam carries RGBA and provenance, once** (`core/postcard_face.hpp`): the texel
   order (BGRA on Chrome's preferred format) and a photograph's clear-alpha sky stop at the
   seam; the shell receives an opaque RGBA square plus the crop, the hung aspect, the kind
   and a filename stem, read from the heap by bytes (no runtime string helper is exported).
6. **The postcard is the picture as it hangs.** A painting is cropped to `uv_scale` (padded
   into its square at the origin by `authored_stage_decoded_image`); a photograph is stored
   square (`photographer_vp` is built with the shot's aspect into the full target) and hung
   at `scale_x / scale_y`, so the shell resamples it to that. JPEG at 0.92.
7. **One hand-off, two cameras.** DOORS_2's share-or-download moves into `window.T7_KEEP`;
   `#ctlPhoto` calls it; the postcard calls it. The message leaves from the visitor's own
   mail, never from the site (OPEN.md's open-relay finding stands).
8. **No world-side effect.** The badge is the acknowledgment; the world is not a camera.
9. **The estate learns its first texture→buffer copy.** One `RESOURCES` row; the reach
   census gains `.buffer =` beside `.texture =`, its perturbation shown (R-2 flagged the row
   an orphan without it).
10. **Four numbers, constants not dials** (`TAKE_REACH_MULT`, `TAKE_REACH_MIN_WU`,
    `TAKE_SLACK_MULT`, `TAKE_CONE_COS`): control-panel material, enrolled nowhere — the
    pilot's rule, until a measurement asks.

## THE MECHANISM AUDIT (L16) — the facts each edit stands on, and the grep

- **The wall has no CopySrc.** `grep -n 'makeTextureArray("Exhibition"' -A2 state.hpp` →
  `CopyDst | TextureBinding`. The photographer's staging (`"Snapshot Staging"`) and the
  offscreen colour carry `CopySrc` already; the hung layer is the EXHIBITION's (promotion
  copies staging → exhibition), so the wall is what the postcard copies.
- **The readback grammar.** `grep -n "ReadbackState" cartridge.hpp`: four `IDLE, COPIED,
  MAPPING` machines; copies in `phase_witness_capture` (R11, after `dispatch_compute`),
  maps in `phase_witness_harvest` (R1, captureless lambda + `this` userdata), COPIED
  cancelled at the teardown arm beside `camera_pose_ = CameraPose{}`.
- **The vendored surface has the copy.** `grep -n "CopyTextureToBuffer\|struct
  TexelCopyBufferInfo" third_party/emdawnwebgpu/emdawnwebgpu_pkg/webgpu_cpp/include/webgpu/webgpu_cpp.h`
  → `CopyTextureToBuffer(TexelCopyTextureInfo const*, TexelCopyBufferInfo const*, Extent3D
  const*)`; `TexelCopyBufferInfo { TexelCopyBufferLayout layout; Buffer buffer; }`. The tree
  already speaks `TexelCopyTextureInfo` + `TexelCopyBufferLayout` at `WriteTexture`.
- **bytesPerRow.** `Dim::PAINTING_RESOLUTION = 512`; 512 × 4 = 2048 = 8 × 256. A
  `static_assert` says so at U2.3.
- **Texel order.** `colorFormat_ = BGRA8Unorm` by default, set in `initOffscreenResources`;
  `grep -c "bool need_swap = (colorFormat_ ==" state.hpp` → **2** — the two inline homes
  U2.4 folds into `exhibition_is_bgra()`.
- **The slot knows the shape.** `GPUPaintingSlot` (state.hpp): `position`, `forward`,
  `scale_x/y`, `uv_scale_x/y`, `texture_layer`, `content_source`, `is_active`. `rec.uv_scale_x
  = (float)dst_w / RES` with the image written at `origin = { 0, 0, layer }`
  (`authored_stage_decoded_image`, `upload_authored_painting`). Snapshot slots write
  `uv_scale = 1.0` and `photographer_vp.m = build_lookat_vp(eye, aim_pt, cfg.fov_rad,
  cfg.aspect_ratio)` renders into the full 512² target — stored anamorphic, hung at
  `scale_x = sqrt(area * aspect)`, `scale_y = scale_x / aspect`.
- **Which picture hangs where.** `exhibition_name[layer].disk_index` → `authored_disk_manifest`
  (paths like `paintings/PAINTING_12.jpg`; `authored_extract_number` reads the number);
  `slot_provenance[slot].shot_type` → `SHOT_TYPE_NAMES`. Both CPU-only.
- **The eye's formula.** `gallery_unwatched` (gallery.hpp): view direction
  `-(cos_el·sin_az, sin_el, cos_el·cos_az)`, "verified against world.wgsl's
  build_view_projection_matrix and compose_camera_position_from_orbit". `camera_pose_`
  (`CameraPose { eye[3], azimuth, elevation, valid }`) is harvested at R1 every frame
  (PLUMB_0 C1) and reset at teardown.
- **In front of.** `begin_visit`: `tx = px + fx * off` with `off = max(PILOT_STANDOFF_MIN_WU
  (4), span * PILOT_STANDOFF_MULT (1.4))` — `forward` is the viewer's side, witnessed by
  VISIT_0's visual gate. `TAKE_REACH_MULT 2.2 / MIN 8` cover that standing point with margin.
- **The point.** `point_.host == PointHost::PAWN`, `point_.x`, `point_.z` — the pilot reads
  exactly these (`pilot_tick`).
- **Keys.** The GLFW port delivers `GLFW_KEY_P` untranslated (`inject_key_event` converts
  A–Z to characters only); `inject_key_event` raises KeyDown on `GLFW_REPEAT` too — hence
  `take_held`, `pulse_held`'s reason. `grep -n "GLFW_KEY_" input.hpp` → P is free (R
  retired with VISIT_0).
- **Touch.** `TOUCH_TARGET = "Module['canvas']"`: a tap on a sibling element never reaches
  the touch grammar. `on_touch`'s lift is a tap only if `!slopped && (now - t0) <= TAP_MS
  (220)`: a longer solo hold is unassigned — free, and not taken (ruling 2).
- **The door's shape.** `g_visit_pending` / `take_visit` (organ_registry.hpp) consumed in
  `organ_flush` (organ_boundary.inc, "THE VISIT DOOR"), which pawn.cpp calls once a frame
  before input and update; `InputState` lives in `contracts/spine_state.hpp`;
  `clear_input_deltas` clears deltas by name, never the pending intents.
- **EM_ASM.** `-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']` only (CMakeLists.txt) — no
  `UTF8ToString`; console.hpp reads strings "bytes into stack buffers through HEAPU8" and
  warns that the preprocessor splits a body on a bare comma. `HEAPU8` is a module-scope
  name inside EM_ASM bodies (precedent: `request_device_web`). The TU gate's stub makes
  EM_ASM a no-op, so the real arm is Jean's emcc; the body was exercised under node.
- **The shell.** `T7_RIDE` is built on first `set`, styled inline, `pointer-events:none`
  by charter (bottom-centre, taps fall through); `note()` is a global function of the
  second script; `#ctlPhoto` owns DOORS_2's share-or-download; the shell gate's check 6
  scans `organ_panel.js` only, so `index.html`'s C-ABI calls are ungated — the badge calls
  `Module._gallery_take` (KEEPALIVE exports it) and says so if absent.
- **The estate census.** `binding_gen.py` parses `makeBuffer(label, size, usage)` sites
  (three args, exact strings) into RESOURCES rows, and one-line `wgpu::Buffer name() const {
  return member_; }` accessors into its accessor map; reach is proven through `Copy*`/
  `Write*`/`MapAsync` argument text, `.texture =` and `.view =` — NOT `.buffer =`.
  Perturbation, shown at the rehearsal: with the row and the copy site but without U2.6,
  `--check` reads `[FAIL] R-2 … 1 orphan(s) — FLAGGED: postcardReadbackStaging_`; with
  U2.6, `96 rows, 0 orphan(s)`. A lazily-created accessor with a body is invisible to that
  map, which is why U2.3 splits the door into a one-line accessor and an `ensure_`.
- **The pins.** `grep -n "sha256:" audit/*.md`: `state.hpp` in BINDING + MIRROR,
  `gallery.hpp` in BINDING + COMMAND, `cartridge.hpp` in COMMAND, `spine_state.hpp` in
  MIRROR. `audit/ORGAN.md` cites `gallery.hpp:554…557` by line — U1.1's include shifts them
  by one. Regeneration order: `binding_ledger` → `command_census` → `mirror_census` (it
  pins BINDING_LEDGER.md) → `organ_ledger`.
- **`binding_gen.py --write` is byte-identical.** The estate row reaches no lane table:
  MANIFEST.md, `limits_floor.gen.inc` and `features_wallet.gen.inc` rewrite unchanged.

## UNITS, IN ORDER — every commit compiles

| unit | what | files | commit |
| --- | --- | --- | --- |
| U0 | preflight; the branch | — | none |
| U1 | before a picture, as a predicate; the stem | `gallery.hpp` | 1 |
| U2 | the machine: the wall's CopySrc, the staging and its estate row, the census arm, the seam, copy at R11, map at R1, the face watcher | `state.hpp`, `binding_schema.py`, `binding_gen.py`, `cartridge.hpp`, `core/postcard_face.hpp` (new) | 1 |
| U3 | one door, two mouths | `spine_state.hpp`, `input.hpp`, `organ_registry.hpp`, `organ_boundary.inc` | 1 |
| U4 | the shell: the one hand-off, the badge, the rows, the copy map | `web/index.html`, `docs/COPY.md` | 1 |
| U5 | ledgers; OPEN.md; push | `audit/*`, `docs/OPEN.md` | 1 |

The order is the compile order: U2's `request_postcard` calls U1's predicate; U3's boundary
calls U2's `request_postcard`; U4 talks to U2's seam and U3's export. At no commit does the
tree fail G-LAW 1 or the TU gate.

---

## U0 — PREFLIGHT (no commit)

```
git rev-parse --is-shallow-repository        # true → git fetch --unshallow origin
git fetch origin master                      # P9
git checkout -b claude/postcard-0 origin/master
git rev-parse HEAD                           # e35b8d5d… expected; a newer tip is DEFAULT-AND-FLAG if every blob below still matches
git ls-tree HEAD src/cartridges/the_board/bodies/gallery.hpp src/cartridges/the_board/realization/state.hpp src/cartridges/the_board/cartridge.hpp src/cartridges/the_board/contracts/spine_state.hpp src/cartridges/the_board/direction/input.hpp src/cartridges/the_board/organ_boundary.inc src/console/organ_registry.hpp tools/binding_schema.py tools/binding_gen.py web/index.html docs/COPY.md docs/OPEN.md
test -e src/core/postcard_face.hpp && echo "EXISTS — HALT U2.0 (stale authority)" || echo "absent, as expected"
sh tools/gates/glaw1/run.sh && python3 tools/gates/console_gate/run.py     # GREEN / PASS before anything moves
```
Report HEAD and the blobs. Every FIND below is a verbatim block; count it before editing
(`grep -cF` on a distinctive line of it is the quick form; the block is the anchor).

---

## U1 — BEFORE A PICTURE, AS A PREDICATE (`gallery.hpp`)

### U1.1 — std::snprintf's header (`src/cartridges/the_board/bodies/gallery.hpp`)

FIND
```
#include <chrono>      // AUBADE U1 — the stb/scale decode accumulator   // (impl, merged)
```
REPLACE
```
#include <chrono>      // AUBADE U1 — the stb/scale decode accumulator   // (impl, merged)
#include <cstdio>      // POSTCARD_0 — std::snprintf (postcard_stem)   // (impl, merged)
```

### U1.2 — the predicate and the stem, after authored_extract_number (`src/cartridges/the_board/bodies/gallery.hpp`)

FIND
```
// ── THE SCALE / PAD / UPLOAD — one body, both twins ──
```
REPLACE
```
// ═══ POSTCARD_0 — BEFORE A PICTURE, AS A PREDICATE ═══════════════════════
//
// The tide's gallery_unwatched (above) is where the world asks whether it
// is WATCHED; this is where it asks WHICH picture the visitor stands before.
// Pure, side-effect free, CPU-only: it reads the slot array the CPU already
// authors and the pose the spine already harvests, and the GPU learns no
// word. Two questions, in order:
//
//   THE ZONE (L13 — a boundary is a zone, not a line). The picture's own
//   footprint pushed forward: in front of its plane (forward is the side a
//   viewer stands on — begin_visit's standing point is position + forward
//   x stand-off, and the pilot walks there), no deeper than TAKE_REACH_*
//   (the pilot's stand-off is 1.4 x the larger side, min 4 wu, so 2.2 x
//   and min 8 wu holds a visitor who arrived by pilot AND one who wandered
//   up close), and no further sideways than the frame plus TAKE_SLACK_MULT
//   of its width each side. Both gallery forms fall out of the one struct:
//   a TERRAIN_QUAD's forward is its facing, a WALL_FRAME's is the wall's
//   normal, and neither is asked which it is.
//
//   THE EYE. Among the pictures whose zone the pawn stands in — a wall
//   holds several side by side — the one nearest the camera's line of
//   sight wins, and it must sit inside TAKE_CONE_COS of that line at all:
//   standing before a picture and looking away is not being before it.
//   The view direction is -(cos_el*sin_az, sin_el, cos_el*cos_az), the
//   tide's formula in full (it keeps only the ground projection), verified
//   there against world.wgsl's build_view_projection_matrix. A pose that
//   has not landed (valid == false) picks nothing: a badge cannot teach
//   what the eye cannot yet confirm.
//
// Returns the slot index, or Dim::PAINTING_MAX_SLOTS for "no picture". The
// caller answers for the host: the point must be the pawn. The four numbers
// are control-panel material, enrolled nowhere yet (the pilot's rule): no
// measurement has asked.
inline constexpr float TAKE_REACH_MULT   = 2.2f;     // x the picture's larger side: how deep the zone runs
inline constexpr float TAKE_REACH_MIN_WU = 8.0f;     // and never shallower: a small picture keeps a doorstep
inline constexpr float TAKE_SLACK_MULT   = 0.5f;     // x the picture's width, each side, beyond the frame
inline constexpr float TAKE_CONE_COS     = 0.7071f;  // cos 45 deg: the eye must hold the picture inside this

inline uint32_t pick_picture_before(const GalleryState& gs, float px, float pz, const CameraPose& cam) {
    if (!cam.valid) return Dim::PAINTING_MAX_SLOTS;
    const float ce = std::cos(cam.elevation);
    const float ex = -ce * std::sin(cam.azimuth);
    const float ey = -std::sin(cam.elevation);
    const float ez = -ce * std::cos(cam.azimuth);
    uint32_t best = Dim::PAINTING_MAX_SLOTS;
    float best_cos = TAKE_CONE_COS;
    for (uint32_t i = 0; i < gs.slot_high_water && i < Dim::PAINTING_MAX_SLOTS; i++) {
        const GPUPaintingSlot& s = gs.painting_slots[i];
        if (s.is_active == 0u) continue;
        // The zone.
        const float fl = std::sqrt(s.forward[0] * s.forward[0] + s.forward[2] * s.forward[2]);
        if (fl < 1e-4f) continue;   // a picture facing straight up or down has no "before"
        const float nx = s.forward[0] / fl, nz = s.forward[2] / fl;
        const float dx = px - s.position[0], dz = pz - s.position[2];
        const float along = dx * nx + dz * nz;
        const float span  = std::max(s.scale_x, s.scale_y);
        const float reach = std::max(TAKE_REACH_MIN_WU, span * TAKE_REACH_MULT);
        if (along < 0.0f || along > reach) continue;
        const float lateral = std::fabs(dx * nz - dz * nx);
        if (lateral > s.scale_x * (0.5f + TAKE_SLACK_MULT)) continue;
        // The eye.
        const float vx = s.position[0] - cam.eye[0];
        const float vy = s.position[1] - cam.eye[1];
        const float vz = s.position[2] - cam.eye[2];
        const float vl = std::sqrt(vx * vx + vy * vy + vz * vz);
        if (vl < 1e-3f) continue;
        const float c = (vx * ex + vy * ey + vz * ez) / vl;
        if (c > best_cos) { best_cos = c; best = i; }
    }
    return best;
}

// POSTCARD_0 — THE STEM OF THE POSTCARD'S NAME, from what the wall knows: a
// painting by its number (exhibition_name -> the manifest -> the number
// authored_extract_number reads), a photograph by its tier (slot_provenance,
// read beside is_active as its charter says). ASCII, lower case, '_' for a
// space, nothing else survives — a filename stem, not copy; the shell wraps
// it (T7_POSTCARD.deliver). `cap` includes the NUL.
inline void postcard_stem(const GalleryState& gs, uint32_t slot, char* out, size_t cap) {
    if (cap == 0) return;
    out[0] = '\0';
    if (slot >= Dim::PAINTING_MAX_SLOTS) return;
    const GPUPaintingSlot& s = gs.painting_slots[slot];
    if (s.content_source == ContentSource::AUTHORED) {
        int number = 0;
        if (s.texture_layer < Dim::EXHIBITION_LAYERS) {
            const uint32_t di = gs.exhibition_name[s.texture_layer].disk_index;
            if (di < gs.authored_disk_manifest.size())
                number = authored_extract_number(gs.authored_disk_manifest[di]);
        }
        std::snprintf(out, cap, "painting_%d", number);
        return;
    }
    const uint32_t tier = gs.slot_provenance[slot].shot_type;
    const char* name = (tier < (uint32_t)ShotType::COUNT) ? SHOT_TYPE_NAMES[tier] : "";
    char raw[64];
    std::snprintf(raw, sizeof raw, "photograph_%s", name);
    size_t j = 0;
    for (size_t i = 0; raw[i] != '\0' && j + 1 < cap; i++) {
        unsigned char ch = (unsigned char)raw[i];
        if (ch >= 'A' && ch <= 'Z') ch = (unsigned char)(ch - 'A' + 'a');
        if (ch == ' ') ch = '_';
        if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_') out[j++] = (char)ch;
    }
    out[j] = '\0';
}

// ── THE SCALE / PAD / UPLOAD — one body, both twins ──
```


### WITNESS — U1
```
grep -c "inline uint32_t pick_picture_before" src/cartridges/the_board/bodies/gallery.hpp   # 1
grep -c "inline void postcard_stem"           src/cartridges/the_board/bodies/gallery.hpp   # 1
grep -c "TAKE_CONE_COS"                       src/cartridges/the_board/bodies/gallery.hpp   # 3 (the banner, the constant, its use)
grep -c "#include <cstdio>"                   src/cartridges/the_board/bodies/gallery.hpp   # 1
sh tools/gates/glaw1/run.sh                        # GREEN
python3 tools/gates/console_gate/run.py            # PASS, zero diagnostics
```
Commit: `POSTCARD_0 U1 — before a picture, as a predicate: pick_picture_before and postcard_stem (gallery.hpp)`

---

## U2 — THE POSTCARD MACHINE

### U2.0 — the seam, a new file (`src/core/postcard_face.hpp`), verbatim, LF, no BOM

```

#pragma once
// ═══ THE POSTCARD FACE — A PICTURE LEAVES THE WALL (POSTCARD_0) ═════════
//
// Two calls across the one seam, the ride face's grammar (ride_face.hpp):
// the shell owns the look, an absent global is a silent no-op, and it ships
// in every build so the teacher exists in the gallery and not only in the
// lab.
//
// postcard_face(mode) — ONE call per EDGE, never per frame: 0 hides the
// badge, 1 says a painting is before you, 2 says a photograph is.
//
// postcard_deliver(...) — the picture itself, once per take. `bytes` is the
// mapped readback: RES x RES texels, four bytes each, rows packed
// (bytesPerRow == RES * 4, 256-aligned by construction). It is read HERE,
// synchronously, into a JS array, because the range dies at Unmap. The
// shell receives RGBA with alpha forced opaque: the texel order (BGRA on
// Chrome's preferred surface format) is the engine's fact and stops at
// this seam, and a photograph's sky may carry a clear alpha the JPEG
// would otherwise paint black. `name` is a filename stem, ASCII, read
// from the heap by bytes: no runtime string helper is exported
// (console.hpp's rule — ccall/cwrap only), so nothing here calls one.
//
// EM_ASM BODIES: double quotes only, and no comma outside parentheses —
// the preprocessor splits the macro on it (console.hpp's own lesson).

#include <emscripten.h>
#include <cstdint>

namespace t7 {

    inline void postcard_face(uint32_t mode) {
        EM_ASM({
            if (typeof window !== "undefined" && window.T7_POSTCARD)
                window.T7_POSTCARD.set($0);
        }, mode);
    }

    inline void postcard_deliver(const void* bytes, uint32_t res, uint32_t bgra,
                                 uint32_t crop_w, uint32_t crop_h, double aspect,
                                 uint32_t kind, const char* name) {
        EM_ASM({
            if (typeof window === "undefined" || !window.T7_POSTCARD) return;
            var p = $0;
            var n = $1 * $1 * 4;
            var src = HEAPU8.subarray(p, p + n);
            var rgba = new Uint8ClampedArray(n);
            if ($2) {
                for (var i = 0; i < n; i += 4) {
                    rgba[i] = src[i + 2]; rgba[i + 1] = src[i + 1]; rgba[i + 2] = src[i]; rgba[i + 3] = 255;
                }
            } else {
                for (var j = 0; j < n; j += 4) {
                    rgba[j] = src[j]; rgba[j + 1] = src[j + 1]; rgba[j + 2] = src[j + 2]; rgba[j + 3] = 255;
                }
            }
            var s = "";
            var q = $7;
            while (HEAPU8[q]) { s += String.fromCharCode(HEAPU8[q] & 0x7f); q++; }
            window.T7_POSTCARD.deliver(rgba, $1, $3, $4, $5, $6, s);
        }, bytes, res, bgra, crop_w, crop_h, aspect, kind, name);
    }

} // namespace t7
```

### U2.1 — the staging member, beside its three siblings (`src/cartridges/the_board/realization/state.hpp`)

FIND
```
            wgpu::Buffer cameraReadbackStaging_;   // ATRIUM_11 — always created; the pose is a spine fact (PLUMB_0 C1)
```
REPLACE
```
            wgpu::Buffer cameraReadbackStaging_;   // ATRIUM_11 — always created; the pose is a spine fact (PLUMB_0 C1)
            wgpu::Buffer postcardReadbackStaging_; // POSTCARD_0 — born on the first take (postcard_readback_staging); null until then
```

### U2.2 — the wall gains CopySrc (`src/cartridges/the_board/realization/state.hpp`)

FIND
```
                exhibitionTexture_ = makeTextureArray("Exhibition",
                    Dim::EXHIBITION_LAYERS,
                    wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding);
```
REPLACE
```
                // POSTCARD_0 — CopySrc: the wall's layer is what the postcard
                // copies out (cartridge.hpp, phase_witness_capture). The
                // photographer's staging has carried the flag from its first
                // day; the wall never needed it until a picture had to leave.
                exhibitionTexture_ = makeTextureArray("Exhibition",
                    Dim::EXHIBITION_LAYERS,
                    wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::TextureBinding);
```

### U2.3 — the staging's door, the sizes, and the one texel-order question (`src/cartridges/the_board/realization/state.hpp`)

FIND
```
            wgpu::Buffer camera_readback_staging() const { return cameraReadbackStaging_; }           // ATRIUM_11
```
REPLACE
```
            wgpu::Buffer camera_readback_staging() const { return cameraReadbackStaging_; }           // ATRIUM_11
            // POSTCARD_0 — THE POSTCARD'S STAGING, born on the first take and
            // kept for the session: a visitor who never takes a picture never
            // pays the megabyte, and the estate report at boot never names a
            // row it did not spend. One layer of the exhibition, rows packed:
            // RES * 4 = 2048 bytes a row, a multiple of 256 by construction
            // (WebGPU's bytesPerRow law), RES rows.
            static constexpr uint32_t postcard_bytes_per_row() { return Dim::PAINTING_RESOLUTION * 4u; }
            static constexpr size_t   postcard_readback_size() { return (size_t)postcard_bytes_per_row() * Dim::PAINTING_RESOLUTION; }
            static_assert((Dim::PAINTING_RESOLUTION * 4u) % 256u == 0u,
                "POSTCARD_0: bytesPerRow must be a multiple of 256 (WebGPU); RES * 4 is, at 512");
            wgpu::Buffer postcard_readback_staging() const { return postcardReadbackStaging_; }   // null until ensure_postcard_readback_staging (the estate census reads this one-line shape)
            void ensure_postcard_readback_staging() {
                if (postcardReadbackStaging_) return;
                postcardReadbackStaging_ = makeBuffer("Postcard Readback Staging",
                    postcard_readback_size(),
                    wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead);
            }
            // POSTCARD_0 — the texel order of the painting arrays, as ONE
            // question: the two upload sites asked it inline, the readback's
            // delivery asks it too, and one home answers all three.
            bool exhibition_is_bgra() const { return colorFormat_ == wgpu::TextureFormat::BGRA8Unorm; }
```

### U2.4 — the two upload sites ask the one home (expected count 2) (`src/cartridges/the_board/realization/state.hpp`)

Expected count **2** — replace every match.

FIND
```
                bool need_swap = (colorFormat_ == wgpu::TextureFormat::BGRA8Unorm);
```
REPLACE
```
                bool need_swap = exhibition_is_bgra();   // POSTCARD_0 — one home for the texel order
```

### U2.5 — the estate row (the census parses label, size and usage from makeBuffer) (`tools/binding_schema.py`)

FIND
```
    'cameraReadbackStaging_': {'kind': 'buffer', 'label': 'Camera State Readback Staging', 'size_expr': 'sizeof(GPUCameraState)', 'usage': 'wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead', 'file': 'src/cartridges/the_board/realization/state.hpp'},
```
REPLACE
```
    'cameraReadbackStaging_': {'kind': 'buffer', 'label': 'Camera State Readback Staging', 'size_expr': 'sizeof(GPUCameraState)', 'usage': 'wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead', 'file': 'src/cartridges/the_board/realization/state.hpp'},
    'postcardReadbackStaging_': {'kind': 'buffer', 'label': 'Postcard Readback Staging', 'size_expr': 'postcard_readback_size()', 'usage': 'wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead', 'file': 'src/cartridges/the_board/realization/state.hpp'},
```

### U2.6 — the reach census learns a copy's .buffer target (its first texture->buffer copy) (`tools/binding_gen.py`)

FIND
```
        # dest.texture = X assignments feed WriteTexture/Copy* structs
        for m in re.finditer(r"\.texture\s*=\s*([^;]+);", text):
            for name in re.findall(r"(\w+)\s*\(\)", m.group(1)):
                touch(name, "copy/write target")
            for name in re.findall(r"(?<![\w.])(\w+_)\b", m.group(1)):
                touch(name, "copy/write target")
```
REPLACE
```
        # dest.texture = X assignments feed WriteTexture/Copy* structs
        for m in re.finditer(r"\.texture\s*=\s*([^;]+);", text):
            for name in re.findall(r"(\w+)\s*\(\)", m.group(1)):
                touch(name, "copy/write target")
            for name in re.findall(r"(?<![\w.])(\w+_)\b", m.group(1)):
                touch(name, "copy/write target")
        # POSTCARD_0 — and dst.buffer = X, the TexelCopyBufferInfo half of a
        # CopyTextureToBuffer: the tree's first texture->buffer copy names
        # its target through a struct field, exactly as .texture does above.
        # Perturbation shown at the rehearsal: without this arm R-2 flags
        # postcardReadbackStaging_ as an orphan; with it the row is reached.
        for m in re.finditer(r"\.buffer\s*=\s*([^;]+);", text):
            for name in re.findall(r"(\w+)\s*\(\)", m.group(1)):
                touch(name, "copy/write target")
            for name in re.findall(r"(?<![\w.])(\w+_)\b", m.group(1)):
                touch(name, "copy/write target")
```

### U2.7 — the include (`src/cartridges/the_board/cartridge.hpp`)

FIND
```
#include "core/ride_face.hpp"                                      // REACH_1 V2 — the face watcher's one call per edge
```
REPLACE
```
#include "core/ride_face.hpp"                                      // REACH_1 V2 — the face watcher's one call per edge
#include "core/postcard_face.hpp"                                  // POSTCARD_0 — the take badge's edge call and the picture's one delivery
```

### U2.8 — the machine's members and the door's answer (`src/cartridges/the_board/cartridge.hpp`)

FIND
```
            // REACH_1 V2 — the face the shell currently wears (0 hidden,
            // 1 board, 2 land). Differenced so only EDGES cross to JS.
            uint32_t rideFaceShown_ = 0;
```
REPLACE
```
            // REACH_1 V2 — the face the shell currently wears (0 hidden,
            // 1 board, 2 land). Differenced so only EDGES cross to JS.
            uint32_t rideFaceShown_ = 0;

            // ═══ POSTCARD_0 — THE POSTCARD MACHINE ═══════════════════════
            //
            // The fifth readback, and the first of a TEXTURE: one layer of
            // the exhibition array leaves the GPU so the visitor can keep
            // the picture they stand before, or send it. The grammar is the
            // pawn's, the floaters' and the camera's — IDLE arms a copy
            // (R11, phase_witness_capture), COPIED issues a map (R1,
            // phase_witness_harvest), and the callback restores IDLE on its
            // way out, so at most one is ever in flight — with two
            // departures, both deliberate:
            //
            //   ON DEMAND, NEVER PER FRAME. `postcardArmed_` is set by
            //   request_postcard (the take door's answer) and spent by R11
            //   in the same frame; an unarmed frame encodes nothing.
            //
            //   NO GENERATION GUARD. The three state readbacks drop a
            //   callback from a world that ended because their bytes would
            //   otherwise become the NEW world's state. A postcard's bytes
            //   are a moment, not a state: a picture pressed in a world
            //   that ended is still the picture pressed, and its facts
            //   (below) were taken at the same press. What teardown DOES
            //   clear is an arming not yet spent — the wall it named is
            //   being emptied under it.
            //
            // WHAT RIDES BESIDE THE BYTES is decided at the press, from the
            // slot the eye picked: the crop (uv_scale — a painting is padded
            // into its square at the origin), the hung aspect (scale_x /
            // scale_y — a photograph is STORED square and hung at its lens's
            // aspect, so the shell resamples it), the kind, and a filename
            // stem. The shell receives RGBA, opaque; the texel order stops
            // at the seam (core/postcard_face.hpp).
            enum class PostcardState { IDLE, COPIED, MAPPING };
            PostcardState postcardState_ = PostcardState::IDLE;
            bool     postcardArmed_ = false;      // a take was pressed; R11 spends it
            uint32_t postcardLayer_ = 0;          // the exhibition layer the press named
            struct PostcardFacts {
                uint32_t crop_w = 0, crop_h = 0;  // the picture's texels inside the square, from the origin
                double   aspect = 1.0;            // width / height as hung
                uint32_t kind   = 0;              // ContentSource::AUTHORED / SNAPSHOT
                char     stem[64] = {};           // postcard_stem's answer
            } postcardFacts_{};
            // The face the shell currently wears for the take (0 hidden,
            // 1 a painting is before you, 2 a photograph). Differenced so
            // only EDGES cross to JS — rideFaceShown_'s grammar.
            uint32_t takeFaceShown_ = 0;

            // THE TAKE DOOR'S ANSWER, called from the frame boundary
            // (organ_boundary.inc) with both mouths folded. Every refusal
            // is a line (P6): the visitor pressed and nothing left, and the
            // console must say why.
            void request_postcard() {
                if (point_.host != PointHost::PAWN) {
                    std::cout << "[Postcard] refused: the point is not the pawn\n";
                    return;
                }
                const uint32_t slot = pick_picture_before(gallery_state_, point_.x, point_.z, camera_pose_);
                if (slot >= Dim::PAINTING_MAX_SLOTS) {
                    std::cout << "[Postcard] refused: no picture before you\n";
                    return;
                }
                if (postcardArmed_ || postcardState_ != PostcardState::IDLE) {
                    std::cout << "[Postcard] refused: one is still on its way\n";
                    return;
                }
                const GPUPaintingSlot& s = gallery_state_.painting_slots[slot];
                if (s.texture_layer >= Dim::EXHIBITION_LAYERS) {
                    std::cout << "[Postcard] refused: slot " << slot << " names layer "
                              << s.texture_layer << ", off the wall\n";
                    return;
                }
                PostcardFacts& f = postcardFacts_;
                const float res = (float)Dim::PAINTING_RESOLUTION;
                f.crop_w = (uint32_t)std::min(res, std::max(1.0f, std::round(res * s.uv_scale_x)));
                f.crop_h = (uint32_t)std::min(res, std::max(1.0f, std::round(res * s.uv_scale_y)));
                f.aspect = (s.scale_y > 0.0f) ? (double)s.scale_x / (double)s.scale_y : 1.0;
                f.kind   = s.content_source;
                postcard_stem(gallery_state_, slot, f.stem, sizeof f.stem);
                postcardLayer_ = s.texture_layer;
                postcardArmed_ = true;
                std::cout << "[Postcard] slot " << slot << " layer " << postcardLayer_
                          << " " << f.stem << " crop " << f.crop_w << "x" << f.crop_h
                          << " aspect " << f.aspect << " — copying\n";
            }
```

### U2.9 — teardown clears an arming not yet spent (`src/cartridges/the_board/cartridge.hpp`)

FIND
```
                        if (cameraReadbackState_ == CameraReadbackState::COPIED)
                            cameraReadbackState_ = CameraReadbackState::IDLE;
```
REPLACE
```
                        if (cameraReadbackState_ == CameraReadbackState::COPIED)
                            cameraReadbackState_ = CameraReadbackState::IDLE;
                        // POSTCARD_0 — an arming not yet spent names a wall this
                        // teardown empties; a COPIED or MAPPING postcard is left
                        // alone (its bytes are a moment, the banner says why).
                        if (postcardArmed_) {
                            postcardArmed_ = false;
                            std::cout << "[Postcard] dropped: the world ended before the copy\n";
                        }
```

### U2.10 — the copy (R11, phase_witness_capture) (`src/cartridges/the_board/cartridge.hpp`)

FIND
```
                if (cameraReadbackState_ == CameraReadbackState::IDLE) {
                    encoder.CopyBufferToBuffer(
                        gpuState_.camera_buffer(), 0,
                        gpuState_.camera_readback_staging(), 0,
                        GPUState::camera_state_buffer_size());
                    cameraReadbackState_ = CameraReadbackState::COPIED;
                }
            }
```
REPLACE
```
                if (cameraReadbackState_ == CameraReadbackState::IDLE) {
                    encoder.CopyBufferToBuffer(
                        gpuState_.camera_buffer(), 0,
                        gpuState_.camera_readback_staging(), 0,
                        GPUState::camera_state_buffer_size());
                    cameraReadbackState_ = CameraReadbackState::COPIED;
                }

                // POSTCARD_0 — THE PICTURE LEAVES THE WALL. On demand, not per
                // frame: request_postcard armed it this frame, from the slot the
                // eye picked; the whole layer is copied (RES x RES, rows packed
                // at 2048 B — the 256-byte law by construction) and the shell
                // crops. Same encoder, after the same dispatches: the wall this
                // frame draws is the wall this copies. The buffer is born here on
                // the first take (GPUState::postcard_readback_staging).
                if (postcardArmed_) {
                    postcardArmed_ = false;
                    if (postcardState_ == PostcardState::IDLE) {
                        gpuState_.ensure_postcard_readback_staging();
                        wgpu::TexelCopyTextureInfo src{};
                        src.texture  = gpuState_.exhibition_texture();
                        src.mipLevel = 0;
                        src.origin   = { 0, 0, postcardLayer_ };
                        src.aspect   = wgpu::TextureAspect::All;
                        wgpu::TexelCopyBufferInfo dst{};
                        dst.layout.offset       = 0;
                        dst.layout.bytesPerRow  = GPUState::postcard_bytes_per_row();
                        dst.layout.rowsPerImage = Dim::PAINTING_RESOLUTION;
                        dst.buffer = gpuState_.postcard_readback_staging();
                        wgpu::Extent3D extent = { Dim::PAINTING_RESOLUTION, Dim::PAINTING_RESOLUTION, 1 };
                        encoder.CopyTextureToBuffer(&src, &dst, &extent);
                        postcardState_ = PostcardState::COPIED;
                    } else {
                        // request_postcard guards this; a line if it ever slips.
                        std::cout << "[Postcard] dropped: the machine was busy at the copy\n";
                    }
                }
            }
```

### U2.11 — the map (R1, phase_witness_harvest), before the ride's watcher (`src/cartridges/the_board/cartridge.hpp`)

FIND
```
                // REACH_1 V2 — THE FACE WATCHER. The formula reads what is
```
REPLACE
```
                // POSTCARD_0 — THE POSTCARD'S MAP, the pawn's grammar,
                // captureless (the typed-userdata overload, `this` in the
                // trailing slot). The callback delivers to the shell THROUGH
                // the mapped range, synchronously, and unmaps on its way out.
                // No generation guard, by the machine's banner.
                if (postcardState_ == PostcardState::COPIED) {
                    postcardState_ = PostcardState::MAPPING;
                    gpuState_.postcard_readback_staging().MapAsync(
                        wgpu::MapMode::Read, 0, GPUState::postcard_readback_size(),
                        wgpu::CallbackMode::AllowSpontaneous,
                        [](wgpu::MapAsyncStatus status, wgpu::StringView, Cartridge* self) {
                            if (status == wgpu::MapAsyncStatus::Success) {
                                const void* data = self->gpuState_.postcard_readback_staging().GetConstMappedRange(
                                    0, GPUState::postcard_readback_size());
                                if (data) {
                                    const PostcardFacts& f = self->postcardFacts_;
                                    t7::postcard_deliver(data, Dim::PAINTING_RESOLUTION,
                                                         self->gpuState_.exhibition_is_bgra() ? 1u : 0u,
                                                         f.crop_w, f.crop_h, f.aspect, f.kind, f.stem);
                                    std::cout << "[Postcard] delivered: " << f.stem << "\n";
                                } else {
                                    std::cout << "[Postcard] dropped: the map gave no range\n";
                                }
                                self->gpuState_.postcard_readback_staging().Unmap();
                            } else {
                                std::cout << "[Postcard] dropped: the map failed\n";
                            }
                            self->postcardState_ = PostcardState::IDLE;
                        },
                        this);
                }

                // REACH_1 V2 — THE FACE WATCHER. The formula reads what is
```

### U2.12 — the take's face watcher, after the ride's (`src/cartridges/the_board/cartridge.hpp`)

FIND
```
                if (face != rideFaceShown_) {
                    rideFaceShown_ = face;
                    t7::ride_face(face);
                }
            }
```
REPLACE
```
                if (face != rideFaceShown_) {
                    rideFaceShown_ = face;
                    t7::ride_face(face);
                }

                // POSTCARD_0 — THE TAKE FACE WATCHER, the ride's grammar: the
                // predicate reads what is already home (the wall the CPU
                // authors, the pose R1 just harvested, the point), and the
                // difference gate means a visitor who stands still costs one
                // EM_ASM per edge and none per frame. Which picture is not
                // remembered here: the door re-asks at the press, so the badge
                // and the take can never disagree by more than a frame.
                uint32_t take = 0u;
                if (point_.host == PointHost::PAWN) {
                    const uint32_t slot = pick_picture_before(gallery_state_, point_.x, point_.z, camera_pose_);
                    if (slot < Dim::PAINTING_MAX_SLOTS)
                        take = (gallery_state_.painting_slots[slot].content_source == ContentSource::SNAPSHOT) ? 2u : 1u;
                }
                if (take != takeFaceShown_) {
                    takeFaceShown_ = take;
                    t7::postcard_face(take);
                }
            }
```


### WITNESS — U2
```
grep -c 'wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::TextureBinding' src/cartridges/the_board/realization/state.hpp   # 1
grep -c "exhibition_is_bgra()"                   src/cartridges/the_board/realization/state.hpp   # 3 (the home and its two callers)
grep -c "postcardReadbackStaging_"               src/cartridges/the_board/realization/state.hpp   # 4
grep -c 'makeBuffer("Postcard Readback Staging"' src/cartridges/the_board/realization/state.hpp   # 1
grep -c "postcardReadbackStaging_"               tools/binding_schema.py                          # 1
grep -c "POSTCARD_0 — and dst.buffer = X"        tools/binding_gen.py                             # 1
grep -c "CopyTextureToBuffer(&src, &dst, &extent)" src/cartridges/the_board/cartridge.hpp         # 1
grep -c "t7::postcard_deliver("                  src/cartridges/the_board/cartridge.hpp           # 1
grep -c "t7::postcard_face(take)"                src/cartridges/the_board/cartridge.hpp           # 1
grep -c "void request_postcard()"                src/cartridges/the_board/cartridge.hpp           # 1
grep -c "\[Postcard\]"                           src/cartridges/the_board/cartridge.hpp           # 10 — every arm has its line (P6)
python3 tools/binding_gen.py --write             # writes MANIFEST.md, limits_floor.gen.inc, features_wallet.gen.inc — BYTE-IDENTICAL:
git status --short audit src/console             # …shows none of the three (flag it if one appears, and include it in the commit)
python3 tools/binding_gen.py --check             # R-1 PASS, R-2 PASS "96 rows, 0 orphan(s)", every other row PASS; ONLY S-6 red (dirty / unpushed)
sh tools/gates/glaw1/run.sh                        # GREEN
python3 tools/gates/console_gate/run.py            # PASS, zero diagnostics, EM_ASM lint clean
```
Perturbation of U2.6, if you want the gate to lose once (P18: sidecar, not HEAD):
`cp tools/binding_gen.py tools/binding_gen.py.bak`, remove the five `.buffer` lines,
`--check` → `[FAIL] R-2 … 1 orphan(s) — FLAGGED: postcardReadbackStaging_`,
`mv tools/binding_gen.py.bak tools/binding_gen.py`.

Commit (the new file included): `POSTCARD_0 U2 — the postcard machine: the wall gains CopySrc, the staging buffer and its estate row, the face seam, copy at R11, map at R1, the take face watcher`

---

## U3 — ONE DOOR, TWO MOUTHS

### U3.1 — the intent (InputState) (`src/cartridges/the_board/contracts/spine_state.hpp`)

FIND
```
    bool  swap_pending = false;
};
```
REPLACE
```
    bool  swap_pending = false;
    // POSTCARD_0 — THE TAKE. Raised by the P key (request_take); spent
    // exactly once at the frame boundary's take door (organ_boundary.inc),
    // beside the badge's own mouth, and NOT cleared by clear_input_deltas.
    // Which picture is taken is a question about the world — the wall, the
    // point and the eye — which no input door may ask (swap_pending's rule,
    // above); the door answers it where all three are in hand.
    bool  take_pending = false;
};
```

### U3.2 — the held flag (KeyState) (`src/cartridges/the_board/direction/input.hpp`)

FIND
```
    bool pulse_held = false;
};
```
REPLACE
```
    bool pulse_held = false;
    // POSTCARD_0 — P's held flag, pulse_held's reason exactly: the console
    // raises KeyDown on GLFW_REPEAT, and a take is a STRIKE. Under
    // autorepeat an unguarded down would ask for a postcard every repeat
    // tick, and the machine would refuse each one in words.
    bool take_held = false;
};
```

### U3.3 — the declaration (`src/cartridges/the_board/direction/input.hpp`)

FIND
```
void request_pulse_swap(InputDeps* c);     // LEAP_1 — the hand's other word (CAPS_LOCK + the right PAIR tap): the ring and the reach for a body
```
REPLACE
```
void request_pulse_swap(InputDeps* c);     // LEAP_1 — the hand's other word (CAPS_LOCK + the right PAIR tap): the ring and the reach for a body
void request_take(InputDeps* c);           // POSTCARD_0 — the take's key mouth (P): raises the intent; the door at the boundary answers it
```

### U3.4 — the key, down edge only (`src/cartridges/the_board/direction/input.hpp`)

FIND
```
    case GLFW_KEY_CAPS_LOCK:  request_pulse_swap(c);                             break;
    }
    update_movement_intent(c);
}
```
REPLACE
```
    case GLFW_KEY_CAPS_LOCK:  request_pulse_swap(c);                             break;

    // ── The postcard (POSTCARD_0) ────────────────────────────────
    // Down edge only, pulse_held's grammar: a take is a strike.
    case GLFW_KEY_P:
        if (!c->keys_.take_held) {
            c->keys_.take_held = true;
            request_take(c);   // one door, two mouths — the badge's is the other (gallery_take)
        }
        break;
    }
    update_movement_intent(c);
}
```

### U3.5 — the release (`src/cartridges/the_board/direction/input.hpp`)

FIND
```
    case GLFW_KEY_SPACE: c->keys_.pulse_held = false; break;
```
REPLACE
```
    case GLFW_KEY_SPACE: c->keys_.pulse_held = false; break;
    case GLFW_KEY_P:     c->keys_.take_held  = false; break;   // POSTCARD_0
```

### U3.6 — the key mouth, after request_pulse_swap (`src/cartridges/the_board/direction/input.hpp`)

FIND
```
    c->inputState_.pulse_pending = true;
    c->inputState_.swap_pending  = true;
}

// RIGHT HALF, one finger, clean tap — the leap (SPACE's twin mouth).
```
REPLACE
```
    c->inputState_.pulse_pending = true;
    c->inputState_.swap_pending  = true;
}

// POSTCARD_0 — THE TAKE'S KEY MOUTH. It raises an intent and asks
// nothing: whether a picture is before the visitor is the wall's, the
// point's and the eye's business, and the door at the frame boundary
// (organ_boundary.inc, THE TAKE DOOR) reads all three, refuses in words,
// or arms the postcard machine. The badge's tap is the same door's other
// mouth (organ_registry.hpp, gallery_take), so no press is privileged.
inline void request_take(InputDeps* c) {
    c->inputState_.take_pending = true;
}

// RIGHT HALF, one finger, clean tap — the leap (SPACE's twin mouth).
```

### U3.7 — the door's bit and its consumer (`src/console/organ_registry.hpp`)

FIND
```
inline bool take_visit(uint32_t& slot) {
    if (g_visit_pending >= the_board::Dim::PAINTING_MAX_SLOTS) return false;
    slot = g_visit_pending;
    g_visit_pending = the_board::Dim::PAINTING_MAX_SLOTS;
    return true;
}
```
REPLACE
```
inline bool take_visit(uint32_t& slot) {
    if (g_visit_pending >= the_board::Dim::PAINTING_MAX_SLOTS) return false;
    slot = g_visit_pending;
    g_visit_pending = the_board::Dim::PAINTING_MAX_SLOTS;
    return true;
}

// ─── THE TAKE DOOR (POSTCARD_0) ───────────────────────────────────
// A door WITHOUT a parameter, the visit door's shape otherwise: the
// badge (web T7_POSTCARD) asks for the picture before the visitor; the
// frame boundary decides which one that is, refuses in words, or arms
// the postcard machine. One pending bit, last press wins, taken once.
// The P key is the same door's other mouth (input.hpp, request_take),
// so the boundary folds two mouths and no press is privileged.
inline bool g_take_pending = false;
inline bool take_take() {
    const bool p = g_take_pending;
    g_take_pending = false;
    return p;
}
```

### U3.8 — the C ABI (`src/console/organ_registry.hpp`)

FIND
```
EMSCRIPTEN_KEEPALIVE inline int gallery_visiting(void) {
    using namespace t7::organ;
    return (g_visit_view & 0x80000000u) ? (int)(g_visit_view & 0x7FFFFFFFu) : -1;
}
```
REPLACE
```
EMSCRIPTEN_KEEPALIVE inline int gallery_visiting(void) {
    using namespace t7::organ;
    return (g_visit_view & 0x80000000u) ? (int)(g_visit_view & 0x7FFFFFFFu) : -1;
}

// POSTCARD_0 — the badge's mouth: ask for the picture before the
// visitor. No parameter: the program knows which one, the shell does not.
EMSCRIPTEN_KEEPALIVE inline void gallery_take(void) {
    using namespace t7::organ;
    g_take_pending = true;
}
```

### U3.9 — the door, drained beside the visit door (`src/cartridges/the_board/organ_boundary.inc`)

FIND
```
                // THE VISIT WINDOW, refreshed here and nowhere else (the rule
                // window's charter): the pane reads whether a walk is on.
                t7::organ::set_visit_view(pilot_.active, pilot_.slot);
```
REPLACE
```
                // THE VISIT WINDOW, refreshed here and nowhere else (the rule
                // window's charter): the pane reads whether a walk is on.
                t7::organ::set_visit_view(pilot_.active, pilot_.slot);

                // THE TAKE DOOR (POSTCARD_0) — one door, two mouths, both spent
                // here: the P key's intent (request_take -> take_pending) and the
                // badge's bit (gallery_take -> the registry). Spent HERE because
                // "which picture" is a question about the world — the wall, the
                // point and the eye are all in hand at the boundary and in no
                // input door. Both mouths are consumed whether or not a picture
                // answers, so a press before an empty wall cannot fire later.
                {
                    const bool by_key = inputState_.take_pending;
                    inputState_.take_pending = false;
                    const bool by_glass = t7::organ::take_take();
                    if (by_key || by_glass) request_postcard();
                }
```


### WITNESS — U3
```
grep -c "take_pending"                src/cartridges/the_board/contracts/spine_state.hpp   # 1
grep -c "GLFW_KEY_P"                  src/cartridges/the_board/direction/input.hpp         # 2 (down, up)
grep -c "take_held"                   src/cartridges/the_board/direction/input.hpp         # 4
grep -c "inline void request_take"    src/cartridges/the_board/direction/input.hpp         # 1
grep -c "inline bool take_take()"     src/console/organ_registry.hpp                       # 1
grep -c "inline void gallery_take(void)" src/console/organ_registry.hpp                    # 1
grep -c "request_postcard()"          src/cartridges/the_board/organ_boundary.inc          # 1
sh tools/gates/glaw1/run.sh                        # GREEN
python3 tools/gates/console_gate/run.py            # PASS
python3 tools/gates/shell_gate/run.py              # GREEN (a new KEEPALIVE export at arity 0; the panel wraps nothing new)
```
Commit: `POSTCARD_0 U3 — one door, two mouths: P (request_take, take_pending, take_held) and the badge (gallery_take), folded at the boundary's take door`

---

## U4 — THE SHELL AND THE COPY MAP

### U4.1 — the one hand-off and the postcard face, before the AUBADE U2 banner (`web/index.html`)

FIND
```
    // ══ AUBADE U2 — THE DEVICE IS ASKED FOR AT PARSE TIME ═════════════
```
REPLACE
```
    // ══ POSTCARD_0 — THE ONE HAND-OFF: A FILE GOES TO THE VISITOR ═════
    //
    // DOORS_2's contract, now with two callers (the visitor's camera,
    // #ctlPhoto, and the postcard): the share sheet where the device has
    // one (Mail, Messages, AirDrop, Photos — the address field is the
    // visitor's own), a download where it has not. No server, no address
    // to abuse, nothing the site sends on anyone's behalf. `tag` names
    // the caller in the log line only.
    window.T7_KEEP = function (blob, name, type, tag) {
      var say = (typeof note === 'function') ? note : function (t) { console.log(t); };
      try {
        var file = window.File ? new File([blob], name, { type: type }) : null;
        if (file && navigator.share && navigator.canShare && navigator.canShare({ files: [file] })) {
          navigator.share({ files: [file], title: 'the_board' })
            .catch(function (err) { if (!err || err.name !== 'AbortError') say('[shell] ' + tag + ': ' + err); });
          return;
        }
        var url = URL.createObjectURL(blob);
        var a = document.createElement('a');
        a.href = url; a.download = name;
        document.body.appendChild(a); a.click(); a.remove();
        setTimeout(function () { URL.revokeObjectURL(url); }, 1000);
      } catch (err) { say('[shell] ' + tag + ': ' + err); }
    };

    // ══ POSTCARD_0 — THE POSTCARD FACE ═══════════════════════════════
    //
    // The take badge. Unlike T7_RIDE it IS a button: on glass the tap on
    // these words is the take (the touch grammar is full — both halves,
    // taps, pairs, pinch — and a hold would fire under a resting finger),
    // on a keyboard P is, and the click here is P's second mouth. C++
    // calls set(mode) on EDGES (core/postcard_face.hpp): 0 hides, 1 a
    // painting is before you, 2 a photograph. deliver(...) arrives once
    // per take with the picture as RGBA — the engine's texel order never
    // reaches here — and hands a JPEG to T7_KEEP.
    //
    // SITED TOP-CENTRE, deliberately not beside T7_RIDE: the ride face
    // sits low and lets every tap FALL THROUGH to the glass, because low
    // centre is where thumbs land; a tappable element there would steal
    // the look. The top is the eye's line toward the picture and no
    // thumb's resting place. Hidden, it takes no pointer events at all,
    // so an invisible badge can never eat a tap. The siting is Jean's
    // visual gate.
    //
    // THE WORDS ARE COPY (docs/COPY.md): "take this painting" / "take
    // this photograph", and " · P" on a fine pointer.
    window.T7_POSTCARD = (function () {
      var el = null, txt = null, fine = false;
      function build() {
        fine = !!(window.matchMedia && window.matchMedia('(pointer: fine)').matches);
        el = document.createElement('button');
        el.type = 'button';
        el.id = 't7postcard';
        el.setAttribute('aria-label', 'take this picture');
        el.style.cssText =
          'position:fixed;left:50%;top:calc(14px + env(safe-area-inset-top));transform:translateX(-50%);' +
          'display:flex;align-items:center;gap:8px;padding:8px 14px;' +
          'background:rgba(14,13,18,0.55);border:1px solid rgba(232,228,218,0.35);border-radius:2px;' +
          'color:#e8e4da;font:inherit;font-size:13px;letter-spacing:0.14em;cursor:pointer;' +
          'z-index:35;opacity:0;pointer-events:none;transition:opacity 0.45s ease;' +
          'text-shadow:0 1px 6px rgba(0,0,0,0.6);-webkit-tap-highlight-color:transparent;';
        el.innerHTML =
          '<svg viewBox="0 0 22 22" width="18" height="18" aria-hidden="true">' +
          '<rect x="3" y="3" width="16" height="16" fill="none" stroke="#e8e4da" stroke-width="1.4"/>' +
          '<rect x="6.5" y="6.5" width="9" height="9" fill="none" stroke="#e8e4da" stroke-width="1" opacity="0.55"/>' +
          '</svg>';
        txt = document.createElement('span');
        el.appendChild(txt);
        el.addEventListener('click', function () {
          el.blur();   // a focused button answers Space with a click, and Space is the leap (DOORS_2's summary lesson)
          var M = window.Module;
          if (M && typeof M._gallery_take === 'function') M._gallery_take();
          else if (typeof note === 'function') note('[shell] postcard: gallery_take not in this build');
        });
        document.body.appendChild(el);
      }
      function show(on) {
        el.style.opacity = on ? '1' : '0';
        el.style.pointerEvents = on ? 'auto' : 'none';
      }
      return {
        set: function (mode) {
          if (!el) { if (!document.body) return; build(); }
          if (mode === 1)      { txt.textContent = 'take this painting'   + (fine ? ' \u00b7 P' : ''); show(true); }
          else if (mode === 2) { txt.textContent = 'take this photograph' + (fine ? ' \u00b7 P' : ''); show(true); }
          else                 { show(false); }
        },
        deliver: function (rgba, res, cw, ch, aspect, kind, stem) {
          var say = (typeof note === 'function') ? note : function (t) { console.log(t); };
          try {
            cw = Math.max(1, Math.min(res, cw | 0));
            ch = Math.max(1, Math.min(res, ch | 0));
            // The crop: the picture's own texels, from the square's origin.
            var crop = document.createElement('canvas');
            crop.width = cw; crop.height = ch;
            crop.getContext('2d').putImageData(new ImageData(rgba, res, res), 0, 0);
            // The shape: a painting's crop already has it; a photograph is
            // stored square and hung at its lens's aspect, so it is
            // resampled to that — the picture as it hangs.
            var ow = cw, oh = ch;
            if (kind === 1) {
              if (aspect >= 1) { ow = res; oh = Math.max(1, Math.round(res / aspect)); }
              else             { ow = Math.max(1, Math.round(res * aspect)); oh = res; }
            }
            var out = crop;
            if (ow !== cw || oh !== ch) {
              out = document.createElement('canvas');
              out.width = ow; out.height = oh;
              var g = out.getContext('2d');
              g.imageSmoothingEnabled = true;
              g.imageSmoothingQuality = 'high';
              g.drawImage(crop, 0, 0, cw, ch, 0, 0, ow, oh);
            }
            var name = 'the_board_' + (stem || 'picture') + '_'
                     + new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19) + '.jpg';
            out.toBlob(function (blob) {
              if (!blob) { say('[shell] postcard: no image'); return; }
              window.T7_KEEP(blob, name, 'image/jpeg', 'postcard');
            }, 'image/jpeg', 0.92);
          } catch (err) { say('[shell] postcard: ' + err); }
        }
      };
    })();

    // ══ AUBADE U2 — THE DEVICE IS ASKED FOR AT PARSE TIME ═════════════
```

### U4.2 — the visitor's camera calls the one hand-off (`web/index.html`)

FIND
```
      // THE VISITOR'S OWN CAMERA (DOORS_2). The world photographs itself
      // for its walls; this is the other camera — what the visitor sees,
      // handed to their own device: the share sheet where there is one
      // (Messages, Mail, AirDrop, Photos), a download where there is not.
      // No server, no readback, no address to abuse. HYPOTHESIS (P1): a
      // WebGPU canvas keeps its last presented image for toBlob; a black
      // picture is the gate row, and the fallback is registered.
      var ctlPhoto = document.getElementById('ctlPhoto');
      var canvasEl = document.getElementById('canvas');
      if (ctlPhoto && canvasEl) ctlPhoto.addEventListener('click', function () {
        try {
          canvasEl.toBlob(function (blob) {
            if (!blob) { note('[shell] photo: the canvas gave no image'); return; }
            var name = 'the_board_' + (window.T7_BUILD_ID || '') + '_'
                     + new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19) + '.png';
            var file = window.File ? new File([blob], name, { type: 'image/png' }) : null;
            if (file && navigator.share && navigator.canShare && navigator.canShare({ files: [file] })) {
              navigator.share({ files: [file], title: 'the_board' })
                .catch(function (err) { if (!err || err.name !== 'AbortError') note('[shell] photo: ' + err); });
              return;
            }
            var url = URL.createObjectURL(blob);
            var a = document.createElement('a');
            a.href = url; a.download = name;
            document.body.appendChild(a); a.click(); a.remove();
            setTimeout(function () { URL.revokeObjectURL(url); }, 1000);
          }, 'image/png');
        } catch (err) { note('[shell] photo: ' + err); }
      });
```
REPLACE
```
      // THE VISITOR'S OWN CAMERA (DOORS_2). The world photographs itself
      // for its walls; this is the other camera — what the visitor sees,
      // handed to their own device through window.T7_KEEP (POSTCARD_0
      // hoisted the hand-off there: one home for two cameras). No server,
      // no readback, no address to abuse. HYPOTHESIS (P1): a WebGPU canvas
      // keeps its last presented image for toBlob; a black picture is the
      // gate row, and the fallback is registered.
      var ctlPhoto = document.getElementById('ctlPhoto');
      var canvasEl = document.getElementById('canvas');
      if (ctlPhoto && canvasEl) ctlPhoto.addEventListener('click', function () {
        try {
          canvasEl.toBlob(function (blob) {
            if (!blob) { note('[shell] photo: the canvas gave no image'); return; }
            var name = 'the_board_' + (window.T7_BUILD_ID || '') + '_'
                     + new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19) + '.png';
            window.T7_KEEP(blob, name, 'image/png', 'photo');
          }, 'image/png');
        } catch (err) { note('[shell] photo: ' + err); }
      });
```

### U4.3 — the phone's control row (`web/index.html`)

FIND
```
        <dt>FPV</dt><dd>two fingers on the left half, landing together and lifting as one</dd>
      </dl>
```
REPLACE
```
        <dt>FPV</dt><dd>two fingers on the left half, landing together and lifting as one</dd>
        <dt>Postcard</dt><dd>before a picture, tap the words that appear — it is yours to keep or send</dd>
      </dl>
```

### U4.4 — the desktop's control row (`web/index.html`)

FIND
```
        <dt>FPV</dt><dd>Ctrl</dd>
      </dl>
```
REPLACE
```
        <dt>FPV</dt><dd>Ctrl</dd>
        <dt>Postcard</dt><dd>P, before a picture — or click the words that appear</dd>
      </dl>
```

### U4.5 — the copy map (`docs/COPY.md`)

FIND
```
| the live row — *Take a photo*, *Fullscreen*, *Sound: on/off* | `#ctlPhoto` / `#ctlFull` / `#ctlSound` |
```
REPLACE
```
| the live row — *Take a photo*, *Fullscreen*, *Sound: on/off* | `#ctlPhoto` / `#ctlFull` / `#ctlSound` |
| the postcard badge — *take this painting* / *take this photograph* (*· P* on a keyboard) | `window.T7_POSTCARD`'s `set`, in the first script. THE FACT (which picture, and when) is the tree's: `bodies/gallery.hpp` `pick_picture_before`. The *Postcard* row of each platform list is copy too |
```


### WITNESS — U4
```
grep -c "window.T7_KEEP = function"      web/index.html   # 1
grep -c "window.T7_KEEP("                web/index.html   # 2 (the camera, the postcard)
grep -c "window.T7_POSTCARD = (function" web/index.html   # 1
grep -c "<dt>Postcard</dt>"              web/index.html   # 2
grep -c "the postcard badge"             docs/COPY.md     # 1
python3 - <<'EOF'
import re, subprocess
html = open('web/index.html', encoding='utf-8').read()
for i, b in enumerate(re.findall(r'<script>(.*?)</script>', html, re.S)):
    open('/tmp/b%d.js' % i, 'w').write(b)
    r = subprocess.run(['node', '--check', '/tmp/b%d.js' % i], capture_output=True, text=True)
    print('script', i, 'OK' if r.returncode == 0 else r.stderr[:400])
EOF
# → script 0 OK / script 1 OK. (No node on the machine: DEFAULT-AND-FLAG; Jean's boot is the witness.)
```
Commit: `POSTCARD_0 U4 — the shell: T7_KEEP (the one hand-off, the camera's too), the T7_POSTCARD badge and its JPEG, the Postcard control rows, the copy map`

---

## U5 — LEDGERS, THE REGISTER, THE PUSH

1. The cascade, in order (D9; the mirror pins BINDING_LEDGER.md, the organ ledger cites
   `gallery.hpp` lines):
   ```
   python3 tools/binding_ledger.py
   python3 tools/command_census.py
   python3 tools/mirror_census.py
   python3 tools/organ_ledger.py
   python3 tools/binding_ledger.py --check && python3 tools/command_census.py --check && python3 tools/mirror_census.py --check && python3 tools/organ_ledger.py --check
   python3 tools/organ_readers.py && python3 tools/organ_gap.py --gate
   python3 tools/gates/score/run.py && python3 tools/gates/shell_gate/run.py && python3 tools/gates/glaw2/run.py && python3 tools/gates/console_gate/run.py && python3 tools/gates/sha256_gate/run.py
   python3 tools/wgsl_gate.py                  # PASS — world.wgsl untouched
   python3 tools/binding_gen.py --check        # everything PASS; S-6 red until the push
   ```
   Expected changed files: `BINDING_LEDGER.md` (the `state.hpp` / `gallery.hpp` pins and
   `state.hpp` line cites), `COMMAND_LEDGER.md` (`cartridge.hpp` / `gallery.hpp` pins, phase
   line cites), `MIRROR_LEDGER.md` (`state.hpp` / `spine_state.hpp` pins, layout line cites,
   the BINDING_LEDGER pin, the provenance stamp), `ORGAN.md` (four `gallery.hpp:55x` cites,
   +1 each). Rehearsed: 4 files, 119 insertions, 119 deletions, pins and line numbers only.
   List every changed line group in the report.

2. `docs/OPEN.md` — read whole; at the top, in the register's own form, the entry below
   (verbatim, then adjust only what the round found); and in the DOORS_2 residuals, amend
   the readback line and the email line as marked.

   Add at the top of the register:
   ```
   ## POSTCARD_0 — A PICTURE LEAVES THE WALL (on `claude/postcard-0`; Jean's gates open)

   The visitor before a picture takes it — a key, or a tap on the words the glass
   shows — and their own device keeps or sends it. DOORS_0's priced readback, built
   because Jean asked; the roll's miniatures now have their machine.

   | ruling | where it lives now |
   |---|---|
   | Before a picture is a CPU predicate: the zone, then the eye | `bodies/gallery.hpp` `pick_picture_before`, `TAKE_*` |
   | The badge is the affordance; on glass it is the door; P is the key | `web/index.html` `window.T7_POSTCARD`; `input.hpp` `request_take` |
   | One door, two mouths, folded at the boundary, refusals in words | `organ_boundary.inc` THE TAKE DOOR; `organ_registry.hpp` `gallery_take` / `take_take`; `cartridge.hpp` `request_postcard` |
   | The fifth readback: on demand, no generation guard (a moment, not a state) | `cartridge.hpp` POSTCARD MACHINE; R11 copy, R1 map |
   | The wall carries CopySrc; the staging is born on the first take | `state.hpp` `makeTextureArray("Exhibition"…)`, `ensure_postcard_readback_staging` |
   | RGBA and provenance cross once; the texel order stops at the seam | `core/postcard_face.hpp` |
   | The postcard is the picture as it hangs (uv crop; a photograph resampled to its hung aspect); JPEG 0.92 | `T7_POSTCARD.deliver` |
   | One hand-off for two cameras; the message leaves from the visitor's own mail | `window.T7_KEEP`; `#ctlPhoto` calls it |
   | The estate learns `.buffer =` as a copy target | `tools/binding_gen.py` `resource_reach`; `tools/binding_schema.py` `postcardReadbackStaging_` |

   ### Residuals — POSTCARD_0

   - **The four `TAKE_*` numbers are control-panel material**, enrolled nowhere until a
     measurement asks (the pilot's rule). The reach (2.2 × the larger side, min 8 wu)
     was chosen to contain the pilot's standing point (1.4 ×, min 4) with margin; the
     cone (45°) to hold a third-person eye that looks down at the pawn and past it.
     Jean's eye may move any of them; each is one number.
   - **The badge's siting is top-centre**, on the argument that the ride face's spot is
     where thumbs land and fall through by charter. A visual row for Jean on both
     platforms; the idiom (inline `cssText`, built on first use) is T7_RIDE's.
   - **The badge's words and the key are copy** (`docs/COPY.md`): *take this painting* /
     *take this photograph*, *· P* on a fine pointer; the two *Postcard* rows. Jean's
     naming gate — the facts are the tree's.
   - **A delivered photograph's orientation is a visual row.** The readback hands the
     texel array as stored (row 0 = top for a decoded painting and for a render
     target); if a photograph arrives inverted, the fix is one vertical flip per kind
     in `T7_POSTCARD.deliver`, not in the engine.
   - **P does nothing while the sandwich is open** — DOORS_2's guard withholds keydown
     inside the open menu; the badge's click is outside `#menu` and still works.
   - **A larger postcard for authored work is a dist decision**, not a mechanism: the
     exhibition ships at `PAINTING_CAP` 512 (the Gallery ships 640/1280 for its sets).
     The postcard is the wall's copy, second-generation JPEG. Parked at Jean's gate.
   - **The roll's miniatures** (DOORS_2's other reader of this readback) would be one
     more door with a slot parameter through the same machine and the same `deliver`;
     the roll is parked (DOORS_3), so they are not built.
   - **The camera's residual stands**: the cursor orb appears in the visitor's PHOTO
     (the frame). It cannot appear in a postcard (a texture).
   - **The estate census's `.buffer =` arm has its perturbation in the handoff**, not in
     a gate row of its own: it is a fifth reach kind of an existing PASS/FAIL row
     (R-2), and R-2 already loses on a perturbed tree.
   ```

   In the DOORS_2 residuals, replace the readback line's second sentence — *"Not built
   until a measurement asks."* — with *"BUILT at POSTCARD_0 (the texture readback, on
   demand); the roll's miniatures remain unbuilt because the roll is parked."*, keeping
   the `surfaceConfig_.usage` note as it stands (the whole-frame C++ fallback is still
   unbuilt and that line is still true). Append to the email line: *"POSTCARD_0 ruling 7:
   the share sheet is the mail path; the site sends nothing."*

   Commit: `POSTCARD_0 U5 — ledgers regenerated (state/gallery/cartridge/spine_state pins; ORGAN.md line shift); OPEN.md: POSTCARD_0 open on claude/postcard-0, residuals registered`

3. Push the branch (`git push -u origin claude/postcard-0`). Do not merge. Then
   `python3 tools/binding_gen.py --check` once more: S-6 turns green at the pushed tip.

---

## JEAN'S GATES — build, then look

```
cmake --build --preset the-board-web
python tools\dist.py
npx wrangler pages dev dist          # http://localhost:8788
```

The visual rows, each on **Chrome desktop, the Pixel, and the iPhone** (L48 — no shader
moves, but `MapAsync` on a texture readback and share-with-files on WebKit are the third
arm, so the round is not landed until the iPhone has run it):

1. **The badge appears and leaves.** Walk the pawn up to a hung painting and turn the
   camera onto it: the words appear top-centre — *take this painting · P* on a keyboard,
   *take this painting* on glass. Turn away: they fade. Before a photograph: *take this
   photograph*. Before nothing: nothing. On the ribbon: nothing.
2. **The take.** P, or a tap on the words. Console: `[Postcard] slot N layer L painting_50
   crop 341x512 aspect 0.66… — copying` then `[Postcard] delivered: painting_50`. The share
   sheet opens (phone; Windows Chrome/Edge; macOS Safari) or a JPEG downloads, named
   `the_board_painting_50_<time>.jpg`. Open it: the painting, no padding, right way up.
3. **A photograph's shape.** Take one: `the_board_photograph_panoramic_<time>.jpg` (or
   `…_birds_eye_…`, `…_closeup_…`); wide where the wall shows it wide, tall where tall,
   right way up (residual above if not).
4. **The mail path.** On the phone, choose Mail in the sheet: the picture is attached, the
   address field is empty and yours. That is ruling 7, witnessed.
5. **The visitor's camera still works** — *Take a photo* in the sandwich hands off a PNG
   through the same `T7_KEEP`.
6. **Refusals speak.** P before an empty wall: `[Postcard] refused: no picture before you`.
   P on the ribbon: `refused: the point is not the pawn`. P held down: ONE postcard
   (autorepeat guarded). P twice in the same instant: the second says `refused: one is
   still on its way`. A portal crossed with a take armed: `dropped: the world ended before
   the copy`.
7. **The keyboard after a click.** Click the words with the mouse, then press Space: the
   pawn leaps and NO second postcard is taken (the badge blurs on click).
8. **Nothing else moved.** The world does not flash on a take; the ride face is where it
   was; the tide, the roll's parked pane, the pilot are untouched.

Naming gates: the key (P), the badge's words, the file-name stems, the badge's siting.

---

## THE ROUND'S REPORT

Per unit — landed / flagged / halted / deferred; witness output verbatim; every default
taken; every count that differed and what the tree said. Cite symbols. Name the branch tip.
Then the gate tables are Jean's.
