#include "record.h"

#include "nes/joy/controllers/standard.h"

#include <cassert>
#include <cstdio>
#include <cstring>

FM2_Record::~FM2_Record() {
  if (this->own_file) fclose(this->file);
}

FM2_Record::FM2_Record() {
  memset(&this->joy, 0, sizeof this->joy);

  this->own_file = false;
  this->file = nullptr;

  this->enabled = false;

  this->frame = 0;
}

bool FM2_Record::init(const char* filename, bool binary) {
  (void)binary; // TODO: support fm2 binary format

  this->own_file = false;
  this->file = fopen(filename, "w");

  this->enabled = bool(this->file);

  return this->enabled;
}

bool FM2_Record::init(FILE* file, bool binary) {
  (void)binary; // TODO: support fm2 binary format

  this->own_file = false;
  this->file = file;

  this->enabled = bool(this->file);

  return this->enabled;
}

void FM2_Record::set_joy(uint port, FM2_Controller::Type type, Memory* joy) {
  assert(port < 3);
  this->joy[port].type = type;
  this->joy[port]._mem = joy;
}

bool FM2_Record::is_enabled() const { return this->enabled; }

void FM2_Record::output_header() {
  // Output fm2 header
    for (uint port = 0; port < 3; port++) {
      fprintf(this->file, "port%u %u\n",
        port,
        uint(this->joy[port].type)
      );
    }
}

void FM2_Record::step_frame() {
  if (this->frame == 0) {
    this->output_header();
  }

  // Output control signal
  fprintf(this->file, "|%d", 0); // TODO: implement me

  // Output Controller Status
  for (uint port = 0; port < 3; port++) {
    switch (this->joy[port].type) {
    case FM2_Controller::SI_NONE: {
      fprintf(this->file, "|");
    } break;
    case FM2_Controller::SI_GAMEPAD: {
      using namespace JOY_Standard_Button;

      char buf [9];
      buf[8] = '\0';

      #define OUTPUT(i, btn, c) \
        buf[i] = this->joy[port].standard->get_button(btn) ? c : '.';
      OUTPUT(0, Right,  'R');
      OUTPUT(1, Left,   'L');
      OUTPUT(2, Down,   'D');
      OUTPUT(3, Up,     'U');
      OUTPUT(4, Start,  'T');
      OUTPUT(5, Select, 'S');
      OUTPUT(6, B,      'B');
      OUTPUT(7, A,      'A');
      #undef OUTPUT

      fprintf(this->file, "|%s", buf);
    } break;
    }
    // case FM2_Controller::SI_ZAPPER: {
    //   using namespace JOY_Zapper_Button;
    // } break;
  }

  fprintf(this->file, "\n");
  this->frame++;
}
