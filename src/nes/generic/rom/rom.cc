#include "rom.h"

#include <cassert>
#include <cstdio>

ROM::ROM(uint rom_size, const u8* rom, const char* label /* = "?" */) {
  this->label = label;

  if (rom_size > 0xFFFF + 1) {
    fprintf(stderr, "[ROM][%s] WARNING: ROM of size 0x%X is not 16 bit addressable!\n",
      label,
      rom_size
    );
  }

  this->rom  = rom;
  this->size = rom_size;
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
