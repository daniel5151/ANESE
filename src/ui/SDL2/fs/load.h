#pragma once

#include "nes/cartridge/rom_file.h"

bool load_file(const char* filepath, u8*& data, uint& data_len);

ROM_File* load_rom_file(const char* filename);
