#pragma once

#include <string>

#include <SDL.h>

#include "config.h"

#include "nes/cartridge/cartridge.h"
#include "nes/nes.h"
#include "nes/params.h"

struct SDL_Common {
  SDL_GameController* controller = nullptr;
};

struct GUIStatus {
  bool in_menu = true;
  uint avg_fps = 60;
};

struct SharedState {
  GUIStatus& status;

  SDL_Common& sdl;

  Config& config;

  NES_Params& nes_params;
  NES& nes;
  Cartridge*& cart;

  const Serializable::Chunk* savestate [4] = { nullptr };

  std::string current_rom_file;
  int load_rom(const char* rompath);
  int unload_rom();

  SharedState(
    GUIStatus& status,
    SDL_Common& sdl,
    Config& config,
    NES_Params& nes_params,
    NES& nes,
    Cartridge*& cart
  )
  : status(status)
  , sdl(sdl)
  , config(config)
  , nes_params(nes_params)
  , nes(nes)
  , cart(cart)
  {}
};
