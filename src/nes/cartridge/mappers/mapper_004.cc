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

  fprintf(stderr, "[Mapper_004] 8K PRG ROM Banks: %u\n", this->banks.prg.len);

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

    fprintf(stderr, "[Mapper_004] 1K CHR ROM Banks: %u\n", this->banks.chr.len);

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
  // Wired to the PPU MMU
  if (in_range(addr, 0x000, 0x1FFF)) {
    return this->chr_bank[addr / 0x400]->read(addr % 0x400);
  }

  // Wired to the CPU MMU
  if (in_range(addr, 0x4020, 0x5FFF)) return 0x00; // Nothing in "Expansion ROM"
  if (in_range(addr, 0x6000, 0x7FFF)) return this->reg.ram_protect.enable_ram
                                           ? this->prg_ram.read(addr - 0x6000)
                                           : 0x00; // should be open bus...
  if (in_range(addr, 0x8000, 0xFFFF)) {
    return this->prg_bank[(addr - 0x8000) / 0x2000]->read(addr % 0x2000);
  }

  assert(false);
  return 0x00;
}

u8 Mapper_004::peek(u16 addr) const {
  // Wired to the PPU MMU
  if (in_range(addr, 0x000, 0x1FFF)) {
    return this->chr_bank[addr / 0x400]->peek(addr % 0x400);
  }

  // Wired to the CPU MMU
  if (in_range(addr, 0x4020, 0x5FFF)) return 0x00; // Nothing in "Expansion ROM"
  if (in_range(addr, 0x6000, 0x7FFF)) return this->reg.ram_protect.enable_ram
                                           ? this->prg_ram.peek(addr - 0x6000)
                                           : 0x00; // should be open bus...
  if (in_range(addr, 0x8000, 0xFFFF)) {
    return this->prg_bank[(addr - 0x8000) / 0x2000]->peek(addr % 0x2000);
  }

  assert(false);
  return 0x00;
}

void Mapper_004::write(u16 addr, u8 val) {
  if (in_range(addr, 0x0000, 0x1FFF)) {
    // CHR might be RAM (potentially)
    return this->chr_bank[addr / 0x400]->write(addr % 0x400, val);
  }
  if (in_range(addr, 0x4020, 0x5FFF)) return; // do nothing to expansion ROM
  if (in_range(addr, 0x6000, 0x7FFF)) {
    (this->reg.ram_protect.write_enable == 0)
      ? this->prg_ram.write(addr - 0x6000, val)
      : void();
  }

  // Otherwise, handle writing to registers

  switch(addr & 0xE001) {
  // Memory Mapping
  case 0x8000: this->reg.bank_select.val = val; this->update_banks(); return;
  case 0x8001: this->reg.bank_values[this->reg.bank_select.bank] = val;
                                                this->update_banks(); return;
  case 0xA000: this->reg.mirroring.val   = val; this->update_banks(); return;
  case 0xA001: this->reg.ram_protect.val = val; this->update_banks(); return;
  // IRQ
  case 0xC000: this->reg.irq_latch = val;     return;
  case 0xC001: this->reg.irq_counter = 0;     return;
  case 0xE000: this->irq_service();
               this->reg.irq_enabled = false; return;
  case 0xE001: this->reg.irq_enabled = true;  return;
  }
}

void Mapper_004::update_banks() {
  // https://wiki.nesdev.com/w/index.php/MMC3#PRG_Banks
  #define PBANK(i, val) \
    this->prg_bank[i] = this->banks.prg.bank[val % this->banks.prg.len];
  if (this->reg.bank_select.prg_rom_mode == 0) {
    PBANK(0, this->reg.bank_values[6]);
    PBANK(1, this->reg.bank_values[7]);
    PBANK(2, this->banks.prg.len - 2);
    PBANK(3, this->banks.prg.len - 1);
  } else {
    PBANK(0, this->banks.prg.len - 2);
    PBANK(1, this->reg.bank_values[7]);
    PBANK(2, this->reg.bank_values[6]);
    PBANK(3, this->banks.prg.len - 1);
  }
  #undef PBANK

  // https://wiki.nesdev.com/w/index.php/MMC3#CHR_Banks
  #define CBANK(i, val) \
    this->chr_bank[i] = this->banks.chr.bank[val % this->banks.chr.len];
  if (this->reg.bank_select.chr_inversion == 0) {
    CBANK(0, this->reg.bank_values[0] & 0xFE);
    CBANK(1, this->reg.bank_values[0] | 0x01);
    CBANK(2, this->reg.bank_values[1] & 0xFE);
    CBANK(3, this->reg.bank_values[1] | 0x01);
    CBANK(4, this->reg.bank_values[2]);
    CBANK(5, this->reg.bank_values[3]);
    CBANK(6, this->reg.bank_values[4]);
    CBANK(7, this->reg.bank_values[5]);
  } else {
    CBANK(0, this->reg.bank_values[2]);
    CBANK(1, this->reg.bank_values[3]);
    CBANK(2, this->reg.bank_values[4]);
    CBANK(3, this->reg.bank_values[5]);
    CBANK(4, this->reg.bank_values[0] & 0xFE);
    CBANK(5, this->reg.bank_values[0] | 0x01);
    CBANK(6, this->reg.bank_values[1] & 0xFE);
    CBANK(7, this->reg.bank_values[1] | 0x01);
  }
  #undef CBANK
}

Mirroring::Type Mapper_004::mirroring() const {
  if (this->fourscreen_mirroring)
    return Mirroring::FourScreen;

  return this->reg.mirroring.mode
    ? Mirroring::Horizontal
    : Mirroring::Vertical;
}

// Ideally, if I had proper timings for PPU sprite fetching implemented, I could
// directly track PPU A12's state (i.e: bit 12 of addr) during reads to know
// when to fire the interrupt.
// I haven't done that, and instead, we use this janky method.
void Mapper_004::cycle(const PPU& ppu) {
  uint scanline    = ppu.getScanLine();
  uint scancycle   = ppu.getScanCycle();
  bool isRendering = ppu.isRendering();
  if (scancycle == 260 && (scanline == 261 || scanline < 240) && isRendering) {
    if (this->reg.irq_counter == 0) {
      this->reg.irq_counter = this->reg.irq_latch;
    } else {
      this->reg.irq_counter--;
    }

    if (this->reg.irq_counter == 0 && this->reg.irq_enabled) {
      this->irq_trigger();
    }
  }
}
