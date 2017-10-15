#include "ram.h"

#include <assert.h>

#include "util.h"

RAM::RAM(uint32 ram_size) {
  // Don't allocate more memory than addressable by a uint16
  if (ram_size > 0xFFFF + 1) {
    ram_size = 0xFFFF  + 1;
  }

  this->size = ram_size;
  this->ram = new uint8 [ram_size];

  // Init RAM
  for (uint32 addr = 0; addr < ram_size; addr++)
    this->ram[addr] = 0x00;
}

RAM::~RAM() {
  delete this->ram;
}

uint8 RAM::read(uint16 addr) {
  assert(addr < this->size);

  return this->ram[addr];
}

void RAM::write(uint16 addr, uint8 val) {
  assert(addr < this->size);

  this->ram[addr] = val;
}
