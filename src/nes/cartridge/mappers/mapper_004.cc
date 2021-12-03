#include "mapper_004.h"

#include <cassert>
#include <cstdio>
#include <cstring>

Mapper_004::Mapper_004(const ROM_File& rom_file)
: Mapper(4, "MMC3", rom_file, 0x2000, 0x400)
, prg_ram(0x2000)
{
  this->fourscreen_mirroring = rom_file.meta.mirror_mode == Mirroring::FourScreen;

  if (this->fourscreen_mirroring) {
    this->four_screen_ram = new RAM (0x1000, "Mapper_004 FourScreen RAM");
  } else {
    this->four_screen_ram = nullptr;
  }
}

Mapper_004::~Mapper_004() {
  if (this->four_screen_ram)
    delete four_screen_ram;
}

u8 Mapper_004::read(u16 addr) {
  // Wired to the PPU MMU
  if (in_range(addr, 0x0000, 0x1FFF)) {
    // The MMC3 scanline counter is based entirely on PPU A12,
    // being clocked on A12's rising edge
    bool next = nth_bit(addr, 12);
    if (this->last_A12 == 0 && next == 1) { // Rising Edge
      if (this->reg.irq_counter == 0) {
        this->reg.irq_counter = this->reg.irq_latch;
      } else {
        this->reg.irq_counter--;
      }

      if (this->reg.irq_counter == 0) {
        if (this->reg.irq_enabled)
          this->irq_trigger();

        _did_irq_callbacks.run(this, this->reg.irq_enabled);
      }
    }
    this->last_A12 = next;

    return this->chr_bank[addr / 0x400]->read(addr % 0x400);
  }

  // 4 Screen RAM
  if (in_range(addr, 0x2000, 0x2FFF)) {
    assert(this->four_screen_ram);
    return this->four_screen_ram->read(addr - 0x2000);
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
  if (in_range(addr, 0x0000, 0x1FFF)) {
    return this->chr_bank[addr / 0x400]->peek(addr % 0x400);
  }

  // 4 Screen RAM
  if (in_range(addr, 0x2000, 0x2FFF)) {
    assert(this->four_screen_ram);
    return this->four_screen_ram->peek(addr - 0x2000);
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
  // 4 Screen RAM
  if (in_range(addr, 0x2000, 0x2FFF)) {
    assert(this->four_screen_ram);
    return this->four_screen_ram->write(addr - 0x2000, val);
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
    this->prg_bank[i] = &this->get_prg_bank(val);
  if (this->reg.bank_select.prg_rom_mode == 0) {
    PBANK(0, this->reg.bank_values[6]);
    PBANK(1, this->reg.bank_values[7]);
    PBANK(2, this->get_prg_bank_len() - 2);
    PBANK(3, this->get_prg_bank_len() - 1);
  } else {
    PBANK(0, this->get_prg_bank_len() - 2);
    PBANK(1, this->reg.bank_values[7]);
    PBANK(2, this->reg.bank_values[6]);
    PBANK(3, this->get_prg_bank_len() - 1);
  }
  #undef PBANK

  // https://wiki.nesdev.com/w/index.php/MMC3#CHR_Banks
  #define CBANK(i, val) \
    this->chr_bank[i] = &this->get_chr_bank(val);
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

void Mapper_004::power_cycle() {
  this->last_A12 = false;
  Mapper::power_cycle();
}

void Mapper_004::reset() {
  memset((char*)&this->reg, 0, sizeof this->reg);
}
