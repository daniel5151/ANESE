#pragma once

#include <vector>

#include <SDL.h>
#include <cute_files.h>

#include "module.h"
#include "emu.h"

#include "../config.h"

class MenuModule : public GUIModule {
private:
  SDL_Rect bg;
  EmuModule& emu;

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
  MenuModule(const SDLCommon&, Config&, EmuModule&);

  void input(const SDL_Event&) override;
  void update() override;
  void output() override;

  uint get_window_id() override { return SDL_GetWindowID(this->emu.sdl.window); }
};
