#pragma once

#include <SDL.h>

#include "../config.h"
#include "module.h"

#include "emu.h"

#include <map>

class WideNESModule : public GUIModule {
private:
  struct {
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
  } sdl;

  struct Tile {
    struct {
      int x, y;
    } pos; // position within tile-grid

    SDL_Texture* texture = nullptr;
    u8 framebuffer [256 * 240 * 4];

    ~Tile();
    Tile(SDL_Renderer* renderer, int x, int y);
  };

  // tilemap
  std::map<int, std::map<int, Tile*>> tiles;

  // not part of the tilemap, it's just convenient to have a copy of the current
  // nes screen
  Tile* nes_screen;

  // used to calculate dx and dy b/w frames
  struct { u8 x; u8 y; } last_scroll { 0, 0 };

  // total scroll (offset from origin)
  struct { int x; int y; int dx; int dy; } scroll { 0, 0, 0, 0 };

  // zoom/pan info
  struct {
    bool active = false;
    struct { int x; int y; } last_mouse_pos { 0, 0 };
    int dx = 0;
    int dy = 0;
    float zoom = 2.0;
  } pan;

  void samplePPU(EmuModule& emu);
  static void frame_callback(void* self, EmuModule& emu);

public:
  virtual ~WideNESModule();
  WideNESModule(const SDLCommon& sdl_common, Config& config, EmuModule& emu);
  void input(const SDL_Event&) override;
  void update() override;
  void output() override;
};
