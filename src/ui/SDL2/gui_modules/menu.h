#pragma once

#include <vector>

#include <cute_files.h>
#include <SDL_inprint2.h>
#include <SDL.h>

#include "module.h"
#include "emu.h"

#include "../config.h"

class MenuModule : public GUIModule {
private:
  SDL_Rect bg;
  SDL2_inprint* inprint;

  EmuModule& emu;

  char current_rom_file [256] = { 0 };

public:
  bool in_menu = true;
  struct {
    std::vector<cf_file_t> files;
    uint selected_i = 0;
    char directory [260] = ".";
    bool should_update_dir = true;
    struct {
      uint timeout = 0;
      char buf [16] = "\0";
      uint i = 0;
    } quicksel;
  } nav;

  struct {
    bool enter, up, down, left, right;
    char last_ascii;
  } hit = {0, 0, 0, 0, 0, 0};

public:
  virtual ~MenuModule();
  MenuModule(SharedState& gui, EmuModule& emu);

  void input(const SDL_Event&) override;
  void update() override;
  void output() override;

  int load_rom(const char* rompath);
  int unload_rom();

  uint get_window_id() override { return SDL_GetWindowID(this->emu.sdl.window); }
};
