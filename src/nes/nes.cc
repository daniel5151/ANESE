#include "nes.h"

#include <cstdio>

// The constructor creates the individual NES components, and "wires them up"
// to one antother.
NES::NES(const NES_Params& params) :
// RAM Modules
cpu_wram(0x800, "WRAM"),
ppu_vram(0x800, "CIRAM"),
ppu_pram(32, "Palette"),
// Processors
// (techincally UB since we pass references to objects that have not been
// initialized yet...)
cpu(params, this->cpu_mmu, this->interrupts),
apu(params, this->cpu_mmu, this->interrupts),
ppu(params,
  this->ppu_mmu,
  this->dma,
  this->interrupts
),
// Wiring
cpu_mmu(
  /* ram */ this->cpu_wram,
  /* ppu */ this->ppu,
  /* apu */ this->apu,
  /* joy */ this->joy
),
ppu_mmu(
  /* vram */ this->ppu_vram,
  /* pram */ this->ppu_pram
),
joy(),
dma(this->cpu_mmu),
interrupts(),
params(params)
{}

void NES::updated_params() {
  this->apu.set_speed(this->params.speed / 100.0);
}

bool NES::loadCartridge(Mapper* cart) {
  if (cart == nullptr)
    return false;

  this->cart = cart;
  this->cart->set_interrupt_line(&this->interrupts);

  this->cpu_mmu.loadCartridge(this->cart);
  this->ppu_mmu.loadCartridge(this->cart);

  _callbacks.cart_changed.run(this->cart);

  return true;
}

void NES::removeCartridge() {
  if (this->cart)
    this->cart->set_interrupt_line(nullptr);
  this->cart = nullptr;

  this->cpu_mmu.removeCartridge();
  this->ppu_mmu.removeCartridge();

  _callbacks.cart_changed.run(nullptr);
}

void NES::attach_joy(uint port, Memory* joy) { this->joy.attach_joy(port, joy); }
void NES::detach_joy(uint port)              { this->joy.detach_joy(port);      }

// Power Cycling initializes all the components to their "power on" state
void NES::power_cycle() {
  if (this->params.apu_sample_rate == 0) {
    fprintf(stderr, "[NES] Fatal Error! No APU sample rate defined. "
                    "Check the NES_Params object passed to NES::NES()!\n");
    assert(this->params.apu_sample_rate != 0);
  }

  this->is_running = true;

  this->cpu_wram.clear();
  this->ppu_pram.clear();
  this->ppu_vram.clear();

  this->interrupts.clear();
  this->interrupts.request(Interrupts::RESET);

  this->apu.power_cycle();
  this->cpu.power_cycle();
  this->ppu.power_cycle();

  if (this->cart)
    this->cart->power_cycle();

  fprintf(stderr, "[NES] Power Cycled\n");
}

void NES::reset() {
  this->is_running = true;

  this->interrupts.clear();
  this->interrupts.request(Interrupts::RESET);

  // cpu_wram, ppu_pram, and ppu_vram are not affected by resets
  // (i.e: they keep previous state)

  this->apu.reset();
  this->cpu.reset();
  this->ppu.reset();

  if (this->cart)
    this->cart->reset();

  fprintf(stderr, "[NES] Reset\n");
}

void NES::cycle() {
  if (this->is_running == false) return;

  // Execute a CPU instruction
  uint cpu_cycles = this->cpu.step();

  // Run APU 1x per cpu_cycle
  for (uint i = 0; i < cpu_cycles; i++)
    this->apu.cycle();

  if (this->apu.stall_cpu())
    cpu_cycles += 4; // not entirely accurate... depends on other factors

  // Run PPU + Cartridge 3x per cpu_cycle
  for (uint i = 0; i < cpu_cycles * 3; i++) {
    this->ppu.cycle();
    this->cart->cycle();
  }

  if (!this->cpu.isRunning())
    this->is_running = false;
}

void NES::step_frame() {
  if (this->is_running == false) return;

  const uint curr_frame = this->ppu.getNumFrames();
  while (this->is_running && this->ppu.getNumFrames() == curr_frame) {
    this->cycle();
  }
}

void NES::getFramebuff(const u8** framebuffer) const {
  this->ppu.getFramebuff(framebuffer);
}

void NES::getAudiobuff(float** samples, uint* len) {
  this->apu.getAudiobuff(samples, len);
}
