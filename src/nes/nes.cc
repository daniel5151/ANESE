#include "nes.h"

#include "common/debug.h"

// The constructor creates the individual NES components, and "wires them up"
// to one antother
// IT DOES NOT INITIALIZE THEM! A CALL TO power_cycle IS NEEDED TO DO THAT!
NES::NES() {
  this->cart = nullptr;

  // Create RAM modules
  this->cpu_wram = new RAM (0x800);
  this->ppu_vram = new RAM (0x800);
  this->ppu_pram = new RAM (32);
  this->ppu_oam  = new RAM (256);

  // Create DMA component
  this->dma = new DMA (*this->cpu_wram, *this->ppu_oam);

  // Create Joypads
  // JOY* joy;

  // Create PPU
  this->ppu_mmu = new PPU_MMU(
    /* vram */ *this->ppu_vram,
    /* pram */ *this->ppu_pram
  );
  this->ppu = new PPU (*this->ppu_mmu, *this->ppu_oam, *this->dma);

  // Create CPU
  this->cpu_mmu = new CPU_MMU(
    /* ram */ *this->cpu_wram,
    /* ppu */ *this->ppu,
    /* apu */ *Void_Memory::Get(),
    /* joy */ *Void_Memory::Get()
  );
  this->cpu = new CPU (*this->cpu_mmu);

  /*----------  Emulator Vars  ----------*/

  this->is_running = false;
}

NES::~NES() {
  // Don't delete Cartridge! It's not owned by NES!

  delete this->cpu_mmu;
  delete this->cpu;

  delete this->ppu_mmu;
  delete this->ppu;

  // delete this->joy;

  delete this->dma;

  delete this->cpu_wram;
  delete this->ppu_pram;
  delete this->ppu_vram;
}

bool NES::loadCartridge(Cartridge* cart) {
  if (cart == nullptr || cart->isValid() == false)
    return false;

  this->cart = cart;

  this->cpu_mmu->loadCartridge(this->cart);
  this->ppu_mmu->loadCartridge(this->cart);

  return true;
}

void NES::removeCartridge() {
  this->cart = nullptr;

  this->cpu_mmu->removeCartridge();
  this->ppu_mmu->removeCartridge();
}

// Power Cycling initializes all the components to their "power on" state
void NES::power_cycle() {
  this->is_running = true;

  this->cpu->power_cycle();
  this->ppu->power_cycle();

  this->cpu_wram->clear();
  this->ppu_pram->clear();
  this->ppu_vram->clear();
}

void NES::reset() {
  this->is_running = true;

  // cpu_wram, ppu_pram, and ppu_vram are not affected by resets
  // (i.e: they keep previous state)

  this->cpu->reset();
  this->ppu->reset();
}

void NES::cycle() {
  if (this->is_running == false) return;

  // Execute a CPU instruction
  uint cpu_cycles = this->cpu->step();

  // Run the PPU 3x for every cpu_cycle it took
  for (uint i = 0; i < cpu_cycles * 3; i++)
    this->ppu->cycle();

  // Check if the CPU halted
  if (this->cpu->getState() == CPU::State::Halted) {
    this->is_running = false;
  }
}

void NES::step_frame() {
  if (this->is_running == false) return;

  for (uint i = 0; i < 2000; i++)
    this->cycle();
}

const u8* NES::getFrame() const { return this->ppu->getFrame(); }

bool NES::isRunning() const { return this->is_running; }
