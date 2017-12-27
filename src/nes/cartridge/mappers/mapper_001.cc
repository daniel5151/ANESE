#include "mapper_001.h"

#include <cassert>
#include <cstdio>

Mapper_001::Mapper_001(const ROM_File& rom_file)
: Mapper(rom_file)
, prg_ram(0x1000)
{
  // Parse the raw memory from the rom file into some ROM banks

  // this small alias makes the code easier to read
  const auto& raw_rom = this->rom_file.rom;

  // Split PRG ROM into 16K banks
  this->banks.prg.len = raw_rom.prg.len / 0x4000;
  this->banks.prg.bank = new ROM* [this->banks.prg.len];

  fprintf(stderr, "[Mapper_001] 16K PRG ROM Banks: %d\n", this->banks.prg.len);

  const u8* prg_data_p = raw_rom.prg.data;
  for (uint i = 0; i < this->banks.prg.len; i++) {
    this->banks.prg.bank[i] = new ROM (0x4000, prg_data_p, "Mapper_001 PRG");
    prg_data_p += 0x4000;
  }

  if (this->rom_file.rom.chr.len != 0) {
    // Split CHR ROM into 4K banks
    this->banks.chr.len = raw_rom.chr.len / 0x1000;
    this->banks.chr.bank = new ROM* [this->banks.chr.len];

    fprintf(stderr, "[Mapper_001] 4K  CHR ROM Banks: %d\n", this->banks.prg.len);

    const u8* chr_data_p = raw_rom.chr.data;
    for (uint i = 0; i < this->banks.chr.len; i++) {
      this->banks.chr.bank[i] = new ROM (0x1000, chr_data_p, "Mapper_001 CHR");
      chr_data_p += 0x1000;
    }
  } else {
    // use CHR RAM
    fprintf(stderr, "[Mapper_001] 8K  CHR RAM\n");
    this->chr_lo = new RAM (0x1000, "Mapper_001 CHR Lo");
    this->chr_hi = new RAM (0x1000, "Mapper_001 CHR Hi");
  }

  // Clear all registers to initial state
  this->reg.sr = 0;
  this->reg.control.val = 0x00;
  this->reg.chr0.val    = 0x00;
  this->reg.chr1.val    = 0x00;
  this->reg.prg.val     = 0x00;

  // this isn't documented anywhere, but seems to be needed
  this->reg.control.prg_bank_mode = 3;

  this->update_banks();

  // ---- Emulation Vars ---- //
  this->write_just_happened = 0;
}

Mapper_001::~Mapper_001() {
  for (uint i = 0; i < this->banks.prg.len; i++)
    delete this->banks.prg.bank[i];
  delete[] this->banks.prg.bank;

  if (this->rom_file.rom.chr.len != 0) {
    for (uint i = 0; i < this->banks.chr.len; i++)
      delete this->banks.chr.bank[i];
    delete[] this->banks.chr.bank;
  } else {
    delete this->chr_lo;
    delete this->chr_hi;
  }
}

u8 Mapper_001::read(u16 addr) {
  // Wired to the PPU MMU
  if (in_range(addr, 0x0000, 0x0FFF)) return this->chr_lo->read(addr - 0x0000);
  if (in_range(addr, 0x1000, 0x1FFF)) return this->chr_hi->read(addr - 0x1000);

  // Wired to the CPU MMU
  if (in_range(addr, 0x4020, 0x5FFF)) return 0x00; // Nothing in "Expansion ROM"
  if (in_range(addr, 0x6000, 0x7FFF)) return this->reg.prg.ram_enable
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
  if (in_range(addr, 0x6000, 0x7FFF)) return this->reg.prg.ram_enable
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
  if (in_range(addr, 0x0000, 0x0FFF)) {
    this->chr_lo->write(addr - 0x0000, val);
    return;
  }
  if (in_range(addr, 0x1000, 0x1FFF)) {
    this->chr_hi->write(addr - 0x1000, val);
    return;
  }
  if (in_range(addr, 0x6000, 0x7FFF)) {
    if (this->reg.prg.ram_enable) {
      this->prg_ram.write(addr - 0x6000, val);
    }
    return;
  }

  // Otherwise, handle writing to registers

  // The shift register is "cleared" to 0x01 to make it easiy to track when the
  // final write is happening (by checking bit 5 of the sr for a value of 1)

  if (nth_bit(val, 7) == 1) {
    this->reg.sr = 0x01;
    this->reg.control.prg_bank_mode = 3;
    this->update_banks();
    return;
  }

  // The shift register is actually filled left-to-right, i.e: from LSB to MSB,
  // but that is hard to do in c, so instead, i'm going to fill it from the MSB
  // to LSB, and just give it a quick reversearoo later
  this->reg.sr = (this->reg.sr << 1) | (val & 1);

  // If 5 writes have not been performed yet, return;
  if (nth_bit(this->reg.sr, 5) == 0)
    return;


  // Otherwise, write the shift register to the appropriate internal register
  // based on what range this final write occured in
  // But first, do the 'ol reversaroo on the shift register...
  const u8 true_val = reverse_u8(this->reg.sr & 0x1F) >> (8 - 5);
  if (in_range(addr, 0x8000, 0x9FFF)) this->reg.control.val = true_val;
  if (in_range(addr, 0xA000, 0xBFFF)) this->reg.chr0.val    = true_val;
  if (in_range(addr, 0xC000, 0xDFFF)) this->reg.chr1.val    = true_val;
  if (in_range(addr, 0xE000, 0xFFFF)) this->reg.prg.val     = true_val;

  this->reg.sr = 0x01; // reset SR

  this->update_banks();
}

void Mapper_001::update_banks() {
  // Update PRG Banks
  switch(u8(this->reg.control.prg_bank_mode)) {
  case 0: case 1: {
    // switch 32 KB at $8000, ignoring low bit of bank number
    const u8 bank = (this->reg.prg.bank & 0xFE) % this->banks.prg.len;
    assert(bank + 1 < this->banks.prg.len);
    this->prg_lo = this->banks.prg.bank[bank + 0];
    this->prg_hi = this->banks.prg.bank[bank + 1];
  } break;
  case 2: {
    const u8 bank = this->reg.prg.bank % this->banks.prg.len;
    // fix first bank at $8000 and switch 16 KB bank at $C000;
    this->prg_lo = this->banks.prg.bank[0];
    this->prg_hi = this->banks.prg.bank[bank];
  } break;
  case 3: {
    const u8 bank = this->reg.prg.bank % this->banks.prg.len;
    // fix last bank at $C000 and switch 16 KB bank at $8000
    this->prg_lo = this->banks.prg.bank[bank];
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
  if (this->rom_file.rom.chr.len != 0) {
    if (this->reg.control.chr_bank_mode == 0) {
      // switch 8 KB at a time
      const u8 bank = this->reg.chr0.bank & 0xFE;
      assert(bank + 1 < this->banks.chr.len);
      this->chr_lo = this->banks.chr.bank[bank + 0];
      this->chr_hi = this->banks.chr.bank[bank + 1];
    } else {
      // switch two separate 4 KB banks
      assert(this->reg.chr0.bank < this->banks.chr.len);
      assert(this->reg.chr1.bank < this->banks.chr.len);
      this->chr_lo = this->banks.chr.bank[this->reg.chr0.bank];
      this->chr_hi = this->banks.chr.bank[this->reg.chr1.bank];
    }
  } else {
    // No need to do any switching with CHR RAM (?)
  }

  // Set mirroring mode
  switch(u8(this->reg.control.mirroring)) {
  case 0: this->mirror_mode = Mirroring::SingleScreenLo; break;
  case 1: this->mirror_mode = Mirroring::SingleScreenHi; break;
  case 2: this->mirror_mode = Mirroring::Vertical;       break;
  case 3: this->mirror_mode = Mirroring::Horizontal;     break;
  default:
    // This should never happen. 2 bits == 4 possible states.
    fprintf(stderr, "[Mapper_001] Unhandled mirroring case %d. Dying...\n",
      u8(this->reg.control.mirroring));
    assert(false);
    break;
  }
}

Mirroring::Type Mapper_001::mirroring() const { return this->mirror_mode; }

void Mapper_001::cycle() {
  if (this->write_just_happened)
    this->write_just_happened--;
}
