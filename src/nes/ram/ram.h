#pragma once

#include "common/interfaces/memory.h"
#include "common/util.h"

// Dead simple 16 bit RAM ADT
// (max 64K of RAM)
class RAM final : public Memory {
private:
  u8* ram;
  u32 size;
public:
  RAM(u32 ram_size);
  ~RAM();

  u8 read(u16 addr) override;
  void write(u16 addr, u8 val) override;
};
