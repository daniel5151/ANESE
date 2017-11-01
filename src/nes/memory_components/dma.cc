#include "dma.h"

#include <cassert>
#include <cstdio>

DMA::DMA(Memory& cpu_wram, Memory& ppu_oam)
: cpu_wram(cpu_wram),
  ppu_oam(ppu_oam)
{
  this->in_dma = false;
  this->page = 0x00;
  this->step = 0x00;
}

void DMA::start(u8 page) {
  assert(this->in_dma == false);

  this->in_dma = true;

  this->page = page;
  this->step = 0x00;
}

void DMA::transfer() {
  u16 cpu_addr = this->step + u16(this->page << 8);
  u16 oam_addr = this->step;
  this->ppu_oam[oam_addr] = this->cpu_wram[cpu_addr];
  this->step++;

  if (this->step > 0xFF)
    this->in_dma = false;
}

bool DMA::isActive() const { return this->in_dma; }
