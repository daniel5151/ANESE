#include "apu.h"

#include <cassert>
#include <cstdio>
#include <cstring>

APU::~APU() {}

APU::APU(Memory& mem, InterruptLines& interrupt)
: interrupt(interrupt)
, mem(mem)
{ this->power_cycle(); }

// https://wiki.nesdev.com/w/index.php/CPU_power_up_state
void APU::power_cycle() {
  memset(&this->chan, 0, sizeof this->chan);

  this->chan.noise.sr = 1;

  this->reset();
}

// https://wiki.nesdev.com/w/index.php/CPU_power_up_state
void APU::reset() {
  this->cycles = 0;
  this->seq_step = 0;
  (*this)[0x4015] = 0x00; // silence APU
  this->frame_counter.inhibit_irq = true;
}

u8 APU::read(u16 addr) {
  u8 retval = this->peek(addr);
  if (addr == 0x4015) {
    this->frame_counter.inhibit_irq = true;
  }
  return retval;
}

u8 APU::peek(u16 addr) const {
  if (addr == 0x4015) {
    // Construct a return val
    APU::StateReg state {0};

    state.dmc_irq   = this->chan.dmc.irq_enabled;
    state.frame_irq = !this->frame_counter.inhibit_irq;

    state.pulse1 = this->chan.pulse1.length_counter > 0;
    state.pulse2 = this->chan.pulse2.length_counter > 0;
    state.tri    = this->chan.tri.length_counter    > 0;
    state.noise  = this->chan.noise.len_count       > 0;
    state.dmc    = this->chan.dmc.sample_len        > 0;

    return state.val;
  }

  fprintf(stderr, "[APU] Peek from Write-Only register: 0x%04X\n", addr);
  return 0x00;
}

static constexpr u8 LEN_COUNTER_TABLE [] = {
  10,254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
  12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

static constexpr u16 NOISE_PERIOD_TABLE [] {
  4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

void APU::write(u16 addr, u8 val) {
  // if (addr == 0x4017) {
  //   if (!this->frame_counter.inhibit_irq   != !!(val & 0x40))
  //     fprintf(stderr, "[APU] IRQ: %s\n", (val & 0x40) ? "OFF" : "ON");
  //   if (this->frame_counter.five_frame_seq != !!(val & 0x80))
  //     fprintf(stderr, "[APU] mode: %s\n", (val & 0x80) ? "5" : "4");
  // }

  // Pulse 1
  switch (addr) {
  case 0x4000: this->chan.pulse1.byte_0 = val; break;
  case 0x4001: this->chan.pulse1.byte_1 = val; break;
  case 0x4002: this->chan.pulse1.byte_2 = val; break;
  case 0x4003: {
    this->chan.pulse1.length_counter = LEN_COUNTER_TABLE[(val & 0xF8) >> 3];
    this->chan.pulse1.timer_hi = (val & 0x07);
  } break; // does additional stuff

  // Pulse 2
  case 0x4004: this->chan.pulse2.byte_0 = val; break;
  case 0x4005: this->chan.pulse2.byte_1 = val; break;
  case 0x4006: this->chan.pulse2.byte_2 = val; break;
  case 0x4007: {
    this->chan.pulse2.length_counter = LEN_COUNTER_TABLE[(val & 0xF8) >> 3];
    this->chan.pulse2.timer_hi = (val & 0x07);
  } break; // does additional stuff

  // Triangle
  case 0x4008: this->chan.tri.byte_0 = val; break;
  case 0x400A: this->chan.tri.byte_2 = val; break;
  case 0x400B: {
    this->chan.tri.length_counter = LEN_COUNTER_TABLE[(val & 0xF8) >> 3];
    this->chan.tri.timer_hi = (val & 0x07);
  } break; // does additional stuff

  // Noise
  case 0x400C: {
    this->chan.noise.envelope.loop    = (val & 0x20);
    this->chan.noise.envelope.enabled = !(val & 0x10);
    this->chan.noise.envelope.period  = (val & 0x0F);
    this->chan.noise.envelope.start   = true;
    this->chan.noise.len_count_enable = !(val & 0x20);
  } break;
  case 0x400E: {
    this->chan.noise.mode = (val & 0x80);
    this->chan.noise.timer_period = NOISE_PERIOD_TABLE[(val & 0x0F)];
  } break;
  case 0x400F: {
    this->chan.noise.len_count = LEN_COUNTER_TABLE[(val & 0xF8) >> 3];
    this->chan.noise.envelope.start = true;
  } break; // does additional stuff

  // DMC
  case 0x4010: this->chan.dmc.byte_0 = val; break;
  case 0x4011: this->chan.dmc.byte_1 = val; break;
  case 0x4012: this->chan.dmc.byte_2 = val; break;
  case 0x4013: this->chan.dmc.byte_3 = val; break;

  case 0x4015: {
    // Read flags, and enable / disable things accordingly
    APU::StateReg state {val};

    if (!(this->chan.pulse1.enabled = state.pulse1))
      this->chan.pulse1.length_counter = 0;

    if (!(this->chan.pulse2.enabled = state.pulse2))
      this->chan.pulse2.length_counter = 0;

    if (!(this->chan.tri.enabled = state.tri))
      this->chan.tri.length_counter = 0;

    if (!(this->chan.noise.enabled = state.noise))
      this->chan.noise.len_count = 0;

    this->chan.dmc.enabled = state.dmc;

  } break;

  case 0x4017: {
    this->frame_counter.val = val;
    if (this->frame_counter.five_frame_seq)
      this->clock_length_counters();
  } break;

  default:
    // fprintf(stderr, "[APU] Writing to Read-Only register: 0x%04X\n", addr);
    break;
  }
}

void APU::clock_timers() {
  // A timer is used in each of the five channels to control the sound frequency.
  // It contains a divider which is clocked by the CPU clock.
  // The triangle channel's timer is clocked on every CPU cycle, but the pulse,
  //  noise, and DMC timers are clocked only on every second CPU cycle
  //  (and thus produce only even periods).

  // this->chan.tri.clock_timer();
  if (this->cycles % 2) {
    this->chan.noise.clock_timer();
  }
}

void APU::clock_envelopes() {
  this->chan.noise.envelope.clock();
}

void APU::clock_length_counters() {
  if (this->chan.pulse1.length_counter && !this->chan.pulse1.halt)
    this->chan.pulse1.length_counter--;

  if (this->chan.pulse2.length_counter && !this->chan.pulse2.halt)
    this->chan.pulse2.length_counter--;

  if (this->chan.tri.length_counter && !this->chan.tri.halt)
    this->chan.tri.length_counter--;

  if (this->chan.noise.len_count && !this->chan.noise.len_count_enable)
    this->chan.noise.len_count--;
}

void APU::Channels::Noise::clock_timer() {
  if (this->timer_val) { this->timer_val--; }
  else {
    // When the timer clocks the shift register, the following occur in order:
    // 1) Feedback is calculated as the exclusive-OR of bit 0 and one other bit:
    //      bit 6 if Mode flag is set, otherwise bit 1.
    // 2) The shift register is shifted right by one bit.
    // 3) Bit 14, the leftmost bit, is set to the feedback calculated earlier.
    bool fb = nth_bit(this->sr, 0) ^ nth_bit(this->sr, this->mode ? 6 : 1);
    this->sr >>= 1;
    this->sr |= fb << 14;

    this->timer_val = this->timer_period;
  }
}

void APU::Envelope::clock() {
  if (this->start) {
    this->decay = 15;
    this->val = this->period;
    this->start = false;
    return;
  }

  if (this->val) this->val--;
  else {
    /**/ if (this->decay) this->decay--;
    else if (this->loop)  this->decay = 15;

    this->val = this->period;
  }
}

u16 APU::Channels::Noise::output() const {
  if (
    !this->enabled   ||
    !this->len_count ||
    this->sr & 1
  ) return 0;

  return this->envelope.enabled
    ? this->envelope.val
    : this->envelope.period;
}

#include <cmath>

// mode 0:    mode 1:       function
// ---------  -----------  -----------------------------
//  - - - f    - - - - -    IRQ (if bit 6 is clear)
//  - l - l    l - l - -    Length counter and sweep
//  e e e e    e e e e -    Envelope and linear counter
void APU::cycle() {
  this->cycles++;

  this->clock_timers();

  // The APU is cycled at 240Hz.
  // The CPU runs at 1789773 Hz
  // Thus, only step frame counter CPUSPEED / 240 cycles
  if (this->cycles % (1789773 / 240) == 0) {
    if (this->frame_counter.five_frame_seq) {
      switch(this->seq_step % 5) {
      case 0: { this->clock_envelopes();
                this->clock_length_counters();
              } break;
      case 1: { this->clock_envelopes();
              } break;
      case 2: { this->clock_envelopes();
                this->clock_length_counters();
              } break;
      case 3: { this->clock_envelopes();
              } break;
      case 4: { /* nothing */
              } break;
      }
    } else {
      switch(this->seq_step % 4) {
      case 0: { this->clock_timers();
              } break;
      case 1: { this->clock_timers();
                this->clock_length_counters();
              } break;
      case 2: { this->clock_timers();
              } break;
      case 3: { this->clock_timers();
                this->clock_length_counters();
                if (this->frame_counter.inhibit_irq == false) {
                  this->interrupt.request(Interrupts::IRQ);
                }
              } break;
      }
    }
    this->seq_step++;
  }

  // 28000*sin(440*pos*2*3.141592653589/44100)

  constexpr uint SAMPLE_RATE = 44100;
  if (this->cycles % (1789773 / SAMPLE_RATE)) {
    // fprintf(stderr, "%u\n", this->chan.noise.output());
    audiobuff.data[audiobuff.i++ % 4096] = 28000*sin(440*(this->cycles % 4096)*2*M_PI/SAMPLE_RATE);
  }
}

void APU::getAudiobuff(short*& samples, uint& len) {
  samples = nullptr;
  len = 0;
}
