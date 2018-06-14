#pragma once

#include "common/util.h"
#include "nes/interfaces/memory.h"

#include "common/serializable.h"

class JOY_Zapper final : public Memory, public Serializable {
private:
  bool trigger = false;
  bool light = false;

  const char* label;

  SERIALIZE_START(2, "JOY_Zapper")
    SERIALIZE_POD(trigger)
    SERIALIZE_POD(light)
  SERIALIZE_END(2)

public:
  JOY_Zapper(const char* label = "?");

  // <Memory>
  u8 read(u16 addr)            override;
  u8 peek(u16 addr) const      override;
  void write(u16 addr, u8 val) override;
  // </Memory>

  void set_trigger(bool active);
  void set_light(bool active);

  bool get_trigger() const;
  bool get_light() const;
};
