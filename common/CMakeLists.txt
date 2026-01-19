project(WizzManiaCommon)

# Create the library 
add_library(wizz_common STATIC
    Packet.cpp
)

# Include directories 
target_include_directories(wizz_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# DEPENDENCIES
if(WIN32)
    target_link_libraries(wizz_common PUBLIC ws2_32)
endif()
