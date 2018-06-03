#pragma once

namespace Mirroring {
  enum Type {
    Vertical = 0,
    Horizontal,
    FourScreen,
    SingleScreenLo,
    SingleScreenHi,
    INVALID
  };

  inline const char* toString(Type mode) {
    #define CASE(x) case x: return #x
    switch (mode) {
    CASE(Vertical);
    CASE(Horizontal);
    CASE(FourScreen);
    CASE(SingleScreenLo);
    CASE(SingleScreenHi);
    CASE(INVALID);
    }
    #undef CASE
    return nullptr;
  }
}
