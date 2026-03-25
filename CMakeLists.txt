cmake_minimum_required(VERSION 3.16)

# Ensure newer policies are enabled when available to silence warnings
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)  # prefer extraction timestamps of download time
endif()

# Project Metadata
project(WizzMania VERSION 0.1.0 LANGUAGES CXX)

# Standard C++17 (Required for modern features like std::optional, std::variant)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SFML 3.0 Global Dependency
# We centralize this at the root because multiple games (BrickBreaker, TicTacToe)
# require it, and having them each call find_package/FetchContent causes conflicts 
# on systems with SFML 2.x installed (like WSL).
include(FetchContent)
find_package(SFML 3.0 COMPONENTS Graphics Window System Audio QUIET)
if(NOT SFML_FOUND)
    message(STATUS "SFML 3.0 not found natively. Fetching from GitHub via FetchContent...")
    set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "Disable SFML examples")
    set(SFML_BUILD_DOC OFF CACHE BOOL "Disable SFML documentation")
    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG        3.0.0
    )
    FetchContent_MakeAvailable(SFML)
endif()

# Global Compiler Warnings (High Quality Code)
if(MSVC)
    add_compile_options(/W4) # High warning level on Windows
else()
    add_compile_options(-Wall -Wextra -Wpedantic) # High warning level on Mac/Linux
endif()

# Core Components
add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client)

# Games Integration
# We check if the directories exist to avoid errors if games are not present
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/games/TileTwister/CMakeLists.txt")
    add_subdirectory(games/TileTwister)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/games/BrickBreaker/CMakeLists.txt")
    add_subdirectory(games/BrickBreaker)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/games/TicTacToe/CMakeLists.txt")
    add_subdirectory(games/TicTacToe)
endif()

# Testing
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
# For Windows: prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_subdirectory(tests)