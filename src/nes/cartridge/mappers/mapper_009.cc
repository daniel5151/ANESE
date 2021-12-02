#include "mapper_009.h"

#include <cassert>
#include <cstdio>
#include <cstring>

Mapper_009::Mapper_009(const ROM_File& rom_file)
: Mapper(9, "MMC2", rom_file, 0x2000, 0x1000)
, prg_ram(0x2000)
{}

u8 Mapper_009::read(u16 addr) {
  // Interestingly enough, MMC2 has bank-switching behavior on certain reads!
  if (
    in_range(addr, 0x0FD8) ||
    in_range(addr, 0x0FE8) ||
    in_range(addr, 0x1FD8, 0x1FDF) ||
    in_range(addr, 0x1FE8, 0x1FEF)
  ) {
    const u8 retval = this->peek(addr); // latch only updated _after_ read
    this->reg.latch[!!(addr & 0x1000)] = ((addr & 0x0FF0) >> 4) == 0xFE;
    this->update_banks();
    return retval;
  }

  // Otherwise, back to our regularly scheduled, uninteresting reads...
  return this->peek(addr);
}

u8 Mapper_009::peek(u16 addr) const {
  // Wired to the PPU MMU
  if (in_range(addr, 0x0000, 0x0FFF))
    return this->chr_rom.lo[this->reg.latch[0]]->peek(addr - 0x0000);
  if (in_range(addr, 0x1000, 0x1FFF))
    return this->chr_rom.hi[this->reg.latch[1]]->peek(addr - 0x1000);

  // Wired to the CPU MMU
  if (in_range(addr, 0x4020, 0x5FFF)) return 0x00; // Nothing in "Expansion ROM"
  if (in_range(addr, 0x6000, 0x7FFF)) return this->prg_ram.peek(addr - 0x6000);
  if (in_range(addr, 0x8000, 0xFFFF))
    return this->prg_rom[(addr - 0x8000) / 0x2000]->peek(addr % 0x2000);

  assert(false);
  return 0x00;
}

void Mapper_009::write(u16 addr, u8 val) {
  // Never any CHR RAM
  if (in_range(addr, 0x4020, 0x5FFF)) return; // do nothing to expansion ROM
  if (in_range(addr, 0x6000, 0x7FFF)) {
    return this->prg_ram.write(addr - 0x6000, val);
  }

  // Otherwise, handle writing to registers

  if (in_range(addr, 0xA000, 0xAFFF)) this->reg.prg.bank = val;
  if (in_range(addr, 0xB000, 0xBFFF)) this->reg.chr.lo[0].bank = val;
  if (in_range(addr, 0xC000, 0xCFFF)) this->reg.chr.lo[1].bank = val;
  if (in_range(addr, 0xD000, 0xDFFF)) this->reg.chr.hi[0].bank = val;
  if (in_range(addr, 0xE000, 0xEFFF)) this->reg.chr.hi[1].bank = val;
  if (in_range(addr, 0xF000, 0xFFFF)) this->reg.mirroring = val;

  this->update_banks();
}

void Mapper_009::update_banks() {
  // Update PRG Banks
  // Swappable first PRG ROM bank
  this->prg_rom[0] = &this->get_prg_bank(this->reg.prg.bank);
  // Fix last-3 PRG ROM banks
  this->prg_rom[1] = &this->get_prg_bank(this->get_prg_bank_len() - 3);
  this->prg_rom[2] = &this->get_prg_bank(this->get_prg_bank_len() - 2);
  this->prg_rom[3] = &this->get_prg_bank(this->get_prg_bank_len() - 1);

  // Update CHR Banks
  this->chr_rom.lo[0] = &this->get_chr_bank(this->reg.chr.lo[0].bank);
  this->chr_rom.lo[1] = &this->get_chr_bank(this->reg.chr.lo[1].bank);
  this->chr_rom.hi[0] = &this->get_chr_bank(this->reg.chr.hi[0].bank);
  this->chr_rom.hi[1] = &this->get_chr_bank(this->reg.chr.hi[1].bank);
}

Mirroring::Type Mapper_009::mirroring() const {
  return this->reg.mirroring
    ? Mirroring::Horizontal
    : Mirroring::Vertical;
}

void Mapper_009::reset() {
  memset((char*)&this->reg, 0, sizeof this->reg);
  this->reg.latch[0] = 1;
  this->reg.latch[1] = 1;
}
