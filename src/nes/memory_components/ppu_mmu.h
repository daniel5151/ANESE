#pragma once

#include "common/interfaces/memory.h"
#include "common/util.h"
#include "nes/cartridge/cartridge.h"

// PPU Memory Map (MMU)
// NESdoc.pdf
// http://wiki.nesdev.com/w/index.php/PPU_memory_map
class PPU_MMU final : public Memory {
private:
  // Fixed References (these will never be invalidated)
  Memory& ciram; // PPU internal VRAM
  Memory& pram;  // Palette RAM

  // Changing References
  Cartridge* cart;
  Memory* vram; // what VRAM to access changes based on mirroring mode

  // Nametable offsets
  u16 nt_0;
  u16 nt_1;
  u16 nt_2;
  u16 nt_3;

public:
  ~PPU_MMU() = default; // no owned resources
  PPU_MMU(
    Memory& ciram,
    Memory& pram
  );

  // <Memory>
  u8 read(u16 addr)       override;
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // <Memory/>

  void loadCartridge(Cartridge* cart);
  void removeCartridge();
};
