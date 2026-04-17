#include "AdminSentinel.h"
#include "../core/TcpServer.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace wizz {

AdminSentinel::AdminSentinel(TcpServer* server) : m_server(server) {}

AdminSentinel::~AdminSentinel() {
    stop();
}

void AdminSentinel::start() {
    if (m_running.exchange(true)) return;
    // TUI loop disabled for protocol debugging to avoid log spam
}

void AdminSentinel::run() {
    start();
    // Block the main thread to keep the server alive
    while (m_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
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

void AdminSentinel::updateMetric(const std::string& name, double value) {
    logEvent("METRIC", name + ": " + std::to_string(value));
}

void AdminSentinel::runTuiLoop() {
    // Disabled
}

void AdminSentinel::renderDashboard() {
    // Disabled
}

} // namespace wizz
