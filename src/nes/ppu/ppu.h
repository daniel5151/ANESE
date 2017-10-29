#pragma once

#include "common/util.h"
#include "common/interfaces/memory.h"
#include "common/bitfield.h"

class PPU {
private:
  Memory& mem;

public:
  ~PPU();
  PPU(Memory& mem);
};
