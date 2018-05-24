#include "standard.h"

#include <cstdio>
#include <cassert>
#include <cstring>

JOY_Standard::JOY_Standard(const char* label /* = "?" */)
: buttons {{0}}
, label(label)
{}

u8 JOY_Standard::read(u16 addr) {
  (void) addr;

  if (this->curr_btn > 7 || this->strobe)
    return 0x01;

  return this->buttons.step[this->curr_btn++] & 0x01;
}

u8 JOY_Standard::peek(u16 addr) const {
  (void) addr;

  if (this->curr_btn > 7 || this->strobe)
    return 0x01;

  return this->buttons.step[this->curr_btn] & 0x01;
}

void JOY_Standard::write(u16 addr, u8 val) {
  (void) addr;

  this->strobe = val & 0x01;
  this->curr_btn = 0;
}

void JOY_Standard::set_button(JOY_Standard_Button::Type btn, bool state) {
  if (unsigned(btn) >= 8) {
    fprintf(stderr, "[JOY_Standard][%s] Setting invalid button '%d'!\n",
      this->label,
      int(btn)
    );
    assert(false);
  }

  this->buttons.step[btn] = state;
}

bool JOY_Standard::get_button(JOY_Standard_Button::Type btn) const {
  if (unsigned(btn) >= 8) {
    fprintf(stderr, "[JOY_Standard][%s] Gettin invalid button '%d'!\n",
      this->label,
      int(btn)
    );
    assert(false);
  }

  return this->buttons.step[btn];
}
