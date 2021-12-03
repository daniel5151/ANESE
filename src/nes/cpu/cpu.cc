#include "cpu.h"

#include <cstdlib>
#include <cstdio>
#include <cassert>

/*-----------------------------  Public Methods  -----------------------------*/

CPU::CPU(const NES_Params& params, Memory& mem, InterruptLines& interrupt)
: interrupt(interrupt)
, mem(mem)
, print_nestest(params.log_cpu)
{
  this->power_cycle();
}

// https://wiki.nesdev.com/w/index.php/CPU_power_up_state
void CPU::power_cycle() {
  this->cycles = 0;

  this->reg.p.raw = 0x34; // 0b00110100, Interrupt = 1, Break = 1, Unused = 1

  this->reg.a = 0x00;
  this->reg.x = 0x00;
  this->reg.y = 0x00;

  this->reg.s = 0xFD;

  this->is_running = true;
}

void CPU::reset() {
  this->is_running = true;
  // RESET interrupt should be asserted exterally
}

/*----------------------------  Private Methods  -----------------------------*/

void CPU::service_interrupt(Interrupts::Type interrupt, bool brk /* = false */) {
  assert(interrupt != Interrupts::NONE);

#ifdef NESTEST
  // custom reset for headless nestest
  if (interrupt == Interrupts::RESET) {
    this->reg.pc = 0xC000;
    this->interrupt.service(interrupt);
    return;
  }
#endif

  // Push stack pointer and processor status onto stack for safekeeping
  if (interrupt != Interrupts::RESET) {
    this->s_push16(this->reg.pc);
    this->s_push(this->reg.p.raw);
  } else {
    this->reg.s -= 3;
  }

  // Interrupts take 7 cycles to execute
  this->cycles += 7;

  switch (interrupt) {
  case Interrupts::IRQ:
    if (brk || !this->reg.p.i) {
      this->reg.pc = this->read16(0xFFFE);
    }
    break;
  case Interrupts::RESET: this->reg.pc = this->read16(0xFFFC); break;
  case Interrupts::NMI:   this->reg.pc = this->read16(0xFFFA); break;
  default: break;
  }

  this->reg.p.i = true; // don't want interrupts being interrupted

  // Clear interrupt
  this->interrupt.service(interrupt);
}

u16 CPU::get_operand_addr(const Instructions::Opcode& opcode) {
  using namespace Instructions::AddrM;

  u16 addr = 0xBAD; // this is what we are trying to find...

  // To keep switch-statement code clean, define some temporary macros that
  // read 8 and 1 bit arguments (respectively)
  #define arg8  (this->mem[this->reg.pc++])
  #define arg16 (this->read16((this->reg.pc += 2) - 2))

  #define dummy_read() this->mem.read(this->reg.pc)

  switch(opcode.addrm) {
    case abs_: addr = arg16;                                             break;
    case absX: addr = arg16 + this->reg.x;                               break;
    case absY: addr = arg16 + this->reg.y;                               break;
    case ind_: addr = this->read16_zpg(arg16);                           break;
    case indY: addr = this->read16_zpg(arg8) + this->reg.y;              break;
    case Xind: addr = this->read16_zpg((arg8 + this->reg.x) & 0xFF);     break;
    case zpg_: addr = arg8;                                              break;
    case zpgX: addr = (arg8 + this->reg.x) & 0xFF;                       break;
    case zpgY: addr = (arg8 + this->reg.y) & 0xFF;                       break;
    case rel : dummy_read(); addr = this->reg.pc++;                      break;
    case imm : dummy_read(); addr = this->reg.pc++;                      break;
    case acc : dummy_read(); addr = this->reg.a;                         break;
    case impl: dummy_read(); addr = u8(0xFACA11);                        break;
    case INVALID:
      fprintf(stderr, "[CPU] Invalid Addressing Mode! Double check table!\n");
      // don't fail *just* yet, let it fail at the CPU instr decode switch
      return 0xBAD;
  }

  // Undefine temporary switch statement macros
  #undef arg16
  #undef arg8

  // Check to see if we need to add extra cycles due to crossing pages
  if (opcode.check_pg_cross == true) {
    // I checked, the only instructions affected by this are those with the
    // following 3 addressing modes. No need to handle any of the other modes
    #define did_pg_cross(a,b) (((a) & 0xFF00) != ((b) & 0xFF00))
    switch (opcode.addrm) {
    case absX: this->cycles += did_pg_cross(addr - this->reg.x, addr); break;
    case absY: this->cycles += did_pg_cross(addr - this->reg.y, addr); break;
    case indY: this->cycles += did_pg_cross(addr - this->reg.y, addr); break;
    default: break;
    }
    #undef did_pg_cross
  }

  return addr;
}

uint CPU::step() {
  uint old_cycles = this->cycles;

  // Service pending interrupts
  if (Interrupts::Type interrupt = this->interrupt.get()) {
    this->service_interrupt(interrupt);
    return this->cycles - old_cycles;
  }

  // Fetch current opcode
  u8 op = this->mem[this->reg.pc++];

  // Lookup info about opcode
  Instructions::Opcode opcode = Instructions::Opcodes[op];

  if (this->print_nestest) {
    this->nestest(*this, opcode);
  }
#ifdef NESTEST
  this->nestest(*this, opcode);
#endif

  u16 addr = this->get_operand_addr(opcode);

  using namespace Instructions::Instr;

  // Define some macros used across multiple instructions

  // Set Zero and Negative flags
  #define set_zn(val) \
    this->reg.p.z = val == 0; \
    this->reg.p.n = nth_bit(val, 7);

  // Branch if condition is satisfied
  #define branch(cond)                                                 \
    if (!cond) break;                                                  \
    i8 offset = i8(this->mem[addr]);                                   \
    /* Extra cycle on succesful branch */                              \
    this->cycles += 1;                                                 \
    /* Check if extra cycles due to jumping across pages */            \
    if ((this->reg.pc & 0xFF00) != ((this->reg.pc + offset) & 0xFF00)) \
      this->cycles += 1;                                               \
    this->reg.pc += offset;

  // This _may_ seem bad, but in reality, this switch statement actually gets
  // compiled down to a jump table!
  // Don't believe me? Check godbolt!
  switch (opcode.instr) {
      case ADC: { u8  val = this->mem[addr];
                  u16 sum = this->reg.a + val + this->reg.p.c;
                  this->reg.p.c = sum > 0xFF;
                  this->reg.p.z = u8(sum) == 0;
                  // http://www.righto.com/2012/12/the-6502-overflow-flag-explained.html
                  this->reg.p.v = ~(this->reg.a ^ val)
                                &  (this->reg.a ^ sum)
                                & 0x80;
                  this->reg.p.n = nth_bit(u8(sum), 7);
                  this->reg.a = u8(sum);
                } break;
      case AND: { this->reg.a &= this->mem[addr];
                  set_zn(this->reg.a);
                } break;
      case ASL: { if (opcode.addrm == Instructions::AddrM::acc) {
                    this->reg.p.c = nth_bit(this->reg.a, 7);
                    this->reg.a <<= 1;
                    set_zn(this->reg.a);
                  } else {
                    u8 val = this->mem[addr];
                    this->mem[addr] = val; // dummy-write
                    this->reg.p.c = nth_bit(val, 7);
                    val <<= 1;
                    set_zn(val);
                    this->mem[addr] = val;
                  }
                } break;
      case BCC: { branch(!this->reg.p.c);
                } break;
      case BCS: { branch(this->reg.p.c);
                } break;
      case BEQ: { branch(this->reg.p.z);
                } break;
      case BIT: { u8 mem = this->mem[addr];
                  this->reg.p.z = (this->reg.a & mem) == 0;
                  this->reg.p.v = nth_bit(mem, 6);
                  this->reg.p.n = nth_bit(mem, 7);
                } break;
      case BMI: { branch(this->reg.p.n);
                } break;
      case BNE: { branch(!this->reg.p.z);
                } break;
      case BPL: { branch(!this->reg.p.n);
                } break;
      case BRK: { // ignores interrupt disable bit, and forces an interrupt
                  this->service_interrupt(Interrupts::IRQ, true);
                } break;
      case BVC: { branch(!this->reg.p.v);
                } break;
      case BVS: { branch(this->reg.p.v);
                } break;
      case CLC: { this->reg.p.c = 0;
                } break;
      case CLD: { this->reg.p.d = 0;
                } break;
      case CLI: { this->reg.p.i = 0;
                } break;
      case CLV: { this->reg.p.v = 0;
                } break;
      case CMP: { u8 val = this->mem[addr];
                  this->reg.p.c = this->reg.a >= val;
                  set_zn(this->reg.a - val);
                } break;
      case CPX: { u8 val = this->mem[addr];
                  this->reg.p.c = this->reg.x >= val;
                  set_zn(this->reg.x - val);
                } break;
      case CPY: { u8 val = this->mem[addr];
                  this->reg.p.c = this->reg.y >= val;
                  set_zn(this->reg.y - val);
                } break;
      case DEC: { u8 val = this->mem[addr];
                  this->mem[addr] = val; // dummy-write
                  val--;
                  set_zn(val);
                  this->mem[addr] = val;
                } break;
      case DEX: { this->reg.x--;
                  set_zn(this->reg.x);
                } break;
      case DEY: { this->reg.y--;
                  set_zn(this->reg.y);
                } break;
      case EOR: { this->reg.a ^= this->mem[addr];
                  set_zn(this->reg.a);
                } break;
      case INC: { u8 val = this->mem[addr];
                  this->mem[addr] = val; // dummy-write
                  val++;
                  set_zn(val);
                  this->mem[addr] = val;
                } break;
      case INX: { this->reg.x++;
                  set_zn(this->reg.x);
                } break;
      case INY: { this->reg.y++;
                  set_zn(this->reg.y);
                } break;
      case JMP: { this->reg.pc = addr;
                } break;
      case JSR: { this->s_push16(this->reg.pc - 1);
                  this->reg.pc = addr;
                } break;
      case LDA: { this->reg.a = this->mem[addr];
                  set_zn(this->reg.a);
                } break;
      case LDX: { this->reg.x = this->mem[addr];
                  set_zn(this->reg.x);
                } break;
      case LDY: { this->reg.y = this->mem[addr];
                  set_zn(this->reg.y);
                } break;
      case LSR: { if (opcode.addrm == Instructions::AddrM::acc) {
                    this->reg.p.c = nth_bit(this->reg.a, 0);
                    this->reg.a >>= 1;
                    set_zn(this->reg.a);
                  } else {
                    u8 val = this->mem[addr];
                    this->mem[addr] = val; // dummy-write
                    this->reg.p.c = nth_bit(val, 0);
                    val >>= 1;
                    set_zn(val);
                    this->mem[addr] = val;
                  }
                } break;
      case NOP: { // me_irl
                } break;
      case ORA: { this->reg.a |= this->mem[addr];
                  set_zn(this->reg.a);
                } break;
      case PHA: { this->s_push(this->reg.a);
                } break;
      case PHP: { this->s_push(this->reg.p.raw | 0x30);
                } break;
      case PLA: { this->reg.a = this->s_pull();
                  set_zn(this->reg.a);
                } break;
      case PLP: { this->reg.p.raw = this->s_pull() | 0x20; // NESTEST
                } break;
      case ROL: { if (opcode.addrm == Instructions::AddrM::acc) {
                    bool old_bit_0 = nth_bit(this->reg.a, 7);
                    this->reg.a = (this->reg.a << 1) | u8(this->reg.p.c);
                    this->reg.p.c = old_bit_0;
                    set_zn(this->reg.a);
                  } else {
                    u8 val = this->mem[addr];
                    this->mem[addr] = val; // dummy-write
                    bool old_bit_0 = nth_bit(val, 7);
                    val = (val << 1) | u8(this->reg.p.c);
                    this->reg.p.c = old_bit_0;
                    set_zn(val);
                    this->mem[addr] = val;
                  }
                } break;
      case ROR: { if (opcode.addrm == Instructions::AddrM::acc) {
                    bool old_bit_0 = nth_bit(this->reg.a, 0);
                    this->reg.a = (this->reg.a >> 1) | (this->reg.p.c << 7);
                    this->reg.p.c = old_bit_0;
                    set_zn(this->reg.a);
                  } else {
                    u8 val = this->mem[addr];
                    this->mem[addr] = val; // dummy-write
                    bool old_bit_0 = nth_bit(val, 0);
                    val = (val >> 1) | (this->reg.p.c << 7);
                    this->reg.p.c = old_bit_0;
                    set_zn(val);
                    this->mem[addr] = val;
                  }
                } break;
      case RTI: { this->reg.p.raw = this->s_pull() | 0x20; // NESTEST
                  this->reg.pc = this->s_pull16();
                } break;
      case RTS: { this->reg.pc = this->s_pull16() + 1;
                } break;
      case SBC: { u8  val = this->mem[addr];
                  u16 sum = this->reg.a + ~val + this->reg.p.c;
                  this->reg.p.c = !(sum > 0xFF);
                  this->reg.p.z = u8(sum) == 0;
                  // http://www.righto.com/2012/12/the-6502-overflow-flag-explained.html
                  this->reg.p.v = ~(this->reg.a ^ ~val)
                                &  (this->reg.a ^ sum)
                                & 0x80;
                  this->reg.p.n = nth_bit(u8(sum), 7);
                  this->reg.a = u8(sum);
                } break;
      case SEC: { this->reg.p.c = 1;
                } break;
      case SED: { this->reg.p.d = 1;
                } break;
      case SEI: { this->reg.p.i = 1;
                } break;
      case STA: { this->mem[addr] = this->reg.a;
                } break;
      case STX: { this->mem[addr] = this->reg.x;
                } break;
      case STY: { this->mem[addr] = this->reg.y;
                } break;
      case TAX: { this->reg.x = this->reg.a;
                  set_zn(this->reg.x);
                } break;
      case TAY: { this->reg.y = this->reg.a;
                  set_zn(this->reg.y);
                } break;
      case TSX: { this->reg.x = this->reg.s;
                  set_zn(this->reg.x);
                } break;
      case TXA: { this->reg.a = this->reg.x;
                  set_zn(this->reg.a);
                } break;
      case TXS: { this->reg.s = this->reg.x;
                } break;
      case TYA: { this->reg.a = this->reg.y;
                  set_zn(this->reg.a);
                } break;
    default:
      fprintf(stderr,
        "[CPU] [%u] Unimplemented Instruction! 0x%02X\n",
        this->cycles,
        opcode.raw
      );
      this->is_running = false;
      break;
  }

  this->cycles += opcode.cycles;
  return this->cycles - old_cycles;
}

/*----------  Helpers  ----------*/

u8   CPU::s_pull()       { return this->mem[0x0100 + ++this->reg.s]; }
void CPU::s_push(u8 val) { this->mem[0x0100 + this->reg.s--] = val;  }

u16 CPU::s_pull16() {
  u16 lo = this->s_pull();
  u16 hi = this->s_pull();
  return (hi << 8) | lo;
}
void CPU::s_push16(u16 val) {
  this->s_push(val >> 8);   // push hi
  this->s_push(val & 0xFF); // push lo
}

u16 CPU::peek16(u16 addr) const {
  return this->mem.peek(addr + 0) |
        (this->mem.peek(addr + 1) << 8);
}
u16 CPU::read16(u16 addr) {
  return this->mem.read(addr + 0) |
        (this->mem.read(addr + 1) << 8);
}

u16 CPU::peek16_zpg(u16 addr) const {
  return this->mem.peek(addr + 0) |
        (this->mem.peek(((addr + 0) & 0xFF00) | ((addr + 1) & 0x00FF)) << 8);
}
u16 CPU::read16_zpg(u16 addr) {
  return this->mem.read(addr + 0) |
        (this->mem.read(((addr + 0) & 0xFF00) | ((addr + 1) & 0x00FF)) << 8);
}

void CPU::write16(u16 addr, u8 val) {
  this->mem.write(addr + 0, val);
  this->mem.write(addr + 1, val);
}
