#pragma once

#include "common/interfaces/memory.h"
#include "common/util.h"

// CPU WRAM to PPU OAM Direct Memory Access (DMA) Unit
// http://wiki.nesdev.com/w/index.php/PPU_programmer_reference#OAMDMA
class DMA final : public Memory {
private:
  Memory& cpu_wram;
  Memory& ppu_oam;

public:
  ~DMA() = default;
  DMA(Memory& cpu_wram, Memory& ppu_oam);

  // <Memory>
  u8 read(u16 addr)       override; // Not a valid operation
  u8 peek(u16 addr) const override; // Not a valid operation
  void write(u16 addr, u8 page) override; // Fill PPU OAM with a given CPU page
  // <Memory/>
};
