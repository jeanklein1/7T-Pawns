════════════════════════════════════════════════════════════════════
7T — PULSE_SPLIT_0 — THE LEAP STOPS RINGING
Four units on master. The one-finger word sheds the ring (U1); the
ledgers follow (U2); the register closes (U3). Authored by Claude at Jean's word: "keep space
and one finger tap only for leap".
════════════════════════════════════════════════════════════════════

GIT LAW: trunk-based, master, direct. One commit per unit, messages
verbatim below, no squash, no branch, no PR. Jean gates build,
visual, deploy, naming.

REGISTER (P17 DEFAULT-AND-FLAG): verify every FIND verbatim and report
its count before the edit; every count below is 1. A mismatch STOPS
THE UNIT, not the round.

THE DESIGN, IN THREE SENTENCES. `request_radial_pulse` raises two
intents today — the ring and the body's verb — so the leap cannot be
taken without ringing the ground. This order removes the ring from
that door: `pulse_pending` is raised only by `request_pulse_swap`
(CAPS_LOCK, the right-half pair tap) and by the musical bus
(`emit_radial_pulse`), which is untouched. One line of behaviour; the
rest is comment made true again.

THE HAND, AFTER THIS ORDER.
  right half, ONE finger, clean tap ....... the leap ladder ALONE
  right half, TWO fingers, clean tap ...... the ring + the swap
  left half, one finger ................... the stick
  left half, SECOND finger, clean tap ..... the aura
  SPACE ................................... the one-finger word
  CAPS_LOCK ............................... the two-finger word
The ring is the two-finger word's now, and the bus's. Nothing else
rings.

RULINGS TAKEN BY DEFAULT (Jean may flip any by a line):
  R1 REVERSES LEAP_0's R1 ("every tap rings, the ring is the tap's
     word"). The leap is silent. The two words had grown apart by
     LEAP_1 and the ring was the half only one of them wanted.
  R2 the ribbon routing STAYS on the one-finger word (LEAP_1 R6 —
     boarding is what the body does with itself). Unchanged, and so
     is the dismount defect that rides it: named in OPEN, not fixed
     here, because moving it is a different ruling.
  R3 `request_radial_pulse` KEEPS ITS NAME this round. Naming is
     Jean's gate. The banner is corrected to say what the door does;
     the rename is priced in the register, not taken.

────────────────────────────────────────────────────────────────────
U0 — PREFLIGHT AND THE ORDER LANDS
────────────────────────────────────────────────────────────────────
git fetch; on master; tip is 3d03c921 or a descendant. Count every
FIND in U1 (all 1). This file is already at docs/HANDOFFS/.

COMMIT: "PULSE_SPLIT_0 U0 — the order lands"

────────────────────────────────────────────────────────────────────
U1 — THE LEAP STOPS RINGING
────────────────────────────────────────────────────────────────────
FIND (direction/input.hpp):
// THE PULSE'S OWNER DOOR. Published here at its SECOND consumer (the
// standing law): the lone tap raised it, SPACE joined, and a thumb and a
// keyboard reach one implementation rather than two — the arrangement
// key 3 / CAPS_LOCK already keep with toggle_aura and try_possess_nearest.
//
// It RAISES AN INTENT rather than stamping the ring. The onset's origin
// is the point and its time is the frame's, and neither is an input
// door's to know; the frame spends it in phase_live_card_write, where
// both are in hand and where the rest law reads the ring immediately
// after — the drain idiom the analog deltas already use.
inline void request_radial_pulse(InputDeps* c) {
    c->inputState_.pulse_pending = true;
    c->inputState_.jump_pending = true;    // LEAP_0 — one word, two verbs: the ring, and the body's leap or somersault
REPLACE:
// THE LEAP'S OWNER DOOR (PULSE_SPLIT_0 — it was the pulse's). Published
// here at its SECOND consumer (the standing law): the lone tap raised it,
// SPACE joined, and a thumb and a keyboard reach one implementation rather
// than two — the arrangement key 3 / CAPS_LOCK already keep with
// toggle_aura and the swap.
//
// IT NO LONGER RINGS. LEAP_0's R1 held that every tap rings and the ring
// is the tap's word; LEAP_1 split the hand into two words and the ring was
// the half only one of them wanted. The ring belongs to the TWO-finger
// word now (request_pulse_swap) and to the bus (emit_radial_pulse). This
// door raises the body's verb and routes the ride, and nothing else.
//
// It RAISES AN INTENT rather than stamping anything: the onset's origin is
// the point and its time is the frame's, and neither is an input door's to
// know. The name is kept for this round — naming is Jean's gate, and the
// register prices the rename.
inline void request_radial_pulse(InputDeps* c) {
    c->inputState_.jump_pending = true;    // PULSE_SPLIT_0 — the leap alone; the ring left with the second word

FIND (contracts/spine_state.hpp):
    // PULSE_1 — THE TAP'S INTENT, not a delta: an EDGE, raised by the
    // glass tap and the SPACE key, spent exactly once by the frame
REPLACE:
    // PULSE_1 — THE TAP'S INTENT, not a delta: an EDGE, raised by the
    // right-half PAIR tap and CAPS_LOCK (PULSE_SPLIT_0 — it was the glass
    // tap and the SPACE key, until the leap stopped ringing), spent exactly once by the frame

FIND (contracts/spine_state.hpp):
    // LEAP_0 — THE SAME TAP'S SECOND INTENT. The door that rings the ground
    // (request_radial_pulse) raises this beside pulse_pending; the signal
REPLACE:
    // LEAP_0 — THE BODY'S OWN VERB, and since PULSE_SPLIT_0 the ONLY thing
    // the one-finger word (request_radial_pulse) raises: that door stopped
    // ringing the ground, so this intent travels alone. The signal

FIND (contracts/spine_state.hpp):
    // CAPS_LOCK) raises this and pulse_pending, never jump_pending: two
    // fingers ring the ground and reach for a body, one finger rings it and
    // leaps. Spent at the same drain pulse_pending is spent at, where the
REPLACE:
    // CAPS_LOCK) raises this and pulse_pending, never jump_pending: two
    // fingers ring the ground and reach for a body, one finger only leaps
    // (PULSE_SPLIT_0 took the ring off that word). Spent at the drain
    // pulse_pending is spent at, where the

FIND (direction/input.hpp):
    // REACH_2 — THE PULSE IS ALSO THE RIDE'S WORD, and the routing lives
    // HERE — the player's door, both mouths (SPACE, the lone tap) — so a
    // musical pulse riding the bus (emit_radial_pulse) can never board.
    // The wave fires regardless, above: on the ground it is the gesture
    // as ever; on a tall summit the same wave announces the boarding
    // it begins — the ribbon may be anywhere, the ease is the abduction;
    // in the sky it marks the departure. The summit check is a COURTESY
    // (no refusal spam on every ground pulse) — the LAW stays in
REPLACE:
    // REACH_2 — THE RIDE'S WORD IS THIS ONE, and the routing lives
    // HERE — the player's door, both mouths (SPACE, the lone RIGHT tap) —
    // so a musical pulse riding the bus (emit_radial_pulse) can never
    // board. PULSE_SPLIT_0 took the ring off this door, so a boarding no
    // longer arrives announced by a wave: on the ground the gesture is the
    // leap; on a tall summit the same press boards instead — the ribbon may
    // be anywhere, the ease is the abduction; in the sky it marks the
    // departure. The summit check is a COURTESY (no refusal spam on every
    // ground leap) — the LAW stays in

GATES: python3 tools/gates/console_gate/run.py (PASS);
       python3 tools/gates/glaw2/run.py (GREEN).
COMMIT: "PULSE_SPLIT_0 U1 — the leap stops ringing; the ring is the second word's"

────────────────────────────────────────────────────────────────────
U2 — THE LEDGERS FOLLOW
────────────────────────────────────────────────────────────────────
Added in flight: BINDING_LEDGER pins contracts/spine_state.hpp, whose
comments U1 edits, so two ledgers move. Run, in this order, from the
root:
  python3 tools/organ_ledger.py
  python3 tools/binding_ledger.py
  python3 tools/mirror_census.py
  python3 tools/command_census.py
Expected motion: BINDING_LEDGER's spine_state.hpp pin refreshes;
MIRROR_LEDGER's source commit and its BINDING_LEDGER pin follow. No
row count moves — this order adds no dial and no binding. Then every
--check row in CLAUDE.md's gate table, plus organ_gap --gate,
organ_readers, score, shell_gate, sha256_gate: all green.

COMMIT: "PULSE_SPLIT_0 U2 — the ledgers follow: one pin, one cascade"

────────────────────────────────────────────────────────────────────
U3 — THE REGISTER WRITTEN, THE ORDER RETIRED
────────────────────────────────────────────────────────────────────
Append to the LEAP_0 / LEAP_1 section's body in docs/OPEN.md, then
git rm docs/HANDOFFS/PULSE_SPLIT_0.md.

COMMIT: "PULSE_SPLIT_0 U3 — the register written, the order retired"
