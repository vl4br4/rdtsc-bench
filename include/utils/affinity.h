#pragma once

#include <iostream>
#include <sched.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <cstring>
#include <cerrno>

namespace benchmarking::details {

    /// Pin current thread to specific CPU core (Linux x86 implementation)
    /// @param cpu CPU core number (0-based)
    /// @return true if successful, false otherwise
    bool PinThread(int cpu) noexcept {
        if (cpu < 0) {
            std::cerr << "[Warning] CPU core number must be >= 0, got: " << cpu << std::endl;
            return false;
        }

        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);
        CPU_SET(cpu, &cpu_set);
        
        if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set) != 0) {
            std::cerr << "[Warning] Failed to set CPU affinity to core " << cpu 
                      << ": " << std::strerror(errno) << std::endl;
            return false;
        }
        
        std::cout << "[Info] Thread pinned to CPU " << cpu << std::endl;
        return true;
    }

    /// Get number of available CPU cores (Linux implementation)
    /// @return number of CPU cores
    int GetCpuCoreCount() noexcept {
        int cores = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
        return cores > 0 ? cores : 1;
    }

} // namespace benchmarking::details
