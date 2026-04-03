project(ServerTests)

# Link against common and server
# wizz_server_lib is defined in server/CMakeLists.txt

# Automated Suite (GTest)
add_executable(unit_tests_server 
    test_session_manager.cpp
    test_packet_router.cpp
    test_db_rules.cpp
    test_game_routing.cpp
    test_rate_limiter_unit.cpp
    test_shield_integration.cpp
)

add_executable(integration_test_server
    integration_test.cpp
)

target_link_libraries(unit_tests_server 
    PRIVATE 
        wizz_server_lib
        GTest::gtest_main
        GTest::gmock_main
)

target_link_libraries(integration_test_server
    PRIVATE
        wizz_server_lib
        GTest::gtest_main
)

target_include_directories(unit_tests_server PRIVATE 
    ../../server
    ../../server/handlers
)

target_include_directories(integration_test_server PRIVATE 
    ../../server
    ../../server/handlers
)

include(GoogleTest)
gtest_discover_tests(unit_tests_server)
gtest_discover_tests(integration_test_server)
