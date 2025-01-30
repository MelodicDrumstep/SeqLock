#pragma once

#include <concepts>
#include <atomic>
#include <type_traits>
#include <array>
#include "util/helper_traits.hpp"
#include "util/helper_data_types.hpp"

namespace lock
{

// Assume the data to be protected is:
// struct Data
// {
//     int data1, data2;
// };

template <int Cacheline_Size = 128> 
class SPSeqLockV2
{
public:
    SPSeqLockV2() : seq_(0) {}

    __attribute__((noinline)) Data load() const noexcept
    {
        Data ret;
        size_t seq0, seq1;
        do
        {
            seq0 = seq_;        // std::memory_order_seq_cst
            ret.data1 = data1_.load(std::memory_order_relaxed); 
            ret.data2 = data2_.load(std::memory_order_relaxed); 
            seq1 = seq_;        // std::memory_order_seq_cst
        } while ((seq0 != seq1) || (seq0 & 1));
        return ret;
    }

    __attribute__((noinline)) void store(const Data & input) noexcept
    {
        size_t seq0 = seq_;     // std::memory_order_seq_cst
        seq_ = seq0 + 1;        // std::memory_order_seq_cst
        data1_ = input.data1;   // std::memory_order_seq_cst
        data2_ = input.data2;   // std::memory_order_seq_cst
        seq_ = seq0 + 2;        // std::memory_order_seq_cst
    }

private:
    alignas(Cacheline_Size) std::atomic<int> data1_;
    std::atomic<int> data2_;
    std::atomic<size_t> seq_;
    std::array<uint8_t, Cacheline_Size - (sizeof(data1_) + sizeof(data2_) + sizeof(seq_)) % Cacheline_Size> padding_;
};

}