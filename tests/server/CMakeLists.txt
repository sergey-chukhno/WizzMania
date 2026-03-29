project(ServerTests)

# Link against common and server
# Note: Since server doesn't have a library yet, we might need to add the files directly
# or create a library in server/CMakeLists.txt. 
# Looking at the project, wizz_server is an executable.
# For testing parts of it, we should ideally have a library.

# Automated Suite (GTest)
add_executable(unit_tests_server 
    test_session_manager.cpp
    test_packet_router.cpp
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

# --- Legacy & Stress Tests ---
add_executable(stress_test stress_test.cpp)
target_link_libraries(stress_test PRIVATE 
    wizz_server_lib
)

add_executable(messaging_test messaging_test.cpp)
target_link_libraries(messaging_test PRIVATE wizz_common)

add_executable(offline_test offline_test.cpp)
target_link_libraries(offline_test PRIVATE wizz_common)

add_executable(contact_test contact_test.cpp)
target_link_libraries(contact_test PRIVATE wizz_common)

if(WIN32)
    target_link_libraries(stress_test PRIVATE ws2_32)
    target_link_libraries(messaging_test PRIVATE ws2_32)
    target_link_libraries(offline_test PRIVATE ws2_32)
    target_link_libraries(contact_test PRIVATE ws2_32)
endif()
