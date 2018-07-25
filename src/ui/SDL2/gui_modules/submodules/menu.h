#pragma once

#include <vector>

#include <cute_files.h>
#include <SDL_inprint2.h>
#include <SDL.h>

#include "submodule.h"

#include "../../config.h"

class MenuSubModule : public GUISubModule {
private:
  SDL_Rect bg;
  SDL2_inprint* inprint;

public:
  struct {
    std::vector<cf_file_t> files;
    uint selected_i = 0;
    char directory [260] = ".";
    bool should_update_dir = true;
    struct {
      uint timeout = 0;
      char buf [16] = "\0";
      uint i = 0;
    } quicksel;
  } nav;

  struct {
    bool enter, up, down, left, right;
    char last_ascii;
  } hit = {0, 0, 0, 0, 0, 0};

public:
  virtual ~MenuSubModule();
  MenuSubModule(SharedState& gui, SDL_Window* window, SDL_Renderer* renderer);

  void input(const SDL_Event&) override;
  void update() override;
  void output() override;
};
