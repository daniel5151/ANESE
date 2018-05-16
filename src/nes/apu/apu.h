#pragma once

#include "common/bitfield.h"
#include "common/util.h"
#include "nes/interfaces/memory.h"

#include "nes/wiring/interrupt_lines.h"

#include <Nes_Apu.h>

// NES APU
// Part of the NES RP2A03
class APU final : public Memory {
private:
  /*----------  Hardware  ----------*/

  InterruptLines& interrupt;
  Memory& mem;

  // thank mr. blargg
  Nes_Apu       blargg_apu;
  Blip_Buffer   blargg_buf;

  short audiobuff [4096];

  /*----------  Emulation Vars  ----------*/

  uint cycles;       // Total Cycles elapsed
  uint frame_cycles; // Frame Cycles elapsed

public:
  ~APU();
  APU(Memory& mem, InterruptLines& interrupt);

  // <Memory>
  u8 read(u16 addr)            override;
  u8 peek(u16 addr) const      override;
  void write(u16 addr, u8 val) override;
  // <Memory/>

  void power_cycle();
  void reset();

  void cycle();

  void getAudiobuff(short*& samples, uint& len);

  void set_speed(double speed);
};
