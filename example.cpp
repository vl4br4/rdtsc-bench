#include <iostream>
#include <vector>
#include <chrono>

#include "tsc_benchmark.h"

int main() {
    std::cout << "TSC Benchmark Example\n";
    std::cout << "====================\n\n";

    // Test data
    std::vector<int> test_vector;
    test_vector.reserve(200);
    for (int i = 0; i < 100; ++i) {
        test_vector.push_back(i);
    }

    // Setup benchmark
    using Benchmark = benchmarking::TSCBenchmarking<false, benchmarking::Barrier::kOneCpuId>;
    
    Benchmark::Settings settings{};
    settings.cycles_number_ = 1000;
    settings.cpu_ = 0;
    settings.cache_warmup_cycles_number_ = 100;

    // Define code to benchmark
    auto code_for_benchmarking = [&test_vector]() {
        for (int i = 100; i < 200; ++i) {
            test_vector.push_back(i);
        }
        test_vector.resize(100); // Reset for next iteration
    };

    // Initialize and run benchmark
    Benchmark benchmark{};
    std::cout << "Initializing benchmark...\n";
    benchmark.Initialize();
    
    std::cout << "Running benchmark...\n";
    auto result = benchmark.Run(code_for_benchmarking, settings);
    
    std::cout << "\nResults:\n";
    std::cout << "- Execution time: " << result.time_ << " ns\n";
    std::cout << "- TSC overhead: " << result.overhead_ << " ns\n";
    std::cout << "- Net time: " << (result.time_ - result.overhead_) << " ns\n";
    
    return 0;
}
