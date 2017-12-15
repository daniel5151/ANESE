#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace {

// Stolen wholesale from
// http://blog.codef00.com/2014/12/06/portable-bitfields-using-c11/

template <size_t LastBit>
struct MinimumTypeHelper2 {
    typedef
        typename std::conditional<LastBit == 0 , void,
        typename std::conditional<LastBit <= 8 , uint8_t,
        typename std::conditional<LastBit <= 16, uint16_t,
        typename std::conditional<LastBit <= 32, uint32_t,
        typename std::conditional<LastBit <= 64, uint64_t,
        void>::type>::type>::type>::type>::type type;
};

}

// Custom unsigned listeral types, with proper overlow handling!
// This is a bit of a WIP...
// I need to implement all the operators
template <unsigned size>
struct Unsigned {
private:
  typedef typename MinimumTypeHelper2<size>::type T;

  T value_ : size;
public:
  constexpr Unsigned(T val) : value_(val) {}
  constexpr Unsigned() : value_(0) {}

  constexpr operator T() const { return value_; }

  template <class T2>
  constexpr bool operator==(T2 val_2) const { return value_ == val_2; }

  template <class T2>
  Unsigned &operator=(T2 new_val) {
    value_ = new_val;
    return *this;
  }
};
