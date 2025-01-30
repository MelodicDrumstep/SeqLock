#include <gtest/gtest.h>
#include <thread>
#include <atomic>

#include "seqlock.hpp"

namespace lock
{

// Test basic functionality of SPMCSeqLock
TEST(SeqLockTest, BasicFunctionality)
{
    SPMCSeqLock<int> seqlock;

    // Test store and load
    seqlock.store(42);
    EXPECT_EQ(seqlock.load(), 42);

    seqlock.store(100);
    EXPECT_EQ(seqlock.load(), 100);
}

// Test correctness in concurrent scenarios
TEST(SeqLockTest, ConcurrencyTest)
{
    SPMCSeqLock<int> seqlock;
    constexpr int kIterations = 10000;

    // Writer thread
    auto writer = [&seqlock]() {
        for (int i = 0; i < kIterations; ++i) {
            seqlock.store(i);
        }
    };

    // Reader thread
    auto reader = [&seqlock, kIterations]() {
        for (int i = 0; i < kIterations; ++i) {
            int value = seqlock.load();
            // Ensure the read value is valid (between 0 and kIterations-1)
            EXPECT_GE(value, 0);
            EXPECT_LT(value, kIterations);
        }
    };

    // Start reader and writer threads
    std::thread write_thread(writer);
    std::thread read_thread(reader);

    write_thread.join();
    read_thread.join();
}

// Test SPMCSeqLock's support for non-trivial types
TEST(SeqLockTest, NonTrivialTypeTest)
{
    struct NonTrivial
    {
        int a;
        double b;
        NonTrivial() : a(0), b(0.0) {}
        NonTrivial(int a, double b) : a(a), b(b) {}
        bool operator==(const NonTrivial &other) const
        {
            return a == other.a && b == other.b;
        }
    };

    SPMCSeqLock<NonTrivial> seqlock;

    // Test initial value
    EXPECT_EQ(seqlock.load(), NonTrivial{});

    // Test store and load
    NonTrivial value1{42, 3.14};
    seqlock.store(value1);
    EXPECT_EQ(seqlock.load(), value1);

    NonTrivial value2{100, 2.71};
    seqlock.store(value2);
    EXPECT_EQ(seqlock.load(), value2);
}

// Fuzz test for SPMCSeqLock
TEST(SeqLockFuzzTest, ConcurrentReadWrite) {
    struct Data {
        size_t a, b, c;
    };

    SPMCSeqLock<Data> sl;
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
                EXPECT_EQ(copy.a + 100, copy.b);
                EXPECT_EQ(copy.c, copy.a + copy.b);
            }
            ready--;
        });
    }

    // Writer thread
    size_t counter = 0;
    while (true) {
        Data data = {counter, counter + 100, counter + (counter + 100)};
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