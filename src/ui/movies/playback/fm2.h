#pragma once

#include "common/util.h"
#include "nes/interfaces/memory.h"

#include "nes/joy/controllers/standard.h"

// Parses .fm2 movies, and creates + updates joypads for playback
class FM2_Playback_Controller final {
private:
  enum Type {
    SI_NONE    = 0,
    SI_GAMEPAD = 1,
  //SI_ZAPPER  = 2,
  };

  struct {
    Type type;
    union {
      Memory* memory;

      JOY_Standard* standard;
    //JOY_Zapper*   zapper;
    };
  } joy [3];

  // original fm2 file
  const char* fm2;
  uint        fm2_len;

  // current location in fm2 file
  const char* current_p;

  void parse_fm2_header();

public:
  ~FM2_Playback_Controller();
  FM2_Playback_Controller(const char* fm2, uint fm2_len);

  Memory* get_joy(uint port);

  void step_frame();
};
