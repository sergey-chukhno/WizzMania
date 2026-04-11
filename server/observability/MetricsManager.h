#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <string>

namespace wizz {

enum class MetricType {
    ConnectionsTotal,
    ActiveSessions,
    PacketsIn,
    PacketsOut,
    BytesIn,
    BytesOut,
    ShieldDrops,
    FailedLogins,
    DbTasksPending
};

class MetricsManager {
public:
    static MetricsManager& getInstance() {
        static MetricsManager instance;
        return instance;
    }

    // Lock-free increment
    void increment(MetricType type, uint64_t amount = 1) {
        m_counters[type].fetch_add(amount, std::memory_order_relaxed);
    }

    // Lock-free decrement (e.g., for active sessions)
    void decrement(MetricType type, uint64_t amount = 1) {
        m_counters[type].fetch_sub(amount, std::memory_order_relaxed);
    }

    // Get current value
    uint64_t getValue(MetricType type) const {
        auto it = m_counters.find(type);
        if (it != m_counters.end()) {
            return it->second.load(std::memory_order_relaxed);
        }
        return 0;
    }

    // Get server uptime in seconds
    double getUptime() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - m_startTime);
        return static_cast<double>(duration.count());
    }

private:
    MetricsManager() : m_startTime(std::chrono::steady_clock::now()) {
        // Initialize all metrics to zero
        m_counters[MetricType::ConnectionsTotal].store(0);
        m_counters[MetricType::ActiveSessions].store(0);
        m_counters[MetricType::PacketsIn].store(0);
        m_counters[MetricType::PacketsOut].store(0);
        m_counters[MetricType::BytesIn].store(0);
        m_counters[MetricType::BytesOut].store(0);
        m_counters[MetricType::ShieldDrops].store(0);
        m_counters[MetricType::FailedLogins].store(0);
        m_counters[MetricType::DbTasksPending].store(0);
    }

    std::map<MetricType, std::atomic<uint64_t>> m_counters;
    std::chrono::steady_clock::time_point m_startTime;
};

} // namespace wizz
