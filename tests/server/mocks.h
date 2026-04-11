#pragma once
#include <gmock/gmock.h>
#include "../../server/core/ClientSession.h"
#include "../../server/core/TcpServer.h"
#include "../../common/Packet.h"
#include <asio.hpp>
#include <asio/ssl.hpp>

namespace wizz {

// A specialized mock that can be used across all server tests
class MockClientSession : public ClientSession {
public:
    // We satisfy the constructor with real but idle Asio objects
    MockClientSession(int id, asio::io_context& io, asio::ssl::context& ssl, TcpServer* server) 
        : ClientSession(id, asio::ip::tcp::socket(io), ssl, server) {}
    
    virtual ~MockClientSession() = default;

    MOCK_METHOD(void, sendPacket, (const Packet& packet), (override));
    MOCK_METHOD(bool, isLoggedIn, (), (const, override));
    MOCK_METHOD(std::string, getUsername, (), (const, override));
    MOCK_METHOD(void, setLoggedIn, (bool b), (override));
};

// A TcpServer that doesn't try to bind to real ports if not needed
class TestableTcpServer : public TcpServer {
public:
    TestableTcpServer(int port = 0, const std::string& dbPath = "test_wizz.db") 
        : TcpServer(port, dbPath) {}
};

} // namespace wizz
