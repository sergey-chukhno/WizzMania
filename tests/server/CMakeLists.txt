project(ServerTests)

add_executable(stress_test
    stress_test.cpp
)

# Link against common to use Packet class
# Link against common to use Packet class
target_link_libraries(stress_test PRIVATE wizz_common)

# Messaging Integration Test (Day 4)
add_executable(messaging_test
    messaging_test.cpp
)
target_link_libraries(messaging_test PRIVATE wizz_common)

if(WIN32)
    target_link_libraries(stress_test PRIVATE ws2_32)
    target_link_libraries(messaging_test PRIVATE ws2_32)
endif()
