#pragma once

#include "util.h"
#include "memory.h"
#include "ram.h"
#include "cartridge.h"

// CPU Memory Map (MMU)
// NESdoc.pdf
// https://wiki.nesdev.com/w/index.php/CPU_memory_map
// https://wiki.nesdev.com/w/index.php/2A03
class CPU_MMU final : public Memory {
private:
  // Fixed Referenced (these will never be invalidated)
  Memory& ram;
  Memory& ppu;
  Memory& apu;
  Memory& dma;
  Memory& joy;

  // ROM is subject to change
  Memory* rom;
public:
  // No Destructor, since no owned resources
  CPU_MMU(
    Memory& ram,
    Memory& ppu,
    Memory& apu,
    Memory& dma,
    Memory& joy,

    Memory* rom
  );

  u8 read(u16 addr) override;
  void write(u16 addr, u8 val) override;

  void addCartridge(Memory* cart);
  void removeCartridge();
};
