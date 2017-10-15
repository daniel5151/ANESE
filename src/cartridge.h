#pragma once

#include "ines.h"
#include "mappers/mapper.h"
#include "memory.h"
#include "util.h"

// Contains Mapper and iNES cartridge
class Cartridge final : public Memory {
private:
  const INES* rom_data;
  Mapper* mapper;

public:
  ~Cartridge();
  Cartridge(const uint8* data, uint32 data_len);

  uint8 read(uint16 addr) override;
  void write(uint16 addr, uint8 val) override;

  bool isValid() const;
};
