#include "mapper.h"

#include "mapper_000.h"

Mapper* Mapper::Factory(const INES* rom_file) {
	if (rom_file->is_valid == false)
		return nullptr;

	switch (rom_file->mapper) {
	case 0: return new Mapper_000(rom_file);
	}

	return nullptr;
}
