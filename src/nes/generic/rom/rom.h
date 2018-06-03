#pragma once

#include "common/util.h"
#include "nes/interfaces/memory.h"

// Dead simple 16 bit ROM ADT
// (max 64K of ROM)
// Provides bounds-checked view into existing memory (zero heap allocation)
class ROM final : public Memory {
private:
  const u8* rom;
  uint      size;

  const char* label;

public:
  ROM() = delete;
  ROM(uint rom_size, const u8* content, const char* label = "?");

  // <Memory>
  u8 read(u16 addr)       override;
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // </Memory>

  // Also provide a const read method (for when there is a `const ROM` type)
  u8 read(u16 addr) const;
};
