#include "mapper_000.h"

#include <cassert>

Mapper_000::Mapper_000(const INES& rom_file)
: Mapper(rom_file)
{
  if (this->rom_file.flags.prg_rom_pages == 2) {
    this->lo_rom = this->rom_file.roms.prg_rom + 0x0000;
    this->hi_rom = this->rom_file.roms.prg_rom + 0x2000;
  } else {
    // lo_rom == hi_rom
    this->lo_rom = this->rom_file.roms.prg_rom + 0x0000;
    this->hi_rom = this->rom_file.roms.prg_rom + 0x0000;
  }
}

Mapper_000::~Mapper_000() {}

u8 Mapper_000::read(u16 addr) {
  // 000 is a static mapper
  // reading has no side-effects
  // read and peek are thus equivalent
  return this->peek(addr);
}

u8 Mapper_000::peek(u16 addr) const {
  assert(addr >= 0x4020); // remove once mapping PPU

       if (addr >= 0x4020 && addr <= 0x5FFF) {} // Nothing in "Cartridge Expansion ROM"
  else if (addr >= 0x6000 && addr <= 0x7FFF) {} // Nothing in SRAM
  else if (addr >= 0x8000 && addr <= 0xBFFF) return this->lo_rom[addr - 0x8000];
  else if (addr >= 0xC000 && addr <= 0xFFFF) return this->hi_rom[addr - 0xC000];

  return 0x00; // default
}

void Mapper_000::write(u16 addr, u8 val) {
  // 000 is a static mapper.
  // No registers, no siwtching, no nothing.
}
