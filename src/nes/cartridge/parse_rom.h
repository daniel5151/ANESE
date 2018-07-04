#pragma once

#include "rom_file.h"

namespace ROMFileFormat {
  enum Type {
    ROM_iNES,
    ROM_NES2,
    ROM_INVALID
  };
}

// Parses data into usable ROM_File
ROM_File* parseROM(const u8* data, uint data_len);
