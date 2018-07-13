#pragma once

#include <SDL.h>

#include "../../shared_state.h"

class GUISubModule {
protected:
  SharedState& gui;
  SDL_Renderer* renderer;

public:
  virtual ~GUISubModule() = default;
  GUISubModule(SharedState& gui, SDL_Renderer* renderer)
  : gui(gui)
  , renderer(renderer)
  {}

  virtual void input(const SDL_Event&);
  virtual void update();
  virtual void output();
};
