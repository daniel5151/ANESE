#include "ines.h"

#include <cstdio>

INES::INES(const u8* data, u32 data_len) {
  (void)data_len; // don't actually use it

  // Can't parse data if there is none ;)
  if (data == nullptr) {
    this->is_valid = false;
    return;
  }

  // Check that the data is iNES compatible
  this->is_valid = (data[0] == 'N' &&
                    data[1] == 'E' &&
                    data[2] == 'S' &&
                    data[3] == 0x1A);

  // If this isn't valid data, don't keep parsing it!
  if (!this->is_valid) { return; }

  fprintf(stderr, "[iNES] Header parsed\n");

  // Parse the rest of the header

  this->flags.prg_rom_pages = data[4];
  fprintf(stderr, "[iNES] PRG-ROM pages: %d\n", this->flags.prg_rom_pages);
  this->flags.chr_rom_pages = data[5];
  fprintf(stderr, "[iNES] CHR-ROM pages: %d\n", this->flags.chr_rom_pages);

  // Can't use a ROM with no prg_rom!
  if (this->flags.prg_rom_pages == 0) {
    this->is_valid = false;
    return;
  }

  // 7       0
  // ---------
  // NNNN FTBM

  // N: Lower 4 bits of the Mapper
  // F: has_4screen
  // T: has_trainer
  // B: has_battery
  // M: mirror_type (0 = horizontal, 1 = vertical)

  this->flags.has_4screen = nth_bit(data[6], 3);
  this->flags.has_trainer = nth_bit(data[6], 2);
  this->flags.has_battery = nth_bit(data[6], 1);
  this->flags.mirror_type = nth_bit(data[6], 0);

  // 7       0
  // ---------
  // NNNN xxPV

  // N: Upper 4 bits of the mapper
  // P: is_PC10
  // V: is_VS
  // x: is_NES2 (when xx == 10)

  this->flags.is_NES2 = nth_bit(data[7], 3) && !nth_bit(data[6], 2);
  this->flags.is_PC10 = nth_bit(data[7], 1);
  this->flags.is_VS   = nth_bit(data[7], 0);

  this->mapper = data[6] >> 4 & (data[7] & 0xFF00);

  fprintf(stderr, "[iNES] Mapper: %d\n", this->mapper);

  // Not parsing the rest of the headers, since it's not really used for much

  // Find addresses for the various ROM sections in the data, and throw them
  // into some ROM ADTs

  // iNES is laid out as follows:

  // Section             | Multiplier    | Size
  // --------------------|---------------|--------
  // Header              | 1             | 0x10
  // Trainer ROM         | has_trainer   | 0x200
  // Program ROM         | prg_rom_pages | 0x4000
  // Character ROM       | chr_rom_pages | 0x2000
  // PlayChoice INST-ROM | is_PC10       | 0x2000
  // PlayChoice PROM     | is_PC10       | 0x10

  const u8* data_p = data + 0x10; // move past header

  if (this->flags.has_trainer) {
    this->roms.trn_rom = new ROM(0x200, data_p);
    data_p += 0x200;
  } else {
    this->roms.trn_rom = nullptr;
  }

  this->roms.prg_roms = new ROM* [this->flags.prg_rom_pages];
  for (uint i = 0; i < this->flags.prg_rom_pages; i++) {
    this->roms.prg_roms[i] = new ROM (0x4000, data_p);
    data_p += 0x4000;
  }

  this->roms.chr_roms = new ROM* [this->flags.chr_rom_pages];
  for (uint i = 0; i < this->flags.chr_rom_pages; i++) {
    this->roms.chr_roms[i] = new ROM (0x2000, data_p);
    data_p += 0x2000;
  }

  if (this->flags.is_PC10) {
    this->roms.pci_rom = new ROM (0x2000, data_p);
    data_p += 0x2000;
    this->roms.pc_prom = new ROM (0x10, data_p);
  } else {
    this->roms.pci_rom = nullptr;
    this->roms.pc_prom = nullptr;
  }
}

INES::~INES() {
  for (uint i = 0; i < this->flags.prg_rom_pages; i++)
    delete this->roms.prg_roms[i];
  delete[] this->roms.prg_roms;

  for (uint i = 0; i < this->flags.chr_rom_pages; i++)
    delete this->roms.chr_roms[i];
  delete[] this->roms.chr_roms;

  delete this->roms.trn_rom;
  delete this->roms.pci_rom;
  delete this->roms.pc_prom;
}
