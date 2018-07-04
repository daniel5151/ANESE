#pragma once

#include "common/util.h"
#include "nes/interfaces/memory.h"

#include "common/serializable.h"

namespace JOY_Standard_Button {
  enum Type : unsigned {
    A      = 0x01,
    B      = 0x02,
    Select = 0x04,
    Start  = 0x08,
    Up     = 0x10,
    Down   = 0x20,
    Left   = 0x40,
    Right  = 0x80,
  };
}

// https://wiki.nesdev.com/w/index.php/Standard_controller
class JOY_Standard final : public Memory, public Serializable {
private:
  bool strobe = false;
  u8 curr_btn = 0x01;
  u8 buttons  = 0x00;

  const char* label;

  SERIALIZE_START(3, "JOY_Standard")
    SERIALIZE_POD(strobe)
    SERIALIZE_POD(curr_btn)
    SERIALIZE_POD(buttons)
  SERIALIZE_END(3)

public:
  JOY_Standard(const char* label = "?");

  // <Memory>
  u8 read(u16 addr)            override;
  u8 peek(u16 addr) const      override;
  void write(u16 addr, u8 val) override;
  // </Memory>

  void set_button(JOY_Standard_Button::Type btn, bool active);
  bool get_button(JOY_Standard_Button::Type btn) const;
};
