#pragma once

#include "common/util.h"
#include "common/interfaces/memory.h"
#include "common/bitfield.h"
#include "instructions.h"

// MOS 6502 (no BCD)
// https://wiki.nesdev.com/w/index.php/CPU
// http://users.telenet.be/kim1-6502/6502
// http://obelisk.me.uk/6502
class CPU {
public:
  enum class State {
    Running,
    Halted
  };

private:
  /*----------  Hardware  ----------*/

  Memory& mem; // Memory

  struct { // Registers
    // -- Special Registers -- //
    u16 pc; // Program Counter
    u8  s;  // Stack Pointer (offset from 0x0100)

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

  // instructions and addressing modes are defined in instructions.h

  /*----------  Emulation Vars  ----------*/

  u32 cycles;  // Cycles elapsed
  State state; // CPU state

  /*--------------  Helpers  -------------*/

  u16 get_operand_addr(const Instructions::Opcode& opcode);

  // Read / Write from mem
  u8  mem_read       (u16 addr); // read 1 byte
  u16 mem_read16     (u16 addr); // read 2 bytes
  u16 mem_read16_zpg (u16 addr); // read 2 bytes, handling zero page bug
  void mem_write     (u16 addr, u8  val); // write 1 byte
  void mem_write16   (u16 addr, u16 val); // write 2 bytes

  // Push / Pop from Stack
  u8   s_pull  ();
  u16  s_pull16();
  void s_push  (u8  val);
  void s_push16(u16 val);

  /*-------------  Testing  --------------*/

  // print nestest golden-log formatted CPU log data
  void nestest(const Instructions::Opcode& opcode) const;

public:
  ~CPU();
  CPU(Memory& mem);
  void init_next();

  void power_cycle();
  void reset();

  CPU::State getState() const;

  u8 step();
};
