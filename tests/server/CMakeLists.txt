project(ServerTests)

add_executable(stress_test
    stress_test.cpp
)

# No dependency on `wizz_common` needed yet for simple connection test, 
# but good to link if we add Packet logic later.
if(WIN32)
    target_link_libraries(stress_test ws2_32)
endif()
