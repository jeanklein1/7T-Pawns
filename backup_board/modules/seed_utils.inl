// ─── seed_utils.inl ──────────────────────────────────────────────
//
// Pure math. Hash, Gaussian, tier selection.
// No member state. No domain knowledge.
// Every layer below depends on these; they depend on nothing.
//
// Public surface:
//   cpu_hash(seed, prop)                   — uint32_t deterministic hash
//   cpu_hash_f(seed, prop)                 — float in [0, 1)
//   tile_seed(master_seed, gx, gz)         — patch-seed derivation
//   cpu_lattice_node_seed(s, nx, nz, band) — lattice-node-seed derivation
//   cpu_smoothstep(e0, e1, x)              — smoothstep
//   cpu_sample_gaussian(s, prop, μ, σ)     — Box-Muller, ±3σ truncated
//   select_tier(seed, prop, weights, n)    — cumulative-weight selection
//
// Included inside the Cartridge class body.
// Depends on: nothing (foundations layer — pure math).
//
// SEAM[seed_utils:P9] textbook "library without state" module — pure
//   functions, no class members referenced, no domain assumptions.
//   Easiest module to extract: zero compilation-order constraints,
//   no semantic change. Same family as musical/trajectory.hpp and
//   entity_types.inl as P9 instances.
// SEAM[seed_utils:contract] cpu_lattice_node_seed and cpu_sample_gaussian
//   are FXC mirrors — must produce identical bit-for-bit results to the
//   WGSL counterparts (lattice_node_seed, sample_gaussian). The
//   per-function comments name this. Same family as agents:L2 and
//   state.hpp's GPU struct contract.
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
//
// SEAM[seed_utils:Q10-target] this is the canonical cumulative-weight
//   tier-picker. Several modules (gol_zones.inl, ribbon.inl,
//   entity_pipeline.inl per-family adapters) currently roll their
//   own copy of this loop. Replacing those with select_tier calls is
//   the Q10 consolidation task in the rollout open-questions report.
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
