project(WizzManiaServer)

add_executable(wizz_server
    main.cpp
    TcpServer.cpp
)

# Link against the common library
target_link_libraries(wizz_server PRIVATE wizz_common)

# Windows Sockets
if(WIN32)
    target_link_libraries(wizz_server PRIVATE ws2_32)
endif()
