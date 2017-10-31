#pragma once

#include "nes/generic/ram/ram.h"
#include "nes/generic/rom/rom.h"
#include "common/util.h"
#include "mapper.h"

// http://wiki.nesdev.com/w/index.php/NROM
class Mapper_000 final : public Mapper {
private:
  // const INES& rom_file; // inherited from Mapper

  // PGR ROM
  const ROM* lo_rom; // 0x8000 ... 0xBFFF
  const ROM* hi_rom; // 0xC000 ... 0xFFFF

  // CHR ROM / RAM
  const ROM* chr_rom;
        RAM* chr_ram;

public:
  Mapper_000(const INES& rom_file);
  ~Mapper_000();

  // <Memory>
  u8 read(u16 addr)       override;
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // <Memory/>
};
