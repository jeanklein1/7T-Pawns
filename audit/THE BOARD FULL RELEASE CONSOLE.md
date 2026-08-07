========================================
  INCUBATOR DUAL (Hot Reload Enabled)
  Clock:    BeatClock
  Render:   the_board
========================================

Warning: loader_get_json: Failed to open JSON file C:\Program Files (x86)\Epic Games\Epic Online Services\managedArtifacts\98bc04bc842e4906993fd6d6644ffb8d\EOSOverlayVkLayer-Win64.json
Warning: loader_get_json: Failed to open JSON file C:\Program Files (x86)\Epic Games\Launcher\Portal\Extras\Overlay\EOSOverlayVkLayer-Win32.json
[Console] Dawn revision: f0bf8ab547a9a23b8b78ff67d8085d4a26600a7d
[Console] Build: Release
[Console] Adapter 0: integrated / D3D12 | Intel(R) HD Graphics 5500 (D3D12 driver version 20.19.15.4703) vendor=intel
[Console] Adapter 1: integrated / D3D11 | Intel(R) HD Graphics 5500 (D3D11 driver version 20.19.15.4703) vendor=intel
[Console] Adapter 2: discrete / D3D12 | NVIDIA GeForce 920M (D3D12 driver version 25.21.14.2531) vendor=nvidia
[Console] Adapter 3: discrete / Vulkan | GeForce 920M (NVIDIA: 425.31 425.31.0.0) vendor=nvidia
[Console] Adapter 4: CPU / D3D12 | Microsoft Basic Render Driver (D3D12 driver version 10.0.19041.3636) vendor=microsoft
[Console] Adapter 5: CPU / D3D11 | Microsoft Basic Render Driver (D3D11 driver version 10.0.19041.3636) vendor=microsoft
[Console] Adapter 6: CPU / Null | Null backend () vendor=
[Console] Adapter selected: index=2
[Console] Adapter limits: storageBuffers/stage=10 uniformBuffers/stage=12 bindingsPerGroup=1000
[Console] Adapter features (32): 1 2 3 4 5 9 10 12 13 14 15 16 17 19 21 22 327680 327681 327682 327684 327692 327696 327700 327701 327704 327715 327722 327724 327727 327728 327729 327732
[Console] feature multi-draw-indirect=no timestamp-query=YES
[Incubator] BeatClock ready (bpm 100)
[GPUState] Design Config: 624 B (C++ side; WGSL DesignConfig mirror must match)
[GPUState] Monolith mesh: 24 verts, 36 indices
[GPUState] Arch buffers (GPU mesh gen): 32000 vert, 120000 index capacity
[GPUState] Column buffers (GPU mesh gen): 48000 vert, 192000 index capacity
[GPUState] Shell buffers: 2048 vert, 8192 index capacity
[GPUState] GoL zone buffers: 8 zones ├ù 32├ù32 grid
[Cartridge] GPUState init:    65 ms
[SPINE] validated: 9 update rows + 22 render rows + 12 dispatch rows name-checked; O-#/RC laws static-asserted
Loaded shader from: ../../../src/cartridges/the_board/realization/world.wgsl
[Renderer] Shader compile:    351 ms
  [Pipeline] update_player_agent: 4789 ms
  [Pipeline] update_other_agents: 5904 ms
  [Pipeline] update_camera: 492 ms
  [Pipeline] update_sphere: 907 ms
  [Pipeline] update_cube: 995 ms
  [Pipeline] compute_vp: 329 ms
  [Pipeline] gen_patch_heights: 880 ms
  [Pipeline] gen_patch_gradients: 390 ms
  [Pipeline] gen_patch_cells: 1078 ms
  [Pipeline] compute_ribbon_rings: 358 ms
  [Pipeline] compute_photographer_vp: 389 ms
  [Pipeline] compute_entity_placement: 623 ms
  [Pipeline] frustum_cull_patches: 438 ms
  [Pipeline] compute_pawn_aura: 1289 ms
  [Pipeline] write_live_card_heights: 681 ms
  [Pipeline] write_live_card_resolve: 397 ms
  [Pipeline] orb_init: 411 ms
  [Pipeline] orb_dynamics: 531 ms
  [Pipeline] orb_recolor: 379 ms
  [Pipeline] orb_state_prev_copy: 303 ms
  [Pipeline] zone_gol_sync: 398 ms
  [Pipeline] zone_gol_evolve: 417 ms
  [Pipeline] zone_derive_params: 583 ms
  [Pipeline] zone_seed_mask: 588 ms
  [Pipeline] arch_mesh_gen: 1129 ms
  [Pipeline] column_mesh_gen: 2250 ms
  [Pipeline] palm_mesh_gen: 688 ms
  [Pipeline] cactus_mesh_gen: 772 ms
  [Pipeline] blade_cluster_mesh_gen: 495 ms
  [Pipeline] patch_terrain: 5292 ms
  [Pipeline] patch_terrain_indirect: 5207 ms
  [Pipeline] pawn: 4117 ms
  [Pipeline] sphere: 2939 ms
  [Pipeline] monolith: 4572 ms
  [Pipeline] arch: 1217 ms
  [Pipeline] column: 1119 ms
  [Pipeline] palm: 1075 ms
  [Pipeline] cactus: 1040 ms
  [Pipeline] blade: 1081 ms
  [Pipeline] shell: 1118 ms
  [Pipeline] ribbon: 872 ms
  [Pipeline] orb: 581 ms
  [Pipeline] gallery_frame: 665 ms
  [Pipeline] wall_painting_canvas: 704 ms
  [Pipeline] wall_painting_frame: 688 ms
  [Pipeline] shadow_patch_terrain: 359 ms
  [Pipeline] shadow_pawn: 1091 ms
  [Pipeline] shadow_sphere: 1876 ms
  [Pipeline] shadow_monolith: 2561 ms
  [Pipeline] shadow_arch: 369 ms
  [Pipeline] shadow_column: 279 ms
  [Pipeline] shadow_palm: 288 ms
  [Pipeline] shadow_cactus: 275 ms
  [Pipeline] shadow_blade: 268 ms
  [Pipeline] shadow_shell: 303 ms
  [Pipeline] shadow_ribbon: 341 ms
  [Pipeline] shadow_gallery_frame: 331 ms
  [Pipeline] shadow_wall_painting: 460 ms
  [Pipeline] fade_overlay: 543 ms

[Renderer] Pipelines by compile time (descending):
      5904 ms  update_other_agents
      5292 ms  patch_terrain
      5207 ms  patch_terrain_indirect
      4789 ms  update_player_agent
      4572 ms  monolith
      4117 ms  pawn
      2939 ms  sphere
      2561 ms  shadow_monolith
      2250 ms  column_mesh_gen
      1876 ms  shadow_sphere
      1289 ms  compute_pawn_aura
      1217 ms  arch
      1129 ms  arch_mesh_gen
      1119 ms  column
      1118 ms  shell
      1091 ms  shadow_pawn
      1081 ms  blade
      1078 ms  gen_patch_cells
      1075 ms  palm
      1040 ms  cactus
       995 ms  update_cube
       907 ms  update_sphere
       880 ms  gen_patch_heights
       872 ms  ribbon
       772 ms  cactus_mesh_gen
       704 ms  wall_painting_canvas
       688 ms  wall_painting_frame
       688 ms  palm_mesh_gen
       681 ms  write_live_card_heights
       665 ms  gallery_frame
       623 ms  compute_entity_placement
       588 ms  zone_seed_mask
       583 ms  zone_derive_params
       581 ms  orb
       543 ms  fade_overlay
       531 ms  orb_dynamics
       495 ms  blade_cluster_mesh_gen
       492 ms  update_camera
       460 ms  shadow_wall_painting
       438 ms  frustum_cull_patches
       417 ms  zone_gol_evolve
       411 ms  orb_init
       398 ms  zone_gol_sync
       397 ms  write_live_card_resolve
       390 ms  gen_patch_gradients
       389 ms  compute_photographer_vp
       379 ms  orb_recolor
       369 ms  shadow_arch
       359 ms  shadow_patch_terrain
       358 ms  compute_ribbon_rings
       341 ms  shadow_ribbon
       331 ms  shadow_gallery_frame
       329 ms  compute_vp
       303 ms  orb_state_prev_copy
       303 ms  shadow_shell
       288 ms  shadow_palm
       279 ms  shadow_column
       275 ms  shadow_cactus
       268 ms  shadow_blade

[Renderer] Compute pipelines: 28930 ms
[Renderer] Render pipelines:  41676 ms
[Renderer] Total pipelines:   70606 ms
[Orbs] Configured: count=128 palette=jwst_deep drag=0.4 noise=0.3 rule=brownian rot=0.012 orbital=0.15 tiers=jwst_stars
[Mood] Applied: open_sunset (mood=0 outdoor)
[Agents] Spawned 10 for mood 0 around (0,0)
[AGENTS t=0.0 trigger=boot] 11/32 active, possessed=0 tier:{worker=7 scout=4} drv:{player=1 biased_walk=10}
[CENSUS t=    0.0 trigger=boot]
  fam    active  claimed   delta     new
  pyr         0        0       0       0
  arch        0        0       0       0
  col         0        0       0       0
  ant         0        0       0       0
  palm        0        0       0       0
  cact        0        0       0       0
  blad        0        0       0       0
  sph         0        ΓÇö       ΓÇö       ΓÇö
  ribn        0        0       0       0
  cube        0        ΓÇö       ΓÇö       ΓÇö
  gol         0        0       0       0
  gall        0        0       0       0
  TOTAL       0        0       0       0    footprints 0/128
  fam      live   hi-wtr     cap  portal
  pyr         0        0       8       ΓÇö
  arch        0        0      16       0
  col         0        0      16       ΓÇö
  ant         0        0      16       ΓÇö
  palm        0        0      24       ΓÇö
  cact        0        0      20       ΓÇö
  blad        0        0      32       ΓÇö
  sph         0        0       8       ΓÇö
  ribn        0        0       1       ΓÇö
  cube        0        0     256       ΓÇö
  gol         0        0       8       ΓÇö
  gall        0        0      48       ΓÇö
[Authored] Scanned assets/paintings ΓÇö found 57 paintings
[Authored] Loaded: assets/paintings\PAINTING_1.jpg (1505x1201) ΓåÆ staging 0
[Authored] Scaled ΓåÆ 512x409 (aspect 1.3)
[Authored] Loaded: assets/paintings\PAINTING_2.jpeg (1280x1007) ΓåÆ staging 1
[Authored] Scaled ΓåÆ 512x403 (aspect 1.3)
[Authored] Loaded: assets/paintings\PAINTING_3.jpeg (1280x843) ΓåÆ staging 2
[Authored] Scaled ΓåÆ 512x337 (aspect 1.5)
[Authored] Loaded: assets/paintings\PAINTING_4.jpeg (1272x825) ΓåÆ staging 3
[Authored] Scaled ΓåÆ 512x332 (aspect 1.5)
[Authored] Loaded: assets/paintings\PAINTING_5.jpeg (1283x1020) ΓåÆ staging 4
[Authored] Scaled ΓåÆ 512x407 (aspect 1.3)
[Authored] Loaded: assets/paintings\PAINTING_6.jpeg (1450x1166) ΓåÆ staging 5
[Authored] Scaled ΓåÆ 512x412 (aspect 1.2)
[Authored] Loaded: assets/paintings\PAINTING_7.jpeg (1600x985) ΓåÆ staging 6
[Authored] Scaled ΓåÆ 512x315 (aspect 1.6)
[Authored] Loaded: assets/paintings\PAINTING_8.jpeg (1180x933) ΓåÆ staging 7
[Authored] Scaled ΓåÆ 512x405 (aspect 1.3)
[Authored] Loaded: assets/paintings\PAINTING_9.jpeg (1080x1011) ΓåÆ staging 8
[Authored] Scaled ΓåÆ 512x479 (aspect 1.1)
[Authored] Loaded: assets/paintings\PAINTING_10.jpeg (777x971) ΓåÆ staging 9
[Authored] Scaled ΓåÆ 410x512 (aspect 0.8)
[Authored] Loaded: assets/paintings\PAINTING_11.jpeg (1264x1572) ΓåÆ staging 10
[Authored] Scaled ΓåÆ 412x512 (aspect 0.8)
[Authored] Loaded: assets/paintings\PAINTING_12.jpeg (1080x1304) ΓåÆ staging 11
[Authored] Scaled ΓåÆ 424x512 (aspect 0.8)
[Authored] Loaded: assets/paintings\PAINTING_14.jpeg (859x696) ΓåÆ staging 12
[Authored] Scaled ΓåÆ 512x415 (aspect 1.2)
[Authored] Loaded: assets/paintings\PAINTING_32.jpeg (1280x1040) ΓåÆ staging 13
[Authored] Scaled ΓåÆ 512x416 (aspect 1.2)
[Authored] Loaded: assets/paintings\PAINTING_50.jpeg (837x1280) ΓåÆ staging 14
[Authored] Scaled ΓåÆ 335x512 (aspect 0.7)
[Authored] Loaded: assets/paintings\PAINTING_60.jpeg (920x926) ΓåÆ staging 15
[Authored] Scaled ΓåÆ 509x512 (aspect 1.0)
[Authored] Loaded: assets/paintings\PAINTING_70.jpeg (1280x906) ΓåÆ staging 16
[Authored] Scaled ΓåÆ 512x362 (aspect 1.4)
[Authored] Loaded: assets/paintings\PAINTING_71.jpeg (1280x1032) ΓåÆ staging 17
[Authored] Scaled ΓåÆ 512x413 (aspect 1.2)
[Authored] Loaded: assets/paintings\PAINTING_72.jpeg (1268x1280) ΓåÆ staging 18
[Authored] Scaled ΓåÆ 507x512 (aspect 1.0)
[Authored] Loaded: assets/paintings\PAINTING_73.jpeg (1279x1280) ΓåÆ staging 19
[Authored] Scaled ΓåÆ 512x512 (aspect 1.0)
[Authored] Loaded: assets/paintings\PAINTING_90.jpeg (1280x506) ΓåÆ staging 20
[Authored] Scaled ΓåÆ 512x202 (aspect 2.5)
[Authored] Loaded: assets/paintings\PAINTING_92.jpeg (1280x720) ΓåÆ staging 21
[Authored] Scaled ΓåÆ 512x288 (aspect 1.8)
[Authored] Loaded: assets/paintings\PAINTING_100.jpeg (995x1028) ΓåÆ staging 22
[Authored] Scaled ΓåÆ 496x512 (aspect 1.0)
[Authored] Loaded: assets/paintings\PAINTING_101.jpeg (1554x1600) ΓåÆ staging 23
[Authored] Scaled ΓåÆ 497x512 (aspect 1.0)
[Authored] Loaded: assets/paintings\PAINTING_102.jpeg (1225x1280) ΓåÆ staging 24
[Authored] Scaled ΓåÆ 490x512 (aspect 1.0)
[Authored] Loaded: assets/paintings\PAINTING_103.jpeg (1508x1600) ΓåÆ staging 25
[Authored] Scaled ΓåÆ 483x512 (aspect 0.9)
[Authored] Loaded: assets/paintings\PAINTING_104.jpeg (1280x1169) ΓåÆ staging 26
[Authored] Scaled ΓåÆ 512x468 (aspect 1.1)
[Authored] Loaded: assets/paintings\PAINTING_105.jpeg (1280x1219) ΓåÆ staging 27
[Authored] Scaled ΓåÆ 512x488 (aspect 1.1)
[Authored] Loaded: assets/paintings\PAINTING_106.jpeg (1079x1280) ΓåÆ staging 28
[Authored] Scaled ΓåÆ 432x512 (aspect 0.8)
[Authored] Loaded: assets/paintings\PAINTING_107.jpeg (1039x1280) ΓåÆ staging 29
[Authored] Scaled ΓåÆ 416x512 (aspect 0.8)
[Authored] Loaded: assets/paintings\PAINTING_108.jpeg (1115x1132) ΓåÆ staging 30
[Authored] Scaled ΓåÆ 504x512 (aspect 1.0)
[Authored] Loaded: assets/paintings\PAINTING_109.jpeg (940x1280) ΓåÆ staging 31
[Authored] Scaled ΓåÆ 376x512 (aspect 0.7)
[Authored] Staged 32/57 images
[Cartridge] Renderer init:    71598 ms
[Cartridge] Patch system:     1487 ms
[Cartridge] Total init:       73085 ms

[GPU Budget] ---- allocation request, boot ----
[GPU Budget] buffers  13.2 MiB
[GPU Budget] textures 253.9 MiB
[GPU Budget] TOTAL    267.1 MiB
[GPU Budget] largest single allocations:
[GPU Budget]   1. 112.5 MiB  Patch Heightfield Array (225x256x256, RGBA16Float; 225 = Dim::MAX_ACTIVE_PATCHES)
[GPU Budget]   2. 40.0 MiB  Exhibition
[GPU Budget]   3. 32.0 MiB  Snapshot Staging
[GPU Budget]   4. 32.0 MiB  Authored Staging
[GPU Budget]   5. 16.0 MiB  Shadow Map
[GPU Budget] estimate: logical texels, uncompressed, no driver padding. Excludes the surface backbuffer and the console depth texture (host-owned).

[Ground] zone rects in core: 0 (boot)
[Ground] zones active anywhere: 0 (boot)
[Card] live-card field: AT REST ΓÇö one clearing write, then skipped (boot)
[Incubator] the_board renderer ready
[Zoetrope] ears bound: 0 of 7 (mask 0x7F)
[SignalLayout] 12 sources unbound (no audio source)
[the_board] fog.density base=0 valid=1 | fog.color base=1 count=3 valid=1
[the_board] terrain.checker_mean base=10 count=3 valid=1 | terrain.checker_var base=13 valid=1
[Incubator] Hot reload enabled: ../../../src/cartridges/the_board/realization/world.wgsl

Controls: WASD=move, Mouse=camera, 5-8=moods, Esc=quit