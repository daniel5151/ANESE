#pragma once

#include "common/util.h"
#include "mapper.h"

// http://wiki.nesdev.com/w/index.php/NROM
class Mapper_000 final : public Mapper {
public:
  Mapper_000(const INES& rom_file);
  ~Mapper_000();

  // <Memory>
  u8 read(u16 addr)       override;
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // <Memory/>
};
