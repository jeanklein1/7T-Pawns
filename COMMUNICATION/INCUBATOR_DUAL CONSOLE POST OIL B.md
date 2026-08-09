
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
[Cartridge] GPUState init:    68 ms
[SPINE] validated: 9 update rows + 22 render rows + 12 dispatch rows name-checked; O-#/RC laws static-asserted
Loaded shader from: ../../../src/cartridges/the_board/realization/world.wgsl
[Renderer] Shader compile:    330 ms
  [Pipeline] update_player_agent: 3684 ms
  [Pipeline] update_other_agents: 4669 ms
  [Pipeline] update_camera: 426 ms
  [Pipeline] update_sphere: 711 ms
  [Pipeline] update_cube: 938 ms
  [Pipeline] compute_vp: 483 ms
  [Pipeline] gen_patch_heights: 1131 ms
  [Pipeline] gen_patch_gradients: 372 ms
  [Pipeline] gen_patch_cells: 1029 ms
  [Pipeline] compute_ribbon_rings: 373 ms
  [Pipeline] compute_photographer_vp: 328 ms
  [Pipeline] compute_entity_placement: 486 ms
  [Pipeline] frustum_cull_patches: 483 ms
  [Pipeline] compute_pawn_aura: 1299 ms
  [Pipeline] write_live_card_heights: 662 ms
  [Pipeline] write_live_card_resolve: 367 ms
  [Pipeline] orb_init: 371 ms
  [Pipeline] orb_dynamics: 538 ms
  [Pipeline] orb_recolor: 335 ms
  [Pipeline] orb_state_prev_copy: 277 ms
  [Pipeline] zone_gol_sync: 300 ms
  [Pipeline] zone_gol_evolve: 338 ms
  [Pipeline] zone_derive_params: 574 ms
  [Pipeline] zone_seed_mask: 539 ms
  [Pipeline] arch_mesh_gen: 1129 ms
  [Pipeline] column_mesh_gen: 2047 ms
  [Pipeline] palm_mesh_gen: 625 ms
  [Pipeline] cactus_mesh_gen: 699 ms
  [Pipeline] blade_cluster_mesh_gen: 513 ms
  [Pipeline] patch_terrain: 5291 ms
  [Pipeline] patch_terrain_indirect: 5128 ms
  [Pipeline] pawn: 3886 ms
  [Pipeline] sphere: 3029 ms
  [Pipeline] monolith: 4171 ms
  [Pipeline] arch: 980 ms
  [Pipeline] column: 1015 ms
  [Pipeline] palm: 1010 ms
  [Pipeline] cactus: 1179 ms
  [Pipeline] blade: 981 ms
  [Pipeline] shell: 966 ms
  [Pipeline] ribbon: 801 ms
  [Pipeline] orb: 517 ms
  [Pipeline] gallery_frame: 606 ms
  [Pipeline] wall_painting_canvas: 804 ms
  [Pipeline] wall_painting_frame: 615 ms
  [Pipeline] shadow_patch_terrain: 323 ms
  [Pipeline] shadow_pawn: 895 ms
  [Pipeline] shadow_sphere: 1740 ms
  [Pipeline] shadow_monolith: 2480 ms
  [Pipeline] shadow_arch: 260 ms
  [Pipeline] shadow_column: 253 ms
  [Pipeline] shadow_palm: 267 ms
  [Pipeline] shadow_cactus: 253 ms
  [Pipeline] shadow_blade: 279 ms
  [Pipeline] shadow_shell: 258 ms
  [Pipeline] shadow_ribbon: 326 ms
  [Pipeline] shadow_gallery_frame: 314 ms
  [Pipeline] shadow_wall_painting: 355 ms
  [Pipeline] fade_overlay: 514 ms

[Renderer] Pipelines by compile time (descending):
      5291 ms  patch_terrain
      5128 ms  patch_terrain_indirect
      4669 ms  update_other_agents
      4171 ms  monolith
      3886 ms  pawn
      3684 ms  update_player_agent
      3029 ms  sphere
      2480 ms  shadow_monolith
      2047 ms  column_mesh_gen
      1740 ms  shadow_sphere
      1299 ms  compute_pawn_aura
      1179 ms  cactus
      1131 ms  gen_patch_heights
      1129 ms  arch_mesh_gen
      1029 ms  gen_patch_cells
      1015 ms  column
      1010 ms  palm
       981 ms  blade
       980 ms  arch
       966 ms  shell
       938 ms  update_cube
       895 ms  shadow_pawn
       804 ms  wall_painting_canvas
       801 ms  ribbon
       711 ms  update_sphere
       699 ms  cactus_mesh_gen
       662 ms  write_live_card_heights
       625 ms  palm_mesh_gen
       615 ms  wall_painting_frame
       606 ms  gallery_frame
       574 ms  zone_derive_params
       539 ms  zone_seed_mask
       538 ms  orb_dynamics
       517 ms  orb
       514 ms  fade_overlay
       513 ms  blade_cluster_mesh_gen
       486 ms  compute_entity_placement
       483 ms  compute_vp
       483 ms  frustum_cull_patches
       426 ms  update_camera
       373 ms  compute_ribbon_rings
       372 ms  gen_patch_gradients
       371 ms  orb_init
       367 ms  write_live_card_resolve
       355 ms  shadow_wall_painting
       338 ms  zone_gol_evolve
       335 ms  orb_recolor
       328 ms  compute_photographer_vp
       326 ms  shadow_ribbon
       323 ms  shadow_patch_terrain
       314 ms  shadow_gallery_frame
       300 ms  zone_gol_sync
       279 ms  shadow_blade
       277 ms  orb_state_prev_copy
       267 ms  shadow_palm
       260 ms  shadow_arch
       258 ms  shadow_shell
       253 ms  shadow_column
       253 ms  shadow_cactus

[Renderer] Compute pipelines: 25770 ms
[Renderer] Render pipelines:  39539 ms
[Renderer] Total pipelines:   65309 ms
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
[Cartridge] Renderer init:    66270 ms
[Cartridge] Patch system:     1264 ms
[Cartridge] Total init:       67535 ms

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
[METER] window 1f  fps 6.0  gpu sampled 0f | budget 16.6 ms
[METER] U fill_signal             mean 0.01  max 0.01
[METER] U advance_clock           mean 0.00  max 0.00
[METER] U motion_drivers          mean 0.06  max 0.06
[METER] U motion_bodies           mean 0.01  max 0.01
[METER] U stage_world             mean 0.00  max 0.00
[METER] U transition_machine      mean 0.00  max 0.00
[METER] U stage_fade_and_upload   mean 0.01  max 0.01
[METER] U witness_photographer    mean 0.00  max 0.00
[METER] U clear_input_deltas      mean 0.00  max 0.00
[METER] R witness_harvest         mean 0.00  max 0.00
[METER] R portal_trigger          mean 0.00  max 0.00
[METER] R stream_patches          mean 5.09  max 5.09
[METER] R respawn_agents          mean 0.01  max 0.01
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
[METER] U_SUM 0.09   R_SUM 5.09
[METER] residue -5.18  (frame_total 0.00 - U_SUM - R_SUM - S_partials 0.00)
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
  palm       20       20       0      15
  cact        8        8       0       7
  blad        8        8       0       7
  sph         3        ΓÇö       ΓÇö       ΓÇö
  ribn        1        1       0       0
  cube       38        ΓÇö       ΓÇö       ΓÇö
  gol         2        2       0       2
  gall        0        0       0       0
  TOTAL     108       67       0      49    footprints 67/128
  fam      live   hi-wtr     cap  portal
  pyr         2        2       8       ΓÇö
  arch        8        8      16       7
  col        11       11      16       ΓÇö
  ant         7        7      16       ΓÇö
  palm       20       20      24       ΓÇö
  cact        8        8      20       ΓÇö
  blad        8        8      32       ΓÇö
  sph         3        3       8       ΓÇö
  ribn        1        1       1       ΓÇö
  cube       38       38     256       ΓÇö
  gol         2        2       8       ΓÇö
  gall        0        0      48       ΓÇö
  claimed ground ΓÇö arrivals (49):
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
    ... +37 more
[METER] window 1413f  fps 45.6  gpu sampled 473f | budget 16.6 ms
[METER] U fill_signal             mean 0.00  max 0.09
[METER] U advance_clock           mean 0.00  max 0.06
[METER] U motion_drivers          mean 0.12  max 9.67
[METER] U motion_bodies           mean 0.00  max 0.17
[METER] U stage_world             mean 0.00  max 0.73
[METER] U transition_machine      mean 0.00  max 0.01
[METER] U stage_fade_and_upload   mean 0.03  max 4.12
[METER] U witness_photographer    mean 0.00  max 0.02
[METER] U clear_input_deltas      mean 0.00  max 0.00
[METER] R witness_harvest         mean 0.01  max 1.41
[METER] R portal_trigger          mean 0.00  max 0.06
[METER] R stream_patches          cpu 0.17/7.96  gpu 0.22/20.71
[METER] R respawn_agents          mean 0.01  max 4.54
[METER] R census_dumps            mean 0.13  max 186.13
[METER] R ribbon_tick             mean 0.15  max 16.52
[METER] R entity_mesh_gen         cpu 0.00/0.10  gpu 0.06/3.01
[METER] R upload_portal_lights    mean 0.00  max 0.04
[METER] R live_card_write         cpu 0.09/2.95  gpu 1.07/1.11
[METER] R dispatch_compute        cpu 0.21/8.35  gpu 1.32/1.38
[METER] R witness_capture         mean 0.00  max 1.03
[METER] R gol_derive_flush        mean 0.00  max 0.48
[METER] R gol_zone_compute        cpu 0.11/5.04  gpu 0.02/0.07
[METER] R pawn_aura               mean 0.00  max 0.94
[METER] R orb_sky                 cpu 0.08/5.06  gpu 0.04/0.07
[METER] R ground_entries          mean 0.00  max 0.07
[METER] R placement_correction    cpu 0.00/0.11  gpu 0.01/0.33
[METER] R frustum_cull            cpu 0.07/2.76  gpu 0.02/0.07
[METER] R shadow_pass             cpu 0.19/6.15  gpu 4.59/4.72
[METER] R main_pass               cpu 0.45/152.58  gpu 13.19/13.50
[METER] R snapshot_pass           mean 0.00  max 0.04
[METER] R promotion_drain         mean 0.00  max 0.29
[METER] S begin_frame             mean 0.03  max 4.35
[METER] S acquire                 mean 14.06  max 47.39
[METER] S finish_submit           mean 3.04  max 49.58
[METER] S present                 mean 2.42  max 296.61
[METER] S frame_total             mean 21.67  max 730.89
[METER] U_SUM 0.16   R_SUM 1.69
[METER] residue 0.28  (frame_total 21.67 - U_SUM - R_SUM - S_partials 19.55)
[Agents] Respawn 1 around (0.0,0.0)
[Agents] Respawn 1 around (10.2,-0.9)
[Agents] Respawn 1 around (19.6,-1.1)
[Photographer] Capture -> layer 0 (Portrait) aspect=0.7 pool=1/32
[Photographer] Rendering snapshot -> layer 0
[Photographer] Capture -> layer 1 (Panoramic) aspect=1.9 pool=2/32
[Photographer] Rendering snapshot -> layer 1
[CENSUS t=   60.1 trigger=periodic]
  fam    active  claimed   delta     new
  pyr         2        2       0       0
  arch        8        8       0       0
  col        11       11       0       1
  ant         7        7       0       0
  palm       20       20       0       0
  cact        8        8       0       0
  blad        8        8       0       0
  sph         3        ΓÇö       ΓÇö       ΓÇö
  ribn        1        1       0       0
  cube       39        ΓÇö       ΓÇö       ΓÇö
  gol         2        2       0       0
  gall        0        0       0       0
  TOTAL     109       67       0       1    footprints 67/128
  fam      live   hi-wtr     cap  portal
  pyr         2        2       8       ΓÇö
  arch        8        8      16       7
  col        11       11      16       ΓÇö
  ant         7        7      16       ΓÇö
  palm       20       20      24       ΓÇö
  cact        8        8      20       ΓÇö
  blad        8        8      32       ΓÇö
  sph         3        3       8       ΓÇö
  ribn        1        1       1       ΓÇö
  cube       39       39     256       ΓÇö
  gol         2        2       8       ΓÇö
  gall        0        0      48       ΓÇö
  claimed ground ΓÇö arrivals (1):
  col t2 (   181.5,   376.8) p(  3,  7) age=1.7
[METER] window 1489f  fps 48.4  gpu sampled 500f | budget 16.6 ms
[METER] U fill_signal             mean 0.00  max 0.07
[METER] U advance_clock           mean 0.00  max 0.01
[METER] U motion_drivers          mean 0.10  max 8.01
[METER] U motion_bodies           mean 0.00  max 0.06
[METER] U stage_world             mean 0.00  max 0.10
[METER] U transition_machine      mean 0.00  max 0.01
[METER] U stage_fade_and_upload   mean 0.02  max 1.76
[METER] U witness_photographer    mean 0.02  max 26.40
[METER] U clear_input_deltas      mean 0.00  max 0.08
[METER] R witness_harvest         mean 0.01  max 1.02
[METER] R portal_trigger          mean 0.00  max 0.01
[METER] R stream_patches          cpu 0.12/3.07  gpu 0.03/4.19
[METER] R respawn_agents          mean 0.01  max 6.65
[METER] R census_dumps            mean 0.40  max 590.28
[METER] R ribbon_tick             mean 0.10  max 5.46
[METER] R entity_mesh_gen         cpu 0.00/0.43  gpu 0.02/2.29
[METER] R upload_portal_lights    mean 0.00  max 0.12
[METER] R live_card_write         cpu 0.08/8.22  gpu 1.10/6.36
[METER] R dispatch_compute        cpu 0.16/11.24  gpu 1.35/7.73
[METER] R witness_capture         mean 0.03  max 46.00
[METER] R gol_derive_flush        mean 0.00  max 0.01
[METER] R gol_zone_compute        cpu 0.08/9.72  gpu 0.02/0.07
[METER] R pawn_aura               mean 0.00  max 0.17
[METER] R orb_sky                 cpu 0.07/6.79  gpu 0.05/0.26
[METER] R ground_entries          mean 0.00  max 0.10
[METER] R placement_correction    cpu 0.00/0.06  gpu 0.00/0.33
[METER] R frustum_cull            cpu 0.05/4.45  gpu 0.02/0.07
[METER] R shadow_pass             cpu 0.15/14.01  gpu 4.65/22.15
[METER] R main_pass               cpu 0.26/12.85  gpu 12.12/84.54
[METER] R snapshot_pass           mean 0.00  max 2.84
[METER] R promotion_drain         mean 0.00  max 0.05
[METER] S begin_frame             mean 0.78  max 506.46
[METER] S acquire                 mean 13.96  max 121.04
[METER] S finish_submit           mean 2.31  max 53.06
[METER] S present                 mean 1.93  max 108.29
[METER] S frame_total             mean 20.84  max 606.63
[METER] U_SUM 0.15   R_SUM 1.54
[METER] residue 0.17  (frame_total 20.84 - U_SUM - R_SUM - S_partials 18.98)
[Agents] Respawn 1 around (52.2,18.2)
[Photographer] Capture -> layer 2 (Portrait) aspect=0.6 pool=3/32
[Photographer] Rendering snapshot -> layer 2
[Agents] Respawn 1 around (81.4,21.4)
[Photographer] Capture -> layer 3 (Medium) aspect=1.4 pool=4/32
[Photographer] Rendering snapshot -> layer 3
[Photographer] Capture -> layer 4 (Bird's Eye) aspect=1.0 pool=5/32
[Photographer] Rendering snapshot -> layer 4
[Agents] Respawn 1 around (91.3,26.1)
[Ground] zones active anywhere: 1
[Photographer] Capture -> layer 5 (Panoramic) aspect=2.3 pool=6/32
[Photographer] Rendering snapshot -> layer 5
[Photographer] Capture -> layer 6 (Low Angle) aspect=2.0 pool=7/32
[Photographer] Rendering snapshot -> layer 6
[Photographer] Capture -> layer 7 (Portrait) aspect=0.6 pool=8/32
[Photographer] Rendering snapshot -> layer 7
[Agents] Respawn 1 around (129.6,39.3)
[GoL] Pulse slot=0 node=(4,-2) corner=(512.5,-206.2) host=(10,-4) period=0.4
[Ground] zones active anywhere: 2
[Photographer] Capture -> layer 8 (Medium) aspect=1.6 pool=9/32
[Photographer] Rendering snapshot -> layer 8
[Agents] Respawn 1 around (161.5,46.8)
[Agents] Respawn 1 around (167.7,47.5)
[GoL] Pulse slot=2 node=(-2,3) corner=(-206.2,393.8) host=(-4,8) period=0.8
[Ground] zones active anywhere: 3
[Agents] Respawn 1 around (195.8,50.6)
[Ground] zones active anywhere: 2
[Photographer] Capture -> layer 9 (Panoramic) aspect=2.1 pool=10/32
[Photographer] Rendering snapshot -> layer 9
[Agents] Respawn 1 around (210.4,52.2)
[Agents] Respawn 1 around (224.6,53.7)
[Gallery] slot=0 at (630.7,-272.9) host=(12,-6) arch=0 paintings=5/5 type=snap
[Photographer] Capture -> layer 10 (Medium) aspect=1.7 pool=11/32
[Photographer] Rendering snapshot -> layer 10
[Agents] Respawn 1 around (284.4,53.7)
[Photographer] Capture -> layer 11 (Panoramic) aspect=2.2 pool=12/32
[Photographer] Rendering snapshot -> layer 11
[GoL] Pulse slot=2 node=(5,2) corner=(646.9,287.5) host=(13,6) HEIGHT period=2.2
[Ground] zones active anywhere: 3
[Agents] Respawn 1 around (311.5,0.4)
[Gallery] slot=1 at (221.1,-387.6) host=(4,-8) arch=2 paintings=4/4 type=snap
[Photographer] Capture -> layer 12 (Panoramic) aspect=2.3 pool=13/32
[Photographer] Rendering snapshot -> layer 12
[Ground] zones active anywhere: 2
[GoL] Pulse slot=2 node=(5,2) corner=(646.9,287.5) host=(13,6) HEIGHT period=2.2
[Ground] zones active anywhere: 3
[Photographer] Capture -> layer 13 (Panoramic) aspect=2.1 pool=14/32
[Photographer] Rendering snapshot -> layer 13
[Ground] zone rects in core: 1
[Photographer] Capture -> layer 14 (Medium) aspect=1.8 pool=15/32
[Photographer] Rendering snapshot -> layer 14
[Ribbon] EVICT slot=0
[Ribbon] SPAWN slot=0 at (726.7, 277.8) tier=1 len=318.3 near=(14,5) far=(20,4)
[Agents] Respawn 1 around (364.5,-20.6)
[Photographer] Capture -> layer 15 (Close-up) aspect=1.3 pool=16/32
[Photographer] Rendering snapshot -> layer 15
[Photographer] Capture -> layer 16 (Portrait) aspect=0.7 pool=17/32
[Photographer] Rendering snapshot -> layer 16
[GoL] Pulse slot=3 node=(6,-2) corner=(765.6,-193.8) host=(15,-4) HEIGHT period=3.7
[Ground] zones active anywhere: 4
[CENSUS t=   90.1 trigger=periodic]
  fam    active  claimed   delta     new
  pyr         0        0       0       0
  arch       10       10       0       5
  col        15       15       0       9
  ant         4        4       0       1
  palm       23       23       0      14
  cact        8        8       0       4
  blad       10       10       0       6
  sph         3        ΓÇö       ΓÇö       ΓÇö
  ribn        1        1       0       1
  cube       61        ΓÇö       ΓÇö       ΓÇö
  gol         4        4       0       3
  gall        2        2       0       2
  TOTAL     141       77       0      45    footprints 77/128
  fam      live   hi-wtr     cap  portal
  pyr         0        0       8       ΓÇö
  arch       10       11      16       8
  col        15       16      16       ΓÇö
  ant         4        8      16       ΓÇö
  palm       23       23      24       ΓÇö
  cact        8        8      20       ΓÇö
  blad       10       10      32       ΓÇö
  sph         3        3       8       ΓÇö
  ribn        1        1       1       ΓÇö
  cube       61       61     256       ΓÇö
  gol         4        4       8       ΓÇö
  gall        2        2      48       ΓÇö
  claimed ground ΓÇö arrivals (45):
  blad t1 (   418.0,   227.1) p(  8,  4) age=29.6
  palm t0 (   432.6,   278.8) p(  8,  5) age=29.6
  col t2 (   425.6,  -276.5) p(  8, -6) age=29.6
  palm t1 (   430.2,   318.7) p(  8,  6) age=29.6
  palm t2 (   470.0,   -20.8) p(  9, -1) age=26.4
  blad t0 (   472.3,   -20.5) p(  9, -1) age=26.4
  arch t0 (   476.0,   173.5) p(  9,  3) age=26.4
  arch t1 (   526.4,    30.8) p( 10,  0) age=22.6
  col t2 (   527.8,   -75.8) p( 10, -2) age=22.6
  col t2 (   516.9,   173.0) p( 10,  3) age=22.6
  gol t8 (   537.5,  -181.2) p( 10, -4) age=22.6
  ant t4 (   575.2,   132.6) p( 11,  2) age=19.3
    ... +33 more
[METER] window 1645f  fps 53.8  gpu sampled 550f | budget 16.6 ms
[METER] U fill_signal             mean 0.00  max 0.01
[METER] U advance_clock           mean 0.00  max 0.01
[METER] U motion_drivers          mean 0.10  max 1.43
[METER] U motion_bodies           mean 0.00  max 0.21
[METER] U stage_world             mean 0.00  max 0.13
[METER] U transition_machine      mean 0.00  max 0.08
[METER] U stage_fade_and_upload   mean 0.02  max 0.76
[METER] U witness_photographer    mean 0.06  max 20.09
[METER] U clear_input_deltas      mean 0.00  max 0.01
[METER] R witness_harvest         mean 0.01  max 0.17
[METER] R portal_trigger          mean 0.00  max 0.01
[METER] R stream_patches          cpu 0.19/17.25  gpu 0.17/4.52
[METER] R respawn_agents          mean 0.05  max 22.40
[METER] R census_dumps            mean 0.27  max 447.54
[METER] R ribbon_tick             mean 0.08  max 0.67
[METER] R entity_mesh_gen         cpu 0.00/0.15  gpu 0.12/3.08
[METER] R upload_portal_lights    mean 0.00  max 0.04
[METER] R live_card_write         cpu 0.06/1.34  gpu 1.08/6.36
[METER] R dispatch_compute        cpu 0.14/7.70  gpu 1.43/7.67
[METER] R witness_capture         mean 0.00  max 0.07
[METER] R gol_derive_flush        cpu 0.00/0.96  gpu 0.00/0.07
[METER] R gol_zone_compute        cpu 0.06/0.43  gpu 0.02/0.07
[METER] R pawn_aura               mean 0.00  max 0.11
[METER] R orb_sky                 cpu 0.05/0.26  gpu 0.04/0.20
[METER] R ground_entries          mean 0.00  max 0.27
[METER] R placement_correction    cpu 0.00/0.13  gpu 0.02/0.33
[METER] R frustum_cull            cpu 0.06/8.96  gpu 0.02/0.07
[METER] R shadow_pass             cpu 0.12/0.66  gpu 4.58/21.56
[METER] R main_pass               cpu 0.21/8.25  gpu 10.09/71.04
[METER] R snapshot_pass           cpu 0.03/22.69  gpu 0.04/12.39
[METER] R promotion_drain         mean 0.00  max 0.02
[METER] S begin_frame             mean 0.49  max 13.22
[METER] S acquire                 mean 12.33  max 108.44
[METER] S finish_submit           mean 2.25  max 25.66
[METER] S present                 mean 1.70  max 27.21
[METER] S frame_total             mean 18.47  max 464.15
[METER] U_SUM 0.18   R_SUM 1.37
[METER] residue 0.15  (frame_total 18.47 - U_SUM - R_SUM - S_partials 16.77)
[Gallery] slot=2 at (778.6,-376.5) host=(15,-8) arch=1 paintings=4/4 type=auth
[Gallery] slot=2 at (24.9,-176.8) host=(0,-4) arch=2 paintings=5/5 type=snap
[Ground] zones active anywhere: 3
[Photographer] Capture -> layer 17 (Low Angle) aspect=1.9 pool=18/32
[Photographer] Rendering snapshot -> layer 17
[Ground] zone rects in core: 0
[Ground] zones active anywhere: 2
[Photographer] Capture -> layer 18 (Panoramic) aspect=2.2 pool=19/32
[Photographer] Rendering snapshot -> layer 18
[Photographer] Capture -> layer 19 (Close-up) aspect=1.4 pool=20/32
[Photographer] Rendering snapshot -> layer 19
[Agents] Respawn 1 around (284.8,-20.0)
[Photographer] Capture -> layer 20 (Portrait) aspect=0.7 pool=21/32
[Photographer] Rendering snapshot -> layer 20
[Photographer] Capture -> layer 21 (Panoramic) aspect=2.1 pool=22/32
[Photographer] Rendering snapshot -> layer 21
[Photographer] Capture -> layer 22 (Panoramic) aspect=2.2 pool=23/32
[Photographer] Rendering snapshot -> layer 22
[Portal] GPU trigger: arch 0 -> seed=1898512436 finite=1
[Agents] Respawn 1 around (224.3,-22.2)
[Authored] Loaded: assets/paintings\PAINTING_110.jpeg (1569x1148) ΓåÆ staging 0
[Authored] Scaled ΓåÆ 512x375 (aspect 1.4)
[Authored] Loaded: assets/paintings\PAINTING_111.jpeg (1221x1280) ΓåÆ staging 1
[Authored] Scaled ΓåÆ 488x512 (aspect 1.0)
[Authored] Loaded: assets/paintings\PAINTING_112.jpeg (1600x985) ΓåÆ staging 2
[Authored] Scaled ΓåÆ 512x315 (aspect 1.6)
[Authored] Loaded: assets/paintings\PAINTING_113.jpeg (1555x1600) ΓåÆ staging 3
[Authored] Scaled ΓåÆ 498x512 (aspect 1.0)
[Authored] Rotated 4 slot(s), 32 valid, disk cursor at 36/57
[Lighting] Cathedral (3 lights, E/W walls)
[Mood] Indoor palette: terracotta (idx=2)
[WallPainting] Placed 19 painting(s) + 9 snapshot(s) across 4 walls (SNAPSHOT)
[Shell] Generated FLAT: 20 verts, 30 indices bounds=[-100.0,150.0] wall_h=20.0 crown=20.0 rise=0.0
[Mood] Applied: indoor_flat (mood=1 INDOOR)
[Agents] Spawned 4 for mood 1 around (0.0,0.0)
[AGENTS t=102.2 trigger=mood-transition] 5/32 active, possessed=0 tier:{worker=2 sentinel=3} drv:{player=1 slow_patrol=4}
[CENSUS t=  102.2 trigger=mood-transition]
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
[Photographer] Capture -> layer 30 (Bird's Eye) aspect=1.2 pool=31/32
[Photographer] Rendering snapshot -> layer 30
[Photographer] Capture -> layer 31 (Panoramic) aspect=2.1 pool=32/32
[Photographer] Rendering snapshot -> layer 31
[CENSUS t=  120.1 trigger=periodic]
  fam    active  claimed   delta     new
  pyr         0        0       0       0
  arch        5        5       0       5
  col         1        1       0       1
  ant         1        1       0       1
  palm        5        5       0       5
  cact        0        0       0       0
  blad        0        0       0       0
  sph         0        ΓÇö       ΓÇö       ΓÇö
  ribn        0        0       0       0
  cube        5        ΓÇö       ΓÇö       ΓÇö
  gol         0        0       0       0
  gall        0        0       0       0
  TOTAL      17       12       0      12    footprints 12/128
  fam      live   hi-wtr     cap  portal
  pyr         0        0       8       ΓÇö
  arch        5        5      16       3
  col         1        1      16       ΓÇö
  ant         1        1      16       ΓÇö
  palm        5        5      24       ΓÇö
  cact        0        0      20       ΓÇö
  blad        0        0      32       ΓÇö
  sph         0        0       8       ΓÇö
  ribn        0        0       1       ΓÇö
  cube        5        5     256       ΓÇö
  gol         0        0       8       ΓÇö
  gall        0        0      48       ΓÇö
  claimed ground ΓÇö arrivals (12):
  arch t0 (    14.8,   -71.6) p(  0, -2) age=17.9
  arch t0 (   121.6,    32.8) p(  2,  0) age=17.9
  arch t0 (    13.4,   121.6) p(  0,  2) age=17.9
  palm t0 (   -28.8,    31.5) p( -1,  0) age=17.9
  col t2 (    18.7,    31.5) p(  0,  0) age=17.9
  palm t0 (   -29.7,   -79.3) p( -1, -2) age=17.9
  ant t4 (   -79.0,   -28.2) p( -2, -1) age=17.9
  palm t1 (    78.7,   -34.0) p(  1, -1) age=17.9
  palm t1 (    77.9,    25.6) p(  1,  0) age=17.9
  palm t1 (   -21.3,    66.0) p( -1,  1) age=17.9
  arch t0 (   118.7,   -70.2) p(  2, -2) age=17.9
  arch t0 (   -70.2,   120.1) p( -2,  2) age=17.9
[METER] window 1571f  fps 50.3  gpu sampled 523f | budget 16.6 ms
[METER] U fill_signal             mean 0.00  max 0.05
[METER] U advance_clock           mean 0.00  max 0.01
[METER] U motion_drivers          mean 0.09  max 4.53
[METER] U motion_bodies           mean 0.00  max 0.04
[METER] U stage_world             mean 0.00  max 0.09
[METER] U transition_machine      mean 0.25  max 393.22
[METER] U stage_fade_and_upload   mean 0.02  max 3.52
[METER] U witness_photographer    mean 0.04  max 6.18
[METER] U clear_input_deltas      mean 0.00  max 1.17
[METER] R witness_harvest         mean 0.01  max 0.16
[METER] R portal_trigger          mean 0.00  max 2.53
[METER] R stream_patches          cpu 0.12/16.04  gpu 0.41/151.91
[METER] R respawn_agents          mean 0.01  max 7.61
[METER] R census_dumps            mean 0.56  max 884.96
[METER] R ribbon_tick             mean 0.04  max 1.90
[METER] R entity_mesh_gen         cpu 0.00/0.10  gpu 0.10/23.00
[METER] R upload_portal_lights    mean 0.00  max 0.04
[METER] R live_card_write         cpu 0.03/2.74  gpu 0.48/7.80
[METER] R dispatch_compute        cpu 0.15/5.01  gpu 1.15/9.96
[METER] R witness_capture         mean 0.00  max 0.14
[METER] R gol_derive_flush        mean 0.00  max 0.01
[METER] R gol_zone_compute        cpu 0.03/1.93  gpu 0.01/0.07
[METER] R pawn_aura               mean 0.00  max 0.36
[METER] R orb_sky                 cpu 0.02/0.70  gpu 0.02/0.20
[METER] R ground_entries          mean 0.00  max 0.10
[METER] R placement_correction    cpu 0.00/0.05  gpu 0.01/1.84
[METER] R frustum_cull            cpu 0.05/4.60  gpu 0.02/0.07
[METER] R shadow_pass             cpu 0.22/1.91  gpu 2.98/21.56
[METER] R main_pass               cpu 0.21/1.59  gpu 13.49/34.73
[METER] R snapshot_pass           mean 0.02  max 4.73
[METER] R promotion_drain         mean 0.00  max 0.29
[METER] S begin_frame             mean 0.72  max 14.69
[METER] S acquire                 mean 13.33  max 269.40
[METER] S finish_submit           mean 2.17  max 28.43
[METER] S present                 mean 1.66  max 20.37
[METER] S frame_total             mean 19.90  max 895.76
[METER] U_SUM 0.41   R_SUM 1.48
[METER] residue 0.14  (frame_total 19.90 - U_SUM - R_SUM - S_partials 17.88)
[Photographer] Capture -> layer 0 (Panoramic) aspect=2.3 pool=32/32
[Photographer] Rendering snapshot -> layer 0
[Photographer] Capture -> layer 1 (Panoramic) aspect=1.8 pool=32/32
[Photographer] Rendering snapshot -> layer 1
[Photographer] Capture -> layer 2 (Close-up) aspect=1.4 pool=32/32
[Photographer] Rendering snapshot -> layer 2
[Photographer] Capture -> layer 3 (Low Angle) aspect=1.9 pool=32/32
[Photographer] Rendering snapshot -> layer 3
[Photographer] Capture -> layer 4 (Medium) aspect=1.7 pool=32/32
[Photographer] Rendering snapshot -> layer 4
[Photographer] Capture -> layer 5 (Close-up) aspect=1.4 pool=32/32
[Photographer] Rendering snapshot -> layer 5
[Photographer] Capture -> layer 6 (Bird's Eye) aspect=1.3 pool=32/32
[Photographer] Rendering snapshot -> layer 6
[Photographer] Capture -> layer 7 (Panoramic) aspect=2.3 pool=32/32
[Photographer] Rendering snapshot -> layer 7
[CENSUS t=  150.2 trigger=periodic]
  fam    active  claimed   delta     new
  pyr         0        0       0       0
  arch        5        5       0       0
  col         1        1       0       0
  ant         1        1       0       0
  palm        5        5       0       0
  cact        0        0       0       0
  blad        0        0       0       0
  sph         0        ΓÇö       ΓÇö       ΓÇö
  ribn        0        0       0       0
  cube        5        ΓÇö       ΓÇö       ΓÇö
  gol         0        0       0       0
  gall        0        0       0       0
  TOTAL      17       12       0       0    footprints 12/128
  fam      live   hi-wtr     cap  portal
  pyr         0        0       8       ΓÇö
  arch        5        5      16       3
  col         1        1      16       ΓÇö
  ant         1        1      16       ΓÇö
  palm        5        5      24       ΓÇö
  cact        0        0      20       ΓÇö
  blad        0        0      32       ΓÇö
  sph         0        0       8       ΓÇö
  ribn        0        0       1       ΓÇö
  cube        5        5     256       ΓÇö
  gol         0        0       8       ΓÇö
  gall        0        0      48       ΓÇö
[METER] window 1716f  fps 56.7  gpu sampled 574f | budget 16.6 ms
[METER] U fill_signal             mean 0.00  max 0.03
[METER] U advance_clock           mean 0.00  max 0.06
[METER] U motion_drivers          mean 0.07  max 3.10
[METER] U motion_bodies           mean 0.00  max 0.05
[METER] U stage_world             mean 0.00  max 0.02
[METER] U transition_machine      mean 0.00  max 0.01
[METER] U stage_fade_and_upload   mean 0.02  max 1.47
[METER] U witness_photographer    mean 0.02  max 5.80
[METER] U clear_input_deltas      mean 0.00  max 0.02
[METER] R witness_harvest         mean 0.01  max 0.31
[METER] R portal_trigger          mean 0.00  max 0.01
[METER] R stream_patches          mean 0.07  max 0.81
[METER] R respawn_agents          mean 0.00  max 0.33
[METER] R census_dumps            mean 0.51  max 881.85
[METER] R ribbon_tick             mean 0.01  max 0.39
[METER] R entity_mesh_gen         mean 0.00  max 0.07
[METER] R upload_portal_lights    mean 0.00  max 0.01
[METER] R live_card_write         mean 0.00  max 0.04
[METER] R dispatch_compute        cpu 0.14/9.33  gpu 0.78/4.85
[METER] R witness_capture         mean 0.00  max 0.99
[METER] R gol_derive_flush        mean 0.00  max 0.06
[METER] R gol_zone_compute        mean 0.00  max 0.00
[METER] R pawn_aura               mean 0.00  max 0.03
[METER] R orb_sky                 mean 0.00  max 0.03
[METER] R ground_entries          mean 0.00  max 0.00
[METER] R placement_correction    mean 0.00  max 0.03
[METER] R frustum_cull            cpu 0.05/0.71  gpu 0.03/0.13
[METER] R shadow_pass             cpu 0.26/5.95  gpu 1.78/8.85
[METER] R main_pass               cpu 0.20/13.36  gpu 14.11/90.11
[METER] R snapshot_pass           mean 0.01  max 2.28
[METER] R promotion_drain         mean 0.00  max 0.05
[METER] S begin_frame             mean 0.53  max 29.63
[METER] S acquire                 mean 12.47  max 139.81
[METER] S finish_submit           mean 1.99  max 156.57
[METER] S present                 mean 1.54  max 27.35
[METER] S frame_total             mean 18.06  max 899.93
[METER] U_SUM 0.11   R_SUM 1.27
[METER] residue 0.14  (frame_total 18.06 - U_SUM - R_SUM - S_partials 16.53)
[Photographer] Capture -> layer 8 (Portrait) aspect=0.6 pool=32/32
[Photographer] Rendering snapshot -> layer 8
[Photographer] Capture -> layer 9 (Close-up) aspect=1.3 pool=32/32
[Photographer] Rendering snapshot -> layer 9
[Photographer] Capture -> layer 10 (Medium) aspect=1.6 pool=32/32
[Photographer] Rendering snapshot -> layer 10
[Photographer] Capture -> layer 11 (Panoramic) aspect=2.3 pool=32/32
[Photographer] Rendering snapshot -> layer 11
[Photographer] Capture -> layer 12 (Medium) aspect=1.4 pool=32/32
[Photographer] Rendering snapshot -> layer 12
[Photographer] Capture -> layer 13 (Close-up) aspect=1.3 pool=32/32
[Photographer] Rendering snapshot -> layer 13
[Photographer] Capture -> layer 14 (Panoramic) aspect=2.3 pool=32/32
[Photographer] Rendering snapshot -> layer 14
[CENSUS t=  180.2 trigger=periodic]
  fam    active  claimed   delta     new
  pyr         0        0       0       0
  arch        5        5       0       0
  col         1        1       0       0
  ant         1        1       0       0
  palm        5        5       0       0
  cact        0        0       0       0
  blad        0        0       0       0
  sph         0        ΓÇö       ΓÇö       ΓÇö
  ribn        0        0       0       0
  cube        5        ΓÇö       ΓÇö       ΓÇö
  gol         0        0       0       0
  gall        0        0       0       0
  TOTAL      17       12       0       0    footprints 12/128
  fam      live   hi-wtr     cap  portal
  pyr         0        0       8       ΓÇö
  arch        5        5      16       3
  col         1        1      16       ΓÇö
  ant         1        1      16       ΓÇö
  palm        5        5      24       ΓÇö
  cact        0        0      20       ΓÇö
  blad        0        0      32       ΓÇö
  sph         0        0       8       ΓÇö
  ribn        0        0       1       ΓÇö
  cube        5        5     256       ΓÇö
  gol         0        0       8       ΓÇö
  gall        0        0      48       ΓÇö
[METER] window 1713f  fps 56.7  gpu sampled 575f | budget 16.6 ms
[METER] U fill_signal             mean 0.00  max 0.04
[METER] U advance_clock           mean 0.00  max 0.02
[METER] U motion_drivers          mean 0.07  max 4.43
[METER] U motion_bodies           mean 0.00  max 0.03
[METER] U stage_world             mean 0.00  max 0.02
[METER] U transition_machine      mean 0.00  max 0.02
[METER] U stage_fade_and_upload   mean 0.02  max 2.27
[METER] U witness_photographer    mean 0.02  max 8.40
[METER] U clear_input_deltas      mean 0.00  max 0.05
[METER] R witness_harvest         mean 0.01  max 0.14
[METER] R portal_trigger          mean 0.00  max 0.25
[METER] R stream_patches          mean 0.08  max 13.55
[METER] R respawn_agents          mean 0.00  max 0.03
[METER] R census_dumps            mean 0.13  max 215.40
[METER] R ribbon_tick             mean 0.01  max 0.14
[METER] R entity_mesh_gen         mean 0.00  max 0.11
[METER] R upload_portal_lights    mean 0.00  max 0.01
[METER] R live_card_write         mean 0.00  max 0.13
[METER] R dispatch_compute        cpu 0.14/9.23  gpu 0.83/2.49
[METER] R witness_capture         mean 0.00  max 0.18
[METER] R gol_derive_flush        mean 0.00  max 0.02
[METER] R gol_zone_compute        mean 0.00  max 0.01
[METER] R pawn_aura               mean 0.00  max 0.02
[METER] R orb_sky                 mean 0.00  max 0.29
[METER] R ground_entries          mean 0.00  max 0.00
[METER] R placement_correction    mean 0.00  max 0.00
[METER] R frustum_cull            cpu 0.06/16.32  gpu 0.03/0.07
[METER] R shadow_pass             cpu 0.24/14.69  gpu 1.84/4.98
[METER] R main_pass               cpu 0.19/17.61  gpu 14.03/27.85
[METER] R snapshot_pass           mean 0.02  max 19.20
[METER] R promotion_drain         mean 0.00  max 0.02
[METER] S begin_frame             mean 0.28  max 12.51
[METER] S acquire                 mean 12.68  max 29.83
[METER] S finish_submit           mean 1.92  max 35.04
[METER] S present                 mean 1.55  max 27.48
[METER] S frame_total             mean 17.58  max 230.47
[METER] U_SUM 0.12   R_SUM 0.89
[METER] residue 0.14  (frame_total 17.58 - U_SUM - R_SUM - S_partials 16.43)
[Photographer] Capture -> layer 15 (Medium) aspect=1.5 pool=32/32
[Photographer] Rendering snapshot -> layer 15
[Photographer] Capture -> layer 16 (Medium) aspect=1.5 pool=32/32
[Photographer] Rendering snapshot -> layer 16
[Photographer] Capture -> layer 17 (Close-up) aspect=1.5 pool=32/32
[Photographer] Rendering snapshot -> layer 17
[Photographer] Capture -> layer 18 (Portrait) aspect=0.7 pool=32/32
[Photographer] Rendering snapshot -> layer 18
[Photographer] Capture -> layer 19 (Close-up) aspect=1.5 pool=32/32
[Photographer] Rendering snapshot -> layer 19
[Photographer] Capture -> layer 20 (Close-up) aspect=1.5 pool=32/32
[Photographer] Rendering snapshot -> layer 20
[Photographer] Capture -> layer 21 (Panoramic) aspect=2.3 pool=32/32
[Photographer] Rendering snapshot -> layer 21
[Photographer] Capture -> layer 22 (Portrait) aspect=0.7 pool=32/32
[Photographer] Rendering snapshot -> layer 22
[Photographer] Capture -> layer 23 (Low Angle) aspect=2.0 pool=32/32
[Photographer] Rendering snapshot -> layer 23
[Photographer] Capture -> layer 24 (Medium) aspect=1.4 pool=32/32
[Photographer] Rendering snapshot -> layer 24
[CENSUS t=  210.2 trigger=periodic]
  fam    active  claimed   delta     new
  pyr         0        0       0       0
  arch        5        5       0       0
  col         1        1       0       0
  ant         1        1       0       0
  palm        5        5       0       0
  cact        0        0       0       0
  blad        0        0       0       0
  sph         0        ΓÇö       ΓÇö       ΓÇö
  ribn        0        0       0       0
  cube        5        ΓÇö       ΓÇö       ΓÇö
  gol         0        0       0       0
  gall        0        0       0       0
  TOTAL      17       12       0       0    footprints 12/128
  fam      live   hi-wtr     cap  portal
  pyr         0        0       8       ΓÇö
  arch        5        5      16       3
  col         1        1      16       ΓÇö
  ant         1        1      16       ΓÇö
  palm        5        5      24       ΓÇö
  cact        0        0      20       ΓÇö
  blad        0        0      32       ΓÇö
  sph         0        0       8       ΓÇö
  ribn        0        0       1       ΓÇö
  cube        5        5     256       ΓÇö
  gol         0        0       8       ΓÇö
  gall        0        0      48       ΓÇö
[METER] window 1472f  fps 48.7  gpu sampled 490f | budget 16.6 ms
[METER] U fill_signal             mean 0.00  max 0.17
[METER] U advance_clock           mean 0.00  max 0.04
[METER] U motion_drivers          mean 0.08  max 1.15
[METER] U motion_bodies           mean 0.00  max 1.22
[METER] U stage_world             mean 0.00  max 0.09
[METER] U transition_machine      mean 0.00  max 0.07
[METER] U stage_fade_and_upload   mean 0.03  max 3.95
[METER] U witness_photographer    mean 0.04  max 12.98
[METER] U clear_input_deltas      mean 0.00  max 0.01
[METER] R witness_harvest         mean 0.01  max 0.64
[METER] R portal_trigger          mean 0.00  max 0.11
[METER] R stream_patches          mean 0.08  max 0.53
[METER] R respawn_agents          mean 0.00  max 0.10
[METER] R census_dumps            mean 0.30  max 438.47
[METER] R ribbon_tick             mean 0.01  max 0.78
[METER] R entity_mesh_gen         mean 0.00  max 0.08
[METER] R upload_portal_lights    mean 0.00  max 0.10
[METER] R live_card_write         mean 0.00  max 0.05
[METER] R dispatch_compute        cpu 0.16/1.20  gpu 0.83/4.39
[METER] R witness_capture         mean 0.00  max 0.05
[METER] R gol_derive_flush        mean 0.00  max 0.01
[METER] R gol_zone_compute        mean 0.00  max 0.05
[METER] R pawn_aura               mean 0.00  max 0.01
[METER] R orb_sky                 mean 0.00  max 0.10
[METER] R ground_entries          mean 0.00  max 0.00
[METER] R placement_correction    mean 0.00  max 0.12
[METER] R frustum_cull            cpu 0.05/0.39  gpu 0.03/0.07
[METER] R shadow_pass             cpu 0.28/2.50  gpu 1.83/8.85
[METER] R main_pass               cpu 0.21/1.38  gpu 16.92/83.62
[METER] R snapshot_pass           mean 0.02  max 11.59
[METER] R promotion_drain         mean 0.00  max 0.05
[METER] S begin_frame             mean 0.33  max 9.06
[METER] S acquire                 mean 15.09  max 94.82
[METER] S finish_submit           mean 2.08  max 17.23
[METER] S present                 mean 1.69  max 13.59
[METER] S frame_total             mean 20.62  max 453.62
[METER] U_SUM 0.15   R_SUM 1.13
[METER] residue 0.15  (frame_total 20.62 - U_SUM - R_SUM - S_partials 19.19)
[Photographer] Capture -> layer 25 (Medium) aspect=1.7 pool=32/32
[Photographer] Rendering snapshot -> layer 25
[Photographer] Capture -> layer 26 (Panoramic) aspect=2.0 pool=32/32
[Photographer] Rendering snapshot -> layer 26
[Portal] GPU trigger: arch 0 -> seed=42 finite=0
[Photographer] Capture -> layer 27 (Low Angle) aspect=1.5 pool=32/32
[Photographer] Rendering snapshot -> layer 27
[Photographer] Capture -> layer 28 (Panoramic) aspect=2.1 pool=32/32
[Photographer] Rendering snapshot -> layer 28
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
[Authored] Loaded: assets/paintings\PAINTING_501.jpeg (1280x1135) ΓåÆ staging 21
[Authored] Scaled ΓåÆ 512x454 (aspect 1.1)
[Authored] Loaded: assets/paintings\PAINTING_900.jpeg (1440x805) ΓåÆ staging 22
[Authored] Scaled ΓåÆ 512x286 (aspect 1.8)
[Authored] Rotated 19 slot(s), 32 valid, disk cursor at 55/57
[Orbs] Configured: count=128 palette=jwst_deep drag=0.4 noise=0.3 rule=brownian rot=0.0 orbital=0.2 tiers=jwst_stars
[Mood] Applied: open_sunset (mood=0 outdoor)
[Agents] Spawned 10 for mood 0 around (0.0,0.0)
[AGENTS t=214.5 trigger=mood-transition] 11/32 active, possessed=0 tier:{worker=7 scout=4} drv:{player=1 biased_walk=10}
[CENSUS t=  214.5 trigger=mood-transition]
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
[World] Teardown complete, seed=42 mode=open
[Ribbon] SPAWN slot=0 at (-22.7, -27.1) tier=0 len=562.0 near=(-1,-1) far=(1,-12)
[Gallery] slot=0 at (72.0,-132.4) host=(1,-3) arch=1 paintings=3/3 type=snap
[Orbs] Init dispatched: 128 orbs, 2 workgroups
[Gallery] slot=1 at (221.3,175.3) host=(4,3) arch=2 paintings=3/3 type=snap
[GoL] Pulse slot=0 node=(-3,-1) corner=(-350.0,-112.5) host=(-6,-2) HEIGHT period=1.4
[Card] live-card field: LIVE ΓÇö writer runs every frame
[Ground] zones active anywhere: 1
[Gallery] slot=2 at (212.3,-226.8) host=(4,-5) arch=2 paintings=4/4 type=auth
[GoL] Conway slot=1 node=(0,2) corner=(34.4,275.0) host=(1,6) HEIGHT period=13.9
[Ground] zones active anywhere: 2
[Gallery] slot=3 at (281.6,326.3) host=(5,6) arch=2 paintings=4/4 type=snap
[Agents] Respawn 1 around (-3.7,9.0)
[Incubator] Shutdown
[Device] LOST reason=2 : Device was destroyed.

C:\dev\7t\out\build\the-board-full-release-meter\incubator_dual.exe (process 256) exited with code 0 (0x0).
To automatically close the console when debugging stops, enable Tools->Options->Debugging->Automatically close the console when debugging stops.
Press any key to close this window . . .