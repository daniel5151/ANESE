#pragma once

#include "common/util.h"
#include "common/interfaces/memory.h"
#include "common/bitfield.h"

// MOS 6502 (no BCD)
// https://wiki.nesdev.com/w/index.php/CPU
class CPU {
private:
  /*----------  Hardware  ----------*/

  Memory& mem; // Memory

  struct { // Registers
    // -- Special Registers -- //
    u16 pc; // Program Counter
    u8  sp; // Stack Pointer

    union { // Processor Status
      u8 raw; // underlying byte
      BitField<7> n; // Negative
      BitField<6> v; // Overflow
    //BitField<5> _; // 1
      BitField<4> b; // Break   (not actually in hardware)
      BitField<3> d; // Decimal (ignored by instructions, but still set)
      BitField<2> i; // Interrupt
      BitField<1> z; // Zero
      BitField<0> c; // Carry
    } p;

    // -- General Registers -- //
    u8 a; // Accumulator
    u8 x; // Index X
    u8 y; // Index Y
  } reg;

  /*----------  Emulation Vars  ----------*/

  u64 cycles; // Cycles elapsed

  bool is_running;

  /*--------------  Helpers  -------------*/

  u8  read     (u16 addr);
  u16 read_16  (u16 addr);
  void write   (u16 addr, u8  val);
  void write_16(u16 addr, u16 val);

public:
  ~CPU();
  CPU(Memory& mem);

  void power_cycle();
  void reset();
};
