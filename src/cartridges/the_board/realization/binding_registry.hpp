#pragma once
// ═══════════════════════════════════════════════════════════════════════
// THE BINDING REGISTRY (C6) — the single source of truth for GPU binding
// NUMBERS. Every bind-group layout entry AND its matching group entry
// reference the SAME named constant here, so the "binding integer typed
// twice" hazard is a compile error, not a
// runtime crash: a typo is an undefined symbol glaw1 catches.
//
// GROUP-SCOPED (forced, not stylistic). A binding number is a per-@group
// key: 22 = terrain_mesh_indices in group 0 AND bilinear_sampler in group
// 1; 25 = tile_grid vs shadow_map; 26 = pier_instances vs shadow_sampler.
// A flat list would fuse
// distinct slots — so g0:: and g1:: are separate namespaces.
//
// AUTHORED, not computed (Frame-Spine law, one layer down). Every constant
// is the literal number authored ONCE. The render = compute + 200 band is
// a static_assert WITNESS at the bottom — it CHECKS the authored literals,
// it is never their SOURCE.
//
// ONE CONSTANT PER SITE, named for the site (the WGSL variable it mirrors),
// NOT one-per-buffer. The number addresses a (group, slot); the shared
// buffer handle lives in the group entry's `.buffer`, not here. So the same
// buffer wears several names: patch_instances(340) /
// zone_patch_instances(165); orb_state(410) / render_orb_state(400)
// / orb_state_ro(413). Each name = one (group, slot).
//
// THE CEILING (Option A). This single-sources the two C++ copies (layout +
// group). The WGSL @binding literals in world.wgsl (102 declarations over 97
// slots — audit cc7; five documented fc_ aliases share slots) stay a MIRROR — the
// shader cannot read a C++ constant — kept in lockstep by the crash-aware
// launch gate (bind-group + pipeline validation), NOT the compiler. The
// name here deliberately equals the WGSL var so the mirror is greppable in
// both files. Closing the third copy (a generated block / token-substituted
// shader — feasible since world.wgsl is loaded as runtime text) is the
// named "true pin" follow-on, not this cut.
// ═══════════════════════════════════════════════════════════════════════

#include <cstdint>

namespace t7 {
    namespace the_board {
        namespace bind {

            // ─────────────────────────────────────────────────────────────
            // GROUP 0 — the everything-group: uniforms + storage sim state +
            // write storage-textures + mesh-gen scratch + the render_* read
            // mirrors. Numeric range 0–501, banded by subsystem.
            // ─────────────────────────────────────────────────────────────
            namespace g0 {
                // core (0–2)
                inline constexpr uint32_t signal                     = 0;
                inline constexpr uint32_t config                     = 1;
                inline constexpr uint32_t vp_data                    = 2;   // aka fc_vp (frustum-cull alias)

                // terrain / patch lattice (20–30)
                // (terrain_state @20 / render_terrain @220 REMOVED:
                //  the dead GPUTerrainState buffer's bindings, no shader reader.)
                inline constexpr uint32_t terrain_mesh_indices       = 22;
                inline constexpr uint32_t patch_params               = 23;
                inline constexpr uint32_t patch_heightfield_array_write = 24;
                inline constexpr uint32_t tile_grid                  = 25;
                inline constexpr uint32_t pier_instances             = 26;
                inline constexpr uint32_t patch_cell_color_array_write = 27;
                inline constexpr uint32_t patch_height_scratch       = 28;
                // (cell_fields_write = 29 RETIRED — Commit C, the LUT retirement.
                //  Number reserved; do not reuse.)
                inline constexpr uint32_t pyramid_instances          = 30;
                inline constexpr uint32_t live_card_write            = 31;  // GROUND_CARD_1: the live card (storage-tex write; writer kernel)

                // agents / camera (60–101)
                inline constexpr uint32_t agent_state                = 60;   // aka fc_agents
                inline constexpr uint32_t portal_array               = 62;
                inline constexpr uint32_t camera_state               = 80;   // aka fc_camera
                inline constexpr uint32_t floating_entities          = 100;

                // agent registries (110–111)
                inline constexpr uint32_t agent_behaviors            = 110;
                inline constexpr uint32_t agent_tier_gains           = 111;

                // ribbon (120–122)
                inline constexpr uint32_t ribbon_state               = 120;
                inline constexpr uint32_t ring_xforms                = 121;
                inline constexpr uint32_t head_poses                 = 122;

                // photographer / placement (140–152)
                inline constexpr uint32_t photographer_config        = 140;
                inline constexpr uint32_t photographer_vp            = 141;
                inline constexpr uint32_t photographer_camera_out    = 142;
                inline constexpr uint32_t photo_painting_slots       = 143;
                inline constexpr uint32_t photo_heightfield          = 145;
                inline constexpr uint32_t photo_sampler              = 146;
                inline constexpr uint32_t arch_ground                = 147;
                inline constexpr uint32_t column_ground              = 148;
                // 149 RETIRED — pyramid_ground (the ground-atlas residue;
                // its slot-48 write was reader-free). Number parked.
                inline constexpr uint32_t plant_ground               = 150;
                inline constexpr uint32_t entity_ground_atlas_write  = 151;
                inline constexpr uint32_t patch_grid                 = 152;

                // GoL zones / pawn aura (160–172)
                inline constexpr uint32_t zone_config                = 160;
                inline constexpr uint32_t zone_life                  = 161;
                inline constexpr uint32_t zone_life_tex_write        = 162;
                inline constexpr uint32_t zone_heightfield           = 163;
                inline constexpr uint32_t zone_hf_sampler            = 164;
                inline constexpr uint32_t zone_patch_instances       = 165;
                inline constexpr uint32_t zone_derive_requests       = 166;
                inline constexpr uint32_t zone_mesh_vertices         = 167;
                inline constexpr uint32_t zone_mesh_indices          = 168;
                inline constexpr uint32_t zone_mesh_indirect         = 169;
                inline constexpr uint32_t pawn_aura_cfg              = 170;
                inline constexpr uint32_t pawn_aura_cells            = 171;
                inline constexpr uint32_t pawn_aura_tex_write        = 172;

                // mesh-gen scratch: palm/cactus/blade (180–188)
                inline constexpr uint32_t palmg_params               = 180;
                inline constexpr uint32_t palmg_vertices             = 181;
                inline constexpr uint32_t palmg_indices              = 182;
                inline constexpr uint32_t cactusg_params             = 183;
                inline constexpr uint32_t cactusg_vertices           = 184;
                inline constexpr uint32_t cactusg_indices            = 185;
                inline constexpr uint32_t bladeg_params              = 186;
                inline constexpr uint32_t bladeg_vertices            = 187;
                inline constexpr uint32_t bladeg_indices             = 188;

                // mesh-gen scratch: arch/column (190–198)
                // (pmg_* @190/191/192 REMOVED: the pyramid mesh-gen
                //  basket, no entry point / pipeline / dispatch ever used it.
                //  190/191 REBORN as the cmg ceiling-fit pair — parked-number
                //  reuse per the 149 precedent; 192 stays parked.)
                inline constexpr uint32_t cmg_config                 = 190;  // DesignConfig view for the cmg kernel (the ceiling gate)
                inline constexpr uint32_t cmg_column_ground          = 191;  // read-only column_ground view (the terrain delta)
                inline constexpr uint32_t amg_params                 = 193;
                inline constexpr uint32_t amg_vertices               = 194;
                inline constexpr uint32_t amg_indices                = 195;
                inline constexpr uint32_t cmg_params                 = 196;
                inline constexpr uint32_t cmg_vertices               = 197;
                inline constexpr uint32_t cmg_indices                = 198;

                // render mirrors — the +200 band (200–361)
                inline constexpr uint32_t render_signal              = 200;
                inline constexpr uint32_t render_vp                  = 201;
                inline constexpr uint32_t render_agents              = 260;
                inline constexpr uint32_t render_camera              = 280;
                inline constexpr uint32_t render_floating            = 300;
                inline constexpr uint32_t render_light               = 320;
                inline constexpr uint32_t render_point_lights        = 321;
                inline constexpr uint32_t render_spot_lights         = 322;
                inline constexpr uint32_t patch_instances            = 340;   // aka fc_patches
                inline constexpr uint32_t render_ribbon              = 360;
                inline constexpr uint32_t render_ring_xforms         = 361;

                // atlas / cull outputs / orbs (390–501)
                inline constexpr uint32_t entity_ground_atlas        = 390;
                inline constexpr uint32_t visible_patch_indices      = 391;
                inline constexpr uint32_t render_orb_state           = 400;
                inline constexpr uint32_t orb_state                  = 410;
                inline constexpr uint32_t orb_config                 = 411;
                inline constexpr uint32_t orb_state_prev             = 412;
                inline constexpr uint32_t orb_state_ro               = 413;
                inline constexpr uint32_t orb_state_prev_rw          = 414;
                inline constexpr uint32_t fc_visible                 = 500;
                inline constexpr uint32_t fc_indirect                = 501;
            } // namespace g0

            // ─────────────────────────────────────────────────────────────
            // GROUP 1 — shared samplers + read-side sampled textures. Numeric
            // range 22–52. (These numbers are UNRELATED to the group-0 numbers
            // of the same value — that is why the namespace exists.)
            // ─────────────────────────────────────────────────────────────
            namespace g1 {
                inline constexpr uint32_t bilinear_sampler           = 22;
                inline constexpr uint32_t nearest_sampler            = 23;
                inline constexpr uint32_t shadow_map                 = 25;
                inline constexpr uint32_t shadow_sampler             = 26;
                inline constexpr uint32_t spot_shadow_map            = 27;
                inline constexpr uint32_t patch_heightfield_array_read = 28;
                inline constexpr uint32_t patch_cell_color_array_read  = 29;
                // (cell_fields_read = 30 RETIRED — Commit C. Number reserved.)
                inline constexpr uint32_t zone_life_read             = 31;
                inline constexpr uint32_t zone_params                = 32;
                inline constexpr uint32_t pawn_aura_read             = 33;
                inline constexpr uint32_t live_card_read             = 34;  // GROUND_CARD_1: the live card (sampled read; render + compute)
                inline constexpr uint32_t painting_slots             = 50;
                inline constexpr uint32_t painting_array             = 51;
                inline constexpr uint32_t painting_sampler_filt      = 52;
            } // namespace g1

            // ─────────────────────────────────────────────────────────────
            // WITNESSES — the render = compute + 200 band. These CHECK the
            // authored literals above; they are NOT the source of any value.
            // If a future edit breaks the band, this fails the BUILD (loud),
            // it does not silently re-derive the number.
            // ─────────────────────────────────────────────────────────────
            static_assert(g0::render_signal  == g0::signal  + 200, "render band: signal");
            static_assert(g0::render_vp      == g0::vp_data  + 199, "render band: vp (2 -> 201)");
            static_assert(g0::render_agents  == g0::agent_state  + 200, "render band: agents");
            static_assert(g0::render_camera  == g0::camera_state + 200, "render band: camera");
            static_assert(g0::render_floating == g0::floating_entities + 200, "render band: floating");

        } // namespace bind
    } // namespace the_board
} // namespace t7
