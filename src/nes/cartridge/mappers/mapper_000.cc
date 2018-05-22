#include "mapper_000.h"

#include <cassert>
#include <cstdio>

Mapper_000::Mapper_000(const ROM_File& rom_file) : Mapper(0, "NROM") {
  this->mirror_mode = rom_file.meta.mirror_mode;

  // ---- PRG ROM ---- //

  if (rom_file.rom.prg.len == 0x8000) {
    // There are two banks, 8k each
    this->prg_lo = new ROM (0x4000, rom_file.rom.prg.data + 0x0000);
    this->prg_hi = new ROM (0x4000, rom_file.rom.prg.data + 0x4000);
  } else {
    // prg_lo == prg_hi
    this->prg_lo = new ROM (0x4000, rom_file.rom.prg.data + 0x0000);
    this->prg_hi = new ROM (0x4000, rom_file.rom.prg.data + 0x0000);
  }

  // ---- CHR ROM ---- //

  if (rom_file.rom.chr.len == 0)
    fprintf(stderr, "[Mapper_000] No CHR ROM detected. Using 8K CHR RAM\n");

  // If there is no chr_rom, then intitialize chr_ram
  this->chr_mem = (rom_file.rom.chr.len == 0x2000)
    ? (Memory*)new ROM (0x2000, rom_file.rom.chr.data)
    : (Memory*)new RAM (0x2000);
}

Mapper_000::~Mapper_000() {
  delete this->prg_lo;
  delete this->prg_hi;
  delete this->chr_mem;
}

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
