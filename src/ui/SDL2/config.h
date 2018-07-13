#pragma once

#include <SimpleIni.h>

#include "common/util.h"

// Config Manager, both CLI parsing and INI parse/save
struct Config {
private:
  CSimpleIniA ini;
  char filename [260];
public:
  Config();
  void load(int argc, char* argv[]);
  void save();

public:
  /*----------  INI Config Args (saved)  ----------*/
  // UI
  uint window_scale = 2;
  // Paths
  char roms_dir [260] = { '.', '\0' };
  // I _would_ write  `char roms_dir [260] = "."`, but I can't.
  // Why? g++4 has a compiler bug, and Travis fails with that _valid_ syntax.

  /*----------  CLI Args (not saved)  ----------*/
  struct {
    bool log_cpu = false;
    bool no_sav  = false;
    bool ppu_timing_hack = false;

    bool ppu_debug = false;
    bool widenes = false;

    std::string record_fm2_path;
    std::string replay_fm2_path;

    std::string config_file;

    std::string rom;
  } cli;
};
