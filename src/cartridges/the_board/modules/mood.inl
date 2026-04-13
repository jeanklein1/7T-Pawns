// ─── mood.inl ──────────────────────────────────────────────────
//
// Atmosphere, indoor lighting, shell geometry, portals.
// derive_indoor_lights → apply_mood → shell → portals → lights.
//
// Included inside the Cartridge class body.
// Depends on: entities.inl, terrain_cpu.inl, seed_utils.inl
// ─────────────────────────────────────────────────────────────────


// ─── Indoor Light Derivation ─────────────────────────────────
//
// Selects a lighting scheme from activeSeed_, then derives
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
            src.aim_yaw_mean, src.aim_yaw_sigma };
    }

    // Derive each light from its slot spec + seed
    for (uint32_t i = 0; i < count; i++) {
        const auto& s = slots[i];
        uint32_t base = IndoorLightProp::SLOT_BASE + i * 10;
        auto& L = cpuSpotLights_.lights[i];

        // Position: slide along anchor surface
        float lat = std::clamp(
            cpu_sample_gaussian(seed, base + IndoorLightProp::LATERAL, 0.5f, 0.15f),
            0.1f, 0.9f);
        float hfrac = std::clamp(
            cpu_sample_gaussian(seed, base + IndoorLightProp::HEIGHT, 0.65f, 0.10f),
            0.4f, 0.85f);

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

// --- Apply mood atmosphere ---
//
// Sets sun direction, sun color/intensity, fog, clear color,
// and indoor shell from the MOOD_TABLE. Called during TEARDOWN.

void apply_mood(uint32_t mood, wgpu::Queue& queue) {
    mood = std::min(mood, MOOD_COUNT - 1);
    activeMood_ = mood;
    const auto& m = MOOD_TABLE[mood];

    sunDirection_[0] = m.sun_direction[0];
    sunDirection_[1] = m.sun_direction[1];
    sunDirection_[2] = m.sun_direction[2];

    // Push to GPU config so compute_vp builds the shadow VP from the correct direction
    {
        float len = std::sqrt(m.sun_direction[0] * m.sun_direction[0] +
            m.sun_direction[1] * m.sun_direction[1] +
            m.sun_direction[2] * m.sun_direction[2]);
        gpuState_.set_sun_direction(m.sun_direction[0] / len,
            m.sun_direction[1] / len,
            m.sun_direction[2] / len);
    }

    sunColor_[0] = m.sun_color[0];
    sunColor_[1] = m.sun_color[1];
    sunColor_[2] = m.sun_color[2];
    sunIntensity_ = m.sun_intensity;
    sunAmbient_ = m.sun_ambient;

    clearColor_[0] = m.clear_color[0];
    clearColor_[1] = m.clear_color[1];
    clearColor_[2] = m.clear_color[2];

    gpuState_.set_fog(m.fog_density,
        m.fog_color[0], m.fog_color[1], m.fog_color[2]);
    gpuState_.set_terrain_amp_ceiling(m.indoor ? 0.5f : 0.0f);
    terrainAmpCeiling_ = m.indoor ? 0.5f : 0.0f;
    lightsDirty_ = true;

    // Spot lights: active only in indoor moods (count=0 disables)
    cpuSpotLights_ = GPUSpotLightArray{};
    if (m.indoor) {
        // Mute the sun VP coupling so compute_vp() stops writing
        // light_vp — it's now owned by CopyBufferToBuffer in the
        // atlas shadow loop.  Without this, the compute write and
        // per-tile copies contend over the same 64-byte slot.
        gpuState_.set_mute_coupling(Coupling::PAWN_TO_SUN_VP, true);

        float bmin = -(float)finiteRadius_ * PATCH_EXTENT;
        float bmax = ((float)finiteRadius_ + 1.0f) * PATCH_EXTENT;

        derive_indoor_lights(activeSeed_, bmin, bmax, m.ceiling_height, m.ceiling_type);

        for (uint32_t i = 0; i < cpuSpotLights_.count; i++) {
            compute_spot_light_vp(cpuSpotLights_.lights[i],
                cpuSpotLights_.lights[i].view_proj);
        }
        gpuState_.stage_spot_vps(queue, cpuSpotLights_);
        spotLightActive_ = true;
    }
    else {
        // Restore the sun VP coupling for outdoor directional shadows.
        gpuState_.set_mute_coupling(Coupling::PAWN_TO_SUN_VP, false);
        spotLightActive_ = false;
    }

    // Indoor shell: generate or clear
    if (m.indoor && m.ceiling_type != CeilingType::NONE) {
        generate_indoor_shell(queue, m);
    }
    else {
        clear_indoor_shell(queue);
    }

    // Camera ceiling clamp: tell the GPU how high the camera can go
    if (m.indoor) {
        float effective_ceiling = m.ceiling_height;
        if (m.ceiling_type == CeilingType::VAULT) {
            // Match the crown computation from generate_indoor_shell
            float bmin = -(float)finiteRadius_ * PATCH_EXTENT;
            float bmax = ((float)finiteRadius_ + 1.0f) * PATCH_EXTENT;
            float half_span = (bmax - bmin) * 0.5f;
            float paint_top = m.ceiling_height * 0.45f + 5.5f;
            float spring_h = paint_top + 8.0f;
            float min_rise = m.ceiling_height - spring_h;
            float rise = std::max(half_span * 0.30f, std::max(min_rise, 5.0f));
            effective_ceiling = spring_h + rise;
        }
        gpuState_.set_ceiling_height(effective_ceiling);
    }
    else {
        gpuState_.set_ceiling_height(0.0f);
    }

    // Polyphony-driven band motion: active when mode toggled on
    // Band motion is retroactively mode 0 — respect mmodeMask_
    bandMotionActive_ = is_mmode_on(MMODE_TERRAIN_WAVES);
    if (bandMotionActive_) {
        for (int i = 0; i < 6; i++) {
            bandBlend_[i] = 0.0f;         // start fully frozen
            bandBlendTarget_[i] = 0.0f;
            bandPhaseOrigin_[i] = 0.0f;
        }
        gpuState_.set_band_motion(bandBlend_, bandPhaseOrigin_);
        gpuState_.set_terrain_time(0.0f);  // will advance per-frame in tick
    }
    else {
        float inactive[6] = { -1.f, -1.f, -1.f, -1.f, -1.f, -1.f };
        float zeros[6] = {};
        gpuState_.set_band_motion(inactive, zeros);
        gpuState_.set_terrain_time(0.0f);  // frozen for non-animated moods
        for (int i = 0; i < 6; i++) {
            bandBlend_[i] = -1.0f;
            bandBlendTarget_[i] = 0.0f;
            bandPhaseOrigin_[i] = 0.0f;
        }
    }

    // Reset all mode intensities on mood change (circuits stay wired, values reset)
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

    // ─── Mood 9 ribbon: seed-derived flying ribbon ───────────────
    if (mood == 5) {
        uint32_t rseed = ribbon_cell_seed(activeSeed_, 0, 0);

        // Anchor: seed-derived position spread across the finite world + margin.
        // The ribbon can land outside terrain bounds — visible from afar.
        float spread = ((float)finiteRadius_ + 1.5f) * PATCH_EXTENT;
        float world_cx = 0.5f * PATCH_EXTENT;   // center of grid origin patch
        float world_cz = 0.5f * PATCH_EXTENT;
        float ax = world_cx + (cpu_hash_f(rseed, RibbonProp::ANCHOR_X) - 0.5f) * spread + moodRibbonOffset_[0];
        float az = world_cz + (cpu_hash_f(rseed, RibbonProp::ANCHOR_Z) - 0.5f) * spread + moodRibbonOffset_[1];

        GPURibbonState ribbon{};
        ribbon.anchor[0] = ax;
        ribbon.anchor[1] = 0.0f;
        ribbon.anchor[2] = az;
        ribbon.time = currentSeconds_;
        float terrain_est = cpu_terrain_base_at(ax, az);
        uint32_t rtier = generate_flying_ribbon(ribbon, rseed, terrain_est);
        gpuState_.upload_ribbon(queue, ribbon);
        activeRibbons_[0].anchor_x = ax;
        activeRibbons_[0].anchor_z = az;
        activeRibbons_[0].active = true;
        activeRibbonCount_ = 1;
        print_ribbon_diagnostic("Mood 9", ribbon, rtier);
    }

    std::cout << "[Mood] Applied: " << mood_name(mood)
        << " (mood=" << mood
        << (m.indoor ? " INDOOR" : " outdoor")
        << (bandMotionActive_ ? " BAND_MOTION" : "")
        << ")\n";
}

// ─── Indoor Shell Generation ─────────────────────────────────
//
// Generates ceiling + wall geometry for indoor moods.
// Uploaded to shell VB/IB and drawn in main + shadow passes.
//
// Flat ceiling: 1 quad.  Vault ceiling: tessellated catenary.
// Walls: 4 quads from floor (y=0) to wall_height.

void clear_indoor_shell(wgpu::Queue& queue) {
    gpuState_.set_shell_index_count(0);
    clear_wall_paintings(queue);
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
    float bmin = -(float)finiteRadius_ * PATCH_EXTENT;
    float bmax = ((float)finiteRadius_ + 1.0f) * PATCH_EXTENT;
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

    place_wall_paintings(queue, bmin, bmax, ch);

    std::cout << "[Shell] Generated "
        << (m.ceiling_type == CeilingType::FLAT ? "FLAT" : "GROIN VAULT")
        << ": " << vc << " verts, " << ic << " indices"
        << " bounds=[" << bmin << "," << bmax << "]"
        << " wall_h=" << wall_h << " crown=" << crown_h
        << " rise=" << rise << "\n";
}

// --- Force-spawn a portal arch at a specific position ---
//
// General helper: places a Doorway arch with fixed tier-mean geometry,
// portal color, and the given destination. Returns the slot used, or
// UINT32_MAX if no slot was free.
uint32_t force_spawn_portal_at(wgpu::Queue& queue,
    float cx, float cz, float rotation,
    const PortalDestination& dest, bool is_back_portal) {

    uint32_t slot = UINT32_MAX;
    for (uint32_t i = 0; i < Dim::MAX_ARCH_INSTANCES; i++) {
        if (!activeArches_[i].active) { slot = i; break; }
    }
    if (slot == UINT32_MAX) return UINT32_MAX;

    const auto& tp = ARCH_TIERS[static_cast<uint32_t>(ArchTier::DOORWAY)];
    float half_span = tp.span_mean * 0.5f;
    float rise = tp.rise_mean;
    float depth = tp.depth_mean;
    float thickness = tp.thickness_mean;
    float pier_height = tp.pier_height_mean;
    float pier_padding = tp.pier_padding_mean;
    float edge_blend = tp.edge_blend_mean;

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

    auto& aa = activeArches_[slot];
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
        float tl = cpu_terrain_base_at(pl_x, pl_z);
        float tr = cpu_terrain_base_at(pr_x, pr_z);
        aa.cached_ground_y = std::min(tl + pier_height, tr + pier_height);
    }

    aa.is_portal = true;
    aa.is_back_portal = is_back_portal;
    aa.position_hash = cpu_hash(static_cast<uint32_t>(cx * 73856093.0f), static_cast<uint32_t>(cz * 19349663.0f));
    aa.destination = dest;

    activeArchCount_++;
    portalsDirty_ = true;

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
    archMeshGenPending_ = true;

    return slot;
}

// --- Force-spawn the guaranteed back-portal ---
void force_spawn_back_portal(wgpu::Queue& queue) {
    backPortalPending_ = false;

    const auto& retMood = MOOD_TABLE[backPortalReturnMood_ % MOOD_COUNT];
    PortalDestination dest{};
    dest.seed = backPortalReturnSeed_;
    dest.finite = retMood.finite;
    dest.finite_radius = backPortalReturnRadius_;
    dest.mood = backPortalReturnMood_;

    float cx = backPortalPosition_[0];
    float cz = backPortalPosition_[1];
    uint32_t slot = force_spawn_portal_at(queue, cx, cz, 0.0f, dest, true);

    if (slot != UINT32_MAX) {
        std::cout << "[Portal] Back-portal spawned at (" << cx << "," << cz
            << ") slot=" << slot
            << " -> return seed=" << backPortalReturnSeed_
            << " mood=" << mood_name(backPortalReturnMood_) << "\n";
    }
    else {
        std::cout << "[Portal] WARNING: no free arch slot for back-portal\n";
    }

    // Spawn additional forward portals around the room perimeter
    force_spawn_finite_portals(queue);
}

// --- Forward portals in finite worlds ---
//
// Places seed-derived portals near the walls so the player can
// easily reach new worlds. Count scales with room size:
//   radius 1 (3×3):  1 extra portal
//   radius 2 (5×5):  2 extra portals
//   radius 3-4:      3 extra portals
//
// Positions are distributed along the room perimeter with seed-driven
// jitter so they feel organic, not mechanical.

void force_spawn_finite_portals(wgpu::Queue& queue) {
    float bmin = -(float)finiteRadius_ * PATCH_EXTENT;
    float bmax = ((float)finiteRadius_ + 1.0f) * PATCH_EXTENT;
    float room_center = (bmin + bmax) * 0.5f;
    float room_half = (bmax - bmin) * 0.5f;
    float margin = 8.0f;  // distance from wall

    uint32_t count = 1;
    if (finiteRadius_ >= 2) count = 2;
    if (finiteRadius_ >= 3) count = 3;

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
        uint32_t j = cpu_hash(activeSeed_, 7700u + i) % (i + 1);
        PortalSpot tmp = candidates[i];
        candidates[i] = candidates[j];
        candidates[j] = tmp;
    }

    uint32_t spawned = 0;
    for (uint32_t i = 0; i < num_candidates && spawned < count; i++) {
        auto& spot = candidates[i];

        // Jitter position along the wall
        float jitter = (cpu_hash_f(activeSeed_, 7710u + i) - 0.5f) * room_half * 0.4f;
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
        uint32_t dest_seed = cpu_hash(activeSeed_, 7800u + i);
        uint32_t mood = pick_portal_mood(activeSeed_, 7900u + i);
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

// --- Upload portal array to GPU ---
void upload_portal_array(wgpu::Queue& queue) {
    if (!portalsDirty_) return;
    portalsDirty_ = false;

    cpuPortalArray_ = GPUPortalArray{};
    uint32_t count = 0;
    for (uint32_t i = 0; i < Dim::MAX_ARCH_INSTANCES && count < MAX_GPU_PORTALS; i++) {
        if (!activeArches_[i].active || !activeArches_[i].is_portal) continue;
        auto& entry = cpuPortalArray_.portals[count];
        entry.x = activeArches_[i].world_x;
        entry.z = activeArches_[i].world_z;
        entry.trigger_radius = PORTAL_TRIGGER_RADIUS;
        entry.arch_index = i;
        count++;
    }
    cpuPortalArray_.count = count;
    gpuState_.upload_portal_array(queue, cpuPortalArray_);
}

// --- Upload light uniforms (must precede compute for shadow VP) ---
void upload_lights(wgpu::Queue& queue) {
    if (!lightsDirty_) return;
    lightsDirty_ = false;

    GPUDirectionalLight sun{};
    float len = std::sqrt(sunDirection_[0] * sunDirection_[0] + sunDirection_[1] * sunDirection_[1] + sunDirection_[2] * sunDirection_[2]);
    sun.direction[0] = sunDirection_[0] / len;
    sun.direction[1] = sunDirection_[1] / len;
    sun.direction[2] = sunDirection_[2] / len;

    sun.color[0] = sunColor_[0];
    sun.color[1] = sunColor_[1];
    sun.color[2] = sunColor_[2];
    sun.intensity = sunIntensity_;
    sun.ambient = sunAmbient_;

    gpuState_.upload_directional_light(queue, sun);

    GPUPointLightArray pointLights{};
    pointLights.count = 0;
    gpuState_.upload_point_lights(queue, pointLights);

    gpuState_.upload_spot_lights(queue, cpuSpotLights_);
}

