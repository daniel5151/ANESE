#include "ram.h"

#include <assert.h>

RAM::RAM(u32 ram_size) {
  // Don't allocate more memory than addressable by a u16
  if (ram_size > 0xFFFF + 1) {
    ram_size = 0xFFFF + 1;
  }

  this->size = ram_size;
  this->ram = new u8 [ram_size];

  // Init RAM
  for (u32 addr = 0; addr < this->size; addr++)
    this->ram[addr] = 0x00;
}

RAM::~RAM() {
  delete[] this->ram;
}

// RAM has no side-effects
u8 RAM::read(u16 addr) { return this->peek(addr); }
u8 RAM::peek(u16 addr) const {
  assert(addr < this->size);
  return this->ram[addr];
}

void RAM::write(u16 addr, u8 val) {
  assert(addr < this->size);
  this->ram[addr] = val;
}

void RAM::clear() {
  // Reset RAM to default values
  for (u32 addr = 0; addr < this->size; addr++)
    this->ram[addr] = 0x00;
}
