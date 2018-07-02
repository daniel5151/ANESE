#pragma once

#include <SDL.h>
#include "../config.h"

struct SDLCommon {
  bool running = true;

  static constexpr uint SAMPLE_RATE = 96000;

  static constexpr uint RES_X = 256;
  static constexpr uint RES_Y = 240;
  static constexpr uint SCREEN_SCALE = 2; // internal screen scale

  SDL_Renderer* renderer = nullptr;
  SDL_Window*   window   = nullptr;

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
};
