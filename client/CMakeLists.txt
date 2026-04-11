
# Wizz Mania Client
# Requires Qt 6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Multimedia)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Client Sources
set(CLIENT_SOURCES
    main.cpp
    NetworkManager.cpp
    AudioManager.cpp
    LoginDialog.cpp
    RegisterDialog.cpp
    AuthWindow.cpp
    AddFriendDialog.cpp
    ChatWindow.cpp
    MainWindow.cpp
    GameLauncher.cpp
    GameBridge.cpp
    AvatarManager.cpp
    data/LocalDatabase.cpp
    crypto/SignalProvider.cpp
    crypto/SignalStoreContext.cpp
    resources.qrc
)

# Enable AutoRcc for resource processing
set(CMAKE_AUTORCC ON)

# --- End-to-End Encryption Dependencies ---

# 1. OpenSSL (For Crypto Provider Hooks)
find_package(OpenSSL REQUIRED)

# 2. SQLite3 (For Local E2EE Storage)
find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
    message(STATUS "Client: SQLite3 not found natively. Fetching amalgamation...")
    include(FetchContent)
    FetchContent_Declare(
        sqlite3_src
        URL "https://sqlite.org/2024/sqlite-amalgamation-3450100.zip"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    if(NOT sqlite3_src_POPULATED)
        FetchContent_Populate(sqlite3_src)
    endif()
    set(SQLITE3_INCLUDE_DIRS ${sqlite3_src_SOURCE_DIR})
    list(APPEND CLIENT_SOURCES ${sqlite3_src_SOURCE_DIR}/sqlite3.c)
endif()

# 3. LibSignal-Protocol-C 
include(FetchContent)
message(STATUS "Fetching libsignal-protocol-c (v2.3.3)...")
FetchContent_Declare(
    libsignal
    GIT_REPOSITORY https://github.com/signalapp/libsignal-protocol-c.git
    GIT_TAG        v2.3.3
)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libsignal)

# Add Executable
add_executable(wizz_client ${CLIENT_SOURCES})

# Link Libraries
target_include_directories(wizz_client PRIVATE 
    ${SQLITE3_INCLUDE_DIRS}
    ${libsignal_SOURCE_DIR}/src
)
target_link_libraries(wizz_client
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
    wizz_common # Link our Packet library
    OpenSSL::SSL 
    OpenSSL::Crypto
    signal-protocol-c
)

if(SQLite3_FOUND)
    target_link_libraries(wizz_client PRIVATE ${SQLite3_LIBRARIES})
endif()
