
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
[Cartridge] GPUState init:    73 ms
[SPINE] validated: 9 update rows + 22 render rows + 12 dispatch rows name-checked; O-#/RC laws static-asserted
Loaded shader from: ../../../src/cartridges/the_board/realization/world.wgsl
[Renderer] Shader compile:    330 ms
  [Pipeline] update_player_agent: 2955 ms
  [Pipeline] update_other_agents: 4654 ms
  [Pipeline] update_camera: 431 ms
  [Pipeline] update_sphere: 766 ms
  [Pipeline] update_cube: 955 ms
  [Pipeline] compute_vp: 333 ms
  [Pipeline] gen_patch_heights: 910 ms
  [Pipeline] gen_patch_gradients: 513 ms
  [Pipeline] gen_patch_cells: 1067 ms
  [Pipeline] compute_ribbon_rings: 370 ms
  [Pipeline] compute_photographer_vp: 319 ms
  [Pipeline] compute_entity_placement: 479 ms
  [Pipeline] frustum_cull_patches: 372 ms
  [Pipeline] compute_pawn_aura: 1283 ms
  [Pipeline] write_live_card_heights: 654 ms
  [Pipeline] write_live_card_resolve: 501 ms
  [Pipeline] orb_init: 382 ms
  [Pipeline] orb_dynamics: 544 ms
  [Pipeline] orb_recolor: 327 ms
  [Pipeline] orb_state_prev_copy: 265 ms
  [Pipeline] zone_gol_sync: 277 ms
  [Pipeline] zone_gol_evolve: 370 ms
  [Pipeline] zone_derive_params: 539 ms
  [Pipeline] zone_seed_mask: 505 ms
  [Pipeline] arch_mesh_gen: 1125 ms
  [Pipeline] column_mesh_gen: 1954 ms
  [Pipeline] palm_mesh_gen: 626 ms
  [Pipeline] cactus_mesh_gen: 759 ms
  [Pipeline] blade_cluster_mesh_gen: 470 ms
  [Pipeline] patch_terrain: 5086 ms
  [Pipeline] patch_terrain_indirect: 5091 ms
  [Pipeline] pawn: 4211 ms
  [Pipeline] sphere: 3009 ms
  [Pipeline] monolith: 4204 ms
  [Pipeline] arch: 1220 ms
  [Pipeline] column: 976 ms
  [Pipeline] palm: 985 ms
  [Pipeline] cactus: 1008 ms
  [Pipeline] blade: 1104 ms
  [Pipeline] shell: 1262 ms
  [Pipeline] ribbon: 1117 ms
  [Pipeline] orb: 519 ms
  [Pipeline] gallery_frame: 593 ms
  [Pipeline] wall_painting_canvas: 631 ms
  [Pipeline] wall_painting_frame: 757 ms
  [Pipeline] shadow_patch_terrain: 405 ms
  [Pipeline] shadow_pawn: 1040 ms
  [Pipeline] shadow_sphere: 1777 ms
  [Pipeline] shadow_monolith: 2623 ms
  [Pipeline] shadow_arch: 367 ms
  [Pipeline] shadow_column: 276 ms
  [Pipeline] shadow_palm: 264 ms
  [Pipeline] shadow_cactus: 287 ms
  [Pipeline] shadow_blade: 258 ms
  [Pipeline] shadow_shell: 257 ms
  [Pipeline] shadow_ribbon: 334 ms
  [Pipeline] shadow_gallery_frame: 309 ms
  [Pipeline] shadow_wall_painting: 380 ms
  [Pipeline] fade_overlay: 546 ms

[Renderer] Pipelines by compile time (descending):
      5091 ms  patch_terrain_indirect
      5086 ms  patch_terrain
      4654 ms  update_other_agents
      4211 ms  pawn
      4204 ms  monolith
      3009 ms  sphere
      2955 ms  update_player_agent
      2623 ms  shadow_monolith
      1954 ms  column_mesh_gen
      1777 ms  shadow_sphere
      1283 ms  compute_pawn_aura
      1262 ms  shell
      1220 ms  arch
      1125 ms  arch_mesh_gen
      1117 ms  ribbon
      1104 ms  blade
      1067 ms  gen_patch_cells
      1040 ms  shadow_pawn
      1008 ms  cactus
       985 ms  palm
       976 ms  column
       955 ms  update_cube
       910 ms  gen_patch_heights
       766 ms  update_sphere
       759 ms  cactus_mesh_gen
       757 ms  wall_painting_frame
       654 ms  write_live_card_heights
       631 ms  wall_painting_canvas
       626 ms  palm_mesh_gen
       593 ms  gallery_frame
       546 ms  fade_overlay
       544 ms  orb_dynamics
       539 ms  zone_derive_params
       519 ms  orb
       513 ms  gen_patch_gradients
       505 ms  zone_seed_mask
       501 ms  write_live_card_resolve
       479 ms  compute_entity_placement
       470 ms  blade_cluster_mesh_gen
       431 ms  update_camera
       405 ms  shadow_patch_terrain
       382 ms  orb_init
       380 ms  shadow_wall_painting
       372 ms  frustum_cull_patches
       370 ms  compute_ribbon_rings
       370 ms  zone_gol_evolve
       367 ms  shadow_arch
       334 ms  shadow_ribbon
       333 ms  compute_vp
       327 ms  orb_recolor
       319 ms  compute_photographer_vp
       309 ms  shadow_gallery_frame
       287 ms  shadow_cactus
       277 ms  zone_gol_sync
       276 ms  shadow_column
       265 ms  orb_state_prev_copy
       264 ms  shadow_palm
       258 ms  shadow_blade
       257 ms  shadow_shell

[Renderer] Compute pipelines: 24749 ms
[Renderer] Render pipelines:  40939 ms
[Renderer] Total pipelines:   65688 ms
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
[Cartridge] Renderer init:    66671 ms
[Cartridge] Patch system:     1297 ms
[Cartridge] Total init:       67968 ms

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
[CENSUS t=    0.1 trigger=periodic]
  fam    active  claimed   delta     new
  pyr         0        0       0       0
  arch        1        1       0       1
  col         4        4       0       4
  ant         5        5       0       5
  palm        5        5       0       5
  cact        1        1       0       1
  blad        1        1       0       1
  sph         0        ΓÇö       ΓÇö       ΓÇö
  ribn        1        1       0       1
  cube       10        ΓÇö       ΓÇö       ΓÇö
  gol         0        0       0       0
  gall        0        0       0       0
  TOTAL      28       18       0      18    footprints 18/128
  fam      live   hi-wtr     cap  portal
  pyr         0        0       8       ΓÇö
  arch        1        2      16       1
  col         4        4      16       ΓÇö
  ant         5        5      16       ΓÇö
  palm        5        5      24       ΓÇö
  cact        1        1      20       ΓÇö
  blad        1        1      32       ΓÇö
  sph         0        0       8       ΓÇö
  ribn        1        1       1       ΓÇö
  cube       10       10     256       ΓÇö
  gol         0        0       8       ΓÇö
  gall        0        0      48       ΓÇö
  claimed ground ΓÇö arrivals (18):
  ribn t0 (   -22.7,   -27.1) p( -1, -1) age=0.0
  ant t5 (   -25.1,   -72.9) p( -1, -2) age=0.0
  palm t1 (   -71.7,    21.4) p( -2,  0) age=0.0
  blad t0 (    79.8,    22.4) p(  1,  0) age=0.0
  palm t1 (   -67.8,   -71.5) p( -2, -2) age=0.0
  palm t2 (    66.5,   -74.4) p(  1, -2) age=0.0
  col t2 (   122.4,   -33.7) p(  2, -1) age=0.0
  col t2 (  -128.4,    70.5) p( -3,  1) age=0.0
  col t2 (   123.2,    73.8) p(  2,  1) age=0.0
  ant t5 (  -128.3,  -119.7) p( -3, -3) age=0.0
  palm t1 (   132.2,   128.5) p(  2,  2) age=0.0
  col t2 (   171.6,   -29.5) p(  3, -1) age=0.0
    ... +6 more
[METER] window 1f  fps 5.6  gpu sampled 0f | budget 16.6 ms
[METER] U fill_signal             mean 0.02  max 0.02
[METER] U advance_clock           mean 0.00  max 0.00
[METER] U motion_drivers          mean 0.07  max 0.07
[METER] U motion_bodies           mean 0.01  max 0.01
[METER] U stage_world             mean 0.01  max 0.01
[METER] U transition_machine      mean 0.01  max 0.01
[METER] U stage_fade_and_upload   mean 0.04  max 0.04
[METER] U witness_photographer    mean 0.00  max 0.00
[METER] U clear_input_deltas      mean 0.00  max 0.00
[METER] R witness_harvest         mean 0.00  max 0.00
[METER] R portal_trigger          mean 0.00  max 0.00
[METER] R stream_patches          mean 16.40  max 16.40
[METER] R respawn_agents          mean 0.00  max 0.00
[METER] R census_dumps            mean 0.00  max 0.00
[METER] R ribbon_tick             mean 0.00  max 0.00
[METER] R entity_mesh_gen         mean 0.00  max 0.00
[METER] R upload_portal_lights    mean 0.00  max 0.00
[METER] R live_card_write         mean 0.00  max 0.00
[METER] R dispatch_compute        mean 0.00  max 0.00
[METER] R witness_capture         mean 0.00  max 0.00
[METER] R gol_derive_flush        mean 0.00  max 0.00
[METER] R gol_zone_compute        mean 0.00  max 0.00
[METER] R pawn_aura               mean 0.00  max 0.00
[METER] R orb_sky                 mean 0.00  max 0.00
[METER] R ground_entries          mean 0.00  max 0.00
[METER] R placement_correction    mean 0.00  max 0.00
[METER] R frustum_cull            mean 0.00  max 0.00
[METER] R shadow_pass             mean 0.00  max 0.00
[METER] R main_pass               mean 0.00  max 0.00
[METER] R snapshot_pass           mean 0.00  max 0.00
[METER] R promotion_drain         mean 0.00  max 0.00
[METER] S begin_frame             mean 0.00  max 0.00
[METER] S acquire                 mean 0.00  max 0.00
[METER] S finish_submit           mean 0.00  max 0.00
[METER] S present                 mean 0.00  max 0.00
[METER] S frame_total             mean 0.00  max 0.00
[METER] U_SUM 0.17   R_SUM 16.40
[METER] residue -16.57  (frame_total 0.00 - U_SUM - R_SUM - S_partials 0.00)
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
[CENSUS t=   30.1 trigger=periodic]
  fam    active  claimed   delta     new
  pyr         2        2       0       2
  arch        8        8       0       7
  col        11       11       0       7
  ant         7        7       0       2
  palm       21       21       0      16
  cact        8        8       0       7
  blad        8        8       0       7
  sph         3        ΓÇö       ΓÇö       ΓÇö
  ribn        1        1       0       0
  cube       39        ΓÇö       ΓÇö       ΓÇö
  gol         2        2       0       2
  gall        0        0       0       0
  TOTAL     110       68       0      50    footprints 68/128
  fam      live   hi-wtr     cap  portal
  pyr         2        2       8       ΓÇö
  arch        8        8      16       7
  col        11       12      16       ΓÇö
  ant         7        7      16       ΓÇö
  palm       21       21      24       ΓÇö
  cact        8        8      20       ΓÇö
  blad        8        8      32       ΓÇö
  sph         3        3       8       ΓÇö
  ribn        1        1       1       ΓÇö
  cube       39       39     256       ΓÇö
  gol         2        2       8       ΓÇö
  gall        0        0      48       ΓÇö
  claimed ground ΓÇö arrivals (50):
  palm t1 (  -184.0,   -66.6) p( -4, -2) age=29.9
  arch t0 (   227.3,   -21.3) p(  4, -1) age=29.9
  palm t0 (    14.0,   228.8) p(  0,  4) age=29.9
  col t2 (    71.8,  -221.8) p(  1, -5) age=29.9
  palm t0 (  -221.2,   -66.1) p( -5, -2) age=29.9
  palm t0 (   216.0,   -81.2) p(  4, -2) age=29.9
  cact t0 (  -177.5,  -172.8) p( -4, -4) age=29.8
  ant t4 (   217.6,  -128.0) p(  4, -3) age=29.8
  cact t0 (   232.6,  -128.4) p(  4, -3) age=29.8
  col t1 (   230.2,   126.7) p(  4,  2) age=29.8
  palm t2 (   123.1,   235.7) p(  2,  4) age=29.8
  pyr t2 (    26.4,  -276.5) p(  0, -6) age=29.8
    ... +38 more
[METER] window 1466f  fps 47.2  gpu sampled 489f | budget 16.6 ms
[METER] U fill_signal             mean 0.00  max 0.12
[METER] U advance_clock           mean 0.00  max 0.01
[METER] U motion_drivers          mean 0.11  max 2.15
[METER] U motion_bodies           mean 0.00  max 0.09
[METER] U stage_world             mean 0.00  max 0.01
[METER] U transition_machine      mean 0.00  max 0.02
[METER] U stage_fade_and_upload   mean 0.02  max 1.05
[METER] U witness_photographer    mean 0.00  max 0.21
[METER] U clear_input_deltas      mean 0.00  max 0.00
[METER] R witness_harvest         mean 0.01  max 0.44
[METER] R portal_trigger          mean 0.00  max 0.05
[METER] R stream_patches          cpu 0.40/11.67  gpu 0.28/48.37
[METER] R respawn_agents          mean 0.01  max 4.06
[METER] R census_dumps            mean 0.14  max 198.63
[METER] R ribbon_tick             mean 0.09  max 0.89
[METER] R entity_mesh_gen         cpu 0.00/0.12  gpu 0.07/3.01
[METER] R upload_portal_lights    mean 0.00  max 0.10
[METER] R live_card_write         cpu 0.08/2.36  gpu 1.07/1.11
[METER] R dispatch_compute        cpu 0.18/4.13  gpu 1.32/1.38
[METER] R witness_capture         mean 0.01  max 4.33
[METER] R gol_derive_flush        mean 0.00  max 1.11
[METER] R gol_zone_compute        cpu 0.08/1.02  gpu 0.02/0.07
[METER] R pawn_aura               mean 0.00  max 0.05
[METER] R orb_sky                 cpu 0.07/5.33  gpu 0.04/0.07
[METER] R ground_entries          mean 0.00  max 0.27
[METER] R placement_correction    cpu 0.00/0.21  gpu 0.01/0.33
[METER] R frustum_cull            cpu 0.07/2.14  gpu 0.02/0.07
[METER] R shadow_pass             cpu 0.20/5.01  gpu 4.58/4.72
[METER] R main_pass               cpu 0.44/164.98  gpu 12.49/13.43
[METER] R snapshot_pass           mean 0.00  max 0.02
[METER] R promotion_drain         mean 0.00  max 0.01
[METER] S begin_frame             mean 0.45  max 20.97
[METER] S acquire                 mean 13.30  max 25.92
[METER] S finish_submit           mean 2.87  max 48.39
[METER] S present                 mean 2.15  max 310.55
[METER] S frame_total             mean 20.92  max 783.63
[METER] U_SUM 0.14   R_SUM 1.78
[METER] residue 0.24  (frame_total 20.92 - U_SUM - R_SUM - S_partials 18.77)
[Agents] Respawn 1 around (-15.1,12.4)
[Ground] zone rects in core: 1
[Photographer] Capture -> layer 0 (Portrait) aspect=0.7 pool=1/32
[Photographer] Rendering snapshot -> layer 0
[Photographer] Capture -> layer 1 (Panoramic) aspect=1.9 pool=2/32
[Photographer] Rendering snapshot -> layer 1
[GoL] Pulse slot=2 node=(-2,3) corner=(-206.2,393.8) host=(-4,8) period=0.8
[Ground] zones active anywhere: 3
[Ground] zone rects in core: 2
[Photographer] Capture -> layer 2 (Portrait) aspect=0.6 pool=3/32
[Photographer] Rendering snapshot -> layer 2
[Photographer] Capture -> layer 3 (Medium) aspect=1.4 pool=4/32
[Photographer] Rendering snapshot -> layer 3
[Photographer] Capture -> layer 4 (Bird's Eye) aspect=1.0 pool=5/32
[Photographer] Rendering snapshot -> layer 4
[Agents] Respawn 2 around (-74.0,94.7)
[Photographer] Capture -> layer 5 (Panoramic) aspect=2.3 pool=6/32
[Photographer] Rendering snapshot -> layer 5
[Photographer] Capture -> layer 6 (Low Angle) aspect=2.0 pool=7/32
[Photographer] Rendering snapshot -> layer 6
[Photographer] Capture -> layer 7 (Portrait) aspect=0.6 pool=8/32
[Photographer] Rendering snapshot -> layer 7
[Photographer] Capture -> layer 8 (Medium) aspect=1.6 pool=9/32
[Photographer] Rendering snapshot -> layer 8
[GoL] Conway slot=3 node=(-3,4) corner=(-325.0,512.5) host=(-6,10) HEIGHT period=4.0
[Ground] zones active anywhere: 4
[Gallery] slot=0 at (-432.8,529.3) host=(-9,10) arch=2 paintings=6/6 type=snap
[Photographer] Capture -> layer 9 (Panoramic) aspect=2.1 pool=10/32
[Photographer] Rendering snapshot -> layer 9
[Agents] Respawn 1 around (-141.1,158.9)
[GoL] Conway slot=4 node=(-5,-2) corner=(-590.6,-231.2) host=(-11,-4) HEIGHT period=0.9
[Ground] zones active anywhere: 5
[Ground] zone rects in core: 3
[Photographer] Capture -> layer 10 (Medium) aspect=1.7 pool=11/32
[Photographer] Rendering snapshot -> layer 10
[Ground] zone rects in core: 2
[Ground] zones active anywhere: 4
[Gallery] slot=1 at (-582.3,422.8) host=(-12,8) arch=0 paintings=5/5 type=snap
[Ground] zone rects in core: 1
[Photographer] Capture -> layer 11 (Panoramic) aspect=2.2 pool=12/32
[Photographer] Rendering snapshot -> layer 11
[Photographer] Capture -> layer 12 (Panoramic) aspect=2.3 pool=13/32
[Photographer] Rendering snapshot -> layer 12
[Photographer] Capture -> layer 13 (Panoramic) aspect=2.1 pool=14/32
[Photographer] Rendering snapshot -> layer 13
[Agents] Respawn 1 around (-275.7,283.6)
[Ground] zone rects in core: 2
[Photographer] Capture -> layer 14 (Medium) aspect=1.8 pool=15/32
[Photographer] Rendering snapshot -> layer 14
[Ground] zones active anywhere: 3
[Agents] Respawn 1 around (-294.1,301.9)
[CENSUS t=   60.1 trigger=periodic]
  fam    active  claimed   delta     new
  pyr         0        0       0       0
  arch       11       11       0       8
  col        10       10       0       7
  ant         1        1       0       0
  palm       17       17       0       9
  cact        5        5       0       4
  blad        7        7       0       4
  sph         2        ΓÇö       ΓÇö       ΓÇö
  ribn        1        1       0       0
  cube       68        ΓÇö       ΓÇö       ΓÇö
  gol         3        3       0       2
  gall        2        2       0       2
  TOTAL     127       57       0      36    footprints 57/128
  fam      live   hi-wtr     cap  portal
  pyr         0        0       8       ΓÇö
  arch       11       12      16       9
  col        10       10      16       ΓÇö
  ant         1        4      16       ΓÇö
  palm       17       24      24       ΓÇö
  cact        5        8      20       ΓÇö
  blad        7       11      32       ΓÇö
  sph         2        3       8       ΓÇö
  ribn        1        1       1       ΓÇö
  cube       68       68     256       ΓÇö
  gol         3        4       8       ΓÇö
  gall        2        2      48       ΓÇö
  claimed ground ΓÇö arrivals (36):
  arch t0 (  -378.6,   -19.1) p( -8, -1) age=29.0
  blad t0 (  -379.5,    68.0) p( -8,  1) age=29.0
  blad t0 (  -381.1,   127.3) p( -8,  2) age=29.0
  arch t2 (  -376.5,   176.7) p( -8,  3) age=29.0
  arch t0 (  -367.9,   327.5) p( -8,  6) age=29.0
  palm t1 (  -125.9,   424.1) p( -3,  8) age=24.8
  gol t8 (  -181.2,   418.8) p( -4,  8) age=24.7
  palm t2 (  -430.0,    79.2) p( -9,  1) age=23.0
  blad t0 (  -427.7,    79.5) p( -9,  1) age=23.0
  palm t2 (  -428.0,   127.2) p( -9,  2) age=23.0
  palm t1 (   -64.4,   466.3) p( -2,  9) age=20.3
  cact t1 (  -228.8,   472.9) p( -5,  9) age=20.3
    ... +24 more
[METER] window 1595f  fps 52.2  gpu sampled 532f | budget 16.6 ms
[METER] U fill_signal             mean 0.00  max 0.09
[METER] U advance_clock           mean 0.00  max 0.01
[METER] U motion_drivers          mean 0.10  max 1.95
[METER] U motion_bodies           mean 0.00  max 0.03
[METER] U stage_world             mean 0.00  max 0.14
[METER] U transition_machine      mean 0.00  max 0.03
[METER] U stage_fade_and_upload   mean 0.02  max 0.97
[METER] U witness_photographer    mean 0.05  max 13.32
[METER] U clear_input_deltas      mean 0.00  max 0.01
[METER] R witness_harvest         mean 0.01  max 0.13
[METER] R portal_trigger          mean 0.00  max 0.03
[METER] R stream_patches          cpu 0.36/10.25  gpu 0.21/4.46
[METER] R respawn_agents          mean 0.02  max 22.52
[METER] R census_dumps            mean 0.44  max 700.26
[METER] R ribbon_tick             mean 0.08  max 1.32
[METER] R entity_mesh_gen         cpu 0.01/0.80  gpu 0.16/3.01
[METER] R upload_portal_lights    mean 0.00  max 0.03
[METER] R live_card_write         cpu 0.07/3.38  gpu 1.24/6.36
[METER] R dispatch_compute        cpu 0.16/5.06  gpu 1.47/7.67
[METER] R witness_capture         mean 0.00  max 0.10
[METER] R gol_derive_flush        mean 0.00  max 0.89
[METER] R gol_zone_compute        cpu 0.07/0.49  gpu 0.02/0.13
[METER] R pawn_aura               mean 0.00  max 0.30
[METER] R orb_sky                 cpu 0.06/0.80  gpu 0.04/0.20
[METER] R ground_entries          mean 0.00  max 0.15
[METER] R placement_correction    cpu 0.00/0.08  gpu 0.02/0.33
[METER] R frustum_cull            cpu 0.07/2.36  gpu 0.02/0.07
[METER] R shadow_pass             cpu 0.17/4.13  gpu 4.60/21.76
[METER] R main_pass               cpu 0.27/11.73  gpu 10.29/36.18
[METER] R snapshot_pass           mean 0.03  max 11.38
[METER] R promotion_drain         mean 0.00  max 0.03
[METER] S begin_frame             mean 0.38  max 9.33
[METER] S acquire                 mean 12.61  max 100.89
[METER] S finish_submit           mean 2.39  max 25.81
[METER] S present                 mean 1.67  max 12.17
[METER] S frame_total             mean 19.19  max 714.52
[METER] U_SUM 0.17   R_SUM 1.80
[METER] residue 0.16  (frame_total 19.19 - U_SUM - R_SUM - S_partials 17.05)
[GoL] Pulse slot=0 node=(-6,2) corner=(-675.0,287.5) host=(-14,6) HEIGHT period=3.7
[Gallery] slot=2 at (-679.0,223.9) host=(-14,4) arch=0 paintings=3/3 type=snap
[Ground] zones active anywhere: 4
[GoL] Conway slot=4 node=(-6,4) corner=(-687.5,512.5) host=(-14,10) HEIGHT period=9.9
[Ground] zones active anywhere: 5
[Ground] zones active anywhere: 4
[Agents] Respawn 1 around (-315.7,309.0)
[Photographer] Capture -> layer 15 (Close-up) aspect=1.3 pool=16/32
[Photographer] Rendering snapshot -> layer 15
[Photographer] Capture -> layer 16 (Portrait) aspect=0.7 pool=17/32
[Photographer] Rendering snapshot -> layer 16
[Agents] Respawn 1 around (-348.3,316.8)
[Gallery] slot=3 at (-725.3,87.0) host=(-15,1) arch=0 paintings=3/3 type=snap
[Portal] GPU trigger: arch 10 -> seed=1758729433 finite=1
[Lighting] Added vault uplight (slot 3)
[Lighting] Cathedral (4 lights, E/W walls)
[Mood] Indoor palette: pale linen (idx=4)
[WallPainting] Placed 21 painting(s) + 0 snapshot(s) across 3 walls (AUTHORED)
[Shell] Generated GROIN VAULT: 1105 verts, 6168 indices bounds=[-150.0,200.0] wall_h=25.0 crown=77.5 rise=52.5
[Mood] Applied: indoor_vault (mood=2 INDOOR)
[Agents] Spawned 4 for mood 2 around (0.0,0.0)
[AGENTS t=66.1 trigger=mood-transition] 5/32 active, possessed=0 tier:{worker=1 sentinel=2 leader=2} drv:{player=1 slow_patrol=4}
[CENSUS t=   66.1 trigger=mood-transition]
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
[World] Teardown complete, seed=1758729433 mode=finite 7x7
[Portal] Back-portal spawned at (171.6,31.7) rot=3.1 slot=0 -> return seed=42 mood=open_sunset
[Portal] Forward portal 1 at (-121.6,56.2) -> seed=2211325671 mood=indoor_flat FINITE
[Portal] Forward portal 2 at (50.2,171.6) -> seed=243141638 mood=open_sunset open
[Portal] Finite world: 2 forward portals + 1 back-portal
[Card] live-card field: AT REST ΓÇö one clearing write, then skipped
[Ground] zone rects in core: 0
[Ground] zones active anywhere: 0
[Photographer] Capture -> layer 17 (Low Angle) aspect=1.9 pool=18/32
[Photographer] Rendering snapshot -> layer 17
[Photographer] Capture -> layer 18 (Panoramic) aspect=2.2 pool=19/32
[Photographer] Rendering snapshot -> layer 18
[Possess] 0 -> 2 (tier 3, dist 8.2)
[Photographer] Capture -> layer 19 (Close-up) aspect=1.4 pool=20/32
[Photographer] Rendering snapshot -> layer 19
[Photographer] Capture -> layer 20 (Portrait) aspect=0.7 pool=21/32
[Photographer] Rendering snapshot -> layer 20
[Photographer] Capture -> layer 21 (Panoramic) aspect=2.1 pool=22/32
[Photographer] Rendering snapshot -> layer 21
[Possess] 2 -> 4 (tier 2, dist 10.1)
[Photographer] Capture -> layer 22 (Panoramic) aspect=2.2 pool=23/32
[Photographer] Rendering snapshot -> layer 22
[Possess] No agent within 20.0 units of the point at (37.0,32.7)
[Possess] No agent within 20.0 units of the point at (33.7,38.4)
[Photographer] Capture -> layer 23 (Close-up) aspect=1.5 pool=24/32
[Photographer] Rendering snapshot -> layer 23
[Photographer] Capture -> layer 24 (Panoramic) aspect=2.3 pool=25/32
[Photographer] Rendering snapshot -> layer 24
[Photographer] Capture -> layer 25 (Panoramic) aspect=2.0 pool=26/32
[Photographer] Rendering snapshot -> layer 25
[Photographer] Capture -> layer 26 (Cinematic) aspect=2.1 pool=27/32
[Photographer] Rendering snapshot -> layer 26
[Photographer] Capture -> layer 27 (Low Angle) aspect=1.5 pool=28/32
[Photographer] Rendering snapshot -> layer 27
[Photographer] Capture -> layer 28 (Cinematic) aspect=2.4 pool=29/32
[Photographer] Rendering snapshot -> layer 28
[Photographer] Capture -> layer 29 (Panoramic) aspect=1.8 pool=30/32
[Photographer] Rendering snapshot -> layer 29
[CENSUS t=   90.1 trigger=periodic]
  fam    active  claimed   delta     new
  pyr         4        4       0       4
  arch        4        4       0       4
  col         0        0       0       0
  ant         0        0       0       0
  palm        0        0       0       0
  cact        0        0       0       0
  blad        0        0       0       0
  sph         0        ΓÇö       ΓÇö       ΓÇö
  ribn        0        0       0       0
  cube       12        ΓÇö       ΓÇö       ΓÇö
  gol         0        0       0       0
  gall        0        0       0       0
  TOTAL      20        8       0       8    footprints 8/128
  fam      live   hi-wtr     cap  portal
  pyr         4        6       8       ΓÇö
  arch        4        5      16       3
  col         0        0      16       ΓÇö
  ant         0        0      16       ΓÇö
  palm        0        0      24       ΓÇö
  cact        0        0      20       ΓÇö
  blad        0        0      32       ΓÇö
  sph         0        0       8       ΓÇö
  ribn        0        0       1       ΓÇö
  cube       12       12     256       ΓÇö
  gol         0        0       8       ΓÇö
  gall        0        0      48       ΓÇö
  claimed ground ΓÇö arrivals (8):
  arch t0 (   171.6,    31.7) p(  3,  0) age=24.1
  arch t0 (  -121.6,    56.2) p( -3,  1) age=24.1
  arch t0 (    50.2,   171.6) p(  1,  3) age=24.1
  pyr t0 (   -25.1,    18.8) p( -1,  0) age=24.1
  pyr t0 (    70.6,   -29.3) p(  1, -1) age=24.1
  pyr t2 (  -118.5,  -118.5) p( -3, -3) age=24.1
  arch t0 (   169.6,   -69.7) p(  3, -2) age=24.1
  pyr t2 (   168.5,   168.5) p(  3,  3) age=24.1
[METER] window 1274f  fps 40.9  gpu sampled 425f | budget 16.6 ms
[METER] U fill_signal             mean 0.00  max 0.14
[METER] U advance_clock           mean 0.00  max 0.03
[METER] U motion_drivers          mean 0.10  max 0.91
[METER] U motion_bodies           mean 0.00  max 0.19
[METER] U stage_world             mean 0.00  max 0.12
[METER] U transition_machine      mean 0.33  max 418.32
[METER] U stage_fade_and_upload   mean 0.02  max 0.39
[METER] U witness_photographer    mean 0.09  max 19.74
[METER] U clear_input_deltas      mean 0.00  max 0.01
[METER] R witness_harvest         mean 0.01  max 1.60
[METER] R portal_trigger          mean 0.00  max 5.67
[METER] R stream_patches          cpu 0.31/42.80  gpu 0.04/4.19
[METER] R respawn_agents          mean 0.01  max 6.28
[METER] R census_dumps            mean 0.60  max 762.50
[METER] R ribbon_tick             mean 0.03  max 0.68
[METER] R entity_mesh_gen         cpu 0.00/0.08  gpu 0.03/3.41
[METER] R upload_portal_lights    mean 0.00  max 0.08
[METER] R live_card_write         cpu 0.02/2.98  gpu 0.38/7.47
[METER] R dispatch_compute        cpu 0.19/4.63  gpu 1.07/9.83
[METER] R witness_capture         mean 0.00  max 0.19
[METER] R gol_derive_flush        cpu 0.00/1.41  gpu 0.00/0.07
[METER] R gol_zone_compute        cpu 0.02/0.85  gpu 0.01/0.07
[METER] R pawn_aura               mean 0.00  max 0.08
[METER] R orb_sky                 cpu 0.02/0.70  gpu 0.01/0.20
[METER] R ground_entries          mean 0.00  max 0.16
[METER] R placement_correction    cpu 0.00/0.10  gpu 0.00/0.33
[METER] R frustum_cull            cpu 0.10/13.52  gpu 0.02/0.07
[METER] R shadow_pass             cpu 0.36/8.33  gpu 2.60/21.76
[METER] R main_pass               cpu 0.31/8.88  gpu 18.42/47.58
[METER] R snapshot_pass           mean 0.06  max 11.68
[METER] R promotion_drain         mean 0.00  max 0.02
[METER] S begin_frame             mean 0.59  max 18.79
[METER] S acquire                 mean 16.76  max 280.63
[METER] S finish_submit           mean 2.60  max 12.55
[METER] S present                 mean 1.79  max 27.77
[METER] S frame_total             mean 24.52  max 778.05
[METER] U_SUM 0.54   R_SUM 2.06
[METER] residue 0.19  (frame_total 24.52 - U_SUM - R_SUM - S_partials 21.73)
[World] Transition (open_sunset): seed 1758729433 -> 4064138229
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
[Authored] Loaded: assets/paintings\PAINTING_214.jpeg (1079x799) ΓåÆ staging 19
[Authored] Scaled ΓåÆ 512x379 (aspect 1.4)
[Authored] Loaded: assets/paintings\PAINTING_500.jpeg (1280x1097) ΓåÆ staging 20
[Authored] Scaled ΓåÆ 512x439 (aspect 1.2)
[Authored] Rotated 21 slot(s), 32 valid, disk cursor at 53/57
[Orbs] Configured: count=128 palette=jwst_deep drag=0.4 noise=0.3 rule=brownian rot=0.0 orbital=0.2 tiers=jwst_stars
[Mood] Applied: open_sunset (mood=0 outdoor)
[Agents] Spawned 10 for mood 0 around (0.0,0.0)
[AGENTS t=91.1 trigger=mood-transition] 11/32 active, possessed=0 tier:{worker=5 scout=5 sentinel=1} drv:{player=1 biased_walk=10}
[CENSUS t=   91.1 trigger=mood-transition]
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
[World] Teardown complete, seed=4064138229 mode=open
[GoL] Conway slot=0 node=(-1,-1) corner=(-87.5,-87.5) host=(-2,-2) HEIGHT period=0.3
[Gallery] slot=2 at (-23.2,173.8) host=(-1,3) arch=2 paintings=4/4 type=snap
[GoL] Conway slot=4 node=(1,1) corner=(153.1,153.1) host=(3,3) HEIGHT period=0.3
[Portal] Door fallback at (35.4,-48.5) -> seed=3216886897 mood=indoor_vault FINITE
[Ribbon] SPAWN slot=0 at (-23.0, -175.0) tier=0 len=676.4 near=(-1,-4) far=(-5,-17)
[Card] live-card field: LIVE ΓÇö writer runs every frame
[Orbs] Init dispatched: 128 orbs, 2 workgroups
[Ground] zone rects in core: 2
[Ground] zones active anywhere: 2
[Gallery] slot=0 at (222.2,-69.6) host=(4,-2) arch=2 paintings=4/4 type=snap
[GoL] Conway slot=1 node=(-2,-2) corner=(-206.2,-206.2) host=(-4,-4) HEIGHT period=8.1
[GoL] Conway slot=2 node=(1,-2) corner=(140.6,-218.8) host=(3,-4) HEIGHT period=5.0
[Ground] zone rects in core: 4
[Ground] zones active anywhere: 4
[GoL] Pulse slot=3 node=(-1,-3) corner=(-87.5,-325.0) host=(-2,-6) period=0.6
[Ground] zones active anywhere: 5
[GoL] Conway slot=6 node=(-3,-1) corner=(-325.0,-87.5) host=(-6,-2) HEIGHT period=7.8
[Ground] zones active anywhere: 6
[Gallery] slot=3 at (-275.4,176.9) host=(-6,3) arch=1 paintings=5/5 type=snap
[Gallery] slot=1 at (231.4,-284.8) host=(4,-6) arch=2 paintings=3/3 type=auth
[GoL] Pulse slot=5 node=(-2,2) corner=(-193.8,287.5) host=(-4,6) HEIGHT period=3.7
[Ground] zones active anywhere: 7
[GoL] Conway slot=7 node=(-3,-3) corner=(-337.5,-337.5) host=(-6,-6) HEIGHT period=4.1
[Ground] zones active anywhere: 8
[the_board] Camera mode: First-Person View
[Photographer] Capture -> layer 30 (Bird's Eye) aspect=1.2 pool=31/32
[Photographer] Rendering snapshot -> layer 30
[the_board] Camera mode: Orbit
[Agents] Respawn 1 around (-28.5,22.7)
[Ground] zone rects in core: 3
[Photographer] Capture -> layer 31 (Panoramic) aspect=2.1 pool=32/32
[Photographer] Rendering snapshot -> layer 31
[Photographer] Capture -> layer 0 (Panoramic) aspect=2.3 pool=32/32
[Photographer] Rendering snapshot -> layer 0
[Photographer] Capture -> layer 1 (Panoramic) aspect=1.8 pool=32/32
[Photographer] Rendering snapshot -> layer 1
[Gallery] slot=4 at (328.0,429.1) host=(6,8) arch=1 paintings=4/4 type=snap
[Ground] zone rects in core: 2
[Agents] Respawn 1 around (-30.0,54.8)
[Photographer] Capture -> layer 2 (Close-up) aspect=1.4 pool=32/32
[Photographer] Rendering snapshot -> layer 2
[Agents] Respawn 1 around (-24.2,62.2)
[Agents] Respawn 1 around (-25.7,63.6)
[Ground] zone rects in core: 3
[World] Transition (open_sunset): seed 4064138229 -> 1963877187
[Authored] Loaded: assets/paintings\PAINTING_501.jpeg (1280x1135) ΓåÆ staging 21
[Authored] Scaled ΓåÆ 512x454 (aspect 1.1)
[Authored] Loaded: assets/paintings\PAINTING_900.jpeg (1440x805) ΓåÆ staging 22
[Authored] Scaled ΓåÆ 512x286 (aspect 1.8)
[Authored] Loaded: assets/paintings\PAINTING_1001.jpeg (1008x1518) ΓåÆ staging 23
[Authored] Scaled ΓåÆ 340x512 (aspect 0.7)
[Authored] Rotated 3 slot(s), 32 valid, disk cursor at 56/57
[Orbs] Configured: count=128 palette=jwst_deep drag=0.4 noise=0.3 rule=brownian rot=0.0 orbital=0.2 tiers=jwst_stars
[Mood] Applied: open_sunset (mood=0 outdoor)
[Agents] Spawned 10 for mood 0 around (0.0,0.0)
[AGENTS t=109.5 trigger=mood-transition] 11/32 active, possessed=0 tier:{worker=4 scout=6 sentinel=1} drv:{player=1 biased_walk=10}
[CENSUS t=  109.5 trigger=mood-transition]
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
[World] Teardown complete, seed=1963877187 mode=open
[Ribbon] SPAWN slot=0 at (-27.9, -19.8) tier=1 len=373.6 near=(-1,-1) far=(0,-8)
[Gallery] slot=0 at (-122.3,80.4) host=(-3,1) arch=2 paintings=1/1 type=snap
[GoL] Conway slot=0 node=(1,-1) corner=(140.6,-100.0) host=(3,-2) HEIGHT period=4.9
[GoL] Conway slot=1 node=(0,1) corner=(21.9,140.6) host=(1,3) HEIGHT period=3.5
[Portal] Door fallback at (-25.9,54.1) -> seed=1313519089 mood=indoor_vault FINITE
[Orbs] Init dispatched: 128 orbs, 2 workgroups
[Ground] zone rects in core: 2
[Ground] zones active anywhere: 2
[GoL] Pulse slot=2 node=(-2,-1) corner=(-206.2,-87.5) host=(-4,-2) period=0.7
[Ground] zone rects in core: 3
[Ground] zones active anywhere: 3
[GoL] Conway slot=3 node=(1,-3) corner=(153.1,-325.0) host=(3,-6) HEIGHT period=5.2
[GoL] Pulse slot=4 node=(-3,-2) corner=(-350.0,-231.2) host=(-6,-4) HEIGHT period=2.6
[Ground] zones active anywhere: 5
[Gallery] slot=2 at (312.4,382.1) host=(6,7) arch=1 paintings=5/5 type=auth
[World] Transition (indoor_flat 7x7): seed 1963877187 -> 1831581725
[Authored] Loaded: assets/paintings\PAINTING_1002.jpeg (718x1600) ΓåÆ staging 24
[Authored] Scaled ΓåÆ 230x512 (aspect 0.4)
[Authored] Loaded: assets/paintings\PAINTING_1.jpg (1505x1201) ΓåÆ staging 25
[Authored] Scaled ΓåÆ 512x409 (aspect 1.3)
[Authored] Loaded: assets/paintings\PAINTING_2.jpeg (1280x1007) ΓåÆ staging 26
[Authored] Scaled ΓåÆ 512x403 (aspect 1.3)
[Authored] Loaded: assets/paintings\PAINTING_3.jpeg (1280x843) ΓåÆ staging 27
[Authored] Scaled ΓåÆ 512x337 (aspect 1.5)
[Authored] Loaded: assets/paintings\PAINTING_4.jpeg (1272x825) ΓåÆ staging 28
[Authored] Scaled ΓåÆ 512x332 (aspect 1.5)
[Authored] Rotated 5 slot(s), 32 valid, disk cursor at 4/57
[Lighting] Cathedral (3 lights, N/S walls)
[Mood] Indoor palette: terracotta (idx=2)
[WallPainting] Placed 28 painting(s) + 0 snapshot(s) across 4 walls (AUTHORED)
[Shell] Generated FLAT: 20 verts, 30 indices bounds=[-150.0,200.0] wall_h=20.0 crown=20.0 rise=0.0
[Mood] Applied: indoor_flat (mood=1 INDOOR)
[Agents] Spawned 4 for mood 1 around (0.0,0.0)
[AGENTS t=115.3 trigger=mood-transition] 5/32 active, possessed=0 tier:{worker=1 sentinel=3 leader=1} drv:{player=1 slow_patrol=4}
[CENSUS t=  115.3 trigger=mood-transition]
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
[World] Teardown complete, seed=1831581725 mode=finite 7x7
[Portal] Back-portal spawned at (24.0,171.6) rot=-1.6 slot=0 -> return seed=1963877187 mood=open_sunset
[Portal] Forward portal 1 at (-121.6,29.4) -> seed=2971222485 mood=indoor_vault FINITE
[Portal] Forward portal 2 at (59.0,-121.6) -> seed=3852821901 mood=open_sunset open
[Portal] Finite world: 2 forward portals + 1 back-portal
[GoL] Conway slot=3 node=(1,1) corner=(128.1,128.1) host=(3,3) HEIGHT period=2.5
[Ground] zone rects in core: 1
[Ground] zones active anywhere: 1
[Photographer] Capture -> layer 3 (Low Angle) aspect=1.9 pool=32/32
[Photographer] Rendering snapshot -> layer 3
[CENSUS t=  120.1 trigger=periodic]
  fam    active  claimed   delta     new
  pyr         0        0       0       0
  arch        4        4       0       4
  col         3        3       0       3
  ant         0        0       0       0
  palm        2        2       0       2
  cact        1        1       0       1
  blad        0        0       0       0
  sph         0        ΓÇö       ΓÇö       ΓÇö
  ribn        0        0       0       0
  cube        7        ΓÇö       ΓÇö       ΓÇö
  gol         1        1       0       1
  gall        0        0       0       0
  TOTAL      18       11       0      11    footprints 11/128
  fam      live   hi-wtr     cap  portal
  pyr         0        0       8       ΓÇö
  arch        4        4      16       3
  col         3        5      16       ΓÇö
  ant         0        0      16       ΓÇö
  palm        2        2      24       ΓÇö
  cact        1        1      20       ΓÇö
  blad        0        0      32       ΓÇö
  sph         0        0       8       ΓÇö
  ribn        0        0       1       ΓÇö
  cube        7        7     256       ΓÇö
  gol         1        4       8       ΓÇö
  gall        0        0      48       ΓÇö
  claimed ground ΓÇö arrivals (11):
  arch t0 (    24.0,   171.6) p(  0,  3) age=4.8
  arch t0 (  -121.6,    29.4) p( -3,  0) age=4.8
  arch t0 (    59.0,  -121.6) p(  1, -3) age=4.8
  cact t2 (   -18.6,   -70.3) p( -1, -2) age=4.8
  palm t2 (   -85.1,    67.2) p( -2,  1) age=4.8
  palm t1 (   -29.5,  -129.4) p( -1, -3) age=4.8
  col t2 (    28.7,  -127.5) p(  0, -3) age=4.8
  arch t2 (   -75.8,  -122.3) p( -2, -3) age=4.8
  col t2 (  -125.8,    81.9) p( -3,  1) age=4.8
  col t2 (   174.2,   -18.1) p(  3, -1) age=4.8
  gol t1 (   178.1,   178.1) p(  3,  3) age=4.8
[METER] window 1478f  fps 44.7  gpu sampled 495f | budget 16.6 ms
[METER] U fill_signal             mean 0.00  max 0.02
[METER] U advance_clock           mean 0.00  max 0.06
[METER] U motion_drivers          mean 0.08  max 1.25
[METER] U motion_bodies           mean 0.00  max 0.01
[METER] U stage_world             mean 0.00  max 0.05
[METER] U transition_machine      mean 1.59  max 973.00
[METER] U stage_fade_and_upload   mean 0.02  max 0.19
[METER] U witness_photographer    mean 0.02  max 8.94
[METER] U clear_input_deltas      mean 0.00  max 0.00
[METER] R witness_harvest         mean 0.01  max 0.33
[METER] R portal_trigger          mean 0.00  max 0.03
[METER] R stream_patches          cpu 0.37/66.13  gpu 1.26/148.77
[METER] R respawn_agents          mean 0.01  max 4.82
[METER] R census_dumps            mean 0.58  max 860.66
[METER] R ribbon_tick             mean 0.05  max 0.75
[METER] R entity_mesh_gen         cpu 0.00/0.11  gpu 0.14/3.28
[METER] R upload_portal_lights    mean 0.00  max 0.09
[METER] R live_card_write         cpu 0.05/1.01  gpu 1.58/1.84
[METER] R dispatch_compute        cpu 0.12/1.15  gpu 1.32/5.05
[METER] R witness_capture         mean 0.00  max 0.15
[METER] R gol_derive_flush        cpu 0.00/1.12  gpu 0.00/0.07
[METER] R gol_zone_compute        cpu 0.05/0.42  gpu 0.02/0.07
[METER] R pawn_aura               mean 0.00  max 0.03
[METER] R orb_sky                 cpu 0.04/2.80  gpu 0.04/0.07
[METER] R ground_entries          mean 0.00  max 0.15
[METER] R placement_correction    cpu 0.00/0.12  gpu 0.02/0.39
[METER] R frustum_cull            cpu 0.07/7.97  gpu 0.02/0.07
[METER] R shadow_pass             cpu 0.18/7.20  gpu 4.08/10.42
[METER] R main_pass               cpu 0.23/3.34  gpu 11.20/25.36
[METER] R snapshot_pass           cpu 0.01/7.95  gpu 0.17/17.83
[METER] R promotion_drain         mean 0.00  max 0.06
[METER] S begin_frame             mean 0.59  max 31.12
[METER] S acquire                 mean 14.77  max 233.26
[METER] S finish_submit           mean 2.12  max 20.34
[METER] S present                 mean 1.41  max 13.22
[METER] S frame_total             mean 22.57  max 1004.64
[METER] U_SUM 1.72   R_SUM 1.81
[METER] residue 0.15  (frame_total 22.57 - U_SUM - R_SUM - S_partials 18.89)
[Possess] No agent within 20.0 units of the point at (-20.7,-9.5)
[Possess] No agent within 20.0 units of the point at (-31.1,-0.4)
[Possess] No agent within 20.0 units of the point at (-51.7,-5.6)
[Possess] No agent within 20.0 units of the point at (-56.7,-11.7)
[Possess] No agent within 20.0 units of the point at (-62.6,-16.3)
[Possess] No agent within 20.0 units of the point at (-68.1,-16.8)
[Ground] zone rects in core: 0
[Photographer] Capture -> layer 4 (Medium) aspect=1.7 pool=32/32
[Photographer] Rendering snapshot -> layer 4
[Possess] No agent within 20.0 units of the point at (-72.5,-17.2)
[Possess] No agent within 20.0 units of the point at (-75.0,-17.5)
[Photographer] Capture -> layer 5 (Close-up) aspect=1.4 pool=32/32
[Photographer] Rendering snapshot -> layer 5
[Possess] No agent within 20.0 units of the point at (-78.2,-17.8)
[Photographer] Capture -> layer 6 (Bird's Eye) aspect=1.3 pool=32/32
[Photographer] Rendering snapshot -> layer 6
[Possess] No agent within 20.0 units of the point at (-114.3,-2.0)
[Possess] No agent within 20.0 units of the point at (-118.7,1.6)
[Possess] No agent within 20.0 units of the point at (-121.3,3.7)
[Possess] No agent within 20.0 units of the point at (-123.0,5.1)
[Possess] No agent within 20.0 units of the point at (-126.4,8.0)
[Incubator] Shutdown
[Device] LOST reason=2 : Device was destroyed.

C:\dev\7t\out\build\the-board-full-release-meter\incubator_dual.exe (process 9072) exited with code 0 (0x0).
To automatically close the console when debugging stops, enable Tools->Options->Debugging->Automatically close the console when debugging stops.
Press any key to close this window . . .