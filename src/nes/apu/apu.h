#pragma once

#include "common/bitfield.h"
#include "common/util.h"
#include "nes/interfaces/memory.h"

#include "nes/wiring/interrupt_lines.h"

#include "nes/interfaces/serializable.h"

#include <Nes_Apu.h>
#include <apu_snapshot.h>
static apu_snapshot_t savestate; // hack

// NES APU
// Part of the NES RP2A03
class APU final : public Memory, public Serializable {
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

  SERIALIZE_START(3, "Blaarg APU")
    SERIALIZE_POD(savestate) // hack
    SERIALIZE_POD(cycles)
    SERIALIZE_POD(frame_cycles)
  SERIALIZE_END(3)

  virtual Chunk* serialize() const override {
    this->blargg_apu.save_snapshot(&savestate);
    return this->Serializable::serialize();
  }
  virtual const Chunk* deserialize(const Chunk* c) override {
    c = this->Serializable::deserialize(c);
    this->blargg_apu.load_snapshot(savestate);
    return c;
  }

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
