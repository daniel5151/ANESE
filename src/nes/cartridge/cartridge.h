#pragma once

#include "common/interfaces/memory.h"
#include "common/util.h"
#include "formats/ines.h"
#include "mappers/mapper.h"
#include "nes/ppu/ppu.h"

// Contains Mapper and iNES cartridge
class Cartridge final : public Memory {
public:
  enum class Mirroring {
    Vertical,
    Horizontal,
    FourScreen
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

  bool isValid() const;

  Mirroring mirroring() const; // get nametable mirroring type

  // Critical importance
  void blowOnContacts() const;
};
