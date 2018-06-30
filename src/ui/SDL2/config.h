#pragma once

#include <SimpleIni.h>

#include "common/util.h"

struct CLIArgs {
  bool log_cpu = false;
  bool no_sav  = false;
  bool ppu_timing_hack = false;

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
  char roms_dir [256] = { '.', '\0' };
  // I _would_ write  `char roms_dir [256] = "."`, but I can;t.
  // You know why?
  // because g++4 has a compiler bug, and Travis fails on it.
  // Weeeeeeeeeeeeeeeeee
};
