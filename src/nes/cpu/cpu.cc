#include "cpu.h"

#include <cstdlib>
#include <cstdio>
#include <cassert>

#include "instructions.h"

CPU::~CPU() {}

CPU::CPU(IMemory& mem)
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

  this->reg.s = 0xFD;

  // Read initial PC from reset vector
  // this->reg.pc = this->mem_read16(0xFFFC);

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
  // this->reg.pc = this->mem_read16(0xFFFC);

  // >> SET TO 0xC000 to do nestest.rom
  this->reg.pc = 0xC000;


  this->cycles = 0;
  this->state = CPU::State::Running;
}

CPU::State CPU::getState() const { return this->state; }

// ... NESTEST DEBUG ... //
static char nestest_buf [64];

u16 CPU::get_instr_args(Instructions::Opcode& opcode) {
  using namespace Instructions::AddrM;

  // We want to return a 8 / 16 bit number back to the CPU that can be used by
  // the Instruction
  u16 retval;

  // There are two sizes of argument:
  u8  argV; // 1 byte - Value, or zero page address
  u16 argA; // 2 bytes - Addresses

  // The 6502 always reads 1 argument byte, regardless of the addressing mode.
  argV = this->mem_read(this->reg.pc);

  // We don't want to eagerly read another memory address, since that would
  // cause side-effects. Instead, to keep code clean, we define `GET_argA` as
  // a function-specific macro that does the extra read on a case-by-case basis
  argA = -1; // defaults to invalid value
  #define read_argA() \
    (argA = (u16(argV) + (this->mem_read(this->reg.pc + 1) << 8)))

  // Define temporary macro to slim down bulky, repeating switch statement code
  #define M(mode, inc_pc, ...) \
    case mode: \
      retval = (__VA_ARGS__); \
      this->reg.pc += inc_pc; \
      /* ... NESTEST DEBUG ...  */ \
      if (inc_pc == 0) printf("       %s ", opcode.instr_name); \
      if (inc_pc == 1) printf("%02X     %s ", argV, opcode.instr_name); \
      if (inc_pc == 2) printf("%02X %02X  %s ", argV, argA >> 8, opcode.instr_name); \
      break;

  switch(opcode.addrm) {
    M(abs_, 2, read_argA()                                                     )
    M(absX, 2, read_argA() + this->reg.x                                       )
    M(absY, 2, read_argA() + this->reg.y                                       )
    M(ind_, 2, this->mem_read16(this->mem_read16(read_argA()))                 )
    M(indY, 1, this->mem_read16(this->mem_read16(argV) + this->reg.y)          )
    M(Xind, 1, this->mem_read16(this->mem_read16((argV + this->reg.x) & 0xFF)) )
    M(zpg_, 1, argV                                                            )
    M(zpgX, 1, (argV + this->reg.x) & 0xFF                                     )
    M(zpgY, 1, (argV + this->reg.y) & 0xFF                                     )
    M(rel , 1, argV                                                            )
    M(imm , 1, argV                                                            )
    M(acc , 0, this->reg.a                                                     )
    M(impl, 0, u8(0xFACA11) /* no args! return fack all :D */                  )
    case INVALID:
      fprintf(stderr, "Invalid Addressing Mode! Check opcode table for!\n");
      exit(-1);
      break;
  }


  // ... NESTEST DEBUG ... //
  switch(opcode.addrm) {
  case abs_: sprintf(nestest_buf, "$%04X ", argA); break;
  case absX: sprintf(nestest_buf, " "); break;
  case absY: sprintf(nestest_buf, " "); break;
  case ind_: sprintf(nestest_buf, " "); break;
  case indY: sprintf(nestest_buf, " "); break;
  case Xind: sprintf(nestest_buf, " "); break;
  case zpg_: sprintf(nestest_buf, "$%02X = %02X", argV, this->mem.peek(argV)); break;
  case zpgX: sprintf(nestest_buf, "($%02X,X) ", argV); break;
  case zpgY: sprintf(nestest_buf, "($%02X),Y ", argV); break;
  case rel : sprintf(nestest_buf, "$%04X ", this->reg.pc + i8(argV)); break;
  case imm : sprintf(nestest_buf, "#$%02X ", argV); break;
  case acc : sprintf(nestest_buf, " "); break;
  case impl: sprintf(nestest_buf, " "); break;
  default: break;
  }

  // Undefine switch statement macros
  #undef M
  #undef read_argA

  // Check to see if we need to add extra cycles due to crossing pages
  if (opcode.check_pg_cross == true) {
    // We know a page boundary was crossed when the calculated addr was of the
    // form $xxFF, since trying to read a 16 bit address from memory at that
    // address would cross into the next page (eg: $12FF -> $1300 crosses pages)
    if ((argA & 0xFF) == 0xFF) {
      this->cycles += 1;
    }
  }

  return retval;
}

u8 CPU::step() {
  u32 old_cycles = this->cycles;

  // Fetch current opcode
  u8 op = this->mem_read(this->reg.pc++);

  // Lookup info about opcode
  Instructions::Opcode opcode = Instructions::Opcodes[op];

  // ... NESTEST DEBUG ... //
  char INITIAL_STATE[64];
  sprintf(INITIAL_STATE, "A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3u\n",
    this->reg.a,
    this->reg.x,
    this->reg.y,
    this->reg.p.raw & 0b11101111, // match nestest "golden" log
    this->reg.s,
    old_cycles * 3 % 341 // CYC measures PPU X coordinates
                         // PPU does 1 x coordinate per cycle
                         // PPU runs 3x as fast as CPU
                         // ergo, multiply cycles by 3 should be fineee
  );

  // ... NESTEST DEBUG ... //
  printf("%04X  %02X ", this->reg.pc - 1, op);

  // Depending on what addrm this instruction uses, this will either be a u8
  // or a u16. Thus, we use a u16 to get the value from the fn, and let
  // individual instructions cast it to u8 when they need to.
  u16 arg = this->get_instr_args(opcode);

  /* EXECUTE INSTRUCTION */
  using namespace Instructions::Instr;

  switch (opcode.instr) {
    case JMP: { this->reg.pc = arg;
              } break;
    case LDX: { this->reg.x = arg;
                this->reg.p.z = this->reg.x == 0;
                this->reg.p.n = nth_bit(this->reg.x, 7);
              } break;
    case STX: { this->mem_write(arg, this->reg.x);
              } break;
    case JSR: { this->s_push16(this->reg.pc);
                this->reg.pc = arg;
              } break;
    case NOP: { // me_irl
              } break;
    case SEC: { this->reg.p.c = 1;
              } break;
    case BCS: { if (this->reg.p.c) this->branch(arg);
              } break;
    case CLC: { this->reg.p.c = 0;
              } break;
    case BCC: { if (!this->reg.p.c) this->branch(arg);
              } break;
    case LDA: { this->reg.a = arg;
                this->reg.p.z = this->reg.a == 0;
                this->reg.p.n = nth_bit(this->reg.a, 7);
              } break;
    case BEQ: { if (this->reg.p.z) this->branch(arg);
              } break;
    case BNE: { if (!this->reg.p.z) this->branch(arg);
              } break;
    case STA: { this->mem_write(arg, this->reg.a);
              } break;
    case BIT: { u8 mem = this->mem.read(arg);
                this->reg.p.z = (this->reg.a & mem) == 0;
                this->reg.p.v = nth_bit(mem, 6);
                this->reg.p.n = nth_bit(mem, 7);
              } break;
    case BVS: { if (this->reg.p.v) this->branch(arg);
              } break;
    case BVC: { if (!this->reg.p.v) this->branch(arg);
              } break;
    case BPL: { if (!this->reg.p.n) this->branch(arg);
              } break;
    case RTS: { this->reg.pc = this->s_pull16();
              } break;
    case AND: { this->reg.a &= arg;
                this->reg.p.z = this->reg.a == 0;
                this->reg.p.n = nth_bit(this->reg.a, 7);
              } break;
    case SEI: { this->reg.p.i = 1;
              } break;
    case SED: { this->reg.p.d = 1;
              } break;
    case PHP: { this->s_push(this->reg.p.raw);
              } break;
    case PLA: { this->reg.a = this->s_pull();
                this->reg.p.z = this->reg.a == 0;
                this->reg.p.n = nth_bit(this->reg.a, 7);
              } break;
    case CMP: { this->reg.p.c = this->reg.a >= arg;
                this->reg.p.z = this->reg.a == arg;
                this->reg.p.n = nth_bit(this->reg.a - arg, 7);
              } break;
    case CLD: { this->reg.p.d = 0;
              } break;
    case PHA: { this->s_push(this->reg.a);
              } break;
    case PLP: { this->reg.p.raw = this->s_pull() | 0x20; // NESTEST
              } break;
    default: fprintf(stderr, "Unimplemented Instruction!\n"); exit(-1);
  }

  printf("%-28s", nestest_buf);

  /* NEED TO IMPLEMENT INTERUPTS BEFORE CONTINUING! */

  // ... NESTEST DEBUG ... //
  printf("%s", INITIAL_STATE);

  this->cycles += opcode.cycles;
  return this->cycles - old_cycles;
}

/*----------  Helpers  ----------*/

u8   CPU::mem_read (u16 addr)         { return this->mem.read(addr); }
void CPU::mem_write(u16 addr, u8 val) { this->mem.write(addr, val);  }

u16 CPU::mem_read16(u16 addr) {
  return this->mem_read(addr + 0) |
        (this->mem_read(addr + 1) << 8);
}

void CPU::mem_write16(u16 addr, u16 val) {
  this->mem_write(addr + 0, val);
  this->mem_write(addr + 1, val);
}

u8   CPU::s_pull()       { return this->mem_read(0x0100 + ++this->reg.s); }
void CPU::s_push(u8 val) { this->mem_write(0x0100 + this->reg.s--, val);  }

u16 CPU::s_pull16() {
  u16 lo = this->s_pull();
  u16 hi = this->s_pull();
  return (hi << 8) | lo;
}
void CPU::s_push16(u16 val) {
  this->s_push(val >> 8); // push hi
  this->s_push(val);      // push lo
}

void CPU::branch(u8 offset) {
  this->cycles += 1;
  if ((this->reg.pc & 0xFF00) != ((this->reg.pc + offset) & 0xFF00))
    this->cycles += 2;
  this->reg.pc += i8(offset);
}
