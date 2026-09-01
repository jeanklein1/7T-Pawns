#pragma once
#include <cstdint>
#include <random>     // std::mt19937 + distributions (PhotographerState sampling)
#include <string>
#include <vector>     // authored disk manifest
#include "core/instruments.hpp"                                        // PURSE_0 R1 — t7::g_served_k, the presentation law's own verdict
#include "core/aubade.hpp"                                            // AUBADE U1 — the authored6 mark and the stb decode accumulator
#include "cartridges/the_board/realization/state.hpp"                    // Dim::*, GPUPaintingSlot, GPUPhotographerConfig, wgpu
#include "cartridges/the_board/contracts/mood_constants.hpp"   // MOOD_COUNT (sizes the mood gate)
#include "cartridges/the_board/primitives/seed_utils.hpp"       // select_weighted (PhotographerState::sample_shot_type)
#include "cartridges/the_board/contracts/wgpu_fwd.hpp"   // wgpu handle fwds (lockstep insurance)
#include "external/stb_image.h"                                // stbi_load/stbi_image_free — authored painting loader (dependency named here)
#include "cartridges/the_board/contracts/entity_types.hpp"     // GallerySelection/GalleryPlacement (the boundary DTOs) + queue types

// ─── gallery.hpp (HEADER: vocabulary + configs + state + decls) ──
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
#include <chrono>      // AUBADE U1 — the stb/scale decode accumulator   // (impl, merged)
#include <cstdint>   // (impl, merged)
#include <filesystem>  // paintings folder scan   // (impl, merged)
#include <iostream>    // capture / gallery / authored logs   // (impl, merged)
#include <iomanip>     // std::fixed, std::setprecision — the time-to-poster witness (ATRIUM_10)   // (impl, merged)
#include <string>      // manifest paths, std::stoi   // (impl, merged)
#include <vector>      // manifest + pixel staging   // (impl, merged)
#include <cstring>            // std::strncpy — emscripten_fetch_attr_t::requestMethod   (EXHIBIT_0)
#include <emscripten/fetch.h> // the web twin's byte source: the network, not a filesystem (EXHIBIT_0)
#include "core/instruments.hpp"   // RIBBON_4 — INSTRUMENTS.stream_witness gates the steady path's witness lines

namespace t7 {
namespace the_board {

// ═══ MODULE DEPS ════════════════════════════════════════════════════
// The photographer + exhibition's requirements face. sun/clear are
// the root cosmetic arrays (by reference); ribbon_state_ read for the
// snapshot draw gate. All reads except GPU wire + renderer.
class Renderer;
struct WorldState; struct TileWorldState; struct RibbonState; struct TimeState;
struct GalleryDeps {
    GPUState&             gpuState_;
    Renderer&            renderer_;
    const WorldState&    world_state_;
    const TileWorldState& tile_world_state_;
    const RibbonState&   ribbon_state_;
    const PlayerState&   player_;
    const PointState&    point_;    // the point's house (position mirror — wall-frame placement)
    const MoodState&     mood_state_;
    // ATRIUM_10 — the clock, const. The entrance's one number is a TIME, and
    // a witness that cannot read the clock cannot be printed. Read-only, like
    // every other row here; the agents' face has carried it since the census.
    const TimeState&     time_state_;
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

    // PURSE_0 R1 — THE PHOTOGRAPHER WAITS FOR HEADROOM. NAMED CONSTANTS,
    // NOT DIALS: nobody sees when a snapshot is taken, so there is nothing
    // here for an operator to have an opinion about.
    //
    // The gate: the last frame was served at k = 1 — the presentation law's
    // own verdict that it made its refresh. The ceiling: a capture deferred
    // longer than this fires anyway. BOUNDED STARVATION IS THE POINT — the
    // pool must still fill on a slow machine, only slower; an unbounded
    // wait would mean a machine that never has headroom never builds a
    // gallery, which is worse than the hitch.
    static constexpr uint32_t PHOTO_HEADROOM_K = 1u;
    static constexpr float    PHOTO_DEFER_MAX_S = 4.0f;

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
    // Spawn probability per terrain archetype id. FLAT (SAND_1): galleries
    // are the program's subject and the audience must not have to hunt them.
    //
    // 0.70 SATURATES. One placed gallery excludes a disc of pi*110^2, about
    // fifteen patches at PATCH_EXTENT 50, so ten of those fifteen roll a hit
    // and exactly one can win — the exclusion sets the density and the roll
    // only picks which patch. That is not a defect here, it is the mechanism:
    // an exclusion-limited packing IS an even distribution, which is what was
    // asked for. The value would only start governing below ~0.07, and there
    // it buys erratic rather than common.
    static constexpr float GALLERY_CHANCE_BY_ARCHETYPE[4] = { 0.70f, 0.70f, 0.70f, 0.70f };

    // Painting count per gallery: gaussian, median 3, σ 1.
    //
    // THE COUNT IS NOT A TASTE HERE, IT IS WHAT THE OTHER TWO RULINGS LEAVE.
    // gallery_fan_radius goes as (count-1) * ROW_SPACING/2, so count and
    // spacing MULTIPLY. SAND_1 raises ROW_SPACING to 26 for the larger
    // artworks; at the old mean of 5 that is a fan of 81 wu, two of which
    // need 162 — check_position would overtake MIN_GALLERY_DISTANCE and the
    // 110 constant would go inert, spacing galleries FURTHER apart than
    // before. At mean 3 the fan is 46.5 and two need 93, so 110 remains the
    // live governor, which is the ruling.
    //
    // It also pays the layer ceiling: EXHIBITION_LAYERS 40 divided by 3 is
    // thirteen resident galleries against eight, and three snapshots spent
    // per site against five.
    //
    // σ 1 not 2: at mean 3 the old sigma put ~16% of draws under the
    // PAINTINGS_MIN clamp, which would pile the distribution on its floor.
    static constexpr float PAINTINGS_MEAN = 3.0f;
    static constexpr float PAINTINGS_SIGMA = 1.0f;
    static constexpr uint32_t PAINTINGS_MIN = 2;
    static constexpr uint32_t PAINTINGS_MAX_BY_ARCHETYPE[4] = { 8, 10, 12, 12 };

    // Layout: paintings share a facing direction, staggered in two rows.
    // Odd paintings step forward, even step back — the pawn walks between.
    static constexpr float ROW_SPACING = 26.0f;       // horizontal distance between paintings
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
    static constexpr float PAINTING_HALF = 13.0f;
    static constexpr float FAN_MARGIN    = 3.0f;

    static constexpr float MONO_TIER_CHANCE = 0.40f;  // 40% mono, 60% chronological

    // AN AREA DIAL, NOT A SCALE DIAL. scale_x = sqrt(area * aspect), so these
    // numbers move linear size by their square root: x4 here is x2 on the
    // canvas. That is why the floor moves by four and the ceiling by less —
    // "everything bigger, the smallest twice as big" compresses the range by
    // construction, and the compression is the ruling, not a side effect.
    //
    // Resulting widths, against ROW_SPACING 26: smallest work 3.0 -> 5.8 wu,
    // middle 7.6 -> 11.2, widest case (PANORAMIC at aspect 2.35, top of range,
    // top of jitter) 22.6 — leaving 3.4 wu of air, which is the clearance the
    // old row carried at its own ceiling.
    static constexpr float GALLERY_SIZE_LO = 3.4f;   // smallest gallery mean
    static constexpr float GALLERY_SIZE_HI = 5.0f;   // largest gallery mean
    // The authored monument's base canvas area — the analogue of
    // PAINTING_AREA for the photographer's tiers, which run 18–30 wu².
    // THE ONE PLACE THE MONUMENT PREMIUM IS STATED, and before SAND_2 it
    // had no place at all: the authored branch multiplied gallery_size_mean
    // into a HEIGHT off a bare 5.0f while snapshots took it into an AREA, so
    // the ratio between Jean's paintings and the photographer's was not a
    // number anyone had chosen — it ran 1.06x at the smallest sites to 3.75x
    // at the largest, times the painting's own aspect. 48 stands a painting
    // at 2.4x a MEDIUM snapshot's canvas, held at every size and aspect, and
    // reproduces the growth factor SAND_1 gave the snapshots (2.18x).
    static constexpr float AUTHORED_AREA = 48.0f;
    // A FRACTION OF THE SITE MEAN, NOT AN ABSOLUTE OFFSET (SAND_1) — see the
    // jitter site in commit_gallery. Additive, this was +-0.45 at every size:
    // a 1.6x spread of width on a small site and 1.14x on a large one, so
    // raising the floor would have made every work in a gallery the same
    // size. Fractional holds one character of variation at every scale.
    static constexpr float PAINTING_SIZE_SIGMA = 0.3f;   // per-painting jitter, FRACTIONAL

    // Snapshots join the pool once three exist — a one-photo gallery is a
    // diary page, not an exhibition. Read by gallery_available_staging and
    // by nothing else: it gates the SNAPSHOT pool, never the family.
    static constexpr uint32_t MIN_POOL_SIZE = 3;

    // Minimum distance between gallery centers (world units). Clearance
    // beyond the median fan (~50 wu half-span); the footprint registry is
    // the actual overlap guard, this is the spacing taste.
    static constexpr float MIN_GALLERY_DISTANCE = 110.0f;

    // Painting slots the wall path may not take. The pool is shared and the
    // walls always win by arriving first: place_wall_paintings runs once at
    // mood entry and takes everything it wants in one pass, while
    // commit_gallery arrives afterwards over many frames at
    // SPAWN_BUDGET_PER_FRAME.
    //
    // THE VALUE STANDS AND ITS DERIVATION DOES NOT (SAND_1). It read as a
    // GEOMETRIC BOUND rather than an expected occupancy because sixteen
    // galleries all rolling n=3 was a TAIL against a mean of 5 — on a
    // near-perfect grid at exactly the exclusion distance, under a spawner
    // that places randomly with rejection. At PAINTINGS_MEAN 3 that roll is
    // the MEDIAN, so sixteen resident sites reach 48 in the ordinary case and
    // MAX_GALLERIES would allow three times that. The sentence bounds
    // nothing now, and the 25-35 peak it concluded with was read off the
    // same expired mean.
    //
    // 48 survives on a different fact: an outdoor painting spends an
    // exhibition layer as well as a slot, so the exhibition array — not this
    // reserve — is the cap outdoor demand meets first. Its size and the
    // reason for that size live in state.hpp and are not repeated here.
    static constexpr uint32_t OUTDOOR_SLOT_RESERVE = 48;

    // ─── Content×Form Mixing ─────────────────────────────────
    //
    // OUTDOORS THE TWO COLLECTIONS STAND LEVEL: a painting and a photograph
    // are equally likely on the sand. The row said seventeen percent authored
    // once (four sites in five snapshot-only), then sixty-eight; this is the
    // ruling that settles it at one in two.
    //
    // THE FIFTY IS SYMMETRIC, NOT ARITHMETIC — it holds at every level rather
    // than only in the aggregate:
    //   · the two PURE bands are equal, 0.35 each;
    //   · the MIXED band between them is an even coin, so a mixed site is
    //     genuinely mixed and does not lean to either collection.
    // Read as a partition of the site roll: [0.00, 0.35) SNAPSHOT_ONLY,
    // [0.35, 0.65) MIXED, residual [0.65, 1.00) AUTHORED_ONLY. Expected
    // authored share of an outdoor painting is 0.35 + 0.30 x 0.50 = one half.
    //
    // The symmetry is also what makes the row safe to tune: while the two
    // pure bands stay equal and the mix stays 0.50, the aggregate is 0.50 for
    // ANY width of the mixed band — so MIXED can be widened or narrowed to
    // taste (how often a site blends the two) without touching the balance.
    //
    // Indoor is untouched, and deliberately unequal: a room is 83% authored
    // (WALL_ART's snapshot_only_share 0.15 + mixed_share 0.05 residual, and
    // mix_snapshot_chance 0.40 — that dial's sense is inverted from this
    // one). The rooms are the exhibition; the sand is where the diary meets
    // it.
    static constexpr float OUTDOOR_SNAPSHOT_ONLY = 0.35f;  // [0.00, 0.35)
    static constexpr float OUTDOOR_MIXED = 0.30f;  // [0.35, 0.65); residual AUTHORED_ONLY 0.35

    // In mixed mode: per-painting chance of being the AUTHORED content. An
    // even coin — the mixed site is the one that shows them side by side, so
    // it is the one place the balance must not lean.
    static constexpr float OUTDOOR_MIX_AUTHORED_CHANCE = 0.50f;  // chance each outdoor painting is authored

    // Photographer pacing by archetype. FLAT, AND DELIBERATELY SO (SAND_1).
    // The old row slowed the photographer to 1.5x trigger distance on basin
    // and pool while GALLERY_CHANCE put galleries there at 0.70/0.85 — the
    // program wanted exhibitions on sand and would not photograph it, which
    // is the whole of why the sand read as empty. 0.6 is below the old
    // fastest tier: one capture per ~15 wu walked everywhere, against 37.5
    // on sand before. A flat table is a dead mechanism and this one is kept
    // only until the even distribution is seen; if it is right, the row
    // folds into TRIGGER_DISTANCE_MEAN and this table goes.
    static constexpr float PHOTO_PACE_BY_ARCHETYPE[4] = { 0.6f, 0.6f, 0.6f, 0.6f };

    // Gallery center jitter (fraction of Dim::PATCH_EXTENT)
    static constexpr float POSITION_JITTER = 0.30f;
};

// Site content type (outdoor gallery)
struct GallerySiteType {
    static constexpr uint32_t SNAPSHOT_ONLY = 0;
    static constexpr uint32_t MIXED = 1;
    static constexpr uint32_t AUTHORED_ONLY = 2;
};

// What place says back. REFUSED is ground (containment, position, a
// full registry) and is final for this patch; DEFERRED is content and is
// the conductor's to retry.
enum class GalleryPlaceResult : uint32_t { PLACED, DEFERRED, REFUSED };

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
    //   0% → 1 wall, 0% → 2, 27.75% → 3, residual ~72.25% → 4
    //
    // THREE OR FOUR, ALWAYS (WALLS_1). The one- and two-wall rooms are gone,
    // not made rare: a room with two bare walls reads as unfinished rather
    // than as sparse, and 0.75% of rooms was never worth the shape. The
    // three-wall case STAYS at 27.75% and is deliberate — four full walls in
    // every room reads as a showroom, and the empty wall is what tells the
    // eye the other three were hung on purpose.
    //
    // This commit is NOT the fix for the empty rooms. It was already 99.25%
    // three-or-four; the rooms Jean saw were selected walls arriving with no
    // content. See COMMIT 2.
    /* wall_count_t1 */ 0.0f,
    /* wall_count_t2 */ 0.0f,
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

// THREE OR FOUR, PROVED (OVERTURE_0). WALLS_1 zeroed the one- and two-wall
// rooms; this keeps them zeroed. A dial that moved them would compile a shape
// the piece has ruled out — and it would do it silently, since a room with two
// bare walls looks exactly like a room that ran out of pictures.
static_assert(WALL_ART.wall_count_t1 == 0.0f && WALL_ART.wall_count_t2 == 0.0f,
    "a room hangs three or four walls; one- and two-wall rooms are not a shape");

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
    MOOD_TABLE[MOOD_INDOOR_FLAT].shape.finite_radius_max
        > MOOD_TABLE[MOOD_INDOOR_VAULT].shape.finite_radius_max
    ? MOOD_TABLE[MOOD_INDOOR_FLAT].shape.finite_radius_max
    : MOOD_TABLE[MOOD_INDOOR_VAULT].shape.finite_radius_max;

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
              <= MOOD_TABLE[MOOD_INDOOR_FLAT].shape.wall_height,
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
    // PURSE_0 R1 — when this capture FIRST wanted to fire, in TimeState
    // seconds; negative means "not waiting". One member, and it is the
    // whole state machine: a want that cannot be served yet remembers when
    // it started wanting, so the ceiling can be measured from it.
    float defer_since = -1.0f;
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

// ── Authored Staging (the ring — STAGING_LAYERS layers, walked by
//    authored_head; REPEAT_0 made the phrase true, and the old banner
//    said sixteen while Dim::STAGING_LAYERS said thirty-two) ──
struct AuthoredStagingRecord {
    uint32_t disk_index = UINT32_MAX;
    // WHICH PAINTING THIS SLOT'S TEXTURE ACTUALLY HOLDS (WALLS_3).
    //
    // `disk_index` is a CLAIM, written when the fetch is requested. The
    // texture still holds the outgoing image until the round trip lands, so
    // the two disagree for the whole flight. That was harmless while a
    // pending record was `valid = false` and unpickable; WALLS_2 made pending
    // records pickable ON PURPOSE — that is the fix — and the disagreement
    // became load-bearing.
    //
    // Everything that asks "which painting is on this wall" reads THIS.
    // Everything that asks "which painting has been spoken for" reads
    // disk_index. Both are true at once and neither can stand in for the
    // other.
    uint32_t shown_disk_index = UINT32_MAX;   // UINT32_MAX = slot holds no image
    float aspect_ratio = 1.0f;
    float uv_scale_x = 1.0f;
    float uv_scale_y = 1.0f;
    bool valid = false;
    // `consumed` STOOD HERE (REPEAT_0 R7). It said "on a wall NOW" and every
    // reader of it was a selector: the pool had to know what it had already
    // handed out, because the pool CHOSE. A playlist does not choose, so
    // there is nothing to mark — the head is spent by advancing past it, and
    // the next hang asks the cursor, not the array. Deleted with its two
    // companions below rather than left as a bit nobody sets.
    // EXHIBIT_0 — A REQUEST IS NOT A PICTURE. On the retired native twin
    // a load either fills this record or fails, inside one call; over the
    // network the two are separated by a round trip. `pending` names that
    // gap: the slot is spoken for (disk_index is already set, so the
    // rotation cursor will not hand the same painting to a second slot)
    // and it is NOT valid, so no gallery can pick it. Cleared on arrival
    // AND on failure — a slot that never clears is a slot the rotation
    // can never reuse.
    bool pending = false;
    // `hung_this_world` AND `exhibition_layer` STOOD HERE, and they were the
    // other two thirds of what OVERTURE_0 called THE CLAIM, WHOLE — three
    // fields written in one breath at each of the two claim sites. Both are
    // deleted at REPEAT_0 U6, and neither for its own sake:
    //
    //   · `hung_this_world` was the ROTATION'S flag, and it had exactly one
    //     reader — rotate_authored_staging, deleted at U5.
    //   · `exhibition_layer` mapped a layer back to the record that fed it,
    //     and it had exactly one reader — authored_release_layer, which
    //     existed to clear `consumed` and is deleted just below.
    //
    // The split WALLS_1 paid for was real and it was right: one flag could not
    // carry both "on a wall now" and "seen this world", and the outdoor leak
    // was the proof. What REPEAT_0 removes is not the distinction but the
    // QUESTION — a sequence never asks which of its entries have been played,
    // because the playhead is the answer.
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
    // THE PLAYHEAD AND THE PLAYLIST CURSOR — and between them they are the
    // WHOLE of the authored state (REPEAT_0 R1). `authored_head` is the ring
    // position the next hang pops; `authored_disk_cursor` is the manifest
    // index the vacated layer is refilled from. They advance together, in
    // one act, at the pop. Nothing else remembers which painting hangs
    // where, because nothing else needs to: the playlist advances, it does
    // not choose.
    //
    // `authored_write_cursor` stood between these two and is DELETED. It had
    // one writer — this fill — and zero readers anywhere in src/, tools/,
    // web/, docs/ or audit/. It was the rotation's book-keeping outliving
    // the rotation by two campaigns.
    uint32_t              authored_head = 0;
    uint32_t              authored_disk_cursor = 0;     // walks authored_disk_manifest
    uint32_t              authored_staged_count = 0;
    // ONE FILL PER SESSION, AND THIS IS ITS LATCH (OVERTURE_0). Read in two
    // places: the fill's own early return, and nowhere else since commit's
    // demotion died. It is NOT set when the manifest is still empty — that
    // "empty" means "the fetch has not landed", and latching on it would lock
    // the exhibition out for the session.
    bool                  authored_textures_loaded = false;
    std::vector<std::string> authored_disk_manifest;    // scanned lazily on first load, sorted numerically
    // WHICH ENTRIES HAVE NOTHING BEHIND THEM (REPEAT_0a R1). Sized and
    // cleared where the manifest is written, because they are two facts
    // about one thing. A fetch that fails kills its index FOR THE SESSION:
    // a picture that is not there this minute is not there in ten, and the
    // alternative is one failed round trip per ghost per lap, forever.
    //
    // THE COST OF BEING WRONG IS PRICED AND REVERSIBLE. If the failures are
    // transient rather than absences — a refused burst rather than a missing
    // file — this drops a live painting for the session. That is R4's
    // one-counter reversal, recorded in docs/OPEN.md and deliberately not
    // built: nothing has measured a real outage yet.
    std::vector<bool>        authored_manifest_dead;
    // Exhaustion is a LEGAL state (R3), and worth saying exactly once.
    bool                  authored_exhausted_logged = false;


    // ── EXHIBIT_0: the web twin's loading gap, named ────────────────
    // The native twin had no state here because its loads were calls
    // that returned with the picture. The web twin's do not, so the
    // two facts that only exist BETWEEN a request and its answer live
    // here: how many are out, and which ones are still waiting for a
    // free lane.
    struct AuthoredFetchRequest {
        uint32_t staging_layer;
        uint32_t disk_index;
        std::string url;
    };
    std::vector<AuthoredFetchRequest> authored_fetch_queue;
    uint32_t authored_fetch_inflight = 0;

    // ── AUBADE U5c — THE VALVE'S HOLDING PEN ────────────────────────
    //
    // A painting that has ARRIVED but has not yet been decoded, padded
    // and uploaded. Everything downstream of the fetch used to happen
    // inside the fetch's own completion callback — a browser event, not
    // a frame — which put a stb_image decode and a ~1 MiB WriteTexture
    // wherever the network happened to land them, first present very
    // much included.
    //
    // They wait here instead, and drain at ONE PER FRAME once the world
    // has actually been seen (pump_authored_valve). The bytes are the
    // COMPRESSED file: emscripten_fetch owns its buffer and frees it at
    // close, so holding it means copying it.
    //
    // THE COST, stated: at most STAGING_LAYERS (32) files at once, and
    // in practice only those that land before first present. At the 512
    // long-edge cap web_dist asserts, a painting is a few hundred KiB,
    // so the pen's ceiling is single-digit MiB and its steady state is
    // empty.
    struct AuthoredHeld {
        uint32_t staging_layer;
        uint32_t disk_index;
        std::string url;
        std::vector<unsigned char> bytes;
    };
    std::vector<AuthoredHeld> authored_held;
    // The manifest is requested once per session, at the earliest
    // instant a GalleryState exists (the cartridge constructor).
    bool authored_manifest_requested = false;
    // The manifest's absence is a true fact every time it is asked before the
    // fetch lands; it is only worth SAYING once (scan_paintings_folder).
    bool authored_absence_logged = false;

    // The occupancy array is the whole record. A companion count used to
    // ride beside it, incremented at every claim and decremented at every
    // free — and read by nothing, not even a log. Deleted rather than
    // reserved: if a later stage needs a population figure it can name its
    // reader when it introduces one.
    //
    // REPEAT_0a U3 IS THAT LATER STAGE, AND IT NAMES ITS READER: the pop line
    // and the evict line both carry `hung=`, counted from the array below.
    // The count is still not stored — it is derived at the two moments it is
    // said, which is the same ruling that deleted the companion.
    bool     exhibition_occupied[Dim::EXHIBITION_LAYERS]{};

    // WHICH PAINTING IS ON WHICH LAYER (REPEAT_0a R5). Before this, an
    // exhibition layer did not know what it held: the staging record that
    // knew was refilled by the very act that hung it, so the answer was
    // already gone one statement after the question could be asked.
    //
    // ONE FACT, ONE HOME, ONE WRITER — authored_pop, at the moment it hands a
    // layer over — and one clearing site, release_exhibition_layer. A layer
    // holding a snapshot, or nothing, reads UINT32_MAX and is never written
    // by the pop, which is why the snapshot path needs no line at all.
    //
    // IT IS A STRUCT AND NOT A BARE uint32_t ARRAY FOR ONE REASON: a bare
    // array value-initialises to ZERO, and zero is a legal disk index, so
    // forty layers would boot claiming to hold painting 0. A member
    // initialiser makes "nothing" the default — the same idiom, for the same
    // reason, as AuthoredStagingRecord's own UINT32_MAX fields.
    struct ExhibitionName { uint32_t disk_index = UINT32_MAX; };
    ExhibitionName exhibition_name[Dim::EXHIBITION_LAYERS]{};

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

    // THE DEFERRED HANG'S TALLY (OVERTURE_0): galleries this world dressed on
    // the retry rather than at their patch's spawn. Zeroed at teardown, so it
    // reads per world — which is the only scale the number means anything at.
    uint32_t deferred_dressed = 0;

    PendingSnapshot pending_snapshot;
};


// The indoor hang's three site types, plus ROLL — the sentinel that says
// "the seed decides", which is what every caller but the atrium wants
// (ATRIUM_3). It left place_wall_paintings' body to become a parameter.
enum class IndoorSiteType : uint32_t { SNAPSHOT_ONLY, MIXED, AUTHORED_ONLY };

// ═══ MODULE FUNCTIONS — DECLARATIONS ═════════════════════════════

// Per-frame
void update_photographer(GalleryState& gs, GalleryDeps* c, wgpu::Queue& queue);
void render_snapshot_pass(GalleryState& gs, GalleryDeps* c, wgpu::CommandEncoder& encoder);
// Outdoor lifecycle (three-phase)
bool select_gallery_for_patch(GalleryState& gs, MachineCtx* c,
    int32_t gx, int32_t gz, GallerySelection& sel);
GalleryPlaceResult place_gallery_from_selection(MachineCtx* c,
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
// ATRIUM_3 — the entrance's own hang: the controls image dead ahead on the
// sand, the next spots flanking it, the walls taking the rest. Deferred,
// because at boot the manifest is still a fetch in flight.
//
// ATRIUM_10 — and SPLIT IN THREE, one thing each, because the poster must
// not wait on the walls. The consumer drives them: the memory on a return,
// the poster the frame ATRIUM_0 lands, the walls when the folder settles.
void clear_wall_paintings(GalleryState& gs, GalleryDeps* c, wgpu::Queue& queue);
// Authored image loading
void load_authored_textures(GalleryState& gs);   // AUBADE U5b — fetch only; no device, so it may run before one exists
void tick_gallery_deferred_hang(MachineCtx* c, wgpu::Queue& queue);
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

// ONE ROAD OUT OF THE EXHIBITION (REPEAT_0a U2), beside the one road in.
// Three sites freed a layer before this existed — evict_paintings_for_patch,
// clear_wall_paintings and teardown_gallery — and each would have needed its
// own copy of the two facts and, at U3, its own copy of the line that says
// them. The occupancy bit and the painting's name are one event, so they get
// one home.
//
// A layer that held nothing, or held a snapshot, clears to the same state it
// was already in; the verb is idempotent and does not care which kind of
// content it is releasing.
// HOW MUCH OF THE EXHIBITION IS UP (REPEAT_0a U3, R6). Derived at the two
// moments it is said — the pop and the release — and deliberately not stored,
// which is the ruling that deleted the occupancy array's companion count. A
// scan of forty on a placement-event path, never per frame.
inline uint32_t authored_hung_count(const GalleryState& gs) {
    uint32_t n = 0;
    for (uint32_t i = 0; i < Dim::EXHIBITION_LAYERS; i++)
        if (gs.exhibition_name[i].disk_index != UINT32_MAX) n++;
    return n;
}

inline void release_exhibition_layer(GalleryState& gs, uint32_t exh) {
    if (exh >= Dim::EXHIBITION_LAYERS) return;
    const uint32_t was = gs.exhibition_name[exh].disk_index;
    gs.exhibition_occupied[exh] = false;
    gs.exhibition_name[exh].disk_index = UINT32_MAX;

    // THE OTHER HALF OF THE WINDOW, MOVING (R6). A painting leaving is the
    // event the console never had: pops counted up and nothing counted down,
    // so `hung` could only ever be inferred. SNAPSHOT LAYERS RELEASE SILENTLY
    // — they were never in the sequence, and a line about them would be a
    // line about the other exhibition.
    //
    // A world transition frees all forty layers, so a heavy room's teardown
    // says up to forty of these at once. That is intended and greppable, and
    // it is lighter than what stood here before REPEAT_0: the rotation era
    // spent up to 97 lines on the same boundary.
    if (was != UINT32_MAX)
        std::cout << "[Authored] Evict exh=" << exh
            << " disk=" << was
            << " hung=" << authored_hung_count(gs) << "\n";
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
// THE RING'S LENGTH — DERIVED, NEVER STORED (REPEAT_0 U1). One expression
// over two facts that are both already here, so there is no third fact to
// drift. Zero before the manifest lands, which makes every pop a no-op
// until there is something to play.
inline uint32_t authored_ring_size(const GalleryState& gs) {
    const uint32_t m = (uint32_t)gs.authored_disk_manifest.size();
    return m < Dim::STAGING_LAYERS ? m : Dim::STAGING_LAYERS;
}

// HOW MANY THE ROW CAN TAKE, WITHOUT TAKING ANY (REPEAT_0 U2). The count of
// records that are `valid && !pending` CONTIGUOUSLY from the head — it stops
// at the first that is not, because the row stops there too (R3). This is
// what every content gate asks now, and it replaces `authored_hangable` at
// each of them.
//
// CONTIGUOUS IS THE WHOLE WORD, AND IT IS WHY FOUR FETCH LANES ARE HARMLESS.
// R9 wanted one lane so arrivals would be FIFO; the constant has been 4 since
// OVERTURE_0 R-E and arrivals can land out of order. It does not matter,
// because the ORDER THAT MATTERS IS THE POP'S, not the fetch's: pops walk the
// ring, so the records still in flight are always the k most recently popped,
// which from the head are the LAST k positions. Out-of-order arrival can only
// make this number smaller than the true count of ready records, never
// larger, and a small number under-hangs a row — the already-legal thin room.
inline uint32_t authored_ready_depth(const GalleryState& gs) {
    const uint32_t ring = authored_ring_size(gs);
    uint32_t depth = 0u;
    while (depth < ring) {
        const auto& r = gs.authored_staging[(gs.authored_head + depth) % ring];
        if (!r.valid || r.pending) break;
        depth++;
    }
    return depth;
}

// THE CURSOR STEPS OVER THE DEAD (REPEAT_0a U1). The first live index at or
// after `from`, walking the manifest at most once; UINT32_MAX when a full lap
// finds nothing — which is EXHAUSTION, a legal state (R3) and not an error.
//
// It fails OPEN on a mask that is shorter than the manifest: an index the mask
// cannot speak for reads as live, which is the pre-campaign behaviour and
// costs one failed round trip rather than silently deleting a painting. The
// mask is sized with the manifest, so that arm is unreachable — it is there
// because "unreachable" is the word that gets campaigns written.
inline uint32_t authored_next_live(const GalleryState& gs, uint32_t from) {
    const uint32_t n = (uint32_t)gs.authored_disk_manifest.size();
    if (n == 0u) return UINT32_MAX;
    uint32_t i = from % n;
    for (uint32_t step = 0; step < n; step++) {
        if (i >= (uint32_t)gs.authored_manifest_dead.size()
            || !gs.authored_manifest_dead[i]) return i;
        i = (i + 1u) % n;
    }
    return UINT32_MAX;
}

// EXHAUSTION SPEAKS ONCE (R3). Every entry dead is the "no paintings folder"
// state reached by a different road: the ring stops appending, popped layers
// go invalid, rows end, the deferred hang idles. No reload logic, no timer.
inline void authored_report_exhausted(GalleryState& gs) {
    if (gs.authored_exhausted_logged) return;
    gs.authored_exhausted_logged = true;
    std::cout << "[Authored] Sequence exhausted — every entry is dead\n";
}

// THE ONE PLACE THE PLAYLIST STEPS FORWARD (REPEAT_0a U1). Takes the next
// LIVE index and advances the cursor past it; UINT32_MAX when the sequence is
// exhausted, and then the caller issues no fetch.
//
// FOUR CALLERS CONSUMED THE CURSOR BEFORE THIS EXISTED, NOT THE THREE THE
// ORDER NAMES — the boot fill, the pop's append, the pop's HOLE-HEAL, and the
// failure path's re-append. The hole-heal is the one no unit named, and a
// patch applied to "the three writers" would have left it handing dead indices
// to the ring forever. One verb, four callers, no fourth to forget.
inline uint32_t authored_take_next(GalleryState& gs) {
    const uint32_t n = (uint32_t)gs.authored_disk_manifest.size();
    if (n == 0u) return UINT32_MAX;
    const uint32_t idx = authored_next_live(gs, gs.authored_disk_cursor);
    if (idx == UINT32_MAX) { authored_report_exhausted(gs); return UINT32_MAX; }
    gs.authored_disk_cursor = (idx + 1u) % n;
    return idx;
}

// THE POP-AND-APPEND — the playlist's whole verb, and after this campaign the
// ONLY authored supply verb. Defined beside the fill, because it is the fill
// continued by one layer at a time. See its body for the contract.
inline uint32_t authored_pop(GalleryState& gs, uint32_t exh);

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
        // ── THE PHOTOGRAPHER WAITS FOR HEADROOM (PURSE_0 R1) ──────────
        //
        // A capture is a ~10 ms render pass. Fired into a frame with no
        // headroom it IS the hitch the audience feels once a second — the
        // laptop capture measured +10 ms every 1-2 s, and that is the
        // second of the three residual stutters this round takes.
        //
        // THE GATE IS THE PRESENTATION LAW'S OWN VERDICT, not an estimate
        // of it: g_served_k is the k the law computed for the last frame
        // (console.hpp), 1 meaning that frame made its refresh. A second
        // estimator would be a second opinion about presentation, and the
        // law is the program's only one.
        //
        // THE CEILING IS BOUNDED STARVATION, DELIBERATELY. A machine that
        // never has headroom must still fill its pool, only slower; past
        // PHOTO_DEFER_MAX_S the capture fires regardless. On a machine
        // with real headroom the rule is invisible — k is 1 nearly always,
        // so nearly every capture fires on the frame it wanted to. On one
        // without, the capture hitch falls from ~1/s to ~1 per ceiling.
        //
        // HONEST LIMIT: THE RULE SCHEDULES THE COST, IT DOES NOT SHRINK
        // IT. A +10 ms pass on a 0-headroom frame still drops that frame.
        // What it buys is that the pass lands on frames that can afford
        // it, and that the ones that cannot are not made worse.
        const float now = c->time_state_.seconds;
        if (gs.photographer.defer_since < 0.0f) gs.photographer.defer_since = now;
        const bool headroom = (t7::g_served_k == PhotographerCaptureConfig::PHOTO_HEADROOM_K);
        const bool ceiling  = (now - gs.photographer.defer_since)
                              >= PhotographerCaptureConfig::PHOTO_DEFER_MAX_S;
        if (!headroom && !ceiling) return;   // keep wanting; nothing else advances

        gs.photographer.defer_since = -1.0f;
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
    // THE EXHIBITION GUARD, CLOSED (PURSE_0 R3). This was the file's own
    // standing TODO — "Autonomous stdout — exhibition-guard candidate,
    // still open" — and it closes on the same flag and for the same reason
    // the `[Gallery] slot=` line below already uses: a capture happens on
    // the travel path, so this is steady-state chatter under a rider. The
    // laptop bookend measured captures every 1-2 s; that is 1-2 blocking
    // console writes a second in the shipped frame, for an event NOBODY
    // SEES. Nothing is deleted — the lab build still narrates the pool.
    if constexpr (t7::INSTRUMENTS.stream_witness) {
        std::cout << "[Photographer] Capture -> layer " << layer
            << " (" << shot_names[static_cast<uint32_t>(shot)] << ")"
            << " aspect=" << aspect_ratio
            << " pool=" << gs.snapshot_count << "/" << Dim::STAGING_LAYERS << "\n";
    }
}

// ═══ GALLERY SITES (outdoor — three-phase) ═══════════════════════

// ── select_gallery_for_patch ──

inline bool select_gallery_for_patch(GalleryState& gs, MachineCtx* c, int32_t gx, int32_t gz, GallerySelection& sel) {
    // NO CONTENT GATE HERE (OVERTURE_0). Content is PLACE's question and has
    // been since SPAWN_4 gave place the reservation; the snapshot floor that
    // stood at the head of this function was the snapshot era's duplicate of
    // it, and it was the reason the boot ring was born gallery-less — at the
    // first stream_patches the photographer has shot nothing, so every patch
    // in the priority window refused before it ever rolled. The floor now
    // lives in the pool arithmetic (gallery_available_staging), where it
    // governs the snapshot pool alone instead of the whole family.

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

// THIS IS THE POOL (OVERTURE_0), not a mirror of one. It used to describe
// itself as commit's availability test minus two things place cannot see —
// which was true, and which let a subtly different number live at each end.
// Now the two ends ask ONE question, and the residual caps commit still
// applies (the mono-tier curation, a per-gallery seed filter; the exhibition
// layer allocator) can only ever REDUCE, so this stays an upper bound and the
// fan radius errs large rather than small.
//
// Two things the arithmetic says that the old spelling did not:
//   · SNAPSHOTS COUNT ONLY ONCE THE FLOOR IS MET. MIN_POOL_SIZE moved here
//     from the head of select. Below three photographs the snapshot pool is
//     zero — which is what makes a boot patch resolve to the authored pool
//     rather than refuse the family outright.
//   · THE AUTHORED POOL IS WHAT COMMIT CAN ACTUALLY HANG. That used to be
//     `valid && !consumed` over all 32 records; since REPEAT_0 it is the
//     READY DEPTH — how many the playlist can hand out before it hits a
//     pending head. authored_staged_count is still a tally of `valid` alone
//     and still answers a different question (is there an exhibition at
//     all), which is why the READY floor keeps reading it and this does not.
//
//     THE RESERVATION IS A COUNTER HELD AGAINST THAT DEPTH, and it stays one:
//     staging_reserved names no record, only a number, so nothing here had to
//     learn about the ring (G5).
inline uint32_t gallery_available_staging(const GalleryState& gs, uint32_t site_type) {
    uint32_t snaps = 0;
    if (gs.snapshot_count >= GalleryConfig::MIN_POOL_SIZE)
        for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++)
            if (gs.snapshot_staging[i].valid && !gs.snapshot_staging[i].consumed) snaps++;
    const uint32_t auth = authored_ready_depth(gs);   // contiguous from the head — what commit hangs
    uint32_t pool = (site_type == GallerySiteType::SNAPSHOT_ONLY) ? snaps
                  : (site_type == GallerySiteType::AUTHORED_ONLY) ? auth
                  : snaps + auth;
    return pool > gs.staging_reserved ? pool - gs.staging_reserved : 0u;
}

inline GalleryPlaceResult place_gallery_from_selection(MachineCtx* c, const GallerySelection& sel, GalleryPlacement& plan) {
    auto& gs = c->gallery_state_;

    // THE ROLL IS A PREFERENCE; THE POOL DECIDES (OVERTURE_0). A rolled pool
    // that is empty resolves to the other pool if that one has content. MIXED
    // reads their sum, so an empty MIXED means both are empty and there is no
    // other to try. Both empty is not a refusal — it is a patch waiting for
    // an exhibition that has not landed yet, and the conductor retries it.
    uint32_t site  = sel.site_type;
    uint32_t avail = gallery_available_staging(gs, site);
    if (avail == 0 && site != GallerySiteType::MIXED) {
        const uint32_t other = (site == GallerySiteType::AUTHORED_ONLY)
            ? GallerySiteType::SNAPSHOT_ONLY : GallerySiteType::AUTHORED_ONLY;
        const uint32_t a2 = gallery_available_staging(gs, other);
        if (a2 > 0) { site = other; avail = a2; }
    }

    // THE RESERVATION. The count is resolved HERE, against content, so the
    // radius below describes the gallery that will actually be built. Commit
    // draws from this instead of discovering scarcity after the ground is
    // already claimed.
    const uint32_t reserved = sel.painting_count < avail ? sel.painting_count : avail;
    if (reserved == 0) return GalleryPlaceResult::DEFERRED;   // no content of either kind — the patch waits

    // ONE extent, used for both questions, and they are the same question.
    // The old code passed the SAME VARIABLE for footprint and containment too
    // — that was never the defect. The defect was that the variable came from
    // the archetype maximum. Ground claim and containment are both "how far
    // does this fan reach", and for a fan of realized count that is one number.
    const float footprint_r = gallery_fan_radius(reserved);

    float cx = sel.cx, cz = sel.cz;
    if (!indoor_bounds_clamp(c, PopFamily::GALLERY, footprint_r, footprint_r, cx, cz))
        return GalleryPlaceResult::REFUSED;
    if (!check_position(c, cx, cz, footprint_r, PopFamily::GALLERY))
        return GalleryPlaceResult::REFUSED;

    int32_t host_gx = (int32_t)std::floor(cx / Dim::PATCH_EXTENT);
    int32_t host_gz = (int32_t)std::floor(cz / Dim::PATCH_EXTENT);

    if (register_footprint(c, cx, cz, footprint_r,
        host_gx, host_gz, PopFamily::GALLERY, sel.slot, sel.archetype) == UINT32_MAX)
        return GalleryPlaceResult::REFUSED;

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
    plan.site_type = site;   // the RESOLVED one, not the rolled one

    return GalleryPlaceResult::PLACED;
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

    // COMMIT NEITHER FILLS NOR DEMOTES (OVERTURE_0). The fill lives at the
    // conductor's deferred-hang head — one call, one home — and the demotion
    // that stood here is unreachable once place resolves: an unlatched fill
    // means the ready depth is zero, and place has already resolved to the
    // other pool or DEFERRED the patch on exactly that fact. The site type
    // arrives resolved.
    uint32_t site_type = plan.site_type;

    // Snapshot candidates
    struct Candidate { uint32_t layer; };
    Candidate candidates[Dim::STAGING_LAYERS];
    uint32_t candidate_count = 0;
    for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) {
        if (gs.snapshot_staging[i].valid && !gs.snapshot_staging[i].consumed)
            candidates[candidate_count++] = { i };
    }

    // THE RESIDUAL ZERO-CONTENT ABORTS. SPAWN_4's reservation removed the
    // dominant path and OVERTURE_0 removed the rest of it: place resolves the
    // rolled site type against the pool and DEFERS when neither pool has
    // anything, so arriving here with the wrong site type is no longer a
    // shape. These three survive for the one reason the reservation
    // structurally cannot cover — the mono-tier curation below is a
    // per-gallery seed-derived filter that only commit can apply, and it can
    // empty a candidate list place counted as full. Each releases the ground
    // place claimed.
    bool have_snapshots = candidate_count > 0;
    bool have_authored = authored_ready_depth(gs) > 0;
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
    //
    // THE AUTHORED CEILING IS THE READY DEPTH, NOT authored_staged_count
    // (OVERTURE_0, repointed by REPEAT_0). The tally counts every VALID
    // record, a wall's included, so it answered "how many pictures are
    // staged" where this line asks "how many can this gallery hang". The
    // difference widened row_start — the fan was laid out for paintings that
    // were never coming, and the row read off-centre by half a ROW_SPACING for
    // every record it wrongly counted. Ready depth asks the exact question and
    // is one walk from the head rather than a scan of 32.
    uint32_t painting_count = plan.reserved_count;
    uint32_t max_available = candidate_count + authored_ready_depth(gs);
    if (site_type == GallerySiteType::SNAPSHOT_ONLY) max_available = candidate_count;
    if (site_type == GallerySiteType::AUTHORED_ONLY) max_available = authored_ready_depth(gs);
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
    // `usedAuthored[Dim::STAGING_LAYERS]` stood here and is DELETED
    // (REPEAT_0 U4). It existed to stop one commit picking the same record
    // twice; the playlist cannot, because each pop advances the head before
    // the next iteration asks. The depth read below is live for the same
    // reason — unlike the indoor hang there is no plan pass to count forward
    // through, so every question is asked of the ring as it stands.

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

        if (use_authored && authored_ready_depth(gs) == 0) {
            use_authored = false;
        }
        if (!use_authored && snap_cursor >= candidate_count) {
            if (authored_ready_depth(gs) > 0) {
                use_authored = true;
            }
            else {
                break;
            }
        }

        auto& s = gs.painting_slots[slot];
        bool placed_this = false;

        if (use_authored) {
            // THE LAYER FIRST, THEN THE POP (U2's ordering law). Two selectors
            // stood here — pick_authored_staging and a hand-rolled twin of it
            // masked by usedAuthored[] — and both are gone. The playlist does
            // not choose; it advances.
            uint32_t exh = find_free_exhibition_layer(gs);
            if (exh == UINT32_MAX) break;

            const uint32_t auth_stg = authored_pop(gs, exh);
            // A PENDING HEAD FALLS THROUGH TO THE SNAPSHOT, not out of the row
            // (R3): `exh` was never marked occupied, so the snapshot branch
            // below claims the same layer one line later and nothing leaks.
            if (auth_stg == UINT32_MAX) { use_authored = false; }

            if (use_authored) {
                const auto& img = gs.authored_staging[auth_stg];
                float jitter = (cpu_hash_f(p_seed, GalleryPaintingProp::SIZE_JITTER_A)
                    + cpu_hash_f(p_seed, GalleryPaintingProp::SIZE_JITTER_B)
                    + cpu_hash_f(p_seed, GalleryPaintingProp::SIZE_JITTER_C) - 1.5f) * GalleryConfig::PAINTING_SIZE_SIGMA;
                float size_mult = std::max(0.5f, gallery_size_mean * (1.0f + jitter));
                float height = std::sqrt((GalleryConfig::AUTHORED_AREA * size_mult) / img.aspect_ratio);

                fill_slot_wall_frame(s,
                    paint_x, 0.0f, paint_z,
                    face_x, 0.0f, face_z,
                    img.aspect_ratio, height,
                    exh, ContentSource::AUTHORED,
                    img.uv_scale_x, img.uv_scale_y,
                    FRAME_AUTHORED, gx, gz);
                s.geometry_seed = cpu_hash_f(p_seed, GalleryPaintingProp::GEOMETRY_SEED);

                gs.exhibition_occupied[exh] = true;
                // NO CLAIM IS WRITTEN — the outdoor twin of U3's cut. The
                // triad OVERTURE_0 called "THE CLAIM, WHOLE" dies with
                // selection (R7): there is no pool to mark, no rotation to
                // signal, and no record to hand back.
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
            float size_mult = std::max(0.5f, gallery_size_mean * (1.0f + jitter));
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
    else if constexpr (t7::INSTRUMENTS.stream_witness) {
        // The exhibition guard, closed at RIBBON_4: a gallery is placed on the
        // patch-spawn path, so this is steady-state chatter under a rider.
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

            // Free the exhibition layer. The record that fed it is NOT
            // released — there is nothing to release since REPEAT_0 R7, and
            // the leak OVERTURE_0 closed here cannot reopen: it was a pool
            // walking down to nothing as patches churned, and the playlist
            // has no pool to walk down.
            uint32_t exh = gs.painting_slots[i].texture_layer;
            release_exhibition_layer(gs, exh);

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
        // LOOM_2 pass head: WORLD + FRAME are every pipeline's strata 0/1.
            { compute.SetBindGroup(0, c->gpuState_.world_group());
          compute.SetBindGroup(1, c->gpuState_.frame_c_group()); }
        c->renderer_.dispatch_compute_photographer_vp(
            compute, c->gpuState_.photo_k_state_group(), c->gpuState_.photo_k_textures_group()
        );
        compute.End();
    }

    // CHORD_3 — the photographer's instance of the render-frame block.
    // Same law as the main camera's (render_passes.hpp, dispatch_compute):
    // the kernel just above is the sovereign writer, the pass boundary is
    // the ordering, and the snapshot pass below reads frame_r.vp /
    // frame_r.camera through the photographer bind group.
    c->gpuState_.encode_frame_r_photo_sync(encoder);

    // Only render the snapshot if a capture is pending
    if (!gs.pending_snapshot.active) return;
    gs.pending_snapshot.active = false;
    uint32_t layer = gs.pending_snapshot.target_layer;
    // The capture's SECOND line, gated with its first (PURSE_0 R3): one
    // event, two writes, and the pass this announces is the ~10 ms one
    // Commit B just taught to wait for headroom. Printing about it from
    // inside the frame it costs is the instrument measuring its own noise.
    if constexpr (t7::INSTRUMENTS.stream_witness) {
        std::cout << "[Photographer] Rendering snapshot -> layer " << layer << "\n";
    }

    // DOMESDAY_2 B10 — the msaa arm mirrors the main pass: render into
    // the multisampled target, resolve into the single-sample offscreen
    // color the portfolio copy chain reads, discard the samples. With
    // msaa=1 every field is byte-identical to the pre-B10 descriptor.
    wgpu::RenderPassColorAttachment colorAtt{};
    colorAtt.view = c->gpuState_.offscreen_color_view();
    colorAtt.loadOp = wgpu::LoadOp::Clear;
    colorAtt.storeOp = wgpu::StoreOp::Store;
    if (c->gpuState_.offscreen_msaa_color_view()) {
        colorAtt.view = c->gpuState_.offscreen_msaa_color_view();
        colorAtt.resolveTarget = c->gpuState_.offscreen_color_view();
        colorAtt.storeOp = wgpu::StoreOp::Discard;
    }
    colorAtt.clearValue = { (double)c->clearColor_[0], (double)c->clearColor_[1], (double)c->clearColor_[2], 1.0 };

    wgpu::RenderPassDepthStencilAttachment depthAtt{};
    depthAtt.view = c->gpuState_.offscreen_depth_view();
    depthAtt.depthLoadOp = wgpu::LoadOp::Clear;
    // DISCARD_0 (PASS_0 F2) — the twin of the main pass's depth, same
    // proof. offscreenDepthTexture_ is created with usage
    // RenderAttachment ALONE (state.hpp): no TextureBinding, no
    // CopySrc, so nothing can sample it and nothing copies it out. The
    // colour attachment above stays Store and MUST — it carries
    // CopySrc and this very function's tail copies it into the
    // snapshot staging array, right after pass.End(). Depth has no
    // such consumer.
    depthAtt.depthStoreOp = wgpu::StoreOp::Discard;
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
    // Group 1 carries the shadow_slot dynamic seat, so the bind passes
    // one offset; the photographer's pipelines never read it.
    pass.SetBindGroup(0, c->gpuState_.world_group());
    pass.SetBindGroup(1, c->gpuState_.frame_photographer_group(), 1, &kFrameSlotZero);
    pass.SetBindGroup(2, c->gpuState_.scene_state_group());   // B5 (R2): the one scene group
    pass.SetBindGroup(3, c->gpuState_.scene_textures_group());

    // THE PHOTOGRAPH IS TAKEN AT LOD1 (PANORAMA_1 U5). This one draw was the
    // whole `snapshot_pass` maximum — 16-55 ms on the laptop, 19-25 on the
    // Pixel, fired every ~30 wu walked and in bursts of up to four: the FULL
    // window (render_patch_count, 147 patches) at FULL mesh, ~2.4 M triangles,
    // uncalled by any frustum, for a 512x512 image of 262k pixels. The
    // photograph was buying a triangle for every ninth pixel.
    //
    // LOD1 is the same terrain at a quarter of the triangles, and it is the
    // band the SHADOW pass has always drawn the whole ring at — so this is not
    // a new level of detail, it is the one the tree already trusts for a
    // second view of the same world. At 512 px the difference is sub-pixel
    // over most of the frame; Jean's gate is two photographs at the size they
    // hang at.
    //
    // The pose is untouched: the photographer's VP, the entity table below and
    // both painting draws are exactly as they were. Only the terrain's index
    // band moves. D-2 — the photographer's own cull window and the two-frame
    // composite — waits behind that stamp.
    c->renderer_.draw_patch_terrain_direct(pass,
        c->gpuState_.scene_state_group(),
        c->gpuState_.scene_textures_group(),
        c->gpuState_.visible_patch_indices_buffer(),   // B3: slot-0 bound, attribute unread (direct variant)
        c->gpuState_.patch_index_buffer_lod1(),
        c->gpuState_.patch_index_count_lod1_live(),
        c->world_state_.render_patch_count);

    // The drawable table — snapshot members, canonical order (the photographer's
    // own entity group). Terrain above is the per-pass FORK (a single direct
    // draw over render_patch_count, no LOD split). Zone is not a snapshot member.
    DrawBind b{ /*shadow=*/false, /*ribbon_bit=*/true };
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
    pass.SetBindGroup(2, c->gpuState_.gallery_state_group());
    pass.SetBindGroup(3, c->gpuState_.gallery_textures_group());
    }
    c->renderer_.draw_wall_paintings(
        pass,
        c->gpuState_.draw_ledger_buffer(),
        GPUState::draw_record_offset(GPUState::DR_WALL)
    );
    c->renderer_.draw_gallery_frames(
        pass,
        c->gpuState_.draw_ledger_buffer(),
        GPUState::draw_record_offset(GPUState::DR_GALLERY_FRAME)
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
// BYTES COME FROM. Native walked a directory and read files; the web
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
    // AUBADE U1 — the decode window opens here. The stb decode itself
    // happened in the caller; what this body costs is the SCALE and the
    // PAD, which is main-thread pixel work by any name and belongs in the
    // same number.
    const auto t_decode0 = std::chrono::steady_clock::now();
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
    // SHOWN CATCHES UP TO CLAIMED, in the same instant `valid` becomes true
    // (WALLS_3). A reader that saw one without the other would see a valid
    // slot showing nothing, or a slot showing a picture it no longer holds.
    rec.shown_disk_index = rec.disk_index;

    // AUBADE U1 — HOW MUCH OF THE DARK WAS PAINTING DECODE. R6 found the
    // authored path decodes with stb_image ON THE MAIN THREAD, so this is
    // the number that separates "the paintings did it" from a device-side
    // wait. Summed only before first present; after it, the question has
    // been answered and the add stops.
    if (!t7::aubade_presented()) {
        t7::aubade_stb() += std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - t_decode0).count();
    }

    // AUBADE U1 — THE SIXTH IS THE ONE THAT MATTERS. Six staged images is
    // the READY floor (OVERTURE_0 U9), so `authored6` is the mark that says
    // when the offer could have been made. Counted here, at the moment a
    // slot becomes valid, because that is what "staged" means.
    {
        static uint32_t staged = 0;
        if (++staged == 6u) t7::aubade_mark("authored6");
    }
    std::cout << "[Authored] Scaled → " << dst_w << "x" << dst_h
        << " (aspect " << rec.aspect_ratio << ")\n";
}


// ═══ THE EXHIBITION ARRIVES OVER THE NETWORK ═════════════════════

// The manifest the dist script writes beside the program. NOT
// manifest.json — that name stays reserved for the PWA web manifest.
inline constexpr const char* EXHIBITION_MANIFEST_URL = "exhibition.json";
// The folder the manifest's bare filenames hang under. One place
// decides the layout; tools/web_dist.py writes to the same one.
inline constexpr const char* EXHIBITION_PAINTINGS_DIR = "paintings/";
// ATRIUM_3 — the entrance's own folder, written by tools/web_dist.py from
// assets/atrium. The manifest names files; the program joins the folder.

// FOUR PAINTINGS IN THE AIR, SIZED FOR THE BOOT (OVERTURE_0). The ring is
// born before its exhibition arrives and dresses from the first arrivals, so
// what this number really sets is how long the first field stands bare. Four
// lanes reach the exhibition floor in two round trips instead of six.
//
// The cost is bounded and small: each arrival is one 512-px JPEG decode
// (PAINTING_CAP) and one 1 MiB WriteTexture, a few ms, and four of them in
// one event turn is still a few ms. They do not even land together — four
// round trips do not answer in lockstep.
//
// ONE LANE WAS A FIREFOX MEASURE, NOT A GENERAL ONE. ORGAN_8 P3 paced the
// uploads to a trickle because Firefox's WebGPU serves them from blocks it
// sub-allocates and a burst opens blocks the trickle never lets empty
// (OPEN.md, FIREFOX STAGING RATCHET). Firefox is HELD at the fallback card:
// the lane count is sized for the browsers that can run the piece, and the
// pacing question is re-owed the day Firefox returns.
inline constexpr uint32_t AUTHORED_FETCH_INFLIGHT_CAP = 4;

// A request that never answers holds its lane forever, so every request
// is given an end. Generous, because a phone on a slow connection
// fetching a half-megabyte JPEG is the normal case this must not kill.
inline constexpr unsigned long AUTHORED_FETCH_TIMEOUT_MS = 30000;

// The fetch's own copy of everything the answer will need. The pointer
// outlives every fetch by construction: GalleryState is a member of the
// Cartridge, which is a member of App, heap-allocated in main() and never
// destroyed on this twin.
//
// AUBADE U5b — THE GPU IS NO LONGER IN HERE, and that absence is the
// unit. The fetch used to carry a GPUState* and a queue because its
// completion callback decoded and uploaded on the spot; U5c moved that
// downstream of first present, so nothing on this path touches the
// device — which is what lets the whole path START BEFORE THE DEVICE
// EXISTS, at manifest scan, so the bytes travel during the device and
// init window instead of after it.
struct AuthoredFetchCtx {
    GalleryState* gs;
    uint32_t      staging_layer;
    uint32_t      disk_index;
    std::string   url;
};

inline void pump_authored_fetches(GalleryState& gs);
// THE CLAIM AND THE QUEUEING, WITHOUT THE PUMP. Split out of
// load_authored_image_to_staging at REPEAT_0 U7 so the failure path can
// append without re-entering a pump that may be its own caller. See the
// helper's body for the whole of the distinction.
inline void authored_enqueue_fetch(GalleryState& gs, uint32_t staging_layer,
    uint32_t disk_index, const char* path);

// authored_staged_count is a tally of valid records, and on this twin
// records become valid at arrival time rather than at call time — so
// the tally is RECOMPUTED where it changes, never incremented at a
// call site that cannot yet know the answer.
inline void recount_authored_staged(GalleryState& gs) {
    gs.authored_staged_count = 0;
    for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++)
        if (gs.authored_staging[i].valid) gs.authored_staged_count++;
}


// A FAILED FETCH DROPS ITS ENTRY AND HEALS THE HOLE IN PLACE (REPEAT_0 R4).
//
// WHY THE HOLE IS FATAL AND NOT MERELY UNTIDY. Native's failure was final and
// harmless — the file was on disk or it was not. A network failure is a
// different animal, and under the playlist it is worse than it was under the
// rotation: a layer left {valid=false, pending=false} is an INVALID HEAD when
// the playhead reaches it, and an invalid head ends every authored row for the
// rest of the session (R3). The rotation could revisit such a slot at the next
// world change. There is no next-world re-ask any more, so the cure has to
// happen here, at the failure, or not at all.
//
// SO THE ENTRY IS DROPPED AND THE NEXT ONE TAKES ITS PLACE. The claim falls
// back to what is actually SHOWN — WALLS_3's fix, kept: this used to blank the
// slot, which threw away the picture it was still holding, and on venue wifi
// that was a bare wall for the rest of the world. Then the cursor advances and
// its entry is appended into this same layer. The painting that slipped is not
// retried now; it comes back on the next lap, which is exactly what a playlist
// does with a track it could not read.
//
// IT APPENDS WITHOUT PUMPING, AND THAT IS LOAD-BEARING. One of this function's
// four callers is start_authored_fetch's synchronous-failure arm, which runs
// INSIDE pump_authored_fetches. Appending through the pumping helper would
// re-enter that loop from underneath itself, and against an environment where
// emscripten_fetch fails on every call that is unbounded recursion, not a
// retry. Queueing alone cannot recurse: the pump's own while-loop takes the
// new request on its next turn, and every other caller reaches the pump
// through authored_fetch_finish a line later.
//
// AND `pending` COMES DOWN HERE. It used to be cleared by each caller
// afterwards (AUBADE U5c), which cannot work now — the append would find the
// slot still spoken for and decline. One home for "the journey ended badly",
// and the three callers that cleared it by hand no longer do.
inline void authored_fetch_release_slot(GalleryState& gs, uint32_t staging_layer, long status) {
    if (staging_layer >= Dim::STAGING_LAYERS) return;
    auto& rec = gs.authored_staging[staging_layer];
    const uint32_t slipped = rec.disk_index;

    rec.pending = false;
    rec.disk_index = rec.shown_disk_index;
    rec.valid = (rec.shown_disk_index != UINT32_MAX);

    const uint32_t manifest_size = (uint32_t)gs.authored_disk_manifest.size();
    if (manifest_size == 0u) return;   // nothing to append from

    // R1 — THE ENTRY IS DEAD FOR THE SESSION, AND THIS IS THE ONLY PLACE THAT
    // SAYS SO. All four failure paths reach here — the HTTP error, the empty
    // body, the fetch that would not start, and the decode the bytes lost — so
    // one line covers every way a painting can fail to arrive.
    //
    // `slipped` IS the failing index at all four, and the pending gate is the
    // proof: authored_enqueue_fetch declines while a layer is pending, and
    // `pending` spans fetch AND valve (AUBADE U5c), so nothing can re-aim
    // rec.disk_index between the request and any of these returns. Guarded
    // against UINT32_MAX and against an index the mask cannot speak for.
    if (slipped != UINT32_MAX
        && slipped < (uint32_t)gs.authored_manifest_dead.size()
        && !gs.authored_manifest_dead[slipped]) {
        gs.authored_manifest_dead[slipped] = true;
        std::cout << "[Authored] Dead disk=" << slipped
            << " (HTTP " << status << ") — dropped for the session\n";
    }

    // THE HEAL NOW ASKS FOR A LIVE INDEX, and the dead mask is what bounds it.
    // REPEAT_0 U7c bounded this cycle with a consecutive-slip budget because
    // one request per failure could otherwise run for the life of the session
    // against a dead origin. R2 supersedes that: fetches are issued ONLY for
    // live indices and each failure kills exactly one, so the session's
    // failing fetches are bounded by the manifest itself and the budget has
    // nothing left to bound. The proof and its one honest caveat are in the
    // commit message.
    const uint32_t next = authored_take_next(gs);
    if (next == UINT32_MAX) return;    // exhausted (R3) — the layer stays invalid

    // Autonomous stdout — the slip is named once, and it names both halves:
    // which painting was lost and which one took its place in the ring.
    std::cout << "[Authored] Slipped disk=" << slipped
        << " (HTTP " << status << "), appended disk=" << next
        << " into layer " << staging_layer << "\n";

    authored_enqueue_fetch(gs, staging_layer, next,
        gs.authored_disk_manifest[next].c_str());
}

// One exit for both outcomes: the lane is freed, the context is
// destroyed, and the queue is pumped so the next painting starts the
// instant this one is done with its lane.
// AUBADE U5c — `pending` NOW SPANS THE WHOLE JOURNEY, fetch AND valve,
// so this function no longer clears it. It used to, because this was
// where a painting finished; with the valve the picture is still owed
// until the upload lands, and a slot cleared early is a slot the fill
// and the rotation would both hand to a second painting. Every caller
// clears `pending` at the point where the journey really is over: the
// two failure paths here and now, the valve when the texture is up.
inline void authored_fetch_finish(AuthoredFetchCtx* ctx) {
    GalleryState& gs = *ctx->gs;
    if (gs.authored_fetch_inflight > 0) gs.authored_fetch_inflight--;
    delete ctx;
    recount_authored_staged(gs);
    pump_authored_fetches(gs);
}

inline void authored_image_onsuccess(emscripten_fetch_t* fetch) {
    AuthoredFetchCtx* ctx = (AuthoredFetchCtx*)fetch->userData;

    // ══ AUBADE U5c — THE VALVE: THE BYTES ARRIVE, NOTHING ELSE HAPPENS ══
    //
    // What used to happen here, inside a browser event: stbi_load_from_memory
    // (a main-thread decode, R6 — there is no browser decode anywhere in this
    // program), a pad to RES, and a ~1 MiB queue WriteTexture. All of it at
    // whatever instant the network chose, first present very much included.
    // U1's `stb` probe exists precisely because that cost was unattributed.
    //
    // Now the file is COPIED into the holding pen and the lane is freed.
    // pump_authored_valve does the rest, one painting a frame, after the
    // world has been seen.
    //
    // THE COPY IS NOT AVOIDABLE. emscripten_fetch owns fetch->data and frees
    // it at close, and closing late would hold the lane. One memcpy of a few
    // hundred KiB against a decode and an upload is the cheap half.
    if (fetch->data && fetch->numBytes > 0) {
        GalleryState& gs = *ctx->gs;
        gs.authored_held.push_back(GalleryState::AuthoredHeld{
            ctx->staging_layer, ctx->disk_index, ctx->url,
            std::vector<unsigned char>(
                (const unsigned char*)fetch->data,
                (const unsigned char*)fetch->data + (size_t)fetch->numBytes) });
        std::cout << "[Authored] Fetched: " << ctx->url
            << " (" << (unsigned long)fetch->numBytes << " B) → held for staging "
            << ctx->staging_layer << "\n";
        emscripten_fetch_close(fetch);
        authored_fetch_finish(ctx);
        return;
    }

    const long status = fetch->status;
    std::cerr << "[Authored] Failed to load: " << ctx->url << " (empty body)\n";
    emscripten_fetch_close(fetch);
    authored_fetch_release_slot(*ctx->gs, ctx->staging_layer, status);
    authored_fetch_finish(ctx);
}

inline void authored_image_onerror(emscripten_fetch_t* fetch) {
    AuthoredFetchCtx* ctx = (AuthoredFetchCtx*)fetch->userData;
    // TAKEN BEFORE THE CLOSE. emscripten_fetch_close frees the struct, so the
    // status has to be a value before it is a second reader's argument — the
    // shape exhibition_manifest_onerror already uses one screen up.
    const long status = fetch->status;
    std::cerr << "[Authored] Failed to load: " << ctx->url
        << " (HTTP " << status << ")\n";
    emscripten_fetch_close(fetch);
    // The hole is healed rather than abandoned — see
    // authored_fetch_release_slot, which now clears `pending` itself because
    // the append it makes would otherwise find the slot spoken for.
    // The 30 s timeout routes here, and the lane-must-return law is untouched.
    authored_fetch_release_slot(*ctx->gs, ctx->staging_layer, status);
    authored_fetch_finish(ctx);
}

inline void start_authored_fetch(GalleryState& gs,
    const GalleryState::AuthoredFetchRequest& req) {
    AuthoredFetchCtx* ctx = new AuthoredFetchCtx{
        &gs, req.staging_layer, req.disk_index, req.url
    };

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    std::strncpy(attr.requestMethod, "GET", sizeof(attr.requestMethod) - 1);
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    // A LANE MUST ALWAYS COME BACK. With no timeout a request that is
    // accepted and then stalls — a captive portal, a proxy holding the
    // connection open — never calls either callback, so its lane and its
    // slot are gone for the session. One of those and the lane is gone
    // with the rest of the exhibition sitting in the queue, silently,
    // forever. The timeout routes to onerror, which is a path that
    // already frees everything.
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
        // 0 — no round trip happened, so there is no HTTP status to name.
        // The append this makes is QUEUED ONLY; the while-loop calling us
        // takes it on its next turn, which is why this cannot recurse.
        authored_fetch_release_slot(gs, req.staging_layer, 0);
        delete ctx;
    }
}

inline void pump_authored_fetches(GalleryState& gs) {
    // ONE PUMP CALL DOES BOUNDED WORK, AND REPEAT_0 U7 IS WHY IT HAS TO (P1).
    // The healing append re-queues on failure, and start_authored_fetch's
    // synchronous-failure arm reaches that append from inside this very loop.
    // Against an environment where emscripten_fetch refuses every call — an
    // allocation failure, a shut-down page — each turn would remove one
    // request and add one, so the queue would never empty and this would spin
    // for the life of the frame. The ceiling costs nothing in ordinary
    // service: the lane cap stops the loop after at most
    // AUTHORED_FETCH_INFLIGHT_CAP starts, long before 32. When it does bind,
    // the queue survives to the next pump — the work is deferred, never lost.
    uint32_t started = 0;
    // Front-erase on a vector, deliberately: the queue is bounded by
    // STAGING_LAYERS (32), so the copy is a rounding error next to a
    // container choice that would need its own include.
    while (gs.authored_fetch_inflight < AUTHORED_FETCH_INFLIGHT_CAP
        && !gs.authored_fetch_queue.empty()
        && started < Dim::STAGING_LAYERS) {
        GalleryState::AuthoredFetchRequest req = gs.authored_fetch_queue.front();
        gs.authored_fetch_queue.erase(gs.authored_fetch_queue.begin());
        start_authored_fetch(gs, req);
        started++;
    }
}

// ── Authored Image Loading (staging model) — the web twin ──
// SAME NAME, SAME CONTRACT, one word weaker: "this slot will hold this
// painting" instead of "this slot holds this painting". Every consumer
// already reads the slot through `valid`, so the weakening is invisible
// to all of them — a not-yet-arrived painting is the no-content case
// they have always handled.
inline void authored_enqueue_fetch(GalleryState& gs, uint32_t staging_layer,
    uint32_t disk_index, const char* path) {
    if (staging_layer >= Dim::STAGING_LAYERS) return;
    auto& rec = gs.authored_staging[staging_layer];
    if (rec.pending) return;   // already spoken for; a second request would race its own slot

    // THE RECORD KEEPS ITS PICTURE (WALLS_2). This used to clear `valid`
    // at REQUEST time, which threw the image away the instant a
    // replacement was asked for — and the room is hung in the SAME FRAME,
    // 47 lines after teardown_gallery, from whatever survived. At
    // any AUTHORED_FETCH_INFLIGHT_CAP the replacements arrive over round
    // trips long after the walls are up, so a four-wall room that used 28
    // of 32 records left the next one four pictures to hang.
    //
    // `pending` already carries "a fetch is outstanding" — it was written
    // for exactly this distinction one level down, where A REQUEST IS NOT
    // A PICTURE. `valid` means only "this slot holds an image", and it
    // does, until onsuccess overwrites it.
    //
    // Nothing on a wall moves when the fetch lands: queue_promotion has
    // already copied this layer into the painting's OWN exhibition layer
    // (R0), so the staging texture can be replaced underneath a hung
    // frame with no visible effect.
    rec.pending = true;
    rec.disk_index = disk_index;   // the slot advertises its claim to the playlist cursor

    gs.authored_fetch_queue.push_back({ staging_layer, disk_index, std::string(path) });
}

// AND THE ORDINARY ENTRY POINT: claim, queue, PUMP. Every caller that is not
// already inside the pump uses this one — the boot fill and authored_pop.
inline void load_authored_image_to_staging(GalleryState& gs, uint32_t staging_layer, uint32_t disk_index, const char* path) {
    authored_enqueue_fetch(gs, staging_layer, disk_index, path);
    pump_authored_fetches(gs);
}


// ══ AUBADE U5c — THE VALVE ══════════════════════════════════════════
//
// Everything downstream of the fetch — decode, pad, upload — happens
// here and nowhere else, one painting per frame, and not one of them
// before the world has been seen.
//
// WHY IT WAITS. The work is a stb_image decode on the main thread (R6:
// there is no browser decode anywhere in this program), a pad to RES,
// and a ~1 MiB WriteTexture. Left in the fetch's completion callback it
// landed wherever the network put it — including inside the window
// between init and first present, which is the window this whole
// campaign exists to empty. U1's `stb` probe was built to measure
// exactly that, and this is what it measures now: zero.
//
// WHY ONE A FRAME. The staging set is 32 slots and a settling frame
// absorbs one decode-and-upload without hitching; thirty-two in one turn
// is a stall with a visitor already looking at it. At 60 Hz the whole
// set completes inside a second, which is below the READY floor's own
// timeout, so the floor is met by paintings that are actually up.
//
// AND ONE THING GOES AWAY. The upload used to be the single GPU write in
// the program outside the frame's device-lost gate — a browser event is
// not a frame, so a painting landing after a loss wrote through a dead
// queue, and the old call site carried a thirty-line comment explaining
// why that was survivable. It is a frame now. The exemption is gone, not
// argued.
inline void pump_authored_valve(GalleryState& gs, GPUState& gpu, wgpu::Queue& queue) {
    if (gs.authored_held.empty()) return;
    // FIRST PRESENT, and the flag is the same latch the waterfall reads
    // (core/aubade.hpp) — one fact about the boot, one home, no second
    // opinion about whether the world has been seen.
    if (!t7::aubade_presented()) return;

    GalleryState::AuthoredHeld h = std::move(gs.authored_held.front());
    gs.authored_held.erase(gs.authored_held.begin());

    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load_from_memory(
        h.bytes.data(), (int)h.bytes.size(), &width, &height, &channels, 4);
    if (!data) {
        std::cerr << "[Authored] Failed to decode: " << h.url << "\n";
        // 0 — the bytes arrived and stb refused them; the failure is not the
        // network's, and the ring is healed the same way regardless.
        authored_fetch_release_slot(gs, h.staging_layer, 0);
        pump_authored_fetches(gs);
        recount_authored_staged(gs);
        return;
    }
    std::cout << "[Authored] Loaded: " << h.url
        << " (" << width << "x" << height << ") → staging " << h.staging_layer << "\n";
    authored_stage_decoded_image(gs, gpu, queue, h.staging_layer, h.disk_index,
                                 data, width, height);
    stbi_image_free(data);
    // The journey is over: the slot holds a picture, so `pending` — which
    // U5c widened to span fetch AND valve — comes down here.
    gs.authored_staging[h.staging_layer].pending = false;
    recount_authored_staged(gs);
}

// ── Paintings folder scan ──


// THE MANIFEST PARSE, BY HAND. exhibition.json is a flat object of
// string arrays that tools/web_dist.py in THIS repo writes — a JSON
// library would be a dependency taken on for one shape. Find the key,
// take its bracket, read the quoted strings. No escape handling
// because the strings are filenames the same script emitted; anything
// else in the file is ignored rather than rejected, so a manifest that
// grows a field later cannot stop the paintings arriving.
inline void parse_exhibition_array(const char* data, size_t len,
    const char* key_name, std::vector<std::string>& out) {
    std::string s(data, len);
    size_t key = s.find(std::string("\"") + key_name + "\"");
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
// ATRIUM_3 appends " + M atrium" to it and touches nothing before: the
// shell parses the FIRST clause and stops, so the coupling holds.
inline void exhibition_manifest_onsuccess(emscripten_fetch_t* fetch) {
    GalleryState* gs = (GalleryState*)fetch->userData;
    std::vector<std::string> paintings;
    parse_exhibition_array(fetch->data, (size_t)fetch->numBytes, "paintings", paintings);
    emscripten_fetch_close(fetch);

    // Sorted here and not trusted from the file: the ORDER paintings
    // hang in is the program's rule, and a hand-edited manifest must
    // not be able to change it. EACH COLLECTION IS SORTED ON ITS OWN and
    // the two are then CONCATENATED, never re-sorted together: sorting the
    // joined vector would interleave "paintings/PAINTING_3" with
    // "atrium/ATRIUM_3" by number and the partition would stop being a
    // partition.
    auto by_number = [](const std::string& a, const std::string& b) {
        return authored_extract_number(a) < authored_extract_number(b);
    };
    std::sort(paintings.begin(), paintings.end(), by_number);

    gs->authored_disk_manifest.clear();
    gs->authored_disk_manifest.reserve(paintings.size());
    for (const std::string& n : paintings)
        gs->authored_disk_manifest.push_back(std::string(EXHIBITION_PAINTINGS_DIR) + n);
    // TWO FACTS ABOUT THE SEQUENCE, WRITTEN IN ONE BREATH (REPEAT_0a U1).
    // The dead mask is meaningless without the manifest it indexes, so it is
    // sized here and nowhere else — there is no second place for the two to
    // disagree about how long the sequence is.
    gs->authored_manifest_dead.assign(gs->authored_disk_manifest.size(), false);
    gs->authored_exhausted_logged = false;

    std::cout << "[Authored] Scanned " << EXHIBITION_MANIFEST_URL
        << " — found " << paintings.size() << " paintings\n";

    // ══ AUBADE U5b — THE BYTES LEAVE NOW ════════════════════════════
    //
    // The fill used to run from the conductor's deferred-hang head — the
    // first frame of a live world, which is after the device, after the
    // pipelines, after init. Every painting therefore travelled in the
    // window AFTER the one this campaign is emptying, and the boot's
    // longest single wait had the network sitting idle beside it.
    //
    // It runs here instead, the instant the manifest is parsed. On this
    // twin that is inside main(), before console.init asks the browser
    // for an adapter (the manifest fetch's own banner says so), so the
    // paintings travel during the device request and the pipeline
    // compiles — dead time, spent.
    //
    // THIS IS ONLY POSSIBLE BECAUSE THE PATH LOST THE DEVICE (U5b) AND
    // THE WORK LOST ITS PLACE (U5c). Nothing between here and the
    // holding pen touches a queue, a texture or a GPUState, so there is
    // nothing to wait for.
    //
    // THE COUNT WAS ALREADY INVARIANT, and it is worth saying because
    // the campaign's charge is that the boot payload is O(first light):
    // the fill asks for min(manifest, STAGING_LAYERS) = at most 32
    // paintings, and has since OVERTURE_0. A catalogue of 57 or 500
    // sends the same 32. What changes here is WHEN, not how many.
    //
    // The conductor still calls load_authored_textures every frame and
    // still finds its latch set; that call is what makes a manifest
    // arriving LATE fill at all, and it stays for that.
    load_authored_textures(*gs);
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
// no-paintings state.
//
// THE SENTENCE IS THE MANIFEST'S, NOT THE FOLDER'S (PANORAMA_0 F14). It read
// "No paintings folder found" — the native twin's verdict, where the folder
// had been walked and was empty. Here the same emptiness means "the fetch has
// not landed", and since the conductor owns the fill this is now printed on
// the FIRST FRAME, before any manifest could have arrived. It was a false
// sentence in the one place it was guaranteed to be said.
//
// It was also read. web/index.html's classify() fires on a line carrying both
// "found" and "paintings", so the veil announced "Hanging the paintings" at
// frame one with nothing staged and nothing on its way yet. The wording below
// carries neither word, so the veil now says that when the manifest actually
// lands and names a count — which is the line that sets the total.
//
// Said once, because it stays true until it stops being asked.
inline void scan_paintings_folder(GalleryState& gs) {
    if (!gs.authored_disk_manifest.empty()) return;
    if (gs.authored_absence_logged) return;
    gs.authored_absence_logged = true;
    std::cout << "[Authored] The exhibition has not arrived yet\n";
}


inline void load_authored_textures(GalleryState& gs) {
    // Scan folder on first load
    if (gs.authored_disk_manifest.empty()) {
        scan_paintings_folder(gs);
    }
    if (gs.authored_disk_manifest.empty()) {
        // WEB: THE FLAG DOES NOT LATCH ON AN EMPTY MANIFEST. Native's
        // "empty" was a verdict — the folder was walked and there was
        // nothing there — so latching was right: a second walk would
        // have found the same nothing. This twin's "empty" is "the
        // fetch has not landed yet", and boot ALWAYS reads it before
        // it can (init_world runs the whole boot inside one rAF turn,
        // so no fetch callback can fire in the middle of it). Latching
        // here would lock the exhibition out for the session. Left
        // false, the conductor re-enters next frame and finds the
        // manifest (OVERTURE_0) — which is the whole of what makes a
        // late exhibition arrive at all.
        return;
    }

    // ONE FILL, ONE HOME (OVERTURE_0). There is one collection and one
    // session, so there is one fill and one plain bool to latch it. The
    // caller is the conductor's deferred-hang head, every frame — which is
    // what makes a manifest that lands LATE fill at all. It used to fill
    // only when some gallery happened to roll a site type that wanted
    // authored content, which made the exhibition's arrival depend on a
    // die roll; before that it was three call sites re-entering each other.
    if (gs.authored_textures_loaded) return;

    // THE FIRST LAP (REPEAT_0 U1). Layer i takes manifest entry i, ascending,
    // and that is the whole fill: no dedupe book, no free-index search, no
    // skip conditions. There is nothing to dedupe against — this runs once
    // per session, before any other writer has touched a record — and the
    // one-index-one-record invariant the book used to keep is now kept by
    // the arithmetic, which hands out each index exactly once per lap (R5).
    //
    // THE RING IS AS LONG AS THE FILL COULD MAKE IT DENSE, and this is the
    // one place that matters. A layer the manifest cannot reach stays
    // {valid=false, pending=false} forever, and such a layer at the head
    // ends every row for the rest of the session (R3) — the playlist would
    // stall at the first hole and never advance past it. So the ring is
    // min(manifest, STAGING_LAYERS) and the head wraps at THAT, not at 32.
    // A five-painting exhibition is a five-layer ring on repeat, which is
    // what "on repeat" means.
    //
    // The boot payload is unchanged by this: the fill asked min(manifest,
    // 32) before REPEAT_0 and asks min(manifest, 32) after it (AUBADE U5b).
    const uint32_t manifest_size = (uint32_t)gs.authored_disk_manifest.size();
    const uint32_t ring = authored_ring_size(gs);

    // THE PLAYHEAD AND CURSOR ARE SET BEFORE THE LOOP, NOT AFTER IT. The loop
    // below can reach authored_fetch_release_slot synchronously — a fetch that
    // refuses to start unwinds through it — and that function ADVANCES the
    // cursor to heal the layer. Assigning afterwards would silently discard
    // the advance and hand one manifest index to two layers. When the manifest
    // is shorter than the ring, head and cursor coincide at 0, which is
    // correct: the second lap re-asks for the first painting.
    gs.authored_head = 0u;
    gs.authored_disk_cursor = ring % manifest_size;

    for (uint32_t i = 0; i < ring; i++)
        load_authored_image_to_staging(gs, i, i, gs.authored_disk_manifest[i].c_str());

    gs.authored_textures_loaded = true;
    recount_authored_staged(gs);
    std::cout << "[Authored] Staged " << gs.authored_staged_count
        << "/" << manifest_size << " images\n";
}

// THE POP-AND-APPEND — the playlist's whole verb. The caller has already
// secured an exhibition layer; this takes the head, hands it back for
// promotion, and refills the vacated layer from the cursor in the same act.
// Vacancy is never observable.
//
// ORDERING LAW AT EVERY CALL SITE: SECURE THE EXHIBITION LAYER FIRST, THEN
// POP. A pop with no layer to promote into would advance the playlist past a
// painting nobody ever saw — the one way this design can lose a picture. The
// law is the SIGNATURE, not a comment: `exh` is the layer the caller has
// already secured, and a caller that has not secured one cannot form the
// call. The verb does nothing with it but say it (U8) — which is the point,
// because what the witness proves is precisely that the two happened in this
// order.
//
// Returns UINT32_MAX if the head is not `valid && !pending` (R3). A pending
// head stops authored supply for that site exactly as a dry pool did before
// it: the row ends, the snapshot fallback and every other row-end behaviour
// are unchanged, and NOTHING skips past the head to a deeper record. Order is
// semantics here — the playlist is a sequence, not a pool.
//
// THE APPEND CANNOT RACE THE READ THE CALLER IS ABOUT TO MAKE. The caller
// reads `aspect_ratio` and the two uv scales off the popped record after this
// returns. Those three are written in exactly one place —
// authored_stage_decoded_image, reached only from pump_authored_valve, which
// is a FRAME verb (AUBADE U5c) and cannot run inside this call. What this
// function does to the record is set `pending` and re-aim `disk_index`; the
// picture and its shape stay put until the round trip lands and overwrites
// them, which is the WALLS_2 guarantee, here merely harmless. And by then the
// frame that hung it owns its own pixels in its own exhibition layer (R0), so
// there is nothing left for a late arrival to disturb.
inline uint32_t authored_pop(GalleryState& gs, uint32_t exh) {
    const uint32_t ring = authored_ring_size(gs);
    if (ring == 0u) return UINT32_MAX;          // no exhibition yet — nothing to play
    if (gs.authored_head >= ring) gs.authored_head = 0u;

    const uint32_t manifest_size = (uint32_t)gs.authored_disk_manifest.size();
    const uint32_t layer = gs.authored_head;
    const AuthoredStagingRecord& head = gs.authored_staging[layer];
    if (!head.valid || head.pending) {
        // A HOLE AT THE HEAD IS REPAIRED BY THE HAND THAT TRIPS OVER IT. An
        // invalid, not-pending head is a layer whose fetch failed after the
        // eager heal had stood down (the slip run below), and nothing else
        // would ever ask for it again — the head cannot advance past it (R3),
        // so the playlist would be over for the session. Asking here costs one
        // request per hang attempt, which is demand-driven and therefore
        // bounded by the visitor rather than by the network, and it is what
        // makes the slip budget safe to have at all.
        if (!head.valid && !head.pending && manifest_size > 0u) {
            const uint32_t heal = authored_take_next(gs);
            if (heal != UINT32_MAX)
                load_authored_image_to_staging(gs, layer, heal,
                    gs.authored_disk_manifest[heal].c_str());
        }
        return UINT32_MAX;   // R3 — the row ends here either way
    }

    // TAKEN BEFORE THE APPEND, and only so the witness below cannot be read
    // as a question. The append does not touch `shown_disk_index` — only
    // `pending` and `disk_index` — but the line that names the hung painting
    // should not have to be argued for through a reference the same statement
    // re-aims.
    const uint32_t hung_disk = head.shown_disk_index;

    // AND THE LAYER LEARNS ITS NAME (REPEAT_0a R5). This is the ONLY writer:
    // the exhibition's half of the window is written by the same act that
    // fills it, from the same value the witness prints, so the two can never
    // disagree about what is on the wall.
    if (exh < Dim::EXHIBITION_LAYERS) gs.exhibition_name[exh].disk_index = hung_disk;

    // THE CURSOR IS TAKEN AND ADVANCED IN ONE ACT, BEFORE THE APPEND. The
    // append can reach authored_fetch_release_slot synchronously (a fetch that
    // refuses to start unwinds through it) and that function takes from the
    // cursor too; advancing afterwards would overwrite its advance with a
    // value derived from the pre-call cursor, handing one manifest index to
    // two ring layers and rewinding the playlist by one.
    //
    // UINT32_MAX IS EXHAUSTION, AND IT DOES NOT STOP THE POP. The head is
    // valid and its picture is going on a wall; there is simply nothing left
    // to refill the vacated layer with, so the layer goes invalid and ends a
    // row later (R3). A dead sequence must never cost the pictures already in
    // the ring.
    const uint32_t disk = authored_take_next(gs);
    gs.authored_head = (layer + 1u) % ring;

    // AND THE APPEND IS SKIPPED WHEN THE LAYER ALREADY HOLDS THAT PAINTING.
    // Where the manifest is no larger than the ring, head and cursor advance
    // in lockstep and the cursor always names the picture this layer is
    // already showing — so the fetch would re-download an image byte for byte
    // to replace itself, and would mark the layer `pending` for a whole round
    // trip while doing it, shortening every row for no reason. A twelve-image
    // exhibition now costs no network at all after its boot lap, which is what
    // "the window means lookahead" should have meant all along.
    if (disk != UINT32_MAX && gs.authored_staging[layer].shown_disk_index != disk)
        load_authored_image_to_staging(gs, layer, disk,
            gs.authored_disk_manifest[disk].c_str());

    // THE PLAYHEAD, MADE VISIBLE (REPEAT_0 U8). Autonomous stdout, and it
    // inherits the slot `[Authored] Rotated` held one-for-one — same family,
    // same always-on rider (none), same standing exhibition-guard candidacy.
    // It is the campaign's whole visual instrument, and each field reads one
    // ruling: `disk` ascends continuously ACROSS a world boundary because
    // teardown no longer touches the ring (U5), and `cursor` wraps at the
    // manifest's end because the lap is the feature (R5).
    //
    // `disk` is what is going ON THE WALL — the SHOWN index, not the claim
    // this pop just re-aimed at the appended painting (WALLS_3).
    // `ready` AND `hung` ARE THE TWO HALVES OF THE WINDOW (R6), read AFTER
    // the pop so they describe the state the visitor is now in. What healthy
    // looks like is worth stating, because `ready` is a gauge and not a
    // tally: it reads ring-1 while the append that refills the vacated layer
    // is still in flight and ring once it lands, so it oscillates just under
    // the ring and only SINKS when the sequence is in trouble. `hung` tracks
    // the room — up as the walls fill, down through the evict lines at a
    // world boundary.
    std::cout << "[Authored] Pop layer=" << layer
        << " disk=" << hung_disk
        << " → exh=" << exh
        << ", cursor=" << gs.authored_disk_cursor
        << " ready=" << authored_ready_depth(gs)
        << " hung=" << authored_hung_count(gs) << "\n";
    return layer;
}

// THE CLERKS WERE DISMISSED HERE (REPEAT_0 U6). Five functions stood between
// this banner and the wall paintings, and all five existed to answer one
// question the program no longer asks — WHICH staged record should hang next:
//
//   · `authored_hangable`      — the pool's size. Now authored_ready_depth.
//   · `count_unused_authored`  — the pool's size, minus a caller's mask.
//   · `pick_authored_staging`  — lowest shown index, the selection order. It
//                                had two hand-rolled twins inline (U3, U4);
//                                all three are gone, so the order cannot fork.
//   · `authored_record_for_disk` — the record holding ONE NAMED index. Its
//                                only caller was the atrium's poster stage,
//                                deleted by ATTIC_ATRIUM D1, which left this
//                                definition orphaned. Zero callers and zero
//                                declarations at f4e2d0dc.
//   · `authored_release_layer` — an exhibition layer going dark released the
//                                record that fed it, so the pool would not
//                                walk down to nothing as the pawn walked. The
//                                pool cannot walk down now: supply is the
//                                cursor, and it is inexhaustible by
//                                construction.
//
// Arithmetic is the guarantee (R5). `disk_in_use[256]` and its deploy-time
// mirror MANIFEST_DEDUPE_CAP go with them, in tools/web_dist.py.
// Resurrect: git checkout f4e2d0dc^ -- src/cartridges/the_board/bodies/gallery.hpp

// ═══ WALL PAINTINGS (indoor) ═════════════════════════════════════

inline void place_wall_paintings(GalleryState& gs, GalleryDeps* c, wgpu::Queue& queue,
    float bmin, float bmax, float wall_height) {
    // Clear any existing wall paintings first (indoor→indoor transitions)
    clear_wall_paintings(gs, c, queue);

    // NO FILL HERE (OVERTURE_0): the conductor owns it. This call was already
    // a no-op after the first successful fill — the latch is a session bool —
    // and the room is entered through a portal, which is many thousands of
    // conductor frames after that fill.

    // ATRIUM_3 — THE COLLECTION IN FORCE, resolved once for this hang. The
    // wall hang runs inside apply_mood, which has already written
    // mood_state_.active, so the live mood IS this world's mood.

    // Painting center base height (fraction of ceiling) — WALL_ART knob.
    constexpr float WALL_OFFSET = 0.05f;    // distance from wall surface

    // wall_height is the true top of the VERTICAL wall on both paths — ch on
    // FLAT, the collapsed spring on VAULT (P5a/P5b) — and it excludes
    // JOINT_OVERLAP, which is a mesh joint device, not usable wall.
    float paint_y_base = wall_height * WALL_ART.paint_y_frac;
    float wall_span = bmax - bmin;
    float wall_center = (bmin + bmax) * 0.5f;

    // Three-way site type: snapshot-only / mixed / authored-only.
    // ATRIUM_3 — THE ROLL IS SKIPPED WHERE THE CALLER HAS DECIDED. A room
    // draws its site type from its seed; the entrance does not roll for
    // anything, so place_atrium_walls passes AUTHORED_ONLY and the seed's
    // word is never asked for. The seed is still hashed — site_seed feeds
    // the wall count, the shuffle and every per-painting roll below, and
    // those are the room's grammar whoever chose the site type.
    uint32_t site_seed = cpu_hash(c->world_state_.active_seed, WallArtProp::SITE_SEED_OFFSET);
    float site_roll = cpu_hash_f(site_seed, WallArtProp::SITE_TYPE_ROLL);
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

        // The SNAPSHOT claim mask, and it is now the only one. Selection does
        // not set `consumed` — constraint 2: a snapshot the placement pass
        // drops on slot exhaustion (Stage C's break) must not have wasted its
        // record. The mask prevents two paintings on THIS wall picking the
        // same record; `consumed`, set at placement, is what later walls see.
        //
        // `authClaimed` is DELETED (REPEAT_0 U3). The authored side has
        // nothing to claim: it does not pick a record, it takes the head, and
        // the head advances at the pop. Within a wall the plan counts its own
        // pops forward from the head; between walls the pops have already
        // happened, so wall two peeks from where wall one left the playhead.
        // The mask was bookkeeping about a choice that is no longer made.
        bool snapClaimed[Dim::STAGING_LAYERS]{};
        uint32_t auth_planned = 0;   // this wall's pops-to-come, counted forward from the head

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
            // plan-side count forward from the playhead, not a live
            // consumed-count.
            bool use_snapshot = (site_type == IndoorSiteType::SNAPSHOT_ONLY)
                || (site_type == IndoorSiteType::MIXED
                    && cpu_hash_f(p_seed, WallPaintingProp::MIX_SNAPSHOT_ROLL) < WALL_ART.mix_snapshot_chance);

            // WHAT THE PLAYLIST CAN STILL HAND THIS WALL: the ready depth,
            // less the pops this wall has already planned but not yet made.
            // A scan of 32 became one walk from the head (REPEAT_0 U3).
            const uint32_t auth_ready = authored_ready_depth(gs);
            const uint32_t auth_free = auth_ready > auth_planned
                ? auth_ready - auth_planned : 0u;
            // ATRIUM_3 — STRICT TURNS THE DRY-POOL FALL-THROUGH OFF. In a
            // room a dry authored pool means "hang a snapshot instead"; in
            // the entrance it means "hang nothing". The atrium hangs its own
            // folder or nothing.
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

            if (!resolved && !use_snapshot && auth_free > 0) {
                // THE PEEK, AND IT IS THE WHOLE OF WHAT SELECTION USED TO BE.
                // The k-th authored frame this wall plans will be fed by the
                // k-th pop this wall makes, and that is the record at
                // head + k — no scan, no mask, no lowest-shown ordering. The
                // depth is well-defined because ready depth is CONTIGUOUS from
                // the head (U2), so every layer from head to head+auth_free-1
                // is valid and not pending.
                //
                // Only the aspect is wanted here, and only to trim the row to
                // real widths. The RECORD is not a plan decision any more, so
                // the plan does not carry one: the pop names it, at placement.
                const uint32_t ring = authored_ring_size(gs);
                const auto& peek = gs.authored_staging[(gs.authored_head + auth_planned) % ring];
                f.record = UINT32_MAX;
                f.is_snapshot = false;
                f.aspect = peek.aspect_ratio;
                auth_planned++;
                resolved = true;
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
        uint32_t wall_placed = 0;   // U7's witness: this wall's own count
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
                // THE POP, AND IT COMES AFTER THE LAYER (U2's ordering law).
                // `exh` is secured above; only now does the playlist advance,
                // so a painting can never be stepped past without a wall to
                // hang it on. If the head went pending between plan and
                // placement the row ends here, the same shape as an exhausted
                // exhibition layer one line up.
                const uint32_t rec = authored_pop(gs, exh);
                if (rec == UINT32_MAX) break;    // this wall, not the hang

                // f.aspect is the peeked record's, and `rec` IS that record —
                // the k-th pop takes what the k-th peek read. Planned width is
                // placed width, so the row stays centred (Stage C's law).
                const auto& img = gs.authored_staging[rec];
                fill_slot_wall_frame(s,
                    px, py, pz,
                    wall.nx, wall.ny, wall.nz,
                    f.aspect, f.height,
                    exh, ContentSource::AUTHORED,
                    img.uv_scale_x, img.uv_scale_y,
                    FRAME_AUTHORED,
                    INT32_MAX, INT32_MAX);

                gs.exhibition_occupied[exh] = true;
                // NO CLAIM IS WRITTEN. `consumed`, `hung_this_world` and
                // `exhibition_layer` were one triad here (OVERTURE_0's "THE
                // CLAIM, WHOLE"); all three die with selection (R7), because
                // supply now comes from the cursor and never from a record
                // handed back.
                queue_promotion(gs, false, rec, exh);
            }

            cursor += f.width + WALL_ART.painting_gap;
            c->gpuState_.upload_painting_slot(queue, slot, s);
            gs.wall_frame_count++;
            wall_placed++;
            if (f.is_snapshot) snapshot_placed++; else authored_placed++;
        }

        // A BARE WALL IS A DEFECT, NEVER CHATTER (OVERTURE_0). The room's
        // summary line reports the ROOM's total, so three walls hung and one
        // bare reads as a slightly thin room rather than as the specific
        // thing it is — a selected wall that got nothing. Always-on, on
        // stderr, and it says which of the two ran out.
        if (wall_placed == 0) {
            uint32_t snaps_free = 0;
            for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++)
                if (gs.snapshot_staging[i].valid && !gs.snapshot_staging[i].consumed) snaps_free++;
            std::cerr << "[WallPainting] BARE WALL " << w
                << ": planned " << effective_count
                << " pieces, authored ready " << authored_ready_depth(gs)
                << ", snapshots " << snaps_free << "\n";
        }
    }

    const char* site_type_name = (site_type == IndoorSiteType::SNAPSHOT_ONLY) ? "SNAPSHOT"
        : (site_type == IndoorSiteType::MIXED) ? "MIXED" : "AUTHORED";
    // Autonomous stdout — exhibition-guard candidate, still open.
    //
    // The counts are split by CONTENT SOURCE, not by tier: there is one tier
    // now. In a MIXED room the split is the mix roll made visible, and in an
    // AUTHORED_ONLY room a non-zero snapshot count is the playlist running dry
    // and falling through — a zero `auth_free`, which since REPEAT_0 means a
    // PENDING HEAD rather than an empty pool — which is the one thing here
    // worth seeing from the console.
    std::cout << "[WallPainting] Placed " << authored_placed
        << " painting(s) + " << snapshot_placed
        << " snapshot(s) across " << active_wall_count << " walls"
        << " (" << site_type_name << ")\n";
}

// ═══ THE ATRIUM'S HANG (ATRIUM_3) ════════════════════════════════
// Index 0 of the folder is the controls scheme and stands dead ahead on the
// sand, facing the arrival point. The next sand spots take the next images;
// the walls take the rest — this folder only, no fall-through to snapshots.
//
// A standing quad on the sand is TERRAIN_QUAD with AUTHORED content: the
// snapshot quad's inline fill in commit_gallery, with the record's uv scale
// (the wall frame's idiom), because the shader reads uv per slot whatever
// the form. The sentinel patch pair is the wall frame's too — no patch
// evicts an image the entrance owns.
//
// The room, once the folder has settled: the walls take everything the sand
// did not, and the memory is taken over BOTH stages. Reached only from the
// in_flight == 0 arm, which is what makes "nothing hangable left" mean "all
// of it is up" rather than "one never arrived".

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
            release_exhibition_layer(gs, exh);
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
    const GalleryPlaceResult r = place_gallery_from_selection(self, e.gallery, pe.gallery);
    if (r == GalleryPlaceResult::PLACED) return true;
    // Either way the centre slot select reserved is released. Only DEFERRED
    // leaves anything behind: the bit on the TRIGGER patch (OVERTURE_0) — the
    // one being spawned, or the one being retried.
    self->gallery_state_.gallery_centers[e.gallery.slot].active = false;
    if (r == GalleryPlaceResult::DEFERRED)
        if (auto* p = find_patch(self, e.gx, e.gz)) p->gallery_deferred = true;
    return false;
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

// ═══ THE DEFERRED HANG (OVERTURE_0) — owner verb, conductor-called ═
//
// A patch whose gallery the pool could not dress is retried here,
// nearest first, one a frame, only while the pool holds something.
//
// The fill sits at the head because it is its own latch and the manifest
// may land after boot: this is the ONE place the exhibition is asked
// for. Boot is not special here — a transition arrives with its pictures
// in hand and defers nothing, so boot is the only live case and it is
// not a special one.
//
// It calls the gallery's dispatch trio DIRECTLY and never touches
// entityQueue_ / placementResults_, so spawn_selected_patches remains the
// SOLE caller of the three queue verbs and SPAWN_QUEUE_MAX's bound stands.
// staging_reserved balances inside the call — place reserves, commit
// releases — for the same reason it does on the spawn path.
//
// collect_sorted_patches is a template declared in the surface's decl tier
// and defined at the cohort tail; the pre-tail call binds at end-of-TU
// instantiation, which is the same reach bodies/ribbon.hpp already makes
// for run_spawn_preamble.
inline void tick_gallery_deferred_hang(MachineCtx* c, wgpu::Queue& queue) {
    auto& gs = c->gallery_state_;
    // AUBADE U5c — the valve, first and unconditionally: every early
    // return below is about the HANGING, and a painting waiting to be
    // decoded must not be held hostage to a room that has nowhere to put
    // it yet. One painting, this frame, and only after first present.
    pump_authored_valve(gs, c->gpuState_, queue);
    load_authored_textures(gs);
    if (gallery_available_staging(gs, GallerySiteType::MIXED) == 0) return;   // the pool's sum
    PatchCandidate cands[Dim::MAX_ACTIVE_PATCHES];
    const uint32_t n = collect_sorted_patches(c, cands, c->point_.x, c->point_.z,
        [](const ActivePatch& p) { return p.gallery_deferred; }, true);
    if (n == 0) return;
    ActivePatch& p = c->patch_system_state_.patches_[cands[0].idx];
    p.gallery_deferred = false;          // re-raised by place if the pool ran dry under us

    // THE TRIGGER FIELDS ARE THE CALLER'S, exactly as on the spawn path
    // (select_entities_for_patch fills them before try_select). The funnel
    // copies them into the placement, and the DEFERRED arm above finds this
    // patch by them.
    EntityQueueEntry e{};
    e.family = PopFamily::GALLERY;
    e.gx = p.grid_x; e.gz = p.grid_z;
    if (!dispatch_select_gallery(c, p.grid_x, p.grid_z, e)) return;
    PlacementEntry pe{};
    if (!dispatch_place_gallery(c, e, pe)) return;
    dispatch_commit_gallery(c, pe, queue);
    gs.deferred_dressed++;
    // THE Y-CORRECTION THIS GALLERY WOULD OTHERWISE NEVER GET. At spawn the
    // raise comes from the patch's own generation; a deferred gallery lands on
    // a patch that is already GENERATED, and commit_gallery raises nothing.
    // R16 consumes this in the same frame — stream_patches runs before it, and
    // the conductor's tail ORs rather than assigns.
    c->world_state_.placement_dirty = true;
    if (n == 1)
        std::cout << "[Gallery] Deferred hang: " << gs.deferred_dressed
                  << " dressed this world, none waiting\n";
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
    gs.deferred_dressed = 0;
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
    // Free all exhibition layers (staging persists across worlds) — except
    // the ones the atrium holds, which outlive every world (ATRIUM_7).
    for (uint32_t i = 0; i < Dim::EXHIBITION_LAYERS; i++)
        release_exhibition_layer(gs, i);
    // TEARDOWN FORGETS THE AUTHORED SIDE ENTIRELY (REPEAT_0 U5). Two things
    // stood here: the rotation, and a loop clearing the claims the rotation
    // read. Both are gone, and the staging array crosses a world boundary
    // untouched — which is the point. The playhead does not reset at a
    // portal, so the next world's first room takes the next paintings in the
    // sequence, and the disk indices ascend continuously across the boundary.
    //
    // THE STARVATION LEAVES WITH THE MECHANISM. The rotation ran BETWEEN
    // freeing the layers and clearing the claims, and its refresh set
    // `valid = false, pending = true` on every record the last world hung —
    // while place_wall_paintings hung the next room in the SAME FRAME, off
    // `valid`. A heavy room consumed up to 28 of 32 records, so the next room
    // opened with four. Nothing here invalidates anything now: a popped layer
    // keeps its picture until its own fetch lands (WALLS_2), so the next room
    // hangs full immediately.
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
