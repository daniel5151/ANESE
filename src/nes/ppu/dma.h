#pragma once

#include "common/interfaces/memory.h"
#include "common/util.h"

// Thin class that gives PPU access to CPU WRAM for Direct Memory Access (DMA)
class DMA final {
private:
  Memory& cpu_mmu;

  bool in_dma;

  u16 addr; // What CPU addr to base read from
  u16 step; // How many transfers have occured (from 0x00 to 0xFF)

public:
  ~DMA() = default;
  DMA(Memory& cpu_mmu);

  void start(u8 page);
  u8   transfer();

  bool isActive() const;
};
