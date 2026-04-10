#pragma once

#include <string>
#include <vector>
#include <memory>
#include "MetricsManager.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

namespace wizz {

class TcpServer;

class AdminSentinel {
public:
    explicit AdminSentinel(TcpServer* server);
    ~AdminSentinel() = default;

    // Starts the interactive TUI loop (blocks current thread)
    void run();

private:
    // UI Construction components
    ftxui::Element renderDashboard();
    ftxui::Element renderMetrics();
    ftxui::Element renderCommandConsole();

    void processCommand(const std::string& command);

private:
    TcpServer* m_server;
    std::vector<std::string> m_consoleHistory;
    std::string m_currentInput;
    
    // FTXUI Components
    ftxui::Component m_inputField;
    ftxui::Component m_layout;
};

} // namespace wizz
