#pragma once

#include <SDL.h>
#include "../config.h"

struct SDLCommon {
  SDL_GameController* controller = nullptr;
};

class GUIModule {
protected:
  const SDLCommon& sdl_common;
  Config& config;
public:
  virtual ~GUIModule() = default;
  GUIModule(const SDLCommon& sdl_common, Config& config)
  : sdl_common(sdl_common)
  , config(config)
  {}

  virtual void input(const SDL_Event&) = 0;
  virtual void update() = 0;
  virtual void output() = 0;

  virtual uint get_window_id() = 0;
};
