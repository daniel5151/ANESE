#pragma once

#include "common/bitfield.h"
#include "common/util.h"
#include "nes/interfaces/memory.h"

#include "nes/wiring/interrupt_lines.h"

//
// CURRENTLY A STUB
// ONLY BASICS IMPLEMENTED FOR TIMING PURPOSES
//

// NES APU
// Part of the NES RP2A03
class APU final : public Memory {
private:
  /*----------  Hardware  ----------*/

  InterruptLines& interrupt;
  Memory& mem;

  struct { // Registers
    // -- Channel Control Registers -- //

    // 0x4000 - 0x4003 - Pulse 1 channel
    // 0x4004 - 0x4007 - Pulse 2 channel
    struct {
      union {
        u8 byte_0;
        BitField<6, 2> duty;
        BitField<5>    disable_len_counter;
        BitField<4>    constant_vol;
        BitField<0, 4> envelope_vol;
      };
      union {
        u8 byte_1;
        BitField<7>    sweep_enabled;
        BitField<4, 3> period;
        BitField<3>    negative;
        BitField<0, 3> shift_count;
      };
      union {
        u8 byte_2;
        BitField<0, 8> timer_lo;
      };
      union {
        u8 byte_3;
        BitField<3, 5> length_counter;
        BitField<0, 3> timer_hi;
      };
    } pulse1, pulse2;

    // 0x4008 - 0x400B - Triangle channel
    struct {
      union {
        u8 byte_0;
        BitField<7>    disable_len_counter;
        BitField<0, 7> len_counter_reload_val;
      };
      union {
        u8 byte_2;
        BitField<0, 8> timer_lo;
      };
      union {
        u8 byte_3;
        BitField<3, 5> length_counter;
        BitField<0, 3> timer_hi;
      };
    } triangle;

    // 0x400C - 0x400F - Noise channel
    struct {
      union {
        u8 byte_0;
        BitField<5>    disable_len_counter;
        BitField<4>    constant_vol;
        BitField<0, 4> envelope_vol;
      //BitField<6, 2> unused;
      };
      union {
        u8 byte_2;
        BitField<7>    loop;
      //BitField<4, 3> unused;
        BitField<0, 4> period;
      };
      union {
        u8 byte_3;
        BitField<3, 5> length_counter;
      //BitField<0, 3> unused;
      };
    } noise;

    // 0x4010 - 0x4014 - DMC channel
    struct {
      union {
        u8 byte_0;
        BitField<7>    irq_enable;
        BitField<6>    loop;
      //BitField<4, 2> unused;
        BitField<0, 4> freq_index;
      };
      union {
        u8 byte_1;
      //BitField<7>    unused;
        BitField<0, 7> direct_load;
      };
      union {
        u8 byte_2;
        BitField<0, 8> sample_addr;
      };
      union {
        u8 byte_3;
        BitField<0, 8> sample_len;
      };
    } dmc;

    // -- General Control / Status Registers -- //

    // 0x4015 - Control (write) / Status (read)
    union {
      u8 val;
      BitField<7> dmc_irq;
      BitField<6> frame_irq;
    //BitField<5> unused;
      BitField<4> dmc;
      BitField<3> noise;
      BitField<2> triangle;
      BitField<1> pulse2;
      BitField<0> pulse1;
    } state;

    // 0x4017 - Frame Counter
    union {
      u8 val;
      BitField<7> five_frame_seq;
      BitField<6> disable_frame_irq;
    //BitField<0, 6> unused;
    } frame_counter;
  } reg;

  /*----------  Emulation Vars  ----------*/

  uint cycles;   // Cycles elapsed
  uint seq_step; // Frame Sequence Step

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
};
