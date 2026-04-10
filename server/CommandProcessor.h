#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <iostream>

namespace wizz {

class TcpServer;

class CommandProcessor {
public:
    explicit CommandProcessor(TcpServer* server) : m_server(server) {}

    std::string execute(const std::string& input) {
        if (input.empty()) return "";

        // Simple parser
        std::vector<std::string> args = split(input, ' ');
        if (args.empty()) return "";

        std::string cmd = args[0];
        
        if (cmd == "/help") {
            return "Available: /broadcast <msg>, /kick <id>, /stats, /clear, /shutdown";
        }
        
        if (cmd == "/stats") {
            return "Stats are displayed in the live dashboard boxes above.";
        }

        if (cmd == "/broadcast") {
            if (args.size() < 2) return "Usage: /broadcast <message>";
            std::string message;
            for (size_t i = 1; i < args.size(); ++i) {
                message += args[i] + (i == args.size() - 1 ? "" : " ");
            }
            if (m_server) {
                m_server->broadcastMessage("SYSTEM", message);
            }
            return "Broadcast sent to all active users.";
        }

        if (cmd == "/kick") {
            if (args.size() < 2) return "Usage: /kick <session_id>";
            try {
                int id = std::stoi(args[1]);
                if (m_server) {
                    // m_server->handleDisconnect(id); // Placeholder for future wiring
                }
                return "Kicking session " + std::to_string(id) + "...";
            } catch (...) {
                return "Error: Invalid session ID.";
            }
        }

        return "Unknown command: " + cmd + ". Type /help for options.";
    }

private:
    std::vector<std::string> split(const std::string& s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)) {
            if (!token.empty()) tokens.push_back(token);
        }
        return tokens;
    }

    TcpServer* m_server;
};

} // namespace wizz
