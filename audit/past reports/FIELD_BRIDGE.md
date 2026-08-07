# FIELD BRIDGE — session handover (written at Phase B CLOSED, merged)

READ FIRST in any fresh window continuing the field work. The tree
is the memory; this file is the index.

## STATE
- Phase A COMPLETE and Phase B MERGED to master (merge e4394ac;
  gated by Jean at 7a07320). The branch `claude/field-phase-b` is
  merged history; the proxy blocked its remote deletion — Jean
  deletes from the design machine.
- THE PANEL exists: contracts/control_panel.hpp authors the field's
  dials (SLACK 3.0, K 300, FMAX 600, occupier/authored mutes 1.0,
  class gains 4/1/4) and the beacon four (R0 25, R 120, S 200,
  LIFT 20; static_assert S < K). The eight WGSL dials ride
  DesignConfig (sizeof 592 → 624, both rooms); rests boot-pinned
  from the panel. gain applies AFTER the FMAX clamp. F5 drives the
  beacon (authored table g2:5, writer in phase_motion_drivers).
- THE LAW HAS A HOME: world.wgsl's "THE FIELD — ONE PRESENCE LAW"
  banner (above field_pair) carries the endpoint and rulings R1–R6
  — the L2-banner precedent, law where it binds. This file indexes;
  that banner governs. LAWS.md untouched (its numbers are Jean's
  to mint).

## RULINGS (index — the banner is the register)
R1 presence migrates, APPROACH stays behavioral · R2 the point's
rows are a separate undesigned arc · R3 possessed emits ×4 to agent
lanes only, never subscribes · R4 one law, two transports (buffer /
direct call — orbs the precedent) · R5 the lure is lateral, the pen
owns altitude · R6 no anti-windup until a measurement asks.

## SUCCESSION LEDGER (the body arc, CLOSED at the merge)
DEAD, landed on master
  agent↔agent contact spring + row_agent_contact ......... B2
  agent←sphere presence + row_agent_sphere ............... B1
  orb↔orb 1/d² separation kernel ......................... B3
  agent-side occupier_contact call ....................... B4a/b
ALIVE — APPROACH class, exempt by R1 (never targets)
  agent↔agent flee/dodge · pursuit · flee behavior ·
  flock cohesion + alignment · orb alignment + cohesion
ALIVE — the POINT ARC, undesigned (R2; needs Jean's design round)
  row_cube_push (cube←point) · row_sphere_push (+camera twin) ·
  row_point_flee / agents-part · occupier_contact, now solely the
  possessed candidate's row (R3)
Survivor census (post-merge): 9 influence_response callers — 3 in
occupier_contact, 2 flee-dodges, pawn/camera sphere push, point
flee, cube push. All point-arc or APPROACH. Zero dead dials.

## KNOWN DEFECTS / DIALS
- Beacon clot: ring capacity ~9 cubes at r0 25 (circumference /
  shell width); dials now at the panel (still rebuild-to-tune until
  FIELD_5 / the panel proper give them a transport). Authored
  SHAPES (rings/arcs) unbuilt.
- The B1/B2/B4 re-pricings passed their gates as-priced.
  Contingencies stay named and unbuilt: FIELD_SLACK_AGENT
  (distance), FIELD_OCCUPIER_GAIN_AGENT (shaft standoff),
  subscriber-mass division (yield — the emitter-side-only mass
  shape is a fidelity gap vs the retired row's two-mass division;
  judged acceptable at gate B2, fix named if authority ever reads
  flat).
- SPAWN_3 parked: F6 newborns seat inside render radius on move.

## ELIMINATION INVENTORY (candidates for the coming round —
RULINGS ONLY, nothing deleted; locations at this writing)
1. Panel exhibit two, the LEAD: the frame-law mirror trio
   MOUNT_TANGENT_ALIGN / MOUNT_BANK_GAIN / MOUNT_BANK_MAX
   (ribbon.hpp, "Frame-law mirrors — LOCKSTEP MIRRORS of
   world.wgsl's") — the last constant-valued lockstep mirror in
   the ribbon room.
2. Panel exhibit two, riders: RIBBON_FIELD_GAIN_XZ / _GAIN_Y /
   RIBBON_FIELD_LOOKAHEAD (ribbon.hpp) — single-homed, no L3
   hazard; join when the panel proper is designed.
3. The whisper pair STEER_LOOKAHEAD_WU / STEER_GAIN (+ the
   GRAD band, world.wgsl) — gate-instrument family, single-homed;
   candidates for a later config graduation, same L5 shape as
   FIELD_2b.
4. Formula lockstep, NOT panel-fixable: the ribbon wave mirrored
   CPU-side "in LOCKSTEP with the GPU ring motor" (ribbon.hpp,
   ribbon_rebuild/mount region) — a duplicated FORMULA, not a
   constant; consolidation needs a design ruling.
5. LOCKSTEP HAZARD, data layout: entity_pipeline.hpp:~358 (tier
   rows vs GPU row order) — witness-less; inventory for the
   census instrument.
6. The point arc itself (R2): four rows + their six constants
   (SPHERE_PUSH_GAIN, CUBE_PUSH_*, BUBBLE_PART_SPEED,
   point_bubble_radius's flee row) — migrate-vs-permanent is THE
   design round; until ruled, they are alive, not residue.

## QUEUE (fresh session picks from here)
1. The point-grammar arc — design round first (R2).
2. Geometry coverage: palms/cacti/blades/galleries/GoL/indoor —
   parity-with-agents was the FIELD_3 ruling; revisit scope.
3. Rings as individual subscribers (they emit; only the head yields).
4. FIELD_5: ch1 → beacon strength/radius (rows coupling-eligible).
5. The panel proper: runtime transport for the panel's dials;
   exhibits per the inventory above.
6. Layer E: the frame is geometry-bound (~22/29 ms). RULED: no
   field-vs-prior METER comparison — the field is load-bearing
   regardless, and a general optimization round is imminent;
   measure there, on the whole frame.

## ANCHORS
THE FIELD LAW banner: world.wgsl, above field_pair (~7729) ·
field consts + index map: world.wgsl ~2230 (dials in config;
FIELD_SUBSCRIBERS the pinned dimension) · THE PANEL:
contracts/control_panel.hpp · the room: binding_registry g2:0–5 ·
authored writer: cartridge.hpp phase_motion_drivers · ribbon field
block: ribbon.hpp (lookahead + lure; trio reads the panel) · recon
corpus: audit/FIELD_1_REPORT.md · laws: src/docs/LAWS.md.
