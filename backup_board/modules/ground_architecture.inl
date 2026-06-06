// ─── ground_architecture.inl ─────────────────────────────────────
//
// Canonical registry for the ground query architecture:
// contributors, explicit dependency DAG, and policies. Single source
// of truth on the C++ side; world.wgsl mirrors the same ids and
// per-policy bitmasks as `const` values so shader code can refer to
// them by symbol. Keep the two in sync.
//
// See ground_hierarchy_design.md for the design rationale and
// ground_refactor_claude_code_brief.md for how the migration landed.
// The architecture overview comment at the top of the ground section
// in world.wgsl describes contributor classes, extension patterns,
// and the fused-inline hot paths that bypass the query API.
//
// ┌─── Public surface (consumed by other files) ────────────────────┐
// │                                                                  │
// │  Identifiers:                                                    │
// │    ContributorId            — stable id per contributor          │
// │    PolicyId                 — stable id per policy               │
// │    CONTRIB_COUNT, POLICY_COUNT                                   │
// │                                                                  │
// │  Tables:                                                         │
// │    CONTRIBUTOR_DAG[]        — placement-order edges              │
// │    POLICIES[]               — per-policy contributor masks       │
// │                                                                  │
// │  Mask helpers:                                                   │
// │    GROUND_STATIC_BASE_MASK  — fused static-base trio             │
// │                                                                  │
// │  Validation (compile-time only — no runtime symbols exported):   │
// │    DAG closure asserts run at the bottom of this file.           │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// ── Vocabulary ──────────────────────────────────────────────────
//
// A *contributor* is a named source of height (or subtractive
// displacement) at a world XZ. A *policy* is a named filter over the
// contributor set: each consumer declares its policy, and a single
// `query_ground_<policy>` function in world.wgsl evaluates the
// policy-selected contributor sum.
//
// ── What this file declares ─────────────────────────────────────
//
//   ContributorId         Stable id per contributor (0..CONTRIB_COUNT).
//                         Drives bit positions in policy masks and
//                         endpoints in CONTRIBUTOR_DAG edges.
//
//   PolicyId              Stable id per policy (0..POLICY_COUNT).
//
//   CONTRIBUTOR_DAG       Explicit dependency edges among
//                         static_landform contributors. "A → B" means
//                         A is placed beneath B (B's eval composes on
//                         top of A). Deformation fields are not in
//                         the DAG — they act orthogonally.
//
//   POLICIES              Per-policy row: id + name + contributor
//                         bitmask + gradient_supported flag. Indexed
//                         by PolicyId (verified by a count
//                         static_assert).
//
//   policy mask helpers   GROUND_STATIC_BASE_MASK groups the three
//                         fused static-base contributors (lattice,
//                         tile_modifiers, solids) for readability.
//
// ── Compile-time validation ─────────────────────────────────────
//
// A policy's contributor set must be *closed under CONTRIBUTOR_DAG*:
// for every edge {from, to}, if the mask contains `to` it must also
// contain `from`. This prevents policies like "pyramids without
// terrain_lattice" from compiling.
//
// Expressed as a macro expansion (ASSERT_POLICY_DAG_CLOSED) rather
// than a constexpr validator call because C++ class-body
// static_asserts cannot invoke member constexpr functions defined in
// the same class (the enclosing type isn't complete at the point of
// the static_assert). DAG_EDGE_CLOSED is a pure bit-arithmetic
// predicate — given the mask and an edge, it checks
// `(mask has to) ⇒ (mask has from)` as a constant expression.
//
// Every policy runs the check for every DAG edge at compile time;
// adding a new policy means adding one ASSERT_POLICY_DAG_CLOSED
// invocation at the bottom of this file.
//
// Included inside the Cartridge class body.
// Depends on: nothing (pure enum + table definitions + macro checks).
//
// SEAM[ground_architecture:P9] header-style file: pure declarations
//   (enums + tables) + compile-time validation macros, zero runtime
//   logic, no class-member coupling. Same family as entity_types.inl,
//   musical/trajectory.hpp, seed_utils.inl as P9 instances. The
//   ASSERT_POLICY_DAG_CLOSED macro pattern is the unique
//   contribution of this file — compile-time relational integrity
//   over a registry table.
// SEAM[ground_architecture:contract] CONTRIBUTOR_COUNT, POLICY_COUNT,
//   the contributor enum values, and POLICIES bitmasks must mirror
//   identical const values in world.wgsl. Drift would mean
//   query_ground_<policy> evaluates a different contributor set on
//   GPU than what CPU placement believed. Same family as agents:L2
//   and seed_utils:contract.
// ─────────────────────────────────────────────────────────────────

enum ContributorId : uint32_t {
    CONTRIB_TERRAIN_LATTICE   = 0,   // fused into contrib_static_base_at
    CONTRIB_TILE_MODIFIERS    = 1,   // fused into contrib_static_base_at
    CONTRIB_SOLIDS            = 2,   // piers, ramps; fused into contrib_static_base_at
    CONTRIB_PYRAMIDS          = 3,
    CONTRIB_PAINTINGS_BASES   = 4,   // placeholder (stub contributor — returns 0.0)
    CONTRIB_VEGETATION_BASES  = 5,   // placeholder (stub contributor — returns 0.0)
    CONTRIB_GOL_ZONES         = 6,   // slow_dynamic
    CONTRIB_TERRAIN_WAVES     = 7,   // deformation_field, global
    CONTRIB_RADIAL_PULSES     = 8,   // deformation_field, global
    CONTRIB_PAWN_AURA         = 9,   // deformation_field, global pawn-centered
    CONTRIB_GOL_SUPPRESSION   = 10,  // deformation_field, consumer-local (subtractive)
    CONTRIB_COUNT             = 11,
};

enum PolicyId : uint32_t {
    POLICY_PLACEMENT_PYRAMID    = 0,
    POLICY_PLACEMENT_PAINTING   = 1,
    POLICY_PLACEMENT_VEGETATION = 2,
    POLICY_BAKED_HEIGHTFIELD    = 3,
    POLICY_FLYER                = 4,
    POLICY_WALKER               = 5,
    POLICY_WALKER_TILT          = 6,   // walker minus self-centered fields (aura, suppression)
    POLICY_WALKER_AGENT         = 7,
    POLICY_CELESTIAL            = 8,
    POLICY_COUNT                = 9,
};

// ═══ DEPENDENCY DAG ══════════════════════════════════════════════
//
// A → B means "A is placed beneath B" — B's evaluation composes on
// top of A. Edges exist only among static_landform contributors.
// Deformation fields are orthogonal to the DAG (applied additively
// across the static+dynamic sum).
//
// Note: LATTICE, TILE_MODIFIERS, SOLIDS fuse into contrib_static_base_at
// at the shader level; the edges are still declared here so policy
// closure validation works on logical contributor ids.

struct ContributorEdge {
    ContributorId from;
    ContributorId to;
};

static constexpr ContributorEdge CONTRIBUTOR_DAG[] = {
    { CONTRIB_TERRAIN_LATTICE,  CONTRIB_PYRAMIDS         },
    { CONTRIB_TILE_MODIFIERS,   CONTRIB_PYRAMIDS         },
    { CONTRIB_SOLIDS,           CONTRIB_PYRAMIDS         },
    { CONTRIB_PYRAMIDS,         CONTRIB_PAINTINGS_BASES  },
    { CONTRIB_SOLIDS,           CONTRIB_PAINTINGS_BASES  },
    { CONTRIB_SOLIDS,           CONTRIB_VEGETATION_BASES },
};
static constexpr uint32_t CONTRIBUTOR_DAG_EDGE_COUNT =
    sizeof(CONTRIBUTOR_DAG) / sizeof(CONTRIBUTOR_DAG[0]);

// ═══ POLICY DEFINITIONS ══════════════════════════════════════════
//
// Each policy declares its contributor set as a bitmask indexed by
// ContributorId, plus a flag for gradient evaluation support.

struct PolicyDef {
    PolicyId    id;
    const char* name;
    uint32_t    contributors;       // bitmask: bit k set iff contributor k is in the policy
    bool        gradient_supported;
};

// The three fused static-base contributors travel together in every
// policy that wants a landform base.
static constexpr uint32_t GROUND_STATIC_BASE_MASK =
    (1u << CONTRIB_TERRAIN_LATTICE) |
    (1u << CONTRIB_TILE_MODIFIERS)  |
    (1u << CONTRIB_SOLIDS);

static constexpr PolicyDef POLICIES[] = {
    // Placement policies — spawn-time Y correction. No deformation
    // fields (placement should be stable against animated terrain).
    { POLICY_PLACEMENT_PYRAMID, "placement_pyramid",
      GROUND_STATIC_BASE_MASK,
      /*gradient=*/false },

    { POLICY_PLACEMENT_PAINTING, "placement_painting",
      GROUND_STATIC_BASE_MASK
        | (1u << CONTRIB_PYRAMIDS)
        | (1u << CONTRIB_GOL_ZONES),       // paintings sit on current GoL (preserves pre-refactor behavior)
      /*gradient=*/false },

    { POLICY_PLACEMENT_VEGETATION, "placement_vegetation",
      GROUND_STATIC_BASE_MASK,              // trees don't stand on pyramids
      /*gradient=*/false },

    // Baked heightfield — cached static ground texture consumed by
    // patch VS interpolation and CPU readbacks.
    { POLICY_BAKED_HEIGHTFIELD, "baked_heightfield",
      GROUND_STATIC_BASE_MASK
        | (1u << CONTRIB_PYRAMIDS),
      /*gradient=*/false },

    // Fly-over policy — spheres, cubes, cameras. Includes all global
    // deformation fields so flyers ride pulses and auras.
    { POLICY_FLYER, "flyer",
      GROUND_STATIC_BASE_MASK
        | (1u << CONTRIB_PYRAMIDS)
        | (1u << CONTRIB_GOL_ZONES)
        | (1u << CONTRIB_TERRAIN_WAVES)
        | (1u << CONTRIB_RADIAL_PULSES)
        | (1u << CONTRIB_PAWN_AURA),
      /*gradient=*/true },

    // Walker — the pawn. Everything flyer includes, plus the
    // consumer-local GoL suppression that flattens the zone under
    // the querying consumer's feet.
    { POLICY_WALKER, "walker",
      GROUND_STATIC_BASE_MASK
        | (1u << CONTRIB_PYRAMIDS)
        | (1u << CONTRIB_GOL_ZONES)
        | (1u << CONTRIB_TERRAIN_WAVES)
        | (1u << CONTRIB_RADIAL_PULSES)
        | (1u << CONTRIB_PAWN_AURA)
        | (1u << CONTRIB_GOL_SUPPRESSION),
      /*gradient=*/true },

    // Walker-tilt — walker minus self-centered fields (PAWN_AURA and
    // GOL_SUPPRESSION), used for tilt/normal computation and step-climb
    // decisions. Including self-centered contributors in the gradient
    // produces manufactured slopes (the pawn would tilt against its own
    // aura's radial profile). The pawn still STANDS on full POLICY_WALKER
    // ground; only the tilt and step decisions read this policy.
    { POLICY_WALKER_TILT, "walker_tilt",
      GROUND_STATIC_BASE_MASK
        | (1u << CONTRIB_PYRAMIDS)
        | (1u << CONTRIB_GOL_ZONES)
        | (1u << CONTRIB_TERRAIN_WAVES)
        | (1u << CONTRIB_RADIAL_PULSES),
      /*gradient=*/true },

    // Walker-agent — agents feel the full GoL lift (no suppression).
    { POLICY_WALKER_AGENT, "walker_agent",
      GROUND_STATIC_BASE_MASK
        | (1u << CONTRIB_PYRAMIDS)
        | (1u << CONTRIB_GOL_ZONES)
        | (1u << CONTRIB_TERRAIN_WAVES)
        | (1u << CONTRIB_RADIAL_PULSES)
        | (1u << CONTRIB_PAWN_AURA),
      /*gradient=*/true },

    // Celestial — no ground (sun, stars). Symmetry slot.
    { POLICY_CELESTIAL, "celestial",
      0u,
      /*gradient=*/false },
};
static constexpr uint32_t POLICY_COUNT_IN_TABLE =
    sizeof(POLICIES) / sizeof(POLICIES[0]);

static_assert(POLICY_COUNT_IN_TABLE == POLICY_COUNT,
              "POLICIES table must declare one row per PolicyId");

// ═══ COMPILE-TIME DAG-CLOSURE VALIDATION ═════════════════════════
//
// For every contributor in a policy's set, all of its ancestors via
// CONTRIBUTOR_DAG must also be in the set. Expressed as one
// static_assert per (policy, edge) pair via the DAG_EDGE_CLOSED macro:
//
//   mask has `to` → mask has `from`
// = !(mask has `to`) || mask has `from`
//
// Using a macro avoids calling a member constexpr function from a
// class-body static_assert (which fails because the enclosing class
// isn't complete at that point).

#define DAG_EDGE_CLOSED(MASK, FROM, TO) \
    ( (((MASK) >> (TO)) & 1u) == 0u || (((MASK) >> (FROM)) & 1u) != 0u )

#define ASSERT_POLICY_DAG_CLOSED(POLICY_IDX, POLICY_NAME)                                                       \
    static_assert(DAG_EDGE_CLOSED(POLICIES[POLICY_IDX].contributors,                                            \
                                  CONTRIB_TERRAIN_LATTICE, CONTRIB_PYRAMIDS),                                   \
                  POLICY_NAME ": includes PYRAMIDS but not TERRAIN_LATTICE");                                   \
    static_assert(DAG_EDGE_CLOSED(POLICIES[POLICY_IDX].contributors,                                            \
                                  CONTRIB_TILE_MODIFIERS, CONTRIB_PYRAMIDS),                                    \
                  POLICY_NAME ": includes PYRAMIDS but not TILE_MODIFIERS");                                    \
    static_assert(DAG_EDGE_CLOSED(POLICIES[POLICY_IDX].contributors,                                            \
                                  CONTRIB_SOLIDS, CONTRIB_PYRAMIDS),                                            \
                  POLICY_NAME ": includes PYRAMIDS but not SOLIDS");                                            \
    static_assert(DAG_EDGE_CLOSED(POLICIES[POLICY_IDX].contributors,                                            \
                                  CONTRIB_PYRAMIDS, CONTRIB_PAINTINGS_BASES),                                   \
                  POLICY_NAME ": includes PAINTINGS_BASES but not PYRAMIDS");                                   \
    static_assert(DAG_EDGE_CLOSED(POLICIES[POLICY_IDX].contributors,                                            \
                                  CONTRIB_SOLIDS, CONTRIB_PAINTINGS_BASES),                                     \
                  POLICY_NAME ": includes PAINTINGS_BASES but not SOLIDS");                                     \
    static_assert(DAG_EDGE_CLOSED(POLICIES[POLICY_IDX].contributors,                                            \
                                  CONTRIB_SOLIDS, CONTRIB_VEGETATION_BASES),                                    \
                  POLICY_NAME ": includes VEGETATION_BASES but not SOLIDS")

ASSERT_POLICY_DAG_CLOSED(POLICY_PLACEMENT_PYRAMID,    "POLICY_PLACEMENT_PYRAMID");
ASSERT_POLICY_DAG_CLOSED(POLICY_PLACEMENT_PAINTING,   "POLICY_PLACEMENT_PAINTING");
ASSERT_POLICY_DAG_CLOSED(POLICY_PLACEMENT_VEGETATION, "POLICY_PLACEMENT_VEGETATION");
ASSERT_POLICY_DAG_CLOSED(POLICY_BAKED_HEIGHTFIELD,    "POLICY_BAKED_HEIGHTFIELD");
ASSERT_POLICY_DAG_CLOSED(POLICY_FLYER,                "POLICY_FLYER");
ASSERT_POLICY_DAG_CLOSED(POLICY_WALKER,               "POLICY_WALKER");
ASSERT_POLICY_DAG_CLOSED(POLICY_WALKER_TILT,          "POLICY_WALKER_TILT");
ASSERT_POLICY_DAG_CLOSED(POLICY_WALKER_AGENT,         "POLICY_WALKER_AGENT");
ASSERT_POLICY_DAG_CLOSED(POLICY_CELESTIAL,            "POLICY_CELESTIAL");

#undef DAG_EDGE_CLOSED
#undef ASSERT_POLICY_DAG_CLOSED
