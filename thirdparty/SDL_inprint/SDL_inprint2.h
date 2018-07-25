#pragma once

#include <SDL.h>

class SDL2_inprint {
private:
  SDL_Renderer* selected_renderer = nullptr;
  SDL_Texture*  inline_font = nullptr;
  SDL_Texture*  selected_font = nullptr;
  Uint16 selected_font_w, selected_font_h;

public:
  SDL2_inprint(SDL_Renderer* renderer);
  ~SDL2_inprint();

  void set_color(Uint32 color); /* Color must be in 0x00RRGGBB format ! */
  void print(const char* str, Uint32 x, Uint32 y);
};
