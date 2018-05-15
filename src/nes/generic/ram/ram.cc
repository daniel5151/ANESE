#include "ram.h"

#include <cassert>
#include <cstdio>

RAM::RAM(uint ram_size, const char* label /* = "?" */) {
  this->label = label;

  // Don't allocate more memory than addressable by a u16
  if (ram_size > 0xFFFF + 1) {
    ram_size = 0xFFFF + 1;
  }

  this->size = ram_size;
  this->ram = new u8 [ram_size];

  // Init RAM
  for (uint addr = 0; addr < this->size; addr++)
    this->ram[addr] = 0x00;
}

RAM::~RAM() {
  delete[] this->ram;
}

// RAM has no side-effects
u8 RAM::read(u16 addr) { return this->peek(addr); }
u8 RAM::peek(u16 addr) const {
  if (addr > this->size) {
    fprintf(stderr, "[RAM][0x%04X][%s] invalid read/peek 0x%04X\n",
      this->size,
      this->label,
      addr
    );
    assert(false);
  }
  return this->ram[addr];
}

void RAM::write(u16 addr, u8 val) {
  if (addr > this->size) {
    fprintf(stderr, "[RAM][0x%04X][%s] invalid write 0x%04X <- 0x%02X\n",
      this->size,
      this->label,
      addr,
      val
    );
    assert(false);
  }
  this->ram[addr] = val;
}

void RAM::clear() {
  // Reset RAM to default values
  for (uint addr = 0; addr < this->size; addr++)
    this->ram[addr] = 0x00;
}
