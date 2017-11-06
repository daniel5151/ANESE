#include "dma.h"

#include <cassert>
#include <cstdio>

DMA::DMA(Memory& cpu_wram)
: cpu_wram(cpu_wram)
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

u8 DMA::transfer() {
  u16 cpu_addr = this->step + u16(this->page << 8);
  u8 retval = this->cpu_wram[cpu_addr];

  this->step++;

  if (this->step > 0xFF)
    this->in_dma = false;

  return retval;
}

bool DMA::isActive() const { return this->in_dma; }
