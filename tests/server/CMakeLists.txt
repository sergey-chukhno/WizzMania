project(ServerTests)

add_executable(stress_test
    stress_test.cpp
)

# Link against common to use Packet class
target_link_libraries(stress_test PRIVATE wizz_common)

if(WIN32)
    target_link_libraries(stress_test PRIVATE ws2_32)
endif()
