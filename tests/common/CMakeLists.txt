project(WizzManiaTestsCommon)

# GoogleTest based unit tests for Common logic
add_executable(unit_tests_common 
    test_protocol.cpp
    test_shm_sync.cpp
)

target_link_libraries(unit_tests_common 
    PRIVATE 
        wizz_common
        GTest::gtest_main
)

# Register with CTest
include(GoogleTest)
gtest_discover_tests(unit_tests_common)
