#include "cartridge.h"

#include "mappers/mapper.h"

Cartridge::Cartridge(const uint8* data, uint32 data_len) {
  this->rom_data = new INES(data, data_len);
  this->mapper = Mapper::Factory(*this->rom_data);
}

Cartridge::~Cartridge() {
  delete this->rom_data;
  delete this->mapper;
}

uint8 Cartridge::read(uint16 addr) {
  return this->mapper->read(addr);
}

void Cartridge::write(uint16 addr, uint8 val) {
  this->mapper->write(addr, val);
}


bool Cartridge::isValid() const {
  return this->rom_data->is_valid && this->mapper != nullptr;
}
