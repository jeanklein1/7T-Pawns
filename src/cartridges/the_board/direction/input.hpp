#pragma once
#include <cstdint>
#include "cartridges/the_board/contracts/spine_state.hpp"      // PlayerState (the anchor's organ) + TransitionPhase (the transition channel) + InputState (graduated)
#include "cartridges/the_board/contracts/mood_constants.hpp"   // MOOD_* IDs (the mood keys) + PortalDestination
#include "cartridges/the_board/contracts/point.hpp"             // PointState/PointHost (the point — the driver toggles its host)
#include "cartridges/the_board/contracts/wgpu_fwd.hpp"   // wgpu handle fwds (lockstep insurance)
#include <algorithm>       // std::max, std::min   // (impl, merged)
#include <cmath>           // std::sqrt   // (impl, merged)
#include <iostream>        // toggle / radius logs   // (impl, merged)
#include <GLFW/glfw3.h>    // the key codes (unpapered — c6)   // (impl, merged)

// ─── input.hpp (MERGED: state + deps + decls + impl) ──────────────
// History: audit/LADDER.md
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
    // Mouse → rotation rate (radians per pixel of drag). Feeds every
    // mouse-authored look/pan delta (on_mouse_move).
    static constexpr float LOOK_SENSITIVITY = 0.005f;
    // W/S/A/D velocity in free-fly (world units per second) — the
    // camera host's MOVE_SPEED, wired to config.point_fly_speed at
    // boot. The pawn host's walk speed is Idle::PAWN_SPEED (state.hpp),
    // untouched by this dial. (30 = 2× the original default — Jean's dial.)
    static constexpr float MOVE_SPEED = 30.0f;

    // Scroll → zoom-delta scale (orbit distance per wheel notch).
    static constexpr float SCROLL_ZOOM_SCALE = 2.0f;
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
};

// Mouse drag state — on_mouse_move reads these to decide which
// deltas a drag writes.
struct MouseState {
    bool left_dragging = false;
    bool right_dragging = false;
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
    PlayerState&  player_;        // fpv — the anchor toggle (v3 §9 Act III)
    WorldState&   world_state_;   // active_radius — the radius command
    RibbonState&  ribbon_state_;  // the rider fixture (Option A): possess() stages the RIBBON release here
    GPUState&     gpuState_;      // the freeze toggle + the fpv wire
    wgpu::Device& device_;        // the queue fetch (the S5-style declared handle)
    PointState&   point_;         // the point — the host toggle (key 4)
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
void on_mouse_button(InputDeps* c, int button, bool pressed);
void on_scroll(InputDeps* c, float delta);
// Per-frame
void update_movement_intent(InputDeps* c);
void clear_input_deltas(InputDeps* c);
// Camera / view commands
void toggle_fpv_mode(InputDeps* c);
void possess(InputDeps* c, PointHost next);   // THE ONE TRANSACTION — release(current) then bind(next); nothing else writes the host
void set_render_radius(InputDeps* c, uint32_t r);
void toggle_veil_dither(InputDeps* c);   // THE RIM knob (key V): icing tint <-> dither-dissolve


// ═══ MODULE IMPLEMENTATION ═══════════════════════════════════════
//
// WRAPPING FORM history in audit/LADDER.md; the impl now
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
#ifndef GLFW_KEY_V
#define GLFW_KEY_V  86
#endif
#ifndef GLFW_KEY_F4
#define GLFW_KEY_F4  293
#endif
#ifndef GLFW_KEY_F5
#define GLFW_KEY_F5  294
#endif
#ifndef GLFW_KEY_F6
#define GLFW_KEY_F6  295
#endif
#ifndef GLFW_KEY_F7
#define GLFW_KEY_F7  296
#endif
#ifndef GLFW_KEY_F8
#define GLFW_KEY_F8  297
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
    case GLFW_KEY_4:   // the point's host: camera (free-fly) <-> pawn; from RIBBON it composes — dismount first, then camera
        possess(c, c->point_.host == PointHost::CAMERA ? PointHost::PAWN : PointHost::CAMERA);
        break;
    case GLFW_KEY_5: request_mood_transition(transitionPhase, pendingDestination, mood_state, c->world_state_, MOOD_OPEN_SUNSET);    break;
    case GLFW_KEY_6: request_mood_transition(transitionPhase, pendingDestination, mood_state, c->world_state_, MOOD_INDOOR_FLAT);    break;
    case GLFW_KEY_7: request_mood_transition(transitionPhase, pendingDestination, mood_state, c->world_state_, MOOD_INDOOR_VAULT);   break;
    case GLFW_KEY_8: request_mood_transition(transitionPhase, pendingDestination, mood_state, c->world_state_, MOOD_FINITE_OUTDOOR); break;
    case GLFW_KEY_0:              cycle_orb_palette(orbs_state, &orbs_deps, q);          break;
    case GLFW_KEY_LEFT_BRACKET:   set_render_radius(c, c->world_state_.active_radius - 1); break;
    case GLFW_KEY_RIGHT_BRACKET:  set_render_radius(c, c->world_state_.active_radius + 1); break;
    case GLFW_KEY_V:              toggle_veil_dither(c);                                   break;  // THE RIM: icing tint <-> dither-dissolve

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

    // ── Diagnostics (function keys) ──────────────────────────────
    case GLFW_KEY_F4: cycle_cube_behavior_override(cube_behaviors_state, &cube_deps, q);   break;
    case GLFW_KEY_F5: cycle_floater_coordination(cube_behaviors_state, &cube_deps);        break;
    case GLFW_KEY_F6: corral_cubes(cube_behaviors_state, &cube_deps, q);                   break;
    case GLFW_KEY_F7: toggle_cube_kite_mode(cube_behaviors_state, &cube_deps, q);          break;
    case GLFW_KEY_F8:
        // ROSTER-GATE ribbon (b): sky-flight's entry
        // door rides the ribbon bit. Ungated, F8 in a ribbon-less demo snaps
        // the pawn to origin (the fail-LOUD zeros turned player-facing —
        // earlier recon). RIBBON is a host (RESIDUE_3); from camera host
        // possess() composes: release camera, mount.
        //
        // THE CANDIDATE SET (SWEEP_1 T9). The transfer targets ribbons IN
        // SCOPE, and the scope is the flown one: ribbon_frame_tick elects
        // rendered_slot as the nearest ACTIVE ribbon to the point and
        // parks it at UINT32_MAX when the world holds none. Mounting an
        // empty set gave the mount gate a zeroed sky head — the same
        // ribbon-less failure the ROSTER gate above was built for, except
        // that indoors, now that ribbons never spawn there, it is the
        // NORMAL case rather than a demo configuration.
        //
        // Empty set falls back to the PAWN, through possess() like every
        // other transfer — no side channel, and no-op when the pawn
        // already hosts (the transaction returns on cur == next).
        if constexpr (ROSTER.ribbon) {
            if (c->point_.host == PointHost::RIBBON) {
                possess(c, PointHost::PAWN);
            } else {
                const uint32_t flown = c->ribbon_state_.rendered_slot;
                const bool have_ribbon = flown != UINT32_MAX
                                      && c->ribbon_state_.active[flown].active;
                possess(c, have_ribbon ? PointHost::RIBBON : PointHost::PAWN);
            }
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
    }
    update_movement_intent(c);
}

// ═══ MOUSE / SCROLL ══════════════════════════════════════════════

inline void on_mouse_move(InputDeps* c, float dx, float dy) {
    constexpr float sensitivity = CameraControls::LOOK_SENSITIVITY;  // the panel dial
    if (c->mouse_.left_dragging) {
        c->inputState_.look_az_delta += dx * sensitivity;
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
    c->inputState_.zoom_delta -= delta * CameraControls::SCROLL_ZOOM_SCALE;
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
}

inline void clear_input_deltas(InputDeps* c) {
    c->inputState_.look_az_delta = 0.0f;
    c->inputState_.look_el_delta = 0.0f;
    c->inputState_.zoom_delta = 0.0f;
    c->inputState_.pan_x_delta = 0.0f;
    c->inputState_.pan_y_delta = 0.0f;
}

// ═══ CAMERA / VIEW COMMANDS ══════════════════════════════════════

inline void toggle_fpv_mode(InputDeps* c) {
    c->player_.fpv_mode = !c->player_.fpv_mode;
    c->gpuState_.set_fpv_mode(c->player_.fpv_mode ? 1 : 0);
    std::cout << "[the_board] Camera mode: "
        << (c->player_.fpv_mode ? "First-Person View" : "Orbit") << std::endl;
}

// THE ONE TRANSACTION (RESIDUE_3): release(current) then bind(next),
// always, in that order — no frame with two live hosts, none with
// none. Both keys call possess(); nothing else writes the host.
//
//   PAWN   — the kite: the body walks (W/A/S/D), the camera follows.
//   CAMERA — free-fly: input moves only the camera; the body idles.
//   RIBBON — sky-flight: the move channel (W/S = throttle, A/D = yaw)
//     steers the rendered ribbon's head under its own forward-biased
//     grammar; the altitude is held by a critically damped pen, not
//     fixed; the possessed body rides the seat (the pawn kernel's
//     mount gate) and the camera kites onto it.
//
// release(RIBBON) = today's sky-OFF outcome for the flown ribbon: the
// death needs the machine face this transaction does not carry, so the
// transaction STAGES it (sky.release_pending) and the one owning verb
// (release_sky_exit_ribbon, R7's head) consumes it this coming tick —
// the mount/dismount EDGE lives here now, not in a mode_prev mirror.
// release(CAMERA) = the host set below, whole. release(PAWN) = nothing:
// the body idles when not hosting, by construction.
// bind(next) = the host set + the GPU mirror; binding RIBBON is today's
// mount path (the kernels read the host next dispatch).
inline void possess(InputDeps* c, PointHost next) {
    const PointHost cur = c->point_.host;
    if (cur == next) return;
    if (cur == PointHost::RIBBON)
        c->ribbon_state_.sky.release_pending = true;
    c->point_.host = next;
    c->gpuState_.set_point_host(static_cast<uint32_t>(next));
    std::cout << "[Point] Host: "
        << (next == PointHost::RIBBON ? "RIBBON (fly with W/S/A/D)"
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
