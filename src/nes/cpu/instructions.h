#pragma once

#include "common/util.h"

namespace Instructions {

namespace Instr {
enum Type {
  INVALID,
  ADC, AND, ASL, BCC, BCS, BEQ, BIT, BMI,
  BNE, BPL, BRK, BVC, BVS, CLC, CLD, CLI,
  CLV, CMP, CPX, CPY, DEC, DEX, DEY, EOR,
  INC, INX, INY, JMP, JSR, LDA, LDX, LDY,
  LSR, NOP, ORA, PHA, PHP, PLA, PLP, ROL,
  ROR, RTI, RTS, SBC, SEC, SED, SEI, STA,
  STX, STY, TAX, TAY, TSX, TXA, TXS, TYA,
};
}

namespace AddrM {
enum Type {
  INVALID,
  abs_, absX, absY, // Absolute (Indexed)
  ind_, indY, Xind, // Indirect (Indexed)
  zpg_, zpgX, zpgY, // Zero Page (Indexed)
  acc,  // Accumulator
  imm,  // Immediate
  impl, // Implied
  rel,  // Relative
};
}

struct Opcode {
  u8          raw;    // raw opcode byte
  Instr::Type instr;  // Instruction Enum
  AddrM::Type addrm;  // Addressing Mode Enum
  u8          cycles; // Base cycles

  bool check_pg_cross; // Should be an extra cycle on page cross?

  const char* instr_name; // Instruction Name
  const char* addrm_type; // Addressing Mode Name
};

// This macro magic makes the Opcode definition list look a lot nicer comapred
// to the raw alternative.
// Just compare a raw definition to a corresponding macro'd definition:
//
// raw:   /* 0x11 */ { Instr::ORA , AddrM::indY , 5 , true, "ORA", "indY" }
// macro: O( 0x11 , ORA , ind_ , 5 , * )
//
// PS: isn't it neat that I can use a * as a flag? pretty wicked imho

#include "common/overload_macro.h"
#define O(...) VFUNC(O, __VA_ARGS__)

// Defines a invalid Opcode
#define O1(opcode) \
  { opcode, Instr::INVALID , AddrM::INVALID , 0 , false , "KIL" , "----" }

// Defines a regular Opcode
#define O4(opcode, instr, addrm, cycles) \
  { opcode, Instr:: instr , AddrM:: addrm , cycles , false , #instr, #addrm }

// Defines a Opcode that takes 1 extra cycle when reading memory across pages
#define O5(opcode, instr, addrm, cycles, page_cross) \
  { opcode, Instr:: instr , AddrM:: addrm , cycles , true  , #instr, #addrm }

// Main Opcode lookup table
constexpr Opcode Opcodes[256] = {
O( 0x00 , BRK , impl , 0     ), // call to CPU::service_interrupt takes 7 cycles
O( 0x01 , ORA , Xind , 6     ),
O( 0x02                      ),
O( 0x03                      ),
O( 0x04                      ),
O( 0x05 , ORA , zpg_ , 3     ),
O( 0x06 , ASL , zpg_ , 5     ),
O( 0x07                      ),
O( 0x08 , PHP , impl , 3     ),
O( 0x09 , ORA , imm  , 2     ),
O( 0x0A , ASL , acc  , 2     ),
O( 0x0B                      ),
O( 0x0C                      ),
O( 0x0D , ORA , abs_ , 4     ),
O( 0x0E , ASL , abs_ , 6     ),
O( 0x0F                      ),
O( 0x10 , BPL , rel  , 2     ),
O( 0x11 , ORA , indY , 5 , * ),
O( 0x12                      ),
O( 0x13                      ),
O( 0x14                      ),
O( 0x15 , ORA , zpgX , 4     ),
O( 0x16 , ASL , zpgX , 6     ),
O( 0x17                      ),
O( 0x18 , CLC , impl , 2     ),
O( 0x19 , ORA , absY , 4 , * ),
O( 0x1A                      ),
O( 0x1B                      ),
O( 0x1C                      ),
O( 0x1D , ORA , absX , 4 , * ),
O( 0x1E , ASL , absX , 7     ),
O( 0x1F                      ),
O( 0x20 , JSR , abs_ , 6     ),
O( 0x21 , AND , Xind,  6     ),
O( 0x22                      ),
O( 0x23                      ),
O( 0x24 , BIT , zpg_ , 3     ),
O( 0x25 , AND , zpg_ , 3     ),
O( 0x26 , ROL , zpg_ , 5     ),
O( 0x27                      ),
O( 0x28 , PLP , impl , 4     ),
O( 0x29 , AND , imm  , 2     ),
O( 0x2A , ROL , acc  , 2     ),
O( 0x2B                      ),
O( 0x2C , BIT , abs_ , 4     ),
O( 0x2D , AND , abs_ , 4     ),
O( 0x2E , ROL , abs_ , 6     ),
O( 0x2F                      ),
O( 0x30 , BMI , rel  , 2     ),
O( 0x31 , AND , indY , 5 , * ),
O( 0x32                      ),
O( 0x33                      ),
O( 0x34                      ),
O( 0x35 , AND , zpgX , 4     ),
O( 0x36 , ROL , zpgX , 6     ),
O( 0x37                      ),
O( 0x38 , SEC , impl , 2     ),
O( 0x39 , AND , absY , 4 , * ),
O( 0x3A                      ),
O( 0x3B                      ),
O( 0x3C                      ),
O( 0x3D , AND , absX , 4 , * ),
O( 0x3E , ROL , absX , 7     ),
O( 0x3F                      ),
O( 0x40 , RTI , impl , 6     ),
O( 0x41 , EOR , Xind,  6     ),
O( 0x42                      ),
O( 0x43                      ),
O( 0x44                      ),
O( 0x45 , EOR , zpg_ , 3     ),
O( 0x46 , LSR , zpg_ , 5     ),
O( 0x47                      ),
O( 0x48 , PHA , impl , 3     ),
O( 0x49 , EOR , imm  , 2     ),
O( 0x4A , LSR , acc  , 2     ),
O( 0x4B                      ),
O( 0x4C , JMP , abs_ , 3     ),
O( 0x4D , EOR , abs_ , 4     ),
O( 0x4E , LSR , abs_ , 6     ),
O( 0x4F                      ),
O( 0x50 , BVC , rel  , 2     ),
O( 0x51 , EOR , indY , 5 , * ),
O( 0x52                      ),
O( 0x53                      ),
O( 0x54                      ),
O( 0x55 , EOR , zpgX , 4     ),
O( 0x56 , LSR , zpgX , 6     ),
O( 0x57                      ),
O( 0x58 , CLI , impl , 2     ),
O( 0x59 , EOR , absY , 4 , * ),
O( 0x5A                      ),
O( 0x5B                      ),
O( 0x5C                      ),
O( 0x5D , EOR , absX , 4 , * ),
O( 0x5E , LSR , absX , 7     ),
O( 0x5F                      ),
O( 0x60 , RTS , impl , 6     ),
O( 0x61 , ADC , Xind,  6     ),
O( 0x62                      ),
O( 0x63                      ),
O( 0x64                      ),
O( 0x65 , ADC , zpg_ , 3     ),
O( 0x66 , ROR , zpg_ , 5     ),
O( 0x67                      ),
O( 0x68 , PLA , impl , 4     ),
O( 0x69 , ADC , imm  , 2     ),
O( 0x6A , ROR , acc  , 2     ),
O( 0x6B                      ),
O( 0x6C , JMP , ind_,  5     ),
O( 0x6D , ADC , abs_ , 4     ),
O( 0x6E , ROR , abs_ , 6     ),
O( 0x6F                      ),
O( 0x70 , BVS , rel  , 2     ),
O( 0x71 , ADC , indY , 5 , * ),
O( 0x72                      ),
O( 0x73                      ),
O( 0x74                      ),
O( 0x75 , ADC , zpgX , 4     ),
O( 0x76 , ROR , zpgX , 6     ),
O( 0x77                      ),
O( 0x78 , SEI , impl , 2     ),
O( 0x79 , ADC , absY , 4 , * ),
O( 0x7A                      ),
O( 0x7B                      ),
O( 0x7C                      ),
O( 0x7D , ADC , absX , 4 , * ),
O( 0x7E , ROR , absX , 7     ),
O( 0x7F                      ),
O( 0x80                      ),
O( 0x81 , STA , Xind,  6     ),
O( 0x82                      ),
O( 0x83                      ),
O( 0x84 , STY , zpg_ , 3     ),
O( 0x85 , STA , zpg_ , 3     ),
O( 0x86 , STX , zpg_ , 3     ),
O( 0x87                      ),
O( 0x88 , DEY , impl , 2     ),
O( 0x89                      ),
O( 0x8A , TXA , impl , 2     ),
O( 0x8B                      ),
O( 0x8C , STY , abs_ , 4     ),
O( 0x8D , STA , abs_ , 4     ),
O( 0x8E , STX , abs_ , 4     ),
O( 0x8F                      ),
O( 0x90 , BCC , rel  , 2     ),
O( 0x91 , STA , indY , 6     ),
O( 0x92                      ),
O( 0x93                      ),
O( 0x94 , STY , zpgX , 4     ),
O( 0x95 , STA , zpgX , 4     ),
O( 0x96 , STX , zpgY , 4     ),
O( 0x97                      ),
O( 0x98 , TYA , impl , 2     ),
O( 0x99 , STA , absY , 5     ),
O( 0x9A , TXS , impl , 2     ),
O( 0x9B                      ),
O( 0x9C                      ),
O( 0x9D , STA , absX , 5     ),
O( 0x9E                      ),
O( 0x9F                      ),
O( 0xA0 , LDY , imm  , 2     ),
O( 0xA1 , LDA , Xind,  6     ),
O( 0xA2 , LDX , imm  , 2     ),
O( 0xA3                      ),
O( 0xA4 , LDY , zpg_ , 3     ),
O( 0xA5 , LDA , zpg_ , 3     ),
O( 0xA6 , LDX , zpg_ , 3     ),
O( 0xA7                      ),
O( 0xA8 , TAY , impl , 2     ),
O( 0xA9 , LDA , imm  , 2     ),
O( 0xAA , TAX , impl , 2     ),
O( 0xAB                      ),
O( 0xAC , LDY , abs_ , 4     ),
O( 0xAD , LDA , abs_ , 4     ),
O( 0xAE , LDX , abs_ , 4     ),
O( 0xAF                      ),
O( 0xB0 , BCS , rel  , 2     ),
O( 0xB1 , LDA , indY , 5 , * ),
O( 0xB2                      ),
O( 0xB3                      ),
O( 0xB4 , LDY , zpgX , 4     ),
O( 0xB5 , LDA , zpgX , 4     ),
O( 0xB6 , LDX , zpgY , 4     ),
O( 0xB7                      ),
O( 0xB8 , CLV , impl , 2     ),
O( 0xB9 , LDA , absY , 4 , * ),
O( 0xBA , TSX , impl , 2     ),
O( 0xBB                      ),
O( 0xBC , LDY , absX , 4 , * ),
O( 0xBD , LDA , absX , 4 , * ),
O( 0xBE , LDX , absY , 4 , * ),
O( 0xBF                      ),
O( 0xC0 , CPY , imm  , 2     ),
O( 0xC1 , CMP , Xind,  6     ),
O( 0xC2                      ),
O( 0xC3                      ),
O( 0xC4 , CPY , zpg_ , 3     ),
O( 0xC5 , CMP , zpg_ , 3     ),
O( 0xC6 , DEC , zpg_ , 5     ),
O( 0xC7                      ),
O( 0xC8 , INY , impl , 2     ),
O( 0xC9 , CMP , imm  , 2     ),
O( 0xCA , DEX , impl , 2     ),
O( 0xCB                      ),
O( 0xCC , CPY , abs_ , 4     ),
O( 0xCD , CMP , abs_ , 4     ),
O( 0xCE , DEC , abs_ , 6     ),
O( 0xCF                      ),
O( 0xD0 , BNE , rel  , 2     ),
O( 0xD1 , CMP , indY , 5 , * ),
O( 0xD2                      ),
O( 0xD3                      ),
O( 0xD4                      ),
O( 0xD5 , CMP , zpgX , 4     ),
O( 0xD6 , DEC , zpgX , 6     ),
O( 0xD7                      ),
O( 0xD8 , CLD , impl , 2     ),
O( 0xD9 , CMP , absY , 4 , * ),
O( 0xDA                      ),
O( 0xDB                      ),
O( 0xDC                      ),
O( 0xDD , CMP , absX , 4 , * ),
O( 0xDE , DEC , absX , 7     ),
O( 0xDF                      ),
O( 0xE0 , CPX , imm  , 2     ),
O( 0xE1 , SBC , Xind,  6     ),
O( 0xE2                      ),
O( 0xE3                      ),
O( 0xE4 , CPX , zpg_ , 3     ),
O( 0xE5 , SBC , zpg_ , 3     ),
O( 0xE6 , INC , zpg_ , 5     ),
O( 0xE7                      ),
O( 0xE8 , INX , impl , 2     ),
O( 0xE9 , SBC , imm  , 2     ),
O( 0xEA , NOP , impl , 2     ),
O( 0xEB                      ),
O( 0xEC , CPX , abs_ , 4     ),
O( 0xED , SBC , abs_ , 4     ),
O( 0xEE , INC , abs_ , 6     ),
O( 0xEF                      ),
O( 0xF0 , BEQ , rel  , 2     ),
O( 0xF1 , SBC , indY , 5 , * ),
O( 0xF2                      ),
O( 0xF3                      ),
O( 0xF4                      ),
O( 0xF5 , SBC , zpgX , 4     ),
O( 0xF6 , INC , zpgX , 6     ),
O( 0xF7                      ),
O( 0xF8 , SED , impl , 2     ),
O( 0xF9 , SBC , absY , 4 , * ),
O( 0xFA                      ),
O( 0xFB                      ),
O( 0xFC                      ),
O( 0xFD , SBC , absX , 4 , * ),
O( 0xFE , INC , absX , 7     ),
O( 0xFF                      ),
};

} // Instructions
