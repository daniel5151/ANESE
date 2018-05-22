#pragma once

#include "nes/generic/ram/ram.h"
#include "nes/generic/rom/rom.h"
#include "common/util.h"
#include "mapper.h"

// http://wiki.nesdev.com/w/index.php/NROM
class Mapper_000 final : public Mapper {
private:
  // CPU Memory Space
  ROM* prg_lo;     // 0x8000 ... 0xBFFF - Fixed
  ROM* prg_hi;     // 0xC000 ... 0xFFFF - Fixed

  // PPU Memory Space
  Memory* chr_mem; // 0x0000 ... 0x1FFF - Fixed

  Mirroring::Type mirror_mode;

public:
  Mapper_000(const ROM_File& rom_file);
  ~Mapper_000();

  // <Memory>
  u8 read(u16 addr)       override;
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // <Memory/>

  Mirroring::Type mirroring() const override { return this->mirror_mode; };
};
