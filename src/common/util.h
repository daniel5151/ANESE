#pragma once

#include <cstdint>

/* Type Aliases */

// Unsigned aliases
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uint32_t uint;

// Signed aliases
typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

/* Helper functions */

// get nth bit from a number x, with bit layout:
// 7      0
// xxxxxxxx
template <typename T>
constexpr inline bool nth_bit(T x, u8 n) { return (x >> n) & 1; }

// check if number is in range
template <typename T, typename T2>
constexpr inline bool in_range(T x, T2 min, T2 max) { return x >= T(min) && x <= T(max); }
template <typename T, typename T2>
constexpr inline bool in_range(T x, T2 val) { return x == val; }
