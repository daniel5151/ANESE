#include "cpu.h"
#include "instructions.h"

#include <cstdio>

void CPU::nestest(const Instructions::Opcode& opcode) const {
  using namespace Instructions;

  // Print PC and raw opcode byte
  printf("%04X  %02X ", this->reg.pc - 1, opcode.raw);

  // create buffer for instruction operands
  char instr_buf [64];

  // Decode addressing mode
  u16 addr = 0;
  { // open a new scope to use AddrM namespace
  using namespace Instructions::AddrM;

  // Evaluate a few useful values
  u8  arg8   = this->mem.peek(this->reg.pc + 0);
  u8  arg8_2 = this->mem.peek(this->reg.pc + 1);
  u16 arg16  = arg8 | (arg8_2 << 8);

  // Print operand bytes
  const char* iname = opcode.instr_name;
  switch(opcode.addrm) {
    case abs_:
    case absX:
    case absY:
    case ind_:
      printf("%02X %02X", arg8, arg8_2);
      break;
    case indY:
    case Xind:
    case zpg_:
    case zpgX:
    case zpgY:
    case rel :
    case imm :
      printf("%02X   "  , arg8);
      break;
    default:
      printf("     ");
      break;
  }

  // Print Instruction Name
  printf("  %s ", iname);

  // Decode addressing mode
  switch(opcode.addrm) {
    case abs_: addr = arg16;                                             break;
    case absX: addr = arg16 + this->reg.x;                               break;
    case absY: addr = arg16 + this->reg.y;                               break;
    case ind_: addr = this->mem.peek16_zpg(arg16);                       break;
    case indY: addr = this->mem.peek16_zpg(arg8) + this->reg.y;          break;
    case Xind: addr = this->mem.peek16_zpg((arg8 + this->reg.x) & 0xFF); break;
    case zpg_: addr = arg8;                                              break;
    case zpgX: addr = (arg8 + this->reg.x) & 0xFF;                       break;
    case zpgY: addr = (arg8 + this->reg.y) & 0xFF;                       break;
    case rel : addr = this->reg.pc;                                      break;
    case imm : addr = this->reg.pc;                                      break;
    case acc : addr = this->reg.a;                                       break;
    case impl: addr = u8(0xFACA11);                                      break;
    default: break;
  }

  // Print specific instrucion operands for each addressing mode
  switch(opcode.addrm) {
  case abs_: sprintf(instr_buf, "$%04X = %02X",
                             arg16,
              this->mem.peek(arg16)
            ); break;
  case absX: sprintf(instr_buf, "$%04X,X @ %04X = %02X",
                             arg16,
                         u16(arg16 + this->reg.x),
              this->mem.peek(arg16 + this->reg.x)
            ); break;
  case absY: sprintf(instr_buf, "$%04X,Y @ %04X = %02X",
                             arg16,
                         u16(arg16 + this->reg.y),
              this->mem.peek(arg16 + this->reg.y)
            ); break;
  case indY: sprintf(instr_buf, "($%02X),Y = %04X @ %04X = %02X",
                                                  arg8,
                             this->mem.peek16_zpg(arg8),
                         u16(this->mem.peek16_zpg(arg8) + this->reg.y),
              this->mem.peek(this->mem.peek16_zpg(arg8) + this->reg.y)
            ); break;
  case Xind: sprintf(instr_buf, "($%02X,X) @ %02X = %04X = %02X",
                                                                   arg8,
                                                  u8(this->reg.x + arg8),
                             this->mem.peek16_zpg(u8(this->reg.x + arg8)),
              this->mem.peek(this->mem.peek16_zpg(u8(this->reg.x + arg8)))
            ); break;
  case ind_: sprintf(instr_buf, "($%04X) = %04X",
                                   arg16,
              this->mem.peek16_zpg(arg16)
            ); break;
  case zpg_: sprintf(instr_buf, "$%02X = %02X",
                             arg8,
              this->mem.peek(arg8)
            ); break;
  case zpgX: sprintf(instr_buf, "$%02X,X @ %02X = %02X",
                                arg8,
                             u8(arg8 + this->reg.x),
              this->mem.peek(u8(arg8 + this->reg.x))
            ); break;
  case zpgY: sprintf(instr_buf, "$%02X,Y @ %02X = %02X",
                                arg8,
                             u8(arg8 + this->reg.y),
              this->mem.peek(u8(arg8 + this->reg.y))
            ); break;
  case rel : sprintf(instr_buf, "$%04X", this->reg.pc + 1 + i8(arg8));   break;
  case imm : sprintf(instr_buf, "#$%02X", arg8);                         break;
  case acc : sprintf(instr_buf, " ");                                    break;
  case impl: sprintf(instr_buf, " ");                                    break;
  default: sprintf(instr_buf, " "); break;
  }

  } // close AddrM scope

  // handle a few edge cases
  switch (opcode.instr) {
    case Instr::JMP:
    case Instr::JSR:
      if (opcode.addrm == AddrM::abs_)
        sprintf(instr_buf, "$%04X", addr);
      break;
    case Instr::LSR:
    case Instr::ASL:
    case Instr::ROR:
    case Instr::ROL:
      if (opcode.addrm == AddrM::acc)
        sprintf(instr_buf, "A");
      break;
    default: break;
  }

  // Print instruction operands
  printf("%-28s", instr_buf);

  // Print processor state
  printf("A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3u\n",
    this->reg.a,
    this->reg.x,
    this->reg.y,
    this->reg.p.raw & ~0x10, // 0b11101111, match nestest "golden" log
    this->reg.s,
    this->cycles * 3 % 341 // CYC measures PPU X coordinates
                           // PPU does 1 x coordinate per cycle
                           // PPU runs 3x as fast as CPU
                           // ergo, multiply cycles by 3 should be fineee
  );
}
