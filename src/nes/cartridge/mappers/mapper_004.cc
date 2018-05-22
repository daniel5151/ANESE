#include "mapper_004.h"

#include <cassert>
#include <cstdio>
#include <cstring>

Mapper_004::Mapper_004(const ROM_File& rom_file)
: Mapper(4, "MMC3")
, prg_ram(0x2000)
{
  // Clear registers
  memset(&this->reg, 0, sizeof this->reg);

  this->fourscreen_mirroring = rom_file.meta.mirror_mode == Mirroring::FourScreen;

  // Split up PRG ROM

  // Split PRG ROM into 8K banks
  this->banks.prg.len = rom_file.rom.prg.len / 0x2000;
  this->banks.prg.bank = new ROM* [this->banks.prg.len];

  fprintf(stderr, "[Mapper_004] 8K PRG ROM Banks: %d\n", this->banks.prg.len);

  const u8* prg_data_p = rom_file.rom.prg.data;
  for (uint i = 0; i < this->banks.prg.len; i++) {
    this->banks.prg.bank[i] = new ROM (0x2000, prg_data_p, "Mapper_004 PRG");
    prg_data_p += 0x2000;
  }

  // Split up CHR ROM

  if (rom_file.rom.chr.len != 0) {
    // Split CHR ROM into 1K banks
    this->banks.chr.len = rom_file.rom.chr.len / 0x400;
    this->banks.chr.bank = new Memory* [this->banks.chr.len];

    fprintf(stderr, "[Mapper_004] 1K CHR ROM Banks: %d\n", this->banks.chr.len);

    const u8* chr_data_p = rom_file.rom.chr.data;
    for (uint i = 0; i < this->banks.chr.len; i++) {
      this->banks.chr.bank[i] = new ROM (0x400, chr_data_p, "Mapper_004 CHR");
      chr_data_p += 0x400;
    }
  } else {
    // use CHR RAM
    fprintf(stderr, "[Mapper_004] No CHR ROM detected. Using 8K CHR RAM\n");

    this->banks.chr.len = 8;
    this->banks.chr.bank = new Memory* [8];
    for (uint i = 0; i < 8; i++) {
      this->banks.chr.bank[i] = new RAM (0x400, "Mapper_004 CHR RAM");
    }
  }

  this->update_banks();
}

Mapper_004::~Mapper_004() {
  for (uint i = 0; i < this->banks.prg.len; i++)
    delete this->banks.prg.bank[i];
  delete[] this->banks.prg.bank;

  for (uint i = 0; i < this->banks.chr.len; i++)
    delete this->banks.chr.bank[i];
  delete[] this->banks.chr.bank;
}

u8 Mapper_004::read(u16 addr) {
  // TODO
  (void)addr;

  assert(false);
  return 0x00;
}

u8 Mapper_004::peek(u16 addr) const {
  // TODO
  (void)addr;

  assert(false);
  return 0x00;
}

void Mapper_004::write(u16 addr, u8 val) {
  // TODO
  (void)addr;
  (void)val;
}

void Mapper_004::update_banks() {
  // TODO
}

Mirroring::Type Mapper_004::mirroring() const {
  if (this->fourscreen_mirroring)
    return Mirroring::FourScreen;

  return this->reg.mirroring.mode
    ? Mirroring::Vertical
    : Mirroring::Horizontal;
}

void Mapper_004::cycle() {
  // ?
}
