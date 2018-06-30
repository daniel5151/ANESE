#pragma once

#include "common/util.h"

struct NES_Params {
  uint apu_sample_rate; // in Hz
  uint speed;           // in %
  bool log_cpu;
  bool ppu_timing_hack;
};
