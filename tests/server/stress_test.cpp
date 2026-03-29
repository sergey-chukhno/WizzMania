#include <asio.hpp>
#include <asio/ssl.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include "../../common/Packet.h"

using asio::ip::tcp;
using namespace wizz;

std::atomic<int> g_successCount{0};
std::atomic<int> g_failureCount{0};

void run_stress_client(int id) {
    asio::io_context io_context;
    asio::ssl::context ssl_context(asio::ssl::context::tlsv12);
    ssl_context.set_verify_mode(asio::ssl::verify_none);

    asio::ssl::stream<tcp::socket> socket(io_context, ssl_context);
    tcp::resolver resolver(io_context);

    try {
        asio::connect(socket.lowest_layer(), resolver.resolve("127.0.0.1", "8080"));
        socket.handshake(asio::ssl::stream_base::client);

        // 1. Register a unique user
        std::string username = "StressUser_" + std::to_string(id) + "_" + std::to_string(std::rand() % 1000);
        Packet reg(PacketType::Register);
        reg.writeString(username);
        reg.writeString("password123");

        asio::write(socket, asio::buffer(reg.serialize()));

        // 2. Wait for response
        uint8_t headerBuf[12];
        asio::read(socket, asio::buffer(headerBuf, 12));
        
        // Protocol: 4 bytes Magic, 4 bytes Type (Network Order), 4 bytes Length (Network Order)
        uint32_t typeNetwork;
        std::memcpy(&typeNetwork, &headerBuf[4], 4);
        uint32_t type = ntohl(typeNetwork);

        if (type == (uint32_t)PacketType::RegisterSuccess) {
            g_successCount++;
        } else {
            g_failureCount++;
            std::cerr << "[Client " << id << "] Wrong Response Type: " << type << std::endl;
        }

    } catch (const std::exception& e) {
        g_failureCount++;
    }
}

int main(int argc, char* argv[]) {
    int numClients = 20;
    if (argc > 1) numClients = std::stoi(argv[1]);

    std::cout << "=== WizzMania Stress Test: " << numClients << " Concurrent TLS Clients ===" << std::endl;
    
    std::vector<std::thread> clients;
    for (int i = 0; i < numClients; ++i) {
        clients.emplace_back(run_stress_client, i);
    }

    for (auto& t : clients) {
        t.join();
    }

    std::cout << "\n--- Load Results ---" << std::endl;
    std::cout << "Success: " << g_successCount.load() << std::endl;
    std::cout << "Failure: " << g_failureCount.load() << std::endl;

    if (g_failureCount == 0 && g_successCount > 0) {
        std::cout << "PASSED: Server handled concurrency perfectly." << std::endl;
        return 0;
    } else {
        std::cout << "FAILED: Some clients failed to connect or register." << std::endl;
        return 1;
    }
}
