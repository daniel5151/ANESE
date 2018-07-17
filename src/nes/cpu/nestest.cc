#include "cpu.h"
#include "instructions.h"

#include <cstdio>

void CPU::nestest(const CPU& cpu, const Instructions::Opcode& opcode) {
  using namespace Instructions;

  // Print PC and raw opcode byte
  printf("%04X  %02X ", cpu.reg.pc - 1, opcode.raw);

  // create buffer for instruction operands
  char instr_buf [64];

  // Decode addressing mode
  u16 addr = 0;
  { // open a new scope to use AddrM namespace
  using namespace Instructions::AddrM;

  // Evaluate a few useful values
  u8  arg8   = cpu.mem.peek(cpu.reg.pc + 0);
  u8  arg8_2 = cpu.mem.peek(cpu.reg.pc + 1);
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
    case abs_: addr = arg16;                                              break;
    case absX: addr = arg16 + cpu.reg.x;                                  break;
    case absY: addr = arg16 + cpu.reg.y;                                  break;
    case ind_: addr = cpu.peek16_zpg(arg16);                              break;
    case indY: addr = cpu.peek16_zpg(arg8) + cpu.reg.y;                   break;
    case Xind: addr = cpu.peek16_zpg((arg8 + cpu.reg.x) & 0xFF);          break;
    case zpg_: addr = arg8;                                               break;
    case zpgX: addr = (arg8 + cpu.reg.x) & 0xFF;                          break;
    case zpgY: addr = (arg8 + cpu.reg.y) & 0xFF;                          break;
    case rel : addr = cpu.reg.pc;                                         break;
    case imm : addr = cpu.reg.pc;                                         break;
    case acc : addr = cpu.reg.a;                                          break;
    case impl: addr = u8(0xFACA11);                                       break;
    default: break;
  }

  // Print specific instrucion operands for each addressing mode
  switch(opcode.addrm) {
  case abs_: sprintf(instr_buf, "$%04X = %02X",
                           arg16,
              cpu.mem.peek(arg16)
            ); break;
  case absX: sprintf(instr_buf, "$%04X,X @ %04X = %02X",
                           arg16,
                       u16(arg16 + cpu.reg.x),
              cpu.mem.peek(arg16 + cpu.reg.x)
            ); break;
  case absY: sprintf(instr_buf, "$%04X,Y @ %04X = %02X",
                           arg16,
                       u16(arg16 + cpu.reg.y),
              cpu.mem.peek(arg16 + cpu.reg.y)
            ); break;
  case indY: sprintf(instr_buf, "($%02X),Y = %04X @ %04X = %02X",
                                          arg8,
                           cpu.peek16_zpg(arg8),
                       u16(cpu.peek16_zpg(arg8) + cpu.reg.y),
              cpu.mem.peek(cpu.peek16_zpg(arg8) + cpu.reg.y)
            ); break;
  case Xind: sprintf(instr_buf, "($%02X,X) @ %02X = %04X = %02X",
                                                         arg8,
                                          u8(cpu.reg.x + arg8),
                           cpu.peek16_zpg(u8(cpu.reg.x + arg8)),
              cpu.mem.peek(cpu.peek16_zpg(u8(cpu.reg.x + arg8)))
            ); break;
  case ind_: sprintf(instr_buf, "($%04X) = %04X",
                             arg16,
              cpu.peek16_zpg(arg16)
            ); break;
  case zpg_: sprintf(instr_buf, "$%02X = %02X",
                           arg8,
              cpu.mem.peek(arg8)
            ); break;
  case zpgX: sprintf(instr_buf, "$%02X,X @ %02X = %02X",
                              arg8,
                           u8(arg8 + cpu.reg.x),
              cpu.mem.peek(u8(arg8 + cpu.reg.x))
            ); break;
  case zpgY: sprintf(instr_buf, "$%02X,Y @ %02X = %02X",
                              arg8,
                           u8(arg8 + cpu.reg.y),
              cpu.mem.peek(u8(arg8 + cpu.reg.y))
            ); break;
  case rel : sprintf(instr_buf, "$%04X", cpu.reg.pc + 1 + i8(arg8));      break;
  case imm : sprintf(instr_buf, "#$%02X", arg8);                          break;
  case acc : sprintf(instr_buf, " ");                                     break;
  case impl: sprintf(instr_buf, " ");                                     break;
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
    cpu.reg.a,
    cpu.reg.x,
    cpu.reg.y,
    cpu.reg.p.raw & ~0x10, // 0b11101111, match nestest "golden" log
    cpu.reg.s,
    (cpu.cycles - 7) * 3 % 341 // CYC measures PPU X coordinates
                         // PPU does 1 x coordinate per cycle
                         // PPU runs 3x as fast as CPU
                         // ergo, multiply cycles by 3 should be fineee
  );
}
