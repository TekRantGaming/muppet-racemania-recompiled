cmake_minimum_required(VERSION 3.20)
set(PSXRECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/psxrecomp" CACHE PATH
    "Path to psxrecomp")
if(NOT EXISTS "${PSXRECOMP_ROOT}/runtime/runtime.cmake")
    message(FATAL_ERROR
        "PSXRecomp runtime is missing: ${PSXRECOMP_ROOT}/runtime/runtime.cmake\n"
        "This project expects the psxrecomp framework checkout at ${PSXRECOMP_ROOT}.\n"
        "Run: git submodule update --init --recursive\n"
        "If psxrecomp/ is not a submodule, set -DPSXRECOMP_ROOT=<path-to-psxrecomp>.")
endif()

project(MuppetRaceMania LANGUAGES C CXX)

# --- Opt-in product surfaces (OFF by default in the framework) -------------
# Scaffold prompts / flags fill these. Without recomp-ui, PSX_RECOMP_UI is OFF.
# The wizard (WIZARD_CMAKE_BLOCK / ENABLE_SETUP_WIZARD) is the first-run disc
# picker; releases are compiled from the committed generated/ C, so it never
# needs to generate or rebuild anything on a player's machine.
set(PSX_RECOMP_UI OFF CACHE BOOL
    "No recomp-ui submodule in this scaffold" FORCE)
# Full netplay UI -- enable after adding recomp-ui + testing:
# if(EXISTS "${PSXRECOMP_ROOT}/lib/recomp-net/CMakeLists.txt")
#     set(PSX_NETPLAY ON CACHE BOOL "..." FORCE)
# endif()
# First-run setup wizard -- enable after testing:
# set(PSX_SETUP_WIZARD ON CACHE BOOL "..." FORCE)
include("${PSXRECOMP_ROOT}/runtime/runtime.cmake")

# This project's own mod catalog. The FRAMEWORK stages it -- together with its
# builtin psx.* packages -- into <exe-dir>/mods/bundled, and refuses to
# configure if a catalog exists here but is not declared. Never copy it by
# hand: five titles did, and framework 4cc04be3's rename of the staged
# directory broke all five silently (bead beads-eio.3.101).
set(PSX_PRELOADED_MODS "${CMAKE_CURRENT_SOURCE_DIR}/mods/preloaded")
if(NOT IS_DIRECTORY "${PSX_PRELOADED_MODS}")
    set(PSX_PRELOADED_MODS "NONE")
endif()

# One program per region: the USA (SLUS-01237) and European (SCES-02008)
# releases are different builds of the game, so each has its own translated
# code (generated/<boot exe>_*) and config. Configure a build folder per region:
#   -DMRM_REGION=USA (default) or -DMRM_REGION=EUR
set(MRM_REGION "USA" CACHE STRING "Game region to build: USA or EUR")
set_property(CACHE MRM_REGION PROPERTY STRINGS USA EUR)
if(MRM_REGION STREQUAL "USA")
    set(MRM_BOOT_EXE "SLUS_012.37")
    set(MRM_GAME_CONFIG "game.toml")
elseif(MRM_REGION STREQUAL "EUR")
    set(MRM_BOOT_EXE "SCES_020.08")
    set(MRM_GAME_CONFIG "game_eur.toml")
else()
    message(FATAL_ERROR "MRM_REGION must be USA or EUR, not '${MRM_REGION}'")
endif()
message(STATUS "Muppet RaceMania region: ${MRM_REGION} (${MRM_BOOT_EXE}, ${MRM_GAME_CONFIG})")

psxrecomp_add_game_runtime(psx-runtime
    PRELOADED_MODS_DIR "${PSX_PRELOADED_MODS}"
    # ENABLE_NETPLAY_IF_PRESENT
    # NETPLAY_LOBBY_URL ws://netplay.retcomm.net:8765
    # ENABLE_SETUP_WIZARD
    WINDOW_TITLE "Muppet RaceMania Recompiled"
    DEFAULT_GAME_CONFIG_PATH "game.toml"
    MAX_PLAYERS 2
    # LAUNCHER_BOXART "${CMAKE_CURRENT_SOURCE_DIR}/launcher_assets/img/boxart.tga"
    APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/psxrecomp.ico"
    # Committed game C (psxrecomp_cli.py generate; git add generated). Release
    # CI configures with -DPSXRECOMP_REQUIRE_GAME_C=ON so a checkout without
    # it fails instead of building a setup host.
    GEN_MARKER "generated/${MRM_BOOT_EXE}_dispatch.c"
    GEN_FULL_GLOB "generated/${MRM_BOOT_EXE}_full_*.c"
    # CODEGEN_SETUP_SOURCES -- add with recomp-ui / wizard later
)
# Muppet RaceMania PC compatibility (mods/preloaded/packages/muppet.pc).
target_sources(psx-runtime PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/muppet_mods.c")
# The TRG launcher is a separate program, so the on-screen text (the frame rate
# counter, hotkey messages) is asked for here rather than by RECOMP_LAUNCHER.
target_compile_definitions(psx-runtime PRIVATE PSX_HOST_OSD_VISUAL=1)

# >>> psxrecomp:recomp-net-headers (managed by tools/update_cmake.{sh,ps1})
# runtime/src/main.cpp includes recomp_net/host_relay.h unconditionally (a
# plain-C types header). With PSX_NETPLAY off nothing adds recomp-net's include
# dir, so expose it whenever the submodule is checked out.
if(EXISTS "${PSXRECOMP_ROOT}/lib/recomp-net/include/recomp_net/host_relay.h")
    target_include_directories(psx-runtime PRIVATE
        "${PSXRECOMP_ROOT}/lib/recomp-net/include")
endif()
# <<< psxrecomp:recomp-net-headers

# Stage runtime configs next to the exe for direct build/ launches. Only copy
# files that exist — never fail POST_BUILD when game_options.toml is absent.
# The region's config is staged as game.toml, the name the runtime looks for.
foreach(_PSX_RUNTIME_CONFIG ${MRM_GAME_CONFIG} game_options.toml)
    if(_PSX_RUNTIME_CONFIG STREQUAL MRM_GAME_CONFIG)
        set(_PSX_STAGED_NAME game.toml)
    else()
        set(_PSX_STAGED_NAME ${_PSX_RUNTIME_CONFIG})
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_PSX_RUNTIME_CONFIG}")
        add_custom_command(TARGET psx-runtime POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/${_PSX_RUNTIME_CONFIG}"
                "$<TARGET_FILE_DIR:psx-runtime>/${_PSX_STAGED_NAME}"
            COMMENT "Staging ${_PSX_RUNTIME_CONFIG}"
            VERBATIM)
    endif()
endforeach()
