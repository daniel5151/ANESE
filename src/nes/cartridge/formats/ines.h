#pragma once

#include "nes/generic/rom/rom.h"
#include "common/util.h"

// iNES file container
// https://wiki.nesdev.com/w/index.php/INES
struct INES {
public:
  u8 mapper; // Mapper number

  bool is_valid;

  // ROMs
  struct {
    ROM** prg_roms; // array of PRG ROM banks (16K each)
    ROM** chr_roms; // array of CHR ROM banks (8k each)

    ROM* trn_rom; // start of Trainer
    ROM* pci_rom; // start of PlayChoice INST-ROM
    ROM* pc_prom; // start PlayChoice PROM
  } roms;

  struct {
    u8 prg_rom_pages; // Num 16K program ROM pages
    u8 chr_rom_pages; // Num 8K character ROM pages (0 indicates CHR RAM)

    bool has_4screen; // Nametable mirroring is FourScreen
    bool has_trainer; // Has 512 byte trainer at 0x7000 - 0x71FF
    bool has_battery; // Has battery backed SRAM at 0x6000 - 0x7FFF
    bool mirror_type; // Nametable mirroring mode (0 = horizontal, 1 = vertical)

    bool is_PC10; // is a PC-10 Game
    bool is_VS;   // is a VS. Game

    bool is_NES2; // is using NES 2.0 extensions
  } flags;

  ~INES();
  INES(const u8* data, u32 data_len);
};
