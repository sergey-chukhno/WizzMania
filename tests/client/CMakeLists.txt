project(WizzManiaTestsClient)

find_package(Qt6 REQUIRED COMPONENTS Core Network Test)

set(CMAKE_AUTOMOC ON)

# Source files needed for testing NetworkManager
set(TEST_C_SOURCES
    test_network_manager.cpp
    ../../client/NetworkManager.cpp
)

add_executable(unit_tests_client ${TEST_C_SOURCES})

target_include_directories(unit_tests_client PRIVATE ../../client)

target_link_libraries(unit_tests_client 
    PRIVATE 
        wizz_common
        Qt6::Core
        Qt6::Network
        Qt6::Test
        GTest::gtest_main
)

# Register with CTest
include(GoogleTest)
gtest_discover_tests(unit_tests_client)
