#pragma once

#include "../file_formats/rom_file.h"
#include "common/util.h"
#include "nes/interfaces/memory.h"
#include "nes/interfaces/mirroring.h"

// Mapper Interface
class Mapper : public Memory {
protected:
  const ROM_File& rom_file; // Owned by Cartridge

public:
  Mapper(const ROM_File& rom_file) : rom_file(rom_file) {};
  virtual ~Mapper() {};

  // <Memory>
  // reading doesn't tend to have side-effects, except in some rare cases
  virtual u8 read(u16 addr) { return this->peek(addr); }
  virtual u8 peek(u16 addr) const      = 0;
  virtual void write(u16 addr, u8 val) = 0;
  // <Memory/>

  // Get mirroring mode
  virtual Mirroring::Type mirroring() const = 0;

  virtual void cycle() {} // most mappers are benign

  // creates correct mapper given an ROM_File object
  static Mapper* Factory(const ROM_File& rom_file);
};
