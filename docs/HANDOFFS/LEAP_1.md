════════════════════════════════════════════════════════════════════
7T — LEAP_1 — ONE FINGER LEAPS, TWO FINGERS SWAP, BOTH RING
Five units on master. The hand splits into two words on the right
(U1); the somersaults stand taller and gain a third rung (U2); the
second word's ring carries the swap (U3); the ledgers follow (U4);
the register closes (U5).
════════════════════════════════════════════════════════════════════

GIT LAW: trunk-based, master, direct. One commit per unit, messages
verbatim below, no squash, no branch, no PR. Jean gates build,
visual, deploy.

REGISTER (P17 DEFAULT-AND-FLAG): verify every FIND verbatim and report
its count before the edit; every count below is 1. A mismatch STOPS
THE UNIT, not the round — report anchor + count and continue with the
next unit. HALT only for an unreachable home or a tip that is not
dcbd4a9-or-later on master. No improvisation, no opportunistic edits.
CC does not compile or run: the python gates named per unit are the
witnesses CC owns; glaw1 and the build are Jean's.

NOTE ON ONE ANCHOR: the `on_key_down` DECLARATION block in U1 opens
with a blank line inside the FIND. That leading newline is what
separates it from the DEFINITION, whose line begins `inline void`.
Reproduce it exactly or the count reads 2 and the unit stops.

REHEARSED at dcbd4a9 by the author: every count 1; naga PASS on the
edited module; TU gate PASS at zero diagnostics (g++ — the 752
witness); G-LAW 2 GREEN; the four ledgers regenerate and all eleven
--check / gate rows green after. sha256 at the tip, for the preflight:
  state.hpp    1fb3fb5f159f386dfd499242a2a3fabd93e44adfb62e1b403c2cdba76d86e568
  world.wgsl   6232961e37b97eff8c0774f6939c68a51cdec67eb378ed28c7a60b1046222ef4
  agents.hpp   66da45d7b85605049cb57803bd10773a29cbb7522a486436da8ea590f51c421b
  console.hpp  8aec4ad7cdfdf4b8bb3115cb4ff1dca7b7457fdd124a450b9d737c9398915cd6
  input.hpp    5bd1da8a5b8347acf380aff802c93d1deb3c51862322fb3ed1057673b1e8c7fa

THE HAND, AFTER THIS ORDER.
  right half, ONE finger, clean tap ....... the ring + the leap ladder
  right half, TWO fingers, clean tap ...... the ring + the swap it carries
  left half, one finger ................... the stick, and nothing else
  left half, SECOND finger, clean tap ..... the aura (untouched)
  SPACE ................................... the one-finger word
  CAPS_LOCK ............................... the two-finger word
Both words ring the ground: the ring is what the hand always says.
What differs is what the ring is given to carry — your own body's
verb, or a reach for someone else's.

WHY THE SPLIT IS A SPLIT AND NOT A TOGGLE. Today the leap rides the
LONE tap on EITHER half, so saying it means abandoning the stick;
possession rides the right-half PAIR tap, which already works
mid-stride because the right half is the look's half. U1 keeps each
gesture's mechanics exactly and re-aims the words: the lone tap gains
a side (`!t.left`), and the pair tap stops taking a body directly and
starts speaking. Neither classifier condition is touched — `alone`
and the pair window are the console's, unchanged.

RULINGS TAKEN BY DEFAULT (Jean may flip any by a line):
  R1 the leap is ONE finger, right half only. The lone LEFT tap goes
     silent — that half is the stick's entire.
  R2 the swap arms only from the GROUND. A body taken mid-flight
     would inherit an arc it never launched, and the air taps are the
     ladder's rungs.
  R3 the commit waits for wave-arrival AND the player's touchdown, so
     the wave's timing bends by a fraction rather than a body falling
     out of its owner.
  R4 CAPS_LOCK JOINS the two-finger word rather than keeping its own
     immediate swap. One word, two mouths — the tree's standing law
     (key 3 / the aura tap; SPACE / the pulse tap). The consequence:
     `try_possess_nearest` has no callers and is RETIRED, its two
     halves shipping as `find_possess_target` + `commit_possession`,
     and `on_key_down` sheds the two parameters that verb alone
     needed. If Jean wants an instant operator door back, the flip is
     one line at CAPS_LOCK — but it would be a second scheduler for
     one idea, which is why the default is this.
  R5 the ladder is 3.0 / 4.5 / 6.5 wu; each rung is one full turn.
     Measured at 60 Hz: peaks 2.8 / 7.1 / 13.4 wu, and every rung
     leaves more than LEAP_FLIP_SECONDS of air, so each lands upright.
  R6 the ribbon routing (summit boarding, dismount) stays on the
     ONE-finger word. Boarding is what the body does with itself.

────────────────────────────────────────────────────────────────────
U0 — PREFLIGHT AND THE ORDER LANDS
────────────────────────────────────────────────────────────────────
git fetch; on master; tip is dcbd4a9 or a descendant. Confirm the
five sha256 above, then count every FIND in U1–U3 (all 1). Copy this
file to docs/HANDOFFS/LEAP_1.md (create the directory).

COMMIT: "LEAP_1 U0 — the order lands"

────────────────────────────────────────────────────────────────────
U1 — THE HAND SPLITS (two words on the right)
────────────────────────────────────────────────────────────────────
Pixel-identical: `swap_pending` is shipped and read by nothing until
U3, and the two-finger tap's swap is momentarily inert. That is the
LEAP_0 U1 shape — the wire before the verb.

FIND (src/console/console.hpp):
            // PULSE — A PRESS THAT GOES NOWHERE, ALONE, ON EITHER HALF. One
            // word, two mouths: whichever thumb is free speaks, and the
            // player never learns which half of the glass owns it.
REPLACE:
            // THE LEAP — A PRESS THAT GOES NOWHERE, ALONE, ON THE RIGHT
            // (LEAP_1). It was either half, on the reasoning that whichever
            // thumb is free should speak. But the left thumb is free only
            // while the pawn stands still, so the word cost the walk: the
            // stick had to be abandoned to say it. The right half is the
            // look's half, where a thumb that lifts and taps interrupts
            // nothing, and the left half is the stick's entire. SPACE is
            // this door's other mouth.

FIND (src/console/console.hpp):
            if (clean && t.alone
                && (now_ms - lastPulseMs_) >= TouchControls::PULSE_DEBOUNCE_MS) {
REPLACE:
            if (clean && t.alone && !t.left
                && (now_ms - lastPulseMs_) >= TouchControls::PULSE_DEBOUNCE_MS) {

FIND (contracts/spine_state.hpp):
    bool  jump_pending = false;
};
REPLACE:
    bool  jump_pending = false;
    // LEAP_1 — THE HAND'S OTHER WORD. The right-half PAIR tap (and
    // CAPS_LOCK) raises this and pulse_pending, never jump_pending: two
    // fingers ring the ground and reach for a body, one finger rings it and
    // leaps. Spent at the same drain pulse_pending is spent at, where the
    // point and the population are both in hand — the arming is a question
    // about the world, which no input door may ask.
    bool  swap_pending = false;
};

FIND (direction/input.hpp):
void request_radial_pulse(InputDeps* c);   // the pulse's owner door (SPACE + the lone tap)
REPLACE:
void request_radial_pulse(InputDeps* c);   // the pulse's owner door (SPACE + the lone RIGHT tap): the ring and the body's own verb
void request_pulse_swap(InputDeps* c);     // LEAP_1 — the hand's other word (CAPS_LOCK + the right PAIR tap): the ring and the reach for a body

FIND (direction/input.hpp):
// RIGHT, two fingers, clean tap — possession (CAPS_LOCK's door).
inline void on_touch_tap_right(InputDeps* c, AgentState& agent_state, AgentsDeps& agents_deps) {
    wgpu::Queue q = c->device_.GetQueue();
    try_possess_nearest(agent_state, &agents_deps, q);
}
REPLACE:
// RIGHT, two fingers, clean tap — THE OTHER WORD (LEAP_1, CAPS_LOCK's
// door still). It was the immediate possession; it rings the ground now
// and lets the ring's own wavefront carry the swap. The organs it used to
// take are the drain's to reach, so the parameters go with the verb.
inline void on_touch_tap_right(InputDeps* c) {
    request_pulse_swap(c);
}

FIND (direction/input.hpp):
// EITHER HALF, one finger, clean tap — the pulse (SPACE's twin mouth).
inline void on_touch_tap_pulse(InputDeps* c) {
    request_radial_pulse(c);
}
REPLACE:
// LEAP_1 — THE OTHER WORD'S OWNER DOOR (CAPS_LOCK + the right pair tap).
// It raises the ring and the REACH, never the leap: the swap is two
// fingers' word and the leap is one's. Both intents are spent at the same
// drain, so the ring and the arming share a frame and the wave the viewer
// sees is the wave the swap is timed to.
//
// The ribbon routing is deliberately NOT here — boarding and dismounting
// are what the body does with itself, which is the leap's door above.
inline void request_pulse_swap(InputDeps* c) {
    c->inputState_.pulse_pending = true;
    c->inputState_.swap_pending  = true;
}

// RIGHT HALF, one finger, clean tap — the leap (SPACE's twin mouth).
inline void on_touch_tap_pulse(InputDeps* c) {
    request_radial_pulse(c);
}

FIND (direction/input.hpp):
    case GLFW_KEY_CAPS_LOCK:  try_possess_nearest(agent_state, &agents_deps, q);  break;
REPLACE:
    case GLFW_KEY_CAPS_LOCK:  request_pulse_swap(c);                             break;

FIND (direction/input.hpp):
void on_touch_tap_right(InputDeps* c, AgentState& agent_state, AgentsDeps& agents_deps);
REPLACE:
void on_touch_tap_right(InputDeps* c);

FIND (direction/input.hpp):

void on_key_down(InputDeps* c, int key,
    PawnState& pawn_state, PawnDeps& pawn_deps,
    OrbsState& orbs_state, OrbsDeps& orbs_deps,
    AgentState& agent_state, AgentsDeps& agents_deps,
REPLACE:

void on_key_down(InputDeps* c, int key,
    PawnState& pawn_state, PawnDeps& pawn_deps,
    OrbsState& orbs_state, OrbsDeps& orbs_deps,

FIND (direction/input.hpp):
inline void on_key_down(InputDeps* c, int key,
    PawnState& pawn_state, PawnDeps& pawn_deps,
    OrbsState& orbs_state, OrbsDeps& orbs_deps,
    AgentState& agent_state, AgentsDeps& agents_deps,
REPLACE:
inline void on_key_down(InputDeps* c, int key,
    PawnState& pawn_state, PawnDeps& pawn_deps,
    OrbsState& orbs_state, OrbsDeps& orbs_deps,

FIND (cartridge.hpp):
                    on_key_down(&input_deps_, event.key,
                        pawn_state_, pawn_deps_,
                        orbs_state_, orbs_deps_,
                        agent_state_, agents_deps_,
REPLACE:
                    on_key_down(&input_deps_, event.key,
                        pawn_state_, pawn_deps_,
                        orbs_state_, orbs_deps_,

FIND (cartridge.hpp):
                    on_touch_tap_right(&input_deps_, agent_state_, agents_deps_);
REPLACE:
                    on_touch_tap_right(&input_deps_);

GATES: python3 tools/gates/console_gate/run.py (PASS — it proves the
       shed parameters and the retired verb agree across three TUs);
       python3 tools/gates/glaw2/run.py (GREEN).
COMMIT: "LEAP_1 U1 — one finger leaps, two fingers speak; the left half is the stick's"

────────────────────────────────────────────────────────────────────
U2 — THE TALLER LADDER (pixel-affecting)
────────────────────────────────────────────────────────────────────
The clock's three states, both mirrors; the config grows 736 -> 752
under the GROWTH LAW (pulse_speed is declared here and read from U3 —
one growth, not two, the LEAP_0 precedent); the rests, the pins, the
rows; the band; the third door; the tumble's two clocks.

FIND (state.hpp):
            float t;               // 12 — the possessed slot's AIR CLOCK (LEAP_0): 0 on the
                                   //      ground; +seconds since the leap; −seconds since the
                                   //      somersault (spent). Zero on every other slot.
REPLACE:
            float t;               // 12 — the possessed slot's AIR CLOCK (LEAP_0/1): 0 on the
                                   //      ground; +seconds since the leap; (−BAND, 0) = seconds
                                   //      since the first somersault; ≤ −BAND = the second's,
                                   //      offset by LEAP_FLIP_BAND (64) — both spent. Zero on
                                   //      every other slot.

FIND (world.wgsl):
    t: f32,         // the possessed slot's AIR CLOCK (LEAP_0): 0 on the ground;
                    // +seconds since the leap; −seconds since the somersault (spent).
                    // Zero on every other slot. Mirrors GPUAgentState.t.
REPLACE:
    t: f32,         // the possessed slot's AIR CLOCK (LEAP_0/1): 0 on the ground;
                    // +seconds since the leap; (−BAND, 0) = the first somersault's
                    // seconds; ≤ −BAND = the second's, offset by LEAP_FLIP_BAND —
                    // both spent. Zero on every other slot. Mirrors GPUAgentState.t.

FIND (state.hpp):
            float leap_flip_apex;          // 728  wu — the somersault's own apex over where it fired
            float _pad736_0;               // 732
        };
REPLACE:
            float leap_flip_apex;          // 728  wu — the somersault's own apex over where it fired
            // LEAP_1 — the second somersault's apex and the ring's speed.
            // Mirror of world.wgsl's config (GROWTH LAW: same commit, same
            // order, same types). pulse_speed is PULSE_SPEED graduated: the
            // ring's expansion was a WGSL const and the swap's delay needs
            // the same number CPU-side — one home ends the twin. Rests:
            // contracts/control_panel.hpp LEAP_FLIP2_APEX / PULSE_RING_SPEED.
            // One pad consumed IN PLACE, one appended, three fresh pads to
            // the boundary: 736 -> 752. Was _pad736_0.
            float leap_flip2_apex;         // 732  wu — the third tap's taller somersault
            float pulse_speed;             // 736  wu/s — the ring's expansion; the swap's clock
            float _pad752_0;               // 740
            float _pad752_1;               // 744
            float _pad752_2;               // 748
        };

FIND (world.wgsl):
    leap_flip_apex: f32,            // 728
    _pad736_0: f32,                 // 732
}
REPLACE:
    leap_flip_apex: f32,            // 728
    // LEAP_1 — the second somersault's apex and the ring's speed (PULSE_SPEED
    // graduated — the contributor reads it here now). Mirror of
    // GPUDesignConfig (state.hpp) — GROWTH LAW, same commit, same order, same
    // types. One pad consumed in place, one appended, three fresh pads:
    // 736 -> 752 (state.hpp carries the witness). Was _pad736_0.
    leap_flip2_apex: f32,           // 732
    pulse_speed: f32,               // 736
    _pad752_0: f32,                 // 740
    _pad752_1: f32,                 // 744
    _pad752_2: f32,                 // 748
}

FIND (state.hpp):
        static_assert(sizeof(GPUDesignConfig) == 736,
            "GPUDesignConfig must be 736 bytes. PRUNING_1 P3 removed nine "
REPLACE:
        static_assert(sizeof(GPUDesignConfig) == 752,
            "GPUDesignConfig must be 752 bytes. PRUNING_1 P3 removed nine "

FIND (state.hpp):
            "LEAP_0: the leap's four dials — one pad consumed IN PLACE, three appended, one fresh pad to the boundary; 720 -> 736.)");
REPLACE:
            "LEAP_0: the leap's four dials — one pad consumed IN PLACE, three appended, one fresh pad to the boundary; 720 -> 736. "
            "LEAP_1: the second somersault's apex and the graduated ring speed — one pad consumed IN PLACE, one appended, three fresh pads; 736 -> 752.)");

FIND (contracts/control_panel.hpp):
inline constexpr float LEAP_FLIP_APEX  = 1.2f;   // wu — the somersault's own apex over where it fired
REPLACE:
inline constexpr float LEAP_FLIP_APEX  = 4.5f;   // wu — the somersault's own apex over where it fired; LEAP_1 raised 1.2 -> 4.5 (the flip stands taller than the leap)
inline constexpr float LEAP_FLIP2_APEX = 6.5f;   // wu — the third tap's somersault, taller still (LEAP_1)
// THE RING'S SPEED, graduated out of world.wgsl (was const PULSE_SPEED —
// LEAP_1): the swap's delay divides by it CPU-side and the contributor
// multiplies by it GPU-side, and two rooms holding one number is the twin
// the possession radius already retired. The organ's CONFIG row edits the
// live copy; at this rest and POSSESSION_RADIUS the wave reaches the
// farthest body it can take in 0.67 s.
inline constexpr float PULSE_RING_SPEED = 30.0f; // wu/s — ring expansion

FIND (state.hpp):
                config_.leap_flip_apex         = LEAP_FLIP_APEX;
REPLACE:
                config_.leap_flip_apex         = LEAP_FLIP_APEX;
                // LEAP_1 — the same road: the second flip and the ring's speed.
                config_.leap_flip2_apex        = LEAP_FLIP2_APEX;
                config_.pulse_speed            = PULSE_RING_SPEED;

FIND (src/console/organ_params.inc):
ORGAN_PARAM(CONFIG, GPUDesignConfig, leap_flip_apex,      F32, 0.0f,  6.0f,  0.1f,   "Interaction · Pawn",   "somersault apex")
REPLACE:
ORGAN_PARAM(CONFIG, GPUDesignConfig, leap_flip_apex,      F32, 0.0f,  12.0f, 0.1f,   "Interaction · Pawn",   "somersault apex")
ORGAN_PARAM(CONFIG, GPUDesignConfig, leap_flip2_apex,     F32, 0.0f,  12.0f, 0.1f,   "Interaction · Pawn",   "somersault 2 apex")

FIND (world.wgsl):
const LEAP_FLIP_SECONDS: f32 = 0.45;
REPLACE:
const LEAP_FLIP_SECONDS: f32 = 0.45;
// THE SECOND TUMBLE'S BAND (LEAP_1). One float carries three air states:
// t in (0, inf) = leaping, no flip spent; t in (-BAND, 0) = the first
// tumble's clock; t <= -BAND = the second's, offset by the band — both
// spent. 64 is exact in f32 and no flight approaches it. Shape, not a dial.
const LEAP_FLIP_BAND: f32 = 64.0;

FIND (world.wgsl):
        } else if (door && agent.t > 0.0) {                 // THE SOMERSAULT — once, from the air
            agent.vel_y = sqrt(2.0 * g_rise * max(config.leap_flip_apex, 0.0));
            agent.t = -dt;                                  // spent; the tumble's clock starts
        }
REPLACE:
        } else if (door && agent.t > 0.0) {                 // THE SOMERSAULT — from the air
            agent.vel_y = sqrt(2.0 * g_rise * max(config.leap_flip_apex, 0.0));
            agent.t = -dt;                                  // the first tumble's clock starts
        } else if (door && agent.t < 0.0 && agent.t > -LEAP_FLIP_BAND) {
            // THE THIRD TAP (LEAP_1) — taller still, once. The band marks
            // both flips spent; touchdown clears it.
            agent.vel_y = sqrt(2.0 * g_rise * max(config.leap_flip2_apex, 0.0));
            agent.t = -(LEAP_FLIP_BAND + dt);
        }

FIND (world.wgsl):
            let turn = 6.2831853 * saturate(-agent.t / LEAP_FLIP_SECONDS);
REPLACE:
            let tumble = select(-agent.t, -agent.t - LEAP_FLIP_BAND, agent.t <= -LEAP_FLIP_BAND);
            let turn = 6.2831853 * saturate(tumble / LEAP_FLIP_SECONDS);

GATES: python3 tools/gates/glaw2/run.py (GREEN);
       python3 tools/wgsl_gate.py (PASS — an absent naga is a FAILED
       gate, report it, never a pass);
       python3 tools/gates/console_gate/run.py (PASS — the 752 witness).
COMMIT: "LEAP_1 U2 — the flips stand taller than the leap, and a third tap stands taller still"

────────────────────────────────────────────────────────────────────
U3 — THE WAVE CARRIES THE SWAP
────────────────────────────────────────────────────────────────────
PULSE_SPEED graduates (the WGSL const dies; config.pulse_speed is the
one home); try_possess_nearest is retired into its two halves, and the
transaction grounds both bodies; the drain arms on the second word and
the same phase commits when the wavefront arrives; teardown forgets.

The last edit is the READER PROOF's own table: `possession.radius`
named `try_possess_nearest` as its reader, and that function is gone.
Repointing it at `find_possess_target` is part of this edit, not
housekeeping — the gate reds without it, which is the gate working.

FIND (world.wgsl):
const PULSE_SPEED: f32 = 30.0;         // world units per second (ring expansion rate)
REPLACE:
// PULSE_SPEED graduated to config.pulse_speed (LEAP_1): the swap's delay
// divides by the same number this contributor multiplies by, so the ring's
// speed has one home. Rest: contracts/control_panel.hpp PULSE_RING_SPEED.

FIND (world.wgsl):
        let wavefront_r = age * PULSE_SPEED;
REPLACE:
        let wavefront_r = age * config.pulse_speed;

FIND (bodies/agents.hpp):
inline void try_possess_nearest(AgentState& as, AgentsDeps* c, wgpu::Queue& queue) {
    if (c->transitionPhase_ != TransitionPhase::IDLE) {
        std::cout << "[Possess] Blocked (mid-transition)\n";
        return;
    }

    const uint32_t cur = c->player_.possessed_slot;
    // THE POINT: possession reaches from the point — the
    // nearest agent to where you ARE. Pawn-host value-identical (same
    // harvest snapshot as the slot mirror); in free-fly Caps Lock
    // grabs a body wherever you flew (xz-plane reach, per the spawn
    // ruling — the population lives there now, so there is one).
    const float px = c->point_.x;
    const float pz = c->point_.z;

    int best_slot = -1;
    // ORGAN_4 P3b — THE SQUARE IS DERIVED HERE, from the live reach. The
    // retired POSSESSION_RADIUS_SQ was a constexpr twin, and a dialled
    // radius against a frozen square is the disagreement its DEFER row
    // named. One authority, squared where it is used.
    const float reach = PANEL_LIVE.possession.radius;
    float best_d2 = reach * reach;
    for (uint32_t s = 0; s < Dim::MAX_AGENTS; s++) {
        if (s == cur) continue;
        const auto& a = as.slots[s];
        if (a.is_active == 0u) continue;
        if (a.behavior_id == AGENT_BEHAVIOR_PLAYER_CONTROLLED) continue;

        float dx = a.pos_x - px;
        float dz = a.pos_z - pz;
        float d2 = dx * dx + dz * dz;
        if (d2 < best_d2) {
            best_d2 = d2;
            best_slot = (int)s;
        }
    }

    if (best_slot < 0) {
        std::cout << "[Possess] No agent within " << reach
                  << " units of the point at (" << px << "," << pz << ")\n";
        return;
    }

    const uint32_t new_slot = (uint32_t)best_slot;

    as.slots[cur].behavior_id = AGENT_BEHAVIOR_RANDOM_WALK;
    if (as.slots[cur].seed == 0u) {
        as.slots[cur].seed = cpu_hash(c->world_state_.active_seed, cur ^ 0xC11Cu);
    }

    // New slot → player control. Reset velocity + portal trigger so the
    // player's first frame on the new body is clean.
    as.slots[new_slot].behavior_id    = AGENT_BEHAVIOR_PLAYER_CONTROLLED;
    as.slots[new_slot].vel_x          = 0.0f;
    as.slots[new_slot].vel_z          = 0.0f;
    as.slots[new_slot].portal_trigger = -1;

    c->gpuState_.upload_agent_slot(queue, cur, &as.slots[cur]);
    c->gpuState_.upload_agent_slot(queue, new_slot, &as.slots[new_slot]);

    c->player_.possessed_slot = new_slot;
    c->gpuState_.set_possessed_slot(new_slot);

    std::cout << "[Possess] " << cur << " -> " << new_slot
              << " (tier " << as.slots[new_slot].tier_idx
              << ", dist " << std::sqrt(best_d2) << ")\n";
}
REPLACE:
// THE SEARCH, ALONE (LEAP_1). The point reaches for the nearest active
// non-player body within PANEL_LIVE.possession.radius — the loop
// try_possess_nearest always ran, extracted so the pulse drain can ask
// the question a frame before it takes the body.  Returns the slot, or -1.
//
// THE POINT: possession reaches from the point — the nearest agent to
// where you ARE. Pawn-host value-identical (same harvest snapshot as the
// slot mirror); in free-fly the reach grabs a body wherever you flew
// (xz-plane, per the spawn ruling — the population lives there).
// ORGAN_4 P3b — THE SQUARE IS DERIVED HERE, from the live reach. The
// retired POSSESSION_RADIUS_SQ was a constexpr twin, and a dialled radius
// against a frozen square is the disagreement its DEFER row named. One
// authority, squared where it is used.
inline int find_possess_target(const AgentState& as, const AgentsDeps* c) {
    const uint32_t cur = c->player_.possessed_slot;
    const float px = c->point_.x;
    const float pz = c->point_.z;
    int best_slot = -1;
    const float reach = PANEL_LIVE.possession.radius;
    float best_d2 = reach * reach;
    for (uint32_t s = 0; s < Dim::MAX_AGENTS; s++) {
        if (s == cur) continue;
        const auto& a = as.slots[s];
        if (a.is_active == 0u) continue;
        if (a.behavior_id == AGENT_BEHAVIOR_PLAYER_CONTROLLED) continue;

        float dx = a.pos_x - px;
        float dz = a.pos_z - pz;
        float d2 = dx * dx + dz * dz;
        if (d2 < best_d2) {
            best_d2 = d2;
            best_slot = (int)s;
        }
    }
    return best_slot;
}

// THE TRANSACTION (LEAP_1) — everything try_possess_nearest did past its
// search, with the timing taken out of it: the caller decides WHEN, this
// decides WHAT. The one commit door, and there is no other.
//
// LEAP_1 adds the grounding: both bodies' air clocks and vertical
// velocities are zeroed in the mirror before the upload — the slot handed
// back to the autonomous kernel carries no stale flight, and the slot the
// player takes starts on the ground's law.
inline void commit_possession(AgentState& as, AgentsDeps* c, wgpu::Queue& queue, uint32_t new_slot) {
    const uint32_t cur = c->player_.possessed_slot;

    as.slots[cur].behavior_id = AGENT_BEHAVIOR_RANDOM_WALK;
    as.slots[cur].t     = 0.0f;   // LEAP_1 — the freed body is grounded by the handover
    as.slots[cur].vel_y = 0.0f;
    if (as.slots[cur].seed == 0u) {
        as.slots[cur].seed = cpu_hash(c->world_state_.active_seed, cur ^ 0xC11Cu);
    }

    // New slot -> player control. Reset velocity + portal trigger so the
    // player's first frame on the new body is clean.
    as.slots[new_slot].behavior_id    = AGENT_BEHAVIOR_PLAYER_CONTROLLED;
    as.slots[new_slot].vel_x          = 0.0f;
    as.slots[new_slot].vel_y          = 0.0f;   // LEAP_1 — and the taken body starts grounded
    as.slots[new_slot].vel_z          = 0.0f;
    as.slots[new_slot].t              = 0.0f;
    as.slots[new_slot].portal_trigger = -1;

    c->gpuState_.upload_agent_slot(queue, cur, &as.slots[cur]);
    c->gpuState_.upload_agent_slot(queue, new_slot, &as.slots[new_slot]);

    c->player_.possessed_slot = new_slot;
    c->gpuState_.set_possessed_slot(new_slot);

    const float ddx = as.slots[new_slot].pos_x - c->point_.x;
    const float ddz = as.slots[new_slot].pos_z - c->point_.z;
    std::cout << "[Possess] " << cur << " -> " << new_slot
              << " (tier " << as.slots[new_slot].tier_idx
              << ", dist " << std::sqrt(ddx * ddx + ddz * ddz) << ")\n";
}

FIND (bodies/agents.hpp):
void try_possess_nearest(AgentState& as, AgentsDeps* c, wgpu::Queue& queue);
REPLACE:
int  find_possess_target(const AgentState& as, const AgentsDeps* c);              // LEAP_1 — the search alone
void commit_possession(AgentState& as, AgentsDeps* c, wgpu::Queue& queue, uint32_t new_slot);   // LEAP_1 — the transaction alone

FIND (src/console/organ_params.inc):
ORGAN_PARAM(PANEL, PanelSurface, pulse.amplitude, F32, 0.0f, 4.0f, 0.05f, "Interaction · Pulse", "tap impulse")
REPLACE:
ORGAN_PARAM(PANEL, PanelSurface, pulse.amplitude, F32, 0.0f, 4.0f, 0.05f, "Interaction · Pulse", "tap impulse")
// LEAP_1 — the ring speed, graduated out of world.wgsl (was PULSE_SPEED).
// The swap's delay divides by it, so the floor is 1, not 0. Slower rings
// make the swap's cause legible from across a room.
ORGAN_PARAM(CONFIG, GPUDesignConfig, pulse_speed, F32, 1.0f, 120.0f, 0.5f, "Interaction · Pulse", "ring speed")

FIND (cartridge.hpp):
            float dtPending_ = 0.0f;
REPLACE:
            float dtPending_ = 0.0f;
            // LEAP_1 — THE SWAP THE WAVE CARRIES. Armed at the drain when
            // the other word is spoken and a body is in reach; committed in
            // the same phase once the ring's radius has crossed that body's
            // distance AND the player is grounded, so no body is abandoned
            // mid-flight. -1 = nothing pending. Teardown clears it beside
            // the portal trigger.
            int    pendingSwapSlot_ = -1;
            double pendingSwapAt_   = 0.0;

FIND (cartridge.hpp):
                if (inputState_.pulse_pending) {
                    inputState_.pulse_pending = false;
                    issue_pulse_from_point();
                }
                retire_aged_pulses();
REPLACE:
                if (inputState_.pulse_pending) {
                    inputState_.pulse_pending = false;
                    issue_pulse_from_point();
                }
                // LEAP_1 — THE OTHER WORD, SPENT WHERE THE FIRST ONE IS. Both
                // fingers rang the ground above; here the ring is given
                // something to carry. Armed only from the GROUND: the swap is
                // a thing you do standing, and a body taken mid-flight would
                // inherit an arc it never launched. P6 — every arm and every
                // refusal speaks, and the refusals are the ones
                // try_possess_nearest used to print.
                if (inputState_.swap_pending) {
                    inputState_.swap_pending = false;
                    if (transitionPhase_ != TransitionPhase::IDLE) {
                        std::cout << "[Swap] blocked (mid-transition)\n";
                    } else if (agent_state_.slots[player_.possessed_slot].t != 0.0f) {
                        std::cout << "[Swap] blocked (aloft)\n";
                    } else {
                        const int s = find_possess_target(agent_state_, &agents_deps_);
                        if (s < 0) {
                            std::cout << "[Swap] no body within "
                                      << PANEL_LIVE.possession.radius << " of the point\n";
                        } else {
                            const float dx = agent_state_.slots[s].pos_x - point_.x;
                            const float dz = agent_state_.slots[s].pos_z - point_.z;
                            const float dist = std::sqrt(dx * dx + dz * dz);
                            const float cfg_speed = gpuState_.config().pulse_speed;
                            const float speed = cfg_speed > 1e-3f ? cfg_speed : 1e-3f;
                            pendingSwapSlot_ = s;
                            pendingSwapAt_   = time_state_.seconds + dist / speed;
                            std::cout << "[Swap] armed slot=" << s << " dist=" << dist
                                      << " delay=" << (dist / speed) << "s\n";
                        }
                    }
                }
                // THE WAVE ARRIVES. A transition drops the promise (the world
                // it named is leaving); a target that died or was taken drops
                // it too; and the touchdown conjunct holds the commit until
                // the player has landed — the wave's timing bends by a
                // fraction rather than a body falling out of its owner.
                if (pendingSwapSlot_ >= 0) {
                    if (transitionPhase_ != TransitionPhase::IDLE) {
                        pendingSwapSlot_ = -1;
                    } else if (time_state_.seconds >= pendingSwapAt_) {
                        const auto& tgt = agent_state_.slots[pendingSwapSlot_];
                        if (tgt.is_active == 0u
                            || tgt.behavior_id == AGENT_BEHAVIOR_PLAYER_CONTROLLED) {
                            pendingSwapSlot_ = -1;
                        } else if (agent_state_.slots[player_.possessed_slot].t == 0.0f) {
                            std::cout << "[Swap] the wave arrives\n";
                            commit_possession(agent_state_, &agents_deps_, c.queue,
                                              (uint32_t)pendingSwapSlot_);
                            pendingSwapSlot_ = -1;
                        }
                    }
                }
                retire_aged_pulses();

FIND (cartridge.hpp):
                        point_.portal_trigger = -1;
REPLACE:
                        point_.portal_trigger = -1;
                        pendingSwapSlot_ = -1;          // LEAP_1 — the wave's promise dies with the world

FIND (tools/organ_readers.py):
        ("src/cartridges/the_board/bodies/agents.hpp", "try_possess_nearest"),
REPLACE:
        ("src/cartridges/the_board/bodies/agents.hpp", "find_possess_target"),

GATES: glaw2 (GREEN), wgsl_gate (PASS), console_gate (PASS),
       python3 tools/organ_readers.py (PASS — 301 proved, 0 suspects;
       red here means the table edit above was skipped).
COMMIT: "LEAP_1 U3 — the second word reaches for a body; the swap lands when the wave does"

────────────────────────────────────────────────────────────────────
U4 — THE LEDGERS FOLLOW
────────────────────────────────────────────────────────────────────
Run, in this order, from the root:
  python3 tools/organ_ledger.py
  python3 tools/binding_ledger.py
  python3 tools/mirror_census.py
  python3 tools/command_census.py
Expected motion: ORGAN.md gains "somersault 2 apex" and "ring speed"
(entries 407 -> 409) and the somersault apex max reads 12;
BINDING_LEDGER's DesignConfig size 736 -> 752 on its seats and every
pin of the edited files refreshes; MIRROR_LEDGER's and
COMMAND_LEDGER's pins refresh. Then every --check row in CLAUDE.md's
gate table plus organ_gap --gate, organ_readers, score, shell_gate,
sha256_gate: all green. binding_gen --check reds ONLY on a shallow
clone (S-6) — report which you saw.
COMMIT: "LEAP_1 U4 — the ledgers follow: 736 -> 752, two rows, one max raised"

────────────────────────────────────────────────────────────────────
U5 — THE REGISTER WRITTEN, THE ORDER RETIRED
────────────────────────────────────────────────────────────────────
In docs/OPEN.md, retitle the LEAP_0 section head to:

## LEAP_0 / LEAP_1 — THE PAWN LEAPS, TUMBLES, AND PASSES ITSELF ALONG (landed; Jean's visual gate open)

and append to that section's body:

**LEAP_1 — the hand has two words, and both ring.** One finger on the
right half (SPACE) is the body's own verb: the leap, then the flip,
then the taller flip — 3.0 / 4.5 / 6.5 wu, one clock, band 64 — and on
a summit, the boarding. Two fingers on the right half (CAPS_LOCK) is
the reach: the ring goes out and the swap commits when the wavefront
crosses the target, `dist / config.pulse_speed` (PULSE_SPEED
graduated, 0.67 s at the reach's edge). The left half is the stick's
entire. `try_possess_nearest` retired into `find_possess_target` +
`commit_possession`; `on_key_down` shed two parameters with it. Both
bodies are grounded by the handover. Config 736 -> 752.

**Jean's readings, all against the screen:** drive with the left thumb
and tap the right half with a second finger — the leap arrives without
the walk stopping; the lone left tap does nothing; the three rungs
read clearly taller each; two right fingers near a pawn and the swap
lands ON the ring's edge, not before it; leap first, then swap-tap,
and the swap waits for your feet; a mood change mid-wave cancels the
promise in silence.

**Priced, not built:** a double turn on the third rung (a value in the
tumble line); the swap's own voice on the bus; arming from the air
(R2 holds it to the ground); a visible mark on the armed body; an
instant operator swap (R4 retired it — one line to restore).

Then: git rm docs/HANDOFFS/LEAP_1.md (the directory dies with it).
COMMIT: "LEAP_1 U5 — the register written, the order retired"

────────────────────────────────────────────────────────────────────
JEAN-SIDE, AFTER U5
────────────────────────────────────────────────────────────────────
glaw1; the-board-web build; boot with ?organ=1.
PHONE — left thumb drives; a second finger taps the right half: ring
and leap, mid-stride. Tap it three times: leap, flip, taller flip,
each a full turn, landing upright. Tap the left half alone: nothing.
Two fingers on the right half near another pawn: the ring leaves, and
at the instant its edge reaches that pawn you are that pawn — the
console reads "[Swap] armed … delay=0.4s" then "[Swap] the wave
arrives".
DESKTOP — SPACE is the one-finger word, CAPS_LOCK the two-finger one.
DIALS — Interaction · Pawn (three apexes), Interaction · Pulse (ring
speed; slow it to make the swap's cause legible across a gallery).
