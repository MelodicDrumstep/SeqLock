#pragma once

#include <concepts>
#include <atomic>
#include <type_traits>
#include <array>
#include "util/helper_traits.hpp"

namespace lock
{

template <CopyableT T, int Cacheline_Size = 128> 
class SPSeqLock
{
public:
    SPSeqLock() : seq_(0) {}

    __attribute__((noinline)) T load() const noexcept
    {
        T ret;
        size_t seq0, seq1;
        do
        {
            seq0 = seq_.load(std::memory_order_acquire);
            std::atomic_signal_fence(std::memory_order_acq_rel);
            ret = value_;
            std::atomic_signal_fence(std::memory_order_acq_rel);
            seq1 = seq_.load(std::memory_order_relaxed); // TODO:
        } while ((seq0 != seq1) || (seq0 & 1));
        return ret;
    }

    __attribute__((noinline)) void store(const T & input) noexcept
    {
        size_t seq0 = seq_.load(std::memory_order_relaxed);
        seq_.store(seq0 + 1, std::memory_order_release);
        value_ = input;
        std::atomic_signal_fence(std::memory_order_acq_rel);
        seq_.store(seq0 + 2, std::memory_order_release);
    }

private:
    alignas(Cacheline_Size) T value_;
    std::atomic<size_t> seq_;
    std::array<uint8_t, Cacheline_Size - (sizeof(value_) + sizeof(seq_)) % Cacheline_Size> padding_;
};

template <CopyableT T, int Cacheline_Size = 128> 
class MPSeqLock
{
public:
    MPSeqLock() : seq_(0) {}

    __attribute__((noinline)) T load() const noexcept
    {
        T ret;
        size_t seq0, seq1;
        do
        {
            seq0 = seq_.load(std::memory_order_acquire);
            std::atomic_signal_fence(std::memory_order_acq_rel);
            ret = value_;
            std::atomic_signal_fence(std::memory_order_acq_rel);
            seq1 = seq_.load(std::memory_order_relaxed); // TODO:
        } while ((seq0 != seq1) || (seq0 & 1));
        return ret;
    }

    __attribute__((noinline)) void store(const T & input) noexcept
    {
        size_t seq0 = seq_.load(std::memory_order_relaxed);
        seq_.store(seq0 + 1, std::memory_order_release);
        while((seq0 & 1) || (!seq_.compare_exchange_weak(seq0, seq0 + 1))) {}
        value_ = input;
        seq_.store(seq0 + 2, std::memory_order_release);
    }

private:
    alignas(Cacheline_Size) T value_;
    std::atomic<size_t> seq_;
    std::array<uint8_t, Cacheline_Size - (sizeof(value_) + sizeof(seq_)) % Cacheline_Size> padding_;

};

}