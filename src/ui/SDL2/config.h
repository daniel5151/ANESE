#pragma once

#include <SimpleIni.h>

#include "common/util.h"

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
