#include "mapper_000.h"

#include <cassert>

Mapper_000::Mapper_000(const INES& rom_file)
: Mapper(rom_file)
{
  auto& roms = this->rom_file.roms;

  // ---- PRG ROM ---- //

  // Setup PRG ROM bank pointers
  if (this->rom_file.flags.prg_rom_pages == 2) {
    this->lo_rom = roms.prg_roms[0];
    this->hi_rom = roms.prg_roms[1];
  } else {
    // lo_rom == hi_rom
    this->lo_rom = roms.prg_roms[0];
    this->hi_rom = roms.prg_roms[0];
  }

  // ---- CHR ROM ---- //
  // i.e: the Pattern Tables

  // If there was no chr_rom, then intitialize chr_ram
  if (this->rom_file.flags.chr_rom_pages == 0) {
    this->chr_ram = new RAM (0x2000);
    this->chr_rom = nullptr;
  } else {
    this->chr_ram = nullptr;
    this->chr_rom = roms.chr_roms[0];
  }
}

Mapper_000::~Mapper_000() {
  delete this->chr_ram;
}

u8 Mapper_000::read(u16 addr) {
  // reading has no side-effects
  return this->peek(addr);
}

u8 Mapper_000::peek(u16 addr) const {
  // Wired to the PPU MMU
  if (in_range(addr, 0x0000, 0x1FFF)) return this->chr_ram != nullptr
                                              ? this->chr_ram->peek(addr)
                                              : this->chr_rom->peek(addr);

  // Wired to the CPU MMU
  if (in_range(addr, 0x4020, 0x5FFF)) return 0x00; // Nothing in "Expansion ROM"
  if (in_range(addr, 0x6000, 0x7FFF)) return 0x00; // Nothing in SRAM
  if (in_range(addr, 0x8000, 0xBFFF)) return this->lo_rom->peek(addr - 0x8000);
  if (in_range(addr, 0xC000, 0xFFFF)) return this->hi_rom->peek(addr - 0xC000);

  assert(false);
  return 0;
}

void Mapper_000::write(u16 addr, u8 val) {
  if (this->chr_ram != nullptr)
    this->chr_ram->write(addr, val);
}
