#include "rom.h"

#include <cassert>
#include <cstdio>

ROM::ROM(uint rom_size, const u8* rom, const char* label /* = "?" */) {
  this->label = label;

  // Don't allocate more memory than addressable by a u16
  if (rom_size > 0xFFFF + 1) {
    rom_size = 0xFFFF + 1;
  }

  this->size = rom_size;
  this->rom = new u8 [rom_size];

  // Init ROM
  for (uint addr = 0; addr < this->size; addr++)
    this->rom[addr] = rom[addr];
}

ROM::~ROM() {
  delete[] this->rom;
}

// ROM read has no side-effects
u8 ROM::read(u16 addr) const { return this->peek(addr); }
u8 ROM::read(u16 addr)       { return this->peek(addr); }

u8 ROM::peek(u16 addr) const {
  if (addr > this->size) {
    fprintf(stderr, "[ROM][0x%04X][%s] invalid read/peek 0x%04X\n",
      this->size,
      this->label,
      addr
    );
    assert(false);
  }

  return this->rom[addr];
}

// this is ROM, writes do nothing
void ROM::write(u16 addr, u8 val) {
  if (addr > this->size) {
    fprintf(stderr, "[ROM][0x%04X][%s] invalid write 0x%04X <- 0x%02X\n",
      this->size,
      this->label,
      addr,
      val
    );
    assert(false);
  }

  (void)addr;
  (void)val;
}
