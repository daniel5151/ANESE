#include "dma.h"

#include <cassert>
#include <cstdio>

DMA::DMA(Memory& cpu_mmu)
: cpu_mmu(cpu_mmu)
{
  this->in_dma = false;
  this->addr = 0x0000;
  this->step = 0x00;
}

void DMA::start(u8 page) {
  assert(this->in_dma == false);

  this->in_dma = true;

  this->addr = u16(page << 8);
  this->step = 0x00;
}

u8 DMA::transfer() {
  u16 cpu_addr = this->addr + this->step;
  u8 retval = this->cpu_mmu[cpu_addr];

  this->step++;

  if (this->step > 0xFF)
    this->in_dma = false;

  return retval;
}

bool DMA::isActive() const { return this->in_dma; }
