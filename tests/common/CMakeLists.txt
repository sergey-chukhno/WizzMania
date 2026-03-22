project(WizzManiaTestsCommon)

# GoogleTest based unit tests
add_executable(unit_tests_common test_packet_gtest.cpp)
target_link_libraries(unit_tests_common 
    PRIVATE 
        wizz_common
        GTest::gtest_main
)

# Register with CTest
include(GoogleTest)
gtest_discover_tests(unit_tests_common)

# Support legacy test during transition (optional)
add_executable(legacy_test_packet test_packet.cpp)
target_link_libraries(legacy_test_packet PRIVATE wizz_common)
