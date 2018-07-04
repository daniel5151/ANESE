#include "standard.h"

#include <cstdio>
#include <cassert>
#include <cstring>

JOY_Standard::JOY_Standard(const char* label /* = "?" */) : label(label) {
  (void)this->label; // silence unused warning
}

u8 JOY_Standard::read(u16 addr) {
  (void) addr;

  if (!this->curr_btn || this->strobe)
    return 0x01;

  const u8 val = !!(this->buttons & this->curr_btn);
  this->curr_btn <<= 1;
  return val;
}

u8 JOY_Standard::peek(u16 addr) const {
  (void) addr;

  if (!this->curr_btn || this->strobe)
    return 0x01;

  const u8 val = !!(this->buttons & this->curr_btn);
  return val;
}

void JOY_Standard::write(u16 addr, u8 val) {
  (void) addr;

  this->strobe = val & 0x01;
  this->curr_btn = 0x01;
}

void JOY_Standard::set_button(JOY_Standard_Button::Type btn, bool active) {
  if (active) this->buttons |=  btn;
  else        this->buttons &= ~btn;
}

bool JOY_Standard::get_button(JOY_Standard_Button::Type btn) const {
  return this->buttons & btn;
}
