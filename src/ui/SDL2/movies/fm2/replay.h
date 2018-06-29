#pragma once

#include "common/util.h"
#include "nes/interfaces/memory.h"

#include "nes/joy/controllers/standard.h"

#include "fm2_common.h"

// Parses .fm2 movies, and creates + updates joypads for playback
class FM2_Replay final {
private:
  struct {
    FM2_Controller::Type type;
    union {
      Memory* _mem;
      JOY_Standard* standard;
    //JOY_Zapper*   zapper;
    };
  } joy [3];

  // Original fm2 file data
  u8*  fm2;
  uint fm2_len;

  // current location in fm2 file
  const u8* curr_p;

  bool enabled;

  bool parse_fm2_header();

public:
  ~FM2_Replay();
  FM2_Replay();

  // Load FM2 from file
  // when lazy is true, file is lazily read (instead of fully loaded into mem)
  bool init(const char* filename, bool lazy = false);

  bool is_enabled() const;

  Memory* get_joy(uint port) const;

  void step_frame();
};
