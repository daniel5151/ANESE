#pragma once

#include "common/util.h"
#include "common/interfaces/memory.h"
#include "common/bitfield.h"

class PPU {
private:
  u8 frame [240 * 256 * 4]; // Pixel buffer

  Memory& mem;

  u32 cycles;

  struct {
    u16 x;
    u16 y;
  } scan;

public:
  ~PPU();
  PPU(Memory& mem);

  void power_cycle();
  void reset();

  void cycle();

  const u8* getFrame() const;
};
