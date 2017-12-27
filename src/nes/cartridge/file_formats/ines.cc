#include "rom_file.h"

#include <cstdio>

bool ROM_File::parse_iNES() {
  // Parse the iNES file

  const u8 prg_rom_pages = this->data[4];
  const u8 chr_rom_pages = this->data[5];

  fprintf(stderr, "[iNES] PRG-ROM pages: %d\n", prg_rom_pages);
  fprintf(stderr, "[iNES] CHR-ROM pages: %d\n", chr_rom_pages);

  // Can't use a ROM with no prg_rom!
  if (prg_rom_pages == 0) {
    fprintf(stderr, "[iNES] Invalid ROM! Can't have rom with no PRG ROM!\n");
    this->is_valid = false;
    return false;
  }

  // 7       0
  // ---------
  // NNNN FTBM

  // N: Lower 4 bits of the Mapper
  // F: has_4screen
  // T: has_trainer
  // B: has_battery
  // M: mirror_type (0 = horizontal, 1 = vertical)

  this->meta.has_trainer = nth_bit(this->data[6], 2);
  this->meta.has_battery = nth_bit(this->data[6], 1);

  if (nth_bit(this->data[6], 3)) {
    this->meta.mirror_mode = Mirroring::FourScreen;
  } else {
    if (nth_bit(this->data[6], 0)) {
      this->meta.mirror_mode = Mirroring::Vertical;
    } else {
      this->meta.mirror_mode = Mirroring::Horizontal;
    }
  }

  if (this->meta.has_trainer) fprintf(stderr, "[iNES] ROM has a trainer\n");
  if (this->meta.has_battery) fprintf(stderr, "[iNES] ROM has a battery\n");
  fprintf(stderr, "[iNES] Initial Mirroring Mode: %s\n",
    Mirroring::toString(this->meta.mirror_mode));

  // 7       0
  // ---------
  // NNNN xxPV

  // N: Upper 4 bits of the mapper
  // P: is_PC10
  // V: is_VS
  // x: is_NES2 (when xx == 10)

  const bool is_NES2 = nth_bit(this->data[7], 3) && !nth_bit(this->data[6], 2);
  if (is_NES2) {
    // Ideally, use this data, but for now, just log a message...
    fprintf(stderr, "[iNES] ROM has NES 2.0 header.\n");
  }

  this->meta.is_PC10 = nth_bit(this->data[7], 1);
  this->meta.is_VS   = nth_bit(this->data[7], 0);

  if (this->meta.is_PC10) fprintf(stderr, "[iNES] This is a PC10 ROM\n");
  if (this->meta.is_VS)   fprintf(stderr, "[iNES] This is a VS ROM\n");

  this->meta.mapper = this->data[6] >> 4 | (this->data[7] & 0xFF00);

  fprintf(stderr, "[iNES] Mapper: %d\n", this->meta.mapper);

  // I'm not parsing the rest of the header, since it's not that useful

  // Finally, use the header data to get pointers into the data for the various
  // chunks of ROM

  // iNES is laid out as follows:

  // Section             | Multiplier    | Size
  // --------------------|---------------|--------
  // Header              | 1             | 0x10
  // Trainer ROM         | has_trainer   | 0x200
  // Program ROM         | prg_rom_pages | 0x4000
  // Character ROM       | chr_rom_pages | 0x2000
  // PlayChoice INST-ROM | is_PC10       | 0x2000
  // PlayChoice PROM     | is_PC10       | 0x10

  const u8* data_p = this->data + 0x10; // move past header

  // Look for the trainer
  if (this->meta.has_trainer) {
    this->rom.misc.trn_rom = data_p;
    data_p += 0x200;
  } else {
    this->rom.misc.trn_rom = nullptr;
  }

  this->rom.prg.len = prg_rom_pages * 0x4000;
  this->rom.prg.data = data_p;
  data_p += this->rom.prg.len;

  this->rom.chr.len = chr_rom_pages * 0x2000;
  this->rom.chr.data = data_p;
  data_p += this->rom.chr.len;

  if (this->meta.is_PC10) {
    this->rom.misc.pci_rom = data_p;
    this->rom.misc.pc_prom = data_p + 0x2000;
  } else {
    this->rom.misc.pci_rom = nullptr;
    this->rom.misc.pc_prom = nullptr;
  }

  return true;
}
