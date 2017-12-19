#pragma once

#include "common/util.h"
#include "formats/ines.h"
#include "mappers/mapper.h"
#include "nes/interfaces/memory.h"

// Contains Mapper and iNES cartridge
class Cartridge final : public Memory {
public:
  enum class Mirroring {
    Vertical,
    Horizontal,
    FourScreen
  };

  enum class Error {
    NO_ERROR,
    BAD_MAPPER,
    BAD_DATA
  };

private:
  const INES*   const rom_data; // ROM file data does not change
        Mapper* const mapper;

  Mirroring mirroring_type;

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

  Mirroring mirroring() const; // get nametable mirroring type

  // Critical importance
  void blowOnContacts() const;
};
