#include "standard.h"

#include <cstdio>
#include <cassert>
#include <cstring>

JOY_Standard::JOY_Standard(const char* label /* = "?" */)
: buttons {{0}}
, label(label)
{
  this->movie_buf[8] = '\0';
}

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
  using namespace JOY_Standard_Button;

  if (unsigned(btn) >= 8) {
    fprintf(stderr, "[JOY_Standard][%s] Setting invalid button '%d'!\n",
      this->label,
      btn
    );
    assert(false);
  }

  this->buttons.step[btn] = state;
}

const char* JOY_Standard::get_movie_frame() {
  #define OUTPUT(button, c, i) \
    this->movie_buf[i] = this->buttons.btn.button ? c : '.';
  OUTPUT(Right,  'R', 0);
  OUTPUT(Left,   'L', 1);
  OUTPUT(Down,   'D', 2);
  OUTPUT(Up,     'U', 3);
  OUTPUT(Start,  'T', 4);
  OUTPUT(Select, 'S', 5);
  OUTPUT(B,      'B', 6);
  OUTPUT(A,      'A', 7);
  #undef OUTPUT

  return this->movie_buf;
}
