#pragma once

#include "common/util.h"
#include "nes/generic/rom/rom.h"
#include "nes/interfaces/mirroring.h"

// NES ROM file container
// Owns the raw ROM file data, and has helpful data about it / pointers into it
struct ROM_File {
  // Raw file data
  const u8* data;
  uint      data_len;

  struct { // ROM metadata
    Mirroring::Type mirror_mode;

    u8 mapper;

    bool has_battery; // Has battery backed SRAM at 0x6000 - 0x7FFF
    bool has_trainer; // Has 512 byte trainer at 0x7000 - 0x71FF
    bool is_PC10;     // is a PC-10 Game
    bool is_VS;       // is a VS. Game
  } meta;

  struct { // ROM sections
    struct {
      const u8* data;
      uint len;
    } prg, chr; // Program and Character ROMs

    struct {
      const u8* trn_rom; // Trainer
      const u8* pci_rom; // PlayChoice INST-ROM
      const u8* pc_prom; // PlayChoice PROM
    } misc;
  } rom;

  ~ROM_File() { delete this->data; }
};
