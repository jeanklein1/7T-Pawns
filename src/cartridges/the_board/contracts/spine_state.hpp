#pragma once
#include <iostream>   // ATTIC_ATRIUM — mood_def refuses an out-of-range id, loudly
#include <cstdint>
#include "cartridges/the_board/contracts/mood_constants.hpp"   // MOOD_COUNT (sizes MOOD_TABLE) + the Mood IDs + PortalDestination (the request door)

// ─── spine_state.hpp (CONTRACT: the spine's organ types) ─────────
//
// The in-class trio graduates to file scope so module deps structs
// can name the types without the complete Cartridge. The
// INSTANCES (time_state_, player_, transitionPhase_) stay at the
// composition root; the residency rulings (SEAM[spine:P8],
// SEAM[spine:transitions]) are unchanged — this is a type move, not
// an ownership move. MoodState — the spine-resident organ TYPE —
// lives here with its transition machine, beside InputState,
// along with the atmosphere vocabulary its early readers
// need (CeilingType / MoodProfile / MOOD_TABLE) and the transition
// request door's DECLARATION (the def rides merged mood.hpp).
//
// ─────────────────────────────────────────────────────────────────

namespace t7 {
namespace the_board {

// ═══ TIME STATE ══════════════════════════════════════════════════
// Per-frame clock state used everywhere. beats/seconds advance
// monotonically; dt is the most recent frame delta.
// ═══ THE CAMERA POSE (PLUMB_0 C1, RULING-1) ═══════════════════════
//
// The eye and the two angles, on the CPU, once a frame. GPUCameraState is a
// realization type and stays there; this is the CONTRACTS-tier projection of
// it, so a body (the gallery's tide) can ask where the viewer is looking
// without the bodies tier learning a realization struct.
//
// ONE HOME. The harvest fills this from the mapped readback and drops the raw
// struct, exactly as it always dropped it — what changed is that the four
// numbers worth keeping now survive the callback.
//
// THE FORWARD VECTOR, for every reader:
//     -(cos(el)*sin(az), sin(el), cos(el)*cos(az))
// whose ground projection is the already-normalized (-sin(az), -cos(az)).
// That is the convention build_view_projection_matrix,
// compose_camera_position_from_orbit and the camera-host fly branch all
// share, so one formula is right in FPV, orbit, fly and chase alike.
//
// ONE FRAME STALE by the same law as PointState — the copy is encoded at R11
// and mapped at R1 of the following frame. `valid` is false until the first
// map lands; a reader that cannot tell where the eye is must not conclude
// that nobody is looking.
struct CameraPose {
    float eye[3]    = { 0.0f, 0.0f, 0.0f };
    float azimuth   = 0.0f;
    float elevation = 0.0f;
    bool  valid     = false;
};

struct TimeState {
    float beats   = 0.0f;
    // PLUMB_0 B1 — THE WALL CLOCK ACCUMULATES IN DOUBLE, AND THIS IS WHY.
    //
    // It was a float fed from AnalysisSignal::t_seconds, itself a float
    // accumulator (`signal_.t_seconds += dt` in BeatClock). A float
    // accumulator STOPS when dt falls below half an ulp: measured at
    // 524288.0 s — 4.8 days at 60 fps, sooner at higher refresh. Past that
    // every cooldown stays cooling for ever, every retry stamp is in the
    // future for ever, and anything keyed on an age never matures. The piece
    // is permanently hosted; four days is not a hypothetical.
    //
    // Accumulated at U1 (phase_fill_signal) from signal.dt, which is the
    // first update row and is enabled by a literal true, so this advances on
    // every frame that updates at all. A double at 60 fps is exact past any
    // horizon the exhibition has.
    double seconds = 0.0;
    // THE GPU'S ZERO (PLUMB_0 B2, RULING-3). Shaders take
    // float(seconds - world_epoch), stamped at world birth, so the number
    // they animate on stays young however long the session runs — a float at
    // 5e5 s resolves to 0.03 s, which is visible stepping in a sin(t) twinkle
    // even though the clock behind it is exact.
    //
    // REBASING IS SAFE HERE AND ONLY HERE. It would break any GPU-resident
    // time stamped in one world and differenced in the next; B-G1's census
    // found the three seams that carry one — the frame signal, the orb config
    // and GPURibbonState::time — and all three die at TEARDOWN
    // (teardown_ribbon, teardown_orbs), so no promise crosses the boundary.
    // THE PULSE RING IS THE FOURTH, and since PULSE_1 it is no longer
    // inert: the glass tap and the SPACE key stamp real onsets in this
    // clock. It is cleared in become_destination — the one door every
    // world enters by, and the line above this one — so the promise still
    // does not cross the boundary.
    double world_epoch = 0.0;
    // THE ONE PLACE THE CLOCK NARROWS. Every CPU consumer reads `seconds`;
    // every GPU seam reads this. Two homes for one number is exactly the
    // drift this campaign is closing, so there is one function and the seams
    // call it rather than each doing its own subtraction.
    float gpu_seconds() const { return (float)(seconds - world_epoch); }
    float dt      = 0.016f;
    // Musical tempo follower: beats/sec, HELD-LAST through silence
    // and stopped transport; defaults to 100 BPM (the calibration
    // anchor for the authored idle motion).
    float beat_rate   = 100.0f / 60.0f;
    float prev_beats  = 0.0f;
};

// ═══ PLAYER STATE — THE WITNESS RECORD (v3 §11) ═══════════════════
//
// THE WITNESS CONTRACT, declared and census-checked (the score
// census, Direction W):
//   · THE POINT'S RECORD LEFT THIS STRUCT at POINT_1 — the position
//     mirror (x/z) and the bubble sensor (portal_trigger) live in
//     their semantic home, PointState (contracts/point.hpp), which
//     carries the full authoring law (P5 HARVEST sole author; the
//     TEARDOWN reset and the portal door's consume are the spine's
//     only other touches).
//   · possessed_slot — possession is RE-ANCHORING (v3 §9 Act III:
//     the anchor is a role; the camera is what we control). The
//     writes live behind the agents door (try_possess_nearest,
//     reseed_player_body), paired with the GPU selector.
//   · aura_presence — P8, the pawn is the semantic owner (writes in
//     bodies/pawn.hpp only).
//   · THE CAMERA HAS NO CPU MIRROR — it lives GPU-resident, keyed on
//     config.possessed_slot. The ONE sanctioned window: in
//     CAMERA-HOST the P5 harvest reads camera pos.xz back as the
//     point's position (PointState.x/z) — a two-float harvest, not a
//     mirror. RIBBON_1 added PointState.y/heading, and they are the
//     BODY hosts' alone: the possessed slot's readback authors them, so
//     possess() can capture the pose the body left. The camera still has
//     no y — the witness altitude is GPU-only and is not to be invented.
//   · the rider state LEFT this record per Option A — it lives in
//     RibbonState.sky, which RIBBON_1 reduced to the possess()-staged
//     release request (the eased hand went to the head kernel, where the
//     hand it eases is read); riding ROUTES on the host machine
//     (point_.host == RIBBON — RESIDUE_3, closed player-side).
//
// SEAM[spine:P8] PlayerState commented "Future (deferred)" fields
//   are explicit latent infrastructure: aura_presence is live here;
//   the other deferred fields await the unified entity layer.
//   Pattern P8 visible in source.
struct PlayerState {
    uint32_t possessed_slot = 0;   // slot in agent_state[] that the player inhabits

    // ── Camera ──
    bool    fpv_mode = false;                // first-person view toggle
    // (The point's position mirror + bubble sensor moved HOME at
    //  POINT_1: PointState.x/z/portal_trigger — contracts/point.hpp.)

    // ── Aura presence (closes SEAM[spine:P8]) ──
    float aura_presence = 0.0f;                  // pawn aura ramp (was pawn_state_.aura_presence)

    // Future (deferred):
    //   uint32_t active_couplings;         // COUPLING_* bitmask owned by player
};

// ═══ TRANSITION PHASE ════════════════════════════════════════════
// The transition machine's phase enum. The MACHINE (transitionPhase_,
// pendingDestination_ and kin) stays spine-owned orchestration
// (SEAM[spine:transitions], K4); the enum TYPE lives here so its two
// module readers (mood's request door, agents' possession guard) name
// it unqualified instead of paying the Cartridge:: tax.
enum class TransitionPhase { IDLE, FADE_OUT, TEARDOWN, FADE_IN };

// ═══ INPUT STATE — THE DRIVER'S INTENT ORGAN ══════════════════════
// Type at the contract tier, instance at the root. The
// driver WRITES it (the callbacks + update_movement_intent); the
// spine's signal fill and the ribbon's sky flight READ it (v3 §9
// Act I: drivers write intents; bodies translate them). KeyState /
// MouseState stay with input — they are the driver's private organs;
// this record is the intent CHANNEL the bodies consume.
struct InputState {
    float move_x = 0.0f;
    float move_z = 0.0f;
    float look_az_delta = 0.0f;
    float look_el_delta = 0.0f;
    float zoom_delta = 0.0f;
    float pan_x_delta = 0.0f;
    float pan_y_delta = 0.0f;
    // PULSE_1 — THE TAP'S INTENT, not a delta: an EDGE, raised by the
    // glass tap and the SPACE key, spent exactly once by the frame
    // (phase_live_card_write) and therefore NOT cleared by
    // clear_input_deltas. It is a bool rather than a count because the
    // glass debounce is 80 ms and the key is a down-edge, so two taps
    // cannot reach one frame; a musician wanting both would call
    // emit_radial_pulse directly, which is what the bus is for.
    bool  pulse_pending = false;
};


struct WorldState;   // contracts/surface_services.hpp — the request door reads active_seed (fwd: reference param)

// ═══ MOOD STATE (the spine's mood organ; instance at the root) ════
// Type at the contract tier; the instance was ALWAYS spine-resident with the
// transition machine (SEAM[spine:transitions], K4). The boot value is
// authored at the composition root — no include-order cable.
struct MoodState {
    // ── Currently active mood ──
    uint32_t active = 0;  // drawn at the composition root (Cartridge ctor) under the destination law (USHER_0)

    // ── Mood-applied values (authored by apply_mood, boot included) ──
    // 0 is deliberate: if apply_mood ever failed to run, the sun goes out and
    // the failure is visible on frame 1 rather than hiding behind mood 0's
    // values. Fails loud.
    float sun_intensity = 0.0f;
    float sun_ambient   = 0.0f;
    uint32_t regime = 0;                    // REGIME_1 — which Regime this world was drawn into:
                                            // the WORLD's fact, rolled by apply_mood_regime from the
                                            // mood's regime_weight; every subscriber indexes its own
                                            // columns with it
    // ATMOS_1 — the fog's REST, drawn per world from the mood's atmosphere.
    // The U4 seam (phase_motion_drivers) composes the canvas's deviation
    // over it every frame. 0 is the same fails-loud choice as the sun's:
    // if the draw never ran, the world is fogless and black-fogged on
    // frame 1 rather than quietly wearing the sunset's.
    float fog_rest_density  = 0.0f;
    float fog_rest_color[3] = { 0.0f, 0.0f, 0.0f };
    float terrain_amp_ceiling = 0.0f;       // mirrors GPU config.terrain_amp_ceiling
    bool  spot_light_active = false;

    // ── Transition machinery ──
    float transition_timer         = 0.0f;
    float transition_fade_duration = 0.5f;  // seconds per fade direction
    float transition_fade_alpha    = 0.0f;

    // ── Portal upload flag ──
    bool portals_dirty = true;              // true at boot → first upload guaranteed

    // ── Back-portal return state ──
    bool     back_portal_pending       = false;
    // ATRIUM_0 — the forwards no longer hang off the return. Raised at every
    // finite birth (boot and transition); the back is raised only where
    // something precedes. Consumed at the same population site, back first.
    bool     forward_portals_pending   = false;
    // One-shot arm for the door-guarantee fallback (U2). TRUE at boot
    // (the boot world runs no teardown) and re-armed at every teardown;
    // consumed by the world's FIRST fullRegen. NOT tied to fullRegen
    // itself: request_recenter re-arms fullRegen mid-world (the
    // render-radius keys), and the guarantee is AT POPULATION only.
    bool     door_fallback_pending     = true;
    uint32_t back_portal_return_seed   = 0;
    uint32_t back_portal_return_mood   = 0;
    uint32_t back_portal_return_radius = 2;

    // ── Sun orbit (musical coupling) ──
    float sun_orbit_phase = 0.0f;

    // ── Light re-upload flag (re-homed from entities_state_:
    //    mood was both producer and consumer — the organ was wrong,
    //    not the channel). Set true at init/teardown/apply, cleared
    //    after upload. ──
    bool lights_dirty = true;
};

// ═══ MOOD SYSTEM (vocabulary) ════════════════════════════════════
//
// A MOOD IS A SHAPE WEARING AN ATMOSPHERE (ATMOS_1). The shape is what
// a world IS — read by generation, torn down with the world, never
// re-spoken. The atmosphere is what it WEARS — a DISTRIBUTION, not a
// point: centres and spreads, and a light regime drawn from weighted
// tiers. At every apply the world's seed draws ONE atmosphere from it
// (draw_atmosphere, direction/mood.hpp); the same seed draws the same
// sky, so the back portal's promise holds. The panel writes the
// distribution, and the draw moves WITH the dial rather than re-rolling.
//
// THE PERSISTENCE LADDER — every parameter stands on one rung, and the
// rung says who takes it back and when (docs/ORGAN.md, "The persistence
// ladder"):
//   1 the instrument's registration — the LIVE banks; held through all
//   2 the player's preferences — seeded once, then the player's; held
//   3 the environment's instance — authored by apply_mood at entry;
//     re-spoken at the boundary when rung 1 is edited
//   4 the world's draw — (seed, tables) → terrain, spawns, THIS SKY;
//     reborn at teardown, the same seed the same world
//   5 the drivers' output — rest (3) + gain (1) · deviation, per frame
//   6 the live simulation — advances per frame; a discrete command
//     changes the LAW, not the state; reborn only at teardown
// A transition holds 1-2, re-speaks 3, reborns 4 and 6; 5 continues over
// the new rest. "Held regardless" is rungs 1 and 2; "a custom
// environment" is a rung-1 row plus its rung-4 draw.

enum class CeilingType : uint32_t {
    NONE = 0,   // outdoor — no shell geometry
    FLAT = 1,   // flat slab ceiling
    VAULT = 2,   // catenary vault ceiling
};


// ATRIUM_5 — THE TWO PINS' SENTINELS. A shape either lets the seed draw its
// indoor light scheme and its wall palette, or it says which. The atrium
// says which, because "the same room for everyone" is what an entrance is;
// every other shape rolls, as it always did.
inline constexpr uint32_t SCHEME_ROLL  = 0xFFFFFFFFu;
inline constexpr uint32_t PALETTE_ROLL = 0xFFFFFFFFu;

// THE SHAPE — what a world IS. Structural by the eligibility rule
// (stated beside MOOD_LIVE below): no field here may take a definition
// target, because world GENERATION reads it, and rewriting it without
// regenerating the world would mean nothing at best and disagree at
// worst.
struct WorldShape {
    bool        finite;              // true = walled world with finite radius
    uint32_t    finite_radius_min;   // min patch radius (when finite)
    uint32_t    finite_radius_max;   // max patch radius (when finite)
    bool        indoor;              // true = enclosed space with ceiling
    CeilingType ceiling_type;        // NONE / FLAT / VAULT
    float       wall_height;         // where the VERTICAL wall stops (world units).
                                     // NOT the ceiling on VAULT — there the ceiling is
                                     // the crown, 47-92 against this 25. See vault_crown.
    float       terrain_amp_ceiling; // indoor terrain-amp cap (0 = uncapped, outdoor)
    bool        allow_gol_zones;     // GoL zone spawning + visualization
    bool        allow_pawn_aura;     // toroidal spring grid tinting + height boost
    bool        allow_frustum_cull;  // STATUS: LATENT[mood_cull_opt_out] — INERT.
                                     // Reaches renderer_.set_frustum_cull_active
                                     // and stops: the flag's reader was retired
                                     // when the draw plan took every mood, so
                                     // indoor terrain IS culled despite the two
                                     // `false` rows below. See renderer.hpp for
                                     // the full note and the cut (OPT_1 O0-f).
    uint32_t    light_scheme;        // ATRIUM_5 — SCHEME_ROLL: the seed rolls (scheme_weights);
                                     // else the scheme id, pinned. Structural: drawn at apply.
    uint32_t    palette;             // ATRIUM_5 — PALETTE_ROLL: the seed rolls INDOOR_PALETTES;
                                     // else the row index, pinned.
};
// The open field, by property: neither walled nor roofed. The triad's
// way out asks this; no id is kept for it, because the two flags already
// say it.
inline constexpr bool shape_is_open(const WorldShape& s) {
    return !s.finite && !s.indoor;
}

// A REGIME (ATMOS_2) — one of up to four weighted rows a world may be
// drawn into, and a WHOLE SKY: every parameter the draw reads, except
// the sun's bearing, has its centre and its spread here. Regimes make a
// sky MULTIMODAL: a moonless-and-clear night and a moonlit-and-thick one
// are two regimes, not two ends of one smear — and because the light and
// the fog sit in one row, "the light according to this fog" is authored
// by writing the row, not by coupling two tables. The weights are the
// MOOD's (MoodProfile::regime_weight — REGIME_1): a regime row is what a
// world wears; how often the mood draws it is what the mood is. Weight 0
// there is an absent regime. A parameter the operator wants the same in
// every regime is set
// equal in every regime (a per-parameter "mood-wide" flag is priced in
// OPEN.md, not built).
//
// A colour's spread is a ± on BRIGHTNESS over the whole triple, hue
// kept (0.2 = ±20%): a darker or lighter draw of the same thing.
struct Regime {
    float sun_color[3];          // sun RGB — the centre
    float sun_color_spread;      // ± brightness
    float intensity;             // diffuse strength — the centre
    float intensity_spread;      // ± around it, uniform
    float ambient;               // ambient fill strength — the centre
    float ambient_spread;        // ± around it, uniform
    float fog_density;           // the REST the drivers' seam composes over — the centre
    float fog_density_spread;    // ± around it, uniform
    float fog_color[3];          // the rest colour — the centre
    float fog_color_spread;      // ± brightness
    float clear_color[3];        // sky or dark ceiling RGB — the centre
    float clear_color_spread;    // ± brightness
};
inline constexpr uint32_t REGIME_COUNT = 4;

// THE ATMOSPHERE — what a world WEARS, as a distribution. Every "spread"
// is a uniform ± around its centre; spread 0 draws the centre EXACTLY
// (no hash is taken), which is what keeps a carried row bit-identical
// to the point value it replaced. A colour's spread is a ± on
// brightness, and spread 0 multiplies by exactly 1.0f.
struct Atmosphere {
    // ─── The sun's bearing — the mood's, not a regime's ───────
    float  sun_direction[3];        // the light vector's CENTRE — the direction light
                                    // travels; its readers normalize it
    float  sun_az_spread_deg;       // ± azimuth turn about +Y, degrees (180 = any bearing)
    float  sun_el_spread_deg;       // ± elevation, degrees; the draw clamps elevation to [5°, 88°]
    // ─── The regimes — the seed picks one; it is the whole sky ──
    Regime regime[REGIME_COUNT];
    // wall/ceiling colors: INDOOR_PALETTES (mood.hpp) is the authority —
    // seed-picked per world; the profile never authored them in effect.
};

struct MoodProfile {
    WorldShape shape;                      // what the world is
    Atmosphere atmos;                      // what it wears
    float      regime_weight[REGIME_COUNT];// how often it wears each regime (REGIME_1) —
                                           // the world's roll; 0 = that regime is absent
};

// ═══ THE SHAPES ══════════════════════════════════════════════════
// One authored home per shape. Three moods wear SHAPE_OPEN, and that
// they are one stage is stated by this constant, not by three copies.
//                                              fin    r_min r_max indoor ceil                wall_h amp_c  zones aura  cull   scheme         palette
inline constexpr WorldShape SHAPE_OPEN       = { false, 2,    2,    false, CeilingType::NONE,  0.0f,  0.0f,  true, true, true,  SCHEME_ROLL,   PALETTE_ROLL };
inline constexpr WorldShape SHAPE_ROOM_FLAT  = { true,  1,    3,    true,  CeilingType::FLAT,  20.0f, 0.5f,  true, true, false, SCHEME_ROLL,   PALETTE_ROLL };
inline constexpr WorldShape SHAPE_ROOM_VAULT = { true,  1,    3,    true,  CeilingType::VAULT, 25.0f, 0.5f,  true, true, false, SCHEME_ROLL,   PALETTE_ROLL };
inline constexpr WorldShape SHAPE_FINITE     = { true,  1,    4,    false, CeilingType::NONE,  0.0f,  0.0f,  true, true, true,  SCHEME_ROLL,   PALETTE_ROLL };
// THE ATRIUM'S SHAPE (ATRIUM_1). Radius pinned (min == max, no roll): every
// visitor's first room is the same room. No GoL — the floor is for the images
// and the passers. Flat ceiling, the flat room's wall. The roster is the ARC
// (ATRIUM_2): one door per other mood, not PORTAL_2's triad.
//
// ATRIUM_13 — THE LAMPS ARE DRAWN, NOT AUTHORED (Jean). The entrance PINS the
// scheme so the count is four and does not roll; everything else about the
// lamps — aim, intensity, placement — is QUARTET's spreads under the world's
// seed, exactly as any indoor room. The entrance had a scheme of its own whose
// whole content was "four, straight down, nothing drawn", and that is the
// sentence being retracted; what is left after the retraction is QUARTET,
// which already says the rest better.
//
// THIS IS THE ONE THING IN THE ENTRANCE THAT NOW VARIES BETWEEN VISITS. The
// walls, the floor, the camera, the arc, the palette and the images are all
// pinned; the light is not. That is the trade "as usual" buys, and SCHEME_ROLL
// is the word that would take the COUNT with it — which is why the pin stays
// and only the index moves. Same column, same clamp, no new mechanism.
//
// ATRIUM_12 — AND THE FLOOR IS PINNED TOO, at 0.01 and NOT at 0. Every other
// room rolls its ground and the poster's fit was a hope: the CPU cannot
// sample an indoor floor (the seat is a GPU pass), so the headroom witness
// could only ever print the flat-floor answer and call it clearance. Near-flat
// makes that answer true to within a bound the arithmetic can state, and it
// keeps the camera's eye out of a rise — which at elevation -18, where the
// composed eye is already under the floor and riding the terrain clamp, is
// not free. The last rolled thing in the entrance, and the entrance is always
// as it is.
//
// ZERO WOULD HAVE DONE THE OPPOSITE. terrain_amp_ceiling is a per-wave
// amplitude CAP applied as `if (config.terrain_amp_ceiling > 0.0) { amp =
// min(amp, ceiling); }` (world.wgsl, evaluate_lattice_wave_pair), so 0 is the
// OUTDOOR sentinel — uncapped — and writing it here would have made the
// entrance the most rolled indoor room in the program. 0.01 is the smallest
// number that still ARMS the cap.
//
// AND "FLAT" IS A BOUNDED CLAIM, NOT AN ABSOLUTE. The cap governs the wave
// term only: six bands, each a bilinear blend over four lattice nodes whose
// weights sum to 1, so |wave sum| <= 6 * 0.01 = 0.06 wu. The other
// contributors to the walker's ground are untouched and named here so the
// claim cannot be mistaken for more than it is — GoL zones (already off for
// this shape, the `false` below), the baked pyramids inside
// sample_terrain_y_at, the live card's radial pulses, and the pawn's own aura
// dome, which is a floor by Jean's ruling and is meant to lift the eye.
// ATRIUM_5 — THE SMALL ROOM. Radius 1: 3x3 patches, 150 wu a side. The
// bounds are asymmetric by their own formula, [-r*PE, (r+1)*PE] = [-50, 100],
// so the pawn at the origin has 50 wu of room behind it and 100 ahead on each
// axis — the wall behind, the arc ahead, and the long side is +X +Z.
inline constexpr WorldShape SHAPE_ATRIUM     = { true,  1,    1,    true,  CeilingType::FLAT,  20.0f, 0.01f, false, true, false, SCHEME_QUARTET, /* warm charcoal */ 7u };

// ═══ THE ATMOSPHERES ═════════════════════════════════════════════
// The carried rows are the pre-ATMOS_1 MOOD_TABLE values exactly: one
// regime at weight 1 with every spread 0 — so the boot draw is the old
// table, bit for bit (the witnesses below pin it). Their fog rest is
// the old drivers'-room rest, FOG_DENSITY_NONE / FOG_COLOR_NONE
// (coupling/visual_canvas.hpp), which every mood wore before the rest
// came home to the mood.
//
// Row shape:  sun centre, az spread, el spread ·
//             regimes { sun colour, sun± · int, int± · amb, amb± ·
//                       fog density, density± · fog colour, colour± ·
//                       clear colour, clear± }   — {} is absent
//             weights: MOOD_TABLE, per mood
inline constexpr Atmosphere ATMOS_SUNSET = {
    { 0.94f, -0.29f, -0.13f }, 0.0f, 0.0f,
    { { { 1.0f, 0.75f, 0.45f }, 0.0f, 0.90f, 0.0f, 0.20f, 0.0f,
        0.0030f, 0.0f, { 0.85f, 0.78f, 0.72f }, 0.0f, { 0.95f, 0.70f, 0.45f }, 0.0f },   // today's sky, exactly
      {}, {}, {} },
};
// THE TWO ROOMS STOPPED BEING ONE SKY. ATMOS_ROOM was one home for both
// because both wore the same numbers; the desk gave them different ones,
// so one home became two. Neither is a point row any more either — the
// flat's bearing wanders ±14° and its fog has a spread, the vault's
// light has one — which is why the carry witness below now names only
// the two rows that still carry.
//
// BOTH SUNS CAME A LITTLE OFF AMBER on Jean's eye, after the export and
// against it. RED DOES NOT MOVE — it is each colour's brightest channel
// and already near the ceiling — and green and blue come about a third
// of the way up to it, so the light DESATURATES rather than brightening.
// The fog colours are untouched: the note was about the light.
inline constexpr Atmosphere ATMOS_ROOM_FLAT = {
    { 0.34f, -0.10f, 0.06f }, 14.0f, 0.0f,          // low, ~16° up; ±14° of bearing, no elevation spread
    { { { 0.9843137f, 0.850f, 0.720f }, 0.0f, 1.15f, 0.0f, 0.11f, 0.0f,
        0.0024f, 0.0019f, { 0.85882354f, 0.58431375f, 0.36078432f }, 0.0f, { 0.15f, 0.12f, 0.10f }, 0.0f },
      {}, {}, {} },
};
inline constexpr Atmosphere ATMOS_ROOM_VAULT = {
    { -0.15f, -0.67f, -0.37f }, 0.0f, 0.0f,         // steep, ~59° up; one bearing, no spread
    { { { 0.91764706f, 0.780f, 0.650f }, 0.0f, 0.69f, 0.16f, 0.115f, 0.0f,
        0.0012f, 0.0009f, { 0.99607843f, 0.7490196f, 0.54509807f }, 0.0f, { 0.15f, 0.12f, 0.10f }, 0.0f },
      {}, {}, {} },
};
inline constexpr Atmosphere ATMOS_FINITE_DAY = {
    { 0.56f, -0.82f, -0.11f }, 0.0f, 0.0f,
    { { { 1.0f, 0.95f, 0.90f }, 0.0f, 0.80f, 0.0f, 0.25f, 0.0f,
        0.0030f, 0.0f, { 0.85f, 0.78f, 0.72f }, 0.0f, { 0.85f, 0.78f, 0.72f }, 0.0f },
      {}, {}, {} },
};

// ═══ THE TWO NEW SKIES (ATMOS_1, regimes at ATMOS_2) ═════════════
// BOTH SKIES ARE TUNED NOW — no number below is a sketch any more; each
// came back from the panel and was transcribed here at SHIP TIME
// (docs/ORGAN.md, "Presets"). Both collapsed the same way: every regime
// ROW survives, and regime 0 alone carries weight (MOOD_TABLE, below), so
// each mood draws ONE sky — the night a hard moon over air that is clear
// unless the fog's spread finds it, the noon a high clear day. The rest
// are ABSENT, not deleted; that is what a weight of 0 means, and giving
// one weight back brings its row back unchanged.
//
// THE NIGHT'S FOG CENTRE IS 0 WITH A SPREAD OF 0.0011. The draw is
// max(0, centre + jitter) and the jitter is ±spread, so half the seeds
// get no fog at all and the other half get up to 0.0011 — a rectified
// draw, and the only place in the table where a centre sits on the floor
// of its own distribution. THE SPREAD IS THE CEILING here: with the
// centre at 0 it is the whole of what "how thick can this night get"
// means, which is why halving the thickest night was halving this one
// number (0.0022 -> 0.0011) and nothing else.
inline constexpr Atmosphere ATMOS_NIGHT = {
    { 0.38f, -0.44f, 0.15f }, 3.0f, 3.0f,          // moon centre ~47° up; ±3° of bearing; ±3°
    { { { 0.76f, 0.80f, 0.79f }, 0.66f, 0.85f, 0.12f, 0.05f, 0.02f,          // THE DRAWN ROW — a hard moon over
        0.0f,    0.0011f, { 0.11f, 0.12f, 0.15f }, 0.0f,  { 0.02f, 0.03f, 0.06f }, 0.25f },  // air that is clear by default
      { { 0.72f, 0.80f, 1.00f }, 0.05f, 1.12f, 0.32f, 0.055f, 0.03f,         // moonlit & hazy — weight 0
        0.0048f, 0.0010f, { 0.05f, 0.06f, 0.10f }, 0.15f, { 0.03f, 0.04f, 0.08f }, 0.15f },
      { { 0.80f, 0.86f, 1.00f }, 0.05f, 0.55f, 0.10f, 0.14f, 0.03f,          // bright moon & clear — weight 0
        0.0020f, 0.0005f, { 0.04f, 0.05f, 0.09f }, 0.10f, { 0.04f, 0.05f, 0.10f }, 0.10f },
      { { 0.72f, 0.80f, 1.00f }, 0.05f, 1.81f, 0.00f, 0.135f, 0.02f,         // moonless & thick — weight 0
        0.0168f, 0.0020f, { 0.09f, 0.09f, 0.12f }, 0.10f, { 0.02f, 0.02f, 0.04f }, 0.10f } },
};
inline constexpr Atmosphere ATMOS_NOON = {
    { 0.24f, -0.88f, -0.20f }, 40.0f, 8.0f,        // sun centre ~70° up; ±40° bearing; ±8°
    { { { 1.00f, 0.98f, 0.92f }, 0.05f, 1.15f, 0.10f, 0.19f, 0.03f,          // THE DRAWN ROW — clear
        0.0004f, 0.0004f, { 0.78f, 0.86f, 0.97f }, 0.05f, { 0.45f, 0.68f, 0.95f }, 0.08f },
      { { 1.00f, 0.98f, 0.92f }, 0.0f, 1.00f, 0.10f, 0.38f, 0.05f,           // hazy — weight 0
        0.0036f, 0.0008f, { 0.86f, 0.90f, 0.96f }, 0.05f, { 0.62f, 0.76f, 0.94f }, 0.08f },
      {}, {} },
};

// ═══ THE ATRIUM'S SKY (ATRIUM_1) ═════════════════════════════════
// A POINT, ON PURPOSE — the atrium draws the same sky for everyone; Jean
// tunes it on the desk (?organ=1&mood=6) and the export lands here. Every
// spread is 0 and both bearing spreads are 0, so no hash is taken and the
// draw is these numbers exactly. The centres are the flat room's regime 0
// — the atrium wears the flat room's wall, so it wears the flat room's
// light — with the intensity, the ambient and the clear colour each halved:
// a small DARK room, the images and the doors carrying it. The fog centre
// is unchanged, and only its spread goes: fog is the room's depth, not its
// brightness. Regimes 1-3 are absent.
// ATRIUM_14 — NO FOG IN THE ENTRANCE (Jean). The rest goes 0.0024 -> 0. The
// entrance is a small room whose whole job is to be READ: the controls poster
// at 15 wu, the arc's doors at 38, and a haze over either is a haze over the
// lesson. Every other room keeps its own.
//
// AND ZERO HERE IS ZERO ON SCREEN, at rest. The live value is
// max(0, fog_rest_density + gain * deviation) at the U4 seam, and the canvas
// writes the deviation as FOG_BY_FIELD[idx] - FOG_BY_FIELD[0] — a difference
// from field 0, so it is exactly 0 until the music changes field. The mood's
// number is therefore the whole of the fog in a quiet room; the drivers'
// gain is global and stays every mood's, not this one's to silence.
//
// THE FOG COLOUR STAYS AUTHORED. At density 0 nothing reads it, and a rest
// of black would be a second, invisible decision to unpick the day a hand or
// the music lifts the density off the floor.
inline constexpr Atmosphere ATMOS_ATRIUM = {
    { 0.34f, -0.10f, 0.06f }, 0.0f, 0.0f,           // the flat room's bearing; no spread, either axis
    { { { 0.9843137f, 0.850f, 0.720f }, 0.0f, 0.575f, 0.0f, 0.055f, 0.0f,
        0.0f, 0.0f, { 0.85882354f, 0.58431375f, 0.36078432f }, 0.0f, { 0.075f, 0.06f, 0.05f }, 0.0f },
      {}, {}, {} },
};

// ═══ MOOD DEFINITIONS ════════════════════════════════════════════
//
// SEAM[mood:K1] indoor/outdoor binary lives in WorldShape as bool
//   `finite` + bool `indoor`. finite_outdoor is walled AND outdoor, so
//   it sits astride the binary and the encoding doesn't survive contact
//   — correct for today but worth re-examining when finite_outdoor
//   design lands.
//                                      shape             atmosphere
inline constexpr MoodProfile MOOD_TABLE[MOOD_COUNT] = {
    //                                shape             atmosphere        regime weights (REGIME_1)
    /* MOOD_OPEN_SUNSET        */  { SHAPE_OPEN,       ATMOS_SUNSET,     { 1.0f, 0.0f,  0.0f,  0.0f  } },
    /* MOOD_INDOOR_FLAT        */  { SHAPE_ROOM_FLAT,  ATMOS_ROOM_FLAT,  { 1.0f, 0.0f,  0.0f,  0.0f  } },
    /* MOOD_INDOOR_VAULT       */  { SHAPE_ROOM_VAULT, ATMOS_ROOM_VAULT, { 1.0f, 0.0f,  0.0f,  0.0f  } },
    /* MOOD_FINITE_OUTDOOR     */  { SHAPE_FINITE,     ATMOS_FINITE_DAY, { 1.0f, 0.0f,  0.0f,  0.0f  } },
    /* MOOD_OPEN_NIGHT         */  { SHAPE_OPEN,       ATMOS_NIGHT,      { 1.0f, 0.0f,  0.0f,  0.0f  } },
    /* MOOD_OPEN_NOON          */  { SHAPE_OPEN,       ATMOS_NOON,       { 1.0f, 0.0f,  0.0f,  0.0f  } },
    /* MOOD_ATRIUM             */  { SHAPE_ATRIUM,     ATMOS_ATRIUM,     { 1.0f, 0.0f,  0.0f,  0.0f  } },
};

// F-3: MOOD_TABLE rows are POSITIONAL in
// mood-id order and carry no id field (the CUBE_POPULATIONS-style
// per-row assert is impossible here) — so pin the ids AT the table:
// drift in mood_constants.hpp fails HERE, where the rows live.
static_assert(MOOD_OPEN_SUNSET  == 0 && MOOD_INDOOR_FLAT    == 1
           && MOOD_INDOOR_VAULT == 2 && MOOD_FINITE_OUTDOOR == 3
           && MOOD_OPEN_NIGHT   == 4 && MOOD_OPEN_NOON      == 5
           && MOOD_ATRIUM       == 6
           && MOOD_COUNT == 7,
    "MOOD_TABLE rows are positional in mood-id order (F-3): "
    "reorder the table together with the ids");

// COLUMN WITNESSES. F-3 pins ROW order; these pin COLUMN offsets. The
// shapes and the atmospheres are positionally brace-initialised, so a
// column added or cut mid-row shifts every field after it with no
// diagnostic. One probe per region of each row — head, middle, tail —
// so a shift anywhere trips. light_scheme / palette are WorldShape's last
// two fields and take the tail probe (ATRIUM_5); allow_frustum_cull, the
// field they displaced, keeps a probe of its own one step in, and no two
// of the three agree at the same rows, so none names what another does.
static_assert(MOOD_TABLE[MOOD_OPEN_SUNSET].shape.finite         == false, "WorldShape column drift: finite (head)");
static_assert(MOOD_TABLE[MOOD_FINITE_OUTDOOR].shape.finite      == true,  "WorldShape column drift: finite (head)");
static_assert(MOOD_TABLE[MOOD_OPEN_SUNSET].shape.indoor         == false, "WorldShape column drift: indoor (middle)");
static_assert(MOOD_TABLE[MOOD_INDOOR_VAULT].shape.indoor        == true,  "WorldShape column drift: indoor (middle)");
static_assert(MOOD_TABLE[MOOD_INDOOR_FLAT].shape.wall_height    == 20.0f, "WorldShape column drift: wall_height");
static_assert(MOOD_TABLE[MOOD_INDOOR_VAULT].shape.wall_height   == 25.0f, "WorldShape column drift: wall_height");
static_assert(MOOD_TABLE[MOOD_OPEN_SUNSET].shape.allow_frustum_cull == true,  "WorldShape column drift: allow_frustum_cull (tail)");
static_assert(MOOD_TABLE[MOOD_INDOOR_FLAT].shape.allow_frustum_cull == false, "WorldShape column drift: allow_frustum_cull (tail)");
static_assert(MOOD_TABLE[MOOD_ATRIUM].shape.finite_radius_min == MOOD_TABLE[MOOD_ATRIUM].shape.finite_radius_max,
    "WorldShape: the atrium's radius is pinned (ATRIUM_1)");
static_assert(MOOD_TABLE[MOOD_ATRIUM].shape.palette == 7u
           && MOOD_TABLE[MOOD_INDOOR_FLAT].shape.palette == PALETTE_ROLL
           && MOOD_TABLE[MOOD_INDOOR_FLAT].shape.light_scheme == SCHEME_ROLL,
    "WorldShape column drift: light_scheme / palette (tail)");
static_assert(MOOD_TABLE[MOOD_OPEN_SUNSET].atmos.sun_direction[0]            == 0.94f,   "Atmosphere column drift: sun_direction (head)");
static_assert(MOOD_TABLE[MOOD_OPEN_SUNSET].atmos.regime[0].sun_color[0]      == 1.0f,    "Atmosphere column drift: regime[0].sun_color");
static_assert(MOOD_TABLE[MOOD_OPEN_SUNSET].atmos.regime[0].intensity         == 0.90f,   "Atmosphere column drift: regime[0].intensity (middle)");
static_assert(MOOD_TABLE[MOOD_FINITE_OUTDOOR].atmos.regime[0].ambient        == 0.25f,   "Atmosphere column drift: regime[0].ambient (middle)");
static_assert(MOOD_TABLE[MOOD_OPEN_SUNSET].atmos.regime[0].fog_density       == 0.0030f, "Atmosphere column drift: regime[0].fog_density");
static_assert(MOOD_TABLE[MOOD_INDOOR_FLAT].atmos.regime[0].clear_color[2]    == 0.10f,   "Atmosphere column drift: regime[0].clear_color");
static_assert(MOOD_TABLE[MOOD_OPEN_NIGHT].atmos.regime[3].fog_density        == 0.0168f, "Atmosphere column drift: regime[3].fog_density");
static_assert(MOOD_TABLE[MOOD_OPEN_NIGHT].atmos.regime[3].clear_color_spread == 0.10f,   "Atmosphere column drift: regime[3].clear_color_spread (tail)");
static_assert(MOOD_TABLE[MOOD_OPEN_SUNSET].regime_weight[0]                == 1.0f,    "MoodProfile column drift: regime_weight (sunset, one regime)");
// EVERY MOOD WEIGHTS ONE REGIME TODAY, so no lane anywhere carries a
// distinctive value and a probe on lane 1, 2 or 3 could only expect 0.
// The head probe above still pins lane 0, the probe below still READS
// lane 3, and the per-lane work is done by mood_carries_point, which
// checks all four lanes on the rows that must carry a point.
static_assert(MOOD_TABLE[MOOD_OPEN_NIGHT].regime_weight[3]                 == 0.0f,    "MoodProfile column drift: regime_weight (night, tail)");

// The open family is one stage: three moods, one SHAPE_OPEN, stated once.
static_assert(shape_is_open(MOOD_TABLE[MOOD_OPEN_SUNSET].shape)
           && shape_is_open(MOOD_TABLE[MOOD_OPEN_NIGHT].shape)
           && shape_is_open(MOOD_TABLE[MOOD_OPEN_NOON].shape)
           && !shape_is_open(MOOD_TABLE[MOOD_FINITE_OUTDOOR].shape)
           && !shape_is_open(MOOD_TABLE[MOOD_INDOOR_FLAT].shape),
    "ATMOS_1: sunset, night and noon wear the open shape; the rooms and the finite field do not");

// THE CARRY WITNESS (ATMOS_1). A carried row draws its old point value
// exactly only if every spread is 0 and regime 0 holds the whole weight;
// draw_atmosphere short-circuits on exactly those conditions. A spread
// or a second regime on one of the rows named below is a design change,
// not a carry — make it on purpose and take the row off this list.
// The predicate takes the mood BY PARAMETER and the four rows are
// NAMED at the assert: MOOD_TABLE is mentioned only inside the
// static_assert, which is what keeps the design table's reader census
// (tools/organ_gap.py) reading this proof as the proof it is.
inline constexpr bool mood_carries_point(const MoodProfile& m) {
    const Atmosphere& a = m.atmos;
    return a.sun_az_spread_deg == 0.0f && a.sun_el_spread_deg == 0.0f
        && a.regime[0].sun_color_spread == 0.0f
        && a.regime[0].intensity_spread == 0.0f && a.regime[0].ambient_spread == 0.0f
        && a.regime[0].fog_density_spread == 0.0f && a.regime[0].fog_color_spread == 0.0f
        && a.regime[0].clear_color_spread == 0.0f
        && m.regime_weight[0] == 1.0f
        && m.regime_weight[1] == 0.0f && m.regime_weight[2] == 0.0f && m.regime_weight[3] == 0.0f;
}
// TWO OF THE FOUR STOPPED CARRYING, ON PURPOSE. The desk gave both rooms
// distributions — the flat a bearing spread and a fog spread, the vault a
// light spread and a fog spread — so they no longer draw a point and the
// witness cannot name them without failing. It still names the two rows
// nobody has tuned, which is the whole of what it was ever proving: a row
// that was a point before ATMOS_1 draws that point still.
static_assert(mood_carries_point(MOOD_TABLE[MOOD_OPEN_SUNSET])
           && mood_carries_point(MOOD_TABLE[MOOD_FINITE_OUTDOOR])
           && mood_carries_point(MOOD_TABLE[MOOD_ATRIUM]),
    "ATMOS_1 carry witness: the untuned pre-ATMOS_1 rows must draw their old "
    "point values exactly; the atrium draws a point by design");

// ═══ THE MOOD DEFINITION IN FORCE (O1b) ══════════════════════════
// MOOD_TABLE above is the DESIGNED definition: constexpr, asserted,
// the value the program ships with. MOOD_LIVE is the definition IN
// FORCE — seeded from the design when the program loads, and the one
// thing every RUNTIME reader reads. The split exists because a panel
// that can only write the INSTANCE writes something the next mood
// apply takes back; to change what a mood MEANS, the panel has to
// reach the definition, and a constexpr table is not reachable.
//
// ONE HOME, NOT TWO. MOOD_TABLE keeps exactly two jobs — seeding this
// array and standing under the asserts above — and no runtime reader
// is left pointing at it. The constexpr readers in bodies/gallery.hpp
// are the deliberate exception: they are compile-time budgets derived
// from the DESIGN, and a wall's geometry allowance is not a live dial.
//
// THE ELIGIBILITY RULE (docs/ORGAN.md, "Instance and definition").
// A field may take a definition target in the ORGAN registry only if
// the mood apply is its ONLY runtime reader. The whole of `atmos`
// passes: apply_mood_lighting is the one place it is read (through
// draw_atmosphere), and re-running it is how a change to it lands. The
// whole of `shape` does not: it is read all over world GENERATION, and
// rewriting it without regenerating the world would mean nothing at
// best and disagree at worst.
inline MoodProfile MOOD_LIVE[MOOD_COUNT] = {
    MOOD_TABLE[0], MOOD_TABLE[1], MOOD_TABLE[2], MOOD_TABLE[3],
    MOOD_TABLE[4], MOOD_TABLE[5], MOOD_TABLE[6],
};
static_assert(MOOD_COUNT == 7,
    "MOOD_LIVE is seeded row by row (constexpr copy, one per mood): "
    "a new mood needs its row here as well as in MOOD_TABLE");

// The one runtime door onto a mood's fields. Non-const because the
// panel writes through it; every reader takes it by const reference.
// ATTIC_ATRIUM — AN OUT-OF-RANGE ID FAILS LOUDLY, IT DOES NOT ALIAS. The
// wrap was `% MOOD_COUNT`, which landed a stale id silently on mood 0 —
// filed in docs/OPEN.md as an alias-rather-than-refusal for as long as
// nothing could produce a stale id. Deleting a mood is what makes one
// producible: a preset exported before the deletion keys its definitions
// "<mood>/<param>", and importing it after would have written the retired
// mood's rows onto the sunset's without a word. It still returns a
// reference, so the refusal is a clamp and a line, not a throw — but the
// line names the id, once per id, and the value it lands on.
inline MoodProfile& mood_def(uint32_t mood) {
    if (mood >= MOOD_COUNT) {
        static bool warned[64]{};
        const uint32_t slot = mood & 63u;
        if (!warned[slot]) {
            warned[slot] = true;
            std::cout << "[Mood] REFUSED an out-of-range mood id " << mood
                      << " (MOOD_COUNT is " << MOOD_COUNT
                      << "); reading mood 0 instead — a stale preset or URL\n";
        }
        return MOOD_LIVE[0];
    }
    return MOOD_LIVE[mood];
}

// ═══ THE TRANSITION REQUEST DOOR (decl; def rides merged mood.hpp) ═
// The single canonical transition entry point — one door, many keys.
// DEPS-FORM: the driver world holds no MachineCtx; the door
// takes the transition channel explicitly (the deps-form precedent).
void request_mood_transition(TransitionPhase& phase, PortalDestination& pending,
    MoodState& ms, const WorldState& ws, uint32_t mood);

} // namespace the_board
} // namespace t7
