#pragma once

// Force inline macro (GCC/Clang)
#ifndef FORCE_INLINE
    #define FORCE_INLINE inline __attribute__((always_inline))
#endif

// Branch prediction hints (GCC/Clang)
#ifndef LIKELY
    #define LIKELY(x) __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

// Compiler barrier (GCC/Clang)
#ifndef COMPILER_BARRIER
    #define COMPILER_BARRIER() __asm__ __volatile__("" ::: "memory")
#endif
