#pragma once

#include "cartridge/cartridge.h"
#include "common/interfaces/memory.h"
#include "common/util.h"
#include "ram/ram.h"

// CPU IMemory Map (MMU)
// NESdoc.pdf
// https://wiki.nesdev.com/w/index.php/CPU_memory_map
// https://wiki.nesdev.com/w/index.php/2A03
class CPU_MMU final : public IMemory {
private:
  // Fixed Referenced (these will never be invalidated)
  IMemory& ram;
  IMemory& ppu;
  IMemory& apu;
  IMemory& dma;
  IMemory& joy;

  // ROM is subject to change
  IMemory* rom;
public:
  // No Destructor, since no owned resources
  CPU_MMU(
    IMemory& ram,
    IMemory& ppu,
    IMemory& apu,
    IMemory& dma,
    IMemory& joy,

    IMemory* rom
  );

  // <IMemory>
  u8 read(u16 addr)       override;
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // <IMemory/>

  void addCartridge(IMemory* cart);
  void removeCartridge();
};
