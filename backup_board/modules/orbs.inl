// ─── orbs.inl ────────────────────────────────────────────────────
//
// Sky orb layer — luminous points on a dome above the world. A fixed
// population of billboarded quads sampled from a dome of radius
// ORB_DOME_RADIUS, driven by CPU-authored mood config + per-frame
// musical couplings, updated by a compute kernel, rendered additively.
//
// The control surfaces (tuning console, palette registry, tier sets,
// flocking gestures, ORB_MOOD_TABLE in cartridge.hpp) are intended
// to be read as matrices: one row per item, aligned columns, minimum
// narrative. All runtime levers (cycle keys, toggles) are exposed
// as named functions with a single-line purpose comment.
//
// ┌─── Public surface (called from outside this file) ─────────────┐
// │                                                                 │
// │  Module functions are static, take OrbsState& explicitly        │
// │  (or const OrbsState& when read-only).                          │
// │                                                                 │
// │  Lifecycle:                                                     │
// │    configure_orbs(os, c, cfg, queue) — mood entry: upload + arm │
// │    teardown_orbs(os, c)              — mood exit: disable       │
// │                                                                 │
// │  Player commands:                                               │
// │    cycle_orb_palette(os, c, queue)     — KP_0  next palette     │
// │    cycle_orb_motion_rule(os, c, queue) — KP_8  next rule        │
// │    cycle_orb_gesture(os, c, queue)     — KP_DECIMAL next gest.  │
// │    toggle_orb_anchor(os, c)            — KP_9  world↔pawn       │
// │                                                                 │
// │  Per-frame updates:                                             │
// │    update_orb_anchor(os, c, x, z, q)   — dirty-flagged push     │
// │    update_orb_coupling(os, c, poly, dt, q) — polyphony → couples│
// │                                                                 │
// │  GPU dispatches (called from render tick):                      │
// │    dispatch_orb_init(os, c, encoder)        — one-shot seed     │
// │    dispatch_orb_recolor(os, c, encoder)     — palette resample  │
// │    dispatch_orb_copy_prev(os, c, encoder)   — snapshot prev     │
// │    dispatch_orb_dynamics(os, c, enc, q)     — rule + couplings  │
// │    render_orbs(os, c, pass)                 — additive draw     │
// │                                                                 │
// │  Cross-module reads: orbs_state_ is fully encapsulated; no      │
// │  external module reaches into its fields.                       │
// │                                                                 │
// └─────────────────────────────────────────────────────────────────┘
//
// Included inside the Cartridge class body.
// Depends on: state.hpp (GPUOrbConfig/State, MAX_ORBS), renderer.hpp.
// ─────────────────────────────────────────────────────────────────


// ═══ TUNING CONSOLE ══════════════════════════════════════════════
//
// All orb-system tuning dials in one place. Anything that shapes
// the feel of the sky — dome size, sprite size, coupling rates,
// sanitization defaults — is here. Mood-authored values live in
// ORB_MOOD_TABLE (cartridge.hpp); these are system-level dials
// that apply across every mood.

// ── Dome geometry ────────────────────────────────────────────────
static constexpr float ORB_DOME_RADIUS = 450.0f;
static constexpr float ORB_BASE_SIZE = 3.0f;

// ── Musical couplings ────────────────────────────────────────────
// Attack/release rates are in 1/s; exponential ramps. Attack faster
// than release = onsets feel punchy, the sky "holds" after phrases
// end. Each coupling's extras (scales, floors, ceilings) sit with
// its rates rather than in a separate orphan block.

// Force — radial expansion; also drives the noise ceiling lerp.
static constexpr float ORB_FORCE_ATTACK   = 3.0f;
static constexpr float ORB_FORCE_RELEASE  = 1.5f;
static constexpr float ORB_FORCE_SCALE    = 40.0f;   // world-units/s² at full intensity
static constexpr float ORB_NOISE_FLOOR    = 0.3f;    // barely perceptible drift in silence

// Color — pulse / converge / surge trajectories, gated per-mood.
static constexpr float ORB_COLOR_ATTACK   = 5.0f;
static constexpr float ORB_COLOR_RELEASE  = 2.5f;

// Flock — neighbor-force tightening under pressure. Release matches
// attack so silence returns the flock smoothly (~0.4s time constant).
static constexpr float ORB_FLOCK_ATTACK   = 2.5f;
static constexpr float ORB_FLOCK_RELEASE  = 2.5f;

// Speed — population-wide velocity multiplier. At rest (silence)
// the target is 1.0 (identity); at full polyphony it's ORB_SPEED_CEILING.
// Every rule's dominant speed parameter scales with the multiplier.
static constexpr float ORB_SPEED_ATTACK   = 2.0f;
static constexpr float ORB_SPEED_RELEASE  = 1.0f;    // slower return feels like settling
static constexpr float ORB_SPEED_CEILING  = 3.0f;    // 3× baseline at full music

// ── Rule-critical parameter floors ───────────────────────────────
// Applied in configure_orbs when the mood authors 0.0 for a given
// field: zero reads as "no opinion, use system default" so every
// rule has working parameters regardless of mood authorship. A mood
// wanting "almost zero" should author a tiny non-zero (e.g. 0.001f),
// which reads as intentional.
static constexpr float ORB_DEFAULT_DRAG = 0.5f;
static constexpr float ORB_DEFAULT_NOISE_AMP = 8.0f;
static constexpr float ORB_DEFAULT_ORBITAL_SPEED = 0.15f;
static constexpr float ORB_DEFAULT_FLOCK_SEP_R = 50.0f;
static constexpr float ORB_DEFAULT_FLOCK_ALIGN_R = 120.0f;
static constexpr float ORB_DEFAULT_FLOCK_COH_R = 200.0f;
static constexpr float ORB_DEFAULT_FLOCK_SEP_W = 30.0f;
static constexpr float ORB_DEFAULT_FLOCK_ALIGN_W = 8.0f;
static constexpr float ORB_DEFAULT_FLOCK_COH_W = 15.0f;
static constexpr float ORB_DEFAULT_FLOCK_MAX_SPEED = 60.0f;


// ═══ REGISTRY: PALETTES ══════════════════════════════════════════
//
// Each palette is up to 4 weighted HSV "pockets" — a mood's sky can
// be mostly one color with rare accents. Selection weights across
// pockets should sum ≈ 1.0 (the last pocket catches the tail).
// Indexed by palette_id in mood config.

static constexpr uint32_t MAX_ORB_PALETTE_ENTRIES = 4;

struct OrbPaletteEntry {
    float hue;         // HSV hue center (0..1)
    float hue_var;     // spread around center
    float saturation;  // base saturation
    float weight;      // selection probability
};

struct OrbPalette {
    uint32_t count;
    float    value_variance;   // per-orb HSV value (brightness) spread
    OrbPaletteEntry entries[MAX_ORB_PALETTE_ENTRIES];
};

// Stellar classification — diagnostic baseline with bold equally-
// weighted pockets (O/B hot blue, F/G yellow-white, K orange, M red).
static constexpr OrbPalette ORB_PALETTE_JWST_DEEP = {
    4, 0.15f,
    {
        //  hue    hue_var  sat    weight
        { 0.60f, 0.03f, 0.90f, 0.25f },   // hot blue
        { 0.12f, 0.04f, 0.60f, 0.25f },   // yellow-white
        { 0.07f, 0.04f, 0.85f, 0.25f },   // orange
        { 0.01f, 0.03f, 0.95f, 0.25f },   // deep red
    }
};

// Hubble SHO (Pillars of Creation): teal and gold with copper.
static constexpr OrbPalette ORB_PALETTE_PILLARS = {
    4, 0.30f,
    {
        //  hue    hue_var  sat    weight
        { 0.10f, 0.03f, 0.65f, 0.40f },   // gold
        { 0.48f, 0.04f, 0.55f, 0.35f },   // teal
        { 0.02f, 0.02f, 0.70f, 0.15f },   // copper accent
        { 0.55f, 0.02f, 0.35f, 0.10f },   // pale blue
    }
};

// Carina Nebula: rich blues and amber-orange, no greens.
static constexpr OrbPalette ORB_PALETTE_CARINA = {
    3, 0.30f,
    {
        //  hue    hue_var  sat    weight
        { 0.60f, 0.05f, 0.55f, 0.45f },   // rich blue
        { 0.08f, 0.04f, 0.60f, 0.40f },   // amber-orange
        { 0.55f, 0.03f, 0.30f, 0.15f },   // desaturated blue-white
        { 0.00f, 0.00f, 0.00f, 0.00f },   // (unused)
    }
};

// Single-pocket warm (legacy-equivalent, for testing).
static constexpr OrbPalette ORB_PALETTE_WARM_MONO = {
    1, 0.20f,
    {
        //  hue    hue_var  sat    weight
        { 0.08f, 0.06f, 0.60f, 1.00f },
        { 0.00f, 0.00f, 0.00f, 0.00f },
        { 0.00f, 0.00f, 0.00f, 0.00f },
        { 0.00f, 0.00f, 0.00f, 0.00f },
    }
};

static constexpr uint32_t ORB_PAL_JWST_DEEP = 0;
static constexpr uint32_t ORB_PAL_PILLARS = 1;
static constexpr uint32_t ORB_PAL_CARINA = 2;
static constexpr uint32_t ORB_PAL_WARM_MONO = 3;
static constexpr uint32_t ORB_PAL_COUNT = 4;

static constexpr OrbPalette ORB_PALETTES[ORB_PAL_COUNT] = {
    ORB_PALETTE_JWST_DEEP,
    ORB_PALETTE_PILLARS,
    ORB_PALETTE_CARINA,
    ORB_PALETTE_WARM_MONO,
};

static constexpr const char* ORB_PAL_NAMES[ORB_PAL_COUNT] = {
    "jwst_deep", "pillars", "carina", "warm_mono"
};


// ═══ REGISTRY: TIER SETS ═════════════════════════════════════════
//
// A tier classifies "what kind of orb is this" — at init each orb
// rolls into a tier by weight, then samples its physics (mass, drag,
// size, brightness) from that tier's ranges and carries the tier's
// gains into the dynamics kernel (noise, force, color, flocking).
//
// tierset_id = 0xFFFFFFFFu (ORB_TIERSET_NONE) → uniform population,
// ignores every tier field (see configure_orbs + orb_init).

static constexpr uint32_t MAX_ORB_TIERS = 4;

struct OrbTier {
    // Physics (ranges sampled per-orb at init)
    float mass_mult = 1.0f;
    float drag_mult = 1.0f;
    float size_min = 0.7f;   // multiplier on base_size
    float size_max = 1.3f;
    float brightness_min = 0.7f;   // multiplier on palette value
    float brightness_max = 1.0f;
    // Coupling gains (apply each frame in dynamics kernel)
    float noise_gain = 1.0f;   // 0..1+
    float force_gain = 1.0f;
    float color_gain = 1.0f;
    // Selection
    float weight = 0.25f;  // relative selection probability
    // Flocking gains (used when motion_rule == 3)
    float flock_sep_gain = 1.0f;
    float flock_align_gain = 1.0f;
    float flock_coh_gain = 1.0f;
};

struct OrbTierSet {
    uint32_t count;
    OrbTier tiers[MAX_ORB_TIERS];
};

// "JWST Stars" — classic deep-field population.
//   giants  : rare, eye-catchers, slow to settle, full color, lead the flock
//   main    : the population baseline
//   faint   : numerous, small, reduced color; follows flow without clustering
//   flickers: hyperactive tiny ones; high noise, edge-scouts
static constexpr OrbTierSet ORB_TIERSET_JWST_STARS = {
    4,
    {
        //  mass   drag   s_min  s_max  b_min  b_max  n_g    f_g    c_g    w        fs     fa     fc
        {   1.8f,  0.6f,  1.2f,  1.6f,  0.9f,  1.0f,  0.6f,  0.8f,  1.0f,  0.10f,   0.6f,  0.8f,  0.3f  },  // giants
        {   1.0f,  1.0f,  0.8f,  1.1f,  0.6f,  0.9f,  1.0f,  1.0f,  1.0f,  0.60f,   1.0f,  1.0f,  1.0f  },  // main
        {   0.6f,  1.2f,  0.5f,  0.8f,  0.4f,  0.7f,  1.0f,  0.9f,  0.3f,  0.25f,   0.8f,  1.2f,  0.5f  },  // faint
        {   0.3f,  0.8f,  0.4f,  0.6f,  0.5f,  0.8f,  1.8f,  0.5f,  0.8f,  0.05f,   1.8f,  0.5f,  0.0f  },  // flickers
    }
};

// "Resonant" — voiced chamber-like sky.
//   drones : heavy, slow, convergence-responsive, strong cohesion (anchor)
//   voices : balanced baseline
//   sparks : pure motion, no color, high separation (edge agitators)
static constexpr OrbTierSet ORB_TIERSET_RESONANT = {
    3,
    {
        //  mass   drag   s_min  s_max  b_min  b_max  n_g    f_g    c_g    w        fs     fa     fc
        {   2.5f,  0.5f,  1.3f,  1.7f,  0.7f,  1.0f,  0.3f,  0.6f,  1.0f,  0.20f,   0.5f,  0.7f,  1.5f  },  // drones
        {   1.0f,  1.0f,  0.8f,  1.2f,  0.6f,  0.9f,  1.0f,  1.0f,  0.7f,  0.50f,   1.0f,  1.0f,  1.0f  },  // voices
        {   0.4f,  0.9f,  0.4f,  0.7f,  0.5f,  0.8f,  1.8f,  1.2f,  0.0f,  0.30f,   1.8f,  0.6f,  0.2f  },  // sparks
        {   0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,    0.0f,  0.0f,  0.0f  },  // (unused)
    }
};

static constexpr uint32_t ORB_TIERSET_JWST = 0;
static constexpr uint32_t ORB_TIERSET_RES = 1;
static constexpr uint32_t ORB_TIERSET_COUNT = 2;
static constexpr uint32_t ORB_TIERSET_NONE = 0xFFFFFFFFu;  // → uniform population

static constexpr OrbTierSet ORB_TIERSETS[ORB_TIERSET_COUNT] = {
    ORB_TIERSET_JWST_STARS,
    ORB_TIERSET_RESONANT,
};

static constexpr const char* ORB_TIERSET_NAMES[ORB_TIERSET_COUNT] = {
    "jwst_stars", "resonant"
};


// ═══ REGISTRY: FLOCKING GESTURES ═════════════════════════════════
//
// Eight named sign combinations over the 2³ space (separation,
// alignment, cohesion). Cycled by KP_DECIMAL at runtime. Each row
// is one recognizable behavior; sign flipping reverses the
// direction of that force in the dynamics kernel.
//
//   flock    (+++)  standard cohere-align-space
//   antiflock(---)  full dispersal
//   swirl    (+-+)  cohere but counter-flow within the cluster
//   orbit    (-+-)  ring around empty core
//   huddle   (-++)  dense aligned clump
//   flee     (++-)  spaced + aligned, runs from the center
//   chaos    (+--)  spaced + counter-flow, fleeing
//   trap     (--+)  pulled in, counter-flow, cohesive knot

struct OrbFlockGesture {
    float       sep_sign;
    float       align_sign;
    float       coh_sign;
    const char* name;
};

static constexpr uint32_t ORB_FLOCK_GESTURE_COUNT = 8;
static constexpr OrbFlockGesture ORB_FLOCK_GESTURES[ORB_FLOCK_GESTURE_COUNT] = {
    //  sep    align  coh    name
    { +1.0f, +1.0f, +1.0f, "flock"     },  // 0
    { -1.0f, -1.0f, -1.0f, "antiflock" },  // 1
    { +1.0f, -1.0f, +1.0f, "swirl"     },  // 2
    { -1.0f, +1.0f, -1.0f, "orbit"     },  // 3
    { -1.0f, +1.0f, +1.0f, "huddle"    },  // 4
    { +1.0f, +1.0f, -1.0f, "flee"      },  // 5
    { +1.0f, -1.0f, -1.0f, "chaos"     },  // 6
    { -1.0f, -1.0f, +1.0f, "trap"      },  // 7
};


// ═══ REGISTRY: BROWNIAN GESTURES ═════════════════════════════════
//
// Six variations over three binary dimensions — radial sign
// (expansion vs contraction on music), vertical bias (isotropic
// vs upward), and coherence (per-orb independent vs per-block
// shared). Cycled by the gesture key when the active rule is
// Brownian; player state persists across mood transitions.

struct OrbBrownianGesture {
    float       radial_sign;   // ±1 — polyphony expands or contracts
    float       vert_bias;     // 0 = isotropic, 1 = upward-biased noise
    float       coherence;     // 0 = per-orb, 1 = per-block ("wind gusts")
    const char* name;
};

static constexpr uint32_t ORB_BROWNIAN_GESTURE_COUNT = 6;
static constexpr OrbBrownianGesture ORB_BROWNIAN_GESTURES[ORB_BROWNIAN_GESTURE_COUNT] = {
    //  radial  vert   coh    name
    { +1.0f,   0.0f,  0.0f,  "drift"  },  // 0  default wandering diffusion
    { -1.0f,   0.0f,  0.0f,  "gather" },  // 1  music pulls orbs inward
    { +1.0f,   1.0f,  0.0f,  "rise"   },  // 2  upward drift (embers)
    { +1.0f,   0.0f,  1.0f,  "gust"   },  // 3  wind-gust group motion
    { -1.0f,   0.0f,  1.0f,  "tide"   },  // 4  coherent inward pull
    { +1.0f,   1.0f,  1.0f,  "swell"  },  // 5  upward column with coherence
};


// ═══ REGISTRY: ORBITAL GESTURES ══════════════════════════════════
//
// Four variations over axis alignment + speed variance. Cycled
// by the gesture key when the active rule is Orbital.

struct OrbOrbitalGesture {
    float       alignment_mode;   // 0 scatter, 1 parallel, 2 mirror
    float       speed_var_mult;   // multiplier on per-orb speed spread
    const char* name;
};

static constexpr uint32_t ORB_ORBITAL_GESTURE_COUNT = 4;
static constexpr OrbOrbitalGesture ORB_ORBITAL_GESTURES[ORB_ORBITAL_GESTURE_COUNT] = {
    //  align  var    name
    { 0.0f,   1.0f,  "scatter"  },  // 0  random axes, chaotic rotation
    { 1.0f,   0.0f,  "parallel" },  // 1  unified shell around Y
    { 2.0f,   0.0f,  "mirror"   },  // 2  two counter-rotating streams
    { 1.0f,   3.0f,  "shear"    },  // 3  parallel axis, layered sheets
};


// ═══ MOOD CONFIG (authoring surface) ═════════════════════════════
//
// Declared here; instantiated per-mood in ORB_MOOD_TABLE (cartridge.hpp).
// configure_orbs consumes this and sanitizes rule-critical zeros
// against the ORB_DEFAULT_* floors above.

struct OrbMoodConfig {
    // Population
    bool     enabled = false;
    uint32_t count = 0;        // clamped to Dim::MAX_ORBS
    // Color (legacy — superseded by palette_id when a palette is set)
    float    base_hue = 0.08f;
    float    hue_variance = 0.05f;
    float    brightness = 0.8f;     // value center; palette spreads around it
    // Motion
    float    drag = ORB_DEFAULT_DRAG;
    float    noise_amp = ORB_DEFAULT_NOISE_AMP;  // ceiling (floor is ORB_NOISE_FLOOR)
    uint32_t motion_rule = 0;                       // 0=Brownian 1=Orbital 2=Frozen 3=Flocking
    float    rotation_speed = 0.0f;                   // rad/s
    float    rotation_axis[3] = { 0.0f, 1.0f, 0.0f };   // normalized in configure_orbs
    float    orbital_base_speed = 0.0f;               // rad/s, rule 1 only
    // Color palette
    uint32_t palette_id = ORB_PAL_JWST_DEEP;
    bool     color_pulse_enabled = false;
    bool     color_converge_enabled = false;
    bool     color_surge_enabled = false;
    float    hue_converge_target = 0.12f;
    // Anchor (mood default applies only on first configure; player wins after)
    bool     anchor_to_pawn_default = false;
    // Population variety (ORB_TIERSET_NONE = uniform population)
    uint32_t tierset_id = ORB_TIERSET_NONE;
    // Flocking (used when motion_rule == 3)
    float    flock_sep_radius = 50.0f;
    float    flock_align_radius = 120.0f;
    float    flock_coh_radius = 200.0f;
    float    flock_sep_weight = 30.0f;
    float    flock_align_weight = 8.0f;
    float    flock_coh_weight = 15.0f;
    float    flock_max_speed = 60.0f;
    // Flock gesture seed (player cycle persists across transitions)
    uint32_t flock_gesture_default = 0u;
    // Per-rule drag multipliers (0 = pass-through, 1.0×)
    float    rule_drag_brownian = 0.0f;
    float    rule_drag_orbital = 0.0f;
    float    rule_drag_frozen = 0.0f;
    float    rule_drag_flocking = 0.0f;
};


// ═══ ORBS MODULE STATE (Scope B migration #7) ════════════════════
//
// All orb-owned state lives in this struct, accessed via orbs_state_
// on the Cartridge. Module functions take `OrbsState& os` explicitly
// rather than reaching via Cartridge*, making ownership language-
// visible and dependencies explicit in signatures.
//
// Sub-grouped by role:
//   • lifecycle      — kernel arming flags, palette
//   • anchor         — player-owned dome-center follow
//   • motion         — current rule + per-rule gesture indices
//   • couplings      — smoothed musical intensities + active flags
//   • speed          — population speed multiplier
//
// Motion rule identifiers, named for legibility at gesture-dispatch
// sites. Index into gesture_idx / gesture_initialized.

static constexpr uint32_t ORB_RULE_BROWNIAN = 0u;
static constexpr uint32_t ORB_RULE_ORBITAL = 1u;
static constexpr uint32_t ORB_RULE_FROZEN = 2u;
static constexpr uint32_t ORB_RULE_FLOCKING = 3u;

struct OrbsState {
    // ── Lifecycle / kernel arming ────────────────────────────────
    bool     active = false;
    uint32_t count = 0;
    bool     init_pending = false;
    bool     recolor_pending = false;
    uint32_t current_palette_id = ORB_PAL_JWST_DEEP;

    // ── Anchor (dome-center follow) ──────────────────────────────
    // Player state: persists across mood transitions. Dirty-flag cache
    // means an idle or unanchored dome produces no per-frame queue traffic.
    bool     pawn_anchored = false;
    bool     anchor_initialized = false;   // seeded by mood default on first configure
    float    last_dome_center_x = 0.0f;
    float    last_dome_center_z = 0.0f;
    bool     dome_center_initialized = false;

    // ── Motion rule + flocking gesture ───────────────────────────
    // Motion rule and flock gesture are both player-owned: they persist
    // across mood transitions. The rule seeds to Brownian on first run.
    uint32_t current_motion_rule = 0u;
    bool     motion_rule_initialized = false;  // one-time Brownian seed guard
    // Per-rule gesture indices. Index 2 (Frozen) is vestigial
    // — Frozen has no gestures; cycle_orb_gesture short-circuits. All
    // four indices persist across mood transitions (player state).
    uint32_t gesture_idx[4]         = { 0u, 0u, 0u, 0u };
    bool     gesture_initialized[4] = { false, false, false, false };

    // ── Musical couplings (smoothed intensities) ─────────────────
    // Force + noise share a single intensity (polyphony-driven). Color
    // and flocking have their own smoothers. Active flags gate the
    // couplings per mood.
    float    force_intensity = 0.0f;
    float    active_noise_amp = 0.0f;   // mood's configured ceiling

    float    color_pulse_intensity = 0.0f;
    float    color_converge_intensity = 0.0f;
    float    color_surge_intensity = 0.0f;
    bool     color_pulse_active = false;
    bool     color_converge_active = false;
    bool     color_surge_active = false;

    float    flock_intensity = 0.0f;
    bool     flock_active = false;  // true when active rule is Flocking

    // ── Speed ────────────────────────────────────────────────────
    // Population speed multiplier. Smoothed on the CPU, uploaded via
    // upload_orb_speed_mult only when it moves.
    float    speed_mult_current = 1.0f;
};
OrbsState orbs_state_;


// ═══ GPU LAYOUT HELPERS ══════════════════════════════════════════

// Map a tier index (0..3) to the first float of its 40-byte block
// inside a GPUOrbConfig instance. Matches the per-offset layout in
// state.hpp: tier0 starts at 192, stride 40.
static inline float* orb_tier_block_ptr(GPUOrbConfig& cfg, uint32_t i) {
    auto* base = reinterpret_cast<char*>(&cfg);
    return reinterpret_cast<float*>(base + 192u + i * 40u);
}

// Map a tier index (0..3) to the first float of its 16-byte flocking-
// gains block. Flocking gains are stored in a parallel block after
// the main tier block: base offset 416, stride 16. Fields per tier:
// sep_gain, align_gain, coh_gain, pad.
static inline float* orb_tier_flock_ptr(GPUOrbConfig& cfg, uint32_t i) {
    auto* base = reinterpret_cast<char*>(&cfg);
    return reinterpret_cast<float*>(base + 416u + i * 16u);
}


// ═══ CONFIGURE HELPERS ═══════════════════════════════════════════

// Apply mood's first-run defaults to player-owned state. Anchor and
// flock gesture are both "mood seeds once, player wins after."
static void apply_mood_first_run_defaults_(OrbsState& os, const OrbMoodConfig& cfg) {
    if (!os.anchor_initialized) {
        os.pawn_anchored = cfg.anchor_to_pawn_default;
        os.anchor_initialized = true;
    }
    // Seed each rule's gesture index on first configure.
    // The mood carries one default (flock_gesture_default); we reuse
    // it for all three rules with per-rule count clamping. A future
    // pass can split this into per-rule mood defaults if wanted.
    if (!os.gesture_initialized[ORB_RULE_BROWNIAN]) {
        os.gesture_idx[ORB_RULE_BROWNIAN] = std::min(
            cfg.flock_gesture_default, ORB_BROWNIAN_GESTURE_COUNT - 1u);
        os.gesture_initialized[ORB_RULE_BROWNIAN] = true;
    }
    if (!os.gesture_initialized[ORB_RULE_ORBITAL]) {
        os.gesture_idx[ORB_RULE_ORBITAL] = std::min(
            cfg.flock_gesture_default, ORB_ORBITAL_GESTURE_COUNT - 1u);
        os.gesture_initialized[ORB_RULE_ORBITAL] = true;
    }
    if (!os.gesture_initialized[ORB_RULE_FLOCKING]) {
        os.gesture_idx[ORB_RULE_FLOCKING] = std::min(
            cfg.flock_gesture_default, ORB_FLOCK_GESTURE_COUNT - 1u);
        os.gesture_initialized[ORB_RULE_FLOCKING] = true;
    }
    // ORB_RULE_FROZEN has no gestures — index stays at 0, unread.
}

// Pack the active palette's per-entry HSV pockets into GPU config.
static void pack_palette_(OrbsState& os, GPUOrbConfig& gpuCfg, uint32_t palette_id) {
    uint32_t pal_id = std::min(palette_id, ORB_PAL_COUNT - 1u);
    os.current_palette_id = pal_id;
    const auto& pal = ORB_PALETTES[pal_id];

    gpuCfg.palette_count = pal.count;
    gpuCfg.value_variance = pal.value_variance;
    gpuCfg.pal0_hue = pal.entries[0].hue;
    gpuCfg.pal0_hue_var = pal.entries[0].hue_var;
    gpuCfg.pal0_sat = pal.entries[0].saturation;
    gpuCfg.pal0_weight = pal.entries[0].weight;
    gpuCfg.pal1_hue = pal.entries[1].hue;
    gpuCfg.pal1_hue_var = pal.entries[1].hue_var;
    gpuCfg.pal1_sat = pal.entries[1].saturation;
    gpuCfg.pal1_weight = pal.entries[1].weight;
    gpuCfg.pal2_hue = pal.entries[2].hue;
    gpuCfg.pal2_hue_var = pal.entries[2].hue_var;
    gpuCfg.pal2_sat = pal.entries[2].saturation;
    gpuCfg.pal2_weight = pal.entries[2].weight;
    gpuCfg.pal3_hue = pal.entries[3].hue;
    gpuCfg.pal3_hue_var = pal.entries[3].hue_var;
    gpuCfg.pal3_sat = pal.entries[3].saturation;
    gpuCfg.pal3_weight = pal.entries[3].weight;
}

// Pack the selected tier set into the main tier block (with
// cumulative weights) AND the parallel flocking-gains block.
// Sentinel tierset_id → zero everything; orb_init falls back to
// legacy uniform population.
static void pack_tiers_(GPUOrbConfig& gpuCfg, uint32_t tierset_id) {
    // Note: offsets 180-188 used to be _pad_tier0/1/2 here. They now
    // hold Brownian gesture fields (brownian_radial_sign/vert_bias/
    // coherence), written by pack_flocking_. Do NOT zero them here.

    if (tierset_id >= ORB_TIERSET_COUNT) {
        // Legacy path: zero main tier block + flocking gains.
        // Skip pf[3] for tier 0 — offset 428 is orbital_speed_var_mult
        // (repurposed), written later by pack_flocking_.
        gpuCfg.tier_count = 0;
        for (uint32_t i = 0; i < MAX_ORB_TIERS; i++) {
            float* p = orb_tier_block_ptr(gpuCfg, i);
            for (int k = 0; k < 10; k++) p[k] = 0.0f;
            float* pf = orb_tier_flock_ptr(gpuCfg, i);
            pf[0] = 1.0f; pf[1] = 1.0f; pf[2] = 1.0f;
            if (i != 0u) pf[3] = 0.0f;
        }
        return;
    }

    const auto& ts = ORB_TIERSETS[tierset_id];
    uint32_t n = std::min(ts.count, MAX_ORB_TIERS);
    gpuCfg.tier_count = n;

    // Normalize weights → cumulative table so the shader can roll
    // a single uniform sample and bucket it into a tier.
    float wsum = 0.0f;
    for (uint32_t i = 0; i < n; i++) wsum += ts.tiers[i].weight;
    if (wsum < 1e-6f) wsum = 1.0f;  // pathological: avoid div-by-zero

    float cum = 0.0f;
    for (uint32_t i = 0; i < MAX_ORB_TIERS; i++) {
        float* p = orb_tier_block_ptr(gpuCfg, i);
        float* pf = orb_tier_flock_ptr(gpuCfg, i);

        if (i < n) {
            const auto& t = ts.tiers[i];
            cum += t.weight / wsum;
            if (i == n - 1) cum = 1.0f;   // clamp final bucket to avoid rounding drift
            p[0] = t.mass_mult;
            p[1] = t.drag_mult;
            p[2] = t.size_min;
            p[3] = t.size_max;
            p[4] = t.brightness_min;
            p[5] = t.brightness_max;
            p[6] = t.noise_gain;
            p[7] = t.force_gain;
            p[8] = t.color_gain;
            p[9] = cum;
            pf[0] = t.flock_sep_gain;
            pf[1] = t.flock_align_gain;
            pf[2] = t.flock_coh_gain;
            // pf[3] for tier 0 is orbital_speed_var_mult — don't touch.
            if (i != 0u) pf[3] = 0.0f;
        }
        else {
            // Unused tier slot: zero fields, cumulative = 1.0 so no roll lands here.
            for (int k = 0; k < 10; k++) p[k] = 0.0f;
            p[9] = 1.0f;
            pf[0] = 1.0f; pf[1] = 1.0f; pf[2] = 1.0f;
            if (i != 0u) pf[3] = 0.0f;
        }
    }
}

// Pack flocking params (mood-authored, sanitized), gesture signs
// (from current player-owned gesture index), and per-rule drag
// multipliers (sanitized with zero → pass-through).
static void pack_flocking_(const OrbsState& os, GPUOrbConfig& gpuCfg,
    float sep_r, float align_r, float coh_r,
    float sep_w, float align_w, float coh_w,
    float max_speed,
    float rule_drag_bwn, float rule_drag_orb,
    float rule_drag_frz, float rule_drag_flk) {
    gpuCfg.flock_sep_radius = sep_r;
    gpuCfg.flock_align_radius = align_r;
    gpuCfg.flock_coh_radius = coh_r;
    gpuCfg.flock_sep_weight = sep_w;
    gpuCfg.flock_align_weight = align_w;
    gpuCfg.flock_coh_weight = coh_w;
    gpuCfg.flock_max_speed = max_speed;
    gpuCfg.flock_coupling_intensity = 0.0f;

    // Pack all three rule gesture bundles. Each rule reads
    // its own slice in the dynamics kernel; writing all three at
    // configure time keeps them in sync whatever rule becomes active.
    {
        const auto& gb = ORB_BROWNIAN_GESTURES[os.gesture_idx[ORB_RULE_BROWNIAN]];
        gpuCfg.brownian_radial_sign = gb.radial_sign;
        gpuCfg.brownian_vert_bias = gb.vert_bias;
        gpuCfg.brownian_coherence = gb.coherence;

        const auto& go = ORB_ORBITAL_GESTURES[os.gesture_idx[ORB_RULE_ORBITAL]];
        gpuCfg.orbital_alignment_mode = go.alignment_mode;
        gpuCfg.orbital_speed_var_mult = go.speed_var_mult;

        const auto& gf = ORB_FLOCK_GESTURES[os.gesture_idx[ORB_RULE_FLOCKING]];
        gpuCfg.flock_sep_sign = gf.sep_sign;
        gpuCfg.flock_align_sign = gf.align_sign;
        gpuCfg.flock_coh_sign = gf.coh_sign;
    }

    // Per-rule drag: zero → 1.0× pass-through (mood has no opinion).
    auto passthrough = [](float authored) {
        return (authored > 0.0f) ? authored : 1.0f;
        };
    gpuCfg.rule_drag_brownian = passthrough(rule_drag_bwn);
    gpuCfg.rule_drag_orbital = passthrough(rule_drag_orb);
    gpuCfg.rule_drag_frozen = passthrough(rule_drag_frz);
    gpuCfg.rule_drag_flocking = passthrough(rule_drag_flk);

    // Preserve the current smoothed speed multiplier across
    // mood transitions — use the in-flight value rather than a literal
    // 1.0 so a mood portal doesn't snap the sky back to baseline.
    gpuCfg.speed_mult = os.speed_mult_current;
}

// Log the effective config after sanitization. Shows what the GPU
// actually runs with (not what the mood authored), which is what
// the operator cares about when cycling rules or tuning.
static void log_configure_(const OrbsState& os, const OrbMoodConfig& cfg,
    float eff_drag, float eff_orbital_speed,
    uint32_t palette_id) {
    static const char* RULE_NAMES[] = { "brownian", "orbital", "frozen", "flocking" };

    std::cout << "[Orbs] Configured: count=" << os.count
        << " palette=" << ORB_PAL_NAMES[palette_id]
        << " drag=" << eff_drag
        << " noise=" << ORB_NOISE_FLOOR << ".." << os.active_noise_amp
        << " rule=" << RULE_NAMES[std::min(os.current_motion_rule, 3u)]
        << " rot=" << cfg.rotation_speed
        << " orbital=" << eff_orbital_speed
        << " color:"
        << (cfg.color_pulse_enabled ? " pulse" : "")
        << (cfg.color_converge_enabled ? " converge" : "")
        << (cfg.color_surge_enabled ? " surge" : "")
        << (cfg.color_pulse_enabled || cfg.color_converge_enabled || cfg.color_surge_enabled
            ? "" : " off")
        << " anchor=" << (os.pawn_anchored ? "pawn" : "origin")
        << " tiers="
        << (cfg.tierset_id < ORB_TIERSET_COUNT
            ? ORB_TIERSET_NAMES[cfg.tierset_id]
            : "legacy")
        << "\n";
}


// ═══ LIFECYCLE ═══════════════════════════════════════════════════

// Mood entry: sanitize rule-critical zeros against system defaults,
// apply first-run player defaults, pack the full GPU config, arm
// the init kernel. Called from apply_mood (mood.inl) and from the
// initial-mood setup in the init path (cartridge.hpp).
static void configure_orbs(OrbsState& os, Cartridge* c, const OrbMoodConfig& cfg, wgpu::Queue& queue) {
    os.active = cfg.enabled;
    os.count = std::min(cfg.count, (uint32_t)Dim::MAX_ORBS);
    if (!os.active || os.count == 0) return;

    // Effective values: "zero = no opinion, use system default."
    auto eff = [](float authored, float fallback) {
        return (authored > 0.0f) ? authored : fallback;
        };
    const float eff_drag = eff(cfg.drag, ORB_DEFAULT_DRAG);
    const float eff_noise_amp = eff(cfg.noise_amp, ORB_DEFAULT_NOISE_AMP);
    const float eff_orbital_speed = eff(cfg.orbital_base_speed, ORB_DEFAULT_ORBITAL_SPEED);
    const float eff_flock_sep_r = eff(cfg.flock_sep_radius, ORB_DEFAULT_FLOCK_SEP_R);
    const float eff_flock_align_r = eff(cfg.flock_align_radius, ORB_DEFAULT_FLOCK_ALIGN_R);
    const float eff_flock_coh_r = eff(cfg.flock_coh_radius, ORB_DEFAULT_FLOCK_COH_R);
    const float eff_flock_sep_w = eff(cfg.flock_sep_weight, ORB_DEFAULT_FLOCK_SEP_W);
    const float eff_flock_align_w = eff(cfg.flock_align_weight, ORB_DEFAULT_FLOCK_ALIGN_W);
    const float eff_flock_coh_w = eff(cfg.flock_coh_weight, ORB_DEFAULT_FLOCK_COH_W);
    const float eff_flock_max_speed = eff(cfg.flock_max_speed, ORB_DEFAULT_FLOCK_MAX_SPEED);

    os.active_noise_amp = eff_noise_amp;

    // First-run mood defaults for player-owned state.
    apply_mood_first_run_defaults_(os, cfg);
    // Motion rule is player-owned (like the flock gesture): seed once to
    // Brownian, then leave it — mood transitions no longer overwrite it.
    if (!os.motion_rule_initialized) {
        os.current_motion_rule = ORB_RULE_BROWNIAN;
        os.motion_rule_initialized = true;
    }

    // Normalize rotation axis on CPU (GPU renormalizes too but this
    // keeps uploaded values unit-length to avoid surprises).
    float rx = cfg.rotation_axis[0];
    float ry = cfg.rotation_axis[1];
    float rz = cfg.rotation_axis[2];
    float rlen = std::sqrt(rx * rx + ry * ry + rz * rz);
    if (rlen > 0.001f) { rx /= rlen; ry /= rlen; rz /= rlen; }
    else { rx = 0.0f; ry = 1.0f; rz = 0.0f; }

    // Build the GPU config in one place.
    GPUOrbConfig gpuCfg{};
    gpuCfg.count = os.count;
    gpuCfg.seed = c->world_state_.active_seed;
    gpuCfg.base_hue = cfg.base_hue;
    gpuCfg.hue_variance = cfg.hue_variance;
    gpuCfg.brightness = cfg.brightness;
    gpuCfg.drag = eff_drag;
    gpuCfg.noise_amp = ORB_NOISE_FLOOR;   // start at floor; coupling lerps up to ceiling
    gpuCfg.dome_radius = ORB_DOME_RADIUS;
    gpuCfg.base_size = ORB_BASE_SIZE;
    gpuCfg.dt = 0.0f;
    gpuCfg.t_seconds = 0.0f;
    gpuCfg.force_radial = 0.0f;
    gpuCfg.motion_rule = os.current_motion_rule;
    gpuCfg.rotation_speed = cfg.rotation_speed;
    gpuCfg.rotation_axis_x = rx;
    gpuCfg.rotation_axis_y = ry;
    gpuCfg.rotation_axis_z = rz;
    gpuCfg.orbital_base_speed = eff_orbital_speed;

    pack_palette_(os, gpuCfg, cfg.palette_id);

    // Color dynamics start at rest; couplings lift them under music.
    // hue_converge_target is mood-scoped (changes only at mood entry).
    gpuCfg.color_pulse = 0.0f;
    gpuCfg.color_converge = 0.0f;
    gpuCfg.color_surge = 0.0f;
    gpuCfg.hue_converge_target = cfg.hue_converge_target;

    // Dome center: start at origin; per-frame update_orb_anchor catches
    // up next frame with fresh pawnReadback values (not yet ready here).
    gpuCfg.dome_center_x = 0.0f;
    gpuCfg.dome_center_y = 0.0f;
    gpuCfg.dome_center_z = 0.0f;
    gpuCfg._pad_anchor = 0.0f;
    os.dome_center_initialized = false;   // force dirty-flag re-eval

    os.color_pulse_active = cfg.color_pulse_enabled;
    os.color_converge_active = cfg.color_converge_enabled;
    os.color_surge_active = cfg.color_surge_enabled;

    pack_tiers_(gpuCfg, cfg.tierset_id);
    pack_flocking_(os, gpuCfg,
        eff_flock_sep_r, eff_flock_align_r, eff_flock_coh_r,
        eff_flock_sep_w, eff_flock_align_w, eff_flock_coh_w,
        eff_flock_max_speed,
        cfg.rule_drag_brownian, cfg.rule_drag_orbital,
        cfg.rule_drag_frozen, cfg.rule_drag_flocking);

    os.flock_active = (os.current_motion_rule == ORB_RULE_FLOCKING);

    c->gpuState_.upload_orb_config(queue, gpuCfg);
    os.init_pending = true;

    log_configure_(os, cfg, eff_drag, eff_orbital_speed, os.current_palette_id);
}

// Mood exit: stop dispatching. Resets mood-owned runtime state
// (intensities, active flags, dome-center cache). Preserves
// player-owned state (anchor, flock gesture) across transitions.
static void teardown_orbs(OrbsState& os, Cartridge* c) {
    os.active = false;
    os.count = 0;
    os.init_pending = false;
    os.recolor_pending = false;

    os.force_intensity = 0.0f;
    os.active_noise_amp = 0.0f;

    os.color_pulse_intensity = 0.0f;
    os.color_converge_intensity = 0.0f;
    os.color_surge_intensity = 0.0f;
    os.color_pulse_active = false;
    os.color_converge_active = false;
    os.color_surge_active = false;

    os.flock_intensity = 0.0f;
    os.flock_active = false;

    // Speed multiplier resets with the mood (not player state).
    os.speed_mult_current = 1.0f;

    // Force fresh anchor upload on next configure (cache cleared, flag state preserved).
    os.last_dome_center_x = 0.0f;
    os.last_dome_center_z = 0.0f;
    os.dome_center_initialized = false;
}


// ═══ PLAYER COMMANDS ═════════════════════════════════════════════

// KP_0: cycle palette forward. Re-arms the recolor kernel so
// positions and colors both refresh on the next frame. Session-
// local within a mood — the next mood transition resets to that
// mood's configured palette.
static void cycle_orb_palette(OrbsState& os, Cartridge* c, wgpu::Queue& queue) {
    if (!os.active || os.count == 0) {
        std::cout << "[Orbs] Palette cycle ignored (no active dome)\n";
        return;
    }

    os.current_palette_id = (os.current_palette_id + 1u) % ORB_PAL_COUNT;
    const auto& pal = ORB_PALETTES[os.current_palette_id];

    float pal_data[16];
    for (uint32_t i = 0; i < 4; i++) {
        pal_data[i * 4 + 0] = pal.entries[i].hue;
        pal_data[i * 4 + 1] = pal.entries[i].hue_var;
        pal_data[i * 4 + 2] = pal.entries[i].saturation;
        pal_data[i * 4 + 3] = pal.entries[i].weight;
    }
    c->gpuState_.upload_orb_palette(queue, pal.count, pal.value_variance, pal_data);

    // Color-only refresh: positions, velocities, twinkle phase all
    // persist so the sky "holds" and only the hues transition.
    os.recolor_pending = true;

    std::cout << "[Orbs] Palette: " << ORB_PAL_NAMES[os.current_palette_id] << "\n";
}

// KP_8: cycle motion rule (Brownian → Orbital → Frozen → Flocking → …).
// Does NOT reset orb state — positions and velocities carry over so
// the character shifts seamlessly into the new rule.
static void cycle_orb_motion_rule(OrbsState& os, Cartridge* c, wgpu::Queue& queue) {
    if (!os.active || os.count == 0) {
        std::cout << "[Orbs] Motion rule cycle ignored (no active dome)\n";
        return;
    }

    os.current_motion_rule = (os.current_motion_rule + 1u) % 4u;
    c->gpuState_.upload_orb_motion_rule(queue, os.current_motion_rule);
    os.flock_active = (os.current_motion_rule == 3u);   // coupling gate

    static const char* RULE_NAMES[] = { "brownian", "orbital", "frozen", "flocking" };
    std::cout << "[Orbs] Motion rule: " << RULE_NAMES[os.current_motion_rule];
    const uint32_t r = os.current_motion_rule;
    const uint32_t gidx = os.gesture_idx[r];
    if (r == ORB_RULE_BROWNIAN && gidx != 0u) {
        std::cout << " (gesture: " << ORB_BROWNIAN_GESTURES[gidx].name << ")";
    }
    else if (r == ORB_RULE_ORBITAL && gidx != 0u) {
        std::cout << " (gesture: " << ORB_ORBITAL_GESTURES[gidx].name << ")";
    }
    else if (r == ORB_RULE_FLOCKING && gidx != 0u) {
        std::cout << " (gesture: " << ORB_FLOCK_GESTURES[gidx].name << ")";
    }
    std::cout << "\n";
}

// KP_DECIMAL: cycle the gesture registry for the CURRENTLY ACTIVE
// motion rule. Brownian / Orbital / Flocking each have their own
// table; Frozen has none and short-circuits. Player state: each
// rule's index persists across mood transitions.
static void cycle_orb_gesture(OrbsState& os, Cartridge* c, wgpu::Queue& queue) {
    const uint32_t r = os.current_motion_rule;

    if (r == ORB_RULE_BROWNIAN) {
        os.gesture_idx[r] = (os.gesture_idx[r] + 1u) % ORB_BROWNIAN_GESTURE_COUNT;
        const auto& g = ORB_BROWNIAN_GESTURES[os.gesture_idx[r]];
        c->gpuState_.upload_orb_brownian_gesture(queue,
            g.radial_sign, g.vert_bias, g.coherence);
        std::cout << "[Orbs] Brownian gesture: " << g.name << "\n";
        return;
    }
    if (r == ORB_RULE_ORBITAL) {
        os.gesture_idx[r] = (os.gesture_idx[r] + 1u) % ORB_ORBITAL_GESTURE_COUNT;
        const auto& g = ORB_ORBITAL_GESTURES[os.gesture_idx[r]];
        c->gpuState_.upload_orb_orbital_gesture(queue,
            g.alignment_mode, g.speed_var_mult);
        std::cout << "[Orbs] Orbital gesture: " << g.name << "\n";
        return;
    }
    if (r == ORB_RULE_FLOCKING) {
        os.gesture_idx[r] = (os.gesture_idx[r] + 1u) % ORB_FLOCK_GESTURE_COUNT;
        const auto& g = ORB_FLOCK_GESTURES[os.gesture_idx[r]];
        c->gpuState_.upload_orb_flock_signs(queue,
            g.sep_sign, g.align_sign, g.coh_sign);
        std::cout << "[Orbs] Flocking gesture: " << g.name
            << " (sep=" << (g.sep_sign > 0 ? "+" : "-")
            << " align=" << (g.align_sign > 0 ? "+" : "-")
            << " coh=" << (g.coh_sign > 0 ? "+" : "-")
            << ")\n";
        return;
    }

    // ORB_RULE_FROZEN: stillness is the rule's defining property.
    std::cout << "[Orbs] Frozen has no gestures (stillness is the rule).\n";
}

// KP_9: flip dome anchor between world origin and pawn-following.
// Player state: persists across mood transitions.
static void toggle_orb_anchor(OrbsState& os, const Cartridge* c) {
    os.pawn_anchored = !os.pawn_anchored;
    std::cout << "[Orbs] Anchor: "
        << (os.pawn_anchored ? "ON — dome follows pawn"
            : "OFF — dome fixed at world origin")
        << "  (pawn readback: "
        << c->player_.readback_x << ", " << c->player_.readback_z << ")"
        << "\n";
}


// ═══ PER-FRAME UPDATES ═══════════════════════════════════════════

// Push the current dome center to the GPU. Dirty-flagged: a stationary
// anchored pawn or an unanchored session produces no per-frame traffic.
// Horizontal-only: Y is always 0 regardless of pawn altitude so the
// sky doesn't bob with terrain.
static void update_orb_anchor(OrbsState& os, Cartridge* c, float pawn_x, float pawn_z, wgpu::Queue& queue) {
    if (!os.active || os.count == 0) return;

    float target_x = os.pawn_anchored ? pawn_x : 0.0f;
    float target_z = os.pawn_anchored ? pawn_z : 0.0f;

    bool changed = !os.dome_center_initialized
        || target_x != os.last_dome_center_x
        || target_z != os.last_dome_center_z;

    if (changed) {
        c->gpuState_.upload_orb_dome_center(queue, target_x, 0.0f, target_z);
        os.last_dome_center_x = target_x;
        os.last_dome_center_z = target_z;
        os.dome_center_initialized = true;
    }
}

// Polyphony drives three smoothed intensities:
//   force+noise (motion) — radial expansion + noise ceiling lerp
//   color (pulse/converge/surge) — three trajectories, mood-gated
//   flock — cohesion/separation balance
// Each is structurally independent; they share polyphony for now
// so they move together. Swapping sources later desyncs the channels.
// Uploads fire only when a value actually moves.
//
// SEAM[orbs:P1] this function is the architectural exemplar for the
//   "per-frame coupling lives in the owning module, not the spine"
//   pattern. Originally the only instance; now the convention,
//   joined by tick_musical_couplings (musical:K2),
//   reset_musical_couplings (mood:K3), and tick_pawn_couplings
//   (pawn:K1). Spine update() is a phase-orchestration call list;
//   each named tick lives where its data lives. Referenced from
//   cartridge.hpp::update() at the orb coupling call site.
static void update_orb_coupling(OrbsState& os, Cartridge* c, float polyphony, float dt, wgpu::Queue& queue) {
    if (!os.active || os.count == 0) return;

    float target = std::min(polyphony / 6.0f, 1.0f);

    // Local smoother — returns true iff the intensity actually moved.
    auto smooth = [&](bool enabled, float& intensity,
        float attack, float release) -> bool {
            float t = enabled ? target : 0.0f;
            float prev = intensity;
            float rate = (t > prev) ? attack : release;
            float next = prev + (t - prev) * (1.0f - std::exp(-rate * dt));
            if (next < 0.001f && t == 0.0f) next = 0.0f;
            if (next > 0.999f && t >= 1.0f) next = 1.0f;
            if (next != prev) { intensity = next; return true; }
            return false;
        };

    // Motion: force + noise share one intensity.
    if (smooth(true, os.force_intensity, ORB_FORCE_ATTACK, ORB_FORCE_RELEASE)) {
        float radial = os.force_intensity * ORB_FORCE_SCALE;
        c->gpuState_.upload_orb_force(queue, radial);

        float noise = ORB_NOISE_FLOOR
            + os.force_intensity * (os.active_noise_amp - ORB_NOISE_FLOOR);
        c->gpuState_.upload_orb_noise(queue, noise);
    }

    // Color: three independent trajectories, each gated per-mood.
    bool color_changed = false;
    if (smooth(os.color_pulse_active, os.color_pulse_intensity,
        ORB_COLOR_ATTACK, ORB_COLOR_RELEASE))    color_changed = true;
    if (smooth(os.color_converge_active, os.color_converge_intensity,
        ORB_COLOR_ATTACK, ORB_COLOR_RELEASE))    color_changed = true;
    if (smooth(os.color_surge_active, os.color_surge_intensity,
        ORB_COLOR_ATTACK, ORB_COLOR_RELEASE))    color_changed = true;

    if (color_changed) {
        c->gpuState_.upload_orb_color_dynamics(queue,
            os.color_pulse_intensity,
            os.color_converge_intensity,
            os.color_surge_intensity);
    }

    // Flocking: separate attack/release from color so the flock
    // tightens as a gesture rather than snapping.
    if (os.flock_active) {
        float prev = os.flock_intensity;
        float rate = (target > prev) ? ORB_FLOCK_ATTACK : ORB_FLOCK_RELEASE;
        float next = prev + (target - prev) * (1.0f - std::exp(-rate * dt));
        if (next < 0.001f && target == 0.0f) next = 0.0f;
        if (next > 0.999f && target >= 1.0f) next = 1.0f;
        if (next != prev) {
            os.flock_intensity = next;
            c->gpuState_.upload_orb_flock_intensity(queue, os.flock_intensity);
        }
    }

    // Population speed attractor. `target` (computed above,
    // clamped polyphony 0..1) drives speed_target from 1.0 at silence
    // up to ORB_SPEED_CEILING at full music. The sky breathes in
    // literal speed-of-motion with whatever the music is doing.
    {
        const float speed_target = 1.0f + target * (ORB_SPEED_CEILING - 1.0f);

        float prev = os.speed_mult_current;
        float rate = (speed_target > prev) ? ORB_SPEED_ATTACK : ORB_SPEED_RELEASE;
        float next = prev + (speed_target - prev) * (1.0f - std::exp(-rate * dt));
        // Snap back to unity when silence returns, so the idle state
        // stops generating queue traffic.
        if (next < 1.001f && next > 0.999f && speed_target == 1.0f) next = 1.0f;
        if (next != prev) {
            os.speed_mult_current = next;
            c->gpuState_.upload_orb_speed_mult(queue, os.speed_mult_current);
        }
    }
}


// ═══ GPU DISPATCHES ══════════════════════════════════════════════

// One-shot seed-to-dome kernel. Fires once per configure_orbs when
// os.init_pending is armed.
static void dispatch_orb_init(OrbsState& os, Cartridge* c, wgpu::CommandEncoder& encoder) {
    if (!os.init_pending) return;
    os.init_pending = false;

    wgpu::ComputePassDescriptor cpd{};
    cpd.label = "Orb Init";
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&cpd);
    uint32_t wgs = (os.count + 63u) / 64u;
    c->renderer_.dispatch_orb_init(pass, c->gpuState_.orb_compute_group(), wgs);
    pass.End();

    std::cout << "[Orbs] Init dispatched: " << os.count
        << " orbs, " << wgs << " workgroups\n";
}

// Palette re-sample kernel. Fires on cycle_orb_palette — keeps
// positions/velocities/twinkle, only hues shift.
static void dispatch_orb_recolor(OrbsState& os, Cartridge* c, wgpu::CommandEncoder& encoder) {
    if (!os.recolor_pending) return;
    os.recolor_pending = false;

    wgpu::ComputePassDescriptor cpd{};
    cpd.label = "Orb Recolor";
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&cpd);
    uint32_t wgs = (os.count + 63u) / 64u;
    c->renderer_.dispatch_orb_recolor(pass, c->gpuState_.orb_compute_group(), wgs);
    pass.End();
}

// Snapshot orb_state → orb_state_prev. Dynamics reads the snapshot
// for neighbor queries so flocking invocations all see a stable
// previous-frame view. Cheap (N parallel copies) — runs every frame
// regardless of motion_rule so future rules can rely on prev.
static void dispatch_orb_copy_prev(OrbsState& os, Cartridge* c, wgpu::CommandEncoder& encoder) {
    if (!os.active || os.count == 0) return;

    wgpu::ComputePassDescriptor cpd{};
    cpd.label = "Orb Copy Prev";
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&cpd);
    uint32_t wgs = (os.count + 63u) / 64u;
    c->renderer_.dispatch_orb_copy_prev(pass, c->gpuState_.orb_copy_group(), wgs);
    pass.End();
}

// Per-frame rule + couplings. Uploads dt/t_seconds, then dispatches.
static void dispatch_orb_dynamics(OrbsState& os, Cartridge* c, wgpu::CommandEncoder& encoder,
    wgpu::Queue& queue) {
    if (!os.active || os.count == 0) return;

    c->gpuState_.upload_orb_frame(queue, c->time_state_.dt, c->time_state_.seconds);

    wgpu::ComputePassDescriptor cpd{};
    cpd.label = "Orb Dynamics";
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&cpd);
    uint32_t wgs = (os.count + 63u) / 64u;
    c->renderer_.dispatch_orb_dynamics(pass, c->gpuState_.orb_compute_group(), wgs);
    pass.End();
}

// ═══ RENDER ══════════════════════════════════════════════════════

// Additive billboard draw into the main render pass.
static void render_orbs(OrbsState& os, Cartridge* c, wgpu::RenderPassEncoder& pass) {
    if (!os.active || os.count == 0) return;
    c->renderer_.draw_orbs(pass,
        c->gpuState_.render_entity_group(),
        c->gpuState_.render_texture_group(),
        c->gpuState_.orb_quad_vb(),
        c->gpuState_.orb_quad_ib(),
        os.count);
}

// ─── Orb Mood Table ─────────────────────────────────────────────
//
// Lives at end-of-file because OrbMoodConfig is defined above and
// MOOD_COUNT is visible here (declared in cartridge.hpp before this
// module is #included). Resolves orbs:D2.
//
// DONE[orbs:D2] per-mood orb authoring data lives with the
//   OrbMoodConfig struct that defines its shape.
// SEAM[mood:K4] mood-5 row is bit-identical to mood-0 (open_default)
//   except for the implicit context that mood-5 is finite_outdoor_ref.
//   Mirrors the same MOOD_TABLE pattern. Resolves with the
//   has_anchor_ribbon flag (mood:L1).
//
// Per-mood orb config. Indexed by the same mood index as MOOD_TABLE.
// See OrbMoodConfig in orbs.inl (above) for field semantics. Zero-
// valued rule-critical fields (drg, nz, orbS, flock radii/weights)
// are sanitized to system defaults in configure_orbs — "0 = no
// opinion, system picks a working value." Explicit small non-zero
// reads as deliberate authorship.
//
//  Column legend (short → field name):
//    en      enabled              rotAxis rotation_axis[3]
//    n       count                orbS    orbital_base_speed (rule 1)
//    hueB    base_hue (legacy)    pal     palette_id (ORB_PAL_*)
//    hueV    hue_variance         pul     color_pulse_enabled
//    bri     brightness           cnv     color_converge_enabled
//    drg     drag (1/s)           srg     color_surge_enabled
//    nz      noise_amp ceiling    hct     hue_converge_target
//    rul     motion_rule          anc     anchor_to_pawn_default
//    rotS    rotation_speed       trs     tierset_id (0xFFFFFFFFu = legacy)
//    sepR/alnR/cohR/sepW/alnW/cohW/maxS   flocking parameters
//    gst     flock_gesture_default (0..7, ORB_FLOCK_GESTURES index)
//    drgB/drgO/drgF/drgK          per-rule drag multipliers (0 = 1.0× pass-through)
//
//                                              en     n    hueB   hueV   bri    drg   nz      rul  rotS    rotAxis                  orbS  pal  pul    cnv    srg    hct    anc    trs           sepR   alnR    cohR    sepW   alnW   cohW   maxS   gst  drgB  drgO  drgF  drgK
static constexpr OrbMoodConfig ORB_MOOD_TABLE[MOOD_COUNT] = {
    /* 0 open_default        */ {  true,  128, 0.08f, 0.06f, 0.85f, 0.4f, 20.0f,  0u,  0.012f, {0.15f, 0.97f, 0.10f},  0.0f, 0u,  true,  true,  true,  0.12f, true,  0u,           50.0f, 120.0f, 200.0f, 30.0f, 8.0f,  15.0f, 60.0f, 0u,  0.0f, 0.0f, 0.0f, 0.0f },
    /* 1 open_sunset         */ {  true,  128, 0.08f, 0.06f, 0.85f, 0.4f, 20.0f,  3u,  0.012f, {0.15f, 0.97f, 0.10f},  0.0f, 0u,  true,  false, false, 0.08f, false, 0u,           50.0f, 120.0f, 200.0f, 30.0f, 8.0f,  15.0f, 60.0f, 0u,  0.0f, 0.0f, 0.0f, 0.0f },
    /* 2 indoor_flat         */ {  false, 0,   0.08f, 0.05f, 0.80f, 0.5f, 0.0f,   0u,  0.000f, {0.00f, 1.00f, 0.00f},  0.0f, 0u,  false, false, false, 0.12f, false, 0xFFFFFFFFu,  50.0f, 120.0f, 200.0f, 30.0f, 8.0f,  15.0f, 60.0f, 0u,  0.0f, 0.0f, 0.0f, 0.0f },
    /* 3 indoor_vault        */ {  false, 0,   0.08f, 0.05f, 0.80f, 0.5f, 0.0f,   0u,  0.000f, {0.00f, 1.00f, 0.00f},  0.0f, 0u,  false, false, false, 0.12f, false, 0xFFFFFFFFu,  50.0f, 120.0f, 200.0f, 30.0f, 8.0f,  15.0f, 60.0f, 0u,  0.0f, 0.0f, 0.0f, 0.0f },
    /* 4 finite_outdoor      */ {  true,  128, 0.08f, 0.06f, 0.85f, 0.4f, 20.0f,  0u,  0.012f, {0.15f, 0.97f, 0.10f},  0.0f, 0u,  true,  true,  true,  0.12f, true,  0u,           50.0f, 120.0f, 200.0f, 30.0f, 8.0f,  15.0f, 60.0f, 0u,  0.0f, 0.0f, 0.0f, 0.0f },
    /* 5 finite_outdoor_ref  */ {  true,  128, 0.08f, 0.06f, 0.85f, 0.4f, 20.0f,  0u,  0.012f, {0.15f, 0.97f, 0.10f},  0.0f, 0u,  true,  true,  true,  0.12f, true,  0u,           50.0f, 120.0f, 200.0f, 30.0f, 8.0f,  15.0f, 60.0f, 0u,  0.0f, 0.0f, 0.0f, 0.0f },
};
