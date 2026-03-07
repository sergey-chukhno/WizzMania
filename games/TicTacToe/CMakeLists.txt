cmake_minimum_required(VERSION 3.10)
project(TicTacToe VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# SFML
find_package(SFML 3.0 COMPONENTS Graphics Window System REQUIRED)

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
