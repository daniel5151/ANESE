#include "joy.h"

#include <cassert>
#include <cstdio>

JOY::JOY()
: joy {nullptr}
{}

// TODO: more than 2 controller support
u8 JOY::read(u16 addr) {
  if (addr == 0x4016) return this->joy[0] ? this->joy[0]->read(addr) : 0x00;
  if (addr == 0x4017) return this->joy[1] ? this->joy[1]->read(addr) : 0x00;

  fprintf(stderr,
    "[JOY] Should not be able to read from 0x%04X! Check MMU!\n", addr
  );

  assert(false);
  return 0x00;
}

u8 JOY::peek(u16 addr) const {
  if (addr == 0x4016) return this->joy[0] ? this->joy[0]->peek(addr) : 0x00;
  if (addr == 0x4017) return this->joy[1] ? this->joy[1]->peek(addr) : 0x00;

  fprintf(stderr,
    "[JOY] Should not be able to peek from 0x%04X! Check MMU!\n", addr
  );

  assert(false);
  return 0x00;
}

void JOY::write(u16 addr, u8 val) {
  if (addr == 0x4016) {
    if (this->joy[0]) this->joy[0]->write(addr, val);
    if (this->joy[1]) this->joy[1]->write(addr, val);
  } else {
    fprintf(stderr,
      "[JOY] Should not be able to write to 0x%04X! Check MMU!\n", addr
    );
    assert(false);
  }
}

void JOY::attach_joy(uint port, Memory* joy) {
  assert(port < 2);
  this->joy[port] = joy;
}

void JOY::detach_joy(uint port) {
  assert(port < 2);
  this->joy[port] = nullptr;
}
