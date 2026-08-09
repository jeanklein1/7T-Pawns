#pragma once
#include <cstdint>
#include <random>     // std::mt19937 + distributions (PhotographerState sampling)
#include <string>
#include <vector>     // authored disk manifest
#include "cartridges/the_board/realization/state.hpp"                    // Dim::*, GPUPaintingSlot, GPUPhotographerConfig, wgpu
#include "cartridges/the_board/contracts/mood_constants.hpp"   // MOOD_COUNT (sizes the mood gate)
#include "cartridges/the_board/primitives/seed_utils.hpp"       // select_weighted (PhotographerState::sample_shot_type)
#include "cartridges/the_board/contracts/wgpu_fwd.hpp"   // wgpu handle fwds (lockstep insurance)
#include "external/stb_image.h"                                // stbi_load/stbi_image_free — authored painting loader (dependency named here)
#include "cartridges/the_board/contracts/entity_types.hpp"     // GallerySelection/GalleryPlacement (the boundary DTOs) + queue types

// ─── gallery.hpp (HEADER: vocabulary + configs + state + decls) ──
// History: audit/LADDER.md
//
// The art system.
//
// The impl additionally reaches the spawn-engine services and
// GLOBAL_ENTITY_DENSITY (contracts/spawn_services.hpp), Dim::PATCH_EXTENT
// (patch_system.hpp), PopFamily (roster.hpp vocabulary), and
// stb_image (authored disk loading).
//
// SEAM[gallery:complete-subsystem] complete bespoke pipeline in one
//   module — vocabulary + state + lifecycle + dispatch all together.
//   Same family as gol_zones.hpp and ribbon.hpp.
// SEAM[gallery:dual-role] two named sub-systems sharing infrastructure:
//   painting-on-terrain (outdoor) and painting-on-wall (indoor) with
//   shared image loading + frame rendering, divergent spawn paths.
//   The header's "Two halves" box names the division. Not a leak;
//   intentional dual role.
// SEAM[gallery:P8] keeps ENVIRONMENTAL at weight
//   0.01 deliberately — authored-but-unused, kept available for a
//   future "wide environmental" framing pass. Same family as the
//   ribbon harmonic-ratio palettes (ribbon:P8).
// ─────────────────────────────────────────────────────────────────

#include <algorithm>   // std::max, std::min, std::sort, std::transform   // (impl, merged)
#include <cmath>       // std::sqrt, std::floor, std::cos, std::sin, std::round   // (impl, merged)
#include <cstdint>   // (impl, merged)
#include <filesystem>  // paintings folder scan   // (impl, merged)
#include <iostream>    // capture / gallery / authored logs   // (impl, merged)
#include <string>      // manifest paths, std::stoi   // (impl, merged)
#include <vector>      // manifest + pixel staging   // (impl, merged)
#ifdef __EMSCRIPTEN__
#include <cstring>            // std::strncpy — emscripten_fetch_attr_t::requestMethod   (EXHIBIT_0)
#include <emscripten/fetch.h> // the web twin's byte source: the network, not a filesystem (EXHIBIT_0)
#endif

namespace t7 {
namespace the_board {

// ═══ MODULE DEPS ════════════════════════════════════════════════════
// The photographer + exhibition's requirements face. sun/clear are
// the root cosmetic arrays (by reference); ribbon_state_ read for the
// snapshot draw gate. All reads except GPU wire + renderer.
class Renderer;
struct WorldState; struct TileWorldState; struct RibbonState;
struct GalleryDeps {
    GPUState&             gpuState_;
    Renderer&            renderer_;
    const WorldState&    world_state_;
    const TileWorldState& tile_world_state_;
    const RibbonState&   ribbon_state_;
    const PlayerState&   player_;
    const PointState&    point_;    // the point's house (position mirror — wall-frame placement)
    const MoodState&     mood_state_;
    const float          (&sunDirection_)[3];
    const float          (&clearColor_)[3];
};

// ═══ SHOT TIERS (vocabulary) ═════════════════════════════════════

enum class ShotType : uint32_t {
    PANORAMIC = 0,   // distant landscape, pawn small in frame
    ENVIRONMENTAL = 1,   // wide terrain study, pawn incidental
    MEDIUM = 2,   // balanced framing, pawn clearly visible
    CLOSE_UP = 3,   // near, pawn fills much of the frame
    PORTRAIT = 4,   // intimate vertical, pawn centered
    BIRDS_EYE = 5,   // steep overhead, map-like perspective
    LOW_ANGLE = 6,   // near ground level, looking up at pawn
    CINEMATIC = 7,   // dramatic distance + wide lens distortion
    COUNT = 8
};

struct ShotTypeParams {
    float distance_mean, distance_sigma;    // camera-to-pawn distance (world units)
    float elevation_mean, elevation_sigma;  // angle above horizon (radians)
    float fov_degrees, fov_sigma;           // vertical field of view (degrees)
    float aspect_lo, aspect_hi;             // painting width/height ratio range
    bool tracks_pawn;                       // camera aims at pawn vs free direction
    float offset_x_range;                   // max horizontal frame shift (symmetric)
    float offset_y_range;                   // max vertical frame shift (symmetric)
    float weight;                           // tier selection probability (all must sum to 1.0)
};

// ─── Tier Definitions ───────────────────────────────────────────

//                          dist  σ     elev   σ     fov    σ     asp_lo asp_hi  track  off_x  off_y   weight
// ENVIRONMENTAL keeps weight 0.01 deliberately — held near-zero rather
// than removed (deleting the enum value would rotate every downstream
// tier index); bump the weight to revive. See SEAM[gallery:P8].
inline constexpr ShotTypeParams SHOT_PARAMS[] = {
    /* PANORAMIC     */ {  6.0f, 4.0f,  0.16f, 0.15f,  45.0f, 15.0f,  1.78f, 2.35f,  true,  0.6f, 0.4f,   0.30f },
    /* ENVIRONMENTAL */ { 10.0f, 4.0f,  0.30f, 0.15f,  45.0f, 10.0f,  1.50f, 2.00f,  true,  0.7f, 0.5f,   0.01f },
    /* MEDIUM        */ {  6.0f, 2.0f,  0.18f, 0.16f,  55.0f, 10.0f,  1.33f, 1.78f,  true,  0.35f, 0.25f, 0.20f },
    /* CLOSE_UP      */ {  4.5f, 1.5f,  0.18f, 0.08f,  55.0f,  5.0f,  1.20f, 1.60f,  true,  0.15f, 0.10f, 0.15f },
    /* PORTRAIT      */ {  5.0f, 1.5f,  0.20f, 0.15f,  45.0f,  5.0f,  0.56f, 0.75f,  true,  0.08f, 0.12f, 0.13f },
    /* BIRDS_EYE     */ {  5.0f, 2.0f,  1.20f, 0.20f,  50.0f,  8.0f,  1.00f, 1.33f,  true,  0.4f, 0.4f,   0.07f },
    /* LOW_ANGLE     */ {  3.5f, 1.0f,  0.03f, 0.02f,  50.0f,  8.0f,  1.50f, 2.00f,  true,  0.3f, 0.2f,   0.07f },
    /* CINEMATIC     */ {  8.0f, 3.0f,  0.12f, 0.10f,  90.0f, 12.0f,  2.00f, 2.39f,  true,  0.5f, 0.3f,   0.07f },
};

// painting canvas base area (world units²) — determines physical size
// on terrain before the right-skewed multiplier [0.85, 3.0]
inline constexpr float PAINTING_AREA[] = {
    30.0f,   // PANORAMIC:     large, cinematic canvas
    24.0f,   // ENVIRONMENTAL: medium canvas
    20.0f,   // MEDIUM:        moderate canvas
    20.0f,   // CLOSE_UP:      moderate canvas
    20.0f,   // PORTRAIT:      moderate (aspect makes it tall)
    18.0f,   // BIRDS_EYE:     moderate, near-square
    22.0f,   // LOW_ANGLE:     wide, dramatic
    28.0f,   // CINEMATIC:     large, ultrawide
};

// ═══ TUNING CONSOLE ══════════════════════════════════════════════
//
// SEAM[gallery:L2] this is a clean instance of pattern P3 (player
//   state vs mood state, explicit) — concerns separated into
//   named sub-structures rather than mixed in one big config.
//   Same shape as orbs.hpp's player-state vs mood-state split.
//
// SEAM[gallery:wall-art] WallArtConfig + WALL_ART are the indoor
//   half of gallery's :dual-role surface. They live here (not in
//   cartridge.hpp) because place_wall_paintings — the only
//   consumer — lives in this module. Same migration class as
//   ribbon active state (Q-closed-4).

struct PhotographerCaptureConfig {
    // Trigger: how far THE POINT travels between capture events
    // (the body's walk in pawn-host — identical; the flight in
    // free-fly — the travelogue)
    static constexpr float TRIGGER_DISTANCE_MEAN = 50.0f;
    static constexpr float TRIGGER_DISTANCE_SIGMA = 8.0f;
    static constexpr float TRIGGER_DISTANCE_FLOOR = 20.0f;

    // Burst: how many snapshots per trigger event
    static constexpr float BURST_WEIGHT_1 = 0.40f;
    static constexpr float BURST_WEIGHT_2 = 0.70f;
    static constexpr float BURST_WEIGHT_3 = 0.90f;
    static constexpr uint32_t BURST_MAX = 4;
    static constexpr uint32_t BURST_COOLDOWN_FRAMES = 12;

    // Clamps: hard floors on sampled camera parameters
    static constexpr float DISTANCE_FLOOR = 0.5f;
    static constexpr float ELEVATION_FLOOR = 0.005f;
    static constexpr float FOV_FLOOR = 15.0f;

    // Artistic override: wide-lens on any tier
    static constexpr float WIDE_LENS_CHANCE = 0.10f;
    static constexpr float WIDE_LENS_FOV_LO = 90.0f;
    static constexpr float WIDE_LENS_FOV_HI = 110.0f;
};

struct GalleryConfig {
    // Spawn probability per terrain archetype id — galleries favor calm
    // ground:                                     mountain varied basin  pool
    static constexpr float GALLERY_CHANCE_BY_ARCHETYPE[4] = { 0.12f, 0.24f, 0.70f, 0.85f };

    // Painting count per gallery: gaussian, median 5, σ 2
    // Max varies by archetype — basin gets the largest galleries
    static constexpr float PAINTINGS_MEAN = 5.0f;
    static constexpr float PAINTINGS_SIGMA = 2.0f;
    static constexpr uint32_t PAINTINGS_MIN = 2;
    static constexpr uint32_t PAINTINGS_MAX_BY_ARCHETYPE[4] = { 8, 10, 12, 12 };

    // Layout: paintings share a facing direction, staggered in two rows.
    // Odd paintings step forward, even step back — the pawn walks between.
    static constexpr float ROW_SPACING = 18.0f;       // horizontal distance between paintings
    static constexpr float ROW_DEPTH_MIN = 8.0f;      // minimum depth gap between rows
    static constexpr float ROW_DEPTH_RANGE = 4.0f;    // depth jitter on top of minimum
    static constexpr float ROW_LATERAL_JITTER = 2.0f;
    // The fan's extent reaches past the outermost painting's CENTRE to its
    // EDGE, and then a little further. Two authored numbers, split because
    // they answer different questions:
    //   PAINTING_HALF — half the canvas width the layout budgets per painting.
    //     ROW_SPACING/2 by construction: that IS the width the row reserves per
    //     slot, so a canvas wider than this already overlaps its neighbour and
    //     the spacing, not the radius, is what would need changing.
    //   FAN_MARGIN — authored clearance beyond that edge.
    // Together 12, against the old formula's undifferentiated trailing 15. The
    // allowance was never the defect (the count it multiplied was), but 15 put
    // a mean-count gallery at 54.8 wu against a legal 55 in the smallest room —
    // passing by 0.2 wu is luck, not a design.
    static constexpr float PAINTING_HALF = 9.0f;
    static constexpr float FAN_MARGIN    = 3.0f;

    static constexpr float MONO_TIER_CHANCE = 0.40f;  // 40% mono, 60% chronological

    static constexpr float GALLERY_SIZE_LO = 0.85f;  // smallest gallery mean
    static constexpr float GALLERY_SIZE_HI = 3.0f;   // largest gallery mean
    static constexpr float PAINTING_SIZE_SIGMA = 0.3f;   // per-painting jitter around gallery mean

    // Minimum snapshots before galleries start appearing
    static constexpr uint32_t MIN_POOL_SIZE = 3;

    // Minimum distance between gallery centers (world units). Clearance
    // beyond the median fan (~50 wu half-span); the footprint registry is
    // the actual overlap guard, this is the spacing taste.
    static constexpr float MIN_GALLERY_DISTANCE = 110.0f;

    // Painting slots the wall path may not take. The pool is shared and the
    // walls always win by arriving first: place_wall_paintings runs once at
    // mood entry and takes everything it wants in one pass, while
    // commit_gallery arrives afterwards over many frames at
    // SPAWN_BUDGET_PER_FRAME. This is a GEOMETRIC BOUND, not an expected
    // occupancy — sixteen galleries all rolling n=3 against a mean of 5, on a
    // near-perfect grid at exactly the exclusion distance, under a spawner
    // that places randomly with rejection. Realistic peak is 25-35. Derivation
    // in docs/audit/SALON_1_B4_REPORT.md.
    static constexpr uint32_t OUTDOOR_SLOT_RESERVE = 48;

    // ─── Content×Form Mixing ─────────────────────────────────
    //
    static constexpr float OUTDOOR_SNAPSHOT_ONLY = 0.80f;  // [0.00, 0.80)
    static constexpr float OUTDOOR_MIXED = 0.05f;  // [0.80, 0.85)

    // In mixed mode: per-painting chance of being the minority content
    static constexpr float OUTDOOR_MIX_AUTHORED_CHANCE = 0.35f;  // chance each outdoor painting is authored

    // Photographer pacing by archetype
    static constexpr float PHOTO_PACE_BY_ARCHETYPE[4] = { 0.7f, 0.8f, 1.5f, 1.5f };

    // Gallery center jitter (fraction of Dim::PATCH_EXTENT)
    static constexpr float POSITION_JITTER = 0.30f;
};

// Site content type (outdoor gallery)
struct GallerySiteType {
    static constexpr uint32_t SNAPSHOT_ONLY = 0;
    static constexpr uint32_t MIXED = 1;
    static constexpr uint32_t AUTHORED_ONLY = 2;
};

// ── Wall art configuration (indoor) ──────────────────────────────

struct WallArtScaleBucket {
    float height_lo;     // uniform [lo, hi] sample for painting height
    float height_hi;
    float weight;        // selection weight (the three weights must sum to 1)
    float y_offset_lo;   // uniform [lo, hi] additive offset from base_py
    float y_offset_hi;
};

struct WallArtConfig {
    // ─── Wall participation (cumulative thresholds, 0..1) ───
    // roll < t1 → 1 wall, < t2 → 2, < t3 → 3, residual → 4 walls
    float wall_count_t1;
    float wall_count_t2;
    float wall_count_t3;

    // ─── Per-wall row density: ONE dial, read against the wall ─
    // The count is not rolled. A uniform [1,5] gave the same short row on a
    // 126 wu wall and a 426 wu one, so the biggest rooms read as the emptiest
    // — which is the whole defect PROPORTION exists to remove. Divide the
    // wall instead: mean piece ~12 wu plus a ~10 wu share of gap and air.
    float target_spacing;       // world units of wall per piece
    // And a ceiling on it, because proportional is not the same as unbounded:
    // at radius 4 the raw division asks for 19 a wall, 76 a room, against a
    // library of 64 distinct images. The cap is what makes the layer budget
    // provable — see the two asserts below.
    uint32_t per_wall_cap;      // hard ceiling on pieces per wall

    // ─── Wall surface geometry ──────────────────────────────
    float corner_margin;        // distance from wall corners
    float painting_gap;         // gap between adjacent painting edges
    float paint_y_frac;         // base center as fraction of ceiling
    // The bottom edge is clamped from BOTH sides. max_ keeps a piece from
    // riding up the wall; min_ keeps it off the floor. The pair is one
    // clamp with two ends, not two mechanisms.
    float max_bottom_height;    // hard upper clamp on bottom edge (m)
    float min_bottom_height;    // hard lower clamp on bottom edge (m)
    // R4's other side: the matting above. Measured against wall_height, the
    // top of the vertical wall — NOT the ceiling, which on VAULT is the crown.
    float top_margin;           // hard clamp on the top edge, below wall_height (m)

    // ─── Size buckets (intimate / standard / statement) ─────
    WallArtScaleBucket intimate;
    WallArtScaleBucket standard;
    WallArtScaleBucket statement;

    // ─── Indoor content mix (snapshot vs authored) ──────────
    float snapshot_only_share;
    float mixed_share;
    // In mixed mode: per-painting chance of being a snapshot.
    float mix_snapshot_chance;
};

inline constexpr WallArtConfig WALL_ART = {
    // wall_count cumulative thresholds:
    //   0.5% → 1 wall, 0.25% → 2, 27% → 3, residual ~72% → 4
    /* wall_count_t1 */ 0.005f,
    /* wall_count_t2 */ 0.0075f,
    /* wall_count_t3 */ 0.2775f,

    // per-wall row density — the division yields 5 / 10 / 14 / 19 at radius
    // 1..4, and the cap flattens the top two to 7
    /* target_spacing */ 22.0f,
    /* per_wall_cap   */ 7,

    // wall surface geometry
    /* corner_margin     */ 12.0f,
    /* painting_gap      */ 6.0f,
    /* paint_y_frac      */ 0.45f,
    /* max_bottom_height */ 4.0f,   // bottom no higher than 4 m above floor
    // Floor side. MUST stay above the frame border (FrameStyle::width,
    // 0.45 at both FRAME_AUTHORED and FRAME_SNAPSHOT) or the frame's lower
    // edge dips below the wall base even when the canvas clears it.
    /* min_bottom_height */ 1.0f,   // bottom no lower than 1 m above floor
    // Inert on today's population: the reachable band tops at 16.45 against a
    // wall_height of 20.0, so anything below 3.55 never fires. It is R4's
    // ceiling side kept honest against the buckets, and the assert below is
    // what proves the two clamps cannot cross.
    /* top_margin        */ 2.0f,   // top no closer than 2 m to the wall top

    //                 height_lo, height_hi, weight, y_offset_lo, y_offset_hi
    /* intimate  */  {  6.0f,    11.0f,    0.25f,   0.0f,        2.0f },
    /* standard  */  {  8.0f,    12.0f,    0.50f,  -1.5f,        1.5f },
    /* statement */  { 10.0f,    14.0f,    0.25f,  -3.5f,       -1.5f },

    // content mix
    /* snapshot_only_share */ 0.15f,
    /* mixed_share         */ 0.05f,
    /* mix_snapshot_chance */ 0.40f,
};

// THE ROW'S UPPER BOUND, DERIVED RATHER THAN GUESSED. The count is now a
// function of the wall, so its maximum is a function of the biggest wall —
// and every input is constexpr, so the plan array and the slot budget can
// both ride the real number instead of a padded literal.
//
//   wall_span   = (2 * finite_radius + 1) * PATCH_EXTENT
//   usable_span = max(wall_span - 2*corner_margin, wall_span * 0.3)
//   count       = usable_span / target_spacing
//
// Monotone in finite_radius, so the bound is the value at finite_radius_max.
// Dim::PATCH_EXTENT comes through this file's own include of state.hpp;
// MOOD_TABLE comes by COHORT ORDER, the same reach the R4 assert below uses.
inline constexpr uint32_t INDOOR_RADIUS_MAX =
    MOOD_TABLE[MOOD_INDOOR_FLAT].finite_radius_max
        > MOOD_TABLE[MOOD_INDOOR_VAULT].finite_radius_max
    ? MOOD_TABLE[MOOD_INDOOR_FLAT].finite_radius_max
    : MOOD_TABLE[MOOD_INDOOR_VAULT].finite_radius_max;

inline constexpr float INDOOR_MAX_WALL_SPAN =
    (2.0f * (float)INDOOR_RADIUS_MAX + 1.0f) * Dim::PATCH_EXTENT;

inline constexpr float INDOOR_MAX_USABLE_SPAN =
    (INDOOR_MAX_WALL_SPAN - 2.0f * WALL_ART.corner_margin) > (INDOOR_MAX_WALL_SPAN * 0.3f)
    ? (INDOOR_MAX_WALL_SPAN - 2.0f * WALL_ART.corner_margin)
    : (INDOOR_MAX_WALL_SPAN * 0.3f);

// The cap and the division both bound the row, so the array bound is the
// smaller. Keeping the min HERE rather than only at the call site means plan[]
// shrinks with the cap instead of carrying storage for a row that can no
// longer happen.
inline constexpr uint32_t INDOOR_MAX_ROW_DERIVED =
    (uint32_t)(INDOOR_MAX_USABLE_SPAN / WALL_ART.target_spacing);   // 19 today

inline constexpr uint32_t INDOOR_MAX_ROW_COUNT =
    WALL_ART.per_wall_cap < INDOOR_MAX_ROW_DERIVED
    ? WALL_ART.per_wall_cap : INDOOR_MAX_ROW_DERIVED;               // 7 today

// THE SLOT BUDGET, PROVED RATHER THAN CLAIMED. The dials on this panel feed
// the sum and they will be turned; a number derived by hand in a handoff is a
// claim, the same number checked by the compiler is a fact — the
// sizeof(GPUPaintingSlot) == 128 handshake, one level up.
//
// It lives HERE, not beside Dim::PAINTING_MAX_SLOTS, because this is the
// first point in the translation unit where all three facts coexist:
// bodies/gallery.hpp includes realization/state.hpp and never the reverse,
// so WALL_ART and GalleryConfig are not reachable from the declaration
// site. Beside the dials is also where a dial-turner will see it.
static_assert(Dim::PAINTING_MAX_SLOTS >=
      4u * INDOOR_MAX_ROW_COUNT
    + GalleryConfig::OUTDOOR_SLOT_RESERVE,
    "slot budget: the four walls' rows and the outdoor reserve must fit");

// THE TWO SUPPLY BUDGETS, which the slot budget above never covered. It proves
// SLOTS fit — 288 of them — and says nothing about the two arrays that
// actually ran out: a frame needs an exhibition layer to draw from and a
// staging record to draw. Four full walls need both, and neither was checked
// until the day they ran out on wall two.
//
// These bound against the CAP, not against the library. PROPORTION's version
// asserted `2 * STAGING_LAYERS <= EXHIBITION_LAYERS` — with an uncapped row
// frames were bounded only by distinct records, so every record had to be able
// to hold a layer at once. That premise expired with the cap: staging is now a
// library to draw FROM, not a set that must be simultaneously resident.
static_assert(Dim::EXHIBITION_LAYERS >= 4u * WALL_ART.per_wall_cap,
    "layer supply: four full walls must each own a distinct exhibition layer");
static_assert(Dim::STAGING_LAYERS >= 4u * WALL_ART.per_wall_cap,
    "content supply: a room must be fillable with distinct images");

// 40 >= 28 and 32 >= 28. The outdoor galleries share what is left over — 12
// layers at today's dials — and their row ends on its own when it runs out.
// That is a SHARE, not a reservation, and it is deliberately not asserted:
// writing it down would turn a soft degradation into a build failure the first
// time either dial moved, and outdoor is the one that should give way.

// R4's TWO CLAMPS MUST NOT CROSS. min_bottom_height pushes a piece up,
// top_margin pushes it down, and top wins because it is applied last — so a
// top_margin large enough to drive the tallest bucket back through the floor
// guard would do it silently. This is a dial Jean turns at the visual gate,
// which is exactly when a silent conflict would be blamed on the hang.
//
// MOOD_TABLE reaches here by COHORT ORDER, not by this file's includes:
// cartridge.hpp takes contracts/spine_state.hpp (:60) before bodies/gallery.hpp
// (:78), and gallery.hpp has exactly one includer. Reordering those breaks this
// assert loudly, which is the right failure.
static_assert(WALL_ART.min_bottom_height
            + WALL_ART.statement.height_hi
            + WALL_ART.top_margin
              <= MOOD_TABLE[MOOD_INDOOR_FLAT].wall_height,
    "R4: floor margin + tallest bucket + top margin must fit the FLAT wall");

// ── Property index registries ────────────────────────────────────

// Outdoor — patch-level seed ───────────────────────────────────
struct GalleryProp {
    static constexpr uint32_t SPAWN_ROLL          = 500u;  // gallery presence gate
    static constexpr uint32_t PAINTING_COUNT_R1   = 501u;  // sum-of-3-uniforms (Gaussian approx)
    static constexpr uint32_t PAINTING_COUNT_R2   = 502u;
    static constexpr uint32_t PAINTING_COUNT_R3   = 503u;
    static constexpr uint32_t FACING_ANGLE        = 504u;
    static constexpr uint32_t CENTER_OFFSET       = 505u;
    static constexpr uint32_t CENTER_ANGLE        = 506u;
    static constexpr uint32_t PER_PAINTING_BASE   = 510u;  // p_seed = hash(seed, BASE + p*STRIDE)
    static constexpr uint32_t PER_PAINTING_STRIDE = 7u;
    static constexpr uint32_t MONO_TIER_ROLL      = 520u;
    static constexpr uint32_t FAVORITE_TIER_PICK  = 521u;
    static constexpr uint32_t SIZE_JITTER         = 530u;
    static constexpr uint32_t SITE_TYPE_ROLL      = 540u;
};

// Outdoor — per-painting (p_seed = hash(seed, GalleryProp::PER_PAINTING_BASE + p*STRIDE))
struct GalleryPaintingProp {
    static constexpr uint32_t LATERAL_JITTER  = 0u;
    static constexpr uint32_t DEPTH_JITTER    = 1u;
    static constexpr uint32_t SIZE_JITTER_A   = 3u;  // sum-of-3 component (a)
    static constexpr uint32_t GEOMETRY_SEED   = 4u;
    static constexpr uint32_t SIZE_JITTER_B   = 5u;  // sum-of-3 component (b)
    static constexpr uint32_t SIZE_JITTER_C   = 6u;  // sum-of-3 component (c)
    static constexpr uint32_t MIX_AUTHOR_ROLL = 8u;  // chance this painting is authored in MIXED gallery
    static constexpr uint32_t AUTH_STG_PICK   = 9u;  // unused — see Q30 in rollout report
};

// Indoor — site_seed structure
struct WallArtProp {
    // site_seed = hash(c->world_state_.active_seed, SITE_SEED_OFFSET)
    static constexpr uint32_t SITE_SEED_OFFSET    = 5500u;

    // off site_seed:
    static constexpr uint32_t SITE_TYPE_ROLL      = 0u;
    static constexpr uint32_t WALL_COUNT_ROLL     = 1u;
    static constexpr uint32_t WALL_SHUFFLE_BASE   = 2u;   // shuffle index = WALL_SHUFFLE_BASE + i
    // 6 (the R5 density roll) and 1000/3 (the fill-frame stride) are VACANT —
    // PROPORTION deleted the second system that used them.
    static constexpr uint32_t PER_WALL_BASE       = 10u;  // w_seed = hash(site_seed, BASE + w*STRIDE)
    static constexpr uint32_t PER_WALL_STRIDE     = 20u;

    // off w_seed:
    // 0 (the per-wall count roll) is VACANT — PROPORTION reads the count off
    // the wall's own length instead, so there is nothing left to roll.
    static constexpr uint32_t PER_PAINTING_BASE       = 100u;  // p_seed = hash(w_seed, BASE + p*STRIDE)
    static constexpr uint32_t PER_PAINTING_STRIDE     = 10u;
};

// Indoor — per-painting (p_seed = hash(w_seed, WallArtProp::PER_PAINTING_BASE + p*STRIDE))
struct WallPaintingProp {
    static constexpr uint32_t Y_OFFSET_JITTER   = 1u;
    static constexpr uint32_t MIX_SNAPSHOT_ROLL = 2u;  // chance this painting is snapshot in MIXED site
    static constexpr uint32_t HEIGHT_JITTER     = 3u;
    static constexpr uint32_t AUTH_STG_PICK     = 4u;  // unused — see Q30 in rollout report
    // ASPECT_ESTIMATE = 5u died with E-a's reorder: widths are planned from the
    // REAL aspect now, so there is nothing left to estimate. 5 stays vacant
    // rather than being reused — these are hash props, and a later prop taking
    // the index would be free to, but leaving the gap costs nothing and keeps
    // the numbering legible against the campaign history.
    static constexpr uint32_t SCALE_ROLL        = 7u;
};

// ═══ STATE: PHOTOGRAPHER ═════════════════════════════════════════

struct PhotographerState {
    float cumulative_distance = 0.0f;
    float next_threshold = PhotographerCaptureConfig::TRIGGER_DISTANCE_MEAN;
    uint32_t pending_shots = 0;
    float prev_point_x = 0.0f;
    float prev_point_z = 0.0f;
    bool initialized = false;
    uint32_t frame_cooldown = 0;
    std::mt19937 rng{ 7742u };

    float uniform(float lo, float hi) {
        std::uniform_real_distribution<float> dist(lo, hi);
        return dist(rng);
    }
    float gaussian(float mean, float sigma) {
        std::normal_distribution<float> dist(mean, sigma);
        return dist(rng);
    }
    // how many snapshots per burst (weighted: 1 most common)
    uint32_t sample_shot_count() {
        float roll = uniform(0.0f, 1.0f);
        if (roll < PhotographerCaptureConfig::BURST_WEIGHT_1) return 1;
        if (roll < PhotographerCaptureConfig::BURST_WEIGHT_2) return 2;
        if (roll < PhotographerCaptureConfig::BURST_WEIGHT_3) return 3;
        return PhotographerCaptureConfig::BURST_MAX;
    }
    // tier selection — reads weights from SHOT_PARAMS matrix
    ShotType sample_shot_type() {
        constexpr uint32_t n = static_cast<uint32_t>(ShotType::COUNT);
        float w[n];
        for (uint32_t t = 0; t < n; t++) w[t] = SHOT_PARAMS[t].weight;
        return static_cast<ShotType>(select_weighted(uniform(0.0f, 1.0f), w, n));
    }
};

// ═══ STATE SUB-STRUCTS ═══════════════════════════════════════════

// ── Snapshot Staging (circular buffer, 16 layers) ──
struct SnapshotStagingRecord {
    float aspect_ratio = 1.0f;
    uint32_t shot_type = 0;
    bool valid = false;
    bool consumed = false;    // promoted to exhibition, no longer a candidate
    float capture_x = 0.0f;
    float capture_z = 0.0f;
    float capture_distance = 0.0f;
    uint32_t capture_frame = 0;
};

// ── Authored Staging (circular buffer, 16 layers) ──
struct AuthoredStagingRecord {
    uint32_t disk_index = UINT32_MAX;
    float aspect_ratio = 1.0f;
    float uv_scale_x = 1.0f;
    float uv_scale_y = 1.0f;
    bool valid = false;
    bool consumed = false;
#ifdef __EMSCRIPTEN__
    // EXHIBIT_0 — A REQUEST IS NOT A PICTURE. On the native twin a load
    // either fills this record or fails, inside one call; over the
    // network the two are separated by a round trip. `pending` names
    // that gap: the slot is spoken for (disk_index is already set, so
    // the rotation cursor will not hand the same painting to a second
    // slot) and it is NOT valid, so no gallery can pick it. Cleared on
    // arrival AND on failure — a slot that never clears is a slot the
    // rotation can never reuse.
    bool pending = false;
#endif
};

// Pending texture promotions (staging → exhibition, executed in render)
struct PendingPromotion {
    bool is_snapshot;       // true = snapshot staging, false = authored staging
    uint32_t staging_layer;
    uint32_t exhibition_layer;
};
// ONE FACT, NOT TWO. Every promotion accompanies exactly one slot fill —
// each of the four queue_promotion calls sits beside a find_free_exhibition_layer
// and a slot write — so promotions per frame can never exceed total slots.
// Spelled twice and agreeing by coincidence until now; the drop at
// queue_promotion is silent, so a divergence would have produced frames
// pointing at exhibition layers nothing ever wrote.
inline constexpr uint32_t MAX_PROMOTIONS_PER_FRAME = Dim::PAINTING_MAX_SLOTS;

// Active gallery centers (for minimum distance enforcement)
struct GalleryCenter {
    float x = 0.0f, z = 0.0f;
    int32_t patch_gx = INT32_MAX, patch_gz = INT32_MAX;
    int32_t host_gx = 0, host_gz = 0;   // host patch (for entity_refs eviction)
    bool active = false;
};
inline constexpr uint32_t MAX_GALLERIES = 48;

struct PendingSnapshot {
    bool active = false;
    uint32_t target_slot = 0;
    uint32_t target_layer = 0;
};

// ═══ SPAWN PAYLOADS — AT THE CONTRACT HOME ═══════════════════════
//
// The gallery Selection/Placement DTOs live in entity_types.hpp,
// beside the EntityQueueEntry / PlacementEntry unions that are their
// reason to exist: a DTO that exists to cross a boundary belongs to
// the boundary's contract, not to either side.

// ═══ GALLERY MODULE STATE ════════════════════════════════════════

struct GalleryState {
    PhotographerState photographer;

    float    total_walk_distance = 0.0f;
    uint32_t frame_counter = 0;

    SnapshotStagingRecord snapshot_staging[Dim::STAGING_LAYERS]{};
    uint32_t              snapshot_write_cursor = 0;
    uint32_t              snapshot_count = 0;

    // Staging layers claimed by galleries that have been PLACED but not yet
    // COMMITTED. Up to SPAWN_BUDGET_PER_FRAME galleries can be placed before
    // any of them commits, and without this they would each reserve against
    // the same unclaimed pool. Incremented at place by reserved_count,
    // decremented at commit by the same value — every placement commits
    // exactly once (commit_entity_queue drains placementResults_ whole), so
    // it is balanced by construction and cannot drift.
    uint32_t              staging_reserved = 0;

    AuthoredStagingRecord authored_staging[Dim::STAGING_LAYERS]{};
    uint32_t              authored_write_cursor = 0;
    uint32_t              authored_disk_cursor = 0;     // walks through authored_disk_manifest
    uint32_t              authored_staged_count = 0;
    bool                  authored_textures_loaded = false;
    std::vector<std::string> authored_disk_manifest;    // scanned lazily on first load, sorted numerically

#ifdef __EMSCRIPTEN__
    // ── EXHIBIT_0: the web twin's loading gap, named ────────────────
    // The native twin has no state here because its loads are calls
    // that return with the picture. The web twin's do not, so the two
    // facts that only exist BETWEEN a request and its answer live
    // here: how many are out, and which ones are still waiting for a
    // free lane.
    struct AuthoredFetchRequest {
        uint32_t staging_layer;
        uint32_t disk_index;
        std::string url;
    };
    std::vector<AuthoredFetchRequest> authored_fetch_queue;
    uint32_t authored_fetch_inflight = 0;
    // The manifest is requested once per session, at the earliest
    // instant a GalleryState exists (the cartridge constructor).
    bool authored_manifest_requested = false;
    // "No paintings folder found" is a true sentence every time it is
    // asked before the manifest lands; it is only worth SAYING once.
    bool authored_absence_logged = false;
#endif

    // The occupancy array is the whole record. A companion count used to
    // ride beside it, incremented at every claim and decremented at every
    // free — and read by nothing, not even a log. Deleted rather than
    // reserved: if a later stage needs a population figure it can name its
    // reader when it introduces one.
    bool     exhibition_occupied[Dim::EXHIBITION_LAYERS]{};

    PendingPromotion pending_promotions[MAX_PROMOTIONS_PER_FRAME]{};
    uint32_t         pending_promotion_count = 0;

    GPUPaintingSlot painting_slots[Dim::PAINTING_MAX_SLOTS]{};
    uint32_t        active_painting_count = 0;
    uint32_t        wall_frame_count = 0;
    // One past the highest ACTIVE slot index — what the two painting draws
    // are sized from, instead of Dim::PAINTING_MAX_SLOTS. Both draws walk
    // slot indices and cull in the shader, so the constant made every frame
    // pay for the ceiling rather than for the hang; B3 multiplied that by 9.
    // Raised where a slot is claimed, recomputed exactly where slots are
    // released (those sites already scan the whole array), zeroed at
    // teardown. Under-drawing is safe by construction — gallery_frame_vs and
    // wall_painting_vs both guard their index (B1).
    uint32_t        slot_high_water = 0;

    GalleryCenter gallery_centers[MAX_GALLERIES]{};

    PendingSnapshot pending_snapshot;
};

// ═══ MODULE FUNCTIONS — DECLARATIONS ═════════════════════════════

// Per-frame
void update_photographer(GalleryState& gs, GalleryDeps* c, wgpu::Queue& queue);
void render_snapshot_pass(GalleryState& gs, GalleryDeps* c, wgpu::CommandEncoder& encoder);
// Outdoor lifecycle (three-phase)
bool select_gallery_for_patch(GalleryState& gs, MachineCtx* c,
    int32_t gx, int32_t gz, GallerySelection& sel);
bool place_gallery_from_selection(MachineCtx* c,
    const GallerySelection& sel, GalleryPlacement& plan);
void commit_gallery(GalleryState& gs, MachineCtx* c,
    const GalleryPlacement& plan,
    int32_t trigger_gx, int32_t trigger_gz, wgpu::Queue& queue);
void evict_paintings_for_patch(GalleryState& gs, MachineCtx* c,
    int32_t gx, int32_t gz, wgpu::Queue& queue);
// The evictor — MachineCtx-shaped
// to match the FAMILY_DISPATCH evict slot (table in cartridge.hpp, post-class)
void evict_gallery(MachineCtx* self, uint32_t slot, wgpu::Queue& queue);
// Dispatch funnels (table-shaped; the FAMILY_DISPATCH rows point here)
bool dispatch_select_gallery(MachineCtx* self, int32_t gx, int32_t gz, EntityQueueEntry& e);
bool dispatch_place_gallery(MachineCtx* self, EntityQueueEntry& e, PlacementEntry& pe);
void dispatch_commit_gallery(MachineCtx* self, PlacementEntry& pe, wgpu::Queue& queue);
// Indoor entry (called by mood.hpp::apply_mood)
void place_wall_paintings(GalleryState& gs, GalleryDeps* c, wgpu::Queue& queue,
    float bmin, float bmax, float wall_height);
void clear_wall_paintings(GalleryState& gs, GalleryDeps* c, wgpu::Queue& queue);
// Authored image loading
void load_authored_textures(GalleryState& gs, GPUState& gpu, wgpu::Queue& queue);  // GPUState& deps-form: context-agnostic dual-entry door
void rotate_authored_staging(GalleryState& gs, GalleryDeps* c, wgpu::Queue& queue);
void teardown_gallery(GalleryState& gs, GalleryDeps* c, wgpu::Queue& queue);
void drain_gallery_promotions(GalleryState& gs, GalleryDeps* c, wgpu::CommandEncoder& encoder);

// ═══ IMPL:
// bodies deref gallery_state_(own) + world/tile/ribbon/player/mood via
// GalleryDeps + sun/clear root arrays; rows via MachineCtx. COHORT: after
// ribbon (RibbonState) + renderer + machine + tile/patch. stb_image/fs local.

// ═══ STATE-LOCAL HELPERS (impl-only, take GalleryState&) ═════════

inline uint32_t find_free_exhibition_layer(const GalleryState& gs) {
    for (uint32_t i = 0; i < Dim::EXHIBITION_LAYERS; i++)
        if (!gs.exhibition_occupied[i]) return i;
    return UINT32_MAX;
}

inline void queue_promotion(GalleryState& gs,
    bool is_snapshot, uint32_t staging_layer, uint32_t exhibition_layer) {
    if (gs.pending_promotion_count < MAX_PROMOTIONS_PER_FRAME) {
        gs.pending_promotions[gs.pending_promotion_count++] = {
            is_snapshot, staging_layer, exhibition_layer
        };
    }
}

// ── Slot lookup helpers ──

// THE FLOOR LIVES IN THE ALLOCATOR. One handout point, one place to hold a
// reserve — a counter beside it would be a second fact about the same thing.
// `floor` is how many slots must remain free AFTER this one is taken;
// returns UINT32_MAX rather than allocate past it, which both callers already
// handle as ordinary exhaustion.
//
// Single pass: the free tally is counted while scanning for the first free
// index, so this stays the O(n) it already was. Placement-event path, not a
// frame path.
inline uint32_t find_free_painting_slot(const GalleryState& gs, uint32_t floor) {
    uint32_t first_free = UINT32_MAX;
    uint32_t free_count = 0;
    for (uint32_t i = 0; i < Dim::PAINTING_MAX_SLOTS; i++) {
        if (gs.painting_slots[i].is_active == 0) {
            if (first_free == UINT32_MAX) first_free = i;
            free_count++;
        }
    }
    if (first_free == UINT32_MAX) return UINT32_MAX;   // nothing free at all
    return free_count > floor ? first_free : UINT32_MAX;
}

// Exact, not a running high-water: called from the release sites, which
// already walk the array, so it costs nothing they were not paying.
inline void recompute_slot_high_water(GalleryState& gs) {
    uint32_t mark = 0;
    for (uint32_t i = 0; i < Dim::PAINTING_MAX_SLOTS; i++)
        if (gs.painting_slots[i].is_active != 0) mark = i + 1;
    gs.slot_high_water = mark;
}

// ── Impl-internal forward declarations ───────────────────────────
// Used before their definitions (which keep their original section
// homes below). Impl-only — not part of the header surface.
inline void capture_snapshot(GalleryState& gs, GalleryDeps* c, float pawn_x, float pawn_z, wgpu::Queue& queue);
inline uint32_t count_unused_authored(const GalleryState& gs, const bool usedAuthored[]);
inline uint32_t pick_authored_staging(GalleryState& gs, uint32_t seed, uint32_t prop);

// ═══ FRAME STYLE PRESETS + SLOT FILL ═════════════════════════════

// ── Frame style presets ──
struct FrameStyle {
    float depth, width, recess;
    float color[3];
};
// Authored: thick dark wood (museum frame)
inline constexpr FrameStyle FRAME_AUTHORED = { 0.30f, 0.45f, 0.09f, { 0.25f, 0.15f, 0.08f } };
// Snapshot on wall: same museum frame (content is different, ceremony is the same)
inline constexpr FrameStyle FRAME_SNAPSHOT = { 0.30f, 0.45f, 0.09f, { 0.25f, 0.15f, 0.08f } };

// ── Slot fill helper ──

inline void fill_slot_wall_frame(
    GPUPaintingSlot& s,
    float x, float y, float z,
    float nx, float ny, float nz,
    float aspect_ratio, float base_height,
    uint32_t layer, uint32_t content,
    float uv_sx, float uv_sy,
    const FrameStyle& frame,
    int32_t gx, int32_t gz
) {
    s = {};
    s.position[0] = x; s.position[1] = y; s.position[2] = z;
    s.forward[0] = nx; s.forward[1] = ny; s.forward[2] = nz;
    s.up[0] = 0.0f; s.up[1] = 1.0f; s.up[2] = 0.0f;
    s.form_type = FormType::WALL_FRAME;
    s.is_active = 1;
    s.scale_x = base_height * aspect_ratio;
    s.scale_y = base_height;
    s.texture_layer = layer;
    s.content_source = content;
    s.uv_scale_x = uv_sx;
    s.uv_scale_y = uv_sy;
    s.frame_depth = frame.depth;
    s.frame_width = frame.width;
    s.canvas_recess = frame.recess;
    s.frame_color[0] = frame.color[0];
    s.frame_color[1] = frame.color[1];
    s.frame_color[2] = frame.color[2];
    s.patch_gx = gx; s.patch_gz = gz;
}

// ═══ PHOTOGRAPHER LIFECYCLE ══════════════════════════════════════

inline void update_photographer(GalleryState& gs, GalleryDeps* c, wgpu::Queue& queue) {
    float px = c->point_.x;
    float pz = c->point_.z;

    if (!gs.photographer.initialized) {
        gs.photographer.prev_point_x = px;
        gs.photographer.prev_point_z = pz;
        gs.photographer.initialized = true;
        return;
    }

    float dx = px - gs.photographer.prev_point_x;
    float dz = pz - gs.photographer.prev_point_z;
    float step = std::sqrt(dx * dx + dz * dz);
    gs.photographer.prev_point_x = px;
    gs.photographer.prev_point_z = pz;
    if (step > 5.0f) return;

    gs.photographer.cumulative_distance += step;
    gs.total_walk_distance += step;
    gs.frame_counter++;
    if (gs.photographer.frame_cooldown > 0) gs.photographer.frame_cooldown--;

    if (gs.photographer.pending_shots > 0 && gs.photographer.frame_cooldown == 0) {
        capture_snapshot(gs, c, px, pz, queue);
        gs.photographer.pending_shots--;
        gs.photographer.frame_cooldown = PhotographerCaptureConfig::BURST_COOLDOWN_FRAMES;
        return;
    }

    if (gs.photographer.cumulative_distance >= gs.photographer.next_threshold) {
        gs.photographer.cumulative_distance = 0.0f;

        // Pace modulation: less active in sand/basin, more in colored terrain
        float pace = 1.0f;
        int32_t tx = (int32_t)std::floor(px / Dim::PATCH_EXTENT);
        int32_t tz = (int32_t)std::floor(pz / Dim::PATCH_EXTENT);
        uint32_t pace_archetype = 0;
        if (tile_archetype(c->tile_world_state_, tx, tz, pace_archetype)) {  // F4: miss keeps pace 1.0
            pace = GalleryConfig::PHOTO_PACE_BY_ARCHETYPE[pace_archetype];
        }

        gs.photographer.next_threshold = std::max(
            PhotographerCaptureConfig::TRIGGER_DISTANCE_FLOOR,
            gs.photographer.gaussian(
                PhotographerCaptureConfig::TRIGGER_DISTANCE_MEAN * pace,
                PhotographerCaptureConfig::TRIGGER_DISTANCE_SIGMA));
        gs.photographer.pending_shots = gs.photographer.sample_shot_count();
        gs.photographer.frame_cooldown = 0;
    }
}

inline void capture_snapshot(GalleryState& gs, GalleryDeps* c, float pawn_x, float pawn_z, wgpu::Queue& queue) {
    ShotType shot = gs.photographer.sample_shot_type();
    const auto& params = SHOT_PARAMS[static_cast<uint32_t>(shot)];

    float aspect_ratio = gs.photographer.uniform(params.aspect_lo, params.aspect_hi);
    float azimuth = gs.photographer.uniform(0.0f, 6.283185f);
    float dist = std::max(PhotographerCaptureConfig::DISTANCE_FLOOR,
        gs.photographer.gaussian(params.distance_mean, params.distance_sigma));
    float elev = std::max(PhotographerCaptureConfig::ELEVATION_FLOOR,
        gs.photographer.gaussian(params.elevation_mean, params.elevation_sigma));
    float fov_deg = std::max(PhotographerCaptureConfig::FOV_FLOOR,
        gs.photographer.gaussian(params.fov_degrees, params.fov_sigma));

    if (gs.photographer.uniform(0.0f, 1.0f) < PhotographerCaptureConfig::WIDE_LENS_CHANCE) {
        fov_deg = gs.photographer.uniform(PhotographerCaptureConfig::WIDE_LENS_FOV_LO,
            PhotographerCaptureConfig::WIDE_LENS_FOV_HI);
    }

    float fov_rad = fov_deg * 3.14159f / 180.0f;
    float offset_x = gs.photographer.uniform(-params.offset_x_range, params.offset_x_range);
    float offset_y = gs.photographer.uniform(-params.offset_y_range, params.offset_y_range);

    // Record in snapshot staging — cursor wraps freely, unconditional overwrite
    uint32_t layer = gs.snapshot_write_cursor;
    auto& rec = gs.snapshot_staging[layer];
    rec.aspect_ratio = aspect_ratio;
    rec.shot_type = static_cast<uint32_t>(shot);
    rec.valid = true;
    rec.consumed = false;  // fresh capture, available for exhibition
    rec.capture_x = pawn_x;
    rec.capture_z = pawn_z;
    rec.capture_distance = gs.total_walk_distance;
    rec.capture_frame = gs.frame_counter;
    gs.snapshot_write_cursor = (layer + 1) % Dim::STAGING_LAYERS;
    if (gs.snapshot_count < Dim::STAGING_LAYERS) gs.snapshot_count++;

    // Upload photographer config for GPU compute
    GPUPhotographerConfig cfg{};
    float slen = std::sqrt(c->sunDirection_[0] * c->sunDirection_[0] + c->sunDirection_[1] * c->sunDirection_[1] + c->sunDirection_[2] * c->sunDirection_[2]);
    cfg.sun_direction[0] = c->sunDirection_[0] / slen;
    cfg.sun_direction[1] = c->sunDirection_[1] / slen;
    cfg.sun_direction[2] = c->sunDirection_[2] / slen;
    cfg.azimuth = azimuth;
    cfg.elevation = elev;
    cfg.distance = dist;
    cfg.fov_rad = fov_rad;
    cfg.aspect_ratio = aspect_ratio;
    cfg.patch_count = c->world_state_.all_patch_count;
    cfg.frame_offset_x = offset_x;
    cfg.frame_offset_y = offset_y;
    cfg._pad0 = 0.0f;
    c->gpuState_.upload_photographer_config(queue, cfg);

    gs.pending_snapshot.active = true;
    gs.pending_snapshot.target_slot = UINT32_MAX;
    gs.pending_snapshot.target_layer = layer;

    const char* shot_names[] = {
        "Panoramic", "Environmental", "Medium", "Close-up",
        "Portrait", "Bird's Eye", "Low Angle", "Cinematic"
    };
    // Autonomous stdout — exhibition-guard candidate, still open.
    std::cout << "[Photographer] Capture -> layer " << layer
        << " (" << shot_names[static_cast<uint32_t>(shot)] << ")"
        << " aspect=" << aspect_ratio
        << " pool=" << gs.snapshot_count << "/" << Dim::STAGING_LAYERS << "\n";
}

// ═══ GALLERY SITES (outdoor — three-phase) ═══════════════════════

// ── select_gallery_for_patch ──

inline bool select_gallery_for_patch(GalleryState& gs, MachineCtx* c, int32_t gx, int32_t gz, GallerySelection& sel) {
    // Content gate: minimum snapshot pool
    if (gs.snapshot_count < GalleryConfig::MIN_POOL_SIZE) return false;

    // THE COMPOSITION LAW: base authority is ARCHETYPE-INDEXED —
    // resolved first, passed as data. Mood = explicit veto; proximity
    // OFF (gallery's affinity row is zero); clamp NONE — the absent
    // clamp is CARRIED AS DATA per the sub-ruling (behavior-identical;
    // ruling a clamp IN is a separate taste gate).
    uint32_t archetype = 1;
    tile_archetype(c->tile_world_state_, gx, gz, archetype);   // F4: miss keeps 1
    auto composed = compose_spawn_chance(c, gx, gz, PopFamily::GALLERY,
        GalleryConfig::GALLERY_CHANCE_BY_ARCHETYPE[archetype],
        mood_mult_for(PopFamily::GALLERY),
        /*use_proximity=*/false, /*veto_on_zero_mood=*/true,
        SpawnClamp::NONE);
    if (composed.vetoed) return false;

    // Idempotency: skip if paintings already exist at this patch
    for (uint32_t i = 0; i < Dim::PAINTING_MAX_SLOTS; i++) {
        if (gs.painting_slots[i].is_active != 0 &&
            gs.painting_slots[i].patch_gx == gx && gs.painting_slots[i].patch_gz == gz) {
            return false;
        }
    }

    // Also check if a gallery center is already active for this patch
    for (uint32_t g = 0; g < MAX_GALLERIES; g++) {
        if (gs.gallery_centers[g].active &&
            gs.gallery_centers[g].patch_gx == gx && gs.gallery_centers[g].patch_gz == gz)
            return false;
    }

    // Spawn roll (the chance arrived composed)
    uint32_t seed = tile_seed(c->world_state_.active_seed, gx, gz);
    float gallery_roll = cpu_hash_f(seed, GalleryProp::SPAWN_ROLL);
    if (gallery_roll >= composed.chance) return false;

    // Gallery center (jittered within patch)
    float patch_cx = (gx + 0.5f) * Dim::PATCH_EXTENT;
    float patch_cz = (gz + 0.5f) * Dim::PATCH_EXTENT;
    float center_offset = cpu_hash_f(seed, GalleryProp::CENTER_OFFSET) * Dim::PATCH_EXTENT * GalleryConfig::POSITION_JITTER;
    float center_angle = cpu_hash_f(seed, GalleryProp::CENTER_ANGLE) * 6.283185f;
    float gallery_cx = patch_cx + std::cos(center_angle) * center_offset;
    float gallery_cz = patch_cz + std::sin(center_angle) * center_offset;

    // Gallery-to-gallery distance check (belt + suspenders; MIN_SEPARATION handles most)
    float min_dist_sq = GalleryConfig::MIN_GALLERY_DISTANCE * GalleryConfig::MIN_GALLERY_DISTANCE;
    for (uint32_t g = 0; g < MAX_GALLERIES; g++) {
        if (!gs.gallery_centers[g].active) continue;
        float dx = gallery_cx - gs.gallery_centers[g].x;
        float dz = gallery_cz - gs.gallery_centers[g].z;
        if (dx * dx + dz * dz < min_dist_sq) return false;
    }

    // Find free center slot
    uint32_t gallery_slot = UINT32_MAX;
    for (uint32_t g = 0; g < MAX_GALLERIES; g++) {
        if (!gs.gallery_centers[g].active) { gallery_slot = g; break; }
    }
    if (gallery_slot == UINT32_MAX) return false;

    // Reserve slot
    gs.gallery_centers[gallery_slot].active = true;
    gs.gallery_centers[gallery_slot].patch_gx = gx;
    gs.gallery_centers[gallery_slot].patch_gz = gz;

    // NO RADIUS HERE. It was computed from PAINTINGS_MAX_BY_ARCHETYPE — the
    // archetype MAXIMUM — before commit capped the count to available content,
    // giving 87/105/123/123 wu against a median true half-span near 50. The
    // radius is a PLACEMENT fact and is now computed there, from the count
    // place actually reserves. See place_gallery_from_selection.

    // Painting count (seed-derived; capped to content availability in commit)
    float count_raw = GalleryConfig::PAINTINGS_MEAN
        + (cpu_hash_f(seed, GalleryProp::PAINTING_COUNT_R1) + cpu_hash_f(seed, GalleryProp::PAINTING_COUNT_R2)
            + cpu_hash_f(seed, GalleryProp::PAINTING_COUNT_R3) - 1.5f) * GalleryConfig::PAINTINGS_SIGMA;
    uint32_t painting_count = (uint32_t)std::max(
        (float)GalleryConfig::PAINTINGS_MIN,
        std::min((float)GalleryConfig::PAINTINGS_MAX_BY_ARCHETYPE[archetype],
            std::round(count_raw)));

    // Facing + size
    float facing_angle = cpu_hash_f(seed, GalleryProp::FACING_ANGLE) * 6.283185f;
    float gallery_size_mean = GalleryConfig::GALLERY_SIZE_LO
        + cpu_hash_f(seed, GalleryProp::SIZE_JITTER) * (GalleryConfig::GALLERY_SIZE_HI - GalleryConfig::GALLERY_SIZE_LO);

    // Site type (seed-derived; content availability validated in commit)
    float site_roll = cpu_hash_f(seed, GalleryProp::SITE_TYPE_ROLL);
    uint32_t site_type;
    if (site_roll < GalleryConfig::OUTDOOR_SNAPSHOT_ONLY) {
        site_type = GallerySiteType::SNAPSHOT_ONLY;
    }
    else if (site_roll < GalleryConfig::OUTDOOR_SNAPSHOT_ONLY + GalleryConfig::OUTDOOR_MIXED) {
        site_type = GallerySiteType::MIXED;
    }
    else {
        site_type = GallerySiteType::AUTHORED_ONLY;
    }

    sel.seed = seed;
    sel.trigger_gx = gx;
    sel.trigger_gz = gz;
    sel.slot = gallery_slot;
    sel.cx = gallery_cx;
    sel.cz = gallery_cz;
    sel.archetype = archetype;
    sel.painting_count = painting_count;
    sel.facing_angle = facing_angle;
    sel.gallery_size_mean = gallery_size_mean;
    sel.site_type = site_type;

    return true;
}

// ── place_gallery_from_selection ──

// The fan's true extent, from the count that will actually be laid out.
//
// The paintings sit on a LINE of half-span (count-1) x ROW_SPACING/2 along the
// row axis, offset by up to ROW_DEPTH_MIN + ROW_DEPTH_RANGE perpendicular and
// ROW_LATERAL_JITTER along it. The whole fan then ROTATES by facing_angle.
//
// indoor_bounds_clamp takes ONE scalar and applies it to both axes, so the
// honest answer for a fan that can face any direction is the CIRCUMSCRIBING
// radius of that rectangle — rotation-invariant, and correct at every angle
// rather than only at the axis-aligned ones.
inline float gallery_fan_radius(uint32_t count) {
    const float half_span = (count > 0 ? (float)(count - 1) : 0.0f)
        * 0.5f * GalleryConfig::ROW_SPACING + GalleryConfig::ROW_LATERAL_JITTER;
    const float half_depth = GalleryConfig::ROW_DEPTH_MIN + GalleryConfig::ROW_DEPTH_RANGE;
    return std::sqrt(half_span * half_span + half_depth * half_depth)
        + GalleryConfig::PAINTING_HALF + GalleryConfig::FAN_MARGIN;
}

// How many staging layers are free for a gallery being placed right now.
// Mirrors commit's availability test, minus the two things place cannot see:
// the mono-tier curation (a per-gallery filter) and the lazy authored texture
// load (a GPU write, and the place phase writes no GPU state — preserved law).
// Both can only REDUCE what commit finds, so this is an upper bound and the
// radius errs large rather than small.
inline uint32_t gallery_available_staging(const GalleryState& gs, uint32_t site_type) {
    uint32_t snaps = 0;
    for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++)
        if (gs.snapshot_staging[i].valid && !gs.snapshot_staging[i].consumed) snaps++;
    uint32_t pool = (site_type == GallerySiteType::SNAPSHOT_ONLY) ? snaps
                  : (site_type == GallerySiteType::AUTHORED_ONLY) ? gs.authored_staged_count
                  : snaps + gs.authored_staged_count;
    return pool > gs.staging_reserved ? pool - gs.staging_reserved : 0u;
}

inline bool place_gallery_from_selection(MachineCtx* c, const GallerySelection& sel, GalleryPlacement& plan) {
    auto& gs = c->gallery_state_;

    // THE RESERVATION. The count is resolved HERE, against content, so the
    // radius below describes the gallery that will actually be built. Commit
    // draws from this instead of discovering scarcity after the ground is
    // already claimed.
    const uint32_t avail = gallery_available_staging(gs, sel.site_type);
    const uint32_t reserved = sel.painting_count < avail ? sel.painting_count : avail;
    if (reserved == 0) return false;   // no content: never claim ground for a gallery that cannot exist

    // ONE extent, used for both questions, and they are the same question.
    // The old code passed the SAME VARIABLE for footprint and containment too
    // — that was never the defect. The defect was that the variable came from
    // the archetype maximum. Ground claim and containment are both "how far
    // does this fan reach", and for a fan of realized count that is one number.
    const float footprint_r = gallery_fan_radius(reserved);

    float cx = sel.cx, cz = sel.cz;
    if (!indoor_bounds_clamp(c, PopFamily::GALLERY, footprint_r, footprint_r, cx, cz))
        return false;
    if (!check_position(c, cx, cz, footprint_r, PopFamily::GALLERY))
        return false;

    int32_t host_gx = (int32_t)std::floor(cx / Dim::PATCH_EXTENT);
    int32_t host_gz = (int32_t)std::floor(cz / Dim::PATCH_EXTENT);

    if (register_footprint(c, cx, cz, footprint_r,
        host_gx, host_gz, PopFamily::GALLERY, sel.slot, sel.archetype) == UINT32_MAX)
        return false;

    gs.staging_reserved += reserved;   // released at commit, by the same value

    plan = GalleryPlacement{};
    plan.slot = sel.slot;
    plan.trigger_gx = sel.trigger_gx;
    plan.trigger_gz = sel.trigger_gz;
    plan.host_gx = host_gx;
    plan.host_gz = host_gz;
    plan.tier_idx = sel.archetype;
    plan.cx = cx;
    plan.cz = cz;
    plan.footprint_r = footprint_r;
    plan.reserved_count = reserved;
    plan.archetype = sel.archetype;
    plan.painting_count = sel.painting_count;
    plan.facing_angle = sel.facing_angle;
    plan.gallery_size_mean = sel.gallery_size_mean;
    plan.site_type = sel.site_type;

    return true;
}

// ── commit_gallery ──

inline void commit_gallery(GalleryState& gs, MachineCtx* c,
    const GalleryPlacement& plan,
    int32_t trigger_gx, int32_t trigger_gz, wgpu::Queue& queue)
{
    uint32_t seed = plan.trigger_gx != INT32_MAX
        ? tile_seed(c->world_state_.active_seed, plan.trigger_gx, plan.trigger_gz) : 0u;
    int32_t gx = plan.trigger_gx, gz = plan.trigger_gz;
    float gallery_cx = plan.cx, gallery_cz = plan.cz;
    uint32_t archetype = plan.archetype;
    float gallery_size_mean = plan.gallery_size_mean;
    float facing_angle = plan.facing_angle;

    // Release the reservation place took. Balanced by construction: every
    // placement reaches commit exactly once, because commit_entity_queue
    // drains placementResults_ whole. Done FIRST so every early return below
    // releases it too.
    gs.staging_reserved = gs.staging_reserved > plan.reserved_count
        ? gs.staging_reserved - plan.reserved_count : 0u;

    // Resolve site type with content availability
    uint32_t site_type = plan.site_type;
    if (site_type != GallerySiteType::SNAPSHOT_ONLY && !gs.authored_textures_loaded) {
        load_authored_textures(gs, c->gpuState_, queue);
    }
    if (site_type != GallerySiteType::SNAPSHOT_ONLY && !gs.authored_textures_loaded) {
        site_type = GallerySiteType::SNAPSHOT_ONLY;
    }

    // Snapshot candidates
    struct Candidate { uint32_t layer; };
    Candidate candidates[Dim::STAGING_LAYERS];
    uint32_t candidate_count = 0;
    for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) {
        if (gs.snapshot_staging[i].valid && !gs.snapshot_staging[i].consumed)
            candidates[candidate_count++] = { i };
    }

    // THE RESIDUAL ZERO-CONTENT ABORTS. SPAWN_4's reservation removed the
    // dominant path (place rejects when nothing is available), but these three
    // survive for a reason the reservation structurally cannot cover: the
    // mono-tier curation below is a per-gallery seed-derived filter that only
    // commit can apply, and load_authored_textures is a GPU write, barred from
    // place by the standing law that the place phase writes no GPU state.
    // Each releases the ground place claimed.
    bool have_snapshots = candidate_count > 0;
    bool have_authored = gs.authored_staged_count > 0;
    if ((site_type == GallerySiteType::SNAPSHOT_ONLY && !have_snapshots)
        || (site_type == GallerySiteType::AUTHORED_ONLY && !have_authored)
        || (site_type == GallerySiteType::MIXED && !have_snapshots && !have_authored)) {
        unregister_footprint_for(c, PopFamily::GALLERY, plan.slot);
        gs.gallery_centers[plan.slot].active = false;
        return;
    }

    // Snapshot curation (mono tier + chronological sort)
    if (have_snapshots) {
        bool mono_tier = cpu_hash_f(seed, GalleryProp::MONO_TIER_ROLL) < GalleryConfig::MONO_TIER_CHANCE;
        if (mono_tier) {
            static constexpr uint32_t FAVORITE_TIERS[] = {
                (uint32_t)ShotType::PANORAMIC,
                (uint32_t)ShotType::PORTRAIT,
                (uint32_t)ShotType::CINEMATIC
            };
            uint32_t chosen = FAVORITE_TIERS[cpu_hash(seed, GalleryProp::FAVORITE_TIER_PICK) % 3];
            uint32_t write = 0;
            for (uint32_t c = 0; c < candidate_count; c++) {
                if (gs.snapshot_staging[candidates[c].layer].shot_type == chosen)
                    candidates[write++] = candidates[c];
            }
            candidate_count = write;
            have_snapshots = candidate_count > 0;
        }
        for (uint32_t i = 0; i < candidate_count; i++) {
            for (uint32_t j = i + 1; j < candidate_count; j++) {
                if (gs.snapshot_staging[candidates[j].layer].capture_frame
                    < gs.snapshot_staging[candidates[i].layer].capture_frame) {
                    Candidate tmp = candidates[i];
                    candidates[i] = candidates[j];
                    candidates[j] = tmp;
                }
            }
        }
    }
    uint32_t snap_cursor = 0;

    // DRAW FROM THE RESERVATION. Place already resolved the count against
    // content and sized the registered footprint from it, so re-deriving a
    // larger number here would lay out paintings the claimed ground does not
    // cover. The two remaining caps below it are the ones place cannot see:
    // mono-tier curation (per-gallery) and the authored texture load (a GPU
    // write, barred from place). Both only ever REDUCE.
    uint32_t painting_count = plan.reserved_count;
    uint32_t max_available = candidate_count + gs.authored_staged_count;
    if (site_type == GallerySiteType::SNAPSHOT_ONLY) max_available = candidate_count;
    if (site_type == GallerySiteType::AUTHORED_ONLY) max_available = gs.authored_staged_count;
    if (painting_count > max_available) painting_count = max_available;

    // Layout
    float face_x = std::cos(facing_angle);
    float face_z = std::sin(facing_angle);
    float row_x = -face_z;
    float row_z = face_x;
    // GUARDED: painting_count is uint32_t and the caps above can drive it to
    // 0, where (count - 1) wraps to 0xFFFFFFFF and row_start becomes about
    // -3.87e10. Inert before this stage only because the loop never ran at 0 —
    // and this stage is exactly the refactor that hoists the layout math.
    float row_start = painting_count > 0
        ? -(float)(painting_count - 1) * 0.5f * GalleryConfig::ROW_SPACING
        : 0.0f;

    uint32_t placed = 0;
    bool usedAuthored[Dim::STAGING_LAYERS]{};

    for (uint32_t p = 0; p < painting_count; p++) {
        // Outdoor takes from the whole pool: it is the path the reserve
        // exists to PROTECT, not the one it holds back.
        uint32_t slot = find_free_painting_slot(gs, 0u);
        if (slot == UINT32_MAX) break;
        if (slot + 1 > gs.slot_high_water) gs.slot_high_water = slot + 1;

        uint32_t p_seed = cpu_hash(seed, GalleryProp::PER_PAINTING_BASE + p * GalleryProp::PER_PAINTING_STRIDE);

        float t = row_start + (float)p * GalleryConfig::ROW_SPACING;
        float lateral_jitter = (cpu_hash_f(p_seed, GalleryPaintingProp::LATERAL_JITTER) - 0.5f) * 2.0f
            * GalleryConfig::ROW_LATERAL_JITTER;
        float depth_offset = GalleryConfig::ROW_DEPTH_MIN
            + cpu_hash_f(p_seed, GalleryPaintingProp::DEPTH_JITTER) * GalleryConfig::ROW_DEPTH_RANGE;
        if (p % 2 == 1) depth_offset = -depth_offset;

        float paint_x = gallery_cx + row_x * (t + lateral_jitter) + face_x * depth_offset;
        float paint_z = gallery_cz + row_z * (t + lateral_jitter) + face_z * depth_offset;

        bool use_authored = (site_type == GallerySiteType::AUTHORED_ONLY)
            || (site_type == GallerySiteType::MIXED
                && cpu_hash_f(p_seed, GalleryPaintingProp::MIX_AUTHOR_ROLL) < GalleryConfig::OUTDOOR_MIX_AUTHORED_CHANCE);

        if (use_authored && count_unused_authored(gs, usedAuthored) == 0) {
            use_authored = false;
        }
        if (!use_authored && snap_cursor >= candidate_count) {
            if (count_unused_authored(gs, usedAuthored) > 0) {
                use_authored = true;
            }
            else {
                break;
            }
        }

        auto& s = gs.painting_slots[slot];
        bool placed_this = false;

        if (use_authored) {
            uint32_t auth_stg = pick_authored_staging(gs, p_seed, GalleryPaintingProp::AUTH_STG_PICK);
            if (auth_stg == UINT32_MAX || usedAuthored[auth_stg]) {
                uint32_t best = UINT32_MAX, best_disk = UINT32_MAX;
                for (uint32_t a = 0; a < Dim::STAGING_LAYERS; a++) {
                    if (!usedAuthored[a] && gs.authored_staging[a].valid && !gs.authored_staging[a].consumed
                        && gs.authored_staging[a].disk_index < best_disk) {
                        best_disk = gs.authored_staging[a].disk_index;
                        best = a;
                    }
                }
                if (best == UINT32_MAX) { use_authored = false; }
                else { auth_stg = best; }
            }

            if (use_authored) {
                uint32_t exh = find_free_exhibition_layer(gs);
                if (exh == UINT32_MAX) break;

                usedAuthored[auth_stg] = true;
                const auto& img = gs.authored_staging[auth_stg];
                float jitter = (cpu_hash_f(p_seed, GalleryPaintingProp::SIZE_JITTER_A)
                    + cpu_hash_f(p_seed, GalleryPaintingProp::SIZE_JITTER_B)
                    + cpu_hash_f(p_seed, GalleryPaintingProp::SIZE_JITTER_C) - 1.5f) * GalleryConfig::PAINTING_SIZE_SIGMA;
                float height = std::max(2.0f, (5.0f + jitter) * gallery_size_mean);

                fill_slot_wall_frame(s,
                    paint_x, 0.0f, paint_z,
                    face_x, 0.0f, face_z,
                    img.aspect_ratio, height,
                    exh, ContentSource::AUTHORED,
                    img.uv_scale_x, img.uv_scale_y,
                    FRAME_AUTHORED, gx, gz);
                s.geometry_seed = cpu_hash_f(p_seed, GalleryPaintingProp::GEOMETRY_SEED);

                gs.exhibition_occupied[exh] = true;
                gs.authored_staging[auth_stg].consumed = true;
                queue_promotion(gs, false, auth_stg, exh);
                gs.wall_frame_count++;
                placed_this = true;
            }
        }

        if (!placed_this && snap_cursor < candidate_count) {
            uint32_t staging_layer = candidates[snap_cursor].layer;
            const auto& snap = gs.snapshot_staging[staging_layer];
            snap_cursor++;

            uint32_t exh = find_free_exhibition_layer(gs);
            if (exh == UINT32_MAX) break;

            uint32_t shot_idx = snap.shot_type;
            float jitter = (cpu_hash_f(p_seed, GalleryPaintingProp::SIZE_JITTER_A)
                + cpu_hash_f(p_seed, GalleryPaintingProp::SIZE_JITTER_B)
                + cpu_hash_f(p_seed, GalleryPaintingProp::SIZE_JITTER_C) - 1.5f) * GalleryConfig::PAINTING_SIZE_SIGMA;
            float size_mult = std::max(0.5f, gallery_size_mean + jitter);
            float area = PAINTING_AREA[shot_idx] * size_mult;
            float scale_x = std::sqrt(area * snap.aspect_ratio);
            float scale_y = scale_x / snap.aspect_ratio;

            s = {};
            s.position[0] = paint_x;
            s.position[1] = 0.0f;
            s.position[2] = paint_z;
            s.geometry_seed = cpu_hash_f(p_seed, GalleryPaintingProp::GEOMETRY_SEED);
            s.forward[0] = face_x; s.forward[1] = 0.0f; s.forward[2] = face_z;
            s.scale_x = scale_x;
            s.up[0] = 0.0f; s.up[1] = 1.0f; s.up[2] = 0.0f;
            s.scale_y = scale_y;
            s.texture_layer = exh;
            s.form_type = FormType::TERRAIN_QUAD;
            s.is_active = 1;
            s.content_source = ContentSource::SNAPSHOT;
            // A snapshot fills its exhibition layer edge to edge. Written
            // inline rather than through fill_slot_wall_frame, which is why a
            // search for that CALL misses this site.
            s.uv_scale_x = 1.0f;
            s.uv_scale_y = 1.0f;
            s.patch_gx = gx; s.patch_gz = gz;

            gs.exhibition_occupied[exh] = true;
            gs.snapshot_staging[staging_layer].consumed = true;
            queue_promotion(gs, true, staging_layer, exh);
            placed_this = true;
        }

        if (!placed_this) break;

        c->gpuState_.upload_painting_slot(queue, slot, s);
        gs.active_painting_count++;
        placed++;
    }

    // Record gallery center
    auto& gc = gs.gallery_centers[plan.slot];
    gc.x = gallery_cx;
    gc.z = gallery_cz;
    gc.host_gx = plan.host_gx;
    gc.host_gz = plan.host_gz;

    if (placed == 0) {
        // Reachable when the mono-tier filter empties the candidate list after
        // the guards above passed. Release the ground with the centre.
        unregister_footprint_for(c, PopFamily::GALLERY, plan.slot);
        gc.active = false;  // no paintings placed — release center
    }
    else {
        // Autonomous stdout — exhibition-guard candidate, still open.
        std::cout << "[Gallery] slot=" << plan.slot
            << " at (" << gallery_cx << "," << gallery_cz << ")"
            << " host=(" << plan.host_gx << "," << plan.host_gz << ")"
            << " arch=" << archetype
            << " paintings=" << placed << "/" << painting_count
            << " type=" << (site_type == GallerySiteType::SNAPSHOT_ONLY ? "snap" :
                site_type == GallerySiteType::MIXED ? "mix" : "auth")
            << "\n";
    }
}
inline void evict_paintings_for_patch(GalleryState& gs, MachineCtx* c, int32_t gx, int32_t gz, wgpu::Queue& queue) {
    for (uint32_t i = 0; i < Dim::PAINTING_MAX_SLOTS; i++) {
        if (gs.painting_slots[i].is_active != 0 &&
            gs.painting_slots[i].patch_gx == gx && gs.painting_slots[i].patch_gz == gz) {

            // Free the exhibition layer
            uint32_t exh = gs.painting_slots[i].texture_layer;
            if (exh < Dim::EXHIBITION_LAYERS) {
                gs.exhibition_occupied[exh] = false;
            }

            if (gs.painting_slots[i].form_type == FormType::WALL_FRAME) {
                gs.wall_frame_count--;
            }

            gs.painting_slots[i].is_active = 0;
            c->gpuState_.deactivate_painting_slot(queue, i);
            gs.active_painting_count--;
        }
    }
    recompute_slot_high_water(gs);
}

// ═══ SNAPSHOT RENDER ═════════════════════════════════════════════

inline void render_snapshot_pass(GalleryState& gs, GalleryDeps* c, wgpu::CommandEncoder& encoder) {
    if (!gs.pending_snapshot.active) return;

    {
        wgpu::ComputePassDescriptor cpd{};
        cpd.label = "Photographer VP Compute";
        cpd.timestampWrites = c->gpuState_.meter_arm_compute(meter_row::SnapshotPass);
        wgpu::ComputePassEncoder compute = encoder.BeginComputePass(&cpd);
        c->renderer_.dispatch_compute_photographer_vp(
            compute, c->gpuState_.photographer_compute_group()
        );
        compute.End();
    }

    // Only render the snapshot if a capture is pending
    if (!gs.pending_snapshot.active) return;
    gs.pending_snapshot.active = false;
    uint32_t layer = gs.pending_snapshot.target_layer;
    std::cout << "[Photographer] Rendering snapshot -> layer " << layer << "\n";

    wgpu::RenderPassColorAttachment colorAtt{};
    colorAtt.view = c->gpuState_.offscreen_color_view();
    colorAtt.loadOp = wgpu::LoadOp::Clear;
    colorAtt.storeOp = wgpu::StoreOp::Store;
    colorAtt.clearValue = { (double)c->clearColor_[0], (double)c->clearColor_[1], (double)c->clearColor_[2], 1.0 };

    wgpu::RenderPassDepthStencilAttachment depthAtt{};
    depthAtt.view = c->gpuState_.offscreen_depth_view();
    depthAtt.depthLoadOp = wgpu::LoadOp::Clear;
    depthAtt.depthStoreOp = wgpu::StoreOp::Store;
    depthAtt.depthClearValue = 1.0f;

    wgpu::RenderPassDescriptor desc{};
    desc.label = "Photographer Snapshot";
    desc.colorAttachmentCount = 1;
    desc.colorAttachments = &colorAtt;
    desc.depthStencilAttachment = &depthAtt;
    desc.timestampWrites = c->gpuState_.meter_arm_render(meter_row::SnapshotPass);

    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&desc);

    // OIL_1 U13 (ledger: R19, C7) — this pass's head binds. The
    // photographer's entity window + the render texture group serve the
    // terrain fork AND every table draw below (the helpers no longer
    // bind their own). Bound before the first draw so the terrain fork
    // keeps its own bindings byte-for-byte.
    pass.SetBindGroup(0, c->gpuState_.photographer_render_entity_group());
    pass.SetBindGroup(1, c->gpuState_.render_texture_group());

    c->renderer_.draw_patch_terrain_direct(pass,
        c->gpuState_.photographer_render_entity_group(),
        c->gpuState_.render_texture_group(),
        c->gpuState_.patch_index_buffer_lod0_live(),
        c->gpuState_.patch_index_count_lod0_live(),
        c->world_state_.render_patch_count);

    // The drawable table — snapshot members, canonical order (the photographer's
    // own entity group). Terrain above is the per-pass FORK (a single direct
    // draw over render_patch_count, no LOD split). Zone is not a snapshot member.
    DrawBind b{ /*shadow=*/false,
                c->ribbon_state_.rendered_slot != UINT32_MAX };
    draw_table(c->renderer_, c->gpuState_, pass, b, DRAW_SNAPSHOT);

    // FORKS — the artworks, on the gallery layout bound to the
    // photographer's VP. THE PHOTOGRAPHER CAN NOW SHOOT THE EXHIBITION.
    // Not optional beside UMBRA_9: there is ONE sun shadow map and this
    // pass samples it, so an artwork that casts and is not drawn here
    // leaves a rectangle of shade lying on empty sand. The nesting this
    // opens is bounded — the sampled layer is already flat and resolved,
    // and R20 reads the exhibition texture before R21 writes it.
    // OIL_1 U13: the gallery pair (photographer VP window), bound ONCE
    // for both draws. ROSTER-GATE gallery (a') — matches the consumers.
    if constexpr (ROSTER.gallery) {
    pass.SetBindGroup(0, c->gpuState_.gallery_photographer_entity_group());
    pass.SetBindGroup(1, c->gpuState_.gallery_texture_group());
    }
    c->renderer_.draw_wall_paintings(
        pass,
        gs.wall_frame_count,
        gs.slot_high_water
    );
    c->renderer_.draw_gallery_frames(
        pass,
        gs.active_painting_count,
        gs.slot_high_water
    );

    pass.End();

    wgpu::TexelCopyTextureInfo src{};
    src.texture = c->gpuState_.offscreen_color_texture();
    src.mipLevel = 0;
    src.origin = { 0, 0, 0 };

    wgpu::TexelCopyTextureInfo dst{};
    dst.texture = c->gpuState_.snapshot_staging_texture();
    dst.mipLevel = 0;
    dst.origin = { 0, 0, layer };

    wgpu::Extent3D extent = { Dim::PAINTING_RESOLUTION, Dim::PAINTING_RESOLUTION, 1 };
    encoder.CopyTextureToTexture(&src, &dst, &extent);
}

// ═══ AUTHORED IMAGE LOADING ══════════════════════════════════════

// ═══ THE PLATFORM SEAM (EXHIBIT_0) ═══════════════════════════════
//
// LAW: the bundle carries the program; the network carries the
// exhibition. The two twins differ in exactly one thing — WHERE THE
// BYTES COME FROM. Native walks a directory and reads files; the web
// twin fetches a manifest and then each painting by URL. Everything
// downstream of the decode is one body, shared, because everything
// downstream is the same act.
//
// The two facts the seam must not fork are the ORDER the paintings
// hang in and the RECORD a hung painting is described by. They get one
// home each below, and both twins call them.

// ── THE NUMERIC SORT KEY — one rule, both twins ──
// PAINTING_1 < PAINTING_2 < PAINTING_10 < PAINTING_100, which is not
// what a lexicographic sort says. Semantics are the native lambda's,
// unchanged: the stem, the text after the FIRST '_', std::stoi (leading
// digits, and its throw caught as 0).
inline int authored_extract_number(const std::string& path) {
    namespace fs = std::filesystem;
    std::string name = fs::path(path).stem().string();  // "PAINTING_12"
    size_t pos = name.find('_');
    if (pos == std::string::npos || pos + 1 >= name.size()) return 0;
    try { return std::stoi(name.substr(pos + 1)); }
    catch (...) { return 0; }
}

// ── THE SCALE / PAD / UPLOAD — one body, both twins ──
// The decode differs (a path vs a buffer); everything after it does
// not. This body is also where aspect_ratio and the two uv_scale
// fields are DERIVED from dst_w/dst_h — a second copy would be a
// second place for the frame's shape to disagree with its texture.
// The caller owns `data` and frees it; the caller also prints the
// "[Authored] Loaded:" line, because only the caller knows the name.
inline void authored_stage_decoded_image(GalleryState& gs, GPUState& gpu, wgpu::Queue& queue,
    uint32_t staging_layer, uint32_t disk_index,
    const unsigned char* data, int width, int height) {
    constexpr uint32_t RES = Dim::PAINTING_RESOLUTION;
    float scale = std::min((float)RES / width, (float)RES / height);
    if (scale > 1.0f) scale = 1.0f;
    uint32_t dst_w = std::min((uint32_t)(width * scale + 0.5f), RES);
    uint32_t dst_h = std::min((uint32_t)(height * scale + 0.5f), RES);

    std::vector<uint8_t> padded(RES * RES * 4, 0);
    for (uint32_t dy = 0; dy < dst_h; ++dy) {
        float src_yf = (float)dy / scale;
        uint32_t sy0 = (uint32_t)src_yf;
        uint32_t sy1 = std::min(sy0 + 1, (uint32_t)(height - 1));
        float fy = src_yf - sy0;
        for (uint32_t dx = 0; dx < dst_w; ++dx) {
            float src_xf = (float)dx / scale;
            uint32_t sx0 = (uint32_t)src_xf;
            uint32_t sx1 = std::min(sx0 + 1, (uint32_t)(width - 1));
            float fx = src_xf - sx0;
            uint32_t i00 = (sy0 * width + sx0) * 4;
            uint32_t i10 = (sy0 * width + sx1) * 4;
            uint32_t i01 = (sy1 * width + sx0) * 4;
            uint32_t i11 = (sy1 * width + sx1) * 4;
            uint32_t di = (dy * RES + dx) * 4;
            for (int c = 0; c < 4; ++c) {
                float v = (1 - fx) * (1 - fy) * data[i00 + c] + fx * (1 - fy) * data[i10 + c]
                    + (1 - fx) * fy * data[i01 + c] + fx * fy * data[i11 + c];
                padded[di + c] = (uint8_t)(v + 0.5f);
            }
        }
    }

    gpu.upload_authored_painting(queue, staging_layer, padded.data(), RES, RES);

    auto& rec = gs.authored_staging[staging_layer];
    rec.disk_index = disk_index;
    rec.aspect_ratio = (height > 0) ? (float)width / (float)height : 1.0f;
    rec.uv_scale_x = (float)dst_w / RES;
    rec.uv_scale_y = (float)dst_h / RES;
    rec.valid = true;
    rec.consumed = false;

    std::cout << "[Authored] Scaled → " << dst_w << "x" << dst_h
        << " (aspect " << rec.aspect_ratio << ")\n";
}

#ifdef __EMSCRIPTEN__

// ═══ THE EXHIBITION ARRIVES OVER THE NETWORK ═════════════════════

// The manifest the dist script writes beside the program. NOT
// manifest.json — that name stays reserved for the PWA web manifest.
inline constexpr const char* EXHIBITION_MANIFEST_URL = "exhibition.json";
// The folder the manifest's bare filenames hang under. One place
// decides the layout; tools/web_dist.py writes to the same one.
inline constexpr const char* EXHIBITION_PAINTINGS_DIR = "paintings/";

// HOW MANY PAINTINGS MAY BE IN THE AIR AT ONCE. Above this the browser
// queues them anyway, and every request past the browser's own limit
// only makes the FIRST one land later — which is the one a gallery is
// waiting on. The rest wait here instead, in a queue this file can see.
inline constexpr uint32_t AUTHORED_FETCH_INFLIGHT_CAP = 4;

// A request that never answers holds its lane forever, so every request
// is given an end. Generous, because a phone on a slow connection
// fetching a half-megabyte JPEG is the normal case this must not kill.
inline constexpr unsigned long AUTHORED_FETCH_TIMEOUT_MS = 30000;

// The fetch's own copy of everything the answer will need. The two
// pointers outlive every fetch by construction: GalleryState and
// GPUState are members of the Cartridge, which is a member of App,
// heap-allocated in main() and never destroyed on this twin. The queue
// is a REFERENCE, not a pointer — wgpu handles are refcounted, so this
// copy keeps the queue alive on its own account.
struct AuthoredFetchCtx {
    GalleryState* gs;
    GPUState*     gpu;
    wgpu::Queue   queue;
    uint32_t      staging_layer;
    uint32_t      disk_index;
    std::string   url;
};

inline void pump_authored_fetches(GalleryState& gs, GPUState& gpu, wgpu::Queue& queue);

// authored_staged_count is a tally of valid records, and on this twin
// records become valid at arrival time rather than at call time — so
// the tally is RECOMPUTED where it changes, never incremented at a
// call site that cannot yet know the answer.
inline void recount_authored_staged(GalleryState& gs) {
    gs.authored_staged_count = 0;
    for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++)
        if (gs.authored_staging[i].valid) gs.authored_staged_count++;
}

// A SLOT THAT FAILED MUST STAY REACHABLE. Native's failure is final and
// harmless — the file is on disk or it is not, and a second read would
// fail the same way. A network failure is a different animal: a 502, a
// dropped connection on a phone, a name that lost its file between two
// dist runs. Left as {valid=false, pending=false, consumed=false} the
// slot would be unreachable forever: load_authored_textures has latched,
// and rotate only revisits CONSUMED slots. So a failure marks the slot
// consumed — still invalid, so nothing can pick it, but now exactly the
// shape the rotation is looking for, and the next world change re-asks.
// The disk claim is dropped with it so the cursor is free to hand that
// painting to whichever slot comes up.
inline void authored_fetch_release_slot(GalleryState& gs, uint32_t staging_layer) {
    auto& rec = gs.authored_staging[staging_layer];
    rec.valid = false;
    rec.consumed = true;
    rec.disk_index = UINT32_MAX;
}

// One exit for both outcomes: the lane is freed, the context is
// destroyed, and the queue is pumped so the next painting starts the
// instant this one is done with its lane.
inline void authored_fetch_finish(AuthoredFetchCtx* ctx) {
    GalleryState& gs = *ctx->gs;
    GPUState& gpu = *ctx->gpu;
    wgpu::Queue queue = ctx->queue;
    gs.authored_staging[ctx->staging_layer].pending = false;
    if (gs.authored_fetch_inflight > 0) gs.authored_fetch_inflight--;
    recount_authored_staged(gs);
    delete ctx;
    pump_authored_fetches(gs, gpu, queue);
}

inline void authored_image_onsuccess(emscripten_fetch_t* fetch) {
    AuthoredFetchCtx* ctx = (AuthoredFetchCtx*)fetch->userData;
    int width = 0, height = 0, channels = 0;
    // stb allocates its own pixels, so the fetch buffer is dead the
    // moment the decode returns — closed here rather than later, so no
    // path below can leak it.
    unsigned char* data = stbi_load_from_memory(
        (const stbi_uc*)fetch->data, (int)fetch->numBytes, &width, &height, &channels, 4);
    emscripten_fetch_close(fetch);

    if (!data) {
        std::cerr << "[Authored] Failed to load: " << ctx->url << "\n";
        authored_fetch_release_slot(*ctx->gs, ctx->staging_layer);
        authored_fetch_finish(ctx);
        return;
    }

    std::cout << "[Authored] Loaded: " << ctx->url
        << " (" << width << "x" << height << ") → staging " << ctx->staging_layer << "\n";

    // ── THE DEVICE-LOST EXEMPTION, NAMED ─────────────────────────
    // The call below ends in a queue WriteTexture, and this is the one
    // GPU write in the program that does NOT sit under the frame gate
    // (incubator_dual.cpp: `if (app->console.device_lost()) return;`).
    // A fetch completion is a browser event, not a frame, so a painting
    // that lands after the device is lost writes through a dead queue.
    //
    // That is allowed here, deliberately, and the reasoning is the
    // whole comment:
    //   · The gate's own warning is about NATIVE Dawn, where the loss
    //     destroys the objects and driving them afterwards is heap
    //     corruption. This path is __EMSCRIPTEN__-only and cannot reach
    //     that case.
    //   · On this twin the queue is a JS WebGPU handle. Per the spec,
    //     work submitted to a lost device is dropped — a no-op, not a
    //     fault.
    //   · By the time it could happen the visitor is already looking at
    //     the LOST card: console.hpp's loss callback prints
    //     "[Device] LOST", and web/index.html treats that line as
    //     terminal and replaces the world with the card.
    //   · The exposure is bounded by AUTHORED_FETCH_INFLIGHT_CAP —
    //     at most four uploads, once, into a page that is already over.
    //
    // So: NO SIGNAL, NO PLUMBING. Carrying a device-lost flag down to
    // this callback would add a second source of truth about the
    // device's health to buy nothing a dead page can spend.
    authored_stage_decoded_image(*ctx->gs, *ctx->gpu, ctx->queue,
        ctx->staging_layer, ctx->disk_index, data, width, height);
    stbi_image_free(data);
    authored_fetch_finish(ctx);
}

inline void authored_image_onerror(emscripten_fetch_t* fetch) {
    AuthoredFetchCtx* ctx = (AuthoredFetchCtx*)fetch->userData;
    std::cerr << "[Authored] Failed to load: " << ctx->url
        << " (HTTP " << fetch->status << ")\n";
    emscripten_fetch_close(fetch);
    // The record stays invalid, and it is handed back to the rotation
    // rather than abandoned — see authored_fetch_release_slot.
    authored_fetch_release_slot(*ctx->gs, ctx->staging_layer);
    authored_fetch_finish(ctx);
}

inline void start_authored_fetch(GalleryState& gs, GPUState& gpu, wgpu::Queue& queue,
    const GalleryState::AuthoredFetchRequest& req) {
    AuthoredFetchCtx* ctx = new AuthoredFetchCtx{
        &gs, &gpu, queue, req.staging_layer, req.disk_index, req.url
    };

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    std::strncpy(attr.requestMethod, "GET", sizeof(attr.requestMethod) - 1);
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    // A LANE MUST ALWAYS COME BACK. With no timeout a request that is
    // accepted and then stalls — a captive portal, a proxy holding the
    // connection open — never calls either callback, so its lane and its
    // slot are gone for the session. Four of those and the cap is
    // exhausted with the rest of the exhibition sitting in the queue,
    // silently, forever. The timeout routes to onerror, which is a path
    // that already frees everything.
    attr.timeoutMSecs = AUTHORED_FETCH_TIMEOUT_MS;
    attr.onsuccess = authored_image_onsuccess;
    attr.onerror = authored_image_onerror;
    attr.userData = ctx;

    gs.authored_fetch_inflight++;
    // THE START CAN FAIL, AND THEN NO CALLBACK EVER RUNS. Unwound here
    // by hand rather than through authored_fetch_finish, which would
    // re-enter the pump loop that is calling us.
    if (!emscripten_fetch(&attr, ctx->url.c_str())) {
        std::cerr << "[Authored] Failed to load: " << ctx->url << " (fetch not started)\n";
        if (gs.authored_fetch_inflight > 0) gs.authored_fetch_inflight--;
        gs.authored_staging[req.staging_layer].pending = false;
        authored_fetch_release_slot(gs, req.staging_layer);
        delete ctx;
    }
}

inline void pump_authored_fetches(GalleryState& gs, GPUState& gpu, wgpu::Queue& queue) {
    // Front-erase on a vector, deliberately: the queue is bounded by
    // STAGING_LAYERS (32), so the copy is a rounding error next to a
    // container choice that would need its own include.
    while (gs.authored_fetch_inflight < AUTHORED_FETCH_INFLIGHT_CAP
        && !gs.authored_fetch_queue.empty()) {
        GalleryState::AuthoredFetchRequest req = gs.authored_fetch_queue.front();
        gs.authored_fetch_queue.erase(gs.authored_fetch_queue.begin());
        start_authored_fetch(gs, gpu, queue, req);
    }
}

// ── Authored Image Loading (staging model) — the web twin ──
// SAME NAME, SAME CONTRACT, one word weaker: "this slot will hold this
// painting" instead of "this slot holds this painting". Every consumer
// already reads the slot through `valid`, so the weakening is invisible
// to all of them — a not-yet-arrived painting is the no-content case
// they have always handled.
inline void load_authored_image_to_staging(GalleryState& gs, GPUState& gpu, wgpu::Queue& queue, uint32_t staging_layer, uint32_t disk_index, const char* path) {
    if (staging_layer >= Dim::STAGING_LAYERS) return;
    auto& rec = gs.authored_staging[staging_layer];
    if (rec.pending) return;   // already spoken for; a second request would race its own slot

    // THE OLD PICTURE STOPS COUNTING NOW, not when the new one lands.
    // The upload overwrites this staging layer, so a record left valid
    // would describe a texture that is about to change under it —
    // teardown's `consumed = false` sweep would then hand the stale
    // record back to a gallery just in time for the swap.
    rec.valid = false;
    rec.pending = true;
    rec.disk_index = disk_index;   // the slot advertises its claim to the rotation cursor

    gs.authored_fetch_queue.push_back({ staging_layer, disk_index, std::string(path) });
    pump_authored_fetches(gs, gpu, queue);
}

#else

// ── Authored Image Loading (staging model) ──

inline void load_authored_image_to_staging(GalleryState& gs, GPUState& gpu, wgpu::Queue& queue, uint32_t staging_layer, uint32_t disk_index, const char* path) {
    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 4);
    if (!data) {
        // Try fallback paths
        std::string alt = std::string("7t/") + path;
        data = stbi_load(alt.c_str(), &width, &height, &channels, 4);
    }
    if (!data) {
        std::cerr << "[Authored] Failed to load: " << path << "\n";
        return;
    }

    std::cout << "[Authored] Loaded: " << path
        << " (" << width << "x" << height << ") → staging " << staging_layer << "\n";

    authored_stage_decoded_image(gs, gpu, queue, staging_layer, disk_index, data, width, height);
    stbi_image_free(data);
}

#endif   // __EMSCRIPTEN__ — the byte source, and only the byte source

// ── Paintings folder scan ──

#ifdef __EMSCRIPTEN__

// THE MANIFEST PARSE, BY HAND. exhibition.json is a flat object of
// string arrays that tools/web_dist.py in THIS repo writes — a JSON
// library would be a dependency taken on for one shape. Find the key,
// take its bracket, read the quoted strings. No escape handling
// because the strings are filenames the same script emitted; anything
// else in the file is ignored rather than rejected, so a manifest that
// grows a field later cannot stop the paintings arriving.
inline void parse_exhibition_paintings(const char* data, size_t len,
    std::vector<std::string>& out) {
    std::string s(data, len);
    size_t key = s.find("\"paintings\"");
    if (key == std::string::npos) return;
    size_t open = s.find('[', key);
    if (open == std::string::npos) return;
    size_t close = s.find(']', open);
    if (close == std::string::npos) return;

    size_t i = open + 1;
    while (i < close) {
        size_t q0 = s.find('"', i);
        if (q0 == std::string::npos || q0 >= close) break;
        size_t q1 = s.find('"', q0 + 1);
        if (q1 == std::string::npos || q1 > close) break;
        if (q1 > q0 + 1) out.push_back(s.substr(q0 + 1, q1 - q0 - 1));
        i = q1 + 1;
    }
}

// THE ONE LINE THE SHELL COUNTS. web/index.html reads "found N
// paintings" off this sentence to size its progress (SHIP_0 U3), so
// the wording is the native wording, verbatim — only the place it
// names changes, because on this twin the place IS the manifest.
inline void exhibition_manifest_onsuccess(emscripten_fetch_t* fetch) {
    GalleryState* gs = (GalleryState*)fetch->userData;
    std::vector<std::string> names;
    parse_exhibition_paintings(fetch->data, (size_t)fetch->numBytes, names);
    emscripten_fetch_close(fetch);

    gs->authored_disk_manifest.clear();
    gs->authored_disk_manifest.reserve(names.size());
    for (const std::string& n : names)
        gs->authored_disk_manifest.push_back(std::string(EXHIBITION_PAINTINGS_DIR) + n);

    // Sorted here and not trusted from the file: the ORDER paintings
    // hang in is the program's rule, and a hand-edited manifest must
    // not be able to change it.
    std::sort(gs->authored_disk_manifest.begin(), gs->authored_disk_manifest.end(),
        [](const std::string& a, const std::string& b) {
            return authored_extract_number(a) < authored_extract_number(b);
        });

    std::cout << "[Authored] Scanned " << EXHIBITION_MANIFEST_URL
        << " — found " << gs->authored_disk_manifest.size() << " paintings\n";
}

inline void exhibition_manifest_onerror(emscripten_fetch_t* fetch) {
    long status = fetch->status;
    emscripten_fetch_close(fetch);
    // An exhibition that did not arrive is an exhibition that is not
    // there — the same sentence, and the same already-legal state, as
    // a missing folder on the native twin.
    std::cout << "[Authored] No paintings folder found"
        << " (" << EXHIBITION_MANIFEST_URL << ", HTTP " << status << ")\n";
}

// ONE FETCH, AT THE EARLIEST INSTANT THERE IS A GalleryState TO FILL.
// Called from the cartridge's constructor — which on this twin runs in
// main() BEFORE the console asks the browser for an adapter, so the
// manifest travels while the device request is still outstanding and
// is normally parsed before anything can want it.
inline void kick_exhibition_manifest_fetch(GalleryState& gs) {
    if (gs.authored_manifest_requested) return;
    gs.authored_manifest_requested = true;

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    std::strncpy(attr.requestMethod, "GET", sizeof(attr.requestMethod) - 1);
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = exhibition_manifest_onsuccess;
    attr.onerror = exhibition_manifest_onerror;
    attr.userData = &gs;
    emscripten_fetch(&attr, EXHIBITION_MANIFEST_URL);
}

// SAME NAME, SAME CONTRACT: "make authored_disk_manifest current".
// There is no directory to walk on this twin, so the honest answer is
// whatever the fetch has delivered so far — and before it lands, that
// is nothing. Not an error: the empty manifest is the already-legal
// no-paintings state, and the sentence that names it is the native
// one. Said once, because it stays true until it stops being asked.
inline void scan_paintings_folder(GalleryState& gs) {
    if (!gs.authored_disk_manifest.empty()) return;
    if (gs.authored_absence_logged) return;
    gs.authored_absence_logged = true;
    std::cout << "[Authored] No paintings folder found\n";
}

#else

inline void scan_paintings_folder(GalleryState& gs) {
    namespace fs = std::filesystem;
    gs.authored_disk_manifest.clear();

    // Try multiple base paths (build dir vs working dir)
    static constexpr const char* SEARCH_DIRS[] = {
        "assets/paintings",
        "7t/assets/paintings",
    };

    fs::path found_dir;
    for (const char* dir : SEARCH_DIRS) {
        if (fs::exists(dir) && fs::is_directory(dir)) {
            found_dir = dir;
            break;
        }
    }
    if (found_dir.empty()) {
        std::cout << "[Authored] No paintings folder found\n";
        return;
    }

    for (const auto& entry : fs::directory_iterator(found_dir)) {
        if (!entry.is_regular_file()) continue;
        std::string name = entry.path().filename().string();
        // Match PAINTING_*.jpg or PAINTING_*.jpeg (case-insensitive extension)
        if (name.rfind("PAINTING_", 0) != 0) continue;
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext != ".jpg" && ext != ".jpeg") continue;
        gs.authored_disk_manifest.push_back(entry.path().string());
    }

    // Sort by numeric value after PAINTING_ (not lexicographic)
    // PAINTING_1 < PAINTING_2 < PAINTING_10 < PAINTING_100
    // The rule itself now lives at authored_extract_number, above —
    // the web twin sorts its manifest by the same one.
    std::sort(gs.authored_disk_manifest.begin(), gs.authored_disk_manifest.end(),
        [](const std::string& a, const std::string& b) {
            return authored_extract_number(a) < authored_extract_number(b);
        });

    std::cout << "[Authored] Scanned " << found_dir.string()
        << " — found " << gs.authored_disk_manifest.size() << " paintings\n";
}

#endif   // __EMSCRIPTEN__ — the manifest's source, and only its source

inline void load_authored_textures(GalleryState& gs, GPUState& gpu, wgpu::Queue& queue) {
    if (gs.authored_textures_loaded) return;

    // Scan folder on first load
    if (gs.authored_disk_manifest.empty()) {
        scan_paintings_folder(gs);
    }
    if (gs.authored_disk_manifest.empty()) {
#ifndef __EMSCRIPTEN__
        gs.authored_textures_loaded = true;
#endif
        // WEB: THE FLAG DOES NOT LATCH ON AN EMPTY MANIFEST. Native's
        // "empty" is a verdict — the folder was walked and there is
        // nothing there — so latching is right: a second walk would
        // find the same nothing. This twin's "empty" is "the fetch has
        // not landed yet", and boot ALWAYS reads it before it can
        // (init_world runs the whole boot inside one rAF turn, so no
        // fetch callback can fire in the middle of it). Latching here
        // would lock the exhibition out for the session. Left false,
        // the next caller re-enters and finds the manifest.
        //
        // Leaving it false also wakes commit_gallery's demotion at the
        // top of this file (site_type != SNAPSHOT_ONLY && !loaded ->
        // SNAPSHOT_ONLY), which is dead code on native and correct
        // here: with no manifest there is no authored content, and
        // snapshot-only is exactly what such a gallery should be.
        return;
    }

    // Fill staging with the first STAGING_LAYERS images from manifest
    uint32_t manifest_size = (uint32_t)gs.authored_disk_manifest.size();
    uint32_t to_load = std::min(manifest_size, Dim::STAGING_LAYERS);
    for (uint32_t i = 0; i < to_load; i++) {
#ifdef __EMSCRIPTEN__
        // A PICTURE ALREADY HERE IS NOT RE-ASKED FOR. Today the flag
        // below makes this loop run exactly once, so nothing can be
        // valid or pending yet — but the flag no longer latches on an
        // empty manifest, and a re-entered fill would otherwise drop
        // rec.valid on a texture already uploaded and re-download it.
        // One line, and the interlock stops being the only thing
        // standing between this loop and thrown-away work.
        if (gs.authored_staging[i].valid || gs.authored_staging[i].pending) continue;
#endif
        load_authored_image_to_staging(gs, gpu, queue, i, i, gs.authored_disk_manifest[i].c_str());
        // WEB: adds nothing here — the record cannot be valid yet, and
        // the tally is recomputed at each arrival instead. The line
        // stays because on native it IS the tally.
        if (gs.authored_staging[i].valid) gs.authored_staged_count++;
    }
    gs.authored_write_cursor = to_load % Dim::STAGING_LAYERS;
    gs.authored_disk_cursor = to_load % manifest_size;
    gs.authored_textures_loaded = true;
    std::cout << "[Authored] Staged " << gs.authored_staged_count
        << "/" << manifest_size << " images\n";
}

inline void rotate_authored_staging(GalleryState& gs, GalleryDeps* c, wgpu::Queue& queue) {
    if (gs.authored_disk_manifest.empty()) return;
    uint32_t manifest_size = (uint32_t)gs.authored_disk_manifest.size();

    // Collect disk indices currently in unconsumed (surviving) slots
    // to avoid loading duplicates
    // THE CAP FAILS OPEN, and that is worth naming where it lives. Every
    // read below is guarded `disk_idx < 256 && disk_in_use[disk_idx]`, so
    // an index past the array does not overflow — it short-circuits to
    // "not in use", and the no-duplicates rule quietly stops applying to
    // the overflow. A manifest of 300 can hang one canvas twice.
    // "Generous" was true while this was a directory the repo owned;
    // EXHIBIT_0 made the manifest a deploy-time input, so
    // tools/web_dist.py mirrors this number as MANIFEST_DEDUPE_CAP and
    // warns loudly when a dist exceeds it. THIS array is the source —
    // raise it and that constant together.
    bool disk_in_use[256]{};  // generous upper bound
    for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) {
        if (gs.authored_staging[i].valid && !gs.authored_staging[i].consumed) {
            if (gs.authored_staging[i].disk_index < 256)
                disk_in_use[gs.authored_staging[i].disk_index] = true;
        }
#ifdef __EMSCRIPTEN__
        // A SLOT IN FLIGHT ALREADY OWNS ITS PAINTING. It is not valid
        // yet, so the test above cannot see it — and without this the
        // cursor would hand the same disk index to a second slot and
        // the wall would hang the same picture twice.
        if (gs.authored_staging[i].pending && gs.authored_staging[i].disk_index < 256)
            disk_in_use[gs.authored_staging[i].disk_index] = true;
#endif
    }

    uint32_t rotated = 0;
    for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) {
        if (!gs.authored_staging[i].consumed) continue;  // keep unconsumed
#ifdef __EMSCRIPTEN__
        // A SLOT ALREADY IN FLIGHT IS NOT ROTATED. The loop below reads
        // the disk cursor, ADVANCES it, and then assumes the load took —
        // it marks the painting in use and counts a rotation. On this
        // twin the load can decline (the pending guard), and the cursor
        // would have moved anyway: that manifest entry would be skipped
        // for a full lap and the log would report a rotation that never
        // happened. Skipping here costs the slot nothing; its fetch is
        // already on its way.
        if (gs.authored_staging[i].pending) continue;
#endif

        // Find next disk image not already in a surviving slot
        uint32_t attempts = 0;
        while (attempts < manifest_size) {
            uint32_t disk_idx = gs.authored_disk_cursor;
            gs.authored_disk_cursor = (gs.authored_disk_cursor + 1) % manifest_size;
            if (disk_idx < 256 && disk_in_use[disk_idx]) {
                attempts++;
                continue;
            }
            // Load this image into the vacated staging slot
            load_authored_image_to_staging(gs, c->gpuState_, queue, i, disk_idx,
                gs.authored_disk_manifest[disk_idx].c_str());
            if (disk_idx < 256) disk_in_use[disk_idx] = true;
            rotated++;
            break;
        }
        // If all manifest images are in surviving slots (unlikely with 50+),
        // the consumed slot just stays invalid
    }

    if (rotated > 0) {
        // Recount valid slots after rotation
        gs.authored_staged_count = 0;
        for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) {
            if (gs.authored_staging[i].valid) gs.authored_staged_count++;
        }
        // Autonomous stdout — exhibition-guard candidate, still open.
        std::cout << "[Authored] Rotated " << rotated
            << " slot(s), " << gs.authored_staged_count << " valid"
            << ", disk cursor at " << gs.authored_disk_cursor
            << "/" << manifest_size << "\n";
    }
}

// Count how many valid authored staging entries aren't in usedAuthored[]
inline uint32_t count_unused_authored(const GalleryState& gs, const bool usedAuthored[]) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) {
        if (gs.authored_staging[i].valid && !gs.authored_staging[i].consumed && !usedAuthored[i]) count++;
    }
    return count;
}

// Pick the next authored painting in numeric order (lowest disk_index first)
inline uint32_t pick_authored_staging(GalleryState& gs, uint32_t /*seed*/, uint32_t /*prop*/) {
    uint32_t best_slot = UINT32_MAX;
    uint32_t best_disk = UINT32_MAX;
    for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) {
        if (gs.authored_staging[i].valid && !gs.authored_staging[i].consumed
            && gs.authored_staging[i].disk_index < best_disk) {
            best_disk = gs.authored_staging[i].disk_index;
            best_slot = i;
        }
    }
    return best_slot;
}

// ═══ WALL PAINTINGS (indoor) ═════════════════════════════════════

inline void place_wall_paintings(GalleryState& gs, GalleryDeps* c, wgpu::Queue& queue, float bmin, float bmax, float wall_height) {
    // Clear any existing wall paintings first (indoor→indoor transitions)
    clear_wall_paintings(gs, c, queue);

    load_authored_textures(gs, c->gpuState_, queue);

    // Painting center base height (fraction of ceiling) — WALL_ART knob.
    constexpr float WALL_OFFSET = 0.05f;    // distance from wall surface

    // wall_height is the true top of the VERTICAL wall on both paths — ch on
    // FLAT, the collapsed spring on VAULT (P5a/P5b) — and it excludes
    // JOINT_OVERLAP, which is a mesh joint device, not usable wall.
    float paint_y_base = wall_height * WALL_ART.paint_y_frac;
    float wall_span = bmax - bmin;
    float wall_center = (bmin + bmax) * 0.5f;

    // Three-way site type: snapshot-only / mixed / authored-only
    uint32_t site_seed = cpu_hash(c->world_state_.active_seed, WallArtProp::SITE_SEED_OFFSET);
    float site_roll = cpu_hash_f(site_seed, WallArtProp::SITE_TYPE_ROLL);
    enum class IndoorSiteType { SNAPSHOT_ONLY, MIXED, AUTHORED_ONLY };
    IndoorSiteType site_type;
    if (site_roll < WALL_ART.snapshot_only_share && gs.snapshot_count > 0) {
        site_type = IndoorSiteType::SNAPSHOT_ONLY;
    }
    else if (site_roll < WALL_ART.snapshot_only_share + WALL_ART.mixed_share
        && gs.snapshot_count > 0) {
        site_type = IndoorSiteType::MIXED;
    }
    else {
        site_type = IndoorSiteType::AUTHORED_ONLY;
    }

    // Wall definitions: position, normal, tangent (for spacing)
    struct WallDef {
        float px, py, pz;    // wall center position
        float nx, ny, nz;    // inward normal
        float tx, tz;        // tangent direction (for spacing paintings along wall)
        float span;          // wall length
    };
    WallDef walls[] = {
        { wall_center, paint_y_base, bmin + WALL_OFFSET,   0,0,1,   1,0,  wall_span },
        { wall_center, paint_y_base, bmax - WALL_OFFSET,   0,0,-1,  -1,0, wall_span },
        { bmin + WALL_OFFSET, paint_y_base, wall_center,   1,0,0,   0,1,  wall_span },
        { bmax - WALL_OFFSET, paint_y_base, wall_center,  -1,0,0,   0,-1, wall_span },
    };
    constexpr uint32_t WALL_COUNT = 4;

    // Roll how many walls get paintings (1–4). Cumulative thresholds
    // come from WALL_ART — t1/t2/t3, residual → 4 walls.
    float wall_count_roll = cpu_hash_f(site_seed, WallArtProp::WALL_COUNT_ROLL);
    uint32_t active_wall_count;
    if (wall_count_roll < WALL_ART.wall_count_t1)      active_wall_count = 1;
    else if (wall_count_roll < WALL_ART.wall_count_t2) active_wall_count = 2;
    else if (wall_count_roll < WALL_ART.wall_count_t3) active_wall_count = 3;
    else                                               active_wall_count = 4;
    uint32_t active_walls[4] = { 0, 1, 2, 3 };
    // Fisher-Yates shuffle
    for (uint32_t i = 3; i > 0; i--) {
        uint32_t j = cpu_hash(site_seed, WallArtProp::WALL_SHUFFLE_BASE + i) % (i + 1);
        uint32_t tmp = active_walls[i];
        active_walls[i] = active_walls[j];
        active_walls[j] = tmp;
    }

    // Console tallies, split by content source. Counted here rather than
    // derived from wall_frame_count, which commit_gallery's authored branch
    // also increments.
    uint32_t authored_placed = 0;
    uint32_t snapshot_placed = 0;

    // Track which authored layers have been used across all walls (no duplicates)
    bool usedAuthored[Dim::STAGING_LAYERS]{};

    // THE WELD IS RESTORED. D broke it deliberately — texture_layer became a
    // reference so many frames could draw one image, which at three frames a
    // wall was invisible and at nineteen is the visible bug: the same snapshot
    // repeating down the row. One frame owns one layer owns one image again,
    // and D's whole mechanism goes with it — the snapLayer map on the
    // placement side AND the spacing rule on the selection side, which was the
    // half that handed a record out twice in the first place. Deleting only
    // the map would have kept the repeat and paid a second layer for it.
    //
    // What replaces it is nothing. Selection takes fresh records until there
    // are none, then the wall ends on its own `break`. Running out of content
    // is now a visible, bounded thing rather than a silent duplication.

    // ─── Painting scale buckets ────────────────────────────────────
    // Tabulated form lets the bucket-selection loop iterate the WALL_ART
    // sub-structs uniformly without repeating field names.
    const WallArtScaleBucket* INDOOR_SCALES[] = {
        &WALL_ART.intimate,
        &WALL_ART.standard,
        &WALL_ART.statement,
    };
    static constexpr uint32_t INDOOR_SCALE_COUNT = 3;

    for (uint32_t aw = 0; aw < active_wall_count; aw++) {
        uint32_t w = active_walls[aw];
        const auto& wall = walls[w];
        uint32_t w_seed = cpu_hash(site_seed, WallArtProp::PER_WALL_BASE + w * WallArtProp::PER_WALL_STRIDE);

        // Keep paintings away from corners — WALL_ART knob.
        float usable_span = std::max(wall.span - 2.0f * WALL_ART.corner_margin,
            wall.span * 0.3f);

        // ONE DIAL AND ITS CEILING. The count is READ OFF THE WALL, not rolled
        // — a long wall gets a long row and a short wall a short one, at one
        // scale, with no second system and therefore no seam. INDOOR_MAX_ROW_COUNT
        // is already min(per_wall_cap, derived), so this single min applies both
        // bounds; a second min against the cap here would apply the same
        // ceiling twice. It is also the plan array's bound made unconditional,
        // so an index cannot outrun its storage if a future mood widens the room.
        uint32_t count = std::min(INDOOR_MAX_ROW_COUNT,
            std::max(1u, (uint32_t)(usable_span / WALL_ART.target_spacing)));

        // ─── SELECTION PASS — AUTHORITATIVE ──────────────────────
        // Content source, staging record and the REAL aspect are all resolved
        // here; the placement pass below re-decides nothing. Planned width is
        // then placed width by construction, and group_start is exact.
        //
        // THE DEFECT THIS REMOVES. Widths were planned from est_aspect in
        // [0.8, 1.6] while cursor advanced by real widths, whose range over
        // SHOT_PARAMS is [0.56, 2.39] with a weighted mean near 1.60. So the
        // row ran long by ~4 wu per painting while group_start was computed
        // from the short total — a one-way push, always in the wall's own
        // cursor direction, +6 wu at three paintings and +10 at five. Around
        // four walls that reads as a rotational bias, and it is far easier to
        // see than the rare right-edge truncation it also caused.
        //
        // The trim loop dies as a separate structure; its job moves in here,
        // where it can bound against REAL widths. Plan only what fits, and the
        // row is both exactly centred and guaranteed inside usable_span.
        struct PlannedFrame {
            float    height;
            float    aspect;
            float    width;       // height * REAL aspect
            uint32_t record;      // staging index — snapshot or authored array
            uint32_t scale_idx;   // the bucket, carried rather than re-derived
            bool     is_snapshot;
        };
        // Sized from the derived bound, not from a padded literal — the array
        // and the slot budget then move together the moment target_spacing or
        // finite_radius_max does, and neither can silently fall behind.
        PlannedFrame plan[INDOOR_MAX_ROW_COUNT]{};
        uint32_t effective_count = count;   // already clamped to the bound
        float    total_width = 0.0f;
        uint32_t planned = 0;

        // Per-WALL claim masks. Selection does not set `consumed` — constraint
        // 2: a painting the placement pass drops on slot exhaustion (Stage C's
        // break) must not have wasted its record. The mask prevents two
        // paintings on THIS wall picking the same record; `consumed`, set at
        // placement, is what later walls see.
        bool snapClaimed[Dim::STAGING_LAYERS]{};
        bool authClaimed[Dim::STAGING_LAYERS]{};

        for (uint32_t p = 0; p < effective_count; p++) {
            uint32_t p_seed = cpu_hash(w_seed, WallArtProp::PER_PAINTING_BASE + p * WallArtProp::PER_PAINTING_STRIDE);

            // Scale selection (weighted) — unchanged, and the placement pass
            // re-derives the same bucket from the same p_seed for the Y offset.
            float wsel[INDOOR_SCALE_COUNT];
            for (uint32_t si = 0; si < INDOOR_SCALE_COUNT; si++) wsel[si] = INDOOR_SCALES[si]->weight;
            uint32_t scale_idx = select_tier(p_seed, WallPaintingProp::SCALE_ROLL, wsel, INDOOR_SCALE_COUNT);
            float h = INDOOR_SCALES[scale_idx]->height_lo
                + cpu_hash_f(p_seed, WallPaintingProp::HEIGHT_JITTER)
                  * (INDOOR_SCALES[scale_idx]->height_hi - INDOOR_SCALES[scale_idx]->height_lo);

            // Content decision — the one obstacle Stage A named against this
            // reorder. It resolves here now because the authored tally is a
            // plan-side count over the claim mask, not a live consumed-count.
            bool use_snapshot = (site_type == IndoorSiteType::SNAPSHOT_ONLY)
                || (site_type == IndoorSiteType::MIXED
                    && cpu_hash_f(p_seed, WallPaintingProp::MIX_SNAPSHOT_ROLL) < WALL_ART.mix_snapshot_chance);

            uint32_t auth_free = 0;
            for (uint32_t a = 0; a < Dim::STAGING_LAYERS; a++)
                if (gs.authored_staging[a].valid && !gs.authored_staging[a].consumed && !authClaimed[a])
                    auth_free++;
            if (!use_snapshot && auth_free == 0) use_snapshot = true;

            PlannedFrame f{};
            f.height = h;
            f.scale_idx = scale_idx;   // placement reads this instead of re-rolling
            bool resolved = false;

            if (use_snapshot) {
                uint32_t rec = UINT32_MAX;
                for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) {
                    if (gs.snapshot_staging[i].valid && !gs.snapshot_staging[i].consumed && !snapClaimed[i]) {
                        rec = i;
                        break;
                    }
                }
                if (rec != UINT32_MAX) {
                    snapClaimed[rec] = true;
                    f.record = rec; f.is_snapshot = true;
                    f.aspect = gs.snapshot_staging[rec].aspect_ratio;
                    resolved = true;
                }
                else if (auth_free > 0) {
                    use_snapshot = false;   // snapshots dry; try authored
                }
            }

            if (!resolved && !use_snapshot) {
                // Lowest disk_index first, the pick_authored_staging order,
                // with the claim mask on top.
                uint32_t a_rec = UINT32_MAX, best_disk = UINT32_MAX;
                for (uint32_t a = 0; a < Dim::STAGING_LAYERS; a++) {
                    if (!authClaimed[a] && gs.authored_staging[a].valid && !gs.authored_staging[a].consumed
                        && gs.authored_staging[a].disk_index < best_disk) {
                        best_disk = gs.authored_staging[a].disk_index;
                        a_rec = a;
                    }
                }
                if (a_rec != UINT32_MAX) {
                    authClaimed[a_rec] = true;
                    f.record = a_rec; f.is_snapshot = false;
                    f.aspect = gs.authored_staging[a_rec].aspect_ratio;
                    resolved = true;
                }
            }

            if (!resolved) break;   // no content of either kind — this wall is done

            f.width = f.height * f.aspect;
            const float add = f.width + (planned > 0 ? WALL_ART.painting_gap : 0.0f);
            if (total_width + add > usable_span) break;   // the trim, on REAL widths
            total_width += add;
            plan[planned++] = f;
        }
        effective_count = planned;

        // Exact: total_width IS the row's real width, so the row is centred and
        // its right edge lands at or inside wall_center + usable_span/2.
        float group_start = wall_center - total_width * 0.5f;
        float cursor = group_start;

        // ─── PLACEMENT PASS — consumes the plan, decides nothing ──
        for (uint32_t p = 0; p < effective_count; p++) {
            const PlannedFrame& f = plan[p];

            // The wall path stops short of the outdoor reserve. It runs once,
            // at mood entry, and would otherwise take everything before
            // commit_gallery gets a frame.
            uint32_t slot = find_free_painting_slot(gs, GalleryConfig::OUTDOOR_SLOT_RESERVE);
            if (slot != UINT32_MAX && slot + 1 > gs.slot_high_water) gs.slot_high_water = slot + 1;
            // Exhaustion ends THIS WALL, not the hang.
            if (slot == UINT32_MAX) break;

            // Y is placement's own: the vertical offset never fed the width
            // plan, so it stays here. The BUCKET is carried on the plan rather
            // than re-rolled — re-deriving it was correct only while both of
            // selection's failure paths were `break`, which keeps plan[] densely
            // packed so index p means the same thing in both loops. Turn one of
            // them into `continue` and heights would silently decorrelate from
            // vertical offsets. One field removes the trap with the duplication.
            uint32_t p_seed = cpu_hash(w_seed, WallArtProp::PER_PAINTING_BASE + p * WallArtProp::PER_PAINTING_STRIDE);
            const auto& bucket = *INDOOR_SCALES[f.scale_idx];
            float y_offset = bucket.y_offset_lo
                + cpu_hash_f(p_seed, WallPaintingProp::Y_OFFSET_JITTER) * (bucket.y_offset_hi - bucket.y_offset_lo);

            float py = wall.py + y_offset;
            const float half_h = f.height * 0.5f;

            float bottom = py - half_h;
            if (bottom > WALL_ART.max_bottom_height) {
                py = WALL_ART.max_bottom_height + half_h;
            }
            // The clamp's other end. Only the statement bucket can reach it:
            // its bottom spans [-1.5, 2.5] against standard's [1.5, 6.5] and
            // intimate's [3.5, 8.0], so a tall piece hung low was the one
            // shape that put a frame through the floor.
            else if (bottom < WALL_ART.min_bottom_height) {
                py = WALL_ART.min_bottom_height + half_h;
            }
            // R4's top side, symmetric to min_bottom_height and applied last so
            // it wins any conflict. It cannot fire on today's population — the
            // reachable top is 16.45 against wall_height 20.0 — and exists so
            // E-b's fill cannot breach the wall top.
            if (py + half_h > wall_height - WALL_ART.top_margin) {
                py = wall_height - WALL_ART.top_margin - half_h;
            }

            // ONE FRAME, ONE LAYER, ONE IMAGE. No sharing, no lookup — a free
            // layer or the wall ends here (Stage C's break, unchanged).
            uint32_t exh = find_free_exhibition_layer(gs);
            if (exh == UINT32_MAX) break;    // this wall, not the hang

            // Planned width IS placed width, so this cursor cannot run past
            // wall_center + usable_span/2 — selection bounded the total.
            float paint_center = cursor + f.width * 0.5f;
            float px = wall.px + wall.tx * (paint_center - wall_center);
            float pz = wall.pz + wall.tz * (paint_center - wall_center);

            auto& s = gs.painting_slots[slot];
            if (f.is_snapshot) {
                fill_slot_wall_frame(s,
                    px, py, pz,
                    wall.nx, wall.ny, wall.nz,
                    f.aspect, f.height,
                    exh, ContentSource::SNAPSHOT,
                    1.0f, 1.0f,
                    FRAME_SNAPSHOT,
                    INT32_MAX, INT32_MAX);

                gs.exhibition_occupied[exh] = true;
                gs.snapshot_staging[f.record].consumed = true;
                queue_promotion(gs, true, f.record, exh);
            }
            else {
                const auto& img = gs.authored_staging[f.record];
                fill_slot_wall_frame(s,
                    px, py, pz,
                    wall.nx, wall.ny, wall.nz,
                    f.aspect, f.height,
                    exh, ContentSource::AUTHORED,
                    img.uv_scale_x, img.uv_scale_y,
                    FRAME_AUTHORED,
                    INT32_MAX, INT32_MAX);

                gs.exhibition_occupied[exh] = true;
                gs.authored_staging[f.record].consumed = true;
                usedAuthored[f.record] = true;
                queue_promotion(gs, false, f.record, exh);
            }

            cursor += f.width + WALL_ART.painting_gap;
            c->gpuState_.upload_painting_slot(queue, slot, s);
            gs.wall_frame_count++;
            if (f.is_snapshot) snapshot_placed++; else authored_placed++;
        }
    }

    const char* site_type_name = (site_type == IndoorSiteType::SNAPSHOT_ONLY) ? "SNAPSHOT"
        : (site_type == IndoorSiteType::MIXED) ? "MIXED" : "AUTHORED";
    // Autonomous stdout — exhibition-guard candidate, still open.
    //
    // The counts are split by CONTENT SOURCE, not by tier: there is one tier
    // now. In a MIXED room the split is the mix roll made visible, and in an
    // AUTHORED_ONLY room a non-zero snapshot count is the authored pool
    // running dry and falling through (gallery.hpp's `count_unused_authored`
    // branch) — which is the one thing here worth seeing from the console.
    std::cout << "[WallPainting] Placed " << authored_placed
        << " painting(s) + " << snapshot_placed
        << " snapshot(s) across " << active_wall_count << " walls"
        << " (" << site_type_name << ")\n";
}

inline void clear_wall_paintings(GalleryState& gs, GalleryDeps* c, wgpu::Queue& queue) {
    for (uint32_t i = 0; i < Dim::PAINTING_MAX_SLOTS; i++) {
        // WALL_FRAME alone does not mean indoor. commit_gallery's authored
        // branch fills outdoor monuments through the same helper, so form
        // type matches those too; the sentinel patch pair is what separates
        // them, and it is the pair — the same discriminator world.wgsl uses
        // to skip indoor frames during outdoor Y-correction.
        if (gs.painting_slots[i].is_active != 0 &&
            gs.painting_slots[i].form_type == FormType::WALL_FRAME &&
            gs.painting_slots[i].patch_gx == INT32_MAX &&
            gs.painting_slots[i].patch_gz == INT32_MAX) {
            uint32_t exh = gs.painting_slots[i].texture_layer;
            if (exh < Dim::EXHIBITION_LAYERS) {
                gs.exhibition_occupied[exh] = false;
            }
            gs.painting_slots[i].is_active = 0;
            c->gpuState_.deactivate_painting_slot(queue, i);
        }
    }
    gs.wall_frame_count = 0;
    recompute_slot_high_water(gs);
}

// ═══ DISPATCH FUNNELS (table-shaped; declared in entity_types.hpp) ═

inline bool dispatch_select_gallery(MachineCtx* self,
    int32_t gx, int32_t gz, EntityQueueEntry& e) {
    return select_gallery_for_patch(self->gallery_state_, self, gx, gz, e.gallery);
}

inline bool dispatch_place_gallery(MachineCtx* self,
    EntityQueueEntry& e, PlacementEntry& pe) {
    pe.family = e.family; pe.gx = e.gx; pe.gz = e.gz;
    if (place_gallery_from_selection(self, e.gallery, pe.gallery)) {
        return true;
    }
    else {
        self->gallery_state_.gallery_centers[e.gallery.slot].active = false;
        return false;
    }
}

inline void dispatch_commit_gallery(MachineCtx* self,
    PlacementEntry& pe, wgpu::Queue& queue) {
    auto* host = find_patch(self, pe.gallery.host_gx, pe.gallery.host_gz);
    if (host) {
        commit_gallery(self->gallery_state_, self, pe.gallery, pe.gx, pe.gz, queue);
        // Only record entity_ref if gallery is still active (commit may deactivate on 0 paintings)
        if (self->gallery_state_.gallery_centers[pe.gallery.slot].active) {
            host->record_entity(PopFamily::GALLERY, pe.gallery.slot);
        }
    }
    else {
        // Host patch gone — release by owner (the patch key can never match).
        unregister_footprint_for(self, PopFamily::GALLERY, pe.gallery.slot);
        self->gallery_state_.gallery_centers[pe.gallery.slot].active = false;
    }
}

// ═══ THE EVICTOR ══════════════════════════════════════════════════

inline void evict_gallery(MachineCtx* self,
    uint32_t slot, wgpu::Queue& queue) {
    auto& gc = self->gallery_state_.gallery_centers[slot];
    if (gc.active) {
        evict_paintings_for_patch(self->gallery_state_, self, gc.patch_gx, gc.patch_gz, queue);
        unregister_footprint_for(self, PopFamily::GALLERY, slot);   // the hand that claims is the hand that frees
        gc.active = false;
    }
}


// ─── Teardown (owner verb) ────────────────────────────────────────
// NOTE the organ is SHARED with the indoor_shell feature (wall frames
// live in the same painting slots — form_type); the score gates the
// call on (ROSTER.gallery || ROSTER.indoor_shell).
inline void teardown_gallery(GalleryState& gs, GalleryDeps* c, wgpu::Queue& queue) {
    // Gallery / paintings — clear all exhibition + slots, keep staging intact
    for (uint32_t i = 0; i < MAX_GALLERIES; i++) {
        gs.gallery_centers[i] = GalleryCenter{};
    }
    gs.pending_snapshot.active = false;
    gs.pending_promotion_count = 0;
    gs.wall_frame_count = 0;
    gs.active_painting_count = 0;
    gs.slot_high_water = 0;
    // The place/commit reservation balance is void here: reset_surface (same
    // TEARDOWN block) zeroes placementCount_ without committing, so any
    // gallery placed-but-not-committed never reaches its release. Without this
    // the counter would be permanently high and every later gallery would
    // reserve against a pool it thinks is smaller than it is.
    gs.staging_reserved = 0;
    // Clear all painting slots (CPU + GPU)
    for (uint32_t i = 0; i < Dim::PAINTING_MAX_SLOTS; i++) {
        gs.painting_slots[i] = GPUPaintingSlot{};
    }
    {
        GPUPaintingSlot empty[Dim::PAINTING_MAX_SLOTS]{};
        c->gpuState_.upload_painting_slots(queue, empty, Dim::PAINTING_MAX_SLOTS);
    }
    // Free all exhibition layers (staging persists across worlds)
    for (uint32_t i = 0; i < Dim::EXHIBITION_LAYERS; i++) gs.exhibition_occupied[i] = false;
    rotate_authored_staging(gs, c, queue);
    for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) gs.authored_staging[i].consumed = false;
}

// ─── Promotion drain (owner verb) ─ copy staged snapshot/authored layers into the exhibition
// array. ORDER (O-7): after render_snapshot_pass, so the snapshot
// staging texture holds this frame's shot.
inline void drain_gallery_promotions(GalleryState& gs, GalleryDeps* c, wgpu::CommandEncoder& encoder) {
    for (uint32_t i = 0; i < gs.pending_promotion_count; i++) {
        auto& p = gs.pending_promotions[i];
        wgpu::Texture src = p.is_snapshot
            ? c->gpuState_.snapshot_staging_texture()
            : c->gpuState_.authored_staging_texture();
        c->gpuState_.promote_to_exhibition(encoder, src, p.staging_layer, p.exhibition_layer);
    }
    gs.pending_promotion_count = 0;
}

} // namespace the_board
} // namespace t7
