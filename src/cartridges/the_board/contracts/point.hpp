#pragma once
#include <cstdint>

// ─── point.hpp (CONTRACT: the point — the parent of the player system) ─
// Jean's correction, ratified — the point model.
//
// THE POINT IS THE PARENT. The anchor IS a point; THE POINT OWNS THE
// BUBBLE. The camera is the point's permanent witness — it renders
// from wherever the point is — but does not own it. The point is
// HOSTED, like a spirit, wherever context demands (v3 §9 Act III /
// §11 made real):
//
//   PAWN host (the default) — the pawn hosts the point; the camera
//     couples to the pawn LIKE A KITE (the damped aim-point orbit,
//     world.wgsl update_camera_vp — preserved pixel-identical). The
//     body carries its own terrain-snap (the walker ground resolve);
//     TERRAIN RULE = SNAP.
//   CAMERA host (free-fly) — the camera hosts the point: they
//     coincide; input moves it; body-specific contributions IDLE
//     (masked, not absent — the idleness principle at the structural
//     level); TERRAIN RULE = NONE (pure fly, clips freely — the
//     revision camera).
//
// THE CHAIN extends by the same grammar: when the pawn rides the
// ribbon (sky mode), the whole chain rides — possession, free-fly,
// riding are ONE mechanism: the point migrates between hosts;
// everything else couples to it or carries it. config.possessed_slot
// was always a host pointer restricted to agent slots; this contract
// names the general form.
//
// REALIZATION: POSITION lives in the HOST's GPU
// storage — the agent slot when the pawn hosts
// (agent_state[possessed_slot]), the camera state when the camera
// hosts — read through point_pos() (world.wgsl) on the GPU and the
// host-routed P5 harvest on the CPU, whose mirror lives HERE
// (PointState.x/z + the bubble sensor — POINT_1 moved it home from
// the pawn-era PlayerState); no separate point BUFFER was needed
// (the point readback, option A, Jean's stamp) — the GPU half of
// that ruling stands. The host flag (config.point_host) routes reads and the
// input intent channel. PRESENCE FOLLOWS THE POINT (the ratified
// rule): streaming, LOD/cull, the shadow box, the orb dome, the
// living population's existence (agent + floater spawn/evict/
// possess-reach/kite/corral), the photographer's record. EMANATION
// STAYS THE BODY'S: the walk, the aura dome, the forcefield, the
// AI-pursuit-target role — all idle in free-fly by construction.
// THE BUBBLE is real (below): its first field and first sensor
// are live (below).
// ─────────────────────────────────────────────────────────────────

namespace t7 {
namespace the_board {

// ═══ THE HOSTS ═════════════════════════════════════════════════════
// Mirrored to the GPU as config.point_host (u32) — the kernels route
// the intent channel and the camera's stance on this value.

enum class PointHost : uint32_t {
    PAWN   = 0,   // the default — the body hosts; the camera kites
    CAMERA = 1,   // free-fly — the witness hosts; input flies the point
    RIBBON = 2,   // sky-flight — the ribbon hosts; the possessed body
                  // rides the seat (the pawn kernel's mount gate), the
                  // camera kites onto it; input steers the head
                  // (RESIDUE_3: riding landed as a host — the contract
                  // above made real)
};

// ═══ THE BUBBLE (first field + first sensor live) ══════════
// The bounded awareness region around the point (v3 §11): proximity,
// the portal trigger, the coming event source. Its FIRST FIELD is the
// radius; its FIRST SENSOR is the portal (pulled by Jean's
// altitude question): in camera-host an arch fires only when it sits
// within the bubble — the xz ellipse as ever, gated vertically by
// this radius (skim over → fire; fly high → no fire). In pawn-host
// the body's own crossing is the sensor, byte-identical to the
// pre-point era. The trigger rides the possessed slot's wire and the
// same P5 harvest — the wire is the realization, the bubble is the
// semantics.

inline constexpr float POINT_BUBBLE_RADIUS = 80.0f;   // world units; boot-pinned into config.point_bubble_radius (the WGSL side reads the config field). 20 until the desk raised it 4×: the vertical gate lets an arch fire from far higher up now.

struct PointBubble {
    float radius = POINT_BUBBLE_RADIUS;   // the awareness bound (the portal's vertical gate today)
    // REACH_0 — the second sensor. TRUE means: the rendered ribbon's seat
    // sits within RIBBON_LIVE.reach of the pawn, PAWN host, this frame's
    // harvest. Composed at the ONE site (the witness harvest); the boarding
    // door reads it and nothing else writes it. Rests false; teardown
    // re-darkens it.
    bool  ribbon_reach = false;
};

// ═══ THE WITNESS'S OWN DIALS ═══════════════════════════════════════
// The camera does not own the point, but it has a stance of its own, and
// the stance has rests. They are authored HERE — the witness is the
// point's, so its rests live in the point's house — and boot-pinned into
// the config mirror by the same idiom POINT_BUBBLE_RADIUS uses. One home
// authors; the boot is the transport; the organ edits the live copy.

// THE KITE LOCK. A first-order ease trails a constant-velocity target by
// v·tau, so the chase adds v·tau back to the target and the trail cancels
// identically. 1.0 is the whole cancellation and IS the shipped behavior;
// 0.0 restores the plain trail, which makes the dial the proof that the
// mechanism is the one described. Only the RIBBON host reads it — the
// walk kite is untouched.
inline constexpr float CAMERA_CHASE_FF = 1.0f;

// THE WITNESS'S PRESENCE. The eye repels floaters as it passes — cubes
// only; a sphere's motor owns its orbit and perturbing a motor is the
// complicated dynamics the ruling excludes. OFF in free-fly, like every
// other emanation.
//
// GAIN rests at half the pawn's own presence gain (world.wgsl
// CUBE_PUSH_GAIN, 25). The cube spring is 4/s², so a sustained push
// displaces by gain/4 ≈ 3 wu — the same visible shed CurlField's
// amplitude 12 already produces, which is the band this force belongs in.
// RADIUS rests at the pawn forcefield's moving-radius (world.wgsl
// PAWN_FORCEFIELD_RADIUS_MOVING, 2), so at rest the shell is a brush-past
// and not a bow wave. Both are Jean's to tune at the desk.
inline constexpr float CAMERA_PUSH_GAIN   = 12.5f;
inline constexpr float CAMERA_PUSH_RADIUS = 2.0f;

// ═══ THE POINT ═════════════════════════════════════════════════════
// The instance (point_) lives at the composition root, beside the
// witness record (PlayerState) — spine-resident, like every organ.
// POSITION is realized host-side (see the banner); host-specific
// fields (velocity, body awareness tuning, the snapped placement)
// live with the hosts and IDLE when their host is not the one.

struct PointState {
    PointHost   host = PointHost::PAWN;   // the default host — the kite

    // ── The position mirror (POINT_1: re-homed from the pawn-era
    //    PlayerState — the point's CPU face, in the point's house) ──
    float   x = 0.0f;                     // THE POINT's world X (host-authored)
    float   z = 0.0f;                     // THE POINT's world Z (host-authored)
    // RIBBON_1 — y and heading join the mirror, for one reason: possess()
    // captures the EDGE from them. A host change hands the GPU the pose the
    // body LEFT, and the trajectory is eased from there onto the saddle (or
    // off it onto the walked pose). Body hosts author both; camera-host
    // leaves them held-last, as it leaves x/z.
    float   y = 0.0f;                     // THE POINT's world Y (body hosts author it)
    float   heading = 0.0f;               // THE POINT's heading (radians)
    // ── The bubble sensor, on the possessed slot's wire in both hosts ──
    int32_t portal_trigger = -1;

    PointBubble bubble{};                 // declared whole; first sensor above
};

// ═══ THE MOUNT (RIBBON_1) ══════════════════════════════════════════
// A HOST CHANGE IS A TRAJECTORY, NOT A TELEPORT. The POSE at the far end
// is the GPU's — the saddle the ribbon's body kernel writes, or the
// walked pose the pawn kernel computes — and the CPU has no business
// knowing either. What the CPU owns is the EDGE: where the body was at
// the instant the host changed, and how far along the ease it is now.
// possess() captures the edge from the point's mirror; the frame carries
// the phase; the kernels do the mixing.
//
// CPU-only: this record rides the signal's mount block and comes back
// from nowhere.
struct MountState {
    float    phase = 1.0f;          // 0 -> 1; 1 = arrived (the rest: nothing in flight)
    uint32_t kind  = 0;             // 0 none, 1 boarding (-> the saddle), 2 landing (-> the walked pose)
    float    from[3] = { 0.0f, 0.0f, 0.0f };
    float    from_heading = 0.0f;
};

} // namespace the_board
} // namespace t7
