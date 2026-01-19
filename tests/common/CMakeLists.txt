project(WizzManiaTestsCommon)

add_executable(unit_tests_common test_packet.cpp)

# Link against the common library
target_link_libraries(unit_tests_common PRIVATE wizz_common)

# Register with CTest
add_test(NAME CommonPacketTest COMMAND unit_tests_common)
