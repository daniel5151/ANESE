#include "rom.h"

#include <assert.h>

ROM::ROM(u32 rom_size, const u8* rom) {
  // Don't allocate more memory than addressable by a u16
  if (rom_size > 0xFFFF + 1) {
    rom_size = 0xFFFF + 1;
  }

  this->size = rom_size;
  this->rom = new u8 [rom_size];

  // Init ROM
  for (u32 addr = 0; addr < this->size; addr++)
    this->rom[addr] = rom[addr];
}

ROM::~ROM() {
  delete[] this->rom;
}

// ROM read has no side-effects
u8 ROM::read(u16 addr) const { return this->peek(addr); }
u8 ROM::read(u16 addr)       { return this->peek(addr); }

u8 ROM::peek(u16 addr) const {
  assert(addr < this->size);
  return this->rom[addr];
}

// this is ROM, writes do nothing
void ROM::write(u16 addr, u8 val) {
  (void)addr;
  (void)val;
}
