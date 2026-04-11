#include "AdminSentinel.h"
#include "../core/TcpServer.h"
#include "MetricsManager.h"
#include "CommandProcessor.h"
#include "../data/DatabaseManager.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <ctime>
#include <iomanip>

namespace wizz {

AdminSentinel::AdminSentinel(TcpServer* server) : m_server(server) {}

AdminSentinel::~AdminSentinel() {
    stop();
}

void AdminSentinel::start() {
    if (m_running.exchange(true)) return;
    std::thread(&AdminSentinel::runTuiLoop, this).detach();
}

void AdminSentinel::stop() {
    m_running = false;
}

void AdminSentinel::logEvent(const std::string& component, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_eventMutex);
    
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%H:%M:%S");
    
    m_events.push_back({component, message, ss.str()});
    if (m_events.size() > 50) m_events.erase(m_events.begin());
}

void AdminSentinel::runTuiLoop() {
    while (m_running) {
        renderDashboard();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void AdminSentinel::renderDashboard() {
    // Clear screen (POSIX)
    std::cout << "\033[2J\033[1;1H";
    
    std::cout << "================================================================" << std::endl;
    std::cout << "   WIZZ MANIA - ADMIN SENTINEL - PROTOCOL SECURITY DASHBOARD    " << std::endl;
    std::cout << "================================================================" << std::endl;
    
    if (m_server) {
        std::cout << " Server Port: " << 8080 << " | Status: RUNNING" << std::endl;
        std::cout << " Active Sessions: " << m_server->getSessionManager().getActiveSessionCount() << std::endl;
    }
    
    std::cout << "----------------------------------------------------------------" << std::endl;
    std::cout << " RECENT EVENTS:" << std::endl;
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        for (const auto& ev : m_events) {
            std::cout << " [" << ev.timestamp << "] [" << ev.component << "] " << ev.message << std::endl;
        }
    }
    
    std::cout << "----------------------------------------------------------------" << std::endl;
    std::cout << " Sentinel Command Interface > ";
    std::cout.flush();
}

} // namespace wizz
