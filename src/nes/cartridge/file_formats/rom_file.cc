#include "rom_file.h"

#include <cstdio>

// Currently, only parses iNES roms
// https://wiki.nesdev.com/w/index.php/INES
ROM_File::ROM_File(const u8* data, u32 data_len) {
  // Can't parse data if there is none ;)
  if (data == nullptr) {
    this->is_valid = false;
    return;
  }

  // Make a copy of the data
  this->data = new u8 [data_len];
  this->data_len = data_len;
  for (uint i = 0; i < data_len; i++)
    this->data[i] = data[i];

  // Try to determine ROM format
  const bool is_iNES = (data[0] == 'N' &&
                        data[1] == 'E' &&
                        data[2] == 'S' &&
                        data[3] == 0x1A);

  if (is_iNES) {
    fprintf(stderr, "[File Parsing] ROM has iNES header.\n");
    const bool is_NES2 = nth_bit(data[7], 3) && !nth_bit(data[6], 2);
    if (is_NES2) {
      fprintf(stderr, "[File Parsing] ROM has NES 2.0 header.\n");
      // _should_ use this new info, but I haven't implemented that yet...
    }

    this->is_valid = ROM_File::parse_iNES();
    return;
  }


  fprintf(stderr, "[File Parsing] Cannot identify ROM type!\n");
  this->is_valid = false;
}

ROM_File::~ROM_File() { delete this->data; }
