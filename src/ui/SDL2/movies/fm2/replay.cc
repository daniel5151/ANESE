#include "replay.h"

#include "ui/SDL2/fs/load.h"

#include <cstdio>
#include <cassert>
#include <cstring>

FM2_Replay::~FM2_Replay() {
  delete this->joy[0]._mem;
  delete this->joy[1]._mem;
  delete this->joy[2]._mem;

  delete this->fm2;
}

FM2_Replay::FM2_Replay() {
  memset(&this->joy, 0, sizeof this->joy);

  this->fm2     = nullptr;
  this->fm2_len = 0;

  this->curr_p = nullptr;

  this->enabled = false;
}

bool FM2_Replay::init(const char* filename, bool lazy) {
  (void)lazy; // TODO: lazy-loading FM2

  ANESE_fs::load::load_file(filename, this->fm2, this->fm2_len);

  bool did_parse = this->parse_fm2_header();

  this->enabled = did_parse;

  return did_parse;
}

bool FM2_Replay::parse_fm2_header() {
  if (!this->fm2) {
    fprintf(stderr, "[Replay][fm2] fm2 data is null!\n");
    return false;
  }

  const u8* s = this->fm2;
  while (*s && *s != '|') {
    // parse what to put in each port
    if (!memcmp("port", s, 4)) {
      const uint port = s[4] - '0';
      while (*s != ' ') s++;
      while (*s == ' ') s++;

      assert(this->joy[port].type == FM2_Controller::SI_NONE);

      this->joy[port].type = FM2_Controller::Type(*s - '0');
      switch (this->joy[port].type) {
        using namespace FM2_Controller;
        case SI_NONE: break;
        case SI_GAMEPAD: this->joy[port].standard = new JOY_Standard("fm2"); break;
      //case SI_ZAPPER:  this->joy[port].zapper   = new JOY_Zapper  ("fm2"); break;
      }
    }

    // Go to start of next line.
    while(*s++ != '\n');
  }
  this->curr_p = s;

  return true;
}

Memory* FM2_Replay::get_joy(uint port) const {
  assert(port < 2);
  return this->joy[port]._mem;
}

bool FM2_Replay::is_enabled() const { return this->enabled; }

void FM2_Replay::step_frame() {
  if (!this->enabled) return;
  if (!*this->curr_p) {
    fprintf(stderr, "[Replay][fm2] Reached end of movie.\n");
    this->enabled = false;
    return;
  }

  if (*this->curr_p != '|') {
    fprintf(stderr, "[Replay][fm2] no support for binary fm2 yet!\n");
    assert(false);
  }

  // parse control code
  const uint control = this->curr_p[1] - '0';
  this->curr_p += 2;

  // ignore it for now
  (void)control;

  // parse port0, port1, and port2
  for (uint i = 0; i < 3; i++) {
    assert(*this->curr_p == '|');

    if (this->joy[i].type == FM2_Controller::SI_NONE) {
      this->curr_p++;
      continue;
    }

    // Otherwise, parse the controller
    if (this->joy[i].type == FM2_Controller::SI_GAMEPAD) {
      using namespace JOY_Standard_Button;
      // order: RLDUTSBA

      this->curr_p++;

      #define is_pressed(c) ((c) != '.' && (c) != ' ')
      #define UPDATE(btn, c) \
        this->joy[i].standard->set_button(btn, is_pressed(this->curr_p[c]));
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

      this->curr_p += 8;
    }
  }

  // Go to start of next line
  while(*this->curr_p && *this->curr_p++ != '\n');
}
