#include <gtest/gtest.h>
#include "../../server/security/RateLimiter.h"
#include <thread>
#include <vector>

using namespace wizz;

TEST(RateLimiterTest, BurstCapacity) {
    // 5 initial tokens, 1 token/sec refill
    RateLimiter limiter(5.0, 1.0);

    // Consume all 5
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(limiter.consume(1.0));
    }

    // 6th should fail
    EXPECT_FALSE(limiter.consume(1.0));
}

TEST(RateLimiterTest, RefillLogic) {
    // 1 token capacity, 10 tokens/sec refill
    RateLimiter limiter(1.0, 10.0);

    EXPECT_TRUE(limiter.consume(1.0));
    EXPECT_FALSE(limiter.consume(1.0));

    // Wait 200ms -> should refill 2 tokens, but capped at 1.0
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_TRUE(limiter.consume(1.0));
}

TEST(RateLimiterTest, ThreadSafety) {
    RateLimiter limiter(100.0, 0.0); // No refill

    auto worker = [&]() {
        for (int i = 0; i < 10; ++i) {
            limiter.consume(1.0);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back(worker);
    }

    for (auto& t : threads) {
        t.join();
    }

    // Should have consumed exactly 100
    EXPECT_FALSE(limiter.consume(1.0));
    EXPECT_NEAR(limiter.getTokens(), 0.0, 0.01);
}
