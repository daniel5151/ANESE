#pragma once

#include "nes/generic/ram/ram.h"
#include "nes/generic/rom/rom.h"
#include "common/bitfield.h"
#include "common/util.h"
#include "../mapper.h"

// https://wiki.nesdev.com/w/index.php/MMC2
// Only used for Mike Tyson's Punchout
class Mapper_009 final : public Mapper {
private:
  // CPU Memory Space
  // 0x6000 ... 0x7FFF - Fixed RAM
  RAM  prg_ram;
  // 0x8000 ... 0x9FFF - Switchable 8K bank
  // 0xA000 ... 0xFFFF - Fixed
  ROM* prg_rom [4];

  // PPU Memory Space
  struct {
    Memory* lo [2]; // 0x0000 ... 0x0FFF - Switchable b/w 2 4k banks
    Memory* hi [2]; // 0x1000 ... 0x1FFF - Switchable b/w 2 4k banks
  } chr_rom;

  struct { // Registers
    // PRG ROM bank select ($A000-$AFFF)
    // 7  bit  0
    // ---- ----
    // xxxx PPPP
    //      ||||
    //      ++++- Select 8 KB PRG ROM bank for CPU $8000-$9FFF
    union {
      u8 val;
      BitField<0, 4> bank;
    } prg;

    bool latch [2]; // never directly written to by CPU/PPU
    // CHR ROM $FD/0000 bank select ($B000-$BFFF)
    // CHR ROM $FE/0000 bank select ($C000-$CFFF)
    // CHR ROM $FD/1000 bank select ($D000-$DFFF)
    // CHR ROM $FE/1000 bank select ($E000-$EFFF)
    // 7  bit  0
    // ---- ----
    // xxxC CCCC
    //    | ||||
    //    +-++++- Select 4 KB CHR ROM bank for PPU $0000/$1000-$0FFF/$1FFF
    //            used when latch 0/1 = $FD/$FE
    struct {
      union {
        u8 val;
        BitField<0, 5> bank;
      } lo [2], hi [2];
    } chr;

    // Mirroring ($F000-$FFFF)
    // 7  bit  0
    // ---- ----
    // xxxx xxxM
    //         |
    //         +- Select nametable mirroring (0: vertical; 1: horizontal)
    bool mirroring;
  } reg;

  void update_banks() override;

  void reset() override;

  SERIALIZE_PARENT(Mapper)
  SERIALIZE_START(2, "Mapper_009")
    SERIALIZE_SERIALIZABLE(prg_ram)
    SERIALIZE_POD(reg)
  SERIALIZE_END(2)

public:
  Mapper_009(const ROM_File& rom_file);

  // <Memory>
  u8 read(u16 addr) override;
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
};
