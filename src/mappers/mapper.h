#pragma once

#include "util.h"
#include "../ines.h"
#include "../memory.h"

// Mapper Interface

class Mapper : public Memory {
protected:
  const INES& rom_file; // Owned by Cartridge

  const u8* lo_rom;
  const u8* hi_rom;
public:
  Mapper(const INES& rom_file) : rom_file(rom_file) {};
  virtual ~Mapper() {};

  virtual u8 read(u16 addr) = 0;
  virtual void write(u16 addr, u8 val) = 0;

  // creates correct mapper given an iNES object
  static Mapper* Factory(const INES& rom_file);
};
