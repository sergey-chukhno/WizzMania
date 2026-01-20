project(WizzManiaServer)

add_executable(wizz_server
    main.cpp
    TcpServer.cpp
    ClientSession.cpp
    DatabaseManager.cpp
)

# Link against the common library
target_link_libraries(wizz_server PRIVATE wizz_common)

# SQLite3
find_package(SQLite3 REQUIRED)
target_include_directories(wizz_server PRIVATE ${SQLite3_INCLUDE_DIRS})
target_link_libraries(wizz_server PRIVATE ${SQLite3_LIBRARIES})

# Windows Sockets
if(WIN32)
    target_link_libraries(wizz_server PRIVATE ws2_32)
endif()
