#pragma once

#include "common/util.h"
#include "nes/interfaces/memory.h"

#include "nes/joy/controllers/standard.h"

#include "fm2_common.h"

#include <cstdio>

// Records input into a fm2 file
class FM2_Record final {
private:
  // Doesn't own controllers!
  struct {
    FM2_Controller::Type type;
    union {
      Memory* _mem;
      JOY_Standard* standard;
    //JOY_Zapper*   zapper;
    };
  } joy [3];

  // fm2 file handle
  bool own_file;
  FILE* file;

  bool enabled;

  uint frame; // current frame

  void output_header();

public:
  ~FM2_Record();
  FM2_Record();

  bool init(const char* filename, bool binary = false);
  bool init(FILE* file, bool binary = false);

  void set_joy(uint port, FM2_Controller::Type type, Memory* joy);

  bool is_enabled() const;

  void step_frame();
};
