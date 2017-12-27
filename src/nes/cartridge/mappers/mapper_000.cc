#include "mapper_000.h"

#include <cassert>

Mapper_000::Mapper_000(const ROM_File& rom_file)
: Mapper(rom_file)
{
  // a small alias, to make the code easier to read
  const auto& rom = this->rom_file.rom;

  // ---- PRG ROM ---- //

  if (rom.prg.len == 0x8000) {
    // There are two banks, 8k each
    this->lo_rom = new ROM (0x4000, rom.prg.data + 0x0000);
    this->hi_rom = new ROM (0x4000, rom.prg.data + 0x4000);
  } else {
    // lo_rom == hi_rom
    this->lo_rom = new ROM (0x4000, rom.prg.data + 0x0000);
    this->hi_rom = new ROM (0x4000, rom.prg.data + 0x0000);
  }

  // ---- CHR ROM ---- //

  // If there is no chr_rom, then intitialize chr_ram
  this->chr_mem = (rom.chr.len == 0x2000)
    ? (Memory*)new ROM (0x2000, rom.chr.data)
    : (Memory*)new RAM (0x2000);
}

Mapper_000::~Mapper_000() {
  delete this->lo_rom;
  delete this->hi_rom;
  delete this->chr_mem;
}

u8 Mapper_000::read(u16 addr) {
  // reading has no side-effects
  return this->peek(addr);
}

u8 Mapper_000::peek(u16 addr) const {
  // Wired to the PPU MMU
  if (in_range(addr, 0x0000, 0x1FFF)) return this->chr_mem->peek(addr);

  // Wired to the CPU MMU
  if (in_range(addr, 0x4020, 0x5FFF)) return 0x00; // Nothing in "Expansion ROM"
  if (in_range(addr, 0x6000, 0x7FFF)) return 0x00; // Nothing in SRAM
  if (in_range(addr, 0x8000, 0xBFFF)) return this->lo_rom->peek(addr - 0x8000);
  if (in_range(addr, 0xC000, 0xFFFF)) return this->hi_rom->peek(addr - 0xC000);

  assert(false);
  return 0;
}

void Mapper_000::write(u16 addr, u8 val) {
  // Since there is potentially CHR RAM, try to write to it (if in range)
  if (in_range(addr, 0x0000, 0x1FFF)) {
    this->chr_mem->write(addr, val);
  }
}

Mirroring::Type Mapper_000::mirroring() const {
  return this->rom_file.meta.mirror_mode;
};
