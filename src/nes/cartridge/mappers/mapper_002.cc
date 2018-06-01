#include "mapper_002.h"

#include <cassert>
#include <cstdio>
#include <cstring>

Mapper_002::Mapper_002(const ROM_File& rom_file)
: Mapper(2, "UxROM")
{
  // Clear registers
  memset(&this->reg, 0, sizeof this->reg);

  this->mirror_mode = rom_file.meta.mirror_mode;

  // ---- PRG ROM ---- //

  // Split PRG ROM into 16K banks
  this->banks.prg.len = rom_file.rom.prg.len / 0x4000;
  this->banks.prg.bank = new ROM* [this->banks.prg.len];

  fprintf(stderr, "[Mapper_002] 16K PRG ROM Banks: %u\n", this->banks.prg.len);

  const u8* prg_data_p = rom_file.rom.prg.data;
  for (uint i = 0; i < this->banks.prg.len; i++) {
    this->banks.prg.bank[i] = new ROM (0x4000, prg_data_p, "Mapper_002 PRG");
    prg_data_p += 0x4000;
  }

  // Set default banks
  this->prg_lo = this->banks.prg.bank[0];
  this->prg_hi = this->banks.prg.bank[this->banks.prg.len - 1]; // FIXED ROM

  // ---- CHR ROM ---- //

  if (rom_file.rom.chr.len == 0)
    fprintf(stderr, "[Mapper_002] No CHR ROM detected. Using 8K CHR RAM\n");

  // If there is no chr_rom, then intitialize chr_ram
  this->chr_mem = (rom_file.rom.chr.len == 0x2000)
    ? (Memory*)new ROM (0x2000, rom_file.rom.chr.data)
    : (Memory*)new RAM (0x2000);
}

Mapper_002::~Mapper_002() {
  for (uint i = 0; i < this->banks.prg.len; i++)
    delete this->banks.prg.bank[i];
  delete[] this->banks.prg.bank;

  delete this->chr_mem;
}

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
  this->prg_lo = this->banks.prg.bank[this->reg.bank_select % this->banks.prg.len];
}
