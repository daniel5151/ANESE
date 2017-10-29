#include "ppu.h"

#include <cassert>

PPU::~PPU() {}
PPU::PPU(Memory& mem, Memory& oam, Memory& dma)
: dma(dma),
  mem(mem),
  oam(oam)
{
  this->cycles = 0;

  this->scan.x = 0;
  this->scan.y = 0;
}

void PPU::power_cycle() {

}

void PPU::reset() {

}

u8 PPU::read(u16 addr) {
  assert((addr >= 0x2000 && addr <= 0x2007) || addr == 0x4014);

  switch(addr) {
  /* PPUSTATUS */ case 0x2002: break;
  /* OAMDATA   */ case 0x2004: break;
  /* PPUDATA   */ case 0x2007: break;
  default: break; // reading from write only register... not good!
  }

  return 0x0;
}
u8 PPU::peek(u16 addr) const {
  assert((addr >= 0x2000 && addr <= 0x2007) || addr == 0x4014);

  switch(addr) {
  /* PPUSTATUS */ case 0x2002: break;
  /* OAMDATA   */ case 0x2004: break;
  /* PPUDATA   */ case 0x2007: break;
  default: break; // reading from write only register... not good!
  }

  return 0x0;
}
void PPU::write(u16 addr, u8 val) {
  assert((addr >= 0x2000 && addr <= 0x2007) || addr == 0x4014);

  switch(addr) {
  case 0x2000: /* PPUCTRL   */ break;
  case 0x2001: /* PPUMASK   */ break;
  case 0x2003: /* OAMADDR   */ break;
  case 0x2004: /* OAMDATA   */ break;
  case 0x2005: /* PPUSCROLL */ break;
  case 0x2006: /* PPUADDR   */ break;
  case 0x2007: /* PPUDATA   */ break;
  case 0x4014: { /* OAMDMA  */
    this->dma.write(addr, val);
    // DMA takes 513 / 514 CPU cycles (+1 cycle if starting on an odd CPU cycle)
    // The CPU doesn't do anthhing at that time, but the PPU does!
    uint dma_cycles = 513 + ((this->cycles / 3) % 2);
    for (uint i = 0; i < dma_cycles; i++)
      this->cycle();
  } break;
  default: break; // writing to read only register... not good!
  }
}

// Just dicking around rn
#include <cmath>

void PPU::cycle() {
  this->cycles += 1;

  const u32 offset = (256 * 4 * this->scan.y) + this->scan.x * 4;
  /* b */ this->frame[offset + 0] = (sin(this->cycles / 1000.0) + 1) * 125;
  /* g */ this->frame[offset + 1] = (this->scan.y / float(240)) * 255;
  /* r */ this->frame[offset + 2] = (this->scan.x / float(256)) * 255;
  /* a */ this->frame[offset + 3] = 255;


  // Always increment x
  this->scan.x += 1;
  this->scan.x %= 256;

  // Only increment y when x is back at 0
  this->scan.y += (this->scan.x == 0 ? 1 : 0);
  this->scan.y %= 240;
}

const u8* PPU::getFrame() const { return this->frame; }
