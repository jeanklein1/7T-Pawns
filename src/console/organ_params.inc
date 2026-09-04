// ═══════════════════════════════════════════════════════════════════════
// ORGAN — THE ENROLLMENT LIST
//
// organ_registry.hpp compiles these lines into kOrganParams[] and the
// compiler takes every offset, so a rename fails the build at the
// enrollment line. This file's order IS the panel's table of contents,
// and audit/ORGAN.md is the generated book over the rows.
//
// EVERY RANGE HERE IS EVIDENCE, NOT TASTE: a wrong range on a dial is
// worse than a missing dial, because the missing one is silent and the
// wrong one lies.
// ═══════════════════════════════════════════════════════════════════════

// ─── ADDING A DIAL IS ONE LINE IN THIS FILE ────────────────────────────
// Nothing else, anywhere. Five forms, and EVERY FORM HAS AN _NS TWIN
// taking the enrolled struct's NAMESPACE first — the plain names forward
// `the_board`, so a line that does not care never sees the parameter:
//
//   ORGAN_PARAM(BLOCK, STRUCT, FIELD, TYPE, MIN, MAX, STEP, GROUP, LABEL)
//   ORGAN_PARAM_GEN(… the same …)      a GENERATIONAL dial: the edit lands
//                                      at the author's next natural event,
//                                      and the row wears `on respawn`
//   ORGAN_PARAM_DEF(… , DEFKIND, DEFSTRUCT, DEFFIELD)
//                                      the same, plus the DEFINITION it writes
//   ORGAN_PARAM_DEFONLY(TYPE, MIN, MAX, STEP, GROUP, LABEL,
//                       DEFKIND, DEFSTRUCT, DEFFIELD)
//                                      a definition with NO INSTANCE:
//                                      preview on it is refused
//   ORGAN_PARAM_RO(BLOCK, STRUCT, FIELD, TYPE, GROUP, LABEL)
//                                      a WITNESS: metered, never written,
//                                      and carrying no range
//
// BLOCK is an ORGAN_BLOCK_* (organ_registry.hpp) and there is no way to
// name anything else, which is the sovereignty boundary. STRUCT is the
// home type the offset is taken in; FIELD is the member as offsetof takes
// it, so `tier_gains[2].color_r` is legal. TYPE is F32 / U32 / BOOL /
// VEC3 / VEC4, and a VEC3 over 0…1 renders as a colour.
//
// ELIGIBILITY: a field is a definition only if its applier is the field's
// ONE runtime reader, and a kind shares a flag when it shares an author.
// DEFKIND is MOOD, TIER, BEHAVIOR or ORB_MOOD; a definition-only row's
// block is a SENTINEL DERIVED FROM ITS KIND.

// GROUP IS A PATH, "Section · Group": the shell splits on the FIRST
// separator and renders two levels, never three. A dial sits under its
// SUBJECT, never under its implementation home. Pose is GPU truth and
// carries no dial — but the WITNESS has behaviour of its own now (how it
// holds a flight, what it pushes aside), and behaviour is a subject like
// any body's, so CAMERA is a section. Its input GRAMMAR stays under
// Interaction, beside the hand that moves it. KEEP THIS FILE ORDERED BY
// SECTION PATH: a section must be CONTIGUOUS, because the shell opens a
// new block whenever the first token changes.

// ─── THE SUN ─────────────────────────────────────────────────────────
// GPULighting::sun is the fragment shader's window, and apply_mood_lighting
// DRAWS it — from the live regime's row, under the world's seed — at every
// mood entry and at every MOOD definition re-speak. So its four rows are
// witnesses: what the draw produced. The sun's BEARING is the mood's and is
// authored here (centre + spreads); its colour, intensity and ambient are the
// regime's and are authored under Atmosphere · Regime N, through the lens.
ORGAN_PARAM_DEFONLY(VEC3, -1.0f, 1.0f, 0.01f, "Sky & Light · Sun", "direction (centre)", MOOD, MoodProfile, atmos.sun_direction)
ORGAN_PARAM_DEFONLY(F32, 0.0f, 180.0f, 1.0f, "Sky & Light · Sun", "azimuth spread (±deg)",   MOOD, MoodProfile, atmos.sun_az_spread_deg)
ORGAN_PARAM_DEFONLY(F32, 0.0f, 45.0f,  0.5f, "Sky & Light · Sun", "elevation spread (±deg)", MOOD, MoodProfile, atmos.sun_el_spread_deg)
ORGAN_PARAM_RO(LIGHTING, GPULighting, sun.direction, VEC3, "Sky & Light · Sun", "direction (drawn)")
ORGAN_PARAM_RO(LIGHTING, GPULighting, sun.color,     VEC3, "Sky & Light · Sun", "colour (drawn)")
ORGAN_PARAM_RO(LIGHTING, GPULighting, sun.intensity, F32,  "Sky & Light · Sun", "intensity (drawn)")
ORGAN_PARAM_RO(LIGHTING, GPULighting, sun.ambient,   F32,  "Sky & Light · Sun", "ambient (drawn)")

// ═══ THE RULE CONSOLE ════════════════════════════════════════════
// A SECTION IS ORGANIZED FOR THE HAND THAT PLAYS IT, NOT THE STRUCT THAT
// STORES IT. The five groups below are the hand's taxonomy: what the sky
// IS (Dome), what populates it (Orbs), what moves all of it (Motion), and
// one group per rule with dials of its own. A group whose name ends
// "<rule> rule" grows a live-rule line, derived from the name itself.

// ─── Sky & Light · Dome ─────────────────────────────────────────────
// ORB_CONSOLE_LIVE (contracts/orb_surface.hpp). Dome radius is a per-frame
// read and lands smooth; base size is baked at seed time and re-speaks.
ORGAN_PARAM(ORBS, OrbConsole, dome_radius, F32, 0.0f, 2000.0f, 5.0f,   "Sky & Light · Dome", "dome radius")
ORGAN_PARAM(ORBS, OrbConsole, base_size,   F32, 0.0f, 12.0f,   0.05f,  "Sky & Light · Dome", "base size")

// ─── Sky & Light · Orbs ─────────────────────────────────────────────
// ORB_MOOD_LIVE (contracts/orb_surface.hpp), definition-only and
// mood-selected. Four of these five re-seed the sky; brightness rides.
ORGAN_PARAM_DEFONLY(BOOL, 0.0f,   1.0f,     1.0f,    "Sky & Light · Orbs", "enabled", ORB_MOOD, OrbMoodConfig, enabled)
ORGAN_PARAM_DEFONLY(U32,  0.0f,   256.0f,   1.0f,    "Sky & Light · Orbs", "count", ORB_MOOD, OrbMoodConfig, count)
ORGAN_PARAM_DEFONLY(U32,  0.0f,   3.0f,     1.0f,    "Sky & Light · Orbs", "palette id", ORB_MOOD, OrbMoodConfig, palette_id)
ORGAN_PARAM_DEFONLY(F32,  0.01f,  2.0f,     0.01f,   "Sky & Light · Orbs", "drag", ORB_MOOD, OrbMoodConfig, drag)
ORGAN_PARAM_DEFONLY(F32,  0.0f,   1.0f,     0.005f,  "Sky & Light · Orbs", "brightness", ORB_MOOD, OrbMoodConfig, brightness)

// ─── Sky & Light · Motion — all rules ───────────────────────────────
// Every row here acts under every rule, so the group carries no live-rule
// line. The four rule drags floor one step off configure_orbs' sentinel.
ORGAN_PARAM(ORBS, OrbConsole, speed_mult,  F32, 0.0f, 4.0f, 0.01f,  "Sky & Light · Motion — all rules", "speed mult")
ORGAN_PARAM(ORBS, OrbConsole, noise_floor, F32, 0.0f, 3.0f, 0.005f, "Sky & Light · Motion — all rules", "noise floor")
ORGAN_PARAM_DEFONLY(F32,  -2.0f,  2.0f,     0.005f,  "Sky & Light · Motion — all rules", "rotation speed (rad/s)", ORB_MOOD, OrbMoodConfig, rotation_speed)
ORGAN_PARAM_DEFONLY(VEC3, -1.0f,  1.0f,     0.01f,   "Sky & Light · Motion — all rules", "rotation axis", ORB_MOOD, OrbMoodConfig, rotation_axis)
ORGAN_PARAM_DEFONLY(F32,  0.02f,  4.0f,     0.02f,   "Sky & Light · Motion — all rules", "drag × brownian rule", ORB_MOOD, OrbMoodConfig, rule_drag_brownian)
ORGAN_PARAM_DEFONLY(F32,  0.02f,  4.0f,     0.02f,   "Sky & Light · Motion — all rules", "drag × orbital rule", ORB_MOOD, OrbMoodConfig, rule_drag_orbital)
ORGAN_PARAM_DEFONLY(F32,  0.02f,  4.0f,     0.02f,   "Sky & Light · Motion — all rules", "drag × frozen rule", ORB_MOOD, OrbMoodConfig, rule_drag_frozen)
ORGAN_PARAM_DEFONLY(F32,  0.02f,  4.0f,     0.02f,   "Sky & Light · Motion — all rules", "drag × flocking rule", ORB_MOOD, OrbMoodConfig, rule_drag_flocking)

// ─── Conductor — THE ORB SKY'S OWN AUTHOR (ORRERY_2) ────────────────
// ORB_CONDUCTOR_LIVE (contracts/orb_conductor.hpp), block CONDUCTOR.
// Every number the conductor speaks is here; its RULES are not — the six
// states' identity and the ceremony that orders them are structural.
// THE ROW-WATCH: an edit to the REIGNING state's row lands on the sky
// within one frame, mid-reign, with no transition to wait for.
// GESTURE's ceiling is the largest registry (flocking, 8). The tick
// clamps per rule (brownian 6, orbital 4), so an over-dial reads that
// rule's last real gesture rather than running off the end.
// JITTER ships at 0 — the metronome — and is the "non regular periods"
// clause waiting for Jean's hand.
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, enabled,      BOOL, 0.0f, 1.0f,  1.0f, "Conductor", "enabled")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, frost_one_in, U32,  1.0f, 64.0f, 1.0f, "Conductor", "frost 1-in-N")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, flock_one_in, U32,  1.0f, 64.0f, 1.0f, "Conductor", "flock 1-in-N")

ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[0].gesture,         U32,  0.0f,   7.0f,     1.0f,   "Conductor · brownian-medium", "gesture")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[0].drag,            F32,  0.0f,   4.0f,     0.05f,  "Conductor · brownian-medium", "brn drag x")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[0].orbital_speed,   F32,  0.0f,   2.0f,     0.01f,  "Conductor · brownian-medium", "orbital speed")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[0].noise,           F32,  0.0f,   3.0f,     0.01f,  "Conductor · brownian-medium", "noise")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[0].speed_mult,      F32,  0.0f,   4.0f,     0.01f,  "Conductor · brownian-medium", "speed mult")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[0].duration_beats,  F32,  1.0f,   128.0f,   1.0f,   "Conductor · brownian-medium", "duration (beats)")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[0].jitter_beats,    F32,  0.0f,   64.0f,    1.0f,   "Conductor · brownian-medium", "jitter (± beats)")

ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[1].gesture,         U32,  0.0f,   7.0f,     1.0f,   "Conductor · brownian-intense", "gesture")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[1].drag,            F32,  0.0f,   4.0f,     0.05f,  "Conductor · brownian-intense", "brn drag x")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[1].orbital_speed,   F32,  0.0f,   2.0f,     0.01f,  "Conductor · brownian-intense", "orbital speed")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[1].noise,           F32,  0.0f,   3.0f,     0.01f,  "Conductor · brownian-intense", "noise")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[1].speed_mult,      F32,  0.0f,   4.0f,     0.01f,  "Conductor · brownian-intense", "speed mult")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[1].duration_beats,  F32,  1.0f,   128.0f,   1.0f,   "Conductor · brownian-intense", "duration (beats)")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[1].jitter_beats,    F32,  0.0f,   64.0f,    1.0f,   "Conductor · brownian-intense", "jitter (± beats)")

ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[2].gesture,         U32,  0.0f,   7.0f,     1.0f,   "Conductor · flocking", "gesture")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[2].drag,            F32,  0.0f,   4.0f,     0.05f,  "Conductor · flocking", "brn drag x")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[2].orbital_speed,   F32,  0.0f,   2.0f,     0.01f,  "Conductor · flocking", "orbital speed")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[2].noise,           F32,  0.0f,   3.0f,     0.01f,  "Conductor · flocking", "noise")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[2].speed_mult,      F32,  0.0f,   4.0f,     0.01f,  "Conductor · flocking", "speed mult")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[2].duration_beats,  F32,  1.0f,   128.0f,   1.0f,   "Conductor · flocking", "duration (beats)")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[2].jitter_beats,    F32,  0.0f,   64.0f,    1.0f,   "Conductor · flocking", "jitter (± beats)")

ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[3].gesture,         U32,  0.0f,   7.0f,     1.0f,   "Conductor · orbital-medium", "gesture")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[3].drag,            F32,  0.0f,   4.0f,     0.05f,  "Conductor · orbital-medium", "brn drag x")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[3].orbital_speed,   F32,  0.0f,   2.0f,     0.01f,  "Conductor · orbital-medium", "orbital speed")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[3].noise,           F32,  0.0f,   3.0f,     0.01f,  "Conductor · orbital-medium", "noise")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[3].speed_mult,      F32,  0.0f,   4.0f,     0.01f,  "Conductor · orbital-medium", "speed mult")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[3].duration_beats,  F32,  1.0f,   128.0f,   1.0f,   "Conductor · orbital-medium", "duration (beats)")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[3].jitter_beats,    F32,  0.0f,   64.0f,    1.0f,   "Conductor · orbital-medium", "jitter (± beats)")

ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[4].gesture,         U32,  0.0f,   7.0f,     1.0f,   "Conductor · orbital-intense", "gesture")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[4].drag,            F32,  0.0f,   4.0f,     0.05f,  "Conductor · orbital-intense", "brn drag x")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[4].orbital_speed,   F32,  0.0f,   2.0f,     0.01f,  "Conductor · orbital-intense", "orbital speed")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[4].noise,           F32,  0.0f,   3.0f,     0.01f,  "Conductor · orbital-intense", "noise")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[4].speed_mult,      F32,  0.0f,   4.0f,     0.01f,  "Conductor · orbital-intense", "speed mult")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[4].duration_beats,  F32,  1.0f,   128.0f,   1.0f,   "Conductor · orbital-intense", "duration (beats)")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[4].jitter_beats,    F32,  0.0f,   64.0f,    1.0f,   "Conductor · orbital-intense", "jitter (± beats)")

ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[5].gesture,         U32,  0.0f,   7.0f,     1.0f,   "Conductor · frozen", "gesture")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[5].drag,            F32,  0.0f,   4.0f,     0.05f,  "Conductor · frozen", "brn drag x")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[5].orbital_speed,   F32,  0.0f,   2.0f,     0.01f,  "Conductor · frozen", "orbital speed")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[5].noise,           F32,  0.0f,   3.0f,     0.01f,  "Conductor · frozen", "noise")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[5].speed_mult,      F32,  0.0f,   4.0f,     0.01f,  "Conductor · frozen", "speed mult")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[5].duration_beats,  F32,  1.0f,   128.0f,   1.0f,   "Conductor · frozen", "duration (beats)")
ORGAN_PARAM(CONDUCTOR, OrbConductorConsole, states[5].jitter_beats,    F32,  0.0f,   64.0f,    1.0f,   "Conductor · frozen", "jitter (± beats)")

// ─── Sky & Light · Orbital rule ─────────────────────────────────────
// One row, and the group's name is what tells the shell to print the
// live-rule line: it acts under ORBITAL and under no other rule.
ORGAN_PARAM_DEFONLY(F32,  0.005f, 1.0f,     0.005f,  "Sky & Light · Orbital rule", "orbital speed (rad/s)", ORB_MOOD, OrbMoodConfig, orbital_base_speed)

// ─── Sky & Light · Flocking rule ────────────────────────────────────
// The seven boids rows, acting under FLOCKING alone; the header carries
// the live-rule line. Floors sit one step off the same sentinel.
ORGAN_PARAM_DEFONLY(F32,  1.0f,   200.0f,   1.0f,    "Sky & Light · Flocking rule", "separation radius", ORB_MOOD, OrbMoodConfig, flock_sep_radius)
ORGAN_PARAM_DEFONLY(F32,  2.0f,   480.0f,   2.0f,    "Sky & Light · Flocking rule", "alignment radius", ORB_MOOD, OrbMoodConfig, flock_align_radius)
ORGAN_PARAM_DEFONLY(F32,  4.0f,   800.0f,   4.0f,    "Sky & Light · Flocking rule", "cohesion radius", ORB_MOOD, OrbMoodConfig, flock_coh_radius)
ORGAN_PARAM_DEFONLY(F32,  0.5f,   120.0f,   0.5f,    "Sky & Light · Flocking rule", "separation weight", ORB_MOOD, OrbMoodConfig, flock_sep_weight)
ORGAN_PARAM_DEFONLY(F32,  0.2f,   32.0f,    0.2f,    "Sky & Light · Flocking rule", "alignment weight", ORB_MOOD, OrbMoodConfig, flock_align_weight)
ORGAN_PARAM_DEFONLY(F32,  0.25f,  60.0f,    0.25f,   "Sky & Light · Flocking rule", "cohesion weight", ORB_MOOD, OrbMoodConfig, flock_coh_weight)
ORGAN_PARAM_DEFONLY(F32,  1.0f,   240.0f,   1.0f,    "Sky & Light · Flocking rule", "max speed", ORB_MOOD, OrbMoodConfig, flock_max_speed)

// ─── Sky & Light · Schemes — EDITS THE NEXT DRAW ────────────────
// WORLD_DRAW_LIVE (contracts/mood_constants.hpp), rolled once as a room is
// built. The weights are probabilities select_tier normalises.
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, scheme_weights[0], F32, 0.0f, 1.0f, 0.005f, "Sky & Light · Schemes", "cathedral")
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, scheme_weights[1], F32, 0.0f, 1.0f, 0.005f, "Sky & Light · Schemes", "quartet")
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, scheme_weights[2], F32, 0.0f, 1.0f, 0.005f, "Sky & Light · Schemes", "gallery")
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, scheme_weights[3], F32, 0.0f, 1.0f, 0.005f, "Sky & Light · Schemes", "sanctum")
// ─── Sky & Light · Portals — EDITS THE NEXT PORTAL ──────────────
// A portal's colour is a fact about its DESTINATION, so the per-mood rows
// are indexed by destination mood and the last is the way back.
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, portal_colors[0][0], VEC3, 0.0f, 1.0f, 0.01f, "Sky & Light · Portals", "to open sunset")
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, portal_colors[1][0], VEC3, 0.0f, 1.0f, 0.01f, "Sky & Light · Portals", "to indoor flat")
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, portal_colors[2][0], VEC3, 0.0f, 1.0f, 0.01f, "Sky & Light · Portals", "to indoor vault")
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, portal_colors[3][0], VEC3, 0.0f, 1.0f, 0.01f, "Sky & Light · Portals", "to finite outdoor")
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, portal_colors[4][0], VEC3, 0.0f, 1.0f, 0.01f, "Sky & Light · Portals", "to open night")
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, portal_colors[5][0], VEC3, 0.0f, 1.0f, 0.01f, "Sky & Light · Portals", "to open noon")
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, portal_colors[6][0], VEC3, 0.0f, 1.0f, 0.01f, "Sky & Light · Portals", "to atrium")
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, portal_color_back[0], VEC3, 0.0f, 1.0f, 0.01f, "Sky & Light · Portals", "back portal")

// ─── Atmosphere · Regimes — THE REGIME LAW ──────────────────────────
// One weight per regime, the MOOD's. The walk normalises, so these are
// proportions and a 0 is an absent regime; a write re-rolls under the seed.
ORGAN_PARAM_DEFONLY(F32, 0.0f, 1.0f, 0.01f, "Atmosphere · Regimes", "draw · regime 1", MOOD, MoodProfile, regime_weight[0])
ORGAN_PARAM_DEFONLY(F32, 0.0f, 1.0f, 0.01f, "Atmosphere · Regimes", "draw · regime 2", MOOD, MoodProfile, regime_weight[1])
ORGAN_PARAM_DEFONLY(F32, 0.0f, 1.0f, 0.01f, "Atmosphere · Regimes", "draw · regime 3", MOOD, MoodProfile, regime_weight[2])
ORGAN_PARAM_DEFONLY(F32, 0.0f, 1.0f, 0.01f, "Atmosphere · Regimes", "draw · regime 4", MOOD, MoodProfile, regime_weight[3])

// ─── Atmosphere · Regime 1-4 ────────────────────────────────────────
// A REGIME IS A WHOLE SKY: every parameter the draw reads, as a centre ±
// a spread. The seed picks one, and a write re-draws the live world's.
ORGAN_PARAM_DEFONLY(VEC3, 0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 1", "sun colour",                    MOOD, MoodProfile, atmos.regime[0].sun_color)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 1", "sun colour spread (±bright)",   MOOD, MoodProfile, atmos.regime[0].sun_color_spread)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 4.0f,  0.01f,   "Atmosphere · Regime 1", "intensity",                     MOOD, MoodProfile, atmos.regime[0].intensity)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 2.0f,  0.01f,   "Atmosphere · Regime 1", "intensity spread",              MOOD, MoodProfile, atmos.regime[0].intensity_spread)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 1.0f,  0.005f,  "Atmosphere · Regime 1", "ambient",                       MOOD, MoodProfile, atmos.regime[0].ambient)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 0.5f,  0.005f,  "Atmosphere · Regime 1", "ambient spread",                MOOD, MoodProfile, atmos.regime[0].ambient_spread)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 0.05f, 0.0002f, "Atmosphere · Regime 1", "fog density",                   MOOD, MoodProfile, atmos.regime[0].fog_density)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 0.02f, 0.0001f, "Atmosphere · Regime 1", "fog density spread",            MOOD, MoodProfile, atmos.regime[0].fog_density_spread)
ORGAN_PARAM_DEFONLY(VEC3, 0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 1", "fog colour",                    MOOD, MoodProfile, atmos.regime[0].fog_color)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 1", "fog colour spread (±bright)",   MOOD, MoodProfile, atmos.regime[0].fog_color_spread)
ORGAN_PARAM_DEFONLY(VEC3, 0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 1", "clear colour",                  MOOD, MoodProfile, atmos.regime[0].clear_color)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 1", "clear colour spread (±bright)", MOOD, MoodProfile, atmos.regime[0].clear_color_spread)

ORGAN_PARAM_DEFONLY(VEC3, 0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 2", "sun colour",                    MOOD, MoodProfile, atmos.regime[1].sun_color)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 2", "sun colour spread (±bright)",   MOOD, MoodProfile, atmos.regime[1].sun_color_spread)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 4.0f,  0.01f,   "Atmosphere · Regime 2", "intensity",                     MOOD, MoodProfile, atmos.regime[1].intensity)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 2.0f,  0.01f,   "Atmosphere · Regime 2", "intensity spread",              MOOD, MoodProfile, atmos.regime[1].intensity_spread)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 1.0f,  0.005f,  "Atmosphere · Regime 2", "ambient",                       MOOD, MoodProfile, atmos.regime[1].ambient)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 0.5f,  0.005f,  "Atmosphere · Regime 2", "ambient spread",                MOOD, MoodProfile, atmos.regime[1].ambient_spread)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 0.05f, 0.0002f, "Atmosphere · Regime 2", "fog density",                   MOOD, MoodProfile, atmos.regime[1].fog_density)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 0.02f, 0.0001f, "Atmosphere · Regime 2", "fog density spread",            MOOD, MoodProfile, atmos.regime[1].fog_density_spread)
ORGAN_PARAM_DEFONLY(VEC3, 0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 2", "fog colour",                    MOOD, MoodProfile, atmos.regime[1].fog_color)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 2", "fog colour spread (±bright)",   MOOD, MoodProfile, atmos.regime[1].fog_color_spread)
ORGAN_PARAM_DEFONLY(VEC3, 0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 2", "clear colour",                  MOOD, MoodProfile, atmos.regime[1].clear_color)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 2", "clear colour spread (±bright)", MOOD, MoodProfile, atmos.regime[1].clear_color_spread)

ORGAN_PARAM_DEFONLY(VEC3, 0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 3", "sun colour",                    MOOD, MoodProfile, atmos.regime[2].sun_color)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 3", "sun colour spread (±bright)",   MOOD, MoodProfile, atmos.regime[2].sun_color_spread)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 4.0f,  0.01f,   "Atmosphere · Regime 3", "intensity",                     MOOD, MoodProfile, atmos.regime[2].intensity)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 2.0f,  0.01f,   "Atmosphere · Regime 3", "intensity spread",              MOOD, MoodProfile, atmos.regime[2].intensity_spread)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 1.0f,  0.005f,  "Atmosphere · Regime 3", "ambient",                       MOOD, MoodProfile, atmos.regime[2].ambient)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 0.5f,  0.005f,  "Atmosphere · Regime 3", "ambient spread",                MOOD, MoodProfile, atmos.regime[2].ambient_spread)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 0.05f, 0.0002f, "Atmosphere · Regime 3", "fog density",                   MOOD, MoodProfile, atmos.regime[2].fog_density)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 0.02f, 0.0001f, "Atmosphere · Regime 3", "fog density spread",            MOOD, MoodProfile, atmos.regime[2].fog_density_spread)
ORGAN_PARAM_DEFONLY(VEC3, 0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 3", "fog colour",                    MOOD, MoodProfile, atmos.regime[2].fog_color)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 3", "fog colour spread (±bright)",   MOOD, MoodProfile, atmos.regime[2].fog_color_spread)
ORGAN_PARAM_DEFONLY(VEC3, 0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 3", "clear colour",                  MOOD, MoodProfile, atmos.regime[2].clear_color)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 3", "clear colour spread (±bright)", MOOD, MoodProfile, atmos.regime[2].clear_color_spread)

ORGAN_PARAM_DEFONLY(VEC3, 0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 4", "sun colour",                    MOOD, MoodProfile, atmos.regime[3].sun_color)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 4", "sun colour spread (±bright)",   MOOD, MoodProfile, atmos.regime[3].sun_color_spread)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 4.0f,  0.01f,   "Atmosphere · Regime 4", "intensity",                     MOOD, MoodProfile, atmos.regime[3].intensity)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 2.0f,  0.01f,   "Atmosphere · Regime 4", "intensity spread",              MOOD, MoodProfile, atmos.regime[3].intensity_spread)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 1.0f,  0.005f,  "Atmosphere · Regime 4", "ambient",                       MOOD, MoodProfile, atmos.regime[3].ambient)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 0.5f,  0.005f,  "Atmosphere · Regime 4", "ambient spread",                MOOD, MoodProfile, atmos.regime[3].ambient_spread)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 0.05f, 0.0002f, "Atmosphere · Regime 4", "fog density",                   MOOD, MoodProfile, atmos.regime[3].fog_density)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 0.02f, 0.0001f, "Atmosphere · Regime 4", "fog density spread",            MOOD, MoodProfile, atmos.regime[3].fog_density_spread)
ORGAN_PARAM_DEFONLY(VEC3, 0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 4", "fog colour",                    MOOD, MoodProfile, atmos.regime[3].fog_color)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 4", "fog colour spread (±bright)",   MOOD, MoodProfile, atmos.regime[3].fog_color_spread)
ORGAN_PARAM_DEFONLY(VEC3, 0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 4", "clear colour",                  MOOD, MoodProfile, atmos.regime[3].clear_color)
ORGAN_PARAM_DEFONLY(F32,  0.0f, 1.0f,  0.01f,   "Atmosphere · Regime 4", "clear colour spread (±bright)", MOOD, MoodProfile, atmos.regime[3].clear_color_spread)

// ─── Atmosphere · Fog (the drivers' room + its witnesses) ────────────
// Fog is DRIVEN, so the dial sits on the driver's seam and the driven
// values are witnesses: out = regime rest + gain·deviation.
ORGAN_PARAM(DRIVERS, DriverSurface, fog.gain,         F32,  0.0f, 1.0f,  0.01f,   "Atmosphere · Fog", "drive gain")
ORGAN_PARAM_RO(CONFIG, GPUDesignConfig, fog_density,  F32,  "Atmosphere · Fog", "density (driven)")
ORGAN_PARAM_RO(CONFIG, GPUDesignConfig, fog_color,    VEC3, "Atmosphere · Fog", "colour (driven)")

// ─── Atmosphere · Veil ───────────────────────────────────────────────
// THE VEIL CHAIN BOUNDS THESE THREE AND THE RANGES CARRY IT: EXIST(350) >
// ring > lod0, and ring − icing > lod0. Widen one and re-prove the chain.
ORGAN_PARAM(CONFIG, GPUDesignConfig, veil_ring,   F32, 265.0f, 349.0f, 0.5f,  "Atmosphere · Veil", "ring (draw authority)")
ORGAN_PARAM(CONFIG, GPUDesignConfig, veil_icing,  F32, 0.0f,   60.0f,  0.25f, "Atmosphere · Veil", "icing band")
ORGAN_PARAM(CONFIG, GPUDesignConfig, lod0_radius, F32, 0.0f,   175.0f, 1.0f,  "Atmosphere · Veil", "LOD0 core")
ORGAN_PARAM(CONFIG, GPUDesignConfig, veil_dither, F32, 0.0f,   1.0f,   1.0f,  "Atmosphere · Veil", "rim dither (>0.5)")
// veil_strength has ONE author and it is not a dial: the cartridge writes
// `finite_mode ? 0 : 1`, a world-shape fact. The panel meters it — 1
// outdoors, 0 wherever walls define the edge.
ORGAN_PARAM_RO(CONFIG, GPUDesignConfig, veil_strength, F32, "Atmosphere · Veil", "authority (driven)")

// ─── Atmosphere · Checker ─────────────────────────────────────────
// Dials on the driver's seam (out = rest + gain·(driven − rest)); the driven
// values are witnesses. Rests are law: amount 0, variance 0 returns the seed colours.
ORGAN_PARAM(DRIVERS, DriverSurface, checker.rest_resultant, VEC3, 0.0f, 1.0f, 0.01f,  "Atmosphere · Checker", "rest colour")
ORGAN_PARAM(DRIVERS, DriverSurface, checker.rest_amount,    F32,  0.0f, 1.0f, 0.005f, "Atmosphere · Checker", "rest amount")
ORGAN_PARAM(DRIVERS, DriverSurface, checker.rest_variance,  F32,  0.0f, 8.0f, 0.04f,  "Atmosphere · Checker", "rest variance")
ORGAN_PARAM(DRIVERS, DriverSurface, checker.gain,           F32,  0.0f, 1.0f, 0.01f,  "Atmosphere · Checker", "drive gain")
ORGAN_PARAM_RO(CONFIG, GPUDesignConfig, checker_resultant,      VEC3, "Atmosphere · Checker", "colour (driven)")
ORGAN_PARAM_RO(CONFIG, GPUDesignConfig, checker_music_amount,   F32,  "Atmosphere · Checker", "amount (driven)")
ORGAN_PARAM_RO(CONFIG, GPUDesignConfig, checker_music_variance, F32,  "Atmosphere · Checker", "variance (driven)")

// ─── Atmosphere · Transition (witnesses only) ───────────────────
// The transition fade is authored per frame by the transition system, so
// a dial here would fight an author and lose confusingly.
ORGAN_PARAM_RO(CONFIG, GPUDesignConfig, fade_alpha,       F32,  "Atmosphere · Transition", "fade alpha (driven)")
ORGAN_PARAM_RO(CONFIG, GPUDesignConfig, fade_color,       VEC3, "Atmosphere · Transition", "fade colour (driven)")


// ─── Atmosphere · Fog envelope ──────────────────────────────────
// CANVAS_LIVE (coupling/canvas_surface.hpp), block 9. An ENVELOPE
// AUTHORITY, not a rest: how long the coupling takes to reach the seam.
ORGAN_PARAM_NS(canvas, CANVAS, CanvasSurface, fog_span, F32, 0.0f, 8.0f, 0.04f, "Atmosphere · Fog envelope", "field glide (beats)")

// ─── Atmosphere · Checker cadence ───────────────────────────────
// CANVAS_LIVE. The pc-colour field's READ cadence and envelope — how often
// the voice is read, and how fast the resultant chases it.
ORGAN_PARAM_NS(canvas, CANVAS, CanvasSurface, checker_read_span, F32, 0.25f, 32.0f, 0.25f, "Atmosphere · Checker cadence", "read span (beats)")
ORGAN_PARAM_NS(canvas, CANVAS, CanvasSurface, checker_attack,    F32, 0.25f, 32.0f, 0.25f, "Atmosphere · Checker cadence", "attack (beats)")
ORGAN_PARAM_NS(canvas, CANVAS, CanvasSurface, checker_release,   F32, 0.25f, 32.0f, 0.25f, "Atmosphere · Checker cadence", "release (beats)")

// ─── Terrain · Palette N ────────────────────────────────────────
// GPUDesignConfig, boot-pinned from surface/terrain_looks.hpp row 1. The
// weights are four lanes the WGSL normalises, not a clamp.
ORGAN_PARAM(CONFIG, GPUDesignConfig, palette_center[0][0], VEC3, 0.0f, 1.0f, 0.01f, "Terrain · Palette 0 sand",   "centre")
ORGAN_PARAM(CONFIG, GPUDesignConfig, palette_light[0][0],  VEC3, 0.0f, 1.0f, 0.01f, "Terrain · Palette 0 sand",   "light")
ORGAN_PARAM(CONFIG, GPUDesignConfig, palette_center[1][0], VEC3, 0.0f, 1.0f, 0.01f, "Terrain · Palette 1 salmon", "centre")
ORGAN_PARAM(CONFIG, GPUDesignConfig, palette_light[1][0],  VEC3, 0.0f, 1.0f, 0.01f, "Terrain · Palette 1 salmon", "light")
ORGAN_PARAM(CONFIG, GPUDesignConfig, palette_center[2][0], VEC3, 0.0f, 1.0f, 0.01f, "Terrain · Palette 2 green",  "centre")
ORGAN_PARAM(CONFIG, GPUDesignConfig, palette_light[2][0],  VEC3, 0.0f, 1.0f, 0.01f, "Terrain · Palette 2 green",  "light")
ORGAN_PARAM(CONFIG, GPUDesignConfig, palette_center[3][0], VEC3, 0.0f, 1.0f, 0.01f, "Terrain · Palette 3 warm",   "centre")
ORGAN_PARAM(CONFIG, GPUDesignConfig, palette_light[3][0],  VEC3, 0.0f, 1.0f, 0.01f, "Terrain · Palette 3 warm",   "light")
ORGAN_PARAM(CONFIG, GPUDesignConfig, palette_weight[0], F32, 0.0f, 1.0f, 0.005f, "Terrain · Palette mix", "sand")
ORGAN_PARAM(CONFIG, GPUDesignConfig, palette_weight[1], F32, 0.0f, 1.0f, 0.005f, "Terrain · Palette mix", "salmon")
ORGAN_PARAM(CONFIG, GPUDesignConfig, palette_weight[2], F32, 0.0f, 1.0f, 0.005f, "Terrain · Palette mix", "green")
ORGAN_PARAM(CONFIG, GPUDesignConfig, palette_weight[3], F32, 0.0f, 1.0f, 0.005f, "Terrain · Palette mix", "warm")

// ─── Terrain · Motion / Bands / Modes / GoL ─────────────────────
// GPUDesignConfig, boot-pinned from terrain_looks.hpp row 2 — the surface
// voice's silence. The ranges are that row's own units.
ORGAN_PARAM(CONFIG, GPUDesignConfig, terrain_time, F32, 0.0f, 64.0f, 0.25f, "Terrain · Motion", "clock (beats)")
ORGAN_PARAM(CONFIG, GPUDesignConfig, band_blend_0, F32, -1.0f, 1.0f, 0.01f, "Terrain · Bands", "0 continental")
ORGAN_PARAM(CONFIG, GPUDesignConfig, band_blend_1, F32, -1.0f, 1.0f, 0.01f, "Terrain · Bands", "1 regional")
ORGAN_PARAM(CONFIG, GPUDesignConfig, band_blend_2, F32, -1.0f, 1.0f, 0.01f, "Terrain · Bands", "2 local")
ORGAN_PARAM(CONFIG, GPUDesignConfig, band_blend_3, F32, -1.0f, 1.0f, 0.01f, "Terrain · Bands", "3 detail")
ORGAN_PARAM(CONFIG, GPUDesignConfig, band_blend_4, F32, -1.0f, 1.0f, 0.01f, "Terrain · Bands", "4 fine")
ORGAN_PARAM(CONFIG, GPUDesignConfig, band_blend_5, F32, -1.0f, 1.0f, 0.01f, "Terrain · Bands", "5 tectonic")
ORGAN_PARAM(CONFIG, GPUDesignConfig, band_phase_origin_0, F32, 0.0f, 64.0f, 0.25f, "Terrain · Band phase", "0 continental")
ORGAN_PARAM(CONFIG, GPUDesignConfig, band_phase_origin_1, F32, 0.0f, 64.0f, 0.25f, "Terrain · Band phase", "1 regional")
ORGAN_PARAM(CONFIG, GPUDesignConfig, band_phase_origin_2, F32, 0.0f, 64.0f, 0.25f, "Terrain · Band phase", "2 local")
ORGAN_PARAM(CONFIG, GPUDesignConfig, band_phase_origin_3, F32, 0.0f, 64.0f, 0.25f, "Terrain · Band phase", "3 detail")
ORGAN_PARAM(CONFIG, GPUDesignConfig, band_phase_origin_4, F32, 0.0f, 64.0f, 0.25f, "Terrain · Band phase", "4 fine")
ORGAN_PARAM(CONFIG, GPUDesignConfig, band_phase_origin_5, F32, 0.0f, 64.0f, 0.25f, "Terrain · Band phase", "5 tectonic")
ORGAN_PARAM(CONFIG, GPUDesignConfig, mode_color_shift,       F32, -1.0f, 1.0f, 0.01f, "Terrain · Modes", "colour shift")
ORGAN_PARAM(CONFIG, GPUDesignConfig, mode_checker_scatter,   F32, -1.0f, 1.0f, 0.01f, "Terrain · Modes", "checker scatter")
ORGAN_PARAM(CONFIG, GPUDesignConfig, mode_palette_target,    F32, 0.0f, 3.0f, 1.0f,   "Terrain · Modes", "palette target (idx)")
ORGAN_PARAM(CONFIG, GPUDesignConfig, mode_palette_intensity, F32, 0.0f, 1.0f, 0.005f, "Terrain · Modes", "palette intensity")
ORGAN_PARAM(CONFIG, GPUDesignConfig, mode_discrete_tier,     F32, 0.0f, 4.0f, 1.0f,   "Terrain · Modes", "discrete tier (idx)")
ORGAN_PARAM(CONFIG, GPUDesignConfig, mode_gol_tick_scale,   F32, 0.1f, 4.0f, 0.02f, "Terrain · GoL", "tick scale")
ORGAN_PARAM(CONFIG, GPUDesignConfig, mode_gol_height_scale, F32, 0.0f, 4.0f, 0.02f, "Terrain · GoL", "height scale")

// ─── Terrain · Mosaic ──────────────────────────────────────────
// GPUDesignConfig. Rests are Dim::MOSAIC_*; state.hpp's own note says
// "All live-tunable via config; these are the rests."
ORGAN_PARAM(CONFIG, GPUDesignConfig, mosaic_enable,        F32, 0.0f, 1.0f,  1.0f,   "Terrain · Mosaic", "enable")
ORGAN_PARAM(CONFIG, GPUDesignConfig, mosaic_shard_size,    F32, 0.0f, 1.2f,  0.005f, "Terrain · Mosaic", "shard size")
ORGAN_PARAM(CONFIG, GPUDesignConfig, mosaic_passage_scale, F32, 0.0f, 48.0f, 0.25f,  "Terrain · Mosaic", "passage scale")
ORGAN_PARAM(CONFIG, GPUDesignConfig, mosaic_blend,         F32, 0.0f, 1.0f,  0.005f, "Terrain · Mosaic", "boundary blend")
ORGAN_PARAM(CONFIG, GPUDesignConfig, mosaic_facet,         F32, 0.0f, 1.0f,  0.005f, "Terrain · Mosaic", "facet lean")

// ─── Terrain · Indoor — EDITS THE NEXT SPAWN ────────────────────
// INDOOR_LIVE (contracts/indoor_module.hpp), DESTRUCTIVE: nine of these
// two fields' ten readers run at spawn, so nothing here is wired.
ORGAN_PARAM_GEN(INDOOR, IndoorSurface, height_cap_fraction, F32, 0.0f, 1.0f, 0.005f, "Terrain · Indoor", "height cap fraction")
ORGAN_PARAM_GEN(INDOOR, IndoorSurface, ribbon_scale,        F32, 0.0f, 0.6f, 0.005f, "Terrain · Indoor", "ribbon scale")
// The three indoor structurals, METERED. Their author is the mood applier,
// composing each from MoodProfile's STRUCTURAL group — world generation
// reads that, so a dial would edit a world already built.
ORGAN_PARAM_RO(CONFIG, GPUDesignConfig, terrain_amp_ceiling, F32, "Terrain · Indoor", "terrain amp ceiling (driven)")
ORGAN_PARAM_RO(CONFIG, GPUDesignConfig, ceiling_height,      F32, "Terrain · Indoor", "ceiling height (driven)")
ORGAN_PARAM_RO(CONFIG, GPUDesignConfig, indoor_height_cap,   F32, "Terrain · Indoor", "GoL lift cap (driven)")

// ─── Pawn · Aura (the drivers' room + its witnesses) ─────────────────
// The intent is the presence ramp's rest TARGET; key 3 and the mood policy
// door write it here too. The two driven outputs witness below.
ORGAN_PARAM(DRIVERS, DriverSurface, aura.intent,      BOOL, 0.0f, 1.0f,  1.0f,    "Pawn · Aura", "intent")
ORGAN_PARAM(DRIVERS, DriverSurface, aura.attack,      F32,  0.05f, 8.0f, 0.05f,   "Pawn · Aura", "attack (1/s)")
ORGAN_PARAM(DRIVERS, DriverSurface, aura.release,     F32,  0.05f, 8.0f, 0.05f,   "Pawn · Aura", "release (1/s)")
ORGAN_PARAM(DRIVERS, DriverSurface, aura.height_gain, F32,  0.0f, 2.0f,  0.01f,   "Pawn · Aura", "height gain")
ORGAN_PARAM_RO(CONFIG, GPUDesignConfig, aura_enabled,     F32, "Pawn · Aura", "enabled (driven)")
ORGAN_PARAM_RO(CONFIG, GPUDesignConfig, pawn_aura_height, F32, "Pawn · Aura", "height (driven)")

// ─── Pawn · Aura profile ────────────────────────────────────────────
// PAWN_AURA_LIVE (contracts/pawn_surface.hpp) — the module's own facts.
// tint_strength rests at 0: the tree says 0 silences the terrain tint.
ORGAN_PARAM(PAWN, PawnAuraProfile, influence_radius, F32, 0.0f, 80.0f, 0.4f,   "Pawn · Aura profile", "influence radius")
ORGAN_PARAM(PAWN, PawnAuraProfile, attack_stiffness, F32, 0.0f, 48.0f, 0.25f,  "Pawn · Aura profile", "attack stiffness")
ORGAN_PARAM(PAWN, PawnAuraProfile, attack_damping,   F32, 0.0f, 2.8f,  0.01f,  "Pawn · Aura profile", "attack damping")
ORGAN_PARAM(PAWN, PawnAuraProfile, release_rate,     F32, 0.05f, 8.0f, 0.05f,  "Pawn · Aura profile", "release rate")
ORGAN_PARAM(PAWN, PawnAuraProfile, tint_strength,    F32, 0.0f, 1.0f,  0.005f, "Pawn · Aura profile", "tint strength")
ORGAN_PARAM(PAWN, PawnAuraProfile, tint_r,           VEC3, 0.0f, 1.0f, 0.01f,  "Pawn · Aura profile", "tint colour")
ORGAN_PARAM(PAWN, PawnAuraProfile, delta_magnitude,  F32, 0.0f, 1.2f,  0.005f, "Pawn · Aura profile", "delta magnitude")
ORGAN_PARAM(PAWN, PawnAuraProfile, height_scale,     F32, 0.0f, 12.0f, 0.05f,  "Pawn · Aura profile", "height scale")
ORGAN_PARAM(PAWN, PawnAuraProfile, delta_mode,       U32, 0.0f, 1.0f,  1.0f,   "Pawn · Aura profile", "delta mode (0 conv, 1 rand)")

// ─── Pawn · Figure (driven) — witnesses only ─────────────
// Re-derived every frame from the POSSESSED FIGURE's row in PAWN_FIGURES;
// the dial is that table, a composite, not a slider on the output.
ORGAN_PARAM_RO(CONFIG, GPUDesignConfig, pawn_tilt_tau,    F32,  "Pawn · Figure (driven)", "tilt tau")
ORGAN_PARAM_RO(CONFIG, GPUDesignConfig, pawn_body_radius, F32,  "Pawn · Figure (driven)", "body radius")
ORGAN_PARAM_RO(CONFIG, GPUDesignConfig, fpv_eye_height,   F32,  "Pawn · Figure (driven)", "eye height")
ORGAN_PARAM_RO(CONFIG, GPUDesignConfig, possessed_height, F32,  "Pawn · Figure (driven)", "figure height")

// ─── Ribbon · Head / Wander ─────────────────────────────────────────
// THE FLIGHT DIALS MOVED HOME AT RIBBON_1, and it is the FIELD'S OWN
// IDIOM: the head is a kernel now (world.wgsl ribbon_head), so the LIVE
// home of every number it reads is config.ribbon_*, and RIBBON_LIVE
// (contracts/ribbon_surface.hpp) is the boot REST — exactly what
// FIELD_SLACK and its seven siblings are to the field's rows below. Two
// enrollments for one fact would be two dials, one of them dead.
// BOTH TERMS OF THE STEERING LAW ARE LIVE: available yaw =
// min(yaw_rate, speed / r_min), r_min ≥ 1 (the divisor's floor).
ORGAN_PARAM(CONFIG, GPUDesignConfig, ribbon_yaw_rate,        F32, 0.0f, 4.0f,   0.02f, "Ribbon · Head", "yaw rate (rad/s)")
ORGAN_PARAM(CONFIG, GPUDesignConfig, ribbon_max_speed,       F32, 0.0f, 160.0f, 0.5f,  "Ribbon · Head", "max speed")
ORGAN_PARAM(CONFIG, GPUDesignConfig, ribbon_r_min,           F32, 1.0f, 160.0f, 0.5f,  "Ribbon · Head", "min turn radius")
ORGAN_PARAM(CONFIG, GPUDesignConfig, ribbon_climb_rate,      F32, 0.0f, 60.0f,  0.25f, "Ribbon · Head", "climb rate")
ORGAN_PARAM(CONFIG, GPUDesignConfig, ribbon_floor_margin,    F32, 0.0f, 100.0f, 0.5f,  "Ribbon · Head", "floor margin")
ORGAN_PARAM(CONFIG, GPUDesignConfig, ribbon_alt_smooth_dist, F32, 0.0f, 720.0f, 2.5f,  "Ribbon · Head", "altitude smoothing")
ORGAN_PARAM(CONFIG, GPUDesignConfig, ribbon_alt_stiff,       F32, 0.0f, 1.44f,  0.005f,"Ribbon · Head", "altitude stiffness")
ORGAN_PARAM(CONFIG, GPUDesignConfig, ribbon_mount_setback,   F32, 0.0f, 6.0f,   0.02f, "Ribbon · Head", "mount setback")
ORGAN_PARAM(CONFIG, GPUDesignConfig, ribbon_hands_tau,       F32, 0.0f, 2.4f,   0.01f, "Ribbon · Head", "hands tau (s)")
// ─── Ribbon · Sky Rule (RIBBON_1) ── self-preservation, two readers ──
// The head steers and climbs at `lookahead` ahead of its nose with
// `clear_head` of berth; the body flows past the same shells at
// `clear_body`, which is deliberately the smaller of the two.
ORGAN_PARAM(CONFIG, GPUDesignConfig, ribbon_lookahead,       F32, 0.0f, 400.0f, 2.0f,  "Ribbon · Sky Rule", "lookahead")
ORGAN_PARAM(CONFIG, GPUDesignConfig, ribbon_clear_head,      F32, 0.0f, 100.0f, 0.5f,  "Ribbon · Sky Rule", "head clearance")
ORGAN_PARAM(CONFIG, GPUDesignConfig, ribbon_clear_body,      F32, 0.0f, 50.0f,  0.25f, "Ribbon · Sky Rule", "body clearance")
// ─── Ribbon · Wander (RIBBON_2) ── the brain came home to the head kernel,
// so its dials took the flight dials' road: config.ribbon_wander_*, boot-
// pinned from the bank. The eased-steering dial went with the CPU brain —
// the command passes through the hands' own tau now, which is the hand it is.
ORGAN_PARAM(CONFIG, GPUDesignConfig, ribbon_wander_soft,     F32, 0.05f, 2.0f,   0.01f, "Ribbon · Wander", "steer softness")
ORGAN_PARAM(CONFIG, GPUDesignConfig, ribbon_wander_yaw_max,  F32, 0.0f,  1.0f,   0.005f,"Ribbon · Wander", "yaw cap")
ORGAN_PARAM(CONFIG, GPUDesignConfig, ribbon_wander_arrive,   F32, 10.0f, 400.0f, 2.0f,  "Ribbon · Wander", "arrive radius")
ORGAN_PARAM(CONFIG, GPUDesignConfig, ribbon_roam_radius,     F32, 50.0f, 1500.0f, 5.0f, "Ribbon · Wander", "roam radius")
// Read on the CPU, so they stay on the bank: the phase clock's anchor and
// the mount's two eases.
ORGAN_PARAM(RIBBON, RibbonSurface, reference_bpm,   F32, 40.0f, 240.0f, 1.0f, "Ribbon · Head", "reference BPM")
ORGAN_PARAM(RIBBON, RibbonSurface, board_seconds,   F32, 0.05f, 6.0f,   0.02f, "Ribbon · Mount", "board ease (s)")
ORGAN_PARAM(RIBBON, RibbonSurface, land_seconds,    F32, 0.05f, 6.0f,   0.02f, "Ribbon · Mount", "land ease (s)")

// ─── Ribbon · Swell / Tint envelope ─────────────────────────────
// CANVAS_LIVE, under RIBBON because a dial sits under its SUBJECT. The
// ranges keep the module's own "(ruled)" ceiling reachable by hand.
ORGAN_PARAM_NS(canvas, CANVAS, CanvasSurface, swell_ceiling,    F32, 1.0f,  4.0f,  0.01f, "Ribbon · Swell", "ceiling (× idle)")
ORGAN_PARAM_NS(canvas, CANVAS, CanvasSurface, swell_ramp,       F32, 0.0f,  32.0f, 0.25f, "Ribbon · Swell", "ramp (beats)")
ORGAN_PARAM_NS(canvas, CANVAS, CanvasSurface, swell_attack,     F32, 0.01f, 8.0f,  0.04f, "Ribbon · Swell", "attack (beats)")
ORGAN_PARAM_NS(canvas, CANVAS, CanvasSurface, swell_release,    F32, 0.01f, 8.0f,  0.04f, "Ribbon · Swell", "release (beats)")
ORGAN_PARAM_NS(canvas, CANVAS, CanvasSurface, pitch_vec_origin, F32, 0.0f,  6.2832f, 0.01f, "Ribbon · Tint", "hue seating (rad)")
ORGAN_PARAM_NS(canvas, CANVAS, CanvasSurface, tint_luma,        F32, 0.0f,  1.0f,  0.005f, "Ribbon · Tint", "luma")
ORGAN_PARAM_NS(canvas, CANVAS, CanvasSurface, tint_chroma,      F32, 0.0f,  1.0f,  0.005f, "Ribbon · Tint", "chroma")
ORGAN_PARAM_NS(canvas, CANVAS, CanvasSurface, tint_mix_max,     F32, 0.0f,  1.0f,  0.005f, "Ribbon · Tint", "mix ceiling")
ORGAN_PARAM_NS(canvas, CANVAS, CanvasSurface, tint_mix_attack,  F32, 0.01f, 12.0f, 0.05f,  "Ribbon · Tint", "mix attack (beats)")
ORGAN_PARAM_NS(canvas, CANVAS, CanvasSurface, tint_mix_release, F32, 0.01f, 12.0f, 0.05f,  "Ribbon · Tint", "mix release (beats)")
ORGAN_PARAM_NS(canvas, CANVAS, CanvasSurface, tint_hue_span,    F32, 0.01f, 12.0f, 0.05f,  "Ribbon · Tint", "hue span (beats)")

// ─── Ribbon · Drivers (the seam's rests and its one gain) ───────
// The four pipes ribbon_frame_tick reads every frame, seamed as
// out = rest + gain·(driven − rest); the rests are the seam's fallbacks.
ORGAN_PARAM(DRIVERS, DriverSurface, ribbon.rest_amp_lat,   F32,  0.0f, 4.0f, 0.01f,  "Ribbon · Drivers", "amp lat rest")
ORGAN_PARAM(DRIVERS, DriverSurface, ribbon.rest_amp_vert,  F32,  0.0f, 4.0f, 0.01f,  "Ribbon · Drivers", "amp vert rest")
ORGAN_PARAM(DRIVERS, DriverSurface, ribbon.rest_tint_stim, VEC3, 0.0f, 1.0f, 0.01f,  "Ribbon · Drivers", "tint stim rest")
ORGAN_PARAM(DRIVERS, DriverSurface, ribbon.rest_tint_mix,  F32,  0.0f, 1.0f, 0.005f, "Ribbon · Drivers", "tint mix rest")
ORGAN_PARAM(DRIVERS, DriverSurface, ribbon.gain,           F32,  0.0f, 2.0f, 0.01f,  "Ribbon · Drivers", "drive gain")

// ─── Ribbon · Spawn — EDITS THE NEXT RIBBON ─────────────────────
// RIBBON_SPAWN_LIVE (contracts/ribbon_surface.hpp), DESTRUCTIVE: every row
// is read once as a ribbon is drawn, so none is wired to the boundary.
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, spawn_chance,        F32, 0.0f,   1.0f,   0.005f, "Ribbon · Spawn", "spawn chance")
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, position_jitter,     F32, 0.0f,   1.0f,   0.005f, "Ribbon · Spawn", "position jitter")
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, wander_chance,       F32, 0.0f,   1.0f,   0.005f, "Ribbon · Spawn", "wander chance")
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, wander_cruise_base,  F32, 0.0f,   1.0f,   0.005f, "Ribbon · Spawn", "cruise mean (× max speed)")
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, wander_cruise_sigma, F32, 0.0f,   1.0f,   0.005f, "Ribbon · Spawn", "cruise sigma")
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, wander_cruise_min,   F32, 0.0f,   1.0f,   0.005f, "Ribbon · Spawn", "cruise floor")
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, wander_cruise_max,   F32, 0.0f,   1.0f,   0.005f, "Ribbon · Spawn", "cruise ceiling")
// ─── Ribbon · Colour — EDITS THE NEXT RIBBON ────────────────────
// The three weights are probabilities select_tier normalises; the palette
// is four base colours the SMOOTH draw varies around, its bias signed.
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, color_weights[0],    F32, 0.0f, 1.0f, 0.005f, "Ribbon · Colour", "weight smooth")
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, color_weights[1],    F32, 0.0f, 1.0f, 0.005f, "Ribbon · Colour", "weight tinted")
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, color_weights[2],    F32, 0.0f, 1.0f, 0.005f, "Ribbon · Colour", "weight contrast")
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, smooth_palette[0][0], VEC3, 0.0f, 1.0f, 0.01f, "Ribbon · Colour", "smooth 0 sandstone")
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, smooth_palette[1][0], VEC3, 0.0f, 1.0f, 0.01f, "Ribbon · Colour", "smooth 1 sky")
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, smooth_palette[2][0], VEC3, 0.0f, 1.0f, 0.01f, "Ribbon · Colour", "smooth 2 golden")
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, smooth_palette[3][0], VEC3, 0.0f, 1.0f, 0.01f, "Ribbon · Colour", "smooth 3 sage")
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, smooth_var_range,    F32,  0.0f, 0.4f,  0.005f, "Ribbon · Colour", "smooth var range")
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, smooth_var_bias,     F32, -0.2f, 0.2f,  0.005f, "Ribbon · Colour", "smooth var bias")
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, smooth_var_g_scale,  F32,  0.0f, 2.0f,  0.01f,  "Ribbon · Colour", "smooth var × green")
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, smooth_var_b_scale,  F32,  0.0f, 2.0f,  0.01f,  "Ribbon · Colour", "smooth var × blue")
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, tinted_range[0],     VEC3, 0.0f, 1.0f,  0.01f,  "Ribbon · Colour", "tinted range")
ORGAN_PARAM_GEN(RIBBON_SPAWN, RibbonSpawnSurface, tinted_base[0],      VEC3, 0.0f, 1.0f,  0.01f,  "Ribbon · Colour", "tinted base")

// ─── THE TIERS ───────────────────────────────────────────────────────
// The home is agentRoomStage_.tier_gains, and organ_flush writes both
// windows onto it. TIER_LIVE is what this world means by its tiers.
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[0].color_r,    VEC3, 0.0f, 1.0f, 0.01f, "Agents · Tier 0", "colour",     TIER, AgentTierBank, t[0].color_r)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[0].speed_gain, F32,  0.0f, 4.0f, 0.01f, "Agents · Tier 0", "speed gain", TIER, AgentTierBank, t[0].speed_gain)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[1].color_r,    VEC3, 0.0f, 1.0f, 0.01f, "Agents · Tier 1", "colour",     TIER, AgentTierBank, t[1].color_r)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[1].speed_gain, F32,  0.0f, 4.0f, 0.01f, "Agents · Tier 1", "speed gain", TIER, AgentTierBank, t[1].speed_gain)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[2].color_r,    VEC3, 0.0f, 1.0f, 0.01f, "Agents · Tier 2", "colour",     TIER, AgentTierBank, t[2].color_r)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[2].speed_gain, F32,  0.0f, 4.0f, 0.01f, "Agents · Tier 2", "speed gain", TIER, AgentTierBank, t[2].speed_gain)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[3].color_r,    VEC3, 0.0f, 1.0f, 0.01f, "Agents · Tier 3", "colour",     TIER, AgentTierBank, t[3].color_r)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[3].speed_gain, F32,  0.0f, 4.0f, 0.01f, "Agents · Tier 3", "speed gain", TIER, AgentTierBank, t[3].speed_gain)

// ─── Agents · Tier N — the bank's remaining columns ──────────────
// flee_gain's max is 0.99 and that is THE TREE'S OWN WORD: a gain ≥ 1.0
// means the agent can never be approached (contracts/agent_tiers.hpp).
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[0].step_gain,       F32, 0.0f,   4.0f,    0.02f,  "Agents · Tier 0", "step gain", TIER, AgentTierBank, t[0].step_gain)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[0].persist_gain,    F32, 0.0f,   4.0f,    0.02f,  "Agents · Tier 0", "persist gain", TIER, AgentTierBank, t[0].persist_gain)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[0].contact_radius,  F32, 0.0f,   8.0f,    0.04f,  "Agents · Tier 0", "contact radius", TIER, AgentTierBank, t[0].contact_radius)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[0].contact_mass,    F32, 0.0f,   4.0f,    0.02f,  "Agents · Tier 0", "contact mass", TIER, AgentTierBank, t[0].contact_mass)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[0].personal_radius, F32, 0.0f,   120.0f,  0.5f,   "Agents · Tier 0", "personal radius", TIER, AgentTierBank, t[0].personal_radius)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[0].flee_gain_player, F32, 0.0f,   0.99f,   0.005f, "Agents · Tier 0", "flee gain", TIER, AgentTierBank, t[0].flee_gain_player)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[1].step_gain,       F32, 0.0f,   4.0f,    0.02f,  "Agents · Tier 1", "step gain", TIER, AgentTierBank, t[1].step_gain)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[1].persist_gain,    F32, 0.0f,   4.0f,    0.02f,  "Agents · Tier 1", "persist gain", TIER, AgentTierBank, t[1].persist_gain)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[1].contact_radius,  F32, 0.0f,   8.0f,    0.04f,  "Agents · Tier 1", "contact radius", TIER, AgentTierBank, t[1].contact_radius)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[1].contact_mass,    F32, 0.0f,   4.0f,    0.02f,  "Agents · Tier 1", "contact mass", TIER, AgentTierBank, t[1].contact_mass)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[1].personal_radius, F32, 0.0f,   120.0f,  0.5f,   "Agents · Tier 1", "personal radius", TIER, AgentTierBank, t[1].personal_radius)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[1].flee_gain_player, F32, 0.0f,   0.99f,   0.005f, "Agents · Tier 1", "flee gain", TIER, AgentTierBank, t[1].flee_gain_player)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[2].step_gain,       F32, 0.0f,   4.0f,    0.02f,  "Agents · Tier 2", "step gain", TIER, AgentTierBank, t[2].step_gain)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[2].persist_gain,    F32, 0.0f,   4.0f,    0.02f,  "Agents · Tier 2", "persist gain", TIER, AgentTierBank, t[2].persist_gain)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[2].contact_radius,  F32, 0.0f,   8.0f,    0.04f,  "Agents · Tier 2", "contact radius", TIER, AgentTierBank, t[2].contact_radius)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[2].contact_mass,    F32, 0.0f,   4.0f,    0.02f,  "Agents · Tier 2", "contact mass", TIER, AgentTierBank, t[2].contact_mass)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[2].personal_radius, F32, 0.0f,   120.0f,  0.5f,   "Agents · Tier 2", "personal radius", TIER, AgentTierBank, t[2].personal_radius)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[2].flee_gain_player, F32, 0.0f,   0.99f,   0.005f, "Agents · Tier 2", "flee gain", TIER, AgentTierBank, t[2].flee_gain_player)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[3].step_gain,       F32, 0.0f,   4.0f,    0.02f,  "Agents · Tier 3", "step gain", TIER, AgentTierBank, t[3].step_gain)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[3].persist_gain,    F32, 0.0f,   4.0f,    0.02f,  "Agents · Tier 3", "persist gain", TIER, AgentTierBank, t[3].persist_gain)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[3].contact_radius,  F32, 0.0f,   8.0f,    0.04f,  "Agents · Tier 3", "contact radius", TIER, AgentTierBank, t[3].contact_radius)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[3].contact_mass,    F32, 0.0f,   4.0f,    0.02f,  "Agents · Tier 3", "contact mass", TIER, AgentTierBank, t[3].contact_mass)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[3].personal_radius, F32, 0.0f,   120.0f,  0.5f,   "Agents · Tier 3", "personal radius", TIER, AgentTierBank, t[3].personal_radius)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, tier_gains[3].flee_gain_player, F32, 0.0f,   0.99f,   0.005f, "Agents · Tier 3", "flee gain", TIER, AgentTierBank, t[3].flee_gain_player)

// ─── Agents · <behaviour> ────────────────────────────────────────
// BEHAVIOR_LIVE (contracts/agent_tiers.hpp) rides the tier bank's author
// and its boundary. Row 0 is all zeros: a possessed body takes input.
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[0].step_rate,       F32, 0.0f,  1.0f,   0.005f,  "Agents · player_controlled", "step rate", BEHAVIOR, AgentBehaviorBank, b[0].step_rate)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[0].step_size,       F32, 0.0f,  10.0f,  0.05f,   "Agents · player_controlled", "step size", BEHAVIOR, AgentBehaviorBank, b[0].step_size)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[0].persistence,     F32, 0.0f,  1.0f,   0.005f,  "Agents · player_controlled", "persistence", BEHAVIOR, AgentBehaviorBank, b[0].persistence)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[0].drag,            F32, 0.0f,  3.75f,  0.02f,   "Agents · player_controlled", "drag", BEHAVIOR, AgentBehaviorBank, b[0].drag)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[0].home_pull,       F32, 0.0f,  10.0f,  0.05f,   "Agents · player_controlled", "home pull", BEHAVIOR, AgentBehaviorBank, b[0].home_pull)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[0].neighbor_radius, F32, 0.0f,  50.0f,  0.25f,   "Agents · player_controlled", "neighbour radius", BEHAVIOR, AgentBehaviorBank, b[0].neighbor_radius)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[0].speed_cap,       F32, 0.0f,  10.0f,  0.05f,   "Agents · player_controlled", "speed cap", BEHAVIOR, AgentBehaviorBank, b[0].speed_cap)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[1].step_rate,       F32, 0.0f,  1.0f,   0.005f,  "Agents · random_walk", "step rate", BEHAVIOR, AgentBehaviorBank, b[1].step_rate)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[1].step_size,       F32, 0.0f,  10.0f,  0.05f,   "Agents · random_walk", "step size", BEHAVIOR, AgentBehaviorBank, b[1].step_size)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[1].persistence,     F32, 0.0f,  1.0f,   0.005f,  "Agents · random_walk", "persistence", BEHAVIOR, AgentBehaviorBank, b[1].persistence)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[1].drag,            F32, 0.0f,  3.75f,  0.02f,   "Agents · random_walk", "drag", BEHAVIOR, AgentBehaviorBank, b[1].drag)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[1].home_pull,       F32, 0.0f,  10.0f,  0.05f,   "Agents · random_walk", "home pull", BEHAVIOR, AgentBehaviorBank, b[1].home_pull)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[1].neighbor_radius, F32, 0.0f,  50.0f,  0.25f,   "Agents · random_walk", "neighbour radius", BEHAVIOR, AgentBehaviorBank, b[1].neighbor_radius)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[1].speed_cap,       F32, 0.0f,  10.0f,  0.05f,   "Agents · random_walk", "speed cap", BEHAVIOR, AgentBehaviorBank, b[1].speed_cap)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[2].step_rate,       F32, 0.0f,  1.0f,   0.005f,  "Agents · biased_walk", "step rate", BEHAVIOR, AgentBehaviorBank, b[2].step_rate)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[2].step_size,       F32, 0.0f,  10.0f,  0.05f,   "Agents · biased_walk", "step size", BEHAVIOR, AgentBehaviorBank, b[2].step_size)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[2].persistence,     F32, 0.0f,  1.0f,   0.005f,  "Agents · biased_walk", "persistence", BEHAVIOR, AgentBehaviorBank, b[2].persistence)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[2].drag,            F32, 0.0f,  3.75f,  0.02f,   "Agents · biased_walk", "drag", BEHAVIOR, AgentBehaviorBank, b[2].drag)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[2].home_pull,       F32, 0.0f,  10.0f,  0.05f,   "Agents · biased_walk", "home pull", BEHAVIOR, AgentBehaviorBank, b[2].home_pull)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[2].neighbor_radius, F32, 0.0f,  50.0f,  0.25f,   "Agents · biased_walk", "neighbour radius", BEHAVIOR, AgentBehaviorBank, b[2].neighbor_radius)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[2].speed_cap,       F32, 0.0f,  10.0f,  0.05f,   "Agents · biased_walk", "speed cap", BEHAVIOR, AgentBehaviorBank, b[2].speed_cap)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[3].step_rate,       F32, 0.0f,  1.0f,   0.005f,  "Agents · wanderer", "step rate", BEHAVIOR, AgentBehaviorBank, b[3].step_rate)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[3].step_size,       F32, 0.0f,  10.0f,  0.05f,   "Agents · wanderer", "step size", BEHAVIOR, AgentBehaviorBank, b[3].step_size)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[3].persistence,     F32, 0.0f,  1.0f,   0.005f,  "Agents · wanderer", "persistence", BEHAVIOR, AgentBehaviorBank, b[3].persistence)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[3].drag,            F32, 0.0f,  3.75f,  0.02f,   "Agents · wanderer", "drag", BEHAVIOR, AgentBehaviorBank, b[3].drag)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[3].home_pull,       F32, 0.0f,  10.0f,  0.05f,   "Agents · wanderer", "home pull", BEHAVIOR, AgentBehaviorBank, b[3].home_pull)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[3].neighbor_radius, F32, 0.0f,  50.0f,  0.25f,   "Agents · wanderer", "neighbour radius", BEHAVIOR, AgentBehaviorBank, b[3].neighbor_radius)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[3].speed_cap,       F32, 0.0f,  10.0f,  0.05f,   "Agents · wanderer", "speed cap", BEHAVIOR, AgentBehaviorBank, b[3].speed_cap)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[4].step_rate,       F32, 0.0f,  1.0f,   0.005f,  "Agents · home_seeker", "step rate", BEHAVIOR, AgentBehaviorBank, b[4].step_rate)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[4].step_size,       F32, 0.0f,  10.0f,  0.05f,   "Agents · home_seeker", "step size", BEHAVIOR, AgentBehaviorBank, b[4].step_size)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[4].persistence,     F32, 0.0f,  1.0f,   0.005f,  "Agents · home_seeker", "persistence", BEHAVIOR, AgentBehaviorBank, b[4].persistence)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[4].drag,            F32, 0.0f,  3.75f,  0.02f,   "Agents · home_seeker", "drag", BEHAVIOR, AgentBehaviorBank, b[4].drag)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[4].home_pull,       F32, 0.0f,  10.0f,  0.05f,   "Agents · home_seeker", "home pull", BEHAVIOR, AgentBehaviorBank, b[4].home_pull)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[4].neighbor_radius, F32, 0.0f,  50.0f,  0.25f,   "Agents · home_seeker", "neighbour radius", BEHAVIOR, AgentBehaviorBank, b[4].neighbor_radius)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[4].speed_cap,       F32, 0.0f,  10.0f,  0.05f,   "Agents · home_seeker", "speed cap", BEHAVIOR, AgentBehaviorBank, b[4].speed_cap)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[5].step_rate,       F32, 0.0f,  1.0f,   0.005f,  "Agents · slow_patrol", "step rate", BEHAVIOR, AgentBehaviorBank, b[5].step_rate)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[5].step_size,       F32, 0.0f,  10.0f,  0.05f,   "Agents · slow_patrol", "step size", BEHAVIOR, AgentBehaviorBank, b[5].step_size)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[5].persistence,     F32, 0.0f,  1.0f,   0.005f,  "Agents · slow_patrol", "persistence", BEHAVIOR, AgentBehaviorBank, b[5].persistence)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[5].drag,            F32, 0.0f,  3.75f,  0.02f,   "Agents · slow_patrol", "drag", BEHAVIOR, AgentBehaviorBank, b[5].drag)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[5].home_pull,       F32, 0.0f,  10.0f,  0.05f,   "Agents · slow_patrol", "home pull", BEHAVIOR, AgentBehaviorBank, b[5].home_pull)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[5].neighbor_radius, F32, 0.0f,  50.0f,  0.25f,   "Agents · slow_patrol", "neighbour radius", BEHAVIOR, AgentBehaviorBank, b[5].neighbor_radius)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[5].speed_cap,       F32, 0.0f,  10.0f,  0.05f,   "Agents · slow_patrol", "speed cap", BEHAVIOR, AgentBehaviorBank, b[5].speed_cap)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[6].step_rate,       F32, 0.0f,  1.0f,   0.005f,  "Agents · pursuit", "step rate", BEHAVIOR, AgentBehaviorBank, b[6].step_rate)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[6].step_size,       F32, 0.0f,  10.0f,  0.05f,   "Agents · pursuit", "step size", BEHAVIOR, AgentBehaviorBank, b[6].step_size)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[6].persistence,     F32, 0.0f,  1.0f,   0.005f,  "Agents · pursuit", "persistence", BEHAVIOR, AgentBehaviorBank, b[6].persistence)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[6].drag,            F32, 0.0f,  3.75f,  0.02f,   "Agents · pursuit", "drag", BEHAVIOR, AgentBehaviorBank, b[6].drag)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[6].home_pull,       F32, 0.0f,  10.0f,  0.05f,   "Agents · pursuit", "home pull", BEHAVIOR, AgentBehaviorBank, b[6].home_pull)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[6].neighbor_radius, F32, 0.0f,  50.0f,  0.25f,   "Agents · pursuit", "neighbour radius", BEHAVIOR, AgentBehaviorBank, b[6].neighbor_radius)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[6].speed_cap,       F32, 0.0f,  10.0f,  0.05f,   "Agents · pursuit", "speed cap", BEHAVIOR, AgentBehaviorBank, b[6].speed_cap)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[7].step_rate,       F32, 0.0f,  1.0f,   0.005f,  "Agents · flee", "step rate", BEHAVIOR, AgentBehaviorBank, b[7].step_rate)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[7].step_size,       F32, 0.0f,  10.0f,  0.05f,   "Agents · flee", "step size", BEHAVIOR, AgentBehaviorBank, b[7].step_size)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[7].persistence,     F32, 0.0f,  1.0f,   0.005f,  "Agents · flee", "persistence", BEHAVIOR, AgentBehaviorBank, b[7].persistence)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[7].drag,            F32, 0.0f,  3.75f,  0.02f,   "Agents · flee", "drag", BEHAVIOR, AgentBehaviorBank, b[7].drag)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[7].home_pull,       F32, 0.0f,  10.0f,  0.05f,   "Agents · flee", "home pull", BEHAVIOR, AgentBehaviorBank, b[7].home_pull)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[7].neighbor_radius, F32, 0.0f,  50.0f,  0.25f,   "Agents · flee", "neighbour radius", BEHAVIOR, AgentBehaviorBank, b[7].neighbor_radius)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[7].speed_cap,       F32, 0.0f,  10.0f,  0.05f,   "Agents · flee", "speed cap", BEHAVIOR, AgentBehaviorBank, b[7].speed_cap)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[8].step_rate,       F32, 0.0f,  1.0f,   0.005f,  "Agents · flock2d", "step rate", BEHAVIOR, AgentBehaviorBank, b[8].step_rate)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[8].step_size,       F32, 0.0f,  10.0f,  0.05f,   "Agents · flock2d", "step size", BEHAVIOR, AgentBehaviorBank, b[8].step_size)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[8].persistence,     F32, 0.0f,  1.0f,   0.005f,  "Agents · flock2d", "persistence", BEHAVIOR, AgentBehaviorBank, b[8].persistence)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[8].drag,            F32, 0.0f,  3.75f,  0.02f,   "Agents · flock2d", "drag", BEHAVIOR, AgentBehaviorBank, b[8].drag)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[8].home_pull,       F32, 0.0f,  10.0f,  0.05f,   "Agents · flock2d", "home pull", BEHAVIOR, AgentBehaviorBank, b[8].home_pull)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[8].neighbor_radius, F32, 0.0f,  50.0f,  0.25f,   "Agents · flock2d", "neighbour radius", BEHAVIOR, AgentBehaviorBank, b[8].neighbor_radius)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[8].speed_cap,       F32, 0.0f,  10.0f,  0.05f,   "Agents · flock2d", "speed cap", BEHAVIOR, AgentBehaviorBank, b[8].speed_cap)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[9].step_rate,       F32, 0.0f,  1.0f,   0.005f,  "Agents · levy_flight", "step rate", BEHAVIOR, AgentBehaviorBank, b[9].step_rate)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[9].step_size,       F32, 0.0f,  10.0f,  0.05f,   "Agents · levy_flight", "step size", BEHAVIOR, AgentBehaviorBank, b[9].step_size)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[9].persistence,     F32, 0.0f,  1.0f,   0.005f,  "Agents · levy_flight", "persistence", BEHAVIOR, AgentBehaviorBank, b[9].persistence)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[9].drag,            F32, 0.0f,  3.75f,  0.02f,   "Agents · levy_flight", "drag", BEHAVIOR, AgentBehaviorBank, b[9].drag)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[9].home_pull,       F32, 0.0f,  10.0f,  0.05f,   "Agents · levy_flight", "home pull", BEHAVIOR, AgentBehaviorBank, b[9].home_pull)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[9].neighbor_radius, F32, 0.0f,  50.0f,  0.25f,   "Agents · levy_flight", "neighbour radius", BEHAVIOR, AgentBehaviorBank, b[9].neighbor_radius)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[9].speed_cap,       F32, 0.0f,  10.0f,  0.05f,   "Agents · levy_flight", "speed cap", BEHAVIOR, AgentBehaviorBank, b[9].speed_cap)
// ATRIUM_4 — the passer. Row 5's ranges (it is slow_patrol's row, tuned),
// plus the aux column: the band, wu, either side of a door's plane.
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[10].step_rate,      F32, 0.0f,  1.0f,   0.005f,  "Agents · passer", "step rate", BEHAVIOR, AgentBehaviorBank, b[10].step_rate)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[10].step_size,      F32, 0.0f,  10.0f,  0.05f,   "Agents · passer", "step size (waypoint radius)", BEHAVIOR, AgentBehaviorBank, b[10].step_size)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[10].persistence,    F32, 0.0f,  1.0f,   0.005f,  "Agents · passer", "persistence", BEHAVIOR, AgentBehaviorBank, b[10].persistence)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[10].drag,           F32, 0.0f,  3.75f,  0.02f,   "Agents · passer", "drag", BEHAVIOR, AgentBehaviorBank, b[10].drag)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[10].home_pull,      F32, 0.0f,  10.0f,  0.05f,   "Agents · passer", "home pull", BEHAVIOR, AgentBehaviorBank, b[10].home_pull)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[10].neighbor_radius,F32, 0.0f,  50.0f,  0.25f,   "Agents · passer", "neighbour radius", BEHAVIOR, AgentBehaviorBank, b[10].neighbor_radius)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[10].speed_cap,      F32, 0.0f,  30.0f,  0.25f,   "Agents · passer", "speed cap", BEHAVIOR, AgentBehaviorBank, b[10].speed_cap)
ORGAN_PARAM_DEF(AGENT_ROOM, GPUAgentRoomConstants, behaviors[10].aux,            F32, 0.0f,  20.0f,  0.25f,   "Agents · passer", "band", BEHAVIOR, AgentBehaviorBank, b[10].aux)

// ─── Agents · Portals — EDITS THE NEXT DRAW ─────────────────────
// WORLD_DRAW_LIVE again, the population half. THE DESTINATION LAW: one
// weight per mood, normalised by the walk, and a 0 shuts a door.
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, portal_density,        F32, 0.0f, 1.0f, 0.005f, "Agents · Portals", "portal density")
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, mood_weights[0], F32, 0.0f, 1.0f, 0.005f, "Agents · Portals", "draw · open sunset")
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, mood_weights[1], F32, 0.0f, 1.0f, 0.005f, "Agents · Portals", "draw · indoor flat")
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, mood_weights[2], F32, 0.0f, 1.0f, 0.005f, "Agents · Portals", "draw · indoor vault")
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, mood_weights[3], F32, 0.0f, 1.0f, 0.005f, "Agents · Portals", "draw · finite outdoor")
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, mood_weights[4], F32, 0.0f, 1.0f, 0.005f, "Agents · Portals", "draw · open night")
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, mood_weights[5], F32, 0.0f, 1.0f, 0.005f, "Agents · Portals", "draw · open noon")
ORGAN_PARAM_GEN(WORLD, WorldDrawSurface, mood_weights[6], F32, 0.0f, 1.0f, 0.005f, "Agents · Portals", "draw · atrium")

// ─── Interaction · Field ───────────────────────────────────────
// GPUDesignConfig, boot-pinned from contracts/control_panel.hpp and read
// by both dialects. field_k is clamped against the beacon's S at the writer.
ORGAN_PARAM(CONFIG, GPUDesignConfig, field_slack,         F32, 0.0f, 12.0f,   0.05f, "Interaction · Field", "slack")
ORGAN_PARAM(CONFIG, GPUDesignConfig, field_arch_slack,    F32, 0.5f,  4.0f,   0.05f, "Interaction · Field", "arch slack")
ORGAN_PARAM(CONFIG, GPUDesignConfig, field_k,             F32, 0.0f, 1200.0f, 5.0f,  "Interaction · Field", "k (accel)")
ORGAN_PARAM(CONFIG, GPUDesignConfig, field_fmax,          F32, 0.0f, 2400.0f, 10.0f, "Interaction · Field", "f max")
ORGAN_PARAM(CONFIG, GPUDesignConfig, field_occupier_gain, F32, 0.0f, 4.0f,    0.02f, "Interaction · Field", "occupier gain")
ORGAN_PARAM(CONFIG, GPUDesignConfig, field_authored_gain, F32, 0.0f, 4.0f,    0.02f, "Interaction · Field", "authored gain")
ORGAN_PARAM(CONFIG, GPUDesignConfig, field_gain_cube,     F32, 0.0f, 16.0f,   0.08f, "Interaction · Field", "cube gain")
ORGAN_PARAM(CONFIG, GPUDesignConfig, field_gain_sphere,   F32, 0.0f, 4.0f,    0.02f, "Interaction · Field", "sphere gain")
ORGAN_PARAM(CONFIG, GPUDesignConfig, field_gain_agent,    F32, 0.0f, 16.0f,   0.08f, "Interaction · Field", "agent gain")

// ─── Interaction · the bodies' own dials ───────────────────────
// Each is boot-pinned from its own code panel and has no second author:
// point.hpp's bubble, Idle::PAWN_SPEED, MOVE_SPEED, CUBE_PLASTICITY.
ORGAN_PARAM(CONFIG, GPUDesignConfig, point_bubble_radius, F32, 0.0f, 80.0f,  0.5f,   "Interaction · Point",  "bubble radius")
ORGAN_PARAM(CONFIG, GPUDesignConfig, pawn_speed,          F32, 0.0f, 60.0f,  0.5f,   "Interaction · Pawn",   "walk speed")
ORGAN_PARAM(CONFIG, GPUDesignConfig, point_fly_speed,     F32, 0.0f, 120.0f, 0.5f,   "Interaction · Camera", "fly speed")
ORGAN_PARAM(CONFIG, GPUDesignConfig, cube_plasticity,     F32, 0.0f, 1.0f,   0.005f, "Interaction · Cubes",  "plasticity λ")

// ─── Interaction · Cubes — the floater synchrony knob ───────
// ONE AUTHOR: this dial. The cycler that shared the home retired
// unreachable (ORGAN_9); cartridge.hpp's floater step is the reader.
ORGAN_PARAM(CONFIG, GPUDesignConfig, floater_coordination, F32, 0.0f, 1.0f, 0.005f, "Interaction · Cubes", "floater coordination")

// ─── Interaction · Beacon / Camera ──────────────────────────────────
// PANEL_LIVE (contracts/control_panel.hpp). beacon.s's max is the tree's
// own FIELD_BEACON_S_MAX; the camera's four are input grammar, not pose.
ORGAN_PARAM(PANEL, PanelSurface, beacon.r0,   F32, 0.0f, 100.0f, 0.5f,  "Interaction · Beacon", "inner radius")
ORGAN_PARAM(PANEL, PanelSurface, beacon.r,    F32, 0.0f, 480.0f, 2.0f,  "Interaction · Beacon", "outer radius")
ORGAN_PARAM(PANEL, PanelSurface, beacon.s,    F32, 0.0f, the_board::FIELD_BEACON_S_MAX, 1.0f, "Interaction · Beacon", "pull strength")
ORGAN_PARAM(PANEL, PanelSurface, beacon.lift, F32, 0.0f, 80.0f,  0.4f,  "Interaction · Beacon", "lift")
ORGAN_PARAM(PANEL, PanelSurface, camera.look_sens_init,    F32, 0.000625f, 0.04f, 0.0002f, "Interaction · Camera", "look sens (anchor)")
ORGAN_PARAM(PANEL, PanelSurface, camera.look_sens_step,    F32, 1.01f, 2.0f,  0.005f, "Interaction · Camera", "look sens step")
ORGAN_PARAM(PANEL, PanelSurface, camera.look_sens_range,   F32, 1.0f,  32.0f, 0.25f,  "Interaction · Camera", "look sens range")
ORGAN_PARAM(PANEL, PanelSurface, camera.scroll_zoom_scale, F32, 0.0f,  8.0f,  0.05f,  "Interaction · Camera", "scroll zoom")
// The possession reach, graduated out of bodies/agents.hpp's tuning
// console; the `_SQ` twin is derived at its one read from the live value.
// The floor is one step, because a zero reach makes the search unwinnable.
ORGAN_PARAM(PANEL, PanelSurface, possession.radius, F32, 0.5f, 80.0f, 0.5f, "Interaction · Possession", "reach")
// THE PULSE'S IMPULSE (PULSE_1 D). The ring's SHAPE — speed, sharpness,
// damping, age decay — is NOT here: those are world.wgsl consts and
// ORGAN_2b's business, and enrolling them from this campaign would put
// four dials on a contributor this campaign only gave a writer to. This
// one row is what PULSE_1 introduced. Floor is 0, which is a legal
// reading and not a defect: it silences the tap without silencing the
// bus, so a soundtrack writer would still be heard.
ORGAN_PARAM(PANEL, PanelSurface, pulse.amplitude, F32, 0.0f, 4.0f, 0.05f, "Interaction · Pulse", "tap impulse")

// ─── Camera · Chase ─────────────────────────────────────────────────
// THE WITNESS'S OWN BEHAVIOUR, not its pose and not its input grammar
// (those are Interaction's, above). Boot-pinned from contracts/point.hpp
// CAMERA_CHASE_FF; update_camera_vp is the one reader, RIBBON host only.
//
// THE RANGE IS EVIDENCE. The aim ease trails a constant-velocity target by
// v·tau exactly, so 1.0 — the rest, and the shipped behaviour — adds back
// exactly that and cancels the trail; 0.0 restores it, which is what makes
// this dial the PROOF that the lock is the mechanism described rather than
// a tuning. Past 1.0 the camera would lead the flight, which is not a
// stance anyone asked for, so 1.0 is the ceiling.
ORGAN_PARAM(CONFIG, GPUDesignConfig, camera_chase_ff, F32, 0.0f, 1.0f, 0.01f, "Camera · Chase", "feed-forward")

// ─── Camera · Presence ──────────────────────────────────────────────
// The eye repels floaters as it passes (cubes only — a sphere's motor owns
// its orbit). Boot-pinned from contracts/point.hpp; cube_force_witness is
// the one reader, and it is OFF in free-fly.
//
// THE RANGES ARE EVIDENCE. GAIN: this force is an acceleration in the same
// integrator CurlField (12) and PhaseWave (30) push through, and the cube
// registry's own authoring note puts the stable ceiling at ~10 x
// spring_stiffness — 4.0/s², so 40. RADIUS: the point's bubble is the
// bound on awareness (80 wu), and a presence has no business reaching past
// what the point can notice, so the bubble's own range is the shell's.
ORGAN_PARAM(CONFIG, GPUDesignConfig, camera_push_gain,   F32, 0.0f, 40.0f, 0.25f, "Camera · Presence", "push gain")
ORGAN_PARAM(CONFIG, GPUDesignConfig, camera_push_radius, F32, 0.0f, 80.0f, 0.5f,  "Camera · Presence", "push radius")

// ─── Debug ──────────────────────────────────────────────────────────
// Config's uint32_t toggle words enroll as BOOL directly; fpv_mode is
// KEY-SHARED, the keyboard writes it too. mute_couplings: docs/ORGAN.md.
ORGAN_PARAM(CONFIG, GPUDesignConfig, mute_dynamics_0d, BOOL, 0.0f, 1.0f, 1.0f, "Debug", "mute dynamics (0D)")
ORGAN_PARAM(CONFIG, GPUDesignConfig, mute_signal,      BOOL, 0.0f, 1.0f, 1.0f, "Debug", "mute signal")
ORGAN_PARAM(CONFIG, GPUDesignConfig, freeze_sphere,    BOOL, 0.0f, 1.0f, 1.0f, "Debug", "freeze sphere")
ORGAN_PARAM(CONFIG, GPUDesignConfig, fpv_mode,         BOOL, 0.0f, 1.0f, 1.0f, "Debug", "FPV mode (key-shared)")

// ─── Measure · the subtraction masks (PANORAMA_1) ────────────────────
// THE INSTRUMENT THE METER LACKS. `[METER]` brackets a PASS and cannot say
// which draw inside it spent the milliseconds. These two words subtract a
// draw AT THE ENCODER — the pass row then reads its absence, and the
// difference is that draw's cost. The bit vocabulary is DrawBit/ShadowBit
// (state.hpp), beside the fields.
//
// They rest OPEN (0xFF / 0x3, pinned at the boot block). A cleared bit is a
// measurement in progress and never a shipped state — which is why they are
// enrolled as MEASURE and not as a taste group: nothing here is a dial the
// piece is tuned with.
ORGAN_PARAM(CONFIG, GPUDesignConfig, draw_mask,   U32, 0.0f, 255.0f, 1.0f, "Measure", "main draw mask (bits: A/B/C terrain, table, ribbon, paintings, orbs, fade)")
ORGAN_PARAM(CONFIG, GPUDesignConfig, shadow_mask, U32, 0.0f,   3.0f, 1.0f, "Measure", "shadow draw mask (bit0 terrain, bit1 entities+artworks)")
// The tap A/B (PANORAMA_1). 16 = the 4x4 kernel (rest), 4 = its inner 2x2.
// A measurement AND a taste gate: the tap cost on both GPUs, and whether the
// narrower penumbra is the piece. Range covers only the two legal values.
ORGAN_PARAM(CONFIG, GPUDesignConfig, shadow_pcf_taps, U32, 4.0f, 16.0f, 12.0f, "Measure", "sun PCF taps (4 or 16)")

// ─── Atrium — EDITS THE NEXT ENTRANCE ───────────────────────────
