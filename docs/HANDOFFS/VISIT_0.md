# VISIT_0 — THE ROLL AS DIRECTORY

*Handoff · authored 2026-09-06 · Claude → CC · Jean holds build, visual, merge, deploy.*
*Filed to `docs/HANDOFFS/VISIT_0.md` while open; the directory dies with the campaign.*

## THE IDEA, IN ONE PARAGRAPH

The world already photographs itself (the photographer, `bodies/gallery.hpp`) and already
hangs the pictures (the exhibition). The sandwich menu (DOORS_0) gains a pane that lists
what is hanging **as facts** — tier, distance, bearing, age, wall or ground — and choosing
one makes the pawn **walk there**, the camera swinging round to face it. No texture leaves
the GPU; the CPU only reads its own slot array. The world stays the viewer. Zero readback,
zero new render pass, zero new GPU word.

## AUTHORITY

Base: `master` at HEAD `dcbd4a900464add6d297b9351cbe31da2b40af67`.
Rides a **held branch `claude/visit-0`** (P12: a spine row, a deps face and a camera
that moves are runtime behaviour a visual gate can catch). Jean authorises the merge; the
branch dies on merge (ATTIC LAW). DOORS_0 is expected on master first; if it is not, U4 is
deferred (not halted) — see U4.

| file | blob |
| --- | --- |
| `src/cartridges/the_board/bodies/gallery.hpp` | `e77dfca0c5497c68ad3c24eb4dfa8cb9ca37c49a` |
| `src/cartridges/the_board/contracts/entity_types.hpp` | (report it at U0) |
| `src/console/organ_registry.hpp` | `baa364f2dfd115d5b0ff915003e05689e0a718d2` |
| `src/cartridges/the_board/organ_boundary.inc` | `d747a67b5931dcbdbd713c00ba47d525e98d5b53` |
| `src/cartridges/the_board/cartridge.hpp` | `54d09f8fe87aba50f9cb9f429ff5923b2b529bab` |
| `src/cartridges/the_board/direction/input.hpp` | `603ba5c11fb8935fc0eab07c560eddf110de7f2e` |
| `src/cartridges/the_board/contracts/spine_state.hpp` | `36b7df6a38e43998599c253ed5e0da8a12d57e03` (read only) |
| `tools/gates/score/run.py` | (report it at U0) |
| `web/routes.json`, `web/menu.css`, `web/index.html` | as left by DOORS_0 U6 (report them at U0) |
| `docs/OPEN.md` | `86a95324e0dd359e72c2dff58639bcd12291c8bb` |

**Pinned files.** `gallery.hpp`, `cartridge.hpp`, `spine_state.hpp` are sha256-pinned in
`audit/COMMAND_LEDGER.md` / `BINDING_LEDGER.md` / `MIRROR_LEDGER.md` (OPEN.md D9). The
ledger gates are red from U1 until U5 regenerates them in order. Expected; say so.

**HALT classes (P17), scoped to the unit (P15):** unreachable file; stale authority (blob
differs AND the FIND does not match). Everything else DEFAULT-AND-FLAG. Line numbers are
hints; boundaries are symbols (P2). Expected match count **1** unless stated. LF only.

## THE RULINGS THIS CAMPAIGN LANDS

1. **Pawn host walks.** Not a camera flight. Terrain height is GPU-only
   (`sample_terrain_y_at` has no CPU twin), so a camera pilot cannot know where the ground
   is at the destination. The pawn's snap solves it by construction, the kite frames it,
   and the walk is the feature. A visit from CAMERA or RIBBON host first presses
   `possess(PAWN)` — the R key's own transaction, its guards untouched.
2. **The pilot is a third hand on the same wheel.** It authors `move_x/move_z` under the
   fold's unit clamp and `look_az_delta` at a bounded rate. It writes no position, ships no
   trajectory, adds no GPU word. It releases on arrival, on any real hand, on a host change,
   and on a stall — each with a line (P6).
3. **Provenance is a CPU side-table.** REPEAT_0 deleted the staging→exhibition link, so a
   hung snapshot could not say which shot it shows. `SlotProvenance { shot_type, taken_at }`
   is written at the two snapshot fill sites, never cleared, read only beside `is_active`.
   Not in the slot's padding: a fact the GPU never reads is not uploaded.
4. **The roll is a window, not a home** (the orb rule window's charter). The registry borrows
   the slot array, the side-table and the tier names through `bind_roll`, and the clock
   through `bind_clock`; `gallery_roll()` renders JSON on demand, `organ_manifest`'s grammar.
5. **The visit is a door with a parameter** (the host door's grammar): `gallery_visit(slot)`
   sets one pending id, last press wins, taken once at the frame boundary.
6. **The pilot is a spine row**, `Pilot`, first in `UPDATE_SPINE`, gated `true`
   (foundational — the driver's authorship, like `FillSignal`), enrolled in the score
   census's `FOUNDATIONAL_PHASES`. It must precede `FillSignal`, which copies the channel;
   a static_assert says so.

## UNITS, IN ORDER

| unit | what | files | commit |
| --- | --- | --- | --- |
| U0 | preflight; the branch | — | none |
| U1 | provenance | `gallery.hpp`, `entity_types.hpp` | 1 |
| U2 | the registry: the window, the door, the C ABI; the binds | `organ_registry.hpp`, `cartridge.hpp` | 1 |
| U3 | the pilot: organ, deps face, tick, spine row, boundary drain, census enrolment | `input.hpp`, `cartridge.hpp`, `organ_boundary.inc`, `tools/gates/score/run.py` | 1 |
| U4 | the pane | `web/routes.json`, `web/menu.css`, `web/index.html` | 1 |
| U5 | ledgers; OPEN.md; merge readiness | `audit/*`, `docs/OPEN.md` | 1 |

---

## U0 — PREFLIGHT (no commit)

```
git rev-parse --is-shallow-repository        # true → git fetch --unshallow origin
git fetch origin master                      # P9
git checkout -b claude/visit-0 origin/master
git rev-parse HEAD
git ls-tree HEAD src/cartridges/the_board/bodies/gallery.hpp src/cartridges/the_board/contracts/entity_types.hpp src/console/organ_registry.hpp src/cartridges/the_board/organ_boundary.inc src/cartridges/the_board/cartridge.hpp src/cartridges/the_board/direction/input.hpp src/cartridges/the_board/contracts/spine_state.hpp tools/gates/score/run.py web/routes.json web/menu.css web/index.html docs/OPEN.md
grep -c "data-pane=\"collection\"" web/index.html     # 1 if DOORS_0 U6 landed; 0 → U4 is DEFERRED, everything else proceeds
grep -n "FOUNDATIONAL_PHASES = {" -A 12 tools/gates/score/run.py    # read the dict's form before U3 edits it
grep -n "possess(InputDeps\* c, PointHost next)" src/cartridges/the_board/direction/input.hpp   # the declaration and the definition: 2 lines
```
Report HEAD, the blobs, and whether `web/index.html` carries DOORS_0.

---

## U1 — PROVENANCE

### U1.1 — the DTO (`contracts/entity_types.hpp`)

By symbol: immediately BEFORE the banner line that begins `// ═══ THE MACHINE FACE`
(expected count of that prefix: 1), insert:
```
// ═══ THE ROLL'S PROVENANCE (VISIT_0) ══════════════════════════════
// A DTO that crosses the gallery→registry boundary: what a hung
// snapshot slot remembers of its shot. Defined here, not in gallery.hpp,
// because the registry (console/organ_registry.hpp) reads it and may not
// include a body. Written by the gallery's two snapshot fill sites
// (outdoor inline, indoor twin), read by gallery_roll(). Never cleared:
// it is read only beside painting_slots[i].is_active, so a stale row
// under an empty slot is unreadable by construction.
struct SlotProvenance {
    uint32_t shot_type = 0;      // ShotType index; SHOT_TYPE_NAMES names it (gallery.hpp)
    double   taken_at  = -1.0;   // TimeState::seconds at capture; negative = unknown
};

```

### U1.2 — the tier names, one home (`gallery.hpp`)

FIND
```
    CINEMATIC = 7,   // dramatic distance + wide lens distortion
    COUNT = 8
};
```
REPLACE
```
    CINEMATIC = 7,   // dramatic distance + wide lens distortion
    COUNT = 8
};

// THE TIER NAMES, ONE HOME (VISIT_0 hoisted them out of capture_snapshot's
// log line). The witness reads them, and so does the roll the sandwich
// shows (gallery_roll, through the registry's window).
inline constexpr const char* SHOT_TYPE_NAMES[] = {
    "Panoramic", "Environmental", "Medium", "Close-up",
    "Portrait", "Bird's Eye", "Low Angle", "Cinematic"
};
static_assert(sizeof(SHOT_TYPE_NAMES) / sizeof(SHOT_TYPE_NAMES[0]) == (size_t)ShotType::COUNT,
    "one name per tier — the roll indexes this table by shot_type");
```

FIND (the local table inside `capture_snapshot`)
```
    const char* shot_names[] = {
        "Panoramic", "Environmental", "Medium", "Close-up",
        "Portrait", "Bird's Eye", "Low Angle", "Cinematic"
    };
```
REPLACE with nothing.

FIND
```
            << " (" << shot_names[static_cast<uint32_t>(shot)] << ")"
```
REPLACE
```
            << " (" << SHOT_TYPE_NAMES[static_cast<uint32_t>(shot)] << ")"
```

### U1.3 — the moment of capture (`gallery.hpp`)

FIND
```
    uint32_t capture_frame = 0;
};
```
REPLACE
```
    uint32_t capture_frame = 0;
    double   capture_seconds = -1.0;   // VISIT_0 — TimeState::seconds at capture; the roll's age. Negative = never.
};
```
(Expected count 1: this is `SnapshotStagingRecord`'s tail. If it matches elsewhere, take the
one inside `struct SnapshotStagingRecord` and flag.)

FIND
```
    rec.capture_frame = gs.frame_counter;
```
REPLACE
```
    rec.capture_frame = gs.frame_counter;
    rec.capture_seconds = c->time_state_.seconds;   // VISIT_0 — the clock is const on the deps face (ATRIUM_10)
```

### U1.4 — the side-table and its two writers (`gallery.hpp`)

FIND
```
    GPUPaintingSlot painting_slots[Dim::PAINTING_MAX_SLOTS]{};
```
REPLACE
```
    GPUPaintingSlot painting_slots[Dim::PAINTING_MAX_SLOTS]{};
    // VISIT_0 — WHAT A HUNG PHOTOGRAPH REMEMBERS. REPEAT_0 deleted the
    // staging→exhibition link, so a slot could no longer say which shot
    // it shows; this side-table holds the two facts the roll needs — the
    // tier and the moment — written at the two snapshot fill sites and
    // never cleared: read ONLY beside is_active, so a stale row under an
    // empty slot is unreadable. CPU-only; world.wgsl learns no word
    // (GalleryCenter's own charter). Indexed as painting_slots is.
    SlotProvenance  slot_provenance[Dim::PAINTING_MAX_SLOTS]{};
```

FIND (the outdoor snapshot fill; `snap` is the staging record in scope, `slot` the index)
```
            gs.exhibition_occupied[exh] = true;
            gs.snapshot_staging[staging_layer].consumed = true;
            queue_promotion(gs, true, staging_layer, exh);
```
REPLACE
```
            gs.exhibition_occupied[exh] = true;
            gs.snapshot_staging[staging_layer].consumed = true;
            gs.slot_provenance[slot] = { snap.shot_type, snap.capture_seconds };   // VISIT_0
            queue_promotion(gs, true, staging_layer, exh);
```

FIND (the indoor snapshot fill; `f.record` is the staging index, `slot` the index)
```
                gs.exhibition_occupied[exh] = true;
                gs.snapshot_staging[f.record].consumed = true;
                queue_promotion(gs, true, f.record, exh);
```
REPLACE
```
                gs.exhibition_occupied[exh] = true;
                gs.snapshot_staging[f.record].consumed = true;
                gs.slot_provenance[slot] = { gs.snapshot_staging[f.record].shot_type,     // VISIT_0
                                             gs.snapshot_staging[f.record].capture_seconds };
                queue_promotion(gs, true, f.record, exh);
```
Refuter (P3, the deletion of `shot_names`): `grep -n "shot_names" src/cartridges/the_board/bodies/gallery.hpp` → no lines (P11: whole file).

### U1 witness
```
grep -c "SHOT_TYPE_NAMES"                 src/cartridges/the_board/bodies/gallery.hpp   # 3 (table, static_assert, log line)
grep -c "slot_provenance\[slot\] = "      src/cartridges/the_board/bodies/gallery.hpp   # 2
grep -c "capture_seconds"                 src/cartridges/the_board/bodies/gallery.hpp   # 4
grep -c "struct SlotProvenance"           src/cartridges/the_board/contracts/entity_types.hpp   # 1
python3 tools/gates/console_gate/run.py   # PASS — the TU type-checks
python3 tools/gates/glaw2/run.py          # GREEN — world.wgsl untouched
python3 tools/mirror_census.py --check    # RED on the gallery.hpp pin, and ONLY that — report the exact lines
```

Commit: `VISIT_0 U1 — provenance: SlotProvenance side-table written at both snapshot fills; capture_seconds; SHOT_TYPE_NAMES hoisted to one home`

---

## U2 — THE REGISTRY: THE WINDOW, THE DOOR, THE C ABI

### U2.1 — includes (`organ_registry.hpp`)

FIND
```
#include "cartridges/the_board/contracts/spine_state.hpp"   // MoodProfile + mood_def: the definition side
```
REPLACE
```
#include "cartridges/the_board/contracts/spine_state.hpp"   // MoodProfile + mood_def: the definition side
#include "cartridges/the_board/contracts/entity_types.hpp"  // VISIT_0 — SlotProvenance, the roll's side-table DTO
```
By symbol: add `#include <cmath>` to the standard-header group (the one containing
`#include <cstdio>`), in alphabetical position.

### U2.2 — the window (`organ_registry.hpp`)

FIND
```
inline void bind_point(const the_board::PointState* p) { g_point = p; }
```
REPLACE
```
inline void bind_point(const the_board::PointState* p) { g_point = p; }

// THE ROLL, BORROWED (VISIT_0). The gallery's slot array, its provenance
// side-table and the tier names, read through pointers the cartridge
// binds at boot — a window, not a home (the orb rule window's charter):
// the panel can never list a picture the world has taken down, because
// it reads the same array the GPU is fed from. The clock rides beside,
// for the age.
struct RollView {
    const the_board::GPUPaintingSlot* slots      = nullptr;
    const the_board::SlotProvenance*  prov       = nullptr;
    const char* const*                tier_names = nullptr;
    uint32_t                          tier_count = 0;
};
inline RollView g_roll;
inline const the_board::TimeState* g_clock = nullptr;
inline void bind_roll(RollView v)                        { g_roll = v; }
inline void bind_clock(const the_board::TimeState* t)    { g_clock = t; }
```

### U2.3 — the door and the visit window (`organ_registry.hpp`)

FIND
```
inline constexpr uint32_t HOST_NONE = 0xFFFFFFFFu;
inline uint32_t g_go_host_pending = HOST_NONE;
inline bool take_go_host(uint32_t& host) {
    if (g_go_host_pending == HOST_NONE) return false;
    host = g_go_host_pending;
    g_go_host_pending = HOST_NONE;
    return true;
}
```
REPLACE
```
inline constexpr uint32_t HOST_NONE = 0xFFFFFFFFu;
inline uint32_t g_go_host_pending = HOST_NONE;
inline bool take_go_host(uint32_t& host) {
    if (g_go_host_pending == HOST_NONE) return false;
    host = g_go_host_pending;
    g_go_host_pending = HOST_NONE;
    return true;
}

// ─── THE VISIT DOOR (VISIT_0) ─────────────────────────────────────
// The host door's shape: a door with a parameter. The shell names a
// painting slot; the frame boundary hands it to begin_visit, whose own
// guards (no pawn to walk, an empty slot) refuse in words. One pending
// id, last press wins, taken once; PAINTING_MAX_SLOTS is "no request"
// and no slot has that index.
inline uint32_t g_visit_pending = the_board::Dim::PAINTING_MAX_SLOTS;
inline bool take_visit(uint32_t& slot) {
    if (g_visit_pending >= the_board::Dim::PAINTING_MAX_SLOTS) return false;
    slot = g_visit_pending;
    g_visit_pending = the_board::Dim::PAINTING_MAX_SLOTS;
    return true;
}

// THE VISIT WINDOW — a copy the boundary writes once a frame (the rule
// window's charter): whether a walk is on, and to which slot. Packed:
// bit 31 active, bits 0..30 the slot.
inline uint32_t g_visit_view = 0;
inline void set_visit_view(bool active, uint32_t slot) {
    g_visit_view = (active ? 0x80000000u : 0u) | (slot & 0x7FFFFFFFu);
}
```

### U2.4 — the C ABI (`organ_registry.hpp`)

FIND
```
EMSCRIPTEN_KEEPALIVE inline const char* organ_mood_names(void) {
```
REPLACE
```
// ═══ THE ROLL (VISIT_0) ══════════════════════════════════════════════
// What the world has hung of its own photographs, as facts: for every
// active snapshot slot — its index, tier, age, distance and bearing from
// the point, size, and whether it hangs on a wall. Positions are not
// emitted: the shell has no use for coordinates it cannot show, and the
// distance/bearing pair is what a caption says. Built on demand into a
// static string, organ_manifest's grammar. Empty before bind_roll.
EMSCRIPTEN_KEEPALIVE inline const char* gallery_roll(void) {
    using namespace t7::organ;
    using namespace t7::the_board;
    static std::string json;
    json.clear();
    json.push_back('[');
    if (g_roll.slots && g_roll.prov && g_point) {
        const double now = g_clock ? g_clock->seconds : 0.0;
        char buf[256];
        bool first = true;
        for (uint32_t i = 0; i < Dim::PAINTING_MAX_SLOTS; ++i) {
            const GPUPaintingSlot& s = g_roll.slots[i];
            if (s.is_active == 0u || s.content_source != ContentSource::SNAPSHOT) continue;
            const SlotProvenance& p = g_roll.prov[i];
            const float dx = s.position[0] - g_point->x;
            const float dz = s.position[2] - g_point->z;
            const float d  = std::sqrt(dx * dx + dz * dz);
            const float bearing = std::atan2(dx, dz) * 57.2957795f;   // degrees; 0 = +Z, clockwise — the WORLD's frame
            const char* tier = (p.shot_type < g_roll.tier_count) ? g_roll.tier_names[p.shot_type] : "";
            std::snprintf(buf, sizeof buf,
                "%s{\"s\":%u,\"tier\":\"%s\",\"age\":%.0f,\"d\":%.0f,\"b\":%.0f,\"w\":%.1f,\"h\":%.1f,\"wall\":%u}",
                first ? "" : ",", (unsigned)i, tier,
                (p.taken_at < 0.0) ? -1.0 : (now - p.taken_at),
                (double)d, (double)bearing, (double)s.scale_x, (double)s.scale_y,
                (unsigned)(s.form_type == FormType::WALL_FRAME ? 1u : 0u));
            json += buf;
            first = false;
        }
    }
    json.push_back(']');
    return json.c_str();
}

// Ask the program to walk the pawn to a hung photograph. Out of range is
// ignored, for organ_door's own reason.
EMSCRIPTEN_KEEPALIVE inline void gallery_visit(uint32_t slot) {
    using namespace t7::organ;
    if (slot < t7::the_board::Dim::PAINTING_MAX_SLOTS) g_visit_pending = slot;
}

// The slot being walked to, or -1. The pane reads it to say so.
EMSCRIPTEN_KEEPALIVE inline int gallery_visiting(void) {
    using namespace t7::organ;
    return (g_visit_view & 0x80000000u) ? (int)(g_visit_view & 0x7FFFFFFFu) : -1;
}

EMSCRIPTEN_KEEPALIVE inline const char* organ_mood_names(void) {
```
`tier` is a name from a constexpr table, never visitor text, so no JSON escaping is owed;
the static_assert in U1.2 is the bound on the index.

### U2.5 — the binds (`cartridge.hpp`)

FIND
```
                t7::organ::bind_point(&point_);   // RIBBON_1 — the panel's host row
```
REPLACE
```
                t7::organ::bind_point(&point_);   // RIBBON_1 — the panel's host row
                // VISIT_0 — the roll's window and the clock it ages by. A
                // window: the panel reads the array the GPU is fed from.
                t7::organ::bind_roll(t7::organ::RollView{
                    gallery_state_.painting_slots, gallery_state_.slot_provenance,
                    SHOT_TYPE_NAMES, (uint32_t)ShotType::COUNT });
                t7::organ::bind_clock(&time_state_);
                std::cout << "[Visit] roll bound; pilot idle\n";   // P6 — the boot state, once
```
If `gallery_state_` is not yet constructed at this point in the ctor body (it is a member;
the binds run in the body, after the init list — expected fine), or `SHOT_TYPE_NAMES` is not
visible (cartridge.hpp includes the bodies before the registry — expected fine), move the
three lines to the first site after `gallery_state_`'s construction and flag.

### U2 witness
```
grep -c "EMSCRIPTEN_KEEPALIVE inline .* gallery_" src/console/organ_registry.hpp   # 3
grep -c "bind_roll\|bind_clock" src/console/organ_registry.hpp src/cartridges/the_board/cartridge.hpp   # 2 each file (decl+def / two calls)
python3 tools/gates/shell_gate/run.py     # GREEN — organ_panel.js cwraps nothing new (it proves shell→export, not the reverse)
python3 tools/gates/console_gate/run.py   # PASS
python3 tools/organ_ledger.py --check     # PASS expected (no dial enrolled); if it differs, regenerate, commit with U5, flag
```

Commit: `VISIT_0 U2 — the registry: RollView window + clock bind, the visit door, gallery_roll / gallery_visit / gallery_visiting`

---

## U3 — THE PILOT

### U3.1 — the organ and its numbers (`direction/input.hpp`)

FIND
```
struct TouchMoveState {
    float x = 0.0f;
    float z = 0.0f;
};
```
REPLACE
```
struct TouchMoveState {
    float x = 0.0f;
    float z = 0.0f;
};

// VISIT_0 — THE PILOT'S ORGAN. The driver's private state for a walk to
// a hung photograph (begin_visit / pilot_tick, below). Instance at the
// root beside touch_, like every driver organ.
struct PilotState {
    bool     active  = false;
    uint32_t slot    = UINT32_MAX;         // painting_slots index being visited
    float    tx = 0.0f, tz = 0.0f;         // the STANDING point: position + forward * stand-off
    float    aim_x = 0.0f, aim_z = 0.0f;   // the painting's centre — the orbit turns to face it
    float    best_d  = 1e9f;               // closest approach so far
    double   best_at = 0.0;                // when best_d last improved (the stall clock)
};
// The pilot's numbers. Control-panel material, enrolled nowhere yet (no
// measurement has asked). PILOT_STALL_S must outlast possess()'s landing
// ease, or a visit begun from the ribbon dies on its own doorstep.
inline constexpr float  PILOT_ARRIVE_WU       = 2.5f;   // within this of the standing point: arrived
inline constexpr float  PILOT_SLOW_WU         = 8.0f;   // inside this the stride eases to a quarter
inline constexpr float  PILOT_PROGRESS_WU     = 0.25f;  // an approach shorter than this is not progress
inline constexpr float  PILOT_TURN_RATE       = 1.8f;   // rad/s the pilot may swing the orbit
inline constexpr double PILOT_STALL_S         = 6.0;    // no progress for this long: release, say so
inline constexpr float  PILOT_STANDOFF_MULT   = 1.4f;   // × the painting's larger side
inline constexpr float  PILOT_STANDOFF_MIN_WU = 4.0f;
```

### U3.2 — the deps face (`direction/input.hpp`)

FIND
```
    CameraControls& camera_;      // the first live panel dial (KP_+/KP_-)
};
```
REPLACE
```
    CameraControls& camera_;      // the first live panel dial (KP_+/KP_-)
    const CameraPose& camera_pose_;   // VISIT_0 — the P5 mirror of the orbit (azimuth for the pilot's frame; eye for its aim). Read-only, one frame stale by law (E-4).
    PilotState&   pilot_;             // VISIT_0 — the third hand's organ
};
```
`CameraPose` lives in `contracts/spine_state.hpp`, which this file already reaches (it
names `PlayerState`, `InputState`); if it does not resolve, add the include and flag.

FIND
```
void clear_input_deltas(InputDeps* c);
```
REPLACE
```
void clear_input_deltas(InputDeps* c);
// VISIT_0 — the pilot: per-frame (the Pilot spine row) and the door's verb.
void pilot_tick(InputDeps* c, double now, double dt);
void begin_visit(InputDeps* c, uint32_t slot, float px, float pz, float fx, float fz, float span, double now);
```

### U3.3 — the tick and the verb (`direction/input.hpp`)

FIND
```
inline void clear_input_deltas(InputDeps* c) {
    c->inputState_.look_az_delta = 0.0f;
    c->inputState_.look_el_delta = 0.0f;
    c->inputState_.zoom_delta = 0.0f;
    c->inputState_.pan_x_delta = 0.0f;
    c->inputState_.pan_y_delta = 0.0f;
}
```
REPLACE
```
inline void clear_input_deltas(InputDeps* c) {
    c->inputState_.look_az_delta = 0.0f;
    c->inputState_.look_el_delta = 0.0f;
    c->inputState_.zoom_delta = 0.0f;
    c->inputState_.pan_x_delta = 0.0f;
    c->inputState_.pan_y_delta = 0.0f;
}

// ═══ THE PILOT (VISIT_0) — A THIRD HAND ON THE SAME WHEEL ═══════════
//
// A visit is the sandwich asking the body to walk to a hung photograph.
// The pilot authors exactly what the keys and the stick author —
// move_x/move_z under the fold's unit clamp, look_az_delta at a bounded
// rate — and nothing else: no position, no trajectory, no GPU word, no
// host flip beyond the R key's own transaction. The pawn kernel's
// terrain snap carries the body over the ground and the kite carries
// the camera; that is why the pilot walks the PAWN (the ruling: the CPU
// cannot sample terrain, so a camera-host flight cannot know where the
// ground is at the destination; the walk solves it by construction, and
// the walk is the feature).
//
// THE HANDS WIN. Any hand-authored move, look or zoom this frame ends
// the visit at once, with a line. So does arriving, the point leaving
// the pawn, and a stall — no closer approach for PILOT_STALL_S: a wall,
// a monument, a body the whisper could not bend around.
//
// THE FRAME. coupling_input_to_pawn_velocity (world.wgsl) maps intent
// (mx, mz) to world velocity (mx·cos az + mz·sin az, −mx·sin az +
// mz·cos az), az = camera_state.azimuth. Its inverse is its transpose:
// for a unit world direction (ux, uz),
//     mx = ux·cos az − uz·sin az,   mz = ux·sin az + uz·cos az.
// W is mz = −1 and lands on (−sin az, −cos az), the camera's forward —
// the same frame the free-fly branch uses, verified against both. The
// azimuth is the P5 readback's, one frame stale (E-4): a quarter of a
// world unit at PAWN_SPEED.
//
// THE AIM. build_view_projection_matrix looks along −(sin az, cos az)
// in XZ, so the azimuth that faces the painting from the eye is
// atan2(−vx, −vz) for v = painting − eye; the pilot turns the orbit
// toward it at PILOT_TURN_RATE and stops within a degree.
inline void pilot_tick(InputDeps* c, double now, double dt) {
    PilotState& p = c->pilot_;
    if (!p.active) return;
    // Re-fold the hands from their held state (pure in keys_/touch_), so
    // move_x/move_z are the HANDS' this frame and the pilot's own last
    // write is gone before anything reads it as a hand.
    update_movement_intent(c);
    const InputState& in = c->inputState_;
    if (in.move_x != 0.0f || in.move_z != 0.0f
        || in.look_az_delta != 0.0f || in.look_el_delta != 0.0f
        || in.zoom_delta != 0.0f) {
        std::cout << "[Visit] released: hands (slot " << p.slot << ")\n";
        p.active = false;
        return;
    }
    if (c->point_.host != PointHost::PAWN) {
        std::cout << "[Visit] released: the point left the pawn (slot " << p.slot << ")\n";
        p.active = false;
        return;
    }
    const float dx = p.tx - c->point_.x;
    const float dz = p.tz - c->point_.z;
    const float d  = std::sqrt(dx * dx + dz * dz);
    if (d <= PILOT_ARRIVE_WU) {
        std::cout << "[Visit] arrived: slot " << p.slot << ", " << d << " wu from the mark\n";
        p.active = false;
        return;
    }
    if (d < p.best_d - PILOT_PROGRESS_WU) { p.best_d = d; p.best_at = now; }
    else if (now - p.best_at > PILOT_STALL_S) {
        std::cout << "[Visit] released: stalled " << d << " wu from slot " << p.slot << "\n";
        p.active = false;
        return;
    }
    if (!c->camera_pose_.valid) return;   // no pose yet: wait, do not guess
    const float az = c->camera_pose_.azimuth;
    const float ca = std::cos(az), sa = std::sin(az);
    const float ux = dx / d, uz = dz / d;
    const float gain = std::min(1.0f, std::max(0.25f, d / PILOT_SLOW_WU));
    c->inputState_.move_x = (ux * ca - uz * sa) * gain;
    c->inputState_.move_z = (ux * sa + uz * ca) * gain;
    {   // the fold's own clamp: two hands cannot buy more than full speed, nor can a third
        const float m = std::sqrt(c->inputState_.move_x * c->inputState_.move_x
                                + c->inputState_.move_z * c->inputState_.move_z);
        if (m > 1.0f) { c->inputState_.move_x /= m; c->inputState_.move_z /= m; }
    }
    const float vx = p.aim_x - c->camera_pose_.eye[0];
    const float vz = p.aim_z - c->camera_pose_.eye[2];
    float want = std::atan2(-vx, -vz) - az;
    while (want >  3.14159265f) want -= 6.28318531f;
    while (want < -3.14159265f) want += 6.28318531f;
    const float step = PILOT_TURN_RATE * (float)dt;
    if (std::fabs(want) > 0.0175f)
        c->inputState_.look_az_delta = std::max(-step, std::min(step, want));
}

// THE DOOR'S VERB. Called once, at the frame boundary, with the slot's
// facts (position, facing, larger side) — scalars, so this driver needs
// no gallery type. `forward` is the quad's facing outdoors and the wall
// normal indoors; the standing point is in front of the picture either
// way, and the visual gate says which sign the wall normal carries.
inline void begin_visit(InputDeps* c, uint32_t slot, float px, float pz,
                        float fx, float fz, float span, double now) {
    PilotState& p = c->pilot_;
    // The R key's own transaction, with its own guards: a visit walks, so
    // the pawn must host. From the ribbon this is the landing ease; the
    // pilot waits it out (PILOT_STALL_S outlasts the ease).
    if (c->point_.host != PointHost::PAWN) possess(c, PointHost::PAWN);
    if (c->point_.host != PointHost::PAWN) {
        std::cout << "[Visit] refused: no pawn to walk (slot " << slot << ")\n";
        return;
    }
    const float off = std::max(PILOT_STANDOFF_MIN_WU, span * PILOT_STANDOFF_MULT);
    p.tx = px + fx * off;   p.tz = pz + fz * off;
    p.aim_x = px;           p.aim_z = pz;
    p.slot = slot;          p.active = true;
    p.best_d = 1e9f;        p.best_at = now;
    std::cout << "[Visit] slot " << slot << ": walking to (" << p.tx << ", " << p.tz
              << "), stand-off " << off << " wu\n";
}
```
`possess` is declared in the declarations block above and defined later in this file; an
inline function may call it on the declaration. `std::sqrt`, `std::min`/`std::max` and
`std::cout` are already used in this file.

### U3.4 — the organ at the root, the deps init, the row (`cartridge.hpp`)

FIND
```
            TouchMoveState touch_;   // SHIP_1 — the stick's organ; never written on native
```
REPLACE
```
            TouchMoveState touch_;   // SHIP_1 — the stick's organ; never written on native
            PilotState pilot_;       // VISIT_0 — the third hand's organ
```

FIND
```
                , input_deps_{ inputState_, keys_, mouse_, touch_, player_, world_state_, ribbon_state_, gpuState_, device_, point_, mount_, camera_ }
```
REPLACE
```
                , input_deps_{ inputState_, keys_, mouse_, touch_, player_, world_state_, ribbon_state_, gpuState_, device_, point_, mount_, camera_, camera_pose_, pilot_ }
```
(References to members bind before construction as `touch_` and `point_` already do; order
of declaration does not matter for a reference member.)

FIND
```
                FillSignal, AdvanceClock, MotionDrivers, MotionBodies,
                StageWorld, TransitionMachine, StageFadeUpload, WitnessPhotographer,
                ClearInputDeltas, COUNT
```
REPLACE
```
                Pilot, FillSignal, AdvanceClock, MotionDrivers, MotionBodies,
                StageWorld, TransitionMachine, StageFadeUpload, WitnessPhotographer,
                ClearInputDeltas, COUNT
```

FIND
```
            void phase_fill_signal(UpdateCtx& c) {
```
REPLACE
```
            // U0 — THE PILOT (VISIT_0): the third hand, authored before the
            // signal fill copies the channel. Inert unless a visit is on.
            void phase_pilot(UpdateCtx& c) {
                pilot_tick(&input_deps_, time_state_.seconds, (double)c.signal.dt);
            }

            void phase_fill_signal(UpdateCtx& c) {
```

FIND
```
                { UPhase::FillSignal,          "fill_signal",           &Cartridge::phase_fill_signal,           Driver::Mixed,     true,             F_SIGNAL | F_CLOCK },
```
REPLACE
```
                { UPhase::Pilot,               "pilot",                 &Cartridge::phase_pilot,                 Driver::Algo,      true,             F_SIGNAL },   // FOUNDATIONAL: the driver's third hand, authored before the fill copies the channel (VISIT_0)
                { UPhase::FillSignal,          "fill_signal",           &Cartridge::phase_fill_signal,           Driver::Mixed,     true,             F_SIGNAL | F_CLOCK },
```

FIND
```
            static_assert((uint32_t)UPhase::FillSignal < (uint32_t)UPhase::AdvanceClock, "O-5a: dt_beats reads prev_beats before the clock advances it");
```
REPLACE
```
            static_assert((uint32_t)UPhase::FillSignal < (uint32_t)UPhase::AdvanceClock, "O-5a: dt_beats reads prev_beats before the clock advances it");
            static_assert((uint32_t)UPhase::Pilot < (uint32_t)UPhase::FillSignal, "O-5f: the pilot's hand is authored before the signal fill copies the channel (VISIT_0)");
```

### U3.5 — the boundary drain and the window (`organ_boundary.inc`)

FIND
```
                {
                    uint32_t host = 0;
                    if (t7::organ::take_go_host(host))
                        possess(&input_deps_, static_cast<PointHost>(host));
                }
```
REPLACE
```
                {
                    uint32_t host = 0;
                    if (t7::organ::take_go_host(host))
                        possess(&input_deps_, static_cast<PointHost>(host));
                }

                // THE VISIT DOOR (VISIT_0) — a door with a parameter, taken
                // once: the shell names a hung photograph; begin_visit turns
                // its slot into a standing point the pilot walks to. An
                // empty slot is refused here, in words, because the roll the
                // shell pressed may be two seconds older than the wall.
                {
                    uint32_t slot = 0;
                    if (t7::organ::take_visit(slot)) {
                        const GPUPaintingSlot& s = gallery_state_.painting_slots[slot];
                        if (s.is_active == 0u)
                            std::cout << "[Visit] refused: slot " << slot << " is empty now\n";
                        else
                            begin_visit(&input_deps_, slot,
                                        s.position[0], s.position[2],
                                        s.forward[0],  s.forward[2],
                                        std::max(s.scale_x, s.scale_y),
                                        time_state_.seconds);
                    }
                }
                // THE VISIT WINDOW, refreshed here and nowhere else (the rule
                // window's charter): the pane reads whether a walk is on.
                t7::organ::set_visit_view(pilot_.active, pilot_.slot);
```

### U3.6 — the census enrolment (`tools/gates/score/run.py`)

By symbol: add `phase_pilot` to `FOUNDATIONAL_PHASES` in the form its neighbours use (a
key with a one-line justification string if the dict carries one; a bare member if it is a
set), with the justification: `the driver's third hand — intent authored before FillSignal
copies the channel (VISIT_0)`. Read the dict first (U0 printed it).

### U3 witness
```
python3 tools/gates/score/run.py           # GREEN — the new row is attributed; if RED, the dict form was misread: fix, flag
python3 tools/gates/console_gate/run.py    # PASS
grep -c "PilotState" src/cartridges/the_board/direction/input.hpp src/cartridges/the_board/cartridge.hpp   # >= 2 / 1
grep -c "UPhase::Pilot" src/cartridges/the_board/cartridge.hpp    # 2 (row + static_assert)
grep -c "take_visit\|set_visit_view" src/cartridges/the_board/organ_boundary.inc   # 2
grep -n "shot_names" src/cartridges/the_board/bodies/gallery.hpp   # still no lines
```
`glaw1` is Jean's: this unit is the one that compiles the pilot for the first time.

Commit: `VISIT_0 U3 — the pilot: a third hand on the intent channel; Pilot spine row before FillSignal; the visit door drained at the boundary`

### U3 gate (Jean, the held branch's web build; P10/P14)
| should FIX | could BREAK (what you would say out loud) |
| --- | --- |
| Boot prints `[Visit] roll bound; pilot idle`. | Nothing about the world changes until a visit is pressed — anything that does is a regression, not this campaign. |
| Pressing a visit (U4, or `Module.ccall('gallery_visit', null, ['number'], [<slot>])` from the console): the pawn turns and walks; the camera swings to face the picture; it stops in front of it with `[Visit] arrived`. | "The pawn walks away from / sideways to the painting" — the intent frame; both derivations are in the block comment. |
| Touching W/A/S/D or the stick stops it at once with `[Visit] released: hands`. | "The pawn keeps walking after I touched the keys." |
| A visit from free-fly or from the ribbon first possesses the pawn, then walks. | "The world jumped" on the possess — that is `possess()`'s own ease and would be its regression, not the pilot's. |
| — | "The pawn walks into the wall behind an indoor painting" — the wall normal's sign; flip `fx, fz` for `WALL_FRAME` at the boundary and flag. |
| — | "The pawn shivers in front of the painting" — `PILOT_ARRIVE_WU` under one stride; raise it. |
| — | "The camera spins" — the wrap; the two `while` lines. |
| — | "It gave up in the open" — `[Visit] released: stalled` with a large distance; `PILOT_STALL_S` shorter than a mount ease, or the whisper steering the body around something. |

---

## U4 — THE PANE

**Precondition (P13, the state):** `web/index.html` carries DOORS_0 U6 — the sandwich's
behaviours block with `C = { pace: w('shell_pace', …) }` and a `.pane[data-pane="collection"]`.
If it does not: **defer** this unit (record it in the report and in U5's OPEN.md entry as
"VISIT_0 U4 pending DOORS_0"), continue to U5. Do not build a menu here.

### U4.1 — the route (`web/routes.json`)

FIND
```
  { "id": "controls",   "label": "Controls",       "engine": "pane" },
```
REPLACE
```
  { "id": "controls",   "label": "Controls",       "engine": "pane" },
  { "id": "roll",       "label": "Photographs",    "engine": "pane" },
```

### U4.2 — the rules (`web/menu.css`)

Append at the end of the file:
```

/* ── VISIT_0 — the roll: a list of captions, each a button that walks ── */
.roll { list-style: none; margin: 0; padding: 0; }
.roll li + li { border-top: 1px solid #1f1e26; }
.roll button { display: block; width: 100%; text-align: left; font: inherit; color: var(--dim); background: none; border: 0; padding: 7px 0; cursor: pointer; }
.roll button:hover, .roll button:focus-visible { color: var(--ink); }
.roll-state { margin: 8px 0 0; color: var(--ink); }
```

### U4.3 — the pane (`web/index.html`)

FIND
```
      <a class="out" data-out target="_blank" rel="noopener" href="/collection/">Open the collection in a new tab →</a>
    </div>
  </details>
```
REPLACE
```
      <a class="out" data-out target="_blank" rel="noopener" href="/collection/">Open the collection in a new tab →</a>
    </div>
    <!-- VISIT_0 — THE ROLL. A directory of what the world has hung of its
         own photographs; choosing one walks the pawn to it. The pictures
         never leave the GPU: the list is the gallery's slot array read
         through the registry's window (gallery_roll). Placeholder words. -->
    <div class="pane" data-pane="roll" hidden>
      <button type="button" class="back" data-back>← menu</button>
      <p class="pane-head">Photographs</p>
      <p class="pane-note">What the world has hung of its own pictures. Choose one and the pawn walks to it; touch anything to stop.</p>
      <ol class="roll" id="roll"></ol>
      <p class="roll-state" id="rollState" hidden></p>
    </div>
  </details>
```

### U4.4 — the behaviours (`web/index.html`)

FIND
```
        C = {
          pace: w('shell_pace', null, ['number'])
        };
```
REPLACE
```
        C = {
          pace:     w('shell_pace', null, ['number']),
          roll:     w('gallery_roll', 'string', []),          // VISIT_0
          visit:    w('gallery_visit', null, ['number']),     // VISIT_0
          visiting: w('gallery_visiting', 'number', [])       // VISIT_0
        };
```

FIND
```
        if (id === 'collection') loadPeek();
```
REPLACE
```
        if (id === 'collection') loadPeek();
        if (id === 'roll') startRoll(); else stopRoll();     // VISIT_0
```

FIND
```
      menu.addEventListener('click', function (e) {
```
REPLACE
```
      // ═══ VISIT_0 — THE ROLL: WHAT THE WORLD HAS HUNG OF ITS OWN PICTURES
      // A directory, not a gallery: the pictures stay in the world; the
      // list takes you to them. gallery_roll() is the gallery's slot array
      // read through the registry's window; gallery_visit(slot) is a door
      // with a parameter; gallery_visiting() says whether a walk is on.
      // Refreshed every two seconds while the pane is open: the world
      // rehangs, and a list that does not move is a list of ghosts.
      var rollEl = document.getElementById('roll');
      var rollState = document.getElementById('rollState');
      var rollTimer = null;
      function compass(b) {            // bearing from the point, 0 = +Z, clockwise. Words, not law.
        var n = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW'];
        return n[Math.round((((b % 360) + 360) % 360) / 45) % 8];
      }
      function caption(w) {
        var age = w.age < 0 ? '' : (w.age < 90 ? Math.round(w.age) + ' s ago' : Math.round(w.age / 60) + ' min ago');
        return [w.tier, Math.round(w.d) + ' wu ' + compass(w.b), age, w.wall ? 'on a wall' : 'on the ground']
          .filter(Boolean).join(' · ');
      }
      function refreshRoll() {
        if (!rollEl || !rollState) return;
        var c = abi();
        if (!c || !c.roll) { rollEl.innerHTML = ''; rollState.hidden = false; rollState.textContent = 'Not in this build.'; return; }
        var list;
        try { list = JSON.parse(c.roll()); } catch (err) { note('[shell] roll: ' + err); return; }
        rollEl.innerHTML = '';
        if (!list.length) { rollState.hidden = false; rollState.textContent = 'Nothing hung yet — walk a while.'; return; }
        list.sort(function (a, b) { return a.d - b.d; });
        list.forEach(function (w) {
          var li = document.createElement('li');
          var b = document.createElement('button');
          b.type = 'button'; b.dataset.visit = w.s; b.textContent = caption(w);
          li.appendChild(b); rollEl.appendChild(li);
        });
        var v = c.visiting ? c.visiting() : -1;
        rollState.hidden = v < 0;
        if (v >= 0) rollState.textContent = 'Walking to one — touch anything to stop.';
      }
      function startRoll() { refreshRoll(); if (!rollTimer) rollTimer = setInterval(refreshRoll, 2000); }
      function stopRoll()  { if (rollTimer) { clearInterval(rollTimer); rollTimer = null; } }

      menu.addEventListener('click', function (e) {
```

FIND
```
        if (t.closest('[data-back]')) { showPane(null); return; }
```
REPLACE
```
        if (t.closest('[data-back]')) { showPane(null); return; }
        var v = t.closest('button[data-visit]');                             // VISIT_0
        if (v) {                                                              // choosing one closes the menu:
          var c = abi();                                                      // the walk happens in full view,
          if (c && c.visit) c.visit(+v.dataset.visit);                        // the world un-idles
          menu.open = false;
          return;
        }
```

### U4 witness
```
grep -c 'data-pane="roll"'   web/index.html   # 1
grep -c "gallery_roll\|gallery_visit\|gallery_visiting" web/index.html   # 3
grep -c "__BUILD_ID__"       web/index.html   # 1
grep -c "/\* __MENU_CSS__ \*/" web/index.html # 1
grep -c "<!-- __ROUTES__ -->"  web/index.html # 1
python3 tools/routes.py | grep -c 'data-pane="roll"'   # 1 (engine side only)
python3 - <<'EOF'
import io,sys; sys.path.insert(0,'tools'); import web_dist
t=io.open('web/index.html',encoding='utf-8',newline='').read()
assert '\r' not in t
print('boot-set violations:', web_dist.boot_set_violations(t) or 'none')
EOF
```

Commit: `VISIT_0 U4 — the Photographs pane: gallery_roll captions, each a door that walks the pawn there`

### U4 gate (Jean)
| should FIX | could BREAK |
| --- | --- |
| *menu → Photographs* lists what is hanging, nearest first: `Portrait · 118 wu NE · 2 min ago · on the ground`. The list changes as the world rehangs. | "It says Not in this build" on the held branch's build — an export name mismatch between the shell and the registry. |
| Choosing one closes the menu and the pawn walks (U3's rows). | "Nothing happens when I choose one" — check for `[Visit] refused` in the console. |
| Before any snapshot is hung: *Nothing hung yet — walk a while.* | The pane lists paintings from disk — `content_source` filter missed. |

---

## U5 — LEDGERS, THE REGISTER, MERGE READINESS

1. The cascade, in order (D9):
   ```
   python3 tools/binding_ledger.py
   python3 tools/command_census.py
   python3 tools/mirror_census.py
   python3 tools/binding_ledger.py --check && python3 tools/command_census.py --check && python3 tools/mirror_census.py --check
   python3 tools/organ_ledger.py --check      # PASS, or regenerate and flag
   python3 tools/binding_gen.py --check       # PASS — no binding changed
   python3 tools/gates/score/run.py && python3 tools/gates/shell_gate/run.py && python3 tools/gates/glaw2/run.py && python3 tools/gates/console_gate/run.py
   ```
   Expected changed files: `COMMAND_LEDGER.md`, `BINDING_LEDGER.md`, `MIRROR_LEDGER.md`
   (the `gallery.hpp` / `cartridge.hpp` / `spine_state.hpp` pins). List every changed line
   group in the report.

2. `docs/OPEN.md` — read whole; in the register's own form:
   - open **VISIT_0** as a held-branch campaign (`claude/visit-0`) awaiting Jean's gates: glaw1,
     the visual rows of U3 and U4, merge;
   - register residuals: the pilot's seven numbers are control-panel material, unenrolled
     until a measurement asks; the wall-normal sign for indoor stand-off is a visual finding;
     bearing words (`compass`) name +Z "N" — a convention, not a fact of the world; the roll
     refreshes by polling (2 s) because the registry has no event out — acceptable at one
     string per two seconds, revisit if the pane grows; the ROLL item (snapshots as a strip
     of film) is superseded by this campaign's directory form and is struck;
   - if U4 was deferred, say so and name its precondition (DOORS_0 on master).

Commit: `VISIT_0 U5 — ledgers regenerated (gallery/cartridge/spine_state pins); OPEN.md: VISIT_0 open on claude/visit-0, residuals registered`

Push the branch (`git push -u origin claude/visit-0`). Do not merge. The report names the
branch tip and the commands Jean runs: `cmake --build --preset the-board-web`,
`python tools\web_dist.py`, `npx wrangler pages dev dist`.

---

## THE ROUND'S REPORT

Per unit — landed / flagged / halted / deferred; witness output verbatim; every default
taken; every count that differed and what the tree said. Cite symbols. Then the gate tables
are Jean's.

## WHAT THIS DOCUMENT CANNOT SEE (P16)

It did not read `possess()` past its guards, so the landing ease's duration is a bound
(`PILOT_STALL_S = 6 s`) not a measurement. It did not read `fill_slot_wall_frame`'s use of
the wall normal, so the indoor stand-off's sign is a gate row. It did not read
`FOUNDATIONAL_PHASES`'s form, so U3.6 is by symbol. It read `coupling_input_to_pawn_velocity`
and the free-fly branch whole, so the intent frame is a derivation from the tree, not a
hypothesis — and it is still gated, because a derivation is not a frame on a screen (P10).
