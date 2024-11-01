# TSC Benchmark Library

A high-performance, low-overhead nanosecond-precision benchmark library using Time Stamp Counter (TSC) for measuring small code sections with nanosecond accuracy.

## Features

- ⚡ **Ultra-low overhead**: Minimal measurement overhead (~1-10ns)
- 🎯 **Nanosecond precision**: TSC-based timing with sub-nanosecond accuracy
- 🔒 **CPU migration detection**: Optional CPU migration detection between measurements
- 🚧 **Configurable barriers**: Multiple memory barrier types for different accuracy/performance trade-offs
- 🖥️ **Cross-platform**: Support for Linux and macOS
- 📊 **Statistical analysis**: Automatic overhead calculation and result averaging
- 🔧 **Header-only**: Easy integration with CMake support

## Quick Start

```cpp
#include "tsc_benchmark.h"

int main() {
    // Setup benchmark with CPU migration detection disabled
    using Benchmark = benchmarking::TSCBenchmarking<false, benchmarking::Barrier::kOneCpuId>;
    
    Benchmark benchmark{};
    benchmark.Initialize();
    
    // Configure benchmark settings
    Benchmark::Settings settings{};
    settings.cycles_number_ = 1000;           // Number of measurement cycles
    settings.cpu_ = 0;                        // CPU core to pin to
    settings.cache_warmup_cycles_number_ = 100; // Cache warmup iterations
    
    // Define code to benchmark
    auto code_to_measure = []() {
        // Your code here
        volatile int x = 42;
        x *= 2;
    };
    
    // Run benchmark
    auto result = benchmark.Run(code_to_measure, settings);
    
    std::cout << "Execution time: " << result.time_ << " ns\n";
    std::cout << "TSC overhead: " << result.overhead_ << " ns\n";
    
    return 0;
}
```

## Memory Barrier Types

The library supports different barrier types for instruction ordering:

| Barrier Type | Description | Accuracy | Overhead | Use Case |
|--------------|-------------|----------|----------|----------|
| `kOneCpuId` | Single CPUID barrier (default) | High | Low | General purpose |
| `kLFence` | Load fence barrier | High | Low | Load ordering critical |
| `kMFence` | Memory fence barrier | Very High | Medium | Full memory ordering |
| `kRdtscp` | RDTSCP instruction | Very High | Low | Intel recommended |
| `kTwoCpuId` | Double CPUID barrier | Maximum | High | Maximum accuracy needed |

### Example with different barriers:

```cpp
// High accuracy with low overhead
using FastBenchmark = benchmarking::TSCBenchmarking<false, benchmarking::Barrier::kRdtscp>;

// Maximum accuracy
using PreciseBenchmark = benchmarking::TSCBenchmarking<true, benchmarking::Barrier::kTwoCpuId>;
```

## CPU Migration Detection

Enable CPU migration detection to ensure measurements are taken on the same CPU core:

```cpp
// Enable CPU migration detection (first template parameter = true)
using SafeBenchmark = benchmarking::TSCBenchmarking<true, benchmarking::Barrier::kOneCpuId>;

SafeBenchmark benchmark{};
benchmark.Initialize();

auto result = benchmark.Run(code_to_measure, settings);
// Measurements with CPU migration are automatically discarded
```

## Building

### Requirements

- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.27+
- x86-64 processor with TSC support

### Build Instructions

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run example
./benchmark_example
```

### Integration with CMake

Add to your `CMakeLists.txt`:

```cmake
find_package(tsc_benchmark REQUIRED)
target_link_libraries(your_target PRIVATE tsc_benchmark)
```

Or as a subdirectory:

```cmake
add_subdirectory(tsc-benchmark)
target_link_libraries(your_target PRIVATE tsc_benchmark)
```

## Advanced Usage

### Minimal Overhead Measurement

For time-critical applications, use the minimal measurement method:

```cpp
Benchmark benchmark{};
benchmark.Initialize();

// Minimal overhead measurement (no statistics, no warmup)
TimePoint raw_time = benchmark.MeasureTime([]() {
    // Critical code here
});
```

### Custom Settings

```cpp
Benchmark::Settings settings{};
settings.cycles_number_ = 10000;              // More cycles for better statistics
settings.cpu_ = 2;                            // Pin to specific CPU core
settings.cache_warmup_cycles_number_ = 500;   // Extensive warmup
```

## Hardware Requirements

### Supported Instructions

The benchmark automatically detects and validates:

- **TSC (Time Stamp Counter)**: Required for basic functionality
- **RDTSCP**: Required for `kRdtscp` barrier and CPU migration detection  
- **Invariant TSC**: Recommended for accurate measurements across P-states

### CPU Features Validation

```cpp
benchmarking::details::CpuInfo cpu_info{};

if (!cpu_info.IsTscEnabled()) {
    std::cerr << "TSC not supported on this CPU\n";
    return -1;
}

if (!cpu_info.IsInvariantTscEnabled()) {
    std::cerr << "Warning: Invariant TSC not available\n";
}
```

## Performance Considerations

### Measurement Overhead

The library automatically measures and subtracts its own overhead:

- **TSC overhead**: Time to execute RDTSC/RDTSCP instructions
- **Clock overhead**: Additional overhead from system calls

### Best Practices

1. **Run as root** for real-time scheduling and memory locking
2. **Disable CPU frequency scaling** for consistent results
3. **Pin to isolated CPU cores** to avoid interference
4. **Use appropriate barrier types** based on your accuracy needs
5. **Warm up caches** before critical measurements

## Error Handling

The library provides comprehensive error reporting:

```cpp
// Check initialization success
benchmark.Initialize();  // Warnings printed to stderr

// CPU pinning failures are reported
if (!details::PinThread(cpu_core)) {
    // Handle pinning failure
}
```

## Limitations

- **x86-64 only**: Requires TSC instruction support
- **Linux/macOS**: Limited Windows support
- **Single-threaded**: Not designed for multi-threaded benchmarking
- **Nano-benchmarks**: Optimized for very short code sections

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Submit a pull request

## License

[License information would go here]

## References

- [Intel® 64 and IA-32 Architectures Software Developer's Manual](https://software.intel.com/content/www/us/en/develop/articles/intel-sdm.html)
- [RDTSC and RDTSCP Instructions](https://www.felixcloutier.com/x86/rdtsc)
- [How to Benchmark Code Execution Times on Intel® IA-32 and IA-64 Instruction Set Architectures](https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/ia-32-ia-64-benchmark-code-execution-paper.pdf)
