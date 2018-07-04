#pragma once

#include "common/bitfield.h"
#include "common/serializable.h"
#include "common/util.h"
#include "instructions.h"
#include "nes/interfaces/memory.h"

#include "nes/wiring/interrupt_lines.h"

#include "nes/params.h"

// MOS 6502 (no BCD)
// https://wiki.nesdev.com/w/index.php/CPU
// http://users.telenet.be/kim1-6502/6502
// http://obelisk.me.uk/6502
class CPU final : public Serializable {
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

  uint cycles; // Cycles elapsed
  bool is_running;

  SERIALIZE_START(3, "CPU")
    SERIALIZE_POD(reg)
    SERIALIZE_POD(cycles)
    SERIALIZE_POD(is_running)
  SERIALIZE_END(3)

  /*--------------  Helpers  -------------*/

  u16 get_operand_addr(const Instructions::Opcode& opcode);

  void service_interrupt(Interrupts::Type type, bool brk = false);

  // Push / Pop from Stack
  u8   s_pull  ();
  u16  s_pull16();
  void s_push  (u8  val);
  void s_push16(u16 val);

  // 16-bit memory accesses
  u16 peek16(u16 addr) const;
  u16 read16(u16 addr);
  u16 peek16_zpg(u16 addr) const;
  u16 read16_zpg(u16 addr);
  void write16(u16 addr, u8 val);

  /*-------------  Debug  --------------*/
  const bool& print_nestest;
  // nestest is implemented in nestest.cc
  static void nestest(const CPU& cpu, const Instructions::Opcode& opcode);

public:
  CPU() = delete;
  CPU(const NES_Params& params, Memory& mem, InterruptLines& interrupt);

  void power_cycle();
  void reset();

  bool isRunning() const { return this->is_running; }

  uint step(); // exec instruction, and return cycles taken
};
