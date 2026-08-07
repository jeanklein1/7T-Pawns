# PORTAL_0 — THE DOOR CENSUS

READING ONLY. Zero edits, zero builds. Every claim carries file + symbol at
HEAD `850f896`. Absences are stated plainly. Seven questions, seven answers.

## 1. TRIGGERS — who initiates a world transition today

Exactly two initiator families post-CUT_1; nothing else writes the phase:

- **Keys 5–8** (`direction/input.hpp`, the four `GLFW_KEY_5..8` cases) →
  `request_mood_transition(transitionPhase, pendingDestination, mood_state,
  world_state, MOOD_*)` (decl graduated to `contracts/spine_state.hpp`; body
  `direction/mood.hpp:1168`) → sets `pendingDestination_` and
  `transitionPhase_ = FADE_OUT`.
- **The GPU portal trigger** — the bubble sensor writes
  `agent.portal_trigger = i32(p.arch_index)` on the possessed slot
  (`world.wgsl:7075-7102`); the agent readback harvests it into
  `point_.portal_trigger` (the P5 wire); `phase_portal_trigger`
  (`cartridge.hpp:1249-1262`, spine row `RPhase::PortalTrigger`, gated
  `ROSTER.transitions`) validates the arch (`active && is_portal`), copies
  `arches[i].destination` into `pendingDestination_`, and sets `FADE_OUT`.

No timers, no other writers of `transitionPhase_` (F-keys died in CUT_1b; the
camera host died in CUT_1e). **Portals already initiate transitions today** —
the key handlers are the residue portals will replace.

## 2. ANATOMY — the transition frame-by-frame

`TransitionPhase { IDLE, FADE_OUT, TEARDOWN, FADE_IN }`
(`contracts/spine_state.hpp:90`); machine = `phase_transition_machine`
(`cartridge.hpp:949`, spine row `UPhase::TransitionMachine`).

- **FADE_OUT**: `transition_fade_alpha` ramps 0→1 over
  `transition_fade_duration = 0.5 s` per direction
  (`spine_state.hpp:131`) — ~30 frames at 60 fps. The eye sees a black
  overlay ramp (`set_fade(alpha, 0,0,0)` `cartridge.hpp:1106`; drawn by the
  fade pass, `render_passes.hpp:527`). The world keeps running beneath it.
- **TEARDOWN**: **one frame** — the entire case block (`cartridge.hpp:961-1086`)
  runs in a single spine tick: `world_gen++` (stale-callback guard), return
  capture, `reset_surface` / `teardown_entities` / `teardown_gol` /
  `teardown_ribbon` / `teardown_gallery` (painting rotation lives here) /
  `teardown_pawn_aura` / `teardown_orbs`, point + player reset, `apply_mood`,
  repopulation, then `transitionPhase_ = FADE_IN` (:1078). The eye is at
  full black (alpha 1) for this frame.
- **FADE_IN**: alpha 1→0 over 0.5 s (~30 frames) while patch generation
  streams under the per-frame budget (`patch_system.hpp` generation ring);
  the reveal and the stream-in overlap by design.

Total ≈ 61 frames at 60 fps: fade / one-frame cut under black / fade. There
is no mid-world teleport anywhere — the mechanism is always
fade→rebuild→fade. This is the vehicle a portal ride inherits as-is.

## 3. DESTINATION — how the next world is chosen

`PortalDestination { seed, finite, finite_radius, mood }`
(`contracts/mood_constants.hpp`). Two grammars today:

- **Portal arches carry their destination BEFORE the transition fires.** It
  is authored at spawn: `pick_portal_mood(&ctx, active_seed, prop)`
  (`mood.hpp:212`, call sites e.g. :1089) derives the mood deterministically
  from the ACTIVE world's seed + a property salt, and the full
  `PortalDestination` is written onto the arch (`arches[i].destination`,
  read back at `cartridge.hpp:1256`). The back portal
  (`force_spawn_back_portal`, `mood.hpp:893`) carries the CAPTURED return
  state — `back_portal_return_seed / _mood / _radius` (captured at TEARDOWN,
  `cartridge.hpp` :993-995 region). Finite worlds force-spawn their exits
  (`force_spawn_finite_portals`, `mood.hpp:1015`, called from both indoor
  and finite-outdoor apply paths :983/:1012).
- **Keys carry only a mood id** (`MOOD_OPEN_SUNSET=0, MOOD_INDOOR_FLAT=1,
  MOOD_INDOOR_VAULT=2, MOOD_FINITE_OUTDOOR=3`; `MOOD_COUNT=4`);
  `request_mood_transition` fills the rest of the destination at request
  time.

**A portal that leads somewhere already knows where** — no ruling needed for
the arch grammar; roll-on-entry exists only in the key path being replaced.

## 4. SENSING — "the pawn is inside region V", the existing inventory

| Mechanism | Home | Rate |
|---|---|---|
| **The portal bubble** — per-portal oriented ellipse (`lat/fwd` against `facing_cos/sin`, `inv_span_sq/inv_depth_sq`), possessed-slot probe | `world.wgsl:7075-7102` over `portal_array` (g0:62, CPU-staged `cpuPortalArray_`) | GPU, per frame, ≤ `portal_array.count` portals |
| The bubble's vertical gate (`point_bubble_radius=20`, `contracts/point.hpp:76-89`) | same site — but the altitude arm runs only under `point_camera_hosted()`, **permanently false since CUT_1e**; pawn-host firing is xz-ellipse only (the pawn is ground-bound, so altitude is implicit) | — |
| Agent readback wire (region result → CPU) | `cartridge.hpp` P5 harvest → `point_.portal_trigger` | per frame, 1-frame stale by law E-4 |
| Patch distance tests | `patch_distance_sq` (`surface/patch_system.hpp`, the banding loop) | CPU, per frame, per resident patch |
| Tile cache / grid queries | `surface/tile_world.hpp` (`tileCache_`), `GPUTileGrid` (g0:25) | CPU per event; GPU per frame |
| Footprint registry (occupier separation) | `machine/spawn_engine.hpp` (§footprint registry, :51-) | CPU, spawn-time queries |
| Ground/manifold query | `manifold_position(pos, POLICY_*, qi)` (`world.wgsl`, used by the bubble's altitude arm) | GPU, on demand |

## 5. THRESHOLDS — geometry that already reads as a door

- **Portal arches are doors, wired end-to-end**: `arches[i].is_portal` +
  `.destination` (read at `cartridge.hpp:1253-1256`), oriented span/depth in
  the portal array, spawned by `force_spawn_back_portal` /
  `force_spawn_finite_portals` (`mood.hpp:893/:1015`); ordinary arches
  (`is_portal=false`) are the same geometry, un-wired.
- Indoor shells: FLAT and VAULT rooms (`contracts/indoor_module.hpp`; vault
  spring/rise/crown `mood.hpp:609`) — walls at
  ±`finite_radius`·`PATCH_EXTENT` (`mood.hpp:592-593`); openings exist only
  where portals stand.
- Gallery painting frames (indoor walls, `bodies/gallery.hpp`
  `place_wall_paintings`) — visual thresholds, no sensing wire.
- **Spatial relation of moods: DISJOINT.** Every transition is a full
  TEARDOWN rebuild; finite/indoor worlds center generation at (0,0)
  (`patch_system.hpp:517-519`); no two moods coexist; the only cross-world
  reference anywhere is `PortalDestination` on an arch.

## 6. THE MOOD SURFACE — the switch the router will own (names + homes)

- Mood ids + count + `PortalDestination`: `contracts/mood_constants.hpp`.
- `MoodProfile` table + `apply_mood` + `pick_portal_mood` +
  `derive_finite_radius`: `direction/mood.hpp` (per-mood colors :113;
  activation is spine-owned via `pendingDestination_/transitionPhase_`).
- World shape: `finite_mode / finite_radius / active_radius / active_seed`
  (`WorldState`, `contracts/surface_services.hpp`); GPU clamp
  `world_bound_min/max` via `set_world_bounds` (`state.hpp:2597`).
- Light + sky: `sunDirection_ / sunColor_ / clearColor_ / cpuSpotLights_`
  (staged through `mood_deps_`, `cartridge.hpp:405`).
- Population: `MOOD_SPAWN_MULT` (`surface/population_themes.hpp`); indoor
  entity law (`contracts/indoor_module.hpp`).
- Portals: `cpuPortalArray_` + `backPortalPosition_` (mood_deps).
- Return state: `back_portal_return_seed/_mood/_radius` (`MoodState`,
  `contracts/spine_state.hpp`).

## 7. CONTINUITY across TEARDOWN

- **Pawn position: RESET**, not held — `point_.x/z = Idle::PAWN_POS_X/Z`
  (`cartridge.hpp` :1036-1038 region), and `reset_player_agent` re-seeds the
  body at the spawn anchor.
- **Pawn identity: PRESERVED** — tier, color RGB, and skin are captured from
  the possessed slot before the reset and re-applied
  (`preserved_tier/color/skin`, `cartridge.hpp` :1038-1045 region).
- **Possession: unchanged** — `point_.host` is untouched by TEARDOWN (and
  since CUT_1e it can only be PAWN); `point_.portal_trigger` is cleared.
- **Camera: HELD** — no teardown verb touches `camera_state` (the GPU
  integrator persists across the rebuild and re-kites onto the reset pawn);
  no CPU camera reset exists in the block.
- **Return state: CAPTURED** — seed/mood/radius into `MoodState` before the
  overwrite, feeding the back portal.

---

*Product of PORTAL_0. The design campaign binds to: the arch grammar
(§3, §5) as the door; the bubble (§4) as the sensor; the fade→rebuild→fade
vehicle (§2) as the ride; the key handlers (§1) as the residue to retire.*
