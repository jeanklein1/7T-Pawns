// ─── floater_vocabulary.inl ──────────────────────────────────────
//
// Vocabulary for the two generic-pipeline floater families: Sphere
// (orbital, PGA motor-driven) and Cube (hover-bob monoliths). Tier
// counts, base tier weights, spawn config, property-index registry,
// runtime tracking structs.
//
// ┌─── Family overview ─────────────────────────────────────────────┐
// │                                                                  │
// │  Family   GPU compute    Vertex shader    Behavior layer         │
// │  ──────   ───────────    ─────────────    ────────────────       │
// │  Sphere   update_sphere  sphere_vs        none (analytical PGA)  │
// │  Cube     update_cube    monolith_vs      cube_behaviors.inl     │
// │                                                                  │
// │  Three concerns, three files (per family):                       │
// │    Vocabulary           → here                                   │
// │    Sampling profile     → entity_pipeline.inl (TierRow + traits) │
// │    Behavior layer       → cube_behaviors.inl (cubes only)        │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// ┌─── Public surface (consumed by other files) ────────────────────┐
// │                                                                  │
// │  Sphere:                                                         │
// │    SPHERE_TIER_COUNT, SPHERE_BASE_TIER_WEIGHTS                   │
// │    SPHERE_TIER_NAMES                                             │
// │    SphereConfig (SPAWN_CHANCE, MOOD_MULTIPLIER, POSITION_JITTER) │
// │    FloatingEntityProp (property indices 100–126)                 │
// │    ActiveFloater                                                 │
// │    activeFloaters_[],  activeFloaterCount_                       │
// │                                                                  │
// │  Cube:                                                           │
// │    CUBE_TIER_COUNT, CUBE_BASE_TIER_WEIGHTS                       │
// │    CUBE_TIER_NAMES                                               │
// │    CubeConfig (SPAWN_CHANCE, MOOD_MULTIPLIER, POSITION_JITTER)   │
// │    CubeEntityProp (property indices 130–156)                     │
// │    ActiveCube                                                    │
// │    activeCubes_[],  activeCubeCount_                             │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// Included inside the Cartridge class body.
// Depends on: state.hpp (Dim::*), entities.inl (MOOD_COUNT).
//
// SEAM[floater_vocabulary:taxonomy] generic-pipeline floater families
//   parallel grounded families (entities.inl) but live here because
//   their tier shapes differ (sphere has orbit_radius/orbit_speed,
//   cube doesn't). Three concerns, three files: vocabulary here,
//   sampling profile in entity_pipeline.inl, cube behavior in
//   cube_behaviors.inl. Spheres have no behavior layer.
// SEAM[sphere:taxonomy] sphere VOCABULARY lives here, not in
//   entities.inl. Generic-pipeline floater family — vocabulary class
//   distinct from grounded families.
// SEAM[cube:taxonomy] cube VOCABULARY lives here, not in entities.inl.
//   Three concerns, three files (vocabulary / sampling profile /
//   behavior gains), each correct.
// SEAM[cube:cx-cz-mirror] ActiveCube has cx, cz fields — CPU mirror
//   of GPU anchor for cube_behaviors.inl::corral_cubes /
//   toggle_cube_kite_mode to read without GPU readback. Same family
//   as agents:D2 (slot-0 reads); when pawn.inl extracts and provides
//   accessors, corral/kite could analogously have cube_anchor(slot)
//   accessors.
// ─────────────────────────────────────────────────────────────────


// ═══ FAMILY: SPHERE ═══════════════════════════════════════════════
//
// Orbital spheres. Rare, PGA motor-driven orbits around anchors.
// Slots 0 .. MAX_SPHERE_INSTANCES-1 in the shared floating entity buffer.
// No behavior layer — the GPU compute kernel (update_sphere) drives
// spheres entirely from analytical PGA orbits.

// ── Tier registry ────────────────────────────────────────────────
static constexpr uint32_t SPHERE_TIER_COUNT = 2;

static constexpr float SPHERE_BASE_TIER_WEIGHTS[SPHERE_TIER_COUNT] = { 0.65f, 0.35f };
static constexpr const char* SPHERE_TIER_NAMES[] = { "Sentinel", "Anomaly" };

// ── Spawn Configuration ──────────────────────────────────────────
struct SphereConfig {
    static constexpr float SPAWN_CHANCE = 0.015f;
    static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
    static constexpr float POSITION_JITTER = 0.4f;
};

// ── Property Index Registry ──────────────────────────────────────
// Range: 100–126 (legacy "FloatingEntity" name preserved for seed
// stability — renaming the struct would change hash inputs and shift
// every sphere's parameters across already-rendered worlds).
struct FloatingEntityProp {
    static constexpr uint32_t SPAWN_ROLL = 100u;
    static constexpr uint32_t ANCHOR_X = 101u;
    static constexpr uint32_t ANCHOR_Z = 102u;
    static constexpr uint32_t TIER = 103u;
    static constexpr uint32_t BODY_RADIUS = 110u;
    static constexpr uint32_t ORBIT_RADIUS = 111u;
    static constexpr uint32_t ORBIT_HEIGHT = 112u;
    static constexpr uint32_t ORBIT_SPEED = 113u;
    static constexpr uint32_t INFLUENCE_RADIUS = 114u;
    static constexpr uint32_t SPIN_SPEED = 115u;
    static constexpr uint32_t BOB_AMPLITUDE = 116u;
    static constexpr uint32_t BOB_PERIOD = 117u;
    static constexpr uint32_t SPIN_TILT_X = 118u;
    static constexpr uint32_t SPIN_TILT_Z = 119u;
    static constexpr uint32_t COLOR_R = 120u;
    static constexpr uint32_t COLOR_G = 121u;
    static constexpr uint32_t COLOR_B = 122u;
    static constexpr uint32_t ASPECT_Y = 123u;
    static constexpr uint32_t ASPECT_Z = 124u;
    static constexpr uint32_t FACE_VARIANCE = 125u;
    static constexpr uint32_t ROTATION = 126u;
};

// ── Active Sphere Tracking ───────────────────────────────────────
//
// SEAM[sphere:P5] last_alloc_time is pattern P5 (release-pending
//   sentinel / race protection) — CPU-timestamp variant. When GPU
//   readback arrives stale ("kernel evicted this slot"), the
//   timestamp protects freshly-allocated slots from being incorrectly
//   marked inactive. Same intent as cube_behaviors.inl::
//   toggle_cube_kite_mode's GPU sentinel; different mechanism.
struct ActiveFloater {
    int32_t patch_gx = 0, patch_gz = 0;
    int32_t host_gx = 0, host_gz = 0;
    // See ActiveCube::last_alloc_time — same race protection for
    // sphere slots. Spheres rarely evict in practice (orbital,
    // anchored at origin), but the readback path covers them
    // uniformly so the protection covers them uniformly too.
    float   last_alloc_time = -1000.0f;
    bool active = false;
};
ActiveFloater activeFloaters_[Dim::MAX_SPHERE_INSTANCES]{};
uint32_t activeFloaterCount_ = 0;


// ═══ FAMILY: CUBE ═════════════════════════════════════════════════
//
// Hover-bob monoliths. Colorful cubes/slabs floating above terrain.
// Slots 0 .. MAX_CUBE_INSTANCES-1 (buffer offset by CUBE_SLOT_OFFSET).
// Behavior layer (forces, coordination, kite mode, corral) lives in
// cube_behaviors.inl.
//
// Lineage. Cube and Sphere are siblings by file (both generic-
// pipeline floaters) but distinct in shape: sphere has orbit
// radius/speed/height, cube doesn't. Sphere has no behavior layer
// (analytical PGA orbits); Cube has a full behavior system. The
// shared infrastructure lives in entity_pipeline.inl as their
// per-family TierRow and adapters.

// ── Tier registry ────────────────────────────────────────────────
static constexpr uint32_t CUBE_TIER_COUNT = 4;

static constexpr float CUBE_BASE_TIER_WEIGHTS[CUBE_TIER_COUNT] = { 0.40f, 0.32f, 0.20f, 0.08f };
static constexpr const char* CUBE_TIER_NAMES[] = { "SmallCube", "MedCube", "LargeCube", "Monolith" };

// ── Spawn Configuration ──────────────────────────────────────────
struct CubeConfig {
    static constexpr float SPAWN_CHANCE = 0.60f;
    static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
    static constexpr float POSITION_JITTER = 0.4f;
};

// ── Property Index Registry ──────────────────────────────────────
// Range: 130–156 (avoids sphere's 100–126).
struct CubeEntityProp {
    static constexpr uint32_t SPAWN_ROLL = 130u;
    static constexpr uint32_t ANCHOR_X = 131u;
    static constexpr uint32_t ANCHOR_Z = 132u;
    static constexpr uint32_t TIER = 133u;
    static constexpr uint32_t BODY_RADIUS = 140u;
    static constexpr uint32_t ORBIT_HEIGHT = 142u;
    static constexpr uint32_t INFLUENCE_RADIUS = 144u;
    static constexpr uint32_t SPIN_SPEED = 145u;
    static constexpr uint32_t BOB_AMPLITUDE = 146u;
    static constexpr uint32_t BOB_PERIOD = 147u;
    static constexpr uint32_t SPIN_TILT_X = 148u;
    static constexpr uint32_t SPIN_TILT_Z = 149u;
    static constexpr uint32_t COLOR_R = 150u;
    static constexpr uint32_t COLOR_G = 151u;
    static constexpr uint32_t COLOR_B = 152u;
    static constexpr uint32_t ASPECT_Y = 153u;
    static constexpr uint32_t ASPECT_Z = 154u;
    static constexpr uint32_t FACE_VARIANCE = 155u;
    static constexpr uint32_t ROTATION = 156u;
};

// ── Active Cube Tracking ─────────────────────────────────────────
struct ActiveCube {
    int32_t patch_gx = 0, patch_gz = 0;
    int32_t host_gx = 0, host_gz = 0;
    // World XZ of the cube's anchor — mirror of fe.anchor[0,2] on GPU.
    // Captured at spawn so cube_behaviors.inl::corral_cubes can read
    // the current anchor without a GPU readback. Updated when corral
    // writes a new anchor.
    float   cx = 0.0f, cz = 0.0f;
    // Time (time_state_.seconds) when this slot was last marked active.
    // Used to suppress race between freshly allocated slots and the
    // floater readback path: readback callbacks process previous-frame
    // data, so a slot allocated this frame would be incorrectly marked
    // inactive by the readback (which sees the *prior tenant* as
    // evicted). Suppression window covers two readback cycles. See
    // render() floater sync block for the consumer.
    float   last_alloc_time = -1000.0f;
    bool active = false;
};
ActiveCube activeCubes_[Dim::MAX_CUBE_INSTANCES]{};
uint32_t activeCubeCount_ = 0;
