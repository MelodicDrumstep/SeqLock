#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include "seqlock_versions/seqlock_v0_wrong.hpp"
#include "util/helper_data_types.hpp"

namespace lock
{

// Test basic functionality of SPMCSeqLockV0
TEST(SeqLockTest, BasicFunctionality)
{
    SPMCSeqLockV0<> seqlock;

    // Test store and load
    Data data1 = {42, 42};
    seqlock.store(data1);
    Data loaded_data1 = seqlock.load();
    EXPECT_EQ(loaded_data1.data1, 42);
    EXPECT_EQ(loaded_data1.data2, 42);

    Data data2 = {100, 100};
    seqlock.store(data2);
    Data loaded_data2 = seqlock.load();
    EXPECT_EQ(loaded_data2.data1, 100);
    EXPECT_EQ(loaded_data2.data2, 100);
}

// Test correctness in concurrent scenarios
TEST(SeqLockTest, ConcurrencyTest)
{
    SPMCSeqLockV0<> seqlock;
    constexpr int kIterations = 10000;

    // Writer thread
    auto writer = [&seqlock]() {
        for (int i = 0; i < kIterations; ++i) {
            Data data = {i, i};
            seqlock.store(data);
        }
    };

    // Reader thread
    auto reader = [&seqlock, kIterations]() {
        for (int i = 0; i < kIterations; ++i) {
            Data value = seqlock.load();
            // Ensure the read value is valid (between 0 and kIterations-1)
            EXPECT_GE(value.data1, 0);
            EXPECT_LT(value.data1, kIterations);
            EXPECT_EQ(value.data1, value.data2); // Ensure data1 and data2 are equal
        }
    };

    // Start reader and writer threads
    std::thread write_thread(writer);
    std::thread read_thread(reader);

    write_thread.join();
    read_thread.join();
}

// Fuzz test for SPMCSeqLockV0
TEST(SeqLockFuzzTest, ConcurrentReadWrite) {
    SPMCSeqLockV0<> sl;
    std::atomic<size_t> ready(0);
    std::vector<std::thread> threads;

    constexpr size_t kNumThreads = 10;
    constexpr size_t kIterations = 1000000;

    // Reader threads
    for (int i = 0; i < kNumThreads; ++i) {
        threads.emplace_back([&sl, &ready]() {
            while (ready == 0) {
                // Wait for the writer thread to start
            }
            for (int j = 0; j < kIterations; ++j) {
                auto copy = sl.load();
                // Ensure the data is consistent
                EXPECT_EQ(copy.data1, copy.data2); // Ensure data1 and data2 are equal
            }
            ready--;
        });
    }

    // Writer thread
    size_t counter = 0;
    while (true) {
        Data data = {static_cast<int>(counter), static_cast<int>(counter)}; // data1 = data2 = counter
        sl.store(data);
        counter++;

        if (counter == 1) {
            // Signal reader threads to start
            ready += kNumThreads;
        }

        if (ready == 0) {
            // All reader threads are done
            break;
        }
    }

    // Join all threads
    for (auto &t : threads) {
        t.join();
    }

    std::cout << "Fuzz test completed with counter = " << counter << std::endl;
}

} // namespace lock