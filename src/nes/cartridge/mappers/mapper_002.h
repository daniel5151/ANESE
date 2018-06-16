#pragma once

#include "nes/generic/ram/ram.h"
#include "nes/generic/rom/rom.h"
#include "common/bitfield.h"
#include "common/util.h"
#include "../mapper.h"

// https://wiki.nesdev.com/w/index.php/UxROM
class Mapper_002 final : public Mapper {
private:
  // CPU Memory Space
  ROM* prg_lo;     // 0x8000 ... 0xBFFF - Switchable
  ROM* prg_hi;     // 0xC000 ... 0xFFFF - Fixed

  // PPU Memory Space
  Memory* chr_mem; // 0x0000 ... 0x1FFF

  struct { // Registers
    // Bank select - 0x8000 ... 0xFFFF
    // 7  bit  0
    // ---- ----
    // xxxx pPPP
    //      ||||
    //      ++++- Select 16 KB PRG ROM bank for CPU $8000-$BFFF
    //           (UNROM uses bits 2-0; UOROM uses bits 3-0)
    //
    // Emulator implementations of iNES mapper 2 treat this as a full 8-bit bank
    // select register, without bus conflicts.
    // This allows the mapper to be used for similar boards that are compatible.
    //
    // To make use of all 8-bits for a 4 MB PRG ROM, an NES 2.0 header must be
    // used (iNES can only effectively go to 2 MB).
    //
    // ANESE will just give it the entire 8 bits...
    //
    // The original UxROM boards used by Nintendo were subject to bus conflicts,
    // and the relevant games all work around this in software. Some emulators
    // (notably FCEUX) will have bus conflicts by default, but others have none.
    // NES 2.0 submappers were assigned to accurately specify whether the game
    // should be emulated with bus conflicts.
    u8 bank_select;
  } reg;

  Mirroring::Type mirror_mode;

  void update_banks() override;

  SERIALIZE_PARENT(Mapper)
  SERIALIZE_START(2, "Mapper_002")
    SERIALIZE_POD(mirror_mode)
    SERIALIZE_POD(reg)
  SERIALIZE_END(2)

  void reset() override;

public:
  Mapper_002(const ROM_File& rom_file);

  // <Memory>
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // <Memory/>

  Mirroring::Type mirroring() const override { return this->mirror_mode; };
};
