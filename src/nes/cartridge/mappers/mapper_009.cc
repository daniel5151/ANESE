#include "mapper_009.h"

#include <cassert>
#include <cstdio>
#include <cstring>

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! UNTESTED !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //

Mapper_009::Mapper_009(const ROM_File& rom_file)
: Mapper(9, "MMC2")
, prg_ram(0x2000)
{
  // Clear registers
  memset(&this->reg, 0, sizeof this->reg);
  this->reg.latch[0] = 0xFE;
  this->reg.latch[1] = 0xFE;

  // ---- PRG ROM ---- //

  // Split PRG ROM into 8K banks
  this->banks.prg.len = rom_file.rom.prg.len / 0x2000;
  this->banks.prg.bank = new ROM* [this->banks.prg.len];

  fprintf(stderr, "[Mapper_009] 8K PRG ROM Banks: %u\n", this->banks.prg.len);

  const u8* prg_data_p = rom_file.rom.prg.data;
  for (uint i = 0; i < this->banks.prg.len; i++) {
    this->banks.prg.bank[i] = new ROM (0x2000, prg_data_p, "Mapper_009 PRG");
    prg_data_p += 0x2000;
  }

  // ---- CHR ROM ---- //
  // Never CHR RAM
  if (rom_file.rom.chr.len == 0) {
    fprintf(stderr, "[Mapper_009] No CHR ROM, this shouldn't happen!\n");
    assert(false);
  }

  // Split CHR ROM into 4K banks
  this->banks.chr.len = rom_file.rom.chr.len / 0x1000;
  this->banks.chr.bank = new ROM* [this->banks.chr.len];

  fprintf(stderr, "[Mapper_009] 4K CHR ROM Banks: %u\n", this->banks.chr.len);

  const u8* chr_data_p = rom_file.rom.chr.data;
  for (uint i = 0; i < this->banks.chr.len; i++) {
    this->banks.chr.bank[i] = new ROM (0x1000, chr_data_p, "Mapper_009 CHR");
    chr_data_p += 0x1000;
  }

  this->update_banks();
}

Mapper_009::~Mapper_009() {
  for (uint i = 0; i < this->banks.prg.len; i++)
    delete this->banks.prg.bank[i];
  delete[] this->banks.prg.bank;

  for (uint i = 0; i < this->banks.chr.len; i++)
    delete this->banks.chr.bank[i];
  delete[] this->banks.chr.bank;
}


u8 Mapper_009::read(u16 addr) {
  // Interestingly enough, MMC2 has bank-switching behavior on certain reads!
  if (
    ((addr & 0xF000) == 0x1000 || (addr & 0xF000) == 0x0000) &&
    ((addr & 0x0FF8) == 0x0FD8 || (addr & 0x0FF8) == 0x0FE8)
  ) {
    const u8 retval = this->peek(addr); // latch only updated _after_ read
    this->reg.latch[!!(addr & 0x1000)] = (addr & 0x0FF0) >> 4;
    this->update_banks();
    return retval;
  }

  // Otherwise, back to our regularly scheduled, uninteresting reads...
  return this->peek(addr);
}

u8 Mapper_009::peek(u16 addr) const {
  // Wired to the PPU MMU
  if (in_range(addr, 0x0000, 0x0FFF))
    return this->chr_rom.lo[this->reg.latch[0] == 0xFE]->peek(addr - 0x0000);
  if (in_range(addr, 0x1000, 0x1FFF))
    return this->chr_rom.hi[this->reg.latch[1] == 0xFE]->peek(addr - 0x1000);

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

  if (in_range(addr, 0xA000, 0xAFFF)) this->reg.prg_bank = val;       return;
  if (in_range(addr, 0xB000, 0xBFFF)) this->reg.chr_bank.lo[0] = val; return;
  if (in_range(addr, 0xC000, 0xCFFF)) this->reg.chr_bank.lo[1] = val; return;
  if (in_range(addr, 0xD000, 0xDFFF)) this->reg.chr_bank.hi[0] = val; return;
  if (in_range(addr, 0xE000, 0xEFFF)) this->reg.chr_bank.hi[1] = val; return;
  if (in_range(addr, 0xF000, 0xFFFF)) this->reg.mirroring = val;      return;
}

void Mapper_009::update_banks() {
  // Update PRG Banks
  // Swappable first PRG ROM bank
  const uint prg_len = this->banks.prg.len;
  this->prg_rom[0] = this->banks.prg.bank[this->reg.prg_bank % prg_len];
  // Fix last-3 PRG ROM banks
  this->prg_rom[1] = this->banks.prg.bank[prg_len - 3];
  this->prg_rom[2] = this->banks.prg.bank[prg_len - 2];
  this->prg_rom[3] = this->banks.prg.bank[prg_len - 1];

  // Update CHR Banks
  const uint chr_len = this->banks.chr.len;
  this->chr_rom.lo[0] = this->banks.chr.bank[this->reg.chr_bank.lo[0] % chr_len];
  this->chr_rom.lo[1] = this->banks.chr.bank[this->reg.chr_bank.lo[1] % chr_len];
  this->chr_rom.hi[0] = this->banks.chr.bank[this->reg.chr_bank.hi[0] % chr_len];
  this->chr_rom.hi[1] = this->banks.chr.bank[this->reg.chr_bank.hi[1] % chr_len];
}

Mirroring::Type Mapper_009::mirroring() const {
  return this->reg.mirroring
    ? Mirroring::Horizontal
    : Mirroring::Vertical;
}
