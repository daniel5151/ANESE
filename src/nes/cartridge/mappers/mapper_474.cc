#include "mapper_474.h"

#include <cassert>
#include <cstdio>

Mapper_474::Mapper_474(const ROM_File& rom_file)
  : Mapper(474, "NROM-383", rom_file, 0x4000, 0x2000)
  , prg_ram(0x2000)
{
  this->mirror_mode = rom_file.meta.mirror_mode;
  this->has_battery = rom_file.meta.has_battery;
}

// reading has no side-effects
u8 Mapper_474::read(u16 addr) { return this->peek(addr); }
u8 Mapper_474::peek(u16 addr) const {
  // Wired to the PPU MMU
  if (in_range(addr, 0x0000, 0x1FFF)) return this->chr_mem->peek(addr);

  // Wired to the CPU MMU

  // submapper 3 has_battery must be true
  if (in_range(addr, 0x4020, 0x5FFF)) return this->has_battery ?
    this->prg_ram.peek(addr - 0x4000) :
    this->prg_extra->peek(addr - 0x4000);
  if (in_range(addr, 0x6000, 0x7FFF)) return this->prg_extra->peek(addr - 0x4000);
  if (in_range(addr, 0x8000, 0xBFFF)) return this->prg_lo->peek(addr - 0x8000);
  if (in_range(addr, 0xC000, 0xFFFF)) return this->prg_hi->peek(addr - 0xC000);

  assert(false);
  return 0;
}

void Mapper_474::write(u16 addr, u8 val) {
  // Since there is potentially CHR RAM, try to write to it (if in range)
  if (in_range(addr, 0x0000, 0x1FFF)) {
    this->chr_mem->write(addr, val);
  }
  else if (in_range(addr, 0x4020, 0x5FFF)) {
    if (this->has_battery) {
      this->prg_ram.write(addr - 0x4000, val);
    }
  }
}

void Mapper_474::update_banks() {
  this->prg_extra = &this->get_prg_bank(0);
  this->prg_lo = &this->get_prg_bank(1);
  this->prg_hi = &this->get_prg_bank(2);

  this->chr_mem = &this->get_chr_bank(0);
}
