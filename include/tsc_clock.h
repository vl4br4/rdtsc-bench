#pragma once

#include "tsc_cpu.h"
#include "utils/compiler.h"
#include "utils/types.h"

namespace benchmarking {

    /// Memory barrier types for TSC measurements
    /// Different barriers provide different levels of instruction ordering
    enum class Barrier {
        kOneCpuId,  ///< Single CPUID barrier (default) - good balance of accuracy/performance
        kLFence,    ///< Load fence barrier - prevents load reordering  
        kMFence,    ///< Memory fence barrier - prevents all memory reordering
        kRdtscp,    ///< RDTSCP barrier - Intel recommended approach
        kTwoCpuId   ///< Double CPUID barrier - maximum accuracy, higher overhead
    };

    /// High-precision TSC-based clock with configurable memory barriers
    /// @tparam BarrierType Type of memory barrier to use for instruction ordering
    template<Barrier BarrierType = Barrier::kOneCpuId>
    class TSCClock {
    public:
        /// Get starting timestamp for measurement
        /// @return TSC timestamp at measurement start
        FORCE_INLINE TimePoint StartTime() noexcept {
            details::CpuId();
            return details::Rdtsc();
        }

        /// Get starting timestamp with CPU migration detection
        /// @param cpu_number Output parameter for CPU number at start
        /// @return TSC timestamp at measurement start
        FORCE_INLINE TimePoint StartTime(CpuId& cpu_number) noexcept {
            details::CpuId();
            return details::Rdtscp(cpu_number);
        }

        /// Get ending timestamp for measurement  
        /// @return TSC timestamp at measurement end
        FORCE_INLINE TimePoint EndTime() noexcept {
            details::CpuId();
            return details::Rdtsc();
        }

        /// Get ending timestamp with CPU migration detection
        /// @param cpu_number Output parameter for CPU number at end
        /// @return TSC timestamp at measurement end
        FORCE_INLINE TimePoint EndTime(CpuId& cpu_number) noexcept {
            details::CpuId();
            return details::Rdtscp(cpu_number);
        }
    };

    /// Specialization for Load Fence barrier
    /// Uses lfence instruction to prevent load reordering after measurement
    template<>
    class TSCClock<Barrier::kLFence> {
    public:
        FORCE_INLINE TimePoint StartTime() noexcept {
            details::CpuId();
            return details::Rdtsc();
        }

        FORCE_INLINE TimePoint StartTime(CpuId& cpu_number) noexcept {
            details::CpuId();
            return details::Rdtscp(cpu_number);
        }

        FORCE_INLINE TimePoint EndTime() noexcept {
            details::LFence();
            TimePoint time = details::Rdtsc();
            details::CpuId();
            return time;
        }

        FORCE_INLINE TimePoint EndTime(CpuId& cpu_number) noexcept {
            details::LFence();
            TimePoint time = details::Rdtscp(cpu_number);
            details::CpuId();
            return time;
        }
    };

    /// Specialization for Memory Fence barrier
    /// Uses mfence instruction to prevent all memory reordering after measurement
    template<>
    class TSCClock<Barrier::kMFence> {
    public:
        FORCE_INLINE TimePoint StartTime() noexcept {
            details::CpuId();
            return details::Rdtsc();
        }

        FORCE_INLINE TimePoint StartTime(CpuId& cpu_number) noexcept {
            details::CpuId();
            return details::Rdtscp(cpu_number);
        }

        FORCE_INLINE TimePoint EndTime() noexcept {
            details::CpuId();
            TimePoint time = details::Rdtsc();
            details::MFence();
            return time;
        }

        FORCE_INLINE TimePoint EndTime(CpuId& cpu_number) noexcept {
            details::CpuId();
            TimePoint time = details::Rdtscp(cpu_number);
            details::MFence();
            return time;
        }
    };

    /// Specialization for RDTSCP barrier
    /// Uses rdtscp instruction which includes implicit ordering (Intel recommended)
    template<>
    class TSCClock<Barrier::kRdtscp> {
    public:
        FORCE_INLINE TimePoint StartTime() noexcept {
            details::CpuId();
            return details::Rdtsc();
        }

        FORCE_INLINE TimePoint StartTime(CpuId& cpu_number) noexcept {
            details::CpuId();
            return details::Rdtscp(cpu_number);
        }

        FORCE_INLINE TimePoint EndTime() noexcept {
            TimePoint time = details::Rdtscp();
            details::CpuId();
            return time;
        }

        FORCE_INLINE TimePoint EndTime(CpuId& cpu_number) noexcept {
            TimePoint time = details::Rdtscp(cpu_number);
            details::CpuId();
            return time;
        }
    };

    /// Specialization for Double CPUID barrier
    /// Uses two CPUID instructions for maximum accuracy at higher overhead cost
    template<>
    class TSCClock<Barrier::kTwoCpuId> {
    public:
        FORCE_INLINE TimePoint StartTime() noexcept {
            return GetTimestamp();
        }

        FORCE_INLINE TimePoint StartTime(CpuId& cpu_number) noexcept {
            return GetTimestamp(cpu_number);
        }

        FORCE_INLINE TimePoint EndTime() noexcept {
            return GetTimestamp();
        }

        FORCE_INLINE TimePoint EndTime(CpuId& cpu_number) noexcept {
            return GetTimestamp(cpu_number);
        }

    private:
        FORCE_INLINE TimePoint GetTimestamp() noexcept {
            details::CpuId();
            TimePoint time = details::Rdtsc();
            details::CpuId();
            return time;
        }

        FORCE_INLINE TimePoint GetTimestamp(CpuId& cpu_number) noexcept {
            details::CpuId();
            TimePoint time = details::Rdtscp(cpu_number);
            details::CpuId();
            return time;
        }
    };

} // namespace benchmarking
