#include "mapper_003.h"

#include <cassert>
#include <cstdio>
#include <cstring>

Mapper_003::Mapper_003(const ROM_File& rom_file)
: Mapper(3, "CNROM")
{
  // Clear registers
  memset(&this->reg, 0, sizeof this->reg);

  this->mirror_mode = rom_file.meta.mirror_mode;

  // ---- PRG ROM ---- //

  if (rom_file.rom.prg.len == 0x8000) {
    // There are two banks, 16k each
    this->prg_lo = new ROM (0x4000, rom_file.rom.prg.data + 0x0000);
    this->prg_hi = new ROM (0x4000, rom_file.rom.prg.data + 0x4000);
  } else {
    // prg_lo == prg_hi
    this->prg_lo = new ROM (0x4000, rom_file.rom.prg.data + 0x0000);
    this->prg_hi = new ROM (0x4000, rom_file.rom.prg.data + 0x0000);
  }

  // ---- CHR ROM ---- //

  if (rom_file.rom.chr.len != 0) {
    // Split CHR ROM into 8K banks
    this->banks.chr.len = rom_file.rom.chr.len / 0x2000;
    this->banks.chr.bank = new Memory* [this->banks.chr.len];

    fprintf(stderr, "[Mapper_003] 8K CHR ROM Banks: %u\n", this->banks.chr.len);

    const u8* chr_data_p = rom_file.rom.chr.data;
    for (uint i = 0; i < this->banks.chr.len; i++) {
      this->banks.chr.bank[i] = new ROM (0x2000, chr_data_p, "Mapper_003 CHR");
      chr_data_p += 0x2000;
    }
  } else {
    // use CHR RAM
    fprintf(stderr, "[Mapper_003] No CHR ROM detected. Using 8K CHR RAM\n");

    this->banks.chr.len = 1;
    this->banks.chr.bank = new Memory* [1];
    this->banks.chr.bank[0] = new RAM (0x2000, "Mapper_003 CHR RAM");
  }

  // Set default CHR bank to bank 0
  this->chr_mem = this->banks.chr.bank[0];
}

Mapper_003::~Mapper_003() {
  delete this->prg_lo;
  delete this->prg_hi;

  for (uint i = 0; i < this->banks.chr.len; i++)
    delete this->banks.chr.bank[i];
  delete[] this->banks.chr.bank;
}

u8 Mapper_003::peek(u16 addr) const {
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

void Mapper_003::write(u16 addr, u8 val) {
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

void Mapper_003::update_banks() {
  this->chr_mem = this->banks.chr.bank[this->reg.bank_select % this->banks.chr.len];
}
