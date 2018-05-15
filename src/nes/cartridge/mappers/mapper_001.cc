#include "mapper_001.h"

#include <cassert>
#include <cstdio>

Mapper_001::Mapper_001(const ROM_File& rom_file)
: Mapper(rom_file)
, prg_ram(0x2000)
{
  // Parse the raw memory from the rom file into some ROM banks

  // Set up PRG Banks

  // Split PRG ROM into 16K banks
  this->banks.prg.len = this->rom_file.rom.prg.len / 0x4000;
  this->banks.prg.bank = new ROM* [this->banks.prg.len];

  fprintf(stderr, "[Mapper_001] 16K PRG ROM Banks: %d\n", this->banks.prg.len);

  const u8* prg_data_p = this->rom_file.rom.prg.data;
  for (uint i = 0; i < this->banks.prg.len; i++) {
    this->banks.prg.bank[i] = new ROM (0x4000, prg_data_p, "Mapper_001 PRG");
    prg_data_p += 0x4000;
  }

  // Split up CHR Banks

  if (this->rom_file.rom.chr.len != 0) {
    // Split CHR ROM into 4K banks
    this->banks.chr.len = this->rom_file.rom.chr.len / 0x1000;
    this->banks.chr.bank = new Memory* [this->banks.chr.len];

    fprintf(stderr, "[Mapper_001] 4K  CHR ROM Banks: %d\n", this->banks.chr.len);

    const u8* chr_data_p = this->rom_file.rom.chr.data;
    for (uint i = 0; i < this->banks.chr.len; i++) {
      this->banks.chr.bank[i] = new ROM (0x1000, chr_data_p, "Mapper_001 CHR");
      chr_data_p += 0x1000;
    }
  } else {
    // use CHR RAM
    fprintf(stderr, "[Mapper_001] No CHR ROM detected. Using 8K CHR RAM\n");

    this->banks.chr.len = 2;
    this->banks.chr.bank = new Memory* [2];
    this->banks.chr.bank[0] = new RAM (0x1000, "Mapper_001 CHR Lo");
    this->banks.chr.bank[1] = new RAM (0x1000, "Mapper_001 CHR Hi");
  }

  // Clear all registers to initial state
  this->reg.sr          = 0x10;
  this->reg.control.val = 0x00;
  this->reg.chr0.val    = 0x00;
  this->reg.chr1.val    = 0x00;
  this->reg.prg.val     = 0x00;

  // this isn't documented anywhere, but seems to be needed
  this->reg.control.prg_bank_mode = 3;

  // And check iNES header for initial mirroring mode
  switch(this->rom_file.meta.mirror_mode) {
  case Mirroring::SingleScreenLo: this->reg.control.mirroring = 0; break;
  case Mirroring::SingleScreenHi: this->reg.control.mirroring = 1; break;
  case Mirroring::Vertical:       this->reg.control.mirroring = 2; break;
  case Mirroring::Horizontal:     this->reg.control.mirroring = 3; break;
  default:
    fprintf(stderr, "[Mapper_001] Invalid initial mirroring mode!\n");
    assert(false);
    break;
  }

  this->update_banks();

  // ---- Emulation Vars ---- //
  this->write_just_happened = 0;
}

Mapper_001::~Mapper_001() {
  for (uint i = 0; i < this->banks.prg.len; i++)
    delete this->banks.prg.bank[i];
  delete[] this->banks.prg.bank;

  for (uint i = 0; i < this->banks.chr.len; i++)
    delete this->banks.chr.bank[i];
  delete[] this->banks.chr.bank;
}

u8 Mapper_001::read(u16 addr) {
  // Wired to the PPU MMU
  if (in_range(addr, 0x0000, 0x0FFF)) return this->chr_lo->read(addr - 0x0000);
  if (in_range(addr, 0x1000, 0x1FFF)) return this->chr_hi->read(addr - 0x1000);

  // Wired to the CPU MMU
  if (in_range(addr, 0x4020, 0x5FFF)) return 0x00; // Nothing in "Expansion ROM"
  if (in_range(addr, 0x6000, 0x7FFF)) return this->reg.prg.ram_enable == 0
                                           ? this->prg_ram.read(addr - 0x6000)
                                           : 0x00; // should be open bus...
  if (in_range(addr, 0x8000, 0xBFFF)) return this->prg_lo->read(addr - 0x8000);
  if (in_range(addr, 0xC000, 0xFFFF)) return this->prg_hi->read(addr - 0xC000);

  assert(false);
  return 0x00;
}

u8 Mapper_001::peek(u16 addr) const {
  // Wired to the PPU MMU
  if (in_range(addr, 0x0000, 0x0FFF)) return this->chr_lo->peek(addr - 0x0000);
  if (in_range(addr, 0x1000, 0x1FFF)) return this->chr_hi->peek(addr - 0x1000);

  // Wired to the CPU MMU
  if (in_range(addr, 0x4020, 0x5FFF)) return 0x00; // Nothing in "Expansion ROM"
  if (in_range(addr, 0x6000, 0x7FFF)) return this->reg.prg.ram_enable == 0
                                           ? this->prg_ram.peek(addr - 0x6000)
                                           : 0x00; // should be open bus...
  if (in_range(addr, 0x8000, 0xBFFF)) return this->prg_lo->peek(addr - 0x8000);
  if (in_range(addr, 0xC000, 0xFFFF)) return this->prg_hi->peek(addr - 0xC000);

  assert(false);
  return 0x00;
}

void Mapper_001::write(u16 addr, u8 val) {
  if (this->write_just_happened) return;
  // Why is it set to 2?
  // Next cycle, it will be decremented to 1, which causes the early return.
  // One more cycle, and it's back at 0, and writing is reenabled.
  this->write_just_happened = 2;

  // If writing to RAM, do it, and then return
  if (in_range(addr, 0x0000, 0x0FFF)) return this->chr_lo->write(addr - 0x0000, val);
  if (in_range(addr, 0x1000, 0x1FFF)) return this->chr_hi->write(addr - 0x1000, val);
  if (in_range(addr, 0x4020, 0x5FFF)) return; // do nothing to expansion ROM
  if (in_range(addr, 0x6000, 0x7FFF)) {
    // 0 means RAM is _enabled_. because fuck you that's why.
    if (this->reg.prg.ram_enable == 0) {
      this->prg_ram.write(addr - 0x6000, val);
    }
    return;
  }

  // Otherwise, handle writing to registers

  // "Unlike almost all other mappers, the MMC1 is configured through a serial
  //  port in order to reduce pin count." - Wiki
  // Yep, that's right! There is not direct writes to internal registers!
  // Instead, one carfeuly loads data into an internal shift register by writing
  // to the cartridge's address space 5 times, with the address of the final
  // write designating which internal register to load the shift register into!
  // WEEEEEE
  //
  // Anywhere from 0x8000 ... 0xFFFF
  // 7  bit  0
  // ---- ----
  // Rxxx xxxD
  // |       |
  // |       +- Data bit to be shifted into shift register, LSB first
  // +--------- 1: Reset shift register and write Control with (Control OR $0C),
  //               locking PRG ROM at $C000-$FFFF to the last bank.

  if (nth_bit(val, 7) == 1) {
    // Reset SR
    this->reg.sr = 0x10;
    this->reg.control.prg_bank_mode = 3;
  } else {
    bool done = this->reg.sr & 1; // sentinel bit
    // Just to make life interesting, the shift register is filled from LSB to
    // MSB, which complicates loading a little bit.
    this->reg.sr >>= 1;
    this->reg.sr |= (val & 1) << 4;
    if (done) {
      // Write the shift register to the appropriate internal register based on
      // what range this final write occured in.
      const u8 sr = this->reg.sr;
      if (in_range(addr, 0x8000, 0x9FFF)) { this->reg.control.val = sr; }
      if (in_range(addr, 0xA000, 0xBFFF)) { this->reg.chr0.val    = sr; }
      if (in_range(addr, 0xC000, 0xDFFF)) { this->reg.chr1.val    = sr; }
      if (in_range(addr, 0xE000, 0xFFFF)) { this->reg.prg.val     = sr; }
      this->reg.sr = 0x10; // and reset the shift-register

      this->update_banks();
    }
  }
}

void Mapper_001::update_banks() {
  // Update PRG Banks
  switch(u8(this->reg.control.prg_bank_mode)) {
  case 0: case 1: {
    // switch 32 KB at $8000, ignoring low bit of bank number
    this->prg_lo = this->banks.prg.bank[this->reg.prg.bank & 0xFE];
    this->prg_hi = this->banks.prg.bank[this->reg.prg.bank | 0x01];
  } break;
  case 2: {
    // fix first bank at $8000 and switch 16 KB bank at $C000;
    this->prg_lo = this->banks.prg.bank[0];
    this->prg_hi = this->banks.prg.bank[this->reg.prg.bank];
  } break;
  case 3: {
    // fix last bank at $C000 and switch 16 KB bank at $8000
    this->prg_lo = this->banks.prg.bank[this->reg.prg.bank];
    this->prg_hi = this->banks.prg.bank[this->banks.prg.len - 1];
  } break;
  default:
    // This should never happen. 2 bits == 4 possible states.
    fprintf(stderr, "[Mapper_001] Unhandled bank mode case %d. Dying...\n",
      u8(this->reg.control.prg_bank_mode));
    assert(false);
    break;
  }

  // Update CHR Banks
  const uint bank0 = this->reg.chr0.bank % this->banks.chr.len;
  const uint bank1 = this->reg.chr1.bank % this->banks.chr.len;
  if (this->reg.control.chr_bank_mode == 0) {
    // switch 8 KB at a time (ignoring low bit)
    this->chr_lo = this->banks.chr.bank[bank0 & 0xFE];
    this->chr_hi = this->banks.chr.bank[bank0 | 0x01];
  } else {
    // switch two separate 4 KB banks
    this->chr_lo = this->banks.chr.bank[bank0];
    this->chr_hi = this->banks.chr.bank[bank1];
  }
}

Mirroring::Type Mapper_001::mirroring() const {
  switch(u8(this->reg.control.mirroring)) {
  case 0: return Mirroring::SingleScreenLo;
  case 1: return Mirroring::SingleScreenHi;
  case 2: return Mirroring::Vertical;
  case 3: return Mirroring::Horizontal;
  default:
    // This should never happen. 2 bits == 4 possible states.
    fprintf(stderr, "[Mapper_001] Unhandled mirroring case %d. Dying...\n",
      u8(this->reg.control.mirroring));
    assert(false);
    break;
  }
}

void Mapper_001::cycle() {
  if (this->write_just_happened)
    this->write_just_happened--;
}
