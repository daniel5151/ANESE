#include "apu.h"

#include <cassert>
#include <cstdio>

// Damn callbacks, making things annoying >:(
// Also, this will fail if I ever decide to implement Cloning NES objects
namespace CallbackHelpers {
  namespace DMC {
    static Memory* mem = nullptr;
    static void set(Memory* cpu_mem) {
      mem = cpu_mem;
    }
    static int read(void* user_data, cpu_addr_t addr) {
      (void)user_data;
      return mem->read(addr);
    }
  }
  // TODO: Figure out why IRQs are killing games...
  // namespace IRQ {
  //   static InterruptLines* interrupt = nullptr;
  //   static void set(InterruptLines* cpu_interrupt) {
  //     interrupt = cpu_interrupt;
  //   }
  //   static void fire(void* user_data) {
  //     (void)user_data;
  //     interrupt->request(Interrupts::IRQ);
  //   }
  // }

}

APU::~APU() {}

APU::APU(Memory& mem, InterruptLines& interrupt)
: interrupt(interrupt)
, mem(mem)
{
  this->blargg_buf.sample_rate(44100);
  this->blargg_buf.clock_rate(1789773);

  CallbackHelpers::DMC::set(&this->mem);
  // CallbackHelpers::IRQ::set(&this->interrupt);

  this->blargg_apu.output(&this->blargg_buf);
  this->blargg_apu.dmc_reader(CallbackHelpers::DMC::read);
  // this->blargg_apu.irq_notifier(CallbackHelpers::IRQ::fire);

  this->power_cycle();
}

// https://wiki.nesdev.com/w/index.php/CPU_power_up_state
void APU::power_cycle() {
  this->cycles = 0;
  this->frame_cycles = 0;

  this->blargg_apu.reset();
  this->blargg_buf.clear();
}

// https://wiki.nesdev.com/w/index.php/CPU_power_up_state
void APU::reset() {
  this->cycles = 0;
  this->frame_cycles = 0;

  this->blargg_apu.reset();
  this->blargg_buf.clear();
}

u8 APU::read(u16 addr) {
  if (addr == 0x4015) {
    return this->blargg_apu.read_status(this->frame_cycles);
  }

  fprintf(stderr, "[APU] Reading from Write-Only register: 0x%04X\n", addr);
  return 0x00;
}

u8 APU::peek(u16 addr) const {
  (void)addr;
  fprintf(stderr, "[APU] Cannot peek from Blargg's NES APU!\n");
  return 0x00;
}

void APU::write(u16 addr, u8 val) {
  this->blargg_apu.write_register(this->frame_cycles, addr, val);
}

void APU::cycle() {
  this->cycles++;

  this->frame_cycles++;
}

void APU::getAudiobuff(short*& samples, uint& len) {
  this->blargg_apu.end_frame(this->frame_cycles);
  this->blargg_buf.end_frame(this->frame_cycles);
  this->frame_cycles = 0;

  if (blargg_buf.samples_avail() >= 4096) {
    len = this->blargg_buf.read_samples(this->audiobuff, 4096);
    samples = this->audiobuff;
  } else {
    len = 0;
    samples = nullptr;
  }
}

void APU::set_speed(double speed) {
  this->blargg_buf.clock_rate(1789773 * speed);
}
