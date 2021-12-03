#include "apu.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <climits>

/*------------------------------  Lookup Tables  -----------------------------*/

// https://wiki.nesdev.com/w/index.php/APU_Mixer#Emulation

// Instead of doing custom waveform synthesis and mixing for each channel, these
// lookup-tables do the trick (or at least close enough for me)
APU::Mixer::Mixer() {
  for (uint i = 0; i < 31;  i++) this->pulse_table[i] = 95.52  / (8128.0  / float(i) + 100);
  for (uint i = 0; i < 203; i++) this->tnd_table[i]   = 163.67 / (24329.0 / float(i) + 100);
}

// This queries the tables (given outputs from each channel)
float APU::Mixer::sample(u8 pulse1, u8 pulse2, u8 tri, u8 noise, u8 dmc) const {
  return pulse_table[pulse1 + pulse2] + tnd_table[3 * tri + 2 * noise + dmc];
}

// https://wiki.nesdev.com/w/index.php/APU_Length_Counter
static constexpr u8 LEN_COUNTER_TABLE [] = {
  10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
  12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

// https://wiki.nesdev.com/w/index.php/APU_Pulse
static constexpr bool DUTY_CYCLE_TABLE [][8] = {
  { 0, 1, 0, 0, 0, 0, 0, 0 },
  { 0, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 1, 1, 1, 1, 0, 0, 0 },
  { 1, 0, 0, 1, 1, 1, 1, 1 }
};

static constexpr u8 TRIANGLE_TABLE [] = {
  15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
};

// https://wiki.nesdev.com/w/index.php/APU_Noise
static constexpr u16 NOISE_PERIOD_TABLE [] = {
  4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

static constexpr uint DMC_PERIOD_TABLE [] = {
  428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106,  84,  72,  54
};

/*---------------------------------  APU I/O  --------------------------------*/

APU::~APU() {
  for (FirstOrderFilter* filter : this->filters) delete filter;
}

APU::APU(const NES_Params& params, Memory& mem, InterruptLines& interrupt)
: interrupt(interrupt)
, mem(mem)
, sample_rate(params.apu_sample_rate)
{
  this->chan.pulse2.isPulse2 = true;
  this->power_cycle();

  // Setup filters
  this->filters[0] = new HiPassFilter (90,    params.apu_sample_rate);
  this->filters[1] = new HiPassFilter (440,   params.apu_sample_rate);
  this->filters[2] = new LoPassFilter (14000, params.apu_sample_rate);
}

// https://wiki.nesdev.com/w/index.php/CPU_power_up_state
void APU::power_cycle() {
  memset((char*)&this->chan, 0, sizeof this->chan);

  // https://wiki.nesdev.com/w/index.php/APU_Noise
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

    state.dmc_irq   = !this->chan.dmc.inhibit_irq;
    state.frame_irq = !this->frame_counter.inhibit_irq;

    state.pulse1 = this->chan.pulse1.len_count.val > 0;
    state.pulse2 = this->chan.pulse2.len_count.val > 0;
    state.tri    = this->chan.tri.len_count.val    > 0;
    state.noise  = this->chan.noise.len_count.val  > 0;
    state.dmc    = this->chan.dmc.read_remaining   > 0;

    return state.val;
  }

  fprintf(stderr, "[APU] Peek from Write-Only register: 0x%04X\n", addr);
  return 0x00;
}

// https://wiki.nesdev.com/w/index.php/APU
// + individual channel documentations
void APU::write(u16 addr, u8 val) {
  // if (addr == 0x4017) {
  //   if (!this->frame_counter.inhibit_irq   != !!(val & 0x40))
  //     fprintf(stderr, "[APU] IRQ: %s\n", (val & 0x40) ? "OFF" : "ON");
  //   if (this->frame_counter.five_frame_seq != !!(val & 0x80))
  //     fprintf(stderr, "[APU] mode: %s\n", (val & 0x80) ? "5" : "4");
  // }

  switch (addr) {
#define pulse_impl(pulse, baddr) /* pulse1 and pulse2 are near identical... */ \
  case baddr+0: { this->chan.pulse.duty_cycle       =   (val & 0xC0) >> 6;     \
                  this->chan.pulse.len_count.on     =  !(val & 0x20);          \
                  this->chan.noise.envelope.loop    = !!(val & 0x20);          \
                  this->chan.pulse.envelope.enabled =  !(val & 0x10);          \
                  this->chan.pulse.envelope.period  =   (val & 0x0F);          \
                  this->chan.noise.envelope.reset = true;                      \
                } break;                                                       \
  case baddr+1: { this->chan.pulse.sweep._val = val;                           \
                  this->chan.pulse.sweep_reset = true;                         \
                } break;                                                       \
  case baddr+2: { const u16 prev = this->chan.pulse.timer_period & 0xFF00;     \
                  this->chan.pulse.timer_period = prev | val;                  \
                } break;                                                       \
  case baddr+3: { const u16 prev = this->chan.pulse.timer_period & 0x00FF;     \
                  this->chan.pulse.timer_period = prev | ((val & 0x07) << 8);  \
                  const u16 len_count = LEN_COUNTER_TABLE[(val & 0xF8) >> 3];  \
                  this->chan.pulse.len_count.val = len_count;                  \
                  this->chan.pulse.envelope.reset = true;                      \
                  this->chan.pulse.duty_val = 0;                               \
                } break;
  pulse_impl(pulse1, 0x4000); // Pulse 1
  pulse_impl(pulse2, 0x4004); // Pulse 2
#undef pulse_impl

  // Triangle
  case 0x4008:  { this->chan.tri.len_count.on     =  !(val & 0x80);
                  this->chan.tri.lin_count_on     =  !(val & 0x80);
                  this->chan.tri.lin_count_period =   (val & 0x7F);
                } break;
  case 0x400A:  { const u16 prev = this->chan.tri.timer_period & 0xFF00;
                  this->chan.tri.timer_period = prev | val;
                } break;
  case 0x400B:  { const u16 prev = this->chan.tri.timer_period & 0x00FF;
                  this->chan.tri.timer_period = prev | ((val & 0x07) << 8);
                  const u16 len_count = LEN_COUNTER_TABLE[(val & 0xF8) >> 3];
                  this->chan.tri.len_count.val = len_count;
                  this->chan.tri.timer_val = this->chan.tri.timer_period;
                  this->chan.tri.lin_count_reset = true;
                } break;

  // Noise
  case 0x400C:  { this->chan.noise.len_count.on     =  !(val & 0x20);
                  this->chan.noise.envelope.loop    = !!(val & 0x20);
                  this->chan.noise.envelope.enabled =  !(val & 0x10);
                  this->chan.noise.envelope.period  =   (val & 0x0F);
                  this->chan.noise.envelope.reset = true;
                } break;
  case 0x400E:  { this->chan.noise.mode = !!(val & 0x80);
                  this->chan.noise.timer_period = NOISE_PERIOD_TABLE[val & 0x0F];
                } break;
  case 0x400F:  { const u16 len_count = LEN_COUNTER_TABLE[(val & 0xF8) >> 3];
                  this->chan.noise.len_count.val = len_count;
                  this->chan.noise.envelope.reset = true;
                } break;

  // DMC
  case 0x4010:  { this->chan.dmc.inhibit_irq = !(val & 0x80);
                  this->chan.dmc.loop        =  (val & 0x40);
                  this->chan.dmc.timer_period = DMC_PERIOD_TABLE[val & 0x0F];
                } break;
  case 0x4011:  { this->chan.dmc.output_val = (val & 0x7F); // only 7 bit
                } break;
  case 0x4012:  { // %11AAAAAA.AA000000 = $C000 + (A * 64)
                  this->chan.dmc.sample_addr = 0xC000 + (val << 6);
                } break;
  case 0x4013:  { // %LLLL.LLLL0001 = (L * 16) + 1
                  this->chan.dmc.sample_len = (val << 4) + 1;
                } break;

  // Control
  case 0x4015:  { // Read flags, and enable / disable things accordingly
                  APU::StateReg state {val};
                  this->chan.pulse1.enabled = state.pulse1;
                  this->chan.pulse2.enabled = state.pulse2;
                  this->chan.tri.enabled    = state.tri;
                  this->chan.noise.enabled  = state.noise;
                  this->chan.dmc.enabled    = state.dmc;
                  if (!state.pulse1) this->chan.pulse1.len_count.val = 0;
                  if (!state.pulse2) this->chan.pulse2.len_count.val = 0;
                  if (!state.tri)    this->chan.tri.len_count.val    = 0;
                  if (!state.noise)  this->chan.noise.len_count.val  = 0;
                  // If the DMC bit is clear, the DMC bytes remaining will be
                  //  set to 0 and the DMC will silence when it empties.
                  if (!state.dmc) this->chan.dmc.read_remaining = 0;
                  // If the DMC bit is set, the DMC sample will be restarted
                  //  only if its bytes remaining is 0.
                  else if (!this->chan.dmc.read_remaining) {
                    this->chan.dmc.read_addr = this->chan.dmc.sample_addr;
                    this->chan.dmc.read_remaining = this->chan.dmc.sample_len;
                    // If there are bits remaining in the sample buffer, they
                    //  will finish playing before the new sample is fetched.
                    if (!this->chan.dmc.output_sr) {
                      this->chan.dmc.dmc_transfer(this->mem, this->interrupt);
                    } else { /* dmc starts later, once buffer empties */ }
                  }
                } break;

  // Frame Counter
  case 0x4017:  { this->frame_counter.val = val;
                  if (this->frame_counter.five_frame_seq) {
                    this->clock_timers();
                    this->clock_length_counters();
                    this->clock_sweeps();
                  }
                } break;
  default:
    // fprintf(stderr, "[APU] Writing to Read-Only register: 0x%04X\n", addr);
    break;
  }
}

/*------------------------------  APU Channels  ------------------------------*/

/*--------  Envelope Common  --------*/

void APU::Envelope::clock() {
  if (this->reset) {
    this->val = 15;

    this->step = this->period;
    this->reset = false;
    return;
  }

  if (this->step) { this->step--; }
  else {
    this->step = this->period;

    /**/ if (this->val)  this->val--;
    else if (this->loop) this->val = 15;
  }
}

/*----------  Length Counter Common  ----------*/

void APU::LenCount::clock() {
  if (this->on && this->val) { this->val--; }
}

/*--------  Pulse  --------*/
// https://wiki.nesdev.com/w/index.php/APU_Pulse
// https://wiki.nesdev.com/w/index.php/APU_Sweep

void APU::Channels::Pulse::timer_clock() {
  if (this->timer_val) { this->timer_val--; }
  else {
    this->timer_val = this->timer_period;
    // Clock sequencer (duty step)
    this->duty_val = (this->duty_val + 1) % 8;
  }
}

void APU::Channels::Pulse::sweep_clock() {
  if (this->sweep_reset) {
    this->sweep_val = this->sweep.period + 1;
    this->sweep_reset = false;
    return;
  }

  if (this->sweep_val) { this->sweep_val--; }
  else {
    this->sweep_val = this->sweep.period + 1;
    if (this->sweep.enabled) {
      // How the sweep unit calculates new target period:
      // 1) Shift the channel's 11-bit timer period right by the shift count,
      //     producing the change amount.
      const u16 change = this->timer_period >> this->sweep.shift;
      // 2) If the negate flag is true, the change amount is made negative.
      //      (taking into account different negate modes b/w pulse1 and pulse2)
      // 3) Target period = sum of the current period and the change amount.
      /**/ if (!this->sweep.negate) this->timer_period += change;     // Normal
      else if (!this->isPulse2)     this->timer_period -= change + 1; // Neg 1
      else if ( this->isPulse2)     this->timer_period -= change + 0; // Neg 2
    }
  }
}

u8 APU::Channels::Pulse::output() const {
  const bool active = DUTY_CYCLE_TABLE[this->duty_cycle][this->duty_val];
  if ( !this->enabled
    || !active
    // || sweep-adder overflow?
    || !this->len_count.val
    || this->timer_val < 8
    || this->timer_period > 0x7FF
  ) return 0;

  return this->envelope.enabled
    ? this->envelope.val
    : this->envelope.period;
}

/*--------  Triangle  --------*/
// https://wiki.nesdev.com/w/index.php/APU_Triangle

void APU::Channels::Triangle::timer_clock() {
  if (this->timer_val) { this->timer_val--; }
  else {
    this->timer_val = this->timer_period;
    // Clock sequencer (duty step)
    this->duty_val = (this->duty_val + 1) % 32;
  }
}

void APU::Channels::Triangle::lin_count_clock() {
  /**/ if (this->lin_count_reset) this->lin_count_val = this->lin_count_period;
  else if (this->lin_count_val)   this->lin_count_val--;

  if (this->lin_count_on)
    this->lin_count_reset = false;
}

u8 APU::Channels::Triangle::output() const {
  if ( !this->enabled
    || !this->len_count.val
    || !this->lin_count_val
  ) return 0;

  return TRIANGLE_TABLE[this->duty_val];
}

/*--------  Noise  --------*/
// https://wiki.nesdev.com/w/index.php/APU_Noise

void APU::Channels::Noise::timer_clock() {
  if (this->timer_val) { this->timer_val--; }
  else {
    this->timer_val = this->timer_period;
    // When the timer clocks the shift register, the following occur in order:
    // 1) Feedback is calculated as the exclusive-OR of bit 0 and one other bit:
    //      bit 6 if Mode flag is set, otherwise bit 1.
    bool fb = nth_bit(this->sr, 0) ^ nth_bit(this->sr, this->mode ? 6 : 1);
    // 2) The shift register is shifted right by one bit.
    this->sr >>= 1;
    // 3) Bit 14, the leftmost bit, is set to the feedback calculated earlier.
    this->sr |= fb << 14;
  }
}

u8 APU::Channels::Noise::output() const {
  if ( !this->enabled
    || !this->len_count.val
    || this->sr & 1
  ) return 0;

  return this->envelope.enabled
    ? this->envelope.val
    : this->envelope.period;
}

/*--------  DMC  --------*/
// https://wiki.nesdev.com/w/index.php/APU_DMC

void APU::Channels::DMC::dmc_transfer(Memory& mem, InterruptLines& interrupt) {
  if (this->read_buffer_empty && this->read_remaining) {
    // 1) The CPU is stalled for up to 4 CPU cycles to allow the longest
    //    possible write (the return address and write after an IRQ) to
    //    finish. If OAM DMA is in progress, it is paused for two cycles.
    this->dmc_stall = true;
    // 2) The sample buffer is filled with the next sample byte read from
    //    the current address
    this->read_buffer = mem[this->read_addr];
    this->read_buffer_empty = false;
    // 3) The address is incremented.
    //    If it exceeds $FFFF, it wraps to $8000
    if (++this->read_addr == 0x0000) this->read_addr = 0x8000;
    // 4) The bytes remaining counter is decremented;
    //    If it becomes zero and the loop flag is set, the sample is
    //    restarted; otherwise, if the bytes remaining counter becomes zero
    //    and the IRQ enabled flag is set, the interrupt flag is set.
    if (--this->read_remaining == 0) {
      if (this->loop) {
        this->read_addr = this->sample_addr;
        this->read_remaining = this->sample_len;
      } else if (!this->inhibit_irq) {
        // this->inhibit_irq = true;
        interrupt.request(Interrupts::IRQ);
      }
    }
  }
}

void APU::Channels::DMC::timer_clock(Memory& mem, InterruptLines& interrupt) {
  if (this->timer_val) { this->timer_val--; return; }
  this->timer_val = this->timer_period;

  // When the timer outputs a clock, the following actions occur in order:

  // 1) If the silence flag is clear, the output level changes based on bit 0 of
  //     the shift register; If the bit is 1, add 2; otherwise, subtract 2.
  if (!this->output_silence) {
    // If adding or subtracting 2 would cause the output level to leave the
    //  0-127 range, leave the output level unchanged.
    if (this->output_sr & 1) this->output_val += (this->output_val <= 125) * 2;
    else                     this->output_val -= (this->output_val >=   2) * 2;
    // 2) The right shift register is clocked
    this->output_sr >>= 1;
  }

  // 3) The bits-remaining counter is updated.
  //    When this counter reaches zero, we say that the output cycle ends.
  if (this->output_bits_remaining) { this->output_bits_remaining--; }
  // When an output cycle ends, a new cycle is started as follows:
  //  1) The bits-remaining counter is loaded with 8.
  //  2) The sample buffer is emptied into the shift register, and is refilled
  //      if needed.
  if (!this->output_bits_remaining) {
    this->output_bits_remaining = 8;
    if (this->read_buffer_empty) {
      this->output_silence = true;
    } else {
      this->output_silence = false;
      this->output_sr = this->read_buffer;
      this->read_buffer_empty = true;
      // Perform memory read
      this->dmc_transfer(mem, interrupt);
    }
  }
}

u8 APU::Channels::DMC::output() const {
  if (!this->enabled) return 0;

  return this->output_val;
}


/*----------------------------  APU Functionality  ---------------------------*/

void APU::clock_timers() {
  // The triangle channel's timer is clocked on every CPU cycle, but the pulse,
  //  noise, and DMC timers are clocked only on every second CPU cycle
  //  (and thus produce only even periods).
  this->chan.tri.timer_clock();
  if (this->cycles % 2) {
    this->chan.pulse1.timer_clock();
    this->chan.pulse2.timer_clock();
    this->chan.noise.timer_clock();
    this->chan.dmc.timer_clock(this->mem, this->interrupt); // ugly param pass
  }
}

void APU::clock_length_counters() {
  this->chan.pulse1.len_count.clock();
  this->chan.pulse2.len_count.clock();
  this->chan.noise.len_count.clock();
  this->chan.tri.len_count.clock();
}

void APU::clock_sweeps() {
  this->chan.pulse1.sweep_clock();
  this->chan.pulse2.sweep_clock();
}

void APU::clock_envelopes() {
  this->chan.pulse1.envelope.clock();
  this->chan.pulse2.envelope.clock();
  this->chan.noise.envelope.clock();
  this->chan.tri.lin_count_clock();
}

// mode 0:    mode 1:       function
// ---------  -----------  -----------------------------
//  - - - f    - - - - -    IRQ (if bit 6 is clear)
//  - l - l    l - l - -    Length counter and sweep
//  e e e e    e e e e -    Envelope and linear counter
void APU::cycle() {
  this->cycles++;

  this->clock_timers();

  // The APU is cycled at 240Hz.
  if (this->cycles % (this->clock_rate / 240) == 0) {
    if (this->frame_counter.five_frame_seq) {
      switch(this->seq_step % 5) {
      case 0: { this->clock_envelopes();
                this->clock_length_counters();
                this->clock_sweeps();
              } break;
      case 1: { this->clock_envelopes();
              } break;
      case 2: { this->clock_envelopes();
                this->clock_length_counters();
                this->clock_sweeps();
              } break;
      case 3: { this->clock_envelopes();
              } break;
      case 4: { /* nothing */
              } break;
      }
    } else {
      switch(this->seq_step % 4) {
      case 0: { this->clock_envelopes();
              } break;
      case 1: { this->clock_envelopes();
                this->clock_length_counters();
                this->clock_sweeps();
              } break;
      case 2: { this->clock_envelopes();
              } break;
      case 3: { this->clock_envelopes();
                this->clock_length_counters();
                this->clock_sweeps();
                if (this->frame_counter.inhibit_irq == false) {
                  this->interrupt.request(Interrupts::IRQ);
                }
              } break;
      }
    }
    this->seq_step++;
  }

  // Sample APU
  if (this->cycles % (this->clock_rate / this->sample_rate) == 0) {
    // Get the sample from the mixer
    float sample = this->mixer.sample(
      this->chan.pulse1.output(),
      this->chan.pulse2.output(),
      this->chan.tri.output(),
      this->chan.noise.output(),
      this->chan.dmc.output()
    );

    // Run though filter chain
    for (FirstOrderFilter* filter : this->filters)
      sample = filter->process(sample);

    // Send it off the the audio-buffer!
    audiobuff.data[audiobuff.i] = sample;
    if (audiobuff.i < 4095) audiobuff.i++;
  }
}

void APU::getAudiobuff(float** samples, uint* len) {
  if (samples == nullptr || len == nullptr) return;
  *samples = this->audiobuff.data;
  *len = this->audiobuff.i;
  this->audiobuff.i = 0;
}

void APU::set_speed(float speed) {
  this->clock_rate = 1789773 * speed;
}
