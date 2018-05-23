#pragma once

#include "rom_file.h"
#include "common/util.h"
#include "nes/interfaces/memory.h"
#include "nes/interfaces/mirroring.h"

// Mapper Interface
class Mapper : public Memory {
private:
  const char* name;
  const uint  number;
public:
  virtual ~Mapper() = default;
  Mapper(uint number, const char* name) : name(name), number(number) {};

  // <Memory>
  // reading doesn't tend to have side-effects, except in very advanced mappers
  virtual u8 read(u16 addr) { return this->peek(addr); }
  virtual u8 peek(u16 addr) const      = 0;
  virtual void write(u16 addr, u8 val) = 0;
  // <Memory/>

  // ---- Mapper Queries ---- //
  const char* mapper_name()   const { return this->name;   };
        uint  mapper_number() const { return this->number; };

  // ---- Mapper Properties ---- //
  virtual Mirroring::Type mirroring() const = 0; // Get mirroring mode

  // ---- Mapper Interactions ---- //
  // A CPU cycle occured
  virtual void cycle() {
    // most mappers are benign
  }

  // ---- Utilities ---- //
  // creates correct mapper given an ROM_File object
  static Mapper* Factory(const ROM_File& rom_file);
};
