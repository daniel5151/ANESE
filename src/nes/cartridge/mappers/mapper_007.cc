#include "mapper_007.h"

#include <cassert>
#include <cstdio>
#include <cstring>

Mapper_007::Mapper_007(const ROM_File& rom_file)
: Mapper(7, "AxROM", rom_file, 0x4000, 0x2000)
{}

u8 Mapper_007::peek(u16 addr) const {
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

void Mapper_007::write(u16 addr, u8 val) {
  // TODO: handle bus conflicts?

  // Since there is potentially CHR RAM, try to write to it (if in range)
  if (in_range(addr, 0x0000, 0x1FFF)) {
    this->chr_mem->write(addr, val);
  }

  // Otherwise, handle writing to registers

  if (in_range(addr, 0x8000, 0xFFFF)) {
    this->reg.bank_select.val = val;
    this->update_banks();
  }
}

Mirroring::Type Mapper_007::mirroring() const {
  return (this->reg.bank_select.vram_page == 0)
    ? Mirroring::SingleScreenLo
    : Mirroring::SingleScreenHi;
}

void Mapper_007::update_banks() {
  this->prg_lo = &this->get_prg_bank(this->reg.bank_select.prg_bank * 2 + 0);
  this->prg_hi = &this->get_prg_bank(this->reg.bank_select.prg_bank * 2 + 1);

  this->chr_mem = &this->get_chr_bank(0);
}

void Mapper_007::reset() {
  memset((char*)&this->reg, 0, sizeof this->reg);
}
