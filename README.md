# TSC Benchmark

A small C++ library for micro-benchmarking using the Time Stamp Counter (TSC). It's designed for measuring small sections of code on x86-64 Linux systems with low overhead.

## Features

- Uses `RDTSC`/`RDTSCP` for high-precision timing.
- Calculates and subtracts its own measurement overhead.
- Supports memory barriers to prevent instruction reordering.
- Can detect if the code migrates between CPU cores during measurement.
- Header-only for easy integration with CMake.

## Quick Start

```cpp
#include "tsc_benchmark.h"

int main() {
    // 1. Setup the benchmark
    using Benchmark = benchmarking::TSCBenchmarking<false, benchmarking::Barrier::kOneCpuId>;
    
    Benchmark benchmark{};
    benchmark.Initialize();
    
    // 2. Configure settings
    Benchmark::Settings settings{};
    settings.cycles_number_ = 1000;
    settings.cpu_ = 0; // Pin to CPU core 0
    
    // 3. Define the code to measure
    auto code_to_measure = []() {
        volatile int x = 42 * 2;
    };
    
    // 4. Run the benchmark
    auto result = benchmark.Run(code_to_measure, settings);
    
    std::cout << "Average time: " << result.time_ << " ns\n";
    std::cout << "Measurement overhead: " << result.overhead_ << " ns\n";
    
    return 0;
}
```

## Building

### Requirements

- A C++20 compatible compiler (like GCC or Clang)
- CMake 3.27+
- An x86-64 processor that supports TSC

### Build Instructions

```bash
mkdir build && cd build
cmake ..
make

# Run the example
./benchmark_example
```

## Memory Barrier Types

To prevent the compiler or CPU from reordering instructions, you can use memory barriers.

| Barrier Type | Description | Accuracy | Overhead |
|--------------|-------------|----------|----------|
| `kOneCpuId` | Single `CPUID` instruction (default) | High | Low |
| `kLFence` | `lfence` instruction | High | Low |
| `kMFence` | `mfence` instruction | Very High | Medium |
| `kRdtscp` | `rdtscp` instruction | Very High | Low |
| `kTwoCpuId` | Two `CPUID` instructions | Maximum | High |

Example:
```cpp
// Use a different barrier for higher accuracy
using PreciseBenchmark = benchmarking::TSCBenchmarking<true, benchmarking::Barrier::kTwoCpuId>;
```

## Limitations

- **x86-64 only**: Requires TSC instruction support.
- **Linux focused**: Uses Linux-specific functions for pinning threads and real-time scheduling. It may not compile on other systems.

## References

- [Intel® 64 and IA-32 Architectures Software Developer's Manual](https://software.intel.com/content/www/us/en/develop/articles/intel-sdm.html)
- [RDTSC Instruction](https://www.felixcloutier.com/x86/rdtsc)
