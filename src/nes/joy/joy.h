#pragma once

#include "common/util.h"
#include "nes/interfaces/memory.h"

// Joypad "router"
// Doesn't construct controllers, simply accepts controllers, and maps them to
//  their appropriate memory address
class JOY final : public Memory {
private:
  Memory* joy [2] = { nullptr };

public:
  ~JOY() = default; // doesn't own joypads
  JOY() = default;

  // <Memory>
  u8 read(u16 addr)            override;
  u8 peek(u16 addr) const      override;
  void write(u16 addr, u8 val) override;
  // </Memory>

  void attach_joy(uint port, Memory* joy);
  void detach_joy(uint port);
};
