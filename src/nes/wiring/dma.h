#pragma once

#include "common/interfaces/memory.h"
#include "common/util.h"

// Class that facilitates CPU WRAM to PPU OAM Direct Memory Access (DMA)
class DMA final {
private:
  Memory& cpu_wram;
  Memory& ppu_oam;

  bool in_dma;

  u8   page; // What CPU page to read from
  u16  step; // How many transfers have occured (from 0x00 to 0xFF)

public:
  ~DMA() = default;
  DMA(Memory& cpu_wram, Memory& ppu_oam);

  void start(u8 page);
  void transfer();

  bool isActive() const;
};
