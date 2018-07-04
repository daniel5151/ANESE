#include "parse_rom.h"

#include "rom_file.h"

#include <cstdio>

// https://wiki.nesdev.com/w/index.php/INES
static ROM_File* parseROM_iNES(const u8* data, uint data_len) {
  (void)data_len; // TODO: actually bounds-check data lol

  const u8 prg_rom_pages = data[4];
  const u8 chr_rom_pages = data[5];

  fprintf(stderr, "[File Parsing][iNES] PRG-ROM pages: %d\n", prg_rom_pages);
  fprintf(stderr, "[File Parsing][iNES] CHR-ROM pages: %d\n", chr_rom_pages);

  // Can't use a ROM with no prg_rom!
  if (prg_rom_pages == 0) {
    fprintf(stderr, "[File Parsing][iNES] Invalid ROM! "
                    "Can't have rom with no PRG ROM!\n");
    return nullptr;
  }

  // Cool, this seems to be a valid iNES rom.
  // Let's allocate the rom_file, handoff data ownership, and get to work!
  ROM_File* rf = new ROM_File();
  rf->data     = data;
  rf->data_len = data_len;

  // 7       0
  // ---------
  // NNNN FTBM

  // N: Lower 4 bits of the Mapper
  // F: has_4screen
  // T: has_trainer
  // B: has_battery
  // M: mirror_type (0 = horizontal, 1 = vertical)

  rf->meta.has_trainer = nth_bit(data[6], 2);
  rf->meta.has_battery = nth_bit(data[6], 1);

  if (nth_bit(data[6], 3)) {
    rf->meta.mirror_mode = Mirroring::FourScreen;
  } else {
    rf->meta.mirror_mode = nth_bit(data[6], 0)
      ? Mirroring::Vertical
      : Mirroring::Horizontal;
  }

  if (rf->meta.has_trainer)
    fprintf(stderr, "[File Parsing][iNES] ROM has a trainer\n");
  if (rf->meta.has_battery)
    fprintf(stderr, "[File Parsing][iNES] ROM has battery-backed SRAM\n");

  fprintf(stderr, "[File Parsing][iNES] Initial Mirroring Mode: %s\n",
    Mirroring::toString(rf->meta.mirror_mode));

  // 7       0
  // ---------
  // NNNN xxPV

  // N: Upper 4 bits of the mapper
  // P: is_PC10
  // V: is_VS
  // x: is_NES2 (when xx == 10)

  const bool is_NES2 = nth_bit(data[7], 3) && !nth_bit(data[6], 2);
  if (is_NES2) {
    // Ideally, use this data, but for now, just log a message...
    fprintf(stderr, "[File Parsing][iNES] ROM has NES 2.0 header.\n");
  }

  rf->meta.is_PC10 = nth_bit(data[7], 1);
  rf->meta.is_VS   = nth_bit(data[7], 0);

  if (rf->meta.is_PC10)
    fprintf(stderr, "[File Parsing][iNES] This is a PC10 ROM\n");
  if (rf->meta.is_VS)
    fprintf(stderr, "[File Parsing][iNES] This is a VS ROM\n");

  rf->meta.mapper = data[6] >> 4 | (data[7] & 0xFF00);

  fprintf(stderr, "[File Parsing][iNES] Mapper: %d\n", rf->meta.mapper);

  // I'm not parsing the rest of the header, since it's not that useful...


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

  const u8* data_p = data + 0x10; // move past header

  // Look for the trainer
  if (rf->meta.has_trainer) {
    rf->rom.misc.trn_rom = data_p;
    data_p += 0x200;
  } else {
    rf->rom.misc.trn_rom = nullptr;
  }

  rf->rom.prg.len = prg_rom_pages * 0x4000;
  rf->rom.prg.data = data_p;
  data_p += rf->rom.prg.len;

  rf->rom.chr.len = chr_rom_pages * 0x2000;
  rf->rom.chr.data = data_p;
  data_p += rf->rom.chr.len;

  if (rf->meta.is_PC10) {
    rf->rom.misc.pci_rom = data_p;
    rf->rom.misc.pc_prom = data_p + 0x2000;
  } else {
    rf->rom.misc.pci_rom = nullptr;
    rf->rom.misc.pc_prom = nullptr;
  }

  return rf;
}

static ROMFileFormat::Type rom_type(const u8* data, uint data_len) {
  (void)data_len;

  // Can't parse data if there is none ;)
  if (data == nullptr) {
    fprintf(stderr, "[File Parsing] ROM file is nullptr!\n");
    return ROMFileFormat::ROM_INVALID;
  }

  // Try to determine ROM format
  const bool is_iNES = (data[0] == 'N' &&
                        data[1] == 'E' &&
                        data[2] == 'S' &&
                        data[3] == 0x1A);

  if (is_iNES) {
    fprintf(stderr, "[File Parsing] ROM has iNES header.\n");

    // Double-check that it's not NES2
    const bool is_NES2 = nth_bit(data[7], 3) && !nth_bit(data[6], 2);
    if (is_NES2) {
      fprintf(stderr, "[File Parsing] ROM has NES 2.0 header.\n");
      return ROMFileFormat::ROM_NES2;
    }

    return ROMFileFormat::ROM_iNES;
  }

  fprintf(stderr, "[File Parsing] Cannot identify ROM type!\n");
  return ROMFileFormat::ROM_INVALID;
}

ROM_File* parseROM(const u8* data, uint data_len) {
  // Determine ROM type, and parse it
  switch(rom_type(data, data_len)) {
  case ROMFileFormat::ROM_iNES: return parseROM_iNES(data, data_len);
  case ROMFileFormat::ROM_NES2: return parseROM_iNES(data, data_len);
  case ROMFileFormat::ROM_INVALID: return nullptr;
  }
  return nullptr;
}
