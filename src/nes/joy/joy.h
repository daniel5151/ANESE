#pragma once

#include "common/util.h"
#include "common/interfaces/memory.h"

// Joypad router
class JOY : public Memory {
private:
  Memory* joy [2];

public:
  ~JOY() = default; // doesn't own joypads
  JOY();

  // <Memory>
  u8 read(u16 addr)            override;
  u8 peek(u16 addr) const      override;
  void write(u16 addr, u8 val) override;
  // </Memory>

  void attach_joy(uint port, Memory* joy);
  void detach_joy(uint port);
};
