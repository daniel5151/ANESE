#include "mapper.h"

#include "mappers/mapper_000.h"
#include "mappers/mapper_001.h"
#include "mappers/mapper_002.h"
#include "mappers/mapper_003.h"
#include "mappers/mapper_004.h"
#include "mappers/mapper_007.h"

Mapper* Mapper::Factory(const ROM_File* rom_file) {
  if (rom_file == nullptr)
    return nullptr;

  switch (rom_file->meta.mapper) {
  case 0: return new Mapper_000(*rom_file);
  case 1: return new Mapper_001(*rom_file);
  case 2: return new Mapper_002(*rom_file);
  case 3: return new Mapper_003(*rom_file);
  case 4: return new Mapper_004(*rom_file);
  case 7: return new Mapper_007(*rom_file);
  }

  return nullptr;
}
