#pragma once

#include "rom_file.h"

namespace ROMFileFormat {
  enum Type {
    iNES,
    NES2,
    INVALID
  };
}

// Parses data into usable ROM_File
ROM_File* parse_ROM(const u8* data, uint data_len);
