#pragma once

#include "common/util.h"
#include "nes/interfaces/memory.h"

class JOY_Standard final : public Memory {
private:
  bool strobe = false;
  uint curr_btn = 0; // from 0 - 7

  union {
    u8 step [8];
    struct {
      u8 A;
      u8 B;
      u8 Select;
      u8 Start;
      u8 Up;
      u8 Down;
      u8 Left;
      u8 Right;
    } btn;
  } buttons;

  const char* label;

public:
  JOY_Standard(const char* label = "?");

  // <Memory>
  u8 read(u16 addr)            override;
  u8 peek(u16 addr) const      override;
  void write(u16 addr, u8 val) override;
  // </Memory>

  void set_button(const char* btn, bool state);
};
