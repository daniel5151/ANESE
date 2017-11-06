#pragma once

#include "common/interfaces/memory.h"
#include "common/util.h"

// Thin class that gives PPU access to CPU WRAM for Direct Memory Access (DMA)
class DMA final {
private:
  Memory& cpu_wram;

  bool in_dma;

  u8   page; // What CPU page to read from
  uint step; // How many transfers have occured (from 0x00 to 0xFF)

public:
  ~DMA() = default;
  DMA(Memory& cpu_wram);

  void start(u8 page);
  u8   transfer();

  bool isActive() const;
};
