//
// Portable Bitfield implementation
// http://blog.codef00.com/2014/12/06/portable-bitfields-using-c11/
//
// Slightly modified by Daniel Prilik
//

#ifndef BITFIELD_H_
#define BITFIELD_H_

#include <cstdint>
#include <cstddef>
#include <type_traits>

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;

namespace {

template <size_t LastBit>
struct MinimumTypeHelper {
    typedef
        typename std::conditional<LastBit == 0 , void,
        typename std::conditional<LastBit <= 8 , uint8_t,
        typename std::conditional<LastBit <= 16, uint16_t,
        typename std::conditional<LastBit <= 32, uint32_t,
        typename std::conditional<LastBit <= 64, uint64_t,
        void>::type>::type>::type>::type>::type type;
};

}

template <size_t Index, size_t Bits = 1>
class BitField {
private:
    enum {
        Mask = (1u << Bits) - 1u
    };

    typedef typename MinimumTypeHelper<Index + Bits>::type T;
public:
    BitField() = default;
    BitField(const BitField& other_bf) = default;

    BitField &operator=(const BitField& other_bf) {
        *this = T(other_bf);
        return *this;
    };

    BitField &operator=(T value) {
        value_ = (value_ & ~(Mask << Index)) | ((value & Mask) << Index);
        return *this;
    }

    operator T() const             { return (value_ >> Index) & Mask; }
    explicit operator bool() const { return value_ & (Mask << Index); }
    BitField &operator++()         { return *this = *this + 1; }
    T operator++(int)              { T r = *this; ++*this; return r; }
    BitField &operator--()         { return *this = *this - 1; }
    T operator--(int)              { T r = *this; --*this; return r; }

private:
    T value_;
};

template <size_t Index>
class BitField<Index, 1> {
private:
    enum {
        Bits = 1,
        Mask = 0x01
    };

    typedef typename MinimumTypeHelper<Index + Bits>::type T;
public:
    BitField() = default;
    BitField(const BitField& other_bf) = default;

    BitField &operator=(const BitField& other_bf) {
        *this = bool(other_bf);
        return *this;
    };

    BitField &operator=(bool value) {
        value_ = (value_ & ~(Mask << Index)) | (value << Index);
        return *this;
    }

    operator bool() const { return bool(value_ & (Mask << Index)); }

private:
    T value_;
};

#endif
