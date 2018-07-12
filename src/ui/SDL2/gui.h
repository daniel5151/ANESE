#pragma once

#include "config.h"

#include "gui_modules/module.h"

#include "gui_modules/emu.h"
#include "gui_modules/menu.h"
#include "gui_modules/widenes.h"

#include "nes/cartridge/cartridge.h"
#include "nes/nes.h"
#include "nes/params.h"

class SDL_GUI final {
private:
  bool running = true;

  /*----------  Shared State  ----------*/

  Config config;
  SDL_Common sdl_common;

  NES_Params nes_params;
  NES* nes; // never null
  Cartridge* cart = nullptr;

  SharedState* shared; // passed to modules

  /*----------  NES Support  ----------*/

  int speed_counter = 0;

  /*----------  Modules  ----------*/

  EmuModule*     emu     = nullptr;
  WideNESModule* widenes = nullptr;
  MenuModule*    menu    = nullptr;

private:
  void input_global(const SDL_Event&);

public:
  SDL_GUI(int argc, char* argv[]);
  ~SDL_GUI();

  int run();
};
