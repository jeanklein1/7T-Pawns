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
// Pass 1 fully implements PlayerControlled + RandomWalk; other
// behaviors are stubbed for Pass 2. Tier gains are authored for all
// four tiers (colors drive visual identity even when gains are 1×).
// Populations are authored for open_default and open_sunset; other
// moods default to count=0 (unpopulated).
//
// Included inside the Cartridge class body, after orbs.inl.
// Depends on: state.hpp (Dim::MAX_AGENTS), musical.inl (MMODE_COUNT),
//             MOOD_COUNT in enclosing class.
// ─────────────────────────────────────────────────────────────────


// ═══ BEHAVIOR IDS ════════════════════════════════════════════════
//
// Stable indices into AGENT_BEHAVIORS. The compute kernel's behavior
// switch dispatches on these values. Pass 1 implements the first two;
// the rest are reserved slots so the mood author and kernel switch
// can be written against the full set without churn when Pass 2 fills
// in the behavior bodies.

enum AgentBehaviorId : uint32_t {
    AGENT_BEHAVIOR_PLAYER_CONTROLLED = 0,
    AGENT_BEHAVIOR_RANDOM_WALK       = 1,
    AGENT_BEHAVIOR_CORRELATED_WALK   = 2,   // Pass 2 — rotated step direction
    AGENT_BEHAVIOR_WANDERER          = 3,   // Pass 2 — correlated + home tether
    AGENT_BEHAVIOR_HOME_SEEKER       = 4,   // Pass 2 — strong spring to home
    AGENT_BEHAVIOR_PATROL            = 5,   // Pass 2 — deterministic waypoints
    AGENT_BEHAVIOR_PURSUIT           = 6,   // Pass 2 — chase target
    AGENT_BEHAVIOR_FLEE              = 7,   // Pass 2 — inverse pursuit
    AGENT_BEHAVIOR_FLOCK2D           = 8,   // Pass 2 — Vicsek alignment
    AGENT_BEHAVIOR_LEVY_FLIGHT       = 9,   // Pass 2 — heavy-tailed steps
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


// ═══ REGISTRY: BEHAVIORS ═════════════════════════════════════════
//
// Per-behavior motion parameters. Units:
//   step_rate         steps per second (RandomWalk-style behaviors)
//   step_size         world units per step
//   persistence       [0,1] — correlation between consecutive step angles
//   drag              1/s — velocity decay coefficient (exponential)
//   home_pull         1/s² — spring coefficient toward home (HomeSeeker)
//   neighbor_radius   world units — flock neighbor search radius
//   speed_cap         world units/s — per-agent max speed
//
// PlayerControlled rows are all zero: the kernel switch case reads
// input directly rather than these parameters.

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
    { AGENT_BEHAVIOR_CORRELATED_WALK,   "correlated_walk",     0.8f,      1.5f,      0.6f,        3.0f, 0.0f,      0.0f,            3.0f    },
    { AGENT_BEHAVIOR_WANDERER,          "wanderer",            0.8f,      1.5f,      0.6f,        3.0f, 0.25f,     0.0f,            3.0f    },
    { AGENT_BEHAVIOR_HOME_SEEKER,       "home_seeker",         1.2f,      0.8f,      0.3f,        2.5f, 1.50f,     0.0f,            3.0f    },
    { AGENT_BEHAVIOR_PATROL,            "patrol",              1.0f,      2.0f,      0.9f,        3.0f, 0.0f,      0.0f,            3.0f    },
    { AGENT_BEHAVIOR_PURSUIT,           "pursuit",             0.0f,      0.0f,      0.0f,        3.0f, 0.0f,      30.0f,           4.0f    },
    { AGENT_BEHAVIOR_FLEE,              "flee",                0.0f,      0.0f,      0.0f,        3.0f, 0.0f,      30.0f,           4.0f    },
    { AGENT_BEHAVIOR_FLOCK2D,           "flock2d",             0.0f,      0.0f,      0.0f,        2.0f, 0.0f,      12.0f,           3.5f    },
    { AGENT_BEHAVIOR_LEVY_FLIGHT,       "levy_flight",         0.5f,      1.5f,      0.0f,        3.0f, 0.0f,      0.0f,            5.0f    },
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
    float       coupling_gain;   // future: per-tier music coupling scale
    float       home_gain;       // multiplies home_pull
    float       weight;          // default selection weight (tier_weights override)
    float       color_r;
    float       color_g;
    float       color_b;
};

static constexpr AgentTierDef AGENT_TIER_GAINS[AGENT_TIER_COUNT] = {
    //  id                     name        step  persist  speed  cpl   home  wt    color
    { AGENT_TIER_WORKER,   "worker",   1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 4.0f,  0.60f, 0.62f, 0.65f },  // slate gray
    { AGENT_TIER_SCOUT,    "scout",    1.8f, 0.4f, 1.4f, 1.0f, 0.5f, 2.0f,  0.85f, 0.65f, 0.40f },  // bronze
    { AGENT_TIER_SENTINEL, "sentinel", 0.6f, 1.2f, 0.5f, 1.0f, 2.0f, 1.0f,  0.30f, 0.40f, 0.70f },  // deep blue
    { AGENT_TIER_LEADER,   "leader",   1.2f, 0.9f, 1.1f, 2.5f, 0.8f, 0.3f,  0.95f, 0.85f, 0.55f },  // pale gold
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
// spawn_radius is the XZ distance from the player at which new
// agents appear; home_seeding_radius is how far each agent's home
// tether offset ranges from its spawn point.

struct AgentPopulationDef {
    uint32_t mood_id;
    uint32_t count;                                          // 0..Dim::MAX_AGENTS-1
    float    behavior_weights[AGENT_BEHAVIOR_COUNT];
    float    tier_weights[AGENT_TIER_COUNT];
    float    spawn_radius;                                   // world units from player
    float    home_seeding_radius;                            // world units from spawn point
};

// Mood ordering matches MOOD_TABLE in cartridge.hpp:
//   0 open_default   1 open_sunset   2 indoor_flat   3 indoor_vault
//   4 finite_outdoor 5 finite_outdoor_ref
//
// Pass 1 authors two outdoor moods with RandomWalk populations.
// Other moods carry count=0 rows so the table stays mood-indexed.
static constexpr AgentPopulationDef AGENT_POPULATIONS[MOOD_COUNT] = {
    /* 0 open_default */
    { /*mood=*/ 0, /*count=*/ 6,
      /*behavior_weights=*/ { 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
      /*tier_weights=*/     { 2.0f, 2.0f, 1.0f, 0.0f },
      /*spawn_radius=*/ 50.0f,
      /*home_seeding_radius=*/ 5.0f },
    /* 1 open_sunset */
    { /*mood=*/ 1, /*count=*/ 4,
      /*behavior_weights=*/ { 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
      /*tier_weights=*/     { 1.0f, 3.0f, 0.0f, 0.0f },
      /*spawn_radius=*/ 60.0f,
      /*home_seeding_radius=*/ 8.0f },
    /* 2 indoor_flat */
    { 2, 0, { 0 }, { 0 }, 0.0f, 0.0f },
    /* 3 indoor_vault */
    { 3, 0, { 0 }, { 0 }, 0.0f, 0.0f },
    /* 4 finite_outdoor */
    { 4, 0, { 0 }, { 0 }, 0.0f, 0.0f },
    /* 5 finite_outdoor_ref */
    { 5, 0, { 0 }, { 0 }, 0.0f, 0.0f },
};

static_assert(sizeof(AGENT_POPULATIONS) / sizeof(AGENT_POPULATIONS[0]) == MOOD_COUNT,
              "AGENT_POPULATIONS must declare one row per mood");


// ═══ CPU MIRROR ══════════════════════════════════════════════════
//
// The CPU shadow of agent state. Readback from the GPU keeps this in
// sync for Caps Lock targeting (nearest-within-radius query) and
// other host-side consumers (patch streaming reads the possessed
// agent's XZ). Wired up in Step 2; declared here so later steps have
// a home for the storage.

GPUAgentState cpuAgents_[Dim::MAX_AGENTS] = {};


// ═══ SPAWN ════════════════════════════════════════════════════════
//
// Deterministic mood-driven population. Preserves slot 0 (player),
// clears 1..MAX_AGENTS-1, then fills the first `count` non-player
// slots by rolling behavior + tier from AGENT_POPULATIONS[mood_id]
// weights. Position is uniform on a disk of radius spawn_radius
// around (center_x, center_z); home tether offset is uniform on a
// disk of radius home_seeding_radius around each spawn point.
//
// Called once at boot (for the initial mood) and once per mood
// transition, after reset_player_agent + apply_mood. Uploads the
// full 32-slot array — slot 0's re-upload is idempotent as long as
// cpuAgents_[0] mirrors the player's idle pose.
void spawn_population_for_mood(uint32_t mood_id,
                               uint32_t seed,
                               float center_x, float center_z,
                               wgpu::Queue& queue) {
    if (mood_id >= MOOD_COUNT) return;
    const auto& pop = AGENT_POPULATIONS[mood_id];

    for (uint32_t s = 1; s < Dim::MAX_AGENTS; s++) {
        cpuAgents_[s] = GPUAgentState{};
    }

    float beh_sum = 0.0f;
    for (uint32_t b = 0; b < AGENT_BEHAVIOR_COUNT; b++) beh_sum += pop.behavior_weights[b];
    float tier_sum = 0.0f;
    for (uint32_t t = 0; t < AGENT_TIER_COUNT; t++) tier_sum += pop.tier_weights[t];

    uint32_t spawned = 0;
    const uint32_t n = std::min(pop.count, Dim::MAX_AGENTS - 1u);
    if (n > 0 && beh_sum > 0.0f && tier_sum > 0.0f) {
        for (uint32_t i = 0; i < n; i++) {
            uint32_t slot = i + 1;
            uint32_t agent_seed = cpu_hash(cpu_hash(seed, 0xA6E00000u + mood_id), i + 1u);

            uint32_t behavior_id = AGENT_BEHAVIOR_RANDOM_WALK;
            {
                float roll = cpu_hash_f(agent_seed, 1u);
                float cum = 0.0f;
                for (uint32_t b = 0; b < AGENT_BEHAVIOR_COUNT; b++) {
                    cum += pop.behavior_weights[b] / beh_sum;
                    if (roll < cum) { behavior_id = b; break; }
                }
            }

            uint32_t tier_idx = AGENT_TIER_WORKER;
            {
                float roll = cpu_hash_f(agent_seed, 2u);
                float cum = 0.0f;
                for (uint32_t t = 0; t < AGENT_TIER_COUNT; t++) {
                    cum += pop.tier_weights[t] / tier_sum;
                    if (roll < cum) { tier_idx = t; break; }
                }
            }

            const float two_pi = 6.28318530718f;
            float theta = cpu_hash_f(agent_seed, 3u) * two_pi;
            float r     = std::sqrt(cpu_hash_f(agent_seed, 4u)) * pop.spawn_radius;
            float sx = center_x + std::cos(theta) * r;
            float sz = center_z + std::sin(theta) * r;

            float h_theta = cpu_hash_f(agent_seed, 5u) * two_pi;
            float h_r     = std::sqrt(cpu_hash_f(agent_seed, 6u)) * pop.home_seeding_radius;
            float hx = sx + std::cos(h_theta) * h_r;
            float hz = sz + std::sin(h_theta) * h_r;

            auto& a = cpuAgents_[slot];
            a.pos_x = sx;   a.pos_y = 0.0f;   a.pos_z = sz;
            a.home_x = hx;  a.home_y = 0.0f;  a.home_z = hz;
            a.heading = 0.0f;
            a.vel_x = 0.0f; a.vel_y = 0.0f; a.vel_z = 0.0f;
            a.orient_x = 0.0f; a.orient_y = 0.0f; a.orient_z = 0.0f; a.orient_w = 1.0f;
            a.seed = agent_seed;
            a.behavior_id = behavior_id;
            a.tier_idx = tier_idx;
            a.is_active = 1u;
            a.portal_trigger = -1;
            spawned++;
        }
    }

    gpuState_.upload_agent_state_all(queue, cpuAgents_);
    std::cout << "[Agents] Spawned " << spawned << " for mood " << mood_id
              << " around (" << center_x << "," << center_z << ")\n";
}


// ═══ RESPAWN (per-frame, evicted slots → fresh agents) ════════════
//
// The GPU's update_agents kernel marks any non-player slot whose XZ
// distance from the possessed slot exceeds AGENT_EVICTION_RADIUS as
// inactive. This routine runs every frame on the CPU side: scans
// cpuAgents_[1..MAX_AGENTS-1] for is_active == 0, refills each with
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
uint32_t agentRespawnCounters_[Dim::MAX_AGENTS] = {};

void respawn_evicted_agents(uint32_t mood_id,
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

    const uint32_t possessed = player_.possessed_slot;
    const float px = cpuAgents_[possessed].pos_x;
    const float pz = cpuAgents_[possessed].pos_z;

    const uint32_t n = std::min(pop.count, Dim::MAX_AGENTS - 1u);
    uint32_t respawned = 0;

    for (uint32_t i = 0; i < n; i++) {
        uint32_t slot = i + 1;
        if (slot == possessed) continue;
        if (cpuAgents_[slot].is_active != 0u) continue;

        agentRespawnCounters_[slot]++;
        uint32_t agent_seed = cpu_hash(
            cpu_hash(world_seed, 0xA6E00000u + mood_id),
            slot * 0x10001u + agentRespawnCounters_[slot] * 0x100u);

        uint32_t behavior_id = AGENT_BEHAVIOR_RANDOM_WALK;
        {
            float roll = cpu_hash_f(agent_seed, 1u);
            float cum = 0.0f;
            for (uint32_t b = 0; b < AGENT_BEHAVIOR_COUNT; b++) {
                cum += pop.behavior_weights[b] / beh_sum;
                if (roll < cum) { behavior_id = b; break; }
            }
        }
        uint32_t tier_idx = AGENT_TIER_WORKER;
        {
            float roll = cpu_hash_f(agent_seed, 2u);
            float cum = 0.0f;
            for (uint32_t t = 0; t < AGENT_TIER_COUNT; t++) {
                cum += pop.tier_weights[t] / tier_sum;
                if (roll < cum) { tier_idx = t; break; }
            }
        }

        const float two_pi = 6.28318530718f;
        float theta = cpu_hash_f(agent_seed, 3u) * two_pi;
        float r     = std::sqrt(cpu_hash_f(agent_seed, 4u)) * pop.spawn_radius;
        float sx = px + std::cos(theta) * r;
        float sz = pz + std::sin(theta) * r;

        float h_theta = cpu_hash_f(agent_seed, 5u) * two_pi;
        float h_r     = std::sqrt(cpu_hash_f(agent_seed, 6u)) * pop.home_seeding_radius;
        float hx = sx + std::cos(h_theta) * h_r;
        float hz = sz + std::sin(h_theta) * h_r;

        auto& a = cpuAgents_[slot];
        a.pos_x = sx;   a.pos_y = 0.0f;   a.pos_z = sz;
        a.home_x = hx;  a.home_y = 0.0f;  a.home_z = hz;
        a.heading = 0.0f;
        a.vel_x = 0.0f; a.vel_y = 0.0f; a.vel_z = 0.0f;
        a.orient_x = 0.0f; a.orient_y = 0.0f; a.orient_z = 0.0f; a.orient_w = 1.0f;
        a.seed = agent_seed;
        a.behavior_id = behavior_id;
        a.tier_idx = tier_idx;
        a.is_active = 1u;
        a.portal_trigger = -1;

        gpuState_.upload_agent_slot(queue, slot, &cpuAgents_[slot]);
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
static constexpr float POSSESSION_RADIUS    = 20.0f;
static constexpr float POSSESSION_RADIUS_SQ = POSSESSION_RADIUS * POSSESSION_RADIUS;

void try_possess_nearest(wgpu::Queue& queue) {
    if (transitionPhase_ != TransitionPhase::IDLE) {
        std::cout << "[Possess] Blocked (mid-transition)\n";
        return;
    }

    const uint32_t cur = player_.possessed_slot;
    const float px = cpuAgents_[cur].pos_x;
    const float pz = cpuAgents_[cur].pos_z;

    int best_slot = -1;
    float best_d2 = POSSESSION_RADIUS_SQ;
    for (uint32_t s = 0; s < Dim::MAX_AGENTS; s++) {
        if (s == cur) continue;
        const auto& a = cpuAgents_[s];
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
    cpuAgents_[cur].behavior_id = AGENT_BEHAVIOR_RANDOM_WALK;
    if (cpuAgents_[cur].seed == 0u) {
        cpuAgents_[cur].seed = cpu_hash(activeSeed_, cur ^ 0xC11Cu);
    }

    // New slot → player control. Reset velocity + portal trigger so the
    // player's first frame on the new body is clean.
    cpuAgents_[new_slot].behavior_id    = AGENT_BEHAVIOR_PLAYER_CONTROLLED;
    cpuAgents_[new_slot].vel_x          = 0.0f;
    cpuAgents_[new_slot].vel_z          = 0.0f;
    cpuAgents_[new_slot].portal_trigger = -1;

    gpuState_.upload_agent_slot(queue, cur, &cpuAgents_[cur]);
    gpuState_.upload_agent_slot(queue, new_slot, &cpuAgents_[new_slot]);

    player_.possessed_slot = new_slot;
    gpuState_.set_possessed_slot(new_slot);

    std::cout << "[Possess] " << cur << " -> " << new_slot
              << " (tier " << cpuAgents_[new_slot].tier_idx
              << ", dist " << std::sqrt(best_d2) << ")\n";
}
