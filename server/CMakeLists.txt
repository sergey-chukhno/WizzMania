project(WizzManiaServer)

add_executable(wizz_server
    main.cpp
    TcpServer.cpp
    ClientSession.cpp
    DatabaseManager.cpp
)

# Link against the common library
target_link_libraries(wizz_server PRIVATE wizz_common)
# OpenSSL
find_package(OpenSSL REQUIRED)
target_link_libraries(wizz_server PRIVATE OpenSSL::SSL OpenSSL::Crypto)

# Asio (Standalone Header-Only via FetchContent to ensure zero local dependencies)
include(FetchContent)
FetchContent_Declare(
    asio
    URL "https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-30-2.tar.gz"
)
FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
    message(STATUS "Fetching Standalone Asio 1.30.2...")
    FetchContent_Populate(asio)
endif()

# Bind headers and define Standalone mode so Asio doesn't look for Boost
target_include_directories(wizz_server PRIVATE ${asio_SOURCE_DIR}/asio/include)
target_compile_definitions(wizz_server PRIVATE ASIO_STANDALONE ASIO_HAS_OPENSSL=1)

# SQLite3
find_package(SQLite3 QUIET)
if(SQLite3_FOUND)
    message(STATUS "SQLite3 found locally.")
    target_include_directories(wizz_server PRIVATE ${SQLite3_INCLUDE_DIRS})
    target_link_libraries(wizz_server PRIVATE ${SQLite3_LIBRARIES})
else()
    message(STATUS "SQLite3 not found natively. Fetching amalgamation for Windows compatibility...")
    include(FetchContent)
    FetchContent_Declare(
        sqlite3_src
        URL "https://sqlite.org/2024/sqlite-amalgamation-3450100.zip"
    )
    FetchContent_GetProperties(sqlite3_src)
    if(NOT sqlite3_src_POPULATED)
        FetchContent_Populate(sqlite3_src)
    endif()
    
    target_include_directories(wizz_server PRIVATE ${sqlite3_src_SOURCE_DIR})
    target_sources(wizz_server PRIVATE ${sqlite3_src_SOURCE_DIR}/sqlite3.c)
    
    find_package(Threads REQUIRED)
    target_link_libraries(wizz_server PRIVATE Threads::Threads)
    if(UNIX)
        target_link_libraries(wizz_server PRIVATE dl)
    endif()
endif()

# Windows Sockets
if(WIN32)
    target_link_libraries(wizz_server PRIVATE ws2_32)
endif()
