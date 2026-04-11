#pragma once

#include <chrono>
#include <mutex>
#include <algorithm>

namespace wizz {

/**
 * @class RateLimiter
 * @brief Thread-safe Token Bucket implementation for throttling requests.
 *
 * Allows for a "burst" of requests (capacity) while enforcing a 
 * steady-state refill rate (tokens per second).
 */
class RateLimiter {
public:
    RateLimiter(double capacity, double refillRate)
        : m_capacity(capacity),
          m_refillRate(refillRate),
          m_tokens(capacity),
          m_lastRefill(std::chrono::steady_clock::now()) {}

    /**
     * @brief Attempt to consume tokens.
     * @param amount Number of tokens to consume.
     * @return true if successful, false if rate limit exceeded.
     */
    bool consume(double amount = 1.0) {
        std::lock_guard<std::mutex> lock(m_mutex);
        refill();

        if (m_tokens >= amount) {
            m_tokens -= amount;
            return true;
        }
        return false;
    }

    /**
     * @brief Get current token count (for testing/monitoring).
     */
    double getTokens() {
        std::lock_guard<std::mutex> lock(m_mutex);
        refill();
        return m_tokens;
    }

private:
    void refill() {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - m_lastRefill);
        double seconds = duration.count() / 1000000.0;

        m_tokens = std::min(m_capacity, m_tokens + seconds * m_refillRate);
        m_lastRefill = now;
    }

private:
    double m_capacity;
    double m_refillRate;
    double m_tokens;
    std::chrono::steady_clock::time_point m_lastRefill;
    std::mutex m_mutex;
};

} // namespace wizz
