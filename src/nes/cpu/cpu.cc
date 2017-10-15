#include "cpu.h"

#include <cstdio>

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

  this->reg.sp = 0x5D;

  // Read initial PC from reset vector
  this->reg.pc = this->read_16(0xFFFC);
}

// https://wiki.nesdev.com/w/index.php/CPU_power_up_state
void CPU::reset() {
  this->reg.sp -= 3; // the stack pointer is decremented by 3 (weird...)
  this->reg.p.i = 1;

  // Read from reset vector
  this->reg.pc = this->read_16(0xFFFC);
}

/*----------  Helpers  ----------*/

u8 CPU::read(u16 addr) {
  return this->mem.read(addr);
}
void CPU::write(u16 addr, u8 val) {
  this->mem.write(addr, val);
}

u16 CPU::read_16(u16 addr) {
  return this->read(addr + 0) |
        (this->read(addr + 1) << 8);
}
void CPU::write_16(u16 addr, u16 val) {
  this->write(addr + 0, val);
  this->write(addr + 1, val);
}
