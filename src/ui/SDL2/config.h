#pragma once

#include <SimpleIni.h>

#include "common/util.h"

struct CLIArgs {
  bool log_cpu;
  bool no_sav;
  bool ppu_timing_hack;

  std::string record_fm2_path;
  std::string replay_fm2_path;

  std::string config_file;

  std::string rom;
};

struct Config {
private:
  CSimpleIniA ini;
public:
  Config();
  void load(const char* filename);
  void save(const char* filename);

public:
  // UI
  uint window_scale = 2;

  // Paths
  char roms_dir [256] = ".";
};
