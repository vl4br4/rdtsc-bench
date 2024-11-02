/**
 * @file tsc_benchmark.h
 * @brief High-performance TSC-based benchmark library for nanosecond-precision measurements
 * 
 * This library provides ultra-low overhead benchmarking using Time Stamp Counter (TSC)
 * instructions for measuring small code sections with nanosecond accuracy.
 * 
 * Features:
 * - Nanosecond precision using RDTSC/RDTSCP instructions
 * - Configurable memory barriers for instruction ordering
 * - Optional CPU migration detection
 * - Automatic overhead calculation and subtraction
 * - Cross-platform support (Linux/macOS)
 * 
 * @author TSC Benchmark Library
 * @version 1.0.0
 */

#pragma once

// Standard library includes  
#include <chrono>
#include <cassert>
#include <algorithm>
#include <limits>
#include <utility>
#include <iostream>

// Linux system includes
#include <unistd.h>
#include <cstring>
#include <sys/mman.h>
#include <sched.h>

// Project includes
#include "tsc_clock.h"
#include "utils/compiler.h"
#include "utils/types.h"
#include "utils/affinity.h"

namespace benchmarking {

    namespace details {
        /// Empty lambda for overhead measurement
        inline constexpr auto kEmptyCode = []() {};
    } // namespace details

    /**
     * @brief High-precision TSC-based benchmark class
     * 
     * This class provides low-overhead timing measurements using Time Stamp Counter
     * with configurable memory barriers and optional CPU migration detection.
     * 
     * @tparam CheckCpuMigration Enable CPU migration detection between measurements
     * @tparam BarrierType Type of memory barrier to use for instruction ordering
     * 
     * Example usage:
     * @code
     * using Benchmark = TSCBenchmarking<false, Barrier::kOneCpuId>;
     * 
     * Benchmark benchmark{};
     * benchmark.Initialize();
     * 
     * Benchmark::Settings settings{};
     * auto result = benchmark.Run([]() { return; }, settings);
     * @endcode
     */
    template<bool CheckCpuMigration = true, Barrier BarrierType = Barrier::kOneCpuId>
    class TSCBenchmarking {
    public:
        class Result;
        class Settings;

        /// Constructor - validates TSC support
        TSCBenchmarking();

        /// Initialize benchmark system (configure scheduling, calculate overhead)
        void Initialize();

        /**
         * @brief Minimal overhead measurement for time-critical applications
         * @tparam Code Callable type for code to measure
         * @param code Code to benchmark
         * @return Raw timestamp difference (includes TSC overhead)
         */
        template<typename Code>
        FORCE_INLINE TimePoint MeasureTime(Code&& code);

        /**
         * @brief Full benchmark with statistics and overhead correction
         * @tparam Code Callable type for code to measure  
         * @param code Code to benchmark
         * @param settings Benchmark configuration
         * @return Benchmark result with timing and overhead information
         */
        template<typename Code>
        Result Run(Code&& code, Settings settings);

        ~TSCBenchmarking() = default;

        /**
         * @brief Benchmark configuration settings
         */
        class Settings {
        public:
            /// Number of measurement cycles for averaging
            std::size_t cycles_number_{kDefaultCyclesNumber};
            
            /// CPU core to pin thread to (0-based)
            int cpu_{0};
            
            /// Number of warmup cycles before measurement  
            std::size_t cache_warmup_cycles_number_{0};
        };

        /**
         * @brief Benchmark measurement result
         */
        class Result {
        public:
            using Nanos = std::chrono::nanoseconds;
            
            /// Average execution time in nanoseconds
            TimePoint time_;
            
            /// Measured TSC overhead in nanoseconds
            TimePoint overhead_;
        };

    private:
        // Default configuration constants
        static constexpr std::size_t kDefaultCyclesNumber = 100;
        static constexpr std::size_t kDefaultStabilizedThreshold = kDefaultCyclesNumber * 10 / 100;
        static constexpr std::size_t kDefaultRunsNumber = 100;

        /// Dummy clock operation for overhead measurement
        static constexpr auto kGetTime = []() {
            timespec ts{};
            clock_gettime(CLOCK_REALTIME, &ts);
        };
        
        // Private measurement methods
        std::pair<TimePoint, TimePoint> MeasureOverhead(std::size_t cycles_number = kDefaultCyclesNumber);
        
        std::pair<TimePoint, TimePoint> MeasureStabilizedOverhead(std::size_t cycles_number = kDefaultCyclesNumber,
                                                                  std::size_t stabilized_threshold = kDefaultStabilizedThreshold);

        template<typename Code>
        FORCE_INLINE TimePoint MeasureMinLatency(std::size_t cycles_number, Code&& code);

        template<typename Code>
        FORCE_INLINE TimePoint MeasureStabilizedMinLatency(std::size_t cycles_number,
                                                           std::size_t stabilized_threshold,
                                                           Code&& code);

        template<typename Code>
        FORCE_INLINE bool Measure(TimePoint& start, TimePoint& end, Code&& code);

    private:
        TSCClock<BarrierType> clock_{};         ///< TSC clock instance
        TimePoint tsc_overhead_{0};             ///< Measured TSC overhead
        TimePoint clock_overhead_{0};           ///< Measured clock overhead
    };


    // Implementation
    template<bool CheckCpuMigration, Barrier BarrierType>
    TSCBenchmarking<CheckCpuMigration, BarrierType>::TSCBenchmarking() {
        details::CpuInfo cpu_info{};
        assert(cpu_info.IsTscEnabled());
        assert(BarrierType != Barrier::kRdtscp || cpu_info.IsRdtscpEnabled());
        if (!cpu_info.IsInvariantTscEnabled()) {
            std::cerr << "[Warning] Invariant TSC is not supported on your system" << std::endl;
        }
    }

    template<bool CheckCpuMigration, Barrier BarrierType>
    void TSCBenchmarking<CheckCpuMigration, BarrierType>::Initialize() {
        // Linux real-time optimizations
        if (geteuid() == 0) {
            sched_param sp{};
            sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
            if (sched_setscheduler(0, SCHED_FIFO, &sp) == -1) {
                std::cerr << "[Warning] Error changing scheduling policy to RT class" << std::endl;
            } else {
                std::cout << "[Info] Scheduling policy changed to RT class with max priority" << std::endl;
            }

            if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
                std::cerr << "[Warning] Error locking pages" << std::endl;
            } else {
                std::cout << "[Info] All pages of process are locked (paging disabled)" << std::endl;
            }
        } else {
            std::cerr << "[Warning] Benchmark launched without ROOT permissions - default scheduler/priority" << std::endl;
        }


        auto overheads = MeasureOverhead();
        tsc_overhead_ = overheads.first;
        clock_overhead_ = overheads.second;
    }

    template<bool CheckCpuMigration, Barrier BarrierType>
    template<typename Code>
    TimePoint TSCBenchmarking<CheckCpuMigration, BarrierType>::MeasureTime(Code&& code) {
        TimePoint start = clock_.StartTime();
        code();
        TimePoint end = clock_.EndTime();
        return end - start;
    }

    template<bool CheckCpuMigration, Barrier BarrierType>
    template<typename Code>
    TSCBenchmarking<CheckCpuMigration, BarrierType>::Result TSCBenchmarking<CheckCpuMigration, BarrierType>::Run(Code&& code,
                                                                           TSCBenchmarking::Settings settings) {
        if (!details::PinThread(settings.cpu_)) {
            std::cerr << "[Warning] Failed to pin thread to CPU " << settings.cpu_ << std::endl;
        }

        TimePoint start, end;

        for (std::size_t r = 0; r < settings.cache_warmup_cycles_number_; ++r) {
            if constexpr (CheckCpuMigration) {
                CpuId cpu_number0{0}, cpu_number1{1};
                start = clock_.StartTime(cpu_number0);
                code.operator()();
                end = clock_.EndTime(cpu_number1);
            } else {
                start = clock_.StartTime();
                code.operator()();
                end = clock_.EndTime();
            }
        }

        std::uint64_t summary_time = 0;
        for (std::size_t r = 0; r < settings.cycles_number_;) {
            if constexpr (CheckCpuMigration) {
                CpuId cpu_number0{0}, cpu_number1{1};
                start = clock_.StartTime(cpu_number0);
                code.operator()();
                end = clock_.EndTime(cpu_number1);
                if (cpu_number0 != cpu_number1) {
                    continue;
                }
            } else {
                start = clock_.StartTime();
                code.operator()();
                end = clock_.EndTime();
            }

            if (LIKELY(end > start)) {
                TimePoint time = end - start;
                if (time > tsc_overhead_) {
                    summary_time += time;
                    ++r;
                }
            }
        }
        return TSCBenchmarking::Result{summary_time / settings.cycles_number_, tsc_overhead_};
    }

    template<bool CheckCpuMigration, Barrier BarrierType>
    std::pair<TimePoint, TimePoint> TSCBenchmarking<CheckCpuMigration, BarrierType>::MeasureOverhead(std::size_t cycles_number) {
        TimePoint min_tsc_overhead = MeasureMinLatency(cycles_number, details::kEmptyCode);
        TimePoint min_clock_overhead = MeasureMinLatency(cycles_number, kGetTime);
        return {min_tsc_overhead, min_clock_overhead - min_tsc_overhead};
    }

    template<bool CheckCpuMigration, Barrier BarrierType>
    std::pair<TimePoint, TimePoint> TSCBenchmarking<CheckCpuMigration, BarrierType>::MeasureStabilizedOverhead(
            std::size_t cycles_number, std::size_t stabilized_threshold) {
        TimePoint min_tsc_overhead = MeasureStabilizedMinLatency(cycles_number,
                                                                 stabilized_threshold,
                                                                 details::kEmptyCode);
        TimePoint min_clock_overhead = MeasureStabilizedMinLatency(cycles_number,
                                                                   stabilized_threshold,
                                                                   kGetTime);
        return {min_tsc_overhead, min_clock_overhead - min_tsc_overhead};
    }

    template<bool CheckCpuMigration, Barrier BarrierType>
    template<typename Code>
    TimePoint TSCBenchmarking<CheckCpuMigration, BarrierType>::MeasureMinLatency(std::size_t cycles_number,
                                                                                 Code&& code) {
        TimePoint start, end;
        TimePoint min_latency = std::numeric_limits<TimePoint>::max();
        for (std::size_t i = 0; i < cycles_number;) {
            if (Measure<Code>(start, end, std::forward<Code>(code)) && LIKELY(end > start)) {
                min_latency = std::min(min_latency, end - start);
                ++i;
            }
        }
        return min_latency;
    }

    template<bool CheckCpuMigration, Barrier BarrierType>
    template<typename Code>
    TimePoint TSCBenchmarking<CheckCpuMigration, BarrierType>::MeasureStabilizedMinLatency(
            std::size_t cycles_number,
            std::size_t stabilized_threshold,
            Code&& code) {
        TimePoint start, end;
        TimePoint min_latency = std::numeric_limits<TimePoint>::max();
        std::size_t min_latency_cycles_number = 0;
        for (std::size_t i = 0; i < cycles_number && min_latency_cycles_number < stabilized_threshold;) {
            if (Measure<Code>(start, end, std::forward<Code>(code)) && LIKELY(end > start)) {
                TimePoint latency = end - start;

                if (latency < min_latency) {
                    min_latency = latency;
                    min_latency_cycles_number = 0;
                }

                ++min_latency_cycles_number;
                ++i;
            }
        }
        return min_latency;
    }


    template<bool CheckCpuMigration, Barrier BarrierType>
    template<typename Code>
    bool TSCBenchmarking<CheckCpuMigration, BarrierType>::Measure(TimePoint& start, TimePoint& end, Code&& code) {
        if constexpr (CheckCpuMigration) {
            uint32_t start_cpu_number{0}, end_cpu_number{1};
            start = clock_.StartTime(start_cpu_number);
            code();
            end = clock_.EndTime(end_cpu_number);
            return start_cpu_number == end_cpu_number;
        } else {
            start = clock_.StartTime();
            code();
            end = clock_.EndTime();
            return true;
        }
    }

} // namespace benchmarking
