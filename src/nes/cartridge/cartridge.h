#pragma once

#include "common/interfaces/memory.h"
#include "common/util.h"
#include "formats/ines.h"
#include "mappers/mapper.h"

// Contains Mapper and iNES cartridge
class Cartridge final : public Memory {
private:
  const INES* rom_data;
  Mapper* mapper;

public:
  ~Cartridge();
  Cartridge(const u8* data, u32 data_len);

  u8 read(u16 addr) override;
  void write(u16 addr, u8 val) override;

  bool isValid() const;

  // Critical importance
  void blowOnContacts() const;
};
