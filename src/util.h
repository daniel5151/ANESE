#include <cstdint>

#pragma once

/* Type Aliases */

// Unsigned aliases
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

// Signed aliases
typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

/* Helper functions */

/// get nth bit from a number x
template <typename T>
inline bool nth_bit(T x, uint8 n) { return (x >> n) & 1; }
