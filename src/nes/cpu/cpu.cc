#include "cpu.h"

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
  u8 op = this->read(this->reg.pc);

  Instructions::Instr instr = Instructions::op_to_instr(op);
  Instructions::AddrM addrm = Instructions::op_to_addrm(op);

  printf("%04X  %02X ", this->reg.pc, op);

  // see commit 838afe4 for the "unsustainable" switch statement

  printf("A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3u\n",
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
