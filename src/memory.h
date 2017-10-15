#pragma once

#include "util.h"

// 16 bit memory interface
class Memory {
public:
  virtual uint8 read(uint16 addr) = 0;
  virtual void write(uint16 addr, uint8 val) = 0;
};
