#pragma once

#include "common/util.h"
#include "nes/cartridge/mapper.h"
#include "nes/interfaces/memory.h"

// PPU Memory Map (MMU)
// NESdoc.pdf
// http://wiki.nesdev.com/w/index.php/PPU_memory_map
class PPU_MMU final : public Memory {
private:
  // Fixed References (these will never be invalidated)
  Memory& ciram; // PPU internal VRAM
  Memory& pram;  // Palette RAM

  // Changing References
  Mapper* cart = nullptr; // Plugged in cartridge
  
  // VRAM is usually == &ciram, but it == cart when FourScreen mirroring
  Memory* vram;

  // Nametable offsets + Mirror mode
  static constexpr uint nt_mirroring [5][4] = {
    /* Vertical       */ { 0, 1, 0, 1 },
    /* Horizontal     */ { 0, 0, 1, 1 },
    /* FourScreen     */ { 0, 1, 2, 3 },
    /* SingleScreenLo */ { 0, 0, 0, 0 },
    /* SingleScreenHi */ { 1, 1, 1, 1 }
  };
  Mirroring::Type mirroring;
  const uint* nt; // pointer to current offsets for current nametable mirroring
  uint fix_4s;    // Set to 0, unless mirroring is FourScreen, then it's 0x2000 

  u16 nt_mirror(u16 addr) const; // helper for doing nametable address routing

  void set_mirroring();

public:
  PPU_MMU() = delete;
  PPU_MMU(
    Memory& ciram,
    Memory& pram
  );

  // <Memory>
  u8 read(u16 addr)       override;
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // <Memory/>

  void loadCartridge(Mapper* cart);
  void removeCartridge();
};
