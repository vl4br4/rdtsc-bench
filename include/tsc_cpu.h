#pragma once

#include "utils/compiler.h"
#include "utils/types.h"

namespace benchmarking::details {

    /// Combine low and high 32-bit registers into 64-bit value
    /// @param low Lower 32 bits
    /// @param high Upper 32 bits  
    /// @return Combined 64-bit value
    FORCE_INLINE Register CombineRegisters(Register low, Register high) noexcept {
        return (high << 32) | low;
    }

    /// Read Time Stamp Counter (RDTSC instruction)
    /// @return Current TSC value
    FORCE_INLINE TimePoint Rdtsc() noexcept {
        uint32_t low, high;
        __asm__ __volatile__("rdtsc" : "=a"(low), "=d"(high));
        return CombineRegisters(low, high);
    }

    /// Read Time Stamp Counter and Processor ID (RDTSCP instruction)
    /// @return Current TSC value
    FORCE_INLINE TimePoint Rdtscp() noexcept {
        uint32_t low, high;
        __asm__ __volatile__("rdtscp" : "=a"(low), "=d"(high) :: "rcx");
        return CombineRegisters(low, high);
    }

    /// Read Time Stamp Counter and Processor ID with chip number
    /// @param chip_number Output parameter for chip/processor number
    /// @return Current TSC value
    FORCE_INLINE TimePoint Rdtscp(CpuId& chip_number) noexcept {
        uint32_t low, high, aux;
        __asm__ __volatile__("rdtscp" : "=a"(low), "=d"(high), "=c"(aux));
        chip_number = static_cast<CpuId>(aux & 0xFFFFFF);
        return CombineRegisters(low, high);
    }

    /// Read Time Stamp Counter and Processor ID with separate chip and core
    /// @param chip Output parameter for chip number
    /// @param core Output parameter for core number
    /// @return Current TSC value
    FORCE_INLINE TimePoint Rdtscp(CpuId& chip, CpuId& core) noexcept {
        uint32_t low, high, aux;
        __asm__ __volatile__("rdtscp" : "=a"(low), "=d"(high), "=c"(aux));
        chip = static_cast<CpuId>((aux & 0xFFF000) >> 12);
        core = static_cast<CpuId>(aux & 0xFFF);
        return CombineRegisters(low, high);
    }

    /// Execute CPUID instruction (serializing instruction)
    FORCE_INLINE void CpuId() noexcept {
        __asm__ __volatile__("cpuid" :: "a"(0) : "rbx", "rcx", "rdx");
    }

    /// Load fence - orders loads
    FORCE_INLINE void LFence() noexcept {
        __asm__ __volatile__("lfence" ::: "memory");
    }

    /// Memory fence - orders all memory operations
    FORCE_INLINE void MFence() noexcept {
        __asm__ __volatile__("mfence" ::: "memory");
    }

    /// CPU information and capability detection
    class CpuInfo {
    private:
        using InternalReg = InternalRegister;

        // CPUID feature bit positions
        static constexpr InternalReg kTscFeatureBit = 1u << 4;      // TSC support
        static constexpr InternalReg kRdtscpFeatureBit = 1u << 27;  // RDTSCP support  
        static constexpr InternalReg kInvariantTscBit = 1u << 8;    // Invariant TSC

    public:
        CpuInfo() {
            // Execute CPUID with EAX=1
            __asm__ __volatile__("cpuid" 
                                : "=a"(registers_[0]), "=b"(registers_[1]), 
                                  "=c"(registers_[2]), "=d"(registers_[3])
                                : "a"(1), "c"(0));
        }

        CpuInfo(const CpuInfo&) = default;
        CpuInfo(CpuInfo&&) = default;
        CpuInfo& operator=(const CpuInfo&) = default;
        CpuInfo& operator=(CpuInfo&&) = default;
        ~CpuInfo() = default;

        /// Check if TSC (Time Stamp Counter) is supported
        [[nodiscard]] bool IsTscEnabled() const noexcept {
            return IsFeatureEnabled(GetEdx(), kTscFeatureBit);
        }

        /// Check if Invariant TSC is supported
        [[nodiscard]] bool IsInvariantTscEnabled() const noexcept {
            return IsFeatureEnabled(GetEdx(), kInvariantTscBit);
        }

        /// Check if RDTSCP instruction is supported
        [[nodiscard]] bool IsRdtscpEnabled() const noexcept {
            return IsFeatureEnabled(GetEdx(), kRdtscpFeatureBit);
        }

    private:
        [[nodiscard]] bool IsFeatureEnabled(InternalReg reg, InternalReg mask) const noexcept {
            return (reg & mask) != 0;
        }

        [[nodiscard]] const InternalReg& GetEdx() const noexcept { return registers_[3]; }

        InternalReg registers_[4]{};
    };

} // namespace benchmarking::details
