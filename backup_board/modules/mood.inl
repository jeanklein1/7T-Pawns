// ─── mood.inl ──────────────────────────────────────────────────
//
// Atmosphere, indoor lighting, shell geometry, portals.
// One canonical mood entry point: apply_mood. Mood transitions are
// requested via request_mood_transition (also exposed for portal
// crossings).
//
// ┌─── Public surface (called from outside this file) ──────────────┐
// │                                                                  │
// │  Mood lifecycle:                                                 │
// │    apply_mood(mood, queue)              — atmosphere + setup     │
// │    request_mood_transition(mood)        — transition entry       │
// │                                                                  │
// │  Indoor support:                                                 │
// │    derive_indoor_lights(seed, ...)      — scheme → spot lights   │
// │    generate_indoor_shell(queue, m)      — walls + ceiling mesh   │
// │    clear_indoor_shell(queue)            — index count to 0       │
// │                                                                  │
// │  Portals (driven by transition / portal crossings):              │
// │    force_spawn_portal_at(queue, cx, cz, rot, dest, is_back)      │
// │    force_spawn_back_portal(queue)                                │
// │    force_spawn_finite_portals(queue)                             │
// │                                                                  │
// │  Per-frame uploads:                                              │
// │    upload_lights(queue)                 — sun + spot + (point)   │
// │    upload_portal_array(queue)           — when mood_state_.portals_dirty     │
// │                                                                  │
// │  Cross-module reads (consumed by other modules):                 │
// │    mood_state_.active                          — read everywhere        │
// │    musical_state_.mood_allows_modes, gol_state_.mood_allowed                  │
// │    mood_state_.back_portal_pending, backPortalPosition_                       │
// │    mood_state_.spot_light_active, entities_state_.lights_dirty, mood_state_.portals_dirty                 │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// Included inside the Cartridge class body.
// Depends on: entities.inl (ARCH_TIERS, ArchIdx, ArchTier),
//             musical.inl (reset_musical_couplings, is_mmode_on,
//             MMODE_TERRAIN_WAVES, band motion state, mmode_intensity),
//             ribbon.inl (RibbonProp, RIBBON_BASE_TIER_WEIGHTS,
//             RIBBON_TIER_COUNT, fill_ribbon_selection_geometry,
//             commit_ribbon, ribbon_state_.gpu, ribbon_state_.rendered_slot,
//             ribbon_state_.mood_offset),
//             gallery.inl (clear_wall_paintings, place_wall_paintings),
//             orbs.inl (configure_orbs, ORB_MOOD_TABLE),
//             spawn_engine.inl (estimate_terrain_height, write_pier,
//             solve_catenary_a, entities_state_.arches, entities_state_.arch_mesh_gen_pending),
//             render_passes.inl (compute_spot_light_vp).
//
// SEAM[mood:K1] apply_mood is the single canonical mood entry point.
//   All mood transitions — keyboard, portal crossings, world
//   teardown — funnel through here. Other code paths set
//   pendingDestination_ / transitionPhase_; the actual mood activation
//   happens here. The orchestrator owns only ordering and the
//   activate-mood bookkeeping; the substantive work splits across
//   five named sub-functions (apply_mood_lighting, _spot_lights,
//   _indoor_shell, _band_motion, _anchor_ribbon).
// SEAM[mood:K3] reset_musical_couplings is called from apply_mood as
//   part of the per-mood-transition lifecycle. The musical reset
//   used to be duplicated across cartridge.hpp::teardown_world and
//   apply_mood; consolidated to a single function in musical.inl
//   with one call site here. See DONE[mood:K3] in musical.inl.
// SEAM[mood:K4] apply_mood_anchor_ribbon below is the second entry
//   point to commit_ribbon (the dual-entry pattern noted at
//   ribbon.inl::SEAM[ribbon:dual-entry]). Mood-5 forces a ribbon
//   spawn through fill_ribbon_selection_geometry + commit_ribbon
//   without going through the patch-streaming dispatch.
// SEAM[mood:L1] MoodProfile.has_anchor_ribbon is the per-mood flag
//   that gates apply_mood_anchor_ribbon's execution. Default false
//   for moods 0-4; true for mood 5 (FINITE_OUTDOOR_REF). Read here
//   and referenced from orbs.inl as mood:L1 in the anchor-ribbon
//   gating logic.
// DONE[mood:K2] apply_mood was a 217-line linear sequence mixing 12
//   concerns (atmospheric, indoor lighting, indoor shell, band
//   motion, musical reset, anchor ribbon, orbs). Split into named
//   helpers; apply_mood itself orchestrates. The five helpers below
//   match the natural seams in the flow. Per-mood-transition order
//   is preserved exactly — no semantic change, just nameable
//   sub-blocks.
// DONE[input:L1] request_mood_transition is the single canonical
//   transition entry point. Replaces the five copy-paste cases in
//   input.inl (KEY_5..KEY_9). Bails if a transition is already in
//   flight. Lives in mood.inl rather than input.inl because portal
//   crossings and other code paths can also drive mood transitions.
//   One door, many keys.
// ─────────────────────────────────────────────────────────────────


// ═══ TUNING DATA ═════════════════════════════════════════════════
//
// Per-mood authoring data: indoor wall palettes and indoor lighting
// schemes. Both are seed-driven (a per-mood roll picks one of the
// palettes / schemes from the corresponding table), then per-light
// or per-color parameters are sampled from the chosen entry.
//
// SEAM[mood:tuning-data] consumed only by apply_mood (palettes) and
//   derive_indoor_lights (schemes). Migrated from cartridge.hpp's
//   class body; same migration class as ORB_MOOD_TABLE → orbs.inl
//   and WALL_ART → gallery.inl.
//
// This module is included inside the public: section of the
// Cartridge class body (alongside its functions, which need to be
// callable from update()). The data declarations below should
// remain private — they're internal authoring tables, not part of
// the cartridge's public surface — so we use a private/public
// access toggle around them. When mood.inl's include placement is
// reconsidered, the toggle can be removed.

        private:

            // ── Indoor Wall Palette ────────────────────────────────────
            //
            // Per-seed wall+ceiling color override for indoor moods. The
            // MOOD_TABLE wall_color/ceiling_color act as fallback for the
            // open-world finite cases; in flat/vault moods, apply_mood
            // selects one of these palettes from world_state_.active_seed and substitutes
            // it before generate_indoor_shell uploads the shell mesh.
            //
            // Each palette is a designed (wall, ceiling) pair where the
            // ceiling is a slightly darker shade of the wall — gives the
            // room visual depth instead of flat single-color surfaces.
            // Authored to feel like museum / gallery wall finishes rather
            // than random RGB rolls.
            struct IndoorPalette {
                const char* name;
                float wall_color[3];
                float ceiling_color[3];
            };
            static constexpr IndoorPalette INDOOR_PALETTES[] = {
                { "warm taupe",     { 0.72f, 0.65f, 0.55f }, { 0.65f, 0.58f, 0.50f } },
                { "slate blue",     { 0.58f, 0.62f, 0.68f }, { 0.50f, 0.55f, 0.62f } },
                { "terracotta",     { 0.65f, 0.50f, 0.42f }, { 0.55f, 0.42f, 0.36f } },
                { "sage",           { 0.62f, 0.68f, 0.58f }, { 0.55f, 0.60f, 0.52f } },
                { "pale linen",     { 0.85f, 0.80f, 0.72f }, { 0.78f, 0.73f, 0.65f } },
                { "deep mocha",     { 0.45f, 0.40f, 0.35f }, { 0.38f, 0.34f, 0.30f } },
                { "dusty rose",     { 0.70f, 0.58f, 0.58f }, { 0.62f, 0.50f, 0.50f } },
                { "warm charcoal",  { 0.40f, 0.38f, 0.36f }, { 0.32f, 0.30f, 0.28f } },
            };
            static constexpr uint32_t INDOOR_PALETTE_COUNT =
                sizeof(INDOOR_PALETTES) / sizeof(INDOOR_PALETTES[0]);

            // ── Indoor Lighting Schemes ───────────────────────────────
            //
            // Seed-driven procedural lighting for indoor moods. Each scheme
            // defines a lighting character (which surfaces carry lights,
            // how many, primary vs accent roles). Per-light parameters
            // (position along surface, intensity, cone width, color warmth)
            // are derived from world_state_.active_seed at mood transition time.
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
                // Anchor-surface position (carried through from LightSchemeSlot).
                float lat_mean, lat_sigma;        // along anchor surface
                float hfrac_mean, hfrac_sigma;    // ceiling: Z; walls: height
            };

            static constexpr float SCHEME_WEIGHTS[] = { 0.35f, 0.35f, 0.15f, 0.15f };
            static constexpr uint32_t SCHEME_COUNT = 4;
            static constexpr const char* SCHEME_NAMES[] = { "Cathedral", "Quartet", "Gallery", "Sanctum" };
            static constexpr const char* ANCHOR_NAMES[] = { "ceiling", "wall_N", "wall_S", "wall_E", "wall_W" };

            // ── Lighting Scheme Table ──
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
                // Position on the anchor surface (0..1 along the surface span).
                // For ceiling: lat=X-axis, hfrac=Z-axis (both in [0..1]).
                // For walls:   lat=along-wall, hfrac=height (both in [0..1]).
                // Existing schemes (Cathedral/Gallery/Sanctum) used hardcoded
                // 0.5/0.65 means with 0.15/0.10 sigmas; they keep that here.
                // New schemes (e.g. Quartet) override per slot to place lights
                // deliberately rather than relying on each one rolling near
                // center independently.
                float lat_mean, lat_sigma;
                float hfrac_mean, hfrac_sigma;
            };

            struct LightScheme {
                uint32_t slot_count;
                LightSchemeSlot slots[4];  // MAX_SPOT_LIGHTS
            };

            // ── Scheme Definitions ──
            //                                role            int_μ  int_σ  inn_μ inn_σ out_μ out_σ wrm_μ wrm_σ  pit_μ  pit_σ  yaw_μ  yaw_σ   lat_μ lat_σ  hfrac_μ hfrac_σ
            static constexpr LightScheme LIGHT_SCHEMES[SCHEME_COUNT] = {
                /* 0: Cathedral — ceiling primary + 2 wall sconces */
                { 3, {
                    { AnchorRole::CEILING,   8.0f, 2.5f, 0.6f, 0.2f,  1.2f, 0.15f, 0.35f, 0.20f,  0.0f,  0.12f, 0.0f, 0.12f,   0.50f, 0.15f, 0.65f, 0.10f },
                    { AnchorRole::WALL_A,    5.0f, 1.5f, 0.4f, 0.15f, 1.0f, 0.15f, 0.20f, 0.15f,  0.60f, 0.40f, 0.0f, 0.30f,   0.50f, 0.15f, 0.65f, 0.10f },
                    { AnchorRole::WALL_B,    5.0f, 1.5f, 0.4f, 0.15f, 1.0f, 0.15f, 0.75f, 0.15f,  0.60f, 0.40f, 0.0f, 0.30f,   0.50f, 0.15f, 0.65f, 0.10f },
                }},
                /* 1: Quartet — 4 ceiling lights at quadrant corners.
                 *    Tight sigma (0.07) keeps each light pinned in its quadrant;
                 *    means at 0.30/0.70 produce a 2x2 layout that fills the
                 *    room evenly. Per-light intensity dropped to 4.0±1.0 to
                 *    keep total scene brightness comparable to Cathedral
                 *    (which has ~8.0 from a single ceiling light + sconces). */
                { 4, {
                    { AnchorRole::CEILING,   4.0f, 1.0f, 0.6f, 0.2f,  1.2f, 0.15f, 0.35f, 0.20f,  0.0f,  0.10f, 0.0f, 0.10f,   0.30f, 0.07f, 0.30f, 0.07f },
                    { AnchorRole::CEILING,   4.0f, 1.0f, 0.6f, 0.2f,  1.2f, 0.15f, 0.35f, 0.20f,  0.0f,  0.10f, 0.0f, 0.10f,   0.70f, 0.07f, 0.30f, 0.07f },
                    { AnchorRole::CEILING,   4.0f, 1.0f, 0.6f, 0.2f,  1.2f, 0.15f, 0.35f, 0.20f,  0.0f,  0.10f, 0.0f, 0.10f,   0.30f, 0.07f, 0.70f, 0.07f },
                    { AnchorRole::CEILING,   4.0f, 1.0f, 0.6f, 0.2f,  1.2f, 0.15f, 0.35f, 0.20f,  0.0f,  0.10f, 0.0f, 0.10f,   0.70f, 0.07f, 0.70f, 0.07f },
                }},
                /* 2: Gallery — 2 opposing wall lights, no ceiling */
                { 2, {
                    { AnchorRole::WALL_A,    7.0f, 2.0f, 0.4f, 0.15f, 1.1f, 0.15f, 0.25f, 0.20f,  0.55f, 0.45f, 0.0f, 0.35f,   0.50f, 0.15f, 0.65f, 0.10f },
                    { AnchorRole::WALL_B,    7.0f, 2.0f, 0.4f, 0.15f, 1.1f, 0.15f, 0.65f, 0.20f,  0.55f, 0.45f, 0.0f, 0.35f,   0.50f, 0.15f, 0.65f, 0.10f },
                }},
                /* 3: Sanctum — single dramatic source */
                { 1, {
                    { AnchorRole::SEED_PICK, 10.0f, 2.5f, 0.5f, 0.2f, 1.2f, 0.15f, 0.45f, 0.25f,  0.50f, 0.40f, 0.0f, 0.30f,   0.50f, 0.15f, 0.65f, 0.10f },
                }},
            };

        public:


// ═══ INDOOR LIGHT DERIVATION ═════════════════════════════════════
//
// Selects a lighting scheme from world_state_.active_seed, then derives
// per-light parameters (position, direction, intensity, cone,
// color) from the seed. Called once at mood transition.

void derive_indoor_lights(uint32_t seed, float bmin, float bmax,
    float ceiling_height, CeilingType ceiling_type = CeilingType::FLAT) {
    cpuSpotLights_ = GPUSpotLightArray{};

    float room_size = bmax - bmin;
    float room_range = room_size * 0.8f;

    // Scheme selection
    uint32_t scheme = select_tier(seed, IndoorLightProp::SCHEME,
        SCHEME_WEIGHTS, SCHEME_COUNT);

    // Wall pair: N/S or E/W — gives axis variety across seeds
    bool use_ew = cpu_hash_f(seed, IndoorLightProp::WALL_PAIR) > 0.5f;
    LightAnchor wall_a = use_ew ? LightAnchor::WALL_EAST : LightAnchor::WALL_NORTH;
    LightAnchor wall_b = use_ew ? LightAnchor::WALL_WEST : LightAnchor::WALL_SOUTH;

    // Read slot definitions from scheme table
    const auto& sch = LIGHT_SCHEMES[scheme];
    LightSlotDef slots[MAX_SPOT_LIGHTS];
    uint32_t count = sch.slot_count;

    for (uint32_t i = 0; i < count; i++) {
        const auto& src = sch.slots[i];
        // Resolve anchor role to concrete LightAnchor
        LightAnchor anchor;
        switch (src.role) {
        case AnchorRole::CEILING:   anchor = LightAnchor::CEILING; break;
        case AnchorRole::WALL_A:    anchor = wall_a; break;
        case AnchorRole::WALL_B:    anchor = wall_b; break;
        case AnchorRole::SEED_PICK: {
            float anchor_roll = cpu_hash_f(seed, IndoorLightProp::ANCHOR_PICK);
            anchor = (anchor_roll < 0.55f) ? LightAnchor::CEILING
                : (anchor_roll < 0.775f) ? wall_a : wall_b;
            break;
        }
        }
        slots[i] = { anchor,
            src.intensity_mean, src.intensity_sigma,
            src.inner_mean, src.inner_sigma,
            src.outer_mean, src.outer_sigma,
            src.warmth_mean, src.warmth_sigma,
            src.aim_pitch_mean, src.aim_pitch_sigma,
            src.aim_yaw_mean, src.aim_yaw_sigma,
            src.lat_mean, src.lat_sigma,
            src.hfrac_mean, src.hfrac_sigma };
    }

    // Derive each light from its slot spec + seed
    for (uint32_t i = 0; i < count; i++) {
        const auto& s = slots[i];
        uint32_t base = IndoorLightProp::SLOT_BASE + i * 10;
        auto& L = cpuSpotLights_.lights[i];

        // Position: slide along anchor surface. Mean/sigma come from the
        // scheme slot — Cathedral/Gallery/Sanctum keep their previous
        // (0.5, 0.15) / (0.65, 0.10) behavior, while Quartet pins each
        // slot to a specific quadrant via tighter sigma.
        // Clamp ranges differ by anchor: ceilings use 10–90% of either axis
        // (lights stay inside the room edge by 10%); walls use 40–85% along
        // the wall and 40–85% in height (sconce range, not floor or trim).
        float lat = std::clamp(
            cpu_sample_gaussian(seed, base + IndoorLightProp::LATERAL,
                s.lat_mean, s.lat_sigma),
            0.1f, 0.9f);
        float hfrac_raw = cpu_sample_gaussian(seed,
            base + IndoorLightProp::HEIGHT, s.hfrac_mean, s.hfrac_sigma);
        float hfrac = (s.anchor == LightAnchor::CEILING)
            ? std::clamp(hfrac_raw, 0.1f, 0.9f)    // ceiling: Z-axis position
            : std::clamp(hfrac_raw, 0.4f, 0.85f);  // walls: sconce height

        float wall_off = 1.0f;
        float px, py, pz;

        // Position: anchor-dependent placement on surface
        switch (s.anchor) {
        case LightAnchor::CEILING:
            px = bmin + lat * room_size;
            py = ceiling_height - 0.5f;
            pz = bmin + hfrac * room_size;
            break;
        case LightAnchor::WALL_NORTH:
            px = bmin + lat * room_size;
            py = ceiling_height * hfrac;
            pz = bmax - wall_off;
            break;
        case LightAnchor::WALL_SOUTH:
            px = bmin + lat * room_size;
            py = ceiling_height * hfrac;
            pz = bmin + wall_off;
            break;
        case LightAnchor::WALL_EAST:
            px = bmax - wall_off;
            py = ceiling_height * hfrac;
            pz = bmin + lat * room_size;
            break;
        case LightAnchor::WALL_WEST:
            px = bmin + wall_off;
            py = ceiling_height * hfrac;
            pz = bmin + lat * room_size;
            break;
        }

        // Direction: seed-driven pitch + yaw per slot definition.
        //   pitch — angle below horizontal (wall) or off-vertical (ceiling)
        //   yaw   — lateral rotation along the anchor surface
        float pitch = cpu_sample_gaussian(seed, base + IndoorLightProp::AIM_PITCH,
            s.aim_pitch_mean, s.aim_pitch_sigma);
        float yaw = cpu_sample_gaussian(seed, base + IndoorLightProp::AIM_YAW,
            s.aim_yaw_mean, s.aim_yaw_sigma);

        float dx, dy, dz;
        if (s.anchor == LightAnchor::CEILING) {
            // pitch=0 → straight down; positive tilts off-vertical
            pitch = std::clamp(pitch, 0.0f, 0.50f);
            yaw = std::clamp(yaw, -0.60f, 0.60f);
            dx = std::sin(pitch) * std::sin(yaw);
            dy = -std::cos(pitch);
            dz = std::sin(pitch) * std::cos(yaw);
        }
        else {
            // pitch=0 → horizontal into room; π/2 → straight down
            // Floor at 0.08 ensures light never aims upward.
            // Ceiling at 1.45 (~83°) allows steep floor pools.
            pitch = std::clamp(pitch, 0.08f, 1.45f);
            yaw = std::clamp(yaw, -0.80f, 0.80f);
            float cp = std::cos(pitch), sp = std::sin(pitch);
            float sy = std::sin(yaw);
            switch (s.anchor) {
            case LightAnchor::WALL_NORTH: dx = sy; dy = -sp; dz = -cp; break;
            case LightAnchor::WALL_SOUTH: dx = sy; dy = -sp; dz = cp; break;
            case LightAnchor::WALL_EAST:  dx = -cp; dy = -sp; dz = sy; break;
            case LightAnchor::WALL_WEST:  dx = cp; dy = -sp; dz = sy; break;
            default: dx = 0; dy = -1; dz = 0; break;  // unreachable
            }
        }

        // Normalize direction
        float dlen = std::sqrt(dx * dx + dy * dy + dz * dz);
        dx /= dlen; dy /= dlen; dz /= dlen;

        // Intensity and cone angles
        float intensity = std::max(1.0f,
            cpu_sample_gaussian(seed, base + IndoorLightProp::INTENSITY,
                s.intensity_mean, s.intensity_sigma));
        float inner_half = std::max(0.2f,
            cpu_sample_gaussian(seed, base + IndoorLightProp::INNER_CONE,
                s.inner_mean, s.inner_sigma));
        float outer_half = std::max(inner_half + 0.3f,
            cpu_sample_gaussian(seed, base + IndoorLightProp::OUTER_CONE,
                s.outer_mean, s.outer_sigma));
        // Hard cap: outer cone must not exceed shadow map FOV capability.
        // At 1.3 rad (75°), shadow FOV = 2×1.3+0.2 = 2.8 rad → coverable.
        static constexpr float MAX_OUTER_HALF = 1.3f;
        outer_half = std::min(outer_half, MAX_OUTER_HALF);
        inner_half = std::min(inner_half, outer_half - 0.1f);

        // Color: warm amber (1.0,0.85,0.65) ↔ cool blue (0.80,0.88,1.0)
        float w = std::clamp(
            cpu_sample_gaussian(seed, base + IndoorLightProp::WARMTH,
                s.warmth_mean, s.warmth_sigma),
            0.0f, 1.0f);
        float cr = 1.00f + w * (0.80f - 1.00f);
        float cg = 0.85f + w * (0.88f - 0.85f);
        float cb = 0.65f + w * (1.00f - 0.65f);

        L.position[0] = px; L.position[1] = py; L.position[2] = pz;
        L.direction[0] = dx; L.direction[1] = dy; L.direction[2] = dz;
        L.color[0] = cr; L.color[1] = cg; L.color[2] = cb;
        L.intensity = intensity;
        L.inner_cone = std::cos(inner_half);
        L.outer_cone = std::cos(outer_half);
        L.range = (s.anchor == LightAnchor::CEILING)
            ? ceiling_height + 30.0f : room_range;

    }

    cpuSpotLights_.count = count;

    // ─── Vault Uplight ───────────────────────────────────────
    //
    // If ceiling is VAULT and a slot is free, add an upward-facing
    // floor light. Low intensity, very wide cone aimed at the crown.
    // Self-shadowing reveals groin vault ridge structure.
    if (ceiling_type == CeilingType::VAULT && count < MAX_SPOT_LIGHTS) {
        auto& L = cpuSpotLights_.lights[count];
        float center = (bmin + bmax) * 0.5f;
        L.position[0] = center;
        L.position[1] = 2.0f;           // near floor level
        L.position[2] = center;
        L.direction[0] = 0.0f;
        L.direction[1] = 1.0f;           // straight up
        L.direction[2] = 0.0f;
        L.color[0] = 0.95f;
        L.color[1] = 0.90f;
        L.color[2] = 0.80f;              // warm ambient
        L.intensity = 2.5f;               // subtle — structural reveal, not flood
        L.inner_cone = std::cos(0.9f);    // ~52° half-angle
        L.outer_cone = std::cos(1.25f);   // ~72° half-angle — within shadow FOV cap
        L.range = ceiling_height + 80.0f; // reach the crown with headroom
        count++;
        cpuSpotLights_.count = count;
        std::cout << "[Lighting] Added vault uplight (slot " << (count - 1) << ")\n";
    }

    std::cout << "[Lighting] " << SCHEME_NAMES[scheme]
        << " (" << count << " lights, "
        << (use_ew ? "E/W" : "N/S") << " walls)\n";
}

// ═══ APPLY MOOD ══════════════════════════════════════════════════
//
// One canonical mood entry point: apply_mood. Five named sub-
// functions handle atmospheric, spot lighting, indoor shell, band
// motion, and anchor ribbon. The orchestrator owns only ordering
// and the activate-mood bookkeeping. See DONE[mood:K2] in the file
// header for the split rationale.

// 1) Atmospheric: sun direction/color/intensity, fog, ambient,
//    terrain amp ceiling. Touches GPU directly + a few member fields.
void apply_mood_lighting(const MoodProfile& m, wgpu::Queue& /*queue*/) {
    sunDirection_[0] = m.sun_direction[0];
    sunDirection_[1] = m.sun_direction[1];
    sunDirection_[2] = m.sun_direction[2];

    // Push to GPU config so compute_vp builds the shadow VP from the correct direction.
    {
        const float len = std::sqrt(m.sun_direction[0] * m.sun_direction[0] +
                                    m.sun_direction[1] * m.sun_direction[1] +
                                    m.sun_direction[2] * m.sun_direction[2]);
        gpuState_.set_sun_direction(m.sun_direction[0] / len,
                                    m.sun_direction[1] / len,
                                    m.sun_direction[2] / len);
    }

    sunColor_[0] = m.sun_color[0];
    sunColor_[1] = m.sun_color[1];
    sunColor_[2] = m.sun_color[2];
    mood_state_.sun_intensity = m.sun_intensity;
    mood_state_.sun_ambient   = m.sun_ambient;

    clearColor_[0] = m.clear_color[0];
    clearColor_[1] = m.clear_color[1];
    clearColor_[2] = m.clear_color[2];

    gpuState_.set_fog(m.fog_density,
                      m.fog_color[0], m.fog_color[1], m.fog_color[2]);
    gpuState_.set_terrain_amp_ceiling(m.indoor ? 0.5f : 0.0f);
    mood_state_.terrain_amp_ceiling = m.indoor ? 0.5f : 0.0f;
    entities_state_.lights_dirty = true;
}

// 2) Indoor spot lights — only fires when m.indoor. Mutes the sun-VP
//    coupling because the atlas shadow loop owns light_vp in indoor
//    moods (see comment-as-policy at the call site).
void apply_mood_spot_lights(const MoodProfile& m, wgpu::Queue& queue) {
    cpuSpotLights_ = GPUSpotLightArray{};
    if (m.indoor) {
        gpuState_.set_mute_coupling(Coupling::PAWN_TO_SUN_VP, true);

        const float bmin = -(float)world_state_.finite_radius * PATCH_EXTENT;
        const float bmax = ((float)world_state_.finite_radius + 1.0f) * PATCH_EXTENT;
        derive_indoor_lights(world_state_.active_seed, bmin, bmax, m.ceiling_height, m.ceiling_type);

        for (uint32_t i = 0; i < cpuSpotLights_.count; i++) {
            compute_spot_light_vp(cpuSpotLights_.lights[i],
                                  cpuSpotLights_.lights[i].view_proj);
        }
        gpuState_.stage_spot_vps(queue, cpuSpotLights_);
        mood_state_.spot_light_active = true;
    } else {
        gpuState_.set_mute_coupling(Coupling::PAWN_TO_SUN_VP, false);
        mood_state_.spot_light_active = false;
    }
}

// 3) Indoor shell + camera ceiling — only fires when m.indoor.
//    Substitutes a seed-picked INDOOR_PALETTE for the wall+ceiling
//    colors, then matches the crown computation from
//    generate_indoor_shell to set the camera ceiling clamp.
void apply_mood_indoor_shell(const MoodProfile& m, wgpu::Queue& queue) {
    if (m.indoor && m.ceiling_type != CeilingType::NONE) {
        const uint32_t pal_idx = cpu_hash(world_state_.active_seed, 5800u) % INDOOR_PALETTE_COUNT;
        const auto& pal = INDOOR_PALETTES[pal_idx];
        MoodProfile localMood = m;
        for (int c = 0; c < 3; c++) {
            localMood.wall_color[c]    = pal.wall_color[c];
            localMood.ceiling_color[c] = pal.ceiling_color[c];
        }
        std::cout << "[Mood] Indoor palette: " << pal.name
                  << " (idx=" << pal_idx << ")\n";
        generate_indoor_shell(queue, localMood);
    } else {
        clear_indoor_shell(queue);
    }

    // Camera ceiling clamp (matches crown computation in generate_indoor_shell).
    if (m.indoor) {
        float effective_ceiling = m.ceiling_height;
        if (m.ceiling_type == CeilingType::VAULT) {
            const float bmin = -(float)world_state_.finite_radius * PATCH_EXTENT;
            const float bmax = ((float)world_state_.finite_radius + 1.0f) * PATCH_EXTENT;
            const float half_span = (bmax - bmin) * 0.5f;
            const float paint_top = m.ceiling_height * 0.45f + 5.5f;
            const float spring_h  = paint_top + 8.0f;
            const float min_rise  = m.ceiling_height - spring_h;
            const float rise = std::max(half_span * 0.30f, std::max(min_rise, 5.0f));
            effective_ceiling = spring_h + rise;
        }
        gpuState_.set_ceiling_height(effective_ceiling);
    } else {
        gpuState_.set_ceiling_height(0.0f);
    }
}

// 4) Polyphony band motion setup. Snapshots state at mood-entry
//    so per-frame tick_musical_couplings starts from a known origin.
void apply_mood_band_motion() {
    musical_state_.band_motion_active = is_mmode_on(musical_state_, MMODE_TERRAIN_WAVES);
    if (musical_state_.band_motion_active) {
        for (int i = 0; i < 6; i++) {
            musical_state_.band_blend[i]       = 0.0f;
            musical_state_.band_blend_target[i] = 0.0f;
            musical_state_.band_phase_origin[i] = 0.0f;
        }
        gpuState_.set_band_motion(musical_state_.band_blend, musical_state_.band_phase_origin);
        gpuState_.set_terrain_time(0.0f);
    } else {
        float inactive[6] = { -1.f, -1.f, -1.f, -1.f, -1.f, -1.f };
        float zeros[6]    = {};
        gpuState_.set_band_motion(inactive, zeros);
        gpuState_.set_terrain_time(0.0f);
        for (int i = 0; i < 6; i++) {
            musical_state_.band_blend[i]       = -1.0f;
            musical_state_.band_blend_target[i] = 0.0f;
            musical_state_.band_phase_origin[i] = 0.0f;
        }
    }
}

// 5) Anchor ribbon spawn — only fires when MoodProfile.has_anchor_ribbon.
//    Seed-derived position centered on the finite world; goes through
//    fill_ribbon_selection_geometry + commit_ribbon (the dual-entry
//    site flagged at ribbon:dual-entry / mood:K4).
void apply_mood_anchor_ribbon(uint32_t mood, wgpu::Queue& queue) {
    if (!MOOD_TABLE[mood].has_anchor_ribbon) return;

    const uint32_t rseed = tile_seed(world_state_.active_seed, 0, 0);

    // Anchor: seed-derived position spread across the finite world + margin
    const float spread   = ((float)world_state_.finite_radius + 1.5f) * PATCH_EXTENT;
    const float world_cx = 0.5f * PATCH_EXTENT;
    const float world_cz = 0.5f * PATCH_EXTENT;
    const float ax = world_cx + (cpu_hash_f(rseed, RibbonProp::ANCHOR_X) - 0.5f) * spread + ribbon_state_.mood_offset[0];
    const float az = world_cz + (cpu_hash_f(rseed, RibbonProp::ANCHOR_Z) - 0.5f) * spread + ribbon_state_.mood_offset[1];

    // Tier selection (neutral weights — no theme bias in mood)
    const uint32_t tier_idx = select_tier_biased(rseed, RibbonProp::TIER,
        RIBBON_BASE_TIER_WEIGHTS, RIBBON_TIER_COUNT, PopFamily::RIBBON);

    // Sample geometry through the shared helper
    const float terrain_est = estimate_terrain_height(ax, az);
    RibbonSelection sel{};
    sel.seed       = rseed;
    sel.trigger_gx = 0;
    sel.trigger_gz = 0;
    sel.slot       = 0;
    sel.tier_idx   = tier_idx;
    fill_ribbon_selection_geometry(rseed, tier_idx, terrain_est, sel);

    // Build placement (forced position — no negotiation)
    RibbonPlacement plan{};
    plan.slot           = 0;
    plan.trigger_gx     = 0;
    plan.trigger_gz     = 0;
    plan.host_gx        = (int32_t)std::floor(ax / PATCH_EXTENT);
    plan.host_gz        = (int32_t)std::floor(az / PATCH_EXTENT);
    plan.tier_idx       = tier_idx;
    plan.cx             = ax;
    plan.cz             = az;
    plan.rotation       = sel.orientation;
    plan.cube_count     = sel.cube_count;
    plan.cube_size      = sel.cube_size;
    plan.height         = sel.height;
    plan.orientation    = sel.orientation;
    plan.lateral_amp    = sel.lateral_amp;
    plan.lateral_cycles = sel.lateral_cycles;
    plan.lateral_speed  = sel.lateral_speed;
    plan.vertical_amp   = sel.vertical_amp;
    plan.vertical_cycles= sel.vertical_cycles;
    plan.vertical_speed = sel.vertical_speed;
    plan.twist_amp      = sel.twist_amp;
    plan.twist_cycles   = sel.twist_cycles;
    plan.twist_speed    = sel.twist_speed;
    plan.color_mode     = sel.color_mode;
    std::memcpy(plan.color, sel.color, sizeof(plan.color));

    // Commit through the standard path
    commit_ribbon(ribbon_state_, this, plan, 0, 0, queue);

    // Immediate GPU upload (per-frame loop may not run before first render)
    gpuState_.upload_ribbon(queue, ribbon_state_.gpu[0]);
    ribbon_state_.rendered_slot = 0;
}

// ── apply_mood (orchestrator) ──
//
// Owns the mood activation bookkeeping (mood id, feature gates) and
// the order-of-operations of the five sub-functions. The sub-
// functions own the substantive work.
void apply_mood(uint32_t mood, wgpu::Queue& queue) {
    mood = std::min(mood, MOOD_COUNT - 1);
    mood_state_.active = mood;
    const auto& m = MOOD_TABLE[mood];

    // Frustum cull is mood-driven (not tied to indoor/outdoor).
    renderer_.set_frustum_cull_active(m.allow_frustum_cull);

    // Per-mood feature gates: musical modes, GoL zones, aura.
    // Aura policy: respect player preference when permitted, force off when forbidden.
    musical_state_.mood_allows_modes = m.allow_musical_modes;
    gol_state_.mood_allowed     = m.allow_gol_zones;
    if (!m.allow_pawn_aura) pawn_state_.aura_enabled = false;

    apply_mood_lighting(m, queue);          // sun + fog + amp ceiling
    apply_mood_spot_lights(m, queue);       // indoor only
    apply_mood_indoor_shell(m, queue);      // shell + camera ceiling clamp
    apply_mood_band_motion();               // polyphony→bands setup
    reset_musical_couplings(musical_state_, this, queue);         // SEAM[mood:K3] anchor — mode intensities, pulse, palette drift
    apply_mood_anchor_ribbon(mood, queue);  // SEAM[mood:K4]/[mood:L1] anchor — has_anchor_ribbon only
    configure_orbs(orbs_state_, this, ORB_MOOD_TABLE[mood], queue);

    std::cout << "[Mood] Applied: " << mood_name(mood)
        << " (mood=" << mood
        << (m.indoor ? " INDOOR" : " outdoor")
        << (musical_state_.band_motion_active ? " BAND_MOTION" : "")
        << ")\n";
}

// ═══ INDOOR SHELL GENERATION ═════════════════════════════════════
//
// Generates ceiling + wall geometry for indoor moods. Uploaded to
// shell VB/IB and drawn in main + shadow passes.
//
// Flat ceiling: 1 quad. Vault ceiling: tessellated catenary.
// Walls: 4 quads from floor (y=0) to wall_height.

void clear_indoor_shell(wgpu::Queue& queue) {
    gpuState_.set_shell_index_count(0);
    clear_wall_paintings(gallery_state_, this, queue);
}

// Helper: push a quad (2 triangles) into vertex/index vectors
static void push_quad(
    std::vector<ShellVertex>& verts,
    std::vector<uint32_t>& indices,
    float ax, float ay, float az,
    float bx, float by, float bz,
    float cx, float cy, float cz,
    float dx, float dy, float dz,
    float nx, float ny, float nz,
    const float* color
) {
    uint32_t base = static_cast<uint32_t>(verts.size());
    verts.push_back({ { ax, ay, az }, { nx, ny, nz }, { color[0], color[1], color[2] } });
    verts.push_back({ { bx, by, bz }, { nx, ny, nz }, { color[0], color[1], color[2] } });
    verts.push_back({ { cx, cy, cz }, { nx, ny, nz }, { color[0], color[1], color[2] } });
    verts.push_back({ { dx, dy, dz }, { nx, ny, nz }, { color[0], color[1], color[2] } });
    indices.push_back(base); indices.push_back(base + 1); indices.push_back(base + 2);
    indices.push_back(base); indices.push_back(base + 2); indices.push_back(base + 3);
}

void generate_indoor_shell(wgpu::Queue& queue, const MoodProfile& m) {
    float bmin = -(float)world_state_.finite_radius * PATCH_EXTENT;
    float bmax = ((float)world_state_.finite_radius + 1.0f) * PATCH_EXTENT;
    float ch = m.ceiling_height;

    std::vector<ShellVertex> verts;
    std::vector<uint32_t> indices;
    verts.reserve(2400);
    indices.reserve(12000);

    static constexpr float JOINT_OVERLAP = 3.0f;
    static constexpr float WALL_FLOOR = -50.0f;

    // ─── Compute wall height (depends on ceiling type) ───────
    //
    // For vault: spring line must clear paintings. Painting centers
    // sit at ceiling_height × 0.45, half-height ≈ 5.5 units.
    // Spring line = painting_top + margin.
    float wall_h = ch;  // flat ceiling: walls go to ceiling
    float crown_h = ch; // effective crown height for log
    float rise = 0.0f;

    if (m.ceiling_type == CeilingType::VAULT) {
        static constexpr float VAULT_RISE_FRACTION = 0.30f;
        static constexpr float SPRING_MARGIN = 8.0f;

        float half_span = (bmax - bmin) * 0.5f;
        float paint_center = ch * 0.45f;
        float paint_top = paint_center + 5.5f;
        wall_h = paint_top + SPRING_MARGIN;

        float min_rise = ch - wall_h;
        rise = std::max(half_span * VAULT_RISE_FRACTION, std::max(min_rise, 5.0f));
        crown_h = wall_h + rise;
    }

    float wall_top = wall_h + JOINT_OVERLAP;

    // ─── 4 Walls (deep floor to wall_top) ────────────────────
    // South wall (z = bmin, facing +Z into room)
    push_quad(verts, indices,
        bmin, WALL_FLOOR, bmin, bmax, WALL_FLOOR, bmin,
        bmax, wall_top, bmin, bmin, wall_top, bmin,
        0.0f, 0.0f, 1.0f, m.wall_color);
    // North wall (z = bmax, facing -Z into room)
    push_quad(verts, indices,
        bmax, WALL_FLOOR, bmax, bmin, WALL_FLOOR, bmax,
        bmin, wall_top, bmax, bmax, wall_top, bmax,
        0.0f, 0.0f, -1.0f, m.wall_color);
    // West wall (x = bmin, facing +X into room)
    push_quad(verts, indices,
        bmin, WALL_FLOOR, bmax, bmin, WALL_FLOOR, bmin,
        bmin, wall_top, bmin, bmin, wall_top, bmax,
        1.0f, 0.0f, 0.0f, m.wall_color);
    // East wall (x = bmax, facing -X into room)
    push_quad(verts, indices,
        bmax, WALL_FLOOR, bmin, bmax, WALL_FLOOR, bmax,
        bmax, wall_top, bmax, bmax, wall_top, bmin,
        -1.0f, 0.0f, 0.0f, m.wall_color);

    // ─── Ceiling ─────────────────────────────────────────────
    if (m.ceiling_type == CeilingType::FLAT) {
        push_quad(verts, indices,
            bmin, ch, bmin, bmax, ch, bmin,
            bmax, ch, bmax, bmin, ch, bmax,
            0.0f, -1.0f, 0.0f, m.ceiling_color);
    }
    else if (m.ceiling_type == CeilingType::VAULT) {
        // ─── Groin vault (cross vault) ───────────────────────
        //
        // Two perpendicular catenary barrels; ceiling = min of both.
        // Rise scales with room span for real architectural depth.

        float half_x = (bmax - bmin) * 0.5f;
        float half_z = (bmax - bmin) * 0.5f;
        float center_x = (bmin + bmax) * 0.5f;
        float center_z = (bmin + bmax) * 0.5f;
        float cat_a_x = solve_catenary_a(half_x, rise);
        float cat_a_z = solve_catenary_a(half_z, rise);

        static constexpr uint32_t VAULT_N = 32;

        auto catenary_y = [&](float dist, float cat_a) -> float {
            return crown_h - cat_a * (std::cosh(dist / cat_a) - 1.0f);
            };

        uint32_t v_base = static_cast<uint32_t>(verts.size());
        for (uint32_t iz = 0; iz <= VAULT_N; iz++) {
            float tz = (float)iz / (float)VAULT_N;
            float z = bmin + tz * (bmax - bmin);
            float dz = z - center_z;

            for (uint32_t ix = 0; ix <= VAULT_N; ix++) {
                float tx = (float)ix / (float)VAULT_N;
                float x = bmin + tx * (bmax - bmin);
                float dx = x - center_x;

                float y_barrel_x = catenary_y(dx, cat_a_x);
                float y_barrel_z = catenary_y(dz, cat_a_z);
                float y = std::min(y_barrel_x, y_barrel_z);

                bool on_edge = (ix == 0 || ix == VAULT_N || iz == 0 || iz == VAULT_N);
                if (on_edge) {
                    y = wall_h - JOINT_OVERLAP;
                }

                // Normal via finite differences
                float eps = (bmax - bmin) / (float)VAULT_N * 0.5f;
                float y_px = std::min(catenary_y(dx + eps, cat_a_x), catenary_y(dz, cat_a_z));
                float y_mx = std::min(catenary_y(dx - eps, cat_a_x), catenary_y(dz, cat_a_z));
                float y_pz = std::min(catenary_y(dx, cat_a_x), catenary_y(dz + eps, cat_a_z));
                float y_mz = std::min(catenary_y(dx, cat_a_x), catenary_y(dz - eps, cat_a_z));

                float ddx = (y_px - y_mx) / (2.0f * eps);
                float ddz = (y_pz - y_mz) / (2.0f * eps);
                float nmag = std::sqrt(ddx * ddx + 1.0f + ddz * ddz);
                float fnx = -ddx / nmag;
                float fny = -1.0f / nmag;
                float fnz = -ddz / nmag;

                if (on_edge) { fnx = 0.0f; fny = -1.0f; fnz = 0.0f; }

                verts.push_back({ { x, y, z }, { fnx, fny, fnz },
                    { m.ceiling_color[0], m.ceiling_color[1], m.ceiling_color[2] } });
            }
        }

        for (uint32_t iz = 0; iz < VAULT_N; iz++) {
            for (uint32_t ix = 0; ix < VAULT_N; ix++) {
                uint32_t a = v_base + iz * (VAULT_N + 1) + ix;
                uint32_t b = a + 1;
                uint32_t c = a + (VAULT_N + 1);
                uint32_t d = c + 1;
                indices.push_back(a); indices.push_back(b); indices.push_back(c);
                indices.push_back(b); indices.push_back(d); indices.push_back(c);
            }
        }
    }

    uint32_t vc = std::min(static_cast<uint32_t>(verts.size()), Dim::SHELL_MAX_VERTICES);
    uint32_t ic = std::min(static_cast<uint32_t>(indices.size()), Dim::SHELL_MAX_INDICES);

    gpuState_.upload_shell_mesh(queue, verts.data(), vc, indices.data(), ic);

    place_wall_paintings(gallery_state_, this, queue, bmin, bmax, ch);

    std::cout << "[Shell] Generated "
        << (m.ceiling_type == CeilingType::FLAT ? "FLAT" : "GROIN VAULT")
        << ": " << vc << " verts, " << ic << " indices"
        << " bounds=[" << bmin << "," << bmax << "]"
        << " wall_h=" << wall_h << " crown=" << crown_h
        << " rise=" << rise << "\n";
}

// ═══ PORTAL SPAWNING ═════════════════════════════════════════════
//
// Three forced-spawn paths plus the array uploader. All portals are
// arches with portal-color override + portal-destination payload;
// the difference is *where* and *why* they spawn:
//   force_spawn_portal_at      — general helper (called by the others)
//   force_spawn_back_portal    — guaranteed return-path portal in
//                                finite worlds (one per world, perimeter)
//   force_spawn_finite_portals — additional forward portals in finite
//                                worlds, count scales with room size

// ── force_spawn_portal_at ──
//
// General helper: places a Doorway arch with fixed tier-mean geometry,
// portal color, and the given destination. Returns the slot used, or
// UINT32_MAX if no slot was free.
uint32_t force_spawn_portal_at(wgpu::Queue& queue,
    float cx, float cz, float rotation,
    const PortalDestination& dest, bool is_back_portal) {

    uint32_t slot = UINT32_MAX;
    for (uint32_t i = 0; i < Dim::MAX_ARCH_INSTANCES; i++) {
        if (!entities_state_.arches[i].active) { slot = i; break; }
    }
    if (slot == UINT32_MAX) return UINT32_MAX;

    const auto& tp = ARCH_TIERS[static_cast<uint32_t>(ArchTier::DOORWAY)];
    float half_span = tp.profile.params[ArchIdx::SPAN].mean * 0.5f;
    float rise = tp.profile.params[ArchIdx::RISE].mean;
    float depth = tp.profile.params[ArchIdx::DEPTH].mean;
    float thickness = tp.profile.params[ArchIdx::THICKNESS].mean;
    float pier_height = tp.profile.params[ArchIdx::PIER_HEIGHT].mean;
    float pier_padding = tp.profile.params[ArchIdx::PIER_PADDING].mean;
    float edge_blend = tp.profile.params[ArchIdx::EDGE_BLEND].mean;

    float pier_half_x = thickness * 0.5f + pier_padding + edge_blend;
    float pier_half_z = depth * 0.5f + pier_padding + edge_blend;

    int32_t gx = static_cast<int32_t>(std::floor(cx / PATCH_EXTENT));
    int32_t gz = static_cast<int32_t>(std::floor(cz / PATCH_EXTENT));

    float cos_r = std::cos(rotation);
    float sin_r = std::sin(rotation);
    uint32_t pier_l_slot = Dim::PIER_ARCH_BASE + slot * 2;
    uint32_t pier_r_slot = pier_l_slot + 1;

    float pl_x = cx + (-half_span) * cos_r;
    float pl_z = cz + (-half_span) * sin_r;
    float pr_x = cx + half_span * cos_r;
    float pr_z = cz + half_span * sin_r;

    GPUPierInstance pl{};
    pl.origin[0] = pl_x;  pl.origin[1] = pl_z;
    pl.half_size[0] = pier_half_x;  pl.half_size[1] = pier_half_z;
    pl.height_near = pier_height;  pl.height_far = pier_height;
    pl.rotation = rotation;  pl.edge_blend = edge_blend;
    pl.tier = PierTier::ARCH_DOORWAY;
    pl.is_active = 1;
    write_pier(queue, pier_l_slot, pl);

    GPUPierInstance pr{};
    pr.origin[0] = pr_x;  pr.origin[1] = pr_z;
    pr.half_size[0] = pier_half_x;  pr.half_size[1] = pier_half_z;
    pr.height_near = pier_height;  pr.height_far = pier_height;
    pr.rotation = rotation;  pr.edge_blend = edge_blend;
    pr.tier = PierTier::ARCH_DOORWAY;
    pr.is_active = 1;
    write_pier(queue, pier_r_slot, pr);

    auto& aa = entities_state_.arches[slot];
    aa.patch_gx = gx;
    aa.patch_gz = gz;
    aa.active = true;
    aa.draw_visible = true;
    aa.world_x = cx;
    aa.world_z = cz;
    aa.rotation = rotation;
    aa.half_span = half_span;
    aa.total_height = pier_height + rise;
    aa.tier = ArchTier::DOORWAY;
    aa.depth = depth;
    aa.thickness = thickness;
    aa.rise = rise;
    aa.pier_height = pier_height;
    aa.burial = std::max(0.2f, pier_height * tp.burial);
    aa.segs_u = tp.segs_u;
    aa.segs_v = tp.segs_v;
    aa.col_r = 0.75f;  aa.col_g = 0.68f;  aa.col_b = 0.60f;

    {
        // GPU compute_entity_placement handles ground_y from heightfield
        aa.cached_ground_y = 0.0f;
    }

    aa.is_portal = true;
    aa.is_back_portal = is_back_portal;
    aa.position_hash = cpu_hash(static_cast<uint32_t>(cx * 73856093.0f), static_cast<uint32_t>(cz * 19349663.0f));
    aa.destination = dest;

    entities_state_.arch_count++;
    mood_state_.portals_dirty = true;

    const float* pc = is_back_portal
        ? PORTAL_COLOR_BACK
        : PORTAL_COLORS[dest.mood % MOOD_COUNT];
    GPUArchMeshParams meshParams{};
    meshParams.center_x = cx;
    meshParams.center_z = cz;
    meshParams.rotation = rotation;
    meshParams.half_span = half_span;
    meshParams.rise = rise;
    meshParams.depth = depth;
    meshParams.thickness = thickness;
    meshParams.pier_height = pier_height;
    meshParams.burial = aa.burial;
    meshParams.catenary_a = solve_catenary_a(half_span, rise);
    meshParams.segs_u = tp.segs_u;
    meshParams.segs_v = tp.segs_v;
    meshParams.color_r = pc[0];
    meshParams.color_g = pc[1];
    meshParams.color_b = pc[2];
    meshParams.is_active = 1;
    gpuState_.upload_arch_mesh_params_slot(queue, slot, meshParams);
    entities_state_.arch_mesh_gen_pending = true;

    return slot;
}

// ── force_spawn_back_portal ──
//
// Guaranteed return-path portal in finite worlds. Picks a perimeter
// spot on one of the 4 walls (seed-driven side order, jittered
// along the wall), enforces a minimum distance from origin so the
// pawn doesn't land next to it, falls back to first candidate if
// no spot satisfies the origin-distance check.
void force_spawn_back_portal(wgpu::Queue& queue) {
    mood_state_.back_portal_pending = false;

    // ─── Seed-driven placement ───────────────────────────────────────
    //
    // Pick a perimeter spot on one of the 4 walls, jittered along the
    // wall, with two constraints:
    //   1) at least WALL_MARGIN from any wall (avoids pier/wall clipping)
    //   2) at least MIN_FROM_ORIGIN from the spawn point (so the pawn
    //      doesn't land right next to the back-portal)
    //
    // The 4 sides are tried in seed-shuffled order, and the first that
    // satisfies the origin-distance check wins. If none does (only happens
    // in radius-1 worlds where every perimeter point is too close to
    // origin), we fall back to whichever side came up first — better to
    // have a portal nearby than to fail to spawn.
    if (world_state_.finite_mode) {
        float bmin = -(float)world_state_.finite_radius * PATCH_EXTENT;
        float bmax = ((float)world_state_.finite_radius + 1.0f) * PATCH_EXTENT;
        float room_center = (bmin + bmax) * 0.5f;
        float room_half = (bmax - bmin) * 0.5f;

        // Wall margin: footprint-aware in indoor moods (the arch's wall-
        // side pier sits at offset (-half_span) from arch center plus
        // pier_half_x; we add INDOOR_ENTITY_WALL_MARGIN on top so the
        // pier edge stays ≥ that distance from the wall). Outdoor finite
        // worlds have no rendered walls, so the legacy 8 m offset is fine.
        float WALL_MARGIN;
        if (MOOD_TABLE[mood_state_.active].indoor) {
            const auto& doorway = ARCH_TIERS[static_cast<uint32_t>(ArchTier::DOORWAY)].profile;
            const float doorway_half_span = doorway.params[ArchIdx::SPAN].mean * 0.5f;
            const float doorway_pier_half = doorway.params[ArchIdx::THICKNESS].mean * 0.5f
                + doorway.params[ArchIdx::PIER_PADDING].mean
                + doorway.params[ArchIdx::EDGE_BLEND].mean;
            WALL_MARGIN = INDOOR_ENTITY_WALL_MARGIN
                + doorway_half_span + doorway_pier_half;
        }
        else {
            WALL_MARGIN = 8.0f;
        }

        constexpr float MIN_FROM_ORIGIN = 30.0f;
        constexpr float MIN_FROM_ORIGIN_SQ = MIN_FROM_ORIGIN * MIN_FROM_ORIGIN;

        struct Spot { float x, z, rotation; };
        Spot candidates[4] = {
            { room_center,        bmin + WALL_MARGIN,  1.5708f  },  // south, faces +Z
            { bmax - WALL_MARGIN, room_center,         3.14159f },  // east,  faces -X
            { room_center,        bmax - WALL_MARGIN, -1.5708f  },  // north, faces -Z
            { bmin + WALL_MARGIN, room_center,         0.0f     },  // west,  faces +X
        };

        // Fisher–Yates on the side order, seeded from the world seed.
        uint32_t order[4] = { 0, 1, 2, 3 };
        for (uint32_t i = 3; i > 0; i--) {
            uint32_t j = cpu_hash(world_state_.active_seed, 6600u + i) % (i + 1);
            uint32_t tmp = order[i]; order[i] = order[j]; order[j] = tmp;
        }

        bool placed = false;
        float chosen_rotation = 0.0f;
        for (uint32_t k = 0; k < 4 && !placed; k++) {
            uint32_t side = order[k];
            const auto& cand = candidates[side];
            // Jitter along the wall (perpendicular to its inward normal).
            float jitter = (cpu_hash_f(world_state_.active_seed, 6610u + side) - 0.5f) * room_half * 0.4f;
            float x = cand.x, z = cand.z;
            if (side == 0 || side == 2) x += jitter;  // S/N walls run along X
            else                          z += jitter; // E/W walls run along Z

            if (x * x + z * z >= MIN_FROM_ORIGIN_SQ) {
                backPortalPosition_[0] = x;
                backPortalPosition_[1] = z;
                chosen_rotation = cand.rotation;
                placed = true;
            }
        }
        if (!placed) {
            // Fallback for rooms where every perimeter midpoint sits inside
            // the origin-distance ring (radius-1 worlds = 150-unit side, half
            // span 75 — perimeter midpoints are 75–MARGIN ≈ 67 from origin,
            // still > 30, so this branch is rarely reached in practice).
            uint32_t side = order[0];
            backPortalPosition_[0] = candidates[side].x;
            backPortalPosition_[1] = candidates[side].z;
            chosen_rotation = candidates[side].rotation;
        }

        const auto& retMood = MOOD_TABLE[mood_state_.back_portal_return_mood % MOOD_COUNT];
        PortalDestination dest{};
        dest.seed = mood_state_.back_portal_return_seed;
        dest.finite = retMood.finite;
        dest.finite_radius = mood_state_.back_portal_return_radius;
        dest.mood = mood_state_.back_portal_return_mood;

        float cx = backPortalPosition_[0];
        float cz = backPortalPosition_[1];
        uint32_t slot = force_spawn_portal_at(queue, cx, cz, chosen_rotation, dest, true);

        if (slot != UINT32_MAX) {
            std::cout << "[Portal] Back-portal spawned at (" << cx << "," << cz
                << ") rot=" << chosen_rotation << " slot=" << slot
                << " -> return seed=" << mood_state_.back_portal_return_seed
                << " mood=" << mood_name(mood_state_.back_portal_return_mood) << "\n";
        }
        else {
            std::cout << "[Portal] WARNING: no free arch slot for back-portal\n";
        }

        // Spawn additional forward portals around the room perimeter
        force_spawn_finite_portals(queue);
        return;
    }

    // ─── Non-finite fallback (open-world back-portal) ───────────────
    // Open worlds don't normally request back-portals, but if they do
    // we keep the legacy fixed-position behavior at backPortalPosition_.
    const auto& retMood = MOOD_TABLE[mood_state_.back_portal_return_mood % MOOD_COUNT];
    PortalDestination dest{};
    dest.seed = mood_state_.back_portal_return_seed;
    dest.finite = retMood.finite;
    dest.finite_radius = mood_state_.back_portal_return_radius;
    dest.mood = mood_state_.back_portal_return_mood;

    float cx = backPortalPosition_[0];
    float cz = backPortalPosition_[1];
    uint32_t slot = force_spawn_portal_at(queue, cx, cz, 0.0f, dest, true);

    if (slot != UINT32_MAX) {
        std::cout << "[Portal] Back-portal spawned at (" << cx << "," << cz
            << ") slot=" << slot
            << " -> return seed=" << mood_state_.back_portal_return_seed
            << " mood=" << mood_name(mood_state_.back_portal_return_mood) << "\n";
    }
    else {
        std::cout << "[Portal] WARNING: no free arch slot for back-portal\n";
    }

    // Spawn additional forward portals around the room perimeter
    force_spawn_finite_portals(queue);
}

// ── force_spawn_finite_portals ──
//
// Additional forward portals in finite worlds. Count scales with
// room size: radius 1 (3×3) = 1 extra; radius 2 (5×5) = 2 extra;
// radius 3-4 = 3 extra. Positions distributed along perimeter with
// seed-driven jitter so they feel organic, not mechanical.
void force_spawn_finite_portals(wgpu::Queue& queue) {
    float bmin = -(float)world_state_.finite_radius * PATCH_EXTENT;
    float bmax = ((float)world_state_.finite_radius + 1.0f) * PATCH_EXTENT;
    float room_center = (bmin + bmax) * 0.5f;
    float room_half = (bmax - bmin) * 0.5f;

    // Wall margin: in indoor moods, use a footprint-aware computation
    // that keeps the wall-side pier's EDGE at least INDOOR_ENTITY_WALL_MARGIN
    // from the actual wall. Outdoor finite moods (no rendered walls) keep
    // the legacy 8 m offset since there's no visual wall to clip into.
    float margin;
    if (MOOD_TABLE[mood_state_.active].indoor) {
        const auto& doorway = ARCH_TIERS[static_cast<uint32_t>(ArchTier::DOORWAY)].profile;
        const float doorway_half_span = doorway.params[ArchIdx::SPAN].mean * 0.5f;
        const float doorway_pier_half = doorway.params[ArchIdx::THICKNESS].mean * 0.5f
            + doorway.params[ArchIdx::PIER_PADDING].mean
            + doorway.params[ArchIdx::EDGE_BLEND].mean;
        margin = INDOOR_ENTITY_WALL_MARGIN
            + doorway_half_span + doorway_pier_half;
    }
    else {
        margin = 8.0f;
    }

    uint32_t count = 1;
    if (world_state_.finite_radius >= 2) count = 2;
    if (world_state_.finite_radius >= 3) count = 3;

    // Perimeter positions: distribute along the 4 walls
    struct PortalSpot {
        float x, z, rotation;
    };

    // Fixed candidate positions: one per wall, offset from center
    PortalSpot candidates[] = {
        // South wall, facing +Z (into room)
        { room_center, bmin + margin, 1.5708f },
        // East wall, facing -X (into room)
        { bmax - margin, room_center, 3.14159f },
        // North wall, facing -Z (into room)
        { room_center, bmax - margin, -1.5708f },
        // West wall, facing +X (into room)
        { bmin + margin, room_center, 0.0f },
    };
    uint32_t num_candidates = 4;

    // Shuffle candidates with seed so different rooms use different walls
    for (uint32_t i = num_candidates - 1; i > 0; i--) {
        uint32_t j = cpu_hash(world_state_.active_seed, 7700u + i) % (i + 1);
        PortalSpot tmp = candidates[i];
        candidates[i] = candidates[j];
        candidates[j] = tmp;
    }

    uint32_t spawned = 0;
    for (uint32_t i = 0; i < num_candidates && spawned < count; i++) {
        auto& spot = candidates[i];

        // Jitter position along the wall
        float jitter = (cpu_hash_f(world_state_.active_seed, 7710u + i) - 0.5f) * room_half * 0.4f;
        // Apply jitter perpendicular to the wall normal
        float jx = spot.x, jz = spot.z;
        if (std::abs(spot.rotation - 1.5708f) < 0.1f || std::abs(spot.rotation + 1.5708f) < 0.1f) {
            jx += jitter;  // south/north wall: jitter along X
        }
        else {
            jz += jitter;  // east/west wall: jitter along Z
        }

        // Don't collide with back-portal (at backPortalPosition_)
        float dbx = jx - backPortalPosition_[0];
        float dbz = jz - backPortalPosition_[1];
        if (dbx * dbx + dbz * dbz < 10.0f * 10.0f) continue;

        // Generate destination
        uint32_t dest_seed = cpu_hash(world_state_.active_seed, 7800u + i);
        uint32_t mood = pick_portal_mood(world_state_.active_seed, 7900u + i);
        const auto& mp = MOOD_TABLE[mood];
        PortalDestination dest{};
        dest.seed = dest_seed;
        dest.mood = mood;
        dest.finite = mp.finite;
        dest.finite_radius = derive_finite_radius(dest_seed, mp);

        uint32_t slot = force_spawn_portal_at(queue, jx, jz, spot.rotation, dest, false);
        if (slot != UINT32_MAX) {
            std::cout << "[Portal] Forward portal " << (spawned + 1)
                << " at (" << jx << "," << jz
                << ") -> seed=" << dest_seed
                << " mood=" << mood_name(mood)
                << (dest.finite ? " FINITE" : " open") << "\n";
            spawned++;
        }
    }

    std::cout << "[Portal] Finite world: " << spawned << " forward portals + 1 back-portal\n";
}

// ═══ PER-FRAME UPLOAD ════════════════════════════════════════════
//
// Dirty-flagged uploads called by the spine each frame. Portal
// array re-uploads only when an arch's portal state changes;
// lights re-upload only when sun/spot lights change.

// ── upload_portal_array ──
void upload_portal_array(wgpu::Queue& queue) {
    if (!mood_state_.portals_dirty) return;
    mood_state_.portals_dirty = false;

    cpuPortalArray_ = GPUPortalArray{};
    uint32_t count = 0;
    for (uint32_t i = 0; i < Dim::MAX_ARCH_INSTANCES && count < MAX_GPU_PORTALS; i++) {
        if (!entities_state_.arches[i].active || !entities_state_.arches[i].is_portal) continue;
        const auto& aa = entities_state_.arches[i];
        auto& entry = cpuPortalArray_.portals[count];
        entry.x = aa.world_x;
        entry.z = aa.world_z;
        entry.facing_cos = std::cos(aa.rotation);
        entry.facing_sin = std::sin(aa.rotation);
        float half_depth = aa.depth * 0.5f;
        entry.inv_span_sq = 1.0f / (aa.half_span * aa.half_span);
        entry.inv_depth_sq = 1.0f / (half_depth * half_depth);
        entry.arch_index = i;
        entry._pad = 0;
        count++;
    }
    cpuPortalArray_.count = count;
    gpuState_.upload_portal_array(queue, cpuPortalArray_);
}

// ── upload_lights ──
// (Must precede compute for shadow VP.)
void upload_lights(wgpu::Queue& queue) {
    if (!entities_state_.lights_dirty) return;
    entities_state_.lights_dirty = false;

    GPUDirectionalLight sun{};
    float len = std::sqrt(sunDirection_[0] * sunDirection_[0] + sunDirection_[1] * sunDirection_[1] + sunDirection_[2] * sunDirection_[2]);
    sun.direction[0] = sunDirection_[0] / len;
    sun.direction[1] = sunDirection_[1] / len;
    sun.direction[2] = sunDirection_[2] / len;

    sun.color[0] = sunColor_[0];
    sun.color[1] = sunColor_[1];
    sun.color[2] = sunColor_[2];
    sun.intensity = mood_state_.sun_intensity;
    sun.ambient = mood_state_.sun_ambient;

    gpuState_.upload_directional_light(queue, sun);

    GPUPointLightArray pointLights{};
    pointLights.count = 0;
    gpuState_.upload_point_lights(queue, pointLights);

    gpuState_.upload_spot_lights(queue, cpuSpotLights_);
}

// ═══ MOOD TRANSITION REQUEST ═════════════════════════════════════
//
// Single canonical entry point for mood transitions. Bails if a
// transition is already in flight. See DONE[input:L1] in the file
// header for why this lives in mood.inl rather than input.inl
// (one door, many keys).
void request_mood_transition(uint32_t mood) {
    if (transitionPhase_ != TransitionPhase::IDLE) return;
    if (mood >= MOOD_COUNT) return;

    const auto& mp = MOOD_TABLE[mood];
    uint32_t dest_seed = cpu_hash(world_state_.active_seed, 999u);
    uint32_t radius = derive_finite_radius(dest_seed, mp);
    pendingDestination_ = { dest_seed, mp.finite, radius, mood };
    transitionPhase_ = TransitionPhase::FADE_OUT;
    mood_state_.transition_timer = 0.0f;

    if (mp.finite) {
        uint32_t side = 2 * radius + 1;
        std::cout << "[World] Transition (" << mood_name(mood) << " "
            << side << "x" << side << "): seed " << world_state_.active_seed
            << " -> " << pendingDestination_.seed << "\n";
    } else {
        std::cout << "[World] Transition (" << mood_name(mood) << "): seed "
            << world_state_.active_seed << " -> " << pendingDestination_.seed << "\n";
    }
}

