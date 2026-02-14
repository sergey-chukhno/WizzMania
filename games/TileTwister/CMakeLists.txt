cmake_minimum_required(VERSION 3.15)
project(TileTwister VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE) # Force all dependencies to be static

# --- Dependencies ---
include(FetchContent)

# 1. GoogleTest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent GTest from overriding our compiler/linker options
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 2. SDL2
# We try to find it on the system first (e.g. brew install sdl2).
# If not found, we could FetchContent it, but building SDL2 from source can be slow.
# For now, we will rely on find_package. 
# For your Windows teammates, the best approach is actually "vcpkg", 
# but simply fetching it here is a "zero-install" fallback.
# Force Static Build for Simplicity
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)

find_package(SDL2 QUIET)
if(NOT SDL2_FOUND)
  message(STATUS "SDL2 not found on system, fetching from source...")
  FetchContent_Declare(
    SDL2
    URL https://github.com/libsdl-org/SDL/releases/download/release-2.28.5/SDL2-2.28.5.zip
  )
  FetchContent_MakeAvailable(SDL2)
  # SDL2 CMake creates 'SDL2::SDL2-static' or 'SDL2::SDL2' depending on settings
  if(TARGET SDL2-static)
    add_library(SDL2::SDL2 ALIAS SDL2-static)
  elseif(TARGET SDL2)
    add_library(SDL2::SDL2 ALIAS SDL2)
  endif()
endif()

# 3. Freetype (Required for SDL2_ttf)
find_package(Freetype QUIET)
if(NOT Freetype_FOUND)
  message(STATUS "Freetype not found, fetching from source...")
  FetchContent_Declare(
      Freetype
      GIT_REPOSITORY https://github.com/freetype/freetype.git
      GIT_TAG VER-2-13-2
  )
  FetchContent_MakeAvailable(Freetype)
  if(TARGET freetype)
      add_library(Freetype::Freetype ALIAS freetype)
  endif()
endif()

# 4. SDL2_ttf
# Disable vendored freetype to use our modern fetched one
set(SDL2TTF_VENDORED OFF CACHE BOOL "" FORCE) 

find_package(SDL2_ttf QUIET)
if(NOT SDL2_ttf_FOUND)
  message(STATUS "SDL2_ttf not found on system, fetching from source...")
  FetchContent_Declare(
    SDL2_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG release-2.22.0
  )
  FetchContent_MakeAvailable(SDL2_ttf)

# SDL2_image
FetchContent_Declare(
  SDL2_image
  GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
  GIT_TAG        release-2.6.3
)
set(SDL2IMAGE_VENDORED OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_BACKEND_IMAGEIO OFF CACHE BOOL "" FORCE) # Mac fix
set(SDL2IMAGE_INSTALL OFF CACHE BOOL "" FORCE) # Disable install to fix export error
FetchContent_MakeAvailable(SDL2_image)
# SDL2_mixer
FetchContent_Declare(
  SDL2_mixer
  GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
  GIT_TAG        release-2.6.3
)
set(SDL2MIXER_VENDORED OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_OPUS OFF CACHE BOOL "" FORCE)     # Disable extra formats for speed
set(SDL2MIXER_FLAC OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_MOD OFF CACHE BOOL "" FORCE)
set(SDL2MIXER_MIDI OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL2_mixer)

  if(TARGET SDL2_ttf::SDL2_ttf)
     # Good
  elseif(TARGET SDL2_ttf)
    add_library(SDL2_ttf::SDL2_ttf ALIAS SDL2_ttf)
  endif()
endif()

# --- Project Sources ---
# We will define libraries for Core and Engine to enforce our architecture

# Core Library (Pure C++)
add_library(TileTwister_Core
    src/core/Tile.cpp
    src/core/Grid.cpp
    src/core/GameLogic.cpp
)
target_include_directories(TileTwister_Core PUBLIC src/core)

# Engine Library (SDL dependents)
add_library(TileTwister_Engine STATIC
    src/engine/Window.cpp
    src/engine/Renderer.cpp
    src/engine/Font.cpp
    src/engine/Texture.cpp
    src/engine/SoundManager.cpp
)
target_include_directories(TileTwister_Engine PUBLIC src/engine)
target_link_libraries(TileTwister_Engine PUBLIC SDL2::SDL2 SDL2_ttf::SDL2_ttf SDL2_image SDL2_mixer TileTwister_Core)

# Game Executable
add_executable(TileTwister
    src/main.cpp
    src/game/Game.cpp
    src/game/InputManager.cpp
    src/game/AnimationManager.cpp
    src/game/PersistenceManager.cpp
)
target_include_directories(TileTwister PUBLIC src)
target_link_libraries(TileTwister PRIVATE TileTwister_Core TileTwister_Engine)

# --- Testing ---
enable_testing()
add_executable(TileTwister_Tests 
    tests/main_test.cpp
    tests/core/Tile_test.cpp
    tests/core/Grid_test.cpp
    tests/core/GameLogic_test.cpp
)
target_link_libraries(TileTwister_Tests PRIVATE GTest::gtest_main TileTwister_Core)

# --- Integration Tests ---
add_executable(IntegrationTests
    tests/integration/IntegrationTests.cpp
    src/game/PersistenceManager.cpp
)
# Integration tests need Core (Grid/Tile) and access to Game headers
target_include_directories(IntegrationTests PRIVATE src src/game)
target_link_libraries(IntegrationTests PRIVATE GTest::gtest_main TileTwister_Core)

include(GoogleTest)
gtest_discover_tests(TileTwister_Tests)
gtest_discover_tests(IntegrationTests)
