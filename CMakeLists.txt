# The Board : C++ / WebGPU engine + React design tools
cmake_minimum_required(VERSION 3.16)
project(the_board VERSION 0.1.0 LANGUAGES CXX)

# ═══ SUNSET_2 — THE GUARD FIRES FIRST, AND NAMES THE ROAD ════════════════════
#
# SUNSET_0's refusal was correct but it was the LAST thing a wrong configure
# printed: the cartridge banner, the seed line and the config axis all spoke
# first, so a tree that was never going to configure looked like it was
# configuring. It is the first thing now. Nothing above this line prints.
#
# The occasion was Visual Studio auto-selecting a retired native preset and
# the guard catching it. The guard did its job; the furniture that invited
# the attempt is gone with SUNSET_2 (CMakePresets.json carries the web flow
# only), and this message names the presets that actually exist rather than
# an emcmake incantation this tree does not use. A sign that names a road
# nobody drives is how the wrong turn gets taken twice.
if(NOT EMSCRIPTEN)
    message(FATAL_ERROR
        "This tree builds the WEB twin only (SUNSET_0). Use the web presets: "
        "cmake --preset the-board-web && cmake --build --preset the-board-web "
        "(also: the-board-web-meter, the-board-web-debug). "
        "Native is archived at tag `native-sunset` — docs/LAWS.md.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── CONFIG AXIS ──────────────────────────────────────────────────────────────
# Debug and Release, and nothing else. The axis outlived the reason it was
# born with (SUNSET_0: Dawn's prebuilt static libraries came in exactly those
# two configurations, and nothing links them any more) because PORT_2c gave it
# a second one: Debug is THE DIAGNOSTIC TWIN, -O0 with SAFE_HEAP and the stack
# canary armed; Release is -O2. Multi-config generators choose per build via
# --config; single-config generators choose per tree via CMAKE_BUILD_TYPE.
get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(IS_MULTI_CONFIG)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING
        "The diagnostic axis (PORT_2c)" FORCE)
else()
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
            "Build type: Debug (diagnostic) or Release (-O2)" FORCE)
    endif()
    if(NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release)$")
        message(FATAL_ERROR
            "CMAKE_BUILD_TYPE '${CMAKE_BUILD_TYPE}' — the axis is Debug or Release. PORT_2c reads it: Debug arms -O0 -g plus ASSERTIONS=2, STACK_OVERFLOW_CHECK=2 and SAFE_HEAP=1; Release pins -O2 and strips them. A third name would select neither.")
    endif()
endif()

# ═══════════════════════════════════════════════════════════════════════════════
# ACTIVE CARTRIDGE SELECTION
# ═══════════════════════════════════════════════════════════════════════════════
#
# Controls which render cartridge the the_board target includes.
# IntelliSense will only index it — no noise from other cartridges.
#
# Change here to switch focus. No need to edit .cpp files.
#
#   Render cartridges:   src/cartridges/<name>/cartridge.hpp
#
#   Available render cartridges: the_board
#
# The analysis side is the BeatClock (src/analysis/beat_clock.hpp,
# CUT_1c) — no analysis cartridge selection exists anymore.
#
# ABOVE THE EMSCRIPTEN GUARD, deliberately. Both twins paste these values
# into enumerators (INCUBATE_DEMO into demos/matrix.hpp, T7_INSTRUMENTS into
# core/instruments.hpp). CMake drops an empty ENTRY but keeps an entry whose
# VALUE is empty, so a dial left unset arrives DEFINED AND EMPTY: #ifndef
# never fires and the paste names an enumerator that does not exist. An
# unset value here is a compile error in every TU, never a fallback.

set(T7_RENDER_CARTRIDGE     "the_board"        CACHE STRING "Render cartridge for the 'the_board' target")
set(THE_BOARD_DEMO                    "full"             CACHE STRING "the_board demo sentence (a DemoCol column of demos/matrix.hpp; compile-time door, DEMO-1). Default IS the golden path.")
set(T7_INSTRUMENTS                    "off"              CACHE STRING "Recurring self-measurement (an InstrumentCol column of core/instruments.hpp): off | meter | full. Default off — the shipped frame pays for no reading nobody reads. 'meter' arms the frame meter + its [METER] table for a measurement session.")

# THE PIN (DRAW_0). The one dial here whose EMPTY IS LEGAL — and empty
# is the default. Empty means the world's master seed is DRAWN at boot
# from the wall clock (cartridge.hpp, boot_seed), so the web twin draws
# with no CMake change at all. A number PINS the world to that seed,
# exactly reproducible: -DT7_WORLD_SEED=42 restores the authored world
# (DEMO_SEED, demos/matrix.hpp).
#
# The paste law above does NOT apply to this one, and the guard below is
# why: an unset value here is never pasted at all. The define is dropped
# entirely, so the source reads #ifdef — not an empty paste that would
# name nothing.
set(T7_WORLD_SEED                     ""                 CACHE STRING "The world's master seed. EMPTY (the default) = DRAWN at boot from the wall clock — each visit is a new world. A number PINS the world to that seed and reproduces it exactly (42 is the authored world, DEMO_SEED in demos/matrix.hpp).")

set(T7_WORLD_SEED_DEFS)
if(NOT T7_WORLD_SEED STREQUAL "")
    set(T7_WORLD_SEED_DEFS T7_WORLD_SEED=${T7_WORLD_SEED})
endif()

message(STATUS "")
message(STATUS "Active cartridges:")
message(STATUS "  the_board -> render: ${T7_RENDER_CARTRIDGE} demo: ${THE_BOARD_DEMO}")
message(STATUS "  Instruments:    ${T7_INSTRUMENTS}")
if(T7_WORLD_SEED STREQUAL "")
    message(STATUS "  World seed:     drawn at boot (wall clock)")
else()
    message(STATUS "  World seed:     pinned to ${T7_WORLD_SEED}")
endif()
message(STATUS "")

# ═══════════════════════════════════════════════════════════════════════════════
# BUILD INFO
# ═══════════════════════════════════════════════════════════════════════════════

message(STATUS "")
message(STATUS "Configuration complete:")
message(STATUS "  Generator: ${CMAKE_GENERATOR}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "")


# ═══════════════════════════════════════════════════════════════════════════════
# THE BOARD — Render cartridge development
# ═══════════════════════════════════════════════════════════════════════════════
#
# The render cartridge (the_board) driven by the BeatClock.
# Uses T7_RENDER_CARTRIDGE.
#
# Build:  cmake --build --preset the-board-web  (full chain: CLAUDE.md)
#

# Only the active cartridge's headers → scoped IntelliSense
file(GLOB_RECURSE DUAL_RENDER_HEADERS
    "src/cartridges/${T7_RENDER_CARTRIDGE}/*.hpp"
)

add_executable(the_board
    src/the_board.cpp
    src/external/stb_image.cpp
    ${DUAL_RENDER_HEADERS}
)

target_include_directories(the_board PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# ── PURSE_0 R2 — THE BUILD STAMP ─────────────────────────────────────
#
# The program states its own provenance at boot. The wasm sha names the
# ARTIFACT (web_dist.py's __BUILD_ID__); this names the TREE.
#
# IT LIVES IN THE BUILD DIRECTORY, and that is the whole design decision.
# Every other generated file here is tracked and hand-run; a stamp cannot
# be, because it carries a timestamp — a tracked copy would dirty the tree
# on every build and hold the binding gate's S-6 witness ("porcelain
# clean; HEAD == pushed tip") red forever, and `git describe --dirty`
# would report dirt caused by its own output. tools/build_stamp.py's
# banner carries the full reasoning.
#
# TWO STEPS, AND EACH ANSWERS A DIFFERENT QUESTION. The configure-time run
# guarantees the file EXISTS before the first compile; the custom target is
# always out of date by construction, which is the only way to observe HEAD
# moving WITHOUT a reconfigure (R-E). The script writes only when the
# string differs, so an always-run step costs no relink.
#
# A PROVENANCE INSTRUMENT MUST NOT BE ABLE TO BREAK THE BUILD. With no
# Python interpreter the configure writes an honest "unknown" stamp once
# and no target is added: the program compiles and says it does not know,
# which is the one thing a stamp may say other than the truth.
set(T7_STAMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen")
set(T7_STAMP_FILE "${T7_STAMP_DIR}/build_stamp.gen.inc")
set(T7_STAMP_TOOL "${CMAKE_CURRENT_SOURCE_DIR}/tools/build_stamp.py")
file(MAKE_DIRECTORY "${T7_STAMP_DIR}")

# TWO WAYS TO FIND THE INTERPRETER, because one of them is enough to lose
# the instrument silently. find_package is the correct road; find_program
# on both spellings is the net under it (Windows ships `python`, not
# `python3`, and this tree's own build chain is driven from a .bat).
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    set(T7_PYTHON "${Python3_EXECUTABLE}")
else()
    find_program(T7_PYTHON NAMES python3 python)
endif()

if(T7_PYTHON)
    execute_process(COMMAND "${T7_PYTHON}" "${T7_STAMP_TOOL}" "${T7_STAMP_FILE}"
                    RESULT_VARIABLE T7_STAMP_RC)
else()
    set(T7_STAMP_RC 1)
endif()

if(T7_STAMP_RC EQUAL 0)
    add_custom_target(t7_build_stamp
        BYPRODUCTS "${T7_STAMP_FILE}"
        COMMAND "${T7_PYTHON}" "${T7_STAMP_TOOL}" "${T7_STAMP_FILE}"
        COMMENT "build stamp (git describe + UTC; rewritten only on change)"
        VERBATIM)
    add_dependencies(the_board t7_build_stamp)
    message(STATUS "  Build stamp:    ${T7_STAMP_FILE}")
else()
    file(WRITE "${T7_STAMP_FILE}"
         "// GENERATED at configure time — the stamp tool did not run.\n"
         "inline constexpr const char* BUILD_STAMP = \"unknown\";\n")
    message(STATUS "  Build stamp:    unknown (the stamp tool did not run; it says so)")
endif()

target_include_directories(the_board PRIVATE "${T7_STAMP_DIR}")

if(EMSCRIPTEN)
    # ── PORT_1c: the web twin ────────────────────────────────────────────
    # GLFW verbatim via contrib.glfw3 (emscripten ports registry, >= 3.1.55).
    # WebGPU via the emdawnwebgpu generation VENDORED IN THIS TREE — and only
    # that one.
    #
    # F5F — A WITNESS ATTESTS THE ARTIFACT, NOT THE INTENTION.
    # F5-a pinned a generation on paper and wired a PREFERENCE for it: use
    # the vendored port if its bytes are present, else fall back to the port
    # built into Emscripten (the bare registry name, deliberately not spelled
    # here so the invariant grep for it stays clean). The bytes were never
    # fetched, so every build took the fallback — and that port downloads the
    # EMSDK's own pinned version, a generation that predates the immediates
    # API. The result was three compile errors
    # (console.hpp HasWGSLLanguageFeature/ImmediateAddressSpace,
    # render_passes.hpp SetImmediates ×2) that read as code defects and were
    # a missing payload. A preference is what let that happen quietly: it had
    # a second thing it was willing to be.
    #
    # So there is no preference here any more, and no override. One path,
    # checked once; absence is a configure-time FATAL. The payload is
    # third_party/emdawnwebgpu/emdawnwebgpu_pkg/ and PINNED.md is its receipt.
    set(T7_EMDAWNWEBGPU_PORT
        "${CMAKE_SOURCE_DIR}/third_party/emdawnwebgpu/emdawnwebgpu_pkg/emdawnwebgpu.port.py")
    if(NOT EXISTS "${T7_EMDAWNWEBGPU_PORT}")
      message(FATAL_ERROR
        "emdawnwebgpu payload missing at ${T7_EMDAWNWEBGPU_PORT}. "
        "The build consumes ONLY the in-tree generation (F5F). "
        "Fetch per third_party/emdawnwebgpu/PINNED.md. "
        "A witness attests the artifact, not the intention.")
    endif()
    message(STATUS "  emdawnwebgpu port: ${T7_EMDAWNWEBGPU_PORT}")
    message(STATUS "    generation:      in-tree pin (third_party/emdawnwebgpu/PINNED.md)")
    # Both --use-port flags must reach BOTH the compile and the link command
    # (emdawnwebgpu's README requires it), which is why this is one list used
    # at both sites rather than two spellings that can drift apart.
    set(T7_WEB_PORTS
        "--use-port=contrib.glfw3"
        "--use-port=${T7_EMDAWNWEBGPU_PORT}"
    )

    # PORT_2c — THE DIAGNOSTIC TWIN. -O2 strips the checks that would
    # have NAMED the PORT_2a stack overflow in one line instead of
    # costing a hypothesis; Debug arms them. The split rides the CONFIG
    # AXIS that already exists (Debug|Release, enforced at the head of
    # this file) rather than a new dial — one fact, one home. Debug is
    # slow by construction: SAFE_HEAP instruments every load and store.
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(T7_WEB_OPT -O0 -g)
        set(T7_WEB_DIAG
            "-sASSERTIONS=2"           # verbose runtime checks + stack trace on abort
            "-sSTACK_OVERFLOW_CHECK=2" # full canary + guard page; names the overflow
            "-sSAFE_HEAP=1"            # traps out-of-bounds / misaligned heap access
        )
    else()
        set(T7_WEB_OPT -O2)
        set(T7_WEB_DIAG)
    endif()

    # Program defines.
    target_compile_definitions(the_board PRIVATE
        INCUBATE_RENDER=${T7_RENDER_CARTRIDGE}
        INCUBATE_DEMO=${THE_BOARD_DEMO}
        T7_INSTRUMENTS=${T7_INSTRUMENTS}
        ${T7_WORLD_SEED_DEFS}   # empty ⇒ nothing pasted ⇒ the seed is drawn
    )
    target_compile_options(the_board PRIVATE ${T7_WEB_PORTS} ${T7_WEB_OPT})
    # ONE PRELOAD, AND IT IS PROGRAM (EXHIBIT_0 U3). The MEMFS mapping
    # left is the shader: the loader's 2nd search entry is
    # "src/cartridges/the_board/realization/world.wgsl" and it hits this
    # explicit file mapping verbatim from CWD "/". The search list lives
    # in renderer.hpp; the quoted path above is its own anchor — grep it
    # and you land on the entry, which a line number stops doing the next
    # time anyone edits that file. The shader ships inside the program's
    # bundle because the two are one artifact — but that was a claim
    # about INTENT, and the graph only ever enforced it one way: a
    # program change relinked and repackaged the shader; a shader change
    # did nothing at all. LINK_DEPENDS below is what makes the other
    # direction real.
    #
    # The assets@/assets mapping is GONE. It put the entire exhibition
    # inside the_board.data, which meant every repainted wall and every
    # new canvas cost a link — the compiler woken by a curator. The
    # paintings and the soundtrack are now plain files beside the
    # program, assembled by tools/web_dist.py and fetched by URL at
    # runtime (gallery.hpp, the __EMSCRIPTEN__ forms). Nothing else read
    # /assets: the gallery scan was its only consumer on this twin.
    # PORT_2a — THE STACK. Emscripten's default STACK_SIZE dropped from
    # 5MB to 64KB in 3.1.27; Windows gave the native twin 1MB. At -O2
    # stack-overflow checking is OFF, so an overflow does not abort — it
    # writes past the stack into the heap and dlmalloc's metadata is the
    # first casualty, which is why the fault surfaces inside $malloc from
    # unrelated call sites. 4MB restores native parity with headroom.
    # Costs address space, not runtime: with ALLOW_MEMORY_GROWTH the
    # stack is a reservation carved out of the initial memory.
    # EXHIBIT_0 U2 — THE NETWORK IS A BYTE SOURCE. -sFETCH arms
    # emscripten_fetch, which is how the web twin reads exhibition.json
    # and each painting: the bundle carries the program, the network
    # carries the exhibition. Native was unaffected — this whole block
    # is behind if(EMSCRIPTEN).
    # The shader's path, once. It is a link INPUT (--preload-file) and a
    # link DEPENDENCY (LINK_DEPENDS) — two different facts about one file,
    # and the second was missing. The MEMFS target on the right of the @
    # stays literal: it must match the loader's search string verbatim, so
    # it is a different fact and does not share this home.
    set(T7_WEB_SHADER "${CMAKE_CURRENT_SOURCE_DIR}/src/cartridges/the_board/realization/world.wgsl")
    target_link_options(the_board PRIVATE
        ${T7_WEB_PORTS}
        # ORGAN_0b — the panel's ABI. EMSCRIPTEN_KEEPALIVE keeps the
        # functions from being garbage-collected, but ccall/cwrap are
        # runtime methods and are NOT exported by default; without this the
        # panel would find Module.cwrap undefined and wait forever. The
        # cost is a few hundred bytes of glue, on every build, so that an
        # instrument does not need its own build configuration.
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','HEAPU8']"   # DARKROOM_1 — the shell copies developed bytes into this heap
        "-sEXPORTED_FUNCTIONS=['_main','_malloc','_free']"        # DARKROOM_1 — and allocates them here; KEEPALIVE exports ride alongside
        "-sALLOW_MEMORY_GROWTH=1"
        "-sSTACK_SIZE=4MB"
        "-sFETCH"
        ${T7_WEB_OPT}
        ${T7_WEB_DIAG}
        "SHELL:--preload-file ${T7_WEB_SHADER}@/src/cartridges/the_board/realization/world.wgsl"
    )
    # A --preload-file option is a COMMAND-LINE STRING: ninja hashes the
    # command, not the file the command names. So editing world.wgsl left
    # the link CLEAN and the_board.data stale — silently, no error, no
    # rebuild, and the web twin shipped a shader fifteen hours older than
    # its own source while native was correct. LINK_DEPENDS is what makes
    # a shader edit dirty the link. Ninja and Makefile generators only.
    # The native twin needed nothing: it read world.wgsl off disk at
    # runtime and had no bundle that could go stale — which is exactly
    # why the twins disagreed about collisions and only one of them was
    # wrong.
    set_target_properties(the_board PROPERTIES
        OUTPUT_NAME "the_board"
        SUFFIX ".js"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/web"
        LINK_DEPENDS "${T7_WEB_SHADER}"
    )

    # ═══ DARKROOM_1 — THE DARKROOM WORKER'S WASM ═══════════════════════════
    # A second, tiny program: stb_image + core/develop.hpp, nothing else, built
    # for a Worker (ENVIRONMENT=worker, MODULARIZE so the worker script owns
    # its instance). No FETCH, no filesystem, no main. It develops a painting
    # off the main thread; the main program hangs what comes back. The same
    # develop.hpp compiles into both, which is the whole point.
    add_executable(the_board_darkroom
        src/darkroom/darkroom.cpp
        src/external/stb_image.cpp
    )
    target_include_directories(the_board_darkroom PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_compile_options(the_board_darkroom PRIVATE ${T7_WEB_OPT})
    target_link_options(the_board_darkroom PRIVATE
        "-sMODULARIZE=1"
        "-sEXPORT_NAME=createDarkroom"
        "-sENVIRONMENT=worker"
        "-sEXPORTED_FUNCTIONS=['_t7_develop','_malloc','_free']"
        "-sEXPORTED_RUNTIME_METHODS=['HEAPU8','HEAPU32']"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sINITIAL_MEMORY=33554432"
        "-sFILESYSTEM=0"
        "--no-entry"
        ${T7_WEB_OPT}
    )
    set_target_properties(the_board_darkroom PROPERTIES
        OUTPUT_NAME "darkroom"
        SUFFIX ".js"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/web"
    )
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src/cartridges/${T7_RENDER_CARTRIDGE}"
    PREFIX "Render Cartridge"
    FILES ${DUAL_RENDER_HEADERS}
)
