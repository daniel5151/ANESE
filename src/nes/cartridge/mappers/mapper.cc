#include "mapper.h"

#include "mapper_000.h"
#include "mapper_001.h"

Mapper* Mapper::Factory(const ROM_File& rom_file) {
  if (rom_file.is_valid == false)
    return nullptr;

  switch (rom_file.meta.mapper) {
  case 0: return new Mapper_000(rom_file);
  case 1: return new Mapper_001(rom_file);
  }

  return nullptr;
}
