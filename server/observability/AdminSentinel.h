#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <atomic>

namespace wizz {

class TcpServer;

class AdminSentinel {
public:
    explicit AdminSentinel(TcpServer* server);
    ~AdminSentinel();

    void start();
    void run();
    void stop();

    // API for other components to push log/event data to the dashboard
    void logEvent(const std::string& component, const std::string& message);
    void updateMetric(const std::string& name, double value);

private:
    void runTuiLoop();
    void renderDashboard();

    TcpServer* m_server;
    std::atomic<bool> m_running{false};
    
    struct Event {
        std::string component;
        std::string message;
        std::string timestamp;
    };
    
    std::vector<Event> m_events;
    std::mutex m_eventMutex;
};

} // namespace wizz
