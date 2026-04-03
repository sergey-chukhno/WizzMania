#include <gtest/gtest.h>
#include "../../common/NativeSharedMemory.h"
#include <thread>
#include <chrono>

namespace wizz {

// Custom struct to test template flexibility and versioning
struct TestData {
    uint32_t dataVersion;
    int value;
};

class SharedMemorySyncTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure we start with a clean segment
        NativeSharedMemory<TestData> shm("test_sync_segment");
        shm.unlink();
    }

    void TearDown() override {
        NativeSharedMemory<TestData> shm("test_sync_segment");
        shm.unlink();
    }
};

/**
 * @brief Verifies that dataVersion increments are correctly detected.
 */
TEST_F(SharedMemorySyncTest, VersionDetection) {
    NativeSharedMemory<TestData> writer("test_sync_segment");
    ASSERT_TRUE(writer.createAndMap());

    NativeSharedMemory<TestData> reader("test_sync_segment");
    ASSERT_TRUE(reader.openAndMap());

    uint32_t lastVersion = 0;
    
    // 1. Initial State: No update
    EXPECT_FALSE(reader.hasUpdate(lastVersion));
    EXPECT_EQ(lastVersion, 0);

    // 2. Update Version
    writer.lock();
    writer.data()->dataVersion = 5;
    writer.data()->value = 100;
    writer.unlock();

    // 3. Detection: hasUpdate should return TRUE and update lastVersion
    EXPECT_TRUE(reader.hasUpdate(lastVersion));
    EXPECT_EQ(lastVersion, 5);
    EXPECT_EQ(reader.data()->value, 100);

    // 4. Repeated check: Should return FALSE (idempotency)
    EXPECT_FALSE(reader.hasUpdate(lastVersion));
    EXPECT_EQ(lastVersion, 5);
}

/**
 * @brief Simulates a Producer-Consumer scenario with threads.
 */
TEST_F(SharedMemorySyncTest, ThreadedUpdateDetection) {
    auto producer = std::thread([]() {
        NativeSharedMemory<TestData> writer("test_threaded_sync");
        writer.unlink();
        if (!writer.createAndMap()) return;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        for (int i = 1; i <= 10; ++i) {
            writer.lock();
            writer.data()->dataVersion = i;
            writer.data()->value = i * 10;
            writer.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    });

    NativeSharedMemory<TestData> reader("test_threaded_sync");
    // Wait for segment to be created by producer
    int retries = 0;
    while (!reader.openAndMap() && retries < 10) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        retries++;
    }
    ASSERT_TRUE(reader.openAndMap());

    uint32_t lastVersion = 0;
    int updatesDetected = 0;
    int maxVal = 0;

    // Poll for 500ms
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(1000)) {
        if (reader.hasUpdate(lastVersion)) {
            updatesDetected++;
            maxVal = std::max(maxVal, reader.data()->value);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (producer.joinable()) producer.join();
    
    // We should have detected multiple updates. 
    // Note: In high-speed scenarios, we might miss some intermediate versions, 
    // but the last version MUST be 10.
    EXPECT_GT(updatesDetected, 0);
    EXPECT_EQ(lastVersion, 10);
    EXPECT_EQ(maxVal, 100);

    reader.unlink();
}

} // namespace wizz
