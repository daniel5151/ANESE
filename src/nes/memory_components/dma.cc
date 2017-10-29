#include "dma.h"

#include <cassert>

DMA::DMA(Memory& cpu_wram, Memory& ppu_oam)
: cpu_wram(cpu_wram),
  ppu_oam(ppu_oam)
{}

// Reading from DMA is not a valid operation...
u8 DMA::read(u16 addr)       { assert(addr == 0x4014); (void)addr; return 0x0; }
u8 DMA::peek(u16 addr) const { assert(addr == 0x4014); (void)addr; return 0x0; }

void DMA::write(u16 dma_addr, u8 page) {
  assert(dma_addr == 0x4014);
  for (u16 addr = 0; addr <= 256; addr++) {
    // Read value from CPU WRAM
    u8 cpu_val = this->cpu_wram.read((u16(page) << 8) + addr);
    // And dump it into the PPU OAM
    this->ppu_oam.write(addr, cpu_val);
  }
}
