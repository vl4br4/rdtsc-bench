#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>
#include <chrono>
#include <iomanip>
#include <numeric>
#include <random>

#include "../include/tsc_benchmark.h"

void demonstrate_basic_usage() {
    std::cout << "\n=== Basic Usage Example ===\n";
    
    using Benchmark = benchmarking::TSCBenchmarking<false, benchmarking::Barrier::kOneCpuId>;
    
    Benchmark benchmark{};
    benchmark.Initialize();
    
    // Simple arithmetic operation
    auto simple_operation = []() {
        volatile int result = 0;
        for (int i = 0; i < 100; ++i) {
            result += i * 2;
        }
    };
    
    Benchmark::Settings settings{};
    settings.cycles_number_ = 1000;
    settings.cpu_ = 0;
    settings.cache_warmup_cycles_number_ = 100;
    
    auto result = benchmark.Run(simple_operation, settings);
    
    std::cout << "Simple arithmetic (100 iterations):\n";
    std::cout << "  Time: " << result.time_ << " ns\n";
    std::cout << "  Overhead: " << result.overhead_ << " ns\n";
    std::cout << "  Net time: " << (result.time_ - result.overhead_) << " ns\n";
}

void demonstrate_barrier_comparison() {
    std::cout << "\n=== Barrier Types Comparison ===\n";
    
    // Test different barrier types
    auto test_code = []() {
        volatile int x = 42;
        x = x * x + 1;
    };
    
    using OneCpuId = benchmarking::TSCBenchmarking<false, benchmarking::Barrier::kOneCpuId>;
    using LFence = benchmarking::TSCBenchmarking<false, benchmarking::Barrier::kLFence>;
    using MFence = benchmarking::TSCBenchmarking<false, benchmarking::Barrier::kMFence>;
    
    // Test each barrier type
    {
        OneCpuId benchmark{};
        benchmark.Initialize();
        OneCpuId::Settings settings{};
        settings.cycles_number_ = 1000;
        settings.cpu_ = 0;
        auto result = benchmark.Run(test_code, settings);
        std::cout << "OneCpuId barrier: " << result.time_ << " ns\n";
    }
    
    {
        LFence benchmark{};
        benchmark.Initialize();
        LFence::Settings settings{};
        settings.cycles_number_ = 1000;
        settings.cpu_ = 0;
        auto result = benchmark.Run(test_code, settings);
        std::cout << "LFence barrier:   " << result.time_ << " ns\n";
    }
    
    {
        MFence benchmark{};
        benchmark.Initialize();
        MFence::Settings settings{};
        settings.cycles_number_ = 1000;
        settings.cpu_ = 0;
        auto result = benchmark.Run(test_code, settings);
        std::cout << "MFence barrier:   " << result.time_ << " ns\n";
    }
}

void demonstrate_cpu_migration_detection() {
    std::cout << "\n=== CPU Migration Detection ===\n";
    
    using SafeBenchmark = benchmarking::TSCBenchmarking<true, benchmarking::Barrier::kRdtscp>;
    
    SafeBenchmark benchmark{};
    benchmark.Initialize();
    
    auto code_with_potential_migration = []() {
        // Simulate some work that might trigger CPU migration
        volatile int sum = 0;
        for (int i = 0; i < 1000; ++i) {
            sum += i;
        }
    };
    
    SafeBenchmark::Settings settings{};
    settings.cycles_number_ = 500;
    settings.cpu_ = 0;
    
    auto result = benchmark.Run(code_with_potential_migration, settings);
    
    std::cout << "With CPU migration detection:\n";
    std::cout << "  Time: " << result.time_ << " ns\n";
    std::cout << "  (Invalid measurements due to CPU migration are automatically discarded)\n";
}

void demonstrate_memory_operations() {
    std::cout << "\n=== Memory Operations Benchmark ===\n";
    
    using Benchmark = benchmarking::TSCBenchmarking<false, benchmarking::Barrier::kOneCpuId>;
    
    Benchmark benchmark{};
    benchmark.Initialize();
    
    // Prepare test data
    std::vector<int> data(1000);
    std::iota(data.begin(), data.end(), 0);
    
    Benchmark::Settings settings{};
    settings.cycles_number_ = 100;
    settings.cpu_ = 0;
    settings.cache_warmup_cycles_number_ = 50;
    
    // Test different memory access patterns
    
    // Sequential access
    auto sequential_access = [&data]() {
        volatile int sum = 0;
        for (size_t i = 0; i < data.size(); ++i) {
            sum += data[i];
        }
    };
    
    auto result1 = benchmark.Run(sequential_access, settings);
    std::cout << "Sequential memory access: " << result1.time_ << " ns\n";
    
    // Random access
    std::vector<size_t> indices(data.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(indices.begin(), indices.end(), g);
    
    auto random_access = [&data, &indices]() {
        volatile int sum = 0;
        for (size_t idx : indices) {
            sum += data[idx];
        }
    };
    
    auto result2 = benchmark.Run(random_access, settings);
    std::cout << "Random memory access:    " << result2.time_ << " ns\n";
    
    // Cache line traversal
    auto cache_line_access = [&data]() {
        volatile int sum = 0;
        for (size_t i = 0; i < data.size(); i += 16) { // Assume 64-byte cache lines, 4-byte ints
            sum += data[i];
        }
    };
    
    auto result3 = benchmark.Run(cache_line_access, settings);
    std::cout << "Cache line access:       " << result3.time_ << " ns\n";
}

void demonstrate_minimal_overhead() {
    std::cout << "\n=== Minimal Overhead Measurement ===\n";
    
    using Benchmark = benchmarking::TSCBenchmarking<false, benchmarking::Barrier::kOneCpuId>;
    
    Benchmark benchmark{};
    benchmark.Initialize();
    
    // For critical applications where you need minimal measurement overhead
    auto critical_code = []() {
        volatile int x = 1;
        x <<= 1;
    };
    
    // Single measurement with minimal overhead
    auto raw_time = benchmark.MeasureTime(critical_code);
    
    std::cout << "Minimal overhead measurement: " << raw_time << " ns (raw)\n";
    std::cout << "Note: This includes TSC overhead, use Run() for overhead-corrected results\n";
}

void display_cpu_info() {
    std::cout << "\n=== CPU Information ===\n";
    
    benchmarking::details::CpuInfo cpu_info{};
    
    std::cout << "TSC supported:          " << (cpu_info.IsTscEnabled() ? "Yes" : "No") << "\n";
    std::cout << "RDTSCP supported:       " << (cpu_info.IsRdtscpEnabled() ? "Yes" : "No") << "\n";
    std::cout << "Invariant TSC:          " << (cpu_info.IsInvariantTscEnabled() ? "Yes" : "No") << "\n";
    
    // Display number of CPU cores
    int cpu_cores = benchmarking::details::GetCpuCoreCount();
    std::cout << "CPU cores available:    " << cpu_cores << "\n";
}

int main() {
    std::cout << "TSC Benchmark Library - Advanced Examples\n";
    std::cout << "=========================================\n";
    
    display_cpu_info();
    
    try {
        demonstrate_basic_usage();
        demonstrate_barrier_comparison();
        demonstrate_cpu_migration_detection();
        demonstrate_memory_operations();
        demonstrate_minimal_overhead();
        
        std::cout << "\n=== All examples completed successfully! ===\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 