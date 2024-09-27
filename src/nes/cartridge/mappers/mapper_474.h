#pragma once

#include "nes/generic/ram/ram.h"
#include "nes/generic/rom/rom.h"
#include "common/util.h"
#include "../mapper.h"

#include "common/serializable.h"

// https://www.nesdev.org/wiki/NES_2.0_Mapper_474
class Mapper_474 final : public Mapper {
private:
  // CPU Memory Space
  RAM  prg_ram; // 0x4020 ... 0x5FFF - Fixed RAM (only submapper 3)
  ROM* prg_extra; // 0x4020 ... 0x7FFF - Fixed
  ROM* prg_lo; // 0x8000 ... 0xBFFF - Fixed
  ROM* prg_hi; // 0xC000 ... 0xFFFF - Fixed

  // PPU Memory Space
  Memory* chr_mem; // 0x0000 ... 0x1FFF - Fixed

  Mirroring::Type mirror_mode;
  bool has_battery;

  SERIALIZE_PARENT(Mapper)
    SERIALIZE_START(1, "Mapper_474")
    SERIALIZE_POD(mirror_mode)
    SERIALIZE_END(1)

    void update_banks() override;

  // Nothing to reset...
  void reset() override {}

public:
  Mapper_474(const ROM_File& rom_file);

  // <Memory>
  u8 read(u16 addr)       override;
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // <Memory/>

  Mirroring::Type mirroring() const override { return this->mirror_mode; };
};
