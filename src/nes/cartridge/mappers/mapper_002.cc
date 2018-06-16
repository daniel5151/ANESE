#include "mapper_002.h"

#include <cassert>
#include <cstdio>
#include <cstring>

Mapper_002::Mapper_002(const ROM_File& rom_file)
: Mapper(2, "UxROM", rom_file, 0x4000, 0x2000)
{ this->mirror_mode = rom_file.meta.mirror_mode; }

u8 Mapper_002::peek(u16 addr) const {
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

void Mapper_002::write(u16 addr, u8 val) {
  // TODO: handle bus conflicts?

  // Since there is potentially CHR RAM, try to write to it (if in range)
  if (in_range(addr, 0x0000, 0x1FFF)) {
    this->chr_mem->write(addr, val);
  }

  // Otherwise, handle writing to registers

  if (in_range(addr, 0x8000, 0xFFFF)) {
    this->reg.bank_select = val;
    this->update_banks();
  }
}

void Mapper_002::update_banks() {
  this->prg_lo = &this->get_prg_bank(this->reg.bank_select);
  this->prg_hi = &this->get_prg_bank(this->get_prg_bank_len() - 1); // Fixed

  this->chr_mem = &this->get_chr_bank(0);
}

void Mapper_002::reset() {
  memset(&this->reg, 0, sizeof this->reg);
}
