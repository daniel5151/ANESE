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

#include "unsigned.h"
typedef Unsigned<2> u2;
typedef Unsigned<3> u3;

/* Helper functions */

// get nth bit from a number x, with bit layout:
// 7      0
// xxxxxxxx
template <typename T>
inline bool nth_bit(T x, u8 n) { return (x >> n) & 1; }

// check if number is in range
template <typename T, typename T2>
inline bool in_range(T x, T2 min, T2 max) { return x >= T(min) && x <= T(max); }
template <typename T, typename T2>
inline bool in_range(T x, T2 val) { return x == val; }

// Reverse binary using Lookup Tables my dudes
// https://www.geeksforgeeks.org/reverse-bits-using-lookup-table-in-o1-time/

// Generate a lookup table for 32bit operating system using macros
#define R2(n)   (n),   (n + 2*64),   (n + 1*64),   (n + 3*64)
#define R4(n) R2(n), R2(n + 2*16), R2(n + 1*16), R2(n + 3*16)
#define R6(n) R4(n), R4(n + 2*4 ), R4(n + 1*4 ), R4(n + 3*4 )

// Lookup table that store the reverse of each table
static constexpr u8 rev_u8_lut[256] = { R6(0), R6(2), R6(1), R6(3) };

#undef R2
#undef R4
#undef R6

inline constexpr u8 reverse_u8(u8 val) { return rev_u8_lut[val]; }
