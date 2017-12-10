#include "common/util.h"
#include "common/bitfield.h"

// Literal value Color type, (mostly) usable in a constexpr context

// Since i'm not a templating god, I couldn't figure out how to get BitField to
// play nicely in a constexpr context, so unfortuantely, you can't access
// individual channels through the BitField class when using constexpr colors.

union Color {
  u32 val;
  BitField<24, 8> a;
  BitField<16, 8> r;
  BitField<8,  8> g;
  BitField<0,  8> b;

  Color& operator=(const Color& val2) { this->val = val2; return *this; }

  constexpr inline Color() : val { 0x00000000 } {}
  constexpr inline Color(u8 r, u8 g, u8 b, u8 a = 0xFF)
  : val { u32(a) << 24 | r << 16 | g << 8 | b << 0 }
  {}
  constexpr inline Color(u32 color)
  : val { u32(0xFF) << 24 | color } // force alpha channel
  {}

  constexpr inline operator u32() const { return this->val; }
};
