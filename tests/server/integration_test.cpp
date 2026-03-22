#include <gtest/gtest.h>
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <thread>
#include <chrono>
#include "../../server/TcpServer.h"
#include "../../common/Packet.h"

using asio::ip::tcp;
using namespace wizz;

class ServerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Remove old test db if exists
        std::remove("wizz_test.db");

        m_serverThread = std::thread([this]() {
            try {
                // Use a dedicated test port (8100) and database
                m_server = std::make_unique<TcpServer>(8100, "wizz_test.db");
                m_server->start();
            } catch (const std::exception& e) {
                std::cerr << "[IntegrationTest] Server Startup Error: " << e.what() << std::endl;
            }
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    void TearDown() override {
        if (m_server) m_server->stop();
        if (m_serverThread.joinable()) m_serverThread.join();
        std::remove("wizz_test.db"); // Cleanup after test
    }

    std::thread m_serverThread;
    std::unique_ptr<TcpServer> m_server;
};

TEST_F(ServerIntegrationTest, RegistrationFlow) {
    asio::io_context io_context;
    asio::ssl::context ssl_context(asio::ssl::context::tlsv12);
    ssl_context.set_verify_mode(asio::ssl::verify_none);

    asio::ssl::stream<tcp::socket> socket(io_context, ssl_context);
    tcp::resolver resolver(io_context);
    
    try {
        asio::connect(socket.lowest_layer(), resolver.resolve("127.0.0.1", "8100"));
        socket.handshake(asio::ssl::stream_base::client);
    } catch (const std::exception& e) {
        // If we fail here, the server probably didn't start (port conflict)
        FAIL() << "Connection/Handshake failed: " << e.what();
    }

    // 1. Send Registration Packet
    Packet reg(PacketType::Register);
    reg.writeString("testuser_int_" + std::to_string(std::rand()));
    reg.writeString("password123");
    
    asio::write(socket, asio::buffer(reg.serialize()));

    // 2. Receive Response Header
    uint8_t headerBuf[12];
    asio::read(socket, asio::buffer(headerBuf, 12));
    
    // Use ntohl for type and length as per protocol
    uint32_t typeNetwork, lengthNetwork;
    std::memcpy(&typeNetwork, &headerBuf[4], 4);
    std::memcpy(&lengthNetwork, &headerBuf[8], 4);
    
    uint32_t type = ntohl(typeNetwork);
    uint32_t length = ntohl(lengthNetwork);
    
    std::cout << "[IntegrationTest] Received Packet Type: " << type << " Length: " << length << std::endl;

    // Successful registration/login response types are normally 100-105
    EXPECT_GE(type, 100);
    EXPECT_LE(type, 105);
    
    if (length > 0) {
        std::vector<uint8_t> body(length);
        asio::read(socket, asio::buffer(body));
        // Optionally parse body with Packet class if needed
    }
}
