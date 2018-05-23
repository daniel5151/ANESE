#pragma once

#include "nes/cartridge/rom_file.h"

#include "load_rom.h" // for FileFormat enums

// Tries to guess file-format of given data
FileFormat::Type rom_type(const u8* data, uint data_len);

// Parses data as an iNES file
ROM_File* parse_iNES(const u8* data, uint data_len);
