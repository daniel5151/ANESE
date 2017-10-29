#include "cartridge.h"

#include "mappers/mapper.h"

Cartridge::Cartridge(const u8* data, u32 data_len)
: rom_data(new INES(data, data_len)),
  mapper(Mapper::Factory(*this->rom_data))
{}

Cartridge::~Cartridge() {
  delete this->rom_data;
  delete this->mapper;
}

u8 Cartridge::read(u16 addr)       { return this->mapper->read(addr); }
u8 Cartridge::peek(u16 addr) const { return this->mapper->peek(addr); }
void Cartridge::write(u16 addr, u8 val) { this->mapper->write(addr, val); }

bool Cartridge::isValid() const {
  return this->rom_data->is_valid && this->mapper != nullptr;
}

PPU::Mirroring Cartridge::mirroring() const {
  return this->rom_data->flags.mirror_type;
}


void Cartridge::blowOnContacts() const {
  // *huff puff puff puff*

  // Yeah, that aughta do it.
}
