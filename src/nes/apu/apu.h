#pragma once

#include "common/bitfield.h"
#include "common/util.h"
#include "nes/interfaces/memory.h"

#include "nes/wiring/interrupt_lines.h"

#include "common/serializable.h"

// NES APU
// Part of the NES RP2A03
class APU final : public Memory, public Serializable {
private:
  /*----------  Hardware  ----------*/

  InterruptLines& interrupt;
  Memory& mem;

  struct Envelope {
    bool enabled;
    bool loop;
    bool start;
    u8   period;
    u8   val;
    u8   decay;

    void clock();
  };

  struct Channels {
    // -- Channel Control Registers -- //

    // 0x4000 - 0x4003 - Pulse 1 channel
    // 0x4004 - 0x4007 - Pulse 2 channel
    struct Pulse {
      bool enabled;

      union {
        u8 byte_0;
        BitField<6, 2> duty;
        BitField<5>    halt;
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
    struct Triangle {
      bool enabled;

      union {
        u8 byte_0;
        BitField<7>    halt;
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
    } tri;

    // 0x400C - 0x400F - Noise channel
    struct Noise {
      bool enabled; // Set though 0x4015

      Envelope envelope; // 0x400C:0-4 (period) and 0x400C:5 (enable)
      u16  timer_period; // 0x400E:0-3 (through table lookup)
      bool mode;         // 0x400E:7
      u8   len_count;    // 0x400F:3-7
      bool len_count_enable; // 0x400C:5

      // Internals
      u16 sr;
      u16 timer_val;

      void clock_timer();

      u8 out_volume;
      u16 output() const;
    } noise;

    // 0x4010 - 0x4014 - DMC channel
    struct DMC {
      bool enabled;

      union {
        u8 byte_0;
        BitField<7>    irq_enabled;
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
  } chan;

  // 0x4015 - Control (write) / Status (read)
  // Not a physical register, value generated per-access
  union StateReg {
    u8 val;
    BitField<7> dmc_irq;
    BitField<6> frame_irq;
  //BitField<5> unused;
    BitField<4> dmc;
    BitField<3> noise;
    BitField<2> tri;
    BitField<1> pulse2;
    BitField<0> pulse1;
  };

  // 0x4017 - Frame Counter
  union FrameCounter {
    u8 val;
    BitField<7> five_frame_seq;
    BitField<6> inhibit_irq;
  //BitField<0, 6> unused;
  } frame_counter;

  /*----------  Emulation Vars  ----------*/

  uint cycles;   // Total Cycles elapsed
  uint seq_step; // Frame Sequence Step

  struct {
    uint i = 0;
    short data [4096] = {0};
  } audiobuff;

  SERIALIZE_START(4, "APU")
    SERIALIZE_POD(chan)
    SERIALIZE_POD(frame_counter)
    SERIALIZE_POD(cycles)
    SERIALIZE_POD(seq_step)
  SERIALIZE_END(4)

  /*----------  Helpers  ----------*/

  void clock_envelopes();
  void clock_timers();
  void clock_length_counters();

public:
  ~APU();
  APU() = delete;
  APU(Memory& mem, InterruptLines& interrupt);

  // <Memory>
  u8 read(u16 addr)            override;
  u8 peek(u16 addr) const      override;
  void write(u16 addr, u8 val) override;
  // <Memory/>

  void power_cycle();
  void reset();

  void cycle();

  // Stub functionallity
  void getAudiobuff(short*& samples, uint& len);
  // {
  //   samples = nullptr;
  //   len = 0;
  // }
  void set_speed(double speed) { (void)speed; }
};
