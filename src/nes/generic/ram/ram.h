#pragma once

#include "common/util.h"
#include "nes/interfaces/memory.h"

#include "nes/interfaces/serializable.h"

// Dead simple 16 bit RAM ADT
// (max 64K of RAM)
class RAM final : public Memory, public Serializable {
private:
  u8*  ram;
  uint size;

  const char* label;

  SERIALIZE_START(2, "RAM")
    SERIALIZE_POD(size)
    SERIALIZE_ARRAY_VARIABLE(ram, size)
  SERIALIZE_END(2)

public:
  RAM(uint ram_size, const char* label = "?");
  ~RAM();

  // <Memory>
  u8 read(u16 addr)       override;
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // </Memory>

  void clear();
};
