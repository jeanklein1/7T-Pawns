// ─── entities.inl ────────────────────────────────────────────────
//
// Vocabulary for the grounded entity families that share the
// generic dispatch pipeline. Tier enums, color palettes, configs,
// property registries, active-instance tracking. Seven families:
// Arch, Column, Antenna, Palm, Cactus, Blade, Pyramid.
//
// ┌─── Family overview ─────────────────────────────────────────────┐
// │                                                                  │
// │  Family   GPU tier  Notes                                        │
// │  ──────   ────────  ─────────────────────────────────────────    │
// │  Arch     0–2       catenary + piers                             │
// │  Column   0–2       revolution mesh                              │
// │  Antenna  3–5       sibling of Column (design cell division)     │
// │  Palm     —         no piers, no solids, no heightfield          │
// │  Cactus   —         no piers, no solids, no heightfield          │
// │  Blade    —         ground-level leaf clusters                   │
// │  Pyramid  —         baked into heightfield                       │
// │                                                                  │
// │  All seven share entity_pipeline.inl as their machinery home.    │
// │  Tier matrices migrated to entity_pipeline.inl as <Family>TierRow│
// │  per entities:K1 (Option B). Tier enums stay here — they're      │
// │  indexing semantics, not data.                                   │
// │                                                                  │
// │  Not here:                                                       │
// │    Ribbon  → ribbon.inl (complete subsystem)                     │
// │    Gallery → gallery.inl (complete subsystem)                    │
// │    GoL     → gol_zones.inl (complete subsystem)                  │
// │    Sphere/Cube → floater_vocabulary.inl                          │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// ┌─── Public surface (called from outside this file) ──────────────┐
// │                                                                  │
// │  Module functions are static, take EntitiesState& explicitly.    │
// │                                                                  │
// │  Mesh-gen preparers (per family — set GPU index counts):         │
// │    prepare_palm_mesh_gen(es, c, queue)                           │
// │    prepare_cactus_mesh_gen(es, c, queue)                         │
// │    prepare_blade_mesh_gen(es, c, queue)                          │
// │                                                                  │
// │  Cross-module reads (consumed by entity_pipeline.inl, mesh-gen   │
// │  dispatchers, render passes, the spine):                         │
// │    entities_state_.arches[],     entities_state_.arch_count      │
// │    entities_state_.columns[],    entities_state_.column_count    │
// │    entities_state_.antennas[],   entities_state_.antenna_count   │
// │    entities_state_.palms[],      entities_state_.palm_count      │
// │    entities_state_.cacti[],      entities_state_.cactus_count    │
// │    entities_state_.blades[],     entities_state_.blade_count     │
// │    entities_state_.pyramids[],   entities_state_.pyramid_count   │
// │    entities_state_.cpu_pyramids  (CPU mirror — heightfield bake) │
// │    entities_state_.*_mesh_gen_pending flags                      │
// │    entities_state_.lights_dirty                                  │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// Included inside the Cartridge class body.
// Depends on: seed_utils.inl
//
// SEAM[entities:P10] this block is the canonical home of pattern P10
//   (per-family vocabulary block). Seven family applications follow.
//   Each block has the same structural template: TierEnum / Color
//   palette / Config / Prop registry / Active tracking. Don't fight
//   the cookie-cutter — it's intentional specificity per family.
// SEAM[entities:taxonomy] this block holds vocabulary for the seven
//   grounded families that share entity_pipeline.inl. Sphere/Cube
//   vocabulary is in floater_vocabulary.inl; Ribbon, Gallery, and
//   GoL are complete subsystems in their own files.
// DONE[entities:K1] resolved via Option B: tier sampling profile +
//   extras live as a single per-family TierRow struct in
//   entity_pipeline.inl. The TierParams structs and TIERS arrays that
//   used to sit here are gone (single source of truth, no converters,
//   no derived tables). The legacy enum classes (ArchTier, ColumnTier,
//   etc.) stay here — they're indexing semantics, not data.
// ─────────────────────────────────────────────────────────────────


// ═══ SHARED CONSTANTS ════════════════════════════════════════════

static constexpr float PAWN_HEIGHT_UNITS = 1.5f;     // matches WGSL PAWN_HEIGHT


// ═══ VOCABULARY: ARCH ════════════════════════════════════════════
//
// Generative catenary arches. Three tiers: doorway, standard,
// monumental. Mesh is a CPU-generated barrel vault with per-vertex
// color. Pawn-walkable via step-height; two pier solids per arch.
//
// Pier sizing rule. Pier footprint is DERIVED from shell geometry:
//     pier_half_x = thickness/2 + pier_padding + edge_blend
//     pier_half_z = depth/2     + pier_padding + edge_blend
//   The artist controls pier_padding (extra margin beyond the shell
//   base). Piers always cover the arch feet with room to spare.
//
// Color: default = warm sandstone with per-instance variance;
//        override = arch palette (one entry, room to grow).

enum class ArchTier : uint32_t {
    DOORWAY = 0,   // human-scale passage
    STANDARD = 1,   // the arch we started with
    MONUMENTAL = 2,   // cathedral-scale gateway
    COUNT = 3
};

// ── Color Palette ────────────────────────────────────────────────
static constexpr float ARCH_PALETTE[][3] = {
    { 0.82f, 0.80f, 0.78f },   // 0: light grey stone
};
static constexpr uint32_t ARCH_PALETTE_COUNT = 1;
static constexpr float ARCH_SANDSTONE_BASE[3] = { 0.75f, 0.68f, 0.60f };
static constexpr float ARCH_SANDSTONE_VARIANCE = 0.04f;

// ── Spawn Configuration ──────────────────────────────────────────
// Controls WHERE arches appear; deliberately separate from the tier
// matrix in entity_pipeline.inl (which controls WHAT each looks
// like). Spawn rules are placeholder, awaiting full object vocabulary.
struct ArchConfig {
    static constexpr float SPAWN_CHANCE = 0.030f;
    // Per-mood spawn multiplier (Bayesian: prior × mood_factor × adjacency_factor)
    static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    // Position jitter within patch (fraction of PATCH_EXTENT)
    static constexpr float POSITION_JITTER = 0.35f;
};

// ── Property Index Registry ──────────────────────────────────────
// Named constants for cpu_hash_f(seed, prop) → deterministic draw.
// 600-series: decorrelated from ribbons (400) and galleries (500).
struct ArchProp {
    static constexpr uint32_t SPAWN_ROLL = 600u;
    static constexpr uint32_t POSITION_X = 601u;
    static constexpr uint32_t POSITION_Z = 602u;
    static constexpr uint32_t ROTATION = 603u;
    static constexpr uint32_t TIER = 604u;
    static constexpr uint32_t SPAN = 610u;
    static constexpr uint32_t RISE = 611u;
    static constexpr uint32_t DEPTH = 612u;
    static constexpr uint32_t THICKNESS = 613u;
    static constexpr uint32_t PIER_HEIGHT = 614u;
    static constexpr uint32_t PIER_PADDING = 615u;
    static constexpr uint32_t EDGE_BLEND = 616u;
    static constexpr uint32_t COLOR_OVER = 620u;
    static constexpr uint32_t COLOR_VAR_R = 621u;
    static constexpr uint32_t COLOR_VAR_G = 622u;
    static constexpr uint32_t COLOR_VAR_B = 623u;
};

// ── Active Arch Tracking ─────────────────────────────────────────
struct ActiveArch {
    int32_t patch_gx = 0, patch_gz = 0;   // trigger patch (idempotency)
    int32_t host_gx = 0, host_gz = 0;     // actual patch covering entity position (eviction)
    // Pier slots derived from arch slot: PIER_ARCH_BASE + slot*2, +1
    bool active = false;
    bool draw_visible = true;    // false = mesh zeroed for distance culling

    // Geometry (for portal detection + mesh rebuild)
    float world_x = 0.0f, world_z = 0.0f;
    float rotation = 0.0f;               // facing angle (radians)
    float half_span = 0.0f;
    float total_height = 0.0f;            // pier_height + rise
    ArchTier tier = ArchTier::DOORWAY;

    // Cached mesh parameters (set at spawn, read by rebuild)
    float depth = 0.0f;
    float thickness = 0.0f;
    float rise = 0.0f;                    // catenary height above piers
    float pier_height = 0.0f;
    float burial = 0.0f;
    uint32_t segs_u = 16, segs_v = 4;
    float col_r = 0.75f, col_g = 0.68f, col_b = 0.60f;

    // Placement (computed once at spawn, immutable)
    float cached_ground_y = 0.0f;         // absolute pier-top Y for VS offset

    // Portal state
    bool is_portal = false;
    bool is_back_portal = false;
    uint32_t position_hash = 0;           // for crossing_seed
    PortalDestination destination{};      // world this portal leads to
};

// (Arch state lives in EntitiesState — see end of file)


// ═══ VOCABULARY: COLUMN ══════════════════════════════════════════
//
// Generative columns. Three classical tiers: pillar, doric, ornate.
// Mesh is a surface of revolution from a profile curve: base layers
// → shaft (with taper + entasis) → capital layers.
//
// Collision: one solid per column (square footprint with edge_blend).
// Color: default = warm sandstone with per-instance variance;
//        override = column palette (seven entries).

enum class ColumnTier : uint32_t {
    PILLAR = 0,        // thick sturdy post, minimal ornamentation
    DORIC = 1,         // classical proportions, no base, subtle taper
    ORNATE = 2,        // monumental, entasis, layered base + capital
    COUNT = 3
};
static constexpr uint32_t COLUMN_TIER_COUNT = static_cast<uint32_t>(ColumnTier::COUNT);

// ── Color Palette ────────────────────────────────────────────────
static constexpr float COLUMN_PALETTE[][3] = {
    { 0.937f, 0.902f, 0.831f },   // 0: sand      #EFE6D4
    { 0.882f, 0.827f, 0.714f },   // 1: bone      #E1D3B6
    { 0.129f, 0.118f, 0.110f },   // 2: ink       #211E1C
    { 0.353f, 0.333f, 0.298f },   // 3: ink-soft  #5A554C
    { 0.431f, 0.608f, 0.753f },   // 4: sky       #6E9BC0
    { 0.863f, 0.596f, 0.482f },   // 5: coral     #DC987B
    { 0.878f, 0.635f, 0.306f },   // 6: gold      #E0A24E
    { 0.616f, 0.631f, 0.467f },   // 7: olive     #9DA177
    { 0.635f, 0.620f, 0.686f },   // 8: lavender  #A29EAF
    { 0.753f, 0.325f, 0.184f },   // 9: orb       #C0532F
};
static constexpr uint32_t COLUMN_PALETTE_COUNT = 10;
static constexpr float COLUMN_SANDSTONE_BASE[3] = { 0.75f, 0.68f, 0.60f };
static constexpr float COLUMN_SANDSTONE_VARIANCE = 0.04f;

// ── Spawn Configuration ──────────────────────────────────────────
struct ColumnConfig {
    static constexpr float SPAWN_CHANCE = 0.030f;
    static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };
    static constexpr float POSITION_JITTER = 0.35f;
};

// ── Property Index Registry ──────────────────────────────────────
struct ColumnProp {
    static constexpr uint32_t SPAWN_ROLL = 700u;
    static constexpr uint32_t POSITION_X = 701u;
    static constexpr uint32_t POSITION_Z = 702u;
    static constexpr uint32_t TIER = 703u;
    static constexpr uint32_t HEIGHT = 710u;
    static constexpr uint32_t SHAFT_RADIUS = 711u;
    static constexpr uint32_t TAPER = 712u;
    static constexpr uint32_t ENTASIS = 713u;
    static constexpr uint32_t BASE_LAYERS = 714u;
    static constexpr uint32_t BASE_HEIGHT = 715u;
    static constexpr uint32_t BASE_OVERHANG = 716u;
    static constexpr uint32_t CAPITAL_LAYERS = 720u;
    static constexpr uint32_t CAPITAL_HEIGHT = 721u;
    static constexpr uint32_t CAPITAL_OVERHANG = 722u;
    static constexpr uint32_t SOLID_PADDING = 730u;
    static constexpr uint32_t SOLID_HEIGHT = 731u;
    static constexpr uint32_t EDGE_BLEND = 732u;
    static constexpr uint32_t COLOR_OVER = 740u;
    static constexpr uint32_t COLOR_VAR_R = 741u;
    static constexpr uint32_t COLOR_VAR_G = 742u;
    static constexpr uint32_t COLOR_VAR_B = 743u;
};


// ═══ VOCABULARY: ANTENNA ═════════════════════════════════════════
//
// Generative antennas. Three tiers: antenna, squat, colossal. Tall
// posts with stacked drum elements.
//
// Lineage. Antenna is a design cell division from Column. The two
// share ColumnTierRow shape in entity_pipeline.inl (with field reuse:
// base_layers = drum_count, base_height = drum_height,
// base_overhang = drum_radius_overhang, capital_height = spacer_height),
// share ActiveColumn tracking, share the mesh-gen pipeline. They
// occupy distinct GPU tier indices: Column 0–2, Antenna 3–5.

enum class AntennaTier : uint32_t {
    ANTENNA = 0,       // tall post with stacked drum elements
    SQUAT = 1,         // wider post + wider/shorter drums
    COLOSSAL = 2,      // massive tower-scale antenna
    COUNT = 3
};
static constexpr uint32_t ANTENNA_TIER_COUNT = static_cast<uint32_t>(AntennaTier::COUNT);

// Antenna has no separate color palette — it shares COLUMN_PALETTE.
// Drum colors are sampled into the per-instance ActiveColumn::drum_colors[]
// array (cached per spawn). See entity_pipeline.inl for the sampling.

// ── Spawn Configuration ──────────────────────────────────────────
struct AntennaConfig {
    static constexpr float SPAWN_CHANCE = 0.025f;
    static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };
    static constexpr float POSITION_JITTER = 0.35f;
};

// ── Property Index Registry ──────────────────────────────────────
struct AntennaProp {
    static constexpr uint32_t SPAWN_ROLL = 900u;
    static constexpr uint32_t POSITION_X = 901u;
    static constexpr uint32_t POSITION_Z = 902u;
    static constexpr uint32_t TIER = 903u;
    static constexpr uint32_t HEIGHT = 910u;
    static constexpr uint32_t SHAFT_RADIUS = 911u;
    static constexpr uint32_t TAPER = 912u;
    static constexpr uint32_t ENTASIS = 913u;
    static constexpr uint32_t BASE_LAYERS = 914u;
    static constexpr uint32_t BASE_HEIGHT = 915u;
    static constexpr uint32_t BASE_OVERHANG = 916u;
    static constexpr uint32_t CAPITAL_LAYERS = 920u;
    static constexpr uint32_t CAPITAL_HEIGHT = 921u;
    static constexpr uint32_t CAPITAL_OVERHANG = 922u;
    static constexpr uint32_t SOLID_PADDING = 930u;
    static constexpr uint32_t SOLID_HEIGHT = 931u;
    static constexpr uint32_t EDGE_BLEND = 932u;
    static constexpr uint32_t COLOR_OVER = 940u;
    static constexpr uint32_t COLOR_VAR_R = 941u;
    static constexpr uint32_t COLOR_VAR_G = 942u;
    static constexpr uint32_t COLOR_VAR_B = 943u;
};

// ── Active Column Tracking (shared by Column and Antenna) ────────
struct ActiveColumn {
    int32_t patch_gx = 0, patch_gz = 0;   // trigger patch (idempotency)
    int32_t host_gx = 0, host_gz = 0;     // actual patch covering entity position (eviction)
    // Pier slot derived from column slot: PIER_COLUMN_BASE + slot
    bool active = false;
    bool draw_visible = true;    // false = mesh zeroed for distance culling

    // Cached mesh parameters (set at spawn, read by rebuild)
    float world_x = 0.0f, world_z = 0.0f;
    float height = 0.0f;
    float shaft_radius = 0.0f;
    float taper = 1.0f;
    float entasis = 0.0f;
    uint32_t base_layers = 0;
    float base_height = 0.0f;
    float base_overhang = 0.0f;
    uint32_t cap_layers = 0;
    float cap_height = 0.0f;
    float cap_overhang = 0.0f;
    float solid_height = 0.0f;
    float burial = 0.0f;
    uint32_t segs_around = 12;
    uint32_t shaft_rings = 4;
    float col_r = 0.75f, col_g = 0.68f, col_b = 0.60f;
    uint32_t tier_idx = 0;
    // Antenna drum colors (cached for rebuild)
    float drum_colors[9] = {};  // 3 drums × RGB

    // Placement (computed once at spawn, immutable)
    float cached_ground_y = 0.0f;         // absolute pier-top Y for VS offset
};

// (Column + Antenna state lives in EntitiesState — see end of file)


// ═══ VOCABULARY: PALM ════════════════════════════════════════════
//
// Generative palms. Three tiers: Sapling, Coastal, Royal. Tapered
// trunk with lean + bark rings, crowned with radial fronds.
//
// Lineage. Member of the vegetation cluster (Palm, Cactus, Blade) —
// no piers, no collision solids, no heightfield contribution.
// Color paradigm: body-part bases (trunk / frond / aged frond) with
// per-instance variance.

enum class PalmTier : uint32_t { SAPLING = 0, COASTAL = 1, ROYAL = 2, COUNT = 3 };
static constexpr uint32_t PALM_TIER_COUNT = static_cast<uint32_t>(PalmTier::COUNT);

// ── Color Palette ────────────────────────────────────────────────
static constexpr float PALM_TRUNK_BASE[3] = { 0.45f, 0.35f, 0.25f };
static constexpr float PALM_FROND_BASE[3] = { 0.25f, 0.45f, 0.20f };
static constexpr float PALM_AGED_BASE[3] = { 0.35f, 0.38f, 0.18f };

// ── Spawn Configuration ──────────────────────────────────────────
struct PalmConfig {
    static constexpr float SPAWN_CHANCE = 0.200f;
    static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };
    static constexpr float POSITION_JITTER = 0.45f;
};

// ── Property Index Registry ──────────────────────────────────────
struct PalmProp {
    static constexpr uint32_t SPAWN_ROLL = 950u;
    static constexpr uint32_t POSITION_X = 951u;
    static constexpr uint32_t POSITION_Z = 952u;
    static constexpr uint32_t ROTATION = 953u;
    static constexpr uint32_t TIER = 954u;
    static constexpr uint32_t HEIGHT = 960u;
    static constexpr uint32_t BASE_R = 961u;
    static constexpr uint32_t TOP_R = 962u;
    static constexpr uint32_t LEAN = 963u;
    static constexpr uint32_t LEAN_DIR = 964u;
    static constexpr uint32_t BARK_RINGS = 965u;
    static constexpr uint32_t BARK_DEPTH = 966u;
    static constexpr uint32_t FROND_COUNT = 970u;
    static constexpr uint32_t FROND_LEN = 971u;
    static constexpr uint32_t FROND_WIDTH = 972u;
    static constexpr uint32_t FROND_DROOP = 973u;
    static constexpr uint32_t FROND_ARCH = 974u;
    static constexpr uint32_t CROWN_SPREAD = 975u;
    static constexpr uint32_t CROWN_SKIRT = 976u;
    static constexpr uint32_t SOLID_PADDING = 980u;
    static constexpr uint32_t EDGE_BLEND = 981u;
    static constexpr uint32_t COLOR_OVER = 990u;
    static constexpr uint32_t COLOR_VAR_R = 991u;
    static constexpr uint32_t COLOR_VAR_G = 992u;
    static constexpr uint32_t COLOR_VAR_B = 993u;
};

// ── Active Palm Tracking ─────────────────────────────────────────
struct ActivePalm {
    int32_t patch_gx = 0, patch_gz = 0;
    int32_t host_gx = 0, host_gz = 0;
    bool active = false;
    bool draw_visible = true;
    float world_x = 0.0f, world_z = 0.0f;
    float height = 0.0f;
    float base_r = 0.0f;
    uint32_t tier_idx = 0;
    float cached_ground_y = 0.0f;
};

// (Palm state lives in EntitiesState — see end of file)
// (prepare_palm_mesh_gen moved to end of file — must follow EntitiesState definition)


// ═══ VOCABULARY: CACTUS ══════════════════════════════════════════
//
// Generative cacti. Three tiers: Finger, Saguaro, Candelabra. Ribbed
// columnar trunk with optional forking arms.
//
// Lineage. Member of the vegetation cluster (Palm, Cactus, Blade) —
// no piers, no collision solids, no heightfield contribution. Color
// paradigm: body / rib bases with per-instance variance.

enum class CactusTier : uint32_t { FINGER = 0, SAGUARO = 1, CANDELABRA = 2, COUNT = 3 };
static constexpr uint32_t CACTUS_TIER_COUNT = static_cast<uint32_t>(CactusTier::COUNT);

// ── Color Palette ────────────────────────────────────────────────
static constexpr float CACTUS_BODY_BASE[3] = { 0.30f, 0.45f, 0.25f };
static constexpr float CACTUS_RIB_BASE[3] = { 0.35f, 0.55f, 0.30f };

// ── Spawn Configuration ──────────────────────────────────────────
struct CactusConfig {
    static constexpr float SPAWN_CHANCE = 0.100f;
    static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };
    static constexpr float POSITION_JITTER = 0.35f;
};

// ── Property Index Registry ──────────────────────────────────────
struct CactusProp {
    static constexpr uint32_t SPAWN_ROLL = 1000u;
    static constexpr uint32_t POSITION_X = 1001u;
    static constexpr uint32_t POSITION_Z = 1002u;
    static constexpr uint32_t ROTATION = 1003u;
    static constexpr uint32_t TIER = 1004u;
    static constexpr uint32_t HEIGHT = 1010u;
    static constexpr uint32_t RADIUS = 1011u;
    static constexpr uint32_t TAPER = 1012u;
    static constexpr uint32_t RIBS = 1013u;
    static constexpr uint32_t RIB_DEPTH = 1014u;
    static constexpr uint32_t LEAN = 1015u;
    static constexpr uint32_t LEAN_DIR = 1016u;
    static constexpr uint32_t CAP_ROUND = 1017u;
    static constexpr uint32_t ARM_COUNT = 1020u;
    static constexpr uint32_t ARM_HEIGHT = 1021u;
    static constexpr uint32_t ARM_LENGTH = 1022u;
    static constexpr uint32_t ARM_RADIUS = 1023u;
    static constexpr uint32_t ARM_CURVE = 1024u;
    static constexpr uint32_t COLOR_OVER = 1030u;
    static constexpr uint32_t COLOR_VAR_R = 1031u;
    static constexpr uint32_t COLOR_VAR_G = 1032u;
    static constexpr uint32_t COLOR_VAR_B = 1033u;
};

// ── Active Cactus Tracking ───────────────────────────────────────
struct ActiveCactus {
    int32_t patch_gx = 0, patch_gz = 0;
    int32_t host_gx = 0, host_gz = 0;
    bool active = false;
    bool draw_visible = true;
    float world_x = 0.0f, world_z = 0.0f;
    float height = 0.0f;
    float radius = 0.0f;
    uint32_t tier_idx = 0;
    float cached_ground_y = 0.0f;
};

// (Cactus state lives in EntitiesState — see end of file)
// (prepare_cactus_mesh_gen moved to end of file — must follow EntitiesState definition)


// ═══ VOCABULARY: BLADE ═══════════════════════════════════════════
//
// Generative blade clusters. Three tiers: Sprout, Clump, Thicket.
// Ground-level leaf clusters of 3–7 thick pointed blades from a
// single ground point. Geometry: flat quad strips along curved
// midribs, golden-angle packed.
//
// Lineage. Member of the vegetation cluster (Palm, Cactus, Blade) —
// no piers, no collision solids, no heightfield contribution. Color
// paradigm: body / aged bases with per-instance variance.

enum class BladeClusterTier : uint32_t { SPROUT = 0, CLUMP = 1, THICKET = 2, COUNT = 3 };
static constexpr uint32_t BLADE_TIER_COUNT = static_cast<uint32_t>(BladeClusterTier::COUNT);

// ── Color Palette ────────────────────────────────────────────────
static constexpr float BLADE_BODY_BASE[3] = { 0.28f, 0.52f, 0.22f };
static constexpr float BLADE_AGED_BASE[3] = { 0.48f, 0.45f, 0.28f };

// ── Spawn Configuration ──────────────────────────────────────────
struct BladeClusterConfig {
    static constexpr float SPAWN_CHANCE = 0.025f;
    static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };
    static constexpr float POSITION_JITTER = 0.30f;
};

// ── Property Index Registry ──────────────────────────────────────
struct BladeProp {
    static constexpr uint32_t SPAWN_ROLL = 1100u;
    static constexpr uint32_t POSITION_X = 1101u;
    static constexpr uint32_t POSITION_Z = 1102u;
    static constexpr uint32_t ROTATION = 1103u;
    static constexpr uint32_t TIER = 1104u;
    static constexpr uint32_t BLADE_COUNT = 1110u;
    static constexpr uint32_t HEIGHT = 1111u;
    static constexpr uint32_t HEIGHT_VAR = 1112u;
    static constexpr uint32_t WIDTH = 1113u;
    static constexpr uint32_t SPLAY = 1114u;
    static constexpr uint32_t CURVE = 1115u;
    static constexpr uint32_t TWIST = 1116u;
    static constexpr uint32_t TAPER = 1117u;
    static constexpr uint32_t COLOR_VAR_R = 1120u;
    static constexpr uint32_t COLOR_VAR_G = 1121u;
    static constexpr uint32_t COLOR_VAR_B = 1122u;
};

// ── Active Blade Tracking ────────────────────────────────────────
struct ActiveBlade {
    int32_t patch_gx = 0, patch_gz = 0;
    int32_t host_gx = 0, host_gz = 0;
    bool active = false;
    bool draw_visible = true;
    float world_x = 0.0f, world_z = 0.0f;
    float height = 0.0f;
    float radius = 0.0f;
    uint32_t tier_idx = 0;
    float cached_ground_y = 0.0f;
};

// (Blade state lives in EntitiesState — see end of file)
// (prepare_blade_mesh_gen moved to end of file — must follow EntitiesState definition)


// ═══ VOCABULARY: PYRAMID ═════════════════════════════════════════
//
// Generative pyramids. Three tiers: obelisk, temple, colossus.
//
// Collision: pyramid height function baked into heightfield —
//   pawn blocked by step-height on steep faces (no solid needed).
// Visual: CPU-generated 4-face (pointed) or 5-face (truncated) mesh.
// Color paradigm: sandstone base with per-instance variance only
//   (no palette, distinct from arch/column).

enum class PyramidTier : uint32_t {
    OBELISK = 0,     // tall narrow marker, pointed apex
    TEMPLE = 1,      // medium, truncated platform top
    COLOSSUS = 2,    // massive landmark, slight or no truncation
    COUNT = 3
};

// ── Color Palette ────────────────────────────────────────────────
// Sandstone base only. All pyramids derive color from a single base.
static constexpr float PYRAMID_SANDSTONE_BASE[3] = { 0.80f, 0.72f, 0.58f };
static constexpr float PYRAMID_SANDSTONE_VARIANCE = 0.05f;

// ── Spawn Configuration ──────────────────────────────────────────
struct PyramidConfig {
    static constexpr float SPAWN_CHANCE = 0.030f;
    static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };
    static constexpr float POSITION_JITTER = 0.25f;
};

// ── Property Index Registry ──────────────────────────────────────
struct PyramidProp {
    static constexpr uint32_t SPAWN_ROLL = 800u;
    static constexpr uint32_t POSITION_X = 801u;
    static constexpr uint32_t POSITION_Z = 802u;
    static constexpr uint32_t ROTATION = 803u;
    static constexpr uint32_t TIER = 804u;
    static constexpr uint32_t HEIGHT = 810u;
    static constexpr uint32_t BASE_HALF = 811u;
    static constexpr uint32_t ASPECT = 812u;
    static constexpr uint32_t TRUNCATION = 813u;
    static constexpr uint32_t EDGE_BLEND = 814u;
    static constexpr uint32_t COLOR_OVER = 820u;
    static constexpr uint32_t COLOR_VAR_R = 821u;
    static constexpr uint32_t COLOR_VAR_G = 822u;
    static constexpr uint32_t COLOR_VAR_B = 823u;
};

// ── Active Pyramid Tracking ──────────────────────────────────────
struct ActivePyramid {
    int32_t patch_gx = 0, patch_gz = 0;   // trigger patch (idempotency)
    int32_t host_gx = 0, host_gz = 0;     // actual patch covering entity position (eviction)
    bool active = false;
    // Cached color (set at spawn, read by rebuild)
    float col_r = 0.80f, col_g = 0.72f, col_b = 0.58f;

    // Placement (computed once at spawn, immutable)
    float cached_ground_y = 0.0f;         // absolute base Y for VS offset
};

// ═══ ENTITIES MODULE STATE (Scope B migration #9) ═════════════════
//
// All grounded-entity state lives in this struct, accessed via
// entities_state_ on the Cartridge. Module functions (the per-family
// prepare_*_mesh_gen helpers) take `EntitiesState& es` explicitly.
//
// Cross-module reads (this state is consumed widely):
//   • entity_pipeline.inl — adapter functions for each family read
//     and write the per-family arrays + counts
//   • render_passes.inl   — consumes counts to drive draw calls
//   • cartridge.hpp       — teardown loops, dispatch wrappers,
//                           per-frame mesh-gen orchestration
//   • mood.inl            — clears arrays on mood transition

struct EntitiesState {
    // ── Arch ─────────────────────────────────────────────────────
    ActiveArch arches[Dim::MAX_ARCH_INSTANCES]{};
    uint32_t   arch_count = 0;
    bool       arch_mesh_gen_pending = false;
    bool       lights_dirty = true;          // set true at init, cleared after first upload

    // ── Column + Antenna (sibling families, shared mesh-gen flag) ─
    ActiveColumn columns[Dim::MAX_COLUMN_ONLY]{};
    ActiveColumn antennas[Dim::MAX_ANTENNA_ONLY]{};
    uint32_t     column_count = 0;
    uint32_t     antenna_count = 0;
    bool         column_mesh_gen_pending = false;  // shared by column + antenna

    // ── Palm ─────────────────────────────────────────────────────
    ActivePalm palms[Dim::MAX_PALM_INSTANCES]{};
    uint32_t   palm_count = 0;
    bool       palm_mesh_gen_pending = false;

    // ── Cactus ───────────────────────────────────────────────────
    ActiveCactus cacti[Dim::MAX_CACTUS_INSTANCES]{};
    uint32_t     cactus_count = 0;
    bool         cactus_mesh_gen_pending = false;

    // ── Blade ────────────────────────────────────────────────────
    ActiveBlade blades[Dim::MAX_BLADE_INSTANCES]{};
    uint32_t    blade_count = 0;
    bool        blade_mesh_gen_pending = false;

    // ── Pyramid ──────────────────────────────────────────────────
    ActivePyramid   pyramids[Dim::MAX_PYRAMID_INSTANCES]{};
    uint32_t        pyramid_count = 0;
    bool            pyramid_mesh_gen_pending = false;
    GPUPyramidArray cpu_pyramids{};                 // CPU mirror for heightfield baking
};
EntitiesState entities_state_;


// ═══ MESH-GEN PREPARERS ═══════════════════════════════════════════
//
// Per-family CPU-side mesh-gen prep. Each preparer:
//   • Reads the family's *_mesh_gen_pending flag and clears it
//   • Scans the family's active array to find the highest active slot
//   • Uploads index count = (max_slot + 1) * indices_per_slot to GPU
//
// Functions are placed here (not in their family vocab blocks above)
// because they reference EntitiesState, which must be defined before
// use — and EntitiesState in turn must be defined after every Active*
// type it contains. The result: state struct + functions consuming
// it both live at the file's end.
//
// Counterparts for arch / column / antenna / pyramid currently live
// in spawn_engine.inl (non-static Cartridge members); they will be
// hoisted here in migration #10 along with the remaining helpers.

static bool prepare_palm_mesh_gen(EntitiesState& es, Cartridge* c, wgpu::Queue& queue) {
    if (!es.palm_mesh_gen_pending) return false;
    es.palm_mesh_gen_pending = false;
    uint32_t maxSlot = 0;
    bool anyActive = false;
    for (uint32_t i = 0; i < Dim::MAX_PALM_INSTANCES; i++) {
        if (es.palms[i].active) { maxSlot = i; anyActive = true; }
    }
    c->gpuState_.set_palm_index_count(anyActive
        ? (maxSlot + 1) * Dim::PALMG_MAX_INDICES_PER_SLOT : 0);
    return true;
}

static bool prepare_cactus_mesh_gen(EntitiesState& es, Cartridge* c, wgpu::Queue& queue) {
    if (!es.cactus_mesh_gen_pending) return false;
    es.cactus_mesh_gen_pending = false;
    uint32_t maxSlot = 0;
    bool anyActive = false;
    for (uint32_t i = 0; i < Dim::MAX_CACTUS_INSTANCES; i++) {
        if (es.cacti[i].active) { maxSlot = i; anyActive = true; }
    }
    c->gpuState_.set_cactus_index_count(anyActive
        ? (maxSlot + 1) * Dim::CACTUSG_MAX_INDICES_PER_SLOT : 0);
    return true;
}

static bool prepare_blade_mesh_gen(EntitiesState& es, Cartridge* c, wgpu::Queue& queue) {
    if (!es.blade_mesh_gen_pending) return false;
    es.blade_mesh_gen_pending = false;
    uint32_t maxSlot = 0;
    bool anyActive = false;
    for (uint32_t i = 0; i < Dim::MAX_BLADE_INSTANCES; i++) {
        if (es.blades[i].active) { maxSlot = i; anyActive = true; }
    }
    c->gpuState_.set_blade_index_count(anyActive
        ? (maxSlot + 1) * Dim::BLADEG_MAX_INDICES_PER_SLOT : 0);
    return true;
}

static bool prepare_column_mesh_gen(EntitiesState& es, Cartridge* c, wgpu::Queue& queue) {
    if (!es.column_mesh_gen_pending) return false;
    es.column_mesh_gen_pending = false;

    uint32_t maxSlot = 0;
    bool anyActive = false;
    for (uint32_t i = 0; i < Dim::MAX_COLUMN_ONLY; i++) {
        if (es.columns[i].active) { maxSlot = i; anyActive = true; }
    }
    for (uint32_t i = 0; i < Dim::MAX_ANTENNA_ONLY; i++) {
        if (es.antennas[i].active) {
            maxSlot = i + Dim::ANTENNA_SLOT_OFFSET;
            anyActive = true;
        }
    }
    c->gpuState_.set_column_index_count(anyActive
        ? (maxSlot + 1) * Dim::CMG_MAX_INDICES_PER_SLOT : 0);
    return true;
}

static bool prepare_arch_mesh_gen(EntitiesState& es, Cartridge* c, wgpu::Queue& queue) {
    if (!es.arch_mesh_gen_pending) return false;
    es.arch_mesh_gen_pending = false;

    uint32_t maxSlot = 0;
    bool anyActive = false;
    for (uint32_t i = 0; i < Dim::MAX_ARCH_INSTANCES; i++) {
        if (es.arches[i].active) { maxSlot = i; anyActive = true; }
    }
    c->gpuState_.set_arch_index_count(anyActive
        ? (maxSlot + 1) * Dim::AMG_MAX_INDICES_PER_SLOT : 0);
    return true;
}

static bool prepare_pyramid_mesh_gen(EntitiesState& es, Cartridge* c, wgpu::Queue& queue) {
    if (!es.pyramid_mesh_gen_pending) return false;
    es.pyramid_mesh_gen_pending = false;

    uint32_t maxSlot = 0;
    bool anyActive = false;
    for (uint32_t i = 0; i < Dim::MAX_PYRAMID_INSTANCES; i++) {
        if (es.pyramids[i].active) { maxSlot = i; anyActive = true; }
    }
    c->gpuState_.set_pyramid_index_count(anyActive
        ? (maxSlot + 1) * Dim::PMG_MAX_INDICES_PER_SLOT : 0);
    return true;
}
