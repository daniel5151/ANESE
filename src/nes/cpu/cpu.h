#pragma once

#include "common/util.h"
#include "common/interfaces/memory.h"
#include "common/bitfield.h"
#include "instructions.h"

#include "nes/wiring/interrupt_lines.h"

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

  InterruptLines& interrupt;

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

  /*----------  Emulation Vars  ----------*/

  uint cycles;  // Cycles elapsed
  State state; // CPU state

  /*--------------  Helpers  -------------*/

  u16 get_operand_addr(const Instructions::Opcode& opcode);

  void service_interrupt(Interrupts::Type type, bool brk = false);

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
  CPU(Memory& mem, InterruptLines& interrupt);

  void power_cycle();
  void reset();

  CPU::State getState() const;

  uint step(); // exec instruction, and return cycles taken
};
