project(WizzManiaCommon)

# Create the library (Static, as discussed)
# We will add source files here as we create them
add_library(wizz_common STATIC
    # Packet.cpp (To be created)
)

# Include directories for this library
target_include_directories(wizz_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# DEPENDENCIES
# Windows Sockets 2 (ws2_32)
if(WIN32)
    target_link_libraries(wizz_common PUBLIC ws2_32)
    message(STATUS "Windows detected: Linking against ws2_32")
endif()
