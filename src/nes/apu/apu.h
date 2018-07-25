#pragma once

#include "common/bitfield.h"
#include "common/serializable.h"
#include "common/util.h"
#include "nes/interfaces/memory.h"

#include "nes/params.h"

#include "nes/wiring/interrupt_lines.h"
#include "filters.h"

// NES APU
// https://wiki.nesdev.com/w/index.php/APU#Mixer
class APU final : public Memory, public Serializable {
private:
  /*----------  Common Hardware Types  ----------*/

  // Used on Pulse 1, Pulse 2, and Noise
  struct Envelope {
    // Set via registers
    bool enabled;
    u8   period;
    bool loop;

    // Internals
    bool reset;
    u8   step;
    u8   val;

    void clock();
  };

  // Used on Pulse 1, Pulse 2, Triangle, and Noise
  struct LenCount {
    // Set via registers
    bool on;
    u8   val; // auto-decrements

    void clock();
  };

  /*----------  Hardware  ----------*/

  InterruptLines& interrupt;
  Memory& mem;

  struct Channels {
    // 0x4000 - 0x4003 - Pulse 1 channel
    // 0x4004 - 0x4007 - Pulse 2 channel
    struct Pulse {
      // 0x4015:0 / 0x4015:1
      bool enabled;

      // 0x4000:0-4 - period
      // 0x4000:5   - enable
      Envelope envelope;

      // 0x4000:5   - on
      // 0x4003:3-7 - val (set via table)
      LenCount len_count;

      // 0x4000:6-7 (index into duty-table)
      u8 duty_cycle;

      // 0x4001:0-7
      union {
        u8 _val;
        BitField<7>    enabled;
        BitField<4, 3> period;
        BitField<3>    negate;
        BitField<0, 3> shift;
      } sweep;

      // 0x4002:0-8 - low bits
      // 0x4003:0-3 - high bits
      u16 timer_period;

      // Internals
      bool sweep_reset;
      u8   sweep_val;
      u8   duty_val;
      u16  timer_val;

      void sweep_clock();
      void timer_clock();

      u8 output() const;

      bool isPulse2; // pulse2 has different negate-flag behavior
    } pulse1, pulse2;

    // 0x4008 - 0x400B - Triangle channel
    struct Triangle {
      bool enabled;

      // 0x4008:7   - on
      // 0x400B:3-7 - val (set via table)
      LenCount len_count;

      bool lin_count_on;     // 0x4008:7
      u8   lin_count_period; // 0x4008:0-6
      u16  timer_period;     // 0x400A:0-8 (low bits) and
                             // 0x400B:0-3 (high bits)

      // Internals
      bool lin_count_reset;
      u8   lin_count_val;
      u8   duty_val;
      u16  timer_val;

      void timer_clock();
      void lin_count_clock();

      u8 output() const;
    } tri;

    // 0x400C - 0x400F - Noise channel
    struct Noise {
      // 0x4015:3
      bool enabled;

      // 0x400C:0-4 - period
      // 0x400C:5   - enable
      Envelope envelope;

      // 0x400C:5   - on
      // 0x400F:3-7 - val (set via table)
      LenCount len_count;

      bool mode;          // 0x400E:7
      u16  timer_period;  // 0x400E:0-3 (set via table)

      // Internals
      u16 sr;
      u16 timer_val;

      void timer_clock();

      u8 output() const;
    } noise;

    // 0x4010 - 0x4014 - DMC channel
    struct DMC {
      bool enabled;

      bool inhibit_irq;   // 0x4010:7
      bool loop;          // 0x4010:6
      uint timer_period;  // 0x4010:0-3 (set via table)
      u8   output_val;    // 0x4011:0-7
      u16  sample_addr;   // 0x4012:0-7 (using formula)
      u16  sample_len;    // 0x4013:0-7 (using formula)

      // Internals
      u16  timer_val;
      u8   read_buffer;
      bool read_buffer_empty;
      u16  read_addr;
      uint read_remaining;
      u8   output_sr;
      uint output_bits_remaining;
      bool output_silence;

      // Emulator
      bool dmc_stall;

      void timer_clock(Memory& mem, InterruptLines& interrupt);

      void dmc_transfer(Memory& mem, InterruptLines& interrupt);

      u8 output() const;
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

  FirstOrderFilter* filters [3]; // Hi/Lo pass filter chain

  uint cycles;   // Total Cycles elapsed
  uint seq_step; // Frame Sequence Step

  struct {
    uint i = 0;
    float data [4096] = {0};
  } audiobuff;

  uint clock_rate = 1789773; // changes when speeding up / slowing down NES

  SERIALIZE_START(4, "APU")
    SERIALIZE_POD(chan)
    SERIALIZE_POD(frame_counter)
    SERIALIZE_POD(cycles)
    SERIALIZE_POD(seq_step)
  SERIALIZE_END(4)

  /*----------  Helpers  ----------*/

  void clock_envelopes();
  void clock_sweeps();
  void clock_timers();
  void clock_length_counters();

  class Mixer {
  private:
    float pulse_table [31];
    float tnd_table  [203];
  public:
    Mixer();
    float sample(u8 pulse1, u8 pulse2, u8 triangle, u8 noise, u8 dmc) const;
  } mixer;

  /*----------  Params  ----------*/
  const uint& sample_rate;

public:
  ~APU();
  APU() = delete;
  APU(const NES_Params& params, Memory& mem, InterruptLines& interrupt);

  // <Memory>
  u8 read(u16 addr)            override;
  u8 peek(u16 addr) const      override;
  void write(u16 addr, u8 val) override;
  // <Memory/>

  void power_cycle();
  void reset();

  void cycle();
  bool stall_cpu() {
    bool stall = this->chan.dmc.dmc_stall;
    this->chan.dmc.dmc_stall = false;
    return stall;
  }

  void getAudiobuff(float** samples, uint* len);
  void set_speed(float speed);
};
