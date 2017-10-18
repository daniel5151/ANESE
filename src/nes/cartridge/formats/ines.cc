#include "ines.h"

#include <iostream>

INES::INES(const u8* data, u32 data_len) {
  // Hold on to the raw data (to be deleted later)
  this->raw_data = data;
  this->data_len = data_len;

  // Can't parse data if there is none ;)
  if (data == nullptr) {
    this->is_valid = false;
    return;
  }

  // Check that the data is iNES compatible
  this->is_valid = (data[0] == 'N' &&
                    data[1] == 'E' &&
                    data[2] == 'S');

  // If this isn't valid data, don't keep parsing it!
  if (!this->is_valid) { return; }

  // Parse the rest of the header

  this->flags.prg_rom_pages = data[3];
  this->flags.chr_rom_pages = data[4];

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

  this->flags.has_4screen = nth_bit(data[5], 3);
  this->flags.has_trainer = nth_bit(data[5], 2);
  this->flags.has_battery = nth_bit(data[5], 1);
  this->flags.mirror_type = nth_bit(data[5], 0);

  // 7       0
  // ---------
  // NNNN xxPV

  // N: Upper 4 bits of the mapper
  // P: is_PC10
  // V: is_VS
  // x: is_NES2 (when xx == 10)

  this->flags.is_NES2 = nth_bit(data[6], 3) && !nth_bit(data[6], 2);
  this->flags.is_PC10 = nth_bit(data[6], 1);
  this->flags.is_VS   = nth_bit(data[6], 0);

  this->mapper = data[5] >> 4 & (data[6] & 0xFF00);

  // Find base addresses for the various ROM sections in the data
  // iNES is laid out as follows:

  // Section             | Multiplier    | Size
  // --------------------|---------------|--------
  // Header              | 1             | 0x10
  // Trainer ROM         | has_trainer   | 0x200
  // Program ROM         | prg_rom_pages | 0x4000
  // Character ROM       | chr_rom_pages | 0x2000
  // PlayChoice INST-ROM | is_PC10       | 0x2000
  // PlayChoice PROM     | is_PC10       | 0x10

  this->roms.trn_rom = data + 0x10;
  this->roms.prg_rom = this->roms.trn_rom + 0x200  * this->flags.has_trainer;
  this->roms.chr_rom = this->roms.prg_rom + 0x4000 * this->flags.prg_rom_pages;
  this->roms.pci_rom = this->roms.chr_rom + 0x2000 * this->flags.chr_rom_pages;
  this->roms.pc_prom = this->roms.pci_rom + 0x2000 * this->flags.is_PC10;
}

INES::~INES() {
  delete[] raw_data;
}
