#pragma once

#include "nes/cartridge/rom_file.h"

namespace FileFormat {
  enum Type {
    iNES,
    NES2,
    INVALID
  };
}

void load_file(const char* filepath, u8*& data, uint& data_len);

ROM_File* load_rom_file(const char* filename);
