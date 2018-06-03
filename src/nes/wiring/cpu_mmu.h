#pragma once

#include "common/util.h"
#include "nes/cartridge/mapper.h"
#include "nes/interfaces/memory.h"

// CPU Memory Map (MMU)
// NESdoc.pdf
// https://wiki.nesdev.com/w/index.php/CPU_memory_map
// https://wiki.nesdev.com/w/index.php/2A03
class CPU_MMU final : public Memory {
private:
  // Fixed References (these will never be invalidated)
  Memory& ram;
  Memory& ppu;
  Memory& apu;
  Memory& joy;

  // Changing References
  Mapper* cart;
public:
  CPU_MMU() = delete;
  CPU_MMU(
    Memory& ram,
    Memory& ppu,
    Memory& apu,
    Memory& joy
  );

  // <Memory>
  u8 read(u16 addr)       override;
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // <Memory/>

  void loadCartridge(Mapper* cart);
  void removeCartridge();
};
