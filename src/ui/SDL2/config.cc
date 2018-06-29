#include "config.h"

Config::Config() { this->ini.SetUnicode(); }

void Config::load(const char* filename) {
  // Try to load config, setting up a new one if none exists
  if (SI_Error err = this->ini.LoadFile(filename)) {
    fprintf(stderr, "[Config] Could not open config file %s!\n", filename);
    fprintf(stderr, "[Config]  Will generate a new one.\n");
    this->save(filename);
  }

  // Load config vals
  this->window_scale = this->ini.GetLongValue("ui", "window_scale");
  strcpy(this->roms_dir, this->ini.GetValue("paths", "roms_dir"));
}

void Config::save(const char* filename) {
  this->ini.SetLongValue("ui",    "window_scale", this->window_scale);
  this->ini.SetValue    ("paths", "roms_dir",     this->roms_dir);

  if (SI_Error err = this->ini.SaveFile(filename)) {
    fprintf(stderr, "[Config] Could not save config file %s!\n", filename);
  }
}
