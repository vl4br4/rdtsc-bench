#pragma once

#include <cstdint>

namespace benchmarking {

    /// Type for storing TSC timestamp values
    using TimePoint = std::uint64_t;

    /// Type for CPU register values  
    using Register = std::uint64_t;

    /// Type for CPU core/chip identification
    using CpuId = std::uint32_t;

    namespace details {
        /// Internal register type for low-level operations
        using InternalRegister = std::uint32_t;
    }

} // namespace benchmarking
