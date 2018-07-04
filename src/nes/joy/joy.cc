#include "joy.h"

#include <cassert>
#include <cstdio>

u8 JOY::read(u16 addr) {
  // Why | 0x40?
  // On the NES and Famicom, the top three (or five) bits are Open Bus.
  // Usually, the last value on the bus prior to accessing the controlleris is
  // the byte of the controller port address, i.e: 0x40.
  // Certain games (such as Paperboy) rely on this behavior and require that
  // reads from the controller ports return exactly $40 or $41 as appropriate.
  if (addr == 0x4016) return this->joy[0] ? this->joy[0]->read(addr) | 0x40 : 0;
  if (addr == 0x4017) return this->joy[1] ? this->joy[1]->read(addr) | 0x40 : 0;

  fprintf(stderr,
    "[JOY] Should not be able to read from 0x%04X! Check MMU!\n", addr
  );

  assert(false);
  return 0x00;
}

u8 JOY::peek(u16 addr) const {
  if (addr == 0x4016) return this->joy[0] ? this->joy[0]->peek(addr) | 0x40 : 0;
  if (addr == 0x4017) return this->joy[1] ? this->joy[1]->peek(addr) | 0x40 : 0;

  fprintf(stderr,
    "[JOY] Should not be able to peek from 0x%04X! Check MMU!\n", addr
  );

  assert(false);
  return 0x00;
}

void JOY::write(u16 addr, u8 val) {
  if (addr == 0x4016) {
    if (this->joy[0]) return this->joy[0]->write(addr, val);
    if (this->joy[1]) return this->joy[1]->write(addr, val);
  }

  fprintf(stderr,
    "[JOY] Should not be able to write to 0x%04X! Check MMU!\n", addr
  );
  assert(false);
}

void JOY::attach_joy(uint port, Memory* joy) {
  assert(port < 2);
  this->joy[port] = joy;
}

void JOY::detach_joy(uint port) {
  assert(port < 2);
  this->joy[port] = nullptr;
}
