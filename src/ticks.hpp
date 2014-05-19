// CodeDB - public domain - 2010 Daniel Andersson
#pragma once

#include <cstdint>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#if defined(__GNUC__) && defined(__i386__)

static __inline__ std::uint64_t getticks() {
  std::uint64_t x;
  asm volatile("rdtsc" : "=A"(x));
  return x;
}

#elif defined(__GNUC__) && defined(__x86_64__)

static __inline__ std::uint64_t getticks() {
  std::uint32_t hi, lo;
  asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return static_cast<std::uint64_t>(lo) | static_cast<std::uint64_t>(hi) << 32;
}

#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))

static __forceinline std::uint64_t getticks() { return __rdtsc(); }

#else

inline std::uint64_t getticks() { return 0; }

#endif

int cpu_mhz();
