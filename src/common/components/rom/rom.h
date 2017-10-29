#pragma once

#include "common/interfaces/memory.h"
#include "common/util.h"

// Dead simple 16 bit ROM ADT
// (max 64K of ROM)
class ROM final : public Memory {
private:
  u8* rom;
  u32 size;
public:
  ROM(u32 ram_size, const u8* content);
  ~ROM();

  // <Memory>
  u8 read(u16 addr)       override;
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // </Memory>

  // Also provide a const read method (for when there is a `const ROM` type)
  u8 read(u16 addr) const;
};
