#include "nes.h"

#include "cpu/cpu_mmu.h"
#include "common/debug.h"

// The constructor creates the individual NES components, and "wires them up"
// to one antother
// IT DOES NOT INITIALIZE THEM!
NES::NES() {
  this->cart = nullptr;

  // Create RAM modules
  this->cpu_ram = new RAM (0x800);
  // this->ppu_ram = new RAM (/* Stuff */);

  // Create DMA
  // this->dma = new DMA (this->cpu_ram, this->ppu_ram);

  // Create Joypads
  // JOY* joy;

  // Create MMUs
  this->cpu_mmu = new CPU_MMU(
    *this->cpu_ram,
    *Void_Memory::Get(),
    *Void_Memory::Get(),
    *Void_Memory::Get(),
    *Void_Memory::Get(),
     this->cart
  );

  // this->ppu_mmu = new PPU_MMU (/* Stuff */);

  // Create Processors
  this->cpu = new CPU (*this->cpu_mmu);
  // this->ppu = new PPU (*this->ppu_mmu);

  /*----------  Emulator Vars  ----------*/
  this->is_running = false;
}

NES::~NES() {
  // Don't delete Cartridge! It's not ours!

  delete this->cpu;
  delete this->cpu_ram;
  delete this->cpu_mmu;
}

bool NES::loadCartridge(Cartridge* cart) {
  if (cart->isValid() == false)
    return false;

  this->cart = cart;
  this->cpu_mmu->addCartridge(this->cart);
  return true;
}

// Power Cycling initializes all the components to their "power on" state
void NES::power_cycle() {
  this->is_running = true;

  this->cpu->power_cycle();
  // this->ppu->power_cycle();

  this->cpu_ram->clear();
  // this->ppu_ram->clear();
}

void NES::reset() {
  this->is_running = true;

  // cpu_ram and ppu_ram are not affected by resets (keep previous state)

  this->cpu->reset();
  // this->ppu->reset();
}

void NES::step() {
  u8 cpu_cycles = this->cpu->step();

  if (this->cpu->getState() == CPU::State::Halted) {
    this->is_running = false;
  }
}

bool NES::isRunning() const { return this->is_running; }
