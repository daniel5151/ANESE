#pragma once

#include "memory.h"
#include "util.h"

// Dead simple 16 bit RAM ADT
// (max 64K of RAM)
class RAM final : public Memory {
private:
  uint8* ram;
  uint32 size;
public:
  RAM(uint32 ram_size);
  ~RAM();

  uint8 read(uint16 addr) override;
  void write(uint16 addr, uint8 val) override;
};
