#pragma once
#include <cstdint>
#include "cartridges/the_board/contracts/spine_state.hpp"      // PlayerState (the anchor's organ) + TransitionPhase (the transition channel) + InputState (graduated)
#include "cartridges/the_board/contracts/mood_constants.hpp"   // MOOD_* IDs (the mood keys) + PortalDestination
#include "cartridges/the_board/contracts/point.hpp"             // PointState/PointHost (the point — the driver toggles its host)
#include "cartridges/the_board/contracts/wgpu_fwd.hpp"   // wgpu handle fwds (lockstep insurance)
#include "cartridges/the_board/contracts/control_panel.hpp"   // ORGAN_3b — PANEL_LIVE.camera: the look/zoom controls' live surface
#include <algorithm>       // std::max, std::min   // (impl, merged)
#include <cmath>           // std::sqrt   // (impl, merged)
#include <iostream>        // toggle / radius logs   // (impl, merged)
#include "core/instruments.hpp"   // PURSE_0 R3 — INSTRUMENTS.stream_witness gates the sensitivity readback
#include <GLFW/glfw3.h>    // the key codes (unpapered — c6)   // (impl, merged)

// ─── input.hpp (MERGED: state + deps + decls + impl) ──────────────
// COHORT: after ribbon.hpp (possess stages the release on RibbonState.sky) +
// the door owners (pawn/orbs/agents/cube 66-71); InputState graduated
// to contracts/spine_state.hpp (it precedes ribbon in the cohort).
//
// The home of input dispatch.
// ── A note on bindings (READ ME if changing keys) ─────────────────
// ──────────────────────────────────────────────────────────────────
//
// The impl reaches GLFW (<GLFW/glfw3.h>, its own include), the deps
// face below (inputState / keys_ / mouse_ / player_ / world_state_ /
// ribbon_state / gpuState_ / device_), the command fan's TARGET
// organs (on_key_down's parameters — pawn / orbs / agents / cubes +
// the transition channel), the owner command doors (pawn.hpp / orbs.hpp
// / agents.hpp / cube_behaviors.hpp / mood.hpp's
// request_mood_transition / patch_system.hpp's request_recenter) +
// mood_constants.hpp's MOOD_* IDs, and the patch radii (Dim::PATCH_GRID_RADIUS /
// Dim::PATCH_PREGEN_RADIUS — patch_system.hpp vocabulary).
// ─────────────────────────────────────────────────────────────────

// The deps face holds the queue-fetch handle (the S5 pattern, gol);
// the wgpu::Device fwd rides contracts/wgpu_fwd.hpp (include block).

namespace t7 {
namespace the_board {

// fwd — the driver's true reaches (InputDeps reference members) and
// the command fan's target organs (reference params in on_key_down's
// declaration); complete types arrive with their owners in the cohort.
struct WorldState; struct RibbonState; class GPUState;
struct PawnState;  struct PawnDeps;
struct OrbsState;  struct OrbsDeps;
struct AgentState; struct AgentsDeps;
struct CubeBehaviorsState; struct CubeDeps;
struct MoodState;

// ═══ CameraControls — PARAMETER PANEL (the first panel, deliberately
// MINIMAL — the FORM TEST for per-module
// panels: one organized block, clear names, editable without hunting.
// Terrain inherits this convention.) ═══════════════════════════════
//
// Deferred growth (named, not carried): smoothing/damping, invert-Y,
// sprint multiplier, split H/V sensitivity, a vertical-lift axis.
struct CameraControls {
    // ── LIVE (run-time) ──────────────────────────────────────────
    // Mouse → rotation rate (radians per pixel of drag). Feeds every
    // mouse-authored look/pan delta (on_mouse_move). The initialiser IS
    // the design value: tune with KP_+/KP_-, read the printed number,
    // write it here. One number, one home, no default table.
    float look_sensitivity = 0.005f;

    // ── DESIGN-TIME (the dial's own grammar) ─────────────────────
    // Multiplicative: sensitivity is perceived logarithmically, so a
    // fixed additive step is huge at the bottom and invisible at the top.
    // LOOK_SENS_STEP / _RANGE / _INIT and SCROLL_ZOOM_SCALE graduated to
    // PANEL_LIVE.camera (contracts/control_panel.hpp) at ORGAN_3 w2, where
    // PANEL_TABLE is the design row. ORGAN_3b moved the readers, which w2
    // had left behind — the values are the same, the home is now one.
    // W/S/A/D velocity in free-fly (world units per second) — the
    // camera host's MOVE_SPEED, wired to config.point_fly_speed at
    // boot. The pawn host's walk speed is Idle::PAWN_SPEED (state.hpp),
    // untouched by this dial. (30 = 2× the original default — Jean's dial.)
    static constexpr float MOVE_SPEED = 30.0f;

    // Scroll → zoom-delta scale (orbit distance per wheel notch).
};

// ═══ INPUT STATE ═════════════════════════════════════════════════
// struct InputState GRADUATED to contracts/spine_state.hpp (the
// driver's intent organ — read by the spine's signal fill and the
// ribbon's sky flight, so it precedes them in the cohort). KeyState /
// MouseState stay here: the driver's private organs.

// Held movement keys (W/S/A/D — THE UNIVERSAL MOVE CHANNEL):
// one key set authors the driver's move-intent; WHOEVER HOSTS THE
// POINT consumes it, under that host's own constraint AND mapping —
// the constraint-and-mapping IS the host's behavioral identity (pawn:
// camera-relative full-directional, snap; camera: camera-relative
// full-directional, rule none; ribbon when it hosts: its own
// forward-biased grammar — a ribbon that could reverse and strafe
// wouldn't be a ribbon). update_movement_intent folds these into
// inputState.move_x/move_z.
struct KeyState {
    bool forward = false;
    bool backward = false;
    bool left = false;
    bool right = false;
    // RIBBON_1 — the ride key's held flag. The console raises KeyDown on
    // GLFW_REPEAT as well as GLFW_PRESS (console.hpp inject_key_event), and a
    // toggle that flip-flops under an autorepeat is a toggle that cannot be
    // held. The four above already carry held state; this is the fifth, and
    // it exists to make R a DOWN EDGE.
    bool ride_held = false;
    // PULSE_1 — SPACE's held flag, the sixth, and for the same reason:
    // the pulse is a STRIKE, not a switch and not a stream. Under
    // autorepeat an unguarded key-down would issue a pulse every repeat
    // tick and hold the live card's writer awake for as long as a thumb
    // rested on the bar.
    bool pulse_held = false;
};

// Mouse drag state — on_mouse_move reads these to decide which
// deltas a drag writes.
struct MouseState {
    bool left_dragging = false;
    bool right_dragging = false;
};

// SHIP_1 — THE THUMB'S HALF OF THE UNIVERSAL MOVE CHANNEL. The stick
// is analog where the keys are a four-way fold, so it cannot be
// expressed as held-key booleans without inventing a synthetic
// keypress — which would be a SECOND home for an intent move_x/move_z
// already holds. It gets its own organ instead, and
// update_movement_intent folds both hands into the one channel.
//
// The console writes this already dead-zoned and unit-clamped, in the
// same axis convention the keys use (x right, z forward-negative).
// Native never wrote it — there were no touch callbacks there — so the
// fold below is native-identical by construction, not by care.
struct TouchMoveState {
    float x = 0.0f;
    float z = 0.0f;
};

// ═══ THE DEPS FACE ═══════════════════════════════════════════════
//
// Input's own organs plus its true reaches — the driver's face (v3
// §9 Act I: a driver writes intents through bodies it does not own).
// The command fan's TARGET organs are deliberately NOT members: they
// ride on_key_down's parameters — the root addresses the fan's
// targets at the call site, through the owner command doors. THE F6
// SOCKET stays RESERVED (the door registry): when a driver must address a
// body it does not own by synchronous command beyond this fan, the
// addressed-intent socket (v3 §9 Act II, §13) is where it routes.
struct InputDeps {
    InputState&   inputState_;
    KeyState&     keys_;
    MouseState&   mouse_;
    TouchMoveState& touch_;       // SHIP_1 — the stick's analog vector (zero on native)
    PlayerState&  player_;        // fpv — the anchor toggle (v3 §9 Act III)
    WorldState&   world_state_;   // active_radius — the radius command
    RibbonState&  ribbon_state_;  // the rider fixture: possess() stages the RIBBON release here
    GPUState&     gpuState_;      // the freeze toggle + the fpv wire
    wgpu::Device& device_;        // the queue fetch (the S5-style declared handle)
    PointState&   point_;         // the point — the host, and the mirror possess() captures the edge from
    MountState&   mount_;         // the mount's edge + ease (RIBBON_1); possess() writes it, FillSignal ships it
    CameraControls& camera_;      // the first live panel dial (KP_+/KP_-)
};

// ═══ MODULE FUNCTIONS — DECLARATIONS ═════════════════════════════

// GLFW callbacks (routed by the spine's on_input override). The
// command fan's targets are parameters — organ-named, so the fan's
// door calls read as organ addressing (door-registry order).
void on_key_down(InputDeps* c, int key,
    PawnState& pawn_state, PawnDeps& pawn_deps,
    OrbsState& orbs_state, OrbsDeps& orbs_deps,
    AgentState& agent_state, AgentsDeps& agents_deps,
    CubeBehaviorsState& cube_behaviors_state, CubeDeps& cube_deps,
    TransitionPhase& transitionPhase, PortalDestination& pendingDestination,
    MoodState& mood_state);
void on_key_up(InputDeps* c, int key);
void on_mouse_move(InputDeps* c, float dx, float dy);
// SHIP_1 — the touch doors. Each writes the SAME organ its mouse/key
// sibling writes; none of them invents a channel.
void on_touch_move(InputDeps* c, float x, float z);
void on_touch_look(InputDeps* c, float dx, float dy);
void on_touch_zoom(InputDeps* c, float delta);
// The two clean-tap verbs. Shaped like on_key_down: the TARGET organs
// ride the parameters, because the driver owns neither of them and the
// root addresses them at the call site through the owner doors.
void on_touch_tap_left(InputDeps* c, PawnState& pawn_state, PawnDeps& pawn_deps);
void on_touch_tap_right(InputDeps* c, AgentState& agent_state, AgentsDeps& agents_deps);
void request_radial_pulse(InputDeps* c);   // the pulse's owner door (SPACE + the lone tap)
void on_touch_tap_pulse(InputDeps* c);
void on_mouse_button(InputDeps* c, int button, bool pressed);
void on_scroll(InputDeps* c, float delta);
// Per-frame
void update_movement_intent(InputDeps* c);
void clear_input_deltas(InputDeps* c);
// Camera / view commands
void toggle_fpv_mode(InputDeps* c);
void possess(InputDeps* c, PointHost next);   // THE ONE TRANSACTION — capture the edge, flip the host both rooms, start the ease
void set_render_radius(InputDeps* c, uint32_t r);
void toggle_veil_dither(InputDeps* c);   // THE RIM knob (key V): icing tint <-> dither-dissolve
void nudge_look_sensitivity(InputDeps* c, bool up);   // KP_+ / KP_- — multiplicative, clamped


// ═══ MODULE IMPLEMENTATION ═══════════════════════════════════════
//
// WRAPPING FORM: the impl now
// rides its own header. GLFW is named here —
// the dependency is the module's, not inherited from the host TU.
// The fallback #defines below are preprocessor — namespace-blind.

// ═══ GLFW KEY CODE FALLBACKS ═════════════════════════════════════

#ifndef GLFW_KEY_KP_8
#define GLFW_KEY_KP_8  328
#endif
#ifndef GLFW_KEY_KP_DECIMAL
#define GLFW_KEY_KP_DECIMAL  330
#endif
#ifndef GLFW_KEY_LEFT_CONTROL
#define GLFW_KEY_LEFT_CONTROL   341
#endif
#ifndef GLFW_KEY_RIGHT_CONTROL
#define GLFW_KEY_RIGHT_CONTROL  345
#endif
#ifndef GLFW_KEY_CAPS_LOCK
#define GLFW_KEY_CAPS_LOCK   280
#endif
#ifndef GLFW_KEY_4
#define GLFW_KEY_4  52
#endif
#ifndef GLFW_KEY_9
#define GLFW_KEY_9  57
#endif
#ifndef GLFW_KEY_W
#define GLFW_KEY_W  87
#endif
#ifndef GLFW_KEY_A
#define GLFW_KEY_A  65
#endif
#ifndef GLFW_KEY_S
#define GLFW_KEY_S  83
#endif
#ifndef GLFW_KEY_D
#define GLFW_KEY_D  68
#endif
#ifndef GLFW_KEY_R
#define GLFW_KEY_R  82
#endif
#ifndef GLFW_KEY_V
#define GLFW_KEY_V  86
#endif
#ifndef GLFW_KEY_SPACE
#define GLFW_KEY_SPACE  32
#endif


// ═══ KEY DISPATCH ════════════════════════════════════════════════

inline void on_key_down(InputDeps* c, int key,
    PawnState& pawn_state, PawnDeps& pawn_deps,
    OrbsState& orbs_state, OrbsDeps& orbs_deps,
    AgentState& agent_state, AgentsDeps& agents_deps,
    CubeBehaviorsState& cube_behaviors_state, CubeDeps& cube_deps,
    TransitionPhase& transitionPhase, PortalDestination& pendingDestination,
    MoodState& mood_state)
{
    // Single queue fetch: every queue-using case below reuses this.
    wgpu::Queue q = c->device_.GetQueue();

    switch (key) {

    // ── Movement ─────────────────────────────────────────────────
    case GLFW_KEY_W: c->keys_.forward = true;  break;
    case GLFW_KEY_S: c->keys_.backward = true; break;
    case GLFW_KEY_A: c->keys_.left = true;     break;
    case GLFW_KEY_D: c->keys_.right = true;    break;

    // ── World / aura toggles ─────────────────────────────────────
    case GLFW_KEY_2: toggle_aura_height(pawn_state, &pawn_deps);  break;  // pawn command door
    case GLFW_KEY_3: toggle_aura(pawn_state, &pawn_deps);          break;  // pawn command door
    case GLFW_KEY_5: request_mood_transition(transitionPhase, pendingDestination, mood_state, c->world_state_, MOOD_OPEN_SUNSET);    break;
    case GLFW_KEY_6: request_mood_transition(transitionPhase, pendingDestination, mood_state, c->world_state_, MOOD_INDOOR_FLAT);    break;
    case GLFW_KEY_7: request_mood_transition(transitionPhase, pendingDestination, mood_state, c->world_state_, MOOD_INDOOR_VAULT);   break;
    case GLFW_KEY_8: request_mood_transition(transitionPhase, pendingDestination, mood_state, c->world_state_, MOOD_FINITE_OUTDOOR); break;
    case GLFW_KEY_9: request_mood_transition(transitionPhase, pendingDestination, mood_state, c->world_state_, MOOD_OPEN_NIGHT);     break;
    case GLFW_KEY_4: request_mood_transition(transitionPhase, pendingDestination, mood_state, c->world_state_, MOOD_OPEN_NOON);      break;
    case GLFW_KEY_0:              cycle_orb_palette(orbs_state, &orbs_deps, q);          break;
    case GLFW_KEY_LEFT_BRACKET:   set_render_radius(c, c->world_state_.active_radius - 1); break;
    case GLFW_KEY_RIGHT_BRACKET:  set_render_radius(c, c->world_state_.active_radius + 1); break;
    case GLFW_KEY_V:              toggle_veil_dither(c);                                   break;  // THE RIM: icing tint <-> dither-dissolve
    // SPACE RINGS THE GROUND. Down edge only — the held flag above is
    // why. Momentary by ruling: the pulse is an event, not a mode, so
    // there is no toggle here and MMODE_RADIAL_PULSE is not replaced.
    case GLFW_KEY_SPACE:
        if (!c->keys_.pulse_held) {
            c->keys_.pulse_held = true;
            request_radial_pulse(c);   // one door, two mouths — the glass's is the other
        }
        break;

    // ── Look dial (numpad) ───────────────────────────────────────
    case GLFW_KEY_KP_ADD:      nudge_look_sensitivity(c, true);   break;
    case GLFW_KEY_KP_SUBTRACT: nudge_look_sensitivity(c, false);  break;

    // ── Orb utilities (numpad) ───────────────────────────────────
    case GLFW_KEY_KP_8:       cycle_orb_motion_rule(orbs_state, &orbs_deps, q);            break;
    // KP_9 freed: the dome anchor toggle retired — the dome is
    // a skybox, eye-centered always.
    case GLFW_KEY_KP_DECIMAL: cycle_orb_gesture(orbs_state, &orbs_deps, q);                break;

    // ── Camera / possession ──────────────────────────────────────
    case GLFW_KEY_LEFT_CONTROL:
    case GLFW_KEY_RIGHT_CONTROL:
        toggle_fpv_mode(c);
        break;
    case GLFW_KEY_CAPS_LOCK:  try_possess_nearest(agent_state, &agents_deps, q);  break;
    // R RIDES. Down edge only — the held flag above is why. From the ribbon
    // it dismounts back to the pawn; from anywhere else it asks to board and
    // the REACH gate answers (possess refuses out of reach). CAMERA stays
    // reachable through the panel row.
    case GLFW_KEY_R:
        if (!c->keys_.ride_held) {
            c->keys_.ride_held = true;
            possess(c, c->point_.host == PointHost::RIBBON
                       ? PointHost::PAWN : PointHost::RIBBON);
        }
        break;
    }
    update_movement_intent(c);
}

inline void on_key_up(InputDeps* c, int key) {
    switch (key) {
    case GLFW_KEY_W: c->keys_.forward = false;  break;
    case GLFW_KEY_S: c->keys_.backward = false; break;
    case GLFW_KEY_A: c->keys_.left = false;     break;
    case GLFW_KEY_D: c->keys_.right = false;    break;
    case GLFW_KEY_R: c->keys_.ride_held = false; break;
    case GLFW_KEY_SPACE: c->keys_.pulse_held = false; break;
    }
    update_movement_intent(c);
}

// ═══ MOUSE / SCROLL ══════════════════════════════════════════════

inline void on_mouse_move(InputDeps* c, float dx, float dy) {
    const float sensitivity = c->camera_.look_sensitivity;  // the panel dial, now live
    if (c->mouse_.left_dragging) {
        c->inputState_.look_az_delta -= dx * sensitivity;
        c->inputState_.look_el_delta += dy * sensitivity;
    }
    if (c->mouse_.right_dragging) {
        c->inputState_.pan_x_delta += dx * sensitivity;
        c->inputState_.pan_y_delta -= dy * sensitivity;
    }
}

inline void on_mouse_button(InputDeps* c, int button, bool pressed) {
    if (button == 0) c->mouse_.left_dragging = pressed;
    if (button == 1) c->mouse_.right_dragging = pressed;
}

inline void on_scroll(InputDeps* c, float delta) {
    c->inputState_.zoom_delta -= delta * PANEL_LIVE.camera.scroll_zoom_scale;
}

// ═══ TOUCH (SHIP_1) ══════════════════════════════════════════════

// The stick moved (or lifted — a lift is the zero vector, and it
// arrives as an ordinary event so the release is not a special case).
// Re-folds the move channel immediately, exactly as a key edge does:
// the thumb has no key-up to piggyback on.
inline void on_touch_move(InputDeps* c, float x, float z) {
    c->touch_.x = x;
    c->touch_.z = z;
    update_movement_intent(c);
}

// The thumb's look. Lands on the SAME two deltas the mouse drag lands
// on, with the same signs, so the camera cannot tell which hand moved
// it. No drag gate: the mouse needs a held button to distinguish look
// from an idle pointer, and a finger on the glass IS the held button.
// Already scaled by LOOK_SENS_TOUCH at the console's frame tick — the
// touch sensitivity is deliberately not CameraControls::look_sensitivity,
// because a thumb sweeps a fraction of the arc a mouse does.
inline void on_touch_look(InputDeps* c, float dx, float dy) {
    c->inputState_.look_az_delta -= dx;
    c->inputState_.look_el_delta += dy;
}

// The pinch. ONE zoom channel: this is the same zoom_delta the scroll
// wheel writes, clamped downstream by the kernel's CAMERA_MIN/MAX_DISTANCE
// — no second dial, no touch-only range. The console already signed it
// (spread = in) and scaled it by PINCH_SENS, so the only thing left is
// which accumulator it belongs to.
inline void on_touch_zoom(InputDeps* c, float delta) {
    c->inputState_.zoom_delta += delta;
}

// ── The reserved taps, bound (SHIP_1 U5) ─────────────────────────
// Recon found both verbs already in the kernel with desktop bindings, so
// both slots bind rather than staying reserved. These call the SAME
// owner doors the keys call — key 3 and CAPS_LOCK — so a thumb and a
// keyboard reach one implementation, not two.

// LEFT, second finger, clean tap — the aura (key 3's door).
inline void on_touch_tap_left(InputDeps* c, PawnState& pawn_state, PawnDeps& pawn_deps) {
    (void)c;
    toggle_aura(pawn_state, &pawn_deps);
}

// RIGHT, two fingers, clean tap — possession (CAPS_LOCK's door).
inline void on_touch_tap_right(InputDeps* c, AgentState& agent_state, AgentsDeps& agents_deps) {
    wgpu::Queue q = c->device_.GetQueue();
    try_possess_nearest(agent_state, &agents_deps, q);
}

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
}

// EITHER HALF, one finger, clean tap — the pulse (SPACE's twin mouth).
inline void on_touch_tap_pulse(InputDeps* c) {
    request_radial_pulse(c);
}

// ═══ MOVEMENT INTENT + DELTA CLEAR ═══════════════════════════════

inline void update_movement_intent(InputDeps* c) {
    c->inputState_.move_x = 0.0f;
    c->inputState_.move_z = 0.0f;

    // THE UNIVERSAL MOVE CHANNEL: W/S/A/D author this fold;
    // consumption is host-routed downstream (the pawn kernel's
    // point_camera_hosted guard, the camera's fly branch, the ribbon's
    // sky steering) — no CPU-side routing needed with one key set.
    if (c->keys_.forward)  c->inputState_.move_z -= 1.0f;
    if (c->keys_.backward) c->inputState_.move_z += 1.0f;
    if (c->keys_.left)     c->inputState_.move_x -= 1.0f;
    if (c->keys_.right)    c->inputState_.move_x += 1.0f;

    float len = std::sqrt(c->inputState_.move_x * c->inputState_.move_x +
        c->inputState_.move_z * c->inputState_.move_z);
    if (len > 1.0f) {
        c->inputState_.move_x /= len;
        c->inputState_.move_z /= len;
    }

    // SHIP_1 — THE SECOND HAND ON THE SAME WHEEL. The keys fold to a
    // unit-or-less vector above; the stick arrives already dead-zoned
    // and unit-clamped. Summing them lets both hands drive at once
    // (a thumb walking while a key strafes) and the clamp below keeps
    // the total honest — two hands cannot buy more than full speed.
    //
    // NATIVE: touch_ was never written there, so this added 0, `total`
    // equalled `len` which the branch above already brought to <= 1,
    // and the clamp could not fire. WASD was equivalent before and
    // after, by construction rather than by inspection.
    c->inputState_.move_x += c->touch_.x;
    c->inputState_.move_z += c->touch_.z;
    float total = std::sqrt(c->inputState_.move_x * c->inputState_.move_x +
        c->inputState_.move_z * c->inputState_.move_z);
    if (total > 1.0f) {
        c->inputState_.move_x /= total;
        c->inputState_.move_z /= total;
    }
}

inline void clear_input_deltas(InputDeps* c) {
    c->inputState_.look_az_delta = 0.0f;
    c->inputState_.look_el_delta = 0.0f;
    c->inputState_.zoom_delta = 0.0f;
    c->inputState_.pan_x_delta = 0.0f;
    c->inputState_.pan_y_delta = 0.0f;
}

// ═══ CAMERA / VIEW COMMANDS ══════════════════════════════════════

// The printed line is the dial's whole point: it reads back as the
// number to write into look_sensitivity's initialiser. The run-time
// value is a VIEW of the source constant, not a rival to it.
inline void nudge_look_sensitivity(InputDeps* c, bool up) {
    const auto& cam = PANEL_LIVE.camera;
    const float lo = cam.look_sens_init / cam.look_sens_range;
    const float hi = cam.look_sens_init * cam.look_sens_range;
    const float k  = up ? cam.look_sens_step
                        : 1.0f / cam.look_sens_step;

    c->camera_.look_sensitivity =
        std::min(hi, std::max(lo, c->camera_.look_sensitivity * k));

    // A LAB TUNING READBACK, AND THE ONE PRINT IN THIS FILE WITH NO
    // CHANGE DETECTOR (PURSE_0 R3). The value is clamped into [lo,hi] and
    // then printed unconditionally, so holding KP_+ at the top of the
    // range streams an identical line at the console's autorepeat rate —
    // KeyState::ride_held records that KeyDown fires on GLFW_REPEAT too.
    //
    // P6 DOES NOT PROTECT IT: look_sensitivity is a continuous multiplier
    // feeding on_mouse_move's deltas, not a runtime switch that SELECTS
    // behaviour, so there is no switch whose silence would need
    // explaining. This file's own header states the purpose — "tune with
    // KP_+/KP_-, read the printed number, write it here" — which is the
    // lab build's job exactly.
    if constexpr (t7::INSTRUMENTS.stream_witness) {
        std::cout << "[Camera] Look sensitivity: " << c->camera_.look_sensitivity
                  << "  (x" << (c->camera_.look_sensitivity / PANEL_LIVE.camera.look_sens_init)
                  << " of design)\n";
    }
}

inline void toggle_fpv_mode(InputDeps* c) {
    c->player_.fpv_mode = !c->player_.fpv_mode;
    c->gpuState_.set_fpv_mode(c->player_.fpv_mode ? 1 : 0);
    std::cout << "[the_board] Camera mode: "
        << (c->player_.fpv_mode ? "First-Person View" : "Orbit") << std::endl;
}

// THE ONE TRANSACTION (RESIDUE_3, re-authored at RIBBON_1): capture the
// EDGE, flip the host in both rooms, start the ease. Nothing else writes
// the host.
//
//   PAWN   — the kite: the body walks (W/A/S/D), the camera follows.
//   CAMERA — free-fly: input moves only the camera; the body idles.
//   RIBBON — sky-flight: the move channel (W = throttle, A/D = yaw) steers
//     the rendered ribbon's head under its own forward-biased grammar; the
//     altitude is held by a critically damped pen, not fixed; the possessed
//     body rides the saddle the ribbon's body kernel writes, and the camera
//     kites onto it.
//
// A HOST CHANGE IS A TRAJECTORY. The far pose belongs to a kernel — the
// saddle when boarding, the walked pose when landing — so this captures
// only where the body WAS, from the point's mirror, and hands the GPU a
// phase to ease along. No teleport, and no CPU opinion about the far end.
//
// release(RIBBON) = the ribbon is handed back to the wander brain rather
// than freed (RIBBON_1's ruling): the transaction STAGES the request
// (sky.release_pending) and the one owning verb — ribbon_on_dismount, which
// carries the machine face this door does not — consumes it next tick.
// release(CAMERA) = the host set below, whole. release(PAWN) = nothing: the
// body idles when not hosting, by construction.
inline void possess(InputDeps* c, PointHost next) {
    const PointHost cur = c->point_.host;
    if (cur == next) return;
    if (next == PointHost::RIBBON
        && c->ribbon_state_.rendered_slot == UINT32_MAX) {
        std::cout << "[Point] no ribbon to ride\n";
        return;
    }
    // REACH_0 — BOARDING IS EARNED. The bubble's second sensor must be
    // live: the rendered ribbon's seat within RIBBON_LIVE.reach of the
    // pawn — in practice a colossus summit under a crossing, because the
    // pen's floor makes that the one place the gap closes. The descend
    // direction is NEVER gated: we come back down from anywhere.
    if (next == PointHost::RIBBON && !c->point_.bubble.ribbon_reach) {
        std::cout << "[Point] ribbon out of reach\n";
        return;
    }
    c->mount_.from[0] = c->point_.x;
    c->mount_.from[1] = c->point_.y;
    c->mount_.from[2] = c->point_.z;
    c->mount_.from_heading = c->point_.heading;
    c->mount_.phase = 0.0f;
    c->mount_.kind  = (next == PointHost::RIBBON) ? 1u
                    : (cur == PointHost::RIBBON) ? 2u : 0u;
    if (cur == PointHost::RIBBON) c->ribbon_state_.sky.release_pending = true;
    c->point_.host = next;
    c->gpuState_.set_point_host(static_cast<uint32_t>(next));
    std::cout << "[Point] Host: "
        << (next == PointHost::RIBBON ? "RIBBON (W throttle, A/D steer, R dismount)"
            : next == PointHost::CAMERA ? "CAMERA (free-fly)"
            : "PAWN (the kite)") << "\n";
}

inline void set_render_radius(InputDeps* c, uint32_t r) {
    r = std::max(r, Dim::PATCH_GRID_RADIUS);
    r = std::min(r, Dim::PATCH_PREGEN_RADIUS);
    if (r == c->world_state_.active_radius) return;
    c->world_state_.active_radius = r;
    uint32_t side = 2 * r + 1;
    std::cout << "[the_board] Render radius: " << r
        << " (" << side << "x" << side << " = " << side * side << " patches)" << std::endl;
    // Force full re-evaluation on next frame — through the owner's door
    request_recenter(c->world_state_);
}

// THE RIM knob (key V): flip the veil's icing between TINT (fade to fog,
// default) and DITHER-DISSOLVE (geometry condenses). Reads the current
// config value and flips it — the dirty-gated setter rides the next U8
// config drain (no queue needed here).
inline void toggle_veil_dither(InputDeps* c) {
    bool on = c->gpuState_.config().veil_dither > 0.5f;
    c->gpuState_.set_veil_dither(on ? 0.0f : 1.0f);
    std::cout << "[the_board] Veil rim: " << (on ? "TINT (fade to fog)" : "DITHER-DISSOLVE") << std::endl;
}

} // namespace the_board
} // namespace t7
