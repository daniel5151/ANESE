#pragma once

#include <vector>
#include <map>

#include "config.h"

#include "shared_state.h"
#include "gui_modules/module.h"

#include "nes/cartridge/cartridge.h"
#include "nes/nes.h"
#include "nes/params.h"

class SDL_GUI final {
private:
  bool running = true;

  /*----------  Shared State  ----------*/

  std::map<std::string, GUIModule*> modules;
  GUIStatus status;
  Config config;
  SDL_Common sdl_common;
  NES_Params nes_params;
  NES* nes; // never null

  SharedState* shared; // passed to evey module

  /*----------  NES Support  ----------*/

  int speed_counter = 0;


private:
  void input_global(const SDL_Event&);

public:
  SDL_GUI(int argc, char* argv[]);
  ~SDL_GUI();

  int run();
};
