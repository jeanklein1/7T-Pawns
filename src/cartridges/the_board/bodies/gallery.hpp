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
// NOTE[gallery:shadows-missing] paintings (terrain quads) and wall
//   frames are not currently drawn in the shadow pass
//   (render_passes.hpp::draw_shadow_all). They render in the main
//   pass via draw_wall_paintings + draw_gallery_frames but cast no
//   shadows. Known gap; not addressed in this pass.
// ─────────────────────────────────────────────────────────────────

#include <algorithm>   // std::max, std::min, std::sort, std::transform   // (impl, merged)
#include <cmath>       // std::sqrt, std::floor, std::cos, std::sin, std::round   // (impl, merged)
#include <cstdint>   // (impl, merged)
#include <filesystem>  // paintings folder scan   // (impl, merged)
#include <iostream>    // capture / gallery / authored logs   // (impl, merged)
#include <string>      // manifest paths, std::stoi   // (impl, merged)
#include <vector>      // manifest + pixel staging   // (impl, merged)

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
    static constexpr float GALLERY_CHANCE_BY_ARCHETYPE[4] = { 0.03f, 0.06f, 0.30f, 0.40f };

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

    static constexpr float MONO_TIER_CHANCE = 0.40f;  // 40% mono, 60% chronological

    static constexpr float GALLERY_SIZE_LO = 0.85f;  // smallest gallery mean
    static constexpr float GALLERY_SIZE_HI = 3.0f;   // largest gallery mean
    static constexpr float PAINTING_SIZE_SIGMA = 0.3f;   // per-painting jitter around gallery mean

    // Minimum snapshots before galleries start appearing
    static constexpr uint32_t MIN_POOL_SIZE = 3;

    // Minimum distance between gallery centers (world units)
    static constexpr float MIN_GALLERY_DISTANCE = 150.0f;

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

    // ─── Per-wall painting count: uniform [lo, hi] inclusive ─
    uint32_t per_wall_count_lo;
    uint32_t per_wall_count_hi;

    // ─── Wall surface geometry ──────────────────────────────
    float corner_margin;        // distance from wall corners
    float painting_gap;         // gap between adjacent painting edges
    float paint_y_frac;         // base center as fraction of ceiling
    float max_bottom_height;    // hard upper clamp on bottom edge (m)

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

    // per-wall painting count
    /* per_wall_count_lo */ 1,
    /* per_wall_count_hi */ 5,

    // wall surface geometry
    /* corner_margin     */ 12.0f,
    /* painting_gap      */ 6.0f,
    /* paint_y_frac      */ 0.45f,
    /* max_bottom_height */ 4.0f,   // bottom no higher than 4 m above floor

    //                 height_lo, height_hi, weight, y_offset_lo, y_offset_hi
    /* intimate  */  {  6.0f,    11.0f,    0.25f,   0.0f,        2.0f },
    /* standard  */  {  8.0f,    12.0f,    0.50f,  -1.5f,        1.5f },
    /* statement */  { 10.0f,    14.0f,    0.25f,  -3.5f,       -1.5f },

    // content mix
    /* snapshot_only_share */ 0.15f,
    /* mixed_share         */ 0.05f,
    /* mix_snapshot_chance */ 0.40f,
};

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
    static constexpr uint32_t PER_WALL_BASE       = 10u;  // w_seed = hash(site_seed, BASE + w*STRIDE)
    static constexpr uint32_t PER_WALL_STRIDE     = 20u;

    // off w_seed:
    static constexpr uint32_t WALL_PAINTING_COUNT     = 0u;
    static constexpr uint32_t PER_PAINTING_BASE       = 100u;  // p_seed = hash(w_seed, BASE + p*STRIDE)
    static constexpr uint32_t PER_PAINTING_STRIDE     = 10u;
};

// Indoor — per-painting (p_seed = hash(w_seed, WallArtProp::PER_PAINTING_BASE + p*STRIDE))
struct WallPaintingProp {
    static constexpr uint32_t Y_OFFSET_JITTER   = 1u;
    static constexpr uint32_t MIX_SNAPSHOT_ROLL = 2u;  // chance this painting is snapshot in MIXED site
    static constexpr uint32_t HEIGHT_JITTER     = 3u;
    static constexpr uint32_t AUTH_STG_PICK     = 4u;  // unused — see Q30 in rollout report
    static constexpr uint32_t ASPECT_ESTIMATE   = 5u;
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
};

// Pending texture promotions (staging → exhibition, executed in render)
struct PendingPromotion {
    bool is_snapshot;       // true = snapshot staging, false = authored staging
    uint32_t staging_layer;
    uint32_t exhibition_layer;
};
inline constexpr uint32_t MAX_PROMOTIONS_PER_FRAME = 32;

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

    AuthoredStagingRecord authored_staging[Dim::STAGING_LAYERS]{};
    uint32_t              authored_write_cursor = 0;
    uint32_t              authored_disk_cursor = 0;     // walks through authored_disk_manifest
    uint32_t              authored_staged_count = 0;
    bool                  authored_textures_loaded = false;
    std::vector<std::string> authored_disk_manifest;    // scanned lazily on first load, sorted numerically

    bool     exhibition_occupied[Dim::EXHIBITION_LAYERS]{};
    uint32_t exhibition_count = 0;

    PendingPromotion pending_promotions[MAX_PROMOTIONS_PER_FRAME]{};
    uint32_t         pending_promotion_count = 0;

    GPUPaintingSlot painting_slots[Dim::PAINTING_MAX_SLOTS]{};
    uint32_t        active_painting_count = 0;
    uint32_t        wall_frame_count = 0;

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
    float bmin, float bmax, float ceiling_h);
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

inline uint32_t find_free_painting_slot(const GalleryState& gs) {
    for (uint32_t i = 0; i < Dim::PAINTING_MAX_SLOTS; i++)
        if (gs.painting_slots[i].is_active == 0) return i;
    return UINT32_MAX;
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
    float px = c->player_.readback_x;
    float pz = c->player_.readback_z;

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
    // DIAG-unwrapped (census: constitution §5): autonomous stdout
    // (walk-cadence capture) — wrap in #ifdef DIAG_GALLERY at ship.
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

    // Footprint radius: gallery spread
    float footprint_r = (float)GalleryConfig::PAINTINGS_MAX_BY_ARCHETYPE[archetype]
        * 0.5f * GalleryConfig::ROW_SPACING + 15.0f;

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
    sel.footprint_r = footprint_r;
    sel.archetype = archetype;
    sel.painting_count = painting_count;
    sel.facing_angle = facing_angle;
    sel.gallery_size_mean = gallery_size_mean;
    sel.site_type = site_type;

    return true;
}

// ── place_gallery_from_selection ──

inline bool place_gallery_from_selection(MachineCtx* c, const GallerySelection& sel, GalleryPlacement& plan) {
    // FULL containment (INDOOR_TREATMENT): the sand-standing fan
    // stays wholly inside the walls. The containment extent is
    // sel.footprint_r — its authored formula IS the fan law
    // (max-count half-span + the painting allowance). Wall-hung
    // frames are the shell's and never cross this site. A
    // collapsed box skips the gallery (loud line in the clamp).
    float cx = sel.cx, cz = sel.cz;
    if (!indoor_bounds_clamp(c, PopFamily::GALLERY,
        sel.footprint_r, sel.footprint_r, cx, cz))
        return false;
    if (!check_position(c, cx, cz, sel.footprint_r, PopFamily::GALLERY))
        return false;

    int32_t host_gx = (int32_t)std::floor(cx / Dim::PATCH_EXTENT);
    int32_t host_gz = (int32_t)std::floor(cz / Dim::PATCH_EXTENT);

    if (register_footprint(c, cx, cz, sel.footprint_r,
        host_gx, host_gz, PopFamily::GALLERY, sel.archetype) == UINT32_MAX)
        return false;

    plan = GalleryPlacement{};
    plan.slot = sel.slot;
    plan.trigger_gx = sel.trigger_gx;
    plan.trigger_gz = sel.trigger_gz;
    plan.host_gx = host_gx;
    plan.host_gz = host_gz;
    plan.tier_idx = sel.archetype;
    plan.cx = cx;
    plan.cz = cz;
    plan.footprint_r = sel.footprint_r;
    plan.archetype = sel.archetype;
    plan.painting_count = sel.painting_count;
    plan.facing_angle = sel.facing_angle;
    plan.gallery_size_mean = sel.gallery_size_mean;
    plan.site_type = sel.site_type;

    record_placement_bookkeeping(PopFamily::GALLERY, plan.tier_idx);
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

    bool have_snapshots = candidate_count > 0;
    bool have_authored = gs.authored_staged_count > 0;
    if (site_type == GallerySiteType::SNAPSHOT_ONLY && !have_snapshots) {
        gs.gallery_centers[plan.slot].active = false;
        return;
    }
    if (site_type == GallerySiteType::AUTHORED_ONLY && !have_authored) {
        gs.gallery_centers[plan.slot].active = false;
        return;
    }
    if (site_type == GallerySiteType::MIXED && !have_snapshots && !have_authored) {
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

    // Cap painting count to available content
    uint32_t painting_count = plan.painting_count;
    uint32_t max_available = candidate_count + gs.authored_staged_count;
    if (site_type == GallerySiteType::SNAPSHOT_ONLY) max_available = candidate_count;
    if (site_type == GallerySiteType::AUTHORED_ONLY) max_available = gs.authored_staged_count;
    if (painting_count > max_available) painting_count = max_available;

    // Layout
    float face_x = std::cos(facing_angle);
    float face_z = std::sin(facing_angle);
    float row_x = -face_z;
    float row_z = face_x;
    float row_start = -(float)(painting_count - 1) * 0.5f * GalleryConfig::ROW_SPACING;

    uint32_t placed = 0;
    bool usedAuthored[Dim::STAGING_LAYERS]{};

    for (uint32_t p = 0; p < painting_count; p++) {
        uint32_t slot = find_free_painting_slot(gs);
        if (slot == UINT32_MAX) break;

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
                gs.exhibition_count++;
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
            s.uv_scale_x = 1.0f;
            s.uv_scale_y = 1.0f;
            s.patch_gx = gx; s.patch_gz = gz;

            gs.exhibition_occupied[exh] = true;
            gs.exhibition_count++;
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
        gc.active = false;  // no paintings placed — release center
    }
    else {
        // DIAG-unwrapped (census: constitution §5): autonomous stdout
        // (procedural placement) — wrap in #ifdef DIAG_GALLERY at ship.
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
                gs.exhibition_count--;
            }

            if (gs.painting_slots[i].form_type == FormType::WALL_FRAME) {
                gs.wall_frame_count--;
            }

            gs.painting_slots[i].is_active = 0;
            c->gpuState_.deactivate_painting_slot(queue, i);
            gs.active_painting_count--;
        }
    }
}

// ═══ SNAPSHOT RENDER ═════════════════════════════════════════════

inline void render_snapshot_pass(GalleryState& gs, GalleryDeps* c, wgpu::CommandEncoder& encoder) {
    if (!gs.pending_snapshot.active) return;

    {
        wgpu::ComputePassDescriptor cpd{};
        cpd.label = "Photographer VP Compute";
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

    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&desc);

    c->renderer_.draw_patch_terrain_direct(pass,
        c->gpuState_.photographer_render_entity_group(),
        c->gpuState_.render_texture_group(),
        c->gpuState_.patch_index_buffer(),
        c->gpuState_.patch_index_count(),
        c->world_state_.render_patch_count);

    // The drawable table — snapshot members, canonical order (the photographer's
    // own entity group). Terrain above is the per-pass FORK (a single direct
    // draw over render_patch_count, no LOD split). Zone is not a snapshot member.
    DrawBind b{ c->gpuState_.photographer_render_entity_group(), c->gpuState_.render_texture_group(),
                /*shadow=*/false,
                c->ribbon_state_.rendered_slot != UINT32_MAX };
    draw_table(c->renderer_, c->gpuState_, pass, b, DRAW_SNAPSHOT);

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
    stbi_image_free(data);

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

// ── Paintings folder scan ──

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
    auto extract_number = [](const std::string& path) -> int {
        namespace fs = std::filesystem;
        std::string name = fs::path(path).stem().string();  // "PAINTING_12"
        size_t pos = name.find('_');
        if (pos == std::string::npos || pos + 1 >= name.size()) return 0;
        try { return std::stoi(name.substr(pos + 1)); }
        catch (...) { return 0; }
        };
    std::sort(gs.authored_disk_manifest.begin(), gs.authored_disk_manifest.end(),
        [&](const std::string& a, const std::string& b) {
            return extract_number(a) < extract_number(b);
        });

    std::cout << "[Authored] Scanned " << found_dir.string()
        << " — found " << gs.authored_disk_manifest.size() << " paintings\n";
}

inline void load_authored_textures(GalleryState& gs, GPUState& gpu, wgpu::Queue& queue) {
    if (gs.authored_textures_loaded) return;

    // Scan folder on first load
    if (gs.authored_disk_manifest.empty()) {
        scan_paintings_folder(gs);
    }
    if (gs.authored_disk_manifest.empty()) {
        gs.authored_textures_loaded = true;
        return;
    }

    // Fill staging with the first STAGING_LAYERS images from manifest
    uint32_t manifest_size = (uint32_t)gs.authored_disk_manifest.size();
    uint32_t to_load = std::min(manifest_size, Dim::STAGING_LAYERS);
    for (uint32_t i = 0; i < to_load; i++) {
        load_authored_image_to_staging(gs, gpu, queue, i, i, gs.authored_disk_manifest[i].c_str());
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
    bool disk_in_use[256]{};  // generous upper bound
    for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) {
        if (gs.authored_staging[i].valid && !gs.authored_staging[i].consumed) {
            if (gs.authored_staging[i].disk_index < 256)
                disk_in_use[gs.authored_staging[i].disk_index] = true;
        }
    }

    uint32_t rotated = 0;
    for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) {
        if (!gs.authored_staging[i].consumed) continue;  // keep unconsumed

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
        // DIAG-unwrapped (census: constitution §5): autonomous stdout
        // (teardown rotation) — wrap in #ifdef DIAG_GALLERY at ship.
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

inline void place_wall_paintings(GalleryState& gs, GalleryDeps* c, wgpu::Queue& queue, float bmin, float bmax, float ceiling_h) {
    // Clear any existing wall paintings first (indoor→indoor transitions)
    clear_wall_paintings(gs, c, queue);

    load_authored_textures(gs, c->gpuState_, queue);

    // Painting center base height (fraction of ceiling) — WALL_ART knob.
    constexpr float WALL_OFFSET = 0.05f;    // distance from wall surface

    float paint_y_base = ceiling_h * WALL_ART.paint_y_frac;
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

    // Track which authored layers have been used across all walls (no duplicates)
    bool usedAuthored[Dim::STAGING_LAYERS]{};

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

        // Per-wall count: uniform [lo, hi] inclusive, from WALL_ART.
        uint32_t count_range = WALL_ART.per_wall_count_hi - WALL_ART.per_wall_count_lo + 1;
        uint32_t count = WALL_ART.per_wall_count_lo + cpu_hash(w_seed, WallArtProp::WALL_PAINTING_COUNT) % count_range;

        // Keep paintings away from corners — WALL_ART knob.
        float usable_span = std::max(wall.span - 2.0f * WALL_ART.corner_margin,
            wall.span * 0.3f);

        // ─── Pre-compute widths to center the group on the wall ──
        float total_width = 0.0f;
        float painting_widths[8]{};
        float painting_heights[8]{};
        uint32_t effective_count = std::min(count, 8u);

        for (uint32_t p = 0; p < effective_count; p++) {
            uint32_t p_seed = cpu_hash(w_seed, WallArtProp::PER_PAINTING_BASE + p * WallArtProp::PER_PAINTING_STRIDE);

            // Scale selection (weighted)
            float w[INDOOR_SCALE_COUNT];
            for (uint32_t si = 0; si < INDOOR_SCALE_COUNT; si++) w[si] = INDOOR_SCALES[si]->weight;
            uint32_t scale_idx = select_tier(p_seed, WallPaintingProp::SCALE_ROLL, w, INDOOR_SCALE_COUNT);
            float h = INDOOR_SCALES[scale_idx]->height_lo
                + cpu_hash_f(p_seed, WallPaintingProp::HEIGHT_JITTER) * (INDOOR_SCALES[scale_idx]->height_hi - INDOOR_SCALES[scale_idx]->height_lo);
            painting_heights[p] = h;

            // Estimate width from typical aspect ratio (~1.3)
            float est_aspect = 0.8f + cpu_hash_f(p_seed, WallPaintingProp::ASPECT_ESTIMATE) * 0.8f;  // [0.8, 1.6]
            painting_widths[p] = h * est_aspect;
            total_width += painting_widths[p];
            if (p > 0) total_width += WALL_ART.painting_gap;
        }

        // Trim paintings that don't fit
        while (effective_count > 1 && total_width > usable_span) {
            total_width -= painting_widths[effective_count - 1];
            total_width -= WALL_ART.painting_gap;
            effective_count--;
        }

        // Center the group on the wall
        float group_start = wall_center - total_width * 0.5f;
        float cursor = group_start;

        for (uint32_t p = 0; p < effective_count; p++) {

            uint32_t slot = find_free_painting_slot(gs);
            if (slot == UINT32_MAX) return;

            uint32_t p_seed = cpu_hash(w_seed, WallArtProp::PER_PAINTING_BASE + p * WallArtProp::PER_PAINTING_STRIDE);

            // Vertical position: scale-dependent offset from base
            // Intimate pieces: hung higher. Statement pieces: anchored lower.
            float w[INDOOR_SCALE_COUNT];
            for (uint32_t si = 0; si < INDOOR_SCALE_COUNT; si++) w[si] = INDOOR_SCALES[si]->weight;
            uint32_t scale_idx = select_tier(p_seed, WallPaintingProp::SCALE_ROLL, w, INDOOR_SCALE_COUNT);

            // Y-offset: uniform [lo, hi] per bucket from WALL_ART.
            const auto& bucket = *INDOOR_SCALES[scale_idx];
            float y_offset = bucket.y_offset_lo
                + cpu_hash_f(p_seed, WallPaintingProp::Y_OFFSET_JITTER) * (bucket.y_offset_hi - bucket.y_offset_lo);

            float py = wall.py + y_offset;

            float h_for_clamp = painting_heights[p];
            float bottom = py - h_for_clamp * 0.5f;
            if (bottom > WALL_ART.max_bottom_height) {
                py = WALL_ART.max_bottom_height + h_for_clamp * 0.5f;
            }

            // ─── Content decision (three-way) ────────────────
            bool use_snapshot = (site_type == IndoorSiteType::SNAPSHOT_ONLY)
                || (site_type == IndoorSiteType::MIXED
                    && cpu_hash_f(p_seed, WallPaintingProp::MIX_SNAPSHOT_ROLL) < WALL_ART.mix_snapshot_chance);

            if (!use_snapshot && count_unused_authored(gs, usedAuthored) == 0) {
                use_snapshot = true;
            }

            auto& s = gs.painting_slots[slot];
            float paint_width = 0.0f;  // will be set by whichever path fills the slot

            if (use_snapshot) {
                uint32_t snap_stg = UINT32_MAX;
                for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) {
                    if (gs.snapshot_staging[i].valid && !gs.snapshot_staging[i].consumed) {
                        snap_stg = i;
                        break;
                    }
                }
                if (snap_stg == UINT32_MAX) {
                    if (count_unused_authored(gs, usedAuthored) == 0) continue;
                    use_snapshot = false;
                }
                else {
                    uint32_t exh = find_free_exhibition_layer(gs);
                    if (exh == UINT32_MAX) return;

                    const auto& snap = gs.snapshot_staging[snap_stg];
                    float height = painting_heights[p];
                    paint_width = height * snap.aspect_ratio;

                    // Safety check: actual width may differ from estimate
                    float wall_right = wall_center + usable_span * 0.5f;
                    if (cursor + paint_width > wall_right) break;

                    float paint_center = cursor + paint_width * 0.5f;
                    float px = wall.px + wall.tx * (paint_center - wall_center);
                    float pz = wall.pz + wall.tz * (paint_center - wall_center);

                    fill_slot_wall_frame(s,
                        px, py, pz,
                        wall.nx, wall.ny, wall.nz,
                        snap.aspect_ratio, height,
                        exh, ContentSource::SNAPSHOT,
                        1.0f, 1.0f,
                        FRAME_SNAPSHOT,
                        INT32_MAX, INT32_MAX);

                    gs.exhibition_occupied[exh] = true;
                    gs.exhibition_count++;
                    gs.snapshot_staging[snap_stg].consumed = true;
                    queue_promotion(gs, true, snap_stg, exh);

                    cursor += paint_width + WALL_ART.painting_gap;
                    c->gpuState_.upload_painting_slot(queue, slot, s);
                    gs.wall_frame_count++;
                    continue;
                }
            }

            if (!use_snapshot) {
                uint32_t auth_stg = pick_authored_staging(gs, p_seed, WallPaintingProp::AUTH_STG_PICK);
                if (auth_stg == UINT32_MAX) continue;
                if (usedAuthored[auth_stg]) {
                    uint32_t best = UINT32_MAX, best_disk = UINT32_MAX;
                    for (uint32_t a = 0; a < Dim::STAGING_LAYERS; a++) {
                        if (!usedAuthored[a] && gs.authored_staging[a].valid && !gs.authored_staging[a].consumed
                            && gs.authored_staging[a].disk_index < best_disk) {
                            best_disk = gs.authored_staging[a].disk_index;
                            best = a;
                        }
                    }
                    if (best == UINT32_MAX) continue;
                    auth_stg = best;
                }

                uint32_t exh = find_free_exhibition_layer(gs);
                if (exh == UINT32_MAX) return;

                usedAuthored[auth_stg] = true;

                const auto& img = gs.authored_staging[auth_stg];
                float height = painting_heights[p];
                paint_width = height * img.aspect_ratio;

                // Safety check: actual width may differ from estimate
                float wall_right = wall_center + usable_span * 0.5f;
                if (cursor + paint_width > wall_right) break;

                float paint_center = cursor + paint_width * 0.5f;
                float px = wall.px + wall.tx * (paint_center - wall_center);
                float pz = wall.pz + wall.tz * (paint_center - wall_center);

                fill_slot_wall_frame(s,
                    px, py, pz,
                    wall.nx, wall.ny, wall.nz,
                    img.aspect_ratio, height,
                    exh, ContentSource::AUTHORED,
                    img.uv_scale_x, img.uv_scale_y,
                    FRAME_AUTHORED,
                    INT32_MAX, INT32_MAX);

                gs.exhibition_occupied[exh] = true;
                gs.exhibition_count++;
                gs.authored_staging[auth_stg].consumed = true;
                queue_promotion(gs, false, auth_stg, exh);

                cursor += paint_width + WALL_ART.painting_gap;
                c->gpuState_.upload_painting_slot(queue, slot, s);
                gs.wall_frame_count++;
            }
        }
    }

    const char* site_type_name = (site_type == IndoorSiteType::SNAPSHOT_ONLY) ? "SNAPSHOT"
        : (site_type == IndoorSiteType::MIXED) ? "MIXED" : "AUTHORED";
    // DIAG-unwrapped (census: constitution §5): autonomous stdout
    // (mood apply) — wrap in #ifdef DIAG_GALLERY at ship.
    std::cout << "[WallPainting] Placed " << gs.wall_frame_count
        << " frame(s) across " << active_wall_count << " walls"
        << " (" << site_type_name << ")\n";
}

inline void clear_wall_paintings(GalleryState& gs, GalleryDeps* c, wgpu::Queue& queue) {
    for (uint32_t i = 0; i < Dim::PAINTING_MAX_SLOTS; i++) {
        if (gs.painting_slots[i].is_active != 0 &&
            gs.painting_slots[i].form_type == FormType::WALL_FRAME) {
            uint32_t exh = gs.painting_slots[i].texture_layer;
            if (exh < Dim::EXHIBITION_LAYERS) {
                gs.exhibition_occupied[exh] = false;
                gs.exhibition_count--;
            }
            gs.painting_slots[i].is_active = 0;
            c->gpuState_.deactivate_painting_slot(queue, i);
        }
    }
    gs.wall_frame_count = 0;
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
        self->gallery_state_.gallery_centers[pe.gallery.slot].active = false;
#ifdef DIAG_ENTITY_LIFECYCLE
        std::cout << "[DIAG:REJECT] gall slot=" << pe.gallery.slot
            << " host=(" << pe.gallery.host_gx << "," << pe.gallery.host_gz
            << ") -- no host patch\n";
#endif
    }
}

// ═══ THE EVICTOR ══════════════════════════════════════════════════

inline void evict_gallery(MachineCtx* self,
    uint32_t slot, wgpu::Queue& queue) {
    auto& gc = self->gallery_state_.gallery_centers[slot];
    if (gc.active) {
        evict_paintings_for_patch(self->gallery_state_, self, gc.patch_gx, gc.patch_gz, queue);
        gc.active = false;
    }
#ifdef DIAG_ENTITY_LIFECYCLE
    std::cout << "[DIAG:EVICT]   gall slot=" << slot << "\n";
#endif
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
    gs.exhibition_count = 0;
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
