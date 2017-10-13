#pragma once

#include <istream>

#include "util.h"

// iNES file container
// https://wiki.nesdev.com/w/index.php/INES
struct INES {
private:
  /* Raw ROM data */
  uint8* raw_data;
  uint32 data_len;

public:
  uint8 mapper; // Mapper number

  bool is_valid;

  // ROMs
  struct {
    uint8* prg_rom; // start of prg_rom banks (16K each)
    uint8* chr_rom; // start of chr_rom banks (8k each)

    uint8* trn_rom; // start of Trainer
    uint8* pci_rom; // start of PlayChoice INST-ROM
    uint8* pc_prom; // start PlayChoice PROM
  } roms;

  struct {
    uint8 prg_rom_pages; // Num 16K program ROM pages
    uint8 chr_rom_pages; // Num 8K character ROM pages

    bool mirror_type; // 0 = horizontal mirroring, 1 = vertical mirroring
    bool has_4screen; // Uses Four Screen Mode
    bool has_trainer; // Has 512 byte trainer at $7000 - $71FFh
    bool has_battery; // Has battery backed SRAM at $6000 - $7FFFh

    bool is_PC10; // is a PC-10 Game
    bool is_VS;   // is a VS. Game

    bool is_NES2; // is using NES 2.0 extensions
  } flags;

  ~INES();
  INES(std::istream& file);
  INES(const INES& other);
  INES& operator= (const INES& other);
};
