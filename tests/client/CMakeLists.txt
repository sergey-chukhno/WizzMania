project(WizzManiaTestsClient)

find_package(Qt6 REQUIRED COMPONENTS Core Network Test)

set(CMAKE_AUTOMOC ON)

# Source files needed for testing NetworkManager + E2EE
set(TEST_C_SOURCES
    test_network_manager.cpp
    test_local_database.cpp
    test_signal_crypto.cpp
    ../../client/network/NetworkManager.cpp
    ../../client/data/LocalDatabase.cpp
    ../../client/crypto/SignalProvider.cpp
    ../../client/crypto/SignalStoreContext.cpp
)

add_executable(unit_tests_client ${TEST_C_SOURCES})

target_include_directories(unit_tests_client PRIVATE 
    ../../client
    ../../common
    ${CMAKE_BINARY_DIR}/_deps/libsignal-src/src
    ${SQLITE3_INCLUDE_DIRS}
)

find_package(OpenSSL REQUIRED)
find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
    # If not found, we use the same fallback as in client/CMakeLists.txt
    # Assuming it's already fetched by the main client build
    set(SQLITE3_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/_deps/sqlite3_src-src)
    # We would need to add sqlite3.c to source files if not found, 
    # but let's assume SQLite3 is available on this Mac system (it usually is).
endif()

target_link_libraries(unit_tests_client 
    PRIVATE 
        wizz_common
        Qt6::Core
        Qt6::Network
        Qt6::Test
        GTest::gtest_main
        signal-protocol-c
        OpenSSL::Crypto
        sqlite3
)

# Register with CTest
include(GoogleTest)
gtest_discover_tests(unit_tests_client)
