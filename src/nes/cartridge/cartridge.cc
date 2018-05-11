#include "cartridge.h"

#include <cstdio>

Cartridge::Cartridge(const u8* data, u32 data_len)
: rom_file(data, data_len)
, mapper(Mapper::Factory(this->rom_file))
{}

Cartridge::~Cartridge() { delete this->mapper; }

u8 Cartridge::read(u16 addr)       { return this->mapper->read(addr); }
u8 Cartridge::peek(u16 addr) const { return this->mapper->peek(addr); }
void Cartridge::write(u16 addr, u8 val) { this->mapper->write(addr, val); }

uint Cartridge::getMapper() const { return this->rom_file.meta.mapper; }

Cartridge::Error Cartridge::getError() const {
  if (!this->rom_file.is_valid) { return Cartridge::Error::BAD_DATA;   }
  if (!this->mapper)            { return Cartridge::Error::BAD_MAPPER; }

  return Cartridge::Error::NO_ERROR;
}

Mirroring::Type Cartridge::mirroring() const {
  return this->mapper->mirroring();
}

void Cartridge::cycle() {
  this->mapper->cycle();
}


void Cartridge::blowOnContacts() const {
  // *huff puff puff puff*

  // Yeah, that aughta do it.
}
