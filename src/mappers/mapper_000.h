#pragma once

#include "mapper.h"
#include "util.h"

// http://wiki.nesdev.com/w/index.php/NROM
class Mapper_000 final : public Mapper {
public:
  Mapper_000(const INES& rom_file);
  ~Mapper_000();

  uint8 read(uint16 addr) override;
  void write(uint16 addr, uint8 val) override;
};
