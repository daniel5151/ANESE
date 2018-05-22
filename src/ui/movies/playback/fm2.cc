#include "fm2.h"

#include <cstdio>
#include <cassert>
#include <cstring>

FM2_Playback_Controller::~FM2_Playback_Controller() {
  delete this->joy[0].memory;
  delete this->joy[1].memory;

  delete[] this->fm2;
}

FM2_Playback_Controller::FM2_Playback_Controller(const char* fm2, uint fm2_len) {
  memset(&this->joy, 0, sizeof this->joy);

  // make a copy of the text
  this->fm2_len = fm2_len;
  this->fm2 = new char [fm2_len];
  memcpy(this->fm2, fm2, fm2_len);

  this->parse_fm2_header();
}

void FM2_Playback_Controller::parse_fm2_header() {
  char* s = this->fm2;
  while (*s && *s != '|') {
    // parse what to put in each port
    if (!memcmp("port", s, 4)) {
      const uint port = s[4] - '0';
      while (*s != ' ') s++;
      while (*s == ' ') s++;

      assert(this->joy[port].type == FM2_Playback_Controller::SI_NONE);

      this->joy[port].type = FM2_Playback_Controller::Type(*s - '0');
      switch (this->joy[port].type) {
        case SI_NONE: break;
        case SI_GAMEPAD: this->joy[port].standard = new JOY_Standard("fm2_0"); break;
      //case SI_ZAPPER:  this->joy[port].zapper   = new JOY_Zapper  ("fm2_0"); break;
      }
    }

    // Go to start of next line.
    while(*s++ != '\n');
  }
  this->current_p = s;
}

Memory* FM2_Playback_Controller::get_joy(uint port) {
  assert(port < 2);
  return this->joy[port].memory;
}

void FM2_Playback_Controller::step_frame() {
  if (!*this->current_p) return;

  if (*this->current_p != '|') {
    fprintf(stderr, "no support for binary fm2 yet!\n");
    assert(false);
  }

  // parse control code
  const uint control = this->current_p[1] - '0';
  this->current_p += 2;

  // ignore it for now
  (void)control;

  // parse port0, port1, and port2
  for (uint i = 0; i < 3; i++) {
    assert(*this->current_p = '|');

    if (this->joy[i].type == SI_NONE) {
      this->current_p++;
      continue;
    }

    // Otherwise, parse the controller
    if (this->joy[i].type == SI_GAMEPAD) {
      using namespace JOY_Standard_Button;
      // order: RLDUTSBA

      this->current_p++;

      #define is_pressed(c) ((c) != '.' && (c) != ' ')
      #define UPDATE(btn, c) \
        this->joy[i].standard->set_button(btn, is_pressed(this->current_p[c]));
      UPDATE(Right,  0);
      UPDATE(Left,   1);
      UPDATE(Down,   2);
      UPDATE(Up,     3);
      UPDATE(Start,  4);
      UPDATE(Select, 5);
      UPDATE(B,      6);
      UPDATE(A,      7);
      #undef is_pressed
      #undef UPDATE

      this->current_p += 8;
    }
  }

  // Go to start of next line
  while(*this->current_p && *this->current_p++ != '\n');
}
