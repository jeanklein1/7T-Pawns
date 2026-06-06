// ─── agents.inl ─────────────────────────────────────────────────
//
// Unified entity registry: the control panel for the agent system.
// Every pawn-like body on the board — the one the player inhabits and
// every mood-authored wanderer — is one slot in agentStateBuffer_,
// driven by one of these behaviors, tinted by one of these tiers,
// populated per mood from AGENT_POPULATIONS.
//
// The player's relationship to this array is `player_.possessed_slot`;
// the compute kernel treats that slot as the PlayerControlled branch
// and every other active slot as its authored behavior. See
// agent_system_design.md for the full rationale.
//
// ┌─── Three registries ────────────────────────────────────────────┐
// │                                                                  │
// │  AGENT_BEHAVIORS    per-behavior motion parameters               │
// │                     (step rate, step size, drag, speed cap, ...) │
// │                                                                  │
// │  AGENT_TIER_GAINS   per-tier multipliers + render color          │
// │                     (Worker / Scout / Sentinel / Leader)         │
// │                                                                  │
// │  AGENT_POPULATIONS  per-mood population authoring                │
// │                     (count, behavior/tier weights, spawn radius) │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// All ten behaviors (PlayerControlled + nine algorithmic) are
// implemented and tunable. Tier gains are authored for all four
// tiers. Populations are authored for the outdoor + gallery moods;
// the two finite_outdoor moods default to count=0 (unpopulated).
//
// ┌─── Public surface (called from outside this file) ──────────────┐
// │                                                                  │
// │  Module functions are static, take AgentState& explicitly        │
// │  (or const AgentState& when read-only).                          │
// │                                                                  │
// │  Lifecycle:                                                      │
// │    upload_agent_registries_to_gpu(c, queue)   — once at init     │
// │      (note: takes no AgentState — uploads constexpr registries   │
// │       only, doesn't touch agent slots)                           │
// │    spawn_population_for_mood(as, c, ...)      — mood entry       │
// │    respawn_evicted_agents(as, c, ...)         — every-frame fill │
// │                                                                  │
// │  Player commands:                                                │
// │    try_possess_nearest(as, c, queue)          — Caps Lock        │
// │                                                                  │
// │  Diagnostic cycling (wired in input.inl):                       │
// │    cycle_agent_behavior_override(as, c, q)    F1                 │
// │    cycle_agent_tier_override(as, c, q)        F2                 │
// │    force_respawn_population(as, c, q)         F3                 │
// │                                                                  │
// │  Logging:                                                        │
// │    dump_agent_census(as, c, trigger)          — periodic+on event│
// │                                                                  │
// │  Cross-module reads (consumed by other modules):                 │
// │    agent_state_.slots[player_.possessed_slot] — read by spine,  │
// │      cube_behaviors.inl (player position queries)                │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// ┌─── Why not EntityFamilyAdapter? ─────────────────────────────────┐
// │                                                                  │
// │  Other living things in the world (blades, palms, arches, etc.)  │
// │  flow through entity_pipeline.inl's generic select / place /     │
// │  commit machinery. Agents are deliberately separate because:     │
// │                                                                  │
// │   • Fixed-size 32-slot array, not streaming. No per-frame        │
// │     entity_refs eviction; the agent kernel decides who lives.    │
// │   • Player-possessable. Slot PLAYER_SLOT is special-cased on     │
// │     both CPU (camera, input, readback) and GPU (separate kernel  │
// │     update_player_agent vs update_other_agents).                 │
// │   • Has its own state-evolution kernel; entities are placed once │
// │     and only re-rendered.                                        │
// │                                                                  │
// │  The `tier` vocabulary overlaps but the machinery is unrelated:  │
// │  AgentTierDef is not EntityFamilyTraits::TierProfile.            │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// Included inside the Cartridge class body, after orbs.inl.
// Depends on: state.hpp (Dim::MAX_AGENTS), musical.inl (MMODE_COUNT),
//             MOOD_COUNT in enclosing class.
// ─────────────────────────────────────────────────────────────────


// ═══ BEHAVIOR IDS ════════════════════════════════════════════════
//
// Stable indices into AGENT_BEHAVIORS. The compute kernel's behavior
// switch dispatches on these values. Names below are also exported as
// AGENT_BEHAVIOR_NAMES[] for diagnostics — keep the two in lockstep.

enum AgentBehaviorId : uint32_t {
    AGENT_BEHAVIOR_PLAYER_CONTROLLED = 0,
    AGENT_BEHAVIOR_RANDOM_WALK       = 1,
    AGENT_BEHAVIOR_BIASED_WALK       = 2,   // persistent direction + soft cohesion
    AGENT_BEHAVIOR_WANDERER          = 3,   // random walk + soft home tether
    AGENT_BEHAVIOR_HOME_SEEKER       = 4,   // strong spring to home
    AGENT_BEHAVIOR_SLOW_PATROL       = 5,   // waypoints around home, slow
    AGENT_BEHAVIOR_PURSUIT           = 6,   // steers toward player when in range
    AGENT_BEHAVIOR_FLEE              = 7,   // flees player when in range, idles otherwise
    AGENT_BEHAVIOR_FLOCK2D           = 8,   // Vicsek alignment + cohesion
    AGENT_BEHAVIOR_LEVY_FLIGHT       = 9,   // power-law step magnitudes
    AGENT_BEHAVIOR_COUNT             = 10,
};


// ═══ TIER IDS ════════════════════════════════════════════════════
//
// Visual + parametric archetype — a property of the body, not of the
// driver. A Scout stays a Scout when the player leaves it; a
// RandomWalk scout moves with Scout-tier speed/persistence.

enum AgentTierId : uint32_t {
    AGENT_TIER_WORKER   = 0,
    AGENT_TIER_SCOUT    = 1,
    AGENT_TIER_SENTINEL = 2,
    AGENT_TIER_LEADER   = 3,
    AGENT_TIER_COUNT    = 4,
};

// Cross-check: GPU-side count constants in state.hpp must match the
// authoritative enums above. If you add a behavior or a tier, bump
// the GPU constant in state.hpp at the same time. The compiler will
// catch any drift here.
static_assert(AGENT_BEHAVIOR_COUNT == GPU_AGENT_BEHAVIOR_COUNT,
    "AGENT_BEHAVIOR_COUNT must match GPU_AGENT_BEHAVIOR_COUNT in state.hpp");
static_assert(AGENT_TIER_COUNT == GPU_AGENT_TIER_COUNT,
    "AGENT_TIER_COUNT must match GPU_AGENT_TIER_COUNT in state.hpp");

// Display names for diagnostics (census output, error messages).
// Order MUST match the enums above — index by the enum value.
// Mirrors the ORB_PAL_NAMES / ORB_TIERSET_NAMES pattern from orbs.inl.
static constexpr const char* AGENT_BEHAVIOR_NAMES[AGENT_BEHAVIOR_COUNT] = {
    "player",        //  0  PLAYER_CONTROLLED
    "random_walk",   //  1  RANDOM_WALK
    "biased_walk",   //  2  BIASED_WALK
    "wanderer",      //  3  WANDERER
    "home_seeker",   //  4  HOME_SEEKER
    "slow_patrol",   //  5  SLOW_PATROL
    "pursuit",       //  6  PURSUIT
    "flee",          //  7  FLEE
    "flock2d",       //  8  FLOCK2D
    "levy_flight",   //  9  LEVY_FLIGHT
};

static constexpr const char* AGENT_TIER_NAMES[AGENT_TIER_COUNT] = {
    "worker",        //  0
    "scout",         //  1
    "sentinel",      //  2
    "leader",        //  3
};


// ═══ TUNING CONSOLE ══════════════════════════════════════════════
//
// All authored radii live here so the relationships between them are
// visible at a glance. Mirrors the tuning-console block at the top of
// orbs.inl. If you change one of these, scan the comments below to
// see what else may need to move.

// PLAYER_SLOT — slot 0 of agentStateBuffer_ is the player's INITIAL
// body and the canonical "skip this in respawn loops" sentinel. Every
// spawn/respawn skips slot 0. Caps Lock possession transfer can move
// the player into a non-zero slot (player_.possessed_slot tracks
// which one); reads of "where is the player right now" should go
// through agent_state_.slots[player_.possessed_slot], not [PLAYER_SLOT].
// DONE[agents:L1] comment updated to reflect possession transfer.
static constexpr uint32_t PLAYER_SLOT = 0;

// POSSESSION_RADIUS — Caps Lock teleports the player into the nearest
// active agent within this radius. Set so the player can comfortably
// pick a target by walking near it; any larger and possession becomes
// "leap to wherever Caps Lock feels like it." See try_possess_nearest.
static constexpr float POSSESSION_RADIUS    = 20.0f;
static constexpr float POSSESSION_RADIUS_SQ = POSSESSION_RADIUS * POSSESSION_RADIUS;

// AGENT_EVICTION_RADIUS — outdoor agents that drift further than this
// from the player are evicted (set is_active=0 by the GPU kernel) and
// later refilled by respawn_evicted_agents.
//
// SEAM[agents:L2] hardware mirror — AGENT_EVICTION_RADIUS must agree
//   with world.wgsl's identically-named const. The compiler cannot
//   catch drift; the prose below is the contract.
//
// MIRRORED MANUALLY in world.wgsl as `const AGENT_EVICTION_RADIUS:
// f32 = 360.0` (the WGSL needs a compile-time const for FXC inlining;
// no runtime upload exists). If you change this value, change the
// WGSL constant too — the compiler will not catch the drift.
//
// Couples with each population's spawn_radius below: a population
// whose spawn_radius approaches the eviction radius will see immediate
// re-eviction and look like it's flickering. Keep at least ~20 units
// of headroom (spawn_radius=340 + this=360 gives a 20-unit "alive"
// band where agents persist before being recycled).
static constexpr float AGENT_EVICTION_RADIUS    = 360.0f;
static constexpr float AGENT_EVICTION_RADIUS_SQ = AGENT_EVICTION_RADIUS * AGENT_EVICTION_RADIUS;

// AGENT_CENSUS_INTERVAL — period (seconds) between automatic
// [AGENTS] census log lines + the [Player] position emission.
static constexpr float AGENT_CENSUS_INTERVAL = 30.0f;


// ═══ REGISTRY: BEHAVIORS ═════════════════════════════════════════
//
// Per-behavior motion parameters. Units:
//   step_rate         steps per *beat* (musical time)
//   step_size         world units per step (BiasedWalk) /
//                     waypoint radius from home (SlowPatrol)
//   persistence       [0,1] — directional commitment (BiasedWalk:
//                     1=lock to travel azimuth, 0=full random)
//   drag              1/s — velocity decay coefficient (exponential)
//   home_pull         1/s² — spring coefficient (SlowPatrol uses for
//                     waypoint steering)
//   neighbor_radius   world units — flock/cohesion neighbor radius
//   speed_cap         world units/s — per-agent max speed
//
// PlayerControlled rows are all zero: the kernel switch case reads
// input directly rather than these parameters.
//
// Home tether (home_x/y/z in GPUAgentState) is consumed by:
//   WANDERER     — soft pull back to home, lets the agent meander
//   HOME_SEEKER  — strong spring to home, dominant force
//   SLOW_PATROL  — home is the patrol anchor; waypoints orbit it
// Other behaviors ignore the home position; spawn still seeds it
// (cheap, harmless) so the field is meaningful if a slot's behavior
// later changes via possession transfer or override.

struct AgentBehaviorDef {
    AgentBehaviorId id;
    const char*     name;
    float           step_rate;
    float           step_size;
    float           persistence;
    float           drag;
    float           home_pull;
    float           neighbor_radius;
    float           speed_cap;
};

static constexpr AgentBehaviorDef AGENT_BEHAVIORS[AGENT_BEHAVIOR_COUNT] = {
    //  id                                  name               step_rate  step_size  persistence  drag  home_pull  neighbor_radius  speed_cap
    { AGENT_BEHAVIOR_PLAYER_CONTROLLED, "player_controlled",   0.0f,      0.0f,      0.0f,        0.0f, 0.0f,      0.0f,            0.0f    },
    { AGENT_BEHAVIOR_RANDOM_WALK,       "random_walk",         0.8f,      1.5f,      0.0f,        3.0f, 0.0f,      0.0f,            3.0f    },
    { AGENT_BEHAVIOR_BIASED_WALK,       "biased_walk",         0.5f,      2.5f,      0.85f,       0.6f, 0.0f,      25.0f,           5.0f    },
    { AGENT_BEHAVIOR_WANDERER,          "wanderer",            0.7f,      1.8f,      0.0f,        1.0f, 0.4f,      0.0f,            4.0f    },
    { AGENT_BEHAVIOR_HOME_SEEKER,       "home_seeker",         0.4f,      1.0f,      0.0f,        1.5f, 3.0f,      0.0f,            3.0f    },
    { AGENT_BEHAVIOR_SLOW_PATROL,       "slow_patrol",         0.25f,     8.0f,      0.0f,        2.0f, 4.0f,      0.0f,            2.0f    },
    { AGENT_BEHAVIOR_PURSUIT,           "pursuit",             0.5f,      1.5f,      0.0f,        1.0f, 5.0f,      40.0f,           5.0f    },
    { AGENT_BEHAVIOR_FLEE,              "flee",                0.4f,      1.0f,      0.0f,        1.5f, 8.0f,      30.0f,           8.0f    },
    { AGENT_BEHAVIOR_FLOCK2D,           "flock2d",             0.6f,      1.5f,      0.7f,        0.6f, 0.0f,      30.0f,           4.5f    },
    { AGENT_BEHAVIOR_LEVY_FLIGHT,       "levy_flight",         0.4f,      0.8f,      0.0f,        1.5f, 0.0f,      0.0f,            8.0f    },
};

static_assert(sizeof(AGENT_BEHAVIORS) / sizeof(AGENT_BEHAVIORS[0]) == AGENT_BEHAVIOR_COUNT,
              "AGENT_BEHAVIORS must declare one row per AgentBehaviorId");


// ═══ REGISTRY: TIER GAINS ════════════════════════════════════════
//
// Per-tier multipliers on behavior parameters, plus render color.
// Compound with behavior params: a Scout running RandomWalk takes
// longer, less-persistent steps than a Worker running RandomWalk.
//
// Color authored as RGB [0,1]. The tier color is the body's identity
// — a Scout is bronze regardless of who's driving it.

struct AgentTierDef {
    AgentTierId id;
    const char* name;
    float       step_gain;       // multiplies step_size
    float       persist_gain;    // multiplies persistence
    float       speed_gain;      // multiplies speed_cap
    float       color_r;
    float       color_g;
    float       color_b;
};

static constexpr AgentTierDef AGENT_TIER_GAINS[AGENT_TIER_COUNT] = {
    //  id                     name        step  persist  speed  color
    { AGENT_TIER_WORKER,   "worker",   1.0f, 1.0f, 1.0f, 0.60f, 0.62f, 0.65f },  // slate gray
    { AGENT_TIER_SCOUT,    "scout",    1.8f, 0.4f, 1.4f, 0.85f, 0.65f, 0.40f },  // bronze
    { AGENT_TIER_SENTINEL, "sentinel", 0.6f, 1.2f, 0.5f, 0.30f, 0.40f, 0.70f },  // deep blue
    { AGENT_TIER_LEADER,   "leader",   1.2f, 0.9f, 1.1f, 0.95f, 0.85f, 0.55f },  // pale gold
};

static_assert(sizeof(AGENT_TIER_GAINS) / sizeof(AGENT_TIER_GAINS[0]) == AGENT_TIER_COUNT,
              "AGENT_TIER_GAINS must declare one row per AgentTierId");


// ═══ REGISTRY: POPULATIONS ═══════════════════════════════════════
//
// Per-mood population authoring. Indexed by mood id; one row per
// mood. `count = 0` means the mood spawns no agents (the player is
// alone — a valid configuration).
//
// behavior_weights[] and tier_weights[] are probabilities (any
// non-negative values; they're normalized at spawn time).
//
// Spawn distribution: agents appear on an *annulus* around the player
// (uniform area distribution). spawn_inner_radius and spawn_radius
// define the annulus bounds. When inner == 0, the annulus collapses
// to a uniform disk — used for indoor/gallery moods where there's no
// "horizon" to spawn beyond.
//
// Outdoor moods set inner just outside the visible horizon (~200) and
// outer near the patch pre-gen edge (~340), so agents appear at
// distance and walk inward. This matches the floater lifecycle.
// Gallery moods set inner = 0 and outer to the room's reach, so
// agents spawn within the room.
//
// home_seeding_radius is how far each agent's home tether offset
// ranges from its spawn point.

struct AgentPopulationDef {
    uint32_t mood_id;
    uint32_t count;                                          // 0..Dim::MAX_AGENTS-1
    std::array<float, AGENT_BEHAVIOR_COUNT> behavior_weights;
    std::array<float, AGENT_TIER_COUNT>     tier_weights;
    float    spawn_inner_radius;                             // world units (annulus inner)
    float    spawn_radius;                                   // world units (annulus outer)
    float    home_seeding_radius;                            // world units from spawn point
};

// ─── Why no constexpr helper builders ───────────────────────────
//
// An earlier draft factored the weight arrays through helpers like
// `bw_only(AGENT_BEHAVIOR_BIASED_WALK)`. MSVC rejected it: agents.inl
// is included into the Cartridge class body, so the helpers became
// static member functions of an incomplete class, and AGENT_POPULATIONS
// (a class-body constexpr initializer) cannot call them at constant-
// expression time. Same restriction the ground_architecture.inl
// comment warns about for static_asserts; here it bites the array
// initializer instead.
//
// The fix is plain literal initialization with column-header comments
// above each row indicating which behavior / tier slot is non-zero.
// std::array accepts braced lists via brace elision (single braces
// suffice), so the rows stay readable.

// Mood ordering matches MOOD_TABLE in cartridge.hpp. The named
// constants (MOOD_OPEN_DEFAULT, etc.) and per-row static_asserts
// below catch any reordering at compile time.
//
// Outdoor moods spawn agents on a wide annulus near the patch
// pre-gen edge — agents appear at distance, walk inward, evict at
// the world's horizon. Gallery moods use a tight annulus inside the
// indoor world bounds; agents spawn within visibility because there
// is no "horizon" inside a room.
//
// Each row's behavior_weights and tier_weights are preceded by a
// column-header comment so the active slot is visible without
// counting zeros.
static constexpr AgentPopulationDef AGENT_POPULATIONS[MOOD_COUNT] = {
    /* MOOD_OPEN_DEFAULT — desert travelers (BiasedWalk) */
    { /*mood_id=*/ MOOD_OPEN_DEFAULT, /*count=*/ 10,
      //                       player rwalk  bwalk wandr hseek slowp pursu  flee flock  levy
      /*behavior_weights=*/ {    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
      //                     worker scout sentl leadr
      /*tier_weights=*/     {  2.0f, 2.0f, 1.0f, 0.0f },
      /*spawn_inner_radius=*/ 200.0f,
      /*spawn_radius=*/       340.0f,
      /*home_seeding_radius=*/ 5.0f },
    /* MOOD_OPEN_SUNSET — Scout-heavy travelers (BiasedWalk) */
    { /*mood_id=*/ MOOD_OPEN_SUNSET, /*count=*/ 10,
      //                       player rwalk  bwalk wandr hseek slowp pursu  flee flock  levy
      /*behavior_weights=*/ {    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
      //                     worker scout sentl leadr
      /*tier_weights=*/     {  1.0f, 3.0f, 0.0f, 0.0f },
      /*spawn_inner_radius=*/ 200.0f,
      /*spawn_radius=*/       340.0f,
      /*home_seeding_radius=*/ 8.0f },
    /* MOOD_INDOOR_FLAT — gallery walkers (SlowPatrol) */
    { /*mood_id=*/ MOOD_INDOOR_FLAT, /*count=*/ 4,
      //                       player rwalk  bwalk wandr hseek slowp pursu  flee flock  levy
      /*behavior_weights=*/ {    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f },
      //                     worker scout sentl leadr
      /*tier_weights=*/     {  2.0f, 0.0f, 2.0f, 1.0f },
      /*spawn_inner_radius=*/ 0.0f,
      /*spawn_radius=*/       60.0f,
      /*home_seeding_radius=*/ 30.0f },
    /* MOOD_INDOOR_VAULT — gallery walkers (SlowPatrol) */
    { /*mood_id=*/ MOOD_INDOOR_VAULT, /*count=*/ 4,
      //                       player rwalk  bwalk wandr hseek slowp pursu  flee flock  levy
      /*behavior_weights=*/ {    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f },
      //                     worker scout sentl leadr
      /*tier_weights=*/     {  2.0f, 0.0f, 2.0f, 1.0f },
      /*spawn_inner_radius=*/ 0.0f,
      /*spawn_radius=*/       60.0f,
      /*home_seeding_radius=*/ 30.0f },
    /* MOOD_FINITE_OUTDOOR — unpopulated */
    { /*mood_id=*/ MOOD_FINITE_OUTDOOR, /*count=*/ 0,
      /*behavior_weights=*/ {},
      /*tier_weights=*/     {},
      /*spawn_inner_radius=*/ 0.0f,
      /*spawn_radius=*/       0.0f,
      /*home_seeding_radius=*/ 0.0f },
    /* MOOD_FINITE_OUTDOOR_REF — unpopulated */
    { /*mood_id=*/ MOOD_FINITE_OUTDOOR_REF, /*count=*/ 0,
      /*behavior_weights=*/ {},
      /*tier_weights=*/     {},
      /*spawn_inner_radius=*/ 0.0f,
      /*spawn_radius=*/       0.0f,
      /*home_seeding_radius=*/ 0.0f },
};

static_assert(sizeof(AGENT_POPULATIONS) / sizeof(AGENT_POPULATIONS[0]) == MOOD_COUNT,
              "AGENT_POPULATIONS must declare one row per mood");

// Row order must match the mood ids in MOOD_TABLE (cartridge.hpp).
// Unfolded rather than written as a constexpr loop because class-body
// static_asserts cannot call member constexpr functions (the enclosing
// type isn't complete during parsing). Same workaround as in
// ground_architecture.inl.
static_assert(AGENT_POPULATIONS[MOOD_OPEN_DEFAULT      ].mood_id == MOOD_OPEN_DEFAULT,       "AGENT_POPULATIONS row 0 must be MOOD_OPEN_DEFAULT");
static_assert(AGENT_POPULATIONS[MOOD_OPEN_SUNSET       ].mood_id == MOOD_OPEN_SUNSET,        "AGENT_POPULATIONS row 1 must be MOOD_OPEN_SUNSET");
static_assert(AGENT_POPULATIONS[MOOD_INDOOR_FLAT       ].mood_id == MOOD_INDOOR_FLAT,        "AGENT_POPULATIONS row 2 must be MOOD_INDOOR_FLAT");
static_assert(AGENT_POPULATIONS[MOOD_INDOOR_VAULT      ].mood_id == MOOD_INDOOR_VAULT,       "AGENT_POPULATIONS row 3 must be MOOD_INDOOR_VAULT");
static_assert(AGENT_POPULATIONS[MOOD_FINITE_OUTDOOR    ].mood_id == MOOD_FINITE_OUTDOOR,     "AGENT_POPULATIONS row 4 must be MOOD_FINITE_OUTDOOR");
static_assert(AGENT_POPULATIONS[MOOD_FINITE_OUTDOOR_REF].mood_id == MOOD_FINITE_OUTDOOR_REF, "AGENT_POPULATIONS row 5 must be MOOD_FINITE_OUTDOOR_REF");


// ═══ REGISTRY UPLOAD (CPU table → GPU buffer, once at world-init) ═
//
// AGENT_BEHAVIORS and AGENT_TIER_GAINS are the single source of
// truth for behavior/tier parameters. The compute kernels read them
// from GPU storage buffers (bindings 110 / 111), uploaded once at
// world-init by this helper. Values are constexpr-equivalent —
// they never change during a session, so a one-shot upload at boot
// is sufficient.
//
// The translation is a straight memcpy: AgentBehaviorDef and
// GPUAgentBehaviorDef are intentionally laid out so the float
// fields share order. The CPU struct has extra leading fields
// (`id`, `name`) that aren't uploaded; we copy from `step_rate`
// onward by offset.

// upload_agent_registries_to_gpu: takes Cartridge* for gpuState_
// access. No agent state needed — uploads constexpr registries only.
static void upload_agent_registries_to_gpu(Cartridge* c, wgpu::Queue& queue) {
    GPUAgentBehaviorDef gpu_behaviors[AGENT_BEHAVIOR_COUNT] = {};
    for (uint32_t i = 0; i < AGENT_BEHAVIOR_COUNT; i++) {
        const auto& src = AGENT_BEHAVIORS[i];
        gpu_behaviors[i].step_rate       = src.step_rate;
        gpu_behaviors[i].step_size       = src.step_size;
        gpu_behaviors[i].persistence     = src.persistence;
        gpu_behaviors[i].drag            = src.drag;
        gpu_behaviors[i].home_pull       = src.home_pull;
        gpu_behaviors[i].neighbor_radius = src.neighbor_radius;
        gpu_behaviors[i].speed_cap       = src.speed_cap;
        gpu_behaviors[i]._pad            = 0.0f;
    }

    GPUAgentTierDef gpu_tiers[AGENT_TIER_COUNT] = {};
    for (uint32_t i = 0; i < AGENT_TIER_COUNT; i++) {
        const auto& src = AGENT_TIER_GAINS[i];
        gpu_tiers[i].step_gain     = src.step_gain;
        gpu_tiers[i].persist_gain  = src.persist_gain;
        gpu_tiers[i].speed_gain    = src.speed_gain;
        gpu_tiers[i].color_r       = src.color_r;
        gpu_tiers[i].color_g       = src.color_g;
        gpu_tiers[i].color_b       = src.color_b;
        gpu_tiers[i]._pad[0] = 0.0f;
        gpu_tiers[i]._pad[1] = 0.0f;
    }

    c->gpuState_.upload_agent_registries(queue,
        gpu_behaviors, AGENT_BEHAVIOR_COUNT,
        gpu_tiers,     AGENT_TIER_COUNT);
}


// ═══ AGENT MODULE STATE (Scope B migration #3) ══════════════════
//
// All agent-owned state lives in this struct, accessed via
// agent_state_ on the Cartridge. Module functions take
// `AgentState& as` explicitly rather than reaching via Cartridge*,
// making ownership language-visible and dependencies explicit in
// signatures.
//
// Fields:
//
//   slots — Host-side mirror of agentStateBuffer_ refreshed from a
//     GPU readback. Used by Caps Lock targeting (nearest-within-
//     radius) and by patch streaming (which reads the possessed
//     slot's XZ).
//
//   respawn_counters — Per-slot respawn count. Mixed into the seed
//     each time a slot is refilled, so successive respawns of the
//     same slot roll different attributes / pose.
//
//   behavior_override / tier_override — Inspection toggles. When set,
//     every active non-player slot is forced to the override value,
//     bypassing the population's natural rolls. AGENT_OVERRIDE_NONE
//     means "no override — population wins." Overrides apply both to
//     existing active agents (cycle_*_override rewrites them in
//     place) and to newly-respawned agents (populate_agent_slot_
//     consults the override). Clearing the override does NOT revert
//     existing agents — only future respawns roll naturally again.
//     Intended as developer/diagnostic dials, not gameplay state.
//
//   last_census_dump — Periodic census log timestamp. Read +
//     written from the cartridge's update loop.

static constexpr uint32_t AGENT_OVERRIDE_NONE = 0xFFFFFFFFu;

struct AgentState {
    GPUAgentState slots[Dim::MAX_AGENTS]            = {};
    uint32_t      respawn_counters[Dim::MAX_AGENTS] = {};
    uint32_t      behavior_override                 = AGENT_OVERRIDE_NONE;
    uint32_t      tier_override                     = AGENT_OVERRIDE_NONE;
    float         last_census_dump                  = -999.0f;
};
AgentState agent_state_;


// ═══ SHARED POPULATION HELPER ════════════════════════════════════
//
// One source of truth for "given a population and a seed, fill this
// slot's GPUAgentState." Used by both spawn_population_for_mood
// (mood entry, full-array refill) and respawn_evicted_agents
// (per-frame, evicted-slot refill). The two callers differ only in:
//   • their seed-mixing recipe (different per-call so the same slot
//     produces different agents on successive respawns)
//   • their disk center (mood spawn uses the mood-spawn center;
//     respawn trails the player's current XZ)
//   • their upload strategy (full-array vs. per-slot)
// Everything else — weight rolling, annulus sampling, home offset,
// field initialization — lives here.
//
// beh_sum / tier_sum are passed in (not recomputed) so callers
// processing many slots don't redo the sum on each call.

static void populate_agent_slot_(const AgentState& as,
                          GPUAgentState& out,
                          const AgentPopulationDef& pop,
                          uint32_t agent_seed,
                          float beh_sum, float tier_sum,
                          float center_x, float center_z) {
    // ── Roll behavior (or honor override) ─────────────────────────
    uint32_t behavior_id = AGENT_BEHAVIOR_RANDOM_WALK;
    if (as.behavior_override != AGENT_OVERRIDE_NONE) {
        behavior_id = as.behavior_override;
    } else {
        float roll = cpu_hash_f(agent_seed, 1u);
        float cum = 0.0f;
        for (uint32_t b = 0; b < AGENT_BEHAVIOR_COUNT; b++) {
            cum += pop.behavior_weights[b] / beh_sum;
            if (roll < cum) { behavior_id = b; break; }
        }
    }

    // ── Roll tier (or honor override) ─────────────────────────────
    uint32_t tier_idx = AGENT_TIER_WORKER;
    if (as.tier_override != AGENT_OVERRIDE_NONE) {
        tier_idx = as.tier_override;
    } else {
        float roll = cpu_hash_f(agent_seed, 2u);
        float cum = 0.0f;
        for (uint32_t t = 0; t < AGENT_TIER_COUNT; t++) {
            cum += pop.tier_weights[t] / tier_sum;
            if (roll < cum) { tier_idx = t; break; }
        }
    }

    // ── Sample annulus position (uniform area distribution) ───────
    //   r² = inner² + uniform·(outer² − inner²)
    // When inner == 0, this collapses to a uniform disk (gallery
    // moods that spawn anywhere in the room).
    const float two_pi = 6.28318530718f;
    float theta = cpu_hash_f(agent_seed, 3u) * two_pi;
    const float inner_sq = pop.spawn_inner_radius * pop.spawn_inner_radius;
    const float outer_sq = pop.spawn_radius       * pop.spawn_radius;
    float u = cpu_hash_f(agent_seed, 4u);
    float r = std::sqrt(inner_sq + u * (outer_sq - inner_sq));
    float sx = center_x + std::cos(theta) * r;
    float sz = center_z + std::sin(theta) * r;

    // ── Sample home offset (uniform on disk around spawn point) ───
    float h_theta = cpu_hash_f(agent_seed, 5u) * two_pi;
    float h_r     = std::sqrt(cpu_hash_f(agent_seed, 6u)) * pop.home_seeding_radius;
    float hx = sx + std::cos(h_theta) * h_r;
    float hz = sz + std::sin(h_theta) * h_r;

    // ── Write the slot ────────────────────────────────────────────
    out.pos_x   = sx;   out.pos_y   = 0.0f; out.pos_z   = sz;
    out.home_x  = hx;   out.home_y  = 0.0f; out.home_z  = hz;
    out.heading = 0.0f;
    out.vel_x   = 0.0f; out.vel_y   = 0.0f; out.vel_z   = 0.0f;
    out.orient_x = 0.0f; out.orient_y = 0.0f; out.orient_z = 0.0f; out.orient_w = 1.0f;
    out.seed           = agent_seed;
    out.behavior_id    = behavior_id;
    out.tier_idx       = tier_idx;
    out.is_active      = 1u;
    out.portal_trigger = -1;

    // Body color — random pick from COLUMN_PALETTE, deterministic from the
    // agent seed (salt 7u). Resolved CPU-side so the slot carries final RGB
    // (mirrors how columns upload final color).
    uint32_t ci = cpu_hash(agent_seed, 7u) % COLUMN_PALETTE_COUNT;
    out.color_r = COLUMN_PALETTE[ci][0];
    out.color_g = COLUMN_PALETTE[ci][1];
    out.color_b = COLUMN_PALETTE[ci][2];
    out._pad0   = 0.0f;
}


// ═══ SPAWN ════════════════════════════════════════════════════════
//
// Deterministic mood-driven population. Preserves slot 0 (player),
// clears 1..MAX_AGENTS-1, then fills the first `count` non-player
// slots by rolling behavior + tier from AGENT_POPULATIONS[mood_id]
// weights.
//
// Position is uniform on an annulus around (center_x, center_z),
// with bounds pop.spawn_inner_radius and pop.spawn_radius. Outdoor
// moods set the inner radius just outside the visible horizon so
// agents appear at distance; gallery moods set inner = 0 so agents
// spawn anywhere in the room.
//
// home tether offset is uniform on a disk of radius
// home_seeding_radius around each spawn point.
//
// Called once at boot (for the initial mood) and once per mood
// transition, after reset_player_agent + apply_mood. Uploads the
// full 32-slot array — slot 0's re-upload is idempotent as long as
// agent_state_.slots[0] mirrors the player's idle pose.

static void spawn_population_for_mood(AgentState& as, Cartridge* c,
                               uint32_t mood_id,
                               uint32_t seed,
                               float center_x, float center_z,
                               wgpu::Queue& queue) {
    if (mood_id >= MOOD_COUNT) return;
    const auto& pop = AGENT_POPULATIONS[mood_id];

    // Zero every non-player slot before refilling. The player's body
    // (slot PLAYER_SLOT) is preserved across mood transitions.
    for (uint32_t s = PLAYER_SLOT + 1; s < Dim::MAX_AGENTS; s++) {
        as.slots[s] = GPUAgentState{};
    }

    float beh_sum = 0.0f;
    for (uint32_t b = 0; b < AGENT_BEHAVIOR_COUNT; b++) beh_sum += pop.behavior_weights[b];
    float tier_sum = 0.0f;
    for (uint32_t t = 0; t < AGENT_TIER_COUNT; t++) tier_sum += pop.tier_weights[t];

    uint32_t spawned = 0;
    const uint32_t n = std::min(pop.count, Dim::MAX_AGENTS - 1u);
    if (n > 0 && beh_sum > 0.0f && tier_sum > 0.0f) {
        for (uint32_t i = 0; i < n; i++) {
            // Slot 0 is reserved for PLAYER_SLOT; non-player slots
            // pack densely from slot 1 upward.
            uint32_t slot = i + 1u;
            uint32_t agent_seed = cpu_hash(cpu_hash(seed, 0xA6E00000u + mood_id), i + 1u);

            populate_agent_slot_(as, as.slots[slot], pop, agent_seed,
                                 beh_sum, tier_sum,
                                 center_x, center_z);
            spawned++;
        }
    }

    c->gpuState_.upload_agent_state_all(queue, as.slots);
    std::cout << "[Agents] Spawned " << spawned << " for mood " << mood_id
              << " around (" << center_x << "," << center_z << ")\n";
}


// ═══ RESPAWN (per-frame, evicted slots → fresh agents) ════════════
//
// The GPU's update_agents kernel marks any non-player slot whose XZ
// distance from the possessed slot exceeds AGENT_EVICTION_RADIUS as
// inactive. This routine runs every frame on the CPU side: scans
// agent_state_.slots[1..MAX_AGENTS-1] for is_active == 0, refills each with
// a fresh agent in a disk around the *player's current position*
// (not the original mood spawn center — the population trails the
// player as they wander).
//
// Each respawn uses a per-slot counter to advance the seed, so the
// same slot in the same world produces a different agent on each
// successive respawn cycle (otherwise an agent that drifts out and
// gets re-evicted would respawn into the same place forever).
//
// Writes only the changed slots — never the player slot — so the
// GPU's per-frame player update never sees a stale CPU snapshot.
// (respawn_counters lives in the CPU MIRROR section above.)

static void respawn_evicted_agents(AgentState& as, Cartridge* c,
                            uint32_t mood_id,
                            uint32_t world_seed,
                            wgpu::Queue& queue) {
    if (mood_id >= MOOD_COUNT) return;
    const auto& pop = AGENT_POPULATIONS[mood_id];
    if (pop.count == 0) return;

    float beh_sum = 0.0f;
    for (uint32_t b = 0; b < AGENT_BEHAVIOR_COUNT; b++) beh_sum += pop.behavior_weights[b];
    float tier_sum = 0.0f;
    for (uint32_t t = 0; t < AGENT_TIER_COUNT; t++) tier_sum += pop.tier_weights[t];
    if (beh_sum <= 0.0f || tier_sum <= 0.0f) return;

    const uint32_t possessed = c->player_.possessed_slot;
    const float px = as.slots[possessed].pos_x;
    const float pz = as.slots[possessed].pos_z;

    const uint32_t n = std::min(pop.count, Dim::MAX_AGENTS - 1u);
    uint32_t respawned = 0;

    for (uint32_t i = 0; i < n; i++) {
        // Non-player slots pack densely from slot 1 upward.
        uint32_t slot = i + 1u;
        if (slot == possessed) continue;
        if (as.slots[slot].is_active != 0u) continue;

        as.respawn_counters[slot]++;
        uint32_t agent_seed = cpu_hash(
            cpu_hash(world_seed, 0xA6E00000u + mood_id),
            slot * 0x10001u + as.respawn_counters[slot] * 0x100u);

        // Center on the player so the population trails the player as
        // they wander, rather than refilling at the original mood-spawn
        // anchor (which would empty out as soon as they walked away).
        populate_agent_slot_(as, as.slots[slot], pop, agent_seed,
                             beh_sum, tier_sum,
                             px, pz);

        c->gpuState_.upload_agent_slot(queue, slot, &as.slots[slot]);
        respawned++;
    }

    if (respawned > 0) {
        std::cout << "[Agents] Respawn " << respawned
                  << " around (" << px << "," << pz << ")\n";
    }
}


// ═══ POSSESSION TRANSFER (Caps Lock) ══════════════════════════════
//
// Player jumps from their current body to the nearest active non-
// player slot within POSSESSION_RADIUS. The vacated slot stays where
// it was, switches to RANDOM_WALK, and continues under autopilot —
// it remains a body in the world, just no longer driven by input.
// The new slot keeps its tier (and tier color), inherits the camera,
// inherits portal-detection, and resets velocity to zero so the
// player has clean control on entry.
//
// Blocked during portal transitions to avoid mid-teardown swaps.
// No-op when no candidate is in range.
// (POSSESSION_RADIUS lives in the TUNING CONSOLE at the top of the file.)

static void try_possess_nearest(AgentState& as, Cartridge* c, wgpu::Queue& queue) {
    if (c->transitionPhase_ != TransitionPhase::IDLE) {
        std::cout << "[Possess] Blocked (mid-transition)\n";
        return;
    }

    const uint32_t cur = c->player_.possessed_slot;
    const float px = as.slots[cur].pos_x;
    const float pz = as.slots[cur].pos_z;

    int best_slot = -1;
    float best_d2 = POSSESSION_RADIUS_SQ;
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
        std::cout << "[Possess] No agent within " << POSSESSION_RADIUS
                  << " units of player at (" << px << "," << pz << ")\n";
        return;
    }

    const uint32_t new_slot = (uint32_t)best_slot;

    // Old slot → autopilot RandomWalk. Slot 0's seed is zero by default
    // (reset_player_agent leaves it zero-init); give it a fresh seed so
    // its random walk doesn't lock to hash(0, ...).
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


// ═══ DIAGNOSTIC CYCLING ══════════════════════════════════════════
//
// Runtime knobs for inspecting behaviors and tiers without editing
// AGENT_POPULATIONS and recompiling. Mirrors the orbs.inl pattern of
// player-cycleable system-wide dials (cycle_orb_motion_rule, etc.).
//
// Wired in input.inl alongside the existing CapsLock → possess.
// Originally bound to B/T/R; reassigned to function keys when we
// realized A-Z plays MIDI piano in the analysis layer above the
// cartridge — letter keys can't double as cartridge diagnostics
// without playing notes every press.
//   F1 → cycle_agent_behavior_override   step through nine algorithmic
//                                        behaviors, then back to none
//   F2 → cycle_agent_tier_override       step through four tiers, then
//                                        back to none
//   F3 → force_respawn_population        re-roll the current mood's
//                                        population around the player
//
// These rewrite agent_state_.slots in place and re-upload the changed slots,
// so the effect is immediate. PlayerControlled is excluded from the
// behavior cycle — overriding every body to that would orphan input.

// Walk every active non-player slot and apply whichever overrides
// are currently set. No-op for slots that already match. Helper
// shared by cycle_agent_behavior_override and cycle_agent_tier_override.
static void apply_agent_overrides_(AgentState& as, Cartridge* c, wgpu::Queue& queue) {
    for (uint32_t s = PLAYER_SLOT + 1; s < Dim::MAX_AGENTS; s++) {
        auto& a = as.slots[s];
        if (a.is_active == 0u) continue;
        bool changed = false;
        if (as.behavior_override != AGENT_OVERRIDE_NONE
            && a.behavior_id != as.behavior_override) {
            a.behavior_id = as.behavior_override;
            changed = true;
        }
        if (as.tier_override != AGENT_OVERRIDE_NONE
            && a.tier_idx != as.tier_override) {
            a.tier_idx = as.tier_override;
            changed = true;
        }
        if (changed) {
            c->gpuState_.upload_agent_slot(queue, s, &as.slots[s]);
        }
    }
}

// Cycle: NONE → RANDOM_WALK → BIASED_WALK → ... → LEVY_FLIGHT → NONE.
// PlayerControlled (id 0) is skipped — putting every body under input
// control is not a meaningful diagnostic state.
static void cycle_agent_behavior_override(AgentState& as, Cartridge* c, wgpu::Queue& queue) {
    if (as.behavior_override == AGENT_OVERRIDE_NONE) {
        as.behavior_override = AGENT_BEHAVIOR_RANDOM_WALK;
    } else {
        uint32_t next = as.behavior_override + 1u;
        as.behavior_override = (next >= AGENT_BEHAVIOR_COUNT)
            ? AGENT_OVERRIDE_NONE : next;
    }

    apply_agent_overrides_(as, c, queue);

    if (as.behavior_override == AGENT_OVERRIDE_NONE) {
        std::cout << "[Agents] behavior override: none\n";
    } else {
        std::cout << "[Agents] behavior override: "
                  << AGENT_BEHAVIOR_NAMES[as.behavior_override] << "\n";
    }
}

// Cycle: NONE → WORKER → SCOUT → SENTINEL → LEADER → NONE.
static void cycle_agent_tier_override(AgentState& as, Cartridge* c, wgpu::Queue& queue) {
    if (as.tier_override == AGENT_OVERRIDE_NONE) {
        as.tier_override = AGENT_TIER_WORKER;
    } else {
        uint32_t next = as.tier_override + 1u;
        as.tier_override = (next >= AGENT_TIER_COUNT)
            ? AGENT_OVERRIDE_NONE : next;
    }

    apply_agent_overrides_(as, c, queue);

    if (as.tier_override == AGENT_OVERRIDE_NONE) {
        std::cout << "[Agents] tier override: none\n";
    } else {
        std::cout << "[Agents] tier override: "
                  << AGENT_TIER_NAMES[as.tier_override] << "\n";
    }
}

// Mark every non-player slot inactive. respawn_evicted_agents (called
// every frame from the main tick) will refill them in a disk around
// the player on the next pass — same path as natural eviction-driven
// refill, so no new code path. Useful for re-rolling a population
// after editing weights or just to "shuffle" what's around the player.
static void force_respawn_population(AgentState& as, Cartridge* c, wgpu::Queue& queue) {
    uint32_t cleared = 0;
    for (uint32_t s = PLAYER_SLOT + 1; s < Dim::MAX_AGENTS; s++) {
        if (as.slots[s].is_active == 0u) continue;
        as.slots[s].is_active = 0u;
        c->gpuState_.upload_agent_slot(queue, s, &as.slots[s]);
        cleared++;
    }
    std::cout << "[Agents] force-respawn cleared " << cleared
              << " slot(s); refill on next frame\n";
}


// ═══ DIAGNOSTIC: agent census ═════════════════════════════════════
//
// Snapshot of agent_state_.slots broken down by tier + behavior, plus the
// player's current possessed slot. Reads the same CPU mirror that
// drives possession queries and patch streaming, so it's only as
// fresh as the latest readback (one-frame lag is fine for a log).
//
// Loops over the full names arrays so adding a behavior or tier
// updates the census output automatically; only non-zero buckets are
// printed to keep lines readable.
//
// (last_census_dump lives in agent_state_; AGENT_CENSUS_INTERVAL
// in the TUNING CONSOLE at the top.)

static void dump_agent_census(const AgentState& as, const Cartridge* c, const char* trigger) {
    uint32_t active = 0;
    uint32_t by_behavior[AGENT_BEHAVIOR_COUNT] = {};
    uint32_t by_tier[AGENT_TIER_COUNT] = {};

    for (uint32_t i = 0; i < Dim::MAX_AGENTS; i++) {
        const auto& a = as.slots[i];
        if (a.is_active == 0u) continue;
        active++;
        if (a.behavior_id < AGENT_BEHAVIOR_COUNT) by_behavior[a.behavior_id]++;
        if (a.tier_idx     < AGENT_TIER_COUNT)     by_tier[a.tier_idx]++;
    }

    std::cout << "[AGENTS t=" << std::fixed << std::setprecision(1) << c->time_state_.seconds
              << " trigger=" << trigger << "] " << active << "/" << Dim::MAX_AGENTS
              << " active, possessed=" << c->player_.possessed_slot;

    std::cout << " tier:{";
    bool first = true;
    for (uint32_t t = 0; t < AGENT_TIER_COUNT; t++) {
        if (by_tier[t] == 0) continue;
        if (!first) std::cout << " ";
        std::cout << AGENT_TIER_NAMES[t] << "=" << by_tier[t];
        first = false;
    }
    std::cout << "}";

    std::cout << " drv:{";
    first = true;
    for (uint32_t b = 0; b < AGENT_BEHAVIOR_COUNT; b++) {
        if (by_behavior[b] == 0) continue;
        if (!first) std::cout << " ";
        std::cout << AGENT_BEHAVIOR_NAMES[b] << "=" << by_behavior[b];
        first = false;
    }
    std::cout << "}";

    // Override state — only printed when something is actually
    // overridden, to keep the line short in the common (no-override)
    // case.
    if (as.behavior_override != AGENT_OVERRIDE_NONE
        || as.tier_override != AGENT_OVERRIDE_NONE) {
        std::cout << " override:{";
        bool first_o = true;
        if (as.behavior_override != AGENT_OVERRIDE_NONE) {
            std::cout << "beh=" << AGENT_BEHAVIOR_NAMES[as.behavior_override];
            first_o = false;
        }
        if (as.tier_override != AGENT_OVERRIDE_NONE) {
            if (!first_o) std::cout << " ";
            std::cout << "tier=" << AGENT_TIER_NAMES[as.tier_override];
        }
        std::cout << "}";
    }

    std::cout << "\n";
}
