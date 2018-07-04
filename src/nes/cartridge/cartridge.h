#pragma once

#include "mapper.h"
#include "rom_file.h"

// Container for ROM_File* and associated mapper.
// Takes ownership of any ROM_File passed to it, and handles Mapper detection
//  and construction
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
    CART_NO_ERROR = 0,
    CART_BAD_MAPPER,
    CART_BAD_DATA
  };

  Status status() {
    if (this->rom_file == nullptr) return Status::CART_BAD_DATA;
    if (this->mapper   == nullptr) return Status::CART_BAD_MAPPER;
    return Status::CART_NO_ERROR;
  }

  const ROM_File* get_rom_file() { return this->rom_file; }
  Mapper*         get_mapper()   { return this->mapper;   }
};
