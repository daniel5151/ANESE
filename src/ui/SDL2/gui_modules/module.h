#pragma once

#include <SDL.h>

#include "../config.h"

#include "nes/cartridge/cartridge.h"
#include "nes/nes.h"
#include "nes/params.h"

struct SDL_Common {
  SDL_GameController* controller = nullptr;
};

struct SharedState {
  SDL_Common& sdl;

  Config& config;

  NES_Params& nes_params;
  NES& nes;
  Cartridge*& cart;

  SharedState(
    SDL_Common& sdl,
    Config& config,
    NES_Params& nes_params,
    NES& nes,
    Cartridge*& cart
  )
  : sdl(sdl)
  , config(config)
  , nes_params(nes_params)
  , nes(nes)
  , cart(cart)
  {}
};

class GUIModule {
protected:
  SharedState& gui;

public:
  virtual ~GUIModule() = default;
  GUIModule(SharedState& gui)
  : gui(gui)
  {}

  virtual void input(const SDL_Event&) = 0;
  virtual void update() = 0;
  virtual void output() = 0;

  virtual uint get_window_id() = 0;
};
