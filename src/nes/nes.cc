#include "nes.h"

#include "cpu/cpu_mmu.h"
#include "common/debug.h"

// The constructor creates the individual NES components, and "wires them up"
// to one antother
// IT DOES NOT INITIALIZE THEM!
NES::NES() {
  this->cart = nullptr;

  // Create RAM modules
  this->cpu_ram   = new RAM (0x800);
  this->ppu_pram  = new RAM (32);
  this->ppu_ciram = new RAM (0x800);

  // Create DMA
  // this->dma = new DMA (this->cpu_ram, this->ppu_ram);

  // Create Joypads
  // JOY* joy;

  // Create MMUs
  this->cpu_mmu = new CPU_MMU(
    /* ram */ *this->cpu_ram,
    /* ppu */ *Void_Memory::Get(),
    /* apu */ *Void_Memory::Get(),
    /* dma */ *Void_Memory::Get(),
    /* joy */ *Void_Memory::Get(),
    /* rom */  this->cart
  );

  // this->ppu_mmu = new PPU_MMU (/* Stuff */);

  // Create Processors
  this->cpu = new CPU (*this->cpu_mmu);
  // this->ppu = new PPU (*this->ppu_mmu);

  /*----------  Emulator Vars  ----------*/
  this->is_running = false;
  this->clock_cycles = 0;
}

NES::~NES() {
  // Don't delete Cartridge! It's not ours!

  delete this->cpu;
  delete this->cpu_ram;
  delete this->cpu_mmu;
  delete this->ppu_pram;
  delete this->ppu_ciram;
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

void NES::step_frame() {
  if (this->is_running == false) return;

  // We need to run this thing until the PPU has a full frame ready to spit out

  // Once the PPU is implemented, I will add a boolean to the return value of
  // the PPU step method, and I will use that to determine when to break out of
  // the loop.

  // Right now though, i'm going to be a bum and just run the CPU for the
  // equivalent ammount of time :P
  // - PPU renders 262 scanlines per frame
  // - Each scanline lasts for 341 PPU clock cycles
  // - 1 CPU cycle = 3 PPU cycles
  // constexpr u32 CPU_CYCLES_PER_FRAME = 262 * 341 / 3;

  // for (u32 orig_cycles = this->clock_cycles; (this->clock_cycles - orig_cycles) / CPU_CYCLES_PER_FRAME == 0;) {
    u8 cpu_cycles = this->cpu->step();

    if (this->cpu->getState() == CPU::State::Halted) {
      this->is_running = false;
    }

    this->clock_cycles += cpu_cycles * 3;
  // }
}

bool NES::isRunning() const { return this->is_running; }
