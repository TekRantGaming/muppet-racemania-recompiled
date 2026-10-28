# Muppet RaceMania Recompiled's launcher (MuppetRaceMania.exe): TRG Launcher 2.0 in its own SDL2 + OpenGL 3 window.
# A separate build from the game, because the runtime uses SDL3 and the launcher SDL2; the launcher starts the
# game build for the player's disc. Built by build-launcher.bat.
cmake_minimum_required(VERSION 3.20)
project(MuppetRaceManiaLauncher LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
# The C runtime linked in, so the launcher runs on a PC without the Visual C++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(TRG_LAUNCHER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/trg-launcher" CACHE PATH "The TRG Launcher repository")
set(TRG_LAUNCHER_STANDALONE ON CACHE BOOL "" FORCE)
set(TRG_LAUNCHER_BUILD_DEMO OFF CACHE BOOL "" FORCE)
set(TRG_LAUNCHER_2 ON CACHE BOOL "" FORCE)
set(TRG_LAUNCHER2_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
add_subdirectory(${TRG_LAUNCHER_DIR} trg-launcher)

# The version players see and the self updater compares: the project's VERSION file.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" MRM_VERSION LIMIT_COUNT 1)
string(STRIP "${MRM_VERSION}" MRM_VERSION)
string(REPLACE "." "," _commas "${MRM_VERSION}")

add_executable(MuppetRaceMania WIN32 project.cpp launcher.rc)
target_link_libraries(MuppetRaceMania PRIVATE trg::launcher2)
target_compile_definitions(MuppetRaceMania PRIVATE PROJECT_VERSION="${MRM_VERSION}" PROJECT_VERSION_COMMAS=${_commas},0)
if(MSVC)
  target_compile_options(MuppetRaceMania PRIVATE /W4 /utf-8 /permissive-)
  # A window program (no console) that starts at main().
  target_link_options(MuppetRaceMania PRIVATE /ENTRY:mainCRTStartup)
endif()
