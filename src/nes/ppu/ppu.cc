#include "ppu.h"

PPU::~PPU() {}
PPU::PPU(Memory& mem)
: mem(mem)
{
  this->cycles = 0;

  this->scan.x = 0;
  this->scan.y = 0;
}

void PPU::power_cycle() {

}

void PPU::reset() {

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
