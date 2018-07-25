#pragma once

#include "nes/generic/ram/ram.h"
#include "nes/generic/rom/rom.h"
#include "common/bitfield.h"
#include "common/callback_manager.h"
#include "common/util.h"
#include "../mapper.h"

// https://wiki.nesdev.com/w/index.php/MMC3
class Mapper_004 final : public Mapper {
private:
  // CPU Memory Space

  // 0x6000 ... 0x7FFF: 8 KB PRG RAM bank
  RAM  prg_ram;
  // 0x8000 ... 0x9FFF (or $C000-$DFFF): 8 KB switchable PRG ROM bank
  // 0xA000 ... 0xBFFF:                  8 KB switchable PRG ROM bank
  // 0xC000 ... 0xDFFF (or $8000-$9FFF): 8 KB PRG ROM bank, fixed to the second-last bank
  // 0xE000 ... 0xFFFF:                  8 KB PRG ROM bank, fixed to the last bank
  ROM* prg_bank [4];

  // PPU Memory Space

  // 0x0000 0x07FF (or $1000-$17FF): 2 KB switchable CHR bank
  // 0x0800 0x0FFF (or $1800-$1FFF): 2 KB switchable CHR bank
  // 0x1000 0x13FF (or $0000-$03FF): 1 KB switchable CHR bank
  // 0x1400 0x17FF (or $0400-$07FF): 1 KB switchable CHR bank
  // 0x1800 0x1BFF (or $0800-$0BFF): 1 KB switchable CHR bank
  // 0x1C00 0x1FFF (or $0C00-$0FFF): 1 KB switchable CHR bank
  Memory* chr_bank [8];

  // 4 Screen Mirroring RAM
  RAM* four_screen_ram;

  struct { // Registers
    // Bank select - 0x8000 ... $9FFE, even
    // 7  bit  0
    // ---- ----
    // CPMx xRRR
    // |||   |||
    // |||   +++- Specify which bank register to update on next write to
    // |||         Bank Data register
    // |||        0: Select 2 KB CHR bank at PPU $0000-$07FF (or $1000-$17FF);
    // |||        1: Select 2 KB CHR bank at PPU $0800-$0FFF (or $1800-$1FFF);
    // |||        2: Select 1 KB CHR bank at PPU $1000-$13FF (or $0000-$03FF);
    // |||        3: Select 1 KB CHR bank at PPU $1400-$17FF (or $0400-$07FF);
    // |||        4: Select 1 KB CHR bank at PPU $1800-$1BFF (or $0800-$0BFF);
    // |||        5: Select 1 KB CHR bank at PPU $1C00-$1FFF (or $0C00-$0FFF);
    // |||        6: Select 8 KB PRG ROM bank at $8000-$9FFF (or $C000-$DFFF);
    // |||        7: Select 8 KB PRG ROM bank at $A000-$BFFF
    // ||+------- Nothing on the MMC3, see MMC6
    // |+-------- PRG ROM bank mode (0: $8000-$9FFF swappable,
    // |                                $C000-$DFFF fixed to second-last bank;
    // |                             1: $C000-$DFFF swappable,
    // |                                $8000-$9FFF fixed to second-last bank)
    // +--------- CHR A12 inversion (0: two 2 KB banks at $0000-$0FFF,
    //                                  four 1 KB banks at $1000-$1FFF;
    //                               1: two 2 KB banks at $1000-$1FFF,
    //                                  four 1 KB banks at $0000-$0FFF)
    union {
      u8 val;
      BitField<7>    chr_inversion;
      BitField<6>    prg_rom_mode;
    //BitField<5>    unused in MMC3;
    //BitField<3, 2> unused;
      BitField<0, 3> bank;
    } bank_select;

    // Bank select - 0x8000 ... $9FFF, odd
    // 7  bit  0
    // ---- ----
    // DDDD DDDD
    // |||| ||||
    // ++++-++++- New bank value, based on last value written to
    //             Bank select register (mentioned above)
    u8 bank_values [8];

    // Mirroring - 0xA000 ... 0xBFFE, even
    // 7  bit  0
    // ---- ----
    // xxxx xxxM
    //         |
    //         +- Nametable mirroring (0: vertical; 1: horizontal)
    union {
      u8 val;
      BitField<0> mode;
    } mirroring;

    // PRG RAM protect - 0xA001 ... 0xBFFF, odd
    // 7  bit  0
    // ---- ----
    // RWXX xxxx
    // ||||
    // ||++------ Nothing on the MMC3, see MMC6
    // |+-------- Write protection (0: allow writes; 1: deny writes)
    // +--------- PRG RAM chip enable (0: disable; 1: enable)
    union {
      u8 val;
      BitField<7>    enable_ram;
      BitField<6>    write_enable;
    //BitField<4, 2> unused in MMC3;
    } ram_protect;

    u8 irq_counter;

    // IRQ latch - 0xC000 ... 0xDFFE, even
    // 7  bit  0
    // ---- ----
    // DDDD DDDD
    // |||| ||||
    // ++++-++++- IRQ latch value
    //
    // This register specifies the IRQ counter reload value.
    // When the IRQ counter is zero (or a reload is requested through $C001),
    // this value will be copied to the IRQ counter at the NEXT rising edge of
    // the PPU address, presumably at PPU cycle 260 of the current scanline.
    u8 irq_latch;

    // IRQ reload - 0xC001 ... 0xDFFF, odd
    // Writing any value to this register reloads the MMC3 IRQ counter at the
    // NEXT rising edge of the PPU address, presumably at PPU cycle 260 of the
    // current scanline.

    // IRQ Disable - 0xE000 ... 0xFFFE, even
    // IRQ Enable  - 0xE001 ... 0xFFFF, odd
    // Note: Disabling IRQ will disable MMC3 interrupts AND
    // acknowledge any pending interrupts.
    bool irq_enabled;
  } reg;

  // ---- Emulation Vars and Helpers ---- //

  bool last_A12 = false; // bit 12 of last CHR Memory read, used for IRQ

  bool fourscreen_mirroring = false;

  void update_banks() override;

  void power_cycle() override;
  void reset() override;

  SERIALIZE_PARENT(Mapper)
  SERIALIZE_START(3, "Mapper_004")
    SERIALIZE_SERIALIZABLE(prg_ram)
    SERIALIZE_SERIALIZABLE_PTR(four_screen_ram)
    SERIALIZE_POD(reg)
  SERIALIZE_END(3)

public:
  Mapper_004(const ROM_File& rom_file);
  ~Mapper_004();

  // <Memory>
  u8 read(u16 addr)       override;
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // <Memory/>

  Mirroring::Type mirroring() const override;

  const Serializable::Chunk* getBatterySave() const override {
    return this->prg_ram.serialize();
  }

  void setBatterySave(const Serializable::Chunk* c) override {
    this->prg_ram.deserialize(c);
  }

  u8 peek_irq_latch() const { return this->reg.irq_latch; }

  CallbackManager<Mapper_004*, bool> _did_irq_callbacks;
};
