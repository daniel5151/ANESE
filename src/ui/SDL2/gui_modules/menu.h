#pragma once

#include <vector>

#include <SDL.h>
#include "../config.h"

#include "module.h"
#include "emu.h"

#include <cute_files.h>

class MenuModule : public GUIModule {
private:
  SDL_Rect bg;
  EmuModule& emu_module;

public:
  bool in_menu = true;
  struct {
    std::vector<cf_file_t> files;
    char directory [256] = ".";
    bool should_update_dir = true;
    uint selected_i = 0;
    struct {
      uint timeout = 0;
      char buf [16] = "\0";
      uint i = 0;
    } quicknav;

    struct {
      bool enter, up, down, left, right;
      char last_ascii;
    } hit = {0, 0, 0, 0, 0, 0};
  } menu;

public:
  virtual ~MenuModule();
  MenuModule(const SDLCommon&, const CLIArgs&, Config&, EmuModule&);
  void input(const SDL_Event&) override;
  void update() override;
  void output() override;
};
