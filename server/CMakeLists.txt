cmake_minimum_required(VERSION 3.10)
project(WizzManiaServer)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the Server Logic Library (to be shared with tests)
add_library(wizz_server_lib STATIC
    core/TcpServer.cpp
    core/ClientSession.cpp
    data/DatabaseManager.cpp
    data/AuthDAO.cpp
    data/SocialDAO.cpp
    data/CryptoDAO.cpp
    logic/SessionManager.cpp
    logic/GameRoomManager.cpp
    logic/handlers/PacketRouter.cpp
    logic/handlers/AuthHandlers.cpp
    logic/handlers/SocialHandlers.cpp
    logic/handlers/GameHandlers.cpp
    logic/handlers/CryptoHandlers.cpp
    observability/AdminSentinel.cpp
)

# Target include directories for the server library
target_include_directories(wizz_server_lib PUBLIC 
    core
    logic
    logic/handlers
    data
    observability
    security
)

# Link against the common library
target_link_libraries(wizz_server_lib PUBLIC wizz_common)

# OpenSSL
find_package(OpenSSL REQUIRED)
target_link_libraries(wizz_server_lib PUBLIC 
    OpenSSL::SSL 
    OpenSSL::Crypto
    ftxui::screen
    ftxui::dom
    ftxui::component
)

# Asio (Standalone Header-Only via FetchContent)
include(FetchContent)
FetchContent_Declare(
    asio
    URL "https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-30-2.tar.gz"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
    message(STATUS "Fetching Standalone Asio 1.30.2...")
    FetchContent_Populate(asio)
endif()

# Bind headers and define Standalone mode
target_include_directories(wizz_server_lib PUBLIC ${asio_SOURCE_DIR}/asio/include)
target_compile_definitions(wizz_server_lib PUBLIC ASIO_STANDALONE ASIO_HAS_OPENSSL=1)

# SQLite3
find_package(SQLite3 QUIET)
if(SQLite3_FOUND)
    message(STATUS "SQLite3 found locally.")
    target_include_directories(wizz_server_lib PUBLIC ${SQLite3_INCLUDE_DIRS})
    target_link_libraries(wizz_server_lib PUBLIC ${SQLite3_LIBRARIES})
else()
    message(STATUS "SQLite3 not found natively. Fetching amalgamation...")
    include(FetchContent)
    FetchContent_Declare(
        sqlite3_src
        URL "https://sqlite.org/2024/sqlite-amalgamation-3450100.zip"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_GetProperties(sqlite3_src)
    if(NOT sqlite3_src_POPULATED)
        FetchContent_Populate(sqlite3_src)
    endif()
    
    target_include_directories(wizz_server_lib PUBLIC ${sqlite3_src_SOURCE_DIR})
    target_sources(wizz_server_lib PRIVATE ${sqlite3_src_SOURCE_DIR}/sqlite3.c)
    
    find_package(Threads REQUIRED)
    target_link_libraries(wizz_server_lib PUBLIC Threads::Threads)
    if(UNIX)
        target_link_libraries(wizz_server_lib PUBLIC dl)
    endif()
endif()

# Windows Sockets
if(WIN32)
    target_link_libraries(wizz_server_lib PUBLIC ws2_32)
endif()

# Main Executable
add_executable(wizz_server core/main.cpp)
target_link_libraries(wizz_server PRIVATE wizz_server_lib)
