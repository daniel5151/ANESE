#pragma once

#include "util.h"
#include "ines.h"
#include "mappers/mapper.h"

// Contains Mapper and iNES cartridge
class Cartridge {
private:
	const INES* rom_data;
  Mapper* mapper;

public:
	~Cartridge();
	Cartridge(const uint8* data, uint32 data_len);

	uint8 read(uint16 addr);
	void write(uint16 addr, uint8 val);

	bool isValid();
};
