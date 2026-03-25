cmake_minimum_required(VERSION 3.10)
project(TicTacToe VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# SFML 3.0 (Try native first, then fallback to FetchContent)
find_package(SFML 3.0 COMPONENTS Graphics Window System QUIET)

if(NOT SFML_FOUND)
    message(STATUS "SFML 3.0 not found natively. Fetching from GitHub...")
    include(FetchContent)
    set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "Disable SFML examples")
    set(SFML_BUILD_DOC OFF CACHE BOOL "Disable SFML documentation")
    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG        3.0.0
    )
    FetchContent_MakeAvailable(SFML)
endif()

# Common Headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../common)



file(GLOB_RECURSE SRC_FILES
    "src/*.cpp"
)

add_executable(TicTacToe ${SRC_FILES})
target_link_libraries(TicTacToe PRIVATE SFML::Graphics SFML::Window SFML::System)

# Install step - dump the executable into WizzMania/bin 
install(TARGETS TicTacToe RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/../../bin)
