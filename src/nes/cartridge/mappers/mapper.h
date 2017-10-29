#pragma once

#include "../formats/ines.h"
#include "common/interfaces/memory.h"
#include "common/util.h"

// Mapper Interface

class Mapper : public Memory {
protected:
  const INES& rom_file; // Owned by Cartridge

public:
  Mapper(const INES& rom_file) : rom_file(rom_file) {};
  virtual ~Mapper() {};

  // <Memory>
  virtual u8 read(u16 addr) = 0;
  virtual u8 peek(u16 addr) const = 0;
  virtual void write(u16 addr, u8 val) = 0;
  // <Memory/>

  // creates correct mapper given an iNES object
  static Mapper* Factory(const INES& rom_file);
};
