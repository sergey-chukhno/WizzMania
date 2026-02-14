cmake_minimum_required(VERSION 3.16)

# Project Metadata
project(WizzMania VERSION 0.1.0 LANGUAGES CXX)

# Standard C++17 (Required for modern features like std::optional, std::variant)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# Testing
enable_testing()
add_subdirectory(tests)