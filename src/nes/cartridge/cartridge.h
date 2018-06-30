#pragma once

#include "mapper.h"
#include "rom_file.h"

#undef NO_ERROR // TODO: track down nebulous Windows.h include that defines this

// A thin class that owns a ROM_File and it's associated mapper
class Cartridge {
private:
  const ROM_File* const rom_file;
        Mapper*   const mapper;

public:
  ~Cartridge() {
    delete this->mapper;
    delete this->rom_file;
  }

  Cartridge(ROM_File* rom_file)
  : rom_file(rom_file)
  , mapper(Mapper::Factory(rom_file))
  {}

  enum class Status {
    NO_ERROR = 0,
    BAD_MAPPER,
    BAD_DATA
  };

  Status status() {
    if (this->rom_file == nullptr) return Status::BAD_DATA;
    if (this->mapper   == nullptr) return Status::BAD_MAPPER;
    return Status::NO_ERROR;
  }

  const ROM_File* get_rom_file() { return this->rom_file; }
  Mapper*         get_mapper()   { return this->mapper;   }
};
