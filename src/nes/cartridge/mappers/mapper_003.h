#pragma once

#include "nes/generic/ram/ram.h"
#include "nes/generic/rom/rom.h"
#include "common/bitfield.h"
#include "common/util.h"
#include "../mapper.h"

// https://wiki.nesdev.com/w/index.php/INES_Mapper_003
class Mapper_003 final : public Mapper {
private:
  // CPU Memory Space
  ROM* prg_lo;     // 0x8000 ... 0xBFFF - Fixed
  ROM* prg_hi;     // 0xC000 ... 0xFFFF - Fixed

  // PPU Memory Space
  Memory* chr_mem; // 0x0000 ... 0x1FFF - Switchable

  struct { // Registers
    // Bank select - 0x8000 ... 0xFFFF
    // 7  bit  0
    // ---- ----
    // cccc ccCC
    // |||| ||||
    // ++++-++++- Select 8 KB CHR ROM bank for PPU $0000-$1FFF
    // CNROM only implements the lowest 2 bits, capping it at 32 KiB CHR.
    // Other boards may implement 4 or more bits for larger CHR.
    //
    // ANESE will just give it the entire 8 bits...
    u8 bank_select;
  } reg;

  Mirroring::Type mirror_mode;

  void update_banks() override;

  void reset() override;

  SERIALIZE_PARENT(Mapper)
  SERIALIZE_START(2, "Mapper_003")
    SERIALIZE_POD(mirror_mode)
    SERIALIZE_POD(reg)
  SERIALIZE_END(2)

public:
  Mapper_003(const ROM_File& rom_file);

  // <Memory>
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // <Memory/>

  Mirroring::Type mirroring() const override { return this->mirror_mode; };
};
