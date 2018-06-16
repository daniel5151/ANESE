#pragma once

#include "nes/generic/ram/ram.h"
#include "nes/generic/rom/rom.h"
#include "common/bitfield.h"
#include "common/util.h"
#include "../mapper.h"

// https://wiki.nesdev.com/w/index.php/AxROM
class Mapper_007 final : public Mapper {
private:
  // CPU Memory Space
  // 0x8000 ... 0xFFFF - 32 KB switchable PRG ROM bank
  // Split into two ROM* to handle case where only 16 KB of ROM exists
  ROM* prg_lo; // 0x8000 ... 0xBFFF - 32 KB switchable PRG ROM bank
  ROM* prg_hi; // 0xC000 ... 0xFFFF - 32 KB switchable PRG ROM bank

  // PPU Memory Space
  Memory* chr_mem; // 0x0000 ... 0x1FFF

  struct { // Registers
    // Bank select - 0x8000 ... 0xFFFF
    // 7  bit  0
    // ---- ----
    // xxxM xPPP
    //    |  |||
    //    |  +++- Select 32 KB PRG ROM bank for CPU $8000-$FFFF
    //    +------ Select 1 KB VRAM page for all 4 nametables
    union {
      u8 val;
      BitField<4>    vram_page;
      BitField<0, 3> prg_bank;
    } bank_select;
  } reg;

  void update_banks() override;

  void reset() override;

  SERIALIZE_PARENT(Mapper)
  SERIALIZE_START(1, "Mapper_007")
    SERIALIZE_POD(reg)
  SERIALIZE_END(1)

public:
  Mapper_007(const ROM_File& rom_file);

  // <Memory>
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // <Memory/>

  Mirroring::Type mirroring() const override;
};
