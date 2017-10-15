#include "nes.h"

#include "cpu_mmu.h"
#include "common/debug.h"

NES::NES() {
  this->cart = nullptr;

  // Init RAM modules
  this->cpu_ram = new RAM (0x800);
  // this->ppu_ram = new RAM (/* Stuff */);

  // Init DMA
  // this->dma = new DMA (this->cpu_ram, this->ppu_ram);

  // Init Joypads
  // JOY* joy;

  // Init MMUs
  this->cpu_mmu = new CPU_MMU(
    *new Memory_Sniffer("CPU -> RAM", this->cpu_ram),
    *new Memory_Sniffer("CPU -> PPU", Void_Memory::Get()), // These new calls
    *new Memory_Sniffer("CPU -> APU", Void_Memory::Get()), // are never cleaned
    *new Memory_Sniffer("CPU -> DMA", Void_Memory::Get()), // up, but w/e, they
    *new Memory_Sniffer("CPU -> JOY", Void_Memory::Get()), // are just for debug
     new Memory_Sniffer("CPU -> ROM", this->cart)
  );

  // this->ppu_mmu = new PPU_MMU (/* Stuff */);

  // Init Processors
  // this->cpu = new CPU (this->cpu_mmu);
  // this->ppu = new PPU (this->ppu_mmu);
}

NES::~NES() {
  // Don't delete Cartridge! It's not ours!

  delete this->cpu_ram;
  delete this->cpu_mmu;
}

bool NES::loadCartridge(Cartridge* cart) {
  if (cart->isValid() == false)
    return false;

  this->cart = cart;
  this->cpu_mmu->addCartridge(new Memory_Sniffer("CPU -> ROM", this->cart));
  return true;
}


#include <iostream>
void NES::start() {
  // stub
}

void NES::stop() {
  // stub
}

bool NES::isRunning() const { return this->is_running; }
