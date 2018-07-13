#pragma once

#include <SDL.h>

#include "../../shared_state.h"

class GUISubModule {
protected:
  SharedState& gui;
  SDL_Window* window;
  SDL_Renderer* renderer;

public:
  virtual ~GUISubModule() = default;
  GUISubModule(SharedState& gui, SDL_Window* window, SDL_Renderer* renderer)
  : gui(gui)
  , window(window)
  , renderer(renderer)
  {}

  virtual void input(const SDL_Event&) = 0;
  virtual void update() = 0;
  virtual void output() = 0;
};
