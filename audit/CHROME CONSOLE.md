(index):27 
(index):27 ========================================
(index):27   INCUBATOR DUAL (web twin — no hot reload)
(index):27   Clock:    BeatClock
(index):27   Render:   the_board
(index):27 ========================================
(index):27 
(index):27 [Device] limits path: core defaults + censused exceptions
(index):27 [Incubator] BeatClock ready (bpm 100)
(index):27 [GPUState] Design Config: 624 B (C++ side; WGSL DesignConfig mirror must match)
(index):27 [GPUState] Monolith mesh: 24 verts, 36 indices
(index):27 [GPUState] Arch buffers (GPU mesh gen): 32000 vert, 120000 index capacity
(index):27 [GPUState] Column buffers (GPU mesh gen): 48000 vert, 192000 index capacity
(index):27 [GPUState] Shell buffers: 2048 vert, 8192 index capacity
(index):27 [GPUState] GoL zone buffers: 8 zones × 32×32 grid
(index):27 [Cartridge] GPUState init:    15 ms
(index):27 [SPINE] validated: 9 update rows + 22 render rows + 12 dispatch rows name-checked; O-#/RC laws static-asserted
(index):27 Loaded shader from: ../../../src/cartridges/the_board/realization/world.wgsl
(index):27 [Renderer] Shader compile:    18 ms
(index):27   [Pipeline] update_player_agent: 0 ms
(index):27   [Pipeline] update_other_agents: 0 ms
(index):27   [Pipeline] update_camera: 0 ms
(index):27   [Pipeline] update_sphere: 0 ms
(index):27   [Pipeline] update_cube: 0 ms
(index):27   [Pipeline] compute_vp: 0 ms
(index):27   [Pipeline] gen_patch_heights: 0 ms
(index):27   [Pipeline] gen_patch_gradients: 0 ms
(index):27   [Pipeline] gen_patch_cells: 0 ms
(index):27   [Pipeline] compute_ribbon_rings: 0 ms
(index):27   [Pipeline] compute_photographer_vp: 0 ms
(index):27   [Pipeline] compute_entity_placement: 0 ms
(index):27   [Pipeline] frustum_cull_patches: 0 ms
(index):27   [Pipeline] compute_pawn_aura: 0 ms
(index):27   [Pipeline] write_live_card_heights: 0 ms
(index):27   [Pipeline] write_live_card_resolve: 0 ms
(index):27   [Pipeline] orb_init: 0 ms
(index):27   [Pipeline] orb_dynamics: 0 ms
(index):27   [Pipeline] orb_recolor: 0 ms
(index):27   [Pipeline] orb_state_prev_copy: 0 ms
(index):27   [Pipeline] zone_gol_sync: 0 ms
(index):27   [Pipeline] zone_gol_evolve: 0 ms
(index):27   [Pipeline] zone_derive_params: 0 ms
(index):27   [Pipeline] zone_seed_mask: 0 ms
(index):27   [Pipeline] arch_mesh_gen: 0 ms
(index):27   [Pipeline] column_mesh_gen: 0 ms
(index):27   [Pipeline] palm_mesh_gen: 0 ms
(index):27   [Pipeline] cactus_mesh_gen: 0 ms
(index):27   [Pipeline] blade_cluster_mesh_gen: 0 ms
(index):27   [Pipeline] patch_terrain: 1 ms
(index):27   [Pipeline] patch_terrain_indirect: 0 ms
(index):27   [Pipeline] pawn: 0 ms
(index):27   [Pipeline] sphere: 0 ms
(index):27   [Pipeline] monolith: 0 ms
(index):27   [Pipeline] arch: 0 ms
(index):27   [Pipeline] column: 0 ms
(index):27   [Pipeline] palm: 0 ms
(index):27   [Pipeline] cactus: 0 ms
(index):27   [Pipeline] blade: 0 ms
(index):27   [Pipeline] shell: 0 ms
(index):27   [Pipeline] ribbon: 0 ms
(index):27   [Pipeline] orb: 0 ms
(index):27   [Pipeline] gallery_frame: 0 ms
(index):27   [Pipeline] wall_painting_canvas: 0 ms
(index):27   [Pipeline] wall_painting_frame: 0 ms
(index):27   [Pipeline] shadow_patch_terrain: 0 ms
(index):27   [Pipeline] shadow_pawn: 0 ms
(index):27   [Pipeline] shadow_sphere: 0 ms
(index):27   [Pipeline] shadow_monolith: 0 ms
(index):27   [Pipeline] shadow_arch: 0 ms
(index):27   [Pipeline] shadow_column: 0 ms
(index):27   [Pipeline] shadow_palm: 0 ms
(index):27   [Pipeline] shadow_cactus: 0 ms
(index):27   [Pipeline] shadow_blade: 0 ms
(index):27   [Pipeline] shadow_shell: 0 ms
(index):27   [Pipeline] shadow_ribbon: 0 ms
(index):27   [Pipeline] shadow_gallery_frame: 0 ms
(index):27   [Pipeline] shadow_wall_painting: 0 ms
(index):27   [Pipeline] fade_overlay: 0 ms
(index):27 
(index):27 [Renderer] Pipelines by compile time (descending):
(index):27          1 ms  patch_terrain
(index):27          0 ms  update_player_agent
(index):27          0 ms  fade_overlay
(index):27          0 ms  update_sphere
(index):27          0 ms  update_cube
(index):27          0 ms  compute_vp
(index):27          0 ms  gen_patch_heights
(index):27          0 ms  gen_patch_gradients
(index):27          0 ms  gen_patch_cells
(index):27          0 ms  compute_ribbon_rings
(index):27          0 ms  compute_photographer_vp
(index):27          0 ms  compute_entity_placement
(index):27          0 ms  frustum_cull_patches
(index):27          0 ms  compute_pawn_aura
(index):27          0 ms  write_live_card_heights
(index):27          0 ms  write_live_card_resolve
(index):27          0 ms  orb_init
(index):27          0 ms  orb_dynamics
(index):27          0 ms  orb_recolor
(index):27          0 ms  orb_state_prev_copy
(index):27          0 ms  zone_gol_sync
(index):27          0 ms  zone_gol_evolve
(index):27          0 ms  zone_derive_params
(index):27          0 ms  zone_seed_mask
(index):27          0 ms  arch_mesh_gen
(index):27          0 ms  column_mesh_gen
(index):27          0 ms  palm_mesh_gen
(index):27          0 ms  cactus_mesh_gen
(index):27          0 ms  blade_cluster_mesh_gen
(index):27          0 ms  update_other_agents
(index):27          0 ms  patch_terrain_indirect
(index):27          0 ms  pawn
(index):27          0 ms  sphere
(index):27          0 ms  monolith
(index):27          0 ms  arch
(index):27          0 ms  column
(index):27          0 ms  palm
(index):27          0 ms  cactus
(index):27          0 ms  blade
(index):27          0 ms  shell
(index):27          0 ms  ribbon
(index):27          0 ms  orb
(index):27          0 ms  gallery_frame
(index):27          0 ms  wall_painting_canvas
(index):27          0 ms  wall_painting_frame
(index):27          0 ms  shadow_patch_terrain
(index):27          0 ms  shadow_pawn
(index):27          0 ms  shadow_sphere
(index):27          0 ms  shadow_monolith
(index):27          0 ms  shadow_arch
(index):27          0 ms  shadow_column
(index):27          0 ms  shadow_palm
(index):27          0 ms  shadow_cactus
(index):27          0 ms  shadow_blade
(index):27          0 ms  shadow_shell
(index):27          0 ms  shadow_ribbon
(index):27          0 ms  shadow_gallery_frame
(index):27          0 ms  shadow_wall_painting
(index):27          0 ms  update_camera
(index):27 
(index):27 [Renderer] Compute pipelines: 5 ms
(index):27 [Renderer] Render pipelines:  8 ms
(index):27 [Renderer] Total pipelines:   14 ms
(index):27 [Orbs] Configured: count=128 palette=jwst_deep drag=0.4 noise=0.3 rule=brownian rot=0.012 orbital=0.15 tiers=jwst_stars
(index):27 [Mood] Applied: open_sunset (mood=0 outdoor)
(index):27 [Agents] Spawned 10 for mood 0 around (0,0)
(index):27 [AGENTS t=0.0 trigger=boot] 11/32 active, possessed=0 tier:{worker=7 scout=4} drv:{player=1 biased_walk=10}
(index):27 [CENSUS t=    0.0 trigger=boot]
(index):27   fam    active  claimed   delta     new
(index):27   pyr         0        0       0       0
(index):27   arch        0        0       0       0
(index):27   col         0        0       0       0
(index):27   ant         0        0       0       0
(index):27   palm        0        0       0       0
(index):27   cact        0        0       0       0
(index):27   blad        0        0       0       0
(index):27   sph         0        —       —       —
(index):27   ribn        0        0       0       0
(index):27   cube        0        —       —       —
(index):27   gol         0        0       0       0
(index):27   gall        0        0       0       0
(index):27   TOTAL       0        0       0       0    footprints 0/128
(index):27   fam      live   hi-wtr     cap  portal
(index):27   pyr         0        0       8       —
(index):27   arch        0        0      16       0
(index):27   col         0        0      16       —
(index):27   ant         0        0      16       —
(index):27   palm        0        0      24       —
(index):27   cact        0        0      20       —
(index):27   blad        0        0      32       —
(index):27   sph         0        0       8       —
(index):27   ribn        0        0       1       —
(index):27   cube        0        0     256       —
(index):27   gol         0        0       8       —
(index):27   gall        0        0      48       —
(index):27 [Authored] Scanned assets/paintings — found 57 paintings
(index):27 [Authored] Loaded: assets/paintings/PAINTING_1.jpg (1505x1201) → staging 0
(index):27 [Authored] Scaled → 512x409 (aspect 1.3)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_2.jpeg (1280x1007) → staging 1
(index):27 [Authored] Scaled → 512x403 (aspect 1.3)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_3.jpeg (1280x843) → staging 2
(index):27 [Authored] Scaled → 512x337 (aspect 1.5)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_4.jpeg (1272x825) → staging 3
(index):27 [Authored] Scaled → 512x332 (aspect 1.5)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_5.jpeg (1283x1020) → staging 4
(index):27 [Authored] Scaled → 512x407 (aspect 1.3)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_6.jpeg (1450x1166) → staging 5
(index):27 [Authored] Scaled → 512x412 (aspect 1.2)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_7.jpeg (1600x985) → staging 6
(index):27 [Authored] Scaled → 512x315 (aspect 1.6)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_8.jpeg (1180x933) → staging 7
(index):27 [Authored] Scaled → 512x405 (aspect 1.3)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_9.jpeg (1080x1011) → staging 8
(index):27 [Authored] Scaled → 512x479 (aspect 1.1)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_10.jpeg (777x971) → staging 9
(index):27 [Authored] Scaled → 410x512 (aspect 0.8)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_11.jpeg (1264x1572) → staging 10
(index):27 [Authored] Scaled → 412x512 (aspect 0.8)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_12.jpeg (1080x1304) → staging 11
(index):27 [Authored] Scaled → 424x512 (aspect 0.8)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_14.jpeg (859x696) → staging 12
(index):27 [Authored] Scaled → 512x415 (aspect 1.2)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_32.jpeg (1280x1040) → staging 13
(index):27 [Authored] Scaled → 512x416 (aspect 1.2)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_50.jpeg (837x1280) → staging 14
(index):27 [Authored] Scaled → 335x512 (aspect 0.7)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_60.jpeg (920x926) → staging 15
(index):27 [Authored] Scaled → 509x512 (aspect 1.0)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_70.jpeg (1280x906) → staging 16
(index):27 [Authored] Scaled → 512x362 (aspect 1.4)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_71.jpeg (1280x1032) → staging 17
(index):27 [Authored] Scaled → 512x413 (aspect 1.2)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_72.jpeg (1268x1280) → staging 18
(index):27 [Authored] Scaled → 507x512 (aspect 1.0)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_73.jpeg (1279x1280) → staging 19
(index):27 [Authored] Scaled → 512x512 (aspect 1.0)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_90.jpeg (1280x506) → staging 20
(index):27 [Authored] Scaled → 512x202 (aspect 2.5)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_92.jpeg (1280x720) → staging 21
(index):27 [Authored] Scaled → 512x288 (aspect 1.8)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_100.jpeg (995x1028) → staging 22
(index):27 [Authored] Scaled → 496x512 (aspect 1.0)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_101.jpeg (1554x1600) → staging 23
(index):27 [Authored] Scaled → 497x512 (aspect 1.0)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_102.jpeg (1225x1280) → staging 24
(index):27 [Authored] Scaled → 490x512 (aspect 1.0)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_103.jpeg (1508x1600) → staging 25
(index):27 [Authored] Scaled → 483x512 (aspect 0.9)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_104.jpeg (1280x1169) → staging 26
(index):27 [Authored] Scaled → 512x468 (aspect 1.1)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_105.jpeg (1280x1219) → staging 27
(index):27 [Authored] Scaled → 512x488 (aspect 1.1)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_106.jpeg (1079x1280) → staging 28
(index):27 [Authored] Scaled → 432x512 (aspect 0.8)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_107.jpeg (1039x1280) → staging 29
(index):27 [Authored] Scaled → 416x512 (aspect 0.8)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_108.jpeg (1115x1132) → staging 30
(index):27 [Authored] Scaled → 504x512 (aspect 1.0)
(index):27 [Authored] Loaded: assets/paintings/PAINTING_109.jpeg (940x1280) → staging 31
(index):27 [Authored] Scaled → 376x512 (aspect 0.7)
(index):27 [Authored] Staged 32/57 images
(index):27 [Cartridge] Renderer init:    65 ms
(index):27 [Cartridge] Patch system:     49544 ms
(index):27 [Cartridge] Total init:       49609 ms
(index):27 
(index):27 [GPU Budget] ---- allocation request, boot ----
(index):27 [GPU Budget] buffers  13.2 MiB
(index):27 [GPU Budget] textures 253.9 MiB
(index):27 [GPU Budget] TOTAL    267.1 MiB
(index):27 [GPU Budget] largest single allocations:
(index):27 [GPU Budget]   1. 112.5 MiB  Patch Heightfield Array (225x256x256, RGBA16Float; 225 = Dim::MAX_ACTIVE_PATCHES)
(index):27 [GPU Budget]   2. 40.0 MiB  Exhibition
(index):27 [GPU Budget]   3. 32.0 MiB  Snapshot Staging
(index):27 [GPU Budget]   4. 32.0 MiB  Authored Staging
(index):27 [GPU Budget]   5. 16.0 MiB  Shadow Map
(index):27 [GPU Budget] estimate: logical texels, uncompressed, no driver padding. Excludes the surface backbuffer and the console depth texture (host-owned).
(index):27 
(index):27 [Ground] zone rects in core: 0 (boot)
(index):27 [Incubator] the_board renderer ready
(index):28 [Zoetrope] ears bound: 0 of 7 (mask 0x7F)
printErr @ (index):28
(index):28 [SignalLayout] 12 sources unbound (no audio source)
printErr @ (index):28
(index):28 [the_board] fog.density base=0 valid=1 | fog.color base=1 count=3 valid=1
printErr @ (index):28
(index):28 [the_board] terrain.checker_mean base=10 count=3 valid=1 | terrain.checker_var base=13 valid=1
printErr @ (index):28
(index):27 Controls: WASD=move, Mouse=camera, 5-8=moods, Esc=quit
(index):27 
(index):27 [Ribbon] SPAWN slot=0 at (-22.7, -27.1) tier=0 len=562.0 near=(-1,-1) far=(1,-12)
(index):27 [Orbs] Init dispatched: 128 orbs, 2 workgroups
(index):1 A valid external Instance reference no longer exists.
(index):28 [Device] LOST reason=1 : A valid external Instance reference no longer exists.
printErr @ (index):28
