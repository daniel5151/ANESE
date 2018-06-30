#pragma once

#include <string>
#include <vector>

#include "nes/cartridge/cartridge.h"
#include "nes/joy/controllers/standard.h"
#include "nes/joy/controllers/zapper.h"
#include "nes/nes.h"

#include "movies/fm2/record.h"
#include "movies/fm2/replay.h"

#include <SDL.h>
#include "util/Sound_Queue.h"

#include <cute_files.h>

#include "config.h"
#include "nes/params.h"

#include "gui_modules/module.h"
#include "gui_modules/emu.h"
#include "gui_modules/menu.h"

/**
 * Implementations are strewn-across multiple files, as having everything in a
 * single file became unwieldy.
 */

class SDL_GUI final {
private:
  // Command-line args
  CLIArgs args;

  // Global config
  Config config;

  // Initialized once, passed by const-ref to all gui modules
  SDLCommon sdl_common;

  /*----------  Modules  ----------*/

  EmuModule*  emu  = nullptr;
  MenuModule* menu = nullptr;

private:
  void input_global(const SDL_Event&);

public:
  ~SDL_GUI();

  int init(int argc, char* argv[]);
  int run();
};
