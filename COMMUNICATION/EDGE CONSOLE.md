(index):377 
(index):377 ========================================
(index):377   INCUBATOR DUAL (web twin — no hot reload)
(index):377   Clock:    BeatClock
(index):377   Render:   the_board
(index):377 ========================================
(index):377 
the_board.js:1 The powerPreference option is currently ignored when calling requestAdapter() on Windows. See https://crbug.com/369219127
_emwgpuInstanceRequestAdapter @ the_board.js:1
:8000/favicon.ico:1  Failed to load resource: the server responded with a status of 404 (File not found)
(index):377 [Device] adapter: nvidia | kepler | ? | ?
(index):377 [Device] requesting CORE DEFAULTS; exceptions carried: none (C6 cleared maxStorageBuffersPerShaderStage 9->8)
(index):377 [Device] granted vs floor: maxTextureDimension2D=8192/2048 maxStorageBuffersPerShaderStage=8/8 maxUniformBufferBindingSize=65536/65536
(index):377 [Device] modest device accepted — NO DISCARD
(index):377 [Device] KEEPING the device from: core defaults + censused exceptions (this is the one the frame loop runs on)
(index):377 [Incubator] BeatClock ready (bpm 100)
(index):377 [GPUState] Design Config: 624 B (C++ side; WGSL DesignConfig mirror must match)
(index):377 [GPUState] Monolith mesh: 24 verts, 36 indices
(index):377 [GPUState] Arch buffers (GPU mesh gen): 32000 vert, 120000 index capacity
(index):377 [GPUState] Column buffers (GPU mesh gen): 48000 vert, 192000 index capacity
(index):377 [GPUState] Shell buffers: 2048 vert, 8192 index capacity
(index):377 [GPUState] GoL zone buffers: 8 zones × 32×32 grid
(index):377 [Cartridge] GPUState init:    17 ms
(index):377 [SPINE] validated: 9 update rows + 22 render rows + 12 dispatch rows name-checked; O-#/RC laws static-asserted
(index):377 Loaded shader from: ../../../src/cartridges/the_board/realization/world.wgsl
(index):377 [Renderer] Shader compile:    15 ms
(index):377   [Pipeline] update_player_agent: 0 ms
(index):377   [Pipeline] update_other_agents: 0 ms
(index):377   [Pipeline] update_camera: 0 ms
(index):377   [Pipeline] update_sphere: 0 ms
(index):377   [Pipeline] update_cube: 0 ms
(index):377   [Pipeline] compute_vp: 0 ms
(index):377   [Pipeline] gen_patch_heights: 0 ms
(index):377   [Pipeline] gen_patch_gradients: 0 ms
(index):377   [Pipeline] gen_patch_cells: 0 ms
(index):377   [Pipeline] compute_ribbon_rings: 0 ms
(index):377   [Pipeline] compute_photographer_vp: 0 ms
(index):377   [Pipeline] compute_entity_placement: 0 ms
(index):377   [Pipeline] frustum_cull_patches: 0 ms
(index):377   [Pipeline] compute_pawn_aura: 0 ms
(index):377   [Pipeline] write_live_card_heights: 0 ms
(index):377   [Pipeline] write_live_card_resolve: 0 ms
(index):377   [Pipeline] orb_init: 0 ms
(index):377   [Pipeline] orb_dynamics: 0 ms
(index):377   [Pipeline] orb_recolor: 0 ms
(index):377   [Pipeline] orb_state_prev_copy: 0 ms
(index):377   [Pipeline] zone_gol_sync: 0 ms
(index):377   [Pipeline] zone_gol_evolve: 0 ms
(index):377   [Pipeline] zone_derive_params: 0 ms
(index):377   [Pipeline] zone_seed_mask: 0 ms
(index):377   [Pipeline] arch_mesh_gen: 0 ms
(index):377   [Pipeline] column_mesh_gen: 0 ms
(index):377   [Pipeline] palm_mesh_gen: 0 ms
(index):377   [Pipeline] cactus_mesh_gen: 0 ms
(index):377   [Pipeline] blade_cluster_mesh_gen: 0 ms
(index):377   [Pipeline] patch_terrain: 0 ms
(index):377   [Pipeline] patch_terrain_indirect: 0 ms
(index):377   [Pipeline] pawn: 0 ms
(index):377   [Pipeline] sphere: 0 ms
(index):377   [Pipeline] monolith: 0 ms
(index):377   [Pipeline] arch: 0 ms
(index):377   [Pipeline] column: 0 ms
(index):377   [Pipeline] palm: 0 ms
(index):377   [Pipeline] cactus: 0 ms
(index):377   [Pipeline] blade: 0 ms
(index):377   [Pipeline] shell: 0 ms
(index):377   [Pipeline] ribbon: 0 ms
(index):377   [Pipeline] orb: 0 ms
(index):377   [Pipeline] gallery_frame: 0 ms
(index):377   [Pipeline] wall_painting_canvas: 0 ms
(index):377   [Pipeline] wall_painting_frame: 0 ms
(index):377   [Pipeline] shadow_patch_terrain: 0 ms
(index):377   [Pipeline] shadow_pawn: 0 ms
(index):377   [Pipeline] shadow_sphere: 0 ms
(index):377   [Pipeline] shadow_monolith: 0 ms
(index):377   [Pipeline] shadow_arch: 0 ms
(index):377   [Pipeline] shadow_column: 0 ms
(index):377   [Pipeline] shadow_palm: 0 ms
(index):377   [Pipeline] shadow_cactus: 0 ms
(index):377   [Pipeline] shadow_blade: 0 ms
(index):377   [Pipeline] shadow_shell: 0 ms
(index):377   [Pipeline] shadow_ribbon: 0 ms
(index):377   [Pipeline] shadow_gallery_frame: 0 ms
(index):377   [Pipeline] shadow_wall_painting: 0 ms
(index):377   [Pipeline] fade_overlay: 0 ms
(index):377 
(index):377 [Renderer] Pipelines by compile time (descending):
(index):377          0 ms  update_player_agent
(index):377          0 ms  update_other_agents
(index):377          0 ms  update_camera
(index):377          0 ms  update_sphere
(index):377          0 ms  update_cube
(index):377          0 ms  compute_vp
(index):377          0 ms  gen_patch_heights
(index):377          0 ms  gen_patch_gradients
(index):377          0 ms  gen_patch_cells
(index):377          0 ms  compute_ribbon_rings
(index):377          0 ms  compute_photographer_vp
(index):377          0 ms  compute_entity_placement
(index):377          0 ms  frustum_cull_patches
(index):377          0 ms  compute_pawn_aura
(index):377          0 ms  write_live_card_heights
(index):377          0 ms  write_live_card_resolve
(index):377          0 ms  orb_init
(index):377          0 ms  orb_dynamics
(index):377          0 ms  orb_recolor
(index):377          0 ms  orb_state_prev_copy
(index):377          0 ms  zone_gol_sync
(index):377          0 ms  zone_gol_evolve
(index):377          0 ms  zone_derive_params
(index):377          0 ms  zone_seed_mask
(index):377          0 ms  arch_mesh_gen
(index):377          0 ms  column_mesh_gen
(index):377          0 ms  palm_mesh_gen
(index):377          0 ms  cactus_mesh_gen
(index):377          0 ms  blade_cluster_mesh_gen
(index):377          0 ms  patch_terrain
(index):377          0 ms  patch_terrain_indirect
(index):377          0 ms  pawn
(index):377          0 ms  sphere
(index):377          0 ms  monolith
(index):377          0 ms  arch
(index):377          0 ms  column
(index):377          0 ms  palm
(index):377          0 ms  cactus
(index):377          0 ms  blade
(index):377          0 ms  shell
(index):377          0 ms  ribbon
(index):377          0 ms  orb
(index):377          0 ms  gallery_frame
(index):377          0 ms  wall_painting_canvas
(index):377          0 ms  wall_painting_frame
(index):377          0 ms  shadow_patch_terrain
(index):377          0 ms  shadow_pawn
(index):377          0 ms  shadow_sphere
(index):377          0 ms  shadow_monolith
(index):377          0 ms  shadow_arch
(index):377          0 ms  shadow_column
(index):377          0 ms  shadow_palm
(index):377          0 ms  shadow_cactus
(index):377          0 ms  shadow_blade
(index):377          0 ms  shadow_shell
(index):377          0 ms  shadow_ribbon
(index):377          0 ms  shadow_gallery_frame
(index):377          0 ms  shadow_wall_painting
(index):377          0 ms  fade_overlay
(index):377 
(index):377 [Renderer] Compute pipelines: 5 ms
(index):377 [Renderer] Render pipelines:  9 ms
(index):377 [Renderer] Total pipelines:   14 ms
(index):377 [Orbs] Configured: count=128 palette=jwst_deep drag=0.4 noise=0.3 rule=brownian rot=0.012 orbital=0.15 tiers=jwst_stars
(index):377 [Mood] Applied: open_sunset (mood=0 outdoor)
(index):377 [Agents] Spawned 10 for mood 0 around (0,0)
(index):377 [AGENTS t=0.0 trigger=boot] 11/32 active, possessed=0 tier:{worker=7 scout=4} drv:{player=1 biased_walk=10}
(index):377 [CENSUS t=    0.0 trigger=boot]
(index):377   fam    active  claimed   delta     new
(index):377   pyr         0        0       0       0
(index):377   arch        0        0       0       0
(index):377   col         0        0       0       0
(index):377   ant         0        0       0       0
(index):377   palm        0        0       0       0
(index):377   cact        0        0       0       0
(index):377   blad        0        0       0       0
(index):377   sph         0        —       —       —
(index):377   ribn        0        0       0       0
(index):377   cube        0        —       —       —
(index):377   gol         0        0       0       0
(index):377   gall        0        0       0       0
(index):377   TOTAL       0        0       0       0    footprints 0/128
(index):377   fam      live   hi-wtr     cap  portal
(index):377   pyr         0        0       8       —
(index):377   arch        0        0      16       0
(index):377   col         0        0      16       —
(index):377   ant         0        0      16       —
(index):377   palm        0        0      24       —
(index):377   cact        0        0      20       —
(index):377   blad        0        0      32       —
(index):377   sph         0        0       8       —
(index):377   ribn        0        0       1       —
(index):377   cube        0        0     256       —
(index):377   gol         0        0       8       —
(index):377   gall        0        0      48       —
(index):377 [Authored] Scanned assets/paintings — found 57 paintings
(index):377 [Authored] Loaded: assets/paintings/PAINTING_1.jpg (1505x1201) → staging 0
(index):377 [Authored] Scaled → 512x409 (aspect 1.3)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_2.jpeg (1280x1007) → staging 1
(index):377 [Authored] Scaled → 512x403 (aspect 1.3)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_3.jpeg (1280x843) → staging 2
(index):377 [Authored] Scaled → 512x337 (aspect 1.5)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_4.jpeg (1272x825) → staging 3
(index):377 [Authored] Scaled → 512x332 (aspect 1.5)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_5.jpeg (1283x1020) → staging 4
(index):377 [Authored] Scaled → 512x407 (aspect 1.3)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_6.jpeg (1450x1166) → staging 5
(index):377 [Authored] Scaled → 512x412 (aspect 1.2)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_7.jpeg (1600x985) → staging 6
(index):377 [Authored] Scaled → 512x315 (aspect 1.6)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_8.jpeg (1180x933) → staging 7
(index):377 [Authored] Scaled → 512x405 (aspect 1.3)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_9.jpeg (1080x1011) → staging 8
(index):377 [Authored] Scaled → 512x479 (aspect 1.1)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_10.jpeg (777x971) → staging 9
(index):377 [Authored] Scaled → 410x512 (aspect 0.8)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_11.jpeg (1264x1572) → staging 10
(index):377 [Authored] Scaled → 412x512 (aspect 0.8)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_12.jpeg (1080x1304) → staging 11
(index):377 [Authored] Scaled → 424x512 (aspect 0.8)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_14.jpeg (859x696) → staging 12
(index):377 [Authored] Scaled → 512x415 (aspect 1.2)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_32.jpeg (1280x1040) → staging 13
(index):377 [Authored] Scaled → 512x416 (aspect 1.2)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_50.jpeg (837x1280) → staging 14
(index):377 [Authored] Scaled → 335x512 (aspect 0.7)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_60.jpeg (920x926) → staging 15
(index):377 [Authored] Scaled → 509x512 (aspect 1.0)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_70.jpeg (1280x906) → staging 16
(index):377 [Authored] Scaled → 512x362 (aspect 1.4)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_71.jpeg (1280x1032) → staging 17
(index):377 [Authored] Scaled → 512x413 (aspect 1.2)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_72.jpeg (1268x1280) → staging 18
(index):377 [Authored] Scaled → 507x512 (aspect 1.0)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_73.jpeg (1279x1280) → staging 19
(index):377 [Authored] Scaled → 512x512 (aspect 1.0)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_90.jpeg (1280x506) → staging 20
(index):377 [Authored] Scaled → 512x202 (aspect 2.5)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_92.jpeg (1280x720) → staging 21
(index):377 [Authored] Scaled → 512x288 (aspect 1.8)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_100.jpeg (995x1028) → staging 22
(index):377 [Authored] Scaled → 496x512 (aspect 1.0)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_101.jpeg (1554x1600) → staging 23
(index):377 [Authored] Scaled → 497x512 (aspect 1.0)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_102.jpeg (1225x1280) → staging 24
(index):377 [Authored] Scaled → 490x512 (aspect 1.0)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_103.jpeg (1508x1600) → staging 25
(index):377 [Authored] Scaled → 483x512 (aspect 0.9)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_104.jpeg (1280x1169) → staging 26
(index):377 [Authored] Scaled → 512x468 (aspect 1.1)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_105.jpeg (1280x1219) → staging 27
(index):377 [Authored] Scaled → 512x488 (aspect 1.1)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_106.jpeg (1079x1280) → staging 28
(index):377 [Authored] Scaled → 432x512 (aspect 0.8)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_107.jpeg (1039x1280) → staging 29
(index):377 [Authored] Scaled → 416x512 (aspect 0.8)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_108.jpeg (1115x1132) → staging 30
(index):377 [Authored] Scaled → 504x512 (aspect 1.0)
(index):377 [Authored] Loaded: assets/paintings/PAINTING_109.jpeg (940x1280) → staging 31
(index):377 [Authored] Scaled → 376x512 (aspect 0.7)
(index):377 [Authored] Staged 32/57 images
(index):377 [Cartridge] Renderer init:    73 ms
(index):377 [Cartridge] Patch system:     36334 ms
(index):377 [Cartridge] Total init:       36408 ms
(index):377 
(index):377 [GPU Budget] ---- allocation request, boot ----
(index):377 [GPU Budget] buffers  13.2 MiB
(index):377 [GPU Budget] textures 253.9 MiB
(index):377 [GPU Budget] TOTAL    267.1 MiB
(index):377 [GPU Budget] largest single allocations:
(index):377 [GPU Budget]   1. 112.5 MiB  Patch Heightfield Array (225x256x256, RGBA16Float; 225 = Dim::MAX_ACTIVE_PATCHES)
(index):377 [GPU Budget]   2. 40.0 MiB  Exhibition
(index):377 [GPU Budget]   3. 32.0 MiB  Snapshot Staging
(index):377 [GPU Budget]   4. 32.0 MiB  Authored Staging
(index):377 [GPU Budget]   5. 16.0 MiB  Shadow Map
(index):377 [GPU Budget] estimate: logical texels, uncompressed, no driver padding. Excludes the surface backbuffer and the console depth texture (host-owned).
(index):377 
(index):377 [Ground] zone rects in core: 0 (boot)
(index):377 [Ground] zones active anywhere: 0 (boot)
(index):377 [Card] live-card field: AT REST — one clearing write, then skipped (boot)
(index):377 [Incubator] the_board renderer ready
(index):378 [Zoetrope] ears bound: 0 of 7 (mask 0x7F)
printErr @ (index):378
(index):378 [SignalLayout] 12 sources unbound (no audio source)
printErr @ (index):378
(index):378 [the_board] fog.density base=0 valid=1 | fog.color base=1 count=3 valid=1
printErr @ (index):378
(index):378 [the_board] terrain.checker_mean base=10 count=3 valid=1 | terrain.checker_var base=13 valid=1
printErr @ (index):378
(index):377 Controls: WASD=move, Mouse=camera, 5-8=moods, Esc=quit
(index):377 
(index):377 [Ribbon] SPAWN slot=0 at (-22.7, -27.1) tier=0 len=562.0 near=(-1,-1) far=(1,-12)
(index):377 [Orbs] Init dispatched: 128 orbs, 2 workgroups
(index):377 [GoL] Pulse slot=0 node=(-3,-1) corner=(-350.0,-112.5) host=(-6,-2) HEIGHT period=1.4
(index):377 [Card] live-card field: LIVE — writer runs every frame
(index):377 [Ground] zones active anywhere: 1
(index):377 [GoL] Conway slot=1 node=(0,2) corner=(34.4,275.0) host=(1,6) HEIGHT period=13.9
(index):377 [Ground] zones active anywhere: 2
(index):377 [Agents] Respawn 1 around (0.0,0.0)
[NEW] Explain Console errors by using Copilot in Edge: click  to explain an error. Learn moreDon't show again
(index):377 [Agents] Respawn 1 around (0.0,0.0)
(index):377 [Agents] Respawn 1 around (0.0,0.0)
(index):377 [Agents] Respawn 1 around (0.0,0.0)
