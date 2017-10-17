#pragma once

#include "common/util.h"

// 16 bit memory interface
class IMemory {
public:
  virtual u8 peek(u16 addr) const = 0; // read, but without the side-effects
  virtual u8 read(u16 addr) = 0;
  virtual void write(u16 addr, u8 val) = 0;
};
