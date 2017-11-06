#include "cartridge.h"

#include <cstdio>

Cartridge::Cartridge(const u8* data, u32 data_len)
: rom_data(new INES(data, data_len)),
  mapper(Mapper::Factory(*this->rom_data))
{
  if (this->rom_data->flags.has_4screen) {
    fprintf(stderr, "[Mirroring] FourScreen\n");
    this->mirroring_type = Cartridge::Mirroring::FourScreen;
  }
  else if (this->rom_data->flags.mirror_type == 0) {
    fprintf(stderr, "[Mirroring] Vertical\n");
    this->mirroring_type = Cartridge::Mirroring::Vertical;
  }
  else /* (this->rom_data->flags.mirror_type == 1) */ {
    fprintf(stderr, "[Mirroring] Horizontal\n");
    this->mirroring_type = Cartridge::Mirroring::Horizontal;
  }
}

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

Cartridge::Mirroring Cartridge::mirroring() const {
  return this->mirroring_type;
}


void Cartridge::blowOnContacts() const {
  // *huff puff puff puff*

  // Yeah, that aughta do it.
}
