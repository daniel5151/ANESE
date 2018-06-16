#include "nes.h"

#include <cstdio>

// The constructor creates the individual NES components, and "wires them up"
// to one antother.
NES::NES() :
// RAM Modules
cpu_wram(0x800, "WRAM"),
ppu_vram(0x800, "CIRAM"),
ppu_pram(32, "Palette"),
// Processors
// (techincally UB since we pass references to objects that have not been
// initialized yet...)
cpu(this->cpu_mmu, this->interrupts),
apu(this->cpu_mmu, this->interrupts),
ppu(
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
interrupts()
{}

bool NES::loadCartridge(Mapper* cart) {
  if (cart == nullptr)
    return false;

  this->cart = cart;
  this->cart->set_interrupt_line(&this->interrupts);

  this->cpu_mmu.loadCartridge(this->cart);
  this->ppu_mmu.loadCartridge(this->cart);

  return true;
}

void NES::removeCartridge() {
  if (this->cart)
    this->cart->set_interrupt_line(nullptr);
  this->cart = nullptr;

  this->cpu_mmu.removeCartridge();
  this->ppu_mmu.removeCartridge();
}

void NES::attach_joy(uint port, Memory* joy) { this->joy.attach_joy(port, joy); }
void NES::detach_joy(uint port)              { this->joy.detach_joy(port);      }

// Power Cycling initializes all the components to their "power on" state
void NES::power_cycle() {
  this->is_running = true;

  this->apu.power_cycle();
  this->cpu.power_cycle();
  this->ppu.power_cycle();

  if (this->cart) this->cart->power_cycle();

  this->cpu_wram.clear();
  this->ppu_pram.clear();
  this->ppu_vram.clear();

  this->interrupts.clear();
  this->interrupts.request(Interrupts::RESET);

  fprintf(stderr, "[NES] Power Cycled\n");
}

void NES::reset() {
  this->is_running = true;

  // cpu_wram, ppu_pram, and ppu_vram are not affected by resets
  // (i.e: they keep previous state)

  this->apu.reset();
  this->cpu.reset();
  this->ppu.reset();

  if (this->cart) this->cart->reset();

  this->interrupts.clear();
  this->interrupts.request(Interrupts::RESET);

  fprintf(stderr, "[NES] Reset\n");
}

void NES::cycle() {
  if (this->is_running == false) return;

  // Execute a CPU instruction
  uint cpu_cycles = this->cpu.step();

  // Run PPU  + Cartridge 3x per cpu_cycle, and APU 1x per cpu_cycle
  for (uint i = 0; i < cpu_cycles    ; i++) this->apu.cycle();
  for (uint i = 0; i < cpu_cycles * 3; i++) {
    this->ppu.cycle();
    this->cart->cycle();
  }

  // Check if the CPU halted, and stop NES if it is
  if (this->cpu.getState() == CPU::State::Halted)
    this->is_running = false;
}

void NES::step_frame() {
  if (this->is_running == false) return;

  const uint curr_frame = this->ppu.getFrames();
  while (this->is_running && this->ppu.getFrames() == curr_frame) {
    this->cycle();
  }
}

const u8* NES::getFramebuff() const {
  return this->ppu.getFramebuff();
}

void NES::getAudiobuff(short*& samples, uint& len) {
  this->apu.getAudiobuff(samples, len);
}

bool NES::isRunning() const {
  return this->is_running;
}

void NES::set_speed(uint speed) {
  this->apu.set_speed(speed / 100.0);
}
