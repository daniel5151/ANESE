#pragma once

#include <SDL.h>

#include "../shared_state.h"

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
