#include "standard.h"

#include <cstdio>
#include <cassert>
#include <cstring>

JOY_Standard::JOY_Standard(const char* label)
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

void JOY_Standard::set_button(const char* btn, bool state) {
  fprintf(stderr,
    "[JOY_Standard][%s] %s : %s\n",
    this->label,
    btn,
    state ? "ON" : "OFF"
  );

  if (!strcmp("A",      btn)) { this->buttons.btn.A      = state; return; }
  if (!strcmp("B",      btn)) { this->buttons.btn.B      = state; return; }
  if (!strcmp("Start",  btn)) { this->buttons.btn.Start  = state; return; }
  if (!strcmp("Select", btn)) { this->buttons.btn.Select = state; return; }
  if (!strcmp("Up",     btn)) { this->buttons.btn.Up     = state; return; }
  if (!strcmp("Down",   btn)) { this->buttons.btn.Down   = state; return; }
  if (!strcmp("Left",   btn)) { this->buttons.btn.Left   = state; return; }
  if (!strcmp("Right",  btn)) { this->buttons.btn.Right  = state; return; }

  fprintf(stderr, "[JOY_Standard] Setting invalid button '%s'\n", btn);
  assert(false);
}
