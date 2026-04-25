#pragma once

// THE_BOARD — Generative world engine. CPU orchestration.
// See world.wgsl for GPU-side (single source of truth).

#include "render/render_cartridge.hpp"
#include "core/input_event.hpp"
#include "cartridges/the_board/state.hpp"
#include "cartridges/the_board/renderer.hpp"
#include <cmath>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <filesystem>
#include <algorithm>
#include <string>
#include <vector>
#include "external/stb_image.h"

namespace t7 {
    namespace the_board {

        class Cartridge : public RenderCartridge {

        private:

            wgpu::Device device_;
            wgpu::TextureFormat colorFormat_;
            wgpu::TextureFormat depthFormat_;

            GPUState gpuState_;
            Renderer renderer_;

            struct InputState {
                float move_x = 0.0f;
                float move_z = 0.0f;
                float look_az_delta = 0.0f;
                float look_el_delta = 0.0f;
                float zoom_delta = 0.0f;
                float pan_x_delta = 0.0f;
                float pan_y_delta = 0.0f;
            } inputState_;

            struct KeyState {
                bool forward = false;
                bool backward = false;
                bool left = false;
                bool right = false;
            } keys_;

            struct MouseState {
                bool left_dragging = false;
                bool right_dragging = false;
            } mouse_;

            bool fpvMode_ = false;
            float currentBeats_ = 0.0f;
            float currentSeconds_ = 0.0f;
            float currentDt_ = 0.016f;

            // Sun + atmosphere (driven by active mood — see apply_mood)
            float sunDirection_[3] = { 0.69f, -0.71f, -0.14f };
            float sunColor_[3] = { 1.0f, 0.95f, 0.9f };
            float sunIntensity_ = 0.8f;
            float sunAmbient_ = 0.25f;
            float clearColor_[3] = { 0.85f, 0.78f, 0.72f };
            uint32_t activeMood_ = 0;
            float terrainAmpCeiling_ = 0.0f;    // mirrors GPU config.terrain_amp_ceiling

            // ── Musical Coupling State (modules/musical.inl) ──
#include "modules/musical.inl"

            // ── Player State (unified entity layer) ──
            //
            // The player's relationship to the world, not a physical body.
            // The body lives in agentStateBuffer_[possessed_slot]; this
            // struct is what travels with the player on possession
            // transfer (Caps Lock). Pass 1 Step 1: only possessed_slot
            // is authoritative here; aura/mmodes still live in their
            // respective modules. Future passes consolidate.
            //
            // See agent_system_design.md §2.1 for the full design.
            struct PlayerState {
                uint32_t possessed_slot = 0;   // slot in agent_state[] that the player inhabits
                // Future (deferred):
                //   uint32_t active_couplings;         // COUPLING_* bitmask owned by player
                //   float    aura_presence;            // migrated from auraPresence_
                //   float    mmode_intensities[MMODE_COUNT];  // migrated from mmodeIntensity_
            };
            PlayerState player_{};

            GPUSpotLightArray cpuSpotLights_{};  // count=0 disables (outdoor)
            bool spotLightActive_ = false;

            // --- World Transition State Machine ---
            enum class TransitionPhase { IDLE, FADE_OUT, TEARDOWN, FADE_IN };
            TransitionPhase transitionPhase_ = TransitionPhase::IDLE;
            float transitionTimer_ = 0.0f;
            float transitionFadeDuration_ = 0.5f;   // seconds per fade direction
            float transitionFadeAlpha_ = 0.0f;

            // Portal destination — describes the world a door leads to.
            // Also used as the pending transition target (keys + portal crossings).
            struct PortalDestination {
                uint32_t seed = 0;
                bool finite = false;
                uint32_t finite_radius = 2;
                uint32_t mood = 0;               // 0=open, 1=finite (expandable)
            };
            PortalDestination pendingDestination_{};

            // --- Finite patch mode ---
            bool finiteMode_ = false;
            uint32_t finiteRadius_ = 2;              // 2 → 5×5 = 25 patches

            // --- Portal detection ---
            static constexpr float PORTAL_DENSITY = 1.00f;  // fraction of Doorway arches that become portals (was 0.25)

            // Portal color by mood (indexed by destination.mood)
            static constexpr float PORTAL_COLORS[6][3] = {
                { 0.90f, 0.45f, 0.70f },  // mood 0  open_default    — pink
                { 0.72f, 0.45f, 0.85f },  // mood 1  open_sunset     — lilac
                { 0.95f, 0.55f, 0.15f },  // mood 2  indoor_flat     — orange
                { 0.95f, 0.80f, 0.20f },  // mood 3  indoor_vault    — yellow
                { 0.85f, 0.20f, 0.15f },  // mood 4  finite_outdoor  — red
                { 0.70f, 0.15f, 0.12f },  // mood 5  finite_outdoor_ref — dark red
            };
            static constexpr float PORTAL_COLOR_BACK[3] = { 0.35f, 0.55f, 0.90f };  // back-portal — blue

            // ─── Mood System ─────────────────────────────────────────────
            //
            // Each mood defines an atmosphere: sun direction/color, fog,
            // finite vs. open, patch radius. Portals pick a mood for
            // their destination; the mood is applied during teardown.
            //
            // Moods 0-1: infinite outdoor.  Moods 2-3: finite indoor.  Mood 4: finite outdoor.  Mood 5: finite outdoor (reference clone).

            enum class CeilingType : uint32_t {
                NONE = 0,   // outdoor — no shell geometry
                FLAT = 1,   // flat slab ceiling
                VAULT = 2,   // catenary vault ceiling
            };

            struct MoodProfile {
                // ─── World bounds ───────────────────────────────────────
                bool   finite;                 // true = walled world with finite radius
                uint32_t finite_radius_min;    // min patch radius (when finite)
                uint32_t finite_radius_max;    // max patch radius (when finite)

                // ─── Lighting ───────────────────────────────────────────
                float  sun_direction[3];       // directional light vector (normalized)
                float  sun_color[3];           // sun RGB
                float  sun_intensity;          // diffuse strength
                float  sun_ambient;            // ambient fill strength

                // ─── Atmosphere ─────────────────────────────────────────
                float  fog_density;            // exponential fog coefficient
                float  fog_color[3];           // fog/horizon RGB

                // ─── Indoor shell ───────────────────────────────────────
                bool   indoor;                 // true = enclosed space with ceiling
                CeilingType ceiling_type;      // NONE / FLAT / VAULT
                float  ceiling_height;         // ceiling Y (world units)

                // ─── Background ─────────────────────────────────────────
                float  clear_color[3];         // sky or dark ceiling RGB
                float  wall_color[3];          // indoor wall surface RGB
                float  ceiling_color[3];       // indoor ceiling surface RGB

                // ─── Feature selection (per-mood) ───────────────────────
                // Each mood independently declares which systems are active.
                // Not tied to indoor/outdoor — a walled mood can still have
                // musical modes, an open mood can still skip pawn aura.
                bool   allow_musical_modes;    // mode_* config values + terrain wave overlay
                bool   allow_gol_zones;        // GoL zone spawning + visualization
                bool   allow_pawn_aura;        // toroidal spring grid tinting + height boost
                bool   allow_frustum_cull;     // GPU frustum cull for LOD0 terrain (Tier 4)

                // Sky orb config is a parallel table (ORB_MOOD_TABLE, below),
                // indexed by the same mood index as MOOD_TABLE. See orbs.inl
                // for the OrbMoodConfig field definitions.
            };

            static constexpr uint32_t MOOD_COUNT = 6;

            // ─── Mood Definitions ───────────────────────────────────────────
            //
            // Sky orb config lives in ORB_MOOD_TABLE below, indexed by the
            // same mood index.
            //
            //                                  fin  r_min r_max  sun_dir                sun_color              int   amb   fog_d   fog_color               indoor  ceil       ceil_h  clear_color            wall_color             ceil_color               modes   zones  aura   cull
            static constexpr MoodProfile MOOD_TABLE[MOOD_COUNT] = {
                /* 0  open_default        */  { false, 2, 2, { 0.69f,-0.71f,-0.14f}, {1.0f, 0.95f, 0.90f}, 0.80f, 0.25f, 0.0030f, {0.85f, 0.78f, 0.72f},  false, CeilingType::NONE,  0.0f,  {0.85f, 0.78f, 0.72f}, {0.75f,0.68f,0.60f}, {0.75f,0.68f,0.60f},   true,  true,  true,  true  },
                /* 1  open_sunset         */  { false, 2, 2, { 0.96f,-0.26f,-0.13f}, {1.0f, 0.75f, 0.45f}, 0.90f, 0.20f, 0.0050f, {0.95f, 0.70f, 0.45f},  false, CeilingType::NONE,  0.0f,  {0.95f, 0.70f, 0.45f}, {0.75f,0.68f,0.60f}, {0.75f,0.68f,0.60f},   true,  true,  true,  true  },
                /* 2  indoor_flat         */  { true,  1, 4, { 0.20f,-0.90f, 0.00f}, {1.0f, 0.90f, 0.80f}, 0.35f, 0.35f, 0.0003f, {0.15f, 0.12f, 0.10f},  true,  CeilingType::FLAT,  20.0f, {0.15f, 0.12f, 0.10f}, {0.65f,0.58f,0.50f}, {0.60f,0.55f,0.48f},   true,  true,  true,  false },
                /* 3  indoor_vault        */  { true,  1, 4, { 0.20f,-0.90f, 0.00f}, {1.0f, 0.90f, 0.80f}, 0.35f, 0.35f, 0.0003f, {0.15f, 0.12f, 0.10f},  true,  CeilingType::VAULT, 25.0f, {0.15f, 0.12f, 0.10f}, {0.70f,0.62f,0.52f}, {0.65f,0.58f,0.50f},   true,  true,  true,  false },
                /* 4  finite_outdoor      */  { true,  1, 4, { 0.69f,-0.71f,-0.14f}, {1.0f, 0.95f, 0.90f}, 0.80f, 0.25f, 0.0030f, {0.85f, 0.78f, 0.72f},  false, CeilingType::NONE,  0.0f,  {0.85f, 0.78f, 0.72f}, {0.75f,0.68f,0.60f}, {0.75f,0.68f,0.60f},   true,  true,  true,  true  },
                /* 5  finite_outdoor_ref  */  { true,  1, 4, { 0.69f,-0.71f,-0.14f}, {1.0f, 0.95f, 0.90f}, 0.80f, 0.25f, 0.0030f, {0.85f, 0.78f, 0.72f},  false, CeilingType::NONE,  0.0f,  {0.85f, 0.78f, 0.72f}, {0.75f,0.68f,0.60f}, {0.75f,0.68f,0.60f},   true,  true,  true,  true  },
            };

            // Orb mood config lives in ORB_MOOD_TABLE, declared right after
            // the #include "modules/orbs.inl" below (where OrbMoodConfig is
            // defined). Same MOOD_COUNT size, indexed by the same mood index.

            static const char* mood_name(uint32_t mood) {
                static const char* NAMES[] = { "open_default", "open_sunset", "indoor_flat", "indoor_vault", "finite_outdoor", "finite_outdoor_ref" };
                return (mood < MOOD_COUNT) ? NAMES[mood] : "unknown";
            }

            // ─── Indoor Lighting Schemes ─────────────────────────────────
            //
            // Seed-driven procedural lighting for indoor moods. Each scheme
            // defines a lighting character (which surfaces carry lights,
            // how many, primary vs accent roles). Per-light parameters
            // (position along surface, intensity, cone width, color warmth)
            // are derived from activeSeed_ at mood transition time.
            //
            // Three schemes:
            //   Cathedral — ceiling primary + two opposing wall sconces
            //   Gallery   — two opposing wall lights, no ceiling (dramatic)
            //   Sanctum   — single source, maximum contrast
            //
            // The seed also picks which wall pair (N/S or E/W) carries the
            // sconces, so rooms with the same scheme still feel different.

            enum class LightAnchor : uint32_t {
                CEILING, WALL_NORTH, WALL_SOUTH, WALL_EAST, WALL_WEST
            };

            struct IndoorLightProp {
                static constexpr uint32_t SCHEME = 1100u;
                static constexpr uint32_t WALL_PAIR = 1101u;
                static constexpr uint32_t ANCHOR_PICK = 1102u;
                static constexpr uint32_t SLOT_BASE = 1110u;  // + slot*10 + field
                // Per-slot field offsets
                static constexpr uint32_t LATERAL = 0u;
                static constexpr uint32_t HEIGHT = 1u;
                static constexpr uint32_t INTENSITY = 2u;
                static constexpr uint32_t INNER_CONE = 3u;
                static constexpr uint32_t OUTER_CONE = 4u;
                static constexpr uint32_t WARMTH = 5u;
                static constexpr uint32_t AIM_PITCH = 6u;
                static constexpr uint32_t AIM_YAW = 7u;
            };

            // Slot definition: anchor surface + gaussian ranges for the
            // light's character. Position slide uses fixed sigmas.
            // Direction is fully parameterised per slot:
            //   aim_pitch — angle below horizontal (wall) or off-vertical (ceiling), radians
            //   aim_yaw   — lateral rotation along the anchor surface, radians
            struct LightSlotDef {
                LightAnchor anchor;
                float intensity_mean, intensity_sigma;
                float inner_mean, inner_sigma;    // inner half-angle (radians)
                float outer_mean, outer_sigma;    // outer half-angle (radians)
                float warmth_mean, warmth_sigma;  // 0 = warm amber, 1 = cool blue
                float aim_pitch_mean, aim_pitch_sigma;  // radians
                float aim_yaw_mean, aim_yaw_sigma;      // radians
            };

            static constexpr float SCHEME_WEIGHTS[] = { 0.55f, 0.25f, 0.20f };
            static constexpr uint32_t SCHEME_COUNT = 3;
            static constexpr const char* SCHEME_NAMES[] = { "Cathedral", "Gallery", "Sanctum" };
            static constexpr const char* ANCHOR_NAMES[] = { "ceiling", "wall_N", "wall_S", "wall_E", "wall_W" };

            // ─── Lighting Scheme Table ───────────────────────────────────────
            //
            // Constexpr tier matrix for indoor lighting.
            // AnchorRole is resolved to a concrete LightAnchor at runtime
            // (WALL_A/B → N/S or E/W depending on seed-driven wall pair).
            //
            // To tune a scheme: adjust its rows. To add a scheme: add a block
            // + SCHEME_COUNT + SCHEME_WEIGHTS entry.

            enum class AnchorRole : uint32_t {
                CEILING,    // always ceiling
                WALL_A,     // seed-selected wall pair, side A
                WALL_B,     // seed-selected wall pair, side B
                SEED_PICK   // anchor chosen from seed (ceiling or wall)
            };

            struct LightSchemeSlot {
                AnchorRole role;
                float intensity_mean, intensity_sigma;
                float inner_mean, inner_sigma;
                float outer_mean, outer_sigma;
                float warmth_mean, warmth_sigma;
                float aim_pitch_mean, aim_pitch_sigma;
                float aim_yaw_mean, aim_yaw_sigma;
            };

            struct LightScheme {
                uint32_t slot_count;
                LightSchemeSlot slots[4];  // MAX_SPOT_LIGHTS
            };

            // ─── Scheme Definitions ─────────────────────────────────────────
            //
            //                                role            int_μ  int_σ  inn_μ inn_σ out_μ out_σ wrm_μ wrm_σ  pit_μ  pit_σ yaw_μ  yaw_σ
            static constexpr LightScheme LIGHT_SCHEMES[SCHEME_COUNT] = {
                /* 0: Cathedral — ceiling primary + 2 wall sconces */
                { 3, {
                    { AnchorRole::CEILING,   8.0f, 2.5f, 0.6f, 0.2f,  1.2f, 0.15f, 0.35f, 0.20f,  0.0f,  0.12f, 0.0f, 0.12f },
                    { AnchorRole::WALL_A,    5.0f, 1.5f, 0.4f, 0.15f, 1.0f, 0.15f, 0.20f, 0.15f,  0.60f, 0.40f, 0.0f, 0.30f },
                    { AnchorRole::WALL_B,    5.0f, 1.5f, 0.4f, 0.15f, 1.0f, 0.15f, 0.75f, 0.15f,  0.60f, 0.40f, 0.0f, 0.30f },
                }},
                /* 1: Gallery — 2 opposing wall lights, no ceiling */
                { 2, {
                    { AnchorRole::WALL_A,    7.0f, 2.0f, 0.4f, 0.15f, 1.1f, 0.15f, 0.25f, 0.20f,  0.55f, 0.45f, 0.0f, 0.35f },
                    { AnchorRole::WALL_B,    7.0f, 2.0f, 0.4f, 0.15f, 1.1f, 0.15f, 0.65f, 0.20f,  0.55f, 0.45f, 0.0f, 0.35f },
                }},
                /* 2: Sanctum — single dramatic source */
                { 1, {
                    { AnchorRole::SEED_PICK, 10.0f, 2.5f, 0.5f, 0.2f, 1.2f, 0.15f, 0.45f, 0.25f,  0.50f, 0.40f, 0.0f, 0.30f },
                }},
            };

            GPUPortalArray cpuPortalArray_{};
            bool portalsDirty_ = true;   // true at boot → first upload guaranteed

            // --- Back-portal (guaranteed exit from finite worlds) ---
            // Position is configurable so special-case layouts can relocate it.
            float backPortalPosition_[2] = { 10.0f, 0.0f };   // world XZ
            bool  backPortalPending_ = false;
            uint32_t backPortalReturnSeed_ = 0;
            uint32_t backPortalReturnMood_ = 0;
            uint32_t backPortalReturnRadius_ = 2;

            // GPU agent-state readback machine: IDLE → COPIED → MAPPING → IDLE
            // Reads the full agent_state buffer (MAX_AGENTS × GPUAgentState).
            // Consumers: patch streaming / ribbon / photographer (possessed
            // slot's XZ), portal triggers (possessed slot's portal_trigger),
            // Caps Lock nearest-agent query (all slots, Step 7).
            enum class PawnReadbackState { IDLE, COPIED, MAPPING };
            PawnReadbackState pawnReadbackState_ = PawnReadbackState::IDLE;
            int32_t readbackPortalTrigger_ = -1;
            float pawnReadback_x_ = 0.0f;
            float pawnReadback_z_ = 0.0f;

            // --- Unified Pier System ─────────────────────────────────────────
            //
            // Deterministic slot addressing: test rig at 0-2, arch piers at 4-35,
            // column piers at 36-67. CPU mirrors the GPU buffer for dead-reckoning
            // step-height checks. No allocator — slot = f(entity_slot).
            GPUPierInstance cpuPiers_[Dim::PIER_TOTAL]{};

            // ── Seed Utilities (modules/seed_utils.inl) ──────────────────────────
            // ═══ INLINED: modules/seed_utils.inl ═══════════════════════════════

// ─── seed_utils.inl ──────────────────────────────────────────────
//
// Pure math. Hash, Gaussian, tier selection.
// No member state. No domain knowledge.
// Every layer below depends on these; they depend on nothing.
//
// Included inside the Cartridge class body.
// ─────────────────────────────────────────────────────────────────

// Hashing utilities (mirror GPU hash functions for determinism)
            static uint32_t cpu_hash(uint32_t seed, uint32_t property) {
                uint32_t h = seed * 747796405u + property * 2891336453u + 1u;
                h = ((h >> 16) ^ h) * 2654435769u;
                h = ((h >> 16) ^ h) * 2654435769u;
                h = (h >> 16) ^ h;
                return h;
            }

            static float cpu_hash_f(uint32_t seed, uint32_t property) {
                return (float)cpu_hash(seed, property) / (float)0xFFFFFFFFu;
            }

            static uint32_t tile_seed(uint32_t master_seed, int32_t gx, int32_t gz) {
                uint32_t h = master_seed;
                h ^= (uint32_t)gx * 73856093u;
                h ^= (uint32_t)gz * 19349663u;
                h = (h ^ (h >> 16)) * 2654435769u;
                h = (h ^ (h >> 16)) * 2654435769u;
                return h;
            }

            // CPU mirror of WGSL lattice_node_seed (must produce identical results)
            static uint32_t cpu_lattice_node_seed(uint32_t master_seed, int32_t nx, int32_t nz, uint32_t band) {
                uint32_t h = master_seed;
                h ^= (uint32_t)nx * 73856093u;
                h ^= (uint32_t)nz * 19349663u;
                h ^= band * 83492791u;
                h = (h ^ (h >> 16)) * 2654435769u;
                h = (h ^ (h >> 16)) * 2654435769u;
                return h;
            }

            static float cpu_smoothstep(float e0, float e1, float x) {
                float t = std::max(0.0f, std::min(1.0f, (x - e0) / (e1 - e0)));
                return t * t * (3.0f - 2.0f * t);
            }

            // CPU-side Gaussian sampling that mirrors the WGSL sample_gaussian exactly.
            // (seed, property) → Box-Muller → truncated at ±3σ.
            static float cpu_sample_gaussian(uint32_t seed, uint32_t property, float mean, float sigma) {
                float u1 = std::max(cpu_hash_f(seed, property), 1e-6f);
                float u2 = cpu_hash_f(seed, property + 1000u);  // matches GAUSSIAN_PAIR_OFFSET
                float z = std::sqrt(-2.0f * std::log(u1)) * std::cos(2.0f * 3.14159265359f * u2);
                z = std::max(-3.0f, std::min(3.0f, z));
                return mean + z * sigma;
            }

            // Weighted tier selection from cumulative weights.
            static uint32_t select_tier(uint32_t seed, uint32_t tier_prop,
                const float* weights, uint32_t count) {
                float roll = cpu_hash_f(seed, tier_prop);
                float cumul = 0.0f;
                for (uint32_t t = 0; t < count; t++) {
                    cumul += weights[t];
                    if (roll < cumul) return t;
                }
                return count - 1;
            }

            // ═══ END INLINED: modules/seed_utils.inl ═════════════════════════

            // ── Terrain CPU mirror deleted ────────────────────────────────
            // GPU is single source of truth for entity ground_y.
            // compute_entity_placement samples the heightfield directly.
            // Only estimate_terrain_height (tileCache_ lookup) survives for ribbon.

            // ── Entity Type Definitions (modules/entities.inl) ──
            // ═══ INLINED: modules/entities.inl ═══════════════════════════════

// ─── entities.inl ────────────────────────────────────────────────
//
// The vocabulary of forms: Ribbon, Arch, Column, Pyramid.
// Tier profiles, property registries, tracking structs.
//
// Included inside the Cartridge class body.
// Depends on: seed_utils.inl
// ─────────────────────────────────────────────────────────────────


//
// Ribbons exist at deterministic world locations on a coarse grid (~600 units).
// Each cell has a probability of containing a ribbon. When the pawn is within
// render distance, the ribbon activates with seed-derived parameters.
//
// Flying ribbons are alive, high in the sky, with animated waves.
//
// Cube count: 100–400. Cube size: pawn height (1.5) to 4× (6.0).
// Flying height: 50–80 units. All wave params independently seeded.

// ─── Sky Ribbon System ─────────────────────────────────────────
//
// Flying ribbons: compound wave functions (lateral + vertical + twist)
// forming square-tube geometry in the sky. Each ribbon is a tier
// instance with Gaussian-sampled parameters.
//
// Architecture follows the entity pattern:
//   RibbonProp        — property index registry
//   RibbonColorMode   — color tier weights
//   RibbonTierProfile — mean+sigma for all wave/geometry parameters

            // ─── Ribbon dispatch-pipeline config ────────────────────────────
            struct RibbonConfig {
                static constexpr float SPAWN_CHANCE = 0.400f;
                static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
                static constexpr float POSITION_JITTER = 0.3f;
            };

            // ── Color Modes ──────────────────────────────────────────────────
            struct RibbonColorMode {
                static constexpr uint32_t SMOOTH = 0;  // terrain-derived monochrome
                static constexpr uint32_t TINTED = 1;  // warm/cool hue shift
                static constexpr uint32_t CONTRAST = 2;  // high-contrast alternating segments
                static constexpr uint32_t COUNT = 3;
                static constexpr float WEIGHTS[COUNT] = { 0.40f, 0.35f, 0.25f };
            };

            // Smooth color palettes: base colors for SMOOTH mode ribbons
            static constexpr float RIBBON_SMOOTH_PALETTE[][3] = {
                { 0.82f, 0.75f, 0.62f },   // warm sandstone
                { 0.55f, 0.65f, 0.78f },   // sky blue
                { 0.85f, 0.78f, 0.58f },   // golden
                { 0.50f, 0.68f, 0.55f },   // sage green
            };
            static constexpr uint32_t RIBBON_SMOOTH_PALETTE_COUNT = 4;

            // ── Property Index Registry ──────────────────────────────────────
            struct RibbonProp {
                static constexpr uint32_t SPAWN_ROLL = 400u;
                static constexpr uint32_t ANCHOR_X = 401u;
                static constexpr uint32_t ANCHOR_Z = 402u;
                static constexpr uint32_t TIER = 403u;
                static constexpr uint32_t COLOR_ROLL = 404u;
                static constexpr uint32_t ORIENTATION = 405u;
                static constexpr uint32_t COLOR_R = 406u;
                static constexpr uint32_t COLOR_G = 407u;
                static constexpr uint32_t COLOR_B = 408u;
                static constexpr uint32_t PALETTE_IDX = 409u;
                // Gaussian draw indices
                static constexpr uint32_t CUBE_COUNT = 410u;
                static constexpr uint32_t CUBE_SIZE = 411u;
                static constexpr uint32_t HEIGHT = 412u;
                static constexpr uint32_t LATERAL_AMP = 420u;
                static constexpr uint32_t LATERAL_CYCLES = 421u;
                static constexpr uint32_t LATERAL_SPEED = 422u;
                static constexpr uint32_t VERTICAL_AMP = 430u;
                static constexpr uint32_t VERTICAL_SPEED = 432u;
                static constexpr uint32_t VERTICAL_RATIO = 433u;    // seed roll for vertical harmonic ratio selection
                static constexpr uint32_t TWIST_AMP = 440u;
                static constexpr uint32_t TWIST_SPEED = 442u;
                static constexpr uint32_t TWIST_RATIO = 443u;       // seed roll for twist harmonic ratio selection
            };

            // ─── Harmonic Ratio Palettes ─────────────────────────────────────
            //
            // Secondary wave cycles (vertical, twist) are derived as simple
            // ratios of the lateral fundamental. This eliminates irrational
            // beating and gives each ribbon a harmonically coherent form.
            //
            // Ratios ≤ 1 keep secondary motion slower than lateral sway
            // (contemplative, not snaky). Weights favor the middle intervals.

            struct HarmonicRatio {
                float ratio;
                float weight;
                const char* name;      // musical interval name for diagnostics
            };

            static constexpr uint32_t VERTICAL_RATIO_COUNT = 4;
            static constexpr HarmonicRatio VERTICAL_RATIOS[VERTICAL_RATIO_COUNT] = {
                { 1.0f / 3.0f,  0.15f, "1:3" },   // breathe at 1/3 of sway
                { 1.0f / 2.0f,  0.35f, "1:2" },   // octave below — one breath per two sways
                { 2.0f / 3.0f,  0.30f, "2:3" },   // fifth below — gentle polyrhythm
                { 1.0f / 1.0f,  0.20f, "1:1" },   // unison — breathe with sway
            };

            static constexpr uint32_t TWIST_RATIO_COUNT = 4;
            static constexpr HarmonicRatio TWIST_RATIOS[TWIST_RATIO_COUNT] = {
                { 1.0f / 4.0f,  0.15f, "1:4" },   // very slow corkscrew
                { 1.0f / 3.0f,  0.30f, "1:3" },   // one turn per three sways
                { 1.0f / 2.0f,  0.35f, "1:2" },   // one turn per two sways
                { 2.0f / 3.0f,  0.20f, "2:3" },   // fifth below
            };

            static uint32_t select_harmonic_ratio(uint32_t seed, uint32_t prop,
                const HarmonicRatio* palette, uint32_t count) {
                float roll = cpu_hash_f(seed, prop);
                float cumul = 0.0f;
                for (uint32_t i = 0; i < count; i++) {
                    cumul += palette[i].weight;
                    if (roll < cumul) return i;
                }
                return count - 1;
            }

            // ── Tier Profile (mean+sigma, matches GoLTierProfile pattern) ────
            static constexpr uint32_t RIBBON_TIER_COUNT = 3;
            static constexpr float RIBBON_BASE_TIER_WEIGHTS[RIBBON_TIER_COUNT] = {
                0.45f, 0.30f, 0.25f
            };

            struct RibbonTierProfile {
                // ─── Geometry ────────────────────────────────────────────
                float cube_count_mean, cube_count_sigma;
                float cube_size_mean, cube_size_sigma;

                // ─── Altitude ────────────────────────────────────────────
                float height_mean, height_sigma;

                // ─── Lateral wave ─────────────────────────────────────
                float lateral_amp_mean, lateral_amp_sigma;
                float lateral_cycles_mean, lateral_cycles_sigma;
                float lateral_speed_mean, lateral_speed_sigma;

                // ─── Vertical wave ────────────────────────────────────
                float vertical_amp_mean, vertical_amp_sigma;
                float vertical_cycles_mean, vertical_cycles_sigma;
                float vertical_speed_mean, vertical_speed_sigma;

                // ─── Twist (corkscrew) ───────────────────────────────────
                float twist_amp_mean, twist_amp_sigma;
                float twist_cycles_mean, twist_cycles_sigma;
                float twist_speed_mean, twist_speed_sigma;

                // ─── Selection ───────────────────────────────────────────
                float weight;
            };

            //                          ┌── Serpentine ──┬──── Helix ─────┬─── Streamer ───┐
            //                          │   μ       σ    │   μ       σ    │   μ       σ    │
            // ─── Geometry ────────────┤                │                │                │
            //   cube_count             │ 350      60    │ 150      40    │ 250      50    │
            //   cube_size              │   8.0     2.0  │   5.0     1.2  │   6.0     1.6  │
            // ─── Altitude ────────────┤                │                │                │
            //   height                 │  60      15    │  55      12    │  70      20    │
            // ─── Lateral wave ────────┤                │                │                │
            //   lateral_amp             │  10.0     0.6  │   3.5     0.6  │   5.5     0.8  │
            //   lateral_cycles          │   1.5     0.4  │   3.0     0.8  │   2.0     0.5  │
            //   lateral_speed           │   0.25   0.075 │   0.60   0.175 │   0.40    0.10 │
            // ─── Vertical wave ───────┤  cycles + speed = lateral                        │
            //   vertical_amp            │   5.0     0.8  │   2.5     0.5  │   8.0     1.2  │
            // ─── Twist (corkscrew) ───┤  cycles + speed = lateral                        │
            //   twist_amp              │   0.6     0.2  │   6.0     0.8  │   1.6     0.6  │
            // ─── Selection ───────────┤                │                │                │
            //   weight                 │   0.45         │   0.30         │   0.25         │
            //                          └────────────────┴────────────────┴────────────────┘
            static constexpr RibbonTierProfile RIBBON_TIERS[RIBBON_TIER_COUNT] = {
                // Tier 0: Serpentine — long, massive, slow motion
                {   350.0f, 60.0f,      // cube_count
                      8.0f,  2.0f,      // cube_size
                     60.0f, 15.0f,      // height
                     10.0f,  0.6f,      // lateral_amp
                      1.5f,  0.4f,      // lateral_cycles
                      0.25f, 0.075f,    // lateral_speed
                      5.0f,  0.8f,      // vertical_amp
                      0.8f,  0.2f,      // vertical_cycles (overridden = lateral)
                      0.20f, 0.06f,     // vertical_speed (overridden = lateral)
                      0.6f,  0.2f,      // twist_amp
                      0.5f,  0.2f,      // twist_cycles (overridden = lateral)
                      0.15f, 0.05f,     // twist_speed (overridden = lateral)
                      0.45f },          // weight
                      // Tier 1: Helix — tighter cycles, visible corkscrew
                      {   150.0f, 40.0f,      // cube_count
                            5.0f,  1.2f,      // cube_size
                           55.0f, 12.0f,      // height
                            3.5f,  0.6f,      // lateral_amp
                            3.0f,  0.8f,      // lateral_cycles
                            0.60f, 0.175f,    // lateral_speed
                            2.5f,  0.5f,      // vertical_amp
                            2.5f,  0.6f,      // vertical_cycles (overridden = lateral)
                            0.50f, 0.15f,     // vertical_speed (overridden = lateral)
                            6.0f,  0.8f,      // twist_amp
                            2.0f,  0.5f,      // twist_cycles (overridden = lateral)
                            0.20f, 0.05f,     // twist_speed (overridden = lateral)
                            0.30f },          // weight
                            // Tier 2: Streamer — tall vertical form, deep breathing
                            {   250.0f, 50.0f,      // cube_count
                                  6.0f,  1.6f,      // cube_size
                                 70.0f, 20.0f,      // height
                                  5.5f,  0.8f,      // lateral_amp
                                  2.0f,  0.5f,      // lateral_cycles
                                  0.40f, 0.10f,     // lateral_speed
                                  8.0f,  1.2f,      // vertical_amp
                                  1.2f,  0.3f,      // vertical_cycles (overridden = lateral)
                                  0.325f, 0.075f,   // vertical_speed (overridden = lateral)
                                  1.6f,  0.6f,      // twist_amp
                                  1.5f,  0.4f,      // twist_cycles (overridden = lateral)
                                  0.25f, 0.08f,     // twist_speed (overridden = lateral)
                                  0.25f },          // weight
            };

            static constexpr const char* RIBBON_TIER_NAMES[] = {
                "Serpentine", "Helix", "Streamer"
            };
            static constexpr const char* RIBBON_COLOR_NAMES[] = {
                "smooth", "tinted", "contrast"
            };

            static constexpr float PAWN_HEIGHT_UNITS = 1.5f;     // matches WGSL PAWN_HEIGHT

            // ─── Generative Catenary Arches ──────────────────────────────────
            //
            // Three tiers: doorway, standard, monumental. Each defined by a
            // parameter row in the ARCH_TIERS matrix (one row = one character).
            //
            // Collision: two pier solids per arch, walkable via step-height.
            // Visual:    CPU-generated barrel vault mesh with per-vertex color.
            // Color:     default = warm sandstone; override = arch palette.
            //
            // Distribution: placeholder — piggybacks on patch streaming,
            // per-archetype probability. Will be replaced once the full
            // object vocabulary exists and placement rules emerge.
            //
            // ─── Pier Sizing Rule ────────────────────────────────────────────
            //
            //   Pier footprint is DERIVED from shell geometry, not independent:
            //     pier_half_x = thickness/2 + pier_padding + edge_blend
            //     pier_half_z = depth/2     + pier_padding + edge_blend
            //
            //   The artist controls pier_padding (extra margin beyond the shell
            //   base). Piers always cover the arch feet with room to spare.

            enum class ArchTier : uint32_t {
                DOORWAY = 0,   // human-scale passage
                STANDARD = 1,   // the arch we started with
                MONUMENTAL = 2,   // cathedral-scale gateway
                COUNT = 3
            };

            // ─── Tier Parameter Struct ───────────────────────────────────────
            //
            // Fields grouped by artistic concern:
            //   Shell:      span, rise, depth, thickness
            //   Piers:      pier_height, pier_padding, edge_blend
            //   Appearance: color_override, burial
            //   Quality:    segs_u, segs_v
            //   Selection:  weight (probability of this tier being chosen)

            struct ArchTierParams {
                // ─── Shell geometry (catenary + barrel vault) ─────────────
                float span_mean, span_sigma;       // full opening width (world units)
                float rise_mean, rise_sigma;       // catenary height above piers
                float depth_mean, depth_sigma;      // barrel vault depth (walk-through distance)
                float thickness_mean, thickness_sigma;   // shell wall thickness

                // ─── Pier solids (collision foundations) ──────────────────
                float pier_height_mean, pier_height_sigma;  // height of solid above terrain
                float pier_padding_mean, pier_padding_sigma; // extra XZ margin beyond shell footprint
                float edge_blend_mean, edge_blend_sigma;    // smoothstep width at solid edges

                // ─── Appearance ──────────────────────────────────────────
                float color_override;   // probability of palette color vs terrain sandstone
                float burial;           // mesh sinks into piers (fraction of pier_height, min 0.2)

                // ─── Quality ─────────────────────────────────────────────
                uint32_t segs_u;        // segments along catenary profile
                uint32_t segs_v;        // segments along barrel depth

                // ─── Selection ───────────────────────────────────────────
                float weight;           // tier selection probability (all must sum to 1.0)
            };

            // ─── Tier Definitions ────────────────────────────────────────────
            //
            // To tune a tier: adjust its row. To add a tier: add a row + enum.
            //
            //                         span   σ    rise   σ    depth  σ    thick  σ     pier_h  σ     pad    σ    blend  σ     col%  burial  su  sv  weight
            static constexpr ArchTierParams ARCH_TIERS[] = {
                /* DOORWAY     */  {  12.0f, 2.4f, 12.0f, 2.4f,  4.5f, 0.9f,  1.2f, 0.18f,   1.5f, 0.9f,  0.9f, 0.3f,  0.9f, 0.15f,  0.15f, 0.20f,  16, 4,  0.50f },
                /* STANDARD    */  {  50.0f, 15.f, 42.0f, 7.0f,  5.6f, 1.1f,  1.4f, 0.21f,   5.6f, 2.1f,  0.7f, 0.3f,  0.7f, 0.14f,  0.25f, 0.20f,  32, 8,  0.15f },
                /* MONUMENTAL  */  {  60.0f, 10.f, 80.0f, 12.f, 10.0f, 2.0f,  2.5f, 0.30f,   8.0f, 2.5f,  1.0f, 0.3f,  0.8f, 0.15f,  0.35f, 0.20f,  48, 12, 0.15f },
            };

            // ─── Color Palette ───────────────────────────────────────────────
            // Paradigm: palette + sandstone fallback.
            // When color_override fires, the arch draws from this palette
            // instead of terrain sandstone. Extend by adding rows.

            static constexpr float ARCH_PALETTE[][3] = {
                { 0.82f, 0.80f, 0.78f },   // 0: light grey stone
            };
            static constexpr uint32_t ARCH_PALETTE_COUNT = 1;

            // Default terrain-derived sandstone (used when no color override)
            static constexpr float ARCH_SANDSTONE_BASE[3] = { 0.75f, 0.68f, 0.60f };
            static constexpr float ARCH_SANDSTONE_VARIANCE = 0.04f;

            // ─── Spawn Configuration ─────────────────────────────────────────
            //
            // These control WHERE arches appear and are deliberately separate
            // from the tier matrix (which controls WHAT each arch looks like).
            // The spawn rules are a placeholder awaiting the full object vocabulary.

            struct ArchConfig {
                // Flat spawn probability — terrain-independent (themes control variation)
                static constexpr float SPAWN_CHANCE = 0.030f;

                // Per-mood spawn multiplier (Bayesian: prior × mood_factor × adjacency_factor)
                static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

                // Position jitter within patch (fraction of PATCH_EXTENT)
                static constexpr float POSITION_JITTER = 0.35f;
            };

            // ─── Property Indices ────────────────────────────────────────────
            //
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

            // ─── Active Arch Tracking ────────────────────────────────────────

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

            ActiveArch activeArches_[Dim::MAX_ARCH_INSTANCES]{};
            uint32_t activeArchCount_ = 0;
            bool archMeshGenPending_ = false;  // true → dispatch GPU mesh gen
            bool lightsDirty_ = true;      // set true at init, cleared after first upload

            // ─── Generative Columns ──────────────────────────────────────────
            //
            // Three classical tiers: pillar, doric, ornate. Each defined by a
            // parameter row in the COLUMN_TIERS matrix. Mesh is a surface of
            // revolution from a profile curve: base layers → shaft (with taper
            // + entasis) → capital layers.
            //
            // Collision: one solid per column (square footprint with edge_blend).
            // Visual:    CPU-generated revolution mesh, per-vertex color.
            // Color:     default = warm sandstone; override = column palette.

            enum class ColumnTier : uint32_t {
                PILLAR = 0,        // thick sturdy post, minimal ornamentation
                DORIC = 1,         // classical proportions, no base, subtle taper
                ORNATE = 2,        // monumental, entasis, layered base + capital
                COUNT = 3
            };

            // ─── Generative Antennas ─────────────────────────────────────────
            //
            // Three tiers: antenna, squat, colossal. Tall posts with stacked
            // drum elements. Shares ColumnTierParams struct (field reuse:
            // base_layers=drum_count, base_height=drum_height,
            // base_overhang=drum_radius_overhang, capital_height=spacer_height).
            //
            // Shares ActiveColumn tracking, mesh gen pipeline, and GPU tier
            // indices (3/4/5) with classical columns.

            enum class AntennaTier : uint32_t {
                ANTENNA = 0,       // tall post with stacked drum elements
                SQUAT = 1,         // wider post + wider/shorter drums
                COLOSSAL = 2,      // massive tower-scale antenna
                COUNT = 3
            };

            // Tier counts (GPU tier indices: 0–2 column, 3–5 antenna)
            static constexpr uint32_t COLUMN_TIER_COUNT = static_cast<uint32_t>(ColumnTier::COUNT);
            static constexpr uint32_t ANTENNA_TIER_COUNT = static_cast<uint32_t>(AntennaTier::COUNT);

            // ─── Tier Parameter Struct ───────────────────────────────────────
            //
            // Fields grouped by artistic concern:
            //   Profile:    height, shaft_radius, taper, entasis
            //   Base:       base_layers, base_height, base_overhang
            //   Capital:    capital_layers, capital_height, capital_overhang
            //   Collision:  solid_radius_padding, solid_height, edge_blend
            //   Appearance: color_override, burial
            //   Quality:    segs_around, shaft_rings
            //   Selection:  weight

            struct ColumnTierParams {
                // ─── Profile geometry (surface of revolution) ─────────────
                float height_mean, height_sigma;       // total column height
                float shaft_radius_mean, shaft_radius_sigma; // shaft radius at base
                float taper_mean, taper_sigma;        // top_radius / bottom_radius (0.8–1.0)
                float entasis_mean, entasis_sigma;      // belly bulge at 1/3 height (fraction of radius)

                // ─── Base (stepped rings below shaft) ────────────────────
                float base_layers_mean, base_layers_sigma;  // number of concentric rings (0–3)
                float base_height_mean, base_height_sigma;  // total height of base section
                float base_overhang_mean, base_overhang_sigma; // extra radius beyond shaft

                // ─── Capital (stepped rings above shaft) ─────────────────
                float capital_layers_mean, capital_layers_sigma;
                float capital_height_mean, capital_height_sigma;
                float capital_overhang_mean, capital_overhang_sigma;

                // ─── Collision solid ─────────────────────────────────────
                float solid_padding_mean, solid_padding_sigma;  // extra radius beyond base
                float solid_height_mean, solid_height_sigma;   // solid height (must exceed step_height)
                float edge_blend_mean, edge_blend_sigma;

                // ─── Appearance ──────────────────────────────────────────
                float color_override;   // probability of palette color
                float burial;           // mesh sinks into solid (fraction of solid_height)

                // ─── Quality ─────────────────────────────────────────────
                uint32_t segs_around;   // revolution segments
                uint32_t shaft_rings;   // vertical subdivisions along shaft

                // ─── Selection ───────────────────────────────────────────
                float weight;
            };

            // ─── Column Tier Definitions ────────────────────────────────────
            //
            //                         h_μ    σ    rad_μ  σ    taper  σ    ent    σ     bL_μ  σ    bH_μ  σ    bO_μ  σ     cL_μ  σ    cH_μ  σ    cO_μ  σ     sp_μ  σ    sH_μ  σ    eb_μ  σ     col%  bur   seg  shR  wt
            static constexpr ColumnTierParams COLUMN_TIERS[] = {
                /* PILLAR  */  {  6.5f, 1.2f,  1.80f, 0.30f,  1.00f, 0.0f,  0.00f, 0.0f,   1.0f, 0.3f,  0.50f, 0.10f,  0.20f, 0.05f,   1.0f, 0.3f,  0.40f, 0.08f,  0.15f, 0.04f,   0.3f, 0.08f,  1.5f, 0.3f,  0.4f, 0.08f,   0.15f, 0.25f,  16, 4,   0.05f },
                /* DORIC   */  {  6.4f, 1.2f,  0.38f, 0.06f,  0.85f, 0.03f, 0.02f, 0.01f,  0.0f, 0.0f,  0.00f, 0.00f,  0.00f, 0.00f,   2.0f, 0.5f,  0.50f, 0.10f,  0.15f, 0.04f,   0.2f, 0.05f,  1.0f, 0.2f,  0.3f, 0.05f,   0.25f, 0.20f,  20, 8,   0.20f },
                /* ORNATE  */  { 16.8f, 2.8f,  1.35f, 0.19f,  0.82f, 0.03f, 0.04f, 0.01f,  2.0f, 0.5f,  1.20f, 0.25f,  0.30f, 0.08f,   3.0f, 0.5f,  1.50f, 0.30f,  0.40f, 0.10f,   0.4f, 0.10f,  1.5f, 0.3f,  0.5f, 0.10f,   0.35f, 0.20f,  28, 12,  0.18f },
            };

            // ─── Antenna Tier Definitions ───────────────────────────────────
            //
            // Field reuse: base_layers=drum_count, base_height=drum_height,
            // base_overhang=drum_radius_overhang, capital_height=spacer_height.
            //
            //                         h_μ    σ    rad_μ  σ    taper  σ    ent    σ     dC_μ  σ    dH_μ  σ    dO_μ  σ     __    __   sH_μ  σ    __    __     sp_μ  σ    sH_μ  σ    eb_μ  σ     col%  bur   seg  shR  wt
            static constexpr ColumnTierParams ANTENNA_TIERS[] = {
                /* ANTENNA */  { 17.5f, 3.5f,  0.30f, 0.05f,  0.85f, 0.05f, 0.00f, 0.0f,   2.0f, 0.5f,  2.1f, 0.42f,   1.5f, 0.3f,    0.0f, 0.0f,  1.5f, 0.3f,   0.0f, 0.0f,    0.2f, 0.05f,  1.0f, 0.2f,  0.3f, 0.05f,   0.40f, 0.20f,  16, 6,   0.10f },
                /* SQUAT   */  { 32.5f, 6.5f,  0.90f, 0.15f,  0.85f, 0.05f, 0.00f, 0.0f,   2.0f, 0.5f,  2.0f, 0.4f,   6.0f, 1.2f,    0.0f, 0.0f,  1.5f, 0.3f,   0.0f, 0.0f,    0.4f, 0.10f,  1.5f, 0.3f,  0.4f, 0.08f,   0.40f, 0.20f,  16, 6,   0.22f },
                /* COLOSSAL */ { 125.0f, 25.0f,  3.00f, 0.50f,  0.85f, 0.05f, 0.00f, 0.0f,   2.0f, 0.5f,  7.5f, 1.5f,  17.5f, 3.5f,    0.0f, 0.0f,  7.5f, 1.5f,   0.0f, 0.0f,    1.95f, 0.39f, 12.0f, 2.4f,  1.0f, 0.20f,   0.40f, 0.20f,  20, 8,   0.13f },
            };

            // ─── Color Palette ───────────────────────────────────────────────
            // Paradigm: palette + sandstone fallback (same as arch).
            // When color_override fires, column draws from this palette.
            // Default: terrain-derived sandstone with per-instance variance.
            static constexpr float COLUMN_PALETTE[][3] = {
                { 0.82f, 0.80f, 0.78f },   // 0: light grey stone
                { 0.88f, 0.83f, 0.72f },   // 1: warm limestone (cream/ivory)
                { 0.55f, 0.58f, 0.63f },   // 2: cool blue-grey slate
                { 0.72f, 0.45f, 0.32f },   // 3: terracotta / burnt sienna
                { 0.90f, 0.87f, 0.82f },   // 4: weathered marble (off-white, subtle warmth)
                { 0.35f, 0.33f, 0.32f },   // 5: dark basalt
                { 0.52f, 0.58f, 0.48f },   // 6: mossy green-grey
            };
            static constexpr uint32_t COLUMN_PALETTE_COUNT = 7;
            static constexpr float COLUMN_SANDSTONE_BASE[3] = { 0.75f, 0.68f, 0.60f };
            static constexpr float COLUMN_SANDSTONE_VARIANCE = 0.04f;

            struct ColumnConfig {
                static constexpr float SPAWN_CHANCE = 0.030f;
                static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };
                static constexpr float POSITION_JITTER = 0.35f;
            };

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

            struct AntennaConfig {
                static constexpr float SPAWN_CHANCE = 0.025f;
                static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };
                static constexpr float POSITION_JITTER = 0.35f;
            };

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

            // ─── Active Column Tracking ──────────────────────────────────────

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

            ActiveColumn activeColumns_[Dim::MAX_COLUMN_ONLY]{};
            ActiveColumn activeAntennas_[Dim::MAX_ANTENNA_ONLY]{};
            uint32_t activeColumnCount_ = 0;
            uint32_t activeAntennaCount_ = 0;
            bool columnMeshGenPending_ = false;  // true → dispatch GPU mesh gen (shared by column + antenna)

            // (generate_column_mesh removed — replaced by GPU compute: column_mesh_gen)

            // ─── Generative Palms ────────────────────────────────────────────
            //
            // Three tiers: Sapling, Coastal, Royal. Tapered trunk with lean +
            // bark rings, crowned with radial fronds. No piers, no collision
            // solids, no heightfield contribution.

            enum class PalmTier : uint32_t { SAPLING = 0, COASTAL = 1, ROYAL = 2, COUNT = 3 };
            static constexpr uint32_t PALM_TIER_COUNT = static_cast<uint32_t>(PalmTier::COUNT);

            struct PalmTierParams {
                // ─── Trunk geometry ─────────────────────────────────────
                float height_mean, height_sigma;         // total trunk height
                float base_r_mean, base_r_sigma;         // radius at ground
                float top_r_mean, top_r_sigma;           // radius at crown
                float lean_mean, lean_sigma;             // trunk lean angle (fraction)

                // ─── Bark texture ───────────────────────────────────────
                float bark_rings_mean, bark_rings_sigma;   // horizontal ring count
                float bark_depth_mean, bark_depth_sigma;   // ring indentation depth

                // ─── Crown (frond array) ────────────────────────────────
                float frond_count_mean, frond_count_sigma; // number of fronds
                float frond_len_mean, frond_len_sigma;     // frond length from crown center
                float frond_width_mean, frond_width_sigma; // frond blade width
                float frond_droop_mean, frond_droop_sigma; // downward droop angle
                float frond_arch_mean, frond_arch_sigma;   // inward arch curvature
                float crown_spread_mean, crown_spread_sigma; // angular spread of frond ring
                float crown_skirt_mean, crown_skirt_sigma;   // dead frond skirt depth

                // ─── Collision solid ────────────────────────────────────
                float solid_padding_mean, solid_padding_sigma; // extra radius beyond trunk
                float edge_blend_mean, edge_blend_sigma;       // smoothstep width at solid edges

                // ─── Appearance ─────────────────────────────────────────
                float color_over;                        // probability of palette color
                float burial;                            // mesh sinks into solid
                float trunk_var, frond_var;              // color variance (trunk, fronds)

                // ─── Quality ────────────────────────────────────────────
                uint32_t trunk_segs, frond_segs;         // mesh subdivision counts

                // ─── Selection ──────────────────────────────────────────
                float weight;                            // tier selection probability
            };

            //                                h_μ    σ    br_μ   σ    tr_μ   σ    lean_μ σ    bark_μ σ    bd_μ   σ     fc_μ  σ    fl_μ  σ    fw_μ  σ    fd_μ  σ    fa_μ  σ    cs_μ  σ    ck_μ  σ    sp_μ  σ    eb_μ  σ    co%  bur  tv   fv   ts  fs   wt
            static constexpr PalmTierParams PALM_TIERS[] = {
                /* SAPLING  */ { 8.0f, 2.0f,  0.25f,0.05f, 0.12f,0.02f, 0.15f,0.05f, 8.0f,2.0f,  0.03f,0.01f, 7.0f,1.0f,  3.0f,0.5f,  0.8f,0.15f, 0.3f,0.1f, 0.4f,0.1f, 0.6f,0.1f, 0.3f,0.1f, 0.5f,0.1f, 0.5f,0.1f, 0.1f, 0.1f, 0.06f,0.04f, 12, 4,  0.50f },
                /* COASTAL  */ { 16.0f,3.0f,  0.40f,0.08f, 0.15f,0.03f, 0.20f,0.08f, 12.0f,3.0f, 0.04f,0.01f, 11.0f,2.0f, 5.0f,0.8f,  1.2f,0.2f,  0.5f,0.15f,0.5f,0.12f,0.7f,0.1f, 0.4f,0.1f, 0.8f,0.15f,0.8f,0.15f,0.15f, 0.15f,0.05f,0.03f, 16, 5,  0.35f },
                /* ROYAL    */ { 28.0f,5.0f,  0.55f,0.10f, 0.18f,0.04f, 0.10f,0.05f, 18.0f,4.0f, 0.05f,0.01f, 15.0f,2.0f, 7.0f,1.0f,  1.5f,0.3f,  0.6f,0.15f,0.6f,0.15f,0.8f,0.1f, 0.5f,0.1f, 1.0f,0.2f, 1.0f,0.2f, 0.20f, 0.2f, 0.04f,0.03f, 20, 6,  0.15f },
            };

            // ─── Color Palette ───────────────────────────────────────────────
            // Paradigm: body-part bases with per-instance variance.
            // Each part (trunk, frond, aged frond) has an independent RGB base.
            // Variance from tier's trunk_var / frond_var applied per-spawn.
            static constexpr float PALM_TRUNK_BASE[3] = { 0.45f, 0.35f, 0.25f };
            static constexpr float PALM_FROND_BASE[3] = { 0.25f, 0.45f, 0.20f };
            static constexpr float PALM_AGED_BASE[3] = { 0.35f, 0.38f, 0.18f };

            struct PalmConfig {
                static constexpr float SPAWN_CHANCE = 0.200f;
                static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };
                static constexpr float POSITION_JITTER = 0.45f;
            };

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

            ActivePalm activePalms_[Dim::MAX_PALM_INSTANCES]{};
            uint32_t activePalmCount_ = 0;
            bool palmMeshGenPending_ = false;

            bool prepare_palm_mesh_gen(wgpu::Queue& queue) {
                if (!palmMeshGenPending_) return false;
                palmMeshGenPending_ = false;
                uint32_t maxSlot = 0;
                bool anyActive = false;
                for (uint32_t i = 0; i < Dim::MAX_PALM_INSTANCES; i++) {
                    if (activePalms_[i].active) { maxSlot = i; anyActive = true; }
                }
                gpuState_.set_palm_index_count(anyActive
                    ? (maxSlot + 1) * Dim::PALMG_MAX_INDICES_PER_SLOT : 0);
                return true;
            }

            // ─── Generative Cacti ─────────────────────────────────────────────
            //
            // Three tiers: Finger, Saguaro, Candelabra. Ribbed columnar trunk
            // with optional forking arms. No piers, no collision solids, no
            // heightfield contribution.

            enum class CactusTier : uint32_t { FINGER = 0, SAGUARO = 1, CANDELABRA = 2, COUNT = 3 };
            static constexpr uint32_t CACTUS_TIER_COUNT = static_cast<uint32_t>(CactusTier::COUNT);

            struct CactusTierParams {
                // ─── Trunk geometry ─────────────────────────────────────
                float height_mean, height_sigma;           // total trunk height
                float radius_mean, radius_sigma;           // trunk radius at base
                float taper_mean, taper_sigma;             // top/bottom radius ratio
                float ribs_mean, ribs_sigma;               // number of vertical ribs
                float rib_depth_mean, rib_depth_sigma;     // rib groove depth
                float lean_mean, lean_sigma;               // trunk lean angle
                float cap_round_mean, cap_round_sigma;     // dome roundness at top

                // ─── Arms (forking branches) ────────────────────────────
                float arm_count_mean, arm_count_sigma;     // number of arms (0 = fingerlike)
                float arm_height_mean, arm_height_sigma;   // fork height (fraction of trunk)
                float arm_length_mean, arm_length_sigma;   // arm length
                float arm_radius_mean, arm_radius_sigma;   // arm thickness
                float arm_curve_mean, arm_curve_sigma;     // upward curve strength

                // ─── Appearance ─────────────────────────────────────────
                float color_over;                          // probability of palette color
                float color_var;                           // per-instance color variance

                // ─── Quality ────────────────────────────────────────────
                uint32_t trunk_segs, arm_segs;             // mesh subdivision counts

                // ─── Selection ──────────────────────────────────────────
                float weight;                              // tier selection probability
            };

            //                                   h_μ  σ     r_μ    σ      tp_μ   σ     ribs_μ σ   rd_μ   σ     ln_μ  σ     cr_μ  σ     ac_μ  σ     ah_μ  σ     al_μ  σ     ar_μ   σ     acv_μ  σ     co   cv    ts  as   wt
            static constexpr CactusTierParams CACTUS_TIERS[] = {
                /* FINGER     */ { 3.0f, 1.0f,  0.15f,0.03f, 0.85f,0.05f, 8.0f,1.0f,  0.04f,0.01f, 0.1f,0.05f, 0.6f,0.1f, 0.0f,0.0f, 0.5f,0.1f, 1.0f,0.3f, 0.08f,0.02f, 0.7f,0.1f,  0.1f, 0.04f, 12, 6,  0.50f },
                /* SAGUARO    */ { 8.0f, 2.0f,  0.35f,0.06f, 0.9f, 0.04f, 12.0f,2.0f, 0.05f,0.01f, 0.08f,0.04f,0.5f,0.1f, 1.5f,0.7f, 0.45f,0.1f,3.0f,0.8f, 0.2f, 0.04f, 0.6f,0.15f, 0.15f,0.03f, 16, 8,  0.35f },
                /* CANDELABRA */ { 14.0f,3.0f,  0.45f,0.08f, 0.92f,0.03f, 16.0f,2.0f, 0.06f,0.01f, 0.05f,0.03f,0.4f,0.1f, 3.0f,0.8f, 0.4f, 0.1f,5.0f,1.0f, 0.25f,0.05f, 0.5f,0.15f, 0.2f, 0.03f, 20, 10, 0.15f },
            };

            // ─── Color Palette ───────────────────────────────────────────────
            // Paradigm: body-part bases with per-instance variance.
            // Trunk body and rib highlights have independent RGB bases.
            // Variance from tier's color_var applied per-spawn.
            static constexpr float CACTUS_BODY_BASE[3] = { 0.30f, 0.45f, 0.25f };
            static constexpr float CACTUS_RIB_BASE[3] = { 0.35f, 0.55f, 0.30f };

            struct CactusConfig {
                static constexpr float SPAWN_CHANCE = 0.100f;
                static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };
                static constexpr float POSITION_JITTER = 0.35f;
            };

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

            ActiveCactus activeCacti_[Dim::MAX_CACTUS_INSTANCES]{};
            uint32_t activeCactusCount_ = 0;
            bool cactusMeshGenPending_ = false;

            // ─── Generative Blade Clusters ──────────────────────────────────
            //
            // Ground-level leaf clusters: 3-7 thick pointed blades from a
            // single ground point. Three tiers: Sprout / Clump / Thicket.
            // Geometry: flat quad strips along curved midribs, golden-angle packed.

            enum class BladeClusterTier : uint32_t { SPROUT = 0, CLUMP = 1, THICKET = 2, COUNT = 3 };
            static constexpr uint32_t BLADE_TIER_COUNT = static_cast<uint32_t>(BladeClusterTier::COUNT);

            struct BladeClusterTierParams {
                // ─── Cluster geometry ───────────────────────────────────
                float blade_count_mean, blade_count_sigma; // number of blades per cluster
                float blade_h_mean, blade_h_sigma;         // blade height
                float blade_h_var_mean, blade_h_var_sigma; // per-blade height variance
                float blade_w_mean, blade_w_sigma;         // blade width at base

                // ─── Blade shape ────────────────────────────────────────
                float splay_mean, splay_sigma;             // outward spread angle
                float curve_mean, curve_sigma;             // midrib curvature
                float twist_mean, twist_sigma;             // axial twist along blade
                float taper_mean, taper_sigma;             // width taper toward tip

                // ─── Appearance ─────────────────────────────────────────
                float color_over, color_var;               // palette override chance, variance

                // ─── Quality ────────────────────────────────────────────
                uint32_t blade_segs;                       // segments per blade

                // ─── Selection ──────────────────────────────────────────
                float weight;                              // tier selection probability
            };

            //                      cnt   σ      h_μ   σ      hv_μ  σ      w_μ   σ
            //                      splay σ      curve σ      twist σ      taper σ      co%  cv    seg  wt
            static constexpr BladeClusterTierParams BLADE_TIERS[] = {
                /* SPROUT  */  { 3.0f, 0.5f,   1.8f, 0.4f,   0.35f, 0.08f,   0.30f, 0.06f,
                                 0.18f, 0.06f,  0.12f, 0.04f,  0.05f, 0.02f,  0.85f, 0.05f,
                                 0.15f, 0.06f,  5,  0.50f },
                                 /* CLUMP   */  { 5.0f, 1.0f,   3.2f, 0.6f,   0.40f, 0.10f,   0.45f, 0.08f,
                                                  0.25f, 0.08f,  0.18f, 0.06f,  0.08f, 0.03f,  0.82f, 0.05f,
                                                  0.20f, 0.06f,  6,  0.35f },
                                                  /* THICKET */  { 6.0f, 1.0f,   5.5f, 1.2f,   0.45f, 0.10f,   0.55f, 0.10f,
                                                                   0.30f, 0.10f,  0.22f, 0.08f,  0.10f, 0.04f,  0.80f, 0.06f,
                                                                   0.25f, 0.08f,  7,  0.15f },
            };

            // ─── Color Palette ───────────────────────────────────────────────
            // Paradigm: body-part bases with per-instance variance.
            // Fresh blade body and aged (dried) blade have independent RGB bases.
            // Variance from tier's color_var applied per-spawn.
            static constexpr float BLADE_BODY_BASE[3] = { 0.28f, 0.52f, 0.22f };
            static constexpr float BLADE_AGED_BASE[3] = { 0.48f, 0.45f, 0.28f };

            struct BladeClusterConfig {
                static constexpr float SPAWN_CHANCE = 0.025f;
                static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };
                static constexpr float POSITION_JITTER = 0.30f;
            };

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

            ActiveBlade activeBlades_[Dim::MAX_BLADE_INSTANCES]{};
            uint32_t activeBladeCount_ = 0;
            bool bladeMeshGenPending_ = false;

            bool prepare_blade_mesh_gen(wgpu::Queue& queue) {
                if (!bladeMeshGenPending_) return false;
                bladeMeshGenPending_ = false;
                uint32_t maxSlot = 0;
                bool anyActive = false;
                for (uint32_t i = 0; i < Dim::MAX_BLADE_INSTANCES; i++) {
                    if (activeBlades_[i].active) { maxSlot = i; anyActive = true; }
                }
                gpuState_.set_blade_index_count(anyActive
                    ? (maxSlot + 1) * Dim::BLADEG_MAX_INDICES_PER_SLOT : 0);
                return true;
            }

            bool prepare_cactus_mesh_gen(wgpu::Queue& queue) {
                if (!cactusMeshGenPending_) return false;
                cactusMeshGenPending_ = false;
                uint32_t maxSlot = 0;
                bool anyActive = false;
                for (uint32_t i = 0; i < Dim::MAX_CACTUS_INSTANCES; i++) {
                    if (activeCacti_[i].active) { maxSlot = i; anyActive = true; }
                }
                gpuState_.set_cactus_index_count(anyActive
                    ? (maxSlot + 1) * Dim::CACTUSG_MAX_INDICES_PER_SLOT : 0);
                return true;
            }

            // ─── Generative Pyramids ─────────────────────────────────────────
            //
            // Three tiers: obelisk, temple, colossus. Each defined by a
            // parameter row in the PYRAMID_TIERS matrix.
            //
            // Collision: pyramid height function baked into heightfield.
            //   Pawn blocked by step-height on steep faces (no solid needed).
            // Visual:    CPU-generated 4-face (pointed) or 5-face (truncated) mesh.
            // Color:     sandstone (shared palette, distinct base tone).

            enum class PyramidTier : uint32_t {
                OBELISK = 0,     // tall narrow marker, pointed apex
                TEMPLE = 1,      // medium, truncated platform top
                COLOSSUS = 2,    // massive landmark, slight or no truncation
                COUNT = 3
            };

            struct PyramidTierParams {
                // ─── Base geometry ───────────────────────────────────────
                float height_mean, height_sigma;
                float base_half_mean, base_half_sigma;
                float aspect_ratio_mean, aspect_ratio_sigma;
                float truncation_mean, truncation_sigma;

                // ─── Blending ────────────────────────────────────────────
                float edge_blend_mean, edge_blend_sigma;

                // ─── Appearance ──────────────────────────────────────────
                float color_override;
                float color_variance;

                // ─── Selection ───────────────────────────────────────────
                float weight;
            };

            //                              h_μ     σ    base_μ  σ    asp_μ  σ     trunc_μ σ     blend_μ σ     col%  var    weight
            static constexpr PyramidTierParams PYRAMID_TIERS[] = {
                /* OBELISK  */  {  28.0f, 6.0f,  16.0f, 3.0f,  1.0f, 0.15f,  0.00f, 0.00f,  1.5f, 0.3f,  0.10f, 0.04f,  0.50f },
                /* TEMPLE   */  {  45.0f, 8.0f,  40.0f, 6.0f,  1.0f, 0.20f,  0.25f, 0.08f,  3.0f, 0.75f, 0.15f, 0.04f,  0.25f },
                /* COLOSSUS */  {  78.0f, 14.4f, 60.0f, 9.6f,  1.0f, 0.10f,  0.05f, 0.04f,  3.6f, 1.0f,  0.20f, 0.04f,  0.25f },
            };

            // ─── Color Palette ───────────────────────────────────────────────
            // Paradigm: sandstone base with per-instance variance (no palette).
            // All pyramids derive color from a single sandstone RGB base.
            static constexpr float PYRAMID_SANDSTONE_BASE[3] = { 0.80f, 0.72f, 0.58f };
            static constexpr float PYRAMID_SANDSTONE_VARIANCE = 0.05f;

            struct PyramidConfig {
                // Flat spawn probability — terrain-independent (themes control variation)
                static constexpr float SPAWN_CHANCE = 0.030f;
                static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };
                static constexpr float POSITION_JITTER = 0.25f;
            };

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

            // ─── Active Pyramid Tracking ─────────────────────────────────────

            struct ActivePyramid {
                int32_t patch_gx = 0, patch_gz = 0;   // trigger patch (idempotency)
                int32_t host_gx = 0, host_gz = 0;     // actual patch covering entity position (eviction)
                bool active = false;
                // Cached color (set at spawn, read by rebuild)
                float col_r = 0.80f, col_g = 0.72f, col_b = 0.58f;

                // Placement (computed once at spawn, immutable)
                float cached_ground_y = 0.0f;         // absolute base Y for VS offset
            };

            ActivePyramid activePyramids_[Dim::MAX_PYRAMID_INSTANCES]{};
            uint32_t activePyramidCount_ = 0;
            bool pyramidMeshGenPending_ = false;  // true → dispatch GPU mesh gen

            // CPU mirror of GPU pyramid instances (for heightfield baking)
            GPUPyramidArray cpuPyramids_{};

            // (generate_pyramid_mesh removed — replaced by GPU compute: pyramid_mesh_gen)

                        // ═══ END INLINED: modules/entities.inl ═════════════════════════

                        // ── Pawn Aura (modules/pawn_aura.inl) ──
#include "modules/pawn_aura.inl"

                        // ── Ground Architecture (modules/ground_architecture.inl) ──
#include "modules/ground_architecture.inl"

                        // ── Sky Orbs (modules/orbs.inl) ──
#include "modules/orbs.inl"

            // ─── Orb Mood Table ─────────────────────────────────────────────
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
                /* 0 open_default        */ {  true,  128, 0.08f, 0.06f, 0.85f, 0.4f, 20.0f,  0u,  0.012f, {0.15f, 0.97f, 0.10f},  0.0f, 0u,  true,  true,  true,  0.12f, false, 0u,           50.0f, 120.0f, 200.0f, 30.0f, 8.0f,  15.0f, 60.0f, 0u,  0.0f, 0.0f, 0.0f, 0.0f },
                /* 1 open_sunset         */ {  true,  128, 0.08f, 0.06f, 0.85f, 0.4f, 20.0f,  3u,  0.012f, {0.15f, 0.97f, 0.10f},  0.0f, 0u,  true,  false, false, 0.08f, false, 0u,           50.0f, 120.0f, 200.0f, 30.0f, 8.0f,  15.0f, 60.0f, 0u,  0.0f, 0.0f, 0.0f, 0.0f },
                /* 2 indoor_flat         */ {  false, 0,   0.08f, 0.05f, 0.80f, 0.5f, 0.0f,   0u,  0.000f, {0.00f, 1.00f, 0.00f},  0.0f, 0u,  false, false, false, 0.12f, false, 0xFFFFFFFFu,  50.0f, 120.0f, 200.0f, 30.0f, 8.0f,  15.0f, 60.0f, 0u,  0.0f, 0.0f, 0.0f, 0.0f },
                /* 3 indoor_vault        */ {  false, 0,   0.08f, 0.05f, 0.80f, 0.5f, 0.0f,   0u,  0.000f, {0.00f, 1.00f, 0.00f},  0.0f, 0u,  false, false, false, 0.12f, false, 0xFFFFFFFFu,  50.0f, 120.0f, 200.0f, 30.0f, 8.0f,  15.0f, 60.0f, 0u,  0.0f, 0.0f, 0.0f, 0.0f },
                /* 4 finite_outdoor      */ {  true,  128, 0.08f, 0.06f, 0.85f, 0.4f, 20.0f,  0u,  0.012f, {0.15f, 0.97f, 0.10f},  0.0f, 0u,  true,  true,  true,  0.12f, false, 0u,           50.0f, 120.0f, 200.0f, 30.0f, 8.0f,  15.0f, 60.0f, 0u,  0.0f, 0.0f, 0.0f, 0.0f },
                /* 5 finite_outdoor_ref  */ {  true,  128, 0.08f, 0.06f, 0.85f, 0.4f, 20.0f,  0u,  0.012f, {0.15f, 0.97f, 0.10f},  0.0f, 0u,  true,  true,  true,  0.12f, false, 0u,           50.0f, 120.0f, 200.0f, 30.0f, 8.0f,  15.0f, 60.0f, 0u,  0.0f, 0.0f, 0.0f, 0.0f },
            };

            // ── Agents (modules/agents.inl) ──
            // Unified entity registry: behaviors, tier gains, populations.
            // Pass 1 scaffold — registries declared; kernel/render wiring
            // lands in later steps.
#include "modules/agents.inl"

// ── Spawn Engine & Entity Lifecycle (modules/spawn_engine.inl) ──
// ═══ INLINED: modules/spawn_engine.inl ═══════════════════════════════

// ─── spawn_engine.inl ────────────────────────────────────────────
//
// How and when things appear. All spawn/evict logic, footprint
// registry, adjacency, population dynamics, entity runtime.
//
// Included inside the Cartridge class body.
// Depends on: entities.inl, terrain_cpu.inl, seed_utils.inl
// ─────────────────────────────────────────────────────────────────

// #define DIAG_ENTITY_LIFECYCLE   // uncomment to enable spawn/evict diagnostics


// ─── Shared Spawn Helpers ────────────────────────────────────────
//
// ── Shared Entity Lifecycle Helpers ──────────────────────────────
// Used by the generic entity pipeline (entity_pipeline.inl) and
// bespoke families (ribbon, GoL, gallery).

            // ── Helper 1: SpawnGatePreamble ──────────────────────────────
            //
            // Idempotency-through-slot-reservation gate.
            // Templated on the active-slot array type so it works with
            // all Active* structs that share .active, .patch_gx, .patch_gz.

            struct SpawnGatePreambleResult {
                uint32_t seed;          // from evaluate_spawn_gate
                uint32_t slot;          // reserved slot index
                uint32_t theme_idx;     // active_theme_idx_ at evaluation time
                bool ok;                // false = early exit (idempotency, gate, no slot)
            };

            template<typename ActiveT>
            SpawnGatePreambleResult run_spawn_preamble(
                int32_t gx, int32_t gz,
                ActiveT* active_arr, uint32_t max_instances,
                uint32_t spawn_roll_prop, float spawn_chance,
                const float* mood_mult,
                uint32_t family, const char* diag_name)
            {
                SpawnGatePreambleResult r{};
                r.ok = false;

                // 1. Idempotency
                for (uint32_t i = 0; i < max_instances; i++) {
                    if (active_arr[i].active &&
                        active_arr[i].patch_gx == gx &&
                        active_arr[i].patch_gz == gz) {
                        return r;
                    }
                }

                // 2-6. Spawn modifier chain
                float adj_mod = mood_mult[activeMood_];
                adj_mod *= population_type_affinity(family);
                adj_mod *= GLOBAL_ENTITY_DENSITY;
                r.theme_idx = active_theme_idx_;
                {
                    auto dit = tileCache_.find({ gx, gz });
                    if (dit != tileCache_.end()) {
                        adj_mod *= dit->second.entity_density;
                        adj_mod *= dit->second.theme_spawn[family];
                    }
                }

                // 6b. Proximity affinity boost (nearby entities attract)
                {
                    float pcx = (gx + 0.5f) * PATCH_EXTENT;
                    float pcz = (gz + 0.5f) * PATCH_EXTENT;
                    adj_mod *= proximity_affinity_boost(pcx, pcz, family);
                }

                // 7. Spawn gate
                auto ctx = evaluate_spawn_gate(gx, gz, spawn_roll_prop,
                    spawn_chance, adj_mod);
                if (!ctx.passed) return r;

                // 8-9. Find and reserve slot
                uint32_t slot = UINT32_MAX;
                for (uint32_t i = 0; i < max_instances; i++) {
                    if (!active_arr[i].active) { slot = i; break; }
                }
                if (slot == UINT32_MAX) return r;
                active_arr[slot].active = true;

#ifdef DIAG_ENTITY_LIFECYCLE
                std::cout << "[DIAG:SEL] " << diag_name << " slot=" << slot
                    << " patch=(" << gx << "," << gz << ")\n";
#endif

                r.seed = ctx.seed;
                r.slot = slot;
                r.ok = true;
                return r;
            }

            // ── Helper 2: NegotiatePosition ─────────────────────────────
            //
            // Jittered position → separation + footprint check →
            // host patch + footprint registration.  Returns the
            // accepted position or failure.

            struct PositionResult {
                float cx, cz, rotation;
                int32_t host_gx, host_gz;
                bool ok;
            };

            PositionResult negotiate_position(
                uint32_t seed, int32_t trigger_gx, int32_t trigger_gz,
                uint32_t pos_x_prop, uint32_t pos_z_prop, float jitter,
                uint32_t rotation_seed_prop,
                float footprint_r, uint32_t family, uint32_t tier = 0)
            {
                PositionResult r{};
                r.ok = false;

                // 1. Jittered position
                jittered_position(seed, trigger_gx, trigger_gz,
                    pos_x_prop, pos_z_prop, jitter, r.cx, r.cz);
                r.rotation = cpu_hash_f(seed, rotation_seed_prop) * 6.283185f;

                // 2. Separation + footprint check (single pass)
                if (!check_position(r.cx, r.cz, footprint_r, family))
                    return r;

                // 3. Host patch + footprint registration
                r.host_gx = (int32_t)std::floor(r.cx / PATCH_EXTENT);
                r.host_gz = (int32_t)std::floor(r.cz / PATCH_EXTENT);
                if (register_footprint(r.cx, r.cz, footprint_r,
                    r.host_gx, r.host_gz, family, tier) == UINT32_MAX) return r;

                r.ok = true;
                return r;
            }

            // ── Helper 3: record_placement_bookkeeping ──────────────────
            //
            // Tail bookkeeping shared by all three families.

            void record_placement_bookkeeping(uint32_t family, uint32_t tier_idx)
            {
                record_population_observation(family, tier_idx);
            }

            // ─── Column mesh gen preparation ──────────────────────────────
            // CPU-side prep: draw range + ground origin upload.
            // Returns true if a dispatch is needed.
            bool prepare_column_mesh_gen(wgpu::Queue& queue) {
                if (!columnMeshGenPending_) return false;
                columnMeshGenPending_ = false;

                uint32_t maxSlot = 0;
                bool anyActive = false;
                for (uint32_t i = 0; i < Dim::MAX_COLUMN_ONLY; i++) {
                    if (activeColumns_[i].active) { maxSlot = i; anyActive = true; }
                }
                for (uint32_t i = 0; i < Dim::MAX_ANTENNA_ONLY; i++) {
                    if (activeAntennas_[i].active) {
                        maxSlot = i + Dim::ANTENNA_SLOT_OFFSET;
                        anyActive = true;
                    }
                }
                gpuState_.set_column_index_count(anyActive
                    ? (maxSlot + 1) * Dim::CMG_MAX_INDICES_PER_SLOT : 0);

                return true;
            }

            // ─── GPU Pyramid Mesh Generation ─────────────────────────────
            //
            // Dispatches the compute shader that generates all 8 pyramid mesh
            // slots. Called when any pyramid spawns or is evicted. Also uploads
            // the pyramid ground origins for Y-correction.

            // ─── Pyramid mesh gen preparation ────────────────────────────
            // CPU-side prep: draw range + ground origin upload.
            // Returns true if a dispatch is needed.
            bool prepare_pyramid_mesh_gen(wgpu::Queue& queue) {
                if (!pyramidMeshGenPending_) return false;
                pyramidMeshGenPending_ = false;

                uint32_t maxSlot = 0;
                bool anyActive = false;
                for (uint32_t i = 0; i < Dim::MAX_PYRAMID_INSTANCES; i++) {
                    if (activePyramids_[i].active) { maxSlot = i; anyActive = true; }
                }
                gpuState_.set_pyramid_index_count(anyActive
                    ? (maxSlot + 1) * Dim::PMG_MAX_INDICES_PER_SLOT : 0);

                // Ground entries (terrain base Y) are uploaded every frame
                // by upload_ground_entries(), not here.
                return true;
            }

            // ─── Pier Write Helper ───────────────────────────────────────────
            //
            // Writes a pier to both CPU mirror and GPU buffer (48 bytes per slot).
            // Maintains pier_count in config for bounded GPU iteration.
            // Inactive pier: default-constructed GPUPierInstance (is_active=0).

            void write_pier(wgpu::Queue& queue, uint32_t slot, const GPUPierInstance& pier) {
                cpuPiers_[slot] = pier;
                gpuState_.upload_pier_slot(queue, slot, pier);
                pierCountDirty_ = true;
                groundEntriesDirty_ = true;
            }

            void clear_pier(wgpu::Queue& queue, uint32_t slot) {
                GPUPierInstance empty{};
                cpuPiers_[slot] = empty;
                gpuState_.upload_pier_slot(queue, slot, empty);
                pierCountDirty_ = true;
                groundEntriesDirty_ = true;
            }

            void recompute_and_upload_pier_count(wgpu::Queue& queue) {
                uint32_t highest = 0;
                for (uint32_t i = 0; i < Dim::PIER_TOTAL; i++) {
                    if (cpuPiers_[i].is_active) highest = i + 1;
                }
                gpuState_.config().pier_count = highest;
                gpuState_.upload_pier_count(queue);
            }

            void flush_pier_count(wgpu::Queue& queue) {
                if (!pierCountDirty_) return;
                pierCountDirty_ = false;
                recompute_and_upload_pier_count(queue);
            }

            // ─── Entity Distance Culling ─────────────────────────────────────
            //
            // Entities beyond a size-proportional distance from the pawn are
            // temporarily hidden by zeroing their GPU mesh params (producing
            // degenerate triangles the rasterizer discards for free).
            //
            // IMPORTANT: the cull floor is set to the PREGEN radius so that
            // every entity within the allocation window always has its mesh
            // ready. Entities are spawned at allocation time; their meshes
            // must be built before the visible circle can reach them.
            // The system becomes active only when the world is extended
            // beyond the current pregen ring (future LOD work).
            //
            // Hysteresis prevents oscillation near the threshold.

            static constexpr float ENTITY_CULL_BASE = (float)Dim::PATCH_PREGEN_RADIUS * Dim::PATCH_EXTENT;  // = 350: never cull inside pregen
            static constexpr float ENTITY_CULL_ARCH_SCALE = 2.5f;   // per unit of max(span, total_height)
            static constexpr float ENTITY_CULL_COL_SCALE = 3.0f;   // per unit of column height
            static constexpr float ENTITY_CULL_HYSTERESIS = 50.0f;  // band width: show at far-hyst, hide at far

            // Rebuild GPUArchMeshParams from cached ActiveArch data.
            GPUArchMeshParams build_arch_mesh_params(uint32_t slot) const {
                const auto& a = activeArches_[slot];
                GPUArchMeshParams p{};
                p.center_x = a.world_x;
                p.center_z = a.world_z;
                p.rotation = a.rotation;
                p.half_span = a.half_span;
                p.rise = a.rise;
                p.depth = a.depth;
                p.thickness = a.thickness;
                p.pier_height = a.pier_height;
                p.burial = a.burial;
                p.catenary_a = solve_catenary_a(a.half_span, a.rise);
                p.segs_u = a.segs_u;
                p.segs_v = a.segs_v;
                // Portal color override (mirrors spawn logic)
                if (a.is_portal) {
                    const float* pc = a.is_back_portal
                        ? PORTAL_COLOR_BACK
                        : PORTAL_COLORS[a.destination.mood % MOOD_COUNT];
                    p.color_r = pc[0]; p.color_g = pc[1]; p.color_b = pc[2];
                }
                else {
                    p.color_r = a.col_r; p.color_g = a.col_g; p.color_b = a.col_b;
                }
                p.is_active = 1;
                return p;
            }

            // Rebuild GPUColumnMeshParams from cached ActiveColumn data.
            static GPUColumnMeshParams build_column_mesh_params_from(const ActiveColumn& c) {
                GPUColumnMeshParams p{};
                p.center_x = c.world_x;
                p.center_z = c.world_z;
                p.height = c.height;
                p.shaft_radius = c.shaft_radius;
                p.taper = c.taper;
                p.entasis = c.entasis;
                p.base_height = c.base_height;
                p.base_overhang = c.base_overhang;
                p.capital_height = c.cap_height;
                p.capital_overhang = c.cap_overhang;
                p.burial = c.burial;
                p.color_r = c.col_r;
                p.color_g = c.col_g;
                p.color_b = c.col_b;
                p.base_layers = c.base_layers;
                p.capital_layers = c.cap_layers;
                p.segs_around = c.segs_around;
                p.shaft_rings = c.shaft_rings;
                p.is_active = 1;
                p.tier = c.tier_idx;
                p.drum_color_r1 = c.drum_colors[0];
                p.drum_color_g1 = c.drum_colors[1];
                p.drum_color_b1 = c.drum_colors[2];
                p.drum_color_r2 = c.drum_colors[3];
                p.drum_color_g2 = c.drum_colors[4];
                p.drum_color_b2 = c.drum_colors[5];
                p.drum_color_r3 = c.drum_colors[6];
                p.drum_color_g3 = c.drum_colors[7];
                p.drum_color_b3 = c.drum_colors[8];
                return p;
            }

            GPUColumnMeshParams build_column_mesh_params(uint32_t slot) const {
                return build_column_mesh_params_from(activeColumns_[slot]);
            }

            // Scan all active entities, toggle draw_visible with hysteresis,
            // and upload mesh param changes. Returns count of currently hidden entities.
            uint32_t update_entity_draw_visibility(wgpu::Queue& queue) {
                uint32_t culled = 0;

                // Arches
                for (uint32_t i = 0; i < Dim::MAX_ARCH_INSTANCES; i++) {
                    if (!activeArches_[i].active) continue;
                    const auto& a = activeArches_[i];
                    float dx = a.world_x - pawnReadback_x_;
                    float dz = a.world_z - pawnReadback_z_;
                    float dist = std::sqrt(dx * dx + dz * dz);

                    float entity_size = std::max(a.half_span * 2.0f, a.total_height);
                    float cull_far = ENTITY_CULL_BASE + entity_size * ENTITY_CULL_ARCH_SCALE;
                    float cull_near = cull_far - ENTITY_CULL_HYSTERESIS;

                    bool should_show = a.draw_visible
                        ? (dist <= cull_far)          // currently visible: hide when exceeding far
                        : (dist <= cull_near);        // currently hidden:  show when inside near

                    if (should_show != a.draw_visible) {
                        activeArches_[i].draw_visible = should_show;
                        if (should_show) {
                            gpuState_.upload_arch_mesh_params_slot(queue, i, build_arch_mesh_params(i));
                        }
                        else {
                            GPUArchMeshParams empty{};
                            gpuState_.upload_arch_mesh_params_slot(queue, i, empty);
                        }
                        archMeshGenPending_ = true;
                    }

                    if (!activeArches_[i].draw_visible) culled++;
                }

                // Columns
                for (uint32_t i = 0; i < Dim::MAX_COLUMN_ONLY; i++) {
                    if (!activeColumns_[i].active) continue;
                    const auto& c = activeColumns_[i];
                    float dx = c.world_x - pawnReadback_x_;
                    float dz = c.world_z - pawnReadback_z_;
                    float dist = std::sqrt(dx * dx + dz * dz);

                    float cull_far = ENTITY_CULL_BASE + c.height * ENTITY_CULL_COL_SCALE;
                    float cull_near = cull_far - ENTITY_CULL_HYSTERESIS;

                    bool should_show = c.draw_visible
                        ? (dist <= cull_far)
                        : (dist <= cull_near);

                    if (should_show != c.draw_visible) {
                        activeColumns_[i].draw_visible = should_show;
                        if (should_show) {
                            gpuState_.upload_column_mesh_params_slot(queue, i, build_column_mesh_params(i));
                        }
                        else {
                            GPUColumnMeshParams empty{};
                            gpuState_.upload_column_mesh_params_slot(queue, i, empty);
                        }
                        columnMeshGenPending_ = true;
                    }

                    if (!activeColumns_[i].draw_visible) culled++;
                }

                // Antennas
                for (uint32_t i = 0; i < Dim::MAX_ANTENNA_ONLY; i++) {
                    if (!activeAntennas_[i].active) continue;
                    const auto& c = activeAntennas_[i];
                    float dx = c.world_x - pawnReadback_x_;
                    float dz = c.world_z - pawnReadback_z_;
                    float dist = std::sqrt(dx * dx + dz * dz);
                    uint32_t gpu_slot = i + Dim::ANTENNA_SLOT_OFFSET;

                    float cull_far = ENTITY_CULL_BASE + c.height * ENTITY_CULL_COL_SCALE;
                    float cull_near = cull_far - ENTITY_CULL_HYSTERESIS;

                    bool should_show = c.draw_visible
                        ? (dist <= cull_far)
                        : (dist <= cull_near);

                    if (should_show != c.draw_visible) {
                        activeAntennas_[i].draw_visible = should_show;
                        if (should_show) {
                            gpuState_.upload_column_mesh_params_slot(queue, gpu_slot, build_column_mesh_params_from(c));
                        }
                        else {
                            GPUColumnMeshParams empty{};
                            gpuState_.upload_column_mesh_params_slot(queue, gpu_slot, empty);
                        }
                        columnMeshGenPending_ = true;
                    }

                    if (!activeAntennas_[i].draw_visible) culled++;
                }

                return culled;
            }

            // ─── Ground Footprint Registry ───────────────────────────────────
            //
            // Prevents grounded entities (pyramids, arches, columns, galleries)
            // from overlapping. Each entity registers a circular exclusion zone
            // at spawn time. Subsequent spawns check against the registry.
            //
            // Priority order (enforced by spawn call sequence):
            //   1. Pyramids  — rarest, largest footprint
            //   2. Arches    — moderate, two pier footprints
            //   3. Columns   — common, small
            //   4. Galleries — common, moderate spread
            //
            // Linear scan is sufficient: max ~88 active footprints.

            struct GroundFootprint {
                float x = 0.0f, z = 0.0f;
                float radius = 0.0f;
                int32_t patch_gx = 0, patch_gz = 0;
                uint32_t family = UINT32_MAX;  // PopFamily index
                uint32_t tier = 0;             // tier index within family
                float spawn_time = 0.0f;       // currentSeconds_ at registration
                bool active = false;
            };

            static constexpr uint32_t MAX_FOOTPRINTS = 128;
            GroundFootprint footprints_[MAX_FOOTPRINTS]{};

            // Single-pass spatial check: physical overlap + aesthetic separation.
            // For entity footprints (family < COUNT): effective_min = gap + both radii.
            // Gap is reduced when proximity affinity exists between the pair.
            bool check_position(float px, float pz, float placing_radius,
                uint32_t placing_family) const {
                for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
                    if (!footprints_[i].active) continue;
                    float dx = px - footprints_[i].x;
                    float dz = pz - footprints_[i].z;
                    float effective_min = placing_radius + footprints_[i].radius;
                    if (footprints_[i].family < PopFamily::COUNT) {
                        float min_gap = MIN_SEPARATION[placing_family][footprints_[i].family];
                        if (min_gap > 0.0f) {
                            float aff = PROXIMITY_AFFINITY[placing_family][footprints_[i].family];
                            if (aff > 0.0f) min_gap *= (1.0f - aff * PROXIMITY_GAP_REDUCTION[placing_family]);
                            effective_min += min_gap;
                        }
                    }
                    if (dx * dx + dz * dz < effective_min * effective_min) return false;
                }
                return true;
            }

            uint32_t register_footprint(float x, float z, float radius,
                int32_t gx, int32_t gz, uint32_t family = UINT32_MAX,
                uint32_t tier = 0) {
                for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
                    if (!footprints_[i].active) {
                        footprints_[i] = { x, z, radius, gx, gz, family, tier, currentSeconds_, true };
                        return i;
                    }
                }
                return UINT32_MAX;  // full — entity should not spawn
            }

            void unregister_footprints_for_patch(int32_t gx, int32_t gz) {
                for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
                    if (footprints_[i].active &&
                        footprints_[i].patch_gx == gx && footprints_[i].patch_gz == gz) {
                        footprints_[i].active = false;
                    }
                }
            }

            // ─── Entity Census ───────────────────────────────────────────────
            //
            // Complete snapshot of all active entities via the footprint registry.
            // Printed to console periodically and on theme transitions.
            // Enables determinism verification: same seed + pawn path → same census.

            float lastCensusDump_ = -999.0f;
            static constexpr float CENSUS_DUMP_INTERVAL = 30.0f;

            static const char* family_short_name(uint32_t family) {
                static const char* NAMES[] = { "pyr", "arch", "col", "ant", "palm", "cact", "blad", "sph", "ribn", "cube", "gol", "gall" };
                return (family < PopFamily::COUNT) ? NAMES[family] : "???";
            }

            static const char* theme_short_name(uint32_t theme) {
                static const char* NAMES[] = { "transition", "monumental", "colonnade", "antenna", "barren" };
                return (theme < THEME_COUNT) ? NAMES[theme] : "???";
            }

            void dump_entity_census(const char* trigger) const {
                uint32_t count = 0;
                uint32_t by_family[PopFamily::COUNT] = {};
                for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
                    if (!footprints_[i].active) continue;
                    if (footprints_[i].family >= PopFamily::COUNT) continue;
                    count++;
                    by_family[footprints_[i].family]++;
                }

                std::cout << "[CENSUS t=" << std::fixed << std::setprecision(1) << currentSeconds_
                    << " trigger=" << trigger << "] " << count << " entities (";
                for (uint32_t f = 0; f < PopFamily::COUNT; f++) {
                    if (f > 0) std::cout << " ";
                    std::cout << family_short_name(f) << ":" << by_family[f];
                }
                std::cout << ")\n";

                // Per-entity detail, sorted by family then spawn_time
                struct CensusEntry { uint32_t fp_idx; uint32_t family; uint32_t tier; float spawn_time; };
                CensusEntry entries[MAX_FOOTPRINTS];
                uint32_t n = 0;
                for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
                    if (!footprints_[i].active || footprints_[i].family >= PopFamily::COUNT) continue;
                    entries[n++] = { i, footprints_[i].family, footprints_[i].tier, footprints_[i].spawn_time };
                }
                // Insertion sort by (family, spawn_time)
                for (uint32_t i = 1; i < n; i++) {
                    CensusEntry key = entries[i]; uint32_t j = i;
                    while (j > 0 && (entries[j - 1].family > key.family ||
                        (entries[j - 1].family == key.family && entries[j - 1].spawn_time > key.spawn_time))) {
                        entries[j] = entries[j - 1]; j--;
                    }
                    entries[j] = key;
                }
                for (uint32_t i = 0; i < n; i++) {
                    const auto& fp = footprints_[entries[i].fp_idx];
                    std::cout << "  " << family_short_name(fp.family)
                        << " t" << fp.tier
                        << " (" << std::setw(8) << std::setprecision(1) << fp.x
                        << "," << std::setw(8) << fp.z << ")"
                        << " p(" << std::setw(3) << fp.patch_gx << "," << std::setw(3) << fp.patch_gz << ")"
                        << " age=" << std::setprecision(1) << (currentSeconds_ - fp.spawn_time)
                        << "\n";
                }
                std::cout << std::flush;
            }

            // ─── Spawn Utilities ─────────────────────────────────────────────
            //
            // Shared preamble for entity spawn functions. Extracted from the
            // common skeleton of spawn_{arches,columns,pyramids}_for_patch.
            //
            // Usage:
            //   auto ctx = evaluate_spawn_gate(gx, gz, Prop::SPAWN_ROLL, Config::SPAWN_CHANCE);
            //   if (!ctx.passed) return;
            //   uint32_t tier = select_tier(ctx.seed, Prop::TIER, weights, count);
            //   jittered_position(ctx.seed, gx, gz, Prop::POSITION_X, Prop::POSITION_Z, jitter, cx, cz);

            struct SpawnPreamble {
                uint32_t seed;          // tile_seed(activeSeed_, gx, gz)
                uint32_t archetype;     // 0=mountainous, 1=varied, 2=basin, 3=pool
                bool passed;            // false if spawn gate failed
            };

            // Evaluate the spawn gate: seed + flat probability check.
            // adjacency_mod is a multiplier from the full spawn cascade.
            SpawnPreamble evaluate_spawn_gate(int32_t gx, int32_t gz,
                uint32_t spawn_roll_prop,
                float spawn_chance,
                float adjacency_mod = 1.0f) const {
                SpawnPreamble result{};
                result.archetype = 1;
                auto tile_it = tileCache_.find({ gx, gz });
                if (tile_it != tileCache_.end()) result.archetype = tile_it->second.archetype;

                result.seed = tile_seed(activeSeed_, gx, gz);
                float chance = std::min(spawn_chance * adjacency_mod, 1.0f);
                result.passed = cpu_hash_f(result.seed, spawn_roll_prop) < chance;
                return result;
            }

            // Jittered world position within a patch.
            static void jittered_position(uint32_t seed, int32_t gx, int32_t gz,
                uint32_t prop_x, uint32_t prop_z, float jitter,
                float& out_x, float& out_z) {
                out_x = (gx + 0.5f) * PATCH_EXTENT + (cpu_hash_f(seed, prop_x) - 0.5f) * PATCH_EXTENT * jitter;
                out_z = (gz + 0.5f) * PATCH_EXTENT + (cpu_hash_f(seed, prop_z) - 0.5f) * PATCH_EXTENT * jitter;
            }

            // Entity families for observation indexing
            struct PopFamily {
                static constexpr uint32_t PYRAMID = 0;
                static constexpr uint32_t ARCH = 1;
                static constexpr uint32_t COLUMN = 2;
                static constexpr uint32_t ANTENNA = 3;
                static constexpr uint32_t PALM = 4;
                static constexpr uint32_t CACTUS = 5;
                static constexpr uint32_t BLADE = 6;
                static constexpr uint32_t SPHERE = 7;    // orbital spheres
                static constexpr uint32_t RIBBON = 8;
                static constexpr uint32_t CUBE = 9;      // hover-bob monoliths (split from legacy FLOATING)
                static constexpr uint32_t GOL = 10;       // Game of Life / Pulse automaton zones
                static constexpr uint32_t GALLERY = 11;   // outdoor art exhibitions (composite: 1 center → N paintings)
                static constexpr uint32_t COUNT = 12;
            };

            // ─── Spawn Configuration Summary ────────────────────────────────
            //
            // Single-glance view of all entity spawn parameters.
            // Authoritative values live in each entity's Config struct.
            //
            //  ┌──────────┬──────────┬───────────────────────────────────────┬────────┐
            //  │ Family   │ CHANCE   │ MOOD_MULTIPLIER                       │ JITTER │
            //  │          │          │ open  sunset flat  vault  fin   finR  │        │
            //  ├──────────┼──────────┼───────────────────────────────────────┼────────┤
            //  │ Pyramid  │  0.030   │ 1.0   1.0    1.0   1.0    1.0   0.0  │  0.25  │
            //  │ Arch     │  0.030   │ 1.0   1.0    1.0   1.0    1.0   1.0  │  0.35  │
            //  │ Column   │  0.030   │ 1.0   1.0    1.0   1.0    1.0   0.0  │  0.35  │
            //  │ Antenna  │  0.025   │ 1.0   1.0    1.0   1.0    1.0   0.0  │  0.35  │
            //  │ Palm     │  0.200   │ 1.0   1.0    1.0   1.0    1.0   0.0  │  0.45  │
            //  │ Cactus   │  0.100   │ 1.0   1.0    1.0   1.0    1.0   0.0  │  0.35  │
            //  │ Blade    │  0.025   │ 1.0   1.0    1.0   1.0    1.0   0.0  │  0.30  │
            //  │ Sphere   │  0.015   │ 1.0   1.0    0.0   0.0    1.0   0.0  │  0.40  │
            //  │ Ribbon   │  0.400   │ 1.0   1.0    0.0   0.0    1.0   0.0  │  0.30  │
            //  │ Cube     │  0.060   │ 1.0   1.0    0.0   0.0    1.0   0.0  │  0.40  │
            //  │ GoL      │  0.150   │ 1.0   1.0    0.0   0.0    1.0   0.0  │  (lattice) │
            //  │ Gallery  │  varies  │ 1.0   1.0    1.0   1.0    1.0   0.0  │  0.30  │
            //  └──────────┴──────────┴───────────────────────────────────────┴────────┘
            //
            // Spawn chance is flat — archetype/terrain no longer gates spawning.
            // Spatial variation comes from theme lattice and density field.
            // Themes further multiply spawn_chance via PopulationTheme::spawn_weight[family].
            // Moods gate entire families (0.0 = suppressed in that mood).
            // Jitter is fraction of PATCH_EXTENT for position randomization.

            // ─── Global Entity Density ──────────────────────────────────────
            // Master multiplier applied to ALL entity spawn chances.
            // 1.0 = normal, <1.0 = sparser world, >1.0 = denser world.
            // Stacks with per-theme density_mult and per-tile entity_density.
            static constexpr float GLOBAL_ENTITY_DENSITY = 1.0f;

            // ─── Property Index Registry ────────────────────────────────────
            //
            // Master allocation map for cpu_hash_f(seed, prop) indices.
            // Each family claims a non-overlapping range within its seed source.
            // Collisions between families that share a seed source produce
            // correlated rolls — a subtle bug. Check this table before
            // allocating indices for new entities.
            //
            // SEED SOURCE: tile_seed(activeSeed_, gx, gz)
            //  ┌───────────┬───────────┬────────────────────────────────────┐
            //  │ Range     │ Family    │ Struct                             │
            //  ├───────────┼───────────┼────────────────────────────────────┤
            //  │   1       │ Heightfld │ (inline)                           │
            //  │ 200 – 208 │ Terrain   │ CPU_WAVE_* constants               │
            //  │ 220 – 221 │ Activity  │ CPU_ACT_PROP_* constants           │
            //  │ 370       │ Theme     │ select_theme_at_node (inline)      │
            //  │ 400 – 443 │ Ribbon    │ RibbonProp                         │
            //  │ 500 – 540 │ Gallery   │ select_gallery_for_patch           │
            //  │ 600 – 623 │ Arch      │ ArchProp                           │
            //  │ 700 – 743 │ Column    │ ColumnProp                         │
            //  │ 800 – 823 │ Pyramid   │ PyramidProp                        │
            //  │ 900 – 943 │ Antenna   │ AntennaProp                        │
            //  │ 950 – 993 │ Palm      │ PalmProp                           │
            //  │1000 –1033 │ Cactus    │ CactusProp                         │
            //  │1100 –1122 │ Blade     │ BladeProp                          │
            //  │1200+      │ (free)    │ next entity family starts here     │
            //  └───────────┴───────────┴────────────────────────────────────┘
            //
            // SEED SOURCE: tile_seed (shared with all families)
            //  ┌───────────┬───────────┬────────────────────────────────────┐
            //  │ Range     │ Family    │ Struct                             │
            //  ├───────────┼───────────┼────────────────────────────────────┤
            //  │ 100 – 126 │ Floater   │ FloatingEntityProp                 │
            //  └───────────┴───────────┴────────────────────────────────────┘
            //
            // SEED SOURCE: lattice_node_seed (band 250) — GoL zones
            //  ┌───────────┬───────────┬────────────────────────────────────┐
            //  │ Range     │ Family    │ Struct                             │
            //  ├───────────┼───────────┼────────────────────────────────────┤
            //  │ 920 – 938 │ GoL Zone  │ GoLZoneProp                        │
            //  │ 950 – 954 │ Pulse     │ PulseZoneProp                      │
            //  └───────────┴───────────┴────────────────────────────────────┘
            //  NOTE: GoL/Pulse indices overlap Antenna and Palm numerically,
            //  but are safe — they use a different seed source (lattice band
            //  250 vs tile_seed). Do NOT move GoL props into the tile_seed
            //  range without resolving the collision.


            // ─── Proximity Affinity ───────────────────────────────────────────
            //
            // Distance-based spawn boost: "how much does a nearby entity of
            // family B increase the spawn chance of family A?"
            //
            // PROXIMITY_AFFINITY[spawning][nearby] — boost per entity within radius.
            // 0.0 = no effect. Positive = attraction. The scan accumulates a
            // weighted count and returns a multiplier capped at max boost.
            //
            // Per-family parameters control the search geometry:
            //   PROXIMITY_RADIUS     — search distance (0 = disabled)
            //   PROXIMITY_MAX_BOOST  — cap on multiplier (1.0 = no boost possible)
            //   PROXIMITY_THRESHOLD  — min nearby count before boost activates
            //   PROXIMITY_GAP_REDUCTION — how much affinity softens separation
            //
            // Precomputed participation masks enable zero-cost early-out for
            // families with no affinities defined.

            //                              Pyr    Arch   Col    Ant    Palm   Cact   Blad   Sph    Ribn   Cube   GoL    Gall
            static constexpr float    PROXIMITY_RADIUS[PopFamily::COUNT] = { 0.0f,  0.0f, 60.0f,  0.0f,150.0f,120.0f,120.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f };
            static constexpr float    PROXIMITY_MAX_BOOST[PopFamily::COUNT] = { 1.0f,  1.0f,  2.0f,  1.0f,  3.0f,  3.0f,  3.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f };
            static constexpr uint32_t PROXIMITY_THRESHOLD[PopFamily::COUNT] = { 0,     0,     2,     0,     1,     1,     1,     0,     0,     0,     0,     0 };
            static constexpr float    PROXIMITY_GAP_REDUCTION[PopFamily::COUNT] = { 0.0f, 0.0f, 0.3f, 0.0f, 0.6f, 0.6f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

            static constexpr float PROXIMITY_AFFINITY[PopFamily::COUNT][PopFamily::COUNT] = {
                //           near: Pyr   Arch  Col   Ant   Palm  Cact  Blad  Sph   Ribn  Cube  GoL   Gall
                /* Pyr   */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                /* Arch  */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                /* Col   */ { 0.0f, 0.0f, 0.4f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                /* Ant   */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                /* Palm  */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.3f, 0.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                /* Cact  */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.5f, 0.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                /* Blad  */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.3f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                /* Sph   */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                /* Ribn  */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                /* Cube  */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                /* GoL   */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                /* Gall  */ { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
            };

            // Precomputed: does this family have any non-zero affinity?
            static constexpr bool proximity_row_active(uint32_t family) {
                for (uint32_t f = 0; f < PopFamily::COUNT; f++)
                    if (PROXIMITY_AFFINITY[family][f] > 0.0f) return true;
                return false;
            }

            float proximity_affinity_boost(float cx, float cz, uint32_t family) const {
                if (!proximity_row_active(family)) return 1.0f;
                float radius = PROXIMITY_RADIUS[family];
                if (radius <= 0.0f) return 1.0f;
                float r2 = radius * radius;
                float weighted = 0.0f;
                uint32_t count = 0;
                for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
                    if (!footprints_[i].active) continue;
                    if (footprints_[i].family >= PopFamily::COUNT) continue;
                    float aff = PROXIMITY_AFFINITY[family][footprints_[i].family];
                    if (aff <= 0.0f) continue;
                    float dx = cx - footprints_[i].x;
                    float dz = cz - footprints_[i].z;
                    if (dx * dx + dz * dz < r2) {
                        weighted += aff;
                        count++;
                    }
                }
                if (count < PROXIMITY_THRESHOLD[family]) return 1.0f;
                return std::min(1.0f + weighted, PROXIMITY_MAX_BOOST[family]);
            }

            // Mark already-generated patches that overlap a world-space AABB
            // for regeneration. Used by arches, pyramids, and any entity whose
            // collision geometry must be baked into the heightfield.
            void mark_patches_for_regen(float min_wx, float min_wz,
                float max_wx, float max_wz,
                int32_t home_gx, int32_t home_gz) {
                int32_t pg_x0 = (int32_t)std::floor(min_wx / PATCH_EXTENT);
                int32_t pg_x1 = (int32_t)std::floor(max_wx / PATCH_EXTENT);
                int32_t pg_z0 = (int32_t)std::floor(min_wz / PATCH_EXTENT);
                int32_t pg_z1 = (int32_t)std::floor(max_wz / PATCH_EXTENT);

                for (uint32_t p = 0; p < activePatchCount_; p++) {
                    if (patches_[p].phase != PatchPhase::GENERATED) continue;
                    if (patches_[p].grid_x == home_gx && patches_[p].grid_z == home_gz) continue;
                    if (patches_[p].grid_x >= pg_x0 && patches_[p].grid_x <= pg_x1 &&
                        patches_[p].grid_z >= pg_z0 && patches_[p].grid_z <= pg_z1) {
                        patches_[p].phase = PatchPhase::NEEDS_REGEN;
                    }
                }
            }


            // (generate_arch_mesh removed — replaced by GPU compute: arch_mesh_gen)

            // Precompute catenary parameter 'a' from (half_span, rise).
            // 50-iteration bisection, passed to GPU in ArchMeshParams.
            static float solve_catenary_a(float half_span, float target_h) {
                float a_lo = 0.1f, a_hi = std::max(half_span * 10.0f, 5.0f);
                float a = half_span;
                for (int iter = 0; iter < 50; iter++) {
                    a = 0.5f * (a_lo + a_hi);
                    float val = a * (std::cosh(half_span / a) - 1.0f);
                    if (val > target_h) a_lo = a; else a_hi = a;
                }
                return a;
            }

            // ─── GoL Zone Selection / Placement ──────────────────────────────

            struct GoLSelection {
                uint32_t seed;
                int32_t  trigger_gx, trigger_gz;
                uint32_t slot;
                int32_t  zone_nx, zone_nz;     // lattice node
                float    corner_x, corner_z;   // zone corner (cell-grid-snapped)
                uint32_t algorithm;            // AlgorithmType::CONWAY or PULSE
                uint32_t tier_idx;             // compound: Conway 0–6, Pulse 7–9
                float    tick_period;
                float    initial_density;
                bool     height_enabled;
                float    footprint_r;
            };

            struct GoLPlacement {
                uint32_t slot;
                int32_t  trigger_gx, trigger_gz;
                int32_t  host_gx, host_gz;
                uint32_t tier_idx;
                float    cx, cz;               // zone center
                int32_t  zone_nx, zone_nz;
                float    corner_x, corner_z;
                uint32_t algorithm;
                float    tick_period;
                float    initial_density;
                bool     height_enabled;
            };

            // ─── Gallery Selection / Placement ───────────────────────────────
            // (Outdoor art exhibitions — composite: 1 center → N paintings)

            struct GallerySelection {
                uint32_t seed;
                int32_t  trigger_gx, trigger_gz;
                uint32_t slot;              // gallery center slot
                float    cx, cz;            // gallery center (jittered)
                float    footprint_r;       // gallery spatial envelope
                uint32_t archetype;         // 0–3 (terrain type, used as tier_idx)
                uint32_t painting_count;
                float    facing_angle;
                float    gallery_size_mean;
                uint32_t site_type;         // 0=snapshot, 1=mixed, 2=authored
            };

            struct GalleryPlacement {
                uint32_t slot;
                int32_t  trigger_gx, trigger_gz;
                int32_t  host_gx, host_gz;
                uint32_t tier_idx;          // = archetype
                float    cx, cz;
                float    footprint_r;
                uint32_t archetype;
                uint32_t painting_count;
                float    facing_angle;
                float    gallery_size_mean;
                uint32_t site_type;
            };

            // ─── Ribbon Selection / Placement ────────────────────────────

            struct RibbonSelection {
                uint32_t seed;
                int32_t  trigger_gx, trigger_gz;
                uint32_t slot;
                uint32_t tier_idx;
                // Geometry (from select_ribbon_for_patch)
                uint32_t cube_count;
                float cube_size;
                float height;
                float orientation;
                float lateral_amp, lateral_cycles, lateral_speed;
                float vertical_amp, vertical_cycles, vertical_speed;
                float twist_amp, twist_cycles, twist_speed;
                // Color
                uint32_t color_mode;
                float color[3];
                // Footprint
                float footprint_r;
            };

            struct RibbonPlacement {
                uint32_t slot;
                int32_t  trigger_gx, trigger_gz;
                int32_t  host_gx, host_gz;
                uint32_t tier_idx;
                float cx, cz, rotation;
                // Geometry (copied from selection)
                uint32_t cube_count;
                float cube_size, height, orientation;
                float lateral_amp, lateral_cycles, lateral_speed;
                float vertical_amp, vertical_cycles, vertical_speed;
                float twist_amp, twist_cycles, twist_speed;
                uint32_t color_mode;
                float color[3];
            };

            // ── Generic Entity Types (modules/entity_types.inl) ──
#include "modules/entity_types.inl"

            // ─── Entity Selection Queue ─────────────────────────────────────
            //
            // Lightweight tagged entry holding one family's selection.
            // Produced by select_entities_for_patch, consumed by
            // drain_entity_queue. The queue decouples WHAT exists from
            // WHERE it goes — selections are position-independent.

            struct EntityQueueEntry {
                uint32_t family;    // PopFamily index
                int32_t  gx, gz;    // trigger patch (for commit bookkeeping)
                union {
                    RibbonSelection ribbon;
                    GoLSelection    gol;
                    GallerySelection gallery;
                    EntityInstance   generic;    // used by all 9 migrated families
                };
                EntityQueueEntry() : family(0), gx(0), gz(0) { std::memset(&generic, 0, sizeof(generic)); }
            };

            std::vector<EntityQueueEntry> entityQueue_;

            // ─── Placement Results ──────────────────────────────────────────
            //
            // Output of place_entity_queue: entities that passed spatial
            // negotiation and are ready for GPU commit. Tagged union mirrors
            // EntityQueueEntry but holds Placement structs instead of Selections.

            struct PlacementEntry {
                uint32_t family;
                int32_t  gx, gz;
                union {
                    RibbonPlacement ribbon;
                    GoLPlacement    gol;
                    GalleryPlacement gallery;
                    EntityInstance   generic;    // used by all 9 migrated families
                };
                PlacementEntry() : family(0), gx(0), gz(0) { std::memset(&generic, 0, sizeof(generic)); }
            };

            std::vector<PlacementEntry> placementResults_;

            // ─── Select / Place / Commit dispatch loops ─────────────────────

            void select_entities_for_patch(int32_t gx, int32_t gz) {
                for (uint32_t f = 0; f < PopFamily::COUNT; f++) {
                    EntityQueueEntry e{};
                    e.family = f;
                    e.gx = gx; e.gz = gz;
                    if (FAMILY_DISPATCH[f].try_select(this, gx, gz, e))
                        entityQueue_.push_back(e);
                }
            }

            // ─── Place: spatial negotiation (no GPU writes) ──────────────
            //
            // Processes entityQueue_ in FIFO order via FAMILY_DISPATCH table.
            // Each selection goes through separation, footprint. Successful
            // placements are pushed to placementResults_. Failed placements
            // unreserve the slot and are dropped.
            //
            // Mutates: footprints_, spawn records, population batch.
            // Does NOT touch: GPU queue, GPU buffers, Active* arrays.

            void place_entity_queue() {
                for (auto& e : entityQueue_) {
                    PlacementEntry pe{};
                    if (FAMILY_DISPATCH[e.family].try_place(this, e, pe))
                        placementResults_.push_back(pe);
                }
                entityQueue_.clear();
            }

            // ─── Commit: GPU writes from placement results ──────────────
            //
            // Iterates placementResults_ via FAMILY_DISPATCH table, writes
            // GPU state for each entity. Clears placementResults_ when done.
            //
            // Mutates: GPU buffers via queue, Active* arrays, pier mirrors,
            // portal array, mesh gen flags — all render-layer state.
            // Does NOT touch: footprints, spawn records.

            void commit_entity_queue(wgpu::Queue& queue) {
                for (auto& pe : placementResults_)
                    FAMILY_DISPATCH[pe.family].try_commit(this, pe, queue);
                placementResults_.clear();
            }

            // ─── GPU Arch Mesh Generation ─────────────────────────────────
            //
            // CPU-side prep: draw range + ground origin upload.
            // Returns true if a dispatch is needed.

            bool prepare_arch_mesh_gen(wgpu::Queue& queue) {
                if (!archMeshGenPending_) return false;
                archMeshGenPending_ = false;

                uint32_t maxSlot = 0;
                bool anyActive = false;
                for (uint32_t i = 0; i < Dim::MAX_ARCH_INSTANCES; i++) {
                    if (activeArches_[i].active) { maxSlot = i; anyActive = true; }
                }
                gpuState_.set_arch_index_count(anyActive
                    ? (maxSlot + 1) * Dim::AMG_MAX_INDICES_PER_SLOT : 0);

                // Ground entries (pier positions, corrections) are uploaded
                // every frame by upload_ground_entries(), not here.
                return true;
            }



            // ─── Ribbon Lifecycle (patch-based dispatch pipeline) ────────────

            static constexpr uint32_t MAX_RIBBON_INSTANCES = 1;  // single-render; raise when GPU supports multi-ribbon
            static constexpr float    RIBBON_MAX_LENGTH = 700.0f;

            struct ActiveRibbon {
                int32_t patch_gx = 0, patch_gz = 0;   // trigger patch
                int32_t host_gx = 0, host_gz = 0;     // host patch (anchor position)
                float anchor_x = 0.0f, anchor_z = 0.0f;
                // Two-tip anchoring: ribbon survives until BOTH tips' patches are gone
                float near_tip_x = 0.0f, near_tip_z = 0.0f;
                float far_tip_x = 0.0f, far_tip_z = 0.0f;
                int32_t near_tip_gx = 0, near_tip_gz = 0;
                int32_t far_tip_gx = 0, far_tip_gz = 0;
                bool near_tip_registered = false;
                bool far_tip_registered = false;
                uint32_t ref_count = 0;     // patches referencing this ribbon via record_entity
                bool active = false;
            };
            ActiveRibbon activeRibbons_[MAX_RIBBON_INSTANCES]{};
            uint32_t activeRibbonCount_ = 0;
            GPURibbonState ribbonStates_[MAX_RIBBON_INSTANCES]{};  // CPU mirror per slot
            uint32_t renderedRibbonSlot_ = UINT32_MAX;             // which slot is on GPU

            // ─── Mood 5 Ribbon Anchor ─────────────────────────────────────
            // Seed-derived position centered on the finite world.
            // Adjust moodRibbonOffset_ to manually shift the anchor XZ.
            float moodRibbonOffset_[2] = { 0.0f, 0.0f };

            // CPU mirror of WGSL ribbon_spine_at — evaluate one ring's world position.
            static void ribbon_spine_at_cpu(const GPURibbonState& r, float time, uint32_t ring_idx, float out[3]) {
                constexpr float PI = 3.14159265359f;
                float t = (float)ring_idx / (float)std::max(r.cube_count - 1u, 1u);
                float total_length = (float)r.cube_count * r.cube_size;

                float along = t * total_length;
                float lateral = std::sin(time * r.lateral_speed + t * r.lateral_cycles * 2.0f * PI) * r.lateral_amp;
                float vertical = r.height + std::sin(time * r.vertical_speed + t * r.vertical_cycles * 2.0f * PI) * r.vertical_amp;

                float c = std::cos(r.orientation);
                float s = std::sin(r.orientation);
                float rotated_along = along * c - lateral * s;
                float rotated_lateral = along * s + lateral * c;

                float twist_phase = time * r.twist_speed + t * r.twist_cycles * 2.0f * PI;
                float twist_depth = std::sin(twist_phase) * 0.4f * r.twist_amp;
                float twist_vert = std::cos(twist_phase) * 0.3f * r.twist_amp;

                out[0] = r.anchor[0] + rotated_along;
                out[1] = vertical + twist_vert;
                out[2] = r.anchor[2] + rotated_lateral + twist_depth;
            }

            // CPU mirror of WGSL ribbon_tangent_at — central finite difference.
            static void ribbon_tangent_cpu(const GPURibbonState& r, float time, uint32_t ring_idx, float out[3]) {
                constexpr float eps = 0.0005f;
                float t = (float)ring_idx / (float)std::max(r.cube_count - 1u, 1u);
                // Evaluate spine at t±eps using the raw parametric form
                auto eval = [&](float tp, float p[3]) {
                    constexpr float PI = 3.14159265359f;
                    float total_length = (float)r.cube_count * r.cube_size;
                    float along = tp * total_length;
                    float lateral = std::sin(time * r.lateral_speed + tp * r.lateral_cycles * 2.0f * PI) * r.lateral_amp;
                    float vertical = r.height + std::sin(time * r.vertical_speed + tp * r.vertical_cycles * 2.0f * PI) * r.vertical_amp;
                    float c = std::cos(r.orientation);
                    float s = std::sin(r.orientation);
                    float rotated_along = along * c - lateral * s;
                    float rotated_lateral = along * s + lateral * c;
                    float twist_phase = time * r.twist_speed + tp * r.twist_cycles * 2.0f * PI;
                    float twist_depth = std::sin(twist_phase) * 0.4f * r.twist_amp;
                    float twist_vert = std::cos(twist_phase) * 0.3f * r.twist_amp;
                    p[0] = r.anchor[0] + rotated_along;
                    p[1] = vertical + twist_vert;
                    p[2] = r.anchor[2] + rotated_lateral + twist_depth;
                    };
                float a[3], b[3];
                eval(t + eps, a);
                eval(t - eps, b);
                float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
                float len = std::sqrt(dx * dx + dy * dy + dz * dz);
                if (len < 1e-8f) { out[0] = 1; out[1] = 0; out[2] = 0; return; }
                out[0] = dx / len; out[1] = dy / len; out[2] = dz / len;
            }

            // CPU mirror of rotor construction — returns axis, angle, and whether it degenerated.
            struct RotorDiag {
                float axis[3];
                float angle_deg;
                float cross_len;   // length of cross(forward, tangent) — near 0 = degenerate
                bool antiparallel; // tangent ≈ -forward
            };
            static RotorDiag ribbon_rotor_diag(const float tangent[3]) {
                RotorDiag d{};
                // forward = (1,0,0)
                // cross(forward, tangent) = (0*tz - 0*ty, 0*tx - 1*tz, 1*ty - 0*tx)
                //                         = (0, -tz, ty)
                d.axis[0] = 0.0f;
                d.axis[1] = -tangent[2];
                d.axis[2] = tangent[1];
                d.cross_len = std::sqrt(d.axis[1] * d.axis[1] + d.axis[2] * d.axis[2]);
                float dot = tangent[0]; // dot((1,0,0), tangent)
                d.angle_deg = std::acos(std::max(-1.0f, std::min(1.0f, dot))) * 57.2958f;
                d.antiparallel = (d.cross_len < 0.001f && dot < 0.0f);
                return d;
            }

            // Check a 3×3 neighborhood of ribbon cells around the pawn.
            // Estimate terrain height from tile cache (rough CPU-side approximation).
            //
            // NOT a ground policy query. Deliberately kept as the CPU fast
            // path per ground_hierarchy_design.md §6.2: the CPU stays on an
            // approximate tile-cache lookup rather than growing a parallel
            // query_ground_* system. Callers that need accurate height must
            // either (a) pick up the GPU-baked heightfield via readback
            // (POLICY_BAKED_HEIGHTFIELD texture) or (b) defer the decision
            // to a GPU compute pass.
            float estimate_terrain_height(float wx, float wz) const {
                int32_t tx = (int32_t)std::floor(wx / PATCH_EXTENT);
                int32_t tz = (int32_t)std::floor(wz / PATCH_EXTENT);
                auto it = tileCache_.find({ tx, tz });
                if (it != tileCache_.end())
                    return it->second.height_bias + it->second.amp_scale * 5.0f;
                return 0.0f;
            }


            // ═══ END INLINED: modules/spawn_engine.inl ═════════════════════════

// ─── Sphere Entity System ────────────────────────────────────────
//
// Orbital spheres. Rare, PGA motor-driven orbits around anchors.
// Slots 0 .. MAX_SPHERE_INSTANCES-1 in the shared floating entity buffer.
// GPU compute: update_sphere. Vertex shader: sphere_vs.
// ─────────────────────────────────────────────────────────────────

            // ─── Sphere Tier Profile ─────────────────────────────────────
            // (Reuses FloatingEntityTierProfile — orbit fields are meaningful,
            //  hover-bob fields are zero.)
            struct FloatingEntityTierProfile {
                float body_radius_mean, body_radius_sigma;
                float orbit_radius_mean, orbit_radius_sigma;
                float orbit_height_mean, orbit_height_sigma;
                float orbit_speed_mean, orbit_speed_sigma;
                float influence_radius_mean, influence_radius_sigma;
                float spin_speed_mean, spin_speed_sigma;
                float bob_amplitude_mean, bob_amplitude_sigma;
                float bob_period_mean, bob_period_sigma;
                float spin_tilt_sigma;
                float aspect_y_mean, aspect_y_sigma;
                float aspect_z_mean, aspect_z_sigma;
                float face_variance_mean, face_variance_sigma;
                uint32_t geometry_type;
                uint32_t motion_type;
                float weight;
            };

            static constexpr uint32_t SPHERE_TIER_COUNT = 2;
            static constexpr FloatingEntityTierProfile SPHERE_TIERS[SPHERE_TIER_COUNT] = {
                //                    rad_μ  σ     orb_μ  σ     ht_μ   σ      spd_μ  σ     inf_μ  σ     spin_μ σ     bob_μ  σ    per_μ  σ    tilt   asp_y  σ    asp_z  σ    fvar_μ σ     geo  mot  wt
                /* 0: Sentinel  */ {  1.5f, 0.3f,  12.0f, 3.0f,   6.0f,  2.0f, 1.4f, 0.3f,  8.0f, 2.0f,  0.0f, 0.0f,  0.0f, 0.0f,  0.0f, 0.0f,  0.0f,  1.0f,0.0f,  1.0f,0.0f,  0.0f,0.0f,   0, 0, 0.65f },
                /* 1: Anomaly   */ {  1.2f, 0.2f,   8.0f, 2.0f,   4.0f,  1.5f, 2.0f, 0.5f,  6.0f, 1.5f,  0.0f, 0.0f,  0.0f, 0.0f,  0.0f, 0.0f,  0.0f,  1.0f,0.0f,  1.0f,0.0f,  0.0f,0.0f,   0, 0, 0.35f },
            };

            static constexpr float SPHERE_BASE_TIER_WEIGHTS[SPHERE_TIER_COUNT] = { 0.65f, 0.35f };
            static constexpr const char* SPHERE_TIER_NAMES[] = { "Sentinel", "Anomaly" };

            // ─── Sphere Spawn Configuration ──────────────────────────────
            struct SphereConfig {
                static constexpr float SPAWN_CHANCE = 0.015f;
                static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
                static constexpr float POSITION_JITTER = 0.4f;
            };

            // ─── Property Index Registry (Sphere) ────────────────────────
            // Seed source: tile_seed (shared with all families)
            // Range: 100–126 (original floating range, preserved for seed stability)
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

            // ─── Sphere CPU Tracking ─────────────────────────────────────
            struct ActiveFloater {
                int32_t patch_gx = 0, patch_gz = 0;
                int32_t host_gx = 0, host_gz = 0;
                bool active = false;
            };
            ActiveFloater activeFloaters_[Dim::MAX_SPHERE_INSTANCES]{};
            uint32_t activeFloaterCount_ = 0;

            // ═══ Sphere Entity System End ═════════════════════════════════


// ─── Cube Entity System ─────────────────────────────────────────
//
// Hover-bob monoliths. Colorful cubes/slabs floating above terrain.
// Slots 0 .. MAX_CUBE_INSTANCES-1 (buffer offset by CUBE_SLOT_OFFSET).
// GPU compute: update_cube. Vertex shader: monolith_vs.
// ─────────────────────────────────────────────────────────────────

            // ─── Cube Tier Profile ───────────────────────────────────────
            struct CubeTierProfile {
                float body_radius_mean, body_radius_sigma;
                float orbit_height_mean, orbit_height_sigma;
                float influence_radius_mean, influence_radius_sigma;
                float spin_speed_mean, spin_speed_sigma;
                float bob_amplitude_mean, bob_amplitude_sigma;
                float bob_period_mean, bob_period_sigma;
                float spin_tilt_sigma;
                float aspect_y_mean, aspect_y_sigma;
                float aspect_z_mean, aspect_z_sigma;
                float face_variance_mean, face_variance_sigma;
                float weight;
            };

            static constexpr uint32_t CUBE_TIER_COUNT = 4;
            static constexpr CubeTierProfile CUBE_TIERS[CUBE_TIER_COUNT] = {
                //                    rad_μ  σ     ht_μ   σ      inf_μ  σ     spin_μ σ     bob_μ  σ    per_μ  σ    tilt   asp_y  σ    asp_z  σ    fvar_μ σ     wt
                /* 0: SmallCube */ {  1.8f, 0.5f,  15.0f, 12.0f,  6.0f, 1.5f,  0.04f,0.015f,1.0f, 0.3f,  5.0f, 1.5f,  0.12f, 1.0f,0.15f, 1.0f,0.15f, 0.40f,0.12f,  0.40f },
                /* 1: MedCube   */ {  4.0f, 1.2f,  25.0f, 18.0f, 10.0f, 2.0f,  0.03f,0.01f, 1.5f, 0.4f,  6.0f, 2.0f,  0.10f, 1.0f,0.20f, 1.0f,0.20f, 0.45f,0.15f,  0.32f },
                /* 2: LargeCube */ {  8.0f, 2.5f,  35.0f, 20.0f, 14.0f, 3.0f,  0.02f,0.008f,2.0f, 0.5f,  8.0f, 2.5f,  0.08f, 1.0f,0.25f, 1.0f,0.25f, 0.35f,0.10f,  0.20f },
                /* 3: Monolith  */ {  3.0f, 0.8f,   5.0f,  2.0f, 12.0f, 3.0f,  0.015f,0.005f,1.2f,0.3f,  6.0f, 2.0f,  0.10f, 5.0f,1.2f,  0.15f,0.03f, 0.45f,0.12f,  0.08f },
            };

            static constexpr float CUBE_BASE_TIER_WEIGHTS[CUBE_TIER_COUNT] = { 0.40f, 0.32f, 0.20f, 0.08f };
            static constexpr const char* CUBE_TIER_NAMES[] = { "SmallCube", "MedCube", "LargeCube", "Monolith" };

            // ─── Cube Spawn Configuration ────────────────────────────────
            struct CubeConfig {
                static constexpr float SPAWN_CHANCE = 0.060f;
                static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
                static constexpr float POSITION_JITTER = 0.4f;
            };

            // ─── Property Index Registry (Cube) ──────────────────────────
            // Range: 130–156 (avoids sphere's 100–126)
            struct CubeEntityProp {
                static constexpr uint32_t SPAWN_ROLL       = 130u;
                static constexpr uint32_t ANCHOR_X         = 131u;
                static constexpr uint32_t ANCHOR_Z         = 132u;
                static constexpr uint32_t TIER             = 133u;
                static constexpr uint32_t BODY_RADIUS      = 140u;
                static constexpr uint32_t ORBIT_HEIGHT     = 142u;
                static constexpr uint32_t INFLUENCE_RADIUS = 144u;
                static constexpr uint32_t SPIN_SPEED       = 145u;
                static constexpr uint32_t BOB_AMPLITUDE    = 146u;
                static constexpr uint32_t BOB_PERIOD       = 147u;
                static constexpr uint32_t SPIN_TILT_X      = 148u;
                static constexpr uint32_t SPIN_TILT_Z      = 149u;
                static constexpr uint32_t COLOR_R          = 150u;
                static constexpr uint32_t COLOR_G          = 151u;
                static constexpr uint32_t COLOR_B          = 152u;
                static constexpr uint32_t ASPECT_Y         = 153u;
                static constexpr uint32_t ASPECT_Z         = 154u;
                static constexpr uint32_t FACE_VARIANCE    = 155u;
                static constexpr uint32_t ROTATION         = 156u;
            };

            // ─── Cube CPU Tracking ───────────────────────────────────────
            struct ActiveCube {
                int32_t patch_gx = 0, patch_gz = 0;
                int32_t host_gx = 0, host_gz = 0;
                bool active = false;
            };
            ActiveCube activeCubes_[Dim::MAX_CUBE_INSTANCES]{};
            uint32_t activeCubeCount_ = 0;

            // ═══ Cube Entity System End ═══════════════════════════════════

            // ═══ Ribbon Dispatch Pipeline ═══════════════════════════════════
            //
            // Single-instance ribbon through the 3-phase dispatch pipeline.
            // GPU buffer is singleton (upload_ribbon, not slot-indexed).

            // ─── fill_ribbon_selection_geometry ───────────────────────────
            // Shared geometry + color sampler used by both the dispatch
            // pipeline and the mood forced-spawn path.
            void fill_ribbon_selection_geometry(
                uint32_t seed, uint32_t tier_idx, float terrain_est,
                RibbonSelection& sel) const
            {
                const auto& tp = RIBBON_TIERS[tier_idx];

                float count_f = std::max(20.0f,
                    cpu_sample_gaussian(seed, RibbonProp::CUBE_COUNT, tp.cube_count_mean, tp.cube_count_sigma));
                sel.cube_count = std::min((uint32_t)count_f, Dim::RIBBON_MAX_RINGS);
                sel.cube_size = std::max(1.0f,
                    cpu_sample_gaussian(seed, RibbonProp::CUBE_SIZE, tp.cube_size_mean, tp.cube_size_sigma));

                // Length cap — keeps anchor coverage viable (~30 patches max)
                if ((float)sel.cube_count * sel.cube_size > RIBBON_MAX_LENGTH)
                    sel.cube_count = (uint32_t)(RIBBON_MAX_LENGTH / sel.cube_size);

                sel.height = terrain_est + std::max(20.0f,
                    cpu_sample_gaussian(seed, RibbonProp::HEIGHT, tp.height_mean, tp.height_sigma));

                sel.orientation = cpu_hash_f(seed, RibbonProp::ORIENTATION) * 6.2831853f;

                sel.lateral_amp = std::max(0.1f, cpu_sample_gaussian(seed, RibbonProp::LATERAL_AMP, tp.lateral_amp_mean, tp.lateral_amp_sigma));
                sel.lateral_cycles = std::max(0.1f, cpu_sample_gaussian(seed, RibbonProp::LATERAL_CYCLES, tp.lateral_cycles_mean, tp.lateral_cycles_sigma));
                sel.lateral_speed = std::max(0.005f, cpu_sample_gaussian(seed, RibbonProp::LATERAL_SPEED, tp.lateral_speed_mean, tp.lateral_speed_sigma));

                sel.vertical_amp = std::max(0.1f, cpu_sample_gaussian(seed, RibbonProp::VERTICAL_AMP, tp.vertical_amp_mean, tp.vertical_amp_sigma));
                sel.vertical_cycles = sel.lateral_cycles;
                sel.vertical_speed = sel.lateral_speed;

                sel.twist_amp = std::max(0.0f, cpu_sample_gaussian(seed, RibbonProp::TWIST_AMP, tp.twist_amp_mean, tp.twist_amp_sigma));
                sel.twist_cycles = sel.lateral_cycles;
                sel.twist_speed = sel.lateral_speed;

                // Color
                float color_roll = cpu_hash_f(seed, RibbonProp::COLOR_ROLL);
                sel.color_mode = RibbonColorMode::COUNT - 1;
                float ccum = 0.0f;
                for (uint32_t c = 0; c < RibbonColorMode::COUNT; c++) {
                    ccum += RibbonColorMode::WEIGHTS[c];
                    if (color_roll < ccum) { sel.color_mode = c; break; }
                }

                if (sel.color_mode == RibbonColorMode::SMOOTH) {
                    uint32_t pal_idx = (uint32_t)(cpu_hash_f(seed, RibbonProp::PALETTE_IDX) * RIBBON_SMOOTH_PALETTE_COUNT);
                    if (pal_idx >= RIBBON_SMOOTH_PALETTE_COUNT) pal_idx = RIBBON_SMOOTH_PALETTE_COUNT - 1;
                    float var = cpu_hash_f(seed, RibbonProp::COLOR_R) * 0.10f - 0.05f;
                    sel.color[0] = RIBBON_SMOOTH_PALETTE[pal_idx][0] + var;
                    sel.color[1] = RIBBON_SMOOTH_PALETTE[pal_idx][1] + var * 0.8f;
                    sel.color[2] = RIBBON_SMOOTH_PALETTE[pal_idx][2] + var * 0.6f;
                }
                else if (sel.color_mode == RibbonColorMode::TINTED) {
                    sel.color[0] = cpu_hash_f(seed, RibbonProp::COLOR_R) * 0.45f + 0.40f;
                    sel.color[1] = cpu_hash_f(seed, RibbonProp::COLOR_G) * 0.40f + 0.35f;
                    sel.color[2] = cpu_hash_f(seed, RibbonProp::COLOR_B) * 0.45f + 0.35f;
                }
                else {
                    float hue = cpu_hash_f(seed, RibbonProp::COLOR_R);
                    sel.color[0] = 0.20f + hue * 0.35f;
                    sel.color[1] = 0.18f + (1.0f - hue) * 0.30f;
                    sel.color[2] = 0.22f + cpu_hash_f(seed, RibbonProp::COLOR_B) * 0.25f;
                }

                sel.footprint_r = 5.0f;
            }

            // ─── select_ribbon_for_patch ──────────────────────────────────
            bool select_ribbon_for_patch(int32_t gx, int32_t gz, RibbonSelection& sel) {
                // Tip-overlap idempotency: reject if ANY active ribbon's
                // near or far tip falls within this trigger patch.
                for (uint32_t i = 0; i < MAX_RIBBON_INSTANCES; i++) {
                    if (!activeRibbons_[i].active) continue;
                    if ((activeRibbons_[i].near_tip_gx == gx && activeRibbons_[i].near_tip_gz == gz) ||
                        (activeRibbons_[i].far_tip_gx == gx && activeRibbons_[i].far_tip_gz == gz))
                        return false;
                }
                auto gate = run_spawn_preamble(gx, gz,
                    activeRibbons_, MAX_RIBBON_INSTANCES,
                    RibbonProp::SPAWN_ROLL, RibbonConfig::SPAWN_CHANCE,
                    RibbonConfig::MOOD_MULTIPLIER,
                    PopFamily::RIBBON, "ribn");
                if (!gate.ok) return false;

                // Tier selection with theme bias
                float tier_weights[RIBBON_TIER_COUNT];
                for (uint32_t t = 0; t < RIBBON_TIER_COUNT; t++)
                    tier_weights[t] = RIBBON_BASE_TIER_WEIGHTS[t];
                for (uint32_t t = 0; t < RIBBON_TIER_COUNT; t++)
                    tier_weights[t] *= THEMES[gate.theme_idx].tier_wt_ribbon[t];
                uint32_t tier_idx = select_tier_biased(gate.seed, RibbonProp::TIER,
                    tier_weights, RIBBON_TIER_COUNT, PopFamily::RIBBON);

                sel.seed = gate.seed;
                sel.trigger_gx = gx;
                sel.trigger_gz = gz;
                sel.slot = gate.slot;
                sel.tier_idx = tier_idx;

                float terrain_est = estimate_terrain_height(
                    (gx + 0.5f) * PATCH_EXTENT, (gz + 0.5f) * PATCH_EXTENT);

                fill_ribbon_selection_geometry(gate.seed, tier_idx, terrain_est, sel);

                // Constrain orientation: ribbon body extends primarily away
                // from the pawn. The hash provides ±60° of spread around the
                // away direction so ribbons aren't all perfectly radial.
                {
                    float patch_cx = (gx + 0.5f) * PATCH_EXTENT;
                    float patch_cz = (gz + 0.5f) * PATCH_EXTENT;
                    float away_angle = std::atan2(patch_cz - pawnReadback_z_,
                                                  patch_cx - pawnReadback_x_);
                    constexpr float SPREAD = 1.0472f; // ±60° = π/3
                    float hash_spread = cpu_hash_f(gate.seed, RibbonProp::ORIENTATION);
                    sel.orientation = away_angle + (hash_spread * 2.0f - 1.0f) * SPREAD;
                }

                return true;
            }

            // ─── place_ribbon_from_selection ──────────────────────────────
            bool place_ribbon_from_selection(const RibbonSelection& sel, RibbonPlacement& plan) {
                auto pos = negotiate_position(sel.seed,
                    sel.trigger_gx, sel.trigger_gz,
                    RibbonProp::ANCHOR_X, RibbonProp::ANCHOR_Z,
                    RibbonConfig::POSITION_JITTER,
                    RibbonProp::ORIENTATION,
                    sel.footprint_r, PopFamily::RIBBON, sel.tier_idx);
                if (!pos.ok) return false;

                plan = RibbonPlacement{};
                plan.slot = sel.slot;
                plan.trigger_gx = sel.trigger_gx;
                plan.trigger_gz = sel.trigger_gz;
                plan.host_gx = pos.host_gx;
                plan.host_gz = pos.host_gz;
                plan.tier_idx = sel.tier_idx;
                plan.cx = pos.cx;
                plan.cz = pos.cz;
                plan.rotation = pos.rotation;

                plan.cube_count = sel.cube_count;
                plan.cube_size = sel.cube_size;
                plan.height = sel.height;
                plan.orientation = sel.orientation;
                plan.lateral_amp = sel.lateral_amp;
                plan.lateral_cycles = sel.lateral_cycles;
                plan.lateral_speed = sel.lateral_speed;
                plan.vertical_amp = sel.vertical_amp;
                plan.vertical_cycles = sel.vertical_cycles;
                plan.vertical_speed = sel.vertical_speed;
                plan.twist_amp = sel.twist_amp;
                plan.twist_cycles = sel.twist_cycles;
                plan.twist_speed = sel.twist_speed;
                plan.color_mode = sel.color_mode;
                std::memcpy(plan.color, sel.color, sizeof(plan.color));

                record_placement_bookkeeping(PopFamily::RIBBON, plan.tier_idx);
                return true;
            }

            // ─── commit_ribbon ───────────────────────────────────────────
            void commit_ribbon(const RibbonPlacement& plan,
                int32_t trigger_gx, int32_t trigger_gz, wgpu::Queue& queue)
            {
                GPURibbonState r{};
                r.anchor[0] = plan.cx;
                r.anchor[1] = 0.0f;
                r.anchor[2] = plan.cz;
                r.time = currentSeconds_;
                r.cube_count = plan.cube_count;
                r.cube_size = plan.cube_size;
                r.height = plan.height;
                r.orientation = plan.orientation;
                r.lateral_amp = plan.lateral_amp;
                r.lateral_cycles = plan.lateral_cycles;
                r.lateral_speed = plan.lateral_speed;
                r.vertical_amp = plan.vertical_amp;
                r.vertical_cycles = plan.vertical_cycles;
                r.vertical_speed = plan.vertical_speed;
                r.twist_amp = plan.twist_amp;
                r.twist_cycles = plan.twist_cycles;
                r.twist_speed = plan.twist_speed;
                r.color_mode = plan.color_mode;
                r.color[0] = plan.color[0];
                r.color[1] = plan.color[1];
                r.color[2] = plan.color[2];
                r.is_visible = 1u;

                // Store in CPU mirror (per-frame nearest-selection uploads to GPU)
                uint32_t s = plan.slot;
                ribbonStates_[s] = r;

                auto& ar = activeRibbons_[s];
                ar.patch_gx = trigger_gx;
                ar.patch_gz = trigger_gz;
                ar.host_gx = plan.host_gx;
                ar.host_gz = plan.host_gz;
                ar.anchor_x = plan.cx;
                ar.anchor_z = plan.cz;

                // Two-tip anchoring: anchor IS the near tip (t=0).
                // Body extends entirely in the orientation direction (away from pawn).
                float total_length = (float)plan.cube_count * plan.cube_size;
                float dir_x = std::cos(plan.orientation);
                float dir_z = std::sin(plan.orientation);

                ar.near_tip_x = plan.cx;
                ar.near_tip_z = plan.cz;
                ar.far_tip_x = plan.cx + dir_x * total_length;
                ar.far_tip_z = plan.cz + dir_z * total_length;

                ar.near_tip_gx = (int32_t)std::floor(ar.near_tip_x / PATCH_EXTENT);
                ar.near_tip_gz = (int32_t)std::floor(ar.near_tip_z / PATCH_EXTENT);
                ar.far_tip_gx = (int32_t)std::floor(ar.far_tip_x / PATCH_EXTENT);
                ar.far_tip_gz = (int32_t)std::floor(ar.far_tip_z / PATCH_EXTENT);

                ar.near_tip_registered = false;
                ar.far_tip_registered = false;
                ar.ref_count = 0;

                ar.active = true;
                activeRibbonCount_++;
                std::cout << "[Ribbon] SPAWN slot=" << s << " at (" << plan.cx << ", " << plan.cz
                    << ") tier=" << plan.tier_idx
                    << " len=" << total_length
                    << " near=(" << ar.near_tip_gx << "," << ar.near_tip_gz
                    << ") far=(" << ar.far_tip_gx << "," << ar.far_tip_gz << ")\n";
            }

            // ═══ Ribbon Dispatch Pipeline End ═══════════════════════════════

            // ── GoL Zones (modules/gol_zones.inl) ──
            // ═══ INLINED: modules/gol_zones.inl ═══════════════════════════════

// ─── gol_zones.inl ──────────────────────────────────────────────
//
// Zone-local Game of Life + Pulse automata. Each zone is a 32×32
// grid anchored to a mode lattice node. Conway zones evolve by
// neighbor rules; Pulse zones breathe periodically.
//
// Complete subsystem: tier profiles, spawn detection, life buffer
// seeding, eviction, per-frame config upload, derive request flush.
//
// Included inside the Cartridge class body.
// Depends on: seed_utils.inl, spawn_engine.inl (footprint registry)
// ─────────────────────────────────────────────────────────────────

// ─── GoL Zone System ─────────────────────────────────────────────
//
// Zone-local Game of Life. Each zone is a 32×32 automaton grid
// anchored to a mode lattice node (MODE_LATTICE_SPACING units).
// Zone detection replicates the GPU's tag_cell_behavior roll
// using the same deterministic seed, ensuring CPU and GPU agree.
//
// Architecture follows the Column entity pattern:
//   GoLZoneProp       — property index registry (seed-based rolls)
//   GoLZoneSpawnConfig — spawn chances and spatial constants
//   GoLTierProfile    — mean+sigma tier matrix (Gaussian sampling)
//   GoLColorMode      — color tier weights (declarative)
//   GoLZoneState      — per-instance runtime state

            static constexpr float MODE_LATTICE_SPACING = 120.0f;
            static constexpr float PATCH_CELL_SIZE = (float)Dim::PATCH_EXTENT / 16.0f;  // 3.125

            // ── Property Index Registry (seed band 250, indices 920–939) ─────
            struct GoLZoneProp {
                static constexpr uint32_t SEED_BAND = 250u;
                // Zone-level decisions
                static constexpr uint32_t SPAWN_ROLL = 920u;
                static constexpr uint32_t TIER = 921u;
                static constexpr uint32_t HEIGHT_ROLL = 922u;
                static constexpr uint32_t COLOR_ROLL = 923u;
                // Per-zone continuous parameters (Gaussian draws)
                static constexpr uint32_t DENSITY = 930u;
                static constexpr uint32_t TICK_PERIOD = 931u;
                static constexpr uint32_t SPRING = 932u;
                static constexpr uint32_t HEIGHT = 933u;
                static constexpr uint32_t TRANSITION = 934u;
                // Per-zone color target
                static constexpr uint32_t TARGET_R = 935u;
                static constexpr uint32_t TARGET_G = 936u;
                static constexpr uint32_t TARGET_B = 937u;
                // Per-cell seeding
                static constexpr uint32_t HEIGHT_FACTOR = 938u;
            };

            // ── Spawn Configuration ──────────────────────────────────────────
            struct GoLZoneSpawnConfig {
                static constexpr float SPAWN_CHANCE = 0.15f;  // fraction of checkerboard zones (was 0.10)
                static constexpr float HEIGHT_CHANCE = 0.30f;  // fraction of zones that get extrusion (was 0.40)
                static constexpr float ZONE_EXTENT = 100.0f; // 32 × 3.125 = cell-aligned
                static constexpr float MODE_THRESHOLD = 0.50f;  // min interpolated mode for eligibility
                // Per-cell height factor seeding (Gaussian draw per cell)
                static constexpr float HEIGHT_FACTOR_MEAN = 1.0f;
                static constexpr float HEIGHT_FACTOR_SIGMA = 0.15f;
                static constexpr float HEIGHT_FACTOR_CLAMP_LO = 0.6f;
                static constexpr float HEIGHT_FACTOR_CLAMP_HI = 1.4f;
                // Lens target color range: color = hash * RANGE + LO
                static constexpr float LENS_TARGET_LO = 0.2f;
                static constexpr float LENS_TARGET_RANGE = 0.6f;
                // Footprint: inscribed circle of 100×100 zone
                static constexpr float FOOTPRINT_RADIUS = 50.0f;
                // Mood gate (suppressed in flat/vault/finR — same as spheres/cubes)
                static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
            };

            // ── Color Modes ──────────────────────────────────────────────────
            struct GoLColorMode {
                static constexpr uint32_t NEUTRAL = 0;  // no color change (height-only extrusion)
                static constexpr uint32_t LENS = 1;  // shift toward per-zone target color
                static constexpr uint32_t BLACKISH = 2;  // darken toward near-black
                static constexpr uint32_t COUNT = 3;

                // Weight matrix: color_mode selection weights
                // Index 0 = NEUTRAL (only available if height_enabled)
                // Index 1 = LENS
                // Index 2 = BLACKISH
                static constexpr float WEIGHTS_HEIGHT[COUNT] = { 0.30f, 0.40f, 0.30f };
                static constexpr float WEIGHTS_NO_HEIGHT[COUNT] = { 0.00f, 0.55f, 0.45f };
            };

            // ── Tier Profile (mean+sigma, matches ColumnTierParams pattern) ──
            static constexpr uint32_t GOL_TIER_COUNT = 7;

            struct GoLTierProfile {
                // ─── Initial conditions ──────────────────────────────────
                float density_mean, density_sigma;

                // ─── Temporal ────────────────────────────────────────────
                float tick_period_mean, tick_period_sigma;

                // ─── Visual transition ───────────────────────────────────
                float spring_stiffness_mean, spring_stiffness_sigma;
                float transition_fraction_mean, transition_fraction_sigma;

                // ─── Height ──────────────────────────────────────────────
                float alive_height_mean, alive_height_sigma;

                // ─── Per-cell variation ──────────────────────────────────
                float spring_variance;     // [0,1] per-cell spring speed scatter

                // ─── Selection ───────────────────────────────────────────
                float weight;
                bool  force_no_height;
            };

            //                                                    dens_μ   σ    tick_μ  σ    spring_μ σ    trans_μ  σ     ht_μ    σ    sv    wt    no_h
            static constexpr GoLTierProfile GOL_TIERS[GOL_TIER_COUNT] = {
                /* 0: Pillars  */ { 0.30f, 0.05f,   8.0f, 2.0f,   0.5f, 0.1f,   0.05f, 0.01f,  30.0f, 9.0f,  0.30f,  0.10f, false },
                /* 1: Sparse   */ { 0.15f, 0.05f,   2.0f, 0.5f,   4.0f, 1.0f,   0.12f, 0.03f,  18.0f, 6.0f,  0.20f,  0.20f, false },
                /* 2: Moderate */ { 0.30f, 0.08f,   1.0f, 0.3f,   8.0f, 2.0f,   0.15f, 0.03f,   9.0f, 3.0f,  0.15f,  0.18f, false },
                /* 3: Dense    */ { 0.45f, 0.10f,   0.5f, 0.15f, 12.0f, 3.0f,   0.25f, 0.05f,   6.0f, 1.5f,  0.10f,  0.10f, false },
                /* 4: Flash    */ { 0.35f, 0.10f,  0.25f, 0.05f, 20.0f, 5.0f,   0.30f, 0.05f,   0.0f, 0.0f,  0.40f,  0.17f, true  },
                /* 5: Monolith */ { 0.20f, 0.03f,  12.0f, 3.0f,   0.3f, 0.05f,  0.03f, 0.01f,  42.0f, 12.f,  0.05f,  0.12f, false },
                /* 6: Glacier  */ { 0.12f, 0.03f,   4.0f, 1.0f,   2.0f, 0.5f,   0.08f, 0.02f,  24.0f, 7.5f,  0.25f,  0.13f, false },
            };

            static constexpr const char* GOL_TIER_NAMES[] = {
                "Pillars", "Sparse", "Moderate", "Dense",
                "Flash", "Monolith", "Glacier"
            };

            static constexpr const char* GOL_COLOR_NAMES[] = {
                "neutral", "lens", "blackish"
            };

            // ── Algorithm Types ───────────────────────────────────────────────
            struct AlgorithmType {
                static constexpr uint32_t CONWAY = 0;
                static constexpr uint32_t PULSE = 1;
            };

            // ── Boundary Modes ────────────────────────────────────────────────
            struct BoundaryMode {
                static constexpr uint32_t REFLECT = 0;
                static constexpr uint32_t WRAP = 1;
            };

            // ── Pulse Tier Profile ────────────────────────────────────────────
            //
            // Pulse zones: periodic breathing of cell color/height, no neighbor rules.
            // Each cell oscillates between terrain base and a displaced target.
            static constexpr uint32_t PULSE_TIER_COUNT = 3;

            struct PulseTierProfile {
                // ─── Temporal ────────────────────────────────────────────
                float tick_period_mean, tick_period_sigma;

                // ─── Visual transition ───────────────────────────────────
                float spring_stiffness_mean, spring_stiffness_sigma;
                float transition_fraction_mean, transition_fraction_sigma;

                // ─── Phase scatter ───────────────────────────────────────
                float phase_randomness_mean, phase_randomness_sigma;

                // ─── Tempo scatter ───────────────────────────────────────
                float tempo_randomness_mean, tempo_randomness_sigma;

                // ─── Height ──────────────────────────────────────────────
                float alive_height_mean, alive_height_sigma;

                // ─── Wander ──────────────────────────────────────────────
                float wander_radius_mean, wander_radius_sigma;

                // ─── Per-cell variation ──────────────────────────────────
                float spring_variance;

                // ─── Selection ───────────────────────────────────────────
                float weight;
                bool  force_no_height;
                uint32_t boundary_mode;
            };

            //                                                        tick_μ   σ    spring_μ σ    trans_μ  σ    phase_μ  σ    tempo_μ σ    ht_μ   σ    wand_μ  σ    sv    wt    no_h  bnd
            static constexpr PulseTierProfile PULSE_TIERS[PULSE_TIER_COUNT] = {
                /* 0: Breathe  */ { 2.0f, 0.5f,   4.0f, 1.0f,   0.20f, 0.05f,   0.15f, 0.05f,   0.10f, 0.03f,   2.0f, 0.8f,  10.0f, 3.0f,   0.20f,  0.45f, false, BoundaryMode::REFLECT },
                /* 1: Sparkle  */ { 0.5f, 0.15f, 12.0f, 3.0f,   0.25f, 0.05f,   0.90f, 0.10f,   0.60f, 0.15f,   0.0f, 0.0f,   5.0f, 2.0f,   0.50f,  0.30f, true,  BoundaryMode::REFLECT },
                /* 2: Drift    */ { 4.0f, 1.0f,   1.5f, 0.4f,   0.10f, 0.03f,   0.50f, 0.15f,   0.40f, 0.10f,   4.0f, 1.5f,  25.0f, 8.0f,   0.35f,  0.25f, false, BoundaryMode::WRAP    },
            };

            static constexpr const char* PULSE_TIER_NAMES[] = {
                "Breathe", "Sparkle", "Drift"
            };

            // Probability of a zone being Pulse (vs Conway)
            static constexpr float PULSE_ALGORITHM_CHANCE = 0.35f;

            // ── Property Indices for Pulse-specific parameters ────────────────
            struct PulseZoneProp {
                static constexpr uint32_t ALGORITHM_ROLL = 950u;
                static constexpr uint32_t PULSE_TIER = 951u;
                static constexpr uint32_t PHASE_RANDOM = 952u;
                static constexpr uint32_t WANDER = 953u;
                static constexpr uint32_t TEMPO_RANDOM = 954u;
            };

            // ── Per-Instance Runtime State ────────────────────────────────────
            // CPU retains only what's needed for: tick mask computation, life seeding,
            // and zone slot lifecycle. All visual/spring/color parameters are GPU-derived.
            struct GoLZoneState {
                int32_t zone_nx = 0, zone_nz = 0;
                int32_t host_gx = 0, host_gz = 0;   // host patch (for entity_refs eviction)
                bool active = false;
                uint32_t algorithm = AlgorithmType::CONWAY;
                float tick_period = 1.0f;        // CPU derives this for tick mask (matches GPU)
                float initial_density = 0.3f;    // CPU needs this for life buffer seeding
                int32_t last_tick_index = -1;
            };

            GoLZoneState golZones_[Dim::MAX_GOL_ZONES]{};
            uint32_t golZoneCount_ = 0;

            // Per-mood feature gates — set by apply_mood from MoodProfile flags.
            // These gate where features enter the frame loop; default = all on.
            bool moodAllowsMusicalModes_ = true;
            bool moodAllowsGoLZones_     = true;

            // Derive request queue: accumulated during patch gen, flushed once per frame.
            GPUZoneDeriveRequestArray pendingDeriveRequests_{};
            uint32_t activeZoneSlotCount_ = 0;  // highest active slot + 1 (for dispatch sizing)

            // ─── select_gol_for_patch ─────────────────────────────────────
            //
            // Phase 1: lattice-node-gated selection. Scans which MODE_LATTICE
            // nodes overlap this patch, runs spawn roll, algorithm + tier
            // selection, parameter sampling. At most one zone per patch.

            bool select_gol_for_patch(int32_t gx, int32_t gz, GoLSelection& sel) {
                // Mood gate
                float adj_mod = GoLZoneSpawnConfig::MOOD_MULTIPLIER[activeMood_];
                if (adj_mod <= 0.0f) return false;

                // Density + theme modifiers
                adj_mod *= GLOBAL_ENTITY_DENSITY;
                adj_mod *= population_type_affinity(PopFamily::GOL);
                {
                    auto dit = tileCache_.find({ gx, gz });
                    if (dit != tileCache_.end()) {
                        adj_mod *= dit->second.entity_density;
                        adj_mod *= dit->second.theme_spawn[PopFamily::GOL];
                    }
                }

                // Scan lattice nodes overlapping this patch
                float wx0 = gx * PATCH_EXTENT;
                float wx1 = (gx + 1) * PATCH_EXTENT;
                float wz0 = gz * PATCH_EXTENT;
                float wz1 = (gz + 1) * PATCH_EXTENT;

                int32_t nx0 = (int32_t)std::floor(wx0 / MODE_LATTICE_SPACING);
                int32_t nx1 = (int32_t)std::floor(wx1 / MODE_LATTICE_SPACING);
                int32_t nz0 = (int32_t)std::floor(wz0 / MODE_LATTICE_SPACING);
                int32_t nz1 = (int32_t)std::floor(wz1 / MODE_LATTICE_SPACING);

                for (int32_t nz = nz0; nz <= nz1; nz++) {
                    for (int32_t nx = nx0; nx <= nx1; nx++) {
                        // Zone center from lattice node
                        float raw_cx = (nx + 0.5f) * MODE_LATTICE_SPACING;
                        float raw_cz = (nz + 0.5f) * MODE_LATTICE_SPACING;

                        // Authoritative patch: only the patch containing the center owns this node
                        int32_t auth_gx = (int32_t)std::floor(raw_cx / PATCH_EXTENT);
                        int32_t auth_gz = (int32_t)std::floor(raw_cz / PATCH_EXTENT);
                        if (auth_gx != gx || auth_gz != gz) continue;

                        // Idempotency: already active at this node?
                        bool exists = false;
                        for (uint32_t i = 0; i < Dim::MAX_GOL_ZONES; i++) {
                            if (golZones_[i].active &&
                                golZones_[i].zone_nx == nx && golZones_[i].zone_nz == nz) {
                                exists = true; break;
                            }
                        }
                        if (exists) continue;

                        // Spawn roll with modifiers
                        uint32_t seed = cpu_lattice_node_seed(activeSeed_, nx, nz, GoLZoneProp::SEED_BAND);
                        float roll = cpu_hash_f(seed, GoLZoneProp::SPAWN_ROLL);
                        float chance = GoLZoneSpawnConfig::SPAWN_CHANCE * adj_mod;
                        chance += population_automata_bias();
                        chance = std::max(0.0f, std::min(1.0f, chance));
                        if (roll >= chance) continue;

                        // Find free slot
                        uint32_t slot = UINT32_MAX;
                        for (uint32_t i = 0; i < Dim::MAX_GOL_ZONES; i++) {
                            if (!golZones_[i].active) { slot = i; break; }
                        }
                        if (slot == UINT32_MAX) continue;

                        // Reserve slot
                        golZones_[slot].active = true;

                        // Zone corner (cell-grid-snapped)
                        float corner_x = std::floor(
                            (raw_cx - GoLZoneSpawnConfig::ZONE_EXTENT * 0.5f) / PATCH_CELL_SIZE) * PATCH_CELL_SIZE;
                        float corner_z = std::floor(
                            (raw_cz - GoLZoneSpawnConfig::ZONE_EXTENT * 0.5f) / PATCH_CELL_SIZE) * PATCH_CELL_SIZE;

                        // Algorithm selection
                        float algo_roll = cpu_hash_f(seed, PulseZoneProp::ALGORITHM_ROLL);
                        uint32_t algorithm = (algo_roll < PULSE_ALGORITHM_CHANCE)
                            ? AlgorithmType::PULSE : AlgorithmType::CONWAY;

                        // Height enabled
                        float height_roll = cpu_hash_f(seed, GoLZoneProp::HEIGHT_ROLL);
                        bool height_enabled = (height_roll < GoLZoneSpawnConfig::HEIGHT_CHANCE);

                        // Tier + CPU-side params
                        float tick_period = 1.0f;
                        float initial_density = 0.0f;
                        uint32_t tier_idx = 0;

                        if (algorithm == AlgorithmType::CONWAY) {
                            float tier_roll = cpu_hash_f(seed, GoLZoneProp::TIER);
                            uint32_t tier = GOL_TIER_COUNT - 1;
                            float cumul = 0.0f;
                            for (uint32_t t = 0; t < GOL_TIER_COUNT; t++) {
                                cumul += GOL_TIERS[t].weight;
                                if (tier_roll < cumul) { tier = t; break; }
                            }
                            const auto& tp = GOL_TIERS[tier];
                            if (tp.force_no_height) height_enabled = false;
                            tick_period = std::max(0.1f,
                                cpu_sample_gaussian(seed, GoLZoneProp::TICK_PERIOD,
                                    tp.tick_period_mean, tp.tick_period_sigma));
                            initial_density = std::max(0.05f, std::min(0.9f,
                                cpu_sample_gaussian(seed, GoLZoneProp::DENSITY,
                                    tp.density_mean, tp.density_sigma)));
                            tier_idx = tier;  // Conway: 0–6
                        } else {
                            float tier_roll = cpu_hash_f(seed, PulseZoneProp::PULSE_TIER);
                            uint32_t tier = PULSE_TIER_COUNT - 1;
                            float cumul = 0.0f;
                            for (uint32_t t = 0; t < PULSE_TIER_COUNT; t++) {
                                cumul += PULSE_TIERS[t].weight;
                                if (tier_roll < cumul) { tier = t; break; }
                            }
                            const auto& pp = PULSE_TIERS[tier];
                            if (pp.force_no_height) height_enabled = false;
                            tick_period = std::max(0.1f,
                                cpu_sample_gaussian(seed, GoLZoneProp::TICK_PERIOD,
                                    pp.tick_period_mean, pp.tick_period_sigma));
                            initial_density = 0.0f;
                            tier_idx = GOL_TIER_COUNT + tier;  // Pulse: 7–9 (compound index)
                        }

                        // Fill selection
                        sel.seed = seed;
                        sel.trigger_gx = gx;
                        sel.trigger_gz = gz;
                        sel.slot = slot;
                        sel.zone_nx = nx;
                        sel.zone_nz = nz;
                        sel.corner_x = corner_x;
                        sel.corner_z = corner_z;
                        sel.algorithm = algorithm;
                        sel.tier_idx = tier_idx;
                        sel.tick_period = tick_period;
                        sel.initial_density = initial_density;
                        sel.height_enabled = height_enabled;
                        sel.footprint_r = GoLZoneSpawnConfig::FOOTPRINT_RADIUS;

                        return true;  // at most one zone per patch
                    }
                }
                return false;
            }

            // ─── place_gol_from_selection ─────────────────────────────────
            //
            // Phase 2: footprint check + registration. Position is lattice-
            // determined (no jitter), so we bypass negotiate_position and
            // call check_position + register_footprint directly.

            bool place_gol_from_selection(const GoLSelection& sel, GoLPlacement& plan) {
                float cx = sel.corner_x + GoLZoneSpawnConfig::ZONE_EXTENT * 0.5f;
                float cz = sel.corner_z + GoLZoneSpawnConfig::ZONE_EXTENT * 0.5f;

                if (!check_position(cx, cz, sel.footprint_r, PopFamily::GOL))
                    return false;

                int32_t host_gx = (int32_t)std::floor(cx / PATCH_EXTENT);
                int32_t host_gz = (int32_t)std::floor(cz / PATCH_EXTENT);

                if (register_footprint(cx, cz, sel.footprint_r,
                    host_gx, host_gz, PopFamily::GOL, sel.tier_idx) == UINT32_MAX)
                    return false;

                plan = GoLPlacement{};
                plan.slot = sel.slot;
                plan.trigger_gx = sel.trigger_gx;
                plan.trigger_gz = sel.trigger_gz;
                plan.host_gx = host_gx;
                plan.host_gz = host_gz;
                plan.tier_idx = sel.tier_idx;
                plan.cx = cx;
                plan.cz = cz;
                plan.zone_nx = sel.zone_nx;
                plan.zone_nz = sel.zone_nz;
                plan.corner_x = sel.corner_x;
                plan.corner_z = sel.corner_z;
                plan.algorithm = sel.algorithm;
                plan.tick_period = sel.tick_period;
                plan.initial_density = sel.initial_density;
                plan.height_enabled = sel.height_enabled;

                record_placement_bookkeeping(PopFamily::GOL, plan.tier_idx);
                return true;
            }

            // ─── commit_gol ──────────────────────────────────────────────
            //
            // Phase 3: CPU state + life buffer seeding + GPU derive request.

            void commit_gol(const GoLPlacement& plan,
                int32_t trigger_gx, int32_t trigger_gz, wgpu::Queue& queue)
            {
                auto& zone = golZones_[plan.slot];
                zone.zone_nx = plan.zone_nx;
                zone.zone_nz = plan.zone_nz;
                zone.host_gx = plan.host_gx;
                zone.host_gz = plan.host_gz;
                zone.active = true;
                zone.algorithm = plan.algorithm;
                zone.tick_period = plan.tick_period;
                zone.initial_density = plan.initial_density;
                zone.last_tick_index = -1;
                golZoneCount_++;

                seed_gol_zone(plan.slot, queue);

                if (pendingDeriveRequests_.count < Dim::MAX_GOL_ZONES) {
                    auto& req = pendingDeriveRequests_.requests[pendingDeriveRequests_.count++];
                    req.slot = plan.slot;
                    req.nx = plan.zone_nx;
                    req.nz = plan.zone_nz;
                    req.algorithm = plan.algorithm;
                    req.height_enabled = plan.height_enabled ? 1u : 0u;
                    req.world_seed = activeSeed_;
                }

                std::cout << "[GoL] "
                    << (plan.algorithm == AlgorithmType::PULSE ? "Pulse" : "Conway")
                    << " slot=" << plan.slot
                    << " node=(" << plan.zone_nx << "," << plan.zone_nz << ")"
                    << " corner=(" << plan.corner_x << "," << plan.corner_z << ")"
                    << " host=(" << plan.host_gx << "," << plan.host_gz << ")"
                    << (plan.height_enabled ? " HEIGHT" : "")
                    << " period=" << plan.tick_period
                    << "\n";
            }

            void seed_gol_zone(uint32_t slot, wgpu::Queue& queue) {
                auto& zone = golZones_[slot];
                uint32_t seed = cpu_lattice_node_seed(activeSeed_, zone.zone_nx, zone.zone_nz, GoLZoneProp::SEED_BAND);

                // Generate initial pattern
                std::vector<float> life(Dim::GOL_ZONE_CELLS, 0.0f);
                if (zone.algorithm == AlgorithmType::CONWAY) {
                    // Conway: random alive/dead from density
                    for (uint32_t i = 0; i < Dim::GOL_ZONE_CELLS; i++) {
                        float roll = cpu_hash_f(seed + i, GoLZoneProp::DENSITY);
                        life[i] = (roll < zone.initial_density) ? 1.0f : 0.0f;
                    }
                }
                // Pulse: all zeros — oscillation builds from silence

                // Generate per-cell height factors: Gaussian draw, clamped
                std::vector<float> height_factors(Dim::GOL_ZONE_CELLS);
                for (uint32_t i = 0; i < Dim::GOL_ZONE_CELLS; i++) {
                    float hf = cpu_sample_gaussian(seed + i, GoLZoneProp::HEIGHT_FACTOR,
                        GoLZoneSpawnConfig::HEIGHT_FACTOR_MEAN, GoLZoneSpawnConfig::HEIGHT_FACTOR_SIGMA);
                    height_factors[i] = std::max(GoLZoneSpawnConfig::HEIGHT_FACTOR_CLAMP_LO,
                        std::min(GoLZoneSpawnConfig::HEIGHT_FACTOR_CLAMP_HI, hf));
                }

                // Upload all 7 slots
                gpuState_.upload_zone_life(queue, slot, life.data(), height_factors.data(), Dim::GOL_ZONE_CELLS);
            }

            // upload_gol_zone_config: per-frame header-only upload.
            // Per-zone config is GPU-derived via zone_derive_params — we only
            // write count, t_beats, dt, tick_mask. Slot deactivation happens
            // at eviction time (via dispatch_evict_gol through entity_refs).
            void upload_gol_zone_config(wgpu::Queue& queue) {
                uint32_t count = 0;
                uint32_t tick_mask = 0;

                // GoL tempo mode: scale tick period (> 1 = slower, inverse of polyphony)
                float gol_tick_scale = 1.0f + mmodeIntensity_[MMODE_GOL_TEMPO] * 3.0f;

                for (uint32_t i = 0; i < Dim::MAX_GOL_ZONES; i++) {
                    if (!golZones_[i].active) continue;

                    // Conway tick gating: exactly one tick per period
                    // Scale period by musical mode (affects both Conway and Pulse in sync)
                    float effective_period = std::max(golZones_[i].tick_period * gol_tick_scale, 0.01f);
                    int32_t current_tick = (int32_t)std::floor(currentBeats_ / effective_period);
                    if (current_tick != golZones_[i].last_tick_index) {
                        tick_mask |= (1u << i);
                        golZones_[i].last_tick_index = current_tick;
                    }

                    count = i + 1;
                }
                gpuState_.upload_zone_config_header(queue, count, currentBeats_, currentDt_, tick_mask);
                activeZoneSlotCount_ = count;
            }

            // Flush pending zone derive requests as a GPU compute dispatch.
            // Called once per frame after all patch generation is complete.
            void flush_zone_derive_requests(wgpu::Queue& queue) {
                if (pendingDeriveRequests_.count == 0) return;

                gpuState_.upload_zone_derive_requests(queue, pendingDeriveRequests_);

                wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
                wgpu::ComputePassDescriptor desc{};
                desc.label = "Zone Derive Params";
                wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&desc);
                renderer_.dispatch_zone_derive_params(
                    pass,
                    gpuState_.zone_gol_compute_group(),
                    pendingDeriveRequests_.count);
                pass.End();
                wgpu::CommandBuffer cmd = encoder.Finish();
                queue.Submit(1, &cmd);

                pendingDeriveRequests_.count = 0;
            }

            // ═══ END INLINED: modules/gol_zones.inl ═════════════════════════

            // ── Gallery System (modules/gallery.inl) ──
            // ═══ INLINED: modules/gallery.inl ═══════════════════════════════

// ─── gallery.inl ─────────────────────────────────────────────────
//
// The art system. Photographer, paintings, exhibitions,
// wall paintings, authored image loading + staging.
//
// Outdoor: photographer captures snapshots, paintings spawn on terrain.
// Indoor: wall paintings placed by mood system (authored + snapshot mix).
//
// Included inside the Cartridge class body.
// Depends on: entities.inl, terrain_cpu.inl, seed_utils.inl
// ─────────────────────────────────────────────────────────────────


// ─── Self-Portrait Gallery (photographer system) ─────────────────
//
// ─── Shot Tiers ─────────────────────────────────────────────────
//
// Each tier defines a complete photographic character: how the
// invisible camera relates to the pawn in distance, angle, lens,
// and how the resulting painting takes shape on the terrain.
//
// ShotTypeParams fields:
//   distance_mean/sigma  — how far the camera orbits from the pawn (gaussian)
//   elevation_mean/sigma — vertical angle above horizon in radians (gaussian)
//   fov_degrees/sigma    — vertical field of view of the lens (gaussian)
//   aspect_lo/hi         — width/height ratio of the painting (uniform)
//   tracks_pawn          — whether the camera looks at the pawn or freely

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
            //
            // All parameters that define a tier's character live here.
            // To tune a tier: adjust its row. To add a tier: add a row + enum.

            //                          dist  σ     elev   σ     fov    σ     asp_lo asp_hi  track  off_x  off_y   weight
            static constexpr ShotTypeParams SHOT_PARAMS[] = {
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
            static constexpr float PAINTING_AREA[] = {
                30.0f,   // PANORAMIC:     large, cinematic canvas
                24.0f,   // ENVIRONMENTAL: medium canvas
                20.0f,   // MEDIUM:        moderate canvas
                20.0f,   // CLOSE_UP:      moderate canvas
                20.0f,   // PORTRAIT:      moderate (aspect makes it tall)
                18.0f,   // BIRDS_EYE:     moderate, near-square
                22.0f,   // LOW_ANGLE:     wide, dramatic
                28.0f,   // CINEMATIC:     large, ultrawide
            };

            // ─── Painting Spawn Configuration ───────────────────────────────
            //
            // Split into two concerns:
            //   PhotographerConfig — controls snapshot capture cadence
            //   GalleryConfig      — controls where/how paintings appear on terrain

            struct PhotographerCaptureConfig {
                // Trigger: how far the pawn walks between capture events
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
                // Per-archetype gallery probability
                //   mountainous (0): very rare
                //   varied (1):      rare — checkerboard patterns, not exhibition space
                //   basin (2):       moderate — smooth sand, natural gallery ground
                static constexpr float GALLERY_CHANCE_BY_ARCHETYPE[4] = { 0.03f, 0.06f, 0.30f, 0.40f };
                static constexpr float MOOD_MULTIPLIER[MOOD_COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };

                // Painting count per gallery: gaussian, median 5, σ 2
                // Max varies by archetype — basin gets the largest galleries
                static constexpr float PAINTINGS_MEAN = 5.0f;
                static constexpr float PAINTINGS_SIGMA = 2.0f;
                static constexpr uint32_t PAINTINGS_MIN = 2;
                static constexpr uint32_t PAINTINGS_MAX_BY_ARCHETYPE[4] = { 8, 10, 12, 12 };

                // Layout: paintings share a facing direction, staggered in two rows.
                // Odd paintings step forward, even step back — the pawn walks between.
                static constexpr float ROW_SPACING = 18.0f;       // horizontal distance between paintings (was 10)
                static constexpr float ROW_DEPTH_MIN = 8.0f;      // minimum depth gap between rows (was 12)
                static constexpr float ROW_DEPTH_RANGE = 4.0f;    // depth jitter on top of minimum (was 6)
                static constexpr float ROW_LATERAL_JITTER = 2.0f;

                // Gallery mode: two options, no mixing
                //   MONO: all paintings from one tier (Portrait, Panoramic, or Cinematic)
                //   CHRONOLOGICAL: paintings in capture order, any tier
                static constexpr float MONO_TIER_CHANCE = 0.40f;  // 40% mono, 60% chronological

                // Per-gallery canvas size: the gallery rolls a size mean, then
                // each painting jitters around that mean.
                //   Gallery mean: uniform in [GALLERY_SIZE_LO, GALLERY_SIZE_HI]
                //   Per-painting jitter: gaussian σ = PAINTING_SIZE_SIGMA
                static constexpr float GALLERY_SIZE_LO = 0.85f;  // smallest gallery mean
                static constexpr float GALLERY_SIZE_HI = 3.0f;   // largest gallery mean
                static constexpr float PAINTING_SIZE_SIGMA = 0.3f;   // per-painting jitter around gallery mean

                // Minimum snapshots before galleries start appearing
                static constexpr uint32_t MIN_POOL_SIZE = 3;

                // Minimum distance between gallery centers (world units)
                static constexpr float MIN_GALLERY_DISTANCE = 150.0f;

                // ─── Content×Form Mixing ─────────────────────────────────
                //
                // Each site rolls a three-way type: pure-snapshot, pure-authored, or mixed.
                // In mixed mode, each painting independently rolls its content source.
                //
                // Outdoor (select_gallery_for_patch → commit_gallery):
                //   70% snapshot-only terrain quads
                //   20% mixed (each painting rolls independently)
                //   10% authored-only wall frames (monuments in the desert)
                //
                // Indoor (place_wall_paintings):
                //   40% snapshot-only wall frames
                //   20% mixed (each painting rolls independently)
                //   40% authored-only wall frames
                //
                static constexpr float OUTDOOR_SNAPSHOT_ONLY = 0.80f;  // [0.00, 0.80)
                static constexpr float OUTDOOR_MIXED = 0.05f;  // [0.80, 0.85)
                // remainder 0.15 = authored-only                       // [0.85, 1.00)

                static constexpr float INDOOR_SNAPSHOT_ONLY = 0.15f;  // [0.00, 0.15)
                static constexpr float INDOOR_MIXED = 0.05f;  // [0.15, 0.20)
                // remainder 0.80 = authored-only                       // [0.20, 1.00)

                // In mixed mode: per-painting chance of being the minority content
                static constexpr float OUTDOOR_MIX_AUTHORED_CHANCE = 0.35f;  // chance each outdoor painting is authored
                static constexpr float INDOOR_MIX_SNAPSHOT_CHANCE = 0.40f;  // chance each indoor painting is snapshot

                // Photographer pacing by archetype
                static constexpr float PHOTO_PACE_BY_ARCHETYPE[4] = { 0.7f, 0.8f, 1.5f, 1.5f };

                // Gallery center jitter (fraction of PATCH_EXTENT)
                static constexpr float POSITION_JITTER = 0.30f;
            };

            // Site content type (outdoor gallery)
            struct GallerySiteType {
                static constexpr uint32_t SNAPSHOT_ONLY = 0;
                static constexpr uint32_t MIXED = 1;
                static constexpr uint32_t AUTHORED_ONLY = 2;
            };

            struct PhotographerState {
                float cumulative_distance = 0.0f;
                float next_threshold = PhotographerCaptureConfig::TRIGGER_DISTANCE_MEAN;
                uint32_t pending_shots = 0;
                float prev_pawn_x = 0.0f;
                float prev_pawn_z = 0.0f;
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
                    float roll = uniform(0.0f, 1.0f);
                    float cumul = 0.0f;
                    for (uint32_t t = 0; t < static_cast<uint32_t>(ShotType::COUNT); t++) {
                        cumul += SHOT_PARAMS[t].weight;
                        if (roll < cumul) return static_cast<ShotType>(t);
                    }
                    return static_cast<ShotType>(static_cast<uint32_t>(ShotType::COUNT) - 1);
                }
            } photographer_;

            // ─── Snapshot Staging (circular buffer, 16 layers) ───────────
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
            SnapshotStagingRecord snapshotStaging_[Dim::STAGING_LAYERS]{};
            uint32_t snapshotWriteCursor_ = 0;
            uint32_t snapshotCount_ = 0;
            float totalWalkDistance_ = 0.0f;
            uint32_t frameCounter_ = 0;

            // ─── Authored Staging (circular buffer, 16 layers) ───────────
            struct AuthoredStagingRecord {
                uint32_t disk_index = UINT32_MAX;
                float aspect_ratio = 1.0f;
                float uv_scale_x = 1.0f;
                float uv_scale_y = 1.0f;
                bool valid = false;
                bool consumed = false;
            };
            AuthoredStagingRecord authoredStaging_[Dim::STAGING_LAYERS]{};
            uint32_t authoredWriteCursor_ = 0;
            uint32_t authoredDiskCursor_ = 0;     // walks through authoredDiskManifest_
            uint32_t authoredStagedCount_ = 0;
            bool authoredTexturesLoaded_ = false;
            std::vector<std::string> authoredDiskManifest_;  // scanned at startup, sorted alphabetically

            // ─── Exhibition (32 layers, stable until portal) ─────────────
            bool exhibitionOccupied_[Dim::EXHIBITION_LAYERS]{};
            uint32_t exhibitionCount_ = 0;

            uint32_t find_free_exhibition_layer() const {
                for (uint32_t i = 0; i < Dim::EXHIBITION_LAYERS; i++)
                    if (!exhibitionOccupied_[i]) return i;
                return UINT32_MAX;
            }

            // Pending texture promotions (staging → exhibition, executed in render)
            struct PendingPromotion {
                bool is_snapshot;       // true = snapshot staging, false = authored staging
                uint32_t staging_layer;
                uint32_t exhibition_layer;
            };
            static constexpr uint32_t MAX_PROMOTIONS_PER_FRAME = 32;
            PendingPromotion pendingPromotions_[MAX_PROMOTIONS_PER_FRAME]{};
            uint32_t pendingPromotionCount_ = 0;

            void queue_promotion(bool is_snapshot, uint32_t staging_layer, uint32_t exhibition_layer) {
                if (pendingPromotionCount_ < MAX_PROMOTIONS_PER_FRAME) {
                    pendingPromotions_[pendingPromotionCount_++] = {
                        is_snapshot, staging_layer, exhibition_layer
                    };
                }
            }

            // ─── Painting Slots (exhibited paintings) ────────────────────
            GPUPaintingSlot paintingSlots_[Dim::PAINTING_MAX_SLOTS]{};
            uint32_t activePaintingCount_ = 0;
            uint32_t wallFrameCount_ = 0;

            // Active gallery centers (for minimum distance enforcement)
            struct GalleryCenter {
                float x = 0.0f, z = 0.0f;
                int32_t patch_gx = INT32_MAX, patch_gz = INT32_MAX;
                int32_t host_gx = 0, host_gz = 0;   // host patch (for entity_refs eviction)
                bool active = false;
            };
            static constexpr uint32_t MAX_GALLERIES = 48;
            GalleryCenter galleryCenters_[MAX_GALLERIES]{};

            struct PendingSnapshot {
                bool active = false;
                uint32_t target_slot = 0;
                uint32_t target_layer = 0;
            } pendingSnapshot_;

            // ─── Slot Management ─────────────────────────────────────────────

            uint32_t find_free_painting_slot() const {
                for (uint32_t i = 0; i < Dim::PAINTING_MAX_SLOTS; i++)
                    if (paintingSlots_[i].is_active == 0) return i;
                return UINT32_MAX;
            }

            // ─── Photographer: Capture Only (no placement) ───────────────────
            //
            // Captures snapshots into a circular buffer of texture layers.
            // Never places paintings. Gallery sites consume the pool.

            void update_photographer(wgpu::Queue& queue) {
                float px = pawnReadback_x_;
                float pz = pawnReadback_z_;

                if (!photographer_.initialized) {
                    photographer_.prev_pawn_x = px;
                    photographer_.prev_pawn_z = pz;
                    photographer_.initialized = true;
                    return;
                }

                float dx = px - photographer_.prev_pawn_x;
                float dz = pz - photographer_.prev_pawn_z;
                float step = std::sqrt(dx * dx + dz * dz);
                photographer_.prev_pawn_x = px;
                photographer_.prev_pawn_z = pz;
                if (step > 5.0f) return;

                photographer_.cumulative_distance += step;
                totalWalkDistance_ += step;
                frameCounter_++;
                if (photographer_.frame_cooldown > 0) photographer_.frame_cooldown--;

                if (photographer_.pending_shots > 0 && photographer_.frame_cooldown == 0) {
                    capture_snapshot(px, pz, queue);
                    photographer_.pending_shots--;
                    photographer_.frame_cooldown = PhotographerCaptureConfig::BURST_COOLDOWN_FRAMES;
                    return;
                }

                if (photographer_.cumulative_distance >= photographer_.next_threshold) {
                    photographer_.cumulative_distance = 0.0f;

                    // Pace modulation: less active in sand/basin, more in colored terrain
                    float pace = 1.0f;
                    int32_t tx = (int32_t)std::floor(px / PATCH_EXTENT);
                    int32_t tz = (int32_t)std::floor(pz / PATCH_EXTENT);
                    auto it = tileCache_.find({ tx, tz });
                    if (it != tileCache_.end()) {
                        pace = GalleryConfig::PHOTO_PACE_BY_ARCHETYPE[it->second.archetype];
                    }

                    photographer_.next_threshold = std::max(
                        PhotographerCaptureConfig::TRIGGER_DISTANCE_FLOOR,
                        photographer_.gaussian(
                            PhotographerCaptureConfig::TRIGGER_DISTANCE_MEAN * pace,
                            PhotographerCaptureConfig::TRIGGER_DISTANCE_SIGMA));
                    photographer_.pending_shots = photographer_.sample_shot_count();
                    photographer_.frame_cooldown = 0;
                }
            }

            void capture_snapshot(float pawn_x, float pawn_z, wgpu::Queue& queue) {
                ShotType shot = photographer_.sample_shot_type();
                const auto& params = SHOT_PARAMS[static_cast<uint32_t>(shot)];

                float aspect_ratio = photographer_.uniform(params.aspect_lo, params.aspect_hi);
                float azimuth = photographer_.uniform(0.0f, 6.283185f);
                float dist = std::max(PhotographerCaptureConfig::DISTANCE_FLOOR,
                    photographer_.gaussian(params.distance_mean, params.distance_sigma));
                float elev = std::max(PhotographerCaptureConfig::ELEVATION_FLOOR,
                    photographer_.gaussian(params.elevation_mean, params.elevation_sigma));
                float fov_deg = std::max(PhotographerCaptureConfig::FOV_FLOOR,
                    photographer_.gaussian(params.fov_degrees, params.fov_sigma));

                if (photographer_.uniform(0.0f, 1.0f) < PhotographerCaptureConfig::WIDE_LENS_CHANCE) {
                    fov_deg = photographer_.uniform(PhotographerCaptureConfig::WIDE_LENS_FOV_LO,
                        PhotographerCaptureConfig::WIDE_LENS_FOV_HI);
                }

                float fov_rad = fov_deg * 3.14159f / 180.0f;
                float offset_x = photographer_.uniform(-params.offset_x_range, params.offset_x_range);
                float offset_y = photographer_.uniform(-params.offset_y_range, params.offset_y_range);

                // Record in snapshot staging — cursor wraps freely, unconditional overwrite
                uint32_t layer = snapshotWriteCursor_;
                auto& rec = snapshotStaging_[layer];
                rec.aspect_ratio = aspect_ratio;
                rec.shot_type = static_cast<uint32_t>(shot);
                rec.valid = true;
                rec.consumed = false;  // fresh capture, available for exhibition
                rec.capture_x = pawn_x;
                rec.capture_z = pawn_z;
                rec.capture_distance = totalWalkDistance_;
                rec.capture_frame = frameCounter_;
                snapshotWriteCursor_ = (layer + 1) % Dim::STAGING_LAYERS;
                if (snapshotCount_ < Dim::STAGING_LAYERS) snapshotCount_++;

                // Upload photographer config for GPU compute
                GPUPhotographerConfig cfg{};
                float slen = std::sqrt(sunDirection_[0] * sunDirection_[0] + sunDirection_[1] * sunDirection_[1] + sunDirection_[2] * sunDirection_[2]);
                cfg.sun_direction[0] = sunDirection_[0] / slen;
                cfg.sun_direction[1] = sunDirection_[1] / slen;
                cfg.sun_direction[2] = sunDirection_[2] / slen;
                cfg.azimuth = azimuth;
                cfg.elevation = elev;
                cfg.distance = dist;
                cfg.fov_rad = fov_rad;
                cfg.aspect_ratio = aspect_ratio;
                cfg.patch_count = allPatchCount_;
                cfg.frame_offset_x = offset_x;
                cfg.frame_offset_y = offset_y;
                cfg._pad0 = 0.0f;
                gpuState_.upload_photographer_config(queue, cfg);

                pendingSnapshot_.active = true;
                pendingSnapshot_.target_slot = UINT32_MAX;
                pendingSnapshot_.target_layer = layer;

                const char* shot_names[] = {
                    "Panoramic", "Environmental", "Medium", "Close-up",
                    "Portrait", "Bird's Eye", "Low Angle", "Cinematic"
                };
                std::cout << "[Photographer] Capture -> layer " << layer
                    << " (" << shot_names[static_cast<uint32_t>(shot)] << ")"
                    << " aspect=" << aspect_ratio
                    << " pool=" << snapshotCount_ << "/" << Dim::STAGING_LAYERS << "\n";
            }

            // ─── Gallery Sites: Paintings Spawned When Patches Stream In ─────
            //
            // Each patch's seed determines if it hosts a gallery, how many
            // paintings, and where. Paintings draw from the snapshot pool.
            // The user discovers photos from other places in distant galleries.

            // ─── select_gallery_for_patch ────────────────────────────────
            //
            // Phase 1: content gate, spawn roll, center jitter, parameter
            // sampling. No GPU writes. No content availability validation
            // (deferred to commit where queue is available).

            bool select_gallery_for_patch(int32_t gx, int32_t gz, GallerySelection& sel) {
                // Content gate: minimum snapshot pool
                if (snapshotCount_ < GalleryConfig::MIN_POOL_SIZE) return false;

                // Mood gate
                float adj_mod = GalleryConfig::MOOD_MULTIPLIER[activeMood_];
                if (adj_mod <= 0.0f) return false;

                // Density + theme modifiers
                adj_mod *= GLOBAL_ENTITY_DENSITY;
                adj_mod *= population_type_affinity(PopFamily::GALLERY);
                uint32_t archetype = 1;
                {
                    auto dit = tileCache_.find({ gx, gz });
                    if (dit != tileCache_.end()) {
                        adj_mod *= dit->second.entity_density;
                        adj_mod *= dit->second.theme_spawn[PopFamily::GALLERY];
                        archetype = dit->second.archetype;
                    }
                }

                // Idempotency: skip if paintings already exist at this patch
                for (uint32_t i = 0; i < Dim::PAINTING_MAX_SLOTS; i++) {
                    if (paintingSlots_[i].is_active != 0 &&
                        paintingSlots_[i].patch_gx == gx && paintingSlots_[i].patch_gz == gz) {
                        return false;
                    }
                }

                // Also check if a gallery center is already active for this patch
                for (uint32_t g = 0; g < MAX_GALLERIES; g++) {
                    if (galleryCenters_[g].active &&
                        galleryCenters_[g].patch_gx == gx && galleryCenters_[g].patch_gz == gz)
                        return false;
                }

                // Spawn roll
                uint32_t seed = tile_seed(activeSeed_, gx, gz);
                float gallery_roll = cpu_hash_f(seed, 500u);
                float gallery_chance = GalleryConfig::GALLERY_CHANCE_BY_ARCHETYPE[archetype] * adj_mod;
                if (gallery_roll >= gallery_chance) return false;

                // Gallery center (jittered within patch)
                float patch_cx = (gx + 0.5f) * PATCH_EXTENT;
                float patch_cz = (gz + 0.5f) * PATCH_EXTENT;
                float center_offset = cpu_hash_f(seed, 505u) * PATCH_EXTENT * GalleryConfig::POSITION_JITTER;
                float center_angle = cpu_hash_f(seed, 506u) * 6.283185f;
                float gallery_cx = patch_cx + std::cos(center_angle) * center_offset;
                float gallery_cz = patch_cz + std::sin(center_angle) * center_offset;

                // Gallery-to-gallery distance check (belt + suspenders; MIN_SEPARATION handles most)
                float min_dist_sq = GalleryConfig::MIN_GALLERY_DISTANCE * GalleryConfig::MIN_GALLERY_DISTANCE;
                for (uint32_t g = 0; g < MAX_GALLERIES; g++) {
                    if (!galleryCenters_[g].active) continue;
                    float dx = gallery_cx - galleryCenters_[g].x;
                    float dz = gallery_cz - galleryCenters_[g].z;
                    if (dx * dx + dz * dz < min_dist_sq) return false;
                }

                // Find free center slot
                uint32_t gallery_slot = UINT32_MAX;
                for (uint32_t g = 0; g < MAX_GALLERIES; g++) {
                    if (!galleryCenters_[g].active) { gallery_slot = g; break; }
                }
                if (gallery_slot == UINT32_MAX) return false;

                // Reserve slot
                galleryCenters_[gallery_slot].active = true;
                galleryCenters_[gallery_slot].patch_gx = gx;
                galleryCenters_[gallery_slot].patch_gz = gz;

                // Footprint radius: gallery spread
                float footprint_r = (float)GalleryConfig::PAINTINGS_MAX_BY_ARCHETYPE[archetype]
                    * 0.5f * GalleryConfig::ROW_SPACING + 15.0f;

                // Painting count (seed-derived; capped to content availability in commit)
                float count_raw = GalleryConfig::PAINTINGS_MEAN
                    + (cpu_hash_f(seed, 501u) + cpu_hash_f(seed, 502u)
                        + cpu_hash_f(seed, 503u) - 1.5f) * GalleryConfig::PAINTINGS_SIGMA;
                uint32_t painting_count = (uint32_t)std::max(
                    (float)GalleryConfig::PAINTINGS_MIN,
                    std::min((float)GalleryConfig::PAINTINGS_MAX_BY_ARCHETYPE[archetype],
                        std::round(count_raw)));

                // Facing + size
                float facing_angle = cpu_hash_f(seed, 504u) * 6.283185f;
                float gallery_size_mean = GalleryConfig::GALLERY_SIZE_LO
                    + cpu_hash_f(seed, 530u) * (GalleryConfig::GALLERY_SIZE_HI - GalleryConfig::GALLERY_SIZE_LO);

                // Site type (seed-derived; content availability validated in commit)
                float site_roll = cpu_hash_f(seed, 540u);
                uint32_t site_type;
                if (site_roll < GalleryConfig::OUTDOOR_SNAPSHOT_ONLY) {
                    site_type = GallerySiteType::SNAPSHOT_ONLY;
                } else if (site_roll < GalleryConfig::OUTDOOR_SNAPSHOT_ONLY + GalleryConfig::OUTDOOR_MIXED) {
                    site_type = GallerySiteType::MIXED;
                } else {
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

            // ─── place_gallery_from_selection ────────────────────────────
            //
            // Phase 2: footprint check + registration. Gallery center is
            // seed-determined (no negotiation), but standard check_position
            // enforces MIN_SEPARATION against all families.

            bool place_gallery_from_selection(const GallerySelection& sel, GalleryPlacement& plan) {
                if (!check_position(sel.cx, sel.cz, sel.footprint_r, PopFamily::GALLERY))
                    return false;

                int32_t host_gx = (int32_t)std::floor(sel.cx / PATCH_EXTENT);
                int32_t host_gz = (int32_t)std::floor(sel.cz / PATCH_EXTENT);

                if (register_footprint(sel.cx, sel.cz, sel.footprint_r,
                    host_gx, host_gz, PopFamily::GALLERY, sel.archetype) == UINT32_MAX)
                    return false;

                plan = GalleryPlacement{};
                plan.slot = sel.slot;
                plan.trigger_gx = sel.trigger_gx;
                plan.trigger_gz = sel.trigger_gz;
                plan.host_gx = host_gx;
                plan.host_gz = host_gz;
                plan.tier_idx = sel.archetype;
                plan.cx = sel.cx;
                plan.cz = sel.cz;
                plan.footprint_r = sel.footprint_r;
                plan.archetype = sel.archetype;
                plan.painting_count = sel.painting_count;
                plan.facing_angle = sel.facing_angle;
                plan.gallery_size_mean = sel.gallery_size_mean;
                plan.site_type = sel.site_type;

                record_placement_bookkeeping(PopFamily::GALLERY, plan.tier_idx);
                return true;
            }

            // ─── commit_gallery ──────────────────────────────────────────
            //
            // Phase 3: content curation, painting layout, slot allocation,
            // GPU upload. All content-dependent decisions happen here where
            // queue is available for authored texture loading.

            void commit_gallery(const GalleryPlacement& plan,
                int32_t trigger_gx, int32_t trigger_gz, wgpu::Queue& queue)
            {
                uint32_t seed = plan.trigger_gx != INT32_MAX
                    ? tile_seed(activeSeed_, plan.trigger_gx, plan.trigger_gz) : 0u;
                int32_t gx = plan.trigger_gx, gz = plan.trigger_gz;
                float gallery_cx = plan.cx, gallery_cz = plan.cz;
                uint32_t archetype = plan.archetype;
                float gallery_size_mean = plan.gallery_size_mean;
                float facing_angle = plan.facing_angle;

                // Resolve site type with content availability
                uint32_t site_type = plan.site_type;
                if (site_type != GallerySiteType::SNAPSHOT_ONLY && !authoredTexturesLoaded_) {
                    load_authored_textures(queue);
                }
                if (site_type != GallerySiteType::SNAPSHOT_ONLY && !authoredTexturesLoaded_) {
                    site_type = GallerySiteType::SNAPSHOT_ONLY;
                }

                // Snapshot candidates
                struct Candidate { uint32_t layer; };
                Candidate candidates[Dim::STAGING_LAYERS];
                uint32_t candidate_count = 0;
                for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) {
                    if (snapshotStaging_[i].valid && !snapshotStaging_[i].consumed)
                        candidates[candidate_count++] = { i };
                }

                bool have_snapshots = candidate_count > 0;
                bool have_authored = authoredStagedCount_ > 0;
                if (site_type == GallerySiteType::SNAPSHOT_ONLY && !have_snapshots) {
                    galleryCenters_[plan.slot].active = false;
                    return;
                }
                if (site_type == GallerySiteType::AUTHORED_ONLY && !have_authored) {
                    galleryCenters_[plan.slot].active = false;
                    return;
                }
                if (site_type == GallerySiteType::MIXED && !have_snapshots && !have_authored) {
                    galleryCenters_[plan.slot].active = false;
                    return;
                }

                // Snapshot curation (mono tier + chronological sort)
                if (have_snapshots) {
                    bool mono_tier = cpu_hash_f(seed, 520u) < GalleryConfig::MONO_TIER_CHANCE;
                    if (mono_tier) {
                        static constexpr uint32_t FAVORITE_TIERS[] = {
                            (uint32_t)ShotType::PANORAMIC,
                            (uint32_t)ShotType::PORTRAIT,
                            (uint32_t)ShotType::CINEMATIC
                        };
                        uint32_t chosen = FAVORITE_TIERS[cpu_hash(seed, 521u) % 3];
                        uint32_t write = 0;
                        for (uint32_t c = 0; c < candidate_count; c++) {
                            if (snapshotStaging_[candidates[c].layer].shot_type == chosen)
                                candidates[write++] = candidates[c];
                        }
                        candidate_count = write;
                        have_snapshots = candidate_count > 0;
                    }
                    for (uint32_t i = 0; i < candidate_count; i++) {
                        for (uint32_t j = i + 1; j < candidate_count; j++) {
                            if (snapshotStaging_[candidates[j].layer].capture_frame
                                < snapshotStaging_[candidates[i].layer].capture_frame) {
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
                uint32_t max_available = candidate_count + authoredStagedCount_;
                if (site_type == GallerySiteType::SNAPSHOT_ONLY) max_available = candidate_count;
                if (site_type == GallerySiteType::AUTHORED_ONLY) max_available = authoredStagedCount_;
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
                    uint32_t slot = find_free_painting_slot();
                    if (slot == UINT32_MAX) break;

                    uint32_t p_seed = cpu_hash(seed, 510u + p * 7u);

                    float t = row_start + (float)p * GalleryConfig::ROW_SPACING;
                    float lateral_jitter = (cpu_hash_f(p_seed, 0u) - 0.5f) * 2.0f
                        * GalleryConfig::ROW_LATERAL_JITTER;
                    float depth_offset = GalleryConfig::ROW_DEPTH_MIN
                        + cpu_hash_f(p_seed, 1u) * GalleryConfig::ROW_DEPTH_RANGE;
                    if (p % 2 == 1) depth_offset = -depth_offset;

                    float paint_x = gallery_cx + row_x * (t + lateral_jitter) + face_x * depth_offset;
                    float paint_z = gallery_cz + row_z * (t + lateral_jitter) + face_z * depth_offset;

                    bool use_authored = (site_type == GallerySiteType::AUTHORED_ONLY)
                        || (site_type == GallerySiteType::MIXED
                            && cpu_hash_f(p_seed, 8u) < GalleryConfig::OUTDOOR_MIX_AUTHORED_CHANCE);

                    if (use_authored && count_unused_authored(usedAuthored) == 0) {
                        use_authored = false;
                    }
                    if (!use_authored && snap_cursor >= candidate_count) {
                        if (count_unused_authored(usedAuthored) > 0) {
                            use_authored = true;
                        } else {
                            break;
                        }
                    }

                    auto& s = paintingSlots_[slot];
                    bool placed_this = false;

                    if (use_authored) {
                        uint32_t auth_stg = pick_authored_staging(p_seed, 9u);
                        if (auth_stg == UINT32_MAX || usedAuthored[auth_stg]) {
                            uint32_t best = UINT32_MAX, best_disk = UINT32_MAX;
                            for (uint32_t a = 0; a < Dim::STAGING_LAYERS; a++) {
                                if (!usedAuthored[a] && authoredStaging_[a].valid && !authoredStaging_[a].consumed
                                    && authoredStaging_[a].disk_index < best_disk) {
                                    best_disk = authoredStaging_[a].disk_index;
                                    best = a;
                                }
                            }
                            if (best == UINT32_MAX) { use_authored = false; }
                            else { auth_stg = best; }
                        }

                        if (use_authored) {
                            uint32_t exh = find_free_exhibition_layer();
                            if (exh == UINT32_MAX) break;

                            usedAuthored[auth_stg] = true;
                            const auto& img = authoredStaging_[auth_stg];
                            float jitter = (cpu_hash_f(p_seed, 3u) + cpu_hash_f(p_seed, 5u)
                                + cpu_hash_f(p_seed, 6u) - 1.5f) * GalleryConfig::PAINTING_SIZE_SIGMA;
                            float height = std::max(2.0f, (5.0f + jitter) * gallery_size_mean);

                            fill_slot_wall_frame(s,
                                paint_x, 0.0f, paint_z,
                                face_x, 0.0f, face_z,
                                img.aspect_ratio, height,
                                exh, ContentSource::AUTHORED,
                                img.uv_scale_x, img.uv_scale_y,
                                FRAME_AUTHORED, gx, gz);
                            s.geometry_seed = cpu_hash_f(p_seed, 4u);

                            exhibitionOccupied_[exh] = true;
                            exhibitionCount_++;
                            authoredStaging_[auth_stg].consumed = true;
                            queue_promotion(false, auth_stg, exh);
                            wallFrameCount_++;
                            placed_this = true;
                        }
                    }

                    if (!placed_this && snap_cursor < candidate_count) {
                        uint32_t staging_layer = candidates[snap_cursor].layer;
                        const auto& snap = snapshotStaging_[staging_layer];
                        snap_cursor++;

                        uint32_t exh = find_free_exhibition_layer();
                        if (exh == UINT32_MAX) break;

                        uint32_t shot_idx = snap.shot_type;
                        float jitter = (cpu_hash_f(p_seed, 3u) + cpu_hash_f(p_seed, 5u)
                            + cpu_hash_f(p_seed, 6u) - 1.5f) * GalleryConfig::PAINTING_SIZE_SIGMA;
                        float size_mult = std::max(0.5f, gallery_size_mean + jitter);
                        float area = PAINTING_AREA[shot_idx] * size_mult;
                        float scale_x = std::sqrt(area * snap.aspect_ratio);
                        float scale_y = scale_x / snap.aspect_ratio;

                        s = {};
                        s.position[0] = paint_x;
                        s.position[1] = 0.0f;
                        s.position[2] = paint_z;
                        s.geometry_seed = cpu_hash_f(p_seed, 4u);
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

                        exhibitionOccupied_[exh] = true;
                        exhibitionCount_++;
                        snapshotStaging_[staging_layer].consumed = true;
                        queue_promotion(true, staging_layer, exh);
                        placed_this = true;
                    }

                    if (!placed_this) break;

                    gpuState_.upload_painting_slot(queue, slot, s);
                    activePaintingCount_++;
                    placed++;
                }

                // Record gallery center
                auto& gc = galleryCenters_[plan.slot];
                gc.x = gallery_cx;
                gc.z = gallery_cz;
                gc.host_gx = plan.host_gx;
                gc.host_gz = plan.host_gz;

                if (placed == 0) {
                    gc.active = false;  // no paintings placed — release center
                } else {
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
            void evict_paintings_for_patch(int32_t gx, int32_t gz, wgpu::Queue& queue) {
                for (uint32_t i = 0; i < Dim::PAINTING_MAX_SLOTS; i++) {
                    if (paintingSlots_[i].is_active != 0 &&
                        paintingSlots_[i].patch_gx == gx && paintingSlots_[i].patch_gz == gz) {

                        // Free the exhibition layer
                        uint32_t exh = paintingSlots_[i].texture_layer;
                        if (exh < Dim::EXHIBITION_LAYERS) {
                            exhibitionOccupied_[exh] = false;
                            exhibitionCount_--;
                        }

                        if (paintingSlots_[i].form_type == FormType::WALL_FRAME) {
                            wallFrameCount_--;
                        }

                        paintingSlots_[i].is_active = 0;
                        gpuState_.deactivate_painting_slot(queue, i);
                        activePaintingCount_--;
                    }
                }
                // Evict gallery center for this patch — NO: gallery centers persist
                // beyond patch lifetime to prevent overlap when patches re-stream.
                // Centers are evicted by distance sweep in stream_patches instead.
            }

            // ─── Snapshot Render + Y Correction ──────────────────────────────

            // (pendingYCorrection_ removed — placement correction now runs every frame)

            void render_snapshot_pass(wgpu::CommandEncoder& encoder) {
                if (!pendingSnapshot_.active) return;

                // Snapshot needs its own VP compute (camera position + VP matrix).
                // Entity Y-correction already ran in dispatch_placement_correction
                // (separate pipeline, unconditional every frame).
                // This dispatch only builds the photographer camera VP + light VP.
                {
                    wgpu::ComputePassDescriptor cpd{};
                    cpd.label = "Photographer VP Compute";
                    wgpu::ComputePassEncoder compute = encoder.BeginComputePass(&cpd);
                    renderer_.dispatch_compute_photographer_vp(
                        compute, gpuState_.photographer_compute_group()
                    );
                    compute.End();
                }

                // Only render the snapshot if a capture is pending
                if (!pendingSnapshot_.active) return;
                pendingSnapshot_.active = false;
                uint32_t layer = pendingSnapshot_.target_layer;
                std::cout << "[Photographer] Rendering snapshot -> layer " << layer << "\n";

                wgpu::RenderPassColorAttachment colorAtt{};
                colorAtt.view = gpuState_.offscreen_color_view();
                colorAtt.loadOp = wgpu::LoadOp::Clear;
                colorAtt.storeOp = wgpu::StoreOp::Store;
                colorAtt.clearValue = { (double)clearColor_[0], (double)clearColor_[1], (double)clearColor_[2], 1.0 };

                wgpu::RenderPassDepthStencilAttachment depthAtt{};
                depthAtt.view = gpuState_.offscreen_depth_view();
                depthAtt.depthLoadOp = wgpu::LoadOp::Clear;
                depthAtt.depthStoreOp = wgpu::StoreOp::Store;
                depthAtt.depthClearValue = 1.0f;

                wgpu::RenderPassDescriptor desc{};
                desc.label = "Photographer Snapshot";
                desc.colorAttachmentCount = 1;
                desc.colorAttachments = &colorAtt;
                desc.depthStencilAttachment = &depthAtt;

                wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&desc);

                renderer_.draw_patch_terrain_direct(pass,
                    gpuState_.photographer_render_entity_group(),
                    gpuState_.render_texture_group(),
                    gpuState_.patch_index_buffer(),
                    gpuState_.patch_index_count(),
                    renderPatchCount_);

                renderer_.draw_pawn(pass,
                    gpuState_.photographer_render_entity_group(),
                    gpuState_.render_texture_group(),
                    GPUState::pawn_vertex_count());

                renderer_.draw_sphere(pass,
                    gpuState_.photographer_render_entity_group(),
                    gpuState_.render_texture_group(),
                    gpuState_.sphere_vertex_buffer(),
                    gpuState_.sphere_index_buffer(),
                    gpuState_.sphere_index_count());

                if (renderedRibbonSlot_ != UINT32_MAX) {
                    renderer_.draw_ribbon(pass,
                        gpuState_.photographer_render_entity_group(),
                        gpuState_.render_texture_group(),
                        GPUState::ribbon_vertex_count());
                }

                renderer_.draw_arch(pass,
                    gpuState_.photographer_render_entity_group(),
                    gpuState_.render_texture_group(),
                    gpuState_.arch_vertex_buffer(),
                    gpuState_.arch_index_buffer(),
                    gpuState_.arch_index_count());

                renderer_.draw_column(pass,
                    gpuState_.photographer_render_entity_group(),
                    gpuState_.render_texture_group(),
                    gpuState_.column_vertex_buffer(),
                    gpuState_.column_index_buffer(),
                    gpuState_.column_index_count());

                renderer_.draw_shell(pass,
                    gpuState_.photographer_render_entity_group(),
                    gpuState_.render_texture_group(),
                    gpuState_.shell_vertex_buffer(),
                    gpuState_.shell_index_buffer(),
                    gpuState_.shell_index_count());

                // Pyramids: terrain surface IS the pyramid shape (via the baked
                // heightfield, which caches POLICY_BAKED_HEIGHTFIELD = static
                // base + pyramids). No separate mesh draw needed.

                pass.End();

                wgpu::TexelCopyTextureInfo src{};
                src.texture = gpuState_.offscreen_color_texture();
                src.mipLevel = 0;
                src.origin = { 0, 0, 0 };

                wgpu::TexelCopyTextureInfo dst{};
                dst.texture = gpuState_.snapshot_staging_texture();
                dst.mipLevel = 0;
                dst.origin = { 0, 0, layer };

                wgpu::Extent3D extent = { Dim::PAINTING_RESOLUTION, Dim::PAINTING_RESOLUTION, 1 };
                encoder.CopyTextureToTexture(&src, &dst, &extent);
            }


            // ─── Wall Paintings + Authored Image Loading ─────────────────────
            // Called by apply_mood / generate_indoor_shell for indoor worlds.

                        // ─── Authored Image Loading (staging model) ─────────────────

            void load_authored_image_to_staging(wgpu::Queue& queue, uint32_t staging_layer, uint32_t disk_index, const char* path) {
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

                gpuState_.upload_authored_painting(queue, staging_layer, padded.data(), RES, RES);
                stbi_image_free(data);

                auto& rec = authoredStaging_[staging_layer];
                rec.disk_index = disk_index;
                rec.aspect_ratio = (height > 0) ? (float)width / (float)height : 1.0f;
                rec.uv_scale_x = (float)dst_w / RES;
                rec.uv_scale_y = (float)dst_h / RES;
                rec.valid = true;
                rec.consumed = false;

                std::cout << "[Authored] Scaled → " << dst_w << "x" << dst_h
                    << " (aspect " << rec.aspect_ratio << ")\n";
            }

            // ─── Paintings Folder Scan ─────────────────────────────────
            // Scans assets/paintings/ for PAINTING_*.jpg/jpeg, sorted alphabetically.
            // Called once at first load. The full collection lives on disk;
            // a rotating 16-layer staging window loads into GPU memory.

            void scan_paintings_folder() {
                namespace fs = std::filesystem;
                authoredDiskManifest_.clear();

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
                    authoredDiskManifest_.push_back(entry.path().string());
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
                std::sort(authoredDiskManifest_.begin(), authoredDiskManifest_.end(),
                    [&](const std::string& a, const std::string& b) {
                        return extract_number(a) < extract_number(b);
                    });

                std::cout << "[Authored] Scanned " << found_dir.string()
                    << " — found " << authoredDiskManifest_.size() << " paintings\n";
            }

            void load_authored_textures(wgpu::Queue& queue) {
                if (authoredTexturesLoaded_) return;

                // Scan folder on first load
                if (authoredDiskManifest_.empty()) {
                    scan_paintings_folder();
                }
                if (authoredDiskManifest_.empty()) {
                    authoredTexturesLoaded_ = true;
                    return;
                }

                // Fill staging with the first STAGING_LAYERS images from manifest
                uint32_t manifest_size = (uint32_t)authoredDiskManifest_.size();
                uint32_t to_load = std::min(manifest_size, Dim::STAGING_LAYERS);
                for (uint32_t i = 0; i < to_load; i++) {
                    load_authored_image_to_staging(queue, i, i, authoredDiskManifest_[i].c_str());
                    if (authoredStaging_[i].valid) authoredStagedCount_++;
                }
                authoredWriteCursor_ = to_load % Dim::STAGING_LAYERS;
                authoredDiskCursor_ = to_load % manifest_size;
                authoredTexturesLoaded_ = true;
                std::cout << "[Authored] Staged " << authoredStagedCount_
                    << "/" << manifest_size << " images\n";
            }

            // Replace consumed authored staging slots with the next images from disk.
            // Called at teardown — consumed slots get fresh paintings, unconsumed survive.
            // The disk cursor walks through the entire manifest across world transitions,
            // so the pawn sees different paintings in each world.
            void rotate_authored_staging(wgpu::Queue& queue) {
                if (authoredDiskManifest_.empty()) return;
                uint32_t manifest_size = (uint32_t)authoredDiskManifest_.size();

                // Collect disk indices currently in unconsumed (surviving) slots
                // to avoid loading duplicates
                bool disk_in_use[256]{};  // generous upper bound
                for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) {
                    if (authoredStaging_[i].valid && !authoredStaging_[i].consumed) {
                        if (authoredStaging_[i].disk_index < 256)
                            disk_in_use[authoredStaging_[i].disk_index] = true;
                    }
                }

                uint32_t rotated = 0;
                for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) {
                    if (!authoredStaging_[i].consumed) continue;  // keep unconsumed

                    // Find next disk image not already in a surviving slot
                    uint32_t attempts = 0;
                    while (attempts < manifest_size) {
                        uint32_t disk_idx = authoredDiskCursor_;
                        authoredDiskCursor_ = (authoredDiskCursor_ + 1) % manifest_size;
                        if (disk_idx < 256 && disk_in_use[disk_idx]) {
                            attempts++;
                            continue;
                        }
                        // Load this image into the vacated staging slot
                        load_authored_image_to_staging(queue, i, disk_idx,
                            authoredDiskManifest_[disk_idx].c_str());
                        if (disk_idx < 256) disk_in_use[disk_idx] = true;
                        rotated++;
                        break;
                    }
                    // If all manifest images are in surviving slots (unlikely with 50+),
                    // the consumed slot just stays invalid
                }

                if (rotated > 0) {
                    // Recount valid slots after rotation
                    authoredStagedCount_ = 0;
                    for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) {
                        if (authoredStaging_[i].valid) authoredStagedCount_++;
                    }
                    std::cout << "[Authored] Rotated " << rotated
                        << " slot(s), " << authoredStagedCount_ << " valid"
                        << ", disk cursor at " << authoredDiskCursor_
                        << "/" << manifest_size << "\n";
                }
            }

            // Count how many valid authored staging entries aren't in usedAuthored[]
            uint32_t count_unused_authored(const bool usedAuthored[]) const {
                uint32_t count = 0;
                for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) {
                    if (authoredStaging_[i].valid && !authoredStaging_[i].consumed && !usedAuthored[i]) count++;
                }
                return count;
            }

            // Pick the next authored painting in alphabetical order (lowest disk_index first)
            uint32_t pick_authored_staging(uint32_t /*seed*/, uint32_t /*prop*/) {
                uint32_t best_slot = UINT32_MAX;
                uint32_t best_disk = UINT32_MAX;
                for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) {
                    if (authoredStaging_[i].valid && !authoredStaging_[i].consumed
                        && authoredStaging_[i].disk_index < best_disk) {
                        best_disk = authoredStaging_[i].disk_index;
                        best_slot = i;
                    }
                }
                return best_slot;
            }

            // ─── Frame Style Presets ─────────────────────────────────────
            struct FrameStyle {
                float depth, width, recess;
                float color[3];
            };
            // Authored: thick dark wood (museum frame)
            static constexpr FrameStyle FRAME_AUTHORED = { 0.30f, 0.45f, 0.09f, { 0.25f, 0.15f, 0.08f } };
            // Snapshot on wall: same museum frame (content is different, ceremony is the same)
            static constexpr FrameStyle FRAME_SNAPSHOT = { 0.30f, 0.45f, 0.09f, { 0.25f, 0.15f, 0.08f } };

            // ─── Slot Fill Helpers ────────────────────────────────────────

            void fill_slot_wall_frame(
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

            // ─── Wall Painting Placement (unified slots, multi-wall, mixing) ─

            void place_wall_paintings(wgpu::Queue& queue, float bmin, float bmax, float ceiling_h) {
                // Clear any existing wall paintings first (indoor→indoor transitions)
                clear_wall_paintings(queue);

                load_authored_textures(queue);

                constexpr float PAINT_Y_FRAC = 0.45f;  // center height as fraction of ceiling
                constexpr float WALL_OFFSET = 0.05f;    // distance from wall surface

                float paint_y_base = ceiling_h * PAINT_Y_FRAC;
                float wall_span = bmax - bmin;
                float wall_center = (bmin + bmax) * 0.5f;

                // Three-way site type: snapshot-only / mixed / authored-only
                uint32_t site_seed = cpu_hash(activeSeed_, 5500u);
                float site_roll = cpu_hash_f(site_seed, 0u);
                enum class IndoorSiteType { SNAPSHOT_ONLY, MIXED, AUTHORED_ONLY };
                IndoorSiteType site_type;
                if (site_roll < GalleryConfig::INDOOR_SNAPSHOT_ONLY && snapshotCount_ > 0) {
                    site_type = IndoorSiteType::SNAPSHOT_ONLY;
                }
                else if (site_roll < GalleryConfig::INDOOR_SNAPSHOT_ONLY + GalleryConfig::INDOOR_MIXED
                    && snapshotCount_ > 0) {
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

                // Roll how many walls get paintings (1–4), then shuffle to pick which
                uint32_t wall_roll = cpu_hash(site_seed, 1u) % 4;  // 0–3
                uint32_t active_wall_count = 1 + wall_roll;         // 1–4
                uint32_t active_walls[4] = { 0, 1, 2, 3 };
                // Fisher-Yates shuffle
                for (uint32_t i = 3; i > 0; i--) {
                    uint32_t j = cpu_hash(site_seed, 2u + i) % (i + 1);
                    uint32_t tmp = active_walls[i];
                    active_walls[i] = active_walls[j];
                    active_walls[j] = tmp;
                }

                // Track which authored layers have been used across all walls (no duplicates)
                bool usedAuthored[Dim::STAGING_LAYERS]{};

                // ─── Size variation: three painting scales ────────────────────
                // Intimate (small accent), Standard (default), Statement (focal point)
                // Rolled per painting via seed — gives visual hierarchy on each wall.
                struct PaintingScale {
                    float height_lo, height_hi;
                    float weight;
                };
                static constexpr PaintingScale INDOOR_SCALES[] = {
                    { 3.0f,  5.5f,  0.25f },   // intimate — small, hung higher
                    { 6.0f,  9.0f,  0.50f },   // standard — medium, eye level
                    { 10.0f, 14.0f, 0.25f },   // statement — large focal piece
                };
                static constexpr uint32_t INDOOR_SCALE_COUNT = 3;

                for (uint32_t aw = 0; aw < active_wall_count; aw++) {
                    uint32_t w = active_walls[aw];
                    const auto& wall = walls[w];
                    uint32_t w_seed = cpu_hash(site_seed, 10u + w * 20u);

                    uint32_t count = 1 + cpu_hash(w_seed, 0u) % 5;  // 1-5 per wall

                    // Keep paintings away from corners
                    constexpr float CORNER_MARGIN = 12.0f;
                    float usable_span = std::max(wall.span - 2.0f * CORNER_MARGIN, wall.span * 0.3f);
                    constexpr float PAINTING_GAP = 6.0f;  // gap between painting edges (was 3)

                    // ─── Pre-compute widths to center the group on the wall ──
                    float total_width = 0.0f;
                    float painting_widths[8]{};
                    float painting_heights[8]{};
                    uint32_t effective_count = std::min(count, 8u);

                    for (uint32_t p = 0; p < effective_count; p++) {
                        uint32_t p_seed = cpu_hash(w_seed, 100u + p * 10u);

                        // Scale selection (weighted)
                        float scale_roll = cpu_hash_f(p_seed, 7u);
                        float cumul = 0.0f;
                        uint32_t scale_idx = INDOOR_SCALE_COUNT - 1;
                        for (uint32_t si = 0; si < INDOOR_SCALE_COUNT; si++) {
                            cumul += INDOOR_SCALES[si].weight;
                            if (scale_roll < cumul) { scale_idx = si; break; }
                        }
                        float h = INDOOR_SCALES[scale_idx].height_lo
                            + cpu_hash_f(p_seed, 3u) * (INDOOR_SCALES[scale_idx].height_hi - INDOOR_SCALES[scale_idx].height_lo);
                        painting_heights[p] = h;

                        // Estimate width from typical aspect ratio (~1.3)
                        float est_aspect = 0.8f + cpu_hash_f(p_seed, 5u) * 0.8f;  // [0.8, 1.6]
                        painting_widths[p] = h * est_aspect;
                        total_width += painting_widths[p];
                        if (p > 0) total_width += PAINTING_GAP;
                    }

                    // Trim paintings that don't fit
                    while (effective_count > 1 && total_width > usable_span) {
                        total_width -= painting_widths[effective_count - 1];
                        total_width -= PAINTING_GAP;
                        effective_count--;
                    }

                    // Center the group on the wall
                    float group_start = wall_center - total_width * 0.5f;
                    float cursor = group_start;

                    for (uint32_t p = 0; p < effective_count; p++) {

                        uint32_t slot = find_free_painting_slot();
                        if (slot == UINT32_MAX) return;

                        uint32_t p_seed = cpu_hash(w_seed, 100u + p * 10u);

                        // Vertical position: scale-dependent offset from base
                        // Intimate pieces: hung higher. Statement pieces: anchored lower.
                        float scale_roll = cpu_hash_f(p_seed, 7u);
                        float cumul = 0.0f;
                        uint32_t scale_idx = INDOOR_SCALE_COUNT - 1;
                        for (uint32_t si = 0; si < INDOOR_SCALE_COUNT; si++) {
                            cumul += INDOOR_SCALES[si].weight;
                            if (scale_roll < cumul) { scale_idx = si; break; }
                        }

                        float y_offset = 0.0f;
                        if (scale_idx == 0) y_offset = 2.0f + cpu_hash_f(p_seed, 1u) * 2.0f;        // intimate: +2 to +4
                        else if (scale_idx == 1) y_offset = (cpu_hash_f(p_seed, 1u) - 0.5f) * 3.0f;  // standard: -1.5 to +1.5
                        else y_offset = -1.5f - cpu_hash_f(p_seed, 1u) * 2.0f;                        // statement: -1.5 to -3.5

                        float py = wall.py + y_offset;

                        // ─── Content decision (three-way) ────────────────
                        bool use_snapshot = (site_type == IndoorSiteType::SNAPSHOT_ONLY)
                            || (site_type == IndoorSiteType::MIXED
                                && cpu_hash_f(p_seed, 2u) < GalleryConfig::INDOOR_MIX_SNAPSHOT_CHANCE);

                        if (!use_snapshot && count_unused_authored(usedAuthored) == 0) {
                            use_snapshot = true;
                        }

                        auto& s = paintingSlots_[slot];
                        float paint_width = 0.0f;  // will be set by whichever path fills the slot

                        if (use_snapshot) {
                            uint32_t snap_stg = UINT32_MAX;
                            for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) {
                                if (snapshotStaging_[i].valid && !snapshotStaging_[i].consumed) {
                                    snap_stg = i;
                                    break;
                                }
                            }
                            if (snap_stg == UINT32_MAX) {
                                if (count_unused_authored(usedAuthored) == 0) continue;
                                use_snapshot = false;
                            }
                            else {
                                uint32_t exh = find_free_exhibition_layer();
                                if (exh == UINT32_MAX) return;

                                const auto& snap = snapshotStaging_[snap_stg];
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

                                exhibitionOccupied_[exh] = true;
                                exhibitionCount_++;
                                snapshotStaging_[snap_stg].consumed = true;
                                queue_promotion(true, snap_stg, exh);

                                cursor += paint_width + PAINTING_GAP;
                                gpuState_.upload_painting_slot(queue, slot, s);
                                wallFrameCount_++;
                                continue;
                            }
                        }

                        if (!use_snapshot) {
                            uint32_t auth_stg = pick_authored_staging(p_seed, 4u);
                            if (auth_stg == UINT32_MAX) continue;
                            if (usedAuthored[auth_stg]) {
                                uint32_t best = UINT32_MAX, best_disk = UINT32_MAX;
                                for (uint32_t a = 0; a < Dim::STAGING_LAYERS; a++) {
                                    if (!usedAuthored[a] && authoredStaging_[a].valid && !authoredStaging_[a].consumed
                                        && authoredStaging_[a].disk_index < best_disk) {
                                        best_disk = authoredStaging_[a].disk_index;
                                        best = a;
                                    }
                                }
                                if (best == UINT32_MAX) continue;
                                auth_stg = best;
                            }

                            uint32_t exh = find_free_exhibition_layer();
                            if (exh == UINT32_MAX) return;

                            usedAuthored[auth_stg] = true;

                            const auto& img = authoredStaging_[auth_stg];
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

                            exhibitionOccupied_[exh] = true;
                            exhibitionCount_++;
                            authoredStaging_[auth_stg].consumed = true;
                            queue_promotion(false, auth_stg, exh);

                            cursor += paint_width + PAINTING_GAP;
                            gpuState_.upload_painting_slot(queue, slot, s);
                            wallFrameCount_++;
                        }
                    }
                }

                const char* site_type_name = (site_type == IndoorSiteType::SNAPSHOT_ONLY) ? "SNAPSHOT"
                    : (site_type == IndoorSiteType::MIXED) ? "MIXED" : "AUTHORED";
                std::cout << "[WallPainting] Placed " << wallFrameCount_
                    << " frame(s) across " << active_wall_count << " walls"
                    << " (" << site_type_name << ")\n";
            }

            void clear_wall_paintings(wgpu::Queue& queue) {
                for (uint32_t i = 0; i < Dim::PAINTING_MAX_SLOTS; i++) {
                    if (paintingSlots_[i].is_active != 0 &&
                        paintingSlots_[i].form_type == FormType::WALL_FRAME) {
                        uint32_t exh = paintingSlots_[i].texture_layer;
                        if (exh < Dim::EXHIBITION_LAYERS) {
                            exhibitionOccupied_[exh] = false;
                            exhibitionCount_--;
                        }
                        paintingSlots_[i].is_active = 0;
                        gpuState_.deactivate_painting_slot(queue, i);
                    }
                }
                wallFrameCount_ = 0;
            }

            // ═══ END INLINED: modules/gallery.inl ═════════════════════════

            uint32_t activeSeed_ = 42;     // world master seed (mutable for world transitions)
            // Patch dimensions aliased from Dim:: for local readability
            static constexpr float    PATCH_EXTENT = Dim::PATCH_EXTENT;
            static constexpr uint32_t GRID_RADIUS = Dim::PATCH_GRID_RADIUS;   // inner priority (3 → 7×7)
            static constexpr uint32_t GRID_SIDE = Dim::PATCH_GRID_SIDE;
            static constexpr uint32_t RENDER_RADIUS = Dim::PATCH_RENDER_RADIUS;  // visible radius (5)
            static constexpr uint32_t RENDER_SIDE = Dim::PATCH_RENDER_SIDE;
            static constexpr uint32_t PREGEN_RADIUS = Dim::PATCH_PREGEN_RADIUS; // deep pre-gen buffer (7)
            static constexpr uint32_t MAX_PATCHES = Dim::MAX_ACTIVE_PATCHES;    // 225

            // Runtime render radius — toggleable within [GRID_RADIUS, RENDER_RADIUS].
            // Buffers/textures always allocated for PREGEN_RADIUS.
            // Visibility uses circular VISIBLE_RADIUS; RENDER_RADIUS retained for
            // allocation bounds and GoL zone eviction.
            uint32_t activeRadius_ = PREGEN_RADIUS;

            enum class PatchPhase : uint8_t {
                ALLOCATED,      // layer assigned, tile cached, no entities yet
                SPAWNED,        // entities selected + placed + committed
                GENERATED,      // heightfield computed, gallery + GoL spawned
                NEEDS_REGEN,    // heightfield stale (new pier in range)
            };

            struct ActivePatch {
                int32_t grid_x = 0;
                int32_t grid_z = 0;
                uint32_t layer = 0;
                bool valid = false;
                PatchPhase phase = PatchPhase::ALLOCATED;
                bool animated = false;   // true if patch overlaps an active pool

                // Entity ownership (recorded at commit, read at eviction)
                struct EntityRef {
                    uint32_t family;   // PopFamily index
                    uint32_t slot;     // index into Active* array
                };
                static constexpr uint32_t MAX_ENTITY_REFS = 10;
                EntityRef entity_refs[MAX_ENTITY_REFS]{};
                uint32_t entity_ref_count = 0;

                void record_entity(uint32_t family, uint32_t slot) {
                    if (entity_ref_count < MAX_ENTITY_REFS) {
                        entity_refs[entity_ref_count++] = { family, slot };
                    }
                }

                void unregister_entity(uint32_t family, uint32_t slot) {
                    for (uint32_t i = 0; i < entity_ref_count; i++) {
                        if (entity_refs[i].family == family && entity_refs[i].slot == slot) {
                            entity_refs[i] = entity_refs[--entity_ref_count];
                            return;
                        }
                    }
                }


            };

            ActivePatch patches_[MAX_PATCHES]{};
            int32_t lastCenterX_ = INT32_MAX;  // force full regeneration on first frame
            int32_t lastCenterZ_ = INT32_MAX;
            uint32_t activePatchCount_ = 0;
            uint32_t renderPatchCount_ = 0;  // visible patches (within circular VISIBLE_RADIUS)
            uint32_t lod0PatchCount_ = 0;    // subset of rendered: within LOD_FULL_RADIUS (full mesh)
            uint32_t allPatchCount_ = 0;     // all generated patches (including pre-gen ring)
            uint32_t entitiesCulled_ = 0;    // entities hidden by distance culling this frame

            // Find the ActivePatch entry for a given grid coordinate.
            // Returns nullptr if not found (should not happen for host patches
            // within the allocation window).
            ActivePatch* find_patch(int32_t gx, int32_t gz) {
                for (uint32_t i = 0; i < activePatchCount_; i++) {
                    if (patches_[i].valid && patches_[i].grid_x == gx && patches_[i].grid_z == gz)
                        return &patches_[i];
                }
                return nullptr;
            }

            // Hook: full eviction of a single patch.
            void evict_patch(uint32_t pi, wgpu::Queue& queue) {
                free_layer(patches_[pi].layer);
                // Painting eviction now handled by dispatch_evict_gallery via entity_refs
                evict_patch_entities(patches_[pi], queue);
                unregister_footprints_for_patch(patches_[pi].grid_x, patches_[pi].grid_z);
                patches_[pi].valid = false;
            }

            void evict_patch_entities(ActivePatch& patch, wgpu::Queue& queue) {
#ifdef DIAG_ENTITY_LIFECYCLE
                if (patch.entity_ref_count > 0) {
                    float wx = (patch.grid_x + 0.5f) * PATCH_EXTENT;
                    float wz = (patch.grid_z + 0.5f) * PATCH_EXTENT;
                    float dx = wx - pawnReadback_x_, dz = wz - pawnReadback_z_;
                    std::cout << "[DIAG:EVICT] patch(" << patch.grid_x << "," << patch.grid_z
                        << ") dist=" << std::sqrt(dx * dx + dz * dz)
                        << " refs=" << patch.entity_ref_count << "\n";
                }
#endif
                for (uint32_t i = 0; i < patch.entity_ref_count; i++) {
                    auto& ref = patch.entity_refs[i];
                    FAMILY_DISPATCH[ref.family].evict_slot(this, ref.slot, queue);
                }

                int32_t gx = patch.grid_x, gz = patch.grid_z;
                for (uint32_t f = 0; f < PopFamily::COUNT; f++)

                    patch.entity_ref_count = 0;
            }

            void audit_entity_integrity() {
#ifdef DIAG_ENTITY_LIFECYCLE
                // Count actual active slots
                uint32_t act_a = 0, act_c = 0, act_n = 0, act_p = 0;
                for (uint32_t i = 0; i < Dim::MAX_ARCH_INSTANCES; i++) if (activeArches_[i].active) act_a++;
                for (uint32_t i = 0; i < Dim::MAX_COLUMN_ONLY; i++) if (activeColumns_[i].active) act_c++;
                for (uint32_t i = 0; i < Dim::MAX_ANTENNA_ONLY; i++) if (activeAntennas_[i].active) act_n++;
                for (uint32_t i = 0; i < Dim::MAX_PYRAMID_INSTANCES; i++) if (activePyramids_[i].active) act_p++;

                // Count consistency
                if (act_a != activeArchCount_)
                    std::cout << "[DIAG:AUDIT] ARCH COUNT active=" << act_a << " tracked=" << activeArchCount_ << "\n";
                if (act_c != activeColumnCount_)
                    std::cout << "[DIAG:AUDIT] COL COUNT active=" << act_c << " tracked=" << activeColumnCount_ << "\n";
                if (act_n != activeAntennaCount_)
                    std::cout << "[DIAG:AUDIT] ANT COUNT active=" << act_n << " tracked=" << activeAntennaCount_ << "\n";
                if (act_p != activePyramidCount_)
                    std::cout << "[DIAG:AUDIT] PYR COUNT active=" << act_p << " tracked=" << activePyramidCount_ << "\n";

                // Collect refs from all patches
                bool ra[Dim::MAX_ARCH_INSTANCES]{};
                bool rc[Dim::MAX_COLUMN_ONLY]{};
                bool rn[Dim::MAX_ANTENNA_ONLY]{};
                bool rp[Dim::MAX_PYRAMID_INSTANCES]{};
                for (uint32_t p = 0; p < activePatchCount_; p++) {
                    if (!patches_[p].valid) continue;
                    for (uint32_t r = 0; r < patches_[p].entity_ref_count; r++) {
                        auto& ref = patches_[p].entity_refs[r];
                        switch (ref.family) {
                        case PopFamily::PYRAMID:
                            if (ref.slot < Dim::MAX_PYRAMID_INSTANCES) {
                                if (rp[ref.slot]) std::cout << "[DIAG:AUDIT] DUP REF pyr slot=" << ref.slot << " patch=(" << patches_[p].grid_x << "," << patches_[p].grid_z << ")\n";
                                rp[ref.slot] = true;
                            } break;
                        case PopFamily::ARCH:
                            if (ref.slot < Dim::MAX_ARCH_INSTANCES) {
                                if (ra[ref.slot]) std::cout << "[DIAG:AUDIT] DUP REF arch slot=" << ref.slot << " patch=(" << patches_[p].grid_x << "," << patches_[p].grid_z << ")\n";
                                ra[ref.slot] = true;
                            } break;
                        case PopFamily::COLUMN:
                            if (ref.slot < Dim::MAX_COLUMN_ONLY) {
                                if (rc[ref.slot]) std::cout << "[DIAG:AUDIT] DUP REF col slot=" << ref.slot << " patch=(" << patches_[p].grid_x << "," << patches_[p].grid_z << ")\n";
                                rc[ref.slot] = true;
                            } break;
                        case PopFamily::ANTENNA:
                            if (ref.slot < Dim::MAX_ANTENNA_ONLY) {
                                if (rn[ref.slot]) std::cout << "[DIAG:AUDIT] DUP REF ant slot=" << ref.slot << " patch=(" << patches_[p].grid_x << "," << patches_[p].grid_z << ")\n";
                                rn[ref.slot] = true;
                            } break;
                        }
                    }
                }

                // Ghost: active but no ref (will never be evicted)
                for (uint32_t i = 0; i < Dim::MAX_ARCH_INSTANCES; i++)
                    if (activeArches_[i].active && !ra[i])
                        std::cout << "[DIAG:AUDIT] GHOST arch slot=" << i << " host=(" << activeArches_[i].host_gx << "," << activeArches_[i].host_gz << ")\n";
                for (uint32_t i = 0; i < Dim::MAX_COLUMN_ONLY; i++)
                    if (activeColumns_[i].active && !rc[i])
                        std::cout << "[DIAG:AUDIT] GHOST col slot=" << i << " host=(" << activeColumns_[i].host_gx << "," << activeColumns_[i].host_gz << ")\n";
                for (uint32_t i = 0; i < Dim::MAX_ANTENNA_ONLY; i++)
                    if (activeAntennas_[i].active && !rn[i])
                        std::cout << "[DIAG:AUDIT] GHOST ant slot=" << i << " host=(" << activeAntennas_[i].host_gx << "," << activeAntennas_[i].host_gz << ")\n";
                for (uint32_t i = 0; i < Dim::MAX_PYRAMID_INSTANCES; i++)
                    if (activePyramids_[i].active && !rp[i])
                        std::cout << "[DIAG:AUDIT] GHOST pyr slot=" << i << " host=(" << activePyramids_[i].host_gx << "," << activePyramids_[i].host_gz << ")\n";

                // Orphan: ref but not active (ref points to freed slot)
                for (uint32_t i = 0; i < Dim::MAX_ARCH_INSTANCES; i++)
                    if (!activeArches_[i].active && ra[i])
                        std::cout << "[DIAG:AUDIT] ORPHAN arch slot=" << i << "\n";
                for (uint32_t i = 0; i < Dim::MAX_COLUMN_ONLY; i++)
                    if (!activeColumns_[i].active && rc[i])
                        std::cout << "[DIAG:AUDIT] ORPHAN col slot=" << i << "\n";
                for (uint32_t i = 0; i < Dim::MAX_ANTENNA_ONLY; i++)
                    if (!activeAntennas_[i].active && rn[i])
                        std::cout << "[DIAG:AUDIT] ORPHAN ant slot=" << i << "\n";
                for (uint32_t i = 0; i < Dim::MAX_PYRAMID_INSTANCES; i++)
                    if (!activePyramids_[i].active && rp[i])
                        std::cout << "[DIAG:AUDIT] ORPHAN pyr slot=" << i << "\n";

                // Ref overflow: any patch at capacity
                for (uint32_t p = 0; p < activePatchCount_; p++) {
                    if (patches_[p].valid && patches_[p].entity_ref_count >= ActivePatch::MAX_ENTITY_REFS)
                        std::cout << "[DIAG:AUDIT] REF FULL patch=(" << patches_[p].grid_x << "," << patches_[p].grid_z << ") count=" << patches_[p].entity_ref_count << "\n";
                }
#endif
            }

            // ─── Deferred Upload Flags ───────────────────────────────────
            bool pierCountDirty_ = false;        // defer recompute_and_upload_pier_count
            bool groundEntriesDirty_ = true;     // defer upload_ground_entries (true at boot)
            bool patchInstancesDirty_ = true;    // defer LOD sort + upload_patch_instances
            bool placementDirty_ = true;         // defer dispatch_placement_correction

            // Free-list of available texture layers
            uint32_t freeLayerStack_[MAX_PATCHES]{};
            uint32_t freeLayerCount_ = MAX_PATCHES;

            // ─── Dynamic Budgets ─────────────────────────────────────────
            //
            // Entity spawning and heightfield generation are both distance-
            // driven and budgeted per frame. Spawning must complete before
            // generation (piers affect heightfields), enforced by requiring
            // phase must be SPAWNED before a patch enters the generation scan.
            static constexpr uint32_t SPAWN_BUDGET_PER_FRAME = 4;    // max patches to spawn entities for
            static constexpr uint32_t ALLOC_BUDGET_PER_FRAME = 4;    // max patches to allocate per frame
            static constexpr uint32_t EVICT_BUDGET_PER_FRAME = 4;    // max patches to evict per frame
            static constexpr uint32_t PATCH_BUDGET_MIN = 1;
            static constexpr uint32_t PATCH_BUDGET_MAX = 6;
            static constexpr uint32_t PATCH_PENDING_TIER_1 = 3;
            static constexpr uint32_t PATCH_PENDING_TIER_2 = 8;
            static constexpr uint32_t PATCH_PENDING_TIER_3 = 20;
            static constexpr uint32_t PATCH_PENDING_TIER_4 = 40;
            static constexpr uint32_t PATCH_BUDGET_MOVE_THRESHOLD = 4;

            uint32_t count_pending_patches() const {
                uint32_t n = 0;
                for (uint32_t i = 0; i < activePatchCount_; i++) {
                    if (!patches_[i].valid) continue;
                    if (patches_[i].phase == PatchPhase::SPAWNED ||
                        patches_[i].phase == PatchPhase::NEEDS_REGEN) n++;
                }
                return n;
            }

            uint32_t patches_budget_this_frame() const {
                uint32_t pending = count_pending_patches();
                uint32_t budget = PATCH_BUDGET_MIN;
                if (pending >= PATCH_PENDING_TIER_4) budget = 6;
                else if (pending >= PATCH_PENDING_TIER_3) budget = 4;
                else if (pending >= PATCH_PENDING_TIER_2) budget = 3;
                else if (pending >= PATCH_PENDING_TIER_1) budget = 2;

                bool moving = (std::abs(inputState_.move_x) > 0.01f ||
                    std::abs(inputState_.move_z) > 0.01f);
                if (moving && pending > PATCH_BUDGET_MOVE_THRESHOLD)
                    budget += 1;

                return std::min(budget, PATCH_BUDGET_MAX);
            }

            // --- Tile World System ---------------------------------------------------
            //
            // Each patch is a tile with an archetype that configures its terrain
            // character. Archetypes are rolled based on the spatial cache of
            // neighboring tiles, creating coherent regions with variety.

            // Derive finite world radius from seed within mood-defined bounds.
            static uint32_t derive_finite_radius(uint32_t seed, const MoodProfile& mood) {
                if (mood.finite_radius_min >= mood.finite_radius_max) return mood.finite_radius_min;
                uint32_t range = mood.finite_radius_max - mood.finite_radius_min + 1;
                return mood.finite_radius_min + cpu_hash(seed, 77u) % range;
            }

            // Biased mood selection for portal destinations.
            // In finite mode: 55% indoor (moods 2-3), 25% infinite outdoor (moods 0-1), 20% finite outdoor (moods 4-5).
            // In open mode: uniform across all moods.
            uint32_t pick_portal_mood(uint32_t seed, uint32_t prop) const {
                float roll = cpu_hash_f(seed, prop);
                if (finiteMode_) {
                    // 0.00–0.125: mood 0 (open_default)
                    // 0.125–0.25: mood 1 (open_sunset)
                    // 0.25–0.525: mood 2 (indoor_flat)
                    // 0.525–0.80: mood 3 (indoor_vault)
                    // 0.80–0.90:  mood 4 (finite_outdoor)
                    // 0.90–1.00:  mood 5 (finite_outdoor_ref)
                    if (roll < 0.125f) return 0;
                    if (roll < 0.25f)  return 1;
                    if (roll < 0.525f) return 2;
                    if (roll < 0.80f)  return 3;
                    if (roll < 0.90f)  return 4;
                    return 5;
                }
                return cpu_hash(seed, prop) % MOOD_COUNT;
            }

            // --- Three Archetypes ---------------------------------------------------
            //
            //  0: Mountainous — high amplitude, elevated, sparse fine detail
            //  1: Varied      — moderate amplitude, wide range, balanced
            //  2: Basin       — low amplitude, depressed, rich fine detail
            //  3: Pool        — near-flat terrain, degenerate wave shape

            static constexpr uint32_t ARCHETYPE_COUNT = 4;

            struct ArchetypeProfile {
                // ─── Terrain modifiers ───────────────────────────────
                float amp_scale;           // height field amplitude multiplier
                float height_bias;         // vertical offset (positive = elevated)
                float activation_scale;    // activity field sensitivity

                // ─── Selection ───────────────────────────────────────
                float base_weight;         // prior probability (before neighbor influence)

                // ─── Per-tile jitter ─────────────────────────────────
                float amp_jitter_range;    // amp_scale *= 1 ± jitter/2
                float bias_jitter_range;   // height_bias += uniform(-jitter/2, +jitter/2)
            };

            //                                     amp   bias   act   weight  amp_jit  bias_jit
            static constexpr ArchetypeProfile ARCHETYPES[ARCHETYPE_COUNT] = {
                /* 0: mountainous */  {  2.0f,   4.0f,  0.7f,  1.8f,   0.3f,    1.0f  },
                /* 1: varied      */  {  1.0f,   0.0f,  1.0f,  1.3f,   0.3f,    1.0f  },
                /* 2: basin       */  {  0.5f,  -2.0f,  1.3f,  1.0f,   0.3f,    1.0f  },
                /* 3: pool        */  {  0.04f, -0.5f,  0.2f,  0.0f,   0.02f,   0.2f  },
            };

            // Neighbor coherence rules for archetype selection.
            // These control how the presence of neighboring archetypes
            // biases the selection for a new tile.
            struct ArchetypeSelectionRules {
                // Neighbor count thresholds and corresponding weight multipliers.
                // Applied in order: first matching threshold wins.
                static constexpr uint32_t DOMINANT_THRESHOLD = 4;    // >= this many → suppress
                static constexpr float    DOMINANT_MULTIPLIER = 0.2f; // strongly reduced
                static constexpr uint32_t COMMON_THRESHOLD = 2;    // >= this many → mild boost
                static constexpr float    COMMON_MULTIPLIER = 1.5f;
                static constexpr uint32_t PRESENT_THRESHOLD = 1;    // == this many → strong coherence
                static constexpr float    PRESENT_MULTIPLIER = 2.0f;
                // 0 neighbors: weight stays at base_weight (no modification)
            };

            // ── Entity Density Field ─────────────────────────────────────────
            //
            // Coarse spatial noise that creates dense and sparse regions.
            // Evaluated per-tile in generate_tile_state, stored on TileState.
            // All entity spawn gates multiply by this value.
            //
            //  ┌──────────────────────────────────┬───────────┬──────────────────────────────────────┐
            //  │ Constant                         │ Value     │ Effect                                │
            //  ├──────────────────────────────────┼───────────┼──────────────────────────────────────┤
            //  │ DENSITY_LATTICE_SPACING          │ 250 wu    │ Region size (~5 patches)              │
            //  │ DENSITY_SEED_BAND                │ 160       │ Decorrelated from terrain/color       │
            //  │ DENSITY_EXPONENT                 │ 0.6       │ <1 = skew toward dense, >1 = sparse  │
            //  │ DENSITY_MIN                      │ 0.1       │ Floor (never fully empty)             │
            //  │ DENSITY_MAX                      │ 3.0       │ Ceiling (3× base spawn rates)         │
            //  └──────────────────────────────────┴───────────┴──────────────────────────────────────┘

            static constexpr float DENSITY_LATTICE_SPACING = 250.0f;
            static constexpr uint32_t DENSITY_SEED_BAND = 160u;
            static constexpr float DENSITY_EXPONENT = 0.6f;
            static constexpr float DENSITY_MIN = 1.0f;
            static constexpr float DENSITY_MAX = 1.0f;

            // ─── Family Dispatch Table ──────────────────────────────────────
            //
            // Table-driven dispatch for the full entity lifecycle:
            // select, place, commit, evict, and mesh generation.
            // Adding a new entity family: write select/place/commit/
            // prepare_mesh functions, add wrappers, add 1 row here.

            struct FamilyDispatch {
                bool (*try_select)(Cartridge* self, int32_t gx, int32_t gz, EntityQueueEntry& e);
                bool (*try_place)(Cartridge* self, EntityQueueEntry& e, PlacementEntry& pe);
                void (*try_commit)(Cartridge* self, PlacementEntry& pe, wgpu::Queue& queue);
                void (*evict_slot)(Cartridge* self, uint32_t slot, wgpu::Queue& queue);
                bool (*prepare_mesh)(Cartridge* self, wgpu::Queue& queue);
                void (*dispatch_mesh)(Cartridge* self, wgpu::ComputePassEncoder& pass);
                const char* name;
            };

            // ── Mesh gen dispatch wrappers ──

            static bool dispatch_prepare_mesh_pyramid(Cartridge* self, wgpu::Queue& queue) {
                return self->prepare_pyramid_mesh_gen(queue);
            }
            static void dispatch_mesh_gen_pyramid(Cartridge* self, wgpu::ComputePassEncoder& pass) {
                self->renderer_.dispatch_pyramid_mesh_gen(pass, self->gpuState_.pyramid_mesh_gen_group());
            }

            static bool dispatch_prepare_mesh_arch(Cartridge* self, wgpu::Queue& queue) {
                return self->prepare_arch_mesh_gen(queue);
            }
            static void dispatch_mesh_gen_arch(Cartridge* self, wgpu::ComputePassEncoder& pass) {
                self->renderer_.dispatch_arch_mesh_gen(pass, self->gpuState_.arch_mesh_gen_group());
            }

            static bool dispatch_prepare_mesh_column(Cartridge* self, wgpu::Queue& queue) {
                return self->prepare_column_mesh_gen(queue);
            }
            static void dispatch_mesh_gen_column(Cartridge* self, wgpu::ComputePassEncoder& pass) {
                self->renderer_.dispatch_column_mesh_gen(pass, self->gpuState_.column_mesh_gen_group());
            }

            // ── Eviction dispatch wrappers ──

            static void dispatch_evict_pyramid(Cartridge* self,
                uint32_t slot, wgpu::Queue& queue)
            {
                self->cpuPyramids_.instances[slot] = GPUPyramidInstance{};
                self->activePyramids_[slot].active = false;
                self->activePyramidCount_--;
                self->groundEntriesDirty_ = true;
                { GPUPyramidMeshParams ep{}; self->gpuState_.upload_pyramid_mesh_params_slot(queue, slot, ep); }
                self->pyramidMeshGenPending_ = true;

                uint32_t max_idx = 0;
                for (uint32_t i = 0; i < Dim::MAX_PYRAMID_INSTANCES; i++) {
                    if (self->activePyramids_[i].active) max_idx = i + 1;
                }
                self->cpuPyramids_.count = max_idx;
                self->gpuState_.upload_pyramids(queue, self->cpuPyramids_);
#ifdef DIAG_ENTITY_LIFECYCLE
                std::cout << "[DIAG:EVICT]   pyr slot=" << slot << "\n";
#endif
            }

            static void dispatch_evict_arch(Cartridge* self,
                uint32_t slot, wgpu::Queue& queue)
            {
                self->clear_pier(queue, Dim::PIER_ARCH_BASE + slot * 2);
                self->clear_pier(queue, Dim::PIER_ARCH_BASE + slot * 2 + 1);
                self->activeArches_[slot].active = false;
                self->activeArchCount_--;
                self->portalsDirty_ = true;
                { GPUArchMeshParams ep{}; self->gpuState_.upload_arch_mesh_params_slot(queue, slot, ep); }
                self->archMeshGenPending_ = true;
#ifdef DIAG_ENTITY_LIFECYCLE
                std::cout << "[DIAG:EVICT]   arch slot=" << slot << "\n";
#endif
            }

            static void dispatch_evict_column(Cartridge* self,
                uint32_t slot, wgpu::Queue& queue)
            {
                self->clear_pier(queue, Dim::PIER_COLUMN_BASE + slot);
                self->activeColumns_[slot].active = false;
                self->activeColumnCount_--;
                { GPUColumnMeshParams ep{}; self->gpuState_.upload_column_mesh_params_slot(queue, slot, ep); }
                self->columnMeshGenPending_ = true;
#ifdef DIAG_ENTITY_LIFECYCLE
                std::cout << "[DIAG:EVICT]   col slot=" << slot << "\n";
#endif
            }

            static void dispatch_evict_antenna(Cartridge* self,
                uint32_t slot, wgpu::Queue& queue)
            {
                uint32_t gpu_slot = slot + Dim::ANTENNA_SLOT_OFFSET;
                self->clear_pier(queue, Dim::PIER_COLUMN_BASE + gpu_slot);
                self->activeAntennas_[slot].active = false;
                self->activeAntennaCount_--;
                { GPUColumnMeshParams ep{}; self->gpuState_.upload_column_mesh_params_slot(queue, gpu_slot, ep); }
                self->columnMeshGenPending_ = true;
#ifdef DIAG_ENTITY_LIFECYCLE
                std::cout << "[DIAG:EVICT]   ant slot=" << slot << "\n";
#endif
            }

            static void dispatch_evict_palm(Cartridge* self,
                uint32_t slot, wgpu::Queue& queue)
            {
                self->activePalms_[slot].active = false;
                self->activePalmCount_--;
                { GPUPalmMeshParams ep{}; self->gpuState_.upload_palm_mesh_params_slot(queue, slot, ep); }
                self->palmMeshGenPending_ = true;
                self->groundEntriesDirty_ = true;
#ifdef DIAG_ENTITY_LIFECYCLE
                std::cout << "[DIAG:EVICT]   palm slot=" << slot << "\n";
#endif
            }

            // ── Mesh gen dispatch wrappers (palm) ──

            static bool dispatch_prepare_mesh_palm(Cartridge* self, wgpu::Queue& queue) {
                return self->prepare_palm_mesh_gen(queue);
            }
            static void dispatch_mesh_gen_palm(Cartridge* self, wgpu::ComputePassEncoder& pass) {
                self->renderer_.dispatch_palm_mesh_gen(pass, self->gpuState_.palm_mesh_gen_group());
            }

            static void dispatch_evict_cactus(Cartridge* self,
                uint32_t slot, wgpu::Queue& queue)
            {
                self->activeCacti_[slot].active = false;
                self->activeCactusCount_--;
                { GPUCactusMeshParams ep{}; self->gpuState_.upload_cactus_mesh_params_slot(queue, slot, ep); }
                self->cactusMeshGenPending_ = true;
                self->groundEntriesDirty_ = true;
#ifdef DIAG_ENTITY_LIFECYCLE
                std::cout << "[DIAG:EVICT]   cact slot=" << slot << "\n";
#endif
            }

            // ── Mesh gen dispatch wrappers (cactus) ──

            static bool dispatch_prepare_mesh_cactus(Cartridge* self, wgpu::Queue& queue) {
                return self->prepare_cactus_mesh_gen(queue);
            }
            static void dispatch_mesh_gen_cactus(Cartridge* self, wgpu::ComputePassEncoder& pass) {
                self->renderer_.dispatch_cactus_mesh_gen(pass, self->gpuState_.cactus_mesh_gen_group());
            }

            static void dispatch_evict_blade(Cartridge* self,
                uint32_t slot, wgpu::Queue& queue)
            {
                self->activeBlades_[slot].active = false;
                self->activeBladeCount_--;
                { GPUBladeClusterMeshParams ep{}; self->gpuState_.upload_blade_mesh_params_slot(queue, slot, ep); }
                self->bladeMeshGenPending_ = true;
                self->groundEntriesDirty_ = true;
#ifdef DIAG_ENTITY_LIFECYCLE
                std::cout << "[DIAG:EVICT]   blad slot=" << slot << "\n";
#endif
            }

            static bool dispatch_prepare_mesh_blade(Cartridge* self, wgpu::Queue& queue) {
                return self->prepare_blade_mesh_gen(queue);
            }
            static void dispatch_mesh_gen_blade(Cartridge* self, wgpu::ComputePassEncoder& pass) {
                self->renderer_.dispatch_blade_mesh_gen(pass, self->gpuState_.blade_mesh_gen_group());
            }

            static void dispatch_evict_sphere(Cartridge* self,
                uint32_t slot, wgpu::Queue& queue) {
                self->activeFloaters_[slot].active = false;
                self->activeFloaterCount_--;
                GPUFloatingEntityState empty{};
                self->gpuState_.upload_sphere_entity_slot(queue, slot, empty);
#ifdef DIAG_ENTITY_LIFECYCLE
                std::cout << "[DIAG:EVICT]   sph slot=" << slot << "\n";
#endif
            }

            static bool dispatch_prepare_mesh_sphere(Cartridge* self, wgpu::Queue& queue) {
                (void)self; (void)queue;
                return false;  // no CPU mesh gen — GPU compute handles update_sphere
            }
            static void dispatch_mesh_gen_sphere(Cartridge* self, wgpu::ComputePassEncoder& pass) {
                (void)self; (void)pass;
            }

            static void dispatch_evict_cube(Cartridge* self,
                uint32_t slot, wgpu::Queue& queue) {
                self->activeCubes_[slot].active = false;
                self->activeCubeCount_--;
                GPUFloatingEntityState empty{};
                self->gpuState_.upload_cube_entity_slot(queue, slot, empty);
#ifdef DIAG_ENTITY_LIFECYCLE
                std::cout << "[DIAG:EVICT]   cube slot=" << slot << "\n";
#endif
            }

            static bool dispatch_prepare_mesh_cube(Cartridge* self, wgpu::Queue& queue) {
                (void)self; (void)queue;
                return false;  // no CPU mesh gen — GPU compute handles update_cube
            }
            static void dispatch_mesh_gen_cube(Cartridge* self, wgpu::ComputePassEncoder& pass) {
                (void)self; (void)pass;
            }

            // ── GoL dispatch wrappers ──

            static bool dispatch_select_gol(Cartridge* self,
                int32_t gx, int32_t gz, EntityQueueEntry& e) {
                if (!self->moodAllowsGoLZones_) { return false; }   // mood gate — no new zones
                return self->select_gol_for_patch(gx, gz, e.gol);
            }

            static bool dispatch_place_gol(Cartridge* self,
                EntityQueueEntry& e, PlacementEntry& pe) {
                pe.family = e.family; pe.gx = e.gx; pe.gz = e.gz;
                if (self->place_gol_from_selection(e.gol, pe.gol)) {
                    return true;
                } else {
                    self->golZones_[e.gol.slot].active = false;
                    return false;
                }
            }

            static void dispatch_commit_gol(Cartridge* self,
                PlacementEntry& pe, wgpu::Queue& queue) {
                auto* host = self->find_patch(pe.gol.host_gx, pe.gol.host_gz);
                if (host) {
                    self->commit_gol(pe.gol, pe.gx, pe.gz, queue);
                    host->record_entity(PopFamily::GOL, pe.gol.slot);
                } else {
                    self->golZones_[pe.gol.slot].active = false;
#ifdef DIAG_ENTITY_LIFECYCLE
                    std::cout << "[DIAG:REJECT] gol slot=" << pe.gol.slot
                        << " host=(" << pe.gol.host_gx << "," << pe.gol.host_gz
                        << ") -- no host patch\n";
#endif
                }
            }

            static void dispatch_evict_gol(Cartridge* self,
                uint32_t slot, wgpu::Queue& queue) {
                self->gpuState_.deactivate_zone_slot(queue, slot);
                self->golZones_[slot].active = false;
                self->golZoneCount_--;
#ifdef DIAG_ENTITY_LIFECYCLE
                std::cout << "[DIAG:EVICT]   gol slot=" << slot << "\n";
#endif
            }

            static bool dispatch_prepare_mesh_gol(Cartridge* self, wgpu::Queue& queue) {
                (void)self; (void)queue;
                return false;  // zone mesh gen is a separate compute pass
            }
            static void dispatch_mesh_gen_gol(Cartridge* self, wgpu::ComputePassEncoder& pass) {
                (void)self; (void)pass;
            }

            // ── Gallery dispatch wrappers ──

            static bool dispatch_select_gallery(Cartridge* self,
                int32_t gx, int32_t gz, EntityQueueEntry& e) {
                return self->select_gallery_for_patch(gx, gz, e.gallery);
            }

            static bool dispatch_place_gallery(Cartridge* self,
                EntityQueueEntry& e, PlacementEntry& pe) {
                pe.family = e.family; pe.gx = e.gx; pe.gz = e.gz;
                if (self->place_gallery_from_selection(e.gallery, pe.gallery)) {
                    return true;
                } else {
                    self->galleryCenters_[e.gallery.slot].active = false;
                    return false;
                }
            }

            static void dispatch_commit_gallery(Cartridge* self,
                PlacementEntry& pe, wgpu::Queue& queue) {
                auto* host = self->find_patch(pe.gallery.host_gx, pe.gallery.host_gz);
                if (host) {
                    self->commit_gallery(pe.gallery, pe.gx, pe.gz, queue);
                    // Only record entity_ref if gallery is still active (commit may deactivate on 0 paintings)
                    if (self->galleryCenters_[pe.gallery.slot].active) {
                        host->record_entity(PopFamily::GALLERY, pe.gallery.slot);
                    }
                } else {
                    self->galleryCenters_[pe.gallery.slot].active = false;
#ifdef DIAG_ENTITY_LIFECYCLE
                    std::cout << "[DIAG:REJECT] gall slot=" << pe.gallery.slot
                        << " host=(" << pe.gallery.host_gx << "," << pe.gallery.host_gz
                        << ") -- no host patch\n";
#endif
                }
            }

            static void dispatch_evict_gallery(Cartridge* self,
                uint32_t slot, wgpu::Queue& queue) {
                auto& gc = self->galleryCenters_[slot];
                if (gc.active) {
                    self->evict_paintings_for_patch(gc.patch_gx, gc.patch_gz, queue);
                    gc.active = false;
                }
#ifdef DIAG_ENTITY_LIFECYCLE
                std::cout << "[DIAG:EVICT]   gall slot=" << slot << "\n";
#endif
            }

            static bool dispatch_prepare_mesh_gallery(Cartridge* self, wgpu::Queue& queue) {
                (void)self; (void)queue;
                return false;
            }
            static void dispatch_mesh_gen_gallery(Cartridge* self, wgpu::ComputePassEncoder& pass) {
                (void)self; (void)pass;
            }

            // ── Ribbon dispatch wrappers ──

            static bool dispatch_select_ribbon(Cartridge* self,
                int32_t gx, int32_t gz, EntityQueueEntry& e) {
                return self->select_ribbon_for_patch(gx, gz, e.ribbon);
            }

            static bool dispatch_place_ribbon(Cartridge* self,
                EntityQueueEntry& e, PlacementEntry& pe) {
                pe.family = e.family; pe.gx = e.gx; pe.gz = e.gz;
                if (self->place_ribbon_from_selection(e.ribbon, pe.ribbon)) {
                    return true;
                } else {
                    self->activeRibbons_[e.ribbon.slot].active = false;
                    return false;
                }
            }

            static void dispatch_commit_ribbon(Cartridge* self,
                PlacementEntry& pe, wgpu::Queue& queue) {
                // Commit the ribbon state (GPU mirror, active record, tip positions)
                self->commit_ribbon(pe.ribbon, pe.gx, pe.gz, queue);

                uint32_t slot = pe.ribbon.slot;
                auto& ar = self->activeRibbons_[slot];

                // Register with tip patches that currently exist.
                // Late registration handles the other tip when its patch is allocated.
                uint32_t refs = 0;
                auto* near_host = self->find_patch(ar.near_tip_gx, ar.near_tip_gz);
                if (near_host) {
                    near_host->record_entity(PopFamily::RIBBON, slot);
                    ar.near_tip_registered = true;
                    refs++;
                }
                auto* far_host = self->find_patch(ar.far_tip_gx, ar.far_tip_gz);
                if (far_host && (ar.far_tip_gx != ar.near_tip_gx || ar.far_tip_gz != ar.near_tip_gz)) {
                    far_host->record_entity(PopFamily::RIBBON, slot);
                    ar.far_tip_registered = true;
                    refs++;
                }

                if (refs == 0) {
                    std::cout << "[Ribbon] REJECT slot=" << slot
                        << " — no tip patches alive\n";
                    ar = ActiveRibbon{};
                    self->ribbonStates_[slot] = GPURibbonState{};
                    self->activeRibbonCount_--;
                    return;
                }
                ar.ref_count = refs;
            }

            static void dispatch_evict_ribbon(Cartridge* self,
                uint32_t slot, wgpu::Queue& queue) {
                auto& ar = self->activeRibbons_[slot];
                if (!ar.active) return;

                // Decrement ref count — one anchor patch has been evicted.
                // Only fully evict when all referencing patches are gone.
                if (ar.ref_count > 1) {
                    ar.ref_count--;
                    return;
                }

                // Final reference gone — full eviction
                ar = ActiveRibbon{};
                self->ribbonStates_[slot] = GPURibbonState{};
                self->activeRibbonCount_--;
                if (self->renderedRibbonSlot_ == slot) {
                    GPURibbonState empty{};
                    self->gpuState_.upload_ribbon(queue, empty);
                    self->renderedRibbonSlot_ = UINT32_MAX;
                }
                std::cout << "[Ribbon] EVICT slot=" << slot << "\n";
            }

            static bool dispatch_prepare_mesh_ribbon(Cartridge* self, wgpu::Queue& queue) {
                (void)self; (void)queue;
                return false;
            }
            static void dispatch_mesh_gen_ribbon(Cartridge* self, wgpu::ComputePassEncoder& pass) {
                (void)self; (void)pass;
                // no-op — GPU compute handles ribbon rendering
            }

            // ── Dispatch table (order matches PopFamily enum) ──

            // ── Generic Entity Pipeline (modules/entity_pipeline.inl) ──
#include "modules/entity_pipeline.inl"

            static constexpr FamilyDispatch FAMILY_DISPATCH[PopFamily::COUNT] = {
                { dispatch_select_pyramid_generic, dispatch_place_pyramid_generic, dispatch_commit_pyramid_generic,
                  dispatch_evict_pyramid, dispatch_prepare_mesh_pyramid, dispatch_mesh_gen_pyramid,
                  "pyr" },
                { dispatch_select_arch_generic, dispatch_place_arch_generic, dispatch_commit_arch_generic,
                  dispatch_evict_arch,    dispatch_prepare_mesh_arch,    dispatch_mesh_gen_arch,
                  "arch" },
                { dispatch_select_column_generic, dispatch_place_column_generic, dispatch_commit_column_generic,
                  dispatch_evict_column,  dispatch_prepare_mesh_column,  dispatch_mesh_gen_column,
                  "col" },
                { dispatch_select_antenna_generic, dispatch_place_antenna_generic, dispatch_commit_antenna_generic,
                  dispatch_evict_antenna, dispatch_prepare_mesh_column,  dispatch_mesh_gen_column,
                  "ant" },
                { dispatch_select_palm_generic, dispatch_place_palm_generic, dispatch_commit_palm_generic,
                  dispatch_evict_palm,   dispatch_prepare_mesh_palm,   dispatch_mesh_gen_palm,
                  "palm" },
                { dispatch_select_cactus_generic, dispatch_place_cactus_generic, dispatch_commit_cactus_generic,
                  dispatch_evict_cactus, dispatch_prepare_mesh_cactus, dispatch_mesh_gen_cactus,
                  "cact" },
                { dispatch_select_blade_generic, dispatch_place_blade_generic, dispatch_commit_blade_generic,
                  dispatch_evict_blade, dispatch_prepare_mesh_blade, dispatch_mesh_gen_blade,
                  "blad" },
                { dispatch_select_sphere_generic, dispatch_place_sphere_generic, dispatch_commit_sphere_generic,
                  dispatch_evict_sphere, dispatch_prepare_mesh_sphere, dispatch_mesh_gen_sphere,
                  "sph" },
                { dispatch_select_ribbon, dispatch_place_ribbon, dispatch_commit_ribbon,
                  dispatch_evict_ribbon, dispatch_prepare_mesh_ribbon, dispatch_mesh_gen_ribbon,
                  "ribn" },
                { dispatch_select_cube_generic, dispatch_place_cube_generic, dispatch_commit_cube_generic,
                  dispatch_evict_cube, dispatch_prepare_mesh_cube, dispatch_mesh_gen_cube,
                  "cube" },
                { dispatch_select_gol, dispatch_place_gol, dispatch_commit_gol,
                  dispatch_evict_gol, dispatch_prepare_mesh_gol, dispatch_mesh_gen_gol,
                  "gol" },
                { dispatch_select_gallery, dispatch_place_gallery, dispatch_commit_gallery,
                  dispatch_evict_gallery, dispatch_prepare_mesh_gallery, dispatch_mesh_gen_gallery,
                  "gall" },
            };

            // ─── Population Themes ───────────────────────────────────────────
            //
            // A theme is the compositional intent for a region. Like a palette
            // slot sets color character, a theme sets entity character: what
            // spawns, at what scale, how densely, and how it arranges itself.
            //
            // A stochastic lattice at THEME_LATTICE_SPACING picks which theme
            // dominates at each point. Spawn weights blend smoothly across
            // boundaries. Tier bias comes from the dominant theme (no blending).
            //
            // The transition theme is the default — most of the world. Sparse
            // pyramids, small antennas, column clusters, occasional arch.
            // Interesting themes are the exceptions that emerge from the field.
            //
            //  ┌──────────────────────────────────────────────────────────────────────────────────────────┐
            //  │ THEME CONTROL SURFACE                                                                   │
            //  ├──────────────────────┬──────────────────────┬────────────────────────────────────────────┤
            //  │ Theme                │ Weight  Density      │ Character                                  │
            //  ├──────────────────────┼──────────────────────┼────────────────────────────────────────────┤
            //  │ 0: Transition        │  0.40   ×1.0         │ Sparse, quiet connective tissue            │
            //  │ 1: Monumental        │  0.12   ×0.7         │ Big pyramids, monumental arches, imposing  │
            //  │ 2: Colonnade         │  0.18   ×1.5         │ Dense columns, doorway arcades, no pyramid │
            //  │ 3: Antenna path      │  0.15   ×1.2         │ Antenna corridor, colossal sentinels       │
            //  │ 4: Barren            │  0.15   ×0.3         │ Near-empty, occasional obelisk             │
            //  └──────────────────────┴──────────────────────┴────────────────────────────────────────────┘

            static constexpr float THEME_LATTICE_SPACING = 500.0f;
            static constexpr uint32_t THEME_SEED_BAND = 170u;
            static constexpr uint32_t THEME_COUNT = 5;
            static constexpr float THEME_BASE_WEIGHT = 10.0f;

            // ── Theme Envelope ──────────────────────────────────────────────
            //
            // Single active theme at a time. When selected, its weight spikes
            // and decays over a patch count. Cooldown prevents immediate
            // repetition after expiry.

            struct ThemeEnvelope {
                int32_t  active = -1;              // theme index, or -1 (no bias)
                uint32_t elapsed = 0;               // patches since this theme fired
                uint32_t cooldowns[THEME_COUNT]{};  // per-theme remaining cooldown
            };

            struct PopulationTheme {
                float spawn_weight[PopFamily::COUNT];          // multiplier on base spawn chance per family
                float tier_wt_pyramid[3];                      // multiplier on pyramid tier base weights
                float tier_wt_arch[3];                         // multiplier on arch tier base weights
                float tier_wt_column[3];                       // multiplier on column tier base weights (Pillar, Doric, Ornate)
                float tier_wt_antenna[3];                      // multiplier on antenna tier base weights (Antenna, Squat, Colossal)
                float tier_wt_palm[3];                         // multiplier on palm tier base weights (Sapling, Coastal, Royal)
                float tier_wt_cactus[3];                       // multiplier on cactus tier base weights (Finger, Saguaro, Candelabra)
                float tier_wt_blade[3];                        // multiplier on blade tier base weights (Sprout, Clump, Thicket)
                float tier_wt_sphere[2];                       // multiplier on sphere tier base weights (Sentinel, Anomaly)
                float tier_wt_ribbon[3];                       // multiplier on ribbon tier base weights (Serpentine, Helix, Streamer)
                float tier_wt_cube[4];                         // multiplier on cube tier base weights (SmCube..Monolith)
                float density_mult;                            // multiplier on entity_density

                // Envelope parameters (replace lattice weight for theme selection)
                float    spike;
                uint32_t sustain;       // patches at full spike
                uint32_t decay;         // patches for linear decay to base
                uint32_t cooldown;      // patches before re-eligible after expiry

                // Lattice node weight (spatial distribution of themes)
                float weight;
            };

            //  ┌──────────────────────────────────────────────────────────────────────────────┐
            //  │ THEME PROFILES — Envelope-selected                                            │
            //  ├──────────────────┬────────┬────────┬────────┬─────────┬─────────────────────────┤
            //  │ Theme            │ Pyr sp │ Arch sp│ Col sp │ Density │ Envelope                │
            //  ├──────────────────┼────────┼────────┼────────┼─────────┼─────────────────────────┤
            //  │ 0 Transition     │  0.4   │  0.3   │  0.7   │  ×1.0   │ 150/20/3/0             │
            //  │ 1 Monumental     │  1.5   │  1.0   │  1.0   │  ×1.0   │ 150/10/10/8            │
            //  │ 2 Colonnade      │  0.3   │  1.0   │  4.0   │  ×1.0   │ 150/15/6/6             │
            //  │ 3 Antenna        │  0.5   │  0.5   │  4.0   │  ×1.0   │ 180/10/5/5             │
            //  │ 4 Barren         │  0.4   │  0.3   │  0.5   │  ×1.0   │ 100/12/3/4             │
            //  └──────────────────┴────────┴────────┴────────┴─────────┴─────────────────────────┘

            static constexpr PopulationTheme THEMES[THEME_COUNT] = {
                // ── 0: TRANSITION — sparse connective tissue ─────────────────
                {   { 0.4f, 0.3f, 0.7f, 0.3f, 0.3f, 0.3f, 0.5f, 0.3f, 1.0f, 0.5f, 0.5f, 0.5f },   // spawn_weight [pyr..sph, ribn, cube, gol, gall]
                    { 1.0f, 1.0f, 1.0f },                                       // tier_pyr
                    { 1.0f, 0.3f, 1.0f },                                       // tier_arch
                    { 0.1f, 0.2f, 0.3f },                                       // tier_col
                    { 0.1f, 2.0f, 0.7f },                                       // tier_ant
                    { 1.0f, 1.0f, 1.0f },                                       // tier_palm
                    { 1.0f, 1.0f, 1.0f },                                       // tier_cactus
                    { 1.0f, 1.0f, 1.0f },                                       // tier_blade
                    { 1.0f, 1.0f },                                              // tier_sphere (neutral)
                    { 1.0f, 1.0f, 1.0f },                                       // tier_ribbon (neutral)
                    { 1.0f, 1.0f, 1.0f, 1.0f },                                 // tier_cube (neutral)
                    1.0f,                                                         // density
                    150.0f, 20u, 3u, 0u,                                          // spike, sustain, decay, cooldown
                    0.21f                                                         // weight
                },
                // ── 1: MONUMENTAL — big pyramids, varied arches, heavy columns
                {   { 1.5f, 1.0f, 1.0f, 0.5f, 0.2f, 0.2f, 0.5f, 0.3f, 1.0f, 0.3f, 0.3f, 0.3f },
                    { 0.2f, 0.5f, 3.0f },
                    { 2.0f, 0.1f, 3.0f },
                    { 0.01f, 0.01f, 1.0f },
                    { 0.5f, 1.5f, 0.5f },
                    { 1.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f },
                    { 1.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f, 1.0f, 1.0f },
                    1.0f,
                    150.0f, 10u, 10u, 8u,
                    0.30f
                },
                // ── 2: COLONNADE — dense columns, moderate arches ────────────
                {   { 0.3f, 1.0f, 4.0f, 0.5f, 0.3f, 0.3f, 0.5f, 0.3f, 1.0f, 0.3f, 0.5f, 0.4f },
                    { 1.0f, 1.0f, 1.0f },
                    { 3.0f, 0.5f, 1.0f },
                    { 0.3f, 3.0f, 5.0f },
                    { 0.2f, 0.1f, 0.1f },
                    { 1.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f },
                    { 1.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f, 1.0f, 1.0f },
                    1.0f,
                    150.0f, 15u, 6u, 6u,
                    0.31f
                },
                // ── 3: ANTENNA — antenna-dominant corridor ───────────────────
                {   { 0.5f, 0.5f, 1.0f, 4.0f, 0.5f, 0.5f, 0.5f, 0.3f, 1.0f, 0.3f, 0.3f, 0.3f },
                    { 1.0f, 0.05f, 2.0f },
                    { 1.0f, 0.2f, 0.8f },
                    { 0.1f, 0.3f, 0.3f },
                    { 0.5f, 3.5f, 1.0f },
                    { 1.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f },
                    { 1.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f, 1.0f, 1.0f },
                    1.0f,
                    180.0f, 10u, 5u, 5u,
                    0.18f
                },
                // ── 4: BARREN — near-empty ───────────────────────────────────
                {   { 0.4f, 0.3f, 0.5f, 0.3f, 0.2f, 0.2f, 0.1f, 0.3f, 1.0f, 0.1f, 0.2f, 0.6f },
                    { 2.0f, 0.5f, 0.2f },
                    { 1.0f, 1.0f, 1.0f },
                    { 0.2f, 0.5f, 0.5f },
                    { 1.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f },
                    { 1.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f, 1.0f, 1.0f },
                    1.0f,
                    100.0f, 12u, 3u, 4u,
                    0.04f
                },
            };

            // Select a theme at a lattice node from cumulative weights
            static uint32_t select_theme_at_node(uint32_t node_seed) {
                float roll = cpu_hash_f(node_seed, 370u);
                float cumul = 0.0f;
                float total = 0.0f;
                for (uint32_t t = 0; t < THEME_COUNT; t++) total += THEMES[t].weight;
                for (uint32_t t = 0; t < THEME_COUNT; t++) {
                    cumul += THEMES[t].weight / total;
                    if (roll < cumul) return t;
                }
                return THEME_COUNT - 1;
            }

            // ── Theme Envelope — sequential theme selection ──────────────────
            //
            // Replaces the lattice-based theme blend for spawn decisions.
            // One theme is active at a time. Its weight spikes and decays
            // over a patch count. Cooldown prevents immediate repetition.

            static float theme_envelope_weight(const PopulationTheme& theme, uint32_t elapsed) {
                if (elapsed < theme.sustain) return theme.spike;
                if (elapsed < theme.sustain + theme.decay) {
                    float t = (float)(elapsed - theme.sustain) / (float)theme.decay;
                    return theme.spike + (THEME_BASE_WEIGHT - theme.spike) * t;
                }
                return THEME_BASE_WEIGHT;
            }

            // Called ONCE per patch, inside the spawn loop, BEFORE per-family gates.
            // Returns the theme index to use for this patch.
            uint32_t evaluate_theme_envelope(uint32_t tile_seed_value) {
                auto& env = themeEnvelope_;

                // Build effective weights
                float weights[THEME_COUNT];
                float total = 0.0f;
                for (uint32_t i = 0; i < THEME_COUNT; i++) {
                    if (env.cooldowns[i] > 0) {
                        weights[i] = 0.0f;
                    }
                    else if ((int32_t)i == env.active) {
                        weights[i] = theme_envelope_weight(THEMES[i], env.elapsed);
                    }
                    else {
                        weights[i] = THEME_BASE_WEIGHT;
                    }
                    total += weights[i];
                }
                if (total < 0.001f) total = 1.0f;

                // Roll from weights
                float roll = cpu_hash_f(tile_seed_value, 370u);
                uint32_t selected = THEME_COUNT - 1;
                float cumul = 0.0f;
                for (uint32_t i = 0; i < THEME_COUNT; i++) {
                    cumul += weights[i] / total;
                    if (roll < cumul) { selected = i; break; }
                }

                // State transitions
                if ((int32_t)selected != env.active) {
                    if (env.active >= 0) {
                        env.cooldowns[env.active] = THEMES[env.active].cooldown;
                    }
                    env.active = (int32_t)selected;
                    env.elapsed = 0;

                    // Census dump on theme transition
#ifdef DIAG_ENTITY_CENSUS
                    dump_entity_census(theme_short_name(selected));
#endif
                }
                else {
                    env.elapsed++;
                }

                // Check expiry
                if (env.active >= 0) {
                    const auto& th = THEMES[env.active];
                    if (env.elapsed >= th.sustain + th.decay) {
                        env.cooldowns[env.active] = th.cooldown;
                        env.active = -1;
                        env.elapsed = 0;
                    }
                }

                // Tick cooldowns
                for (uint32_t i = 0; i < THEME_COUNT; i++) {
                    if (env.cooldowns[i] > 0) env.cooldowns[i]--;
                }

                return selected;
            }

            // ─── Terrain Tokens ──────────────────────────────────────────────
            //
            // Carried compositional priors that bias sequential tile generation.
            // Each token holds per-archetype weight multipliers and a generation
            // budget that decrements with each primary tile generation.
            // When budget reaches zero, the token is cleared.
            //
            // Tokens are READ inside generate_tile_state() (member access),
            // TICKED and EMITTED by tick_terrain_tokens() after each primary
            // tile generation. Neighbor padding calls do NOT tick.
            //
            // The mechanism:
            //   1. Patch generates → reads active tokens as priors on archetype weights
            //   2. Archetype outcome + its jitter properties → emission roll
            //   3. Emission may push a new token (bias type + budget drawn stochastically)
            //   4. All tokens decrement budget; dead tokens cleared
            //
            // The stack is small and fixed. If full, the oldest token (lowest budget)
            // is evicted to make room. In practice, ≤4 are alive at any time.

            static constexpr uint32_t MAX_TERRAIN_TOKENS = 8;

            struct TerrainToken {
                float archetype_bias[ARCHETYPE_COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f };
                uint32_t budget = 0;
                bool active = false;
            };

            TerrainToken terrainTokens_[MAX_TERRAIN_TOKENS]{};

            // ── Emission Profiles ────────────────────────────────────────────
            //
            // Each archetype defines how it biases subsequent tile generation
            // when it emits a terrain token. The emission mechanism:
            //
            //   1. After a tile generates, it rolls emit_chance to decide
            //      whether it emits a token at all (0.0 = never, 1.0 = always).
            //   2. If emitting, it rolls pivot_chance: continuation vs pivot.
            //      Continuation carries the current terrain character forward.
            //      Pivot transitions to a different landform.
            //   3. Budget is drawn uniformly in [budget_min, budget_max]:
            //      how many primary tile generations the token survives.
            //   4. The bias vector multiplies into archetype selection weights
            //      for all tiles generated while the token is alive.
            //      Values >1.0 boost that archetype, <1.0 suppress it.
            //
            // Bias vector order: { mountainous, varied, basin, pool }
            //
            // Tuning these profiles IS the art direction for terrain composition.

            struct TerrainEmissionProfile {
                float emit_chance;                            // [0,1] probability of emitting any token
                uint32_t budget_min, budget_max;              // generation lifespan range
                float continuation_bias[ARCHETYPE_COUNT];     // archetype weight multipliers when continuing
                float pivot_chance;                           // [0,1] probability of pivoting vs continuing
                float pivot_bias[ARCHETYPE_COUNT];            // archetype weight multipliers when pivoting
            };

            //  ┌────────────────────┬────────┬─────────┬──────────────────────────────────────────┬────────┬──────────────────────────────────────────┐
            //  │                    │ emit%  │ budget  │ continuation bias                         │ pivot% │ pivot bias                               │
            //  │                    │        │ min max │ mount  varied basin  pool                 │        │ mount  varied basin  pool                 │
            //  ├────────────────────┼────────┼─────────┼──────────────────────────────────────────┼────────┼──────────────────────────────────────────┤
            //  │ 0: mountainous     │  0.45  │  2   5  │  2.0    1.5    0.3    0.0  (ridge runs)  │  0.25  │  0.3    2.0    1.5    0.0  (descend)     │
            //  │ 1: varied          │  0.25  │  1   3  │  0.8    1.5    0.8    0.2  (neutral)     │  0.30  │  1.5    0.5    1.5    0.1  (diversify)   │
            //  │ 2: basin           │  0.40  │  2   4  │  0.2    0.8    2.0    1.0  (flat runs)   │  0.20  │  0.5    1.5    0.5    0.3  (ascend)      │
            //  │ 3: pool            │  0.20  │  1   2  │  0.0    0.5    1.5    1.5  (hold flat)   │  0.35  │  0.3    1.0    2.0    0.2  (drain out)   │
            //  └────────────────────┴────────┴─────────┴──────────────────────────────────────────┴────────┴──────────────────────────────────────────┘
            static constexpr TerrainEmissionProfile TERRAIN_EMISSION[ARCHETYPE_COUNT] = {
                /* 0: mountainous */ { 0.45f,  2, 5,  { 2.0f, 1.5f, 0.3f, 0.0f },  0.25f, { 0.3f, 2.0f, 1.5f, 0.0f } },
                /* 1: varied      */ { 0.25f,  1, 3,  { 0.8f, 1.5f, 0.8f, 0.2f },  0.30f, { 1.5f, 0.5f, 1.5f, 0.1f } },
                /* 2: basin       */ { 0.40f,  2, 4,  { 0.2f, 0.8f, 2.0f, 1.0f },  0.20f, { 0.5f, 1.5f, 0.5f, 0.3f } },
                /* 3: pool        */ { 0.20f,  1, 2,  { 0.0f, 0.5f, 1.5f, 1.5f },  0.35f, { 0.3f, 1.0f, 2.0f, 0.2f } },
            };

            // ── Amplitude Momentum ───────────────────────────────────────────
            //
            // When amp_jitter rolls extreme, the token also carries amplitude
            // bias that nudges the next patch further in that direction.
            // Creates natural ridgelines and depth sequences.

            static constexpr float AMP_MOMENTUM_THRESHOLD = 0.15f;  // |jitter - 1.0| above this → emit amp momentum
            static constexpr float AMP_MOMENTUM_CARRY = 0.6f;       // fraction of excess carried forward

            // ─── Population Batch System ─────────────────────────────────────
            //
            // Entities spawn in observed batches. The first few entities of a
            // batch define the neighborhood's character; the rest follow it.
            //
            // Two derived biases update LIVE as observations accumulate:
            //   Type affinity — types that appeared more get boosted proportionally.
            //   Scale tendency — average tier scale biases select_tier toward similar.
            //
            // After POP_BATCH_SIZE patches, the batch resets: a few "exploratory"
            // patches with neutral priors, then the new batch character emerges.
            //
            // Each batch rolls a MODE at birth:
            //   Affinity  — more of the same (columns attract columns)
            //   Repulsion — opposites attract (columns push toward pyramids/arches)
            //   Neutral   — no bias (pure independent rolls, breathing room)
            //
            // No hand-crafted affinity matrices. The correlation IS the aesthetic.
            //
            //  ┌──────────────────────────────────────────────────────────────────────────────────────────┐
            //  │ POPULATION BATCH CONTROL SURFACE                                                        │
            //  ├─────────────────────────────────┬───────────┬────────────────────────────────────────────┤
            //  │ Constant                        │ Value     │ Effect                                     │
            //  ├─────────────────────────────────┼───────────┼────────────────────────────────────────────┤
            //  │ POP_BATCH_SIZE                  │  4        │ Patches per observation window              │
            //  │ POP_TYPE_AFFINITY_STRENGTH      │  3.0      │ Max spawn boost for dominant type           │
            //  │ POP_SCALE_TENDENCY_STRENGTH     │  2.0      │ Max tier proximity boost                    │
            //  │ POP_GOL_SUPPRESSION             │  0.05     │ GoL chance reduction per unit structure     │
            //  │ POP_MIN_OBSERVATIONS            │  2        │ Min entities before bias activates          │
            //  │ POP_MODE_AFFINITY_CHANCE        │  0.50     │ Probability of affinity batch               │
            //  │ POP_MODE_REPULSION_CHANCE       │  0.25     │ Probability of repulsion batch              │
            //  │ (remainder)                     │  0.25     │ Probability of neutral batch                │
            //  └─────────────────────────────────┴───────────┴────────────────────────────────────────────┘

            struct PopBatchMode {
                static constexpr uint32_t AFFINITY = 0;  // more of the same
                static constexpr uint32_t REPULSION = 1;  // opposites attract
                static constexpr uint32_t NEUTRAL = 2;  // pure independent rolls
            };

            static constexpr uint32_t POP_BATCH_SIZE = 16;
            static constexpr float POP_TYPE_AFFINITY_STRENGTH = 0.0f;
            static constexpr float POP_SCALE_TENDENCY_STRENGTH = 0.0f;
            static constexpr float POP_GOL_SUPPRESSION = 0.05f;
            static constexpr uint32_t POP_MIN_OBSERVATIONS = 1;
            static constexpr float POP_MODE_AFFINITY_CHANCE = 0.0f;
            static constexpr float POP_MODE_REPULSION_CHANCE = 0.0f;
            // remainder (1.0) = neutral

            // ── Cross-Family Affinity Matrix ──────────────────────────────────
            //
            // When family X is observed, how much does it influence family Y's
            // spawn chance? Read as: row = observed, column = target.
            // 1.0 = neutral. >1.0 = attracts. <1.0 = suppresses.
            //
            // In AFFINITY mode, values >1 boost the target.
            // In REPULSION mode, the matrix is read inverted (1/value).
            //
            //  ┌──────────────────────────────────────────────────────────────────────────┐
            //  │ CROSS-FAMILY AFFINITY              target →                              │
            //  │ observed ↓          │  Pyramid     │  Arch        │  Column              │
            //  ├──────────────────────┼──────────────┼──────────────┼──────────────────────┤
            //  │ Pyramid              │  0.5 (rare)  │  2.0 (gates) │  1.5 (colonnades)   │
            //  │ Arch                 │  0.8 (mild)  │  1.5 (chain) │  2.0 (flanking)     │
            //  │ Column               │  0.3 (supp)  │  1.2 (mild)  │  1.8 (cluster)      │
            //  └──────────────────────┴──────────────┴──────────────┴──────────────────────┘

            static constexpr float POP_CROSS_AFFINITY[PopFamily::COUNT][PopFamily::COUNT] = {
                //          target:  Pyramid  Arch    Column  Antenna  Palm    Cactus  Blade   Sphere  Ribn    Cube    GoL     Gall
                /* Pyramid */     {  0.5f,    2.0f,   1.5f,   1.5f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f },
                /* Arch    */     {  0.8f,    1.5f,   2.0f,   2.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f },
                /* Column  */     {  0.3f,    1.2f,   1.8f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f },
                /* Antenna */     {  0.3f,    1.2f,   1.0f,   1.8f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f },
                /* Palm    */     {  0.3f,    1.0f,   1.0f,   1.0f,   1.5f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f },
                /* Cactus  */     {  1.0f,    1.0f,   1.0f,   1.0f,   1.0f,   1.5f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f },
                /* Blade   */     {  1.0f,    1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f },
                /* Sphere  */     {  1.0f,    1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f },
                /* Ribn    */     {  1.0f,    1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f },
                /* Cube    */     {  1.0f,    1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f },
                /* GoL     */     {  1.0f,    1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f },
                /* Gall    */     {  1.0f,    1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f,   1.0f },
            };

            // ── Per-Tier Scale Character ──────────────────────────────────────
            //
            // Each tier declares its compositional "size character" on [0, 1].
            // 0.0 = human-scale intimate. 1.0 = monumental/colossal.
            // This replaces the linear tier_idx/(count-1) mapping.
            //
            // Used by record_population_observation to accumulate scale_sum,
            // and by select_tier_biased to compute proximity to the tendency.
            //
            //  ┌──────────────────────────────────────────────────────────┐
            //  │ TIER SCALE CHARACTER                                    │
            //  ├──────────────────────────┬─────────┬────────────────────┤
            //  │ Entity                   │ Scale   │ Character          │
            //  ├──────────────────────────┼─────────┼────────────────────┤
            //  │ Pyramid: Obelisk         │  0.35   │ tall but narrow    │
            //  │ Pyramid: Temple          │  0.60   │ moderate platform  │
            //  │ Pyramid: Colossus        │  1.00   │ massive landmark   │
            //  ├──────────────────────────┼─────────┼────────────────────┤
            //  │ Arch: Doorway            │  0.10   │ human passage      │
            //  │ Arch: Standard           │  0.55   │ medium gateway     │
            //  │ Arch: Monumental         │  0.95   │ cathedral-scale    │
            //  ├──────────────────────────┼─────────┼────────────────────┤
            //  │ Column: Pillar           │  0.15   │ squat post         │
            //  │ Column: Doric            │  0.30   │ classical human    │
            //  │ Column: Ornate           │  0.50   │ decorated medium   │
            //  │ Column: Antenna          │  0.60   │ tall with drums    │
            //  │ Column: Antenna Squat    │  0.45   │ wide + short drums │
            //  │ Column: Antenna Colossal │  0.85   │ tower-scale        │
            //  └──────────────────────────┴─────────┴────────────────────┘

            static constexpr float TIER_SCALE_PYRAMID[] = { 0.35f, 0.60f, 1.00f };
            static constexpr float TIER_SCALE_ARCH[] = { 0.10f, 0.55f, 0.95f };
            static constexpr float TIER_SCALE_COLUMN[] = { 0.15f, 0.30f, 0.50f };
            static constexpr float TIER_SCALE_ANTENNA[] = { 0.60f, 0.45f, 0.85f };
            static constexpr float TIER_SCALE_PALM[] = { 0.20f, 0.45f, 0.75f };
            static constexpr float TIER_SCALE_CACTUS[] = { 0.15f, 0.40f, 0.70f };
            static constexpr float TIER_SCALE_SPHERE[] = { 0.25f, 0.20f };
            static constexpr float TIER_SCALE_CUBE[] = { 0.15f, 0.35f, 0.60f, 0.40f };
            static constexpr float TIER_SCALE_RIBBON[] = { 0.80f, 0.50f, 0.65f };
            // GoL compound tiers: Conway 0–6, then Pulse 7–9
            static constexpr float TIER_SCALE_GOL[] = {
                0.70f, 0.30f, 0.45f, 0.55f, 0.35f, 0.80f, 0.65f,   // Conway: Pillars..Glacier
                0.40f, 0.25f, 0.50f                                   // Pulse: Breathe, Sparkle, Drift
            };
            // Gallery tiers = archetypes: 0=mountainous, 1=varied, 2=basin, 3=pool
            static constexpr float TIER_SCALE_GALLERY[] = { 0.20f, 0.40f, 0.65f, 0.75f };

            // Accessor: look up scale character by family + tier index.
            static float tier_scale_character(uint32_t family, uint32_t tier_idx) {
                switch (family) {
                case PopFamily::PYRAMID: return (tier_idx < 3) ? TIER_SCALE_PYRAMID[tier_idx] : 0.5f;
                case PopFamily::ARCH:    return (tier_idx < 3) ? TIER_SCALE_ARCH[tier_idx] : 0.5f;
                case PopFamily::COLUMN:  return (tier_idx < 3) ? TIER_SCALE_COLUMN[tier_idx] : 0.5f;
                case PopFamily::ANTENNA: return (tier_idx < 3) ? TIER_SCALE_ANTENNA[tier_idx] : 0.5f;
                case PopFamily::PALM:    return (tier_idx < 3) ? TIER_SCALE_PALM[tier_idx] : 0.5f;
                case PopFamily::CACTUS:   return (tier_idx < 3) ? TIER_SCALE_CACTUS[tier_idx] : 0.5f;
                case PopFamily::SPHERE:   return (tier_idx < 2) ? TIER_SCALE_SPHERE[tier_idx] : 0.5f;
                case PopFamily::RIBBON:   return (tier_idx < 3) ? TIER_SCALE_RIBBON[tier_idx] : 0.5f;
                case PopFamily::CUBE:     return (tier_idx < 4) ? TIER_SCALE_CUBE[tier_idx] : 0.5f;
                case PopFamily::GOL:      return (tier_idx < 10) ? TIER_SCALE_GOL[tier_idx] : 0.5f;
                case PopFamily::GALLERY:  return (tier_idx < 4) ? TIER_SCALE_GALLERY[tier_idx] : 0.5f;
                default: return 0.5f;
                }
            }

            struct PopulationBatch {
                uint32_t type_count[PopFamily::COUNT] = {};  // entities per family
                float scale_sum = 0.0f;        // sum of normalized tier positions [0,1]
                uint32_t scale_n = 0;          // number of scale observations
                uint32_t patches_elapsed = 0;  // patches since batch start
                uint32_t mode = PopBatchMode::AFFINITY;  // rolled at batch birth
            };

            PopulationBatch popBatch_{};
            uint32_t popBatchCounter_ = 0;  // global counter for deterministic mode rolls

            // ── Recording ─────────────────────────────────────────────────────
            //
            // Called inside each spawn function after successful spawn.
            // tier_idx: which tier was selected (0-based).
            // tier_count: total tiers in that family (for normalization).

            void record_population_observation(uint32_t family, uint32_t tier_idx) {
                popBatch_.type_count[family]++;
                float scale = tier_scale_character(family, tier_idx);
                popBatch_.scale_sum += scale;
                popBatch_.scale_n++;
            }

            // ── Batch Advance ─────────────────────────────────────────────────
            //
            // Called once per patch after all entity spawns complete.
            // Increments patch counter; resets batch when budget expires.
            // New batch rolls its mode from (seed, batchCounter).

            void advance_population_batch() {
                popBatch_.patches_elapsed++;
                if (popBatch_.patches_elapsed >= POP_BATCH_SIZE) {
                    popBatch_ = PopulationBatch{};
                    // Roll batch mode deterministically
                    popBatchCounter_++;
                    uint32_t mode_seed = cpu_hash(activeSeed_ ^ popBatchCounter_, 330u);
                    float mode_roll = cpu_hash_f(mode_seed, 331u);
                    if (mode_roll < POP_MODE_AFFINITY_CHANCE) {
                        popBatch_.mode = PopBatchMode::AFFINITY;
                    }
                    else if (mode_roll < POP_MODE_AFFINITY_CHANCE + POP_MODE_REPULSION_CHANCE) {
                        popBatch_.mode = PopBatchMode::REPULSION;
                    }
                    else {
                        popBatch_.mode = PopBatchMode::NEUTRAL;
                    }
                }
            }

            // ── Live Accessors (read current batch state) ─────────────────────
            //
            // Called inside spawn functions and GoL detection.
            // Bias builds as observations accumulate within the batch.
            // Before POP_MIN_OBSERVATIONS, returns neutral (1.0 / 0.0 / 0.5).
            //
            // In AFFINITY mode: types that appeared more get boosted.
            // In REPULSION mode: types that appeared LESS get boosted.
            // In NEUTRAL mode: always returns 1.0 (no bias).

            float population_type_affinity(uint32_t family) const {
                if (popBatch_.mode == PopBatchMode::NEUTRAL) return 1.0f;
                uint32_t total = popBatch_.type_count[0] + popBatch_.type_count[1] + popBatch_.type_count[2];
                if (total < POP_MIN_OBSERVATIONS) return 1.0f;
                // Weighted sum: each observed family contributes its cross-affinity to the target
                float influence = 0.0f;
                for (uint32_t obs = 0; obs < PopFamily::COUNT; obs++) {
                    float fraction = (float)popBatch_.type_count[obs] / (float)total;
                    float affinity = POP_CROSS_AFFINITY[obs][family];
                    if (popBatch_.mode == PopBatchMode::REPULSION) {
                        affinity = (affinity > 0.01f) ? (1.0f / affinity) : 10.0f;  // invert
                    }
                    influence += fraction * affinity;
                }
                return 1.0f + (influence - 1.0f) * POP_TYPE_AFFINITY_STRENGTH;
            }

            float population_scale_tendency() const {
                if (popBatch_.mode == PopBatchMode::NEUTRAL) return 0.5f;
                if (popBatch_.scale_n < POP_MIN_OBSERVATIONS) return 0.5f;
                float raw = popBatch_.scale_sum / (float)popBatch_.scale_n;
                if (popBatch_.mode == PopBatchMode::REPULSION) {
                    raw = 1.0f - raw;  // invert: small observations push toward large
                }
                return raw;
            }

            float population_automata_bias() const {
                if (popBatch_.mode == PopBatchMode::NEUTRAL) return 0.0f;
                uint32_t total = popBatch_.type_count[0] + popBatch_.type_count[1] + popBatch_.type_count[2];
                if (total < POP_MIN_OBSERVATIONS) return 0.0f;
                float avg_affinity = (population_type_affinity(0) +
                    population_type_affinity(1) +
                    population_type_affinity(2)) / 3.0f;
                return -(avg_affinity - 1.0f) * POP_GOL_SUPPRESSION;
            }

            // ── Biased Tier Selection ─────────────────────────────────────────
            //
            // Applies scale tendency to tier weights before rolling.
            // Tiers near the batch's average scale get boosted (affinity)
            // or tiers FAR from it get boosted (repulsion).
            // Falls back to unbiased select_tier in neutral mode or pre-observations.
            //
            // family: PopFamily index — needed to look up tier scale character.

            uint32_t select_tier_biased(uint32_t seed, uint32_t tier_prop,
                const float* base_weights, uint32_t count, uint32_t family) const {
                if (popBatch_.mode == PopBatchMode::NEUTRAL ||
                    popBatch_.scale_n < POP_MIN_OBSERVATIONS) {
                    return select_tier(seed, tier_prop, base_weights, count);
                }
                float tendency = population_scale_tendency();
                float weights[8];  // max tiers across all families
                float total = 0.0f;
                for (uint32_t t = 0; t < count && t < 8; t++) {
                    float scale = tier_scale_character(family, t);
                    float proximity = 1.0f - std::abs(scale - tendency);
                    weights[t] = base_weights[t] * (1.0f + proximity * POP_SCALE_TENDENCY_STRENGTH);
                    total += weights[t];
                }
                for (uint32_t t = 0; t < count; t++) weights[t] /= total;
                float roll = cpu_hash_f(seed, tier_prop);
                float cumul = 0.0f;
                for (uint32_t t = 0; t < count; t++) {
                    cumul += weights[t];
                    if (roll < cumul) return t;
                }
                return count - 1;
            }

            // ── Theme Envelope State (replaces lattice-based selection) ─────
            ThemeEnvelope themeEnvelope_{};
            uint32_t active_theme_idx_ = 0;   // set per-patch by evaluate_theme_envelope

            // ── Minimum Separation Matrix ─────────────────────────────────────
            //
            // Compositional spacing: how far apart entities of each family pair
            // must be. Checked against the footprint registry before accepting
            // any position. Footprints persist until patch eviction, so this
            // gives full spatial coverage across the entire active world.
            //
            // 0.0 = no minimum (exception — allow intimate proximity).
            // Positive = minimum world-space center-to-center distance.
            //
            // Read as: row = entity being placed, column = existing entity.
            // The check is asymmetric: placing an arch near a pyramid may have a
            // different minimum than placing a pyramid near an arch.
            //
            //  ┌──────────────────────────────────────────────────────────────────────────────┐
            //  │ MINIMUM SEPARATION — edge-to-edge gap (wu)                                    │
            //  │ placing ↓           │  Pyramid      │  Arch         │  Column                │
            //  ├──────────────────────┼───────────────┼───────────────┼────────────────────────┤
            //  │ Pyramid              │  15 (sparse)  │  10 (wide)    │   5 (spacing)          │
            //  │ Arch                 │  10 (wide)    │  20 (corridor)│  10 (spacing)          │
            //  │ Column               │   5 (spacing) │  10 (spacing) │   8 (colonnade)        │
            //  └──────────────────────┴───────────────┴───────────────┴────────────────────────┘
            //
            // Key exception: Arch→Pyramid = 0. Doorway arches (which become portals)
            // are explicitly allowed on top of pyramids. The footprint system still
            // prevents physical overlap of collision geometry — this matrix only
            // governs aesthetic spacing.

            static constexpr float MIN_SEPARATION[PopFamily::COUNT][PopFamily::COUNT] = {
                //                near:  Pyr    Arch   Col    Ant    Palm   Cact   Blad   Sph    Ribn   Cube   GoL    Gall
                /* placing Pyramid  */ { 15.0f, 10.0f,  5.0f,  5.0f,  5.0f,  5.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f },
                /* placing Arch     */ { 10.0f, 20.0f, 10.0f, 10.0f,  8.0f,  5.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f },
                /* placing Column   */ {  5.0f, 10.0f,  8.0f,  6.0f,  5.0f,  5.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f },
                /* placing Antenna  */ {  5.0f, 10.0f,  6.0f, 12.0f,  5.0f,  5.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f },
                /* placing Palm     */ {  5.0f,  8.0f,  5.0f,  5.0f,  8.0f,  5.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f },
                /* placing Cactus   */ {  5.0f,  5.0f,  5.0f,  5.0f,  5.0f,  8.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f },
                /* placing Blade    */ {  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f },
                /* placing Sphere   */ {  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 20.0f,  0.0f,  0.0f,  0.0f,  0.0f },
                /* placing Ribbon   */ {  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 40.0f,  0.0f,  0.0f,  0.0f },
                /* placing Cube     */ {  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 15.0f,  0.0f,  0.0f },
                /* placing GoL      */ { 10.0f, 10.0f,  5.0f,  5.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 60.0f,  0.0f },
                /* placing Gallery  */ { 10.0f, 10.0f,  5.0f,  5.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 10.0f, 30.0f },
            };

            // --- Tile State (what we remember about each generated tile) ----------

            struct TileState {
                uint32_t archetype = 1;      // default: varied
                float height_bias = 0.0f;
                float amp_scale = 1.0f;
                float activation_scale = 1.0f;
                float amp_momentum = 0.0f;   // signed amplitude excess, carried by terrain tokens
                float entity_density = 1.0f; // spatial density multiplier for entity spawning
                // Theme: evaluated from theme lattice at tile generation time
                float theme_spawn[PopFamily::COUNT] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f }; // blended per-family spawn multiplier
                uint32_t theme_idx = 0;      // dominant theme index (for tier bias)
            };

            // Spatial cache: keyed by (grid_x, grid_z)
            struct GridKey {
                int32_t x, z;
                bool operator==(const GridKey& o) const { return x == o.x && z == o.z; }
            };
            struct GridKeyHash {
                size_t operator()(const GridKey& k) const {
                    return (size_t)k.x * 73856093u ^ (size_t)k.z * 19349663u;
                }
            };

            std::unordered_map<GridKey, TileState, GridKeyHash> tileCache_;

            // Forgetting radius: tiles beyond this many grid cells get evicted
            static constexpr int32_t FORGET_RADIUS = (int32_t)PREGEN_RADIUS + 2;  // eviction radius (beyond pre-gen)

            void evict_distant_tiles(int32_t centerX, int32_t centerZ) {
                auto it = tileCache_.begin();
                while (it != tileCache_.end()) {
                    int32_t dx = it->first.x - centerX;
                    int32_t dz = it->first.z - centerZ;
                    if (dx < -FORGET_RADIUS || dx > FORGET_RADIUS ||
                        dz < -FORGET_RADIUS || dz > FORGET_RADIUS) {
                        it = tileCache_.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
            }

            // Build and upload GPUTileGrid from tile cache, centered on (cx, cz).
            void upload_tile_grid_now(wgpu::Queue& queue, int32_t cx, int32_t cz) {
                static constexpr int32_t TILE_PAD = 1;
                int32_t rp = (int32_t)activeRadius_ + TILE_PAD;
                uint32_t tileGridSide = 2 * (activeRadius_ + TILE_PAD) + 1;
                GPUTileGrid grid{};
                grid.origin_x = cx - rp;
                grid.origin_z = cz - rp;
                grid.side = tileGridSide;
                grid.cell_extent = PATCH_EXTENT;

                for (int32_t gz = cz - rp; gz <= cz + rp; gz++) {
                    for (int32_t gx = cx - rp; gx <= cx + rp; gx++) {
                        int32_t lx = gx - grid.origin_x;
                        int32_t lz = gz - grid.origin_z;
                        uint32_t idx = lz * tileGridSide + lx;
                        auto it = tileCache_.find({ gx, gz });
                        if (it != tileCache_.end()) {
                            grid.entries[idx].amp_scale = it->second.amp_scale;
                            grid.entries[idx].height_bias = it->second.height_bias;
                            grid.entries[idx].activation_scale = it->second.activation_scale;
                            grid.entries[idx].archetype = it->second.archetype;
                        }
                        else {
                            grid.entries[idx].amp_scale = 1.0f;
                            grid.entries[idx].height_bias = 0.0f;
                            grid.entries[idx].activation_scale = 1.0f;
                            grid.entries[idx].archetype = 1;
                        }
                    }
                }
                gpuState_.upload_tile_grid(queue, grid);
            }

            // --- Archetype Generation Rule ------------------------------------------
            //
            // Consult cached neighbors → weight archetypes → deterministic roll.
            // All thresholds and multipliers live in ArchetypeSelectionRules.
            // All per-archetype parameters live in the ARCHETYPES matrix.

            TileState generate_tile_state(int32_t gx, int32_t gz) {
                // Count neighbor archetypes
                uint32_t neighbor_counts[ARCHETYPE_COUNT] = {};
                uint32_t total_neighbors = 0;

                for (int32_t dz = -1; dz <= 1; dz++) {
                    for (int32_t dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dz == 0) continue;
                        auto it = tileCache_.find({ gx + dx, gz + dz });
                        if (it != tileCache_.end()) {
                            neighbor_counts[it->second.archetype]++;
                            total_neighbors++;
                        }
                    }
                }

                // Build selection weights from archetype base weights + neighbor influence
                float weights[ARCHETYPE_COUNT];
                for (uint32_t a = 0; a < ARCHETYPE_COUNT; a++) {
                    weights[a] = ARCHETYPES[a].base_weight;
                }

                // Pool archetype: mood-aware injection.
                // Indoor: common (flat floors are natural).
                // Outdoor: very rare (special feature).
                static constexpr uint32_t POOL_IDX = 3;
                if (MOOD_TABLE[activeMood_].indoor) {
                    weights[POOL_IDX] = 1.5f;   // ~30% of indoor tiles become pools
                }
                else {
                    weights[POOL_IDX] = 0.05f;  // ~1.5% of outdoor tiles
                }

                // ── Terrain token priors: multiply active tokens into weights ──
                for (uint32_t t = 0; t < MAX_TERRAIN_TOKENS; t++) {
                    if (!terrainTokens_[t].active) continue;
                    for (uint32_t a = 0; a < ARCHETYPE_COUNT; a++) {
                        weights[a] *= terrainTokens_[t].archetype_bias[a];
                    }
                }

                if (total_neighbors > 0) {
                    using R = ArchetypeSelectionRules;
                    for (uint32_t a = 0; a < ARCHETYPE_COUNT; a++) {
                        if (neighbor_counts[a] >= R::DOMINANT_THRESHOLD) {
                            weights[a] *= R::DOMINANT_MULTIPLIER;
                        }
                        else if (neighbor_counts[a] >= R::COMMON_THRESHOLD) {
                            weights[a] *= R::COMMON_MULTIPLIER;
                        }
                        else if (neighbor_counts[a] >= R::PRESENT_THRESHOLD) {
                            weights[a] *= R::PRESENT_MULTIPLIER;
                        }
                    }
                }

                // Normalize and roll
                float total_weight = 0.0f;
                for (uint32_t a = 0; a < ARCHETYPE_COUNT; a++) total_weight += weights[a];
                for (uint32_t a = 0; a < ARCHETYPE_COUNT; a++) weights[a] /= total_weight;

                uint32_t seed = tile_seed(activeSeed_, gx, gz);
                float roll = cpu_hash_f(seed, 300u);

                uint32_t archetype = ARCHETYPE_COUNT - 1;
                float cumulative = 0.0f;
                for (uint32_t a = 0; a < ARCHETYPE_COUNT; a++) {
                    cumulative += weights[a];
                    if (roll < cumulative) { archetype = a; break; }
                }

                // Per-tile jitter from archetype profile
                const auto& profile = ARCHETYPES[archetype];
                float amp_jitter = 1.0f + (cpu_hash_f(seed, 301u) - 0.5f) * profile.amp_jitter_range;
                float bias_jitter = (cpu_hash_f(seed, 302u) - 0.5f) * profile.bias_jitter_range;

                TileState ts;
                ts.archetype = archetype;
                ts.amp_scale = profile.amp_scale * amp_jitter;
                ts.height_bias = profile.height_bias + bias_jitter;
                ts.activation_scale = profile.activation_scale;
                ts.amp_momentum = amp_jitter - 1.0f;  // signed: positive = amplified, negative = dampened

                // ── Entity density field (coarse spatial noise) ──────────
                {
                    float patch_cx = (gx + 0.5f) * PATCH_EXTENT;
                    float patch_cz = (gz + 0.5f) * PATCH_EXTENT;
                    float dlx = patch_cx / DENSITY_LATTICE_SPACING;
                    float dlz = patch_cz / DENSITY_LATTICE_SPACING;
                    int32_t dbx = (int32_t)std::floor(dlx);
                    int32_t dbz = (int32_t)std::floor(dlz);
                    float dfx = dlx - dbx, dfz = dlz - dbz;
                    float dwx = dfx * dfx * (3.0f - 2.0f * dfx);
                    float dwz = dfz * dfz * (3.0f - 2.0f * dfz);
                    float density = 0.0f;
                    for (int dz = 0; dz <= 1; dz++) for (int dx = 0; dx <= 1; dx++) {
                        uint32_t ns = cpu_lattice_node_seed(activeSeed_, dbx + dx, dbz + dz, DENSITY_SEED_BAND);
                        float raw = cpu_hash_f(ns, 350u);
                        float shaped = std::pow(raw, DENSITY_EXPONENT);
                        float w = ((dx == 1) ? dwx : (1.0f - dwx)) * ((dz == 1) ? dwz : (1.0f - dwz));
                        density += shaped * w;
                    }
                    ts.entity_density = DENSITY_MIN + density * (DENSITY_MAX - DENSITY_MIN);
                }

                // ── Theme field (coarse compositional character) ─────────
                {
                    float patch_cx = (gx + 0.5f) * PATCH_EXTENT;
                    float patch_cz = (gz + 0.5f) * PATCH_EXTENT;
                    float tlx = patch_cx / THEME_LATTICE_SPACING;
                    float tlz = patch_cz / THEME_LATTICE_SPACING;
                    int32_t tbx = (int32_t)std::floor(tlx);
                    int32_t tbz = (int32_t)std::floor(tlz);
                    float tfx = tlx - tbx, tfz = tlz - tbz;
                    float twx = tfx * tfx * (3.0f - 2.0f * tfx);
                    float twz = tfz * tfz * (3.0f - 2.0f * tfz);

                    // Blend spawn weights across 4 lattice nodes.
                    // Track dominant node for discrete tier bias lookup.
                    float blended_spawn[PopFamily::COUNT] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
                    float blended_density = 0.0f;
                    float best_w = -1.0f;
                    uint32_t dominant_theme = 0;

                    for (int dz = 0; dz <= 1; dz++) for (int dx = 0; dx <= 1; dx++) {
                        uint32_t ns = cpu_lattice_node_seed(activeSeed_, tbx + dx, tbz + dz, THEME_SEED_BAND);
                        uint32_t tidx = select_theme_at_node(ns);
                        const auto& theme = THEMES[tidx];
                        float w = ((dx == 1) ? twx : (1.0f - twx)) * ((dz == 1) ? twz : (1.0f - twz));
                        for (uint32_t f = 0; f < PopFamily::COUNT; f++) {
                            blended_spawn[f] += theme.spawn_weight[f] * w;
                        }
                        blended_density += theme.density_mult * w;
                        if (w > best_w) { best_w = w; dominant_theme = tidx; }
                    }

                    for (uint32_t f = 0; f < PopFamily::COUNT; f++)
                        ts.theme_spawn[f] = blended_spawn[f];
                    ts.theme_idx = dominant_theme;
                    ts.entity_density *= blended_density;  // theme density stacks with spatial density
                }

                return ts;
            }

            // ─── Terrain Token Tick + Emission ───────────────────────────────
            //
            // Called ONCE per primary tile generation, NEVER for neighbor padding.
            // Decrements all active token budgets, clears expired tokens,
            // then evaluates the tile outcome for emission of a new token.

            void tick_terrain_tokens(const TileState& outcome, uint32_t seed) {
                // ── Tick existing tokens ─────────────────────────────────
                for (uint32_t t = 0; t < MAX_TERRAIN_TOKENS; t++) {
                    if (!terrainTokens_[t].active) continue;
                    if (terrainTokens_[t].budget <= 1) {
                        terrainTokens_[t].active = false;
                    }
                    else {
                        terrainTokens_[t].budget--;
                    }
                }

                // ── Emission from outcome ────────────────────────────────
                const auto& ep = TERRAIN_EMISSION[outcome.archetype];

                // Roll: does this outcome emit a token?
                // Property index 310: decorrelated from archetype roll (300-302)
                float emit_roll = cpu_hash_f(seed, 310u);
                if (emit_roll >= ep.emit_chance) return;

                // Roll: continuation or pivot?
                float pivot_roll = cpu_hash_f(seed, 311u);
                bool pivot = (pivot_roll < ep.pivot_chance);

                // Budget draw (uniform in [budget_min, budget_max])
                float budget_t = cpu_hash_f(seed, 312u);
                uint32_t budget = ep.budget_min +
                    (uint32_t)(budget_t * (float)(ep.budget_max - ep.budget_min + 1));
                budget = std::min(budget, ep.budget_max);  // clamp rounding

                // Build the token
                TerrainToken token{};
                const float* bias = pivot ? ep.pivot_bias : ep.continuation_bias;
                for (uint32_t a = 0; a < ARCHETYPE_COUNT; a++) {
                    token.archetype_bias[a] = bias[a];
                }

                // Amplitude momentum: if this patch rolled extreme, carry it
                if (std::abs(outcome.amp_momentum) > AMP_MOMENTUM_THRESHOLD) {
                    float carry = outcome.amp_momentum * AMP_MOMENTUM_CARRY;
                    if (carry > 0.0f) {
                        token.archetype_bias[0] *= (1.0f + carry);  // mountainous
                    }
                    else {
                        token.archetype_bias[2] *= (1.0f - carry);  // basin (carry is negative)
                    }
                }

                token.budget = budget;
                token.active = true;

                // ── Insert into stack ────────────────────────────────────
                // Find a free slot. If none, evict the token with lowest budget.
                uint32_t slot = MAX_TERRAIN_TOKENS;
                uint32_t min_budget = UINT32_MAX;
                uint32_t min_slot = 0;

                for (uint32_t t = 0; t < MAX_TERRAIN_TOKENS; t++) {
                    if (!terrainTokens_[t].active) { slot = t; break; }
                    if (terrainTokens_[t].budget < min_budget) {
                        min_budget = terrainTokens_[t].budget;
                        min_slot = t;
                    }
                }
                if (slot == MAX_TERRAIN_TOKENS) slot = min_slot;  // evict oldest

                terrainTokens_[slot] = token;
            }

            // --- World teardown: reset all runtime state for world transition ---

            void teardown_world(wgpu::Queue& queue) {
                // Patches + tile cache
                init_patch_system();
                lastCenterX_ = INT32_MAX;  // force full regen on next frame
                lastCenterZ_ = INT32_MAX;

                // Terrain tokens
                for (uint32_t t = 0; t < MAX_TERRAIN_TOKENS; t++) {
                    terrainTokens_[t] = TerrainToken{};
                }

                // Population batch
                popBatch_ = PopulationBatch{};
                popBatchCounter_ = 0;
                entityQueue_.clear();
                placementResults_.clear();

                // Theme envelope
                themeEnvelope_ = ThemeEnvelope{};
                active_theme_idx_ = 0;

                // Clear all entity piers (keep test rig at slots 0-2)
                for (uint32_t i = Dim::PIER_ARCH_BASE; i < Dim::PIER_TOTAL; i++) {
                    clear_pier(queue, i);
                }

                // Arches
                for (uint32_t i = 0; i < Dim::MAX_ARCH_INSTANCES; i++) {
                    activeArches_[i] = ActiveArch{};
                }
                activeArchCount_ = 0;
                portalsDirty_ = true;
                gpuState_.set_arch_index_count(0);
                // Clear all arch mesh gen param slots
                {
                    GPUArchMeshParams emptyParams{};
                    for (uint32_t i = 0; i < Dim::MAX_ARCH_INSTANCES; i++) {
                        gpuState_.upload_arch_mesh_params_slot(queue, i, emptyParams);
                    }
                    archMeshGenPending_ = true;
                }

                // Columns + Antennas
                for (uint32_t i = 0; i < Dim::MAX_COLUMN_ONLY; i++) {
                    activeColumns_[i] = ActiveColumn{};
                }
                for (uint32_t i = 0; i < Dim::MAX_ANTENNA_ONLY; i++) {
                    activeAntennas_[i] = ActiveColumn{};
                }
                activeColumnCount_ = 0;
                activeAntennaCount_ = 0;
                gpuState_.set_column_index_count(0);
                // Clear all column mesh gen param slots
                {
                    GPUColumnMeshParams emptyParams{};
                    for (uint32_t i = 0; i < Dim::MAX_COLUMN_INSTANCES; i++) {
                        gpuState_.upload_column_mesh_params_slot(queue, i, emptyParams);
                    }
                    columnMeshGenPending_ = true;
                }

                // Palms
                for (uint32_t i = 0; i < Dim::MAX_PALM_INSTANCES; i++) {
                    activePalms_[i] = ActivePalm{};
                }
                activePalmCount_ = 0;
                gpuState_.set_palm_index_count(0);
                {
                    GPUPalmMeshParams emptyParams{};
                    for (uint32_t i = 0; i < Dim::MAX_PALM_INSTANCES; i++) {
                        gpuState_.upload_palm_mesh_params_slot(queue, i, emptyParams);
                    }
                    palmMeshGenPending_ = true;
                }

                // Cacti
                for (uint32_t i = 0; i < Dim::MAX_CACTUS_INSTANCES; i++) {
                    activeCacti_[i] = ActiveCactus{};
                }
                activeCactusCount_ = 0;
                gpuState_.set_cactus_index_count(0);
                {
                    GPUCactusMeshParams emptyParams{};
                    for (uint32_t i = 0; i < Dim::MAX_CACTUS_INSTANCES; i++) {
                        gpuState_.upload_cactus_mesh_params_slot(queue, i, emptyParams);
                    }
                    cactusMeshGenPending_ = true;
                }

                // Blade clusters
                for (uint32_t i = 0; i < Dim::MAX_BLADE_INSTANCES; i++) {
                    activeBlades_[i] = ActiveBlade{};
                }
                activeBladeCount_ = 0;
                gpuState_.set_blade_index_count(0);
                {
                    GPUBladeClusterMeshParams emptyParams{};
                    for (uint32_t i = 0; i < Dim::MAX_BLADE_INSTANCES; i++) {
                        gpuState_.upload_blade_mesh_params_slot(queue, i, emptyParams);
                    }
                    bladeMeshGenPending_ = true;
                }

                // Pyramids
                for (uint32_t i = 0; i < Dim::MAX_PYRAMID_INSTANCES; i++) {
                    activePyramids_[i] = ActivePyramid{};
                }
                activePyramidCount_ = 0;
                cpuPyramids_ = GPUPyramidArray{};
                gpuState_.upload_pyramids(queue, cpuPyramids_);
                gpuState_.set_pyramid_index_count(0);
                // Clear all mesh gen param slots (inactive → degenerates on next dispatch)
                {
                    GPUPyramidMeshParams emptyParams{};
                    for (uint32_t i = 0; i < Dim::MAX_PYRAMID_INSTANCES; i++) {
                        gpuState_.upload_pyramid_mesh_params_slot(queue, i, emptyParams);
                    }
                    pyramidMeshGenPending_ = true;
                }

                // GoL zones
                for (uint32_t i = 0; i < Dim::MAX_GOL_ZONES; i++) {
                    golZones_[i] = GoLZoneState{};
                }
                golZoneCount_ = 0;
                activeZoneSlotCount_ = 0;
                pendingDeriveRequests_.count = 0;
                GPUGoLZoneArray emptyZones{};
                gpuState_.upload_zone_config(queue, emptyZones);

                // Ribbon — clear all slots
                {
                    for (uint32_t i = 0; i < MAX_RIBBON_INSTANCES; i++) {
                        activeRibbons_[i] = ActiveRibbon{};
                        ribbonStates_[i] = GPURibbonState{};
                    }
                    activeRibbonCount_ = 0;
                    renderedRibbonSlot_ = UINT32_MAX;
                    GPURibbonState empty{};
                    gpuState_.upload_ribbon(queue, empty);
                }

                // Sphere entities — clear all slots (0..MAX_SPHERE_INSTANCES-1)
                for (uint32_t i = 0; i < Dim::MAX_SPHERE_INSTANCES; i++) {
                    activeFloaters_[i] = ActiveFloater{};
                    GPUFloatingEntityState empty{};
                    gpuState_.upload_sphere_entity_slot(queue, i, empty);
                }
                activeFloaterCount_ = 0;

                // Cube entities — clear all slots (0..MAX_CUBE_INSTANCES-1)
                for (uint32_t i = 0; i < Dim::MAX_CUBE_INSTANCES; i++) {
                    activeCubes_[i] = ActiveCube{};
                    GPUFloatingEntityState empty{};
                    gpuState_.upload_cube_entity_slot(queue, i, empty);
                }
                activeCubeCount_ = 0;

                // Gallery / paintings — clear all exhibition + slots, keep staging intact
                for (uint32_t i = 0; i < MAX_GALLERIES; i++) {
                    galleryCenters_[i] = GalleryCenter{};
                }
                pendingSnapshot_.active = false;
                pendingPromotionCount_ = 0;
                wallFrameCount_ = 0;
                activePaintingCount_ = 0;
                // Clear all painting slots (CPU + GPU)
                for (uint32_t i = 0; i < Dim::PAINTING_MAX_SLOTS; i++) {
                    paintingSlots_[i] = GPUPaintingSlot{};
                }
                {
                    GPUPaintingSlot empty[Dim::PAINTING_MAX_SLOTS]{};
                    gpuState_.upload_painting_slots(queue, empty, Dim::PAINTING_MAX_SLOTS);
                }
                // Free all exhibition layers (staging persists across worlds)
                for (uint32_t i = 0; i < Dim::EXHIBITION_LAYERS; i++) exhibitionOccupied_[i] = false;
                exhibitionCount_ = 0;
                // Snapshot staging: consumed flags persist — exhibited snapshots stay consumed.
                // Only new captures (photographer overwrites) make a slot fresh again.
                // Authored staging: rotate consumed slots with fresh images from disk.
                rotate_authored_staging(queue);
                for (uint32_t i = 0; i < Dim::STAGING_LAYERS; i++) authoredStaging_[i].consumed = false;

                // Footprints
                for (uint32_t i = 0; i < MAX_FOOTPRINTS; i++) {
                    footprints_[i] = GroundFootprint{};
                }

                // Aura
                auraNeedsClear_ = true;
                auraCfgDirty_ = true;

                // Sky orbs: apply_mood re-enables + re-seeds as needed
                teardown_orbs();

                // Indoor shell
                gpuState_.set_shell_index_count(0);

                // Lights need re-upload with potentially new config
                lightsDirty_ = true;

                // Band motion: reset (apply_mood will re-initialize if needed)
                bandMotionActive_ = false;
                for (int i = 0; i < 6; i++) {
                    bandBlend_[i] = -1.0f;
                    bandBlendTarget_[i] = 0.0f;
                    bandPhaseOrigin_[i] = 0.0f;
                }

                // Musical modes: reset intensities (mask stays — circuits remain wired)
                for (uint32_t m = 0; m < MMODE_COUNT; m++) mmodeIntensity_[m] = 0.0f;
                paletteDriftTarget_ = 0.0f;
                paletteDriftDesired_ = 0.0f;
                gpuState_.set_mode_color_shift(0.0f);
                gpuState_.set_mode_checker_scatter(0.0f);
                gpuState_.set_mode_palette_drift(0.0f, 0.0f, 0.0f);
                gpuState_.set_mode_gol_scales(1.0f, 1.0f);
                for (int i = 0; i < 32; i++) pulseRing_[i] = 0.0f;
                pulseWriteIdx_ = 0;
                prevPolyphony_ = 0.0f;
                float zero_pulses[32] = {};
                gpuState_.set_pulse_data(0, zero_pulses);

                // Y correction

                // New world decides its own upload frequency policy
                gpuState_.set_config_dynamic(false);
            }

            void init_patch_system() {
                for (uint32_t i = 0; i < MAX_PATCHES; i++) {
                    freeLayerStack_[i] = MAX_PATCHES - 1 - i;
                }
                freeLayerCount_ = MAX_PATCHES;
                activePatchCount_ = 0;
                renderPatchCount_ = 0;
                lod0PatchCount_ = 0;
                allPatchCount_ = 0;
                gpuState_.config().placement_patch_count = 0;
                tileCache_.clear();
                pierCountDirty_ = true;
                groundEntriesDirty_ = true;
                patchInstancesDirty_ = true;
                placementDirty_ = true;
            }

            // Test rig piers: ramp + plateau + block at pier slots 0-2.
            // Same geometry as the old test rig solids, now as GPUPierInstance.
            void setup_test_rig_piers(wgpu::Queue queue) {
                // Ramp: height 0→3 along +X.
                GPUPierInstance ramp{};
                ramp.origin[0] = 12.0f;  ramp.origin[1] = 0.0f;
                ramp.half_size[0] = 6.5f; ramp.half_size[1] = 3.0f;
                ramp.height_near = 0.0f;  ramp.height_far = 3.0f;
                ramp.rotation = 0.0f;
                ramp.edge_blend = 0.5f;
                ramp.tier = PierTier::TEST_RIG;
                ramp.is_active = 1;
                write_pier(queue, 0, ramp);

                // Plateau: flat at height 3, overlaps ramp at x=18.
                GPUPierInstance plat{};
                plat.origin[0] = 21.0f;  plat.origin[1] = 0.0f;
                plat.half_size[0] = 3.5f; plat.half_size[1] = 3.0f;
                plat.height_near = 3.0f;  plat.height_far = 3.0f;
                plat.rotation = 0.0f;
                plat.edge_blend = 0.5f;
                plat.tier = PierTier::TEST_RIG;
                plat.is_active = 1;
                write_pier(queue, 1, plat);

                // Block: sharp edges → step-height walls (impassable).
                GPUPierInstance block{};
                block.origin[0] = 21.0f;  block.origin[1] = 0.0f;
                block.half_size[0] = 1.2f; block.half_size[1] = 1.2f;
                block.height_near = 5.0f;  block.height_far = 5.0f;
                block.rotation = 0.0f;
                block.edge_blend = 0.0f;
                block.tier = PierTier::TEST_RIG;
                block.is_active = 1;
                write_pier(queue, 2, block);
            }

            // Batch-generate patches into the caller's command encoder.
            // Two-pass heightfield: pass 1 evaluates ground_formed_with_complexity
            // (POLICY_BAKED_HEIGHTFIELD contributor set, fused with the complexity
            // byproduct) per texel, pass 2 reads neighbors for gradients +
            // evaluates complexity.
            // Compute pass boundary between them provides the storage texture barrier.
            // stagingOffset: slot index into the staging buffer, so multiple
            // batches per frame don't overwrite each other's params.
            void generate_patch_batch(wgpu::CommandEncoder& encoder, wgpu::Queue& queue,
                const GPUPatchParams* params, uint32_t count,
                uint32_t stagingOffset = 0) {
                if (count == 0) return;

                // One WriteBuffer: all params into staging at the given offset
                gpuState_.upload_patch_staging(queue, params, count, stagingOffset);

                for (uint32_t i = 0; i < count; i++) {
                    // Copy this patch's params from staging slot → active params buffer
                    encoder.CopyBufferToBuffer(
                        gpuState_.patch_staging_buffer(), (stagingOffset + i) * sizeof(GPUPatchParams),
                        gpuState_.patch_params_buffer(), 0,
                        sizeof(GPUPatchParams));

                    // Pass 1: heights only (one ground_formed_with_complexity per texel)
                    {
                        wgpu::ComputePassDescriptor cpd{};
                        cpd.label = "Patch Heights (pass 1)";
                        wgpu::ComputePassEncoder cp = encoder.BeginComputePass(&cpd);
                        renderer_.dispatch_generate_patch_heights(cp, gpuState_.patch_gen_group(), GPUState::patch_heightfield_workgroups());
                        cp.End();
                    }

                    // Pass boundary: storage texture write → read barrier

                    // Pass 2: gradients from neighbor reads + complexity + cell colors
                    {
                        wgpu::ComputePassDescriptor cpd{};
                        cpd.label = "Patch Gradients + Cells (pass 2)";
                        wgpu::ComputePassEncoder cp = encoder.BeginComputePass(&cpd);
                        renderer_.dispatch_generate_patch_gradients(cp, gpuState_.patch_gen_group(), GPUState::patch_heightfield_workgroups());
                        renderer_.dispatch_generate_patch_cells(cp, gpuState_.patch_gen_group(), GPUState::patch_cell_workgroups());
                        cp.End();
                    }
                }
            }

            GPUPatchParams make_patch_params(int32_t gx, int32_t gz, uint32_t layer) const {
                GPUPatchParams p{};
                p.origin[0] = (gx + 0.5f) * PATCH_EXTENT;
                p.origin[1] = (gz + 0.5f) * PATCH_EXTENT;
                p.extent = PATCH_EXTENT;
                p.resolution = 256;
                p.master_seed = activeSeed_;
                p.time = 0.0f;
                p.layer = layer;
                p._pad1 = 0.0f;
                return p;
            }

            uint32_t alloc_layer() {
                if (freeLayerCount_ == 0) {
                    // Safety: no free layers — recycle layer 0 rather than crash.
                    // This shouldn't happen if eviction works correctly.
                    return 0;
                }
                return freeLayerStack_[--freeLayerCount_];
            }

            void free_layer(uint32_t layer) {
                freeLayerStack_[freeLayerCount_++] = layer;
            }

            // Check if grid coordinate is within the allocation window (activeRadius_ = PREGEN_RADIUS)
            bool in_render_window(int32_t gx, int32_t gz, int32_t cx, int32_t cz) {
                int32_t r = (int32_t)activeRadius_;
                return gx >= cx - r && gx <= cx + r &&
                    gz >= cz - r && gz <= cz + r;
            }

            // Check if grid coordinate is within the VISIBLE circle (Euclidean).
            // Radius 5.5 in grid units: inscribes cleanly within the PREGEN square,
            // drops ~24 corner patches that would be deep in fog anyway.
            // Pre-gen patches outside this circle are allocated and generated but NOT rendered.
            static constexpr float VISIBLE_RADIUS = 5.5f;
            static constexpr float VISIBLE_RADIUS_SQ = VISIBLE_RADIUS * VISIBLE_RADIUS;

            // Multi-LOD distance bands (grid units, Euclidean from center).
            // LOD-0 (full 64×64 mesh): patches within LOD_FULL_RADIUS
            // LOD-1 (half 32×32 mesh): patches between LOD_FULL_RADIUS and VISIBLE_RADIUS
            static constexpr float LOD_FULL_RADIUS = 3.5f;
            static constexpr float LOD_FULL_RADIUS_SQ = LOD_FULL_RADIUS * LOD_FULL_RADIUS;

            // ─── Visibility Cylinder ─────────────────────────────────────
            //
            // World-space cylinder centered on the pawn's actual position.
            // Patches enter the draw list when their nearest edge crosses
            // inside the cylinder — one at a time as the pawn moves,
            // not in batches when a grid boundary is crossed.
            //
            // Grid-based allocation/eviction is unchanged; only the
            // draw-list gate uses world-space distance.
            static constexpr float VISIBILITY_CYLINDER_RADIUS = VISIBLE_RADIUS * PATCH_EXTENT;
            static constexpr float VISIBILITY_CYLINDER_RADIUS_SQ = VISIBILITY_CYLINDER_RADIUS * VISIBILITY_CYLINDER_RADIUS;
            static constexpr float LOD0_CYLINDER_RADIUS = LOD_FULL_RADIUS * PATCH_EXTENT;
            static constexpr float LOD0_CYLINDER_RADIUS_SQ = LOD0_CYLINDER_RADIUS * LOD0_CYLINDER_RADIUS;

            // Distance² from point (px,pz) to nearest edge of a patch AABB.
            // Zero when the point is inside the patch.
            static float patch_distance_sq(float px, float pz,
                float origin_x, float origin_z, float half) {
                float dx = std::max(0.0f, std::abs(px - origin_x) - half);
                float dz = std::max(0.0f, std::abs(pz - origin_z) - half);
                return dx * dx + dz * dz;
            }

            // ── Distance-sorted patch scan helper ──────────────────────────

            struct PatchCandidate {
                uint32_t idx;
                float dist2;
            };

            template<typename Pred>
            uint32_t collect_sorted_patches(
                PatchCandidate* out,
                float pawn_wx, float pawn_wz,
                Pred&& pred,
                bool nearest_first) const
            {
                float half = PATCH_EXTENT * 0.5f;
                uint32_t count = 0;
                for (uint32_t i = 0; i < activePatchCount_; i++) {
                    if (!patches_[i].valid) continue;
                    if (!pred(patches_[i])) continue;
                    float ox = (patches_[i].grid_x + 0.5f) * PATCH_EXTENT;
                    float oz = (patches_[i].grid_z + 0.5f) * PATCH_EXTENT;
                    float d2 = patch_distance_sq(pawn_wx, pawn_wz, ox, oz, half);
                    out[count++] = { i, d2 };
                }
                for (uint32_t i = 1; i < count; i++) {
                    PatchCandidate key = out[i];
                    uint32_t j = i;
                    if (nearest_first) {
                        while (j > 0 && out[j - 1].dist2 > key.dist2) {
                            out[j] = out[j - 1]; j--;
                        }
                    }
                    else {
                        while (j > 0 && out[j - 1].dist2 < key.dist2) {
                            out[j] = out[j - 1]; j--;
                        }
                    }
                    out[j] = key;
                }
                return count;
            }

            // Check if grid coordinate is within the priority window (GRID_RADIUS)
            bool in_priority_window(int32_t gx, int32_t gz, int32_t cx, int32_t cz) {
                int32_t r = (int32_t)GRID_RADIUS;
                return gx >= cx - r && gx <= cx + r &&
                    gz >= cz - r && gz <= cz + r;
            }

            // Process entity spawn for pre-collected patch candidates.
            void spawn_selected_patches(
                const PatchCandidate* candidates, uint32_t count,
                wgpu::Queue& queue)
            {
                for (uint32_t s = 0; s < count; s++) {
                    uint32_t pi = candidates[s].idx;
                    active_theme_idx_ = evaluate_theme_envelope(
                        tile_seed(activeSeed_, patches_[pi].grid_x, patches_[pi].grid_z));
                    select_entities_for_patch(patches_[pi].grid_x, patches_[pi].grid_z);
                    advance_population_batch();
                    patches_[pi].phase = PatchPhase::SPAWNED;
                }
                place_entity_queue();
                commit_entity_queue(queue);

                // Late registration: check if newly-spawned patches contain
                // an active ribbon's unregistered tip. This closes the gap
                // when a ribbon's far tip is beyond the streaming window at
                // spawn time but comes into range as the pawn approaches.
                for (uint32_t s = 0; s < count; s++) {
                    uint32_t pi = candidates[s].idx;
                    int32_t gx = patches_[pi].grid_x;
                    int32_t gz = patches_[pi].grid_z;
                    for (uint32_t r = 0; r < MAX_RIBBON_INSTANCES; r++) {
                        auto& ar = activeRibbons_[r];
                        if (!ar.active) continue;
                        // Check near tip
                        if (!ar.near_tip_registered &&
                            ar.near_tip_gx == gx && ar.near_tip_gz == gz) {
                            patches_[pi].record_entity(PopFamily::RIBBON, r);
                            ar.near_tip_registered = true;
                            ar.ref_count++;
                        }
                        // Check far tip
                        if (!ar.far_tip_registered &&
                            ar.far_tip_gx == gx && ar.far_tip_gz == gz) {
                            patches_[pi].record_entity(PopFamily::RIBBON, r);
                            ar.far_tip_registered = true;
                            ar.ref_count++;
                        }
                    }
                }
            }

            // Hook: fires once when a patch transitions SPAWNED → GENERATED.
            void on_patch_first_generated(uint32_t pi, wgpu::Queue& queue) {
                // Galleries → entity pipeline (select_gallery_for_patch)
                // GoL zones → entity pipeline (select_gol_for_patch)
                (void)pi; (void)queue;
            }

            // Process heightfield generation for pre-collected patch candidates.
            void generate_selected_patches(
                const PatchCandidate* candidates, uint32_t count,
                wgpu::CommandEncoder& encoder, wgpu::Queue& queue,
                uint32_t& patchStagingOffset, bool& tileGridDirty)
            {
                if (count == 0) return;
                if (tileGridDirty) {
                    upload_tile_grid_now(queue, lastCenterX_, lastCenterZ_);
                    tileGridDirty = false;
                }
                GPUPatchParams batchParams[MAX_PATCHES];
                uint32_t batchIdx[MAX_PATCHES];
                for (uint32_t i = 0; i < count; i++) {
                    uint32_t pi = candidates[i].idx;
                    batchParams[i] = make_patch_params(
                        patches_[pi].grid_x, patches_[pi].grid_z, patches_[pi].layer);
                    batchIdx[i] = pi;
                }
                generate_patch_batch(encoder, queue, batchParams, count, patchStagingOffset);
                patchStagingOffset += count;
                for (uint32_t b = 0; b < count; b++) {
                    uint32_t pi = batchIdx[b];
                    bool first_gen = (patches_[pi].phase == PatchPhase::SPAWNED);
                    patches_[pi].phase = PatchPhase::GENERATED;
                    if (first_gen) {
                        on_patch_first_generated(pi, queue);
                    }
                }
                patchInstancesDirty_ = true;
            }

        public:
            Cartridge() = default;

            Cartridge(const Cartridge&) = delete;
            Cartridge& operator=(const Cartridge&) = delete;

            void initialize(wgpu::Device device) override {
                device_ = device;
                auto tGpu0 = std::chrono::high_resolution_clock::now();
                gpuState_.init(device);
                auto tGpu1 = std::chrono::high_resolution_clock::now();
                std::cout << "[Cartridge] GPUState init:    "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(tGpu1 - tGpu0).count()
                    << " ms\n";
            }

            bool init_renderer(
                wgpu::TextureFormat colorFormat,
                wgpu::TextureFormat depthFormat
            ) {
                colorFormat_ = colorFormat;
                depthFormat_ = depthFormat;

                auto t0 = std::chrono::high_resolution_clock::now();
                if (!renderer_.init(
                    device_,
                    gpuState_,
                    colorFormat,
                    depthFormat
                )) return false;

                // Create offscreen textures with the actual swapchain format
                if (!gpuState_.initOffscreenResources(colorFormat)) {
                    std::cerr << "[Cartridge] Failed to init offscreen resources\n";
                    return false;
                }

                auto t1 = std::chrono::high_resolution_clock::now();

                // --- One-shot: generate terrain index buffer on GPU -----------------
                {
                    wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
                    wgpu::ComputePassDescriptor desc{};
                    desc.label = "Terrain Index Gen (one-shot)";
                    wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&desc);
                    renderer_.dispatch_generate_terrain_indices(
                        pass,
                        gpuState_.terrain_index_gen_group(),
                        GPUState::terrain_mesh_workgroups()
                    );
                    pass.End();
                    wgpu::CommandBuffer cmd = encoder.Finish();
                    device_.GetQueue().Submit(1, &cmd);
                }
                auto t2 = std::chrono::high_resolution_clock::now();

                init_patch_system();
                setup_test_rig_piers(device_.GetQueue());

                // Sky orbs for the initial mood (apply_mood runs only on transitions).
                {
                    wgpu::Queue q = device_.GetQueue();
                    configure_orbs(ORB_MOOD_TABLE[activeMood_], q);
                }

                // Initial agent population for boot mood. Slot 0 (player) is
                // already live on the GPU via GPUState's init; this populates
                // slots 1..MAX_AGENTS-1 from AGENT_POPULATIONS[activeMood_].
                // Mirror the player's idle pose into cpuAgents_[0] first so
                // the full-buffer upload is idempotent.
                {
                    cpuAgents_[0].pos_x       = Idle::PAWN_POS_X;
                    cpuAgents_[0].pos_y       = Idle::PAWN_POS_Y;
                    cpuAgents_[0].pos_z       = Idle::PAWN_POS_Z;
                    cpuAgents_[0].heading     = Idle::PAWN_HEADING;
                    cpuAgents_[0].orient_w    = 1.0f;
                    cpuAgents_[0].is_active   = 1u;
                    cpuAgents_[0].behavior_id = AGENT_BEHAVIOR_PLAYER_CONTROLLED;
                    cpuAgents_[0].tier_idx    = AGENT_TIER_WORKER;
                    cpuAgents_[0].portal_trigger = -1;

                    wgpu::Queue q = device_.GetQueue();
                    spawn_population_for_mood(activeMood_, activeSeed_,
                                              Idle::PAWN_POS_X, Idle::PAWN_POS_Z, q);
                }

                // Eager-load authored paintings at boot (avoids mid-frame stall on first gallery)
                {
                    wgpu::Queue q = device_.GetQueue();
                    load_authored_textures(q);
                }

                auto t3 = std::chrono::high_resolution_clock::now();

                std::cout << "[Cartridge] Renderer init:    "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms\n";
                std::cout << "[Cartridge] Terrain gen:      "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " ms\n";
                std::cout << "[Cartridge] Patch system:     "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count() << " ms\n";
                std::cout << "[Cartridge] Total init:       "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t0).count() << " ms\n";

                return true;
            }

            void update(const AnalysisSignal& signal,
                float aspect_ratio,
                wgpu::Queue& queue) override {
                // --- Build GPU signal from analysis + input -------------------------
                GPUFrameSignal gpuSignal;

                gpuSignal.t_seconds = signal.t_seconds;
                gpuSignal.t_beats = signal.t_beats;
                gpuSignal.dt = signal.dt;
                gpuSignal.aspect_ratio = aspect_ratio;

                for (size_t i = 0; i < signal.stats.size(); ++i) {
                    gpuSignal.stats[i] = signal.stats[i];
                }

                gpuSignal.move_x = inputState_.move_x;
                gpuSignal.move_z = inputState_.move_z;
                gpuSignal.look_az_delta = inputState_.look_az_delta;
                gpuSignal.look_el_delta = inputState_.look_el_delta;
                gpuSignal.zoom_delta = inputState_.zoom_delta;
                gpuSignal.pan_x_delta = inputState_.pan_x_delta;
                gpuSignal.pan_y_delta = inputState_.pan_y_delta;
                gpuSignal._pad1 = 0.0f;

                currentBeats_ = signal.t_beats;
                currentSeconds_ = signal.t_seconds;
                currentDt_ = signal.dt;

                // --- Upload to GPU --------------------------------------------------

                // Aura presence trajectory: smooth ramp on enable/disable
                {
                    float target = auraEnabled_ ? 1.0f : 0.0f;
                    float rate = (target > auraPresence_) ? AURA_PRESENCE_ATTACK : AURA_PRESENCE_RELEASE;
                    float prev = auraPresence_;
                    auraPresence_ = prev + (target - prev) * (1.0f - std::exp(-rate * currentDt_));
                    if (auraPresence_ < 0.001f && target == 0.0f) auraPresence_ = 0.0f;
                    if (auraPresence_ > 0.999f && target == 1.0f) auraPresence_ = 1.0f;
                    if (auraPresence_ != prev) auraCfgDirty_ = true;
                }

                // Pawn aura height: presence × base height × expansion
                // This same value is used by terrain VS for extrusion, so pawn and terrain always agree.
                float aura_expand_mult = 1.0f + mmodeIntensity_[MMODE_AURA_EXPAND] * 3.0f;
                float effective_aura_height = auraHeightEnabled_
                    ? activeAuraProfile_.height_scale * auraPresence_ * aura_expand_mult : 0.0f;
                gpuState_.set_pawn_aura_height(effective_aura_height);
                gpuState_.set_aura_enabled(auraPresence_ > 0.001f);  // keep compute running while ramping down
                gpuState_.set_world_seed(activeSeed_);
                if (finiteMode_) {
                    float bmin = -(float)finiteRadius_ * PATCH_EXTENT;
                    float bmax = ((float)finiteRadius_ + 1.0f) * PATCH_EXTENT;
                    gpuState_.set_world_bounds(bmin, bmin, bmax, bmax);
                }
                else {
                    gpuState_.set_world_bounds(0.0f, 0.0f, 0.0f, 0.0f);
                }

                // --- Transition state machine ---
                if (transitionPhase_ != TransitionPhase::IDLE) {
                    transitionTimer_ += signal.dt;
                    switch (transitionPhase_) {
                    case TransitionPhase::FADE_OUT:
                        transitionFadeAlpha_ = std::min(1.0f, transitionTimer_ / transitionFadeDuration_);
                        if (transitionFadeAlpha_ >= 1.0f) {
                            transitionPhase_ = TransitionPhase::TEARDOWN;
                        }
                        break;
                    case TransitionPhase::TEARDOWN:
                    {
                        // Capture return seed + mood + radius before overwrite
                        backPortalReturnSeed_ = activeSeed_;
                        backPortalReturnMood_ = activeMood_;
                        backPortalReturnRadius_ = finiteRadius_;

                        activeSeed_ = pendingDestination_.seed;
                        finiteMode_ = pendingDestination_.finite;
                        finiteRadius_ = pendingDestination_.finite_radius;
                        teardown_world(queue);
                        // NOTE: do NOT force pawnReadbackState_ to IDLE here.
                        // If a MapAsync is in-flight (MAPPING), forcing IDLE would
                        // cause CopyBufferToBuffer to a still-mapped buffer.
                        // The existing state machine guards will skip readback
                        // until the pending callback resolves naturally.
                        readbackPortalTrigger_ = -1;
                        pawnReadback_x_ = 0.0f;
                        pawnReadback_z_ = 0.0f;
                        gpuState_.reset_player_agent(queue);
                        gpuState_.set_possessed_slot(0);
                        // Keep cpuAgents_ in sync with the GPU reset so
                        // patch streaming + ribbon + Caps Lock see current state.
                        std::memset(cpuAgents_, 0, sizeof(cpuAgents_));
                        cpuAgents_[0].pos_x       = 0.0f;  // Idle::PAWN_POS_X
                        cpuAgents_[0].pos_y       = 0.0f;
                        cpuAgents_[0].pos_z       = 0.0f;
                        cpuAgents_[0].orient_w    = 1.0f;
                        cpuAgents_[0].is_active   = 1u;
                        cpuAgents_[0].behavior_id = AGENT_BEHAVIOR_PLAYER_CONTROLLED;
                        cpuAgents_[0].tier_idx    = AGENT_TIER_WORKER;
                        cpuAgents_[0].portal_trigger = -1;
                        player_.possessed_slot = 0;
                        gpuState_.set_world_seed(activeSeed_);
                        apply_mood(pendingDestination_.mood, queue);
                        spawn_population_for_mood(pendingDestination_.mood, activeSeed_,
                                                  Idle::PAWN_POS_X, Idle::PAWN_POS_Z, queue);
                        // Deactivate ribbons in finite mode (mood 5 spawns its own in apply_mood)
                        if (finiteMode_ && activeRibbonCount_ > 0 && activeMood_ != 5) {
                            for (uint32_t i = 0; i < MAX_RIBBON_INSTANCES; i++) {
                                activeRibbons_[i] = ActiveRibbon{};
                                ribbonStates_[i] = GPURibbonState{};
                            }
                            activeRibbonCount_ = 0;
                            renderedRibbonSlot_ = UINT32_MAX;
                            GPURibbonState empty{};
                            gpuState_.upload_ribbon(queue, empty);
                        }
                        // Schedule guaranteed back-portal in finite worlds
                        backPortalPending_ = finiteMode_;

                        transitionPhase_ = TransitionPhase::FADE_IN;
                        transitionTimer_ = 0.0f;
                        uint32_t side = finiteMode_ ? 2 * finiteRadius_ + 1 : 0;
                        std::cout << "[World] Teardown complete, seed=" << activeSeed_
                            << " mode=" << (finiteMode_ ? "finite" : "open")
                            << (finiteMode_ ? " " + std::to_string(side) + "x" + std::to_string(side) : "")
                            << "\n";
                    }
                    break;
                    case TransitionPhase::FADE_IN:
                        transitionFadeAlpha_ = std::max(0.0f, 1.0f - transitionTimer_ / transitionFadeDuration_);
                        if (transitionFadeAlpha_ <= 0.0f) {
                            transitionPhase_ = TransitionPhase::IDLE;
                            transitionFadeAlpha_ = 0.0f;
                        }
                        break;
                    default: break;
                    }
                }
                gpuState_.set_fade(transitionFadeAlpha_, 0.0f, 0.0f, 0.0f);

                gpuState_.upload_signal(queue, gpuSignal);

                // ─── Polyphony-driven band motion ────────────────────────
                if (bandMotionActive_) {
                    float polyphony = signal.stats[0];
                    uint32_t active_count = (uint32_t)std::max(0.0f, std::min(polyphony, 6.0f));

                    // Set per-band targets: bands activate in order from fine to tectonic
                    for (uint32_t i = 0; i < 6; i++) bandBlendTarget_[i] = 0.0f;
                    for (uint32_t i = 0; i < active_count; i++) {
                        bandBlendTarget_[BAND_ACTIVATION_ORDER[i]] = 1.0f;
                    }

                    float dt = signal.dt;
                    bool changed = false;
                    for (uint32_t i = 0; i < 6; i++) {
                        float prev = bandBlend_[i];
                        float target = bandBlendTarget_[i];

                        // Capture phase origin at the moment a band activates
                        if (target > 0.5f && prev < 0.01f) {
                            bandPhaseOrigin_[i] = currentBeats_;
                        }

                        // Exponential ramp toward target
                        float rate = (target > prev) ? BAND_BLEND_ATTACK : BAND_BLEND_RELEASE;
                        bandBlend_[i] = prev + (target - prev) * (1.0f - std::exp(-rate * dt));

                        // Snap to endpoints to avoid perpetual drift
                        if (bandBlend_[i] < 0.001f && target == 0.0f) bandBlend_[i] = 0.0f;
                        if (bandBlend_[i] > 0.999f && target == 1.0f) bandBlend_[i] = 1.0f;

                        if (bandBlend_[i] != prev) changed = true;
                    }

                    if (changed) {
                        gpuState_.set_band_motion(bandBlend_, bandPhaseOrigin_);
                    }
                    gpuState_.set_terrain_time(currentBeats_);
                }

                // ─── Musical animation modes: per-frame intensity ramp ───
                {
                    float polyphony = signal.stats[0];
                    float dt = signal.dt;
                    bool any_changed = false;

                    for (uint32_t m = 0; m < MMODE_COUNT; m++) {
                        // Skip mode 0 (terrain waves) — handled by band motion system above
                        // Skip mode 3 (palette drift) — has its own steeper intensity curve below
                        if (m == MMODE_TERRAIN_WAVES || m == MMODE_PALETTE_DRIFT) continue;

                        bool on = is_mmode_on(m);
                        float target = on ? std::min(polyphony / 6.0f, 1.0f) : 0.0f;
                        float prev = mmodeIntensity_[m];
                        float rate = (target > prev) ? MMODE_ATTACK : MMODE_RELEASE;
                        float next = prev + (target - prev) * (1.0f - std::exp(-rate * dt));

                        // Snap to endpoints
                        if (next < 0.001f && target == 0.0f) next = 0.0f;
                        if (next > 0.999f && target >= 1.0f) next = 1.0f;

                        if (next != prev) {
                            mmodeIntensity_[m] = next;
                            any_changed = true;
                        }
                    }

                    if (any_changed) {
                        // Color shift: intensity → mode field bias
                        gpuState_.set_mode_color_shift(mmodeIntensity_[MMODE_COLOR_SHIFT] * 0.6f);

                        // Checker scatter: intensity → sparse threshold reduction
                        gpuState_.set_mode_checker_scatter(mmodeIntensity_[MMODE_CHECKER_SCATTER] * 0.5f);

                        // Aura expand: intensity scales aura parameters
                        if (mmodeIntensity_[MMODE_AURA_EXPAND] > 0.0f || is_mmode_on(MMODE_AURA_EXPAND)) {
                            auraCfgDirty_ = true;
                        }

                        // GoL tempo: intensity → tick slow-down + height boost
                        // Inverse: more polyphony = slower GoL (contemplation).
                        // When BPM detection arrives, this source gets swapped.
                        {
                            float gi = mmodeIntensity_[MMODE_GOL_TEMPO];
                            // tick_scale > 1 = slower. Lerp from 1.0 up to 4.0 (4× slower at full)
                            float tick_scale = 1.0f + gi * 3.0f;
                            // height_scale > 1 = taller. Lerp from 1.0 up to 3.0
                            float height_scale = 1.0f + gi * 2.0f;
                            gpuState_.set_mode_gol_scales(tick_scale, height_scale);
                        }
                    }

                    // Palette drift: smooth target transition + push to GPU
                    // Uses its own intensity curve — steeper than generic mmodeIntensity
                    // because palette colors are close and need strong push to read.
                    {
                        float poly = signal.stats[0];
                        bool drift_on = is_mmode_on(MMODE_PALETTE_DRIFT);

                        // Smooth palette mapping: ordered by contrast from sand baseline.
                        //   1 note → green(2)  — biggest hue shift
                        //   2 notes → grey(3)   — desaturated, clearly different
                        //   3+ notes → salmon(1) — warm shift, completes cycle
                        static constexpr float SMOOTH_PALETTE_MAP[] = { 0.0f, 2.0f, 3.0f, 1.0f };

                        // Discrete tier mapping: cycle through all vocabularies.
                        //   Idle   → whatever the threshold cascade gives (natural)
                        //   1 note → tinted mono(1)    — desaturated, grey-tinted cells
                        //   2 notes → chess colorful(4) — parity + vivid per-node colors
                        //   3 notes → pure B&W(2)       — high contrast random assignment
                        //   4+ notes → chess B&W(3)     — structured classic pattern
                        // Full color(0) is the natural idle state for most cells,
                        // so it's not a useful drift target — already there.
                        static constexpr float DISCRETE_TIER_MAP[] = { 0.0f, 1.0f, 4.0f, 2.0f, 3.0f };

                        if (drift_on && poly >= 1.0f) {
                            uint32_t idx = std::min((uint32_t)poly, 3u);
                            paletteDriftDesired_ = SMOOTH_PALETTE_MAP[idx];
                        }
                        if (!drift_on || poly < 0.5f) {
                            paletteDriftDesired_ = 0.0f;
                        }

                        // Ramp target smoothly to avoid color snaps
                        float prev_t = paletteDriftTarget_;
                        paletteDriftTarget_ += (paletteDriftDesired_ - paletteDriftTarget_)
                            * (1.0f - std::exp(-PALETTE_DRIFT_TARGET_RATE * dt));

                        // Intensity: poly/3 so single note is partial, 3 notes = full
                        float drift_intensity = drift_on
                            ? std::min(poly / 3.0f, 1.0f) : 0.0f;
                        // Use same exponential ramp as other modes for smooth on/off
                        float prev_i = mmodeIntensity_[MMODE_PALETTE_DRIFT];
                        float rate_i = (drift_intensity > prev_i) ? MMODE_ATTACK : MMODE_RELEASE;
                        mmodeIntensity_[MMODE_PALETTE_DRIFT] = prev_i
                            + (drift_intensity - prev_i) * (1.0f - std::exp(-rate_i * dt));
                        float intensity = mmodeIntensity_[MMODE_PALETTE_DRIFT];

                        // Discrete tier from lookup
                        float discrete_tier = 0.0f;
                        if (drift_on && poly >= 1.0f) {
                            uint32_t tidx = std::min((uint32_t)poly, 4u);
                            discrete_tier = DISCRETE_TIER_MAP[tidx];
                        }

                        if (intensity > 0.001f || paletteDriftTarget_ != prev_t) {
                            gpuState_.set_mode_palette_drift(paletteDriftTarget_, intensity, discrete_tier);
                        }
                    }
                }

                // ─── Radial pulse onset detection ────────────────────────
                {
                    float poly = signal.stats[0];
                    bool pulse_on = is_mmode_on(MMODE_RADIAL_PULSE);

                    // Detect note onsets: polyphony increased since last frame
                    if (pulse_on && poly > prevPolyphony_ + 0.5f) {
                        float increase = poly - std::max(prevPolyphony_, 0.0f);
                        // Emit one pulse per onset, amplitude proportional to note count
                        uint32_t slot = pulseWriteIdx_ % PULSE_RING_SIZE;
                        uint32_t base = slot * 4;
                        pulseRing_[base + 0] = pawnReadback_x_;      // origin X
                        pulseRing_[base + 1] = pawnReadback_z_;      // origin Z
                        pulseRing_[base + 2] = currentSeconds_;      // onset time
                        pulseRing_[base + 3] = PULSE_AMPLITUDE * std::min(increase, 3.0f);
                        pulseWriteIdx_++;
                        std::cout << "[Pulse] ONSET slot=" << slot
                            << " pos=(" << pawnReadback_x_ << "," << pawnReadback_z_ << ")"
                            << " t=" << currentSeconds_
                            << " amp=" << pulseRing_[base + 3]
                            << " poly=" << poly << " prev=" << prevPolyphony_
                            << "\n";
                    }
                    prevPolyphony_ = poly;

                    // Count active (non-expired) pulses and upload
                    uint32_t active = 0;
                    for (uint32_t i = 0; i < PULSE_RING_SIZE; i++) {
                        float onset = pulseRing_[i * 4 + 2];
                        float amp = pulseRing_[i * 4 + 3];
                        if (amp > 0.001f && (currentSeconds_ - onset) < PULSE_MAX_AGE) {
                            active = std::max(active, i + 1);
                        }
                    }
                    // Always upload if any pulses exist (even decaying ones for GPU to evaluate)
                    gpuState_.set_pulse_data(active, pulseRing_);
                }

                gpuState_.upload_config(queue);

                // Orb musical coupling: polyphony → radial expansion.
                // Always-on when orbs are active; future coupling grammar
                // will put this behind a gate.
                update_orb_coupling(signal.stats[0], signal.dt, queue);

                // Orb dome anchor: follow pawn when toggled on. Uses
                // last-frame pawn readback — one-frame lag is imperceptible.
                update_orb_anchor(pawnReadback_x_, pawnReadback_z_, queue);

                // Pawn position comes from GPU readback (one-frame latency).
                // See render() for the readback state machine.

                // --- Clear deltas for next frame ------------------------------------
                update_photographer(queue);
                clear_input_deltas();
            }

            // ORDER (STREAMING PATCH MODE):
            //   1. (Optional) Compute: compute_ribbon_rings   [0D] -- ring transforms for flying ribbon
            //   2. Compute: update_world                      [0D] -- entities, trajectories, couplings
            //   3. Compute: compute_vp                        [0D] -- camera VP + sun VP (shadow)
            //   4. Render:  patch terrain instances           -- heightfield array sampled in VS
            //   5. Render:  pawn entity                       -- chess pawn
            //   6. Render:  sphere entity                     -- sphere
            //   7. Render:  ribbon rings                      -- instanced ring geometry

            void render(wgpu::CommandEncoder& encoder,
                wgpu::TextureView backbuffer,
                wgpu::TextureView depth) override {

                wgpu::Queue queue = device_.GetQueue();

                // --- GPU agent buffer readback (one-frame latency) ---
                // Copies the full agent_state array (MAX_AGENTS × 80 bytes)
                // to staging each frame after compute. CPU mirror is used
                // for patch streaming / ribbon / photographer (possessed
                // slot's XZ) and for Caps Lock nearest-agent targeting in
                // Step 7. Portal triggers surface from the possessed slot's
                // portal_trigger field.
                //
                // State machine: IDLE → copy agent buffer to staging → COPIED
                //                COPIED → call MapAsync → MAPPING
                //                MAPPING → callback fires, reads data → IDLE
                if (pawnReadbackState_ == PawnReadbackState::COPIED) {
                    pawnReadbackState_ = PawnReadbackState::MAPPING;
                    gpuState_.agent_state_readback_staging().MapAsync(
                        wgpu::MapMode::Read, 0, GPUState::agent_state_buffer_size(),
                        wgpu::CallbackMode::AllowSpontaneous,
                        [this](wgpu::MapAsyncStatus status, wgpu::StringView) {
                            if (status == wgpu::MapAsyncStatus::Success) {
                                const auto* data = static_cast<const GPUAgentState*>(
                                    gpuState_.agent_state_readback_staging().GetConstMappedRange(
                                        0, GPUState::agent_state_buffer_size()));
                                if (data) {
                                    std::memcpy(cpuAgents_, data,
                                        GPUState::agent_state_buffer_size());
                                    const auto& p = cpuAgents_[player_.possessed_slot];
                                    pawnReadback_x_ = p.pos_x;
                                    pawnReadback_z_ = p.pos_z;
                                    readbackPortalTrigger_ = p.portal_trigger;
                                }
                                gpuState_.agent_state_readback_staging().Unmap();
                            }
                            pawnReadbackState_ = PawnReadbackState::IDLE;
                        });
                }

                // Check if GPU reported a portal trigger
                if (readbackPortalTrigger_ >= 0 && transitionPhase_ == TransitionPhase::IDLE) {
                    uint32_t arch_idx = static_cast<uint32_t>(readbackPortalTrigger_);
                    readbackPortalTrigger_ = -1;
                    if (arch_idx < Dim::MAX_ARCH_INSTANCES &&
                        activeArches_[arch_idx].active &&
                        activeArches_[arch_idx].is_portal) {
                        pendingDestination_ = activeArches_[arch_idx].destination;
                        transitionPhase_ = TransitionPhase::FADE_OUT;
                        transitionTimer_ = 0.0f;
                        std::cout << "[Portal] GPU trigger: arch " << arch_idx
                            << " -> seed=" << pendingDestination_.seed
                            << " finite=" << pendingDestination_.finite << "\n";
                    }
                }

                // Refill any agent slots the GPU evicted last frame.
                // No-op when no slots were evicted — just a 32-slot scan.
                respawn_evicted_agents(activeMood_, activeSeed_, queue);

                stream_patches(encoder, queue);

                // Periodic entity census dump
#ifdef DIAG_ENTITY_CENSUS
                if (currentSeconds_ - lastCensusDump_ >= CENSUS_DUMP_INTERVAL) {
                    dump_entity_census("periodic");
                    lastCensusDump_ = currentSeconds_;
                }
#endif

                // ─── Ribbon per-frame: eviction, time, nearest-rendering ─────
                {
                    // Ribbon eviction is now fully event-driven via ref_count
                    // in evict_patch_entities — no per-frame scan needed.

                    // Update time on all CPU mirrors
                    for (uint32_t i = 0; i < MAX_RIBBON_INSTANCES; i++) {
                        if (activeRibbons_[i].active)
                            ribbonStates_[i].time = currentSeconds_;
                    }

                    // Render one ribbon: hold the current slot until it's evicted,
                    // then pick the nearest active ribbon as the new rendered slot.
                    bool current_alive = renderedRibbonSlot_ != UINT32_MAX
                                      && activeRibbons_[renderedRibbonSlot_].active;

                    if (current_alive) {
                        // Hold — just update time
                        gpuState_.upload_ribbon_time(queue, currentSeconds_);
                    } else {
                        // Current slot is gone — find nearest active ribbon
                        uint32_t nearest = UINT32_MAX;
                        float nearest_d2 = FLT_MAX;
                        for (uint32_t i = 0; i < MAX_RIBBON_INSTANCES; i++) {
                            if (!activeRibbons_[i].active) continue;
                            float dx = activeRibbons_[i].anchor_x - pawnReadback_x_;
                            float dz = activeRibbons_[i].anchor_z - pawnReadback_z_;
                            float d2 = dx * dx + dz * dz;
                            if (d2 < nearest_d2) { nearest = i; nearest_d2 = d2; }
                        }

                        if (nearest != UINT32_MAX) {
                            gpuState_.upload_ribbon(queue, ribbonStates_[nearest]);
                            renderedRibbonSlot_ = nearest;
                        } else if (renderedRibbonSlot_ != UINT32_MAX) {
                            GPURibbonState empty{};
                            gpuState_.upload_ribbon(queue, empty);
                            renderedRibbonSlot_ = UINT32_MAX;
                        }
                    }
                }

                // ─── Entity mesh gen: single compute pass for all dirty families ──
                {
                    bool dirty[PopFamily::COUNT];
                    bool anyDirty = false;
                    for (uint32_t f = 0; f < PopFamily::COUNT; f++) {
                        dirty[f] = FAMILY_DISPATCH[f].prepare_mesh(this, queue);
                        anyDirty = anyDirty || dirty[f];
                    }
                    if (anyDirty) {
                        wgpu::ComputePassDescriptor cpd{};
                        cpd.label = "Entity Mesh Gen";
                        wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&cpd);
                        for (uint32_t f = 0; f < PopFamily::COUNT; f++) {
                            if (dirty[f]) FAMILY_DISPATCH[f].dispatch_mesh(this, pass);
                        }
                        pass.End();
                    }
                }
                upload_portal_array(queue);
                upload_lights(queue);
                dispatch_compute(encoder);

                // Copy full agent buffer from GPU to staging (for readback next frame)
                if (pawnReadbackState_ == PawnReadbackState::IDLE) {
                    encoder.CopyBufferToBuffer(
                        gpuState_.agent_state_buffer(), 0,
                        gpuState_.agent_state_readback_staging(), 0,
                        GPUState::agent_state_buffer_size());
                    pawnReadbackState_ = PawnReadbackState::COPIED;
                }

                // GoL zone compute — derive params + sync + evolve (separate passes for barrier)
                if (golZoneCount_ > 0) {
                    flush_zone_derive_requests(queue);
                    upload_gol_zone_config(queue);

                    {
                        wgpu::ComputePassDescriptor cpd{};
                        cpd.label = "GoL Zone Sync";
                        wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&cpd);
                        renderer_.dispatch_zone_gol_sync(pass,
                            gpuState_.zone_gol_compute_group(), activeZoneSlotCount_);
                        pass.End();
                    }
                    {
                        wgpu::ComputePassDescriptor cpd{};
                        cpd.label = "GoL Zone Evolve";
                        wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&cpd);
                        renderer_.dispatch_zone_gol_evolve(pass,
                            gpuState_.zone_gol_compute_group(), activeZoneSlotCount_);
                        pass.End();
                    }

                    // Mesh gen pass (Group 0 = compute entity, Group 1 = zone mesh gen)
                    {
                        wgpu::ComputePassDescriptor cpd{};
                        cpd.label = "GoL Zone Mesh Gen";
                        wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&cpd);
                        renderer_.dispatch_zone_mesh_reset(pass,
                            gpuState_.zone_mesh_gen_group());
                        renderer_.dispatch_zone_mesh_gen(pass,
                            gpuState_.zone_mesh_gen_group(),
                            activeZoneSlotCount_);
                        pass.End();
                    }
                }

                // Pawn aura compute — persistent terrain influence
                // Run while presence > 0 (ramping down after toggle-off) or clearing
                if (auraPresence_ > 0.0f || auraNeedsClear_) {
                    if (auraCfgDirty_) {
                        // Full config upload — profile changed or first frame
                        auraCfgDirty_ = false;
                        const auto& ap = activeAuraProfile_;
                        // Aura expansion mode: scale radius, height, tint by intensity
                        float aura_expand = mmodeIntensity_[MMODE_AURA_EXPAND];
                        float radius_scale = 1.0f + aura_expand * 2.0f;    // up to 3× radius
                        float tint_scale = 1.0f + aura_expand * 1.5f;      // up to 2.5× tint

                        // Presence scales all aura params for smooth raise/lower
                        float p = auraPresence_;

                        GPUPawnAuraConfig auraCfg{};
                        auraCfg.cell_size = PATCH_CELL_SIZE;
                        auraCfg.influence_radius = ap.influence_radius * radius_scale * p;
                        auraCfg.attack_stiffness = ap.attack_stiffness;
                        auraCfg.attack_damping = ap.attack_damping;
                        auraCfg.release_rate = (p > 0.01f) ? ap.release_rate : 999.0f;
                        auraCfg.dt = currentDt_;
                        auraCfg.effect_mask = ap.effect_mask;
                        auraCfg.aura_n = 64;
                        auraCfg.tint_strength = std::min(ap.tint_strength * tint_scale * p, 1.0f);
                        auraCfg.tint_r = ap.tint_r;
                        auraCfg.tint_g = ap.tint_g;
                        auraCfg.tint_b = ap.tint_b;
                        auraCfg.delta_mode = ap.delta_mode;
                        auraCfg.delta_magnitude = ap.delta_magnitude;
                        auraCfg.t_beats = currentBeats_;
                        // height_scale gates the compute shader's R channel write (> 0.01 = enabled).
                        // Actual terrain extrusion magnitude comes from config.pawn_aura_height in the VS.
                        auraCfg.height_scale = (auraHeightEnabled_ && p > 0.01f) ? ap.height_scale : 0.0f;
                        gpuState_.upload_pawn_aura_config(queue, auraCfg);
                    }
                    else {
                        // Steady state — only dt and t_beats change per frame
                        gpuState_.upload_pawn_aura_frame(queue, currentDt_, currentBeats_);
                    }

                    wgpu::ComputePassDescriptor cpd{};
                    cpd.label = "Pawn Aura";
                    wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&cpd);
                    renderer_.dispatch_compute_pawn_aura(pass,
                        gpuState_.pawn_aura_compute_group(),
                        GPUState::pawn_aura_workgroups());
                    pass.End();

                    // After one cleanup frame with release_rate=999, all cells are zero
                    if (auraNeedsClear_) { auraNeedsClear_ = false; }
                }

                // Orb sky layer: one-shot init, optional color-only refresh,
                // snapshot previous state for flocking neighbor reads, then
                // advance dynamics.
                dispatch_orb_init(encoder);
                dispatch_orb_recolor(encoder);
                dispatch_orb_copy_prev(encoder);
                dispatch_orb_dynamics(encoder, queue);

                if (groundEntriesDirty_) {
                    groundEntriesDirty_ = false;
                    placementDirty_ = true;
                    upload_ground_entries(queue);
                }
                if (placementDirty_) {
                    placementDirty_ = false;
                    dispatch_placement_correction(encoder);
                }

                // DIAG: frustum cull bypassed — direct draw active
                dispatch_frustum_cull(encoder, queue);

                render_shadow_pass(encoder);
                render_main_pass(encoder, backbuffer, depth);
                render_snapshot_pass(encoder);

                // --- Flush pending texture promotions (staging → exhibition) ---
                // Must run AFTER render_snapshot_pass so fresh captures are in staging
                // before being copied to exhibition layers.
                for (uint32_t i = 0; i < pendingPromotionCount_; i++) {
                    auto& p = pendingPromotions_[i];
                    wgpu::Texture src = p.is_snapshot
                        ? gpuState_.snapshot_staging_texture()
                        : gpuState_.authored_staging_texture();
                    gpuState_.promote_to_exhibition(encoder, src, p.staging_layer, p.exhibition_layer);
                }
                pendingPromotionCount_ = 0;
            }


            // --- Patch streaming: determine active 7×7 grid, generate new patches ---
            void stream_patches(wgpu::CommandEncoder& encoder, wgpu::Queue& queue) {
                // ─── Patch Generation Pipeline ─────────────────────────────────
                //
                // This function orchestrates terrain streaming. Every stage is
                // continuous and budgeted per frame — no batched operations at
                // grid boundaries except the lightweight grid shift event.
                //
                // ON GRID SHIFT (pawn crosses a patch boundary):
                //   1. Update grid center
                //   2. Evict distant tiles from spatial cache (map erase, no GPU)
                //   3. Evict out-of-range GoL zones (flag clear + deactivate)
                //   4. Re-upload tile grid with new origin
                //   5. FULLREGEN ONLY — batch-allocate ALL, batch-spawn + generate
                //      inner patches synchronously. Pawn needs ground immediately.
                //
                // CONTINUOUS EVICTION (every frame):
                //   Scans patches outside the render window. Evicts up to
                //   EVICT_BUDGET_PER_FRAME (farthest first): free layer, clear
                //   entities, unregister footprints. Compact array afterward.
                //
                // CONTINUOUS ALLOCATION (every frame, after eviction):
                //   Scans grid cells within activeRadius_ of pawn's world position.
                //   Allocates missing patches up to ALLOC_BUDGET_PER_FRAME, nearest
                //   first. Populates tile cache and re-uploads tile grid.
                //
                // DISTANCE-DRIVEN SPAWN (every frame, after allocation):
                //   Scans unspawned patches, sorts by distance to pawn.
                //   Spawns up to SPAWN_BUDGET_PER_FRAME (pyramids → arches → columns).
                //
                // DISTANCE-DRIVEN GENERATION (every frame, after spawn):
                //   Scans spawned-but-ungenerated patches + pending regens.
                //   Sorts by distance to pawn (nearest first), generates up to budget.
                //
                // VISIBILITY CYLINDER (render list gate):
                //   World-space distance from pawn to patch edge. Patches enter
                //   the draw list one at a time as the pawn moves.
                //
                // EXTERNALLY (in render(), after stream_patches returns):
                //   - Entity mesh gen (single compute pass: arches + columns + pyramids)
                //   - dispatch_compute (pawn, camera, VP)
                //   - dispatch GoL zone compute (sync + evolve, if zones active)
                //   - dispatch_placement_correction (Y-correct arches, columns, pyramids, paintings — decoupled from photographer)
                //   - render passes (shadow, main, snapshot)
                //
                // GOL ZONE LIFECYCLE:
                //   - select_gol_for_patch: entity pipeline (select → place → commit),
                //     lattice-gated, footprint-registered. Evicts via host-patch entity_refs.
                //   - upload_gol_zone_config: per frame, before GoL compute dispatch.

                int32_t centerX, centerZ;
                uint32_t patchStagingOffset = 0;  // running offset into staging buffer (multiple batches per frame)
                bool tileGridDirty = false;        // coalesce tile grid uploads to one per frame
                if (finiteMode_) {
                    centerX = 0;
                    centerZ = 0;
                }
                else {
                    centerX = (int32_t)std::floor(pawnReadback_x_ / PATCH_EXTENT);
                    centerZ = (int32_t)std::floor(pawnReadback_z_ / PATCH_EXTENT);
                }

                // In finite mode, cap the effective radius
                uint32_t savedRadius = activeRadius_;
                if (finiteMode_ && activeRadius_ > finiteRadius_) {
                    activeRadius_ = finiteRadius_;
                }

                bool gridChanged = (centerX != lastCenterX_ || centerZ != lastCenterZ_);

                if (gridChanged) {
                    int32_t oldCX = lastCenterX_;
                    int32_t oldCZ = lastCenterZ_;
                    lastCenterX_ = centerX;
                    lastCenterZ_ = centerZ;

                    bool fullRegen = (oldCX == INT32_MAX);  // first frame

                    // Lightweight cache maintenance (no GPU buffer writes)
                    evict_distant_tiles(centerX, centerZ);
                    // GoL zones now evict through host-patch entity_refs

                    // Tile grid origin depends on grid center — re-upload
                    // unconditionally so GPU heightfield gen reads correct
                    // modifiers from the new origin.
                    if (!fullRegen) {
                        tileGridDirty = true;
                    }

                    // Guaranteed back-portal in finite worlds (fires once after teardown)
                    // DEFERRED: must wait for tile cache below (portals need terrain heights)

                    // ─── FULLREGEN: synchronous bootstrap ────────────────────
                    //
                    // First frame of a new world: batch-allocate ALL patches,
                    // spawn + generate inner patches synchronously so the pawn
                    // has ground immediately. Outer patches use the per-frame
                    // distance-driven scans like everything else.
                    if (fullRegen) {
                        int32_t rr = (int32_t)activeRadius_;
                        static constexpr int32_t TILE_PAD = 1;
                        int32_t rp = rr + TILE_PAD;
                        for (int32_t gz = centerZ - rp; gz <= centerZ + rp; gz++) {
                            for (int32_t gx = centerX - rp; gx <= centerX + rp; gx++) {
                                GridKey key{ gx, gz };
                                if (tileCache_.find(key) == tileCache_.end()) {
                                    TileState ts = generate_tile_state(gx, gz);
                                    tick_terrain_tokens(ts, tile_seed(activeSeed_, gx, gz));
                                    tileCache_[key] = ts;
                                }
                            }
                        }

                        // NOW spawn portals — tile cache is populated, terrain heights are correct
                        if (backPortalPending_) {
                            force_spawn_back_portal(queue);
                        }
                        for (int32_t gz = centerZ - rr; gz <= centerZ + rr; gz++) {
                            for (int32_t gx = centerX - rr; gx <= centerX + rr; gx++) {
                                bool found = false;
                                for (uint32_t i = 0; i < activePatchCount_; i++) {
                                    if (patches_[i].grid_x == gx && patches_[i].grid_z == gz) {
                                        found = true; break;
                                    }
                                }
                                if (!found && freeLayerCount_ > 0) {
                                    uint32_t layer = alloc_layer();
                                    patches_[activePatchCount_] = ActivePatch{};
                                    patches_[activePatchCount_].grid_x = gx;
                                    patches_[activePatchCount_].grid_z = gz;
                                    patches_[activePatchCount_].layer = layer;
                                    patches_[activePatchCount_].valid = true;
                                    activePatchCount_++;
                                }
                            }
                        }
                        tileGridDirty = true;

                        // Spawn inner patches
                        PatchCandidate spawnCands[MAX_PATCHES];
                        uint32_t spawnCount = collect_sorted_patches(spawnCands,
                            pawnReadback_x_, pawnReadback_z_,
                            [&](const ActivePatch& p) {
                                return p.phase == PatchPhase::ALLOCATED &&
                                    in_priority_window(p.grid_x, p.grid_z, centerX, centerZ);
                            }, true);
                        spawn_selected_patches(spawnCands, spawnCount, queue);

                        // Generate inner patches
                        PatchCandidate genCands[MAX_PATCHES];
                        uint32_t genCount = collect_sorted_patches(genCands,
                            pawnReadback_x_, pawnReadback_z_,
                            [&](const ActivePatch& p) {
                                return p.phase == PatchPhase::SPAWNED &&
                                    in_priority_window(p.grid_x, p.grid_z, centerX, centerZ);
                            }, true);
                        generate_selected_patches(genCands, genCount,
                            encoder, queue, patchStagingOffset, tileGridDirty);
                    }
                }

                // ─── CONTINUOUS PATCH EVICTION ────────────────────────────────
                //
                // Every frame, scan for patches outside the render window
                // (relative to grid center). Evict up to EVICT_BUDGET_PER_FRAME,
                // farthest first. Frees layers for reuse by the allocation scan.
                // Compact the array after eviction to remove holes.
                {
                    PatchCandidate candidates[MAX_PATCHES];
                    uint32_t count = collect_sorted_patches(candidates,
                        pawnReadback_x_, pawnReadback_z_,
                        [&](const ActivePatch& p) {
                            return !in_render_window(p.grid_x, p.grid_z,
                                lastCenterX_, lastCenterZ_);
                        }, false);  // farthest first

                    uint32_t evictThisFrame = std::min(count, EVICT_BUDGET_PER_FRAME);
                    for (uint32_t e = 0; e < evictThisFrame; e++) {
                        evict_patch(candidates[e].idx, queue);
                    }

                    if (evictThisFrame > 0) {
                        uint32_t write = 0;
                        for (uint32_t i = 0; i < activePatchCount_; i++) {
                            if (patches_[i].valid) patches_[write++] = patches_[i];
                        }
                        activePatchCount_ = write;
                        patchInstancesDirty_ = true;
                    }
                }

                // ─── CONTINUOUS PATCH ALLOCATION ──────────────────────────────
                //
                // Every frame, scan for grid cells within activeRadius_ of
                // the pawn's actual world position that don't have patches.
                // Allocate up to ALLOC_BUDGET_PER_FRAME, nearest first. This
                // spreads allocation across idle frames so patches are ready
                // before the grid shift that would have created them.
                //
                // The pawn's world position can be up to half a patch ahead
                // of the grid center, so this naturally pre-allocates one
                // ring in the direction of movement.
                {
                    int32_t pawnGX = (int32_t)std::floor(pawnReadback_x_ / PATCH_EXTENT);
                    int32_t pawnGZ = (int32_t)std::floor(pawnReadback_z_ / PATCH_EXTENT);
                    int32_t rr = (int32_t)activeRadius_;
                    float pawn_wx = pawnReadback_x_;
                    float pawn_wz = pawnReadback_z_;
                    float half = PATCH_EXTENT * 0.5f;

                    // O(1) patch existence lookup (replaces O(N) inner scan)
                    std::unordered_set<GridKey, GridKeyHash> activePatchSet;
                    activePatchSet.reserve(activePatchCount_);
                    for (uint32_t i = 0; i < activePatchCount_; i++) {
                        activePatchSet.insert({ patches_[i].grid_x, patches_[i].grid_z });
                    }

                    struct AllocCandidate { int32_t gx, gz; float dist2; };
                    AllocCandidate candidates[MAX_PATCHES];
                    uint32_t candidateCount = 0;

                    for (int32_t gz = pawnGZ - rr; gz <= pawnGZ + rr; gz++) {
                        for (int32_t gx = pawnGX - rr; gx <= pawnGX + rr; gx++) {
                            // Must be within allocation window of grid center
                            if (!in_render_window(gx, gz, lastCenterX_, lastCenterZ_)) continue;
                            bool found = activePatchSet.count({ gx, gz }) > 0;
                            if (!found && freeLayerCount_ > 0 && candidateCount < MAX_PATCHES) {
                                float ox = (gx + 0.5f) * PATCH_EXTENT;
                                float oz = (gz + 0.5f) * PATCH_EXTENT;
                                float d2 = patch_distance_sq(pawn_wx, pawn_wz, ox, oz, half);
                                candidates[candidateCount++] = { gx, gz, d2 };
                            }
                        }
                    }

                    // Sort by distance (nearest first)
                    for (uint32_t i = 1; i < candidateCount; i++) {
                        AllocCandidate key = candidates[i];
                        uint32_t j = i;
                        while (j > 0 && candidates[j - 1].dist2 > key.dist2) {
                            candidates[j] = candidates[j - 1];
                            j--;
                        }
                        candidates[j] = key;
                    }

                    bool allocated_any = false;
                    uint32_t allocThisFrame = std::min(candidateCount, ALLOC_BUDGET_PER_FRAME);
                    for (uint32_t a = 0; a < allocThisFrame; a++) {
                        int32_t gx = candidates[a].gx;
                        int32_t gz = candidates[a].gz;
                        // Ensure tile cache entry (primary — ticks terrain tokens)
                        GridKey key{ gx, gz };
                        if (tileCache_.find(key) == tileCache_.end()) {
                            TileState ts = generate_tile_state(gx, gz);
                            tick_terrain_tokens(ts, tile_seed(activeSeed_, gx, gz));
                            tileCache_[key] = ts;
                        }
                        // Also cache neighbors for tile grid padding
                        for (int dz = -1; dz <= 1; dz++) for (int dx = -1; dx <= 1; dx++) {
                            GridKey nk{ gx + dx, gz + dz };
                            if (tileCache_.find(nk) == tileCache_.end()) {
                                tileCache_[nk] = generate_tile_state(gx + dx, gz + dz);
                            }
                        }
                        uint32_t layer = alloc_layer();
                        patches_[activePatchCount_] = ActivePatch{};
                        patches_[activePatchCount_].grid_x = gx;
                        patches_[activePatchCount_].grid_z = gz;
                        patches_[activePatchCount_].layer = layer;
                        patches_[activePatchCount_].valid = true;
                        activePatchCount_++;
                        allocated_any = true;
                    }

                    // Mark tile grid and patch instances dirty whenever new patches were allocated
                    if (allocated_any) {
                        tileGridDirty = true;
                        patchInstancesDirty_ = true;
                    }
                }

                // ─── DISTANCE-DRIVEN ENTITY SPAWNING ─────────────────────────
                //
                // Every frame, scan for unspawned patches. Sort by distance
                // to pawn (nearest first), spawn up to SPAWN_BUDGET_PER_FRAME.
                // Priority order within each patch: pyramids → arches → columns
                // (largest footprint first, matching the ground hierarchy).
                //
                // Spawning must complete before generation — piers from spawned
                // entities affect heightfield baking. The generation scan below
                // only considers patches with phase == SPAWNED or NEEDS_REGEN.
                {
                    PatchCandidate candidates[MAX_PATCHES];
                    uint32_t count = collect_sorted_patches(candidates,
                        pawnReadback_x_, pawnReadback_z_,
                        [](const ActivePatch& p) {
                            return p.phase == PatchPhase::ALLOCATED;
                        }, true);
                    spawn_selected_patches(candidates,
                        std::min(count, SPAWN_BUDGET_PER_FRAME), queue);
                }

                // ─── DISTANCE-DRIVEN HEIGHTFIELD GENERATION ──────────────────
                //
                // Every frame, scan all spawned patches for pending work
                // (SPAWNED or NEEDS_REGEN). Sort by world-space distance
                // to pawn (nearest first) and generate up to budget.
                //
                // Regens (stale heightfields from new piers) are already
                // inside the visibility cylinder, so they're always closer
                // than frontier patches and naturally get priority.
                {
                    PatchCandidate candidates[MAX_PATCHES];
                    uint32_t count = collect_sorted_patches(candidates,
                        pawnReadback_x_, pawnReadback_z_,
                        [](const ActivePatch& p) {
                            return p.phase == PatchPhase::SPAWNED ||
                                p.phase == PatchPhase::NEEDS_REGEN;
                        }, true);
                    generate_selected_patches(candidates,
                        std::min(count, patches_budget_this_frame()),
                        encoder, queue, patchStagingOffset, tileGridDirty);
                }

                // Upload patch instances sorted by LOD band, then pre-gen ring.
                // Layout: [0..lod0) LOD-0 full mesh, [lod0..render) LOD-1 half mesh,
                //          [render..all) pre-gen ring (not drawn, used for placement).
                // This lets render passes issue two indexed draws with firstInstance offset.
                {
                    GPUPatchInstance instances[MAX_PATCHES]{};
                    uint32_t lod0Count = 0;
                    uint32_t lod1Count = 0;
                    uint32_t pregenCount = 0;

                    // Temporary arrays for each band
                    GPUPatchInstance lod0[MAX_PATCHES]{};
                    GPUPatchInstance lod1[MAX_PATCHES]{};
                    GPUPatchInstance pregen[MAX_PATCHES]{};

                    // Visibility cylinder: world-space distance from pawn to
                    // nearest patch edge. Patches cross the threshold one at a
                    // time as the pawn moves — no batch pop on grid shifts.
                    float pawn_wx = pawnReadback_x_;
                    float pawn_wz = pawnReadback_z_;
                    float half = PATCH_EXTENT * 0.5f;

                    for (uint32_t i = 0; i < activePatchCount_; i++) {
                        if (patches_[i].phase != PatchPhase::GENERATED &&
                            patches_[i].phase != PatchPhase::NEEDS_REGEN) continue;

                        float ox = (patches_[i].grid_x + 0.5f) * PATCH_EXTENT;
                        float oz = (patches_[i].grid_z + 0.5f) * PATCH_EXTENT;

                        GPUPatchInstance inst{};
                        inst.origin[0] = ox;
                        inst.origin[1] = oz;
                        inst.extent = PATCH_EXTENT;
                        inst.layer = patches_[i].layer;

                        float d2 = patch_distance_sq(pawn_wx, pawn_wz, ox, oz, half);

                        // Finite mode: all patches visible (walls define boundary, not fog)
                        if (finiteMode_ || d2 <= VISIBILITY_CYLINDER_RADIUS_SQ) {
                            if (d2 <= LOD0_CYLINDER_RADIUS_SQ) {
                                lod0[lod0Count++] = inst;
                            }
                            else {
                                lod1[lod1Count++] = inst;
                            }
                        }
                        else {
                            pregen[pregenCount++] = inst;
                        }
                    }

                    // Pack: LOD-0, then LOD-1, then pregen
                    uint32_t w = 0;
                    std::memcpy(instances + w, lod0, lod0Count * sizeof(GPUPatchInstance)); w += lod0Count;
                    std::memcpy(instances + w, lod1, lod1Count * sizeof(GPUPatchInstance)); w += lod1Count;
                    std::memcpy(instances + w, pregen, pregenCount * sizeof(GPUPatchInstance)); w += pregenCount;

                    gpuState_.upload_patch_instances(queue, instances, w);
                    lod0PatchCount_ = lod0Count;
                    renderPatchCount_ = lod0Count + lod1Count;
                    allPatchCount_ = w;

                    // Sync placement_patch_count so compute_entity_placement
                    // can sample heightfields from the current frame's patch set.
                    gpuState_.config().placement_patch_count = w;
                    gpuState_.upload_placement_patch_count(queue);

                    // ─── Patch grid: O(1) spatial index for sample_terrain_y_at ────────
                    // Populate (patch_gx, patch_gz) → layer map. The shader derives a
                    // patch's grid cell from floor(world_xz / cell_extent) and looks up
                    // the layer directly, replacing the previous linear bbox scan.
                    // Entries store (layer + 1) so that 0 encodes an empty slot.
                    // Anchor is the bounding-box minimum of the valid set — always fits
                    // in a PATCH_PREGEN_SIDE × PATCH_PREGEN_SIDE window (15×15 = 225).
                    {
                        GPUPatchGrid grid{};
                        grid.side = Dim::PATCH_PREGEN_SIDE;
                        grid.cell_extent = PATCH_EXTENT;

                        int32_t min_gx = INT32_MAX;
                        int32_t min_gz = INT32_MAX;
                        for (uint32_t i = 0; i < activePatchCount_; i++) {
                            if (!patches_[i].valid) continue;
                            if (patches_[i].phase != PatchPhase::GENERATED &&
                                patches_[i].phase != PatchPhase::NEEDS_REGEN) continue;
                            min_gx = std::min(min_gx, patches_[i].grid_x);
                            min_gz = std::min(min_gz, patches_[i].grid_z);
                        }
                        if (min_gx == INT32_MAX) { min_gx = 0; min_gz = 0; }
                        grid.origin_x = min_gx;
                        grid.origin_z = min_gz;
                        // entries[] are zero-initialized by value-init above.

                        for (uint32_t i = 0; i < activePatchCount_; i++) {
                            if (!patches_[i].valid) continue;
                            if (patches_[i].phase != PatchPhase::GENERATED &&
                                patches_[i].phase != PatchPhase::NEEDS_REGEN) continue;
                            int32_t lx = patches_[i].grid_x - grid.origin_x;
                            int32_t lz = patches_[i].grid_z - grid.origin_z;
                            if (lx < 0 || lz < 0 ||
                                lx >= int32_t(grid.side) || lz >= int32_t(grid.side)) continue;
                            grid.entries[lz * grid.side + lx] = patches_[i].layer + 1u;
                        }

                        gpuState_.upload_patch_grid(queue, grid);
                    }
                }
                // GPU Y-correction is additive (ground_y += terrain), so ground
                // entries must be re-uploaded with offset-only values whenever
                // the heightfield changes. Tie groundEntriesDirty to patch changes.
                groundEntriesDirty_ = groundEntriesDirty_ || patchInstancesDirty_;
                placementDirty_ = placementDirty_ || patchInstancesDirty_;
                patchInstancesDirty_ = false;

                // ─── Entity distance culling ─────────────────────────────
                entitiesCulled_ = update_entity_draw_visibility(queue);

                // ─── Deferred uploads (one per frame max) ────────────────
                if (tileGridDirty) upload_tile_grid_now(queue, lastCenterX_, lastCenterZ_);
                flush_pier_count(queue);

                audit_entity_integrity();

                // Restore radius if we capped it for finite mode
                if (finiteMode_) { activeRadius_ = savedRadius; }
            }

            // ── Mood System (modules/mood.inl) ──
#include "modules/mood.inl"


// ── Render Passes (modules/render_passes.inl) ──
#include "modules/render_passes.inl"

        public:

            void on_input(const InputEvent& event) override {
                switch (event.type) {
                case InputEvent::Type::KeyDown:
                    on_key_down(event.key);
                    break;
                case InputEvent::Type::KeyUp:
                    on_key_up(event.key);
                    break;
                case InputEvent::Type::MouseMove:
                    on_mouse_move(event.x, event.y);
                    break;
                case InputEvent::Type::MouseButton:
                    on_mouse_button(event.button, event.pressed);
                    break;
                case InputEvent::Type::Scroll:
                    on_scroll(event.y);
                    break;
                }
            }

        private:

            // ── Input Handling (modules/input.inl) ──
#include "modules/input.inl"

        public:

            void get_clear_color(float& r, float& g, float& b) const override {
                r = clearColor_[0];
                g = clearColor_[1];
                b = clearColor_[2];
            }

            wgpu::TextureFormat depth_format() const override {
                return wgpu::TextureFormat::Depth24Plus;
            }

            bool supports_backspace() const override {
                return true;
            }

            bool reload_shaders() override { return renderer_.reload(); }
            const std::string& shader_path() const { return renderer_.shader_path(); }

        };

    } // namespace the_board
} // namespace t7/