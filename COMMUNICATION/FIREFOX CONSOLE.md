
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
[Cartridge] GPUState init:    64 ms
[SPINE] validated: 9 update rows + 22 render rows + 12 dispatch rows name-checked; O-#/RC laws static-asserted
Loaded shader from: ../../../src/cartridges/the_board/realization/world.wgsl
[Renderer] Shader compile:    326 ms
  [Pipeline] update_player_agent: 2602 ms
  [Pipeline] update_other_agents: 5070 ms
  [Pipeline] update_camera: 861 ms
  [Pipeline] update_sphere: 2053 ms
  [Pipeline] update_cube: 891 ms
  [Pipeline] compute_vp: 327 ms
  [Pipeline] gen_patch_heights: 1153 ms
  [Pipeline] gen_patch_gradients: 371 ms
  [Pipeline] gen_patch_cells: 1038 ms
  [Pipeline] compute_ribbon_rings: 322 ms
  [Pipeline] compute_photographer_vp: 299 ms
  [Pipeline] compute_entity_placement: 451 ms
  [Pipeline] frustum_cull_patches: 363 ms
  [Pipeline] compute_pawn_aura: 1081 ms
  [Pipeline] write_live_card_heights: 666 ms
  [Pipeline] write_live_card_resolve: 594 ms
  [Pipeline] orb_init: 393 ms
  [Pipeline] orb_dynamics: 523 ms
  [Pipeline] orb_recolor: 320 ms
  [Pipeline] orb_state_prev_copy: 246 ms
  [Pipeline] zone_gol_sync: 247 ms
  [Pipeline] zone_gol_evolve: 343 ms
  [Pipeline] zone_derive_params: 556 ms
  [Pipeline] zone_seed_mask: 459 ms
  [Pipeline] arch_mesh_gen: 1024 ms
  [Pipeline] column_mesh_gen: 1943 ms
  [Pipeline] palm_mesh_gen: 619 ms
  [Pipeline] cactus_mesh_gen: 660 ms
  [Pipeline] blade_cluster_mesh_gen: 586 ms
  [Pipeline] patch_terrain: 4863 ms
  [Pipeline] patch_terrain_indirect: 4788 ms
  [Pipeline] pawn: 3712 ms
  [Pipeline] sphere: 2843 ms
  [Pipeline] monolith: 3819 ms
  [Pipeline] arch: 1017 ms
  [Pipeline] column: 905 ms
  [Pipeline] palm: 924 ms
  [Pipeline] cactus: 946 ms
  [Pipeline] blade: 924 ms
  [Pipeline] shell: 910 ms
  [Pipeline] ribbon: 748 ms
  [Pipeline] orb: 442 ms
  [Pipeline] gallery_frame: 548 ms
  [Pipeline] wall_painting_canvas: 588 ms
  [Pipeline] wall_painting_frame: 597 ms
  [Pipeline] shadow_patch_terrain: 295 ms
  [Pipeline] shadow_pawn: 797 ms
  [Pipeline] shadow_sphere: 1870 ms
  [Pipeline] shadow_monolith: 2372 ms
  [Pipeline] shadow_arch: 269 ms
  [Pipeline] shadow_column: 275 ms
  [Pipeline] shadow_palm: 250 ms
  [Pipeline] shadow_cactus: 230 ms
  [Pipeline] shadow_blade: 250 ms
  [Pipeline] shadow_shell: 240 ms
  [Pipeline] shadow_ribbon: 322 ms
  [Pipeline] shadow_gallery_frame: 312 ms
  [Pipeline] shadow_wall_painting: 324 ms
  [Pipeline] fade_overlay: 459 ms

[Renderer] Pipelines by compile time (descending):
      5070 ms  update_other_agents
      4863 ms  patch_terrain
      4788 ms  patch_terrain_indirect
      3819 ms  monolith
      3712 ms  pawn
      2843 ms  sphere
      2602 ms  update_player_agent
      2372 ms  shadow_monolith
      2053 ms  update_sphere
      1943 ms  column_mesh_gen
      1870 ms  shadow_sphere
      1153 ms  gen_patch_heights
      1081 ms  compute_pawn_aura
      1038 ms  gen_patch_cells
      1024 ms  arch_mesh_gen
      1017 ms  arch
       946 ms  cactus
       924 ms  palm
       924 ms  blade
       910 ms  shell
       905 ms  column
       891 ms  update_cube
       861 ms  update_camera
       797 ms  shadow_pawn
       748 ms  ribbon
       666 ms  write_live_card_heights
       660 ms  cactus_mesh_gen
       619 ms  palm_mesh_gen
       597 ms  wall_painting_frame
       594 ms  write_live_card_resolve
       588 ms  wall_painting_canvas
       586 ms  blade_cluster_mesh_gen
       556 ms  zone_derive_params
       548 ms  gallery_frame
       523 ms  orb_dynamics
       459 ms  zone_seed_mask
       459 ms  fade_overlay
       451 ms  compute_entity_placement
       442 ms  orb
       393 ms  orb_init
       371 ms  gen_patch_gradients
       363 ms  frustum_cull_patches
       343 ms  zone_gol_evolve
       327 ms  compute_vp
       324 ms  shadow_wall_painting
       322 ms  compute_ribbon_rings
       322 ms  shadow_ribbon
       320 ms  orb_recolor
       312 ms  shadow_gallery_frame
       299 ms  compute_photographer_vp
       295 ms  shadow_patch_terrain
       275 ms  shadow_column
       269 ms  shadow_arch
       250 ms  shadow_blade
       250 ms  shadow_palm
       247 ms  zone_gol_sync
       246 ms  orb_state_prev_copy
       240 ms  shadow_shell
       230 ms  shadow_cactus

[Renderer] Compute pipelines: 26105 ms
[Renderer] Render pipelines:  36884 ms
[Renderer] Total pipelines:   62990 ms
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
[Cartridge] Renderer init:    63731 ms
[Cartridge] Patch system:     1622 ms
[Cartridge] Total init:       65354 ms

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

[Ribbon] SPAWN slot=0 at (-22.7, -27.1) tier=0 len=562.0 near=(-1,-1) far=(1,-12)
[Orbs] Init dispatched: 128 orbs, 2 workgroups
[GoL] Pulse slot=0 node=(-3,-1) corner=(-350.0,-112.5) host=(-6,-2) HEIGHT period=1.4
[Card] live-card field: LIVE ΓÇö writer runs every frame
[Ground] zones active anywhere: 1
[GoL] Conway slot=1 node=(0,2) corner=(34.4,275.0) host=(1,6) HEIGHT period=13.9
[Ground] zones active anywhere: 2
[Agents] Respawn 1 around (0.0,0.0)
[Agents] Respawn 1 around (0.0,0.0)
[Agents] Respawn 1 around (0.0,0.0)
[Agents] Respawn 1 around (0.0,0.0)
[Agents] Respawn 1 around (0.0,0.0)
[Agents] Respawn 1 around (0.0,0.0)
[Agents] Respawn 1 around (0.0,0.0)
[Agents] Respawn 1 around (24.3,1.0)
[Photographer] Capture -> layer 0 (Portrait) aspect=0.7 pool=1/32
[Photographer] Rendering snapshot -> layer 0
[Photographer] Capture -> layer 1 (Panoramic) aspect=1.9 pool=2/32
[Photographer] Rendering snapshot -> layer 1
[Agents] Respawn 1 around (56.0,-1.8)
[Photographer] Capture -> layer 2 (Portrait) aspect=0.6 pool=3/32
[Photographer] Rendering snapshot -> layer 2
[Photographer] Capture -> layer 3 (Medium) aspect=1.4 pool=4/32
[Photographer] Rendering snapshot -> layer 3
[Photographer] Capture -> layer 4 (Bird's Eye) aspect=1.0 pool=5/32
[Photographer] Rendering snapshot -> layer 4
[Ground] zones active anywhere: 1
[Agents] Respawn 1 around (119.7,-11.9)
[Photographer] Capture -> layer 5 (Panoramic) aspect=2.3 pool=6/32
[Photographer] Rendering snapshot -> layer 5
[Photographer] Capture -> layer 6 (Low Angle) aspect=2.0 pool=7/32
[Photographer] Rendering snapshot -> layer 6
[Photographer] Capture -> layer 7 (Portrait) aspect=0.6 pool=8/32
[Photographer] Rendering snapshot -> layer 7
[GoL] Pulse slot=0 node=(4,-2) corner=(512.5,-206.2) host=(10,-4) period=0.4
[Ground] zones active anywhere: 2
[Agents] Respawn 1 around (193.5,-19.3)
[Agents] Respawn 1 around (201.0,-21.4)
[Photographer] Capture -> layer 8 (Medium) aspect=1.6 pool=9/32
[Photographer] Rendering snapshot -> layer 8
[Agents] Respawn 1 around (211.9,-22.1)
[Agents] Respawn 1 around (219.9,-21.4)
[Portal] GPU trigger: arch 0 -> seed=1898512436 finite=1
[Lighting] Cathedral (3 lights, E/W walls)
[Mood] Indoor palette: terracotta (idx=2)
[WallPainting] Placed 19 painting(s) + 9 snapshot(s) across 4 walls (SNAPSHOT)
[Shell] Generated FLAT: 20 verts, 30 indices bounds=[-100.0,150.0] wall_h=20.0 crown=20.0 rise=0.0
[Mood] Applied: indoor_flat (mood=1 INDOOR)
[Agents] Spawned 4 for mood 1 around (0.0,0.0)
[AGENTS t=115.5 trigger=mood-transition] 5/32 active, possessed=0 tier:{worker=2 sentinel=3} drv:{player=1 slow_patrol=4}
[CENSUS t=  115.5 trigger=mood-transition]
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
[World] Teardown complete, seed=1898512436 mode=finite 5x5
[Portal] Back-portal spawned at (14.8,-71.6) rot=1.6 slot=0 -> return seed=42 mood=open_sunset
[Portal] Forward portal 1 at (121.6,32.8) -> seed=3437462692 mood=indoor_vault FINITE
[Portal] Forward portal 2 at (13.4,121.6) -> seed=1100791468 mood=open_sunset open
[Portal] Finite world: 2 forward portals + 1 back-portal
[Card] live-card field: AT REST ΓÇö one clearing write, then skipped
[Ground] zones active anywhere: 0
[Photographer] Capture -> layer 9 (Panoramic) aspect=2.1 pool=10/32
[Photographer] Rendering snapshot -> layer 9
[Photographer] Capture -> layer 10 (Medium) aspect=1.7 pool=11/32
[Photographer] Rendering snapshot -> layer 10
[Photographer] Capture -> layer 11 (Panoramic) aspect=2.2 pool=12/32
[Photographer] Rendering snapshot -> layer 11
[Photographer] Capture -> layer 12 (Panoramic) aspect=2.3 pool=13/32
[Photographer] Rendering snapshot -> layer 12
[Portal] GPU trigger: arch 2 -> seed=1100791468 finite=0
[Authored] Loaded: assets/paintings\PAINTING_110.jpeg (1569x1148) ΓåÆ staging 0
[Authored] Scaled ΓåÆ 512x375 (aspect 1.4)
[Authored] Loaded: assets/paintings\PAINTING_111.jpeg (1221x1280) ΓåÆ staging 1
[Authored] Scaled ΓåÆ 488x512 (aspect 1.0)
[Authored] Loaded: assets/paintings\PAINTING_112.jpeg (1600x985) ΓåÆ staging 2
[Authored] Scaled ΓåÆ 512x315 (aspect 1.6)
[Authored] Loaded: assets/paintings\PAINTING_113.jpeg (1555x1600) ΓåÆ staging 3
[Authored] Scaled ΓåÆ 498x512 (aspect 1.0)
[Authored] Loaded: assets/paintings\PAINTING_114.jpeg (1028x1060) ΓåÆ staging 4
[Authored] Scaled ΓåÆ 497x512 (aspect 1.0)
[Authored] Loaded: assets/paintings\PAINTING_115.jpeg (1266x1280) ΓåÆ staging 5
[Authored] Scaled ΓåÆ 506x512 (aspect 1.0)
[Authored] Loaded: assets/paintings\PAINTING_200.jpeg (752x1280) ΓåÆ staging 6
[Authored] Scaled ΓåÆ 301x512 (aspect 0.6)
[Authored] Loaded: assets/paintings\PAINTING_201.jpeg (731x1280) ΓåÆ staging 7
[Authored] Scaled ΓåÆ 292x512 (aspect 0.6)
[Authored] Loaded: assets/paintings\PAINTING_202.jpeg (736x1280) ΓåÆ staging 8
[Authored] Scaled ΓåÆ 294x512 (aspect 0.6)
[Authored] Loaded: assets/paintings\PAINTING_203.jpeg (734x1280) ΓåÆ staging 9
[Authored] Scaled ΓåÆ 294x512 (aspect 0.6)
[Authored] Loaded: assets/paintings\PAINTING_205.jpeg (1280x734) ΓåÆ staging 10
[Authored] Scaled ΓåÆ 512x294 (aspect 1.7)
[Authored] Loaded: assets/paintings\PAINTING_206.jpeg (1280x701) ΓåÆ staging 11
[Authored] Scaled ΓåÆ 512x280 (aspect 1.8)
[Authored] Loaded: assets/paintings\PAINTING_207.jpeg (1055x1600) ΓåÆ staging 12
[Authored] Scaled ΓåÆ 338x512 (aspect 0.7)
[Authored] Loaded: assets/paintings\PAINTING_208.jpeg (769x1280) ΓåÆ staging 13
[Authored] Scaled ΓåÆ 308x512 (aspect 0.6)
[Authored] Loaded: assets/paintings\PAINTING_209.jpeg (1008x654) ΓåÆ staging 14
[Authored] Scaled ΓåÆ 512x332 (aspect 1.5)
[Authored] Loaded: assets/paintings\PAINTING_210.jpeg (795x1280) ΓåÆ staging 15
[Authored] Scaled ΓåÆ 318x512 (aspect 0.6)
[Authored] Loaded: assets/paintings\PAINTING_211.jpeg (1035x1600) ΓåÆ staging 16
[Authored] Scaled ΓåÆ 331x512 (aspect 0.6)
[Authored] Loaded: assets/paintings\PAINTING_212.jpeg (801x1280) ΓåÆ staging 17
[Authored] Scaled ΓåÆ 320x512 (aspect 0.6)
[Authored] Loaded: assets/paintings\PAINTING_213.jpeg (912x676) ΓåÆ staging 18
[Authored] Scaled ΓåÆ 512x380 (aspect 1.3)
[Authored] Rotated 19 slot(s), 32 valid, disk cursor at 51/57
[Orbs] Configured: count=128 palette=jwst_deep drag=0.4 noise=0.3 rule=brownian rot=0.0 orbital=0.2 tiers=jwst_stars
[Mood] Applied: open_sunset (mood=0 outdoor)
[Agents] Spawned 10 for mood 0 around (0.0,0.0)
[AGENTS t=140.8 trigger=mood-transition] 11/32 active, possessed=0 tier:{worker=2 scout=9} drv:{player=1 biased_walk=10}
[CENSUS t=  140.8 trigger=mood-transition]
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
[World] Teardown complete, seed=1100791468 mode=open
[Ribbon] SPAWN slot=0 at (-22.5, -25.1) tier=1 len=545.5 near=(-1,-1) far=(-11,-6)
[Gallery] slot=3 at (-30.6,175.1) host=(-1,3) arch=2 paintings=5/5 type=auth
[Portal] Door fallback at (20.9,56.2) -> seed=560530079 mood=indoor_flat FINITE
[Orbs] Init dispatched: 128 orbs, 2 workgroups
[GoL] Pulse slot=0 node=(-3,-1) corner=(-325.0,-87.5) host=(-6,-2) period=0.5
[Card] live-card field: LIVE ΓÇö writer runs every frame
[Ground] zones active anywhere: 1
[Gallery] slot=0 at (-127.4,-285.2) host=(-3,-6) arch=0 paintings=3/3 type=snap
[Gallery] slot=1 at (-123.9,285.9) host=(-3,5) arch=2 paintings=6/6 type=mix
[GoL] Conway slot=1 node=(-3,1) corner=(-325.0,153.1) host=(-6,3) HEIGHT period=12.9
[Ground] zones active anywhere: 2
[Gallery] slot=2 at (165.7,336.1) host=(3,6) arch=2 paintings=1/1 type=snap
[Photographer] Capture -> layer 13 (Panoramic) aspect=2.1 pool=14/32
[Photographer] Rendering snapshot -> layer 13
[Photographer] Capture -> layer 14 (Medium) aspect=1.8 pool=15/32
[Photographer] Rendering snapshot -> layer 14
[Photographer] Capture -> layer 15 (Close-up) aspect=1.3 pool=16/32
[Photographer] Rendering snapshot -> layer 15
[Photographer] Capture -> layer 16 (Portrait) aspect=0.7 pool=17/32
[Photographer] Rendering snapshot -> layer 16
[Incubator] Shutdown
[Device] LOST reason=2 : Device was destroyed.

C:\dev\7t\out\build\the-board-full-release\incubator_dual.exe (process 12452) exited with code 0 (0x0).
To automatically close the console when debugging stops, enable Tools->Options->Debugging->Automatically close the console when debugging stops.
Press any key to close this window . . .