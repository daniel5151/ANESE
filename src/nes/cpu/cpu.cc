#include "cpu.h"

#include <cstdio>
#include <cassert>

CPU::~CPU() {}

CPU::CPU(Memory& mem)
: mem(mem)
{
  this->power_cycle();
}

// https://wiki.nesdev.com/w/index.php/CPU_power_up_state
void CPU::power_cycle() {
  this->reg.p.raw = 0b00110100; // Interrupt = 1, Break = 1

  this->reg.a = 0x00;
  this->reg.x = 0x00;
  this->reg.y = 0x00;

  this->reg.s = 0x5D;

  // Read initial PC from reset vector
  // this->reg.pc = this->read_16(0xFFFC);

  // >> SET TO 0xC000 to do nestest.rom
  this->reg.pc = 0xC000;


  this->cycles = 0;
  this->state = CPU::State::Running;
}

// https://wiki.nesdev.com/w/index.php/CPU_power_up_state
void CPU::reset() {
  this->reg.s -= 3; // the stack pointer is decremented by 3 (weird...)
  this->reg.p.i = 1;

  // Read from reset vector
  // this->reg.pc = this->read_16(0xFFFC);

  // >> SET TO 0xC000 to do nestest.rom
  this->reg.pc = 0xC000;


  this->cycles = 0;
  this->state = CPU::State::Running;
}

CPU::State CPU::getState() const { return this->state; }

#include <string>

u8 CPU::step() {
  u32 old_cycles = this->cycles;

  // Fetch instruction
  u8 instr = this->read(this->reg.pc);

  printf("%04X  %02X ", this->reg.pc, instr);
  const char* byte_1 = "       %s                             ";
  const char* byte_2 = "%02X     %s %-28s";
  const char* byte_3 = "%02X %02X  %s %-28s";

  static char* args = new char [32];
  static char* mode = new char [32];

  switch (instr) {
  case 0x78: {
    // Set the interrupt disable flag to one.

    this->reg.p.i = 1;

    this->reg.pc += 1;

    this->cycles += 2;

    sprintf(mode, "Implied");
    printf(byte_1, "SEI");
  } break;

  case 0xD8: {
    // Sets the decimal mode flag to zero.

    this->reg.p.i = 0;

    this->reg.pc += 1;

    this->cycles += 2;

    sprintf(mode, "Implied");
    printf(byte_1, "CLD");
  } break;

  case 0xA2: {
    // Loads a byte of memory into the X register setting the zero and negative
    // flags as appropriate.

    u8 constant = this->read(this->reg.pc + 1);

    this->reg.p.z = constant == 0;
    this->reg.p.n = nth_bit(constant, 7);
    this->reg.x = constant;

    this->reg.pc += 2;

    this->cycles += 2;

    sprintf(mode, "Immediate");
    sprintf(args, "#$%02X", constant);
    printf(byte_2, constant, "LDX", args);
  } break;

  case 0x9A: {
    // Copies the current contents of the X register into the accumulator and
    // sets the zero and negative flags as appropriate.

    this->reg.s = this->reg.x;

    this->reg.pc += 1;

    this->cycles += 2;

    sprintf(mode, "Implied");
    printf(byte_1, "TXS");
  } break;

  case 0xAD: {
    // Loads a byte of memory into the accumulator setting the zero and negative
    // flags as appropriate.

    u16 addr = this->read_16(this->reg.pc + 1);

    u8 val = this->read(addr);
    this->reg.p.z = val == 0;
    this->reg.p.n = nth_bit(this->reg.a, 7);
    this->reg.a = val;

    this->reg.pc += 3;

    this->cycles += 4;

    sprintf(mode, "Absolute");
    sprintf(args, "$%04X", addr);
    printf(byte_3, addr & 0x00FF, (addr & 0xFF00) >> 8, "LDA", args);
  } break;

  case 0x10: {
    // If the zero flag is clear then add the relative displacement to the
    // program counter to cause a branch to a new location.

    u8 offset = this->read(this->reg.pc + 1);

    i8 offset_int = offset;

    u16 old_addr = this->reg.pc + 2;
    u16 new_addr = old_addr + offset_int;

    if (!this->reg.p.n) {
      this->cycles += 1; // add cycle for successful branch

      // Check for extra cycles due to crossing page boundaries
      if ((new_addr & 0x00FF) != (old_addr & 0x00FF)) {
        this->cycles += 2;
      }

      this->reg.pc += offset_int;
    } else {
      this->reg.pc += 2;
    }

    this->cycles += 2;

    sprintf(mode, "Relative");
    sprintf(args, "$%04X", new_addr);
    printf(byte_2, offset, "BPL", args);
  } break;

  case 0x4C: {
    // Sets the program counter to the address specified by the operand.

    u16 addr = this->read_16(this->reg.pc + 1);

    this->reg.pc = addr;

    this->cycles += 3;

    sprintf(mode, "Absolute");
    sprintf(args, "$%04X", addr);
    printf(byte_3, addr & 0x00FF, (addr & 0xFF00) >> 8, "JMP", args);
  } break;

  case 0x86: {
    // Stores the contents of the X register into memory.

    u8 addr = this->read(this->reg.pc + 1);

    this->write(addr, this->reg.x);

    this->reg.pc += 2;

    this->cycles += 3;

    sprintf(mode, "ZeroPage");
    sprintf(args, "$%02X = %02X", addr, this->reg.x);
    printf(byte_2, addr, "STX", args);
  } break;

  case 0x20: {
    // Pushes the address (minus one) of the return point on to the stack and
    // then sets the program counter to the target memory address.

    u16 addr = this->read_16(this->reg.pc + 1);

    this->s_push_16(this->reg.pc + 3 - 1);

    this->reg.pc = addr;

    this->cycles += 6;

    sprintf(mode, "Absolute");
    sprintf(args, "$%04X", addr);
    printf(byte_3, addr & 0x00FF, (addr & 0xFF00) >> 8, "JSR", args);
  } break;

  case 0xEA: {
    // Increments the program counter to the next instruction.

    this->reg.pc += 1;

    this->cycles += 2;

    sprintf(mode, "Implied");
    printf(byte_1, "NOP");
  } break;

  default:
    sprintf(mode, "");
    printf("       !Unimplemented Instruction!     ");
    this->state = CPU::State::Halted;
  }

  printf("A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3u | %-12s\n",
    this->reg.a,
    this->reg.x,
    this->reg.y,
    this->reg.p.raw & 0b11101111, // match nestest "golden" log
    this->reg.s,
    old_cycles * 3 % 341, // CYC measures PPU X coordinates
                          // PPU does 1 x coordinate per cycle
                          // PPU runs 3x as fast as CPU
                          // ergo, multiply cycles by 3 should be fineee
    mode
  );

  return this->cycles - old_cycles;
}

/*----------  Helpers  ----------*/

u8   CPU::read (u16 addr)         { return this->mem.read(addr); }
void CPU::write(u16 addr, u8 val) { this->mem.write(addr, val);  }

u16 CPU::read_16(u16 addr) {
  return this->read(addr + 0) |
        (this->read(addr + 1) << 8);
}

void CPU::write_16(u16 addr, u16 val) {
  this->write(addr + 0, val);
  this->write(addr + 1, val);
}

u8   CPU::s_pull()       { return this->read(0x0100 + this->reg.s++); }
void CPU::s_push(u8 val) { this->write(0x0100 + this->reg.s--, val);  }

u16 CPU::s_pull_16() {
  u8 lo = this->s_pull();
  u8 hi = this->s_pull();
  return (hi << 8) | lo;
}
void CPU::s_push_16(u16 val) {
  this->s_push(val >> 8); // push hi
  this->s_push(val);      // push lo
}
