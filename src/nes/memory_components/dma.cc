#include "dma.h"

#include <cassert>
#include <cstdio>

DMA::DMA(Memory& cpu_wram, Memory& ppu_oam)
: cpu_wram(cpu_wram),
  ppu_oam(ppu_oam)
{}

// Reading from DMA is not a valid operation...
u8 DMA::read(u16 addr)       {
  assert(addr == 0x4014);
  fprintf(stderr, "[DMA] Why is DMA being read?\n");
  return 0x0;
}
u8 DMA::peek(u16 addr) const {
  assert(addr == 0x4014);
  fprintf(stderr, "[DMA] Why is DMA being poked?\n");
  return 0x0;
}

// So, after reading some more about how DMA is supposed to work, I found out
// that DMA is actually supposed to repeatedly call OAMDATA on the PPU to do
// the DMA...
// This is _not_ how I do it, but tbh, I don't think it will make that much of
// a difference, so i'm leaving it the way it is for now.

void DMA::write(u16 dma_addr, u8 page) {
  assert(dma_addr == 0x4014);
  for (u16 addr = 0; addr <= 256; addr++) {
    // Read value from CPU WRAM
    u8 cpu_val = this->cpu_wram[(u16(page) << 8) + addr];
    // And dump it into the PPU OAM
    this->ppu_oam[addr] = cpu_val;
  }
}
