════════════════════════════════════════════════════════════════════
7T — LEAP_0 — THE PAWN LEAPS, AND ONCE PER FLIGHT, TUMBLES
Six units on master. The tap's edge reaches the GPU (U1, identical);
the body gets a second law (U2); a second verb (U3); the ledgers
follow (U4); the register closes (U5).
════════════════════════════════════════════════════════════════════

GIT LAW: trunk-based, master, direct. One commit per unit, messages
verbatim below, no squash, no branch, no PR. Jean gates build,
visual, deploy.

REGISTER (P17 DEFAULT-AND-FLAG): verify every FIND verbatim and report
its count before the edit; every count below is 1. A mismatch STOPS
THE UNIT, not the round — report anchor + count and continue with the
next unit. HALT only for an unreachable home or a tip that is not
8b4a026-or-later on master. No improvisation, no opportunistic edits.
CC does not compile or run: the python gates named per unit are the
witnesses CC owns; glaw1 and the build are Jean's.

REHEARSED at 8b4a026 by the author: counts, naga, TU gate (g++), G-LAW
2, ledger regeneration — all green. sha256 of the four pinned files at
the tip, for the preflight (they are BINDING_LEDGER's own pins):
  state.hpp        ab48016e61c44b54c80010f28615ee7ae72e3d5a5974c1cb89f930916f592e39
  world.wgsl       091ffbb4c386645caeb674787e689e5dac227f0656e78849ec2a414d9cd96a9b
  cartridge.hpp    9ba517d13cc49fb7281e251d95c96db789c136c915889d9b035e9b41fd29e088
  spine_state.hpp  685f803bb3a2e7cd63d9644f9948341e067900080d5d1ecc4c9efa287a0664b9

THE DESIGN, IN FOUR SENTENCES. Today the pawn's Y is a lookup —
behavior_player_controlled snaps it to the walker policy every frame,
and the slope law is the only wall. agent.t (reserved, unwritten on
every slot) becomes the possessed slot's AIR CLOCK: 0 on the ground,
+seconds since the leap, −seconds since the somersault. Aloft, Y
integrates under two gravities (rise, fall) and the wall is a LIP
instead of a grade; the touchdown hands Y back. The door is the
pulse's (SPACE, the lone tap): the ring fires as ever, and the body
leaps from the ground or tumbles from the air — which is the GPU's
to know, not the door's.

RULINGS TAKEN BY DEFAULT (Jean may flip any by a line):
  R1 every tap rings, the somersault's included — no CPU airborne
     sensor, no gate at the drain. The ring is the tap's word.
  R2 the ladder: any riser under the apex is a step now (GoL
     plateaus, solids). Pyramids are linear tapers; the summit route
     is unchanged. The lip law prevents tunnelling.
  R3 the somersault is in (U3). No wall kick (no walls), no ground
     pound (the pulse is flat by ruling), no triple jump.
  R4 the summit sensor keeps its premise ("xz inside the apex disc IS
     standing at the top"); a tap aloft over the apex boards from the
     air, and the mount ease absorbs the height. No comment amended.

────────────────────────────────────────────────────────────────────
U0 — PREFLIGHT AND THE ORDER LANDS
────────────────────────────────────────────────────────────────────
git fetch; on master; tip is 8b4a026 or a descendant. Confirm the
four sha256 above, then count every FIND in U1–U3 (all 1). Copy this
file to docs/HANDOFFS/LEAP_0.md (create the directory).

COMMIT: "LEAP_0 U0 — the order lands"

────────────────────────────────────────────────────────────────────
U1 — THE WIRE (pixel-identical: the edge is shipped and read by nothing)
────────────────────────────────────────────────────────────────────
FIND (state.hpp):
            uint32_t _mp0;                // 60
REPLACE:
            uint32_t jump_edge;           // 60  LEAP_0 — 1 on a frame the leap door fired (request_radial_pulse); the GPU decides leap or somersault by the body's state. Was _mp0.

FIND (world.wgsl):
    _mp0: u32,                 // 60
REPLACE:
    jump_edge: u32,            // 60  LEAP_0 — the leap door's edge; read by behavior_player_controlled. Was _mp0.

FIND (contracts/spine_state.hpp):
    bool  pulse_pending = false;
};
REPLACE:
    bool  pulse_pending = false;
    // LEAP_0 — THE SAME TAP'S SECOND INTENT. The door that rings the ground
    // (request_radial_pulse) raises this beside pulse_pending; the signal
    // fill ships it as FrameSignal.jump_edge and frame_submitted() lowers
    // it — the dtPending_ idiom, because an edge written on an update the
    // GPU never consumed would otherwise be overwritten and lost. Which verb
    // the body performs (leap from the ground, somersault from the air) is
    // the GPU's word: the CPU does not know where the body is.
    bool  jump_pending = false;
};

FIND (direction/input.hpp):
inline void request_radial_pulse(InputDeps* c) {
    c->inputState_.pulse_pending = true;
REPLACE:
inline void request_radial_pulse(InputDeps* c) {
    c->inputState_.pulse_pending = true;
    c->inputState_.jump_pending = true;    // LEAP_0 — one word, two verbs: the ring, and the body's leap or somersault

FIND (cartridge.hpp):
                gpuSignal.mount_from_heading = mount_.from_heading;
REPLACE:
                gpuSignal.mount_from_heading = mount_.from_heading;
                // LEAP_0 — the leap door's edge rides the pad the mount block
                // left. It stays raised until frame_submitted() so a dropped
                // acquire cannot delete it (the dtPending_ idiom above).
                gpuSignal.jump_edge          = inputState_.jump_pending ? 1u : 0u;

FIND (cartridge.hpp):
            void frame_submitted() { dtPending_ = 0.0f; }
REPLACE:
            void frame_submitted() { dtPending_ = 0.0f; inputState_.jump_pending = false; }

GATES: python3 tools/gates/console_gate/run.py (PASS);
       python3 tools/gates/glaw2/run.py (GREEN).
COMMIT: "LEAP_0 U1 — the door's second word: a tap's edge reaches the GPU"

────────────────────────────────────────────────────────────────────
U2 — THE LEAP (pixel-affecting from here)
────────────────────────────────────────────────────────────────────
The air clock, both mirrors.

FIND (state.hpp):
            float t;               // 12 — personal clock
REPLACE:
            float t;               // 12 — the possessed slot's AIR CLOCK (LEAP_0): 0 on the
                                   //      ground; +seconds since the leap; −seconds since the
                                   //      somersault (spent). Zero on every other slot.

FIND (world.wgsl):
    t: f32,         // reserved (per-agent local clock; padding to vec4)
REPLACE:
    t: f32,         // the possessed slot's AIR CLOCK (LEAP_0): 0 on the ground;
                    // +seconds since the leap; −seconds since the somersault (spent).
                    // Zero on every other slot. Mirrors GPUAgentState.t.

The config grows, both mirrors, GROWTH LAW: one pad consumed in
place, three appended, one fresh pad; 720 -> 736. leap_flip_apex is
declared here and read from U3 — one growth, not two.

FIND (state.hpp):
            float possessed_height;        // 712
            float _pad720_2;               // 716
        };
REPLACE:
            float possessed_height;        // 712
            // LEAP_0 — THE LEAP'S FOUR DIALS. Mirror of world.wgsl's config
            // (GROWTH LAW: same commit, same order, same types). What the eye
            // measures is what the dial says — heights and a time; launch
            // speed and gravity are derived at the one read
            // (behavior_player_controlled), so no room holds a second copy of
            // the arithmetic. Rests: contracts/control_panel.hpp LEAP_*,
            // boot-pinned by initializeState. One pad consumed IN PLACE, three
            // appended, one fresh pad to the boundary: 720 -> 736. Was _pad720_2.
            float leap_apex;               // 716  wu — the leap's height over the ground it left
            float leap_rise;               // 720  s — ground to apex; the dial floors it above 0
            float leap_fall_ratio;         // 724  falling gravity / rising gravity — the apex hang
            float leap_flip_apex;          // 728  wu — the somersault's own apex over where it fired
            float _pad736_0;               // 732
        };

FIND (world.wgsl):
    possessed_height: f32,          // 712
    _pad720_2: f32,                 // 716
}
REPLACE:
    possessed_height: f32,          // 712
    // LEAP_0 — the leap's four dials. Mirror of GPUDesignConfig (state.hpp)
    // — GROWTH LAW, same commit, same order, same types. Read by
    // behavior_player_controlled, which derives launch speed and gravity
    // from them at the read. One pad consumed in place, three appended, one
    // fresh pad: 720 -> 736 (state.hpp carries the witness). Was _pad720_2.
    leap_apex: f32,                 // 716
    leap_rise: f32,                 // 720
    leap_fall_ratio: f32,           // 724
    leap_flip_apex: f32,            // 728
    _pad736_0: f32,                 // 732
}

FIND (state.hpp):
        static_assert(sizeof(GPUDesignConfig) == 720,
            "GPUDesignConfig must be 720 bytes. PRUNING_1 P3 removed nine "
REPLACE:
        static_assert(sizeof(GPUDesignConfig) == 736,
            "GPUDesignConfig must be 736 bytes. PRUNING_1 P3 removed nine "

FIND (state.hpp):
            "boundary; 704 -> 720. PANORAMA_1: the PCF tap count consumes one of those three pads IN PLACE — 720 unmoved.)");
REPLACE:
            "boundary; 704 -> 720. PANORAMA_1: the PCF tap count consumes one of those three pads IN PLACE — 720 unmoved. "
            "LEAP_0: the leap's four dials — one pad consumed IN PLACE, three appended, one fresh pad to the boundary; 720 -> 736.)");

The rests, the boot pins, the rows.

FIND (contracts/control_panel.hpp):
inline constexpr float PULSE_TAP_AMPLITUDE = 1.5f;
REPLACE:
inline constexpr float PULSE_TAP_AMPLITUDE = 1.5f;

// ═══ THE LEAP (LEAP_0) ═══════════════════════════════════════════════
// THE SAME TAP, THE BODY'S HALF — input grammar again (how high a gesture
// carries), so it lives here beside the pulse's impulse. The GPU is the
// one reader (behavior_player_controlled, through the config); the CPU
// reads none of these. Boot-pinned by GPUState::initializeState; the
// organ's CONFIG rows edit the live copy. What the eye measures is what
// is authored — heights and a time; launch speed and gravity are derived
// at the read. The walk's PAWN_SPEED (15 wu/s) carries the leap about
// eight world units at these rests.
inline constexpr float LEAP_APEX       = 3.0f;   // wu — two pawn heights (PAWN_HEIGHT 1.5)
inline constexpr float LEAP_RISE       = 0.30f;  // s — ground to apex
inline constexpr float LEAP_FALL_RATIO = 1.8f;   // falling gravity / rising gravity — the apex hang
inline constexpr float LEAP_FLIP_APEX  = 1.2f;   // wu — the somersault's own apex over where it fired

FIND (state.hpp):
                config_.camera_push_radius     = CAMERA_PUSH_RADIUS;
REPLACE:
                config_.camera_push_radius     = CAMERA_PUSH_RADIUS;
                // LEAP_0 — the leap's dials, boot-pinned from THE PANEL
                // (contracts/control_panel.hpp) by the field dials' idiom.
                config_.leap_apex              = LEAP_APEX;
                config_.leap_rise              = LEAP_RISE;
                config_.leap_fall_ratio        = LEAP_FALL_RATIO;
                config_.leap_flip_apex         = LEAP_FLIP_APEX;

FIND (src/console/organ_params.inc):
ORGAN_PARAM(CONFIG, GPUDesignConfig, pawn_speed,          F32, 0.0f, 60.0f,  0.5f,   "Interaction · Pawn",   "walk speed")
REPLACE:
ORGAN_PARAM(CONFIG, GPUDesignConfig, pawn_speed,          F32, 0.0f, 60.0f,  0.5f,   "Interaction · Pawn",   "walk speed")
// LEAP_0 — the leap's dials (contracts/control_panel.hpp LEAP_*), GPU-read
// by behavior_player_controlled. The rise's floor is a step, not zero:
// launch speed and gravity divide by it.
ORGAN_PARAM(CONFIG, GPUDesignConfig, leap_apex,           F32, 0.0f,  12.0f, 0.1f,   "Interaction · Pawn",   "leap apex")
ORGAN_PARAM(CONFIG, GPUDesignConfig, leap_rise,           F32, 0.05f, 1.0f,  0.01f,  "Interaction · Pawn",   "leap rise (s)")
ORGAN_PARAM(CONFIG, GPUDesignConfig, leap_fall_ratio,     F32, 1.0f,  3.0f,  0.05f,  "Interaction · Pawn",   "leap fall ratio")

The lip, and the air resolve.

FIND (world.wgsl):
const PAWN_TURN_SPEED: f32 = 8.0;
REPLACE:
const PAWN_TURN_SPEED: f32 = 8.0;
// THE LIP LAW (LEAP_0). Aloft, a candidate move is blocked iff the ground
// there stands above the body by more than this: the walk's wall is a
// grade (PAWN_MAX_SLOPE), the flight's wall is a lip, and a rise the body
// is already clearing is not a wall. Shape, not a dial — the standing the
// ring's own consts keep.
const PAWN_AIR_LIP: f32 = 0.35;

FIND (world.wgsl — the tail of pawn_ground_resolve):
    // Fully blocked — revert, reuse prev_y (was snapped last frame)
    return vec4(prev_xz.x, prev_y, prev_xz.y, 0.0);
}
REPLACE:
    // Fully blocked — revert, reuse prev_y (was snapped last frame)
    return vec4(prev_xz.x, prev_y, prev_xz.y, 0.0);
}

// --- Pawn air resolve (LEAP_0)
//
// pawn_ground_resolve's twin for a body aloft. Same shape — the full
// move, the two axis slides, the revert — under THE LIP LAW instead of
// THE SLOPE LAW: the ground is a wall when it stands above the body (by
// more than PAWN_AIR_LIP) and a floor when it is below. Only the walker
// height is read (the standing height the body will land on); the tilt
// height has no business here — there is no grade to compare aloft.
// Returns (x, ground_y at the returned xz, z, ok). The caller owns Y: it
// integrates the flight and decides the touchdown against ground_y.
fn lip_passable(ground_y: f32, body_y: f32) -> bool {
    return ground_y <= body_y + PAWN_AIR_LIP;
}

fn pawn_air_resolve(
    new_xz: vec2<f32>, prev_xz: vec2<f32>, body_y: f32, qi: QueryInputs
) -> vec4<f32> {
    let y_new = query_ground_walker(new_xz, qi);
    if (lip_passable(y_new, body_y)) {
        return vec4(new_xz.x, y_new, new_xz.y, 1.0);           // happy path
    }

    let slide_x = vec2(new_xz.x, prev_xz.y);
    let y_x     = query_ground_walker(slide_x, qi);
    let x_ok    = lip_passable(y_x, body_y);

    let slide_z = vec2(prev_xz.x, new_xz.y);
    let y_z     = query_ground_walker(slide_z, qi);
    let z_ok    = lip_passable(y_z, body_y);

    if (x_ok && z_ok) {
        if (abs(new_xz.x - prev_xz.x) >= abs(new_xz.y - prev_xz.y)) {
            return vec4(slide_x.x, y_x, slide_x.y, 1.0);
        }
        return vec4(slide_z.x, y_z, slide_z.y, 1.0);
    }
    if (x_ok) { return vec4(slide_x.x, y_x, slide_x.y, 1.0); }
    if (z_ok) { return vec4(slide_z.x, y_z, slide_z.y, 1.0); }

    // Fully blocked — hold xz; the ground under the held xz is the floor.
    return vec4(prev_xz.x, query_ground_walker(prev_xz, qi), prev_xz.y, 0.0);
}

The walker: the hands' gate hoisted, the saddle grounding the clock,
the two laws, and an upright body aloft. All four inside
behavior_player_controlled.

FIND (world.wgsl):
    if (coupling_active(COUPLING_INPUT_MOVES_PLAYER) && !point_camera_hosted()) {
REPLACE:
    let hands_live = coupling_active(COUPLING_INPUT_MOVES_PLAYER) && !point_camera_hosted();
    if (hands_live) {

FIND (world.wgsl — the RIBBON-host arm):
        agent.vel_y = 0.0;
        agent.vel_z = 0.0;
        // Composed in the ring motor's verified order — roll, then pitch,
REPLACE:
        agent.vel_y = 0.0;
        agent.vel_z = 0.0;
        agent.t = 0.0;          // LEAP_0 — the saddle grounds the air clock; a dismount lands on the ground's law
        // Composed in the ring motor's verified order — roll, then pitch,

FIND (world.wgsl):
    if (coupling_active(COUPLING_TERRAIN_TO_PAWN_Y)) {
        let resolved = pawn_ground_resolve(vec2(agent.pos_x, agent.pos_z), prev_xz, prev_y, qi);
        agent.pos_x = resolved.x;
        agent.pos_y = resolved.y;
        agent.pos_z = resolved.z;
        if (resolved.w < 0.5) {
            agent.vel_x = 0.0;
            agent.vel_z = 0.0;
        }
    }
REPLACE:
    if (coupling_active(COUPLING_TERRAIN_TO_PAWN_Y)) {
        // ── LEAP_0: TWO LAWS, ONE CLOCK ─────────────────────────────
        // agent.t is the AIR CLOCK (see AgentState): 0 on the ground,
        // +seconds since the leap, −seconds since the somersault. On the
        // ground Y is a lookup and the wall is a grade (pawn_ground_resolve).
        // Aloft Y integrates under gravity and the wall is a lip
        // (pawn_air_resolve); the touchdown hands Y back. The door
        // (signal.jump_edge) obeys the hands' gate, so the camera host earns
        // nothing. The clock advances first, so a door that fires on an
        // existing flight starts a fresh count.
        if (agent.t > 0.0) { agent.t += dt; }
        let door   = hands_live && signal.jump_edge != 0u;
        let rise   = max(config.leap_rise, 1e-3);          // the dial floors it; this is the guard
        let g_rise = 2.0 * config.leap_apex / (rise * rise);
        if (door && agent.t == 0.0) {                       // THE LEAP — from the ground
            agent.vel_y = 2.0 * config.leap_apex / rise;
            agent.t = dt;
        }
        if (agent.t != 0.0) {
            // THE BODY'S LAW. Rising and falling gravity differ — the apex
            // hangs — and the step is semi-implicit: the velocity first,
            // then the position it carries.
            let g = select(g_rise * config.leap_fall_ratio, g_rise, agent.vel_y > 0.0);
            agent.vel_y -= g * dt;
            let y_air = prev_y + agent.vel_y * dt;
            let r = pawn_air_resolve(vec2(agent.pos_x, agent.pos_z), prev_xz, y_air, qi);
            agent.pos_x = r.x;
            agent.pos_z = r.z;
            if (r.w < 0.5) {
                agent.vel_x = 0.0;
                agent.vel_z = 0.0;
            }
            if (agent.vel_y <= 0.0 && y_air <= r.y) {       // TOUCHDOWN — the ground's law resumes
                agent.pos_y = r.y;
                agent.vel_y = 0.0;
                agent.t = 0.0;
            } else {
                agent.pos_y = y_air;
            }
        } else {
            // THE GROUND'S LAW, as ever.
            let resolved = pawn_ground_resolve(vec2(agent.pos_x, agent.pos_z), prev_xz, prev_y, qi);
            agent.pos_x = resolved.x;
            agent.pos_y = resolved.y;
            agent.pos_z = resolved.z;
            if (resolved.w < 0.5) {
                agent.vel_x = 0.0;
                agent.vel_z = 0.0;
            }
        }
    }

FIND (world.wgsl):
        let normal = terrain_normal_at(vec2(agent.pos_x, agent.pos_z), qi);
REPLACE:
        // LEAP_0 — aloft the ground has no word: the body stands upright, and
        // the tilt lag below (where a figure has one) eases it back onto the
        // slope it lands on.
        var normal = vec3(0.0, 1.0, 0.0);
        if (agent.t == 0.0) { normal = terrain_normal_at(vec2(agent.pos_x, agent.pos_z), qi); }

GATES: python3 tools/gates/glaw2/run.py (GREEN);
       python3 tools/wgsl_gate.py (PASS — naga on PATH; an absent naga
       is a failed gate, report it, do not pass it);
       python3 tools/gates/console_gate/run.py (PASS — this is the
       736 witness).
COMMIT: "LEAP_0 U2 — the leap: two laws, one clock; the ground lends the body its height"

────────────────────────────────────────────────────────────────────
U3 — THE SOMERSAULT (once per flight, from the air)
────────────────────────────────────────────────────────────────────
FIND (world.wgsl):
const PAWN_AIR_LIP: f32 = 0.35;
REPLACE:
const PAWN_AIR_LIP: f32 = 0.35;
// THE SOMERSAULT'S DURATION (LEAP_0): one full turn, measured from the
// flip's own launch. Sized to land upright from a flip fired at the
// leap's apex over flat ground at the authored rests; the screen is the
// gate. Shape, not a dial.
const LEAP_FLIP_SECONDS: f32 = 0.45;

FIND (world.wgsl):
        if (agent.t > 0.0) { agent.t += dt; }
REPLACE:
        if (agent.t > 0.0) { agent.t += dt; } else if (agent.t < 0.0) { agent.t -= dt; }

FIND (world.wgsl):
        if (door && agent.t == 0.0) {                       // THE LEAP — from the ground
            agent.vel_y = 2.0 * config.leap_apex / rise;
            agent.t = dt;
        }
REPLACE:
        if (door && agent.t == 0.0) {                       // THE LEAP — from the ground
            agent.vel_y = 2.0 * config.leap_apex / rise;
            agent.t = dt;
        } else if (door && agent.t > 0.0) {                 // THE SOMERSAULT — once, from the air
            agent.vel_y = sqrt(2.0 * g_rise * max(config.leap_flip_apex, 0.0));
            agent.t = -dt;                                  // spent; the tumble's clock starts
        }

FIND (world.wgsl):
        let orient_target = quat_multiply(tilt_quat, heading_quat);
REPLACE:
        var orient_target = quat_multiply(tilt_quat, heading_quat);
        // LEAP_0 — THE SOMERSAULT: one turn about the body's own lateral
        // axis (body X; forward is body Z — heading = atan2(vel.x, vel.z))
        // over LEAP_FLIP_SECONDS, applied in the body frame (quat_multiply
        // applies its SECOND argument first) so it tumbles along the
        // heading. The clock's negative half is the tumble's; a touchdown
        // before the turn completes cuts it. The pawn is symmetric: if the
        // tumble reads backward on the screen, negate `turn` — a value, not
        // a shape.
        if (agent.t < 0.0) {
            let turn = 6.2831853 * saturate(-agent.t / LEAP_FLIP_SECONDS);
            orient_target = quat_multiply(orient_target, quat_from_axis_angle(vec3(1.0, 0.0, 0.0), turn));
        }

FIND (src/console/organ_params.inc):
ORGAN_PARAM(CONFIG, GPUDesignConfig, leap_fall_ratio,     F32, 1.0f,  3.0f,  0.05f,  "Interaction · Pawn",   "leap fall ratio")
REPLACE:
ORGAN_PARAM(CONFIG, GPUDesignConfig, leap_fall_ratio,     F32, 1.0f,  3.0f,  0.05f,  "Interaction · Pawn",   "leap fall ratio")
ORGAN_PARAM(CONFIG, GPUDesignConfig, leap_flip_apex,      F32, 0.0f,  6.0f,  0.1f,   "Interaction · Pawn",   "somersault apex")

GATES: glaw2 (GREEN), wgsl_gate (PASS), console_gate (PASS).
COMMIT: "LEAP_0 U3 — the somersault: the same word from the air, once"

────────────────────────────────────────────────────────────────────
U4 — THE LEDGERS FOLLOW
────────────────────────────────────────────────────────────────────
Run, in this order, from the root:
  python3 tools/organ_ledger.py
  python3 tools/binding_ledger.py
  python3 tools/mirror_census.py
  python3 tools/command_census.py
Expected motion: ORGAN.md gains the four Interaction · Pawn rows
(entries 403 -> 407); BINDING_LEDGER's DesignConfig size 720 -> 736
on its three seats and every pin of the edited files refreshes;
MIRROR_LEDGER's pins refresh; COMMAND_LEDGER's cartridge.hpp pin
refreshes. Then every --check row in CLAUDE.md's gate table, plus
organ_gap --gate, organ_readers, score, shell_gate, sha256_gate: all
green. binding_gen --check goes red ONLY on a shallow clone (S-6);
on an unshallowed tree it is green — report which you saw.
COMMIT: "LEAP_0 U4 — the ledgers follow: 720 -> 736, four rows"

────────────────────────────────────────────────────────────────────
U5 — THE REGISTER WRITTEN, THE ORDER RETIRED
────────────────────────────────────────────────────────────────────
Prepend to docs/OPEN.md, after its three-line header:

## LEAP_0 — THE PAWN LEAPS (landed; Jean's visual gate open)

The pawn's height was a lookup; it is now a lookup on the ground and an
integration aloft, one clock (`agent.t`) saying which. The door is the
pulse's — the ring fires as ever, the body leaps or, once per flight,
somersaults. Four dials under Interaction · Pawn; two shapes
(`PAWN_AIR_LIP`, `LEAP_FLIP_SECONDS`) in world.wgsl. The slope law is the
walk's law only now; the leap is a ladder (any riser under the apex is a
step). Config 720 -> 736.

**Jean's readings, all against the screen:** the tumble's direction along
the heading (negate `turn` if it reads backward); the apex against the
world's risers (3 wu = two pawn heights); a tap on a summit still boards.

**Priced, not built:** hold-to-extend (keyboard-only — the glass tap
resolves at lift); the ring at touchdown instead of takeoff (needs the
P5 harvest, like `portal_trigger`); a CPU airborne sensor to silence the
somersault's ring (R1 kept every tap ringing).

Then: git rm docs/HANDOFFS/LEAP_0.md (the directory dies with it).
COMMIT: "LEAP_0 U5 — the register written, the order retired"

────────────────────────────────────────────────────────────────────
JEAN-SIDE, AFTER U5
────────────────────────────────────────────────────────────────────
glaw1; the-board-web build; boot with ?organ=1. Tap on the ground: the
ring and a leap of two pawn heights, a hang at the top, a faster fall,
full steering in the air, upright body, touchdown snaps to the walker
ground. Tap again in the air: a smaller second arc and one tumble.
FPV: the eye rides the arc. Camera host: nothing. A leap into a raised
solid's wall stops at the wall and falls beside it (the lip). Dials
under Interaction · Pawn move the arc live.
