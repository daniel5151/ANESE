#pragma once

#include "common/util.h"
#include "nes/cartridge/mapper.h"
#include "nes/interfaces/memory.h"

#include "nes/interfaces/serializable.h"

// PPU Memory Map (MMU)
// NESdoc.pdf
// http://wiki.nesdev.com/w/index.php/PPU_memory_map
class PPU_MMU final : public Memory, public Serializable {
private:
  // Fixed References (these will never be invalidated)
  Memory& ciram; // PPU internal VRAM
  Memory& pram;  // Palette RAM

  // Changing References
  Mapper* cart; // Plugged in cartridge
  Memory* vram; // VRAM can be ciram or cart (depending on mirroring)

  // Nametable offsets + Mirror mode
  Mirroring::Type mirroring;
  u16 nt_0;
  u16 nt_1;
  u16 nt_2;
  u16 nt_3;

  SERIALIZE_START(5, "PPU_MMU")
    SERIALIZE_POD(mirroring)
    SERIALIZE_POD(nt_0)
    SERIALIZE_POD(nt_1)
    SERIALIZE_POD(nt_2)
    SERIALIZE_POD(nt_3)
  SERIALIZE_END(5)

  void set_mirroring();

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

  void loadCartridge(Mapper* cart);
  void removeCartridge();
};
