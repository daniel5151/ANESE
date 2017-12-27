#pragma once

#include "common/util.h"
#include "file_formats/rom_file.h"
#include "mappers/mapper.h"
#include "nes/interfaces/memory.h"

// Converts raw ROM data into a proper, ready to use Cartridge
class Cartridge final : public Memory {
public:
  enum class Error {
    NO_ERROR,
    BAD_MAPPER,
    BAD_DATA
  };

private:
  const ROM_File rom_file; // Structured container for raw ROM data
  Mapper* mapper;          // iNES mapper hardware, varies game-per-game

public:
  ~Cartridge();
  Cartridge(const u8* data, u32 data_len);

  // <Memory>
  u8 read(u16 addr)       override;
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // </Memory>

  Error getError() const;
  uint  getMapper() const;

  Mirroring::Type mirroring() const;

  void cycle();

  // Critical importance
  void blowOnContacts() const;
};
