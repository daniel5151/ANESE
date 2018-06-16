#include "mapper_000.h"

#include <cassert>
#include <cstdio>

Mapper_000::Mapper_000(const ROM_File& rom_file)
: Mapper(0, "NROM", rom_file, 0x4000, 0x2000)
{ this->mirror_mode = rom_file.meta.mirror_mode; }

// reading has no side-effects
u8 Mapper_000::read(u16 addr) { return this->peek(addr); }
u8 Mapper_000::peek(u16 addr) const {
  // Wired to the PPU MMU
  if (in_range(addr, 0x0000, 0x1FFF)) return this->chr_mem->peek(addr);

  // Wired to the CPU MMU
  if (in_range(addr, 0x4020, 0x5FFF)) return 0x00; // Nothing in "Expansion ROM"
  if (in_range(addr, 0x6000, 0x7FFF)) return 0x00; // Nothing in SRAM
  if (in_range(addr, 0x8000, 0xBFFF)) return this->prg_lo->peek(addr - 0x8000);
  if (in_range(addr, 0xC000, 0xFFFF)) return this->prg_hi->peek(addr - 0xC000);

  assert(false);
  return 0;
}

void Mapper_000::write(u16 addr, u8 val) {
  // Since there is potentially CHR RAM, try to write to it (if in range)
  if (in_range(addr, 0x0000, 0x1FFF)) {
    this->chr_mem->write(addr, val);
  }
}

void Mapper_000::update_banks() {
  this->prg_lo = &this->get_prg_bank(0);
  this->prg_hi = &this->get_prg_bank(1); // Same as bank 0 when only 16K PRG ROM

  this->chr_mem = &this->get_chr_bank(0);
}
