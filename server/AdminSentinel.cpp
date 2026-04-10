#include "AdminSentinel.h"
#include "TcpServer.h"
#include "CommandProcessor.h"
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <iomanip>
#include <sstream>

namespace wizz {

using namespace ftxui;

AdminSentinel::AdminSentinel(TcpServer* server) : m_server(server) {
    m_consoleHistory.push_back("[System] Sentinel TUI Initialized.");
}

void AdminSentinel::run() {
    auto screen = ScreenInteractive::Fullscreen();

    // Input field configuration
    Component input_field = Input(&m_currentInput, "Type command (e.g. /broadcast)...");

    // Wrap input field to handle 'Enter'
    auto input_handler = CatchEvent(input_field, [&](Event event) {
        if (event == Event::Return) {
            if (!m_currentInput.empty()) {
                processCommand(m_currentInput);
                m_currentInput.clear();
            }
            return true;
        }
        return false;
    });

    // The main layout loop
    auto renderer = Renderer(input_handler, [&] {
        return vbox({
            // Header
            hbox({
                text(" 🛡️ WIZZMANIA SERVER SENTINEL ") | bold | color(Color::Cyan) | border,
                filler(),
                text(" Uptime: " + std::to_string(static_cast<int>(MetricsManager::getInstance().getUptime())) + "s ") | border,
                text(" Status: [ACTIVE] ") | color(Color::Green) | border,
            }),

            // Dashboard Body
            hbox({
                renderMetrics() | flex,
                separator(),
                renderCommandConsole() | size(WIDTH, GREATER_THAN, 40),
            }) | border,

            // Input Area
            hbox({
                text(" > ") | bold | color(Color::Yellow),
                input_handler->Render() | flex,
            }) | border,
        });
    });

    // Background refresh logic (5 FPS)
    std::atomic<bool> refresh_ui(true);
    std::thread refresh_thread([&] {
        while (refresh_ui) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            screen.PostEvent(Event::Custom);
        }
    });

    screen.Loop(renderer);
    refresh_ui = false;
    if (refresh_thread.joinable()) refresh_thread.join();
}

Element AdminSentinel::renderMetrics() {
    auto& metrics = MetricsManager::getInstance();

    auto network_table = vbox({
        text("Networking STATS") | bold | color(Color::Blue),
        separator(),
        hbox({ text("Active Sessions: ") | size(WIDTH, EQUAL, 20), text(std::to_string(metrics.getValue(MetricType::ActiveSessions))) | color(Color::Green) }),
        hbox({ text("Total Connections: ") | size(WIDTH, EQUAL, 20), text(std::to_string(metrics.getValue(MetricType::ConnectionsTotal))) }),
        separator(),
        hbox({ text("Packets In: ") | size(WIDTH, EQUAL, 20), text(std::to_string(metrics.getValue(MetricType::PacketsIn))) | color(Color::Yellow) }),
        hbox({ text("Packets Out: ") | size(WIDTH, EQUAL, 20), text(std::to_string(metrics.getValue(MetricType::PacketsOut))) | color(Color::Yellow) }),
        hbox({ text("Traffic In: ") | size(WIDTH, EQUAL, 20), text(std::to_string(metrics.getValue(MetricType::BytesIn) / 1024) + " KB") }),
        hbox({ text("Traffic Out: ") | size(WIDTH, EQUAL, 20), text(std::to_string(metrics.getValue(MetricType::BytesOut) / 1024) + " KB") }),
    }) | border;

    auto security_table = vbox({
        text("Security SHIELDS") | bold | color(Color::Red),
        separator(),
        hbox({ text("Shield Drops: ") | size(WIDTH, EQUAL, 20), text(std::to_string(metrics.getValue(MetricType::ShieldDrops))) | bold | color(Color::Red) }),
        hbox({ text("Failed Logins: ") | size(WIDTH, EQUAL, 20), text(std::to_string(metrics.getValue(MetricType::FailedLogins))) | color(Color::Red) }),
    }) | border;

    return vbox({
        network_table,
        security_table,
    });
}

Element AdminSentinel::renderCommandConsole() {
    Elements history;
    // Show last 15 entries
    int start = std::max(0, static_cast<int>(m_consoleHistory.size()) - 15);
    for (size_t i = start; i < m_consoleHistory.size(); ++i) {
        history.push_back(text(m_consoleHistory[i]));
    }
    
    return vbox({
        text(" ADMIN CONSOLE ") | bold | center,
        separator(),
        vbox(std::move(history)) | flex,
    });
}

void AdminSentinel::processCommand(const std::string& command) {
    m_consoleHistory.push_back("[Admin] " + command);
    
    // Core Commands (State affectingSentinel/Server)
    if (command == "/exit" || command == "/shutdown") {
        m_consoleHistory.push_back("[System] Shutting down...");
        if (m_server) m_server->stop();
        return;
    } 
    
    if (command == "/clear") {
        m_consoleHistory.clear();
        m_consoleHistory.push_back("[System] Console cleared.");
        return;
    }

    // High-level Logic Commands
    CommandProcessor processor(m_server);
    std::string result = processor.execute(command);
    if (!result.empty()) {
        m_consoleHistory.push_back(result);
    }
}

} // namespace wizz
