#pragma once

#include <concepts>
#include <atomic>
#include <type_traits>
#include <array>
#include "util/helper_traits.hpp"
#include "util/helper_data_types.hpp"

namespace lockfree
{

// Assume the data to be protected is:
// struct Data
// {
//     int data1, data2;
// };

template <int Cacheline_Size = 64>
class SPSeqLockV1
{
public:
    SPSeqLockV1() : seq_(0) {}

    __attribute__((noinline)) Data load() const noexcept
    {
        Data ret;
        size_t seq0, seq1;
        do
        {
            seq0 = seq_.load(std::memory_order_acquire);
            ret.data1 = data1_;
            ret.data2 = data2_;
            seq1 = seq_.load(std::memory_order_acquire);
        } while ((seq0 != seq1) || (seq0 & 1));
        return ret;
    }

    __attribute__((noinline)) void store(const Data & input) noexcept
    {
        size_t seq0 = seq_.load(std::memory_order_relaxed); 
        seq_.store(seq0 + 1, std::memory_order_release);
        data1_ = input.data1;
        data2_ = input.data2;
        seq_.store(seq0 + 2, std::memory_order_release);
    }

private:
    alignas(Cacheline_Size) int data1_;
    int data2_;
    std::atomic<size_t> seq_;
    std::array<uint8_t, Cacheline_Size - (sizeof(data1_) + sizeof(data2_) + sizeof(seq_)) % Cacheline_Size> padding_;
};

}