cmake_minimum_required(VERSION 3.16)
project(WizzMania LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(WizzMania main.cpp)

target_include_directories(WizzMania PRIVATE
    ${CMAKE_SOURCE_DIR}/Crow/include
    ${CMAKE_SOURCE_DIR}/third_party/asio/include
)

target_compile_definitions(WizzMania PRIVATE 
    ASIO_STANDALONE
    ASIO_DISABLE_IOCP
    _WIN32_WINNT=0x0601
)
if (WIN32)
    target_link_libraries(WizzMania ws2_32)
endif()
