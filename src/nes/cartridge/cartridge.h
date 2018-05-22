#pragma once

#include "common/util.h"
#include "file_formats/rom_file.h"
#include "mappers/mapper.h"
#include "nes/interfaces/memory.h"

// Container that facilitates conversion from raw ROM data into a ready-to-go
// cartridge (i.e: data + mapper)
class Cartridge final : public Memory {
public:
  enum class Error {
    NO_ERROR,
    BAD_MAPPER,
    BAD_DATA
  };

private:
  const ROM_File rom_file; // Structured container for ROM data
  Mapper* const  mapper;   // iNES mapper hardware, varies game-per-game

public:
  ~Cartridge();
  Cartridge(const u8* data, u32 data_len);

  // <Memory>
  u8 read(u16 addr)       override;
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // </Memory>

  // Mapper interface
  const char* mapper_name()   const { return this->mapper->mapper_name();   };
        uint  mapper_number() const { return this->mapper->mapper_number(); };

  Mirroring::Type mirroring() const { return this->mapper->mirroring(); }

  void cycle() { this->mapper->cycle(); }

  // Cartridge specific functions
  Error getError() const; // Returns error code from construction
  void  blowOnContacts(); // Critical importance

  const ROM_File& getROM_File() const;
};
